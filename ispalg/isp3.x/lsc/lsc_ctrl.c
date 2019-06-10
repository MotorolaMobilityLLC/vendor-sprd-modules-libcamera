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
#define LOG_TAG "lsc_ctrl"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <cutils/trace.h>
#include <cutils/properties.h>
#include "lsc_adv.h"
#include "isp_adpt.h"
#include <dlfcn.h>
#include "isp_mw.h"
#include <utils/Timers.h>
#include <sys/stat.h>
#include <math.h>
#define SMART_LSC_VERSION 1

cmr_u32 proc_start_gain_w = 0;                  // SBS master gain width
cmr_u32 proc_start_gain_h = 0;                  // SBS master gain height
cmr_u32 proc_start_gain_pattern = 0;            // SBS master gain pattern
cmr_u16 proc_start_param_table[32*32*4] = {0};  // SBS master DNP table
cmr_u16 proc_start_output_table[32*32*4] = {0}; // SBS master output table
cmr_u16 *id1_addr = NULL;
cmr_u16 *id2_addr = NULL;


struct lsc_ctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct lsc_ctrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	struct lsc_ctrl_work_lib work_lib;
	struct lsc_adv_calc_result proc_out;
};

char liblsc_path[][20] = {
	"liblsc.so",
	"liblsc_v1.so",
	"liblsc_v2.so",
	"liblsc_v3.so",
	"liblsc_v4.so",
	"liblsc_v5.so",
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

static int alsc_get_init_cmd(void)
{
	char prop[256];
	int val = 0;

	property_get("debug.isp.alsc.cmd.enable", prop, (char *)"0");
	val = atoi(prop);
	if(0 <= val)
		return val;

	return 0;
}

static void alsc_get_cmd(struct lsc_ctrl_context *cxt)
{
	char prop[256];
	int val = 0;

	property_get("debug.isp.alsc.table.pattern", prop, (char *)"0");
	val = atoi(prop);
	if(0 <= val)
		cxt->cmd_alsc_table_pattern = val;

	property_get("debug.isp.alsc.table.index", prop, (char *)"9");
	val = atoi(prop);
	if(0 <= val)
		cxt->cmd_alsc_table_index = val;

	property_get("debug.isp.alsc.dump.aem", prop, (char *)"0");
	val = atoi(prop);
	if(0 <= val)
		cxt->cmd_alsc_dump_aem = val;

	property_get("debug.isp.alsc.dump.table", prop, (char *)"0");
	val = atoi(prop);
	if(0 <= val)
		cxt->cmd_alsc_dump_table = val;

	property_get("debug.isp.alsc.bypass", prop, (char *)"0");
	val = atoi(prop);
	if(0 <= val)
		cxt->cmd_alsc_bypass = val;

	property_get("debug.isp.alsc.bypass.otp", prop, (char *)"0");
	val = atoi(prop);
	if(0 <= val)
		cxt->cmd_alsc_bypass_otp = val;
}

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

static uint16_t *_lsc_table_wrapper(uint16_t * lsc_otp_tbl, int grid, int image_width, int image_height, int *tbl_w, int *tbl_h)
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
	int i;
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

	for (i = 0; i < 4; i++) {
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

void _lsc_get_otp_size_info(cmr_s32 full_img_width, cmr_s32 full_img_height, cmr_s32 * lsc_otp_width, cmr_s32 * lsc_otp_height, cmr_s32 lsc_otp_grid)
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

void _scale_bilinear_short(unsigned short* src_buf, int src_width, int src_height, unsigned short* dst_buf, int dst_width, int dst_height){

    int i, j, x, y;
    float xx, yy;
    int a, b, c, d, tmp;

    for (j=0; j<dst_height; j++){
        float sy = (float)(j * src_height) / dst_height;
        if (sy > src_height-2) sy = (float)(src_height-2);
        y = (short)sy;
        yy = sy - y;

        for (i=0; i<dst_width; i++){
            float sx = (float)(i * src_width) / dst_width;
            if (sx > src_width-2) sx = (float)(src_width-2);
            x = (short)sx;
            xx = sx - x;

            a = src_buf[src_width * y + x];
            b = src_buf[src_width * (y+1) + x];
            c = src_buf[src_width * y + x+1];
            d = src_buf[src_width * (y+1) + x+1];

            tmp = (short)(a * (1-xx) * (1-yy) + b * (1-xx) * yy + c * xx * (1-yy) + d * xx * yy + 0.5f);

            dst_buf[dst_width * j + i] = tmp;
        }
    }
}

static void save_tab_to_channel(cmr_u32 gain_width, cmr_u32 gain_height, cmr_u32 gain_pattern, cmr_u16 *ch_r, cmr_u16 *ch_gr, cmr_u16 *ch_gb, cmr_u16 *ch_b, cmr_u16 *rlt_tab)
{
	cmr_u32 i;
	for(i=0; i<gain_width*gain_height; i++){
		switch (gain_pattern){
			case 0:
				ch_gr[i] = rlt_tab[4*i + 0];
				ch_r [i] = rlt_tab[4*i + 1];
				ch_b [i] = rlt_tab[4*i + 2];
				ch_gb[i] = rlt_tab[4*i + 3];
			break;
			case 1:
				ch_r [i] = rlt_tab[4*i + 0];
				ch_gr[i] = rlt_tab[4*i + 1];
				ch_gb[i] = rlt_tab[4*i + 2];
				ch_b [i] = rlt_tab[4*i + 3];
			break;
			case 2:
				ch_b [i] = rlt_tab[4*i + 0];
				ch_gb[i] = rlt_tab[4*i + 1];
				ch_gr[i] = rlt_tab[4*i + 2];
				ch_r [i] = rlt_tab[4*i + 3];
			break;
			case 3:
				ch_gb[i] = rlt_tab[4*i + 0];
				ch_b [i] = rlt_tab[4*i + 1];
				ch_r [i] = rlt_tab[4*i + 2];
				ch_gr[i] = rlt_tab[4*i + 3];
			break;
				default:
			break;
		}
	}
}

static void set_channel_to_tab(cmr_u32 gain_width, cmr_u32 gain_height, cmr_u32 gain_pattern, cmr_u16 *ch_r, cmr_u16 *ch_gr, cmr_u16 *ch_gb, cmr_u16 *ch_b, cmr_u16 *rlt_tab)
{
	cmr_u32 i;
	for(i=0; i<gain_width*gain_height; i++){
		switch (gain_pattern){
			case LSC_GAIN_PATTERN_GRBG:
				rlt_tab[4*i + 0] = ch_gr[i];
				rlt_tab[4*i + 1] = ch_r [i];
				rlt_tab[4*i + 2] = ch_b [i];
				rlt_tab[4*i + 3] = ch_gb[i];
			break;
			case LSC_GAIN_PATTERN_RGGB:
				rlt_tab[4*i + 0] = ch_r [i];
				rlt_tab[4*i + 1] = ch_gr[i];
				rlt_tab[4*i + 2] = ch_gb[i];
				rlt_tab[4*i + 3] = ch_b [i];
			break;
			case LSC_GAIN_PATTERN_BGGR:
				rlt_tab[4*i + 0] = ch_b [i];
				rlt_tab[4*i + 1] = ch_gb[i];
				rlt_tab[4*i + 2] = ch_gr[i];
				rlt_tab[4*i + 3] = ch_r [i];
			break;
			case LSC_GAIN_PATTERN_GBRG:
				rlt_tab[4*i + 0] = ch_gb[i];
				rlt_tab[4*i + 1] = ch_b [i];
				rlt_tab[4*i + 2] = ch_r [i];
				rlt_tab[4*i + 3] = ch_gr[i];
			break;
				default:
			break;
		}
	}
}

static void change_lsc_pattern(cmr_u16* lsc_table, cmr_u32 gain_width, cmr_u32 gain_height, cmr_u32 gain_pattern, cmr_u32 output_gain_pattern)
{
	cmr_u16 lsc_table_r [32*32*4] = {0};
	cmr_u16 lsc_table_gr[32*32*4] = {0};
	cmr_u16 lsc_table_gb[32*32*4] = {0};
	cmr_u16 lsc_table_b [32*32*4] = {0};

	save_tab_to_channel(gain_width, gain_height, gain_pattern       , lsc_table_r, lsc_table_gr, lsc_table_gb, lsc_table_b, lsc_table);
	set_channel_to_tab (gain_width, gain_height, output_gain_pattern, lsc_table_r, lsc_table_gr, lsc_table_gb, lsc_table_b, lsc_table);
}

static void _table_linear_scaler(cmr_u16 *src_tab, cmr_u32 src_width, cmr_u32 src_height, cmr_u16 *dst_tab, cmr_u32 dst_width, cmr_u32 dst_height, cmr_u32 plane_flag)
{
	ISP_LOGV("[ALSC] _table_linear_scaler, src_tab[%d,%d,%d,%d], src_width=%d, src_height=%d, dst_width=%d, dst_height=%d, plane_flag=%d",
			src_tab[0], src_tab[1], src_tab[2], src_tab[3],
			src_width, src_height, dst_width, dst_height, plane_flag);
	if(src_width == dst_width && src_height == dst_height){
		memcpy(dst_tab, src_tab, src_width*src_height*4*sizeof(cmr_u16));
	}else{
		cmr_u32 i,j;

		//scale pre table to new gain size
		cmr_u16 pre_contain_r_tab [ 32 * 32 ]={0};
		cmr_u16 pre_contain_gr_tab[ 32 * 32 ]={0};
		cmr_u16 pre_contain_gb_tab[ 32 * 32 ]={0};
		cmr_u16 pre_contain_b_tab [ 32 * 32 ]={0};
		cmr_u16 new_contain_r_tab [ 32 * 32 ]={0};
		cmr_u16 new_contain_gr_tab[ 32 * 32 ]={0};
		cmr_u16 new_contain_gb_tab[ 32 * 32 ]={0};
		cmr_u16 new_contain_b_tab [ 32 * 32 ]={0};
		cmr_u16 output_r_tab [ 32 * 32 ]={0};
		cmr_u16 output_gr_tab[ 32 * 32 ]={0};
		cmr_u16 output_gb_tab[ 32 * 32 ]={0};
		cmr_u16 output_b_tab [ 32 * 32 ]={0};
		cmr_u16* ch_r  = src_tab;
		cmr_u16* ch_gr = src_tab + src_width * src_height;
		cmr_u16* ch_gb = src_tab + src_width * src_height*2;
		cmr_u16* ch_b  = src_tab + src_width * src_height*3;

		if(plane_flag == 1){
			memcpy(output_r_tab , ch_r , src_width * src_height * sizeof(cmr_u16));
			memcpy(output_gr_tab, ch_gr, src_width * src_height * sizeof(cmr_u16));
			memcpy(output_gb_tab, ch_gb, src_width * src_height * sizeof(cmr_u16));
			memcpy(output_b_tab , ch_b , src_width * src_height * sizeof(cmr_u16));
		}else{
			save_tab_to_channel(src_width, src_height, 3, output_r_tab, output_gr_tab, output_gb_tab, output_b_tab, src_tab);
		}
		ISP_LOGV("[ALSC] _table_linear_scaler, src_rggb[%d,%d,%d,%d]",
				output_r_tab[0], output_gr_tab[0], output_gb_tab[0], output_b_tab[0]);

	// get contain from pre_tab
	for(j=0; j<src_height-2; j++){
		for(i=0; i<src_width-2; i++){
			pre_contain_r_tab [ j*(src_width-2) + i ] = output_r_tab [ (j+1)*src_width + (i+1)];
			pre_contain_gr_tab[ j*(src_width-2) + i ] = output_gr_tab[ (j+1)*src_width + (i+1)];
			pre_contain_gb_tab[ j*(src_width-2) + i ] = output_gb_tab[ (j+1)*src_width + (i+1)];
			pre_contain_b_tab [ j*(src_width-2) + i ] = output_b_tab [ (j+1)*src_width + (i+1)];
		}
	}

	// scale pre_contain to new_contain
	_scale_bilinear_short(pre_contain_r_tab , src_width-2, src_height-2, new_contain_r_tab , dst_width-2, dst_height-2);
	_scale_bilinear_short(pre_contain_gr_tab, src_width-2, src_height-2, new_contain_gr_tab, dst_width-2, dst_height-2);
	_scale_bilinear_short(pre_contain_gb_tab, src_width-2, src_height-2, new_contain_gb_tab, dst_width-2, dst_height-2);
	_scale_bilinear_short(pre_contain_b_tab , src_width-2, src_height-2, new_contain_b_tab , dst_width-2, dst_height-2);

	// set contain to output tab
	for(j=0; j<dst_height-2; j++){
		for(i=0; i<dst_width-2; i++){
			output_r_tab [ (j+1)*dst_width + (i+1)] = new_contain_r_tab [ j*(dst_width-2) + i];
			output_gr_tab[ (j+1)*dst_width + (i+1)] = new_contain_gr_tab[ j*(dst_width-2) + i];
			output_gb_tab[ (j+1)*dst_width + (i+1)] = new_contain_gb_tab[ j*(dst_width-2) + i];
			output_b_tab [ (j+1)*dst_width + (i+1)] = new_contain_b_tab [ j*(dst_width-2) + i];
		}
	}

	// set top and bottom edge
	for(i=1; i<dst_width-1; i++){
		output_r_tab [ 0*dst_width + i ] = 3*output_r_tab [ 1*dst_width + i ] - 3*output_r_tab [ 2*dst_width + i ] + output_r_tab [ 3*dst_width + i ];
		output_gr_tab[ 0*dst_width + i ] = 3*output_gr_tab[ 1*dst_width + i ] - 3*output_gr_tab[ 2*dst_width + i ] + output_gr_tab[ 3*dst_width + i ];
		output_gb_tab[ 0*dst_width + i ] = 3*output_gb_tab[ 1*dst_width + i ] - 3*output_gb_tab[ 2*dst_width + i ] + output_gb_tab[ 3*dst_width + i ];
		output_b_tab [ 0*dst_width + i ] = 3*output_b_tab [ 1*dst_width + i ] - 3*output_b_tab [ 2*dst_width + i ] + output_b_tab [ 3*dst_width + i ];
		output_r_tab [ (dst_height-1)*dst_width + i ] = 3*output_r_tab [ (dst_height-2)*dst_width + i ] - 3*output_r_tab [ (dst_height-3)*dst_width + i ] + output_r_tab [ (dst_height-4)*dst_width + i ];
		output_gr_tab[ (dst_height-1)*dst_width + i ] = 3*output_gr_tab[ (dst_height-2)*dst_width + i ] - 3*output_gr_tab[ (dst_height-3)*dst_width + i ] + output_gr_tab[ (dst_height-4)*dst_width + i ];
		output_gb_tab[ (dst_height-1)*dst_width + i ] = 3*output_gb_tab[ (dst_height-2)*dst_width + i ] - 3*output_gb_tab[ (dst_height-3)*dst_width + i ] + output_gb_tab[ (dst_height-4)*dst_width + i ];
		output_b_tab [ (dst_height-1)*dst_width + i ] = 3*output_b_tab [ (dst_height-2)*dst_width + i ] - 3*output_b_tab [ (dst_height-3)*dst_width + i ] + output_b_tab [ (dst_height-4)*dst_width + i ];
	}

	// set left and right edge
	for(j=0; j<dst_height; j++){
		output_r_tab [ j*dst_width + 0 ] = 3*output_r_tab [ j*dst_width + 1 ] - 3*output_r_tab [ j*dst_width + 2 ] + output_r_tab [ j*dst_width + 3 ];
		output_gr_tab[ j*dst_width + 0 ] = 3*output_gr_tab[ j*dst_width + 1 ] - 3*output_gr_tab[ j*dst_width + 2 ] + output_gr_tab[ j*dst_width + 3 ];
		output_gb_tab[ j*dst_width + 0 ] = 3*output_gb_tab[ j*dst_width + 1 ] - 3*output_gb_tab[ j*dst_width + 2 ] + output_gb_tab[ j*dst_width + 3 ];
		output_b_tab [ j*dst_width + 0 ] = 3*output_b_tab [ j*dst_width + 1 ] - 3*output_b_tab [ j*dst_width + 2 ] + output_b_tab [ j*dst_width + 3 ];
		output_r_tab [ j*dst_width + (dst_width-1) ] = 3*output_r_tab [ j*dst_width + (dst_width-2) ] - 3*output_r_tab [ j*dst_width + (dst_width-3) ] + output_r_tab [ j*dst_width + (dst_width-4) ];
		output_gr_tab[ j*dst_width + (dst_width-1) ] = 3*output_gr_tab[ j*dst_width + (dst_width-2) ] - 3*output_gr_tab[ j*dst_width + (dst_width-3) ] + output_gr_tab[ j*dst_width + (dst_width-4) ];
		output_gb_tab[ j*dst_width + (dst_width-1) ] = 3*output_gb_tab[ j*dst_width + (dst_width-2) ] - 3*output_gb_tab[ j*dst_width + (dst_width-3) ] + output_gb_tab[ j*dst_width + (dst_width-4) ];
		output_b_tab [ j*dst_width + (dst_width-1) ] = 3*output_b_tab [ j*dst_width + (dst_width-2) ] - 3*output_b_tab [ j*dst_width + (dst_width-3) ] + output_b_tab [ j*dst_width + (dst_width-4) ];
	}

		// merge color channels to table
		if(plane_flag == 1){
			ch_r  = dst_tab;
			ch_gr = dst_tab + dst_width * dst_height;
			ch_gb = dst_tab + dst_width * dst_height*2;
			ch_b  = dst_tab + dst_width * dst_height*3;
			memcpy(ch_r , output_r_tab , dst_width * dst_height * sizeof(cmr_u16));
			memcpy(ch_gr, output_gr_tab, dst_width * dst_height * sizeof(cmr_u16));
			memcpy(ch_gb, output_gb_tab, dst_width * dst_height * sizeof(cmr_u16));
			memcpy(ch_b , output_b_tab , dst_width * dst_height * sizeof(cmr_u16));
		}else{
			set_channel_to_tab(dst_width, dst_height, 3, output_r_tab, output_gr_tab, output_gb_tab, output_b_tab, dst_tab);
		}
	}
}

static cmr_s32 lnc_master_slave_sync(struct lsc_ctrl_context* cxt, struct alsc_fwprocstart_info* fwprocstart_info)
{
	cmr_u32 i;
    cmr_u32 pre_gain_width   = proc_start_gain_w;
    cmr_u32 pre_gain_height  = proc_start_gain_h;
    cmr_u32 pre_gain_pattern = proc_start_gain_pattern;
    cmr_u16 lsc_pre_reslut_table[32 * 32 * 4]={0};
    cmr_u16 lsc_pre_table       [32 * 32 * 4]={0};

	for (i = 0; i < pre_gain_width * pre_gain_height * 4 ; i++){
		lsc_pre_reslut_table[i] = proc_start_output_table[i];   // master output table
		lsc_pre_table[i]        = proc_start_param_table[i];    // master DNP param table
	}

	cmr_u32 new_gain_width =  fwprocstart_info->gain_width_new;
	cmr_u32 new_gain_height = fwprocstart_info->gain_height_new;;
	cmr_u32 new_gain_pattern = 3;  //for initial value
	cmr_u16 *lsc_result_address_new = fwprocstart_info->lsc_result_address_new;     // slave output table buffer
	cmr_u16 lsc_new_table         [32 *32*4]={0};                                   // slave DNP param table

	memcpy(lsc_new_table, fwprocstart_info->lsc_tab_address_new[0], new_gain_width * new_gain_height * 4 * sizeof(unsigned short));    // slave DNP param table
	ISP_LOGV("[ALSC] lnc_master_slave_sync, slave size[%d,%d], slave_DNP[%d,%d,%d,%d]", new_gain_width, new_gain_height, lsc_new_table[0], lsc_new_table[1], lsc_new_table[2], lsc_new_table[3]);

	switch (fwprocstart_info->image_pattern_new){
		case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
			new_gain_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_R:
			new_gain_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_B:
			new_gain_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
			new_gain_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

		default:
		break;
	}

	// scale master dnp table to slave size
	cmr_u16 output_tab [ 32 * 32 * 4]={0};
	cmr_u16 output_r_tab [ 32 * 32 ]={0};
	cmr_u16 output_gr_tab[ 32 * 32 ]={0};
	cmr_u16 output_gb_tab[ 32 * 32 ]={0};
	cmr_u16 output_b_tab [ 32 * 32 ]={0};

	_table_linear_scaler(lsc_pre_table, pre_gain_width, pre_gain_height, output_tab, new_gain_width, new_gain_height, 0);
	save_tab_to_channel(new_gain_width, new_gain_height, pre_gain_pattern, output_r_tab, output_gr_tab, output_gb_tab, output_b_tab, output_tab);

	// get slave dnp table
	cmr_u16 output_r_tab_new [ 32 * 32 ]={0};
	cmr_u16 output_gr_tab_new[ 32 * 32 ]={0};
	cmr_u16 output_gb_tab_new[ 32 * 32 ]={0};
	cmr_u16 output_b_tab_new [ 32 * 32 ]={0};

	save_tab_to_channel(new_gain_width, new_gain_height, new_gain_pattern, output_r_tab_new, output_gr_tab_new, output_gb_tab_new, output_b_tab_new, lsc_new_table);

	//get level weight matrix
	float lsc_new_weight_tab_gb[32*32]={0};
	float lsc_new_weight_tab_b[32*32]={0};
	float lsc_new_weight_tab_r[32*32]={0};
	float lsc_new_weight_tab_gr[32*32]={0};
	float rate_gb=0.0;
	float rate_b=0.0;
	float rate_r=0.0;
	float rate_gr=0.0;

	for(i=0; i< new_gain_width * new_gain_height ;i++){
		rate_gr=0.0;
		rate_gb=0.0;
		rate_b=0.0;
		rate_r=0.0;
		//gr
		if( output_gr_tab[i] == 0 || output_gr_tab[i]==1024 ){
			rate_gr=1;
		}else{
			rate_gr=(float)output_gr_tab_new[i]/(float)output_gr_tab[i];
		}
		lsc_new_weight_tab_gr[i]=rate_gr;
		//gb
		if( output_gb_tab[i] == 0 || output_gb_tab[i]==1024 ){
			rate_gb=1;
		}else{
			rate_gb=(float)output_gb_tab_new[i]/(float)output_gb_tab[i];
		}
		lsc_new_weight_tab_gb[i]=rate_gb;
		//r
		if( output_r_tab[i] == 0 || output_r_tab[i]==1024 ){
			rate_r=1;
		}else{
			rate_r=(float)output_r_tab_new[i]/(float)output_r_tab[i];
		}
		lsc_new_weight_tab_r[i]=rate_r;
		//b
		if( output_b_tab[i] == 0 || output_b_tab[i]==1024 ){
			rate_b=1;
		}else{
			rate_b=(float)output_b_tab_new[i]/(float)output_b_tab[i];
		}
		lsc_new_weight_tab_b[i]=rate_b;
	}

	// scale master output table to slave size
	cmr_u16 output_r [ 32 * 32 ]={0};
	cmr_u16 output_gr[ 32 * 32 ]={0};
	cmr_u16 output_gb[ 32 * 32 ]={0};
	cmr_u16 output_b [ 32 * 32 ]={0};
	_table_linear_scaler(lsc_pre_reslut_table, pre_gain_width, pre_gain_height, lsc_result_address_new, new_gain_width, new_gain_height, 0);
	save_tab_to_channel(new_gain_width, new_gain_height, pre_gain_pattern, output_r, output_gr, output_gb, output_b, lsc_result_address_new);

	//change_lsc_pattern
	if(cxt->change_pattern_flag) {
		ISP_LOGV("[ALSC] lnc_master_slave_sync, slave change lsc pattern, gain_pattern=%d, output_gain_pattern=%d", new_gain_pattern, cxt->output_gain_pattern);
		new_gain_pattern = cxt->output_gain_pattern;
	}

	// apply weight and send output to lsc_result_address_new
	for(i=0; i< new_gain_width * new_gain_height; i++){
		switch (new_gain_pattern){
			case LSC_GAIN_PATTERN_GRBG:
				lsc_result_address_new[4*i + 0] = output_gr[i]*lsc_new_weight_tab_gr[i];
				lsc_result_address_new[4*i + 1] = output_r [i]*lsc_new_weight_tab_r[i];
				lsc_result_address_new[4*i + 2] = output_b [i]*lsc_new_weight_tab_b[i];
				lsc_result_address_new[4*i + 3] = output_gb[i]*lsc_new_weight_tab_gb[i];
			break;
			case LSC_GAIN_PATTERN_RGGB:
				lsc_result_address_new[4*i + 0] = output_r [i]*lsc_new_weight_tab_r[i];
				lsc_result_address_new[4*i + 1] = output_gr[i]*lsc_new_weight_tab_gr[i];
				lsc_result_address_new[4*i + 2] = output_gb[i]*lsc_new_weight_tab_gb[i];
				lsc_result_address_new[4*i + 3] = output_b [i]*lsc_new_weight_tab_b[i];
			break;
			case LSC_GAIN_PATTERN_BGGR:
				lsc_result_address_new[4*i + 0] = output_b [i]*lsc_new_weight_tab_b[i];
				lsc_result_address_new[4*i + 1] = output_gb[i]*lsc_new_weight_tab_gb[i];
				lsc_result_address_new[4*i + 2] = output_gr[i]*lsc_new_weight_tab_gr[i];
				lsc_result_address_new[4*i + 3] = output_r [i]*lsc_new_weight_tab_r[i];
			break;
			case LSC_GAIN_PATTERN_GBRG:
				lsc_result_address_new[4*i + 0] = output_gb[i]*lsc_new_weight_tab_gb[i];
				lsc_result_address_new[4*i + 1] = output_b [i]*lsc_new_weight_tab_b[i];
				lsc_result_address_new[4*i + 2] = output_r [i]*lsc_new_weight_tab_r[i];
				lsc_result_address_new[4*i + 3] = output_gr[i]*lsc_new_weight_tab_gr[i];
			break;
			default:
			break;
		}
	}

	//cliping for table max value 16383, min 1024
	for(i=0; i< new_gain_width * new_gain_height * 4; i++){
		if(lsc_result_address_new[i] > 16383) lsc_result_address_new[i]=16383;
		if(lsc_result_address_new[i] < 1024) lsc_result_address_new[i]=1024;
	}
	ISP_LOGV("[ALSC] lnc_master_slave_sync, slave output table address %p,  output table=[%d,%d,%d,%d]", lsc_result_address_new,
			lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);

    //keep the update for calc to as a source for inversing static data
    memcpy(cxt->fwstart_new_scaled_table, lsc_result_address_new, new_gain_width*new_gain_height*4*sizeof(unsigned short));

    ISP_LOGV("[ALSC] lnc_master_slave_sync, SBS slave output Done");
    return 0;
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

cmr_int _lsc_parser_otp(struct lsc_adv_init_param *lsc_param)
{
	struct sensor_otp_data_info *lsc_otp_info;
	struct sensor_otp_data_info *oc_otp_info;
	cmr_u8 *module_info;
	cmr_u32 full_img_width = lsc_param->img_width;
	cmr_u32 full_img_height = lsc_param->img_height;
	cmr_u32 lsc_otp_grid = lsc_param->grid;
	cmr_u8 *lsc_otp_addr;
	cmr_u16 lsc_otp_len;
	cmr_s32 compressed_lens_bits = 14;
	cmr_s32 lsc_otp_width, lsc_otp_height;
	cmr_s32 lsc_otp_len_chn;
	cmr_s32 lsc_otp_chn_gain_num;
	cmr_s32 gain_w, gain_h;
	uint16_t *lsc_table = NULL;
	cmr_u8 *oc_otp_data;
	cmr_u16 oc_otp_len;
	cmr_u8 *otp_data_ptr;
	cmr_u32 otp_data_len;
	cmr_u32 resolution = 0;
	struct sensor_otp_section_info *lsc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *oc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;

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

					if(lsc_otp_info != NULL && oc_otp_info != NULL){
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
		} else if (module_info[4] == 1 && module_info[5] == 0 && module_info[0] == 0x53 && module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
			ISP_LOGV("lsc otp map v1.0");
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

				resolution = (full_img_width * full_img_height + 500000) / 1000000;
				switch (resolution) {
				case 16:
				case 13:
				case 12:
				case 8:
				case 5:
				case 4:
				case 2:
					lsc_otp_len_chn = _lsc_calculate_otplen_chn(full_img_width, full_img_height,lsc_otp_grid);
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
		lsc_otp_chn_gain_num = lsc_otp_width * lsc_otp_height;
		ISP_LOGD("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num, lsc_otp_width,
				 lsc_otp_height, lsc_otp_grid);
		//goto EXIT;
	}

	cmr_s32 lsc_ori_chn_len = lsc_otp_chn_gain_num * sizeof(uint16_t);

	if ((lsc_otp_addr != NULL) && (lsc_otp_len != 0)) {

		uint16_t *lsc_16_bits = (uint16_t *) malloc(lsc_ori_chn_len * 4);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 0), lsc_16_bits + lsc_otp_chn_gain_num * 0, lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 1), lsc_16_bits + lsc_otp_chn_gain_num * 1, lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 2), lsc_16_bits + lsc_otp_chn_gain_num * 2, lsc_otp_chn_gain_num);
		_lsc_gain_14bits_to_16bits((unsigned short *)(lsc_otp_addr + lsc_otp_len_chn * 3), lsc_16_bits + lsc_otp_chn_gain_num * 3, lsc_otp_chn_gain_num);

		lsc_table = _lsc_table_wrapper(lsc_16_bits, lsc_otp_grid, full_img_width, full_img_height, &gain_w, &gain_h);	//  wrapper otp table

		free(lsc_16_bits);
		if (lsc_table == NULL) {
			ISP_LOGE("init_lsc_otp, sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_len, lsc_otp_chn_gain_num,
					 lsc_otp_width, lsc_otp_height, lsc_otp_grid);
			goto EXIT;
		}

		// case for isp2.0 of 8M and 13M
		if((3264-100 <= full_img_width && full_img_width <= 3264+100 && 2448-100 <= full_img_height && full_img_height <= 2448+100 && lsc_param->grid == 128)
		 ||(4224-100 <= full_img_width && full_img_width <= 4224+100 && 3136-100 <= full_img_height && full_img_height <= 3136+100 && lsc_param->grid == 128)){
			_lsc_get_otp_size_info(full_img_width, full_img_height, &lsc_otp_width, &lsc_otp_height, 128);
			_table_linear_scaler(lsc_table, gain_w, gain_h, lsc_table, lsc_otp_width+2, lsc_otp_height+2, 1);
			gain_w = lsc_otp_width+2;
			gain_h = lsc_otp_height+2;
			lsc_otp_grid = 128;
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

static cmr_s32 _lscctrl_deinit_adpt(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
		lib_ptr->lib_handle = NULL;
	} else {
		ISP_LOGI("adpt_deinit fun is NULL");
	}

  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_destroy_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param , param is NULL");
		rtn = LSC_ERROR;
		goto exit;
	}

	if (cxt_ptr->thr_handle) {
		rtn = cmr_thread_destroy(cxt_ptr->thr_handle);
		if (!rtn) {
			cxt_ptr->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_process(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI("process fun is NULL");
	}
  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_int _lscctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *handle = (struct lsc_ctrl_cxt *)p_data;

	if (NULL == handle) {
		ISP_LOGE("Error: handle is NULL");
		return rtn;
	}

	if (!message || !p_data) {
		ISP_LOGE("fail to chcek param");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case LSCCTRL_EVT_INIT:
		break;
	case LSCCTRL_EVT_DEINIT:
		rtn = _lscctrl_deinit_adpt(handle);
		break;
	case LSCCTRL_EVT_IOCTRL:
		break;
	case LSCCTRL_EVT_PROCESS:	// ISP_PROC_LSC_CALC
		rtn = _lscctrl_process(handle, (struct lsc_adv_calc_param *)message->data, &handle->proc_out);
		break;
	default:
		ISP_LOGE("fail to proc,don't support msg");
		break;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_init_lib(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param ,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, NULL);
	} else {
		ISP_LOGI("adpt_init fun is NULL");
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_init_adpt(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn = adpt_get_ops(ADPT_LIB_LSC, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = _lscctrl_init_lib(cxt_ptr, in_ptr);
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

static cmr_s32 _lscctrl_create_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, _lscctrl_ctrl_thr_proc, (void *)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = LSC_ERROR;
		goto exit;
	}
	rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "lscctrl");
	if (CMR_MSG_SUCCESS != rtn) {
		ISP_LOGE("fail to set lscctrl name");
		rtn = CMR_MSG_SUCCESS;
	}
  exit:
	ISP_LOGI("lsc_ctrl thread rtn %ld", rtn);
	return rtn;
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

static void lsc_otp_calc(void* lsc_otp_handle, cmr_u32 width, cmr_u32 height)
{
	struct lsc_otp_context* handle = (struct lsc_otp_context *) lsc_otp_handle;
	cmr_u16 *gain_golden_r  = handle->golden_otp_r;
	cmr_u16 *gain_golden_gr = handle->golden_otp_gr;
	cmr_u16 *gain_golden_gb = handle->golden_otp_gb;
	cmr_u16 *gain_golden_b  = handle->golden_otp_b;
	cmr_u16 *gain_random_r  = handle->random_otp_r;
	cmr_u16 *gain_random_gr = handle->random_otp_gr;
	cmr_u16 *gain_random_gb = handle->random_otp_gb;
	cmr_u16 *gain_random_b  = handle->random_otp_b;
	cmr_u16 *gain_input_r   = handle->prm_gain_r;
	cmr_u16 *gain_input_gr  = handle->prm_gain_gr;
	cmr_u16 *gain_input_gb  = handle->prm_gain_gb;
	cmr_u16 *gain_input_b   = handle->prm_gain_b;
	cmr_u16 *gain_output_r  = handle->prm_gain_r;
	cmr_u16 *gain_output_gr = handle->prm_gain_gr;
	cmr_u16 *gain_output_gb = handle->prm_gain_gb;
	cmr_u16 *gain_output_b  = handle->prm_gain_b;
	float* gain_ratio_g           = handle->gain_ratio_g;
	float* gain_ratio_random_rgbg = handle->gain_ratio_random_rgbg;
	float* gain_ratio_golden_rgbg = handle->gain_ratio_golden_rgbg;
	float* ratio_rgbg             = handle->ratio_rgbg;
	float* gain_ratio_input_rgbg  = handle->gain_ratio_input_rgbg;
	float* gain_ratio_output_rgbg = handle->gain_ratio_output_rgbg;
	cmr_u32 i;

	/* step 1: gain_output_gr & gain_output_gb */
	for(i=0; i<height*width; i++)
		gain_ratio_g[i] = (float)(gain_random_gr[i] + gain_random_gb[i]) / (float)(gain_golden_gr[i] + gain_golden_gb[i]);
	for(i=0; i<height*width; i++) {
		gain_output_gr[i] = (uint16_t)(gain_input_gr[i] * gain_ratio_g[i] + 0.5f);
		gain_output_gb[i] = (uint16_t)(gain_input_gb[i] * gain_ratio_g[i] + 0.5f);
	}

	/* step 2: gain_output_r */
	for(i=0; i<height*width; i++)
		gain_ratio_random_rgbg[i] = (float)(gain_random_r[i] + gain_random_r[i]) / (float)(gain_random_gr[i] + gain_random_gb[i]);
	for(i=0; i<height*width; i++)
		gain_ratio_golden_rgbg[i] = (float)(gain_golden_r[i] + gain_golden_r[i]) / (float)(gain_golden_gr[i] + gain_golden_gb[i]);
	for(i=0; i<height*width; i++)
		ratio_rgbg[i] = gain_ratio_random_rgbg[i] / gain_ratio_golden_rgbg[i];
	for(i=0; i<height*width; i++)
		gain_ratio_input_rgbg[i] = (float)(gain_input_r[i] + gain_input_r[i]) / (float)(gain_input_gr[i] + gain_input_gb[i]);
	for(i=0; i<height*width; i++)
		gain_ratio_output_rgbg[i] = gain_ratio_input_rgbg[i] * ratio_rgbg[i];
	for(i=0; i<height*width; i++)
		gain_output_r[i] = (uint16_t)((gain_input_gr[i] + gain_input_gb[i]) * gain_ratio_g[i] / 2.0f * gain_ratio_output_rgbg[i] + 0.5f);

    /* step 3: gain_output_b */
	for(i=0; i<height*width; i++)
		gain_ratio_random_rgbg[i] = (float)(gain_random_b[i] + gain_random_b[i]) / (float)(gain_random_gr[i] + gain_random_gb[i]);
	for(i=0; i<height*width; i++)
		gain_ratio_golden_rgbg[i] = (float)(gain_golden_b[i] + gain_golden_b[i]) / (float)(gain_golden_gr[i] + gain_golden_gb[i]);
	for(i=0; i<height*width; i++)
		ratio_rgbg[i] = gain_ratio_random_rgbg[i] / gain_ratio_golden_rgbg[i];
	for(i=0; i<height*width; i++)
		gain_ratio_input_rgbg[i] = (float)(gain_input_b[i] + gain_input_b[i]) / (float)(gain_input_gr[i] + gain_input_gb[i]);
	for(i=0; i<height*width; i++)
		gain_ratio_output_rgbg[i] = gain_ratio_input_rgbg[i] * ratio_rgbg[i];
	for(i=0; i<height*width; i++)
		gain_output_b[i] = (uint16_t)((gain_input_gr[i] + gain_input_gb[i]) * gain_ratio_g[i] / 2.0f * gain_ratio_output_rgbg[i] + 0.5f);
}

static void otp_tab_converter(unsigned short* *otp_tab, unsigned short* *pm_tab, unsigned short* random_otp, unsigned int gain_width, unsigned int gain_height, unsigned int gain_pattern)
{
	struct lsc_otp_context* handle = (struct lsc_otp_context*)malloc(sizeof(struct lsc_otp_context));
	cmr_u16 golden_otp_r [32*32] = {0};
	cmr_u16 golden_otp_gr[32*32] = {0};
	cmr_u16 golden_otp_gb[32*32] = {0};
	cmr_u16 golden_otp_b [32*32] = {0};
	cmr_u16 *random_otp_r  = random_otp;
	cmr_u16 *random_otp_gr = random_otp + gain_width * gain_height;
	cmr_u16 *random_otp_gb = random_otp + gain_width * gain_height*2;
	cmr_u16 *random_otp_b  = random_otp + gain_width * gain_height*3;
	cmr_u16 prm_gain_r [32*32] = {0};
	cmr_u16 prm_gain_gr[32*32] = {0};
	cmr_u16 prm_gain_gb[32*32] = {0};
	cmr_u16 prm_gain_b [32*32] = {0};
	float gain_ratio_g[32*32] = {0.0};
	float gain_ratio_random_rgbg[32*32] = {0.0};
	float gain_ratio_golden_rgbg[32*32] = {0.0};
	float ratio_rgbg[32*32] = {0.0};;
	float gain_ratio_input_rgbg[32*32] = {0.0};
	float gain_ratio_output_rgbg[32*32] = {0.0};

	handle->golden_otp_r  = golden_otp_r;
	handle->golden_otp_gr = golden_otp_gr;
	handle->golden_otp_gb = golden_otp_gb;
	handle->golden_otp_b  = golden_otp_b;
	handle->random_otp_r  = random_otp_r;
	handle->random_otp_gr = random_otp_gr;
	handle->random_otp_gb = random_otp_gb;
	handle->random_otp_b  = random_otp_b;
	handle->prm_gain_r  = prm_gain_r;
	handle->prm_gain_gr = prm_gain_gr;
	handle->prm_gain_gb = prm_gain_gb;
	handle->prm_gain_b  = prm_gain_b;
	handle->gain_ratio_g           = gain_ratio_g;
	handle->gain_ratio_random_rgbg = gain_ratio_random_rgbg;
	handle->gain_ratio_golden_rgbg = gain_ratio_golden_rgbg;
	handle->ratio_rgbg             = ratio_rgbg;
	handle->gain_ratio_input_rgbg  = gain_ratio_input_rgbg;
	handle->gain_ratio_output_rgbg = gain_ratio_output_rgbg;

	save_tab_to_channel(gain_width, gain_height, gain_pattern, golden_otp_r, golden_otp_gr, golden_otp_gb, golden_otp_b, pm_tab[8]);
	ISP_LOGV("[ALSC] OTP, pm_tab[0]=DNP[%d,%d,%d,%d], pm_tab[8]=OTP[%d,%d,%d,%d]", pm_tab[0][0], pm_tab[0][1], pm_tab[0][2], pm_tab[0][3], pm_tab[8][0], pm_tab[8][1], pm_tab[8][2], pm_tab[8][3]);
	ISP_LOGV("[ALSC] OTP, random_otp_RGGB0[%d,%d,%d,%d], random_otp_RGGB1[%d,%d,%d,%d]", random_otp_r[0], random_otp_gr[0], random_otp_gb[0], random_otp_b[0],
			random_otp_r[gain_width+1], random_otp_gr[gain_width+1], random_otp_gb[gain_width+1], random_otp_b[gain_width+1]);

	for (int i=0; i<8; i ++){
		save_tab_to_channel(gain_width, gain_height, gain_pattern, prm_gain_r, prm_gain_gr, prm_gain_gb, prm_gain_b, pm_tab[i]);

		lsc_otp_calc(handle, gain_width, gain_height);

		set_channel_to_tab(gain_width, gain_height, gain_pattern, prm_gain_r, prm_gain_gr, prm_gain_gb, prm_gain_b, otp_tab[i]);
		ISP_LOGV("[ALSC] OTP, OTP_tab0[%d][%d,%d,%d,%d,], OTP_tab1[%d][%d,%d,%d,%d,]", i, otp_tab[i][0], otp_tab[i][1], otp_tab[i][2], otp_tab[i][3],
				i, otp_tab[i][(gain_width+1)*4 + 0], otp_tab[i][(gain_width+1)*4 + 1], otp_tab[i][(gain_width+1)*4 + 2], otp_tab[i][(gain_width+1)*4 + 3]);
	}
	free (handle);
	handle = NULL;
}

static void sync_g_channel(cmr_u16* *tab_address, cmr_u32 gain_width, cmr_u32 gain_height, cmr_u32 gain_pattern)
{
	cmr_u32 i, k;
	cmr_u16 dnp_r [32*32*4] = {0};
	cmr_u16 dnp_gr[32*32*4] = {0};
	cmr_u16 dnp_gb[32*32*4] = {0};
	cmr_u16 dnp_b [32*32*4] = {0};
	cmr_u16 tmp_r [32*32*4] = {0};
	cmr_u16 tmp_gr[32*32*4] = {0};
	cmr_u16 tmp_gb[32*32*4] = {0};
	cmr_u16 tmp_b [32*32*4] = {0};
	cmr_u16 new_r [32*32*4] = {0};
	cmr_u16 new_b [32*32*4] = {0};

	save_tab_to_channel(gain_width, gain_height, gain_pattern, dnp_r, dnp_gr, dnp_gb, dnp_b, tab_address[0]);

	for(k=1; k<8; k++){
		save_tab_to_channel(gain_width, gain_height, gain_pattern, tmp_r, tmp_gr, tmp_gb, tmp_b, tab_address[k]);
		ISP_LOGV("[ALSC] sync g, before_rggb[%d]=[%d,%d,%d,%d]", k, tmp_r[0], tmp_gr[0], tmp_gb[0], tmp_b[0]);
		for(i=0; i<gain_width*gain_height; i++){
			new_r[i] = (cmr_u16)(tmp_r[i] * ((float)(dnp_gr[i]+dnp_gb[i]))/(tmp_gr[i]+tmp_gb[i]));
			new_b[i] = (cmr_u16)(tmp_b[i] * ((float)(dnp_gr[i]+dnp_gb[i]))/(tmp_gr[i]+tmp_gb[i]));
		}
		set_channel_to_tab (gain_width, gain_height, gain_pattern, new_r, dnp_gr, dnp_gb, new_b, tab_address[k]);
		ISP_LOGV("[ALSC] sync g, after_rggb[%d]=[%d,%d,%d,%d]", k, new_r[0], dnp_gr[0], dnp_gb[0], new_b[0]);
	}
}

static cmr_s32 _lscsprd_lsc_param_preprocess(struct lsc_adv_init_param *init_param, struct lsc_ctrl_context *cxt)
{
	cmr_u32 i;
	cmr_s32 rtn = LSC_SUCCESS;
	cmr_u16 *lsc_otp_table_addr = NULL;

	if(cxt->cmd_alsc_cmd_enable){
		alsc_get_cmd(cxt);
		if(cxt->cmd_alsc_bypass_otp){
			init_param->lsc_otp_table_en = 0;
			ISP_LOGI("[ALSC] cmd_alsc_bypass_otp, lsc_id=%d", cxt->lsc_id);
		}
	}

	if(init_param->lsc_otp_table_addr && init_param->lsc_otp_table_en &&
		cxt->init_gain_width  == init_param->lsc_otp_table_width &&
		cxt->init_gain_height == init_param->lsc_otp_table_height){

		lsc_otp_table_addr = init_param->lsc_otp_table_addr;
		ISP_LOGV("[ALSC] lsc_otp_table_rggb[%d,%d,%d,%d], camera_id=%d, lsc_id=%d", lsc_otp_table_addr[0], lsc_otp_table_addr[cxt->init_gain_width*cxt->init_gain_height], lsc_otp_table_addr[cxt->init_gain_width*cxt->init_gain_height*2], lsc_otp_table_addr[cxt->init_gain_width*cxt->init_gain_height*3], cxt->camera_id, cxt->lsc_id);

		otp_tab_converter(cxt->std_init_lsc_table_param_buffer, init_param->lsc_tab_address, lsc_otp_table_addr, cxt->init_gain_width, cxt->init_gain_height, cxt->gain_pattern);
	}else{
		for(i=0; i<8; i++)
			memcpy(cxt->std_init_lsc_table_param_buffer[i], init_param->lsc_tab_address[i], cxt->init_gain_width*cxt->init_gain_height*4*sizeof(cmr_u16));
		ISP_LOGV("[ALSC] there is no LSC OTP table, lsc_id=%d", cxt->lsc_id);
	}

	for(i=0; i<8; i++){
		change_lsc_pattern(cxt->std_init_lsc_table_param_buffer[i], cxt->init_gain_width, cxt->init_gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
		init_param->lsc_tab_address[i] = cxt->std_init_lsc_table_param_buffer[i];
	}

	sync_g_channel(cxt->std_init_lsc_table_param_buffer, cxt->init_gain_width, cxt->init_gain_height, cxt->output_gain_pattern);

	return rtn;
}

static void* post_shading_gain_init()
{
	struct post_shading_gain_param *param = (struct post_shading_gain_param*) malloc(sizeof(struct post_shading_gain_param));
	param->bv2gainw_en = 1;					// tunable param
	param->bv2gainw_p_bv[0] = 0;			// tunable param, src + 4 points + dst, bv data range: 0 ~ 1600
	param->bv2gainw_p_bv[1] = 150;
	param->bv2gainw_p_bv[2] = 200;
	param->bv2gainw_p_bv[3] = 250;
	param->bv2gainw_p_bv[4] = 300;
	param->bv2gainw_p_bv[5] = 1600;
	param->pbits_gainw = 10;				// set gainw data range
	param->pbits_trunc = 1<<(param->pbits_gainw-1);
	param->bv2gainw_b_gainw[0] = 1;		// tunable param, src + 4 points + dst, gainw data range (Q1.10): 0(0%) ~ 1024(100%)
	param->bv2gainw_b_gainw[1] = 1;
	param->bv2gainw_b_gainw[2] = 400;
	param->bv2gainw_b_gainw[3] = 800;
	param->bv2gainw_b_gainw[4] = 1024;
	param->bv2gainw_b_gainw[5] = 1024;
	param->action_bv = 0;
	param->action_bv_gain = 12800;
	return param;
}

static void* lsc_flash_proc_init ()
{
	struct lsc_flash_proc_param *param = (struct lsc_flash_proc_param*) malloc(sizeof(struct lsc_flash_proc_param));
	param->captureFlashEnvRatio=0;
	param->captureFlash1ofALLRatio=0;
	param->main_flash_from_other_parameter=0;
	param->preflash_current_lnc_table_address = NULL;

	//for touch preflash
	param->is_touch_preflash=-99;
	param->ae_touch_framecount=-99;
	param->pre_flash_before_ae_touch_framecount=-99;
	param->pre_flash_before_framecount=-99;
	return param;
}

static void cmd_set_lsc_output(cmr_u16* table, cmr_u32 width, cmr_u32 height, cmr_u32 gain_pattern)
{
	char prop[256];
	property_get("debug.isp.alsc.table.pattern", prop, "0");
	int index_r, index_gr, index_gb, index_b;
	unsigned int i, j;

	switch (gain_pattern){
		case 0:    //  Gr first
			index_gr = 0;
			index_r  = 1;
			index_b  = 2;
			index_gb = 3;
		break;
		case 1:    //  R first
			index_r  = 0;
			index_gr = 1;
			index_gb = 2;
			index_b  = 3;
		break;
		case 2:    //  B first
			index_b  = 0;
			index_gb = 1;
			index_gr = 2;
			index_r  = 3;
		break;
		case 3:    //  Gb first
			index_gb = 0; //B    //R 3
			index_b  = 1; //Gb   //Gr2
			index_r  = 2; //Gr   //Gb1
			index_gr = 3; //R    //B 0
		break;
		default:    //  Gb first
			index_gb = 0;
			index_b  = 1;
			index_r  = 2;
			index_gr = 3;
		break;
	}

	for(i=0; i<width*height*4; i++){
		table[i] = 1024;
	}

	if(strcmp(prop, "1000") == 0){           // R gain(8x)
		for(i=0; i<width*height; i++){
			table[4*i + index_r] = 8192;
		}
	}else if(strcmp(prop, "0100") == 0){    // Gr gain(8x)
		for(i=0; i<width*height; i++){
			table[4*i + index_gr] = 8192;
			table[4*i + index_r] = 8192;
		}
	}else if(strcmp(prop, "0010") == 0){    // Gb gain(8x)
		for(i=0; i<width*height; i++){
			table[4*i + index_gb] = 8192;
			table[4*i + index_b] = 8192;
		}
	}else if(strcmp(prop, "0001") == 0){    // B gain(8x)
		for(i=0; i<width*height; i++){
			table[4*i + index_b] = 8192;
		}
	}else if(strcmp(prop, "1100") == 0){    // up(8x)bottom(1x)        
		for(j=0; j<height/2; j++){
			for (i=0; i<width; i++){
				table[4*(j*width + i)+0] = 8192;
				table[4*(j*width + i)+1] = 8192;
				table[4*(j*width + i)+2] = 8192;
				table[4*(j*width + i)+3] = 8192;
			}
		}
	}else if(strcmp(prop, "0011") == 0){    // up(1x)bottom(8x)
		for(j=height/2; j<height; j++){
			for (i=0; i<width; i++){
				table[4*(j*width + i)+0] = 8192;
				table[4*(j*width + i)+1] = 8192;
				table[4*(j*width + i)+2] = 8192;
				table[4*(j*width + i)+3] = 8192;
			}
		}
	}
}

static int dump_stat_bmp(char* filename, unsigned int img_width, unsigned int img_height, unsigned int width, unsigned int height, unsigned int* buf_r, unsigned int* buf_g, unsigned int* buf_b)
{
	unsigned int i, j;
	unsigned int line_align = (width*3 + 3) & (~3);
	unsigned char* pbgr = (unsigned char*)malloc(line_align * height);
	memset(pbgr, 0, line_align * height);

	unsigned int channel_width = img_width / 2;
	unsigned int channel_height = img_height / 2;
	unsigned int pix_num_per_block = (unsigned int)(channel_width/width) * (unsigned int)(channel_height/height);
	unsigned int divisor = pix_num_per_block * 4; // 10 bit to 8 bit

	unsigned char* pbuf = pbgr;
	for (j=0; j<height; j++){
		for (i=0; i<width; i++){
			unsigned int r32 = *buf_r++ / divisor;
			unsigned int g32 = *buf_g++ / divisor;
			unsigned int b32 = *buf_b++ / divisor;
			if (r32 > 255)   r32 = 255;
			if (g32 > 255)   g32 = 255;
			if (b32 > 255)   b32 = 255;

			unsigned char r = r32;
			unsigned char g = g32;
			unsigned char b = b32;

			*pbuf++ = b;
			*pbuf++ = g;
			*pbuf++ = r;
		}
		pbuf += line_align - width*3;
	}

	FILE* fp = fopen(filename, "wb");
	if (fp == NULL){
		free(pbgr);
		return -1;
	}

#ifndef WIN32
	struct BITMAPFILEHEADER{
		unsigned short bfType;
		unsigned int bfSize;
		unsigned short bfReserved1;
		unsigned short bfReserved2;
		unsigned int bfOffBits;
	} __attribute__((packed));

	struct BITMAPINFOHEADER{
		unsigned int biSize;
		unsigned int biWidth;
		unsigned int biHeight;
		unsigned short biPlanes;
		unsigned short biBitCount;
		unsigned int biCompression;
		unsigned int biSizeImage;
		unsigned int biXPelsPerMeter;
		unsigned int biYPelsPerMeter;
		unsigned int biClrUsed;
		unsigned int biClrImportant;
	};
	struct BITMAPFILEHEADER bfheader;
	struct BITMAPINFOHEADER biheader;
#else
	BITMAPFILEHEADER bfheader;
	BITMAPINFOHEADER biheader;
#endif

	memset(&bfheader, 0, sizeof(bfheader));
	bfheader.bfType = 'B' | ('M' << 8);
	bfheader.bfSize = sizeof(bfheader) + sizeof(biheader) + line_align * height;
	bfheader.bfOffBits = sizeof(bfheader) + sizeof(biheader);
	if (fwrite(&bfheader, 1, sizeof(bfheader), fp) != sizeof(bfheader)){
		fclose(fp);
		free(pbgr);
		return -1;
	}

	memset(&biheader, 0, sizeof(biheader));
	biheader.biSize = sizeof(biheader);
	biheader.biPlanes = 1;
	biheader.biWidth = width;
	biheader.biHeight = 0 - height;
	biheader.biBitCount = 24;
	biheader.biSizeImage = line_align * height;
	if (fwrite(&biheader, 1, sizeof(biheader), fp) != sizeof(biheader)){
		fclose(fp);
		free(pbgr);
		return -1;
	}

	if (fwrite(pbgr, 1, line_align * height, fp) != line_align * height){
		fclose(fp);
		free(pbgr);
		return -1;
	}

	fclose(fp);
	free(pbgr);

	return 0;
}

static void cmd_dump_aem(cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32* stat_r, cmr_u32* stat_g, cmr_u32* stat_b, cmr_u32 lsc_id, cmr_u32 type)
{
	int dump_state;
	static int dump_index[2] = {1, 1};
	char filename[256];
	char version[1024];
	property_get("ro.build.version.release", version, (char*)"");

	if (version[0] >= '9'){
		sprintf(filename, "/data/vendor/cameraserver/lsc/%05d_AEM%d_cam%d_org.bmp", dump_index[lsc_id-1], type, lsc_id);
	}else if (version[0] >= '7'){
		sprintf(filename, "/data/misc/cameraserver/lsc/%05d_AEM%d_cam%d_org.bmp", dump_index[lsc_id-1], type, lsc_id);
	}else{
		sprintf(filename, "/data/misc/media/lsc/%05d_AEM%d_cam%d_org.bmp", dump_index[lsc_id-1], type, lsc_id);
	}

	dump_state = dump_stat_bmp(filename, raw_width, raw_height, 32, 32, stat_r, stat_g, stat_b);
	dump_index[lsc_id-1] ++;
	ISP_LOGI("dump_state=%d, lsc_id=%d", dump_state, lsc_id);
}

void dump_lsc(unsigned short* gain, int width, int height, const char* filename )
{
	FILE* fp  = fopen(filename, "w");
	if(fp != NULL)
	{
		int i, j;
		for(j=0; j<height; j++)
		{
			for(i=0; i<width; i++)
			{
				fprintf(fp, "%4d,",gain[j*width+i]);
			}
			fprintf(fp,"\r\n");
		}
		fclose(fp);
	}
}

void dump_lsc_gain(unsigned short* lsc_table_in, unsigned int lsc_with,unsigned int lsc_height)
{
	char prop[256];
	property_get("debug.isp.alsc.dump.table",prop,"0");
	if(strcmp(prop,"0") != 0){
		unsigned short tmp[1118*4];
		unsigned i;
		static int index = 1;
		char version[1024];
		property_get("ro.build.version.release",version,(char*)"");
		ISP_LOGI("debug.isp.alsc.dump.table: %s ro.build.version.release: %s",prop,version);
		for(i = 0; i<lsc_with * lsc_height; i++)
			{
				tmp[lsc_with * lsc_height * 0 + i] = lsc_table_in[4 * i + 1];
				tmp[lsc_with * lsc_height * 1 + i] = lsc_table_in[4 * i + 0];
				tmp[lsc_with * lsc_height * 2 + i] = lsc_table_in[4 * i + 3];
				tmp[lsc_with * lsc_height * 3 + i] = lsc_table_in[4 * i + 2];
			}
		if(version[0] > '6'){
			if(mkdir("/data/vendor/cameraserver/lsc/", 0755) != 0){
				char filename[256];
				sprintf(filename,"/data/vendor/cameraserver/lsc/%05d_r0c0.txt",index);
				dump_lsc(tmp + lsc_with * lsc_height * 0, lsc_with, lsc_height, filename);

				sprintf(filename,"/data/vendor/cameraserver/lsc/%05d_r0c1.txt",index);
				dump_lsc(tmp + lsc_with * lsc_height * 1, lsc_with, lsc_height, filename);

				sprintf(filename,"/data/vendor/cameraserver/lsc/%05d_r1c0.txt",index);
				dump_lsc(tmp + lsc_with * lsc_height * 2, lsc_with, lsc_height, filename);

				sprintf(filename,"/data/vendor/cameraserver/lsc/%05d_r1c1.txt",index);
				dump_lsc(tmp + lsc_with * lsc_height * 3, lsc_with, lsc_height, filename);

				index ++;
			}
		}
	}
}

static void gen_flash_y_gain(unsigned short* flash_y_gain, unsigned int gain_width, unsigned int gain_height, unsigned int percent, unsigned int shift_x, unsigned int shift_y)
{
	unsigned int i, j;
	float ratio_4, ratio_2, x_2, y_2, cos_2, cos_4, inv_cos_4;
	unsigned int table_width  = 200;
	unsigned int table_height = 150;
	unsigned short inv_cos4_table[200*150] = {0};
	unsigned int center_x = table_width /2;
	unsigned int center_y = table_height/2;
	unsigned int length_2 = center_x*center_x + center_y*center_y;

	if(percent == 0 || shift_x == 0 || shift_y ==0){
		for(i=0; i<gain_width*gain_height; i++){
			flash_y_gain[i] = 0;
		}
	}else{
		percent = percent <    0 ?    0 : percent;
		percent = percent > 1000 ? 1000 : percent;
		shift_x = shift_x <  50 ?  50 : shift_x;
		shift_x = shift_x > 150 ? 150 : shift_x;
		shift_y = shift_y <  50 ?  50 : shift_y;
		shift_y = shift_y > 150 ? 150 : shift_y;
		center_x = center_x + shift_x - 100;
		center_y = center_y + shift_y - 100;

		ratio_4 = 100.0 / (float)(percent + 100);
		ratio_2 = (float)(sqrt((double)(ratio_4)));
		x_2 = ratio_2/(1.0 - ratio_2);

		ISP_LOGI("[ALSC] flash_y_gain, percent=%d, shift_x=%d, shift_y=%d, center_x=%d, center_y=%d, length_2=%d, ratio_4=%f, ratio_2=%f, x_2=%f", percent, shift_x, shift_y, center_x, center_y, length_2, ratio_4, ratio_2, x_2);

		for(j=0; j<table_height; j++){
			for(i=0; i<table_width; i++){
				y_2 = (float)((i-center_x)*(i-center_x) + (j-center_y)*(j-center_y))/length_2;
				cos_2 = x_2 / (x_2 + y_2);
				cos_4 = cos_2 * cos_2;
				inv_cos_4 = 1.0/cos_4;

				inv_cos4_table[j*table_width + i] = (unsigned short)(inv_cos_4 * 1024);
			}
		}

		_scale_bilinear_short(inv_cos4_table, table_width, table_height, flash_y_gain, gain_width, gain_height);
	}
}

static cmr_s32 _lscsprd_set_tuning_param(struct lsc_adv_init_param *init_param, struct lsc_ctrl_context *cxt)
{
	cmr_s32 i, post_act_index;
	cmr_s32 rtn = LSC_SUCCESS;
	init_param->lsc_id = cxt->lsc_id;
	cxt->alg_locked = 0;
	cxt->flash_mode = 0;
	cxt->pre_flash_mode = 0;
	cxt->flash_done_frame_count = 100;
	cxt->frame_count = 0;
	cxt->alg_count = 0;
	cxt->alg_quick_in = 0;
	cxt->alg_quick_in_frame = 3;   // alsc ctrl setting
	cxt->quik_in_start_frame = -99;
	cxt->init_skip_frame = 5;      // alsc ctrl setting
	cxt->bv_skip_frame = 0;
	cxt->cur_lsc_pm_mode = 0;   // 0: common table size,  1: 720p table size
	cxt->pre_lsc_pm_mode = 0;   // 0: common table size,  1: 720p table size
	cxt->img_width = init_param->img_width;
	cxt->img_height = init_param->img_height;
	cxt->gain_width  =  init_param->gain_width;
	cxt->gain_height =  init_param->gain_height;
	cxt->init_img_width   =  init_param->img_width;
	cxt->init_img_height  =  init_param->img_height;
	cxt->init_gain_width  =  init_param->gain_width;
	cxt->init_gain_height =  init_param->gain_height;
	cxt->init_grid        =  init_param->grid;
	cxt->fw_start_bv      =  1600;
	cxt->fw_start_bv_gain =  128;
	cxt->gain_pattern = init_param->gain_pattern;
	cxt->output_gain_pattern = init_param->output_gain_pattern;
	cxt->change_pattern_flag = init_param->change_pattern_flag;
	cxt->is_master =init_param->is_master;
	cxt->is_multi_mode=init_param->is_multi_mode;
	cxt->grid = init_param->grid;
	cxt->camera_id = init_param->camera_id;
	cxt->can_update_dest = 1;
	cxt->alsc_update_flag = 0;
	cxt->fw_start_end = 0;
	cxt->alg_bypass = 0;
	cxt->lsc_pm0 = init_param->lsc_tab_address[0];
	cxt->cmd_alsc_cmd_enable = alsc_get_init_cmd();
	if(cxt->cmd_alsc_cmd_enable)
		ISP_LOGI("[ALSC] cmd mode enable");

    // if no pm data, set default parameter and bypass alsc
    if( init_param->tune_param_ptr == NULL){
		// control_param
		cxt->calc_freq = 3;
		init_param->lsc2_tune_param.LSC_SPD_VERSION = 99;
		init_param->lsc2_tune_param.user_mode = 2;
		init_param->lsc2_tune_param.alg_mode = 0;
		init_param->lsc2_tune_param.flash_enhance_en = 0;
		init_param->lsc2_tune_param.number_table = 5;   // need to delete from tool list
		init_param->lsc2_tune_param.table_base_index = 2;  // 0.DNP  1.A  2.TL84  3. D65  4.CWF 5.H
		init_param->lsc2_tune_param.freq = 3;
		init_param->lsc2_tune_param.IIR_weight = 5;   // [0 ~ 16]

		// smart_lsc2.0 param
		init_param->lsc2_tune_param.num_seg_queue = 10;
		init_param->lsc2_tune_param.num_seg_vote_th = 8;
		init_param->lsc2_tune_param.IIR_smart2 = 2;    // [0 ~ 16]

		// Auto_lsc1.0 param
		init_param->lsc2_tune_param.strength = 5;

		// Auto_lsc2.0 param
		init_param->lsc2_tune_param.lambda_r = 10;
		init_param->lsc2_tune_param.lambda_b = 10;
		init_param->lsc2_tune_param.weight_r = 40;
		init_param->lsc2_tune_param.weight_b = 40;

		// post_gain
		init_param->lsc2_tune_param.bv2gainw_en = 0;

		cxt->alg_bypass = 1;
		ISP_LOGE("load alsc tuning parameters error, by pass alsc_calc");
		return LSC_ERROR;
	}

	// init_param->tune_param_ptr are ALSC parameters from isp tool
	struct lsc2_tune_param* param = (struct lsc2_tune_param*)init_param->tune_param_ptr;
	init_param->lsc2_tune_param.LSC_SPD_VERSION = param->LSC_SPD_VERSION;
	cxt->LSC_SPD_VERSION = param->LSC_SPD_VERSION;

	// system setting
	switch (param->user_mode){
		case 0:    //  Debug mode 0 , OTP mode 0
			init_param->lsc2_tune_param.alg_mode = 0;// id off
			break;
		case 1:    //  Debug mode 0 , OTP mode 1
			init_param->lsc2_tune_param.alg_mode = 1;// 1d on
			break;
		case 2:    //  Debug mode 1 , OTP mode 0
			init_param->lsc2_tune_param.alg_mode = 0;// id off
			break;
		case 3:    //  Debug mode 1 , OTP mode 1
			init_param->lsc2_tune_param.alg_mode = 1;// id on
			break;
		default:
			break;
        }

	// control_param
	cxt->output_gain_pattern = init_param->output_gain_pattern;
	cxt->calc_freq = param->freq;
	cxt->IIR_weight = param->IIR_weight;
	init_param->lsc2_tune_param.user_mode = param->user_mode;
	init_param->lsc2_tune_param.flash_enhance_en = param->flash_enhance_en;
	init_param->lsc2_tune_param.number_table = 5;   // need to delete from tool list
	init_param->lsc2_tune_param.table_base_index = param->table_base_index;  // 0. Alight  1. TL84  2. CWF  3. D65  4. DNP
	init_param->lsc2_tune_param.freq = param->freq;
	init_param->lsc2_tune_param.IIR_weight = param->IIR_weight;   // [0 ~ 16]

	// smart_lsc2.0 param
	init_param->lsc2_tune_param.num_seg_queue = param->num_seg_queue;
	init_param->lsc2_tune_param.num_seg_vote_th = param->num_seg_vote_th;
	init_param->lsc2_tune_param.IIR_smart2 = param->IIR_smart2;    // [0 ~ 16]

	// Auto_lsc1.0 param
	init_param->lsc2_tune_param.strength = param->strength;

	// Auto_lsc2.0 param
	init_param->lsc2_tune_param.lambda_r = param->lambda_r;
	init_param->lsc2_tune_param.lambda_b = param->lambda_b;
	init_param->lsc2_tune_param.weight_r = param->weight_r;
	init_param->lsc2_tune_param.weight_b = param->weight_b;

	// post_gain
	struct post_shading_gain_param *post_param = (struct post_shading_gain_param*)cxt->post_shading_gain_param;
	post_param->bv2gainw_en = param->bv2gainw_en;
	for(i=0; i<6; i++){
		post_param->bv2gainw_p_bv[i] = param->bv2gainw_p_bv[i];
		post_param->bv2gainw_b_gainw[i] = param->bv2gainw_b_gainw[i];
	}

	for(int i=5; i>=0; i--){
		if(post_param->bv2gainw_b_gainw[i] == 1024){
			post_act_index = i;
		}
	}
	post_param->action_bv = post_param->bv2gainw_p_bv [post_act_index];
	post_param->action_bv_gain = post_param->bv2gainw_p_bv [post_act_index];

	ISP_LOGV("[ALSC] init_LSC_SPD_VERSION = %d, lsc_id=%d", param->LSC_SPD_VERSION, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_user_mode = %d, lsc_id=%d", param->user_mode, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_alg_mode = %d, lsc_id=%d", param->alg_mode, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_table_base_index = %d, lsc_id=%d", param->table_base_index, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_freq = %d, lsc_id=%d", param->freq, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_IIR_weight = %d, lsc_id=%d", param->IIR_weight, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_num_seg_queue = %d, lsc_id=%d", param->num_seg_queue, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_num_seg_vote_th = %d, lsc_id=%d", param->num_seg_vote_th, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_IIR_smart2 = %d, lsc_id=%d", param->IIR_smart2, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_strength = %d, lsc_id=%d", param->strength, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_lambda_r = %d, lsc_id=%d", param->lambda_r, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_lambda_b = %d, lsc_id=%d", param->lambda_b, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_weight_r = %d, lsc_id=%d", param->weight_r, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_weight_b = %d, lsc_id=%d", param->weight_b, cxt->lsc_id);
	ISP_LOGV("[ALSC] init_bv2gainw_en = %d, lsc_id=%d", param->bv2gainw_en, cxt->lsc_id);
	ISP_LOGV("[ALSC] flash_y_ratio = %d, lsc_id=%d", param->flash_enhance_en, cxt->lsc_id);
	ISP_LOGV("[ALSC] flash_shift_x = %d, lsc_id=%d", param->flash_enhance_max_strength, cxt->lsc_id);
	ISP_LOGV("[ALSC] flash_shift_y = %d, lsc_id=%d", param->flash_enahnce_gain, cxt->lsc_id);
	for(i=0; i<6; i++){
		ISP_LOGV("[ALSC] init_bv2gainw_p_bv[%d]=%d, lsc_id=%d", i, param->bv2gainw_p_bv[i], cxt->lsc_id);
		ISP_LOGV("[ALSC] init_bv2gainw_b_gainw[%d]=%d, lsc_id=%d", i, param->bv2gainw_b_gainw[i], cxt->lsc_id);
	}
	ISP_LOGV("[ALSC] init_action_bv=%d, action_bv_gain=%d, lsc_id=%d", post_param->action_bv, post_param->action_bv_gain, cxt->lsc_id);

	// lsc_debug_info_ptr
	init_param->lsc_debug_info_ptr = cxt->lsc_debug_info_ptr;

	// generate flash_y_gain
	gen_flash_y_gain(cxt->flash_y_gain, cxt->gain_width, cxt->gain_height, param->flash_enhance_en, param->flash_enhance_max_strength, param->flash_enahnce_gain);

	return rtn;
}

static void *lsc_sprd_init(void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_init_param *init_param = (struct lsc_adv_init_param *)in;
	void *alsc_handle = NULL;
	cmr_s32 free_otp_flag = 1;
	cmr_s32 i = 0;
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
	}

	memset(cxt, 0, sizeof(*cxt));

	// create lsc_ctrl buffers +++
	struct debug_lsc_param* lsc_debug_info_ptr = (struct debug_lsc_param*)malloc(sizeof(struct debug_lsc_param));
	if (NULL == lsc_debug_info_ptr) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc lsc_debug_info_ptr!");
		goto EXIT;
	}
	memset(lsc_debug_info_ptr, 0, sizeof(struct debug_lsc_param));
	cxt->lsc_debug_info_ptr = lsc_debug_info_ptr;

	for(i=0; i<8; i++) {
		cxt->std_init_lsc_table_param_buffer[i] = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
		if (NULL == cxt->std_init_lsc_table_param_buffer[i]) {
			rtn = LSC_ALLOC_ERROR;
			ISP_LOGE("fail to alloc std_init_lsc_table_param_buffer!");
			goto EXIT;
		}
	}

	for(i=0; i<8; i++) {
		cxt->std_lsc_table_param_buffer[i] = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
		if (NULL == cxt->std_lsc_table_param_buffer[i]) {
			rtn = LSC_ALLOC_ERROR;
			ISP_LOGE("fail to alloc std_lsc_table_param_buffer!");
			goto EXIT;
		}
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

	cxt->fwstart_new_scaled_table = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
	if (NULL == cxt->fwstart_new_scaled_table) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc fwstart_new_scaled_table!");
		goto EXIT;
	}

	cxt->fwstop_output_table = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
	if (NULL == cxt->fwstop_output_table) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc fwstop_output_table!");
		goto EXIT;
	}

	cxt->flash_y_gain = (cmr_u16 *) malloc(32 * 32 * sizeof(cmr_u16));
	if (NULL == cxt->flash_y_gain) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc flash_y_gain!");
		goto EXIT;
	}

	cxt->lsc_last_info = (struct lsc_last_info *)malloc(sizeof(struct lsc_last_info));
	if (NULL == cxt->lsc_last_info) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc lsc_last_info!");
		goto EXIT;
	}

	cxt->ae_stat = (cmr_u32 *)malloc(1024*3*sizeof(cmr_u32));
	if (NULL == cxt->ae_stat) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc ae_stat!");
		goto EXIT;
	}

	cxt->post_shading_gain_param = post_shading_gain_init();
	cxt->lsc_flash_proc_param = lsc_flash_proc_init();
	// create lsc_ctrl buffers ---

	// get lsc_id
	if(id1_addr == NULL){
		id1_addr = cxt->lsc_buffer;
		cxt->lsc_id = 1;
	}else{
		id2_addr = cxt->lsc_buffer;
		cxt->lsc_id = 2;
	}
	init_param->lsc_buffer_addr = cxt->lsc_buffer;

	cxt->gain_pattern = init_param->gain_pattern;

	cxt->lib_info = &init_param->lib_param;

	rtn = _lscsprd_load_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to load lsc lib");
		goto EXIT;
	}

	// alsc_init
    rtn = _lscsprd_set_tuning_param(init_param, cxt);
	rtn = _lscsprd_lsc_param_preprocess(init_param, cxt);
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
		free(cxt);
		cxt = NULL;
	}

	ISP_LOGI("done rtn = %d", rtn);

	return NULL;
}

static void std_free(void* buffer)
{
	if(buffer) {
		free(buffer);
		buffer = NULL;
	}
}

static cmr_s32 lsc_sprd_deinit(void *handle, void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	cmr_s32 i = 0;
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

	std_free(cxt->lsc_debug_info_ptr);
	for(i=0; i<8; i++){
		std_free(cxt->std_init_lsc_table_param_buffer[i]);
		std_free(cxt->std_lsc_table_param_buffer[i]);
	}
	std_free(cxt->dst_gain);
	std_free(cxt->lsc_buffer);
	std_free(cxt->fwstart_new_scaled_table);
	std_free(cxt->fwstop_output_table);
	std_free(cxt->lsc_last_info);
	std_free(cxt->ae_stat);
	std_free(cxt->post_shading_gain_param);
	std_free(cxt->lsc_flash_proc_param);
	std_free(cxt->flash_y_gain);
	if(cxt->lsc_id==1){
		id1_addr = NULL;
	}else{
		id2_addr = NULL;
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
	cmr_u64 r = 0, g = 0, b = 0;
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

static void scale_table_to_stat_size(cmr_u16 *new_tab, cmr_u16 *org_tab, cmr_u32 gain_width, cmr_u32 gain_height, cmr_u32 stat_width, cmr_u32 stat_height, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 grid )
{
	cmr_u32 i,j,ii,jj;
	cmr_u32 stat_dw = (raw_width  / stat_width)  / 2 * 2;
	cmr_u32 stat_dh = (raw_height / stat_height) / 2 * 2;
	cmr_u32 dx = (raw_width  - stat_width  * stat_dw) / 2;
	cmr_u32 dy = (raw_height - stat_height * stat_dh) / 2;

	float raw_width2  = (float)( raw_width  - 2*dx );
	float raw_height2 = (float)( raw_height - 2*dy );
	float step_width  = raw_width2  / stat_width;
	float step_height = raw_height2 / stat_height;
	float org_x = ( step_width  / 2 ) + dx;
	float org_y = ( step_height / 2 ) + dy;

	//////////////////////// match location  ////////////////////////

	float x,y,lt,rt;
	cmr_u32 X_L[100]={ 0 };
	float D_L[100]={ 0.0f };
	cmr_u32 Y_U[100]={ 0 };
	float D_U[100]={ 0.0f };

	// x-axis
	for(i=0; i<stat_width; i++)	{
		x=(float)(step_width*i) + org_x;
		for(j=1; j<gain_width-1; j++) {
			lt = (float)( 2*grid * (j-1) );
			rt = (float)( 2*grid *  j    );

			if( lt<=x && x<=rt ) {
				X_L[i]=j;
				D_L[i]=x-lt;
			}
		}
	}

	// y-axis
	for(i=0; i<stat_height; i++) {
		y=(float)(step_height*i) + org_y;
		for(j=1; j<gain_height-1; j++) {
			lt = (float)( 2*grid * (j-1) );
			rt = (float)( 2*grid *  j    );

			if(lt<=y && y<=rt) {
				Y_U[i]=j;
				D_U[i]=y-lt;
			}
		}
	}

	//////////////////////// compute interpolation  ////////////////////////

	float coef = 1.0f / ( 2*grid * 2*grid );
	float TL,TR,BL,BR,det_L,det_U,det_R,det_D,tmp;

	for(j=0; j<stat_height; j++) {
		for(i=0; i<stat_width; i++) {
			ii=X_L[i];
			jj=Y_U[j];
			TL=(float)org_tab[  jj    * gain_width +  ii    ];
			TR=(float)org_tab[  jj    * gain_width + (ii+1) ];
			BL=(float)org_tab[ (jj+1) * gain_width +  ii    ];
			BR=(float)org_tab[ (jj+1) * gain_width + (ii+1) ];
			det_L=D_L[i];
			det_U=D_U[j];
			det_R=2*grid - det_L;
			det_D=2*grid - det_U;

			tmp = coef * ( TL*det_D*det_R + TR*det_D*det_L + BL*det_U*det_R + BR*det_U*det_L );
			new_tab[ j * stat_width + i] = (unsigned short) ( tmp + 0.5f );
		}
	}
}

static void lsc_inverse_ae_stat(struct lsc_ctrl_context *cxt, cmr_u16 *inverse_table)
{
	cmr_u32 i;
	cmr_u16 gain_r [ 32*32 ];
	cmr_u16 gain_gr[ 32*32 ];
	cmr_u16 gain_gb[ 32*32 ];
	cmr_u16 gain_b [ 32*32 ];
	cmr_u16 gain_g [ 32*32 ];
	cmr_u16 scaled_gain_r[ 32*32 ];
	cmr_u16 scaled_gain_g[ 32*32 ];
	cmr_u16 scaled_gain_b[ 32*32 ];

	cmr_u32 *stat_r = &cxt->ae_stat[0];
	cmr_u32 *stat_g = &cxt->ae_stat[1024];
	cmr_u32 *stat_b = &cxt->ae_stat[2048];
	cmr_u32 stat_width  = 32;
	cmr_u32 stat_height = 32;
	cmr_u32 img_width  = cxt->img_width;
	cmr_u32 img_height = cxt->img_height;
	cmr_u32 gain_width  = cxt->gain_width;
	cmr_u32 gain_height = cxt->gain_height;
	cmr_u32 grid = cxt->grid;

	save_tab_to_channel(cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern, gain_r, gain_gr, gain_gb, gain_b, inverse_table);
	for(i=0; i<32*32; i++){
		gain_g[i] = (cmr_u32)((gain_gr[i] + gain_gb[i])/2);
	}

	scale_table_to_stat_size( scaled_gain_r , gain_r , gain_width, gain_height, stat_width, stat_height, img_width, img_height, grid );
	scale_table_to_stat_size( scaled_gain_g , gain_g , gain_width, gain_height, stat_width, stat_height, img_width, img_height, grid );
	scale_table_to_stat_size( scaled_gain_b , gain_b , gain_width, gain_height, stat_width, stat_height, img_width, img_height, grid );

	for(i=0; i<32*32; i++){
		stat_r[i] = (cmr_u32)(stat_r[i] / scaled_gain_r [i]) * 1024;
		stat_g[i] = (cmr_u32)(stat_g[i] / scaled_gain_g [i]) * 1024;
		stat_b[i] = (cmr_u32)(stat_b[i] / scaled_gain_b [i]) * 1024;
	}
}

static cmr_s32 linear_lut_bv(cmr_s32 input, cmr_s32 *lut_x_val, cmr_s32 *lut_y_val, cmr_s32 num_ctrl_point)
{
	cmr_s32 output = 0;
	cmr_s32 x0 = 0, x1 = 0;
	cmr_s32 y0 = 0, y1 = 0;
	cmr_s32 input_clip = (input >= 1600) ? 1599 : ((input < 0) ? 0 : input);
	for (cmr_s32 i = 0 ; i < num_ctrl_point-1; i++){
		if (lut_x_val[i] <= input_clip && input_clip < lut_x_val[i+1]){
			x0 = lut_x_val[i];
			x1 = lut_x_val[i+1];
			y0 = lut_y_val[i];
			y1 = lut_y_val[i+1];
		}
	}
	float delta_y = y1 - y0;
	float delta_x = x1 - x0;
	if(delta_x<1.0){
		delta_x = 1.0;
	}
	float slope = delta_y/delta_x;
	output = (cmr_s32)(slope * (input_clip - x0) + 0.5f) + y0;
	return output;
}

static cmr_s32 linear_lut_bv_gain(cmr_s32 input, cmr_s32 *lut_x_val, cmr_s32 *lut_y_val, cmr_s32 num_ctrl_point)
{
	cmr_s32 output = 0;
	cmr_s32 x0 = 0, x1 = 0;
	cmr_s32 y0 = 0, y1 = 0;
	for (cmr_s32 i = 0 ; i < num_ctrl_point-1; i++){
		if (lut_x_val[i] >= input && input > lut_x_val[i+1]){
			x0 = lut_x_val[i];
			x1 = lut_x_val[i+1];
			y0 = lut_y_val[i];
			y1 = lut_y_val[i+1];
		}
	}
	float delta_y = y1 - y0;
	float delta_x = x1 - x0;
	float slope = delta_y/delta_x;
	output = (cmr_s32)(slope * (input - x0) + 0.5f) + y0;
	return output;
}

static void post_shading_gain(cmr_u16 *dst_gain, cmr_u16 *org_gain, cmr_u32 gain_width, cmr_u32 gain_height, cmr_u32 gain_pattern, cmr_u32 frame_count,
							cmr_s32 bv, cmr_s32 bv_gain, cmr_u32 flash_mode, cmr_u32 pre_flash_mode, cmr_u32 LSC_SPD_VERSION, struct post_shading_gain_param *param)
{
	cmr_s32 off_gb = 0, off_gr = 0, off_b = 0, off_r = 0;
	switch (gain_pattern){
		case 0:    //LSC_GAIN_PATTERN_GRBG:
			off_gr = 0;
			off_r  = 1;
			off_b  = 2;
			off_gb = 3;
			break;
		case 1:    //LSC_GAIN_PATTERN_RGGB:
			off_r  = 0;
			off_gr = 1;
			off_gb = 2;
			off_b  = 3;
			break;
		case 2:    //LSC_GAIN_PATTERN_BGGR:
			off_b  = 0;
			off_gb = 1;
			off_gr = 2;
			off_r  = 3;
			break;
		case 3:    //LSC_GAIN_PATTERN_GBRG:
			off_gb = 0;
			off_b  = 1;
			off_r  = 2;
			off_gr = 3;
			break;
		default:
			break;
	}
	cmr_u32 i;
	cmr_s32 bv2gainw_en       = param->bv2gainw_en;			// tunable param
	cmr_s32 *bv2gainw_p_bv    = param->bv2gainw_p_bv;			// tunable param, src + 4 points + dst
	cmr_s32 *bv2gainw_b_gainw = param->bv2gainw_b_gainw;		// tunable param, src + 4 points + dst
	cmr_s32 pbits_gainw       = param->pbits_gainw;
	cmr_s32 pbits_trunc       = param->pbits_trunc;
	cmr_s32 action_bv         = param->action_bv;
	cmr_s32 action_bv_gain    = param->action_bv_gain;
	cmr_s32 gain_adjust_threshold = 1024;
	cmr_s32 num_ctrl_point = 6;
	cmr_s32 pbits_calc = 8;
	cmr_s32 pbits_calc_trunc = 1 << (pbits_calc-1);

	//for in MAIN_FLASH or PRE_FLASH
	if(flash_mode==1 || pre_flash_mode ==1){
        memcpy(dst_gain, org_gain, gain_width*gain_height*4*sizeof(unsigned short));
        bv2gainw_en = 0;  // bypass post gain during flash
        ISP_LOGV("[ALSC] post_gain_act, flash bypass, frame_count=%d", frame_count);
	}

	if(bv2gainw_en){
		if ((bv < action_bv && LSC_SPD_VERSION <=4) || (bv_gain > action_bv_gain && LSC_SPD_VERSION >4)){
			cmr_s32 bv2gainw = 1024;
			if(LSC_SPD_VERSION <=4){
				if(bv <= bv2gainw_p_bv[0]){
					bv2gainw = bv2gainw_b_gainw[0];
				}else{
					bv2gainw = linear_lut_bv(bv, bv2gainw_p_bv, bv2gainw_b_gainw, num_ctrl_point);
				}
			}else{
				if(bv_gain >= bv2gainw_p_bv[0]){
					bv2gainw = bv2gainw_b_gainw[0];
				}else{
					bv2gainw = linear_lut_bv_gain(bv_gain, bv2gainw_p_bv, bv2gainw_b_gainw, num_ctrl_point);
				}
			}
			ISP_LOGV("[ALSC] post_gain_act, bv=%d, bv_gain=%d, bv2gainw=%d, frame_count=%d", bv, bv_gain, bv2gainw, frame_count);

			if(bv2gainw == 0){
				for(i=0; i<gain_width*gain_height*4; i++){
					dst_gain[i] = 1024;
				}
				ISP_LOGV("[ALSC] post_gain_act, bv2gainw=0, set 1X gain, bv=%d, bv_gain=%d, frame_count=%d", bv, bv_gain, frame_count);
			}else{
				for (i=0; i<gain_width*gain_height; i++){
					cmr_s32 gain_gr = org_gain[4*i+off_gr];
					cmr_s32 gain_gb = org_gain[4*i+off_gb];
					cmr_s32 gain_g = (gain_gr + gain_gb) >> 1;
					if ( gain_g > gain_adjust_threshold ){
						cmr_s32 gain_diff_g = gain_g - gain_adjust_threshold;
						cmr_s32 gain_g_compensated =((bv2gainw * gain_diff_g  + pbits_trunc) >> pbits_gainw) + gain_adjust_threshold;
						cmr_s32 gain_align = gain_g;

						cmr_s32 cgain_r  = (org_gain[4*i+off_r ] << pbits_calc)/gain_align;
						cmr_s32 cgain_gr = (org_gain[4*i+off_gr] << pbits_calc)/gain_align;
						cmr_s32 cgain_gb = (org_gain[4*i+off_gb] << pbits_calc)/gain_align;
						cmr_s32 cgain_b  = (org_gain[4*i+off_b ] << pbits_calc)/gain_align;

						cmr_s32 cgain_r_compensated  = (1024 * cgain_r  + pbits_trunc) >> pbits_gainw;
						cmr_s32 cgain_gr_compensated = (1024 * cgain_gr + pbits_trunc) >> pbits_gainw;
						cmr_s32 cgain_gb_compensated = (1024 * cgain_gb + pbits_trunc) >> pbits_gainw;
						cmr_s32 cgain_b_compensated  = (1024 * cgain_b  + pbits_trunc) >> pbits_gainw;
						dst_gain[4*i+off_r ] = (cgain_r_compensated  * gain_g_compensated + pbits_calc_trunc) >> pbits_calc;
						dst_gain[4*i+off_gr] = (cgain_gr_compensated * gain_g_compensated + pbits_calc_trunc) >> pbits_calc;
						dst_gain[4*i+off_gb] = (cgain_gb_compensated * gain_g_compensated + pbits_calc_trunc) >> pbits_calc;
						dst_gain[4*i+off_b ] = (cgain_b_compensated  * gain_g_compensated + pbits_calc_trunc) >> pbits_calc;
					}
				}
			}
		}else{
			memcpy(dst_gain, org_gain, gain_width*gain_height*4*sizeof(unsigned short));
			ISP_LOGV("[ALSC] post_gain_act, bypass, action_bv=%d, action_bv_gain=%d, bv=%d, bv_gain=%d, frame_count=%d", action_bv, action_bv_gain, bv, bv_gain, frame_count);
		}
	}else{
		memcpy(dst_gain, org_gain, gain_width*gain_height*4*sizeof(unsigned short));
	}
}

cmr_u32 get_alsc_alg_in_flag(struct lsc_ctrl_context *cxt, cmr_u32 *IIR_weight)
{
	cmr_s32 rtn = 0;

	// pre_flash_mode in
	if(cxt->pre_flash_mode){
		ISP_LOGV("[ALSC] pre_flash_mode=1, do alsc_calc, lsc_id=%d", cxt->lsc_id);
		return 1;
	}

	if(cxt->frame_count == 1){
		cxt->alg_quick_in = 1;
		cxt->quik_in_start_frame = -99;
		ISP_LOGV("[ALSC] frame_count=1, set alg_quick_in=1 and wait for init_skip_frame, lsc_id=%d", cxt->lsc_id);
	}

	// init_skip_frame return
	if(cxt->frame_count < cxt->init_skip_frame){
		ISP_LOGV("[ALSC] init_skip_frame=%d, frame_count=%d, return, lsc_id=%d", cxt->init_skip_frame, cxt->frame_count, cxt->lsc_id);
		return rtn;
	}

	// main flash return
	if(cxt->flash_mode == 1){
		ISP_LOGV("[ALSC] MAIN_FLASH return, ctx->flash_mode=1, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		return rtn;
	}

	// can_update_dest 0 return
	if(cxt->can_update_dest == 0){
		ISP_LOGV("[ALSC] can_update_dest = 0 return, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		return rtn;
	}

	// alg_quick_in
	if(cxt->alg_quick_in == 1){
		// alg_quick_in start
		if(cxt->quik_in_start_frame == -99){
			cxt->quik_in_start_frame = cxt->frame_count;
			*IIR_weight = 16;   // drop the last calc result, and send new calc result to output
		}

		// during alg_quick_in
		if(cxt->frame_count < (cxt->quik_in_start_frame + cxt->alg_quick_in_frame)){
			rtn = 1;
			ISP_LOGV("[ALSC] rtn=%d, frame_count=%d, IIR_weight=%d ,quik_in_start_frame=%d, alg_quick_in=%d, lsc_id=%d",
					rtn, cxt->frame_count, *IIR_weight, cxt->quik_in_start_frame, cxt->alg_quick_in, cxt->lsc_id);
		// alg_quick_in end
		}else{
			rtn = 0;
			cxt->quik_in_start_frame = -99;
			cxt->alg_quick_in = 0;
		}
	// normal in
	}else{
		rtn = 0;
		cxt->quik_in_start_frame = -99;

		// set calc_freq
		if(cxt->frame_count%(cxt->calc_freq*3) == 1)
			rtn = 1;

		// alsc locked
		if(cxt->alg_locked && cxt->alg_quick_in == 0 && cxt->pre_flash_mode == 0 && cxt->frame_count){
			ISP_LOGV("[ALSC] is locked && cxt->alg_quick_in==0 && cxt->pre_flash_mode==0 , frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
			rtn=0;
		}
	}

    return rtn;
}

static cmr_u32 print_lsc_log(void)
{
	char value[256] = { 0 };
	cmr_u32 is_print = 0;
	property_get("debug.camera.isp.lsc", value, "0");

	if (!strcmp(value, "1")) {
		is_print = 1;
	}
	return is_print;
}

static cmr_s32 lsc_sprd_calculation(void *handle, void *in, void *out)
{
	cmr_u32 i;
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_calc_param *param = (struct lsc_adv_calc_param *)in;
	struct lsc_adv_calc_result *result = (struct lsc_adv_calc_result *)out;
    cmr_u32 alg_in = 0;
	cmr_u32 img_width  = param->img_size.w;
	cmr_u32 img_height = param->img_size.h;
	cmr_u32 gain_width  = param->gain_width;
	cmr_u32 gain_height = param->gain_height;
	cmr_u32 grid = param->grid;
	cmr_u32 IIR_weight = 5;
	cmr_u32 *stat_g = NULL;

	if (!handle) {
		ISP_LOGE("fail to check param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;
	IIR_weight = cxt->IIR_weight;
	struct debug_lsc_param* lsc_debug_info_ptr = (struct debug_lsc_param*)cxt->lsc_debug_info_ptr;
	struct lsc_last_info* lsc_last_info = (struct lsc_last_info*)cxt->lsc_last_info;
	struct post_shading_gain_param *post_param = (struct post_shading_gain_param*)cxt->post_shading_gain_param;
	lsc_last_info->bv = param->bv;
	lsc_last_info->bv_gain = param->bv_gain;

	if(cxt->LSC_SPD_VERSION >= 6){
		gain_width  = cxt->gain_width;
		gain_height = cxt->gain_height;
		grid = cxt->grid;
	}

	//save debug info
    lsc_debug_info_ptr->gain_width = gain_width;
    lsc_debug_info_ptr->gain_height = gain_height;
    lsc_debug_info_ptr->gain_pattern = cxt->output_gain_pattern;
    lsc_debug_info_ptr->grid = grid;

	if( cxt->lsc_id == 2 /*slave*/ && cxt->is_multi_mode ==2/*ISP_DUAL_SBS*/ && cxt->is_master ==0 /*slave*/){
		ISP_LOGV("[ALSC] return due to ISP_DUAL_SBS, slave, lsc_id=%d, frame_count=%d", cxt->lsc_id, cxt->frame_count);
		return rtn;
	}

	result->dst_gain = cxt->dst_gain;
	ATRACE_BEGIN(__FUNCTION__);
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);

	// alsc_calc ++
	// change mode
	if(cxt->fw_start_end){
		ISP_LOGV("[ALSC] change mode: LSC_SPD_VERSION=%d, start frame_count=%d, lsc_id=%d", cxt->LSC_SPD_VERSION, cxt->frame_count, cxt->lsc_id);
		if(cxt->LSC_SPD_VERSION <=5){
			if(cxt->lsc_pm0 != param->lsc_tab_address[0]){
				ISP_LOGV("[ALSC] change mode: pre_pm0=%p, new_pm0=%p", cxt->lsc_pm0, param->lsc_tab_address[0]);
				ISP_LOGV("[ALSC] change mode: pre, img_size[%d,%d], table_size[%d,%d], grid=%d, lsc_id=%d", cxt->img_width, cxt->img_height, cxt->gain_width, cxt->gain_height, cxt->grid, cxt->lsc_id);
				ISP_LOGV("[ALSC] change mode: new, img_size[%d,%d], table_size[%d,%d], grid=%d, lsc_id=%d", img_width, img_height, gain_width, gain_height, grid, cxt->lsc_id);

			if(gain_width != cxt->gain_width || gain_height != cxt->gain_height || cxt->flash_mode == 1 || cxt->pre_flash_mode == 1 || cxt->frame_count == 0){			
				cxt->img_width = img_width;
				cxt->img_height = img_height;
				cxt->gain_width = gain_width;
				cxt->gain_height = gain_height;
				cxt->grid = grid;
				cxt->lsc_pm0 = param->lsc_tab_address[0];
				memcpy(lsc_debug_info_ptr->last_lsc_table  , cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(cmr_u16));
				memcpy(lsc_debug_info_ptr->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(cmr_u16));

				// do lsc param preprocess without otp
				for(i=0; i<8; i++){
					memcpy(cxt->std_lsc_table_param_buffer[i], param->lsc_tab_address[i], gain_width*gain_height*4*sizeof(cmr_u16));
					change_lsc_pattern(cxt->std_lsc_table_param_buffer[i], gain_width, gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
				}
				sync_g_channel(cxt->std_lsc_table_param_buffer, gain_width, gain_height, cxt->output_gain_pattern);
			}

				cxt->fw_start_end=0;
				cxt->can_update_dest=1;
				if(cxt->frame_count == 0){//lunch camera with binning size or 720p both will run the fw_start
					cxt->frame_count = 1;
					cxt->alg_count = 0;   // set zero to skip iir
					ISP_LOGV("[ALSC] change mode END (return 0): set frame_count=1, set alg_count=1 to do quick in, lsc_id=%d", cxt->lsc_id);
					return rtn;
				}else{
					cxt->frame_count = cxt->calc_freq*3-2;  // do not quick in and skip 3 frame for AEM stable
					cxt->alg_count = 0;                     // set zero to skip iir
					ISP_LOGV("[ALSC] change mode END (return 0): set ALSC normal in, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
					return rtn;
				}
			}else{
				cxt->fw_start_end=0;
				cxt->can_update_dest=1;
				ISP_LOGV("[ALSC] change mode SKIP (return 0): protect for previouse calc thread, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
				return rtn;
			}
		}else{
			ISP_LOGV("[ALSC] change mode: pre_lsc_pm_mode=%d, cur_lsc_pm_mode=%d, lsc_id=%d", cxt->pre_lsc_pm_mode, cxt->cur_lsc_pm_mode, cxt->lsc_id);
			ISP_LOGV("[ALSC] change mode: pre_img_size=[%d,%d], new_img_size=[%d,%d], lsc_id=%d", cxt->img_width, cxt->img_height, img_width, img_height, cxt->lsc_id);

			cxt->pre_lsc_pm_mode = cxt->cur_lsc_pm_mode;
			cxt->img_width = img_width;
			cxt->img_height = img_height;
			memcpy(lsc_debug_info_ptr->last_lsc_table  , cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(cmr_u16));
			memcpy(lsc_debug_info_ptr->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(cmr_u16));

			cxt->fw_start_end=0;
			cxt->can_update_dest=1;
			if(cxt->frame_count == 0){//lunch camera with binning size or 720p both will run the fw_start
				cxt->frame_count = 1;
				cxt->alg_count = 0;   // set zero to skip iir
				ISP_LOGV("[ALSC] change mode END (return 0): set frame_count=1, set alg_count=1 to do quick in, lsc_id=%d", cxt->lsc_id);
				return rtn;
			}else{
				cxt->frame_count = cxt->calc_freq*3-2;  // do not quick in and skip 3 frame for AEM stable
				cxt->alg_count = 0;                     // set zero to skip iir
				ISP_LOGV("[ALSC] change mode END (return 0): set ALSC normal in, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
				return rtn;
			}
		}
	}

	// updata the mlog info
	if(print_lsc_log()==1 && cxt->alg_count >=1){
		FILE *pf = NULL;
		const char saveLogFile1[50] = "/data/mlog/lsc1.txt";
		const char saveLogFile2[50] = "/data/mlog/lsc2.txt";
		if(cxt->lsc_id == 1){
			pf = fopen(saveLogFile1, "wb");
		}else{
			pf = fopen(saveLogFile2, "wb");
		}
		if (NULL != pf){
			fprintf(pf, "LSC BASIC INFO\r\n");
			fprintf(pf, "alg2.c VER: %s \r\n", lsc_debug_info_ptr->LSC_version);
			fprintf(pf, "final_index: %d, final_ratio_x10000: %d\r\n", lsc_debug_info_ptr->final_index, lsc_debug_info_ptr->final_ratio_x10000);
			fprintf(pf, "bv: %d, bv_gain: %d, ct: %d\r\n", param->bv, param->bv_gain, param->ct);
			fprintf(pf, "rGain: %d, bGain: %d\r\n", param->r_gain, param->b_gain);
			fprintf(pf, "image_size: [%d,%d], grid: %d\r\n", img_width, img_height, grid);
			fprintf(pf, "gain_size: [%d,%d], gain_pattern: %d\r\n", gain_width, gain_height, cxt->output_gain_pattern);
			fprintf(pf, "flash_mode: %d, front_cam: %d\r\n", cxt->flash_mode, cxt->camera_id);
			fprintf(pf, "alg_locked: %d, cam_id: %d\r\n", cxt->alg_locked, cxt->lsc_id);
			fprintf(pf, "alg_cnt: %d, frame_cnt: %d\r\n", cxt->alg_count, cxt->frame_count);
			fprintf(pf, "TAB_outer: [%d,%d,%d,%d]\r\n", lsc_debug_info_ptr->output_lsc_table[0], lsc_debug_info_ptr->output_lsc_table[1], lsc_debug_info_ptr->output_lsc_table[2], lsc_debug_info_ptr->output_lsc_table[3]);
			fprintf(pf, "TAB_inner: [%d,%d,%d,%d]\r\n", lsc_debug_info_ptr->output_lsc_table[4*(gain_width+1)+0], lsc_debug_info_ptr->output_lsc_table[4*(gain_width+1)+1], lsc_debug_info_ptr->output_lsc_table[4*(gain_width+1)+2], lsc_debug_info_ptr->output_lsc_table[4*(gain_width+1)+3]);
			fclose(pf);
		}
	}

	// first frame just copy the output from fw_start
	if(cxt->frame_count == 0){   //lunch camera with fullsize
		memcpy(lsc_debug_info_ptr->last_lsc_table  , cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(cmr_u16));
		memcpy(lsc_debug_info_ptr->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(cmr_u16));
		alg_in = 0;
		ISP_LOGV("[ALSC] frame_count=0, init table is fwstart_new_scaled_table[%d,%d,%d,%d], lsc_id=%d",
				cxt->fwstart_new_scaled_table[0], cxt->fwstart_new_scaled_table[1], cxt->fwstart_new_scaled_table[2], cxt->fwstart_new_scaled_table[3], cxt->lsc_id);
	}else{
		alg_in = get_alsc_alg_in_flag(cxt, &IIR_weight);
		ISP_LOGV("[ALSC] frame_count=%d, alg_in=%d, lsc_id=%d", cxt->frame_count, alg_in, cxt->lsc_id);
	}

	if(cxt->cmd_alsc_cmd_enable){
		alsc_get_cmd(cxt);
		if(cxt->cmd_alsc_bypass){
			alg_in = 0;
			ISP_LOGI("[ALSC] cmd_alsc_bypass, frame_count=%d, alg_in=%d, lsc_id=%d", cxt->frame_count, alg_in, cxt->lsc_id);
		}
	}

	// alsc calculation process
	if(alg_in && cxt->alg_bypass == 0){
		// select std lsc param
		if(cxt->init_gain_width == gain_width && cxt->init_gain_height == gain_height) {
			for(i=0; i<8; i++)
				param->std_tab_param[i] = cxt->std_init_lsc_table_param_buffer[i];
		}else{
			for(i=0; i<8; i++)
				param->std_tab_param[i] = cxt->std_lsc_table_param_buffer[i];
		}

		// scale AEM size to 32x32
		lsc_scl_for_ae_stat(cxt,param);

		// cmd dump AEM0
		if(cxt->cmd_alsc_cmd_enable){
			if(cxt->cmd_alsc_dump_aem){
				cmd_dump_aem(img_width, img_height, param->stat_img.r, param->stat_img.gr, param->stat_img.b, cxt->lsc_id, 0);
			}
		}

		// stat_g = 0 return
		stat_g = param->stat_img.gr;
		for(i=0; i<32*32; i++){
			if(stat_g[i] == 0){
				ISP_LOGV("[ALSC] stat_g = 0 return, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
				return rtn;
			}
		}

		// inverse the LSC on AEM
		lsc_inverse_ae_stat(cxt, lsc_debug_info_ptr->output_lsc_table);

		// cmd dump AEM1
		if(cxt->cmd_alsc_cmd_enable){
			if(cxt->cmd_alsc_dump_aem){
				cmd_dump_aem(img_width, img_height, param->stat_img.r, param->stat_img.gr, param->stat_img.b, cxt->lsc_id, 1);
			}
		}

		// call liblsc.so
		rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, param, result);

		// table smooth process
		if(cxt->alg_count){
			if(cxt->can_update_dest){
				ISP_LOGV("[ALSC] smooth Begin, result->dst_gain=[%d,%d,%d,%d], alg_count=%d, frame_count=%d, lsc_id=%d", result->dst_gain[0], result->dst_gain[1], result->dst_gain[2], result->dst_gain[3], cxt->alg_count, cxt->frame_count, cxt->lsc_id);
				for(i=0; i<gain_width*gain_height*4; i++){
					lsc_debug_info_ptr->alsc_smooth_lsc_table[i] = (cmr_u16)( ( IIR_weight*result->dst_gain[i] + (16 - IIR_weight)*lsc_debug_info_ptr->last_lsc_table[i] ) / 16.0f );
				}
				memcpy(lsc_debug_info_ptr->last_lsc_table, lsc_debug_info_ptr->alsc_smooth_lsc_table, gain_width*gain_height*4*sizeof(cmr_u16));
				ISP_LOGV("[ALSC] smooth End, alsc_smooth_lsc_table=[%d,%d,%d,%d], alg_count=%d, frame_count=%d, lsc_id=%d", lsc_debug_info_ptr->alsc_smooth_lsc_table[0], lsc_debug_info_ptr->alsc_smooth_lsc_table[1], lsc_debug_info_ptr->alsc_smooth_lsc_table[2], lsc_debug_info_ptr->alsc_smooth_lsc_table[3], cxt->alg_count, cxt->frame_count, cxt->lsc_id);

			}else{
				ISP_LOGV("[ALSC] smooth SKIP, can_update_dest=%d, frame_count=%d, lsc_id=%d ", cxt->can_update_dest, cxt->frame_count, cxt->lsc_id);
			}
		}else{
			memcpy(lsc_debug_info_ptr->last_lsc_table, result->dst_gain, gain_width*gain_height*4*sizeof(cmr_u16));
			ISP_LOGV("[ALSC] alg_count = 0, smooth SKIP, can_update_dest=%d, frame_count=%d, lsc_id=%d ", cxt->can_update_dest, cxt->frame_count, cxt->lsc_id);
		}
		cxt->alg_count ++;
	}

	// replace bv and bv_gain after flash
	cxt->bv_memory[cxt->frame_count%10] = param->bv;
	cxt->bv_gain_memory[cxt->frame_count%10] = param->bv_gain;
	if(cxt->bv_skip_frame){
		cxt->bv_skip_frame --;
		ISP_LOGV("[ALSC] flash end in bv_skip_frame, replace param->bv=%d to bv=%d, param->bv_gain=%d to bv_gain=%d, bv_skip_frame=%d, frame_count=%d, lsc_id=%d ", param->bv, cxt->bv_before_flash, param->bv_gain, cxt->bv_gain_before_flash, cxt->bv_skip_frame, cxt->frame_count, cxt->lsc_id);
		param->bv = cxt->bv_before_flash;
		param->bv_gain = cxt->bv_gain_before_flash;
	}

	// cmd set lsc output
	if(cxt->cmd_alsc_cmd_enable && !cxt->cmd_alsc_bypass){
		if(cxt->cmd_alsc_table_pattern){
			cmd_set_lsc_output(cxt->lsc_buffer, gain_width, gain_height, cxt->output_gain_pattern);
			ISP_LOGV("[ALSC] cmd_set_lsc_output, final output cxt->lsc_buffer[%d,%d,%d,%d], lsc_id=%d",
					cxt->lsc_buffer[0], cxt->lsc_buffer[1], cxt->lsc_buffer[2], cxt->lsc_buffer[3] , cxt->lsc_id);
			return rtn;
		}
	}

	// cmd set table index
	if(cxt->cmd_alsc_cmd_enable && !cxt->cmd_alsc_bypass){
		ISP_LOGI("[ALSC] cmd_alsc_table_index=%d", cxt->cmd_alsc_table_index);
		if(cxt->cmd_alsc_table_index <= 8 && cxt->cmd_alsc_table_index >= 0){
			if(cxt->init_gain_width == gain_width && cxt->init_gain_height == gain_height) {
				memcpy(cxt->lsc_buffer, cxt->std_init_lsc_table_param_buffer[cxt->cmd_alsc_table_index], gain_width*gain_height*4*sizeof(cmr_u16));
			}else{
				memcpy(cxt->lsc_buffer, cxt->std_lsc_table_param_buffer[cxt->cmd_alsc_table_index], gain_width*gain_height*4*sizeof(cmr_u16));
			}
			ISP_LOGI("[ALSC]  cmd set table index %d, final output cxt->lsc_buffer[%d,%d,%d,%d], lsc_id=%d", cxt->cmd_alsc_table_index,
					cxt->lsc_buffer[0], cxt->lsc_buffer[1], cxt->lsc_buffer[2], cxt->lsc_buffer[3] , cxt->lsc_id);
			return rtn;
		}
	}

	if(!cxt->cmd_alsc_bypass){
		if(cxt->alg_count <= cxt->alg_quick_in_frame + 1 && cxt->frame_count < cxt->calc_freq*3 + 1 && cxt->can_update_dest == 1 && param->bv*cxt->fw_start_bv > 0){
			memcpy(lsc_debug_info_ptr->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(unsigned short));
			memcpy(cxt->lsc_buffer                     , cxt->fwstart_new_scaled_table, gain_width*gain_height*4*sizeof(unsigned short));
			ISP_LOGV("[ALSC] alg_quick_in_frame=%d, alg_count=%d, frame_count=%d, replace output by fwstart_new_scaled_table[%d,%d,%d,%d], lsc_id=%d", cxt->alg_quick_in_frame, cxt->alg_count, cxt->frame_count,
					cxt->lsc_buffer[0], cxt->lsc_buffer[1], cxt->lsc_buffer[2], cxt->lsc_buffer[3], cxt->lsc_id);
		}else{
			if( cxt->can_update_dest ==1 ){
				post_shading_gain( lsc_debug_info_ptr->output_lsc_table, lsc_debug_info_ptr->last_lsc_table, gain_width, gain_height, cxt->output_gain_pattern, cxt->frame_count,
								param->bv, param->bv_gain, cxt->flash_mode, cxt->pre_flash_mode, cxt->LSC_SPD_VERSION, post_param);
				// copy the table after post_gain_act
				memcpy(cxt->lsc_buffer, lsc_debug_info_ptr->output_lsc_table, gain_width*gain_height*4*sizeof(unsigned short));
			}else{
				ISP_LOGV("[ALSC] post_gain SKIP, Not Update dst_gain due to just fw_start, frame_count=%d, lsc_id=%d, can_update_dest=%d", cxt->frame_count, cxt->lsc_id, cxt->can_update_dest);
			}
		}
	}else{
		ISP_LOGV("[ALSC] cmd bypass");
	}
	ISP_LOGV("[ALSC] final output cxt->lsc_buffer[%d,%d,%d,%d], frame_count=%d, lsc_id=%d, can_update_dest=%d",
			cxt->lsc_buffer[0], cxt->lsc_buffer[1], cxt->lsc_buffer[2], cxt->lsc_buffer[3], 
			cxt->frame_count, cxt->lsc_id, cxt->can_update_dest);

	cxt->flash_done_frame_count++;
	cxt->frame_count++;
	// alsc_calc --
	if(cxt->cmd_alsc_cmd_enable){
		if(cxt->cmd_alsc_dump_table){
			dump_lsc_gain(cxt->lsc_buffer,gain_width,gain_height);
		}
	}
	cxt->alsc_update_flag = 1;
	if(!cxt->can_update_dest)
		cxt->alsc_update_flag = 0;

	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("SYSTEM_TEST -lsc_test  %dus ", (cmr_s32) ((ae_time1 - ae_time0) / 1000));
	return rtn;
}

static cmr_s32 fwstart_update_first_tab(struct lsc_ctrl_context *cxt, struct alsc_fwstart_info* fwstart_info)
{
	cmr_u32 i;
	ISP_LOGV("[ALSC] FW_START, fwstart_update_first_tab, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);

	// protection
	if( cxt->lsc_buffer == NULL || cxt->lsc_pm0 == NULL || cxt->fwstop_output_table[0] == 0){
		ISP_LOGV("[ALSC] FW_START, fwstart_update_first_tab SKIP, cxt->lsc_buffer %p, cxt->lsc_pm0 %p, cxt->fwstop_output_table[0] = %d", cxt->lsc_buffer, cxt->lsc_pm0, cxt->fwstop_output_table[0]);
		return -1;
	}

	// collect the previouse dest table, previouse dnp table and other info.
	struct lsc_flash_proc_param* flash_param = (struct lsc_flash_proc_param*)cxt->lsc_flash_proc_param;
	cmr_u32 pre_gain_width   = cxt->gain_width;
	cmr_u32 pre_gain_height  = cxt->gain_height;
	cmr_u32 pre_gain_pattern = cxt->gain_pattern;
	cmr_u16 lsc_pre_reslut_table[32 * 32 * 4]={0};
	cmr_u16 lsc_pre_table       [32 * 32 * 4]={0};

	// flash mode, table without post gain
	if(flash_param->main_flash_from_other_parameter ==1){
		ISP_LOGV("[ALSC] FW_START, Flash Mode, flash_param->main_flash_from_other_parameter ==1, COPY the flash_param->preflash_current_output_table(with post gain) to lsc_pre_reslut_table");
		ISP_LOGV("[ALSC] FW_START, Flash Mode, flash_param->main_flash_from_other_parameter ==1, COPY the flash_param->preflash_current_lnc_table to lsc_pre_table");
		for(i = 0; i < pre_gain_width * pre_gain_height * 4 ; i++){
			//destination buffer table with post gain
			lsc_pre_reslut_table[i] = flash_param->preflash_current_output_table[i];
			//DNP table
			lsc_pre_table[i]        = flash_param->preflash_current_lnc_table[i];     // golden pre DNP from pm, no OTP apply
		}
		flash_param->main_flash_from_other_parameter =0;
		flash_param->preflash_current_lnc_table_address = NULL;
	// normal mode, table with post gain
	}else{
		for(i = 0; i < pre_gain_width * pre_gain_height * 4 ; i++){
			//destination buffer table
			lsc_pre_reslut_table[i] = cxt->fwstop_output_table[i];
			//DNP table
			lsc_pre_table[i]        = cxt->lsc_pm0[i];                            // golden pre DNP from pm, no OTP apply
		}
	}
	memset(cxt->fwstop_output_table, 0, sizeof(cmr_u16)*32*32*4);

	// collect the new dest table, new dnp table and other info
	cmr_u32 new_gain_width  = fwstart_info->gain_width_new;
	cmr_u32 new_gain_height = fwstart_info->gain_height_new;
	cmr_u32 new_gain_pattern = 3;
	cmr_u16 *lsc_result_address_new = fwstart_info->lsc_result_address_new;   //new dest output address
	cmr_u16 lsc_new_table[32 *32*4]={0};                                      // golden new DNP table from pm, no OTP apply

	memcpy(lsc_new_table, fwstart_info->lsc_tab_address_new[0], sizeof(cmr_u16)*new_gain_width*new_gain_height*4); // golden new DNP table from pm, no OTP apply

	switch (fwstart_info->image_pattern_new){
		case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
			new_gain_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_R:
			new_gain_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_B:
			new_gain_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
			new_gain_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

		default:
		break;
	}
	ISP_LOGV("[ALSC] FW_START, pre_gain_width=%d, pre_gain_height=%d, pre_gain_pattern=%d, pre_DNP[%d,%d,%d,%d], pre_result[%d,%d,%d,%d]",
			pre_gain_width, pre_gain_height, pre_gain_pattern, lsc_pre_table[0], lsc_pre_table[1], lsc_pre_table[2], lsc_pre_table[3],
			lsc_pre_reslut_table[0], lsc_pre_reslut_table[1], lsc_pre_reslut_table[2], lsc_pre_reslut_table[3]);
	ISP_LOGV("[ALSC] FW_START, new_gain_width=%d, new_gain_height=%d, new_gain_pattern=%d, new_DNP[%d,%d,%d,%d]",
			new_gain_width, new_gain_height, new_gain_pattern, lsc_new_table[0], lsc_new_table[1], lsc_new_table[2], lsc_new_table[3]);

	// same table size, just copy result
	if(pre_gain_width == new_gain_width && pre_gain_height == new_gain_height){
		memcpy(lsc_result_address_new, lsc_pre_reslut_table, pre_gain_width*pre_gain_height*4*sizeof(cmr_u16));
	// different table size, scale old size table to new size table
	}else{
		// scale pre DNP table to new gain size
		cmr_u16 output_tab   [ 32 * 32 * 4] = {0};
		cmr_u16 output_r_tab [ 32 * 32 ] = {0};
		cmr_u16 output_gr_tab[ 32 * 32 ] = {0};
		cmr_u16 output_gb_tab[ 32 * 32 ] = {0};
		cmr_u16 output_b_tab [ 32 * 32 ] = {0};

		_table_linear_scaler(lsc_pre_table, pre_gain_width, pre_gain_height, output_tab, new_gain_width, new_gain_height, 0);
		save_tab_to_channel(new_gain_width, new_gain_height, pre_gain_pattern, output_r_tab, output_gr_tab, output_gb_tab, output_b_tab, output_tab);

		// get new DNP tab to 4 channel
		cmr_u16 output_r_tab_new [ 32 * 32 ] = {0};
		cmr_u16 output_gr_tab_new[ 32 * 32 ] = {0};
		cmr_u16 output_gb_tab_new[ 32 * 32 ] = {0};
		cmr_u16 output_b_tab_new [ 32 * 32 ] = {0};

		save_tab_to_channel(new_gain_width, new_gain_height, new_gain_pattern, output_r_tab_new, output_gr_tab_new, output_gb_tab_new, output_b_tab_new, lsc_new_table);

		// get scale weight table from the ratio of the 2 DNP tables, that is (new DNP)/(old scaled DNP)
		float lsc_new_weight_tab_gb[32*32] = {0};
		float lsc_new_weight_tab_b [32*32] = {0};
		float lsc_new_weight_tab_r [32*32] = {0};
		float lsc_new_weight_tab_gr[32*32] = {0};
		float rate_gb = 0.0;
		float rate_b  = 0.0;
		float rate_r  = 0.0;
		float rate_gr = 0.0;

		for(i=0; i< new_gain_width * new_gain_height ;i++){
			rate_r  = 0.0;
			rate_gr = 0.0;
			rate_gb = 0.0;
			rate_b  = 0.0;
			//r
			if( output_r_tab[i] == 0 || output_r_tab[i]==1024 ){
				rate_r=1;
			}else{
				rate_r=(float)output_r_tab_new[i]/(float)output_r_tab[i];
			}
			lsc_new_weight_tab_r[i]=rate_r;
			//gr
			if( output_gr_tab[i] == 0 || output_gr_tab[i]==1024 ){
				rate_gr=1;
			}else{
				rate_gr=(float)output_gr_tab_new[i]/(float)output_gr_tab[i];
			}
			lsc_new_weight_tab_gr[i]=rate_gr;
			//gb
			if( output_gb_tab[i] == 0 || output_gb_tab[i]==1024 ){
				rate_gb=1;
			}else{
				rate_gb=(float)output_gb_tab_new[i]/(float)output_gb_tab[i];
			}
			lsc_new_weight_tab_gb[i]=rate_gb;
			//b
			if( output_b_tab[i] == 0 || output_b_tab[i]==1024 ){
				rate_b=1;
			}else{
				rate_b=(float)output_b_tab_new[i]/(float)output_b_tab[i];
			}
			lsc_new_weight_tab_b[i]=rate_b;
		}

		// scale dest table
		cmr_u16 output_r [ 32 * 32 ] = {0};
		cmr_u16 output_gr[ 32 * 32 ] = {0};
		cmr_u16 output_gb[ 32 * 32 ] = {0};
		cmr_u16 output_b [ 32 * 32 ] = {0};

		_table_linear_scaler(lsc_pre_reslut_table, pre_gain_width, pre_gain_height, lsc_result_address_new, new_gain_width, new_gain_height, 0);
		save_tab_to_channel(new_gain_width, new_gain_height, cxt->output_gain_pattern, output_r, output_gr, output_gb, output_b, lsc_result_address_new);
		ISP_LOGV("[ALSC] FW_START, pre_scaled_DNP[%d,%d,%d,%d], pre_scaled_result[%d,%d,%d,%d]",
				output_tab[0], output_tab[1], output_tab[2], output_tab[3],
				lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);

		if(cxt->change_pattern_flag)
			new_gain_pattern = cxt->output_gain_pattern;

		// send output to lsc_result_address_new
		for(i=0; i< new_gain_width * new_gain_height; i++){
			switch (new_gain_pattern){
				case LSC_GAIN_PATTERN_GRBG:
					lsc_result_address_new[4*i + 0] = output_gr[i]*lsc_new_weight_tab_gr[i];
					lsc_result_address_new[4*i + 1] = output_r [i]*lsc_new_weight_tab_r[i];
					lsc_result_address_new[4*i + 2] = output_b [i]*lsc_new_weight_tab_b[i];
					lsc_result_address_new[4*i + 3] = output_gb[i]*lsc_new_weight_tab_gb[i];
				break;
				case LSC_GAIN_PATTERN_RGGB:
					lsc_result_address_new[4*i + 0] = output_r [i]*lsc_new_weight_tab_r[i];
					lsc_result_address_new[4*i + 1] = output_gr[i]*lsc_new_weight_tab_gr[i];
					lsc_result_address_new[4*i + 2] = output_gb[i]*lsc_new_weight_tab_gb[i];
					lsc_result_address_new[4*i + 3] = output_b [i]*lsc_new_weight_tab_b[i];
				break;
				case LSC_GAIN_PATTERN_BGGR:
					lsc_result_address_new[4*i + 0] = output_b [i]*lsc_new_weight_tab_b[i];
					lsc_result_address_new[4*i + 1] = output_gb[i]*lsc_new_weight_tab_gb[i];
					lsc_result_address_new[4*i + 2] = output_gr[i]*lsc_new_weight_tab_gr[i];
					lsc_result_address_new[4*i + 3] = output_r [i]*lsc_new_weight_tab_r[i];
				break;
				case LSC_GAIN_PATTERN_GBRG:
					lsc_result_address_new[4*i + 0] = output_gb[i]*lsc_new_weight_tab_gb[i];
					lsc_result_address_new[4*i + 1] = output_b [i]*lsc_new_weight_tab_b[i];
					lsc_result_address_new[4*i + 2] = output_r [i]*lsc_new_weight_tab_r[i];
					lsc_result_address_new[4*i + 3] = output_gr[i]*lsc_new_weight_tab_gr[i];
				break;
				default:
				break;
			}
		}
		ISP_LOGV("[ALSC] FW_START, pre_scaled_weighted_result[%d,%d,%d,%d]",
				lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);

		//cliping for table max value 16383
		for(i=0; i< new_gain_width * new_gain_height * 4; i++){
			if(lsc_result_address_new[i] > 16383) lsc_result_address_new[i]=16383;
			if(lsc_result_address_new[i] < 1024) lsc_result_address_new[i]=1024;
		}
	}

	// update zone
	memcpy(cxt->fwstart_new_scaled_table, lsc_result_address_new, new_gain_width*new_gain_height*4*sizeof(cmr_u16));

	ISP_LOGV("[ALSC] FW_START, COPY the lsc_result_address_new to guessing mainflash output table");
	// binning preflash -> fullsize main flash
	memcpy(flash_param->preflash_guessing_mainflash_output_table, lsc_result_address_new, new_gain_width*new_gain_height*4*sizeof(cmr_u16));

	ISP_LOGV("[ALSC] FW_START, fwstart_update_first_tab done, lsc_result_address_new address %p, lsc_result_address_new=[%d,%d,%d,%d]", lsc_result_address_new,
			lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);
    return 0;
}

static void lsc_save_last_info(struct lsc_last_info* cxt, cmr_u32 camera_id, cmr_u32 full_flag)
{
	FILE* fp = NULL;

	if(camera_id == 0){
		if(full_flag == 1){
			fp = fopen(CAMERA_DATA_FILE"/lsc.file", "wb");
		}else{
			fp = fopen(CAMERA_DATA_FILE"/lsc_video.file", "wb");
		}
	}else if(camera_id == 1){
		if(full_flag == 1){
			fp = fopen(CAMERA_DATA_FILE"/lsc_front.file", "wb");
		}else{
			fp = fopen(CAMERA_DATA_FILE"/lsc_front_video.file", "wb");
		}
	}

	if (fp) {
		ISP_LOGV("[ALSC] write_lsc_last_info, bv=%d, table_rgb[%d,%d,%d], gain_widht=%d, gain_height=%d", cxt->bv
			, cxt->table_r[0], cxt->table_g[0], cxt->table_b[0], cxt->gain_width, cxt->gain_height);

		fwrite((char*)cxt, 1, sizeof(struct lsc_last_info), fp);
		fclose(fp);
		fp = NULL;
	}
}

static void lsc_read_last_info(struct lsc_last_info* cxt, cmr_u32 camera_id, cmr_u32 full_flag)
{
	FILE* fp = NULL;

	if(camera_id == 0){
		if(full_flag == 1){
			fp = fopen(CAMERA_DATA_FILE"/lsc.file", "rb");
		}else{
			fp = fopen(CAMERA_DATA_FILE"/lsc_video.file", "rb");
		}
	}else if(camera_id == 1){
		if(full_flag == 1){
			fp = fopen(CAMERA_DATA_FILE"/lsc_front.file", "rb");
		}else{
			fp = fopen(CAMERA_DATA_FILE"/lsc_front_video.file", "rb");
		}
	}

	if (fp) {
		memset((void*)cxt, 0, sizeof(struct lsc_last_info));
		fread((char*)cxt, 1, sizeof(struct lsc_last_info), fp);
		fclose(fp);
		fp = NULL;

		ISP_LOGV("[ALSC] read_lsc_last_info, bv=%d, table_rgb[%d,%d,%d], gain_widht=%d, gain_height=%d", cxt->bv
				, cxt->table_r[0], cxt->table_g[0], cxt->table_b[0], cxt->gain_width, cxt->gain_height);
	}
}

int pm_lsc_table_crop(struct pm_lsc_full *src, struct pm_lsc_crop *dst)
{
	int rtn = 0;
	unsigned int i, j, k;

	ISP_LOGV("pm_lsc_full[%d,%d,%d,%d,%d,%p]", src->img_width, src->img_height, src->grid, src->gain_width, src->gain_height, src->input_table_buffer);
	ISP_LOGV("pm_lsc_full->input_table_buffer [%d,%d,%d,%d]", src->input_table_buffer[0], src->input_table_buffer[1], src->input_table_buffer[2], src->input_table_buffer[3]);
	ISP_LOGV("pm_lsc_crop[%d,%d,%d,%d,%d,%d,%d,%p]", dst->img_width, dst->img_height, dst->start_x, dst->start_y, dst->grid, dst->gain_width, dst->gain_height, dst->output_table_buffer);

	// error cases
	if (dst->start_x > src->img_width
		|| dst->start_y > src->img_height
		|| dst->start_x + dst->img_width > src->img_width
		|| dst->start_y + dst->img_height > src->img_height
		|| dst->start_x < 0
		|| dst->start_y < 0
		|| dst->start_x + (dst->gain_width - 2) * (dst->grid * 2) > (src->gain_width - 2) * (src->grid * 2)
		|| dst->start_y + (dst->gain_height - 2) * (dst->grid * 2) > (src->gain_height - 2) * (src->grid * 2)) {

		for (i = 0; i < dst->gain_width * dst->gain_height * 4; i++) {
			dst->output_table_buffer[i] = 1024;
		}
		ISP_LOGE("do LSC_CROP error, return 1X gain table");
		return -1;
	}

	// save input table to channel
	unsigned short *src_ch0 = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src_ch1 = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src_ch2 = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	unsigned short *src_ch3 = (unsigned short *)malloc(src->gain_width * src->gain_height * sizeof(unsigned short));
	for (i = 0; i < src->gain_width * src->gain_height; i++) {
		src_ch0[i] = src->input_table_buffer[4 * i + 0];
		src_ch1[i] = src->input_table_buffer[4 * i + 1];
		src_ch2[i] = src->input_table_buffer[4 * i + 2];
		src_ch3[i] = src->input_table_buffer[4 * i + 3];
	}

	// define crop table parameters
	unsigned int ch_start_x = 0;		// start_x on channel plane;
	unsigned int ch_start_y = 0;		// start_y on channel plane;
	unsigned int crop_table_x = 0;		// dst table coord-x on channel plane
	unsigned int crop_table_y = 0;		// dst table coord-y on channel plane
	int TL_i = 0;						// src table top left index-i
	int TL_j = 0;						// src table top left index-j
	float dx = 0.0;						// distence to left  , where total length normalize to 1
	float dy = 0.0;						// distence to bottem, where total length normalize to 1

	// start to crop table

	ch_start_x = dst->start_x / 2;
	ch_start_y = dst->start_y / 2;
	for (j = 1; j < dst->gain_height - 1; j++) {
		for (i = 1; i < dst->gain_width - 1; i++) {
			crop_table_x = ch_start_x + (i - 1) * dst->grid;
			crop_table_y = ch_start_y + (j - 1) * dst->grid;

			TL_i = (int)(crop_table_x / src->grid) + 1;
			TL_j = (int)(crop_table_y / src->grid) + 1;
			dx = (float)(crop_table_x - (TL_i - 1) * src->grid) / src->grid;
			dy = (float)(TL_j * src->grid - crop_table_y) / src->grid;

			dst->output_table_buffer[(j * dst->gain_width + i) * 4 + 0] = (unsigned short)table_bicubic_interpolation(src_ch0, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
			dst->output_table_buffer[(j * dst->gain_width + i) * 4 + 1] = (unsigned short)table_bicubic_interpolation(src_ch1, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
			dst->output_table_buffer[(j * dst->gain_width + i) * 4 + 2] = (unsigned short)table_bicubic_interpolation(src_ch2, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
			dst->output_table_buffer[(j * dst->gain_width + i) * 4 + 3] = (unsigned short)table_bicubic_interpolation(src_ch3, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
		}
	}

	// generate outer table edge
	unsigned short inner_table_1 = 0;
	unsigned short inner_table_2 = 0;
	unsigned short inner_table_3 = 0;
	for (k = 0; k < 4; k++) {
		for (j = 1; j < dst->gain_height - 1; j++) {
			inner_table_1 = dst->output_table_buffer[(j * dst->gain_width + 1) * 4 + k];
			inner_table_2 = dst->output_table_buffer[(j * dst->gain_width + 2) * 4 + k];
			inner_table_3 = dst->output_table_buffer[(j * dst->gain_width + 3) * 4 + k];
			dst->output_table_buffer[(j * dst->gain_width + 0) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
			inner_table_1 = dst->output_table_buffer[(j * dst->gain_width + (dst->gain_width - 2)) * 4 + k];
			inner_table_2 = dst->output_table_buffer[(j * dst->gain_width + (dst->gain_width - 3)) * 4 + k];
			inner_table_3 = dst->output_table_buffer[(j * dst->gain_width + (dst->gain_width - 4)) * 4 + k];
			dst->output_table_buffer[(j * dst->gain_width + (dst->gain_width - 1)) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
		}
	}

	for (k = 0; k < 4; k++) {
		for (i = 0; i < dst->gain_width; i++) {
			inner_table_1 = dst->output_table_buffer[(1 * dst->gain_width + i) * 4 + k];
			inner_table_2 = dst->output_table_buffer[(2 * dst->gain_width + i) * 4 + k];
			inner_table_3 = dst->output_table_buffer[(3 * dst->gain_width + i) * 4 + k];
			dst->output_table_buffer[(0 * dst->gain_width + i) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
			inner_table_1 = dst->output_table_buffer[((dst->gain_height - 2) * dst->gain_width + i) * 4 + k];
			inner_table_2 = dst->output_table_buffer[((dst->gain_height - 3) * dst->gain_width + i) * 4 + k];
			inner_table_3 = dst->output_table_buffer[((dst->gain_height - 4) * dst->gain_width + i) * 4 + k];
			dst->output_table_buffer[((dst->gain_height - 1) * dst->gain_width + i) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
		}
	}

	// free generated buffer
	std_free(src_ch0);
	std_free(src_ch1);
	std_free(src_ch2);
	std_free(src_ch3);

	return rtn;
}

static cmr_s32 lsc_sprd_ioctrl(void *handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_u32 i;
	cmr_s32 change_mode_rtn = 0;
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;

	if (!handle) {
		ISP_LOGE("fail to check param, param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;
	struct debug_lsc_param* lsc_debug_info_ptr = (struct debug_lsc_param*)cxt->lsc_debug_info_ptr;
	struct tg_alsc_debug_info* ptr_out = NULL;
	struct alsc_flash_info* flash_info = NULL;
	struct alsc_ver_info* alsc_ver_info_out = NULL;
	struct alsc_update_info* alsc_update_info_out = NULL;
	struct alsc_fwstart_info* fwstart_info = NULL;
	struct alsc_fwprocstart_info* fwprocstart_info = NULL;
	struct lsc_last_info* lsc_last_info = (struct lsc_last_info*)cxt->lsc_last_info;
	struct lsc_flash_proc_param* flash_param = (struct lsc_flash_proc_param*)cxt->lsc_flash_proc_param;
	struct post_shading_gain_param *post_param = (struct post_shading_gain_param*)cxt->post_shading_gain_param;
	struct pm_lsc_full* pm_lsc_full = NULL;
	struct pm_lsc_crop* pm_lsc_crop = NULL;
	int binning_crop = 0;
	struct alsc_do_simulation* alsc_do_simulation   = NULL;
	struct lsc_adv_calc_param* lsc_adv_calc_param   = NULL;
	struct lsc_adv_calc_result* lsc_adv_calc_result = NULL;
	cmr_u32 *tmp_buffer_r = NULL;
	cmr_u32 *tmp_buffer_g = NULL;
	cmr_u32 *tmp_buffer_b = NULL;
	cmr_u16 *tmp_buffer = NULL;
	cmr_u16 *pm0_new = NULL;
	cmr_s32 is_gr = cxt->output_gain_pattern;
	cmr_s32 is_gb = 3 - cxt->output_gain_pattern;
	cmr_s32 is_r = cxt->output_gain_pattern - (cxt->output_gain_pattern%2)*2 + 1;
	cmr_s32 is_b = 3 - cxt->output_gain_pattern + (cxt->output_gain_pattern%2)*2 - 1;
	cmr_u32 full_flag = 0;

	// rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, (enum alsc_io_ctrl_cmd)cmd, in, out);  // io cmd to lsc lib

	switch(cmd){

		case ALSC_FW_START:// You have to update two table in FW_START: 1.fwstart_info->lsc_result_address_new, 2.cxt->fwstart_new_scaled_table

			fwstart_info = (struct alsc_fwstart_info*)in;
			ISP_LOGV("[ALSC] FW_START +++++, LSC_SPD_VERSION=%d, frame_count=%d, fwstart_info->camera_id=%d, lsc_id=%d, cxt->can_update_dest=%d, fwstart_info %p", cxt->LSC_SPD_VERSION, cxt->frame_count, fwstart_info->camera_id, cxt->lsc_id, cxt->can_update_dest, fwstart_info);

			// cmd set lsc output
			if(cxt->cmd_alsc_cmd_enable && !cxt->cmd_alsc_bypass){
				alsc_get_cmd(cxt);
				if(cxt->cmd_alsc_table_pattern){
					cmd_set_lsc_output(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->output_gain_pattern);
					ISP_LOGI("[ALSC] FW_START, cmd_set_lsc_output, final output fwstart_info->lsc_result_address_new[%d,%d,%d,%d], lsc_id=%d",
							fwstart_info->lsc_result_address_new[0], fwstart_info->lsc_result_address_new[1], fwstart_info->lsc_result_address_new[2], fwstart_info->lsc_result_address_new[3] , cxt->lsc_id);
					return rtn;
				}
			}

			// cmd set table index
			//if(cxt->cmd_alsc_cmd_enable && !cxt->cmd_alsc_bypass){
				//if(cxt->cmd_alsc_table_index <= 8 && cxt->cmd_alsc_table_index >= 0){
					//memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[cxt->cmd_alsc_table_index], fwstart_info->gain_width_new*fwstart_info->gain_height_new*4*sizeof(cmr_u16));
					//ISP_LOGV("[ALSC]  cmd set table index %d, final output fwstart_info->lsc_result_address_new[%d,%d,%d,%d], lsc_id=%d", cxt->cmd_alsc_table_index,
							//fwstart_info->lsc_result_address_new[0], fwstart_info->lsc_result_address_new[1], fwstart_info->lsc_result_address_new[2], fwstart_info->lsc_result_address_new[3] , cxt->lsc_id);
					//return rtn;
				//}
			//}

			if(cxt->LSC_SPD_VERSION <= 5){
				pm0_new = fwstart_info->lsc_tab_address_new[0];
				if(fwstart_info->gain_width_new == cxt->init_gain_width && fwstart_info->gain_height_new == cxt->init_gain_height && fwstart_info->grid_new == cxt->init_grid)
					full_flag = 1;
				if(fwstart_info->camera_id <= 1){
					lsc_read_last_info(lsc_last_info, fwstart_info->camera_id, full_flag);
					cxt->fw_start_bv = lsc_last_info->bv;
					cxt->fw_start_bv_gain = lsc_last_info->bv_gain;
				}

				ISP_LOGV("[ALSC] FW_START, old tab0 address = %p, lsc_id=%d", cxt->lsc_pm0, cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, new tab0 address = %p, lsc_id=%d", fwstart_info->lsc_tab_address_new[0], cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, old tab0=[%d,%d,%d,%d], lsc_id=%d", cxt->lsc_pm0[0], cxt->lsc_pm0[1], cxt->lsc_pm0[2], cxt->lsc_pm0[3], cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, new tab0=[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_tab_address_new[0][0], fwstart_info->lsc_tab_address_new[0][1], fwstart_info->lsc_tab_address_new[0][2], fwstart_info->lsc_tab_address_new[0][3], cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, new dest address = %p, lsc_id=%d", fwstart_info->lsc_result_address_new, cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, old (%d,%d) grid %d, lsc_id=%d ", cxt->gain_width, cxt->gain_height, cxt->grid, cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, new (%d,%d) grid %d, lsc_id=%d", fwstart_info->gain_width_new, fwstart_info->gain_height_new, fwstart_info->grid_new, cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, preflash_current_lnc_table_address %p, lsc_id=%d", flash_param->preflash_current_lnc_table_address, cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_START, main_flash_from_other_parameter %d, lsc_id=%d", flash_param->main_flash_from_other_parameter, cxt->lsc_id);

			// change to 720p mode
			if(fwstart_info->gain_width_new == 23 && fwstart_info->gain_height_new == 15 && fwstart_info->grid_new == 32){
				ISP_LOGV("[ALSC] FW_START, 720p Mode, Send TL84 table[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_tab_address_new[2][0], fwstart_info->lsc_tab_address_new[2][1], fwstart_info->lsc_tab_address_new[2][2], fwstart_info->lsc_tab_address_new[2][3], cxt->lsc_id);
				memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], fwstart_info->gain_width_new * fwstart_info->gain_height_new *4*sizeof(cmr_u16));                
				change_lsc_pattern(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->gain_pattern, cxt->output_gain_pattern);
				post_shading_gain(fwstart_info->lsc_result_address_new, fwstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern,
								cxt->frame_count, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, cxt->LSC_SPD_VERSION, post_param);
				memcpy(cxt->fwstart_new_scaled_table, fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new * fwstart_info->gain_height_new*4*sizeof(cmr_u16));
			// first frame
			}else if( cxt->frame_count == 0 ){
				ISP_LOGV("[ALSC] FW_START, First Frame Mode, read table from file, lsc_id=%d", cxt->lsc_id);
				// apply lsc_last_info
				if((fwstart_info->camera_id == 0 || fwstart_info->camera_id == 1) && cxt->lsc_id == 1
				&& fwstart_info->gain_width_new  == lsc_last_info->gain_width
				&& fwstart_info->gain_height_new == lsc_last_info->gain_height
				&& lsc_last_info->table_r[0] && lsc_last_info->table_g[0] && lsc_last_info->table_b[0]){
					for(i=0; i<fwstart_info->gain_width_new*fwstart_info->gain_height_new; i++){
						fwstart_info->lsc_result_address_new[4*i + is_r]  = (cmr_u16)(lsc_last_info->table_r[i]);
						fwstart_info->lsc_result_address_new[4*i + is_gr] = (cmr_u16)(lsc_last_info->table_g[i]);
						fwstart_info->lsc_result_address_new[4*i + is_gb] = (cmr_u16)(lsc_last_info->table_g[i]);
						fwstart_info->lsc_result_address_new[4*i + is_b]  = (cmr_u16)(lsc_last_info->table_b[i]);
					}
					ISP_LOGV("[ALSC] FW_START, last_table_rgb[%d,%d,%d], camera_id=%d, lsc_id=%d", lsc_last_info->table_r[0], lsc_last_info->table_g[0], lsc_last_info->table_b[0], fwstart_info->camera_id, cxt->lsc_id);
					ISP_LOGV("[ALSC] FW_START, fwstart_info->lsc_result_address_new[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_result_address_new[0], fwstart_info->lsc_result_address_new[1], fwstart_info->lsc_result_address_new[2], fwstart_info->lsc_result_address_new[3], cxt->lsc_id);
				// apply TL84 table
				}else{
					ISP_LOGV("[ALSC] FW_START, no last info, Send TL84 table[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_tab_address_new[2][0], fwstart_info->lsc_tab_address_new[2][1], fwstart_info->lsc_tab_address_new[2][2], fwstart_info->lsc_tab_address_new[2][3], cxt->lsc_id);
					memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], fwstart_info->gain_width_new * fwstart_info->gain_height_new *4*sizeof(cmr_u16));
					change_lsc_pattern(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->gain_pattern, cxt->output_gain_pattern);
					post_shading_gain(fwstart_info->lsc_result_address_new, fwstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern,
									cxt->frame_count, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, cxt->LSC_SPD_VERSION, post_param);
				}
				ISP_LOGV("[ALSC] FW_START, init lsc table=[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_result_address_new[0], fwstart_info->lsc_result_address_new[1], fwstart_info->lsc_result_address_new[2], fwstart_info->lsc_result_address_new[3], cxt->lsc_id);

				// copy output table to fwstart_new_scaled_table for next alsc calc
				memcpy(cxt->fwstart_new_scaled_table, fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new * fwstart_info->gain_height_new*4*sizeof(cmr_u16));
			// change mode
			}else{
				ISP_LOGV("[ALSC] FW_START, Change Mode, lsc_id=%d", cxt->lsc_id);
				// case for old parameter file the same with new parameter file, then we just copy the output_lsc_table, and calc do nothing.
				if( cxt->lsc_pm0 == fwstart_info->lsc_tab_address_new[0] ){
					ISP_LOGV("[ALSC] FW_START, The same parameter file Mode, COPY fwstop_output_table for output, lsc_id=%d", cxt->lsc_id);
					memcpy(fwstart_info->lsc_result_address_new, cxt->fwstop_output_table, fwstart_info->gain_width_new * fwstart_info->gain_height_new *4*sizeof(cmr_u16));
					memcpy(cxt->fwstart_new_scaled_table, cxt->fwstop_output_table, fwstart_info->gain_width_new * fwstart_info->gain_height_new*4*sizeof(cmr_u16));
				// flash change mode with same param file
				}else if(flash_param->main_flash_from_other_parameter==1 && flash_param->preflash_current_lnc_table_address == fwstart_info->lsc_tab_address_new[0]){
					ISP_LOGV("[ALSC] FW_START, Flash change mode, the same param file, don't need scale, COPY the preflash_current_output_table to fwstop_output_table and lsc_result_address_new, lsc_id=%d", cxt->lsc_id);
					memcpy(cxt->fwstart_new_scaled_table,        flash_param->preflash_current_output_table, fwstart_info->gain_width_new * fwstart_info->gain_height_new *4*sizeof(unsigned short));
					memcpy(fwstart_info->lsc_result_address_new, flash_param->preflash_current_output_table, fwstart_info->gain_width_new * fwstart_info->gain_height_new *4*sizeof(unsigned short));
					flash_param->main_flash_from_other_parameter = 0;
					flash_param->preflash_current_lnc_table_address = NULL;
				// normal change mode & flash change mode, with different param file
				}else{
					ISP_LOGV("[ALSC] FW_START, Normal change mode, prepare to update first tab, lsc_id=%d", cxt->lsc_id);
					change_mode_rtn = fwstart_update_first_tab(cxt, fwstart_info);
				}

					if(change_mode_rtn == -1)
						ISP_LOGV("[ALSC] FW_START, Change Mode Failed, lsc_id=%d", cxt->lsc_id);
				}
			}else{
				if(cxt->init_img_width == fwstart_info->img_width_new && cxt->init_img_height == fwstart_info->img_height_new)
					full_flag = 1;
				if(fwstart_info->camera_id <= 1){
					lsc_read_last_info(lsc_last_info, fwstart_info->camera_id, full_flag);
					cxt->fw_start_bv = lsc_last_info->bv;
					cxt->fw_start_bv_gain = lsc_last_info->bv_gain;
				}

				ISP_LOGE("[ALSC] FW_START, new dest address = %p, lsc_id=%d", fwstart_info->lsc_result_address_new, cxt->lsc_id);
				ISP_LOGE("[ALSC] FW_START, preflash_current_lnc_table_address %p, lsc_id=%d", flash_param->preflash_current_lnc_table_address, cxt->lsc_id);
				ISP_LOGE("[ALSC] FW_START, main_flash_from_other_parameter %d, lsc_id=%d", flash_param->main_flash_from_other_parameter, cxt->lsc_id);
				ISP_LOGE("[ALSC] FW_START, old table size=[%d,%d] grid=%d, lsc_id=%d ", cxt->gain_width, cxt->gain_height, cxt->grid, cxt->lsc_id);

				// do parameter normalization
				if(cxt->LSC_SPD_VERSION >= 6){
					if(cxt->init_img_width / fwstart_info->img_width_new ==  cxt->init_img_height / fwstart_info->img_height_new){
						cxt->cur_lsc_pm_mode = 0;
						cxt->grid = (cmr_u32)(cxt->init_grid / (cxt->init_img_width / fwstart_info->img_width_new));
						cxt->gain_width = cxt->init_gain_width;
						cxt->gain_height = cxt->init_gain_height;
						fwstart_info->grid_new = cxt->grid;
						fwstart_info->gain_width_new = cxt->gain_width;
						fwstart_info->gain_height_new = cxt->gain_height;
						for(i=0; i<8; i++){
							fwstart_info->lsc_tab_address_new[i] = cxt->std_init_lsc_table_param_buffer[i];
							memcpy(cxt->std_lsc_table_param_buffer[i],cxt->std_init_lsc_table_param_buffer[i],cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));
						}
						ISP_LOGI("[ALSC] FW_START parameter normalization, case1 n binning, grid=%d, lsc_id=%d", cxt->grid, cxt->lsc_id);
					}else if(fwstart_info->img_width_new == 1280 && fwstart_info->img_height_new == 720){
						cxt->cur_lsc_pm_mode = 1;
						cxt->grid = 32;
						cxt->gain_width = 23;
						cxt->gain_height = 15;
						fwstart_info->grid_new = cxt->grid;
						fwstart_info->gain_width_new = cxt->gain_width;
						fwstart_info->gain_height_new = cxt->gain_height;
						pm_lsc_full = (struct pm_lsc_full*)malloc(sizeof(struct pm_lsc_full));
						pm_lsc_full->img_width = cxt->init_img_width;
						pm_lsc_full->img_height = cxt->init_img_height;
						pm_lsc_full->grid = cxt->init_grid;
						pm_lsc_full->gain_width = cxt->init_gain_width;
						pm_lsc_full->gain_height = cxt->init_gain_height;
						// Notice, if the crop action from binning size raw, do following action
						binning_crop = 1;
						if(binning_crop){
							pm_lsc_full->img_width /= 2;
							pm_lsc_full->img_height /= 2;
							pm_lsc_full->grid /= 2;
						}

						pm_lsc_crop = (struct pm_lsc_crop*)malloc(sizeof(struct pm_lsc_crop));
						pm_lsc_crop->img_width = fwstart_info->img_width_new;
						pm_lsc_crop->img_height = fwstart_info->img_height_new;
						pm_lsc_crop->start_x = (pm_lsc_full->img_width - pm_lsc_crop->img_width) / 2;      // for crop center case
						pm_lsc_crop->start_y = (pm_lsc_full->img_height - pm_lsc_crop->img_height) / 2;    // for crop center case
						pm_lsc_crop->grid = 32;
						pm_lsc_crop->gain_width = 23;
						pm_lsc_crop->gain_height = 15;
						for(i=0; i<8; i++){
							pm_lsc_full->input_table_buffer = cxt->std_init_lsc_table_param_buffer[i];
							pm_lsc_crop->output_table_buffer = cxt->std_lsc_table_param_buffer[i];
							rtn = pm_lsc_table_crop(pm_lsc_full, pm_lsc_crop);
							fwstart_info->lsc_tab_address_new[i] = cxt->std_lsc_table_param_buffer[i];
						}
						std_free(pm_lsc_full);
						std_free(pm_lsc_crop);
					}else{
						ISP_LOGE("[ALSC] FW_START, lsc do not support img_size=[%d,%d], please check, lsc_id=%d", fwstart_info->img_width_new, fwstart_info->img_height_new, cxt->lsc_id);
					}
				}
				ISP_LOGE("[ALSC] FW_START, new table size=[%d,%d] grid=%d, lsc_id=%d ", cxt->gain_width, cxt->gain_height, cxt->grid, cxt->lsc_id);
				ISP_LOGE("[ALSC] FW_START, pre_lsc_pm_mode=%d, cur_lsc_pm_mode=%d, lsc_id=%d ", cxt->pre_lsc_pm_mode, cxt->cur_lsc_pm_mode, cxt->lsc_id);

				// change to 720p mode
				if(fwstart_info->img_width_new == 1280 && fwstart_info->img_height_new == 720){
					ISP_LOGV("[ALSC] FW_START, 720p Mode, Send TL84 table[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_tab_address_new[2][0], fwstart_info->lsc_tab_address_new[2][1], fwstart_info->lsc_tab_address_new[2][2], fwstart_info->lsc_tab_address_new[2][3], cxt->lsc_id);
					memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], cxt->gain_width*cxt->gain_height *4*sizeof(cmr_u16));
					memcpy(cxt->fwstart_new_scaled_table, fwstart_info->lsc_result_address_new, cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));
				// first frame
				}else if( cxt->frame_count == 0 ){
					ISP_LOGV("[ALSC] FW_START, First Frame Mode, read table from file, lsc_id=%d", cxt->lsc_id);
					// apply lsc_last_info
					if((fwstart_info->camera_id == 0 || fwstart_info->camera_id == 1) && cxt->lsc_id == 1
					&& cxt->gain_width  == lsc_last_info->gain_width
					&& cxt->gain_height == lsc_last_info->gain_height){
						for(i=0; i<cxt->gain_width*cxt->gain_height; i++){
							fwstart_info->lsc_result_address_new[4*i + is_r]  = (cmr_u16)(lsc_last_info->table_r[i]);
							fwstart_info->lsc_result_address_new[4*i + is_gr] = (cmr_u16)(lsc_last_info->table_g[i]);
							fwstart_info->lsc_result_address_new[4*i + is_gb] = (cmr_u16)(lsc_last_info->table_g[i]);
							fwstart_info->lsc_result_address_new[4*i + is_b]  = (cmr_u16)(lsc_last_info->table_b[i]);
						}
						ISP_LOGV("[ALSC] FW_START, last_table_rgb[%d,%d,%d], camera_id=%d, lsc_id=%d", lsc_last_info->table_r[0], lsc_last_info->table_g[0], lsc_last_info->table_b[0], fwstart_info->camera_id, cxt->lsc_id);
					}else{
						ISP_LOGV("[ALSC] FW_START, no last info, Send TL84 table[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_tab_address_new[2][0], fwstart_info->lsc_tab_address_new[2][1], fwstart_info->lsc_tab_address_new[2][2], fwstart_info->lsc_tab_address_new[2][3], cxt->lsc_id);
						memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], cxt->gain_width * cxt->gain_height *4*sizeof(cmr_u16));
						post_shading_gain(fwstart_info->lsc_result_address_new, fwstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern,
										cxt->frame_count, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, cxt->LSC_SPD_VERSION, post_param);
					}

					// copy output table to fwstart_new_scaled_table for next alsc calc
					memcpy(cxt->fwstart_new_scaled_table, fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new * fwstart_info->gain_height_new*4*sizeof(cmr_u16));
					ISP_LOGV("[ALSC] FW_START, init lsc table=[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_result_address_new[0], fwstart_info->lsc_result_address_new[1], fwstart_info->lsc_result_address_new[2], fwstart_info->lsc_result_address_new[3], cxt->lsc_id);
				// change mode
				}else{
					ISP_LOGV("[ALSC] FW_START, Change Mode, lsc_id=%d", cxt->lsc_id);
					// case for the same table size, then we just copy the output_lsc_table, and calc do nothing.
					if( cxt->pre_lsc_pm_mode == cxt->cur_lsc_pm_mode && flash_param->main_flash_from_other_parameter !=1 ){
						ISP_LOGV("[ALSC] FW_START, The same table size mode, COPY fwstop_output_table for output, lsc_id=%d", cxt->lsc_id);
						memcpy(fwstart_info->lsc_result_address_new, cxt->fwstop_output_table, cxt->gain_width * cxt->gain_height *4*sizeof(cmr_u16));
						memcpy(cxt->fwstart_new_scaled_table, cxt->fwstop_output_table, cxt->gain_width * cxt->gain_height*4*sizeof(cmr_u16));
					// flash change mode with same param file
					}else if(cxt->pre_lsc_pm_mode == cxt->cur_lsc_pm_mode && flash_param->main_flash_from_other_parameter ==1){
						ISP_LOGV("[ALSC] FW_START, Flash change mode, the same table size, don't need scale, COPY the preflash_current_output_table to fwstop_output_table and lsc_result_address_new, lsc_id=%d", cxt->lsc_id);
						memcpy(cxt->fwstart_new_scaled_table,        flash_param->preflash_current_output_table, cxt->gain_width * cxt->gain_height *4*sizeof(unsigned short));
						memcpy(fwstart_info->lsc_result_address_new, flash_param->preflash_current_output_table, cxt->gain_width * cxt->gain_height *4*sizeof(unsigned short));
						flash_param->main_flash_from_other_parameter = 0;
						flash_param->preflash_current_lnc_table_address = NULL;
					// non common size mode change back to common size mode
					}else{
						if(cxt->cur_lsc_pm_mode == 0){
							ISP_LOGV("[ALSC] FW_START, non common size mode change back to common size mode, use last_info, lsc_id=%d", cxt->lsc_id);
							if((fwstart_info->camera_id == 0 || fwstart_info->camera_id == 1) && cxt->lsc_id == 1
							&& cxt->gain_width  == lsc_last_info->gain_width
							&& cxt->gain_height == lsc_last_info->gain_height
							&& lsc_last_info->table_r[0] && lsc_last_info->table_g[0] && lsc_last_info->table_b[0]){
								for(i=0; i<cxt->gain_width*cxt->gain_height; i++){
									fwstart_info->lsc_result_address_new[4*i + is_r]  = (cmr_u16)(lsc_last_info->table_r[i]);
									fwstart_info->lsc_result_address_new[4*i + is_gr] = (cmr_u16)(lsc_last_info->table_g[i]);
									fwstart_info->lsc_result_address_new[4*i + is_gb] = (cmr_u16)(lsc_last_info->table_g[i]);
									fwstart_info->lsc_result_address_new[4*i + is_b]  = (cmr_u16)(lsc_last_info->table_b[i]);
								}
								ISP_LOGV("[ALSC] FW_START, last_table_rgb[%d,%d,%d], camera_id=%d, lsc_id=%d", lsc_last_info->table_r[0], lsc_last_info->table_g[0], lsc_last_info->table_b[0], fwstart_info->camera_id, cxt->lsc_id);
							}else{
								ISP_LOGV("[ALSC] FW_START, no last info, Send TL84 table[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_tab_address_new[2][0], fwstart_info->lsc_tab_address_new[2][1], fwstart_info->lsc_tab_address_new[2][2], fwstart_info->lsc_tab_address_new[2][3], cxt->lsc_id);
								memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], cxt->gain_width * cxt->gain_height *4*sizeof(cmr_u16));
								post_shading_gain(fwstart_info->lsc_result_address_new, fwstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern,
											cxt->frame_count, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, cxt->LSC_SPD_VERSION, post_param);
							}

							// copy output table to fwstart_new_scaled_table for next alsc calc
							memcpy(cxt->fwstart_new_scaled_table, fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new * fwstart_info->gain_height_new*4*sizeof(cmr_u16));
							ISP_LOGV("[ALSC] FW_START, init lsc table=[%d,%d,%d,%d], lsc_id=%d", fwstart_info->lsc_result_address_new[0], fwstart_info->lsc_result_address_new[1], fwstart_info->lsc_result_address_new[2], fwstart_info->lsc_result_address_new[3], cxt->lsc_id);
						}
					}
				}
			}

			ISP_LOGV("[ALSC] FW_START -----, frame_count=%d, cxt->can_update_dest=%d, lsc_id=%d", cxt->frame_count, cxt->can_update_dest, cxt->lsc_id);
		break;

		case ALSC_FW_START_END:
			fwstart_info = (struct alsc_fwstart_info*)in;
			ISP_LOGV("[ALSC] FW_START_END +++++, LSC_SPD_VERSION=%d, frame_count=%d, fwstart_info %p, lsc_id=%d", cxt->LSC_SPD_VERSION, cxt->frame_count, fwstart_info, cxt->lsc_id);
			ISP_LOGV("[ALSC] FW_START_END, Ori cxt->fw_start_end=%d, cxt->can_update_dest=%d, lsc_id=%d", cxt->fw_start_end, cxt->can_update_dest, cxt->lsc_id);
			ISP_LOGV("[ALSC] FW_START_END, old tab address = %p, lsc_id=%d", cxt->lsc_pm0, cxt->lsc_id);
			ISP_LOGV("[ALSC] FW_START_END, new tab address = %p, lsc_id=%d", fwstart_info->lsc_tab_address_new[0], cxt->lsc_id);
			ISP_LOGV("[ALSC] FW_START_END, new dest address = %p, lsc_id=%d", fwstart_info->lsc_result_address_new, cxt->lsc_id);

			if(cxt->LSC_SPD_VERSION <= 5){
				//case for old parameter file the same with new parameter file, then we just copy the output_lsc_table, and calc do nothing.
				if( cxt->lsc_pm0 == fwstart_info->lsc_tab_address_new[0]){
					//reset the flag
					cxt->fw_start_end =0;    //set 0, we hope calc don't perform change mode.
					cxt->can_update_dest=1;  //set 1, then the calc can keep update the destination buffer and post gain.
					ISP_LOGV("[ALSC] FW_START_END, The same parameter file, SET lsc_id->fw_start_end=%d, lsc_id->can_update_dest=%d, lsc_id=%d", cxt->fw_start_end, cxt->can_update_dest, cxt->lsc_id);
					ISP_LOGV("[ALSC] FW_START_END, The same parameter file, calc will not perform change mode, lsc_id=%d", cxt->lsc_id);
				}else{
					cxt->fw_start_end =1;// calc will perforem the change mode
					ISP_LOGV("[ALSC] FW_START_END, calc will perform change mode, lsc_id=%d", cxt->lsc_id);
				}
			}else{
				if( cxt->pre_lsc_pm_mode == cxt->cur_lsc_pm_mode){
					//reset the flag
					cxt->fw_start_end =0;    //set 0, we hope calc don't perform change mode.
					cxt->can_update_dest=1;  //set 1, then the calc can keep update the destination buffer and post gain.
					ISP_LOGV("[ALSC] FW_START_END, pre_lsc_pm_mode=cur_lsc_pm_mode=%d, SET lsc_id->fw_start_end=%d, lsc_id->can_update_dest=%d, lsc_id=%d", cxt->cur_lsc_pm_mode, cxt->fw_start_end, cxt->can_update_dest, cxt->lsc_id);
					ISP_LOGV("[ALSC] FW_START_END, The same parameter file, calc will not perform change mode, lsc_id=%d", cxt->lsc_id);
				}else{
					cxt->fw_start_end =1;// calc will perforem the change mode
					ISP_LOGV("[ALSC] FW_START_END, calc will perform change mode, lsc_id=%d", cxt->lsc_id);
				}
			}

			ISP_LOGV("[ALSC] FW_START_END, SET cxt->fw_start_end=%d, cxt->can_update_dest=%d, lsc_id=%d", cxt->fw_start_end, cxt->can_update_dest, cxt->lsc_id);
			ISP_LOGV("[ALSC] FW_START_END -----, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

		case  ALSC_FW_STOP:
			ISP_LOGV("[ALSC] FW_STOP +++++, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);

			//copy the output to last_info
			if( !(cxt->gain_width == 23 && cxt->gain_height == 15 && cxt->grid == 32) && cxt->lsc_id == 1 && (cxt->camera_id == 0 || cxt->camera_id == 1) ){
				for(i=0; i<cxt->gain_width*cxt->gain_height; i++){
					lsc_last_info->table_r[i] = lsc_debug_info_ptr->output_lsc_table[4*i + is_r];
					lsc_last_info->table_g[i] = lsc_debug_info_ptr->output_lsc_table[4*i + is_gr];
					lsc_last_info->table_b[i] = lsc_debug_info_ptr->output_lsc_table[4*i + is_b];
				}
				lsc_last_info->gain_width  = cxt->gain_width;
				lsc_last_info->gain_height = cxt->gain_height;
				ISP_LOGV("[ALSC] FW_STOP, copy_last_table_rgb[%d,%d,%d]", lsc_last_info->table_r[0], lsc_last_info->table_g[0], lsc_last_info->table_b[0]);
			}
			if(cxt->gain_width == cxt->init_gain_width && cxt->gain_height == cxt->init_gain_height && cxt->grid == cxt->init_grid)
				full_flag = 1;
			if(cxt->lsc_id == 1){
				lsc_save_last_info(lsc_last_info, cxt->camera_id, full_flag);
				ISP_LOGV("[ALSC] FW_STOP, save info done, cxt->camera_id=%d, full_flag=%d, lsc_id=%d", cxt->camera_id, full_flag, cxt->lsc_id);
			}

			// let calc not to update the result->dst_gain
			cxt->can_update_dest = 0;
			cxt->alsc_update_flag = 0;
			ISP_LOGV("[ALSC] FW_STOP SET cxt->can_update_dest=%d, cxt->alsc_update_flag=%d, lsc_id=%d", cxt->can_update_dest, cxt->alsc_update_flag, cxt->lsc_id);

			// flash mode
			if(flash_param->is_touch_preflash == 0 || flash_param->is_touch_preflash == 1){
				//copy the output table after preflash
				ISP_LOGV("[ALSC] FW_STOP, flash mode, size(%d, %d), gain_pattern=%d, grid=%d, lsc_id=%d", cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern, cxt->grid, cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_STOP, flash mode, COPY the preflash_guessing_mainflash_output_table table to fwstop_output_table, lsc_id=%d", cxt->lsc_id);
				memcpy(cxt->fwstop_output_table, flash_param->preflash_guessing_mainflash_output_table, cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));
			// normal mode
			}else{
				// copy the last output table
				ISP_LOGV("[ALSC] FW_STOP, normal mode, size(%d, %d), gain_pattern=%d, grid=%d, lsc_id=%d", cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern, cxt->grid, cxt->lsc_id);
				ISP_LOGV("[ALSC] FW_STOP, normal mode, COPY the output_lsc_table table[%d,%d,%d,%d] to fwstop_output_table, lsc_id=%d",
						lsc_debug_info_ptr->output_lsc_table[0], lsc_debug_info_ptr->output_lsc_table[1], lsc_debug_info_ptr->output_lsc_table[2], lsc_debug_info_ptr->output_lsc_table[3], cxt->lsc_id);
				memcpy(cxt->fwstop_output_table, lsc_debug_info_ptr->output_lsc_table, cxt->gain_width*cxt->gain_height*4*sizeof(unsigned short));
			}

			// reset is_touch_preflash
			flash_param->is_touch_preflash =-99;
			ISP_LOGV("[ALSC] FW_STOP SET flash_param->is_touch_preflash=%d, lsc_id=%d", flash_param->is_touch_preflash, cxt->lsc_id);
			ISP_LOGV("[ALSC] FW_STOP, cxt->flash_mode %d, flash_param->preflash_current_lnc_table_address=%p, cxt->lsc_pm0=%p, lsc_id=%d", cxt->flash_mode, flash_param->preflash_current_lnc_table_address, cxt->lsc_pm0, cxt->lsc_id);
			// flash capture end, prepare to back to preview, if the address not equal, means we take flash shot on different parameter file between the preflash one.
			if(cxt->flash_mode==1 && flash_param->preflash_current_lnc_table_address != cxt->lsc_pm0 && flash_param->preflash_current_lnc_table_address != NULL){
				// set main_flash_from_other_parameter will let the next fwstart_update_first_tab to select preflash_current_output_table when back to preview
				flash_param->main_flash_from_other_parameter =1;
			}else{
				flash_param->main_flash_from_other_parameter =0;
			}

			ISP_LOGV("[ALSC] FW_STOP, flash_param->main_flash_from_other_parameter=%d, lsc_id=%d",flash_param->main_flash_from_other_parameter, cxt->lsc_id);
			ISP_LOGV("[ALSC] FW_STOP -----, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

		case  ALSC_CMD_GET_DEBUG_INFO:
			ptr_out = (struct tg_alsc_debug_info*)out;
			ptr_out->log = (unsigned char *)cxt->lsc_debug_info_ptr;
			ptr_out->size = sizeof(struct debug_lsc_param);
		break;

		case LSC_INFO_TO_AWB:
		break;

		case ALSC_GET_VER:
			alsc_ver_info_out = (struct alsc_ver_info*)out;
			alsc_ver_info_out->LSC_SPD_VERSION = cxt->LSC_SPD_VERSION;
		break;

		case ALSC_FLASH_PRE_BEFORE:
			if(cxt->lsc_buffer == NULL || cxt->frame_count == 0){
				ISP_LOGV("[ALSC] FLASH_PRE_BEFORE, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
				return rtn;
			}

			// for quick in
			cxt->pre_flash_mode=1;
			cxt->alg_quick_in=1;
			cxt->quik_in_start_frame=-99;
			ISP_LOGV("[ALSC] FLASH_PRE_BEFORE, setup alg_quick_in=1, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);

			// log bv_before_flash
			if(cxt->flash_done_frame_count > 50){
				cxt->bv_before_flash = 1600;
				cxt->bv_gain_before_flash = 1280;
				for(i=0; i<10; i++){
					cxt->bv_before_flash      = cxt->bv_memory[i] < cxt->bv_before_flash ? cxt->bv_memory[i] : cxt->bv_before_flash;
					cxt->bv_gain_before_flash = cxt->bv_gain_memory[i] > cxt->bv_gain_before_flash ? cxt->bv_gain_memory[i] : cxt->bv_gain_before_flash;
				}
			}

			// for handle touch preflash
			flash_param->pre_flash_before_ae_touch_framecount = flash_param->ae_touch_framecount;
			flash_param->pre_flash_before_framecount = cxt->frame_count;

			if(abs(flash_param->pre_flash_before_ae_touch_framecount - flash_param->pre_flash_before_framecount)<3)
				// the preflash is caused by touch
				flash_param->is_touch_preflash =1;
			else
				// the preflash is caused by normal flash capture
				flash_param->is_touch_preflash =0;

			ISP_LOGV("[ALSC] FLASH_PRE_BEFORE, pre_flash_before_ae_touch_framecount=%d, pre_flash_before_framecount=%d, is_touch_preflash=%d, lsc_id=%d",
					flash_param->pre_flash_before_ae_touch_framecount, flash_param->pre_flash_before_framecount, flash_param->is_touch_preflash, cxt->lsc_id);

			//for change mode flash (capture flashing -> preview ) (with post gain)
			//copy the previous table, to restore back when flash off (with post gain)
			memcpy(flash_param->preflash_current_output_table, lsc_debug_info_ptr->output_lsc_table, cxt->gain_width * cxt->gain_height*4*sizeof(cmr_u16));
			ISP_LOGV("[ALSC] FLASH_PRE_BEFORE, COPY the output_lsc_table table to flash_param->preflash_current_output_table, lsc_id=%d", cxt->lsc_id);

			// copy current DNP table
			memcpy(flash_param->preflash_current_lnc_table, cxt->lsc_pm0, cxt->gain_width * cxt->gain_height*4*sizeof(cmr_u16));
			ISP_LOGV("[ALSC] FLASH_PRE_BEFORE, COPY the lsc_pm0 table to flash_param->preflash_current_lnc_table, lsc_id=%d", cxt->lsc_id);

			// log the current DNP table address (preview mode)
			flash_param->preflash_current_lnc_table_address = cxt->lsc_pm0;
			ISP_LOGV("[ALSC] FLASH_PRE_BEFORE, COPY the lsc_pm0 table addr %p to flash_param->preflash_current_lnc_table_address %p, lsc_id=%d", cxt->lsc_pm0, flash_param->preflash_current_lnc_table_address, cxt->lsc_id);

			flash_param->main_flash_from_other_parameter = 0;
			ISP_LOGV("[ALSC] FLASH_PRE_BEFORE, Set flash_param->main_flash_from_other_parameter %d", flash_param->main_flash_from_other_parameter);

			//for calc to do the inverse (without post gain)
			memcpy(lsc_debug_info_ptr->output_lsc_table, lsc_debug_info_ptr->last_lsc_table, cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));

			//overwrite the dest buffer with one table without post-gain. not really helpful, but give a try.
			memcpy(cxt->lsc_buffer, lsc_debug_info_ptr->last_lsc_table, cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));
			ISP_LOGV("[ALSC] FLASH_PRE_BEFORE END, rewrite the cxt->lsc_buffer without post_gain, lsc_id=%d", cxt->lsc_id);
        break;

		case ALSC_FLASH_PRE_AFTER:
			if(cxt->lsc_buffer == NULL || cxt->frame_count == 0){
				ISP_LOGV("[ALSC} FLASH_PRE_AFTER, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
				return rtn;
			}
			cxt->pre_flash_mode=0;
			ISP_LOGV("[ALSC} FLASH_PRE_AFTER, Not setup alg_quick_in, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);

			flash_info = (struct alsc_flash_info*)in;
			flash_param->captureFlashEnvRatio = flash_info->io_captureFlashEnvRatio;
			flash_param->captureFlash1ofALLRatio = flash_info->io_captureFlash1Ratio;
			ISP_LOGV("[ALSC} FLASH_PRE_AFTER, flash_param->captureFlashEnvRatio=%f, flash_param->captureFlash1ofALLRatio=%f, lsc_id=%d",
					flash_param->captureFlashEnvRatio, flash_param->captureFlash1ofALLRatio, cxt->lsc_id);

			// obtain the mainflash guessing table, use the output from preflash (without post gain)
			memcpy(flash_param->preflash_guessing_mainflash_output_table, lsc_debug_info_ptr->output_lsc_table, cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));
			ISP_LOGV("[ALSC} FLASH_PRE_AFTER, preflash_guessing_mainflash_output_table=[%d,%d,%d,%d], lsc_id=%d",
					flash_param->preflash_guessing_mainflash_output_table[0], flash_param->preflash_guessing_mainflash_output_table[1], flash_param->preflash_guessing_mainflash_output_table[2], flash_param->preflash_guessing_mainflash_output_table[3], cxt->lsc_id);

			// reset
			flash_param->ae_touch_framecount=-99;
			flash_param->pre_flash_before_ae_touch_framecount=-99;
			flash_param->pre_flash_before_framecount=-99;
		break;

		case ALSC_FLASH_MAIN_LIGHTING:
			ISP_LOGV("[ALSC] FLASH_MAIN_LIGHTING, Do nothing, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

		case ALSC_FLASH_PRE_LIGHTING:
			ISP_LOGV("[ALSC] FLASH_PRE_LIGHTING, Do nothing, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

		case ALSC_FLASH_MAIN_BEFORE:
			if(cxt->lsc_buffer == NULL || cxt->frame_count == 0){
				ISP_LOGV("[ALSC] FLASH_MAIN_BEFORE, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
				return rtn;
			}

			// stop calc update the dest buffer
			cxt->can_update_dest=0;
			cxt->alsc_update_flag = 0;

			if(cxt->flash_y_gain[0] == 0){
				// apply the guessing table
				memcpy(cxt->lsc_buffer, flash_param->preflash_guessing_mainflash_output_table, cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));
			}else{
				// apply the guessing table and flash_y_gain
				ISP_LOGV("[ALSC] flash lsc apply flash_y_gain");
				for(i=0; i<cxt->gain_width*cxt->gain_height; i++){
					cxt->lsc_buffer[4*i + 0] = flash_param->preflash_guessing_mainflash_output_table[4*i + 0] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[4*i + 1] = flash_param->preflash_guessing_mainflash_output_table[4*i + 1] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[4*i + 2] = flash_param->preflash_guessing_mainflash_output_table[4*i + 2] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[4*i + 3] = flash_param->preflash_guessing_mainflash_output_table[4*i + 3] * cxt->flash_y_gain[i] / 1024;
				}
			}
			ISP_LOGV("[ALSC] FLASH_MAIN_BEFORE, copy preflash_guessing_mainflash_output_table=[%d,%d,%d,%d] to cxt->lsc_buffer, lsc_id=%d",
					flash_param->preflash_guessing_mainflash_output_table[0], flash_param->preflash_guessing_mainflash_output_table[1], flash_param->preflash_guessing_mainflash_output_table[2], flash_param->preflash_guessing_mainflash_output_table[3], cxt->lsc_id);

			// set flash_mode will quick in
			cxt->flash_mode = 1;
			ISP_LOGV("[ALSC] FLASH_MAIN_BEFORE, setup flash_mode=1 to quick in, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

		case ALSC_FLASH_MAIN_AFTER:
			if(cxt->lsc_buffer == NULL || cxt->frame_count == 0){
				ISP_LOGV("[ALSC] FLASH_MAIN_AFTER, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
				return rtn;
			}

			// set status
			cxt->flash_mode = 0;

			// quick in and set bv_skip_frame
			cxt->alg_quick_in=1;
			cxt->quik_in_start_frame=-99;
			cxt->bv_skip_frame = 15;
			cxt->flash_done_frame_count = 0;

			// allow calc update the dest buffer
			cxt->can_update_dest=1;

			// reset parameter
			flash_param->captureFlashEnvRatio = 0.0;
			flash_param->captureFlash1ofALLRatio = 0.0;
			ISP_LOGV("[ALSC] FLASH_MAIN_AFTER, setup quick in, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

		case ALSC_GET_TOUCH:
			flash_param->ae_touch_framecount = cxt->frame_count;
			ISP_LOGV("[ALSC] GET_TOUCH, FLASH_ , frame_count=%d, ae_touch_framecount =%d, lsc_id=%d", cxt->frame_count, flash_param->ae_touch_framecount, cxt->lsc_id);
		break;

		case SMART_LSC_ALG_LOCK:
			cxt->alg_locked = 1;
		break;

		case SMART_LSC_ALG_UNLOCK:
			cxt->alg_locked = 0;
		break;

		case ALSC_FW_PROC_START: //for sbs feature now
			// ISP_SINGLE 0, ISP_DUAL_NORMAL 1, ISP_DUAL_SBS 2, ISP_DUAL_SWITCH 3
			ISP_LOGV("[ALSC] FW_PROC_START, LSC_SPD_VERSION=%d, is_master=%d, is_multi_mode=%d, frame_count=%d, lsc_id=%d", cxt->LSC_SPD_VERSION, cxt->is_master, cxt->is_multi_mode, cxt->frame_count, cxt->lsc_id);

			fwprocstart_info = (struct alsc_fwprocstart_info*)in;

			if(cxt->LSC_SPD_VERSION <= 5){
				pm0_new = fwprocstart_info->lsc_tab_address_new[0];
				if(fwprocstart_info->gain_width_new == cxt->init_gain_width && fwprocstart_info->gain_height_new == cxt->init_gain_height && fwprocstart_info->grid_new == cxt->init_grid)
					full_flag = 1;
				if(fwprocstart_info->camera_id <= 1){
					lsc_read_last_info(lsc_last_info, fwprocstart_info->camera_id, full_flag);
					cxt->fw_start_bv = lsc_last_info->bv;
					cxt->fw_start_bv_gain = lsc_last_info->bv_gain;
				}
				if(cxt->is_multi_mode == 2 ){
					ISP_LOGV("[ALSC] FW_PROC_START, ISP_DUAL_SBS MODE, camera_id=%d", fwprocstart_info->camera_id);
					ISP_LOGV("[ALSC] FW_PROC_START, old tab address = %p", cxt->lsc_pm0);
					ISP_LOGV("[ALSC] FW_PROC_START, new tab address = %p", fwprocstart_info->lsc_tab_address_new[0]);
					ISP_LOGV("[ALSC] FW_PROC_START, new dest address = %p", fwprocstart_info->lsc_result_address_new);
					ISP_LOGV("[ALSC] FW_PROC_START, old table size(%d,%d) grid %d ", cxt->gain_width, cxt->gain_height, cxt->grid);
					ISP_LOGV("[ALSC] FW_PROC_START, new table size(%d,%d) grid %d", fwprocstart_info->gain_width_new,fwprocstart_info->gain_height_new,fwprocstart_info->grid_new);

				ISP_LOGV("[ALSC] FW_PROC_START, new DNP=[%d,%d,%d,%d]", fwprocstart_info->lsc_tab_address_new[0][0], fwprocstart_info->lsc_tab_address_new[0][1],
																		fwprocstart_info->lsc_tab_address_new[0][2], fwprocstart_info->lsc_tab_address_new[0][3]);
                ISP_LOGV("ALSC_FW_PROC_START, lsc_result_address_new=[%d,%d,%d,%d]", fwprocstart_info->lsc_result_address_new[0], fwprocstart_info->lsc_result_address_new[1],
																					fwprocstart_info->lsc_result_address_new[2], fwprocstart_info->lsc_result_address_new[3]);
				if(cxt->lsc_id == 1){
					ISP_LOGV("[ALSC] FW_PROC_START, lsc_id=%d , frame_count=%d", cxt->lsc_id ,cxt->frame_count);
					// get the master capture parameter info
					proc_start_gain_w = fwprocstart_info->gain_width_new;
					proc_start_gain_h = fwprocstart_info->gain_height_new;
					proc_start_gain_pattern = cxt->output_gain_pattern;
					memcpy(proc_start_param_table, fwprocstart_info->lsc_tab_address_new[0], proc_start_gain_w*proc_start_gain_h*4*sizeof(cmr_u16));
					_table_linear_scaler(lsc_debug_info_ptr->output_lsc_table, cxt->gain_width, cxt->gain_height, proc_start_output_table, proc_start_gain_w, proc_start_gain_h, 0);

					ISP_LOGV("[ALSC] FW_PROC_START, master table size(%d,%d)", proc_start_gain_w, proc_start_gain_h);
					ISP_LOGV("[ALSC] FW_PROC_START, master output table=[%d,%d,%d,%d]", proc_start_output_table[0], proc_start_output_table[1], proc_start_output_table[2], proc_start_output_table[3]);
					ISP_LOGV("[ALSC] FW_PROC_START, master DNP table=[%d,%d,%d,%d]", proc_start_param_table[0], proc_start_param_table[1], proc_start_param_table[2], proc_start_param_table[3]);
				}else if(cxt->lsc_id ==2){
					ISP_LOGV("[ALSC] FW_PROC_START, lsc_id=%d , frame_count=%d", cxt->lsc_id ,cxt->frame_count);
					ISP_LOGV("[ALSC] FW_PROC_START, Get master table size (%d,%d)", proc_start_gain_w, proc_start_gain_h);
					ISP_LOGV("[ALSC] FW_PROC_START, Get master output table=[%d,%d,%d,%d]", proc_start_output_table[0], proc_start_output_table[1], proc_start_output_table[2], proc_start_output_table[3]);
					ISP_LOGV("[ALSC] FW_PROC_START, Get master DNP table=[%d,%d,%d,%d]", proc_start_param_table[0], proc_start_param_table[1], proc_start_param_table[2], proc_start_param_table[3]);

						lnc_master_slave_sync(cxt, fwprocstart_info);
					}
				}else{
					ISP_LOGV("[ALSC] FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do as FW_START.");
					if(fwprocstart_info->camera_id <= 1 && cxt->lsc_id == 1
						&& fwprocstart_info->gain_width_new == lsc_last_info->gain_width
						&& fwprocstart_info->gain_height_new == lsc_last_info->gain_height
						&& lsc_last_info->table_r[0] && lsc_last_info->table_g[0] && lsc_last_info->table_b[0]){
						for(cmr_u32 i=0; i<fwprocstart_info->gain_width_new*fwprocstart_info->gain_height_new; i++){
							fwprocstart_info->lsc_result_address_new[4*i + is_r]  = (cmr_u16)(lsc_last_info->table_r[i]);
							fwprocstart_info->lsc_result_address_new[4*i + is_gr] = (cmr_u16)(lsc_last_info->table_g[i]);
							fwprocstart_info->lsc_result_address_new[4*i + is_gb] = (cmr_u16)(lsc_last_info->table_g[i]);
							fwprocstart_info->lsc_result_address_new[4*i + is_b]  = (cmr_u16)(lsc_last_info->table_b[i]);
						}
						ISP_LOGV("[ALSC] FW_PROC_START, last_table_rgb[%d,%d,%d]", lsc_last_info->table_r[0], lsc_last_info->table_g[0], lsc_last_info->table_b[0]);
					}else{
						memcpy(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_tab_address_new[2], fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new *4*sizeof(unsigned short));
						post_shading_gain(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern,
										cxt->frame_count, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, cxt->LSC_SPD_VERSION, post_param);
						change_lsc_pattern(fwprocstart_info->lsc_result_address_new, fwprocstart_info->gain_width_new, fwprocstart_info->gain_height_new, cxt->gain_pattern, cxt->output_gain_pattern);
					}
					ISP_LOGV("[ALSC] FW_PROC_START, init lsc table=[%d,%d,%d,%d], lsc_id=%d", fwprocstart_info->lsc_result_address_new[0], fwprocstart_info->lsc_result_address_new[1], fwprocstart_info->lsc_result_address_new[2], fwprocstart_info->lsc_result_address_new[3], cxt->lsc_id);

					//output for first tab, ex: fwprocstart_info->lsc_tab_address_new[2] is TW84
					//keep the update for calc to as a source for inversing static data
					memcpy(cxt->fwstart_new_scaled_table, fwprocstart_info->lsc_result_address_new, fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new*4*sizeof(unsigned short));
					ISP_LOGV("[ALSC] FW_PROC_START, send the TL84 to output, lsc_id=%d", cxt->lsc_id);
				}
			}else{
				ISP_LOGV("[ALSC] FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do nothing.");
				full_flag = 1;
				if(fwprocstart_info->camera_id <= 1){
					lsc_read_last_info(lsc_last_info, fwprocstart_info->camera_id, full_flag);
					cxt->fw_start_bv = lsc_last_info->bv;
					cxt->fw_start_bv_gain = lsc_last_info->bv_gain;
				}

				for(i=0; i<8; i++){
					fwprocstart_info->lsc_tab_address_new[i] = cxt->std_init_lsc_table_param_buffer[i];
				}

				if(fwprocstart_info->camera_id <= 1 && cxt->lsc_id == 1
					&& fwprocstart_info->gain_width_new == lsc_last_info->gain_width
					&& fwprocstart_info->gain_height_new == lsc_last_info->gain_height){
					for(cmr_u32 i=0; i<fwprocstart_info->gain_width_new*fwprocstart_info->gain_height_new; i++){
						fwprocstart_info->lsc_result_address_new[4*i + is_r]  = (cmr_u16)(lsc_last_info->table_r[i]);
						fwprocstart_info->lsc_result_address_new[4*i + is_gr] = (cmr_u16)(lsc_last_info->table_g[i]);
						fwprocstart_info->lsc_result_address_new[4*i + is_gb] = (cmr_u16)(lsc_last_info->table_g[i]);
						fwprocstart_info->lsc_result_address_new[4*i + is_b]  = (cmr_u16)(lsc_last_info->table_b[i]);
					}
					ISP_LOGV("[ALSC] FW_PROC_START, last_table_rgb[%d,%d,%d]", lsc_last_info->table_r[0], lsc_last_info->table_g[0], lsc_last_info->table_b[0]);
				}else{
					memcpy(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_tab_address_new[2], fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new *4*sizeof(unsigned short));
					post_shading_gain(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern,
									cxt->frame_count, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, cxt->LSC_SPD_VERSION, post_param);
				}
				ISP_LOGV("[ALSC] FW_PROC_START, init lsc table=[%d,%d,%d,%d], lsc_id=%d", fwprocstart_info->lsc_result_address_new[0], fwprocstart_info->lsc_result_address_new[1], fwprocstart_info->lsc_result_address_new[2], fwprocstart_info->lsc_result_address_new[3], cxt->lsc_id);

				//output for first tab, ex: fwprocstart_info->lsc_tab_address_new[2] is TW84
				//keep the update for calc to as a source for inversing static data
				memcpy(cxt->fwstart_new_scaled_table, fwprocstart_info->lsc_result_address_new, fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new*4*sizeof(unsigned short));
				ISP_LOGV("[ALSC] FW_PROC_START, send the TL84 to output, lsc_id=%d", cxt->lsc_id);
			}
		break;

		case ALSC_FW_PROC_START_END:
			ISP_LOGV("[ALSC] FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do as ALSC FW_START.");
		break;

		case ALSC_GET_UPDATE_INFO:
			alsc_update_info_out = (struct alsc_update_info*)out;
			alsc_update_info_out->alsc_update_flag = cxt->alsc_update_flag;
			alsc_update_info_out->can_update_dest = cxt->can_update_dest;
			alsc_update_info_out->lsc_buffer_addr = cxt->lsc_buffer;
			if(cxt->flash_mode){
				alsc_update_info_out->alsc_update_flag = 1;
				alsc_update_info_out->can_update_dest = 1;
			}
		break;

		case ALSC_DO_SIMULATION:
			cxt->can_update_dest = 0;

			ISP_LOGI("ALSC_DO_SIMULATION, begin ++");
			alsc_do_simulation  = (struct alsc_do_simulation*)in;
			lsc_adv_calc_param  = (struct lsc_adv_calc_param*)malloc(sizeof(struct lsc_adv_calc_param));
			lsc_adv_calc_result = (struct lsc_adv_calc_result*)malloc(sizeof(struct lsc_adv_calc_result));
			tmp_buffer_r = (cmr_u32*)malloc(32*32*4*sizeof(cmr_u32));
			tmp_buffer_g = (cmr_u32*)malloc(32*32*4*sizeof(cmr_u32));
			tmp_buffer_b = (cmr_u32*)malloc(32*32*4*sizeof(cmr_u32));
			tmp_buffer = (cmr_u16*)malloc(32*32*4*sizeof(cmr_u16));
			lsc_adv_calc_param->stat_img.r  = tmp_buffer_r;
			lsc_adv_calc_param->stat_img.gr = tmp_buffer_g;
			lsc_adv_calc_param->stat_img.b  = tmp_buffer_b;
			lsc_adv_calc_result->dst_gain = tmp_buffer;

			ISP_LOGI("ALSC_DO_SIMULATION, stat_r=[%d,%d,%d,%d], stat_g=[%d,%d,%d,%d], stat_b=[%d,%d,%d,%d]",
						alsc_do_simulation->stat_r[0], alsc_do_simulation->stat_r[1], alsc_do_simulation->stat_r[2], alsc_do_simulation->stat_r[3],
						alsc_do_simulation->stat_g[0], alsc_do_simulation->stat_g[1], alsc_do_simulation->stat_g[2], alsc_do_simulation->stat_g[3],
						alsc_do_simulation->stat_b[0], alsc_do_simulation->stat_b[1], alsc_do_simulation->stat_b[2], alsc_do_simulation->stat_b[3]);
			ISP_LOGI("ALSC_DO_SIMULATION, ct=%d, bv=%d, bv_gain=%d", alsc_do_simulation->ct, alsc_do_simulation->bv, alsc_do_simulation->bv_gain);

			memcpy(lsc_debug_info_ptr->last_lsc_table, cxt->std_init_lsc_table_param_buffer[3], cxt->init_gain_width*cxt->init_gain_height*4*sizeof(cmr_u16));
			ISP_LOGI("ALSC_DO_SIMULATION, reset last_lsc_table to D65[%d,%d,%d,%d]",
						lsc_debug_info_ptr->last_lsc_table[0], lsc_debug_info_ptr->last_lsc_table[1], lsc_debug_info_ptr->last_lsc_table[2], lsc_debug_info_ptr->last_lsc_table[3]);

			ISP_LOGI("ALSC_DO_SIMULATION, reset lsc_adv_calc_param parameters");
			lsc_adv_calc_param->img_size.w = cxt->init_img_width;
			lsc_adv_calc_param->img_size.h = cxt->init_img_height;
			lsc_adv_calc_param->gain_width = cxt->init_gain_width;
			lsc_adv_calc_param->gain_height = cxt->init_gain_height;
			lsc_adv_calc_param->grid = cxt->init_grid;
			memcpy(lsc_adv_calc_param->stat_img.r,  alsc_do_simulation->stat_r, 32*32*sizeof(cmr_u32));
			memcpy(lsc_adv_calc_param->stat_img.gr, alsc_do_simulation->stat_g, 32*32*sizeof(cmr_u32));
			memcpy(lsc_adv_calc_param->stat_img.b,  alsc_do_simulation->stat_b, 32*32*sizeof(cmr_u32));
			for(i=0; i<8; i++)
				lsc_adv_calc_param->std_tab_param[i] = cxt->std_init_lsc_table_param_buffer[i];

			ISP_LOGI("ALSC_DO_SIMULATION, call liblsc.so do simulation1");
			rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, lsc_adv_calc_param, lsc_adv_calc_result);
			memcpy(lsc_debug_info_ptr->last_lsc_table, lsc_debug_info_ptr->alsc_lsc_table, cxt->init_gain_width*cxt->init_gain_height*4*sizeof(cmr_u16));
			ISP_LOGI("ALSC_DO_SIMULATION, simulation1, alsc_lsc_table=[%d,%d,%d,%d]",
						lsc_debug_info_ptr->alsc_lsc_table[0], lsc_debug_info_ptr->alsc_lsc_table[1], lsc_debug_info_ptr->alsc_lsc_table[2], lsc_debug_info_ptr->alsc_lsc_table[3]);

			ISP_LOGI("ALSC_DO_SIMULATION, call liblsc.so do simulation2");
			rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, lsc_adv_calc_param, lsc_adv_calc_result);

			ISP_LOGI("ALSC_DO_SIMULATION, post_gain Begin, alsc_lsc_table=[%d,%d,%d,%d]",
						lsc_debug_info_ptr->alsc_lsc_table[0], lsc_debug_info_ptr->alsc_lsc_table[1], lsc_debug_info_ptr->alsc_lsc_table[2], lsc_debug_info_ptr->alsc_lsc_table[3]);
			post_shading_gain(alsc_do_simulation->sim_output_table, lsc_debug_info_ptr->alsc_lsc_table, cxt->init_gain_width, cxt->init_gain_height, cxt->output_gain_pattern,
							cxt->frame_count, alsc_do_simulation->bv, alsc_do_simulation->bv_gain, 0, 0, cxt->LSC_SPD_VERSION, post_param);
			memcpy(lsc_debug_info_ptr->output_lsc_table, alsc_do_simulation->sim_output_table, cxt->init_gain_width*cxt->init_gain_height*4*sizeof(unsigned short));
			ISP_LOGI("ALSC_DO_SIMULATION, post_gain End, sim_output_table=[%d,%d,%d,%d]",
						alsc_do_simulation->sim_output_table[0], alsc_do_simulation->sim_output_table[1], alsc_do_simulation->sim_output_table[2], alsc_do_simulation->sim_output_table[3]);

			std_free(tmp_buffer);
			std_free(tmp_buffer_r);
			std_free(tmp_buffer_g);
			std_free(tmp_buffer_b);
			std_free(lsc_adv_calc_result);
			std_free(lsc_adv_calc_param);
			ISP_LOGI("ALSC_DO_SIMULATION, end ++");

			cxt->can_update_dest = 1;
		break;

		default:
			ISP_LOGV("[ALSC] IO_CTRL, NO cmd=%d", cmd);
		break;
	}

	return rtn;
}

cmr_int lsc_ctrl_init(struct lsc_adv_init_param * input_ptr, cmr_handle * handle_lsc)
{
	cmr_int rtn = ISP_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = NULL;

	cxt_ptr = (struct lsc_ctrl_cxt *)malloc(sizeof(struct lsc_ctrl_cxt));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create lsc ctrl context!");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(struct lsc_ctrl_cxt));

	rtn = _lscctrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = _lscctrl_init_adpt(cxt_ptr, input_ptr);
	if (rtn) {
		goto exit;
	}

  exit:
	if (rtn) {
		if (cxt_ptr) {
			free(cxt_ptr);
		}
	} else {
		*handle_lsc = (cmr_handle) cxt_ptr;
	}
	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int lsc_ctrl_deinit(cmr_handle * handle_lsc)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = *handle_lsc;
	CMR_MSG_INIT(message);

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	message.msg_type = LSCCTRL_EVT_DEINIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);
	if (rtn) {
		ISP_LOGE("failed to send msg to lsc thr %ld", rtn);
		goto exit;
	}

	rtn = _lscctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy lscctrl thread %ld", rtn);
		goto exit;
	}

  exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle_lsc = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param * in_ptr, struct lsc_adv_calc_result * result)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt *)handle_lsc;

	if (!handle_lsc || !in_ptr || !result) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}
	//cxt_ptr->proc_out.dst_gain = result->dst_gain;

	CMR_MSG_INIT(message);
	message.data = malloc(sizeof(struct lsc_adv_calc_param));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}

	memcpy(message.data, (void *)in_ptr, sizeof(struct lsc_adv_calc_param));
	message.alloc_flag = 1;
	message.msg_type = LSCCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to lsc thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto exit;
	}

  exit:
	ISP_LOGV("done %ld", rtn);
	return rtn;
}

cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt *)handle_lsc;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!handle_lsc) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
  exit:
	ISP_LOGV("cmd = %d,done %ld", cmd, rtn);
	return rtn;
}

struct adpt_ops_type lsc_sprd_adpt_ops_ver0 = {
	.adpt_init = lsc_sprd_init,
	.adpt_deinit = lsc_sprd_deinit,
	.adpt_process = lsc_sprd_calculation,
	.adpt_ioctrl = lsc_sprd_ioctrl,
};
