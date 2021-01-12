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
#define LOG_TAG "isp_blk_rgb_gtm"
#include "isp_blocks_cfg.h"
#include <math.h>

#include <math.h>


#define GTM_HIST_BIN_NUM             128

enum PIX_BITS
{
	PIX_BITS_8  = 8,
	PIX_BITS_10 = 10,
	PIX_BITS_12 = 12,
	PIX_BITS_14 = 14,
	PIX_BITS_UNKNOWN,
} ;

double gtm_log2(double x)
{
	double out;
	out = (double)(log(x)/log((double)(2.0)));
	return out;
}

static void cal_gtm_hist(cmr_s32 in_bit_depth, struct dcam_dev_rgb_gtm_block_info *gtm)
{
	cmr_s32 i;
	cmr_s32 bin0 = 2;
	cmr_s32 length1, length2, length3;
	cmr_u16 expo_ratio = (1<<(in_bit_depth-10))*16;

	float d1, d2, q;
	float bin_f[GTM_HIST_BIN_NUM];
	length1 = 18;
	length2 = 30;
	length3 = 128 - (length1 + length2);
	d1 = (float)sqrt((float)(expo_ratio)/16.0f);
	d2 = 2*(expo_ratio/16.0f);
	bin_f[0] = (float)bin0;
	for(i=1; i<length1; i++)
	{
		bin_f[i] = bin_f[i-1] + d1;
	}
	for(i=length1; i<(length1+length2); i++)
	{
		bin_f[i] = bin_f[i-1] + d2;
	}
	q = (float)pow(2, (float)gtm_log2((1024*(expo_ratio/16.0f)-1)/bin_f[length1 + length2-1])/length3);
	for(i = (length1 + length2); i < GTM_HIST_BIN_NUM; i++)
	{
		bin_f[i] = bin_f[i-1]*q;
	}
	for(i = 0; i < GTM_HIST_BIN_NUM; i++)
	{
		gtm->tm_hist_xpts[i] = (cmr_s32)(bin_f[i] + 0.5);
	}
}

cmr_s32 _pm_rgb_gtm_init(void *dst_rgb_gtm_param, void *src_rgb_gtm_param, void *param1, void *param2)
{
	cmr_u32 i, j, k = 0;
	cmr_u32 index = 0;
	float e = 2.718281828f;
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gtm_param *dst_ptr = (struct isp_rgb_gtm_param *)dst_rgb_gtm_param;
	struct sensor_rgb_gtm_param *src_ptr = (struct sensor_rgb_gtm_param *)src_rgb_gtm_param;
	struct isp_pm_block_header *rgb_gtm_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.gtm_mod_en = !rgb_gtm_header_ptr->bypass;
	index = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x0 = src_ptr->cur_idx.x0;
	dst_ptr->cur_idx.x1 = src_ptr->cur_idx.x1;
	dst_ptr->cur_idx.weight0 = src_ptr->cur_idx.weight0;
	dst_ptr->cur_idx.weight1 = src_ptr->cur_idx.weight1;

	if (dst_ptr->cur.gtm_mod_en) {
		for (i = 0; i < SENSOR_RGB_GTM_NUM; i++) {
			dst_ptr->rgb_gtm_param[i].gtm_hist_stat_bypass = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_bypass;

			dst_ptr->rgb_gtm_param[i].gtm_rgb2y_mode = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_rgb2y.rgb2y_mode;
			dst_ptr->rgb_gtm_param[i].tm_rgb2y_r_coeff = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_rgb2y.rgb2y_r_coeff;
			dst_ptr->rgb_gtm_param[i].tm_rgb2y_g_coeff = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_rgb2y.rgb2y_g_coeff;
			dst_ptr->rgb_gtm_param[i].tm_rgb2y_b_coeff = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_rgb2y.rgb2y_b_coeff;

			dst_ptr->rgb_gtm_param[i].max_percentile_16bit = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.percentile_16bit.max_percentile_16bit;
			dst_ptr->rgb_gtm_param[i].min_percentile_16bit = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.percentile_16bit.min_percentile_16bit;

			dst_ptr->rgb_gtm_param[i].gtm_target_norm_coeff = (cmr_u32)(src_ptr->rgb_gtm_param[i].rgb_gtm_stat.target_norm.target_norm_coeff*(1<<14) + 0.5f);
			dst_ptr->rgb_gtm_param[i].gtm_target_norm = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.target_norm.target_norm;
			dst_ptr->rgb_gtm_param[i].gtm_target_norm_setting_mode = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.target_norm.target_norm_set_mode;

			dst_ptr->rgb_gtm_param[i].gtm_imgkey_setting_value = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_image.image_key;
			dst_ptr->rgb_gtm_param[i].gtm_imgkey_setting_mode = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_image.image_key_set_mode;

			dst_ptr->rgb_gtm_param[i].gtm_ymax_diff_thr = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_video.luma_sm_Ymax_diff_thr;
			dst_ptr->rgb_gtm_param[i].gtm_yavg_diff_thr = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_video.luma_sm_Yavg_diff_thr;
			dst_ptr->rgb_gtm_param[i].gtm_pre_ymin_weight = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_video.pre_Ymin_weight;
			dst_ptr->rgb_gtm_param[i].gtm_cur_ymin_weight = src_ptr->rgb_gtm_param[i].rgb_gtm_stat.gtm_stat_video.cur_Ymin_weight;

			dst_ptr->rgb_gtm_param[i].gtm_map_bypass = src_ptr->rgb_gtm_param[i].rgb_gtm_map.bypass;
			dst_ptr->rgb_gtm_param[i].gtm_map_video_mode = src_ptr->rgb_gtm_param[i].rgb_gtm_map.map_video_mode;
			dst_ptr->rgb_gtm_param[i].gtm_map_bilateral_sigma_d = (cmr_u32) (src_ptr->rgb_gtm_param[i].rgb_gtm_map.bilateral_sigma_d);
			dst_ptr->rgb_gtm_param[i].gtm_map_bilateral_sigma_r = (cmr_u32) (src_ptr->rgb_gtm_param[i].rgb_gtm_map.bilateral_sigma_r);

			for(j = 0; j < 7; j++){
				for(k = 0 ;k < 7; k++){
					dst_ptr->rgb_gtm_param[i].gtm_map_dist[j][k] = src_ptr->rgb_gtm_param[i].rgb_gtm_map.dist[j][k];
				}
			}
			for(j = 0; j < 19; j++){
				src_ptr->rgb_gtm_param[i].rgb_gtm_map.dis_weight[j] =
					(cmr_u32) (pow (e, -j /(2 * src_ptr->rgb_gtm_param[i].rgb_gtm_map.bilateral_sigma_d)) * ((1 << 9) - 1));
				dst_ptr->rgb_gtm_param[i].gtm_map_dis_weight[j] = src_ptr->rgb_gtm_param[i].rgb_gtm_map.dis_weight[j];
			}
			for(k = 0; k < 61; k++){
				src_ptr->rgb_gtm_param[i].rgb_gtm_map.range_weight[k] =
					(cmr_u32) (pow (e, -(k * k) /(2 * src_ptr->rgb_gtm_param[i].rgb_gtm_map.bilateral_sigma_r)) * ((1 << 9) - 1));
				dst_ptr->rgb_gtm_param[i].gtm_map_range_weight[k] = src_ptr->rgb_gtm_param[i].rgb_gtm_map.range_weight[k];
			}
		}

		dst_ptr->cur.gtm_hist_stat_bypass = dst_ptr->rgb_gtm_param[index].gtm_hist_stat_bypass;
		dst_ptr->cur.gtm_rgb2y_mode= dst_ptr->rgb_gtm_param[index].gtm_rgb2y_mode;
		dst_ptr->cur.tm_rgb2y_r_coeff = dst_ptr->rgb_gtm_param[index].tm_rgb2y_r_coeff;
		dst_ptr->cur.tm_rgb2y_g_coeff = dst_ptr->rgb_gtm_param[index].tm_rgb2y_g_coeff;
		dst_ptr->cur.tm_rgb2y_b_coeff = dst_ptr->rgb_gtm_param[index].tm_rgb2y_b_coeff;
		dst_ptr->cur.gtm_max_per = dst_ptr->rgb_gtm_param[index].max_percentile_16bit;
		dst_ptr->cur.gtm_min_per = dst_ptr->rgb_gtm_param[index].min_percentile_16bit;
		dst_ptr->cur.gtm_target_norm_coeff = dst_ptr->rgb_gtm_param[index].gtm_target_norm_coeff;
		dst_ptr->cur.gtm_target_norm = dst_ptr->rgb_gtm_param[index].gtm_target_norm;
		dst_ptr->cur.gtm_target_norm_setting_mode = dst_ptr->rgb_gtm_param[index].gtm_target_norm_setting_mode;
		dst_ptr->cur.gtm_imgkey_setting_value = dst_ptr->rgb_gtm_param[index].gtm_imgkey_setting_value;
		dst_ptr->cur.gtm_imgkey_setting_mode = dst_ptr->rgb_gtm_param[index].gtm_imgkey_setting_mode;
		dst_ptr->cur.gtm_ymax_diff_thr = dst_ptr->rgb_gtm_param[index].gtm_ymax_diff_thr;
		dst_ptr->cur.gtm_yavg_diff_thr = dst_ptr->rgb_gtm_param[index].gtm_yavg_diff_thr;
		dst_ptr->cur.gtm_pre_ymin_weight = dst_ptr->rgb_gtm_param[index].gtm_pre_ymin_weight;
		dst_ptr->cur.gtm_cur_ymin_weight = dst_ptr->rgb_gtm_param[index].gtm_cur_ymin_weight;
		dst_ptr->cur.gtm_map_bypass = dst_ptr->rgb_gtm_param[index].gtm_map_bypass;
		dst_ptr->cur.gtm_map_video_mode = dst_ptr->rgb_gtm_param[index].gtm_map_video_mode;
		dst_ptr->cur.gtm_map_bilateral_sigma_d = dst_ptr->rgb_gtm_param[index].gtm_map_bilateral_sigma_d;
		dst_ptr->cur.gtm_map_bilateral_sigma_r = dst_ptr->rgb_gtm_param[index].gtm_map_bilateral_sigma_r;

		for(j = 0; j < 7; j++){
			for(k = 0 ;k < 7; k++){
				dst_ptr->cur.tm_filter_dist_c[j * 7 + k] = dst_ptr->rgb_gtm_param[index].gtm_map_dist[j][k];
			}
		}
		for(j = 0; j < 19; j++){
			dst_ptr->cur.tm_filter_distw_c[j] = dst_ptr->rgb_gtm_param[index].gtm_map_dis_weight[j];
		}
		for(k = 0; k < 61; k++){
			dst_ptr->cur.tm_filter_rangw_c[k] = dst_ptr->rgb_gtm_param[index].gtm_map_range_weight[k];
		}
		cal_gtm_hist(PIX_BITS_14, &(dst_ptr->cur));
	} else {
		dst_ptr->cur.gtm_map_bypass = 1;
		dst_ptr->cur.gtm_hist_stat_bypass = 1;
	}

	if (dst_ptr->cur.gtm_map_bypass && dst_ptr->cur.gtm_hist_stat_bypass)
		dst_ptr->cur.gtm_mod_en = 0;

	rgb_gtm_header_ptr->is_update = ISP_ONE;

	ISP_LOGD("rgb_gtm bypass %d en %d, map %d stat %d\n",
		rgb_gtm_header_ptr->bypass, dst_ptr->cur.gtm_mod_en,
		dst_ptr->cur.gtm_map_bypass, dst_ptr->cur.gtm_hist_stat_bypass);

	return rtn;
}

cmr_s32 _pm_rgb_gtm_set_param(void *rgb_gtm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 j, k;
	struct isp_rgb_gtm_param *dst_ptr = (struct isp_rgb_gtm_param *)rgb_gtm_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
		{
			cmr_u32 data_num;
			cmr_u16 weight[2] = { 0, 0 };
			void * src1[2] = { NULL, NULL };
			void *src2[2] = {NULL, NULL};
			void *src3[2] = {NULL, NULL};
			void *dst1 = NULL;
			void *dst2 = NULL;
			void *dst3 = NULL;
			cmr_u32 index = 0;
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_weight_value *weight_value = NULL;
			struct isp_range val_range = { 0, 0 };
			struct isp_weight_value *bv_value;

			if (0 == block_result->update) {
				ISP_LOGI("do not need update\n");
				return ISP_SUCCESS;
			}

			if (header_ptr->bypass) {
				dst_ptr->cur.gtm_mod_en = 0;
				dst_ptr->cur.gtm_map_bypass = 1;
				dst_ptr->cur.gtm_hist_stat_bypass = 1;
				header_ptr->is_update = ISP_ONE;
				ISP_LOGV("bypass all\n");
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

			dst_ptr->cur.gtm_hist_stat_bypass = dst_ptr->rgb_gtm_param[index].gtm_hist_stat_bypass;
			dst_ptr->cur.gtm_rgb2y_mode= dst_ptr->rgb_gtm_param[index].gtm_rgb2y_mode;
			dst_ptr->cur.tm_rgb2y_r_coeff = dst_ptr->rgb_gtm_param[index].tm_rgb2y_r_coeff;
			dst_ptr->cur.tm_rgb2y_g_coeff = dst_ptr->rgb_gtm_param[index].tm_rgb2y_g_coeff;
			dst_ptr->cur.tm_rgb2y_b_coeff = dst_ptr->rgb_gtm_param[index].tm_rgb2y_b_coeff;
			dst_ptr->cur.gtm_max_per = dst_ptr->rgb_gtm_param[index].max_percentile_16bit;
			dst_ptr->cur.gtm_min_per = dst_ptr->rgb_gtm_param[index].min_percentile_16bit;
			dst_ptr->cur.gtm_target_norm_coeff = dst_ptr->rgb_gtm_param[index].gtm_target_norm_coeff;
			dst_ptr->cur.gtm_target_norm = dst_ptr->rgb_gtm_param[index].gtm_target_norm;
			dst_ptr->cur.gtm_target_norm_setting_mode = dst_ptr->rgb_gtm_param[index].gtm_target_norm_setting_mode;
			dst_ptr->cur.gtm_imgkey_setting_value = dst_ptr->rgb_gtm_param[index].gtm_imgkey_setting_value;
			dst_ptr->cur.gtm_imgkey_setting_mode = dst_ptr->rgb_gtm_param[index].gtm_imgkey_setting_mode;
			dst_ptr->cur.gtm_ymax_diff_thr = dst_ptr->rgb_gtm_param[index].gtm_ymax_diff_thr;
			dst_ptr->cur.gtm_yavg_diff_thr = dst_ptr->rgb_gtm_param[index].gtm_yavg_diff_thr;
			dst_ptr->cur.gtm_pre_ymin_weight = dst_ptr->rgb_gtm_param[index].gtm_pre_ymin_weight;
			dst_ptr->cur.gtm_cur_ymin_weight = dst_ptr->rgb_gtm_param[index].gtm_cur_ymin_weight;
			dst_ptr->cur.gtm_map_bypass = dst_ptr->rgb_gtm_param[index].gtm_map_bypass;
			dst_ptr->cur.gtm_map_video_mode = dst_ptr->rgb_gtm_param[index].gtm_map_video_mode;
			dst_ptr->cur.gtm_map_bilateral_sigma_d = dst_ptr->rgb_gtm_param[index].gtm_map_bilateral_sigma_d;
			dst_ptr->cur.gtm_map_bilateral_sigma_r = dst_ptr->rgb_gtm_param[index].gtm_map_bilateral_sigma_r;

			for(j = 0; j < 7; j++){
				for(k = 0 ;k < 7; k++){
					dst_ptr->cur.tm_filter_dist_c[j * 7 + k] = dst_ptr->rgb_gtm_param[index].gtm_map_dist[j][k];
				}
			}
			for(j = 0; j < 19; j++){
				dst_ptr->cur.tm_filter_distw_c[j] = dst_ptr->rgb_gtm_param[index].gtm_map_dis_weight[j];
			}
			for(k = 0; k < 61; k++){
				dst_ptr->cur.tm_filter_rangw_c[k] = dst_ptr->rgb_gtm_param[index].gtm_map_range_weight[k];
			}

			cal_gtm_hist(PIX_BITS_14, &(dst_ptr->cur));
			data_num = 1;
			if (!dst_ptr->cur.gtm_target_norm_setting_mode) {
				dst1 = &dst_ptr->cur.gtm_target_norm;
				src1[0] = (void *)&dst_ptr->rgb_gtm_param[bv_value->value[0]].gtm_target_norm;
				src1[1] = (void *)&dst_ptr->rgb_gtm_param[bv_value->value[1]].gtm_target_norm;
				isp_interp_data((void *)dst1, src1 , weight , data_num , ISP_INTERP_UINT16);
			}
			if (!dst_ptr->cur.gtm_imgkey_setting_mode) {
				dst2 = &dst_ptr->cur.gtm_imgkey_setting_value;
				src1[0] = (void *)&dst_ptr->rgb_gtm_param[bv_value->value[0]].gtm_imgkey_setting_value;
				src1[1] = (void *)&dst_ptr->rgb_gtm_param[bv_value->value[1]].gtm_imgkey_setting_value;
				isp_interp_data((void *)dst2, src2 , weight , data_num , ISP_INTERP_UINT16);
			}

			dst3 = &dst_ptr->cur.gtm_target_norm_coeff;
			src3[0] = (void *)&dst_ptr->rgb_gtm_param[bv_value->value[0]].gtm_target_norm_coeff;
			src3[1] = (void *)&dst_ptr->rgb_gtm_param[bv_value->value[1]].gtm_target_norm_coeff;
			isp_interp_data((void *)dst3, src3 , weight , data_num , ISP_INTERP_UINT16);

			if (dst_ptr->cur.gtm_map_bypass && dst_ptr->cur.gtm_hist_stat_bypass)
				dst_ptr->cur.gtm_mod_en = 0;
			else
				dst_ptr->cur.gtm_mod_en = 1;

			ISP_LOGV("SMART: w (%d %d) v (%d %d); weight (%d %d), index %d; en %d map %d stat %d\n",
				bv_value->weight[0], bv_value->weight[1], bv_value->value[0], bv_value->value[1],
				weight[0], weight[1], index, dst_ptr->cur.gtm_mod_en,
				dst_ptr->cur.gtm_map_bypass, dst_ptr->cur.gtm_hist_stat_bypass);

			header_ptr->is_update = ISP_ONE;
		}
		break;

	default:

		break;
	}

	return rtn;
}

cmr_s32 _pm_rgb_gtm_get_param(void *rgb_gtm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gtm_param *gtm_ptr = (struct isp_rgb_gtm_param *)rgb_gtm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_GTM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&gtm_ptr->cur;
		param_data_ptr->data_size = sizeof(gtm_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_GTM_STATUS:
		param_data_ptr->data_ptr = (void *)&gtm_ptr->cur.gtm_mod_en;
		param_data_ptr->data_size = sizeof(gtm_ptr->cur.gtm_mod_en);
		ISP_LOGD("rgb_gtm on %d\n", gtm_ptr->cur.gtm_mod_en);
		break;

	default:
		break;
	}

	return rtn;
}