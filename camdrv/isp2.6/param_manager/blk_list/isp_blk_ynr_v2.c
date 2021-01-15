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
#define LOG_TAG "isp_blk_ynr"
#include "isp_blocks_cfg.h"

static cmr_u32 _pm_ynr_convert_param(void *dst_param, cmr_u32 strength_level,
	cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)dst_param;
	struct sensor_ynr_level *ynr_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		ynr_param = (struct sensor_ynr_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		ynr_param = (struct sensor_ynr_level *)((cmr_u8 *) dst_ptr->param_ptr +
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_ynr_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (ynr_param != NULL) {
		dst_ptr->cur_v3.bypass = ynr_param[strength_level].bypass;
		dst_ptr->cur_v3.radius = ynr_param[strength_level].radius;
		dst_ptr->cur_v3.radius_base = ynr_param[strength_level].radius_base;
		dst_ptr->cur_v3.radius_factor = ynr_param[strength_level].radius_factor;
		dst_ptr->cur_v3.imgCenterX = ynr_param[strength_level].imgCenterX;
		dst_ptr->cur_v3.imgCenterY = ynr_param[strength_level].imgCenterY;

		dst_ptr->cur_v3.l1_layer_gf_enable = ynr_param[strength_level].ynr_layer[0].gf_enable;
		dst_ptr->cur_v3.l1_layer_gf_radius = ynr_param[strength_level].ynr_layer[0].gf_radius;
		dst_ptr->cur_v3.l1_layer_gf_rnr_offset = ynr_param[strength_level].ynr_layer[0].gf_rnr_offset;
		dst_ptr->cur_v3.l1_layer_gf_rnr_ratio = ynr_param[strength_level].ynr_layer[0].gf_rnr_ratio;
		dst_ptr->cur_v3.l1_layer_gf_addback_enable = ynr_param[strength_level].ynr_layer[0].gf_addback_enable;
		dst_ptr->cur_v3.l1_layer_gf_addback_ratio = ynr_param[strength_level].ynr_layer[0].gf_addback_ratio;
		dst_ptr->cur_v3.l1_layer_gf_addback_clip = ynr_param[strength_level].ynr_layer[0].gf_addback_clip;
		dst_ptr->cur_v3.l1_layer_lum_thresh1 = ynr_param[strength_level].ynr_layer[0].lum_thresh[0];
		dst_ptr->cur_v3.l1_layer_lum_thresh2 = ynr_param[strength_level].ynr_layer[0].lum_thresh[1];
		dst_ptr->cur_v3.l1_layer_epsilon_gf_epsilon_low = ynr_param[strength_level].ynr_layer[0].ynr_epsilon.gf_epsilon_low;
		dst_ptr->cur_v3.l1_layer_epsilon_gf_epsilon_mid = ynr_param[strength_level].ynr_layer[0].ynr_epsilon.gf_epsilon_mid;
		dst_ptr->cur_v3.l1_layer_epsilon_gf_epsilon_high = ynr_param[strength_level].ynr_layer[0].ynr_epsilon.gf_epsilon_high;

		dst_ptr->cur_v3.l2_layer_gf_enable = ynr_param[strength_level].ynr_layer[1].gf_enable;
		dst_ptr->cur_v3.l2_layer_gf_radius = ynr_param[strength_level].ynr_layer[1].gf_radius;
		dst_ptr->cur_v3.l2_layer_gf_rnr_offset = ynr_param[strength_level].ynr_layer[1].gf_rnr_offset;
		dst_ptr->cur_v3.l2_layer_gf_rnr_ratio = ynr_param[strength_level].ynr_layer[1].gf_rnr_ratio;
		dst_ptr->cur_v3.l2_layer_gf_addback_enable = ynr_param[strength_level].ynr_layer[1].gf_addback_enable;
		dst_ptr->cur_v3.l2_layer_gf_addback_ratio = ynr_param[strength_level].ynr_layer[1].gf_addback_ratio;
		dst_ptr->cur_v3.l2_layer_gf_addback_clip = ynr_param[strength_level].ynr_layer[1].gf_addback_clip;
		dst_ptr->cur_v3.l2_layer_lum_thresh1 = ynr_param[strength_level].ynr_layer[1].lum_thresh[0];
		dst_ptr->cur_v3.l2_layer_lum_thresh2 = ynr_param[strength_level].ynr_layer[1].lum_thresh[1];
		dst_ptr->cur_v3.l2_layer_epsilon_gf_epsilon_low = ynr_param[strength_level].ynr_layer[1].ynr_epsilon.gf_epsilon_low;
		dst_ptr->cur_v3.l2_layer_epsilon_gf_epsilon_mid = ynr_param[strength_level].ynr_layer[1].ynr_epsilon.gf_epsilon_mid;
		dst_ptr->cur_v3.l2_layer_epsilon_gf_epsilon_high = ynr_param[strength_level].ynr_layer[1].ynr_epsilon.gf_epsilon_high;

		dst_ptr->cur_v3.l3_layer_gf_enable = ynr_param[strength_level].ynr_layer[2].gf_enable;
		dst_ptr->cur_v3.l3_layer_gf_radius = ynr_param[strength_level].ynr_layer[2].gf_radius;
		dst_ptr->cur_v3.l3_layer_gf_rnr_offset = ynr_param[strength_level].ynr_layer[2].gf_rnr_offset;
		dst_ptr->cur_v3.l3_layer_gf_rnr_ratio = ynr_param[strength_level].ynr_layer[2].gf_rnr_ratio;
		dst_ptr->cur_v3.l3_layer_gf_addback_enable = ynr_param[strength_level].ynr_layer[2].gf_addback_enable;
		dst_ptr->cur_v3.l3_layer_gf_addback_ratio = ynr_param[strength_level].ynr_layer[2].gf_addback_ratio;
		dst_ptr->cur_v3.l3_layer_gf_addback_clip = ynr_param[strength_level].ynr_layer[2].gf_addback_clip;
		dst_ptr->cur_v3.l3_layer_lum_thresh1 = ynr_param[strength_level].ynr_layer[2].lum_thresh[0];
		dst_ptr->cur_v3.l3_layer_lum_thresh2 = ynr_param[strength_level].ynr_layer[2].lum_thresh[1];
		dst_ptr->cur_v3.l3_layer_epsilon_gf_epsilon_low = ynr_param[strength_level].ynr_layer[2].ynr_epsilon.gf_epsilon_low;
		dst_ptr->cur_v3.l3_layer_epsilon_gf_epsilon_mid = ynr_param[strength_level].ynr_layer[2].ynr_epsilon.gf_epsilon_mid;
		dst_ptr->cur_v3.l3_layer_epsilon_gf_epsilon_high = ynr_param[strength_level].ynr_layer[2].ynr_epsilon.gf_epsilon_high;

		dst_ptr->cur_v3.l4_layer_gf_enable = ynr_param[strength_level].ynr_layer[3].gf_enable;
		dst_ptr->cur_v3.l4_layer_gf_radius = ynr_param[strength_level].ynr_layer[3].gf_radius;
		dst_ptr->cur_v3.l4_layer_gf_rnr_offset = ynr_param[strength_level].ynr_layer[3].gf_rnr_offset;
		dst_ptr->cur_v3.l4_layer_gf_rnr_ratio = ynr_param[strength_level].ynr_layer[3].gf_rnr_ratio;
		dst_ptr->cur_v3.l4_layer_gf_addback_enable = ynr_param[strength_level].ynr_layer[3].gf_addback_enable;
		dst_ptr->cur_v3.l4_layer_gf_addback_ratio = ynr_param[strength_level].ynr_layer[3].gf_addback_ratio;
		dst_ptr->cur_v3.l4_layer_gf_addback_clip = ynr_param[strength_level].ynr_layer[3].gf_addback_clip;
		dst_ptr->cur_v3.l4_layer_lum_thresh1 = ynr_param[strength_level].ynr_layer[3].lum_thresh[0];
		dst_ptr->cur_v3.l4_layer_lum_thresh2 = ynr_param[strength_level].ynr_layer[3].lum_thresh[1];
		dst_ptr->cur_v3.l4_layer_epsilon_gf_epsilon_low = ynr_param[strength_level].ynr_layer[3].ynr_epsilon.gf_epsilon_low;
		dst_ptr->cur_v3.l4_layer_epsilon_gf_epsilon_mid = ynr_param[strength_level].ynr_layer[3].ynr_epsilon.gf_epsilon_mid;
		dst_ptr->cur_v3.l4_layer_epsilon_gf_epsilon_high = ynr_param[strength_level].ynr_layer[3].ynr_epsilon.gf_epsilon_high;

		dst_ptr->cur_v3.l5_layer_gf_enable = ynr_param[strength_level].ynr_layer[4].gf_enable;
		dst_ptr->cur_v3.l5_layer_gf_radius = ynr_param[strength_level].ynr_layer[4].gf_radius;
		dst_ptr->cur_v3.l5_layer_gf_rnr_offset = ynr_param[strength_level].ynr_layer[4].gf_rnr_offset;
		dst_ptr->cur_v3.l5_layer_gf_rnr_ratio = ynr_param[strength_level].ynr_layer[4].gf_rnr_ratio;
		dst_ptr->cur_v3.l5_layer_gf_addback_enable = ynr_param[strength_level].ynr_layer[4].gf_addback_enable;
		dst_ptr->cur_v3.l5_layer_gf_addback_ratio = ynr_param[strength_level].ynr_layer[4].gf_addback_ratio;
		dst_ptr->cur_v3.l5_layer_gf_addback_clip = ynr_param[strength_level].ynr_layer[4].gf_addback_clip;
		dst_ptr->cur_v3.l5_layer_lum_thresh1 = ynr_param[strength_level].ynr_layer[4].lum_thresh[0];
		dst_ptr->cur_v3.l5_layer_lum_thresh2 = ynr_param[strength_level].ynr_layer[4].lum_thresh[1];
		dst_ptr->cur_v3.l5_layer_epsilon_gf_epsilon_low = ynr_param[strength_level].ynr_layer[4].ynr_epsilon.gf_epsilon_low;
		dst_ptr->cur_v3.l5_layer_epsilon_gf_epsilon_mid = ynr_param[strength_level].ynr_layer[4].ynr_epsilon.gf_epsilon_mid;
		dst_ptr->cur_v3.l5_layer_epsilon_gf_epsilon_high = ynr_param[strength_level].ynr_layer[4].ynr_epsilon.gf_epsilon_high;
	}
	return rtn;
}

cmr_s32 _pm_ynr_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)dst_ynr_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_ynr_param;
	struct isp_pm_block_header *ynr_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur_v3.bypass = ynr_header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;
	if (!ynr_header_ptr->bypass)
		rtn = _pm_ynr_convert_param(dst_ptr, dst_ptr->cur_level, ynr_header_ptr->mode_id, ISP_SCENEMODE_AUTO);
	dst_ptr->cur_v3.bypass |= ynr_header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm ynr param!");
		return rtn;
	}

	ynr_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_ynr_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)ynr_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_YNR_BYPASS:
		dst_ptr->cur_v3.bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

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
			if (cur_level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_YNR_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_YNR_T] = 0;

				rtn = _pm_ynr_convert_param(dst_ptr, cur_level, header_ptr->mode_id, block_result->scene_flag);
				dst_ptr->cur_v3.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm ynr param!");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART_NR: cmd = %d, is_update = %d, ynr_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);
		}
		break;

	default:

		break;
	}

	return rtn;
}

cmr_s32 _pm_ynr_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *ynr_ptr = (struct isp_ynr_param *)ynr_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&ynr_ptr->cur_v3;
		param_data_ptr->data_size = sizeof(ynr_ptr->cur_v3);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_YNR_BYPASS:
		param_data_ptr->data_ptr = (void *)&ynr_ptr->cur_v3.bypass;
		param_data_ptr->data_size = sizeof(ynr_ptr->cur_v3.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
