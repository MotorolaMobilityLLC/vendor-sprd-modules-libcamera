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
#define LOG_TAG "isp_blk_hsv"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_hsv_new2_init(void *dst_hsv_param, void *src_hsv_param, void *param1, void *param2)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param_new2 *dst_ptr = (struct isp_hsv_param_new2 *)dst_hsv_param;
	struct sensor_hsv_new2_param *src_ptr = (struct sensor_hsv_new2_param *)src_hsv_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;
	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx.weight1;

	for (i = 0; i < SENSOR_HSV_NUM; i++) {
		for (j = 0; j < SENSOR_HSV_TAB_NUM; j++){
				dst_ptr->hsv_table[i].hue_table[j] = src_ptr->hsv_table[i].hue_table[j];
				dst_ptr->hsv_table[i].sat_table[j] = src_ptr->hsv_table[i].sat_table[j];
			}
	}

	for (i = 0; i < 5; i++) {
		for (j = 0; j < 4; j++) {
			dst_ptr->cur.curve_info.s_curve[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_s_curve[j];
			dst_ptr->cur.curve_info.v_curve[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_v_curve[j];
		}
		for (j = 0; j < 2; j++) {
			dst_ptr->cur.curve_info.r_s[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_r_s[j];
			dst_ptr->cur.curve_info.r_v[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_r_v[j];
		}
		dst_ptr->cur.curve_info.hrange_left[i] = src_ptr->sensor_hsv_cfg[i].hsv_hrange_left;
		dst_ptr->cur.curve_info.hrange_right[i] = src_ptr->sensor_hsv_cfg[i].hsv_hrange_right;
	}

	for (i = 0; i < SENSOR_HSV_TAB_NUM; i++) {
		dst_ptr->cur.d.hs.hue[i] = dst_ptr->hsv_table[index].hue_table[i];
		dst_ptr->cur.d.hs.sat[i] = dst_ptr->hsv_table[index].sat_table[i];
	}

	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_hsv_new2_set_param(void *hsv_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param_new2 *dst_hsv_ptr = (struct isp_hsv_param_new2 *)hsv_param;
	struct isp_pm_block_header *hsv_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			cmr_u32 data_num;
			cmr_u16 weight[2] = { 0, 0 };
			cmr_s32 hsv_level = -1;
			void * src_h[2] = { NULL, NULL };
			void *src_s[2] = {NULL, NULL};
			void *dst = NULL;
			void *dst1 = NULL;
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_range val_range = { 0, 0 };
			struct isp_weight_value *bv_value;

			if (0 == block_result->update || hsv_header_ptr->bypass) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			hsv_header_ptr->is_update = ISP_ZERO;
			val_range.min = 0;
			val_range.max = 255;
			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_WEIGHT_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}
			weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
			bv_value = &weight_value[0];

			switch (block_result->ai_scene_id) {
			case ISP_PM_AI_SCENE_FOOD:
				hsv_level = 10;
				break;
			case ISP_PM_AI_SCENE_PORTRAIT:
				hsv_level = 11;
				break;
			case ISP_PM_AI_SCENE_FOLIAGE:
				hsv_level = 12;
				break;
			case ISP_PM_AI_SCENE_SKY:
				hsv_level = 13;
				break;
			case ISP_PM_AI_SCENE_NIGHT:
				hsv_level = 14;
				break;
			case ISP_PM_AI_SCENE_TEXT:
				hsv_level = 16;
				break;
			case ISP_PM_AI_SCENE_SUNRISE:
				hsv_level = 17;
				break;
			case ISP_PM_AI_SCENE_BUILDING:
				hsv_level = 18;
				break;
			case ISP_PM_AI_SCENE_SNOW:
				hsv_level = 20;
				break;
			case ISP_PM_AI_SCENE_FIREWORK:
				hsv_level = 21;
				break;
			case ISP_PM_AI_SCENE_PET:
				hsv_level = 23;
				break;
			case ISP_PM_AI_SCENE_FLOWER:
				hsv_level = 24;
				break;
			default:
				hsv_level = -1;
				break;
			}

			if (block_result->ai_scene_pro_flag == 1 || block_result->ai_scene_id == ISP_PM_AI_SCENE_NIGHT)
				hsv_level = -1;

			if (hsv_level != -1) {
				bv_value->value[0] = hsv_level;
				bv_value->value[1] = hsv_level;
				bv_value->weight[0] = 256;
				bv_value->weight[1] = 0;
			}

			dst = &dst_hsv_ptr->cur.d.hs.hue;
			dst1 = &dst_hsv_ptr->cur.d.hs.sat;
			data_num = SENSOR_HSV_TAB_NUM;
			src_h[0] = (void *)&dst_hsv_ptr->hsv_table[bv_value->value[0]].hue_table;
			src_h[1] = (void *)&dst_hsv_ptr->hsv_table[bv_value->value[1]].hue_table;
			src_s[0] = (void *)&dst_hsv_ptr->hsv_table[bv_value->value[0]].sat_table;
			src_s[1] = (void *)&dst_hsv_ptr->hsv_table[bv_value->value[1]].sat_table;
			weight[0] = bv_value->weight[0];
			weight[1] = bv_value->weight[1];
			weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
			weight[1] = SMART_WEIGHT_UNIT - weight[0];
			isp_interp_data((void *)dst, src_h , weight , data_num , ISP_INTERP_UINT16);
			isp_interp_data((void *)dst1, src_s , weight , data_num , ISP_INTERP_UINT16);

			hsv_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_AI_SCENE_UPDATE_HSV:
		{
			cmr_u32 i;
			cmr_s16 smooth_factor, smooth_base;
			struct isp_ai_update_param *cfg_data;
			struct isp_ai_hsv_info *hsv_cur;
			struct isp_hsv_table  data, *hsv_param = &data;

			cfg_data = (struct isp_ai_update_param *)param_ptr0;
			hsv_cur = (struct isp_ai_hsv_info *)cfg_data->param_ptr;
			smooth_factor = cfg_data->smooth_factor;
			smooth_base = cfg_data->smooth_base;
			if (smooth_factor == 0) {
				if (!hsv_header_ptr->is_update)
					break;
				smooth_factor = 1;
				smooth_base = 1;
			} else if (!hsv_header_ptr->is_update) {
				smooth_factor = (smooth_factor > 0) ? 1 :  -1;
			}

			memcpy(hsv_param, &dst_hsv_ptr->cur.d.hs, sizeof(struct isp_hsv_table));
			for (i = 0; i < 360; i++) {
				hsv_param->hue_table[i] += hsv_cur->hue_table_item_offset[i] * smooth_factor / smooth_base;
				hsv_param->sat_table[i] += hsv_cur->saturation_table_item_offset[i]  * smooth_factor / smooth_base;
				hsv_param->hue_table[i] = MAX(0, MIN(359, hsv_param->hue_table[i]));
				hsv_param->sat_table[i] = MAX(0, MIN(2047, hsv_param->sat_table[i]));
			}
			memcpy(&dst_hsv_ptr->cur.d.hs, hsv_param, sizeof(struct isp_hsv_table));

			hsv_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
		{
			hsv_header_ptr->is_update = ISP_ONE;
		}
		break;

	default:
		break;
	}


	return rtn;
}

cmr_s32 _pm_hsv_new2_get_param(void *hsv_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param_new2 *hsv_ptr = (struct isp_hsv_param_new2 *)hsv_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&hsv_ptr->cur;
		param_data_ptr->data_size = sizeof(hsv_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
