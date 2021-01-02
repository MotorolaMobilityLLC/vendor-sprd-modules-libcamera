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

#define LOG_TAG "isp_mw"

#include "cmr_msg.h"
#include "isp_dev_access.h"
#include "isp_alg_fw.h"

enum isp_mw_status_type {
	ISP_MW_INIT,
	ISP_MW_DEINIT,
	ISP_MW_MAX
};

struct isp_mw_context {
	cmr_handle alg_fw_handle;
	cmr_handle dev_access_handle;
	pthread_mutex_t isp_mw_mutex;
	cmr_u32 isp_mw_sts;
};

void isp_statis_evt_cb(cmr_int evt, void *data, void *privdata)
{
	struct isp_mw_context *cxt = (struct isp_mw_context *)privdata;
	UNUSED(evt);

	pthread_mutex_lock(&cxt->isp_mw_mutex);

	if (cxt->isp_mw_sts == ISP_MW_INIT)
		isp_dev_statis_info_proc(cxt->dev_access_handle, data);
	else
		ISP_LOGW("warn sensor is stream off");

	pthread_mutex_unlock(&cxt->isp_mw_mutex);
}

void isp_irq_proc_evt_cb(cmr_int evt, void *data, void *privdata)
{
	struct isp_mw_context *cxt = (struct isp_mw_context *)privdata;
	UNUSED(evt);

	pthread_mutex_lock(&cxt->isp_mw_mutex);

	if (cxt->isp_mw_sts == ISP_MW_INIT)
		isp_dev_irq_info_proc(cxt->dev_access_handle, data);
	else
		ISP_LOGW("warn sensor is stream off");

	pthread_mutex_unlock(&cxt->isp_mw_mutex);
}

static cmr_s32 ispmw_check_proc_start_param(struct ips_in_param *in_param_ptr)
{
	cmr_s32 ret = ISP_SUCCESS;

	if ((ISP_ZERO != (in_param_ptr->src_frame.img_size.w & ISP_ONE)) ||
	    (ISP_ZERO != (in_param_ptr->src_frame.img_size.h & ISP_ONE))) {
		ret = -ISP_PARAM_ERROR;
		ISP_RETURN_IF_FAIL(ret, ("fail to check start param input size: w:%d, h:%d",
					 in_param_ptr->src_frame.img_size.w,
					 in_param_ptr->src_frame.img_size.h));
	}

	return ret;
}

cmr_int isp_init(struct isp_init_param *input_ptr, cmr_handle *handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_mw_context *cxt = NULL;
	struct isp_alg_fw_init_in ispalg_input;

	isp_init_log_level();
	ISP_LOGV("E");
	if (!input_ptr || !handle) {
		ISP_LOGE("fail to check init param, input_ptr %p handler %p", input_ptr, handle);
		ret = -ISP_PARAM_NULL;
		goto exit;
	}

	*handle = NULL;
	cxt = (struct isp_mw_context *)malloc(sizeof(struct isp_mw_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	memset((void *)cxt, 0x00, sizeof(*cxt));

	pthread_mutex_init(&cxt->isp_mw_mutex, NULL);
	pthread_mutex_lock(&cxt->isp_mw_mutex);
	cxt->isp_mw_sts = ISP_MW_INIT;
	pthread_mutex_unlock(&cxt->isp_mw_mutex);

	ret = isp_dev_access_init(input_ptr->dcam_fd, &cxt->dev_access_handle);
	if (ret) {
		goto exit;
	}

	ispalg_input.dev_access_handle = cxt->dev_access_handle;
	ispalg_input.init_param = input_ptr;
	ret = isp_alg_fw_init(&ispalg_input, &cxt->alg_fw_handle);

exit:
	if (ret) {
		if (cxt) {
			ret = isp_alg_fw_deinit(cxt->alg_fw_handle);
			ret = isp_dev_access_deinit(cxt->dev_access_handle);
			pthread_mutex_destroy(&cxt->isp_mw_mutex);
			free((void *)cxt);
			cxt = NULL;
		}
	} else {
		*handle = (cmr_handle) cxt;
	}

	ISP_LOGI("done %ld", ret);

	return ret;
}

cmr_int isp_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)handle;

	ISP_CHECK_HANDLE_VALID(handle);

	pthread_mutex_lock(&cxt->isp_mw_mutex);
	cxt->isp_mw_sts = ISP_MW_DEINIT;
	pthread_mutex_unlock(&cxt->isp_mw_mutex);

	ret = isp_alg_fw_deinit(cxt->alg_fw_handle);
	if (ret)
		ISP_LOGE("fail to deinit 3a fw %ld", ret);
	ret = isp_dev_access_deinit(cxt->dev_access_handle);
	if (ret)
		ISP_LOGE("fail to deinit access %ld", ret);

	pthread_mutex_destroy(&cxt->isp_mw_mutex);

	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}

	ISP_LOGI("done %ld", ret);

	return ret;
}

cmr_int isp_capability(cmr_handle handle, enum isp_capbility_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)handle;

	switch (cmd) {
	case ISP_VIDEO_SIZE:
		ret = isp_dev_access_capability(cxt->dev_access_handle, cmd, param_ptr);
		break;
	case ISP_LOW_LUX_EB:
	case ISP_CUR_ISO:
	case ISP_CTRL_GET_AE_LUM:
		ret = isp_alg_fw_capability(cxt->alg_fw_handle, cmd, param_ptr);
		break;
	default:
		break;
	}

	return ret;
}

cmr_int isp_ioctl(cmr_handle handle, enum isp_ctrl_cmd cmd, void *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)handle;

	if (NULL == cxt) {
		ISP_LOGE("fail to check isp handler");
		return ISP_PARAM_NULL;
	}

	pthread_mutex_lock(&cxt->isp_mw_mutex);
	if (cxt->isp_mw_sts == ISP_MW_DEINIT) {
		ISP_LOGE("fail to check isp_mw_sts");
		pthread_mutex_unlock(&cxt->isp_mw_mutex);
		return ret;
	}
	ret = isp_alg_fw_ioctl(cxt->alg_fw_handle, cmd, param_ptr);
        pthread_mutex_unlock(&cxt->isp_mw_mutex);

	ISP_TRACE_IF_FAIL(ret, ("fail to do isp_ioctl"));

	return ret;
}

cmr_int isp_video_start(cmr_handle handle, struct isp_video_start *param_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)handle;

	if (!handle || !param_ptr) {
		ret = -ISP_PARAM_ERROR;
		goto exit;
	}

	pthread_mutex_lock(&cxt->isp_mw_mutex);
	if (cxt->isp_mw_sts == ISP_MW_DEINIT) {
		ISP_LOGE("fail to check isp_mw_sts");
		pthread_mutex_unlock(&cxt->isp_mw_mutex);
		return ret;
	}
	ret = isp_alg_fw_start(cxt->alg_fw_handle, param_ptr);
        pthread_mutex_unlock(&cxt->isp_mw_mutex);

exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp_video_stop(cmr_handle handle)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)handle;

	if (!handle) {
		ret = -ISP_PARAM_NULL;
		goto exit;
	}

	pthread_mutex_lock(&cxt->isp_mw_mutex);
	if (cxt->isp_mw_sts == ISP_MW_DEINIT) {
		ISP_LOGE("fail to check isp_mw_sts");
		pthread_mutex_unlock(&cxt->isp_mw_mutex);
		return ret;
	}
	ret = isp_alg_fw_stop(cxt->alg_fw_handle);
        pthread_mutex_unlock(&cxt->isp_mw_mutex);

exit:
	return ret;
}

cmr_int isp_proc_start(cmr_handle handle, struct ips_in_param *in_ptr, struct ips_out_param *out_ptr)
{
	cmr_int ret = ISP_SUCCESS;
	struct isp_mw_context *cxt = (struct isp_mw_context *)handle;
	UNUSED(out_ptr);

	if (NULL == cxt) {
		ISP_LOGE("fail to check isp handler");
		return ISP_PARAM_NULL;
	}

	pthread_mutex_lock(&cxt->isp_mw_mutex);
	if (cxt->isp_mw_sts == ISP_MW_DEINIT) {
		ISP_LOGE("fail to check isp_mw_sts");
		pthread_mutex_unlock(&cxt->isp_mw_mutex);
		return ret;
	}
	pthread_mutex_unlock(&cxt->isp_mw_mutex);
	ret = ispmw_check_proc_start_param(in_ptr);
	ISP_RETURN_IF_FAIL(ret, ("fail to check init param"));

	ret = isp_alg_fw_proc_start(cxt->alg_fw_handle, in_ptr);

	return ret;
}
