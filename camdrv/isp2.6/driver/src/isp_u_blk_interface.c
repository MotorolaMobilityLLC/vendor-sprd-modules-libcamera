/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "isp_drv.h"

typedef cmr_s32(*isp_cfg_fun_ptr) (cmr_handle handle, void *param_ptr);

struct isp_cfg_fun {
	cmr_u32 sub_block;
	isp_cfg_fun_ptr cfg_fun;
};

static struct isp_cfg_fun s_isp_cfg_fun_tab[] = {
	{ISP_BLK_BLC, dcam_u_blc_block},
	{ISP_BLK_RGB_GAIN, dcam_u_rgb_gain_block},
	{ISP_BLK_RGB_DITHER, dcam_u_rgb_dither_block},
	{ISP_BLK_2D_LSC, dcam_u_lsc_block},
	{ISP_BLK_AWB_NEW, dcam_u_awbc_block},
	{ISP_BLK_BPC, dcam_u_bpc_block},
	{ISP_BLK_RGB_AFM, dcam_u_afm_block},

	/*{ISP_BLK_BCHS, isp_u_bchs_block};*/
	{ISP_BLK_CCE, isp_u_cce_matrix_block},
	{ISP_BLK_CFA, isp_u_cfa_block},
	{ISP_BLK_CMC10, isp_u_cmc_block},
	{ISP_BLK_EDGE, isp_u_edge_block},
	{ISP_BLK_RGB_GAMC, isp_u_gamma_block},
	{ISP_BLK_GRGB, isp_u_grgb_block},
	{ISP_BLK_HIST2, isp_u_hist2_block},
	{ISP_BLK_HSV, isp_u_hsv_block},
	{ISP_BLK_IIRCNR_IIR, isp_u_iircnr_block},
	{ISP_BLK_IIRCNR_YRANDOM, isp_u_yrandom_block},
	{ISP_BLK_NLM, isp_u_nlm_block},
	{ISP_BLK_YUV_PRECDN, isp_u_yuv_precdn_block},
	{ISP_BLK_UV_POSTCDN, isp_u_yuv_postcdn_block},
	{ISP_BLK_UV_CDN, isp_u_yuv_cdn_block},
	{ISP_BLK_POSTERIZE, isp_u_posterize_block},
	{ISP_BLK_UVDIV, isp_u_uvd_block},
	{ISP_BLK_Y_GAMMC, isp_u_ygamma_block},
	{ISP_BLK_YNR, isp_u_ynr_block},
};

cmr_s32 isp_cfg_block(cmr_handle handle, void *param_ptr, cmr_u32 sub_block)
{
	cmr_s32 ret = ISP_SUCCESS;
	cmr_u32 i = 0, cnt = 0;
	isp_cfg_fun_ptr cfg_fun_ptr = PNULL;

	cnt = sizeof(s_isp_cfg_fun_tab) / sizeof(s_isp_cfg_fun_tab[0]);
	for (i = 0; i < cnt; i++) {
		if (sub_block == s_isp_cfg_fun_tab[i].sub_block) {
			cfg_fun_ptr = s_isp_cfg_fun_tab[i].cfg_fun;
			break;
		}
	}

	if (PNULL != cfg_fun_ptr) {
		ret = cfg_fun_ptr(handle, param_ptr);
	}

	return ret;
}

