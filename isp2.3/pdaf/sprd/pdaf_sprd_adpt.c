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
#define LOG_TAG "pdaf_adpt"

#include <assert.h>
#include <stdlib.h>
#include <cutils/properties.h>

#include "pdaf_sprd_adpt.h"
#include "pdaf_ctrl.h"
#include "isp_adpt.h"
#include "dlfcn.h"
#include "pd_algo.h"
#include "cmr_common.h"

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
	 cmr_int(*pd_set_buffer) (struct pd_frame_in * cb_param);
	void *pd_left;
	void *pd_right;
	alPD_RECT roi;
	PDInReg pd_reg_in;
	cmr_u8 pd_reg_out[PD_REG_OUT_SIZE];
	cmr_u8 pdotp_pack_data[PD_OTP_PACK_SIZE];
	void *caller;
	PD_GlobalSetting pd_gobal_setting;
	struct pdaf_ppi_info ppi_info;
	struct pdaf_roi_info roi_info;
	struct sprd_pdaf_report_t report_data;
	cmr_u32(*pdaf_set_pdinfo_to_af) (void *handle, struct pd_result * in_parm);
	cmr_u32(*pdaf_set_cfg_parm) (void *handle, struct isp_dev_pdaf_info * in_parm);
	cmr_u32(*pdaf_set_bypass) (void *handle, cmr_u32 in_parm);
	cmr_u32(*pdaf_set_work_mode) (void *handle, cmr_u32 in_parm);
	cmr_u32(*pdaf_set_skip_num) (void *handle, cmr_u32 in_parm);
	cmr_u32(*pdaf_set_ppi_info) (void *handle, struct pdaf_ppi_info * in_parm);
	cmr_u32(*pdaf_set_roi) (void *handle, struct pdaf_roi_info * in_parm);
	cmr_u32(*pdaf_set_extractor_bypass) (void *handle, cmr_u32 in_parm);
};

//testcode
#define ROI_X 1048
#define ROI_Y 792
#define ROI_Width 2048
#define ROI_Height 1536
#define PDAF_PATTERN_COUNT	 8

struct isp_dev_pdaf_info pdafTestCase[] = {
	//bypass,  corrector_bypass      phase_map_corr_en; block_size; grid_mode;win;block;gain_upperbound;phase_txt_smooth;phase_gfilter;phase_flat_smoother;
	//hot_pixel_th[3]dead_pixel_th[3];flat_th;edge_ratio_hv;edge_ratio_rd;edge_ratio_hv_rd;phase_left_addr;phase_right_addr;phase_pitch;
	//pattern_pixel_is_right[64];
	//pattern_pixel_row[64];
	//pattern_pixel_col[64];
	//gain_ori_left[2];gain_ori_right[2];extractor_bypass;mode_sel;skip_num; phase_data_dword_num;
	//pdaf_blc_r;pdaf_blc_b;pdaf_blc_gr;pdaf_blc_gb;phase_cfg_rdy;phase_skip_num_clr;
	{
	 0, 0, 1, {2, 2}, 1, {1048, 792, 1048 + 2048, 792 + 1536}, {24, 24, 4208 - 24, 3120 - 24},
	 {600, 800, 200, 400}, 1, 1, 0,
	 {0, 1, 2}, {255, 511, 765}, 765, 127, 0, 510,
	 0x86000000, 0x86800000,
	 0,
	 {0, 0, 1, 1, 1, 1, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {5, 5, 8, 8, 21, 21, 24, 24,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {2, 18, 1, 17, 10, 26, 9, 25,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,},
	 {2172, 16332}, {8087, 15728}, 0, 1, 0, 0xB,
	 {511, 1023, 127, 255}, {0, 0}, {0, 0}
	 },
};

struct isp_dev_pdaf_info *ispGetPdafTestCase(cmr_u32 index)
{
	return &pdafTestCase[index];
}

cmr_int _ispSetPdafParam(void *param, cmr_u32 index)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 i = 0;
	struct isp_dev_pdaf_info *pdaf_info_ptr = (struct isp_dev_pdaf_info *)param;
	struct isp_dev_pdaf_info *pdafTestCase_ptr = ispGetPdafTestCase(index);
	cmr_u16 phase_left[128] = { 0 }, phase_right[128] = {
	0};
	cmr_u32 data_left[128] = { 0 }, data_right[128] = {
	0};

#if 0
	//store_info_ptr->bypass          = ISP_ZERO;
	if (pdaf_info_ptr->bypass)
		return rtn;
#else
	pdaf_info_ptr->bypass = 0;
	pdafTestCase_ptr->phase_left_addr = (cmr_u32) & phase_left[0];
	pdafTestCase_ptr->phase_right_addr = (cmr_u32) & phase_right[0];
	pdafTestCase_ptr->data_ptr_left[0] = (cmr_u32) & data_left[0];
	pdafTestCase_ptr->data_ptr_left[1] = 0;
	pdafTestCase_ptr->data_ptr_right[0] = (cmr_u32) & data_right[0];
	pdafTestCase_ptr->data_ptr_right[1] = 0;
#endif

	pdaf_info_ptr->corrector_bypass = pdafTestCase_ptr->corrector_bypass;
	pdaf_info_ptr->phase_map_corr_en = pdafTestCase_ptr->phase_map_corr_en;
	pdaf_info_ptr->block_size = pdafTestCase_ptr->block_size;
	pdaf_info_ptr->grid_mode = pdafTestCase_ptr->grid_mode;
	pdaf_info_ptr->win = pdafTestCase_ptr->win;
	pdaf_info_ptr->block = pdafTestCase_ptr->block;
	pdaf_info_ptr->gain_upperbound = pdafTestCase_ptr->gain_upperbound;
	pdaf_info_ptr->phase_txt_smooth = pdafTestCase_ptr->phase_txt_smooth;
	pdaf_info_ptr->phase_gfilter = pdafTestCase_ptr->phase_gfilter;
	pdaf_info_ptr->phase_flat_smoother = pdafTestCase_ptr->phase_flat_smoother;
	pdaf_info_ptr->flat_th = pdafTestCase_ptr->flat_th;
	pdaf_info_ptr->edge_ratio_hv = pdafTestCase_ptr->edge_ratio_hv;
	pdaf_info_ptr->edge_ratio_rd = pdafTestCase_ptr->edge_ratio_rd;
	pdaf_info_ptr->edge_ratio_hv_rd = pdafTestCase_ptr->edge_ratio_hv_rd;
	pdaf_info_ptr->phase_left_addr = pdafTestCase_ptr->phase_left_addr;
	pdaf_info_ptr->phase_right_addr = pdafTestCase_ptr->phase_right_addr;
	pdaf_info_ptr->phase_pitch = pdafTestCase_ptr->phase_pitch;
	pdaf_info_ptr->gain_ori_left[0] = pdafTestCase_ptr->gain_ori_left[0];
	pdaf_info_ptr->gain_ori_left[1] = pdafTestCase_ptr->gain_ori_left[1];
	pdaf_info_ptr->gain_ori_right[0] = pdafTestCase_ptr->gain_ori_right[0];
	pdaf_info_ptr->gain_ori_right[1] = pdafTestCase_ptr->gain_ori_right[1];

	cmr_s32 block_x = (pdaf_info_ptr->win.end_x - pdaf_info_ptr->win.start_x) / (8 << pdaf_info_ptr->block_size.width);
	cmr_s32 block_y = (pdaf_info_ptr->win.end_y - pdaf_info_ptr->win.start_y) / (8 << pdaf_info_ptr->block_size.height);
	cmr_u32 phasepixel_total_num = block_x * block_y * PDAF_PATTERN_COUNT / 2;
	pdaf_info_ptr->phase_data_dword_num = (phasepixel_total_num + 5) / 6;
	pdaf_info_ptr->extractor_bypass = pdafTestCase_ptr->extractor_bypass;
	pdaf_info_ptr->mode_sel = pdafTestCase_ptr->mode_sel;
	pdaf_info_ptr->skip_num = pdafTestCase_ptr->skip_num;
	pdaf_info_ptr->pdaf_blc = pdafTestCase_ptr->pdaf_blc;

	for (i = 0; i < 3; i++) {
		pdaf_info_ptr->hot_pixel_th[i] = pdafTestCase_ptr->hot_pixel_th[i];
		pdaf_info_ptr->dead_pixel_th[i] = pdafTestCase_ptr->dead_pixel_th[i];
	}

	for (i = 0; i < 64; i++) {
		pdaf_info_ptr->pattern_pixel_is_right[i] = pdafTestCase_ptr->pattern_pixel_is_right[i];
		pdaf_info_ptr->pattern_pixel_row[i] = pdafTestCase_ptr->pattern_pixel_row[i];
		pdaf_info_ptr->pattern_pixel_col[i] = pdafTestCase_ptr->pattern_pixel_col[i];
	}

	memset((void *)(unsigned long)pdaf_info_ptr->phase_left_addr, 0, 0x100);
	memset((void *)(unsigned long)pdaf_info_ptr->phase_right_addr, 0, 0x100);
	return rtn;
}

cmr_int isp_get_pdaf_default_param(struct isp_dev_pdaf_info * pdaf_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	_ispSetPdafParam(pdaf_param, 0);

	return rtn;
}

static cmr_int pdaf_setup(cmr_handle pdaf)
{
	cmr_int rtn = ISP_SUCCESS;

	ISP_CHECK_HANDLE_VALID(pdaf);
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)pdaf;
	/* user for debug*/
	/*struct isp_dev_pdaf_info pdaf_param;

	memset(&pdaf_param, 0x00, sizeof(pdaf_param));
	rtn = isp_get_pdaf_default_param(&pdaf_param);
	cxt->pdaf_set_cfg_parm(cxt->caller, &pdaf_param);*/
	if (cxt->pdaf_set_bypass) {
		cxt->pdaf_set_bypass(cxt->caller, 0);
	}
	if (cxt->pdaf_set_work_mode) {
		cxt->pdaf_set_work_mode(cxt->caller, 1);
	}
	if (cxt->pdaf_set_skip_num) {
		cxt->pdaf_set_skip_num(cxt->caller, 0);
	}
	if (cxt->pdaf_set_ppi_info) {
		cxt->pdaf_set_ppi_info(cxt->caller, &(cxt->ppi_info));
	}
	if (cxt->pdaf_set_roi) {
		cxt->pdaf_set_roi(cxt->caller, &(cxt->roi_info));
	}
	if (cxt->pdaf_set_extractor_bypass) {
		cxt->pdaf_set_extractor_bypass(cxt->caller, 0);
	}
	return rtn;
}

cmr_handle sprd_pdaf_adpt_init(void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_ctrl_init_in *in_p = (struct pdaf_ctrl_init_in *)in;
	struct pdaf_ctrl_init_out *out_p = (struct pdaf_ctrl_init_out *)out;
	struct sprd_pdaf_context *cxt = NULL;
	struct sensor_otp_af_info *otp_af_info = NULL;
	struct isp_alg_fw_context *isp_ctx = NULL;
	cmr_u32 pd_in_size = 0;

	if (!in_p) {
		ISP_LOGE("fail to check init param %p!!!", in_p);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	isp_ctx = (struct isp_alg_fw_context *)in_p->caller_handle;
	cxt = (struct sprd_pdaf_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc pdaf");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	cmr_bzero(cxt, sizeof(*cxt));
	cxt->caller = in_p->caller;
	cxt->caller_handle = in_p->caller_handle;
	cxt->pdaf_set_pdinfo_to_af = in_p->pdaf_set_pdinfo_to_af;
	cxt->pdaf_set_cfg_parm = in_p->pdaf_set_cfg_param;
	cxt->pdaf_set_bypass = in_p->pdaf_set_bypass;
	cxt->pdaf_set_work_mode= in_p->pdaf_set_work_mode;
	cxt->pdaf_set_skip_num = in_p->pdaf_set_skip_num;
	cxt->pdaf_set_ppi_info = in_p->pdaf_set_ppi_info;
	cxt->pdaf_set_roi = in_p->pdaf_set_roi;
	cxt->pdaf_set_extractor_bypass = in_p->pdaf_set_extractor_bypass;

	cxt->ppi_info.block.start_x = in_p->pd_info->pd_offset_x;
	cxt->ppi_info.block.end_x = in_p->pd_info->pd_end_x;
	cxt->ppi_info.block.start_y = in_p->pd_info->pd_offset_y;
	cxt->ppi_info.block.end_y = in_p->pd_info->pd_end_y;
	cxt->ppi_info.block_size.height = in_p->pd_info->pd_block_h;
	cxt->ppi_info.block_size.width = in_p->pd_info->pd_block_w;
	for (cmr_u16 i=0; i< in_p->pd_info->pd_pos_size; i++) {
		cxt->ppi_info.pattern_pixel_is_right[i] = in_p->pd_info->pd_is_right[i];
		cxt->ppi_info.pattern_pixel_row[i] = in_p->pd_info->pd_pos_row[i];
		cxt->ppi_info.pattern_pixel_col[i] = in_p->pd_info->pd_pos_col[i];
	}

	cxt->roi_info.win.start_x = 1048;
	cxt->roi_info.win.start_y = 792;
	cxt->roi_info.win.end_x = 1048+2048 ;
	cxt->roi_info.win.end_y = 792+1536;
	cmr_s32 block_num_x = (cxt->roi_info.win.end_x - cxt->roi_info.win.start_x)/(8 << cxt->ppi_info.block_size.width);
	cmr_s32 block_num_y = (cxt->roi_info.win.end_y - cxt->roi_info.win.start_y)/(8 << cxt->ppi_info.block_size.height);
	cmr_u32 phasepixel_total_num = block_num_x*block_num_y * in_p->pd_info->pd_pos_size / 2;
	cxt->roi_info.phase_data_write_num = (phasepixel_total_num + 5) / 6;

	cxt->pd_left = malloc(pd_in_size);
	if (NULL == cxt->pd_left) {
		ISP_LOGE("fail to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}
	cxt->pd_right = malloc(pd_in_size);
	if (NULL == cxt->pd_right) {
		ISP_LOGE("fail to malloc");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	cmr_bzero(cxt->pd_left, pd_in_size);
	cmr_bzero(cxt->pd_right, pd_in_size);

	cxt->pd_gobal_setting.dImageW = in_p->sensor_max_size.w;
	cxt->pd_gobal_setting.dImageH = in_p->sensor_max_size.h;
	cxt->pd_gobal_setting.dBeginX = in_p->pd_info->pd_offset_x;
	cxt->pd_gobal_setting.dBeginY = in_p->pd_info->pd_offset_y;

	/*TBD dSensorID 0:for imx258 1: for OV13850 */
	/*cxt->pd_gobal_setting.dSensorMode = in_p->camera_id; */
	cxt->pd_gobal_setting.dSensorMode = SENSOR_ID;	/*TBD get from sensor id */
/*TBD init sharkle isp*/
	pdaf_setup(cxt);

	ret = PD_Init((void *)&cxt->pd_gobal_setting);

	if (ret) {
		ISP_LOGE("fail to init lib %ld", ret);
		goto exit;
	}

	cxt->is_busy = 0;

	return (cmr_handle) cxt;

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
	return (cmr_handle) cxt;
}

static cmr_s32 sprd_pdaf_adpt_deinit(cmr_handle adpt_handle, void *param, void *result)
{
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	cmr_s32 ret = ISP_SUCCESS;

	UNUSED(param);
	UNUSED(result);
	if (cxt) {
		/* deinit lib */
		ret = (cmr_s32) PD_Uninit();

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
	ISP_LOGI("done");

	return ret;
}

static cmr_s32 sprd_pdaf_adpt_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	struct pdaf_ctrl_process_in *proc_in = (struct pdaf_ctrl_process_in *)in;
	struct pdaf_ctrl_callback_in callback_in;
	alGE_RECT pdroi;
	cmr_u16 image_width;
	cmr_u16 image_height;
	char value[PROPERTY_VALUE_MAX];

	UNUSED(out);
	bzero(&pdroi, sizeof(pdroi));
	if (!in) {
		ISP_LOGE("fail to init param %p!!!", in);
		ret = ISP_PARAM_NULL;
		return ret;
	}

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	cmr_bzero(&callback_in, sizeof(callback_in));

#if 0
	if (!cxt->pd_enable || !cxt->pd_open) {
		ISP_LOGI("pd enable %d, open %d", cxt->pd_enable, cxt->pd_open);
		ret = ISP_SUCCESS;
		goto exit;
	}
#endif
	cxt->is_busy = 1;

	cxt->pd_reg_in.dcurrentVCM = proc_in->dcurrentVCM;
	cxt->pd_reg_in.dBv = (float)proc_in->dBv / 1000.0;
	pdroi.m_wLeft = 0;
	pdroi.m_wTop = 0;

	void *pInPhaseBuf_left = (cmr_s32 *)(unsigned long)(proc_in->u_addr);	//TODO
	void *pInPhaseBuf_right = (cmr_s32 *)(unsigned long)(proc_in->u_addr + 0x25800);	//TODO
	ISP_LOGV("pInPhaseBuf_left = %p", pInPhaseBuf_left);
	cmr_s32 dRectX = ROI_X;
	cmr_s32 dRectY = ROI_Y;
	cmr_s32 dRectW = ROI_Width;
	cmr_s32 dRectH = ROI_Height;
	struct pd_result pd_calc_result;

	for (cmr_s32 area_index = 0; area_index < AREA_LOOP; area_index++) {
		ret = PD_DoType2(pInPhaseBuf_left, pInPhaseBuf_right, dRectX, dRectY, dRectW, dRectH, area_index);
		if (ret) {
			ISP_LOGE("fail to do pd algo.");
			goto exit;
		}
	}

	cxt->report_data.token_id = cxt->token_id;
	cxt->report_data.frame_id = cxt->frame_id;
	cxt->report_data.enable = cxt->pd_enable;

	callback_in.proc_out.pd_report_data = cxt->report_data;
	/*call back to haf lib */

	/*TBD get clac result from so & send to af */
	ret = PD_GetResult(&pd_calc_result.pdConf[4], &pd_calc_result.pdPhaseDiff[4], &pd_calc_result.pdGetFrameID, 4);
	if (ret) {
		ISP_LOGE("fail to do get pd_result.");
		goto exit;
	}
	ISP_LOGV("PD_GetResult pd_calc_result.pdConf[4] = %d, pd_calc_result.pdPhaseDiff[4] = 0x%lf", pd_calc_result.pdConf[4], pd_calc_result.pdPhaseDiff[4]);
	cxt->pdaf_set_pdinfo_to_af(cxt->caller, &pd_calc_result);
	cxt->frame_id++;	//= proc_in->pd_raw.frame_id;
exit:
	cxt->is_busy = 0;
	return ret;
}

static cmr_s32 pdafsprd_adpt_set_param(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_int ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	UNUSED(in);


	ISP_CHECK_HANDLE_VALID(adpt_handle);
	pdaf_setup(cxt);
	return 0;
}


static cmr_s32 sprd_pdaf_adpt_ioctrl(cmr_handle adpt_handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct pdaf_ctrl_param_in *in_ptr = (struct pdaf_ctrl_param_in *)in;
	struct pdaf_ctrl_param_out *out_ptr = (struct pdaf_ctrl_param_out *)out;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ISP_LOGV("cmd = %d", cmd);

	switch (cmd) {
	case PDAF_CTRL_CMD_SET_PARAM:
		ret = pdafsprd_adpt_set_param(adpt_handle, in_ptr);
		break;
	default:
		ISP_LOGE("fail to case cmd = %d", cmd);
		break;
	}

	return ret;
}

struct adpt_ops_type sprd_pdaf_adpt_ops = {
	.adpt_init = sprd_pdaf_adpt_init,
	.adpt_deinit = sprd_pdaf_adpt_deinit,
	.adpt_process = sprd_pdaf_adpt_process,
	.adpt_ioctrl = sprd_pdaf_adpt_ioctrl,
};
