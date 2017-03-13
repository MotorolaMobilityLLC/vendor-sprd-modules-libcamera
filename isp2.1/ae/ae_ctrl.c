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


#include "ae_ctrl.h"

#define AECTRL_EVT_BASE            0x2000
#define AECTRL_EVT_INIT            AECTRL_EVT_BASE
#define AECTRL_EVT_DEINIT          (AECTRL_EVT_BASE + 1)
#define AECTRL_EVT_IOCTRL          (AECTRL_EVT_BASE + 2)
#define AECTRL_EVT_PROCESS         (AECTRL_EVT_BASE + 3)
#define AECTRL_EVT_EXIT            (AECTRL_EVT_BASE + 4)

struct aectrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct aectrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	isp_ae_cb ae_set_cb;
	struct aectrl_work_lib work_lib;
	struct ae_ctrl_param_out ioctrl_out;
	struct ae_calc_out proc_out;
	cmr_u32 bakup_rgb_gain;
	pthread_mutex_t ioctrl_sync_lock;
};

int32_t _isp_get_flash_cali_param(isp_pm_handle_t pm_handle, struct isp_flash_param **out_param_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};
	memset(&param_data, 0, sizeof(param_data));

	if (NULL == out_param_ptr) {
		return ISP_PARAM_NULL;
	}

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_FLASH_CALI, NULL, 0);
	rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == rtn && 1 == output.param_num) {
		(*out_param_ptr) = (struct isp_flash_param*)output.param_data->data_ptr;
	} else {
		rtn = ISP_ERROR;
	}

	return rtn;
}

static int32_t ae_ex_set_exposure(void *handler, struct ae_exposure *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_EX_SET_EXPOSURE, in_param, NULL);
	}

	return 0;
}

static int32_t ae_set_exposure(void *handler, struct ae_exposure *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_EXPOSURE, &in_param->exposure, NULL);
	}

	return 0;
}

static int32_t ae_set_again(void *handler, struct ae_gain *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_GAIN, &in_param->gain, NULL);
	}
	ISP_LOGD("AE set gain %d", in_param->gain);
	return 0;
}

static int32_t ae_set_monitor(void *handler, struct ae_monitor_cfg *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_MONITOR, &in_param->skip_num, NULL);
	}

	return 0;
}

static int32_t ae_set_monitor_win(void *handler, struct ae_monitor_info *in_param)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_MONITOR_WIN, in_param, NULL);
	}

	return 0;
}

static int32_t ae_set_monitor_bypass(void *handler, uint32_t is_bypass)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_MONITOR_BYPASS, &is_bypass, NULL);
	}

	return 0;
}

static int32_t ae_set_statistics_mode(void *handler, enum ae_statistics_mode mode, uint32_t skip_number)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_STATISTICS_MODE, &mode, 	&skip_number);
	}

	return 0;
}

static int32_t ae_callback(void *handler, enum ae_cb_type cb_type)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_AE_CALLBACK, &cb_type, NULL);
	}

	return 0;
}

static int32_t ae_get_system_time(void *handler, uint32_t *sec, uint32_t *usec)
{
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_SYSTEM_TIME, sec, usec);
	}

	return 0;
}

static int32_t ae_flash_get_charge(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr)
{
	int32_t                         ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_FLASH_CHARGE, cfg_ptr, cell_ptr);
	}

	return ret;
}

static int32_t ae_flash_get_time(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr)
{
	int32_t                         ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_FLASH_TIME, cfg_ptr, cell_ptr);
	}

	return ret;
}

static int32_t ae_flash_set_charge(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr)
{
	int32_t                         ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		ISP_LOGD("led_idx=%d, led_type=%d, level=%d", cfg_ptr->led_idx, cfg_ptr->type, element_ptr->index);
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_FLASH_CHARGE, cfg_ptr, element_ptr);
	}

	return ret;
}

static int32_t ae_flash_set_time(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr)
{
	int32_t                         ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_FLASH_TIME, cfg_ptr, element_ptr);
	}

	return ret;
}

static int32_t ae_flash_ctrl_enable(void *handler, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr)
{
	int32_t                         ret = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		ISP_LOGD("led_en=%d, led_type=%d", cfg_ptr->led_idx, cfg_ptr->type);
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_FLASH_CTRL, cfg_ptr, element_ptr);
	}

	return ret;
}

static int32_t ae_set_rgb_gain(void *handler, double gain)
{
	cmr_int rtn = ISP_SUCCESS;
	cmr_u32 final_gain = 0;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		final_gain = (cmr_u32)(gain * cxt_ptr->bakup_rgb_gain + 0.5);
		ISP_LOGD("d-gain: coeff: %f-%d-%d\n", gain, cxt_ptr->bakup_rgb_gain, final_gain);
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_SET_RGB_GAIN, &final_gain, NULL);
	}

	return rtn;
}

static cmr_int ae_get_rgb_gain(void *handler, cmr_u32 *gain)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handler;

	if (cxt_ptr->ae_set_cb) {
		cxt_ptr->ae_set_cb(cxt_ptr->caller_handle, ISP_AE_GET_RGB_GAIN, gain, NULL);
	}

	return rtn;
}

static int32_t ae_set_shutter_gain_delay_info(void* handler, void* param)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context*)handler;

	if ((NULL == ctrl_context) || (NULL == param)) {
		ISP_LOGE("input is NULL error, cxt: %p, param: %p\n", ctrl_context, param);
		return ISP_PARAM_NULL;
	}

	struct ae_valid_fn valid_info = {0, 0, 0};
	struct ae_exp_gain_delay_info *delay_info_ptr = (struct ae_exp_gain_delay_info*)param;

	valid_info.group_hold_flag = delay_info_ptr->group_hold_flag;
	valid_info.valid_expo_num = delay_info_ptr->valid_exp_num;
	valid_info.valid_gain_num = delay_info_ptr->valid_gain_num;

	return 0;
}

static cmr_int aectrl_deinit_adpt(struct aectrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
	} else {
		ISP_LOGI("fail to do adpt_deinit");
	}

exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_ioctrl(struct aectrl_cxt *cxt_ptr, enum ae_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}

exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_process(struct aectrl_cxt *cxt_ptr, struct ae_calc_in *in_ptr, struct ae_calc_out *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to to check param, param is NULL!");
		goto exit;
	}

	pthread_mutex_lock(&cxt_ptr->ioctrl_sync_lock);
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI(":ISP:ioctrl fun is NULL");
	}
	pthread_mutex_unlock(&cxt_ptr->ioctrl_sync_lock);

exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto exit;
	}
	ISP_LOGI(":ISP:message.msg_type 0x%x, data %p", message->msg_type,
		 message->data);

	switch (message->msg_type) {
	case AECTRL_EVT_INIT:
		break;
	case AECTRL_EVT_DEINIT:
		rtn = aectrl_deinit_adpt(cxt_ptr);
		break;
	case AECTRL_EVT_EXIT:
		break;
	case AECTRL_EVT_IOCTRL:
		break;
	case AECTRL_EVT_PROCESS:
		rtn = aectrl_process(cxt_ptr, (struct ae_calc_in*)message->data, &cxt_ptr->proc_out);
		break;
	default:
		ISP_LOGE("fail to check param, don't support msg");
		break;
	}

exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}


static cmr_int aectrl_create_thread(struct aectrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, aectrl_ctrl_thr_proc, (void*)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = ISP_ERROR;
	}

	ISP_LOGI(":ISP:ae_ctrl thread rtn %ld", rtn);
	return rtn;
}

static cmr_int aectrl_destroy_thread(struct aectrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check input parm");
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
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static cmr_int aectrl_init_lib(struct aectrl_cxt *cxt_ptr, struct ae_init_in *in_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, NULL);
	} else {
		ISP_LOGI(":ISP:adpt_init fun is NULL");
	}
exit:
	ISP_LOGI(":ISP:done %ld", ret);
	return ret;
}

static cmr_int aectrl_init_adpt(struct aectrl_cxt *cxt_ptr, struct ae_init_in *in_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn  = adpt_get_ops(ADPT_LIB_AE, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = aectrl_init_lib(cxt_ptr, in_ptr);
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

cmr_int ae_ctrl_ioctrl(cmr_handle handle, enum ae_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handle;
	struct aectrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param ,param is NULL!");
		goto exit;
	}

	pthread_mutex_lock(&cxt_ptr->ioctrl_sync_lock);
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI(":ISP:ioctrl fun is NULL");
	}
	pthread_mutex_unlock(&cxt_ptr->ioctrl_sync_lock);
exit:
	ISP_LOGV(":ISP:cmd = %d,done %ld", cmd, rtn);
	return rtn;
}

int32_t ae_ctrl_init(struct ae_init_in *input_ptr, cmr_handle *handle_ae)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = NULL;
	struct aectrl_work_lib *lib_ptr = NULL;

	input_ptr->isp_ops.set_again                 	= ae_set_again;
	input_ptr->isp_ops.set_exposure              	= ae_set_exposure;
	input_ptr->isp_ops.set_monitor_win           	= ae_set_monitor_win;
	input_ptr->isp_ops.set_monitor               	= ae_set_monitor;
	input_ptr->isp_ops.callback                  	= ae_callback;
	input_ptr->isp_ops.set_monitor_bypass        	= ae_set_monitor_bypass;
	input_ptr->isp_ops.get_system_time           	= ae_get_system_time;
	input_ptr->isp_ops.set_statistics_mode       	= ae_set_statistics_mode;
	input_ptr->isp_ops.flash_get_charge          	= ae_flash_get_charge;
	input_ptr->isp_ops.flash_get_time            	= ae_flash_get_time;
	input_ptr->isp_ops.flash_set_charge          	= ae_flash_set_charge;
	input_ptr->isp_ops.flash_set_time            	= ae_flash_set_time;
	input_ptr->isp_ops.flash_ctrl					= ae_flash_ctrl_enable;
	input_ptr->isp_ops.ex_set_exposure           	= ae_ex_set_exposure;
	input_ptr->isp_ops.set_rgb_gain              	= ae_set_rgb_gain;
	input_ptr->isp_ops.set_shutter_gain_delay_info 	= ae_set_shutter_gain_delay_info;

	cxt_ptr = (struct aectrl_cxt*)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create ae ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt_ptr, sizeof(*cxt_ptr));

	input_ptr->isp_ops.isp_handler = (cmr_handle)cxt_ptr;
	cxt_ptr->caller_handle = input_ptr->caller_handle;
	cxt_ptr->ae_set_cb = input_ptr->ae_set_cb;

	pthread_mutex_init(&cxt_ptr->ioctrl_sync_lock,NULL);

	rtn = aectrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = aectrl_init_adpt(cxt_ptr, input_ptr);
	if (rtn) {
		goto error_adpt_init;
	}

	ae_get_rgb_gain(cxt_ptr, &cxt_ptr->bakup_rgb_gain);

	*handle_ae = (cmr_handle)cxt_ptr;

	ISP_LOGI(":ISP: done %d", rtn);
	return rtn;

error_adpt_init:
	aectrl_destroy_thread(cxt_ptr);
	pthread_mutex_destroy(&cxt_ptr->ioctrl_sync_lock);
exit:
	if (cxt_ptr) {
		free((void*)cxt_ptr);
		cxt_ptr = NULL;
	}

	return rtn;
}

cmr_int ae_ctrl_deinit(cmr_handle *handle_ae)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = *handle_ae;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, in parm is NULL");
		return ISP_ERROR;
	}

	message.msg_type = AECTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn );
		goto exit;
	}

	rtn = aectrl_destroy_thread(cxt_ptr);
	if (rtn) {
		pthread_mutex_destroy(&cxt_ptr->ioctrl_sync_lock);
		ISP_LOGE("fail to destroy aectrl thread %ld", rtn );
		goto exit;
	}

	pthread_mutex_destroy(&cxt_ptr->ioctrl_sync_lock);
exit:
	if (cxt_ptr) {
		free((void*)cxt_ptr);
		*handle_ae = NULL;
	}

	ISP_LOGI(":ISP:done %d", rtn);
	return rtn;
}

cmr_int ae_ctrl_process(cmr_handle handle_ae, struct ae_calc_in *in_ptr, struct ae_calc_out *result)
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct aectrl_cxt *cxt_ptr = (struct aectrl_cxt*)handle_ae;

	ISP_CHECK_HANDLE_VALID(handle_ae);
	CMR_MSG_INIT(message);

	message.data = malloc(sizeof(struct ae_calc_in));
	if (!message.data) {
		CMR_LOGE("fail to malloc msg");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memcpy(message.data, (void *)in_ptr, sizeof(struct ae_calc_in));
	message.alloc_flag = 1;

	message.msg_type = AECTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
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
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}
