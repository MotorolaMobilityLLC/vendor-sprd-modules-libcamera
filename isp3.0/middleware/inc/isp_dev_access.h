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
#ifndef _ISP_DEV_ACCESS_H_
#define _ISP_DEV_ACCESS_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/

#include <sys/types.h>
#include "isp_common_types.h"
#include "isp_drv.h"
#include "isp_3a_fw.h"

enum isp_dev_access_ctrl_cmd {
	ISP_DEV_ACCESS_SET_AWB_GAIN,
	ISP_DEV_ACCESS_SET_ISP_SPEED,
	ISP_DEV_ACCESS_SET_3A_PARAM,
	ISP_DEV_ACCESS_SET_AE_PARAM,
	ISP_DEV_ACCESS_SET_AWB_PARAM,
	ISP_DEV_ACCESS_SET_AF_PARAM,
	ISP_DEV_ACCESS_SET_ANTIFLICKER,
	ISP_DEV_ACCESS_SET_YHIS_PARAM,
	ISP_DEV_ACCESS_SET_BRIGHTNESS,
	ISP_DEV_ACCESS_SET_SATURATION,
	ISP_DEV_ACCESS_SET_CONSTRACT,
	ISP_DEV_ACCESS_SET_SHARPNESS,
	ISP_DEV_ACCESS_SET_SPECIAL_EFFECT,
	ISP_DEV_ACCESS_SET_SUB_SAMPLE,
	ISP_DEV_ACCESS_SET_STATIS_BUF,
	ISP_DEV_ACCESS_GET_STATIS_BUF,
	ISP_DEV_ACCESS_GET_TIME,
	ISP_DEV_ACCESS_CMD_MAX
};

struct isp_dev_postproc_in {
	cmr_u32 src_avail_height;
	cmr_u32 src_slice_height;
	cmr_u32 dst_slice_height;
	struct isp_img_frm src_frame;
	struct isp_img_frm dst_frame;
	struct isp_3a_cfg_param hw_cfg;
	struct isp_3a_dld_sequence dldseq;
	struct isp_awb_gain awb_gain;
	struct isp_awb_gain awb_gain_balanced;
};

struct isp_dev_postproc_out {
	cmr_u32 out_height;
};

struct isp_dev_init_in {
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	cmr_handle mem_cb_handle;
	struct isp_init_param init_param;
};

struct dev_time {
	cmr_u32 sec;
	cmr_u32 usec;
};
union isp_dev_ctrl_cmd_in {
	cmr_u32 value;
};

union isp_dev_ctrl_cmd_out {
	cmr_u32 value;
	struct dev_time time;
};

struct isp_dev_access_start_in {
	struct isp_video_start common_in;
	struct isp_3a_cfg_param hw_cfg;
	struct isp_3a_dld_sequence dld_seq;
};

cmr_int isp_dev_access_init(struct isp_dev_init_in *input_ptr, cmr_handle *isp_dev_handle);
cmr_int isp_dev_access_deinit(cmr_handle isp_dev_handle);
cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, enum isp_dev_access_ctrl_cmd cmd, union isp_dev_ctrl_cmd_in *input_ptr, union isp_dev_ctrl_cmd_out *output_ptr);
cmr_int isp_dev_access_capability(cmr_handle isp_dev_handle, enum isp_capbility_cmd cmd, void* param_ptr);
cmr_int isp_dev_access_start_multiframe(cmr_handle isp_dev_handle, struct isp_dev_access_start_in* param_ptr);
cmr_int isp_dev_access_stop_multiframe(cmr_handle isp_dev_handle);
cmr_int isp_dev_access_start_postproc(cmr_handle isp_dev_handle, struct isp_dev_postproc_in* input_ptr, struct isp_dev_postproc_out* output_ptr);
void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata);
cmr_int isp_dev_access_cfg_awb_param(cmr_handle isp_dev_handle, struct isp3a_awb_hw_cfg *data);
cmr_int isp_dev_access_cfg_awb_gain(cmr_handle isp_dev_handle, struct isp_awb_gain *data);
cmr_int isp_dev_access_set_stats_buf(cmr_handle isp_dev_handle, struct isp_statis_buf *buf);
cmr_int isp_dev_access_cfg_af_param(cmr_handle isp_dev_handle, struct isp3a_af_hw_cfg *data);
cmr_int isp_dev_access_cfg_iso_speed(cmr_handle isp_dev_handle, cmr_u32 *data);
/**---------------------------------------------------------------------------*/

#endif

