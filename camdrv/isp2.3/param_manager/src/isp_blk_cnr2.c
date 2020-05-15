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
#define LOG_TAG "isp_blk_cnr2"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_cnr2_convert_param(void *dst_cnr2_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_cnr2_param *dst_ptr = (struct isp_cnr2_param *)dst_cnr2_param;
	struct sensor_cnr_level *cnr2_param = PNULL;
	cmr_s32 i = 0, j = 0;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		cnr2_param = (struct sensor_cnr_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		cnr2_param = (struct sensor_cnr_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_cnr_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (cnr2_param != NULL) {
		for (i = 0; i < 4; i++) {
			dst_ptr->cur.filter_en[i] = cnr2_param[strength_level].filter_en[i];
			dst_ptr->cur.rangTh[i][0] = cnr2_param[strength_level].rangTh[i][0];
			dst_ptr->cur.rangTh[i][1] = cnr2_param[strength_level].rangTh[i][1];
			for (j = 0; j < 9; j++) {
				dst_ptr->cur.weight[i][0].distWeight[j] = cnr2_param[strength_level].weight[i][0].distWeight[j];
				dst_ptr->cur.weight[i][1].distWeight[j] = cnr2_param[strength_level].weight[i][1].distWeight[j];
			}
			for (j = 0; j < 128; j++) {
				dst_ptr->cur.weight[i][0].rangWeight[j] = cnr2_param[strength_level].weight[i][0].rangWeight[j];
				dst_ptr->cur.weight[i][1].rangWeight[j] = cnr2_param[strength_level].weight[i][1].rangWeight[j];
			}
		}
		dst_ptr->level_info.level_enable = cnr2_param[strength_level].weight[0][0].level_enable;
		dst_ptr->level_info.low_ct_thrd = cnr2_param[strength_level].weight[0][0].low_ct_thrd;
	}

	return rtn;
}

cmr_s32 _pm_cnr2_init(void *dst_cnr2_param, void *src_cnr2_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_cnr2_param;
	struct isp_cnr2_param *dst_ptr = (struct isp_cnr2_param *)dst_cnr2_param;
	UNUSED(param2);

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_cnr2_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	if (header_ptr->bypass) {
		dst_ptr->level_info.level_enable = 0;
		dst_ptr->level_info.low_ct_thrd = 0;
	}
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm cnr2 param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_cnr2_set_param(void *cnr2_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_cnr2_param *dst_ptr = (struct isp_cnr2_param *)cnr2_param;

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

			if (level != dst_ptr->cur_level
				|| nr_tool_flag[ISP_BLK_CNR2_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_CNR2_T] = 0;

				rtn = _pm_cnr2_convert_param(dst_ptr, dst_ptr->cur_level, block_result->mode_flag, block_result->scene_flag);
				if (header_ptr->bypass) {
					dst_ptr->level_info.level_enable = 0;
					dst_ptr->level_info.low_ct_thrd = 0;
				}
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm edge param !");
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

cmr_s32 _pm_cnr2_get_param(void *cnr2_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_cnr2_param *cnr2_ptr = (struct isp_cnr2_param *)cnr2_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_CNR2;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cnr2_ptr->cur;
		param_data_ptr->data_size = sizeof(cnr2_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	case ISP_PM_BLK_CNR2_LEVEL_INFO:
		param_data_ptr->data_ptr = &cnr2_ptr->level_info;
		param_data_ptr->data_size = sizeof(cnr2_ptr->level_info);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}

