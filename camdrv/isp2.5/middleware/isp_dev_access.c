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

#define ISP_LSC_BUF_SIZE (32 * 1024)
#define ISP_LSC_BUF_NUM 1

enum {
	SCENE_MODE_PRV = 0,
	SCENE_MODE_CAP,
	SCENE_MODE_MAX
};

struct isp_dev_access_context {
	cmr_handle evt_alg_handle;
	isp_evt_cb isp_event_cb;
	cmr_handle isp_driver_handle;
	struct isp_statis_mem_info statis_mem_info;
	cmr_u32 cam_4in1_flag;
};

cmr_int isp_dev_4in1_flag(cmr_handle isp_dev_handle, cmr_u32 flag)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	cxt->cam_4in1_flag = flag;
	ISP_LOGV("cam_4in1_flag = %d", cxt->cam_4in1_flag);
	return ret;
}

cmr_int isp_get_statis_buf_vir_addr(cmr_handle isp_dev_handle, struct isp_statis_info *in_ptr, cmr_uint *u_addr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (!isp_dev_handle) {
		ret = ISP_PARAM_ERROR;
		return ret;
	}

	*u_addr = cxt->statis_mem_info.isp_statis_u_addr + in_ptr->addr_offset;

	ISP_LOGV("u_addr = 0x%lx, isp_statis_u_addr = 0x%lx, addr_offset = 0x%x",
		*u_addr, cxt->statis_mem_info.isp_statis_u_addr, in_ptr->addr_offset);

	return ret;
}

cmr_int isp_dev_statis_buf_malloc(cmr_handle isp_dev_handle, struct isp_statis_mem_info *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	cmr_s32 fds[2];
	cmr_uint kaddr[2];
	cmr_u32 dcam_stats_buffer_size = 0;
	cmr_u32 isp_stats_buffer_size = 0;

	statis_mem_info->oem_handle = in_ptr->oem_handle;
	if (statis_mem_info->isp_lsc_alloc_flag == 0) {
		statis_mem_info->isp_lsc_mem_num = ISP_LSC_BUF_NUM;
		statis_mem_info->isp_lsc_mem_size = ISP_LSC_BUF_SIZE;
		if (in_ptr->alloc_cb) {
			in_ptr->alloc_cb(CAMERA_ISP_LSC,
				statis_mem_info->oem_handle,
				&statis_mem_info->isp_lsc_mem_size,
				&statis_mem_info->isp_lsc_mem_num,
				&statis_mem_info->isp_lsc_physaddr,
				&statis_mem_info->isp_lsc_virtaddr,
				&statis_mem_info->lsc_mfd);
			statis_mem_info->isp_lsc_alloc_flag = 1;
			}
		}

	if (statis_mem_info->lsc_mfd == 0) {
		ret = ISP_ALLOC_ERROR;
		ISP_LOGE("failed to malloc isp lsc buffer");
		goto exit;
	}

	if (in_ptr->statis_valid & ISP_STATIS_VALID_AEM)
		dcam_stats_buffer_size += ISP_AEM_STATIS_BUF_SIZE *
			(ISP_AEM_STATIS_BUF_NUM + 1);
	dcam_stats_buffer_size += ISP_AFM_STATIS_BUF_SIZE *
		(ISP_AFM_STATIS_BUF_NUM + 1);
	if (in_ptr->statis_valid & ISP_STATIS_VALID_AFL)
		dcam_stats_buffer_size += ISP_AFL_STATIS_BUF_SIZE *
			(ISP_AFL_STATIS_BUF_NUM + 1);
	if (in_ptr->statis_valid & ISP_STATIS_VALID_EBD)
		dcam_stats_buffer_size += ISP_EBD_STATIS_BUF_SIZE *
			(ISP_EBD_STATIS_BUF_NUM + 1);
	if (in_ptr->statis_valid & ISP_STATIS_VALID_HIST)
		isp_stats_buffer_size += ISP_HIST_STATIS_BUF_SIZE *
			ISP_HIST_STATIS_BUF_NUM;

	/* pdaf allocates memory using supports function */
	if (in_ptr->pdaf_support)
		dcam_stats_buffer_size += ISP_PDAF_STATIS_BUF_SIZE *
			(ISP_PDAF_STATIS_BUF_NUM + 1);

	statis_mem_info->statis_valid = in_ptr->statis_valid;
	if (statis_mem_info->isp_statis_alloc_flag == 0) {
		statis_mem_info->alloc_cb = in_ptr->alloc_cb;
		statis_mem_info->free_cb = in_ptr->free_cb;

		statis_mem_info->isp_dcam_stat_mem_size = dcam_stats_buffer_size;
		statis_mem_info->isp_statis_mem_size =
			statis_mem_info->isp_dcam_stat_mem_size +
			isp_stats_buffer_size;

		statis_mem_info->isp_statis_mem_num = 1;

		memset(&fds, 0x00, sizeof(fds));
		if (statis_mem_info->alloc_cb) {
			in_ptr->alloc_cb(CAMERA_ISP_STATIS,
				  statis_mem_info->oem_handle,
				  &statis_mem_info->isp_statis_mem_size,
				  &statis_mem_info->isp_statis_mem_num,
				  kaddr,
				  &statis_mem_info->isp_statis_u_addr,
				  fds);
		}
		if (fds[0] == 0 || fds[1] == 0) {
			ISP_LOGE("fail to malloc statis_bq buffer");
			ret = ISP_ALLOC_ERROR;
			goto exit;
		}
		statis_mem_info->isp_statis_k_addr[0] = kaddr[0];
		statis_mem_info->isp_statis_k_addr[1] = kaddr[1];
		statis_mem_info->statis_mfd = fds[0];
		statis_mem_info->statis_buf_dev_fd = fds[1];
		statis_mem_info->isp_statis_alloc_flag = 1;
	}

exit:

	return ret;
}

cmr_int isp_dev_trans_addr(cmr_handle isp_dev_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	struct isp_file *file = NULL;
	struct isp_statis_buf_input isp_statis_buf;
	struct isp_statis_buf_input isp_2d_lsc_buf;

	memset(&isp_statis_buf, 0x00, sizeof(isp_statis_buf));
	memset(&isp_2d_lsc_buf, 0x00, sizeof(isp_2d_lsc_buf));
	file = (struct isp_file *)(cxt->isp_driver_handle);
	file->reserved = (void *)statis_mem_info->isp_lsc_virtaddr;
	ISP_LOGI("file %p reserved %p", file, file->reserved);
	isp_2d_lsc_buf.buf_size = statis_mem_info->isp_lsc_mem_size;
	isp_2d_lsc_buf.buf_num = statis_mem_info->isp_lsc_mem_num;
	isp_2d_lsc_buf.phy_addr = statis_mem_info->isp_lsc_physaddr;
	isp_2d_lsc_buf.vir_addr = statis_mem_info->isp_lsc_virtaddr;
	isp_2d_lsc_buf.mfd = statis_mem_info->lsc_mfd;

	isp_statis_buf.buf_size = statis_mem_info->isp_statis_mem_size;
	isp_statis_buf.dcam_stat_buf_size = statis_mem_info->isp_dcam_stat_mem_size;
	isp_statis_buf.buf_num = statis_mem_info->isp_statis_mem_num;
	isp_statis_buf.kaddr[0] = statis_mem_info->isp_statis_k_addr[0];
	isp_statis_buf.kaddr[1] = statis_mem_info->isp_statis_k_addr[1];
	isp_statis_buf.vir_addr = statis_mem_info->isp_statis_u_addr;
	isp_statis_buf.buf_flag = 0;
	isp_statis_buf.mfd = statis_mem_info->statis_mfd;
	isp_statis_buf.dev_fd = statis_mem_info->statis_buf_dev_fd;
	isp_statis_buf.statis_valid = statis_mem_info->statis_valid;

	isp_u_2d_lsc_transaddr(cxt->isp_driver_handle, &isp_2d_lsc_buf, 0);

	ret = isp_dev_access_ioctl(isp_dev_handle, ISP_DEV_SET_STSTIS_BUF, &isp_statis_buf, NULL);

	return ret;
}

cmr_int isp_dev_set_interface(struct isp_drv_interface_param *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	if (!in_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	ret = isp_set_comm_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_comm_param"));

	ret = isp_set_slice_size((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_slice_size"));

	ret = isp_set_fetch_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_fetch_param"));

	ret = isp_set_store_param((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_store_param"));

	ret = isp_set_dispatch((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_dispatch"));

	ret = isp_set_arbiter((cmr_handle) in_ptr);
	ISP_TRACE_IF_FAIL(ret, ("fail to isp_set_arbiter"));

exit:
	return ret;
}

cmr_int isp_dev_start(cmr_handle isp_dev_handle, struct isp_drv_interface_param *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
#ifdef ISP_DEFAULT_CFG_FOR_BRING_UP
	struct isp_dev_cfa_info cfa_param;
#endif

	isp_u_fetch_raw_transaddr(cxt->isp_driver_handle, &in_ptr->fetch.fetch_addr);

	ret = isp_get_fetch_addr(in_ptr, &in_ptr->fetch);
	ISP_RETURN_IF_FAIL(ret, ("fail to get isp fetch addr"));

#ifdef ISP_DEFAULT_CFG_FOR_BRING_UP
	ret = isp_get_cfa_default_param(&cfa_param);
	ISP_RETURN_IF_FAIL(ret, ("fail to get isp cfa default param"));

	ret = isp_u_cfa_block(cxt->isp_driver_handle, (void *)&cfa_param);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp cfa"));
#endif

	ret = isp_u_fetch_block(cxt->isp_driver_handle, (void *)&in_ptr->fetch);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp fetch"));

	ret = isp_u_store_block(cxt->isp_driver_handle, (void *)&in_ptr->store);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp store"));

	isp_cfg_dispatch(cxt->isp_driver_handle, (void *)&in_ptr->dispatch);

	ret = isp_u_arbiter_block(cxt->isp_driver_handle, (void *)&in_ptr->arbiter);
	ISP_RETURN_IF_FAIL(ret, ("fail to cfg isp arbiter"));

	isp_cfg_comm_data(cxt->isp_driver_handle, (void *)&in_ptr->com);

	isp_cfg_slice_size(cxt->isp_driver_handle, (void *)&in_ptr->slice);

	return ret;
}

cmr_int isp_dev_anti_flicker_bypass(cmr_handle isp_dev_handle, cmr_int bypass)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (cxt->cam_4in1_flag)
		ret = isp_u_anti_flicker_bypass(cxt->isp_driver_handle, (void *)&bypass, SCENE_MODE_CAP);
	ret = isp_u_anti_flicker_bypass(cxt->isp_driver_handle, (void *)&bypass, SCENE_MODE_PRV);

	return ret;
}

cmr_int isp_dev_anti_flicker_new_bypass(cmr_handle isp_dev_handle, cmr_int bypass)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (cxt->cam_4in1_flag)
		ret = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
	ret = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, bypass, SCENE_MODE_PRV);

	return ret;
}

cmr_int isp_dev_cfg_block(cmr_handle isp_dev_handle, void *data_ptr, cmr_int data_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_cfg_block(cxt->isp_driver_handle, data_ptr, data_id);

	return ret;
}

cmr_int isp_dev_awb_gain(cmr_handle isp_dev_handle, cmr_u32 r, cmr_u32 g, cmr_u32 b)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	cmr_u32 r_gain = 0;
	cmr_u32 g_gain = 0;
	cmr_u32 b_gain = 0;

	if (cxt->cam_4in1_flag) {
		r_gain = r ;
		g_gain = g ;
		b_gain = b ;
		ISP_LOGV("r_gain = %d, g_gain = %d, b_gain = %d", r_gain, g_gain, b_gain);
		ret = isp_u_awbc_gain(cxt->isp_driver_handle, r_gain, g_gain, b_gain, SCENE_MODE_CAP);
	}
	ret = isp_u_awbc_gain(cxt->isp_driver_handle, r, g, b, SCENE_MODE_PRV);

	return ret;
}

cmr_int isp_dev_comm_shadow(cmr_handle isp_dev_handle, cmr_int shadow)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_u_comm_shadow_ctrl(cxt->isp_driver_handle, shadow, 0);

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
	struct sprd_img_statis_info *irq_info = (struct sprd_img_statis_info *)param_ptr;
	struct isp_statis_info *statis_info = NULL;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	statis_info = malloc(sizeof(*statis_info));

	statis_info->phy_addr = irq_info->phy_addr;
	statis_info->vir_addr = irq_info->vir_addr;
	statis_info->addr_offset = irq_info->addr_offset;
	statis_info->kaddr[0] = irq_info->kaddr[0];
	statis_info->kaddr[1] = irq_info->kaddr[1];
	statis_info->irq_property = irq_info->irq_property;
	statis_info->buf_size = irq_info->buf_size;
	statis_info->mfd = irq_info->mfd;
	statis_info->frame_id = irq_info->frame_id;
	statis_info->zoom_ratio = irq_info->zoom_ratio;
	statis_info->sec = irq_info->sec;
	statis_info->usec = irq_info->usec;
	statis_info->monoboottime = irq_info->monoboottime;
	statis_info->is_last_frm = irq_info->is_last_frm;
	statis_info->time_diff = irq_info->time_diff;

	ISP_LOGV("got one frame stats offset 0x%x vaddr 0x%x property %d frame id %d timestamp %ds %dus",
		 statis_info->addr_offset, statis_info->vir_addr,
		 statis_info->irq_property,
		 statis_info->frame_id,
		 statis_info->sec, statis_info->usec);
	if (irq_info->irq_property == IRQ_AEM_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_AE, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_AFM_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_AF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_AFL_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_PROC_AFL_DONE, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_PDAF_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_PDAF, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_EBD_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_CTRL_EVT_EBD, statis_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_HIST_STATIS) {
		if (cxt->isp_event_cb) {
			(*cxt->isp_event_cb) (ISP_PROC_HIST_DONE, statis_info, (void *)cxt->evt_alg_handle);
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
	struct isp_u_irq_info *irq_u_info = NULL;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (irq_info->irq_property == IRQ_DCAM_SOF) {
		if (cxt->isp_event_cb) {
			irq_u_info = malloc(sizeof(*irq_u_info));
			irq_u_info->frame_id = irq_info->frame_id;
			irq_u_info->sec = irq_info->sec;
			irq_u_info->usec = irq_info->usec;
			irq_u_info->monoboottime = irq_info->monoboottime;
			irq_u_info->is_last_frm = irq_info->is_last_frm;
			irq_u_info->time_diff = irq_info->time_diff;

			(cxt->isp_event_cb) (ISP_CTRL_EVT_SOF, irq_u_info, (void *)cxt->evt_alg_handle);
		}
	} else if (irq_info->irq_property == IRQ_RAW_CAP_DONE) {
		if (cxt->isp_event_cb) {
			(cxt->isp_event_cb) (ISP_CTRL_EVT_TX, NULL, (void *)cxt->evt_alg_handle);
		}
	} else {
		ISP_LOGW("there is no irq_property %d", irq_info->irq_property);
	}
}

cmr_int isp_dev_access_init(cmr_s32 fd, cmr_handle *isp_dev_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = NULL;

	cxt = (struct isp_dev_access_context *)malloc(sizeof(struct isp_dev_access_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	*isp_dev_handle = NULL;
	memset((void *)cxt, 0x00, sizeof(*cxt));

	ret = isp_dev_open(fd, &cxt->isp_driver_handle);

	if (ret) {
		ISP_LOGE("fail to open isp dev!");
		goto exit;
	}
exit:
	if (ret) {
		if (cxt) {
			free((void *)cxt);
			cxt = NULL;
		}
	} else {
		*isp_dev_handle = (cmr_handle) cxt;
	}

	ISP_LOGI("done %ld", ret);

	return ret;
}

cmr_int isp_dev_access_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)handle;
	struct isp_statis_mem_info *statis_mem_info = &cxt->statis_mem_info;
	cmr_uint type = 0;

	ISP_CHECK_HANDLE_VALID(handle);

	if (statis_mem_info->isp_lsc_alloc_flag == 1) {
		type = CAMERA_ISP_LSC;
		if (statis_mem_info->free_cb) {
			statis_mem_info->free_cb(type,
				statis_mem_info->oem_handle,
				&statis_mem_info->isp_lsc_physaddr,
				&statis_mem_info->isp_lsc_virtaddr,
				&statis_mem_info->lsc_mfd,
				statis_mem_info->isp_lsc_mem_num);
		}

		statis_mem_info->isp_lsc_alloc_flag = 0;
	}

	if (statis_mem_info->isp_statis_alloc_flag == 1) {
		type = CAMERA_ISP_STATIS;
		if (statis_mem_info->free_cb) {
			statis_mem_info->free_cb(type,
				statis_mem_info->oem_handle,
				&statis_mem_info->isp_statis_k_addr[0],
				&statis_mem_info->isp_statis_u_addr,
				&statis_mem_info->statis_mfd,
				statis_mem_info->isp_statis_mem_num);
		}

		statis_mem_info->isp_statis_alloc_flag = 0;
	}
	isp_dev_close(cxt->isp_driver_handle);
	if (cxt) {
		free((void *)cxt);
		cxt = NULL;
	}

	ISP_LOGI("done %ld", ret);

	return ret;
}

cmr_int isp_dev_access_capability(cmr_handle isp_dev_handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (!isp_dev_handle || !param_ptr) {
		ret = ISP_PARAM_ERROR;
		goto exit;
	}

	switch (cmd) {
	case ISP_VIDEO_SIZE:{
			struct isp_video_limit *size_ptr = param_ptr;

			ret = isp_u_capability_continue_size(cxt->isp_driver_handle, &size_ptr->width, &size_ptr->height);
			break;
		}
	default:
		break;
	}

exit:
	return ret;
}

static cmr_int ispdev_access_ae_set_stats_mode(cmr_handle isp_dev_handle, struct isp_dev_access_ae_stats_info *stats_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	switch (stats_info->mode) {
	case ISP_DEV_AE_STATS_MODE_SINGLE:
		isp_u_3a_ctrl(cxt->isp_driver_handle, 1, 0);
		if (cxt->cam_4in1_flag) {
			isp_u_raw_aem_mode(cxt->isp_driver_handle, 0, SCENE_MODE_CAP);
			isp_u_raw_aem_skip_num(cxt->isp_driver_handle, stats_info->skip_num, SCENE_MODE_CAP);
		}
		isp_u_raw_aem_mode(cxt->isp_driver_handle, 0, SCENE_MODE_PRV);
		isp_u_raw_aem_skip_num(cxt->isp_driver_handle, stats_info->skip_num, SCENE_MODE_PRV);
		break;
	case ISP_DEV_AE_STATS_MODE_CONTINUE:
		if (cxt->cam_4in1_flag) {
			isp_u_raw_aem_mode(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
			isp_u_raw_aem_skip_num(cxt->isp_driver_handle, 0, SCENE_MODE_CAP);
		}
		isp_u_raw_aem_mode(cxt->isp_driver_handle, 1, SCENE_MODE_PRV);
		isp_u_raw_aem_skip_num(cxt->isp_driver_handle, 0, SCENE_MODE_PRV);
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int ispdev_access_ae_set_rgb_gain(cmr_handle isp_dev_handle, void *rgb_gain, cmr_u32 scene_id)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_dev_rgb_gain_info *gain_info = (struct isp_dev_rgb_gain_info *)rgb_gain;
	struct isp_u_blocks_info block_info;

	memset(&block_info, 0x00, sizeof(block_info));

	block_info.block_info = (void *)gain_info;
	block_info.scene_id = scene_id;

	isp_u_rgb_gain_block(cxt->isp_driver_handle, (void *)&block_info);

	return ret;
}

static cmr_int ispdev_access_set_af_monitor(cmr_handle isp_dev_handle, struct isp_dev_access_afm_info *afm_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (cxt->cam_4in1_flag) {
		isp_u_raw_afm_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
		isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
		isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, 0, SCENE_MODE_CAP);
		isp_u_raw_afm_skip_num(cxt->isp_driver_handle, afm_info->skip_num, SCENE_MODE_CAP);
		isp_u_raw_afm_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
	}
	isp_u_raw_afm_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_PRV);
	isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, 1, SCENE_MODE_PRV);
	isp_u_raw_afm_skip_num_clr(cxt->isp_driver_handle, 0, SCENE_MODE_PRV);
	isp_u_raw_afm_skip_num(cxt->isp_driver_handle, afm_info->skip_num, SCENE_MODE_PRV);
	isp_u_raw_afm_bypass(cxt->isp_driver_handle, afm_info->bypass, SCENE_MODE_PRV);

	return ret;
}

static cmr_int ispdev_access_set_slice_raw(cmr_handle isp_dev_handle, struct isp_raw_proc_info *info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_set_slice_raw_info(cxt->isp_driver_handle, info);
	ISP_TRACE_IF_FAIL(ret, ("fail to slice raw info"));
	ret = isp_u_fetch_start_isp(cxt->isp_driver_handle, ISP_ONE);
	ISP_TRACE_IF_FAIL(ret, ("fail to fetch start isp"));

	return ret;
}

static cmr_int ispdev_access_set_aem_win(cmr_handle isp_dev_handle, struct isp_dev_access_aem_win_info *aem_win_info)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	if (cxt->cam_4in1_flag) {
		ret = isp_u_raw_aem_offset(cxt->isp_driver_handle, aem_win_info->offset.x, aem_win_info->offset.y, SCENE_MODE_CAP);
		ISP_TRACE_IF_FAIL(ret, ("fail to aem offset"));
		ret = isp_u_raw_aem_blk_size(cxt->isp_driver_handle, aem_win_info->blk_size.width, aem_win_info->blk_size.height, SCENE_MODE_CAP);
		ISP_TRACE_IF_FAIL(ret, ("fail to aem blk"));
	}

	ret = isp_u_raw_aem_offset(cxt->isp_driver_handle, aem_win_info->offset.x, aem_win_info->offset.y, SCENE_MODE_PRV);
	ISP_TRACE_IF_FAIL(ret, ("fail to aem offset"));
	ret = isp_u_raw_aem_blk_size(cxt->isp_driver_handle, aem_win_info->blk_size.width, aem_win_info->blk_size.height, SCENE_MODE_PRV);
	ISP_TRACE_IF_FAIL(ret, ("fail to aem blk"));

	return ret;
}

static cmr_int ispdev_access_get_timestamp(cmr_handle isp_dev_handle, struct isp_time *time)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	cmr_u32 sec = 0;
	cmr_u32 usec = 0;

	ret = isp_u_capability_time(cxt->isp_driver_handle, &sec, &usec);
	time->sec = sec;
	time->usec = usec;

	return ret;
}

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, cmr_int cmd, void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_dev_access_context *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	switch (cmd) {
	case ISP_DEV_SET_AE_MONITOR: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_aem_skip_num(cxt->isp_driver_handle, *(cmr_u32 *) in, SCENE_MODE_CAP);
		ret = isp_u_raw_aem_skip_num(cxt->isp_driver_handle, *(cmr_u32 *) in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AE_MONITOR_WIN: {
		ret = ispdev_access_set_aem_win(cxt, in);
		break;
	}
	case ISP_DEV_SET_AE_MONITOR_BYPASS: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_aem_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
		ret = isp_u_raw_aem_bypass(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AE_STATISTICS_MODE: {
		ret = ispdev_access_ae_set_stats_mode(cxt, in);
		break;
	}
	case ISP_DEV_SET_RGB_GAIN: {
		ret = ispdev_access_ae_set_rgb_gain(cxt, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_RGB_GAIN_FOR_4IN1: {
		ret = ispdev_access_ae_set_rgb_gain(cxt, in, SCENE_MODE_CAP);
		break;
	}
	case ISP_DEV_GET_SYSTEM_TIME: {
		ret = ispdev_access_get_timestamp(cxt, out);
		break;
	}
	case ISP_DEV_CFG_START: {
		ret = isp_dev_cfg_start(cxt->isp_driver_handle);
		break;
	}
	case ISP_DEV_SET_AFL_BLOCK: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_anti_flicker_block(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_anti_flicker_block(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AFL_NEW_BLOCK: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_RAW_AEM_BYPASS: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_aem_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
		ret = isp_u_raw_aem_bypass(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_RAW_AFM_BYPASS: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_bypass(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AF_MONITOR: {
		ret = ispdev_access_set_af_monitor(cxt, in);
		break;
	}
	case ISP_DEV_SET_AF_MONITOR_WIN: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_win(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_win(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AF_MONITOR_WIN_NUM: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_win_num(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_win_num(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_STSTIS_BUF: {
		ret = isp_dev_set_statis_buf(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_PDAF_CFG_PARAM: {
		ret = isp_u_pdaf_block(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_PDAF_BYPASS: {
		ret = isp_u_pdaf_bypass(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_PDAF_WORK_MODE: {
		ret = isp_u_pdaf_work_mode(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_PDAF_SKIP_NUM: {
		ret = isp_u_pdaf_skip_num(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_PDAF_ROI: {
		ret = isp_u_pdaf_roi(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_PDAF_TYPE1_CFG: {
		ret = isp_u_pdaf_type1_block(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_PDAF_TYPE2_CFG: {
		ret = isp_u_pdaf_type2_block(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_EBD_CFG: {
		ret = isp_u_ebd_block(cxt->isp_driver_handle);
		break;
	}
	case ISP_DEV_SET_PDAF_PPI_INFO: {
		ret = isp_u_pdaf_ppi_info(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_AFL_CFG_PARAM: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_anti_flicker_block(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_anti_flicker_block(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AFL_NEW_CFG_PARAM: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_anti_flicker_new_block(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AFL_BYPASS: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_anti_flicker_bypass(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_anti_flicker_bypass(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AFL_NEW_BYPASS: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, 1, SCENE_MODE_CAP);
		ret = isp_u_anti_flicker_new_bypass(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AE_SHIFT: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_aem_shift(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_aem_shift(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AF_WORK_MODE: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_mode(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_mode(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AF_SKIP_NUM: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_skip_num(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_skip_num(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_POST_3DNR: {
		ret = isp_dev_3dnr(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_POST_YNR: {
		ret = isp_dev_ynr(cxt->isp_driver_handle, in);
		break;
	}
	case ISP_DEV_SET_RAW_SLICE: {
		ret = ispdev_access_set_slice_raw(cxt, in);
		break;
	}
	case ISP_DEV_UPDATE_PARAM_START: {
		ret = isp_dev_update_param_start(cxt->isp_driver_handle);
		break;
	}
	case ISP_DEV_UPDATE_PARAM_END: {
		ret = isp_dev_update_param_end(cxt->isp_driver_handle);
		break;
	}
	case ISP_DEV_SET_AE_BLK_NUM: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_aem_blk_num(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_aem_blk_num(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AE_RGB_THR: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_aem_rgb_thr(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_aem_rgb_thr(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AE_SKIP_NUM_CLR: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_aem_skip_num_clr(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_aem_skip_num_clr(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AF_CROP_EB: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_crop_eb(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_crop_eb(cxt->isp_driver_handle, *(cmr_u32 *)in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AF_CROP_SIZE: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_crop_size(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_crop_size(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_SET_AF_DONE_TILE_NUM: {
		if (cxt->cam_4in1_flag)
			ret = isp_u_raw_afm_done_tile_num(cxt->isp_driver_handle, in, SCENE_MODE_CAP);
		ret = isp_u_raw_afm_done_tile_num(cxt->isp_driver_handle, in, SCENE_MODE_PRV);
		break;
	}
	case ISP_DEV_RESET: {
		ret = isp_dev_reset(cxt->isp_driver_handle);
		break;
	}
	default:
		break;
	}
	return ret;
}
