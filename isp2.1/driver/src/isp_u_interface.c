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

#include "isp_drv.h"

#define YUV_OVERLAP_UP         3
#define YUV_OVERLAP_DOWN       2
#define YUV_OVERLAP_LEFT       6
#define YUV_OVERLAP_RIGHT      4
#define RAW_OVERLAP_UP         11
#define RAW_OVERLAP_DOWN       10
#define RAW_OVERLAP_LEFT       14
#define RAW_OVERLAP_RIGHT      12

#define YUV_OVERLAP_UP_V1      3
#define YUV_OVERLAP_DOWN_V1    2
#define YUV_OVERLAP_LEFT_V1    6
#define YUV_OVERLAP_RIGHT_V1   4
#define RAW_OVERLAP_UP_V1      27
#define RAW_OVERLAP_DOWN_V1    28
#define RAW_OVERLAP_LEFT_V1    56
#define RAW_OVERLAP_RIGHT_V1   56

#define ISP_MIN(x,y) (((x) > (y)) ? (y) : (x))

typedef isp_s32 (*isp_cfg_fun_ptr)(isp_handle handle, void *param_ptr);

struct isp_cfg_fun {
	isp_u32 sub_block;
	isp_cfg_fun_ptr cfg_fun;
};

	/************************************Tshark2*******************************************************/
static struct isp_cfg_fun s_isp_cfg_fun_tab_tshark2[] = {
	{ISP_BLK_PRE_GBL_GAIN,  isp_u_pgg_block},
	{ISP_BLK_BLC,           isp_u_blc_block},
	{ISP_BLK_RGB_GAIN,      isp_u_rgb_gain_block},
	{ISP_BLK_NLC,           isp_u_nlc_block},
	{ISP_BLK_2D_LSC,           isp_u_2d_lsc_block},
	{ISP_BLK_1D_LSC,           isp_u_1d_lsc_block},
	{ISP_BLK_BINNING4AWB,   isp_u_binning4awb_block},
	{ISP_BLK_AWB_NEW,           isp_u_awb_block},
	{ISP_BLK_BPC,           isp_u_bpc_block},
	{ISP_BLK_AE_NEW,            isp_u_raw_aem_block},
	{ISP_BLK_GRGB,          isp_u_grgb_block},
//	{ISP_BLK_RGB_GAIN2,        isp_u_rgb_gain2_block},
	{ISP_BLK_CFA,           isp_u_cfa_block},
	{ISP_BLK_CMC10,            isp_u_cmc_block},
//	{ISP_BLK_CTM,              isp_u_ct_block},
//	{ISP_BLK_RGB_GAMC,         isp_u_gamma_block},
//	{ISP_BLK_CCE,           isp_u_cce_matrix_block},
//	{ISP_BLK_RADIAL_CSC,       isp_u_csc_block},
//	{ISP_BLK_RGB_PRECDN,       isp_u_frgb_precdn_block},
	{ISP_BLK_POSTERIZE,        isp_u_posterize_block},
	{ISP_BLK_YUV_PRECDN,       isp_u_yuv_precdn_block},
//	{ISP_BLK_PREF_V1,          isp_u_prefilter_block},
	{ISP_BLK_BRIGHT,           isp_u_brightness_block},
	{ISP_BLK_CONTRAST,         isp_u_contrast_block},
//	{ISP_BLK_AUTO_CONTRAST_V1, isp_u_aca_block},
	{ISP_BLK_UV_CDN,           isp_u_yuv_cdn_block},
	{ISP_BLK_EDGE,          isp_u_edge_block},
//	{ISP_BLK_EMBOSS_V1,        isp_u_emboss_block},
	{ISP_BLK_SATURATION,    isp_u_csa_block},
	{ISP_BLK_HUE,           isp_u_hue_block},
	{ISP_BLK_UV_POSTCDN,       isp_u_yuv_postcdn_block},
	{ISP_BLK_Y_GAMMC,          isp_u_ygamma_block},
	{ISP_BLK_Y_DELAY,           isp_u_ydelay_block},
	{ISP_BLK_NLM,              isp_u_nlm_block},
	{ISP_BLK_UVDIV,         isp_u_cce_uv_block},
	{ISP_BLK_AF_NEW,            isp_u_raw_afm_block},
	{ISP_BLK_HSV,              isp_u_hsv_block},
//	{ISP_BLK_PRE_WAVELET_V1,   isp_u_pwd_block},
//	{ISP_BLK_BL_NR_V1,         isp_u_bdn_block},
//	{ISP_BLK_CSS_V1,           isp_u_css_block},
//	{ISP_BLK_CMC8,             isp_u_cmc8_block},
	{ISP_BLK_IIRCNR_IIR,       isp_u_iircnr_block},
	{ISP_BLK_IIRCNR_YRANDOM,   isp_u_yrandom_block},
//	{ISP_BLK_YIQ_AFM,          isp_u_yiq_afm_block},
//	{ISP_BLK_YIQ_AEM,          isp_u_yiq_aem_block},
	{ISP_BLK_HIST,          isp_u_hist_block},
	{ISP_BLK_HIST2,            isp_u_hist2_block},
//	{ISP_BLK_YIQ_AFL,          isp_u_anti_flicker_block},
};

isp_s32 isp_cfg_block(isp_handle handle, void *param_ptr, isp_u32 sub_block)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, cnt = 0;
	isp_cfg_fun_ptr cfg_fun_ptr = PNULL;

	cnt = sizeof(s_isp_cfg_fun_tab_tshark2) / sizeof(s_isp_cfg_fun_tab_tshark2[0]);
	for (i = 0; i < cnt; i++) {
		if (sub_block == s_isp_cfg_fun_tab_tshark2[i].sub_block) {
			cfg_fun_ptr = s_isp_cfg_fun_tab_tshark2[i].cfg_fun;
			break;
		}
	}

	if (PNULL != cfg_fun_ptr) {
		rtn = cfg_fun_ptr(handle, param_ptr);
	}

	return rtn;
}

isp_u32 isp_get_cfa_default_param(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_cfa_info_v1 *cfa_param)
{
	isp_s32 rtn = ISP_SUCCESS;

	cfa_param->bypass = 0;

	cfa_param->grid_thr = 500;
	cfa_param->min_grid_new = 500;

	cfa_param->grid_gain_new = 4;
	cfa_param->strong_edge_thr = 127;
	cfa_param->weight_control_bypass = 1;
	cfa_param->uni_dir_intplt_thr_new = 20;

	cfa_param->smooth_area_thr = 0;
	cfa_param->cdcr_adj_factor = 8;

	cfa_param->readblue_high_sat_thr = 280;
	cfa_param->grid_dir_weight_t1 = 8;
	cfa_param->grid_dir_weight_t2 = 8;

	cfa_param->low_lux_03_thr = 100;
	cfa_param->round_diff_03_thr = 100;
	cfa_param->low_lux_12_thr = 200;
	cfa_param->round_diff_12_thr = 200;

	cfa_param->css_bypass = 0;
	cfa_param->css_edge_thr =150;
	cfa_param->css_weak_edge_thr = 100;
	cfa_param->css_texture1_thr = 20;
	cfa_param->css_texture2_thr = 5;

	cfa_param->css_uv_val_thr = 15;
	cfa_param->css_uv_diff_thr = 30;
	cfa_param->css_gray_thr = 40;

	cfa_param->css_pix_similar_thr = 200;
	cfa_param->css_green_edge_thr = 0;
	cfa_param->css_green_weak_edge_thr = 0;

	cfa_param->css_green_tex1_thr = 0;
	cfa_param->css_green_tex2_thr = 5;
	cfa_param->css_green_flat_thr = 0;
	cfa_param->css_edge_corr_ratio_r  = 120;
	cfa_param->css_edge_corr_ratio_b = 80;
	cfa_param->css_text1_corr_ratio_r = 0;
	cfa_param->css_text1_corr_ratio_b = 0;
	cfa_param->css_text2_corr_ratio_r = 50;
	cfa_param->css_text2_corr_ratio_b = 50;
	cfa_param->css_flat_corr_ratio_r = 0;
	cfa_param->css_flat_corr_ratio_b = 0;
	cfa_param->css_wedge_corr_ratio_r =100;
	cfa_param->css_wedge_corr_ratio_b = 60;

	cfa_param->css_alpha_for_tex2 = 8;
	cfa_param->css_skin_u_top[0] = 508;
	cfa_param->css_skin_u_down[0] = 308;
	cfa_param->css_skin_v_top[0] = 636;
	cfa_param->css_skin_v_down[0] = 532;
	cfa_param->css_skin_u_top[1] = 511;
	cfa_param->css_skin_u_down[1] = 461;
	cfa_param->css_skin_v_top[1] = 557;
	cfa_param->css_skin_v_down[1] = 517;

	cfa_param->gbuf_addr_max = (isp_context_ptr->store.size.width>>1) +1;

	return rtn;
}

isp_s32 isp_get_cce_default_param(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_cce_info_v1 *cce_param)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	unsigned short cce_matrix[] = {77, 150, 29,
				-43, -85, 128,
				128, -107, -21};

	cce_param->bypass = 0;
	for (i = 0; i< 9; i++) {
		cce_param->matrix[i] = cce_matrix[i];
	}
	cce_param->y_offset = 0;
	cce_param->u_offset = 0;
	cce_param->v_offset = 0;

	return rtn;
}
isp_s32 isp_set_arbiter(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_interface_param_v1 *isp_context_ptr = (struct isp_interface_param_v1 *)isp_handler;
	struct isp_dev_arbiter_info_v1 *isp_arbiter_ptr = &isp_context_ptr->arbiter;
#if 0
	isp_arbiter_ptr->endian_ch0.bpc_endian = ISP_ENDIAN_BIG;
	isp_arbiter_ptr->endian_ch0.lens_endian = ISP_ENDIAN_BIG;
	isp_arbiter_ptr->endian_ch0.fetch_bit_reorder = ISP_LSB;
	if (ISP_EMC_MODE == isp_context_ptr->data.input) {
		isp_arbiter_ptr->endian_ch0.fetch_endian = ISP_ENDIAN_LITTLE;
	} else {
		isp_arbiter_ptr->endian_ch0.fetch_endian = ISP_ENDIAN_BIG;
	}
	if (ISP_EMC_MODE == isp_context_ptr->data.output) {
		isp_arbiter_ptr->endian_ch0.store_endian = ISP_ENDIAN_LITTLE;
	} else {
		isp_arbiter_ptr->endian_ch0.store_endian = ISP_ENDIAN_BIG;
	}
#endif
	if (ISP_EMC_MODE == isp_context_ptr->data.input) {
		isp_arbiter_ptr->fetch_raw_endian = ISP_ENDIAN_BIG;
		isp_arbiter_ptr->fetch_raw_word_change = ISP_ZERO;
		isp_arbiter_ptr->fetch_bit_reorder = ISP_ZERO;
		isp_arbiter_ptr->fetch_yuv_endian = ISP_ENDIAN_LITTLE;
		isp_arbiter_ptr->fetch_yuv_word_change = ISP_ZERO;
	}

	return rtn;
}

isp_s32 isp_set_dispatch(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_interface_param_v1 *isp_context_ptr = (struct isp_interface_param_v1 *)isp_handler;
	struct isp_dev_dispatch_info_v1 *isp_dispatch_ptr = &isp_context_ptr->dispatch;

	isp_dispatch_ptr->bayer_ch0 = isp_context_ptr->data.format_pattern;
	isp_dispatch_ptr->ch0_size.width = isp_context_ptr->data.input_size.w;
	isp_dispatch_ptr->ch0_size.height = isp_context_ptr->data.input_size.h;
	isp_dispatch_ptr->bayer_ch1 = isp_context_ptr->data.format_pattern;
	isp_dispatch_ptr->ch1_size.width = isp_context_ptr->data.input_size.w;
	isp_dispatch_ptr->ch1_size.height = isp_context_ptr->data.input_size.h;
	isp_dispatch_ptr->height_dly_num_ch0 = 0x10; //default value
	isp_dispatch_ptr->width_dly_num_ch0 = 0x3C;
	isp_dispatch_ptr->pipe_dly_num = 0x8;

	return rtn;
}

static enum isp_fetch_format isp_get_fetch_format(enum isp_format in_format)
{
	enum isp_fetch_format format = ISP_FETCH_FORMAT_MAX;

	switch (in_format) {
	case ISP_DATA_YUV422_3FRAME:
		format = ISP_FETCH_YUV422_3FRAME;
		break;
	case ISP_DATA_YUYV:
		format = ISP_FETCH_YUYV;
		break;
	case ISP_DATA_UYVY:
		format = ISP_FETCH_UYVY;
		break;
	case ISP_DATA_YVYU:
		format = ISP_FETCH_YVYU;
		break;
	case ISP_DATA_VYUY:
		format = ISP_FETCH_VYUY;
		break;
	case ISP_DATA_YUV422_2FRAME:
		format = ISP_FETCH_YUV422_2FRAME;
		break;
	case ISP_DATA_YVU422_2FRAME:
		format = ISP_FETCH_YVU422_2FRAME;
		break;
	case ISP_DATA_NORMAL_RAW10:
		format = ISP_FETCH_NORMAL_RAW10;
		break;
	case ISP_DATA_CSI2_RAW10:
		format = ISP_FETCH_CSI2_RAW10;
		break;
	default:
		break;
	}

	return format;
}

static isp_s32 isp_get_fetch_pitch(struct isp_pitch *pitch_ptr, isp_u16 width, enum isp_format format)
{
	isp_s32 rtn = ISP_SUCCESS;

	pitch_ptr->chn0 = ISP_ZERO;
	pitch_ptr->chn1 = ISP_ZERO;
	pitch_ptr->chn2 = ISP_ZERO;

	switch (format) {
	case ISP_DATA_YUV422_3FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width >> ISP_ONE;
		pitch_ptr->chn2 = width >> ISP_ONE;
		break;
	case ISP_DATA_YUV422_2FRAME:
	case ISP_DATA_YVU422_2FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width;
		break;
	case ISP_DATA_YUYV:
	case ISP_DATA_UYVY:
	case ISP_DATA_YVYU:
	case ISP_DATA_VYUY:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	case ISP_DATA_NORMAL_RAW10:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	case ISP_DATA_CSI2_RAW10:
		pitch_ptr->chn0 = (width * 5) >> ISP_TWO;
		break;
	default:
		break;
	}

	return rtn;
}

isp_s32 isp_get_fetch_addr_v1(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_fetch_info_v1 *fetch_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
#if 0
	struct isp_size *cur_slice_ptr = &isp_context_ptr->slice.cur_slice_num;
	isp_u32 ch0_offset = ISP_ZERO;
	isp_u32 ch1_offset = ISP_ZERO;
	isp_u32 ch2_offset = ISP_ZERO;
	isp_u32 slice_w_offset = ISP_ZERO;
	isp_u32 slice_h_offset = ISP_ZERO;
	isp_u32 overlap_up = ISP_ZERO;
	isp_u32 overlap_left = ISP_ZERO;
	isp_u16 src_width = isp_context_ptr->src.w;
	isp_u16 complete_line = isp_context_ptr->slice.complete_line;
	isp_u16 slice_width = isp_context_ptr->slice.max_size.w;
#endif
	isp_u16 fetch_width = fetch_ptr->size.width;//isp_context_ptr->slice.size[ISP_FETCH].w;
	isp_u32 start_col = ISP_ZERO;
	isp_u32 end_col = ISP_ZERO;
	isp_u32 mipi_word_num_start[16] = {0,
										1,1,1,1,
										2,2,2,
										3,3,3,
										4,4,4,
										5,5
	};
	isp_u32 mipi_word_num_end[16] = {0,
									2,2,2,2,
									3,3,3,3,
									4,4,4,4,
									5,5,5
	};
#if 0
	fetch_ptr->bypass = isp_context_ptr->fetch.bypass;
	fetch_ptr->subtract = isp_context_ptr->fetch.subtract;
	fetch_ptr->color_format = isp_context_ptr->fetch.color_format;
	fetch_ptr->addr.chn0 = isp_context_ptr->fetch.addr.chn0;
	fetch_ptr->addr.chn1 = isp_context_ptr->fetch.addr.chn1;
	fetch_ptr->addr.chn2 = isp_context_ptr->fetch.addr.chn2;
	fetch_ptr->pitch.chn0 = isp_context_ptr->fetch.pitch.chn0;
	fetch_ptr->pitch.chn1 = isp_context_ptr->fetch.pitch.chn1;
	fetch_ptr->pitch.chn2 = isp_context_ptr->fetch.pitch.chn2;

	if ((ISP_FETCH_NORMAL_RAW10 == isp_context_ptr->com.fetch_color_format)
		|| (ISP_FETCH_CSI2_RAW10 == isp_context_ptr->com.fetch_color_format)) {
		overlap_up = RAW_OVERLAP_UP_V1;
		overlap_left = RAW_OVERLAP_LEFT_V1;
	} else {
		overlap_up = YUV_OVERLAP_UP_V1;
		overlap_left = YUV_OVERLAP_LEFT_V1;
	}

	if (ISP_ZERO != cur_slice_ptr->w) {
		slice_w_offset = overlap_left;
	}

	if ((ISP_SLICE_MID == isp_context_ptr->slice.pos_info)
		|| (ISP_SLICE_LAST == isp_context_ptr->slice.pos_info)) {
		slice_h_offset = overlap_up;
	}

	switch (isp_context_ptr->com.fetch_color_format) {
	case ISP_FETCH_YUV422_3FRAME:
		ch0_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		ch1_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) >> 0x01;
		ch2_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) >> 0x01;
		break;
	case ISP_FETCH_YUV422_2FRAME:
	case ISP_FETCH_YVU422_2FRAME:
		ch0_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		ch1_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		break;
	case ISP_FETCH_YUYV:
	case ISP_FETCH_UYVY:
	case ISP_FETCH_YVYU:
	case ISP_FETCH_VYUY:
	case ISP_FETCH_NORMAL_RAW10:
		ch0_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) << 0x01;
		break;
	case ISP_FETCH_CSI2_RAW10:
		ch0_offset = ((src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) * 0x05) >> 0x02;
		break;
	default:
		break;
	}

	fetch_ptr->addr.chn0 += ch0_offset;
	fetch_ptr->addr.chn1 += ch1_offset;
	fetch_ptr->addr.chn2 += ch2_offset;

	start_col = slice_width * cur_slice_ptr->w - slice_w_offset;
#endif
	end_col = start_col + fetch_width - ISP_ONE;

	fetch_ptr->mipi_byte_rel_pos = start_col & 0x0f;
	fetch_ptr->mipi_word_num = (((end_col + 1) >> 4) * 5 + mipi_word_num_end[(end_col + 1) & 0x0f]) -
								(((start_col + 1) >> 4) * 5 + mipi_word_num_start[(start_col + 1) & 0x0f]) + 1;

	return rtn;
}

isp_s32 isp_set_fetch_param_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_interface_param_v1 *isp_context_ptr = (struct isp_interface_param_v1 *)isp_handler;
	struct isp_dev_fetch_info_v1 *fetch_param_ptr = &isp_context_ptr->fetch;
	struct isp_dev_block_addr *fetch_addr = &fetch_param_ptr->fetch_addr;

	fetch_param_ptr->bypass = ISP_ZERO;
	fetch_param_ptr->subtract = ISP_ZERO;
	fetch_param_ptr->size.width = isp_context_ptr->data.input_size.w;
	fetch_param_ptr->size.height = isp_context_ptr->data.input_size.h;
	fetch_param_ptr->color_format = isp_get_fetch_format(isp_context_ptr->data.input_format);
	fetch_addr->img_offset.chn0 = isp_context_ptr->data.input_addr.chn0;
	fetch_addr->img_offset.chn1 = isp_context_ptr->data.input_addr.chn1;
	fetch_addr->img_offset.chn2 = isp_context_ptr->data.input_addr.chn2;
	fetch_addr->img_vir.chn0 = isp_context_ptr->data.input_vir.chn0;
	fetch_addr->img_vir.chn1 = isp_context_ptr->data.input_vir.chn1;
	fetch_addr->img_vir.chn2 = isp_context_ptr->data.input_vir.chn2;
	fetch_addr->img_fd = isp_context_ptr->data.input_fd;
	isp_get_fetch_pitch(&(fetch_param_ptr->pitch), isp_context_ptr->data.input_size.w,
		isp_context_ptr->data.input_format);

	ISP_LOGI("fetch format %d\n", fetch_param_ptr->color_format);

	return rtn;
}

static enum isp_store_format isp_get_store_format(enum isp_format in_format)
{
	enum isp_store_format format = ISP_STORE_FORMAT_MAX;

	switch (in_format) {
	case ISP_DATA_UYVY:
		format = ISP_STORE_UYVY;
		break;
	case ISP_DATA_YUV422_2FRAME:
		format = ISP_STORE_YUV422_2FRAME;
		break;
	case ISP_DATA_YVU422_2FRAME:
		format = ISP_STORE_YVU422_2FRAME;
		break;
	case ISP_DATA_YUV422_3FRAME:
		format = ISP_STORE_YUV422_3FRAME;
		break;
	case ISP_DATA_YUV420_2FRAME:
		format = ISP_STORE_YUV420_2FRAME;
		break;
	case ISP_DATA_YVU420_2FRAME:
		format = ISP_STORE_YVU420_2FRAME;
		break;
	case ISP_DATA_YUV420_3_FRAME:
		format = ISP_STORE_YUV420_3FRAME;
		break;
	default:
		break;
	}

	return format;
}

static isp_s32 isp_get_store_pitch(struct isp_pitch *pitch_ptr, isp_u16 width, enum isp_format format)
{
	isp_s32 rtn = ISP_SUCCESS;

	pitch_ptr->chn0 = ISP_ZERO;
	pitch_ptr->chn1 = ISP_ZERO;
	pitch_ptr->chn2 = ISP_ZERO;

	switch (format) {
	case ISP_DATA_YUV422_3FRAME:
	case ISP_DATA_YUV420_3_FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width >> ISP_ONE;
		pitch_ptr->chn2 = width >> ISP_ONE;
		break;
	case ISP_DATA_YUV422_2FRAME:
	case ISP_DATA_YVU422_2FRAME:
	case ISP_DATA_YUV420_2FRAME:
	case ISP_DATA_YVU420_2FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width;
		break;
	case ISP_DATA_UYVY:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	default:
		break;
	}

	return rtn;
}

isp_s32 isp_get_store_addr_v1(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_store_info_v1 *store_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_size *cur_slice_ptr = &isp_context_ptr->slice.cur_slice_num;
	isp_u32 ch0_offset = ISP_ZERO;
	isp_u32 ch1_offset = ISP_ZERO;
	isp_u32 ch2_offset = ISP_ZERO;
	isp_u16 slice_width = isp_context_ptr->slice.max_size.w;
	isp_u16 slice_height = isp_context_ptr->slice.max_size.h;

	store_ptr->bypass = isp_context_ptr->store.bypass;
	store_ptr->color_format = isp_context_ptr->store.color_format;
	store_ptr->pitch.chn0 = isp_context_ptr->store.pitch.chn0;
	store_ptr->pitch.chn1 = isp_context_ptr->store.pitch.chn1;
	store_ptr->pitch.chn2 = isp_context_ptr->store.pitch.chn2;
	store_ptr->addr.chn0 = isp_context_ptr->store.addr.chn0;
	store_ptr->addr.chn1 = isp_context_ptr->store.addr.chn1;
	store_ptr->addr.chn2 = isp_context_ptr->store.addr.chn2;

	switch (isp_context_ptr->com.store_color_format) {
	case ISP_STORE_YUV422_3FRAME:
	case ISP_STORE_YUV420_3FRAME:
		ch0_offset = slice_width * cur_slice_ptr->w;
		ch1_offset = (slice_width * cur_slice_ptr->w) >> 0x01;
		ch2_offset = (slice_width * cur_slice_ptr->w) >> 0x01;
		break;
	case ISP_STORE_YUV422_2FRAME:
	case ISP_STORE_YVU422_2FRAME:
	case ISP_STORE_YUV420_2FRAME:
	case ISP_STORE_YVU420_2FRAME:
		ch0_offset = slice_width * cur_slice_ptr->w;
		ch1_offset = slice_width * cur_slice_ptr->w;
		break;
	case ISP_STORE_UYVY:
		ch0_offset = (slice_width * cur_slice_ptr->w) << 0x01;
		break;
	default:
		break;
	}

	store_ptr->addr.chn0 += ch0_offset;
	store_ptr->addr.chn1 += ch1_offset;
	store_ptr->addr.chn2 += ch2_offset;

	return rtn;
}

isp_s32 isp_set_store_param_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_interface_param_v1 *isp_context_ptr = (struct isp_interface_param_v1 *)isp_handler;
	struct isp_dev_store_info *store_param_ptr = &isp_context_ptr->store;

	store_param_ptr->bypass = 0;
	store_param_ptr->border.right_border = 0;
	store_param_ptr->border.left_border = 0;
	store_param_ptr->border.up_border = 0;
	store_param_ptr->border.down_border = 0;

	store_param_ptr->size.width = isp_context_ptr->data.input_size.w;
	store_param_ptr->size.height = isp_context_ptr->data.input_size.h;
	store_param_ptr->endian = ISP_ENDIAN_LITTLE;

	store_param_ptr->color_format = isp_get_store_format(isp_context_ptr->data.output_format);
	store_param_ptr->addr.chn0 = isp_context_ptr->data.output_addr.chn0;
	store_param_ptr->addr.chn1 = isp_context_ptr->data.output_addr.chn1;
	store_param_ptr->addr.chn2 = isp_context_ptr->data.output_addr.chn2;
	isp_get_store_pitch(&(store_param_ptr->pitch), isp_context_ptr->data.input_size.w,
		isp_context_ptr->data.output_format);

	store_param_ptr->rd_ctrl = 0;
	store_param_ptr->shadow_clr = 1;
	store_param_ptr->shadow_clr_sel =1;
	store_param_ptr->store_res = 1;

	return rtn;
}

isp_s32 isp_set_slice_size_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_interface_param_v1 *isp_context_ptr = (struct isp_interface_param_v1 *)isp_handler;
	struct isp_slice_param_v1 *isp_slice_ptr = &isp_context_ptr->slice;
	isp_s32 i = 0;

	for (i = 0; i < ISP_SLICE_TYPE_MAX_V1; i++) {
		isp_slice_ptr->size[i].x = 0;
		isp_slice_ptr->size[i].y = 0;
		isp_slice_ptr->size[i].w = isp_context_ptr->data.input_size.w;
		isp_slice_ptr->size[i].h = isp_context_ptr->data.input_size.h;
	}

	return rtn;
}

isp_s32 isp_cfg_slice_size_v1(isp_handle handle, struct isp_slice_param_v1 *slice_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_1d_lsc_slice_size(handle, slice_ptr->size[ISP_LSC_V1].w, slice_ptr->size[ISP_LSC_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp 1d lsc slice size error"));

	/*rtn = isp_u_csc_pic_size(handle, slice_ptr->size[ISP_CSC_V1].w, slice_ptr->size[ISP_CSC_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp csc pic size error"));

	rtn = isp_u_bdn_slice_size(handle, slice_ptr->size[ISP_BDN_V1].w, slice_ptr->size[ISP_BDN_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp wdenoise slice size error"));

	rtn = isp_u_pwd_slice_size(handle, slice_ptr->size[ISP_PWD_V1].w, slice_ptr->size[ISP_PWD_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp pwd slice size error"));*/

	rtn = isp_u_2d_lsc_slice_size(handle, slice_ptr->size[ISP_LENS_V1].w, slice_ptr->size[ISP_LENS_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp 2d lsc slice size error"));

	rtn = isp_u_raw_aem_slice_size(handle, slice_ptr->size[ISP_AEM_V1].w, slice_ptr->size[ISP_AEM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp raw aem slice size error"));

	/*rtn = isp_u_yiq_aem_slice_size(handle, slice_ptr->size[ISP_Y_AEM_V1].w, slice_ptr->size[ISP_Y_AEM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp yiq aem slice size error"));*/

	rtn = isp_u_raw_afm_slice_size(handle, slice_ptr->size[ISP_RGB_AFM_V1].w, slice_ptr->size[ISP_RGB_AFM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp raw afm slice size error"));

	/*rtn = isp_u_yiq_afm_slice_size(handle, slice_ptr->size[ISP_Y_AFM_V1].w, slice_ptr->size[ISP_Y_AFM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp yiq afm slice size error"));*/

	rtn = isp_u_hist_slice_size(handle, slice_ptr->size[ISP_HISTS_V1].w, slice_ptr->size[ISP_HISTS_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp hist slice size error"));

	rtn = isp_u_dispatch_ch0_size(handle, slice_ptr->size[ISP_DISPATCH_V1].w, slice_ptr->size[ISP_DISPATCH_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp dispatch ch0 size error"));

	rtn = isp_u_fetch_slice_size(handle, slice_ptr->size[ISP_FETCH_V1].w, slice_ptr->size[ISP_FETCH_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp fetch slice size error"));

	rtn = isp_u_store_slice_size(handle, slice_ptr->size[ISP_STORE_V1].w, slice_ptr->size[ISP_STORE_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp store slice size error"));

	return rtn;
}

isp_s32 isp_set_comm_param_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_interface_param_v1 *isp_context_ptr = (struct isp_interface_param_v1 *)isp_handler;
	struct isp_dev_common_info_v1 *com_param_ptr = &isp_context_ptr->com;

	if (ISP_EMC_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_sel_0 = 0x2;
		com_param_ptr->store_sel_0 = 0x2;
		com_param_ptr->fetch_sel_1 = 0x3;
		com_param_ptr->store_sel_1 = 0x3;
	} else if (ISP_CAP_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_sel_0 = 0x0;
		com_param_ptr->store_sel_0 = 0x0;
		com_param_ptr->fetch_sel_1 = 0x3;
		com_param_ptr->store_sel_1 = 0x3;
	}

	com_param_ptr->fetch_color_format= isp_context_ptr->data.input_format;
	com_param_ptr->store_color_format= isp_context_ptr->data.output_format;

	com_param_ptr->fetch_color_space_sel= 0x0;
	com_param_ptr->store_color_space_sel= 0x2;

	com_param_ptr->ch0_path_ctrl = 0;
	com_param_ptr->bin_pos_sel = 0;
	com_param_ptr->ram_mask = 0;

	if (isp_context_ptr->data.input_format == ISP_DATA_NORMAL_RAW10 ||
		isp_context_ptr->data.input_format == ISP_DATA_CSI2_RAW10) {
		com_param_ptr->store_out_path_sel = 0x0;
	} else {
		com_param_ptr->store_out_path_sel = 0x1;
	}

	com_param_ptr->shadow_ctrl_ch0.shadow_mctrl = 1;
	com_param_ptr->shadow_ctrl_ch1.shadow_mctrl = 1;
	com_param_ptr->lbuf_off.ydly_lbuf_offset = 0x119;
	com_param_ptr->lbuf_off.comm_lbuf_offset = 0x460;

	com_param_ptr->gclk_ctrl_rrgb = 0xffffffff;
	com_param_ptr->gclk_ctrl_yiq_frgb = 0xffffffff;
	com_param_ptr->gclk_ctrl_yuv = 0xffffffff;
	com_param_ptr->gclk_ctrl_scaler_3dnr = 0xffff;

	return rtn;
}

isp_s32 isp_cfg_comm_data_v1(isp_handle handle, struct isp_dev_common_info_v1 *param_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_comm_block(handle, (void *)param_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("store block cfg error"));

	return rtn;
}

isp_s32 isp_cfg_all_shadow_v1(isp_handle handle, isp_u32 auto_shadow)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_shadow_ctrl_all(handle, auto_shadow);
	ISP_RETURN_IF_FAIL(rtn, ("all shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_awbm_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_awbm_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("awbm shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_ae_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_ae_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("ae shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_af_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_af_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("af shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_afl_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_afl_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("afl shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_comm_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_comm_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("comm shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_3a_single_frame_shadow_v1(isp_handle handle, isp_u32 enable)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_3a_ctrl(handle, enable);
	ISP_RETURN_IF_FAIL(rtn, ("3a shadow cfg error"));

	return rtn;
}
