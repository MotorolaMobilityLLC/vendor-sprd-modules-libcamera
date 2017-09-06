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
#define LOG_TAG "af_ctrl"

#include "af_ctrl.h"
#include "isp_adpt.h"
#include <cutils/properties.h>

#define AFCTRL_EVT_BASE				0x2000
#define AFCTRL_EVT_INIT				AFCTRL_EVT_BASE
#define AFCTRL_EVT_DEINIT			(AFCTRL_EVT_BASE + 1)
#define AFCTRL_EVT_IOCTRL			(AFCTRL_EVT_BASE + 2)
#define AFCTRL_EVT_PROCESS			(AFCTRL_EVT_BASE + 3)
#define AFCTRL_EVT_EXIT				(AFCTRL_EVT_BASE + 4)

struct afctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct afctrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	struct afctrl_work_lib work_lib;
	struct afctrl_calc_out proc_out;
	isp_af_cb af_set_cb;
};

static cmr_s32 af_set_pos(void *handle_af, struct af_motor_pos *in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_SET_POS, &in_param->motor_pos, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_lens_move(void *handle_af, cmr_u16 in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_LENS_SET_POS, (void *)&in_param, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_get_otp(void *handle_af, uint16_t * inf, uint16_t * macro)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_LENS_GET_OTP, inf, macro);
	}

	return ISP_SUCCESS;
}

static uint32_t af_get_motor_pos(void *handle_af, cmr_u16 * in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_GET_MOTOR_POS, (void *)in_param, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_set_motor_bestmode(void *handle_af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_SET_MOTOR_BESTMODE, NULL, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_set_vcm_test_mode(void *handle_af, char *vcm_mode)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_SET_VCM_TEST_MODE, vcm_mode, NULL);
	}

	return ISP_SUCCESS;
}

static uint32_t af_get_vcm_test_mode(void *handle_af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_GET_VCM_TEST_MODE, NULL, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_end_notice(void *handle_af, struct af_result_param *in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	struct isp_af_notice af_notice = { 0x00, 0x00 };

	af_notice.mode = ISP_FOCUS_MOVE_END;
	af_notice.valid_win = in_param->suc_win;
	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_END_NOTICE, (void *)&af_notice, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_start_notice(void *handle_af)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	struct isp_af_notice af_notice = { 0x00, 0x00 };

	af_notice.mode = ISP_FOCUS_MOVE_START;
	af_notice.valid_win = 0x00;
	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_START_NOTICE, (void *)&af_notice, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_lock_module(void *handle_af, cmr_int af_locker_type)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	cmr_int rtn = ISP_SUCCESS;

	if (NULL == cxt_ptr->af_set_cb) {
		ISP_LOGE("fail to check param!");
		return ISP_PARAM_NULL;
	}

	switch (af_locker_type) {
	case AF_LOCKER_AE:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_AE_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_AE_CAF:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_AE_CAF_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_AWB:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_AWB_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_LSC:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_LSC_LOCK, NULL, NULL);
		break;
	case AF_LOCKER_NLM:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_NLM_LOCK, NULL, NULL);
		break;
	default:
		ISP_LOGE("fail to do af lock,af_locker_type is not supported!");
		break;
	}

	return rtn;
}

static cmr_s32 af_unlock_module(void *handle_af, cmr_int af_locker_type)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	cmr_int rtn = ISP_SUCCESS;

	if (NULL == cxt_ptr->af_set_cb) {
		ISP_LOGE("fail to check param!");
		return ISP_PARAM_NULL;
	}

	switch (af_locker_type) {
	case AF_LOCKER_AE:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_AE_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_AE_CAF:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_AE_CAF_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_AWB:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_AWB_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_LSC:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_LSC_UNLOCK, NULL, NULL);
		break;
	case AF_LOCKER_NLM:
		rtn = cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_NLM_UNLOCK, NULL, NULL);
		break;
	default:
		ISP_LOGE("fail to unlock, af_unlocker_type is not supported!");
		break;
	}

	return rtn;
}

static cmr_s32 af_set_monitor(void *handle_af, struct af_monitor_set *in_param, cmr_u32 cur_envi)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_SET_MONITOR, (void *)in_param, (void *)&cur_envi);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_set_monitor_win(void *handle_af, struct af_monitor_win *in_param)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_SET_MONITOR_WIN, (void *)(in_param->win_pos), NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_get_monitor_win_num(void *handle_af, cmr_u32 * win_num)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_GET_MONITOR_WIN_NUM, (void *)win_num, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_bypass(void *handle_af, cmr_u32 * bypass)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AFM_BYPASS, (void *)bypass, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_skip_num(void *handle_af, cmr_u32 * afm_skip_num)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AFM_SKIP_NUM, (void *)afm_skip_num, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_mode(void *handle_af, cmr_u32 * afm_mode)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AFM_MODE, (void *)afm_mode, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_iir_nr_cfg(void *handle_af, struct af_iir_nr_info *af_iir_nr)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AFM_IIR_NR_CFG, (void *)af_iir_nr, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_monitor_module_cfg(void *handle_af, struct af_enhanced_module_info *af_enhanced_module)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AFM_MODULES_CFG, (void *)af_enhanced_module, NULL);
	}

	return ISP_SUCCESS;
}

static cmr_s32 af_get_system_time(cmr_handle handle_af, cmr_u32 * sec, cmr_u32 * usec)
{
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	if (cxt_ptr->af_set_cb) {
		cxt_ptr->af_set_cb(cxt_ptr->caller_handle, ISP_AF_GET_SYSTEM_TIME, sec, usec);
	}

	return 0;
}

static cmr_int afctrl_process(struct afctrl_cxt *cxt_ptr, struct afctrl_calc_in *in_ptr, struct af_result_param *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_work_lib *lib_ptr = NULL;
	cmr_s8 value[PROPERTY_VALUE_MAX];
	struct af_motor_pos pos = { 0, 0, 0 };

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param!");
		goto exit;
	}

	property_get("persist.sys.isp.vcm.tuning.mode", (char *)value, "0");
	if (1 == atoi((char *)value)) {
		property_get("persist.sys.isp.vcm.position", (char *)value, "0");
		pos.motor_pos = atoi((char *)value);
		af_set_pos(cxt_ptr, &pos);
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int afctrl_deinit_adpt(struct afctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("adpt_deinit fun is NULL");
	}

	ISP_LOGI(" af_deinit is OK!");
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int afctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AFCTRL_EVT_INIT:
		break;
	case AFCTRL_EVT_DEINIT:
		rtn = afctrl_deinit_adpt(cxt_ptr);
		break;
	case AFCTRL_EVT_EXIT:
		break;
	case AFCTRL_EVT_IOCTRL:
		break;
	case AFCTRL_EVT_PROCESS:
		rtn = afctrl_process(cxt_ptr, (struct afctrl_calc_in *)message->data, (struct af_result_param *)&cxt_ptr->proc_out);
		break;
	default:
		ISP_LOGE("fail to proc ,don't support msg");
		break;
	}

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int afctrl_create_thread(struct afctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, afctrl_ctrl_thr_proc, (void *)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = ISP_ERROR;
	}

	ISP_LOGI("af_ctrl thread rtn %ld", rtn);
	return rtn;
}

static cmr_int afctrl_destroy_thread(struct afctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (cxt_ptr->thr_handle) {
		rtn = cmr_thread_destroy(cxt_ptr->thr_handle);
		if (!rtn) {
			cxt_ptr->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int afctrl_init_lib(struct afctrl_cxt *cxt_ptr, struct afctrl_init_in *in_ptr, struct afctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct afctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, out_ptr);
		if (NULL == lib_ptr->lib_handle) {
			ISP_LOGE("fail to check af lib_handle!");
			ret = ISP_ERROR;
		}
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int afctrl_init_adpt(struct afctrl_cxt *cxt_ptr, struct afctrl_init_in *in_ptr, struct afctrl_init_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	ISP_LOGI("E %ld", rtn);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param!");
		goto exit;
	}

	/* find vendor adpter */
	rtn = adpt_get_ops(ADPT_LIB_AF, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = afctrl_init_lib(cxt_ptr, in_ptr, out_ptr);
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int af_ctrl_init(struct afctrl_init_in * input_ptr, cmr_handle * handle_af)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = NULL;
	struct afctrl_init_out result;

	memset((void *)&result, 0, sizeof(result));
	input_ptr->go_position = af_set_pos;
	input_ptr->end_notice = af_end_notice;
	input_ptr->start_notice = af_start_notice;
	input_ptr->set_monitor = af_set_monitor;
	input_ptr->set_monitor_win = af_set_monitor_win;
	input_ptr->get_monitor_win_num = af_get_monitor_win_num;
	input_ptr->lock_module = af_lock_module;
	input_ptr->unlock_module = af_unlock_module;
	input_ptr->af_lens_move = af_lens_move;
	input_ptr->af_get_otp = af_get_otp;
	input_ptr->af_get_motor_pos = af_get_motor_pos;
	input_ptr->af_set_motor_bestmode = af_set_motor_bestmode;
	input_ptr->af_set_test_vcm_mode = af_set_vcm_test_mode;
	input_ptr->af_get_test_vcm_mode = af_get_vcm_test_mode;
	input_ptr->af_monitor_bypass = af_monitor_bypass;
	input_ptr->af_monitor_skip_num = af_monitor_skip_num;
	input_ptr->af_monitor_mode = af_monitor_mode;
	input_ptr->af_monitor_iir_nr_cfg = af_monitor_iir_nr_cfg;
	input_ptr->af_monitor_module_cfg = af_monitor_module_cfg;
	input_ptr->af_get_system_time = af_get_system_time;

	cxt_ptr = (struct afctrl_cxt *)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create af ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset((void *)cxt_ptr, 0x00, sizeof(*cxt_ptr));

	input_ptr->caller = (void *)cxt_ptr;
	cxt_ptr->af_set_cb = input_ptr->af_set_cb;
	cxt_ptr->caller_handle = input_ptr->caller_handle;
	rtn = afctrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = afctrl_init_adpt(cxt_ptr, input_ptr, &result);
	if (rtn) {
		goto error_adpt_init;
	}

	*handle_af = (cmr_handle) cxt_ptr;

	ISP_LOGI(" done %ld", rtn);
	return rtn;

error_adpt_init:
	rtn = afctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy afctrl thr %ld", rtn);
	}

exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		cxt_ptr = NULL;
	}

	return ISP_SUCCESS;
}

cmr_int af_ctrl_deinit(cmr_handle * handle_af)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = *handle_af;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to deinit, handle_af is NULL");
		return -ISP_ERROR;
	}
	message.msg_type = AFCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		goto exit;
	}

	rtn = afctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy afctrl thr %ld", rtn);
		goto exit;
	}

exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle_af = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int af_ctrl_process(cmr_handle handle_af, void *in_ptr, struct afctrl_calc_out * result)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;

	ISP_CHECK_HANDLE_VALID(handle_af);
	CMR_MSG_INIT(message);

	message.data = malloc(sizeof(struct afctrl_calc_in));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, (void *)in_ptr, sizeof(struct afctrl_calc_in));
	message.alloc_flag = 1;

	message.msg_type = AFCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto exit;
	}

	if (result) {
		*result = cxt_ptr->proc_out;
	}

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int af_ctrl_ioctrl(cmr_handle handle_af, cmr_int cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct afctrl_cxt *cxt_ptr = (struct afctrl_cxt *)handle_af;
	struct afctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGV("fail to check param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

exit:
	ISP_LOGV("cmd = %ld,done %ld", cmd, rtn);
	return rtn;
}
