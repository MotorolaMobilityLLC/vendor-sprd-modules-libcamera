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

#ifndef _ISP_SLICE_H_
#define _ISP_SLICE_H_


#define SLICE_NUM_MAX				4
#define SLICE_W_NUM_MAX				4
#define SLICE_H_NUM_MAX				1
#define SLICE_OVERLAP_W_MAX			64


#define RAW_OVERLAP_UP				62
#define RAW_OVERLAP_DOWN			82
#define RAW_OVERLAP_LEFT			90
#define RAW_OVERLAP_RIGHT			142
#define YUV_OVERLAP_UP				46
#define YUV_OVERLAP_DOWN			68
#define YUV_OVERLAP_LEFT			74
#define YUV_OVERLAP_RIGHT			126

#define YUVSCALER_OVERLAP_UP			32
#define YUVSCALER_OVERLAP_DOWN			52
#define YUVSCALER_OVERLAP_LEFT			16
#define YUVSCALER_OVERLAP_RIGHT			68


struct slice_scaler_info {
	uint32_t out_of_range;
	uint32_t scaler_bypass;
	uint32_t odata_mode;
	uint32_t trim0_size_x;
	uint32_t trim0_size_y;
	uint32_t trim0_start_x;
	uint32_t trim0_start_y;
	uint32_t trim1_size_x;
	uint32_t trim1_size_y;
	uint32_t trim1_start_x;
	uint32_t trim1_start_y;
	uint32_t scaler_ip_int;
	uint32_t scaler_ip_rmd;
	uint32_t scaler_cip_int;
	uint32_t scaler_cip_rmd;
	uint32_t scaler_factor_in;
	uint32_t scaler_factor_out;
	uint32_t scaler_ip_int_ver;
	uint32_t scaler_ip_rmd_ver;
	uint32_t scaler_cip_int_ver;
	uint32_t scaler_cip_rmd_ver;
	uint32_t scaler_factor_in_ver;
	uint32_t scaler_factor_out_ver;
	uint32_t scaler_in_width;
	uint32_t scaler_in_height;
	uint32_t scaler_out_width;
	uint32_t scaler_out_height;
	uint32_t src_size_x;
	uint32_t src_size_y;
	uint32_t dst_size_x;
	uint32_t dst_size_y;
	uint32_t chk_sum_clr;
};

struct slice_thumbscaler_info {
	uint32_t out_of_range;
	uint32_t scaler_bypass;
	uint32_t odata_mode;

	struct img_deci_info y_deci;
	struct img_deci_info uv_deci;

	struct img_size y_factor_in;
	struct img_size y_factor_out;
	struct img_size uv_factor_in;
	struct img_size uv_factor_out;

	struct img_size src0; /* input image/slice size */

	struct img_trim y_trim;
	struct img_size y_src_after_deci;
	struct img_size y_dst_after_scaler;
	struct img_size y_init_phase;

	struct img_trim uv_trim;
	struct img_size uv_src_after_deci;
	struct img_size uv_dst_after_scaler;
	struct img_size uv_init_phase;
};

struct slice_nlm_info {
	uint32_t center_x_relative;
	uint32_t center_y_relative;
};

struct slice_postcdn_info {
	uint32_t start_row_mod4;
};

struct slice_ynr_info {
	int32_t center_offset_x;
	int32_t center_offset_y;
	uint32_t slice_width;
	uint32_t slice_height;
};

struct slice_pos_info {
	uint32_t start_col;
	uint32_t start_row;
	uint32_t end_col;
	uint32_t end_row;
};

struct slice_overlap_info {
	uint32_t overlap_up;
	uint32_t overlap_down;
	uint32_t overlap_left;
	uint32_t overlap_right;
};

struct slice_border_info {
	uint32_t up_border;
	uint32_t down_border;
	uint32_t left_border;
	uint32_t right_border;
};

struct slice_store_info {
	uint32_t store_bypass;
	struct img_size size;
	struct img_addr addr;
	struct img_pitch pitch;
	struct slice_border_info border;
};

struct slice_fetch_info {
	struct img_size size;
	struct img_addr addr;
	uint32_t mipi_byte_rel_pos;
	uint32_t mipi_word_num;
};

struct slice_fbd_raw_info {
	/* ISP_FBD_RAW_SEL */
	uint32_t pixel_start_in_hor:6;
	uint32_t pixel_start_in_ver:2;
	uint32_t fetch_fbd_bypass:1;
	/* ISP_FBD_RAW_SLICE_SIZE */
	uint32_t height;
	uint32_t width;
	/* ISP_FBD_RAW_PARAM0 */
	uint32_t tiles_num_in_ver:11;
	uint32_t tiles_num_in_hor:6;
	/* ISP_FBD_RAW_PARAM1 */
	uint32_t tiles_start_odd:1;
	uint32_t tiles_num_pitch:8;
	/* ISP_FBD_RAW_PARAM2 */
	uint32_t header_addr_init;
	/* ISP_FBD_RAW_PARAM3 */
	uint32_t tile_addr_init_x256;
	/* ISP_FBD_RAW_PARAM4 */
	uint32_t fbd_cr_ch0123_val0;
	/* ISP_FBD_RAW_PARAM5 */
	uint32_t fbd_cr_ch0123_val1;
	/* ISP_FBD_RAW_PARAM6 */
	uint32_t fbd_cr_uv_val1:8;
	uint32_t fbd_cr_y_val1:8;
	uint32_t fbd_cr_uv_val0:8;
	uint32_t fbd_cr_y_val0:8;
	/* ISP_FBD_RAW_LOW_PARAM0 */
	uint32_t low_bit_addr_init;
	/* ISP_FBD_RAW_LOW_PARAM1 */
	uint32_t low_bit_pitch:16;
};

struct slice_3dnr_memctrl_info {
	uint32_t bypass;
	uint32_t start_col;
	uint32_t start_row;
	uint32_t first_line_mode;
	uint32_t last_line_mode;
	struct img_size src;
	struct img_size ft_y;
	struct img_size ft_uv;
	struct img_addr addr;
};

struct slice_3dnr_store_info {
	uint32_t bypass;
	struct img_size size;
	struct img_addr addr;
};

struct slice_3dnr_crop_info {
	uint32_t bypass;
	uint32_t start_x;
	uint32_t start_y;
	struct img_size src;
	struct img_size dst;
};

struct slice_ltm_map_info {
	uint32_t bypass;

	uint32_t tile_width;
	uint32_t tile_height;

	uint32_t tile_num_x;
	uint32_t tile_num_y;
	uint32_t tile_right_flag;
	uint32_t tile_left_flag;
	uint32_t tile_start_x;
	uint32_t tile_start_y;
	uint32_t mem_addr;
};

struct slice_noisefilter_info {
	uint32_t seed0;
	uint32_t seed1;
	uint32_t seed2;
	uint32_t seed3;
	uint32_t seed_int;
};

struct slice_noisefilter_mode_info {
	uint32_t seed_for_mode1;
	uint32_t shape_mode;
};

struct isp_slice_desc {
	uint32_t valid;
	uint32_t x;
	uint32_t y;
	uint32_t path_en[ISP_SPATH_NUM];
	/* slice position in src buffer, for fetch */
	struct slice_pos_info slice_pos_fetch;
	/* slice position (in fetched image) without overlap*/
	struct slice_pos_info slice_pos_orig;
	/* slice position (in fetched image) with overlap*/
	struct slice_pos_info slice_pos;
	struct slice_overlap_info slice_overlap;
	struct slice_fetch_info slice_fetch;
	struct slice_fbd_raw_info slice_fbd_raw;
	struct slice_store_info slice_store[ISP_SPATH_NUM];
	struct slice_scaler_info slice_scaler[ISP_SPATH_NUM];
	struct slice_thumbscaler_info slice_thumbscaler;
	struct slice_nlm_info slice_nlm;
	struct slice_postcdn_info slice_postcdn;
	struct slice_ynr_info slice_ynr;
	struct slice_3dnr_memctrl_info slice_3dnr_memctrl;
	struct slice_3dnr_store_info   slice_3dnr_store;
	struct slice_3dnr_crop_info    slice_3dnr_crop;
	struct slice_ltm_map_info      slice_ltm_map;
	struct slice_noisefilter_info  noisefilter_info;
	struct slice_noisefilter_mode_info slice_noisefilter_mode;
};

struct isp_slice_context {
	struct isp_slice_desc slices[SLICE_NUM_MAX];
	uint32_t slice_row_num;
	uint32_t slice_col_num;
	uint32_t slice_num;
	uint32_t slice_height;
	uint32_t slice_width;
	uint32_t img_width;
	uint32_t img_height;
	uint32_t overlap_up;
	uint32_t overlap_down;
	uint32_t overlap_left;
	uint32_t overlap_right;
};

extern uint32_t line_buffer_len;

int isp_cfg_slice_fetch_info(
			void *cfg_in, struct isp_slice_context *slc_ctx);

int isp_cfg_slice_store_info(
		void *cfg_in, struct isp_slice_context *slc_ctx);

int isp_cfg_slice_3dnr_info(
		void *cfg_in, struct isp_slice_context *slc_ctx);
int isp_cfg_slice_noisefilter_info(void *cfg_in, struct isp_slice_context *slc_ctx);


int isp_cfg_slice_ltm_info(
	       void *cfg_in, struct isp_slice_context *slc_ctx);
int isp_cfg_slices(void *cfg_in, struct isp_slice_context *slc_ctx);

void *get_isp_slice_ctx(void);
int put_isp_slice_ctx(void **slc_ctx);

int isp_set_slices_fmcu_cmds(
		void *fmcu_handle,
		void *slc_handle,
		uint32_t ctx_idx,
		uint32_t wmode);

int isp_set_slw_fmcu_cmds(
		void *fmcu_handle,
		struct isp_pipe_context *pctx);

#endif
