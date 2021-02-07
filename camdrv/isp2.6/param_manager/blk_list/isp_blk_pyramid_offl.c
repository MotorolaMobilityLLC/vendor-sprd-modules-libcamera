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
#define LOG_TAG "isp_blk_pyramid_offl"
#include "isp_blocks_cfg.h"

cmr_u32 _pm_pyramid_offl_convert_param(void *dst_pyramid_offl_param,
		cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_pyramid_offl_param *dst_ptr = (struct isp_pyramid_offl_param *)dst_pyramid_offl_param;
	struct sensor_pyramid_offl_level *pyramid_offl_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		pyramid_offl_param = (struct sensor_pyramid_offl_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		pyramid_offl_param = (struct sensor_pyramid_offl_level *)((cmr_u8 *) dst_ptr->param_ptr +
					total_offset_units * dst_ptr->level_num * sizeof(struct sensor_pyramid_offl_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (pyramid_offl_param != NULL) {
		dst_ptr->cur.dec_offline_bypass = pyramid_offl_param[strength_level].dec_offline_bypass;
		dst_ptr->cur.reconstruction_byass = pyramid_offl_param[strength_level].reconstruction_byass;
		dst_ptr->cur.offline_layer_num = pyramid_offl_param[strength_level].offline_layer_num;
	}
	return rtn;
}

cmr_s32 _pm_pyramid_offl_init(void *dst_pyramid_offl_param, void *src_pyramid_offl_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pyramid_offl_param *dst_ptr = (struct isp_pyramid_offl_param *)dst_pyramid_offl_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_pyramid_offl_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.dec_offline_bypass = header_ptr->bypass;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	if(!header_ptr->bypass)
		rtn = _pm_pyramid_offl_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.dec_offline_bypass |= header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm pyramid_offl param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_pyramid_offl_set_param(void * pyramid_offl_param, cmr_u32 cmd, void * param_ptr0, void * param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pyramid_offl_param *dst_ptr = (struct isp_pyramid_offl_param *)pyramid_offl_param;
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

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_RGB_DITHER_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_RGB_DITHER_T] = 0;

				rtn = _pm_pyramid_offl_convert_param(dst_ptr, dst_ptr->cur_level, header_ptr->mode_id, block_result->scene_flag);
				dst_ptr->cur.dec_offline_bypass |= header_ptr->bypass;

				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm pyramid_offl param!");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART: cmd=%d, update=%d, pyramid_offl_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_pyramid_offl_get_param(void * pyramid_offl_param, cmr_u32 cmd, void * rtn_param0, void * rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pyramid_offl_param *pyramid_onl_ptr = (struct isp_pyramid_offl_param *)pyramid_offl_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&pyramid_onl_ptr->cur;
		param_data_ptr->data_size = sizeof(pyramid_onl_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

