/*
 * Copyright (C) 2012--2015 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/sprd_ion.h>
#include <linux/mfd/syscon.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/sprd_iommu.h>
#include <linux/sprd_ion.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/notifier.h>
#include <linux/compat.h>
#include "sprd_vdsp.h"
#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/crc32.h>
#include <linux/dma-mapping.h>
#include "sprd_vdsp_cmd.h"

#define VDSP_MINOR MISC_DYNAMIC_MINOR
#define PAGE_SIZE_4K 0x1000
/* VDSP Log buffer size*/
#define LOG_BUFFER_SIZE	  0x1000
/* Log task buffer size*/
#define BUFFER_SIZE 220

/* Share-Memory Buff offset definition*/

#define VDSP_SHARE_BUFF_CODE_OFFSET	0x0
#define VDSP_SHARE_BUFF_EXT_CODE_OFFSET	0x10000
#define VDSP_SHARE_BUFF_EXT_DATA_OFFSET	0x50000
#define VDSP_SHARE_BUFF_DATA_OFFSET	0x90000
#define VDSP_SHARE_BUFF_CMD_OFFSET	0xd0000
#define VDSP_SHARE_BUFF_LOG_POS_OFFSET	0xd1000
#define VDSP_SHARE_BUFF_LOG_OFFSET	0xd2000

LIST_HEAD(vdsp_core_head);
LIST_HEAD(vdsp_clk_head);
LIST_HEAD(vdsp_glb_head);
static bool need_load_fw = 1;
static int get_xm6log_flag = 0;
u32 vdsp_share_buff_addr;
extern int xm6_state_changed;
static struct vdsp_dev_t vdsp_hw_dev;
static bool evt_update;
void *g_kva_vdsp_share_buff_addr;
extern struct class *vdsp_class;


//static struct task_struct *log_task;
module_param(need_load_fw, bool, 0644);
module_param(get_xm6log_flag, int, 0644);

extern int vdsp_edp_iommu_map(void);
extern int sprd_vdsp_sysfs_init(struct device *dev);


int vdsp_get_ionbuf(struct vdsp_buf *pfinfo)
{
	int i = 0, ret = 0;

	for (i = 0; i < 2; i++) {
		if (pfinfo->mfd[i] > 0) {
			ret = sprd_ion_get_buffer(pfinfo->mfd[i],
						NULL,
						&pfinfo->buf[i],
						&pfinfo->size[i]);
			if (ret) {
				pr_err("fail to get ion buffer\n");
				return -EFAULT;
			}
			//consider dma_buffer handle
		}
	}
	return 0;
}

int vdsp_iommu_map(struct vdsp_buf *pfinfo)
{
	int i = 0, ret = 0;
	struct sprd_iommu_map_data iommu_data;

	memset(&iommu_data, 0x00, sizeof(iommu_data));

	for (i = 0; i < 2; i++) {
		if (pfinfo->size[i] <= 0)
			continue;

		if (sprd_iommu_attach_device(pfinfo->dev) == 0) {
			memset(&iommu_data, 0x00, sizeof(iommu_data));
			iommu_data.buf = pfinfo->buf[i];
			iommu_data.iova_size = pfinfo->size[i];
			iommu_data.ch_type = SPRD_IOMMU_FM_CH_RW;
			iommu_data.sg_offset = pfinfo->offset[i];

			ret = sprd_iommu_map(pfinfo->dev, &iommu_data);
			if (ret) {
				pr_err("fail to get iommu kaddr %d\n", i);
				return -EFAULT;
			}

			pfinfo->iova[i] = iommu_data.iova_addr
					+ pfinfo->offset[i];
		} else {
			ret = sprd_ion_get_phys_addr(pfinfo->mfd[i],
					NULL,
					&pfinfo->iova[i],
					&pfinfo->size[i]);
			if (ret) {
				pr_err("fail to get iommu phy addr %d mfd 0x%x\n",
				       i, pfinfo->mfd[i]);
				return -EFAULT;
			}
			pfinfo->iova[i] += pfinfo->offset[i];
		}
	}

	return 0;
}

int vdsp_iommu_unmap(struct vdsp_buf *pfinfo)
{
	int i, ret;
	struct sprd_iommu_unmap_data iommu_data;

	memset(&iommu_data, 0x00, sizeof(iommu_data));

	for (i = 0; i < 2; i++) {
		if (pfinfo->size[i] <= 0)
			continue;

		if (sprd_iommu_attach_device(pfinfo->dev) == 0) {
			iommu_data.iova_addr = pfinfo->iova[i]
					- pfinfo->offset[i];
			iommu_data.iova_size = pfinfo->size[i];
			iommu_data.ch_type = SPRD_IOMMU_FM_CH_RW;
			iommu_data.buf = NULL;

			ret = sprd_iommu_unmap(pfinfo->dev,
				&iommu_data);
			if (ret) {
				pr_err("failed to free iommu %d\n", i);
				return -EFAULT;
			}
		}
	}

	return 0;
}

int32_t vdsp_wait_xm6_done(void)
{
	int rc;

	/* if this function was called more than once without */
	if (!vdsp_hw_dev.is_opened) {
		VDSP_INFO("vdsp has not opened!\n");
		return 0;
	}

	/*wait for stop done interrupt*/
	rc = wait_event_interruptible_timeout(vdsp_hw_dev.wait_queue, evt_update,
					       msecs_to_jiffies(500));
	evt_update = false;

	if (!rc) {
		/* time out */
		VDSP_ERROR("vdsp wait for ceva done time out!\n");
		vdsp_hw_dev.is_opened = true;
		return -1;
	}
	get_xm6log_flag = 1;
	VDSP_DEBUG("get_xm6log_flag=%d\n",get_xm6log_flag);
	return 0;
}

void send_msg_to_share_memory(struct sprd_dsp_cmd *node)
{
	struct sprd_dsp_cmd* msg_queue = (struct sprd_dsp_cmd *)(g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_CMD_OFFSET);
	memcpy(msg_queue, node, sizeof(struct sprd_dsp_cmd));
      //  __dma_flush_area(msg_queue,sizeof(struct sprd_dsp_cmd));
	VDSP_DEBUG("send_msg_to_vdsp \n");
	//disable cxrcvr,no send recovery signal when icu_intn is activated
	//*(volatile u32*)(0x20800064) &= 0xfffffff0;
	if (vdsp_hw_dev.core && vdsp_hw_dev.core->isr_triggle_int0)
		vdsp_hw_dev.core->isr_triggle_int0(&vdsp_hw_dev.ctx, 0);
}

static void msg_post_work_func(struct kthread_work *work)
{
	struct msg_pending_post *post, *next;
	struct list_head saved_list;
	mutex_lock(&vdsp_hw_dev.post_lock);
	memcpy(&saved_list, &vdsp_hw_dev.post_list, sizeof(saved_list));
	list_replace_init(&vdsp_hw_dev.post_list, &saved_list);
	mutex_unlock(&vdsp_hw_dev.post_lock);

	list_for_each_entry_safe(post, next, &saved_list, head) {

		VDSP_DEBUG("msg_post_work_func entry\n");
		send_msg_to_share_memory(&post->msg);
		xm6_state_changed = 1;
		vdsp_wait_xm6_done();
		VDSP_DEBUG("msg_post_work_func  end\n");
		list_del(&post->head);
		kfree(post);
	}
	// xm6 enter stand-by mode
	if (vdsp_hw_dev.core && vdsp_hw_dev.core->isr_triggle_int1)
		vdsp_hw_dev.core->isr_triggle_int1(&vdsp_hw_dev.ctx, 1);
}

int vdsp_cmd_convertor(
	struct vdsp_dev_t *vdsp_hw_dev,
	struct sprd_dsp_cmd *msg)
{
	int ret = 0;
	int i = 0;
	struct vdsp_buf *p_in_buf = &vdsp_hw_dev->ion_in_buf;
	struct vdsp_buf *p_out_buf = &vdsp_hw_dev->ion_out_buf;
	struct vdsp_buf *p_dsp_buf = &vdsp_hw_dev->ion_dsp_buf;

	pr_err("vdsp_cmd_convertor entry\n");
//in_data addr		
		if (msg->in_data_fd > 0) {
			p_in_buf->mfd[0] = msg->in_data_fd;
			p_in_buf->dev = &vdsp_hw_dev->dev;
			ret = vdsp_get_ionbuf(p_in_buf);
				if (ret) {
					pr_err("fail to get in_ion_buf\n");
					 return -1;
				}
				ret = vdsp_iommu_map(p_in_buf);
				if (ret) {
				pr_err("fail to get in_data addr!!!!\n");
				  return -1;
				}
				msg->in_data_addr = (uint32_t)p_in_buf->iova[0];
			}
		//out_data addr
		if (msg->out_data_fd > 0) {
			p_out_buf->mfd[0] = msg->out_data_fd;
			p_out_buf->dev = &vdsp_hw_dev->dev;
			ret = vdsp_get_ionbuf(p_out_buf);
				if (ret) {
					pr_err("fail to get  ion_out_buf\n");
					  return -1;
				}
				ret = vdsp_iommu_map(p_out_buf);
				if (ret) {
				pr_err("fail to get out_data addr!!!!\n");
				 return -1;
				}
				msg->out_data_addr = (uint32_t)p_out_buf->iova[0];
			}
		//bufer addr
		
		for (i=0;i< 4;i++) {
			if (msg->buffer_data[i].fd > 0) {
					p_dsp_buf->mfd[0] = msg->buffer_data[i].fd;
					p_dsp_buf->dev = &vdsp_hw_dev->dev;
					ret = vdsp_get_ionbuf(p_dsp_buf);
				if (ret) {
					pr_err("fail to get  ion_des_buf\n");
					  return -1;
				}
				ret = vdsp_iommu_map(p_dsp_buf);
				if (ret) {
				pr_err("fail to get dsp addr[%d]!!!!\n", i);
				 return -1;
				}
				msg->buffer_data[i].addr = (uint32_t)p_dsp_buf->iova[0];
				}
		}
	pr_err("vdsp_cmd_convertor end msg->in_data_addr:%d, msg->out_data_addr:%d msg->buffer_data[].addr:%d;%d;%d\n",
		msg->in_data_addr,
		msg->out_data_addr,
		msg->buffer_data[0].addr,
		msg->buffer_data[1].addr,
		msg->buffer_data[2].addr
		);
	return ret;
}

#if 0
int sprd_vdsp_buf_alloc(struct vdsp_dev_t *vdsp_hw_dev,
			    int heap_type, size_t *size, u32 *buffer)
{
#if 0
	struct device *parent = NULL;
	struct page *page;
	dma_addr_t dma_handle;
	g_kva_vdsp_share_buff_addr = (void *)__get_free_pages(GFP_DMA32 |
			GFP_ATOMIC | __GFP_ZERO, get_order(*size));
	parent = vdsp_hw_dev->dev.parent;
	page = virt_to_page(g_kva_vdsp_share_buff_addr);

	dma_handle = dma_map_page(parent, page, 0, *size,
					DMA_TO_DEVICE);
	if (dma_mapping_error(parent, dma_handle)) {
		VDSP_ERROR("dma map failed for address %p\n", g_kva_vdsp_share_buff_addr);
		free_pages((unsigned long)g_kva_vdsp_share_buff_addr, get_order(*size));
		return -1;
	}
	dma_sync_single_for_device(parent, dma_handle,
				*size, DMA_TO_DEVICE);
	*buffer = dma_handle;
	VDSP_INFO("logo_dst_p:0x%x, logo_dst_v:0x%p\n", (u32)dma_handle, g_kva_vdsp_share_buff_addr);
#else
	int fd = 0;
	struct dma_buf *dmabuf = NULL;
	unsigned long phyaddr;
	int ret = 0;

	fd = ion_alloc((size_t)(*size), ION_HEAP_ID_MASK_FB, 0);
	if (fd < 0) {
		pr_emerg("ion_alloc buffer fail(fd=%d)\n", fd);
		return fd;
	}
	pr_emerg("fd:%d\n",fd);
	dmabuf = dma_buf_get(fd);
	ret = sprd_ion_get_phys_addr(fd, dmabuf, &phyaddr, size);
	*buffer = (u32)phyaddr;
	pr_emerg("test3 allocated for vdsp buffer:0x%x, type %d\n",
	      (u32)phyaddr, ION_HEAP_ID_MASK_FB);

	pr_emerg("jun============\n");
	pr_emerg("vdsp_epp_probe call1(dmabuf =%p)\n", dmabuf);
	g_kva_vdsp_share_buff_addr = sprd_ion_map_kernel(dmabuf, 0);
	pr_emerg("vdsp_epp_probe call2(kvaddr=%p)\n", g_kva_vdsp_share_buff_addr);
	dma_buf_put(dmabuf);
	pr_emerg("vdsp_epp_probe call3(dmabuf =%p)\n", dmabuf);
	VDSP_INFO("logo_dst_p:0x%x, logo_dst_v:0x%p\n", (u32)*buffer, g_kva_vdsp_share_buff_addr);
#endif
	return 0;
}
#endif

unsigned int src_buf[2*1024];
static int sprd_vdsp_buf_alloc_2(struct vdsp_dev_t *vdsp_hw_dev,
			    int heap_type, size_t *size, u32 *buffer)
{
		int ret = 0;
		int i =0;
		unsigned long phyaddr;
		struct camera_buf buf;

		buf.dmabuf_p[0] = ion_new_alloc((size_t)(*size), heap_type, 0);
			if (IS_ERR_OR_NULL(buf.dmabuf_p[0])) {
					pr_emerg("failed to alloc ion buf size 0x%x \n", (u32)(*size));
					ret = -1;
					return ret;
			}
	pr_emerg("sprd_vdsp_buf_alloc_2 alloc buffer successed");

	ret = sprd_ion_get_buffer(-1,
			buf.dmabuf_p[0],
			&buf.ionbuf[0],
			&buf.size[0]);
	if (ret) {
		pr_emerg("failed to get ionbuf for kernel buffer %p\n", buf.dmabuf_p[0]);
		ret = -1;
		goto failed;
	}		

	ret = sprd_ion_get_phys_addr(-1, buf.dmabuf_p[0], &phyaddr, size);
		if (ret) {
				pr_err("failed to get phyaddr %d\n", (u32)phyaddr);
				ret = -1;
				goto failed;
			}
	*buffer = (u32)phyaddr;
	pr_emerg("sprd_vdsp_buf_alloc_2 allocated for vdsp buffer:0x%x, type %d\n",
	      (u32)phyaddr, ION_HEAP_ID_MASK_FB);
	
	buf.addr_k[0] = (unsigned long)sprd_ion_map_kernel(buf.dmabuf_p[0], 0);
	if (IS_ERR_OR_NULL((void *)buf.addr_k[0])) {
		pr_err("fail to map k_addr for dmabuf[%p]\n", buf.dmabuf_p[0]);
		ret = -1;
		goto map_fail;
	}
	pr_emerg("get kernel vaddr successed (kvaddr=%p)\n", (void *)buf.addr_k[0]);
	
	for(i=0;i<100;i++){
		unsigned int * p;
		p = (unsigned int *)buf.addr_k[0];
		p = p + i;
		pr_info("vvv:b:data<%d>[%d]\n",i,*p);
	}

	for (i=0;i<2*1024;i++){
		src_buf[i] = i;
	}
	
	memcpy((void *)buf.addr_k[0],(void *)src_buf,2*1024);

	for(i=0;i<100;i++){
		unsigned int * p;
		p = (unsigned int *)buf.addr_k[0];
		p = p + i;
		pr_info("vvv:e:data<%d>[%d]\n",i,*p);
	}

	g_kva_vdsp_share_buff_addr = (void *)buf.addr_k[0];

	pr_emerg("g_kva_vdsp_share_buff_addr (kvaddr=%p)\n", g_kva_vdsp_share_buff_addr);

	return ret;
	
map_fail:
	sprd_ion_unmap_kernel(buf.dmabuf_p[0], 0);
failed:
	ion_free(buf.dmabuf_p[0]);
	buf.dmabuf_p[0] = NULL;
	buf.size[0] = 0;

	return ret;
	
}

static u32 overwrite_firmware(struct vdsp_dev_t *vdsp_hw_dev)
{
	u32 err = 0;
	const struct firmware *fw;
	u32 src_crc = 0, dst_crc = 0;
	u32 src_data_crc = 0, dst_data_crc = 0;
	u32 copy_times = 0;
	VDSP_DEBUG("overwrite_firmware called !\n");
	/* TO DO ---- Upload firmware.*/
	if (request_firmware
		(&fw, "test_vdsp00.bin", &vdsp_hw_dev->dev)) {
		pr_emerg("get firmware failed.");
		err = EINVAL;
		goto overwrite_firmware_end;
	}
	VDSP_DEBUG(
		"g_kva_vdsp_share_buff_addr: %p, fw->data: 0x%p, fw->size: 0x%lx",
		g_kva_vdsp_share_buff_addr, fw->data, fw->size);

	src_crc = crc32(0xffffeeee, fw->data, fw->size);
	if (fw->size != ALIGN(fw->size, 32))
		vdsp_hw_dev->fw_code_size = ALIGN(fw->size, 32) - 32 ;
	else
		vdsp_hw_dev->fw_code_size = fw->size;
	VDSP_DEBUG("crc32 magic: 0x%x, 0x%x\n", 0xffffeeee, src_crc);

	while (src_crc != dst_crc) {
		memcpy(g_kva_vdsp_share_buff_addr,
			fw->data, fw->size);
	//	__dma_flush_area(g_kva_vdsp_share_buff_addr,
	//		fw->size);
		dst_crc = crc32(0xffffeeee, g_kva_vdsp_share_buff_addr, fw->size);
		copy_times++;
		pr_emerg("src_crc: 0x%x, dst_crc: 0x%x, times: %d\n",
			src_crc, dst_crc, copy_times);
 		if (copy_times >= 10)
			break;
 	}

	VDSP_DEBUG("firmware load code memory ok.\n");
	release_firmware(fw);

	if (request_firmware
		(&fw, "test_vdsp10.bin", &vdsp_hw_dev->dev)) {
		pr_emerg("get firmware failed.");
		err = EINVAL;
		goto overwrite_firmware_end;
	}
	VDSP_DEBUG(
		"g_kva_vdsp_share_buff_addr+VDSP_SHARE_BUFF_EXT_CODE_OFFSET: %p, fw->data: 0x%p, fw->size: 0x%lx",
		g_kva_vdsp_share_buff_addr+ VDSP_SHARE_BUFF_EXT_CODE_OFFSET, fw->data, fw->size);

	src_data_crc = crc32(0xffffeeee, fw->data, fw->size);
	if (fw->size != ALIGN(fw->size, 32))
		vdsp_hw_dev->fw_ext_code_size = ALIGN(fw->size, 32) - 32;
	else
		vdsp_hw_dev->fw_ext_code_size = fw->size;
        VDSP_DEBUG("crc32 magic: 0x%x, 0x%x\n", 0xffffeeee, src_data_crc);

	while (src_data_crc != dst_data_crc) {
		memcpy(g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_EXT_CODE_OFFSET,
			fw->data, fw->size);
		//__dma_flush_area(g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_EXT_CODE_OFFSET,
		//	fw->size);
		dst_data_crc = crc32(0xffffeeee, g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_EXT_CODE_OFFSET, fw->size);
		copy_times++;
		VDSP_DEBUG("src_data_crc: 0x%x, dst_data_crc: 0x%x, times: %d\n",
		src_data_crc, dst_data_crc, copy_times);
		if (copy_times >= 10)
		break;
	}
	VDSP_DEBUG("firmware load ext code memory ok.\n");
	release_firmware(fw);

	if (request_firmware
		(&fw, "test_vdsp1d0.bin", &vdsp_hw_dev->dev)) {
		pr_emerg("get firmware failed.");
		err = EINVAL;
		goto overwrite_firmware_end;
	}
	VDSP_DEBUG(
		"g_kva_vdsp_share_buff_addr+VDSP_SHARE_BUFF_EXT_DATA_OFFSET: %p, fw->data: 0x%p, fw->size: 0x%lx",
		g_kva_vdsp_share_buff_addr+ VDSP_SHARE_BUFF_EXT_DATA_OFFSET, fw->data, fw->size);

	src_data_crc = crc32(0xffffeeee, fw->data, fw->size);
	if (fw->size != ALIGN(fw->size, 32))
		vdsp_hw_dev->fw_ext_data_size = ALIGN(fw->size, 32) - 32;
	else
		vdsp_hw_dev->fw_ext_data_size = fw->size;
        VDSP_DEBUG("crc32 magic: 0x%x, 0x%x\n", 0xffffeeee, src_data_crc);

	while (src_data_crc != dst_data_crc) {
		memcpy(g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_EXT_DATA_OFFSET,
			fw->data, fw->size);
	//	__dma_flush_area(g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_EXT_DATA_OFFSET,
	//		fw->size);
		dst_data_crc = crc32(0xffffeeee, g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_EXT_DATA_OFFSET, fw->size);
		copy_times++;
		VDSP_DEBUG("src_data_crc: 0x%x, dst_data_crc: 0x%x, times: %d\n",
			src_data_crc, dst_data_crc, copy_times);
		if (copy_times >= 10)
			break;
	}
	VDSP_DEBUG("firmware load ext data memory ok.\n");
	release_firmware(fw);

	if (request_firmware
		(&fw, "test_vdsp0d0.bin", &vdsp_hw_dev->dev)) {
		pr_emerg("get firmware failed.");
		err = EINVAL;
		goto overwrite_firmware_end;
	}
	VDSP_DEBUG(
		"g_kva_vdsp_share_buff_addr+VDSP_SHARE_BUFF_DATA_OFFSET %p, fw->data: 0x%p, fw->size: 0x%lx",
		g_kva_vdsp_share_buff_addr+VDSP_SHARE_BUFF_DATA_OFFSET, fw->data, fw->size);

	src_crc = crc32(0xffffeeee, fw->data, fw->size);
	if (fw->size != ALIGN(fw->size, 32))
		vdsp_hw_dev->fw_data_size = ALIGN(fw->size, 32) - 32 ;
	else
		vdsp_hw_dev->fw_data_size = fw->size;
	VDSP_DEBUG("crc32 magic: 0x%x, 0x%x\n", 0xffffeeee, src_crc);

	while (src_crc != dst_crc) {
		memcpy(g_kva_vdsp_share_buff_addr+VDSP_SHARE_BUFF_DATA_OFFSET,
			fw->data, fw->size);
	//	__dma_flush_area(g_kva_vdsp_share_buff_addr+VDSP_SHARE_BUFF_DATA_OFFSET,
	//		fw->size);
		dst_crc = crc32(0xffffeeee, g_kva_vdsp_share_buff_addr+VDSP_SHARE_BUFF_DATA_OFFSET, fw->size);
		copy_times++;
		pr_emerg("src_crc: 0x%x, dst_crc: 0x%x, times: %d\n",
			src_crc, dst_crc, copy_times);
		if (copy_times >= 10)
			break;
	}
	VDSP_DEBUG("firmware load data memory ok.\n");
	release_firmware(fw);

	need_load_fw = 0;
overwrite_firmware_end:
        return err;
}

/*
 *\brief
 *\param ahb_buf_size [in]
 *\param cur_offset [in] print log until this position
 *\param p_pre_offset [in] start position
 */
static void print_xm6_log(const u32 ahb_buf_size,
		u32 cur_offset, u32 *p_pre_offset)
{
	char output_str[BUFFER_SIZE];
	const char *delim = "\n";
	char *token;
	char *p;
	u32 offset = 0, offset2 = 0;
	u32 copy_size = 0;
	u8 *buff_ptr;

	VDSP_INFO("*p_pre_offset=%d,cur_offset=%d\n", *p_pre_offset,cur_offset);
	if (cur_offset == 0 || cur_offset >= ahb_buf_size) {/* out of range */
		VDSP_INFO("print_xm6_log out of range\n");
		return;
	}
	if (cur_offset == *p_pre_offset) {/* no update */
		VDSP_INFO("print_xm6_log no update\n");
		return;
	}

	/* remove the start, end tags to reduce the number of logs */
	if (*p_pre_offset < cur_offset) /* new log added */
		offset = *p_pre_offset;
	while (offset < cur_offset) {
		buff_ptr =  (u8 *) (g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_LOG_OFFSET);
		if (!buff_ptr) {
			VDSP_ERROR("fw log vdsp closed: %p", buff_ptr);
			break;
		}
		//pr_emerg("offset=%d,cur_offset=%d\n", offset,cur_offset);
		copy_size = cur_offset - offset;
		if (copy_size > BUFFER_SIZE)
			copy_size = BUFFER_SIZE;
		memset(output_str, 0, sizeof(output_str));
		/* Force to turn on firmware log for debugging */
		memcpy(output_str, (buff_ptr + offset), copy_size);
		//pr_emerg("output_str: %s\n", output_str);
		p = output_str;
		offset2 = 0;
		do {
			token = strsep(&p, delim);
			//VDSP_INFO("fw logout: %s\n", output_str);
			if (p != NULL) {
				offset2 = (p - output_str);
				VDSP_INFO("fw log %s\n", token);
			} else if (offset2 == 0) {
				/* can not find a complete line in the buffer.
				 * just print it out. (include the last time)
				 */
				VDSP_INFO("fw log %s\n", output_str);
				offset2 = copy_size;
			} else if ((offset + copy_size) >= cur_offset) {
				/* the last time, print the last token */
				if (token != NULL && strlen(token) > 0)
					VDSP_INFO("fw log %s\n", token);
				offset2 = copy_size;
			}
			/* Others are strings which is possibly truncated
			 * rather than a complete whole line when p is null.
			 */
#if 0
			pr_emerg("pos: %p, offset: %d %d, token: %s, len:%zd\n",
					p, offset, offset2,
					token, strlen(token));
#endif
		} while (p != NULL);
		offset += offset2;
	}
	*p_pre_offset = cur_offset;

	return;
}

static int log_task_func(void *data)
{
	u32 err = 0;
	u32 previous_offset = 0, current_offset = 0;
	u32 *p = (void *) (g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_LOG_POS_OFFSET);
	do {
		if (get_xm6log_flag == 0) {
			usleep_range(1000000, 1000000);
			continue;
		}

		/* polling the reg*/
		usleep_range(250, 255);

		current_offset = p[0];
		print_xm6_log(PAGE_SIZE_4K, current_offset, &previous_offset);
		get_xm6log_flag = 0;
	} while (!kthread_should_stop());


	return err;
}
static long vdsp_ioctl(struct file *filp, unsigned int cmd, unsigned long param)
{
	int ret = 0;
	struct sprd_dsp_cmd msg;
	struct msg_pending_post* cfg;
	VDSP_DEBUG("vdsp_ioctl called !\n");
	switch (cmd) {
	case SPRD_VDSP_IO_VERSION:
		VDSP_DEBUG("vdsp version -enter\n");
		put_user(vdsp_hw_dev.version, (int __user *)param);

		break;
	case SPRD_VDSP_IO_LOAD_FW:
		VDSP_DEBUG("vdsp_ioctl  VDSP_IO_LOAD_FW called !\n");
		ret = overwrite_firmware(&vdsp_hw_dev);
		if (ret != 0)
			VDSP_ERROR("vdsp_load_firmware failed ret = %d\n", ret);
		break;

	case SPRD_VDSP_IO_SET_MSG:
		VDSP_DEBUG("vdsp_ioctl  SPRD_VDSP_IO_SET_MSG entry !\n");
		ret = copy_from_user((void *)&msg, (void __user *)param,
                           sizeof(struct sprd_dsp_cmd));
		// send msg
		// if stop cmd,first send out
		cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
		if (!cfg)
			return -ENOMEM;
		INIT_LIST_HEAD(&cfg->head);
		mutex_lock(&vdsp_hw_dev.post_lock);
		vdsp_cmd_convertor(&vdsp_hw_dev, &msg);
		cfg->msg = msg;
		list_add_tail(&cfg->head, &vdsp_hw_dev.post_list);
		kthread_queue_work(&vdsp_hw_dev.post_worker, &vdsp_hw_dev.post_work);
		mutex_unlock(&vdsp_hw_dev.post_lock);
		vdsp_wait_xm6_done();
		VDSP_DEBUG("vdsp_ioctl SPRD_VDSP_IO_SET_MSG end\n");
		break;
	default:
		VDSP_ERROR("bad vdsp-ioctl cmd %d\n", cmd);
		return -EINVAL;
	}
	return ret;
}

static irqreturn_t vdsp_isr(int irq, void *data)
{
	struct vdsp_dev_t *vdsp = data;
	if (vdsp->core && vdsp->core->isr_clear)
		vdsp->core->isr_clear(&vdsp->ctx, 2);
	VDSP_INFO("vdsp isr rec\n");
	evt_update = true;
	wake_up_interruptible_all(&vdsp_hw_dev.wait_queue);

	return IRQ_HANDLED;
}


static const struct sprd_vdsp_cfg_data roc1_vdsp_data = {
	.version = ROC1,
	.max_freq_level = 5,
	.softreset_reg_offset = 0x4,
	.reset_mask = BIT(2),
};


static const struct of_device_id of_match_table_vdsp[] = {
	{.compatible = "sprd,roc1-vdsp", .data = &roc1_vdsp_data},
	{},
};


static int vdsp_parse_dt(struct vdsp_dev_t *vdsp,
				struct device_node *np)
{
	struct resource r;
	struct vdsp_context *ctx = &vdsp->ctx;

	VDSP_INFO("vdsp_parse_dt called !\n");
	vdsp_hw_dev.version = vdsp_hw_dev.vdsp_cfg_data->version;
	if (of_address_to_resource(np, 0, &r)) {
		VDSP_ERROR("parse dt base address failed\n");
		return -ENODEV;
	}

	ctx->xm6_base = (unsigned long)ioremap_nocache(r.start,
					resource_size(&r)) + 0x400400;
	if (ctx->xm6_base == 0) {
		VDSP_ERROR("ioremap base address failed\n");
		return -EFAULT;
	}
	VDSP_INFO("sprd_vdsp_base_addr = %lx\n", (unsigned long)r.start);
	VDSP_INFO("xm6_base = %lx\n", ctx->xm6_base);

	if (!of_address_to_resource(np, 1, &r)) {
		ctx->icu_base = (unsigned long)
		    ioremap_nocache(r.start, resource_size(&r));
		if (ctx->icu_base == 0) {
			VDSP_ERROR("icu apbbase ioremap failed\n");
			return -EFAULT;
		}
		VDSP_INFO("sprd_icu_phys_addr = %lx\n", (unsigned long)r.start);
		VDSP_INFO("sprd_icu_base = %lx\n", ctx->icu_base);
	}

	VDSP_INFO("glb->parse_dt\n");
	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->parse_dt)
		vdsp_hw_dev.glb->parse_dt(&vdsp_hw_dev.ctx, np);
	if (vdsp_hw_dev.core && vdsp_hw_dev.core->parse_dt)
		vdsp_hw_dev.core->parse_dt(&vdsp_hw_dev.ctx, np);
	return 0;
}


static int sprd_vdsp_irq_request(struct vdsp_dev_t *vdsp)
{
	int err;
	int irq_num;

	irq_num = irq_of_parse_and_map(vdsp->dev.of_node, 0);
	if (!irq_num) {
		VDSP_ERROR("error: vdsp parse irq num failed\n");
		return -EINVAL;
	}
	VDSP_INFO("vdsp irq_num = %d\n", irq_num);

	err = request_irq(irq_num, vdsp_isr, 0, "VDSP", vdsp);
	if (err) {
		VDSP_ERROR("error: vdsp request irq failed\n");
		return -EINVAL;
	}
	vdsp->ctx.irq = irq_num;

	return 0;
}

static int vdsp_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	u32 code_addr = vdsp_share_buff_addr;
	u32 ext_code_addr = vdsp_share_buff_addr+ VDSP_SHARE_BUFF_EXT_CODE_OFFSET;
	u32 data_addr = vdsp_share_buff_addr + VDSP_SHARE_BUFF_DATA_OFFSET;
	bool enable = true;
	
	if (need_load_fw == 1)
		ret = overwrite_firmware(&vdsp_hw_dev);
	if (ret != 0)
		VDSP_ERROR("vdsp_load_firmware failed ret = %d\n", ret);

	VDSP_DEBUG("vdsp_open called !\n");
	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->power)
		vdsp_hw_dev.glb->power(&vdsp_hw_dev.ctx, enable);
	//vdsp_clk_en();

	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->init)
		vdsp_hw_dev.glb->init(&vdsp_hw_dev.ctx);

	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->vector_addr_set)
		vdsp_hw_dev.glb->vector_addr_set(&vdsp_hw_dev.ctx, ext_code_addr);

	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->boot_signal_set)
		vdsp_hw_dev.glb->boot_signal_set(&vdsp_hw_dev.ctx, enable);

	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->reset)
		vdsp_hw_dev.glb->reset(&vdsp_hw_dev.ctx);

	if (vdsp_hw_dev.core && vdsp_hw_dev.core->do_pdma)
		vdsp_hw_dev.core->do_pdma(&vdsp_hw_dev.ctx,code_addr,vdsp_hw_dev.fw_code_size);

	if (vdsp_hw_dev.core && vdsp_hw_dev.core->do_ddma)
		vdsp_hw_dev.core->do_ddma(&vdsp_hw_dev.ctx,data_addr,vdsp_hw_dev.fw_data_size);

	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->acu_acc)
		vdsp_hw_dev.glb->acu_acc(&vdsp_hw_dev.ctx, 1);

	if (vdsp_hw_dev.core && vdsp_hw_dev.core->set_pcache)
		vdsp_hw_dev.core->set_pcache(&vdsp_hw_dev.ctx);

	if (vdsp_hw_dev.core && vdsp_hw_dev.core->set_edp_aximo_range)
		vdsp_hw_dev.core->set_edp_aximo_range(&vdsp_hw_dev.ctx);

	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->acu_acc)
		vdsp_hw_dev.glb->acu_acc(&vdsp_hw_dev.ctx, 0);
	//release vdsp core reset
	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->core_reset)
		vdsp_hw_dev.glb->core_reset(&vdsp_hw_dev.ctx);
	vdsp_hw_dev.is_opened = true;
	VDSP_DEBUG("vdsp_open called end!\n");
	return ret;
}

static int vdsp_release(struct inode *inode, struct file *filp)
{
#if 0
	bool enable = false;

	VDSP_DEBUG("vdsp_release called !\n");
	if (vdsp_hw_dev.glb && vdsp_hw_dev.glb->power)
		vdsp_hw_dev.glb->power(&vdsp_hw_dev.ctx, enable);
	vdsp_hw_dev.is_opened = false;
#endif
	return 0;
}

static const struct file_operations vdsp_fops = {
	.owner = THIS_MODULE,
	//.mmap = vsp_nocache_mmap,
	.open = vdsp_open,
	.release = vdsp_release,
	.unlocked_ioctl = vdsp_ioctl,
#ifdef CONFIG_COMPAT
	//.compat_ioctl = compat_vdsp_ioctl,
	.compat_ioctl = vdsp_ioctl,
#endif
};

static struct miscdevice vdsp_dev = {
	.minor = VDSP_MINOR,
	.name = "sprd_vdsp",
	.fops = &vdsp_fops,
};

//extern struct class *vdsp_class;

static int vdsp_device_register(struct vdsp_dev_t *vdsp_hw_dev,
                                struct device *parent)
{
        int ret;

        VDSP_DEBUG("vdsp device register begin\n");
        vdsp_hw_dev->dev.class = vdsp_class;
        vdsp_hw_dev->dev.parent = parent;
        vdsp_hw_dev->dev.of_node = parent->of_node;
        dev_set_name(&vdsp_hw_dev->dev, "vdsp%d", 0);
        dev_set_drvdata(&vdsp_hw_dev->dev, vdsp_hw_dev);

        ret = device_register(&vdsp_hw_dev->dev);
        if (ret)
                VDSP_ERROR("vdsp device register failed\n");

        return ret;
}

extern int vdsp_glb_register(void);
extern int vdsp_class_init(void);
extern int vdsp_core_register(void);
static int vdsp_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *node = pdev->dev.of_node;
	const struct of_device_id *of_id;
	const char *str;
	size_t  buf_size = 0xe0000;

	VDSP_DEBUG("vdsp_probe called !\n");
	vdsp_glb_register();
	vdsp_class_init();
	vdsp_core_register();

	of_id = of_match_node(of_match_table_vdsp, node);

	if (of_id) {
		VDSP_INFO("%s: find matched id!", __func__);
		vdsp_hw_dev.vdsp_cfg_data =
		  (struct sprd_vdsp_cfg_data *)of_id->data;
		}
	else
		VDSP_ERROR("%s: Not find matched id!", __func__);

	if (!of_property_read_string(node, "sprd,soc", &str)) {
		vdsp_hw_dev.core = vdsp_core_ops_attach(str);
		vdsp_hw_dev.clk = vdsp_clk_ops_attach(str);
		vdsp_hw_dev.glb = vdsp_glb_ops_attach(str);
		VDSP_INFO("\"sprd,soc\" was found\n");
	} else {
		VDSP_ERROR("error: \"sprd,soc\" was not found\n");
		vdsp_hw_dev.clk = NULL;
		vdsp_hw_dev.glb = NULL;
	}

	if (pdev->dev.of_node) {
		if (vdsp_parse_dt(&vdsp_hw_dev,node)) {
			VDSP_ERROR("vdsp_parse_dt failed\n");
			return -EINVAL;
		}
	}

	ret = misc_register(&vdsp_dev);
	if (ret) {
		VDSP_ERROR("cannot register miscdev on minor=%d (%d)\n",
		       VDSP_MINOR, ret);
		goto errout;
	}

	vdsp_device_register(&vdsp_hw_dev, &pdev->dev);
	sprd_vdsp_sysfs_init(&vdsp_hw_dev.dev);
	
	sprd_vdsp_irq_request(&vdsp_hw_dev);

	ret = sprd_vdsp_buf_alloc_2(&vdsp_hw_dev, ION_HEAP_ID_MASK_FB,
                                        &buf_size, &(vdsp_share_buff_addr));
        if (ret)
                return -1;
        else
                VDSP_INFO("vdsp alloc buf(vdsp_addr_v=0x%x,g_kva_vdsp_share_buff_addr=%p)\n",vdsp_share_buff_addr,
				g_kva_vdsp_share_buff_addr);

	kthread_init_worker(&vdsp_hw_dev.post_worker);
	VDSP_INFO("vdsp trace1");
	vdsp_hw_dev.post_thread = kthread_run(kthread_worker_fn,
			&vdsp_hw_dev.post_worker, "vdsp_msg");
	VDSP_INFO("vdsp trace2");
	if (IS_ERR(vdsp_hw_dev.post_thread)) {
		VDSP_INFO("vdsp trace3");
		ret = PTR_ERR(vdsp_hw_dev.post_thread);
		vdsp_hw_dev.post_thread = NULL;

		VDSP_ERROR("%s: failed to run config posting thread: %d\n",
				__func__, ret);
		goto errout;
	}
	VDSP_INFO("vdsp trace4");
	kthread_init_work(&vdsp_hw_dev.post_work, msg_post_work_func);
	mutex_init(&vdsp_hw_dev.post_lock);
	INIT_LIST_HEAD(&vdsp_hw_dev.post_list);
	init_waitqueue_head(&vdsp_hw_dev.wait_queue);
	VDSP_INFO("vdsp trace5");
	//memset((void *)(g_kva_vdsp_share_buff_addr + VDSP_SHARE_BUFF_CMD_OFFSET),0,32);	
	vdsp_hw_dev.log_task = kthread_run(log_task_func, NULL, "xm6_log_thread");
	VDSP_INFO("vdsp trace6");
		if (IS_ERR(vdsp_hw_dev.log_task)) {
			VDSP_INFO("vdsp trace7");
			ret = PTR_ERR(vdsp_hw_dev.log_task);
			vdsp_hw_dev.log_task = NULL;
	}	
	VDSP_INFO("vdsp trace8");
	return 0;

errout:
	VDSP_INFO("vdsp trace9");
	misc_deregister(&vdsp_dev);

	return ret;
}

static int vdsp_remove(struct platform_device *pdev)
{
	VDSP_DEBUG("vdsp_remove called !\n");

	misc_deregister(&vdsp_dev);
	free_irq(vdsp_hw_dev.irq, &vdsp_hw_dev);

	VDSP_DEBUG("vdsp_remove Success !\n");
	return 0;
}

static struct platform_driver vdsp_driver = {
	.probe = vdsp_probe,
	.remove = vdsp_remove,

	.driver = {
		   .owner = THIS_MODULE,
		   .name = "sprd_vdsp",
		   .of_match_table = of_match_ptr(of_match_table_vdsp),
		   },
};

module_platform_driver(vdsp_driver);

MODULE_DESCRIPTION("Sprd VDSP Driver");
MODULE_LICENSE("GPL");
