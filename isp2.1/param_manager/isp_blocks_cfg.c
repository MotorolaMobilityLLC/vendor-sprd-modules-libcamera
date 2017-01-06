/*
 * Copyright (C) 2012 The Android Open Source Project
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
#ifdef WIN32
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include "isp_type.h"
#endif
#include "isp_blocks_cfg.h"
#include "isp_com.h"
#include "isp_pm_com_type.h"
#include "isp_com_alg.h"
#include "smart_ctrl.h"
#include "isp_otp_calibration.h"
#include <cutils/properties.h>
#include "isp_video.h"

/***************************************************************************************/
struct isp_block_operations s_bright_ops = {_pm_brightness_init, _pm_brightness_set_param, _pm_brightness_get_param, PNULL, PNULL};
struct isp_block_operations s_contrast_ops = {_pm_contrast_init, _pm_contrast_set_param, _pm_contrast_get_param, PNULL, PNULL};
struct isp_block_operations s_flash_ops = {_pm_flashlight_init, _pm_flashlight_set_param, _pm_flashlight_get_param, PNULL, PNULL};
struct isp_block_operations s_envi_detect_ops = {_pm_envi_detect_init, _pm_envi_detect_set_param, _pm_envi_detect_get_param, PNULL, PNULL};

/************************************Tshark2*******************************************************/
struct isp_block_operations s_pre_gbl_gain_ops_v1 = {_pm_pre_gbl_gain_init_v1,_pm_pre_gbl_gain_set_param_v1,_pm_pre_gbl_gain_get_param_v1, PNULL,PNULL};
struct isp_block_operations s_blc_ops_v1 = {_pm_blc_init,_pm_blc_set_param,_pm_blc_get_param, PNULL, PNULL};
struct isp_block_operations s_postblc_ops = {_pm_postblc_init,_pm_postblc_set_param,_pm_postblc_get_param, PNULL, PNULL};
struct isp_block_operations s_rgb_gain_ops_v1 = {_pm_rgb_gain_init_v1,_pm_rgb_gain_set_param_v1,_pm_rgb_gain_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_nlc_ops_v1 = {_pm_nlc_init_v1,_pm_nlc_set_param_v1,_pm_nlc_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_2d_lsc_ops = {_pm_2d_lsc_init,_pm_2d_lsc_set_param,_pm_2d_lsc_get_param, _pm_common_rest, _pm_2d_lsc_deinit};
struct isp_block_operations s_1d_lsc_ops = {_pm_1d_lsc_init,_pm_1d_lsc_set_param,_pm_1d_lsc_get_param, PNULL, PNULL};
struct isp_block_operations s_binning4awb_ops_v1 = {_pm_binning4awb_init_v1,_pm_binning4awb_set_param_v1,_pm_binning4awb_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_bpc_ops_v1 = {_pm_bpc_init_v1,_pm_bpc_set_param_v1,_pm_bpc_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_grgb_ops_v1 = {_pm_grgb_init_v1,_pm_grgb_set_param_v1,_pm_grgb_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_awb_ops_v1 = {_pm_awb_init_v1,_pm_awb_set_param_v1,_pm_awb_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_ynr_ops = {_pm_ynr_init,_pm_ynr_set_param,_pm_ynr_get_param, PNULL, PNULL};
struct isp_block_operations s_pdaf_extraction_ops = {_pm_pdaf_extraction_init,_pm_pdaf_extraction_set_param,_pm_pdaf_extraction_get_param, PNULL, PNULL};
struct isp_block_operations s_pdaf_correct_ops = {_pm_pdaf_correct_init,_pm_pdaf_correct_set_param,_pm_pdaf_correct_get_param, PNULL, PNULL};
struct isp_block_operations s_nlm_ops_v1 = {_pm_nlm_init,_pm_nlm_set_param,_pm_nlm_get_param, _pm_common_rest, _pm_nlm_deinit};
struct isp_block_operations s_cfa_ops_v1= {_pm_cfa_init_v1,_pm_cfa_set_param_v1,_pm_cfa_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_cmc10_ops= {_pm_cmc10_init,_pm_cmc10_set_param,_pm_cmc10_get_param, PNULL, PNULL};
struct isp_block_operations s_hsv_ops = {_pm_hsv_init,_pm_hsv_set_param,_pm_hsv_get_param, _pm_common_rest, _pm_hsv_deinit};
struct isp_block_operations s_posterize_ops= {_pm_posterize_init,_pm_posterize_set_param,_pm_posterize_get_param, PNULL, PNULL};
struct isp_block_operations s_frgb_gamc_ops= {_pm_frgb_gamc_init,_pm_frgb_gamc_set_param,_pm_frgb_gamc_get_param, PNULL, PNULL};
struct isp_block_operations s_rgb_aem_ops= {_pm_rgb_aem_init,_pm_rgb_aem_set_param,_pm_rgb_aem_get_param, PNULL, PNULL};
struct isp_block_operations s_rgb_afm_ops= {_pm_rgb_afm_init,_pm_rgb_afm_set_param,_pm_rgb_afm_get_param, PNULL, PNULL};
struct isp_block_operations s_yiq_afl_ops_v1 = {_pm_yiq_afl_init_v1,_pm_yiq_afl_set_param_v1,_pm_yiq_afl_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_yiq_afl_ops_v3 = {_pm_yiq_afl_init_v3,_pm_yiq_afl_set_param_v3,_pm_yiq_afl_get_param_v3, PNULL, PNULL};
struct isp_block_operations s_cce_ops_v1 = {_pm_cce_init_v1,_pm_cce_set_param_v1,_pm_cce_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_yuv_precdn_ops = {_pm_yuv_precdn_init, _pm_yuv_precdn_set_param, _pm_yuv_precdn_get_param, PNULL, PNULL};
struct isp_block_operations s_hist_ops_v1 = {_pm_hist_init_v1, _pm_hist_set_param_v1, _pm_hist_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_hist2_ops_v1 = {_pm_hist2_init_v1, _pm_hist2_set_param_v1, _pm_hist2_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_uv_cdn_ops_v1 = {_pm_uv_cdn_init_v1, _pm_uv_cdn_set_param_v1, _pm_uv_cdn_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_rgb_dither_ops = {_pm_rgb_dither_init, _pm_rgb_dither_set_param, _pm_rgb_dither_get_param, PNULL, PNULL};
struct isp_block_operations s_edge_ops_v1 = {_pm_edge_init_v1, _pm_edge_set_param_v1, _pm_edge_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_saturation_ops_v1= {_pm_saturation_init_v1,_pm_saturation_set_param_v1,_pm_saturation_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_hue_ops_v1= {_pm_hue_init_v1,_pm_hue_set_param_v1,_pm_hue_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_uv_postcdn_ops= {_pm_uv_postcdn_init,_pm_uv_postcdn_set_param,_pm_uv_postcdn_get_param, PNULL, PNULL};
struct isp_block_operations s_yuv_ygamma_ops = {_pm_yuv_ygamma_init,_pm_yuv_ygamma_set_param,_pm_yuv_ygamma_get_param, PNULL, PNULL};
struct isp_block_operations s_ydelay_ops= {_pm_ydelay_init,_pm_ydelay_set_param,_pm_ydelay_get_param, PNULL, PNULL};
struct isp_block_operations s_iircnr_iir_ops= {_pm_iircnr_iir_init,_pm_iircnr_iir_set_param,_pm_iircnr_iir_get_param, PNULL, PNULL};
struct isp_block_operations s_iircnr_yrandom_ops= {_pm_iircnr_yrandom_init,_pm_iircnr_yrandom_set_param,_pm_iircnr_yrandom_get_param, PNULL, PNULL};
struct isp_block_operations s_uvdiv_ops_v1 = {_pm_uv_div_init_v1, _pm_uv_div_set_param_v1, _pm_uv_div_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_smart_ops = {_pm_smart_init, _pm_smart_set_param, _pm_smart_get_param, PNULL, PNULL};
struct isp_block_operations s_sft_af_ops = {_pm_sft_af_init, _pm_sft_af_set_param, _pm_sft_af_get_param, PNULL, PNULL};
struct isp_block_operations s_alsc_ops = {_pm_alsc_init, _pm_alsc_set_param, _pm_alsc_get_param, PNULL, PNULL};
struct isp_block_operations s_aft_ops = {_pm_aft_init, _pm_aft_set_param, _pm_aft_get_param, PNULL, PNULL};
struct isp_block_operations s_ae_new_ops = {_pm_ae_new_init, _pm_ae_new_set_param, _pm_ae_new_get_param, PNULL, PNULL};
struct isp_block_operations s_awb_new_ops = {_pm_awb_new_init, _pm_awb_new_set_param, _pm_awb_new_get_param, PNULL, PNULL};
struct isp_block_operations s_af_new_ops = {_pm_af_new_init, _pm_af_new_set_param, _pm_af_new_get_param, PNULL, PNULL};
struct isp_block_operations s_3d_nr_pre_ops = {_pm_3d_nr_pre_init, _pm_3d_nr_pre_set_param, _pm_3d_nr_pre_get_param, PNULL, PNULL};
struct isp_block_operations s_3d_nr_cap_ops = {_pm_3d_nr_cap_init, _pm_3d_nr_cap_set_param, _pm_3d_nr_cap_get_param, PNULL, PNULL};

struct isp_block_cfg s_blk_cfgs[] = {
	/************************************Tshark2*******************************************************/
	{ISP_BLK_FLASH_CALI, array_offset(struct isp_context, flash), sizeof(struct isp_flash_param), &s_flash_ops},
	{ISP_BLK_ENVI_DETECT, array_offset(struct isp_context, envi_detect), sizeof(struct isp_envi_detect_param), &s_envi_detect_ops},
	{ISP_BLK_PRE_GBL_GAIN,array_offset(struct isp_context,pre_gbl_gain_v1),sizeof(struct isp_pre_global_gain_param_v1),&s_pre_gbl_gain_ops_v1},
	{ISP_BLK_BLC,array_offset(struct isp_context,blc_v1),sizeof(struct isp_blc_param),&s_blc_ops_v1},
	{ISP_BLK_POSTBLC,array_offset(struct isp_context, post_blc),sizeof(struct isp_postblc_param),&s_postblc_ops},
	{ISP_BLK_RGB_GAIN,array_offset(struct isp_context,rgb_gain_v1),sizeof(struct isp_rgb_gain_param_v1),&s_rgb_gain_ops_v1},
	{ISP_BLK_NLC,array_offset(struct isp_context,nlc_v1),sizeof(struct isp_nlc_param_v1),&s_nlc_ops_v1},
	{ISP_BLK_2D_LSC,array_offset(struct isp_context,lsc_2d),sizeof(struct isp_2d_lsc_param),&s_2d_lsc_ops},
	{ISP_BLK_1D_LSC,array_offset(struct isp_context,lsc_1d),sizeof(struct isp_1d_lsc_param),&s_1d_lsc_ops},
	{ISP_BLK_BINNING4AWB,array_offset(struct isp_context,binning4awb_v1),sizeof(struct isp_binning4awb_param_v1),&s_binning4awb_ops_v1},
	{ISP_BLK_AWBC, array_offset(struct isp_context,awb_v1), sizeof(struct isp_awb_param_v1), &s_awb_ops_v1},
	{ISP_BLK_BPC,array_offset(struct isp_context,bpc_v1),sizeof(struct isp_bpc_param_v1),&s_bpc_ops_v1},
	{ISP_BLK_GRGB,array_offset(struct isp_context,grgb),sizeof(struct isp_grgb_param),&s_grgb_ops_v1},
	{ISP_BLK_YNR,array_offset(struct isp_context,ynr),sizeof(struct isp_ynr_param),&s_ynr_ops},
	{ISP_BLK_PDAF_CORRECT,array_offset(struct isp_context,pdaf_correct),sizeof(struct isp_pdaf_correction_param),&s_pdaf_correct_ops},
	{ISP_BLK_PDAF_CORRECT,array_offset(struct isp_context,pdaf_extraction),sizeof(struct isp_pdaf_extraction_param),&s_pdaf_extraction_ops},
	{ISP_BLK_NLM,array_offset(struct isp_context,nlm_v1),sizeof(struct isp_nlm_param_v1),&s_nlm_ops_v1},
	{ISP_BLK_CFA, array_offset(struct isp_context,cfa_v1), sizeof(struct isp_cfa_param_v1),                        &s_cfa_ops_v1},
	{ISP_BLK_CMC10,array_offset(struct isp_context,cmc10),sizeof(struct isp_cmc10_param),&s_cmc10_ops},
	{ISP_BLK_HSV,array_offset(struct isp_context,hsv),sizeof(struct isp_hsv_param),&s_hsv_ops},
	{ISP_BLK_POSTERIZE,array_offset(struct isp_context,posterize),sizeof(struct isp_posterize_param),&s_posterize_ops},
//	{ISP_BLK_YUV_PRECDN,  array_offset(struct isp_context,yuv_precdn), sizeof(struct isp_yuv_precdn_param),                             &s_yuv_precdn_ops},
	{ISP_BLK_BRIGHT, array_offset(struct isp_context, bright), sizeof(struct isp_bright_param), &s_bright_ops},
	{ISP_BLK_CONTRAST, array_offset(struct isp_context, contrast), sizeof(struct isp_contrast_param), &s_contrast_ops},
	{ISP_BLK_HIST, array_offset(struct isp_context,hist_v1),sizeof(struct isp_hist_param_v1),&s_hist_ops_v1},
	{ISP_BLK_HIST2, array_offset(struct isp_context,hist2_v1),sizeof(struct isp_hist2_param_v1),&s_hist2_ops_v1},
	{ISP_BLK_UV_CDN,array_offset(struct isp_context,uv_cdn_v1),sizeof(struct isp_uv_cdn_param_v1), &s_uv_cdn_ops_v1},
	{ISP_BLK_RGB_DITHER,array_offset(struct isp_context,rgb_dither),sizeof(struct isp_rgb_dither_param), &s_rgb_dither_ops},
	{ISP_BLK_EDGE, array_offset(struct isp_context,edge_v1),sizeof(struct isp_edge_param_v1),&s_edge_ops_v1},
	{ISP_BLK_SATURATION,array_offset(struct isp_context,saturation_v1),sizeof(struct isp_chrom_saturation_param),&s_saturation_ops_v1},
	{ISP_BLK_HUE,array_offset(struct isp_context,hue_v1),sizeof(struct isp_hue_param_v1),&s_hue_ops_v1},
	{ISP_BLK_UV_POSTCDN,     array_offset(struct isp_context,uv_postcdn),           sizeof(struct isp_uv_postcdn_param),                     &s_uv_postcdn_ops},
	{ISP_BLK_Y_GAMMC,array_offset(struct isp_context,yuv_ygamma),sizeof(struct isp_yuv_ygamma_param),&s_yuv_ygamma_ops},
	{ISP_BLK_Y_DELAY,array_offset(struct isp_context,ydelay),sizeof(struct isp_ydelay_param),&s_ydelay_ops},
	{ISP_BLK_IIRCNR_IIR,array_offset(struct isp_context,iircnr_iir),sizeof(struct isp_iircnr_iir_param),&s_iircnr_iir_ops},
	{ISP_BLK_IIRCNR_YRANDOM,array_offset(struct isp_context,iircnr_yrandom),sizeof(struct isp_iircnr_yrandom_param),&s_iircnr_yrandom_ops},
	{ISP_BLK_UVDIV, array_offset(struct isp_context, uv_div_v1), sizeof(struct isp_cce_uvdiv_param_v1), &s_uvdiv_ops_v1},
	{ISP_BLK_SMART, array_offset(struct isp_context, smart), sizeof(struct isp_smart_param), &s_smart_ops},
	{ISP_BLK_SFT_AF, array_offset(struct isp_context, sft_af), sizeof(struct isp_sft_af_param), &s_sft_af_ops},
	{ISP_BLK_ALSC, array_offset(struct isp_context, alsc), sizeof(struct isp_alsc_param), &s_alsc_ops},
	{ISP_BLK_AFT, array_offset(struct isp_context, aft), sizeof(struct isp_aft_param), &s_aft_ops},
};

struct isp_block_cfg* isp_pm_get_block_cfg(isp_u32 id)
{
	isp_u32 num = 0;
	isp_u32 i = 0;
	isp_u32 blk_id = 0;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_block_cfg *blk_cfg_array = s_blk_cfgs;

	num = sizeof (s_blk_cfgs) / sizeof(struct isp_block_cfg);
	ISP_LOGE("id is %x\n", id);
	for (i = 0; i < num; ++i) {
		blk_id = blk_cfg_array[i].id;
		if (blk_id == id) {
			break;
		}
	}

	if (i <num) {
		blk_cfg_ptr  = &blk_cfg_array[i];
	} else {
		blk_cfg_ptr = PNULL;
	}

	return blk_cfg_ptr;
}

