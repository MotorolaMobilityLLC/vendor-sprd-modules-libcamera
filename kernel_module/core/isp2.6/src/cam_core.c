/*
 * Copyright (C) 2017-2018 Spreadtrum Communications Inc.
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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sprd_iommu.h>
#include <linux/sprd_ion.h>

#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>
#include "sprd_img.h"


#include "cam_types.h"
#include "cam_buf.h"
#include "cam_queue.h"
#include "cam_hw.h"
#include "cam_block.h"

#include "isp_interface.h"
#include "dcam_interface.h"
#include "flash_interface.h"

#include "sprd_sensor_drv.h"

#ifdef CONFIG_COMPAT
#include "compat_cam_drv.h"
#endif

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "CAM_CORE: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

#define IMG_DEVICE_NAME			"sprd_image"
#define CAMERA_TIMEOUT			5000
#define THREAD_STOP_TIMEOUT		3000


#define  CAM_COUNT  CAM_ID_MAX
/* TODO: extend this for slow motion dev */
#define  CAM_SHARED_BUF_NUM  16
#define  CAM_FRAME_Q_LEN   16
#define  CAM_IRQ_Q_LEN        16
#define  CAM_STATIS_Q_LEN   16
#define  CAM_ZOOM_COEFF_Q_LEN   10

/* TODO: tuning ratio limit for power/image quality */
#define MAX_RDS_RATIO 3
#define RATIO_SHIFT 16

#define ISP_PATHID_BITS 8
#define ISP_PATHID_MASK 0x3
#define ISP_CTXID_OFFSET ISP_PATHID_BITS

enum camera_module_state {
	CAM_INIT = 0,
	CAM_IDLE,
	CAM_CFG_CH,
	CAM_STREAM_ON,
	CAM_STREAM_OFF,
	CAM_RUNNING,
	CAM_ERROR,
};

/* Static Variables Declaration */
static struct camera_format output_img_fmt[] = {
	{ /*ISP_STORE_UYVY = 0 */
		.name = "4:2:2, packed, UYVY",
		.fourcc = IMG_PIX_FMT_UYVY,
		.depth = 16,
	},
	{ /* ISP_STORE_YUV422_3FRAME,*/
		.name = "YUV 4:2:2, planar, (Y-Cb-Cr)",
		.fourcc = IMG_PIX_FMT_YUV422P,
		.depth = 16,
	},
	{ /*ISP_STORE_YUV420_2FRAME*/
		.name = "YUV 4:2:0 planar (Y-CbCr)",
		.fourcc = IMG_PIX_FMT_NV12,
		.depth = 12,
	},
	{ /* ISP_STORE_YVU420_2FRAME,*/
		.name = "YVU 4:2:0 planar (Y-CrCb)",
		.fourcc = IMG_PIX_FMT_NV21,
		.depth = 12,
	},
	{ /*ISP_STORE_YUV420_3FRAME,*/
		.name = "YUV 4:2:0 planar (Y-Cb-Cr)",
		.fourcc = IMG_PIX_FMT_YUV420,
		.depth = 12,
	},
	{
		.name = "RawRGB",
		.fourcc = IMG_PIX_FMT_GREY,
		.depth = 8,
	},
};


struct camera_group;

/* user set information for camera module */
struct camera_uchannel {
	uint32_t sn_fmt;
	uint32_t dst_fmt;

	uint32_t deci_factor;/* for ISP output path */
	uint32_t is_high_fps;/* for DCAM slow motion feature */
	uint32_t high_fps_skip_num;/* for DCAM slow motion feature */

	struct sprd_img_size src_size;
	struct sprd_img_rect src_crop;
	struct sprd_img_size dst_size;

	/* for binding small picture */
	uint32_t slave_img_en;
	uint32_t slave_img_fmt;
	struct sprd_img_size slave_img_size;

	struct dcam_regular_desc regular_desc;
};

struct camera_uinfo {
	/* cap info */
	struct sprd_img_sensor_if sensor_if;
	struct sprd_img_size sn_size;
	struct sprd_img_size sn_max_size;
	struct sprd_img_rect sn_rect;
	uint32_t capture_mode;
	uint32_t capture_skip;

	uint32_t is_4in1;
	uint32_t is_3dnr;
	uint32_t is_ltm;
};

struct channel_context {
	enum cam_ch_id ch_id;
	uint32_t enable;
	uint32_t frm_base_id;
	uint32_t frm_cnt;
	atomic_t err_status;

	int32_t dcam_path_id;

	/* for which need anoter dcam & path offline processing.*/
	int32_t aux_dcam_path_id;

	/* isp_path_id combined from ctx_id | path_id.*/
	/* isp_path_id = (ctx_id << ISP_PATH_BITS) | path_id */
	int32_t isp_path_id;
	int32_t slave_isp_path_id;
	int32_t reserved_buf_fd;

	struct camera_uchannel ch_uinfo;
	struct img_size swap_size;
	struct img_trim trim_dcam;
	struct img_trim trim_isp;
	struct img_size dst_dcam;
	uint32_t rds_ratio;

	/* to store isp offline param data if frame is discarded. */
	void *isp_updata;

	uint32_t alloc_start;
	struct completion alloc_com;
	struct sprd_cam_work alloc_buf_work;

	uint32_t type_3dnr;
	uint32_t mode_ltm;
	struct camera_frame *nr3_bufs[ISP_NR3_BUF_NUM];
	struct camera_frame *ltm_bufs[ISP_LTM_BUF_NUM];

	/* dcam/isp shared frame buffer for full path */
	struct camera_queue share_buf_queue;
	struct camera_queue zoom_coeff_queue; /* channel specific coef queue */
};

struct camera_module {
	uint32_t idx;
	atomic_t state;
	atomic_t timeout_flag;
	struct mutex lock;
	struct camera_group *grp;

	int attach_sensor_id;
	uint32_t iommu_enable;
	enum camera_cap_status cap_status;

	void *isp_dev_handle;
	void *dcam_dev_handle;
	/* for which need another dcam offline processing raw data*/
	void *aux_dcam_dev;
	void *flash_core_handle;
	uint32_t dcam_idx;
	uint32_t aux_dcam_id;

	uint32_t is_smooth_zoom;
	uint32_t zoom_solution; /* for dynamic zoom type swicth. */
	uint32_t rds_limit; /* raw downsizer limit */
	struct camera_uinfo cam_uinfo;

	uint32_t last_channel_id;
	struct channel_context channel[CAM_CH_MAX];

	struct completion frm_com;
	struct camera_queue frm_queue; /* frame message queue for user*/
	struct camera_queue irq_queue; /* IRQ message queue for user*/
	struct camera_queue statis_queue; /* statis data queue or user*/

	struct cam_thread_info cap_thrd;
	struct cam_thread_info zoom_thrd;

	/*  dump raw  for debug*/
	struct cam_thread_info dump_thrd;
	struct camera_queue dump_queue;
	struct completion dump_com;
	struct timespec cur_dump_ts;
	uint32_t dump_count;
	uint32_t in_dump;

	struct timer_list cam_timer;
	struct workqueue_struct *workqueue;
	struct sprd_cam_work work;
};

struct camera_group {
	atomic_t camera_opened;

	spinlock_t module_lock;
	uint32_t  module_used;
	struct camera_module *module[CAM_COUNT];

	uint32_t dcam_count; /*dts cfg dcam count*/
	uint32_t isp_count; /*dts cfg isp count*/
	struct sprd_cam_hw_info *dcam[DCAM_ID_MAX]; /* dcam hw dev from dts */
	struct sprd_cam_hw_info *isp[2]; /* isp hw dev from dts */

	struct miscdevice *md;
	struct platform_device *pdev;
	struct camera_queue empty_frm_q;
};

struct cam_ioctl_cmd {
	unsigned int cmd;
	int (*cmd_proc)(struct camera_module *module,
			unsigned long arg);
};

struct camera_queue *g_empty_frm_q;

static struct isp_pipe_ops *isp_ops;
static struct dcam_pipe_ops *dcam_ops;
static struct cam_flash_ops *flash_ops;

static int img_ioctl_stream_off(
			struct camera_module *module,
			unsigned long arg);


static void put_k_frame(void *param)
{
	int ret = 0;
	struct camera_frame *frame;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;
	if (frame->buf.type == CAM_BUF_USER)
		cambuf_put_ionbuf(&frame->buf);
	else {
		cambuf_kunmap(&frame->buf);
		cambuf_free(&frame->buf);
	}
	ret = put_empty_frame(frame);
}

static void camera_put_empty_frame(void *param)
{
	int ret = 0;
	struct camera_frame *frame;

	if (!param) {
		pr_err("error: null input ptr.\n");
		return;
	}

	frame = (struct camera_frame *)param;
	if (frame->priv_data)
		kfree(frame->priv_data);
	cambuf_put_ionbuf(&frame->buf);
	ret = put_empty_frame(frame);
}

static void alloc_buffers(struct work_struct *work)
{
	int ret = 0;
	int i, count, total, iommu_enable;
	uint32_t width, height;
	ssize_t  size;
	struct sprd_cam_work *alloc_work;
	struct camera_module *module;
	struct camera_frame *pframe;
	struct channel_context *channel;

	pr_info("enter.\n");

	alloc_work = container_of(work, struct sprd_cam_work, work);
	channel = container_of(alloc_work, struct channel_context, alloc_buf_work);
	atomic_set(&alloc_work->status, CAM_WORK_RUNNING);

	module = (struct camera_module *)alloc_work->priv_data;
	iommu_enable = module->iommu_enable;

	pr_info("orig ch%d swap size %d , %d\n",
		channel->ch_id,  channel->swap_size.w, channel->swap_size.h);

	width = channel->swap_size.w;
	height = channel->swap_size.h;
	if (channel->ch_uinfo.sn_fmt == IMG_PIX_FMT_GREY)
		size = cal_sprd_raw_pitch(width) * height;
	else
		size = width * height * 3;
	size = ALIGN(size, CAM_BUF_ALIGN_SIZE);
	pr_debug("channel %d alloc shared buffer size: %d (w %d h %d)\n",
			channel->ch_id, (int)size, width, height);

	total = 5;
	if (module->dump_thrd.thread_task)
		total += 3;

	/* extend buffer queue for slow motion */
	if (channel->ch_uinfo.is_high_fps)
		total = CAM_SHARED_BUF_NUM;

	for (i = 0, count = 0; i < total; i++) {
		do {
			pframe = get_empty_frame();
			pframe->channel_id = channel->ch_id;
			ret = cambuf_alloc(
					&pframe->buf, size,
					0, iommu_enable);
			if (ret) {
				pr_err("fail to alloc buf: %d ch %d\n",
						i, channel->ch_id);
				put_empty_frame(pframe);
				atomic_inc(&channel->err_status);
				goto exit;
			}
			cambuf_kmap(&pframe->buf);
			ret = camera_enqueue(&channel->share_buf_queue, pframe);
			if (ret) {
				pr_err("fail to enqueue shared buf: %d ch %d\n",
						i, channel->ch_id);
				cambuf_kunmap(&pframe->buf);
				cambuf_free(&pframe->buf);
				put_empty_frame(pframe);
				/* no break here and will retry */
			} else {
				count++;
				pr_debug("frame %p,idx %d,cnt %d,phy_addr %p\n",
					pframe, i, count,
					(void *)pframe->buf.addr_vir[0]);
				break;
			}
		} while (1);
	}

	if (channel->type_3dnr == CAM_3DNR_HW) {
		/* YUV420 for 3DNR ref*/
		size = ((width + 1) & (~1)) * height * 3 / 2;
		size = ALIGN(size, CAM_BUF_ALIGN_SIZE);
		pr_info("ch %d 3dnr buffer size: %d.\n",
					channel->ch_id, (int)size);
		for (i = 0; i < ISP_NR3_BUF_NUM; i++) {
			pframe = get_empty_frame();
			ret = cambuf_alloc(&pframe->buf, size, 0, iommu_enable);
			if (ret) {
				pr_err("fail to alloc 3dnr buf: %d ch %d\n",
					i, channel->ch_id);
				put_empty_frame(pframe);
				atomic_inc(&channel->err_status);
				goto exit;
			}
			cambuf_kmap(&pframe->buf);
			channel->nr3_bufs[i] = pframe;
		}
	}

	if (channel->mode_ltm != MODE_LTM_OFF) {
		/* todo: ltm buffer size needs to be refined.*/
		/* size = ((width + 1) & (~1)) * height * 3 / 2; */
		/*
		 * sizeof histo from 1 tile: 128 * 16 bit
		 * MAX tile num: 8 * 8
		 * */
		size = 64 * 128 * 2;

		size = ALIGN(size, CAM_BUF_ALIGN_SIZE);

		pr_info("ch %d ltm buffer size: %d.\n",
			channel->ch_id, (int)size);
		for (i = 0; i < ISP_LTM_BUF_NUM; i++) {
			if (channel->ch_id == CAM_CH_PRE) {
				pframe = get_empty_frame();
				ret = cambuf_alloc(&pframe->buf, size, 0,
						   iommu_enable);
				if (ret) {
					pr_err("fail to alloc ltm buf: %d ch %d\n",
					       i, channel->ch_id);
					put_empty_frame(pframe);
					atomic_inc(&channel->err_status);
					goto exit;
				}
				cambuf_kmap(&pframe->buf);
				channel->ltm_bufs[i] = pframe;
			} else { /* CAM_CH_CAP case */
				/*
				 * LTM capture, USING preview path histo,
				 * So, setting preview buf to capture path
				 * */
				channel->ltm_bufs[i] =
					module->channel[CAM_CH_PRE].ltm_bufs[i];
			}
		}
	}

exit:
	complete(&channel->alloc_com);
	atomic_set(&alloc_work->status, CAM_WORK_DONE);
	pr_info("ch %d done. status %d\n",
		channel->ch_id, atomic_read(&channel->err_status));
}

static int set_cap_info(struct camera_module *module)
{
	int ret = 0;
	struct camera_uinfo *info = &module->cam_uinfo;
	struct sprd_img_sensor_if *sensor_if = &info->sensor_if;
	struct dcam_cap_cfg cap_info = { 0 };

	cap_info.mode = info->capture_mode;
	cap_info.frm_skip = info->capture_skip;
	cap_info.is_4in1 = info->is_4in1;
	cap_info.sensor_if = sensor_if->if_type;
	cap_info.format =  sensor_if->img_fmt;
	cap_info.pattern = sensor_if->img_ptn;
	cap_info.frm_deci = sensor_if->frm_deci;
	if (cap_info.sensor_if == DCAM_CAP_IF_CSI2) {
		cap_info.href = sensor_if->if_spec.mipi.use_href;
		cap_info.data_bits = sensor_if->if_spec.mipi.bits_per_pxl;
	}
	cap_info.cap_size.start_x = info->sn_rect.x;
	cap_info.cap_size.start_y = info->sn_rect.y;
	cap_info.cap_size.size_x = info->sn_rect.w;
	cap_info.cap_size.size_y = info->sn_rect.h;

	ret = dcam_ops->ioctl(module->dcam_dev_handle,
				DCAM_IOCTL_CFG_CAP, &cap_info);

	return ret;
}

int isp_callback(enum isp_cb_type type, void *param, void *priv_data)
{
	int ret = 0;
	uint32_t ch_id;
	struct camera_frame *pframe;
	struct camera_module *module;
	struct channel_context *channel;

	if (!param || !priv_data) {
		pr_err("Input ptr is NULL\n");
		return -EFAULT;
	}

	module = (struct camera_module *)priv_data;

	if (type == ISP_CB_DEV_ERR) {
		pr_err("ISP error happens. camera %d\n", module->idx);
		pframe = get_empty_frame();
		pframe->evt = IMG_TX_ERR;
		pframe->irq_type = CAMERA_IRQ_IMG;
		ret = camera_enqueue(&module->frm_queue, pframe);
		complete(&module->frm_com);
		return 0;
	}

	pframe = (struct camera_frame *)param;
	pframe->priv_data = NULL;
	channel = &module->channel[pframe->channel_id];

	if ((pframe->fid & 0x3F) == 0)
		pr_debug("cam %d, module %p, frame %p, ch %d\n",
		module->idx, module, pframe, pframe->channel_id);

	switch (type) {
	case ISP_CB_RET_SRC_BUF:
		if ((atomic_read(&module->state) != CAM_RUNNING) ||
			(channel->dcam_path_id < 0)) {
			/* stream off or test_isp_only */
			pr_info("isp ret src frame %p\n", pframe);
			camera_enqueue(&channel->share_buf_queue, pframe);
		} else if (module->cap_status == CAM_CAPTURE_RAWPROC) {
			/* for case raw capture post-proccessing
			 * just release it, no need to return
			 */
			if (pframe->buf.type == CAM_BUF_USER)
				cambuf_put_ionbuf(&pframe->buf);
			else
				cambuf_free(&pframe->buf);
			put_empty_frame(pframe);
			pr_info("raw proc return mid frame %p\n", pframe);
		} else {
			/* return offline buffer to dcam available queue. */
			pr_debug("isp reset dcam path out %d\n",
				channel->dcam_path_id);

			if (module->dump_thrd.thread_task && module->in_dump) {
				ret = camera_enqueue(&module->dump_queue, pframe);
				if (ret == 0) {
					complete(&module->dump_com);
					return 0;
				}
			}

			if (module->cam_uinfo.is_4in1 &&
				channel->aux_dcam_path_id == DCAM_PATH_BIN)
				/* 4in1, buf set to dcam1 bin path */
				ret = dcam_ops->cfg_path(module->aux_dcam_dev,
						DCAM_PATH_CFG_OUTPUT_BUF,
						channel->aux_dcam_path_id,
						pframe);
			else
				ret = dcam_ops->cfg_path(
					module->dcam_dev_handle,
					DCAM_PATH_CFG_OUTPUT_BUF,
					channel->dcam_path_id, pframe);
		}
		break;

	case ISP_CB_RET_DST_BUF:
		if (atomic_read(&module->state) == CAM_RUNNING) {
			if (module->cap_status == CAM_CAPTURE_RAWPROC) {
				pr_info("raw proc return dst frame %p\n", pframe);
				cambuf_put_ionbuf(&pframe->buf);
				module->cap_status = CAM_CAPTURE_RAWPROC_DONE;
				pframe->irq_type = CAMERA_IRQ_DONE;
				pframe->irq_property = IRQ_RAW_PROC_DONE;
			} else {
				pframe->irq_type = CAMERA_IRQ_IMG;
			}
			pframe->evt = IMG_TX_DONE;
			ch_id = pframe->channel_id;
			ret = camera_enqueue(&module->frm_queue, pframe);
			complete(&module->frm_com);
			pr_debug("ch %d get out frame: %p, evt %d\n",
					ch_id, pframe, pframe->evt);
		} else {
			cambuf_put_ionbuf(&pframe->buf);
			put_empty_frame(pframe);
		}
		break;

	default:
		pr_err("unsupported cb cmd: %d\n", type);
		break;
	}

	return ret;
}

int dcam_callback(enum dcam_cb_type type, void *param, void *priv_data)
{
	int ret = 0;
	struct camera_frame *pframe;
	struct camera_module *module;
	struct channel_context *channel;
	struct isp_offline_param *cur;

	if (!param || !priv_data) {
		pr_err("Input ptr is NULL\n");
		return -EFAULT;
	}

	module = (struct camera_module *)priv_data;
	if (type == DCAM_CB_DEV_ERR) {
		pr_err("DCAM error happens. camera %d\n", module->idx);
		dcam_ops->stop(module->dcam_dev_handle);

		pframe = get_empty_frame();
		pframe->evt = IMG_TX_ERR;
		pframe->irq_type = CAMERA_IRQ_IMG;
		ret = camera_enqueue(&module->frm_queue, pframe);
		complete(&module->frm_com);
		return 0;
	}

	pframe = (struct camera_frame *)param;
	pframe->priv_data = NULL;
	channel = &module->channel[pframe->channel_id];

	pr_debug("module %p, cam%d ch %d.  cb cmd %d, frame %p\n",
		module, module->idx, pframe->channel_id, type, pframe);

	switch (type) {
	case DCAM_CB_DATA_DONE:
		if (pframe->buf.addr_k[0]) {
			uint32_t *ptr = (uint32_t *)pframe->buf.addr_k[0];

			pr_debug("dcam path %d. outdata: %08x %08x %08x %08x\n",
				channel->dcam_path_id, ptr[0], ptr[1], ptr[2], ptr[3]);
		}

		if (atomic_read(&module->state) != CAM_RUNNING) {
			pr_info("stream off. put frame %p\n", pframe);
			if (pframe->buf.type == CAM_BUF_KERNEL) {
				camera_enqueue(&channel->share_buf_queue, pframe);
			} else {
				/* 4in1 or raw buffer is allocate from user */
				cambuf_put_ionbuf(&pframe->buf);
				put_empty_frame(pframe);
			}
		} else if (channel->ch_id == CAM_CH_RAW) {
			/* RAW capture or test_dcam only */
			pframe->evt = IMG_TX_DONE;
			pframe->irq_type = CAMERA_IRQ_IMG;
			ret = camera_enqueue(&module->frm_queue, pframe);
			complete(&module->frm_com);
			pr_info("get out raw frame: %p\n", pframe);

		} else if (channel->ch_id == CAM_CH_PRE) {

			pr_debug("proc isp path %d\n", channel->isp_path_id);
			/* ISP in_queue maybe overflow.
			 * If previous frame with size updating is dicarded by ISP,
			 * we should set it in current frame for ISP input
			 * If current frame also has new updating param,
			 * here will set it as previous one in a queue for ISP,
			 * ISP can check and free it.
			 */
			if (channel->isp_updata) {
				pr_info("next %p,  prev %p\n",
					pframe->param_data, channel->isp_updata);
				if (pframe->param_data) {
					cur = (struct isp_offline_param *)pframe->param_data;
					cur->prev = channel->isp_updata;
				} else {
					pframe->param_data = channel->isp_updata;
				}
				channel->isp_updata = NULL;
				pr_info("cur %p\n", pframe->param_data);
			}

			ret = isp_ops->proc_frame(module->isp_dev_handle, pframe,
					channel->isp_path_id >> ISP_CTXID_OFFSET);
			if (ret) {
				pr_warn_ratelimited("warning: isp proc frame failed.\n");
				/* ISP in_queue maybe overflow.
				 * If current frame taking (param_data ) for size updating
				 * we should hold it here and set it in next frame for ISP
				 */
				if (pframe->param_data) {
					cur = (struct isp_offline_param *)pframe->param_data;
					cur->prev = channel->isp_updata;
					channel->isp_updata = (void *)cur;
					pframe->param_data = NULL;
					pr_info("store:  cur %p   prev %p\n", cur, cur->prev);
				}
				ret = dcam_ops->cfg_path(module->dcam_dev_handle,
						DCAM_PATH_CFG_OUTPUT_BUF,
						channel->dcam_path_id, pframe);
				/* release sync if ISP don't need it */
				if (pframe->sync_data)
					dcam_if_release_sync(pframe->sync_data,
							     pframe);
			}
		} else if (channel->ch_id == CAM_CH_CAP) {

			if ((module->cap_status != CAM_CAPTURE_START) &&
				(module->cap_status != CAM_CAPTURE_RAWPROC)) {
				/*
				 * Release sync if we don't deliver this @pframe
				 * to ISP.
				 */
				if (pframe->sync_data)
					dcam_if_release_sync(pframe->sync_data,
							     pframe);
				ret = dcam_ops->cfg_path(
						module->dcam_dev_handle,
						DCAM_PATH_CFG_OUTPUT_BUF,
						channel->dcam_path_id, pframe);
				return ret;
			}

			if (module->cam_uinfo.is_4in1 &&
				(pframe->irq_type == CAMERA_IRQ_4IN1_DONE)) {
				/* 4in1 send buf to hal for remosaic */
				pframe->evt = IMG_TX_DONE;
				ret = camera_enqueue(&module->frm_queue, pframe);
				complete(&module->frm_com);
				pr_info("return 4in1 raw frame\n");
				return ret;
			}

			ret = camera_enqueue(&channel->share_buf_queue, pframe);
			if (ret) {
				pr_debug("capture queue overflow\n");
				ret = dcam_ops->cfg_path(
						module->dcam_dev_handle,
						DCAM_PATH_CFG_OUTPUT_BUF,
						channel->dcam_path_id, pframe);
			} else {
				complete(&module->cap_thrd.thread_com);
			}
		} else {
			/* should not be here */
			pr_warn("reset dcam path out %d for ch %d\n",
				channel->dcam_path_id, channel->ch_id);
			ret = dcam_ops->cfg_path(module->dcam_dev_handle,
						DCAM_PATH_CFG_OUTPUT_BUF,
						channel->dcam_path_id, pframe);
		}
		break;
	case DCAM_CB_STATIS_DONE:
		pframe->evt = IMG_TX_DONE;
		pframe->irq_type = CAMERA_IRQ_STATIS;
		/* temp: statis/irq share same queue with frame data. */
		/* todo: separate statis/irq and frame queue. */
		if (atomic_read(&module->state) == CAM_RUNNING) {
			ret = camera_enqueue(&module->frm_queue, pframe);
			complete(&module->frm_com);
			pr_debug("get statis frame: %p, type %d, %d\n",
				pframe, pframe->irq_type, pframe->irq_property);
		} else {
			put_empty_frame(pframe);
		}
		break;

	case DCAM_CB_IRQ_EVENT:
		if (pframe->irq_property == IRQ_DCAM_SN_EOF) {
			flash_ops->start_flash(module->flash_core_handle);
			put_empty_frame(pframe);
			break;
		}
		/* temp: statis/irq share same queue with frame data. */
		/* todo: separate statis/irq and frame queue. */
		if (atomic_read(&module->state) == CAM_RUNNING) {
			ret = camera_enqueue(&module->frm_queue, pframe);
			complete(&module->frm_com);
			pr_debug("get irq frame: %p, type %d, %d\n",
				pframe, pframe->irq_type, pframe->irq_property);
		} else {
			put_empty_frame(pframe);
		}
		break;

	case DCAM_CB_RET_SRC_BUF:
		pr_info("dcam ret src frame %p\n", pframe);
		if ((module->cap_status == CAM_CAPTURE_RAWPROC) ||
			(atomic_read(&module->state) != CAM_RUNNING)) {
			/* for case raw capture post-proccessing
			 * and case 4in1 after stream off
			 * just release it, no need to return
			 */
			cambuf_put_ionbuf(&pframe->buf);
			put_empty_frame(pframe);
			pr_info("cap status %d, state %d\n",
				module->cap_status,
					atomic_read(&module->state));
		} else if (module->cam_uinfo.is_4in1) {
			/* 4in1 capture case: dcam offline src buffer
			 * should be re-used for dcam online output (raw)
			 */
			dcam_ops->cfg_path(module->dcam_dev_handle,
				DCAM_PATH_CFG_OUTPUT_BUF,
				channel->dcam_path_id, pframe);
		} else {
			pr_err("todo: unknown case to handle\n");
			cambuf_put_ionbuf(&pframe->buf);
			put_empty_frame(pframe);
		}
		break;

	default:
		break;
	}

	return ret;
}

static inline uint32_t fix_scale(uint32_t size_in, uint32_t ratio16)
{
	uint64_t size_scaled;

	size_scaled = (uint64_t)size_in;
	size_scaled <<= (2 * RATIO_SHIFT);
	size_scaled = ((size_scaled / ratio16) >> RATIO_SHIFT);
	return (uint32_t)size_scaled;
}

static void get_diff_trim(struct sprd_img_rect *orig,
	uint32_t ratio16, struct img_trim *trim0, struct img_trim *trim1)
{
	trim1->start_x = fix_scale(orig->x - trim0->start_x, ratio16);
	trim1->start_y = fix_scale(orig->y - trim0->start_y, ratio16);
	trim1->size_x =  fix_scale(orig->w, ratio16);
	trim1->size_y =  fix_scale(orig->h, ratio16);
}

static inline void get_largest_crop(
	struct sprd_img_rect *crop_dst, struct sprd_img_rect *crop1)
{
	uint32_t end_x, end_y;
	uint32_t end_x_new, end_y_new;

	if (crop1) {
		end_x = crop_dst->x + crop_dst->w;
		end_y = crop_dst->y + crop_dst->h;
		end_x_new = crop1->x + crop1->w;
		end_y_new = crop1->y + crop1->h;

		crop_dst->x = MIN(crop1->x, crop_dst->x);
		crop_dst->y = MIN(crop1->y, crop_dst->y);
		end_x_new = MAX(end_x, end_x_new);
		end_y_new = MAX(end_y, end_y_new);
		crop_dst->w = end_x_new - crop_dst->x;
		crop_dst->h = end_y_new - crop_dst->y;
	}
}

/* align dcam output w to 16 for debug convenience */
static void align_w16(struct img_size *in, struct img_size *out, uint32_t *ratio)
{
	struct img_size align;

	align.w = (out->w + 15) & (~0xF);
	if (align.w <= in->w) {
		*ratio = (1 << RATIO_SHIFT) * in->w / align.w;
		align.h = fix_scale(in->h, *ratio);
		align.h = (align.h + 1) & ~1;
		*out = align;
	}
}

static int cal_channel_size(struct camera_module *module)
{
	uint32_t ratio_min, is_same_fov = 0;
	uint32_t ratio_p_w, ratio_p_h;
	uint32_t ratio_v_w, ratio_v_h;
	uint32_t end_x, end_y;
	uint32_t illegal;
	struct channel_context *ch_prev;
	struct channel_context *ch_vid;
	struct channel_context *ch_cap;
	struct sprd_img_rect *crop_p, *crop_v, *crop_c;
	struct sprd_img_rect crop_dst;
	struct img_trim trim_pv, trim_c;
	struct img_size dst_p, dst_v, dcam_out, max, temp;

	ch_prev = &module->channel[CAM_CH_PRE];
	ch_cap = &module->channel[CAM_CH_CAP];
	ch_vid = &module->channel[CAM_CH_VID];
	if (!ch_prev->enable && !ch_cap->enable && !ch_vid->enable)
		return 0;

	dst_p.w = dst_p.h = 1;
	dst_v.w = dst_v.h = 1;
	ratio_v_w = ratio_v_h = 1;
	crop_p = crop_v = crop_c = NULL;
	if (ch_prev->enable) {
		crop_p = &ch_prev->ch_uinfo.src_crop;
		dst_p.w = ch_prev->ch_uinfo.dst_size.w;
		dst_p.h = ch_prev->ch_uinfo.dst_size.h;
		pr_info("src crop prev %d %d %d %d\n", crop_p->x, crop_p->y,
			crop_p->x + crop_p->w, crop_p->y + crop_p->h);
	}

	if (ch_vid->enable) {
		crop_v = &ch_vid->ch_uinfo.src_crop;
		dst_v.w = ch_vid->ch_uinfo.dst_size.w;
		dst_v.h = ch_vid->ch_uinfo.dst_size.h;
		pr_info("src crop vid %d %d %d %d\n", crop_v->x, crop_v->y,
			crop_v->x + crop_v->w, crop_v->y + crop_v->h);
	}

	if (ch_cap->enable && ch_prev->enable &&
		(ch_cap->mode_ltm == MODE_LTM_PRE)) {
		crop_c = &ch_cap->ch_uinfo.src_crop;
		is_same_fov = 1;
		pr_info("src crop cap %d %d %d %d\n", crop_c->x, crop_c->y,
			crop_c->x + crop_c->w, crop_c->y + crop_c->h);
	}

	if (ch_prev->enable) {
		crop_dst = *crop_p;
		get_largest_crop(&crop_dst, crop_v);
		get_largest_crop(&crop_dst, crop_c);
		end_x = (crop_dst.x + crop_dst.w + 3) & ~3;
		end_y = (crop_dst.y + crop_dst.h + 3) & ~3;
		trim_pv.start_x = crop_dst.x & ~3;
		trim_pv.start_y = crop_dst.y & ~3;
		trim_pv.size_x = end_x - trim_pv.start_x;
		trim_pv.size_y = end_y - trim_pv.start_y;
	}

	if (is_same_fov)
		trim_c = trim_pv;
	else if (ch_cap->enable) {
		trim_c.start_x = ch_cap->ch_uinfo.src_crop.x & ~1;
		trim_c.start_y = ch_cap->ch_uinfo.src_crop.y & ~1;
		trim_c.size_x = (ch_cap->ch_uinfo.src_crop.w + 1) & ~1;
		trim_c.size_y = (ch_cap->ch_uinfo.src_crop.h + 1) & ~1;
	}

	pr_info("trim_pv: %d %d %d %d\n", trim_pv.start_x, trim_pv.start_y,
		trim_pv.start_x + trim_pv.size_x, trim_pv.start_y + trim_pv.size_y);
	pr_info("trim_c: %d %d %d %d\n", trim_c.start_x, trim_c.start_y,
		trim_c.start_x + trim_c.size_x, trim_c.start_y + trim_c.size_y);

	if (ch_prev->enable) {
		if (module->zoom_solution == 0) {
			if (module->cam_uinfo.sn_size.w > ISP_MAX_LINE_WIDTH)
				ratio_min = (2 << RATIO_SHIFT);
			else
				ratio_min = (1 << RATIO_SHIFT);
		} else {
			ratio_p_w = (1 << RATIO_SHIFT) * crop_p->w / dst_p.w;
			ratio_p_h = (1 << RATIO_SHIFT) * crop_p->h / dst_p.h;
			ratio_min = MIN(ratio_p_w, ratio_p_h);
			if (ch_vid->enable) {
				ratio_v_w = (1 << RATIO_SHIFT) * crop_v->w / dst_v.w;
				ratio_v_h = (1 << RATIO_SHIFT) * crop_v->h / dst_v.h;
				ratio_min = MIN(ratio_min, MIN(ratio_v_w, ratio_v_h));
			}
			ratio_min = MIN(ratio_min, ((module->rds_limit << RATIO_SHIFT) / 10));
			ratio_min = MAX(ratio_min, (1 << RATIO_SHIFT));
			pr_info("ratio_p %d %d, ratio_v %d %d ratio_min %d\n",
				ratio_p_w, ratio_p_h, ratio_v_w, ratio_v_h, ratio_min);
		}

		dcam_out.w = fix_scale(trim_pv.size_x, ratio_min);
		dcam_out.w = (dcam_out.w + 1) & ~1;
		dcam_out.h = fix_scale(trim_pv.size_y, ratio_min);
		dcam_out.h = (dcam_out.h + 1) & ~1;
		temp.w = trim_pv.size_x;
		temp.h = trim_pv.size_y;
		align_w16(&temp, &dcam_out, &ratio_min);

		pr_info("dst_p %d %d, dst_v %d %d, dcam_out %d %d\n",
			dst_p.w, dst_p.h, dst_v.w, dst_v.h, dcam_out.w, dcam_out.h);

		ch_prev->trim_dcam = trim_pv;
		ch_prev->rds_ratio = ratio_min;
		ch_prev->dst_dcam = dcam_out;

		get_diff_trim(&ch_prev->ch_uinfo.src_crop,
			ratio_min, &trim_pv, &ch_prev->trim_isp);

		illegal = ((ch_prev->trim_isp.start_x + ch_prev->trim_isp.size_x) > dcam_out.w);
		illegal |= ((ch_prev->trim_isp.start_y + ch_prev->trim_isp.size_y) > dcam_out.h);
		pr_info("trim isp, prev %d %d %d %d\n",
			ch_prev->trim_isp.start_x, ch_prev->trim_isp.start_y,
			ch_prev->trim_isp.size_x, ch_prev->trim_isp.size_y);

		if (ch_vid->enable) {
			get_diff_trim(&ch_vid->ch_uinfo.src_crop,
				ratio_min, &trim_pv, &ch_vid->trim_isp);
			illegal |= ((ch_vid->trim_isp.start_x + ch_vid->trim_isp.size_x) > dcam_out.w);
			illegal |= ((ch_vid->trim_isp.start_y + ch_vid->trim_isp.size_y) > dcam_out.h);
			pr_info("trim isp, vid %d %d %d %d\n",
				ch_vid->trim_isp.start_x, ch_vid->trim_isp.start_y,
				ch_vid->trim_isp.size_x, ch_vid->trim_isp.size_y);
		}
		if (illegal)
			pr_err("fatal error. trim size out of range.");
	}

	/* calculate possible max swap buffer size before stream on*/
	if (ch_prev->enable && ch_prev->swap_size.w == 0) {
		max.w = module->cam_uinfo.sn_rect.w;
		max.h = module->cam_uinfo.sn_rect.h;
		if (module->zoom_solution == 0) {
			if (module->cam_uinfo.sn_size.w > ISP_MAX_LINE_WIDTH)
				ratio_min = (2 << RATIO_SHIFT);
			else
				ratio_min = (1 << RATIO_SHIFT);
		} else {
			ratio_p_w = (1 << RATIO_SHIFT) * max.w / dst_p.w;
			ratio_p_h = (1 << RATIO_SHIFT) * max.h / dst_p.h;
			ratio_min = MIN(ratio_p_w, ratio_p_h);
			if (ch_vid->enable) {
				ratio_v_w = (1 << RATIO_SHIFT) * max.w / dst_v.w;
				ratio_v_h = (1 << RATIO_SHIFT) * max.h / dst_v.h;
				ratio_min = MIN(ratio_min, MIN(ratio_v_w, ratio_v_h));
			}
			ratio_min = MIN(ratio_min, ((module->rds_limit << RATIO_SHIFT) / 10));
			ratio_min = MAX(ratio_min, (1 << RATIO_SHIFT));
		}

		max.w = fix_scale(max.w, ratio_min);
		max.w = (max.w + 1) & ~1;
		max.h = fix_scale(max.h, ratio_min);
		max.h = (max.h + 1) & ~1;
		temp.w = module->cam_uinfo.sn_rect.w;
		temp.h = module->cam_uinfo.sn_rect.h;
		align_w16(&temp, &max, &ratio_min);

		ch_prev->swap_size = max;
		pr_info("prev path swap size %d %d\n",
			ch_prev->swap_size.w, ch_prev->swap_size.h);
	}

	if (ch_cap->enable) {
		ch_cap->trim_dcam = trim_c;
		ch_cap->swap_size.w = ch_cap->ch_uinfo.src_size.w;
		ch_cap->swap_size.h = ch_cap->ch_uinfo.src_size.h;
		get_diff_trim(&ch_cap->ch_uinfo.src_crop,
			(1 << RATIO_SHIFT), &trim_c, &ch_cap->trim_isp);
		pr_info("trim isp, cap %d %d %d %d\n",
				ch_cap->trim_isp.start_x, ch_cap->trim_isp.start_y,
				ch_cap->trim_isp.size_x, ch_cap->trim_isp.size_y);
	}
	pr_info("done.\n");
	return 0;
}

static int config_channel_size(
	struct camera_module *module,
	struct channel_context *channel)
{
	int ret = 0;
	int is_zoom, loop_count;
	uint32_t isp_ctx_id, isp_path_id;
	struct isp_offline_param *isp_param;
	struct channel_context *vid;
	struct camera_uchannel *ch_uinfo = NULL;
	struct dcam_path_cfg_param ch_desc;
	struct isp_ctx_size_desc ctx_size;
	struct img_trim path_trim;

	if ((atomic_read(&module->state) == CAM_RUNNING)) {
		is_zoom = 1;
		loop_count = 5;
	} else {
		is_zoom = 0;
		loop_count = 1;
	}

	ch_uinfo = &channel->ch_uinfo;
	/* DCAM full path not updating for zoom. */
	if (is_zoom && channel->ch_id == CAM_CH_CAP)
		goto cfg_isp;

	if (!is_zoom && (channel->swap_size.w > 0)) {
		camera_queue_init(&channel->share_buf_queue,
			CAM_SHARED_BUF_NUM, 0, put_k_frame);

		/* alloc middle buffer for channel */
		channel->alloc_start = 1;
		channel->alloc_buf_work.priv_data = module;
		atomic_set(&channel->alloc_buf_work.status, CAM_WORK_PENDING);
		INIT_WORK(&channel->alloc_buf_work.work, alloc_buffers);
		pr_info("module %p, channel %d, size %d %d\n", module,
			channel->ch_id, channel->swap_size.w, channel->swap_size.h);
		queue_work(module->workqueue, &channel->alloc_buf_work.work);
	}

	memset(&ch_desc, 0, sizeof(ch_desc));
	ch_desc.input_size.w = ch_uinfo->src_size.w;
	ch_desc.input_size.h = ch_uinfo->src_size.h;
	if (channel->ch_id == CAM_CH_CAP) {
		/* no trim in dcam full path. */
		ch_desc.output_size = ch_desc.input_size;
		ch_desc.input_trim.start_x = 0;
		ch_desc.input_trim.start_y = 0;
		ch_desc.input_trim.size_x = ch_desc.input_size.w;
		ch_desc.input_trim.size_y = ch_desc.input_size.h;
	} else {
		if (module->zoom_solution == 0)
			ch_desc.force_rds = 0;
		else
			ch_desc.force_rds = 1;
		ch_desc.input_trim = channel->trim_dcam;
		ch_desc.output_size = channel->dst_dcam;
	}

	pr_info("update dcam path %d size for channel %d\n",
		channel->dcam_path_id, channel->ch_id);

	if (channel->ch_id == CAM_CH_PRE) {
		isp_param = kzalloc(sizeof(struct isp_offline_param), GFP_KERNEL);
		if (isp_param == NULL) {
			pr_err("fail to alloc memory.\n");
			return -ENOMEM;
		}
		ch_desc.priv_size_data = (void *)isp_param;
		isp_param->valid |= ISP_SRC_SIZE;
		isp_param->src_info.src_size = ch_desc.input_size;
		isp_param->src_info.src_trim = ch_desc.input_trim;
		isp_param->src_info.dst_size = ch_desc.output_size;
		isp_param->valid |= ISP_PATH0_TRIM;
		isp_param->trim_path[0] = channel->trim_isp;
		vid = &module->channel[CAM_CH_VID];
		if (vid->enable) {
			isp_param->valid |= ISP_PATH1_TRIM;
			isp_param->trim_path[1] = vid->trim_isp;
		}
		pr_info("isp_param %p\n", isp_param);
	}

	do {
		ret = dcam_ops->cfg_path(module->dcam_dev_handle,
				DCAM_PATH_CFG_SIZE,
				channel->dcam_path_id, &ch_desc);
		if (ret) {
			/* todo: if previous updating is not applied yet.
			 * this case will happen.
			 * (zoom ratio changes in short gap)
			 * wait here and retry(how long?)
			 */
			pr_err("update dcam path %d size failed, zoom %d, lp %d\n",
				channel->dcam_path_id, is_zoom, loop_count);
			msleep(20);
		} else {
			break;
		}
	} while (--loop_count);

	if (ret && ch_desc.priv_size_data)
		kfree(ch_desc.priv_size_data);

	/* isp path for prev/video will update from input frame. */
	if (channel->ch_id == CAM_CH_PRE) {
		pr_info("update channel size done for preview\n");
		return ret;
	}

cfg_isp:
	isp_ctx_id = (channel->isp_path_id >> ISP_CTXID_OFFSET);
	isp_path_id = (channel->isp_path_id & ISP_PATHID_MASK);

	if (channel->ch_id == CAM_CH_CAP) {
		pr_info("cfg isp ctx %d size\n", isp_ctx_id);
		ctx_size.src.w = ch_uinfo->src_size.w;
		ctx_size.src.h = ch_uinfo->src_size.h;
		ctx_size.crop = channel->trim_dcam;
		ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_CTX_SIZE,
				isp_ctx_id, 0, &ctx_size);
	}
	path_trim = channel->trim_isp;

cfg_path:
	pr_info("cfg isp ctx %d path %d size\n", isp_ctx_id, isp_path_id);
	ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_PATH_SIZE,
				isp_ctx_id, isp_path_id,  &path_trim);
	if (channel->ch_id == CAM_CH_CAP && is_zoom) {
		channel = &module->channel[CAM_CH_CAP_THM];
		if (channel->enable) {
			isp_path_id = (channel->isp_path_id & ISP_PATHID_MASK);
			goto cfg_path;
		}
	}
	pr_info("update channel size done for CAP\n");
	return ret;
}

static int zoom_proc(void *param)
{
	int ret = 0;
	int update_pv = 0, update_c = 0;
	int update_always = 0;
	struct camera_module *module;
	struct channel_context *ch_prev, *ch_vid, *ch_cap;
	struct camera_frame *pre_zoom_coeff = NULL;
	struct camera_frame *vid_zoom_coeff = NULL;
	struct camera_frame *cap_zoom_coeff = NULL;
	struct sprd_img_rect *crop;

	module = (struct camera_module *)param;
	ch_prev = &module->channel[CAM_CH_PRE];
	ch_cap = &module->channel[CAM_CH_CAP];
	ch_vid = &module->channel[CAM_CH_VID];
next:
	pre_zoom_coeff = vid_zoom_coeff = cap_zoom_coeff =NULL;
	update_pv = update_c = update_always = 0;
	/* Get node from the preview/video/cap coef queue if exist */
	if (ch_prev->enable)
		pre_zoom_coeff = camera_dequeue(&ch_prev->zoom_coeff_queue);
	if (pre_zoom_coeff) {
		crop = (struct sprd_img_rect *)pre_zoom_coeff->priv_data;
		ch_prev->ch_uinfo.src_crop = *crop;
		kfree(crop);
		put_empty_frame(pre_zoom_coeff);
		update_pv |= 1;
	}

	if (ch_vid->enable)
		vid_zoom_coeff = camera_dequeue(&ch_vid->zoom_coeff_queue);
	if (vid_zoom_coeff) {
		crop = (struct sprd_img_rect *)vid_zoom_coeff->priv_data;
		ch_vid->ch_uinfo.src_crop = *crop;
		kfree(crop);
		put_empty_frame(vid_zoom_coeff);
		update_pv |= 1;
	}

	if (ch_cap->enable)
		cap_zoom_coeff = camera_dequeue(&ch_cap->zoom_coeff_queue);
	if (cap_zoom_coeff) {
		crop = (struct sprd_img_rect *)cap_zoom_coeff->priv_data;
		ch_cap->ch_uinfo.src_crop = *crop;
		kfree(crop);
		put_empty_frame(cap_zoom_coeff);
		update_c |= 1;
	}

	if (update_pv || update_c) {
		if (ch_cap->enable && (ch_cap->mode_ltm == MODE_LTM_CAP))
			update_always = 1;

		ret = cal_channel_size(module);
		if (ch_cap->enable && (update_c || update_always))
			config_channel_size(module, ch_cap);
		if (ch_prev->enable && (update_pv || update_always))
			config_channel_size(module, ch_prev);
		goto next;
	}
	return 0;
}

static int capture_proc(void *param)
{
	int ret = 0;
	struct camera_module *module;
	struct camera_frame *pframe;
	struct channel_context *channel;

	module = (struct camera_module *)param;
	channel = &module->channel[CAM_CH_CAP];
	pframe = camera_dequeue(&channel->share_buf_queue);
	if (!pframe)
		return 0;

	ret = -1;
	if (module->cap_status != CAM_CAPTURE_STOP) {
		pr_info("capture frame No.%d\n", pframe->fid);
		ret = isp_ops->proc_frame(module->isp_dev_handle, pframe,
			channel->isp_path_id >> ISP_CTXID_OFFSET);
	}

	if (ret) {
		pr_info("capture stop or isp queue overflow\n");
		ret = dcam_ops->cfg_path(
				module->dcam_dev_handle,
				DCAM_PATH_CFG_OUTPUT_BUF,
				channel->dcam_path_id, pframe);
	}
	return 0;
}


#define CAMERA_DUMP_PATH "/data/ylog/"
/* will create thread in user to read raw buffer*/
#define BYTE_PER_ONCE 4096
static void write_image_to_file(uint8_t *buffer,
	ssize_t size, const char *file)
{
	ssize_t result = 0, total = 0, writ = 0;
	struct file *wfp;

	wfp = filp_open(file, O_CREAT|O_RDWR, 0666);
	if (IS_ERR_OR_NULL(wfp)) {
		pr_err("fail to open file %s\n", file);
		return;
	}
	pr_debug("write image buf=%p, size=%d\n", buffer, (uint32_t)size);
	do {
		writ = (BYTE_PER_ONCE < size) ? BYTE_PER_ONCE : size;
		result = kernel_write(wfp, buffer, writ, &wfp->f_pos);
		pr_debug("write result: %d, size: %d, pos: %d\n",
		(uint32_t)result,  (uint32_t)size, (uint32_t)wfp->f_pos);

		if (result > 0) {
			size -= result;
			buffer += result;
		}
		total += result;
	} while ((result > 0) && (size > 0));
	filp_close(wfp, NULL);
	pr_debug("write image done, total=%d \n", (uint32_t)total);
}

static int dump_one_frame  (struct camera_module *module,
	struct camera_frame *pframe)
{
	ssize_t size = 0;
	struct channel_context *channel;
	enum cam_ch_id ch_id;
	uint8_t file_name[256] = { '\0' };
	uint8_t tmp_str[20] = { '\0' };

	ch_id = pframe->channel_id;
	channel = &module->channel[ch_id];
	strcat(file_name, CAMERA_DUMP_PATH);
	if (ch_id == CAM_CH_PRE)
		strcat(file_name,"prevraw_");
	else
		strcat(file_name,"capraw_");

	sprintf(tmp_str, "%d.", (uint32_t)module->cur_dump_ts.tv_sec);
	strcat(file_name, tmp_str);
	sprintf(tmp_str, "%06d", (uint32_t)(module->cur_dump_ts.tv_nsec / NSEC_PER_USEC));
	strcat(file_name, tmp_str);

	sprintf(tmp_str, "_w%d", pframe->width);
	strcat(file_name, tmp_str);
	sprintf(tmp_str, "_h%d", pframe->height);
	strcat(file_name, tmp_str);

	sprintf(tmp_str, "_No%d", pframe->fid);
	strcat(file_name,tmp_str);
	strcat(file_name,".mipi_raw");

	size = cal_sprd_raw_pitch(pframe->width) * pframe->height;
	write_image_to_file((char*)pframe->buf.addr_k[0],size, file_name);

	pr_debug("dump for ch %d, size %d, kaddr %p, file %s\n", ch_id,
		(int)size, (void *)pframe->buf.addr_k[0], file_name);

	/* return it to dcam output queue */
	dcam_ops->cfg_path(module->dcam_dev_handle,
			DCAM_PATH_CFG_OUTPUT_BUF,
			channel->dcam_path_id, pframe);
	return 0;
}

static int dumpraw_proc(void *param)
{
	uint32_t idx, cnt = 0;
	struct camera_module *module;
	struct camera_frame *pframe = NULL;
	struct cam_dbg_dump *dbg = &g_dbg_dump;

	pr_info("enter. %p\n", param);
	module = (struct camera_module *)param;
	idx = module->dcam_idx;
	if (idx > 1 || !module->dcam_dev_handle)
		return 0;

	mutex_lock(&dbg->dump_lock);
	dbg->dump_ongoing |= (1 << idx);
	module->dump_count = dbg->dump_count;
	init_completion(&module->dump_com);
	mutex_unlock(&dbg->dump_lock);

	pr_info("start dump count: %d\n", module->dump_count);
	while (module->dump_count) {
		module->in_dump = 1;
		ktime_get_ts(&module->cur_dump_ts);
		if (wait_for_completion_interruptible(
			&module->dump_com) == 0) {
			if ((atomic_read(&module->state) != CAM_RUNNING) ||
				(module->dump_count == 0))
				break;
			pframe = camera_dequeue(&module->dump_queue);
			if (!pframe)
				break;
			dump_one_frame(module, pframe);
			cnt++;
		} else {
			pr_debug("dump raw proc exit.");
			break;
		}
		module->dump_count--;
	}
	module->dump_count = 0;
	module->in_dump = 0;
	pr_info("end dump, real cnt %d\n", cnt);

	mutex_lock(&dbg->dump_lock);
	dbg->dump_count = 0;
	dbg->dump_ongoing &= ~(1 << idx);
	mutex_unlock(&dbg->dump_lock);
	return 0;
}

static int init_4in1_aux(struct camera_module *module,
			struct channel_context *channel)
{
	int ret = 0;
	uint32_t dcam_idx = 1;
	uint32_t dcam_path_id;
	void *dcam = NULL;
	struct camera_group *grp = module->grp;
	struct dcam_path_cfg_param ch_desc;

	dcam = module->aux_dcam_dev;
	if (dcam == NULL) {
		dcam = dcam_if_get_dev(dcam_idx, grp->dcam[dcam_idx]);
		if (IS_ERR_OR_NULL(dcam)) {
			pr_err("fail to get dcam%d\n", dcam_idx);
			return -EFAULT;
		}
		module->aux_dcam_dev = dcam;
		module->aux_dcam_id = dcam_idx;
	}

	ret = dcam_ops->open(module->aux_dcam_dev);
	if (ret < 0) {
		pr_err("fail to open aux dcam dev\n");
		ret = -EFAULT;
		goto exit_dev;
	}
	ret = dcam_ops->set_callback(module->aux_dcam_dev,
				dcam_callback, module);
	if (ret) {
		pr_err("fail to set aux dcam callback\n");
		ret = -EFAULT;
		goto exit_close;
	}
	/* todo: will update after dcam offline ctx done. */
	dcam_path_id = DCAM_PATH_BIN;
	ret = dcam_ops->get_path(module->aux_dcam_dev,
				dcam_path_id);
	if (ret < 0) {
		pr_err("fail to get dcam path %d\n", dcam_path_id);
		ret = -EFAULT;
		goto exit_close;
	} else {
		channel->aux_dcam_path_id = dcam_path_id;
		pr_info("get aux dcam path %d\n", dcam_path_id);
	}

	/* cfg dcam1 bin path */
	memset(&ch_desc, 0, sizeof(ch_desc));
	ch_desc.endian.y_endian = ENDIAN_LITTLE;
	ch_desc.input_size.w = channel->ch_uinfo.src_size.w;
	ch_desc.input_size.h = channel->ch_uinfo.src_size.h;
	/* dcam1 not trim, do it by isp */
	ch_desc.input_trim.size_x = channel->ch_uinfo.src_size.w;
	ch_desc.input_trim.size_y = channel->ch_uinfo.src_size.h;
	ch_desc.output_size.w = ch_desc.input_trim.size_x;
	ch_desc.output_size.h = ch_desc.input_trim.size_y;
	ret = dcam_ops->cfg_path(module->aux_dcam_dev,
					DCAM_PATH_CFG_BASE,
					channel->aux_dcam_path_id,
					&ch_desc);
	ret = dcam_ops->cfg_path(module->aux_dcam_dev,
					DCAM_PATH_CFG_SIZE,
					channel->aux_dcam_path_id,
					&ch_desc);
	/* 4in1 not choose 1 from 3 frames, TODO
	 * channel->frm_cnt = (uint32_t)(-3);
	 */

	pr_info("done\n");
	return ret;

exit_close:
	dcam_ops->close(module->aux_dcam_dev);
exit_dev:
	dcam_if_put_dev(module->aux_dcam_dev);
	module->aux_dcam_dev = NULL;
	return ret;
}

static int deinit_4in1_aux(struct camera_module *module)
{
	int ret = 0;
	void *dev;

	pr_info("E\n");
	dev = module->aux_dcam_dev;
	ret = dcam_ops->put_path(dev, DCAM_PATH_BIN);
	ret += dcam_ops->close(dev);
	ret += dcam_if_put_dev(dev);
	module->aux_dcam_dev = NULL;
	pr_info("Done, ret = %d\n", ret);

	return ret;
}

static int init_cam_channel(
			struct camera_module *module,
			struct channel_context *channel)
{
	int ret = 0;
	int isp_ctx_id = 0, isp_path_id = 0, dcam_path_id = 0;
	int slave_path_id = 0;
	int new_isp_ctx, new_isp_path, new_dcam_path;
	struct channel_context *channel_prev = NULL;
	struct channel_context *channel_cap = NULL;
	struct camera_uchannel *ch_uinfo;
	struct isp_path_base_desc path_desc;
	struct isp_init_param init_param;

	/* for debug. */
	module->zoom_solution = g_dbg_zoom_mode;
	pr_info("zoom_solution %d\n", module->zoom_solution);

	ch_uinfo = &channel->ch_uinfo;
	ch_uinfo->src_size.w = module->cam_uinfo.sn_rect.w;
	ch_uinfo->src_size.h = module->cam_uinfo.sn_rect.h;
	new_isp_ctx = 0;
	new_isp_path = 0;
	new_dcam_path = 0;
	switch (channel->ch_id) {
	case CAM_CH_PRE:
		dcam_path_id = DCAM_PATH_BIN;
		isp_path_id = ISP_SPATH_CP;
		new_isp_ctx = 1;
		new_isp_path = 1;
		new_dcam_path = 1;

		module->rds_limit = g_dbg_rds_limit;
		if (module->zoom_solution == 0) {
			init_param.max_size.w = ch_uinfo->src_size.w / 2;
		} else {
			init_param.max_size.w = (ch_uinfo->src_size.w * 10 + 40) / module->rds_limit;
			if (init_param.max_size.w < ch_uinfo->dst_size.w)
				init_param.max_size.w = ch_uinfo->dst_size.w;
		}
		break;

	case CAM_CH_VID:
		/* only consider video/pre share same
		 * dcam path and isp ctx now.
		 */
		channel_prev = &module->channel[CAM_CH_PRE];
		if (channel_prev->enable == 0) {
			/* todo: video only support ?? */
			pr_err("vid channel is not independent from preview\n");
		}
		channel->dcam_path_id = channel_prev->dcam_path_id;
		isp_ctx_id = (channel_prev->isp_path_id >> ISP_CTXID_OFFSET);
		isp_path_id = ISP_SPATH_VID;
		new_isp_path = 1;
		break;

	case CAM_CH_CAP:
		dcam_path_id = DCAM_PATH_FULL;
		isp_path_id = ISP_SPATH_CP;
		new_isp_ctx = 1;
		new_isp_path = 1;
		new_dcam_path = 1;
		init_param.max_size.w = MAX(ch_uinfo->src_size.w, ch_uinfo->dst_size.w);
		break;

	case CAM_CH_PRE_THM:
		channel_prev = &module->channel[CAM_CH_PRE];
		if (channel_prev->enable == 0) {
			pr_err("error: preview channel is not enable.\n");
			return -EINVAL;
		}
		channel->dcam_path_id = channel_prev->dcam_path_id;
		isp_ctx_id = (channel_prev->isp_path_id >> ISP_CTXID_OFFSET);
		isp_path_id = ISP_SPATH_FD;
		new_isp_path = 1;
		break;

	case CAM_CH_CAP_THM:
		channel_cap = &module->channel[CAM_CH_CAP];
		if (channel_cap->enable == 0) {
			pr_err("error: capture channel is not enable.\n");
			return -EINVAL;
		}
		channel->dcam_path_id = channel_cap->dcam_path_id;
		isp_ctx_id = (channel_prev->isp_path_id >> ISP_CTXID_OFFSET);
		isp_path_id = ISP_SPATH_FD;
		new_isp_path = 1;
		break;

	case CAM_CH_RAW:
		dcam_path_id = DCAM_PATH_VCH2;
		new_dcam_path = 1;
		break;

	default:
		pr_err("unknown channel id %d\n", channel->ch_id);
		return -EINVAL;
	}

	pr_info("ch %d, new: (%d %d %d)  path (%d %d %d)\n",
		channel->ch_id, new_isp_ctx, new_isp_path, new_dcam_path,
		isp_ctx_id, isp_path_id, dcam_path_id);

	if (new_dcam_path) {
		struct dcam_path_cfg_param ch_desc;

		ret = dcam_ops->get_path(
				module->dcam_dev_handle, dcam_path_id);
		if (ret < 0) {
			pr_err("fail to get dcam path %d\n", dcam_path_id);
			return -EFAULT;
		}
		channel->dcam_path_id = dcam_path_id;
		pr_info("get dcam path : %d\n", channel->dcam_path_id);

		/* todo: cfg param to user setting. */
		memset(&ch_desc, 0, sizeof(ch_desc));
		ch_desc.is_loose =
			module->cam_uinfo.sensor_if.if_spec.mipi.is_loose;
		/*
		 * Configure slow motion for BIN path. HAL must set @is_high_fps
		 * and @high_fps_skip_num for both preview channel and video
		 * channel so BIN path can enable slow motion feature correctly.
		 */
		ch_desc.enable_slowmotion = ch_uinfo->is_high_fps;
		ch_desc.slowmotion_count = ch_uinfo->high_fps_skip_num;

		ch_desc.endian.y_endian = ENDIAN_LITTLE;
		ch_desc.enable_3dnr = module->cam_uinfo.is_3dnr;
		if (channel->ch_id == CAM_CH_RAW)
			ch_desc.is_raw = 1;
		if ((channel->ch_id == CAM_CH_CAP) && module->cam_uinfo.is_4in1)
			ch_desc.is_raw = 1;
		ret = dcam_ops->cfg_path(module->dcam_dev_handle,
				DCAM_PATH_CFG_BASE,
				channel->dcam_path_id, &ch_desc);
	}

	if (new_isp_ctx) {
		struct isp_ctx_base_desc ctx_desc;
		init_param.is_high_fps = ch_uinfo->is_high_fps;
		ret = isp_ops->get_context(module->isp_dev_handle, &init_param);
		if (ret < 0) {
			pr_err("fail to get isp context for size %d %d.\n",
					init_param.max_size.w, init_param.max_size.h);
			goto exit;
		}
		isp_ctx_id = ret;
		isp_ops->set_callback(module->isp_dev_handle,
				isp_ctx_id, isp_callback, module);

		/* todo: cfg param to user setting. */
		ctx_desc.in_fmt = ch_uinfo->sn_fmt;
		ctx_desc.bayer_pattern = module->cam_uinfo.sensor_if.img_ptn;
		ctx_desc.fetch_fbd = 0;
		ctx_desc.mode_ltm = MODE_LTM_OFF;
		ctx_desc.mode_3dnr = MODE_3DNR_OFF;
		ctx_desc.enable_slowmotion = ch_uinfo->is_high_fps;
		ctx_desc.slowmotion_count = ch_uinfo->high_fps_skip_num;
		ctx_desc.slw_state = CAM_SLOWMOTION_OFF;
		if (module->cam_uinfo.is_3dnr) {
			if (channel->ch_id == CAM_CH_CAP) {
				channel->type_3dnr = CAM_3DNR_SW;
				ctx_desc.mode_3dnr = MODE_3DNR_OFF;
			} else if (channel->ch_id == CAM_CH_PRE) {
				channel->type_3dnr = CAM_3DNR_HW;
				ctx_desc.mode_3dnr = MODE_3DNR_PRE;
			}
		}
		if (module->cam_uinfo.is_ltm) {
			if (channel->ch_id == CAM_CH_CAP) {
				channel->mode_ltm = MODE_LTM_CAP;
				ctx_desc.mode_ltm = MODE_LTM_CAP;
			} else if (channel->ch_id == CAM_CH_PRE) {
				channel->mode_ltm = MODE_LTM_PRE;
				ctx_desc.mode_ltm = MODE_LTM_PRE;
			}
		}
		ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_CTX_BASE, isp_ctx_id, 0, &ctx_desc);
	}

	if (new_isp_path) {
		ret = isp_ops->get_path(
				module->isp_dev_handle, isp_ctx_id, isp_path_id);
		if (ret < 0) {
			pr_err("fail to get isp path %d from context %d\n",
				isp_path_id, isp_ctx_id);
			if (new_isp_ctx)
				isp_ops->put_context(module->isp_dev_handle, isp_ctx_id);
			goto exit;
		}
		channel->isp_path_id =
			(int32_t)((isp_ctx_id << ISP_CTXID_OFFSET) | isp_path_id);
		pr_info("get isp path : 0x%x\n", channel->isp_path_id);

		memset(&path_desc, 0, sizeof(path_desc));
		if (channel->ch_uinfo.slave_img_en) {
			slave_path_id = ISP_SPATH_VID;
			path_desc.slave_type = ISP_PATH_MASTER;
			path_desc.slave_path_id = slave_path_id;
		}
		path_desc.out_fmt = ch_uinfo->dst_fmt;
		path_desc.endian.y_endian = ENDIAN_LITTLE;
		path_desc.endian.uv_endian = ENDIAN_LITTLE;
		path_desc.output_size.w = ch_uinfo->dst_size.w;
		path_desc.output_size.h = ch_uinfo->dst_size.h;
		ret = isp_ops->cfg_path(module->isp_dev_handle,
					ISP_PATH_CFG_PATH_BASE,
					isp_ctx_id, isp_path_id, &path_desc);
	}

	if (new_isp_path && channel->ch_uinfo.slave_img_en) {
		ret = isp_ops->get_path(module->isp_dev_handle,
				isp_ctx_id, slave_path_id);
		if (ret < 0) {
			pr_err("fail to get isp path %d from context %d\n",
				slave_path_id, isp_ctx_id);
			isp_ops->put_path(
				module->isp_dev_handle, isp_ctx_id, isp_path_id);
			if (new_isp_ctx)
				isp_ops->put_context(module->isp_dev_handle, isp_ctx_id);
			goto exit;
		}
		channel->slave_isp_path_id =
			(int32_t)((isp_ctx_id << ISP_CTXID_OFFSET) | slave_path_id);
		path_desc.slave_type = ISP_PATH_SLAVE;
		path_desc.out_fmt = ch_uinfo->slave_img_fmt;
		path_desc.endian.y_endian = ENDIAN_LITTLE;
		path_desc.endian.uv_endian = ENDIAN_LITTLE;
		path_desc.output_size.w = ch_uinfo->slave_img_size.w;
		path_desc.output_size.h = ch_uinfo->slave_img_size.h;
		ret = isp_ops->cfg_path(module->isp_dev_handle,
					ISP_PATH_CFG_PATH_BASE,
					isp_ctx_id, slave_path_id, &path_desc);
	}

	/* 4in1 setting */
	if (channel->ch_id == CAM_CH_CAP && module->cam_uinfo.is_4in1) {
		ret = init_4in1_aux(module, channel);
		if (ret < 0) {
			pr_err("init dcam for 4in1 error, ret = %d\n", ret);
			goto exit;
		}
	}
exit:
	pr_info("path id:dcam = %d, aux dcam = %d, isp = %d\n",
		channel->dcam_path_id, channel->aux_dcam_path_id,
		channel->isp_path_id);
	pr_info("ch %d done. ret = %d\n", channel->ch_id, ret);
	return ret;
}

static void cam_timer_callback(unsigned long data)
{
	struct camera_module *module = (struct camera_module *)data;
	struct camera_frame *frame;
	int ret = 0;

	if (!module || atomic_read(&module->state) != CAM_RUNNING) {
		pr_err("fail to get valid input ptr or error state.\n");
		return;
	}

	if (atomic_read(&module->timeout_flag) == 1) {
		pr_err("CAM%d timeout.\n", module->idx);
		frame = get_empty_frame();
		if (module->cap_status == CAM_CAPTURE_RAWPROC) {
			module->cap_status = CAM_CAPTURE_RAWPROC_DONE;
			frame->evt = IMG_TX_DONE;
			frame->irq_type = CAMERA_IRQ_DONE;
			frame->irq_property = IRQ_RAW_PROC_TIMEOUT;
		} else {
			frame->evt = IMG_TIMEOUT;
			frame->irq_type = CAMERA_IRQ_IMG;
			frame->irq_property = IRQ_MAX_DONE;
		}
		ret = camera_enqueue(&module->frm_queue, frame);
		complete(&module->frm_com);
		if (ret)
			pr_err("fail to enqueue.\n");
	}
}

static void sprd_init_timer(struct timer_list *cam_timer,
			unsigned long data)
{
	setup_timer(cam_timer, cam_timer_callback, data);
}

static int sprd_start_timer(struct timer_list *cam_timer,
			uint32_t time_val)
{
	int ret = 0;

	pr_debug("starting timer %ld\n", jiffies);
	ret = mod_timer(cam_timer, jiffies + msecs_to_jiffies(time_val));
	if (ret)
		pr_err("fail to start in mod_timer %d\n", ret);

	return ret;
}

static int sprd_stop_timer(struct timer_list *cam_timer)
{
	pr_debug("stop timer\n");
	del_timer_sync(cam_timer);
	return 0;
}

static int camera_thread_loop(void *arg)
{
	int idx;
	struct camera_module *module;
	struct cam_thread_info *thrd;

	if (!arg) {
		pr_err("fail to get valid input ptr\n");
		return -1;
	}

	thrd = (struct cam_thread_info *)arg;
	module = (struct camera_module *)thrd->ctx_handle;
	idx = module->idx;
	pr_info("%s loop starts\n", thrd->thread_name);
	while (1) {
		if (wait_for_completion_interruptible(
			&thrd->thread_com) == 0) {
			if (atomic_cmpxchg(&thrd->thread_stop, 1, 0) == 1) {
				pr_info("thread %s should stop.\n", thrd->thread_name);
				break;
			}
			pr_info("thread %s trigger\n", thrd->thread_name);
			thrd->proc_func(module);
		} else {
			pr_debug("thread %s exit!", thrd->thread_name);
			break;
		}
	}
	complete(&thrd->thread_stop_com);
	pr_info("%s thread stopped.\n", thrd->thread_name);
	return 0;
}

static int camera_create_thread(struct camera_module *module,
	struct cam_thread_info *thrd, void *func)
{
	thrd->ctx_handle = module;
	thrd->proc_func = func;
	atomic_set(&thrd->thread_stop, 0);
	init_completion(&thrd->thread_com);
	init_completion(&thrd->thread_stop_com);
	thrd->thread_task = kthread_run(camera_thread_loop,
				thrd, thrd->thread_name);
	if (IS_ERR_OR_NULL(thrd->thread_task)) {
		pr_err("fail to start thread %s\n", thrd->thread_name);
		thrd->thread_task = NULL;
		return -EFAULT;
	}
	return 0;
}

static void camera_stop_thread(struct cam_thread_info *thrd)
{
	unsigned long timeleft = 0;

	if (thrd->thread_task) {
		atomic_set(&thrd->thread_stop, 1);
		complete(&thrd->thread_com);
		timeleft = wait_for_completion_timeout(&thrd->thread_stop_com,
				msecs_to_jiffies(THREAD_STOP_TIMEOUT));
		if (timeleft == 0)
			pr_err("%s thread stop timeout\n", thrd->thread_name);
		thrd->thread_task = NULL;
	}
}

static int camera_module_init(struct camera_module *module)
{
	int ret = 0;
	int ch;
	struct channel_context *channel;
	struct cam_thread_info *thrd;

	pr_info("sprd_img: camera dev %d init start!\n", module->idx);

	atomic_set(&module->state, CAM_INIT);
	mutex_init(&module->lock);
	init_completion(&module->frm_com);
	module->cap_status = CAM_CAPTURE_STOP;

	for (ch = 0; ch < CAM_CH_MAX; ch++) {
		channel = &module->channel[ch];
		channel->ch_id = ch;
		channel->dcam_path_id = -1;
		channel->isp_path_id = -1;
		init_completion(&channel->alloc_com);
	}

	/* create capture thread */
	thrd = &module->cap_thrd;
	sprintf(thrd->thread_name, "cam%d_capture", module->idx);
	ret = camera_create_thread(module, thrd, capture_proc);
	if (ret)
		goto exit;

	/* create zoom thread */
	thrd = &module->zoom_thrd;
	sprintf(thrd->thread_name, "cam%d_zoom", module->idx);
	ret = camera_create_thread(module, thrd, zoom_proc);
	if (ret)
		goto exit;

	if (g_dbg_dump.dump_en) {
		/* create dump thread */
		thrd = &module->dump_thrd;
		sprintf(thrd->thread_name, "cam%d_dumpraw", module->idx);
		ret = camera_create_thread(module, thrd, dumpraw_proc);
		if (ret)
			goto exit;
	}

	module->flash_core_handle = get_cam_flash_handle(module->idx);

	sprd_init_timer(&module->cam_timer, (unsigned long)module);
	module->attach_sensor_id = SPRD_SENSOR_ID_MAX + 1;
	module->is_smooth_zoom = 1; /* temp for smooth zoom */

	pr_info("module[%d] init OK %p!\n", module->idx, module);
	return 0;
exit:
	camera_stop_thread(&module->cap_thrd);
	camera_stop_thread(&module->zoom_thrd);
	camera_stop_thread(&module->dump_thrd);
	return ret;
}

static int camera_module_deinit(struct camera_module *module)
{
	put_cam_flash_handle(module->flash_core_handle);
	camera_stop_thread(&module->cap_thrd);
	camera_stop_thread(&module->zoom_thrd);
	camera_stop_thread(&module->dump_thrd);
	mutex_destroy(&module->lock);
	return 0;
}

/*---------------  Misc interface start --------------- */

static int img_ioctl_get_time(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_time utime;
	struct timespec ts;

	ktime_get_ts(&ts);
	utime.sec = ts.tv_sec;
	utime.usec = ts.tv_nsec / NSEC_PER_USEC;
	pr_debug("get_time %d.%06d\n", utime.sec, utime.usec);

	ret = copy_to_user((void __user *)arg, &utime,
				sizeof(struct sprd_img_time));
	if (unlikely(ret)) {
		pr_err("fail to put user info, ret %d\n", ret);
		return -EFAULT;
	}

	return 0;
}

static int img_ioctl_set_flash(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_set_flash set_param;

	ret = copy_from_user((void *)&set_param,
				(void __user *)arg,
				sizeof(struct sprd_img_set_flash));
	if (ret) {
		pr_err("fail to get user info\n");
		ret = -EFAULT;
		goto exit;
	}

	ret = flash_ops->set_flash(module->flash_core_handle,
		(void *)&set_param);
exit:
	return ret;
}

static int img_ioctl_cfg_flash(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_flash_cfg_param cfg_parm;

	ret = copy_from_user((void *) &cfg_parm,
				(void __user *)arg,
				sizeof(struct sprd_flash_cfg_param));
	if (ret) {
		pr_err("fail to get user info\n");
		ret = -EFAULT;
		goto exit;
	}
	ret = flash_ops->cfg_flash(module->flash_core_handle,
		(void *)&cfg_parm);

exit:
	return ret;
}

static int img_ioctl_get_flash(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_flash_capacity flash_info = {0};

	ret = flash_ops->get_flash(module->flash_core_handle,
		(void *)&flash_info);

	ret = copy_to_user((void __user *)arg, (void *)&flash_info,
					sizeof(struct sprd_flash_capacity));

	return ret;
}

static int img_ioctl_get_iommu_status(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	unsigned int iommu_enable;
	struct device	*dcam_dev;

	dcam_dev = &module->grp->pdev->dev;
	if (get_iommu_status(CAM_IOMMUDEV_DCAM) == 0)
		iommu_enable = 1;
	else
		iommu_enable = 0;
	module->iommu_enable = iommu_enable;

	ret = copy_to_user((void __user *)arg, &iommu_enable,
				sizeof(unsigned char));

	if (unlikely(ret)) {
		pr_err("fail to copy to user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}
	pr_info("iommu_enable:%d\n", iommu_enable);
exit:
	return ret;
}


static int img_ioctl_set_statis_buf(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct isp_statis_buf_input statis_buf;

	ret = copy_from_user((void *)&statis_buf,
			(void *)arg, sizeof(struct isp_statis_buf_input));
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	if ((statis_buf.type == STATIS_INIT) &&
		(atomic_read(&module->state) != CAM_IDLE) &&
		(atomic_read(&module->state) != CAM_CFG_CH)) {
		pr_err("error state to init statis buf: %d\n",
			atomic_read(&module->state));
		ret = -EFAULT;
		goto exit;
	}

	if ((statis_buf.type != STATIS_INIT) &&
		(atomic_read(&module->state) != CAM_RUNNING)) {
		pr_err("error state to configure statis buf: %d\n",
			atomic_read(&module->state));
		ret = -EFAULT;
		goto exit;
	}

	ret = dcam_ops->ioctl(module->dcam_dev_handle,
				DCAM_IOCTL_CFG_STATIS_BUF,
				&statis_buf);
exit:
	return ret;
}

static int img_ioctl_cfg_param(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct channel_context *channel;
	struct isp_io_param param;

	ret = copy_from_user((void *)&param,
			(void *)arg, sizeof(struct isp_io_param));
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}
	if (param.property_param == NULL) {
		pr_err("fail to get user param ptr.\n");
		ret = -EFAULT;
		goto exit;
	}
	if ((atomic_read(&module->state) != CAM_RUNNING) &&
		(atomic_read(&module->state) != CAM_CFG_CH)) {
		pr_info("cam%d state: %d\n", module->idx,
				atomic_read(&module->state));
		return -EFAULT;
	}

	if ((param.sub_block & DCAM_ISP_BLOCK_MASK) == DCAM_BLOCK_BASE) {
		if (unlikely((param.scene_id == PM_SCENE_CAP) &&
				module->cam_uinfo.is_4in1))
			/* 4in1 capture should cfg offline dcam */
			ret = dcam_ops->cfg_blk_param(
				module->aux_dcam_dev, &param);
		else
			ret = dcam_ops->cfg_blk_param(
				module->dcam_dev_handle, &param);
	} else {
		if (param.scene_id == PM_SCENE_PRE)
			channel = &module->channel[CAM_CH_PRE];
		else
			channel = &module->channel[CAM_CH_CAP];
		if (channel->enable && channel->isp_path_id >= 0) {
			ret = isp_ops->cfg_blk_param(module->isp_dev_handle,
				channel->isp_path_id >> ISP_CTXID_OFFSET,
				&param);
		}
	}

exit:
	return ret;
}

static int img_ioctl_cfg_start(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;

	ret = dcam_ops->ioctl(module->dcam_dev_handle,
			DCAM_IOCTL_CFG_START, NULL);
	return ret;
}

static int img_ioctl_set_function_mode(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	/*struct sprd_img_sensor_if *dst;*/
	struct sprd_img_function_mode __user *uparam;

	uparam = (struct sprd_img_function_mode __user *)arg;
	ret |= get_user(module->cam_uinfo.is_4in1, &uparam->need_4in1);
	ret |= get_user(module->cam_uinfo.is_3dnr, &uparam->need_3dnr);
	module->cam_uinfo.is_ltm = 0;

	pr_info("4in1:[%d], 3dnr[%d], ltm[%d]\n",
		module->cam_uinfo.is_4in1,
		module->cam_uinfo.is_3dnr,
		module->cam_uinfo.is_ltm);

	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}


/*---------------  Misc interface end --------------- */




/*---------------  capture(sensor input) config interface start -------------*/

static int img_ioctl_set_sensor_if(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_sensor_if *dst;

	dst = &module->cam_uinfo.sensor_if;

	ret = copy_from_user(dst,
				(void __user *)arg,
				sizeof(struct sprd_img_sensor_if));
	pr_info("sensor_if %d %x %x, %d.....mipi %d %d %d %d\n",
		dst->if_type, dst->img_fmt, dst->img_ptn, dst->frm_deci,
		dst->if_spec.mipi.use_href, dst->if_spec.mipi.bits_per_pxl,
		dst->if_spec.mipi.is_loose, dst->if_spec.mipi.lane_num);

	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}

static int img_ioctl_set_mode(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;

	ret = copy_from_user(&module->cam_uinfo.capture_mode,
				(void __user *)arg,
				sizeof(uint32_t));

	pr_info("mode %d\n", module->cam_uinfo.capture_mode);
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}

static int img_ioctl_set_sensor_size(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_size *dst;

	dst = &module->cam_uinfo.sn_size;

	ret = copy_from_user(dst,
				(void __user *)arg,
				sizeof(struct sprd_img_size));

	pr_info("sensor_size %d %d\n", dst->w, dst->h);
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}

static int img_ioctl_set_sensor_trim(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_rect *dst;

	dst = &module->cam_uinfo.sn_rect;

	ret = copy_from_user(dst,
				(void __user *)arg,
				sizeof(struct sprd_img_rect));
	pr_info("sensor_trim %d %d %d %d\n", dst->x, dst->y, dst->w, dst->h);
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}

static int img_ioctl_set_sensor_max_size(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_size *dst;

	dst = &module->cam_uinfo.sn_max_size;

	ret = copy_from_user(dst,
				(void __user *)arg,
				sizeof(struct sprd_img_size));
	pr_info("sensor_max_size %d %d\n", dst->w, dst->h);

	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}

static int img_ioctl_set_cap_skip_num(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t *dst;

	dst = &module->cam_uinfo.capture_skip;

	ret = copy_from_user(dst,
				(void __user *)arg,
				sizeof(uint32_t));
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		return -EFAULT;
	}

	pr_debug("set cap skip frame %d\n", *dst);
	return 0;
}


/*---------------  capture(sensor input) config interface end --------------- */



/*---------------  Channel config interface start --------------- */

static int img_ioctl_set_output_size(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t scene_mode;
	uint32_t cap_type;
	uint32_t sn_fmt, dst_fmt;
	struct channel_context *channel = NULL;
	struct camera_uchannel *dst;
	struct sprd_img_parm __user *uparam;

	module->last_channel_id = CAM_CH_MAX;
	uparam = (struct sprd_img_parm __user *)arg;

	ret |= get_user(scene_mode, &uparam->scene_mode);
	ret |= get_user(cap_type, &uparam->need_isp_tool);
	ret |= get_user(sn_fmt, &uparam->sn_fmt);
	ret |= get_user(dst_fmt, &uparam->pixel_fmt);

	pr_info("scene %d  cap_type %d, fmt %x %x\n",
		scene_mode, cap_type, sn_fmt, dst_fmt);
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		goto exit;
	}

	if (((cap_type == CAM_CAP_RAW_FULL) || (dst_fmt == sn_fmt)) &&
		(module->channel[CAM_CH_RAW].enable == 0)) {
		channel = &module->channel[CAM_CH_RAW];
		channel->enable = 1;
	} else if ((scene_mode == DCAM_SCENE_MODE_PREVIEW)  &&
			(module->channel[CAM_CH_PRE].enable == 0)) {
		channel = &module->channel[CAM_CH_PRE];
		channel->enable = 1;
	} else if (((scene_mode == DCAM_SCENE_MODE_RECORDING) ||
			(scene_mode == DCAM_SCENE_MODE_CAPTURE_CALLBACK)) &&
			(module->channel[CAM_CH_VID].enable == 0)) {
		channel = &module->channel[CAM_CH_VID];
		channel->enable = 1;
	} else if ((scene_mode == DCAM_SCENE_MODE_CAPTURE)  &&
			(module->channel[CAM_CH_CAP].enable == 0)) {
		channel = &module->channel[CAM_CH_CAP];
		channel->enable = 1;
	} else if (module->channel[CAM_CH_PRE].enable == 0) {
		channel = &module->channel[CAM_CH_PRE];
		channel->enable = 1;
	} else if (module->channel[CAM_CH_CAP].enable == 0) {
		channel = &module->channel[CAM_CH_CAP];
		channel->enable = 1;
	}

	if (channel == NULL) {
		pr_err("fail to get valid channel\n");
		goto exit;
	}

	pr_info("get ch %d\n", channel->ch_id);

	module->last_channel_id = channel->ch_id;
	channel->dcam_path_id = -1;
	channel->isp_path_id = -1;
	channel->slave_isp_path_id = -1;

	dst = &channel->ch_uinfo;
	dst->sn_fmt = sn_fmt;
	dst->dst_fmt = dst_fmt;
	ret |= get_user(dst->is_high_fps, &uparam->is_high_fps);
	ret |= get_user(dst->high_fps_skip_num, &uparam->high_fps_skip_num);
	ret |= copy_from_user(&dst->src_crop,
			&uparam->crop_rect, sizeof(struct sprd_img_rect));
	ret |= copy_from_user(&dst->dst_size,
			&uparam->dst_size, sizeof(struct sprd_img_size));
	ret |= get_user(dst->slave_img_en, &uparam->aux_img.enable);
	ret |= get_user(dst->slave_img_fmt, &uparam->aux_img.pixel_fmt);
	ret |= copy_from_user(&dst->slave_img_size,
			&uparam->aux_img.dst_size, sizeof(struct sprd_img_size));

	pr_info("high fps %u %u. crop %d %d %d %d. dst size %d %d. aux %d %d %d %d\n",
		dst->is_high_fps, dst->high_fps_skip_num,
		dst->src_crop.x, dst->src_crop.y,
		dst->src_crop.w, dst->src_crop.h,
		dst->dst_size.w, dst->dst_size.h,
		dst->slave_img_en, dst->slave_img_fmt,
		dst->slave_img_size.w, dst->slave_img_size.h);

exit:
	return ret;
}

static int img_ioctl_get_ch_id(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;

	if ((atomic_read(&module->state) != CAM_IDLE) &&
		(atomic_read(&module->state) != CAM_CFG_CH)) {
		pr_err("error: only for state IDLE or CFG_CH\n");
		return -EFAULT;
	}

	if (module->last_channel_id >= CAM_CH_MAX) {
		ret = -EINVAL;
		goto exit;
	}
	pr_info("get ch id: %d\n", module->last_channel_id);

	ret = copy_to_user((void __user *)arg, &module->last_channel_id,
				sizeof(uint32_t));
	if (unlikely(ret))
		pr_err("fail to copy to user. ret %d\n", ret);

exit:
	/* todo: error handling. */
	atomic_set(&module->state, CAM_CFG_CH);
	return ret;
}

static int img_ioctl_dcam_path_size(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_dcam_path_size param;

	ret = copy_from_user(
			&param, (void __user *)arg,
			sizeof(struct sprd_dcam_path_size));
	param.dcam_out_w = param.dcam_in_w;
	param.dcam_out_h = param.dcam_in_h;

	if (atomic_read(&module->timeout_flag) == 1)
		pr_info("in %d  %d. pre %d %d, vid %d, %d\n",
			param.dcam_in_w, param.dcam_in_h,
			param.pre_dst_w, param.pre_dst_h,
			param.vid_dst_w, param.vid_dst_h);

	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

	ret = copy_to_user((void __user *)arg, &param,
			sizeof(struct sprd_dcam_path_size));

exit:
	return ret;
}



static int img_ioctl_set_shrink(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t channel_id;
	struct sprd_img_parm __user *uparam;

	uparam = (struct sprd_img_parm __user *)arg;

	ret = get_user(channel_id, &uparam->channel_id);
	if (ret  == 0 && channel_id < CAM_CH_MAX) {
		ret = copy_from_user(
			&module->channel[channel_id].ch_uinfo.regular_desc,
			(void __user *)&uparam->regular_desc,
			sizeof(struct dcam_regular_desc));
	}

	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}



static int img_ioctl_pdaf_control(
			struct camera_module *module,
			unsigned long arg)
{
	int ret;
	uint32_t channel_id;
	struct sprd_pdaf_control tmp;
	struct sprd_img_parm __user *uparam;

	uparam = (struct sprd_img_parm __user *)arg;
	ret = get_user(channel_id, &uparam->channel_id);
	if (ret || (channel_id == CAM_CH_RAW))
		return 0;

	ret = copy_from_user(&tmp, &uparam->pdaf_ctrl,
			sizeof(struct sprd_pdaf_control));
	if (unlikely(ret)) {
		pr_err("fail to copy pdaf param from user, ret %d\n", ret);
		return -EFAULT;
	}

	pr_info("mode %d, type %d, vc %d, dt 0x%x, isp %d\n",
		tmp.mode, tmp.phase_data_type, tmp.image_vc,
		tmp.image_dt, tmp.isp_tool_mode);

	/* config pdaf */
	ret = dcam_ops->ioctl(module->dcam_dev_handle,
				DCAM_IOCTL_CFG_PDAF, &tmp);
	return 0;
}

static int img_ioctl_set_zoom_mode(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;

	ret = copy_from_user(&module->is_smooth_zoom,
			     (void __user *)arg,
			     sizeof(uint32_t));

	pr_info("is_smooth_zoom %d\n", module->is_smooth_zoom);
	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}

exit:
	return ret;
}

static int img_ioctl_set_crop(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0, zoom = 0;
	uint32_t channel_id;
	struct channel_context *ch;
	struct sprd_img_rect *crop;
	struct camera_frame *first = NULL;
	struct camera_frame *zoom_param = NULL;
	struct sprd_img_parm __user *uparam;

	if ((atomic_read(&module->state) != CAM_CFG_CH) &&
		(atomic_read(&module->state) != CAM_RUNNING)) {
		pr_err("error module state: %d\n", atomic_read(&module->state));
		return -EFAULT;
	}

	uparam = (struct sprd_img_parm __user *)arg;

	ret = get_user(channel_id, &uparam->channel_id);
	ch = &module->channel[channel_id];
	if (ret || (channel_id >= CAM_CH_MAX) || !ch->enable) {
		pr_err("error set crop, ret %d, ch %d\n", ret, channel_id);
		ret = -EINVAL;
		goto exit;
	}

	/* CAM_RUNNING: for zoom update
	 * only crop rect size can be re-configured during zoom
	 * and it is forbidden during capture.
	 */
	if (atomic_read(&module->state) == CAM_RUNNING) {
		if (module->cap_status == CAM_CAPTURE_START) {
			pr_err("zoom is not allowed during capture\n");
			goto exit;
		}
		crop = kzalloc(sizeof(struct sprd_img_rect), GFP_KERNEL);
		if (crop == NULL) {
			ret = -ENOMEM;
			goto exit;
		}
		zoom_param = get_empty_frame();
		zoom_param->priv_data = (void *)crop;
		zoom = 1;
	} else {
		crop = &ch->ch_uinfo.src_crop;
	}

	ret = copy_from_user(crop,
			(void __user *)&uparam->crop_rect,
			sizeof(struct sprd_img_rect));
	pr_info("set ch%d crop %d %d %d %d.\n", channel_id,
			crop->x, crop->y, crop->w, crop->h);

	if (unlikely(ret)) {
		pr_err("fail to copy from user, ret %d\n", ret);
		goto exit;
	}

	if (zoom) {
		if (camera_enqueue(&ch->zoom_coeff_queue, zoom_param)) {
			/* if zoom queue overflow, discard first one node in queue*/
			pr_warn("ch %d zoom q overflow\n", channel_id);
			first = camera_dequeue(&ch->zoom_coeff_queue);
			if (first) {
				kfree(first->priv_data);
				put_empty_frame(first);
			}
			camera_enqueue(&ch->zoom_coeff_queue, zoom_param);
		}
		zoom_param = NULL;
		complete(&module->zoom_thrd.thread_com);
	}
exit:
	if (zoom_param) {
		kfree(zoom_param->priv_data);
		put_empty_frame(zoom_param);
	}
	return ret;
}

static int img_ioctl_get_fmt(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_get_fmt fmt_desc;
	struct camera_format *fmt = NULL;

	ret = copy_from_user(&fmt_desc, (void __user *)arg,
				sizeof(struct sprd_img_get_fmt));
	if (unlikely(ret)) {
		pr_err("fail to copy from user ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}
	if (unlikely(fmt_desc.index >= ARRAY_SIZE(output_img_fmt))) {
		pr_err("fail to get valid index > arrar size\n");
		ret = -EINVAL;
		goto exit;
	}

	fmt = &output_img_fmt[fmt_desc.index];
	fmt_desc.fmt = fmt->fourcc;

	ret = copy_to_user((void __user *)arg,
				&fmt_desc,
				sizeof(struct sprd_img_get_fmt));
	if (unlikely(ret)) {
		pr_err("fail to put user info, GET_FMT, ret %d\n", ret);
		ret = -EFAULT;
		goto exit;
	}
exit:
	return ret;
}

static int img_ioctl_check_fmt(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t channel_id;
	struct sprd_img_format img_format;
	struct channel_context *channel;

	pr_debug("check fmt\n");
	ret = copy_from_user(&img_format,
				(void __user *)arg,
				sizeof(struct sprd_img_format));
	if (ret) {
		pr_err("fail to get img_format\n");
		return -EFAULT;
	}

	if ((atomic_read(&module->state) != CAM_CFG_CH) &&
		(atomic_read(&module->state) != CAM_RUNNING)) {
		pr_err("error module state: %d\n", atomic_read(&module->state));
		return -EFAULT;
	}

	channel_id = img_format.channel_id;
	channel = &module->channel[channel_id];

	if (atomic_read(&module->state) == CAM_CFG_CH) {
		/* to do: check & set channel format / param before stream on */
		pr_info("chk_fmt ch %d\n", channel_id);

		ret = init_cam_channel(module, channel);
		if (ret) {
			/* todo: error handling. */
			pr_err("fail to init channel %d\n", channel->ch_id);
			goto exit;
		}
	}

	img_format.need_binning = 0;
	ret = copy_to_user((void __user *)arg,
			&img_format,
			sizeof(struct sprd_img_format));
	if (ret) {
		ret = -EFAULT;
		pr_err("fail to copy to user\n");
		goto exit;
	}
exit:
	return ret;
}

static int img_ioctl_set_frame_addr(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t i, cmd;
	struct sprd_img_parm param;
	struct channel_context *ch;
	struct camera_frame *pframe;

	ret = copy_from_user(&param, (void __user *)arg,
				sizeof(struct sprd_img_parm));
	if (ret) {
		pr_err("fail to copy from user. ret %d\n", ret);
		return -EFAULT;
	}

	if ((atomic_read(&module->state) != CAM_CFG_CH) &&
		(atomic_read(&module->state) != CAM_RUNNING)) {
		pr_err("error: only for state CFG_CH or RUNNING\n");
		return -EFAULT;
	}

	if ((param.channel_id >= CAM_CH_MAX) ||
		(param.buffer_count == 0) ||
		(module->channel[param.channel_id].enable == 0)) {
		pr_err("error: invalid channel id %d. buf cnt %d\n",
			param.channel_id,  param.buffer_count);
		return -EFAULT;
	}
	pr_debug("ch %d, buffer_count %d\n", param.channel_id,
		param.buffer_count);

	ch = &module->channel[param.channel_id];
	for (i = 0; i < param.buffer_count; i++) {
		pframe = get_empty_frame();
		if (pframe == NULL) {
			pr_err("fail to get empty frame node\n");
			ret = -EFAULT;
			break;
		}
		pframe->buf.type = CAM_BUF_USER;
		pframe->buf.mfd[0] = param.fd_array[i];
		pframe->channel_id = ch->ch_id;
		pr_debug("frame %p,  mfd %d, reserved %d\n",
				pframe, pframe->buf.mfd[0],
					param.is_reserved_buf);

		ret = cambuf_get_ionbuf(&pframe->buf);
		if (ret) {
			put_empty_frame(pframe);
			ret = -EFAULT;
			break;
		}

		if (ch->isp_path_id >= 0) {
			cmd = ISP_PATH_CFG_OUTPUT_BUF;
			if (param.is_reserved_buf) {
				ch->reserved_buf_fd = pframe->buf.mfd[0];
				cmd = ISP_PATH_CFG_OUTPUT_RESERVED_BUF;
			}
			ret = isp_ops->cfg_path(module->isp_dev_handle, cmd,
					ch->isp_path_id >> ISP_CTXID_OFFSET,
					ch->isp_path_id & ISP_PATHID_MASK,
					pframe);
		} else {
			cmd = DCAM_PATH_CFG_OUTPUT_BUF;
			if (param.is_reserved_buf) {
				ch->reserved_buf_fd = pframe->buf.mfd[0];
				cmd = DCAM_PATH_CFG_OUTPUT_RESERVED_BUF;
			}
			ret = dcam_ops->cfg_path(module->dcam_dev_handle,
					cmd, ch->dcam_path_id, pframe);
		}

		if (ret) {
			pr_err("fail to set output buffer for ch%d.\n",
				ch->ch_id);
			cambuf_put_ionbuf(&pframe->buf);
			put_empty_frame(pframe);
			ret = -EFAULT;
			break;
		}
	}

	return ret;
}

/*
 * SPRD_IMG_IO_PATH_FRM_DECI
 *
 * Set frame deci factor for each channel, which controls the number of dropped
 * frames in ISP output path. It's typically used in slow motion scene. There're
 * two situations in slow motion: preview and recording. HAL will set related
 * parameters according to the table below:
 *
 * ===================================================================================
 * |     scene     |  channel  |  is_high_fps  |  high_fps_skip_num  |  deci_factor  |
 * |---------------|-----------|---------------|---------------------|---------------|
 * |    normal     |  preview  |       0       |          0          |       0       |
 * |    preview    |           |               |                     |               |
 * |---------------|-----------|---------------|---------------------|---------------|
 * |  slow motion  |  preview  |       1       |          4          |       3       |
 * |    preview    |           |               |                     |               |
 * |---------------|-----------|---------------|---------------------|---------------|
 * |               |  preview  |       1       |          4          |       3       |
 * |  slow motion  |           |               |                     |               |
 * |   recording   |-----------|---------------|---------------------|---------------|
 * |               |   video   |       1       |          4          |       0       |
 * |               |           |               |                     |               |
 * ===================================================================================
 *
 * Here, is_high_fps means sensor is running at a high frame rate, thus DCAM
 * slow motion function should be enabled. And deci_factor controls how many
 * frames will be dropped by ISP path before DONE interrupt generated. The
 * high_fps_skip_num is responsible for keeping SOF interrupt running at 30
 * frame rate.
 */
static int img_ioctl_set_frm_deci(struct camera_module *module,
				  unsigned long arg)
{
	struct sprd_img_parm __user *uparam = NULL;
	struct channel_context *ch = NULL;
	uint32_t deci_factor = 0, channel_id = 0;
	int ret = 0;

	if (unlikely(!module))
		return -EINVAL;

	if ((atomic_read(&module->state) != CAM_CFG_CH)) {
		pr_err("error: only for state CFG_CH\n");
		return -EPERM;
	}

	uparam = (struct sprd_img_parm __user *)arg;
	ret |= get_user(channel_id, &uparam->channel_id);
	ret |= get_user(deci_factor, &uparam->deci);
	if (ret) {
		pr_err("fail to get from user. ret %d\n", ret);
		return -EFAULT;
	}

	if ((channel_id >= CAM_CH_MAX) ||
	    (module->channel[channel_id].enable == 0)) {
		pr_err("error: invalid channel id %d\n", channel_id);
		return -EPERM;
	}

	ch = &module->channel[channel_id];
	ch->ch_uinfo.deci_factor = deci_factor;

	return ret;
}

static int img_ioctl_set_frame_id_base(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t channel_id, frame_id_base;
	struct channel_context *ch;
	struct sprd_img_parm __user *uparam;

	if ((atomic_read(&module->state) != CAM_CFG_CH) &&
		(atomic_read(&module->state) != CAM_RUNNING)) {
		pr_err("error: only for state CFG_CH or RUNNING\n");
		return -EFAULT;
	}

	uparam = (struct sprd_img_parm __user *)arg;
	ret =  get_user(channel_id, &uparam->channel_id);
	ret |= get_user(frame_id_base, &uparam->frame_base_id);
	if (ret) {
		pr_err("fail to get from user. ret %d\n", ret);
		return -EFAULT;
	}

	if ((channel_id >= CAM_CH_MAX) ||
		(module->channel[channel_id].enable == 0)) {
		pr_err("error: invalid channel id %d\n", channel_id);
		return -EFAULT;
	}

	ch = &module->channel[channel_id];
	ch->frm_base_id = frame_id_base;

	return ret;
}
/*---------------  Channel config interface end --------------- */




/*--------------- Core controlling interface start --------------- */

static int img_ioctl_get_cam_res(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	int dcam_idx;
	struct sprd_img_res res = {0};
	struct camera_group *grp = module->grp;
	void *dcam = NULL;
	void *isp = NULL;

	ret = copy_from_user(&res, (void __user *)arg,
				sizeof(struct sprd_img_res));
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	if (atomic_read(&module->state) != CAM_INIT) {
		pr_err("error cam%d state: %d\n",
			module->idx, atomic_read(&module->state));
		return -EFAULT;
	}
	pr_info("cam%d get res\n", module->idx);

	dcam_idx = -1;
#ifdef TEST_ON_HAPS
	if (res.sensor_id == 0)
		dcam_idx = 0;
	else
		dcam_idx = 1;
#else
	if (res.sensor_id < SPRD_SENSOR_ID_MAX) {
		/* get a preferred dcam dev */
		dcam_idx = sprd_sensor_find_dcam_id(res.sensor_id);
	}
#endif
	if (dcam_idx == -1) {
		pr_err("fail to get dcam id for sensor: %d\n", res.sensor_id);
		return -EFAULT;
	}

	dcam = module->dcam_dev_handle;
	if (dcam == NULL) {
		dcam = dcam_if_get_dev(dcam_idx, grp->dcam[dcam_idx]);
		if (IS_ERR_OR_NULL(dcam)) {
			pr_err("fail to get dcam%d\n", dcam_idx);
			ret = -EINVAL;
			goto no_dcam;
		}
		module->dcam_dev_handle = dcam;
		module->dcam_idx = dcam_idx;
	}

	ret = dcam_ops->open(dcam);
	if (ret) {
		dcam_if_put_dev(dcam);
		ret = -EINVAL;
		goto dcam_fail;
	}

	ret = dcam_ops->set_callback(dcam, dcam_callback, module);
	if (ret) {
		pr_err("fail to set cam%d callback for dcam.\n", dcam_idx);
		goto dcam_cb_fail;
	}

	isp = module->isp_dev_handle;
	if (isp == NULL) {
		isp = get_isp_pipe_dev();
		if (IS_ERR_OR_NULL(isp)) {
			pr_err("fail to get isp\n");
			module->isp_dev_handle = NULL;
			ret = -EINVAL;
			goto no_isp;
		}
		module->isp_dev_handle = isp;
	}

	ret = isp_ops->open(isp, grp->isp[0]);
	if (ret) {
		pr_err("faile to enable isp module.\n");
		ret = -EINVAL;
		goto isp_fail;
	}


	module->attach_sensor_id = res.sensor_id;
	module->workqueue = create_workqueue("sprd_camera_module");
	if (!module->workqueue) {
		pr_err("fail to create camera dev wq\n");
		ret = -EINVAL;
		goto wq_fail;
	}
	atomic_set(&module->work.status, CAM_WORK_DONE);


	if (dcam_idx == 0)
		res.flag = DCAM_RES_DCAM0_CAP | DCAM_RES_DCAM0_PATH;
	else if (dcam_idx == 1)
		res.flag = DCAM_RES_DCAM1_CAP | DCAM_RES_DCAM1_PATH;
	else
		res.flag = DCAM_RES_DCAM2_CAP | DCAM_RES_DCAM2_PATH;


	pr_debug("sensor %d w %u h %u, cam [%d]\n",
		res.sensor_id, res.width, res.height, module->idx);

	pr_info("get camera res for sensor %d res %x done.\n",
					res.sensor_id, res.flag);

	ret = copy_to_user((void __user *)arg, &res,
				sizeof(struct sprd_img_res));
	if (ret) {
		pr_err("fail to copy_to_user\n");
		ret = -EFAULT;
		goto copy_fail;
	}
	atomic_set(&module->state, CAM_IDLE);
	pr_info("cam%d get res done\n", module->idx);
	return 0;

copy_fail:
	destroy_workqueue(module->workqueue);
	module->workqueue  = NULL;

wq_fail:
	isp_ops->close(isp);

isp_fail:
	put_isp_pipe_dev(isp);
	module->isp_dev_handle = NULL;

no_isp:
dcam_cb_fail:
	dcam_ops->close(dcam);

dcam_fail:
	dcam_if_put_dev(dcam);
	module->dcam_dev_handle = NULL;
no_dcam:
	pr_err("fail to get camera res for sensor: %d\n", res.sensor_id);

	return ret;
}


static int img_ioctl_put_cam_res(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t idx;
	struct sprd_img_res res = {0};

	ret = copy_from_user(&res, (void __user *)arg,
				sizeof(struct sprd_img_res));
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	pr_info("cam%d put res state: %d\n", module->idx,
		atomic_read(&module->state));

	ret = img_ioctl_stream_off(module, arg);

	if (atomic_read(&module->state) != CAM_IDLE) {
		pr_info("cam%d error state: %d\n", module->idx,
			atomic_read(&module->state));
		return -EFAULT;
	}

	idx = module->idx;

	if (module->attach_sensor_id != res.sensor_id) {
		pr_warn("warn: mismatch sensor id: %d, %d for cam %d\n",
				module->attach_sensor_id, res.sensor_id,
				module->idx);
	}

	destroy_workqueue(module->workqueue);
	module->workqueue  = NULL;
	module->attach_sensor_id = -1;

	if (module->dcam_dev_handle) {
		dcam_ops->close(module->dcam_dev_handle);
		dcam_if_put_dev(module->dcam_dev_handle);
		module->dcam_dev_handle = NULL;
	}
	if (module->isp_dev_handle) {
		isp_ops->close(module->isp_dev_handle);
		put_isp_pipe_dev(module->isp_dev_handle);
		module->isp_dev_handle = NULL;
	}

	atomic_set(&module->state, CAM_INIT);

	pr_debug("sensor %d w %u h %u, cam [%d]\n",
		res.sensor_id, res.width, res.height, module->idx);

	pr_info("put camera res for sensor %d res %x done.",
					res.sensor_id, res.flag);
	ret = copy_to_user((void __user *)arg, &res,
			sizeof(struct sprd_img_res));
	if (ret)
		pr_err("fail to copy_to_user!\n");

	return ret;
}

static int img_ioctl_stream_on(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t i, j, line_w, isp_ctx_id, isp_path_id;
	struct channel_context *ch;

	if (atomic_read(&module->state) != CAM_CFG_CH) {
		pr_info("cam%d error state: %d\n", module->idx,
				atomic_read(&module->state));
		return -EFAULT;
	}

	atomic_set(&module->state, CAM_STREAM_ON);
	pr_info("cam%d stream on starts\n", module->idx);

	ret = cal_channel_size(module);
	ch = &module->channel[CAM_CH_PRE];
	if (ch->enable)
		config_channel_size(module, ch);
	ch = &module->channel[CAM_CH_CAP];
	if (ch->enable)
		config_channel_size(module, ch);

	/* line buffer share mode setting
	 * Precondition: dcam0, dcam1 size not conflict
	 */
	line_w = module->cam_uinfo.sn_rect.w;
	if (module->cam_uinfo.is_4in1)
		line_w /= 2;
	dcam_lbuf_share_mode(module->dcam_idx, line_w);

	ret = dcam_ops->ioctl(module->dcam_dev_handle,
				DCAM_IOCTL_INIT_STATIS_Q, NULL);

	for (i = 0;  i < CAM_CH_MAX; i++) {
		ch = &module->channel[i];
		if (!ch->enable)
			continue;

		camera_queue_init(&ch->zoom_coeff_queue,
			(module->is_smooth_zoom ? CAM_ZOOM_COEFF_Q_LEN : 1),
			0, camera_put_empty_frame);

		if (ch->alloc_start) {
			if (atomic_read(&ch->err_status) != 0) {
				pr_err("ch %d error happens.", ch->ch_id);
				ret = -EFAULT;
				goto exit;
			}

			ret = wait_for_completion_interruptible(&ch->alloc_com);
			if (ret != 0) {
				pr_err("config channel/path param work %d\n",
					ret);
				goto exit;
			}
			ch->alloc_start = 0;

			/* set shared frame for dcam output */
			while (1) {
				struct camera_frame *pframe = NULL;

				pframe = camera_dequeue(&ch->share_buf_queue);
				if (pframe == NULL)
					break;
				if (module->cam_uinfo.is_4in1 &&
					i == CAM_CH_CAP)
					ret = dcam_ops->cfg_path(
						module->aux_dcam_dev,
						DCAM_PATH_CFG_OUTPUT_BUF,
						ch->aux_dcam_path_id,
						pframe);
				else
					ret = dcam_ops->cfg_path(
						module->dcam_dev_handle,
						DCAM_PATH_CFG_OUTPUT_BUF,
						ch->dcam_path_id,
						pframe);
				if (ret) {
					pr_err("fail config dcam output buffer\n");
					camera_enqueue(&ch->share_buf_queue,
						pframe);
					ret = -EINVAL;
					goto exit;
				}
			}
			isp_ctx_id = ch->isp_path_id >> ISP_CTXID_OFFSET;
			isp_path_id = ch->isp_path_id & ISP_PATHID_MASK;

			for (j = 0; j < ISP_NR3_BUF_NUM; j++) {
				if (ch->nr3_bufs[j] == NULL)
					continue;
				ret = isp_ops->cfg_path(module->isp_dev_handle,
						ISP_PATH_CFG_3DNR_BUF,
						isp_ctx_id, isp_path_id,
						ch->nr3_bufs[j]);
				if (ret) {
					pr_err("fail config isp 3DNR buffer\n");
					goto exit;
				}
			}

			for (j = 0; j < ISP_LTM_BUF_NUM; j++) {
				if (ch->ltm_bufs[j] == NULL) {
					pr_info("ch->ltm_bufs[%d] NULL\n", j);
					continue;
				}
				/*
				pr_info("ch->ltm_bufs[%d] ISP_PATH_CFG_LTM_BUF ctx[%d] path[%d]\n",
					j, isp_ctx_id, isp_path_id);
				*/
				ret = isp_ops->cfg_path(module->isp_dev_handle,
						ISP_PATH_CFG_LTM_BUF,
						isp_ctx_id, isp_path_id,
						ch->ltm_bufs[j]);
				if (ret) {
					pr_err("fail config isp LTM buffer\n");
					goto exit;
				}
			}
		}
	}

	pr_info("wait for wq done.\n");
	flush_workqueue(module->workqueue);

	camera_queue_init(&module->frm_queue,
		CAM_FRAME_Q_LEN, 0, camera_put_empty_frame);
	camera_queue_init(&module->irq_queue,
		CAM_IRQ_Q_LEN, 0, camera_put_empty_frame);
	camera_queue_init(&module->statis_queue,
		CAM_STATIS_Q_LEN, 0, camera_put_empty_frame);

	set_cap_info(module);

	atomic_set(&module->state, CAM_RUNNING);
	ret = dcam_ops->start(module->dcam_dev_handle);

	atomic_set(&module->timeout_flag, 1);
	ret = sprd_start_timer(&module->cam_timer, CAMERA_TIMEOUT);

	if (module->dump_thrd.thread_task) {
		camera_queue_init(&module->dump_queue, 10, 0, put_k_frame);
		init_completion(&module->dump_com);
		mutex_lock(&g_dbg_dump.dump_lock);
		i = module->dcam_idx;
		if (i < 2) {
			g_dbg_dump.dump_start[i] = &module->dump_thrd.thread_com;
			g_dbg_dump.dump_count = 0;
		}
		mutex_unlock(&g_dbg_dump.dump_lock);
	}

	pr_info("stream on done.\n");
	return 0;

exit:
	atomic_set(&module->state, CAM_CFG_CH);
	pr_info("stream on failed\n");

	/* call stream_off to clear buffers/path */
	img_ioctl_stream_off(module, 0L);

	return ret;
}


static int img_ioctl_stream_off(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	uint32_t i, j;
	uint32_t running = 0;
	struct channel_context *ch;
	int isp_ctx_id[CAM_CH_MAX] = { -1 };

	if ((atomic_read(&module->state) != CAM_RUNNING) &&
		(atomic_read(&module->state) != CAM_CFG_CH)) {
		pr_info("cam%d state: %d\n", module->idx,
				atomic_read(&module->state));
		return -EFAULT;
	}

	if (atomic_read(&module->state) == CAM_RUNNING)
		running = 1;

	pr_info("cam %d stream off. state: %d\n",
		module->idx, atomic_read(&module->state));
	atomic_set(&module->state, CAM_STREAM_OFF);

	/* stop raw dump */
	if (module->dump_thrd.thread_task) {
		if (module->in_dump)
			complete(&module->dump_com);
		mutex_lock(&g_dbg_dump.dump_lock);
		i = module->dcam_idx;
		if (i < 2) {
			g_dbg_dump.dump_start[i] = NULL;
			g_dbg_dump.dump_count = 0;
		}
		mutex_unlock(&g_dbg_dump.dump_lock);
	}

	if (running) {
		ret = dcam_ops->stop(module->dcam_dev_handle);
		sprd_stop_timer(&module->cam_timer);
	}
	if (module->cam_uinfo.is_4in1 && module->aux_dcam_dev)
		deinit_4in1_aux(module);

	if (module->dcam_dev_handle)
		ret = dcam_ops->ioctl(module->dcam_dev_handle,
				DCAM_IOCTL_DEINIT_STATIS_Q, NULL);

	for (i = 0;  i < CAM_CH_MAX; i++) {
		ch = &module->channel[i];
		isp_ctx_id[i] = -1;
		if (!ch->enable)
			continue;
		pr_info("clear ch %d, dcam path %d, isp path 0x%x\n",
				ch->ch_id,
				ch->dcam_path_id,
				ch->isp_path_id);
		if (ch->dcam_path_id >= 0) {
			dcam_ops->put_path(module->dcam_dev_handle,
					ch->dcam_path_id);
		}
		if (ch->isp_path_id >= 0) {
			isp_ctx_id[i] = ch->isp_path_id >> ISP_CTXID_OFFSET;
			isp_ops->put_path(module->isp_dev_handle,
					isp_ctx_id[i],
					ch->isp_path_id & ISP_PATHID_MASK);
		}
		if (ch->slave_isp_path_id >= 0) {
			isp_ops->put_path(module->isp_dev_handle,
					isp_ctx_id[i],
					ch->slave_isp_path_id & ISP_PATHID_MASK);
		}
	}

	for (i = 0;  i < CAM_CH_MAX; i++) {
		ch = &module->channel[i];
		if (!ch->enable)
			continue;
		camera_queue_clear(&ch->zoom_coeff_queue);

		if ((ch->ch_id == CAM_CH_PRE) || (ch->ch_id == CAM_CH_CAP)) {
			if (isp_ctx_id[i] != -1)
				isp_ops->put_context(module->isp_dev_handle,
					isp_ctx_id[i]);

			if (ch->alloc_start) {
				ret = wait_for_completion_interruptible(
					&ch->alloc_com);
				if (ret != 0)
					pr_err("error: config channel/path param work %d\n",
						ret);
				pr_info("alloc buffer done.\n");
				ch->alloc_start = 0;
			}
			if (ch->isp_updata) {
				struct isp_offline_param *cur, *prev;

				cur = (struct isp_offline_param *)ch->isp_updata;
				ch->isp_updata = NULL;
				while (cur) {
					prev = (struct isp_offline_param *)cur->prev;
					kfree(cur);
					pr_info("free %p\n", cur);
					cur = prev;
				}
			}
			camera_queue_clear(&ch->share_buf_queue);

			for (j = 0; j < ISP_NR3_BUF_NUM; j++) {
				if (ch->nr3_bufs[j]) {
					put_k_frame(ch->nr3_bufs[j]);
					ch->nr3_bufs[j] = NULL;
				}
			}
			for (j = 0; j < ISP_LTM_BUF_NUM; j++) {
				if (ch->ltm_bufs[j]) {
					if (ch->ch_id == CAM_CH_PRE)
						put_k_frame(ch->ltm_bufs[j]);
					ch->ltm_bufs[j] = NULL;
				}
			}
		}
	}

	for (i = 0; i < CAM_CH_MAX; i++) {
		ch = &module->channel[i];
		memset(ch, 0, sizeof(struct channel_context));
		ch->ch_id = i;
		ch->dcam_path_id = -1;
		ch->isp_path_id = -1;
		init_completion(&ch->alloc_com);
	}

	if (running) {
		camera_queue_clear(&module->frm_queue);
		camera_queue_clear(&module->irq_queue);
		camera_queue_clear(&module->statis_queue);
		if (module->dump_thrd.thread_task)
			camera_queue_clear(&module->dump_queue);
	}

	atomic_set(&module->state, CAM_IDLE);

	ret = mdbg_check();
	pr_info("cam %d stream off done.\n", module->idx);

	return ret;
}


static int img_ioctl_start_capture(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_capture_param param;

	ret = copy_from_user(&param, (void __user *)arg,
			sizeof(struct sprd_img_capture_param));
	if (ret) {
		pr_err("fail to copy user info\n");
		return -EFAULT;
	}

	if (atomic_read(&module->state) != CAM_RUNNING) {
		pr_info("cam%d error state: %d\n", module->idx,
				atomic_read(&module->state));
		return -EFAULT;
	}

	if (param.type != DCAM_CAPTURE_STOP)
		module->cap_status = CAM_CAPTURE_START;

	/* alway trigger dump for capture */
	if (module->dump_thrd.thread_task) {
		uint32_t idx = module->dcam_idx;
		struct cam_dbg_dump *dbg = &g_dbg_dump;

		if (idx < 2 && module->dcam_dev_handle) {
			mutex_lock(&dbg->dump_lock);
			if (!(dbg->dump_ongoing & (1 << idx))){
				complete(&module->dump_thrd.thread_com);
				dbg->dump_count = 99;
				pr_debug("trigger sdump capture raw\n");
			}
			mutex_unlock(&dbg->dump_lock);
		}
	}

	pr_info("cam %d start capture type %d, cnt %d, time %lld\n",
		module->idx, param.type, param.cnr_cnt, param.timestamp);
	return ret;
}


static int img_ioctl_stop_capture(
			struct camera_module *module,
			unsigned long arg)
{
	module->cap_status = CAM_CAPTURE_STOP;
	pr_info("cam %d stop capture.\n", module->idx);

	/* stop dump for capture */
	if (module->dump_thrd.thread_task && module->in_dump) {
		module->dump_count = 0;
		complete(&module->dump_com);
	}

	return 0;
}

static int raw_proc_done(struct camera_module *module)
{
	int ret = 0;
	int isp_ctx_id, isp_path_id;
	struct channel_context *ch;

	module->cap_status = CAM_CAPTURE_STOP;
	atomic_set(&module->state, CAM_STREAM_OFF);

	if (atomic_read(&module->timeout_flag) == 1)
		pr_err("raw proc timeout.\n");

	ret = dcam_ops->stop(module->dcam_dev_handle);
	sprd_stop_timer(&module->cam_timer);

	ret = dcam_ops->ioctl(module->dcam_dev_handle,
			DCAM_IOCTL_DEINIT_STATIS_Q, NULL);

	ch = &module->channel[CAM_CH_CAP];
	dcam_ops->put_path(module->dcam_dev_handle,
					ch->dcam_path_id);

	isp_ctx_id = ch->isp_path_id >> ISP_CTXID_OFFSET;
	isp_path_id = ch->isp_path_id & ISP_PATHID_MASK;
	isp_ops->put_path(module->isp_dev_handle,
					isp_ctx_id, isp_path_id);
	isp_ops->put_context(module->isp_dev_handle, isp_ctx_id);

	ch->enable = 0;
	ch->dcam_path_id = -1;
	ch->isp_path_id = -1;
	ch->aux_dcam_path_id = -1;
	camera_queue_clear(&module->frm_queue);
	camera_queue_clear(&ch->share_buf_queue);
	atomic_set(&module->state, CAM_IDLE);
	pr_info("camera%d rawproc done.\n", module->idx);

	return ret;
}

/* build channel/path in pre-processing */
static int raw_proc_pre(
		struct camera_module *module,
		struct isp_raw_proc_info *proc_info)
{
	int ret = 0;
	int ctx_id, dcam_path_id, isp_path_id;
	struct channel_context *ch;
	struct img_size max_size;
	struct img_trim path_trim;
	struct dcam_path_cfg_param ch_desc;
	struct isp_ctx_base_desc ctx_desc;
	struct isp_ctx_size_desc ctx_size;
	struct isp_path_base_desc isp_path_desc;

	pr_info("start\n");
	ch = &module->channel[CAM_CH_CAP];
	ch->dcam_path_id = -1;
	ch->isp_path_id = -1;
	ch->aux_dcam_path_id = -1;

	/* specify dcam path */
	dcam_path_id = DCAM_PATH_BIN;
	ret = dcam_ops->get_path(
		module->dcam_dev_handle, dcam_path_id);
	if (ret < 0) {
		pr_err("fail to get dcam path %d\n", dcam_path_id);
		return -EFAULT;
	}
	ch->dcam_path_id = dcam_path_id;

	/* config dcam path  */
	memset(&ch_desc, 0, sizeof(ch_desc));
	ch_desc.is_loose = module->cam_uinfo.sensor_if.if_spec.mipi.is_loose;
	ch_desc.endian.y_endian = proc_info->src_y_endian;
	ret = dcam_ops->cfg_path(module->dcam_dev_handle,
			DCAM_PATH_CFG_BASE,
			ch->dcam_path_id, &ch_desc);

	ch_desc.input_size.w = proc_info->src_size.width;
	ch_desc.input_size.h = proc_info->src_size.height;
	ch_desc.input_trim.start_x = 0;
	ch_desc.input_trim.start_y = 0;
	ch_desc.input_trim.size_x = ch_desc.input_size.w;
	ch_desc.input_trim.size_y = ch_desc.input_size.h;
	ch_desc.output_size = ch_desc.input_size;
	ch_desc.priv_size_data = NULL;
	ret = dcam_ops->cfg_path(module->dcam_dev_handle,
			DCAM_PATH_CFG_SIZE,
			ch->dcam_path_id, &ch_desc);

	/* specify isp context & path */
	max_size.w = proc_info->src_size.width;
	max_size.h = proc_info->src_size.height;
	ret = isp_ops->get_context(module->isp_dev_handle, &max_size);
	if (ret < 0) {
		pr_err("fail to get isp context for size %d %d.\n",
				max_size.w, max_size.h);
		goto fail_ispctx;
	}
	ctx_id = ret;
	isp_ops->set_callback(module->isp_dev_handle,
		ctx_id, isp_callback, module);

	/* config isp context base */
	memset(&ctx_desc, 0, sizeof(ctx_desc));
	ctx_desc.in_fmt = proc_info->src_format;
	ctx_desc.bayer_pattern = proc_info->src_pattern;
	ctx_desc.fetch_fbd = 0;
	ctx_desc.mode_ltm = MODE_LTM_OFF;
	ctx_desc.mode_3dnr = MODE_3DNR_OFF;
	ret = isp_ops->cfg_path(module->isp_dev_handle,
			ISP_PATH_CFG_CTX_BASE, ctx_id, 0, &ctx_desc);

	isp_path_id = ISP_SPATH_CP;
	ret = isp_ops->get_path(
		module->isp_dev_handle, ctx_id, isp_path_id);
	if (ret < 0) {
		pr_err("fail to get isp path %d from context %d\n",
			isp_path_id, ctx_id);
		goto fail_isppath;
	}
	ch->isp_path_id =
		(int32_t)((ctx_id << ISP_CTXID_OFFSET) | isp_path_id);
	pr_info("get isp path : 0x%x\n", ch->isp_path_id);

	memset(&isp_path_desc, 0, sizeof(isp_path_desc));
	isp_path_desc.out_fmt = IMG_PIX_FMT_NV21;
	isp_path_desc.endian.y_endian = ENDIAN_LITTLE;
	isp_path_desc.endian.uv_endian = ENDIAN_LITTLE;
	isp_path_desc.output_size.w = proc_info->dst_size.width;
	isp_path_desc.output_size.h = proc_info->dst_size.height;
	ret = isp_ops->cfg_path(module->isp_dev_handle,
			ISP_PATH_CFG_PATH_BASE,
			ctx_id, isp_path_id, &isp_path_desc);

	/* config isp input/path size */
	ctx_size.src.w = proc_info->src_size.width;
	ctx_size.src.h = proc_info->src_size.height;
	ctx_size.crop.start_x = 0;
	ctx_size.crop.start_y = 0;
	ctx_size.crop.size_x = ctx_size.src.w;
	ctx_size.crop.size_y = ctx_size.src.h;
	ret = isp_ops->cfg_path(module->isp_dev_handle,
			ISP_PATH_CFG_CTX_SIZE, ctx_id, 0, &ctx_size);

	path_trim.start_x = 0;
	path_trim.start_y = 0;
	path_trim.size_x = proc_info->src_size.width;
	path_trim.size_y = proc_info->src_size.height;
	ret = isp_ops->cfg_path(module->isp_dev_handle,
			ISP_PATH_CFG_PATH_SIZE,
			ctx_id, isp_path_id, &path_trim);

	ch->enable = 1;
	atomic_set(&module->state, CAM_CFG_CH);
	pr_info("done, dcam path %d, isp_path 0x%x\n",
		dcam_path_id, ch->isp_path_id);
	return 0;

fail_isppath:
	isp_ops->put_context(module->isp_dev_handle, ctx_id);
fail_ispctx:
	dcam_ops->put_path(module->dcam_dev_handle, ch->dcam_path_id);
	ch->dcam_path_id = -1;
	ch->isp_path_id = -1;
	ch->aux_dcam_path_id = -1;

	pr_err("failed\n");
	return ret;
}

static int raw_proc_post(
		struct camera_module *module,
		struct isp_raw_proc_info *proc_info)
{
	int ret = 0;
	uint32_t width;
	uint32_t height;
	size_t size;
	struct channel_context *ch;
	struct camera_frame *src_frame;
	struct camera_frame *mid_frame;
	struct camera_frame *dst_frame;

	ch = &module->channel[CAM_CH_CAP];
	if (ch->enable == 0) {
		pr_err("error: pre proc is not done.\n");
		return -EFAULT;
	}

	ret = dcam_ops->ioctl(module->dcam_dev_handle,
				DCAM_IOCTL_INIT_STATIS_Q, NULL);

	src_frame = get_empty_frame();
	src_frame->buf.type = CAM_BUF_USER;
	src_frame->buf.mfd[0] = proc_info->fd_src;
	src_frame->channel_id = ch->ch_id;
	src_frame->width = proc_info->src_size.width;
	src_frame->height = proc_info->src_size.height;
	ret = cambuf_get_ionbuf(&src_frame->buf);
	if (ret)
		goto src_fail;

	dst_frame = get_empty_frame();
	dst_frame->buf.type = CAM_BUF_USER;
	dst_frame->buf.mfd[0] = proc_info->fd_dst1;
	dst_frame->channel_id = ch->ch_id;
	ret = cambuf_get_ionbuf(&dst_frame->buf);
	if (ret)
		goto dst_fail;

	mid_frame = get_empty_frame();
	mid_frame->channel_id = ch->ch_id;
	/* if user set this buffer, we use it for dcam output
	 * or else we will allocate one for it.
	 */
	if (proc_info->fd_dst0 > 0) {
		mid_frame->buf.type = CAM_BUF_USER;
		mid_frame->buf.mfd[0] = proc_info->fd_dst0;
		mid_frame->buf.offset[0] = proc_info->dst0_offset;
		ret = cambuf_get_ionbuf(&mid_frame->buf);
		if (ret)
			goto mid_fail;
	} else {
		width = proc_info->src_size.width;
		height = proc_info->src_size.height;
		/* todo: accurate buffer size for formats other than mipi-raw*/
		if (proc_info->src_format == IMG_PIX_FMT_GREY)
			size = cal_sprd_raw_pitch(width) * height;
		else
			size = width * height * 3;
		size = ALIGN(size, CAM_BUF_ALIGN_SIZE);
		ret = cambuf_alloc(&mid_frame->buf,
				size, 0, module->iommu_enable);
		if (ret)
			goto mid_fail;
	}

	ret = dcam_ops->cfg_path(module->dcam_dev_handle,
			DCAM_PATH_CFG_OUTPUT_BUF,
			ch->dcam_path_id, mid_frame);
	if (ret) {
		pr_err("fail to cfg dcam out buffer.\n");
		goto dcam_out_fail;
	}

	ret = isp_ops->cfg_path(module->isp_dev_handle,
			ISP_PATH_CFG_OUTPUT_BUF,
			ch->isp_path_id >> ISP_CTXID_OFFSET,
			ch->isp_path_id & ISP_PATHID_MASK, dst_frame);
	if (ret)
		pr_err("fail to cfg isp out buffer.\n");

	pr_info("raw proc, src %p, mid %p, dst %p\n",
		src_frame, mid_frame, dst_frame);

	camera_queue_init(&ch->share_buf_queue,
			CAM_SHARED_BUF_NUM, 0, put_k_frame);
	camera_queue_init(&module->frm_queue,
		CAM_FRAME_Q_LEN, 0, camera_put_empty_frame);
	module->cap_status = CAM_CAPTURE_RAWPROC;
	atomic_set(&module->state, CAM_RUNNING);
	ret = dcam_ops->proc_frame(module->dcam_dev_handle, src_frame);
	if (ret)
		pr_err("fail to start dcam proc frame\n");

	atomic_set(&module->timeout_flag, 1);
	ret = sprd_start_timer(&module->cam_timer, CAMERA_TIMEOUT);

	return ret;

dcam_out_fail:
	if (mid_frame->buf.type == CAM_BUF_USER)
		cambuf_put_ionbuf(&mid_frame->buf);
	else
		cambuf_free(&mid_frame->buf);
mid_fail:
	put_empty_frame(mid_frame);
	cambuf_put_ionbuf(&dst_frame->buf);
dst_fail:
	put_empty_frame(dst_frame);
	cambuf_put_ionbuf(&src_frame->buf);
src_fail:
	put_empty_frame(src_frame);
	pr_err("failed\n");
	return ret;
}


static int img_ioctl_raw_proc(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	int error_state = 0;
	struct isp_raw_proc_info proc_info;

	ret = copy_from_user(&proc_info, (void __user *)arg,
				sizeof(struct isp_raw_proc_info));
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	if (!module->dcam_dev_handle || !module->isp_dev_handle) {
		pr_err("error: not init hw resource.\n");
		return -EFAULT;
	}

	error_state = ((proc_info.cmd == RAW_PROC_PRE) &&
		(atomic_read(&module->state) != CAM_IDLE));
	error_state |= ((proc_info.cmd == RAW_PROC_POST) &&
		(atomic_read(&module->state) != CAM_CFG_CH));
	if (error_state) {
		pr_info("cam%d rawproc %d error state: %d\n",
				module->idx, proc_info.cmd,
				atomic_read(&module->state));
		return -EFAULT;
	}

	if (proc_info.cmd == RAW_PROC_PRE) {
		ret = raw_proc_pre(module, &proc_info);
	} else if (proc_info.cmd == RAW_PROC_POST) {
		ret = raw_proc_post(module, &proc_info);
	} else if (proc_info.cmd == RAW_PROC_DONE) {
		ret = raw_proc_done(module);
	} else {
		pr_err("error: unknown cmd %d\n", proc_info.cmd);
		ret = -EINVAL;
	}
	return ret;
}



/* set addr for 4in1 raw which need remosaic
 */
static int img_ioctl_4in1_set_raw_addr(struct camera_module *module,
					unsigned long arg)
{
	int ret;
	enum dcam_id idx;
	struct sprd_img_parm param;
	struct channel_context *ch;
	struct camera_frame *pframe;
	int i;

	ret = copy_from_user(&param, (void __user *)arg,
				sizeof(struct sprd_img_parm));
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}
	idx = module->idx;
	if ((param.channel_id >= CAM_CH_MAX) ||
		(param.buffer_count == 0) ||
		(module->channel[param.channel_id].enable == 0)) {
		pr_err("error: invalid channel id %d. buf cnt %d\n",
				param.channel_id,  param.buffer_count);
		return -EFAULT;
	}
	pr_debug("ch %d, buffer_count %d\n", param.channel_id,
		param.buffer_count);

	ch = &module->channel[param.channel_id];
	for (i = 0; i < param.buffer_count; i++) {
		pframe = get_empty_frame();
		if (pframe == NULL) {
			pr_err("fail to get empty frame node\n");
			ret = -EFAULT;
			break;
		}
		pframe->buf.type = CAM_BUF_USER;
		pframe->buf.mfd[0] = param.fd_array[i];
		pframe->channel_id = ch->ch_id;
		pr_debug("frame %p,  mfd %d, reserved %d\n",
			pframe, pframe->buf.mfd[0], param.is_reserved_buf);

		ret = cambuf_get_ionbuf(&pframe->buf);
		if (ret) {
			pr_err("fail cambuf_get_ionbuf\n");
			put_empty_frame(pframe);
			ret = -EFAULT;
			break;
		}

		if (param.is_reserved_buf) {
			ch->reserved_buf_fd = pframe->buf.mfd[0];
			ret = dcam_ops->cfg_path(module->dcam_dev_handle,
					DCAM_PATH_CFG_OUTPUT_RESERVED_BUF,
					ch->dcam_path_id, pframe);
		} else {
			ret = dcam_ops->cfg_path(module->dcam_dev_handle,
					DCAM_PATH_CFG_OUTPUT_BUF,
					ch->dcam_path_id, pframe);
		}

		if (ret) {
			pr_err("fail to set output buffer for ch%d.\n",
				ch->ch_id);
			cambuf_put_ionbuf(&pframe->buf);
			put_empty_frame(pframe);
			ret = -EFAULT;
			break;
		}
	}
	pr_debug("exit, ret = %d\n", ret);

	return ret;
}

/* buffer set to kernel after remosaic
 */
static int img_ioctl_4in1_post_proc(struct camera_module *module,
					unsigned long arg)
{
	struct sprd_img_parm param;
	int ret = 0;
	int iommu_enable;
	struct channel_context *channel;
	struct camera_frame *pframe;

	ret = copy_from_user(&param, (void __user *)arg,
				sizeof(struct sprd_img_parm));
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	pr_info("E\n");
	/* about image data address
	 * fetch: from HAL, in param
	 * dcam out: alloc by kernel, share buf
	 */
	channel = &module->channel[CAM_CH_CAP];
	iommu_enable = module->iommu_enable;
	pframe = get_empty_frame();
	if (pframe) {
		pframe->width = channel->ch_uinfo.src_size.w;
		pframe->height = channel->ch_uinfo.src_size.h;
		pframe->buf.type = CAM_BUF_USER;
		pframe->buf.mfd[0] = param.fd_array[0];
		pframe->channel_id = channel->ch_id;
	} else {
		pr_err("no empty frame.\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = dcam_ops->proc_frame(module->aux_dcam_dev, pframe);


	return ret;

exit:
	return ret;
}

/*--------------- Core controlling interface end --------------- */



/* for user test isp/dcam/fd directly. */
static int test_dcam(struct camera_module *module,
			struct dev_test_info *test_info)
{
	int ret = 0;
	int dcam_idx = 0;
	int iommu_enable;
	int path_id;
	size_t size;
	struct channel_context *channel;
	struct camera_frame *pframe;
	struct camera_frame *user_frame;
	struct dcam_path_cfg_param ch_desc = { 0 };
	void *dcam;

	if (test_info->dev == 1)
		dcam_idx = 0;
	else
		dcam_idx = 1;

	pr_info("test dcam %d\n", dcam_idx);

	/* test dcam only */
	dcam = module->dcam_dev_handle;
	if (dcam == NULL) {
		dcam = dcam_if_get_dev(dcam_idx, module->grp->dcam[dcam_idx]);
		if (IS_ERR_OR_NULL(dcam)) {
			pr_err("fail to get dcam%d\n", dcam_idx);
			ret = -EINVAL;
			goto exit;
		}
		ret = dcam_ops->open(dcam);
		if (ret) {
			dcam_if_put_dev(dcam);
			goto exit;
		}

		ret = dcam_ops->set_callback(dcam, dcam_callback, module);
		if (ret) {
			pr_err("fail to set cam%d callback for dcam.\n",
				dcam_idx);
			dcam_ops->close(dcam);
			dcam_if_put_dev(dcam);
			goto exit;
		}
		pr_info("dcam module open done: %p\n", dcam);
		module->dcam_dev_handle = dcam;
	}

	channel = &module->channel[0];
	if (dcam_ops->get_path) {
		path_id = DCAM_PATH_BIN;
		ret = dcam_ops->get_path(module->dcam_dev_handle,
					path_id);
		if (ret < 0) {
			pr_err("fail to get dcam full path\n");
			goto exit;
		} else {
			channel->dcam_path_id = path_id;
			pr_info("Get full path for dcam offline test.\n");
		}
	}

	if (channel->dcam_path_id >= 0) {
		/* no trim in DCAM full path for offline test. */
		ch_desc.is_raw = 0;
		ch_desc.is_loose = 0;
		ch_desc.frm_deci = 0;
		ch_desc.frm_skip = 0;
		ch_desc.endian.y_endian = ENDIAN_LITTLE;
		ch_desc.input_size.w = test_info->input_size.width;
		ch_desc.input_size.h = test_info->input_size.height;
		ch_desc.input_trim.size_x = ch_desc.input_size.w;
		ch_desc.input_trim.size_y = ch_desc.input_size.h;
		ch_desc.output_size = ch_desc.input_size;
		ch_desc.force_rds = 0;
		ch_desc.priv_size_data = NULL;
		ret = dcam_ops->cfg_path(module->dcam_dev_handle,
					DCAM_PATH_CFG_BASE,
					channel->dcam_path_id,
					&ch_desc);
		ret = dcam_ops->cfg_path(module->dcam_dev_handle,
					DCAM_PATH_CFG_SIZE,
					channel->dcam_path_id,
					&ch_desc);
	}

	iommu_enable = test_info->iommu_en;
	pframe = get_empty_frame();
	if (pframe) {
		pframe->channel_id = channel->ch_id;
		pframe->is_reserved = 0;
		pframe->fid = 0;
		pframe->width = ch_desc.input_size.w;
		pframe->height = ch_desc.input_size.h;
		size = cal_sprd_raw_pitch(ch_desc.input_size.w);
		size *= ch_desc.input_size.h;
		ret = cambuf_alloc(
				&pframe->buf, size,
				0, iommu_enable);
		if (ret) {
			pr_err("fail to alloc buffer.\n");
			put_empty_frame(pframe);
			goto nobuf;
		}
		cambuf_kmap(&pframe->buf);
	} else {
		pr_err("no empty frame.\n");
		goto exit;
	}

	user_frame = get_empty_frame();
	if (user_frame) {
		user_frame->channel_id = channel->ch_id;
		user_frame->buf.type = CAM_BUF_USER;
		user_frame->buf.mfd[0] = test_info->inbuf_fd;
		cambuf_get_ionbuf(&user_frame->buf);
		pr_info("src buf kaddr %p.\n",
				(void *)user_frame->buf.addr_k[0]);
	} else {
		pr_err("no empty frame.\n");
		goto exit;
	}

	pr_info("copy source image.\n");
	/* todo: user should alloc new buffer for output */
	/* memcpy((void *)pframe->buf.addr_k[0],
			(void *)user_frame->buf.addr_k[0], size); */

	ret = dcam_ops->cfg_path(module->dcam_dev_handle,
				DCAM_PATH_CFG_OUTPUT_BUF,
				channel->dcam_path_id, user_frame);

	camera_queue_init(&channel->share_buf_queue,
			CAM_SHARED_BUF_NUM, 0, put_k_frame);
	camera_queue_init(&module->frm_queue,
			CAM_FRAME_Q_LEN, 0, camera_put_empty_frame);

	atomic_set(&module->state, CAM_RUNNING);

	ret = dcam_ops->proc_frame(module->dcam_dev_handle, pframe);

	pr_info("start wait for DCAM callback.\n");
	ret = wait_for_completion_interruptible(&module->frm_com);
	pr_info("wait done.\n");
	user_frame = camera_dequeue(&module->frm_queue);

	cambuf_put_ionbuf(&user_frame->buf);
	put_empty_frame(user_frame);

	atomic_set(&module->state, CAM_IDLE);

	dcam_ops->put_path(module->dcam_dev_handle, channel->dcam_path_id);
	dcam_ops->close(module->dcam_dev_handle);

	camera_queue_clear(&module->frm_queue);
	camera_queue_clear(&channel->share_buf_queue);

	return ret;

nobuf:
	dcam_ops->put_path(module->dcam_dev_handle, channel->dcam_path_id);
	dcam_ops->close(module->dcam_dev_handle);
exit:
	return ret;
}

static int test_isp(struct camera_module *module,
			struct dev_test_info *test_info)
{
	int ret = 0;
	int i;
	int iommu_enable;
	int ctx_id, path_id;
	int prop_0, prop_1;
	int double_ch = 0;
	size_t size = 0;
	struct img_size max_size;
	struct channel_context *channel = NULL;
	struct channel_context *channel_1 = NULL;
	struct camera_frame *pframe_in = NULL;
	struct camera_frame *pframe_out = NULL;
	struct camera_frame *pframe_out_1 = NULL;
	void *isp = NULL;

	isp = module->isp_dev_handle;
	if (isp == NULL) {
		isp = get_isp_pipe_dev();
		if (IS_ERR_OR_NULL(isp)) {
			pr_err("fail to get isp\n");
			module->isp_dev_handle = NULL;
			ret = -EINVAL;
			goto exit;
		}

		ret = isp_ops->open(isp, module->grp->isp[0]);
		if (ret) {
			pr_err("faile to enable isp module.\n");
			put_isp_pipe_dev(isp);
			ret = -EINVAL;
			goto exit;
		}
		module->isp_dev_handle = isp;
		pr_info("isp module open done: %p\n", isp);
	}

	channel = &module->channel[0];
	channel_1 = &module->channel[1];

	prop_0 = test_info->prop & 0xf;
	if ((test_info->prop >> 8) != 0) {
		double_ch = 1;
		prop_1 = (test_info->prop >> 4) & 0xf;
		if (prop_0 == prop_1)
			double_ch = 0;
	}

	if (test_info->input_size.width > test_info->output_size.width)
		max_size.w = test_info->input_size.width;
	else
		max_size.w = test_info->output_size.width;
	max_size.h = test_info->input_size.height;

	ret = isp_ops->get_context(module->isp_dev_handle, &max_size);
	if (ret < 0) {
		pr_err("fail to get isp context for size %d %d.\n",
				max_size.w, max_size.h);
	} else {
		ctx_id = ret;
		if (prop_0 <= PROP_CAP)
			path_id = ISP_SPATH_CP;
		else if (prop_0 == PROP_FD)
			path_id = ISP_SPATH_FD;
		else if (prop_0 == PROP_VIDEO)
			path_id = ISP_SPATH_VID;
		else
			path_id = ISP_SPATH_CP;

		ret = isp_ops->get_path(module->isp_dev_handle, ctx_id,
			path_id);
		if (ret < 0) {
			pr_err("fail to get isp path %d from context %d\n",
				path_id, ctx_id);
			goto exit;
		}
		channel->isp_path_id = (int32_t)((
			ctx_id << ISP_CTXID_OFFSET) | path_id);
		pr_info("get isp path : %d\n", channel->isp_path_id);
		ret = isp_ops->set_callback(module->isp_dev_handle,
					ctx_id, isp_callback, module);
		if (ret)
			pr_err("fail to set cam%d callback for isp.\n",
				module->idx);

		if (double_ch) {
			if (prop_1 <= PROP_CAP)
				path_id = ISP_SPATH_CP;
			else if (prop_1 == PROP_FD)
				path_id = ISP_SPATH_FD;
			else if (prop_1 == PROP_VIDEO)
				path_id = ISP_SPATH_VID;
			else
				path_id = ISP_SPATH_CP;

			ret = isp_ops->get_path(module->isp_dev_handle,
						ctx_id, path_id);
			if (ret < 0) {
				pr_err("fail to get isp path %d from ctx %d\n",
					path_id, ctx_id);
				goto exit;
			}
			channel_1->isp_path_id = (int32_t)((
				ctx_id << ISP_CTXID_OFFSET) | path_id);
			pr_info("get isp path : %d\n", channel_1->isp_path_id);
		}
	}

	if (channel->isp_path_id >= 0) {
		struct isp_ctx_base_desc ctx_desc;
		struct isp_path_base_desc path_desc;
		struct isp_ctx_size_desc ctx_size;
		struct img_trim path_trim;

		/* todo: cfg param to user setting. */
		ctx_desc.in_fmt = test_info->in_fmt;
		ctx_desc.bayer_pattern = COLOR_ORDER_GB;
		ctx_desc.fetch_fbd = 0;
		ctx_desc.mode_ltm = MODE_LTM_OFF;
		ctx_desc.mode_3dnr = MODE_3DNR_OFF;
		ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_CTX_BASE,
				channel->isp_path_id >> ISP_CTXID_OFFSET, 0,
				&ctx_desc);

		ctx_size.src.w = test_info->input_size.width;
		ctx_size.src.h = test_info->input_size.height;
		ctx_size.crop.start_x = 0;
		ctx_size.crop.start_y = 0;
		ctx_size.crop.size_x = ctx_size.src.w;
		ctx_size.crop.size_y = ctx_size.src.h;
		ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_CTX_SIZE,
				channel->isp_path_id >> ISP_CTXID_OFFSET, 0,
				&ctx_size);

		path_desc.out_fmt = test_info->out_fmt;
		path_desc.endian.y_endian = ENDIAN_LITTLE;
		path_desc.endian.uv_endian = ENDIAN_LITTLE;
		path_desc.output_size.w = test_info->output_size.width;
		path_desc.output_size.h = test_info->output_size.height;
		ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_PATH_BASE,
				channel->isp_path_id >> ISP_CTXID_OFFSET,
				channel->isp_path_id & ISP_PATHID_MASK,
				&path_desc);

		if (double_ch && channel_1->isp_path_id >= 0)
			ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_PATH_BASE,
				channel_1->isp_path_id >> ISP_CTXID_OFFSET,
				channel_1->isp_path_id & ISP_PATHID_MASK,
				&path_desc);

		path_trim.start_x = 0;
		path_trim.start_y = 0;
		path_trim.size_x = ctx_size.src.w;
		path_trim.size_y = ctx_size.src.h;
		ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_PATH_SIZE,
				channel->isp_path_id >> ISP_CTXID_OFFSET,
				channel->isp_path_id & ISP_PATHID_MASK,
					&path_trim);

		if (double_ch && channel_1->isp_path_id >= 0)
			ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_PATH_SIZE,
				channel_1->isp_path_id >> ISP_CTXID_OFFSET,
				channel_1->isp_path_id & ISP_PATHID_MASK,
					&path_trim);

	}

	iommu_enable = test_info->iommu_en;

	pframe_in = get_empty_frame();
	if (pframe_in) {
		pframe_in->buf.type = CAM_BUF_USER;
		pframe_in->buf.mfd[0] = test_info->inbuf_fd;
		pframe_in->channel_id = channel->ch_id;
		cambuf_get_ionbuf(&pframe_in->buf);
	}

	pframe_out = get_empty_frame();
	if (pframe_out) {
		pframe_out->buf.type = CAM_BUF_USER;
		pframe_out->buf.mfd[0] = test_info->outbuf_fd;
		pframe_out->channel_id = channel->ch_id;
		cambuf_get_ionbuf(&pframe_out->buf);
	}

	ret = isp_ops->cfg_path(module->isp_dev_handle,
			ISP_PATH_CFG_OUTPUT_BUF,
			channel->isp_path_id >> ISP_CTXID_OFFSET,
			channel->isp_path_id & ISP_PATHID_MASK,
			pframe_out);

	if (double_ch && channel_1->isp_path_id >= 0) {
		pframe_out_1 = get_empty_frame();
		if (pframe_out_1) {
			pframe_out_1->channel_id = channel_1->ch_id;
			pframe_out_1->is_reserved = 0;
			pframe_out_1->fid = 0;
			size = cal_sprd_raw_pitch(test_info->input_size.width);
			size *= test_info->input_size.height;
			ret = cambuf_alloc(
					&pframe_out_1->buf, size,
					0, iommu_enable);
			if (ret) {
				pr_err("fail to alloc buffer.\n");
				put_empty_frame(pframe_out_1);
				goto nobuf;
			}
		}
		ret = isp_ops->cfg_path(module->isp_dev_handle,
				ISP_PATH_CFG_OUTPUT_BUF,
				channel_1->isp_path_id >> ISP_CTXID_OFFSET,
				channel_1->isp_path_id & ISP_PATHID_MASK,
				pframe_out_1);
	}

	camera_queue_init(&channel->share_buf_queue,
			CAM_SHARED_BUF_NUM, 0, put_k_frame);
	camera_queue_init(&module->frm_queue,
			CAM_FRAME_Q_LEN, 0, camera_put_empty_frame);

	atomic_set(&module->state, CAM_RUNNING);

	ret = isp_ops->proc_frame(module->isp_dev_handle,
			pframe_in, channel->isp_path_id >> ISP_CTXID_OFFSET);

	for (i = 0; i < (double_ch + 1); i++) {
		pr_info("start wait for ISP callback.\n");
		ret = wait_for_completion_interruptible(&module->frm_com);
		pr_info("wait done.\n");

		pframe_out = camera_dequeue(&module->frm_queue);
		pr_info("get ch %d dst frame %p. mfd %d\n",
			pframe_out->channel_id, pframe_out,
			pframe_out->buf.mfd[0]);
		if (pframe_out->buf.type == CAM_BUF_USER)
			cambuf_put_ionbuf(&pframe_out->buf);
		else
			cambuf_free(&pframe_out->buf);
		put_empty_frame(pframe_out);
	}

	camera_queue_clear(&module->frm_queue);
	pframe_in = camera_dequeue(&channel->share_buf_queue);
	if (pframe_in) {
		cambuf_put_ionbuf(&pframe_in->buf);
		put_empty_frame(pframe_in);
	}

	atomic_set(&module->state, CAM_IDLE);
nobuf:
	isp_ops->put_path(module->isp_dev_handle,
			channel->isp_path_id >> ISP_CTXID_OFFSET,
			channel->isp_path_id & ISP_PATHID_MASK);
	if (double_ch)
		isp_ops->put_path(module->isp_dev_handle,
				channel_1->isp_path_id >> ISP_CTXID_OFFSET,
				channel_1->isp_path_id & ISP_PATHID_MASK);
	isp_ops->put_context(module->isp_dev_handle,
			channel->isp_path_id >>	ISP_CTXID_OFFSET);
exit:
	pr_info("done. ret: %d\n", ret);
	return ret;
}

static int ioctl_test_dev(
			struct camera_module *module,
			unsigned long arg)
{
	int ret = 0;
	struct sprd_img_parm param;
	struct dev_test_info *test_info = (struct dev_test_info *)&param;

	ret = copy_from_user(&param, (void __user *)arg,
				sizeof(struct sprd_img_parm));
	if (ret) {
		pr_err("fail to copy_from_user\n");
		return -EFAULT;
	}

	if (test_info->dev == 0) {
		ret = test_isp(module, test_info);
		goto exit;
	}
	ret = test_dcam(module, test_info);

exit:
	if (ret >= 0)
		ret = copy_to_user((void __user *)arg, &param,
				sizeof(struct sprd_img_parm));
	pr_info("done. ret: %d\n", ret);
	return ret;
}


static struct cam_ioctl_cmd ioctl_cmds_table[66] = {
	[_IOC_NR(SPRD_IMG_IO_SET_MODE)]		= {SPRD_IMG_IO_SET_MODE,	img_ioctl_set_mode},
	[_IOC_NR(SPRD_IMG_IO_SET_CAP_SKIP_NUM)]	= {SPRD_IMG_IO_SET_CAP_SKIP_NUM,	img_ioctl_set_cap_skip_num},
	[_IOC_NR(SPRD_IMG_IO_SET_SENSOR_SIZE)]	= {SPRD_IMG_IO_SET_SENSOR_SIZE,	img_ioctl_set_sensor_size},
	[_IOC_NR(SPRD_IMG_IO_SET_SENSOR_TRIM)]	= {SPRD_IMG_IO_SET_SENSOR_TRIM,	img_ioctl_set_sensor_trim},
	[_IOC_NR(SPRD_IMG_IO_SET_FRM_ID_BASE)]	= {SPRD_IMG_IO_SET_FRM_ID_BASE,	img_ioctl_set_frame_id_base},
	[_IOC_NR(SPRD_IMG_IO_SET_CROP)]		= {SPRD_IMG_IO_SET_CROP,	img_ioctl_set_crop},
	[_IOC_NR(SPRD_IMG_IO_SET_FLASH)]	= {SPRD_IMG_IO_SET_FLASH,	img_ioctl_set_flash},
	[_IOC_NR(SPRD_IMG_IO_SET_OUTPUT_SIZE)]	= {SPRD_IMG_IO_SET_OUTPUT_SIZE,	img_ioctl_set_output_size},
	[_IOC_NR(SPRD_IMG_IO_SET_ZOOM_MODE)]	= {SPRD_IMG_IO_SET_ZOOM_MODE,	img_ioctl_set_zoom_mode},
	[_IOC_NR(SPRD_IMG_IO_SET_SENSOR_IF)]	= {SPRD_IMG_IO_SET_SENSOR_IF,	img_ioctl_set_sensor_if},
	[_IOC_NR(SPRD_IMG_IO_SET_FRAME_ADDR)]	= {SPRD_IMG_IO_SET_FRAME_ADDR,	img_ioctl_set_frame_addr},
	[_IOC_NR(SPRD_IMG_IO_PATH_FRM_DECI)]	= {SPRD_IMG_IO_PATH_FRM_DECI,	img_ioctl_set_frm_deci},
/*	[_IOC_NR(SPRD_IMG_IO_PATH_PAUSE)]	= {SPRD_IMG_IO_PATH_PAUSE,	NULL},*/
	[_IOC_NR(SPRD_IMG_IO_PATH_RESUME)]	= {SPRD_IMG_IO_PATH_RESUME,	NULL},
	[_IOC_NR(SPRD_IMG_IO_STREAM_ON)]	= {SPRD_IMG_IO_STREAM_ON,	img_ioctl_stream_on},
	[_IOC_NR(SPRD_IMG_IO_STREAM_OFF)]	= {SPRD_IMG_IO_STREAM_OFF,	img_ioctl_stream_off},
	[_IOC_NR(SPRD_IMG_IO_GET_FMT)]		= {SPRD_IMG_IO_GET_FMT,		img_ioctl_get_fmt},
	[_IOC_NR(SPRD_IMG_IO_GET_CH_ID)]	= {SPRD_IMG_IO_GET_CH_ID,	img_ioctl_get_ch_id},
	[_IOC_NR(SPRD_IMG_IO_GET_TIME)]		= {SPRD_IMG_IO_GET_TIME,	img_ioctl_get_time},
	[_IOC_NR(SPRD_IMG_IO_CHECK_FMT)]	= {SPRD_IMG_IO_CHECK_FMT,	img_ioctl_check_fmt},
	[_IOC_NR(SPRD_IMG_IO_SET_SHRINK)]	= {SPRD_IMG_IO_SET_SHRINK,	img_ioctl_set_shrink},
	[_IOC_NR(SPRD_IMG_IO_SET_FREQ_FLAG)]	= {SPRD_IMG_IO_SET_FREQ_FLAG,	NULL},
	[_IOC_NR(SPRD_IMG_IO_CFG_FLASH)]	= {SPRD_IMG_IO_CFG_FLASH,	img_ioctl_cfg_flash},
	[_IOC_NR(SPRD_IMG_IO_PDAF_CONTROL)]	= {SPRD_IMG_IO_PDAF_CONTROL,	img_ioctl_pdaf_control},
	[_IOC_NR(SPRD_IMG_IO_GET_IOMMU_STATUS)]	= {SPRD_IMG_IO_GET_IOMMU_STATUS,	img_ioctl_get_iommu_status},
	[_IOC_NR(SPRD_IMG_IO_DISABLE_MODE)]	= {SPRD_IMG_IO_DISABLE_MODE,	NULL},
	[_IOC_NR(SPRD_IMG_IO_ENABLE_MODE)]	= {SPRD_IMG_IO_ENABLE_MODE,	NULL},
	[_IOC_NR(SPRD_IMG_IO_START_CAPTURE)]	= {SPRD_IMG_IO_START_CAPTURE,	img_ioctl_start_capture},
	[_IOC_NR(SPRD_IMG_IO_STOP_CAPTURE)]	= {SPRD_IMG_IO_STOP_CAPTURE,	img_ioctl_stop_capture},
	[_IOC_NR(SPRD_IMG_IO_SET_PATH_SKIP_NUM)]	= {SPRD_IMG_IO_SET_PATH_SKIP_NUM,	NULL},
	[_IOC_NR(SPRD_IMG_IO_SBS_MODE)]		= {SPRD_IMG_IO_SBS_MODE,	NULL},
	[_IOC_NR(SPRD_IMG_IO_DCAM_PATH_SIZE)]	= {SPRD_IMG_IO_DCAM_PATH_SIZE,	img_ioctl_dcam_path_size},
	[_IOC_NR(SPRD_IMG_IO_SET_SENSOR_MAX_SIZE)]	= {SPRD_IMG_IO_SET_SENSOR_MAX_SIZE,	img_ioctl_set_sensor_max_size},
	[_IOC_NR(SPRD_ISP_IO_IRQ)]		= {SPRD_ISP_IO_IRQ,		NULL},
	[_IOC_NR(SPRD_ISP_IO_READ)]		= {SPRD_ISP_IO_READ,		NULL},
	[_IOC_NR(SPRD_ISP_IO_WRITE)]		= {SPRD_ISP_IO_WRITE,		NULL},
	[_IOC_NR(SPRD_ISP_IO_RST)]		= {SPRD_ISP_IO_RST,		NULL},
	[_IOC_NR(SPRD_ISP_IO_STOP)]		= {SPRD_ISP_IO_STOP,		NULL},
	[_IOC_NR(SPRD_ISP_IO_INT)]		= {SPRD_ISP_IO_INT,		NULL},
	[_IOC_NR(SPRD_ISP_IO_SET_STATIS_BUF)]	= {SPRD_ISP_IO_SET_STATIS_BUF,	img_ioctl_set_statis_buf},
	[_IOC_NR(SPRD_ISP_IO_CFG_PARAM)]	= {SPRD_ISP_IO_CFG_PARAM,	img_ioctl_cfg_param},
	[_IOC_NR(SPRD_ISP_REG_READ)]		= {SPRD_ISP_REG_READ,		NULL},
	[_IOC_NR(SPRD_ISP_IO_POST_3DNR)]	= {SPRD_ISP_IO_POST_3DNR,	NULL},
	[_IOC_NR(SPRD_STATIS_IO_CFG_PARAM)]	= {SPRD_STATIS_IO_CFG_PARAM,	NULL},
	[_IOC_NR(SPRD_ISP_IO_RAW_CAP)]		= {SPRD_ISP_IO_RAW_CAP,		img_ioctl_raw_proc},

	[_IOC_NR(SPRD_IMG_IO_GET_DCAM_RES)]	= {SPRD_IMG_IO_GET_DCAM_RES,	img_ioctl_get_cam_res},
	[_IOC_NR(SPRD_IMG_IO_PUT_DCAM_RES)]	= {SPRD_IMG_IO_PUT_DCAM_RES,	img_ioctl_put_cam_res},

	[_IOC_NR(SPRD_ISP_IO_SET_PULSE_LINE)]	= {SPRD_ISP_IO_SET_PULSE_LINE,	NULL},
	[_IOC_NR(SPRD_ISP_IO_CFG_START)]	= {SPRD_ISP_IO_CFG_START,	img_ioctl_cfg_start},
	[_IOC_NR(SPRD_ISP_IO_POST_YNR)]		= {SPRD_ISP_IO_POST_YNR,	NULL},
	[_IOC_NR(SPRD_ISP_IO_SET_NEXT_VCM_POS)]	= {SPRD_ISP_IO_SET_NEXT_VCM_POS,	NULL},
	[_IOC_NR(SPRD_ISP_IO_SET_VCM_LOG)]	= {SPRD_ISP_IO_SET_VCM_LOG,	NULL},
	[_IOC_NR(SPRD_IMG_IO_SET_3DNR)]		= {SPRD_IMG_IO_SET_3DNR,	NULL},
	[_IOC_NR(SPRD_IMG_IO_SET_FUNCTION_MODE)]	= {SPRD_IMG_IO_SET_FUNCTION_MODE,	img_ioctl_set_function_mode},
	[_IOC_NR(SPRD_IMG_IO_GET_FLASH_INFO)]	= {SPRD_IMG_IO_GET_FLASH_INFO,	img_ioctl_get_flash},
	[_IOC_NR(SPRD_ISP_IO_MASK_3A)]		= {SPRD_ISP_IO_MASK_3A,		NULL},
	[_IOC_NR(SPRD_IMG_IO_SET_4IN1_ADDR)]	= {SPRD_IMG_IO_SET_4IN1_ADDR,	img_ioctl_4in1_set_raw_addr},
	[_IOC_NR(SPRD_IMG_IO_4IN1_POST_PROC)]	= {SPRD_IMG_IO_4IN1_POST_PROC,	img_ioctl_4in1_post_proc},
	[_IOC_NR(SPRD_IMG_IO_PATH_PAUSE)]	= {SPRD_IMG_IO_PATH_PAUSE,	ioctl_test_dev},
};


static long sprd_img_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	int ret = 0;
	struct camera_module *module = NULL;
	struct cam_ioctl_cmd *ioctl_cmd_p = NULL;
	int nr = _IOC_NR(cmd);

	pr_debug("cam ioctl, cmd:0x%x, cmdnum %d\n", cmd, nr);

	module = (struct camera_module *)file->private_data;
	if (!module) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	if (unlikely(!(nr >= 0 && nr < ARRAY_SIZE(ioctl_cmds_table)))) {
		pr_info("invalid cmd: 0x%xn", cmd);
		return -EINVAL;
	}

	ioctl_cmd_p = &ioctl_cmds_table[nr];
	if (unlikely((ioctl_cmd_p->cmd != cmd) ||
			(ioctl_cmd_p->cmd_proc == NULL))) {
		pr_debug("unsupported cmd_k: 0x%x, cmd_u: 0x%x, nr: %d\n",
			ioctl_cmd_p->cmd, cmd, nr);
		return 0;
	}
	mutex_lock(&module->lock);
	ret = ioctl_cmd_p->cmd_proc(module, arg);
	if (ret) {
		pr_err("fail to ioctl cmd:%x, nr:%d, func %ps\n",
			cmd, nr, ioctl_cmd_p->cmd_proc);
		mutex_unlock(&module->lock);
		return -EFAULT;
	}

	if (atomic_read(&module->state) != CAM_RUNNING)
		pr_debug("cam id:%d, %ps, done!\n",
				module->idx,
				ioctl_cmd_p->cmd_proc);
	mutex_unlock(&module->lock);
	return 0;
}

static ssize_t sprd_img_read(struct file *file, char __user *u_data,
				size_t cnt, loff_t *cnt_ret)
{
	int ret = 0;
	struct sprd_img_read_op read_op;
	struct camera_module *module = NULL;
	struct camera_frame *pframe;
	struct channel_context *pchannel;
	struct sprd_img_path_capability *cap;

	module = (struct camera_module *)file->private_data;
	if (!module) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	if (cnt != sizeof(struct sprd_img_read_op)) {
		pr_err("fail to img read, cnt %zd read_op %d\n", cnt,
			(int32_t)sizeof(struct sprd_img_read_op));
		return -EIO;
	}

	if (copy_from_user(&read_op, (void __user *)u_data, cnt)) {
		pr_err("fail to get user info\n");
		return -EFAULT;
	}

	pr_debug("cam %d read cmd %d\n", module->idx, read_op.cmd);

	switch (read_op.cmd) {
	case SPRD_IMG_GET_SCALE_CAP:
		read_op.parm.reserved[0] = 4672;
		read_op.parm.reserved[1] = 4;
		read_op.parm.reserved[2] = 4672;
		pr_debug("line threshold %d, sc factor %d, scaling %d.\n",
				read_op.parm.reserved[0],
				read_op.parm.reserved[1],
				read_op.parm.reserved[2]);
		break;
	case SPRD_IMG_GET_FRM_BUFFER:
rewait:
		memset(&read_op, 0, sizeof(struct sprd_img_read_op));
		while (1) {
			ret = wait_for_completion_interruptible(
				&module->frm_com);
			if (ret == 0) {
				break;
			} else if (ret == -ERESTARTSYS) {
				read_op.evt = IMG_SYS_BUSY;
				ret = 0;
				goto read_end;
			} else {
				pr_err("read frame buf, fail to down, %d\n",
					ret);
				return -EPERM;
			}
		}

		pchannel = NULL;
		pframe = camera_dequeue(&module->frm_queue);

		if (!pframe) {
			/* any exception happens or user trigger exit. */
			pr_err("fail to read frame buffer queue. tx stop.\n");
			read_op.evt = IMG_TX_STOP;
		} else if (pframe->evt == IMG_TX_DONE) {
			atomic_set(&module->timeout_flag, 0);
			if ((pframe->irq_type == CAMERA_IRQ_4IN1_DONE) ||
				(pframe->irq_type == CAMERA_IRQ_IMG)) {
				cambuf_put_ionbuf(&pframe->buf);
				pchannel = &module->channel[pframe->channel_id];
				if (pframe->buf.mfd[0] ==
					pchannel->reserved_buf_fd) {
					pr_info("get output buffer with reserved frame fd %d\n",
						pchannel->reserved_buf_fd);
					put_empty_frame(pframe);
					goto rewait;
				}
				read_op.parm.frame.channel_id = pframe->channel_id;
				read_op.parm.frame.index = pchannel->frm_base_id;
				read_op.parm.frame.frm_base_id = pchannel->frm_base_id;
				read_op.parm.frame.img_fmt = pchannel->ch_uinfo.dst_fmt;
			}
			read_op.evt = pframe->evt;
			read_op.parm.frame.irq_type = pframe->irq_type;
			read_op.parm.frame.irq_property = pframe->irq_property;
			read_op.parm.frame.length = pframe->width;
			read_op.parm.frame.height = pframe->height;
			read_op.parm.frame.real_index = pframe->fid;
			read_op.parm.frame.frame_id = pframe->fid;
			read_op.parm.frame.sec = pframe->time.tv_sec;
			read_op.parm.frame.usec = pframe->time.tv_usec;
			read_op.parm.frame.monoboottime = pframe->boot_time;
			read_op.parm.frame.yaddr_vir = (uint32_t)pframe->buf.addr_vir[0];
			read_op.parm.frame.uaddr_vir = (uint32_t)pframe->buf.addr_vir[1];
			read_op.parm.frame.vaddr_vir = (uint32_t)pframe->buf.addr_vir[2];
			read_op.parm.frame.mfd = pframe->buf.mfd[0];
			/* for statis buffer address below. */
			read_op.parm.frame.phy_addr = (uint32_t)pframe->buf.iova[0];
			read_op.parm.frame.addr_offset = (uint32_t)pframe->buf.addr_vir[0];
			read_op.parm.frame.vir_addr =
				(uint32_t)((uint64_t)pframe->buf.addr_vir[0] >> 32);
		} else {
			pr_err("error event %d\n", pframe->evt);
			read_op.evt = pframe->evt;
			read_op.parm.frame.irq_type = pframe->irq_type;
			read_op.parm.frame.irq_property = pframe->irq_property;
		}

		if (pframe)
			put_empty_frame(pframe);

		pr_debug("read frame, evt 0x%x irq %d ch 0x%x index 0x%x mfd %d\n",
				read_op.evt,
				read_op.parm.frame.irq_type,
				read_op.parm.frame.channel_id,
				read_op.parm.frame.real_index,
				read_op.parm.frame.mfd);
		break;

	case SPRD_IMG_GET_PATH_CAP:
		pr_debug("get path capbility\n");
		cap = &read_op.parm.capability;
		memset(cap, 0, sizeof(struct sprd_img_path_capability));
		cap->support_3dnr_mode = 1;
		cap->count = 4;
		cap->path_info[CAM_CH_RAW].support_yuv = 0;
		cap->path_info[CAM_CH_RAW].support_raw = 1;
		cap->path_info[CAM_CH_RAW].support_jpeg = 0;
		cap->path_info[CAM_CH_RAW].support_scaling = 0;
		cap->path_info[CAM_CH_RAW].support_trim = 1;
		cap->path_info[CAM_CH_RAW].is_scaleing_path = 0;
		cap->path_info[CAM_CH_PRE].line_buf = ISP_MAX_WIDTH;
		cap->path_info[CAM_CH_PRE].support_yuv = 1;
		cap->path_info[CAM_CH_PRE].support_raw = 0;
		cap->path_info[CAM_CH_PRE].support_jpeg = 0;
		cap->path_info[CAM_CH_PRE].support_scaling = 1;
		cap->path_info[CAM_CH_PRE].support_trim = 1;
		cap->path_info[CAM_CH_PRE].is_scaleing_path = 0;
		cap->path_info[CAM_CH_CAP].line_buf = ISP_MAX_WIDTH;
		cap->path_info[CAM_CH_CAP].support_yuv = 1;
		cap->path_info[CAM_CH_CAP].support_raw = 0;
		cap->path_info[CAM_CH_CAP].support_jpeg = 0;
		cap->path_info[CAM_CH_CAP].support_scaling = 1;
		cap->path_info[CAM_CH_CAP].support_trim = 1;
		cap->path_info[CAM_CH_CAP].is_scaleing_path = 0;
		cap->path_info[CAM_CH_VID].line_buf = ISP_MAX_WIDTH;
		cap->path_info[CAM_CH_VID].support_yuv = 1;
		cap->path_info[CAM_CH_VID].support_raw = 0;
		cap->path_info[CAM_CH_VID].support_jpeg = 0;
		cap->path_info[CAM_CH_VID].support_scaling = 1;
		cap->path_info[CAM_CH_VID].support_trim = 1;
		cap->path_info[CAM_CH_VID].is_scaleing_path = 0;
		break;

	default:
		pr_err("fail to get valid cmd\n");
		return -EINVAL;
	}

read_end:
	if (copy_to_user((void __user *)u_data, &read_op, cnt))
		ret = -EFAULT;

	if (ret)
		cnt = ret;

	return cnt;
}

static ssize_t sprd_img_write(struct file *file, const char __user *u_data,
				size_t cnt, loff_t *cnt_ret)
{
	int ret = 0;
	struct sprd_img_write_op write_op;
	struct camera_module *module = NULL;

	module = (struct camera_module *)file->private_data;
	if (!module) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	if (cnt != sizeof(struct sprd_img_write_op)) {
		pr_err("fail to write, cnt %zd  write_op %d\n", cnt,
				(uint32_t)sizeof(struct sprd_img_write_op));
		return -EIO;
	}

	if (copy_from_user(&write_op, (void __user *)u_data, cnt)) {
		pr_err("fail to get user info\n");
		return -EFAULT;
	}

	switch (write_op.cmd) {
	case SPRD_IMG_STOP_DCAM:
		pr_info("user stop camera %d\n", module->idx);
		complete(&module->frm_com);
		break;

	default:
		pr_err("error: unsupported write cmd %d\n", write_op.cmd);
		ret = -EINVAL;
		break;
	}

	ret =  copy_to_user((void __user *)u_data, &write_op, cnt);
	if (ret) {
		pr_err("fail to get user info\n");
		return -EFAULT;
	}

	if (ret)
		cnt = ret;

	return cnt;
}


static int sprd_img_open(struct inode *node, struct file *file)
{
	int ret = 0;
	unsigned long flag;
	struct camera_module *module = NULL;
	struct camera_group *grp = NULL;
	struct miscdevice *md = file->private_data;
	uint32_t i, idx, count = 0;

	grp = md->this_device->platform_data;
	count = grp->dcam_count;

	if (count == 0 || count > CAM_COUNT) {
		pr_err("error: invalid dts configured dcam count\n");
		return -ENODEV;
	}

	if (atomic_inc_return(&grp->camera_opened) > count) {
		pr_err("sprd_img: all %d cameras opened already.", count);
		atomic_dec(&grp->camera_opened);
		return -EMFILE;
	}

	pr_info("sprd_img: the camera opened count %d\n",
			atomic_read(&grp->camera_opened));

	spin_lock_irqsave(&grp->module_lock, flag);
	for (i = 0, idx = count; i < count; i++) {
		if ((grp->module_used & (1 << i)) == 0) {
			if (grp->module[i] != NULL) {
				pr_err("fatal: un-release camera module:  %p, idx %d\n",
						grp->module[i], i);
				spin_unlock_irqrestore(&grp->module_lock, flag);
				ret = -EMFILE;
				goto exit;
			}
			idx = i;
			grp->module_used |= (1 << i);
			break;
		}
	}
	spin_unlock_irqrestore(&grp->module_lock, flag);

	if (idx == count) {
		pr_err("error: no available camera module.\n");
		ret = -EMFILE;
		goto exit;
	}

	pr_debug("kzalloc. size of module %x, group %x\n",
		(int)sizeof(struct camera_module),
		(int) sizeof(struct camera_group));

	module = vzalloc(sizeof(struct camera_module));
	if (!module) {
		pr_err("fail to alloc camera module %d\n", idx);
		ret = -ENOMEM;
		goto alloc_fail;
	}

	module->idx = idx;
	ret = camera_module_init(module);
	if (ret) {
		pr_err("fail to init camera module %d\n", idx);
		ret = -ENOMEM;
		goto init_fail;
	}

	if (atomic_read(&grp->camera_opened) == 1) {
		isp_ops = get_isp_ops();
		dcam_ops = dcam_if_get_ops();
		flash_ops = get_flash_ops();
		if (isp_ops == NULL || dcam_ops == NULL) {
			pr_err("error:  isp ops %p, dcam ops %p\n",
					isp_ops, dcam_ops);
			goto init_fail;
		}
		/* should check all needed interface here. */

		if (grp->dcam[0] && grp->dcam[0]->pdev)
			ret = cambuf_reg_iommudev(
					&grp->dcam[0]->pdev->dev,
					CAM_IOMMUDEV_DCAM);
		if (grp->isp[0] && grp->isp[0]->pdev)
			ret = cambuf_reg_iommudev(
					&grp->isp[0]->pdev->dev,
					CAM_IOMMUDEV_ISP);

		g_empty_frm_q = &grp->empty_frm_q;
		camera_queue_init(g_empty_frm_q,
				CAM_EMP_Q_LEN_MAX, 0,
				free_empty_frame);

		pr_info("init %p\n", g_empty_frm_q);
	}

	module->idx = idx;
	module->grp = grp;
	grp->module[idx] = module;
	file->private_data = (void *)module;

	pr_info("sprd_img: open end! %d, %p, %p, grp %p\n",
		idx, module, grp->module[idx], grp);

	return 0;

init_fail:
	vfree(module);

alloc_fail:
	spin_lock_irqsave(&grp->module_lock, flag);
	grp->module_used &= ~(1 << idx);
	grp->module[idx] = NULL;
	spin_unlock_irqrestore(&grp->module_lock, flag);

exit:
	atomic_dec(&grp->camera_opened);

	pr_err("open camera failed: %d\n", ret);
	return ret;
}

/* sprd_img_release may be called for all state.
 * if release is called,
 * all other system calls in this file should be returned before.
 * state may be (RUNNING / IDLE / INIT).
 */
static int sprd_img_release(struct inode *node, struct file *file)
{
	int ret = 0;
	int idx = 0;
	unsigned long flag;
	struct camera_group *group = NULL;
	struct camera_module *module;
	struct dcam_pipe_dev *dcam_dev = NULL;
	struct isp_pipe_dev *isp_dev = NULL;

	pr_info("sprd_img: cam release start.\n");

	module = (struct camera_module *)file->private_data;
	if (!module || !module->grp) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	group = module->grp;
	idx = module->idx;

	pr_info("cam %d, state %d\n", idx,
		atomic_read(&module->state));
	pr_info("used: %d, module %p, %p, grp %p\n",
		group->module_used, module, group->module[idx], group);

	spin_lock_irqsave(&group->module_lock, flag);
	if (((group->module_used & (1 << idx)) == 0) ||
		(group->module[idx] != module)) {
		pr_err("fatal error to release camera %d. used:%x, module:%p\n",
					idx, group->module_used, module);
		spin_unlock_irqrestore(&group->module_lock, flag);
		return -EFAULT;
	}
	spin_unlock_irqrestore(&group->module_lock, flag);

	ret = img_ioctl_stream_off(module, 0L);

	if (atomic_read(&module->state) == CAM_IDLE) {
		module->attach_sensor_id = -1;

		if (module->workqueue) {
			destroy_workqueue(module->workqueue);
			module->workqueue  = NULL;
		}

		dcam_dev = module->dcam_dev_handle;
		isp_dev = module->isp_dev_handle;

		if (dcam_dev) {
			pr_info("force close dcam %p\n", dcam_dev);
			dcam_ops->close(dcam_dev);
			dcam_if_put_dev(dcam_dev);
			module->dcam_dev_handle = NULL;
		}

		if (isp_dev) {
			pr_info("force close isp %p\n", isp_dev);
			isp_ops->close(isp_dev);
			put_isp_pipe_dev(isp_dev);
			module->isp_dev_handle = NULL;
		}
	}
	camera_module_deinit(module);

	spin_lock_irqsave(&group->module_lock, flag);
	group->module_used &= ~(1 << idx);
	group->module[idx] = NULL;
	spin_unlock_irqrestore(&group->module_lock, flag);

	vfree(module);
	file->private_data = NULL;

	if (atomic_dec_return(&group->camera_opened) == 0) {

		cambuf_unreg_iommudev(CAM_IOMMUDEV_DCAM);
		cambuf_unreg_iommudev(CAM_IOMMUDEV_ISP);

		pr_info("release %p\n", g_empty_frm_q);

		/* g_leak_debug_cnt should be 0 after clr, or else memory leak.
		 */
		ret = camera_queue_clear(g_empty_frm_q);
		g_empty_frm_q = NULL;

		ret = mdbg_check();

		dcam_ops = NULL;
		isp_ops = NULL;
	}

	pr_info("sprd_img: cam %d release end.\n", idx);

	return ret;
}


static const struct file_operations image_fops = {
	.open = sprd_img_open,
	.unlocked_ioctl = sprd_img_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_sprd_img_ioctl,
#endif
	.release = sprd_img_release,
	.read = sprd_img_read,
	.write = sprd_img_write,
};

static struct miscdevice image_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = IMG_DEVICE_NAME,
	.fops = &image_fops,
};

static int sprd_img_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct camera_group *group = NULL;

	if (!pdev) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	pr_info("Start camera img probe\n");
	group = kzalloc(sizeof(struct camera_group), GFP_KERNEL);
	if (group == NULL) {
		pr_err("alloc memory fail.");
		return -ENOMEM;
	}

	ret = misc_register(&image_dev);
	if (ret) {
		pr_err("fail to register misc devices, ret %d\n", ret);
		kfree(group);
		return -EACCES;
	}

	image_dev.this_device->of_node = pdev->dev.of_node;
	image_dev.this_device->platform_data = (void *)group;
	group->md = &image_dev;
	group->pdev = pdev;
	atomic_set(&group->camera_opened, 0);
	spin_lock_init(&group->module_lock);

	pr_info("sprd img probe pdev name %s\n", pdev->name);
	pr_info("sprd dcam dev name %s\n", pdev->dev.init_name);
	ret = dcam_if_parse_dt(pdev, &group->dcam[0], &group->dcam_count);
	if (ret) {
		pr_err("fail to parse dcam dts\n");
		goto probe_pw_fail;
	}

	pr_info("sprd isp dev name %s\n", pdev->dev.init_name);
	ret = sprd_isp_parse_dt(pdev->dev.of_node,
						&group->isp[0],
						&group->isp_count);
	if (ret) {
		pr_err("fail to parse isp dts\n");
		goto probe_pw_fail;
	}

	ret = sprd_dcam_debugfs_init();
	if (ret)
		pr_err("fail to init dcam debugfs\n");

	ret = sprd_isp_debugfs_init();
	if (ret)
		pr_err("fail to init isp debugfs\n");

	return 0;

probe_pw_fail:
	misc_deregister(&image_dev);
	kfree(group);

	return ret;
}

static int sprd_img_remove(struct platform_device *pdev)
{
	struct camera_group *group = NULL;

	if (!pdev) {
		pr_err("fail to get valid input ptr\n");
		return -EFAULT;
	}

	group = image_dev.this_device->platform_data;
	if (group)
		kfree(group);
	misc_deregister(&image_dev);

	return 0;
}



static const struct of_device_id sprd_dcam_of_match[] = {
	{ .compatible = "sprd,dcam", },
	{},
};

static struct platform_driver sprd_img_driver = {
	.probe = sprd_img_probe,
	.remove = sprd_img_remove,
	.driver = {
		.name = IMG_DEVICE_NAME,
		.of_match_table = of_match_ptr(sprd_dcam_of_match),
	},
};

module_platform_driver(sprd_img_driver);

MODULE_DESCRIPTION("Sprd CAM Driver");
MODULE_AUTHOR("Multimedia_Camera@Spreadtrum");
MODULE_LICENSE("GPL");
