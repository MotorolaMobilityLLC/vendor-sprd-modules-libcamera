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
#define LOG_TAG "isp_blk_ynrs"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_ynrs_convert_param(void *dst_ynr2_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_ynrs_param *dst_ptr = (struct isp_ynrs_param *)dst_ynr2_param;
	struct sensor_ynrs_level *ynrs_param = PNULL;
	cmr_s32 i = 0;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		ynrs_param = (struct sensor_ynrs_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		ynrs_param = (struct sensor_ynrs_level *)((cmr_u8 *) dst_ptr->param_ptr + total_offset_units * dst_ptr->level_num * sizeof(struct sensor_ynrs_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if(ynrs_param != PNULL){
		for(i = 0; i < 5; i++){
			dst_ptr->cur.gf_rnr_ratio[i] = ynrs_param[strength_level].gf_rnr_ratio[i];
			dst_ptr->cur.gf_addback_enable[i] = ynrs_param[strength_level].gf_addback_enable[i];
			dst_ptr->cur.gf_addback_ratio[i] = ynrs_param[strength_level].gf_addback_ratio[i];
			dst_ptr->cur.gf_addback_clip[i] = ynrs_param[strength_level].gf_addback_clip[i];
			dst_ptr->cur.gf_enable[i] = ynrs_param[strength_level].gf_enable[i];
			dst_ptr->cur.gf_radius[i] = ynrs_param[strength_level].gf_radius[i];
			dst_ptr->cur.gf_rnr_offset[i] = ynrs_param[strength_level].gf_rnr_offset[i];
			dst_ptr->cur.gf_epsilon[i][0] = ynrs_param[strength_level].gf_epsilon[i][0];
			dst_ptr->cur.gf_epsilon[i][1] = ynrs_param[strength_level].gf_epsilon[i][1];
			dst_ptr->cur.gf_epsilon[i][2] = ynrs_param[strength_level].gf_epsilon[i][2];
		}
		dst_ptr->cur.lumi_thresh[0] = ynrs_param[strength_level].lumi_thresh[0];
		dst_ptr->cur.lumi_thresh[1] = ynrs_param[strength_level].lumi_thresh[1];
		dst_ptr->cur.Radius = ynrs_param[strength_level].Radius;
		dst_ptr->cur.imgCenterX = ynrs_param[strength_level].imgCenterX;
		dst_ptr->cur.imgCenterY = ynrs_param[strength_level].imgCenterY;
		dst_ptr->cur.bypass = ynrs_param[strength_level].bypass;
		ISP_LOGV("Radius = 0x%x imgCenterX = 0x%x imgCenterY = 0x%x bypass = 0x%x lumi_thresh[0] = 0x%x gf_rnr_ratio[0]= 0x%x \n",
			dst_ptr->cur.Radius, dst_ptr->cur.imgCenterX, dst_ptr->cur.imgCenterY, dst_ptr->cur.bypass, dst_ptr->cur.lumi_thresh[0], dst_ptr->cur.gf_rnr_ratio[0]);
	}

	return rtn;
}

cmr_s32 _pm_ynrs_init(void *dst_ynrs_param, void *src_ynrs_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_ynrs_param;
	struct isp_ynrs_param *dst_ptr = (struct isp_ynrs_param *)dst_ynrs_param;
	UNUSED(param2);

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_ynrs_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm ynr2 param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_ynrs_set_param(void *ynrs_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_ynrs_param *dst_ptr = (struct isp_ynrs_param *)ynrs_param;

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

			if (level != dst_ptr->cur_level || nr_tool_flag[18] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[18] = 0;

				rtn = _pm_ynrs_convert_param(dst_ptr, dst_ptr->cur_level, header_ptr->mode_id, block_result->scene_flag);
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

cmr_s32 _pm_ynrs_get_param(void *ynrs_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynrs_param *ynrs_ptr = (struct isp_ynrs_param *)ynrs_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_YNRS;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &ynrs_ptr->cur;
		param_data_ptr->data_size = sizeof(ynrs_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
