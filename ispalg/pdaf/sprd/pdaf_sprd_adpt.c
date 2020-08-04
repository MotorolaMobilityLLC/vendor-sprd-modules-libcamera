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

static cmr_s32 PD_FRAME_ID = 0;
static cmr_s32 g_getype2 = 0;
static cmr_s32 g_getdual = 0;

struct sprd_pdaf_context {
	cmr_u32 camera_id;
	cmr_u8 af_mode;
	cmr_u8 af_type;
	cmr_u8 pd_open;
	cmr_u8 is_busy;
	cmr_u32 frame_id;
	cmr_u32 token_id;
	cmr_u8 pd_enable;
	cmr_handle caller_handle;
	isp_pdaf_cb pdaf_set_cb;
	 cmr_int(*pd_set_buffer) (struct pd_frame_in * cb_param);
	void *caller;
	PD_GlobalSetting pd_gobal_setting;
	struct pdaf_ppi_info ppi_info;
	struct pdaf_roi_info roi_info;
	struct sprd_pdaf_report_t report_data;
	struct af_win_rect touch_area;
	struct SetPD_ROI_param af_roi;
	cmr_u32 ot_switch;
	struct afctrl_ot_info ot_info;
	cmr_u32 pdaf_type;
	void *af_addr;// afm statis
	cmr_u32 af_addr_len;// afm statis buffer length
	 cmr_u32(*pdaf_set_pdinfo_to_af) (void *handle, struct pd_result * in_parm);
	 cmr_u32(*pdaf_set_cfg_parm) (void *handle, struct isp_dev_pdaf_info * in_parm);
	 cmr_u32(*pdaf_set_bypass) (void *handle, cmr_u32 in_parm);
	 cmr_u32(*pdaf_set_work_mode) (void *handle, cmr_u32 in_parm);
	 cmr_u32(*pdaf_set_skip_num) (void *handle, cmr_u32 in_parm);
	 cmr_u32(*pdaf_set_ppi_info) (void *handle, struct pdaf_ppi_info * in_parm);
	 cmr_u32(*pdaf_set_roi) (void *handle, struct pdaf_roi_info * in_parm);
	 cmr_u32(*pdaf_set_extractor_bypass) (void *handle, cmr_u32 in_parm);
};

#define PDAF_PATTERN_COUNT	 8
#define PDAF_FULL_NUM_IMX258 49920
#define PDAF_FULL_NUM_IMX362 1524096 //4032*756/2
#define PDAF_FULL_NUM_IMX362_SIZE 3810240  //4032*756*5/4


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

	pdaf_info_ptr->bypass = 0;
	pdaf_info_ptr->block_size = pdafTestCase_ptr->block_size;
	pdaf_info_ptr->win = pdafTestCase_ptr->win;
	pdaf_info_ptr->mode_sel = pdafTestCase_ptr->mode_sel;
	pdaf_info_ptr->skip_num = pdafTestCase_ptr->skip_num;

	for (i = 0; i < 64; i++) {
		pdaf_info_ptr->pattern_pixel_is_right[i] = pdafTestCase_ptr->pattern_pixel_is_right[i];
		pdaf_info_ptr->pattern_pixel_row[i] = pdafTestCase_ptr->pattern_pixel_row[i];
		pdaf_info_ptr->pattern_pixel_col[i] = pdafTestCase_ptr->pattern_pixel_col[i];
	}

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
	/* user for debug */
	/*struct isp_dev_pdaf_info pdaf_param;

	   memset(&pdaf_param, 0x00, sizeof(pdaf_param));
	   rtn = isp_get_pdaf_default_param(&pdaf_param);
	   cxt->pdaf_set_cfg_parm(cxt->caller, &pdaf_param); */
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

cmr_s32 pdaf_otp_info_parser(struct pdaf_ctrl_init_in * in_p)
{
	struct sensor_otp_section_info *pdaf_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;

	if (NULL != in_p->otp_info_ptr) {
		if (in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			pdaf_otp_info_ptr = in_p->otp_info_ptr->single_otp.pdaf_info;
			module_info_ptr = in_p->otp_info_ptr->single_otp.module_info;
			ISP_LOGV("pass pdaf otp, single cam");
		} else if (in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || in_p->otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (in_p->is_master == 1) {
				pdaf_otp_info_ptr = in_p->otp_info_ptr->dual_otp.master_pdaf_info;
				module_info_ptr = in_p->otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("pass pdaf otp, dual cam master");
			} else {
				pdaf_otp_info_ptr = in_p->otp_info_ptr->dual_otp.slave_pdaf_info;
				module_info_ptr = in_p->otp_info_ptr->dual_otp.slave_module_info;
				ISP_LOGV("pass pdaf otp, dual cam slave");
			}
		}
	} else {
		pdaf_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("pdaf otp_info_ptr is NULL");
	}

	if (NULL != pdaf_otp_info_ptr && NULL != module_info_ptr) {
		cmr_u8 *module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL != module_info) {
			if ((module_info[4] == 0 && module_info[5] == 1)
				|| (module_info[4] == 0 && module_info[5] == 2)
				|| (module_info[4] == 0 && module_info[5] == 3)
				|| (module_info[4] == 0 && module_info[5] == 4)
				|| (module_info[4] == 0 && module_info[5] == 5)
				|| (module_info[4] == 1 && module_info[5] == 0 && (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
				|| (module_info[4] == 2 && module_info[5] == 0)
				|| (module_info[4] == 3 && module_info[5] == 0)
				|| (module_info[4] == 4 && module_info[5] == 0)
				|| (module_info[4] == 5 && module_info[5] == 0)) {
				ISP_LOGV("pdaf otp map v0.4 or v0.5");
				in_p->pdaf_otp.otp_data = pdaf_otp_info_ptr->rdm_info.data_addr;
				in_p->pdaf_otp.size = pdaf_otp_info_ptr->rdm_info.data_size;
			} else if (module_info[4] == 1 && (module_info[5] == 0 || module_info[5] == 1) && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
				ISP_LOGV("pdaf otp map v1.0 or v1.1");
				in_p->pdaf_otp.otp_data = (void *)((cmr_u8 *) pdaf_otp_info_ptr->rdm_info.data_addr + 1);
				in_p->pdaf_otp.size = pdaf_otp_info_ptr->rdm_info.data_size - 1;
			} else {
				in_p->pdaf_otp.otp_data = NULL;
				in_p->pdaf_otp.size = 0;
				ISP_LOGE("pdaf otp map version error");
			}
		} else {
			in_p->pdaf_otp.otp_data = NULL;
			in_p->pdaf_otp.size = 0;
			ISP_LOGE("pdaf module_info is NULL");
		}
	} else {
		ISP_LOGE("pfaf pdaf_otp_info_ptr = %p, module_info_ptr = %p. Parser fail !", pdaf_otp_info_ptr, module_info_ptr);
	}

	return ISP_SUCCESS;
}

cmr_handle sprd_pdaf_adpt_init(void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct pdaf_ctrl_init_in *in_p = (struct pdaf_ctrl_init_in *)in;
	struct sprd_pdaf_context *cxt = NULL;
	struct isp_alg_fw_context *isp_ctx = NULL;
	cmr_u16 i;
	cmr_u32 base_w = 512, base_h = 384;
	cmr_s32 offset_w = 0, offset_h = 0, temp_win_start_x = 0, temp_win_start_y = 0;
	UNUSED(out);

	if (!in_p) {
		ISP_LOGE("fail to check init param %p!!!", in_p);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	// parser pdaf otp info
	pdaf_otp_info_parser(in_p);

	char otp_pdaf_name[1024];

	char value[PROPERTY_VALUE_MAX];
	property_get("ro.debuggable", value, "0");
	if (!strcmp(value, "1")) {
		// userdebug: replace otp with binary file specific by property
		property_get("debug.isp.pdaf.otp.filename", otp_pdaf_name, "/dev/null");
		if (strcmp(otp_pdaf_name, "/dev/null") != 0) {
			FILE *fp = fopen(otp_pdaf_name, "rb");
			if (fp != NULL) {
				if (in_p->pdaf_otp.otp_data != NULL) {
					in_p->pdaf_otp.size = fread(in_p->pdaf_otp.otp_data, 1, 1024, fp);
				}
				fclose(fp);
			}
		}
	} else {
		// user: replace pdaf otp if /productinfo/otp_pdaf.bin exist
		strcpy(otp_pdaf_name, "/productinfo/otp_pdaf.bin");
		FILE *fp = fopen(otp_pdaf_name, "rb");
		if (fp != NULL) {
			if (in_p->pdaf_otp.otp_data != NULL) {
				in_p->pdaf_otp.size = fread(in_p->pdaf_otp.otp_data, 1, 1024, fp);
			}
			fclose(fp);
		}
	}

	isp_ctx = (struct isp_alg_fw_context *)in_p->caller_handle;
	cxt = (struct sprd_pdaf_context *)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		ISP_LOGE("fail to malloc pdaf");
		ret = -ISP_ALLOC_ERROR;
		goto exit;
	}

	memset(cxt, 0, sizeof(*cxt));
	cxt->caller = in_p->caller;
	cxt->caller_handle = in_p->caller_handle;
	cxt->pdaf_set_pdinfo_to_af = in_p->pdaf_set_pdinfo_to_af;
	cxt->pdaf_set_cfg_parm = in_p->pdaf_set_cfg_param;
	cxt->pdaf_set_bypass = in_p->pdaf_set_bypass;
	cxt->pdaf_set_work_mode = in_p->pdaf_set_work_mode;
	cxt->pdaf_set_skip_num = in_p->pdaf_set_skip_num;
	cxt->pdaf_set_ppi_info = in_p->pdaf_set_ppi_info;
	cxt->pdaf_set_roi = in_p->pdaf_set_roi;
	cxt->pdaf_set_extractor_bypass = in_p->pdaf_set_extractor_bypass;
	cxt->af_type = PASSIVE;
	cxt->pdaf_type = in_p->pdaf_support;
	/*TBD dSensorID 0:Imx258 Type3 1:OV13855 2:3L8 3:IMX258 Type2 4:IMX362 Dual PD 5:OV12A10*/
	if (SENSOR_VENDOR_IMX258_TYPE3 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_0;
	} else if (SENSOR_VENDOR_OV13855 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_1;
	} else if (SENSOR_VENDOR_S5K3L8XXM3 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_2;
	}else if (SENSOR_VENDOR_IMX258_TYPE2 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_3;
	} else if (SENSOR_VENDOR_IMX362_DUAL_PD == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_4;
	} else if (SENSOR_VENDOR_OV12A10 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_5;
	} else if (SENSOR_VENDOR_OV16885 == in_p->pd_info->vendor_type) {
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_6;
	} else {
		ISP_LOGE("fail to support the sensor:%d\n", in_p->pd_info->vendor_type);
		goto exit;
	}

	ISP_LOGV("PDALGO Init. Sensor Mode[%d] ", cxt->pd_gobal_setting.dSensorMode);

	cxt->pd_gobal_setting.dImageW = in_p->sensor_max_size.w;
	cxt->pd_gobal_setting.dImageH = in_p->sensor_max_size.h;
	cxt->pd_gobal_setting.OTPBuffer = in_p->pdaf_otp.otp_data;
	cxt->pd_gobal_setting.dAreaW = (in_p->pd_info->pd_end_x - in_p->pd_info->pd_offset_x);
	cxt->pd_gobal_setting.dAreaH = (in_p->pd_info->pd_end_y - in_p->pd_info->pd_offset_y);
	cxt->pd_gobal_setting.dCalibration = 1;
	cxt->pd_gobal_setting.dOVSpeedup = 1;
	//0: Normal, 1:Mirror+Flip
	cxt->pd_gobal_setting.dSensorSetting = in_p->pd_info->sns_orientation;
	ISP_LOGV("gobal_setting.dSensorSetting = %d\n", cxt->pd_gobal_setting.dSensorSetting);

	cxt->pd_gobal_setting.pd_unit_w = (8 << in_p->pd_info->pd_block_w);	//pd_block_w: 1->16*16	2->32*32	3->64*64
	cxt->pd_gobal_setting.pd_unit_h = (8 << in_p->pd_info->pd_block_h);
	cxt->pd_gobal_setting.pd_pair_w = in_p->pd_info->pd_density_x;
	cxt->pd_gobal_setting.pd_pair_h = in_p->pd_info->pd_density_y;
	cxt->pd_gobal_setting.pd_pairs_num_unit = in_p->pd_info->pd_pos_size;

	if((in_p->pd_info->pd_pos_size * 2) > PD_PIXEL_PAIRS_NUM){
		ISP_LOGE("Pd pixels number greater than Array Size!");
		goto exit;
	}
	for (i = 0; i < in_p->pd_info->pd_pos_size * 2; i++) {
		cxt->pd_gobal_setting.pd_is_right[i] = in_p->pd_info->pd_is_right[i];
		cxt->pd_gobal_setting.pd_pos_row[i] = in_p->pd_info->pd_pos_row[i];
		cxt->pd_gobal_setting.pd_pos_col[i] = in_p->pd_info->pd_pos_col[i];
	}

	if(cxt->pd_gobal_setting.dAreaW < ROI_Width * 2 || cxt->pd_gobal_setting.dAreaH < ROI_Height * 2){
		base_w = (cxt->pd_gobal_setting.dAreaW / cxt->pd_gobal_setting.pd_unit_w / 8) * cxt->pd_gobal_setting.pd_unit_w;
		base_h = (cxt->pd_gobal_setting.dAreaH / cxt->pd_gobal_setting.pd_unit_h / 8) * cxt->pd_gobal_setting.pd_unit_h;
	}

	cxt->pd_gobal_setting.dBeginX = in_p->pd_info->pd_offset_x;
	cxt->pd_gobal_setting.dBeginY = in_p->pd_info->pd_offset_y;
	temp_win_start_x = cxt->pd_gobal_setting.dBeginX + 2 * base_w;
	temp_win_start_y = cxt->pd_gobal_setting.dBeginY + 2 * base_h;
	offset_w = cxt->pd_gobal_setting.dAreaW - 8 * base_w;
	offset_h = cxt->pd_gobal_setting.dAreaH - 8 * base_h;

	if((offset_w / cxt->pd_gobal_setting.pd_unit_w) > 1){
		if(0 == (offset_w / cxt->pd_gobal_setting.pd_unit_w) % 2){
			cxt->pd_gobal_setting.dBeginX += (offset_w / 2);
			temp_win_start_x += (offset_w / 2);
		} else {
			cxt->pd_gobal_setting.dBeginX += ((offset_w - cxt->pd_gobal_setting.pd_unit_w) / 2);
			temp_win_start_x += ((offset_w - cxt->pd_gobal_setting.pd_unit_w) / 2);
		}
	}

	if((offset_h / cxt->pd_gobal_setting.pd_unit_h) > 1){
		if(0 == (offset_h / cxt->pd_gobal_setting.pd_unit_h) % 2){
			cxt->pd_gobal_setting.dBeginY += (offset_h / 2);
			temp_win_start_y += (offset_h / 2);
		} else {
			cxt->pd_gobal_setting.dBeginY += ((offset_h - cxt->pd_gobal_setting.pd_unit_h) / 2);
			temp_win_start_y += ((offset_h - cxt->pd_gobal_setting.pd_unit_h) / 2);
		}
	}
	cxt->pd_gobal_setting.dAreaW = 8 * base_w;
	cxt->pd_gobal_setting.dAreaH = 8 * base_h;
	ISP_LOGI("ROI_start_coor(%d, %d) ROI_WH(%d, %d) PD_area_begin_coor(%d, %d) PD_area_WH(%d, %d)", temp_win_start_x, temp_win_start_y,
				(4*base_w), (4*base_h), cxt->pd_gobal_setting.dBeginX, cxt->pd_gobal_setting.dBeginY, cxt->pd_gobal_setting.dAreaW, cxt->pd_gobal_setting.dAreaH);

	cxt->ppi_info.block_size.height = in_p->pd_info->pd_block_h;
	cxt->ppi_info.block_size.width = in_p->pd_info->pd_block_w;
	for (i=0; i< in_p->pd_info->pd_pos_size * 2; i++) {
		cxt->ppi_info.pattern_pixel_is_right[i] = in_p->pd_info->pd_is_right[i];
		cxt->ppi_info.pattern_pixel_row[i] = in_p->pd_info->pd_pos_row[i];
		cxt->ppi_info.pattern_pixel_col[i] = in_p->pd_info->pd_pos_col[i];
	}


	cxt->ppi_info.block.start_x = in_p->pd_info->pd_offset_x;
	cxt->ppi_info.block.end_x = in_p->pd_info->pd_end_x;
	cxt->ppi_info.block.start_y = in_p->pd_info->pd_offset_y;
	cxt->ppi_info.block.end_y = in_p->pd_info->pd_end_y;

	cxt->roi_info.win.start_x = temp_win_start_x;
	cxt->roi_info.win.start_y = temp_win_start_y;
	cxt->roi_info.win.end_x = cxt->roi_info.win.start_x + 4 * base_w;
	cxt->roi_info.win.end_y = cxt->roi_info.win.start_y + 4 * base_h;

	cmr_s32 block_num_x = (cxt->roi_info.win.end_x - cxt->roi_info.win.start_x) / (8 << cxt->ppi_info.block_size.width);
	cmr_s32 block_num_y = (cxt->roi_info.win.end_y - cxt->roi_info.win.start_y) / (8 << cxt->ppi_info.block_size.height);
	cmr_u32 phasepixel_total_num = block_num_x * block_num_y * in_p->pd_info->pd_pos_size;
	cxt->roi_info.phase_data_write_num = (phasepixel_total_num + 5) / 6;


	property_get("debug.isp.pdaf.otp.dump", otp_pdaf_name, "/dev/null");
	if (strcmp(otp_pdaf_name, "/dev/null") != 0) {
		FILE *fp = fopen(otp_pdaf_name, "wb");
		if (fp != NULL) {
			if (in_p->pdaf_otp.otp_data != NULL) {
				fwrite(in_p->pdaf_otp.otp_data, 1, in_p->pdaf_otp.size, fp);
			}
			fclose(fp);
		}
	}
	char prop[256];
	char prop1[256];

	property_get("debug.isp.pdaf.getype2", prop, "0");
	g_getype2 = atoi(prop);

	property_get("debug.isp.pdaf.getdual", prop1, "0");
	g_getdual = atoi(prop1);

	if(cxt->pd_gobal_setting.dSensorMode ==SENSOR_ID_5){
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_1;
		ret = PD_Init((void *)&cxt->pd_gobal_setting);
		cxt->pd_gobal_setting.dSensorMode = SENSOR_ID_5;
	}
	else{
		ret = PD_Init((void *)&cxt->pd_gobal_setting);
	}

	//ret = PD_Init((void *)&cxt->pd_gobal_setting);

	if (ret) {
		ISP_LOGE("fail to init lib %ld", ret);
		goto exit;
	}

	cxt->is_busy = 0;

	return (cmr_handle) cxt;

  exit:
	if (cxt) {
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
		if(NULL != cxt->af_addr)// free afm statis buffer
			free(cxt->af_addr);

		ret = (cmr_s32) PD_Uninit();
		memset(cxt, 0x00, sizeof(*cxt));
		free(cxt);
		cxt = NULL;
	}
	ISP_LOGI("done,ret = %d", ret);

	return ret;
}

static cmr_s32 sprd_pdaf_adpt_process(cmr_handle adpt_handle, void *in, void *out)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	struct pdaf_ctrl_process_in *proc_in = (struct pdaf_ctrl_process_in *)in;
	struct pdaf_ctrl_callback_in callback_in;
	struct pd_result pd_calc_result;
	cmr_s32 dRectX = 0;
	cmr_s32 dRectY = 0;
	cmr_s32 dRectW = 0;
	cmr_s32 dRectH = 0;
	cmr_s32 dBuf_width = 0;
	cmr_s32 dBuf_height = 0;
	cmr_s32 *pPD_left = NULL;
	cmr_s32 *pPD_right = NULL;
	cmr_s32 *pPD_left_final = NULL;
	cmr_s32 *pPD_right_final = NULL;
	cmr_s32 *pPD_left_reorder = NULL;
	cmr_s32 *pPD_right_reorder = NULL;

	//For IMX258 Type2
	cmr_u16 *pBufLeft_Type2 = NULL;
	cmr_u16 *pBufRight_Type2 = NULL;
	cmr_u16 *pBufLeft_Type2_ROI = NULL;
	cmr_u16 *pBufRight_Type2_ROI = NULL;
	cmr_s32 ROI_Start = 0;
	cmr_s32 dummy = 80;
	cmr_s32 offset = 80*5;
	cmr_s32 index = 0;

	cmr_s32 i=0;
	cmr_s32 j=0;
	cmr_s32 area_index;
	cmr_s32 pixel_num_x = 0;
	cmr_s32 pixel_num_y = 0;
	cmr_s32 pixel_num = 0;
	char value[PROPERTY_VALUE_MAX];
	void *pInPhaseBuf_left = NULL;
	void *pInPhaseBuf_right = NULL;
	cmr_u16 *pInPhaseBuf_Type2 = NULL;
	cmr_u8 * pInPhaseBuf_Dual_PD = NULL;

	UNUSED(out);
	if (!in) {
		ISP_LOGE("fail to init param %p!!!", in);
		ret = ISP_PARAM_NULL;
		return ret;
	}

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	memset(&callback_in, 0, sizeof(callback_in));
	memset(&pd_calc_result, 0, sizeof(pd_calc_result));
	cxt->is_busy = 1;

	if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_3){
		pInPhaseBuf_Type2 = (cmr_u16 *) (cmr_uint)(proc_in->u_addr);
		ISP_LOGV("pInPhaseBuf_Type2 = %p", pInPhaseBuf_Type2);
	}
	else if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_4){
		pInPhaseBuf_Dual_PD = (cmr_u8 *) (cmr_uint)(proc_in->u_addr);
		ISP_LOGV("pInPhaseBuf_Dual_PD = %p", pInPhaseBuf_Dual_PD);
	}else{
		pInPhaseBuf_left = (cmr_s32 *) (cmr_uint)(proc_in->u_addr);
		pInPhaseBuf_right = (cmr_s32 *) (cmr_uint)(proc_in->u_addr_right);
		ISP_LOGV("pInPhaseBuf_left = %p", pInPhaseBuf_left);
	}


	dRectX = cxt->roi_info.win.start_x;
	dRectY = cxt->roi_info.win.start_y;
	dRectW = cxt->roi_info.win.end_x - cxt->roi_info.win.start_x;
	dRectH = cxt->roi_info.win.end_y - cxt->roi_info.win.start_y;

	pixel_num_x = dRectW / cxt->pd_gobal_setting.pd_pair_w;
	pixel_num_y = dRectH / cxt->pd_gobal_setting.pd_pair_h;
	pixel_num = pixel_num_x * pixel_num_y;

	property_get("debug.camera.dump.pdaf.raw",(char *)value,"0");
	if(atoi(value)) {
		if((PD_FRAME_ID % 30) ==0){
			//#define MLOG_BUF_SIZE 1024
			#define MLOG_FILE_NAME_SIZE 200
			char file_name_r[MLOG_FILE_NAME_SIZE] = {0};
			char file_name_l[MLOG_FILE_NAME_SIZE] = {0};
			cmr_s32 *pTempBuf = NULL;
			cmr_s32 dumpNum = (pixel_num * 4) / 3;
			FILE *fp = NULL;
			sprintf(file_name_l, CAMERA_DATA_FILE"/pdaf_L_tp3_%dX%d_%d.raw", pixel_num_x, pixel_num_y, PD_FRAME_ID);
			sprintf(file_name_r, CAMERA_DATA_FILE"/pdaf_R_tp3_%dX%d_%d.raw", pixel_num_x, pixel_num_y, PD_FRAME_ID);

			pTempBuf = ((pInPhaseBuf_left == NULL) ? NULL : pInPhaseBuf_left);
			fp = fopen(file_name_l, "wb+");
			if(fp != NULL && pTempBuf != NULL){
				fwrite(pTempBuf, sizeof(cmr_u8), dumpNum, fp);
			}
			if(fp != NULL){
				fclose(fp);
			}
			pTempBuf = NULL;
			fp = NULL;

			pTempBuf = ((pInPhaseBuf_right == NULL) ? NULL : pInPhaseBuf_right);
			fp = fopen(file_name_r, "wb+");
			if(fp != NULL && pTempBuf != NULL){
				fwrite(pTempBuf, sizeof(cmr_u8), dumpNum, fp);
			}
			if(fp != NULL){
				fclose(fp);
			}
			fp = NULL;
			pTempBuf = NULL;
		}
		PD_FRAME_ID++;
	}

	ISP_LOGV("PDALGO Converter. Sensor[%d] Mode[%d]", cxt->pd_gobal_setting.dSensorSetting, cxt->pd_gobal_setting.dSensorMode);

	pPD_left  = (cmr_s32 *)malloc(pixel_num*  (cmr_s32) sizeof(cmr_s32));
	pPD_right = (cmr_s32 *)malloc(pixel_num*  (cmr_s32) sizeof(cmr_s32));
	pPD_left_reorder  = (cmr_s32 *)malloc(pixel_num * (cmr_s32) sizeof(cmr_s32));
	pPD_right_reorder = (cmr_s32 *)malloc(pixel_num* (cmr_s32) sizeof(cmr_s32));

	//For IMX258 Type2
	pBufLeft_Type2 = (cmr_u16 *)malloc(PDAF_FULL_NUM_IMX258*sizeof(cmr_u16));
	pBufRight_Type2 = (cmr_u16 *)malloc(PDAF_FULL_NUM_IMX258*sizeof(cmr_u16));
	pBufLeft_Type2_ROI = (cmr_u16 *)malloc(12288*sizeof(cmr_u16));
	pBufRight_Type2_ROI = (cmr_u16 *)malloc(12288*sizeof(cmr_u16));

	if(cxt->pd_gobal_setting.dSensorMode==SENSOR_ID_3) {
		dummy = 80;
		offset = 80*5;
		index = 0;
		if(cxt->pd_gobal_setting.dSensorSetting == 1){ //Mirror+Flip
			for(i=0;i<153600;i=i+1600){
				for(j=0;j<260;j++){
					*(pBufRight_Type2+index+j) = *(pInPhaseBuf_Type2+i+dummy+j);
					*(pBufLeft_Type2+index+j) = *(pInPhaseBuf_Type2+i+offset+j);
				}
				index = index+ 260;

				for(j=0;j<260;j++){
					*(pBufLeft_Type2+index+j) = *(pInPhaseBuf_Type2+i+800+dummy+j);
					*(pBufRight_Type2+index+j) = *(pInPhaseBuf_Type2+i+800+offset+j);
				}
				index = index+ 260;
			}
		}
		else{ //Normal
			for(i=0;i<153600;i=i+1600){
				for(j=0;j<260;j++){
					*(pBufLeft_Type2+index+j) = *(pInPhaseBuf_Type2+i+dummy+j);
					*(pBufRight_Type2+index+j) = *(pInPhaseBuf_Type2+i+offset+j);
				}
				index = index+ 260;

				for(j=0;j<260;j++){
					*(pBufRight_Type2+index+j) = *(pInPhaseBuf_Type2+i+800+dummy+j);
					*(pBufLeft_Type2+index+j) = *(pInPhaseBuf_Type2+i+800+offset+j);
				}
				index = index+ 260;
			}
		}

		//Crop ROI
		ROI_Start = 48*260+64;
		for(i=0;i<96;i++){
			for(j=0;j<128;j++){
				*(pPD_left+i*128+j) = *(pBufLeft_Type2 + ((cmr_s32) ROI_Start + i*260+j));
				*(pPD_right+i*128+j) = *(pBufRight_Type2 +  ((cmr_s32)ROI_Start + i*260+j));
				*(pBufLeft_Type2_ROI+i*128+j) = *(pBufLeft_Type2 + ((cmr_s32)ROI_Start + i*260+j));
				*(pBufRight_Type2_ROI+i*128+j) = *(pBufRight_Type2 + ((cmr_s32)ROI_Start + i*260+j));
			}
		}

		//Dump Left/Right Buffer [adb shell setprop debug.isp.pdaf.getype2 1]
		if(g_getype2 == 1) {
			if((PD_FRAME_ID % 40) ==0){
				char file_name_l[200] = {0};
				char file_name_r[200] = {0};
				char file_name_l_ROI[200] = {0};
				char file_name_r_ROI[200] = {0};

				FILE *fp = NULL;
				sprintf(file_name_l, CAMERA_DATA_FILE"/Type2BufferL_%d.raw", PD_FRAME_ID);
				sprintf(file_name_r, CAMERA_DATA_FILE"/Type2BufferR_%d.raw", PD_FRAME_ID);
				sprintf(file_name_l_ROI, CAMERA_DATA_FILE"/Type2BufferL_ROI_%d.raw", PD_FRAME_ID);
				sprintf(file_name_r_ROI, CAMERA_DATA_FILE"/Type2BufferR_ROI_%d.raw", PD_FRAME_ID);

				fp = fopen(file_name_l, "wb");
				if (fp != NULL && pBufLeft_Type2 != NULL){
					fwrite((void*)pBufLeft_Type2, 1, 99840, fp); //49920*2 bytes
					fclose(fp);
				}
				fp = fopen(file_name_r, "wb");
				if (fp != NULL && pBufRight_Type2 != NULL){
					fwrite((void*)pBufRight_Type2, 1, 99840, fp); //49920*2 bytes
					fclose(fp);
				}
				fp = fopen(file_name_l_ROI, "wb");
				if (fp != NULL && pBufLeft_Type2_ROI != NULL){
					fwrite((void*)pBufLeft_Type2_ROI, 1, 24576, fp); //12288*2 bytes
					fclose(fp);
				}
				fp = fopen(file_name_r_ROI, "wb");
				if (fp != NULL && pBufRight_Type2_ROI != NULL){
					fwrite((void*)pBufRight_Type2_ROI, 1, 24576, fp); //12288*2 bytes
					fclose(fp);
				}

				fp = NULL;
			}
			PD_FRAME_ID++;
		}
	}

	//For IMX362 Dual PD Mode4
	if(cxt->pd_gobal_setting.dSensorMode==SENSOR_ID_4) {
#if(0)
		ISP_LOGI("PDALGO Dual PD Parser S: SensorID[%d]", cxt->pd_gobal_setting.dSensorMode);

		cmr_s32 *pBufLeft_DualPD_IMX362 = NULL;
		cmr_s32 *pBufRight_DualPD_IMX362 = NULL;

		pBufLeft_DualPD_IMX362 = (cmr_s32 *)malloc(PDAF_FULL_NUM_IMX362*sizeof(cmr_s32));
		pBufRight_DualPD_IMX362 = (cmr_s32 *)malloc(PDAF_FULL_NUM_IMX362*sizeof(cmr_s32));

		index=0;
		for(i=0;i<PDAF_FULL_NUM_IMX362_SIZE;i+=5) {
			*(pBufLeft_DualPD_IMX362+index)  = (*(pInPhaseBuf_Dual_PD+i+0) << 2) | (*(pInPhaseBuf_Dual_PD+i+4) & 0x03);
			*(pBufRight_DualPD_IMX362+index) = (*(pInPhaseBuf_Dual_PD+i+1) << 2) | ((*(pInPhaseBuf_Dual_PD+i+4) & 0x0C) >> 2);
			*(pBufLeft_DualPD_IMX362+index+1)  = (*(pInPhaseBuf_Dual_PD+i+2) << 2) | ((*(pInPhaseBuf_Dual_PD+i+4) & 0x30) >> 4);
			*(pBufRight_DualPD_IMX362+index+1) = (*(pInPhaseBuf_Dual_PD+i+3) << 2) | ((*(pInPhaseBuf_Dual_PD+i+4) & 0xC0) >> 6);

			index+=2;
		}

		free(pBufLeft_DualPD_IMX362);
		free(pBufRight_DualPD_IMX362);
		ISP_LOGI("PDALGO Dual PD E: Parser Index:[%d]", cxt->pd_gobal_setting.dSensorMode);
#endif
		//Dump Left/Right Dual PD Buffer [adb shell setprop debug.isp.pdaf.getdual 1]
		if(g_getdual == 1) {
			if((PD_FRAME_ID % 30) ==0){
				char file_name_dual_l[200] = {0};
				char file_name_dual_r[200] = {0};
				cmr_u8 temp[5] = {0};
				cmr_s32 dual_index = 0;
				cmr_s16 dual_left0 = 0, dual_left1 = 0;
				cmr_s16 dual_right0 = 0, dual_right1 = 0;
				cmr_u8 *DualTempBuf = NULL;

				FILE *fpL = NULL;
				FILE *fpR = NULL;
				sprintf(file_name_dual_l, CAMERA_DATA_FILE"/DualPDBuf_L_2016X756_%d.raw", PD_FRAME_ID);
				sprintf(file_name_dual_r, CAMERA_DATA_FILE"/DualPDBuf_R_2016X756_%d.raw", PD_FRAME_ID);

				DualTempBuf = pInPhaseBuf_Dual_PD;
				fpL = fopen(file_name_dual_l, "wb+");
				fpR = fopen(file_name_dual_r, "wb+");
				while(dual_index != PDAF_FULL_NUM_IMX362_SIZE && *DualTempBuf){	//4032*756*5/4 bytes
					for(int i = 0; i < 5; i++){
						temp[i] = *DualTempBuf;
						DualTempBuf++;
					}
					dual_left0 = ((temp[0] << 2) | (temp[4] & 0x03));	//mipi_raw convert to raw
					dual_left1 = ((temp[2] << 2) | ((temp[4] & 0x30) >> 4));

					dual_right0 = ((temp[1] << 2) | ((temp[4] & 0x0c) >> 2));
					dual_right1 = ((temp[3] << 2) | ((temp[4] & 0xc0) >> 6));

					if (fpL !=NULL){
						fwrite(&dual_left0, sizeof(cmr_s16), 1, fpL);
					}
					if ( fpL !=NULL){
						fwrite(&dual_left1, sizeof(cmr_s16), 1, fpL);
					}
					if (fpR !=NULL){
						fwrite(&dual_right0, sizeof(cmr_s16), 1, fpR);
					}
					if (fpR !=NULL){
						fwrite(&dual_right1, sizeof(cmr_s16), 1, fpR);
					}
					dual_index = dual_index + 5;
				}
				if(fpL !=NULL){
					fclose(fpL);
				}
				if(fpR !=NULL){
					fclose(fpR);
				}

				DualTempBuf = NULL;
				fpL = NULL;
				fpR = NULL;
			}
			PD_FRAME_ID++;
		}
	}

	if( (cxt->pd_gobal_setting.dSensorMode != SENSOR_ID_3) && (cxt->pd_gobal_setting.dSensorMode != SENSOR_ID_4)) {
	  ret = PD_PhaseFormatConverter((cmr_u8 *)pInPhaseBuf_left, (cmr_u8 *)pInPhaseBuf_right, pPD_left, pPD_right, pixel_num, pixel_num);
	}
	else{
		ISP_LOGV("PDALGO No need Converter. SensorID[%d]", cxt->pd_gobal_setting.dSensorMode);
	}

	pPD_left_final = pPD_left;
	pPD_right_final = pPD_right;

	//Phase Pixel Reorder: for 3L8
	if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_2) {
		dBuf_width = dRectW / SENSOR_ID_2_BLOCK_W * 4;
		dBuf_height = dRectH / SENSOR_ID_2_BLOCK_H * 4;
		ISP_LOGI("PDALGO Reorder Buf. W[%d] H[%d]", dBuf_width, dBuf_height);
		ret = PD_PhasePixelReorder(pPD_left_final, pPD_right_final, pPD_left_reorder, pPD_right_reorder, dBuf_width, dBuf_height);
	}

	if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_4) {
		switch(cxt->af_type) {
		case MULTIZONE:
			for (area_index = 0; area_index < cxt->af_roi.ROI_Size; area_index++) {
				ret = PD_DoPoint2((void *)pInPhaseBuf_Dual_PD, cxt->af_roi.ROI_info[area_index].Center_X, cxt->af_roi.ROI_info[area_index].Center_Y,
				 cxt->af_roi.ROI_info[area_index].sWidth, cxt->af_roi.ROI_info[area_index].sHeight);
				if (ret) {
					ISP_LOGE("fail to do algo.");
					goto exit;
				}
				ret = PD_GetResult(&pd_calc_result.pdConf[area_index], &pd_calc_result.pdPhaseDiff[area_index], &pd_calc_result.pdGetFrameID, &pd_calc_result.pdDCCGain[area_index], area_index);
				if (ret) {
					ISP_LOGE("fail to get pd_result.");
					goto exit;
				}
			}
		break;
		case ACTIVE:
			if(1 == cxt->ot_switch) {
				ISP_LOGI("center x,y (%d,%d)", cxt->ot_info.centorX, cxt->ot_info.centorY);
				ret = PD_DoPoint2((void *)pInPhaseBuf_Dual_PD, cxt->ot_info.centorX, cxt->ot_info.centorY, 480, 480);
			} else {
				ret = PD_DoPoint2((void *)pInPhaseBuf_Dual_PD, ((cxt->touch_area.sx + cxt->touch_area.ex)>>2)<<1, ((cxt->touch_area.sy + cxt->touch_area.ey)>>2)<<1,
					 480, 480);
			}
		break;
		case PASSIVE:
		default:
			ret = PD_DoPoint2((void *)pInPhaseBuf_Dual_PD, 2016, 1512, 512, 384);
		break;
		}
		if (ret) {
			ISP_LOGE("fail to get pd_result.");
			goto exit;
		}
	}
	else {
		for (area_index = 0; area_index < 1; area_index++) {
			if (cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_2) {
				ret = PD_DoType2((void *)pPD_left_reorder, (void *)pPD_right_reorder, dRectX, dRectY, dRectW, dRectH, area_index);
			} else {
				ret = PD_DoType2((void *)pPD_left_final, (void *)pPD_right_final, dRectX, dRectY, dRectW, dRectH, area_index);
			}

			if (ret) {
				ISP_LOGE("fail to do pd algo.");
				goto exit;
			}
		}
		for (area_index = 0; area_index < 1; area_index++) {
			ret = PD_GetResult(&pd_calc_result.pdConf[area_index], &pd_calc_result.pdPhaseDiff[area_index], &pd_calc_result.pdGetFrameID, &pd_calc_result.pdDCCGain[area_index], area_index);
			if (ret) {
				ISP_LOGE("fail to do get pd_result.");
				goto exit;
			}
		}
	}
	if(MULTIZONE != cxt->af_type) {// normal way for PASSIVE and ACTIVE mode
		ret = PD_GetResult(&pd_calc_result.pdConf[4], &pd_calc_result.pdPhaseDiff[4], &pd_calc_result.pdGetFrameID, &pd_calc_result.pdDCCGain[4], 0);
		if (ret) {
			ISP_LOGE("fail to do get pd_result.");
			goto exit;
		}
		pd_calc_result.pd_roi_num = AREA_LOOP + 1;
	} else {
		pd_calc_result.pd_roi_num = cxt->af_roi.ROI_Size;
	}
	pd_calc_result.af_type = cxt->af_type;

	if(cxt->pd_gobal_setting.dSensorMode == SENSOR_ID_4) {
		cmr_u16 i = 0;
		for(; i < MAX_MULTIZONE_NUM + 1; i++) {
			pd_calc_result.pdDCCGain[i] = 34;
		}
	}

	//Transfer Sensor ID to AF_adpt: MAX_MULTIZONE_NUM = 45
	pd_calc_result.pdDCCGain[15] = 2000 + cxt->pd_gobal_setting.dSensorMode;
	ISP_LOGV("PD_GetResult pd_calc_result.pdConf[4] = %d, pd_calc_result.pdPhaseDiff[4] = 0x%lf, DCC[4]= %d, DCC[15]=%d",
	pd_calc_result.pdConf[4], pd_calc_result.pdPhaseDiff[4], pd_calc_result.pdDCCGain[4], pd_calc_result.pdDCCGain[15]);

	cxt->pdaf_set_pdinfo_to_af(cxt->caller, &pd_calc_result);
	cxt->frame_id++;
  exit:
	cxt->is_busy = 0;

	free(pPD_left);
	free(pPD_right);
	free(pPD_left_reorder);
	free(pPD_right_reorder);
	free(pBufLeft_Type2);
	free(pBufRight_Type2);
	free(pBufLeft_Type2_ROI);
	free(pBufRight_Type2_ROI);

	return ret;
}

static cmr_s32 pdafsprd_adpt_set_param(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	UNUSED(in);

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	pdaf_setup(cxt);
	return ret;
}

static cmr_s32 pdafsprd_adpt_get_busy(cmr_handle adpt_handle, struct pdaf_ctrl_param_out *out)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	out->is_busy = cxt->is_busy;
	return ret;
}

static cmr_s32 pdafsprd_adpt_disable_pdaf(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	UNUSED(in);

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	if (cxt->pdaf_set_bypass) {
		cxt->pdaf_set_bypass(cxt->caller, 1);
	}
	return ret;
}

static cmr_s32 pdafsprd_adpt_set_coor(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	cxt->touch_area.sx = in->touch_area.sx;
	cxt->touch_area.sy = in->touch_area.sy;
	cxt->touch_area.ex = in->touch_area.ex;
	cxt->touch_area.ey = in->touch_area.ey;
	ISP_LOGI("touch pd af coor (%d %d %d %d)",cxt->touch_area.sx, cxt->touch_area.sy, cxt->touch_area.ex ,cxt->touch_area.ey);

	return ret;
}

static cmr_s32 pdafsprd_adpt_set_mode(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	cxt->af_mode = in->af_mode;

	switch (in->af_mode) {
	case AF_MODE_NORMAL:
		cxt->af_type = ACTIVE;
		break;
	case AF_MODE_CONTINUE:
	case AF_MODE_VIDEO:
		if (1 == cxt->ot_switch) {
			cxt->af_type = ACTIVE;
		} else {
			cxt->af_type = PASSIVE;
		}
		break;
	default:
		cxt->af_type = PASSIVE;
		ISP_LOGW("af_mode %d is not supported", in->af_mode);
		break;
	}

	ISP_LOGI("pdaf mode %s",cxt->af_type == ACTIVE ? "active" : "passive");

	return ret;
}

static cmr_s32 pdafsprd_adpt_set_multizone(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;
	cmr_u32 i = 0;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	memset(&cxt->af_roi, 0, sizeof(cxt->af_roi));
	for(; i < in->af_roi.ROI_Size; i++) {
		cxt->af_roi.ROI_info[i].Center_X = in->af_roi.ROI_info[i].Center_X;
		cxt->af_roi.ROI_info[i].Center_Y = in->af_roi.ROI_info[i].Center_Y;
		cxt->af_roi.ROI_info[i].sWidth = in->af_roi.ROI_info[i].sWidth;
		cxt->af_roi.ROI_info[i].sHeight = in->af_roi.ROI_info[i].sHeight;
		ISP_LOGV("af set roi_%d (%d %d %d %d)", i, cxt->af_roi.ROI_info[i].Center_X, cxt->af_roi.ROI_info[i].Center_Y,
			cxt->af_roi.ROI_info[i].sWidth, cxt->af_roi.ROI_info[i].sHeight);
	}
	cxt->af_roi.ROI_Size = in->af_roi.ROI_Size;
	cxt->af_type = MULTIZONE;

	return ret;
}

static cmr_s32 pdafsprd_adpt_set_afmfv(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	if(NULL != in->af_addr && 0 != in->af_addr_len){
		if(NULL == cxt->af_addr){
			cxt->af_addr = (cmr_u8*)malloc(in->af_addr_len);
			if(NULL != cxt->af_addr){
				cxt->af_addr_len = in->af_addr_len;
				ISP_LOGI("the af_addr_len is %d", in->af_addr_len);
			}
		}

		if(NULL != cxt->af_addr && cxt->af_addr_len == in->af_addr_len){
			memcpy(cxt->af_addr, in->af_addr, in->af_addr_len);
		}
	}

	return ret;
}

static cmr_s32 pdafsprd_adpt_set_ot_switch(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	ISP_LOGI("ot switch %d", in->ot_switch);
	cxt->ot_switch = in->ot_switch;
	if (1 == cxt->ot_switch) {
		cxt->af_type = ACTIVE;
	} else {
		cxt->af_type = AF_MODE_NORMAL == cxt->af_mode ? ACTIVE : PASSIVE;
	}

	return ret;
}

static cmr_s32 pdafsprd_adpt_set_ot_info(cmr_handle adpt_handle, struct pdaf_ctrl_param_in *in)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct sprd_pdaf_context *cxt = (struct sprd_pdaf_context *)adpt_handle;

	ISP_CHECK_HANDLE_VALID(adpt_handle);

	ISP_LOGV("ot coor (%d %d %d %d)", in->ot_info.centorX, in->ot_info.centorY, in->ot_info.sizeX, in->ot_info.sizeY);
	cxt->ot_info.centorX = in->ot_info.centorX;
	cxt->ot_info.centorY = in->ot_info.centorY;

	return ret;
}

static cmr_s32 sprd_pdaf_adpt_ioctrl(cmr_handle adpt_handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_s32 ret = ISP_SUCCESS;
	struct pdaf_ctrl_param_in *in_ptr = (struct pdaf_ctrl_param_in *)in;

	ISP_CHECK_HANDLE_VALID(adpt_handle);
	ISP_LOGV("cmd = %d", cmd);

	switch (cmd) {
	case PDAF_CTRL_CMD_SET_PARAM:
		ret = pdafsprd_adpt_set_param(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_GET_BUSY:
		ret = pdafsprd_adpt_get_busy(adpt_handle, out);
		break;
	case PDAF_CTRL_CMD_DISABLE_PDAF:
		ret = pdafsprd_adpt_disable_pdaf(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_COOR:
		ret = pdafsprd_adpt_set_coor(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_MODE:
		ret = pdafsprd_adpt_set_mode(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_AFMFV:
		ret = pdafsprd_adpt_set_afmfv(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_MULTIZONE:
		pdafsprd_adpt_set_multizone(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_OTSWITCH:
		pdafsprd_adpt_set_ot_switch(adpt_handle, in_ptr);
		break;
	case PDAF_CTRL_CMD_SET_OTINFO:
		pdafsprd_adpt_set_ot_info(adpt_handle, in_ptr);
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
