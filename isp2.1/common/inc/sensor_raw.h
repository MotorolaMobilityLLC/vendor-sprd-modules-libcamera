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
#ifndef _SENSOR_RAW_H_
#define _SENSOR_RAW_H_

#ifndef WIN32
#include <sys/types.h>
#else
#include ".\isp_type.h"
#endif
#include "isp_type.h"
#include "cmr_sensor_info.h"
#include "../../ae/inc/ae_types.h"

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
#define AE_WEIGHT_TABLE_NUM 3
#define AE_SCENE_NUM 8
#define LNC_MAP_COUNT 9

#define AE_VERSION    0x00000000
#define AWB_VERSION   0x00000000
#define NR_VERSION    0x00000000
#define LNC_VERSION   0x00000000
#define PM_CODE_VERSION  0x00070005

#define TUNE_FILE_CHIP_VER_MASK 0x0000FFFF
#define TUNE_FILE_SW_VER_MASK 0xFFFF0000

#define TUNE_FILE_CHIP_VER_MASK 0x0000FFFF
#define TUNE_FILE_SW_VER_MASK 0xFFFF0000

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

#if 1
//testcode,should ask AE module to change struct define mothod
struct ae_exp_gain_index_2 {
	uint32_t min_index;
	uint32_t max_index;
};
struct ae_exp_gain_table_2 {
	struct ae_exp_gain_index_2 index;
	uint32_t exposure[AE_EXP_GAIN_TABLE_SIZE];
	uint32_t dummy[AE_EXP_GAIN_TABLE_SIZE];
	uint16_t again[AE_EXP_GAIN_TABLE_SIZE];
	uint16_t dgain[AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_scence_info_header_2 {
	uint32_t enable;
	uint32_t scene_mode;
	uint32_t target_lum;
	uint32_t iso_index;
	uint32_t ev_offset;
	uint32_t max_fps;
	uint32_t min_fps;
	uint32_t weight_mode;
	uint32_t default_index;
	uint32_t table_enable;
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
#endif

struct sensor_nr_header_param {
	uint32_t level_number;
	uint32_t default_strength_level;
	uint32_t nr_mode_setting;
	isp_uint *multi_nr_map_ptr;
	isp_uint *param_ptr;
	isp_uint *param1_ptr;
	isp_uint *param2_ptr;
	isp_uint *param3_ptr;
};

struct sensor_nr_simple_header_param {
	uint32_t level_number;
	uint32_t default_strength_level;
	uint32_t nr_mode_setting;
	isp_uint *multi_nr_map_ptr;
	isp_uint *param_ptr;
};

/************************************************************************************/
//flash
struct sensor_flash_attrib_param {
	uint32_t r_sum;
	uint32_t gr_sum;
	uint32_t gb_sum;
	uint32_t b_sum;
};

struct sensor_flash_attrib_cali{
	struct sensor_flash_attrib_param global;
	struct sensor_flash_attrib_param random;
};

struct sensor_flash_cali_param{
	uint16_t auto_threshold;
	uint16_t r_ratio;
	uint16_t g_ratio;
	uint16_t b_ratio;
	struct sensor_flash_attrib_cali attrib;
	uint16_t lum_ratio;
	uint16_t reserved1;
	uint32_t reserved0[19];
};

/************************************************************************************/
//environment detec
struct sensor_envi_detect_param {
	uint32_t enable;
	uint32_t num;
	struct isp_range envi_range[SENSOR_ENVI_NUM];
};



#define AE_FLICKER_NUM 2
#define AE_ISO_NUM_NEW 8
#define AE_WEIGHT_TABLE_NUM 3
#define AE_SCENE_NUM 8



#define _HAIT_MODIFIED_FLAG
#define SENSOR_CCE_NUM 0x09
#define SENSOR_CMC_NUM 0x09
#define SENSOR_CTM_NUM 0x09
#define SENSOR_HSV_NUM 0x09
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



/************************************************************************************/
//Pre-global gain
struct sensor_pre_global_gain_param {
	uint16_t gain;
	uint16_t reserved;
};

/************************************************************************************/
//Black level correction, between preGain and PDAF
struct sensor_blc_offset {
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct sensor_blc_param {
	struct isp_sample_point_info cur_idx;
	struct sensor_blc_offset tab[SENSOR_BLC_NUM];
};

/************************************************************************************/
//Post Black level correction, between NLM and RGBGain
struct sensor_postblc_param {
	struct isp_sample_point_info cur_idx;
	struct sensor_blc_offset tab[SENSOR_BLC_NUM];
};
/************************************************************************************/
// ppi, phase pixel inspector,PDAF
// definiton of phase pixel location, should define in sensor driver
struct sensor_pdaf_pattern_map {
         uint32_t is_right;
         struct isp_pos pattern_pixel;
};

struct sensor_pdaf_region {
         uint16_t start_col;
         uint16_t start_row;
         uint16_t end_col;
         uint16_t end_row;
         uint16_t width;
         uint16_t height;
         struct sensor_pdaf_pattern_map pdaf_pattern[64];
};

/************************************************************************************/
// ppi, phase pixel inspector,PDAF
// extract phase pixel to support PDAF
struct sensor_pdaf_af_win {
	uint16_t af_win_sx0;
	uint16_t af_win_sy0;
	uint16_t af_win_ex0;
	uint16_t af_win_ey0;
};

struct sensor_pdaf_extraction {
	uint16_t extractor_bypass;
	uint8_t continuous_mode;
	uint8_t skip_num;
	struct sensor_pdaf_af_win pdaf_af_win;
	struct sensor_pdaf_region pdaf_region;
};

/************************************************************************************/
// ppi, phase pixel inspector,PDAF
//correct phase pixel to get right image
struct sensor_pdaf_rgb_upboundary {
	uint16_t gr;
	uint16_t gb;
	uint16_t r;
	uint16_t b;
};

struct sensor_pdaf_edge_ratio {
	uint16_t ee_ratio_hv_rd;
	uint16_t ee_ratio_hv;
	uint32_t ee_ratio_rd;
};

struct sensor_pdaf_correction_level {
	uint16_t flat_th;
	uint16_t hot_pixel_th[3];
	uint16_t dead_pixel_th[3];
	uint16_t phase_map_corr_eb;
	uint8_t pdaf_grid;//0:1/32, 1:1/64
	uint8_t gfilter_flag;
	uint8_t flat_smt_index;
	uint8_t txt_smt_index;
	uint16_t phase_l_gain_map[128];
	uint16_t phase_r_gain_map[128];
	struct sensor_pdaf_rgb_upboundary pdaf_upperbound;
	struct sensor_blc_offset pdaf_blacklevel;//should be set to OB
	struct sensor_pdaf_edge_ratio pdaf_edgeRatio;
	uint32_t corrector_bypass;
};

/************************************************************************************/
//rgb gain
struct sensor_rgb_gain_param {
	uint16_t glb_gain;
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
};

/************************************************************************************/
//YUV noisefilter
struct sensor_yuv_noisefilter_gaussian {
	uint16_t random_r_shift;
	uint16_t random_r_offset;
	uint32_t random_seed[4];
	uint8_t random_takebit[8];
};

struct sensor_yuv_noisefilter_adv {
	uint8_t filter_thr;
	uint8_t filter_thr_mode;
	uint8_t filter_clip_p;
	uint8_t filter_clip_n;
	uint16_t filter_cv_t[4];
	uint8_t filter_cv_r[3];
	uint8_t reserved;
};

struct sensor_yuv_noisefilter_level {
	uint32_t noisefilter_shape_mode;
	struct sensor_yuv_noisefilter_gaussian yuv_noisefilter_gaussian;
	struct sensor_yuv_noisefilter_adv yuv_noisefilter_adv;
	uint32_t bypass;
};

/************************************************************************************/
//rgb gain yrandom, as a separate module in NR
struct sensor_rgb_dither_level {
	uint8_t pseudo_random_raw_range;
	uint8_t yrandom_mode;
	uint16_t yrandom_shift;
	uint32_t yrandom_seed;
	uint16_t yrandom_offset;
	uint16_t reserved;
	uint8_t yrandom_takebit[8];
	uint32_t pseudo_random_raw_bypass;
};

/************************************************************************************/
//non-linear correction
struct sensor_nlc_param {
	uint16_t r_node[29];
	uint16_t g_node[29];
	uint16_t b_node[29];
	uint16_t l_node[27];
};

/************************************************************************************/
// 2D-lens shading correction
struct sensor_lens_map_addr {
	uint32_t envi;
	uint32_t ct;
	uint32_t width;
	uint32_t height;
	uint32_t grid;
	uint32_t len;
	uint32_t offset;
};

struct sensor_2d_lsc_param {
	struct isp_sample_point_info cur_idx;
	isp_u32 tab_num;
	struct sensor_lens_map_addr map[SENSOR_LENS_NUM];
	void *data_area;
};
/************************************************************************************/
// 1D-lens shading correction, Radial lens
struct sensor_multi_curve_discription {
	struct isp_pos center_pos;
	uint32_t rlsc_init_r;
	uint32_t rlsc_init_r2;
	uint32_t rlsc_init_dr2;
};

struct sensor_1d_lsc_map {
	uint32_t rlsc_radius_step;
	struct sensor_multi_curve_discription curve_distcptn[SENSOR_LNC_RC_NUM];
	uint32_t len;  // indirect parameter
	uint32_t offset; //indirect parameter
};

struct sensor_1d_lsc_param {
	struct isp_sample_point_info cur_idx;
	isp_u32 tab_num;
	struct sensor_1d_lsc_map map[SENSOR_LENS_NUM];
	void *data_area;
};

/************************************************************************************/
//AWB Correction
struct sensor_awbc_thr {
	uint16_t r_thr;
	uint16_t g_thr;
	uint16_t b_thr;
	uint16_t reserved;
};

struct sensor_awbc_gain {
	uint16_t r_gain;
	uint16_t b_gain;
	uint16_t gr_gain;
	uint16_t gb_gain;
};

struct sensor_awbc_offset {
	uint16_t r_offset;
	uint16_t b_offset;
	uint16_t gr_offset;
	uint16_t gb_offset;
};

struct sensor_awbc_param {
	struct sensor_awbc_gain awbc_gain;
	struct sensor_awbc_thr awbc_thr;
	struct sensor_awbc_offset awbc_offset;
};

/************************************************************************************/

struct sensor_awbm_param {
	uint32_t comp_1d_bypass;
	uint32_t skip_num;
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

/************************************************************************************/
//AE monitor in RGB domain
struct sensor_rgb_aem_param {
	uint32_t aem_skip_num;
	uint32_t reserved[3];
	struct isp_pos win_start;
	struct isp_size win_size;
};

/************************************************************************************/
//Bad Pixel Correction
struct sensor_bpc_rules {
	struct isp_range k_val;
	uint8_t lowcoeff;
	uint8_t lowoffset;
	uint8_t highcoeff;
	uint8_t highoffset;
	uint16_t hv_ratio;
	uint16_t rd_ration;
};

struct sensor_bpc_comm {
	uint8_t bpc_mode;//0:normal,1:map,2:both
	uint8_t hv_mode;
	uint8_t rd_mode;//0:3x3,1:5x5,2:line mask
	uint8_t reserved;
	uint16_t lut_level[8];
	uint16_t slope_k[8];
	uint16_t intercept_b[8];
	float dtol;
};

struct sensor_bpc_thr {
	uint16_t double_th[4];
	uint16_t three_th[4];
	uint16_t four_th[4];
	uint8_t shift[3];
	uint8_t reserved;
	uint16_t flat_th;
	uint16_t texture_th;
};
struct sensor_bpc_pos {
	uint32_t pos_out_en;
};

struct sensor_bpc_level {
	struct sensor_bpc_comm bpc_comm;
	struct sensor_bpc_pos bpc_pos;
	struct sensor_bpc_thr bpc_thr;
	struct sensor_bpc_rules bpc_rules;
	uint32_t bypass;
};

/************************************************************************************/
//Y-NR, reduce noise on luma channel in YUV domain
struct sensor_ynr_texture_calc {
	uint8_t ynr_lowlux_bypass;
	uint8_t flat_thr[7];
	uint8_t edge_step[8];
	uint8_t sedge_thr;
	uint8_t txt_thr;
	uint8_t l1_txt_thr1;
	uint8_t l1_txt_thr2;
	uint8_t l0_lut_thr1;
	uint8_t l0_lut_thr0;
	uint8_t reserved[2];
};

struct sensor_ynr_layer_str {
	uint8_t blf_en;
	uint8_t wfindex;
	uint8_t eurodist[3];
	uint8_t reserved[3];
};

struct sensor_ynr_den_str {
	uint32_t wv_nr_en;
	uint16_t wlt_thr[3];
	uint16_t wlt_ratio[3];
	uint32_t ad_para[3];
	uint8_t sal_nr_str[8];
	uint8_t sal_offset[8];
	uint8_t subthresh[9];
	uint8_t lut_thresh[7];
	uint8_t addback[9];//filter_ratio in ISP TOOL
	uint8_t reserved[3];
	struct sensor_ynr_layer_str layer1;
	struct sensor_ynr_layer_str layer2;
	struct sensor_ynr_layer_str layer3;
};

struct sensor_ynr_region {
	uint16_t max_radius;
	uint16_t radius;
	uint16_t imgcetx;
	uint16_t imgcety;
	uint32_t dist_interval;
};

struct sensor_ynr_level {
	struct sensor_ynr_region ynr_region;
	struct sensor_ynr_texture_calc ynr_txt_calc;
	struct sensor_ynr_den_str ynr_den_str;
	uint16_t freqratio[24];
	uint8_t wltT[24];
	uint32_t bypass;
};

/************************************************************************************/
// UVDIV
struct sensor_cce_uvdiv_th {
	uint8_t uvdiv_th_l;
	uint8_t uvdiv_th_h;
	uint8_t reserved[2];
};

struct sensor_cce_uvdiv_chroma {
	uint8_t chroma_min_h;
	uint8_t chroma_min_l;
	uint8_t chroma_max_h;
	uint8_t chroma_max_l;
};

struct sensor_cce_uvdiv_lum {
	uint8_t lum_th_h_len;
	uint8_t lum_th_h;
	uint8_t lum_th_l_len;
	uint8_t lum_th_l;
};

struct sensor_cce_uvdiv_ratio {
	uint8_t ratio_0;
	uint8_t ratio_1;
	uint8_t ratio;
	uint8_t ratio_uv_min;
	uint8_t ratio_y_min0;
	uint8_t ratio_y_min1;
	uint8_t reserved[2];
};

struct sensor_cce_uvdiv_level{
	struct sensor_cce_uvdiv_lum uvdiv_lum;
	struct sensor_cce_uvdiv_chroma uvdiv_chroma;
	struct sensor_cce_uvdiv_th u_th_1;
	struct sensor_cce_uvdiv_th u_th_0;
	struct sensor_cce_uvdiv_th v_th_1;
	struct sensor_cce_uvdiv_th v_th_0;
	struct sensor_cce_uvdiv_ratio uvdiv_ratio;
	uint8_t uv_abs_th_len;
	uint8_t y_th_l_len;
	uint8_t y_th_h_len;
	uint8_t bypass;
};

/************************************************************************************/
//3DNR, two sets for pre and cap at least
struct sensor_3dnr_cfg {
	uint8_t src_wgt;
	uint8_t nr_thr;
	uint8_t nr_wgt;
	uint8_t thr_polyline[9];
	uint8_t gain_polyline[9];
	uint8_t reserved[3];
};

struct sensor_3dnr_factor {
	uint8_t u_thr[4];
	uint8_t v_thr[4];
	uint8_t u_div[4];
	uint8_t v_div[4];
};

struct sensor_3dnr_yuv_cfg {
	uint8_t grad_wgt_polyline[11];
	uint8_t reserved;
	struct sensor_3dnr_cfg y_cfg;
	struct sensor_3dnr_cfg u_cfg;
	struct sensor_3dnr_cfg v_cfg;
};

struct sensor_3dnr_radialval {
	uint16_t min;
	uint16_t max;
};
//reduce color noise around corners
struct sensor_3dnr_radialval_str {
	uint16_t r_circle[3];
	uint16_t reserved;
	struct sensor_3dnr_radialval u_range;
	struct sensor_3dnr_radialval v_range;
	struct sensor_3dnr_factor uv_factor;
};

struct sensor_3dnr_level {
	uint8_t bypass;
	uint8_t fusion_mode;
	uint8_t filter_swt_en;
	uint8_t reserved;
	struct sensor_3dnr_yuv_cfg yuv_cfg;
	struct sensor_3dnr_radialval_str sensor_3dnr_cor;
};

/************************************************************************************/
//GrGb Correction
struct sensor_grgb_ratio {
	uint8_t gr_ratio;
	uint8_t gb_ratio;
	uint16_t reserved;
	};
struct sensor_grgb_t_cfg {
	uint16_t  grgb_t1_cfg;
	uint16_t  grgb_t2_cfg;
	uint16_t  grgb_t3_cfg;
	uint16_t  grgb_t4_cfg;
	};

struct sensor_grgb_r_cfg {
	uint8_t grgb_r1_cfg;
	uint8_t grgb_r2_cfg;
	uint8_t grgb_r3_cfg;
	uint8_t reserved;
	};
struct sensor_grgb_curve {
	struct sensor_grgb_t_cfg t_cfg;
	struct sensor_grgb_r_cfg r_cfg;
};

struct sensor_grgb_level {
	uint8_t diff_th;
	uint8_t hv_edge_thr;
	uint16_t hv_flat_thr;
	uint16_t slash_edge_thr;
	uint16_t slash_flat_thr;
	struct sensor_grgb_ratio grgb_ratio;
	struct sensor_grgb_curve lum_curve_edge;
	struct sensor_grgb_curve lum_curve_flat;
	struct sensor_grgb_curve lum_curve_tex;
	struct sensor_grgb_curve frez_curve_edge;
	struct sensor_grgb_curve frez_curve_flat;
	struct sensor_grgb_curve frez_curve_tex;
	uint32_t bypass;
};

/************************************************************************************/
//rgb gain 2
struct sensor_rgb_gain2_param {
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
	uint16_t r_offset;
	uint16_t g_offset;
	uint16_t b_offset;
};


/************************************************************************************/
// nlm
struct sesor_simple_bpc{
	uint8_t simple_bpc_bypass;
	uint8_t simple_bpc_thr;
	uint16_t simple_bpc_lum_thr;
};

struct sensor_nlm_flat_degree {
	uint8_t flat_inc_str;
	uint8_t flat_match_cnt;
	uint16_t flat_thresh;
	uint16_t addback0;//for G channel
	uint16_t addback1;//for R and B channel
	uint16_t addback_clip_max;//plus noise
	uint16_t addback_clip_min;//minus noise
};

struct sensor_nlm_texture {
	uint8_t texture_dec_str;
	uint8_t addback30;
	uint8_t addback31;
	uint8_t reserved;
	uint16_t addback_clip_max;
	uint16_t addback_clip_min;
};

struct sensor_nlm_lum {
	struct sensor_nlm_flat_degree nlm_flat[3];
	struct sensor_nlm_texture nlm_texture;
};

struct sensor_nlm_first_lum {
	uint8_t nlm_flat_opt_bypass;
	uint8_t flat_opt_mode;
	uint8_t first_lum_bypass;
	uint8_t reserve;
	uint16_t lum_thr0;
	uint16_t lum_thr1;
	struct sensor_nlm_lum nlm_lum[3];
};

struct sensor_nlm_direction {
	uint8_t direction_mode_bypass;
	uint8_t dist_mode;
	uint8_t w_shift[3];
	uint8_t cnt_th;
	uint8_t reserved[2];
	uint16_t diff_th;
	uint16_t tdist_min_th;
};

struct sensor_vst_level {
	uint32_t vst_param[1024];
};

struct sensor_ivst_level {
	uint32_t ivst_param[1024];
};

struct sensor_lutw_level {
	uint32_t lut_w[72];
};

struct sensor_nlm_level {
	struct sensor_nlm_first_lum first_lum;
	struct sensor_nlm_direction nlm_dic;
	struct sesor_simple_bpc simple_bpc;
	struct sensor_lutw_level lut_w;
	uint8_t nlm_den_strenth;
	uint8_t imp_opt_bypass;
	uint8_t vst_bypass;
	uint8_t nlm_bypass;

};

/************************************************************************************/
// CFAI, css color saturation suppression
struct sensor_cfai_css_thr {
	uint16_t css_edge_thr;
	uint16_t css_weak_egde_thr;
	uint16_t css_txt1_thr;
	uint16_t css_txt2_thr;
	uint16_t uv_val_thr;
	uint16_t uv_diff_thr;
	uint16_t gray_thr;
	uint16_t pix_similar_thr;
};

struct sensor_cfai_css_skin_bundary {
	uint16_t skin_top;
	uint16_t skin_down;
};
struct sensor_cfai_css_skin {
	struct sensor_cfai_css_skin_bundary skin_u1;
	struct sensor_cfai_css_skin_bundary skin_v1;
	struct sensor_cfai_css_skin_bundary skin_u2;
	struct sensor_cfai_css_skin_bundary skin_v2;
};

struct sensor_cfai_css_green {
	uint16_t green_edge_thr;
	uint16_t green_weak_edge_thr;
	uint16_t green_txt1_thr;
	uint16_t green_txt2_thr;
	uint32_t green_flat_thr;
};

struct sensor_cfai_css_ratio {
	uint8_t edge_ratio;
	uint8_t weak_edge_ratio;
	uint8_t txt1_ratio;
	uint8_t txt2_ratio;
	uint32_t flat_ratio;
};

struct sensor_cfai_css_level {
	struct sensor_cfai_css_thr css_comm_thr;
	struct sensor_cfai_css_skin css_skin_uv;
	struct sensor_cfai_css_green css_green_thr;
	struct sensor_cfai_css_ratio r_ratio;
	struct sensor_cfai_css_ratio b_ratio;
	uint16_t css_alpha_for_txt2;
	uint16_t css_bypass;
};

//CFAI, CFA interpolation
struct sensor_cfai_gref_thr {
	uint16_t round_diff_03_thr;
	uint16_t lowlux_03_thr;
	uint16_t round_diff_12_thr;
	uint16_t lowlux_12_thr;
};

struct sensor_cfai_wgtctrl {
	uint8_t wgtctrl_bypass;
	uint8_t grid_dir_wgt1;
	uint8_t grid_dir_wgt2;
	uint8_t reversed;
};

struct sensor_cfai_level {
	uint32_t min_grid_new;
	uint8_t grid_gain_new;
	uint8_t str_edge_tr;
	uint8_t cdcr_adj_factor;
	uint8_t reserved[3];
	uint16_t grid_tr;
	uint32_t smooth_tr;
	uint16_t uni_dir_intplt_tr;//cfa_uni_dir_intplt_tr_new
	uint16_t rb_high_sat_thr;
	struct sensor_cfai_gref_thr gref_thr;
	struct sensor_cfai_wgtctrl cfai_dir_intplt;
	uint32_t cfa_bypass;//cfa interpolatin control, always open!!!
};

struct sensor_cfa_param_level {
	struct sensor_cfai_level cfai_intplt_level;
	struct sensor_cfai_css_level cfai_css_level;
};

/************************************************************************************/
//AF monitor in rgb domain, remove YAFM in YIQ domain
struct sensor_rgb_afm_iir_denoise {
	uint8_t afm_iir_en;
	uint8_t reserved[3];
	uint16_t iir_g[2];
	uint16_t iir_c[10];
};

struct sensor_rgb_afm_enhanced_fv {
	uint8_t fv_enhanced_mode;//0~5
	uint8_t clip_en;
	uint8_t fv_shift;
	uint8_t reserved;
	uint32_t fv_min_th;
	uint32_t fv_max_th;
};

struct sensor_rgb_afm_enhanced_pre {
	uint8_t channel_sel;//R/G/B for calc
	uint8_t denoise_mode;
	uint8_t center_wgt;//2^n
};

struct sensor_rgb_afm_enhanced_process {
	uint8_t fv1_coeff[4][9];//4x3x3, fv0 is fixed in the code
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
	uint8_t afm_skip_num;
	uint8_t touch_mode;
	uint8_t oflow_protect_en;
	uint8_t data_update_sel;
	uint32_t afm_mode;
	struct isp_pos_rect win_range[10];
	struct sensor_rgb_afm_iir_denoise afm_iir_denoise;
	struct sensor_rgb_afm_enhanced afm_enhanced;
	uint32_t bypass;
};

/************************************************************************************/
// CMC10
struct sensor_cmc10_param {
	struct isp_sample_point_info cur_idx;
	uint16_t matrix[SENSOR_CMC_NUM][9];
};

/************************************************************************************/
//Gamma Correction in full RGB domain
#define SENSOR_GAMMA_POINT_NUM 257

struct sensor_gamma_curve {
	struct isp_point points[SENSOR_GAMMA_POINT_NUM];
};
struct sensor_frgb_gammac_param {
	struct isp_sample_point_info cur_idx_info;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];

};

/************************************************************************************/
// CCE, color conversion enhance
struct sensor_cce_matrix_info {
	uint16_t matrix[9];
	uint16_t y_shift;
	uint16_t u_shift;
	uint16_t v_shift;
};

struct sensor_cce_param {
	isp_u32 cur_idx;
	struct sensor_cce_matrix_info tab[SENSOR_CCE_NUM];
	struct sensor_cce_matrix_info specialeffect[MAX_SPECIALEFFECT_NUM];
};

/************************************************************************************/
//HSV domain
struct sensor_hsv_cfg {
	uint16_t hsv_hrange_left;
	uint16_t hsv_s_curve[4];
	uint16_t hsv_v_curve[4];
	uint16_t hsv_hrange_right;
};

struct sensor_hsv_param {
	struct sensor_hsv_cfg sensor_hsv_cfg[5];
	struct isp_sample_point_info cur_idx;
	struct isp_data_bin_info map[SENSOR_HSV_NUM];
	void* data_area;
	struct isp_data_bin_info specialeffect[MAX_SPECIALEFFECT_NUM];
	void* specialeffect_data_area;
};

/************************************************************************************/
// radial color denoise
#if 0
struct sensor_radial_csc_map {
	uint32_t red_thr;
	uint32_t blue_thr;
	uint32_t max_gain_thr;
	struct sensor_multi_curve_discription r_curve_distcptn;
	struct sensor_multi_curve_discription b_curve_distcptn;
};

struct sensor_radial_csc_param {
	struct isp_sample_point_info cur_idx;
	struct sensor_radial_csc_map map[SENSOR_LENS_NUM];
};
#endif
/************************************************************************************/
//pre-color noise remove in rgb domain
#if 0
struct sensor_rgb_precdn_level{
	uint16_t thru0;
	uint16_t thru1;
	uint16_t thrv0;
	uint16_t thrv1;
	uint16_t median_mode;
	uint16_t median_thr;
	uint32_t bypass;
};

struct sensor_rgb_precdn_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_rgb_precdn_level rgb_precdn_level;
	uint32_t strength_level;
	isp_u32 reserved[2];
};
#endif
/************************************************************************************/
//Posterize, is Special Effect
struct sensor_posterize_param {
	uint8_t pstrz_bot[8];
	uint8_t pstrz_top[8];
	uint8_t pstrz_out[8];
	uint8_t specialeffect_bot[MAX_SPECIALEFFECT_NUM][8];
	uint8_t specialeffect_top[MAX_SPECIALEFFECT_NUM][8];
	uint8_t specialeffect_out[MAX_SPECIALEFFECT_NUM][8];
};

/************************************************************************************/
//AF monitor in rgb domain
#if 0
struct sensor_rgb_afm_sobel {
	uint32_t af_sobel_type;			//sobel filter win control
	struct isp_range sobel;		// filter thresholds
};

struct sensor_rgb_afm_filter_sel {
	uint8_t af_sel_filter1;		//filter select control
	uint8_t af_sel_filter2;		//filter select control
	uint8_t reserved[2];
};


struct sensor_rgb_afm_spsmd {
	uint8_t af_spsmd_rtgbot;		//filter data mode control
	uint8_t af_spsmd_diagonal;	//filter data mode control
	uint8_t af_spsmd_cal_mode;	//data output mode control
	uint8_t af_spsmd_type;		//data output mode control
	struct isp_range spsmd;		// filter thresholds
};

struct sensor_rgb_afm_param {
	uint8_t skip_num;
	struct sensor_rgb_afm_filter_sel filter_sel;
	struct sensor_rgb_afm_spsmd spsmd_ctl;
	struct sensor_rgb_afm_sobel sobel_ctl;
	struct isp_rect windows[25];
	uint32_t af_sobel_type;			//sobel filter win control
	struct isp_range sobel;		// filter thresholds
};
#endif
/************************************************************************************/
//Gamma Correction in YUV domain
struct sensor_y_gamma_param {
	isp_u32 cur_idx;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
	struct sensor_gamma_curve specialeffect[MAX_SPECIALEFFECT_NUM];
};

/************************************************************************************/
//Anti-flicker, old and new
struct sensor_y_afl_param_v1 {
	uint8_t skip_num;
	uint8_t line_step;
	uint8_t frm_num;
	uint8_t reserved0;
	uint16_t v_height;
	uint16_t reserved1;
	uint16_t col_st;//x st
	uint16_t col_ed;//x end
};

struct sensor_y_afl_param_v3 {
	uint16_t skip_num;
	uint16_t frm_num;
	uint16_t afl_x_st;
	uint16_t afl_x_ed;
	uint16_t afl_x_st_region;
	uint16_t afl_x_ed_region;
	struct isp_pos afl_step;
	struct isp_pos afl_step_region;
};

/************************************************************************************/
// AF monitor in YUV domain
struct sensor_y_afm_level {
	uint32_t iir_bypass;
	uint8_t skip_num;
	uint8_t afm_format;	//filter choose control
	uint8_t afm_position_sel;	//choose afm after CFA or auto contrust adjust
	uint8_t shift;
	uint16_t win[25][4];
	uint16_t coef[11];		//int16
	uint16_t reserved1;
};

/************************************************************************************/
//pre-color denoise in YUV domain
struct sensor_yuv_precdn_comm {
	uint8_t precdn_mode;
	uint8_t median_writeback_en;
	uint8_t median_mode;
	uint8_t den_stren;
	uint8_t uv_joint;
	uint8_t uv_thr;
	uint8_t y_thr;
	uint8_t reserved;
	uint8_t median_thr_u[2];
	uint8_t median_thr_v[2];
	uint32_t median_thr;
};

struct sensor_yuv_precdn_level{
	struct sensor_yuv_precdn_comm precdn_comm;
	float sigma_y;
	float sigma_d;
	float sigma_u;
	float sigma_v;
	uint8_t r_segu[2][7];							// param1
	uint8_t r_segv[2][7];							// param2
	uint8_t r_segy[2][7];							// param3
	uint8_t dist_w[25];								// param4
	uint8_t reserved0;
	uint32_t bypass;
};

/************************************************************************************/
//Brightness
struct sensor_bright_param {
	int8_t factor[16];
	uint32_t cur_index;
	int8_t scenemode[MAX_SCENEMODE_NUM];
};

/************************************************************************************/
//Contrast
struct sensor_contrast_param {
	uint8_t factor[16];
	uint32_t cur_index;
	uint8_t scenemode[MAX_SCENEMODE_NUM];
};

/************************************************************************************/
//Hist in YUV domain
struct sensor_yuv_hists_param {
	uint16_t hist_skip_num;
	uint16_t hist_mode_sel;
};

/************************************************************************************/
//Hist 2
struct sensor_yuv_hists2_param {
	uint16_t hist2_skip_num;
	uint16_t hist2_mode_sel;
	struct isp_pos_rect hist2_roi;
};

/************************************************************************************/
//CDN, Color Denoise
struct sensor_uv_cdn_level {
	float sigma_u;
	float sigma_v;
	uint8_t median_thru0;
	uint8_t median_thru1;
	uint8_t median_thrv0;
	uint8_t median_thrv1;
	uint8_t u_ranweight[31];
	uint8_t v_ranweight[31];
	uint8_t cdn_gaussian_mode;
	uint8_t median_mode;
	uint8_t median_writeback_en;
	uint8_t filter_bypass;
	uint16_t median_thr;
	uint32_t bypass;
};

/************************************************************************************/
//Edge Enhancement
struct sensor_ee_pn {
	uint16_t negative;
	uint16_t positive;
};

struct sensor_ee_ratio {
	uint8_t ratio_hv_3;
	uint8_t ratio_hv_5;
	uint8_t ratio_dg_3;
	uint8_t ratio_dg_5;
};

struct sensor_ee_t_cfg {
	uint16_t ee_t1_cfg;
	uint16_t ee_t2_cfg;
	uint16_t ee_t3_cfg;
	uint16_t ee_t4_cfg;
};

struct sensor_ee_r_cfg {
	uint8_t ee_r1_cfg;
	uint8_t ee_r2_cfg;
	uint8_t ee_r3_cfg;
	uint8_t reserved;
};

struct sensor_ee_c_cfg {
	uint8_t ee_c1_cfg;
	uint8_t ee_c2_cfg;
	uint8_t ee_c3_cfg;
	uint8_t reserved;
};

struct sensor_ee_polyline_cfg {
	struct sensor_ee_t_cfg t_cfg;
	struct sensor_ee_r_cfg r_cfg;
};

//sensor_ee_corner:
struct sensor_ee_corner {
	uint8_t ee_corner_cor;
	uint8_t reserved0[3];
	struct sensor_ee_pn ee_corner_th;
	struct sensor_ee_pn ee_corner_gain;
	struct sensor_ee_pn ee_corner_sm;
};

//sensor_ee_ipd
struct sensor_ee_ipd_smooth {
	uint32_t smooth_en;
	struct sensor_ee_pn smooth_mode;
	struct sensor_ee_pn smooth_ee_diff;
	struct sensor_ee_pn smooth_ee_thr;
};

struct sensor_ee_ipd {
	uint16_t ipd_bypass;
	uint16_t ipd_mask_mode;
	struct sensor_ee_pn ipd_flat_thr;
	struct sensor_ee_pn ipd_eq_thr;
	struct sensor_ee_pn ipd_more_thr;
	struct sensor_ee_pn ipd_less_thr;
	struct sensor_ee_ipd_smooth ipd_smooth;
};

//sensor_ee_polyline:
struct sensor_ee_gradient {
	uint16_t grd_cmpt_type;
	uint16_t wgt_hv2diag;
	uint32_t wgt_diag2hv;
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
	uint8_t ee_mode;
	uint8_t flat_smooth_mode;
	uint8_t edge_smooth_mode;
	uint8_t bypass;
};

/************************************************************************************/
//4A, smart, AFT
#if 0
struct ae_new_tuning_param {//total bytes must be 263480
    uint32_t version;
    uint32_t verify;
    uint32_t alg_id;
    uint32_t target_lum;
    uint32_t target_lum_zone;   // x16
    uint32_t convergence_speed;// x16
    uint32_t flicker_index;
    uint32_t min_line;
    uint32_t start_index;
    uint32_t exp_skip_num;
    uint32_t gain_skip_num;
    struct ae_stat_req stat_req;
    struct ae_flash_tuning flash_tuning;
    struct touch_zone touch_param;
    struct ae_ev_table ev_table;
    struct ae_exp_gain_table ae_table[AE_FLICKER_NUM][AE_ISO_NUM];
    struct ae_exp_gain_table flash_table[AE_FLICKER_NUM][AE_ISO_NUM];
    struct ae_weight_table weight_table[AE_WEIGHT_TABLE_NUM];
    struct ae_scene_info scene_info[AE_SCENE_NUM];
    struct ae_auto_iso_tab auto_iso_tab;
    struct ae_exp_anti exp_anti;
    struct ae_ev_cali ev_cali;
    struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
    struct ae_touch_param touch_info;/*it is in here,just for compatible; 3 * 4bytes */
    struct ae_face_tune_param face_info;

    /*13 * 4bytes */
    uint8_t monitor_mode;   /*0: single, 1: continue */
    uint8_t ae_tbl_exp_mode;    /*0: ae table exposure is exposure time; 1: ae table exposure is exposure line */
    uint8_t enter_skip_num; /*AE alg skip frame as entering camera */
    uint8_t cnvg_stride_ev_num;
    int8_t cnvg_stride_ev[32];
    int8_t stable_zone_ev[16];

    struct ae_sensor_cfg sensor_cfg;    /*sensor cfg information: 2 * 4bytes */

    struct ae_lv_calibration lv_cali;   /*1 * 4bytes */
    /*scene detect and others alg */
    /*for touch info */

    struct flat_tuning_param flat_param;    /*51 * 4bytes */
    struct ae_flash_tuning_param flash_param;   /*1 * 4bytes */
    struct region_tuning_param region_param;    /*180 * 4bytes */
    struct mulaes_tuning_param mulaes_param;    /*9 * 4bytes */
    struct face_tuning_param face_param;

    uint32_t reserved[2046];
};

typedef struct _af_tuning_param{
    uint8_t             flag;// Tuning parameter switch, 1 enable tuning parameter, 0 disenable it
    filter_clip_t       filter_clip[SCENE_NUM][GAIN_TOTAL];// AF filter threshold
    int32_t             bv_threshold[SCENE_NUM][SCENE_NUM];//BV threshold
    AF_Window_Config SAF_win;// SAF window config
    AF_Window_Config CAF_win; // CAF window config
    AF_Window_Config VAF_win; // VAF window config
    // default param for indoor/outdoor/dark
    AF_Tuning AF_Tuning_Data[SCENE_NUM];// Algorithm related parameter
    uint8_t dummy[101];// for 4-bytes alignment issue
}af_tuning_param_t;

#endif
/**********************************************************************************/
//aft
typedef struct isp_aft_param{
    uint32_t img_blk_support;
    uint32_t hist_support;
    uint32_t afm_support;
    uint32_t caf_blk_support;
    uint32_t gyro_support;
    uint32_t gyro_debug_support;
    uint32_t gsensor_support;
    uint32_t need_rough_support;
    uint32_t abort_af_support;
    uint32_t need_print_trigger_support;
    uint32_t ae_select_support;
    uint32_t ae_calibration_support;
    uint32_t dump_support;
    uint32_t afm_abort_support;
    uint32_t ae_skip_line;
    uint32_t img_blk_value_thr;
    uint32_t img_blk_num_thr;
    uint32_t img_blk_cnt_thr;
    uint32_t img_blk_value_stab_thr;
    uint32_t img_blk_num_stab_thr;
    uint32_t img_blk_cnt_stab_thr;
    uint32_t img_blk_cnt_stab_max_thr;
    uint32_t img_blk_skip_cnt;
    uint32_t img_blk_value_video_thr;
    uint32_t img_blk_num_video_thr;
    uint32_t img_blk_cnt_video_thr;
    uint32_t img_blk_value_video_stab_thr;
    uint32_t img_blk_num_video_stab_thr;
    uint32_t img_blk_cnt_video_stab_thr;
    uint32_t img_blk_value_abort_thr;
    uint32_t img_blk_num_abort_thr;
    uint32_t img_blk_cnt_abort_thr;
    uint32_t ae_mean_sat_thr[3];//saturation
    uint8_t afm_roi_left_ration;
    uint8_t afm_roi_top_ration;
    uint8_t afm_roi_width_ration;
    uint8_t afm_roi_heigth_ration;
    uint8_t afm_num_blk_hor;
    uint8_t afm_num_blk_vec;
    uint32_t afm_value_thr;
    uint32_t afm_num_thr;
    uint32_t afm_value_stab_thr;
    uint32_t afm_num_stab_thr;
    uint32_t afm_cnt_stab_thr;
    uint32_t afm_cnt_move_thr;
    uint32_t afm_value_video_thr;
    uint32_t afm_num_video_thr;
    uint32_t afm_cnt_video_move_thr;
    uint32_t afm_value_video_stab_thr;
    uint32_t afm_num_video_stab_thr;
    uint32_t afm_cnt_video_stab_thr;
    uint32_t afm_skip_cnt;
    uint32_t afm_need_af_cnt_thr[3];
    uint32_t afm_fv_diff_thr[3];
    uint32_t afm_fv_queue_cnt[3];
    uint32_t afm_fv_stab_diff_thr[3];
    uint32_t afm_need_af_cnt_video_thr[3];
    uint32_t afm_fv_video_diff_thr[3];
    uint32_t afm_fv_video_queue_cnt[3];
    uint32_t afm_fv_video_stab_diff_thr[3];
    uint32_t afm_need_af_cnt_abort_thr[3];
    uint32_t afm_fv_diff_abort_thr[3];
//    uint32_t bv_thr[3];
    uint32_t afm_need_rough_thr;
    uint32_t caf_work_lum_thr;  //percent of ae target
    float gyro_move_thr;
    uint32_t gyro_move_cnt_thr;
    float gyro_stab_thr;
    uint32_t gyro_stab_cnt_thr;
    float gyro_video_move_thr;
    uint32_t gyro_video_move_cnt_thr;
    float gyro_video_stab_thr;
    uint32_t gyro_video_stab_cnt_thr;
    float gyro_move_abort_thr;
    uint32_t gyro_move_cnt_abort_thr;
    uint32_t sensor_queue_cnt;
    uint32_t gyro_move_percent_thr;
    uint32_t gyro_stab_percent_thr;
    float gsensor_stab_thr;
    float hist_stab_thr;
    float hist_stab_diff_thr;
    float hist_base_stab_thr;
    float hist_need_rough_cc_thr;
    uint32_t hist_max_frame_queue;
    uint32_t hist_stab_cnt_thr;
    uint32_t vcm_pos_abort_thr;
    uint32_t abort_af_y_thr;
    uint32_t reserved[16];
}isp_aft_param_t;

typedef struct aft_tuning_param{
    uint32_t version;
    struct isp_aft_param aft_tuning_param;
}aft_tuning_param_t;
#if 0
struct awb_tuning_param
{
    int magic;
    int version;
    int date;
    int time;

    /* MWB table */
    unsigned char wbModeNum;
    unsigned char wbModeId[10];
    struct awb_rgb_gain wbMode_gain[10];   // mwb_gain[0] is init_gain
    struct awb_rgb_gain mwb_gain[101];   // mwb_gain[0] is init_gain

    /* AWB parameter */
    int rgb_12bit_sat_value;

    int pg_x_ratio;
    int pg_y_ratio;
    int pg_shift;

    int isotherm_ratio;
    int isotherm_shift;

    int rmeanCali;
    int gmeanCali;
    int bmeanCali;

    int dct_scale_65536; //65536=1x


    short pg_center[512];
    short pg_up[512];
    short pg_dn[512];

    short cct_tab[512];
    short cct_tab_div;
    short cct_tab_sz;
    int cct_tab_a[40];
    int cct_tab_b[40];

    int dct_start;
    int pg_start;

    int dct_div1;
    int dct_div2;

    int w_lv_len;
    short w_lv[8];
    short w_lv_tab[8][512];

    int dct_sh_bv[4];
    short dct_sh[4][512];
    short dct_pg_sh100[4][512];
    short dct_range[4][2];
    short dct_range2[4];
    short dct_range3[4];
    short dct_def[4];
    short dct_def2[4];

    int red_num;
    short red_x[10];
    short red_y[10];

    int blue_num;
    short blue_x[10];
    short blue_comp[10];
    short blue_sh[10];

    short ctCompX1;
    short ctCompX2;
    short ctCompY1;
    short ctCompY2;

    short defev100[2];
    short defx[2][9];

    int check;

    int reserved[20160/4];
};
struct alsc_alg0_turn_para{
    float pa;               //threshold for seg
    float pb;
    uint32_t fft_core_id;   //fft param ID
    uint32_t con_weight;        //convergence rate
    uint32_t bv;
    uint32_t ct;
    uint32_t pre_ct;
    uint32_t pre_bv;
    uint32_t restore_open;
    uint32_t freq;
    float threshold_grad;
};

struct tuning_param {
    uint32_t version;
    uint32_t bypass;
    struct isp_smart_param param;
};
#endif

/**************************************************************************/
//sft af
struct isp_sft_af_param {

};

/*************************************************************************/
struct isp_alsc_param {

};
/************************************************************************************/
//smart param begin
#define ISP_SMART_MAX_BV_SECTION 8
#define ISP_SMART_MAX_BLOCK_NUM 32 //28
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

enum isp_smart_id {
	ISP_SMART_LNC = 0,
	ISP_SMART_COLOR_CAST = 1,
	ISP_SMART_CMC = 2,
	ISP_SMART_SATURATION_DEPRESS = 3,
	ISP_SMART_HSV = 4,
	ISP_SMART_GAMMA = 5,
	ISP_SMART_GAIN_OFFSET = 6,
	ISP_SMART_BLC = 7,
	ISP_SMART_POSTBLC = 8,
//NR
	ISP_SMART_PDAF_CORRECT = 9,
	ISP_SMART_NLM = 10,
	ISP_SMART_RGB_DITHER = 11,
	ISP_SMART_BPC = 12,
	ISP_SMART_GRGB = 13,
	ISP_SMART_CFAE = 14,
	ISP_SMART_RGB_AFM = 15,
	ISP_SMART_UVDIV = 16,
	ISP_SMART_3DNR_PRE = 17,
	ISP_SMART_3DNR_CAP = 18,
	ISP_SMART_EDGE = 19,
	ISP_SMART_YUV_PRECDN = 20,
	ISP_SMART_YNR = 21,
	ISP_SMART_UVCDN = 22,
	ISP_SMART_UV_POSTCDN = 23,
	ISP_SMART_IIRCNR_IIR = 24,
	ISP_SMART_IIR_YRANDOM = 25,
	ISP_SMART_YUV_NOISEFILTER = 26,
	ISP_SMART_MAX
};

struct isp_smart_component_cfg {
	uint32_t id;
	uint32_t type;
	uint32_t offset;
	uint32_t size;

	uint32_t x_type;	// isp_smart_x_type
	uint32_t y_type;	// isp_smart_y_type
	int32_t default_val;
	int32_t use_flash_val;
	int32_t flash_val;	//use this value when flash is open

	uint32_t section_num;
	struct isp_range bv_range[ISP_SMART_MAX_BV_SECTION];
	struct isp_piecewise_func func[ISP_SMART_MAX_BV_SECTION];
};

struct isp_smart_block_cfg {
	uint32_t enable;
	uint32_t smart_id;	//id to identify the smart block
	uint32_t block_id;	//id to identify the isp block (destination block)
	uint32_t component_num;
	struct isp_smart_component_cfg component[ISP_SMART_MAX_VALUE_NUM];
};

/*tuning param for smart block*/
struct isp_smart_param {
	uint32_t block_num;
	struct isp_smart_block_cfg block[ISP_SMART_MAX_BLOCK_NUM];
};
/*smart param end*/

/************************************************************************************/
//Color Saturation Adjustment
struct sensor_saturation_param {
	uint8_t csa_factor_u[16];
	uint8_t csa_factor_v[16];
	uint32_t index_u;
	uint32_t index_v;
	uint8_t scenemode[2][MAX_SCENEMODE_NUM];
};

/************************************************************************************/
//Hue Adjustment
struct sensor_hue_param {
	uint16_t hue_theta[16];
	uint32_t cur_index;
};

/************************************************************************************/
//post-color denoise
struct sensor_postcdn_thr {
	uint16_t thr0;//algorithm reserved
	uint16_t thr1;//algorithm reserved
};

struct sensor_postcdn_r_seg {
	uint8_t r_seg0[7];
	uint8_t r_seg1[7];
	uint8_t reserved0[2];
};

struct sensor_postcdn_distw {
	uint8_t distw[15][5];
	uint8_t reserved0;
};

struct sensor_uv_postcdn_level{
	struct sensor_postcdn_r_seg r_segu;
	struct sensor_postcdn_r_seg r_segv;
	struct sensor_postcdn_distw r_distw;
	struct sensor_postcdn_thr uv_thr;
	struct sensor_postcdn_thr u_thr;
	struct sensor_postcdn_thr v_thr;
	float sigma_d;
	float sigma_u;
	float sigma_v;
	uint16_t adpt_med_thr;
	uint8_t median_mode;
	uint8_t uv_joint;
	uint8_t median_res_wb_en;
	uint8_t postcdn_mode;
	uint8_t downsample_bypass;
	uint32_t bypass;
};

/************************************************************************************/
//Y_DELAY, should define in other
struct sensor_y_delay_param {
	//uint16_t bypass;//   can't bypass!!!
	uint16_t ydelay_step;
};

/************************************************************************************/
//IIR color noise reduction, should named CCNR in tuning tool
struct sensor_iircnr_pre {
	uint16_t iircnr_pre_uv_th;
	uint16_t iircnr_sat_ratio;
};

struct sensor_iircnr_ee_judge {
	uint16_t y_edge_thr_max[8];
	uint16_t iircnr_uv_s_th;
	uint16_t iircnr_uv_th;
	uint16_t iircnr_uv_dist;
	uint16_t uv_low_thr1[8];
};

struct sensor_iircnr_uv_thr {
	uint32_t uv_low_thr2;
	uint32_t uv_high_thr2;
};

struct sensor_iircnr_str {
	uint8_t iircnr_y_max_th;
	uint8_t iircnr_y_th;
	uint8_t iircnr_y_min_th;
	uint8_t iircnr_uv_diff_thr;
	uint16_t y_edge_thr_min[8];
	uint16_t iircnr_alpha_hl_diff_u;
	uint16_t iircnr_alpha_hl_diff_v;
	uint32_t iircnr_alpha_low_u;
	uint32_t iircnr_alpha_low_v;
	struct sensor_iircnr_uv_thr cnr_uv_thr2[8];
};

struct sensor_iircnr_post {
	uint32_t iircnr_css_lum_thr;
};

struct sensor_iircnr_level{
	uint16_t cnr_iir_mode;
	uint16_t cnr_uv_pg_th;
	uint32_t ymd_u;
	uint32_t ymd_v;
	uint32_t ymd_min_u;
	uint32_t ymd_min_v;
	uint16_t slop_uv[8];
	uint16_t slop_y[8];
	uint32_t middle_factor_uv[8];
	uint32_t middle_factor_y[8];
	struct sensor_iircnr_pre pre_uv_th;
	struct sensor_iircnr_ee_judge iircnr_ee;
	struct sensor_iircnr_str iircnr_str;
	struct sensor_iircnr_post css_lum_thr;
	uint32_t bypass;
};

/************************************************************************************/
//IIR yrandom
struct sensor_iircnr_yrandom_level {
	uint8_t yrandom_shift;
	uint8_t reserved0[3];
	uint32_t yrandom_seed;
	uint16_t yrandom_offset;
	uint16_t reserved1;
	uint8_t yrandom_takebit[8];
	uint32_t bypass;
};
struct sensor_iircnr_yrandom_param {
	struct sensor_iircnr_yrandom_level iircnr_yrandom_level;
};

/************************************************************************************/
//alsc
struct alsc_alg0_turn_param {
	float pa;				//threshold for seg
	float pb;
	uint32_t fft_core_id; 	//fft param ID
	uint32_t con_weight;		//convergence rate
	uint32_t bv;
	uint32_t ct;
	uint32_t pre_ct;
	uint32_t pre_bv;
	uint32_t restore_open;
	uint32_t freq;
	float threshold_grad;
};
/************************************************************************************/
/********** parameter block definition **********/
enum ISP_BLK_ID {
	ISP_BLK_ID_BASE = 0,
	ISP_BLK_SHARKL2_BASE = 0x4000,
	ISP_BLK_PRE_GBL_GAIN = 0x4001,
	ISP_BLK_BLC = 0x4002,
	ISP_BLK_PDAF_EXTRACT = 0x4003,
	ISP_BLK_PDAF_CORRECT = 0x4004,
	ISP_BLK_NLM = 0x4005, // + ISP_BLK_VST + ISP_BLK_IVST
	ISP_BLK_POSTBLC = 0x4006,
	ISP_BLK_RGB_GAIN = 0x4007,
	ISP_BLK_RGB_DITHER = 0x4008,//RGBgain yrandom
	ISP_BLK_NLC = 0x4009,
	ISP_BLK_2D_LSC = 0x400A,
	ISP_BLK_1D_LSC = 0x400B,//radial lens
	ISP_BLK_BINNING4AWB = 0x400C,
	ISP_BLK_AWBC = 0x400D,
	ISP_BLK_RGB_AEM = 0x400E,
	ISP_BLK_BPC = 0x400F,
	ISP_BLK_GRGB = 0x4010,
	ISP_BLK_CFA = 0x4011,//include CFAI intplt and CFAI CSS
	ISP_BLK_RGB_AFM = 0x4012,
	ISP_BLK_CMC10 = 0x4013,
	ISP_BLK_RGB_GAMC = 0x4014,
	ISP_BLK_HSV = 0x4015,
	ISP_BLK_POSTERIZE = 0x4016,
	ISP_BLK_CCE = 0x4017,
	ISP_BLK_UVDIV = 0x4018,
	ISP_BLK_YIQ_AFL_V1 = 0x4019,
	ISP_BLK_YIQ_AFL_V3  = 0x401A,
	ISP_BLK_3DNR_PRE= 0x401B,
	ISP_BLK_3DNR_CAP = 0x401C,
	ISP_BLK_YUV_PRECDN = 0x401D,
	ISP_BLK_YNR = 0x401E,
	ISP_BLK_BRIGHT = 0x401F,
	ISP_BLK_CONTRAST = 0x4020,
	ISP_BLK_HIST = 0x4021,
	ISP_BLK_HIST2 = 0x4022,
	ISP_BLK_EDGE = 0x4023,
	ISP_BLK_Y_GAMMC = 0x4024,
	ISP_BLK_UV_CDN = 0x4025,
	ISP_BLK_UV_POSTCDN = 0x4026,
	ISP_BLK_Y_DELAY = 0x4027,
	ISP_BLK_SATURATION = 0x4028,
	ISP_BLK_HUE = 0x4029,
	ISP_BLK_IIRCNR_IIR = 0x402A,
	ISP_BLK_IIRCNR_YRANDOM = 0x402B,
	ISP_BLK_YUV_NOISEFILTER= 0X402C,
//3A and smart
	ISP_BLK_SMART = 0x402D,
	ISP_BLK_SFT_AF = 0x402E,
	ISP_BLK_ALSC = 0x402F,
	ISP_BLK_AFT	= 0x4030,
	ISP_BLK_AE_NEW = 0X4031,
	ISP_BLK_AWB_NEW = 0x4032,
	ISP_BLK_AF_NEW = 0x4033,
//Flash and envi
	ISP_BLK_FLASH_CALI = 0x4034,
	ISP_BLK_ENVI_DETECT = 0x4035,
	ISP_BLK_EXT,
	ISP_BLK_ID_MAX,
};

enum {
	ISP_BLK_PDAF_CORRECT_T = 0x00,
	ISP_BLK_NLM_T,
	ISP_BLK_VST_T,
	ISP_BLK_IVST_T,
	ISP_BLK_RGB_DITHER_T,
	ISP_BLK_BPC_T,
	ISP_BLK_GRGB_T,
	ISP_BLK_CFA_T,
	ISP_BLK_RGB_AFM_T,
	ISP_BLK_UVDIV_T,
	ISP_BLK_3DNR_PRE_T,
	ISP_BLK_3DNR_CAP_T,
	ISP_BLK_YUV_PRECDN_T,
	ISP_BLK_CDN_T,
	ISP_BLK_POSTCDN_T,
	ISP_BLK_YNR_T,
	ISP_BLK_EDGE_T,
	ISP_BLK_IIRCNR_T,
	ISP_BLK_YUV_NOISEFILTER_T,
	ISP_BLK_TYPE_MAX
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

	unsigned int* data_ptr;
	unsigned int  data_size;
};

union sensor_version_name{
		uint32_t sensor_name_in_word[8];
		uint8_t sensor_name[32];
	};

struct sensor_version_info {
	uint32_t version_id;
	union sensor_version_name sensor_ver_name;
	uint32_t ae_struct_version;
	uint32_t awb_struct_version;
	uint32_t lnc_struct_version;
	uint32_t nr_struct_version;
	uint32_t reserve0;
	uint32_t reserve1;
	uint32_t reserve2;
	uint32_t reserve3;
	uint32_t reserve4;
	uint32_t reserve5;
	uint32_t reserve6;
};

struct sensor_ae_tab_param {
	uint8_t *ae;
	uint32_t  ae_len;
};

struct sensor_awb_tab_param {
	uint8_t *awb;
	uint32_t  awb_len;
};

struct sensor_lnc_tab_param {
	uint8_t *lnc;
	uint32_t  lnc_len;
};

 struct ae_exp_gain_tab{
	uint32_t *index;
	uint32_t index_len;
	uint32_t *exposure;
	uint32_t exposure_len;
	uint32_t *dummy;
	uint32_t dummy_len;
	uint16_t *again;
	uint32_t again_len;
	uint16_t *dgain;
	uint32_t dgain_len;
};

  struct ae_scene_exp_gain_tab{
	uint32_t *scene_info;
	uint32_t scene_info_len;
	uint32_t *index;
	uint32_t index_len;
	uint32_t *exposure;
	uint32_t exposure_len;
	uint32_t *dummy;
	uint32_t dummy_len;
	uint16_t *again;
	uint32_t again_len;
	uint16_t *dgain;
	uint32_t dgain_len;
};

 struct ae_weight_tab{
	uint8_t *weight_table;
	uint32_t len;
 };

 struct ae_auto_iso_tab_v1{
	 uint16_t *auto_iso_tab;
	 uint32_t len;
 };

struct sensor_ae_tab{
	struct sensor_ae_tab_param ae_param;
	struct ae_exp_gain_tab ae_tab[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_exp_gain_tab ae_flash_tab[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_weight_tab weight_tab[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_exp_gain_tab scene_tab[AE_SCENE_NUM][AE_FLICKER_NUM];
	struct ae_auto_iso_tab_v1  auto_iso_tab[AE_FLICKER_NUM];
};
/*******************************new***************/
struct sensor_2d_lsc_param_v1 {
	struct isp_sample_point_info cur_idx;
	isp_u32 tab_num;
	//struct sensor_lens_map_addr map[SENSOR_LENS_NUM];
	//void *data_area;
};


struct sensor_lens_map_info{
	uint32_t envi;
	uint32_t ct;
	uint32_t width;
	uint32_t height;
	uint32_t grid;
};

struct sensor_lens_map {
	uint32_t *map_info;
	uint32_t  map_info_len;
	uint16_t *weight_info;
	uint32_t  weight_info_len;
	uint32_t *lnc_map_tab_len;
	uint32_t *lnc_map_tab_offset;
	uint16_t *lnc_addr;
	uint32_t lnc_len;
};

struct sensor_lsc_map{
	struct sensor_lnc_tab_param lnc_param;
	struct sensor_lens_map map[LNC_MAP_COUNT];
};

struct sensor_nr_scene_map_param {
	uint32_t nr_scene_map[MAX_MODE_NUM];
};

struct sensor_nr_level_map_param {
	uint8_t nr_level_map[MAX_NR_NUM];
};

struct sensor_nr_map_group_param {
	uint32_t *nr_scene_map_ptr;
	uint32_t nr_scene_map_len;
	uint8_t *nr_level_map_ptr;
	uint32_t nr_level_map_len;
};

struct nr_set_group_unit {
	uint8_t*  nr_ptr;
	uint32_t  nr_len;
};
struct sensor_nr_set_group_param {
	uint8_t *pdaf_correct;
	uint32_t  pdaf_correct_len;
	uint8_t *nlm;
	uint32_t  nlm_len;
	uint8_t *vst;
	uint32_t  vst_len;
	uint8_t *ivst;
	uint32_t  ivst_len;
	uint8_t *rgb_dither;
	uint32_t  rgb_dither_len;
	uint8_t *bpc;
	uint32_t  bpc_len;
	uint8_t *grgb;
	uint32_t  grgb_len;
	uint8_t *cfa;
	uint32_t  cfa_len;
	uint8_t *rgb_afm;
	uint32_t  rgb_afm_len;
	uint8_t *uvdiv;
	uint32_t  uvdiv_len;
	uint8_t *nr3d_pre;
	uint32_t  nr3d_pre_len;
	uint8_t *nr3d_cap;
	uint32_t  nr3d_cap_len;
	uint8_t *yuv_precdn;
	uint32_t  yuv_precdn_len;
	uint8_t *cdn;
	uint32_t  cdn_len;
	uint8_t *postcdn;
	uint32_t  postcdn_len;
	uint8_t *ynr;
	uint32_t  ynr_len;
	uint8_t *edge;
	uint32_t  edge_len;
	uint8_t *iircnr;
	uint32_t  iircnr_len;
	uint8_t *yuv_noisefilter;
	uint32_t  yuv_noisefilter_len;
};
struct sensor_nr_param{
	struct sensor_nr_set_group_param nr_set_group;
};


struct sensor_awb_map{
	uint16_t *addr;
	uint32_t len;		//by bytes
};

struct sensor_awb_weight{
	uint8_t *addr;
	uint32_t weight_len;
	uint16_t *size;
	uint32_t size_param_len;
};

struct sensor_awb_map_weight_param{
	struct sensor_awb_tab_param awb_param;
	struct sensor_awb_map awb_map;
	struct sensor_awb_weight awb_weight;
};
struct sensor_awb_table_param {
	uint8_t awb_pos_weight[AWB_POS_WEIGHT_LEN];
	uint16_t awb_pos_weight_width_height[AWB_POS_WEIGHT_WIDTH_HEIGHT/sizeof(uint16_t)];
	uint16_t awb_map[];
};
struct sensor_lsc_2d_map_info {
	uint32_t envi;
	uint32_t ct;
	uint32_t width;
	uint32_t height;
	uint32_t grid;
};
struct sensor_lsc_2d_tab_info_param {
	struct sensor_lsc_2d_map_info lsc_2d_map_info;
	uint16_t lsc_2d_weight[4096];
	uint32_t lsc_2d_len;
	uint32_t lsc_2d_offset;
};
struct sensor_lsc_2d_table_param {
	struct sensor_lsc_2d_tab_info_param lsc_2d_info[LNC_MAP_COUNT];
	uint16_t lsc_2d_map[];
};
struct sensor_raw_fix_info{
	struct sensor_ae_tab ae;
	struct sensor_lsc_map lnc;
	struct sensor_awb_map_weight_param awb;
	struct sensor_nr_param nr;
};

struct sensor_raw_note_info{
	uint8_t *note;
	uint32_t node_len;
};

struct sensor_nr_fix_info {
	struct sensor_nr_scene_map_param *nr_scene_ptr;
	struct sensor_nr_level_map_param *nr_level_number_ptr;
	struct sensor_nr_level_map_param *nr_default_level_ptr;
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
};
#if 0
struct raw_param_info_tab {
	uint32_t param_id;
	struct sensor_raw_info *info_ptr;
	uint32_t(*identify_otp) (void *param_ptr);
	uint32_t(*cfg_otp) (void *param_ptr);
};
#endif
struct denoise_param_update {
	//struct sensor_pwd_level *pwd_level_ptr;
	struct sensor_bpc_level *bpc_level_ptr;
	//struct sensor_bdn_level *bdn_level_ptr;
	struct sensor_grgb_level *grgb_level_ptr;
	struct sensor_nlm_level *nlm_level_ptr;
	struct sensor_vst_level *vst_level_ptr;
	struct sensor_ivst_level *ivst_level_ptr;
	//struct sensor_flat_offset_level *flat_offset_level_ptr;
	struct sensor_cfai_level *cfae_level_ptr;
	//struct sensor_rgb_precdn_level *rgb_precdn_level_ptr;
	struct sensor_yuv_precdn_level *yuv_precdn_level_ptr;
	//struct sensor_prfy_level *prfy_level_ptr;
	struct sensor_uv_cdn_level *uv_cdn_level_ptr;
	struct sensor_ee_level *ee_level_ptr;
	struct sensor_uv_postcdn_level *uv_postcdn_level_ptr;
	struct sensor_iircnr_level *iircnr_level_ptr;
	//struct sensor_iircnr_yrandom_level *iircnr_yrandom_level_ptr;
	struct sensor_cce_uvdiv_level *cce_uvdiv_level_ptr;
	struct sensor_3dnr_level *dnr_cap_level_ptr;
	struct sensor_3dnr_level *dnr_pre_level_ptr;
	struct sensor_pdaf_correction_level *pdaf_correction_level_ptr;
	struct sensor_rgb_afm_level *rgb_afm_level_ptr;
	struct sensor_rgb_dither_level *rgb_dither_level_ptr;
	struct sensor_ynr_level *ynr_level_ptr;
	struct sensor_yuv_noisefilter_level *yuv_noisefilter_level_ptr;
	//struct sensor_y_afm_level *y_afm_level_ptr;
	uint32_t *nr_scene_map_ptr;
	uint8_t *nr_level_number_map_ptr;
	uint8_t *nr_default_level_map_ptr;
	uint32_t multi_nr_flag;
};


#endif
