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
#ifndef _ISP_MATCH_H_
#define _ISP_MATCH_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/

#include "isp_type.h"
#include "cmr_sensor_info.h"
#include "ae_ctrl.h"
#include "ae_com.h"

#ifdef __cplusplus
extern "C" {
#endif

enum isp_br_ctrl_cmd_type {
	GET_MATCH_AWB_DATA,
	SET_MATCH_AWB_DATA,

	//module info(vcm/f.no/view angle/efl)
	SET_SLAVE_MODULE_INFO,
	GET_SLAVE_MODULE_INFO,
	SET_MASTER_MODULE_INFO,
	GET_MASTER_MODULE_INFO,
	//turnning info

	//otp match
	SET_SLAVE_OTP_AE,  //slave ae otp from sensor;
	GET_SLAVE_OTP_AE,  //slave ae otp from sensor;

	SET_MASTER_OTP_AE,  //slave ae otp from sensor;
	GET_MASTER_OTP_AE,  //slave ae otp from sensor;

	//ae match
	SET_SLAVE_AESYNC_SETTING,  //slave ae setting   obtained by the master   //10
	GET_SLAVE_AESYNC_SETTING,  //slave ae setting from master

	SET_SLAVE_AECALC_RESULT, //slave ae calc result
	GET_SLAVE_AECALC_RESULT,//slave ae output from master

	SET_MASTER_AECALC_RESULT, //master ae calc result                      //14
	GET_MASTER_AECALC_RESULT,  //master ae output from master

	SET_MASTER_AESYNC_SETTING,  //master ae setting   obtained by the slave
	GET_MASTER_AESYNC_SETTING, //master ae setting from slave
};

struct awb_match_data {
	uint32_t ct;
	uint32_t ct_flash_off;
	uint32_t ct_capture;
	uint32_t is_update;
	uint32_t awb_states;
	uint16_t light_source;
	uint32_t awb_decision;
};

struct ae_otp_param {
	struct sensor_otp_ae_info otp_info;
};

struct ae_sync_out {
	uint32_t updata_flag;
	uint32_t exposure_time;
	uint32_t exposure_line;
	uint32_t sensor_ad_gain;
	uint32_t exposure_dummy;
	uint32_t isp_d_gain;
};

struct sensor_info {
	cmr_s16 min_exp_line;
	cmr_s16 max_again;
	cmr_s16 min_again;
	cmr_s16 sensor_gain_precision;
	cmr_s16 line_time;
};

struct module_sensor_info {
	struct sensor_info slave_sensor_info;
	struct sensor_info master_sensor_info;
};

struct module_otp_info{
	struct ae_otp_param master_ae_otp;
	struct ae_otp_param slave_ae_otp;
};

struct module_info {
	struct module_sensor_info module_sensor_info;
	struct module_otp_info module_otp_info;
};

struct ae_match_data {
	struct ae_alg_calc_result ae_calc_result;
	struct ae_sync_out ae_sync_result;
};

struct match_data_param {
	struct module_info module_info;
	struct ae_match_data master_ae_info;
	struct ae_match_data slave_ae_info;
	struct awb_match_data master_awb_info;
	struct awb_match_data slave_awb_info;
};

cmr_handle isp_br_get_3a_handle(uint8_t is_master);
int32_t isp_br_init(uint8_t is_master, void* isp_3a_handle);
int32_t isp_br_deinit(uint8_t is_master);
int32_t isp_br_ioctrl(uint32_t is_master, int32_t cmd, void *in, void *out);
int32_t isp_br_save_dual_otp(uint32_t camera_id, struct sensor_dual_otp_info *dual_otp);
int32_t isp_br_get_dual_otp(uint32_t camera_id, struct sensor_dual_otp_info **dual_otp);

#ifdef __cplusplus
}
#endif

#endif
