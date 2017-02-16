/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "isp_blocks_cfg.h"


isp_u32 _pm_pdaf_correct_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag, isp_u32 scene_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	isp_u32 total_offset_units = 0;
	struct isp_pdaf_correction_param *dst_ptr = (struct isp_pdaf_correction_param*)dst_param;
	struct sensor_pdaf_correction_level *pdaf_correct_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		pdaf_correct_param = (struct sensor_pdaf_correction_level *)(dst_ptr->param_ptr);
	} else {
		isp_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (isp_u32 *)dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		pdaf_correct_param = (struct sensor_pdaf_correction_level *)((isp_u8 *)dst_ptr->param_ptr +
			total_offset_units * dst_ptr->level_num * sizeof(struct sensor_pdaf_correction_level));
	}

	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (pdaf_correct_param != NULL) {
		dst_ptr->cur.phase_flat_smoother = pdaf_correct_param[strength_level].flat_smt_index;
		dst_ptr->cur.phase_txt_smooth  = pdaf_correct_param[strength_level].txt_smt_index;
		dst_ptr->cur.corrector_bypass    = pdaf_correct_param[strength_level].corrector_bypass;
		for(i = 0; i < 3; i++) {
			dst_ptr->cur.hot_pixel_th[i] = pdaf_correct_param[strength_level].hot_pixel_th[i];
			dst_ptr->cur.dead_pixel_th[i]= pdaf_correct_param[strength_level].dead_pixel_th[i];
		}
		dst_ptr->cur.flat_th = pdaf_correct_param[strength_level].flat_th;
		dst_ptr->cur.edge_ratio_hv_rd = pdaf_correct_param[strength_level].pdaf_edgeRatio.ee_ratio_hv_rd;
		dst_ptr->cur.edge_ratio_hv = pdaf_correct_param[strength_level].pdaf_edgeRatio.ee_ratio_hv;
		dst_ptr->cur.edge_ratio_rd = pdaf_correct_param[strength_level].pdaf_edgeRatio.ee_ratio_rd;
		dst_ptr->cur.phase_map_corr_en = pdaf_correct_param[strength_level].phase_map_corr_eb;
		dst_ptr->cur.grid_mode = pdaf_correct_param[strength_level].pdaf_grid;
		dst_ptr->cur.phase_gfilter = pdaf_correct_param[strength_level].gfilter_flag;
		dst_ptr->cur.phase_left_addr= (int)pdaf_correct_param[strength_level].phase_l_gain_map;//??
		dst_ptr->cur.phase_right_addr= (int)pdaf_correct_param[strength_level].phase_r_gain_map;//??

		dst_ptr->cur.gain_upperbound.r = pdaf_correct_param[strength_level].pdaf_upperbound.r;
		dst_ptr->cur.gain_upperbound.gr = pdaf_correct_param[strength_level].pdaf_upperbound.gr;
		dst_ptr->cur.gain_upperbound.gb = pdaf_correct_param[strength_level].pdaf_upperbound.gb;
		dst_ptr->cur.gain_upperbound.b = pdaf_correct_param[strength_level].pdaf_upperbound.b;

		dst_ptr->cur.pdaf_blc.r = pdaf_correct_param[strength_level].pdaf_blacklevel.r;
		dst_ptr->cur.pdaf_blc.gr = pdaf_correct_param[strength_level].pdaf_blacklevel.gr;
		dst_ptr->cur.pdaf_blc.gb = pdaf_correct_param[strength_level].pdaf_blacklevel.gb;
		dst_ptr->cur.pdaf_blc.b = pdaf_correct_param[strength_level].pdaf_blacklevel.b;
		for(i = 0; i < 2; i++) {
			dst_ptr->cur.gain_ori_left[i] = pdaf_correct_param[strength_level].phase_l_gain_map[i];
			dst_ptr->cur.gain_ori_right[i] = pdaf_correct_param[strength_level].phase_r_gain_map[i];
		}
	}
	return rtn;
}

isp_s32 _pm_pdaf_correct_init(void *dst_pdaf_correct_param, void *src_pdaf_correct_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pdaf_correction_param *dst_ptr = (struct isp_pdaf_correction_param*)dst_pdaf_correct_param;
	struct sensor_nr_header_param *src_ptr = (struct sensor_nr_header_param*)src_pdaf_correct_param;
	struct isp_pm_block_header *header_ptr =(struct isp_pm_block_header*)param1;

	struct isp_size *img_size_ptr = (struct isp_size*)param2;

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	dst_ptr->cur.block_size.width = img_size_ptr->w;
	dst_ptr->cur.block_size.height = img_size_ptr->h;

	rtn = _pm_pdaf_correct_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_ppi_CONVERT_PARAM: error!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

isp_s32 _pm_pdaf_correct_set_param(void *pdaf_correct_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pdaf_correction_param *dst_ptr = (struct isp_pdaf_correction_param*)pdaf_correct_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;

	switch(cmd){
	case ISP_PM_BLK_PDAF_BYPASS:
		dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);
		header_ptr->is_update = ISP_ONE;
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0, 0};
		isp_u32 cur_level = 0;

		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];
		if(cur_level != dst_ptr->cur_level || nr_tool_flag[8] || block_result->mode_flag_changed){
			dst_ptr->cur_level = cur_level;
			header_ptr->is_update = ISP_ONE;
			nr_tool_flag[8] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_pdaf_correct_convert_param(dst_ptr,cur_level,block_result->mode_flag, block_result->scene_flag);
			dst_ptr->cur.bypass |= header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_PPI_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:

	break;
	}
	//ISP_LOGI("ISP_SMART: cmd = %d, is_update = %d, cur_level=%ld", cmd, pdaf_correct_header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}

isp_s32 _pm_pdaf_correct_get_param(void *pdaf_correct_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pdaf_correction_param* pdaf_correct_ptr = (struct isp_pdaf_correction_param*)pdaf_correct_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_PDAF_CORRECT;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&pdaf_correct_ptr->cur;
		param_data_ptr->data_size = sizeof(pdaf_correct_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_PDAF_BYPASS:
		param_data_ptr->data_ptr = (void*)&pdaf_correct_ptr->cur.bypass; // not sure
		param_data_ptr->data_size = sizeof(pdaf_correct_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

