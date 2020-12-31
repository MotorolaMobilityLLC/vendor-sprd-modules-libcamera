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
#define LOG_TAG "isp_blk_2d_lsc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_2d_lsc_init(void *dst_lnc_param, void *src_lnc_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 max_len = 0;
	intptr_t addr = 0, index = 0;
	struct isp_size *img_size_ptr = (struct isp_size *)param2;
	struct isp_2d_lsc_param *dst_ptr = (struct isp_2d_lsc_param *)dst_lnc_param;
	struct sensor_2d_lsc_param *src_ptr = (struct sensor_2d_lsc_param *)src_lnc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;

	ISP_LOGD("src: %p,  tab_num=%d, image size %d %d, hdr %p\n", src_lnc_param,
		src_ptr->tab_num, img_size_ptr->w, img_size_ptr->h, dst_ptr);

	dst_ptr->tab_num = src_ptr->tab_num;
	for (i = 0; i < ISP_COLOR_TEMPRATURE_NUM; ++i) {
		addr = (intptr_t) & (src_ptr->tab_info.lsc_2d_map) + src_ptr->tab_info.lsc_2d_info[i].lsc_2d_offset;
		dst_ptr->map_tab[i].param_addr = (void *)addr;
		dst_ptr->map_tab[i].len = src_ptr->tab_info.lsc_2d_info[i].lsc_2d_len;
		dst_ptr->map_tab[i].grid = src_ptr->tab_info.lsc_2d_info[i].lsc_2d_map_info.grid;
		dst_ptr->map_tab[i].gain_w = _pm_get_lens_grid_pitch(dst_ptr->map_tab[i].grid, img_size_ptr->w, ISP_ONE);
		dst_ptr->map_tab[i].gain_h = _pm_get_lens_grid_pitch(dst_ptr->map_tab[i].grid, img_size_ptr->h, ISP_ONE);

		if (dst_ptr->map_tab[i].len == 0) {
			ISP_LOGE("map_tab len is 0 for idx: %d", i);
			dst_ptr->map_tab[i].param_addr = NULL;
		}
		max_len = (max_len < dst_ptr->map_tab[i].len) ? dst_ptr->map_tab[i].len : max_len;
		ISP_LOGV("%d, %p, %d,  %d, %d, %d\n",
			i, dst_ptr->map_tab[i].param_addr,
			dst_ptr->map_tab[i].len, dst_ptr->map_tab[i].grid,
			dst_ptr->map_tab[i].gain_w, dst_ptr->map_tab[i].gain_h);
	}

	if (max_len == 0) {
		rtn = ISP_ERROR;
		ISP_LOGE("no map tab for isp 2d_lsc!");
		return rtn;
	}

	if (dst_ptr->final_lsc_param.size < max_len) {
		if (NULL != dst_ptr->final_lsc_param.data_ptr) {
			free(dst_ptr->final_lsc_param.data_ptr);
			dst_ptr->final_lsc_param.data_ptr = NULL;
		}
		if (NULL != dst_ptr->final_lsc_param.param_ptr) {
			free(dst_ptr->final_lsc_param.param_ptr);
			dst_ptr->final_lsc_param.param_ptr = NULL;
		}
		dst_ptr->final_lsc_param.size = 0;
	}

	if (NULL == dst_ptr->final_lsc_param.data_ptr) {
		dst_ptr->final_lsc_param.data_ptr = (void *)malloc(max_len);
		if (NULL == dst_ptr->final_lsc_param.data_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to malloc\n");
			return rtn;
		}
	}

	if (NULL == dst_ptr->final_lsc_param.param_ptr) {
		dst_ptr->final_lsc_param.param_ptr = (void *)malloc(max_len);
		if (NULL == dst_ptr->final_lsc_param.param_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("fail to malloc\n");
			return rtn;
		}
	}

	dst_ptr->tab_num = src_ptr->tab_num;
	dst_ptr->cur_index_info = src_ptr->cur_idx;
	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_index_info.weight0 = 0;
	dst_ptr->cur_index_info.x0 = 0;
	dst_ptr->final_lsc_param.size = src_ptr->tab_info.lsc_2d_info[index].lsc_2d_len;
	memcpy((void *)dst_ptr->final_lsc_param.data_ptr,
			(void *)dst_ptr->map_tab[index].param_addr,
			dst_ptr->map_tab[index].len);

	memset((void *)dst_ptr->weight_tab, 0, sizeof(dst_ptr->weight_tab));
	_pm_generate_bicubic_weight_table(dst_ptr->weight_tab, dst_ptr->map_tab[index].grid);

	dst_ptr->cur.grid_tab_addr = (cmr_u64)dst_ptr->final_lsc_param.data_ptr;
	dst_ptr->cur.gridtab_len = dst_ptr->map_tab[index].gain_w * dst_ptr->map_tab[index].gain_h * 4 * sizeof(cmr_u16);

	dst_ptr->cur.weight_tab_addr = (cmr_u64)dst_ptr->weight_tab;
	dst_ptr->cur.weight_num =  (dst_ptr->map_tab[index].grid / 2 + 1) * 3 * sizeof(cmr_s16);
	dst_ptr->cur.grid_width = dst_ptr->map_tab[index].grid;

	/* grid_x_num = Ceiling(image_width/2/grid_width, 1) +1 +2; */
	/* grid_y_num = Ceiling(image_height/2/grid_width, 1) +1 +2; */
	i = dst_ptr->cur.grid_width<<1;
	dst_ptr->cur.grid_x_num = (img_size_ptr->w + i - 1) /i + 1 + 2;
	dst_ptr->cur.grid_y_num = (img_size_ptr->h + i - 1) /i + 1 + 2;
	dst_ptr->cur.grid_num_t = dst_ptr->cur.grid_x_num * dst_ptr->cur.grid_y_num;
	dst_ptr->resolution = *img_size_ptr;

	dst_ptr->lsc_info.cur_idx = dst_ptr->cur_index_info;
	dst_ptr->lsc_info.gain_w = dst_ptr->map_tab[dst_ptr->lsc_info.cur_idx.x0].gain_w;
	dst_ptr->lsc_info.gain_h = dst_ptr->map_tab[dst_ptr->lsc_info.cur_idx.x0].gain_h;
	dst_ptr->lsc_info.grid = dst_ptr->map_tab[dst_ptr->lsc_info.cur_idx.x0].grid;
	dst_ptr->lsc_info.data_ptr = dst_ptr->final_lsc_param.data_ptr;
	dst_ptr->lsc_info.len = dst_ptr->final_lsc_param.size;
	dst_ptr->lsc_info.param_ptr = dst_ptr->final_lsc_param.param_ptr;

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.update_all = 1;
	header_ptr->is_update = ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;

	ISP_LOGD("image size %d %d, grid %d, %d, gain_w %d, gain_h %d, (x, y) = (%d %d),  hdr %p\n",
		dst_ptr->resolution.w, dst_ptr->resolution.h,
		dst_ptr->cur.grid_width, dst_ptr->cur.gridtab_len,
		dst_ptr->lsc_info.gain_w, dst_ptr->lsc_info.gain_h,
		dst_ptr->cur.grid_x_num, dst_ptr->cur.grid_y_num, dst_ptr);

	return rtn;
}

cmr_s32 _pm_2d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *dst_lnc_ptr = (struct isp_2d_lsc_param *)lnc_param;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_LSC_MEM_ADDR:
		{
			cmr_u16 *ptr = NULL;

			memcpy((void *)dst_lnc_ptr->final_lsc_param.data_ptr, param_ptr0, dst_lnc_ptr->final_lsc_param.size);
			dst_lnc_ptr->cur.grid_tab_addr = (cmr_u64)dst_lnc_ptr->final_lsc_param.data_ptr;

			lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
			dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;

			ptr = (cmr_u16 *)dst_lnc_ptr->final_lsc_param.data_ptr;

			ISP_LOGV("lsc[0]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3));
			ISP_LOGV("lsc[1]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *(ptr + 4), *(ptr + 5), *(ptr + 6), *(ptr + 7));
		}
		break;

	case ISP_PM_BLK_LSC_UPDATE_GRID:
		{
			cmr_u32* adaptive_size_info = (cmr_u32 *) param_ptr0;
			cmr_u32 cur_resolution_w = adaptive_size_info[0];
			cmr_u32 cur_resolution_h = adaptive_size_info[1];
			cmr_u32 lsc_grid = adaptive_size_info[2];
			struct isp_2d_lsc_param *dst_ptr = dst_lnc_ptr;

			ISP_LOGD("LSC_NORMAL ISP_PM_BLK_LSC_UPDATE_GRID, new_grid=%d, orig=%d, hdr %p\n",
						lsc_grid, dst_lnc_ptr->cur.grid_width, dst_ptr);

			if (lsc_grid != dst_lnc_ptr->cur.grid_width ||
				dst_ptr->resolution.w != cur_resolution_w ||
				dst_ptr->resolution.h != cur_resolution_h) {
				ISP_LOGD("LSC_NORMAL ISP_PM_BLK_LSC_UPDATE_GRID, new_resolution[%d,%d], orig[%d,%d]",
						cur_resolution_w, cur_resolution_h,
						dst_ptr->resolution.w, dst_ptr->resolution.h);

				memset((void *)dst_ptr->weight_tab, 0, sizeof(dst_ptr->weight_tab));
				_pm_generate_bicubic_weight_table(dst_ptr->weight_tab, lsc_grid);

				dst_ptr->cur.weight_tab_addr = (cmr_u64)dst_ptr->weight_tab;

				dst_ptr->lsc_info.grid = lsc_grid;
				dst_ptr->resolution.w = cur_resolution_w;
				dst_ptr->resolution.h = cur_resolution_h;
				dst_ptr->lsc_info.gain_w = _pm_get_lens_grid_pitch(lsc_grid, dst_ptr->resolution.w, ISP_ONE);
				dst_ptr->lsc_info.gain_h = _pm_get_lens_grid_pitch(lsc_grid, dst_ptr->resolution.h, ISP_ONE);

				dst_ptr->cur.grid_width = lsc_grid;
				dst_ptr->cur.grid_x_num = _pm_get_lens_grid_pitch(lsc_grid, dst_ptr->resolution.w, ISP_ZERO)+2;
				dst_ptr->cur.grid_y_num = _pm_get_lens_grid_pitch(lsc_grid, dst_ptr->resolution.h, ISP_ZERO)+2;
				dst_ptr->cur.grid_num_t = dst_ptr->cur.grid_x_num * dst_ptr->cur.grid_y_num ;
				dst_ptr->cur.gridtab_len = dst_ptr->lsc_info.gain_w * dst_ptr->lsc_info.gain_h * 4 * sizeof(cmr_u16);
				dst_ptr->cur.weight_num = (lsc_grid / 2 + 1) * 3 * sizeof(cmr_s16);

			}

			dst_ptr->cur.update_all = 1;
			lnc_header_ptr->is_update = ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;

			ISP_LOGD("image size %d %d, grid %d %d, gain_w %d, gain_h %d, (x, y) = (%d %d),  hdr %p\n",
				dst_ptr->resolution.w, dst_ptr->resolution.h,
				dst_ptr->cur.grid_width, dst_ptr->cur.gridtab_len,
				dst_ptr->lsc_info.gain_w, dst_ptr->lsc_info.gain_h,
				dst_ptr->cur.grid_x_num, dst_ptr->cur.grid_y_num, dst_ptr);
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_2d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	struct isp_2d_lsc_param *lnc_ptr = (struct isp_2d_lsc_param *)lnc_param;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&lnc_ptr->cur;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur);
		*update_flag &= (~(ISP_PM_BLK_LSC_UPDATE_MASK_PARAM | ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE));
		lnc_ptr->update_flag = *update_flag;
		break;

	case ISP_PM_BLK_LSC_INFO:
		param_data_ptr->data_ptr = (void *)&lnc_ptr->lsc_info;
		param_data_ptr->data_size = sizeof(lnc_ptr->lsc_info);
		break;

	case ISP_PM_BLK_LSC_GET_LSCTAB:
		param_data_ptr->data_ptr = (void *)lnc_ptr;
		param_data_ptr->data_size = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_w
						* lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_h * 4;
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_2d_lsc_deinit(void *lnc_param)
{
	cmr_u32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *lsc_param_ptr = (struct isp_2d_lsc_param *)lnc_param;

	if (lsc_param_ptr->final_lsc_param.data_ptr) {
		free(lsc_param_ptr->final_lsc_param.data_ptr);
		lsc_param_ptr->final_lsc_param.data_ptr = PNULL;
		lsc_param_ptr->final_lsc_param.size = 0;
	}

	if (lsc_param_ptr->final_lsc_param.param_ptr) {
		free(lsc_param_ptr->final_lsc_param.param_ptr);
		lsc_param_ptr->final_lsc_param.param_ptr = PNULL;
	}

	return rtn;
}
