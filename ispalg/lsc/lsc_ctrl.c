/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "lsc_ctrl"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>
#include <cutils/properties.h>
#include "lsc_adv.h"
#include "isp_adpt.h"
#include <dlfcn.h>
#include "isp_mw.h"
#include <utils/Timers.h>
#include <sys/stat.h>
#include <math.h>

struct lsc_ctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct lsc_ctrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	isp_lsc_cb lsc_set_cb;
	struct lsc_ctrl_work_lib work_lib;
	struct lsc_adv_calc_result proc_out;
};

static cmr_s32 _lscctrl_deinit_adpt(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("adpt_deinit fun is NULL");
	}

exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_destroy_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param , param is NULL");
		rtn = LSC_ERROR;
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

static cmr_s32 _lscctrl_process(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("process fun is NULL");
	}
exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int _lscctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *handle = (struct lsc_ctrl_cxt *)p_data;

	if (NULL == handle) {
		ISP_LOGE("Error: handle is NULL");
		return rtn;
	}

	if (!message || !p_data) {
		ISP_LOGE("fail to chcek param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case LSCCTRL_EVT_INIT:
		break;
	case LSCCTRL_EVT_DEINIT:
		rtn = _lscctrl_deinit_adpt(handle);
		break;
	case LSCCTRL_EVT_IOCTRL:
		break;
	case LSCCTRL_EVT_PROCESS:	// ISP_PROC_LSC_CALC
		rtn = _lscctrl_process(handle, (struct lsc_adv_calc_param *)message->data, &handle->proc_out);
		break;
	default:
		ISP_LOGE("fail to proc,don't support msg");
		break;
	}

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_init_lib(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param ,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, (cmr_handle)cxt_ptr);
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_init_adpt(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn = adpt_get_ops(ADPT_LIB_LSC, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = _lscctrl_init_lib(cxt_ptr, in_ptr);
exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_create_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, _lscctrl_ctrl_thr_proc, (void *)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = LSC_ERROR;
		goto exit;
	}
	rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "lscctrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set lscctrl name");
		rtn = CMR_MSG_SUCCESS;
	}
exit:
	ISP_LOGI("lsc_ctrl thread rtn %ld", rtn);
	return rtn;
}

cmr_int lsc_ctrl_init(struct lsc_adv_init_param * input_ptr, cmr_handle * handle_lsc)
{
	cmr_int rtn = ISP_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = NULL;

	cxt_ptr = (struct lsc_ctrl_cxt *)malloc(sizeof(struct lsc_ctrl_cxt));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create lsc ctrl context!");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(struct lsc_ctrl_cxt));

	rtn = _lscctrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}
	cxt_ptr->lsc_set_cb = input_ptr->lsc_set_cb;
	cxt_ptr->caller_handle = input_ptr->caller_handle;
	rtn = _lscctrl_init_adpt(cxt_ptr, input_ptr);
	if (rtn) {
		goto exit;
	}

exit:
	if (rtn) {
		if (cxt_ptr) {
			free(cxt_ptr);
		}
	} else {
		*handle_lsc = (cmr_handle) cxt_ptr;
	}
	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int lsc_ctrl_deinit(cmr_handle * handle_lsc)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = *handle_lsc;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	message.msg_type = LSCCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("failed to send msg to lsc thr %ld", rtn);
		goto exit;
	}

	rtn = _lscctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy lscctrl thread %ld", rtn);
		goto exit;
	}

exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle_lsc = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param * in_ptr, struct lsc_adv_calc_result * result)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt *)handle_lsc;

	if (!handle_lsc || !in_ptr || !result) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}
	//cxt_ptr->proc_out.dst_gain = result->dst_gain;

	CMR_MSG_INIT(message);
	message.data = malloc(sizeof(struct lsc_adv_calc_param));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}

	memcpy(message.data, (void *)in_ptr, sizeof(struct lsc_adv_calc_param));
	message.alloc_flag = 1;
	message.msg_type = LSCCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to lsc thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto exit;
	}

exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_s32 lsc_set_monitor(cmr_handle handler, struct lsc_monitor_info *in_param)
{
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt *)handler;
	ISP_LOGI("in_param win_size[%d,%d] win_num[%d,%d]",in_param->win_size.w,in_param->win_size.h,in_param->win_num.w,in_param->win_num.h);

	if (cxt_ptr->lsc_set_cb) {
		cxt_ptr->lsc_set_cb(cxt_ptr->caller_handle, ISP_LSC_SET_MONITOR, in_param, NULL);
	}

	return 0;
}

cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt *)handle_lsc;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!handle_lsc) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGV("cmd = %d,done %ld", cmd, rtn);
	return rtn;
}
