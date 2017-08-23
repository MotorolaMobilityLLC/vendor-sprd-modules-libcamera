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

static int ispalg_lsc_gain_14bits_to_16bits(unsigned short *src_14bits, unsigned short *dst_16bits, unsigned int size_bytes)
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

static uint16_t *ispalg_lsc_table_wrapper(uint16_t * lsc_otp_tbl, int grid, int image_width, int image_height, int *tbl_w, int *tbl_h)
{
	int ret = ISP_SUCCESS;
	lsc2d_calib_param_t calib_param;
	int lpf_radius = 16;
	int shading_pct = 100;
	int nx, ny, sx, sy;
	uint16_t *otp_chn[4], *tbl_chn[4];
	int w = image_width / 2;
	int h = image_height / 2;
	uint16_t *lsc_table = NULL;

	void *lsc_handle = dlopen("libsprdlsc.so", RTLD_NOW);
	if (!lsc_handle) {
		ISP_LOGE("init_lsc_otp, fail to dlopen libsprdlsc lib");
		ret = ISP_ERROR;
		return lsc_table;
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

	lsc_table = (uint16_t *) malloc(4 * sx * sy * sizeof(uint16_t));

	*tbl_w = sx;
	*tbl_h = sy;

	for (int i = 0; i < 4; i++) {
		otp_chn[i] = lsc_otp_tbl + i * nx * ny;
		tbl_chn[i] = lsc_table + i * sx * sy;
	}

	lsc_ops.lsc2d_calib_param_default(&calib_param, grid, lpf_radius, shading_pct);

	lsc_ops.lsc2d_table_preproc(otp_chn, tbl_chn, w, h, sx, sy, &calib_param);

	lsc_ops.lsc2d_table_postproc(tbl_chn, w, h, sx, sy, &calib_param);

error_dlsym:
	dlclose(lsc_handle);
	lsc_handle = NULL;

	return lsc_table;
}

void get_lsc_otp_size_info(int full_img_width, int full_img_height, int* lsc_otp_width, int* lsc_otp_height, int lsc_otp_grid)
{
	*lsc_otp_width  = 0;
	*lsc_otp_height = 0;

	*lsc_otp_width  = (int)( full_img_width  / ( 2*lsc_otp_grid) ) + 1;
	*lsc_otp_height = (int)( full_img_height / ( 2*lsc_otp_grid) ) + 1;

	if ( full_img_width % ( 2*lsc_otp_grid ) !=0 ){
		*lsc_otp_width += 1;
	}

	if ( full_img_height % ( 2*lsc_otp_grid ) !=0 ){
		*lsc_otp_height += 1;
	}
}


static uint16_t *need_mv_lsc_otp(struct sensor_otp_cust_info *otp_data,
			struct isp_2d_lsc_param *lsc_tab_param_ptr,
			struct lsc_adv_init_param *lsc_param)
{
	int compressed_lens_bits = 14;
	int full_img_width  = (int)otp_data->single_otp.lsc_info.full_img_width;
	int full_img_height = (int)otp_data->single_otp.lsc_info.full_img_height;
	int lsc_otp_grid = (int)otp_data->single_otp.lsc_info.lsc_otp_grid;
	int lsc_otp_len = otp_data->single_otp.lsc_info.lsc_data_size;
	int lsc_otp_width, lsc_otp_height;
	int lsc_otp_len_chn = lsc_otp_len / 4;
	int lsc_otp_chn_gain_num = lsc_otp_len_chn * 8 / compressed_lens_bits;

	ISP_LOGI("init_lsc_otp, full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d", full_img_width, full_img_height, lsc_otp_grid);
	ISP_LOGI("init_lsc_otp, before, lsc_otp_chn_gain_num=%d", lsc_otp_chn_gain_num);

	// error handling
	if( lsc_otp_grid < 32 || lsc_otp_grid > 256 || full_img_width < 800 || full_img_height < 600){
		ISP_LOGE("init_lsc_otp, fail to sensor setting, full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d", full_img_width, full_img_height, lsc_otp_grid);
		return NULL;
	}
	get_lsc_otp_size_info(full_img_width, full_img_height, &lsc_otp_width, &lsc_otp_height, lsc_otp_grid);
	lsc_otp_chn_gain_num = lsc_otp_width*lsc_otp_height;

	ISP_LOGI("init_lsc_otp, after, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d", lsc_otp_chn_gain_num, lsc_otp_width, lsc_otp_height);

	int lsc_ori_chn_len = lsc_otp_chn_gain_num * sizeof(uint16_t);
	int gain_w, gain_h;

	uint8_t *lsc_otp_addr = otp_data->single_otp.lsc_info.lsc_data_addr;
	uint16_t *lsc_table = NULL;

	if ((lsc_otp_addr != NULL) && (lsc_otp_len != 0)) {

		uint16_t *lsc_16_bits = (uint16_t *) malloc(lsc_ori_chn_len * 4);
		ispalg_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 0),
						 lsc_16_bits + lsc_otp_chn_gain_num * 0, lsc_otp_chn_gain_num);
		ispalg_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 1),
						 lsc_16_bits + lsc_otp_chn_gain_num * 1, lsc_otp_chn_gain_num);
		ispalg_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 2),
						 lsc_16_bits + lsc_otp_chn_gain_num * 2, lsc_otp_chn_gain_num);
		ispalg_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 3),
						 lsc_16_bits + lsc_otp_chn_gain_num * 3, lsc_otp_chn_gain_num);

		lsc_table = ispalg_lsc_table_wrapper(lsc_16_bits, lsc_otp_grid,
						     lsc_tab_param_ptr->resolution.w,
						     lsc_tab_param_ptr->resolution.h,
						     &gain_w, &gain_h);	//  wrapper otp table
		free(lsc_16_bits);
		if (lsc_table == NULL) {
			return NULL;
		}
		lsc_param->lsc_otp_table_width = gain_w;
		lsc_param->lsc_otp_table_height = gain_h;
		lsc_param->lsc_otp_grid = lsc_otp_grid;
		lsc_param->lsc_otp_table_addr = lsc_table;
		lsc_param->lsc_otp_table_en = 1;
	}

	lsc_param->lsc_otp_oc_r_x = otp_data->single_otp.optical_center_info.R.x;
	lsc_param->lsc_otp_oc_r_y = otp_data->single_otp.optical_center_info.R.y;
	lsc_param->lsc_otp_oc_gr_x = otp_data->single_otp.optical_center_info.GR.x;
	lsc_param->lsc_otp_oc_gr_y = otp_data->single_otp.optical_center_info.GR.y;
	lsc_param->lsc_otp_oc_gb_x = otp_data->single_otp.optical_center_info.GB.x;
	lsc_param->lsc_otp_oc_gb_y = otp_data->single_otp.optical_center_info.GB.y;
	lsc_param->lsc_otp_oc_b_x = otp_data->single_otp.optical_center_info.B.x;
	lsc_param->lsc_otp_oc_b_y = otp_data->single_otp.optical_center_info.B.y;
	lsc_param->lsc_otp_oc_en = 1;
	return lsc_table;
}
