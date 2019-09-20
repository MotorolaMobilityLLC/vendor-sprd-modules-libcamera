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
#ifndef _ISP_BRIDGE_H_
#define _ISP_BRIDGE_H_

#include "isp_type.h"
#include "cmr_sensor_info.h"
#include "isp_com.h"
#include "af_ctrl.h"

typedef cmr_int (*func_isp_br_ioctrl)(cmr_u32 sensor_role, cmr_int cmd, void *in, void *out);

enum isp_br_ioctl_cmd {
	// AE
	SET_MATCH_AE_DATA = 0x00,
	GET_MATCH_AE_DATA,
	SET_AEM_SYNC_STAT,
	GET_AEM_SYNC_STAT,
	SET_AEM_STAT_BLK_NUM,
	SET_MATCH_BV_DATA,
	GET_MATCH_BV_DATA,
	SET_SLAVE_AEM_INFO,
	GET_SLAVE_AEM_INFO,
	GET_STAT_AWB_DATA_AE,
	SET_AE_TARGET_REGION,
	SET_AE_REF_CAMERA_ID,
	GET_AE_SYNC_DATA,

	// AWB
	SET_MATCH_AWB_DATA,
	GET_MATCH_AWB_DATA,
	SET_STAT_AWB_DATA,
	GET_STAT_AWB_DATA,
	SET_GAIN_AWB_DATA,
	GET_GAIN_AWB_DATA,
	SET_FOV_DATA,
	GET_FOV_DATA,

	// OTP
	SET_OTP_AE,
	GET_OTP_AE,
	SET_OTP_AWB,
	GET_OTP_AWB,

	SET_MODULE_INFO,
	GET_MODULE_INFO,

	GET_CAMERA_ID,
	GET_SENSOR_ROLE,/* dont care first argument */
	GET_ISPALG_FW_BY_ID,/* dont care first argument */
	SET_SLAVE_SENSOR_MODE,
	GET_SLAVE_SENSOR_MODE,

	SET_ALL_MODULE_AND_OTP,
	GET_ALL_MODULE_AND_OTP,

	//control
	SET_USER_COUNT,
	GET_USER_COUNT,
};

struct awb_gain_data {
	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
};

struct awb_match_data {
	cmr_u32 ct;
};

struct ae_otp_param {
	struct sensor_otp_ae_info otp_info;
};

struct awb_otp_param {
	struct sensor_otp_awb_info awb_otp_info;
};

struct sensor_info {
	cmr_s16 min_exp_line;
	cmr_s16 max_again;
	cmr_s16 min_again;
	cmr_s16 sensor_gain_precision;
	cmr_u32 line_time;
	cmr_u32 frm_len_def;
};

struct aem_info {
	cmr_u32 aem_stat_blk_pixels;
	cmr_u32 aem_stat_win_w;
	cmr_u32 aem_stat_win_h;
};

struct module_sensor_info {
	struct sensor_info sensor_info[CAM_SENSOR_MAX];
};

struct module_otp_info {
	struct ae_otp_param ae_otp[CAM_SENSOR_MAX];
	struct awb_otp_param awb_otp[CAM_SENSOR_MAX];
};

struct module_info {
	struct module_sensor_info module_sensor_info;
	struct module_otp_info module_otp_info;
};

struct ae_match_data {
	cmr_u32 frame_len;
	cmr_u32 frame_len_def;
	cmr_u32 gain;
	cmr_u32 isp_gain;
	struct sensor_ex_exposure exp;
};

struct ae_match_stats_data {
	cmr_u32 *stats_data;
	cmr_u32 len;
	cmr_s64 monoboottime;
	cmr_u32 is_last_frm;
};

struct awb_match_stats_data {
	cmr_u32 *stats_data;
};

struct fov_data {
	float physical_size[2];
	float focal_lengths;
};

struct ae_target_region {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t width;
	uint32_t height;
};

struct ae_sync_data {
	cmr_u32 num;
	cmr_u32 ref_camera_id;
	struct ae_rect target_rect;
};

cmr_int isp_br_init(cmr_u32 camera_id, cmr_handle isp_3a_handle, cmr_u32 is_master);
cmr_int isp_br_deinit(cmr_u32 camera_id);
cmr_int isp_br_ioctrl(cmr_u32 sensor_role, cmr_int cmd, void *in, void *out);
#endif
