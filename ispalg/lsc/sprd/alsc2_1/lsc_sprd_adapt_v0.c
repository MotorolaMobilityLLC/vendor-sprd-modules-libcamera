/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "lsc_sprd_adapt"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>
#include "lsc_sprd_adapt_v0.h"
#include "isp_adpt.h"
#include <dlfcn.h>
#include "isp_mw.h"
#include <utils/Timers.h>

#define SMART_LSC_VERSION 1


static char liblsc_path[][20] = {
	"liblsc.so",
	"libalsc.so",
	"liblsc_v1.so",
	"liblsc_v2.so",
	"liblsc_v3.so",
	"liblsc_v4.so",
};

typedef struct {
	int grid_size;
	int lpf_mode;
	int lpf_radius;
	int lpf_border;
	int border_patch;
	int border_expand;
	int shading_mode;
	int shading_pct;
} lsc2d_calib_param_t;

struct lsc_wrapper_ops {
	void (*lsc2d_grid_samples) (int w, int h, int gridx, int gridy, int *nx, int *ny);
	void (*lsc2d_calib_param_default) (lsc2d_calib_param_t * calib_param, int grid_size, int lpf_radius, int shading_pct);
	int (*lsc2d_table_preproc) (uint16_t * otp_chn[4], uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
	int (*lsc2d_table_postproc) (uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
};

static int _lsc_gain_14bits_to_16bits(unsigned short *src_14bits, unsigned short *dst_16bits, unsigned int size_bytes)
{
	unsigned int gain_compressed_bits = 14;
	unsigned int gain_origin_bits = 16;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int bit_left = 0;
	unsigned int bit_buf = 0;
	unsigned int offset = 0;
	unsigned int dst_gain_num = 0;
	unsigned int src_uncompensate_bytes = size_bytes * gain_compressed_bits % gain_origin_bits;
	unsigned int cmp_bits = size_bytes * gain_compressed_bits;
	unsigned int src_bytes = (cmp_bits + gain_origin_bits - 1) / gain_origin_bits * (gain_origin_bits / 8);

	if (0 == src_bytes || 0 != (src_bytes & 1)) {
		return 0;
	}

	for (i = 0; i < src_bytes / 2; i++) {
		bit_buf |= src_14bits[i] << bit_left;
		bit_left += 16;

		if (bit_left > gain_compressed_bits) {
			offset = 0;
			while (bit_left >= gain_compressed_bits) {
				dst_16bits[j] = (unsigned short)(bit_buf & 0x3fff);
				j++;
				bit_left -= gain_compressed_bits;
				bit_buf = (bit_buf >> gain_compressed_bits);
			}
		}
	}

	if (gain_compressed_bits == src_uncompensate_bytes) {
		dst_gain_num = j - 1;
	} else {
		dst_gain_num = j;
	}

	return dst_gain_num;
}

static int _lsc_table_wrapper(uint16_t * lsc_otp_tbl, int grid, int image_width, int image_height, int *tbl_w, int *tbl_h, uint16_t * dst_table)
{
	int ret = ISP_SUCCESS;
	lsc2d_calib_param_t calib_param;
	int lpf_radius = 16;
	int shading_pct = 100;
	int nx, ny, sx, sy;
	uint16_t *otp_chn[4], *tbl_chn[4];
	int w = image_width / 2;
	int h = image_height / 2;
	int i;
	void *lsc_handle = dlopen("libsprdlsc.so", RTLD_NOW);
	if (!lsc_handle) {
		ISP_LOGE("init_lsc_otp, fail to dlopen libsprdlsc lib");
		ret = ISP_ERROR;
		return ret;
	}

	struct lsc_wrapper_ops lsc_ops;

	lsc_ops.lsc2d_grid_samples = dlsym(lsc_handle, "lsc2d_grid_samples");
	if (!lsc_ops.lsc2d_grid_samples) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_grid_samples");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_calib_param_default = dlsym(lsc_handle, "lsc2d_calib_param_default");
	if (!lsc_ops.lsc2d_calib_param_default) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_calib_param_default");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_table_preproc = dlsym(lsc_handle, "lsc2d_table_preproc");
	if (!lsc_ops.lsc2d_table_preproc) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_table_preproc");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_table_postproc = dlsym(lsc_handle, "lsc2d_table_postproc");
	if (!lsc_ops.lsc2d_table_postproc) {
		ISP_LOGE("init_lsc_otp, fail to dlsym lsc2d_table_postproc");
		ret = ISP_ERROR;
		goto error_dlsym;
	}

	lsc_ops.lsc2d_grid_samples(w, h, grid, grid, &nx, &ny);
	sx = nx + 2;
	sy = ny + 2;

	*tbl_w = sx;
	*tbl_h = sy;

	for (i = 0; i < 4; i++) {
		otp_chn[i] = lsc_otp_tbl + ((long) i * nx * ny);
		tbl_chn[i] = dst_table + ((long) i * sx * sy);
	}

	lsc_ops.lsc2d_calib_param_default(&calib_param, grid, lpf_radius, shading_pct);

	lsc_ops.lsc2d_table_preproc(otp_chn, tbl_chn, w, h, sx, sy, &calib_param);

	lsc_ops.lsc2d_table_postproc(tbl_chn, w, h, sx, sy, &calib_param);

  error_dlsym:
	dlclose(lsc_handle);
	lsc_handle = NULL;

	return ret;
}

static void _lsc_get_otp_size_info(cmr_s32 full_img_width, cmr_s32 full_img_height, cmr_s32 * lsc_otp_width, cmr_s32 * lsc_otp_height, cmr_s32 lsc_otp_grid)
{
	*lsc_otp_width = 0;
	*lsc_otp_height = 0;

	*lsc_otp_width = (int)(full_img_width / (2 * lsc_otp_grid)) + 1;
	*lsc_otp_height = (int)(full_img_height / (2 * lsc_otp_grid)) + 1;

	if (full_img_width % (2 * lsc_otp_grid) != 0) {
		*lsc_otp_width += 1;
	}

	if (full_img_height % (2 * lsc_otp_grid) != 0) {
		*lsc_otp_height += 1;
	}
}

static void _lsc_chnl_linear_scaler(unsigned short* src_tab, int src_grid, int src_width, int src_height, unsigned short* dst_tab, int dst_grid, int dst_width, int dst_height)
{
	int i, j, start_x, start_y;
	float x_info, y_info, ratio_x1, ratio_y1, ratio_x2, ratio_y2;
	unsigned short LT, RT, LD, RD, LT_L1, LD_L1, LT_U1, RT_U1;

	for(j=1; j<dst_height-1; j++){
		for(i=1; i<dst_width-1; i++){
			x_info = ((float)dst_grid * (i-1) / src_grid) + 1.0;
			y_info = ((float)dst_grid * (j-1) / src_grid) + 1.0;
			start_x = (int)x_info;
			start_y = (int)y_info;
			ratio_x1 = x_info - start_x;
			ratio_y1 = y_info - start_y;
			ratio_x2 = 1.0 - ratio_x1;
			ratio_y2 = 1.0 - ratio_y1;
			LT = src_tab[start_y    *(src_width) + start_x];
			RT = src_tab[start_y    *(src_width) + (start_x+1)];
			LD = src_tab[(start_y+1)*(src_width) + start_x];
			RD = src_tab[(start_y+1)*(src_width) + (start_x+1)];

			if(start_x >= src_width - 1 && start_y < src_height - 1){
				LT_L1 = src_tab[start_y    *(src_width) + start_x - 1];
				LD_L1 = src_tab[(start_y+1)*(src_width) + start_x - 1];
				dst_tab[j*(dst_width) + i] =  (unsigned short)((LT + (LT - LT_L1)*ratio_x1)*ratio_y2 + (LD + (LD -LD_L1)*ratio_x1)*ratio_y1);
			}else if(start_x < src_width - 1 && start_y >= src_height - 1){
				LT_U1 = src_tab[(start_y-1)*(src_width) + start_x];
				RT_U1 = src_tab[(start_y-1)*(src_width) + (start_x+1)];
				dst_tab[j*(dst_width) + i] =  (unsigned short)((LT + (LT -LT_U1)*ratio_y1)*ratio_x2 + (RT + (RT - RT_U1)*ratio_y1)*ratio_x1);
			}else if(start_x >= src_width - 1 && start_y >= src_height - 1){
				LT_L1 = src_tab[start_y    *(src_width) + start_x - 1];
				LT_U1 = src_tab[(start_y-1)*(src_width) + start_x];
				dst_tab[j*(dst_width) + i] =  (unsigned short)((LT + (LT - LT_L1)*ratio_x1)*((ratio_x1+0.00001)/(ratio_x1+ratio_y1+0.00002)) + (LT + (LT - LT_U1)*ratio_y1)*((ratio_y1+0.00001)/(ratio_x1+ratio_y1+0.00002)));
			}else{
				dst_tab[j*(dst_width) + i] = (unsigned short)(LT*ratio_x2*ratio_y2 + RT*ratio_x1*ratio_y2 + LD*ratio_x2*ratio_y1 + RD*ratio_x1*ratio_y1 + 0.5);
			}
		}
	}

	// set top and bottom edge
	for(i=1; i<dst_width-1; i++){
		dst_tab [ 0*dst_width + i ] = 3*dst_tab [ 1*dst_width + i ] - 3*dst_tab [ 2*dst_width + i ] + dst_tab [ 3*dst_width + i ];
		dst_tab [ (dst_height-1)*dst_width + i ] = 3*dst_tab [ (dst_height-2)*dst_width + i ] - 3*dst_tab [ (dst_height-3)*dst_width + i ] + dst_tab [ (dst_height-4)*dst_width + i ];
	}

	// set left and right edge
	for(j=0; j<dst_height; j++){
		dst_tab [ j*dst_width + 0 ] = 3*dst_tab [ j*dst_width + 1 ] - 3*dst_tab [ j*dst_width + 2 ] + dst_tab [ j*dst_width + 3 ];
		dst_tab [ j*dst_width + (dst_width-1) ] = 3*dst_tab [ j*dst_width + (dst_width-2) ] - 3*dst_tab [ j*dst_width + (dst_width-3) ] + dst_tab [ j*dst_width + (dst_width-4) ];
	}
}

static void  _lsc_table_scaler(unsigned short *table, unsigned int src_grid, unsigned int src_width, unsigned int src_height, unsigned int dst_grid, unsigned int dst_width, unsigned int dst_height)
{
	unsigned short src_tab_r [MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short src_tab_gr[MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short src_tab_gb[MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short src_tab_b [MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short dst_tab_r [MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short dst_tab_gr[MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short dst_tab_gb[MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short dst_tab_b [MAX_WIDTH*MAX_HEIGHT] = {0};
	unsigned short* ch_r  = table;
	unsigned short* ch_gr = table + ((long)src_width * src_height);
	unsigned short* ch_gb = table + ((long)src_width * src_height * 2);
	unsigned short* ch_b  = table + ((long)src_width * src_height * 3);
	if(NULL == table){
		return;
	}

	ISP_LOGV("src:[%d, %d, %d], dst:[%d, %d, %d]", src_grid, src_width, src_height, dst_grid, dst_width, dst_height);

	//scale pre table to new gain size
	memcpy(src_tab_r , ch_r , src_width * src_height * sizeof(unsigned short));
	memcpy(src_tab_gr, ch_gr, src_width * src_height * sizeof(unsigned short));
	memcpy(src_tab_gb, ch_gb, src_width * src_height * sizeof(unsigned short));
	memcpy(src_tab_b , ch_b , src_width * src_height * sizeof(unsigned short));

	// scale src_tab to dst_tab
	_lsc_chnl_linear_scaler(src_tab_r , src_grid, src_width, src_height, dst_tab_r , dst_grid, dst_width, dst_height);
	_lsc_chnl_linear_scaler(src_tab_gr, src_grid, src_width, src_height, dst_tab_gr, dst_grid, dst_width, dst_height);
	_lsc_chnl_linear_scaler(src_tab_gb, src_grid, src_width, src_height, dst_tab_gb, dst_grid, dst_width, dst_height);
	_lsc_chnl_linear_scaler(src_tab_b , src_grid, src_width, src_height, dst_tab_b , dst_grid, dst_width, dst_height);

	ch_r  = table;
	ch_gr = table + ((long)dst_width * dst_height);
	ch_gb = table + ((long)dst_width * dst_height * 2);
	ch_b  = table + ((long)dst_width * dst_height * 3);
	memcpy(ch_r , dst_tab_r , dst_width * dst_height * sizeof(unsigned short));
	memcpy(ch_gr, dst_tab_gr, dst_width * dst_height * sizeof(unsigned short));
	memcpy(ch_gb, dst_tab_gb, dst_width * dst_height * sizeof(unsigned short));
	memcpy(ch_b , dst_tab_b , dst_width * dst_height * sizeof(unsigned short));
}

static cmr_s32 _lsc_calculate_otplen_chn(cmr_u32 full_width , cmr_u32 full_height , cmr_u32 lsc_grid)
{
	cmr_u32 half_width, half_height , lsc_otp_width , lsc_otp_height;
	cmr_s32 otp_len_chn;
	half_width = full_width / 2;
	half_height = full_height / 2;
	lsc_otp_width = ((half_width % lsc_grid) > 0) ? (half_width / lsc_grid + 2) : (half_width / lsc_grid + 1);
	lsc_otp_height = ((half_height % lsc_grid) > 0) ? (half_height / lsc_grid + 2) : (half_height / lsc_grid + 1);
	otp_len_chn = ((lsc_otp_width * lsc_otp_height) * 14 % 8) ? (((lsc_otp_width * lsc_otp_height) * 14 / 8)+1) : ((lsc_otp_width * lsc_otp_height) * 14 / 8);
	otp_len_chn = (otp_len_chn % 2) ? (otp_len_chn + 1) : (otp_len_chn);
	return otp_len_chn;
}

static cmr_int _lsc_parser_otp(struct lsc_adv_init_param *lsc_param)
{
	int ret = ISP_SUCCESS;
	struct sensor_otp_data_info *lsc_otp_info;
	struct sensor_otp_data_info *oc_otp_info;
	cmr_u8 *module_info;
	cmr_u32 full_img_width = lsc_param->img_width;
	cmr_u32 full_img_height = lsc_param->img_height;
	cmr_u32 lsc_otp_grid = lsc_param->grid;
	cmr_u8 *lsc_otp_addr = NULL;
	cmr_u16 lsc_otp_len = 0;
	cmr_s32 compressed_lens_bits = 14;
	cmr_s32 lsc_otp_width, lsc_otp_height;
	cmr_s32 lsc_otp_len_chn = 0;
	cmr_s32 lsc_otp_chn_gain_num = 0;
	cmr_s32 gain_w, gain_h;
	uint16_t *lsc_table = NULL;
	cmr_u8 *oc_otp_data = NULL;
	cmr_u16 oc_otp_len;
	cmr_u8 *otp_data_ptr;
	cmr_u32 otp_data_len;
	cmr_u32 resolution = 0;
	struct sensor_otp_section_info *lsc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *oc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;
	cmr_u32 otp_raw_img_width = full_img_width;
	cmr_u32 otp_raw_img_height = full_img_height;
	double grid_ratio = 1.0;
	unsigned int param_table_width = lsc_param->gain_width;
	unsigned int param_table_height = lsc_param->gain_height;
	unsigned int param_grid = lsc_param->grid;

	// case for isp2.0 of 8M and 13M
	if((3264-100 <= full_img_width && full_img_width <= 3264+100 && 2448-100 <= full_img_height && full_img_height <= 2448+100 && lsc_param->grid == 128)
	 ||(4224-100 <= full_img_width && full_img_width <= 4224+100 && 3136-100 <= full_img_height && full_img_height <= 3136+100 && lsc_param->grid == 128))
		lsc_otp_grid = 96;
	_lsc_get_otp_size_info(full_img_width, full_img_height, &lsc_otp_width, &lsc_otp_height, lsc_otp_grid);

	if (NULL != lsc_param->otp_info_ptr) {
		struct sensor_otp_cust_info *otp_info_ptr = (struct sensor_otp_cust_info *)lsc_param->otp_info_ptr;
		if (otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE) {
			lsc_otp_info_ptr = otp_info_ptr->single_otp.lsc_info;
			oc_otp_info_ptr = otp_info_ptr->single_otp.optical_center_info;
			module_info_ptr = otp_info_ptr->single_otp.module_info;
			ISP_LOGV("init_lsc_otp, single cam");
		} else if (otp_info_ptr->otp_vendor == OTP_VENDOR_SINGLE_CAM_DUAL || otp_info_ptr->otp_vendor == OTP_VENDOR_DUAL_CAM_DUAL) {
			if (lsc_param->is_master == 1) {
				lsc_otp_info_ptr = otp_info_ptr->dual_otp.master_lsc_info;
				oc_otp_info_ptr = otp_info_ptr->dual_otp.master_optical_center_info;
				module_info_ptr = otp_info_ptr->dual_otp.master_module_info;
				ISP_LOGV("init_lsc_otp, dual cam master");
			} else {
				lsc_otp_info_ptr = otp_info_ptr->dual_otp.slave_lsc_info;
				oc_otp_info_ptr = otp_info_ptr->dual_otp.slave_optical_center_info;
				module_info_ptr = otp_info_ptr->dual_otp.slave_module_info;
				ISP_LOGV("init_lsc_otp, dual cam slave");
			}
		}
	} else {
		lsc_otp_info_ptr = NULL;
		oc_otp_info_ptr = NULL;
		module_info_ptr = NULL;
		ISP_LOGE("lsc otp_info_ptr is NULL");
	}

	if (NULL != module_info_ptr) {
		module_info = (cmr_u8 *) module_info_ptr->rdm_info.data_addr;

		if (NULL == module_info) {
			ISP_LOGE("lsc module_info is NULL");
			goto EXIT;
		}

		if ((module_info[4] == 0 && module_info[5] == 1)
			|| (module_info[4] == 0 && module_info[5] == 2)
			|| (module_info[4] == 0 && module_info[5] == 3)
			|| (module_info[4] == 0 && module_info[5] == 4)
			|| (module_info[4] == 0 && module_info[5] == 5)
			|| (module_info[4] == 1 && module_info[5] == 0 && (module_info[0] != 0x53 || module_info[1] != 0x50 || module_info[2] != 0x52 || module_info[3] != 0x44))
			|| (module_info[4] == 2 && module_info[5] == 0)
			|| (module_info[4] == 3 && module_info[5] == 0)
			|| (module_info[4] == 4 && module_info[5] == 0)
			|| (module_info[4] == 5 && module_info[5] == 0)) {
			ISP_LOGV("lsc otp map v0.4 or v0.5");
			if (NULL != lsc_otp_info_ptr && NULL != oc_otp_info_ptr) {
				lsc_otp_info = &lsc_otp_info_ptr->rdm_info;
				oc_otp_info = &oc_otp_info_ptr->rdm_info;

					if(NULL != lsc_otp_info && NULL!= oc_otp_info ){
					lsc_otp_addr = (cmr_u8 *) lsc_otp_info->data_addr;
					lsc_otp_len = lsc_otp_info->data_size;
					lsc_otp_len_chn = lsc_otp_len / 4;
					lsc_otp_chn_gain_num = lsc_otp_len_chn * 8 / compressed_lens_bits;
					oc_otp_data = (cmr_u8 *) oc_otp_info->data_addr;
					oc_otp_len = oc_otp_info->data_size;
					}else{
						ISP_LOGE("lsc lsc_otp_info = %p, oc_otp_info = %p. Parser fail !", lsc_otp_info, oc_otp_info);
						goto EXIT;
						}
			} else {
				ISP_LOGE("lsc otp_info_lsc_ptr = %p, otp_info_optical_center_ptr = %p. Parser fail !", lsc_otp_info_ptr, oc_otp_info_ptr);
				goto EXIT;
			}
		} else if (module_info[4] == 1 && (module_info[5] == 0 || module_info[5] == 1) && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
			ISP_LOGV("lsc otp map v1.0 or v1.1");
			if (NULL != lsc_otp_info_ptr) {
				otp_data_ptr = lsc_otp_info_ptr->rdm_info.data_addr;
				otp_data_len = lsc_otp_info_ptr->rdm_info.data_size;

				if(otp_data_ptr != NULL && otp_data_len != 0 ){
					lsc_otp_addr = otp_data_ptr + 1 + 16 + 5;
					lsc_otp_len = otp_data_len - 1 - 16 - 5;
				}else{
					ISP_LOGE("lsc otp_data_ptr = %p, otp_data_len = %d. Parser fail !", otp_data_ptr, otp_data_len);
					goto EXIT;
				}

				otp_raw_img_width   = (cmr_u32)((otp_data_ptr[18] << 8) | otp_data_ptr[17]);
				otp_raw_img_height  = (cmr_u32)((otp_data_ptr[20] << 8) | otp_data_ptr[19]);
				lsc_otp_grid = (cmr_u32)otp_data_ptr[21];

				_lsc_get_otp_size_info(otp_raw_img_width, otp_raw_img_height, &lsc_otp_width, &lsc_otp_height, lsc_otp_grid);

				full_img_width = otp_raw_img_width;
				full_img_height = otp_raw_img_height;

				resolution = (full_img_width * full_img_height + 500000) / 1000000;
				switch (resolution) {
				case 16:
				case 13:
				case 12:
				case 8:
				case 5:
				case 4:
				case 2:
					lsc_otp_len_chn = _lsc_calculate_otplen_chn(otp_raw_img_width, otp_raw_img_height,lsc_otp_grid);
					break;
				default:
					ISP_LOGW("not support resolution now , may be add later");
					lsc_otp_len_chn = 0;
					break;
				}
				ISP_LOGV("resolution:%d , lsc otp len chn is:%d" , resolution , lsc_otp_len_chn);
				lsc_otp_chn_gain_num = lsc_otp_len_chn * 8 / compressed_lens_bits;

				oc_otp_data = otp_data_ptr + 1;
				oc_otp_len = 16;
			} else {
				ISP_LOGE("lsc lsc_otp_info_ptr = %p. Parser fail !", lsc_otp_info_ptr);
				goto EXIT;
			}
		} else {
			ISP_LOGE("lsc otp map version error");
			goto EXIT;
		}
	} else {
		ISP_LOGE("lsc module_info_ptr = %p. Parser fail !", module_info_ptr);
		goto EXIT;
	}

	ISP_LOGV("init_lsc_otp, full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d", full_img_width, full_img_height, lsc_otp_grid);
	ISP_LOGV("init_lsc_otp, before, lsc_otp_chn_gain_num=%d", lsc_otp_chn_gain_num);

	if (lsc_otp_chn_gain_num < 100 || lsc_otp_grid < 32 || lsc_otp_grid > 256 || full_img_width < 800 || full_img_height < 600) {
		ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d", lsc_otp_len, full_img_width, full_img_height, lsc_otp_grid);
		goto EXIT;
	}

	if (lsc_otp_chn_gain_num != lsc_otp_width * lsc_otp_height) {
		ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num, lsc_otp_width,
				 lsc_otp_height, lsc_otp_grid);
		goto EXIT;
	}

	cmr_u32 lsc_ori_chn_len = (cmr_u32)(lsc_otp_chn_gain_num * sizeof(uint16_t));

	if ((lsc_otp_addr != NULL) && (lsc_otp_len != 0)) {

		uint16_t *lsc_16_bits = (uint16_t *) malloc(lsc_ori_chn_len * 4);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + ((long)lsc_otp_len_chn * 0)), lsc_16_bits + ((long)lsc_otp_chn_gain_num * 0), lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + ((long)lsc_otp_len_chn * 1)), lsc_16_bits + ((long)lsc_otp_chn_gain_num * 1), lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + ((long)lsc_otp_len_chn * 2)), lsc_16_bits + ((long)lsc_otp_chn_gain_num * 2), lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + ((long)lsc_otp_len_chn * 3)), lsc_16_bits + ((long)lsc_otp_chn_gain_num * 3), lsc_otp_chn_gain_num);

		lsc_table = (uint16_t *) malloc(32 * 32 * 4);
		ret = _lsc_table_wrapper(lsc_16_bits, lsc_otp_grid, otp_raw_img_width, otp_raw_img_height, &gain_w, &gain_h, lsc_table);	//  wrapper otp table

		free(lsc_16_bits);
		if (ret != ISP_SUCCESS) {
			ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num,
					 lsc_otp_width, lsc_otp_height, lsc_otp_grid);
			goto EXIT;
		}

		// scale otp table to param table size
		if(gain_w !=param_table_width || gain_h != param_table_height){

			grid_ratio = 1.0;
			if((gain_w > param_table_width || gain_h > param_table_height) && (lsc_otp_grid > param_grid))
				grid_ratio = 2.0; // otp use full size image, camera is binning size image
			else if((gain_w < param_table_width || gain_h < param_table_height) && (lsc_otp_grid < param_grid))
				grid_ratio = 0.5; // otp use bining size image, camera is full size image
			else if((gain_w-1 == param_table_width || gain_h-1 == param_table_height) && (lsc_otp_grid*2 == param_grid))
				grid_ratio = 0.5; // otp use bining size image (this raw image is without crop), camera is full size image

			_lsc_table_scaler(lsc_table, lsc_otp_grid, gain_w, gain_h, (unsigned int)(param_grid*grid_ratio), param_table_width, param_table_height);
			gain_w = lsc_param->gain_width;
			gain_h = lsc_param->gain_height;
			lsc_otp_grid = lsc_param->grid;
		}

		lsc_param->lsc_otp_table_width = gain_w;
		lsc_param->lsc_otp_table_height = gain_h;
		lsc_param->lsc_otp_grid = lsc_otp_grid;
		lsc_param->lsc_otp_table_addr = lsc_table;
		lsc_param->lsc_otp_table_en = 1;

		ISP_LOGV("init_lsc_otp, lsc_otp_width=%d, lsc_otp_height=%d, gain_w=%d, gain_h=%d, lsc_otp_grid=%d", lsc_otp_width, lsc_otp_height, gain_w, gain_h, lsc_otp_grid);
		ISP_LOGV("init_lsc_otp, lsc_table0_RGGB=[%d,%d,%d,%d]", lsc_table[0], lsc_table[gain_w * gain_h], lsc_table[gain_w * gain_h * 2], lsc_table[gain_w * gain_h * 3]);
		ISP_LOGV("init_lsc_otp, lsc_table1_RGGB=[%d,%d,%d,%d]", lsc_table[gain_w + 1], lsc_table[gain_w * gain_h + gain_w + 1], lsc_table[gain_w * gain_h * 2 + gain_w + 1],
				 lsc_table[gain_w * gain_h * 3 + gain_w + 1]);
	} else {
		ISP_LOGE("lsc_otp_addr = %p, lsc_otp_len = %d. Parser lsc otp fail", lsc_otp_addr, lsc_otp_len);
		ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num, lsc_otp_width,
				 lsc_otp_height, lsc_otp_grid);
		goto EXIT;
	}

	if (NULL != oc_otp_data && 0 != oc_otp_len) {
		lsc_param->lsc_otp_oc_r_x = (oc_otp_data[1] << 8) | oc_otp_data[0];
		lsc_param->lsc_otp_oc_r_y = (oc_otp_data[3] << 8) | oc_otp_data[2];
		lsc_param->lsc_otp_oc_gr_x = (oc_otp_data[5] << 8) | oc_otp_data[4];
		lsc_param->lsc_otp_oc_gr_y = (oc_otp_data[7] << 8) | oc_otp_data[6];
		lsc_param->lsc_otp_oc_gb_x = (oc_otp_data[9] << 8) | oc_otp_data[8];
		lsc_param->lsc_otp_oc_gb_y = (oc_otp_data[11] << 8) | oc_otp_data[10];
		lsc_param->lsc_otp_oc_b_x = (oc_otp_data[13] << 8) | oc_otp_data[12];
		lsc_param->lsc_otp_oc_b_y = (oc_otp_data[15] << 8) | oc_otp_data[14];
		lsc_param->lsc_otp_oc_en = 1;

		ISP_LOGV("init_lsc_otp, lsc_otp_oc_r=[%d,%d], lsc_otp_oc_gr=[%d,%d], lsc_otp_oc_gb=[%d,%d], lsc_otp_oc_b=[%d,%d] ",
				 lsc_param->lsc_otp_oc_r_x,
				 lsc_param->lsc_otp_oc_r_y,
				 lsc_param->lsc_otp_oc_gr_x, lsc_param->lsc_otp_oc_gr_y, lsc_param->lsc_otp_oc_gb_x, lsc_param->lsc_otp_oc_gb_y, lsc_param->lsc_otp_oc_b_x, lsc_param->lsc_otp_oc_b_y);
	} else {
		ISP_LOGE("oc_otp_data = %p, oc_otp_len = %d, Parser OC otp fail", oc_otp_data, oc_otp_len);
		goto EXIT;
	}
	return LSC_SUCCESS;

  EXIT:
	if(lsc_param->lsc_otp_table_addr != NULL)
	{
		free(lsc_param->lsc_otp_table_addr);
		lsc_param->lsc_otp_table_addr = NULL;
	}
	lsc_param->lsc_otp_table_en = 0;
	lsc_param->lsc_otp_oc_en = 0;
	lsc_param->lsc_otp_oc_r_x = 0;
	lsc_param->lsc_otp_oc_r_y = 0;
	lsc_param->lsc_otp_oc_gr_x = 0;
	lsc_param->lsc_otp_oc_gr_y = 0;
	lsc_param->lsc_otp_oc_gb_x = 0;
	lsc_param->lsc_otp_oc_gb_y = 0;
	lsc_param->lsc_otp_oc_b_x = 0;
	lsc_param->lsc_otp_oc_b_y = 0;
	lsc_param->lsc_otp_oc_en = 0;
	return LSC_ERROR;
}

static cmr_s32 _lscsprd_unload_lib(struct lsc_ctrl_context *cxt)
{
	cmr_s32 rtn = LSC_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to check param, Param is NULL");
		rtn = LSC_PARAM_NULL;
		goto exit;
	}

	if (cxt->lib_handle) {
		dlclose(cxt->lib_handle);
		cxt->lib_handle = NULL;
	}

  exit:
	return rtn;
}

static cmr_s32 _lscsprd_load_lib(struct lsc_ctrl_context *cxt)
{
	cmr_s32 rtn = LSC_SUCCESS;
	cmr_u32 v_count = 0;
	cmr_u32 version_id = 0;

	if (NULL == cxt) {
		ISP_LOGE("fail to check param,Param is NULL");
		rtn = LSC_PARAM_NULL;
		goto exit;
	}

	version_id = cxt->lib_info->version_id;
	v_count = sizeof(liblsc_path) / sizeof(liblsc_path[0]);
	if (version_id >= v_count) {
		ISP_LOGE("fail to get lsc lib version , version_id :%d", version_id);
		rtn = LSC_ERROR;
		goto exit;
	}

	ISP_LOGI("lib lsc v_count : %d, version id: %d, liblsc path :%s", v_count, version_id, liblsc_path[version_id]);

	cxt->lib_handle = dlopen(liblsc_path[version_id], RTLD_NOW);
	if (!cxt->lib_handle) {
		ISP_LOGE("fail to dlopen lsc lib");
		rtn = LSC_ERROR;
		goto exit;
	}

	cxt->lib_ops.alsc_init = dlsym(cxt->lib_handle, "lsc_adv_init");
	if (!cxt->lib_ops.alsc_init) {
		ISP_LOGE("fail to dlsym lsc_sprd_init");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_calc = dlsym(cxt->lib_handle, "lsc_adv_calculation");
	if (!cxt->lib_ops.alsc_calc) {
		ISP_LOGE("fail to dlsym lsc_sprd_calculation");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_io_ctrl = dlsym(cxt->lib_handle, "lsc_adv_ioctrl");
	if (!cxt->lib_ops.alsc_io_ctrl) {
		ISP_LOGE("fail to dlsym lsc_sprd_io_ctrl");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_deinit = dlsym(cxt->lib_handle, "lsc_adv_deinit");
	if (!cxt->lib_ops.alsc_deinit) {
		ISP_LOGE("fail to dlsym lsc_sprd_deinit");
		goto error_dlsym;
	}
	ISP_LOGI("load lsc lib success");

	return LSC_SUCCESS;

  error_dlsym:
	rtn = _lscsprd_unload_lib(cxt);

  exit:
	ISP_LOGE("fail to load lsc lib ret = %d", rtn);
	return rtn;
}

static void *lsc_sprd_init(void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_init_param *init_param = (struct lsc_adv_init_param *)in;
	void *alsc_handle = NULL;
	cmr_s32 free_otp_flag = 1;
	UNUSED(out);

	//parser lsc otp info
	if(init_param->lsc_otp_table_addr != NULL)
	{
		ISP_LOGE("initparam lsc_otp_table_addr is not NULL, please check the caller");
		free_otp_flag = 0;
		goto EXIT;
	}
	_lsc_parser_otp(init_param);

	cxt = (struct lsc_ctrl_context *)malloc(sizeof(struct lsc_ctrl_context));
	if (NULL == cxt) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc!");
		goto EXIT;
	}else{
		memset(cxt, 0, sizeof(*cxt));
	}

	cxt->dst_gain = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
	if (NULL == cxt->dst_gain) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc dst_gain!");
		goto EXIT;
	}

	cxt->lsc_buffer = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
	if (NULL == cxt->lsc_buffer) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc lsc_buffer!");
		goto EXIT;
	}
	init_param->lsc_buffer_addr = cxt->lsc_buffer;

	cxt->gain_pattern = init_param->gain_pattern;

	cxt->lib_info = &init_param->lib_param;
	cxt->ae_stat = (cmr_u32 *)malloc(1024*3*sizeof(cmr_u32));

	rtn = _lscsprd_load_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to load lsc lib");
		goto EXIT;
	}

	alsc_handle = cxt->lib_ops.alsc_init(init_param);
	if (NULL == alsc_handle) {
		ISP_LOGE("fail to do alsc init!");
		rtn = LSC_ALLOC_ERROR;
		goto EXIT;
	}
	if(init_param->lsc_otp_table_addr != NULL)
	{
		free(init_param->lsc_otp_table_addr);
	}
	cxt->alsc_handle = alsc_handle;

	pthread_mutex_init(&cxt->status_lock, NULL);

	ISP_LOGI("lsc init success rtn = %d", rtn);
	return (void *)cxt;

  EXIT:
	if((init_param->lsc_otp_table_addr != NULL) && (free_otp_flag == 1))
	{
		ISP_LOGW("error happend free lsc_otp_table_addr:%p" , init_param->lsc_otp_table_addr);
		free(init_param->lsc_otp_table_addr);
	}
	if (NULL != cxt) {
		rtn = _lscsprd_unload_lib(cxt);
		free(cxt->dst_gain);
		cxt->dst_gain = NULL;
		free(cxt->ae_stat);
		cxt->ae_stat = NULL;
		free(cxt);
		cxt = NULL;
	}

	ISP_LOGI("done rtn = %d", rtn);

	return NULL;
}

static cmr_s32 lsc_sprd_deinit(void *handle, void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	UNUSED(in);
	UNUSED(out);

	if (!handle) {
		ISP_LOGE("fail to check param!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;

	rtn = cxt->lib_ops.alsc_deinit(cxt->alsc_handle);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to do alsc deinit!");
	}

	rtn = _lscsprd_unload_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to unload lsc lib");
	}

	pthread_mutex_destroy(&cxt->status_lock);

	free(cxt->dst_gain);
	cxt->dst_gain = NULL;
	free(cxt->lsc_buffer);
	cxt->lsc_buffer = NULL;

	if(cxt->ae_stat){
		free(cxt->ae_stat);
		cxt->ae_stat = NULL;
       }
	memset(cxt, 0, sizeof(*cxt));
	free(cxt);
	cxt = NULL;
	ISP_LOGI("done rtn = %d", rtn);

	return rtn;
}

static void lsc_scl_for_ae_stat(struct lsc_ctrl_context *cxt, struct lsc_adv_calc_param *param)
{
	cmr_u32 i,j,ii,jj;
	cmr_u32 r = 0, g = 0, b = 0;
	cmr_u32 blk_num_w = param->stat_size.w;
	cmr_u32 blk_num_h = param->stat_size.h;
	cmr_u32 *r_stat = (cmr_u32*)param->stat_img.r;
	cmr_u32 *g_stat = (cmr_u32*)param->stat_img.gr;
	cmr_u32 *b_stat = (cmr_u32*)param->stat_img.b;

	blk_num_h = (blk_num_h < 32)? 32:blk_num_h;
	blk_num_w = (blk_num_w < 32)? 32:blk_num_w;
	cmr_u32 ratio_h = blk_num_h/32;
	cmr_u32 ratio_w = blk_num_w/32;

	memset(cxt->ae_stat,0,1024 * 3* sizeof(cmr_u32));

	for (i = 0; i < blk_num_h; ++i) {
		ii = (cmr_u32)(i / ratio_h);
		for (j = 0; j < blk_num_w; ++j) {
			jj = j / ratio_w;
			/*for r channel */
			r = r_stat[i * blk_num_w + j];
			/*for g channel */
			g = g_stat[i * blk_num_w + j];
			/*for b channel */
			b = b_stat[i * blk_num_w + j];

			cxt->ae_stat[ii * 32 + jj] += r;
			cxt->ae_stat[ii * 32 + jj + 1024] += g;
			cxt->ae_stat[ii * 32 + jj + 2048] += b;
		}
	}
	param->stat_img.r = cxt->ae_stat;
	param->stat_img.gr = param->stat_img.gb = &cxt->ae_stat[1024];
	param->stat_img.b = &cxt->ae_stat[2048];
	param->stat_size.w = 32;
	param->stat_size.h = 32;
}

static cmr_s32 lsc_sprd_calculation(void *handle, void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_calc_param *param = (struct lsc_adv_calc_param *)in;
	struct lsc_adv_calc_result *result = (struct lsc_adv_calc_result *)out;
	struct alsc_update_info update_info = { 0, 0, NULL };
	cmr_u32 dst_gain_size = (cmr_u32)(param->gain_width * param->gain_height * 4 * sizeof(cmr_u16));

	if (!handle) {
		ISP_LOGE("fail to check param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;
	lsc_scl_for_ae_stat(cxt,param);

	result->dst_gain = cxt->dst_gain;
	ATRACE_BEGIN(__FUNCTION__);
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);
	rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, param, result);
	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("SYSTEM_TEST -lsc_test  %dus ", (cmr_s32) ((ae_time1 - ae_time0) / 1000));
	rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, ALSC_GET_UPDATE_INFO, NULL, (void *)&update_info);
	if (update_info.can_update_dest == 1) {
		memcpy(update_info.lsc_buffer_addr, result->dst_gain, dst_gain_size);
		rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, ALSC_UNLOCK_UPDATE_FLAG, NULL, NULL);
	}

	return rtn;
}

static cmr_s32 lsc_sprd_ioctrl(void *handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;

	if (!handle) {
		ISP_LOGE("fail to check param, param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;

	rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, (enum alsc_io_ctrl_cmd)cmd, in, out);

	return rtn;
}

struct adpt_ops_type lsc_sprd_adpt_ops_ver0 = {
	.adpt_init = lsc_sprd_init,
	.adpt_deinit = lsc_sprd_deinit,
	.adpt_process = lsc_sprd_calculation,
	.adpt_ioctrl = lsc_sprd_ioctrl,
};

