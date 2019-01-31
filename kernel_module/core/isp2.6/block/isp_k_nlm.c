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
#include <linux/vmalloc.h>
#include <sprd_mm.h>
#include <sprd_isp_r8p1.h>

#include "isp_reg.h"
#include "cam_types.h"
#include "cam_block.h"


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "NLM: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__


static int load_vst_ivst_buf(
	struct isp_dev_nlm_info_v2 *nlm_info, uint32_t idx)
{
	int ret = 0;
	uint32_t buf_len;
	uint32_t buf_sel;
	unsigned long reg_addr;
	unsigned long utab_addr;
	void __user *vst_table;
	void __user *ivst_table;

	buf_sel = 0;
	ISP_REG_MWR(idx, ISP_VST_PARA, BIT_1, buf_sel << 1);
	ISP_REG_MWR(idx, ISP_IVST_PARA, BIT_1, buf_sel << 1);

	if (nlm_info->vst_bypass == 0 && nlm_info->vst_table_addr) {
		buf_len = ISP_VST_IVST_NUM * 4;
		if (nlm_info->vst_len < (ISP_VST_IVST_NUM * 4))
			buf_len = nlm_info->vst_len;

		utab_addr = (unsigned long)nlm_info->vst_table_addr;
		pr_debug("vst table addr 0x%lx\n", utab_addr);

		vst_table = (void __user *)utab_addr;
		reg_addr = ISP_BASE_ADDR(idx) + ISP_VST_BUF0_ADDR;
		ret = copy_from_user((void *)reg_addr, vst_table, buf_len);
	}

	if (nlm_info->ivst_bypass == 0 && nlm_info->ivst_table_addr) {
		buf_len = ISP_VST_IVST_NUM * 4;
		if (nlm_info->ivst_len < (ISP_VST_IVST_NUM * 4))
			buf_len = nlm_info->ivst_len;

		utab_addr = (unsigned long)nlm_info->ivst_table_addr;
		pr_debug("ivst table addr 0x%lx\n", utab_addr);

		ivst_table = (void __user *)utab_addr;
		reg_addr = ISP_BASE_ADDR(idx) + ISP_IVST_BUF0_ADDR;
		ret = copy_from_user((void *)reg_addr, ivst_table, buf_len);
	}

	return ret;
}

static int isp_k_nlm_block(struct isp_io_param *param,
		struct isp_k_block *isp_k_param, uint32_t idx)
{
	int ret = 0;
	uint32_t i, j, val = 0;
	struct isp_dev_nlm_info_v2 *p;

	p = &isp_k_param->nlm_info;
	ret = copy_from_user((void *)p,
			(void __user *)param->property_param,
			sizeof(struct isp_dev_nlm_info_v2));
	if (ret != 0) {
		pr_err("fail to copy from user, ret = %d\n", ret);
		return  ret;
	}
	if (s_isp_bypass[idx] & (1 << _EISP_NLM))
		p->bypass = 1;
	if (s_isp_bypass[idx] & (1 << _EISP_VST))
		p->vst_bypass = 1;
	if (s_isp_bypass[idx] & (1 << _EISP_IVST))
		p->ivst_bypass = 1;
	ISP_REG_MWR(idx, ISP_NLM_PARA, BIT_0, p->bypass);
	ISP_REG_MWR(idx, ISP_VST_PARA, BIT_0, p->vst_bypass);
	ISP_REG_MWR(idx, ISP_IVST_PARA, BIT_0, p->ivst_bypass);
	if (p->bypass)
		return 0;

	val = ((p->imp_opt_bypass & 0x1) << 1) |
		((p->flat_opt_bypass & 0x1) << 2) |
		((p->direction_mode_bypass & 0x1) << 4) |
		((p->first_lum_byapss & 0x1) << 5) |
		((p->simple_bpc_bypass & 0x1) << 6);
	ISP_REG_MWR(idx, ISP_NLM_PARA, 0x76, val);

	val = ((p->direction_cnt_th & 0x3) << 24) |
		((p->w_shift[2] & 0x3) << 20) |
		((p->w_shift[1] & 0x3) << 18) |
		((p->w_shift[0] & 0x3) << 16) |
		(p->dist_mode & 0x3);
	ISP_REG_WR(idx, ISP_NLM_MODE_CNT, val);

	val = ((p->simple_bpc_th & 0xFF) << 16) |
		(p->simple_bpc_lum_th & 0x3FF);
	ISP_REG_WR(idx, ISP_NLM_SIMPLE_BPC, val);

	val = ((p->lum_th1 & 0x3FF) << 16) |
		(p->lum_th0 & 0x3FF);
	ISP_REG_WR(idx, ISP_NLM_LUM_THRESHOLD, val);

	val = ((p->tdist_min_th & 0xFFFF)  << 16) |
		(p->diff_th & 0xFFFF);
	ISP_REG_WR(idx, ISP_NLM_DIRECTION_TH, val);

	for (i = 0; i < 24; i++) {
		val = (p->lut_w[i * 3 + 0] & 0x3FF) |
			((p->lut_w[i * 3 + 1] & 0x3FF) << 10) |
			((p->lut_w[i * 3 + 2] & 0x3FF) << 20);
		ISP_REG_WR(idx, ISP_NLM_LUT_W_0 + i * 4, val);
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			val = ((p->lum_flat[i][j].thresh & 0x3FFF) << 16) |
				((p->lum_flat[i][j].match_count & 0x1F) << 8) |
				(p->lum_flat[i][j].inc_strength & 0xFF);
			ISP_REG_WR(idx,
				ISP_NLM_LUM0_FLAT0_PARAM + (i * 4 + j) * 8,
				val);
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 4; j++) {
			val = ((p->lum_flat_addback_min[i][j] & 0x7FF) << 20) |
				((p->lum_flat_addback_max[i][j] & 0x3FF) << 8) |
				(p->lum_flat_addback0[i][j] & 0x7F);
			ISP_REG_WR(idx,
				ISP_NLM_LUM0_FLAT0_ADDBACK + (i * 4 + j) * 8,
					val);
		}
	}

	for (i = 0; i < 3; i++) {
		val = ((p->lum_flat_addback1[i][0] & 0x7F) << 22) |
			((p->lum_flat_addback1[i][1] & 0x7F) << 15) |
			((p->lum_flat_addback1[i][2] & 0x7F) << 8) |
			(p->lum_flat_dec_strenth[i] & 0xFF);
		ISP_REG_WR(idx, ISP_NLM_LUM0_FLAT3_PARAM + i * 32, val);
	}

	val = (p->radius_bypass & 0x1) |
		((p->nlm_radial_1D_bypass & 0x1) << 1) |
		((p->nlm_direction_addback_mode_bypass & 0x1) << 2) |
		((p->update_flat_thr_bypass & 0x1) << 3);
	ISP_REG_MWR(idx, ISP_NLM_RADIAL_1D_PARAM, 0xF, val);

	val = ((p->nlm_radial_1D_center_y & 0x3FFF) << 16) |
			(p->nlm_radial_1D_center_x & 0x3FFF);
	ISP_REG_WR(idx, ISP_NLM_RADIAL_1D_DIST, val);

	val = p->nlm_radial_1D_radius_threshold & 0x7FFF;
	ISP_REG_MWR(idx, ISP_NLM_RADIAL_1D_THRESHOLD, 0x7FFF, val);
	val = p->nlm_radial_1D_protect_gain_max & 0x1FFF;
	ISP_REG_MWR(idx, ISP_NLM_RADIAL_1D_GAIN_MAX, 0x1FFF, val);

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			val = (p->nlm_first_lum_flat_thresh_coef[i][j] &
				0x7FFF) |
				((p->nlm_first_lum_flat_thresh_max[i][j] &
				0x3FFF) << 16);
			ISP_REG_WR(idx,
				ISP_NLM_RADIAL_1D_THR0 + i * 12 + j * 4, val);
		}
	}

	for (i = 0; i < 3; i++) {
		uint32_t *pclip, *pratio;

		pclip = p->nlm_first_lum_direction_addback_noise_clip[i];
		pratio = p->nlm_radial_1D_radius_threshold_filter_ratio[i];
		for (j = 0; j < 4; j++) {
			val = (p->nlm_radial_1D_protect_gain_min[i][j] &
				0x1FFF) | ((p->nlm_radial_1D_coef2[i][j] &
				0x3FFF) << 16);
			ISP_REG_WR(idx, ISP_NLM_RADIAL_1D_RATIO + i * 16 +
				j * 4, val);
			val = (pclip[j] & 0x3FF) |
				((p->nlm_first_lum_direction_addback[i][j] &
				0x7F) << 10) | ((pratio[j] & 0x7FFF) << 17);
			ISP_REG_WR(idx, ISP_NLM_RADIAL_1D_ADDBACK00 + i * 16 +
				j * 4, val);
		}
	}

	ret = load_vst_ivst_buf(p, idx);
	return ret;
}

static int isp_k_nlm_imblance(
		struct isp_io_param *param, uint32_t idx)
{
	int ret = 0;
	struct isp_dev_nlm_imblance imblance_info;

	ret = copy_from_user((void *)&imblance_info,
			(void __user *)param->property_param,
			sizeof(struct isp_dev_nlm_imblance));
	if (ret != 0) {
		pr_err("fail to copy from user, ret = %d\n", ret);
		return  ret;
	}

	/* new added below */
	ISP_REG_MWR(idx, ISP_NLM_PARA, BIT_7,
			(imblance_info.nlm_imblance_en)<<7);
	if (imblance_info.nlm_imblance_en == 0)
		return 0;

	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA1,
		(imblance_info.nlm_imblance_slash_edge_thr & 0xff) |
		((imblance_info.nlm_imblance_hv_edge_thr & 0xff) << 8) |
		((imblance_info.nlm_imblance_S_baohedu2 & 0xff) << 16) |
		((imblance_info.nlm_imblance_S_baohedu1 & 0xff) << 24));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA2,
		(imblance_info.nlm_imblance_slash_flat_thr & 0x3ff) |
		((imblance_info.nlm_imblance_hv_flat_thr & 0x3ff) << 10));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA3,
		(imblance_info.nlm_imblance_flag3_frez & 0x3ff) |
		((imblance_info.nlm_imblance_flag3_lum & 0x3ff) << 10) |
		((imblance_info.nlm_imblance_flag3_grid & 0x3ff) << 20));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA4,
		(imblance_info.nlm_imblance_lumth2 & 0xffff) |
		((imblance_info.nlm_imblance_lumth1 & 0xffff) << 16));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA5,
		(imblance_info.nlm_imblance_lum1_flag4_r & 0x7ff) |
		((imblance_info.nlm_imblance_lum1_flag2_r & 0x7ff) << 11) |
		((imblance_info.nlm_imblance_flag12_frezthr & 0x3ff) << 22));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA6,
		(imblance_info.nlm_imblance_lum1_flag0_r & 0x7ff) |
		((imblance_info.nlm_imblance_lum1_flag0_rs & 0x7ff) << 11));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA7,
		(imblance_info.nlm_imblance_lum2_flag2_r & 0x7ff) |
		((imblance_info.nlm_imblance_lum1_flag1_r & 0x7ff) << 11));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA8,
		(imblance_info.nlm_imblance_lum2_flag0_rs & 0x7ff) |
		((imblance_info.nlm_imblance_lum2_flag4_r & 0x7ff) << 11));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA9,
		(imblance_info.nlm_imblance_lum2_flag1_r & 0x7ff) |
		((imblance_info.nlm_imblance_lum2_flag0_r & 0x7ff) << 11));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA10,
		(imblance_info.nlm_imblance_lum3_flag4_r & 0x7ff) |
		((imblance_info.nlm_imblance_lum3_flag2_r & 0x7ff) << 11));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA11,
		(imblance_info.nlm_imblance_lum3_flag0_r & 0x7ff) |
		((imblance_info.nlm_imblance_lum3_flag0_rs & 0x7ff) << 11));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA12,
		(imblance_info.nlm_imblance_diff & 0x3ff) |
		((imblance_info.nlm_imblance_lum3_flag1_r & 0x7ff) << 10));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA13,
		(imblance_info.nlm_imblance_faceRmax & 0xffff) |
		((imblance_info.nlm_imblance_faceRmin & 0xffff) << 16));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA14,
		(imblance_info.nlm_imblance_faceBmax & 0xffff) |
		((imblance_info.nlm_imblance_faceBmin & 0xffff) << 16));
	ISP_REG_WR(idx, ISP_NLM_IMBLANCE_PARA15,
		(imblance_info.nlm_imblance_faceGmax & 0xffff) |
		((imblance_info.nlm_imblance_faceGmin & 0xffff) << 16));

	return ret;
}

int isp_k_cfg_nlm(struct isp_io_param *param,
		struct isp_k_block *isp_k_param, uint32_t idx)
{
	int ret = 0;

	switch (param->property) {
	case ISP_PRO_NLM_BLOCK:
		ret = isp_k_nlm_block(param, isp_k_param, idx);
		break;
	case ISP_PRO_NLM_IMBLANCE:
		ret = isp_k_nlm_imblance(param, idx);
		break;
	default:
		pr_err("fail to support cmd id = %d\n",
			param->property);
		break;
	}

	return ret;
}

int isp_k_update_nlm(uint32_t idx,
	struct isp_k_block *isp_k_param,
	uint32_t new_width, uint32_t old_width,
	uint32_t new_height, uint32_t old_height)
{
	int ret = 0;
	int i, j, loop;
	uint32_t val;
	uint32_t center_x, center_y, radius_threshold;
	uint32_t radius_limit, r_factor, r_base;
	uint32_t filter_ratio, coef2, flat_thresh_coef;
	struct isp_dev_nlm_info_v2 *p;

	p = &isp_k_param->nlm_info;
	if (p->bypass)
		return 0;

	center_x = new_width >> 1;
	center_y = new_height >> 1;
	val = ((center_y & 0x3FFF) << 16) | (center_x & 0x3FFF);
	ISP_REG_WR(idx, ISP_NLM_RADIAL_1D_DIST, val);

	r_base = p->radius_base;
	r_factor = p->nlm_radial_1D_radius_threshold_factor;
	radius_threshold = p->nlm_radial_1D_radius_threshold;
	radius_threshold *= new_width;
	radius_threshold = (radius_threshold + (old_width / 2)) / old_width;
	radius_limit = (new_width + new_height) * r_factor / r_base;
	radius_threshold = (radius_threshold < radius_limit) ? radius_threshold : radius_limit;
	ISP_REG_MWR(idx, ISP_NLM_RADIAL_1D_THRESHOLD, 0x7FFF, radius_threshold);

	pr_debug("center (%d %d)  raius %d  (%d %d), new %d\n",
		center_x, center_y, p->nlm_radial_1D_radius_threshold,
		r_factor, r_base, radius_threshold);

	for (loop = 0; loop < 12; loop++) {
		i = loop >> 2;
		j = loop & 0x3;
		filter_ratio =
			p->nlm_radial_1D_radius_threshold_filter_ratio[i][j];
		filter_ratio *= new_width;
		filter_ratio += (old_width / 2);
		filter_ratio /= old_width;

		r_factor = p->nlm_radial_1D_radius_threshold_filter_ratio_factor[i][j];
		radius_limit = (new_width + new_height) * r_factor / r_base;
		filter_ratio = (filter_ratio < radius_limit) ? filter_ratio : radius_limit;
		pr_debug("%d, factor %d , new %d\n",
			p->nlm_radial_1D_radius_threshold_filter_ratio[i][j],
			r_factor, filter_ratio);

		coef2 = p->nlm_radial_1D_coef2[i][j];
		coef2 *= old_width;
		coef2 = (coef2 + (new_width / 2)) / new_width;
		ISP_REG_MWR(idx,
			ISP_NLM_RADIAL_1D_ADDBACK00 + i * 16 + j * 4,
			0xFFFE0000, (filter_ratio << 17));
		ISP_REG_MWR(idx,
			ISP_NLM_RADIAL_1D_RATIO + i * 16 + j * 4,
			0x3FFF0000, (coef2 << 16));
		if (j < 3) {
			flat_thresh_coef =
				p->nlm_first_lum_flat_thresh_coef[i][j];
			flat_thresh_coef *= old_width;
			flat_thresh_coef += (new_width / 2);
			flat_thresh_coef /= new_width;
			ISP_REG_MWR(idx,
				ISP_NLM_RADIAL_1D_THR0 + i * 12 + j * 4,
				0x7FFF, flat_thresh_coef);
		}
		if (loop == 0)
			pr_debug("filter coef2 flat (%d %d %d) (%d %d %d)\n",
			p->nlm_radial_1D_radius_threshold_filter_ratio[i][j],
			p->nlm_radial_1D_coef2[i][j],
			p->nlm_first_lum_flat_thresh_coef[i][j],
			filter_ratio, coef2, flat_thresh_coef);
	}

	return ret;
}
