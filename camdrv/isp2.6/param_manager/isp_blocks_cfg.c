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
#define LOG_TAG "isp_blk_cfg"
#ifdef WIN32
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include "cmr_types.h"
#include "isp_type.h"
#endif
#include "isp_blocks_cfg.h"
#include "isp_pm_com_type.h"
#include "isp_com_alg.h"
#include "smart_ctrl.h"
#include <cutils/properties.h>
#include "isp_video.h"

struct isp_block_operations s_blc_ops = { _pm_blc_init, _pm_blc_set_param, _pm_blc_get_param, PNULL, PNULL };
struct isp_block_operations s_rgb_gain_ops = { _pm_rgb_gain_init, _pm_rgb_gain_set_param, _pm_rgb_gain_get_param, PNULL, PNULL };
struct isp_block_operations s_rgb_dither_ops = { _pm_rgb_dither_init, _pm_rgb_dither_set_param, _pm_rgb_dither_get_param, PNULL, PNULL };
struct isp_block_operations s_2d_lsc_ops = { _pm_2d_lsc_init, _pm_2d_lsc_set_param, _pm_2d_lsc_get_param, _pm_common_rest, _pm_2d_lsc_deinit };
struct isp_block_operations s_rgb_aem_ops = { _pm_rgb_aem_init, _pm_rgb_aem_set_param, _pm_rgb_aem_get_param, PNULL, PNULL };
struct isp_block_operations s_awb_new_ops = { _pm_awb_new_init, _pm_awb_new_set_param, _pm_awb_new_get_param, PNULL, PNULL };
struct isp_block_operations s_bpc_ops = { _pm_bpc_init, _pm_bpc_set_param, _pm_bpc_get_param, PNULL, PNULL };
struct isp_block_operations s_rgb_afm_ops = { _pm_rgb_afm_init, _pm_rgb_afm_set_param, _pm_rgb_afm_get_param, PNULL, PNULL };

struct isp_block_operations s_3dnr_ops = { _pm_3dnr_init, _pm_3dnr_set_param, _pm_3dnr_get_param, PNULL, PNULL };
struct isp_block_operations s_cce_ops = { _pm_cce_init, _pm_cce_set_param, _pm_cce_get_param, PNULL, PNULL };
struct isp_block_operations s_cfa_ops = { _pm_cfa_init, _pm_cfa_set_param, _pm_cfa_get_param, PNULL, PNULL };
struct isp_block_operations s_cmc10_ops = { _pm_cmc10_init, _pm_cmc10_set_param, _pm_cmc10_get_param, PNULL, PNULL };
struct isp_block_operations s_edge_ops = { _pm_edge_init, _pm_edge_set_param, _pm_edge_get_param, PNULL, PNULL };
struct isp_block_operations s_frgb_gamc_ops = { _pm_frgb_gamc_init, _pm_frgb_gamc_set_param, _pm_frgb_gamc_get_param, PNULL, PNULL };
struct isp_block_operations s_grgb_ops = { _pm_grgb_init, _pm_grgb_set_param, _pm_grgb_get_param, PNULL, PNULL };
struct isp_block_operations s_iircnr_iir_ops = { _pm_iircnr_iir_init, _pm_iircnr_iir_set_param, _pm_iircnr_iir_get_param, PNULL, PNULL };
struct isp_block_operations s_iircnr_yrandom_ops = { _pm_iircnr_yrandom_init, _pm_iircnr_yrandom_set_param, _pm_iircnr_yrandom_get_param, PNULL, PNULL };
struct isp_block_operations s_nlm_ops = { _pm_nlm_init, _pm_nlm_set_param, _pm_nlm_get_param, _pm_common_rest, _pm_nlm_deinit };
struct isp_block_operations s_posterize_ops = { _pm_posterize_init, _pm_posterize_set_param, _pm_posterize_get_param, PNULL, PNULL };
struct isp_block_operations s_uvdiv_ops = { _pm_uv_div_init, _pm_uv_div_set_param, _pm_uv_div_get_param, PNULL, PNULL };
struct isp_block_operations s_yuv_ygamma_ops = { _pm_yuv_ygamma_init, _pm_yuv_ygamma_set_param, _pm_yuv_ygamma_get_param, PNULL, PNULL };
struct isp_block_operations s_ynr_ops = { _pm_ynr_init, _pm_ynr_set_param, _pm_ynr_get_param, PNULL, PNULL };
struct isp_block_operations s_yuv_precdn_ops = { _pm_yuv_precdn_init, _pm_yuv_precdn_set_param, _pm_yuv_precdn_get_param, PNULL, PNULL };
struct isp_block_operations s_uv_cdn_ops = { _pm_uv_cdn_init, _pm_uv_cdn_set_param, _pm_uv_cdn_get_param, PNULL, PNULL };
struct isp_block_operations s_uv_postcdn_ops = { _pm_uv_postcdn_init, _pm_uv_postcdn_set_param, _pm_uv_postcdn_get_param, PNULL, PNULL };
struct isp_block_operations s_yuv_noisefilter_ops = { _pm_yuv_noisefilter_init, _pm_yuv_noisefilter_set_param, _pm_yuv_noisefilter_get_param, PNULL, PNULL };

struct isp_block_operations s_cnr2_ops = { _pm_cnr2_init, _pm_cnr2_set_param, _pm_cnr2_get_param, PNULL, PNULL };

#ifdef CONFIG_ISP_2_5
struct isp_block_operations s_hsv_ops = { _pm_hsv_init, _pm_hsv_set_param, _pm_hsv_get_param, _pm_common_rest, _pm_hsv_deinit };
struct isp_block_operations s_hsv_new_ops = { _pm_hsv_new_init, _pm_hsv_new_set_param, _pm_hsv_new_get_param, _pm_common_rest, _pm_hsv_new_deinit };
struct isp_block_operations s_bright_ops = { _pm_brightness_init, _pm_brightness_set_param, _pm_brightness_get_param, PNULL, PNULL };
struct isp_block_operations s_contrast_ops = { _pm_contrast_init, _pm_contrast_set_param, _pm_contrast_get_param, PNULL, PNULL };
struct isp_block_operations s_saturation_ops = { _pm_saturation_init, _pm_saturation_set_param, _pm_saturation_get_param, PNULL, PNULL };
struct isp_block_operations s_hue_ops = { _pm_hue_init, _pm_hue_set_param, _pm_hue_get_param, PNULL, PNULL };
struct isp_block_operations s_ynrs_ops = { _pm_ynrs_init, _pm_ynrs_set_param, _pm_ynrs_get_param, PNULL, PNULL };
#endif

#ifdef CONFIG_ISP_2_6
struct isp_block_operations s_hsv_ops = { _pm_hsv_init, _pm_hsv_set_param, _pm_hsv_get_param, _pm_common_rest, _pm_hsv_deinit };
struct isp_block_operations s_ppe_ops = { _pm_ppe_init, _pm_ppe_set_param, _pm_ppe_get_param, PNULL, PNULL };
struct isp_block_operations s_bchs_ops = { _pm_bchs_init, _pm_bchs_set_param, _pm_bchs_get_param, PNULL, PNULL };
struct isp_block_operations s_sw3dnr_ops = { _pm_sw3dnr_init, _pm_sw3dnr_set_param, _pm_sw3dnr_get_param, PNULL, PNULL };
struct isp_block_operations s_ltm_ops = { _pm_ltm_init, _pm_ltm_set_param, _pm_ltm_get_param, PNULL, PNULL };
struct isp_block_operations s_imblance_ops = { _pm_imblance_init, _pm_imblance_set_param, _pm_imblance_get_param, PNULL, PNULL};
#endif

#ifdef CONFIG_ISP_2_7
struct isp_block_operations s_hsv_new2_ops = { _pm_hsv_new2_init, _pm_hsv_new2_set_param, _pm_hsv_new2_get_param, _pm_common_rest, PNULL};
struct isp_block_operations s_ppe_ops = { _pm_ppe_init, _pm_ppe_set_param, _pm_ppe_get_param, PNULL, PNULL };
struct isp_block_operations s_bchs_ops = { _pm_bchs_init, _pm_bchs_set_param, _pm_bchs_get_param, PNULL, PNULL };
struct isp_block_operations s_sw3dnr_ops = { _pm_sw3dnr_init, _pm_sw3dnr_set_param, _pm_sw3dnr_get_param, PNULL, PNULL };
struct isp_block_operations s_imblance_ops = { _pm_imblance_init, _pm_imblance_set_param, _pm_imblance_get_param, PNULL, PNULL};
struct isp_block_operations s_ynrs_ops = { _pm_ynrs_init, _pm_ynrs_set_param, _pm_ynrs_get_param, PNULL, PNULL };
#endif

#ifdef CONFIG_ISP_2_5
struct isp_block_cfg s_blk_cfgs[] = {
	/* ======== dcam blocks list starts ======= */
	{ISP_BLK_BLC, array_offset(struct isp_context, blc), sizeof(struct isp_blc_param), &s_blc_ops},
	{ISP_BLK_RGB_GAIN, array_offset(struct isp_context, rgb_gain), sizeof(struct isp_rgb_gain_param), &s_rgb_gain_ops},
	{ISP_BLK_2D_LSC, array_offset(struct isp_context, lsc_2d), sizeof(struct isp_2d_lsc_param), &s_2d_lsc_ops},
	{ISP_BLK_RGB_AEM, array_offset(struct isp_context, aem), sizeof(struct isp_rgb_aem_param), &s_rgb_aem_ops},
	{ISP_BLK_AWB_NEW, array_offset(struct isp_context, awb), sizeof(struct isp_awb_param), &s_awb_new_ops},
	{DCAM_BLK_RGB_AFM, array_offset(struct isp_context, afm), sizeof(struct isp_rgb_afm_param), &s_rgb_afm_ops},
	{DCAM_BLK_BPC, array_offset(struct isp_context, bpc), sizeof(struct isp_bpc_param), &s_bpc_ops},
	{ISP_BLK_RGB_DITHER, array_offset(struct isp_context, rgb_dither), sizeof(struct isp_rgb_dither_param), &s_rgb_dither_ops},
	{ISP_BLK_GRGB, array_offset(struct isp_context, grgb), sizeof(struct isp_grgb_param), &s_grgb_ops},

	/* ======== isp blocks list starts ======= */
	{ISP_BLK_CCE, array_offset(struct isp_context, cce), sizeof(struct isp_cce_param), &s_cce_ops},
	{ISP_BLK_CMC10, array_offset(struct isp_context, cmc10), sizeof(struct isp_cmc10_param), &s_cmc10_ops},
	{ISP_BLK_RGB_GAMC, array_offset(struct isp_context, rgb_gamma), sizeof(struct isp_frgb_gamc_param), &s_frgb_gamc_ops},
	{ISP_BLK_HSV, array_offset(struct isp_context, hsv), sizeof(struct isp_hsv_param), &s_hsv_ops},
	{ISP_BLK_HSV_NEW, array_offset(struct isp_context, hsv_new), sizeof(struct isp_hsv_param_new), &s_hsv_new_ops},

	{ISP_BLK_IIRCNR_YRANDOM, array_offset(struct isp_context, yrandom), sizeof(struct isp_iircnr_yrandom_param), &s_iircnr_yrandom_ops},
	{ISP_BLK_Y_GAMMC, array_offset(struct isp_context, ygamma), sizeof(struct isp_yuv_ygamma_param), &s_yuv_ygamma_ops},
	{ISP_BLK_POSTERIZE, array_offset(struct isp_context, posterize), sizeof(struct isp_posterize_param), &s_posterize_ops},
	{DCAM_BLK_3DNR_PRE, array_offset(struct isp_context, nr3d), sizeof(struct isp_nr3d_param), &s_3dnr_ops},
	{DCAM_BLK_3DNR_CAP, array_offset(struct isp_context, nr3d), sizeof(struct isp_nr3d_param), &s_3dnr_ops},
	{ISP_BLK_BRIGHT, array_offset(struct isp_context, bchs.brigntness), sizeof(struct isp_bright_param), &s_bright_ops},
	{ISP_BLK_CONTRAST, array_offset(struct isp_context, bchs.contrast), sizeof(struct isp_contrast_param), &s_contrast_ops},
	{ISP_BLK_SATURATION, array_offset(struct isp_context, bchs.saturation), sizeof(struct isp_chrom_saturation_param), &s_saturation_ops},
	{ISP_BLK_HUE, array_offset(struct isp_context, bchs.hue_v0), sizeof(struct isp_hue_param), &s_hue_ops},
	{DCAM_BLK_NLM, array_offset(struct isp_context, nlm), sizeof(struct isp_nlm_param), &s_nlm_ops},
	{ISP_BLK_UVDIV, array_offset(struct isp_context, uvd), sizeof(struct isp_uvdiv_param), &s_uvdiv_ops},
	{ISP_BLK_CFA, array_offset(struct isp_context, cfa), sizeof(struct isp_cfa_param), &s_cfa_ops},
	{ISP_BLK_YUV_PRECDN, array_offset(struct isp_context, pre_cdn), sizeof(struct isp_yuv_precdn_param), &s_yuv_precdn_ops},
	{ISP_BLK_YNR, array_offset(struct isp_context, ynr), sizeof(struct isp_ynr_param), &s_ynr_ops},
	{ISP_BLK_EDGE, array_offset(struct isp_context, edge), sizeof(struct isp_edge_param), &s_edge_ops},
	{ISP_BLK_UV_CDN, array_offset(struct isp_context, cdn), sizeof(struct isp_uv_cdn_param), &s_uv_cdn_ops},
	{ISP_BLK_UV_POSTCDN, array_offset(struct isp_context, post_cdn), sizeof(struct isp_uv_postcdn_param), &s_uv_postcdn_ops},
	{ISP_BLK_IIRCNR_IIR, array_offset(struct isp_context, iircnr), sizeof(struct isp_iircnr_iir_param), &s_iircnr_iir_ops},
	{ISP_BLK_YUV_NOISEFILTER, array_offset(struct isp_context, noisefilter), sizeof(struct isp_dev_noise_filter_param), &s_yuv_noisefilter_ops},

	/* ======== soft algo blocks list starts ======= */
	{ISP_BLK_CNR2, array_offset(struct isp_context, cnr2), sizeof(struct isp_cnr2_param), &s_cnr2_ops},
	{ISP_BLK_YNRS, array_offset(struct isp_context, ynrs), sizeof(struct isp_ynrs_param), &s_ynrs_ops},
};
#elif defined CONFIG_ISP_2_6
struct isp_block_cfg s_blk_cfgs[] = {
	/* ======== dcam blocks list starts ======= */
	{ISP_BLK_BLC, array_offset(struct isp_context, blc), sizeof(struct isp_blc_param), &s_blc_ops},
	{ISP_BLK_RGB_GAIN, array_offset(struct isp_context, rgb_gain), sizeof(struct isp_rgb_gain_param), &s_rgb_gain_ops},
	{ISP_BLK_2D_LSC, array_offset(struct isp_context, lsc_2d), sizeof(struct isp_2d_lsc_param), &s_2d_lsc_ops},
	{ISP_BLK_RGB_AEM, array_offset(struct isp_context, aem), sizeof(struct isp_rgb_aem_param), &s_rgb_aem_ops},
	{ISP_BLK_AWB_NEW, array_offset(struct isp_context, awb), sizeof(struct isp_awb_param), &s_awb_new_ops},
	{DCAM_BLK_BPC_V1, array_offset(struct isp_context, bpc), sizeof(struct isp_bpc_param), &s_bpc_ops},
	{DCAM_BLK_PPE, array_offset(struct isp_context, ppe), sizeof(struct isp_ppe_param), &s_ppe_ops},
	{DCAM_BLK_RGB_AFM_V1, array_offset(struct isp_context, afm), sizeof(struct isp_rgb_afm_param), &s_rgb_afm_ops},
	{DCAM_BLK_RGB_DITHER, array_offset(struct isp_context, rgb_dither), sizeof(struct isp_rgb_dither_param), &s_rgb_dither_ops},

	/* ======== isp blocks list starts ======= */
	{ISP_BLK_CCE, array_offset(struct isp_context, cce), sizeof(struct isp_cce_param), &s_cce_ops},
	{ISP_BLK_CMC10, array_offset(struct isp_context, cmc10), sizeof(struct isp_cmc10_param), &s_cmc10_ops},
	{ISP_BLK_RGB_GAMC, array_offset(struct isp_context, rgb_gamma), sizeof(struct isp_frgb_gamc_param), &s_frgb_gamc_ops},
	{ISP_BLK_HSV, array_offset(struct isp_context, hsv), sizeof(struct isp_hsv_param), &s_hsv_ops},
	{ISP_BLK_IIRCNR_YRANDOM, array_offset(struct isp_context, yrandom), sizeof(struct isp_iircnr_yrandom_param), &s_iircnr_yrandom_ops},
	{ISP_BLK_Y_GAMMC, array_offset(struct isp_context, ygamma), sizeof(struct isp_yuv_ygamma_param), &s_yuv_ygamma_ops},
	{ISP_BLK_POSTERIZE, array_offset(struct isp_context, posterize), sizeof(struct isp_posterize_param), &s_posterize_ops},
	{ISP_BLK_3DNR, array_offset(struct isp_context, nr3d), sizeof(struct isp_nr3d_param), &s_3dnr_ops},
	{ISP_BLK_BCHS, array_offset(struct isp_context, bchs), sizeof(struct isp_bchs_param), &s_bchs_ops},
	{ISP_BLK_CFA_V1, array_offset(struct isp_context, cfa), sizeof(struct isp_cfa_param), &s_cfa_ops},
	{ISP_BLK_EE_V1, array_offset(struct isp_context, edge), sizeof(struct isp_edge_param), &s_edge_ops},
	{ISP_BLK_GRGB_V1, array_offset(struct isp_context, grgb), sizeof(struct isp_grgb_param), &s_grgb_ops},
	{ISP_BLK_IIRCNR_IIR_V1, array_offset(struct isp_context, iircnr), sizeof(struct isp_iircnr_iir_param), &s_iircnr_iir_ops},
	{ISP_BLK_LTM, array_offset(struct isp_context, ltm), sizeof(struct isp_ltm_param), &s_ltm_ops},
	{ISP_BLK_NLM_V1, array_offset(struct isp_context, nlm), sizeof(struct isp_nlm_param), &s_nlm_ops},
	{ISP_BLK_IMBALANCE, array_offset(struct isp_context, imblance), sizeof(struct isp_imblance_param), &s_imblance_ops},
	{ISP_BLK_UVDIV_V1, array_offset(struct isp_context, uvd), sizeof(struct isp_uvdiv_param), &s_uvdiv_ops},
	{ISP_BLK_YNR_V1, array_offset(struct isp_context, ynr), sizeof(struct isp_ynr_param), &s_ynr_ops},
	{ISP_BLK_YUV_PRECDN_V1, array_offset(struct isp_context, pre_cdn), sizeof(struct isp_yuv_precdn_param), &s_yuv_precdn_ops},
	{ISP_BLK_UV_CDN_V1, array_offset(struct isp_context, cdn), sizeof(struct isp_uv_cdn_param), &s_uv_cdn_ops},
	{ISP_BLK_UV_POSTCDN_V1, array_offset(struct isp_context, post_cdn), sizeof(struct isp_uv_postcdn_param), &s_uv_postcdn_ops},
	{ISP_BLK_YUV_NOISEFILTER_V1, array_offset(struct isp_context, noisefilter), sizeof(struct isp_dev_noise_filter_param), &s_yuv_noisefilter_ops},

	/* ======== soft algo blocks list starts ======= */
	{ISP_BLK_CNR2_V1, array_offset(struct isp_context, cnr2), sizeof(struct isp_cnr2_param), &s_cnr2_ops},
	{ISP_BLK_SW3DNR, array_offset(struct isp_context, sw3dnr), sizeof(struct isp_sw3dnr_param), &s_sw3dnr_ops},
};
#elif defined CONFIG_ISP_2_7
struct isp_block_cfg s_blk_cfgs[] = {
	/* ======== dcam blocks list starts ======= */
	{ISP_BLK_BLC, array_offset(struct isp_context, blc), sizeof(struct isp_blc_param), &s_blc_ops},
	{ISP_BLK_RGB_GAIN, array_offset(struct isp_context, rgb_gain), sizeof(struct isp_rgb_gain_param), &s_rgb_gain_ops},
	{ISP_BLK_2D_LSC, array_offset(struct isp_context, lsc_2d), sizeof(struct isp_2d_lsc_param), &s_2d_lsc_ops},
	{ISP_BLK_RGB_AEM, array_offset(struct isp_context, aem), sizeof(struct isp_rgb_aem_param), &s_rgb_aem_ops},
	{ISP_BLK_AWB_NEW, array_offset(struct isp_context, awb), sizeof(struct isp_awb_param), &s_awb_new_ops},
	{DCAM_BLK_BPC_V1, array_offset(struct isp_context, bpc), sizeof(struct isp_bpc_param), &s_bpc_ops},
	{ISP_BLK_PPE_V1, array_offset(struct isp_context, ppe), sizeof(struct isp_ppe_param), &s_ppe_ops},
	{DCAM_BLK_RGB_AFM_V1, array_offset(struct isp_context, afm), sizeof(struct isp_rgb_afm_param), &s_rgb_afm_ops},
	{DCAM_BLK_RGB_DITHER, array_offset(struct isp_context, rgb_dither), sizeof(struct isp_rgb_dither_param), &s_rgb_dither_ops},

	/* ======== isp blocks list starts ======= */
	{ISP_BLK_CCE, array_offset(struct isp_context, cce), sizeof(struct isp_cce_param), &s_cce_ops},
	{ISP_BLK_CMC10, array_offset(struct isp_context, cmc10), sizeof(struct isp_cmc10_param), &s_cmc10_ops},
	{ISP_BLK_RGB_GAMC, array_offset(struct isp_context, rgb_gamma), sizeof(struct isp_frgb_gamc_param), &s_frgb_gamc_ops},
	{ISP_BLK_HSV_NEW2, array_offset(struct isp_context, hsv_new2), sizeof(struct isp_hsv_param_new2), &s_hsv_new2_ops},
	{ISP_BLK_IIRCNR_YRANDOM, array_offset(struct isp_context, yrandom), sizeof(struct isp_iircnr_yrandom_param), &s_iircnr_yrandom_ops},
	{ISP_BLK_Y_GAMMC, array_offset(struct isp_context, ygamma), sizeof(struct isp_yuv_ygamma_param), &s_yuv_ygamma_ops},
	{ISP_BLK_POSTERIZE, array_offset(struct isp_context, posterize), sizeof(struct isp_posterize_param), &s_posterize_ops},
	{ISP_BLK_3DNR, array_offset(struct isp_context, nr3d), sizeof(struct isp_nr3d_param), &s_3dnr_ops},
	{ISP_BLK_BCHS, array_offset(struct isp_context, bchs), sizeof(struct isp_bchs_param), &s_bchs_ops},
	{ISP_BLK_CFA_V1, array_offset(struct isp_context, cfa), sizeof(struct isp_cfa_param), &s_cfa_ops},
	{ISP_BLK_EE_V1, array_offset(struct isp_context, edge), sizeof(struct isp_edge_param), &s_edge_ops},
	{ISP_BLK_GRGB_V1, array_offset(struct isp_context, grgb), sizeof(struct isp_grgb_param), &s_grgb_ops},
	{ISP_BLK_IIRCNR_IIR_V1, array_offset(struct isp_context, iircnr), sizeof(struct isp_iircnr_iir_param), &s_iircnr_iir_ops},
	{ISP_BLK_NLM_V2, array_offset(struct isp_context, nlm), sizeof(struct isp_nlm_param), &s_nlm_ops},
	{ISP_BLK_IMBALANCE_V1, array_offset(struct isp_context, imblance), sizeof(struct isp_imblance_param), &s_imblance_ops},
	{ISP_BLK_UVDIV_V1, array_offset(struct isp_context, uvd), sizeof(struct isp_uvdiv_param), &s_uvdiv_ops},
	{ISP_BLK_YNR_V1, array_offset(struct isp_context, ynr), sizeof(struct isp_ynr_param), &s_ynr_ops},
	{ISP_BLK_YUV_PRECDN_V1, array_offset(struct isp_context, pre_cdn), sizeof(struct isp_yuv_precdn_param), &s_yuv_precdn_ops},
	{ISP_BLK_UV_CDN_V1, array_offset(struct isp_context, cdn), sizeof(struct isp_uv_cdn_param), &s_uv_cdn_ops},
	{ISP_BLK_UV_POSTCDN_V1, array_offset(struct isp_context, post_cdn), sizeof(struct isp_uv_postcdn_param), &s_uv_postcdn_ops},
	{ISP_BLK_YUV_NOISEFILTER_V1, array_offset(struct isp_context, noisefilter), sizeof(struct isp_dev_noise_filter_param), &s_yuv_noisefilter_ops},

	/* ======== soft algo blocks list starts ======= */
	{ISP_BLK_CNR2_V1, array_offset(struct isp_context, cnr2), sizeof(struct isp_cnr2_param), &s_cnr2_ops},
	{ISP_BLK_SW3DNR, array_offset(struct isp_context, sw3dnr), sizeof(struct isp_sw3dnr_param), &s_sw3dnr_ops},
	{ISP_BLK_YNRS, array_offset(struct isp_context, ynrs), sizeof(struct isp_ynrs_param), &s_ynrs_ops},
};
#endif


struct isp_block_cfg *isp_pm_get_block_cfg(cmr_u32 id)
{
	cmr_u32 num = 0;
	cmr_u32 i = 0;
	cmr_u32 blk_id = 0;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_block_cfg *blk_cfg_array = s_blk_cfgs;

	num = sizeof(s_blk_cfgs) / sizeof(struct isp_block_cfg);
	for (i = 0; i < num; ++i) {
		blk_id = blk_cfg_array[i].id;
		if (blk_id == id) {
			break;
		}
	}

	if (i < num) {
		blk_cfg_ptr = &blk_cfg_array[i];
	} else {
		blk_cfg_ptr = PNULL;
	}

	return blk_cfg_ptr;
}
