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
#define LOG_TAG "awb_ctrl"

#include "awb_ctrl.h"
#include "lib_ctrl.h"
#include "isp_adpt.h"
#include "isp_pm.h"
#include "ae_ctrl.h"
#include "isp_otp_calibration.h"

/**************************************** MACRO DEFINE *****************************************/



/************************************* INTERNAL DATA TYPE ***************************************/
struct awbctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct awbctrl_cxt {
	cmr_handle thr_handle;
	struct awbctrl_work_lib work_lib;
	//void *ioctrl_out;
};

/*************************************EXTERNAL FUNCTION ***************************************/
static cmr_int awbctrl_deinit_adpt(struct awbctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
	} else {
		ISP_LOGI(":ISP:adpt_deinit fun is NULL");
	}

exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}


static cmr_int awbctrl_init_lib(struct awbctrl_cxt *cxt_ptr, struct awb_ctrl_init_param *in_ptr, struct awb_ctrl_init_result *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, out_ptr);
	} else {
		ISP_LOGI(":ISP:adpt_init fun is NULL");
	}
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static cmr_int awbctrl_init_adpt(struct awbctrl_cxt *cxt_ptr, struct awb_ctrl_init_param *in_ptr, struct awb_ctrl_init_result *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check para, param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn  = adpt_get_ops(ADPT_LIB_AWB, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = awbctrl_init_lib(cxt_ptr, in_ptr, out_ptr);
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

cmr_int awb_ctrl_init(struct awb_ctrl_init_param *input_ptr, cmr_handle *handle_awb)
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = NULL;
	struct awb_ctrl_init_result     result;

	memset((void*)&result, 0, sizeof(result));

	cxt_ptr = (struct awbctrl_cxt*)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create awb ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt_ptr, sizeof(*cxt_ptr));

	rtn = awbctrl_init_adpt(cxt_ptr, input_ptr, &result);
	if (rtn) {
		goto exit;
	}

exit:
	if (rtn) {
		if (cxt_ptr) {
			free(cxt_ptr);
		}
	} else {
		*handle_awb = (cmr_handle)cxt_ptr;
	}
	ISP_LOGI(":ISP:isp_3a_ctrl awb_init rtn = %d", rtn);

	return rtn;
}

static cmr_int awbctrl_process(struct awbctrl_cxt *cxt_ptr, struct awb_ctrl_calc_param *in_ptr, struct awb_ctrl_calc_result *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI(":ISP:process fun is NULL");
	}
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

cmr_int awb_ctrl_process(cmr_handle handle_awb, struct awb_ctrl_calc_param *param, struct awb_ctrl_calc_result *result)
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = (struct awbctrl_cxt *)handle_awb;

	ISP_CHECK_HANDLE_VALID(handle_awb);

	if (!param || !result) {
		ISP_LOGI(":ISP:input param is error 0x%lx", (cmr_uint)param);
		goto exit;
	}

	rtn = awbctrl_process(cxt_ptr, param, result);

exit:
	ISP_LOGI(":ISP: done %ld", rtn);
	return rtn;
}

cmr_int awb_ctrl_deinit(cmr_handle handle_awb)
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = (struct awbctrl_cxt *)handle_awb;

	ISP_CHECK_HANDLE_VALID(handle_awb);

	rtn = awbctrl_deinit_adpt(cxt_ptr);
	if (ISP_SUCCESS == rtn) {
		if (handle_awb) {
			free(handle_awb);
			handle_awb = NULL;
		}
	}

	ISP_LOGI(":ISP:done %d", rtn);
	return rtn;
}

cmr_int awb_ctrl_ioctrl(cmr_handle handle_awb, enum awb_ctrl_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	struct awbctrl_cxt *cxt_ptr = (struct awbctrl_cxt*)handle_awb;
	struct awbctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI(":ISP:ioctrl fun is NULL");
	}
exit:
	ISP_LOGI(":ISP:cmd = %d,done %ld", cmd, rtn);
	return rtn;
}
