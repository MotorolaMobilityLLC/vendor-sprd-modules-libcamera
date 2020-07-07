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
#define LOG_TAG "isp_blk_edge"
#include "isp_blocks_cfg.h"

#define INVALID_EE_COEFF ((cmr_u32)(-1))


static cmr_u32 _pm_edge_convert_param(
	void *dst_edge_param, cmr_u32 strength_level,
	cmr_u32 mode_flag, cmr_u32 scene_flag, cmr_u32 ai_scene_id, cmr_u32 ai_pro_scene_flag)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 total_offset_units = 0;
	cmr_u32 i;
	char prop[PROPERTY_VALUE_MAX];
	cmr_u32 ee_param_log_en = 0;
	cmr_u32 foliage_coeff = 10;
	cmr_u32 text_coeff = 7;
	cmr_u32 pet_coeff = 8;
	cmr_u32 building_coeff = 2;
	cmr_u32 snow_coeff = 6;
	cmr_u32 night_coeff = 9;
	cmr_u32 sel_coeff = INVALID_EE_COEFF;
	cmr_u32 max_ee_neg = 0x100;
	struct isp_edge_param *dst_ptr = (struct isp_edge_param *)dst_edge_param;
	struct sensor_ee_level *edge_param = PNULL;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->nr_mode_setting) {
		edge_param = (struct sensor_ee_level *)(dst_ptr->param_ptr);
	} else {
		cmr_u32 *multi_nr_map_ptr = PNULL;
		multi_nr_map_ptr = (cmr_u32 *) dst_ptr->scene_ptr;
		total_offset_units = _pm_calc_nr_addr_offset(mode_flag, scene_flag, multi_nr_map_ptr);
		edge_param = (struct sensor_ee_level *)((cmr_u8 *) dst_ptr->param_ptr + \
				total_offset_units * dst_ptr->level_num * sizeof(struct sensor_ee_level));

	}
	strength_level = PM_CLIP(strength_level, 0, dst_ptr->level_num - 1);

	if (edge_param != NULL) {
		dst_ptr->cur.bypass = edge_param[strength_level].bypass;

		dst_ptr->cur.flat_smooth_mode = edge_param[strength_level].flat_smooth_mode;
		dst_ptr->cur.edge_smooth_mode = edge_param[strength_level].edge_smooth_mode;
		dst_ptr->cur.mode = edge_param[strength_level].ee_mode;

		dst_ptr->cur.ee_str_d.p = edge_param[strength_level].str_d_p.positive;
		dst_ptr->cur.ee_str_d.n = edge_param[strength_level].str_d_p.negative;
		dst_ptr->cur.ee_incr_d.p = edge_param[strength_level].ee_incr_d.positive;
		dst_ptr->cur.ee_incr_d.n = edge_param[strength_level].ee_incr_d.negative;
		dst_ptr->cur.ee_edge_thr_d.p = edge_param[strength_level].ee_thr_d.positive;
		dst_ptr->cur.ee_edge_thr_d.n = edge_param[strength_level].ee_thr_d.negative;
		dst_ptr->cur.ee_cv_clip.p = edge_param[strength_level].ee_cv_clip.positive;
		dst_ptr->cur.ee_cv_clip.n = edge_param[strength_level].ee_cv_clip.negative;

		dst_ptr->cur.ee_corner_sm.p = edge_param[strength_level].ee_corner.ee_corner_sm.positive;
		dst_ptr->cur.ee_corner_sm.n = edge_param[strength_level].ee_corner.ee_corner_sm.negative;
		dst_ptr->cur.ee_corner_gain.p = edge_param[strength_level].ee_corner.ee_corner_gain.positive;
		dst_ptr->cur.ee_corner_gain.n = edge_param[strength_level].ee_corner.ee_corner_gain.negative;
		dst_ptr->cur.ee_corner_th.p = edge_param[strength_level].ee_corner.ee_corner_th.positive;
		dst_ptr->cur.ee_corner_th.n = edge_param[strength_level].ee_corner.ee_corner_th.negative;
		dst_ptr->cur.ee_corner_cor = edge_param[strength_level].ee_corner.ee_corner_cor;

		dst_ptr->cur.ipd_enable= !edge_param[strength_level].ee_ipd.ipd_bypass;
		dst_ptr->cur.ipd_mask_mode = edge_param[strength_level].ee_ipd.ipd_mask_mode;
		dst_ptr->cur.ipd_less_thr.p = edge_param[strength_level].ee_ipd.ipd_less_thr.positive;
		dst_ptr->cur.ipd_less_thr.n = edge_param[strength_level].ee_ipd.ipd_less_thr.negative;
		dst_ptr->cur.ipd_flat_thr.p = edge_param[strength_level].ee_ipd.ipd_flat_thr.positive;
		dst_ptr->cur.ipd_flat_thr.n = edge_param[strength_level].ee_ipd.ipd_flat_thr.negative;
		dst_ptr->cur.ipd_eq_thr.p = edge_param[strength_level].ee_ipd.ipd_eq_thr.positive;
		dst_ptr->cur.ipd_eq_thr.n = edge_param[strength_level].ee_ipd.ipd_eq_thr.negative;
		dst_ptr->cur.ipd_more_thr.p = edge_param[strength_level].ee_ipd.ipd_more_thr.positive;
		dst_ptr->cur.ipd_more_thr.n = edge_param[strength_level].ee_ipd.ipd_more_thr.negative;
		dst_ptr->cur.ipd_smooth_en = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_en;
		dst_ptr->cur.ipd_smooth_mode.p = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_mode.positive;
		dst_ptr->cur.ipd_smooth_mode.n = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_mode.negative;
		dst_ptr->cur.ipd_smooth_edge_thr.p = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_thr.positive;
		dst_ptr->cur.ipd_smooth_edge_thr.n = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_thr.negative;
		dst_ptr->cur.ipd_smooth_edge_diff.p = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_diff.positive;
		dst_ptr->cur.ipd_smooth_edge_diff.n = edge_param[strength_level].ee_ipd.ipd_smooth.smooth_ee_diff.negative;

		dst_ptr->cur.ee_ratio_hv_3 = edge_param[strength_level].ee_gradient.ratio.ratio_hv_3;
		dst_ptr->cur.ee_ratio_hv_5 = edge_param[strength_level].ee_gradient.ratio.ratio_hv_5;
		dst_ptr->cur.ee_ratio_diag_3 = edge_param[strength_level].ee_gradient.ratio.ratio_dg_3;
		dst_ptr->cur.ee_ratio_diag_5 = edge_param[strength_level].ee_gradient.ratio.ratio_dg_5;
		dst_ptr->cur.ee_weight_hv2diag = edge_param[strength_level].ee_gradient.wgt_hv2diag;
		dst_ptr->cur.ee_weight_diag2hv = edge_param[strength_level].ee_gradient.wgt_diag2hv;

		dst_ptr->cur.ee_gradient_computation_type = edge_param[strength_level].ee_gradient.grd_cmpt_type;

		dst_ptr->cur.ee_gain_hv_t[0][0] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_hv_t[0][1] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_hv_t[0][2] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_hv_t[0][3] = edge_param[strength_level].ee_gradient.ee_gain_hv1.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][0] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][1] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][2] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_hv_t[1][3] = edge_param[strength_level].ee_gradient.ee_gain_hv2.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_hv_r[0][0] = edge_param[strength_level].ee_gradient.ee_gain_hv1.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_hv_r[0][1] = edge_param[strength_level].ee_gradient.ee_gain_hv1.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_hv_r[0][2] = edge_param[strength_level].ee_gradient.ee_gain_hv1.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_gain_hv_r[1][0] = edge_param[strength_level].ee_gradient.ee_gain_hv2.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_hv_r[1][1] = edge_param[strength_level].ee_gradient.ee_gain_hv2.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_hv_r[1][2] = edge_param[strength_level].ee_gradient.ee_gain_hv2.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_gain_diag_t[0][0] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_diag_t[0][1] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_diag_t[0][2] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_diag_t[0][3] = edge_param[strength_level].ee_gradient.ee_gain_dg1.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][0] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][1] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][2] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_gain_diag_t[1][3] = edge_param[strength_level].ee_gradient.ee_gain_dg2.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_gain_diag_r[0][0] = edge_param[strength_level].ee_gradient.ee_gain_dg1.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_diag_r[0][1] = edge_param[strength_level].ee_gradient.ee_gain_dg1.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_diag_r[0][2] = edge_param[strength_level].ee_gradient.ee_gain_dg1.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_gain_diag_r[1][0] = edge_param[strength_level].ee_gradient.ee_gain_dg2.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_gain_diag_r[1][1] = edge_param[strength_level].ee_gradient.ee_gain_dg2.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_gain_diag_r[1][2] = edge_param[strength_level].ee_gradient.ee_gain_dg2.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_cv_t[0] = edge_param[strength_level].ee_cv.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_cv_t[1] = edge_param[strength_level].ee_cv.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_cv_t[2] = edge_param[strength_level].ee_cv.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_cv_t[3] = edge_param[strength_level].ee_cv.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_cv_r[0] = edge_param[strength_level].ee_cv.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_cv_r[1] = edge_param[strength_level].ee_cv.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_cv_r[2] = edge_param[strength_level].ee_cv.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_lum_t[0] = edge_param[strength_level].ee_lum.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_lum_t[1] = edge_param[strength_level].ee_lum.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_lum_t[2] = edge_param[strength_level].ee_lum.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_lum_t[3] = edge_param[strength_level].ee_lum.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_lum_r[0] = edge_param[strength_level].ee_lum.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_lum_r[1] = edge_param[strength_level].ee_lum.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_lum_r[2] = edge_param[strength_level].ee_lum.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_freq_t[0] = edge_param[strength_level].ee_freq.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_freq_t[1] = edge_param[strength_level].ee_freq.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_freq_t[2] = edge_param[strength_level].ee_freq.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_freq_t[3] = edge_param[strength_level].ee_freq.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_freq_r[0] = edge_param[strength_level].ee_freq.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_freq_r[1] = edge_param[strength_level].ee_freq.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_freq_r[2] = edge_param[strength_level].ee_freq.r_cfg.ee_r3_cfg;

		dst_ptr->cur.ee_pos_t[0] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_pos_t[1] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_pos_t[2] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_pos_t[3] = edge_param[strength_level].ee_clip.ee_pos.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_pos_r[0] = edge_param[strength_level].ee_clip.ee_pos.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_pos_r[1] = edge_param[strength_level].ee_clip.ee_pos.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_pos_r[2] = edge_param[strength_level].ee_clip.ee_pos.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_pos_c[0] = edge_param[strength_level].ee_clip.ee_pos_c.ee_c1_cfg;
		dst_ptr->cur.ee_pos_c[1] = edge_param[strength_level].ee_clip.ee_pos_c.ee_c2_cfg;
		dst_ptr->cur.ee_pos_c[2] = edge_param[strength_level].ee_clip.ee_pos_c.ee_c3_cfg;
		dst_ptr->cur.ee_neg_t[0] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t1_cfg;
		dst_ptr->cur.ee_neg_t[1] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t2_cfg;
		dst_ptr->cur.ee_neg_t[2] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t3_cfg;
		dst_ptr->cur.ee_neg_t[3] = edge_param[strength_level].ee_clip.ee_neg.t_cfg.ee_t4_cfg;
		dst_ptr->cur.ee_neg_r[0] = edge_param[strength_level].ee_clip.ee_neg.r_cfg.ee_r1_cfg;
		dst_ptr->cur.ee_neg_r[1] = edge_param[strength_level].ee_clip.ee_neg.r_cfg.ee_r2_cfg;
		dst_ptr->cur.ee_neg_r[2] = edge_param[strength_level].ee_clip.ee_neg.r_cfg.ee_r3_cfg;
		dst_ptr->cur.ee_neg_c[0] = edge_param[strength_level].ee_clip.ee_neg_c.ee_c1_cfg;
		dst_ptr->cur.ee_neg_c[1] = edge_param[strength_level].ee_clip.ee_neg_c.ee_c2_cfg;
		dst_ptr->cur.ee_neg_c[2] = edge_param[strength_level].ee_clip.ee_neg_c.ee_c3_cfg;
	}

	property_get("debug.isp.ee.foliage_coeff.val", prop, "10");
	foliage_coeff = atoi(prop);
	property_get("debug.isp.ee.text_coeff.val", prop, "7");
	text_coeff = atoi(prop);
	property_get("debug.isp.ee.pet_coeff.val", prop, "8");
	pet_coeff = atoi(prop);
	property_get("debug.isp.ee.building_coeff.val", prop, "2");
	building_coeff = atoi(prop);
	property_get("debug.isp.ee.snow_coeff.val", prop, "6");
	snow_coeff = atoi(prop);
	property_get("debug.isp.ee.night_coeff.val", prop, "9");
	night_coeff = atoi(prop);
	property_get("debug.isp.ee.param.log.en", prop, "0");
	ee_param_log_en = atoi(prop);

	ISP_LOGV("ai_scene_id = %d", ai_scene_id);

	switch (ai_scene_id) {
	case ISP_PM_AI_SCENE_FOLIAGE:
	case ISP_PM_AI_SCENE_FLOWER:
		sel_coeff = foliage_coeff;
		break;

	case ISP_PM_AI_SCENE_TEXT:
		sel_coeff = text_coeff;
		break;

	case ISP_PM_AI_SCENE_PET:
		sel_coeff = pet_coeff;
		break;

	case ISP_PM_AI_SCENE_BUILDING:
		sel_coeff = building_coeff;
		break;

	case ISP_PM_AI_SCENE_SNOW:
		sel_coeff = snow_coeff;
		break;

	case ISP_PM_AI_SCENE_NIGHT:
		sel_coeff = night_coeff;
		break;

	default:
		break;
	}

	if (ai_pro_scene_flag == 1 || ai_scene_id == ISP_PM_AI_SCENE_NIGHT)
		sel_coeff = 10;

	if (sel_coeff == 0)
		sel_coeff = INVALID_EE_COEFF;

	if (sel_coeff != INVALID_EE_COEFF) {
		dst_ptr->cur.ee_pos_r[0] = dst_ptr->cur.ee_pos_r[0] * 10 / sel_coeff;
		dst_ptr->cur.ee_pos_r[1] = dst_ptr->cur.ee_pos_r[1] * 10 / sel_coeff;
		dst_ptr->cur.ee_pos_r[2] = dst_ptr->cur.ee_pos_r[2] * 10 / sel_coeff;
		dst_ptr->cur.ee_pos_c[0] = dst_ptr->cur.ee_pos_c[0] * 10 / sel_coeff;
		dst_ptr->cur.ee_pos_c[1] = dst_ptr->cur.ee_pos_c[1] * 10 / sel_coeff;
		dst_ptr->cur.ee_pos_c[2] = dst_ptr->cur.ee_pos_c[2] * 10 / sel_coeff;
		dst_ptr->cur.ee_neg_r[0] = dst_ptr->cur.ee_neg_r[0] * 10 / sel_coeff;
		dst_ptr->cur.ee_neg_r[1] = dst_ptr->cur.ee_neg_r[1] * 10 / sel_coeff;
		dst_ptr->cur.ee_neg_r[2] = dst_ptr->cur.ee_neg_r[2] * 10 / sel_coeff;
		dst_ptr->cur.ee_neg_c[0] = max_ee_neg - (max_ee_neg - dst_ptr->cur.ee_neg_c[0]) * 10 / sel_coeff;
		dst_ptr->cur.ee_neg_c[1] = max_ee_neg - (max_ee_neg - dst_ptr->cur.ee_neg_c[1]) * 10 / sel_coeff;
		dst_ptr->cur.ee_neg_c[2] = max_ee_neg - (max_ee_neg - dst_ptr->cur.ee_neg_c[2]) * 10 / sel_coeff;

		/* Bug 1082178 ee_neg_c param value should be betwen
		  * [-128,0]. To check we just  consider 8 LSBs. If any of the value is
		  * not in range then we assign 0x80 effectively it will become  128-256 = -128.
		  */

		if ((dst_ptr->cur.ee_neg_c[0] & 0xFF) < 0x80)
			dst_ptr->cur.ee_neg_c[0] = 0x80;

		if ((dst_ptr->cur.ee_neg_c[1] & 0xFF) < 0x80)
			dst_ptr->cur.ee_neg_c[1] = 0x80;

		if ((dst_ptr->cur.ee_neg_c[2] & 0xFF) < 0x80)
			dst_ptr->cur.ee_neg_c[2] = 0x80;

		if (ee_param_log_en) {
			for (i = 0; i < 3; i++) {
				ISP_LOGV("i = %d, pos_r = 0x%x, pos_c = 0x%x, neg_r = 0x%x, neg_c = 0x%x",
					i, dst_ptr->cur.ee_pos_r[i], dst_ptr->cur.ee_pos_c[i],
					dst_ptr->cur.ee_neg_r[i], dst_ptr->cur.ee_neg_c[i]);
			}
		}
	}

	return rtn;
}

cmr_s32 _pm_edge_init(void *dst_edge_param, void *src_edge_param, void *param1, void *param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	struct isp_pm_nr_header_param *src_ptr = (struct isp_pm_nr_header_param *)src_edge_param;
	struct isp_edge_param *dst_ptr = (struct isp_edge_param *)dst_edge_param;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->param_ptr = src_ptr->param_ptr;
	dst_ptr->cur_level = src_ptr->default_strength_level;
	dst_ptr->level_num = src_ptr->level_number;
	dst_ptr->scene_ptr = src_ptr->multi_nr_map_ptr;
	dst_ptr->nr_mode_setting = src_ptr->nr_mode_setting;
	if (!header_ptr->bypass)
		rtn = _pm_edge_convert_param(dst_ptr, dst_ptr->cur_level,
				ISP_MODE_ID_COMMON, ISP_SCENEMODE_AUTO, ISP_PM_AI_SCENE_DEFAULT, 0);
	dst_ptr->cur.bypass |= header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to convert pm edge param !");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

cmr_s32 _pm_edge_set_param(void *edge_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_edge_param *dst_ptr = (struct isp_edge_param *)edge_param;

	switch (cmd) {
	case ISP_PM_BLK_EDGE_BYPASS:
		dst_ptr->cur.bypass = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_EDGE_STRENGTH:
		dst_ptr->cur_level = *((cmr_u32 *) param_ptr0);
		header_ptr->is_update = ISP_ONE;
		break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct smart_block_result *block_result = (struct smart_block_result *)param_ptr0;
			struct isp_range val_range = { 0, 0 };
			cmr_u32 level = 0;

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

			if (level != dst_ptr->cur_level || nr_tool_flag[ISP_BLK_EDGE_T] || block_result->mode_flag_changed) {
				dst_ptr->cur_level = level;
				header_ptr->is_update = ISP_ONE;
				nr_tool_flag[ISP_BLK_EDGE_T] = 0;

				rtn = _pm_edge_convert_param(dst_ptr, dst_ptr->cur_level,
							header_ptr->mode_id,
							block_result->scene_flag,
							block_result->ai_scene_id,
							block_result->ai_scene_pro_flag);
				dst_ptr->cur.bypass |= header_ptr->bypass;
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("fail to convert pm edge param !");
					return rtn;
				}
			}
			ISP_LOGV("ISP_SMART_NR: cmd=%d, update=%d, ee_level=%d",
				cmd, header_ptr->is_update, dst_ptr->cur_level);
		}
		break;

	case ISP_PM_BLK_AI_SCENE_UPDATE_EE:
		{
			cmr_u32 i, k;
			cmr_s16 smooth_factor, smooth_base;
			struct isp_ai_update_param *cfg_data;
			struct isp_ai_ee_param_v1 *ee_cur;
			struct isp_edge_ai_param_v1 ee_updata;

			cfg_data = (struct isp_ai_update_param *)param_ptr0;
			ee_cur = (struct isp_ai_ee_param_v1*)cfg_data->param_ptr;
			smooth_factor = cfg_data->smooth_factor;
			smooth_base = cfg_data->smooth_base;
			if (smooth_factor == 0) {
				if (!header_ptr->is_update)
					break;
				smooth_factor = 1;
				smooth_base = 1;
			} else if (!header_ptr->is_update) {
				smooth_factor = (smooth_factor > 0) ? 1 :  -1;
			}

			ee_updata.ee_gain_hv_r[0][0] = dst_ptr->cur.ee_gain_hv_r[0][0] & 0xFF;
			ee_updata.ee_gain_hv_r[0][1] = dst_ptr->cur.ee_gain_hv_r[0][1] & 0xFF;
			ee_updata.ee_gain_hv_r[0][2] = dst_ptr->cur.ee_gain_hv_r[0][2] & 0xFF;
			ee_updata.ee_gain_hv_r[1][0] = dst_ptr->cur.ee_gain_hv_r[1][0] & 0xFF;
			ee_updata.ee_gain_hv_r[1][1] = dst_ptr->cur.ee_gain_hv_r[1][1] & 0xFF;
			ee_updata.ee_gain_hv_r[1][2] = dst_ptr->cur.ee_gain_hv_r[1][2] & 0xFF;
			ee_updata.ee_gain_diag_r[0][0] = dst_ptr->cur.ee_gain_diag_r[0][0] & 0xFF;
			ee_updata.ee_gain_diag_r[0][1] = dst_ptr->cur.ee_gain_diag_r[0][1] & 0xFF;
			ee_updata.ee_gain_diag_r[0][2] = dst_ptr->cur.ee_gain_diag_r[0][2] & 0xFF;
			ee_updata.ee_gain_diag_r[1][0] = dst_ptr->cur.ee_gain_diag_r[1][0] & 0xFF;
			ee_updata.ee_gain_diag_r[1][1] = dst_ptr->cur.ee_gain_diag_r[1][1] & 0xFF;
			ee_updata.ee_gain_diag_r[1][2] = dst_ptr->cur.ee_gain_diag_r[1][2] & 0xFF;
			ee_updata.ee_pos_r[0] = dst_ptr->cur.ee_pos_r[0] & 0xFF;
			ee_updata.ee_pos_r[1] = dst_ptr->cur.ee_pos_r[1] & 0xFF;
			ee_updata.ee_pos_r[2] = dst_ptr->cur.ee_pos_r[2] & 0xFF;
			ee_updata.ee_pos_c[0] = dst_ptr->cur.ee_pos_c[0] & 0xFF;
			ee_updata.ee_pos_c[1] = dst_ptr->cur.ee_pos_c[1] & 0xFF;
			ee_updata.ee_pos_c[2] = dst_ptr->cur.ee_pos_c[2] & 0xFF;
			ee_updata.ee_neg_r[0] = dst_ptr->cur.ee_neg_r[0] & 0xFF;
			ee_updata.ee_neg_r[1] = dst_ptr->cur.ee_neg_r[1] & 0xFF;
			ee_updata.ee_neg_r[2] = dst_ptr->cur.ee_neg_r[2] & 0xFF;
			ee_updata.ee_neg_c[0] = dst_ptr->cur.ee_neg_c[0] & 0xFF;
			ee_updata.ee_neg_c[1] = dst_ptr->cur.ee_neg_c[1] & 0xFF;
			ee_updata.ee_neg_c[2] = dst_ptr->cur.ee_neg_c[2] & 0xFF;

			ee_updata.ee_gain_hv_r[0][0] += ee_cur->ee_gain_hv1.ee_r1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_hv_r[0][1] += ee_cur->ee_gain_hv1.ee_r2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_hv_r[0][2] += ee_cur->ee_gain_hv1.ee_r3_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_hv_r[1][0] += ee_cur->ee_gain_hv2.ee_r1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_hv_r[1][1] += ee_cur->ee_gain_hv2.ee_r2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_hv_r[1][2] += ee_cur->ee_gain_hv2.ee_r3_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_diag_r[0][0] += ee_cur->ee_gain_diag1.ee_r1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_diag_r[0][1] += ee_cur->ee_gain_diag1.ee_r2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_diag_r[0][2] += ee_cur->ee_gain_diag1.ee_r3_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_diag_r[1][0] += ee_cur->ee_gain_diag2.ee_r1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_diag_r[1][1] += ee_cur->ee_gain_diag2.ee_r2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_gain_diag_r[1][2] += ee_cur->ee_gain_diag2.ee_r3_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_pos_r[0] += ee_cur->ee_pos_r.ee_r1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_pos_r[1] += ee_cur->ee_pos_r.ee_r2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_pos_r[2] += ee_cur->ee_pos_r.ee_r3_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_pos_c[0] += ee_cur->ee_pos_c.ee_c1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_pos_c[1] += ee_cur->ee_pos_c.ee_c2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_pos_c[2] += ee_cur->ee_pos_c.ee_c3_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_neg_r[0] += ee_cur->ee_neg_r.ee_r1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_neg_r[1] += ee_cur->ee_neg_r.ee_r2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_neg_r[2] += ee_cur->ee_neg_r.ee_r3_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_neg_c[0] += ee_cur->ee_neg_c.ee_c1_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_neg_c[1] += ee_cur->ee_neg_c.ee_c2_cfg_offset * smooth_factor / smooth_base;
			ee_updata.ee_neg_c[2] += ee_cur->ee_neg_c.ee_c3_cfg_offset * smooth_factor / smooth_base;

			for(i = 0; i < 2; i++) {
				for (k = 0; k < 3; k++) {
					if (ee_updata.ee_gain_hv_r[i][k] > 31 )
						ee_updata.ee_gain_hv_r[i][k] = 31;
					if (ee_updata.ee_gain_hv_r[i][k] < 0 )
						ee_updata.ee_gain_hv_r[i][k] = 0;
					if (ee_updata.ee_gain_diag_r[i][k] > 31 )
						ee_updata.ee_gain_diag_r[i][k] = 31;
					if (ee_updata.ee_gain_diag_r[i][k] < 0 )
						ee_updata.ee_gain_diag_r[i][k] = 0;
				}
			}
			for(i = 0; i < 3; i++) {
				if (ee_updata.ee_pos_r[i] > 127 )
					ee_updata.ee_pos_r[i] = 127;
				if (ee_updata.ee_pos_r[i] < 0 )
					ee_updata.ee_pos_r[i] = 0;
				if (ee_updata.ee_pos_c[i] > 127 )
					ee_updata.ee_pos_c[i] = 127;
				if (ee_updata.ee_pos_c[i] < 0 )
					ee_updata.ee_pos_c[i] = 0;
				if (ee_updata.ee_neg_r[i] > 127 )
					ee_updata.ee_neg_r[i] = 127;
				if (ee_updata.ee_neg_r[i] < 0 )
					ee_updata.ee_neg_r[i] = 0;
				if (ee_updata.ee_neg_c[i] >0 )
					ee_updata.ee_neg_c[i] = 0;
				if (ee_updata.ee_neg_c[i] < -128 )
					ee_updata.ee_neg_c[i] = -128;
			}

			dst_ptr->cur.ee_gain_hv_r[0][0] = ee_updata.ee_gain_hv_r[0][0];
			dst_ptr->cur.ee_gain_hv_r[0][1] = ee_updata.ee_gain_hv_r[0][1];
			dst_ptr->cur.ee_gain_hv_r[0][2] = ee_updata.ee_gain_hv_r[0][2];
			dst_ptr->cur.ee_gain_hv_r[1][0] = ee_updata.ee_gain_hv_r[1][0];
			dst_ptr->cur.ee_gain_hv_r[1][1] = ee_updata.ee_gain_hv_r[1][1];
			dst_ptr->cur.ee_gain_hv_r[1][2] = ee_updata.ee_gain_hv_r[1][2];
			dst_ptr->cur.ee_gain_diag_r[0][0] = ee_updata.ee_gain_diag_r[0][0];
			dst_ptr->cur.ee_gain_diag_r[0][1] = ee_updata.ee_gain_diag_r[0][1];
			dst_ptr->cur.ee_gain_diag_r[0][2] = ee_updata.ee_gain_diag_r[0][2];
			dst_ptr->cur.ee_gain_diag_r[1][0] = ee_updata.ee_gain_diag_r[1][0];
			dst_ptr->cur.ee_gain_diag_r[1][1] = ee_updata.ee_gain_diag_r[1][1];
			dst_ptr->cur.ee_gain_diag_r[1][2] = ee_updata.ee_gain_diag_r[1][2];
			dst_ptr->cur.ee_pos_r[0] = ee_updata.ee_pos_r[0];
			dst_ptr->cur.ee_pos_r[1] = ee_updata.ee_pos_r[1];
			dst_ptr->cur.ee_pos_r[2] = ee_updata.ee_pos_r[2];
			dst_ptr->cur.ee_pos_c[0] = ee_updata.ee_pos_c[0];
			dst_ptr->cur.ee_pos_c[1] = ee_updata.ee_pos_c[1];
			dst_ptr->cur.ee_pos_c[2] = ee_updata.ee_pos_c[2];
			dst_ptr->cur.ee_neg_r[0] = ee_updata.ee_neg_r[0];
			dst_ptr->cur.ee_neg_r[1] = ee_updata.ee_neg_r[1];
			dst_ptr->cur.ee_neg_r[2] = ee_updata.ee_neg_r[2];
			dst_ptr->cur.ee_neg_c[0] = ee_updata.ee_neg_c[0];
			dst_ptr->cur.ee_neg_c[1] = ee_updata.ee_neg_c[1];
			dst_ptr->cur.ee_neg_c[2] = ee_updata.ee_neg_c[2];

			header_ptr->is_update = ISP_ONE;
		}
		break;
	default:
		break;
	}

	return rtn;
}

cmr_s32 _pm_edge_get_param(void *edge_param,
		cmr_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_edge_param *edge_ptr = (struct isp_edge_param *)edge_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &edge_ptr->cur;
		param_data_ptr->data_size = sizeof(edge_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
