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
#define LOG_TAG "isp_blk_post_cnr_h"
#include "isp_blocks_cfg.h"
#include <math.h>

cmr_u32 _pm_post_cnr_h_convert_param(void *dst_post_cnr_h_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_post_cnr_h_param *dst_ptr = (struct isp_post_cnr_h_param *)dst_post_cnr_h_param;
	struct sensor_post_cnr_h_level *post_cnr_h_param = PNULL;
	cmr_s32 i, j;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		post_cnr_h_param = (struct sensor_post_cnr_h_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		post_cnr_h_param = (struct sensor_post_cnr_h_level *)((cmr_u8 *) dst_ptr->param_ptr +
					total_offset_units * dst_ptr->level_num * sizeof(struct sensor_post_cnr_h_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (post_cnr_h_param != NULL) {
		dst_ptr->cur.baseRadius = post_cnr_h_param[strength_level].radius_base;
		dst_ptr->cur.param_post_cnr_h.lowpass_filter_en = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.lowpass_filter_en;
		dst_ptr->cur.param_post_cnr_h.denoise_radial_en = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.denoise_radial_en;
		dst_ptr->cur.param_post_cnr_h.filter_size = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.filter_size;
		dst_ptr->cur.param_post_cnr_h.imgCenterX = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.imgCenterX;
		dst_ptr->cur.param_post_cnr_h.imgCenterY= post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.imgCenterY;
		dst_ptr->cur.param_post_cnr_h.baseRadius = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.baseRadius_factor;
		dst_ptr->cur.param_post_cnr_h.minRatio = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.minRatio;
		dst_ptr->cur.param_post_cnr_h.slope = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.slope;
		for(i = 0; i < 3; i++){
			dst_ptr->cur.param_post_cnr_h.order_y[i] = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.order_y[i];
			dst_ptr->cur.param_post_cnr_h.order_uv[i] = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.order_uv[i];
			dst_ptr->cur.param_post_cnr_h.sigma_y[i] = (cmr_u32) (post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.sigma_y[i]);
			dst_ptr->cur.param_post_cnr_h.sigma_uv[i] = (cmr_u32) (post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.sigma_uv[i]);
			for (j = 0; j < 72; j++) {
				post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.weight_y[i][j] = (cmr_u32) (1.0 / (1.0 + pow((double) (j /
						post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.sigma_y[i]),
						(double) (post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.order_y[i]))) * 255 + 0.5);
				post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.weight_uv[i][j] = (cmr_u32) (1.0 / (1.0 + pow((double) (j /
						post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.sigma_uv[i]),
						(double) (post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.order_uv[i]))) * 255 + 0.5);
				dst_ptr->cur.param_post_cnr_h.weight_y[i][j] = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.weight_y[i][j];
				dst_ptr->cur.param_post_cnr_h.weight_uv[i][j] = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.weight_uv[i][j];
			}
		}
		for(j = 0; j < 2; j++){
			dst_ptr->cur.param_post_cnr_h.luma_th[j] = post_cnr_h_param[strength_level].multilayer_param_post_cnr_h.luma_th[j];
		}
		dst_ptr->level_info.level_enable = post_cnr_h_param[strength_level].level_enable;
		dst_ptr->level_info.low_ct_thrd = post_cnr_h_param[strength_level].low_ct_thrd;
		ISP_LOGV("ISP_SMART_NR: dst_ptr->level_info.level_enable=%d,dst_ptr->level_info.low_ct_thrd %d", dst_ptr->level_info.level_enable,dst_ptr->level_info.low_ct_thrd);
	}

	return rtn;
}

cmr_s32 _pm_post_cnr_h_init(void *dst_post_cnr_h_param, void *src_post_cnr_h_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_post_cnr_h_param *dst_ptr = (struct isp_post_cnr_h_param *)dst_post_cnr_h_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_post_cnr_h_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;

	rtn = _pm_post_cnr_h_convert_param(dst_ptr, dst_ptr->cur_level, ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass = 0;
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm post_cnr_h param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_post_cnr_h_set_param(void * post_cnr_h_param, cmr_u32 cmd, void * param_ptr0, void * param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_post_cnr_h_param *dst_ptr = (struct isp_post_cnr_h_param *)post_cnr_h_param;
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

			if (cur_level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_POST_CNR_H_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_POST_CNR_H_T] = 0;

				rtn = _pm_post_cnr_h_convert_param(dst_ptr, dst_ptr->cur_level, header_ptr->mode_id, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;

				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to  convert pm post_cnr_h param!");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART: cmd=%d, update=%d, post_cnr_h_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_post_cnr_h_get_param(void * post_cnr_h_param, cmr_u32 cmd, void * rtn_param0, void * rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_post_cnr_h_param *post_cnr_h_ptr = (struct isp_post_cnr_h_param *)post_cnr_h_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_POST_CNR_H;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &post_cnr_h_ptr->cur;
		param_data_ptr->data_size = sizeof(post_cnr_h_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	case ISP_PM_BLK_POST_CNR_H_LEVEL_INFO:
		param_data_ptr->data_ptr = &post_cnr_h_ptr->level_info;
		param_data_ptr->data_size = sizeof(post_cnr_h_ptr->level_info);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}

