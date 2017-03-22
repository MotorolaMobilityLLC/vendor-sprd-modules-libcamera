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
 #ifndef _ISP_BLOCKS_CFG_H_
 #define _ISP_BLOCKS_CFG_H_

#ifdef WIN32
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include "isp_type.h"
#endif

#include "isp_com.h"
#include "isp_pm_com_type.h"
#include "isp_com_alg.h"
#include "smart_ctrl.h"
#include "isp_otp_calibration.h"
#include <cutils/properties.h>
#include "isp_video.h"
#include "cmr_types.h"



#ifdef	 __cplusplus
extern	 "C"
{
#endif

/*************************************************************************/
#define array_offset(type, member) (intptr_t)(&((type*)0)->member)

#define ISP_PM_LEVEL_DEFAULT 3
#define ISP_PM_CCE_DEFAULT 0

/*************************************************************************/

#define ISP_PM_CMC_MAX_INDEX 9
#define ISP_PM_CMC_SHIFT 18

/*******************************isp_block_com******************************/
 cmr_s32 PM_CLIP(cmr_s32 x, cmr_s32 bottom, cmr_s32 top);

 cmr_s32 _is_print_log();

 cmr_s32 _pm_check_smart_param(struct smart_block_result *block_result,
						struct isp_range *range,
						cmr_u32 comp_num,
						cmr_u32 type);

 cmr_s32 _pm_common_rest(void *blk_addr, cmr_u32 size);

 cmr_u32 _pm_get_lens_grid_mode(cmr_u32 grid);

 cmr_u16 _pm_get_lens_grid_pitch(cmr_u32 grid_pitch, cmr_u32 width, cmr_u32 flag);

 cmr_u32 _ispLog2n(cmr_u32 index);

cmr_u32 _pm_calc_nr_addr_offset(cmr_u32 mode_flag, cmr_u32 scene_flag, cmr_u32 * one_multi_mode_ptr);

/*******************************isp_pm_blocks******************************/

 cmr_s32 _pm_pre_gbl_gain_init_v1(void *dst_pre_gbl_gain, void *src_pre_gbl_gain, void* param1, void*param2);
 cmr_s32 _pm_pre_gbl_gain_set_param_v1(void *pre_gbl_gain_param, cmr_u32 cmd, void* param_ptr0, void *param_ptr1);
 cmr_s32 _pm_pre_gbl_gain_get_param_v1(void *pre_gbl_gain_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_blc_init(void *dst_blc_param, void *src_blc_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_blc_set_param(void *blc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_blc_get_param(void *blc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_blc_init_v1(void *dst_blc_param, void *src_blc_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_blc_set_param_v1(void *blc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_blc_get_param_v1(void *blc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_pdaf_correct_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_pdaf_correct_init(void *dst_pdaf_correct_param, void *src_pdaf_correct_param, void* param1, void* param2);
 cmr_s32 _pm_pdaf_correct_set_param(void *pdaf_correct_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_pdaf_correct_get_param(void *pdaf_correct_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_pdaf_extraction_init(void *dst_pdaf_extraction_param, void *src_pdaf_extraction_param, void* param1, void* param2);
 cmr_s32 _pm_pdaf_extraction_set_param(void *pdaf_extraction_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_pdaf_extraction_get_param(void *pdaf_extraction_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_postblc_init(void *dst_postblc_param, void *src_postblc_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_postblc_set_param(void *postblc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_postblc_get_param(void *postblc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_rgb_gain_init_v1(void *dst_gbl_gain, void *src_gbl_gain, void* param1, void* param2);
 cmr_s32 _pm_rgb_gain_set_param_v1(void *gbl_gain_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_rgb_gain_get_param_v1(void *gbl_gain_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_rgb_dither_init(void *dst_rgb_dither_param, void *src_rgb_dither_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_rgb_dither_set_param(void *rgb_dither_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_rgb_dither_get_param(void *rgb_dither_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_nlm_convert_param(void *dst_nlm_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_nlm_init(void *dst_nlm_param, void *src_nlm_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_nlm_set_param(void *nlm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_nlm_get_param(void *nlm_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);
 cmr_s32 _pm_nlm_deinit(void* nlm_param);

 cmr_s32 _pm_2d_lsc_init(void * dst_lnc_param,void * src_lnc_param,void * param1,void * param2);
 cmr_s32 _pm_2d_lsc_otp_active(struct sensor_2d_lsc_param *lsc_ptr, struct isp_cali_lsc_info *cali_lsc_ptr);
 cmr_s32 _pm_2d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void* param_ptr0, void *param_ptr1);
 cmr_s32 _pm_2d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);
 cmr_s32 _pm_2d_lsc_deinit(void *lnc_param);

 cmr_s32 _pm_1d_lsc_init(void * dst_lnc_param,void * src_lnc_param,void * param1,void * param2);
 cmr_s32 _pm_1d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void* param_ptr0, void *param_ptr1);
 cmr_s32 _pm_1d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_binning4awb_init_v1(void *dst_binning4awb, void *src_binning4awb, void* param1, void* param2);
 cmr_s32 _pm_binning4awb_set_param_v1(void *binning4awb_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_binning4awb_get_param_v1(void *binning4awb_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

//awbc
 cmr_s32 _pm_awb_init_v1(void *dst_pwd, void *src_pwd, void* param1, void* param2);
 cmr_s32 _pm_awb_set_param_v1(void *pwd_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_awb_get_param_v1(void *pwd_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_frgb_gamc_init(void *dst_gamc_param, void *src_gamc_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_frgb_gamc_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_frgb_gamc_get_param(void *gamc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_bpc_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_bpc_init_v1(void *dst_bpc_param, void *src_bpc_param, void* param1, void* param2);
 cmr_s32 _pm_bpc_set_param_v1(void *bpc_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_bpc_get_param_v1(void *bpc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_grgb_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_grgb_init_v1(void *dst_grgb_param, void *src_grgb_param, void* param1, void* param2);
 cmr_s32 _pm_grgb_set_param_v1(void *grgb_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_grgb_get_param_v1(void *grgb_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_cfa_convert_param(void *dst_cfae_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_cfa_init_v1(void *dst_cfae_param, void *src_cfae_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_cfa_set_param_v1(void *cfae_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_cfa_get_param_v1(void *cfa_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_rgb_afm_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_rgb_afm_init(void *dst_rgb_afm, void *src_rgb_aef, void* param1, void* param2);
 cmr_s32 _pm_rgb_afm_set_param(void *rgb_afm_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_rgb_afm_get_param(void *rgb_afm_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_cmc10_init(void *dst_cmc10_param, void *src_cmc10_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_cmc10_adjust(struct isp_cmc10_param *cmc_ptr, cmr_u32 is_reduce);
 cmr_s32 _pm_cmc10_set_param(void *cmc10_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_cmc10_get_param(void *cmc10_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_yuv_ygamma_init(void *dst_gamc_param, void *src_gamc_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_yuv_ygamma_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_yuv_ygamma_get_param(void *gamc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_hsv_init(void *dst_hsv_param, void *src_hsv_param, void* param1, void* param2);
 cmr_s32 _pm_hsv_set_param(void *hsv_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_hsv_get_param(void *hsv_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);
 cmr_s32 _pm_hsv_deinit(void* hsv_param);

 cmr_s32 _pm_posterize_init(void *dst_pstrz_param, void *src_pstrz_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_posterize_set_param(void *pstrz_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_posterize_get_param(void *pstrz_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_cce_adjust_hue_saturation(struct isp_cce_param_v1 *cce_param, cmr_u32 hue, cmr_u32 saturation);
 cmr_s32 _pm_cce_adjust_gain_offset(struct isp_cce_param_v1 *cce_param, cmr_u16 r_gain, cmr_u16 g_gain, cmr_u16 b_gain);
 cmr_s32 _pm_cce_adjust(struct isp_cce_param_v1 *cce_param);
 cmr_s32 _pm_cce_init_v1(void *dst_cce_param, void *src_cce_param, void* param1, void* param2);
 cmr_s32 _pm_cce_set_param_v1(void *cce_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_cce_get_param_v1(void *cce_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_uv_div_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_uv_div_init_v1(void *dst_uv_div_param, void *src_uv_div_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_uv_div_set_param_v1(void *uv_div_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_uv_div_get_param_v1(void *uv_div_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_yiq_afl_init_v1(void *dst_afl_param, void *src_afl_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_yiq_afl_set_param_v1(void *afl_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_yiq_afl_get_param_v1(void *afl_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_yiq_afl_init_v3(void *dst_afl_param, void *src_afl_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_yiq_afl_set_param_v3(void *afl_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_yiq_afl_get_param_v3(void *afl_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_3d_nr_pre_convert_param(void *dst_3d_nr_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_3d_nr_pre_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_3d_nr_pre_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_3d_nr_pre_get_param(void *ynr_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_3d_nr_cap_convert_param(void *dst_3d_nr_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_3d_nr_cap_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_3d_nr_cap_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_3d_nr_cap_get_param(void *ynr_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_yuv_precdn_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_yuv_precdn_init(void *dst_precdn_param, void *src_precdn_param, void *param1, void *param2);
 cmr_s32 _pm_yuv_precdn_set_param(void *precdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_yuv_precdn_get_param(void *precdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

 cmr_u32 _pm_ynr_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_ynr_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_ynr_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_ynr_get_param(void *ynr_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_brightness_init(void *dst_brightness, void *src_brightness, void* param1, void* param2);
 cmr_s32 _pm_brightness_set_param(void *bright_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_brightness_get_param(void *bright_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_contrast_init(void *dst_contrast, void *src_contrast, void* param1, void* param2);
 cmr_s32 _pm_contrast_set_param(void *contrast_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_contrast_get_param(void *contrast_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_hist_init_v1(void *dst_hist_param, void *src_hist_param, void *param1, void *param2);
 cmr_s32 _pm_hist_set_param_v1(void *hist_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_hist_get_param_v1(void *hist_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

 cmr_s32 _pm_hist2_init_v1(void *dst_hist2_param, void *src_hist2_param, void *param1, void *param2);
 cmr_s32 _pm_hist2_set_param_v1(void *hist2_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_hist2_get_param_v1(void *hist2_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

 cmr_u32 _pm_edge_convert_param(void *dst_edge_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_edge_init_v1(void *dst_edge_param, void *src_edge_param, void *param1, void *param2);
 cmr_s32 _pm_edge_set_param_v1(void *edge_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_edge_get_param_v1(void *edge_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

 cmr_u32 _pm_uv_cdn_convert_param(void *dst_cdn_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_uv_cdn_init_v1(void *dst_cdn_param, void *src_cdn_param, void *param1, void *param2);
 cmr_s32 _pm_uv_cdn_set_param_v1(void *cdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_uv_cdn_get_param_v1(void *cdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

 cmr_u32 _pm_uv_postcdn_convert_param(void *dst_postcdn_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_uv_postcdn_init(void *dst_postcdn_param, void *src_postcdn_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_uv_postcdn_set_param(void *postcdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_uv_postcdn_get_param(void *postcdn_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_ydelay_init(void *dst_ydelay, void *src_ydelay, void *param1, void *param2);
 cmr_s32 _pm_ydelay_set_param(void *ydelay_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_ydelay_get_param(void *ydelay_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_saturation_init_v1(void *dst_csa_param, void *src_csa_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_saturation_set_param_v1(void *csa_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_saturation_get_param_v1(void *csa_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_hue_init_v1(void *dst_hue_param, void *src_hue_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_hue_set_param_v1(void *hue_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_hue_get_param_v1(void *hue_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_iircnr_iir_convert_param(void *dst_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_iircnr_iir_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_iircnr_iir_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_iircnr_iir_get_param(void *iircnr_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_iircnr_yrandom_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_iircnr_yrandom_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_iircnr_yrandom_get_param(void *iircnr_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_u32 _pm_yuv_noisefilter_convert_param(void *dst_yuv_noisefilter_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
 cmr_s32 _pm_yuv_noisefilter_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_yuv_noisefilter_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_yuv_noisefilter_get_param(void *ynr_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_flashlight_init(void *dst_flash_param, void *src_flash_param, void* param1, void* param2);
 cmr_s32 _pm_flashlight_set_param(void *flash_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_flashlight_get_param(void *flash_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_envi_detect_init(void *dst_envi_detect_param, void *src_envi_detect_param, void* param1, void* param2);
 cmr_s32 _pm_envi_detect_set_param(void *envi_detect_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_envi_detect_get_param(void *envi_detect_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);






 cmr_s32 _pm_nlc_init_v1(void *dst_nlc_param, void *src_nlc_param, void* param1, void* param2);
 cmr_s32 _pm_nlc_set_param_v1(void *nlc_param, cmr_u32 cmd, void* param_ptr0, void *param_ptr1);
 cmr_s32 _pm_nlc_get_param_v1(void *nlc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_rgb_aem_init(void *dst_rgb_aem, void *src_rgb_aem, void* param1, void* param2);
 cmr_s32 _pm_rgb_aem_set_param(void *rgb_aem_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_rgb_aem_get_param(void *rgb_aem_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_csc_init(void *dst_csc_param, void *src_csc_param, void* param1, void* param2);
 cmr_s32 _pm_csc_set_param(void *csc_param, cmr_u32 cmd, void* param_ptr0, void* param_ptr1);
 cmr_s32 _pm_csc_get_param(void *csc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_rgb_afm_init_v1(void *dst_afm_param, void *src_afm_param, void* param1, void* param_ptr2);
 cmr_s32 _pm_rgb_afm_set_param_v1(void *afm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_rgb_afm_get_param_v1(void *afm_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_smart_init(void *dst_smart_param, void *src_smart_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_smart_set_param(void *smart_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_smart_get_param(void *smart_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_aft_init(void *dst_aft_param, void *src_aft_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_aft_set_param(void *aft_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_aft_get_param(void *aft_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_alsc_init(void *dst_alsc_param, void *src_alsc_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_alsc_set_param(void *alsc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_alsc_get_param(void *alsc_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_sft_af_init(void *dst_sft_af_param, void *src_sft_af_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_sft_af_set_param(void *aft_af_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_sft_af_get_param(void *aft_af_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_ae_new_init(void *dst_ae_new_param, void *src_ae_new_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_ae_new_set_param(void *ae_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_ae_new_get_param(void *ae_new_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_awb_new_init(void *dst_awb_new_param, void *src_awb_new_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_awb_new_set_param(void *awb_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_awb_new_get_param(void *awb_new_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);

 cmr_s32 _pm_af_new_init(void *dst_af_new_param, void *src_af_new_param, void *param1, void *param_ptr2);
 cmr_s32 _pm_af_new_set_param(void *af_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
 cmr_s32 _pm_af_new_get_param(void *af_new_param, cmr_u32 cmd, void* rtn_param0, void* rtn_param1);


struct isp_block_operations {
	cmr_s32 (*init)(void *blk_ptr, void* param_ptr0, void* param_ptr1, void* param_ptr2);
	cmr_s32 (*set)(void *blk_ptr, cmr_u32 cmd, void* param_ptr0, void*param_ptr1);
	cmr_s32 (*get)(void *blk_ptr, cmr_u32 cmd, void* param_ptr0, void*param_ptr1);
	cmr_s32 (*reset)(void *blk_ptr, cmr_u32 size);
	cmr_s32 (*deinit)(void *blk_ptr);
};




struct isp_block_cfg {
	cmr_u32 id;
	cmr_u32 offset;
	cmr_u32 param_size;
	struct isp_block_operations *ops;
};





struct isp_block_cfg* isp_pm_get_block_cfg(cmr_u32 id);

#ifdef	 __cplusplus
}
#endif
 #endif //
