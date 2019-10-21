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
#define LOG_TAG "isp_blk_ygamc"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_yuv_ygamma_init(void *dst_gamc_param, void *src_gamc_param, void *param1, void *param_ptr2)
{
	cmr_u32 j = 0;
	cmr_u32 index = 0;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *dst_ptr = (struct isp_yuv_ygamma_param *)dst_gamc_param;
	struct sensor_y_gamma_param *src_ptr = (struct sensor_y_gamma_param *)src_gamc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_point *final_points;
	UNUSED(param_ptr2);

	memcpy(dst_ptr->curve_tab, src_ptr->curve_tab, sizeof(dst_ptr->curve_tab));
	memcpy(dst_ptr->specialeffect_tab, src_ptr->specialeffect, sizeof(dst_ptr->specialeffect_tab));

	dst_ptr->cur_idx = src_ptr->cur_idx;
	dst_ptr->cur.bypass = header_ptr->bypass;
	index = src_ptr->cur_idx;

	final_points = &dst_ptr->curve_tab[index].points[0];
	for (j = 0; j < ISP_YUV_GAMMA_NUM - 1; j++) {
		dst_ptr->cur.gain[j] = (final_points[j * 2].y + final_points[j * 2 + 1].y) >> 1;
	}
	dst_ptr->cur.gain[ISP_YUV_GAMMA_NUM - 1] = final_points[SENSOR_GAMMA_POINT_NUM - 1].y;

	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_yuv_ygamma_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *ygamma_ptr = (struct isp_yuv_ygamma_param *)gamc_param;
	struct isp_pm_block_header *gamc_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	gamc_header_ptr->is_update = ISP_ZERO;

	switch (cmd) {
	case ISP_PM_BLK_YGAMMA_BYPSS:
		ygamma_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		gamc_header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_YGAMMA:
		{
			cmr_u32 index = *((cmr_u32 *) param_ptr0);
			cmr_u32 j;
			struct isp_yuv_ygamma_param *dst_ptr = ygamma_ptr;
			struct isp_point *final_points;

			if (dst_ptr->cur.bypass)
				return ISP_SUCCESS;

			ygamma_ptr->cur_idx_weight.x0 = index;
			ygamma_ptr->cur_idx_weight.x1 = index;
			ygamma_ptr->cur_idx_weight.weight0 = 256;
			ygamma_ptr->cur_idx_weight.weight1 = 0;
			final_points = &dst_ptr->curve_tab[index].points[0];
			for (j = 0; j < ISP_YUV_GAMMA_NUM - 1; j++) {
				dst_ptr->cur.gain[j] = (final_points[j * 2].y + final_points[j * 2 + 1].y) >> 1;
			}
			dst_ptr->cur.gain[ISP_YUV_GAMMA_NUM - 1] = final_points[SENSOR_GAMMA_POINT_NUM - 1].y;
			gamc_header_ptr->is_update = ISP_ONE;
		}
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_weight_value gamc_value = { {0}, {0} };
			struct isp_range val_range = { 0, 0 };
			cmr_u32 j;
			struct isp_yuv_ygamma_param *dst_ptr = ygamma_ptr;
			struct isp_point *final_points;

			if (block_result->update == 0 || gamc_header_ptr->bypass ) {
				ISP_LOGV("do not need update\n");
				return ISP_SUCCESS;
			}

			val_range.min = 0;
			val_range.max = SENSOR_GAMMA_NUM - 1;

			rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_WEIGHT_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("fail to check pm smart param !");
				return rtn;
			}

			weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
			gamc_value = *weight_value;

			gamc_value.weight[0] = gamc_value.weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
			gamc_value.weight[1] = SMART_WEIGHT_UNIT - gamc_value.weight[0];
			if ((gamc_value.value[0] != ygamma_ptr->cur_idx_weight.x0) ||
				(gamc_value.weight[0] != ygamma_ptr->cur_idx_weight.weight0)) {

				void *src[2] = { NULL };
				void *dst = NULL;
				src[0] = &ygamma_ptr->curve_tab[gamc_value.value[0]].points[0].x;
				src[1] = &ygamma_ptr->curve_tab[gamc_value.value[1]].points[0].x;
				dst = &ygamma_ptr->final_curve;

				rtn = isp_interp_data(dst, src, gamc_value.weight, SENSOR_GAMMA_POINT_NUM * 2, ISP_INTERP_UINT16);
				if (ISP_SUCCESS == rtn) {
					ygamma_ptr->cur_idx_weight.x0 = weight_value->value[0];
					ygamma_ptr->cur_idx_weight.x1 = weight_value->value[1];
					ygamma_ptr->cur_idx_weight.weight0 = weight_value->weight[0];
					ygamma_ptr->cur_idx_weight.weight1 = weight_value->weight[1];
					final_points = &ygamma_ptr->final_curve.points[0];
					for (j = 0; j < ISP_YUV_GAMMA_NUM - 1; j++) {
						dst_ptr->cur.gain[j] = (final_points[j * 2].y + final_points[j * 2 + 1].y) >> 1;
					}
					dst_ptr->cur.gain[ISP_YUV_GAMMA_NUM - 1] = final_points[SENSOR_GAMMA_POINT_NUM - 1].y;
				}
				gamc_header_ptr->is_update = ISP_ONE;
			}
		}
		break;

	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_yuv_ygamma_get_param(void *gamc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *gamc_ptr = (struct isp_yuv_ygamma_param *)gamc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &gamc_ptr->cur;
		param_data_ptr->data_size = sizeof(gamc_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}
