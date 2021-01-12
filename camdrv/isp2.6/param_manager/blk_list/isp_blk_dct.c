/*
 * Copyright (C) 2018 The Android Open Source Project
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
#define LOG_TAG "isp_blk_dct"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_dct_convert_param(void *dst_dct_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_dct_param *dst_ptr = (struct isp_dct_param *)dst_dct_param;
	struct sensor_dct_level *dct_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		dct_param = (struct sensor_dct_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		dct_param = (struct sensor_dct_level *)((cmr_u8 *) dst_ptr->param_ptr +
					total_offset_units * dst_ptr->level_num * sizeof(struct sensor_dct_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (dct_param != NULL) {
		dst_ptr->cur.bypass = dct_param[strength_level].bypass;

		dst_ptr->cur.shrink_en = dct_param[strength_level].shrink_en;
		dst_ptr->cur.addback_en = dct_param[strength_level].addback_en;
		dst_ptr->cur.addback_ratio = dct_param[strength_level].addback_ratio;
		dst_ptr->cur.addback_clip = dct_param[strength_level].addback_clip;

		dst_ptr->cur.coef_thresh0 = dct_param[strength_level].dct_coef.coef_thresh[0];
		dst_ptr->cur.coef_thresh1 = dct_param[strength_level].dct_coef.coef_thresh[1];
		dst_ptr->cur.coef_thresh2 = dct_param[strength_level].dct_coef.coef_thresh[2];
		dst_ptr->cur.coef_thresh3 = dct_param[strength_level].dct_coef.coef_thresh[3];
		dst_ptr->cur.coef_ratio0 = dct_param[strength_level].dct_coef.coef_ratio[0];
		dst_ptr->cur.coef_ratio1 = dct_param[strength_level].dct_coef.coef_ratio[1];
		dst_ptr->cur.coef_ratio2 = dct_param[strength_level].dct_coef.coef_ratio[2];

		dst_ptr->cur.lnr_en = dct_param[strength_level].dct_luma.lnr_en;
		dst_ptr->cur.luma_thresh0= dct_param[strength_level].dct_luma.luma_thresh[0];
		dst_ptr->cur.luma_thresh1= dct_param[strength_level].dct_luma.luma_thresh[1];
		dst_ptr->cur.luma_ratio0 = dct_param[strength_level].dct_luma.luma_ratio[0];
		dst_ptr->cur.luma_ratio1 = dct_param[strength_level].dct_luma.luma_ratio[1];

		dst_ptr->cur.fnr_en = dct_param[strength_level].dct_fnr.fnr_en;
		dst_ptr->cur.flat_th = dct_param[strength_level].dct_fnr.flat_th;
		dst_ptr->cur.fnr_ratio0 = dct_param[strength_level].dct_fnr.fnr_ratio[0];
		dst_ptr->cur.fnr_ratio1 = dct_param[strength_level].dct_fnr.fnr_ratio[1];
		dst_ptr->cur.fnr_thresh0 = dct_param[strength_level].dct_fnr.fnr_thresh[0];
		dst_ptr->cur.fnr_thresh1 = dct_param[strength_level].dct_fnr.fnr_thresh[1];

		dst_ptr->cur.rnr_en = dct_param[strength_level].dct_rnr.rnr_en;
		dst_ptr->cur.rnr_radius = dct_param[strength_level].dct_rnr.rnr_radius;
		dst_ptr->cur.rnr_radius_factor = dct_param[strength_level].dct_rnr.rnr_radius_factor;
		dst_ptr->cur.rnr_radius_base = dct_param[strength_level].dct_rnr.rnr_radius_base;
		dst_ptr->cur.rnr_imgCenterX = dct_param[strength_level].dct_rnr.rnr_imgCenterX;
		dst_ptr->cur.rnr_imgCenterY= dct_param[strength_level].dct_rnr.rnr_imgCenterY;
		dst_ptr->cur.rnr_step = dct_param[strength_level].dct_rnr.rnr_step;
		dst_ptr->cur.rnr_ratio0 = dct_param[strength_level].dct_rnr.rnr_ratio[0];
		dst_ptr->cur.rnr_ratio1 = dct_param[strength_level].dct_rnr.rnr_ratio[1];

		dst_ptr->cur.blend_en = dct_param[strength_level].dct_blend.blend_en;
		dst_ptr->cur.blend_radius = dct_param[strength_level].dct_blend.blend_radius;
		dst_ptr->cur.blend_weight = dct_param[strength_level].dct_blend.blend_weight;
		dst_ptr->cur.blend_epsilon = dct_param[strength_level].dct_blend.blend_epsilon;
		dst_ptr->cur.blend_thresh0 = dct_param[strength_level].dct_blend.blend_thresh[0];
		dst_ptr->cur.blend_thresh1 = dct_param[strength_level].dct_blend.blend_thresh[0];
		dst_ptr->cur.blend_ratio0 = dct_param[strength_level].dct_blend.blend_ratio[0];
		dst_ptr->cur.blend_ratio1 = dct_param[strength_level].dct_blend.blend_ratio[1];

		dst_ptr->cur.direction_smooth_en = dct_param[strength_level].dct_direction.direction_smooth_en;
		dst_ptr->cur.direction_mode = dct_param[strength_level].dct_direction.direction_mode;
		dst_ptr->cur.direction_freq_hop_control_en = dct_param[strength_level].dct_direction.direction_freq_hop_control_en;
		dst_ptr->cur.direction_freq_hop_thresh = dct_param[strength_level].dct_direction.direction_freq_hop_thresh;
		dst_ptr->cur.direction_freq_hop_total_num_thresh = dct_param[strength_level].dct_direction.direction_freq_hop_total_num_thresh;
		dst_ptr->cur.direction_thresh_diff = dct_param[strength_level].dct_direction.direction_thresh_diff;
		dst_ptr->cur.direction_thresh_min = dct_param[strength_level].dct_direction.direction_thresh_min;
		dst_ptr->cur.direction_hop_thresh_diff = dct_param[strength_level].dct_direction.direction_hop_thresh_diff;
		dst_ptr->cur.direction_hop_thresh_min = dct_param[strength_level].dct_direction.direction_hop_thresh_min;
	}

	return rtn;
}

cmr_s32 _pm_dct_init(void *dst_dct_param, void *src_dct_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_dct_param *dst_ptr = (struct isp_dct_param *)dst_dct_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_dct_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_dct_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass = 0;
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm dct param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_dct_set_param(void * dct_param, cmr_u32 cmd, void * param_ptr0, void * param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_dct_param *dst_ptr = (struct isp_dct_param *)dct_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 cur_level = 0;

			if (!block_result->update || header_ptr->bypass) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}
			val_range.min = 0;
			val_range.max = 255;

			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			cur_level = (cmr_u32) block_result->component[0].fix_data[0];

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_DCT_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_DCT_T] = 0;

				rtn = _pm_dct_convert_param(dst_ptr, dst_ptr->cur_level, header_ptr->mode_id, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;

				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm dct param!");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART: cmd=%d, update=%d, dct_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_dct_get_param(void * dct_param, cmr_u32 cmd, void * rtn_param0, void * rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_dct_param *dct_ptr = (struct isp_dct_param *)dct_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &dct_ptr->cur;
		param_data_ptr->data_size = sizeof(dct_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}

