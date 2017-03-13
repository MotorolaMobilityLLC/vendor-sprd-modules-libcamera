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
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "isp_com.h"
#include "ae_ctrl_types.h"
#include "isp_pm.h"
#include "isp_drv.h"
#include "lib_ctrl.h"
#include "isp_adpt.h"
#include "sensor_drv_u.h"
#include "awb_ctrl.h"
#include "isp_otp_calibration.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

struct ae_init_in {
	uint32_t param_num;
	struct ae_param param[AE_MAX_PARAM_NUM];
	struct ae_size monitor_win_num;
	struct ae_isp_ctrl_ops isp_ops;
	struct ae_resolution_info resolution_info;
	struct third_lib_info lib_param;
	uint32_t camera_id;
	cmr_handle caller_handle;
	isp_ae_cb ae_set_cb;
	uint32_t has_force_bypass;
	struct ae_opt_info otp_info;
	void *lsc_otp_random;
	void *lsc_otp_golden;
	uint32_t lsc_otp_width;
	uint32_t lsc_otp_height;
};

struct ae_init_out {
	uint32_t cur_index;
	uint32_t cur_exposure;
	uint32_t cur_again;
	uint32_t cur_dgain;
	uint32_t cur_dummy;
};


struct ae_calc_in {
	uint32_t stat_fmt;	//enum ae_aem_fmt
	union {
		uint32_t *stat_img;
		uint32_t *rgb_stat_img;
	};
	uint32_t *yiq_stat_img;
	uint32_t awb_gain_r;
	uint32_t awb_gain_g;
	uint32_t awb_gain_b;
	struct ae_stat_img_info info;
	uint32_t sec;
	uint32_t usec;
	struct isp_sensor_fps_info sensor_fps;
};

struct ae_calc_out {
	uint32_t cur_lum;
	uint32_t cur_index;
	uint32_t cur_ev;
	uint32_t cur_exp_line;
	uint32_t cur_dummy;
	uint32_t cur_again;
	uint32_t cur_dgain;
	uint32_t cur_iso;
	uint32_t is_stab;
	uint32_t line_time;
	uint32_t frame_line;
	uint32_t target_lum;
//dy
	uint32_t flag;
	float *ae_data;
	int32_t ae_data_size;

	struct tg_ae_ctrl_alc_log log_ae;
};

struct ae_ctrl_param_out {
	union{
		uint32_t real_iso;
		uint32_t ae_effect;
		uint32_t ae_state;
		uint32_t flash_eb;
		uint32_t lum;
		uint32_t mode;
		int32_t bv_gain;
		int32_t bv_lum;
		float gain;
		float expoture;
		struct ae_calc_out ae_result;
		struct ae_get_ev ae_ev;
		struct ae_monitor_info info;
	};
};

int32_t ae_ctrl_init(struct ae_init_in *input_ptr, cmr_handle *handle_ae);
cmr_int ae_ctrl_deinit(cmr_handle *handle_ae);
cmr_int ae_ctrl_ioctrl(cmr_handle handle, enum ae_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr);
cmr_int ae_ctrl_process(cmr_handle handle, struct ae_calc_in *in_param, struct ae_calc_out *result);
int32_t _isp_get_flash_cali_param(isp_pm_handle_t pm_handle, struct isp_flash_param **out_param_ptr);
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

