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
#define LOG_TAG "isp_blk_ai_pro"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_ai_pro_init(void *dst_ai_param, void *src_ai_param, void *param1, void *param2)
{
	cmr_u32 i ,j = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ai_param *dst_ptr = (struct isp_ai_param *)dst_ai_param;
	struct sensor_ai_param *src_ptr = (struct sensor_ai_param *)src_ai_param;
	struct isp_pm_block_header *ai_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	if (ai_header_ptr->bypass)
		return rtn;

	for (i = 0; i < AI_SCENE_MAX;i++) {
		dst_ptr->isp_ai_bchs[i].ai_brightness.brightness_ai_adj_eb = src_ptr->sensor_ai_bchs[i].ai_brightness.brightness_ai_adj_eb;
		dst_ptr->isp_ai_bchs[i].ai_contrast.contrast_adj_ai_eb = src_ptr->sensor_ai_bchs[i].ai_contrast.contrast_adj_ai_eb;
		dst_ptr->isp_ai_bchs[i].ai_hue.hue_adj_ai_eb = src_ptr->sensor_ai_bchs[i].ai_hue.hue_adj_ai_eb;
		dst_ptr->isp_ai_bchs[i].ai_saturation.saturation_adj_ai_eb = src_ptr->sensor_ai_bchs[i].ai_saturation.saturation_adj_ai_eb;
		dst_ptr->isp_ai_hsv[i].hue_adj_ai_eb = src_ptr->sensor_ai_hsv[i].hue_adj_ai_eb;
		dst_ptr->smooth_frame_ai[i] = src_ptr->smooth_frame_ai[i];

		for (j = 0; j < 8; j++) {
			dst_ptr->isp_ai_bchs[i].ai_brightness.brightness_adj_factor_offset[j] = src_ptr->sensor_ai_bchs[i].ai_brightness.brightness_adj_factor_offset[j];
			dst_ptr->isp_ai_bchs[i].ai_contrast.contrast_adj_factor_offset[j] = src_ptr->sensor_ai_bchs[i].ai_contrast.contrast_adj_factor_offset[j];
			dst_ptr->isp_ai_bchs[i].ai_hue.hue_sin_offset[j] = src_ptr->sensor_ai_bchs[i].ai_hue.hue_sin_offset[j];
			dst_ptr->isp_ai_bchs[i].ai_hue.hue_cos_offset[j] = src_ptr->sensor_ai_bchs[i].ai_hue.hue_cos_offset[j];
			dst_ptr->isp_ai_bchs[i].ai_saturation.saturation_adj_factor_u_offset[j] = src_ptr->sensor_ai_bchs[i].ai_saturation.saturation_adj_factor_u_offset[j];
			dst_ptr->isp_ai_bchs[i].ai_saturation.saturation_adj_factor_v_offset[j] = src_ptr->sensor_ai_bchs[i].ai_saturation.saturation_adj_factor_v_offset[j];
			dst_ptr->isp_ai_ee[i].ee_enable = !src_ptr->sensor_ai_ee[i].ee_bypass;
			dst_ptr->isp_ai_ee[i].ratio_old_gradient_offset[j] = src_ptr->sensor_ai_ee[i].ratio_old_gradient_offset[j];
			dst_ptr->isp_ai_ee[i].ratio_new_pyramid_offset[j] = src_ptr->sensor_ai_ee[i].ratio_new_pyramid_offset[j];
			dst_ptr->isp_ai_ee[i].ee_gain_hv1[j].ee_r1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_hv1[j].ee_r1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_hv1[j].ee_r2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_hv1[j].ee_r2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_hv1[j].ee_r3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_hv1[j].ee_r3_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_hv2[j].ee_r1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_hv2[j].ee_r1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_hv2[j].ee_r2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_hv2[j].ee_r2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_hv2[j].ee_r3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_hv2[j].ee_r3_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_diag1[j].ee_r1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_diag1[j].ee_r1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_diag1[j].ee_r2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_diag1[j].ee_r2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_diag1[j].ee_r3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_diag1[j].ee_r3_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_diag2[j].ee_r1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_diag2[j].ee_r1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_diag2[j].ee_r2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_diag2[j].ee_r2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_gain_diag2[j].ee_r3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_gain_diag2[j].ee_r3_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_pos_r[j].ee_r1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_pos_r[j].ee_r1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_pos_r[j].ee_r2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_pos_r[j].ee_r2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_pos_r[j].ee_r3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_pos_r[j].ee_r3_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_pos_c[j].ee_c1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_pos_c[j].ee_c1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_pos_c[j].ee_c2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_pos_c[j].ee_c2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_pos_c[j].ee_c3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_pos_c[j].ee_c3_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_neg_r[j].ee_r1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_neg_r[j].ee_r1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_neg_r[j].ee_r2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_neg_r[j].ee_r2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_neg_r[j].ee_r3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_neg_r[j].ee_r3_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_neg_c[j].ee_c1_cfg_offset = src_ptr->sensor_ai_ee[i].ee_neg_c[j].ee_c1_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_neg_c[j].ee_c2_cfg_offset = src_ptr->sensor_ai_ee[i].ee_neg_c[j].ee_c2_cfg_offset;
			dst_ptr->isp_ai_ee[i].ee_neg_c[j].ee_c3_cfg_offset = src_ptr->sensor_ai_ee[i].ee_neg_c[j].ee_c3_cfg_offset;
		}

		for (j = 0; j < 360; j++) {
			dst_ptr->isp_ai_hsv[i].hue_table_item_offset[j] = src_ptr->sensor_ai_hsv[i].hue_table_item_offset[j];
			dst_ptr->isp_ai_hsv[i].saturation_table_item_offset[j] = src_ptr->sensor_ai_hsv[i].saturation_table_item_offset[j];
		}
	}

	ai_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_ai_pro_set_param(void *ai_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ai_param *dst_ptr = (struct isp_ai_param *)ai_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };

			if (block_result->update == 0 || header_ptr->bypass) {
				return rtn;
			}

			if (ISP_SMART_AI_BCHS == block_result->smart_id) {
				cmr_u32 data_num;
				cmr_s32 ai_scene = 0;
				cmr_u16 weight[2] = { 0, 0 };
				void * src1[2] = { NULL, NULL };
				void *src2[2] = {NULL, NULL};
				void *src3[2] = {NULL, NULL};
				void *src4[2] = {NULL, NULL};
				void *src5[2] = {NULL, NULL};
				void *src6[2] = {NULL, NULL};
				void *dst1 = NULL;
				void *dst2 = NULL;
				void *dst3 = NULL;
				void *dst4 = NULL;
				void *dst5 = NULL;
				void *dst6 = NULL;
				struct isp_weight_value *weight_value = NULL;
				struct isp_weight_value *bv_value;

				if (0 == block_result->update) {
					ISP_LOGI("do not need update\n");
					return ISP_SUCCESS;
				}

				header_ptr->is_update = ISP_ZERO;
				val_range.min = 0;
				val_range.max = 255;
				rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_WEIGHT_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to check pm smart param !");
					return rtn;
				}
				weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
				bv_value = &weight_value[0];

				weight[0] = bv_value->weight[0];
				weight[1] = bv_value->weight[1];
				weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];

				switch (block_result->ai_scene_id) {
				case ISP_PM_AI_SCENE_DEFAULT:
					ai_scene = AI_SECNE_PM_PRO_DEFAULT;
					break;
				case ISP_PM_AI_SCENE_FOOD:
					ai_scene = AI_SECNE_PM_PRO_FOOD;
					break;
				case ISP_PM_AI_SCENE_PORTRAIT:
					ai_scene = AI_SECNE_PM_PRO_PORTRAIT;
					break;
				case ISP_PM_AI_SCENE_FOLIAGE:
					ai_scene = AI_SECNE_PM_PRO_FOLIAGE;
					break;
				case ISP_PM_AI_SCENE_SKY:
					ai_scene = AI_SECNE_PM_PRO_SKY;
					break;
				case ISP_PM_AI_SCENE_NIGHT:
					ai_scene = AI_SECNE_PM_PRO_NIGHT;
					break;
				case ISP_PM_AI_SCENE_TEXT:
					ai_scene = AI_SECNE_PM_PRO_DOCUMENT;
					break;
				case ISP_PM_AI_SCENE_SUNRISE:
					ai_scene = AI_SECNE_PM_PRO_SUNRISESET;
					break;
				case ISP_PM_AI_SCENE_BUILDING:
					ai_scene = AI_SECNE_PM_PRO_BUILDING;
					break;
				case ISP_PM_AI_SCENE_SNOW:
					ai_scene = AI_SECNE_PM_PRO_SNOW;
					break;
				case ISP_PM_AI_SCENE_FIREWORK:
					ai_scene = AI_SECNE_PM_PRO_FIREWORK;
					break;
				case ISP_PM_AI_SCENE_PET:
					ai_scene = AI_SECNE_PM_PRO_PET;
					break;
				case ISP_PM_AI_SCENE_FLOWER:
					ai_scene = AI_SECNE_PM_PRO_FLOWER;
					break;
				default:
					ai_scene = 0;
					break;
				}

				dst_ptr->bchs_cur.ai_brightness.brightness_ai_adj_eb = dst_ptr->isp_ai_bchs[ai_scene].ai_brightness.brightness_ai_adj_eb;
				dst_ptr->bchs_cur.ai_contrast.contrast_adj_ai_eb = dst_ptr->isp_ai_bchs[ai_scene].ai_contrast.contrast_adj_ai_eb;
				dst_ptr->bchs_cur.ai_hue.hue_adj_ai_eb = dst_ptr->isp_ai_bchs[ai_scene].ai_hue.hue_adj_ai_eb ;
				dst_ptr->bchs_cur.ai_saturation.saturation_adj_ai_eb = dst_ptr->isp_ai_bchs[ai_scene].ai_saturation.saturation_adj_ai_eb;
				dst_ptr->smooth_frame_ai_cur = dst_ptr->smooth_frame_ai[ai_scene];

				data_num = 1;
				if (dst_ptr->bchs_cur.ai_brightness.brightness_ai_adj_eb) {
					dst1 = &dst_ptr->bchs_cur.ai_brightness.brightness_adj_factor_offset;
					src1[0]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_brightness.brightness_adj_factor_offset[bv_value->value[0]];
					src1[1]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_brightness.brightness_adj_factor_offset[bv_value->value[1]];
					isp_interp_data((void *)dst1, src1 , weight , data_num , ISP_INTERP_UINT16);
				}
				if (ai_scene && (!dst_ptr->bchs_cur.ai_brightness.brightness_ai_adj_eb)){
					dst_ptr->bchs_cur.ai_brightness.brightness_adj_factor_offset = 0;
				}

				if (dst_ptr->bchs_cur.ai_contrast.contrast_adj_ai_eb){
					dst2 = &dst_ptr->bchs_cur.ai_contrast.contrast_adj_factor_offset;
					src2[0]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_contrast.contrast_adj_factor_offset[bv_value->value[0]];
					src2[1]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_contrast.contrast_adj_factor_offset[bv_value->value[1]];
					isp_interp_data((void *)dst2, src2 , weight , data_num , ISP_INTERP_UINT16);
				}
				if (ai_scene && (!dst_ptr->bchs_cur.ai_contrast.contrast_adj_ai_eb)){
					dst_ptr->bchs_cur.ai_contrast.contrast_adj_factor_offset = 0;
				}

				if (dst_ptr->bchs_cur.ai_hue.hue_adj_ai_eb) {
					dst3 = &dst_ptr->bchs_cur.ai_hue.hue_cos_offset;
					src3[0]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_hue.hue_cos_offset[bv_value->value[0]];
					src3[1]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_hue.hue_cos_offset[bv_value->value[1]];
					isp_interp_data((void *)dst3, src3 , weight , data_num , ISP_INTERP_UINT16);

					dst4 = &dst_ptr->bchs_cur.ai_hue.hue_sin_offset;
					src4[0]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_hue.hue_sin_offset[bv_value->value[0]];
					src4[1]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_hue.hue_sin_offset[bv_value->value[1]];
					isp_interp_data((void *)dst4, src4 , weight , data_num , ISP_INTERP_UINT16);
				}
				if (ai_scene && (!dst_ptr->bchs_cur.ai_hue.hue_adj_ai_eb)){
					dst_ptr->bchs_cur.ai_hue.hue_cos_offset = 0;
					dst_ptr->bchs_cur.ai_hue.hue_sin_offset = 0;
				}

				if (dst_ptr->bchs_cur.ai_saturation.saturation_adj_ai_eb) {
					dst5 = &dst_ptr->bchs_cur.ai_saturation.saturation_adj_factor_u_offset;
					src5[0]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_saturation.saturation_adj_factor_u_offset[bv_value->value[0]];
					src5[1]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_saturation.saturation_adj_factor_u_offset[bv_value->value[1]];
					isp_interp_data((void *)dst5, src5 , weight , data_num , ISP_INTERP_UINT16);

					dst6 = &dst_ptr->bchs_cur.ai_saturation.saturation_adj_factor_v_offset;
					src6[0]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_saturation.saturation_adj_factor_v_offset[bv_value->value[0]];
					src6[1]= (void *)&dst_ptr->isp_ai_bchs[ai_scene].ai_saturation.saturation_adj_factor_v_offset[bv_value->value[1]];
					isp_interp_data((void *)dst6, src6 , weight , data_num , ISP_INTERP_UINT16);
				}
				if (ai_scene && (!dst_ptr->bchs_cur.ai_saturation.saturation_adj_ai_eb)){
					dst_ptr->bchs_cur.ai_saturation.saturation_adj_factor_u_offset = 0;
					dst_ptr->bchs_cur.ai_saturation.saturation_adj_factor_v_offset = 0;
				}

				ISP_LOGV("hue_cos_offset %d ,hue_sin_offset %d, saturation_adj_factor_u_offset %d,%d,b %d, c %d\n",
					dst_ptr->bchs_cur.ai_hue.hue_cos_offset, dst_ptr->bchs_cur.ai_hue.hue_sin_offset,
					dst_ptr->bchs_cur.ai_saturation.saturation_adj_factor_u_offset,dst_ptr->bchs_cur.ai_saturation.saturation_adj_factor_v_offset,
					dst_ptr->bchs_cur.ai_brightness.brightness_adj_factor_offset,dst_ptr->bchs_cur.ai_contrast.contrast_adj_factor_offset);
				ISP_LOGV("ai_scene: %d, SMART: w (%d %d) v (%d %d); weight (%d %d)\n", block_result->ai_scene_id,
					bv_value->weight[0], bv_value->weight[1], bv_value->value[0], bv_value->value[1], weight[0], weight[1]);

				header_ptr->is_update = ISP_ONE;
			}else if (ISP_SMART_AI_EE == block_result->smart_id) {
				struct isp_range val_range = { 0, 0 };
				cmr_u32 level = 0;
				cmr_s32 ai_scene = 0;
				cmr_u32 i = 0;

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

				level = (cmr_u32) block_result->component[0].fix_data[0];

				switch (block_result->ai_scene_id) {
				case ISP_PM_AI_SCENE_DEFAULT:
					ai_scene = AI_SECNE_PM_PRO_DEFAULT;
					break;
				case ISP_PM_AI_SCENE_FOOD:
					ai_scene = AI_SECNE_PM_PRO_FOOD;
					break;
				case ISP_PM_AI_SCENE_PORTRAIT:
					ai_scene = AI_SECNE_PM_PRO_PORTRAIT;
					break;
				case ISP_PM_AI_SCENE_FOLIAGE:
					ai_scene = AI_SECNE_PM_PRO_FOLIAGE;
					break;
				case ISP_PM_AI_SCENE_SKY:
					ai_scene = AI_SECNE_PM_PRO_SKY;
					break;
				case ISP_PM_AI_SCENE_NIGHT:
					ai_scene = AI_SECNE_PM_PRO_NIGHT;
					break;
				case ISP_PM_AI_SCENE_TEXT:
					ai_scene = AI_SECNE_PM_PRO_DOCUMENT;
					break;
				case ISP_PM_AI_SCENE_SUNRISE:
					ai_scene = AI_SECNE_PM_PRO_SUNRISESET;
					break;
				case ISP_PM_AI_SCENE_BUILDING:
					ai_scene = AI_SECNE_PM_PRO_BUILDING;
					break;
				case ISP_PM_AI_SCENE_SNOW:
					ai_scene = AI_SECNE_PM_PRO_SNOW;
					break;
				case ISP_PM_AI_SCENE_FIREWORK:
					ai_scene = AI_SECNE_PM_PRO_FIREWORK;
					break;
				case ISP_PM_AI_SCENE_PET:
					ai_scene = AI_SECNE_PM_PRO_PET;
					break;
				case ISP_PM_AI_SCENE_FLOWER:
					ai_scene = AI_SECNE_PM_PRO_FLOWER;
					break;
				default:
					ai_scene = 0;
					break;
				}

				dst_ptr->ee_cur.ee_enable = dst_ptr->isp_ai_ee[ai_scene].ee_enable;
				dst_ptr->hsv_cur.hue_adj_ai_eb = dst_ptr->isp_ai_hsv[ai_scene].hue_adj_ai_eb;
				dst_ptr->smooth_frame_ai_cur = dst_ptr->smooth_frame_ai[ai_scene];

				if (dst_ptr->hsv_cur.hue_adj_ai_eb) {
					for (i = 0; i < 360; i++) {
						dst_ptr->hsv_cur.hue_table_item_offset[i] = dst_ptr->isp_ai_hsv[ai_scene].hue_table_item_offset[i];
						dst_ptr->hsv_cur.saturation_table_item_offset[i] = dst_ptr->isp_ai_hsv[ai_scene].saturation_table_item_offset[i];
					}
				}

				if (dst_ptr->ee_cur.ee_enable) {
					dst_ptr->ee_cur.ratio_old_gradient_offset = dst_ptr->isp_ai_ee[ai_scene].ratio_old_gradient_offset[level];
					dst_ptr->ee_cur.ratio_new_pyramid_offset = dst_ptr->isp_ai_ee[ai_scene].ratio_new_pyramid_offset[level];
					dst_ptr->ee_cur.ee_gain_hv1.ee_r1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_hv1[level].ee_r1_cfg_offset;
					dst_ptr->ee_cur.ee_gain_hv1.ee_r2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_hv1[level].ee_r2_cfg_offset;
					dst_ptr->ee_cur.ee_gain_hv1.ee_r3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_hv1[level].ee_r3_cfg_offset;
					dst_ptr->ee_cur.ee_gain_hv2.ee_r1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_hv2[level].ee_r1_cfg_offset;
					dst_ptr->ee_cur.ee_gain_hv2.ee_r2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_hv2[level].ee_r2_cfg_offset;
					dst_ptr->ee_cur.ee_gain_hv2.ee_r3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_hv2[level].ee_r3_cfg_offset;
					dst_ptr->ee_cur.ee_gain_diag1.ee_r1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_diag1[level].ee_r1_cfg_offset;
					dst_ptr->ee_cur.ee_gain_diag1.ee_r2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_diag1[level].ee_r2_cfg_offset;
					dst_ptr->ee_cur.ee_gain_diag1.ee_r3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_diag1[level].ee_r3_cfg_offset;
					dst_ptr->ee_cur.ee_gain_diag2.ee_r1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_diag2[level].ee_r1_cfg_offset;
					dst_ptr->ee_cur.ee_gain_diag2.ee_r2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_diag2[level].ee_r2_cfg_offset;
					dst_ptr->ee_cur.ee_gain_diag2.ee_r3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_gain_diag2[level].ee_r3_cfg_offset;
					dst_ptr->ee_cur.ee_pos_r.ee_r1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_pos_r[level].ee_r1_cfg_offset;
					dst_ptr->ee_cur.ee_pos_r.ee_r2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_pos_r[level].ee_r2_cfg_offset;
					dst_ptr->ee_cur.ee_pos_r.ee_r3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_pos_r[level].ee_r3_cfg_offset;
					dst_ptr->ee_cur.ee_pos_c.ee_c1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_pos_c[level].ee_c1_cfg_offset;
					dst_ptr->ee_cur.ee_pos_c.ee_c2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_pos_c[level].ee_c2_cfg_offset;
					dst_ptr->ee_cur.ee_pos_c.ee_c3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_pos_c[level].ee_c3_cfg_offset;
					dst_ptr->ee_cur.ee_neg_r.ee_r1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_neg_r[level].ee_r1_cfg_offset;
					dst_ptr->ee_cur.ee_neg_r.ee_r2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_neg_r[level].ee_r2_cfg_offset;
					dst_ptr->ee_cur.ee_neg_r.ee_r3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_neg_r[level].ee_r3_cfg_offset;
					dst_ptr->ee_cur.ee_neg_c.ee_c1_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_neg_c[level].ee_c1_cfg_offset;
					dst_ptr->ee_cur.ee_neg_c.ee_c2_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_neg_c[level].ee_c2_cfg_offset;
					dst_ptr->ee_cur.ee_neg_c.ee_c3_cfg_offset = dst_ptr->isp_ai_ee[ai_scene].ee_neg_c[level].ee_c3_cfg_offset;
				}
			}

			header_ptr->is_update = ISP_ONE;
		}
		break;

	default:

		break;
	}

	return rtn;
}

cmr_s32 _pm_ai_pro_get_param(void *ai_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ai_param *ai_ptr = (struct isp_ai_param *)ai_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	UNUSED(rtn_param1);

	param_data_ptr->id = ISP_BLK_AI_PRO_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_AI_SCENE_BCHS_PARAM:
		param_data_ptr->data_ptr = &ai_ptr->bchs_cur;
		param_data_ptr->data_size = sizeof(ai_ptr->bchs_cur);
		break;
	case ISP_PM_BLK_AI_SCENE_HSV_PARAM:
		param_data_ptr->data_ptr = &ai_ptr->hsv_cur;
		param_data_ptr->data_size = sizeof(ai_ptr->hsv_cur);
		break;
	case ISP_PM_BLK_AI_SCENE_EE_PARAM:
		param_data_ptr->data_ptr = &ai_ptr->ee_cur;
		param_data_ptr->data_size = sizeof(ai_ptr->ee_cur);
		break;
	case ISP_PM_BLK_AI_SCENE_SMOOTH:
		param_data_ptr->data_ptr = (void *)&ai_ptr->smooth_frame_ai_cur;
		param_data_ptr->data_size = sizeof(ai_ptr->smooth_frame_ai_cur);
		break;

	default:
		break;
	}

	return rtn;
}
