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
#define LOG_TAG "isp_dev_access"

#include "isp_dev_access.h"
#include "af_ctrl.h"

#define ISP_LSC_BUF_SIZE (16 * 1024)
#define ISP_LSC_BUF_NUM 1

struct isp_dev_access_context {
	cmr_handle evt_alg_handle;
	isp_evt_cb isp_event_cb;
	cmr_handle isp_driver_handle;
};

static cmr_int set_rgb_gain(cmr_handle isp_dev_handle,
		cmr_u32 *rgb_gain_coeff, cmr_u32 scene_id)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 rgb_gain_offset = 4096;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct dcam_dev_rgb_gain_info gain_info;
	struct isp_u_blocks_info block_info;

	gain_info.bypass = 0;
	gain_info.global_gain = *rgb_gain_coeff;
	gain_info.r_gain = rgb_gain_offset;
	gain_info.g_gain = rgb_gain_offset;
	gain_info.b_gain = rgb_gain_offset;

	block_info.block_info = &gain_info;
	block_info.scene_id = scene_id;
	ISP_LOGV("global_gain : %d\n", gain_info.global_gain);

	dcam_u_rgb_gain_block(cxt->isp_driver_handle, &block_info);

	return ret;
}

cmr_int isp_dev_start(cmr_handle isp_dev_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	UNUSED(cxt);

	//ret = isp_dev_start(cxt->isp_driver_handle);

	return ret;
}

cmr_int isp_dev_cfg_block(cmr_handle isp_dev_handle, void *data_ptr, cmr_int data_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_cfg_block(cxt->isp_driver_handle, data_ptr, data_id);

	return ret;
}

cmr_int isp_dev_prepare_buf(cmr_handle isp_dev_handle, struct isp_mem_info *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s32 fds[2] = { 0, 0 };
	cmr_uint kaddr[2] = { 0, 0 };
	cmr_uint lsc_phys_addr;
	cmr_u32 stats_buffer_size = 0;
	struct isp_statis_buf_input statis_buf;
	struct dcam_dev_lsc_buf lsc_buf;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if ((in_ptr->lsc_mfd <= 0) && (in_ptr->lsc_alloc_flag == 0)) {
		in_ptr->lsc_mem_num = ISP_LSC_BUF_NUM;
		in_ptr->lsc_mem_size = ISP_LSC_BUF_SIZE;
		ret = in_ptr->alloc_cb(CAMERA_ISP_LSC,
				in_ptr->oem_handle,
				&in_ptr->lsc_mem_size,
				&in_ptr->lsc_mem_num,
				&lsc_phys_addr,
				&in_ptr->lsc_u_addr,
				&in_ptr->lsc_mfd);
		if (ret) {
			ISP_LOGE("fail to alloc lsc buffer. ret %ld\n", ret);
			return ISP_ALLOC_ERROR;
		}
		ISP_LOGD("alloc lsc buffer, size %d, mfd %d\n",
				in_ptr->lsc_mem_size, in_ptr->lsc_mfd);
		in_ptr->lsc_alloc_flag = 1;
	}

	if (in_ptr->statis_mfd <= 0 && in_ptr->statis_alloc_flag == 0) {
		stats_buffer_size += STATIS_AEM_BUF_NUM * STATIS_AEM_BUF_SIZE;
		stats_buffer_size += STATIS_AFM_BUF_NUM * STATIS_AFM_BUF_SIZE;
		stats_buffer_size += STATIS_AFL_BUF_NUM * STATIS_AFL_BUF_SIZE;
		stats_buffer_size += STATIS_PDAF_BUF_NUM * STATIS_PDAF_BUF_SIZE;
		stats_buffer_size += STATIS_EBD_BUF_NUM * STATIS_EBD_BUF_SIZE;
		stats_buffer_size += STATIS_HIST_BUF_NUM * STATIS_HIST_BUF_SIZE;
		stats_buffer_size += STATIS_3DNR_BUF_NUM * STATIS_3DNR_BUF_SIZE;
		in_ptr->statis_mem_num = 1;
		/* todo: will delete (+6M size).  6M size memory for 3A statis debug. */
		in_ptr->statis_mem_size = stats_buffer_size + 6 * 1024 * 1024;
		ret = in_ptr->alloc_cb(CAMERA_ISP_STATIS,
				in_ptr->oem_handle,
				&in_ptr->statis_mem_size,
				&in_ptr->statis_mem_num,
				kaddr, &in_ptr->statis_u_addr, fds);

		if (ret || (fds[0] <= 0) || (fds[1] <= 0)) {
			ISP_LOGE("fail to alloc statis buffer. ret %ld\n", ret);
			return ISP_ALLOC_ERROR;
		}
		in_ptr->statis_mfd = fds[0];
		in_ptr->statis_k_addr = (cmr_u64)(kaddr[1] & 0xffffffff);
		in_ptr->statis_k_addr <<= 32;
		in_ptr->statis_k_addr |= (cmr_u64)(kaddr[0] & 0xffffffff);
		in_ptr->statis_alloc_flag = 1;

		ISP_LOGD("alloc statis buffer, size %d, mfd %d, kaddr 0x%llx\n",
				in_ptr->statis_mem_size, in_ptr->statis_mfd,
				(unsigned long long)in_ptr->statis_k_addr);
	}

	lsc_buf.mfd = in_ptr->lsc_mfd;
	ret = dcam_u_lsc_transaddr(cxt->isp_driver_handle, &lsc_buf);
	if (ret) {
		ISP_LOGE("lsc tranns addr failed.\n");
		return ISP_ERROR;
	}
	in_ptr->lsc_hw_addr = lsc_buf.haddr;

	/* Initialize statis buffer setting for driver. */
	statis_buf.type = STATIS_INIT;
	statis_buf.u.init_data.mfd = in_ptr->statis_mfd;
	statis_buf.u.init_data.buf_size = in_ptr->statis_mem_size;
	statis_buf.uaddr = (cmr_u64)in_ptr->statis_u_addr;
	statis_buf.kaddr = in_ptr->statis_k_addr;
	ret = isp_dev_set_statis_buf(cxt->isp_driver_handle, &statis_buf);

	return ret;
}

cmr_int isp_dev_free_buf(cmr_handle isp_dev_handle, struct isp_mem_info *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	UNUSED(isp_dev_handle);

	if (in_ptr->lsc_alloc_flag) {
		in_ptr->free_cb(CAMERA_ISP_LSC,
				in_ptr->oem_handle,
				NULL,
				&in_ptr->lsc_u_addr,
				&in_ptr->lsc_mfd,
				in_ptr->lsc_mem_size);
		in_ptr->lsc_alloc_flag = 0;
		in_ptr->lsc_mfd = 0;
		in_ptr->lsc_mem_size = 0;
	}

	if (in_ptr->statis_alloc_flag) {
		in_ptr->free_cb(CAMERA_ISP_STATIS,
				in_ptr->oem_handle,
				NULL,
				&in_ptr->statis_u_addr,
				&in_ptr->statis_mfd,
				in_ptr->statis_mem_size);
		in_ptr->statis_alloc_flag = 0;
		in_ptr->statis_mfd = 0;
		in_ptr->statis_mem_size = 0;
	}

	return ret;
}

static cmr_int set_statis_buf(cmr_handle isp_dev_handle,
				struct isp_statis_info *statis_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_statis_buf_input statis_buf;

	memset((void *)&statis_buf, 0, sizeof(statis_buf));
	statis_buf.type = statis_info->buf_type;
	statis_buf.u.block_data.hw_addr = statis_info->hw_addr;
	statis_buf.uaddr = (cmr_u64)statis_info->uaddr;
	statis_buf.kaddr = statis_info->kaddr;
	ret = isp_dev_set_statis_buf(isp_dev_handle, &statis_buf);

	return ret;
}

void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata)
{
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	cxt->isp_event_cb = isp_event_cb;
	cxt->evt_alg_handle = privdata;
}

void isp_dev_statis_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	cmr_u64 uaddr64;
	cmr_u64 kaddr64;
	struct sprd_img_statis_info *irq_info = (struct sprd_img_statis_info *)param_ptr;
	struct isp_statis_info *statis_info = NULL;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	statis_info = malloc(sizeof(struct isp_statis_info));

	/* compatable for 32/64bits application. */
	uaddr64 = (cmr_u64)irq_info->vir_addr;
	uaddr64 <<= 32;
	uaddr64 += irq_info->addr_offset;

	kaddr64 = (cmr_u64)irq_info->kaddr[1];
	kaddr64 <<= 32;
	kaddr64 |= irq_info->kaddr[0];

	statis_info->uaddr = (cmr_uint)uaddr64;
	statis_info->hw_addr = irq_info->phy_addr;
	statis_info->kaddr = kaddr64;
	statis_info->buf_type = irq_info->irq_property;
	statis_info->frame_id = irq_info->frame_id;
	statis_info->sec = irq_info->sec;
	statis_info->usec = irq_info->usec;
	ISP_LOGV("get stats type %d, uaddr %p, frame id %d time %ds.%dus",
		 statis_info->buf_type,
		 (void *)statis_info->uaddr,
		 statis_info->frame_id,
		 statis_info->sec, statis_info->usec);

	if (irq_info->irq_property == STATIS_AEM) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_AE, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == STATIS_AFM) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_AF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == STATIS_AFL) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_AFL, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == STATIS_PDAF) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_PDAF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == STATIS_EBD) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_EBD, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == STATIS_HIST) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_HIST, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == STATIS_3DNR) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_3DNR, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else {
		free((void *)statis_info);
		statis_info = NULL;
		ISP_LOGW("there is no irq_property %d", irq_info->irq_property);
	}
}

void isp_dev_irq_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	struct sprd_irq_info *irq_info = (struct sprd_irq_info *)param_ptr;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (irq_info->irq_property == IRQ_DCAM_SOF) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_EVT_SOF, NULL, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_RAW_PROC_DONE) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_EVT_TX, NULL, (void *)cxt->evt_alg_handle);
		}
	} else {
		ISP_LOGW("there is no irq_property %d", irq_info->irq_property);
	}
}

cmr_int isp_dev_access_init(cmr_s32 fd, cmr_handle *isp_dev_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = NULL;

	*isp_dev_handle = NULL;

	cxt = (struct isp_dev_access_context *)malloc(sizeof(struct isp_dev_access_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc");
		return -ISP_ALLOC_ERROR;
	}
	memset((void *)cxt, 0x00, sizeof(*cxt));

	ret = isp_dev_open(fd, &cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("fail to open isp dev!");
		goto exit;
	}

	*isp_dev_handle = (cmr_handle) cxt;

	ISP_LOGI("done %ld", ret);
	return 0;

exit:
	free((void *)cxt);
	cxt = NULL;

	ISP_LOGE("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)handle;

	ISP_CHECK_HANDLE_VALID(handle);

	isp_dev_close(cxt->isp_driver_handle);
	free((void *)cxt);
	cxt = NULL;

	ISP_LOGI("done %ld", ret);

	return ret;
}

cmr_int isp_dev_access_capability(
			cmr_handle isp_dev_handle,
			enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt;
	cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (!isp_dev_handle || !param_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	switch (cmd) {
	case ISP_VIDEO_SIZE:{
			struct isp_video_limit *size_ptr = param_ptr;
			ret = isp_dev_get_video_size(
						cxt->isp_driver_handle,
						&size_ptr->width, &size_ptr->height);
			ISP_LOGD("get video size %d %d\n", size_ptr->width, size_ptr->height);
			break;
		}
	default:
		break;
	}

exit:
	return ret;

}

/* todo: delete later. temp enable bayerhist here */
static cmr_int set_dcam_bayerhist(cmr_handle isp_dev_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt;
	struct dcam_dev_hist_info bayerHist_info;

	cxt = (struct isp_dev_access_context *)isp_dev_handle;

	memset(&bayerHist_info, 0, sizeof(bayerHist_info));
	bayerHist_info.hist_bypass = 0;
	bayerHist_info.bayer_hist_endx = 4224;
	bayerHist_info.bayer_hist_endy = 3136;
	bayerHist_info.hist_mode_sel = 1;
	bayerHist_info.hist_mul_enable = 1;
	bayerHist_info.hist_initial_clear = 1;
	bayerHist_info.hist_skip_num_clr = 1;
	ret = dcam_u_bayerhist_block(cxt->isp_driver_handle, &bayerHist_info);

	return ret;
}

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle,
	cmr_int cmd, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	switch (cmd) {
	case ISP_DEV_RESET:
		ret = isp_dev_reset(cxt->isp_driver_handle);
		break;
	/* aem */
	case ISP_DEV_SET_AE_SKIP_NUM:
		dcam_u_aem_skip_num(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AE_MONITOR_BYPASS:
		dcam_u_aem_bypass(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AE_STATISTICS_MODE:
		dcam_u_aem_mode(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		dcam_u_aem_skip_num(cxt->isp_driver_handle, *(cmr_u32 *)param1);
		break;
	case ISP_DEV_SET_AE_MONITOR_WIN:
		dcam_u_aem_win(cxt->isp_driver_handle, param0);
		/* todo: delete later. temp enable bayerhist here */
		set_dcam_bayerhist(cxt);
		break;

	/* awbc */
	case ISP_DEV_SET_AWB_GAIN:
		dcam_u_awbc_gain(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AWB_BYPASS:
		dcam_u_awbc_bypass(cxt->isp_driver_handle, *(cmr_u32 *)param0, *(cmr_u32 *)param1);
		break;

	/* afm */
	case ISP_DEV_SET_AFM_BYPASS:
		ISP_LOGD("afm bypass: %d\n", *(cmr_u32 *)param0);
		ret = dcam_u_afm_bypass(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AFM_WORK_MODE:
		ISP_LOGD("afm work_mode: %d\n", *(cmr_u32 *)param0);
		ret = dcam_u_afm_mode(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AFM_SKIP_NUM:
		ISP_LOGD("afm skip_num: %d\n", *(cmr_u32 *)param0);
		ret = dcam_u_afm_skip_num(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AFM_MONITOR_WIN:
	{
		struct af_monitor_win_rect *win_from = (struct af_monitor_win_rect *)param0;
		struct isp_img_rect win;
		win.x = win_from->x;
		win.y = win_from->y;
		win.w = win_from->w;
		win.h = win_from->h;

		/* todo: delete later. force win size to legal range. */
		ISP_LOGD("afm win_start: (%d %d)  win_size (%d %d)\n", win.x, win.y, win.w, win.h);
		win.w =  ((4224 - 4) / 6) & ~(0xf);
		win.h =  ((3136 - 4) / 3) & ~(0xf);
		ISP_LOGD("afm win_start: (%d %d)  win_size (%d %d)\n", win.x, win.y, win.w, win.h);

		ret = dcam_u_afm_win(cxt->isp_driver_handle, &win);
		break;
	}
	case ISP_DEV_SET_AFM_MONITOR_WIN_NUM:
	{
		struct af_monitor_win_num *win_from = (struct af_monitor_win_num *)param0;
		struct isp_img_size win_num;
		win_num.width = win_from->x;
		win_num.height = win_from->y;
		ISP_LOGD("afm win_num: (%d %d)\n", win_num.width, win_num.height);
		ret = dcam_u_afm_win_num(cxt->isp_driver_handle, &win_num);
		break;
	}
	case ISP_DEV_SET_AFM_CROP_EB:
		ISP_LOGD("afm crop_eb: %d\n", *(cmr_u32 *)param0);
		ret = dcam_u_afm_crop_eb(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_AFM_CROP_SIZE:
	{
		struct af_monitor_win_rect *crop_from = (struct af_monitor_win_rect *)param0;
		struct isp_img_rect crop;
		crop.x = crop_from->x;
		crop.y = crop_from->y;
		crop.w = crop_from->w;
		crop.h = crop_from->h;

		/* todo: delete later. force crop size to image size. Now af algo pass illegal win/crop size. */
		ISP_LOGD("afm crop_size: (%d %d %d %d)\n", crop.x, crop.y, crop.w, crop.h);
		crop.x = 0;
		crop.y = 0;
		crop.w = 4224;
		crop.h = 3136;
		ISP_LOGD("afm crop_size: (%d %d %d %d)\n", crop.x, crop.y, crop.w, crop.h);

		ret = dcam_u_afm_crop_size(cxt->isp_driver_handle, &crop);
		break;
	}
	case ISP_DEV_SET_AFM_DONE_TILE_NUM:
	{
		struct af_monitor_tile_num *tile_from = (struct af_monitor_tile_num *)param0;
		struct isp_img_size tile_num;
		tile_num.width = tile_from->x;
		tile_num.height = tile_from->y;
		ISP_LOGD("afm done tile num: (%d %d)\n", tile_num.width, tile_num.height);
		ret = dcam_u_afm_done_tilenum(cxt->isp_driver_handle, &tile_num);
		break;
	}
	/* afl */
	case ISP_DEV_SET_AFL_NEW_CFG_PARAM:
		ret = dcam_u_afl_new_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_AFL_NEW_BYPASS:
		ISP_LOGD("gtang : afl bypass %d\n", *(cmr_u32 *)param0);
		ret = dcam_u_afl_new_bypass(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;

	case ISP_DEV_GET_SYSTEM_TIME:
		ret = isp_dev_get_ktime(cxt->isp_driver_handle, param0, param1);
		break;
	case ISP_DEV_SET_RGB_GAIN:
		ret = set_rgb_gain(cxt, param0, 0);
		break;
	case ISP_DEV_SET_STSTIS_BUF:
		ret = set_statis_buf(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_CFG_START:
		ret = isp_dev_cfg_start(cxt->isp_driver_handle);
		break;
	case ISP_DEV_RAW_PROC:
		ret = isp_dev_raw_proc(cxt->isp_driver_handle, param0);
		break;

	default:
		break;
	}
	return ret;
}
