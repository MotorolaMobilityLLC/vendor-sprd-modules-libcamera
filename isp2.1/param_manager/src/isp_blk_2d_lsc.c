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
#include "isp_com.h"




 isp_s32 _pm_2d_lsc_init(void * dst_lnc_param,void * src_lnc_param,void * param1,void * param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	intptr_t addr = 0, index = 0;
	struct isp_size *img_size_ptr = (struct isp_size*)param2;
	struct isp_2d_lsc_param *dst_ptr = (struct isp_2d_lsc_param*)dst_lnc_param;
	struct sensor_2d_lsc_param *src_ptr = (struct sensor_2d_lsc_param*)src_lnc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param1;

	dst_ptr->tab_num = src_ptr->tab_num;
	for (i = 0; i < ISP_COLOR_TEMPRATURE_NUM ; ++i) {
	//yongheng.lu
		addr = (intptr_t)&(src_ptr->data_area) + src_ptr->map[i].offset;
		dst_ptr->map_tab[i].param_addr = (void*)addr;
		dst_ptr->map_tab[i].len = src_ptr->map[i].len;
		dst_ptr->map_tab[i].grid = src_ptr->map[i].grid;
		dst_ptr->map_tab[i].grid_mode = src_ptr->map[i].grid;
		dst_ptr->map_tab[i].grid_pitch = _pm_get_lens_grid_pitch(src_ptr->map[i].grid, img_size_ptr->w, ISP_ONE);

		dst_ptr->map_tab[i].gain_w = dst_ptr->map_tab[i].grid_pitch;
		dst_ptr->map_tab[i].gain_h = _pm_get_lens_grid_pitch(src_ptr->map[i].grid, img_size_ptr->h, ISP_ONE);
	}
	if ((PNULL != dst_ptr->final_lsc_param.data_ptr)\
		&& (dst_ptr->final_lsc_param.size < src_ptr->map[0].len)) {
		free(dst_ptr->final_lsc_param.data_ptr);
		dst_ptr->final_lsc_param.data_ptr = PNULL;
		dst_ptr->final_lsc_param.size = 0;
	}
	if (PNULL == dst_ptr->final_lsc_param.data_ptr) {
		dst_ptr->final_lsc_param.data_ptr = (void*)malloc(src_ptr->map[0].len);
		if (PNULL == dst_ptr->final_lsc_param.data_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("_pm_2d_lsc_init: malloc failed\n");
			return rtn;
		}
	}
	dst_ptr->tab_num = src_ptr->tab_num;
	dst_ptr->cur_index_info = src_ptr->cur_idx;
	index  = src_ptr->cur_idx.x0;
	dst_ptr->cur_index_info.weight0 = 0;
	dst_ptr->cur_index_info.x0 = 0;
	dst_ptr->final_lsc_param.size = src_ptr->map[index].len;
	memcpy((void*)dst_ptr->final_lsc_param.data_ptr, (void*)dst_ptr->map_tab[index].param_addr, dst_ptr->map_tab[index].len);
	dst_ptr->cur.buf_len = dst_ptr->final_lsc_param.size;
	dst_ptr->final_lsc_param.param_ptr = (void*)malloc(src_ptr->map[0].len);

	dst_ptr->tmp_ptr_a = (void*)malloc(src_ptr->map[0].len);
	dst_ptr->tmp_ptr_b = (void*)malloc(src_ptr->map[0].len);

#if __WORDSIZE == 64
	dst_ptr->cur.buf_addr[0] = (isp_uint)(dst_ptr->final_lsc_param.data_ptr) & 0xffffffff;
	dst_ptr->cur.buf_addr[1] = (isp_uint)(dst_ptr->final_lsc_param.data_ptr) >> 32;
#else
	dst_ptr->cur.buf_addr[0] = (isp_uint)(dst_ptr->final_lsc_param.data_ptr);
	dst_ptr->cur.buf_addr[1] = 0;
#endif
	dst_ptr->cur.slice_size.width = img_size_ptr->w;
	dst_ptr->cur.slice_size.height = img_size_ptr->h;
	dst_ptr->cur.grid_width = dst_ptr->map_tab[index].grid_mode;
	dst_ptr->cur.grid_x_num = _pm_get_lens_grid_pitch(dst_ptr->cur.grid_width, dst_ptr->cur.slice_size.width, ISP_ZERO);
	dst_ptr->cur.grid_y_num = _pm_get_lens_grid_pitch(dst_ptr->cur.grid_width, dst_ptr->cur.slice_size.height, ISP_ZERO);
	dst_ptr->cur.grid_num_t = (dst_ptr->cur.grid_x_num + 2) * (dst_ptr->cur.grid_y_num + 2);
	dst_ptr->cur.grid_pitch = dst_ptr->cur.grid_x_num + 2;
	dst_ptr->cur.endian = ISP_ONE;
	dst_ptr->resolution = *img_size_ptr;
	dst_ptr->is_init = ISP_ONE;

	dst_ptr->cur.bypass = header_ptr->bypass;
	header_ptr->is_update = ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;


	return rtn;
}

 isp_s32 _pm_2d_lsc_otp_active(struct sensor_2d_lsc_param *lsc_ptr, struct isp_cali_lsc_info *cali_lsc_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, j = 0, num = 0;
	isp_u32 buf_size = 0;
	isp_u32 ct_min = 0, ct_max = 0, ct = 0;
	isp_u16 *tmp_buf = PNULL;
	isp_u16 *data_addr = PNULL, *dst_ptr = PNULL;
	isp_u16* addr_array[ISP_CALIBRATION_MAX_LSC_NUM];
	isp_u32 is_print_log = _is_print_log();

	if (NULL == lsc_ptr || NULL == cali_lsc_ptr) {
		ISP_LOGE("invalid parameter");
		return ISP_ERROR;
	}

	if (is_print_log) {
		ISP_LOGI("calibration lsc map num=%d", cali_lsc_ptr->num);

		for (i=0; i<cali_lsc_ptr->num; i++) {
			ISP_LOGI("[%d], envi=%d, ct=%d, size(%d, %d), grid=%d, offset=%d, len=%d", i,
				cali_lsc_ptr->map[i].ct >> 16, cali_lsc_ptr->map[i].ct & 0xffff,
				cali_lsc_ptr->map[i].width, cali_lsc_ptr->map[i].height, cali_lsc_ptr->map[i].grid,
				cali_lsc_ptr->map[i].offset, cali_lsc_ptr->map[i].len);
		}

		ISP_LOGI("origin lsc map num=%d", lsc_ptr->tab_num);
		for (i=0; i<lsc_ptr->tab_num; i++) {
			ISP_LOGI("[%d], envi=%d, ct=%d, grid=%d, offset=%d, len=%d", i,
				lsc_ptr->map[i].envi, lsc_ptr->map[i].ct, lsc_ptr->map[i].grid, lsc_ptr->map[i].offset,
				lsc_ptr->map[i].len);
		}
	}

	for (i=0; i<cali_lsc_ptr->num; i++) {
		isp_u32 src_envi = cali_lsc_ptr->map[i].ct >> 16;
		isp_u32 src_ct = cali_lsc_ptr->map[i].ct & 0xffff;
		isp_u16 *src_data = (isp_u16 *)((isp_u8 *)&cali_lsc_ptr->data_area + cali_lsc_ptr->map[i].offset);
		isp_u32 src_data_size = cali_lsc_ptr->map[i].len;
		isp_u32 j = 0;
		isp_u32 dst_index = 0xfff;
		isp_u32 min_ct_diff = 0xffff;

		if (is_print_log)
			ISP_LOGI("%d: ------------------", i);

		for (j=0; j<lsc_ptr->tab_num; j++) {
			isp_u32 dst_envi = lsc_ptr->map[j].envi;
			isp_u32 dst_ct = lsc_ptr->map[j].ct;

			if (dst_envi == src_envi) {
				isp_u32 ct_diff = abs(dst_ct - src_ct);
				if (ct_diff < min_ct_diff) {
					min_ct_diff = ct_diff;
					dst_index = j;
					//ISP_LOGI("[%d] min ct diff=%d", j, min_ct_diff);
				}
			}
		}

		//find useful one
		if (min_ct_diff <= 1000 && dst_index < lsc_ptr->tab_num) {
			struct isp_size src_size = {0, 0};
			struct isp_size dst_size = {0, 0};

			if (is_print_log)
				ISP_LOGI("suitable lsc find! min index = %d, min ct diff=%d", dst_index, min_ct_diff);

			src_size.w = cali_lsc_ptr->map[dst_index].width;
			src_size.h = cali_lsc_ptr->map[dst_index].height;
			dst_size.w = lsc_ptr->map[dst_index].width;
			dst_size.h = lsc_ptr->map[dst_index].height;
			isp_u16 *dst_data = (isp_u16 *)((isp_u8 *)&lsc_ptr->data_area +  lsc_ptr->map[dst_index].offset);
			isp_u32 dst_data_size = lsc_ptr->map[dst_index].len;

			if (src_size.w == dst_size.w && src_size.h == dst_size.h
				&& src_data_size == dst_data_size) {
				memcpy(dst_data, src_data, dst_data_size);
				if (is_print_log)
					ISP_LOGI("size is the same, just copy!");
			} else {
				/*need scaling*/
				//isp_scaling_lsc_gain(dst_data, src_data, &dst_size, &src_size);
				if (is_print_log)
					ISP_LOGI("src size=%dX%d, dst size=%dX%d, need scaling", src_size.w, src_size.h, dst_size.w, dst_size.h);
			}
		}

	}

	return rtn;
}

 isp_s32 _pm_2d_lsc_set_param(void *lnc_param, isp_u32 cmd, void* param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *dst_lnc_ptr = (struct isp_2d_lsc_param*)lnc_param;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_LSC_BYPASS:
	{
		dst_lnc_ptr->cur.bypass = *((isp_u32*)param_ptr0);
		lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
		dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
	}
	break;

	case ISP_PM_BLK_LSC_MEM_ADDR:
	{
		uint16_t* plsc = param_ptr0;
		memcpy((void*)dst_lnc_ptr->final_lsc_param.data_ptr, param_ptr0, dst_lnc_ptr->final_lsc_param.size);

#if __WORDSIZE == 64
	dst_lnc_ptr->cur.buf_addr[0] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr) & 0xffffffff;
	dst_lnc_ptr->cur.buf_addr[1] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr) >> 32;
#else
	dst_lnc_ptr->cur.buf_addr[0] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr);
	dst_lnc_ptr->cur.buf_addr[1] = 0;
#endif
	lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
	dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
	}
	break;

	case ISP_PM_BLK_LSC_VALIDATE:
		if (0 != *((isp_u32*)param_ptr0)) {
		lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
		} else {
			lnc_header_ptr->is_update &= (~ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE);
		}
		dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value *weight_value = NULL;
		struct isp_range val_range = {0, 0};
		struct isp_weight_value lnc_value = {{0}, {0}};
		void *src_lsc_ptr0 = NULL;
		void *src_lsc_ptr1 = NULL;
		void *dst_lsc_ptr = NULL;
		isp_u32 index = 0;

		if (block_result->update == 0)
		{
			return rtn;
		}

		val_range.min = 0;
		val_range.max = ISP_COLOR_TEMPRATURE_NUM - 1;
		rtn = _pm_check_smart_param(block_result, &val_range, 1,
					ISP_SMART_Y_TYPE_WEIGHT_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
			return rtn;
		}

		weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;

		struct isp_weight_value *bv_value = &weight_value[0];
		struct isp_weight_value* ct_value[2] = {&weight_value[1], &weight_value[2]};

		void* ct_result[2] = {dst_lnc_ptr->tmp_ptr_a, dst_lnc_ptr->tmp_ptr_b};
		int i;
		for (i=0; i<2; i++)
		{
			void *src[2] = {NULL};
			src[0] = (void*)dst_lnc_ptr->map_tab[ct_value[i]->value[0]].param_addr;
			src[1] = (void*)dst_lnc_ptr->map_tab[ct_value[i]->value[1]].param_addr;

			isp_u16 weight[2] = {0};
			weight[0] = ct_value[i]->weight[0];
			weight[1] = ct_value[i]->weight[1];
			weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
			weight[1] = SMART_WEIGHT_UNIT -weight[0];

			isp_u32 data_num = dst_lnc_ptr->cur.buf_len / sizeof(isp_u16);
			void *dst = ct_result[i];
			isp_interp_data(dst, src, (isp_u16*)weight, data_num, ISP_INTERP_UINT16);
		}

		void *src[2] = {NULL};
		src[0] = ct_result[0];
		src[1] = ct_result[1];

		isp_u16 weight[2] = {0};
		weight[0] = bv_value->weight[0];
		weight[1] = bv_value->weight[1];
		weight[0] = weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
		weight[1] = SMART_WEIGHT_UNIT -weight[0];

		isp_u32 data_num = dst_lnc_ptr->cur.buf_len / sizeof(isp_u16);
		void *dst = NULL;
		#if __WORDSIZE == 64
			dst = (void*)((isp_uint)dst_lnc_ptr->cur.buf_addr[1]<<32 | dst_lnc_ptr->cur.buf_addr[0]);
		#else
			dst = (void*)(dst_lnc_ptr->cur.buf_addr[0]);
		#endif
		isp_interp_data(dst, src, (isp_u16*)weight, data_num, ISP_INTERP_UINT16);


		lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
		dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
		memcpy(dst_lnc_ptr->final_lsc_param.param_ptr, dst, dst_lnc_ptr->cur.buf_len);

	}
	break;

	case ISP_PM_BLK_LSC_OTP:
	{
		void *src[2] = {NULL};
		void *dst = NULL;
		isp_u32 data_num;
		isp_u16 weight[2] = {0};
		struct sensor_2d_lsc_param *lsc_ptr = (struct sensor_2d_lsc_param*)lnc_header_ptr->absolute_addr;
		struct isp_cali_lsc_info *cali_lsc_ptr = (struct isp_cali_lsc_info*)param_ptr0;

		rtn = _pm_2d_lsc_otp_active(lsc_ptr, cali_lsc_ptr);

		if (ISP_ONE == dst_lnc_ptr->is_init) {
			src[0] = (void*)dst_lnc_ptr->map_tab[dst_lnc_ptr->cur_index_info.x0].param_addr;
			src[1] = (void*)dst_lnc_ptr->map_tab[dst_lnc_ptr->cur_index_info.x1].param_addr;
			dst = (void*)dst_lnc_ptr->final_lsc_param.data_ptr;
			data_num = dst_lnc_ptr->cur.buf_len / sizeof(isp_u16);

			weight[0] = dst_lnc_ptr->cur_index_info.weight0;
			weight[1] = dst_lnc_ptr->cur_index_info.weight1;

			rtn = isp_interp_data(dst, src, (isp_u16*)weight, data_num, ISP_INTERP_UINT16);
			if (ISP_SUCCESS == rtn) {
				lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
				dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
			}
		}
	}
	break;

		case ISP_PM_BLK_LSC_INFO:
		{
			lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
			dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
			ISP_LOGI("ISP_PM_BLK_LSC_INFO");

			{
				uint16_t *ptr = NULL;
			#if __WORDSIZE == 64
				ptr = (void*)((isp_uint)dst_lnc_ptr->cur.buf_addr[1]<<32 | dst_lnc_ptr->cur.buf_addr[0]);
			#else
				ptr = (void*)(dst_lnc_ptr->cur.buf_addr[0]);
			#endif
				ISP_LOGI("lsc[0]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *ptr, *(ptr+1), *(ptr+2), *(ptr+3));
				ISP_LOGI("lsc[1]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7));
			}
		}
		break;

		default:
		break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, lnc_header_ptr->is_update,
				dst_lnc_ptr->cur_index_info.x0, dst_lnc_ptr->cur_index_info.x1,
				dst_lnc_ptr->cur_index_info.weight0, dst_lnc_ptr->cur_index_info.weight1);


	return rtn;
}

 isp_s32 _pm_2d_lsc_get_param(void *lnc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	struct isp_2d_lsc_param *lnc_ptr = (struct isp_2d_lsc_param*)lnc_param;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_2D_LSC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&lnc_ptr->cur;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur);
		*update_flag &= (~(ISP_PM_BLK_LSC_UPDATE_MASK_PARAM|ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE));
		lnc_ptr->update_flag = *update_flag;
	break;

	case ISP_PM_BLK_LSC_BYPASS:
		param_data_ptr->data_ptr = (void*)&lnc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur.bypass);
	break;

	case ISP_PM_BLK_LSC_VALIDATE:
		param_data_ptr->data_ptr = PNULL;
		param_data_ptr->data_size = 0;
		param_data_ptr->user_data[1] = lnc_ptr->update_flag;
		*update_flag &= (~ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE);
		lnc_ptr->update_flag = *update_flag;
	break;

	case ISP_PM_BLK_LSC_INFO:
		lnc_ptr->lsc_info.cur_idx = lnc_ptr->cur_index_info;
		lnc_ptr->lsc_info.gain_w = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_w;
		lnc_ptr->lsc_info.gain_h = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_h;
		lnc_ptr->lsc_info.grid = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].grid;
		lnc_ptr->lsc_info.data_ptr = lnc_ptr->final_lsc_param.data_ptr;
		lnc_ptr->lsc_info.len = lnc_ptr->final_lsc_param.size;
		lnc_ptr->lsc_info.param_ptr = lnc_ptr->final_lsc_param.param_ptr;
		param_data_ptr->data_ptr = (void*)&lnc_ptr->lsc_info;
		param_data_ptr->data_size = sizeof(lnc_ptr->lsc_info);
	break;

	case ISP_PM_BLK_LSC_GET_LSCTAB:
		param_data_ptr->data_ptr= (void*)lnc_ptr;
		param_data_ptr->data_size = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_w * lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_h * 4;
		//ALOGE("print_lsc_tab map_tab[0~8]:address = 0x%x", lnc_ptr); //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621
	break;

	default:
	break;
	}


	return rtn;
}

 isp_s32 _pm_2d_lsc_deinit(void *lnc_param)
{
	isp_u32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *lsc_param_ptr = (struct isp_2d_lsc_param*)lnc_param;

	if (lsc_param_ptr->final_lsc_param.data_ptr) {
		free (lsc_param_ptr->final_lsc_param.data_ptr);
		lsc_param_ptr->final_lsc_param.data_ptr = PNULL;
		lsc_param_ptr->final_lsc_param.size = 0;
	}

	if (lsc_param_ptr->final_lsc_param.param_ptr) {
		free (lsc_param_ptr->final_lsc_param.param_ptr);
		lsc_param_ptr->final_lsc_param.param_ptr = PNULL;
	}


	if (lsc_param_ptr->tmp_ptr_a) {
		free(lsc_param_ptr->tmp_ptr_a);
		lsc_param_ptr->tmp_ptr_a = PNULL;
	}

	if (lsc_param_ptr->tmp_ptr_b) {
		free(lsc_param_ptr->tmp_ptr_b);
		lsc_param_ptr->tmp_ptr_b = PNULL;
	}

	return rtn;
}
