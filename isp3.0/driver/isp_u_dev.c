/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_u_dev"

#include <pthread.h>
#include <semaphore.h>
#include "isp_drv.h"
#include "isp_common_types.h"

struct img_addr {
	cmr_uint                                addr_y;
	cmr_uint                                addr_u;
	cmr_uint                                addr_v;
};

enum img_data_type {
	IMG_DATA_TYPE_YUV422 = 0,
	IMG_DATA_TYPE_YUV420,
	IMG_DATA_TYPE_YVU420,
	IMG_DATA_TYPE_YUV420_3PLANE,
	IMG_DATA_TYPE_RAW,
	IMG_DATA_TYPE_RAW2,
	IMG_DATA_TYPE_RGB565,
	IMG_DATA_TYPE_RGB666,
	IMG_DATA_TYPE_RGB888,
	IMG_DATA_TYPE_JPEG,
	IMG_DATA_TYPE_YV12,
	IMG_DATA_TYPE_MAX
};


#define isp_fw_size            0x200000

static char isp_dev_name[50] = "/dev/sprd_isp";
static char isp_fw_name[50] = "/system/vendor/firmware/TBM_G2v1DDR.bin";

struct isp_fw_mem {
	cmr_uint virt_addr;
	cmr_uint phy_addr;
	cmr_int mfd;
	cmr_u32 size;
	cmr_u32 num;
};

struct isp_file {
	int                        fd;
	int                       camera_id;
	cmr_handle                 evt_3a_handle;
	cmr_handle                 grab_handle;
	pthread_mutex_t            cb_mutex;
	pthread_t                  thread_handle;
	isp_evt_cb                 isp_event_cb;  //isp event callback
	isp_evt_cb                 isp_cfg_buf_cb;  //isp event callback
	struct isp_dev_init_info   init_param;
	sem_t                      close_sem;
	struct isp_fw_mem          fw_mem;
	cmr_int                    isp_is_inited;
};

static cmr_int isp_dev_create_thread(isp_handle handle);
static cmr_int isp_dev_kill_thread(isp_handle handle);
static void* isp_dev_thread_proc(void* data);
static cmr_int isp_dev_load_binary(isp_handle handle);

cmr_int isp_dev_init(struct isp_dev_init_info *init_param_ptr, isp_handle *handle)
{
	cmr_int                       ret = 0;
	cmr_int                       fd = -1;
	struct isp_dev_init_param     init_param;
	struct isp_file               *file = NULL;

	file = malloc(sizeof(struct isp_file));
	if (!file) {
		ret = -1;
		CMR_LOGE("alloc memory error.");
		return ret;
	}
	cmr_bzero(file, sizeof(*file));

	if (NULL == init_param_ptr) {
		ret = -1;
		CMR_LOGE("init param null error.");
		goto isp_free;
	}

	/*file->init_param = *init_param_ptr;*/
	memcpy((void *)&file->init_param, init_param_ptr, sizeof(struct isp_dev_init_info));

	file->init_param.shading_bin_offset = ISP_FW_BUF_SIZE+ISP_SHARE_BUFF_SIZE+ISP_WORKING_BUF_SIZE+
		(ISP_STATISTICS_BUF_SIZE*5*3)+ISP_IRP_BIN_BUF_SIZE*3;
	file->init_param.irp_bin_offset = ISP_FW_BUF_SIZE+ISP_SHARE_BUFF_SIZE+ISP_WORKING_BUF_SIZE+
		(ISP_STATISTICS_BUF_SIZE*5*3);

	fd = open(isp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		ret = -1;
		CMR_LOGE("isp_dev_open error.");
		goto isp_free;
	}
	file->fd = fd;

	init_param.camera_id = init_param_ptr->camera_id;
	init_param.width = init_param_ptr->width;
	init_param.height = init_param_ptr->height;
	ret= ioctl(file->fd, ISP_IO_SET_INIT_PARAM, &init_param);
	if (ret) {
		CMR_LOGE("isp set initial param error.");
		goto isp_free;
	}

	sem_init(&file->close_sem, 0, 0);

	ret = pthread_mutex_init(&file->cb_mutex, NULL);
	if (ret) {
		CMR_LOGE("Failed to init mutex : %d", errno);
		goto isp_free;
	}

	file->camera_id = init_param_ptr->camera_id;
	file->isp_is_inited = 0;
	*handle = (isp_handle)file;

	/*create isp dev thread*/
	ret = isp_dev_create_thread((isp_handle)file);
	if (ret) {
		CMR_LOGE("isp dev create thread failed.");
	} else {
		CMR_LOGI("isp device init ok.");
	}
	return ret;

isp_free:
	if (file)
		free(file);
	file = NULL;

	return ret;
}

cmr_int isp_dev_deinit(isp_handle handle)
{
	cmr_int                       ret = 0;
	int fd = 0;
	struct isp_file               *file = (struct isp_file*)handle;

	if (!file) {
		ret = -1;
		CMR_LOGE("isp_handle is null error.");
		return ret;
	}

	ret = isp_dev_kill_thread(handle);
	if (ret) {
		CMR_LOGE("Failed to kill the thread. errno : %d", errno);
		return ret;
	}

	if (file->isp_is_inited) {
		if (-1 != file->fd) {
			sem_wait(&file->close_sem);
			if (-1 == close(file->fd)) {
				CMR_LOGE("close error.");
			}
		} else {
			CMR_LOGE("isp_dev_close error.");
		}

		file->init_param.free_cb(CAMERA_ISP_BINGING4AWB, file->init_param.mem_cb_handle,
			(cmr_uint*)file->fw_mem.phy_addr, (cmr_uint*)file->fw_mem.virt_addr, &fd, file->fw_mem.num);
	}

	pthread_mutex_lock(&file->cb_mutex);
	file->isp_event_cb = NULL;
	pthread_mutex_unlock(&file->cb_mutex);

	pthread_mutex_destroy(&file->cb_mutex);
	sem_destroy(&file->close_sem);

	file->isp_is_inited = 0;
	free(file);

	return ret;
}

cmr_int isp_dev_start(isp_handle handle)
{
	cmr_int                       ret = 0;
	struct isp_file          *file = (struct isp_file*)handle;
	struct isp_init_mem_param    load_input;
	cmr_int                                         fw_size = 0;
	cmr_u32                                       fw_buf_num = 1;
	cmr_u32                                       kaddr[2];
	cmr_u64                                       kaddr_temp;

	if (file->isp_is_inited) {
		CMR_LOGE("isp firmware no need load again ");
		goto exit;
	}

	ret = isp_dev_capability_fw_size((isp_handle)file, &fw_size);
	if (ret) {
		CMR_LOGE("failed to get fw size %ld", ret);
		goto exit;
	}

	memset(&load_input, 0x00, sizeof(load_input));
	file->init_param.alloc_cb(CAMERA_ISP_BINGING4AWB, file->init_param.mem_cb_handle,
				  (cmr_u32*)&fw_size, &fw_buf_num, (cmr_uint*)kaddr,
				  &load_input.fw_buf_vir_addr, &load_input.fw_buf_mfd);
	if (ret) {
		CMR_LOGE("faield to alloc fw buffer %ld", ret);
		goto exit;
	}

	file->isp_is_inited = 1;

	kaddr_temp = kaddr[1];
	load_input.fw_buf_phy_addr = (kaddr[0] | kaddr_temp << 32);
	CMR_LOGI("fw_buf_phy_addr 0x%llx,", load_input.fw_buf_phy_addr);
	CMR_LOGI("kaddr0 0x%x kaddr1 0x%x", kaddr[0], kaddr[1]);

	load_input.fw_buf_size = fw_size;
	load_input.shading_bin_offset = file->init_param.shading_bin_offset;
	load_input.irp_bin_offset = file->init_param.irp_bin_offset;
	CMR_LOGI("shading offset 0x%x irp offset 0x%x", load_input.shading_bin_offset, load_input.irp_bin_offset);
	CMR_LOGI("shading bin addr 0x%p size 0x%x irq bin addr 0x%p size %x",
		 file->init_param.shading_bin_addr, file->init_param.shading_bin_size,
		 file->init_param.irp_bin_addr, file->init_param.irp_bin_size);

	file->fw_mem.virt_addr = load_input.fw_buf_vir_addr;
	file->fw_mem.phy_addr = load_input.fw_buf_phy_addr;
	file->fw_mem.mfd = load_input.fw_buf_mfd;
	file->fw_mem.size = load_input.fw_buf_size;
	file->fw_mem.num = fw_buf_num;

	ret = isp_dev_load_binary((isp_handle)file);
	if (0 != ret) {
		CMR_LOGE("failed to load binary %ld", ret);
		goto exit;
	}

	ret = isp_dev_load_firmware((isp_handle)file, &load_input);
	if ((0 != ret) ||(0 == load_input.fw_buf_phy_addr)) {
		CMR_LOGE("failed to load firmware %ld", ret);
		goto exit;
	}

exit:
	return ret;
}

static cmr_int isp_dev_load_binary(isp_handle handle)
{
	cmr_int                  ret = 0;
	struct isp_file          *file = (struct isp_file*)handle;
	cmr_u32                  isp_id = 0;
	if (!file) {
		ret = -1;
		CMR_LOGE("file hand is null error.");
		return ret;
	}

	if (!file->init_param.shading_bin_addr || !file->init_param.irp_bin_addr) {
		ret = -1;
		CMR_LOGE("param null error");
		goto exit;
	}
	if (file->init_param.shading_bin_size > ISP_SHADING_BIN_BUF_SIZE ||
		file->init_param.irp_bin_size > ISP_IRP_BIN_BUF_SIZE) {
		ret = -1;
		CMR_LOGE("the shading/irp binary size is out of range.");
		goto exit;
	}

	ret = isp_dev_get_isp_id(handle, &isp_id);

	memcpy((void*)(file->fw_mem.virt_addr + file->init_param.shading_bin_offset + ISP_SHADING_BIN_BUF_SIZE * isp_id),
		(void*)file->init_param.shading_bin_addr, file->init_param.shading_bin_size);
	memcpy((void*)(file->fw_mem.virt_addr + file->init_param.irp_bin_offset + ISP_IRP_BIN_BUF_SIZE * isp_id),
		(void*)file->init_param.irp_bin_addr, file->init_param.irp_bin_size);

	CMR_LOGI("shading check 0x%x", *(cmr_u32*)(file->fw_mem.virt_addr + file->init_param.shading_bin_offset));
	CMR_LOGI("irp check 0x%x", *(cmr_u32*)(file->fw_mem.virt_addr + file->init_param.irp_bin_offset));

exit:
	return ret;
}

void isp_dev_evt_reg(isp_handle handle, isp_evt_cb isp_event_cb, void *privdata)
{
	struct isp_file          *file = (struct isp_file*)handle;

	if (!file) {
		CMR_LOGE("isp_handle is null error.");
		return;
	}

	pthread_mutex_lock(&file->cb_mutex);
	file->isp_event_cb = isp_event_cb;
	file->evt_3a_handle = privdata;
	pthread_mutex_unlock(&file->cb_mutex);
	return;
}

void isp_dev_buf_cfg_evt_reg(isp_handle handle, cmr_handle grab_handle, isp_evt_cb isp_event_cb)
{
	struct isp_file          *file = (struct isp_file*)handle;

	if (!file) {
		CMR_LOGE("isp_handle is null error.");
		return;
	}

	pthread_mutex_lock(&file->cb_mutex);
	file->grab_handle = grab_handle;
	file->isp_cfg_buf_cb = isp_event_cb;
	pthread_mutex_unlock(&file->cb_mutex);
	return;
}

static cmr_int isp_dev_create_thread(isp_handle handle)
{
	cmr_int                  ret = 0;
	pthread_attr_t           attr;
	struct isp_file          *file;

	file = (struct isp_file*)handle;
	if (!file) {
		ret = -1;
		CMR_LOGE("file hand is null error.");
		return ret;
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&file->thread_handle, &attr, isp_dev_thread_proc, (void*)handle);
	pthread_attr_destroy(&attr);

	return ret;
}

static cmr_int isp_dev_kill_thread(isp_handle handle)
{
	cmr_int                    ret = 0;
	struct isp_img_write_op    write_op;
	struct isp_file            *file;
	void                       *dummy;

	file = (struct isp_file*)handle;
	if (!file) {
		ret = -1;
		CMR_LOGE("file hand is null error.");
		return ret;
	}

	CMR_LOGI("kill isp proc thread.");
	memset(&write_op, 0, sizeof(struct isp_img_write_op));
	write_op.cmd = ISP_IMG_STOP_ISP;
	ret = write(file->fd, &write_op, sizeof(struct isp_img_write_op));
	if (ret > 0) {
		CMR_LOGI("write OK!");
		ret = pthread_join(file->thread_handle, &dummy);
		file->thread_handle = 0;
	}

	return ret;
}

static void* isp_dev_thread_proc(void *data)
{
	struct isp_file                  *file = NULL;
	struct isp_statis_frame_output    statis_frame;
	struct isp_statis_frame           statis_frame_buf;
	struct isp_frm_info        img_frame;
	struct isp_irq                    irq_node;
	struct isp_irq_info               irq_info;
	struct img_addr addr;

	file = (struct isp_file*)data;
	CMR_LOGI("isp dev thread file %p ", file);
	if(!file) {
		return NULL;
	}

	if(-1 == file->fd) {
		return NULL;
	}

	CMR_LOGI("isp dev thread in.");
	while (1) {
		/*Get irq*/
		if (-1 == ioctl(file->fd, ISP_IO_IRQ, &irq_info)) {
			CMR_LOGI("ISP_IO_IRQ error.");
			break;
		} else {
			if(ISP_IMG_TX_STOP == irq_info.irq_flag) {
				CMR_LOGE("isp_dev_thread_proc exit.");
				break;
			} else if (ISP_IMG_SYS_BUSY == irq_info.irq_flag) {
				cmr_usleep(100);
				CMR_LOGI("continue.");
				continue;
			} else if (ISP_IMG_NO_MEM == irq_info.irq_flag){
				CMR_LOGE("statistics no mem");
				continue;
			} else {
				if(ISP_IMG_TX_DONE == irq_info.irq_flag) {
					if (irq_info.irq_type == ISP_IRQ_STATIS) {
						CMR_LOGI("got one frame statis vaddr 0x%lx paddr 0x%lx buf_size 0x%lx", irq_info.yaddr_vir, irq_info.yaddr, irq_info.length);
						statis_frame.format = irq_info.format;
						statis_frame.buf_size = irq_info.length;
						statis_frame.phy_addr = irq_info.yaddr;
						statis_frame.vir_addr = irq_info.yaddr_vir;
						statis_frame.time_stamp.sec = irq_info.time_stamp.sec;
						statis_frame.time_stamp.usec = irq_info.time_stamp.usec;
						pthread_mutex_lock(&file->cb_mutex);
						if(file->isp_event_cb) {
							(*file->isp_event_cb)(ISP_DRV_STATISTICE, &statis_frame, sizeof(statis_frame), (void *)file->evt_3a_handle);
						}
						pthread_mutex_unlock(&file->cb_mutex);
					} else if (irq_info.irq_type == ISP_IRQ_3A_SOF) {
						CMR_LOGI("got one sof");
						irq_node.irq_val0 = irq_info.irq_id;
						irq_node.reserved = irq_info.frm_index;
						irq_node.ret_val = 0;
						irq_node.time_stamp.sec = irq_info.time_stamp.sec;
						irq_node.time_stamp.usec = irq_info.time_stamp.usec;
						pthread_mutex_lock(&file->cb_mutex);
						if(file->isp_event_cb) {
							(*file->isp_event_cb)(ISP_DRV_SENSOR_SOF, &irq_node, sizeof(irq_node), (void *)file->evt_3a_handle);
						}
						pthread_mutex_unlock(&file->cb_mutex);
					} else if (irq_info.irq_type == ISP_IRQ_CFG_BUF) {
						img_frame.channel_id = irq_info.channel_id;
						img_frame.base= irq_info.base_id;
						img_frame.frame_id= irq_info.base_id;
						img_frame.yaddr = irq_info.yaddr;
						img_frame.uaddr = irq_info.uaddr;
						img_frame.vaddr = irq_info.vaddr;
						img_frame.yaddr_vir= irq_info.yaddr_vir;
						img_frame.uaddr_vir= irq_info.uaddr_vir;
						img_frame.vaddr_vir= irq_info.vaddr_vir;
						//img_frame.length= irq_info.buf_size.width*irq_info.buf_size.height;
						img_frame.fd = irq_info.img_y_fd;
						img_frame.sec= irq_info.time_stamp.sec;
						img_frame.usec= irq_info.time_stamp.usec;
						CMR_LOGI("high iso got one frm vaddr 0x%lx paddr 0x%lx, width %d, height %d",
							 irq_info.yaddr_vir, irq_info.yaddr, file->init_param.width, file->init_param.height);
						pthread_mutex_lock(&file->cb_mutex);
						if(file->isp_cfg_buf_cb) {
							(*file->isp_cfg_buf_cb)(ISP_DRV_CFG_BUF, &img_frame, sizeof(img_frame), (void *)file->grab_handle);
						}
						pthread_mutex_unlock(&file->cb_mutex);
					}
				}
			}
		}
	}

	sem_post(&file->close_sem);
	CMR_LOGI("isp dev thread out.");

	return NULL;
}
cmr_int isp_dev_open(isp_handle *handle, struct isp_dev_init_param *param)
{
	cmr_int ret = 0;
	cmr_int fd = -1;
	struct isp_file *file = NULL;
	struct isp_dev_init_param *init_param = NULL;

	file = malloc(sizeof(struct isp_file));
	if (!file) {
		ret = -1;
		CMR_LOGE("alloc memory error.");
		return ret;
	}
	if (NULL == param) {
		ret = -1;
		CMR_LOGE("init param null error.");
		goto isp_free;
	}

	init_param = (struct isp_dev_init_param *)param;

	fd = open(isp_dev_name, O_RDWR, 0);
	if (fd < 0) {
		ret = -1;
		CMR_LOGE("isp_dev_open error.");
		goto isp_free;
	}

	file->fd = fd;
	file->camera_id = 0;
	*handle = (isp_handle)file;

	ret= ioctl(file->fd, ISP_IO_SET_INIT_PARAM, init_param);
	if (ret) {
		CMR_LOGE("isp initial param error.");
	}

	return ret;

isp_free:
	if (file)
		free(file);
	file = NULL;

	return ret;
}

cmr_int isp_dev_close(isp_handle handle)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("file hand is null error.");
		ret = -1;
		return ret;
	}

	file = (struct isp_file *)handle;

	if (-1 != file->fd) {
		if (-1 == close(file->fd)) {
			CMR_LOGE("close error.");
		}
	} else {
		CMR_LOGE("isp_dev_close error.");
	}

	free(file);

	return ret;
}

cmr_int isp_dev_stop(isp_handle handle)
{
	cmr_int ret = 0;
	cmr_int camera_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	camera_id = file->camera_id;

	ret = ioctl(file->fd, ISP_IO_STOP, &camera_id);
	if (ret) {
		CMR_LOGE("isp_dev_stop error.");
	}

	return ret;
}

cmr_int isp_dev_stream_on(isp_handle handle)
{
	cmr_int ret = 0;
	cmr_int camera_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	camera_id = file->camera_id;

	ret = ioctl(file->fd, ISP_IO_STREAM_ON, &camera_id);
	if (ret) {
		CMR_LOGE("isp_dev_stream_on error.");
	}

	return ret;
}

cmr_int isp_dev_stream_off(isp_handle handle)
{
	cmr_int ret = 0;
	cmr_int camera_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	camera_id = file->camera_id;

	ret = ioctl(file->fd, ISP_IO_STREAM_OFF, &camera_id);
	if (ret) {
		CMR_LOGE("isp_dev_stream_off error.");
	}

	return ret;
}

cmr_int isp_dev_load_firmware(isp_handle handle, struct isp_init_mem_param *param)
{
	cmr_int ret = 0;
	cmr_int fw_size = 0;
	FILE *fp = NULL;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	/*load altek isp firmware from user space*/

	fp = fopen(isp_fw_name, "rd");
	if(NULL == fp) {
		CMR_LOGE("open altek isp firmware failed.");
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	fw_size = ftell(fp);
	if(0 == fw_size || isp_fw_size < fw_size) {
		CMR_LOGE("firmware size fw_size invalid, fw_size = %ld", fw_size);
		fclose(fp);
		return -1;
	}
	fseek(fp, 0, SEEK_SET);

	ret = fread((void*)param->fw_buf_vir_addr, 1, fw_size, fp);
	if(ret < 0) {
		CMR_LOGE("read altek isp firmware failed.");
		fclose(fp);
		return -1;
	}
	fclose(fp);

	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, ISP_IO_LOAD_FW, param);
	if (ret) {
		CMR_LOGE("isp_dev_load_firmware error.");
	}

	return ret;
}

cmr_int isp_dev_set_statis_buf(isp_handle handle, struct isp_statis_buf *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_STATIS_BUF, param);
	if (ret) {
		CMR_LOGE("isp_dev_set_statis_buf error. 0x%lx", ret);
	}

	return ret;
}

cmr_int isp_dev_get_statis_buf(isp_handle handle, struct isp_img_read_op *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_read_op *op = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
	}

	op = param;
	op->cmd = ISP_IMG_GET_STATISTICS_FRAME;

	file = (struct isp_file *)(handle);

	ret = read(file->fd, op, sizeof(struct isp_img_read_op));
	if (ret) {
		CMR_LOGE("isp_dev_get_statis_buf error.");
	}

	return ret;
}

cmr_int isp_dev_set_img_buf(isp_handle handle, struct isp_cfg_img_buf *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_IMG_BUF, param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_img_buf error.");
	}

	return ret;
}

cmr_int isp_dev_get_img_buf(isp_handle handle, struct isp_img_read_op *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_read_op *op = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	op = param;
	op->cmd = ISP_IMG_GET_FRAME;

	file = (struct isp_file *)(handle);

	ret = read(file->fd, op, sizeof(struct isp_img_read_op));
	if (ret) {
		CMR_LOGE("isp_dev_get_img_buf error.");
	}

	return ret;
}

cmr_int isp_dev_set_img_param(isp_handle handle, struct isp_cfg_img_param *param)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!param) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_IMG_PARAM, param);
	if (ret) {
		CMR_LOGE("isp_dev_set_img_param error.");
	}

	return ret;
}

cmr_int isp_dev_set_rawaddr(isp_handle handle, struct isp_raw_data *param)
{
	cmr_int ret = 0;
	cmr_int isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	if (!param) {
	CMR_LOGE("param is null error.");
	return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_RAW10, param);
	if (ret) {
		CMR_LOGE("isp_dev_set_rawaddr error.");
	}

	return ret;
}

cmr_int isp_dev_set_post_yuv_mem(isp_handle handle, struct isp_img_mem *param)
{
	cmr_int ret = 0;
	cmr_int isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	if (!param) {
	CMR_LOGE("param is null error.");
	return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_POST_PROC_YUV, param);
	if (ret) {
		CMR_LOGE("isp_dev_set_post_yuv_mem error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_cap_buf(isp_handle handle, struct isp_img_mem *param)
{
	cmr_int ret = 0;
	cmr_int isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	if (!param) {
		CMR_LOGE("param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_CAP_BUF, param);
	if (ret) {
		CMR_LOGE("isp_dev_isp_cap error.");
	}

	return ret;

}

cmr_int isp_dev_set_fetch_src_buf(isp_handle handle, struct isp_img_mem *param)
{
	cmr_int ret = 0;
	cmr_int isp_id = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	if (!param) {
	CMR_LOGE("param is null error.");
	return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_SET_FETCH_SRC_BUF, param);
	if (ret) {
		CMR_LOGE("isp_dev_set_fetch_src_buf error.");
	}

	return ret;
}

cmr_int isp_dev_get_timestamp(isp_handle handle, cmr_u32 *sec, cmr_u32 *usec)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct sprd_isp_time time;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_GET_TIME, &time);
	if (ret) {
		CMR_LOGE("isp_dev_get_timestamp error.");
	}

	*sec = time.sec;
	*usec = time.usec;
	CMR_LOGI("sec=%d, usec=%d \n", *sec, *usec);

	return ret;
}

cmr_int isp_dev_cfg_scenario_info(isp_handle handle, SCENARIO_INFO_AP *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_SCENARIO_INFO;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_scenario_info error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_iso_speed(isp_handle handle, cmr_u32 *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_ISO_SPEED;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_iso_speed error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_awb_gain(isp_handle handle, struct isp_awb_gain_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AWB_GAIN;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_awb_gain error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_awb_gain_balanced(isp_handle handle, struct isp_awb_gain_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AWB_GAIN_BALANCED;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_awb_gain error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq(isp_handle handle, struct dld_sequence *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQUENCE;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_3a_param(isp_handle handle, struct cfg_3a_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_3A_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_3a_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_ae_param(isp_handle handle, struct ae_cfg_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AE_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_ae_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_af_param(isp_handle handle, struct af_cfg_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AF_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_af_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_awb_param(isp_handle handle, struct awb_cfg_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AWB_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_awb_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_yhis_param(isp_handle handle, struct yhis_cfg_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_YHIS_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_yhis_param error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_sub_sample(isp_handle handle, struct subsample_cfg_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_SUB_SAMP_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_sub_sample error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_anti_flicker(isp_handle handle, struct antiflicker_cfg_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_AFL_CFG;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_anti_flicker error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq_basic_prev(isp_handle handle, cmr_u32 size, cmr_u8*data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQ_BASIC_PREV;
	param.property_param = data;
	param.reserved = size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq_basic_prev error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq_adv_prev(isp_handle handle, cmr_u32 size, cmr_u8*data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQ_ADV_PREV;
	param.property_param = data;
	param.reserved = size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq_adv_prev error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_dld_seq_fast_converge(isp_handle handle, cmr_u32 size, cmr_u8*data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
	}

	param.sub_id = ISP_CFG_SET_DLD_SEQ_BASIC_FAST_CONV;
	param.property_param = data;
	param.reserved = size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_dld_seq_fast_converge error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_sharpness(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_SHARPNESS;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_sharpness error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_saturation(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_SATURATION;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_saturation error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_contrast(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_CONTRAST;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_contrast error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_special_effect(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_SPECIAL_EFFECT;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret) {
		CMR_LOGE("isp_dev_cfg_special_effect error.");
	}

	return ret;
}

cmr_int isp_dev_cfg_brightness_gain(isp_handle handle, struct isp_brightness_gain *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	param.sub_id = ISP_CFG_SET_BRIGHTNESS_GAIN;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_brightness_gain error.");

	return ret;
}

cmr_int isp_dev_cfg_brightness_mode(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 temp_mode = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	temp_mode = mode;
	param.sub_id = ISP_CFG_SET_BRIGHTNESS_MODE;
	param.property_param = &temp_mode;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_brightness_mode error.");

	return ret;
}

cmr_int isp_dev_cfg_color_temp(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 color_t = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	color_t = mode;
	param.sub_id = ISP_CFG_SET_COLOR_TEMPERATURE;
	param.property_param = &color_t;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_color_temp error.");

	return ret;
}

cmr_int isp_dev_cfg_ccm(isp_handle handle, struct isp_iq_ccm_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	param.sub_id = ISP_CFG_SET_CCM;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_ccm error.");

	return ret;
}

cmr_int isp_dev_cfg_valid_adgain(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 ad_gain = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	ad_gain = mode;
	param.sub_id = ISP_CFG_SET_VALID_ADGAIN;
	param.property_param = &ad_gain;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_color_temp error.");

	return ret;
}

cmr_int isp_dev_cfg_exp_time(isp_handle handle, cmr_u32 mode)
{
	cmr_int ret = 0;
	cmr_u32 exposure_time = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	exposure_time = mode;
	param.sub_id = ISP_CFG_SET_VALID_EXP_TIME;
	param.property_param = &exposure_time;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_color_temp error.");

	return ret;
}

cmr_int isp_dev_cfg_otp_info(isp_handle handle, struct isp_iq_otp_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	param.sub_id = ISP_CFG_SET_IQ_OTP_INFO;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_otp_info error.");

	return ret;
}

cmr_int isp_dev_cfg_sof_info(isp_handle handle, struct isp_sof_cfg_info *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	param.sub_id = ISP_CFG_SET_SOF_PARAM;
	param.property_param = data;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (ret)
		CMR_LOGE("isp_dev_cfg_sof_info error.");

	return ret;
}

cmr_int isp_dev_capability_fw_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_FW_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_fw_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_statis_buf_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_STATIS_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_statis_buf_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_dram_buf_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_DRAM_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_dram_buf_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_highiso_buf_size(isp_handle handle, cmr_int *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_HIGH_ISO_BUF_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("isp_dev_capability_highiso_buf_size error.");
	}

	return ret;
}

cmr_int isp_dev_capability_video_size(isp_handle handle, struct isp_img_size *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_CONTINUE_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("error");
	}

	return ret;
}

cmr_int isp_dev_capability_single_size(isp_handle handle, struct isp_img_size *size)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!size) {
		CMR_LOGE("Param is null error.");
	}

	param.index = ISP_GET_SINGLE_SIZE;
	param.property_param = (void *)size;

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);
	if (ret) {
		CMR_LOGE("error");
	}

	return ret;
}

cmr_int isp_dev_get_isp_id(isp_handle handle, cmr_u32 *isp_id)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle || !isp_id) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, ISP_IO_GET_ISP_ID, isp_id);
	if (ret) {
		CMR_LOGE("isp_dev_get_isp_id error.");
	}

	return ret;
}

cmr_int isp_dev_set_capture_mode(isp_handle handle, cmr_u32 capture_mode)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	CMR_LOGI("capture_mode %d", capture_mode);
	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, ISP_IO_SET_CAP_MODE, &capture_mode);
	if (ret) {
		CMR_LOGE("isp_dev_set_capture_mode error.");
	}

	return ret;
}

cmr_int isp_dev_set_skip_num(isp_handle handle, cmr_u32 skip_num)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	CMR_LOGI("skip num %d", skip_num);
	file = (struct isp_file *)(handle);
	ret = ioctl(file->fd, ISP_IO_SET_SKIP_NUM, &skip_num);
	if (ret) {
		CMR_LOGE("isp_dev_set_skip_num error.");
	}

	return ret;
}

cmr_int isp_dev_get_iq_param(isp_handle handle, struct debug_info1 *info1, struct debug_info2 *info2)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct altek_iq_info param;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	ret = ioctl(file->fd, ISP_IO_GET_IQ_PARAM, &param);
	if (ret) {
		CMR_LOGE("ioctl error.");
	}

	if (info1) {
		memcpy((void *)&info1->raw_debug_info1, &param.iq_info_1.tRawInfo, sizeof(struct raw_img_debug));
		memcpy((void *)&info1->shading_debug_info1, &param.iq_info_1.tShadingInfo, sizeof(struct shading_debug));
		memcpy((void *)&info1->irp_tuning_para_debug_info1, &param.iq_info_1.tIrpInfo, sizeof(struct irp_tuning_debug));
		memcpy((void *)&info1->sw_debug1, &param.iq_info_1.tSWInfo, sizeof(struct isp_sw_debug));
	}
	if (info2) {
		memcpy((void *)&info2->irp_tuning_para_debug_info2, &param.iq_info_2.tGammaTone, sizeof(struct irp_gamma_tone));
	}

	CMR_LOGI("%d %d %d %d %d %d\n",
		param.iq_info_1.tRawInfo.uwWidth,
		param.iq_info_1.tRawInfo.uwHeight,
		param.iq_info_1.tRawInfo.ucFormat,
		param.iq_info_1.tRawInfo.ucBitNumber,
		param.iq_info_1.tRawInfo.ucMirrorFlip,
		param.iq_info_1.tRawInfo.nColorOrder);

	return ret;
}

cmr_int isp_dev_set_init_param(isp_handle *handle, struct isp_dev_init_param *init_param_ptr)
{
	cmr_int                        ret = 0;
	struct isp_dev_init_param      init_param;
	struct isp_file                *file = NULL;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!init_param_ptr) {
		CMR_LOGE("init_param_ptr is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	CMR_LOGI("w %d h %d camera id %d", init_param_ptr->width,
		init_param_ptr->height,
		init_param_ptr->camera_id);
	if (init_param_ptr->width == file->init_param.width
		&& init_param_ptr->height == file->init_param.height
		&& init_param_ptr->camera_id == file->init_param.camera_id) {
		CMR_LOGI("same param.");
		return ret;
	}

	file->init_param.width = init_param_ptr->width;
	file->init_param.height = init_param_ptr->height;
	file->init_param.camera_id = init_param_ptr->camera_id;

	init_param.camera_id = init_param_ptr->camera_id;
	init_param.width = init_param_ptr->width;
	init_param.height = init_param_ptr->height;
	ret= ioctl(file->fd, ISP_IO_SET_INIT_PARAM, &init_param);
	if (ret) {
		CMR_LOGE("isp set initial param error.");
	}

	return ret;
}

cmr_int isp_dev_highiso_mode(isp_handle handle, struct highiso_data_buf *data)
{
	cmr_int ret = 0;
	struct isp_file *file = NULL;
	struct isp_raw_data raw_info;
	struct isp_hiso_data hiso_info;

	if (!handle) {
		CMR_LOGE("handle is null error.");
		return -1;
	}
	if (!data) {
		CMR_LOGE("Param is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);

	raw_info.fd = data->raw_buf.fd;
	raw_info.phy_addr = data->raw_buf.phy_addr;
	raw_info.virt_addr = data->raw_buf.virt_addr;
	raw_info.size = data->raw_buf.size;
	raw_info.width = data->raw_buf.width;
	raw_info.height = data->raw_buf.height;
	raw_info.capture_mode = data->capture_mode;
	CMR_LOGD("debug highiso raw fd = 0x%x, phy = 0x%x, vir = 0x%x, size = 0x%x",
		 raw_info.fd, raw_info.phy_addr, raw_info.virt_addr, raw_info.size);

	hiso_info.fd = data->highiso_buf.fd;
	hiso_info.phy_addr = data->highiso_buf.phy_addr;
	hiso_info.virt_addr = data->highiso_buf.virt_addr;
	hiso_info.size = data->highiso_buf.size;
	CMR_LOGD("debug highiso fd = 0x%x, highiso phy = 0x%x, highiso vir = 0x%x, size = 0x%x",
		 hiso_info.fd, hiso_info.phy_addr,hiso_info.virt_addr, hiso_info.size);
	ret = ioctl(file->fd, ISP_IO_SET_RAW10, &raw_info);
	if (ret) {
		CMR_LOGE("ISP_IO_SET_RAW10 error.");
	}

	ret = ioctl(file->fd, ISP_IO_SET_HISO, &hiso_info);
	if (ret) {
		CMR_LOGE("ISP_IO_SET_HISO error.");
	}

	return ret;
}
