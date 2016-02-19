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
	struct isp_dev_access_context          *cxt = NULL;
	struct isp_video_limit            *limit = (struct isp_video_limit*)param_ptr;

	ISP_LOGI("cmd 0x%x", cmd);
	switch (cmd) {
	case ISP_VIDEO_SIZE:
		limit->width  = 1920; //TBD
		limit->height = 1080; //TBD
		break;
	case ISP_CAPTURE_SIZE:
		limit->width  = 1920; //TBD
		limit->height = 1080; //TBD
		break;
       case ISP_REG_VAL:

		break;
	default:
		break;
	}
	ISP_LOGI("width height %d %d,", limit->width, limit->height);

	return ret;
}

cmr_int isp_dev_access_ioctl(cmr_handle isp_dev_handle, enum isp_dev_access_ctrl_cmd cmd, union isp_dev_ctrl_cmd_in *input_ptr, union isp_dev_ctrl_cmd_out *output_ptr)
{
	cmr_int                                ret = ISP_SUCCESS;
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
		break;
	case ISP_DEV_ACCESS_SET_SATURATION:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		ret = isp_dev_cfg_saturation(cxt->isp_driver_handle, input_ptr->value);
		break;
	case ISP_DEV_ACCESS_SET_CONSTRACT:
		if (!input_ptr) {
			ISP_LOGI("failed to set contrast,input is null");
			break;
		}
		ret = isp_dev_cfg_contrast(cxt->isp_driver_handle, input_ptr->value);
		break;
	case ISP_DEV_ACCESS_SET_SHARPNESS:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		ret = isp_dev_cfg_sharpness(cxt->isp_driver_handle, input_ptr->value);
		break;
	case ISP_DEV_ACCESS_SET_SPECIAL_EFFECT:
		if (!input_ptr) {
			ISP_LOGI("failed to set sharpness,input is null");
			break;
		}
		ret = isp_dev_cfg_special_effect(cxt->isp_driver_handle, input_ptr->value);
		break;
	case ISP_DEV_ACCESS_SET_SUB_SAMPLE:
		break;
	case ISP_DEV_ACCESS_SET_STATIS_BUF:
		break;
	case ISP_DEV_ACCESS_GET_STATIS_BUF:
		break;
	case ISP_DEV_ACCESS_GET_TIME:
		if (!input_ptr) {
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
	cmr_u8 a_tAWBInfo_ucYFactor[16] = {0,0,0,4,7,10,12,14,15,15,15,15,14,13,10,5};
	cmr_s8 a_tAWBInfo_BBrFactor[33] = {22,20,18,16,15,13,11,10,8,8,6,5,3,1,-1,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16,-18,-18,-18,-18};
	// Scenario info
	SCENARIO_INFO_AP tSecnarioInfo;

#endif
	ISP_CHECK_HANDLE_VALID(isp_dev_handle);
	ret = isp_dev_start(cxt->isp_driver_handle);
	if (ret) {
		ISP_LOGE("failed to dev_start %ld", ret);
		goto exit;
	}
#ifndef FPGA_TEST
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &input_data);//TBD
	if (ret) {
		ISP_LOGE("failed to cfg scenatio info %ld", ret);
		goto exit;
	}
#endif

#ifdef FPGA_TEST
	memset(&tSecnarioInfo, 0, sizeof(tSecnarioInfo));
	tSecnarioInfo.tSensorInfo.ucSensorMode = 0;
	tSecnarioInfo.tSensorInfo.ucSensorMouduleType = 0;//0-sensor1  1-sensor2
	tSecnarioInfo.tSensorInfo.uwWidth = cxt->input_param.init_param.size.w;
	tSecnarioInfo.tSensorInfo.uwHeight = cxt->input_param.init_param.size.h;
	tSecnarioInfo.tSensorInfo.udLineTime = 1315;//param_ptr->common_in.resolution_info.line_time * 10;
	tSecnarioInfo.tSensorInfo.uwFrameRate = 3000;//param_ptr->common_in.resolution_info.fps.max_fps * 100;
	tSecnarioInfo.tSensorInfo.nColorOrder = COLOR_ORDER_GR;
	tSecnarioInfo.tSensorInfo.uwClampLevel = 64;

	tSecnarioInfo.tScenarioOutBypassFlag.bBypassLV = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassVideo = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassStill = 0;
	tSecnarioInfo.tScenarioOutBypassFlag.bBypassMetaData = 0;

	tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutWidth = 0;
	tSecnarioInfo.tBayerSCLOutInfo.uwBayerSCLOutHeight = 0;
	ISP_LOGE("size %dx%d, line time %d frameRate %d", tSecnarioInfo.tSensorInfo.uwWidth, tSecnarioInfo.tSensorInfo.uwHeight,
		tSecnarioInfo.tSensorInfo.udLineTime, tSecnarioInfo.tSensorInfo.uwFrameRate);
	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &tSecnarioInfo);
	if (ISP_SUCCESS != ret) {
		ISP_LOGE("isp_dev_cfg_scenario_info error %ld", ret);
		return -1;
	}

#ifdef FPGA_TEST
	/*set still image buffer format*/
	memset(&img_buf_param, 0, sizeof(img_buf_param));
	img_size = 2 * 1920* 1080 + 0x400;


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
#endif

#ifdef FPGA_TEST
	//Example 3A cfg
	// AF
	cfg3a_info.tAFInfo.TokenID = 0x100;
	cfg3a_info.tAFInfo.tAFRegion.uwSizeRatioX = 33;
	cfg3a_info.tAFInfo.tAFRegion.uwSizeRatioY = 33;
	cfg3a_info.tAFInfo.tAFRegion.uwOffsetRatioX = 33;
	cfg3a_info.tAFInfo.tAFRegion.uwOffsetRatioY = 33;
	cfg3a_info.tAFInfo.tAFRegion.uwBlkNumX = 3;
	cfg3a_info.tAFInfo.tAFRegion.uwBlkNumY = 3;
	//cfg3a_info.tAFInfo.auwLUT[0] = NULL;
	//cfg3a_info.tAFInfo.auwAFLUT[0] = NULL;
	memcpy((void *)&cfg3a_info.tAFInfo.aucWeight, (void *)&a_tAFInfo_aucWeight, 6);
	cfg3a_info.tAFInfo.uwSH = 0;
	cfg3a_info.tAFInfo.ucThMode = 0;
	memcpy((void *)&cfg3a_info.tAFInfo.aucIndex,(void *)&a_tAFInfo_aucIndex,82);

	memcpy((void *)&cfg3a_info.tAFInfo.auwTH, (void *)&a_tAFInfo_auwTH,4*2);
	memcpy((void *)&cfg3a_info.tAFInfo.pwTV,(void *)&a_tAFInfo_pwTV,4*2);
	cfg3a_info.tAFInfo.udAFoffset = 0;
	cfg3a_info.tAFInfo.bAF_PY_Enable = 0;
	cfg3a_info.tAFInfo.bAF_LPF_Enable = 0;
	cfg3a_info.tAFInfo.nFilterMode = 2;
	cfg3a_info.tAFInfo.ucFilterID = 0;
	cfg3a_info.tAFInfo.uwLineCnt = 0;
#if 0
	// AWB
	cfg3a_info.tAWBInfo.TokenID = 0x100;
	cfg3a_info.tAWBInfo.tAWBRegion.uwBorderRatioX = 100;
	cfg3a_info.tAWBInfo.tAWBRegion.uwBorderRatioY = 100;
	cfg3a_info.tAWBInfo.tAWBRegion.uwBlkNumX = 64;
	cfg3a_info.tAWBInfo.tAWBRegion.uwBlkNumY = 48;
	cfg3a_info.tAWBInfo.tAWBRegion.uwOffsetRatioX = 0;
	cfg3a_info.tAWBInfo.tAWBRegion.uwOffsetRatioY = 0;
	memcpy((void *)cfg3a_info.tAWBInfo.ucYFactor,(void *)a_tAWBInfo_ucYFactor,16);
	memcpy((void *)cfg3a_info.tAWBInfo.BBrFactor,(void *)a_tAWBInfo_BBrFactor,33);
	cfg3a_info.tAWBInfo.uwRGain = 1103;
	cfg3a_info.tAWBInfo.uwGGain = 0;
	cfg3a_info.tAWBInfo.uwBGain = 402;
	cfg3a_info.tAWBInfo.ucCrShift = 100;
	cfg3a_info.tAWBInfo.ucOffsetShift = 100;
	cfg3a_info.tAWBInfo.ucQuantize = 0;
	cfg3a_info.tAWBInfo.ucDamp = 7;
	cfg3a_info.tAWBInfo.ucSumShift = 5;
	cfg3a_info.tAWBInfo.tHis.bEnable = 1;
	cfg3a_info.tAWBInfo.tHis.cCrStart = -46;
	cfg3a_info.tAWBInfo.tHis.cCrEnd = 110;
	cfg3a_info.tAWBInfo.tHis.cOffsetUp = 10;
	cfg3a_info.tAWBInfo.tHis.cOffsetDown = -90;
	cfg3a_info.tAWBInfo.tHis.cCrPurple = 0;
	cfg3a_info.tAWBInfo.tHis.ucOffsetPurPle = 2;
	cfg3a_info.tAWBInfo.tHis.cGrassOffset = -22;
	cfg3a_info.tAWBInfo.tHis.cGrassStart = -30;
	cfg3a_info.tAWBInfo.tHis.cGrassEnd = 25;
	cfg3a_info.tAWBInfo.tHis.ucDampGrass = 4;
	cfg3a_info.tAWBInfo.tHis.cOffset_bbr_w_start = -2;
	cfg3a_info.tAWBInfo.tHis.cOffset_bbr_w_end = 2;
	cfg3a_info.tAWBInfo.tHis.ucYFac_w = 2;
	cfg3a_info.tAWBInfo.tHis.dHisInterp = -178;
	cfg3a_info.tAWBInfo.uwRLinearGain = 270;
	cfg3a_info.tAWBInfo.uwBLinearGain = 168;
#endif
	// AE
	cfg3a_info.tAEInfo.TokenID = 0x111;
	cfg3a_info.tAEInfo.tAERegion.uwBorderRatioX = 100;
	cfg3a_info.tAEInfo.tAERegion.uwBorderRatioY = 100;
	cfg3a_info.tAEInfo.tAERegion.uwBlkNumX = 16;
	cfg3a_info.tAEInfo.tAERegion.uwBlkNumY = 16;
	cfg3a_info.tAEInfo.tAERegion.uwOffsetRatioX = 0;
	cfg3a_info.tAEInfo.tAERegion.uwOffsetRatioY = 0;

	// SubSample
	cfg3a_info.tSubSampleInfo.TokenID = 0x100;
	cfg3a_info.tSubSampleInfo.udBufferImageSize = 320*240;
	cfg3a_info.tSubSampleInfo.uwOffsetRatioX =0;
	cfg3a_info.tSubSampleInfo.uwOffsetRatioY =0;

	//Antiflicker
	cfg3a_info.tAntiFlickerInfo.TokenID = 0x130;
	cfg3a_info.tAntiFlickerInfo.uwOffsetRatioX =0;
	cfg3a_info.tAntiFlickerInfo.uwOffsetRatioY =0;


	// Yhis configuration:
	cfg3a_info.tYHisInfo.TokenID = 0x120;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBorderRatioX = 100;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBorderRatioY = 100;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBlkNumX = 16;
	cfg3a_info.tYHisInfo.tYHisRegion.uwBlkNumY = 16;
	cfg3a_info.tYHisInfo.tYHisRegion.uwOffsetRatioX = 0;
	cfg3a_info.tYHisInfo.tYHisRegion.uwOffsetRatioY = 0;
#else
	memcpy(&cfg3a_info.hw_cfg.tAEInfo, &param_ptr->hw_cfg.ae_cfg, sizeof(AE_CfgInfo));
	memcpy(&cfg3a_info.hw_cfg.tAFInfo, &param_ptr->hw_cfg.af_cfg, sizeof(AF_CfgInfo));
	memcpy(&cfg3a_info.hw_cfg.tAntiFlickerInfo, &param_ptr->hw_cfg.afl_cfg, sizeof(AntiFlicker_CfgInfo));
	memcpy(&cfg3a_info.hw_cfg.tYHisInfo, &param_ptr->hw_cfg.yhis_cfg, sizeof(YHis_CfgInfo);
	memcpy(&cfg3a_info.hw_cfg.tSubSampleInfo, &param_ptr->hw_cfg.subsample_cfg, sizeof(SubSample_CfgInfo));
#endif
#endif
	memcpy(&cfg3a_info.tAWBInfo, &param_ptr->hw_cfg.awb_cfg, sizeof(AWB_CfgInfo));

	ISP_LOGV("token_id = %d, uccr_shift = %d, uc_damp = %d, uc_offset_shift = %d\n",
		param_ptr->hw_cfg.awb_cfg.token_id, param_ptr->hw_cfg.awb_cfg.uccr_shift,
		param_ptr->hw_cfg.awb_cfg.uc_damp, param_ptr->hw_cfg.awb_cfg.uc_offset_shift);
	ISP_LOGV("uc_quantize = %d\n, uc_sum_shift = %d\n, uwblinear_gain = %d\n, uwrlinear_gain = %d\n,\
		uw_bgain = %d\n, uw_gain = %d\n, uw_ggain = %d\n",
		param_ptr->hw_cfg.awb_cfg.uc_quantize, param_ptr->hw_cfg.awb_cfg.uc_sum_shift,
		param_ptr->hw_cfg.awb_cfg.uwblinear_gain, param_ptr->hw_cfg.awb_cfg.uwrlinear_gain,
		param_ptr->hw_cfg.awb_cfg.uw_bgain, param_ptr->hw_cfg.awb_cfg.uw_gain,
		param_ptr->hw_cfg.awb_cfg.uw_ggain);
	for (i=0 ; i<16 ; i++) {
		ISP_LOGV("uc_factor[%ld] = %d\n", i, param_ptr->hw_cfg.awb_cfg.uc_factor[i]);
	}
	for (i=0 ; i<33 ; i++) {
		ISP_LOGV("bbr_factor[%ld] = %d\n",i, param_ptr->hw_cfg.awb_cfg.bbr_factor[i]);
	}
	ISP_LOGV("region:blk_num_X = %d, blk_num_Y = %d\n, border_ratio_X = %d, border_ratio_Y = %d\n,\
		offset_ratio_X = %d, offset_ratio_Y = %d\n",
		param_ptr->hw_cfg.awb_cfg.region.blk_num_X, param_ptr->hw_cfg.awb_cfg.region.blk_num_Y,
		param_ptr->hw_cfg.awb_cfg.region.border_ratio_X, param_ptr->hw_cfg.awb_cfg.region.border_ratio_Y,
		param_ptr->hw_cfg.awb_cfg.region.offset_ratio_X, param_ptr->hw_cfg.awb_cfg.region.offset_ratio_Y);
	ISP_LOGV("t_his:benable = %d\n ccrend = %d\n ccrpurple = %d\n ccrstart = %d\n \
		cgrass_end = %d\n cgrass_offset = %d\n cgrass_start = %d\n coffsetdown = %d\n \
		coffset_bbr_w_end = %d\n coffset_bbr_w_start = %d\n cooffsetup = %d\n \
		dhisinterp = %d\n ucdampgrass = %d\n ucoffsetpurple = %d\n ucyfac_w = %d\n",
		param_ptr->hw_cfg.awb_cfg.t_his.benable,
		param_ptr->hw_cfg.awb_cfg.t_his.ccrend,
		param_ptr->hw_cfg.awb_cfg.t_his.ccrpurple,
		param_ptr->hw_cfg.awb_cfg.t_his.ccrstart,
		param_ptr->hw_cfg.awb_cfg.t_his.cgrass_end,
		param_ptr->hw_cfg.awb_cfg.t_his.cgrass_offset,
		param_ptr->hw_cfg.awb_cfg.t_his.cgrass_start,
		param_ptr->hw_cfg.awb_cfg.t_his.coffsetdown,
		param_ptr->hw_cfg.awb_cfg.t_his.coffset_bbr_w_end,
		param_ptr->hw_cfg.awb_cfg.t_his.coffset_bbr_w_start,
		param_ptr->hw_cfg.awb_cfg.t_his.cooffsetup,
		param_ptr->hw_cfg.awb_cfg.t_his.dhisinterp,
		param_ptr->hw_cfg.awb_cfg.t_his.ucdampgrass,
		param_ptr->hw_cfg.awb_cfg.t_his.ucoffsetpurple,
		param_ptr->hw_cfg.awb_cfg.t_his.ucyfac_w);

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
#ifdef FPGA_TEST
	awb_gain_info.r = 457;
	awb_gain_info.g = 256;
	awb_gain_info.b = 368;
#else
	awb_gain_info = param_ptr->hw_cfg.awb_gain;
#endif
	ret = isp_dev_cfg_awb_gain(cxt->isp_driver_handle, &awb_gain_info);

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

	ret = isp_dev_cfg_scenario_info(cxt->isp_driver_handle, &scenario_in);
	if (ret) {
		ISP_LOGE("failed to cfg scenatio info %ld", ret);
		goto exit;
	}
	memcpy(&cfg_info, &input_ptr->hw_cfg, sizeof(Cfg3A_Info));
	ret = isp_dev_cfg_3a_param(cxt->isp_driver_handle, &cfg_info);
	if (ret) {
		ISP_LOGE("failed to cfg 3a %ld", ret);
		goto exit;
	}
//	img_param.img_id = ;//0-preview, 1-video, 2-still capture 3-statistics
//	img_param.dram_eb = ;
//	img_param.format;
//	img_param.width;
//	img_param.height;
//	img_param.buf_num;
//	img_param.addr[IMG_BUF_NUM_MAX];
//	img_param.addr_vir[IMG_BUF_NUM_MAX];
//	img_param.line_offset;
	ret = isp_dev_set_img_param(cxt->isp_driver_handle, &img_param);
	if (ret) {
		ISP_LOGE("failed to set img param");
		goto  exit;
	}
	memcpy(&dldseq, &input_ptr->dldseq, sizeof(DldSequence));
	ret = isp_dev_cfg_dld_seq(cxt->isp_driver_handle, &dldseq);
	if (ret) {
		ISP_LOGE("failed to cfg_dld_seq");
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

	ret = isp_dev_stream_on(cxt->isp_driver_handle);
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

	memcpy(&awb_param, data, sizeof(AWB_CfgInfo));
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
