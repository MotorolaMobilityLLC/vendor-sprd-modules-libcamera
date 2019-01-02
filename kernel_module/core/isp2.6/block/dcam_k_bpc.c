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

#include "dcam_reg.h"
#include "dcam_interface.h"
#include "cam_types.h"
#include "cam_block.h"


#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "BPC: %d %d %s : "\
	fmt, current->pid, __LINE__, __func__

enum {
	_UPDATE_BLOCK = BIT(0),
	_UPDATE_MAP = BIT(1),
	_UPDATE_HDR = BIT(2),
	_UPDATE_PPE = BIT(3),
};

int dcam_k_bpc_block(struct dcam_dev_param *param)
{
	int ret = 0;
	uint32_t idx = param->idx;
	int i = 0;
	uint32_t val = 0;
	struct dcam_dev_bpc_info *p; /* bpc_info; */

	if (param == NULL)
		return -EPERM;
	/* update ? */
	if (!(param->bpc.update & _UPDATE_BLOCK))
		return 0;
	param->bpc.update &= (~(_UPDATE_BLOCK));
	p = &(param->bpc.bpc_info);
	/* debugfs bpc not bypass then write*/
	if (s_dbg_bypass[idx] & (1 << _E_BPC))
		p->bpc_bypass = 1;

	/* following bit can be 0 only if bpc_bypss is 0 */
	if (p->bpc_bypass == 1)
		p->bpc_double_bypass = 1;
	if (p->bpc_double_bypass == 1)
		p->bpc_three_bypass = 1;
	if (p->bpc_three_bypass == 1)
		p->bpc_four_bypass = 1;

	val = (p->bpc_bypass & 0x1) |
		((p->bpc_double_bypass & 0x1) << 1) |
		((p->bpc_three_bypass & 0x1) << 2) |
		((p->bpc_four_bypass & 0x1) << 3);
	DCAM_REG_MWR(idx, ISP_BPC_PARAM, 0xF, val);
	val = DCAM_REG_RD(idx, ISP_BPC_PARAM);
	if (p->bpc_bypass)
		return 0;

	val = ((p->bpc_mode & 0x3) << 4) |
		((p->bpc_is_mono_sensor & 0x1) << 6) |
		((p->bpc_ppi_en & 0x1) << 7) |
		((p->bpc_edge_hv_mode & 0x3) << 8) |
		((p->bpc_edge_rd_mode & 0x3) << 10) |
		((p->bpc_pos_out_en & 0x1) << 16) |
		((p->bpc_map_clr_en & 0x1) << 17) |
		((p->bpc_rd_max_len_sel & 0x1) << 18) |
		((p->bpc_wr_max_len_sel & 0x1) << 19) |
		((p->bpc_blk_mode & 0x1) << 20) |
		((p->bpc_mod_en & 0x1) << 30) |
		((p->bpc_cg_dis & 0x1) << 31);
	DCAM_REG_MWR(idx, ISP_BPC_PARAM, 0xC01F0FF0, val);

	for (i = 0; i < 4; i++) {
		val = (p->bpc_four_badpixel_th[i] & 0x3FF) |
			((p->bpc_three_badpixel_th[i] & 0x3FF) << 10) |
			((p->bpc_double_badpixel_th[i] & 0x3FF) << 20);
		DCAM_REG_WR(idx, ISP_BPC_BAD_PIXEL_TH0 + i * 4, val);
	}

	val = (p->bpc_texture_th & 0x3FF) |
		((p->bpc_flat_th & 0x3FF) << 10) |
		((p->bpc_shift[2] & 0xF) << 20) |
		((p->bpc_shift[1] & 0xF) << 24) |
		((p->bpc_shift[0] & 0xF) << 28);
	DCAM_REG_WR(idx, ISP_BPC_FLAT_TH, val);

	val = (p->bpc_edgeratio_hv & 0x1FF) |
		((p->bpc_edgeratio_rd & 0x1FF) << 16);
	DCAM_REG_WR(idx, ISP_BPC_EDGE_RATIO, val);

	val = (p->bpc_highoffset & 0xFF) |
		((p->bpc_lowoffset & 0xFF) << 8) |
		((p->bpc_highcoeff & 0x7) << 16) |
		((p->bpc_lowcoeff & 0x7) << 24);
	DCAM_REG_WR(idx, ISP_BPC_BAD_PIXEL_PARAM, val);

	val = (p->bpc_mincoeff & 0x1F) |
		((p->bpc_maxcoeff & 0x1F) << 16);
	DCAM_REG_WR(idx, ISP_BPC_BAD_PIXEL_COEFF, val);

	for (i = 0; i < 8; i++) {
		val = ((p->bpc_lut_level[i] & 0x3FF) << 20) |
			((p->bpc_slope_k[i] & 0x3FF) << 10) |
			(p->bpc_intercept_b[i] & 0x3FF);
		DCAM_REG_WR(idx, ISP_BPC_LUTWORD0 + i * 4, val);
	}

	val = p->bad_pixel_num;
	DCAM_REG_WR(idx, ISP_BPC_MAP_CTRL, val);

	return ret;
}

/* actually map mode is not enable now.*/
int dcam_k_bpc_map(struct dcam_dev_param *param)
{
	int ret = 0;
	uint32_t idx = param->idx;
	uint32_t val = 0;
	struct dcam_dev_bpc_info *p; /* bpc_info; */

	if (param == NULL)
		return -EPERM;
	/* update ? */
	if (!(param->bpc.update & _UPDATE_MAP))
		return 0;
	param->bpc.update &= (~(_UPDATE_MAP));
	p = &(param->bpc.bpc_info);


	val = p->bpc_map_addr & 0xFFFFFFF0;
	DCAM_REG_WR(idx, ISP_BPC_MAP_ADDR, val);

	val = p->bad_pixel_num;
	DCAM_REG_WR(idx, ISP_BPC_MAP_CTRL, val);

	val = p->bpc_bad_pixel_pos_out_addr & 0xFFFFFFF0;
	DCAM_REG_WR(idx, ISP_BPC_OUT_ADDR, val);
	return ret;
}

int dcam_k_bpc_hdr_param(struct dcam_dev_param *param)
{
	int ret = 0;
	uint32_t idx = param->idx;
	uint32_t val = 0;
	struct dcam_dev_bpc_info *p; /* bpc_info; */

	if (param == NULL)
		return -EPERM;
	/* update ? */
	if (!(param->bpc.update & _UPDATE_HDR))
		return 0;
	param->bpc.update &= (~(_UPDATE_HDR));
	p = &(param->bpc.bpc_info);

	val = (p->bpc_hdr_en & 0x1) << 12;
	DCAM_REG_MWR(idx, ISP_BPC_PARAM, 0x1000, val);

	val = (p->bpc_rawhdr_info.zzbpc_hdr_ratio & 0x1FF) |
		((p->bpc_rawhdr_info.zzbpc_hdr_ratio_inv & 0x1FF) << 16) |
		((p->bpc_rawhdr_info.zzbpc_hdr_2badpixel_en & 0x1) << 31);
	DCAM_REG_WR(idx, ISP_ZZBPC_PARAM, val);

	val = (p->bpc_rawhdr_info.zzbpc_long_over_th & 0x3FF) |
		((p->bpc_rawhdr_info.zzbpc_short_over_th & 0x3FF) << 12) |
		((p->bpc_rawhdr_info.zzbpc_over_expo_num & 0xF) << 24);
	DCAM_REG_WR(idx, ISP_ZZBPC_OVER_PARAM, val);

	val = (p->bpc_rawhdr_info.zzbpc_long_under_th & 0x3FF) |
		((p->bpc_rawhdr_info.zzbpc_short_under_th & 0x3FF) << 12) |
		((p->bpc_rawhdr_info.zzbpc_under_expo_num & 0xF) << 24);
	DCAM_REG_WR(idx, ISP_ZZBPC_UNDER_PARAM, val);

	val = (p->bpc_rawhdr_info.zzbpc_flat_th & 0x3FF) |
		((p->bpc_rawhdr_info.zzbpc_edgeratio_rd & 0x1FF) << 12) |
		((p->bpc_rawhdr_info.zzbpc_edgeratio_hv & 0x1FF) << 21);
	DCAM_REG_WR(idx, ISP_ZZBPC_AREA_PARAM, val);

	val = (p->bpc_rawhdr_info.zzbpc_kmin_under_expo & 0x1F) |
		((p->bpc_rawhdr_info.zzbpc_kmax_under_expo & 0x1F) << 8) |
		((p->bpc_rawhdr_info.zzbpc_kmin_over_expo & 0x1F) << 16) |
		((p->bpc_rawhdr_info.zzbpc_kmax_over_expo & 0x1F) << 24);
	DCAM_REG_WR(idx, ISP_ZZBPC_COEFF_PARAM, val);

	return ret;
}

int dcam_k_bpc_ppe_param(struct dcam_dev_param *param)
{
	int ret = 0;
	uint32_t idx = param->idx;
	int i = 0;
	uint32_t val = 0;
	struct dcam_bpc_ppi_info *p;

	if (param == NULL)
		return -EPERM;
	/* update ? */
	if (!(param->bpc.update & _UPDATE_PPE))
		return 0;
	param->bpc.update &= (~(_UPDATE_PPE));
	p = &(param->bpc.bpc_info.bpc_ppe_info);

	val = (p->bpc_ppi_block_start_row & 0xFFFF) |
		((p->bpc_ppi_block_end_row & 0xFFFF) << 16);
	DCAM_REG_WR(idx, ISP_ZZBPC_PPI_RANG, val);

	val = (p->bpc_ppi_block_start_col & 0xFFFF) |
		((p->bpc_ppi_block_end_col & 0xFFFF) << 16);
	DCAM_REG_WR(idx, ISP_ZZBPC_PPI_RANG1, val);

	val = ((p->bpc_ppi_block_width & 0x3) << 4) |
		((p->bpc_ppi_block_height & 0x3) << 6) |
		((p->bpc_ppi_phase_pixel_num & 0x7F) << 16);
	DCAM_REG_WR(idx, ISP_PPI_PARAM, val);

	for (i = 0; i < 32; i++) {
		val = (p->bpc_ppi_pattern_col[i * 2] & 0x3F) |
			((p->bpc_ppi_pattern_row[i * 2] & 0x3F) << 6) |
			((p->bpc_ppi_pattern_pos[i * 2] & 0x1) << 12) |
			((p->bpc_ppi_pattern_col[i * 2 + 1] & 0x3F) << 16) |
			((p->bpc_ppi_pattern_row[i * 2 + 1] & 0x3F) << 22) |
			((p->bpc_ppi_pattern_pos[i * 2 + 1] & 0x1) << 28);
		DCAM_REG_WR(idx, ISP_PPI_PATTERN01 + i * 4, val);
	}

	return ret;
}

/* ioctl transmit whole bpc_info, but only update some
 * part of it
 * Input: s is isp_io_param.property
 */
static int update_param(struct dcam_dev_param *p,
			struct dcam_dev_bpc_info *psrc, uint32_t s)
{
	int i;
	struct dcam_dev_bpc_info *pdst;

	pdst = &(p->bpc.bpc_info);
	switch (s) {
	case DCAM_PRO_BPC_BLOCK:
		pdst->bpc_bypass = psrc->bpc_bypass;
		pdst->bpc_ppi_en = psrc->bpc_ppi_en;
		pdst->bpc_mod_en = psrc->bpc_mod_en;
		pdst->bpc_double_bypass = psrc->bpc_double_bypass;
		pdst->bpc_three_bypass = psrc->bpc_three_bypass;
		pdst->bpc_four_bypass = psrc->bpc_four_bypass;
		pdst->bpc_mode = psrc->bpc_mode;
		pdst->bpc_is_mono_sensor = psrc->bpc_is_mono_sensor;
		pdst->bpc_edge_hv_mode = psrc->bpc_edge_hv_mode;
		pdst->bpc_edge_rd_mode = psrc->bpc_edge_rd_mode;
		pdst->bpc_pos_out_en = psrc->bpc_pos_out_en;
		pdst->bpc_map_clr_en = psrc->bpc_map_clr_en;
		pdst->bpc_rd_max_len_sel = psrc->bpc_rd_max_len_sel;
		pdst->bpc_wr_max_len_sel = psrc->bpc_wr_max_len_sel;
		pdst->bpc_blk_mode = psrc->bpc_blk_mode;
		pdst->bpc_cg_dis = psrc->bpc_cg_dis;
		for (i = 0; i < 4; i++) {
			pdst->bpc_four_badpixel_th[i] =
				psrc->bpc_four_badpixel_th[i];
			pdst->bpc_three_badpixel_th[i] =
				psrc->bpc_three_badpixel_th[i];
			pdst->bpc_double_badpixel_th[i] =
				psrc->bpc_double_badpixel_th[i];
		}
		pdst->bpc_texture_th = psrc->bpc_texture_th;
		pdst->bpc_flat_th = psrc->bpc_flat_th;
		for (i = 0; i < 3; i++)
			pdst->bpc_shift[i] = psrc->bpc_shift[i];

		pdst->bpc_edgeratio_hv = psrc->bpc_edgeratio_hv;
		pdst->bpc_edgeratio_rd = psrc->bpc_edgeratio_rd;
		pdst->bpc_highoffset = psrc->bpc_highoffset;
		pdst->bpc_lowoffset = psrc->bpc_lowoffset;
		pdst->bpc_highcoeff = psrc->bpc_highcoeff;
		pdst->bpc_mincoeff = psrc->bpc_mincoeff;
		pdst->bpc_maxcoeff = psrc->bpc_maxcoeff;
		for (i = 0; i < 4; i++) {
			pdst->bpc_lut_level[i] = psrc->bpc_lut_level[i];
			pdst->bpc_slope_k[i] = psrc->bpc_slope_k[i];
			pdst->bpc_intercept_b[i] = psrc->bpc_intercept_b[i];
		}
		pdst->bad_pixel_num = psrc->bad_pixel_num;
		/* Maybe can do like this
		 * uint32_t *pd, *ps;
		 * for (i = 0; i < (&pdst->bpc_rawhdr_info - pdst); i++)
		 *      *pd++ = *ps++;
		 */
		p->bpc.update |= _UPDATE_BLOCK;
		break;
	case DCAM_PRO_BPC_MAP:
		pdst->bpc_map_addr = psrc->bpc_map_addr;
		pdst->bad_pixel_num = psrc->bad_pixel_num;
		pdst->bpc_bad_pixel_pos_out_addr =
			psrc->bpc_bad_pixel_pos_out_addr;
		p->bpc.update |= _UPDATE_MAP;

		break;
	case DCAM_PRO_BPC_HDR_PARAM:
		pdst->bpc_hdr_en = psrc->bpc_hdr_en;
		pdst->bpc_rawhdr_info = psrc->bpc_rawhdr_info;
		p->bpc.update |= _UPDATE_HDR;
		break;
	case DCAM_PRO_BPC_PPE_PARAM:
		pdst->bpc_ppe_info = psrc->bpc_ppe_info;
		p->bpc.update |= _UPDATE_PPE;
		break;
	}

	return 0;
}
int dcam_k_cfg_bpc(struct isp_io_param *param, struct dcam_dev_param *p)
{
	int ret = 0;
	struct dcam_dev_bpc_info bpc_info;
	FUNC_DCAM_PARAM sub_func = NULL;

	ret = copy_from_user((void *)&bpc_info, param->property_param,
				sizeof(bpc_info));
	if (ret) {
		pr_err("blc_block: copy error, ret=0x%x\n",
			(unsigned int)ret);
		return -EPERM;
	}
	if (!(DCAM_ONLINE_MODE))
		mutex_lock(&p->param_lock);

	update_param(p, &bpc_info, param->property);

	if (!(DCAM_ONLINE_MODE))
		mutex_unlock(&p->param_lock);

	switch (param->property) {
	case DCAM_PRO_BPC_BLOCK:
		sub_func = dcam_k_bpc_block;
		break;
	case DCAM_PRO_BPC_MAP:
		sub_func = dcam_k_bpc_map;
		break;
	case DCAM_PRO_BPC_HDR_PARAM:
		sub_func = dcam_k_bpc_hdr_param;
		break;
	case DCAM_PRO_BPC_PPE_PARAM:
		sub_func = dcam_k_bpc_ppe_param;
		break;
	default:
		pr_err("fail to support property %d\n",
			param->property);
		return -EINVAL;
	}
	if (DCAM_ONLINE_MODE)
		ret = sub_func(p);


	return ret;
}
