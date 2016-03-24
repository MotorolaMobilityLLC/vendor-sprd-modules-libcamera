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
#define LOG_TAG "alk_adpt_af"

#include <dlfcn.h>
#include "isp_type.h"
#include "af_ctrl.h"
#include "af_adpt.h"
#include "isp_adpt.h"
#include "allib_af.h"
#include "hw3a_stats.h"
#include "alwrapper_af_errcode.h"
#include "alwrapper_af.h"
#include "af_alg.h"
#include "cutils/properties.h"

#define FEATRUE_SPRD_CAF_TRIGGER

#define LIBRARY_PATH "libalAFLib.so"
#define CAF_LIBRARY_PATH "libspcaftrigger.so"
#define SEC_TO_US	1000000L
#define AE_CONVERGE_TIMEOUT	(2 * SEC_TO_US)

struct af_altek_lib_ops {
	void *(*init) (void *af_out_obj);
	cmr_u8 (*deinit) (void *alAFLib_runtim_obj, void *alAFLib_out_obj);
	cmr_u8 (*set_parameters) (struct allib_af_input_set_param_t *param,
				  void *alAFLib_out_obj,
				  void *alAFLib_runtim_obj);
	cmr_u8 (*get_parameters) (struct allib_af_input_get_param_t *param,
				  void *alAFLib_out_obj,
				  void *alAFLib_runtim_obj);
	cmr_u8 (*process) (void *alAFLib_hw_stats_t,
			   void *alAFLib_out_obj, void *alAFLib_runtim_obj);
};

struct af_caf_trigger_ops {
	cmr_s32 (*trigger_init) (struct af_alg_tuning_block_param *init_param,
				caf_alg_handle_t *handle);
	cmr_s32 (*trigger_deinit) (caf_alg_handle_t handle);
	cmr_s32 (*trigger_calc) (caf_alg_handle_t handle,
				struct caf_alg_calc_param *alg_calc_in,
				struct caf_alg_result *alg_calc_result);
	cmr_s32 (*trigger_ioctrl) (caf_alg_handle_t handle,
				enum af_alg_cmd cmd,
				void *param0,
				void *param1);
};

struct af_altek_lib_api {
	void (*af_altek_version) (struct allib_af_version_t *alAFLib_ver);
	cmr_u8 (*af_altek_load_func) (struct allib_af_ops_t *alAFLib_ops);
};

enum af_altek_adpt_status_t {
	AF_ADPT_STARTED,
	AF_ADPT_FOCUSING,
	AF_ADPT_FOCUSED,
	AF_ADPT_DONE,
	AF_ADPT_IDLE,
};

struct af_altek_ae_status_info {
	cmr_int ae_converged;
	cmr_u32 ae_locked;
	struct isp3a_timestamp timestamp;
};

struct af_altek_stats_config_t {
	cmr_u16 token_id;
	cmr_u8 need_update_token;
};

struct af_altek_report_t {
	struct af_report_update_t report_out;
	cmr_u8 need_report;
};

struct af_altek_vcm_tune_info {
	cmr_u16 tuning_enable;
	cmr_u16 cur_pos;
};

struct af_altek_context {
	cmr_u8 inited;
	cmr_u32 camera_id;
	cmr_u32 frame_id;
	cmr_u32 af_mode;
	cmr_handle caller_handle;
	cmr_handle altek_lib_handle;
	cmr_handle caf_lib_handle;
	caf_alg_handle_t caf_trigger_handle;
	struct af_ctrl_motor_pos motor_info;
	cmr_int ae_awb_lock_cnt;
	struct af_altek_lib_api lib_api;
	struct af_altek_lib_ops ops;
	struct af_caf_trigger_ops caf_ops;
	struct af_ctrl_cb_ops_type cb_ops;
	struct allib_af_output_report_t af_out_obj;
	void *af_runtime_obj;
	struct af_altek_ae_status_info ae_status_info;
	enum af_altek_adpt_status_t af_cur_status;
	struct af_altek_stats_config_t stats_config;
	struct af_altek_report_t report_data;
	struct allib_af_hw_stats_t af_stats; /* TBD */
	struct af_altek_vcm_tune_info vcm_tune;
	struct caf_alg_result caf_result;
};

/************************************ INTERNAK DECLARATION ************************************/
static cmr_int afaltek_adpt_proc_out_report(cmr_handle adpt_handle,
					    struct allib_af_output_report_t *report_in,
					    void *report_out);

static cmr_u8 afaltek_adpt_set_pos(cmr_handle adpt_handle, cmr_s16 dac, cmr_u8 sensor_id);
static cmr_int afaltek_adpt_config_roi(cmr_handle adpt_handle,
				       struct isp_af_win *roi_in,
				       cmr_int roi_type,
				       struct allib_af_input_roi_info_t *roi_out);
static cmr_int afaltek_adpt_pre_start(cmr_handle adpt_handle,
				      struct allib_af_input_roi_info_t *roi);
static cmr_int afaltek_adpt_caf_stop(cmr_handle adpt_handle);
static cmr_int afaltek_adpt_reset_caf(cmr_handle adpt_handle);

/************************************ INTERNAK FUNCTION ***************************************/

static cmr_int load_altek_library(cmr_handle adpt_handle)
{
	cmr_int ret = -1;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	cxt->altek_lib_handle = dlopen(LIBRARY_PATH, RTLD_NOW);
	if (!cxt->altek_lib_handle) {
		ISP_LOGE("failed to dlopen");
		goto error_dlopen;
	}
	cxt->lib_api.af_altek_version = dlsym(cxt->altek_lib_handle, "allib_af_get_lib_ver");
	if (!cxt->lib_api.af_altek_version) {
		ISP_LOGE("failed to dlsym version");
		goto error_dlsym;
	}
	cxt->lib_api.af_altek_load_func = dlsym(cxt->altek_lib_handle, "allib_af_load_func");
	if (!cxt->lib_api.af_altek_load_func) {
		ISP_LOGE("failed to dlsym load");
		goto error_dlsym;
	}

	return 0;
error_dlsym:
	dlclose(cxt->altek_lib_handle);
	cxt->altek_lib_handle = NULL;
error_dlopen:
	return ret;
}

static void unload_altek_library(cmr_handle adpt_handle)
{
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if (cxt->altek_lib_handle) {
		dlclose(cxt->altek_lib_handle);
		cxt->altek_lib_handle = NULL;
	}
}

static cmr_int load_caf_library(cmr_handle adpt_handle)
{
	cmr_int ret = -1;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	cxt->caf_lib_handle = dlopen(CAF_LIBRARY_PATH, RTLD_NOW);
	if (!cxt->caf_lib_handle) {
		ISP_LOGE("failed to dlopen");
		goto error_dlopen;
	}
	cxt->caf_ops.trigger_init = dlsym(cxt->caf_lib_handle, "caf_trigger_init");
	if (!cxt->caf_ops.trigger_init) {
		ISP_LOGE("failed to dlsym trigger init");
		goto error_dlsym;
	}
	cxt->caf_ops.trigger_deinit = dlsym(cxt->caf_lib_handle, "caf_trigger_deinit");
	if (!cxt->caf_ops.trigger_deinit) {
		ISP_LOGE("failed to dlsym trigger deinit");
		goto error_dlsym;
	}
	cxt->caf_ops.trigger_calc = dlsym(cxt->caf_lib_handle, "caf_trigger_calculation");
	if (!cxt->caf_ops.trigger_calc) {
		ISP_LOGE("failed to dlsym trigger calc");
		goto error_dlsym;
	}
	cxt->caf_ops.trigger_ioctrl = dlsym(cxt->caf_lib_handle, "caf_trigger_ioctrl");
	if (!cxt->caf_ops.trigger_ioctrl) {
		ISP_LOGE("failed to dlsym trigger ioctrl");
		goto error_dlsym;
	}

	return 0;
error_dlsym:
	dlclose(cxt->caf_lib_handle);
	cxt->caf_lib_handle = NULL;
error_dlopen:
	return ret;
}

static void unload_caf_library(cmr_handle adpt_handle)
{
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if (cxt->caf_lib_handle) {
		dlclose(cxt->caf_lib_handle);
		cxt->caf_lib_handle = NULL;
	}
}

static void afaltek_adpt_get_version(cmr_handle adpt_handle)
{
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_version_t version = { 0x00 };
	float wrapper_ver = 0.0;

	cxt->lib_api.af_altek_version(&version);
	al3awrapperaf_get_version(&wrapper_ver);

	ISP_LOGI("version main : %d sub : %d, wraper : %f", version.m_uw_main_ver, version.m_uw_sub_ver, wrapper_ver);
}

static cmr_int afaltek_adpt_load_ops(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_ops_t af_run_obj = { 0x00 };

	if (cxt->lib_api.af_altek_load_func(&af_run_obj)) {
		cxt->ops.init = af_run_obj.initial;
		cxt->ops.deinit = af_run_obj.deinit;
		cxt->ops.set_parameters = af_run_obj.set_param;
		cxt->ops.get_parameters = af_run_obj.get_param;
		cxt->ops.process = af_run_obj.process;
		ret = ISP_SUCCESS;
	}

	return ret;
}

static cmr_int afaltek_adpt_set_parameters(cmr_handle adpt_handle,
					   struct allib_af_input_set_param_t *p)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if (cxt->ops.set_parameters(p, &cxt->af_out_obj, cxt->af_runtime_obj)) {
		if (cxt->af_out_obj.result) {
			ret = afaltek_adpt_proc_out_report(cxt, &cxt->af_out_obj, NULL);
			ISP_LOGI("need repot result type = %d ret = %ld", p->type, ret);
		} else {
			ret = ISP_SUCCESS;
		}
	} else {
		ISP_LOGE("failed to lib set param");
	}

	return ret;
}

static cmr_int afaltek_adpt_get_parameters(cmr_handle adpt_handle,
					   struct allib_af_input_get_param_t *p)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if (cxt->ops.get_parameters(p, &cxt->af_out_obj, cxt->af_runtime_obj)) {
		ret = ISP_SUCCESS;
	} else {
		ISP_LOGE("failed to lib get param");
	}

	return ret;
}

static cmr_int afaltek_adpt_set_nothing(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UNKNOWN;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_cali_data(cmr_handle adpt_handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	if (!data) {
		ISP_LOGE("otp data is null");
		return -ISP_PARAM_NULL;
	}

	p.type = alAFLIB_SET_PARAM_SET_CALIBDATA;
	memcpy(&p.u_set_data.init_info, data, sizeof(struct allib_af_input_init_info_t));
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_setting_file(cmr_handle adpt_handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };
	struct allib_af_input_initial_set_t *init_set = (struct allib_af_input_initial_set_t *)data;

	if (!init_set || !init_set->p_initial_set_data) {
		ISP_LOGE("setting file data is null data = %p", data);
		return -ISP_PARAM_NULL;
	}

	p.type = alAFLIB_SET_PARAM_SET_SETTING_FILE;
	p.u_set_data.init_set = *init_set;
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_param_init(cmr_handle adpt_handle,
					   cmr_s32 initialized)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_INIT;
	p.u_set_data.afctrl_initialized = initialized;
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_roi(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_roi_info_t *roi = (struct allib_af_input_roi_info_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_SET_ROI;
	p.u_set_data.roi_info = *roi;
	ISP_LOGI("roi_updated = %d type = %d frame_id = %d num_roi = %d",
			p.u_set_data.roi_info.roi_updated,
			p.u_set_data.roi_info.type,
			p.u_set_data.roi_info.frame_id,
			p.u_set_data.roi_info.num_roi);

	ISP_LOGI("top = %d left = %d dx = %d dy = %d",
			p.u_set_data.roi_info.roi[0].uw_top,
			p.u_set_data.roi_info.roi[0].uw_left,
			p.u_set_data.roi_info.roi[0].uw_dx,
			p.u_set_data.roi_info.roi[0].uw_dy);

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_u8 afaltek_adpt_lock_ae_awb(cmr_handle adpt_handle, cmr_int lock)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if (ISP_AE_AWB_LOCK == lock)
		cxt->ae_awb_lock_cnt++;
	else if (ISP_AE_AWB_UNLOCK == lock)
		cxt->ae_awb_lock_cnt--;
	ISP_LOGI("lock = %ld, ae_awb_lock_cnt = %ld", lock, cxt->ae_awb_lock_cnt);
	if (0 > cxt->ae_awb_lock_cnt) {
		cxt->ae_awb_lock_cnt = 0; /* TBD */
		return 0;
	}
	/* call af ctrl callback to lock ae & awb */
	if (cxt->cb_ops.lock_ae_awb) {
		ret = cxt->cb_ops.lock_ae_awb(cxt->caller_handle, &lock);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_u8 afaltek_adpt_config_af_stats(cmr_handle adpt_handle, void *data)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	ISP_LOGI("E");
	if (cxt->cb_ops.cfg_af_stats) {
		ret = cxt->cb_ops.cfg_af_stats(cxt->caller_handle, data);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afaltek_adpt_start_notify(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	ISP_LOGI("E");
	if (cxt->cb_ops.start_notify) {
		ret = cxt->cb_ops.start_notify(cxt->caller_handle, NULL);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afaltek_adpt_set_start(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };
	cmr_int status = 0;

	p.type = alAFLIB_SET_PARAM_AF_START;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	ISP_LOGI("ret = %ld", ret);
	status = cxt->af_out_obj.focus_status.t_status;
	if (alAFLib_STATUS_FOCUSING != status) {
		ISP_LOGI("failed to start af focus_status = %ld", status);
		ret = -ISP_ERROR;
	}

	return ret;
}

static cmr_int afaltek_adpt_stop(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };
	cmr_int status = 0;

	p.type = alAFLIB_SET_PARAM_CANCEL_FOCUS;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	ISP_LOGI("ret = %ld", ret);
	status = cxt->af_out_obj.focus_status.t_status;
	if (alAFLib_STATUS_UNKNOWN != status) {
		ISP_LOGI("failed to stop af focus_status = %ld", status);
		ret = -ISP_ERROR;
	}

	return ret;
}

static cmr_int afaltek_adpt_caf_set_mode(cmr_handle adpt_handle, cmr_int mode)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	/* for caf trigger */
	cxt->caf_ops.trigger_ioctrl(cxt->caf_trigger_handle,
				    AF_ALG_CMD_SET_AF_MODE, &mode, NULL);

	return ret;
}

static void afaltek_adpt_config_mode(cmr_int mode_in, cmr_int *mode_out)
{
	switch (mode_in) {
	case ISP_FOCUS_NONE:
		*mode_out = AF_CTRL_MODE_AUTO;
		break;
	case ISP_FOCUS_TRIG:
		*mode_out = AF_CTRL_MODE_AUTO;
		break;
	case ISP_FOCUS_ZONE:
		*mode_out = AF_CTRL_MODE_AUTO;
		break;
	case ISP_FOCUS_MULTI_ZONE:
		*mode_out = AF_CTRL_MODE_AUTO;
	case ISP_FOCUS_MACRO:
		*mode_out = AF_CTRL_MODE_MANUAL;
		break;
	case ISP_FOCUS_WIN:
		*mode_out = AF_CTRL_MODE_CAF;
		break;
	case ISP_FOCUS_CONTINUE:
		*mode_out = AF_CTRL_MODE_CAF;
		break;
	case ISP_FOCUS_MANUAL:
		*mode_out = AF_CTRL_MODE_MANUAL;
		break;
	case ISP_FOCUS_VIDEO:
		*mode_out = AF_CTRL_MODE_CONTINUOUS_VIDEO;
		break;
	case ISP_FOCUS_BYPASS:
		*mode_out = AF_CTRL_MODE_MANUAL;
		break;
	default:
		*mode_out = AF_CTRL_MODE_AUTO;
		ISP_LOGE("oem send a wrong mode");
		break;

	}
}

static cmr_int afaltek_adpt_set_mode(cmr_handle adpt_handle, cmr_int mode)
{
	cmr_int ret = -1;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };
	cmr_int ctrl_mode = 0;

	p.type = alAFLIB_SET_PARAM_FOCUS_MODE;
	p.u_set_data.focus_mode_type = alAFLib_AF_MODE_AUTO;

	afaltek_adpt_config_mode(mode, &ctrl_mode);
	ISP_LOGI("mode = %ld ctrl_mode = %ld", mode, ctrl_mode);
	cxt->af_mode = ctrl_mode;

	switch (ctrl_mode) {
	case AF_CTRL_MODE_MACRO:
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_MACRO;
		ret = afaltek_adpt_set_parameters(cxt, &p);
		break;
	case AF_CTRL_MODE_AUTO:
#ifdef FEATRUE_SPRD_CAF_TRIGGER
		afaltek_adpt_caf_stop(cxt);
#endif
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_AUTO;
		ret = afaltek_adpt_set_parameters(cxt, &p);
		break;
	case AF_CTRL_MODE_CAF:
#ifdef FEATRUE_SPRD_CAF_TRIGGER
		afaltek_adpt_caf_set_mode(cxt, AF_ALG_MODE_CONTINUE);
		afaltek_adpt_reset_caf(cxt);
#else
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_CONTINUOUS_PICTURE;
		ret = afaltek_adpt_set_parameters(cxt, &p);
#endif
		break;
/*
	case AF_CTRL_MODE_INFINITY:
		p.u_set_data.focus_mode_type = AF_MODE_INFINITY;
		break;
*/
	case AF_CTRL_MODE_MANUAL:
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_MANUAL;
		ret = afaltek_adpt_set_parameters(cxt, &p);
		break;
/*
	case AF_CTRL_MODE_AUTO_VIDEO:
		p.u_set_data.focus_mode_type = AF_MODE_AUTO_VIDEO;
		break;
*/
	case AF_CTRL_MODE_CONTINUOUS_VIDEO:
#ifdef FEATRUE_SPRD_CAF_TRIGGER
		afaltek_adpt_caf_set_mode(cxt, AF_ALG_MODE_CONTINUE);
		afaltek_adpt_reset_caf(cxt);
#else
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_CONTINUOUS_VIDEO;
		ret = afaltek_adpt_set_parameters(cxt, &p);
#endif
		break;
	case AF_CTRL_MODE_CONTINUOUS_PICTURE_HYBRID:
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_HYBRID_CONTINUOUS_PICTURE;
		ret = afaltek_adpt_set_parameters(cxt, &p);
		break;
	case AF_CTRL_MODE_CONTINUOUS_VIDEO_HYBRID:
		p.u_set_data.focus_mode_type =
		    alAFLib_AF_MODE_HYBRID_CONTINUOUS_VIDEO;
		ret = afaltek_adpt_set_parameters(cxt, &p);
		break;
	case AF_CTRL_MODE_AUTO_HYBRID:
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_HYBRID_AUTO;
		ret = afaltek_adpt_set_parameters(cxt, &p);
		break;
/*
	case AF_CTRL_MODE_AUTO_INSTANT_HYBRID:
		p.u_set_data.focus_mode_type = AF_MODE_AUTO_INSTANT_HYBRID;
		break;
*/
	case AF_CTRL_MODE_BYPASS:
		p.u_set_data.focus_mode_type = alAFLib_AF_MODE_MANUAL;
		ret = afaltek_adpt_set_parameters(cxt, &p);
		break;
	default:
		ISP_LOGE("error mode");
		break;
	}

	return ret;
}

static cmr_int afaltek_adpt_update_lens_info(cmr_handle adpt_handle,
					     struct allib_af_input_lens_info_t *lens_info)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UPDATE_LENS_INFO;
	p.u_set_data.lens_info = *lens_info;
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_reset_lens(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_RESET_LENS_POS;
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_move_lens_info(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_move_lens_cb_t *move_lens_info = (struct allib_af_input_move_lens_cb_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_SET_LENS_MOVE_CB;

	p.u_set_data.move_lens = *move_lens_info;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_tuning_enable(cmr_handle adpt_handle,
					      cmr_s32 enable)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_TUNING_ENABLE;
	p.u_set_data.al_af_tuning_enable = enable;
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_update_tuning_file(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UPDATE_TUNING_PTR;
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_update_ae_info(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_aec_info_t *ae_info = (struct allib_af_input_aec_info_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UPDATE_AEC_INFO;
	p.u_set_data.aec_info = *ae_info;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_update_awb_info(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_awb_info_t *awb_info = (struct allib_af_input_awb_info_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UPDATE_AWB_INFO;
	p.u_set_data.awb_info = *awb_info;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_update_sensor_info(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_sensor_info_t *sensor_info = (struct allib_af_input_sensor_info_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UPDATE_SENSOR_INFO;
	p.u_set_data.sensor_info = *sensor_info;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_update_gyro_info(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_gyro_info_t *gyro_info = (struct allib_af_input_gyro_info_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UPDATE_GYRO_INFO;
	p.u_set_data.gyro_info = *gyro_info;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_update_gsensor(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_gravity_vector_t *gsensor_info = (struct allib_af_input_gravity_vector_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_UPDATE_GRAVITY_VECTOR;
	p.u_set_data.gravity_info = *gsensor_info;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_update_isp_info(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct af_ctrl_isp_info_t *in_isp_info = (struct af_ctrl_isp_info_t *) in;
	struct allib_af_input_set_param_t p = { 0x00 };

	if (!in) {
		ISP_LOGE("isp info is null");
		return -ISP_PARAM_NULL;
	}

	p.type = alAFLIB_SET_PARAM_UPDATE_ISP_INFO;
	p.u_set_data.isp_info.liveview_img_sz.uw_width = in_isp_info->img_width;
	p.u_set_data.isp_info.liveview_img_sz.uw_height = in_isp_info->img_height;

	ISP_LOGI("uw_width = %d", p.u_set_data.isp_info.liveview_img_sz.uw_width);
	ISP_LOGI("uw_height= %d", p.u_set_data.isp_info.liveview_img_sz.uw_height);

	ret = afaltek_adpt_set_parameters(cxt, &p);
	ISP_LOGI("ret = %ld", ret);
	return ret;
}

static cmr_int afaltek_adpt_vcm_tuning_param(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	cmr_s8 value[PROPERTY_VALUE_MAX];
	cmr_s8 pos[PROPERTY_VALUE_MAX];
	cmr_s16 position = 0;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	memset(value, '\0', sizeof(value));
	property_get("persist.sys.isp.vcm.tuning.mode", (char *)value, "0");

	if (1 == atoi((char *)value)) {
		memset(pos, '\0', sizeof(pos));
		property_get("persist.sys.isp.vcm.position", (char *)pos, "0");
		position = atoi((char *)pos);

		if (position != cxt->vcm_tune.cur_pos) {
			ret = afaltek_adpt_set_pos(adpt_handle, position, 0);
			if (!ret) {
				cxt->vcm_tune.cur_pos = position;
				cxt->vcm_tune.tuning_enable = 1;
				ISP_LOGI("VCM TUNING MODE position %d", position);
			}
		}
	} else {
		cxt->vcm_tune.tuning_enable = 0;
	}

	return ret;

}

static cmr_int afaltek_adpt_update_sof(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct af_ctrl_sof_info *sof_info = (struct af_ctrl_sof_info *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	ret = afaltek_adpt_vcm_tuning_param(adpt_handle);
	if (ret) {
		ISP_LOGE("set vcm tuning position error");
	}

	p.type = alAFLIB_SET_PARAM_UPDATE_SOF;

	p.u_set_data.sof_id.sof_frame_id = sof_info->sof_frame_idx;
	p.u_set_data.sof_id.sof_time.time_stamp_sec = sof_info->timestamp.sec;
	p.u_set_data.sof_id.sof_time.time_stamp_us = sof_info->timestamp.usec;

	ISP_LOGI("sof_frame_idx = %d, sec = %d, usec = %d",
		 sof_info->sof_frame_idx,
		 sof_info->timestamp.sec, sof_info->timestamp.usec);
	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static void afaltek_ae_info_to_af_lib(struct isp3a_ae_info *ae_info,
				      struct allib_af_input_aec_info_t *af_ae_info)
{
	af_ae_info->ae_settled = ae_info->report_data.ae_converge_st;
	af_ae_info->cur_intensity = (float)(ae_info->report_data.cur_mean);
	af_ae_info->target_intensity = (float)(ae_info->report_data.target_mean);
	af_ae_info->brightness = (float)(ae_info->report_data.BV);
	af_ae_info->cur_gain = (float)(ae_info->report_data.sensor_ad_gain);
	af_ae_info->exp_time = (float)(ae_info->report_data.exp_time);
	af_ae_info->preview_fr = ae_info->report_data.fps;
}

static cmr_int afaltek_adpt_caf_process(cmr_handle adpt_handle,
					struct caf_alg_calc_param *caf_in)
{
	cmr_int ret = ISP_SUCCESS;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct isp_af_win roi;
	struct allib_af_input_roi_info_t lib_roi;
	struct caf_alg_result caf_out;

	cmr_bzero(&roi, sizeof(roi));
	cmr_bzero(&lib_roi, sizeof(lib_roi));
	cmr_bzero(&caf_out, sizeof(caf_out));

	ret = cxt->caf_ops.trigger_calc(cxt->caf_trigger_handle, caf_in, &caf_out);

	ISP_LOGI("caf_out.is_caf_trig %d", caf_out.is_caf_trig);
	if ((!cxt->caf_result.is_caf_trig) && caf_out.is_caf_trig) {
		/* notify oem to show box */
		ret = afaltek_adpt_start_notify(adpt_handle);
		if (ret)
			ISP_LOGE("failed to notify");
		afaltek_adpt_config_roi(adpt_handle, &roi,
					alAFLib_ROI_TYPE_TOUCH, &lib_roi);
		ret = afaltek_adpt_pre_start(adpt_handle, &lib_roi);
		ret = cxt->caf_ops.trigger_ioctrl(cxt->caf_trigger_handle,
						  AF_ALG_CMD_SET_CAF_STOP,
						  NULL, NULL);
	}
	cxt->caf_result = caf_out;

	return ret;
}

static cmr_int afaltek_adpt_caf_update_ae_info(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = ISP_SUCCESS;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct caf_alg_calc_param caf_in = { 0};
	struct isp3a_ae_info *isp_ae_info = (struct isp3a_ae_info *)in;
	struct isp_ae_statistic_info *ae_rgb_stats = NULL;

	ae_rgb_stats = (struct isp_ae_statistic_info *)isp_ae_info->report_data.rgb_stats;
	if (NULL == ae_rgb_stats) {
		ISP_LOGE("failed to get ae rgb stats if is the first that's ok.");
		return -ISP_ERROR;
	}

	caf_in.active_data_type = AF_ALG_DATA_IMG_BLK;
	caf_in.img_blk_info.block_w = 16;
	caf_in.img_blk_info.block_h = 16;
	caf_in.img_blk_info.data = (cmr_u32 *)ae_rgb_stats;
	caf_in.ae_info.exp_time = (cmr_u32)(isp_ae_info->report_data.exp_time * 100);
	caf_in.ae_info.gain = isp_ae_info->report_data.sensor_ad_gain;
	caf_in.ae_info.cur_lum = isp_ae_info->report_data.cur_mean;
	caf_in.ae_info.target_lum = isp_ae_info->report_data.target_mean;
	caf_in.ae_info.is_stable = isp_ae_info->report_data.ae_converge_st;
	ret = afaltek_adpt_caf_process(cxt, &caf_in);
	return ret;
}

static cmr_int afaltek_adpt_update_ae(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_aec_info_t ae_info = { 0x00 };
	struct isp3a_ae_info *isp_ae_info = (struct isp3a_ae_info *)in;

	afaltek_ae_info_to_af_lib(isp_ae_info, &ae_info);
	ret = afaltek_adpt_update_ae_info(cxt, &ae_info);
	if (ret)
		ISP_LOGE("failed to update ae info");

	cxt->ae_status_info.ae_converged = ae_info.ae_settled;
	cxt->ae_status_info.ae_locked = (AE_LOCKED == isp_ae_info->report_data.ae_state) ? 1 : 0;

	ISP_LOGI("ae_converged = %ld ae_locked = %d ae_state = %d",
			cxt->ae_status_info.ae_converged,
			cxt->ae_status_info.ae_locked,
			isp_ae_info->report_data.ae_state);

	return ret;
}

static cmr_int afaltek_adpt_update_awb(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_awb_info_t awb_info = { 0x00 };
	ISP_LOGI("E");

	//memcpy(&awb_info, in, sizeof(awb_info)); /* TBD */
	ret = 0;//afaltek_adpt_update_awb_info(cxt, &awb_info);
	return ret;
}

static cmr_int afaltek_adpt_update_aux_sensor(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct af_ctrl_aux_sensor_info_t *aux_sensor_info = (struct af_ctrl_aux_sensor_info_t *)in;
	struct caf_alg_calc_param caf_in;

	memset((void*)&caf_in, 0, sizeof(caf_in));
#if 1 /* TBD */
	{
		struct af_alg_sensor_info *gryo_info = (struct af_alg_sensor_info *)in;
		ISP_LOGI("gyro E");
		caf_in.sensor_info.sensor_type = AF_POSTURE_GYRO;
		caf_in.sensor_info.x = gryo_info->x;
		caf_in.sensor_info.y = gryo_info->y;
		caf_in.sensor_info.z = gryo_info->z;
	}
#else
	switch(aux_sensor_info->sensor_type) {
	case AF_CTRL_ACCELEROMETER:
		ISP_LOGI("accelerometer E");
		break;
	case AF_CTRL_MAGNETIC_FIELD:
		ISP_LOGI("magnetic field E");
		break;
	case AF_CTRL_GYROSCOPE:
	{
		struct af_alg_sensor_info *gryo_info = (struct af_alg_sensor_info *)in;
		ISP_LOGI("gyro E");
		caf_in.sensor_info.sensor_type = AF_POSTURE_GYRO;
		caf_in.sensor_info.x = gryo_info->x;
		caf_in.sensor_info.y = gryo_info->y;
		caf_in.sensor_info.z = gryo_info->z;
	}
		break;
	case AF_CTRL_LIGHT:
		ISP_LOGI("light E");
		break;
	case AF_CTRL_PROXIMITY:
		ISP_LOGI("proximity E");
		break;
	default:
		ISP_LOGI("sensor type not support");
		break;
	}
#endif
	caf_in.active_data_type = AF_ALG_DATA_SENSOR;
	ret = afaltek_adpt_caf_process(cxt, &caf_in);

	return ISP_SUCCESS;
}

static cmr_int afaltek_adpt_wait_flash(cmr_handle adpt_handle,
				       cmr_s32 wait_flash)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_WAIT_FOR_FLASH;

	p.u_set_data.wait_for_flash = wait_flash;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_lock_caf(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_LOCK_CAF;

	p.u_set_data.lock_caf = *(cmr_s32 *) in;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_caf_stop(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

#ifdef FEATRUE_SPRD_CAF_TRIGGER
	ret = cxt->caf_ops.trigger_ioctrl(cxt->caf_trigger_handle,
				    AF_ALG_CMD_SET_CAF_STOP, NULL, NULL);
#endif
	return ret;
}

static cmr_int afaltek_adpt_reset_caf(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
#ifdef FEATRUE_SPRD_CAF_TRIGGER
	ret = cxt->caf_ops.trigger_ioctrl(cxt->caf_trigger_handle,
				    AF_ALG_CMD_SET_CAF_RESET, NULL, NULL);
#else
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_RESET_CAF;

	ret = afaltek_adpt_set_parameters(cxt, &p);
#endif
	return ret;
}

static cmr_int afaltek_adpt_hybird_af_enable(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_enable_hybrid_t *hybrid = (struct allib_af_input_enable_hybrid_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_HYBIRD_AF_ENABLE;
	p.u_set_data.haf_info = *hybrid;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_imgbuf(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_image_buf_t *imgbuf = (struct allib_af_input_image_buf_t *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_SET_IMGBUF;
	p.u_set_data.img_buf = *imgbuf;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_reset_af_setting(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_SET_DEFAULT_SETTING;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_set_trigger_stats(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	return ret;
}

static cmr_int afaltek_adpt_set_special_event(cmr_handle adpt_handle, void *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_special_event *event = (struct allib_af_input_special_event *)in;
	struct allib_af_input_set_param_t p = { 0x00 };

	p.type = alAFLIB_SET_PARAM_SPECIAL_EVENT;
	p.u_set_data.special_event = *event;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_u8 afaltek_adpt_set_pos(cmr_handle adpt_handle, cmr_s16 dac, cmr_u8 sensor_id)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct af_ctrl_motor_pos pos_info = { 0x00 };
	pos_info.motor_pos = dac;
	pos_info.vcm_wait_ms = cxt->motor_info.vcm_wait_ms;
	cxt->motor_info.motor_pos = dac;
	UNUSED(sensor_id);

	/* call af ctrl callback to move lens */
	if (cxt->cb_ops.set_pos) {
		ret = cxt->cb_ops.set_pos(cxt->caller_handle, &pos_info);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afaltek_adpt_end_notify(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct af_result_param af_result = { 0 };

	af_result.motor_pos = cxt->motor_info.motor_pos;
	af_result.suc_win = 1; /* TBD */

	if (cxt->cb_ops.end_notify) {
		ret = cxt->cb_ops.end_notify(cxt->caller_handle, &af_result);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_u8 afaltek_adpt_get_timestamp(cmr_handle adpt_handle, cmr_u32 * sec, cmr_u32 * usec)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if (cxt->cb_ops.get_system_time) {
		ret = cxt->cb_ops.get_system_time(cxt->caller_handle, sec, usec);
	} else {
		ISP_LOGE("cb is null");
		ret = -ISP_CALLBACK_NULL;
	}

	return ret;
}

static cmr_int afaltek_adpt_config_roi(cmr_handle adpt_handle,
				       struct isp_af_win *roi_in,
				       cmr_int roi_type,
				       struct allib_af_input_roi_info_t *roi_out)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	cmr_u8 i = 0;

	roi_out->roi_updated = 1;
	roi_out->type = roi_type;	/* TBD */

	roi_out->frame_id = cxt->frame_id;
	/* only support value 1 */
	roi_out->num_roi = roi_in->valid_win;
	ISP_LOGI("roi_out->num_roi = %d", roi_out->num_roi);
	/* only support array 0 */
	for (i = 0; i < roi_in->valid_win; i++) {
		roi_out->roi[i].uw_left = roi_in->win[i].start_x;
		roi_out->roi[i].uw_top = roi_in->win[i].start_y;
		roi_out->roi[i].uw_dx = roi_in->win[i].end_x - roi_in->win[i].start_x;
		roi_out->roi[i].uw_dy = roi_in->win[i].end_y - roi_in->win[i].start_y;
		ISP_LOGI("top = %d, left = %d, dx = %d, dy = %d",
			 roi_out->roi[i].uw_top, roi_out->roi[i].uw_left,
			 roi_out->roi[i].uw_dx, roi_out->roi[i].uw_dy);
	}
	roi_out->weight[0] = 1;
	roi_out->src_img_sz.uw_width = 2304;	/* TBD */
	roi_out->src_img_sz.uw_height = 1740;

	return 0;
}

static cmr_int afaltek_adpt_pre_start(cmr_handle adpt_handle,
				      struct allib_af_input_roi_info_t *roi)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if ((AF_ADPT_STARTED == cxt->af_cur_status)
		|| (AF_ADPT_FOCUSING == cxt->af_cur_status)) {
		ret = afaltek_adpt_stop(cxt);
		if (ret)
			ISP_LOGE("failed to stop");
	}

	//ret = afaltek_adpt_set_mode(cxt, AF_CTRL_MODE_AUTO);	/* TBD */
	ret = afaltek_adpt_set_roi(adpt_handle, roi);
	if (ret)
		ISP_LOGE("failed to set roi");

	cxt->af_cur_status = AF_ADPT_STARTED;
	afaltek_adpt_get_timestamp(cxt,
				   &cxt->ae_status_info.timestamp.sec,
				   &cxt->ae_status_info.timestamp.usec);
	ISP_LOGI("sec = %d, usec = %d", cxt->ae_status_info.timestamp.sec, cxt->ae_status_info.timestamp.usec);
	return ret;
}

static cmr_int afaltek_adpt_post_start(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	ISP_LOGI("E");
	cxt->af_cur_status = AF_ADPT_FOCUSING;

	ret = afaltek_adpt_set_start(adpt_handle);
	if (ret)
		ISP_LOGE("failed to start");

	afaltek_adpt_lock_ae_awb(cxt, ISP_AE_AWB_LOCK);
	if (ret)
		ISP_LOGE("failed to lock ret = %ld", ret);

	return ret;
}

static cmr_int afaltek_adpt_proc_start(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	cmr_u32 sec = 0;
	cmr_u32 usec = 0;
	cmr_uint time_delta = 0;

	if (AF_ADPT_STARTED == cxt->af_cur_status) {
		if (cxt->ae_status_info.ae_converged) {
			afaltek_adpt_post_start(cxt);
		} else {
			afaltek_adpt_get_timestamp(cxt, &sec, &usec);
			time_delta =
			    (sec * SEC_TO_US + usec) -
			    (cxt->ae_status_info.timestamp.sec * SEC_TO_US +
			     cxt->ae_status_info.timestamp.usec);
			ISP_LOGI("time_delta = %lu", time_delta);
			if (AE_CONVERGE_TIMEOUT <= time_delta)
				afaltek_adpt_post_start(cxt);
		}
	} else if (AF_ADPT_FOCUSING == cxt->af_cur_status) {
		if (cxt->ae_status_info.ae_locked) {
			struct allib_af_input_special_event event;
			cmr_bzero(&event, sizeof(event));
			event.flag = 1;
			event.type = alAFLib_AE_IS_LOCK;
			ret = afaltek_adpt_set_special_event(cxt, &event);
			cxt->af_cur_status = AF_ADPT_FOCUSED;
			ISP_LOGI("cxt->af_cur_status = %d", cxt->af_cur_status);
		}
	}

	return ret;
}

/* TBD */
static cmr_int afaltek_adpt_move_lens(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_set_param_t p = { 0x00 };

	//p.type = alAFLIB_SET_PARAM_MOVE_LENS;

	ret = afaltek_adpt_set_parameters(cxt, &p);
	return ret;
}

static cmr_int afaltek_adpt_lens_move_done(cmr_handle adpt_handle,
					   struct af_ctrl_motor_pos *pos_info)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_lens_info_t lens_info;

	cmr_bzero(&lens_info, sizeof(lens_info));

	lens_info.lens_pos_updated = 1;
	lens_info.lens_pos = pos_info->motor_pos;
	lens_info.lens_status = LENS_MOVE_DONE;
	ret = afaltek_adpt_update_lens_info(adpt_handle, &lens_info);

	return ret;
}

static cmr_int afaltek_adpt_caf_reset_after_af(cmr_handle adpt_handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	if ((AF_CTRL_MODE_CAF == cxt->af_mode) ||
	     (AF_CTRL_MODE_CONTINUOUS_VIDEO == cxt->af_mode)) {
		ret = afaltek_adpt_reset_caf(cxt);
	}

	return ret;
}

static cmr_int afaltek_adpt_af_done(cmr_handle adpt_handle, cmr_int success)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_special_event event;

	ISP_LOGI("E success = %ld", success);

	/*TBD no flash */
	//if (no-flash)
	ret = afaltek_adpt_lock_ae_awb(cxt, ISP_AE_AWB_UNLOCK);
	if (ret)
	    ISP_LOGI("failed to unlock ret = %ld", ret);

	cmr_bzero(&event, sizeof(event));
	event.flag = 0;
	event.type = alAFLib_AE_IS_LOCK;
	ret = afaltek_adpt_set_special_event(cxt, &event);
	if (ret)
	    ISP_LOGI("failed to set special event %ld", ret);
	ret = afaltek_adpt_end_notify(cxt);
	if (ret)
	    ISP_LOGI("failed to end notify ret = %ld", ret);

	ret = afaltek_adpt_caf_reset_after_af(cxt);
	if (ret)
		ISP_LOGI("failed to caf reset ret = %ld", ret);
	cxt->af_cur_status = AF_ADPT_DONE;
	return ret;
}

static cmr_int afaltek_adpt_inctrl(cmr_handle adpt_handle, cmr_int cmd,
				   void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	UNUSED(out);

	ISP_LOGI("cmd = %ld", cmd);

	switch (cmd) {
	case AF_CTRL_CMD_SET_DEBUG:
		ret = afaltek_adpt_set_nothing(adpt_handle);
		break;
	case AF_CTRL_CMD_SET_AF_MODE:
		ret = afaltek_adpt_set_mode(adpt_handle, *(cmr_int *) in);
		break;
	case AF_CTRL_CMD_SET_AF_POS:
		ret = afaltek_adpt_move_lens(adpt_handle);
		break;
	case AF_CTRL_CMD_SET_AF_POS_DONE:
		ret = afaltek_adpt_lens_move_done(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_AF_START:
		{
		struct allib_af_input_roi_info_t lib_roi;
		cmr_bzero(&lib_roi, sizeof(lib_roi));
		afaltek_adpt_config_roi(adpt_handle, in,
					alAFLib_ROI_TYPE_NORMAL, &lib_roi);
		ret = afaltek_adpt_pre_start(adpt_handle, &lib_roi);
		break;
		}
	case AF_CTRL_CMD_SET_AF_STOP:
		ret = afaltek_adpt_stop(adpt_handle);
		break;
	case AF_CTRL_CMD_SET_AF_RESTART:
		ret = afaltek_adpt_reset_af_setting(adpt_handle);
		break;
	case AF_CTRL_CMD_SET_TRIGGER_STATS:
		ret = afaltek_adpt_set_trigger_stats(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_CAF_RESET:
		ret = afaltek_adpt_reset_caf(adpt_handle);
		break;
	case AF_CTRL_CMD_SET_CAF_STOP:
		break;
	case AF_CTRL_CMD_SET_AF_DONE:
		break;
	case AF_CTRL_CMD_SET_AF_BYPASS:
		break;
	case AF_CTRL_CMD_SET_ROI:
		ret = afaltek_adpt_set_roi(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_GYRO_INFO:
		ret = afaltek_adpt_update_gyro_info(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_GSENSOR_INFO:
		ret = afaltek_adpt_update_gsensor(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_FLASH_NOTICE:
		break;
	case AF_CTRL_CMD_SET_TUNING_MODE:
		ret = afaltek_adpt_set_tuning_enable(adpt_handle, *(cmr_int *)in);
		break;
	case AF_CTRL_CMD_SET_SOF_FRAME_IDX:
		ret = afaltek_adpt_update_sof(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_UPDATE_AE:
		ret = afaltek_adpt_update_ae(adpt_handle, in);
		ret = afaltek_adpt_caf_update_ae_info(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_PROC_START:
		ret = afaltek_adpt_proc_start(adpt_handle);
		break;
	case AF_CTRL_CMD_SET_UPDATE_AWB:
		ret = afaltek_adpt_update_awb(adpt_handle, in);
		break;
	case AF_CTRL_CMD_SET_UPDATE_AUX_SENSOR:
		ret = afaltek_adpt_update_aux_sensor(adpt_handle, in);
		break;
	default:
		ISP_LOGE("failed to case cmd = %ld", cmd);
		break;
	}

	return ret;
}

static cmr_int afaltek_adpt_get_mode(cmr_handle adpt_handle, cmr_int *mode)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_get_param_t p = { 0x00 };

	p.type = alAFLIB_GET_PARAM_FOCUS_MODE;

	ret = afaltek_adpt_get_parameters(cxt, &p);
	*mode = p.u_get_data.af_focus_mode;

	return ret;
}

static cmr_int afaltek_adpt_get_default_pos(cmr_handle adpt_handle, cmr_int *pos)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_get_param_t p = { 0x00 };

	p.type = alAFLIB_GET_PARAM_DEFAULT_LENS_POS;

	ret = afaltek_adpt_get_parameters(cxt, &p);
	*pos = p.u_get_data.uw_default_lens_pos;

	return ret;
}

static cmr_int afaltek_adpt_get_cur_pos(cmr_handle adpt_handle, cmr_int *pos)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_get_param_t p = { 0x00 };

	p.type = alAFLIB_GET_PARAM_GET_CUR_LENS_POS;

	ret = afaltek_adpt_get_parameters(cxt, &p);
	*pos = p.u_get_data.w_current_lens_pos;

	return ret;
}

static cmr_int afaltek_adpt_get_nothing(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_get_param_t p = { 0x00 };

	p.type = alAFLIB_GET_PARAM_NOTHING;

	ret = afaltek_adpt_get_parameters(cxt, &p);

	return ret;
}

cmr_int afaltek_adpt_get_exif_info(cmr_handle adpt_handle, struct allib_af_get_data_info_t *out)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_get_param_t p = { 0x00 };

	p.type = alAFLIB_GET_PARAM_EXIF_INFO;

	ret = afaltek_adpt_get_parameters(cxt, &p);
	*out = p.u_get_data.exif_data_info;

	return ret;
}

cmr_int afaltek_adpt_get_debug_info(cmr_handle adpt_handle, struct allib_af_get_data_info_t *out)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_get_param_t p = { 0x00 };

	p.type = alAFLIB_GET_PARAM_DEBUG_INFO;

	ret = afaltek_adpt_get_parameters(cxt, &p);
	*out = p.u_get_data.debug_data_info;

	return ret;
}

static cmr_int afaltek_adpt_outctrl(cmr_handle adpt_handle, cmr_int cmd,
				    void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	UNUSED(in);

	ISP_LOGI("cmd = %ld", cmd);

	switch (cmd) {
	case AF_CTRL_CMD_GET_AF_MODE:
		ret = afaltek_adpt_get_mode(adpt_handle, (cmr_int *) out);
		break;
	case AF_CTRL_CMD_GET_DEFAULT_LENS_POS:
		ret = afaltek_adpt_get_default_pos(adpt_handle, (cmr_int *) out);
		break;
	case AF_CTRL_CMD_GET_AF_CUR_LENS_POS:
		ret = afaltek_adpt_get_cur_pos(adpt_handle, (cmr_int *) out);
		break;
	case AF_CTRL_CMD_GET_DEBUG:
		ret = afaltek_adpt_get_nothing(adpt_handle);
		break;
	case AF_CTRL_CMD_GET_EXIF_DEBUG_INFO:
		ret = afaltek_adpt_get_exif_info(adpt_handle, (struct allib_af_get_data_info_t *)out);
		break;
	case AF_CTRL_CMD_GET_DEBUG_INFO:
		ret = afaltek_adpt_get_debug_info(adpt_handle, (struct allib_af_get_data_info_t *)out);
		break;
	default:
		ISP_LOGE("failed to case cmd = %ld", cmd);
		break;
	}

	return ret;
}

static cmr_int afaltek_get_hw_config(struct isp3a_af_hw_cfg *out)
{
	cmr_int ret = -ISP_ERROR;
	struct alhw3a_af_cfginfo_t *af_cfg = (struct alhw3a_af_cfginfo_t *)out;

	ret = al3awrapperaf_getdefaultcfg(af_cfg);
	if (ret) {
		ISP_LOGE("failed to get hw ret = %ld", ret);
		ret = -ISP_ERROR;
	} else {
		ret = ISP_SUCCESS;
	}

	return ret;
}

static cmr_int afaltek_adpt_param_init(cmr_handle adpt_handle,
				       struct af_ctrl_init_in *in)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct allib_af_input_move_lens_cb_t move_lens_info = { 0 };
	struct allib_af_input_sensor_info_t sensor_info;
	struct allib_af_input_init_info_t init_info;
	struct sensor_otp_af_info *otp_af_info;

	otp_af_info = (struct sensor_otp_af_info *) in->otp_info.otp_data;
	memset(&init_info, 0x00, sizeof(init_info));
	if (otp_af_info) {
		init_info.calib_data.inf_step = otp_af_info->infinite_cali;
		init_info.calib_data.macro_step = otp_af_info->macro_cali;
		ISP_LOGI("infinite = %d, macro = %d",
			 init_info.calib_data.inf_step,
			 init_info.calib_data.macro_step);
	}
	init_info.module_info.f_number = in->module_info.f_num * 1.0 / 100;
	init_info.module_info.focal_lenth = in->module_info.focal_length * 10;
#if 1 //TBD
	init_info.calib_data.inf_distance = 20000;
	init_info.calib_data.macro_distance = 700;
	init_info.calib_data.mech_top = 1023;
	init_info.calib_data.mech_bottom = 0;
	init_info.calib_data.lens_move_stable_time = 20;
	init_info.calib_data.extend_calib_ptr = NULL;
	init_info.calib_data.extend_calib_data_size = 0;
	ISP_LOGI("f_number = %f focal_lenth = %f",
		 init_info.module_info.f_number,
		 init_info.module_info.focal_lenth);
#endif
	/* init otp */
	if (1) {
		cxt->motor_info.vcm_wait_ms = init_info.calib_data.lens_move_stable_time;
		ret = afaltek_adpt_set_cali_data(cxt, &init_info);
		if (ret)
			ISP_LOGI("ret = %ld", ret);
	} else {
		ISP_LOGI("there is no OTP in this module");
	}

	/* tuning file setting */
	ret = afaltek_adpt_set_setting_file(cxt, &in->tuning_info);
	if (ret)
		ISP_LOGI("failed to set tuning file ret = %ld", ret);

	/* register vcm callback */
	move_lens_info.uc_device_id = cxt->camera_id;
	move_lens_info.p_handle = (void *)cxt;
	move_lens_info.move_lens_cb = afaltek_adpt_set_pos;
	ret = afaltek_adpt_set_move_lens_info(cxt, &move_lens_info);
	if (ret)
		ISP_LOGI("failed to set move len info ret = %ld", ret);
#if 1 //TBD
	memset(&sensor_info, 0x00, sizeof(sensor_info));
	sensor_info.preview_img_sz.uw_width = in->isp_info.img_width;
	sensor_info.preview_img_sz.uw_height = in->isp_info.img_height;
	sensor_info.sensor_crop_info.uwx = 0;// in->sensor_info.crop_info.x;
	sensor_info.sensor_crop_info.uwy = 0;//in->sensor_info.crop_info.y;
	sensor_info.sensor_crop_info.dx = in->isp_info.img_width;
	sensor_info.sensor_crop_info.dy = in->isp_info.img_height;
#else
	/* set sensor info */
	cmr_bzero(&sensor_info, sizeof(sensor_info));
	sensor_info.preview_img_sz.uwWidth = in->sensor_info.sensor_res_width;
	sensor_info.preview_img_sz.uwHeight = in->sensor_info.sensor_res_height;
	sensor_info.sensor_crop_info.uwx = in->sensor_info.crop_info.x;
	sensor_info.sensor_crop_info.uwy = in->sensor_info.crop_info.y;
	sensor_info.sensor_crop_info.dx = in->sensor_info.crop_info.width;
	sensor_info.sensor_crop_info.dy = in->sensor_info.crop_info.height;
#endif
	ISP_LOGI("in->isp_info.img_width = %d, in->isp_info.img_height = %d",
			in->isp_info.img_width, in->isp_info.img_height);
	ISP_LOGI("sensor_res_width = %d, sensor_res_height = %d",
			in->sensor_info.sensor_res_width, in->sensor_info.sensor_res_height);
	ISP_LOGI("crop_info.x = %d, crop_info.y = %d crop_info.width = %d, crop_info.height = %d",
			in->sensor_info.crop_info.x, in->sensor_info.crop_info.y, in->sensor_info.crop_info.width, in->sensor_info.crop_info.height);
	ret = afaltek_adpt_update_sensor_info(cxt, &sensor_info);
	if (ret)
		ISP_LOGI("failed to update sensor info ret = %ld", ret);

	ISP_LOGI("isp info img_width = %d, img_height = %d", in->isp_info.img_width, in->isp_info.img_height);
	/* set isp info */
	ret = afaltek_adpt_update_isp_info(cxt, &in->isp_info);
	if (ret)
		ISP_LOGI("failed to update isp info ret = %ld", ret);

	/* sync init param */
	ret = afaltek_adpt_set_param_init(cxt, 1);
	if (ret)
		ISP_LOGI("failed to set param init ret = %ld", ret);

	return ret;
}

static cmr_int afaltek_libops_init(cmr_handle adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	ret = load_altek_library(cxt);
	if (ret) {
		ISP_LOGE("failed to load library");
		goto error_load_lib;
	}

	ret = afaltek_adpt_load_ops(cxt);
	if (ret) {
		ISP_LOGE("failed to load ops");
		goto error_load_ops;
	}

	ret = load_caf_library(cxt);
	if (ret) {
		ISP_LOGE("failed to load caf library");
		goto error_load_caf_lib;
	}

	return ret;

error_load_caf_lib:
error_load_ops:
	unload_altek_library(cxt);
error_load_lib:
	return ret;
}

static void afaltek_libops_deinit(cmr_handle adpt_handle)
{
	unload_caf_library(adpt_handle);
	unload_altek_library(adpt_handle);
}

static cmr_int afaltek_adpt_init(void *in, void *out, cmr_handle * adpt_handle)
{
	cmr_int ret = -ISP_ERROR;
	struct af_adpt_init_in *in_p = (struct af_adpt_init_in *)in;
	struct af_ctrl_init_out *out_p = (struct af_ctrl_init_out *)out;
	struct af_altek_context *cxt = NULL;

	cxt = (struct af_altek_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc cxt");
		ret = -ISP_ALLOC_ERROR;
		goto error_malloc;
	}

	cmr_bzero(cxt, sizeof(*cxt));
	cxt->inited = 0;
	cxt->caller_handle = in_p->caller_handle;
	cxt->camera_id = in_p->ctrl_in->camera_id;
	cxt->cb_ops.set_pos = in_p->cb_ctrl_ops.set_pos;
	cxt->cb_ops.start_notify = in_p->cb_ctrl_ops.start_notify;
	cxt->cb_ops.end_notify = in_p->cb_ctrl_ops.end_notify;
	cxt->cb_ops.lock_ae_awb = in_p->cb_ctrl_ops.lock_ae_awb;
	cxt->cb_ops.cfg_af_stats = in_p->cb_ctrl_ops.cfg_af_stats;
	cxt->cb_ops.get_system_time = in_p->cb_ctrl_ops.get_system_time;
	cxt->af_cur_status = AF_ADPT_IDLE;
	ret = afaltek_libops_init(cxt);
	if (ret) {
		ISP_LOGE("failed to init library and ops");
		goto error_libops_init;
	}

#if 1
	struct af_alg_tuning_block_param tt;
	tt.data = malloc(20);
	memset(tt.data, 0x00, 20);
	tt.data_len = 20;
#endif
	ret = cxt->caf_ops.trigger_init(&tt, &cxt->caf_trigger_handle);
	if (ret) {
		ISP_LOGE("failed to init caf library");
		goto error_caf_init;
	}

	/* show version */
	afaltek_adpt_get_version(cxt);

	/* init lib */
	cxt->af_runtime_obj = cxt->ops.init(&cxt->af_out_obj);
	if (!cxt->af_runtime_obj) {
		ISP_LOGE("failed to init lib");
		goto error_lib_init;
	}

	/* init param */
	ret = afaltek_adpt_param_init(cxt, in_p->ctrl_in);
	if (ret)
		ISP_LOGE("ret = %ld", ret);

	ret = afaltek_get_hw_config(&out_p->hw_cfg);
	if (ret)
		ISP_LOGE("ret = %ld", ret);

	*adpt_handle = (cmr_handle)cxt;
	cxt->inited = 1;
	return ret;

error_lib_init:
	cxt->caf_ops.trigger_deinit(cxt->caf_trigger_handle);
error_caf_init:
	afaltek_libops_deinit(cxt);
error_libops_init:
	free(cxt);
	cxt = NULL;
error_malloc:
	return ret;
}

static cmr_int afaltek_adpt_deinit(cmr_handle adpt_handle)
{
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	ISP_LOGI("cxt = %p", cxt);
	if (cxt) {
		cxt->caf_ops.trigger_deinit(cxt->caf_trigger_handle);
		/* deinit lib */
		cxt->ops.deinit(cxt->af_runtime_obj, &cxt->af_out_obj);
		afaltek_libops_deinit(cxt);
		free(cxt);
		cxt = NULL;
	}

	return ISP_SUCCESS;
}

static cmr_int afaltek_adpt_proc_report_status(cmr_handle adpt_handle,
					       struct allib_af_output_report_t *report)
{
	cmr_int ret = ISP_SUCCESS;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	ISP_LOGI("focus_status.t_status = %d", report->focus_status.t_status);
	if (alAFLib_STATUS_UNKNOWN == report->focus_status.t_status) {
		ret = afaltek_adpt_af_done(cxt, 0);
	} else if (alAFLib_STATUS_FOCUSED == report->focus_status.t_status) {
		ret = afaltek_adpt_af_done(cxt, 1);
	} else {
		ISP_LOGI("unkown status = %d", report->focus_status.t_status);
	}

	return ret;
}

static cmr_int afaltek_adpt_proc_report_stats_cfg(cmr_handle adpt_handle,
						  struct allib_af_output_report_t *report)
{
	cmr_int ret = ISP_SUCCESS;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct alhw3a_af_cfginfo_t af_cfg;
	struct allib_af_input_special_event event;
	cmr_bzero(&af_cfg, sizeof(af_cfg));
	al3awrapperaf_updateispconfig_af(&report->stats_config, &af_cfg);
	cxt->stats_config.token_id = af_cfg.tokenid;
	cxt->stats_config.need_update_token = 1;
	ISP_LOGI("token_id = %d", cxt->stats_config.token_id);

	/* send stats config to framework */
	afaltek_adpt_config_af_stats(cxt, &af_cfg);

	cmr_bzero(&event, sizeof(event));
	event.flag = 0;
	event.type = alAFLib_AF_STATS_CONFIG_UPDATED;
	ret = afaltek_adpt_set_special_event(cxt, &event);
	return ret;
}

static cmr_int afaltek_adpt_proc_report_debug_info(struct allib_af_output_report_t *report,
						   struct af_altek_report_t *out)
{
	cmr_int ret = ISP_SUCCESS;

	if (out) {
		ret = al3awrapper_updateafreport(&report->focus_status, &out->report_out);
		ISP_LOGI("ret = %ld", ret);
		if (ERR_WPR_AF_SUCCESS == ret)
			ret = ISP_SUCCESS;
		out->need_report = 1;
	}
	return ret;
}

static cmr_int afaltek_adpt_proc_out_report(cmr_handle adpt_handle,
					    struct allib_af_output_report_t *report,
					    void *report_out)
{
	cmr_int ret = ISP_SUCCESS;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;

	ISP_LOGI("report->type = 0x%x", report->type);
	if (alAFLIB_OUTPUT_STATUS & report->type) {
		ret = afaltek_adpt_proc_report_status(cxt, report);
	}

	if (alAFLIB_OUTPUT_STATS_CONFIG & report->type) {
		if (cxt->inited)
			ret = afaltek_adpt_proc_report_stats_cfg(cxt, report);
	}

	if (alAFLIB_OUTPUT_DEBUG_INFO & report->type) {
		ret = afaltek_adpt_proc_report_debug_info(report, report_out);
	}

	return ret;
}

static cmr_int afaltek_adpt_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	struct af_altek_context *cxt = (struct af_altek_context *)adpt_handle;
	struct af_ctrl_process_in *proc_in = (struct af_ctrl_process_in *)in;
	struct af_ctrl_process_out *proc_out = (struct af_ctrl_process_out *)out;

	ISP_LOGI("E");
	ret = al3awrapper_dispatchhw3a_afstats(proc_in->statistics_data->addr,
					       (void *)(&cxt->af_stats));
	if (ERR_WPR_AF_SUCCESS != ret) {
		ISP_LOGE("failed to dispatch af stats");
		ret = -ISP_PARAM_ERROR;
		goto exit;
	}

	if (cxt->stats_config.need_update_token) {
		ISP_LOGI("af_token_id = %d, token_id = %d",
				cxt->af_stats.af_token_id, cxt->stats_config.token_id);
		if (cxt->af_stats.af_token_id == cxt->stats_config.token_id) {
			struct allib_af_input_special_event event;

			cmr_bzero(&event, sizeof(event));
			event.flag = 1;
			event.type = alAFLib_AF_STATS_CONFIG_UPDATED;
			ret = afaltek_adpt_set_special_event(cxt, &event);
			cxt->stats_config.need_update_token = 0;
		}
	}

	if (cxt->ops.process(&cxt->af_stats, &cxt->af_out_obj, cxt->af_runtime_obj)) {
		if (cxt->af_out_obj.result) {
			cxt->report_data.need_report = 0;
			ret = afaltek_adpt_proc_out_report(cxt, &cxt->af_out_obj, &cxt->report_data);
			ISP_LOGI("process need repot result ret = %ld", ret);
			if (cxt->report_data.need_report) {
				proc_out->data = &cxt->report_data.report_out;
				proc_out->size = sizeof(cxt->report_data.report_out);
			} else {
				proc_out->data = NULL;
				proc_out->size = 0;
			}
		} else {
			ret = ISP_SUCCESS;
		}
	} else {
		ISP_LOGE("failed to process af stats");
	}
exit:
	return ret;
}

static cmr_int afaltek_adpt_ioctrl(cmr_handle adpt_handle, cmr_int cmd,
				   void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;

	if ((AF_CTRL_CMD_SET_BASE < cmd) && (AF_CTRL_CMD_SET_MAX > cmd))
		ret = afaltek_adpt_inctrl(adpt_handle, cmd, in, out);
	else if ((AF_CTRL_CMD_GET_BASE < cmd) && (AF_CTRL_CMD_GET_MAX > cmd))
		ret = afaltek_adpt_outctrl(adpt_handle, cmd, in, out);

	return ret;
}

static struct adpt_ops_type af_altek_adpt_ops = {
	.adpt_init = afaltek_adpt_init,
	.adpt_deinit = afaltek_adpt_deinit,
	.adpt_process = afaltek_adpt_process,
	.adpt_ioctrl = afaltek_adpt_ioctrl,
};

static struct isp_lib_config af_altek_lib_info = {
	.product_id = 0,
	.version_id = 0,
	.product_name_low = "",
	.product_name_high = "",
};

struct adpt_register_type af_altek_adpt_info = {
	.lib_type = ADPT_LIB_AF,
	.lib_info = &af_altek_lib_info,
	.ops = &af_altek_adpt_ops,
};
