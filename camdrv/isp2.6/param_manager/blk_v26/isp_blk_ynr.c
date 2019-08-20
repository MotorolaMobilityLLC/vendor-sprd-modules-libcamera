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
#define LOG_TAG "isp_blk_ynr"
#include "isp_blocks_cfg.h"

static cmr_u32 _pm_ynr_convert_param(
	void *dst_param, cmr_u32 strength_level,
	cmr_u32 mode_flag, cmr_u32 scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)dst_param;
	struct sensor_ynr_level *ynr_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		ynr_param = (struct sensor_ynr_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		ynr_param = (struct sensor_ynr_level *)((cmr_u8 *) dst_ptr->param_ptr +
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_ynr_level));
	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);
	
	if (ynr_param != NULL) {
		dst_ptr->cur.bypass = ynr_param[strength_level].bypass;
		dst_ptr->cur.coef_model = ynr_param[strength_level].coef_mode;
		dst_ptr->cur.sal_enable = ynr_param[strength_level].ynr_sal_str.sal_enable;
		dst_ptr->cur.lum_thresh0 = ynr_param[strength_level].ynr_sal_str.lum_thresh[0];
		dst_ptr->cur.lum_thresh1 = ynr_param[strength_level].ynr_sal_str.lum_thresh[1];
		dst_ptr->cur.sal_offset0 = ynr_param[strength_level].ynr_sal_str.sal_offset[0];
		dst_ptr->cur.sal_offset1 = ynr_param[strength_level].ynr_sal_str.sal_offset[1];
		dst_ptr->cur.sal_offset2 = ynr_param[strength_level].ynr_sal_str.sal_offset[2];
		dst_ptr->cur.sal_offset3 = ynr_param[strength_level].ynr_sal_str.sal_offset[3];
		dst_ptr->cur.sal_offset4 = ynr_param[strength_level].ynr_sal_str.sal_offset[4];
		dst_ptr->cur.sal_offset5 = ynr_param[strength_level].ynr_sal_str.sal_offset[5];
		dst_ptr->cur.sal_offset6 = ynr_param[strength_level].ynr_sal_str.sal_offset[6];
		dst_ptr->cur.sal_offset7 = ynr_param[strength_level].ynr_sal_str.sal_offset[7];
		dst_ptr->cur.lut_thresh0 = ynr_param[strength_level].ynr_sal_str.sal_thresh[0];
		dst_ptr->cur.lut_thresh1 = ynr_param[strength_level].ynr_sal_str.sal_thresh[1];
		dst_ptr->cur.lut_thresh2 = ynr_param[strength_level].ynr_sal_str.sal_thresh[2];
		dst_ptr->cur.lut_thresh3 = ynr_param[strength_level].ynr_sal_str.sal_thresh[3];
		dst_ptr->cur.lut_thresh4 = ynr_param[strength_level].ynr_sal_str.sal_thresh[4];
		dst_ptr->cur.lut_thresh5 = ynr_param[strength_level].ynr_sal_str.sal_thresh[5];
		dst_ptr->cur.lut_thresh6 = ynr_param[strength_level].ynr_sal_str.sal_thresh[6];
		dst_ptr->cur.sal_nr_str0 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[0];
		dst_ptr->cur.sal_nr_str1 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[1];
		dst_ptr->cur.sal_nr_str2 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[2];
		dst_ptr->cur.sal_nr_str3 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[3];
		dst_ptr->cur.sal_nr_str4 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[4];
		dst_ptr->cur.sal_nr_str5 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[5];
		dst_ptr->cur.sal_nr_str6 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[6];
		dst_ptr->cur.sal_nr_str7 = ynr_param[strength_level].ynr_sal_str.sal_nr_str[7];

		dst_ptr->cur.l1_wv_nr_enable = ynr_param[strength_level].ynr_wv_parm[0].wv_nr_enable;

		dst_ptr->cur.l1_wv_thr1_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_thresh1;
		dst_ptr->cur.l1_wv_thr2_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_thresh2_n;
		dst_ptr->cur.l1_wv_ratio1_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_ratio1;
		dst_ptr->cur.l1_wv_ratio2_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_ratio2;
		dst_ptr->cur.l1_wv_thr_d1_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_thresh_d1;
		dst_ptr->cur.l1_wv_thr_d2_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_thresh_d2_n;
		dst_ptr->cur.l1_wv_ratio_d1_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_ratio_d1;
		dst_ptr->cur.l1_wv_ratio_d2_low = ynr_param[strength_level].ynr_wv_parm[0].low.wv_ratio_d2;
		dst_ptr->cur.l1_soft_offset_low = ynr_param[strength_level].ynr_wv_parm[0].low.soft_offset;
		dst_ptr->cur.l1_soft_offset_d_low = ynr_param[strength_level].ynr_wv_parm[0].low.soft_offsetd;

		dst_ptr->cur.l1_wv_thr1_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_thresh1;
		dst_ptr->cur.l1_wv_thr2_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_thresh2_n;
		dst_ptr->cur.l1_wv_ratio1_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_ratio1;
		dst_ptr->cur.l1_wv_ratio2_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_ratio2;
		dst_ptr->cur.l1_wv_thr_d1_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_thresh_d1;
		dst_ptr->cur.l1_wv_thr_d2_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_thresh_d2_n;
		dst_ptr->cur.l1_wv_ratio_d1_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_ratio_d1;
		dst_ptr->cur.l1_wv_ratio_d2_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.wv_ratio_d2;
		dst_ptr->cur.l1_soft_offset_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.soft_offset;
		dst_ptr->cur.l1_soft_offset_d_mid = ynr_param[strength_level].ynr_wv_parm[0].mid.soft_offsetd;

		dst_ptr->cur.l1_wv_thr1_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_thresh1;
		dst_ptr->cur.l1_wv_thr2_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_thresh2_n;
		dst_ptr->cur.l1_wv_ratio1_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_ratio1;
		dst_ptr->cur.l1_wv_ratio2_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_ratio2;
		dst_ptr->cur.l1_wv_thr_d1_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_thresh_d1;
		dst_ptr->cur.l1_wv_thr_d2_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_thresh_d2_n;
		dst_ptr->cur.l1_wv_ratio_d1_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_ratio_d1;
		dst_ptr->cur.l1_wv_ratio_d2_high = ynr_param[strength_level].ynr_wv_parm[0].high.wv_ratio_d2;
		dst_ptr->cur.l1_soft_offset_high = ynr_param[strength_level].ynr_wv_parm[0].high.soft_offset;
		dst_ptr->cur.l1_soft_offset_d_high = ynr_param[strength_level].ynr_wv_parm[0].high.soft_offsetd;

		dst_ptr->cur.l2_wv_nr_enable = ynr_param[strength_level].ynr_wv_parm[1].wv_nr_enable;

		dst_ptr->cur.l2_wv_thr1_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_thresh1;
		dst_ptr->cur.l2_wv_thr2_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_thresh2_n;
		dst_ptr->cur.l2_wv_ratio1_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_ratio1;
		dst_ptr->cur.l2_wv_ratio2_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_ratio2;
		dst_ptr->cur.l2_wv_thr_d1_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_thresh_d1;
		dst_ptr->cur.l2_wv_thr_d2_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_thresh_d2_n;
		dst_ptr->cur.l2_wv_ratio_d1_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_ratio_d1;
		dst_ptr->cur.l2_wv_ratio_d2_low = ynr_param[strength_level].ynr_wv_parm[1].low.wv_ratio_d2;
		dst_ptr->cur.l2_soft_offset_low = ynr_param[strength_level].ynr_wv_parm[1].low.soft_offset;
		dst_ptr->cur.l2_soft_offset_d_low = ynr_param[strength_level].ynr_wv_parm[1].low.soft_offsetd;

		dst_ptr->cur.l2_wv_thr1_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_thresh1;
		dst_ptr->cur.l2_wv_thr2_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_thresh2_n;
		dst_ptr->cur.l2_wv_ratio1_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_ratio1;
		dst_ptr->cur.l2_wv_ratio2_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_ratio2;
		dst_ptr->cur.l2_wv_thr_d1_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_thresh_d1;
		dst_ptr->cur.l2_wv_thr_d2_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_thresh_d2_n;
		dst_ptr->cur.l2_wv_ratio_d1_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_ratio_d1;
		dst_ptr->cur.l2_wv_ratio_d2_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.wv_ratio_d2;
		dst_ptr->cur.l2_soft_offset_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.soft_offset;
		dst_ptr->cur.l2_soft_offset_d_mid = ynr_param[strength_level].ynr_wv_parm[1].mid.soft_offsetd;

		dst_ptr->cur.l2_wv_thr1_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_thresh1;
		dst_ptr->cur.l2_wv_thr2_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_thresh2_n;
		dst_ptr->cur.l2_wv_ratio1_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_ratio1;
		dst_ptr->cur.l2_wv_ratio2_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_ratio2;
		dst_ptr->cur.l2_wv_thr_d1_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_thresh_d1;
		dst_ptr->cur.l2_wv_thr_d2_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_thresh_d2_n;
		dst_ptr->cur.l2_wv_ratio_d1_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_ratio_d1;
		dst_ptr->cur.l2_wv_ratio_d2_high = ynr_param[strength_level].ynr_wv_parm[1].high.wv_ratio_d2;
		dst_ptr->cur.l2_soft_offset_high = ynr_param[strength_level].ynr_wv_parm[1].high.soft_offset;
		dst_ptr->cur.l2_soft_offset_d_high = ynr_param[strength_level].ynr_wv_parm[1].high.soft_offsetd;

		dst_ptr->cur.l3_wv_nr_enable = ynr_param[strength_level].ynr_wv_parm[2].wv_nr_enable;

		dst_ptr->cur.l3_wv_thr1_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_thresh1;
		dst_ptr->cur.l3_wv_thr2_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_thresh2_n;
		dst_ptr->cur.l3_wv_ratio1_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_ratio1;
		dst_ptr->cur.l3_wv_ratio2_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_ratio2;
		dst_ptr->cur.l3_wv_thr_d1_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_thresh_d1;
		dst_ptr->cur.l3_wv_thr_d2_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_thresh_d2_n;
		dst_ptr->cur.l3_wv_ratio_d1_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_ratio_d1;
		dst_ptr->cur.l3_wv_ratio_d2_low = ynr_param[strength_level].ynr_wv_parm[2].low.wv_ratio_d2;
		dst_ptr->cur.l3_soft_offset_low = ynr_param[strength_level].ynr_wv_parm[2].low.soft_offset;
		dst_ptr->cur.l3_soft_offset_d_low = ynr_param[strength_level].ynr_wv_parm[2].low.soft_offsetd;

		dst_ptr->cur.l3_wv_thr1_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_thresh1;
		dst_ptr->cur.l3_wv_thr2_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_thresh2_n;
		dst_ptr->cur.l3_wv_ratio1_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_ratio1;
		dst_ptr->cur.l3_wv_ratio2_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_ratio2;
		dst_ptr->cur.l3_wv_thr_d1_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_thresh_d1;
		dst_ptr->cur.l3_wv_thr_d2_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_thresh_d2_n;
		dst_ptr->cur.l3_wv_ratio_d1_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_ratio_d1;
		dst_ptr->cur.l3_wv_ratio_d2_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.wv_ratio_d2;
		dst_ptr->cur.l3_soft_offset_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.soft_offset;
		dst_ptr->cur.l3_soft_offset_d_mid = ynr_param[strength_level].ynr_wv_parm[2].mid.soft_offsetd;

		dst_ptr->cur.l3_wv_thr1_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_thresh1;
		dst_ptr->cur.l3_wv_thr2_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_thresh2_n;
		dst_ptr->cur.l3_wv_ratio1_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_ratio1;
		dst_ptr->cur.l3_wv_ratio2_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_ratio2;
		dst_ptr->cur.l3_wv_thr_d1_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_thresh_d1;
		dst_ptr->cur.l3_wv_thr_d2_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_thresh_d2_n;
		dst_ptr->cur.l3_wv_ratio_d1_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_ratio_d1;
		dst_ptr->cur.l3_wv_ratio_d2_high = ynr_param[strength_level].ynr_wv_parm[2].high.wv_ratio_d2;
		dst_ptr->cur.l3_soft_offset_high = ynr_param[strength_level].ynr_wv_parm[2].high.soft_offset;
		dst_ptr->cur.l3_soft_offset_d_high = ynr_param[strength_level].ynr_wv_parm[2].high.soft_offsetd;

		dst_ptr->cur.l3_blf_en = ynr_param[strength_level].ynr_blf_str.ynr_blf_enable;
		dst_ptr->cur.blf_range_s0_low = ynr_param[strength_level].ynr_blf_str.blf_range.s0_low;
		dst_ptr->cur.blf_range_s0_mid = ynr_param[strength_level].ynr_blf_str.blf_range.s0_mid;
		dst_ptr->cur.blf_range_s0_high = ynr_param[strength_level].ynr_blf_str.blf_range.s0_high;
		dst_ptr->cur.radius = ynr_param[strength_level].ynr_blf_str.Radius;
		dst_ptr->cur.radius_factor = ynr_param[strength_level].ynr_blf_str.Radius_factor;
		dst_ptr->cur.max_radius = ynr_param[strength_level].ynr_blf_str.max_Radius;
		dst_ptr->cur.max_radius_factor = ynr_param[strength_level].ynr_blf_str.max_Radius_factor;
		dst_ptr->cur.radius_base = ynr_param[strength_level].radius_base;
		dst_ptr->cur.dis_interval = (ynr_param[strength_level].ynr_blf_str.max_Radius - dst_ptr->cur.radius) >> 2;
		dst_ptr->cur.center_x = ynr_param[strength_level].ynr_blf_str.imgCenterX;
		dst_ptr->cur.center_y = ynr_param[strength_level].ynr_blf_str.imgCenterY;
		dst_ptr->cur.blf_range_index = ynr_param[strength_level].ynr_blf_str.range_index;
		dst_ptr->cur.blf_range_s1 = ynr_param[strength_level].ynr_blf_str.range_s[0];
		dst_ptr->cur.blf_range_s2 = ynr_param[strength_level].ynr_blf_str.range_s[1];
		dst_ptr->cur.blf_range_s3 = ynr_param[strength_level].ynr_blf_str.range_s[2];
		dst_ptr->cur.blf_range_s4 = ynr_param[strength_level].ynr_blf_str.range_s[3];
		dst_ptr->cur.blf_dist_weight0 = ynr_param[strength_level].ynr_blf_str.dist_weight[0];
		dst_ptr->cur.blf_dist_weight1 = ynr_param[strength_level].ynr_blf_str.dist_weight[1];
		dst_ptr->cur.blf_dist_weight2 = ynr_param[strength_level].ynr_blf_str.dist_weight[2];

		dst_ptr->cur.l0_addback_enable = ynr_param[strength_level].addback[0].addback_enable;
		dst_ptr->cur.l0_addback_ratio = ynr_param[strength_level].addback[0].addback_ratio;
		dst_ptr->cur.l0_addback_clip = ynr_param[strength_level].addback[0].addback_clip;
		dst_ptr->cur.l1_addback_enable = ynr_param[strength_level].addback[1].addback_enable;
		dst_ptr->cur.l1_addback_ratio = ynr_param[strength_level].addback[1].addback_ratio;
		dst_ptr->cur.l1_addback_clip = ynr_param[strength_level].addback[1].addback_clip;
		dst_ptr->cur.l2_addback_enable = ynr_param[strength_level].addback[2].addback_enable;
		dst_ptr->cur.l2_addback_ratio = ynr_param[strength_level].addback[2].addback_ratio;
		dst_ptr->cur.l2_addback_clip = ynr_param[strength_level].addback[2].addback_clip;
		dst_ptr->cur.l3_addback_enable = ynr_param[strength_level].addback[3].addback_enable;
		dst_ptr->cur.l3_addback_ratio = ynr_param[strength_level].addback[3].addback_ratio;
		dst_ptr->cur.l3_addback_clip = ynr_param[strength_level].addback[3].addback_clip;
	}
	return rtn;
}

cmr_s32 _pm_ynr_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)dst_ynr_param;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_ynr_param;
	struct isp_pm_block_header *ynr_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = ynr_header_ptr->bypass;

	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;
	if (!ynr_header_ptr->bypass)
		rtn = _pm_ynr_convert_param(dst_ptr, dst_ptr->cur_level, ynr_header_ptr->mode_id, ISP_SCENEMODE_AUTO);
	dst_ptr->cur.bypass |= ynr_header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm ynr param!");
		return rtn;
	}

	ynr_header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_ynr_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *dst_ptr = (struct isp_ynr_param *)ynr_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_YNR_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 cur_level = 0;

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

			cur_level = (cmr_u32) block_result->component[0].fix_data[0];
			if (cur_level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_YNR_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = cur_level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_YNR_T] = 0;

				rtn = _pm_ynr_convert_param(dst_ptr, cur_level, header_ptr->mode_id, block_result->scene_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm ynr param!");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART_NR: cmd = %d, is_update = %d, ynr_level=%d", cmd, header_ptr->is_update, dst_ptr->cur_level);
		}
		break;

	default:

		break;
	}

	return rtn;
}

cmr_s32 _pm_ynr_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_ynr_param *ynr_ptr = (struct isp_ynr_param *)ynr_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&ynr_ptr->cur;
		param_data_ptr->data_size = sizeof(ynr_ptr->cur);
		*update_flag = 0;
		break;

	case ISP_PM_BLK_YNR_BYPASS:
		param_data_ptr->data_ptr = (void *)&ynr_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(ynr_ptr->cur.bypass);
		break;

	default:
		break;
	}

	return rtn;
}
