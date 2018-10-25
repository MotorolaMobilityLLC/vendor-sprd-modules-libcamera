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
#ifndef _DCAM_BLKPARAM_H_
#define _DCAM_BLKPARAM_H_

#include <sprd_isp_r8p1.h>

#define DCAM_ONLINE_MODE		(1)


struct dcam_k_block {
	uint32_t lsc_init;
	unsigned long lsc_gridtab_haddr;
	void *lsc_weight_tab;
	uint32_t weight_tab_size;
	struct camera_buf lsc_buf;
};

struct dcam_dev_lsc_param {
	uint32_t update;
	struct dcam_dev_lsc_info lens_info;
	struct dcam_dev_lsc_buf buf_info;
	struct dcam_k_block blk_handle;
};

struct dcam_dev_blc_param {
	uint32_t update;
	struct dcam_dev_blc_info blc_info;
};

struct dcam_dev_rgb_param {
	uint32_t update;
	struct dcam_dev_rgb_gain_info gain_info;
	struct dcam_dev_rgb_dither_info rgb_dither;
};

struct dcam_dev_pdaf_param {
	uint32_t update;
};

struct dcam_dev_hist_param {
	uint32_t update;
	struct dcam_dev_hist_info bayerHist_info;
};

struct dcam_dev_aem_param {
	uint32_t update;
	uint32_t bypass;
	uint32_t mode;
	struct dcam_dev_aem_win win_info;
	uint32_t skip_num;
	struct dcam_dev_aem_thr aem_info;
};

struct dcam_dev_afl_param {
	uint32_t update;
	struct isp_dev_anti_flicker_new_info afl_info;
	uint32_t bypass;
};

struct dcam_dev_awbc_param {
	uint32_t update;
	struct dcam_dev_awbc_info awbc_info;
	struct img_rgb_info awbc_gain;
	uint32_t bypass;
};

struct dcam_dev_bpc_param {
	uint32_t update;
	struct dcam_dev_bpc_info bpc_info;
	uint32_t bad_pixel_num_bk; /* no use now 180914 */
};
struct dcam_dev_3dnr_param {
	uint32_t update;
	struct dcam_dev_3dnr_me nr3_me;
};

struct dcam_dev_afm_param {
	uint32_t update;
	uint32_t bypass;
	struct dcam_dev_afm_info af_param;
	struct isp_img_rect win;
	struct isp_img_size win_num;
	uint32_t mode;
	uint32_t skip_num;
	uint32_t crop_eb;
	struct isp_img_rect crop_size;
	struct isp_img_size done_tile_num;
};

struct dcam_dev_param {
	struct mutex param_lock;
	uint32_t idx; /* dcam dev idx */

	struct dcam_dev_lsc_param lsc;
	struct dcam_dev_blc_param blc;
	struct dcam_dev_rgb_param rgb;
	struct dcam_dev_pdaf_param pdaf;
	struct dcam_dev_hist_param hist;
	struct dcam_dev_aem_param aem;
	struct dcam_dev_afl_param afl;
	struct dcam_dev_awbc_param awbc;
	struct dcam_dev_bpc_param bpc;
	struct dcam_dev_3dnr_param nr3;
	struct dcam_dev_afm_param afm;
};

typedef int (*FUNC_DCAM_PARAM)(struct dcam_dev_param *param);

#endif
