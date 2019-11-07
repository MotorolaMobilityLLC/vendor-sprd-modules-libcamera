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
	/* dcam blocks for common */
	{ISP_BLK_BLC, dcam_u_blc_block},
	{ISP_BLK_RGB_GAIN, dcam_u_rgb_gain_block},
	{ISP_BLK_2D_LSC, dcam_u_lsc_block},
	{ISP_BLK_AWB_NEW, dcam_u_awbc_block},
	/* dcam blocks for sharkl5 */
	{DCAM_BLK_RGB_DITHER, dcam_u_rgb_dither_block},
	{DCAM_BLK_BPC_V1, dcam_u_bpc_block},
	{DCAM_BLK_PPE, dcam_u_bpc_ppe},
	{DCAM_BLK_RGB_AFM_V1, dcam_u_afm_block},
	/* dcam blocks for sharkl3 */
	{ISP_BLK_RGB_DITHER, dcam_u_rgb_dither_block},
	{DCAM_BLK_BPC, dcam_u_bpc_block},
	{DCAM_BLK_RGB_AFM, dcam_u_afm_block},
	{ISP_BLK_GRGB, dcam_u_grgb_block},

	/* isp blocks common */
	{ISP_BLK_CCE, isp_u_cce_matrix_block},
	{ISP_BLK_CMC10, isp_u_cmc_block},
	{ISP_BLK_RGB_GAMC, isp_u_gamma_block},
	{ISP_BLK_HIST, isp_u_hist_block},
	{ISP_BLK_HIST2, isp_u_hist2_block},
	{ISP_BLK_HSV, isp_u_hsv_block},
	{ISP_BLK_IIRCNR_YRANDOM, isp_u_yrandom_block},
	{ISP_BLK_POSTERIZE, isp_u_posterize_block},
	{ISP_BLK_Y_GAMMC, isp_u_ygamma_block},

	/* isp blocks for sharkl5 */
	{ISP_BLK_3DNR, isp_u_3dnr_block},
	{ISP_BLK_BCHS, isp_u_bchs_block},
	{ISP_BLK_CFA_V1, isp_u_cfa_block},
	{ISP_BLK_EE_V1, isp_u_edge_block},
	{ISP_BLK_GRGB_V1, isp_u_grgb_block},
	{ISP_BLK_IIRCNR_IIR_V1, isp_u_iircnr_block},
	{ISP_BLK_LTM, isp_u_ltm_block},
	{ISP_BLK_NLM_V1, isp_u_nlm_block},
	{ISP_BLK_IMBALANCE, isp_u_nlm_imblance},
	{ISP_BLK_YUV_PRECDN_V1, isp_u_yuv_precdn_block},
	{ISP_BLK_UV_POSTCDN_V1, isp_u_yuv_postcdn_block},
	{ISP_BLK_UV_CDN_V1, isp_u_yuv_cdn_block},
	{ISP_BLK_UVDIV_V1, isp_u_uvd_block},
	{ISP_BLK_YNR_V1, isp_u_ynr_block},
	{ISP_BLK_YUV_NOISEFILTER_V1, isp_u_noisefilter_block},

	/* isp blocks for sharkl3 */
	{ISP_BLK_HSV_NEW, isp_u_hsv_block},
	{ISP_BLK_HSV_NEW2, isp_u_hsv_block},
	{ISP_BLK_BRIGHT, isp_u_brightness_block},
	{ISP_BLK_CONTRAST, isp_u_contrast_block},
	{ISP_BLK_SATURATION, isp_u_csa_block},
	{ISP_BLK_HUE, isp_u_hue_block},
	{DCAM_BLK_3DNR_PRE, isp_u_3dnr_block},
	{DCAM_BLK_3DNR_CAP, isp_u_3dnr_block},
	{DCAM_BLK_NLM, isp_u_nlm_block},
	{ISP_BLK_UVDIV, isp_u_uvd_block},
	{ISP_BLK_CFA, isp_u_cfa_block},
	{ISP_BLK_YUV_PRECDN, isp_u_yuv_precdn_block},
	{ISP_BLK_YNR, isp_u_ynr_block},
	{ISP_BLK_EDGE, isp_u_edge_block},
	{ISP_BLK_UV_CDN, isp_u_yuv_cdn_block},
	{ISP_BLK_UV_POSTCDN, isp_u_yuv_postcdn_block},
	{ISP_BLK_IIRCNR_IIR, isp_u_iircnr_block},
	{ISP_BLK_YUV_NOISEFILTER, isp_u_noisefilter_block},
	/* isp blocks for sharkl5 pro */
	{ISP_BLK_PPE_V1, dcam_u_bpc_ppe},
	{ISP_BLK_IMBALANCE_V1, isp_u_nlm_imblance},
	{ISP_BLK_NLM_V2, isp_u_nlm_block},
	{ISP_BLK_Y_GAMMC_V1, isp_u_ygamma_block},
	{ISP_BLK_RAW_GTM, dcam_u_raw_gtm_block},
	{ISP_BLK_YUV_LTM, isp_u_yuv_ltm_block},
	{ISP_BLK_RGB_LTM, isp_u_rgb_ltm_block},
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

