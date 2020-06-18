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
#ifndef _ISP_DEV_ACCESS_H_
#define _ISP_DEV_ACCESS_H_

#ifndef LOCAL_INCLUDE_ONLY
#error "Hi, This is only for camdrv."
#endif

#include "isp_drv.h"
#include "isp_mw.h"

#define ISP_EVT_TX				(1 << 2)
#define ISP_EVT_SOF				(1 << 3)
#define ISP_EVT_AE				(1 << 4)
#define ISP_EVT_AF				(1 << 5)
#define ISP_EVT_AFL				(1 << 6)
#define ISP_EVT_HIST			(1 << 7)
#define ISP_EVT_PDAF			(1 << 8)
#define ISP_EVT_EBD				(1 << 9)
#define ISP_EVT_3DNR			(1 << 10)
#define ISP_EVT_HIST2			(1 << 11)
#define ISP_EVT_CFG				(1 << 12)
#define ISP_EVT_LSC				(1 << 13)
#define ISP_EVT_PARAM			(1 << 14)


enum isp_dev_access_ctrl_cmd {
	ISP_DEV_SET_BAYERHIST_CFG,
	ISP_DEV_SET_BAYERHIST_BYPASS,

	ISP_DEV_SET_AE_SKIP_NUM,
	ISP_DEV_SET_AE_RGB_THR,
	ISP_DEV_SET_AE_MONITOR_WIN,
	ISP_DEV_SET_AE_MONITOR_BYPASS,
	ISP_DEV_SET_AE_STATISTICS_MODE,
	ISP_DEV_SET_AWB_GAIN,
	ISP_DEV_SET_AWB_BYPASS,

	ISP_DEV_SET_AFM_BYPASS,
	ISP_DEV_SET_AFM_WORK_MODE,
	ISP_DEV_SET_AFM_SKIP_NUM,
	ISP_DEV_SET_AFM_MONITOR_WIN,
	ISP_DEV_SET_AFM_MONITOR_WIN_NUM,
	ISP_DEV_SET_AFM_CROP_EB,
	ISP_DEV_SET_AFM_CROP_SIZE,
	ISP_DEV_SET_AFM_DONE_TILE_NUM,

	ISP_DEV_SET_AFL_NEW_CFG_PARAM,
	ISP_DEV_SET_AFL_NEW_BYPASS,

	ISP_DEV_SET_PDAF_CFG_PARAM,
	ISP_DEV_SET_PDAF_PPI_INFO,
	ISP_DEV_SET_PDAF_BYPASS,
	ISP_DEV_SET_PDAF_WORK_MODE,
	ISP_DEV_SET_PDAF_EXTRACTOR_BYPASS,
	ISP_DEV_SET_PDAF_ROI,
	ISP_DEV_SET_PDAF_SKIP_NUM,
	ISP_DEV_SET_PDAF_TYPE1_CFG,
	ISP_DEV_SET_PDAF_TYPE2_CFG,
	ISP_DEV_SET_PDAF_TYPE3_CFG,
	ISP_DEV_SET_DUAL_PDAF_CFG,
	ISP_DEV_SET_EBD_CFG,

	ISP_DEV_SET_RGB_GAIN,
	ISP_DEV_GET_SYSTEM_TIME,
	ISP_DEV_SET_STSTIS_BUF,
	ISP_DEV_INVALIDATE_STSTIS_BUFCACHE,
	ISP_DEV_CFG_START,
	ISP_DEV_RAW_PROC,

	ISP_DEV_POST_3DNR,
	ISP_DEV_RESET,

	ISP_DEV_SET_LSC_MONITOR_BYPASS,
	ISP_DEV_SET_LSC_MONITOR,
	ISP_DEV_CMD_MAX
};

typedef void (*isp_evt_cb) (cmr_int evt, void *data, void *privdata);

cmr_int isp_dev_start(cmr_handle isp_dev_handle);
cmr_int isp_dev_cfg_block(
			cmr_handle isp_dev_handle, void *data_ptr, cmr_int data_id);
cmr_int isp_dev_prepare_buf(cmr_handle isp_dev_handle, struct isp_mem_info *in_ptr);
cmr_int isp_dev_free_buf(cmr_handle isp_dev_handle, struct isp_mem_info *in_ptr);

void isp_dev_access_evt_reg(
			cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata);
void isp_dev_statis_info_proc(cmr_handle isp_dev_handle, void *param_ptr);
void isp_dev_irq_info_proc(cmr_handle isp_dev_handle, void *param_ptr);

void isp_raw_proc(cmr_handle isp_dev_handle, void *param_ptr);

cmr_int isp_dev_access_init(cmr_s32 fd, cmr_handle *isp_dev_handle);
cmr_int isp_dev_access_deinit(cmr_handle handle);
cmr_int isp_dev_access_ioctl(
			cmr_handle isp_dev_handle, cmr_int cmd,
			void *param0, void *param1);
cmr_int isp_dev_access_capability(
			cmr_handle isp_dev_handle,
			enum isp_capbility_cmd cmd, void *param_ptr);

#endif
