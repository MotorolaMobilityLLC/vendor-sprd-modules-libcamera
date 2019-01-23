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
#ifndef _CAM_BLOCK_H_
#define _CAM_BLOCK_H_

#include "cam_buf.h"
#include "dcam_blkparam.h"

struct isp_k_block {
	struct img_size src;
	struct isp_dev_nlm_info_v2 nlm_info;
	struct isp_dev_ynr_info_v2 ynr_info;
	struct isp_dev_3dnr_info nr3_info;
};

int dcam_init_lsc(void *param, uint32_t online);
int dcam_update_lsc(void *param);
int dcam_k_cfg_blc(struct isp_io_param *param,	struct dcam_dev_param *p);
int dcam_k_cfg_rgb_gain(struct isp_io_param *param, struct dcam_dev_param *p);
int dcam_k_cfg_rgb_dither(struct isp_io_param *param,
	struct dcam_dev_param *p);
int dcam_k_cfg_pdaf(struct isp_io_param *param,	struct dcam_dev_param *p);
int dcam_k_cfg_lsc(struct isp_io_param *param, struct dcam_dev_param *p);
int dcam_k_cfg_bayerhist(struct isp_io_param *param,
	struct dcam_dev_param *p);
int dcam_k_cfg_aem(struct isp_io_param *param, struct dcam_dev_param *p);
int dcam_k_cfg_afl(struct isp_io_param *param, struct dcam_dev_param *p);
int dcam_k_cfg_awbc(struct isp_io_param *param, struct dcam_dev_param *p);
int dcam_k_cfg_bpc(struct isp_io_param *param, struct dcam_dev_param *p);
int dcam_k_cfg_3dnr_me(struct isp_io_param *param, struct dcam_dev_param *p);
int dcam_k_cfg_afm(struct isp_io_param *param, struct dcam_dev_param *p);
void dcam_k_3dnr_set_roi(uint32_t image_width, uint32_t image_height,
			 uint32_t project_mode, uint32_t idx);

/* dcam offline param set interface */
int dcam_k_blc_block(struct dcam_dev_param *param);
int dcam_k_rgb_gain_block(struct dcam_dev_param *param);
int dcam_k_rgb_dither_random_block(struct dcam_dev_param *param);
int dcam_k_lsc_block(struct dcam_dev_param *param);
int dcam_k_bayerhist_block(struct dcam_dev_param *param);

int dcam_k_aem_bypass(struct dcam_dev_param *param);
int dcam_k_aem_mode(struct dcam_dev_param *param);
int dcam_k_aem_win(struct dcam_dev_param *param);
int dcam_k_aem_skip_num(struct dcam_dev_param *param);
int dcam_k_aem_rgb_thr(struct dcam_dev_param *param);

int dcam_k_afl_block(struct dcam_dev_param *param);
int dcam_k_afl_bypass(struct dcam_dev_param *param);
int dcam_k_awbc_block(struct dcam_dev_param *param);
int dcam_k_awbc_gain(struct dcam_dev_param *param);
int dcam_k_awbc_block(struct dcam_dev_param *param);

int dcam_k_bpc_block(struct dcam_dev_param *param);
int dcam_k_bpc_map(struct dcam_dev_param *param);
int dcam_k_bpc_hdr_param(struct dcam_dev_param *param);
int dcam_k_bpc_ppe_param(struct dcam_dev_param *param);

int dcam_k_3dnr_me(struct dcam_dev_param *param);

int dcam_k_afm_block(struct dcam_dev_param *param);
int dcam_k_afm_bypass(struct dcam_dev_param *param);
int dcam_k_afm_win(struct dcam_dev_param *param);
int dcam_k_afm_win_num(struct dcam_dev_param *param);
int dcam_k_afm_mode(struct dcam_dev_param *param);
int dcam_k_afm_skipnum(struct dcam_dev_param *param);
int dcam_k_afm_crop_eb(struct dcam_dev_param *param);
int dcam_k_afm_crop_size(struct dcam_dev_param *param);
int dcam_k_afm_done_tilenum(struct dcam_dev_param *param);
/* dcam offline .. end */

int isp_k_cfg_nlm(struct isp_io_param *param,
	struct isp_k_block *isp_k_param, uint32_t idx);
int isp_k_cfg_ynr(struct isp_io_param *param,
	struct isp_k_block *isp_k_param, uint32_t idx);
int isp_k_cfg_3dnr(struct isp_io_param *param,
	struct isp_k_block *isp_k_param, uint32_t idx);

int isp_k_cfg_bchs(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_cce(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_cdn(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_cfa(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_cmc10(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_edge(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_gamma(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_grgb(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_hist2(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_hsv(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_iircnr(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_ltm(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_post_cdn(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_pre_cdn(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_pstrz(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_uvd(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_ygamma(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_yrandom(struct isp_io_param *param, uint32_t idx);
int isp_k_cfg_yuv_noisefilter(struct isp_io_param *param, uint32_t idx);

int isp_k_update_nlm(uint32_t idx,
	struct isp_k_block *isp_k_param,
	uint32_t new_width, uint32_t old_width,
	uint32_t new_height, uint32_t old_height,
	uint32_t crop_start_x, uint32_t crop_start_y,
	uint32_t crop_end_x, uint32_t crop_end_y);
int isp_k_update_ynr(uint32_t idx,
	struct isp_k_block *isp_k_param,
	uint32_t new_width, uint32_t old_width,
	uint32_t new_height, uint32_t old_height,
	uint32_t crop_start_x, uint32_t crop_start_y,
	uint32_t crop_end_x, uint32_t crop_end_y);
int isp_k_update_3dnr(uint32_t idx,
	struct isp_k_block *isp_k_param,
	uint32_t new_width, uint32_t old_width,
	uint32_t new_height, uint32_t old_height,
	uint32_t crop_start_x, uint32_t crop_start_y,
	uint32_t crop_end_x, uint32_t crop_end_y);
/* for bypass dcam,isp sub-block */
enum block_bypass {
	_E_4IN1 = 0,
	_E_PDAF,
	_E_LSC,
	_E_AEM,
	_E_HIST,
	_E_AFL,
	_E_AFM,
	_E_BPC,
	_E_BLC,
	_E_RGB,
	_E_RAND, /* yuv random */
	_E_PPI,
	_E_AWBC,
	_E_NR3,
};
extern uint32_t s_dbg_bypass[];
enum isp_bypass {
	_EISP_GC = 0, /* E:enum, ISP: isp */
	_EISP_NLM,
	_EISP_VST,
	_EISP_IVST,
	_EISP_CMC,
	_EISP_GAMC, /* gamma correction */
	_EISP_HSV,
	_EISP_HIST2,
	_EISP_PSTRZ,
	_EISP_PRECDN,
	_EISP_YNR,
	_EISP_EE,
	_EISP_GAMY, /* Y gamma */
	_EISP_CDN,
	_EISP_POSTCDN,
	_EISP_UVD,
	_EISP_IIRCNR,
	_EISP_YRAND, /* 17 */
	_EISP_BCHS, /* 18 */
	_EISP_YUVNF,

	_EISP_TOTAL, /* total before this */
	_EISP_LTM = 30,
	_EISP_NR3 = 31,
	/* Attention up to 31 */
};
extern uint32_t s_isp_bypass[];

#endif
