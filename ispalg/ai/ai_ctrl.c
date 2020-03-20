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
#define LOG_TAG "ai_ctrl"

#include "ai_ctrl.h"
#include "isp_type.h"
#include "isp_com.h"
#include "cmr_log.h"
#include "cmr_msg.h"
#include "sprdaicapi.h"

#define AICTRL_EVT_BASE            0x2000
#define AICTRL_EVT_INIT            AICTRL_EVT_BASE
#define AICTRL_EVT_DEINIT          (AICTRL_EVT_BASE + 1)
#define AICTRL_EVT_IOCTRL          (AICTRL_EVT_BASE + 2)

struct ai_ctrl_msg_ctrl {
	cmr_int cmd;
	cmr_handle in;
	cmr_handle out;
};

static cmr_int ai_callback(cmr_handle handler, cmr_int cb_type, cmr_handle param)
{
	struct aictrl_cxt *cxt_ptr = (struct aictrl_cxt *)handler;

	if (cxt_ptr->ai_set_cb) {
		cxt_ptr->ai_set_cb(cxt_ptr->caller_handle, AI_CALLBACK_SET_CB, &cb_type, param);
	}

	return 0;
}

static cmr_int aictrl_ioctrl(struct aictrl_cxt *cxt_ptr, enum ai_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aictrl_work_lib *lib_ptr = NULL;
	struct ai_scene_detect_info scene_info;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		if(AI_SET_AE_PARAM == cmd) {
			out_ptr = &scene_info;
		}
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
		if (ISP_SUCCESS != rtn) {
			goto exit;
		}
	} else {
		ISP_LOGE("ai ioctrl fun is NULL");
		goto exit;
	}

	if ((AI_SET_AE_PARAM == cmd) && (NULL != out_ptr)) {
		cmr_int scend_id_flag = 0;

		if (scene_info.cur_scene_id != cxt_ptr->scene_info.cur_scene_id)
			scend_id_flag = 1;

		memcpy(&cxt_ptr->scene_info, &scene_info, sizeof(struct ai_scene_detect_info));
		(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_AE, &cxt_ptr->scene_info);
		(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_AWB, &cxt_ptr->scene_info);
		(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_AF, &cxt_ptr->scene_info);
		if (scend_id_flag)
			(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_HAL, &cxt_ptr->scene_info.cur_scene_id);
	}

	if (AI_PROCESS_STOP == cmd) {
		memset(&cxt_ptr->scene_info, 0, sizeof(struct ai_scene_detect_info));
		cxt_ptr->scene_info.cur_scene_id = AI_SCENE_DEFAULT;
		(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_HAL, &cxt_ptr->scene_info.cur_scene_id);
		(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_AE, &cxt_ptr->scene_info);
		(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_AWB, &cxt_ptr->scene_info);
		(*cxt_ptr->ai_ops.callback) (cxt_ptr, AI_CB_FOR_AF, &cxt_ptr->scene_info);
	}

	ISP_LOGV("done %ld", rtn);

  exit:

	return rtn;
}

static cmr_int aictrl_deinit_adpt(struct aictrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aictrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to deinit");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("fail to do adpt_deinit");
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aictrl_ctrl_thr_proc(struct cmr_msg *message, cmr_handle p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aictrl_cxt *cxt_ptr = (struct aictrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to check param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case AICTRL_EVT_INIT:
		break;
	case AICTRL_EVT_DEINIT:
		rtn = aictrl_deinit_adpt(cxt_ptr);
		break;
	case AICTRL_EVT_IOCTRL:
		rtn = aictrl_ioctrl(cxt_ptr, ((struct ai_ctrl_msg_ctrl *)message->data)->cmd, ((struct ai_ctrl_msg_ctrl *)message->data)->in, ((struct ai_ctrl_msg_ctrl *)message->data)->out);
		break;
	default:
		ISP_LOGE("fail to check param, don't support msg");
		break;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aictrl_create_thread(struct aictrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, aictrl_ctrl_thr_proc, (cmr_handle)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = ISP_ERROR;
		goto exit;
	}
	rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "aictrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set aictrl name");
		rtn = CMR_MSG_SUCCESS;
	}
  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int aictrl_destroy_thread(struct aictrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check input param");
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
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int ai_ctrl_ioctrl(cmr_handle handle, enum ai_io_ctrl_cmd cmd, cmr_handle in_ptr, cmr_handle out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aictrl_cxt *cxt_ptr = (struct aictrl_cxt *)handle;
	struct ai_ctrl_msg_ctrl msg_ctrl;

	ISP_CHECK_HANDLE_VALID(handle);
	CMR_MSG_INIT(message);
	if ((AI_SYNC_MSG_BEGIN < cmd) && (AI_SYNC_MSG_END > cmd)) {
		msg_ctrl.cmd = cmd;
		msg_ctrl.in = in_ptr;
		msg_ctrl.out = out_ptr;
		message.data = &msg_ctrl;
		message.msg_type = AICTRL_EVT_IOCTRL;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	} else if ((AI_DIRECT_MSG_BEGIN < cmd) && (AI_DIRECT_MSG_END > cmd)) {
		rtn = aictrl_ioctrl(cxt_ptr, cmd, in_ptr, out_ptr);
	}

  exit:
	ISP_LOGV("cmd = %d,done %ld", cmd, rtn);
	return rtn;
}

static cmr_int aictrl_init_lib(struct aictrl_cxt *cxt_ptr, struct ai_init_in *in_ptr, cmr_handle param)
{
	cmr_int ret = ISP_SUCCESS;
	struct aictrl_work_lib *lib_ptr = NULL;

	UNUSED(in_ptr);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(cxt_ptr, param);
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}
  exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

static cmr_int aictrl_init_adpt(struct aictrl_cxt *cxt_ptr, struct ai_init_in *in_ptr, cmr_handle param)
{
	cmr_int rtn = ISP_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn = adpt_get_ops(ADPT_LIB_AI, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = aictrl_init_lib(cxt_ptr, in_ptr, param);
  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_s32 ai_ctrl_init(struct ai_init_in *input_ptr, cmr_handle *handle_ai, cmr_handle result)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct aictrl_cxt *cxt_ptr = NULL;

	cxt_ptr = (struct aictrl_cxt *)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create ai ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(*cxt_ptr));

	cxt_ptr->scene_info.cur_scene_id = AI_SCENE_DEFAULT;
	cxt_ptr->caller_handle = input_ptr->caller_handle;
	cxt_ptr->ai_set_cb = input_ptr->ai_set_cb;
	cxt_ptr->ai_ops.callback = ai_callback;
	cxt_ptr->cameraId = input_ptr->cameraId;

	rtn = aictrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = aictrl_init_adpt(cxt_ptr, input_ptr, result);
	if (rtn) {
		goto error_init;
	}

	*handle_ai = (cmr_handle)cxt_ptr;

	ISP_LOGI("done %d", rtn);
	return rtn;

  error_init:
	aictrl_destroy_thread(cxt_ptr);
  exit:
	if (cxt_ptr) {
		free((cmr_handle) cxt_ptr);
		cxt_ptr = NULL;
	}

	return rtn;
}

cmr_int ai_ctrl_deinit(cmr_handle * handle_ai)
{
	cmr_int rtn = ISP_SUCCESS;
	struct aictrl_cxt *cxt_ptr = *handle_ai;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, in parm is NULL");
		return ISP_ERROR;
	}

	message.msg_type = AICTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		goto exit;
	}

	rtn = aictrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy aectrl thread %ld", rtn);
		goto exit;
	}

  exit:
	if (cxt_ptr) {
		free((cmr_handle) cxt_ptr);
		*handle_ai = NULL;
	}

	ISP_LOGI("done %ld", rtn);

	return rtn;
}
