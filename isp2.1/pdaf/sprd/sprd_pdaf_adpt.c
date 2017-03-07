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
#define LOG_TAG "pdaf_sprd_adpt"

#include <assert.h>
#include <stdlib.h>
#include <cutils/properties.h>

#include "sprd_pdaf_adpt.h"
#include "pdaf_ctrl.h"
#include "isp_adpt.h"
#include "dlfcn.h"
#include "pd_algo.h"
#include "cmr_common.h"
#include "af_ctrl.h"


/************************************ INTERNAK FUNCTION ***************************************/
struct sprd_pdaf_context {
	cmr_u32 camera_id;
	cmr_u8 pd_open;
	cmr_u8 is_busy;
	cmr_u32 frame_id;
	cmr_u32 token_id;
	cmr_u8 pd_enable;
	cmr_handle caller_handle;
	cmr_handle altek_lib_handle;
	cmr_handle extract_lib_handle;
	isp_pdaf_cb pdaf_set_cb;
	cmr_int (*pd_set_buffer) (struct pd_frame_in *cb_param);
	void *pd_left;
	void *pd_right;
	alPD_RECT roi;
	PDInReg pd_reg_in;
	cmr_u8 pd_reg_out[PD_REG_OUT_SIZE];
	cmr_u8 pdotp_pack_data[PD_OTP_PACK_SIZE];
 /*add for sharkl2 pdaf function*/
	void *caller;
	PD_GlobalSetting pd_gobal_setting;
	struct sprd_pdaf_report_t report_data;
	cmr_u32(*af_set_pdinfo) (void *handle, struct pd_result* in_parm);
};

cmr_handle sprd_pdaf_adpt_init(void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_ctrl_init_in *in_p = (struct pdaf_ctrl_init_in *)in;
	struct pdaf_ctrl_init_out *out_p = (struct pdaf_ctrl_init_out *)out;
	struct sprd_pdaf_context *cxt = NULL;
	struct sensor_otp_af_info *otp_af_info = NULL;
	struct isp_alg_fw_context *isp_ctx = (struct isp_alg_fw_context *)in_p->caller_handle;

	cmr_u32 pd_in_size = 0;

	if (!in_p) {
		ISP_LOGE("init param %p is null !!!", in_p);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
#if 0/*TBD get pd_info from sensor & set to isp*/
	if (NULL == in_p->pd_info) {
		ISP_LOGE("failed to get pd data");
		ret = ISP_PARAM_NULL;
		goto exit;
	}
#endif
	cxt = (struct sprd_pdaf_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc pdaf");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	cmr_bzero(cxt, sizeof(*cxt));
	cxt->caller = in_p->caller;
	//cxt->caller_handle = in_p->caller_handle;
	//cxt->pdaf_set_cb = in_p->pdaf_set_cb;
	cxt->af_set_pdinfo = in_p->af_set_pdinfo;
	//pdaf->init_in_param = *in_p;

	cxt->pd_left = malloc(pd_in_size);
	if (NULL == cxt->pd_left) {
		ISP_LOGE("failed to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	cxt->pd_right = malloc(pd_in_size);
	if (NULL == cxt->pd_right) {
		ISP_LOGE("failed to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	cmr_bzero(cxt->pd_left, pd_in_size);
	cmr_bzero(cxt->pd_right, pd_in_size);

	cxt->pd_gobal_setting.dImageW = IMAGE_WIDTH;/*TBD get from sensor*/
	cxt->pd_gobal_setting.dImageH = IMAGE_HEIGHT;/*TBD get from sensor*/
	cxt->pd_gobal_setting.dBeginX = BEGIN_X;/*TBD get from sensor*/
	cxt->pd_gobal_setting.dBeginY = BEGIN_Y;/*TBD get from sensor*/

	/*TBD dSensorID 0:for imx258 1: for OV13850*/
	cxt->pd_gobal_setting.dSensorMode = in_p->camera_id;
	cxt->pd_gobal_setting.dSensorMode = SENSOR_ID;/*TBD get from sensor id*/
#if 0 /*TBD init sharkle isp*/
	pdaf_setup(void * cxt);
#endif
	ret = PD_Init((void *)&cxt->pd_gobal_setting);

	ISP_LOGI("pdaf PD_Init end ret = %ld",ret);
	if (ret) {
		ISP_LOGE("failed to init lib %ld", ret);
		goto exit;
	}

	cxt->is_busy = 0;

	return (cmr_handle)cxt;

error_lib_init:
	//pdafaltek_libops_deinit(pdaf);
exit:
	if (cxt) {
		if (cxt->pd_right) {
			free(cxt->pd_right);
			cxt->pd_right = NULL;
		}
		if (cxt->pd_left) {
			free(cxt->pd_left);
			cxt->pd_left = NULL;
		}
		free(cxt);
		cxt = NULL;
	}
	return (cmr_handle)cxt;
}

static cmr_int sprd_pdaf_adpt_deinit(cmr_handle adpt_handle)
{
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	cmr_int ret = ISP_SUCCESS;

	ISP_LOGI("cxt = %p", cxt);
	if (cxt) {
		/* deinit lib */
		ret = PD_Uninit();

		if (cxt->pd_right) {
			free(cxt->pd_right);
			cxt->pd_right = NULL;
		}
		if (cxt->pd_left) {
			free(cxt->pd_left);
			cxt->pd_left = NULL;
		}
		cmr_bzero(cxt, sizeof(*cxt));
		free(cxt);
		cxt = NULL;
	}

	return ret;
}

static cmr_int sprd_pdaf_adpt_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	struct pdaf_ctrl_process_in *proc_in = (struct pdaf_ctrl_process_in *)in;
	struct pdaf_ctrl_callback_in callback_in;
	alGE_RECT pdroi;
	cmr_u16 image_width;
	cmr_u16 image_height;
	char value[PROPERTY_VALUE_MAX];

	UNUSED(out);
	bzero(&pdroi,sizeof(pdroi));
	if (!in) {
		ISP_LOGE("init param %p is null !!!", in);
		ret = ISP_PARAM_NULL;
		return ret;
	}

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	cmr_bzero(&callback_in, sizeof(callback_in));

#if 0
	if (!cxt->pd_enable || !cxt->pd_open) {
		ISP_LOGI("pd enable %d, open %d",cxt->pd_enable, cxt->pd_open);
		ret = ISP_SUCCESS;
		goto exit;
	}
#endif
	cxt->is_busy = 1;


	cxt->pd_reg_in.dcurrentVCM = proc_in->dcurrentVCM;
	cxt->pd_reg_in.dBv = (float)proc_in->dBv /1000.0;
	pdroi.m_wLeft = 0;
	pdroi.m_wTop = 0;

	void *pInPhaseBuf_left = NULL; //TODO
	void *pInPhaseBuf_right = NULL; //TODO
	int dRectX = ROI_X;
	int dRectY = ROI_Y;
	int dRectW = ROI_Width;
	int dRectH = ROI_Height;
	struct pd_result pd_calc_result;

	for(int area_index=0;area_index<AREA_LOOP;area_index++) {
		ret = PD_DoType2(pInPhaseBuf_left,pInPhaseBuf_right, dRectX, dRectY,
			    dRectW, dRectH,area_index);
		if (ret) {
			ISP_LOGE("failed to do pd algo.");
			goto exit;
		}
	}

	ISP_LOGI("pd report data pd_value: %f, reg_out %p",
			cxt->report_data.pd_value, cxt->report_data.pd_reg_out);

	cxt->report_data.token_id = cxt->token_id;
	cxt->report_data.frame_id = cxt->frame_id;
	cxt->report_data.enable = cxt->pd_enable;

	callback_in.proc_out.pd_report_data = cxt->report_data;
	/*call back to haf lib*/

#if 0	/*TBD get clac result from so & send to af*/
		ret = PD_GetResult(&pdConf[AREA_INDEX], &pdPhaseDiff[AREA_INDEX], &pdGetFrameID,
				      AREA_INDEX);
#endif
	cxt->af_set_pdinfo(cxt->caller, &pd_calc_result);
	cxt->frame_id ++;//= proc_in->pd_raw.frame_id;
exit:
	cxt->is_busy = 0;
	return ret;
}

static cmr_int sprd_pdaf_adpt_ioctrl(cmr_handle adpt_handle, cmr_int cmd,
				   void *in, void *out)
{
	cmr_int ret = -ISP_ERROR;
	struct pdaf_ctrl_param_in *in_ptr = (struct pdaf_ctrl_param_in *)in;
	struct pdaf_ctrl_param_out *out_ptr = (struct pdaf_ctrl_param_out *)out;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ISP_LOGV("cmd = %ld", cmd);
#if 0/*TBD*/
	switch (cmd) {
	case PDAF_CTRL_CMD_SET_OPEN:
		ret = pdafaltek_adpt_set_open(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_CLOSE:
		ret = pdafaltek_adpt_set_close(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_CONFIG:
		ret = pdafaltek_adpt_set_config(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_ENABLE:
		ret = pdafaltek_adpt_set_enable(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_GET_BUSY:
		ret = pdafaltek_adpt_get_busy(adpt_handle, out_ptr);
		break;
	default:
		ISP_LOGE("failed to case cmd = %ld", cmd);
		break;
	}
#endif
	ret = 0;
	return ret;
}

struct adpt_ops_type sprd_pdaf_adpt_ops = {
	.adpt_init = sprd_pdaf_adpt_init,
	.adpt_deinit = sprd_pdaf_adpt_deinit,
	.adpt_process = sprd_pdaf_adpt_process,
	.adpt_ioctrl = sprd_pdaf_adpt_ioctrl,
};

