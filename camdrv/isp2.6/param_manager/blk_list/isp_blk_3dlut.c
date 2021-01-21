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
#define LOG_TAG "isp_blk_3dlut"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_3dlut_init(void *dst_3dlut_param, void *src_3dlut_param, void *param1, void *param2)
{
	cmr_u32 i = 0, j = 0, k =0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_3dlut_param *dst_ptr = (struct isp_3dlut_param *)dst_3dlut_param;
	struct sensor_3dlut_param *src_ptr = (struct sensor_3dlut_param *)src_3dlut_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.rgb3dlut_bypass = header_ptr->bypass;
	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx.weight1;

	for (i = 0; i < ISP_3D_LUT_NUM; i++) {
		for (j = 0; j < 729; j++) {
			for (k = 0; k < 6; k++) {
				dst_ptr->table[i].rgb3dlut_final_table[j][k] = src_ptr->rgb3dlut_ct_table[i][j][k];
			}
		}
	}

	for (i = 0; i < 729; i++) {
		for (j = 0; j < 6; j++) {
			dst_ptr->cur.rgb3dlut_ct_table[i][j] = dst_ptr->table[index].rgb3dlut_final_table[i][j];
		}
	}
	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_3dlut_set_param(void *dlut_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_3dlut_param *dst_3dlut_ptr = (struct isp_3dlut_param *)dlut_param;
	struct isp_pm_block_header *dlut_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			cmr_u16 weight[2] = { 0, 0 };
			cmr_s32 dlut_level = -1;
			cmr_u32 i = 0,j = 0,k = 0;
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_weight_value *bv_value;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 *dst1_b;
			cmr_u32 dst1_c[2][729][6] = {0};

			if (0 == block_result->update || dlut_header_ptr->bypass) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			dlut_header_ptr->is_update = ISP_ZERO;
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
				dlut_level = 10;
				break;
			case ISP_PM_AI_SCENE_PORTRAIT:
				dlut_level = 11;
				break;
			case ISP_PM_AI_SCENE_FOLIAGE:
				dlut_level = 12;
				break;
			case ISP_PM_AI_SCENE_SKY:
				dlut_level = 13;
				break;
			case ISP_PM_AI_SCENE_NIGHT:
				dlut_level = 14;
				break;
			case ISP_PM_AI_SCENE_TEXT:
				dlut_level = 16;
				break;
			case ISP_PM_AI_SCENE_SUNRISE:
				dlut_level = 17;
				break;
			case ISP_PM_AI_SCENE_BUILDING:
				dlut_level = 18;
				break;
			case ISP_PM_AI_SCENE_SNOW:
				dlut_level = 20;
				break;
			case ISP_PM_AI_SCENE_FIREWORK:
				dlut_level = 21;
				break;
			case ISP_PM_AI_SCENE_PET:
				dlut_level = 23;
				break;
			case ISP_PM_AI_SCENE_FLOWER:
				dlut_level = 24;
				break;
			default:
				dlut_level = -1;
				break;
			}

			if (block_result->ai_scene_pro_flag == 1 || block_result->ai_scene_id == ISP_PM_AI_SCENE_NIGHT)
				dlut_level = -1;

			if (dlut_level != -1) {
				bv_value->value[0] = dlut_level;
				bv_value->value[1] = dlut_level;
				bv_value->weight[0] = 256;
				bv_value->weight[1] = 0;
			}

			for (i = 0; i < 2; i++) {
				cmr_u32 src_matrix[2][729][6] = {0};
				//src for smart interp base on ct
				memcpy(src_matrix[0], dst_3dlut_ptr->table[ct_value[i]->value[0]].rgb3dlut_final_table, sizeof(src_matrix[0]));
				memcpy(src_matrix[1], dst_3dlut_ptr->table[ct_value[i]->value[1]].rgb3dlut_final_table, sizeof(src_matrix[1]));
				cmr_u16 weight[2] = { 0 };
				weight[0] = ct_value[i]->weight[0];
				weight[1] = ct_value[i]->weight[1];
				weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];

				for (j = 0; j < 729; j++) {
					for (k = 0;k < 6; k++) {
						dst1_c[i][j][k] = (src_matrix[0][j][k]*weight[0] + src_matrix[1][j][k]*weight[1]) / 256;
					}
				}
			}

			//dst for write reg
			dst1_b = (cmr_u32 *)dst_3dlut_ptr->cur.rgb3dlut_ct_table;

			cmr_u32 src_matrix1[2][729][6] = {0};

			memcpy(src_matrix1[0], dst1_c[0], sizeof(src_matrix1[0]));
			memcpy(src_matrix1[1], dst1_c[1], sizeof(src_matrix1[1]));

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

			for (j = 0; j < 729; j++) {
				for (k = 0;k < 6; k++) {
					*(dst1_b+(j*6+k)) = (src_matrix1[0][j][k]*weight[0] + src_matrix1[1][j][k]*weight[1]) / 256;
				}
			}
			ISP_LOGV("ai_scene: %d, SMART: w (%d %d) v (%d %d); weight (%d %d)\n", block_result->ai_scene_id,
				bv_value->weight[0], bv_value->weight[1], bv_value->value[0], bv_value->value[1], weight[0], weight[1]);
			dlut_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
		{
			dlut_header_ptr->is_update = ISP_ONE;
		}
		break;

	default:
		break;
	}


	return rtn;
}

cmr_s32 _pm_3dlut_get_param(void *dlut_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_3dlut_param *dlut_ptr = (struct isp_3dlut_param *)dlut_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&dlut_ptr->cur;
		param_data_ptr->data_size = sizeof(dlut_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
