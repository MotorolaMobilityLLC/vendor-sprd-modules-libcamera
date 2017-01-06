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

#include "isp_blocks_cfg.h"





isp_s32 _pm_hsv_init(void *dst_hsv_param, void *src_hsv_param, void* param1, void* param2)
{
	isp_u32 i = 0;
	isp_u32 j = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	intptr_t addr = 0;
	isp_u32 *buf_ptr = PNULL;
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
		addr = (intptr_t)&src_ptr->data_area + src_ptr->map[i].offset;
		dst_ptr->map[i].data_ptr = (void*)addr;
	}
	for (i=0; i<MAX_SPECIALEFFECT_NUM; i++) {
		dst_ptr->specialeffect_tab[i].size = src_ptr->specialeffect[i].size;
		addr = (intptr_t)(&src_ptr->specialeffect_data_area + src_ptr->specialeffect[i].offset);
		dst_ptr->specialeffect_tab[i].data_ptr = (void*)addr;
	}

	if (PNULL == dst_ptr->final_map.data_ptr) {
		dst_ptr->final_map.data_ptr = (void*)malloc(src_ptr->map[index].size);
		if (PNULL == dst_ptr->final_map.data_ptr) {
			ISP_LOGE("malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memcpy((void*)dst_ptr->final_map.data_ptr, dst_ptr->map[index].data_ptr, dst_ptr->map[index].size);
	buf_ptr = (isp_u32*)dst_ptr->final_map.data_ptr;
	dst_ptr->final_map.size = dst_ptr->map[index].size;
	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.buf_sel = 0x00;
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 4; j++) {
			dst_ptr->cur.region_info[0].s_curve[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_s_curve[j];
			dst_ptr->cur.region_info[0].v_curve[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_v_curve[j];
		}
		dst_ptr->cur.region_info[0].hrange_left[i] = src_ptr->sensor_hsv_cfg[i].hsv_hrange_left;
		dst_ptr->cur.region_info[0].hrange_right[i] = src_ptr->sensor_hsv_cfg[i].hsv_hrange_right;

	}
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 4; j++) {
			dst_ptr->cur.region_info[1].s_curve[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_s_curve[j];
			dst_ptr->cur.region_info[1].v_curve[i][j] = src_ptr->sensor_hsv_cfg[i].hsv_v_curve[j];
		}
		dst_ptr->cur.region_info[1].hrange_left[i] = src_ptr->sensor_hsv_cfg[i].hsv_hrange_left;
		dst_ptr->cur.region_info[1].hrange_right[i] = src_ptr->sensor_hsv_cfg[i].hsv_hrange_right;

	}
	#if __WORDSIZE == 64
	dst_ptr->cur.data_ptr[0] = (isp_uint)(dst_ptr->final_map.data_ptr) & 0xffffffff;
	dst_ptr->cur.data_ptr[1] = (isp_uint)(dst_ptr->final_map.data_ptr) >> 32;
	#else
	dst_ptr->cur.data_ptr[0] = (isp_uint)(dst_ptr->final_map.data_ptr);
	dst_ptr->cur.data_ptr[1] = 0;
	#endif
	dst_ptr->cur.size = dst_ptr->final_map.size;
	dst_ptr->cur.buf_sel = 0x0;
	dst_ptr->cur.bypass = header_ptr->bypass;

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

isp_s32 _pm_hsv_set_param(void *hsv_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *dst_hsv_ptr = (struct isp_hsv_param *)hsv_param;
	struct isp_pm_block_header *hsv_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value *weight_value = NULL;
		struct isp_range val_range = {0, 0};
		struct isp_weight_value hsv_value = {{0}, {0}};

		hsv_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;
		rtn = _pm_check_smart_param(block_result, &val_range, 1,
					ISP_SMART_Y_TYPE_WEIGHT_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
			return rtn;
		}

		weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
		hsv_value = *weight_value;

		ISP_LOGI("ISP_SMART: value=(%d, %d), weight=(%d, %d)", hsv_value.value[0],
				hsv_value.value[1], hsv_value.weight[0], hsv_value.weight[1]);

		hsv_value.weight[0] = hsv_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
							* (SMART_WEIGHT_UNIT / 16);
		hsv_value.weight[1] = SMART_WEIGHT_UNIT - hsv_value.weight[0];

		if (hsv_value.weight[0] != dst_hsv_ptr->cur_idx.weight0
			|| hsv_value.value[0] != dst_hsv_ptr->cur_idx.x0) {
			void *src[2] = {NULL};
			void *dst = NULL;
			isp_u32 data_num = 0;
			isp_u32 index = 0;

			index = hsv_value.value[0];
			src[0] = (void*)dst_hsv_ptr->map[hsv_value.value[0]].data_ptr;
			src[1] = (void*)dst_hsv_ptr->map[hsv_value.value[1]].data_ptr;
			dst = (void*)dst_hsv_ptr->final_map.data_ptr;
			data_num = dst_hsv_ptr->final_map.size / sizeof(isp_u32);

			rtn = isp_interp_data(dst, src, hsv_value.weight, data_num, ISP_INTERP_UINT20);
			if (ISP_SUCCESS == rtn) {
				dst_hsv_ptr->cur_idx.x0 = hsv_value.value[0];
				dst_hsv_ptr->cur_idx.x1 = hsv_value.value[1];
				dst_hsv_ptr->cur_idx.weight0 = hsv_value.weight[0];
				dst_hsv_ptr->cur_idx.weight1 = hsv_value.weight[1];
				hsv_header_ptr->is_update = 1;
			}
		}
	}
	break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (0 == idx) {
			dst_hsv_ptr->cur.buf_sel = 0;
			dst_hsv_ptr->cur.size = dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].size;
		#if __WORDSIZE == 64
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr) & 0xffffffff;
			dst_hsv_ptr->cur.data_ptr[1] = (isp_uint)(dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr) >> 32;
		#else
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr);
			dst_hsv_ptr->cur.data_ptr[1] = 0;
		#endif
		} else {
			dst_hsv_ptr->cur.buf_sel = 0;
			dst_hsv_ptr->cur.size = dst_hsv_ptr->specialeffect_tab[idx].size;
		#if __WORDSIZE == 64
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->specialeffect_tab[idx].data_ptr) & 0xffffffff;
			dst_hsv_ptr->cur.data_ptr[1] = (isp_uint)(dst_hsv_ptr->specialeffect_tab[idx].data_ptr) >> 32;
		#else
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->specialeffect_tab[idx].data_ptr);
			dst_hsv_ptr->cur.data_ptr[1] = 0;
		#endif
		}
		hsv_header_ptr->is_update = ISP_ZERO;
	}
	break;

	default:
	break;
	}

	ISP_LOGE("ISP_SMART: cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, hsv_header_ptr->is_update,
				dst_hsv_ptr->cur_idx.x0, dst_hsv_ptr->cur_idx.x1,
				dst_hsv_ptr->cur_idx.weight0, dst_hsv_ptr->cur_idx.weight1);

	return rtn;
}

 isp_s32 _pm_hsv_get_param(void *hsv_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *hsv_ptr = (struct isp_hsv_param *)hsv_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_HSV;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&hsv_ptr->cur;
		param_data_ptr->data_size = sizeof(hsv_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

 isp_s32 _pm_hsv_deinit(void* hsv_param)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *hsv_ptr = (struct isp_hsv_param *)hsv_param;

	if (PNULL != hsv_ptr->final_map.data_ptr) {
		free (hsv_ptr->final_map.data_ptr);
		hsv_ptr->final_map.data_ptr = PNULL;
		hsv_ptr->final_map.size = 0;
	}

	return rtn;
}
