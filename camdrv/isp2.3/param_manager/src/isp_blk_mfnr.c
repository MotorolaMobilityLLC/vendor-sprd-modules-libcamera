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
#define LOG_TAG "isp_blk_mfnr"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_mfnr_convert_param(
	void *dst_mfnr_param, cmr_u32 strength_level,
	cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_mfnr_param *dst_ptr =
			(struct isp_mfnr_param *)dst_mfnr_param;
	struct sensor_mfnr_level *mfnr_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		mfnr_param = (struct sensor_mfnr_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		mfnr_param = (struct sensor_mfnr_level *)((cmr_u8 *) dst_ptr->param_ptr +
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_mfnr_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (mfnr_param != NULL) {
		dst_ptr->cur_data = &mfnr_param[strength_level];
	}
	return rtn;
}

cmr_s32 _pm_mfnr_init(void *dst_mfnr_param, void *src_mfnr_param, void *param1, void *param_ptr2)
{
	cmr_s32 rtn = ISP_SUCCESS;

	struct isp_pm_nr_header_param *src_ptr =
				(struct isp_pm_nr_header_param *)src_mfnr_param;
	struct isp_mfnr_param *dst_ptr =
				(struct isp_mfnr_param *)dst_mfnr_param;
	struct isp_pm_block_header *mfnr_header_ptr =
				(struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_mfnr_convert_param(dst_ptr, dst_ptr->cur_level,
							ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm mfnr param  !");
		return rtn;
	}
	mfnr_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_mfnr_set_param(void *mfnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr =
					(struct isp_pm_block_header *)param_ptr1;
	struct isp_mfnr_param *dst_ptr =
					(struct isp_mfnr_param *)mfnr_param;

	switch (cmd) {
	case ISP_PM_BLK_3D_NR_BYPASS:
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_3D_NR_STRENGTH_LEVEL:
		dst_ptr->cur_level = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result =
						(struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 level = 0;

			val_range.min = 0;
			val_range.max = 255;

			if (0 == block_result->update) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			rtn = _pm_check_smart_param(block_result, &val_range,
											1, ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			level = (cmr_u32) block_result->component[0].fix_data[0];

			if (level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_MFNR_T] ||
											block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_MFNR_T] = 0;

				rtn = _pm_mfnr_convert_param(dst_ptr, dst_ptr->cur_level,
								block_result->mode_flag, block_result->scene_flag);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm mfnr param !");
					return rtn;
				}
			}
		}
		break;

	default:

		break;
	}

	return rtn;
}

cmr_s32 _pm_mfnr_get_param(void *mfnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_mfnr_param *mfnr_ptr =
				(struct isp_mfnr_param *)mfnr_param;
	struct isp_pm_param_data *param_data_ptr =
				(struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_MFNR:
		param_data_ptr->data_ptr = mfnr_ptr->cur_data;
		param_data_ptr->data_size = sizeof(struct sensor_mfnr_level);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}