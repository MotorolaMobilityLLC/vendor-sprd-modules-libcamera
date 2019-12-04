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
#define LOG_TAG "isp_blk_yuv_ltm"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_yuv_ltm_init(void *dst_yuv_ltm_param, void *src_yuv_ltm_param, void *param1, void *param2)
{
	cmr_u32 i = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ltm_param *dst_ptr = (struct isp_yuv_ltm_param *)dst_yuv_ltm_param;
	struct isp_pm_block_header *ltm_header_ptr = (struct isp_pm_block_header *)param1;
	struct sensor_yuv_ltm_param *src_ptr = (struct sensor_yuv_ltm_param*)src_yuv_ltm_param;
	UNUSED(param2);

	dst_ptr->cur.ltm_map.bypass = ltm_header_ptr->bypass;
	dst_ptr->cur.ltm_stat.bypass = ltm_header_ptr->bypass;

	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx.weight1;
	if (!ltm_header_ptr->bypass){
		for ( i = 0; i < SENSOR_YUV_LTM_NUM; i++) {
			dst_ptr->ltm_param[i].ltm_map_bypass = src_ptr->yuv_ltm_param[i].yuv_ltm_map.bypass;
			dst_ptr->ltm_param[i].ltm_map_video_mode = src_ptr->yuv_ltm_param[i].yuv_ltm_map.ltm_map_video_mode;

			dst_ptr->ltm_param[i].ltm_stat_bypass = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.bypass;
			dst_ptr->ltm_param[i].ltm_stat_strength = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.strength;
			dst_ptr->ltm_param[i].tile_num_auto = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.tile_num_auto;
			dst_ptr->ltm_param[i].region_est_en = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.region_est_en;

			dst_ptr->ltm_param[i].tile_num_x = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.tile_num.tile_num_x;
			dst_ptr->ltm_param[i].tile_num_y = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.tile_num.tile_num_y;

			dst_ptr->ltm_param[i].text_point_thres = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.ltm_text.text_point_thres;
			dst_ptr->ltm_param[i].textture_proporion = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.ltm_text.textture_proporion;
			dst_ptr->ltm_param[i].text_point_alpha = src_ptr->yuv_ltm_param[i].yuv_ltm_stat.ltm_text.text_point_alpha;
		}
		dst_ptr->cur.ltm_map.bypass = dst_ptr->ltm_param[index].ltm_map_bypass;
		dst_ptr->cur.ltm_map.ltm_map_video_mode = dst_ptr->ltm_param[index].ltm_map_video_mode;
		dst_ptr->cur.ltm_stat.bypass = dst_ptr->ltm_param[index].ltm_stat_bypass;
		dst_ptr->cur.ltm_stat.strength = dst_ptr->ltm_param[index].ltm_stat_strength;
		dst_ptr->cur.ltm_stat.tile_num_auto = dst_ptr->ltm_param[index].tile_num_auto;
		dst_ptr->cur.ltm_stat.region_est_en = dst_ptr->ltm_param[index].region_est_en;
		dst_ptr->cur.ltm_stat.tile_num.tile_num_x = dst_ptr->ltm_param[index].tile_num_x;
		dst_ptr->cur.ltm_stat.tile_num.tile_num_y = dst_ptr->ltm_param[index].tile_num_y;
		dst_ptr->cur.ltm_stat.text_point_thres = dst_ptr->ltm_param[index].text_point_thres;
		dst_ptr->cur.ltm_stat.ltm_text.textture_proporion = dst_ptr->ltm_param[index].textture_proporion;
		ltm_rgb_text_thres_init(dst_ptr->cur.ltm_stat.text_point_thres,
			dst_ptr->ltm_param[index].text_point_alpha, dst_ptr->cur.ltm_stat.ltm_hist_table);
	}

	ltm_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_yuv_ltm_set_param(void *yuv_ltm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ltm_param *dst_ptr = (struct isp_yuv_ltm_param *)yuv_ltm_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			cmr_u32 index = 0;
			cmr_u32 data_num;
			cmr_u16 weight[2] = { 0, 0 };
			void *src1[2] = { NULL, NULL };
			void *src2[2] = {NULL, NULL};
			void *src3[2] = { NULL, NULL };
			void *dst1 = NULL;
			void *dst2 = NULL;
			void *dst3 = NULL;
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_range val_range = { 0, 0 };
			struct isp_weight_value *bv_value;

			if (0 == block_result->update || header_ptr->bypass) {
				ISP_LOGV("do not need update\n");
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
			if (weight[0] > weight[1])
				index = bv_value->value[0];
			else
				index = bv_value->value[1];
			dst_ptr->cur.ltm_map.bypass = dst_ptr->ltm_param[index].ltm_map_bypass;
			dst_ptr->cur.ltm_map.ltm_map_video_mode = dst_ptr->ltm_param[index].ltm_map_video_mode;
			dst_ptr->cur.ltm_stat.bypass = dst_ptr->ltm_param[index].ltm_stat_bypass;
			dst_ptr->cur.ltm_stat.tile_num_auto = dst_ptr->ltm_param[index].tile_num_auto;
			dst_ptr->cur.ltm_stat.region_est_en = dst_ptr->ltm_param[index].region_est_en;
			dst_ptr->cur.ltm_stat.tile_num.tile_num_x = dst_ptr->ltm_param[index].tile_num_x;
			dst_ptr->cur.ltm_stat.tile_num.tile_num_y = dst_ptr->ltm_param[index].tile_num_y;
			dst_ptr->cur.ltm_stat.text_point_thres = dst_ptr->ltm_param[index].text_point_thres;
			dst_ptr->cur.ltm_stat.ltm_text.textture_proporion = dst_ptr->ltm_param[index].textture_proporion;

			data_num = 1;
			if (dst_ptr->cur.ltm_stat.region_est_en) {
				dst1 = &dst_ptr->cur.ltm_stat.text_point_thres;
				dst2 = &dst_ptr->cur.ltm_stat.ltm_text.textture_proporion;
				src1[0] = (void *)&dst_ptr->ltm_param[bv_value->value[0]].text_point_thres;
				src1[1] = (void *)&dst_ptr->ltm_param[bv_value->value[1]].text_point_thres;
				src2[0] = (void *)&dst_ptr->ltm_param[bv_value->value[0]].textture_proporion;
				src2[1] = (void *)&dst_ptr->ltm_param[bv_value->value[1]].textture_proporion;
				isp_interp_data((void *)dst1, src1 , weight , data_num , ISP_INTERP_UINT16);
				isp_interp_data((void *)dst2, src2 , weight , data_num , ISP_INTERP_UINT16);
			}

			ltm_rgb_text_thres_init(dst_ptr->cur.ltm_stat.text_point_thres,
				dst_ptr->ltm_param[index].text_point_alpha, dst_ptr->cur.ltm_stat.ltm_hist_table);

			dst3 = &dst_ptr->cur.ltm_stat.strength;
			src3[0] = (void *)&dst_ptr->ltm_param[bv_value->value[0]].ltm_stat_strength;
			src3[1] = (void *)&dst_ptr->ltm_param[bv_value->value[1]].ltm_stat_strength;
			isp_interp_data((void *)dst3, src3 , weight , data_num , ISP_INTERP_UINT16);

			header_ptr->is_update = ISP_ONE;
		}
		break;

	default:

		break;
	}

	return rtn;
}

cmr_s32 _pm_yuv_ltm_get_param(void *yuv_ltm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ltm_param *ltm_ptr = (struct isp_yuv_ltm_param *)yuv_ltm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&ltm_ptr->cur;
		param_data_ptr->data_size = sizeof(ltm_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
