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
#ifndef _ISP_DRV_H_
#define _ISP_DRV_H_
#include <fcntl.h>
#include <errno.h>
#include "sprd_isp_k.h"
#include "sprd_img.h"
#include "cmr_types.h"
#include "sensor_raw.h"
#include "isp_mw.h"

#ifndef LOCAL_INCLUDE_ONLY
#error "Hi, This is only for camdrv."
#endif

#define ISP_DRV_SLICE_WIN_NUM 0x18

struct isp_file {
	cmr_s32 fd;
	cmr_u32 chip_id;
	cmr_u32 isp_id;
	void *dcam_lsc_vaddr;
};

struct isp_aem_stats_info {
	cmr_u8 stats_mode;
	cmr_u32 mode;
	cmr_u32 skip_num;
};

struct isp_aem_win_info {
	struct isp_img_offset offset;
	struct isp_img_size size;
};

struct isp_afm_info {
	cmr_u32 bypass;
	cmr_u32 skip_num;
	cmr_u32 cur_envi;
};

struct isp_u_blocks_info {
	cmr_u32 scene_id;
	union {
		cmr_u32 mode;
		cmr_u32 skip_num;
		cmr_u32 bypass;
		cmr_u32 enable;
		cmr_u32 type;
		cmr_u32 phy_addr;
		cmr_u32 info;
		cmr_u32 factor;
		void *block_info;
		void *shift;
		struct isp_img_offset offset;
		struct isp_img_size size;
		/*rgb gain block*/
		cmr_u32 rgb_gain_coeff;
		/*gamma block*/
		cmr_u16 *node_ptr;
		/*cmc block*/
		cmr_u16 *matrix_ptr;
		/*fetch block*/
		cmr_u32 fetch_start;
		/*lsc block*/
		cmr_u32 flag;
		/*common block*/
		cmr_u32 pos;
		cmr_u32 auto_shadow;
		cmr_u32 shadow_done;
		/*nlc block*/
		cmr_u16 *r_node_ptr;
		cmr_u16 *g_node_ptr;
		cmr_u16 *b_node_ptr;
		cmr_u16 *l_node_ptr;
		/*aem block*/
		void *statis;
		struct isp_aem_stats_info stats_info;
		struct isp_aem_win_info  win_info;
		/*afm block*/
		cmr_u32 clear;
		cmr_u32 sel_filter;
		cmr_u32 *win_num;
		void *win_range;
		void *thr_rgb;
		struct afm_subfilter subfilter;
		struct afm_shift afm_shift;
		struct isp_afm_info afm_info;
#if 0
		struct sobel_thrd sobel_thrd;
		struct spsmd_thrd spsmd_thrd;
#endif
		/*bpc block*/
		cmr_u32 addr;
		cmr_u32 pixel_num;
		struct isp_bpc_common bpc_param;
		struct isp_bpc_thrd bpc_thrd;
		/*edge block*/
		struct isp_edge_thrd edge_thrd;
		/*pdaf block*/
		void *roi_info;
		void *ppi_info;
		/*cce block*/
		struct isp_cce_shift cce_shift;
		struct isp_cce_uvc *cce_uvc;
		struct isp_cce_uvd *cce_uvd;
		struct isp_cce_matrix_tab *matrix_tab;
		/*grgb block*/
		struct isp_grgb_thrd grgb_thrd;
		/*cfa block*/
		struct isp_cfa_thrd cfa_thrd;
		/*hist2 block*/
		void *hist2_roi;
		/*awbc block*/
		struct isp_awbc_rgb awbc_rgb;
		/*binnging4awb block*/
		cmr_u32 endian;
		cmr_u32 *buf_id;
		struct isp_scaling_ratio scaling_ratio;
		struct isp_b4awb_phys phys_addr;
	};
};

struct isp_statis_mem_info {
	cmr_uint isp_statis_mem_size;
	cmr_uint isp_statis_mem_num;
	cmr_uint isp_statis_k_addr[2];
	cmr_uint isp_statis_u_addr;
	cmr_uint isp_statis_alloc_flag;
	cmr_s32 statis_mfd;
	cmr_s32 statis_buf_dev_fd;

	cmr_uint isp_lsc_mem_size;
	cmr_uint isp_lsc_mem_num;
	cmr_uint isp_lsc_physaddr;
	cmr_uint isp_lsc_virtaddr;
	cmr_s32 lsc_mfd;

	void *buffer_client_data;
	void *cb_of_malloc;
	void *cb_of_free;
};

struct isp_statis_info {
	cmr_u32 irq_type;
	cmr_u32 irq_property;
	cmr_u32 phy_addr;
	cmr_u32 vir_addr;
	cmr_u32 addr_offset;
	cmr_u32 kaddr[2];
	cmr_u32 buf_size;
	cmr_s32 mfd;
	cmr_s32 frame_id;
	cmr_u32 sec;
	cmr_u32 usec;
	cmr_s64 monoboottime;
	struct sprd_img_vcm_dac_info dac_info;
};

enum isp_fetch_format {
	ISP_FETCH_YUV422_3FRAME = 0x00,
	ISP_FETCH_YUYV,
	ISP_FETCH_UYVY,
	ISP_FETCH_YVYU,
	ISP_FETCH_VYUY,
	ISP_FETCH_YUV422_2FRAME,
	ISP_FETCH_YVU422_2FRAME,
	ISP_FETCH_NORMAL_RAW10,
	ISP_FETCH_CSI2_RAW10,
	ISP_FETCH_FORMAT_MAX
};

enum isp_store_format {
	ISP_STORE_UYVY = 0x00,
	ISP_STORE_YUV422_2FRAME,
	ISP_STORE_YVU422_2FRAME,
	ISP_STORE_YUV422_3FRAME,
	ISP_STORE_YUV420_2FRAME,
	ISP_STORE_YVU420_2FRAME,
	ISP_STORE_YUV420_3FRAME,
	ISP_STORE_FORMAT_MAX
};

enum isp_work_mode {
	ISP_SINGLE_MODE = 0x00,
	ISP_CONTINUE_MODE,
};

enum isp_match_mode {
	ISP_CAP_MODE = 0x00,
	ISP_EMC_MODE,
	ISP_DCAM_MODE,
	ISP_SIMULATION_MODE,
};

enum isp_endian {
	ISP_ENDIAN_LITTLE = 0x00,
	ISP_ENDIAN_BIG,
	ISP_ENDIAN_HALFBIG,
	ISP_ENDIAN_HALFLITTLE,
	ISP_ENDIAN_MAX
};

enum isp_drv_slice_type {
	ISP_DRV_LSC = 0x00,
	ISP_DRV_CSC,
	ISP_DRV_BDN,
	ISP_DRV_PWD,
	ISP_DRV_LENS,
	ISP_DRV_AEM,
	ISP_DRV_Y_AEM,
	ISP_DRV_RGB_AFM,
	ISP_DRV_Y_AFM,
	ISP_DRV_HISTS,
	ISP_DRV_DISPATCH,
	ISP_DRV_FETCH,
	ISP_DRV_STORE,
	ISP_DRV_SLICE_TYPE_MAX
};

struct isp_data_param {
	enum isp_work_mode work_mode;
	enum isp_match_mode input;
	enum isp_format input_format;
	cmr_u32 format_pattern;
	struct isp_size input_size;
	struct isp_addr input_addr;
	struct isp_addr input_vir;
	enum isp_format output_format;
	enum isp_match_mode output;
	struct isp_addr output_addr;
	cmr_u32 input_fd;
};

struct isp_drv_slice_param {
	cmr_u32 slice_line;
	cmr_u32 complete_line;
	cmr_u32 all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_DRV_SLICE_WIN_NUM];
	cmr_u32 edge_info;
};

struct isp_drv_interface_param {
	struct isp_data_param data;

	struct isp_dev_fetch_info fetch;
	struct isp_dev_store_info store;
	struct isp_dev_dispatch_info dispatch;
	struct isp_dev_arbiter_info arbiter;
	struct isp_dev_common_info com;
	struct isp_size src;
	struct isp_drv_slice_param slice;
};

/*ISP Hardware Device*/
cmr_s32 isp_dev_open(cmr_s32 fd, cmr_handle * handle);
cmr_s32 isp_dev_close(cmr_handle handle);
cmr_s32 isp_dev_reset(cmr_handle handle);
cmr_s32 isp_dev_set_statis_buf(cmr_handle handle, struct isp_statis_buf_input *param);
cmr_s32 isp_dev_set_slice_raw_info(cmr_handle handle, struct isp_raw_proc_info *param);

/*ISP 3DNR*/
cmr_s32 isp_dev_3dnr(cmr_handle handle, struct isp_3dnr_info *param);
cmr_s32 isp_u_3dnr_cap_block(isp_handle handle, void *param_ptr);
cmr_s32 isp_u_3dnr_pre_block(isp_handle handle, void *param_ptr);

/*ISP Capability*/
cmr_s32 isp_u_capability_continue_size(cmr_handle handle, cmr_u16 * width, cmr_u16 * height);
cmr_s32 isp_u_capability_time(cmr_handle handle, cmr_u32 * sec, cmr_u32 * usec);

/*ISP Sub Block: Fetch*/
cmr_s32 isp_u_fetch_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_fetch_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_fetch_raw_transaddr(cmr_handle handle, struct isp_dev_block_addr *addr);
cmr_s32 isp_u_fetch_start_isp(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_fetch_yuv_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_fetch_yuv_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_fetch_yuv_start_isp(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: BLC*/
cmr_s32 isp_u_blc_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_blc_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_blc_slice_info(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: lens shading calibration*/
cmr_s32 isp_u_2d_lsc_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_2d_lsc_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_2d_lsc_param_update(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_2d_lsc_pos(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_2d_lsc_grid_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_2d_lsc_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_1d_lsc_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_1d_lsc_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_1d_lsc_pos(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: AWBM*/
cmr_s32 isp_u_awb_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbm_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbm_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbm_skip_num(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbm_block_offset(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbm_block_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbm_shift(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbc_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbc_gain(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbc_thrd(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbc_gain_offset(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: BPC*/
cmr_s32 isp_u_bpc_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_bpc_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_bpc_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_bpc_param_common(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_bpc_thrd(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_bpc_map_addr(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_bpc_pixel_num(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: GRGB*/
cmr_s32 isp_u_grgb_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_grgb_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_grgb_thrd(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: CFA*/
cmr_s32 isp_u_cfa_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cfa_thrd(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cfa_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cfa_slice_info(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: CMC*/
cmr_s32 isp_u_cmc_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cmc_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cmc_matrix(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: GAMMA*/
cmr_s32 isp_u_gamma_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_gamma_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_gamma_node(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: CCE*/
cmr_s32 isp_u_cce_matrix_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cce_uv_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cce_uvdivision_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cce_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cce_matrix(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cce_shift(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cce_uvd(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_cce_uvc(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: Brightness*/
cmr_s32 isp_u_brightness_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_brightness_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_brightness_slice_info(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: Contrast*/
cmr_s32 isp_u_contrast_block(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: HIST*/
cmr_s32 isp_u_hist_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist_mode(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: AFM*/
cmr_s32 isp_u_raw_afm_statistic_r6p9(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_spsmd_square_en(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_overflow_protect(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_subfilter(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_spsmd_touch_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_shfit(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_threshold_rgb(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: EDGE*/
cmr_s32 isp_u_edge_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_edge_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_edge_param(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: CSA*/
cmr_s32 isp_u_csa_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_csa_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_csa_factor(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: Store*/
cmr_s32 isp_u_store_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_store_slice_size(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: NLC*/
cmr_s32 isp_u_nlc_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_nlc_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_nlc_r_node(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_nlc_g_node(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_nlc_b_node(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_nlc_l_node(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: Bing4awb*/
cmr_s32 isp_u_binning4awb_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_endian(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_scaling_ratio(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_get_scaling_ratio(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_mem_addr(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_statistics_buf(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_transaddr(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_binning4awb_initbuf(cmr_handle handle);

/*ISP Sub Block: Pre Glb Gain*/
cmr_s32 isp_u_pgg_block(cmr_handle handle, void *param_ptr);

/*ISP Sub Block: COMMON*/
cmr_s32 isp_u_comm_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_comm_shadow_ctrl(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_comm_channel0_y_aem_pos(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_comm_channel1_y_aem_pos(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_shadow_ctrl_all(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_awbm_shadow_ctrl(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_ae_shadow_ctrl(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_af_shadow_ctrl(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_afl_shadow_ctrl(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_3a_ctrl(cmr_handle handle, void *param_ptr);

/*Rgb Gain*/
cmr_s32 isp_u_rgb_gain_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_rgb_dither_block(cmr_handle handle, void *param_ptr);

/*Hue*/
cmr_s32 isp_u_hue_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hue_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hue_Factor(cmr_handle handle, void *param_ptr);

/*Aem*/
cmr_s32 isp_u_raw_aem_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_skip_num(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_shift(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_offset(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_blk_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_aem_statistics(cmr_handle handle, cmr_u32 * r_info, cmr_u32 * g_info, cmr_u32 * b_info);

/*Nlm*/
cmr_s32 isp_u_nlm_block(cmr_handle handle, void *param_ptr);

/*Hsv*/
cmr_s32 isp_u_hsv_block(cmr_handle handle, void *param_ptr);

/*Pstrz*/
cmr_s32 isp_u_posterize_block(cmr_handle handle, void *param_ptr);

/*Raw Afm*/
cmr_s32 isp_u_raw_afm_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_type1_statistic(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_type2_statistic(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_skip_num(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_iir_nr_cfg(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_modules_cfg(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_skip_num_clr(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_spsmd_rtgbot_enable(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_spsmd_diagonal_enable(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_spsmd_cal_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_sel_filter1(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_sel_filter2(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_sobel_type(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_spsmd_type(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_sobel_threshold(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_spsmd_threshold(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_win(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_raw_afm_win_num(cmr_handle handle, void *param_ptr);

/*Anti Flicker*/
cmr_s32 isp_u_anti_flicker_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_anti_flicker_statistic(cmr_handle handle, void *addr);
cmr_s32 isp_u_anti_flicker_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_anti_flicker_transaddr(cmr_handle handle, void *param_ptr);

/*Flicker New*/
cmr_s32 isp_u_anti_flicker_new_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_anti_flicker_new_block(cmr_handle handle, void *param_ptr);

/*Hist*/
cmr_s32 isp_u_hist_v1_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist_slice_size(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist2_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist2_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist2_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_hist2_roi(cmr_handle handle, void *param_ptr);

/*Pre Cdn*/
cmr_s32 isp_u_yuv_precdn_block(cmr_handle handle, void *param_ptr);

/*Cdn*/
cmr_s32 isp_u_yuv_cdn_block(cmr_handle handle, void *param_ptr);

/*Post Cdn*/
cmr_s32 isp_u_yuv_postcdn_block(cmr_handle handle, void *param_ptr);

/*Ygamma*/
cmr_s32 isp_u_ygamma_block(cmr_handle handle, void *param_ptr);

/*Ydelay*/
cmr_s32 isp_u_ydelay_block(cmr_handle handle, void *param_ptr);

/*Iircnr*/
cmr_s32 isp_u_iircnr_block(cmr_handle handle, void *param_ptr);

/*Yrandom*/
cmr_s32 isp_u_yrandom_block(cmr_handle handle, void *param_ptr);

/*Dispatch*/
cmr_s32 isp_u_dispatch_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_dispatch_ch0_size(cmr_handle handle, void *param_ptr);

/*Dispatch Yuv*/
cmr_s32 isp_u_dispatch_yuv_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_dispatch_yuv_ch0_size(cmr_handle handle, void *param_ptr);

/*Arbiter*/
cmr_s32 isp_u_arbiter_block(cmr_handle handle, void *param_ptr);

/*Interface*/
cmr_s32 isp_cfg_block(cmr_handle handle, void *param_ptr, cmr_u32 sub_block);
cmr_s32 isp_set_arbiter(cmr_handle handle);
cmr_s32 isp_set_dispatch(cmr_handle handle);
cmr_s32 isp_get_fetch_addr(struct isp_drv_interface_param *isp_context_ptr, struct isp_dev_fetch_info *fetch_ptr);
cmr_s32 isp_set_fetch_param(cmr_handle handle);
cmr_s32 isp_set_store_param(cmr_handle handle);
cmr_s32 isp_set_slice_size(cmr_handle handle);
cmr_s32 isp_cfg_slice_size(cmr_handle handle, struct isp_u_blocks_info *block_ptr);
cmr_s32 isp_set_comm_param(cmr_handle handle);
cmr_s32 isp_cfg_comm_data(cmr_handle handle, struct isp_u_blocks_info *block_ptr);

/*Rgb2y*/
cmr_s32 isp_u_rgb2y_block(cmr_handle handle, void *param_ptr);

/*Uvd*/
cmr_s32 isp_u_uvd_block(cmr_handle handle, void *param_ptr);

/*Post Blc*/
cmr_s32 isp_u_post_blc_block(cmr_handle handle, void *param_ptr);

/*Ynr*/
cmr_s32 isp_u_ynr_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_ynr_bypass(cmr_handle handle, void *param_ptr);

/*Noise filter*/
cmr_s32 isp_u_noise_filter_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_noise_fliter_bypass(cmr_handle handle, void *param_ptr);

/*Rarius Lsc*/
cmr_s32 isp_u_rarius_lsc_block(cmr_handle handle, void *param_ptr);

/* PDAF sub-blocks */
cmr_s32 isp_u_pdaf_block(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_skip_num(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_work_mode(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_ppi_info(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_extractor_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_roi(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_correct_bypass(cmr_handle handle, void *param_ptr);
cmr_s32 isp_u_pdaf_correction(cmr_handle handle, void *param_ptr);


/* DCAM sub-blocks */
cmr_s32 dcam_u_2d_lsc_block(cmr_handle handle, void *param_ptr);
cmr_s32 dcam_u_2d_lsc_transaddr(cmr_handle handle, struct isp_statis_buf_input * buf);
cmr_s32 dcam_u_blc_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_raw_aem_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_raw_aem_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 dcam_u_raw_aem_skip_num(cmr_handle handle, cmr_u32 skip_num);
cmr_s32 dcam_u_raw_aem_shift(cmr_handle handle, void *shift);
cmr_s32 dcam_u_raw_aem_offset(cmr_handle handle, cmr_u32 x, cmr_u32 y);
cmr_s32 dcam_u_raw_aem_blk_size(cmr_handle handle, cmr_u32 width, cmr_u32 height);

#endif
