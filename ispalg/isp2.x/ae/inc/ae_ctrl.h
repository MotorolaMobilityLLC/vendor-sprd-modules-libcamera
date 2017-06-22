/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_CTRL_H_
#define _AE_CTRL_H_

#include "isp_common_types.h"
#include "isp_pm.h"
#include "isp_adpt.h"
#include "sensor_drv_u.h"
#include "isp_otp_calibration.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_CAMERA_DUAL_SYNC

/* all set ioctl should be even number, while get should be odd */
enum isp_br_ioctl_cmd {
	SET_MATCH_AWB_DATA = 0,
	GET_MATCH_AWB_DATA = 1,

	/* module info (vcm/frame number/view angle/efl) */
	SET_SLAVE_MODULE_INFO = 2,
	GET_SLAVE_MODULE_INFO = 3,
	SET_MASTER_MODULE_INFO = 4,
	GET_MASTER_MODULE_INFO = 5,

	/* sensor otp info */
	SET_SLAVE_OTP_AE = 6,
	GET_SLAVE_OTP_AE = 7,
	SET_MASTER_OTP_AE = 8,
	GET_MASTER_OTP_AE = 9,

	/* slave ae match */
	SET_SLAVE_AESYNC_SETTING = 10,
	GET_SLAVE_AESYNC_SETTING = 11,
	SET_SLAVE_AECALC_RESULT = 12,
	GET_SLAVE_AECALC_RESULT = 13,

	/* master ae match */
	SET_MASTER_AESYNC_SETTING = 14,
	GET_MASTER_AESYNC_SETTING = 15,
	SET_MASTER_AECALC_RESULT = 16,
	GET_MASTER_AECALC_RESULT = 17,

	/* all module and otp */
	SET_ALL_MODULE_AND_OTP = 18,
	GET_ALL_MODULE_AND_OTP = 19,

	// TODO: turnning info
};

typedef int32_t(*func_isp_br_ioctrl) (uint32_t is_master, enum isp_br_ioctl_cmd cmd, void *in, void *out);

#endif

struct ae_init_in {
	cmr_u32 param_num;
	cmr_u32 dflash_num;
	struct ae_param param[AE_MAX_PARAM_NUM];
	struct ae_param flash_tuning[AE_MAX_PARAM_NUM];
	struct ae_size monitor_win_num;
	struct ae_isp_ctrl_ops isp_ops;
	struct ae_resolution_info resolution_info;
	struct third_lib_info lib_param;
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	isp_ae_cb ae_set_cb;
	cmr_u32 has_force_bypass;
	struct ae_opt_info otp_info;
	cmr_handle lsc_otp_random;
	cmr_handle lsc_otp_golden;
	cmr_u32 lsc_otp_width;
	cmr_u32 lsc_otp_height;
	struct ae_ct_table ct_table;
#ifdef CONFIG_CAMERA_DUAL_SYNC
	cmr_u8 ae_role;  //1:master 0: slave
	cmr_u8 sensor_role;
	cmr_u32 is_multi_mode;
	func_isp_br_ioctrl ptr_isp_br_ioctrl;
#endif
};

struct ae_init_out {
	cmr_u32 cur_index;
	cmr_u32 cur_exposure;
	cmr_u32 cur_again;
	cmr_u32 cur_dgain;
	cmr_u32 cur_dummy;
	cmr_u32 flash_ver;
};

struct ae_calc_in {
	cmr_u32 stat_fmt;
	union {
		cmr_u32 *stat_img;
		cmr_u32 *rgb_stat_img;
	};
	cmr_u32 *yiq_stat_img;
	cmr_u32 awb_gain_r;
	cmr_u32 awb_gain_g;
	cmr_u32 awb_gain_b;
	struct ae_stat_img_info info;
	cmr_u32 sec;
	cmr_u32 usec;
	struct isp_sensor_fps_info sensor_fps;
#ifdef CONFIG_CAMERA_DUAL_SYNC
	cmr_s64 monoboottime;
#endif
};

struct ae_ctrl_param_out {
	union {
		cmr_u32 real_iso;
		cmr_u32 ae_effect;
		cmr_u32 ae_state;
		cmr_u32 flash_eb;
		cmr_u32 lum;
		cmr_u32 mode;
		cmr_s32 bv_gain;
		cmr_s32 bv_lum;
		float gain;
		float expoture;
		struct ae_calc_out ae_result;
		struct ae_get_ev ae_ev;
		struct ae_monitor_info info;
	};
};

cmr_s32 ae_ctrl_init(struct ae_init_in *input_ptr, cmr_handle * handle_ae, cmr_handle result);
cmr_int ae_ctrl_deinit(cmr_handle * handle_ae);
cmr_int ae_ctrl_ioctrl(cmr_handle handle, enum ae_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr);
cmr_int ae_ctrl_process(cmr_handle handle, struct ae_calc_in *in_param, struct ae_calc_out *result);
cmr_s32 _isp_get_flash_cali_param(cmr_handle pm_handle, struct isp_flash_param **out_param_ptr);

#ifdef __cplusplus
}
#endif

#endif
