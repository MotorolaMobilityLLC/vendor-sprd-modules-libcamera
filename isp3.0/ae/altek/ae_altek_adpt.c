/*
 * Copyright (C) 2015 The Android Open Source Project
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
#define LOG_TAG "alk_adpt_ae"

#include <dlfcn.h>
#include "ae_altek_adpt.h"
#include "al3ALib_AE.h"
#include "al3ALib_AE_ErrCode.h"
#include "al3AWrapper_AE.h"
#include "al3AWrapper_AEErrCode.h"
#include "sensor_exposure_queue.h"



#define AELIB_PATH "libalAELib.so"

#define SENSOR_GAIN_BASE       128
#define LIB_GAIN_BASE          100
#define F_NUM_BASE             100
#define MAX_EXP_LINE_CNT       65535
#define BRACKET_NUM            5
#define SENSOR_EXP_US_BASE     10000000 /*x10us*/

enum aealtek_work_mode{
	AEALTEK_PREVIEW,
	AEALTEK_BRACKET,
	AEALTEK_SNAPSHOT,
	AEALTEK_MAX
};

struct aealtek_lib_ops {
	BOOL (*load_func)(alAERuntimeObj_t *aec_run_obj, UINT32 identityID);
	void (*get_lib_ver)(alAElib_Version_t* AE_LibVersion);
};

struct aealtek_exposure_param {
	cmr_u32 exp_line;
	cmr_u32 exp_time;
	cmr_u32 dummy;
	cmr_u32 gain;
	cmr_u32 size_index;
	cmr_u32 iso;
	cmr_u32 isp_d_gain;
};

struct aealtek_sensor_exp_data {
	struct aealtek_exposure_param lib_exp;
	struct aealtek_exposure_param actual_exp;
	struct aealtek_exposure_param write_exp;
};

struct aealtek_ui_param {
	struct ae_ctrl_param_work work_info;
	enum ae_ctrl_scene_mode scene;
	enum ae_ctrl_flicker_mode flicker;
	enum ae_ctrl_iso_mode iso;
	cmr_s32 ui_ev_level;
	enum ae_ctrl_measure_lum_mode weight;
	struct ae_ctrl_param_range_fps fps;
	enum ae_ctrl_hdr_ev_level  hdr_level;
};

struct aealtek_lib_ui_param {
	struct ae_ctrl_param_work work_info;
	ae_scene_mode_t scene;
	ae_iso_mode_t iso;
	cmr_s32 ui_ev_level;
	ae_metering_mode_type_t weight;
	struct ae_ctrl_param_range_fps fps;
	ae_antiflicker_mode_t flicker;
};

struct aealtek_status {
	struct aealtek_ui_param ui_param;
	struct aealtek_lib_ui_param lib_ui_param;
	cmr_s32 is_quick_mode;
	cmr_s32 is_hdr_status;
	cmr_u32 min_frame_length;
};

enum aealtek_flash_state {
	AEALTEK_FLASH_STATE_PREPARE_ON,
	AEALTEK_FLASH_STATE_LIGHTING,
	AEALTEK_FLASH_STATE_CLOSE,
	AEALTEK_FLASH_STATE_MAX,
};

struct aealtek_lib_exposure_data {
	/*bracket*/
	cmr_u32 valid_exp_num;
	struct aealtek_exposure_param bracket_exp[BRACKET_NUM];

	/*init*/
	struct aealtek_exposure_param init;

	/*preview*/
	struct aealtek_exposure_param preview;

	/*snapshot*/
	struct aealtek_exposure_param  snapshot;
};

struct aealtek_led_cfg {
	cmr_s32 idx;
	cmr_s32 current;
	cmr_u32 time_ms;
};

struct aealtek_flash_cfg {
	cmr_s32 led_num;
	struct aealtek_led_cfg led_0;
	struct aealtek_led_cfg led_1;
	struct aealtek_exposure_param exp_cell;
};

struct aealtek_flash_param {
	cmr_s32 enable;
	enum aealtek_flash_state flash_state;
	struct aealtek_flash_cfg pre_flash_before;
	struct aealtek_flash_cfg main_flash_est;
};

struct aealtek_touch_param {
	cmr_u32 touch_flag;
	cmr_u32 ctrl_roi_changed_flag;
	cmr_u32 ctrl_roi_converge_flag;
	cmr_u32 ctrl_convergence_req_flag;
};

struct aealtek_update_list {
	cmr_s32 is_iso:1;
	cmr_s32 is_target_lum:1;
	cmr_s32 is_quick:1;
	cmr_s32 is_scene:1;
	cmr_s32 is_weight:1;
	cmr_s32 is_touch_zone:1;
	cmr_s32 is_ev:1;
	cmr_s32 is_fps:1;
	cmr_s32 is_flicker:1;
};

struct aealtek_lib_data {
	cmr_s32 is_called_hwisp_cfg; //called default hw isp config
	alHW3a_AE_CfgInfo_t hwisp_cfg;
	ae_output_data_t output_data;
	al3AWrapper_Stats_AE_t stats_data;
	ae_output_data_t temp_output_data;
	calibration_data_t ae_otp_data;
	struct aealtek_lib_exposure_data exposure_array;
};

/*ae altek context*/
struct aealtek_cxt {
	cmr_u32 is_inited;
	cmr_u32 work_cnt; //work number by called
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	cmr_handle lib_handle;
	struct ae_ctrl_init_in init_in_param;
	alAERuntimeObj_t al_obj;
	struct aealtek_lib_ops lib_ops;

	void *seq_handle;

	cmr_s32 lock_cnt;
	cmr_s32 ae_state;

	struct aealtek_status prv_status;
	struct aealtek_status cur_status;
	struct aealtek_status nxt_status;
	struct aealtek_update_list update_list;

	struct aealtek_flash_param flash_param;
	struct aealtek_touch_param touch_param;

	struct aealtek_lib_data lib_data;

	struct ae_ctrl_proc_in proc_in;
	struct aealtek_sensor_exp_data sensor_exp_data;
	struct aealtek_exposure_param pre_write_exp_data;
};


static cmr_int aealtek_reset_touch_ack(struct aealtek_cxt *cxt_ptr);
static cmr_int aealtek_set_lock(struct aealtek_cxt *cxt_ptr, cmr_int is_lock);
static cmr_int aealtek_set_flash_est(struct aealtek_cxt *cxt_ptr, cmr_u32 is_reset);
/********function start******/
static void aealtek_print_lib_log(struct aealtek_cxt *cxt_ptr, ae_output_data_t *in_ptr)
{
	#define report_str "report data:"
	#define output_str "output data:"


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
#if 0
	ISP_LOGI(output_str"hw3a_frame_id:%d", in_ptr->hw3a_frame_id);
	ISP_LOGI(output_str"sys_sof_index:%d", in_ptr->sys_sof_index);
	ISP_LOGI(output_str"hw3a_ae_block_nums:%d", in_ptr->hw3a_ae_block_nums);
	ISP_LOGI(output_str"hw3a_ae_block_totalpixels:%d", in_ptr->hw3a_ae_block_totalpixels);

	ISP_LOGI(output_str"ae_targetmean:%d", in_ptr->ae_targetmean);
	ISP_LOGI(output_str"ae_cur_mean:%d", in_ptr->ae_cur_mean);
	ISP_LOGI(output_str"ae_nonwt_mean:%d", in_ptr->ae_nonwt_mean);
	ISP_LOGI(output_str"ae_center_mean2x2:%d", in_ptr->ae_center_mean2x2);
	ISP_LOGI(output_str"BVresult:%d", in_ptr->BVresult);
	ISP_LOGI(output_str"BG_BVResult:%d", in_ptr->BG_BVResult);

	ISP_LOGI(output_str"ae_converged:%d", in_ptr->ae_converged);
	ISP_LOGI(output_str"ae_need_flash_flg:%d", in_ptr->ae_need_flash_flg);
	ISP_LOGI(output_str"ae_est_status:%d", in_ptr->ae_est_status);
	ISP_LOGI(output_str"fe_est_status:%d", in_ptr->fe_est_status);

	ISP_LOGI(output_str"udCur_ISO:%d", in_ptr->udCur_ISO);
	ISP_LOGI(output_str"udCur_exposure_time:%d", in_ptr->udCur_exposure_time);
	ISP_LOGI(output_str"udCur_exposure_line:%d", in_ptr->udCur_exposure_line);
	ISP_LOGI(output_str"udCur_sensor_ad_gain:%d", in_ptr->udCur_sensor_ad_gain);
	ISP_LOGI(output_str"udCur_ISP_D_gain:%d", in_ptr->udCur_ISP_D_gain);
	ISP_LOGI(output_str"uwCur_fps:%d", in_ptr->uwCur_fps);

	ISP_LOGI(output_str"udISO:%d", in_ptr->udISO);
	ISP_LOGI(output_str"udexposure_time:%d", in_ptr->udexposure_time);
	ISP_LOGI(output_str"udexposure_line:%d", in_ptr->udexposure_line);
	ISP_LOGI(output_str"udsensor_ad_gain:%d", in_ptr->udsensor_ad_gain);
	ISP_LOGI(output_str"udISP_D_gain:%d", in_ptr->udISP_D_gain);

	ISP_LOGI(output_str"ae_roi_change_st:%d", in_ptr->ae_roi_change_st);
	ISP_LOGI(output_str"flicker_mode:%d", in_ptr->flicker_mode);
	ISP_LOGI(output_str"ae_metering_mode:%d", in_ptr->ae_metering_mode);
	ISP_LOGI(output_str"bv_delta:%d", in_ptr->bv_delta);
	ISP_LOGI(output_str"ae_non_comp_bv_val:%d", in_ptr->ae_non_comp_bv_val);
	ISP_LOGI(output_str"udADGainx100PerISO100Gain:%d", in_ptr->udADGainx100PerISO100Gain);
#endif

#if 0
	ISP_LOGI(report_str"hw3a_frame_id:%d", in_ptr->rpt_3a_update.ae_update.hw3a_frame_id);
	ISP_LOGI(report_str"sys_sof_index:%d", in_ptr->rpt_3a_update.ae_update.sys_sof_index);
	ISP_LOGI(report_str"hw3a_ae_block_nums:%d", in_ptr->rpt_3a_update.ae_update.hw3a_ae_block_nums);
	ISP_LOGI(report_str"hw3a_ae_block_totalpixels:%d", in_ptr->rpt_3a_update.ae_update.hw3a_ae_block_totalpixels);

	ISP_LOGI(report_str"targetmean:%d", in_ptr->rpt_3a_update.ae_update.targetmean);
	ISP_LOGI(report_str"curmean:%d", in_ptr->rpt_3a_update.ae_update.curmean);
	ISP_LOGI(report_str"avgmean:%d", in_ptr->rpt_3a_update.ae_update.avgmean);
	ISP_LOGI(report_str"center_mean2x2:%d", in_ptr->rpt_3a_update.ae_update.center_mean2x2);
	ISP_LOGI(report_str"bv_val:%d", in_ptr->rpt_3a_update.ae_update.bv_val);
	ISP_LOGI(report_str"BG_BVResult:%d", in_ptr->rpt_3a_update.ae_update.BG_BVResult);

	ISP_LOGI(report_str"ae_converged:%d", in_ptr->rpt_3a_update.ae_update.ae_converged);
	ISP_LOGI(report_str"ae_need_flash_flg:%d", in_ptr->rpt_3a_update.ae_update.ae_need_flash_flg);
	ISP_LOGI(report_str"ae_LibStates:%d", in_ptr->rpt_3a_update.ae_update.ae_LibStates);
	ISP_LOGI(report_str"ae_FlashStates:%d", in_ptr->rpt_3a_update.ae_update.ae_FlashStates);

	ISP_LOGI(report_str"sensor_ad_gain:%d", in_ptr->rpt_3a_update.ae_update.sensor_ad_gain);
	ISP_LOGI(report_str"isp_d_gain:%d", in_ptr->rpt_3a_update.ae_update.isp_d_gain);
	ISP_LOGI(report_str"exposure_time:%d", in_ptr->rpt_3a_update.ae_update.exposure_time);
	ISP_LOGI(report_str"exposure_line:%d", in_ptr->rpt_3a_update.ae_update.exposure_line);
	ISP_LOGI(report_str"exposure_max_line:%d", in_ptr->rpt_3a_update.ae_update.exposure_max_line);
	ISP_LOGI(report_str"ae_cur_iso:%d", in_ptr->rpt_3a_update.ae_update.ae_cur_iso);

	ISP_LOGI(report_str"ae_roi_change_st:%d", in_ptr->rpt_3a_update.ae_update.ae_roi_change_st);
	ISP_LOGI(report_str"flicker_mode:%d", in_ptr->rpt_3a_update.ae_update.flicker_mode);
	ISP_LOGI(report_str"ae_metering_mode:%d", in_ptr->rpt_3a_update.ae_update.ae_metering_mode);
	ISP_LOGI(report_str"bv_delta:%d", in_ptr->rpt_3a_update.ae_update.bv_delta);
	ISP_LOGI(report_str"cur_fps:%d", in_ptr->rpt_3a_update.ae_update.cur_fps);
	ISP_LOGI(report_str"ae_non_comp_bv_val:%d", in_ptr->rpt_3a_update.ae_update.ae_non_comp_bv_val);
#endif
exit:
	return;
}

static cmr_int aealtek_convert_otp(struct aealtek_cxt *cxt_ptr, calib_wb_gain_t *otp_ptr)
{
	cmr_int ret = ISP_ERROR;
	calibration_data_t  *lib_otp_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	lib_otp_ptr = &cxt_ptr->lib_data.ae_otp_data;
	if (otp_ptr) {
		lib_otp_ptr->calib_r_gain = otp_ptr->r;
		lib_otp_ptr->calib_g_gain = otp_ptr->g;
		lib_otp_ptr->calib_b_gain = otp_ptr->b;
	} else {
		lib_otp_ptr->calib_r_gain = 1500;
		lib_otp_ptr->calib_g_gain = 1300;
		lib_otp_ptr->calib_b_gain = 1600;
		ISP_LOGE("NO OTP DATA !!!");
	}
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_convert_lib_exposure2outdata(struct aealtek_cxt *cxt_ptr, struct aealtek_exposure_param *from_ptr, ae_output_data_t *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;

	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->rpt_3a_update.ae_update.exposure_line = from_ptr->exp_line;
	to_ptr->rpt_3a_update.ae_update.exposure_time = from_ptr->exp_time;
	to_ptr->rpt_3a_update.ae_update.sensor_ad_gain = from_ptr->gain;
	to_ptr->rpt_3a_update.ae_update.ae_cur_iso = from_ptr->iso;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_lib_exposure2sensor(struct aealtek_cxt *cxt_ptr, ae_output_data_t *from_ptr, struct aealtek_exposure_param *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;

	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->exp_line = from_ptr->rpt_3a_update.ae_update.exposure_line;
	to_ptr->exp_time = from_ptr->rpt_3a_update.ae_update.exposure_time;
	to_ptr->gain = from_ptr->rpt_3a_update.ae_update.sensor_ad_gain;
	to_ptr->iso = from_ptr->rpt_3a_update.ae_update.ae_cur_iso;
	to_ptr->size_index = cxt_ptr->cur_status.ui_param.work_info.resolution.sensor_size_index;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_lib_report2out(ae_report_update_t *from_ptr, struct isp3a_ae_report *to_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p is NULL error!", from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->hw3a_frame_id = from_ptr->hw3a_frame_id;
//	to_ptr->block_num = from_ptr->hw3a_ae_block_num;
	to_ptr->block_totalpixels = from_ptr->hw3a_ae_block_totalpixels;
	to_ptr->bv_val = from_ptr->bv_val;
	to_ptr->y_stats = from_ptr->hw3a_ae_stats;
	to_ptr->sys_sof_index = from_ptr->sys_sof_index;
	to_ptr->bv_delta = from_ptr->bv_delta;

	to_ptr->ae_state = from_ptr->ae_LibStates;
	to_ptr->fe_state = from_ptr->ae_FlashStates;
	to_ptr->ae_converge_st = from_ptr->ae_converged;
	to_ptr->BV = from_ptr->bv_val;
	to_ptr->non_comp_BV = from_ptr->ae_non_comp_bv_val;
	to_ptr->ISO = from_ptr->ae_cur_iso;
	to_ptr->cur_mean = from_ptr->curmean;
	to_ptr->target_mean = from_ptr->targetmean;
	to_ptr->sensor_ad_gain = from_ptr->sensor_ad_gain;
	to_ptr->exp_line = from_ptr->exposure_line;
	to_ptr->exp_time = from_ptr->exposure_time;
	to_ptr->fps = from_ptr->cur_fps;

#if 0
	//	to_ptr->flash_ratio = from_ptr->flash_ratio;
	//	to_ptr->flash_gain_r = from_ptr->flash_gain.r_gain;
	//	to_ptr->flash_gain_g = from_ptr->flash_gain.g_gain;
	//	to_ptr->flash_gain_b = from_ptr->flash_gain.b_gain;
	//	to_ptr->flash_gain_r_led2 = from_ptr->flash_gain_led2.r_gain;
	//	to_ptr->flash_gain_g_led2 = from_ptr->flash_gain_led2.g_gain;
	//	to_ptr->flash_gain_b_led2 = from_ptr->flash_gain_led2.b_gain;
	//	to_ptr->flash_ratio_led2 = from_ptr->flash_ratio_led2;
	//	to_ptr->flash_ct_led1 = from_ptr->flash_c;
	//	to_ptr->flash_ct_led2;
#endif

	to_ptr->debug_data_size = from_ptr->ae_debug_valid_size;
	to_ptr->debug_data = from_ptr->ae_debug_data;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_lib_to_out_report(ae_output_data_t *from_ptr, struct isp3a_ae_report *to_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p is NULL error!", from_ptr, to_ptr);
		goto exit;
	}

	ret = aealtek_lib_report2out(&from_ptr->rpt_3a_update.ae_update, to_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_lib_to_out_info(struct aealtek_cxt *cxt_ptr, ae_output_data_t *from_ptr, struct isp3a_ae_info *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}

	if (cxt_ptr->lock_cnt)
		to_ptr->ae_ctrl_locked = 1;
	else
		to_ptr->ae_ctrl_locked = 0;

	to_ptr->ae_ctrl_converged_flag = from_ptr->rpt_3a_update.ae_update.ae_converged;
	to_ptr->ae_ctrl_state = cxt_ptr->ae_state;

	ISP_LOGI("ae_ctrl_locked=%d, is_converg=%d, ae_state=%d",
			to_ptr->ae_ctrl_locked,
			to_ptr->ae_ctrl_converged_flag,
			to_ptr->ae_ctrl_state);

	ret = aealtek_lib_to_out_report(from_ptr, &to_ptr->report_data);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_unload_lib(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	if (cxt_ptr->lib_handle) {
		dlclose(cxt_ptr->lib_handle);
		cxt_ptr->lib_handle = NULL;
	}
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_get_default_param(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_work *work_ptr, struct aealtek_status *st_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !st_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, st_ptr);
		goto exit;
	}
	st_ptr->ui_param.work_info = *work_ptr;

	st_ptr->is_quick_mode = 0;
	if (0 == st_ptr->ui_param.work_info.resolution.frame_size.w
		|| 0 == st_ptr->ui_param.work_info.resolution.frame_size.h) {
		st_ptr->ui_param.work_info.work_mode = ISP3A_WORK_MODE_PREVIEW;
		st_ptr->ui_param.work_info.capture_mode = ISP_CAP_MODE_AUTO;
		st_ptr->ui_param.work_info.resolution.frame_size.w = 1600;
		st_ptr->ui_param.work_info.resolution.frame_size.h = 1200;
		st_ptr->ui_param.work_info.resolution.frame_line = 1600;
		st_ptr->ui_param.work_info.resolution.line_time = 100; //dummy
		st_ptr->ui_param.work_info.resolution.sensor_size_index = 1;
		st_ptr->ui_param.work_info.resolution.max_fps = 30;
		st_ptr->ui_param.work_info.resolution.max_gain = 8;
	}
	st_ptr->ui_param.scene = AE_CTRL_SCENE_NORMAL;
	st_ptr->ui_param.flicker = AE_CTRL_FLICKER_50HZ;
	st_ptr->ui_param.iso = AE_CTRL_ISO_AUTO;
	st_ptr->ui_param.ui_ev_level = 3;
	st_ptr->ui_param.weight = AE_CTRL_MEASURE_LUM_CENTER;
	st_ptr->ui_param.fps.min_fps = 2;
	st_ptr->ui_param.fps.max_fps = 30;

	if (0 == cxt_ptr->init_in_param.sensor_static_info.f_num) {
		cxt_ptr->init_in_param.sensor_static_info.f_num = 280;
	}
	if (0 == cxt_ptr->init_in_param.sensor_static_info.max_gain) {
		cxt_ptr->init_in_param.sensor_static_info.max_gain = 8;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static void aealtek_flicker_ui2lib(enum ae_ctrl_flicker_mode from, ae_antiflicker_mode_t *to_ptr)
{
	ae_antiflicker_mode_t lib_flicker = 0;


	if (!to_ptr) {
		ISP_LOGE("param is NULL !!!");
		goto exit;
	}
	switch (from) {
	case AE_CTRL_FLICKER_50HZ:
		lib_flicker = ANTIFLICKER_50HZ;
		break;
	case AE_CTRL_FLICKER_60HZ:
		lib_flicker = ANTIFLICKER_60HZ;
		break;
	case AE_CTRL_FLICKER_OFF:
		lib_flicker = ANTIFLICKER_OFF;
		break;
	case AE_CTRL_FLICKER_AUTO:
		lib_flicker = ANTIFLICKER_50HZ;
		break;
	default:
		ISP_LOGW("not support flicker mode %d", from);
		break;
	}
	*to_ptr = lib_flicker;
exit:
	return;
}

static void aealtek_weight_ui2lib(enum ae_ctrl_measure_lum_mode from, ae_metering_mode_type_t *to_ptr)
{
	ae_metering_mode_type_t lib_metering = 0;


	if (!to_ptr) {
		ISP_LOGE("param is NULL !!!");
		goto exit;
	}
	switch (from) {
	case AE_CTRL_MEASURE_LUM_AVG:
		lib_metering = AE_METERING_AVERAGE;
		break;
	case AE_CTRL_MEASURE_LUM_CENTER:
		lib_metering = AE_METERING_CENTERWT;
		break;
	case AE_CTRL_MEASURE_LUM_SPOT:
		lib_metering = AE_METERING_SPOTWT;
		break;
	case AE_CTRL_MEASURE_LUM_TOUCH:
		lib_metering = AE_METERING_INTELLIWT;
		break;
	case AE_CTRL_MEASURE_LUM_USERDEF:
		lib_metering = AE_METERING_USERDEF_WT;
		break;
	default:
		lib_metering = AE_METERING_CENTERWT;
		ISP_LOGW("not support weight mode %d", from);
		break;
	}
	*to_ptr = lib_metering;
exit:
	return;
}

static cmr_int aealtek_sensor_info_ui2lib(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_resolution *from_ptr, ae_sensor_info_t *to_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_sensor_static_info *static_sensor_ptr = NULL;


	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	static_sensor_ptr = &cxt_ptr->init_in_param.sensor_static_info;

	/*	sensor info setting, RAW size, fps , etc. */
	to_ptr->min_fps = 100 * 1;
	to_ptr->max_fps = 100 * from_ptr->max_fps;
	to_ptr->min_line_cnt = 1;
	to_ptr->max_line_cnt = MAX_EXP_LINE_CNT;
	to_ptr->exposuretime_per_exp_line_ns = 1000 * from_ptr->line_time / 10; //base x10 us
	to_ptr->min_gain = 100 * 1;
	to_ptr->max_gain = 100 * from_ptr->max_gain;

	to_ptr->sensor_left = 0;
	to_ptr->sensor_top = 0;
	to_ptr->sensor_width = from_ptr->frame_size.w;
	to_ptr->sensor_height = from_ptr->frame_size.h;

	to_ptr->sensor_fullWidth = from_ptr->frame_size.w;
	to_ptr->sensor_fullHeight = from_ptr->frame_size.h;

	to_ptr->ois_supported = static_sensor_ptr->ois_supported;
	to_ptr->f_number = (float)(1.0 * static_sensor_ptr->f_num / F_NUM_BASE);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_convert_ui2initlib(struct aealtek_cxt *cxt_ptr, struct aealtek_status *from_ptr, ae_set_parameter_init_t *to_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_resolution *resolution_ptr = NULL;


	if (!cxt_ptr || !from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, from_ptr, to_ptr);
		goto exit;
	}
	cmr_bzero(to_ptr, sizeof(*to_ptr));
	resolution_ptr = &from_ptr->ui_param.work_info.resolution;

	/*  tuning parameter setting  */
	to_ptr->ae_setting_data = cxt_ptr->init_in_param.tuning_param;

	/* initial UI setting */
	aealtek_flicker_ui2lib(cxt_ptr->nxt_status.ui_param.flicker, &to_ptr->ae_set_antibaning_mode);
	aealtek_weight_ui2lib(cxt_ptr->nxt_status.ui_param.weight, &to_ptr->ae_metering_mode);

	/*	basic control param  */
	to_ptr->ae_enable = 1;
	to_ptr->ae_lock = 0;

	/*	sensor info setting, RAW size, fps , etc. */
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, resolution_ptr, &to_ptr->preview_sensor_info);
	if (ret)
		goto exit;

	to_ptr->capture_sensor_info = to_ptr->preview_sensor_info;

#if 0
	to_ptr->sensor_raw_w = resolution_ptr->frame_size.w;
	to_ptr->sensor_raw_h = resolution_ptr->frame_size.h;
#endif

	to_ptr->ae_calib_wb_gain = cxt_ptr->lib_data.ae_otp_data;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_load_otp(struct aealtek_cxt *cxt_ptr, void *otp_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	ret = aealtek_convert_otp(cxt_ptr, otp_ptr);
	if (ret)
		goto exit;

	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_OTP_WB_DAT;
	param_ct_ptr->ae_calib_wb_gain = cxt_ptr->lib_data.ae_otp_data;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret= obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);

	ISP_LOGE("r=%d,g=%d,b=%d", param_ct_ptr->ae_calib_wb_gain.calib_r_gain,
			param_ct_ptr->ae_calib_wb_gain.calib_g_gain,
			param_ct_ptr->ae_calib_wb_gain.calib_b_gain);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_load_lib(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int is_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	UINT32 identityID = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	cxt_ptr->lib_handle = dlopen(AELIB_PATH, RTLD_NOW);
	if (!cxt_ptr->lib_handle) {
		ISP_LOGE("failed to dlopen");
		ret = ISP_ERROR;
		goto exit;
	}

	cxt_ptr->lib_ops.load_func = dlsym(cxt_ptr->lib_handle, "alAElib_loadFunc");
	if (!cxt_ptr->lib_ops.load_func) {
		ISP_LOGE("failed to dlsym load func");
		ret = ISP_ERROR;
		goto error_dlsym;
	}
	cxt_ptr->lib_ops.get_lib_ver = dlsym(cxt_ptr->lib_handle, "alAELib_GetLibVersion");
	if (!cxt_ptr->lib_ops.get_lib_ver) {
		ISP_LOGE("failed to dlsym get lib ver");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	return ISP_SUCCESS;
error_dlsym:
	aealtek_unload_lib(cxt_ptr);
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_get_lib_ver(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;
	alAElib_Version_t lib_ver;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	cmr_bzero(&lib_ver, sizeof(lib_ver));
	if (cxt_ptr->lib_ops.get_lib_ver)
		cxt_ptr->lib_ops.get_lib_ver(&lib_ver);
	ISP_LOGE("LIB Ver %d.%d", lib_ver.major_version, lib_ver.minor_version);

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_init(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_init_in *in_ptr, struct ae_ctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int is_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	UINT32 identityID = 0;
	ae_set_parameter_init_t init_setting;
	struct seq_init_in seq_in;


	if (!cxt_ptr ||!in_ptr || !out_ptr) {
		ISP_LOGE("init param is null, input_ptr is %p, output_ptr is %p", in_ptr, out_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	if (!obj_ptr) {
		goto exit;
	}
	identityID = cxt_ptr->camera_id;
	is_ret = cxt_ptr->lib_ops.load_func(obj_ptr, identityID);
	if (!is_ret) {
		ret = ISP_ERROR;
		ISP_LOGE("load func is failed!");
		goto exit;
	}
	aealtek_get_lib_ver(cxt_ptr);

	if (obj_ptr->initial)
		obj_ptr->initial(&obj_ptr->ae);
	ISP_LOGE("lib runtime=%p", obj_ptr->ae);

	ret = aealtek_load_otp(cxt_ptr, in_ptr->otp_param);
	if (ret)
		goto exit;

	ret = aealtek_get_default_param(cxt_ptr, &in_ptr->preview_work, &cxt_ptr->cur_status);
	if (ret)
		goto exit;
	cxt_ptr->prv_status = cxt_ptr->cur_status;
	cxt_ptr->nxt_status = cxt_ptr->cur_status;

	seq_in.capture_skip_num = cxt_ptr->init_in_param.sensor_static_info.capture_skip_num;
	seq_in.preview_skip_num = cxt_ptr->init_in_param.sensor_static_info.capture_skip_num;
	seq_in.exp_valid_num = cxt_ptr->init_in_param.sensor_static_info.exposure_valid_num;
	seq_in.gain_valid_num = cxt_ptr->init_in_param.sensor_static_info.gain_valid_num;
	seq_in.idx_start_from = 1;

	ISP_LOGI("cap skip num=%d, preview skip num=%d, exp_valid_num=%d,gain_valid_num=%d",
			seq_in.capture_skip_num, seq_in.preview_skip_num,
			seq_in.exp_valid_num, seq_in.gain_valid_num);

	ret = seq_init(10, &seq_in, &cxt_ptr->seq_handle);
	if (ret || NULL == cxt_ptr->seq_handle)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static void aealtek_change_ae_state(struct aealtek_cxt *cxt_ptr, enum isp3a_ae_ctrl_state from, enum isp3a_ae_ctrl_state to)
{
	cxt_ptr->ae_state = to;
}

static void aealtek_change_flash_state(struct aealtek_cxt *cxt_ptr, enum aealtek_flash_state from, enum aealtek_flash_state to)
{
	cxt_ptr->flash_param.flash_state = to;
}

static cmr_int aealtek_get_def_hwisp_cfg( alHW3a_AE_CfgInfo_t *cfg_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cfg_ptr) {
		ISP_LOGE("param %p is NULL error!", cfg_ptr);
		goto exit;
	}
	cfg_ptr->TokenID = 0x01;
	cfg_ptr->tAERegion.uwBorderRatioX = 100;   // 100% use of current sensor cropped area
	cfg_ptr->tAERegion.uwBorderRatioY = 100;   // 100% use of current sensor cropped area
	cfg_ptr->tAERegion.uwBlkNumX = 16;         // fixed value for AE lib
	cfg_ptr->tAERegion.uwBlkNumY = 16;         // fixed value for AE lib
	cfg_ptr->tAERegion.uwOffsetRatioX = 0;     // 0% offset of left of current sensor cropped area
	cfg_ptr->tAERegion.uwOffsetRatioY = 0;     // 0% offset of top of current sensor cropped area

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_to_ae_hw_cfg(alHW3a_AE_CfgInfo_t *from_ptr, struct isp3a_ae_hw_cfg *to_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!from_ptr || !to_ptr) {
		ISP_LOGE("param %p %p is NULL error!", from_ptr, to_ptr);
		goto exit;
	}
	to_ptr->tokenID = from_ptr->TokenID;
	to_ptr->region.border_ratio_X = from_ptr->tAERegion.uwBorderRatioX;
	to_ptr->region.border_ratio_Y = from_ptr->tAERegion.uwBorderRatioY;
	to_ptr->region.blk_num_X = from_ptr->tAERegion.uwBlkNumX;
	to_ptr->region.blk_num_Y = from_ptr->tAERegion.uwBlkNumY;
	to_ptr->region.offset_ratio_X = from_ptr->tAERegion.uwOffsetRatioX;
	to_ptr->region.offset_ratio_Y = from_ptr->tAERegion.uwOffsetRatioY;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_get_hwisp_cfg(struct aealtek_cxt *cxt_ptr, struct isp3a_ae_hw_cfg *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int lib_ret = 0;

	alAERuntimeObj_t *obj_ptr = NULL;
	ae_get_param_t in_param;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	if (cxt_ptr->lib_data.is_called_hwisp_cfg) {
		in_param.ae_get_param_type = AE_GET_ALHW3A_CONFIG;

		if (obj_ptr && obj_ptr->get_param)
			lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
		if (lib_ret)
			goto exit;
		cxt_ptr->lib_data.hwisp_cfg = in_param.para.alHW3A_AEConfig;
	} else {
		cxt_ptr->lib_data.is_called_hwisp_cfg = 1;
		ret = aealtek_get_def_hwisp_cfg(&cxt_ptr->lib_data.hwisp_cfg);
		if (ret)
			goto exit;
	}

	ret = aealtek_to_ae_hw_cfg(&cxt_ptr->lib_data.hwisp_cfg, out_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_write_to_sensor(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_sensor_exposure *exp_ptr, struct ae_ctrl_param_sensor_gain *gain_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (! cxt_ptr || !exp_ptr || !gain_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, exp_ptr, gain_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	if (cxt_ptr->init_in_param.ops_in.set_exposure) {
		if (0 != exp_ptr->exp_line && (cxt_ptr->pre_write_exp_data.exp_line != exp_ptr->exp_line
				|| cxt_ptr->pre_write_exp_data.dummy != exp_ptr->dummy)) {
			(*cxt_ptr->init_in_param.ops_in.set_exposure)(cxt_ptr->caller_handle, exp_ptr);
		}
	}

	if (cxt_ptr->init_in_param.ops_in.set_again) {
		if (0 != gain_ptr->gain && cxt_ptr->pre_write_exp_data.gain != gain_ptr->gain) {
			(*cxt_ptr->init_in_param.ops_in.set_again)(cxt_ptr->caller_handle, gain_ptr);
		}
	}

	cxt_ptr->pre_write_exp_data.exp_line = exp_ptr->exp_line;
	cxt_ptr->pre_write_exp_data.dummy = exp_ptr->dummy;
	cxt_ptr->pre_write_exp_data.gain = gain_ptr->gain;


	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_pre_to_sensor(struct aealtek_cxt *cxt_ptr, cmr_int is_sync_call)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_param_sensor_exposure sensor_exp;
	struct ae_ctrl_param_sensor_gain sensor_gain;


	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	if (0 == is_sync_call) {
		sensor_exp.exp_line = cxt_ptr->sensor_exp_data.write_exp.exp_line;
		sensor_exp.dummy = cxt_ptr->sensor_exp_data.write_exp.dummy;
		sensor_exp.size_index = cxt_ptr->sensor_exp_data.write_exp.size_index;
		sensor_gain.gain = cxt_ptr->sensor_exp_data.write_exp.gain * SENSOR_GAIN_BASE / LIB_GAIN_BASE;
	} else {
		sensor_exp.exp_line = cxt_ptr->sensor_exp_data.lib_exp.exp_line;
		sensor_exp.dummy = cxt_ptr->sensor_exp_data.lib_exp.dummy;
		sensor_exp.size_index = cxt_ptr->sensor_exp_data.lib_exp.size_index;
		sensor_gain.gain = cxt_ptr->sensor_exp_data.lib_exp.gain * SENSOR_GAIN_BASE / LIB_GAIN_BASE;
	}
	ret = aealtek_write_to_sensor(cxt_ptr, &sensor_exp, &sensor_gain);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static void aealtek_reset_touch_param(struct aealtek_cxt *cxt_ptr)
{
	if (!cxt_ptr)
		return;

	cmr_bzero(&cxt_ptr->touch_param, sizeof(cxt_ptr->touch_param));
}

static cmr_int aealtek_set_boost(struct aealtek_cxt *cxt_ptr, cmr_u32 is_speed)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (is_speed)
		param_ct_ptr->converge_speedLV = AE_CONVERGE_FAST;
	else
		param_ct_ptr->converge_speedLV = AE_CONVERGE_NORMAL;;
	type = AE_SET_PARAM_CONVERGE_SPD;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_iso(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.iso = in_ptr->iso.iso_mode;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->ISOLevel = in_ptr->iso.iso_mode;
	type = AE_SET_PARAM_ISO_MODE;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	cxt_ptr->nxt_status.lib_ui_param.iso = param_ct_ptr->ISOLevel;
	cxt_ptr->update_list.is_iso = 1;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_exp_comp(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	cmr_s32 lib_ev_comp = 0;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.ui_ev_level = in_ptr->exp_comp.level;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	switch (in_ptr->exp_comp.level) {
	case AE_CTRL_ATTR_LEVEL_1:
		lib_ev_comp = -2000;
		break;
	case AE_CTRL_ATTR_LEVEL_2:
		lib_ev_comp = -1000;
		break;
	case AE_CTRL_ATTR_LEVEL_3:
		lib_ev_comp = 0;
		break;
	case AE_CTRL_ATTR_LEVEL_4:
		lib_ev_comp = 1000;
		break;
	case AE_CTRL_ATTR_LEVEL_5:
		lib_ev_comp = 2000;
		break;
	default:
		ISP_LOGW("UI level =%ld",in_ptr->exp_comp.level);
		break;
	}
	param_ct_ptr->ae_ui_evcomp = lib_ev_comp;
	type = AE_SET_PARAM_UI_EVCOMP;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	cxt_ptr->nxt_status.lib_ui_param.ui_ev_level = param_ct_ptr->ae_ui_evcomp;
	cxt_ptr->update_list.is_ev = 1;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_bypass(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	ISP_LOGI("eb=%d", in_ptr->value);

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->ae_enable = in_ptr->value;
	type = AE_SET_PARAM_ENABLE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_flicker(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	ae_antiflicker_mode_t lib_flicker_mode = 0;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.flicker = in_ptr->flicker.flicker_mode;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	aealtek_flicker_ui2lib(in_ptr->flicker.flicker_mode, &lib_flicker_mode);

	param_ct_ptr->ae_set_antibaning_mode = lib_flicker_mode;
	type = AE_SET_PARAM_ANTIFLICKERMODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	cxt_ptr->nxt_status.lib_ui_param.flicker = lib_flicker_mode;
	cxt_ptr->update_list.is_flicker = 1;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_scene_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	ae_scene_mode_t lib_scene_mode = 0;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.iso = in_ptr->scene.scene_mode;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	switch (in_ptr->scene.scene_mode) {
	case AE_CTRL_SCENE_NORMAL:
		lib_scene_mode = SCENE_MODE_AUTO;
		break;
	case AE_CTRL_SCENE_NIGHT:
		lib_scene_mode = SCENE_MODE_NIGHT;
		break;
	case AE_CTRL_SCENE_SPORT:
		lib_scene_mode = SCENE_MODE_SPORT;
		break;
	case AE_CTRL_SCENE_PORTRAIT:
		lib_scene_mode = SCENE_MODE_PORTRAIT;
		break;
	case AE_CTRL_SCENE_LANDSPACE:
		lib_scene_mode = SCENE_MODE_LANDSCAPE;
		break;
	default:
		ISP_LOGW("NOT defined ui scene %ld!", in_ptr->scene.scene_mode);
		break;
	}
	param_ct_ptr->ae_scene_mode = lib_scene_mode;
	type = AE_SET_PARAM_SCENE_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_fps(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.fps = in_ptr->range_fps;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->min_FPS = 100 * in_ptr->range_fps.min_fps;
	param_ct_ptr->max_FPS = 100 * in_ptr->range_fps.max_fps;
	type = AE_SET_PARAM_FPS;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_lib_metering_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	ae_metering_mode_type_t ae_mode = 0;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	aealtek_weight_ui2lib(in_ptr->measure_lum.lum_mode, &ae_mode);
	param_ct_ptr->ae_metering_mode = ae_mode;

	type = AE_SET_PARAM_METERING_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_measure_lum(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.weight = in_ptr->measure_lum.lum_mode;


	if ((AE_CTRL_MEASURE_LUM_TOUCH != in_ptr->measure_lum.lum_mode)
		&& cxt_ptr->touch_param.touch_flag) {
		aealtek_reset_touch_param(cxt_ptr);
		ret = aealtek_reset_touch_ack(cxt_ptr);
		if (ret)
			goto exit;
		if (cxt_ptr->flash_param.enable) {
			cxt_ptr->flash_param.enable = 0;
			aealtek_set_lock(cxt_ptr, 0);
			ret = aealtek_set_flash_est(cxt_ptr, 1);
			if (ret)
				goto exit;
		}
	}
	ret = aealtek_set_lib_metering_mode(cxt_ptr, in_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_stat_trim(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t output_param;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t param_ct;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_reset_touch_ack(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;

	type = AE_SET_PARAM_RESET_ROI_ACK;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_lib_roi(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	rect_roi_config_t *roi_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	roi_ptr = &in_param.set_param.ae_set_roi_setting;

	roi_ptr->roi_count = 1; // 1  > AL_MAX_ROI_NUM ? AL_MAX_ROI_NUM:1;
	roi_ptr->roi[0].weight = 1;
	roi_ptr->roi[0].roi.left = in_ptr->touch_zone.zone.x;
	roi_ptr->roi[0].roi.top = in_ptr->touch_zone.zone.y;
	roi_ptr->roi[0].roi.w = in_ptr->touch_zone.zone.w;
	roi_ptr->roi[0].roi.h = in_ptr->touch_zone.zone.h;
	type = AE_SET_PARAM_ROI;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_touch_zone(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	if (in_ptr->touch_zone.zone.w <= 0
		|| in_ptr->touch_zone.zone.h <= 0) {
		ISP_LOGE("x=%d,y=%d,w=%d,h=%d",in_ptr->touch_zone.zone.x,
				in_ptr->touch_zone.zone.y,
				in_ptr->touch_zone.zone.w,
				in_ptr->touch_zone.zone.h);
		goto exit;
	}
	aealtek_reset_touch_param(cxt_ptr);

	cxt_ptr->touch_param.touch_flag = 1;
	ret = aealtek_set_lib_roi(cxt_ptr, in_ptr, out_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_convergence_req(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	if (cxt_ptr->touch_param.touch_flag) {
		cxt_ptr->touch_param.ctrl_convergence_req_flag = 1;

		ret = aealtek_set_boost(cxt_ptr, 1);
		if (ret)
			goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_lib_init_expousre(struct aealtek_cxt *cxt_ptr, struct aealtek_lib_exposure_data *exposure_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_get_param_t in_param;
	ae_get_param_type_t type = 0;


	if (!cxt_ptr || !exposure_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, exposure_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	type = AE_GET_INIT_EXPOSURE_PARAM;
	in_param.ae_get_param_type = type;
	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	exposure_ptr->init.gain = in_param.para.ae_get_init_expo_param.ad_gain;
	exposure_ptr->init.exp_line = in_param.para.ae_get_init_expo_param.exp_linecount;
	exposure_ptr->init.exp_time = in_param.para.ae_get_init_expo_param.exp_time;
	exposure_ptr->init.iso = in_param.para.ae_get_expo_param.ISO;
	ISP_LOGI("ad_gain=%d, exp_line=%d, exp_time=%d, iso=%d",
			exposure_ptr->init.gain,
			exposure_ptr->init.exp_line,
			exposure_ptr->init.exp_time,
			exposure_ptr->init.iso);

	ISP_LOGI("init_expo ad_gain=%d, exp_line=%d, exp_time=%d, iso=%d",
		in_param.para.ae_get_expo_param.init_expo.ad_gain,
		in_param.para.ae_get_expo_param.init_expo.exp_linecount,
		in_param.para.ae_get_expo_param.init_expo.exp_time,
		in_param.para.ae_get_expo_param.init_expo.ISO);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_get_lib_expousre(struct aealtek_cxt *cxt_ptr, enum aealtek_work_mode mode, struct aealtek_lib_exposure_data *exposure_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_get_param_t in_param;
	ae_get_param_type_t type = 0;

	cmr_u32 i = 0;


	if (!cxt_ptr || !exposure_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, exposure_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	type = AE_GET_EXPOSURE_PARAM;
	in_param.ae_get_param_type = type;
	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	switch (mode) {
	case AEALTEK_PREVIEW:
		exposure_ptr->preview.gain = in_param.para.ae_get_expo_param.cur_expo.ad_gain;
		exposure_ptr->preview.exp_line = in_param.para.ae_get_expo_param.cur_expo.exp_linecount;
		exposure_ptr->preview.exp_time = in_param.para.ae_get_expo_param.cur_expo.exp_time;
		ISP_LOGI("preview ad_gain=%d, exp_line=%d, exp_time=%d",
				exposure_ptr->preview.gain,
				exposure_ptr->preview.exp_line,
				exposure_ptr->preview.exp_time);
		break;
	case AEALTEK_SNAPSHOT:
		exposure_ptr->snapshot.gain = in_param.para.ae_get_expo_param.still_expo.ad_gain;
		exposure_ptr->snapshot.exp_line = in_param.para.ae_get_expo_param.still_expo.exp_linecount;
		exposure_ptr->snapshot.exp_time = in_param.para.ae_get_expo_param.still_expo.exp_time;
		ISP_LOGI("snapshot ad_gain=%d, exp_line=%d, exp_time=%d",
				exposure_ptr->snapshot.gain,
				exposure_ptr->snapshot.exp_line,
				exposure_ptr->snapshot.exp_time);
		break;
	case AEALTEK_BRACKET:
		exposure_ptr->valid_exp_num = in_param.para.ae_get_expo_param.valid_exp_num;

		for (i = 0; i < exposure_ptr->valid_exp_num; ++i) {
			exposure_ptr->bracket_exp[i].gain = in_param.para.ae_get_expo_param.bracket_expo[i].ad_gain;
			exposure_ptr->bracket_exp[i].exp_line = in_param.para.ae_get_expo_param.bracket_expo[i].exp_linecount;
			exposure_ptr->bracket_exp[i].exp_time = in_param.para.ae_get_expo_param.bracket_expo[i].exp_time;
		}
		break;
	default:
		goto exit;
		break;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_init_lib_setting(struct aealtek_cxt *cxt_ptr, ae_set_parameter_init_t *init_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !init_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, init_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_INIT_SETTING;
	in_param.ae_set_param_type = type;
	param_ct_ptr->ae_initial_setting = *init_ptr;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_first_work(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	ae_set_parameter_init_t param_init;
	struct aealtek_lib_exposure_data ae_exposure;


	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	ret = aealtek_convert_ui2initlib(cxt_ptr, &cxt_ptr->nxt_status, &param_init);
	if (ret)
		goto exit;

	ret = aealtek_init_lib_setting(cxt_ptr, &param_init);
	if (ret)
		goto exit;

	ret = aealtek_get_lib_init_expousre(cxt_ptr, &ae_exposure);
	if (ret)
		goto exit;

	ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &ae_exposure.init, &cxt_ptr->lib_data.output_data);
	if (ret)
		goto exit;

	ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		goto exit;

	/*fps calc*/

	if (0 != cxt_ptr->nxt_status.ui_param.work_info.resolution.max_fps
		&& 0 != cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time) {
		cxt_ptr->nxt_status.min_frame_length = SENSOR_EXP_US_BASE / cxt_ptr->nxt_status.ui_param.work_info.resolution.max_fps / cxt_ptr->nxt_status.ui_param.work_info.resolution.line_time;
	} else {
		cxt_ptr->nxt_status.min_frame_length = 0;
	}
	ISP_LOGI("min_frame_length=%d", cxt_ptr->nxt_status.min_frame_length);


	/*fps ctrl*/
	cxt_ptr->sensor_exp_data.lib_exp.dummy = 0;
	if (0 != cxt_ptr->nxt_status.min_frame_length) {
		if (cxt_ptr->sensor_exp_data.lib_exp.exp_line < cxt_ptr->nxt_status.min_frame_length) {
			cxt_ptr->sensor_exp_data.lib_exp.dummy = cxt_ptr->nxt_status.min_frame_length - cxt_ptr->sensor_exp_data.lib_exp.exp_line;
		}
	}

	cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;
	ISP_LOGE("gain=%d, exp_line=%d, exp_time=%d", cxt_ptr->sensor_exp_data.write_exp.gain,
				cxt_ptr->sensor_exp_data.write_exp.exp_line,
				cxt_ptr->sensor_exp_data.write_exp.exp_time);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_work_preview(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t set_in_param;
	ae_output_data_t *output_data_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr;
	ae_sensor_info_t *preview_sensor_ptr = NULL;

	struct aealtek_lib_exposure_data ae_exposure;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_data_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &set_in_param.set_param;

	/*fps calc*/

	if (0 != in_ptr->work_param.resolution.max_fps
		&& 0 != in_ptr->work_param.resolution.line_time) {
		cxt_ptr->nxt_status.min_frame_length = SENSOR_EXP_US_BASE / in_ptr->work_param.resolution.max_fps / in_ptr->work_param.resolution.line_time;
	} else {
		cxt_ptr->nxt_status.min_frame_length = 0;
	}
	ISP_LOGI("min_frame_length=%d", cxt_ptr->nxt_status.min_frame_length);

	//preview_sensor_info
	preview_sensor_ptr = &param_ct_ptr->normal_sensor_info;
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, &in_ptr->work_param.resolution, preview_sensor_ptr);
	if (ret)
		goto exit;
	type = AE_SET_PARAM_SENSOR_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set sensor info lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	//SET_PARAM_REGEN_EXP_INFO
	type = AE_SET_PARAM_REGEN_EXP_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set regen lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	ret = aealtek_get_lib_expousre(cxt_ptr, AEALTEK_PREVIEW, &ae_exposure);
	if (ret)
		goto exit;
	ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &ae_exposure.preview, &cxt_ptr->lib_data.output_data);
	if (ret)
		goto exit;

	ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		goto exit;
	cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_capture_mode(struct aealtek_cxt *cxt_ptr, enum isp_capture_mode cap_mode)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	ae_capture_mode_t lib_cap_mode = 0;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	switch (cap_mode) {
	case ISP_CAP_MODE_AUTO:
		lib_cap_mode = CAPTURE_MODE_AUTO;
		break;
	case ISP_CAP_MODE_ZSL:
		lib_cap_mode = CAPTURE_MODE_ZSL;
		break;
	case ISP_CAP_MODE_HDR:
		lib_cap_mode = CAPTURE_MODE_HDR;
		break;
	case ISP_CAP_MODE_VIDEO:
		lib_cap_mode = CAPTURE_MODE_VIDEO;
		break;
	case ISP_CAP_MODE_VIDEO_HDR:
		lib_cap_mode = CAPTURE_MODE_VIDEO_HDR;
		break;
	case ISP_CAP_MODE_BRACKET:
		lib_cap_mode = CAPTURE_MODE_BRACKET;
		break;
	default:
		ISP_LOGW("NOT defined cap mode %ld", (cmr_int)cap_mode);
		break;
	}

	type = AE_SET_PARAM_CAPTURE_MODE;
	in_param.ae_set_param_type = type;
	param_ct_ptr->capture_mode = lib_cap_mode;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_capture_hdr(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t set_in_param;
	ae_output_data_t *output_data_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr;
	ae_sensor_info_t *cap_sensor_ptr = NULL;

	struct aealtek_lib_exposure_data ae_exposure;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	cxt_ptr->nxt_status.is_hdr_status = 1;


	ret = aealtek_set_capture_mode(cxt_ptr, ISP_CAP_MODE_BRACKET);
	if (ret)
		goto exit;

	obj_ptr = &cxt_ptr->al_obj;
	output_data_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &set_in_param.set_param;

	/*get 3 exposure time & gain*/
	param_ct_ptr->ae_bracket_param.valid_exp_num = 3;
	param_ct_ptr->ae_bracket_param.bracket_evComp[0] = -1000;
	param_ct_ptr->ae_bracket_param.bracket_evComp[1] =  0;
	param_ct_ptr->ae_bracket_param.bracket_evComp[2] = 1000;
	type = AE_SET_PARAM_BRACKET_MODE;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set hdr lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	//capture_sensor_info
	cap_sensor_ptr = &param_ct_ptr->capture_sensor_info;
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, &in_ptr->work_param.resolution, cap_sensor_ptr);
	if (ret)
		goto exit;

	type = AE_SET_PARAM_SENSOR_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set sensor info lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	//SET_PARAM_REGEN_EXP_INFO
	type = AE_SET_PARAM_REGEN_EXP_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set regen lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	ret = aealtek_get_lib_expousre(cxt_ptr, AEALTEK_BRACKET,&ae_exposure);
	if (ret)
		goto exit;
	cxt_ptr->lib_data.exposure_array = ae_exposure;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_capture_normal(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t set_in_param;
	ae_output_data_t *output_data_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr;
	ae_sensor_info_t *cap_sensor_ptr = NULL;

	struct aealtek_lib_exposure_data ae_exposure;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	ret = aealtek_set_capture_mode(cxt_ptr, ISP_CAP_MODE_AUTO);
	if (ret)
		goto exit;

	obj_ptr = &cxt_ptr->al_obj;
	output_data_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &set_in_param.set_param;

	//capture_sensor_info
	cap_sensor_ptr = &param_ct_ptr->capture_sensor_info;
	ret = aealtek_sensor_info_ui2lib(cxt_ptr, &in_ptr->work_param.resolution, cap_sensor_ptr);
	if (ret)
		goto exit;

	type = AE_SET_PARAM_CAPTURE_SENSOR_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set sensor info lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	//SET_PARAM_REGEN_EXP_INFO
	type = AE_SET_PARAM_REGEN_EXP_INFO;
	set_in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&set_in_param, output_data_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set regen lib_ret=%ld is error!", lib_ret);
		ret = ISP_ERROR;
		goto exit;
	}

	ret = aealtek_get_lib_expousre(cxt_ptr, AEALTEK_SNAPSHOT,&ae_exposure);
	if (ret)
		goto exit;
	cxt_ptr->lib_data.exposure_array = ae_exposure;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_work_capture(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	enum isp_capture_mode cap_mode = 0;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cap_mode = in_ptr->work_param.capture_mode;

	ret = aealtek_set_capture_mode(cxt_ptr, cap_mode);
	if (ret)
		goto exit;

	switch (cap_mode) {
	case ISP_CAP_MODE_HDR:
		ret = aealtek_capture_hdr(cxt_ptr, in_ptr, out_ptr);
		break;
	default:
		ret = aealtek_capture_normal(cxt_ptr, in_ptr, out_ptr);
		break;
	}
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_work_video(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t output_param;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t param_ct;

	enum isp3a_work_mode work_mode = 0;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_work_mode(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t output_param;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t param_ct;

	enum isp3a_work_mode work_mode = 0;


	if (!cxt_ptr || !in_ptr || !out_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, in_ptr, out_ptr);
		goto exit;
	}

	ISP_LOGI("frame size:%dx%d, line_time=%d",in_ptr->work_param.resolution.frame_size.w,
			in_ptr->work_param.resolution.frame_size.h,
			in_ptr->work_param.resolution.line_time);

	cxt_ptr->nxt_status.ui_param.work_info = in_ptr->work_param;
#if 0
	if (0 == cxt_ptr->work_cnt) {
		cxt_ptr->nxt_status.is_hdr_status = 0;
		ret = 0;
		//ret = aealtek_first_work(cxt_ptr, in_ptr, out_ptr);
	} else
#endif
	{
		work_mode = in_ptr->work_param.work_mode;
		switch (work_mode) {
		case ISP3A_WORK_MODE_PREVIEW:
			cxt_ptr->nxt_status.is_hdr_status = 0;
			ret = aealtek_work_preview(cxt_ptr, in_ptr, out_ptr);
			break;
		case ISP3A_WORK_MODE_CAPTURE:
			//ret = aealtek_work_capture(cxt_ptr, in_ptr, out_ptr);
			ret = 0;
			break;
		case ISP3A_WORK_MODE_VIDEO:
			cxt_ptr->nxt_status.is_hdr_status = 0;
			//ret = aealtek_work_video(cxt_ptr, in_ptr, out_ptr);
			ret = 0;
			break;
		default:
			ISP_LOGE("not support work mode %d", work_mode);
			break;
		}
	}

	ret = aealtek_get_hwisp_cfg( cxt_ptr, &out_ptr->proc_out.hw_cfg);
	if (ret)
		goto exit;

	ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		goto exit;

	aealtek_lib_to_out_info(cxt_ptr, &cxt_ptr->lib_data.output_data, &out_ptr->proc_out.ae_info);

	ret = aealtek_pre_to_sensor(cxt_ptr, 1);
	if (ret)
		goto exit;
	++cxt_ptr->work_cnt;
	cxt_ptr->cur_status.ui_param.work_info = cxt_ptr->nxt_status.ui_param.work_info;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_lock(struct aealtek_cxt *cxt_ptr, cmr_int is_lock)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	ISP_LOGI("lock_cnt=%d, is_lock=%ld", cxt_ptr->lock_cnt, is_lock);

	if (is_lock) {
		if (0 == cxt_ptr->lock_cnt) {
		}
		cxt_ptr->lock_cnt++;
	} else {
		cxt_ptr->lock_cnt--;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGI("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_flash_est(struct aealtek_cxt *cxt_ptr, cmr_u32 is_reset)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	ae_flash_st_t lib_led_st = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (is_reset)
		type = AE_SET_PARAM_RESET_FLASH_EST;
	else
		type = AE_SET_PARAM_FLASHAE_EST;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_hw_flash_status(struct aealtek_cxt *cxt_ptr, cmr_int is_lighting)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	ae_flash_st_t lib_led_st = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (is_lighting)
		lib_led_st = AE_FLASH_ON;
	else
		lib_led_st = AE_FLASH_OFF;
	param_ct_ptr->ae_flash_st = lib_led_st;
	type = AE_SET_PARAM_FLASH_ST;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_ui_flash_mode(struct aealtek_cxt *cxt_ptr, enum isp_ui_flash_status ui_flash_status)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	AL3A_FE_UI_FLASH_MODE lib_flash_mode = 0;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	switch (ui_flash_status) {
	case ISP_UI_FLASH_STATUS_OFF:
		lib_flash_mode = AL3A_UI_FLASH_OFF;
		break;
	case ISP_UI_FLASH_STATUS_AUTO:
		lib_flash_mode = AL3A_UI_FLASH_AUTO;
		break;
	case ISP_UI_FLASH_STATUS_ON:
		lib_flash_mode = AL3A_UI_FLASH_ON;
		break;
	case ISP_UI_FLASH_STATUS_TORCH:
		lib_flash_mode = AL3A_UI_FLASH_TORCH;
		break;
	default:
		ISP_LOGW("NOT defined ui flash status %ld!", (cmr_int)ui_flash_status);
		break;
	}
	param_ct_ptr->flash_mode = lib_flash_mode;
	type = AE_SET_PARAM_FLASH_MODE;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_flash_prepare_on(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_PRE_FLASHAE_EST;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_flash_with_lighting(struct aealtek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_PRECAP_START;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_preflash_before(struct aealtek_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	cmr_int index = 0;
	cmr_int exp_gain = 0;


	cmr_bzero(&cxt_ptr->flash_param, sizeof(cxt_ptr->flash_param));
	cxt_ptr->flash_param.enable = 1;

	aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_PREPARE_ON);
	return rtn;
}

static cmr_int aealtek_set_flash_notice(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_flash_notice *notice_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t output_param;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t param_ct;

	enum isp_flash_mode mode = 0;

	if (!cxt_ptr || !notice_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, notice_ptr);
		goto exit;
	}

	mode = notice_ptr->flash_mode;
	ISP_LOGI("mode=%d", mode);
	switch (mode) {
	case ISP_FLASH_PRE_BEFORE:
		ISP_LOGI("=========pre flash before start");
		ret = aealtek_set_preflash_before(cxt_ptr);
		if (ret)
			goto exit;
		notice_ptr->ui_flash_status = ISP_UI_FLASH_STATUS_ON;
		ret = aealtek_set_ui_flash_mode(cxt_ptr, notice_ptr->ui_flash_status);
		if (ret)
			goto exit;

		ret = aealtek_set_flash_prepare_on(cxt_ptr);
		if (ret)
			goto exit;
		ret = aealtek_set_boost(cxt_ptr, 1);
		if (ret)
			goto exit;
		ISP_LOGI("=========pre flash before end");
		break;

	case ISP_FLASH_PRE_LIGHTING:
		ISP_LOGI("=========pre flash lighting start");
		ret = aealtek_set_lock(cxt_ptr, 0);
		if (ret)
			goto exit;
		ret = aealtek_set_flash_est(cxt_ptr, 0);
		if (ret)
			goto exit;
		aealtek_set_hw_flash_status(cxt_ptr, 1);

		aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_LIGHTING);
		ISP_LOGI("=========pre flash lighting end");

		break;

	case ISP_FLASH_PRE_AFTER:
		ISP_LOGI("=========pre flash close start");
		aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_CLOSE);

		aealtek_set_hw_flash_status(cxt_ptr, 0);
		aealtek_set_boost(cxt_ptr, 0);

		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &cxt_ptr->flash_param.pre_flash_before.exp_cell, &cxt_ptr->lib_data.output_data);
		if (ret)
			goto exit;
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			goto exit;
		ISP_LOGI("=========pre flash close end");
		break;

	case ISP_FLASH_MAIN_BEFORE:
		aealtek_change_flash_state(cxt_ptr, cxt_ptr->flash_param.flash_state, AEALTEK_FLASH_STATE_MAX);

		if (cxt_ptr->init_in_param.ops_in.flash_set_charge) {
			struct isp_flash_cfg flash_cfg;
			struct isp_flash_element  flash_element;

			flash_cfg.led_idx = 0;
			flash_cfg.type = ISP_FLASH_TYPE_MAIN;

			flash_element.index = cxt_ptr->flash_param.main_flash_est.led_0.idx;
			flash_element.val = cxt_ptr->flash_param.main_flash_est.led_0.current;
			cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);
		}
		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &cxt_ptr->flash_param.main_flash_est.exp_cell, &cxt_ptr->lib_data.output_data);
		if (ret)
			goto exit;
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			goto exit;
		cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;
		aealtek_pre_to_sensor(cxt_ptr, 1);
		break;

	case ISP_FLASH_MAIN_LIGHTING:
		break;
	case ISP_FLASH_MAIN_AFTER:
		aealtek_set_hw_flash_status(cxt_ptr, 0);
		ret = aealtek_convert_lib_exposure2outdata(cxt_ptr, &cxt_ptr->flash_param.pre_flash_before.exp_cell, &cxt_ptr->lib_data.output_data);
		if (ret)
			goto exit;
		ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
		if (ret)
			goto exit;
		break;
	default:
		break;
	}
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_flash_effect(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;

	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t output_param;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t param_ct;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
exit:
	return ret;
}

static cmr_int aealtek_set_sof_to_lib(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct aealtek_exposure_param exp_param)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	type = AE_SET_PARAM_SOF_NOTIFY;
	param_ct_ptr->sof_notify_param.sys_sof_index = in_ptr->sof_param.frame_index;
	param_ct_ptr->sof_notify_param.exp_linecount = exp_param.exp_line;
	param_ct_ptr->sof_notify_param.exp_adgain = exp_param.gain;
	param_ct_ptr->sof_notify_param.exp_time = exp_param.exp_time;

	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib_ret=%ld", lib_ret);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_set_sof(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct seq_item in_est;
	struct seq_cell out_actual;
	struct seq_cell out_write;


	if (!cxt_ptr || !in_ptr || !out_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, in_ptr, out_ptr);
		goto exit;
	}
	if (0 == in_ptr->sof_param.frame_index)
		seq_reset(cxt_ptr->seq_handle);

	cxt_ptr->sensor_exp_data.actual_exp = cxt_ptr->sensor_exp_data.lib_exp;
	cxt_ptr->sensor_exp_data.write_exp = cxt_ptr->sensor_exp_data.lib_exp;

	ISP_LOGI("gain=%d, exp_line=%d, exp_time=%d", cxt_ptr->sensor_exp_data.write_exp.gain,
				cxt_ptr->sensor_exp_data.write_exp.exp_line,
				cxt_ptr->sensor_exp_data.write_exp.exp_time);

	in_est.work_mode = SEQ_WORK_PREVIEW;
	in_est.cell.frame_id = in_ptr->sof_param.frame_index;
	in_est.cell.exp_line = cxt_ptr->sensor_exp_data.lib_exp.exp_line;
	in_est.cell.exp_time = cxt_ptr->sensor_exp_data.lib_exp.exp_time;
	in_est.cell.gain = cxt_ptr->sensor_exp_data.lib_exp.gain;
	ret = seq_put(cxt_ptr->seq_handle, &in_est, &out_actual, &out_write);
	if (ret) {
		ISP_LOGW("warning seq_put=%ld !!!", ret);
		out_actual = in_est.cell;
		out_write = in_est.cell;
	}

	ISP_LOGI("out_actual exp_line=%d, exp_time=%d", out_actual.exp_line, out_actual.exp_time);

	cxt_ptr->sensor_exp_data.actual_exp.exp_line = out_actual.exp_line;
	cxt_ptr->sensor_exp_data.actual_exp.exp_time = out_actual.exp_time;
	cxt_ptr->sensor_exp_data.actual_exp.gain = out_actual.gain;

	cxt_ptr->sensor_exp_data.write_exp.exp_line = out_write.exp_line;
	cxt_ptr->sensor_exp_data.write_exp.exp_time = out_write.exp_time;
	cxt_ptr->sensor_exp_data.write_exp.gain = out_write.gain;

	ret = aealtek_set_sof_to_lib(cxt_ptr, in_ptr, cxt_ptr->sensor_exp_data.actual_exp);
	if (ret)
		goto exit;
	ret = aealtek_pre_to_sensor(cxt_ptr, 0);
	if (ret)
		goto exit;
	out_ptr->isp_d_gain = cxt_ptr->sensor_exp_data.actual_exp.gain;
	out_ptr->hw_iso_speed = cxt_ptr->sensor_exp_data.actual_exp.iso;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_get_bv_by_lum(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->bv = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.bv_val;
	ISP_LOGI("bv=%d", out_ptr->bv);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_bv_by_gain(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_set_fd_param(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	rect_roi_config_t *roi_ptr = NULL;
	cmr_u16 i = 0;
	cmr_u16 face_num = 0;
	cmr_u32 sx = 0;
	cmr_u32 sy = 0;
	cmr_u32 ex = 0;
	cmr_u32 ey = 0;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	roi_ptr = &in_param.set_param.ae_set_face_roi_setting;

	face_num = in_ptr->face_area.face_num;

	roi_ptr->roi_count = face_num > AL_MAX_ROI_NUM ? AL_MAX_ROI_NUM:face_num;
	for (i = 0; i < face_num; ++i) {
		sx = in_ptr->face_area.face_info[i].sx;;
		sy = in_ptr->face_area.face_info[i].sy;
		ex = in_ptr->face_area.face_info[i].ex;
		ey = in_ptr->face_area.face_info[i].ey;

		roi_ptr->roi[i].weight = 0;
		roi_ptr->roi[i].roi.left = sx;
		roi_ptr->roi[i].roi.top = sy;
		roi_ptr->roi[i].roi.w = ex - sx + 1;
		roi_ptr->roi[i].roi.h = ey - sy + 1;
	}
	type = AE_SET_PARAM_FACE_ROI;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_gyro_param(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;
	cmr_int i = 0;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.iso = in_ptr->iso.iso_mode;

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;


	param_ct_ptr->ae_gyro_info.param_mode_int = in_ptr->gyro.param_type;
	for (i = 0; i < 3; ++i) {
		param_ct_ptr->ae_gyro_info.uinfo[i] = in_ptr->gyro.uinfo[i];
		param_ct_ptr->ae_gyro_info.finfo[i] = in_ptr->gyro.finfo[i];
	}
	type = AE_SET_PARAM_GYRO_INFO;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_hdr_ev(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	enum ae_ctrl_hdr_ev_level level = 0;

	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t output_param;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->nxt_status.ui_param.hdr_level = in_ptr->soft_hdr_ev.level;

	level = in_ptr->soft_hdr_ev.level;
	switch (level) {
	case AE_CTRL_HDR_EV_UNDEREXPOSURE:
		ret = aealtek_pre_to_sensor(cxt_ptr, 1);
		break;
	case AE_CTRL_HDR_EV_NORMAL:
		break;
	case AE_CTRL_HDR_EV_OVEREXPOSURE:
		break;
	default:
		break;
	}
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aealtek_set_awb_info(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_awb_gain *awb_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !awb_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, awb_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	if (awb_ptr->normal_gain.r == 0
		&& awb_ptr->normal_gain.g == 0
		&& awb_ptr->normal_gain.b == 0) {
		param_ct_ptr->ae_awb_info.color_temp = 5000;
		param_ct_ptr->ae_awb_info.gain_r = cxt_ptr->lib_data.ae_otp_data.calib_r_gain;
		param_ct_ptr->ae_awb_info.gain_g = cxt_ptr->lib_data.ae_otp_data.calib_g_gain;;
		param_ct_ptr->ae_awb_info.gain_b = cxt_ptr->lib_data.ae_otp_data.calib_b_gain;
		param_ct_ptr->ae_awb_info.awb_state = 1;
	} else {
		param_ct_ptr->ae_awb_info.color_temp = awb_ptr->color_temp;
		param_ct_ptr->ae_awb_info.gain_r = awb_ptr->normal_gain.r;
		param_ct_ptr->ae_awb_info.gain_g = awb_ptr->normal_gain.g;
		param_ct_ptr->ae_awb_info.gain_b = awb_ptr->normal_gain.b;

		param_ct_ptr->ae_awb_info.awb_balanced_wb.r_gain = awb_ptr->balance_gain.r;
		param_ct_ptr->ae_awb_info.awb_balanced_wb.g_gain = awb_ptr->balance_gain.g;
		param_ct_ptr->ae_awb_info.awb_balanced_wb.b_gain = awb_ptr->balance_gain.b;

		param_ct_ptr->ae_awb_info.awb_state = awb_ptr->awb_state;
	}

	ISP_LOGI("awb color_temp=%d, r=%d, g=%d, b=%d, awb_state=%d",
		param_ct_ptr->ae_awb_info.color_temp,
		param_ct_ptr->ae_awb_info.gain_r,
		param_ct_ptr->ae_awb_info.gain_g,
		param_ct_ptr->ae_awb_info.gain_b,
		param_ct_ptr->ae_awb_info.awb_state);

	type = AE_SET_PARAM_AWB_INFO;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set awb info %ld is error!", lib_ret);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_awb_report(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	ret = aealtek_set_awb_info(cxt_ptr, &in_ptr->awb_report);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_set_af_report(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_set_param_t in_param;
	ae_output_data_t *output_param_ptr = NULL;
	ae_set_param_type_t type = 0;
	ae_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->ae_af_info.af_state = in_ptr->af_report.state;
	param_ct_ptr->ae_af_info.af_distance = in_ptr->af_report.distance;

	type = AE_SET_PARAM_AF_INFO;
	in_param.ae_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, obj_ptr->ae);
	if (lib_ret) {
		ISP_LOGE("lib set af info %ld is error!", lib_ret);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_set_snapshot_finished(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	if (cxt_ptr->flash_param.enable) {
		ret = aealtek_set_flash_est(cxt_ptr, 1);
		if (ret)
			goto exit;
		cxt_ptr->flash_param.enable = 0;
		aealtek_set_lock(cxt_ptr, 0);
	}
	if (cxt_ptr->touch_param.touch_flag) {
		aealtek_reset_touch_param(cxt_ptr);
		ret = aealtek_reset_touch_ack(cxt_ptr);
		if (ret)
			goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_flash_eb(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->flash_eb = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_need_flash_flg;
	ISP_LOGI("need flash=%d", out_ptr->flash_eb);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_iso(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	out_ptr->iso_val = cxt_ptr->sensor_exp_data.lib_exp.iso;
	ISP_LOGI("iso_val=%d", out_ptr->iso_val);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int aealtek_get_ev_table(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_param_in *in_ptr, struct ae_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int ae_altek_adpt_init(void *in, void *out, cmr_handle *handle)
{
	cmr_int ret = ISP_ERROR;
	struct aealtek_cxt *cxt_ptr = NULL;
	struct ae_ctrl_init_in *in_ptr = (struct ae_ctrl_init_in*)in;
	struct ae_ctrl_init_out *out_ptr = (struct ae_ctrl_init_out*)out;


	if (!in_ptr || !out_ptr || !handle) {
		ISP_LOGE("init param %p %p is null !!!", in_ptr, out_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*handle = NULL;
	cxt_ptr = (struct aealtek_cxt*)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("failed to create ae ctrl context!");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt_ptr, sizeof(*cxt_ptr));

	cxt_ptr->camera_id = in_ptr->camera_id;
	cxt_ptr->caller_handle = in_ptr->caller_handle;
	cxt_ptr->init_in_param = *in_ptr;

	ISP_LOGI("init frame %dx%d,lint_time=%d,index=%d",
		cxt_ptr->init_in_param.preview_work.resolution.frame_size.w,
		cxt_ptr->init_in_param.preview_work.resolution.frame_size.h,
		cxt_ptr->init_in_param.preview_work.resolution.line_time,
		cxt_ptr->init_in_param.preview_work.resolution.sensor_size_index);

	ret = aealtek_load_lib(cxt_ptr);
	if (ret) {
		goto exit;
	}
	ret = aealtek_init(cxt_ptr, in_ptr, out_ptr);
	if (ret) {
		goto exit;
	}
	ret = aealtek_first_work(cxt_ptr, NULL, NULL);
	if (ret)
		goto exit;
	out_ptr->hw_iso_speed = cxt_ptr->sensor_exp_data.lib_exp.iso;
	ret = aealtek_get_hwisp_cfg( cxt_ptr, &out_ptr->hw_cfg);
	if (ret)
		goto exit;

	ret = aealtek_pre_to_sensor(cxt_ptr, 1);
	if (ret)
		goto exit;

	*handle = (cmr_handle)cxt_ptr;
	cxt_ptr->is_inited = 1;
	return ISP_SUCCESS;
exit:
	if (cxt_ptr) {
		aealtek_unload_lib(cxt_ptr);

		free((void*)cxt_ptr);
	}
	return ret;
}

static cmr_int ae_altek_adpt_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct aealtek_cxt *cxt_ptr = (struct aealtek_cxt*)handle;

	ISP_CHECK_HANDLE_VALID(handle);

	if (0 == cxt_ptr->is_inited) {
		ISP_LOGW("has been de-initialized");
		goto exit;
	}
	if (cxt_ptr->al_obj.deinit)
		cxt_ptr->al_obj.deinit(cxt_ptr->al_obj.ae);

	aealtek_unload_lib(cxt_ptr);

	seq_deinit(cxt_ptr->seq_handle);

	cxt_ptr->is_inited = 0;
	free((void*)handle);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int ae_altek_adpt_ioctrl(cmr_handle handle, cmr_int cmd, void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct aealtek_cxt *cxt_ptr = (struct aealtek_cxt*)handle;
	struct ae_ctrl_param_in *in_ptr = (struct ae_ctrl_param_in*)in;
	struct ae_ctrl_param_out *out_ptr = (struct ae_ctrl_param_out*)out;


	if (!handle) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	ISP_LOGE("cmd %ld", cmd);
	switch (cmd) {
	case AE_CTRL_GET_LUM:
		break;
	case AE_CTRL_GET_ISO:
		ret = aealtek_get_iso(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_EV_TABLE:
		ret = aealtek_get_ev_table(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_BYPASS:
		ret = aealtek_set_bypass(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FLICKER:
		ret = aealtek_set_flicker(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_SCENE_MODE:
		ret = aealtek_set_scene_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_ISO:
		ret = aealtek_set_iso(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FPS:
		ret = aealtek_set_fps(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_EXP_COMP:
		ret = aealtek_set_exp_comp(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_MEASURE_LUM:
		ret = aealtek_set_measure_lum(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_STAT_TRIM:
		ret = aealtek_set_stat_trim(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_TOUCH_ZONE:
		ret = aealtek_set_touch_zone(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_WORK_MODE:
		ret = aealtek_set_work_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_PAUSE:
		ret = aealtek_set_lock(cxt_ptr, 1);
		break;
	case AE_CTRL_SET_RESTORE:
		ret = aealtek_set_lock(cxt_ptr, 0);
		break;
	case AE_CTRL_SET_FLASH_NOTICE:
		ret = aealtek_set_flash_notice(cxt_ptr, &in_ptr->flash_notice, out_ptr);
		break;
	case AE_CTRL_GET_FLASH_EFFECT:
		ret = aealtek_get_flash_effect(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_AE_STATE:
		break;
	case AE_CTRL_GET_FLASH_EB:
		ret = aealtek_get_flash_eb(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_SOF:
		ret = aealtek_set_sof(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_BV_BY_LUM:
		ret = aealtek_get_bv_by_lum(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_BV_BY_GAIN:
		ret = aealtek_get_bv_by_gain(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_FORCE_PAUSE:
		aealtek_set_lock(cxt_ptr, 1);
		break;
	case AE_CTRL_SET_FORCE_RESTORE:
		aealtek_set_lock(cxt_ptr, 0);
		break;
	case AE_CTRL_GET_MONITOR_INFO:
		break;
	case AE_CTRL_GET_FLICKER_MODE:
		break;
	case AE_CTRL_SET_FD_ENABLE:
		break;
	case AE_CTRL_SET_FD_PARAM:
		ret = aealtek_set_fd_param(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_SKIP_FRAME_NUM:
		break;
	case AE_CTRL_SET_QUICK_MODE:
		break;
	case AE_CTRL_SET_GYRO_PARAM:
		ret = aealtek_set_gyro_param(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_HDR_EV:
		ret = aealtek_set_hdr_ev(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_GET_HWISP_CONFIG:
		break;
	case AE_CTRL_SET_AWB_REPORT:
		ret = aealtek_set_awb_report(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_AF_REPORT:
		ret = aealtek_set_af_report(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_CONVERGENCE_REQ:
		ret = aealtek_set_convergence_req(cxt_ptr, in_ptr, out_ptr);
		break;
	case AE_CTRL_SET_SNAPSHOT_FINISHED:
		ret = aealtek_set_snapshot_finished(cxt_ptr, in_ptr, out_ptr);
		break;
	default:
		ISP_LOGE("cmd %ld is not defined!", cmd);
		break;
	};
exit:

	return ret;
}

static cmr_int aealtek_pre_process(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_proc_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_uint lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;

	if (obj_ptr) {
		if (!in_ptr->stat_data_ptr->addr) {
			ISP_LOGE("LIB AE stat data is NULL");
			goto exit;
		}
		lib_ret = al3AWrapper_DispatchHW3A_AEStats((ISP_DRV_META_AE_t*)in_ptr->stat_data_ptr->addr, &cxt_ptr->lib_data.stats_data, obj_ptr, obj_ptr->ae);
		if (ERR_WPR_AE_SUCCESS != lib_ret) {
			ret = ISP_ERROR;
			ISP_LOGE("dispatch lib_ret=%ld", lib_ret);
			goto exit;
		}
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGI("done %ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aealtek_post_process(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_proc_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct ae_ctrl_callback_in callback_in;
	cmr_u32 is_special_converge_flag = 0;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	//cxt_ptr->update_list. //TBD

	ret = aealtek_lib_exposure2sensor(cxt_ptr, &cxt_ptr->lib_data.output_data, &cxt_ptr->sensor_exp_data.lib_exp);
	if (ret)
		goto exit;

	/*fps ctrl*/
	cxt_ptr->sensor_exp_data.lib_exp.dummy = 0;
	if (0 != cxt_ptr->nxt_status.min_frame_length) {
		if (cxt_ptr->sensor_exp_data.lib_exp.exp_line < cxt_ptr->nxt_status.min_frame_length) {
			cxt_ptr->sensor_exp_data.lib_exp.dummy = cxt_ptr->nxt_status.min_frame_length - cxt_ptr->sensor_exp_data.lib_exp.exp_line;
		}
	}


	aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_SEARCHING);

	/*touch*/
	if (cxt_ptr->touch_param.touch_flag) {
		if (cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_roi_change_st)
			cxt_ptr->touch_param.ctrl_roi_changed_flag = 1;

		/*wait roi converged command*/
		if (cxt_ptr->touch_param.ctrl_convergence_req_flag) {
			is_special_converge_flag = 1;

			if (cxt_ptr->touch_param.ctrl_roi_changed_flag
				&& AE_ROI_CONVERGED == cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_LibStates) {
				ret = aealtek_set_boost(cxt_ptr, 0);
				if (ret)
					ISP_LOGW("set boost failed !!!");
				ISP_LOGI("CB TOUCH_CONVERGED");
				aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_CONVERGED);
				cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRl_CB_TOUCH_CONVERGED, &callback_in);
			}
		}
	}
	/*flash*/
	if (cxt_ptr->flash_param.enable) {
		ISP_LOGI("======lib converged =%d",cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged);
		ISP_LOGI("======lib ae_st=%d",
			cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_LibStates);
		ISP_LOGI("=====lib flash_st=%d",
			cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_FlashStates);
		switch (cxt_ptr->flash_param.flash_state) {
		case AEALTEK_FLASH_STATE_PREPARE_ON:
			ISP_LOGI("========led prepare on");

			is_special_converge_flag = 1;
			if (cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged
				&& AE_EST_WITH_LED_PRE_DONE == cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_FlashStates) {
				aealtek_set_lock(cxt_ptr, 1);
				aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_FLASH_PREPARE_CONVERGED);

				cxt_ptr->flash_param.pre_flash_before.led_num = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.ucDICTotal_idx;
				cxt_ptr->flash_param.pre_flash_before.led_0.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.ucDIC1_idx;
				cxt_ptr->flash_param.pre_flash_before.led_0.current = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.udLED1Current;
				cxt_ptr->flash_param.pre_flash_before.led_1.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.ucDIC2_idx;
				cxt_ptr->flash_param.pre_flash_before.led_1.current = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.preflash_ctrldat.udLED2Current;

				cxt_ptr->flash_param.pre_flash_before.exp_cell.gain = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.ad_gain;
				cxt_ptr->flash_param.pre_flash_before.exp_cell.exp_line = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.exposure_line;
				cxt_ptr->flash_param.pre_flash_before.exp_cell.exp_time = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.exposure_time;
				cxt_ptr->flash_param.pre_flash_before.exp_cell.iso = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.flash_off_exp_dat.ISO;

				if (cxt_ptr->init_in_param.ops_in.flash_set_charge) {
					struct isp_flash_cfg flash_cfg;
					struct isp_flash_element  flash_element;

					flash_cfg.led_idx = 0;
					flash_cfg.type = ISP_FLASH_TYPE_PREFLASH;

					flash_element.index = cxt_ptr->flash_param.pre_flash_before.led_0.idx;
					flash_element.val = cxt_ptr->flash_param.pre_flash_before.led_0.current;
					cxt_ptr->init_in_param.ops_in.flash_set_charge(cxt_ptr->caller_handle, &flash_cfg, &flash_element);
				}
				if (cxt_ptr->touch_param.touch_flag
					&& cxt_ptr->touch_param.ctrl_roi_changed_flag) {
					cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_CONVERGED, &callback_in);
				} else {
					cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_CONVERGED, &callback_in);
				}
			}
			break;
		case AEALTEK_FLASH_STATE_LIGHTING:
			ISP_LOGI("========led on");

			is_special_converge_flag = 1;
			if (cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged
				&& AE_EST_WITH_LED_DONE == cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_FlashStates) {
				aealtek_set_lock(cxt_ptr, 1);
				aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_FLASH_ON_CONVERGED);

				cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_FLASHING_CONVERGED, &callback_in);

				cxt_ptr->flash_param.main_flash_est.led_num = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.ucDICTotal_idx;
				cxt_ptr->flash_param.main_flash_est.led_0.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.ucDIC1_idx;
				cxt_ptr->flash_param.main_flash_est.led_0.current = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.udLED1Current;
				cxt_ptr->flash_param.main_flash_est.led_1.idx = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.ucDIC2_idx;
				cxt_ptr->flash_param.main_flash_est.led_1.current = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.mainflash_ctrldat.udLED2Current;

				cxt_ptr->flash_param.main_flash_est.exp_cell.gain = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.ad_gain;
				cxt_ptr->flash_param.main_flash_est.exp_cell.exp_line = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.exposure_line;
				cxt_ptr->flash_param.main_flash_est.exp_cell.exp_time = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.exposure_time;
				cxt_ptr->flash_param.main_flash_est.exp_cell.iso = cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.snapshot_exp_dat.ISO;
			}
			break;
		default:
			break;
		}
	}

	if (0 == is_special_converge_flag
		&& cxt_ptr->lib_data.output_data.rpt_3a_update.ae_update.ae_converged) {
		aealtek_change_ae_state(cxt_ptr, cxt_ptr->ae_state, ISP3A_AE_CTRL_ST_CONVERGED);
	}

	if (cxt_ptr->init_in_param.ops_in.ae_callback) {
		aealtek_lib_to_out_info(cxt_ptr, &cxt_ptr->lib_data.output_data, &callback_in.proc_out.ae_info);

		callback_in.proc_out.priv_size = sizeof(cxt_ptr->lib_data.output_data.rpt_3a_update);
		callback_in.proc_out.priv_data = &cxt_ptr->lib_data.output_data.rpt_3a_update;

		callback_in.proc_out.ae_frame.is_skip_cur_frame = 0;
		callback_in.proc_out.ae_frame.stats_buff_ptr = in_ptr->stat_data_ptr;
		cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_PROC_OUT, &callback_in);
	}

	return ISP_SUCCESS;
exit:
	ISP_LOGI("ret=%ld", ret);
	return ret;
}

static cmr_int aealtek_process(struct aealtek_cxt *cxt_ptr, struct ae_ctrl_proc_in *in_ptr, struct ae_ctrl_proc_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_uint lib_ret = 0;
	alAERuntimeObj_t *obj_ptr = NULL;
	ae_output_data_t *out_data_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->al_obj;
	out_data_ptr = &cxt_ptr->lib_data.output_data;

	if (obj_ptr && obj_ptr->estimation)
		lib_ret = obj_ptr->estimation(&cxt_ptr->lib_data.stats_data, obj_ptr->ae, out_data_ptr);
//	if (lib_ret)
//		goto exit;
	ISP_LOGI("mean=%d, BV=%d, exposure_line=%d, gain=%d",\
		out_data_ptr->rpt_3a_update.ae_update.curmean,
		out_data_ptr->rpt_3a_update.ae_update.bv_val,
		out_data_ptr->rpt_3a_update.ae_update.exposure_line,
		out_data_ptr->rpt_3a_update.ae_update.sensor_ad_gain);

	aealtek_print_lib_log(cxt_ptr, &cxt_ptr->lib_data.output_data);

	return ISP_SUCCESS;
exit:
	ISP_LOGI("done %ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int ae_altek_adpt_process(cmr_handle handle, void *in, void *out)
{
	cmr_int ret = ISP_ERROR;
	struct aealtek_cxt *cxt_ptr = (struct aealtek_cxt*)handle;
	struct ae_ctrl_proc_in *in_ptr = (struct ae_ctrl_proc_in*)in;
	struct ae_ctrl_proc_out *out_ptr = (struct ae_ctrl_proc_out*)out;


	if (!handle || !in) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	cxt_ptr->proc_in = *in_ptr;


	if (!in_ptr->stat_data_ptr) {
		ISP_LOGE("param stat data is NULL error!");
		goto exit;
	}
	if (cxt_ptr->lock_cnt) {
		ISP_LOGW("lock st %d", cxt_ptr->lock_cnt);
		goto exit;
	}
	ret = aealtek_pre_process(cxt_ptr, in_ptr);
	if (ret)
		goto exit;
	ret = aealtek_process(cxt_ptr, in_ptr, out_ptr);
	if (ret)
		goto exit;
	ret = aealtek_post_process(cxt_ptr, in_ptr);
	if (ret)
		goto exit;

	return ISP_SUCCESS;
exit:
{
	struct ae_ctrl_callback_in callback_in;

		callback_in.proc_out.ae_frame.is_skip_cur_frame = 1;
		callback_in.proc_out.ae_frame.stats_buff_ptr = in_ptr->stat_data_ptr;
		cxt_ptr->init_in_param.ops_in.ae_callback(cxt_ptr->caller_handle, AE_CTRL_CB_PROC_OUT, &callback_in);
}
	ISP_LOGI("done %ld", ret);
	return ret;
}

/*************************************ADAPTER CONFIGURATION ***************************************/
static struct isp_lib_config ae_altek_lib_info = {
	.product_id = 0,
	.version_id = 0,
};

static struct adpt_ops_type ae_altek_adpt_ops = {
	.adpt_init = ae_altek_adpt_init,
	.adpt_deinit = ae_altek_adpt_deinit,
	.adpt_process = ae_altek_adpt_process,
	.adpt_ioctrl = ae_altek_adpt_ioctrl,
};

struct adpt_register_type ae_altek_adpt_info = {
	.lib_type = ADPT_LIB_AE,
	.lib_info = &ae_altek_lib_info,
	.ops = &ae_altek_adpt_ops,
};
