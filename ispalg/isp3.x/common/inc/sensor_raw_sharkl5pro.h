/*
 * Copyright (C) 2012 The Android Open Source Project
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
#ifndef _SENSOR_RAW_SHARKL5PRO_H_
#define _SENSOR_RAW_SHARKL5PRO_H_

#ifndef WIN32
#include <sys/types.h>
#else
#include ".\isp_type.h"
#endif
#include "isp_type.h"
#include "cmr_sensor_info.h"
#include "../../ae/sprd/ae/inc/ae_tuning_type.h"

//Gamma Correction in full RGB domain
#define SENSOR_GAMMA_POINT_NUM 257

#define AWB_POS_WEIGHT_LEN 64
#define AWB_POS_WEIGHT_WIDTH_HEIGHT 4

#define MAX_MODE_NUM 16
#define MAX_NR_NUM 32


#define MAX_SCENEMODE_NUM 16
#define MAX_SPECIALEFFECT_NUM 16

#define SENSOR_AWB_CALI_NUM 0x09
#define SENSOR_PIECEWISE_SAMPLE_NUM 0x10
#define SENSOR_ENVI_NUM 6

#define RAW_INFO_END_ID 0x71717567

#define SENSOR_MULTI_MODE_FLAG  0x55AA5A5A
#define SENSOR_DEFAULT_MODE_FLAG  0x00000000

#define AE_FLICKER_NUM 2
#define AE_ISO_NUM_NEW 8
#define AE_SCENE_NUM 8
#define AEC_EXP_GAIN_TABLE_SIZE 512
#define LNC_MAP_COUNT 9
#define LNC_WEIGHT_LEN 4096
#define PDAF_GAIN_MAP_LEN 128
#define VST_IVST_NUM 1025

#define CNR_LEVEL 4

#define AE_VERSION    0x00000000
#define AWB_VERSION   0x00000000
#define NR_VERSION    0x00000000
#define LNC_VERSION   0x00000000
#define PM_CODE_VERSION  0x00070005

#define TUNE_FILE_CHIP_VER_MASK 0x0000FFFF
#define TUNE_FILE_SW_VER_MASK 0xFFFF0000

#define TUNE_FILE_CHIP_VER_MASK 0x0000FFFF
#define TUNE_FILE_SW_VER_MASK 0xFFFF0000

#define _HAIT_MODIFIED_FLAG
#define SENSOR_CCE_NUM 0x09
#define SENSOR_CMC_NUM 16
#define SENSOR_CTM_NUM 0x09
#define SENSOR_HSV_NUM 32
#define SENSOR_LENS_NUM 0x09
#define SENSOR_BLC_NUM 0x09
#define SENSOR_MODE_NUM 0x02
#define SENSOR_AWBM_WHITE_NUM 0x05
#define SENSOR_LNC_RC_NUM 0x04
#define SENSOR_HIST2_ROI_NUM 0x04
#define SENSOR_GAMMA_POINT_NUM	257
#define SENSOR_GAMMA_NUM 9
#define SENSOR_LEVEL_NUM 16
#define SENSOR_CMC_POINT_NUM 9
//#define SENSOR_SMART_LEVEL_NUM 25
//#define SENSOR_SMART_LEVEL_DEFAULT 15
#define AE_WEIGHT_TABLE_NUM 3

enum isp_scene_mode {
	ISP_SCENEMODE_AUTO = 0x00,
	ISP_SCENEMODE_NIGHT,
	ISP_SCENEMODE_SPORT,
	ISP_SCENEMODE_PORTRAIT,
	ISP_SCENEMODE_LANDSCAPE,
	ISP_SCENEMODE_PANORAMA,
	ISP_SCENEMODE_SNOW,
	ISP_SCENEMODE_FIREWORK,
	ISP_SCENEMODE_DUSK,
	ISP_SCENEMODE_AUTUMN,
	ISP_SCENEMODE_TEXT,
	ISP_SCENEMODE_BACKLIGHT,
	ISP_SCENEMODE_MAX
};

enum isp_special_effect_mode {
	ISP_EFFECT_NORMAL = 0x00,
	ISP_EFFECT_GRAY,
	ISP_EFFECT_WARM,
	ISP_EFFECT_GREEN,
	ISP_EFFECT_COOL,
	ISP_EFFECT_ORANGE,
	ISP_EFFECT_NEGTIVE,
	ISP_EFFECT_OLD,
	ISP_EFFECT_EMBOSS,
	ISP_EFFECT_POSTERIZE,
	ISP_EFFECT_CARTOON,
	ISP_EFFECT_MAX
};


/* Following enum/structure used for new isp pm searching solution */

/* ***Custom*** */
enum tuning_custom {
	DEFMODE_DEFAULT = 0x0000,
	DEFMODE_CUSTOM0,
	DEFMODE_CUSTOM1,
	DEFMODE_CUSTOM2,
	DEFMODE_CUSTOM_MAX
};

/* ***Mode*** */
enum tuning_mode {
	WORKMODE_PREVIEW = 0x0000,
	WORKMODE_CAPTURE,
	WORKMODE_VIDEO,
	WORKMODE_MAX,
};

/* ***Scene (app scene mode)*** */
enum tuning_scene_mode {
	//normal scene
	SCENEMODE_NROMAL = 0x0000,
	SCENEMODE_NIGHT,
	SCENEMODE_SPORT, //0x0002
	SCENEMODE_PORTRAIT,
	SCENEMODE_LANDSCAPE,
	SCENEMODE_PANORAMA, //0x0005
	SCENEMODE_BOKEH,
	SCENEMODE_FIREWORK,
	SCENEMODE_DUSK,
	SCENEMODE_AUTUMN,   //0x0009
	SCENEMODE_SLOWMOTION,

	//known AI  scene
	SCENEMODE_AI_NIGHT = 0x0100,
	SCENEMODE_AI_BACKLIGHT,
	SCENEMODE_AI_SUNRISESET,
	SCENEMODE_AI_FIREWORK,
	SCENEMODE_AI_OTHERS,
	SCENEMODE_AI_FOOD,
	SCENEMODE_AI_GREENPLANT,
	SCENEMODE_AI_DOCUMENT, //texture
	SCENEMODE_AI_CATDOG,
	SCENEMODE_AI_FLOWER,
	SCENEMODE_AI_BLUESKY,
	SCENEMODE_AI_BUILDING,
	SCENEMODE_AI_SNOW,

	//custom scene
	SCENEMODE_WECHAT= 0x0200,
	SCENEMODE_HDR,
	SCENEMODE_FACE_YELLOW,
	SCENEMODE_FACE_BLACK,
	SCENEMODE_FACE_WHITE,
	SCENEMODE_CUSTOM0,
	SCENEMODE_CUSTOM1,
	SCENEMODE_CUSTOM2,

	SCENEMODE_MAX
};

struct sensor_find_param_list {
    cmr_u32 custom; /* should be enum tuning_custom */
    cmr_u32 scene;  /* should be enum tuning_scene_mode*/
    cmr_u32 mode;  /* should be enum tuning_mode*/
    cmr_u32 size_w;
    cmr_u32 size_h;
    cmr_u32 blk_id;
    cmr_u32 param_mode_id;
    cmr_u32 param_scene_id;
};
/* End of new isp pm searching solution enum/structure define */


//testcode,should ask AE module to change struct define mothod
struct ae_exp_gain_index_2 {
	cmr_u32 min_index;
	cmr_u32 max_index;
};
struct ae_exp_gain_table_2 {
	struct ae_exp_gain_index_2 index;
	cmr_u32 exposure[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u32 dummy[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u16 again[AE_EXP_GAIN_TABLE_SIZE];
	cmr_u16 dgain[AE_EXP_GAIN_TABLE_SIZE];
};

//AE 2.5
struct ae_scence_info_header_2 {
	cmr_u32 enable;
	cmr_u32 scene_mode;
	cmr_u32 target_lum;
	cmr_u32 iso_index;
	cmr_u32 ev_offset;
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 weight_mode;
	cmr_u32 default_index;
	cmr_u32 table_enable;
};

struct ae_scene_info_2 {
	struct ae_scence_info_header_2 ae_header;
	struct ae_exp_gain_table_2 ae_table[AE_FLICKER_NUM];
};

struct ae_table_param_2 {
	struct ae_exp_gain_table_2 ae_table[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_exp_gain_table_2 flash_table[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_weight_table weight_table[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_info_2 scene_info[AE_SCENE_NUM];
	struct ae_auto_iso_tab auto_iso_tab;
};

struct sensor_nr_header_param {
	cmr_u32 level_number;
	cmr_u32 default_strength_level;
	cmr_u32 nr_mode_setting;
	cmr_uint *multi_nr_map_ptr;
	cmr_uint *param_ptr;
	cmr_uint *param1_ptr;
	cmr_uint *param2_ptr;
	cmr_uint *param3_ptr;
};

struct sensor_nr_simple_header_param {
	cmr_u32 level_number;
	cmr_u32 default_strength_level;
	cmr_u32 nr_mode_setting;
	cmr_uint *multi_nr_map_ptr;
	cmr_uint *param_ptr;
};

struct dual_flash_tune_param {
	cmr_u8 version;		/*version 0: just for old flash controlled by AE algorithm, and Dual Flash must be 1 */
	cmr_u8 alg_id;
	cmr_u8 flashLevelNum1;
	cmr_u8 flashLevelNum2;	/*1 * 4bytes */
	cmr_u8 preflahLevel1;
	cmr_u8 preflahLevel2;
	cmr_u16 preflashBrightness;	/*1 * 4bytes */

	cmr_u16 brightnessTarget;	//10bit
	cmr_u16 brightnessTargetMax;	//10bit
	/*1 * 4bytes */

	cmr_u32 foregroundRatioHigh;	/*fix data: 1x-->100 */
	cmr_u32 foregroundRatioLow;	/*fix data: 1x-->100 */
	/*2 * 4bytes */
	cmr_u8 flashMask[1024];	/*256 * 4bytes */
	cmr_u16 brightnessTable[1024];	/*512 * 4bytes */
	cmr_u16 rTable[1024];	//g: 1024 /*512 * 4bytes*/
	cmr_u16 bTable[1024];	/*512 * 4bytes */

	cmr_u8 reserved1[1024];	/*256 * 4bytes */
};				/*2053 * 4 bypes */

//capture bokeh
struct bokeh_dof_params{
	cmr_u16 dof_offset;
    float dof_ratio;
    float slope_fore;
    float slope_back;
    cmr_u16 reserved[5];
};

//preview bokeh
struct bokeh_init_params {
    cmr_u8 smooth_win_size;//odd number
    cmr_u8 reserved[3];
};

//depth
struct bokeh_depth_params {
    cmr_u8 sensor_direction; //1 vertical 0 horizontal
    cmr_u16 search_range;
    cmr_u8 enable;
    cmr_u16 reserved[6];
};

struct bokeh_dualcam_params {
    struct bokeh_depth_params depth_params;
    struct bokeh_init_params init_params;
    struct bokeh_dof_params dof_params;
    cmr_u32 bypass;
};

struct bokeh_dualcam_tuning_param {
    struct bokeh_dualcam_params bokeh_params;
    cmr_u16 version;
    cmr_u16 reserve[31];
};

struct bokeh_micro_depth_tune_param {
	cmr_u32 tuning_exist;
	cmr_u32 enable;
	cmr_u32 fir_mode;
	cmr_u32 fir_len;
	cmr_s32 hfir_coeff[7];
	cmr_s32 vfir_coeff[7];
	cmr_u32 fir_channel;
	cmr_u32 fir_cal_mode;
	cmr_s32 fir_edge_factor;
	cmr_u32 depth_mode;
	cmr_u32 smooth_thr;
	cmr_u32 touch_factor;
	cmr_u32 scale_factor;
	cmr_u32 refer_len;
	cmr_u32 merge_factor;
	cmr_u32 similar_factor;
	cmr_u32 similar_coeff[3];
	cmr_u32 tmp_mode;
	cmr_s32 tmp_coeff[8];
	cmr_u32 tmp_thr;
	cmr_u32 reserved[100];
};

struct pdaf_tune_param {
	cmr_u32 min_pd_vcm_steps;
	cmr_u32 max_pd_vcm_steps;
	cmr_u32 coc_range;
	cmr_u32 far_tolerance;
	cmr_u32 near_tolerance;
	cmr_u32 err_limit;
	cmr_u32 pd_converge_thr;
	cmr_u32 pd_converge_thr_2nd;
	cmr_u32 pd_focus_times_thr;
	cmr_u32 pd_thread_sync_frm;
	cmr_u32 pd_thread_sync_frm_init;
	cmr_u32 min_process_frm;
	cmr_u32 max_process_frm;
	cmr_u32 pd_conf_thr;
	cmr_u32 pd_conf_thr_2nd;
	cmr_u32 reserved[52];
};

struct haf_tune_param {
	//default param for outdoor/indoor/dark
	struct pdaf_tune_param pdaf_tune_data[3];
	cmr_u32 reserved[256];
};

struct anti_flicker_tune_param {
	cmr_u32 normal_50hz_thrd;
	cmr_u32 lowlight_50hz_thrd;
	cmr_u32 normal_60hz_thrd;
	cmr_u32 lowlight_60hz_thrd;
	cmr_u32 reserved[16];
};

//environment detec
struct sensor_envi_detect_param {
	cmr_u32 enable;
	cmr_u32 num;
	struct isp_range envi_range[SENSOR_ENVI_NUM];
};

//Black level correction, between preGain and PDAF
struct sensor_blc_offset {
	cmr_u16 r;
	cmr_u16 gr;
	cmr_u16 gb;
	cmr_u16 b;
};

struct sensor_blc_param {
	struct isp_sample_point_info cur_idx;
	struct sensor_blc_offset tab[SENSOR_BLC_NUM];
};

// ppe, pdaf extr ctrl
struct sensor_pdaf_af_win {
	cmr_u16 af_win_sx0;
	cmr_u16 af_win_sy0;
	cmr_u16 af_win_ex0;
	cmr_u16 af_win_ey0;
};

struct sensor_pdaf_pattern_map {
	cmr_u32 is_right;
	struct isp_pos pattern_pixel;
};

struct sensor_pdaf_gain_map_pos{
	cmr_s32 x;
	cmr_s32 y;
};

struct sensor_pdaf_param {
	cmr_u16 pdaf_gr;
	cmr_u16 pdaf_r;
	cmr_u16 pdaf_b;
	cmr_u16 pdaf_gb;
};

struct sensor_pdaf_block_pos{
	cmr_u16 block_start_row;
	cmr_u16 block_start_col;
	cmr_u16 block_end_row;
	cmr_u16 block_end_col;
};

struct sensor_ppe_block_size{
	cmr_u16 width;
	cmr_u16 height;
};

struct sensor_ppe_level {
	struct sensor_pdaf_pattern_map pdaf_pattern[64];
	struct sensor_pdaf_af_win pdaf_af_win;
	struct sensor_pdaf_block_pos pdaf_block_pos;
	struct sensor_ppe_block_size ppe_block_size;
	cmr_u16 pdaf_l_gain_map[PDAF_GAIN_MAP_LEN];
	cmr_u16 pdaf_r_gain_map[PDAF_GAIN_MAP_LEN];
	cmr_u8 continuous_mode;
	cmr_u8 skip_num;
	cmr_u16 gain_map_grid;
	cmr_u16 phase_map_corr_en;
	cmr_u8 ppe_bypass;
	cmr_u8 ppe_extractor_en;
};

//rgb gain
struct sensor_rgb_gain_param {
	cmr_u16 glb_gain;
	cmr_u16 r_gain;
	cmr_u16 g_gain;
	cmr_u16 b_gain;
};

//YUV noisefilter
struct sensor_yuv_noisefilter_gaussian {
	cmr_u16 random_r_shift;
	cmr_s16 random_r_offset;
	cmr_u32 random_seed[4];
	cmr_u8 random_takebit[8];
};

struct sensor_yuv_noisefilter_adv {
	cmr_u8 filter_thr;
	cmr_u8 filter_thr_mode;
	cmr_u8 filter_clip_p;
	cmr_s8 filter_clip_n;
	cmr_u16 filter_cv_t[4];
	cmr_u8 filter_cv_r[3];
	cmr_u8 reserved;
};

struct sensor_yuv_noisefilter_level {
	cmr_u32 noisefilter_shape_mode;
	cmr_u32 noisefilter_random_seed_mode;
	struct sensor_yuv_noisefilter_gaussian yuv_noisefilter_gaussian;
	struct sensor_yuv_noisefilter_adv yuv_noisefilter_adv;
	cmr_u32 bypass;
};

//rgb gain yrandom, as a separate module in NR
struct sensor_rgb_dither_level {
	cmr_u8 pseudo_random_raw_range;
	cmr_u8 yrandom_mode;
	cmr_u16 yrandom_shift;
	cmr_u32 yrandom_seed;
	cmr_u16 yrandom_offset;
	cmr_u16 reserved;
	cmr_u8 yrandom_takebit[8];
	cmr_u32 pseudo_random_raw_bypass;
};

// 2D-lens shading correction
struct sensor_lsc_2d_map_info {
	cmr_u32 envi;
	cmr_u32 ct;
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 grid;
};
struct sensor_lsc_2d_tab_info_param {
	struct sensor_lsc_2d_map_info lsc_2d_map_info;
	cmr_u16 lsc_2d_weight[LNC_WEIGHT_LEN];
	cmr_u32 lsc_2d_len;
	cmr_u32 lsc_2d_offset;
};
struct sensor_lsc_2d_table_param {
	struct sensor_lsc_2d_tab_info_param lsc_2d_info[LNC_MAP_COUNT];
	cmr_u16 lsc_2d_map[];
};
struct sensor_2d_lsc_param {
	struct isp_sample_point_info cur_idx;
	cmr_u32 tab_num;
	struct sensor_lsc_2d_table_param tab_info;
};

// AWB Correction
struct sensor_awbc_thr {
	cmr_u16 r_thr;
	cmr_u16 g_thr;
	cmr_u16 b_thr;
	cmr_u16 reserved;
};

struct sensor_awbc_gain {
	cmr_u16 r_gain;
	cmr_u16 b_gain;
	cmr_u16 gr_gain;
	cmr_u16 gb_gain;
};

struct sensor_awbc_offset {
	cmr_u16 r_offset;
	cmr_u16 b_offset;
	cmr_u16 gr_offset;
	cmr_u16 gb_offset;
};

struct sensor_awbc_param {
	struct sensor_awbc_gain awbc_gain;
	struct sensor_awbc_thr awbc_thr;
	struct sensor_awbc_offset awbc_offset;
};

struct sensor_awbm_param {
	cmr_u32 comp_1d_bypass;
	cmr_u32 skip_num;
	struct isp_pos win_start;
	struct isp_size win_size;
	struct isp_pos_rect white_area[SENSOR_AWBM_WHITE_NUM];
	struct sensor_awbc_thr low_thr;
	struct sensor_awbc_thr high_thr;
};

struct sensor_awb_param {
	struct sensor_awbc_param awbc;
	struct sensor_awbm_param awbm;
};

// AE monitor in RGB domain
struct sensor_rgb_aem_param {
	struct isp_size blk_num;
	cmr_u32 reserved[6];
};

struct sensor_bpc_pixel_pos{
	cmr_s32 x;
	cmr_s32 y;
};

// bwu1
struct sensor_bwu1{
	cmr_u8 bwu1_bypass;
	cmr_u8 bwu1_bitshift;
	cmr_u8 reserved[2];
};

// bwu2
struct sensor_bwu2{
	cmr_u8 bwu2_bypass;
	cmr_u8 bwu2_outbits;
	cmr_u8 reserved[2];
};

// bwd1
struct sensor_bwd1{
	cmr_u8 bwd1_bypass;
	cmr_u8 bwd1_outbits;
	cmr_u8 reserved[2];
};

// bwd2
struct sensor_bwd2{
	cmr_u8 bwd2_bypass;
	cmr_u8 bwd2_outbits;
	cmr_u8 reserved[2];
};

// bwu,bwd block
struct sensor_bwu_bwd_level{
	struct sensor_bwu1 bwu1;
	struct sensor_bwu2 bwu2;
	struct sensor_bwd1 bwd1;
	struct sensor_bwd2 bwd2;
};


// Bad Pixel Correction
struct sensor_bpc_ppi_block{
	cmr_u32 ppi_en;
	cmr_u16 block_start_col;
	cmr_u16 block_start_row;
	cmr_u16 block_end_col;
	cmr_u16 block_end_row;
};
struct sensor_bpc_comm {
	cmr_u8 bpc_mode;	//0:normal,1:map,2:both
	cmr_u8 hv_mode;
	cmr_u8 rd_mode;		//0:3x3,1:5x5,2:line mask
	cmr_u8 isMonoSensor;	//sharkl3
	cmr_u8 double_bypass;	//sharkl3
	cmr_u8 three_bypass;	//sharkl3
	cmr_u8 four_bypass;	//sharkl3
	cmr_u8 reserved;
	cmr_u16 lut_level[8];
	cmr_u16 slope_k[8];
	cmr_u16 intercept_b[8];
	float dtol;
};

struct sensor_bpc_pos_out {
	cmr_u8 continuous_mode;
	cmr_u8 skip_num;
	cmr_u8 reserved[2];
};

struct sensor_bpc_thr {
	cmr_u16 double_th[4];
	cmr_u16 three_th[4];
	cmr_u16 four_th[4];
	cmr_u8 shift[3];
	cmr_u8 reserved;
	cmr_u16 flat_th;
	cmr_u16 texture_th;
};

struct sensor_bpc_kvalue {
	cmr_s16 min;
	cmr_s16 max;
};

struct sensor_bpc_rules {
	struct sensor_bpc_kvalue k_val;
	cmr_u8 lowcoeff;
	cmr_u8 lowoffset;
	cmr_u8 highcoeff;
	cmr_u8 highoffset;
	cmr_u16 hv_ratio;
	cmr_u16 rd_ratio;
};

struct sensor_bpc_level {
	struct sensor_bpc_comm bpc_comm;
	struct sensor_bpc_pos_out bpc_pos;
	struct sensor_bpc_thr bpc_thr;
	struct sensor_bpc_rules bpc_rules;
	struct sensor_bpc_ppi_block ppi_block;
	cmr_u16 bpc_result_en;
	cmr_u16 bypass;
};

struct sensor_ynr_addback{
	cmr_u16 addback_enable;
	cmr_u8 addback_ratio;
	cmr_u8 addback_clip;
};
struct sensor_ynr_blf_range{
	cmr_u32 s0_low;
	cmr_u32 s0_mid;
	cmr_u32 s0_high;
};
struct sensor_ynr_blf_str{
	struct sensor_ynr_blf_range blf_range;
	cmr_u16 max_Radius;
	cmr_u16 max_Radius_factor;
	cmr_u16 Radius;
	cmr_u16 Radius_factor;
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u8 range_index;
	cmr_u8 range_s[4];
	cmr_u8 dist_weight[3];
	cmr_u32 ynr_blf_enable;
};
struct sensor_ynr_wv_str {
	cmr_u8 wv_thresh1;
	cmr_u8 wv_thresh2_n;
	cmr_u8 wv_ratio1;
	cmr_u8 wv_ratio2;
	cmr_u8 wv_thresh_d1;
	cmr_u8 wv_thresh_d2_n;
	cmr_u8 wv_ratio_d1;
	cmr_u8 wv_ratio_d2;
	cmr_u16 soft_offset;
	cmr_u16 soft_offsetd;
};
struct sensor_ynr_den_str {
	cmr_u32 wv_nr_enable;
	struct sensor_ynr_wv_str low;
	struct sensor_ynr_wv_str mid;
	struct sensor_ynr_wv_str high;
};
struct sensor_ynr_sal_str{
	cmr_u32 sal_enable;
	cmr_u8 lum_thresh[2];
	cmr_u8 sal_offset[8];
	cmr_u8 sal_thresh[7];
	cmr_u8 sal_nr_str[8];
	cmr_u8 reserved[3];
};

struct sensor_ynr_level {
	struct sensor_ynr_sal_str ynr_sal_str;
	struct sensor_ynr_den_str ynr_wv_parm[3];
	struct sensor_ynr_blf_str ynr_blf_str;
	struct sensor_ynr_addback addback[4];
	cmr_u16 coef_mode;
	cmr_u16 radius_base;
	cmr_u16 reserved;
	cmr_u16 bypass;
};

// UVDIV
struct sensor_cce_uvdiv_th {
	cmr_s8 uvdiv_th_l;
	cmr_s8 uvdiv_th_h;
	cmr_s8 reserved[2];
};

struct sensor_cce_uvdiv_chroma {
	cmr_u8 chroma_max_h;
	cmr_u8 chroma_max_l;
	cmr_u8 reserved[2];
};

struct sensor_cce_uvdiv_lum {
	cmr_u8 lum_th_h_len;
	cmr_u8 lum_th_h;
	cmr_u8 lum_th_l_len;
	cmr_u8 lum_th_l;
};

struct sensor_cce_uvdiv_ratio {
	cmr_u8 chroma_ratio;
	cmr_u8 luma_ratio;
	cmr_u8 ratio_0;
	cmr_u8 ratio_1;
	cmr_u8 ratio_uv_min;
	cmr_u8 ratio_y_min0;
	cmr_u8 ratio_y_min1;
	cmr_u8 reserved;
};

struct sensor_cce_uvdiv_level {
	struct sensor_cce_uvdiv_lum uvdiv_lum;
	struct sensor_cce_uvdiv_chroma uvdiv_chroma;
	struct sensor_cce_uvdiv_th u_th_0;
	struct sensor_cce_uvdiv_th u_th_1;
	struct sensor_cce_uvdiv_th v_th_0;
	struct sensor_cce_uvdiv_th v_th_1;
	struct sensor_cce_uvdiv_ratio uvdiv_ratio;
	cmr_u8 uv_abs_th_len;
	cmr_u8 y_th_l_len;
	cmr_u8 y_th_h_len;
	cmr_u8 bypass;
};

struct sensor_3dnr_thr_cfg{
    cmr_u8 src_wgt[4];// dynamic weight needs 4 buffer
    cmr_u8 nr_thr;
    cmr_u8 nr_wgt;
    cmr_u8 reserved[2];//for 4 bytes align
    cmr_u16 thr_polyline_cap[9];
    cmr_u16 gain_polyline_cap[9];
};

struct sensor_3dnr_yuv_cfg {
	cmr_u8 grad_wgt_polyline_cap[11];
	cmr_u8 reserved;
	struct sensor_3dnr_thr_cfg y_cfg;
	struct sensor_3dnr_thr_cfg u_cfg;
	struct sensor_3dnr_thr_cfg v_cfg;
};

struct sensor_3dnr_radialval {
	cmr_u16 min_cap;
	cmr_u16 max_cap;
};

struct sensor_3dnr_factor {
    cmr_u8 u_thr[4];
    cmr_u8 v_thr[4];
    cmr_u8 u_div[4];
    cmr_u8 v_div[4];
};

//reduce color noise around corners
struct sensor_3dnr_radialval_str {
	cmr_u16 r_circle_cap[3];
	cmr_u16 r_circle_cap_factor[3]; //r_circle_cap_factor/radius_base*(w+h) = r_circle_cap
	struct sensor_3dnr_radialval u_range;
	struct sensor_3dnr_radialval v_range;
	struct sensor_3dnr_factor uv_factor;
};
//mem_ctrl_submodule
struct sensor_3dnr_ctrl{
	cmr_u16 mv_x;
	cmr_u16 mv_y;
};

// dcam 3dnr ctrl
struct sensor_3dnr_fast_me {
	cmr_u8 bypass;
	cmr_u8 sub_me_bypass;
	cmr_u8 project_mode;
	cmr_u8 channel_sel;
};

struct sensor_3dnr_level {
	cmr_u8 bypass;
	cmr_u8 fusion_mode;
	cmr_u8 filter_swt_en;
	cmr_u8 reserved[3];
	cmr_u16 radius_base;
	struct sensor_3dnr_yuv_cfg yuv_cfg;
	struct sensor_3dnr_radialval_str sensor_3dnr_cor;
	struct sensor_3dnr_fast_me fast_me;
};

//GrGb Correction
struct sensor_grgb_ratio {
	cmr_u8 gr_ratio;
	cmr_u8 gb_ratio;
	cmr_u16 reserved;
};
struct sensor_grgb_t_cfg {
	cmr_u16 grgb_t1_cfg;
	cmr_u16 grgb_t2_cfg;
	cmr_u16 grgb_t3_cfg;
	cmr_u16 grgb_t4_cfg;
};

struct sensor_grgb_r_cfg {
	cmr_u8 grgb_r1_cfg;
	cmr_u8 grgb_r2_cfg;
	cmr_u8 grgb_r3_cfg;
	cmr_u8 reserved;
};
struct sensor_grgb_curve {
	struct sensor_grgb_t_cfg t_cfg;
	struct sensor_grgb_r_cfg r_cfg;
};

struct sensor_grgb_level {
	cmr_u8 diff_th;
	cmr_u8 hv_edge_thr;
	cmr_u16 hv_flat_thr;
	cmr_u16 slash_edge_thr;
	cmr_u16 slash_flat_thr;
	struct sensor_grgb_ratio grgb_ratio;
	struct sensor_grgb_curve lum_curve_edge;
	struct sensor_grgb_curve lum_curve_flat;
	struct sensor_grgb_curve lum_curve_tex;
	struct sensor_grgb_curve frez_curve_edge;
	struct sensor_grgb_curve frez_curve_flat;
	struct sensor_grgb_curve frez_curve_tex;
	cmr_u32 bypass;
};

//rgb gain 2
struct sensor_rgb_gain2_param {
	cmr_u16 r_gain;
	cmr_u16 g_gain;
	cmr_u16 b_gain;
	cmr_u16 r_offset;
	cmr_u16 g_offset;
	cmr_u16 b_offset;
};

// nlm
struct sesor_simple_bpc {
	cmr_u8 simple_bpc_bypass;
	cmr_u8 simple_bpc_thr;
	cmr_u16 simple_bpc_lum_thr;
};

struct sensor_nlm_flat_degree {
	cmr_u8 flat_inc_str;
	cmr_u8 flat_match_cnt;
	cmr_u16 flat_thresh;
	cmr_u16 addback0;	//for G channel
	cmr_u16 addback1;	//for R and B channel
	cmr_u16 addback_clip_max;	//plus noise
	cmr_u16 addback_clip_min;	//minus noise
};

struct sensor_nlm_texture {
	cmr_u8 texture_dec_str;
	cmr_u8 addback30;
	cmr_u8 addback31;
	cmr_u8 reserved;
	cmr_u16 addback_clip_max;
	cmr_u16 addback_clip_min;
};

struct sensor_nlm_lum {
	struct sensor_nlm_flat_degree nlm_flat[3]; //each luma levle includes 4 flats(flat0,flat1,flat2,flat3)
	struct sensor_nlm_texture nlm_texture;
};

struct sensor_nlm_flat_thresh {
	cmr_u16 flat_thresh_max0;
	cmr_u16 flat_thresh_coef0;
	cmr_u16 flat_thresh_max1;
	cmr_u16 flat_thresh_coef1;
	cmr_u16 flat_thresh_max2;
	cmr_u16 flat_thresh_coef2;
};

struct sensor_nlm_direction_addback {
	cmr_u16 direction_addback;
	cmr_u16 direction_addback_noise_clip;
};

struct sensor_nlm_direction_addback_lum {
	cmr_u16 mode_bypass;
	cmr_u16 reserved;
	struct sensor_nlm_direction_addback da[4];
};

struct sensor_nlm_first_lum {
	cmr_u16 nlm_flat_opt_bypass;
	cmr_u16 first_lum_bypass;
	cmr_u16 lum_thr0;
	cmr_u16 lum_thr1;
	struct sensor_nlm_lum nlm_lum[3]; //- 3 luma:level  luma0, luma1, luma2
	struct sensor_nlm_flat_thresh nlm_flat_thr[3];
	struct sensor_nlm_direction_addback_lum dal[3];
};

struct sensor_nlm_radius_param {
	cmr_u16 radius_threshold_filter_ratio;
	cmr_u16 radius_threshold_filter_ratio_factor;
	cmr_u16 coef2;
	cmr_u16 protect_gain_min;
	cmr_u16 reserved[2];
};

struct sensor_nlm_radius_1D {
	cmr_u16 cal_radius_bypass;
	cmr_u16 center_y;
	cmr_u16 center_x;
	cmr_u16 update_flat_thr_bypass;
	cmr_u16 radius_threshold;
	cmr_u16 radius_threshold_factor;
	cmr_u16 reserved1;
	cmr_u16 radial_1D_bypass;
	struct sensor_nlm_radius_param radius[3][4];
	cmr_u16 protect_gain_max;
	cmr_u16 reserved0;
};

struct sensor_nlm_direction {
	cmr_u8 direction_mode_bypass;
	cmr_u8 dist_mode;
	cmr_u8 w_shift[3];
	cmr_u8 cnt_th;
	cmr_u8 reserved[2];
	cmr_u16 diff_th;
	cmr_u16 tdist_min_th;
};

struct sensor_vst_level {
	cmr_u32 vst_param[VST_IVST_NUM];
};

struct sensor_ivst_level {
	cmr_u32 ivst_param[VST_IVST_NUM];
};

struct sensor_lutw_level {
	cmr_u32 lut_w[72];
};

struct sensor_nlm_level {
	struct sensor_nlm_first_lum first_lum;
	struct sensor_nlm_radius_1D radius_1d;
	struct sensor_nlm_direction nlm_dic;
	struct sesor_simple_bpc simple_bpc;
	struct sensor_lutw_level lut_w;
	cmr_u8 imp_opt_bypass;
	cmr_u8 vst_bypass;
	cmr_u8 nlm_bypass;
	cmr_u8 reserved[3];
	cmr_u16 radius_base;
};

struct sensor_nlm_imblance_lum_flag {
	cmr_u16 nlm_imblance_lum_flag_0_rs;
	cmr_u16 nlm_imblance_lum_flag_0_r;
	cmr_u16 nlm_imblance_lum_flag_1_r;
	cmr_u16 nlm_imblance_lum_flag_2_r;
	cmr_u16 nlm_imblance_lum_flag_3_r;
	cmr_u16 nlm_imblance_lum_flag_4_r;
};

struct sensor_imblance_flag3{
	cmr_u16 flag3_grid;
	cmr_u16 flag3_lum;
	cmr_u16 flag3_frez;
	cmr_u16 flag12_frezthr;
};

struct sensor_imblance_face_rgb_level{
	cmr_u16 face_max;
	cmr_u16 face_min;
};
struct sensor_imblance_face_rgb{
	struct sensor_imblance_face_rgb_level face_r;
	struct sensor_imblance_face_rgb_level face_g;
	struct sensor_imblance_face_rgb_level face_b;
};
//sensor_nlm_imblance
struct sensor_imblance_hv{
	cmr_u16 imblance_hv_edge_thr;
	cmr_u16 imblance_hv_slash_edge_thr;
	cmr_u16 imblance_hv_flat_thr;
	cmr_u16 imblance_slash_flat_thr;
};

struct sensor_imblance_curve{
	cmr_u16 imblance_curve_wt0;
	cmr_u16 imblance_curve_wt1;
	cmr_u16 imblance_curve_wt2;
	cmr_u16 imblance_curve_wt3;
	cmr_u8 imblance_curve_wr0;
	cmr_u8 imblance_curve_wr1;
	cmr_u8 imblance_curve_wr2;
	cmr_u8 imblance_curve_wr3;
	cmr_u8 imblance_curve_wr4;
	cmr_u8 resvered[3];  //for 4-byte alignment
};

struct sensor_imblance_1D{
	cmr_u16 imblance_radial_1D_en;
	cmr_u16 imblance_radial_1D_center_x;
	cmr_u16 imblance_radial_1D_center_y;
	cmr_u16 imblance_radial_1D_radius_thr;
	cmr_u16 imblance_radial_1D_protect_ratio_max;
	cmr_s16 imblance_radial_1D_coef_r0;
	cmr_s16 imblance_radial_1D_coef_r1;
	cmr_s16 imblance_radial_1D_coef_r2;
	cmr_s16 imblance_radial_1D_coef_r3;
	cmr_s16 imblance_radial_1D_coef_r4;
};

struct sensor_nlm_imbalance_level {
	struct sensor_imblance_face_rgb imblance_face_rbg;
	struct sensor_nlm_imblance_lum_flag lum_flag[3];
	struct sensor_imblance_flag3 nlm_imblance_flag3;
	struct sensor_imblance_hv imblance_hv[3];
	struct sensor_imblance_1D imblance_1D;
	struct sensor_imblance_curve imblance_curve;
	cmr_u16 imblance_lumth[2];
	cmr_u16 imblance_S_baohedu[3][2];
	cmr_u16 imblance_diff[3];
	cmr_u8 imblance_sat_lumth;
	cmr_u8 reversed[3];//for 4-byte alignment
	cmr_u16 imblance_bypass;
};

// CFAI, css color saturation suppression
struct sensor_cfai_css_thr {
	cmr_u16 css_edge_thr;
	cmr_u16 css_weak_egde_thr;
	cmr_u16 css_txt1_thr;
	cmr_u16 css_txt2_thr;
	cmr_u16 uv_val_thr;
	cmr_u16 uv_diff_thr;
	cmr_u16 gray_thr;
	cmr_u16 pix_similar_thr;
};

struct sensor_cfai_css_skin_bundary {
	cmr_u16 skin_top;
	cmr_u16 skin_down;
};
struct sensor_cfai_css_skin {
	struct sensor_cfai_css_skin_bundary skin_u1;
	struct sensor_cfai_css_skin_bundary skin_v1;
	struct sensor_cfai_css_skin_bundary skin_u2;
	struct sensor_cfai_css_skin_bundary skin_v2;
};

struct sensor_cfai_css_green {
	cmr_u16 green_edge_thr;
	cmr_u16 green_weak_edge_thr;
	cmr_u16 green_txt1_thr;
	cmr_u16 green_txt2_thr;
	cmr_u32 green_flat_thr;
};

struct sensor_cfai_css_ratio {
	cmr_u16 edge_ratio;
	cmr_u16 weak_edge_ratio;
	cmr_u16 txt1_ratio;
	cmr_u16 txt2_ratio;
	cmr_u32 flat_ratio;
};

struct sensor_cfai_css_level {
	struct sensor_cfai_css_thr css_comm_thr;
	struct sensor_cfai_css_skin css_skin_uv;
	struct sensor_cfai_css_green css_green_thr;
	struct sensor_cfai_css_ratio r_ratio;
	struct sensor_cfai_css_ratio b_ratio;
	cmr_u16 css_alpha_for_txt2;
	cmr_u16 css_bypass;
};

//CFAI, CFA interpolation
struct sensor_cfai_gref_thr {
	cmr_u16 round_diff_03_thr;
	cmr_u16 lowlux_03_thr;
	cmr_u16 round_diff_12_thr;
	cmr_u16 lowlux_12_thr;
};

struct sensor_cfai_wgtctrl {
	cmr_u8 wgtctrl_bypass;
	cmr_u8 grid_dir_wgt1;
	cmr_u8 grid_dir_wgt2;
	cmr_u8 reversed;
};

struct sensor_cfai_level {
	cmr_u32 min_grid_new;
	cmr_u8 grid_gain_new;
	cmr_u8 str_edge_tr;
	cmr_u8 cdcr_adj_factor;
	cmr_u8 reserved[3];
	cmr_u16 grid_tr;
	cmr_u32 smooth_tr;
	cmr_u16 uni_dir_intplt_tr;	//cfa_uni_dir_intplt_tr_new
	cmr_u16 rb_high_sat_thr;
	struct sensor_cfai_gref_thr gref_thr;
	struct sensor_cfai_wgtctrl cfai_dir_intplt;
	cmr_u32 cfa_bypass;	//cfa interpolatin control, always open!!!
};

struct sensor_cfa_param_level {
	struct sensor_cfai_level cfai_intplt_level;
	struct sensor_cfai_css_level cfai_css_level;
};

//AF monitor in rgb domain, remove YAFM in YIQ domain
struct sensor_rgb_afm_iir_denoise {
	cmr_u8 afm_iir_en;
	cmr_u8 reserved[3];
	cmr_u16 iir_g[2];
	cmr_u16 iir_c[10];
};

struct sensor_rgb_afm_enhanced_fv {
	cmr_u8 clip_en;
	cmr_u8 fv_shift;
	cmr_u8 reserved[2];
	cmr_u32 fv_min_th;
	cmr_u32 fv_max_th;
};

struct sensor_rgb_afm_enhanced_pre {
	cmr_u8 channel_sel;	//R/G/B for calc
	cmr_u8 denoise_mode;
	cmr_u8 center_wgt;	//2^n
	cmr_u8 reserved;
};

struct sensor_rgb_afm_enhanced_process {
	cmr_u8 fv1_coeff[4][9];	//4x3x3, fv0 is fixed in the code
	cmr_u8 lum_stat_chn_sel;
	cmr_u8 reserved[3];
};

struct sensor_rgb_afm_enhanced_post {
	struct sensor_rgb_afm_enhanced_fv enhanced_fv0;
	struct sensor_rgb_afm_enhanced_fv enhanced_fv1;
};

struct sensor_rgb_afm_enhanced {
	struct sensor_rgb_afm_enhanced_pre afm_enhanced_pre;
	struct sensor_rgb_afm_enhanced_process afm_enhanced_process;
	struct sensor_rgb_afm_enhanced_post afm_enhanced_post;
};

struct sensor_rgb_afm_level {
	struct sensor_rgb_afm_iir_denoise afm_iir_denoise;
	struct sensor_rgb_afm_enhanced afm_enhanced;
};

// CMC10
struct sensor_cmc10_param {
	struct isp_sample_point_info cur_idx;
	cmr_u16 matrix[SENSOR_CMC_NUM][9];
};

struct sensor_gamma_curve {
	struct isp_point points[SENSOR_GAMMA_POINT_NUM];
};

struct sensor_rgbgamma_curve {
	struct isp_point points_r[SENSOR_GAMMA_POINT_NUM];
	struct isp_point points_g[SENSOR_GAMMA_POINT_NUM];
	struct isp_point points_b[SENSOR_GAMMA_POINT_NUM];
};

struct sensor_frgb_gammac_param {
	struct isp_sample_point_info cur_idx_info;
	struct sensor_rgbgamma_curve curve_tab[SENSOR_GAMMA_NUM];
};

// CCE, color conversion enhance
struct sensor_cce_matrix_info {
	cmr_u16 matrix[9];
	cmr_u16 y_shift;
	cmr_u16 u_shift;
	cmr_u16 v_shift;
};

struct sensor_cce_param {
	cmr_u32 cur_idx;
	struct sensor_cce_matrix_info tab[SENSOR_CCE_NUM];
	struct sensor_cce_matrix_info specialeffect[MAX_SPECIALEFFECT_NUM];
};

//HSV domain
struct sensor_hsv_cfg {
	cmr_u16 hsv_s_curve[4];
	cmr_u16 hsv_v_curve[4];
	uint16_t hsv_r_s [2];
	uint16_t hsv_r_v [2];
	cmr_u16 hsv_hrange_left;
	cmr_u16 hsv_hrange_right;
};

struct sensor_hsv_param {
	struct sensor_hsv_cfg sensor_hsv_cfg[5];
	struct isp_sample_point_info cur_idx;
	struct isp_data_bin_info map[SENSOR_HSV_NUM];
	void *data_area;
	struct isp_data_bin_info specialeffect[MAX_SPECIALEFFECT_NUM];
	void *specialeffect_data_area;
};

struct sensor_hsv_table {
	cmr_s16 hue_table[360];
	cmr_u16 sat_table[360];
};

struct sensor_hsv_new2_param {
	struct sensor_hsv_cfg sensor_hsv_cfg[5];
	struct isp_sample_point_info cur_idx;
	struct sensor_hsv_table hsv_table[SENSOR_HSV_NUM];
	struct isp_data_bin_info specialeffect[MAX_SPECIALEFFECT_NUM];
	void *specialeffect_data_area;
};

//Posterize, is Special Effect
struct sensor_posterize_param {
	cmr_u32 cur_idx;
	cmr_u8 bin_mode;
	cmr_u8 pstrz_r_data[129];
	cmr_u8 pstrz_g_data[129];
	cmr_u8 pstrz_b_data[129];
	cmr_u8 specialeffect_r_data[MAX_SPECIALEFFECT_NUM][129];
	cmr_u8 specialeffect_g_data[MAX_SPECIALEFFECT_NUM][129];
	cmr_u8 specialeffect_b_data[MAX_SPECIALEFFECT_NUM][129];
};

//Gamma Correction in YUV domain
struct sensor_y_gamma_param {
	cmr_u32 cur_idx;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
	struct sensor_gamma_curve specialeffect[MAX_SPECIALEFFECT_NUM];
};

//Anti-flicker, old and new
struct sensor_y_afl_param_v1 {
	cmr_u8 skip_num;
	cmr_u8 line_step;
	cmr_u8 frm_num;
	cmr_u8 reserved0;
	cmr_u16 v_height;
	cmr_u16 reserved1;
	cmr_u16 col_st;		//x st
	cmr_u16 col_ed;		//x end
};

struct sensor_y_afl_param_v3 {
	cmr_u16 skip_num;
	cmr_u16 frm_num;
	cmr_u16 afl_x_st;
	cmr_u16 afl_x_ed;
	cmr_u16 afl_x_st_region;
	cmr_u16 afl_x_ed_region;
	struct isp_pos afl_step;
	struct isp_pos afl_step_region;
};

// AF monitor in YUV domain
struct sensor_y_afm_level {
	cmr_u32 iir_bypass;
	cmr_u8 skip_num;
	cmr_u8 afm_format;	//filter choose control
	cmr_u8 afm_position_sel;	//choose afm after CFA or auto contrust adjust
	cmr_u8 shift;
	cmr_u16 win[25][4];
	cmr_u16 coef[11];	//int16
	cmr_u16 reserved1;
};

//pre-color denoise in YUV domain
struct sensor_yuv_precdn_comm {
	cmr_u8 precdn_mode;
	cmr_u8 median_writeback_en;
	cmr_u8 median_mode;
	cmr_u8 den_stren;
	cmr_u8 uv_joint;
	cmr_u8 uv_thr;
	cmr_u8 y_thr;
	cmr_u8 reserved;
	cmr_u8 median_thr_u[2];
	cmr_u8 median_thr_v[2];
	cmr_u32 median_thr;
};

struct sensor_yuv_precdn_level {
	struct sensor_yuv_precdn_comm precdn_comm;
	float sigma_y;
	float sigma_d;
	float sigma_u;
	float sigma_v;
	cmr_u8 r_segu[2][7];	// param1
	cmr_u8 r_segv[2][7];	// param2
	cmr_u8 r_segy[2][7];	// param3
	cmr_u8 dist_w[25];	// param4
	cmr_u8 reserved0;
	cmr_u32 bypass;
};

//Brightness
struct sensor_bright_param {
	cmr_s8 factor[16];
	cmr_u32 cur_index;
	cmr_s8 scenemode[MAX_SCENEMODE_NUM];
	cmr_u32 bypass;
};

//Contrast
struct sensor_contrast_param {
	cmr_u8 factor[16];
	cmr_u32 cur_index;
	cmr_u8 scenemode[MAX_SCENEMODE_NUM];
	cmr_u32 bypass;
};

//Hue Adjustment
struct sensor_hue_param {
	cmr_u16 hue_sin[16];
	cmr_u16 hue_cos[16];
	cmr_u32 cur_index;
	cmr_u32 bypass;
};

//Color Saturation Adjustment
struct sensor_saturation_param {
	cmr_u8 csa_factor_u[16];
	cmr_u8 csa_factor_v[16];
	cmr_u32 index_u;
	cmr_u32 index_v;
	cmr_u8 scenemode[2][MAX_SCENEMODE_NUM];
	cmr_u32 bypass;
};

//sensor BCHS level
struct sensor_bchs_level{
	struct sensor_bright_param brightness;
	struct sensor_contrast_param contrast;
	struct sensor_hue_param hue;
	struct sensor_saturation_param saturation;
};

//Hist
struct sensor_hists_param {
	cmr_u16 hist_skip_num;
	cmr_u16 hist_mode_sel;
};

//Hist 2
struct sensor_hists2_param {
	cmr_u16 hist2_skip_num;
	cmr_u16 hist2_mode_sel;
	cmr_u32 hist2_mul_mode;
	struct isp_pos_rect hist2_roi;
};

//CDN, Color Denoise
struct sensor_uv_cdn_level {
	float sigma_u;
	float sigma_v;
	cmr_u8 median_thru0;
	cmr_u8 median_thru1;
	cmr_u8 median_thrv0;
	cmr_u8 median_thrv1;
	cmr_u8 u_ranweight[31];
	cmr_u8 v_ranweight[31];
	cmr_u8 cdn_gaussian_mode;
	cmr_u8 median_mode;
	cmr_u8 median_writeback_en;
	cmr_u8 filter_bypass;
	cmr_u16 median_thr;
	cmr_u32 bypass;
};

//Edge Enhancement
struct sensor_ee_pn {
	cmr_s16 negative;
	cmr_s16 positive;
};

struct sensor_ee_ratio {
	cmr_u8 ratio_hv_3;
	cmr_u8 ratio_hv_5;
	cmr_u8 ratio_dg_3;
	cmr_u8 ratio_dg_5;
};

struct sensor_ee_t_cfg {
	cmr_u16 ee_t1_cfg;
	cmr_u16 ee_t2_cfg;
	cmr_u16 ee_t3_cfg;
	cmr_u16 ee_t4_cfg;
};

struct sensor_ee_r_cfg {
	cmr_u8 ee_r1_cfg;
	cmr_u8 ee_r2_cfg;
	cmr_u8 ee_r3_cfg;
	cmr_u8 reserved;
};

struct sensor_ee_c_cfg {
	cmr_s8 ee_c1_cfg;
	cmr_s8 ee_c2_cfg;
	cmr_s8 ee_c3_cfg;
	cmr_s8 reserved;
};

struct sensor_ee_polyline_cfg {
	struct sensor_ee_t_cfg t_cfg;
	struct sensor_ee_r_cfg r_cfg;
};

//sensor_ee_corner:
struct sensor_ee_corner {
	cmr_u8 ee_corner_cor;
	cmr_u8 reserved0[3];
	struct sensor_ee_pn ee_corner_th;
	struct sensor_ee_pn ee_corner_gain;
	struct sensor_ee_pn ee_corner_sm;
};

//sensor_ee_ipd
struct sensor_ee_ipd_smooth {
	cmr_u32 smooth_en;
	struct sensor_ee_pn smooth_mode;
	struct sensor_ee_pn smooth_ee_diff;
	struct sensor_ee_pn smooth_ee_thr;
};

struct sensor_ee_ipd {
	cmr_u16 ipd_bypass;
	cmr_u16 ipd_mask_mode;
	struct sensor_ee_pn ipd_flat_thr;
	struct sensor_ee_pn ipd_eq_thr;
	struct sensor_ee_pn ipd_more_thr;
	struct sensor_ee_pn ipd_less_thr;
	struct sensor_ee_ipd_smooth ipd_smooth;
};

//sensor_ee_polyline:
struct sensor_ee_gradient {
	cmr_u16 grd_cmpt_type;
	cmr_u16 wgt_hv2diag;
	cmr_u32 wgt_diag2hv;
	struct sensor_ee_ratio ratio;
	struct sensor_ee_polyline_cfg ee_gain_hv1;
	struct sensor_ee_polyline_cfg ee_gain_hv2;
	struct sensor_ee_polyline_cfg ee_gain_dg1;
	struct sensor_ee_polyline_cfg ee_gain_dg2;
};

struct sensor_ee_clip {
	struct sensor_ee_polyline_cfg ee_pos;
	struct sensor_ee_c_cfg ee_pos_c;
	struct sensor_ee_polyline_cfg ee_neg;
	struct sensor_ee_c_cfg ee_neg_c;
};
struct sensor_ee_offset_list {
	cmr_u16 thr_layer_cv_pos[4];
	cmr_u16 ratio_layer_cv_pos[3];
	cmr_u16 clip_layer_cv_pos[3];
	cmr_u16 thr_layer_cv_neg[4];
	cmr_u16 ratio_layer_cv_neg[3];
	cmr_s16 clip_layer_cv_neg[3];
	cmr_u16 ratio_layer_lum_cv[3];
	cmr_u16 ratio_layer_freq_cv[3];
};
struct sensor_ee_offset_layer {
	cmr_u16 ee_old_gradient_en;
	cmr_u16 ee_new_pyramid_en;
	cmr_u16 ee_ratio_old_gradient;
	cmr_u16 ee_ratio_new_pyramid;
	struct sensor_ee_offset_list ee_offset_layer[3];
};

//sensor_ee_param:
struct sensor_ee_level {
	struct sensor_ee_pn str_d_p;
	struct sensor_ee_pn ee_thr_d;
	struct sensor_ee_pn ee_incr_d;
	struct sensor_ee_pn ee_cv_clip;
	struct sensor_ee_polyline_cfg ee_cv;
	struct sensor_ee_polyline_cfg ee_lum;
	struct sensor_ee_polyline_cfg ee_freq;
	struct sensor_ee_clip ee_clip;
	struct sensor_ee_corner ee_corner;
	struct sensor_ee_ipd ee_ipd;
	struct sensor_ee_gradient ee_gradient;
	struct sensor_ee_offset_layer ee_offset_layer;
	cmr_u8 ee_mode;
	cmr_u8 flat_smooth_mode;
	cmr_u8 edge_smooth_mode;
	cmr_u8 bypass;
};

struct raw_gtm_stat_rgb2y{
	cmr_u8 rgb2y_mode;
	cmr_u8 reserved;
	cmr_u16 rgb2y_r_coeff;
	cmr_u16 rgb2y_g_coeff;
	cmr_u16 rgb2y_b_coeff;
};

struct raw_gtm_stat_percentile_16bit{
	cmr_u16 min_percentile_16bit;
	cmr_u16 max_percentile_16bit;
};

struct raw_gtm_stat_target_norm{
	cmr_u16 target_norm_set_mode;
	cmr_u16 target_norm;
	float target_norm_coeff;
};

struct raw_gtm_stat_image{
	cmr_u8 image_key_set_mode;
	cmr_u8 reserved;
	cmr_u16 image_key;
};

struct raw_gtm_stat_video{
	cmr_u16 luma_sm_Ymax_diff_thr;
	cmr_u16 luma_sm_Yavg_diff_thr;
	cmr_u16 cur_Ymin_weight;
	cmr_u16 pre_Ymin_weight;
};

// raw gtm stat
struct sensor_raw_gtm_stat{
	cmr_u8 gtm_stat_bypass;
	cmr_u8 reserved[3];
	struct raw_gtm_stat_rgb2y gtm_stat_rgb2y;
	struct raw_gtm_stat_percentile_16bit percentile_16bit;
	struct raw_gtm_stat_target_norm target_norm;
	struct raw_gtm_stat_image gtm_stat_image;
	struct raw_gtm_stat_video gtm_stat_video;
};

//ltm map
struct sensor_stat_map{
	cmr_u16 map_video_mode;
	cmr_u16 bypass;
};

//raw gtm level
struct sensor_raw_gtm_level{
	struct sensor_raw_gtm_stat raw_gtm_stat;
	struct sensor_stat_map raw_gtm_map;
};

//sensor_ltm_stat
struct sensor_ltm_tile_num_minus1{
	cmr_u16 tile_num_x;
	cmr_u16 tile_num_y;
};

struct sensor_ltm_tile_size{
	cmr_u16 tile_width;
	cmr_u16 tile_height;
};

struct sensor_ltm_clip_limit{
	cmr_u16 limt;
	cmr_u16 limt_min;
};

struct sensor_ltm_stat{
	struct sensor_ltm_tile_num_minus1 tile_num;
	struct sensor_ltm_tile_size tile_size;
	struct sensor_ltm_clip_limit clip_limit;
	cmr_u8 strength;
	cmr_u8 tile_num_auto;
	cmr_u8 text_point_thres;
	cmr_u8 textture_proporion;
	cmr_u8 region_est_en;
	cmr_u8 binning_en;
	cmr_u16 bypass;
};

//sensor_ltm_map
struct sensor_ltm_map{
	cmr_u16 ltm_map_video_mode;
	cmr_u16 bypass;
};

struct sensor_ltm_level{
	struct sensor_ltm_stat ltm_stat;
	struct sensor_ltm_map ltm_map;
};

struct sensor_ltm_text{
	float text_point_alpha;
	cmr_u8 text_point_thres;
	cmr_u8 textture_proporion;
	cmr_u8 reserved[2];
};

//ltm stat
struct sensor_ltm_stat_l5pro{
	struct sensor_ltm_tile_num_minus1 tile_num;
	struct sensor_ltm_text ltm_text;
	cmr_u8 strength;
	cmr_u8 region_est_en;
	cmr_u8 tile_num_auto;
	cmr_u8 bypass;
};

//yuv ltm level
struct sensor_yuv_ltm_level{
	struct sensor_ltm_stat_l5pro yuv_ltm_stat;
	struct sensor_ltm_map yuv_ltm_map;
};

//rgb ltm level
struct sensor_rgb_ltm_level{
	struct sensor_ltm_stat_l5pro rgb_ltm_stat;
	struct sensor_ltm_map rgb_ltm_map;
};

//4A, smart, AFT
struct isp_pdaf_tune_param {
	cmr_u32 min_pd_vcm_steps;
	cmr_u32 max_pd_vcm_steps;
	cmr_u32 coc_range;
	cmr_u32 far_tolerance;
	cmr_u32 near_tolerance;
	cmr_u32 err_limit;
	cmr_u32 pd_converge_thr;
	cmr_u32 pd_converge_thr_2nd;
	cmr_u32 pd_focus_times_thr;
	cmr_u32 pd_thread_sync_frm;
	cmr_u32 pd_thread_sync_frm_init;
	cmr_u32 min_process_frm;
	cmr_u32 max_process_frm;
	cmr_u32 pd_conf_thr;
	cmr_u32 pd_conf_thr_2nd;
};

struct isp_haf_tune_param {
	//default param for outdoor/indoor/dark
	struct isp_pdaf_tune_param isp_pdaf_tune_data[3];
};

struct sensor_filter_weights
{
	cmr_u8 distWeight[9]; //distance weight for different scale
	cmr_u8 reserved[3]; //alignment 4 byte with distWeight
	cmr_u8 rangWeight[128]; //range weight for different scale
};

struct sensor_cnr_level {
	cmr_u8 level_enable;
	cmr_u16 low_ct_thrd;
	cmr_u8 reserved; //for 4 bytes allign
	cmr_u8 filter_en[CNR_LEVEL]; //enable control of filter
	cmr_u8 rangTh[CNR_LEVEL][2]; //threshold for different scale(rangTh[CNR_LEVEL][0]:U threshold, rangTh[CNR_LEVEL][1]:V threshold)
	struct sensor_filter_weights weight[CNR_LEVEL][2]; //weight table(wTable[CNR_LEVEL][0]:U weight table, wTable[CNR_LEVEL][1]:V weight table)
	float dist_sigma[CNR_LEVEL][2];
	float rang_sigma[CNR_LEVEL][2];
};

struct sensor_ae_adapt_param {
	cmr_u16 binning_factor; // 1x = 128
	cmr_u16 reserved[19];
};

//aft
typedef struct isp_aft_param {
	cmr_u32 img_blk_support;
	cmr_u32 hist_support;
	cmr_u32 afm_support;
	cmr_u32 caf_blk_support;
	cmr_u32 gyro_support;
	cmr_u32 gyro_debug_support;
	cmr_u32 gsensor_support;
	cmr_u32 need_rough_support;
	cmr_u32 abort_af_support;
	cmr_u32 need_print_trigger_support;
	cmr_u32 ae_select_support;
	cmr_u32 ae_calibration_support;
	cmr_u32 dump_support;
	cmr_u32 afm_abort_support;
	cmr_u32 ae_skip_line;
	cmr_u32 img_blk_value_thr;
	cmr_u32 img_blk_num_thr;
	cmr_u32 img_blk_cnt_thr;
	cmr_u32 img_blk_value_stab_thr;
	cmr_u32 img_blk_num_stab_thr;
	cmr_u32 img_blk_cnt_stab_thr;
	cmr_u32 img_blk_cnt_stab_max_thr;
	cmr_u32 img_blk_skip_cnt;
	cmr_u32 img_blk_value_video_thr;
	cmr_u32 img_blk_num_video_thr;
	cmr_u32 img_blk_cnt_video_thr;
	cmr_u32 img_blk_value_video_stab_thr;
	cmr_u32 img_blk_num_video_stab_thr;
	cmr_u32 img_blk_cnt_video_stab_thr;
	cmr_u32 img_blk_value_abort_thr;
	cmr_u32 img_blk_num_abort_thr;
	cmr_u32 img_blk_cnt_abort_thr;
	cmr_u32 ae_mean_sat_thr[3];	//saturation
	cmr_u8 afm_roi_left_ration;
	cmr_u8 afm_roi_top_ration;
	cmr_u8 afm_roi_width_ration;
	cmr_u8 afm_roi_heigth_ration;
	cmr_u8 afm_num_blk_hor;
	cmr_u8 afm_num_blk_vec;
	cmr_u32 afm_value_thr;
	cmr_u32 afm_num_thr;
	cmr_u32 afm_value_stab_thr;
	cmr_u32 afm_num_stab_thr;
	cmr_u32 afm_cnt_stab_thr;
	cmr_u32 afm_cnt_move_thr;
	cmr_u32 afm_value_video_thr;
	cmr_u32 afm_num_video_thr;
	cmr_u32 afm_cnt_video_move_thr;
	cmr_u32 afm_value_video_stab_thr;
	cmr_u32 afm_num_video_stab_thr;
	cmr_u32 afm_cnt_video_stab_thr;
	cmr_u32 afm_skip_cnt;
	cmr_u32 afm_need_af_cnt_thr[3];
	cmr_u32 afm_fv_diff_thr[3];
	cmr_u32 afm_fv_queue_cnt[3];
	cmr_u32 afm_fv_stab_diff_thr[3];
	cmr_u32 afm_need_af_cnt_video_thr[3];
	cmr_u32 afm_fv_video_diff_thr[3];
	cmr_u32 afm_fv_video_queue_cnt[3];
	cmr_u32 afm_fv_video_stab_diff_thr[3];
	cmr_u32 afm_need_af_cnt_abort_thr[3];
	cmr_u32 afm_fv_diff_abort_thr[3];
	cmr_u32 afm_need_rough_thr;
	cmr_u32 caf_work_lum_thr;	//percent of ae target
	float gyro_move_thr;
	cmr_u32 gyro_move_cnt_thr;
	float gyro_stab_thr;
	cmr_u32 gyro_stab_cnt_thr;
	float gyro_video_move_thr;
	cmr_u32 gyro_video_move_cnt_thr;
	float gyro_video_stab_thr;
	cmr_u32 gyro_video_stab_cnt_thr;
	float gyro_move_abort_thr;
	cmr_u32 gyro_move_cnt_abort_thr;
	cmr_u32 sensor_queue_cnt;
	cmr_u32 gyro_move_percent_thr;
	cmr_u32 gyro_stab_percent_thr;
	float gsensor_stab_thr;
	float hist_stab_thr;
	float hist_stab_diff_thr;
	float hist_base_stab_thr;
	float hist_need_rough_cc_thr;
	cmr_u32 hist_max_frame_queue;
	cmr_u32 hist_stab_cnt_thr;
	cmr_u32 vcm_pos_abort_thr;
	cmr_u32 abort_af_y_thr;
	cmr_u32 reserved[16];
} isp_aft_param_t;

typedef struct aft_tuning_param {
	cmr_u32 version;
	struct isp_aft_param aft_tuning_param;
} aft_tuning_param_t;

struct isp_alsc_param {
	// add for buid warning, isp owners shoud complete the struct
	cmr_s32 dummy;
};

//smart param begin
#define ISP_SMART_MAX_BV_SECTION 8
#define ISP_SMART_MAX_BLOCK_NUM 32	//28
#define ISP_SMART_MAX_VALUE_NUM 4

enum isp_smart_x_type {
	ISP_SMART_X_TYPE_BV = 0,
	ISP_SMART_X_TYPE_BV_GAIN = 1,
	ISP_SMART_X_TYPE_CT = 2,
	ISP_SMART_X_TYPE_BV_CT = 3,
};

enum isp_smart_y_type {
	ISP_SMART_Y_TYPE_VALUE = 0,
	ISP_SMART_Y_TYPE_WEIGHT_VALUE = 1,
};

struct isp_smart_component_cfg {
	cmr_u32 id;
	cmr_u32 type;
	cmr_u32 offset;
	cmr_u32 size;

	cmr_u32 x_type;		// isp_smart_x_type
	cmr_u32 y_type;		// isp_smart_y_type
	cmr_s32 default_val;
	cmr_s32 use_flash_val;
	cmr_s32 flash_val;	//use this value when flash is open

	cmr_u32 section_num;
	struct isp_range bv_range[ISP_SMART_MAX_BV_SECTION];
	struct isp_piecewise_func func[ISP_SMART_MAX_BV_SECTION];
};

struct isp_smart_block_cfg {
	cmr_u32 enable;
	cmr_u32 smart_id;	//id to identify the smart block
	cmr_u32 block_id;	//id to identify the isp block (destination block)
	cmr_u32 component_num;
	struct isp_smart_component_cfg component[ISP_SMART_MAX_VALUE_NUM];
};

/*tuning param for smart block*/
struct isp_smart_param {
	cmr_u32 block_num;
	struct isp_smart_block_cfg block[ISP_SMART_MAX_BLOCK_NUM];
};
/*smart param end*/

//post-color denoise
struct sensor_postcdn_thr {
	cmr_u16 thr0;		//algorithm reserved
	cmr_u16 thr1;		//algorithm reserved
};

struct sensor_postcdn_r_seg {
	cmr_u8 r_seg0[7];
	cmr_u8 r_seg1[7];
	cmr_u8 reserved0[2];
};

struct sensor_postcdn_distw {
	cmr_u8 distw[15][5];
	cmr_u8 reserved0;
};

struct sensor_uv_postcdn_level {
	struct sensor_postcdn_r_seg r_segu;
	struct sensor_postcdn_r_seg r_segv;
	struct sensor_postcdn_distw r_distw;
	struct sensor_postcdn_thr uv_thr;
	struct sensor_postcdn_thr u_thr;
	struct sensor_postcdn_thr v_thr;
	float sigma_d;
	float sigma_u;
	float sigma_v;
	cmr_u16 adpt_med_thr;
	cmr_u8 median_mode;
	cmr_u8 uv_joint;
	cmr_u8 median_res_wb_en;
	cmr_u8 postcdn_mode;
	cmr_u16 downsample_bypass;
	cmr_u32 bypass;
};

//Y_DELAY, should define in other
struct sensor_y_delay_param {
	//cmr_u16 bypass;//   can't bypass!!!
	cmr_u16 ydelay_step;
};

//IIR color noise reduction, should named CCNR in tuning tool
struct sensor_iircnr_pre {
	cmr_u16 iircnr_pre_uv_th;
	cmr_u16 iircnr_sat_ratio;
};

struct sensor_iircnr_ee_judge {
	cmr_u16 y_edge_thr_max[8];
	cmr_u16 iircnr_uv_s_th;
	cmr_u16 iircnr_uv_th;
	cmr_u32 iircnr_uv_dist;
	cmr_u16 uv_low_thr1[8];
};

struct sensor_iircnr_uv_thr {
	cmr_u32 uv_low_thr2;
	cmr_u32 uv_high_thr2;
};

struct sensor_iircnr_str {
	cmr_u8 iircnr_y_max_th;
	cmr_u8 iircnr_y_th;
	cmr_u8 iircnr_y_min_th;
	cmr_u8 iircnr_uv_diff_thr;
	cmr_u16 y_edge_thr_min[8];
	cmr_u16 iircnr_alpha_hl_diff_u;
	cmr_u16 iircnr_alpha_hl_diff_v;
	cmr_u32 iircnr_alpha_low_u;
	cmr_u32 iircnr_alpha_low_v;
	struct sensor_iircnr_uv_thr cnr_uv_thr2[8];
};

struct sensor_iircnr_post {
	cmr_u32 iircnr_css_lum_thr;
};

struct sensor_iircnr_level {
	cmr_u16 cnr_iir_mode;
	cmr_u16 cnr_uv_pg_th;
	cmr_u32 ymd_u;
	cmr_u32 ymd_v;
	cmr_u32 ymd_min_u;
	cmr_u32 ymd_min_v;
	cmr_u16 slop_uv[8];
	cmr_u16 slop_y[8];
	cmr_u32 middle_factor_uv[8];
	cmr_u32 middle_factor_y[8];
	struct sensor_iircnr_pre pre_uv_th;
	struct sensor_iircnr_ee_judge iircnr_ee;
	struct sensor_iircnr_str iircnr_str;
	struct sensor_iircnr_post css_lum_thr;
	cmr_u32 bypass;
};

//IIR yrandom
struct sensor_iircnr_yrandom_level {
	cmr_u8 yrandom_shift;
	cmr_u8 reserved0[3];
	cmr_u32 yrandom_seed;
	cmr_u16 yrandom_offset;
	cmr_u16 reserved1;
	cmr_u8 yrandom_takebit[8];
	cmr_u32 bypass;
};
struct sensor_iircnr_yrandom_param {
	struct sensor_iircnr_yrandom_level iircnr_yrandom_level;
};

//alsc
struct alsc_alg0_turn_param {
	float pa;		//threshold for seg
	float pb;
	cmr_u32 fft_core_id;	//fft param ID
	cmr_u32 con_weight;	//convergence rate
	cmr_u32 bv;
	cmr_u32 ct;
	cmr_u32 pre_ct;
	cmr_u32 pre_bv;
	cmr_u32 restore_open;
	cmr_u32 freq;
	float threshold_grad;
};

/* sw 3DNR tuning param */
struct sensor_sw3dnr_level {
	cmr_s32 threshold[4];
	cmr_s32 slope[4];
	cmr_u16 searchWindow_x;
	cmr_u16 searchWindow_y;
	cmr_s32 recur_str;
	cmr_s32 match_ratio_sad;
	cmr_s32 match_ratio_pro;
	cmr_s32 feat_thr;
	cmr_s32 zone_size;
	cmr_s32 luma_ratio_high;
	cmr_s32 luma_ratio_low;
	cmr_s32 reserverd[16];
};

enum {
	ISP_BLK_NLM_T = 0x00,
	ISP_BLK_VST_T,
	ISP_BLK_IVST_T,
	ISP_BLK_RGB_DITHER_T,
	ISP_BLK_BPC_T,
	ISP_BLK_GRGB_T,
	ISP_BLK_CFA_T,
	ISP_BLK_RGB_AFM_T,
	ISP_BLK_UVDIV_T,
	ISP_BLK_3DNR_T,
	ISP_BLK_PPE_T,
	ISP_BLK_YUV_PRECDN_T,
	ISP_BLK_CDN_T,
	ISP_BLK_POSTCDN_T,
	ISP_BLK_YNR_T,
	ISP_BLK_EDGE_T,
	ISP_BLK_IIRCNR_T,
	ISP_BLK_YUV_NOISEFILTER_T,
	ISP_BLK_CNR2_T,
	ISP_BLK_IMBALANCEE_T,
	ISP_BLK_LTM_T,
	ISP_BLK_SW3DNR_T,
	ISP_BLK_BWU_BWD_T,
	ISP_BLK_RAW_GTM_T,
	ISP_BLK_RGB_LTM_T,
	ISP_BLK_YUV_LTM_T,
	ISP_BLK_NR_MAX
};

enum {
	ISP_MODE_ID_COMMON = 0x00,
	ISP_MODE_ID_PRV_0,
	ISP_MODE_ID_PRV_1,
	ISP_MODE_ID_PRV_2,
	ISP_MODE_ID_PRV_3,
	ISP_MODE_ID_CAP_0,
	ISP_MODE_ID_CAP_1,
	ISP_MODE_ID_CAP_2,
	ISP_MODE_ID_CAP_3,
	ISP_MODE_ID_VIDEO_0,
	ISP_MODE_ID_VIDEO_1,
	ISP_MODE_ID_VIDEO_2,
	ISP_MODE_ID_VIDEO_3,
	ISP_MODE_ID_MAX = 0xff,
};

enum {
	ISP_PARAM_ID_SETTING = 0x00,
	ISP_PARAM_ID_CMD,
	ISP_PARAM_ID_AUTO_AE,
	ISP_PARAM_ID_AUTO_AWB,
	ISP_PARAM_ID_AUTO_AF,
	ISP_PARAM_ID_AUTO_SMART,
};

struct isp_block_param {
	struct isp_block_header block_header;

	cmr_u32 *data_ptr;
	cmr_u32 data_size;
};

union sensor_version_name {
	cmr_u32 sensor_name_in_word[8];
	cmr_s8 sensor_name[32];
};

struct sensor_version_info {
	cmr_u32 version_id;
	union sensor_version_name sensor_ver_name;
	cmr_u32 ae_struct_version;
	cmr_u32 awb_struct_version;
	cmr_u32 lnc_struct_version;
	cmr_u32 nr_struct_version;
	cmr_u32 reserve0;
	cmr_u32 reserve1;
	cmr_u32 reserve2;
	cmr_u32 reserve3;
	cmr_u32 reserve4;
	cmr_u32 reserve5;
	cmr_u32 reserve6;
};

struct sensor_ae_tab_param {
	cmr_u8 *ae;
	cmr_u32 ae_len;
};

struct sensor_awb_tab_param {
	cmr_u8 *awb;
	cmr_u32 awb_len;
};

struct sensor_lnc_tab_param {
	cmr_u8 *lnc;
	cmr_u32 lnc_len;
};

struct ae_exp_gain_tab {
	cmr_u32 *index;
	cmr_u32 index_len;
	cmr_u32 *exposure;
	cmr_u32 exposure_len;
	cmr_u32 *dummy;
	cmr_u32 dummy_len;
	cmr_u16 *again;
	cmr_u32 again_len;
	cmr_u16 *dgain;
	cmr_u32 dgain_len;
};

struct ae_scene_exp_gain_tab {
	cmr_u32 *scene_info;
	cmr_u32 scene_info_len;
	cmr_u32 *index;
	cmr_u32 index_len;
	cmr_u32 *exposure;
	cmr_u32 exposure_len;
	cmr_u32 *dummy;
	cmr_u32 dummy_len;
	cmr_u16 *again;
	cmr_u32 again_len;
	cmr_u16 *dgain;
	cmr_u32 dgain_len;
};

struct ae_weight_tab {
	cmr_u8 *weight_table;
	cmr_u32 len;
};

struct ae_auto_iso_tab_v1 {
	cmr_u16 *auto_iso_tab;
	cmr_u32 len;
};

struct sensor_ae_tab {
	struct sensor_ae_tab_param ae_param;
	struct ae_exp_gain_tab ae_tab[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_exp_gain_tab ae_flash_tab[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_weight_tab weight_tab[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_exp_gain_tab scene_tab[AE_SCENE_NUM][AE_FLICKER_NUM];
	struct ae_auto_iso_tab_v1 auto_iso_tab[AE_FLICKER_NUM];
};

struct sensor_lens_map_info {
	cmr_u32 envi;
	cmr_u32 ct;
	cmr_u32 width;
	cmr_u32 height;
	cmr_u32 grid;
};

struct sensor_lens_map {
	cmr_u32 *map_info;
	cmr_u32 map_info_len;
	cmr_u16 *weight_info;
	cmr_u32 weight_info_len;
	cmr_u32 *lnc_map_tab_len;
	cmr_u32 *lnc_map_tab_offset;
	cmr_u16 *lnc_addr;
	cmr_u32 lnc_len;
};

struct sensor_lsc_map {
	struct sensor_lnc_tab_param lnc_param;
	struct sensor_lens_map map[LNC_MAP_COUNT];
};

struct sensor_nr_scene_map_param {
	cmr_u32 nr_scene_map[MAX_MODE_NUM];
};

struct sensor_nr_level_map_param {
	cmr_u8 nr_level_map[MAX_NR_NUM];
};

struct sensor_nr_map_group_param {
	cmr_u32 *nr_scene_map_ptr;
	cmr_u32 nr_scene_map_len;
	cmr_u8 *nr_level_map_ptr;
	cmr_u32 nr_level_map_len;
};

struct nr_set_group_unit {
	cmr_u8 *nr_ptr;
	cmr_u32 nr_len;
};
struct sensor_nr_set_group_param {
	cmr_u8 *nlm;
	cmr_u32 nlm_len;
	cmr_u8 *vst;
	cmr_u32 vst_len;
	cmr_u8 *ivst;
	cmr_u32 ivst_len;
	cmr_u8 *rgb_dither;
	cmr_u32 rgb_dither_len;
	cmr_u8 *bpc;
	cmr_u32 bpc_len;
	cmr_u8 *grgb;
	cmr_u32 grgb_len;
	cmr_u8 *cfa;
	cmr_u32 cfa_len;
	cmr_u8 *rgb_afm;
	cmr_u32 rgb_afm_len;
	cmr_u8 *uvdiv;
	cmr_u32 uvdiv_len;
	cmr_u8 *nr3d;
	cmr_u32 nr3d_len;
	cmr_u8 *ppe;
	cmr_u32 ppe_len;
	cmr_u8 *yuv_precdn;
	cmr_u32 yuv_precdn_len;
	cmr_u8 *cdn;
	cmr_u32 cdn_len;
	cmr_u8 *postcdn;
	cmr_u32 postcdn_len;
	cmr_u8 *ynr;
	cmr_u32 ynr_len;
	cmr_u8 *edge;
	cmr_u32 edge_len;
	cmr_u8 *iircnr;
	cmr_u32 iircnr_len;
	cmr_u8 *yuv_noisefilter;
	cmr_u32 yuv_noisefilter_len;
	cmr_u8 *cnr2;
	cmr_u32 cnr2_len;
	cmr_u8 *imblance;
	cmr_u32 imblance_len;
	cmr_u8 *ltm;
	cmr_u32 ltm_len;
	cmr_u8 *sw_3dnr;
	cmr_u32 sw_3dnr_len;
	cmr_u8 *bwu_bwd;
	cmr_u32 bwu_bwd_len;
	cmr_u8 *raw_gtm;
	cmr_u32 raw_gtm_len;
	cmr_u8 *rgb_ltm;
	cmr_u32 rgb_ltm_len;
	cmr_u8 *yuv_ltm;
	cmr_u32 yuv_ltm_len;
};

struct sensor_nr_param {
	struct sensor_nr_set_group_param nr_set_group;
};

struct sensor_awb_map {
	cmr_u16 *addr;
	cmr_u32 len;		//by bytes
};

struct sensor_awb_weight {
	cmr_u8 *addr;
	cmr_u32 weight_len;
	cmr_u16 *size;
	cmr_u32 size_param_len;
};

struct sensor_awb_map_weight_param {
	struct sensor_awb_tab_param awb_param;
	struct sensor_awb_map awb_map;
	struct sensor_awb_weight awb_weight;
};
struct sensor_awb_table_param {
	cmr_u8 awb_pos_weight[AWB_POS_WEIGHT_LEN];
	cmr_u16 awb_pos_weight_width_height[AWB_POS_WEIGHT_WIDTH_HEIGHT / sizeof(cmr_u16)];
	cmr_u16 awb_map[];
};
struct sensor_raw_fix_info {
	struct sensor_ae_tab ae;
	struct sensor_lsc_map lnc;
	struct sensor_awb_map_weight_param awb;
	struct sensor_nr_param nr;
};

struct sensor_raw_note_info {
	cmr_u8 *note;
	cmr_u32 node_len;
};

struct sensor_nr_fix_info {
	struct sensor_nr_scene_map_param *nr_scene_ptr;
	struct sensor_nr_level_map_param *nr_level_number_ptr;
	struct sensor_nr_level_map_param *nr_default_level_ptr;
};

struct sensor_param_list_info {
	struct sensor_find_param_list  *list_ptr;
	cmr_u32 list_len;
};

struct sensor_raw_info {
	struct sensor_version_info *version_info;
	struct isp_mode_param_info mode_ptr[MAX_MODE_NUM];
	struct sensor_raw_resolution_info_tab *resolution_info_ptr;
	struct sensor_raw_ioctrl *ioctrl_ptr;
	struct sensor_libuse_info *libuse_info;
	struct sensor_raw_fix_info *fix_ptr[MAX_MODE_NUM];
	struct sensor_raw_note_info note_ptr[MAX_MODE_NUM];
	struct sensor_nr_fix_info nr_fix;
	struct sensor_param_list_info param_list_info;
};

struct denoise_param_update {
	struct sensor_bpc_level *bpc_level_ptr;
	struct sensor_grgb_level *grgb_level_ptr;
	struct sensor_nlm_level *nlm_level_ptr;
	struct sensor_vst_level *vst_level_ptr;
	struct sensor_ivst_level *ivst_level_ptr;
	struct sensor_cfai_level *cfae_level_ptr;
	struct sensor_yuv_precdn_level *yuv_precdn_level_ptr;
	struct sensor_uv_cdn_level *uv_cdn_level_ptr;
	struct sensor_ee_level *ee_level_ptr;
	struct sensor_uv_postcdn_level *uv_postcdn_level_ptr;
	struct sensor_iircnr_level *iircnr_level_ptr;
	struct sensor_cce_uvdiv_level *cce_uvdiv_level_ptr;
	struct sensor_3dnr_level *dnr_level_ptr;
	struct sensor_ppe_level *ppe_level_ptr;
	struct sensor_rgb_afm_level *rgb_afm_level_ptr;
	struct sensor_rgb_dither_level *rgb_dither_level_ptr;
	struct sensor_ynr_level *ynr_level_ptr;
	struct sensor_yuv_noisefilter_level *yuv_noisefilter_level_ptr;
	struct sensor_cnr_level *cnr2_level_ptr;
	struct sensor_nlm_imbalance_level *imbalance_level_ptr;
	struct sensor_ltm_level *ltm_level_ptr;
	struct sensor_sw3dnr_level *sw3dnr_level_ptr;
	struct sensor_bwu_bwd_level *bwu_bwd_level_ptr;
	struct sensor_raw_gtm_level *raw_gtm_level_ptr;
	struct sensor_rgb_ltm_level *rgb_ltm_level_ptr;
	struct sensor_yuv_ltm_level *yuv_ltm_level_ptr;
	struct sensor_nr_scene_map_param *nr_scene_map_ptr;
	struct sensor_nr_level_map_param *nr_level_number_map_ptr;
	struct sensor_nr_level_map_param *nr_default_level_map_ptr;
	cmr_u32 multi_nr_flag;
};

#endif
