/*
 * Copyright (C) 2018 The Android Open Source Project
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

struct isp_dev_access_context {
	cmr_handle evt_alg_handle;
	isp_evt_cb isp_event_cb;
	cmr_handle isp_driver_handle;
	cmr_handle mem_handle;
};

#define SPLIT_MEM_SIZE 2048
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static cmr_u32 stats_mem_type[STATIS_TYPE_MAX] = {
	[STATIS_AEM] = CAMERA_ISPSTATS_AEM,
	[STATIS_AFM] = CAMERA_ISPSTATS_AFM,
	[STATIS_AFL] = CAMERA_ISPSTATS_AFL,
	[STATIS_HIST] = CAMERA_ISPSTATS_BAYERHIST,
	[STATIS_PDAF] = CAMERA_ISPSTATS_PDAF,
	[STATIS_EBD] = CAMERA_ISPSTATS_EBD,
	[STATIS_3DNR] = CAMERA_ISPSTATS_3DNR,
	[STATIS_LSCM] = CAMERA_ISPSTATS_LSCM,
	[STATIS_HIST2] = CAMERA_ISPSTATS_YUVHIST,
};

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
	cmr_u32 i, j;
	cmr_u32 alloc_type, offset, total = 0;
	cmr_uint kaddr[2] = { 0, 0 };
	struct isp_statis_buf_input statis_buf;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_stats_alloc_info *buf_info;

	memset(&statis_buf, 0, sizeof(struct isp_statis_buf_input));

	for (i = STATIS_INIT + 1; i < STATIS_TYPE_MAX; i++) {
		buf_info = &in_ptr->buf_info[i];
		if (buf_info->size == 0 || buf_info->num == 0)
			continue;

		alloc_type = stats_mem_type[i];
		if (buf_info->size <= SPLIT_MEM_SIZE) {
			buf_info->alloc_num = 1;
			buf_info->alloc_size = buf_info->size * buf_info->num;
		} else {
			buf_info->alloc_num = buf_info->num;
			buf_info->alloc_size =  buf_info->size;
		}

		buf_info->align_size = (buf_info->size + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));
		buf_info->align_alloc_size = (buf_info->alloc_size + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));
		total += buf_info->align_alloc_size * buf_info->alloc_num;

		ret = in_ptr->alloc_cb(alloc_type,
				in_ptr->oem_handle,
				&buf_info->alloc_size,
				&buf_info->alloc_num,
				kaddr,
				&buf_info->alloc_uaddr[0],
				&buf_info->alloc_mfd[0]);
		if (ret) {
			memset(&buf_info->alloc_mfd[0], 0, sizeof(buf_info->alloc_mfd));
			ISP_LOGE("fail to alloc memory type %d, size %x\n", alloc_type, buf_info->alloc_size);
			continue;
		}

		ISP_LOGV("typs %d %d,  size 0x%x num %d,  total 0x%x\n",
			i, alloc_type,
			buf_info->size, buf_info->num,
			buf_info->align_size * buf_info->alloc_num);

		if (buf_info->size <= SPLIT_MEM_SIZE) {
			if (buf_info->alloc_mfd[0] <= 0) {
				ISP_LOGE("fail to alloc buffer for %d\n", i);
				memset(buf_info, 0,sizeof(struct isp_stats_alloc_info));
				continue;
			}
			offset = 0;
			for (j = 0; j < buf_info->num; j++) {
				buf_info->mfd[j] = buf_info->alloc_mfd[0];
				buf_info->uaddr[j] = buf_info->alloc_uaddr[0] + offset;
				buf_info->offset[j] = offset;
				offset += buf_info->size;

				ISP_LOGV("mfd %d, addr %lx,  offset %d\n",
					buf_info->mfd[j], buf_info->uaddr[j], buf_info->offset[j]);

				statis_buf.mfd_array[i][j] = buf_info->mfd[j];
				statis_buf.offset_array[i][j] = buf_info->offset[j];
			}
		} else {
			for (j = 0; j < buf_info->num; j++) {
				if (buf_info->alloc_mfd[j] <= 0)
					continue;
				buf_info->mfd[j] = buf_info->alloc_mfd[j];
				buf_info->offset[j] = 0;
				buf_info->uaddr[j] = buf_info->alloc_uaddr[j];

				ISP_LOGV("idx %d, mfd %d, addr %lx,  offset %d\n",
					j, buf_info->mfd[j], buf_info->uaddr[j], buf_info->offset[j]);

				statis_buf.mfd_array[i][j] = buf_info->mfd[j];
				statis_buf.offset_array[i][j] = buf_info->offset[j];
			}
		}
	}

	/* Initialize statis buffer setting for driver. */
	statis_buf.type = STATIS_INIT;
	isp_dev_set_statis_buf(cxt->isp_driver_handle, &statis_buf);

	cxt->mem_handle = (cmr_handle)in_ptr;
	ISP_LOGD("statis buf size %d, handle %lx\n", total, (cmr_uint)cxt->mem_handle);

	return ret;
}

cmr_int isp_dev_free_buf(cmr_handle isp_dev_handle, struct isp_mem_info *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_u32 i;
	cmr_u32 alloc_type;
	struct isp_stats_alloc_info *buf_info;
	UNUSED(isp_dev_handle);

	for (i = STATIS_INIT + 1; i < STATIS_TYPE_MAX; i++) {
		buf_info = &in_ptr->buf_info[i];
		if (buf_info->alloc_num == 0) {
			memset(buf_info, 0, sizeof(struct isp_stats_alloc_info ));
			continue;
		}
		alloc_type = stats_mem_type[i];

		ISP_LOGD("free %d, num %d, mfd %d\n", alloc_type,
			buf_info->alloc_num, buf_info->alloc_mfd[0]);

		in_ptr->free_cb(alloc_type,
				in_ptr->oem_handle,
				NULL,
				&buf_info->alloc_uaddr[0],
				&buf_info->alloc_mfd[0],
				buf_info->alloc_num);
	}
	memset(&in_ptr->buf_info[0], 0, sizeof(in_ptr->buf_info));

	return ret;
}

static cmr_int set_statis_buf(struct isp_dev_access_context *cxt,
				struct isp_statis_info *statis_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_statis_buf_input statis_buf;
	struct isp_mem_info *mem_hdl;

	mem_hdl = (struct isp_mem_info *)cxt->mem_handle;
	if (!mem_hdl || !mem_hdl->invalidate_cb)
		ISP_LOGE("fail to get mem handle %p\n", mem_hdl);

	ISP_LOGV("get stats %p type %d, uaddr %p, frame id %d\n",
		 statis_info,
		 statis_info->buf_type,
		 (void *)statis_info->uaddr,
		 statis_info->frame_id);

#if 0 /* temp disable cache for statis buffer */
	if (statis_info->buf_type != STATIS_HIST2 &&
		mem_hdl && mem_hdl->invalidate_cb)
		mem_hdl->invalidate_cb(
				mem_hdl->oem_handle,
				statis_info->mfd,
				statis_info->buf_size,
				0,
				statis_info->uaddr);
#endif

	memset((void *)&statis_buf, 0, sizeof(statis_buf));
	statis_buf.type = statis_info->buf_type;
	statis_buf.mfd = statis_info->mfd;
	statis_buf.offset = statis_info->offset;
	ret = isp_dev_set_statis_buf(cxt->isp_driver_handle, &statis_buf);

	return ret;
}

static cmr_uint get_statis_buf_uaddr(
	struct isp_mem_info *mem_hdl,
	cmr_u32 *buf_size,
	cmr_u32 statis_type, cmr_s32 mfd, cmr_u32 offset)
{
	cmr_u32 j;
	struct isp_stats_alloc_info *buf_info;

	if (statis_type >= STATIS_TYPE_MAX ||
		statis_type == STATIS_INIT) {
		ISP_LOGE("invalid statis type %d\n", statis_type);
		return 0;
	}

	ISP_LOGV("type %d, mfd %d, offset %d\n", statis_type, mfd, offset);

	buf_info = &mem_hdl->buf_info[statis_type];
	for (j = 0; j < buf_info->num; j++) {
		if ((mfd > 0) && (mfd == buf_info->mfd[j])
			&& (offset == buf_info->offset[j])) {

			if (buf_size)
				*buf_size = buf_info->align_size;

			ISP_LOGV("get  idx %d,  uaddr %lx, size %d\n", j, buf_info->uaddr[j], buf_info->size);
			return buf_info->uaddr[j];
		}
	}

	return 0;
}

void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata)
{
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	cxt->isp_event_cb = isp_event_cb;
	cxt->evt_alg_handle = privdata;
}

void isp_dev_statis_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	struct sprd_img_statis_info *irq_info = (struct sprd_img_statis_info *)param_ptr;
	struct isp_statis_info *statis_info = NULL;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	statis_info = malloc(sizeof(struct isp_statis_info));
	if (statis_info == NULL) {
		ISP_LOGE("fail to malloc\n");
		return;
	}

	statis_info->buf_type = irq_info->irq_property;
	statis_info->frame_id = irq_info->frame_id;
	statis_info->sec = irq_info->sec;
	statis_info->usec = irq_info->usec;
	statis_info->zoom_ratio = irq_info->zoom_ratio;
	statis_info->width = irq_info->width;
	statis_info->height = irq_info->height;

	statis_info->mfd = irq_info->mfd;
	statis_info->offset = irq_info->addr_offset;
	statis_info->uaddr = get_statis_buf_uaddr(
				cxt->mem_handle,
				&statis_info->buf_size,
				irq_info->irq_property,
				irq_info->mfd,
				irq_info->addr_offset);

	if (statis_info->uaddr == 0UL) {
		ISP_LOGE("fail to get buf %d  %d\n", irq_info->irq_property, irq_info->mfd);
		free((void *)statis_info);
		statis_info = NULL;
		return;
	}

	ISP_LOGV("get stats %p type %d, uaddr %p, frame id %d time %ds.%dus, zoom_ratio: %d",
		 statis_info,
		 statis_info->buf_type,
		 (void *)statis_info->uaddr,
		 statis_info->frame_id,
		 statis_info->sec, statis_info->usec, statis_info->zoom_ratio);

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
	} else if (irq_info->irq_property == STATIS_HIST2) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_HIST2, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == STATIS_LSCM) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_EVT_LSC, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else {
		set_statis_buf(cxt, statis_info);
		free((void *)statis_info);
		statis_info = NULL;
		ISP_LOGW("there is no irq_property %d", irq_info->irq_property);
		return;
	}

	if (!cxt->isp_event_cb) {
		set_statis_buf(cxt, statis_info);
		free((void *)statis_info);
		statis_info = NULL;
		ISP_LOGW("there is no irq_property!");
	}
}

void isp_dev_irq_info_proc(cmr_handle isp_dev_handle, void *param_ptr)
{
	struct sprd_irq_info *irq_info = (struct sprd_irq_info *)param_ptr;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_raw_proc_info raw_proc_in;
	struct sprd_irq_info *data = NULL;

	data = (struct sprd_irq_info *)malloc(sizeof(struct sprd_irq_info));
	if (data)
		memcpy(data, irq_info, sizeof(struct sprd_irq_info));

	if (irq_info->irq_property == IRQ_DCAM_SOF) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_EVT_SOF, data, (void *)cxt->evt_alg_handle);
		}
	} else if ((irq_info->irq_property == IRQ_RAW_PROC_DONE) ||
			(irq_info->irq_property == IRQ_RAW_PROC_TIMEOUT)){
		raw_proc_in.cmd = RAW_PROC_DONE;
		isp_dev_raw_proc(cxt->isp_driver_handle, &raw_proc_in);
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_EVT_TX, data, (void *)cxt->evt_alg_handle);
		}
	} else {
		free((void *)data);
		data = NULL;
		ISP_LOGW("there is no irq_property %d", irq_info->irq_property);
	}

	if (!cxt->isp_event_cb) {
		free((void *)data);
		data = NULL;
		ISP_LOGW("there is no irq_property!");
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

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle,
	cmr_int cmd, void *param0, void *param1)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	switch (cmd) {
	case ISP_DEV_RESET:
		ret = isp_dev_reset(cxt->isp_driver_handle);
		break;
	/* bayerhist */
	case ISP_DEV_SET_BAYERHIST_CFG:
		dcam_u_bayerhist_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_BAYERHIST_BYPASS:
		dcam_u_bayerhist_bypass(cxt->isp_driver_handle, *(cmr_u32 *)param0);
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
		break;
	case ISP_DEV_SET_AE_RGB_THR:
		dcam_u_aem_rgb_thr(cxt->isp_driver_handle, param0);
		break;
	/*lsc*/
	case ISP_DEV_SET_LSC_MONITOR_BYPASS:
		dcam_u_lscm_bypass(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;
	case ISP_DEV_SET_LSC_MONITOR:
		dcam_u_lsc_monitor(cxt->isp_driver_handle, param0);
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
		ISP_LOGV("afl bypass %d\n", *(cmr_u32 *)param0);
		ret = dcam_u_afl_new_bypass(cxt->isp_driver_handle, *(cmr_u32 *)param0);
		break;

	case ISP_DEV_GET_SYSTEM_TIME:
		ret = isp_dev_get_ktime(cxt->isp_driver_handle, param0, param1);
		break;
	case ISP_DEV_SET_RGB_GAIN:
		ret = dcam_u_rgb_gain_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_STSTIS_BUF:
		ret = set_statis_buf(cxt, param0);
		break;
	case ISP_DEV_CFG_START:
		ret = isp_dev_cfg_start(cxt->isp_driver_handle);
		break;
	case ISP_DEV_RAW_PROC:
		ret = isp_dev_raw_proc(cxt->isp_driver_handle, param0);
		break;
	/* pdaf */
	case ISP_DEV_SET_PDAF_CFG_PARAM:
		ret = dcam_u_pdaf_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_PPI_INFO:
		ret = dcam_u_pdaf_ppi_info(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_BYPASS:
		ret = dcam_u_pdaf_bypass(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_WORK_MODE:
		ret = dcam_u_pdaf_work_mode(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_ROI:
		ret = dcam_u_pdaf_roi(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_SKIP_NUM:
		ret = dcam_u_pdaf_skip_num(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_TYPE1_CFG:
		ret = dcam_u_pdaf_type1_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_TYPE2_CFG:
		ret = dcam_u_pdaf_type2_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_PDAF_TYPE3_CFG:
		ret = dcam_u_pdaf_type3_block(cxt->isp_driver_handle, param0);
		break;
	case ISP_DEV_SET_DUAL_PDAF_CFG:
		ret = dcam_u_dual_pdaf_block(cxt->isp_driver_handle, param0);
		break;
	default:
		break;
	}
	return ret;
}
