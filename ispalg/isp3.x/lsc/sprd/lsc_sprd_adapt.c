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
#include <cutils/properties.h>
#include "lsc_adv.h"
#include "lsc_sprd_adapt.h"
#include "isp_adpt.h"
#include <dlfcn.h>
#include "isp_mw.h"
#include <utils/Timers.h>
#include <sys/stat.h>
#include <math.h>

#define MAX_CAMERA_ID  8

#define DEFAULT_TAB_INDEX 2

cmr_u32 proc_start_gain_w = 0;	// SBS master gain width
cmr_u32 proc_start_gain_h = 0;	// SBS master gain height
cmr_u32 proc_start_gain_pattern = 0;	// SBS master gain pattern
cmr_u16 proc_start_param_table[32 * 32 * 4] = { 0 };	// SBS master DNP table
cmr_u16 proc_start_output_table[32 * 32 * 4] = { 0 };	// SBS master output table

cmr_u16 *id1_addr = NULL;
cmr_u16 *id2_addr = NULL;

#define LSC_GAIN_LOWER (1<<10)
#define LSC_GAIN_UPPER ((1<<14)-1)
#define LSC_CLIP(x, l, u) ((x) < (l) ? (l) : ((x) > (u) ? (u) : (x)))

char liblsc_path[][20] = {
	"libalsc.so",
	"liblsc_v1.so",
	"liblsc_v2.so",
	"liblsc_v3.so",
	"liblsc_v4.so",
	"liblsc_v5.so",
};

static void lsc_get_prop_cmd(struct lsc_sprd_ctrl_context *cxt)
{
	char prop[256];
	int val = 0;

	property_get("debug.isp.alsc.table.pattern", prop, (char *)"0");
	val = atoi(prop);
	if (0 <= val)
		cxt->cmd_alsc_table_pattern = val;

	property_get("debug.isp.alsc.table.index", prop, (char *)"9");
	val = atoi(prop);
	if (0 <= val)
		cxt->cmd_alsc_table_index = val;

	property_get("debug.isp.alsc.dump.aem", prop, (char *)"0");
	val = atoi(prop);
	if (0 <= val)
		cxt->cmd_alsc_dump_aem = val;

	property_get("debug.isp.alsc.dump.table", prop, (char *)"0");
	val = atoi(prop);
	if (0 <= val)
		cxt->cmd_alsc_dump_table = val;

	property_get("debug.isp.alsc.bypass", prop, (char *)"0");
	val = atoi(prop);
	if (0 <= val)
		cxt->cmd_alsc_bypass = val;

	property_get("debug.isp.alsc.bypass.otp", prop, (char *)"0");
	val = atoi(prop);
	if (0 <= val)
		cxt->cmd_alsc_bypass_otp = val;

	property_get("debug.isp.alsc.dump.otp", prop, "0");
	val = atoi(prop);
	if(0 <= val)
	        cxt->cmd_alsc_dump_otp = val;
}

static void lsc_get_channel_index(cmr_u32 pattern, cmr_u8 * off_gr, cmr_u8 * off_r, cmr_u8 * off_b, cmr_u8 * off_gb)
{
	switch (pattern) {
	case LSC_GAIN_PATTERN_GRBG:	//LSC_GAIN_PATTERN_GRBG:
		*off_gr = 0;
		*off_r = 1;
		*off_b = 2;
		*off_gb = 3;
		break;
	case LSC_GAIN_PATTERN_RGGB:	//LSC_GAIN_PATTERN_RGGB:
		*off_r = 0;
		*off_gr = 1;
		*off_gb = 2;
		*off_b = 3;
		break;
	case LSC_GAIN_PATTERN_BGGR:	//LSC_GAIN_PATTERN_BGGR:
		*off_b = 0;
		*off_gb = 1;
		*off_gr = 2;
		*off_r = 3;
		break;
	case LSC_GAIN_PATTERN_GBRG:	//LSC_GAIN_PATTERN_GBRG:
		*off_gb = 0;
		*off_b = 1;
		*off_r = 2;
		*off_gr = 3;
		break;
	default:
		break;
	}
}

static void lsc_std_free(void *buffer)
{
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
}

static void lsc_cmd_set_output(cmr_u16 * table, cmr_u32 width, cmr_u32 height, cmr_u32 gain_pattern)
{
	char prop[256];
	property_get("debug.isp.alsc.table.pattern", prop, "0");
	int index_r, index_gr, index_gb, index_b;
	unsigned int i, j;

	switch (gain_pattern) {
	case 0:		//  Gr first
		index_gr = 0;
		index_r = 1;
		index_b = 2;
		index_gb = 3;
		break;
	case 1:		//  R first
		index_r = 0;
		index_gr = 1;
		index_gb = 2;
		index_b = 3;
		break;
	case 2:		//  B first
		index_b = 0;
		index_gb = 1;
		index_gr = 2;
		index_r = 3;
		break;
	case 3:		//  Gb first
		index_gb = 0;	//B    //R 3
		index_b = 1;	//Gb   //Gr2
		index_r = 2;	//Gr   //Gb1
		index_gr = 3;	//R    //B 0
		break;
	default:		//  Gb first
		index_gb = 0;
		index_b = 1;
		index_r = 2;
		index_gr = 3;
		break;
	}

	for (i = 0; i < width * height * 4; i++) {
		table[i] = 1024;
	}

	if (strcmp(prop, "1000") == 0) {	// R gain(8x)
		for (i = 0; i < width * height; i++) {
			table[4 * i + index_r] = 8192;
		}
	} else if (strcmp(prop, "0100") == 0) {	// Gr gain(8x)
		for (i = 0; i < width * height; i++) {
			table[4 * i + index_gr] = 8192;
			table[4 * i + index_r] = 8192;
		}
	} else if (strcmp(prop, "0010") == 0) {	// Gb gain(8x)
		for (i = 0; i < width * height; i++) {
			table[4 * i + index_gb] = 8192;
			table[4 * i + index_b] = 8192;
		}
	} else if (strcmp(prop, "0001") == 0) {	// B gain(8x)
		for (i = 0; i < width * height; i++) {
			table[4 * i + index_b] = 8192;
		}
	} else if (strcmp(prop, "1100") == 0) {	// up(8x)bottom(1x)
		for (j = 0; j < height / 2; j++) {
			for (i = 0; i < width; i++) {
				table[4 * (j * width + i) + 0] = 8192;
				table[4 * (j * width + i) + 1] = 8192;
				table[4 * (j * width + i) + 2] = 8192;
				table[4 * (j * width + i) + 3] = 8192;
			}
		}
	} else if (strcmp(prop, "0011") == 0) {	// up(1x)bottom(8x)
		for (j = height / 2; j < height; j++) {
			for (i = 0; i < width; i++) {
				table[4 * (j * width + i) + 0] = 8192;
				table[4 * (j * width + i) + 1] = 8192;
				table[4 * (j * width + i) + 2] = 8192;
				table[4 * (j * width + i) + 3] = 8192;
			}
		}
	}
}

static int lsc_dump_stat_bmp(char *file, cmr_uint img_width, cmr_uint img_height, cmr_uint width, cmr_uint height, cmr_u32 * r, cmr_u32 * g, cmr_u32 * b)
{
	unsigned int i, j;
	unsigned int line_align = (width * 3 + 3) & (~3);
	unsigned char *pbgr = (unsigned char *)malloc(line_align * height);
	memset(pbgr, 0, line_align * height);

	unsigned int channel_width = img_width / 2;
	unsigned int channel_height = img_height / 2;
	unsigned int pix_num_per_block = (unsigned int)(channel_width / width) * (unsigned int)(channel_height / height);
	unsigned int divisor = pix_num_per_block * 4;	// 10 bit to 8 bit

	unsigned char *pbuf = pbgr;
	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			unsigned int r32 = *r++ / divisor;
			unsigned int g32 = *g++ / divisor;
			unsigned int b32 = *b++ / divisor;
			if (r32 > 255)
				r32 = 255;
			if (g32 > 255)
				g32 = 255;
			if (b32 > 255)
				b32 = 255;

			unsigned char r = r32;
			unsigned char g = g32;
			unsigned char b = b32;

			*pbuf++ = b;
			*pbuf++ = g;
			*pbuf++ = r;
		}
		pbuf += line_align - width * 3;
	}

	FILE *fp = fopen(file, "wb");
	if (fp == NULL) {
		free(pbgr);
		return -1;
	}
#ifndef WIN32
	struct BITMAPFILEHEADER {
		unsigned short bfType;
		unsigned int bfSize;
		unsigned short bfReserved1;
		unsigned short bfReserved2;
		unsigned int bfOffBits;
	} __attribute__ ((packed));

	struct BITMAPINFOHEADER {
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
	if (fwrite(&bfheader, 1, sizeof(bfheader), fp) != sizeof(bfheader)) {
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
	if (fwrite(&biheader, 1, sizeof(biheader), fp) != sizeof(biheader)) {
		fclose(fp);
		free(pbgr);
		return -1;
	}

	if (fwrite(pbgr, 1, line_align * height, fp) != line_align * height) {
		fclose(fp);
		free(pbgr);
		return -1;
	}

	fclose(fp);
	free(pbgr);

	return 0;
}

static void lsc_dump_stat(cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 * r, cmr_u32 * g, cmr_u32 * b, cmr_u32 lsc_id, cmr_u32 idx, cmr_u32 type)
{
	int dump_state;
	//static int dump_index[2] = {1, 1};
	char filename[256];

	sprintf(filename, CAMERA_DATA_FILE "/lsc/%05d_AEM%d_cam%d.bmp", idx, type, lsc_id);

	dump_state = lsc_dump_stat_bmp(filename, raw_width, raw_height, 32, 32, r, g, b);
	//dump_index[lsc_id-1] ++;
	ISP_LOGI("dump_state=%d, lsc_id=%d", dump_state, lsc_id);
}

static void lsc_dump(unsigned short *gain, int width, int height, const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fp != NULL) {
		int i, j;
		for (j = 0; j < height; j++) {
			for (i = 0; i < width; i++) {
				fprintf(fp, "%4d,", gain[j * width + i]);
			}
			fprintf(fp, "\r\n");
		}
		fclose(fp);
	}
}

static void lsc_dump_gain(cmr_u16 * table, cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u32 is_planar, cmr_u32 frame_count)
{
	cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
	cmr_u32 chnl_off_gr, chnl_off_r, chnl_off_b, chnl_off_gb;
	unsigned short tmp[1118 * 4];
	unsigned i;

	lsc_get_channel_index(pattern, &off_gr, &off_r, &off_b, &off_gb);

	if (is_planar == 0) {
		// if table is interlace as pattern
		chnl_off_gr = off_gr * width * height;
		chnl_off_r = off_r * width * height;
		chnl_off_b = off_b * width * height;
		chnl_off_gb = off_gb * width * height;
		for (i = 0; i < width * height; i++) {
			tmp[chnl_off_gr + i] = table[4 * i + off_gr];
			tmp[chnl_off_r + i] = table[4 * i + off_r];
			tmp[chnl_off_b + i] = table[4 * i + off_b];
			tmp[chnl_off_gb + i] = table[4 * i + off_gb];
		}
	} else {
		// if table is planar as pattern
		memcpy(tmp, table, width * height * sizeof(unsigned short) * 4);
	}

	if (mkdir(CAMERA_DATA_FILE "/lsc/", 0755) != 0) {
		char filename[256];
		sprintf(filename, CAMERA_DATA_FILE "/lsc/%05d_gr.txt", frame_count);
		lsc_dump(tmp + width * height * off_gr, width, height, filename);

		sprintf(filename, CAMERA_DATA_FILE "/lsc/%05d_r.txt", frame_count);
		lsc_dump(tmp + width * height * off_r, width, height, filename);

		sprintf(filename, CAMERA_DATA_FILE "/lsc/%05d_b.txt", frame_count);
		lsc_dump(tmp + width * height * off_b, width, height, filename);

		sprintf(filename, CAMERA_DATA_FILE "/lsc/%05d_gb.txt", frame_count);
		lsc_dump(tmp + width * height * off_gb, width, height, filename);
	}
}

static int lsc_gain_14bits_to_16bits(unsigned short *src_14bits, unsigned short *dst_16bits, unsigned int size_bytes)
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

static void lsc_get_otp_size_info(cmr_s32 img_width, cmr_s32 img_height, cmr_s32 * lsc_otp_width, cmr_s32 * lsc_otp_height, cmr_s32 grid)
{
	*lsc_otp_width = 0;
	*lsc_otp_height = 0;

	*lsc_otp_width = (int)(img_width / (2 * grid)) + 1;
	*lsc_otp_height = (int)(img_height / (2 * grid)) + 1;

	if (img_width % (2 * grid) != 0) {
		*lsc_otp_width += 1;
	}

	if (img_height % (2 * grid) != 0) {
		*lsc_otp_height += 1;
	}
}

static void lsc_scale_bilinear_short(cmr_u16 * src_buf, int src_width, int src_height, cmr_u16 * dst_buf, int dst_width, int dst_height)
{
	int i, j, x, y;
	float xx, yy;
	int a, b, c, d, tmp;

	for (j = 0; j < dst_height; j++) {
		float sy = (float)(j * src_height) / dst_height;
		if (sy > src_height - 2)
			sy = (float)(src_height - 2);
		y = (short)sy;
		yy = sy - y;

		for (i = 0; i < dst_width; i++) {
			float sx = (float)(i * src_width) / dst_width;
			if (sx > src_width - 2)
				sx = (float)(src_width - 2);
			x = (short)sx;
			xx = sx - x;

			a = src_buf[src_width * y + x];
			b = src_buf[src_width * (y + 1) + x];
			c = src_buf[src_width * y + x + 1];
			d = src_buf[src_width * (y + 1) + x + 1];

			tmp = (short)(a * (1 - xx) * (1 - yy) + b * (1 - xx) * yy + c * xx * (1 - yy) + d * xx * yy + 0.5f);

			dst_buf[dst_width * j + i] = tmp;
		}
	}
}

static int lsc_raw_pattern_to_lsc_pattern(cmr_u32 raw_pattern)
{
	int lsc_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR;

	switch (raw_pattern) {
	case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		lsc_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		lsc_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		lsc_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
		lsc_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

	default:
		break;
	}

	return lsc_pattern;
}

static void lsc_planar_to_channel(cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u16 * ch_r, cmr_u16 * ch_gr, cmr_u16 * ch_gb, cmr_u16 * ch_b, cmr_u16 * rlt_tab)
{
	cmr_u32 i;
	cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
	cmr_u32 chnl_gain_num = width * height;
	cmr_u32 gr_chnl_off, r_chnl_off, b_chnl_off, gb_chnl_off;

	lsc_get_channel_index(pattern, &off_gr, &off_r, &off_b, &off_gb);

	gr_chnl_off = off_gr * chnl_gain_num;
	r_chnl_off = off_r * chnl_gain_num;
	b_chnl_off = off_b * chnl_gain_num;
	gb_chnl_off = off_gb * chnl_gain_num;

	for (i = 0; i < chnl_gain_num; i++) {
		ch_gr[i] = rlt_tab[i + gr_chnl_off];
		ch_r[i] = rlt_tab[i + r_chnl_off];
		ch_b[i] = rlt_tab[i + b_chnl_off];
		ch_gb[i] = rlt_tab[i + gb_chnl_off];
	}
}

static void lsc_interlace_to_channel(cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u16 * ch_r, cmr_u16 * ch_gr, cmr_u16 * ch_gb, cmr_u16 * ch_b, cmr_u16 * rlt_tab)
{
	cmr_u32 i;
	cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
	cmr_u32 chnl_gain_num = width * height;

	lsc_get_channel_index(pattern, &off_gr, &off_r, &off_b, &off_gb);

	for (i = 0; i < chnl_gain_num; i++) {
		ch_gr[i] = rlt_tab[4 * i + off_gr];
		ch_r[i] = rlt_tab[4 * i + off_r];
		ch_b[i] = rlt_tab[4 * i + off_b];
		ch_gb[i] = rlt_tab[4 * i + off_gb];
	}
}

static void lsc_channel_to_planar(cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u16 * r, cmr_u16 * gr, cmr_u16 * gb, cmr_u16 * b, cmr_u16 * tab)
{
	cmr_u32 i;
	cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
	unsigned int chnl_gain_num = width * height;
	cmr_u32 gr_chnl_off, r_chnl_off, b_chnl_off, gb_chnl_off;

	lsc_get_channel_index(pattern, &off_gr, &off_r, &off_b, &off_gb);

	gr_chnl_off = off_gr * chnl_gain_num;
	r_chnl_off = off_r * chnl_gain_num;
	b_chnl_off = off_b * chnl_gain_num;
	gb_chnl_off = off_gb * chnl_gain_num;

	for (i = 0; i < chnl_gain_num; i++) {
		tab[i + gr_chnl_off] = gr[i];
		tab[i + r_chnl_off] = r[i];
		tab[i + b_chnl_off] = b[i];
		tab[i + gb_chnl_off] = gb[i];
	}
}

static void lsc_channel_to_interlace(cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u16 * r, cmr_u16 * gr, cmr_u16 * gb, cmr_u16 * b, cmr_u16 * tab)
{
	cmr_u32 i;
	cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
	unsigned int chnl_gain_num = width * height;

	lsc_get_channel_index(pattern, &off_gr, &off_r, &off_b, &off_gb);

	for (i = 0; i < chnl_gain_num; i++) {
		tab[4 * i + off_gr] = gr[i];
		tab[4 * i + off_r] = r[i];
		tab[4 * i + off_b] = b[i];
		tab[4 * i + off_gb] = gb[i];
	}
}

static void lsc_table_planar2interlace(cmr_u16 * lsc_table, cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u32 new_pattern)
{
	cmr_u16 table_r[32 * 32 * 4] = { 0 };
	cmr_u16 table_gr[32 * 32 * 4] = { 0 };
	cmr_u16 table_gb[32 * 32 * 4] = { 0 };
	cmr_u16 table_b[32 * 32 * 4] = { 0 };

	lsc_planar_to_channel(width, height, pattern, table_r, table_gr, table_gb, table_b, lsc_table);
	lsc_channel_to_interlace(width, height, new_pattern, table_r, table_gr, table_gb, table_b, lsc_table);
}

static void lsc_table_interlace2planar(cmr_u16 * lsc_table, cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u32 new_pattern)
{
	cmr_u16 table_r[32 * 32 * 4] = { 0 };
	cmr_u16 table_gr[32 * 32 * 4] = { 0 };
	cmr_u16 table_gb[32 * 32 * 4] = { 0 };
	cmr_u16 table_b[32 * 32 * 4] = { 0 };

	lsc_interlace_to_channel(width, height, pattern, table_r, table_gr, table_gb, table_b, lsc_table);
	lsc_channel_to_planar(width, height, new_pattern, table_r, table_gr, table_gb, table_b, lsc_table);
}

static void lsc_interlace_change_pattern(cmr_u16 * lsc_table, cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u32 new_pattern)
{
	cmr_u16 table_r[32 * 32 * 4] = { 0 };
	cmr_u16 table_gr[32 * 32 * 4] = { 0 };
	cmr_u16 table_gb[32 * 32 * 4] = { 0 };
	cmr_u16 table_b[32 * 32 * 4] = { 0 };

	lsc_interlace_to_channel(width, height, pattern, table_r, table_gr, table_gb, table_b, lsc_table);
	lsc_channel_to_interlace(width, height, new_pattern, table_r, table_gr, table_gb, table_b, lsc_table);
}

static void lsc_table_linear_scaler(cmr_u16 * src_tab, cmr_u32 src_w, cmr_u32 src_h, cmr_u16 * dst_tab, cmr_u32 dst_w, cmr_u32 dst_h, cmr_u32 is_plane)
{
	cmr_u32 i, j;
	cmr_u16 pre_r[32 * 32] = { 0 };
	cmr_u16 pre_gr[32 * 32] = { 0 };
	cmr_u16 pre_gb[32 * 32] = { 0 };
	cmr_u16 pre_b[32 * 32] = { 0 };
	cmr_u16 new_r[32 * 32] = { 0 };
	cmr_u16 new_gr[32 * 32] = { 0 };
	cmr_u16 new_gb[32 * 32] = { 0 };
	cmr_u16 new_b[32 * 32] = { 0 };
	cmr_u16 out_r[32 * 32] = { 0 };
	cmr_u16 out_gr[32 * 32] = { 0 };
	cmr_u16 out_gb[32 * 32] = { 0 };
	cmr_u16 out_b[32 * 32] = { 0 };
	cmr_u16 *ch_r;
	cmr_u16 *ch_gr;
	cmr_u16 *ch_gb;
	cmr_u16 *ch_b;

	ISP_LOGV("src_w=%d, src_h=%d, dst_w=%d, dst_h=%d, plane_flag=%d", src_w, src_h, dst_w, dst_h, is_plane);

	if (src_w == dst_w && src_h == dst_h) {
		memcpy(dst_tab, src_tab, src_w * src_h * 4 * sizeof(cmr_u16));
	} else {
		//scale pre table to new gain size
		ch_r = src_tab;
		ch_gr = src_tab + src_w * src_h;
		ch_gb = src_tab + src_w * src_h * 2;
		ch_b = src_tab + src_w * src_h * 3;

		if (is_plane == 1) {
			memcpy(out_r, ch_r, src_w * src_h * sizeof(cmr_u16));
			memcpy(out_gr, ch_gr, src_w * src_h * sizeof(cmr_u16));
			memcpy(out_gb, ch_gb, src_w * src_h * sizeof(cmr_u16));
			memcpy(out_b, ch_b, src_w * src_h * sizeof(cmr_u16));
		} else {
			lsc_interlace_to_channel(src_w, src_h, 3, out_r, out_gr, out_gb, out_b, src_tab);
		}
		ISP_LOGV("lsc_table_linear_scaler, src_rggb[%d,%d,%d,%d]", out_r[0], out_gr[0], out_gb[0], out_b[0]);

		// get contain from pre_tab
		for (j = 0; j < src_h - 2; j++) {
			for (i = 0; i < src_w - 2; i++) {
				pre_r[j * (src_w - 2) + i] = out_r[(j + 1) * src_w + (i + 1)];
				pre_gr[j * (src_w - 2) + i] = out_gr[(j + 1) * src_w + (i + 1)];
				pre_gb[j * (src_w - 2) + i] = out_gb[(j + 1) * src_w + (i + 1)];
				pre_b[j * (src_w - 2) + i] = out_b[(j + 1) * src_w + (i + 1)];
			}
		}

		// scale pre_contain to new_contain
		lsc_scale_bilinear_short(pre_r, src_w - 2, src_h - 2, new_r, dst_w - 2, dst_h - 2);
		lsc_scale_bilinear_short(pre_gr, src_w - 2, src_h - 2, new_gr, dst_w - 2, dst_h - 2);
		lsc_scale_bilinear_short(pre_gb, src_w - 2, src_h - 2, new_gb, dst_w - 2, dst_h - 2);
		lsc_scale_bilinear_short(pre_b, src_w - 2, src_h - 2, new_b, dst_w - 2, dst_h - 2);

		// set contain to output tab
		for (j = 0; j < dst_h - 2; j++) {
			for (i = 0; i < dst_w - 2; i++) {
				out_r[(j + 1) * dst_w + (i + 1)] = new_r[j * (dst_w - 2) + i];
				out_gr[(j + 1) * dst_w + (i + 1)] = new_gr[j * (dst_w - 2) + i];
				out_gb[(j + 1) * dst_w + (i + 1)] = new_gb[j * (dst_w - 2) + i];
				out_b[(j + 1) * dst_w + (i + 1)] = new_b[j * (dst_w - 2) + i];
			}
		}

		// set top and bottom edge
		for (i = 1; i < dst_w - 1; i++) {
			out_r[0 * dst_w + i] = 3 * out_r[1 * dst_w + i] - 3 * out_r[2 * dst_w + i] + out_r[3 * dst_w + i];
			out_gr[0 * dst_w + i] = 3 * out_gr[1 * dst_w + i] - 3 * out_gr[2 * dst_w + i] + out_gr[3 * dst_w + i];
			out_gb[0 * dst_w + i] = 3 * out_gb[1 * dst_w + i] - 3 * out_gb[2 * dst_w + i] + out_gb[3 * dst_w + i];
			out_b[0 * dst_w + i] = 3 * out_b[1 * dst_w + i] - 3 * out_b[2 * dst_w + i] + out_b[3 * dst_w + i];
			out_r[(dst_h - 1) * dst_w + i] = 3 * out_r[(dst_h - 2) * dst_w + i] - 3 * out_r[(dst_h - 3) * dst_w + i] + out_r[(dst_h - 4) * dst_w + i];
			out_gr[(dst_h - 1) * dst_w + i] = 3 * out_gr[(dst_h - 2) * dst_w + i] - 3 * out_gr[(dst_h - 3) * dst_w + i] + out_gr[(dst_h - 4) * dst_w + i];
			out_gb[(dst_h - 1) * dst_w + i] = 3 * out_gb[(dst_h - 2) * dst_w + i] - 3 * out_gb[(dst_h - 3) * dst_w + i] + out_gb[(dst_h - 4) * dst_w + i];
			out_b[(dst_h - 1) * dst_w + i] = 3 * out_b[(dst_h - 2) * dst_w + i] - 3 * out_b[(dst_h - 3) * dst_w + i] + out_b[(dst_h - 4) * dst_w + i];
		}

		// set left and right edge
		for (j = 0; j < dst_h; j++) {
			out_r[j * dst_w + 0] = 3 * out_r[j * dst_w + 1] - 3 * out_r[j * dst_w + 2] + out_r[j * dst_w + 3];
			out_gr[j * dst_w + 0] = 3 * out_gr[j * dst_w + 1] - 3 * out_gr[j * dst_w + 2] + out_gr[j * dst_w + 3];
			out_gb[j * dst_w + 0] = 3 * out_gb[j * dst_w + 1] - 3 * out_gb[j * dst_w + 2] + out_gb[j * dst_w + 3];
			out_b[j * dst_w + 0] = 3 * out_b[j * dst_w + 1] - 3 * out_b[j * dst_w + 2] + out_b[j * dst_w + 3];
			out_r[j * dst_w + (dst_w - 1)] = 3 * out_r[j * dst_w + (dst_w - 2)] - 3 * out_r[j * dst_w + (dst_w - 3)] + out_r[j * dst_w + (dst_w - 4)];
			out_gr[j * dst_w + (dst_w - 1)] = 3 * out_gr[j * dst_w + (dst_w - 2)] - 3 * out_gr[j * dst_w + (dst_w - 3)] + out_gr[j * dst_w + (dst_w - 4)];
			out_gb[j * dst_w + (dst_w - 1)] = 3 * out_gb[j * dst_w + (dst_w - 2)] - 3 * out_gb[j * dst_w + (dst_w - 3)] + out_gb[j * dst_w + (dst_w - 4)];
			out_b[j * dst_w + (dst_w - 1)] = 3 * out_b[j * dst_w + (dst_w - 2)] - 3 * out_b[j * dst_w + (dst_w - 3)] + out_b[j * dst_w + (dst_w - 4)];
		}

		// merge color channels to table
		if (is_plane == 1) {
			ch_r = dst_tab;
			ch_gr = dst_tab + dst_w * dst_h;
			ch_gb = dst_tab + dst_w * dst_h * 2;
			ch_b = dst_tab + dst_w * dst_h * 3;
			memcpy(ch_r, out_r, dst_w * dst_h * sizeof(cmr_u16));
			memcpy(ch_gr, out_gr, dst_w * dst_h * sizeof(cmr_u16));
			memcpy(ch_gb, out_gb, dst_w * dst_h * sizeof(cmr_u16));
			memcpy(ch_b, out_b, dst_w * dst_h * sizeof(cmr_u16));
		} else {
			lsc_channel_to_interlace(dst_w, dst_h, 3, out_r, out_gr, out_gb, out_b, dst_tab);
		}
	}
}

static cmr_s32 lsc_master_slave_sync(struct lsc_sprd_ctrl_context *cxt, struct alsc_fwprocstart_info *fwprocstart_info)
{
	cmr_u32 i;
	cmr_u32 pre_width = proc_start_gain_w;
	cmr_u32 pre_height = proc_start_gain_h;
	cmr_u32 pre_pattern = proc_start_gain_pattern;
	cmr_u16 lsc_pre_reslut_table[32 * 32 * 4] = { 0 };
	cmr_u16 lsc_pre_table[32 * 32 * 4] = { 0 };
	cmr_u32 new_width = fwprocstart_info->gain_width_new;
	cmr_u32 new_height = fwprocstart_info->gain_height_new;;
	cmr_u32 new_pattern = 3;	//for initial value
	cmr_u16 *lsc_result_address_new = fwprocstart_info->lsc_result_address_new;	// slave output table buffer
	cmr_u16 new_table[32 * 32 * 4] = { 0 };	// slave DNP param table
	cmr_u16 output_tab[32 * 32 * 4] = { 0 };
	cmr_u16 out_r_tab[32 * 32] = { 0 };
	cmr_u16 out_gr_tab[32 * 32] = { 0 };
	cmr_u16 out_gb_tab[32 * 32] = { 0 };
	cmr_u16 out_b_tab[32 * 32] = { 0 };

	for (i = 0; i < pre_width * pre_height * 4; i++) {
		lsc_pre_reslut_table[i] = proc_start_output_table[i];	// master output table
		lsc_pre_table[i] = proc_start_param_table[i];	// master DNP param table
	}

	// slave DNP param table
	memcpy(new_table, fwprocstart_info->lsc_tab_address_new[0], new_width * new_height * 4 * sizeof(unsigned short));
	ISP_LOGV("slave size[%d,%d], slave_DNP[%d,%d,%d,%d]", new_width, new_height, new_table[0], new_table[1], new_table[2], new_table[3]);

	new_pattern = lsc_raw_pattern_to_lsc_pattern(fwprocstart_info->image_pattern_new);

	// scale master dnp table to slave size
	lsc_table_linear_scaler(lsc_pre_table, pre_width, pre_height, output_tab, new_width, new_height, cxt->is_planar);
	lsc_interlace_to_channel(new_width, new_height, pre_pattern, out_r_tab, out_gr_tab, out_gb_tab, out_b_tab, output_tab);

	// get slave dnp table
	cmr_u16 output_r_new[32 * 32] = { 0 };
	cmr_u16 output_gr_new[32 * 32] = { 0 };
	cmr_u16 output_gb_new[32 * 32] = { 0 };
	cmr_u16 output_b_new[32 * 32] = { 0 };

	lsc_interlace_to_channel(new_width, new_height, new_pattern, output_r_new, output_gr_new, output_gb_new, output_b_new, new_table);

	//get level weight matrix
	float lsc_new_weight_tab_gb[32 * 32] = { 0 };
	float lsc_new_weight_tab_b[32 * 32] = { 0 };
	float lsc_new_weight_tab_r[32 * 32] = { 0 };
	float lsc_new_weight_tab_gr[32 * 32] = { 0 };
	float rate_gb = 0.0;
	float rate_b = 0.0;
	float rate_r = 0.0;
	float rate_gr = 0.0;

	for (i = 0; i < new_width * new_height; i++) {
		rate_gr = 0.0;
		rate_gb = 0.0;
		rate_b = 0.0;
		rate_r = 0.0;
		//gr
		if (out_gr_tab[i] == 0 || out_gr_tab[i] == 1024) {
			rate_gr = 1;
		} else {
			rate_gr = (float)output_gr_new[i] / (float)out_gr_tab[i];
		}
		lsc_new_weight_tab_gr[i] = rate_gr;
		//gb
		if (out_gb_tab[i] == 0 || out_gb_tab[i] == 1024) {
			rate_gb = 1;
		} else {
			rate_gb = (float)output_gb_new[i] / (float)out_gb_tab[i];
		}
		lsc_new_weight_tab_gb[i] = rate_gb;
		//r
		if (out_r_tab[i] == 0 || out_r_tab[i] == 1024) {
			rate_r = 1;
		} else {
			rate_r = (float)output_r_new[i] / (float)out_r_tab[i];
		}
		lsc_new_weight_tab_r[i] = rate_r;
		//b
		if (out_b_tab[i] == 0 || out_b_tab[i] == 1024) {
			rate_b = 1;
		} else {
			rate_b = (float)output_b_new[i] / (float)out_b_tab[i];
		}
		lsc_new_weight_tab_b[i] = rate_b;
	}

	// scale master output table to slave size
	cmr_u16 output_r[32 * 32] = { 0 };
	cmr_u16 output_gr[32 * 32] = { 0 };
	cmr_u16 output_gb[32 * 32] = { 0 };
	cmr_u16 output_b[32 * 32] = { 0 };
	lsc_table_linear_scaler(lsc_pre_reslut_table, pre_width, pre_height, lsc_result_address_new, new_width, new_height, cxt->is_planar);
	lsc_interlace_to_channel(new_width, new_height, pre_pattern, output_r, output_gr, output_gb, output_b, lsc_result_address_new);

	//lsc_change_pattern
	if (cxt->change_pattern_flag) {
		ISP_LOGV("slave change lsc pattern, gain_pattern=%d, output_gain_pattern=%d", new_pattern, cxt->output_gain_pattern);
		new_pattern = cxt->output_gain_pattern;
	}
	// apply weight and send output to lsc_result_address_new
	cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
	lsc_get_channel_index(new_pattern, &off_gr, &off_r, &off_b, &off_gb);

	if (cxt->is_planar == 0) {
		for (i = 0; i < new_width * new_height; i++) {
			lsc_result_address_new[4 * i + off_gr] = output_gr[i] * lsc_new_weight_tab_gr[i];
			lsc_result_address_new[4 * i + off_r] = output_r[i] * lsc_new_weight_tab_r[i];
			lsc_result_address_new[4 * i + off_b] = output_b[i] * lsc_new_weight_tab_b[i];
			lsc_result_address_new[4 * i + off_gb] = output_gb[i] * lsc_new_weight_tab_gb[i];
		}
	} else {
		for (i = 0; i < new_width * new_height; i++) {
			lsc_result_address_new[i + off_gr * new_width * new_height] = output_gr[i] * lsc_new_weight_tab_gr[i];
			lsc_result_address_new[i + off_r * new_width * new_height] = output_r[i] * lsc_new_weight_tab_r[i];
			lsc_result_address_new[i + off_b * new_width * new_height] = output_b[i] * lsc_new_weight_tab_b[i];
			lsc_result_address_new[i + off_gb * new_width * new_height] = output_gb[i] * lsc_new_weight_tab_gb[i];
		}
	}

	//cliping for table max value 16383, min 1024
	for (i = 0; i < new_width * new_height * 4; i++) {
		if (lsc_result_address_new[i] > 16383)
			lsc_result_address_new[i] = 16383;
		if (lsc_result_address_new[i] < 1024)
			lsc_result_address_new[i] = 1024;
	}
	ISP_LOGV("slave output table address %p,  output table=[%d,%d,%d,%d]", lsc_result_address_new,
		 lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);

	//keep the update for calc to as a source for inversing static data
	memcpy(cxt->fwstart_new_scaled_table, lsc_result_address_new, new_width * new_height * 4 * sizeof(unsigned short));

	return 0;
}

static cmr_s32 lsc_calculate_otplen_chn(cmr_u32 full_width, cmr_u32 full_height, cmr_u32 lsc_grid)
{
	cmr_u32 half_width, half_height, lsc_width, lsc_height;
	cmr_s32 otp_len_chn;
	half_width = full_width / 2;
	half_height = full_height / 2;
	lsc_width = ((half_width % lsc_grid) > 0) ? (half_width / lsc_grid + 2) : (half_width / lsc_grid + 1);
	lsc_height = ((half_height % lsc_grid) > 0) ? (half_height / lsc_grid + 2) : (half_height / lsc_grid + 1);
	otp_len_chn = ((lsc_width * lsc_height) * 14 % 8) ? (((lsc_width * lsc_height) * 14 / 8) + 1) : ((lsc_width * lsc_height) * 14 / 8);
	otp_len_chn = (otp_len_chn % 2) ? (otp_len_chn + 1) : (otp_len_chn);
	return otp_len_chn;
}

static cmr_int lsc_parser_otp(struct lsc_adv_init_param *lsc_param, struct lsc_sprd_ctrl_context *cxt)
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
	cmr_u8 *oc_otp_data;
	cmr_u16 oc_otp_len;
	cmr_u8 *otp_data_ptr;
	cmr_u32 otp_data_len = 0;
	cmr_u32 resolution = 0;
	struct sensor_otp_section_info *lsc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *oc_otp_info_ptr = NULL;
	struct sensor_otp_section_info *module_info_ptr = NULL;
	cmr_u32 otp_raw_img_width = full_img_width;
	cmr_u32 otp_raw_img_height = full_img_height;

	lsc_get_otp_size_info(otp_raw_img_width, otp_raw_img_height, &lsc_otp_width, &lsc_otp_height, lsc_otp_grid);

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

				if (lsc_otp_info != NULL && oc_otp_info != NULL) {
					lsc_otp_addr = (cmr_u8 *) lsc_otp_info->data_addr;
					lsc_otp_len = lsc_otp_info->data_size;
					lsc_otp_len_chn = lsc_otp_len / 4;
					lsc_otp_chn_gain_num = lsc_otp_len_chn * 8 / compressed_lens_bits;
					oc_otp_data = (cmr_u8 *) oc_otp_info->data_addr;
					oc_otp_len = oc_otp_info->data_size;
				} else {
					ISP_LOGE("lsc lsc_otp_info = %p, oc_otp_info = %p. Parser fail !", lsc_otp_info, oc_otp_info);
					goto EXIT;
				}
			} else {
				ISP_LOGE("lsc otp_info_lsc_ptr = %p, otp_info_optical_center_ptr = %p. Parser fail !", lsc_otp_info_ptr, oc_otp_info_ptr);
				goto EXIT;
			}
		} else if (module_info[4] == 1 && (module_info[5] == 0 || module_info[5] == 1) && module_info[0] == 0x53
				&& module_info[1] == 0x50 && module_info[2] == 0x52 && module_info[3] == 0x44) {
			ISP_LOGV("lsc otp map v1.0 or v1.1");
			if (NULL != lsc_otp_info_ptr) {
				otp_data_ptr = lsc_otp_info_ptr->rdm_info.data_addr;
				otp_data_len = lsc_otp_info_ptr->rdm_info.data_size;

				if (otp_data_ptr != NULL && otp_data_len != 0) {
					lsc_otp_addr = otp_data_ptr + 1 + 16 + 5;
					lsc_otp_len = otp_data_len - 1 - 16 - 5;
				} else {
					ISP_LOGE("lsc otp_data_ptr = %p, otp_data_len = %d. Parser fail !", otp_data_ptr, otp_data_len);
					goto EXIT;
				}

				otp_raw_img_width   = (cmr_u32)((otp_data_ptr[18] << 8) | otp_data_ptr[17]);
				otp_raw_img_height  = (cmr_u32)((otp_data_ptr[20] << 8) | otp_data_ptr[19]);
				lsc_otp_grid = (cmr_u32)otp_data_ptr[21];

				lsc_get_otp_size_info(otp_raw_img_width, otp_raw_img_height, &lsc_otp_width, &lsc_otp_height, lsc_otp_grid);

				gain_w = lsc_otp_width;
				gain_h = lsc_otp_height;
				full_img_width = otp_raw_img_width;
				full_img_height = otp_raw_img_height;

				resolution = (full_img_width * full_img_height + 500000) / 1000000;
				switch (resolution) {
				case 32:
				case 16:
				case 13:
				case 12:
				case 8:
				case 5:
				case 4:
				case 2:
					lsc_otp_len_chn = lsc_calculate_otplen_chn(full_img_width, full_img_height, lsc_otp_grid);
					break;
				default:
					ISP_LOGW("not support resolution now , may be add later");
					lsc_otp_len_chn = 0;
					break;
				}
				ISP_LOGV("resolution:%d , lsc otp len chn is:%d", resolution, lsc_otp_len_chn);
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

	ISP_LOGV("full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d", full_img_width, full_img_height, lsc_otp_grid);
	ISP_LOGV("before, lsc_otp_chn_gain_num=%d", lsc_otp_chn_gain_num);

	if (lsc_otp_chn_gain_num < 100 || lsc_otp_grid < 32 || lsc_otp_grid > 256 || full_img_width < 800 || full_img_height < 600) {
		ISP_LOGE("sensor setting error, lsc_otp_len=%d, full_img_width=%d, full_img_height=%d, lsc_otp_grid=%d",
			 lsc_otp_len, full_img_width, full_img_height, lsc_otp_grid);
		goto EXIT;
	}

	if (lsc_otp_chn_gain_num != lsc_otp_width * lsc_otp_height) {
		lsc_otp_chn_gain_num = lsc_otp_width * lsc_otp_height;
		ISP_LOGD("sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d",
			 lsc_otp_len, lsc_otp_chn_gain_num, lsc_otp_width, lsc_otp_height, lsc_otp_grid);
		//goto EXIT;
	}

	cmr_s32 lsc_ori_chn_len = lsc_otp_chn_gain_num * sizeof(cmr_u16);

	if ((lsc_otp_addr != NULL) && (lsc_otp_len != 0)) {

		cmr_u16 *lsc_16_bits = (cmr_u16 *) malloc(lsc_ori_chn_len * 4);
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + lsc_otp_len_chn * 0), lsc_16_bits + lsc_otp_chn_gain_num * 0, lsc_otp_chn_gain_num);
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + lsc_otp_len_chn * 1), lsc_16_bits + lsc_otp_chn_gain_num * 1, lsc_otp_chn_gain_num);
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + lsc_otp_len_chn * 2), lsc_16_bits + lsc_otp_chn_gain_num * 2, lsc_otp_chn_gain_num);
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + lsc_otp_len_chn * 3), lsc_16_bits + lsc_otp_chn_gain_num * 3, lsc_otp_chn_gain_num);

		lsc_param->lsc_otp_raw_width = otp_raw_img_width;
		lsc_param->lsc_otp_raw_height = otp_raw_img_height;
		lsc_param->lsc_otp_table_width = lsc_otp_width;
		lsc_param->lsc_otp_table_height = lsc_otp_height;
		lsc_param->lsc_otp_grid = lsc_otp_grid;
		lsc_param->lsc_otp_table_addr = lsc_16_bits;
		lsc_param->lsc_otp_table_en = 1;

		if (cxt->cmd_alsc_dump_otp == 1) {
			char filename[256];
			sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_otp_dump_r.txt", lsc_param->camera_id);
			lsc_dump(lsc_16_bits + lsc_otp_width * lsc_otp_height * 0, lsc_otp_width, lsc_otp_height, filename);
			sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_otp_dump_gr.txt", lsc_param->camera_id);
			lsc_dump(lsc_16_bits + lsc_otp_width * lsc_otp_height * 1, lsc_otp_width, lsc_otp_height, filename);
			sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_otp_dump_gb.txt", lsc_param->camera_id);
			lsc_dump(lsc_16_bits + lsc_otp_width * lsc_otp_height * 2, lsc_otp_width, lsc_otp_height, filename);
			sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_otp_dump_b.txt", lsc_param->camera_id);
			lsc_dump(lsc_16_bits + lsc_otp_width * lsc_otp_height * 3, lsc_otp_width, lsc_otp_height, filename);
		}

		ISP_LOGV("lsc_otp_width=%d, lsc_otp_height=%d, gain_w=%d, gain_h=%d, lsc_otp_grid=%d", lsc_otp_width, lsc_otp_height, gain_w, gain_h, lsc_otp_grid);
		ISP_LOGV("lsc_table0_RGGB=[%d,%d,%d,%d]", lsc_16_bits[0], lsc_16_bits[gain_w * gain_h], lsc_16_bits[gain_w * gain_h * 2], lsc_16_bits[gain_w * gain_h * 3]);
		ISP_LOGV("lsc_table1_RGGB=[%d,%d,%d,%d]", lsc_16_bits[gain_w + 1], lsc_16_bits[gain_w * gain_h + gain_w + 1],
			 lsc_16_bits[gain_w * gain_h * 2 + gain_w + 1], lsc_16_bits[gain_w * gain_h * 3 + gain_w + 1]);
	} else {
		ISP_LOGE("lsc_otp_addr = %p, lsc_otp_len = %d. Parser lsc otp fail", lsc_otp_addr, lsc_otp_len);
		ISP_LOGE("sensor setting error, lsc_otp_len=%d, lsc_otp_chn_gain_num=%d, lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d",
			 lsc_otp_len, lsc_otp_chn_gain_num, lsc_otp_width, lsc_otp_height, lsc_otp_grid);
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

		ISP_LOGV("lsc_otp_oc_r=[%d,%d], lsc_otp_oc_gr=[%d,%d], lsc_otp_oc_gb=[%d,%d], lsc_otp_oc_b=[%d,%d] ",
			 lsc_param->lsc_otp_oc_r_x,
			 lsc_param->lsc_otp_oc_r_y,
			 lsc_param->lsc_otp_oc_gr_x,
			 lsc_param->lsc_otp_oc_gr_y, lsc_param->lsc_otp_oc_gb_x, lsc_param->lsc_otp_oc_gb_y, lsc_param->lsc_otp_oc_b_x, lsc_param->lsc_otp_oc_b_y);
	} else {
		ISP_LOGE("oc_otp_data = %p, oc_otp_len = %d, Parser OC otp fail", oc_otp_data, oc_otp_len);
		goto EXIT;
	}
	return LSC_SUCCESS;

EXIT:
	if (lsc_param->lsc_otp_table_addr != NULL) {
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

static cmr_s32 lsc_unload_lib(struct lsc_sprd_ctrl_context *cxt)
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

static cmr_s32 lsc_load_lib(struct lsc_sprd_ctrl_context *cxt)
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

	cxt->lib_ops.alsc_init = dlsym(cxt->lib_handle, "alsc_init");
	if (!cxt->lib_ops.alsc_init) {
		ISP_LOGE("fail to dlsym alsc_init");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_calc = dlsym(cxt->lib_handle, "alsc_calculation");
	if (!cxt->lib_ops.alsc_calc) {
		ISP_LOGE("fail to dlsym alsc_calculation");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_io_ctrl = dlsym(cxt->lib_handle, "alsc_ioctrl");
	if (!cxt->lib_ops.alsc_io_ctrl) {
		ISP_LOGE("fail to dlsym alsc_ioctrl");
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_deinit = dlsym(cxt->lib_handle, "alsc_deinit");
	if (!cxt->lib_ops.alsc_deinit) {
		ISP_LOGE("fail to dlsym alsc_deinit");
		goto error_dlsym;
	}
	ISP_LOGI("load lsc lib success");

	return LSC_SUCCESS;

error_dlsym:
	rtn = lsc_unload_lib(cxt);

exit:
	ISP_LOGE("fail to load lsc lib ret = %d", rtn);
	return rtn;
}

static void *lsc_flash_proc_init(struct lsc_sprd_ctrl_context *cxt)
{
	struct lsc_flash_proc_param *param = (struct lsc_flash_proc_param *)cxt->lsc_flash_proc_param;

	param->captureFlashEnvRatio = 0;
	param->captureFlash1ofALLRatio = 0;
	param->main_flash_from_other_parameter = 0;
	param->preflash_current_lnc_table_address = NULL;

	//for touch preflash
	param->is_touch_preflash = -99;
	param->ae_touch_framecount = -99;
	param->pre_flash_before_ae_touch_framecount = -99;
	param->pre_flash_before_framecount = -99;
	return param;
}

static void lsc_gen_flash_y_gain(cmr_u16 * flash_y_gain, cmr_u32 gain_width, cmr_u32 gain_height, cmr_u32 percent, cmr_u32 shift_x, cmr_u32 shift_y)
{
	unsigned int i, j;
	float ratio_4, ratio_2, x_2, y_2, cos_2, cos_4, inv_cos_4;
	unsigned int table_width = 200;
	unsigned int table_height = 150;
	unsigned short inv_cos4_table[200 * 150] = { 0 };
	unsigned int center_x = table_width / 2;
	unsigned int center_y = table_height / 2;
	unsigned int length_2 = center_x * center_x + center_y * center_y;

	if (percent == 0 || shift_x == 0 || shift_y == 0) {
		for (i = 0; i < gain_width * gain_height; i++) {
			flash_y_gain[i] = 0;
		}
	} else {
		percent = percent < 0 ? 0 : percent;
		percent = percent > 1000 ? 1000 : percent;
		shift_x = shift_x < 50 ? 50 : shift_x;
		shift_x = shift_x > 150 ? 150 : shift_x;
		shift_y = shift_y < 50 ? 50 : shift_y;
		shift_y = shift_y > 150 ? 150 : shift_y;
		center_x = center_x + shift_x - 100;
		center_y = center_y + shift_y - 100;

		ratio_4 = 100.0 / (float)(percent + 100);
		ratio_2 = (float)(sqrt((double)(ratio_4)));
		x_2 = ratio_2 / (1.0 - ratio_2);

		ISP_LOGD("flash_y_gain, percent=%d, shift_x=%d, shift_y=%d, center_x=%d, center_y=%d, length_2=%d, ratio_4=%f, ratio_2=%f, x_2=%f",
			 percent, shift_x, shift_y, center_x, center_y, length_2, ratio_4, ratio_2, x_2);

		for (j = 0; j < table_height; j++) {
			for (i = 0; i < table_width; i++) {
				y_2 = (float)((i - center_x) * (i - center_x) + (j - center_y) * (j - center_y)) / length_2;
				cos_2 = x_2 / (x_2 + y_2);
				cos_4 = cos_2 * cos_2;
				inv_cos_4 = 1.0 / cos_4;

				inv_cos4_table[j * table_width + i] = (unsigned short)(inv_cos_4 * 1024);
			}
		}

		lsc_scale_bilinear_short(inv_cos4_table, table_width, table_height, flash_y_gain, gain_width, gain_height);
	}
}

static cmr_s32 lsc_set_init_param(struct lsc_adv_init_param *init_param, struct lsc_sprd_ctrl_context *cxt, struct lsc_sprd_init_in *sprd_init_param)
{
	cmr_s32 i;
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc2_tune_param *param;

	cxt->alg_locked = 0;
	cxt->flash_mode = 0;
	cxt->pre_flash_mode = 0;
	cxt->flash_done_frame_count = 100;
	cxt->frame_count = 0;
	cxt->alg_count = 0;
	cxt->alg_quick_in = 0;
	cxt->alg_quick_in_frame = 3;	// alsc ctrl setting
	cxt->quik_in_start_frame = -99;
	cxt->init_skip_frame = 5;	// alsc ctrl setting
	cxt->bv_skip_frame = 0;
	cxt->cur_lsc_pm_mode = 0;	// 0: common table size,  1: 720p table size
	cxt->pre_lsc_pm_mode = 0;	// 0: common table size,  1: 720p table size
	cxt->img_width = init_param->img_width;
	cxt->img_height = init_param->img_height;
	cxt->gain_width = init_param->gain_width;
	cxt->gain_height = init_param->gain_height;
	cxt->init_img_width = init_param->img_width;
	cxt->init_img_height = init_param->img_height;
	cxt->init_gain_width = init_param->gain_width;
	cxt->init_gain_height = init_param->gain_height;
	cxt->init_grid = init_param->grid;
	cxt->fw_start_bv = 1600;
	cxt->fw_start_bv_gain = 128;
	cxt->gain_pattern = init_param->gain_pattern;
	cxt->output_gain_pattern = init_param->output_gain_pattern;
	cxt->change_pattern_flag = init_param->change_pattern_flag;
	cxt->is_master = init_param->is_master;
	cxt->is_multi_mode = init_param->is_multi_mode;
	cxt->grid = init_param->grid;
	cxt->camera_id = init_param->camera_id;
	cxt->can_update_dest = 1;
	cxt->alsc_update_flag = 0;
	cxt->fw_start_end = 0;
	cxt->alg_bypass = 0;
	cxt->lsc_pm0 = init_param->lsc_tab_address[0];
	cxt->output_gain_pattern = init_param->output_gain_pattern;

	// if no pm data, set default parameter and bypass alsc
	if (init_param->tune_param_ptr != NULL) {
		// init_param->tune_param_ptr are ALSC parameters from isp tool
		param = (struct lsc2_tune_param *)init_param->tune_param_ptr;
		cxt->LSC_SPD_VERSION = param->LSC_SPD_VERSION;
		cxt->calc_freq = param->freq;
		cxt->IIR_weight = param->IIR_weight;
		cxt->flash_enhance_ratio = param->flash_enhance_en;
		cxt->flash_center_shiftx = param->flash_enhance_max_strength;
		cxt->flash_center_shifty = param->flash_enahnce_gain;
	} else {
		cxt->alg_bypass = 1;
		ISP_LOGE("load alsc tuning parameters error, by pass alsc_calc");
		memset(cxt->flash_y_gain, 0, 32 * 32 * sizeof(cmr_u16));
	}

	//pre process the lsc table
	if (cxt->is_planar == 1) {
		for (i = 0; i < 9; i++) {
			lsc_table_interlace2planar(init_param->lsc_tab_address[i], cxt->init_gain_width, cxt->init_gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
		}
	} else {
		for (i = 0; i < 9; i++)
			lsc_interlace_change_pattern(init_param->lsc_tab_address[i], cxt->init_gain_width, cxt->init_gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
	}

	//set sprd init param
	sprd_init_param->gain_width = init_param->gain_width;
	sprd_init_param->gain_height = init_param->gain_height;
	sprd_init_param->gain_pattern = init_param->gain_pattern;
	sprd_init_param->output_gain_pattern = init_param->output_gain_pattern;
	sprd_init_param->grid = init_param->grid;
	sprd_init_param->camera_id = init_param->camera_id;
	sprd_init_param->lsc_id = cxt->lsc_id;
	sprd_init_param->is_planar = cxt->is_planar;
	for (i = 0; i < 9; i++) {
		sprd_init_param->lsc_tab_address[i] = init_param->lsc_tab_address[i];	// the address of table parameter
	}
	memcpy(&sprd_init_param->tune_param,  init_param->tune_param_ptr, sizeof(struct lsc2_tune_param));
	sprd_init_param->lsc_otp_table_en =  init_param->lsc_otp_table_en;
	sprd_init_param->lsc_otp_table_width =  init_param->lsc_otp_table_width;
	sprd_init_param->lsc_otp_table_height =  init_param->lsc_otp_table_height;
	sprd_init_param->lsc_otp_grid =  init_param->lsc_otp_grid;
	sprd_init_param->lsc_otp_raw_width = init_param->lsc_otp_raw_width;
	sprd_init_param->lsc_otp_raw_height = init_param->lsc_otp_raw_height;
	sprd_init_param->lsc_otp_table_addr =  init_param->lsc_otp_table_addr;
	sprd_init_param->lsc_otp_oc_en =  init_param->lsc_otp_oc_en;
	sprd_init_param->lsc_otp_oc_r_x =  init_param->lsc_otp_oc_r_x;
	sprd_init_param->lsc_otp_oc_r_y =  init_param->lsc_otp_oc_r_y ;
	sprd_init_param->lsc_otp_oc_gr_x =  init_param->lsc_otp_oc_gr_x;
	sprd_init_param->lsc_otp_oc_gr_y =  init_param->lsc_otp_oc_gr_y;
	sprd_init_param->lsc_otp_oc_gb_x =  init_param->lsc_otp_oc_gb_x;
	sprd_init_param->lsc_otp_oc_gb_y =  init_param->lsc_otp_oc_gb_y;
	sprd_init_param->lsc_otp_oc_b_x =  init_param->lsc_otp_oc_b_x;
	sprd_init_param->lsc_otp_oc_b_y =  init_param->lsc_otp_oc_b_y;

	return rtn;
}

static void lsc_scl_for_ae_stat(struct lsc_sprd_ctrl_context *cxt, struct lsc_adv_calc_param *param)
{
	cmr_u32 i, j, ii, jj;
	cmr_u64 r = 0, g = 0, b = 0;
	cmr_u32 blk_num_w = param->stat_size.w;
	cmr_u32 blk_num_h = param->stat_size.h;
	cmr_u32 *r_stat = (cmr_u32 *) param->stat_img.r;
	cmr_u32 *g_stat = (cmr_u32 *) param->stat_img.gr;
	cmr_u32 *b_stat = (cmr_u32 *) param->stat_img.b;

	blk_num_h = (blk_num_h < 32) ? 32 : blk_num_h;
	blk_num_w = (blk_num_w < 32) ? 32 : blk_num_w;
	cmr_u32 ratio_h = blk_num_h / 32;
	cmr_u32 ratio_w = blk_num_w / 32;

	memset(cxt->ae_stat, 0, 1024 * 3 * sizeof(cmr_u32));

	for (i = 0; i < blk_num_h; ++i) {
		ii = (cmr_u32) (i / ratio_h);
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

static void lsc_scale_table_to_stat_size(cmr_u16 * new_tab, cmr_u16 * org_tab, cmr_u32 gain_width, cmr_u32 gain_height,
					 cmr_u32 stat_width, cmr_u32 stat_height, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 grid)
{
	cmr_u32 i, j, ii, jj;
	cmr_u32 stat_dw = (raw_width / stat_width) / 2 * 2;
	cmr_u32 stat_dh = (raw_height / stat_height) / 2 * 2;
	cmr_u32 dx = (raw_width - stat_width * stat_dw) / 2;
	cmr_u32 dy = (raw_height - stat_height * stat_dh) / 2;

	float raw_width2 = (float)(raw_width - 2 * dx);
	float raw_height2 = (float)(raw_height - 2 * dy);
	float step_width = raw_width2 / stat_width;
	float step_height = raw_height2 / stat_height;
	float org_x = (step_width / 2) + dx;
	float org_y = (step_height / 2) + dy;

	//////////////////////// match location  ////////////////////////

	float x, y, lt, rt;
	cmr_u32 X_L[100] = { 0 };
	float D_L[100] = { 0.0f };
	cmr_u32 Y_U[100] = { 0 };
	float D_U[100] = { 0.0f };

	// x-axis
	for (i = 0; i < stat_width; i++) {
		x = (float)(step_width * i) + org_x;
		for (j = 1; j < gain_width - 1; j++) {
			lt = (float)(2 * grid * (j - 1));
			rt = (float)(2 * grid * j);

			if (lt <= x && x <= rt) {
				X_L[i] = j;
				D_L[i] = x - lt;
			}
		}
	}

	// y-axis
	for (i = 0; i < stat_height; i++) {
		y = (float)(step_height * i) + org_y;
		for (j = 1; j < gain_height - 1; j++) {
			lt = (float)(2 * grid * (j - 1));
			rt = (float)(2 * grid * j);

			if (lt <= y && y <= rt) {
				Y_U[i] = j;
				D_U[i] = y - lt;
			}
		}
	}

	//////////////////////// compute interpolation  ////////////////////////

	float coef = 1.0f / (2 * grid * 2 * grid);
	float TL, TR, BL, BR, det_L, det_U, det_R, det_D, tmp;

	for (j = 0; j < stat_height; j++) {
		for (i = 0; i < stat_width; i++) {
			ii = X_L[i];
			jj = Y_U[j];
			TL = (float)org_tab[jj * gain_width + ii];
			TR = (float)org_tab[jj * gain_width + (ii + 1)];
			BL = (float)org_tab[(jj + 1) * gain_width + ii];
			BR = (float)org_tab[(jj + 1) * gain_width + (ii + 1)];
			det_L = D_L[i];
			det_U = D_U[j];
			det_R = 2 * grid - det_L;
			det_D = 2 * grid - det_U;

			tmp = coef * (TL * det_D * det_R + TR * det_D * det_L + BL * det_U * det_R + BR * det_U * det_L);
			new_tab[j * stat_width + i] = (unsigned short)LSC_CLIP(tmp + 0.5f, LSC_GAIN_LOWER, LSC_GAIN_UPPER);
		}
	}
}

static void lsc_inverse_ae_stat(struct lsc_sprd_ctrl_context *cxt, cmr_u16 * inverse_table)
{
	cmr_u32 i;
	cmr_u16 gain_r[32 * 32];
	cmr_u16 gain_gr[32 * 32];
	cmr_u16 gain_gb[32 * 32];
	cmr_u16 gain_b[32 * 32];
	cmr_u16 gain_g[32 * 32];
	cmr_u16 scaled_gain_r[32 * 32];
	cmr_u16 scaled_gain_g[32 * 32];
	cmr_u16 scaled_gain_b[32 * 32];

	cmr_u32 *stat_r = &cxt->ae_stat[0];
	cmr_u32 *stat_g = &cxt->ae_stat[1024];
	cmr_u32 *stat_b = &cxt->ae_stat[2048];
	cmr_u32 stat_width = 32;
	cmr_u32 stat_height = 32;
	cmr_u32 img_width = cxt->img_width;
	cmr_u32 img_height = cxt->img_height;
	cmr_u32 gain_width = cxt->gain_width;
	cmr_u32 gain_height = cxt->gain_height;
	cmr_u32 grid = cxt->grid;

	// convert table to channel
	if (cxt->is_planar == 1) {
		lsc_planar_to_channel(cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern, gain_r, gain_gr, gain_gb, gain_b, inverse_table);
	} else {
		lsc_interlace_to_channel(cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern, gain_r, gain_gr, gain_gb, gain_b, inverse_table);
	}

	for (i = 0; i < 32 * 32; i++) {
		gain_g[i] = (cmr_u32) ((gain_gr[i] + gain_gb[i]) / 2);
	}

	lsc_scale_table_to_stat_size(scaled_gain_r, gain_r, gain_width, gain_height, stat_width, stat_height, img_width, img_height, grid);
	lsc_scale_table_to_stat_size(scaled_gain_g, gain_g, gain_width, gain_height, stat_width, stat_height, img_width, img_height, grid);
	lsc_scale_table_to_stat_size(scaled_gain_b, gain_b, gain_width, gain_height, stat_width, stat_height, img_width, img_height, grid);

	for (i = 0; i < MAX_STAT_WIDTH * MAX_STAT_HEIGHT; i++) {
		stat_r[i] = (cmr_u32) (stat_r[i] / scaled_gain_r[i]) << 10;
		stat_g[i] = (cmr_u32) (stat_g[i] / scaled_gain_g[i]) << 10;
		stat_b[i] = (cmr_u32) (stat_b[i] / scaled_gain_b[i]) << 10;
	}
}

static cmr_u32 lsc_get_alg_in_flag(struct lsc_sprd_ctrl_context *cxt, cmr_u32 * IIR_weight)
{
	cmr_s32 rtn = 0;

	// pre_flash_mode in
	if (cxt->pre_flash_mode) {
		ISP_LOGV("pre_flash_mode=1, do alsc_calc");
		return 1;
	}

	if (cxt->frame_count == 1) {
		cxt->alg_quick_in = 1;
		cxt->quik_in_start_frame = -99;
		ISP_LOGV("frame_count=1, set alg_quick_in=1 and wait for init_skip_frame");
	}
	// init_skip_frame return
	if (cxt->frame_count < cxt->init_skip_frame) {
		ISP_LOGV("init_skip_frame=%d, frame_count=%d, return", cxt->init_skip_frame, cxt->frame_count);
		return rtn;
	}
	// main flash return
	if (cxt->flash_mode == 1) {
		ISP_LOGV("MAIN_FLASH return, ctx->flash_mode=1, frame_count=%d", cxt->frame_count);
		return rtn;
	}
	// can_update_dest 0 return
	if (cxt->can_update_dest == 0) {
		ISP_LOGV("can_update_dest = 0 return, frame_count=%d", cxt->frame_count);
		return rtn;
	}
	// alg_quick_in
	if (cxt->alg_quick_in == 1) {
		// alg_quick_in start
		if (cxt->quik_in_start_frame == -99) {
			cxt->quik_in_start_frame = cxt->frame_count;
			*IIR_weight = 16;	// drop the last calc result, and send new calc result to output
		}
		// during alg_quick_in
		if (cxt->frame_count < (cxt->quik_in_start_frame + cxt->alg_quick_in_frame)) {
			rtn = 1;
			ISP_LOGV("rtn=%d, frame_count=%d, IIR_weight=%d ,quik_in_start_frame=%d, alg_quick_in=%d",
				 rtn, cxt->frame_count, *IIR_weight, cxt->quik_in_start_frame, cxt->alg_quick_in);
			// alg_quick_in end
		} else {
			rtn = 0;
			cxt->quik_in_start_frame = -99;
			cxt->alg_quick_in = 0;
		}
		// normal in
	} else {
		rtn = 0;
		cxt->quik_in_start_frame = -99;

		// set calc_freq
		if (cxt->frame_count % (cxt->calc_freq * 3) == 1)
			rtn = 1;

		// alsc locked
		if (cxt->alg_locked && cxt->alg_quick_in == 0 && cxt->pre_flash_mode == 0 && cxt->frame_count) {
			ISP_LOGV("is locked && cxt->alg_quick_in==0 && cxt->pre_flash_mode==0 , frame_count=%d", cxt->frame_count);
			rtn = 0;
		}
	}

	return rtn;
}

static cmr_s32 lsc_fwstart_update_first_tab(struct lsc_sprd_ctrl_context *cxt, struct alsc_fwstart_info *fwstart_info, cmr_u32 planar_flag)
{
	cmr_u32 i;
	struct lsc_flash_proc_param *flash_param = (struct lsc_flash_proc_param *)cxt->lsc_flash_proc_param;
	cmr_u32 pre_width = cxt->gain_width;
	cmr_u32 pre_height = cxt->gain_height;
	cmr_u32 pre_pattern = cxt->gain_pattern;
	cmr_u16 lsc_pre_reslut_table[32 * 32 * 4] = { 0 };
	cmr_u16 lsc_pre_table[32 * 32 * 4] = { 0 };
	cmr_u32 new_width = fwstart_info->gain_width_new;
	cmr_u32 new_height = fwstart_info->gain_height_new;
	cmr_u32 new_pattern = 3;
	cmr_u16 *lsc_result_address_new = fwstart_info->lsc_result_address_new;	//new dest output address
	cmr_u16 lsc_new_table[32 * 32 * 4] = { 0 };	// golden new DNP table from pm, no OTP apply

	if (cxt->lsc_buffer == NULL || cxt->lsc_pm0 == NULL || cxt->fwstop_output_table[0] == 0) {
		ISP_LOGV("FW_START, lsc_buffer %p, lsc_pm0 %p, fwstop_output_table[0] = %d", cxt->lsc_buffer, cxt->lsc_pm0, cxt->fwstop_output_table[0]);
		return -1;
	}
	// flash mode, table without post gain
	ISP_LOGV("FW_START, Flash Mode, main_flash_from_other_parameter=%d", flash_param->main_flash_from_other_parameter);
	if (flash_param->main_flash_from_other_parameter == 1) {
		for (i = 0; i < pre_width * pre_height * 4; i++) {
			lsc_pre_reslut_table[i] = flash_param->preflash_current_output_table[i];	//destination buffer table with post gain
			lsc_pre_table[i] = flash_param->preflash_current_lnc_table[i];	// golden pre DNP from pm, no OTP apply
		}
		flash_param->main_flash_from_other_parameter = 0;
		flash_param->preflash_current_lnc_table_address = NULL;
		// normal mode, table with post gain
	} else {
		for (i = 0; i < pre_width * pre_height * 4; i++) {
			lsc_pre_reslut_table[i] = cxt->fwstop_output_table[i];
			lsc_pre_table[i] = cxt->lsc_pm0[i];	// golden pre DNP from pm, no OTP apply
		}
	}

	memset(cxt->fwstop_output_table, 0, sizeof(cmr_u16) * 32 * 32 * 4);

	// collect the new dest table, new dnp table and other info
	memcpy(lsc_new_table, fwstart_info->lsc_tab_address_new[0], sizeof(cmr_u16) * new_width * new_height * 4);
	lsc_table_interlace2planar(lsc_new_table, new_width, new_height, cxt->gain_pattern, cxt->output_gain_pattern);

	new_pattern = lsc_raw_pattern_to_lsc_pattern(fwstart_info->image_pattern_new);

	ISP_LOGV("FW_START, pre:[%d, %d, %d], new:[%d, %d, %d]", pre_width, pre_height, pre_pattern, new_width, new_height, new_pattern);

	if (pre_width == new_width && pre_height == new_height) {
		memcpy(lsc_result_address_new, lsc_pre_reslut_table, pre_width * pre_height * 4 * sizeof(cmr_u16));
	} else {
		// scale pre DNP table to new gain size
		cmr_u16 output_tab[32 * 32 * 4] = { 0 };
		cmr_u16 output_r_tab[32 * 32] = { 0 };
		cmr_u16 output_gr_tab[32 * 32] = { 0 };
		cmr_u16 output_gb_tab[32 * 32] = { 0 };
		cmr_u16 output_b_tab[32 * 32] = { 0 };

		lsc_table_linear_scaler(lsc_pre_table, pre_width, pre_height, output_tab, new_width, new_height, planar_flag);

		if (cxt->is_planar == 0) {
			lsc_interlace_to_channel(new_width, new_height, pre_pattern, output_r_tab, output_gr_tab, output_gb_tab, output_b_tab, output_tab);
		} else {
			lsc_planar_to_channel(new_width, new_height, pre_pattern, output_r_tab, output_gr_tab, output_gb_tab, output_b_tab, output_tab);
		}

		// get new DNP tab to 4 channel
		cmr_u16 output_r_tab_new[32 * 32] = { 0 };
		cmr_u16 output_gr_tab_new[32 * 32] = { 0 };
		cmr_u16 output_gb_tab_new[32 * 32] = { 0 };
		cmr_u16 output_b_tab_new[32 * 32] = { 0 };

		if (cxt->is_planar == 0) {
			lsc_interlace_to_channel(new_width, new_height, new_pattern, output_r_tab_new, output_gr_tab_new, output_gb_tab_new, output_b_tab_new, lsc_new_table);
		} else {
			lsc_planar_to_channel(new_width, new_height, new_pattern, output_r_tab_new, output_gr_tab_new, output_gb_tab_new, output_b_tab_new, lsc_new_table);
		}

		// get scale weight table from the ratio of the 2 DNP tables, that is (new DNP)/(old scaled DNP)
		float lsc_new_weight_tab_gb[32 * 32] = { 0 };
		float lsc_new_weight_tab_b[32 * 32] = { 0 };
		float lsc_new_weight_tab_r[32 * 32] = { 0 };
		float lsc_new_weight_tab_gr[32 * 32] = { 0 };
		float rate_gb = 0.0;
		float rate_b = 0.0;
		float rate_r = 0.0;
		float rate_gr = 0.0;

		for (i = 0; i < new_width * new_height; i++) {
			rate_r = 0.0;
			rate_gr = 0.0;
			rate_gb = 0.0;
			rate_b = 0.0;
			//r
			if (output_r_tab[i] == 0 || output_r_tab[i] == 1024) {
				rate_r = 1;
			} else {
				rate_r = (float)output_r_tab_new[i] / (float)output_r_tab[i];
			}
			lsc_new_weight_tab_r[i] = rate_r;
			//gr
			if (output_gr_tab[i] == 0 || output_gr_tab[i] == 1024) {
				rate_gr = 1;
			} else {
				rate_gr = (float)output_gr_tab_new[i] / (float)output_gr_tab[i];
			}
			lsc_new_weight_tab_gr[i] = rate_gr;
			//gb
			if (output_gb_tab[i] == 0 || output_gb_tab[i] == 1024) {
				rate_gb = 1;
			} else {
				rate_gb = (float)output_gb_tab_new[i] / (float)output_gb_tab[i];
			}
			lsc_new_weight_tab_gb[i] = rate_gb;
			//b
			if (output_b_tab[i] == 0 || output_b_tab[i] == 1024) {
				rate_b = 1;
			} else {
				rate_b = (float)output_b_tab_new[i] / (float)output_b_tab[i];
			}
			lsc_new_weight_tab_b[i] = rate_b;
		}

		// scale dest table
		cmr_u16 output_r[32 * 32] = { 0 };
		cmr_u16 output_gr[32 * 32] = { 0 };
		cmr_u16 output_gb[32 * 32] = { 0 };
		cmr_u16 output_b[32 * 32] = { 0 };

		lsc_table_linear_scaler(lsc_pre_reslut_table, pre_width, pre_height, lsc_result_address_new, new_width, new_height, cxt->is_planar);
		if (cxt->is_planar == 0) {
			lsc_interlace_to_channel(new_width, new_height, cxt->output_gain_pattern, output_r, output_gr, output_gb, output_b, lsc_result_address_new);
		} else {
			lsc_planar_to_channel(new_width, new_height, cxt->output_gain_pattern, output_r, output_gr, output_gb, output_b, lsc_result_address_new);
		}
		ISP_LOGV("FW_START, pre_scaled_DNP[%d,%d,%d,%d], pre_scaled_result[%d,%d,%d,%d]",
			 output_tab[0], output_tab[1], output_tab[2], output_tab[3],
			 lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);

		if (cxt->change_pattern_flag)
			new_pattern = cxt->output_gain_pattern;

		// send output to lsc_result_address_new
		cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
		cmr_u32 gr_chnl_off, r_chnl_off, b_chnl_off, gb_chnl_off;

		lsc_get_channel_index(new_pattern, &off_gr, &off_r, &off_b, &off_gb);

		gr_chnl_off = off_gr * new_width * new_height;
		r_chnl_off = off_r * new_width * new_height;
		b_chnl_off = off_b * new_width * new_height;
		gb_chnl_off = off_gb * new_width * new_height;

		if (cxt->is_planar == 0) {
			for (i = 0; i < new_width * new_height; i++) {
				lsc_result_address_new[4 * i + off_gr] = RANGE(output_gr[i] * lsc_new_weight_tab_gr[i], 1024, 16383);
				lsc_result_address_new[4 * i + off_r] = RANGE(output_r[i] * lsc_new_weight_tab_r[i], 1024, 16383);
				lsc_result_address_new[4 * i + off_b] = RANGE(output_b[i] * lsc_new_weight_tab_b[i], 1024, 16383);
				lsc_result_address_new[4 * i + off_gb] = RANGE(output_gb[i] * lsc_new_weight_tab_gb[i], 1024, 16383);
			}
		} else {
			for (i = 0; i < new_width * new_height; i++) {
				lsc_result_address_new[i + gr_chnl_off] = RANGE(output_gr[i] * lsc_new_weight_tab_gr[i], 1024, 16383);
				lsc_result_address_new[i + r_chnl_off] = RANGE(output_r[i] * lsc_new_weight_tab_r[i], 1024, 16383);
				lsc_result_address_new[i + b_chnl_off] = RANGE(output_b[i] * lsc_new_weight_tab_b[i], 1024, 16383);
				lsc_result_address_new[i + gb_chnl_off] = RANGE(output_gb[i] * lsc_new_weight_tab_gb[i], 1024, 16383);
			}
		}

		ISP_LOGV("FW_START, pre_scaled_weighted_result[%d,%d,%d,%d]",
			 lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);
	}

	// update zone
	memcpy(cxt->fwstart_new_scaled_table, lsc_result_address_new, new_width * new_height * 4 * sizeof(cmr_u16));

	// binning preflash -> fullsize main flash
	memcpy(flash_param->preflash_guessing_mainflash_output_table, lsc_result_address_new, new_width * new_height * 4 * sizeof(cmr_u16));

	ISP_LOGV("FW_START, fwstart_update_first_tab done, lsc_result_address_new=[%d,%d,%d,%d]",
		 lsc_result_address_new[0], lsc_result_address_new[1], lsc_result_address_new[2], lsc_result_address_new[3]);
	return 0;
}

static int lsc_pm_table_crop(struct pm_lsc_full *src, struct pm_lsc_crop *dst, cmr_u8 is_planar)
{
	int rtn = 0;
	unsigned int i, j, k;

	ISP_LOGV("pm_lsc_full[%d,%d,%d,%d,%d]", src->img_width, src->img_height, src->grid, src->gain_width, src->gain_height);
	ISP_LOGV("pm_lsc_crop[%d,%d,%d,%d,%d,%d,%d]", dst->img_width, dst->img_height, dst->start_x, dst->start_y, dst->grid, dst->gain_width, dst->gain_height);

	// error cases
	if (dst->start_x > src->img_width
	    || dst->start_y > src->img_height
	    || dst->start_x + dst->img_width > src->img_width
	    || dst->start_y + dst->img_height > src->img_height
	    || dst->start_x < 0 || dst->start_y < 0 || dst->start_x + (dst->gain_width - 2) * (dst->grid * 2) > (src->gain_width - 2) * (src->grid * 2)
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

	if (is_planar == 0) {
		for (i = 0; i < src->gain_width * src->gain_height; i++) {
			src_ch0[i] = src->input_table_buffer[4 * i + 0];
			src_ch1[i] = src->input_table_buffer[4 * i + 1];
			src_ch2[i] = src->input_table_buffer[4 * i + 2];
			src_ch3[i] = src->input_table_buffer[4 * i + 3];
		}
	} else {
		for (i = 0; i < src->gain_width * src->gain_height; i++) {
			src_ch0[i] = src->input_table_buffer[i + 0 * src->gain_width * src->gain_height];
			src_ch1[i] = src->input_table_buffer[i + 1 * src->gain_width * src->gain_height];
			src_ch2[i] = src->input_table_buffer[i + 2 * src->gain_width * src->gain_height];
			src_ch3[i] = src->input_table_buffer[i + 3 * src->gain_width * src->gain_height];
		}
	}

	// malloc buffer to store interlace dst table
	unsigned short *out_buffer = (unsigned short *)malloc(dst->gain_width * dst->gain_height * 4 * sizeof(unsigned short));
	memset(out_buffer, 0, dst->gain_width * dst->gain_height * 4 * sizeof(unsigned short));

	// define crop table parameters
	unsigned int ch_start_x = 0;	// start_x on channel plane;
	unsigned int ch_start_y = 0;	// start_y on channel plane;
	unsigned int crop_table_x = 0;	// dst table coord-x on channel plane
	unsigned int crop_table_y = 0;	// dst table coord-y on channel plane
	int TL_i = 0;		// src table top left index-i
	int TL_j = 0;		// src table top left index-j
	float dx = 0.0;		// distence to left  , where total length normalize to 1
	float dy = 0.0;		// distence to bottem, where total length normalize to 1

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

			out_buffer[(j * dst->gain_width + i) * 4 + 0] = (cmr_u16) table_bicubic_interpolation(src_ch0, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
			out_buffer[(j * dst->gain_width + i) * 4 + 1] = (cmr_u16) table_bicubic_interpolation(src_ch1, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
			out_buffer[(j * dst->gain_width + i) * 4 + 2] = (cmr_u16) table_bicubic_interpolation(src_ch2, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
			out_buffer[(j * dst->gain_width + i) * 4 + 3] = (cmr_u16) table_bicubic_interpolation(src_ch3, src->gain_width, src->gain_height, TL_i, TL_j, dx, dy);
		}
	}

	// generate outer table edge
	unsigned short inner_table_1 = 0;
	unsigned short inner_table_2 = 0;
	unsigned short inner_table_3 = 0;
	for (k = 0; k < 4; k++) {
		for (j = 1; j < dst->gain_height - 1; j++) {
			inner_table_1 = out_buffer[(j * dst->gain_width + 1) * 4 + k];
			inner_table_2 = out_buffer[(j * dst->gain_width + 2) * 4 + k];
			inner_table_3 = out_buffer[(j * dst->gain_width + 3) * 4 + k];
			out_buffer[(j * dst->gain_width + 0) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
			inner_table_1 = out_buffer[(j * dst->gain_width + (dst->gain_width - 2)) * 4 + k];
			inner_table_2 = out_buffer[(j * dst->gain_width + (dst->gain_width - 3)) * 4 + k];
			inner_table_3 = out_buffer[(j * dst->gain_width + (dst->gain_width - 4)) * 4 + k];
			out_buffer[(j * dst->gain_width + (dst->gain_width - 1)) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
		}
	}

	for (k = 0; k < 4; k++) {
		for (i = 0; i < dst->gain_width; i++) {
			inner_table_1 = out_buffer[(1 * dst->gain_width + i) * 4 + k];
			inner_table_2 = out_buffer[(2 * dst->gain_width + i) * 4 + k];
			inner_table_3 = out_buffer[(3 * dst->gain_width + i) * 4 + k];
			out_buffer[(0 * dst->gain_width + i) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
			inner_table_1 = out_buffer[((dst->gain_height - 2) * dst->gain_width + i) * 4 + k];
			inner_table_2 = out_buffer[((dst->gain_height - 3) * dst->gain_width + i) * 4 + k];
			inner_table_3 = out_buffer[((dst->gain_height - 4) * dst->gain_width + i) * 4 + k];
			out_buffer[((dst->gain_height - 1) * dst->gain_width + i) * 4 + k] = (inner_table_1 - inner_table_2) * 3 + inner_table_3;
		}
	}

	// save out_buffer back to dst->output_table_buffer
	if (is_planar == 1) {
		for (i = 0; i < dst->gain_width * dst->gain_height; i++) {
			dst->output_table_buffer[i + 0 * dst->gain_width * dst->gain_height] = out_buffer[4 * i + 0];
			dst->output_table_buffer[i + 1 * dst->gain_width * dst->gain_height] = out_buffer[4 * i + 1];
			dst->output_table_buffer[i + 2 * dst->gain_width * dst->gain_height] = out_buffer[4 * i + 2];
			dst->output_table_buffer[i + 3 * dst->gain_width * dst->gain_height] = out_buffer[4 * i + 3];
		}
	} else {
		memcpy(dst->output_table_buffer, out_buffer, dst->gain_width * dst->gain_height * 4 * sizeof(unsigned short));
	}

	// free generated buffer
	lsc_std_free(src_ch0);
	lsc_std_free(src_ch1);
	lsc_std_free(src_ch2);
	lsc_std_free(src_ch3);
	lsc_std_free(out_buffer);

	return rtn;
}

static void lsc_save_last_info(struct lsc_last_info *cxt, unsigned int camera_id, unsigned int full_flag)
{
	FILE *fp = NULL;
	struct lsc_last_info full_cxt[MAX_CAMERA_ID];
	char *ptr = (char *)full_cxt;

	if (full_flag == 1) {
		fp = fopen(CAMERA_DATA_FILE "/lsc.file", "rb");
	} else {
		fp = fopen(CAMERA_DATA_FILE "/lsc_video.file", "rb");
	}

	//read all last info in lsc.file first
	if (fp == NULL) {
		memset((void *)ptr, 0, sizeof(struct lsc_last_info) * MAX_CAMERA_ID);
	} else {
		fread(ptr, 1, sizeof(struct lsc_last_info) * MAX_CAMERA_ID, fp);
		fclose(fp);
		fp = NULL;
	}

	// save cxt of camera_id to lsc.file
	ptr = ptr + camera_id * sizeof(struct lsc_last_info);
	memcpy(ptr, (char *)cxt, sizeof(struct lsc_last_info));

	if (full_flag == 1) {
		fp = fopen(CAMERA_DATA_FILE "/lsc.file", "wb");
	} else {
		fp = fopen(CAMERA_DATA_FILE "/lsc_video.file", "wb");
	}

	if (fp) {
		ISP_LOGV("bv:%d, bv_gain:%d, table_rgb[%d,%d,%d,%d], table_size[%d,%d]", cxt->bv, cxt->bv_gain,
			 cxt->table[0], cxt->table[1], cxt->table[2], cxt->table[3], cxt->gain_width, cxt->gain_height);

		fwrite((char *)full_cxt, 1, sizeof(struct lsc_last_info) * MAX_CAMERA_ID, fp);
		fclose(fp);
		fp = NULL;
	}
}

static void lsc_read_last_info(struct lsc_last_info *cxt, unsigned int camera_id, unsigned int full_flag)
{
	FILE *fp = NULL;
	struct lsc_last_info full_cxt[MAX_CAMERA_ID];
	char *ptr = (char *)full_cxt;
	int num_read;

	if (full_flag == 1) {
		fp = fopen(CAMERA_DATA_FILE "/lsc.file", "rb");
	} else {
		fp = fopen(CAMERA_DATA_FILE "/lsc_video.file", "rb");
	}

	if (fp) {
		memset((void *)ptr, 0, sizeof(struct lsc_last_info) * MAX_CAMERA_ID);
		num_read = fread((char *)ptr, 1, sizeof(struct lsc_last_info) * MAX_CAMERA_ID, fp);

		ptr = ptr + sizeof(struct lsc_last_info) * camera_id;
		memcpy((char *)cxt, (char *)ptr, sizeof(struct lsc_last_info));

		fclose(fp);
		fp = NULL;

		ISP_LOGV("bv:%d, bv_gain:%d, table_rgb[%d,%d,%d,%d], table_size[%d,%d]", cxt->bv, cxt->bv_gain, cxt->table[0], cxt->table[1],
			cxt->table[2], cxt->table[3], cxt->gain_width, cxt->gain_height);
	}
}

static int lsc_preprocess_fwstart_info(struct lsc_sprd_ctrl_context *cxt, struct alsc_fwstart_info *fwstart_info)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct pm_lsc_full *pm_lsc_full = NULL;
	struct pm_lsc_crop *pm_lsc_crop = NULL;
	int binning_crop = 0;
	int i = 0;

	if (cxt->LSC_SPD_VERSION >= 6) {
		if (cxt->init_img_width / fwstart_info->img_width_new == cxt->init_img_height / fwstart_info->img_height_new) {
			cxt->grid = (cmr_u32) (cxt->init_grid / (cxt->init_img_width / fwstart_info->img_width_new));
			cxt->gain_width = cxt->init_gain_width;
			cxt->gain_height = cxt->init_gain_height;
			fwstart_info->grid_new = cxt->grid;
			fwstart_info->gain_width_new = cxt->gain_width;
			fwstart_info->gain_height_new = cxt->gain_height;
			for (i = 0; i < 8; i++) {
				fwstart_info->lsc_tab_address_new[i] = cxt->std_init_lsc_table_param_buffer[i];
				memcpy(cxt->std_lsc_table_param_buffer[i], cxt->std_init_lsc_table_param_buffer[i], cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));
			}
			ISP_LOGI("FW_START parameter normalization, case1 n binning, grid=%d, lsc_id=%d", cxt->grid, cxt->lsc_id);
		} else if (fwstart_info->img_width_new == 1280 && fwstart_info->img_height_new == 720) {
			cxt->grid = 32;
			cxt->gain_width = 23;
			cxt->gain_height = 15;
			fwstart_info->grid_new = cxt->grid;
			fwstart_info->gain_width_new = cxt->gain_width;
			fwstart_info->gain_height_new = cxt->gain_height;
			pm_lsc_full = (struct pm_lsc_full *)malloc(sizeof(struct pm_lsc_full));
			pm_lsc_full->img_width = cxt->init_img_width;
			pm_lsc_full->img_height = cxt->init_img_height;
			pm_lsc_full->grid = cxt->init_grid;
			pm_lsc_full->gain_width = cxt->init_gain_width;
			pm_lsc_full->gain_height = cxt->init_gain_height;
			// Notice, if the crop action from binning size raw, do following action
			binning_crop = 1;
			if (binning_crop) {
				pm_lsc_full->img_width /= 2;
				pm_lsc_full->img_height /= 2;
				pm_lsc_full->grid /= 2;
			}

			pm_lsc_crop = (struct pm_lsc_crop *)malloc(sizeof(struct pm_lsc_crop));
			pm_lsc_crop->img_width = fwstart_info->img_width_new;
			pm_lsc_crop->img_height = fwstart_info->img_height_new;
			pm_lsc_crop->start_x = (pm_lsc_full->img_width - pm_lsc_crop->img_width) / 2;	// for crop center case
			pm_lsc_crop->start_y = (pm_lsc_full->img_height - pm_lsc_crop->img_height) / 2;	// for crop center case
			pm_lsc_crop->grid = 32;
			pm_lsc_crop->gain_width = 23;
			pm_lsc_crop->gain_height = 15;
			for (i = 0; i < 8; i++) {
				pm_lsc_full->input_table_buffer = cxt->std_init_lsc_table_param_buffer[i];
				pm_lsc_crop->output_table_buffer = cxt->std_lsc_table_param_buffer[i];
				rtn = lsc_pm_table_crop(pm_lsc_full, pm_lsc_crop, cxt->is_planar);
				fwstart_info->lsc_tab_address_new[i] = cxt->std_lsc_table_param_buffer[i];
			}
			lsc_std_free(pm_lsc_full);
			lsc_std_free(pm_lsc_crop);
		} else if (fwstart_info->img_width_new == 1920 && fwstart_info->img_height_new == 1080) {
			cxt->grid = 48;
			cxt->gain_width = 23;
			cxt->gain_height = 15;
			fwstart_info->grid_new = cxt->grid;
			fwstart_info->gain_width_new = cxt->gain_width;
			fwstart_info->gain_height_new = cxt->gain_height;
			pm_lsc_full = (struct pm_lsc_full *)malloc(sizeof(struct pm_lsc_full));
			pm_lsc_full->img_width = cxt->init_img_width;
			pm_lsc_full->img_height = cxt->init_img_height;
			pm_lsc_full->grid = cxt->init_grid;
			pm_lsc_full->gain_width = cxt->init_gain_width;
			pm_lsc_full->gain_height = cxt->init_gain_height;
			// Notice, if the crop action from binning size raw, do following action
			binning_crop = 1;
			if (binning_crop) {
				pm_lsc_full->img_width /= 2;
				pm_lsc_full->img_height /= 2;
				pm_lsc_full->grid /= 2;
			}

			pm_lsc_crop = (struct pm_lsc_crop *)malloc(sizeof(struct pm_lsc_crop));
			pm_lsc_crop->img_width = fwstart_info->img_width_new;
			pm_lsc_crop->img_height = fwstart_info->img_height_new;
			pm_lsc_crop->start_x = (pm_lsc_full->img_width - pm_lsc_crop->img_width) / 2;	// for crop center case
			pm_lsc_crop->start_y = (pm_lsc_full->img_height - pm_lsc_crop->img_height) / 2;	// for crop center case
			pm_lsc_crop->grid = 48;
			pm_lsc_crop->gain_width = 23;
			pm_lsc_crop->gain_height = 15;
			for (i = 0; i < 8; i++) {
				pm_lsc_full->input_table_buffer = cxt->std_init_lsc_table_param_buffer[i];
				pm_lsc_crop->output_table_buffer = cxt->std_lsc_table_param_buffer[i];
				rtn = lsc_pm_table_crop(pm_lsc_full, pm_lsc_crop, cxt->is_planar);
				fwstart_info->lsc_tab_address_new[i] = cxt->std_lsc_table_param_buffer[i];
			}
			lsc_std_free(pm_lsc_full);
			lsc_std_free(pm_lsc_crop);
		} else {
			ISP_LOGI("FW_START, lsc do not support img_size=[%d,%d], please check", fwstart_info->img_width_new, fwstart_info->img_height_new);
		}
	}

	return rtn;
}

static int lsc_malloc_buffer(struct lsc_sprd_ctrl_context *cxt)
{
	cmr_s32 rtn = LSC_SUCCESS;
	int i = 0;

	for (i = 0; i < 8; i++) {
		cxt->std_init_lsc_table_param_buffer[i] = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
		if (NULL == cxt->std_init_lsc_table_param_buffer[i]) {
			ISP_LOGE("fail to alloc std_init_lsc_table_param_buffer!");
			goto exit;
		}
		cxt->std_lsc_table_param_buffer[i] = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
		if (NULL == cxt->std_lsc_table_param_buffer[i]) {
			ISP_LOGE("fail to alloc std_lsc_table_param_buffer!");
			goto exit;
		}
	}

	cxt->dst_gain = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
	if (NULL == cxt->dst_gain) {
		ISP_LOGE("fail to alloc dst_gain!");
		goto exit;
	}

	cxt->lsc_buffer = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
	if (NULL == cxt->lsc_buffer) {
		ISP_LOGE("fail to alloc lsc_buffer!");
		goto exit;
	}

	cxt->fwstart_new_scaled_table = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
	if (NULL == cxt->fwstart_new_scaled_table) {
		ISP_LOGE("fail to alloc fwstart_new_scaled_table!");
		goto exit;
	}

	cxt->fwstop_output_table = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
	if (NULL == cxt->fwstop_output_table) {
		ISP_LOGE("fail to alloc fwstop_output_table!");
		goto exit;
	}

	cxt->flash_y_gain = (cmr_u16 *) malloc(32 * 32 * sizeof(cmr_u16));
	if (NULL == cxt->flash_y_gain) {
		ISP_LOGE("fail to alloc flash_y_gain!");
		goto exit;
	}

	cxt->lsc_last_info = (struct lsc_last_info *)malloc(sizeof(struct lsc_last_info));
	if (NULL == cxt->lsc_last_info) {
		ISP_LOGE("fail to alloc lsc_last_info!");
		goto exit;
	}

	cxt->ae_stat = (cmr_u32 *) malloc(MAX_STAT_WIDTH * MAX_STAT_HEIGHT * 3 * sizeof(cmr_u32));
	if (NULL == cxt->ae_stat) {
		ISP_LOGE("fail to alloc ae_stat!");
		goto exit;
	}

	cxt->lsc_flash_proc_param = (struct lsc_flash_proc_param *)malloc(sizeof(struct lsc_flash_proc_param));
	if (cxt->lsc_flash_proc_param == NULL) {
		ISP_LOGE("malloc lsc_flash_proc_param error!");
		goto exit;
	}

	cxt->last_lsc_table = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
	if (cxt->last_lsc_table == NULL) {
		ISP_LOGE("malloc last_lsc_table error!");
		goto exit;
	}

	cxt->output_lsc_table = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
	if (cxt->output_lsc_table == NULL) {
		ISP_LOGE("malloc output_lsc_table error!");
		goto exit;
	}

	cxt->lsc_buffer_interlace = (cmr_u16 *) malloc(MAX_WIDTH * MAX_HEIGHT * 4 * sizeof(cmr_u16));
	if (cxt->lsc_buffer_interlace == NULL) {
		ISP_LOGE("malloc lsc_buffer_interlace error!");
		goto exit;
	}

	return rtn;

exit:
	rtn = LSC_ALLOC_ERROR;
	return rtn;
}

static void *lsc_sprd_init(void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_sprd_ctrl_context *cxt = NULL;
	struct lsc_adv_init_param *init_param = (struct lsc_adv_init_param *)in;
	struct lsc_sprd_init_in sprd_init_param;
	void *alsc_handle = NULL;
	cmr_s32 free_otp_flag = 1;
	UNUSED(out);

	if (init_param->lsc_otp_table_addr != NULL) {
		ISP_LOGE("initparam lsc_otp_table_addr is not NULL, please check the caller");
		free_otp_flag = 0;
		goto EXIT;
	}

	//create all buffer
	cxt = (struct lsc_sprd_ctrl_context *)malloc(sizeof(struct lsc_sprd_ctrl_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to alloc lsc_sprd_ctrl_context!");
		goto EXIT;
	}
	memset(cxt, 0, sizeof(*cxt));

	rtn = lsc_malloc_buffer(cxt);
	if (LSC_ALLOC_ERROR == rtn) {
		goto EXIT;
	}

	lsc_get_prop_cmd(cxt);

	lsc_parser_otp(init_param, cxt);

	// init flash ctrl param
	lsc_flash_proc_init(cxt);

	// get lsc_id
	if (id1_addr == NULL) {
		id1_addr = cxt->lsc_buffer;
		cxt->lsc_id = 1;
	} else {
		id2_addr = cxt->lsc_buffer;
		cxt->lsc_id = 2;
	}
	init_param->lsc_buffer_addr = cxt->lsc_buffer;

	//load sprd lib
	cxt->lib_info = &init_param->lib_param;
	rtn = lsc_load_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to load lsc lib");
		goto EXIT;
	}
	// alsc_init
	cxt->is_planar = 1;	// 1 -- use planar lsc table in alsc algorithm;  0 -- use interlace lsc table in alsc algorithm
	rtn = lsc_set_init_param(init_param, cxt, &sprd_init_param);

	alsc_handle = cxt->lib_ops.alsc_init(&sprd_init_param);
	if (NULL == alsc_handle) {
		ISP_LOGE("fail to do alsc init!");
		rtn = LSC_ALLOC_ERROR;
		goto EXIT;
	}
	cxt->alsc_handle = alsc_handle;

	// get otp normalized  lsc table from lib
	cxt->lib_ops.alsc_io_ctrl(alsc_handle, LSC_CMD_GET_OTP_STD_TABLE, cxt->std_init_lsc_table_param_buffer, NULL);

	lsc_std_free(init_param->lsc_otp_table_addr);

	pthread_mutex_init(&cxt->status_lock, NULL);

	ISP_LOGI("lsc init success rtn = %d", rtn);
	return (void *)cxt;

EXIT:
	if ((init_param->lsc_otp_table_addr != NULL) && (free_otp_flag == 1)) {
		ISP_LOGW("error happend free lsc_otp_table_addr:%p", init_param->lsc_otp_table_addr);
		free(init_param->lsc_otp_table_addr);
	}

	if (NULL != cxt) {
		rtn = lsc_unload_lib(cxt);
		free(cxt);
		cxt = NULL;
	}

	ISP_LOGI("done rtn = %d", rtn);

	return NULL;
}

static cmr_s32 lsc_sprd_calculation(void *handle, void *in, void *out)
{
	cmr_u32 i;
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_sprd_ctrl_context *cxt = (struct lsc_sprd_ctrl_context *)handle;
	struct lsc_adv_calc_param *param = (struct lsc_adv_calc_param *)in;
	struct lsc_adv_calc_result *result = (struct lsc_adv_calc_result *)out;
	cmr_u32 alg_in = 0;
	cmr_u32 img_width, img_height, gain_width, gain_height, grid;
	cmr_u32 IIR_weight = 5;
	cmr_u32 *stat_g = NULL;
	struct lsc_sprd_calc_in calc_in;
	struct lsc_sprd_calc_out calc_out;
	struct lsc_last_info *lsc_last_info = NULL;
	struct lsc_post_shading_param post_gain_param;
	int debug_index[4];

	if (!handle || !in || !out) {
		ISP_LOGE("fail to check param is NULL!");
		return LSC_ERROR;
	}

	if (cxt->lsc_id == 2 /*slave */  && cxt->is_multi_mode == 2 /*ISP_DUAL_SBS */  && cxt->is_master == 0 /*slave */ ) {
		ISP_LOGV("return due to ISP_DUAL_SBS, slave, lsc_id=%d, frame_count=%d", cxt->lsc_id, cxt->frame_count);
		return rtn;
	}

	IIR_weight = cxt->IIR_weight;
	lsc_last_info = (struct lsc_last_info *)cxt->lsc_last_info;
	lsc_last_info->bv = param->bv;
	lsc_last_info->bv_gain = param->bv_gain;
	img_width = param->img_size.w;
	img_height = param->img_size.h;
	gain_width = param->gain_width;
	gain_height = param->gain_height;
	grid = param->grid;
	result->dst_gain = cxt->dst_gain;

	// lsc param normal
	if (cxt->LSC_SPD_VERSION >= 6) {
		gain_width = cxt->gain_width;
		gain_height = cxt->gain_height;
		grid = cxt->grid;
	}

	ATRACE_BEGIN(__FUNCTION__);
	cmr_u64 ae_time0 = systemTime(CLOCK_MONOTONIC);

	// alsc_calc ++
	// change mode
	if (cxt->fw_start_end) {
		if (cxt->LSC_SPD_VERSION <= 5) {
			if (cxt->lsc_pm0 != param->lsc_tab_address[0]) {
				ISP_LOGV("change mode: start frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
				ISP_LOGV("change mode: pre img[%d,%d], table[%d,%d,%d]", cxt->img_width, cxt->img_height, cxt->gain_width, cxt->gain_height, cxt->grid);
				ISP_LOGV("change mode: new img[%d,%d], table[%d,%d,%d]", img_width, img_height, gain_width, gain_height, grid);

				if (gain_width != cxt->gain_width || gain_height != cxt->gain_height || cxt->flash_mode == 1 || cxt->pre_flash_mode == 1 || cxt->frame_count == 0) {
					cxt->img_width = img_width;
					cxt->img_height = img_height;
					cxt->gain_width = gain_width;
					cxt->gain_height = gain_height;
					cxt->grid = grid;
					cxt->lsc_pm0 = param->lsc_tab_address[0];
					memcpy(cxt->last_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
					memcpy(cxt->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
					memcpy(cxt->lsc_buffer, cxt->output_lsc_table, gain_width * gain_height * 4 * sizeof(unsigned short));
					// do lsc param preprocess without otp
					if (cxt->is_planar == 1) {
						for (i = 0; i < 8; i++) {
							memcpy(cxt->std_lsc_table_param_buffer[i], param->lsc_tab_address[i], gain_width * gain_height * 4 * sizeof(cmr_u16));
							lsc_table_interlace2planar(cxt->std_lsc_table_param_buffer[i], gain_width, gain_height, cxt->gain_pattern,
										   cxt->output_gain_pattern);
						}
					} else {
						for (i = 0; i < 8; i++) {
							memcpy(cxt->std_lsc_table_param_buffer[i], param->lsc_tab_address[i], gain_width * gain_height * 4 * sizeof(cmr_u16));
							lsc_interlace_change_pattern(cxt->std_lsc_table_param_buffer[i], gain_width, gain_height, cxt->gain_pattern,
										     cxt->output_gain_pattern);
						}
					}
					//sync_g_channel(cxt->std_lsc_table_param_buffer, gain_width, gain_height, cxt->output_gain_pattern);
				}

				cxt->fw_start_end = 0;
				cxt->can_update_dest = 1;
				if (cxt->frame_count == 0) {	//lunch camera with binning size or 720p both will run the fw_start
					cxt->frame_count = 1;
					cxt->alg_count = 0;	// set zero to skip iir
					ISP_LOGV("change mode END: set frame_count=1, set alg_count=0 to do quick in");
					return rtn;
				} else {
					cxt->frame_count = cxt->calc_freq * 3 - 2;	// do not quick in and skip 3 frame for AEM stable
					cxt->alg_count = 0;	// set zero to skip iir
					ISP_LOGV("change mode END: set ALSC normal in");
					return rtn;
				}
			} else {
				cxt->fw_start_end = 0;
				cxt->can_update_dest = 1;
				ISP_LOGV("change mode SKIP: protect for previous calc thread");
				return rtn;
			}
		} else {
			// when cxt->LSC_SPD_VERSION >=6
			ISP_LOGV("change mode: pre_lsc_pm_mode=%d, cur_lsc_pm_mode=%d", cxt->pre_lsc_pm_mode, cxt->cur_lsc_pm_mode);

			cxt->pre_lsc_pm_mode = cxt->cur_lsc_pm_mode;
			cxt->img_width = img_width;
			cxt->img_height = img_height;
			memcpy(cxt->last_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
			memcpy(cxt->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));

			cxt->fw_start_end = 0;
			cxt->can_update_dest = 1;
			if (cxt->frame_count == 0) {	//lunch camera with binning size or 720p both will run the fw_start
				cxt->frame_count = 1;
				cxt->alg_count = 0;	// set zero to skip iir
				ISP_LOGV("change mode END (return 0): set frame_count=1, set alg_count=1 to do quick in");
				return rtn;
			} else {
				cxt->frame_count = cxt->calc_freq * 3 - 2;	// do not quick in and skip 3 frame for AEM stable
				cxt->alg_count = 0;	// set zero to skip iir
				ISP_LOGV("change mode END (return 0): set ALSC normal in, frame_count=%d", cxt->frame_count);
				return rtn;
			}

		}
	}
	// first frame just copy the output from fw_start
	if (cxt->frame_count == 0) {	//lunch camera with fullsize
		memcpy(cxt->last_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
		memcpy(cxt->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
		memcpy(cxt->lsc_buffer, cxt->output_lsc_table, gain_width * gain_height * 4 * sizeof(unsigned short));
		alg_in = 0;
	} else {
		alg_in = lsc_get_alg_in_flag(cxt, &IIR_weight);
	}

	if (cxt->cmd_alsc_bypass) {
		alg_in = 0;
	}

	ISP_LOGD("frame_count=%d, alg_in=%d, lsc_id=%d, alg_count = %d, can_update_dest=%d", cxt->frame_count, alg_in, cxt->lsc_id, cxt->alg_count, cxt->can_update_dest);

	debug_index[0] = 0 * gain_width * gain_height;
	debug_index[1] = 1 * gain_width * gain_height;
	debug_index[2] = 2 * gain_width * gain_height;
	debug_index[3] = 3 * gain_width * gain_height;

	// alsc calculation process
	if (alg_in && cxt->alg_bypass == 0) {
		// select std lsc param
		if (cxt->init_gain_width == gain_width && cxt->init_gain_height == gain_height) {
			for (i = 0; i < 8; i++)
				param->std_tab_param[i] = cxt->std_init_lsc_table_param_buffer[i];	// use init lsc table with otp
		} else {
			for (i = 0; i < 8; i++)
				param->std_tab_param[i] = cxt->std_lsc_table_param_buffer[i];	// use calculation in lsc table without otp
		}

		// scale AEM size to 32x32
		lsc_scl_for_ae_stat(cxt, param);

		// cmd dump AEM0
		if (cxt->cmd_alsc_dump_aem) {
			lsc_dump_stat(img_width, img_height, param->stat_img.r, param->stat_img.gr, param->stat_img.b, cxt->lsc_id, cxt->frame_count, 0);
		}
		// stat_g = 0 return
		stat_g = param->stat_img.gr;
		for (i = 0; i < MAX_STAT_WIDTH * MAX_STAT_HEIGHT; i++) {
			if (stat_g[i] == 0) {
				ISP_LOGV("stat_g = 0 return");
				return rtn;
			}
		}

		// inverse the LSC on AEM
		lsc_inverse_ae_stat(cxt, cxt->output_lsc_table);

		// cmd dump AEM1
		if (cxt->cmd_alsc_dump_aem) {
			lsc_dump_stat(img_width, img_height, param->stat_img.r, param->stat_img.gr, param->stat_img.b, cxt->lsc_id, cxt->frame_count, 1);
		}
		// call liblsc.so
		calc_in.stat_img.r = param->stat_img.r;
		calc_in.stat_img.gr = param->stat_img.gr;
		calc_in.stat_img.gb = param->stat_img.gb;
		calc_in.stat_img.b = param->stat_img.b;
		calc_in.stat_img.w = param->stat_size.w;
		calc_in.stat_img.h = param->stat_size.h;
		calc_in.gain_height = param->gain_height;
		calc_in.gain_width = param->gain_width;
		calc_in.grid = param->grid;
		calc_in.img_height = param->img_size.h;
		calc_in.img_width = param->img_size.w;
		for (i = 0; i < 8; i++)
			calc_in.lsc_tab[i] = param->std_tab_param[i];
		memcpy(calc_in.last_lsc_table, cxt->last_lsc_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
		calc_out.dst_gain = result->dst_gain;

		rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, &calc_in, &calc_out);

		// table smooth process
		if (cxt->alg_count) {
			if (cxt->can_update_dest) {
				for (i = 0; i < gain_width * gain_height * 4; i++) {
					cxt->last_lsc_table[i] = (cmr_u16) ((IIR_weight * result->dst_gain[i] + (16 - IIR_weight) * cxt->last_lsc_table[i]) / 16.0f);
				}
			}
		} else {
			memcpy(cxt->last_lsc_table, result->dst_gain, gain_width * gain_height * 4 * sizeof(cmr_u16));
		}
		cxt->alg_count++;
	}
	// replace bv and bv_gain after flash
	cxt->bv_memory[cxt->frame_count % 10] = param->bv;
	cxt->bv_gain_memory[cxt->frame_count % 10] = param->bv_gain;
	if (cxt->bv_skip_frame) {
		cxt->bv_skip_frame--;
		ISP_LOGV("flash end in bv_skip_frame, replace param->bv=%d to bv=%d, param->bv_gain=%d to bv_gain=%d, bv_skip_frame=%d",
			 param->bv, cxt->bv_before_flash, param->bv_gain, cxt->bv_gain_before_flash, cxt->bv_skip_frame);
		param->bv = cxt->bv_before_flash;
		param->bv_gain = cxt->bv_gain_before_flash;
	}

	// do post shading gain
	if (!cxt->cmd_alsc_bypass) {
		if (cxt->alg_count <= cxt->alg_quick_in_frame + 1 && cxt->frame_count < cxt->calc_freq * 3 + 1 && cxt->can_update_dest == 1 && param->bv * cxt->fw_start_bv > 0) {
			memcpy(cxt->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(unsigned short));
			memcpy(cxt->lsc_buffer, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(unsigned short));
			ISP_LOGV("bv=%d, fw_start_bv=%d , use fwstart_new_scaled_table", param->bv, cxt->fw_start_bv);
		} else {
			if (cxt->can_update_dest == 1 && alg_in) {
				post_gain_param.org_gain = cxt->last_lsc_table;
				post_gain_param.gain_width = gain_width;
				post_gain_param.gain_height = gain_height;
				post_gain_param.gain_pattern = cxt->output_gain_pattern;
				post_gain_param.frame_count = cxt->frame_count;
				post_gain_param.bv = param->bv;
				post_gain_param.bv_gain = param->bv_gain;
				post_gain_param.flash_mode = cxt->flash_mode;
				post_gain_param.pre_flash_mode = cxt->pre_flash_mode;
				post_gain_param.LSC_SPD_VERSION = cxt->LSC_SPD_VERSION;
				post_gain_param.tuning_param = NULL;

				cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_DO_POSTPROCESS, &post_gain_param, cxt->output_lsc_table);

				memcpy(cxt->lsc_buffer, cxt->output_lsc_table, gain_width * gain_height * 4 * sizeof(unsigned short));
			} else {
				ISP_LOGV("post_gain SKIP, Not Update dst_gain due to just fw_start");
			}
		}
	} else {
		ISP_LOGV("cmd_alsc_bypass = 1");
	}
	// alsc_calc --

	cxt->flash_done_frame_count++;
	cxt->frame_count++;
	cxt->alsc_update_flag = 1;
	if (!cxt->can_update_dest)
		cxt->alsc_update_flag = 0;

	// cmd set lsc output
	if (cxt->cmd_alsc_table_pattern) {
		lsc_cmd_set_output(cxt->lsc_buffer, gain_width, gain_height, cxt->output_gain_pattern);
		if(cxt->is_planar == 1){
			lsc_table_interlace2planar(cxt->lsc_buffer, gain_width, gain_height, cxt->output_gain_pattern, cxt->output_gain_pattern);
		}
		ISP_LOGD("lsc_cmd_set_output, final output cxt->lsc_buffer[%d,%d,%d,%d]",
			cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);
		return rtn;
	}
	// cmd set table index
	if (cxt->cmd_alsc_table_index < 8 && cxt->cmd_alsc_table_index >= 0) {
		if (cxt->LSC_SPD_VERSION <= 5) {
			memcpy(cxt->lsc_buffer, cxt->std_lsc_table_param_buffer[cxt->cmd_alsc_table_index], gain_width * gain_height * 4 * sizeof(cmr_u16));
		} else {
			memcpy(cxt->lsc_buffer, cxt->std_init_lsc_table_param_buffer[cxt->cmd_alsc_table_index], gain_width * gain_height * 4 * sizeof(cmr_u16));
		}
		ISP_LOGD("cmd set table index %d, final output cxt->lsc_buffer[%d,%d,%d,%d]", cxt->cmd_alsc_table_index,
			 cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);
		return rtn;
	}

	ISP_LOGV("final output [%d,%d,%d,%d]", cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);

	if (cxt->cmd_alsc_dump_table) {
		lsc_dump_gain(cxt->lsc_buffer, gain_width, gain_height, cxt->output_gain_pattern, cxt->is_planar, cxt->frame_count);
	}

	cmr_u64 ae_time1 = systemTime(CLOCK_MONOTONIC);
	ATRACE_END();
	ISP_LOGV("SYSTEM_TEST -lsc_test  %dus ", (cmr_s32) ((ae_time1 - ae_time0) / 1000));
	return rtn;
}

static cmr_s32 lsc_sprd_ioctrl(void *handle, cmr_s32 cmd, void *in, void *out)
{
	cmr_u32 i;
	cmr_s32 change_mode_rtn = 0;
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_sprd_ctrl_context *cxt = NULL;

	if (!handle) {
		ISP_LOGE("fail to check param, param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_sprd_ctrl_context *)handle;
	struct tg_alsc_debug_info *ptr_out = NULL;
	struct alsc_flash_info *flash_info = NULL;
	struct alsc_ver_info *alsc_ver_info_out = NULL;
	struct alsc_update_info *alsc_update_info_out = NULL;
	struct alsc_fwstart_info *fwstart_info = NULL;
	struct alsc_fwprocstart_info *fwprocstart_info = NULL;
	struct lsc_last_info *lsc_last_info = (struct lsc_last_info *)cxt->lsc_last_info;
	struct lsc_flash_proc_param *flash_param = (struct lsc_flash_proc_param *)cxt->lsc_flash_proc_param;
	struct alsc_do_simulation *alsc_do_simulation = NULL;
	struct lsc_sprd_calc_in *lsc_calc_in = NULL;
	struct lsc_sprd_calc_out *lsc_calc_out = NULL;
	struct lsc_post_shading_param post_gain_param;
	cmr_u32 *tmp_buffer_r = NULL;
	cmr_u32 *tmp_buffer_g = NULL;
	cmr_u32 *tmp_buffer_b = NULL;
	cmr_u16 *tmp_buffer = NULL;
	//cmr_u16 *pm0_new = NULL;
	cmr_u8 is_gr, is_gb, is_r, is_b;
	cmr_u32 full_flag = 0;
	cmr_u32 chnl_gain_num = 0;
	cmr_u32 will_do_post_gain = 0;

	lsc_get_channel_index(cxt->output_gain_pattern, &is_gr, &is_r, &is_b, &is_gb);

	switch (cmd) {
	case ALSC_FW_START:	// You have to update two table in FW_START: 1.fwstart_info->lsc_result_address_new, 2.cxt->fwstart_new_scaled_table
		fwstart_info = (struct alsc_fwstart_info *)in;
		ISP_LOGV("FW_START +++++, frame_count=%d, camera_id=%d, lsc_id=%d, LSC_SPD_VERSION=%d", cxt->frame_count, fwstart_info->camera_id, cxt->lsc_id, cxt->LSC_SPD_VERSION);
		ISP_LOGV("FW_START, old tab0 address = %p, (%d,%d), grid %d", cxt->lsc_pm0, cxt->gain_width, cxt->gain_height, cxt->grid);
		ISP_LOGV("FW_START, new tab0 address = %p, (%d,%d), grid %d", fwstart_info->lsc_tab_address_new[0],
			 fwstart_info->gain_width_new, fwstart_info->gain_height_new, fwstart_info->grid_new);

		// read last info
		if (cxt->init_img_width == fwstart_info->img_width_new && cxt->init_img_height == fwstart_info->img_height_new)
			full_flag = 1;
		if (cxt->lsc_id == 1) {
			lsc_read_last_info(lsc_last_info, fwstart_info->camera_id, full_flag);
			cxt->fw_start_bv = lsc_last_info->bv;
			cxt->fw_start_bv_gain = lsc_last_info->bv_gain;
		}

		// main ctrl flow when fw start +++++
		if (cxt->LSC_SPD_VERSION <= 5) {
			chnl_gain_num = fwstart_info->gain_width_new * fwstart_info->gain_height_new;

			if (fwstart_info->gain_width_new == 23 && fwstart_info->gain_height_new == 15 && (fwstart_info->grid_new == 32 || fwstart_info->grid_new == 48)) {
				ISP_LOGV("FW_START, 720p or 1080p Mode, Send TL84 table");
				memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[DEFAULT_TAB_INDEX], chnl_gain_num * 4 * sizeof(cmr_u16));
				lsc_table_interlace2planar(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->gain_pattern, cxt->output_gain_pattern);
				will_do_post_gain = 1;
			} else if (cxt->frame_count == 0) {
				ISP_LOGV("FW_START, First Frame Mod, apply lsc_last_info");
				if (cxt->lsc_id == 1 && fwstart_info->gain_width_new == lsc_last_info->gain_width && fwstart_info->gain_height_new == lsc_last_info->gain_height) {
					memcpy(fwstart_info->lsc_result_address_new, lsc_last_info->table,
					       lsc_last_info->gain_height * lsc_last_info->gain_width * 4 * sizeof(cmr_u16));
				} else {
					ISP_LOGV("FW_START, Send TL84 table");
					memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[DEFAULT_TAB_INDEX], chnl_gain_num * 4 * sizeof(cmr_u16));
					lsc_table_interlace2planar(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->gain_pattern, cxt->output_gain_pattern);
					will_do_post_gain = 1;
				}
			} else {
				// case for old parameter file the same with new parameter file
				if (cxt->lsc_pm0 == fwstart_info->lsc_tab_address_new[0]) {
					ISP_LOGV("FW_START, The same parameter file Mode, COPY fwstop_output_table for output");
					memcpy(fwstart_info->lsc_result_address_new, cxt->fwstop_output_table, chnl_gain_num * 4 * sizeof(cmr_u16));
					// flash change mode with same param file
				} else if (flash_param->main_flash_from_other_parameter == 1
					   && flash_param->preflash_current_lnc_table_address == fwstart_info->lsc_tab_address_new[0]) {
					ISP_LOGV("FW_START, Flash change mode, the same param file");
					memcpy(fwstart_info->lsc_result_address_new, flash_param->preflash_current_output_table, chnl_gain_num * 4 * sizeof(cmr_u16));
					flash_param->main_flash_from_other_parameter = 0;
					flash_param->preflash_current_lnc_table_address = NULL;
					// normal change mode & flash change mode, with different param file
				} else {
					ISP_LOGV("FW_START, Normal change mode, prepare to update first tab");
					change_mode_rtn = lsc_fwstart_update_first_tab(cxt, fwstart_info, cxt->is_planar);
				}

				if (change_mode_rtn == -1)
					ISP_LOGV("FW_START, Change Mode Failed");
			}
		} else {
			// when cxt->LSC_SPD_VERSION >= 6
			lsc_preprocess_fwstart_info(cxt, fwstart_info);

			chnl_gain_num = cxt->gain_width * cxt->gain_height;

			if (cxt->init_img_width / fwstart_info->img_width_new == cxt->init_img_height / fwstart_info->img_height_new) {
				cxt->cur_lsc_pm_mode = 0;	// common table size
			} else if (fwstart_info->img_width_new == 1280 && fwstart_info->img_height_new == 720) {
				cxt->cur_lsc_pm_mode = 1;	// 720p table size
			} else if (fwstart_info->img_width_new == 1920 && fwstart_info->img_height_new == 1080) {
				cxt->cur_lsc_pm_mode = 1;	// 1080p table size
			}

			ISP_LOGI("FW_START, new table size=[%d,%d] grid=%d", cxt->gain_width, cxt->gain_height, cxt->grid);
			ISP_LOGI("FW_START, pre_lsc_pm_mode=%d, cur_lsc_pm_mode=%d", cxt->pre_lsc_pm_mode, cxt->cur_lsc_pm_mode);

			// change to 720p mode
			if (fwstart_info->img_width_new == 1280 && fwstart_info->img_height_new == 720) {
				ISP_LOGV("FW_START, 720p Mode");
				memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], chnl_gain_num * 4 * sizeof(cmr_u16));
			}else if (fwstart_info->img_width_new == 1920 && fwstart_info->img_height_new == 1080) {
				ISP_LOGV("FW_START, 1080p Mode");
				memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], chnl_gain_num * 4 * sizeof(cmr_u16));
			} else if (cxt->frame_count == 0) {
				if (cxt->lsc_id == 1 && cxt->gain_width == lsc_last_info->gain_width && cxt->gain_height == lsc_last_info->gain_height) {
					memcpy(fwstart_info->lsc_result_address_new, lsc_last_info->table, chnl_gain_num * 4 * sizeof(cmr_u16));
				} else {
					ISP_LOGV("FW_START, no last info, Send TL84 table");
					memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], chnl_gain_num * 4 * sizeof(cmr_u16));
					will_do_post_gain = 1;
				}
			} else {
				ISP_LOGV("FW_START, Change Mode");
				// case for the same table size, then we just copy the output_lsc_table, and calc do nothing.
				if (cxt->pre_lsc_pm_mode == cxt->cur_lsc_pm_mode && flash_param->main_flash_from_other_parameter != 1) {
					ISP_LOGV("FW_START, The same table size mode, COPY fwstop_output_table for output");
					memcpy(fwstart_info->lsc_result_address_new, cxt->fwstop_output_table, chnl_gain_num * 4 * sizeof(cmr_u16));
					// flash change mode with same param file
				} else if (cxt->pre_lsc_pm_mode == cxt->cur_lsc_pm_mode && flash_param->main_flash_from_other_parameter == 1) {
					ISP_LOGV("FW_START, Flash change mode, the same table size, don't need scale, COPY the preflash_current_output_table");
					memcpy(fwstart_info->lsc_result_address_new, flash_param->preflash_current_output_table, chnl_gain_num * 4 * sizeof(cmr_u16));
					flash_param->main_flash_from_other_parameter = 0;
					flash_param->preflash_current_lnc_table_address = NULL;
					// non common size mode change back to common size mode
				} else {
					if (cxt->cur_lsc_pm_mode == 0) {
						ISP_LOGV("FW_START, non common size mode change back to common size mode, use last_info");
						if (cxt->lsc_id == 1 && cxt->gain_width == lsc_last_info->gain_width && cxt->gain_height == lsc_last_info->gain_height) {
							memcpy(fwstart_info->lsc_result_address_new, lsc_last_info->table,
							       cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));
						} else {
							ISP_LOGV("FW_START, no last info, Send TL84 table");
							memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], chnl_gain_num * 4 * sizeof(cmr_u16));
							will_do_post_gain = 1;
						}
					}
				}
			}
		}
		// main ctrl flow when fw start -----

		lsc_gen_flash_y_gain(cxt->flash_y_gain, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->flash_enhance_ratio,
				cxt->flash_center_shiftx, cxt->flash_center_shifty);

		if (will_do_post_gain == 1) {
			post_gain_param.org_gain = fwstart_info->lsc_result_address_new;
			post_gain_param.gain_width = fwstart_info->gain_width_new;
			post_gain_param.gain_height = fwstart_info->gain_height_new;
			post_gain_param.gain_pattern = cxt->output_gain_pattern;
			post_gain_param.frame_count = cxt->frame_count;
			post_gain_param.bv = cxt->fw_start_bv;
			post_gain_param.bv_gain = cxt->fw_start_bv_gain;
			post_gain_param.flash_mode = 0;
			post_gain_param.pre_flash_mode = 0;
			post_gain_param.LSC_SPD_VERSION = cxt->LSC_SPD_VERSION;
			post_gain_param.tuning_param = NULL;
			cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_DO_POSTPROCESS, &post_gain_param, fwstart_info->lsc_result_address_new);
		}
		memcpy(cxt->fwstart_new_scaled_table, fwstart_info->lsc_result_address_new, chnl_gain_num * 4 * sizeof(cmr_u16));
		//planar2interlace for driver use
		if (cxt->is_planar == 1) {
			lsc_table_planar2interlace(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new,
				cxt->output_gain_pattern, cxt->output_gain_pattern);
		}
		ISP_LOGV("FW_START -----, cxt->can_update_dest=%d", cxt->can_update_dest);
		break;

	case ALSC_FW_START_END:
		fwstart_info = (struct alsc_fwstart_info *)in;
		ISP_LOGV("FW_START_END +++++, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		ISP_LOGV("FW_START_END, old tab address = %p, new tab address = %p", cxt->lsc_pm0, fwstart_info->lsc_tab_address_new[0]);

		if (cxt->LSC_SPD_VERSION <= 5) {
			//case for old parameter file the same with new parameter file, then we just copy the output_lsc_table, and calc do nothing.
			if (cxt->lsc_pm0 == fwstart_info->lsc_tab_address_new[0]) {
				//reset the flag
				cxt->fw_start_end = 0;	//set 0, we hope calc don't perform change mode.
				cxt->can_update_dest = 1;	//set 1, then the calc can keep update the destination buffer and post gain.
			} else {
				cxt->fw_start_end = 1;	// calc will perforem the change mode
			}
		} else {
			if (cxt->pre_lsc_pm_mode == cxt->cur_lsc_pm_mode) {
				//reset the flag
				cxt->fw_start_end = 1;	//set 1, we hope skip 3 frames when change raw size
				cxt->can_update_dest = 1;	//set 1, then the calc can keep update the destination buffer and post gain.
			} else {
				cxt->fw_start_end = 1;	// calc will perforem the change mode
			}
		}

		ISP_LOGV("FW_START_END -----, fw_start_end=%d, can_update_dest=%d", cxt->fw_start_end, cxt->can_update_dest);
		break;

	case ALSC_FW_STOP:
		ISP_LOGV("FW_STOP +++++, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);

		//copy the output to last_info
		chnl_gain_num = cxt->gain_width * cxt->gain_height;
		if (!(cxt->gain_width == 23 && cxt->gain_height == 15 && (cxt->grid == 32 || cxt->grid == 48))&& cxt->lsc_id == 1) {
			memcpy(lsc_last_info->table, cxt->output_lsc_table, chnl_gain_num * 4 * sizeof(cmr_u16));
			lsc_last_info->gain_width = cxt->gain_width;
			lsc_last_info->gain_height = cxt->gain_height;
		}
		if (cxt->gain_width == cxt->init_gain_width && cxt->gain_height == cxt->init_gain_height && cxt->grid == cxt->init_grid)
			full_flag = 1;
		if (cxt->lsc_id == 1) {
			lsc_save_last_info(lsc_last_info, cxt->camera_id, full_flag);
		}
		// let calc not to update the result->dst_gain
		cxt->can_update_dest = 0;
		cxt->alsc_update_flag = 0;
		ISP_LOGV("FW_STOP can_update_dest=%d, alsc_update_flag=%d, is_touch_preflash=%d", cxt->can_update_dest, cxt->alsc_update_flag, flash_param->is_touch_preflash);

		// flash mode
		if (flash_param->is_touch_preflash == 0 || flash_param->is_touch_preflash == 1) {
			memcpy(cxt->fwstop_output_table, flash_param->preflash_guessing_mainflash_output_table, chnl_gain_num * 4 * sizeof(cmr_u16));
			// normal mode
		} else {
			memcpy(cxt->fwstop_output_table, cxt->output_lsc_table, chnl_gain_num * 4 * sizeof(unsigned short));
		}

		flash_param->is_touch_preflash = -99;

		// flash capture end, prepare to back to preview, if the address not equal, means we take flash shot on different parameter file between the preflash one.
		if (cxt->flash_mode == 1 && flash_param->preflash_current_lnc_table_address != cxt->lsc_pm0 && flash_param->preflash_current_lnc_table_address != NULL) {
			// set main_flash_from_other_parameter will let the next fwstart_update_first_tab to select preflash_current_output_table when back to preview
			flash_param->main_flash_from_other_parameter = 1;
		} else {
			flash_param->main_flash_from_other_parameter = 0;
		}

		ISP_LOGV("FW_STOP -----, main_flash_from_other_parameter=%d", flash_param->main_flash_from_other_parameter);
		break;

	case ALSC_CMD_GET_DEBUG_INFO:
		ptr_out = (struct tg_alsc_debug_info *)out;
		cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_GET_DEBUG_INFO, ptr_out, NULL);
		break;

	case LSC_INFO_TO_AWB:
		break;

	case ALSC_GET_VER:
		alsc_ver_info_out = (struct alsc_ver_info *)out;
		alsc_ver_info_out->LSC_SPD_VERSION = cxt->LSC_SPD_VERSION;
		break;

	case ALSC_FLASH_PRE_BEFORE:
		if (cxt->lsc_buffer == NULL || cxt->frame_count == 0) {
			ISP_LOGV("FLASH_PRE_BEFORE, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
			return rtn;
		}
		// for quick in
		cxt->pre_flash_mode = 1;
		cxt->alg_quick_in = 1;
		cxt->quik_in_start_frame = -99;
		ISP_LOGV("FLASH_PRE_BEFORE, flash_done_frame_count=%d, frame_count=%d", cxt->flash_done_frame_count, cxt->frame_count);

		// log bv_before_flash
		if (cxt->flash_done_frame_count > 50) {
			cxt->bv_before_flash = 1600;
			cxt->bv_gain_before_flash = 1280;
			for (i = 0; i < 10; i++) {
				cxt->bv_before_flash = cxt->bv_memory[i] < cxt->bv_before_flash ? cxt->bv_memory[i] : cxt->bv_before_flash;
				cxt->bv_gain_before_flash = cxt->bv_gain_memory[i] > cxt->bv_gain_before_flash ? cxt->bv_gain_memory[i] : cxt->bv_gain_before_flash;
			}
		}
		// for handle touch preflash
		flash_param->pre_flash_before_ae_touch_framecount = flash_param->ae_touch_framecount;
		flash_param->pre_flash_before_framecount = cxt->frame_count;

		if (abs(flash_param->pre_flash_before_ae_touch_framecount - flash_param->pre_flash_before_framecount) < 3)
			// the preflash is caused by touch
			flash_param->is_touch_preflash = 1;
		else
			// the preflash is caused by normal flash capture
			flash_param->is_touch_preflash = 0;

		ISP_LOGV("FLASH_PRE_BEFORE, pre_flash_before_ae_touch_framecount=%d, pre_flash_before_framecount=%d, is_touch_preflash=%d",
			 flash_param->pre_flash_before_ae_touch_framecount, flash_param->pre_flash_before_framecount, flash_param->is_touch_preflash);

		//for change mode flash (capture flashing -> preview ) (with post gain)
		//copy the previous table, to restore back when flash off (with post gain)
		memcpy(flash_param->preflash_current_output_table, cxt->output_lsc_table, cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));

		// copy current DNP table
		memcpy(flash_param->preflash_current_lnc_table, cxt->lsc_pm0, cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));

		// log the current DNP table address (preview mode)
		flash_param->preflash_current_lnc_table_address = cxt->lsc_pm0;

		flash_param->main_flash_from_other_parameter = 0;

		//for calc to do the inverse (without post gain)
		memcpy(cxt->output_lsc_table, cxt->last_lsc_table, cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));

		//overwrite the dest buffer with one table without post-gain. not really helpful, but give a try.
		memcpy(cxt->lsc_buffer, cxt->last_lsc_table, cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));
		ISP_LOGV("FLASH_PRE_BEFORE END");
		break;

	case ALSC_FLASH_PRE_AFTER:
		if (cxt->lsc_buffer == NULL || cxt->frame_count == 0) {
			ISP_LOGV("FLASH_PRE_AFTER, frame_count=0, return 0 do nothing");
			return rtn;
		}
		cxt->pre_flash_mode = 0;

		flash_info = (struct alsc_flash_info *)in;
		flash_param->captureFlashEnvRatio = flash_info->io_captureFlashEnvRatio;
		flash_param->captureFlash1ofALLRatio = flash_info->io_captureFlash1Ratio;
		ISP_LOGV("FLASH_PRE_AFTER, captureFlashEnvRatio=%f, captureFlash1ofALLRatio=%f, frame_count=%d",
			 flash_param->captureFlashEnvRatio, flash_param->captureFlash1ofALLRatio, cxt->frame_count);

		// obtain the mainflash guessing table, use the output from preflash (without post gain)
		memcpy(flash_param->preflash_guessing_mainflash_output_table, cxt->output_lsc_table, cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));

		// reset
		flash_param->ae_touch_framecount = -99;
		flash_param->pre_flash_before_ae_touch_framecount = -99;
		flash_param->pre_flash_before_framecount = -99;
		break;

	case ALSC_FLASH_MAIN_LIGHTING:
		ISP_LOGV("FLASH_MAIN_LIGHTING, Do nothing, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

	case ALSC_FLASH_PRE_LIGHTING:
		ISP_LOGV("FLASH_PRE_LIGHTING, Do nothing, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

	case ALSC_FLASH_MAIN_BEFORE:
		if (cxt->lsc_buffer == NULL || cxt->frame_count == 0) {
			ISP_LOGV("FLASH_MAIN_BEFORE, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
			return rtn;
		}
		// stop calc update the dest buffer
		cxt->can_update_dest = 0;
		cxt->alsc_update_flag = 0;

		// apply the guessing table
		if (cxt->flash_y_gain[0] == 0) {
			// apply the guessing table
			memcpy(cxt->lsc_buffer, flash_param->preflash_guessing_mainflash_output_table, cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));
		} else {
			// apply the guessing table and flash_y_gain
			ISP_LOGV("flash lsc apply flash_y_gain");
			if (cxt->is_planar) {
				chnl_gain_num = cxt->gain_width * cxt->gain_height;
				for (i = 0; i < cxt->gain_width * cxt->gain_height; i++) {
					cxt->lsc_buffer[i + 0 * chnl_gain_num] =
					    flash_param->preflash_guessing_mainflash_output_table[i + 0 * chnl_gain_num] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[i + 1 * chnl_gain_num] =
					    flash_param->preflash_guessing_mainflash_output_table[i + 1 * chnl_gain_num] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[i + 2 * chnl_gain_num] =
					    flash_param->preflash_guessing_mainflash_output_table[i + 2 * chnl_gain_num] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[i + 3 * chnl_gain_num] =
					    flash_param->preflash_guessing_mainflash_output_table[i + 3 * chnl_gain_num] * cxt->flash_y_gain[i] / 1024;
				}
			} else {
				for (i = 0; i < cxt->gain_width * cxt->gain_height; i++) {
					cxt->lsc_buffer[4 * i + 0] = flash_param->preflash_guessing_mainflash_output_table[4 * i + 0] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[4 * i + 1] = flash_param->preflash_guessing_mainflash_output_table[4 * i + 1] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[4 * i + 2] = flash_param->preflash_guessing_mainflash_output_table[4 * i + 2] * cxt->flash_y_gain[i] / 1024;
					cxt->lsc_buffer[4 * i + 3] = flash_param->preflash_guessing_mainflash_output_table[4 * i + 3] * cxt->flash_y_gain[i] / 1024;
				}
			}
		}
		ISP_LOGV("FLASH_MAIN_BEFORE, frame_count=%d, use preflash_guessing_mainflash_output_table=[%d,%d,%d,%d]", cxt->frame_count,
			 flash_param->preflash_guessing_mainflash_output_table[0], flash_param->preflash_guessing_mainflash_output_table[1],
			 flash_param->preflash_guessing_mainflash_output_table[2], flash_param->preflash_guessing_mainflash_output_table[3]);

		// set flash_mode will quick in
		cxt->flash_mode = 1;
		break;

	case ALSC_FLASH_MAIN_AFTER:
		if (cxt->lsc_buffer == NULL || cxt->frame_count == 0) {
			ISP_LOGV("FLASH_MAIN_AFTER, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
			return rtn;
		}
		// set status
		cxt->flash_mode = 0;

		// quick in and set bv_skip_frame
		cxt->alg_quick_in = 1;
		cxt->quik_in_start_frame = -99;
		cxt->bv_skip_frame = 15;
		cxt->flash_done_frame_count = 0;

		// allow calc update the dest buffer
		cxt->can_update_dest = 1;

		// reset parameter
		flash_param->captureFlashEnvRatio = 0.0;
		flash_param->captureFlash1ofALLRatio = 0.0;
		ISP_LOGV("FLASH_MAIN_AFTER, setup quick in, frame_count=%d, lsc_id=%d", cxt->frame_count, cxt->lsc_id);
		break;

	case ALSC_GET_TOUCH:
		flash_param->ae_touch_framecount = cxt->frame_count;
		ISP_LOGV("GET_TOUCH, frame_count=%d, ae_touch_framecount =%d", cxt->frame_count, flash_param->ae_touch_framecount);
		break;

	case SMART_LSC_ALG_LOCK:
		cxt->alg_locked = 1;
		ISP_LOGD("SMART_LSC_ALG_LOCK, alsc alg lock!");
		break;

	case SMART_LSC_ALG_UNLOCK:
		cxt->alg_locked = 0;
		ISP_LOGD("SMART_LSC_ALG_UNLOCK, alsc alg un-lock!");
		break;

	case ALSC_FW_PROC_START:	//for sbs feature now
		// ISP_SINGLE 0, ISP_DUAL_NORMAL 1, ISP_DUAL_SBS 2, ISP_DUAL_SWITCH 3
		ISP_LOGV("FW_PROC_START, is_master=%d, is_multi_mode=%d, frame_count=%d", cxt->is_master, cxt->is_multi_mode, cxt->frame_count);

		fwprocstart_info = (struct alsc_fwprocstart_info *)in;

		if (cxt->LSC_SPD_VERSION <= 5) {
			//pm0_new = fwprocstart_info->lsc_tab_address_new[0];
			if (fwprocstart_info->gain_width_new == cxt->init_gain_width
			    && fwprocstart_info->gain_height_new == cxt->init_gain_height && fwprocstart_info->grid_new == cxt->init_grid) {
				full_flag = 1;
			}

			lsc_read_last_info(lsc_last_info, fwprocstart_info->camera_id, full_flag);
			cxt->fw_start_bv = lsc_last_info->bv;
			cxt->fw_start_bv_gain = lsc_last_info->bv_gain;

			if (cxt->is_multi_mode == 2) {
				ISP_LOGV("FW_PROC_START, ISP_DUAL_SBS MODE, camera_id=%d", fwprocstart_info->camera_id);
				ISP_LOGV("FW_PROC_START, old tab address=%p, (%d,%d) grid %d", cxt->lsc_pm0, cxt->gain_width, cxt->gain_height, cxt->grid);
				ISP_LOGV("FW_PROC_START, new tab address=%p, (%d,%d) grid %d", fwprocstart_info->lsc_tab_address_new[0],
					 fwprocstart_info->gain_width_new, fwprocstart_info->gain_height_new, fwprocstart_info->grid_new);

				if (cxt->lsc_id == 1) {
					// get the master capture parameter info
					proc_start_gain_w = fwprocstart_info->gain_width_new;
					proc_start_gain_h = fwprocstart_info->gain_height_new;
					proc_start_gain_pattern = cxt->output_gain_pattern;
					memcpy(proc_start_param_table, fwprocstart_info->lsc_tab_address_new[0], proc_start_gain_w * proc_start_gain_h * 4 * sizeof(cmr_u16));
					lsc_table_interlace2planar(proc_start_param_table, fwprocstart_info->gain_width_new, fwprocstart_info->gain_height_new,
						fwprocstart_info->image_pattern_new, cxt->output_gain_pattern);
					lsc_table_linear_scaler(cxt->output_lsc_table, cxt->gain_width, cxt->gain_height, proc_start_output_table, proc_start_gain_w, proc_start_gain_h, 1);

					ISP_LOGV("FW_PROC_START, master table size(%d,%d)", proc_start_gain_w, proc_start_gain_h);
					//ISP_LOGV("FW_PROC_START, master output table=[%d,%d,%d,%d]", proc_start_output_table[0],
					//      proc_start_output_table[1], proc_start_output_table[2], proc_start_output_table[3]);
					ISP_LOGV("FW_PROC_START, master DNP table=[%d,%d,%d,%d]", proc_start_param_table[0],
						 proc_start_param_table[1], proc_start_param_table[2], proc_start_param_table[3]);
				} else if (cxt->lsc_id == 2) {
					ISP_LOGV("FW_PROC_START, Get master table size (%d,%d)", proc_start_gain_w, proc_start_gain_h);
					ISP_LOGV("FW_PROC_START, Get master output table=[%d,%d,%d,%d]", proc_start_output_table[0],
						 proc_start_output_table[1], proc_start_output_table[2], proc_start_output_table[3]);
					ISP_LOGV("FW_PROC_START, Get master DNP table=[%d,%d,%d,%d]", proc_start_param_table[0],
						 proc_start_param_table[1], proc_start_param_table[2], proc_start_param_table[3]);

					lsc_master_slave_sync(cxt, fwprocstart_info);
				}
			} else {
				ISP_LOGV("FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do nothing.");
				chnl_gain_num = fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new;
				if (cxt->lsc_id == 1
				    && fwprocstart_info->gain_width_new == lsc_last_info->gain_width && fwprocstart_info->gain_height_new == lsc_last_info->gain_height) {
					memcpy(fwprocstart_info->lsc_result_address_new, lsc_last_info->table, chnl_gain_num * 4 * sizeof(cmr_u16));
					ISP_LOGV("FW_PROC_START, use last_table_rgb");
				} else {
					ISP_LOGV("FW_PROC_START, use TL84 table");
					memcpy(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_tab_address_new[DEFAULT_TAB_INDEX],
					       chnl_gain_num * 4 * sizeof(cmr_u16));
					lsc_table_interlace2planar(fwprocstart_info->lsc_result_address_new, fwprocstart_info->gain_width_new, fwprocstart_info->gain_height_new,
						fwprocstart_info->image_pattern_new, cxt->output_gain_pattern);

					post_gain_param.org_gain = fwprocstart_info->lsc_result_address_new;
					post_gain_param.gain_width = cxt->gain_width;
					post_gain_param.gain_height = cxt->gain_height;
					post_gain_param.gain_pattern = cxt->output_gain_pattern;
					post_gain_param.frame_count = cxt->frame_count;
					post_gain_param.bv = cxt->fw_start_bv;
					post_gain_param.bv_gain = cxt->fw_start_bv_gain;
					post_gain_param.flash_mode = 0;
					post_gain_param.pre_flash_mode = 0;
					post_gain_param.LSC_SPD_VERSION = cxt->LSC_SPD_VERSION;
					post_gain_param.tuning_param = NULL;
					cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_DO_POSTPROCESS, &post_gain_param, fwprocstart_info->lsc_result_address_new);
				}

				//keep the update for calc to as a source for inversing static data
				memcpy(cxt->fwstart_new_scaled_table, fwprocstart_info->lsc_result_address_new, chnl_gain_num * 4 * sizeof(cmr_u16));
			}
		} else {
			// when cxt->LSC_SPD_VERSION >= 6
			ISP_LOGV("FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do nothing.");
			full_flag = 1;
			lsc_read_last_info(lsc_last_info, fwprocstart_info->camera_id, full_flag);
			cxt->fw_start_bv = lsc_last_info->bv;
			cxt->fw_start_bv_gain = lsc_last_info->bv_gain;
			chnl_gain_num = fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new;

			for (i = 0; i < 8; i++) {
				fwprocstart_info->lsc_tab_address_new[i] = cxt->std_init_lsc_table_param_buffer[i];
			}

			if (cxt->lsc_id == 1 && fwprocstart_info->gain_width_new == lsc_last_info->gain_width && fwprocstart_info->gain_height_new == lsc_last_info->gain_height) {
				memcpy(fwprocstart_info->lsc_result_address_new, lsc_last_info->table, chnl_gain_num * 4 * sizeof(cmr_u16));
			} else {
				memcpy(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_tab_address_new[DEFAULT_TAB_INDEX], chnl_gain_num * 4 * sizeof(cmr_u16));

				post_gain_param.org_gain = fwprocstart_info->lsc_result_address_new;
				post_gain_param.gain_width = cxt->gain_width;
				post_gain_param.gain_height = cxt->gain_height;
				post_gain_param.gain_pattern = cxt->output_gain_pattern;
				post_gain_param.frame_count = cxt->frame_count;
				post_gain_param.bv = cxt->fw_start_bv;
				post_gain_param.bv_gain = cxt->fw_start_bv_gain;
				post_gain_param.flash_mode = 0;
				post_gain_param.pre_flash_mode = 0;
				post_gain_param.LSC_SPD_VERSION = cxt->LSC_SPD_VERSION;
				post_gain_param.tuning_param = NULL;
				cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_DO_POSTPROCESS, &post_gain_param, fwprocstart_info->lsc_result_address_new);
			}

			//keep the update for calc to as a source for inversing static data
			memcpy(cxt->fwstart_new_scaled_table, fwprocstart_info->lsc_result_address_new, chnl_gain_num * 4 * sizeof(cmr_u16));
		}

		lsc_gen_flash_y_gain(cxt->flash_y_gain, fwprocstart_info->gain_width_new, fwprocstart_info->gain_height_new, cxt->flash_enhance_ratio,
				cxt->flash_center_shiftx, cxt->flash_center_shifty);

		//planar2interlace for driver use
		if (cxt->is_planar == 1) {
			lsc_table_planar2interlace(fwprocstart_info->lsc_result_address_new, fwprocstart_info->gain_width_new,
				fwprocstart_info->gain_height_new, cxt->output_gain_pattern, cxt->output_gain_pattern);
		}
		break;

	case ALSC_FW_PROC_START_END:
		ISP_LOGV("FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do as ALSC FW_START.");
		break;

	case ALSC_GET_UPDATE_INFO:
		memcpy(cxt->lsc_buffer_interlace, cxt->lsc_buffer, cxt->gain_width * cxt->gain_height * 4 * sizeof(unsigned short));
		//planar2interlace for driver use
		if (cxt->is_planar == 1) {
			lsc_table_planar2interlace(cxt->lsc_buffer_interlace, cxt->gain_width, cxt->gain_height, cxt->output_gain_pattern, cxt->output_gain_pattern);
		}

		alsc_update_info_out = (struct alsc_update_info *)out;
		alsc_update_info_out->alsc_update_flag = cxt->alsc_update_flag;
		alsc_update_info_out->can_update_dest = cxt->can_update_dest;
		alsc_update_info_out->lsc_buffer_addr = cxt->lsc_buffer_interlace;
		if (cxt->flash_mode) {
			alsc_update_info_out->alsc_update_flag = 1;
			alsc_update_info_out->can_update_dest = 1;
		}
		break;

	case ALSC_DO_SIMULATION:
		cxt->can_update_dest = 0;
		alsc_do_simulation = (struct alsc_do_simulation *)in;
		lsc_calc_in = (struct lsc_sprd_calc_in *)malloc(sizeof(struct lsc_sprd_calc_in));
		lsc_calc_out = (struct lsc_sprd_calc_out *)malloc(sizeof(struct lsc_sprd_calc_out));
		tmp_buffer_r = (cmr_u32 *) malloc(32 * 32 * sizeof(cmr_u32));
		tmp_buffer_g = (cmr_u32 *) malloc(32 * 32 * sizeof(cmr_u32));
		tmp_buffer_b = (cmr_u32 *) malloc(32 * 32 * sizeof(cmr_u32));
		tmp_buffer = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));
		lsc_calc_in->stat_img.r = tmp_buffer_r;
		lsc_calc_in->stat_img.gr = tmp_buffer_g;
		lsc_calc_in->stat_img.gb = tmp_buffer_g;
		lsc_calc_in->stat_img.b = tmp_buffer_b;
		lsc_calc_in->stat_img.w = MAX_STAT_WIDTH;
		lsc_calc_in->stat_img.h = MAX_STAT_HEIGHT;
		lsc_calc_out->dst_gain = tmp_buffer;

		memcpy(lsc_calc_in->last_lsc_table, cxt->std_init_lsc_table_param_buffer[DEFAULT_TAB_INDEX], cxt->init_gain_width * cxt->init_gain_height * 4 * sizeof(cmr_u16));

		lsc_calc_in->img_width = cxt->init_img_width;
		lsc_calc_in->img_height = cxt->init_img_height;
		lsc_calc_in->gain_width = cxt->init_gain_width;
		lsc_calc_in->gain_height = cxt->init_gain_height;
		lsc_calc_in->grid = cxt->init_grid;
		memcpy(lsc_calc_in->stat_img.r, alsc_do_simulation->stat_r, MAX_STAT_WIDTH * MAX_STAT_HEIGHT * sizeof(cmr_u32));
		memcpy(lsc_calc_in->stat_img.gr, alsc_do_simulation->stat_g, MAX_STAT_WIDTH * MAX_STAT_HEIGHT * sizeof(cmr_u32));
		memcpy(lsc_calc_in->stat_img.b, alsc_do_simulation->stat_b, MAX_STAT_WIDTH * MAX_STAT_HEIGHT * sizeof(cmr_u32));

		ISP_LOGI("do_simulation img_size[%d,%d], gain_size[%d,%d], stat_size[%d,%d] grid: %d",lsc_calc_in->img_width,lsc_calc_in->img_height,lsc_calc_in->gain_width,lsc_calc_in->gain_height,lsc_calc_in->stat_img.w,lsc_calc_in->stat_img.h,lsc_calc_in->grid);

		for (i = 0; i < 8; i++)
			lsc_calc_in->lsc_tab[i] = cxt->std_init_lsc_table_param_buffer[i];

		// first alsc calc
		ISP_LOGI("first alsc calc last_lsc_table[%d,%d,%d,%d]",lsc_calc_in->last_lsc_table[0],lsc_calc_in->last_lsc_table[1],lsc_calc_in->last_lsc_table[2],lsc_calc_in->last_lsc_table[3]);
		rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, lsc_calc_in, lsc_calc_out);

		memcpy(lsc_calc_in->last_lsc_table, lsc_calc_out->dst_gain, cxt->init_gain_width * cxt->init_gain_height * 4 * sizeof(cmr_u16));
		// second alsc calc
		ISP_LOGI("second alsc calc last_lsc_table[%d,%d,%d,%d]",lsc_calc_in->last_lsc_table[0],lsc_calc_in->last_lsc_table[1],lsc_calc_in->last_lsc_table[2],lsc_calc_in->last_lsc_table[3]);
		rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, lsc_calc_in, lsc_calc_out);
		ISP_LOGI("second alsc end! lsc_calc_out[%d,%d,%d,%d]",lsc_calc_out->dst_gain[0],lsc_calc_out->dst_gain[1],lsc_calc_out->dst_gain[2],lsc_calc_out->dst_gain[3]);
		// post shading gain
		post_gain_param.org_gain = lsc_calc_out->dst_gain;
		post_gain_param.gain_width = cxt->init_gain_width;
		post_gain_param.gain_height = cxt->init_gain_height;
		post_gain_param.gain_pattern = cxt->output_gain_pattern;
		post_gain_param.frame_count = cxt->frame_count;
		post_gain_param.bv = alsc_do_simulation->bv;
		post_gain_param.bv_gain = alsc_do_simulation->bv_gain;
		post_gain_param.flash_mode = 0;
		post_gain_param.pre_flash_mode = 0;
		post_gain_param.LSC_SPD_VERSION = cxt->LSC_SPD_VERSION;
		post_gain_param.tuning_param = NULL;
		cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_DO_POSTPROCESS, &post_gain_param, alsc_do_simulation->sim_output_table);

		memcpy(cxt->output_lsc_table, alsc_do_simulation->sim_output_table, cxt->init_gain_width * cxt->init_gain_height * 4 * sizeof(cmr_u16));
		ISP_LOGI("post process output_lsc_table[%d,%d,%d,%d]",cxt->output_lsc_table[0],cxt->output_lsc_table[1],cxt->output_lsc_table[2],cxt->output_lsc_table[3]);
		//planar2interlace for driver use
		if(cxt->is_planar == 1){
			lsc_table_planar2interlace(alsc_do_simulation->sim_output_table, cxt->init_gain_width,
				cxt->init_gain_height, cxt->output_gain_pattern, cxt->output_gain_pattern);
		}
		lsc_std_free(tmp_buffer);
		lsc_std_free(tmp_buffer_r);
		lsc_std_free(tmp_buffer_g);
		lsc_std_free(tmp_buffer_b);
		lsc_std_free(lsc_calc_out);
		lsc_std_free(lsc_calc_in);
		cxt->can_update_dest = 1;
		break;

	default:
		ISP_LOGV("IO_CTRL, NO cmd=%d", cmd);
		break;
	}

	return rtn;
}

static cmr_s32 lsc_sprd_deinit(void *handle, void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	cmr_s32 i = 0;
	struct lsc_sprd_ctrl_context *cxt = NULL;
	UNUSED(in);
	UNUSED(out);

	if (!handle) {
		ISP_LOGE("fail to check param!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_sprd_ctrl_context *)handle;

	rtn = cxt->lib_ops.alsc_deinit(cxt->alsc_handle);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to do alsc deinit!");
	}

	rtn = lsc_unload_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to unload lsc lib");
	}

	pthread_mutex_destroy(&cxt->status_lock);

	for (i = 0; i < 8; i++) {
		lsc_std_free(cxt->std_init_lsc_table_param_buffer[i]);
		lsc_std_free(cxt->std_lsc_table_param_buffer[i]);
	}
	lsc_std_free(cxt->dst_gain);
	lsc_std_free(cxt->lsc_buffer);
	lsc_std_free(cxt->fwstart_new_scaled_table);
	lsc_std_free(cxt->fwstop_output_table);
	lsc_std_free(cxt->lsc_last_info);
	lsc_std_free(cxt->ae_stat);
	lsc_std_free(cxt->lsc_flash_proc_param);
	lsc_std_free(cxt->flash_y_gain);
	lsc_std_free(cxt->last_lsc_table);
	lsc_std_free(cxt->output_lsc_table);
	lsc_std_free(cxt->lsc_buffer_interlace);
	if (cxt->lsc_id == 1) {
		id1_addr = NULL;
	} else {
		id2_addr = NULL;
	}

	memset(cxt, 0, sizeof(*cxt));
	lsc_std_free(cxt);
	ISP_LOGI("done rtn = %d", rtn);

	return rtn;
}

struct adpt_ops_type lsc_sprd_adpt_ops_ver0 = {
	.adpt_init = lsc_sprd_init,
	.adpt_deinit = lsc_sprd_deinit,
	.adpt_process = lsc_sprd_calculation,
	.adpt_ioctrl = lsc_sprd_ioctrl,
};
