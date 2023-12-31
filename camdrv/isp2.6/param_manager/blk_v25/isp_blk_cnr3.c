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

#define LOG_TAG "isp_blk_cnr3"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_cnr3_convert_param(void *dst_cnr3_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_cnr3_param *dst_ptr = (struct isp_cnr3_param *)dst_cnr3_param;
	struct sensor_cnr3_level *cnr3_param = PNULL;
	cmr_s32 i = 0, j = 0;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		cnr3_param = (struct sensor_cnr3_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		cnr3_param = (struct sensor_cnr3_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_cnr3_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (cnr3_param != NULL) {
		dst_ptr->cur.baseRadius = cnr3_param[strength_level].radius_base;
		for (i = 0; i < 5; i++) {
			dst_ptr->cur.param_layer[i].lowpass_filter_en = cnr3_param[strength_level].param_layer[i].lowpass_filter_en;
			dst_ptr->cur.param_layer[i].denoise_radial_en = cnr3_param[strength_level].param_layer[i].denoise_radial_en;
			dst_ptr->cur.param_layer[i].imgCenterX = cnr3_param[strength_level].param_layer[i].imgCenterX;
			dst_ptr->cur.param_layer[i].imgCenterY = cnr3_param[strength_level].param_layer[i].imgCenterY;
			dst_ptr->cur.param_layer[i].slope = cnr3_param[strength_level].param_layer[i].slope;
			dst_ptr->cur.param_layer[i].baseRadius = cnr3_param[strength_level].param_layer[i].baseRadius_factor;
			dst_ptr->cur.param_layer[i].minRatio = cnr3_param[strength_level].param_layer[i].minRatio;
			for (j = 0; j < 3; j++) {
				dst_ptr->cur.param_layer[i].order[j] = cnr3_param[strength_level].param_layer[i].order[j];
				dst_ptr->cur.param_layer[i].sigma[j] = cnr3_param[strength_level].param_layer[i].sigma[j];
			}
			for (j = 0; j < 2; j++) {
				dst_ptr->cur.param_layer[i].luma_th[j] = cnr3_param[strength_level].param_layer[i].luma_th[j];
			}
		}
		dst_ptr->level_info.level_enable = cnr3_param[strength_level].level_enable;
		dst_ptr->level_info.low_ct_thrd = cnr3_param[strength_level].low_ct_thrd;
		ISP_LOGV("ISP_SMART_NR: dst_ptr->level_info.level_enable=%d,dst_ptr->level_info.low_ct_thrd %d", dst_ptr->level_info.level_enable,dst_ptr->level_info.low_ct_thrd);
	}

	return rtn;
}

cmr_s32 _pm_cnr3_init(void *dst_cnr3_param, void *src_cnr3_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_cnr3_param;
	struct isp_cnr3_param *dst_ptr = (struct isp_cnr3_param *)dst_cnr3_param;
	UNUSED(param2);

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_cnr3_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass = 0;
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm cnr3 param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_cnr3_set_param(void *cnr3_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_cnr3_param *dst_ptr = (struct isp_cnr3_param *)cnr3_param;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 level = 0;

			val_range.min = 0;
			val_range.max = 255;

			if (0 == block_result->update) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			level = (cmr_u32) block_result->component[0].fix_data[0];

			if (level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_CNR3_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_CNR3_T] = 0;

				rtn = _pm_cnr3_convert_param(dst_ptr, dst_ptr->cur_level, header_ptr->mode_id, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm cnr3 param !");
					return rtn;
				}
			}
		}
		break;

	default:
		break;
	}

	ISP_LOGV("ISP_SMART_NR: cmd=%d, update=%d, ee_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}

cmr_s32 _pm_cnr3_get_param(void *cnr3_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cnr3_param *cnr3_ptr = (struct isp_cnr3_param *)cnr3_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_CNR3;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cnr3_ptr->cur;
		param_data_ptr->data_size = sizeof(cnr3_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	case ISP_PM_BLK_CNR3_LEVEL_INFO:
		param_data_ptr->data_ptr = &cnr3_ptr->level_info;
		param_data_ptr->data_size = sizeof(cnr3_ptr->level_info);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
