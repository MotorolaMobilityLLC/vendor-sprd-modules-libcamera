/*
 * Copyright (C) 2017-2018 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/uaccess.h>
#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>

#include "isp_reg.h"
#include "cam_types.h"
#include "cam_block.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "EDGE: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

static int isp_k_edge_block(struct isp_io_param *param, uint32_t idx)
{
	int ret = 0;
	uint32_t i = 0, val = 0;
	struct isp_dev_edge_info_v2 tmp;

	ret = copy_from_user((void *)&tmp,
			param->property_param,
			sizeof(tmp));
	if (ret != 0) {
		pr_err("fail to copy from user, ret = %d\n", ret);
		return ret;
	}
	if (s_isp_bypass[idx] & (1 << _EISP_EE))
		tmp.bypass = 1;
	ISP_REG_MWR(idx, ISP_EE_PARAM, BIT_0, tmp.bypass);
	if (tmp.bypass)
		return 0;

	val = ((tmp.ee_old_gradient_en & 0x1) << 2)|
			((tmp.ee_new_pyramid_en & 0x1) << 1)|
			(tmp.bypass & 0x1);
	ISP_REG_MWR(idx, ISP_EE_PARAM, 0x7, val);
	if (tmp.bypass)
		return 0;

	val = ((tmp.mode & 0x1) << 28) |
		((tmp.ee_str_d.p & 0x7F) << 21) |
		((tmp.ee_str_d.n & 0x7F) << 14) |
		((tmp.edge_smooth_mode & 0x3) << 2) |
		(tmp.flat_smooth_mode & 0x3);
	ISP_REG_WR(idx, ISP_EE_CFG0, val);

	val = ((tmp.ee_edge_thr_d.p & 0xFF) << 24) |
		((tmp.ee_edge_thr_d.n & 0xFF) << 16) |
		((tmp.ee_incr_d.p & 0xFF) << 8) |
		(tmp.ee_incr_d.n & 0xFF);
	ISP_REG_WR(idx, ISP_EE_CFG1, val);

	val = ((tmp.ee_corner_cor & 0x1) << 28) |
		((tmp.ee_corner_th.p & 0x1F) << 23) |
		((tmp.ee_corner_th.n & 0x1F) << 18) |
		((tmp.ee_corner_gain.p & 0x7F) << 11) |
		((tmp.ee_corner_gain.n & 0x7F) << 4) |
		((tmp.ee_corner_sm.p & 0x3) << 2) |
		(tmp.ee_corner_sm.n & 0x3);
	ISP_REG_WR(idx, ISP_EE_CFG2, val);

	val = ((tmp.ee_cv_t[2] & 0x3FF) << 20) |
		((tmp.ee_cv_t[1] & 0x3FF) << 10) |
		(tmp.ee_cv_t[0] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_ADP_CFG0, val);

	ISP_REG_WR(idx, ISP_EE_ADP_CFG1, val);
	val = ((tmp.ee_cv_clip.p & 0xFF) << 24) |
			((tmp.ee_cv_clip.n & 0xFF) << 16) |
			(tmp.ee_cv_t[3] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_ADP_CFG1, val);

	val = ((tmp.ee_cv_r[2] & 0xFF) << 16) |
		((tmp.ee_cv_r[1] & 0xFF) << 8) |
		(tmp.ee_cv_r[0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_ADP_CFG2, val);

	val = ((tmp.ipd_smooth_mode.n & 0x7) << 14) |
		((tmp.ipd_smooth_mode.p & 0x7) << 11) |
		((tmp.ipd_smooth_en & 0x1) << 10) |
		((tmp.ipd_less_thr.n & 0xF) << 6) |
		((tmp.ipd_less_thr.p & 0xF) << 2) |
		((tmp.ipd_mask_mode & 0x1) << 1) |
		(tmp.ipd_enable & 0x1);
	ISP_REG_WR(idx, ISP_EE_IPD_CFG0, val);

	val = ((tmp.ipd_more_thr.n & 0xF) << 28) |
		((tmp.ipd_more_thr.p & 0xF) << 24) |
		((tmp.ipd_eq_thr.n & 0xF) << 20) |
		((tmp.ipd_eq_thr.p & 0xF) << 16) |
		((tmp.ipd_flat_thr.n & 0xFF) << 8) |
		(tmp.ipd_flat_thr.p & 0xFF);
	ISP_REG_WR(idx, ISP_EE_IPD_CFG1, val);

	val = ((tmp.ipd_smooth_edge_diff.n & 0xFF) << 24) |
		((tmp.ipd_smooth_edge_diff.p & 0xFF) << 16) |
		((tmp.ipd_smooth_edge_thr.n & 0xFF) << 8) |
		(tmp.ipd_smooth_edge_thr.p & 0xFF);
	ISP_REG_WR(idx, ISP_EE_IPD_CFG2, val);

	val = ((tmp.ee_weight_diag2hv & 0x1F) << 27) |
		((tmp.ee_gradient_computation_type & 0x1) << 26) |
		((tmp.ee_weight_hv2diag & 0x1F) << 21) |
		((tmp.ee_ratio_diag_3 & 0x7F) << 14) |
		((tmp.ee_ratio_hv_5 & 0x7F) << 7) |
		(tmp.ee_ratio_hv_3 & 0x7F);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG0, val);

	for (i = 0; i < 2; i++) {
		val = ((tmp.ee_gain_hv_t[i][2] & 0x3FF) << 20) |
			((tmp.ee_gain_hv_t[i][1] & 0x3FF) << 10) |
			(tmp.ee_gain_hv_t[i][0] & 0x3FF);
		ISP_REG_WR(idx, ISP_EE_LUM_CFG1 + 8 * i, val);
	}

	val = ((tmp.ee_ratio_diag_5 & 0x7F) << 25) |
		((tmp.ee_gain_hv_r[0][2] & 0x1F) << 20) |
		((tmp.ee_gain_hv_r[0][1] & 0x1F) << 15) |
		((tmp.ee_gain_hv_r[0][0] & 0x1F) << 10) |
		(tmp.ee_gain_hv_t[0][3] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG2, val);

	val = ((tmp.ee_gain_hv_r[1][2] & 0x1F) << 20) |
		((tmp.ee_gain_hv_r[1][1] & 0x1F) << 15) |
		((tmp.ee_gain_hv_r[1][0] & 0x1F) << 10) |
		(tmp.ee_gain_hv_t[1][3] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG4, val);

	for (i = 0; i < 2; i++) {
		val = ((tmp.ee_gain_diag_t[i][2] & 0x3FF) << 20) |
			((tmp.ee_gain_diag_t[i][1] & 0x3FF) << 10) |
			(tmp.ee_gain_diag_t[i][0] & 0x3FF);
		ISP_REG_WR(idx, ISP_EE_LUM_CFG5 + 8 * i, val);

		val = ((tmp.ee_gain_diag_r[i][2] & 0x1F) << 20) |
			((tmp.ee_gain_diag_r[i][1] & 0x1F) << 15) |
			((tmp.ee_gain_diag_r[i][0] & 0x1F) << 10) |
			(tmp.ee_gain_diag_t[i][3] & 0x3FF);
		ISP_REG_WR(idx, ISP_EE_LUM_CFG6 + 8 * i, val);
	}

	val = ((tmp.ee_lum_t[3] & 0xFF) << 24) |
		((tmp.ee_lum_t[2] & 0xFF) << 16) |
		((tmp.ee_lum_t[1] & 0xFF) << 8) |
		(tmp.ee_lum_t[0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG9, val);

	val = ((tmp.ee_lum_r[2] & 0x7F) << 14) |
		((tmp.ee_lum_r[1] & 0x7F) << 7) |
		(tmp.ee_lum_r[0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG10, val);

	val = ((tmp.ee_pos_t[2]  & 0x3FF) << 20) |
		((tmp.ee_pos_t[1] & 0x3FF) << 10) |
		(tmp.ee_pos_t[0] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG11, val);

	val = ((tmp.ee_pos_r[2] & 0x7F) << 24) |
		((tmp.ee_pos_r[1] & 0x7F) << 17) |
		((tmp.ee_pos_r[0] & 0x7F) << 10) |
		(tmp.ee_pos_t[3] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG12, val);

	val = ((tmp.ee_pos_c[2] & 0x7F) << 14) |
		((tmp.ee_pos_c[1] & 0x7F) << 7) |
		(tmp.ee_pos_c[0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG13, val);

	val = ((tmp.ee_neg_t[2]  & 0x3FF) << 20) |
		((tmp.ee_neg_t[1]  & 0x3FF) << 10) |
		(tmp.ee_neg_t[0]  & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG14, val);

	val = ((tmp.ee_neg_r[2] & 0x7F) << 24) |
		((tmp.ee_neg_r[1] & 0x7F) << 17) |
		((tmp.ee_neg_r[0] & 0x7F) << 10) |
		(tmp.ee_neg_t[3] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG15, val);

	val = ((tmp.ee_neg_c[2] & 0xFF) << 16) |
		((tmp.ee_neg_c[1] & 0xFF) << 8) |
		(tmp.ee_neg_c[0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG16, val);

	val = ((tmp.ee_freq_t[2] & 0x3FF) << 20) |
		((tmp.ee_freq_t[1] & 0x3FF) << 10) |
		(tmp.ee_freq_t[0] & 0x3FF);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG17, val);

	val = ((tmp.ee_freq_t[3] & 0x3FF) << 18) |
		((tmp.ee_freq_r[2] & 0x3F) << 12) |
		((tmp.ee_freq_r[1] & 0x3F) << 6) |
		(tmp.ee_freq_r[0] & 0x3F);
	ISP_REG_WR(idx, ISP_EE_LUM_CFG18, val);


	/* new added below */
	val =  ((tmp.ee_ratio_old_gradient & 0x3F) << 6) |
		(tmp.ee_ratio_new_pyramid & 0x3F);
	ISP_REG_WR(idx, ISP_EE_RATIO, val);

	val = ((tmp.ee_offset_thr_layer_curve_pos[0][3] & 0xFF) << 24) |
		((tmp.ee_offset_thr_layer_curve_pos[0][2] & 0xFF) << 16) |
		((tmp.ee_offset_thr_layer_curve_pos[0][1] & 0xFF) << 8) |
		(tmp.ee_offset_thr_layer_curve_pos[0][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_THR_LAYER0_POS, val);

	val = ((tmp.ee_offset_ratio_layer_curve_pos[0][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_curve_pos[0][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_curve_pos[0][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER0_CURVE_POS, val);

	val = ((tmp.ee_offset_clip_layer_curve_pos[0][2] & 0x7F) << 16) |
		((tmp.ee_offset_clip_layer_curve_pos[0][1] & 0x7F) << 8) |
		(tmp.ee_offset_clip_layer_curve_pos[0][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_CLIP_LAYER0_POS, val);

	val = ((tmp.ee_offset_thr_layer_curve_neg[0][3] & 0xFF) << 24) |
		((tmp.ee_offset_thr_layer_curve_neg[0][2] & 0xFF) << 16) |
		((tmp.ee_offset_thr_layer_curve_neg[0][1] & 0xFF) << 8) |
		(tmp.ee_offset_thr_layer_curve_neg[0][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_THR_LAYER0_NEG, val);

	val = ((tmp.ee_offset_ratio_layer_curve_neg[0][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_curve_neg[0][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_curve_neg[0][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER0_CURVE_NEG, val);

	val = ((tmp.ee_offset_clip_layer_curve_neg[0][2] & 0xFF) << 16) |
		((tmp.ee_offset_clip_layer_curve_neg[0][1] & 0xFF) << 8) |
		(tmp.ee_offset_clip_layer_curve_neg[0][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_CLIP_LAYER0_CURVE_NEG, val);

	val = ((tmp.ee_offset_ratio_layer_lum_curve[0][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_lum_curve[0][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_lum_curve[0][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER0_LUM_CURVE, val);

	val = ((tmp.ee_offset_ratio_layer_freq_curve[0][2] & 0x3F) << 16) |
		((tmp.ee_offset_ratio_layer_freq_curve[0][1] & 0x3F) << 8) |
		(tmp.ee_offset_ratio_layer_freq_curve[0][0] & 0x3F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER0_FREQ_CURVE, val);

	val = ((tmp.ee_offset_thr_layer_curve_pos[1][3] & 0xFF) << 24) |
		((tmp.ee_offset_thr_layer_curve_pos[1][2] & 0xFF) << 16) |
		((tmp.ee_offset_thr_layer_curve_pos[1][1] & 0xFF) << 8) |
		(tmp.ee_offset_thr_layer_curve_pos[1][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_THR_LAYER1_POS, val);

	val = ((tmp.ee_offset_ratio_layer_curve_pos[1][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_curve_pos[1][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_curve_pos[1][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER1_CURVE_POS, val);

	val = ((tmp.ee_offset_clip_layer_curve_pos[1][2] & 0x7F) << 16) |
		((tmp.ee_offset_clip_layer_curve_pos[1][1] & 0x7F) << 8) |
		(tmp.ee_offset_clip_layer_curve_pos[1][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_CLIP_LAYER1_POS, val);

	val = ((tmp.ee_offset_thr_layer_curve_neg[1][3] & 0xFF) << 24) |
		((tmp.ee_offset_thr_layer_curve_neg[1][2] & 0xFF) << 16) |
		((tmp.ee_offset_thr_layer_curve_neg[1][1] & 0xFF) << 8) |
		(tmp.ee_offset_thr_layer_curve_neg[1][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_THR_LAYER1_NEG, val);

	val = ((tmp.ee_offset_ratio_layer_curve_neg[1][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_curve_neg[1][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_curve_neg[1][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER1_CURVE_NEG, val);

	val = ((tmp.ee_offset_clip_layer_curve_neg[1][2] & 0xFF) << 16) |
		((tmp.ee_offset_clip_layer_curve_neg[1][1] & 0xFF) << 8) |
		(tmp.ee_offset_clip_layer_curve_neg[1][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_CLIP_LAYER1_CURVE_NEG, val);

	val = ((tmp.ee_offset_ratio_layer_lum_curve[1][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_lum_curve[1][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_lum_curve[1][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER1_LUM_CURVE, val);

	val = ((tmp.ee_offset_ratio_layer_freq_curve[1][2] & 0x3F) << 16) |
		((tmp.ee_offset_ratio_layer_freq_curve[1][1] & 0x3F) << 8) |
		(tmp.ee_offset_ratio_layer_freq_curve[1][0] & 0x3F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER1_FREQ_CURVE, val);

	val = ((tmp.ee_offset_thr_layer_curve_pos[2][3] & 0xFF) << 24) |
		((tmp.ee_offset_thr_layer_curve_pos[2][2] & 0xFF) << 16) |
		((tmp.ee_offset_thr_layer_curve_pos[2][1] & 0xFF) << 8) |
		(tmp.ee_offset_thr_layer_curve_pos[2][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_THR_LAYER2_POS, val);

	val = ((tmp.ee_offset_ratio_layer_curve_pos[2][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_curve_pos[2][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_curve_pos[2][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER2_CURVE_POS, val);

	val = ((tmp.ee_offset_clip_layer_curve_pos[2][2] & 0x7F) << 16) |
		((tmp.ee_offset_clip_layer_curve_pos[2][1] & 0x7F) << 8) |
		(tmp.ee_offset_clip_layer_curve_pos[2][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_CLIP_LAYER2_POS, val);

	val = ((tmp.ee_offset_thr_layer_curve_neg[2][3] & 0xFF) << 24) |
		((tmp.ee_offset_thr_layer_curve_neg[2][2] & 0xFF) << 16) |
		((tmp.ee_offset_thr_layer_curve_neg[2][1] & 0xFF) << 8) |
		(tmp.ee_offset_thr_layer_curve_neg[2][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_THR_LAYER2_NEG, val);

	val = ((tmp.ee_offset_ratio_layer_curve_neg[2][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_curve_neg[2][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_curve_neg[2][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER2_CURVE_NEG, val);

	val = ((tmp.ee_offset_clip_layer_curve_neg[2][2] & 0xFF) << 16) |
		((tmp.ee_offset_clip_layer_curve_neg[2][1] & 0xFF) << 8) |
		(tmp.ee_offset_clip_layer_curve_neg[2][0] & 0xFF);
	ISP_REG_WR(idx, ISP_EE_OFFSET_CLIP_LAYER2_CURVE_NEG, val);

	val = ((tmp.ee_offset_ratio_layer_lum_curve[2][2] & 0x7F) << 16) |
		((tmp.ee_offset_ratio_layer_lum_curve[2][1] & 0x7F) << 8) |
		(tmp.ee_offset_ratio_layer_lum_curve[2][0] & 0x7F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER2_LUM_CURVE, val);

	val = ((tmp.ee_offset_ratio_layer_freq_curve[2][2] & 0x3F) << 16) |
		((tmp.ee_offset_ratio_layer_freq_curve[2][1] & 0x3F) << 8) |
		(tmp.ee_offset_ratio_layer_freq_curve[2][0] & 0x3F);
	ISP_REG_WR(idx, ISP_EE_OFFSET_RATIO_LAYER2_FREQ_CURVE, val);

	return ret;
}

int isp_k_cfg_edge(struct isp_io_param *param, uint32_t idx)
{
	int ret = 0;

	switch (param->property) {
	case ISP_PRO_EDGE_BLOCK:
		ret = isp_k_edge_block(param, idx);
		break;

	default:
		pr_err("fail to support cmd id = %d\n",
			param->property);
		break;
	}

	return ret;
}
