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

cmr_s32 _pm_hsv_init(void *dst_hsv_param, void *src_hsv_param, void *param1, void *param2)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	intptr_t addr = 0, tmp_addr = 0;
	struct isp_data_bin_info *specialeffect = PNULL;
	struct isp_hsv_param *dst_ptr = (struct isp_hsv_param *)dst_hsv_param;
	struct sensor_hsv_param *src_ptr = (struct sensor_hsv_param *)src_hsv_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx.weight1;

	for (i = 0; i < SENSOR_HSV_NUM; i++) {
		dst_ptr->map[i].size = src_ptr->map[i].size;
		addr = (intptr_t) & src_ptr->data_area + src_ptr->map[i].offset;
		dst_ptr->map[i].data_ptr = (void *)addr;
	}
	addr += src_ptr->map[SENSOR_HSV_NUM - 1].size;
	specialeffect = (struct isp_data_bin_info *)addr;
	addr += sizeof(struct isp_data_bin_info) * MAX_SPECIALEFFECT_NUM;
	for (i = 0; i < MAX_SPECIALEFFECT_NUM; i++) {
		dst_ptr->specialeffect_tab[i].size = specialeffect[i].size;
		tmp_addr = (intptr_t) (addr + specialeffect[i].offset);
		dst_ptr->specialeffect_tab[i].data_ptr = (void *)tmp_addr;
	}

	if (PNULL == dst_ptr->final_map.data_ptr) {
		dst_ptr->final_map.data_ptr = (void *)malloc(src_ptr->map[index].size);
		if (PNULL == dst_ptr->final_map.data_ptr) {
			ISP_LOGE("fail to malloc  hsv map\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}

	for (i = 0; i < 2; i++) {
		if (PNULL == dst_ptr->ct_result[i]) {
			dst_ptr->ct_result[i] = (cmr_u32 *)malloc(src_ptr->map[index].size);
			if (PNULL == dst_ptr->ct_result[i]) {
				ISP_LOGE("fail to malloc hsv ct\n");
				rtn = ISP_ERROR;
				goto exit;
			}
		}
	}

	memcpy((void *)dst_ptr->final_map.data_ptr, dst_ptr->map[index].data_ptr, dst_ptr->map[index].size);
	dst_ptr->final_map.size = dst_ptr->map[index].size;

	dst_ptr->cur.bypass = header_ptr->bypass;
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
	dst_ptr->cur.size = dst_ptr->final_map.size;
	dst_ptr->cur.hsv_table_addr = (cmr_u64)dst_ptr->final_map.data_ptr;
	ISP_LOGV("hsv table addr 0x%lx\n", (cmr_uint)dst_ptr->cur.hsv_table_addr);

	header_ptr->is_update = ISP_ONE;
	return 0;

exit:
	if (dst_ptr->final_map.data_ptr) {
		free(dst_ptr->final_map.data_ptr);
		dst_ptr->final_map.data_ptr = PNULL;
		dst_ptr->final_map.size = 0;
	}
	for (j = 0; j < 2; j++) {
		if (dst_ptr->ct_result[j]) {
			free(dst_ptr->ct_result[j]);
			dst_ptr->ct_result[j] = PNULL;
		}
	}
	return rtn;
}

cmr_s32 _pm_hsv_set_param(void *hsv_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *dst_hsv_ptr = (struct isp_hsv_param *)hsv_param;
	struct isp_pm_block_header *hsv_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			cmr_u32 i;
			cmr_u32 data_num;
			cmr_u16 weight[2] = { 0, 0 };
			cmr_u32 hsv_level = 8;
			cmr_u32 text_coeff = 10;
			cmr_u32 pet_coeff = 8;
			void * src_map[2] = { NULL, NULL };
			cmr_u32 *dst;
			char prop[PROPERTY_VALUE_MAX];
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_range val_range = { 0, 0 };
			struct isp_weight_value *bv_value;
			struct isp_weight_value *ct_value[2];

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
			ct_value[0] = &weight_value[1];
			ct_value[1] = &weight_value[2];

			if (block_result->ai_scene_id == ISP_PM_AI_SCENE_FOLIAGE ||
				block_result->ai_scene_id == ISP_PM_AI_SCENE_SKY) {
				property_get("debug.isp.hsv.hsv_value.level", prop, "8");
				hsv_level = atoi(prop);
				for (i = 0; i < 2; i++) {
					ct_value[i]->value[0] = hsv_level;
					ct_value[i]->value[1] = hsv_level;
					ct_value[i]->weight[0] = 256;
					ct_value[i]->weight[1] = 0;
				}
			} else {
				property_get("debug.isp.hsv.hsv_value.tune", prop, "0");
				if (atoi(prop)) {
					for (i = 0; i < 2; i++) {
						ct_value[i]->value[0] = 0;
						ct_value[i]->value[1] = 0;
						ct_value[i]->weight[0] = 256;
						ct_value[i]->weight[1] = 0;
					}
				}
			}

			dst = (cmr_u32 *)dst_hsv_ptr->final_map.data_ptr;
			data_num = dst_hsv_ptr->final_map.size / sizeof(cmr_u32);
			for(i = 0; i < 2; i++) {
				src_map[0] = (void *)dst_hsv_ptr->map[ct_value[i]->value[0]].data_ptr;
				src_map[1] = (void *)dst_hsv_ptr->map[ct_value[i]->value[1]].data_ptr;
				weight[0] = ct_value[i]->weight[0];
				weight[1] = ct_value[i]->weight[1];
				weight[0] = weight[0]/(SMART_WEIGHT_UNIT/16) * (SMART_WEIGHT_UNIT/16);
				weight[1] = SMART_WEIGHT_UNIT - weight[0];
				isp_interp_data(dst_hsv_ptr->ct_result[i] , src_map , weight , data_num , ISP_INTERP_UINT20);
			}
			src_map[0] = dst_hsv_ptr->ct_result[0];
			src_map[1] = dst_hsv_ptr->ct_result[1];
			weight[0] = bv_value->weight[0];
			weight[1] = bv_value->weight[1];
			weight[0] = weight[0]/(SMART_WEIGHT_UNIT/16) * (SMART_WEIGHT_UNIT/16);
			weight[1] = SMART_WEIGHT_UNIT - weight[0];
			isp_interp_data((void *)dst , src_map , weight , data_num , ISP_INTERP_UINT20);

			property_get("debug.isp.hsv.text_coeff.val", prop, "10");
			text_coeff = atoi(prop);
			property_get("debug.isp.hsv.pet_coeff.val", prop, "8");
			pet_coeff = atoi(prop);

			ISP_LOGV("ai_scene_id = %d", block_result->ai_scene_id);
			switch (block_result->ai_scene_id) {
			case ISP_PM_AI_SCENE_TEXT:
				for (i = 0; i < data_num; i++) {
					cmr_u32 dst_val = *dst;
					cmr_u32 dst_val_h = dst_val & 0x1FF;
					cmr_u32 dst_val_s = (dst_val >> 9) & 0x7FF;
					dst_val_h = dst_val_h * 1;
					dst_val_s = dst_val_s * 10 / text_coeff;
					dst_val = (dst_val_h & 0x1FF) | ((dst_val_s & 0x7FF) << 9);
					*dst++ = dst_val;
				}
				break;

			case ISP_PM_AI_SCENE_PET:
				for (i = 0; i < data_num; i++) {
					cmr_u32 dst_val = *dst;
					cmr_u32 dst_val_h = dst_val & 0x1FF;
					cmr_u32 dst_val_s = (dst_val >> 9) & 0x7FF;
					dst_val_h = dst_val_h * 1;
					dst_val_s = dst_val_s * 10 / pet_coeff;
					dst_val = (dst_val_h & 0x1FF) | ((dst_val_s & 0x7FF) << 9);
					*dst++ = dst_val;
				}
				break;

			default:
				break;
			}

			hsv_header_ptr->is_update = ISP_ONE;

			ISP_LOGV("ISP_SMART: cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n",
				cmd, hsv_header_ptr->is_update, dst_hsv_ptr->cur_idx.x0, dst_hsv_ptr->cur_idx.x1,
				dst_hsv_ptr->cur_idx.weight0, dst_hsv_ptr->cur_idx.weight1);
		}
		break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			if (0 == idx) {
				dst_hsv_ptr->cur.size = dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].size;
				dst_hsv_ptr->cur.hsv_table_addr = (cmr_u64)dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr;
			} else {
				dst_hsv_ptr->cur.size = dst_hsv_ptr->specialeffect_tab[idx].size;
				dst_hsv_ptr->cur.hsv_table_addr = (cmr_u64)dst_hsv_ptr->specialeffect_tab[idx].data_ptr;
			}
			ISP_LOGV("hsv table addr 0x%lx\n", (cmr_uint)dst_hsv_ptr->cur.hsv_table_addr);
			hsv_header_ptr->is_update = ISP_ONE;
		}
		break;

	default:
		break;
	}


	return rtn;
}

cmr_s32 _pm_hsv_get_param(void *hsv_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *hsv_ptr = (struct isp_hsv_param *)hsv_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_HSV;
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

cmr_s32 _pm_hsv_deinit(void *hsv_param)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 j;
	struct isp_hsv_param *dst_ptr = (struct isp_hsv_param *)hsv_param;

	if (dst_ptr->final_map.data_ptr) {
		free(dst_ptr->final_map.data_ptr);
		dst_ptr->final_map.data_ptr = PNULL;
		dst_ptr->final_map.size = 0;
	}
	for (j = 0; j < 2; j++) {
		if (dst_ptr->ct_result[j]) {
			free(dst_ptr->ct_result[j]);
			dst_ptr->ct_result[j] = PNULL;
		}
	}
	return rtn;
}
