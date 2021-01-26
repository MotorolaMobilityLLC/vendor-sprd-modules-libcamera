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

cmr_s32 _pm_hsv_new3_init(void *dst_hsv_param, void *src_hsv_param, void *param1, void *param2)
{
	cmr_u32 i = 0, j = 0,k = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param_new3 *dst_ptr =
		(struct isp_hsv_param_new3 *)dst_hsv_param;
	struct sensor_hsv_lut_param *src_ptr =
		(struct sensor_hsv_lut_param *)src_hsv_param;
	struct isp_pm_block_header *header_ptr =
		(struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.hsv_bypass = header_ptr->bypass;
	//for smart calc init
	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx.weight1;

	dst_ptr->cur.hsv_bypass = src_ptr->hsv_bypass;
	dst_ptr->cur.hsv_delta_value_en = src_ptr->delta_value_en;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 2; j++)
			dst_ptr->cur.hsv_hue_thr[i][j] = src_ptr->hsv_hue_thr[i][j];
	}
	for (i = 0; i < 25; i++) {
		dst_ptr->cur.hsv_1d_hue_lut[i] = src_ptr->hsv_1d_hue_lut[i];
	}
	for (i = 0; i < 17; i++) {
		dst_ptr->cur.hsv_1d_sat_lut[i] = src_ptr->hsv_1d_sat_lut[i];
	}
	for (i = 0; i < 4; i++) {
		dst_ptr->cur.hsv_param[i].hsv_curve_param.start_a = src_ptr->hsv_curve_param[i].start_a;
		dst_ptr->cur.hsv_param[i].hsv_curve_param.end_a = src_ptr->hsv_curve_param[i].end_a;
		dst_ptr->cur.hsv_param[i].hsv_curve_param.start_b = src_ptr->hsv_curve_param[i].start_b;
		dst_ptr->cur.hsv_param[i].hsv_curve_param.end_b = src_ptr->hsv_curve_param[i].end_b;
	}
	for (k = 0; k < SENSOR_HSV_NUM; k++) {
		for (i = 0; i< 25; i++) {
			for( j = 0; j < 17; j++)
				dst_ptr->hsv_lut[k].hsv_2d_hue_lut[i][j] = src_ptr->hsv_lut[k].hsv_2d_hue_lut[i][j];
		}
		for (i = 0; i < 25; i++) {
			for( j = 0; j < 17; j++)
				dst_ptr->hsv_lut[k].hsv_2d_sat_lut[i][j] = src_ptr->hsv_lut[k].hsv_2d_sat_lut[i][j];
		}
		for (i = 0; i < 25; i++) {
			for( j = 0; j < 17; j++)
				dst_ptr->hsv_lut[k].hsv_2d_hue_lut_reg[i][j] =src_ptr->hsv_lut[k].hsv_2d_hue_lut_reg[i][j];
		}
		dst_ptr->hsv_lut[k].y_blending_factor = src_ptr->hsv_lut[k].y_blending_factor;
	}

	for (i = 0; i< 25; i++) {
		for( j = 0; j < 17; j++)
			dst_ptr->cur.buf_param.hsv_2d_sat_lut[i][j] = dst_ptr->hsv_lut[index].hsv_2d_sat_lut[i][j];
	}
	for (i = 0; i < 25; i++) {
		for( j = 0; j < 17; j++)
			dst_ptr->cur.buf_param.hsv_2d_hue_lut_reg[i][j] = dst_ptr->hsv_lut[index].hsv_2d_hue_lut_reg[i][j];
	}
	dst_ptr->cur.y_blending_factor= src_ptr->hsv_lut[index].y_blending_factor;

	for (i = 0; i < MAX_SPECIALEFFECT_NUM; i++) {
		dst_ptr->specialeffect[i].offset = src_ptr->specialeffect[i].offset;
		dst_ptr->specialeffect[i].size = src_ptr->specialeffect[i].size;
	}

	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_hsv_new3_set_param(void *hsv_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param_new3 *dst_hsv_ptr = (struct isp_hsv_param_new3 *)hsv_param;
	struct isp_pm_block_header *hsv_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			cmr_u16 weight[2] = { 0, 0 };
			cmr_s32 hsv_level = -1;
			cmr_u32 i = 0,j = 0,k = 0;
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_weight_value *bv_value;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 *dst0_b;
			cmr_u32 *dst1_b;
			cmr_u32 *dst2_b;
			cmr_u16 dst0_c[2] = {0};
			cmr_u16 dst1_c[2][25][17] = {0};
			cmr_u16 dst2_c[2][25][17] = {0};

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
			struct isp_weight_value *ct_value[2] = { &weight_value[1], &weight_value[2] };
			cmr_u32 index0 = bv_value->value[0];
			cmr_u32 index1 = bv_value->value[1];

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

			for (i = 0; i < 2; i++) {
				cmr_u16 src_sat_matrix[2][25][17] = {0};
				cmr_u16 src_reg_matrix[2][25][17] = {0};
				cmr_u16 src_matrix[2] = { 0 };
				//src for smart interp base on ct
				src_matrix[0] = dst_hsv_ptr->hsv_lut[ct_value[i]->value[0]].y_blending_factor;
				src_matrix[1] = dst_hsv_ptr->hsv_lut[ct_value[i]->value[1]].y_blending_factor;
				memcpy(src_sat_matrix[0], dst_hsv_ptr->hsv_lut[ct_value[i]->value[0]].hsv_2d_sat_lut,
					sizeof(dst_hsv_ptr->hsv_lut[ct_value[i]->value[0]].hsv_2d_sat_lut));
				memcpy(src_sat_matrix[1], dst_hsv_ptr->hsv_lut[ct_value[i]->value[1]].hsv_2d_sat_lut,
					sizeof(dst_hsv_ptr->hsv_lut[ct_value[i]->value[1]].hsv_2d_sat_lut));
				memcpy(src_reg_matrix[0], dst_hsv_ptr->hsv_lut[ct_value[i]->value[0]].hsv_2d_hue_lut_reg,
					sizeof(dst_hsv_ptr->hsv_lut[ct_value[i]->value[0]].hsv_2d_hue_lut_reg));
				memcpy(src_reg_matrix[1], dst_hsv_ptr->hsv_lut[ct_value[i]->value[1]].hsv_2d_hue_lut_reg,
					sizeof(dst_hsv_ptr->hsv_lut[ct_value[i]->value[1]].hsv_2d_hue_lut_reg));
				cmr_u16 weight[2] = { 0 };
				weight[0] = ct_value[i]->weight[0];
				weight[1] = ct_value[i]->weight[1];
				weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];

				dst0_c[i] = src_matrix[0]*weight[0] + src_matrix[1]*weight[1];
				for (j = 0; j < 25; j++) {
					for (k = 0;k < 17; k++) {
						dst1_c[i][j][k] = (src_sat_matrix[0][j][k]*weight[0] + src_sat_matrix[1][j][k]*weight[1]) / 256;
						dst2_c[i][j][k] = (src_reg_matrix[0][j][k]*weight[0] + src_reg_matrix[1][j][k]*weight[1]) / 256;
					}
				}
			}

			//dst for write reg
			dst0_b = &dst_hsv_ptr->cur.y_blending_factor;
			dst1_b = (cmr_u32 *)dst_hsv_ptr->cur.buf_param.hsv_2d_sat_lut;
			dst2_b = (cmr_u32 *)dst_hsv_ptr->cur.buf_param.hsv_2d_hue_lut_reg;

			if (index0 != index1) {
				//src for smart interp base on bv
				weight[0] = bv_value->weight[0];
				weight[1] = bv_value->weight[1];
				weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];
			} else {
				//src for smart interp base on ct
				weight[0] = ct_value[0]->weight[0];
				weight[1] = ct_value[0]->weight[1];
				weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];
			}
			*dst0_b = dst0_c[0]*weight[0] + dst0_c[1]*weight[1];
			for (j = 0; j < 25; j++) {
				for (k = 0;k < 17; k++) {
					*(dst1_b+(j*17+k)) = (dst1_c[0][j][k]*weight[0] + dst1_c[1][j][k]*weight[1]) / 256;
					*(dst2_b+(j*17+k)) = (dst2_c[0][j][k]*weight[0] + dst2_c[1][j][k]*weight[1]) / 256;
				}
			}

			hsv_header_ptr->is_update = ISP_ONE;
		}
		break;
	case ISP_PM_BLK_AI_SCENE_UPDATE_HSV:
		{
			cmr_u32 i = 0,j = 0;
			cmr_s16 smooth_factor, smooth_base;
			struct isp_ai_update_param *cfg_data;
			struct isp_ai_hsv_info_v1 *hsv_cur_v1;
			struct isp_hsv_table_v0 data, *hsv_param = &data;
			cfg_data = (struct isp_ai_update_param *)param_ptr0;
			hsv_cur_v1 = (struct isp_ai_hsv_info_v1 *)cfg_data->param_ptr;
			smooth_factor = cfg_data->smooth_factor;
			smooth_base = cfg_data->smooth_base;
			if (smooth_factor == 0) {
				if (!hsv_header_ptr->is_update)
					break;
				smooth_factor = 1;
				smooth_base = 1;
			} else if (!hsv_header_ptr->is_update) {
				smooth_factor = (smooth_factor > 0) ? 1 : -1;
			}
			for (i = 0; i< 25; i++) {
				for( j = 0; j < 17; j++) {
					hsv_param->hsv_2d_sat_lut[i][j] = dst_hsv_ptr->cur.buf_param.hsv_2d_sat_lut[i][j];
					hsv_param->hsv_2d_hue_lut_reg[i][j] = dst_hsv_ptr->cur.buf_param.hsv_2d_hue_lut_reg[i][j];
				}
			}
			for (i = 0; i < 25; i++) {
				for (j = 0;j < 17; j++) {
				hsv_param->hsv_2d_sat_lut[i][j] += hsv_cur_v1->sat_offset[i][j] * smooth_factor / smooth_base;
				hsv_param->hsv_2d_hue_lut_reg[i][j] += hsv_cur_v1->hue_offset[i][j] * smooth_factor / smooth_base;
				hsv_param->hsv_2d_sat_lut[i][j] = MAX(0, MIN(359, hsv_param->hsv_2d_sat_lut[i][j]));
				hsv_param->hsv_2d_hue_lut_reg[i][j] = MAX(0, MIN(2047, hsv_param->hsv_2d_hue_lut_reg[i][j]));
				}
			}
			for (i = 0; i< 25; i++) {
				for( j = 0; j < 17; j++) {
				dst_hsv_ptr->cur.buf_param.hsv_2d_sat_lut[i][j] = hsv_param->hsv_2d_sat_lut[i][j];
				dst_hsv_ptr->cur.buf_param.hsv_2d_hue_lut_reg[i][j] = hsv_param->hsv_2d_hue_lut_reg[i][j];
				}
			}
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

cmr_s32 _pm_hsv_new3_get_param(void *hsv_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param_new3 *hsv_ptr = (struct isp_hsv_param_new3 *)hsv_param;
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