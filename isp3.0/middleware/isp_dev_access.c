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
#define LOG_TAG "isp_dev_access"

#include <stdlib.h>
#include "cutils/properties.h"
#include <unistd.h>
#include "isp_common_types.h"
#include "isp_dev_access.h"
#include "isp_drv.h"

/**************************************** MACRO DEFINE *****************************************/
#define ISP_BRITNESS_GAIN_0            50
#define ISP_BRITNESS_GAIN_1            70
#define ISP_BRITNESS_GAIN_2            90
#define ISP_BRITNESS_GAIN_3            100
#define ISP_BRITNESS_GAIN_4            120
#define ISP_BRITNESS_GAIN_5            160
#define ISP_BRITNESS_GAIN_6            200
/************************************* INTERNAL DATA TYPE ***************************************/
struct iommu_mem {
	cmr_uint virt_addr;
	cmr_uint phy_addr;
	cmr_int mfd;
	cmr_u32 size;
	cmr_u32 num;
};

struct isp_dev_access_context {
	cmr_u32 camera_id;
	cmr_u32 is_inited;
	cmr_handle caller_handle;
	cmr_handle isp_driver_handle;
	struct isp_dev_init_in input_param;
	struct iommu_mem fw_buffer;
};

/*************************************INTERNAK FUNCTION ***************************************/




/*************************************EXTERNAL FUNCTION ***************************************/
cmr_int isp_dev_access_init(struct isp_dev_init_in *input_ptr, cmr_handle *isp_dev_handle)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = NULL;
	struct isp_dev_init_info               input;
	struct isp_init_mem_param              load_input;
	cmr_int                                fw_size =0;
	cmr_u32                                fw_buf_num = 1;

	if (!input_ptr || !isp_dev_handle) {
		ISP_LOGE("input is NULL, 0x%lx", (cmr_int)input_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*isp_dev_handle = NULL;
	cxt = (struct isp_dev_access_context*)malloc(sizeof(struct isp_dev_access_context));
	if (NULL == cxt) {
		ISP_LOGE("failed to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	ISP_LOGV("input_ptr->camera_id %d\n", input_ptr->camera_id);
	cmr_bzero(cxt, sizeof(*cxt));
	cxt->camera_id = input_ptr->camera_id;
	cxt->caller_handle = input_ptr->caller_handle;

	input.camera_id = input_ptr->camera_id;
	input.width = input_ptr->init_param.size.w;
	input.height = input_ptr->init_param.size.h;
	input.alloc_cb = input_ptr->init_param.alloc_cb;
	input.free_cb = input_ptr->init_param.free_cb;
	input.mem_cb_handle = input_ptr->mem_cb_handle;
	memcpy(&cxt->input_param, input_ptr, sizeof(struct isp_dev_init_in));

	ret= isp_dev_init(&input, &cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("failed to dev initialized");
	}

	ret = isp_dev_start(cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("failed to dev_start %ld", ret);
		goto exit;
	}
exit:
	if (ret) {
		if (cxt) {
			isp_dev_deinit(cxt->isp_driver_handle);
			free((void*)cxt);
		}
	} else {
		cxt->is_inited = 1;
		*isp_dev_handle = (cmr_handle)cxt;
	}
	return ret;
}

cmr_int isp_dev_access_deinit(cmr_handle isp_dev_handle)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context*)isp_dev_handle;

	ISP_CHECK_HANDLE_VALID(isp_dev_handle);

	ret = isp_dev_deinit(cxt->isp_driver_handle);
	free((void*)cxt);
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_capability(cmr_handle isp_dev_handle, enum isp_capbility_cmd cmd, void* param_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context*)isp_dev_handle;;
	struct isp_video_limit                 *limit = (struct isp_video_limit*)param_ptr;
	struct isp_img_size                    size = {0, 0};

	limit->width = 0;
	limit->height = 0;
	switch (cmd) {
	case ISP_VIDEO_SIZE:
	//	limit->width  = 1920; //TBD
	//	limit->height = 1080; //TBD
		ret = isp_dev_capability_video_size(cxt->isp_driver_handle, &size);
		if (!ret) {
			limit->width = (cmr_u16)size.width;
			limit->height = (cmr_u16)size.height;
		}
		break;
	case ISP_CAPTURE_SIZE:
	//	limit->width  = 1920; //TBD
	//	limit->height = 1080; //TBD
		ret = isp_dev_capability_single_size(cxt->isp_driver_handle, &size);
		if (!ret) {
			limit->width = (cmr_u16)size.width;
			limit->height = (cmr_u16)size.height;
		}
		break;
    case ISP_REG_VAL:
		ISP_LOGI("don't support");
		break;
	default:
		break;
	}
	ISP_LOGI("width height %d %d,", limit->width, limit->height);

	return ret;
}

cmr_u32 _isp_dev_access_convert_saturation(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_SATURATION_LV4;

	switch (value) {
	case 0:
		convert_value = ISP_SATURATION_LV1;
		break;
	case 1:
		convert_value = ISP_SATURATION_LV2;
		break;
	case 2:
		convert_value = ISP_SATURATION_LV3;
		break;
	case 3:
		convert_value = ISP_SATURATION_LV4;
		break;
	case 4:
		convert_value = ISP_SATURATION_LV5;
		break;
	case 5:
		convert_value = ISP_SATURATION_LV6;
		break;
	case 6:
		convert_value = ISP_SATURATION_LV7;
		break;
	case 7:
		convert_value = ISP_SATURATION_LV8;
	default:
		ISP_LOGI("don't support saturation %d", value);
	}
	ISP_LOGI("set saturation %d", convert_value);
	return convert_value;
}

cmr_u32 _isp_dev_access_convert_contrast(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_CONTRAST_LV4;

	switch (value) {
	case 0:
		convert_value = ISP_CONTRAST_LV1;
		break;
	case 1:
		convert_value = ISP_CONTRAST_LV2;
		break;
	case 2:
		convert_value = ISP_CONTRAST_LV3;
		break;
	case 3:
		convert_value = ISP_CONTRAST_LV4;
		break;
	case 4:
		convert_value = ISP_CONTRAST_LV5;
		break;
	case 5:
		convert_value = ISP_CONTRAST_LV6;
		break;
	case 6:
		convert_value = ISP_CONTRAST_LV7;
		break;
	case 7:
		convert_value = ISP_CONTRAST_LV8;
		break;
	default:
		ISP_LOGE("don't support contrast %d", value);
		break;
	}
	ISP_LOGI("set contrast %d", convert_value);
	return convert_value;
}

cmr_u32 _isp_dev_access_convert_sharpness(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_SHARPNESS_LV4;

	switch (value) {
	case 0:
		convert_value = ISP_SHARPNESS_LV1;
		break;
	case 1:
		convert_value = ISP_SHARPNESS_LV2;
		break;
	case 2:
		convert_value = ISP_SHARPNESS_LV3;
		break;
	case 3:
		convert_value = ISP_SHARPNESS_LV4;
		break;
	case 4:
		convert_value = ISP_SHARPNESS_LV5;
		break;
	case 5:
		convert_value = ISP_SHARPNESS_LV6;
		break;
	case 6:
		convert_value = ISP_SHARPNESS_LV7;
		break;
	case 7:
		convert_value = ISP_SHARPNESS_LV8;
		break;
	default:
		ISP_LOGE("don't support sharpness %d", value);
		break;
	}
	ISP_LOGI("set sharpness %d", convert_value);
	return convert_value;
}
cmr_u32 _isp_dev_access_convert_effect(cmr_u32 value)
{
	cmr_u32 convert_value = ISP_SPECIAL_EFFECT_OFF;

	switch (value) {
	case 0://CAMERA_EFFECT_NONE
		convert_value = ISP_SPECIAL_EFFECT_OFF;
		break;
	case 1://CAMERA_EFFECT_MONO
		convert_value = ISP_SPECIAL_EFFECT_GRAYSCALE;
		break;
	case 2://CAMERA_EFFECT_RED
	case 3://CAMERA_EFFECT_GREEN
	case 4://CAMERA_EFFECT_BLUE
	case 5://CAMERA_EFFECT_YELLOW
		ISP_LOGE("don't support effect %d", value);
		break;
	case 6://CAMERA_EFFECT_NEGATIVE
		convert_value = ISP_SPECIAL_EFFECT_NEGATIVE;
		break;
	case 7://CAMERA_EFFECT_SEPIA
		convert_value = ISP_SPECIAL_EFFECT_SEPIA;
		break;
	default:
		ISP_LOGI("don't support %d", value);
		break;
	}
	ISP_LOGI("set effect %d", convert_value);
	return convert_value;
}

cmr_int isp_dev_access_set_brightness(cmr_handle isp_dev_handle, union isp_dev_ctrl_cmd_in *input_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                mode = 0;
	struct isp_brightness_gain             brightness_gain;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context*)isp_dev_handle;

	brightness_gain.uw_gain[0] = ISP_BRITNESS_GAIN_0;
	brightness_gain.uw_gain[1] = ISP_BRITNESS_GAIN_1;
	brightness_gain.uw_gain[2] = ISP_BRITNESS_GAIN_2;
	brightness_gain.uw_gain[3] = ISP_BRITNESS_GAIN_3;
	brightness_gain.uw_gain[4] = ISP_BRITNESS_GAIN_4;
	brightness_gain.uw_gain[5] = ISP_BRITNESS_GAIN_5;
	brightness_gain.uw_gain[6] = ISP_BRITNESS_GAIN_6;
	switch (input_ptr->value) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		mode = input_ptr->value;
		break;
	default:
		ISP_LOGE("don't support %d", input_ptr->value);
		goto exit;
	}
	ret = isp_dev_cfg_brightness_gain(cxt->isp_driver_handle, &brightness_gain);
	if (ret) {
		ISP_LOGE("failed to set brightness gain %ld", ret);
		goto exit;
	}
	ret = isp_dev_cfg_brightness_mode(cxt->isp_driver_handle, mode);
	ISP_LOGI("mode %d, done %ld", mode, ret);
exit:
	return ret;
}

cmr_int _isp_dev_access_set_ccm(cmr_handle isp_dev_handle, union isp_dev_ctrl_cmd_in *input_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                i = 0, len = MIN(IQ_CCM_INFO, CCM_TABLE_LEN);
	struct isp_iq_ccm_info                 ccm_info;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context*)isp_dev_handle;

	for ( i= 0 ; i<len ; i++) {
		ccm_info.ad_ccm[i] = input_ptr->ccm_table[i];
	}
	ret = isp_dev_cfg_ccm(cxt->isp_driver_handle, &ccm_info);

	return ret;
}

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, enum isp_dev_access_ctrl_cmd cmd, union isp_dev_ctrl_cmd_in *input_ptr, union isp_dev_ctrl_cmd_out *output_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                value = 0;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context*)isp_dev_handle;

	ISP_CHECK_HANDLE_VALID(isp_dev_handle);

	ISP_LOGI("cmd: %d", cmd);
	switch (cmd) {
	case ISP_DEV_ACCESS_SET_AWB_GAIN:
		break;
	case ISP_DEV_ACCESS_SET_ISP_SPEED:
		break;
	case ISP_DEV_ACCESS_SET_3A_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_AE_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_AWB_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_AF_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_ANTIFLICKER:
		break;
	case ISP_DEV_ACCESS_SET_YHIS_PARAM:
		break;
	case ISP_DEV_ACCESS_SET_BRIGHTNESS:
		ret = isp_dev_access_set_brightness(isp_dev_handle, input_ptr);
		break;
	case ISP_DEV_ACCESS_SET_SATURATION:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		value = _isp_dev_access_convert_saturation(input_ptr->value);
		ret = isp_dev_cfg_saturation(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_CONSTRACT:
		if (!input_ptr) {
			ISP_LOGI("failed to set contrast,input is null");
			break;
		}
		value = _isp_dev_access_convert_contrast(input_ptr->value);
		ret = isp_dev_cfg_contrast(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_SHARPNESS:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		value = _isp_dev_access_convert_sharpness(input_ptr->value);
		ret = isp_dev_cfg_sharpness(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_SPECIAL_EFFECT:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		value = _isp_dev_access_convert_effect(input_ptr->value);
		ret = isp_dev_cfg_special_effect(cxt->isp_driver_handle, value);
		break;
	case ISP_DEV_ACCESS_SET_SUB_SAMPLE:
		break;
	case ISP_DEV_ACCESS_SET_STATIS_BUF:
		break;
	case ISP_DEV_ACCESS_SET_COLOR_TEMP:
		ISP_LOGV("color temp %d", input_ptr->value);
		ret = isp_dev_cfg_color_temp(cxt->isp_driver_handle, input_ptr->value);
		break;
	case ISP_DEV_ACCESS_SET_CCM:
		ret = _isp_dev_access_set_ccm(isp_dev_handle, input_ptr);
		break;
	case ISP_DEV_ACCESS_GET_STATIS_BUF:
		break;
	case ISP_DEV_ACCESS_GET_TIME:
		if (!output_ptr) {
			ISP_LOGI("failed to get dev time,output is null");
			break;
		}
		ret = isp_dev_get_timestamp(cxt->isp_driver_handle, &output_ptr->time.sec, &output_ptr->time.usec);
		break;
	default:
		ISP_LOGI("don't support %d", cmd);
		break;
	}
	return ret;
}

static void isp_dev_access_set_af_hw_cfg(AF_CfgInfo *af_cfg_info, struct isp3a_af_hw_cfg *af_hw_cfg)
{
	cmr_s32 i = 0;
	memcpy(af_cfg_info, af_hw_cfg, sizeof(AF_CfgInfo));

	ISP_LOGV("token_id = %d", af_hw_cfg->token_id);
	ISP_LOGV("uw_size_ratio_x = %d uw_size_ratio_y = %d uw_blk_num_x = %d uw_blk_num_y = %d",
		af_hw_cfg->af_region.uw_size_ratio_x, af_hw_cfg->af_region.uw_size_ratio_y,
		af_hw_cfg->af_region.uw_blk_num_x, af_hw_cfg->af_region.uw_blk_num_y);
	ISP_LOGV("uw_offset_ratio_x = %d uw_offset_ratio_y = %d",
		af_hw_cfg->af_region.uw_offset_ratio_x, af_hw_cfg->af_region.uw_offset_ratio_y);
	ISP_LOGV("enable_af_lut = %d", af_hw_cfg->enable_af_lut);

	for (i = 0; i <259; i++)
	    ISP_LOGV("auw_lut[%d] = %d", i, af_hw_cfg->auw_lut[i]);
	for (i = 0; i <259; i++)
	    ISP_LOGV("auw_af_lut[%d] = %d", i, af_hw_cfg->auw_af_lut[i]);
	for (i = 0; i <6; i++)
	    ISP_LOGV("auc_weight[%d] = %d", i, af_hw_cfg->auc_weight[i]);

	ISP_LOGV("uw_sh = %d", af_hw_cfg->uw_sh);
	ISP_LOGV("uc_th_mode = %d", af_hw_cfg->uc_th_mode);
	for (i = 0; i <82; i++)
	    ISP_LOGV("auc_index[%d] = %d", i, af_hw_cfg->auc_index[i]);
	for (i = 0; i <4; i++)
	    ISP_LOGV("auw_th[%d] = %d", i, af_hw_cfg->auw_th[i]);
	for (i = 0; i <4; i++)
	    ISP_LOGV("pw_tv[%d] = %d", i, af_hw_cfg->pw_tv[i]);
	ISP_LOGV("ud_af_offset = %d", af_hw_cfg->ud_af_offset);
	ISP_LOGV("af_py_enable= %d", af_hw_cfg->af_py_enable);
	ISP_LOGV("af_lpf_enable = %d", af_hw_cfg->af_lpf_enable);
	ISP_LOGV("filter_mode = %ld", af_hw_cfg->filter_mode);
	ISP_LOGV("uc_filter_id = %d", af_hw_cfg->uc_filter_id);
	ISP_LOGV("uw_ine_cnt = %d", af_hw_cfg->uw_ine_cnt);
}

void isp_dev_access_convert_ae_param(struct isp3a_ae_hw_cfg *from, AE_CfgInfo *to)
{
	if (!from || !to) {
		ISP_LOGE("param %p %p is NULL !!!", from, to);
		return;
	}
	to->TokenID = from->tokenID;
	to->tAERegion.uwBorderRatioX = from->region.border_ratio_X;
	to->tAERegion.uwBorderRatioY = from->region.border_ratio_Y;
	to->tAERegion.uwBlkNumX = from->region.blk_num_X;
	to->tAERegion.uwBlkNumY = from->region.blk_num_Y;
	to->tAERegion.uwOffsetRatioX = from->region.offset_ratio_X;
	to->tAERegion.uwOffsetRatioY = from->region.offset_ratio_Y;
}

void isp_dev_access_convert_afl_param(struct isp3a_afl_hw_cfg *from, AntiFlicker_CfgInfo *to)
{
	if (!from || !to) {
		ISP_LOGE("param %p %p is NULL !!!", from, to);
		return;
	}
	to->TokenID = from->token_id;
	to->uwOffsetRatioX = from->offset_ratiox;
	to->uwOffsetRatioY = from->offset_ratioy;
}

void isp_dev_access_convert_awb_param(struct isp3a_awb_hw_cfg *data, AWB_CfgInfo *awb_param)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_u32                                i = 0;

	//memcpy(&awb_param, data, sizeof(AWB_CfgInfo));
	awb_param->TokenID = data->token_id;
	awb_param->tAWBRegion.uwBlkNumX = data->region.blk_num_X;
	awb_param->tAWBRegion.uwBlkNumY = data->region.blk_num_Y;
	awb_param->tAWBRegion.uwBorderRatioX = data->region.border_ratio_X;
	awb_param->tAWBRegion.uwBorderRatioY = data->region.border_ratio_Y;
	awb_param->tAWBRegion.uwOffsetRatioX = data->region.offset_ratio_X;
	awb_param->tAWBRegion.uwOffsetRatioY = data->region.offset_ratio_Y;
	for (i=0 ; i<AWB_UCYFACTOR_NUM ; i++) {
		awb_param->ucYFactor[i] = data->uc_factor[i];
	}
	for (i=0 ; i<AWB_BBRFACTOR_NUM ; i++) {
		awb_param->BBrFactor[i] = data->bbr_factor[i];
	}
	awb_param->uwRGain = data->uw_rgain;
	awb_param->uwGGain = data->uw_ggain;
	awb_param->uwBGain = data->uw_bgain;
	awb_param->ucCrShift = data->uccr_shift;
	awb_param->ucOffsetShift = data->uc_offset_shift;
	awb_param->ucQuantize = data->uc_quantize;
	awb_param->ucDamp = data->uc_damp;
	awb_param->ucSumShift = data->uc_sum_shift;
	awb_param->tHis.bEnable = data->t_his.benable;
	awb_param->tHis.cCrEnd = data->t_his.ccrend;
	awb_param->tHis.cCrPurple = data->t_his.ccrpurple;
	awb_param->tHis.cCrStart = data->t_his.ccrstart;
	awb_param->tHis.cGrassEnd = data->t_his.cgrass_end;
	awb_param->tHis.cGrassOffset = data->t_his.cgrass_offset;
	awb_param->tHis.cGrassStart = data->t_his.cgrass_start;
	awb_param->tHis.cOffsetDown = data->t_his.coffsetdown;
	awb_param->tHis.cOffsetUp = data->t_his.cooffsetup;
	awb_param->tHis.cOffset_bbr_w_end = data->t_his.coffset_bbr_w_end;
	awb_param->tHis.cOffset_bbr_w_start = data->t_his.coffset_bbr_w_start;
	awb_param->tHis.dHisInterp = data->t_his.dhisinterp;
	awb_param->tHis.ucDampGrass = data->t_his.ucdampgrass;
	awb_param->tHis.ucOffsetPurPle = data->t_his.ucoffsetpurple;
	awb_param->tHis.ucYFac_w = data->t_his.ucyfac_w;
	awb_param->uwRLinearGain = data->uwrlinear_gain;
	awb_param->uwBLinearGain = data->uwblinear_gain;
	ISP_LOGE("token_id = %d, uccr_shift = %d, uc_damp = %d, uc_offset_shift = %d\n",
		awb_param->TokenID, awb_param->ucCrShift, awb_param->ucDamp, awb_param->ucOffsetShift);
	ISP_LOGV("uc_quantize = %d\n, uc_sum_shift = %d\n, uwblinear_gain = %d\n, uwrlinear_gain = %d\n,\
		uw_bgain = %d\n, uw_rgain = %d\n, uw_ggain = %d\n",
		awb_param->ucQuantize, awb_param->ucSumShift, awb_param->uwBLinearGain, awb_param->uwRLinearGain,
		awb_param->uwBGain, awb_param->uwRGain, awb_param->uwGGain);
	for (i=0 ; i<16 ; i++) {
		ISP_LOGV("uc_factor[%d] = %d\n", i, awb_param->ucYFactor[i]);
	}
	for (i=0 ; i<33 ; i++) {
		ISP_LOGV("bbr_factor[%d] = %d\n",i, awb_param->BBrFactor[i]);
	}
	ISP_LOGV("region:blk_num_X = %d, blk_num_Y = %d\n, border_ratio_X = %d, border_ratio_Y = %d\n,\
		offset_ratio_X = %d, offset_ratio_Y = %d\n",
		awb_param->tAWBRegion.uwBlkNumX, awb_param->tAWBRegion.uwBlkNumY,
		awb_param->tAWBRegion.uwBorderRatioX, awb_param->tAWBRegion.uwBorderRatioY,
		awb_param->tAWBRegion.uwOffsetRatioX, awb_param->tAWBRegion.uwOffsetRatioY);
	ISP_LOGV("t_his:benable = %d\n ccrend = %d\n ccrpurple = %d\n ccrstart = %d\n \
		cgrass_end = %d\n cgrass_offset = %d\n cgrass_start = %d\n coffsetdown = %d\n \
		coffset_bbr_w_end = %d\n coffset_bbr_w_start = %d\n cooffsetup = %d\n \
		dhisinterp = %d\n ucdampgrass = %d\n ucoffsetpurple = %d\n ucyfac_w = %d\n",
		awb_param->tHis.bEnable, awb_param->tHis.cCrEnd, awb_param->tHis.cCrPurple,
		awb_param->tHis.cCrStart, awb_param->tHis.cGrassEnd, awb_param->tHis.cGrassOffset,
		awb_param->tHis.cGrassStart, awb_param->tHis.cOffsetDown, awb_param->tHis.cOffset_bbr_w_end,
		awb_param->tHis.cOffset_bbr_w_start, awb_param->tHis.cOffsetUp, awb_param->tHis.dHisInterp,
		awb_param->tHis.ucDampGrass, awb_param->tHis.ucOffsetPurPle, awb_param->tHis.ucYFac_w);

}
#define FPGA_TEST     1
cmr_int isp_dev_access_start_multiframe(cmr_handle isp_dev_handle, struct isp_dev_access_start_in* param_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	cmr_int                                i = 0;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	SCENARIO_INFO_AP                       input_data;
	Cfg3A_Info                             cfg3a_info;
	DldSequence                            dldseq_info;
	cmr_u32                                iso_gain = 0;
	struct isp_awb_gain_info               awb_gain_info;
	cmr_u32                                dcam_id = 0;
	struct isp_sensor_resolution_info      *resolution_ptr = NULL;
#ifdef FPGA_TEST
	struct isp_cfg_img_param               img_buf_param;
	cmr_u32 img_size, cnt;
	cmr_u8 a_tAFInfo_aucWeight[6] = {0, 2, 6, 4, 1, 2};
	cmr_u8 a_tAFInfo_aucIndex[82] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	u16 a_tAFInfo_auwTH[4] = {0, 0, 0, 0};
	u16 a_tAFInfo_pwTV[4] = {0, 0, 0, 0};
	SCENARIO_INFO_AP tSecnarioInfo;
#endif
	ISP_CHECK_HANDLE_VALID(isp_dev_handle);

	isp_dev_set_capture_mode(cxt->isp_driver_handle, param_ptr->common_in.capture_mode);

	memset(&tSecnarioInfo, 0, sizeof(tSecnarioInfo));
	resolution_ptr = &param_ptr->common_in.resolution_info;
	tSecnarioInfo.tSensorInfo.uwOriginalWidth = resolution_ptr->sensor_max_size.w;
	tSecnarioInfo.tSensorInfo.uwOriginalHeight = resolution_ptr->sensor_max_size.h;
	tSecnarioInfo.tSensorInfo.uwWidth = resolution_ptr->sensor_output_size.w;
	tSecnarioInfo.tSensorInfo.uwHeight = resolution_ptr->sensor_output_size.h;
	tSecnarioInfo.tSensorInfo.uwCropStartX = resolution_ptr->crop.st_x;
	tSecnarioInfo.tSensorInfo.uwCropStartY = resolution_ptr->crop.st_y;
	tSecnarioInfo.tSensorInfo.uwCropEndX = resolution_ptr->sensor_max_size.w + resolution_ptr->crop.st_x - 1;
	tSecnarioInfo.tSensorInfo.uwCropEndY = resolution_ptr->sensor_max_size.h + resolution_ptr->crop.st_y - 1;
	tSecnarioInfo.tSensorInfo.udLineTime = resolution_ptr->line_time*10;
	tSecnarioInfo.tSensorInfo.uwClampLevel = cxt->input_param.init_param.ex_info.clamp_level;
	tSecnarioInfo.tSensorInfo.uwFrameRate = resolution_ptr->fps.max_fps*100;
#ifndef FPGA_TEST
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &input_data);//TBD
	if (ret) {
		ISP_LOGE("failed to cfg scenatio info %ld", ret);
		goto exit;
	}
#endif

#ifdef FPGA_TEST
	tSecnarioInfo.tSensorInfo.ucSensorMode = 0;
	tSecnarioInfo.tSensorInfo.ucSensorMouduleType = 0;//0-sensor1  1-sensor2
	tSecnarioInfo.tSensorInfo.nColorOrder = cxt->input_param.init_param.image_pattern;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassLV = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassVideo = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassStill = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassMetaData = 0;

	tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutWidth = 0;
	tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutHeight = 0;
	ISP_LOGI("size %dx%d, line time %d frameRate %d clamp %d image_pattern %d ", tSecnarioInfo.tSensorInfo.uwWidth, tSecnarioInfo.tSensorInfo.uwHeight,
		tSecnarioInfo.tSensorInfo.udLineTime, tSecnarioInfo.tSensorInfo.uwFrameRate, tSecnarioInfo.tSensorInfo.uwClampLevel, tSecnarioInfo.tSensorInfo.nColorOrder);
	ISP_LOGI("sensor out %d %d %d %d %d %d",tSecnarioInfo.tSensorInfo.uwCropStartX,
		tSecnarioInfo.tSensorInfo.uwCropStartY, tSecnarioInfo.tSensorInfo.uwCropEndX,
		tSecnarioInfo.tSensorInfo.uwCropEndY, tSecnarioInfo.tSensorInfo.uwOriginalWidth,
		tSecnarioInfo.tSensorInfo.uwOriginalHeight);
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &tSecnarioInfo);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("isp_dev_cfg_scenario_info error %ld", ret);
		return -1;
	}

	/*set still image buffer format*/
	memset(&img_buf_param, 0, sizeof(img_buf_param));

	img_buf_param.format = ISP_OUT_IMG_YUY2;
	img_buf_param.img_id = ISP_IMG_STILL_CAPTURE;
	img_buf_param.dram_eb = 0;
	img_buf_param.buf_num = 4;
	img_buf_param.width = cxt->input_param.init_param.size.w;
	img_buf_param.height = cxt->input_param.init_param.size.h;
	img_buf_param.line_offset = (2 * cxt->input_param.init_param.size.w);
	img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
	ISP_LOGE("set still image buffer param img_id %d", img_buf_param.img_id);
	ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);

#ifdef FPGA_TEST
	// SubSample
	cfg3a_info.tSubSampleInfo.TokenID = 0x100;
	cfg3a_info.tSubSampleInfo.udBufferImageSize = 320*240;
	cfg3a_info.tSubSampleInfo.uwOffsetRatioX =0;
	cfg3a_info.tSubSampleInfo.uwOffsetRatioY =0;


	// Yhis configuration:
	cfg3a_info.tYHisInfo.TokenID = 0x120;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBorderRatioX = 100;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBorderRatioY = 100;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBlkNumX = 16;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBlkNumY = 16;
	cfg3a_info.tYHisInfo.tYHisRegion.uwOffsetRatioX = 0;
	cfg3a_info.tYHisInfo.tYHisRegion.uwOffsetRatioY = 0;
#else
	memcpy(&cfg3a_info.hw_cfg.tAntiFlickerInfo, &param_ptr->hw_cfg.afl_cfg, sizeof(AntiFlicker_CfgInfo));
	memcpy(&cfg3a_info.hw_cfg.tYHisInfo, &param_ptr->hw_cfg.yhis_cfg, sizeof(YHis_CfgInfo);
	memcpy(&cfg3a_info.hw_cfg.tSubSampleInfo, &param_ptr->hw_cfg.subsample_cfg, sizeof(SubSample_CfgInfo));
#endif
#endif

	isp_dev_access_convert_ae_param(&param_ptr->hw_cfg.ae_cfg, &cfg3a_info.tAEInfo);
	isp_dev_access_convert_afl_param(&param_ptr->hw_cfg.afl_cfg, &cfg3a_info.tAntiFlickerInfo);
	isp_dev_access_convert_awb_param(&param_ptr->hw_cfg.awb_cfg, &cfg3a_info.tAWBInfo);
	isp_dev_access_set_af_hw_cfg(&cfg3a_info.tAFInfo, &param_ptr->hw_cfg.af_cfg);

	ret = isp_dev_cfg_3a_param(cxt->isp_driver_handle, &cfg3a_info);
	if (ret) {
		ISP_LOGE("failed to cfg 3a %ld", ret);
		goto exit;
	}
#ifdef FPGA_TEST
	dldseq_info.ucPreview_Baisc_DldSeqLength = 4;
	dldseq_info.aucPreview_Baisc_DldSeq[0] = 1;
	dldseq_info.aucPreview_Baisc_DldSeq[1] = 1;
	dldseq_info.aucPreview_Baisc_DldSeq[2] = 1;
	dldseq_info.aucPreview_Baisc_DldSeq[3] = 1;

	dldseq_info.ucPreview_Adv_DldSeqLength = 3;
	dldseq_info.aucPreview_Adv_DldSeq[0] = 1;
	dldseq_info.aucPreview_Adv_DldSeq[1] = 1;
	dldseq_info.aucPreview_Adv_DldSeq[2] = 1;

	dldseq_info.ucFastConverge_Baisc_DldSeqLength = 2;
	dldseq_info.aucFastConverge_Baisc_DldSeq[0] = 3;
	dldseq_info.aucFastConverge_Baisc_DldSeq[1] = 3;
#else
	memcpy(&dldseq_info, &param_ptr->dld_seq, sizeof(DldSequence));
#endif
	ret = isp_dev_cfg_dld_seq(cxt->isp_driver_handle, &dldseq_info);
	if (ret) {
		ISP_LOGE("failed to cfg dld sequence %ld", ret);
		goto exit;
	}

	iso_gain = 100;
	ret = isp_dev_cfg_iso_speed(cxt->isp_driver_handle, &iso_gain);//TBD ???
	if (ret) {
		ISP_LOGE("failed to cfg iso speed %ld", ret);
		goto exit;
	}

	awb_gain_info.r = param_ptr->hw_cfg.awb_gain.r;
	awb_gain_info.g = param_ptr->hw_cfg.awb_gain.g;
	awb_gain_info.b = param_ptr->hw_cfg.awb_gain.b;
	ret = isp_dev_cfg_awb_gain(cxt->isp_driver_handle, &awb_gain_info);
	if (ret) {
		ISP_LOGE("failed to set awb gain %ld", ret);
		goto exit;
	}
	awb_gain_info.r = param_ptr->hw_cfg.awb_gain_balanced.r;
	awb_gain_info.g = param_ptr->hw_cfg.awb_gain_balanced.g;
	awb_gain_info.b = param_ptr->hw_cfg.awb_gain_balanced.b;
	ret = isp_dev_cfg_awb_gain_balanced(cxt->isp_driver_handle, &awb_gain_info);

#if 0
	if (0 == cxt->camera_id)
		dcam_id = 0;
	else if (2 == cxt->camera_id || 1 == cxt->camera_id)
		dcam_id = 1;
	ISP_LOGI("dcam_id %d", dcam_id);
	ret = isp_dev_set_dcam_id(cxt->isp_driver_handle, dcam_id);
#endif
	ret = isp_dev_stream_on(cxt->isp_driver_handle);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_stop_multiframe(cmr_handle isp_dev_handle)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_stream_off(cxt->isp_driver_handle);
	ret = isp_dev_stop(cxt->isp_driver_handle);

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_dev_access_start_postproc(cmr_handle isp_dev_handle, struct isp_dev_postproc_in* input_ptr, struct isp_dev_postproc_out* output_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	SCENARIO_INFO_AP                       scenario_in;
	Cfg3A_Info                             cfg_info;
	struct isp_cfg_img_param               img_param;
	struct isp_awb_gain_info               awb_gain;
	DldSequence                            dldseq;
	cmr_u32                                dcam_id = 0;
	struct isp_cfg_img_param               img_buf_param;
	cmr_u32                                iso_gain = 0;
	struct isp_raw_data                    isp_raw_mem;
	struct isp_img_mem                     img_mem;
	cmr_int                                cnt = 0;
	struct isp_sensor_resolution_info      *resolution_ptr = NULL;

	ISP_CHECK_HANDLE_VALID(isp_dev_handle);
	ret = isp_dev_start(cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("failed to dev_start %ld", ret);
		goto exit;
	}

	memset(&isp_raw_mem, 0, sizeof(struct isp_raw_data));
	isp_raw_mem.capture_mode = ISP_CAP_MODE_RAW_DATA;
	isp_dev_set_capture_mode(cxt->isp_driver_handle, isp_raw_mem.capture_mode);

	ISP_LOGE("isp_raw_data.capture_mode = %d", isp_raw_mem.capture_mode);
	isp_raw_mem.fd = input_ptr->dst2_frame.img_fd.y;
	isp_raw_mem.phy_addr = input_ptr->dst2_frame.img_addr_phy.chn0;
	isp_raw_mem.virt_addr = input_ptr->dst2_frame.img_addr_vir.chn0;
	isp_raw_mem.size = (input_ptr->dst2_frame.img_size.w*input_ptr->dst2_frame.img_size.h)*4*2/3;
	isp_raw_mem.width = input_ptr->dst2_frame.img_size.w;
	isp_raw_mem.height = input_ptr->dst2_frame.img_size.h;
	ret = isp_dev_set_rawaddr(cxt->isp_driver_handle, &isp_raw_mem);
	ISP_LOGE("raw10_buf fd 0x%x phy_addr 0x%x virt_addr 0x%x", isp_raw_mem.fd,
		isp_raw_mem.phy_addr, isp_raw_mem.virt_addr);

	memset(&img_mem, 0, sizeof(struct isp_img_mem));
	img_mem.img_fmt = input_ptr->dst_frame.img_fmt;
	img_mem.yaddr = input_ptr->dst_frame.img_addr_phy.chn0;
	img_mem.uaddr = input_ptr->dst_frame.img_addr_phy.chn1;
	img_mem.vaddr = input_ptr->dst_frame.img_addr_phy.chn2;
	img_mem.yaddr_vir = input_ptr->dst_frame.img_addr_vir.chn0;
	img_mem.uaddr_vir = input_ptr->dst_frame.img_addr_vir.chn1;
	img_mem.vaddr_vir = input_ptr->dst_frame.img_addr_vir.chn2;
	img_mem.img_y_fd = input_ptr->dst_frame.img_fd.y;
	img_mem.img_u_fd = input_ptr->dst_frame.img_fd.u;
	img_mem.width = input_ptr->dst_frame.img_size.w;
	img_mem.height = input_ptr->dst_frame.img_size.h;
	ISP_LOGE("yuv_raw fd 0x%x yaddr 0x%lx yaddr_vir 0x%lx", img_mem.img_y_fd, img_mem.yaddr, img_mem.yaddr_vir);
	isp_dev_set_post_yuv_mem(cxt->isp_driver_handle, &img_mem);

	memset(&img_mem, 0, sizeof(struct isp_img_mem));
	img_mem.img_fmt = input_ptr->src_frame.img_fmt;
	img_mem.yaddr = input_ptr->src_frame.img_addr_phy.chn0;
	img_mem.uaddr = input_ptr->src_frame.img_addr_phy.chn1;
	img_mem.vaddr = input_ptr->src_frame.img_addr_phy.chn2;
	img_mem.yaddr_vir = input_ptr->src_frame.img_addr_vir.chn0;
	img_mem.uaddr_vir = input_ptr->src_frame.img_addr_vir.chn1;
	img_mem.vaddr_vir = input_ptr->src_frame.img_addr_vir.chn2;
	img_mem.img_y_fd = input_ptr->src_frame.img_fd.y;
	img_mem.img_u_fd = input_ptr->src_frame.img_fd.u;
	img_mem.width = input_ptr->src_frame.img_size.w;
	img_mem.height = input_ptr->src_frame.img_size.h;
	ISP_LOGE("sns_raw fd 0x%x yaddr 0x%lx yaddr_vir 0x%lx", img_mem.img_y_fd, img_mem.yaddr, img_mem.yaddr_vir);

	isp_dev_set_fetch_src_buf(cxt->isp_driver_handle, &img_mem);

#ifdef FPGA_TEST
	resolution_ptr = &input_ptr->resolution_info;
	memset(&scenario_in, 0, sizeof(scenario_in));
	scenario_in.tSensorInfo.ucSensorMode = 0;
	scenario_in.tSensorInfo.ucSensorMouduleType = 0;//0-sensor1  1-sensor2
	scenario_in.tSensorInfo.uwWidth = cxt->input_param.init_param.size.w;
	scenario_in.tSensorInfo.uwHeight = cxt->input_param.init_param.size.h;
	scenario_in.tSensorInfo.udLineTime = resolution_ptr->line_time*10;
	scenario_in.tSensorInfo.uwFrameRate = resolution_ptr->fps.max_fps*100;
	scenario_in.tSensorInfo.nColorOrder = cxt->input_param.init_param.image_pattern;
	scenario_in.tSensorInfo.uwClampLevel = 64;
	scenario_in.tSensorInfo.uwOriginalWidth = resolution_ptr->sensor_max_size.w;
	scenario_in.tSensorInfo.uwOriginalHeight = resolution_ptr->sensor_max_size.h;
	scenario_in.tSensorInfo.uwCropStartX = resolution_ptr->crop.st_x;
	scenario_in.tSensorInfo.uwCropStartY = resolution_ptr->crop.st_y;
	scenario_in.tSensorInfo.uwCropEndX = resolution_ptr->sensor_max_size.w + resolution_ptr->crop.st_x - 1;
	scenario_in.tSensorInfo.uwCropEndY = resolution_ptr->sensor_max_size.h + resolution_ptr->crop.st_y - 1;

	ISP_LOGE("uwOriginal w-h %dx%d", scenario_in.tSensorInfo.uwOriginalWidth, scenario_in.tSensorInfo.uwOriginalHeight);
	ISP_LOGE("image_pattern: %d", scenario_in.tSensorInfo.nColorOrder);

	scenario_in.tScenarioOutBypassFlag.bBypassLV = 0;
	scenario_in.tScenarioOutBypassFlag.bBypassVideo = 1;
	scenario_in.tScenarioOutBypassFlag.bBypassStill = 0;
	scenario_in.tScenarioOutBypassFlag.bBypassMetaData = 0;

	scenario_in.tBayerSCLOutInfo.uwBayerSCLOutWidth = 0;
	scenario_in.tBayerSCLOutInfo.uwBayerSCLOutHeight = 0;
	if (ISP_CAP_MODE_RAW_DATA == isp_raw_mem.capture_mode) {
		ISP_LOGE("bayer scaler wxh %dx%d\n", cxt->input_param.init_param.size.w, cxt->input_param.init_param.size.h);
		scenario_in.tBayerSCLOutInfo.uwBayerSCLOutWidth = 960;//cxt->input_param.init_param.size.w;
		scenario_in.tBayerSCLOutInfo.uwBayerSCLOutHeight = 720;//cxt->input_param.init_param.size.h;
	}
	ISP_LOGE("size %dx%d, line time %d frameRate %d", scenario_in.tSensorInfo.uwWidth, scenario_in.tSensorInfo.uwHeight,
		scenario_in.tSensorInfo.udLineTime, scenario_in.tSensorInfo.uwFrameRate);
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &scenario_in);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("isp_dev_cfg_scenario_info error %ld", ret);
		return -1;
	}

	/*set still image buffer format*/
	memset(&img_buf_param, 0, sizeof(img_buf_param));

	img_buf_param.format = ISP_OUT_IMG_NV12;
	img_buf_param.img_id = ISP_IMG_STILL_CAPTURE;
	img_buf_param.dram_eb = 0;
	img_buf_param.buf_num = 4;
	img_buf_param.width = cxt->input_param.init_param.size.w;
	img_buf_param.height = cxt->input_param.init_param.size.h;
	img_buf_param.line_offset = (cxt->input_param.init_param.size.w);
	img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
	img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
	ISP_LOGE("set still image buffer param img_id %d", img_buf_param.img_id);
	ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);


	/*set preview image buffer format*/
	memset(&img_buf_param, 0, sizeof(img_buf_param));
	ISP_LOGE("set isp_raw_data capture mode = %d", isp_raw_mem.capture_mode);
	if (ISP_CAP_MODE_RAW_DATA == isp_raw_mem.capture_mode) {
		img_buf_param.format = ISP_OUT_IMG_YUY2;
		img_buf_param.img_id = ISP_IMG_PREVIEW;
		img_buf_param.dram_eb = 0;
		img_buf_param.buf_num = 4;
		img_buf_param.width = 960;//cxt->input_param.init_param.size.w;
		img_buf_param.height = 720;//cxt->input_param.init_param.size.h;
		img_buf_param.line_offset = (2 * cxt->input_param.init_param.size.w);
		img_buf_param.addr[0].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[1].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[2].chn0 = 0x2FFFFFFF;
		img_buf_param.addr[3].chn0 = 0x2FFFFFFF;
		ISP_LOGE("set preview image buffer param img_id %d", img_buf_param.img_id);
		ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_buf_param);
	}

#endif

#ifndef FPGA_TEST
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &scenario_in);
	if (ret) {
		ISP_LOGE("failed to cfg scenatio info %ld", ret);
		goto exit;
	}
#endif
#ifdef FPGA_TEST
	cfg_info.tSubSampleInfo.TokenID = 0x100;
	cfg_info.tSubSampleInfo.udBufferImageSize = 320*240;
	cfg_info.tSubSampleInfo.uwOffsetRatioX =0;
	cfg_info.tSubSampleInfo.uwOffsetRatioY =0;
	cfg_info.tYHisInfo.TokenID = 0x120;
	cfg_info.tYHisInfo.tYHisRegion.uwBorderRatioX = 100;
	cfg_info.tYHisInfo.tYHisRegion.uwBorderRatioY = 100;
	cfg_info.tYHisInfo.tYHisRegion.uwBlkNumX = 16;
	cfg_info.tYHisInfo.tYHisRegion.uwBlkNumY = 16;
	cfg_info.tYHisInfo.tYHisRegion.uwOffsetRatioX = 0;
	cfg_info.tYHisInfo.tYHisRegion.uwOffsetRatioY = 0;
#else
	memcpy(&cfg_info.hw_cfg.tAntiFlickerInfo, input_ptr.hw_cfg.afl_cfg, sizeof(AntiFlicker_CfgInfo));
	memcpy(&cfg_info.hw_cfg.tYHisInfo, input_ptr.hw_cfg.yhis_cfg, sizeof(YHis_CfgInfo);
	memcpy(&cfg_info.hw_cfg.tSubSampleInfo, input_ptr.hw_cfg.subsample_cfg, sizeof(SubSample_CfgInfo));
#endif
	isp_dev_access_convert_ae_param(&input_ptr->hw_cfg.ae_cfg, &cfg_info.tAEInfo);
	isp_dev_access_convert_afl_param(&input_ptr->hw_cfg.afl_cfg, &cfg_info.tAntiFlickerInfo);
	isp_dev_access_convert_awb_param(&input_ptr->hw_cfg.awb_cfg, &cfg_info.tAWBInfo);
	isp_dev_access_set_af_hw_cfg(&cfg_info.tAFInfo, &input_ptr->hw_cfg.af_cfg);
	ret = isp_dev_cfg_3a_param(cxt->isp_driver_handle, &cfg_info);
	if (ret) {
		ISP_LOGE("failed to cfg 3a %ld", ret);
		goto exit;
	}
//	img_param.img_id = ;//0-preview, 1-video, 2-still capture 3-statistics
#ifdef FPGA_TEST
	dldseq.ucPreview_Baisc_DldSeqLength = 4;
	dldseq.aucPreview_Baisc_DldSeq[0] = 1;
	dldseq.aucPreview_Baisc_DldSeq[1] = 1;
	dldseq.aucPreview_Baisc_DldSeq[2] = 1;
	dldseq.aucPreview_Baisc_DldSeq[3] = 1;
	dldseq.ucPreview_Adv_DldSeqLength = 3;
	dldseq.aucPreview_Adv_DldSeq[0] = 1;
	dldseq.aucPreview_Adv_DldSeq[1] = 1;
	dldseq.aucPreview_Adv_DldSeq[2] = 1;
	dldseq.ucFastConverge_Baisc_DldSeqLength = 2;
	dldseq.aucFastConverge_Baisc_DldSeq[0] = 3;
	dldseq.aucFastConverge_Baisc_DldSeq[1] = 3;
#else
	memcpy(&dldseq, &input_ptr->dldseq, sizeof(DldSequence));
#endif
	ret = isp_dev_cfg_dld_seq(cxt->isp_driver_handle, &dldseq);
	if (ret) {
		ISP_LOGE("failed to cfg_dld_seq");
		goto  exit;
	}

	iso_gain = 100;
	ret = isp_dev_cfg_iso_speed(cxt->isp_driver_handle, &iso_gain);//TBD ???
	if (ret) {
		ISP_LOGE("failed to cfg iso speed %ld", ret);
		goto exit;
	}
	//ret = isp_dev_cfg_iso_speed(cxt->isp_driver_handle,
	awb_gain.r = input_ptr->awb_gain.r;
	awb_gain.g = input_ptr->awb_gain.g;
	awb_gain.b = input_ptr->awb_gain.b;
	ret = isp_dev_cfg_awb_gain(cxt->isp_driver_handle, &awb_gain);
	if (ret) {
		ISP_LOGE("failed to cfg awb gain");
		goto exit;
	}
	awb_gain.r = input_ptr->awb_gain_balanced.r;
	awb_gain.g = input_ptr->awb_gain_balanced.g;
	awb_gain.b = input_ptr->awb_gain_balanced.b;
	ret = isp_dev_cfg_awb_gain_balanced(cxt->isp_driver_handle, &awb_gain);
	if (ret) {
		ISP_LOGE("failed to cfg awb gain balanced");
		goto exit;
	}
#if 0
	if (0 == cxt->camera_id)
		dcam_id = 0;
	else if (2 == cxt->camera_id || 1 == cxt->camera_id)
		dcam_id = 1;
	ret = isp_dev_set_dcam_id(cxt->isp_driver_handle, dcam_id);
#endif

	ret = isp_dev_stream_on(cxt->isp_driver_handle);

	usleep(100*1000);

	ret = isp_dev_stream_off(cxt->isp_driver_handle);
	ret = isp_dev_stop(cxt->isp_driver_handle);

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

void isp_dev_access_evt_reg(cmr_handle isp_dev_handle, isp_evt_cb isp_event_cb, void *privdata)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ISP_LOGI("reg evt callback");
	isp_dev_evt_reg(cxt->isp_driver_handle, isp_event_cb, privdata);
}

cmr_int isp_dev_access_cfg_awb_param(cmr_handle isp_dev_handle, struct isp3a_awb_hw_cfg *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	AWB_CfgInfo                            awb_param;
	cmr_u32                                i = 0;

	//memcpy(&awb_param, data, sizeof(AWB_CfgInfo));
	awb_param.TokenID = data->token_id;
	awb_param.tAWBRegion.uwBlkNumX = data->region.blk_num_X;
	awb_param.tAWBRegion.uwBlkNumY = data->region.blk_num_Y;
	awb_param.tAWBRegion.uwBorderRatioX = data->region.border_ratio_X;
	awb_param.tAWBRegion.uwBorderRatioY = data->region.border_ratio_Y;
	awb_param.tAWBRegion.uwOffsetRatioX = data->region.offset_ratio_X;
	awb_param.tAWBRegion.uwOffsetRatioY = data->region.offset_ratio_Y;
	for (i=0 ; i<AWB_UCYFACTOR_NUM ; i++) {
		awb_param.ucYFactor[i] = data->uc_factor[i];
	}
	for (i=0 ; i<AWB_BBRFACTOR_NUM ; i++) {
		awb_param.BBrFactor[i] = data->bbr_factor[i];
	}
	awb_param.uwRGain = data->uw_rgain;
	awb_param.uwGGain = data->uw_ggain;
	awb_param.uwBGain = data->uw_bgain;
	awb_param.ucCrShift = data->uccr_shift;
	awb_param.ucOffsetShift = data->uc_offset_shift;
	awb_param.ucQuantize = data->uc_quantize;
	awb_param.ucDamp = data->uc_damp;
	awb_param.ucSumShift = data->uc_sum_shift;
	awb_param.tHis.bEnable = data->t_his.benable;
	awb_param.tHis.cCrEnd = data->t_his.ccrend;
	awb_param.tHis.cCrPurple = data->t_his.ccrpurple;
	awb_param.tHis.cCrStart = data->t_his.ccrstart;
	awb_param.tHis.cGrassEnd = data->t_his.cgrass_end;
	awb_param.tHis.cGrassOffset = data->t_his.cgrass_offset;
	awb_param.tHis.cGrassStart = data->t_his.cgrass_start;
	awb_param.tHis.cOffsetDown = data->t_his.coffsetdown;
	awb_param.tHis.cOffsetUp = data->t_his.cooffsetup;
	awb_param.tHis.cOffset_bbr_w_end = data->t_his.coffset_bbr_w_end;
	awb_param.tHis.cOffset_bbr_w_start = data->t_his.coffset_bbr_w_start;
	awb_param.tHis.dHisInterp = data->t_his.dhisinterp;
	awb_param.tHis.ucDampGrass = data->t_his.ucdampgrass;
	awb_param.tHis.ucOffsetPurPle = data->t_his.ucoffsetpurple;
	awb_param.tHis.ucYFac_w = data->t_his.ucyfac_w;
	awb_param.uwRLinearGain = data->uwrlinear_gain;
	awb_param.uwBLinearGain = data->uwblinear_gain;
	ISP_LOGV("token_id = %d, uccr_shift = %d, uc_damp = %d, uc_offset_shift = %d\n",
		awb_param.TokenID, awb_param.ucCrShift, awb_param.ucDamp, awb_param.ucOffsetShift);
	ISP_LOGV("uc_quantize = %d\n, uc_sum_shift = %d\n, uwblinear_gain = %d\n, uwrlinear_gain = %d\n,\
		uw_bgain = %d\n, uw_rgain = %d\n, uw_ggain = %d\n",
		awb_param.ucQuantize, awb_param.ucSumShift, awb_param.uwBLinearGain, awb_param.uwRLinearGain,
		awb_param.uwBGain, awb_param.uwRGain, awb_param.uwGGain);
	for (i=0 ; i<16 ; i++) {
		ISP_LOGV("uc_factor[%d] = %d\n", i, awb_param.ucYFactor[i]);
	}
	for (i=0 ; i<33 ; i++) {
		ISP_LOGV("bbr_factor[%d] = %d\n",i, awb_param.BBrFactor[i]);
	}
	ISP_LOGV("region:blk_num_X = %d, blk_num_Y = %d\n, border_ratio_X = %d, border_ratio_Y = %d\n,\
		offset_ratio_X = %d, offset_ratio_Y = %d\n",
		awb_param.tAWBRegion.uwBlkNumX, awb_param.tAWBRegion.uwBlkNumY,
		awb_param.tAWBRegion.uwBorderRatioX, awb_param.tAWBRegion.uwBorderRatioY,
		awb_param.tAWBRegion.uwOffsetRatioX, awb_param.tAWBRegion.uwOffsetRatioY);
	ISP_LOGV("t_his:benable = %d\n ccrend = %d\n ccrpurple = %d\n ccrstart = %d\n \
		cgrass_end = %d\n cgrass_offset = %d\n cgrass_start = %d\n coffsetdown = %d\n \
		coffset_bbr_w_end = %d\n coffset_bbr_w_start = %d\n cooffsetup = %d\n \
		dhisinterp = %d\n ucdampgrass = %d\n ucoffsetpurple = %d\n ucyfac_w = %d\n",
		awb_param.tHis.bEnable, awb_param.tHis.cCrEnd, awb_param.tHis.cCrPurple,
		awb_param.tHis.cCrStart, awb_param.tHis.cGrassEnd, awb_param.tHis.cGrassOffset,
		awb_param.tHis.cGrassStart, awb_param.tHis.cOffsetDown, awb_param.tHis.cOffset_bbr_w_end,
		awb_param.tHis.cOffset_bbr_w_start, awb_param.tHis.cOffsetUp, awb_param.tHis.dHisInterp,
		awb_param.tHis.ucDampGrass, awb_param.tHis.ucOffsetPurPle, awb_param.tHis.ucYFac_w);

	ret = isp_dev_cfg_awb_param(cxt->isp_driver_handle, &awb_param);//TBD

	return ret;
}

cmr_int isp_dev_access_cfg_awb_gain(cmr_handle isp_dev_handle, struct isp_awb_gain *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_awb_gain_info               gain;

	gain.r = data->r;
	gain.g = data->g;
	gain.b = data->b;
	ret = isp_dev_cfg_awb_gain(cxt->isp_driver_handle, &gain);

	return ret;
}

cmr_int isp_dev_access_cfg_awb_gain_balanced(cmr_handle isp_dev_handle, struct isp_awb_gain *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	struct isp_awb_gain_info               gain;

	gain.r = data->r;
	gain.g = data->g;
	gain.b = data->b;
	ISP_LOGV("balanced gain %d %d %d", gain.r, gain.g, gain.b);
	ret = isp_dev_cfg_awb_gain_balanced(cxt->isp_driver_handle, &gain);

	return ret;
}

cmr_int isp_dev_access_set_stats_buf(cmr_handle isp_dev_handle, struct isp_statis_buf *buf)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	ISP_LOGI("s");
	ret = isp_dev_set_statis_buf(cxt->isp_driver_handle, buf);

	return ret;
}

cmr_int isp_dev_access_cfg_af_param(cmr_handle isp_dev_handle, struct isp3a_af_hw_cfg *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;
	AF_CfgInfo                             af_param;

	memcpy(&af_param, data, sizeof(AF_CfgInfo));
	ret = isp_dev_cfg_af_param(cxt->isp_driver_handle, &af_param);//TBD

	return ret;
}

cmr_int isp_dev_access_cfg_iso_speed(cmr_handle isp_dev_handle, cmr_u32 *data)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_cfg_iso_speed(cxt->isp_driver_handle, data);
	return ret;
}

cmr_int isp_dev_access_get_exif_debug_info(cmr_handle isp_dev_handle, struct debug_info1 *exif_info)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_get_iq_param(cxt->isp_driver_handle, exif_info, NULL);

	ISP_LOGI("done %ld" ,ret);
	return ret;
}

cmr_int isp_dev_access_get_debug_info(cmr_handle isp_dev_handle, struct debug_info2 *debug_info)
{
	cmr_int                                ret = ISP_SUCCESS;
	struct isp_dev_access_context          *cxt = (struct isp_dev_access_context *)isp_dev_handle;

	ret = isp_dev_get_iq_param(cxt->isp_driver_handle, NULL, debug_info);

	ISP_LOGI("done %ld" ,ret);
	return ret;
}
