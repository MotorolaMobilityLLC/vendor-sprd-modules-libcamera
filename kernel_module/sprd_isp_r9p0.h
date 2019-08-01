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

#ifndef _SPRD_ISP_R9P0_H_
#define _SPRD_ISP_R9P0_H_

#define BUF_ALIGN(w) ((((w) + 512 + 1024 - 1) >> 10) << 10)

#define ISP_PDAF_STATIS_BUF_SIZE                BUF_ALIGN(0x3A23D0)

#define PDAF_PPI_NUM			64
#define PDAF_PPI_GAIN_MAP_LEN 128
#define ISP_HSV_TABLE_NUM 	360
#define ISP_VST_IVST_NUM		1024
#define ISP_FRGB_GAMMA_PT_NUM 257
#define POSTERIZE_NUM			129
#define ISP_YUV_GAMMA_NUM	129
/*
 * 48M  8048 x 6036
 * 24M  5664 x 4248
 * 16M  4672 x 3504
 */
#define ISP_MAX_WIDTH   8048
#define ISP_MAX_HEIGHT  6036

#define DCAM_24M_WIDTH  5664


/* AEM max windows: 128 x 128, 3*16Bytes for each window */
#define STATIS_AEM_BUF_SIZE (128 * 128 * 16 * 3)
#define STATIS_AEM_BUF_NUM 8

/* AFM max windows: 20 x 15, 16 Bytes for each window */
#define STATIS_AFM_BUF_SIZE  (20 * 15 * 16)
#define STATIS_AFM_BUF_NUM 8

/* AFL: global 80 x 16 bytes for one frame, region 482 x 16 bytes one frame */
#define STATIS_AFL_GBUF_SIZE   (80 * 16 * 3 + 64)
#define STATIS_AFL_RBUF_SIZE   (482 * 16 * 3 + 64)
#define STATIS_AFL_BUF_SIZE   (STATIS_AFL_GBUF_SIZE + STATIS_AFL_RBUF_SIZE)
#define STATIS_AFL_BUF_NUM 8

/* hist: 154 x 16 bytes */
#define STATIS_HIST_BUF_SIZE   (154 * 16)
#define STATIS_HIST_BUF_NUM 8

#define STATIS_PDAF_BUF_SIZE  ISP_PDAF_STATIS_BUF_SIZE
#define STATIS_PDAF_BUF_NUM 8

#define STATIS_EBD_BUF_SIZE   0x8000
#define STATIS_EBD_BUF_NUM 8

/*
 * 3dnr: max size is (4672+3504)*2 bytes
 * should save at least 5 mv buffer for capture
 */
#define STATIS_3DNR_BUF_SIZE (2 * (4672 + 3504))
#define STATIS_3DNR_BUF_NUM 8

#define STATIS_ISP_HIST2_BUF_SIZE   (256 * 4)
#define STATIS_ISP_HIST2_BUF_NUM 8

enum SCINFO_COLOR_ORDER {
	COLOR_ORDER_RG = 0,
	COLOR_ORDER_GR,
	COLOR_ORDER_GB,
	COLOR_ORDER_BG
};

enum aem_mode {
	AEM_MODE_SINGLE = 0,
	AEM_MODE_MULTI,
};

enum afm_mode {
	AFL_MODE_SINGLE = 0,
	AFL_MODE_MULTI,
};

enum isp_irq_done_id {
	IRQ_DCAM_SOF,
	IRQ_RAW_PROC_DONE,
	IRQ_RAW_PROC_TIMEOUT,
	IRQ_DCAM_SN_EOF,
	IRQ_MAX_DONE,
};

enum isp_statis_buf_type {
	STATIS_INIT = 0,
	STATIS_AEM,
	STATIS_AFM,
	STATIS_AFL,
	STATIS_HIST,
	STATIS_PDAF,
	STATIS_EBD,
	STATIS_3DNR,
	STATIS_HIST2,
};

enum isp_dev_capability {
	ISP_CAPABILITY_CONTINE_SIZE,
	ISP_CAPABILITY_TIME,
};

enum dcam_block {
	DCAM_ISP_BLOCK_MASK = (1 << 8),
	DCAM_BLOCK_BASE = (0 << 8),
	DCAM_BLOCK_BLC = DCAM_BLOCK_BASE,
	DCAM_BLOCK_RGBG,
	DCAM_BLOCK_RGBG_DITHER,
	DCAM_BLOCK_PDAF,
	DCAM_BLOCK_LSC,
	DCAM_BLOCK_BAYERHIST,
	DCAM_BLOCK_AEM,
	DCAM_BLOCK_AFL,
	DCAM_BLOCK_AWBC,
	DCAM_BLOCK_BPC,
	DCAM_BLOCK_3DNR_ME,
	DCAM_BLOCK_AFM,
	DCAM_BLOCK_TOTAL,

	ISP_BLOCK_BASE = (1 << 8),
	ISP_BLOCK_BCHS = ISP_BLOCK_BASE,
	ISP_BLOCK_CCE,
	ISP_BLOCK_CDN,
	ISP_BLOCK_CFA,
	ISP_BLOCK_CMC,
	ISP_BLOCK_EDGE,
	ISP_BLOCK_GAMMA,
	ISP_BLOCK_GRGB,
	ISP_BLOCK_HIST2,
	ISP_BLOCK_HSV,
	ISP_BLOCK_IIRCNR,
	ISP_BLOCK_LTM,
	ISP_BLOCK_NLM,
	ISP_BLOCK_POST_CDN,
	ISP_BLOCK_PRE_CDN,
	ISP_BLOCK_PSTRZ,
	ISP_BLOCK_UVD,
	ISP_BLOCK_YGAMMA,
	ISP_BLOCK_YNR,
	ISP_BLOCK_YRANDOM,
	ISP_BLOCK_NOISEFILTER,
	ISP_BLOCK_3DNR,
	ISP_BLOCK_TOTAL,
};


enum dcam_blc_property {
	DCAM_PRO_BLC_BLOCK,
};

enum dcam_gain_property {
	DCAM_PRO_GAIN_BLOCK,
	DCAM_PRO_GAIN_DITHER_BLOCK,
};

enum dcam_lsc_property {
	DCAM_PRO_LSC_BLOCK,
};

enum dcam_bayerhist_property {
	DCAM_PRO_BAYERHIST_BLOCK,
};

enum dcam_aem_property {
	DCAM_PRO_AEM_BYPASS,
	DCAM_PRO_AEM_MODE,
	DCAM_PRO_AEM_WIN,
	DCAM_PRO_AEM_SKIPNUM,
	DCAM_PRO_AEM_RGB_THR,
};

enum dcam_afl_property {
	DCAM_PRO_AFL_BLOCK,
	DCAM_PRO_AFL_BYPASS,
};

enum dcam_awbc_property {
	DCAM_PRO_AWBC_BLOCK,
	DCAM_PRO_AWBC_GAIN,
	DCAM_PRO_AWBC_BYPASS,
};

enum dcam_bpc_property {
	DCAM_PRO_BPC_BLOCK,
	DCAM_PRO_BPC_MAP,
	DCAM_PRO_BPC_HDR_PARAM,
	DCAM_PRO_BPC_PPE_PARAM,
};

enum dcam_3dnr_me_property {
	DCAM_PRO_3DNR_ME,
};

enum dcam_afm_property {
	DCAM_PRO_AFM_BYPASS,
	DCAM_PRO_AFM_BLOCK,
	DCAM_PRO_AFM_WIN,
	DCAM_PRO_AFM_WIN_NUM,
	DCAM_PRO_AFM_MODE,
	DCAM_PRO_AFM_SKIPNUM,
	DCAM_PRO_AFM_CROP_EB,
	DCAM_PRO_AFM_CROP_SIZE,
	DCAM_PRO_AFM_DONE_TILENUM,
};

enum dcam_pdaf_property {
	DCAM_PRO_PDAF_BLOCK,
	DCAM_PRO_PDAF_BYPASS,
	DCAM_PRO_PDAF_SET_MODE,
	DCAM_PRO_PDAF_SET_SKIP_NUM,
	DCAM_PRO_PDAF_SET_ROI,
	DCAM_PRO_PDAF_SET_PPI_INFO,
	DCAM_PRO_PDAF_TYPE1_BLOCK,
	DCAM_PRO_PDAF_TYPE2_BLOCK,
	DCAM_PRO_PDAF_TYPE3_BLOCK,
	DCAM_PRO_DUAL_PDAF_BLOCK,
};

enum isp_bchs_property {
	ISP_PRO_BCHS_BLOCK,
	ISP_PRO_BCHS_BRIGHT,
	ISP_PRO_BCHS_CONTRAST,
	ISP_PRO_BCHS_SATUATION,
	ISP_PRO_BCHS_HUE,
};

enum isp_cce_property {
	ISP_PRO_CCE_BLOCK,
};

enum isp_cdn_property {
	ISP_PRO_CDN_BLOCK,
};

enum isp_cfa_property {
	ISP_PRO_CFA_BLOCK,
};

enum isp_cmc_property {
	ISP_PRO_CMC_BLOCK,
};

enum isp_edge_property {
	ISP_PRO_EDGE_BLOCK,
};

enum isp_gamma_property {
	ISP_PRO_GAMMA_BLOCK,
};

enum isp_grgb_property {
	ISP_PRO_GRGB_BLOCK,
};

enum isp_hist2_property {
	ISP_PRO_HIST2_BLOCK,
};

enum isp_hsv_property {
	ISP_PRO_HSV_BLOCK,
};

enum isp_iircnr_property {
	ISP_PRO_IIRCNR_BLOCK,
};

enum isp_nlm_property {
	ISP_PRO_NLM_BLOCK,
	ISP_PRO_NLM_IMBLANCE,
};

enum isp_post_cdn_property {
	ISP_PRO_POST_CDN_BLOCK,
};

enum isp_pre_cdn_property {
	ISP_PRO_PRE_CDN_BLOCK,
};

enum isp_pstrz_property {
	ISP_PRO_POSTERIZE_BLOCK,
};

enum isp_uvd_property {
	ISP_PRO_UVD_BLOCK,
};

enum isp_ygamma_property {
	ISP_PRO_YGAMMA_BLOCK,
};

enum isp_ynr_property {
	ISP_PRO_YNR_BLOCK,
};

enum isp_yrandom_property {
	ISP_PRO_YRANDOM_BLOCK,
};

enum isp_noise_filter_property {
	ISP_PRO_NOISE_FILTER_BLOCK,
};

enum isp_3dnr_property {
	ISP_PRO_3DNR_BLOCK,
};

enum isp_ltm_property {
	ISP_PRO_LTM_BLOCK,
	ISP_PRO_LTM_PRE_PARAM,
	ISP_PRO_LTM_CAP_PARAM,
};

enum cam_pm_scene {
	PM_SCENE_PRE,
	PM_SCENE_CAP,
	PM_SCENE_VID,
	PM_SCENE_MAX,
};

struct isp_io_param {
	uint32_t scene_id;
	uint32_t sub_block;
	uint32_t property;
	void  __user  *property_param;
};

struct isp_addr {
	unsigned long	chn0;
	unsigned long	chn1;
	unsigned long	chn2;
};

struct isp_img_size {
	uint32_t width;
	uint32_t height;
};

struct isp_img_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct isp_img_offset {
	uint32_t x;
	uint32_t y;
	uint32_t Z;
};

struct isp_coord {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
};

struct store_border {
	uint32_t up_border;
	uint32_t down_border;
	uint32_t left_border;
	uint32_t right_border;
};

struct dcam_dev_blc_info {
	uint32_t bypass;
	uint32_t r;
	uint32_t b;
	uint32_t gr;
	uint32_t gb;
};

struct dcam_dev_rgb_gain_info {
	uint32_t  bypass;
	uint32_t  global_gain;
	uint32_t  r_gain;
	uint32_t  g_gain;
	uint32_t  b_gain;
};

struct dcam_dev_rgb_dither_info {
	uint32_t random_bypass;
	uint32_t random_mode;
	uint32_t seed;
	uint32_t range;
	uint32_t r_offset;
	uint32_t r_shift;
	uint32_t takebit[8];
};

struct dcam_dev_lsc_info {
	uint32_t bypass;
	uint32_t update_all;
	uint32_t grid_width;
	uint32_t grid_x_num;
	uint32_t grid_y_num;
	uint32_t grid_num_t;
	uint32_t gridtab_len;
	uint32_t weight_num;
	void __user * grid_tab;
	void __user * weight_tab;
};

struct dcam_dev_hist_info {
	uint32_t hist_bypass;
	uint32_t hist_skip_num;
	uint32_t hist_mul_enable;
	uint32_t hist_mode_sel;
	uint32_t hist_initial_clear;
	uint32_t hist_skip_num_clr;
	uint32_t hist_sgl_start;
	uint32_t bayer_hist_sty;
	uint32_t bayer_hist_stx;
	uint32_t bayer_hist_endy;
	uint32_t bayer_hist_endx;
	uint32_t bayer_hist_hdr_en;
	uint32_t bayer_hist_hdr_zigzag_pattern;
};

struct dcam_dev_aem_win {
	uint32_t offset_x;
	uint32_t offset_y;
	uint32_t blk_width;
	uint32_t blk_height;
	uint32_t blk_num_x;
	uint32_t blk_num_y;
};

struct thr_info{
	uint32_t low_thr;
	uint32_t high_thr;
};

struct dcam_dev_aem_thr {
	uint32_t aem_hdr_en;
	struct thr_info  aem_r_thr;
	struct thr_info  aem_g_thr;
	struct thr_info  aem_b_thr;
	struct thr_info  aem_short_r_thr;
	struct thr_info  aem_short_g_thr;
	struct thr_info  aem_short_b_thr;
};

struct dcam_dev_afl_info {
	uint32_t bayer2y_chanel;
	uint32_t bayer2y_mode;
	uint32_t bayer2y_bypass;
};

/* not used now, just for compiling. */
struct isp_dev_anti_flicker_info {
	uint32_t bypass;
	uint32_t mode;
	uint32_t skip_frame_num;
	uint32_t line_step;
	uint32_t frame_num;
	uint32_t vheight;
	uint32_t start_col;
	uint32_t end_col;
	uint32_t afl_total_num;
	struct isp_img_size img_size;
};

/*anti flicker */
struct isp_dev_anti_flicker_new_info {
	uint32_t bypass;
	uint32_t bayer2y_mode;
	uint32_t bayer2y_chanel;
	uint32_t mode;
	uint32_t skip_frame_num;
	uint32_t afl_stepx;
	uint32_t afl_stepy;
	uint32_t frame_num;
	uint32_t start_col;
	uint32_t end_col;
	uint32_t step_x_region;
	uint32_t step_y_region;
	uint32_t step_x_start_region;
	uint32_t step_x_end_region;
	uint32_t afl_glb_total_num;
	uint32_t afl_region_total_num;
	struct isp_img_size img_size;
};

struct img_rgb_info {
	uint32_t r;
	uint32_t b;
	uint32_t gr;
	uint32_t gb;
};

struct dcam_dev_awbc_info {
	uint32_t awbc_bypass;
	struct img_rgb_info gain;
	struct img_rgb_info thrd;
	struct img_rgb_info gain_offset;
};

struct dcam_bpc_rawhdr_info{
	uint32_t zzbpc_hdr_ratio;
	uint32_t zzbpc_hdr_ratio_inv;
	uint32_t zzbpc_hdr_2badpixel_en;

	uint32_t zzbpc_long_over_th;
	uint32_t zzbpc_short_over_th;
	uint32_t zzbpc_over_expo_num;

	uint32_t zzbpc_long_under_th;
	uint32_t zzbpc_short_under_th;
	uint32_t zzbpc_under_expo_num;

	uint32_t zzbpc_flat_th;
	uint32_t zzbpc_edgeratio_rd;
	uint32_t zzbpc_edgeratio_hv;

	uint32_t zzbpc_kmin_under_expo;
	uint32_t zzbpc_kmax_under_expo;
	uint32_t zzbpc_kmin_over_expo;
	uint32_t zzbpc_kmax_over_expo;
};

struct dcam_bpc_ppi_info{
	uint32_t ppi_bypass;
	uint32_t ppi_upperbound_r;
	uint32_t ppi_upperbound_b;
	uint32_t ppi_upperbound_gr;
	uint32_t ppi_upperbound_gb;
	uint32_t ppi_blc_r;
	uint32_t ppi_blc_b;
	uint32_t ppi_blc_gr;
	uint32_t ppi_blc_gb;
	uint32_t ppi_phase_map_corr_en;
	uint16_t ppi_l_gain_map[PDAF_PPI_GAIN_MAP_LEN];
	uint16_t ppi_r_gain_map[PDAF_PPI_GAIN_MAP_LEN];
};

struct dcam_dev_bpc_info {
	uint32_t bpc_bypass;
	uint32_t bpc_double_bypass;
	uint32_t bpc_three_bypass;
	uint32_t bpc_four_bypass;
	uint32_t bpc_mode;
	uint32_t bpc_is_mono_sensor;
	uint32_t bpc_ppi_en;
	uint32_t bpc_edge_hv_mode;
	uint32_t bpc_edge_rd_mode;
	uint32_t bpc_hdr_en;
	uint32_t bpc_pos_out_en;
	uint32_t bpc_map_clr_en;
	uint32_t bpc_rd_max_len_sel;
	uint32_t bpc_wr_max_len_sel;
	uint32_t bpc_blk_mode;
	uint32_t bpc_mod_en;
	uint32_t bpc_cg_dis;

	uint32_t bpc_four_badpixel_th[4];
	uint32_t bpc_three_badpixel_th[4];
	uint32_t bpc_double_badpixel_th[4];

	uint32_t bpc_texture_th;
	uint32_t bpc_flat_th;
	uint32_t bpc_shift[3];

	uint32_t bpc_edgeratio_hv;
	uint32_t bpc_edgeratio_rd;

	uint32_t bpc_highoffset;
	uint32_t bpc_lowoffset;
	uint32_t bpc_highcoeff;
	uint32_t bpc_lowcoeff;

	uint32_t bpc_mincoeff;
	uint32_t bpc_maxcoeff;

	uint32_t bpc_intercept_b[8];
	uint32_t bpc_slope_k[8];
	uint32_t bpc_lut_level[8];

	uint32_t bad_pixel_num;
	uint32_t bpc_map_addr;
	uint32_t bpc_bad_pixel_pos_out_addr;
	uint32_t bpc_last_waddr;
};

struct dcam_dev_3dnr_me {
	uint32_t bypass;
	uint32_t nr3_channel_sel;
	uint32_t nr3_project_mode;
};

struct thrd_min_max {
	uint32_t min;
	uint32_t max;
};


struct dcam_dev_afm_info {
	uint32_t  bypass;
	uint32_t  afm_mode_sel;
	uint32_t  afm_mul_enable;
	uint32_t  afm_skip_num;
	uint32_t  afm_skip_num_clr;
	uint32_t  afm_sgl_start;
	uint32_t  afm_done_tile_num_x;
	uint32_t  afm_done_tile_num_y;
	uint32_t  afm_lum_stat_chn_sel;
	uint32_t  afm_iir_enable;
	uint32_t  afm_cg_dis;
	uint32_t  afm_fv1_shift;
	uint32_t  afm_fv0_shift;
	uint32_t  afm_clip_en1;
	uint32_t  afm_clip_en0;
	uint32_t  afm_center_weight;
	uint32_t  afm_denoise_mode;
	uint32_t  afm_channel_sel;
	uint32_t  afm_crop_eb;
	uint16_t  afm_iir_g0;
	uint16_t  afm_iir_g1;
	uint16_t  afm_iir_c[10];
	struct thrd_min_max afm_fv0_th;
	struct thrd_min_max afm_fv1_th;
	uint16_t  afm_fv1_coeff[4][9];
};

struct dcam_dev_vc2_control {
	uint32_t bypass;
	uint32_t vch2_vc;
	uint32_t vch2_data_type;
	uint32_t vch2_mode;
};


struct isp_dev_brightness_info {
	uint32_t bypass;
	uint32_t factor;
};

struct isp_dev_contrast_info {
	uint32_t bypass;
	uint32_t factor;
};

struct isp_dev_csa_info {
	uint32_t bypass;
	uint32_t csa_factor_u;
	uint32_t csa_factor_v;
};

struct isp_dev_hue_info {
	uint32_t bypass;
	uint32_t hua_cos_value;
	uint32_t hua_sin_value;
};

struct isp_dev_bchs_info {
	uint32_t bchs_bypass;
	uint32_t cnta_en;
	uint32_t brta_en;
	uint32_t hua_en;
	uint32_t csa_en;
	uint32_t csa_factor_u;
	uint32_t csa_factor_v;
	uint32_t hua_cos_value;
	uint32_t hua_sina_value;
	uint32_t brta_factor;
	uint32_t cnta_factor;
};

struct isp_dev_cce_info {
	uint32_t bypass;
	uint16_t matrix[9];
	uint16_t y_offset;
	uint16_t u_offset;
	uint16_t v_offset;
};

struct isp_dev_cdn_info {
	uint32_t bypass;
	uint32_t filter_bypass;
	uint32_t median_writeback_en;
	uint32_t median_mode;
	uint32_t gaussian_mode;
	uint32_t median_thr;
	uint32_t median_thru0;
	uint32_t median_thru1;
	uint32_t median_thrv0;
	uint32_t median_thrv1;
	uint32_t rangewu[31];
	uint32_t rangewv[31];
	uint32_t level;
};

struct isp_dev_cfa_info {
	uint32_t bypass;
	uint32_t css_bypass;
	uint32_t grid_thr;
	uint32_t min_grid_new;
	uint32_t grid_gain_new;
	uint32_t strong_edge_thr;
	uint32_t uni_dir_intplt_thr_new;
	uint32_t weight_control_bypass;
	uint32_t cdcr_adj_factor;
	uint32_t smooth_area_thr;
	uint32_t readblue_high_sat_thr;
	uint32_t grid_dir_weight_t1;
	uint32_t grid_dir_weight_t2;
	uint32_t round_diff_03_thr;
	uint32_t low_lux_03_thr;
	uint32_t round_diff_12_thr;
	uint32_t low_lux_12_thr;
	uint32_t css_weak_edge_thr;
	uint32_t css_edge_thr;
	uint32_t css_texture1_thr;
	uint32_t css_texture2_thr;
	uint32_t css_uv_val_thr;
	uint32_t css_uv_diff_thr;
	uint32_t css_gray_thr;
	uint32_t css_pix_similar_thr;
	uint32_t css_green_edge_thr;
	uint32_t css_green_weak_edge_thr;
	uint32_t css_green_tex1_thr;
	uint32_t css_green_tex2_thr;
	uint32_t css_green_flat_thr;
	uint32_t css_edge_corr_ratio_r;
	uint32_t css_edge_corr_ratio_b;
	uint32_t css_text1_corr_ratio_r;
	uint32_t css_text1_corr_ratio_b;
	uint32_t css_text2_corr_ratio_r;
	uint32_t css_text2_corr_ratio_b;
	uint32_t css_flat_corr_ratio_r;
	uint32_t css_flat_corr_ratio_b;
	uint32_t css_wedge_corr_ratio_r;
	uint32_t css_wedge_corr_ratio_b;
	uint32_t css_alpha_for_tex2;
	uint32_t css_skin_u_top[2];
	uint32_t css_skin_u_down[2];
	uint32_t css_skin_v_top[2];
	uint32_t css_skin_v_down[2];
};

struct cmc_matrix {
	uint16_t val[9];
};

struct isp_dev_cmc10_info {
	uint32_t bypass;
	struct cmc_matrix matrix;
};

struct edge_pn_config {
	uint32_t p;
	uint32_t n;
};

struct isp_dev_edge_info_v2 {
	uint32_t bypass;
	uint32_t flat_smooth_mode;
	uint32_t edge_smooth_mode;
	struct edge_pn_config ee_str_d;
	uint32_t mode;
	struct edge_pn_config ee_incr_d;
	struct edge_pn_config ee_edge_thr_d;
	struct edge_pn_config ee_corner_sm;
	struct edge_pn_config ee_corner_gain;
	struct edge_pn_config ee_corner_th;
	uint32_t ee_corner_cor;
	uint32_t ee_cv_t[4];
	struct edge_pn_config ee_cv_clip;
	uint32_t ee_cv_r[3];
	uint32_t ipd_enable; /*ipd_bypass in v1 */
	uint32_t ipd_mask_mode;
	struct edge_pn_config ipd_less_thr;
	uint32_t ipd_smooth_en;
	struct edge_pn_config ipd_smooth_mode;
	struct edge_pn_config ipd_flat_thr;
	struct edge_pn_config ipd_eq_thr;
	struct edge_pn_config ipd_more_thr;
	struct edge_pn_config ipd_smooth_edge_thr;
	struct edge_pn_config ipd_smooth_edge_diff;
	uint32_t ee_ratio_hv_3;
	uint32_t ee_ratio_hv_5;
	uint32_t ee_ratio_diag_3;
	uint32_t ee_weight_hv2diag;
	uint32_t ee_gradient_computation_type;
	uint32_t ee_weight_diag2hv;
	uint32_t ee_gain_hv_t[2][4];
	uint32_t ee_gain_hv_r[2][3];
	uint32_t ee_ratio_diag_5;
	uint32_t ee_gain_diag_t[2][4];
	uint32_t ee_gain_diag_r[2][3];
	uint32_t ee_lum_t[4];
	uint32_t ee_lum_r[3];
	uint32_t ee_pos_t[4];
	uint32_t ee_pos_r[3];
	uint32_t ee_pos_c[3];
	uint32_t ee_neg_t[4];
	uint32_t ee_neg_r[3];
	uint32_t ee_neg_c[3];
	uint32_t ee_freq_t[4];
	uint32_t ee_freq_r[3];

	/* new added below */
	uint32_t ee_new_pyramid_en;
	uint32_t ee_old_gradient_en;
	uint32_t  ee_ratio_old_gradient;
	uint32_t  ee_ratio_new_pyramid;
	uint32_t  ee_offset_thr_layer_curve_pos[3][4];
	uint32_t  ee_offset_ratio_layer_curve_pos[3][3];
	uint32_t  ee_offset_clip_layer_curve_pos[3][3];
	uint32_t  ee_offset_thr_layer_curve_neg[3][4];
	uint32_t  ee_offset_ratio_layer_curve_neg[3][3];
	uint32_t  ee_offset_clip_layer_curve_neg[3][3];
	uint32_t  ee_offset_ratio_layer_lum_curve[3][3];
	uint32_t  ee_offset_ratio_layer_freq_curve[3][3];
};

struct isp_dev_gamma_info {
	uint32_t bypass;
	uint8_t gain_r[ISP_FRGB_GAMMA_PT_NUM];
	uint8_t gain_g[ISP_FRGB_GAMMA_PT_NUM];
	uint8_t gain_b[ISP_FRGB_GAMMA_PT_NUM];
};

struct grgb_param {
	uint32_t curve_t[3][4];
	uint32_t curve_r[3][3];
};

struct isp_dev_grgb_info {
	uint32_t bypass;
	uint32_t diff_thd;
	uint32_t hv_edge_thr;
	uint32_t check_sum_clr;
	uint32_t slash_edge_thr;
	uint32_t slash_flat_thr;
	uint32_t gr_ratio;
	uint32_t hv_flat_thr;
	uint32_t gb_ratio;
	struct grgb_param lum;
	struct grgb_param frez;
};

struct isp_dev_hist2_info {
	uint32_t bypass;
	uint32_t mode;
	uint32_t skip_num;
	uint32_t channel_sel;
	struct isp_coord hist_roi;
	uint32_t skip_num_clr;
};


struct isp_dev_hsv_curve_info {
	uint16_t  s_curve[5][4];
	uint16_t  v_curve[5][4];
	uint16_t  r_s[5][2];
	uint16_t  r_v[5][2];
	uint32_t  hrange_left[5];
	uint32_t  hrange_right[5];
};

struct isp_dev_hsv_info_v2 {
	uint32_t  bypass;
	struct isp_dev_hsv_curve_info curve_info;
	uint32_t size;
	/* uint64_t for 32bits/64bits userspace/kernel compatable*/
	uint64_t hsv_table_addr;
};

struct isp_dev_iircnr_info {
	uint32_t bypass;
	uint32_t mode;
	uint32_t uv_th;
	uint32_t y_max_th;
	uint32_t y_min_th;
	uint32_t uv_dist;
	uint32_t uv_pg_th;
	uint32_t sat_ratio;
	uint32_t uv_low_thr2;
	uint32_t uv_low_thr1;
	uint32_t ymd_u;
	uint32_t ymd_v;
	uint32_t uv_s_th;
	uint32_t slope_y_0;
	uint32_t y_th;
	uint32_t alpha_low_u;
	uint32_t alpha_low_v;
	uint32_t middle_factor_y_0;
	uint32_t uv_high_thr2_0;
	uint32_t ymd_min_u;
	uint32_t ymd_min_v;
	uint32_t uv_low_thr[7][2];
	uint32_t y_edge_thr_max[8];
	uint32_t y_edge_thr_min[8];
	uint32_t uv_high_thr2[7];
	uint32_t slope_y[7];
	uint32_t middle_factor_y[7];
	uint32_t middle_factor_uv[8];
	uint32_t slope_uv[8];
	uint32_t pre_uv_th;
	uint32_t css_lum_thr;
	uint32_t uv_diff_thr;
};

struct isp_dev_nlm_imblance {
	uint32_t nlm_imblance_en;
	uint32_t nlm_imblance_hv_edge_thr;
	uint32_t nlm_imblance_slash_edge_thr;
	uint32_t nlm_imblance_hv_flat_thr;
	uint32_t nlm_imblance_slash_flat_thr;
	uint32_t nlm_imblance_flag3_grid;
	uint32_t nlm_imblance_flag3_lum;
	uint32_t nlm_imblance_flag3_frez;
	uint32_t nlm_imblance_S_baohedu1;
	uint32_t nlm_imblance_S_baohedu2;
	uint32_t nlm_imblance_lum1_flag2_r;
	uint32_t nlm_imblance_lum1_flag4_r;
	uint32_t nlm_imblance_lum1_flag0_rs;
	uint32_t nlm_imblance_lum1_flag0_r;
	uint32_t nlm_imblance_lum1_flag1_r;
	uint32_t nlm_imblance_lum2_flag2_r;
	uint32_t nlm_imblance_lum2_flag4_r;
	uint32_t nlm_imblance_lum2_flag0_rs;
	uint32_t nlm_imblance_lum2_flag0_r;
	uint32_t nlm_imblance_lum2_flag1_r;
	uint32_t nlm_imblance_lum3_flag2_r;
	uint32_t nlm_imblance_lum3_flag4_r;
	uint32_t nlm_imblance_lum3_flag0_rs;
	uint32_t nlm_imblance_lum3_flag0_r;
	uint32_t nlm_imblance_lum3_flag1_r;
	uint32_t nlm_imblance_lumth1;
	uint32_t nlm_imblance_lumth2;
	uint32_t nlm_imblance_flag12_frezthr;
	uint32_t nlm_imblance_diff;
	uint32_t nlm_imblance_faceRmin;
	uint32_t nlm_imblance_faceRmax;
	uint32_t nlm_imblance_faceBmin;
	uint32_t nlm_imblance_faceBmax;
	uint32_t nlm_imblance_faceGmin;
	uint32_t nlm_imblance_faceGmax;
};

struct lum_flat_param {
	uint16_t thresh;
	uint16_t match_count;
	uint16_t inc_strength;
	uint16_t reserved;
};

struct isp_dev_nlm_info_v2 {
	uint32_t bypass;
	uint32_t imp_opt_bypass;
	uint32_t flat_opt_bypass;
	uint32_t direction_mode_bypass;
	uint32_t first_lum_byapss;
	uint32_t simple_bpc_bypass;
	uint32_t dist_mode;
	uint32_t radius_bypass;
	uint32_t update_flat_thr_bypass;
	uint8_t w_shift[3];
	uint8_t pack_reserved;
	uint32_t direction_cnt_th;
	uint32_t simple_bpc_lum_th;
	uint32_t simple_bpc_th;
	uint32_t lum_th0;
	uint32_t lum_th1;
	uint32_t diff_th;
	uint32_t tdist_min_th;
	uint16_t lut_w[72];
	struct lum_flat_param lum_flat[3][3];
	uint16_t lum_flat_addback0[3][4];
	uint16_t lum_flat_addback1[3][4];
	uint16_t lum_flat_addback_min[3][4];
	uint16_t lum_flat_addback_max[3][4];
	uint32_t lum_flat_dec_strenth[3];
	uint32_t vst_bypass;
	uint32_t ivst_bypass;
	uint32_t nlm_first_lum_flat_thresh_coef[3][3];
	uint32_t nlm_first_lum_flat_thresh_max[3][3];
	uint32_t nlm_radial_1D_center_x;
	uint32_t nlm_radial_1D_center_y;
	uint32_t nlm_radial_1D_radius_threshold;
	uint32_t nlm_radial_1D_bypass;
	uint32_t nlm_radial_1D_protect_gain_max;
	uint32_t nlm_radial_1D_radius_threshold_filter_ratio[3][4];
	uint32_t nlm_radial_1D_coef2[3][4];
	uint32_t nlm_radial_1D_protect_gain_min[3][4];

	uint32_t nlm_radial_1D_radius_threshold_factor;
	uint32_t nlm_radial_1D_radius_threshold_filter_ratio_factor[3][4];
	uint32_t radius_base;

	uint32_t nlm_direction_addback_mode_bypass;
	uint32_t nlm_first_lum_direction_addback[3][4];
	uint32_t nlm_first_lum_direction_addback_noise_clip[3][4];

	uint32_t vst_len;
	uint32_t ivst_len;
	/* uint64_t for 32bits/64bits userspace/kernel compatable*/
	uint64_t vst_table_addr;
	uint64_t ivst_table_addr;
};

struct cdn_thruv {
	uint16_t thru0;
	uint16_t thru1;
	uint16_t thrv0;
	uint16_t thrv1;
};

#pragma pack(push)
#pragma pack(4)
struct isp_dev_post_cdn_info {
	uint32_t bypass;
	uint32_t downsample_bypass;
	uint32_t mode;
	uint32_t writeback_en;
	uint32_t uvjoint;
	uint32_t median_mode;
	uint32_t adapt_med_thr;
	uint32_t uvthr0;
	uint32_t uvthr1;
	struct cdn_thruv thr_uv;
	uint8_t r_segu[2][7];
	uint8_t r_segv[2][7];
	uint8_t r_distw[15][5];
};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(4)
struct isp_dev_pre_cdn_info {
	uint32_t bypass;
	uint32_t mode;
	uint32_t median_writeback_en;
	uint32_t median_mode;
	uint32_t den_stren;
	uint32_t uv_joint;
	struct cdn_thruv median_thr_uv;
	uint32_t median_thr;
	uint32_t uv_thr;
	uint32_t y_thr;
	uint8_t r_segu[2][7];
	uint8_t r_segv[2][7];
	uint8_t r_segy[2][7];
	uint8_t r_distw[25];
};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(4)
struct isp_dev_posterize_info_v2 {
	uint32_t  bypass;
	uint32_t  sample_en;
	uint8_t pstrz_r_data[POSTERIZE_NUM];
	uint8_t pstrz_g_data[POSTERIZE_NUM];
	uint8_t pstrz_b_data[POSTERIZE_NUM];
};
#pragma pack(pop)

struct uvd_th {
	uint32_t th_h[2];
	uint32_t th_l[2];
};

struct isp_dev_uvd_info_v2 {
	uint32_t bypass;
	uint32_t chk_sum_clr_en;
	uint32_t lum_th_h_len;
	uint32_t lum_th_h;
	uint32_t lum_th_l_len;
	uint32_t lum_th_l;
	uint32_t chroma_ratio;
	uint32_t chroma_max_h;
	uint32_t chroma_max_l;
	struct uvd_th u_th;
	struct uvd_th v_th;
	uint32_t luma_ratio;
	uint32_t ratio_uv_min;
	uint32_t ratio_y_min[2];
	uint32_t ratio0;
	uint32_t ratio1;
	uint32_t y_th_l_len;
	uint32_t y_th_h_len;
	uint32_t uv_abs_th_len;
};

#pragma pack(push)
#pragma pack(4)
struct isp_dev_ygamma_info {
	uint32_t bypass;
	uint8_t gain[ISP_YUV_GAMMA_NUM];
};
#pragma pack(pop)

/* new ynr: all updated. */
struct isp_dev_ynr_info_v2 {
	uint32_t bypass;
	uint32_t l3_addback_enable;
	uint32_t l2_addback_enable;
	uint32_t l1_addback_enable;
	uint32_t l0_addback_enable;
	uint32_t l3_blf_en;
	uint32_t sal_enable;
	uint32_t l3_wv_nr_enable;
	uint32_t l2_wv_nr_enable;
	uint32_t l1_wv_nr_enable;
	uint32_t blf_range_index;
	uint32_t blf_dist_weight2;
	uint32_t blf_dist_weight1;
	uint32_t blf_dist_weight0;
	int32_t blf_range_s4;
	int32_t blf_range_s3;
	int32_t blf_range_s2;
	int32_t blf_range_s1;
	uint32_t coef_model;
	uint32_t blf_range_s0_high;
	uint32_t blf_range_s0_mid;
	uint32_t blf_range_s0_low;
	uint32_t lum_thresh1;
	uint32_t lum_thresh0;
	uint32_t l1_wv_ratio2_low;
	uint32_t l1_wv_ratio1_low;
	uint32_t l1_soft_offset_low;
	uint32_t l1_wv_thr1_low;
	uint32_t l1_wv_ratio_d2_low;
	uint32_t l1_wv_ratio_d1_low;
	uint32_t l1_soft_offset_d_low;
	uint32_t l1_wv_thr_d1_low;
	uint32_t l1_wv_ratio2_mid;
	uint32_t l1_wv_ratio1_mid;
	uint32_t l1_soft_offset_mid;
	uint32_t l1_wv_thr1_mid;
	uint32_t l1_wv_ratio_d2_mid;
	uint32_t l1_wv_ratio_d1_mid;
	uint32_t l1_soft_offset_d_mid;
	uint32_t l1_wv_thr_d1_mid;
	uint32_t l1_wv_ratio2_high;
	uint32_t l1_wv_ratio1_high;
	uint32_t l1_soft_offset_high;
	uint32_t l1_wv_thr1_high;
	uint32_t l1_wv_ratio_d2_high;
	uint32_t l1_wv_ratio_d1_high;
	uint32_t l1_soft_offset_d_high;
	uint32_t l1_wv_thr_d1_high;
	uint32_t l2_wv_ratio2_low;
	uint32_t l2_wv_ratio1_low;
	uint32_t l2_soft_offset_low;
	uint32_t l2_wv_thr1_low;
	uint32_t l2_wv_ratio_d2_low;
	uint32_t l2_wv_ratio_d1_low;
	uint32_t l2_soft_offset_d_low;
	uint32_t l2_wv_thr_d1_low;
	uint32_t l2_wv_ratio2_mid;
	uint32_t l2_wv_ratio1_mid;
	uint32_t l2_soft_offset_mid;
	uint32_t l2_wv_thr1_mid;
	uint32_t l2_wv_ratio_d2_mid;
	uint32_t l2_wv_ratio_d1_mid;
	uint32_t l2_soft_offset_d_mid;
	uint32_t l2_wv_thr_d1_mid;
	uint32_t l2_wv_ratio2_high;
	uint32_t l2_wv_ratio1_high;
	uint32_t l2_soft_offset_high;
	uint32_t l2_wv_thr1_high;
	uint32_t l2_wv_ratio_d2_high;
	uint32_t l2_wv_ratio_d1_high;
	uint32_t l2_soft_offset_d_high;
	uint32_t l2_wv_thr_d1_high;
	uint32_t l3_wv_ratio2_low;
	uint32_t l3_wv_ratio1_low;
	uint32_t l3_soft_offset_low;
	uint32_t l3_wv_thr1_low;
	uint32_t l3_wv_ratio_d2_low;
	uint32_t l3_wv_ratio_d1_low;
	uint32_t l3_soft_offset_d_low;
	uint32_t l3_wv_thr_d1_low;
	uint32_t l3_wv_ratio2_mid;
	uint32_t l3_wv_ratio1_mid;
	uint32_t l3_soft_offset_mid;
	uint32_t l3_wv_thr1_mid;
	uint32_t l3_wv_ratio_d2_mid;
	uint32_t l3_wv_ratio_d1_mid;
	uint32_t l3_soft_offset_d_mid;
	uint32_t l3_wv_thr_d1_mid;
	uint32_t l3_wv_ratio2_high;
	uint32_t l3_wv_ratio1_high;
	uint32_t l3_soft_offset_high;
	uint32_t l3_wv_thr1_high;
	uint32_t l3_wv_ratio_d2_high;
	uint32_t l3_wv_ratio_d1_high;
	uint32_t l3_soft_offset_d_high;
	uint32_t l3_wv_thr_d1_high;
	uint32_t l3_wv_thr2_high;
	uint32_t l3_wv_thr2_mid;
	uint32_t l3_wv_thr2_low;
	uint32_t l2_wv_thr2_high;
	uint32_t l2_wv_thr2_mid;
	uint32_t l2_wv_thr2_low;
	uint32_t l1_wv_thr2_high;
	uint32_t l1_wv_thr2_mid;
	uint32_t l1_wv_thr2_low;
	uint32_t l3_wv_thr_d2_high;
	uint32_t l3_wv_thr_d2_mid;
	uint32_t l3_wv_thr_d2_low;
	uint32_t l2_wv_thr_d2_high;
	uint32_t l2_wv_thr_d2_mid;
	uint32_t l2_wv_thr_d2_low;
	uint32_t l1_wv_thr_d2_high;
	uint32_t l1_wv_thr_d2_mid;
	uint32_t l1_wv_thr_d2_low;
	uint32_t l1_addback_ratio;
	uint32_t l1_addback_clip;
	uint32_t l0_addback_ratio;
	uint32_t l0_addback_clip;
	uint32_t l3_addback_ratio;
	uint32_t l3_addback_clip;
	uint32_t l2_addback_ratio;
	uint32_t l2_addback_clip;
	uint32_t lut_thresh3;
	uint32_t lut_thresh2;
	uint32_t lut_thresh1;
	uint32_t lut_thresh0;
	uint32_t lut_thresh6;
	uint32_t lut_thresh5;
	uint32_t lut_thresh4;
	uint32_t sal_offset3;
	uint32_t sal_offset2;
	uint32_t sal_offset1;
	uint32_t sal_offset0;
	uint32_t sal_offset7;
	uint32_t sal_offset6;
	uint32_t sal_offset5;
	uint32_t sal_offset4;
	uint32_t sal_nr_str3;
	uint32_t sal_nr_str2;
	uint32_t sal_nr_str1;
	uint32_t sal_nr_str0;
	uint32_t sal_nr_str7;
	uint32_t sal_nr_str6;
	uint32_t sal_nr_str5;
	uint32_t sal_nr_str4;
	uint32_t start_row;
	uint32_t start_col;
	uint32_t center_y;
	uint32_t center_x;
	uint32_t dis_interval;
	uint32_t radius;
	uint32_t radius_factor;
	uint32_t max_radius;
	uint32_t max_radius_factor;
	uint32_t radius_base;
};

struct isp_dev_yrandom_info {
	uint32_t bypass;
	uint32_t mode;
	uint32_t seed;
	uint32_t offset;
	uint32_t shift;
	uint8_t takeBit[8];
};

struct isp_dev_noise_filter_info {
	uint32_t yrandom_bypass;
	uint32_t shape_mode;
	uint32_t filter_thr_mode;
	uint32_t yrandom_mode;
	uint32_t yrandom_init;
	uint32_t yrandom_seed[4];
	uint32_t takebit[8];
	uint32_t r_offset;
	uint32_t r_shift;
	uint32_t filter_thr;
	uint32_t cv_t[4];
	uint32_t cv_r[3];
	struct edge_pn_config  noise_clip;
};

struct isp_ltm_tile_num_minus1 {
	uint32_t tile_num_x;
	uint32_t tile_num_y;
};

struct isp_ltm_tile_size {
	uint32_t tile_width;
	uint32_t tile_height;
};

struct isp_ltm_clip_limit {
	uint32_t limit;
	uint32_t limit_min;
};

struct isp_dev_ltm_stat_info {
	uint32_t bypass; /* bypass */

	struct isp_ltm_tile_num_minus1 tile_num;
	struct isp_ltm_tile_size tile_size;
	struct isp_ltm_clip_limit tile_clip;

	uint32_t strength;
	uint32_t tile_num_auto;

	uint32_t text_point_thres; /* text_point_thres */
	uint32_t text_proportion; /* texture_proportion */
	uint32_t region_est_en; /* region_est_en */
	uint32_t binning_en;
};

struct isp_dev_ltm_map_info {
	uint32_t bypass; /* ltm map bypass */
	uint32_t ltm_map_video_mode;
};

struct isp_dev_ltm_info {
	struct isp_dev_ltm_stat_info ltm_stat;
	struct isp_dev_ltm_map_info ltm_map;
};


/*********************************************/
struct isp_rrgb {
	uint32_t r;
	uint32_t b;
	uint32_t gr;
	uint32_t gb;
};

struct isp_dev_pdaf_info {
	uint32_t bypass;
	uint32_t corrector_bypass;
	uint32_t phase_map_corr_en;
	struct isp_img_size block_size;
	uint32_t grid_mode;
	struct isp_coord win;
	struct isp_coord block;
	struct isp_rrgb gain_upperbound;
	uint32_t phase_txt_smooth;
	uint32_t phase_gfilter;
	uint32_t phase_flat_smoother;
	uint32_t hot_pixel_th[3];
	uint32_t dead_pixel_th[3];
	uint32_t flat_th;
	uint32_t edge_ratio_hv;
	uint32_t edge_ratio_rd;
	uint32_t edge_ratio_hv_rd;
	uint32_t phase_left_addr;
	uint32_t phase_right_addr;
	uint32_t phase_pitch;
	uint32_t pattern_pixel_is_right[PDAF_PPI_NUM];
	uint32_t pattern_pixel_row[PDAF_PPI_NUM];
	uint32_t pattern_pixel_col[PDAF_PPI_NUM];
	uint32_t gain_ori_left[2];
	uint32_t gain_ori_right[2];
	uint32_t extractor_bypass;
	uint32_t mode_sel;
	uint32_t skip_num;
	uint32_t phase_data_dword_num;
	struct isp_rrgb pdaf_blc;
	uint32_t data_ptr_left[2];
	uint32_t data_ptr_right[2];
};

struct pdaf_ppi_info {
	struct isp_img_size block_size;
	struct isp_coord block;
	uint32_t pd_pos_size;
	uint32_t pattern_pixel_is_right[PDAF_PPI_NUM];
	uint32_t pattern_pixel_row[PDAF_PPI_NUM];
	uint32_t pattern_pixel_col[PDAF_PPI_NUM];
};

struct pdaf_roi_info {
	struct isp_coord win;
	uint32_t phase_data_write_num;
};

struct dev_dcam_vc2_control {
	uint32_t bypass;
	uint32_t vch2_vc;
	uint32_t vch2_data_type;
	uint32_t vch2_mode;
};



struct isp_statis_buf_input {
	enum isp_statis_buf_type type;
	union {
		struct init {
			int32_t mfd;
			uint32_t  buf_size;
		} init_data;
		struct block {
			uint32_t hw_addr;
		} block_data;
	} u;
	uint64_t uaddr;
	uint64_t kaddr;
};

enum raw_proc_cmd {
	RAW_PROC_PRE,
	RAW_PROC_POST,
	RAW_PROC_DONE,
};

enum raw_proc_scene {
	RAW_PROC_SCENE_RAWCAP = 0,
	RAW_PROC_SCENE_HWSIM,
};

struct isp_raw_proc_info {
	enum raw_proc_cmd cmd;
	struct isp_img_size src_size;
	struct isp_img_size dst_size;
	uint32_t src_y_endian;
	uint32_t src_uv_endian;
	uint32_t dst_y_endian;
	uint32_t dst_uv_endian;
	uint32_t src_format;
	uint32_t src_pattern;
	uint32_t dst_format;
	int32_t fd_src;
	int32_t fd_dst0;
	int32_t fd_dst1;
	uint32_t src_offset;/*first bytes offset in buffer fd_src*/
	uint32_t dst0_offset;/*first bytes offset in buffer fd_dst0*/
	uint32_t dst1_offset;/*first bytes offset in buffer fd_dst1*/
	enum raw_proc_scene scene;
};

struct isp_3dnr_blend_info {
	uint32_t bypass;
	uint32_t filter_switch;
	uint32_t fusion_mode;
	uint32_t y_pixel_src_weight[4];
	uint32_t u_pixel_src_weight[4];
	uint32_t v_pixel_src_weight[4];
	uint32_t y_pixel_noise_threshold;
	uint32_t u_pixel_noise_threshold;
	uint32_t v_pixel_noise_threshold;
	uint32_t y_pixel_noise_weight;
	uint32_t u_pixel_noise_weight;
	uint32_t v_pixel_noise_weight;
	uint32_t threshold_radial_variation_u_range_min;
	uint32_t threshold_radial_variation_u_range_max;
	uint32_t threshold_radial_variation_v_range_min;
	uint32_t threshold_radial_variation_v_range_max;
	uint32_t y_threshold_polyline_0;
	uint32_t y_threshold_polyline_1;
	uint32_t y_threshold_polyline_2;
	uint32_t y_threshold_polyline_3;
	uint32_t y_threshold_polyline_4;
	uint32_t y_threshold_polyline_5;
	uint32_t y_threshold_polyline_6;
	uint32_t y_threshold_polyline_7;
	uint32_t y_threshold_polyline_8;
	uint32_t u_threshold_polyline_0;
	uint32_t u_threshold_polyline_1;
	uint32_t u_threshold_polyline_2;
	uint32_t u_threshold_polyline_3;
	uint32_t u_threshold_polyline_4;
	uint32_t u_threshold_polyline_5;
	uint32_t u_threshold_polyline_6;
	uint32_t u_threshold_polyline_7;
	uint32_t u_threshold_polyline_8;
	uint32_t v_threshold_polyline_0;
	uint32_t v_threshold_polyline_1;
	uint32_t v_threshold_polyline_2;
	uint32_t v_threshold_polyline_3;
	uint32_t v_threshold_polyline_4;
	uint32_t v_threshold_polyline_5;
	uint32_t v_threshold_polyline_6;
	uint32_t v_threshold_polyline_7;
	uint32_t v_threshold_polyline_8;
	uint32_t y_intensity_gain_polyline_0;
	uint32_t y_intensity_gain_polyline_1;
	uint32_t y_intensity_gain_polyline_2;
	uint32_t y_intensity_gain_polyline_3;
	uint32_t y_intensity_gain_polyline_4;
	uint32_t y_intensity_gain_polyline_5;
	uint32_t y_intensity_gain_polyline_6;
	uint32_t y_intensity_gain_polyline_7;
	uint32_t y_intensity_gain_polyline_8;
	uint32_t u_intensity_gain_polyline_0;
	uint32_t u_intensity_gain_polyline_1;
	uint32_t u_intensity_gain_polyline_2;
	uint32_t u_intensity_gain_polyline_3;
	uint32_t u_intensity_gain_polyline_4;
	uint32_t u_intensity_gain_polyline_5;
	uint32_t u_intensity_gain_polyline_6;
	uint32_t u_intensity_gain_polyline_7;
	uint32_t u_intensity_gain_polyline_8;
	uint32_t v_intensity_gain_polyline_0;
	uint32_t v_intensity_gain_polyline_1;
	uint32_t v_intensity_gain_polyline_2;
	uint32_t v_intensity_gain_polyline_3;
	uint32_t v_intensity_gain_polyline_4;
	uint32_t v_intensity_gain_polyline_5;
	uint32_t v_intensity_gain_polyline_6;
	uint32_t v_intensity_gain_polyline_7;
	uint32_t v_intensity_gain_polyline_8;
	uint32_t gradient_weight_polyline_0;
	uint32_t gradient_weight_polyline_1;
	uint32_t gradient_weight_polyline_2;
	uint32_t gradient_weight_polyline_3;
	uint32_t gradient_weight_polyline_4;
	uint32_t gradient_weight_polyline_5;
	uint32_t gradient_weight_polyline_6;
	uint32_t gradient_weight_polyline_7;
	uint32_t gradient_weight_polyline_8;
	uint32_t gradient_weight_polyline_9;
	uint32_t gradient_weight_polyline_10;
	uint32_t u_threshold_factor0;
	uint32_t u_threshold_factor1;
	uint32_t u_threshold_factor2;
	uint32_t u_threshold_factor3;
	uint32_t v_threshold_factor0;
	uint32_t v_threshold_factor1;
	uint32_t v_threshold_factor2;
	uint32_t v_threshold_factor3;
	uint32_t u_divisor_factor0;
	uint32_t u_divisor_factor1;
	uint32_t u_divisor_factor2;
	uint32_t u_divisor_factor3;
	uint32_t v_divisor_factor0;
	uint32_t v_divisor_factor1;
	uint32_t v_divisor_factor2;
	uint32_t v_divisor_factor3;
	uint32_t r1_circle;
	uint32_t r2_circle;
	uint32_t r3_circle;
	uint32_t r1_circle_factor;
	uint32_t r2_circle_factor;
	uint32_t r3_circle_factor;
	uint32_t r_circle_base;
};

struct isp_3dnr_fast_me {
	uint32_t nr3_channel_sel;
	uint32_t nr3_project_mode;
};

struct isp_dev_3dnr_info {
	struct isp_3dnr_fast_me fast_me;
	struct isp_3dnr_blend_info blend;
};


/************  for test only below ************** */
enum ch_property {
	PROP_PRE,
	PROP_CAP,
	PROP_VIDEO,
	PROP_FD,
	PROP_MAX
};

struct dev_test_info {
	uint32_t  dev; /* 0: isp, 1: dcam0, 2: dcam1 */

	/* channel desc  */
	uint32_t in_fmt;  /* forcc */
	uint32_t out_fmt;  /* forcc */
	enum ch_property prop;
	struct isp_img_size input_size;
	struct isp_img_size output_size;

	/* buffer desc */
	uint32_t iommu_en;
	uint32_t inbuf_fd;
	uint32_t inbuf_kaddr[2];
	uint32_t outbuf_fd;
	uint32_t outbuf_kaddr[2];
};
#endif
