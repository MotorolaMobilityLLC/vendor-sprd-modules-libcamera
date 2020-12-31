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
	cmr_u32 index = 0, cpy_size;
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_uint addr = 0, tmp_addr = 0, addr1 = 0;
	cmr_uint base, end;
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

	base = (cmr_uint)src_ptr;
	end = base + header_ptr->size;
	ISP_LOGD("data size %d, addr range (%p ~ %p)\n", header_ptr->size, (void *)base, (void *)end);

	for (i = 0; i < SENSOR_HSV_NUM; i++) {
		dst_ptr->map[i].size = src_ptr->map[i].size;
		addr = (cmr_uint) & src_ptr->data_area + src_ptr->map[i].offset;
		dst_ptr->map[i].data_ptr = (void *)addr;

		addr1 = addr + src_ptr->map[i].size;
		ISP_LOGV("index %d, offset %d, size %d,  cur %p, end %p\n", i,
			src_ptr->map[i].offset, src_ptr->map[i].size, (void *)addr, (void *)addr1);

		if ((addr > base) && (addr1 <= end))
			continue;

		ISP_LOGE("out of range, index %d, offset %d, size %d,  cur %p, end %p\n",
			i, src_ptr->map[i].offset, src_ptr->map[i].size, (void *)addr, (void *)addr1);
		goto exit;
	}
	addr += src_ptr->map[SENSOR_HSV_NUM - 1].size;
	specialeffect = (struct isp_data_bin_info *)addr;
	addr += sizeof(struct isp_data_bin_info) * MAX_SPECIALEFFECT_NUM;
	for (i = 0; i < MAX_SPECIALEFFECT_NUM; i++) {
		dst_ptr->specialeffect_tab[i].size = specialeffect[i].size;
		tmp_addr = (cmr_uint) (addr + specialeffect[i].offset);
		dst_ptr->specialeffect_tab[i].data_ptr = (void *)tmp_addr;

		addr1 = tmp_addr + specialeffect[i].size;
		ISP_LOGV("index %d, offset %d, size %d,  cur %p, end %p\n",
			i, specialeffect[i].offset, specialeffect[i].size, (void *)tmp_addr, (void *)addr1);

		if ((tmp_addr > base) && (addr1 <= end))
			continue;

		ISP_LOGE("out of range, index %d, offset %d, size %d,  cur %p, end %p\n",
			i, specialeffect[i].offset, specialeffect[i].size, (void *)tmp_addr, (void *)addr1);
		goto exit;
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

	cpy_size = (cmr_u32)sizeof(dst_ptr->cur.d.hsv_table);
	if (cpy_size < dst_ptr->map[index].size)
		ISP_LOGE("error: hsv table size %d is smaller than target %d\n", cpy_size, dst_ptr->map[index].size);
	else
		cpy_size =  dst_ptr->map[index].size;

	dst_ptr->final_map.data_ptr = &dst_ptr->cur.d.hsv_table[0];
	dst_ptr->final_map.size = cpy_size;
	memcpy((void *)dst_ptr->final_map.data_ptr, dst_ptr->map[index].data_ptr, cpy_size);

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
	ISP_LOGD("hsv table addr 0x%lx, size %d, size dst %d\n", (cmr_uint)dst_ptr->cur.d.hsv_table,
		dst_ptr->cur.size,  (cmr_u32)sizeof(dst_ptr->cur.d.hsv_table));

	header_ptr->is_update = ISP_ONE;
	return 0;

exit:
	ISP_LOGE("discard hsv block.\n");
	dst_ptr->cur.bypass = 1;
	header_ptr->bypass = 1;
	header_ptr->is_update = ISP_ZERO;
	dst_ptr->final_map.data_ptr = PNULL;
	dst_ptr->final_map.size = 0;
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
			cmr_s32 hsv_level = -1;
			void * src_map[2] = { NULL, NULL };
			cmr_u32 *dst;
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

			ISP_LOGV("ISP_SMART, ai_scene %d,  weit (%d %d %d %d), weit (%d %d %d %d), weit (%d %d %d %d)\n",
				block_result->ai_scene_id,
				ct_value[0]->value[0], ct_value[0]->value[1], ct_value[0]->weight[0], ct_value[0]->weight[1],
				ct_value[1]->value[0], ct_value[1]->value[1], ct_value[1]->weight[0], ct_value[1]->weight[1],
				bv_value->value[0], bv_value->value[1], bv_value->weight[0], bv_value->weight[1]);

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

			if (hsv_level != -1 && hsv_level < SENSOR_HSV_NUM) {
				src_map[0] = dst_hsv_ptr->map[hsv_level].data_ptr;
				if (src_map[0] != NULL) {
					cmr_u32 *temp = (cmr_u32 *)src_map[0];
					memcpy((void *)dst_hsv_ptr->final_map.data_ptr, src_map[0], dst_hsv_ptr->final_map.size);
					ISP_LOGV("hsv level %d, val %05x %05x %05x %05x %05x %05x %05x %05x\n",
						hsv_level, temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]);
				} else {
					ISP_LOGE("hsv level %d data table is null\n", hsv_level);
				}
			} else {
				weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
				bv_value = &weight_value[0];
				ct_value[0] = &weight_value[1];
				ct_value[1] = &weight_value[2];

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

				if (dst) {
					cmr_u32 *temp = dst;

					ISP_LOGV("hsv val %05x %05x %05x %05x %05x %05x %05x %05x\n",
						temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7]);
				}
			}

			hsv_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
		{
			cmr_u32 idx = *((cmr_u32 *) param_ptr0);
			cmr_u32 cpy_size;
			struct isp_data_info *hsv_data;
			if (hsv_header_ptr->bypass) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}
			if (0 == idx)
				hsv_data = &dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0];
			else
				hsv_data = &dst_hsv_ptr->specialeffect_tab[idx];

			cpy_size = (cmr_u32)sizeof(dst_hsv_ptr->cur.d.hsv_table);
			if (cpy_size < hsv_data->size)
				ISP_LOGE("error: hsv table size %d is smaller than target %d\n", cpy_size, hsv_data->size);
			else
				cpy_size =  hsv_data->size;
			dst_hsv_ptr->cur.size = cpy_size;
			memcpy((void *)dst_hsv_ptr->final_map.data_ptr, hsv_data->data_ptr, cpy_size);
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

	dst_ptr->final_map.data_ptr = PNULL;
	dst_ptr->final_map.size = 0;
	for (j = 0; j < 2; j++) {
		if (dst_ptr->ct_result[j]) {
			free(dst_ptr->ct_result[j]);
			dst_ptr->ct_result[j] = PNULL;
		}
	}
	return rtn;
}
