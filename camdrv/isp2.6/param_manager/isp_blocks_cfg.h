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
#ifndef _ISP_BLOCKS_CFG_H_
#define _ISP_BLOCKS_CFG_H_

#ifdef WIN32
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include "isp_type.h"
#endif

#include "isp_pm_com_type.h"
#include "isp_com_alg.h"
#include "smart_ctrl.h"
#include <cutils/properties.h>
#include "isp_video.h"
#include "cmr_types.h"
#include "sprd_isp_k.h"
#include "isp_mw.h"

#ifdef	 __cplusplus
extern "C" {
#endif


/*************************************************************************/
#define array_offset(type, member) (intptr_t)(&((type*)0)->member)

#ifndef SENSOR_HSV_NUM_NEW
#define SENSOR_HSV_NUM_NEW SENSOR_HSV_NUM
#endif

#define SENSOR_HSV_TAB_NUM 360
#define ISP_PM_HSV_CTRESULT_NUM 2
#define SENSOR_YUV_LTM_NUM 16
#define SENSOR_RAW_GTM_NUM 16

/*************************************************************************/

enum isp_pm_ai_scene_type {
	ISP_PM_AI_SCENE_DEFAULT,
	ISP_PM_AI_SCENE_FOOD,
	ISP_PM_AI_SCENE_PORTRAIT,
	ISP_PM_AI_SCENE_FOLIAGE,
	ISP_PM_AI_SCENE_SKY,
	ISP_PM_AI_SCENE_NIGHT,
	ISP_PM_AI_SCENE_BACKLIGHT,
	ISP_PM_AI_SCENE_TEXT,
	ISP_PM_AI_SCENE_SUNRISE,
	ISP_PM_AI_SCENE_BUILDING,
	ISP_PM_AI_SCENE_LANDSCAPE,
	ISP_PM_AI_SCENE_SNOW,
	ISP_PM_AI_SCENE_FIREWORK,
	ISP_PM_AI_SCENE_BEACH,
	ISP_PM_AI_SCENE_PET,
	ISP_PM_AI_SCENE_FLOWER,
	ISP_PM_AI_SCENE_MAX
};

enum
{
	ISP_PM_FB_SKINTONE_DEFAULT,
	ISP_PM_FB_SKINTONE_YELLOW,
	ISP_PM_FB_SKINTONE_WHITE,
	ISP_PM_FB_SKINTONE_BLACK,
	ISP_PM_FB_SKINTONE_INDIAN,
	ISP_PM_FB_SKINTONE_NUM
};

struct isp_blc_offset {
	cmr_u16 r;
	cmr_u16 gr;
	cmr_u16 gb;
	cmr_u16 b;
};

struct isp_blc_param {
	struct dcam_dev_blc_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_blc_offset offset[SENSOR_BLC_NUM];
};

struct isp_rgb_gain_param {
	struct dcam_dev_rgb_gain_info cur;
};

struct isp_rgb_dither_param {
	struct dcam_dev_rgb_dither_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_lsc_map {
	cmr_u32 ct;
	cmr_u32 grid;
	cmr_u32 gain_w;
	cmr_u32 gain_h;
	void *param_addr;
	cmr_u32 len;
};

struct isp_lsc_info {
	struct isp_sample_point_info cur_idx;
	void *data_ptr;
	void *param_ptr;
	cmr_u32 len;
	cmr_u32 grid;
	cmr_u32 gain_w;
	cmr_u32 gain_h;
};

struct isp_2d_lsc_param {
	struct isp_sample_point_info cur_index_info;
	struct dcam_dev_lsc_info cur;
	struct isp_data_info final_lsc_param;	//store the resulted lsc params
	struct isp_lsc_map map_tab[ISP_COLOR_TEMPRATURE_NUM];
	cmr_u32 tab_num;
	struct isp_lsc_info lsc_info;
	struct isp_size resolution;
	cmr_s16 weight_tab[LNC_WEIGHT_LEN];
	cmr_u32 update_flag;
};

struct isp_bayerhist_param {
	struct dcam_dev_hist_info cur;
	/* TBD */
};

struct isp_rgb_aem_param {
	struct isp_size win_num;
};

struct isp_ae_adapt_param {
	cmr_u16 binning_factor; // 1x = 128
};

struct isp_awb_param {
	cmr_u32 ct_value;
	struct dcam_dev_awbc_info cur;
};

struct isp_awbc_cfg {
	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
	cmr_u32 r_offset;
	cmr_u32 g_offset;
	cmr_u32 b_offset;
};

struct isp_bpc_param {
	union {
		struct dcam_dev_bpc_info cur;
		struct dcam_dev_bpc_info_l3 cur_v0;
	};
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_ppe_param {
	struct dcam_bpc_ppi_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_rgb_afm_param {
	struct dcam_dev_afm_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *param_ptr1;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_nr3d_param {
	struct isp_dev_3dnr_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_sw3dnr_param {
	void *cur_data;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_mfnr_param {
	void *cur_data;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_ltm_param {
	struct isp_dev_ltm_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_rgb_ltm_inter {
	cmr_u32 ltm_map_bypass;
	cmr_u32 ltm_map_video_mode;
	cmr_u32 ltm_stat_bypass;
	cmr_u32 ltm_stat_strength;
	cmr_u32 tile_num_auto;
	cmr_u32 region_est_en;
	cmr_u32 channel_sel;
	cmr_u32 tile_num_x;
	cmr_u32 tile_num_y;
	cmr_u32 text_point_thres;
	cmr_u32 textture_proporion;
	float text_point_alpha;
};

struct isp_rgb_ltm_param {
	struct isp_dev_rgb_ltm_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_rgb_ltm_inter ltm_param[SENSOR_YUV_LTM_NUM];
};

struct isp_yuv_ltm_inter {
	cmr_u32 ltm_map_bypass;
	cmr_u32 ltm_map_video_mode;
	cmr_u32 ltm_stat_bypass;
	cmr_u32 ltm_stat_strength;
	cmr_u32 tile_num_auto;
	cmr_u32 region_est_en;
	cmr_u32 tile_num_x;
	cmr_u32 tile_num_y;
	float text_point_alpha;
	cmr_u32 text_point_thres;
	cmr_u32 textture_proporion;
};

struct isp_yuv_ltm_param {
	struct isp_dev_yuv_ltm_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_yuv_ltm_inter ltm_param[SENSOR_YUV_LTM_NUM];
};

struct isp_raw_gtm_inter {
	cmr_u32 gtm_tm_luma_est_mode;//rgb2y_mode
	cmr_u32 gtm_hist_stat_bypass;//gtm_stat_bypass
	cmr_u32 gtm_map_bypass;//bypass
	cmr_u32 gtm_map_video_mode;//map_video_mode
	cmr_u32 gtm_imgkey_setting_value;//image_key
	cmr_u32 gtm_imgkey_setting_mode;//image_key_set_mode
	cmr_u32 gtm_target_norm_coeff;//target_norm_coeff    jianjiejisuan
	cmr_u32 gtm_target_norm;//target_norm
	cmr_u32 gtm_target_norm_setting_mode;//target_norm_set_mode
	cmr_u32 gtm_yavg_diff_thr;//luma_sm_Yavg_diff_thr
	cmr_u32 gtm_hist_total;
	cmr_u32 min_percentile_16bit;
	cmr_u32 max_percentile_16bit;
	cmr_u32 gtm_pre_ymin_weight;//pre_Ymin_weight
	cmr_u32 gtm_cur_ymin_weight;//cur_Ymin_weight
	cmr_u32 gtm_ymax_diff_thr;//luma_sm_Ymax_diff_thr
	cmr_u32 tm_rgb2y_g_coeff;//rgb2y_r_coeff
	cmr_u32 tm_rgb2y_r_coeff;//rgb2y_g_coeff
	cmr_u32 tm_rgb2y_b_coeff;//rgb2y_b_coeff
};

struct isp_raw_gtm_param {
	struct dcam_dev_raw_gtm_block_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_raw_gtm_inter gtm_param[SENSOR_RAW_GTM_NUM];
};

struct isp_facebeauty_level
{
	cmr_u8 skinSmoothLevel[11];
	cmr_u8 skinSmoothDefaultLevel;
	cmr_u8 skinTextureHiFreqLevel[11];
	cmr_u8 skinTextureHiFreqDefaultLevel;
	cmr_u8 skinTextureLoFreqLevel[11];
	cmr_u8 skinTextureLoFreqDefaultLevel;
	cmr_u8 skinSmoothRadiusCoeff[11];
	cmr_u8 skinSmoothRadiusCoeffDefaultLevel;
	cmr_u8 skinBrightLevel[11];
	cmr_u8 skinBrightDefaultLevel;
	cmr_u8 largeEyeLevel[11];
	cmr_u8 largeEyeDefaultLevel;
	cmr_u8 slimFaceLevel[11];
	cmr_u8 slimFaceDefaultLevel;
	cmr_u8 skinColorLevel[11];
	cmr_u8 skinColorDefaultLevel;
	cmr_u8 lipColorLevel[11];
	cmr_u8 lipColorDefaultLevel;
};

struct isp_facebeauty_param
{
	cmr_u8 removeBlemishFlag;
	cmr_u8 blemishSizeThrCoeff;
	cmr_u8 skinColorType;
	cmr_u8 lipColorType;
	struct isp_facebeauty_level fb_layer;
};

struct isp_facebeauty_param_cfg_info
{
	struct isp_facebeauty_param fb_param[ISP_PM_FB_SKINTONE_NUM];
};

struct isp_facebeauty_param_info {
	struct isp_facebeauty_param_cfg_info cur;
};

struct isp_bright_cfg {
	cmr_u32 factor;
};

struct isp_contrast_cfg {
	cmr_u32 factor;
};

struct isp_saturation_cfg {
	cmr_u32 factor;
};


struct isp_bright_param {
	cmr_u32 cur_index;
	struct isp_dev_brightness_info cur;
	cmr_u8 bright_tab[16];
	cmr_u8 scene_mode_tab[MAX_SCENEMODE_NUM];
};

struct isp_contrast_param {
	cmr_u32 cur_index;
	struct isp_dev_contrast_info cur;
	cmr_u8 tab[16];
	cmr_u8 scene_mode_tab[MAX_SCENEMODE_NUM];
};

struct isp_hue_param {
	cmr_u32 cur_idx;
	cmr_s16 tab_sin[16];
	cmr_s16 tab_cos[16];
};

struct isp_hue_param_v0 {
	struct isp_dev_hue_info_l3 cur;
	cmr_u32 cur_idx;
	cmr_s16 tab[SENSOR_LEVEL_NUM];
};

struct isp_chrom_saturation_param {
	struct isp_dev_csa_info cur;
	cmr_u32 cur_u_idx;
	cmr_u32 cur_v_idx;
	cmr_u8 tab[2][SENSOR_LEVEL_NUM];
	cmr_u8 scene_mode_tab[2][MAX_SCENEMODE_NUM];
};

struct isp_bchs_param {
	struct isp_dev_bchs_info cur;
	struct isp_bright_param brigntness;
	struct isp_contrast_param contrast;
	union {
		struct isp_hue_param hue;
		struct isp_hue_param_v0 hue_v0;
	};
	struct isp_chrom_saturation_param saturation;
};

struct isp_cce_param {
	struct isp_dev_cce_info cur;
	//R/G/B coef to change cce //
	cmr_s32 cur_level[2];
	//0: color cast, 1: gain offset //
	cmr_u16 cce_coef[2][3];
	cmr_u16 bakup_cce_coef[3];
	cmr_u32 prv_idx;
	cmr_u32 cur_idx;
	cmr_u32 is_specialeffect;
	struct isp_dev_cce_info cce_tab[16];
	struct isp_dev_cce_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_cfa_param {
	struct isp_dev_cfa_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_cmc10_param {
	struct isp_dev_cmc10_info cur;
	struct isp_sample_point_info cur_idx_info;
	cmr_u16 matrix[SENSOR_CMC_NUM][SENSOR_CMC_POINT_NUM];
	cmr_u16 result_cmc[SENSOR_CMC_POINT_NUM];
	cmr_u16 reserved;
	cmr_u32 reduce_percent;	//reduce saturation.
};

struct isp_edge_cfg {
	cmr_u32 factor;
};

struct isp_edge_param {
	struct isp_dev_edge_info_v2 cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_frgb_gamc_param {
	struct isp_dev_gamma_info cur;
	struct sensor_rgbgamma_curve final_curve;
	struct isp_sample_point_info cur_idx;
	struct sensor_rgbgamma_curve curve_tab[SENSOR_GAMMA_NUM];
};

struct isp_grgb_param {
	struct isp_dev_grgb_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_hist2_param {
	struct isp_dev_hist2_info cur;
};

struct isp_hsv_param {
	struct isp_dev_hsv_info_v2 cur;
	struct isp_sample_point_info cur_idx;
	struct isp_data_info final_map;
	struct isp_data_info map[SENSOR_HSV_NUM];
	struct isp_data_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
	cmr_u32 *ct_result[2];
};

struct isp_hsv_table {
	cmr_s16 hue_table[SENSOR_HSV_TAB_NUM];
	cmr_u16 sat_table[SENSOR_HSV_TAB_NUM];
};

struct isp_hsv_param_new2 {
	struct isp_dev_hsv_info_v2 cur;
	struct isp_sample_point_info cur_idx;
	struct isp_hsv_table hsv_table[SENSOR_HSV_NUM];
	struct isp_data_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_hsv_param_new {
	struct isp_dev_hsv_info_v2 cur;
	struct isp_sample_point_info cur_idx;
	struct isp_data_info final_map;
	struct isp_data_info map[SENSOR_HSV_NUM_NEW];
	struct isp_data_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
	cmr_u32 *ct_result[2];
};

struct isp_iircnr_iir_param {
	struct isp_dev_iircnr_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_iircnr_yrandom_param {
	struct isp_dev_yrandom_info cur;
};

struct isp_nlm_param {
	cmr_u32 cur_level;
	cmr_u32 level_num;
	struct isp_dev_nlm_info_v2 cur;
	struct isp_data_info vst_map;
	struct isp_data_info ivst_map;
	cmr_uint *nlm_ptr;
	cmr_uint *vst_ptr;
	cmr_uint *ivst_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_imblance_param {
	union {
		struct isp_dev_nlm_imblance cur;
		struct isp_dev_nlm_imblance_v1 cur_v1;
	};
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_posterize_param {
	union {
		struct isp_dev_posterize_info_v2 cur;
		struct isp_dev_posterize_info cur_v0;
	};
};

struct isp_yuv_precdn_param {
	struct isp_dev_pre_cdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_uv_cdn_param {
	struct isp_dev_cdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_uv_postcdn_param {
	struct isp_dev_post_cdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_uvdiv_param {
	union {
		struct isp_dev_uvd_info_v2 cur;
		struct isp_dev_uvd_info cur_v0;
	};
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_ynr_param {
	union {
		struct isp_dev_ynr_info_v2 cur;
		struct isp_dev_ynr_info cur_v0;
	};
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_dev_noise_filter_param {
	struct isp_dev_noise_filter_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_yuv_ygamma_param {
	struct isp_dev_ygamma_info cur;
	cmr_u32 cur_idx;
	struct isp_sample_point_info cur_idx_weight;
	struct sensor_gamma_curve final_curve;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
	struct sensor_gamma_curve specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_cnr2_level_info {
	cmr_u8 level_enable;
	cmr_u16 low_ct_thrd;
};

struct isp_cnr3_level_info {
	cmr_u8 level_enable;
	cmr_u16 low_ct_thrd;
};

struct isp_filter_weights
{
	cmr_u8 distWeight[9];
	cmr_u8 rangWeight[128];
};

#ifdef CAMERA_CNR3_ENABLE
struct isp_cnr2_info {
	cmr_u8 filter_en[CNR_LEVEL];
	cmr_u8 rangTh[CNR_LEVEL][2];
	struct isp_filter_weights weight[CNR_LEVEL][2];
};

struct isp_cnr2_param {
	struct isp_cnr2_info cur;
	struct isp_cnr2_level_info level_info;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_multilayer_param {
	cmr_u8 lowpass_filter_en;
	cmr_u8 denoise_radial_en;
	cmr_u8 order[3];
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u16 slope;
	cmr_u16 baseRadius;
	cmr_u16 minRatio;
	cmr_u16 luma_th[2];
	float sigma[3];
};

struct isp_cnr3_info {
	cmr_u8 bypass;
	cmr_u16 baseRadius;
	struct isp_multilayer_param param_layer[CNR3_LAYER_NUM];
};

struct isp_cnr3_param {
	struct isp_cnr3_info cur;
	struct isp_cnr3_level_info level_info;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

#else

struct isp_cnr2_info {
	cmr_u8 filter_en[CNR_LEVEL];
	cmr_u8 rangTh[CNR_LEVEL][2];
	struct isp_filter_weights weight[CNR_LEVEL][2];
};

struct isp_cnr2_param {
	struct isp_cnr2_info cur;
	struct isp_cnr2_level_info level_info;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};
#endif

#ifdef CAMERA_RADIUS_ENABLE
struct isp_ynrs_params {
	cmr_u8 lumi_thresh[2];
	cmr_u8 gf_rnr_ratio[5];
	cmr_u8 gf_addback_enable[5];
	cmr_u8 gf_addback_ratio[5];
	cmr_u8 gf_addback_clip[5];
	cmr_u16 Radius_factor;
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u16 gf_epsilon[5][3];
	cmr_u16 gf_enable[5];
	cmr_u16 gf_radius[5];
	cmr_u16 gf_rnr_offset[5];
	cmr_u16 bypass;
	cmr_u8 reserved[2];
};

struct isp_ynrs_level{
	cmr_u16 radius_base;
	struct isp_ynrs_params ynrs_param;
};

#else

struct isp_ynrs_level{
	cmr_u8 lumi_thresh[2];
	cmr_u8 gf_rnr_ratio[5];
	cmr_u8 gf_addback_enable[5];
	cmr_u8 gf_addback_ratio[5];
	cmr_u8 gf_addback_clip[5];
	cmr_u16 Radius;
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u16 gf_epsilon[5][3];
	cmr_u16 gf_enable[5];
	cmr_u16 gf_radius[5];
	cmr_u16 gf_rnr_offset[5];
	cmr_u16 bypass;
	cmr_u8 reserved[2];
};
#endif

struct isp_ynrs_param {
 	struct isp_ynrs_level cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_dres_param {
	struct isp_dre_level cur;
	struct isp_dre_level levels[16];
};

struct isp_dres_pro_param {
	struct isp_dre_pro_level cur;
	struct isp_dre_pro_level levels[16];
};

struct isp_context {
	cmr_u32 is_validate;
	cmr_u32 mode_id;

	/* 3A owner: */
	struct isp_rgb_aem_param aem;
	struct isp_rgb_afm_param afm;
	struct isp_awb_param awb;
	struct isp_bayerhist_param bayerhist;
	struct isp_ae_adapt_param ae_adapt;

	/* dcam related tuning blocks */
	struct isp_blc_param blc;
	struct isp_rgb_gain_param rgb_gain;
	struct isp_rgb_dither_param rgb_dither;
	struct isp_2d_lsc_param lsc_2d;
	struct isp_bpc_param bpc;
	struct isp_ppe_param ppe;
	/* dcam blocks end. */

	/* isp blocks below */
	struct isp_nr3d_param nr3d;
	struct isp_sw3dnr_param sw3dnr;
	struct isp_mfnr_param mfnr;
	struct isp_bchs_param bchs;
	struct isp_cce_param cce;
	struct isp_cfa_param cfa;
	struct isp_cmc10_param cmc10;
	struct isp_edge_param edge;
	struct isp_frgb_gamc_param rgb_gamma;
	struct isp_grgb_param grgb;
	struct isp_hist2_param hist2;
	union {
		struct isp_hsv_param hsv;
		struct isp_hsv_param_new hsv_new;
		struct isp_hsv_param_new2 hsv_new2;
	};
	struct isp_iircnr_iir_param iircnr;
	struct isp_iircnr_yrandom_param yrandom;
	struct isp_rgb_ltm_param rgb_ltm;
	union {
		struct isp_ltm_param ltm;
		struct isp_yuv_ltm_param yuv_ltm;
	};
	struct isp_raw_gtm_param gtm;
	struct isp_nlm_param nlm;
	struct isp_imblance_param imblance;
	struct isp_posterize_param posterize;
	struct isp_yuv_precdn_param pre_cdn;
	struct isp_uv_cdn_param cdn;
	struct isp_uv_postcdn_param post_cdn;
	struct isp_uvdiv_param uvd;
	struct isp_ynr_param ynr;
	struct isp_dev_noise_filter_param noisefilter;
	struct isp_yuv_ygamma_param ygamma;

	/* soft algo block */
	struct isp_cnr2_param cnr2;
	struct isp_ynrs_param ynrs;
	struct isp_dres_param dre;
	struct isp_dres_pro_param dre_pro;
#ifdef CAMERA_CNR3_ENABLE
	struct isp_cnr3_param cnr3;
#endif
	struct isp_facebeauty_param_info fb;
};

/*******************************isp_block_com******************************/
cmr_s32 PM_CLIP(cmr_s32 x, cmr_s32 bottom, cmr_s32 top);

cmr_s32 _is_print_log();

cmr_s32 _pm_check_smart_param(struct smart_block_result *block_result,
			struct isp_range *range, cmr_u32 comp_num, cmr_u32 type);

cmr_s32 _pm_common_rest(void *blk_addr, cmr_u32 size);

cmr_u32 _pm_get_lens_grid_mode(cmr_u32 grid);

cmr_u16 _pm_get_lens_grid_pitch(cmr_u32 grid_pitch, cmr_u32 width, cmr_u32 flag);

void _pm_generate_bicubic_weight_table(
		cmr_s16 * lnc_bicubic_weight_t_simple, cmr_u32 lsc_grid);

cmr_u32 _ispLog2n(cmr_u32 index);
void ltm_rgb_text_thres_init(cmr_u32 thres_init, float factor, cmr_u32 *table);
cmr_u32 _pm_calc_nr_addr_offset(cmr_u32 mode_flag, cmr_u32 scene_flag, cmr_u32 * one_multi_mode_ptr);

/*******************************isp_pm_blocks******************************/

/* DCAM blocks start......*/
cmr_s32 _pm_blc_init(void *dst_blc_param, void *src_blc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_blc_set_param(void *blc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_blc_get_param(void *blc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_gain_init(void *dst_gbl_gain, void *src_gbl_gain, void *param1, void *param2);
cmr_s32 _pm_rgb_gain_set_param(void *gbl_gain_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_gain_get_param(void *gbl_gain_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_dither_init(void *dst_rgb_dither_param, void *src_rgb_dither_param, void *param1, void *param_ptr2);
cmr_s32 _pm_rgb_dither_set_param(void *rgb_dither_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_dither_get_param(void *rgb_dither_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_2d_lsc_init(void *dst_lnc_param, void *src_lnc_param, void *param1, void *param2);
cmr_s32 _pm_2d_lsc_set_param(void *lnc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_2d_lsc_get_param(void *lnc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
cmr_s32 _pm_2d_lsc_deinit(void *lnc_param);

cmr_s32 _pm_rgb_aem_init(void *dst_rgb_aem, void *src_rgb_aem, void *param1, void *param2);
cmr_s32 _pm_rgb_aem_set_param(void *rgb_aem_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_aem_get_param(void *rgb_aem_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_ae_adapt_init(void *dst_ae_adapt, void *src_ae_adapt, void *param1, void *param2);
cmr_s32 _pm_ae_adapt_set_param(void *ae_adapt_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ae_adapt_get_param(void *ae_adapt_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_awb_new_init(void *dst_awb_new_param, void *src_awb_new_param, void *param1, void *param_ptr2);
cmr_s32 _pm_awb_new_set_param(void *awb_new_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_awb_new_get_param(void *awb_new_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_bpc_init(void *dst_bpc_param, void *src_bpc_param, void *param1, void *param2);
cmr_s32 _pm_bpc_set_param(void *bpc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_bpc_get_param(void *bpc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_ppe_init(void *dst_ppe_param, void *src_ppe_param, void *param1, void *param2);
cmr_s32 _pm_ppe_set_param(void *ppe_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ppe_get_param(void *ppe_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_afm_init(void *dst_afm_param, void *src_afm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_rgb_afm_set_param(void *afm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_afm_get_param(void *afm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
/* DCAM blocks end......*/



/* ISP blocks start..... */

cmr_s32 _pm_bchs_init(void *dst_bchs_param, void *src_bchs_param, void *param1, void *param_ptr2);
cmr_s32 _pm_bchs_set_param(void *bchs_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_bchs_get_param(void *bchs_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_brightness_init(void *dst_brightness, void *src_brightness, void *param1, void *param2);
cmr_s32 _pm_brightness_set_param(void *bright_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_brightness_get_param(void *bright_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_contrast_init(void *dst_contrast, void *src_contrast, void *param1, void *param2);
cmr_s32 _pm_contrast_set_param(void *contrast_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_contrast_get_param(void *contrast_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_saturation_init(void *dst_csa_param, void *src_csa_param, void *param1, void *param_ptr2);
cmr_s32 _pm_saturation_set_param(void *csa_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_saturation_get_param(void *csa_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_hue_init(void *dst_hue_param, void *src_hue_param, void *param1, void *param_ptr2);
cmr_s32 _pm_hue_set_param(void *hue_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hue_get_param(void *hue_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_3dnr_init(void *dst_3d_nr_param, void *src_3d_nr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_3dnr_set_param(void *nr_3d_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_3dnr_get_param(void *nr_3d_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_cce_init(void *dst_cce_param, void *src_cce_param, void *param1, void *param2);
cmr_s32 _pm_cce_set_param(void *cce_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cce_get_param(void *cce_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_cfa_init(void *dst_cfae_param, void *src_cfae_param, void *param1, void *param_ptr2);
cmr_s32 _pm_cfa_set_param(void *cfae_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cfa_get_param(void *cfa_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_cmc10_init(void *dst_cmc10_param, void *src_cmc10_param, void *param1, void *param_ptr2);
cmr_s32 _pm_cmc10_set_param(void *cmc10_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cmc10_get_param(void *cmc10_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_edge_init(void *dst_edge_param, void *src_edge_param, void *param1, void *param2);
cmr_s32 _pm_edge_set_param(void *edge_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_edge_get_param(void *edge_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_frgb_gamc_init(void *dst_gamc_param, void *src_gamc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_frgb_gamc_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_frgb_gamc_get_param(void *gamc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_grgb_init(void *dst_grgb_param, void *src_grgb_param, void *param1, void *param2);
cmr_s32 _pm_grgb_set_param(void *grgb_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_grgb_get_param(void *grgb_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_hist2_init(void *dst_hist2_param, void *src_hist2_param, void *param1, void *param2);
cmr_s32 _pm_hist2_set_param(void *hist2_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hist2_get_param(void *hist2_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_hsv_init(void *dst_hsv_param, void *src_hsv_param, void *param1, void *param2);
cmr_s32 _pm_hsv_set_param(void *hsv_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hsv_get_param(void *hsv_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
cmr_s32 _pm_hsv_deinit(void *hsv_param);

cmr_s32 _pm_hsv_new_init(void *dst_hsv_param, void *src_hsv_param, void *param1, void *param2);
cmr_s32 _pm_hsv_new_set_param(void *hsv_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hsv_new_get_param(void *hsv_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
cmr_s32 _pm_hsv_new_deinit(void *hsv_param);

cmr_s32 _pm_hsv_new2_init(void *dst_hsv_param, void *src_hsv_param, void *param1, void *param2);
cmr_s32 _pm_hsv_new2_set_param(void *hsv_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_hsv_new2_get_param(void *hsv_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_iircnr_iir_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_iircnr_iir_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_iircnr_iir_get_param(void *iircnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_iircnr_yrandom_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_iircnr_yrandom_set_param(void *iircnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_iircnr_yrandom_get_param(void *iircnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_ltm_init(void *dst_ltm_param, void *src_ltm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_ltm_set_param(void *ltm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ltm_get_param(void *ltm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_rgb_ltm_init(void *dst_rgb_ltm_param, void *src_rgb_ltm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_rgb_ltm_set_param(void *rgb_ltm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_rgb_ltm_get_param(void *rgb_ltm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_yuv_ltm_init(void *dst_yuv_ltm_param, void *src_yuv_ltm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_yuv_ltm_set_param(void *yuv_ltm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yuv_ltm_get_param(void *yuv_ltm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_gtm_init(void *dst_gtm_param, void *src_gtm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_gtm_set_param(void *gtm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_gtm_get_param(void *gtm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_nlm_init(void *dst_nlm_param, void *src_nlm_param, void *param1, void *param_ptr2);
cmr_s32 _pm_nlm_set_param(void *nlm_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_nlm_get_param(void *nlm_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
cmr_s32 _pm_nlm_deinit(void *nlm_param);

cmr_s32 _pm_imblance_init(void *dst_imblance_param, void *src_imblance_param, void *param1, void *param_ptr2);
cmr_s32 _pm_imblance_set_param(void *imblance_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_imblance_get_param(void *imblance_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_posterize_init(void *dst_pstrz_param, void *src_pstrz_param, void *param1, void *param_ptr2);
cmr_s32 _pm_posterize_set_param(void *pstrz_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_posterize_get_param(void *pstrz_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_yuv_ygamma_init(void *dst_gamc_param, void *src_gamc_param, void *param1, void *param_ptr2);
cmr_s32 _pm_yuv_ygamma_set_param(void *gamc_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yuv_ygamma_get_param(void *gamc_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_ynr_init(void *dst_ynr_param, void *src_ynr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_ynr_set_param(void *ynr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ynr_get_param(void *ynr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_yuv_precdn_init(void *dst_precdn_param, void *src_precdn_param, void *param1, void *param2);
cmr_s32 _pm_yuv_precdn_set_param(void *precdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yuv_precdn_get_param(void *precdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_uv_cdn_init(void *dst_cdn_param, void *src_cdn_param, void *param1, void *param2);
cmr_s32 _pm_uv_cdn_set_param(void *cdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_uv_cdn_get_param(void *cdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_uv_postcdn_init(void *dst_postcdn_param, void *src_postcdn_param, void *param1, void *param_ptr2);
cmr_s32 _pm_uv_postcdn_set_param(void *postcdn_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_uv_postcdn_get_param(void *postcdn_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_yuv_noisefilter_init(void *dst_yuv_noisefilter_param, void *src_yuv_noisefilter_param, void *param1, void *param_ptr2);
cmr_s32 _pm_yuv_noisefilter_set_param(void *yuv_noisefilter_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_yuv_noisefilter_get_param(void *yuv_noisefilter_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_uv_div_init(void *dst_uv_div_param, void *src_uv_div_param, void *param1, void *param_ptr2);
cmr_s32 _pm_uv_div_set_param(void *uv_div_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_uv_div_get_param(void *uv_div_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);


/* ISP blocks end..... */


/* soft algo blocks */
cmr_s32 _pm_cnr2_init(void *dst_cnr2_param, void *src_cnr2_param, void *param1, void *param2);
cmr_s32 _pm_cnr2_set_param(void *cnr2_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cnr2_get_param(void *cnr2_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_ynrs_init(void *dst_ynrs_param, void *src_ynrs_param, void *param1, void *param2);
cmr_s32 _pm_ynrs_set_param(void *ynrs_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_ynrs_get_param(void *ynrs_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

#ifdef CAMERA_CNR3_ENABLE
cmr_u32 _pm_cnr3_convert_param(void *dst_cnr3_param, cmr_u32 strength_level, cmr_u32 mode_flag, cmr_u32 scene_flag);
cmr_s32 _pm_cnr3_init(void *dst_cnr3_param, void *src_cnr3_param, void *param1, void *param2);
cmr_s32 _pm_cnr3_set_param(void *cnr3_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_cnr3_get_param(void *cnr3_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);
#endif

cmr_s32 _pm_sw3dnr_init(void *dst_3d_nr_param, void *src_3d_nr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_sw3dnr_set_param(void *nr_3d_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_sw3dnr_get_param(void *nr_3d_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_mfnr_init(void *dst_mfnr_param, void *src_mfnr_param, void *param1, void *param_ptr2);
cmr_s32 _pm_mfnr_set_param(void *mfnr_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_mfnr_get_param(void *mfnr_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_dre_init(void *dst_dre_param, void *src_dre_param, void *param1, void *param2);
cmr_s32 _pm_dre_set_param(void *dre_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_dre_get_param(void *dre_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_fb_init(void *dst_fb_param, void *src_fb_param, void *param1, void *param2);
cmr_s32 _pm_fb_set_param(void *fb_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_fb_get_param(void *fb_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

cmr_s32 _pm_dre_pro_init(void *dst_dre_pro_param, void *src_dre_param, void *param1, void *param2);
cmr_s32 _pm_dre_pro_set_param(void *dre_pro_param, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
cmr_s32 _pm_dre_pro_get_param(void *dre_pro_param, cmr_u32 cmd, void *rtn_param0, void *rtn_param1);

struct isp_block_operations {
	cmr_s32(*init) (void *blk_ptr, void *param_ptr0, void *param_ptr1, void *param_ptr2);
	cmr_s32(*set) (void *blk_ptr, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
	cmr_s32(*get) (void *blk_ptr, cmr_u32 cmd, void *param_ptr0, void *param_ptr1);
	cmr_s32(*reset) (void *blk_ptr, cmr_u32 size);
	cmr_s32(*deinit) (void *blk_ptr);
};

struct isp_block_cfg {
	cmr_u32 id;
	cmr_u32 offset;
	cmr_u32 param_size;
	struct isp_block_operations *ops;
};

struct isp_block_cfg *isp_pm_get_block_cfg(cmr_u32 id);

#ifdef	 __cplusplus
}
#endif
#endif
