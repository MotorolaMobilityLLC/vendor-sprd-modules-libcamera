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
#include "lsc_sprd_adapt_v1.h"
#include "isp_adpt.h"
#include <dlfcn.h>
#include "isp_mw.h"
#include <utils/Timers.h>
#include <sys/stat.h>
#include <math.h>
#include "cmr_common.h"

#define MAX_CAMERA_ID  6

#define DEFAULT_TAB_INDEX 2

cmr_u16 lock =0;

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
	"liblsc.so",
	"libalsc3.2.so",
	"liblsc_v1.so",
	"liblsc_v2.so",
	"liblsc_v3.so",
	"liblsc_v4.so",
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
	unsigned int index_r, index_gr, index_gb, index_b;
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
	} else if (strcmp(prop, "10") == 0) {	// up(8x)bottom(1x)
		for (j = 0; j < height / 2; j++) {
			for (i = 0; i < width; i++) {
				table[4 * (j * width + i) + 0] = 8192;
				table[4 * (j * width + i) + 1] = 8192;
				table[4 * (j * width + i) + 2] = 8192;
				table[4 * (j * width + i) + 3] = 8192;
			}
		}
	} else if (strcmp(prop, "01") == 0) {	// up(1x)bottom(8x)
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
	unsigned int line_align = (unsigned int)((width * 3 + 3) & (~3));
	unsigned char *pbgr = (unsigned char *)malloc(line_align * height);
	memset(pbgr, 0, line_align * height);

	unsigned int channel_width = img_width / 2;
	unsigned int channel_height = img_height / 2;
	unsigned int pix_num_per_block = (unsigned int)(channel_width / width) * (unsigned int)(channel_height / height);
	unsigned int divisor = pix_num_per_block * 4;	// 10 bit to 8 bit

	unsigned char *pbuf = pbgr;
	if (NULL == pbgr){
		return -1;
	}
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

static void lsc_dump_gain(cmr_u16 * table, cmr_u32 width, cmr_u32 height, cmr_u32 pattern, cmr_u32 is_planar, cmr_u32 frame_count, cmr_u32 camera_id)
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
		sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_%05d_gr.txt", camera_id, frame_count);
		lsc_dump(tmp + (long)(width * height * off_gr), width, height, filename);

		sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_%05d_r.txt", camera_id, frame_count);
		lsc_dump(tmp + (long)(width * height * off_r), width, height, filename);

		sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_%05d_b.txt", camera_id, frame_count);
		lsc_dump(tmp + (long)(width * height * off_b), width, height, filename);

		sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_%05d_gb.txt", camera_id, frame_count);
		lsc_dump(tmp + (long)(width * height * off_gb), width, height, filename);
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

        if (0 != grid) {
                *lsc_otp_width = (int)(img_width / (2 * grid)) + 1;
	        *lsc_otp_height = (int)(img_height / (2 * grid)) + 1;

                if (img_width % (2 * grid) != 0) {
		        *lsc_otp_width += 1;
	        }

                if (img_height % (2 * grid) != 0) {
		        *lsc_otp_height += 1;
	        }
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

static int lsc_table_linear_scaler(cmr_u16 * src_tab, cmr_u32 src_w, cmr_u32 src_h, cmr_u16 * dst_tab, cmr_u32 dst_w, cmr_u32 dst_h, cmr_u32 is_plane)
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

	if (src_w < 4 || src_h < 4 || src_w > MAX_WIDTH || src_h > MAX_HEIGHT) {
		ISP_LOGE("the size of src_tab out of limit !");
		return -1;
	}

	if (dst_w < 4 || dst_h < 4 || dst_w > MAX_WIDTH || dst_h > MAX_HEIGHT) {
		ISP_LOGE("the size of dst_tab out of limit !");
		return -1;
	}

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

	return 0;
}

static cmr_s32 lsc_master_slave_sync(struct lsc_sprd_ctrl_context *cxt, struct alsc_fwprocstart_info *fwprocstart_info)
{
	cmr_u32 i, rtn = 0;
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
	cmr_u32 new_chnl_gain_num = new_width * new_height;

	for (i = 0; i < pre_width * pre_height * 4; i++) {
		lsc_pre_reslut_table[i] = proc_start_output_table[i];	// master output table
		lsc_pre_table[i] = proc_start_param_table[i];	// master DNP param table
	}

	// slave DNP param table
	memcpy(new_table, fwprocstart_info->lsc_tab_address_new[0], new_width * new_height * 4 * sizeof(unsigned short));
	ISP_LOGV("slave size[%d,%d], slave_DNP[%d,%d,%d,%d]", new_width, new_height, new_table[0*new_chnl_gain_num], new_table[1*new_chnl_gain_num], new_table[2*new_chnl_gain_num], new_table[3*new_chnl_gain_num]);

	new_pattern = lsc_raw_pattern_to_lsc_pattern(fwprocstart_info->image_pattern_new);

	// scale master dnp table to slave size
	rtn = lsc_table_linear_scaler(lsc_pre_table, pre_width, pre_height, output_tab, new_width, new_height, cxt->is_planar);
	if(rtn)
		return -1;

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
	rtn = lsc_table_linear_scaler(lsc_pre_reslut_table, pre_width, pre_height, lsc_result_address_new, new_width, new_height, cxt->is_planar);
	if(rtn)
		return -1;

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
			lsc_result_address_new[4 * i + off_gr] = (cmr_u16)(output_gr[i] * lsc_new_weight_tab_gr[i]);
			lsc_result_address_new[4 * i + off_r] = (cmr_u16)(output_r[i] * lsc_new_weight_tab_r[i]);
			lsc_result_address_new[4 * i + off_b] = (cmr_u16)(output_b[i] * lsc_new_weight_tab_b[i]);
			lsc_result_address_new[4 * i + off_gb] = (cmr_u16)(output_gb[i] * lsc_new_weight_tab_gb[i]);
		}
	} else {
		for (i = 0; i < new_width * new_height; i++) {
			lsc_result_address_new[i + off_gr * new_width * new_height] = (cmr_u16)(output_gr[i] * lsc_new_weight_tab_gr[i]);
			lsc_result_address_new[i + off_r * new_width * new_height] = (cmr_u16)(output_r[i] * lsc_new_weight_tab_r[i]);
			lsc_result_address_new[i + off_b * new_width * new_height] = (cmr_u16)(output_b[i] * lsc_new_weight_tab_b[i]);
			lsc_result_address_new[i + off_gb * new_width * new_height] = (cmr_u16)(output_gb[i] * lsc_new_weight_tab_gb[i]);
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
		 lsc_result_address_new[0*new_chnl_gain_num], lsc_result_address_new[1*new_chnl_gain_num], lsc_result_address_new[2*new_chnl_gain_num], lsc_result_address_new[3*new_chnl_gain_num]);

	//keep the update for calc to as a source for inversing static data
	memcpy(cxt->fwstart_new_scaled_table, lsc_result_address_new, new_width * new_height * 4 * sizeof(unsigned short));

	return 0;
}

static cmr_s32 lsc_calculate_otplen_chn(cmr_u32 full_width, cmr_u32 full_height, cmr_u32 lsc_grid)
{
	cmr_u32 half_width, half_height, lsc_width, lsc_height;
	cmr_s32 otp_len_chn = 0;
	half_width = full_width / 2;
	half_height = full_height / 2;

        if (0 != lsc_grid) {
                lsc_width = ((half_width % lsc_grid) > 0) ? (half_width / lsc_grid + 2) : (half_width / lsc_grid + 1);
	        lsc_height = ((half_height % lsc_grid) > 0) ? (half_height / lsc_grid + 2) : (half_height / lsc_grid + 1);
	        otp_len_chn = ((lsc_width * lsc_height) * 14 % 8) ? (((lsc_width * lsc_height) * 14 / 8) + 1) : ((lsc_width * lsc_height) * 14 / 8);
	        otp_len_chn = (otp_len_chn % 2) ? (otp_len_chn + 1) : (otp_len_chn);
	}

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
	cmr_u8 *lsc_otp_addr = NULL;
	cmr_u16 lsc_otp_len = 0;
	cmr_s32 compressed_lens_bits = 14;
	cmr_s32 lsc_otp_width, lsc_otp_height;
	cmr_s32 lsc_otp_len_chn = 0;
	cmr_s32 lsc_otp_chn_gain_num = 0;
	cmr_u8 *oc_otp_data = NULL;
	cmr_u16 oc_otp_len = 0;
	cmr_u8 *otp_data_ptr = NULL;
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

				if (NULL != lsc_otp_info && NULL != oc_otp_info) {
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
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + (long)(lsc_otp_len_chn * 0)), lsc_16_bits + (long)(lsc_otp_chn_gain_num * 0), lsc_otp_chn_gain_num);
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + (long)(lsc_otp_len_chn * 1)), lsc_16_bits + (long)(lsc_otp_chn_gain_num * 1), lsc_otp_chn_gain_num);
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + (long)(lsc_otp_len_chn * 2)), lsc_16_bits + (long)(lsc_otp_chn_gain_num * 2), lsc_otp_chn_gain_num);
		lsc_gain_14bits_to_16bits((cmr_u16 *) (lsc_otp_addr + (long)(lsc_otp_len_chn * 3)), lsc_16_bits + (long)(lsc_otp_chn_gain_num * 3), lsc_otp_chn_gain_num);

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
			lsc_dump(lsc_16_bits + (long)(lsc_otp_width * lsc_otp_height * 0), lsc_otp_width, lsc_otp_height, filename);
			sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_otp_dump_gr.txt", lsc_param->camera_id);
			lsc_dump(lsc_16_bits + (long)(lsc_otp_width * lsc_otp_height * 1), lsc_otp_width, lsc_otp_height, filename);
			sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_otp_dump_gb.txt", lsc_param->camera_id);
			lsc_dump(lsc_16_bits + (long)(lsc_otp_width * lsc_otp_height * 2), lsc_otp_width, lsc_otp_height, filename);
			sprintf(filename, CAMERA_DATA_FILE "/lsc/cam%d_otp_dump_b.txt", lsc_param->camera_id);
			lsc_dump(lsc_16_bits + (long)(lsc_otp_width * lsc_otp_height * 3), lsc_otp_width, lsc_otp_height, filename);
		}

		ISP_LOGV("lsc_otp_width=%d, lsc_otp_height=%d, lsc_otp_grid=%d", lsc_otp_width, lsc_otp_height, lsc_otp_grid);
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

static cmr_s32 lsc_set_init_param(struct lsc_adv_init_param *init_param, struct lsc_sprd_ctrl_context *cxt, struct lsc_sprd_init_in *sprd_init_param)
{
	cmr_s32 i;
	cmr_s32 rtn = LSC_SUCCESS;

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
	cxt->ref_camera_id = init_param->camera_id;
	cxt->next_camera_id = init_param->camera_id;
	cxt->sync_flag = 0;
	cxt->can_update_dest = 1;
	cxt->alsc_update_flag = 0;
	cxt->fw_start_end = 0;
	cxt->alg_bypass = 0;
	cxt->lsc_pm0 = init_param->lsc_tab_address[0];
	cxt->output_gain_pattern = init_param->output_gain_pattern;

	//pre process the lsc table
	if (cxt->is_planar == 1) {
		for (i = 0; i < 9; i++) {
			memcpy(cxt->std_init_lsc_table_param_buffer[i], init_param->lsc_tab_address[i], cxt->init_gain_width*cxt->init_gain_height*4*sizeof(cmr_u16));
			lsc_table_interlace2planar(cxt->std_init_lsc_table_param_buffer[i], cxt->init_gain_width, cxt->init_gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
		}
	} else {
		for (i = 0; i < 9; i++) {
			memcpy(cxt->std_init_lsc_table_param_buffer[i], init_param->lsc_tab_address[i], cxt->init_gain_width*cxt->init_gain_height*4*sizeof(cmr_u16));
			lsc_interlace_change_pattern(cxt->std_init_lsc_table_param_buffer[i], cxt->init_gain_width, cxt->init_gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
		}
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
		sprd_init_param->lsc_tab_address[i] = cxt->std_init_lsc_table_param_buffer[i];	// the address of table parameter
		sprd_init_param->rg_gradient[i] = init_param->rg_gradient[i];
		sprd_init_param->bg_gradient[i] = init_param->bg_gradient[i];
	}
	sprd_init_param->tune_param_ptr =  init_param->tune_param_ptr;
	sprd_init_param->lsc_otp_table_en =  init_param->lsc_otp_table_en;
	sprd_init_param->lsc_otp_table_width =  init_param->lsc_otp_table_width;
	sprd_init_param->lsc_otp_table_height =  init_param->lsc_otp_table_height;
	sprd_init_param->lsc_otp_grid =  init_param->lsc_otp_grid;
	sprd_init_param->lsc_otp_raw_width = init_param->lsc_otp_raw_width;
	sprd_init_param->lsc_otp_raw_height = init_param->lsc_otp_raw_height;
	sprd_init_param->lsc_otp_table_addr =  init_param->lsc_otp_table_addr;
	sprd_init_param->gridx = init_param->grid;
	sprd_init_param->gridy = init_param->grid;
	sprd_init_param->stat_inverse = cxt->stats_inverse;


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
			det_R = (float)(2 * grid - det_L);
			det_D = (float)(2 * grid - det_U);

			tmp = coef * (TL * det_D * det_R + TR * det_D * det_L + BL * det_U * det_R + BR * det_U * det_L);
			new_tab[j * stat_width + i] = (unsigned short)LSC_CLIP(tmp + 0.5f, LSC_GAIN_LOWER, LSC_GAIN_UPPER);
		}
	}
}

static void lsc_inverse_ae_stat(struct lsc_sprd_ctrl_context *cxt, cmr_u16 * inverse_table)
{
	cmr_u32 i;
	cmr_u16 gain_r[32 * 32] = {0};
	cmr_u16 gain_gr[32 * 32] = {0};
	cmr_u16 gain_gb[32 * 32] = {0};
	cmr_u16 gain_b[32 * 32] = {0};
	cmr_u16 gain_g[32 * 32] = {0};
	cmr_u16 scaled_gain_r[32 * 32] = {0};
	cmr_u16 scaled_gain_g[32 * 32] = {0};
	cmr_u16 scaled_gain_b[32 * 32] = {0};

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
		return 1;
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
	cmr_u32 i, rtn = 0;
	struct lsc_flash_proc_param *flash_param = (struct lsc_flash_proc_param *)cxt->lsc_flash_proc_param;
	cmr_u32 pre_width = cxt->gain_width;
	cmr_u32 pre_height = cxt->gain_height;
	cmr_u32 pre_pattern = cxt->gain_pattern;
	cmr_u16 lsc_pre_reslut_table[32 * 32 * 4] = { 0 };
	cmr_u16 lsc_pre_table[32 * 32 * 4] = { 0 };
	cmr_u32 new_width = fwstart_info->gain_width_new;
	cmr_u32 new_height = fwstart_info->gain_height_new;
	cmr_u32 new_pattern = 3;
	cmr_u16 *lsc_result_new = fwstart_info->lsc_result_address_new;	//new dest output address
	cmr_u16 lsc_new_table[32 * 32 * 4] = { 0 };	// golden new DNP table from pm, no OTP apply
	cmr_u32 output_gain_pattern = cxt->output_gain_pattern;

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

	new_pattern = lsc_raw_pattern_to_lsc_pattern(fwstart_info->image_pattern_new);

	ISP_LOGV("FW_START, pre:[%d, %d, %d], new:[%d, %d, %d]", pre_width, pre_height, pre_pattern, new_width, new_height, new_pattern);
	if (pre_width == new_width && pre_height == new_height) {
		memcpy(lsc_result_new, lsc_pre_reslut_table, pre_width * pre_height * 4 * sizeof(cmr_u16));
	} else {
		cmr_u16 scaled_tab[32 * 32 * 4] = { 0 };
		cmr_u16 r_tab[32 * 32] = { 0 };
		cmr_u16 gr_tab[32 * 32] = { 0 };
		cmr_u16 gb_tab[32 * 32] = { 0 };
		cmr_u16 b_tab[32 * 32] = { 0 };
		cmr_u16 pre_out_r[32 * 32] = { 0 };
		cmr_u16 pre_out_gr[32 * 32] = { 0 };
		cmr_u16 pre_out_gb[32 * 32] = { 0 };
		cmr_u16 pre_out_b[32 * 32] = { 0 };
		cmr_u16 r_tab_new[32 * 32] = { 0 };
		cmr_u16 gr_tab_new[32 * 32] = { 0 };
		cmr_u16 gb_tab_new[32 * 32] = { 0 };
		cmr_u16 b_tab_new[32 * 32] = { 0 };
		float weight_tab_gb[32 * 32] = { 0 };
		float weight_tab_b[32 * 32] = { 0 };
		float weight_tab_r[32 * 32] = { 0 };
		float weight_tab_gr[32 * 32] = { 0 };
		float rate_gb = 0.0;
		float rate_b = 0.0;
		float rate_r = 0.0;
		float rate_gr = 0.0;
		cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
		cmr_u32 gr_chnl_off, r_chnl_off, b_chnl_off, gb_chnl_off;

		// step1: scale pre DNP table to new gain size
		rtn = lsc_table_linear_scaler(lsc_pre_table, pre_width, pre_height, scaled_tab, new_width, new_height, 0);
		if(rtn)
			return -1;

		lsc_interlace_to_channel(new_width, new_height, pre_pattern, r_tab, gr_tab, gb_tab, b_tab, scaled_tab);

		// step2: scale pre dest table
		rtn = lsc_table_linear_scaler(lsc_pre_reslut_table, pre_width, pre_height, scaled_tab, new_width, new_height, planar_flag);
		if(rtn)
			return -1;

		if (cxt->is_planar == 0) {
			lsc_interlace_to_channel(new_width, new_height, output_gain_pattern, pre_out_r, pre_out_gr, pre_out_gb, pre_out_b, scaled_tab);
		} else {
			lsc_planar_to_channel(new_width, new_height, output_gain_pattern, pre_out_r, pre_out_gr, pre_out_gb, pre_out_b, scaled_tab);
		}

		// step3: get new DNP tab to 4 channel
		lsc_interlace_to_channel(new_width, new_height, new_pattern, r_tab_new, gr_tab_new, gb_tab_new, b_tab_new, lsc_new_table);

		// step4: get scale weight table from the ratio of the 2 DNP tables, that is (new DNP)/(old scaled DNP)
		for (i = 0; i < new_width * new_height; i++) {
			rate_r = 0.0;
			rate_gr = 0.0;
			rate_gb = 0.0;
			rate_b = 0.0;
			//r
			if (r_tab[i] == 0 || r_tab[i] == 1024) {
				rate_r = 1;
			} else {
				rate_r = (float)r_tab_new[i] / (float)r_tab[i];
			}
			weight_tab_r[i] = rate_r;
			//gr
			if (gr_tab[i] == 0 || gr_tab[i] == 1024) {
				rate_gr = 1;
			} else {
				rate_gr = (float)gr_tab_new[i] / (float)gr_tab[i];
			}
			weight_tab_gr[i] = rate_gr;
			//gb
			if (gb_tab[i] == 0 || gb_tab[i] == 1024) {
				rate_gb = 1;
			} else {
				rate_gb = (float)gb_tab_new[i] / (float)gb_tab[i];
			}
			weight_tab_gb[i] = rate_gb;
			//b
			if (b_tab[i] == 0 || b_tab[i] == 1024) {
				rate_b = 1;
			} else {
				rate_b = (float)b_tab_new[i] / (float)b_tab[i];
			}
			weight_tab_b[i] = rate_b;
		}

		if (cxt->change_pattern_flag)
			new_pattern = output_gain_pattern;

		//step5: send output to lsc_result_new
		lsc_get_channel_index(new_pattern, &off_gr, &off_r, &off_b, &off_gb);
		gr_chnl_off = off_gr * new_width * new_height;
		r_chnl_off = off_r * new_width * new_height;
		b_chnl_off = off_b * new_width * new_height;
		gb_chnl_off = off_gb * new_width * new_height;

		if (cxt->is_planar == 0) {
			for (i = 0; i < new_width * new_height; i++) {
				lsc_result_new[4 * i + off_gr] = RANGE(pre_out_gr[i] * weight_tab_gr[i], 1024, 16383);
				lsc_result_new[4 * i + off_r] = RANGE(pre_out_r[i] * weight_tab_r[i], 1024, 16383);
				lsc_result_new[4 * i + off_b] = RANGE(pre_out_b[i] * weight_tab_b[i], 1024, 16383);
				lsc_result_new[4 * i + off_gb] = RANGE(pre_out_gb[i] * weight_tab_gb[i], 1024, 16383);
			}
		} else {
			for (i = 0; i < new_width * new_height; i++) {
				lsc_result_new[i + gr_chnl_off] = RANGE(pre_out_gr[i] * weight_tab_gr[i], 1024, 16383);
				lsc_result_new[i + r_chnl_off] = RANGE(pre_out_r[i] * weight_tab_r[i], 1024, 16383);
				lsc_result_new[i + b_chnl_off] = RANGE(pre_out_b[i] * weight_tab_b[i], 1024, 16383);
				lsc_result_new[i + gb_chnl_off] = RANGE(pre_out_gb[i] * weight_tab_gb[i], 1024, 16383);
			}
		}
	}

	// update zone
	memcpy(cxt->fwstart_new_scaled_table, lsc_result_new, new_width * new_height * 4 * sizeof(cmr_u16));

	// binning preflash -> fullsize main flash
	memcpy(flash_param->preflash_guessing_mainflash_output_table, lsc_result_new, new_width * new_height * 4 * sizeof(cmr_u16));

	ISP_LOGV("FW_START, lsc_result_new=[%d,%d,%d,%d]", lsc_result_new[0], lsc_result_new[1], lsc_result_new[2], lsc_result_new[3]);
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
	unsigned short *out_buffer = (unsigned short *)malloc(dst->gain_width * dst->gain_height * 4 * sizeof(unsigned short));

	if(!src_ch0||!src_ch1||!src_ch2||!src_ch3||!out_buffer){
		ISP_LOGE("do LSC_CROP error, malloc buffer error!");
		goto EXIT;
	}
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
EXIT:
	lsc_std_free(src_ch0);
	lsc_std_free(src_ch1);
	lsc_std_free(src_ch2);
	lsc_std_free(src_ch3);
	lsc_std_free(out_buffer);

	return rtn;
}

static void lsc_save_last_info(struct lsc_last_info *cxt, unsigned int camera_id, unsigned int table_flag, unsigned int app_mode)
{
	FILE *fp = NULL;
	struct lsc_last_info full_cxt[MAX_CAMERA_ID * 3]; // each camera maximum have 3 kinds of lsc table size
	char *ptr = (char *)full_cxt;
	int num_read;
	int width = cxt->gain_width;
	int height = cxt->gain_height;
	int index[4] = {0*width*height, 1*width*height, 2*width*height, 3*width*height};

	if(app_mode != 1){
		fp = fopen(CAMERA_DATA_FILE "/lsc.file", "rb");
	}else{
		fp = fopen(CAMERA_DATA_FILE "/lsc_manual.file", "rb");
	}
	//read all last info in lsc.file first
	if (NULL == fp) {
		memset((void *)ptr, 0, sizeof(struct lsc_last_info) * MAX_CAMERA_ID * 3);
	} else {
		num_read = (int)fread(ptr, 1, sizeof(struct lsc_last_info) * MAX_CAMERA_ID * 3, fp);
		fclose(fp);
		fp = NULL;
	}

	// save cxt of camera_id to lsc.file
	ptr = ptr + sizeof(struct lsc_last_info) * 3 * camera_id + sizeof(struct lsc_last_info) * table_flag; // table_flag: 0, 1, 2
	memcpy(ptr, (char *)cxt, sizeof(struct lsc_last_info));

	if(app_mode != 1){
		fp = fopen(CAMERA_DATA_FILE "/lsc.file", "wb");
	}else{
		fp = fopen(CAMERA_DATA_FILE "/lsc_manual.file", "wb");
	}

	if (NULL != fp ) {
		ISP_LOGD("camera_id:%d, bv:%d, bv_gain:%d, table_rgb[%d,%d,%d,%d], table_size[%d,%d]", camera_id, cxt->bv, cxt->bv_gain,
		     cxt->table[index[0]], cxt->table[index[1]], cxt->table[index[2]], cxt->table[index[3]], cxt->gain_width, cxt->gain_height);

		fwrite((char *)full_cxt, 1, sizeof(struct lsc_last_info) * MAX_CAMERA_ID * 3, fp);
		fclose(fp);
		fp = NULL;
	}
}

static int lsc_read_last_info(struct lsc_last_info *cxt, unsigned int camera_id, unsigned int table_flag, unsigned int app_mode)
{
	FILE *fp = NULL;
	struct lsc_last_info full_cxt[MAX_CAMERA_ID * 3];
	char *ptr = (char *)full_cxt;
	int num_read = 0;
	unsigned int width = 0;
	unsigned int height = 0;

	if(app_mode != 1){
		fp = fopen(CAMERA_DATA_FILE "/lsc.file", "rb");
	}else{
		fp = fopen(CAMERA_DATA_FILE "/lsc_manual.file", "rb");
	}

	if (NULL != fp) {
		memset((void *)ptr, 0, sizeof(struct lsc_last_info) * 3 * MAX_CAMERA_ID); // each camera maximum have 3 kinds of lsc table size
		num_read = (int)fread((char *)ptr, 1, sizeof(struct lsc_last_info) * 3 * MAX_CAMERA_ID, fp);

		ptr = ptr + sizeof(struct lsc_last_info) * 3 * camera_id + sizeof(struct lsc_last_info) * table_flag; // table_flag: 0, 1, 2
		memcpy((char *)cxt, (char *)ptr, sizeof(struct lsc_last_info));

		fclose(fp);
		fp = NULL;

		if (cxt->gain_width <= 32 && cxt->gain_height <= 32) {
			width = cxt->gain_width;
		        height = cxt->gain_height;
		} else {
		        cxt->gain_width = 0;
		        cxt->gain_height = 0;
		}

		ISP_LOGD("camera_id:%d, bv:%d, bv_gain:%d, table_rgb[%d,%d,%d,%d], table_size[%d,%d]", camera_id, cxt->bv, cxt->bv_gain, cxt->table[0 * width * height],
			cxt->table[1 * width * height], cxt->table[2 * width * height], cxt->table[3 * width * height], cxt->gain_width, cxt->gain_height);

	}

	return num_read;
}

static int lsc_preprocess_fwstart_info(struct lsc_sprd_ctrl_context *cxt, struct alsc_fwstart_info *fwstart_info)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct pm_lsc_full *pm_lsc_full = NULL;
	struct pm_lsc_crop *pm_lsc_crop = NULL;
	int binning_crop = 0;
	int i = 0;

	if (cxt->LSC_SPD_VERSION >= 6) {
		if (cxt->init_img_width * fwstart_info->img_height_new == cxt->init_img_height * fwstart_info->img_width_new) {
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
			if((pm_lsc_full->img_width / 2) >= 1280 && (pm_lsc_full->img_height / 2) >= 720){
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
			if((pm_lsc_full->img_width / 2) >= 1920 && (pm_lsc_full->img_height / 2) >= 1080){
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

static cmr_s32 lsc_sprd_set_monitor(struct alsc_fwstart_info* in, struct lsc_monitor_info* out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_monitor_info* info = out;
	struct lsc_trim trim;
	info->skip_num = 0;
	info->work_mode = 1;

	info->win_num.w = 32;  //default value
	info->win_num.h = 32;
	trim.x = 0;
	trim.y = 0;
	trim.w = in->img_width_new;
	trim.h = in->img_height_new;
	info->win_size.w = ((trim.w / info->win_num.w) / 2) * 2;
	info->win_size.h = ((trim.h / info->win_num.h) / 2) * 2;
	info->trim.w = info->win_size.w * info->win_num.w;
	info->trim.h =  info->win_size.h * info->win_num.h;
	info->trim.x = trim.x + (trim.w - info->trim.w) / 2;
	info->trim.x = (info->trim.x / 2) * 2;
	info->trim.y = trim.y + (trim.h - info->trim.h) / 2;
	info->trim.y = (info->trim.y / 2) * 2;

	return rtn;

}

static int lsc_malloc_buffer(struct lsc_sprd_ctrl_context *cxt)
{
	cmr_s32 rtn = LSC_SUCCESS;
	int i = 0;

	for (i = 0; i < 9; i++) {
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

	cxt->lsc_last_info = (struct lsc_last_info *)malloc(sizeof(struct lsc_last_info));
	if (NULL == cxt->lsc_last_info) {
		ISP_LOGE("fail to alloc lsc_last_info!");
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

	cxt->lscm_info = (struct lsc_monitor_info*)malloc(sizeof(struct lsc_monitor_info));
	if(cxt->lscm_info == NULL){
		ISP_LOGE("malloc lscm_info error!");
		goto exit;
	}
	return rtn;

exit:
	rtn = LSC_ALLOC_ERROR;
	return rtn;
}

static void lsc_sprd_do_postgain(void *handle,cmr_u16 *org_gain,cmr_u32 gain_width,cmr_u32 gain_height,cmr_s32 bv,cmr_s32 bv_gain,
											cmr_s32 flash_mode,cmr_s32 pre_flash_mode,cmr_u16 *dst_gain)
{
	struct lsc_sprd_ctrl_context *cxt = (struct lsc_sprd_ctrl_context *)handle;
	struct lsc_post_shading_param post_gain_param;
	cmr_u32 chnl_gain_num = gain_width * gain_height;

	post_gain_param.org_gain = org_gain;
	post_gain_param.gain_width = gain_width;
	post_gain_param.gain_height = gain_height;
	post_gain_param.gain_pattern = cxt->output_gain_pattern;
	post_gain_param.bv = bv;
	post_gain_param.bv_gain = bv_gain;
	post_gain_param.flash_mode = flash_mode;
	post_gain_param.pre_flash_mode = pre_flash_mode;

	cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_DO_POSTPROCESS, &post_gain_param, dst_gain);
	ISP_LOGV("postgain process dst_gain[%d,%d,%d,%d]",dst_gain[0*chnl_gain_num],dst_gain[1*chnl_gain_num],dst_gain[2*chnl_gain_num],dst_gain[3*chnl_gain_num]);
}

static void alsc_do_simulation(void *handle,void *in)
{
	struct lsc_sprd_ctrl_context *cxt =(struct lsc_sprd_ctrl_context *)handle;
	struct alsc_do_simulation *alsc_do_simulation = NULL;
	struct lsc_sprd_calc_in *lsc_calc_in = NULL;
	struct lsc_sprd_calc_out *lsc_calc_out = NULL;
	cmr_u32 *tmp_buffer_r = NULL;
	cmr_u32 *tmp_buffer_g = NULL;
	cmr_u32 *tmp_buffer_b = NULL;
	cmr_u16 *tmp_buffer = NULL;
	cmr_u32 rtn = 0;
	cmr_u32 i = 0;
	cmr_u32 chnl_gain_num = 0;

	cxt->can_update_dest = 0;
	alsc_do_simulation = (struct alsc_do_simulation *)in;
	lsc_calc_in = (struct lsc_sprd_calc_in *)malloc(sizeof(struct lsc_sprd_calc_in));
	lsc_calc_out = (struct lsc_sprd_calc_out *)malloc(sizeof(struct lsc_sprd_calc_out));
	tmp_buffer_r = (cmr_u32 *) malloc(32 * 32 * sizeof(cmr_u32));
	tmp_buffer_g = (cmr_u32 *) malloc(32 * 32 * sizeof(cmr_u32));
	tmp_buffer_b = (cmr_u32 *) malloc(32 * 32 * sizeof(cmr_u32));
	tmp_buffer = (cmr_u16 *) malloc(32 * 32 * 4 * sizeof(cmr_u16));

	if(tmp_buffer_r == NULL||tmp_buffer_g == NULL||tmp_buffer_b == NULL||tmp_buffer == NULL||lsc_calc_in == NULL||lsc_calc_out == NULL){
		ISP_LOGE("alsc_do_simulation failure,malloc buffer error!");
		goto EXIT;
	}
	lsc_calc_in->stat_img.r = tmp_buffer_r;
	lsc_calc_in->stat_img.gr = tmp_buffer_g;
	lsc_calc_in->stat_img.gb = tmp_buffer_g;
	lsc_calc_in->stat_img.b = tmp_buffer_b;
	lsc_calc_in->stat_img.w = MAX_STAT_WIDTH;
	lsc_calc_in->stat_img.h = MAX_STAT_HEIGHT;
	lsc_calc_out->dst_gain = tmp_buffer;
	lsc_calc_out->debug_info_ptr = cxt->lsc_debug_info_ptr;
	lsc_calc_out->debug_info_size = cxt->lsc_debug_info_size;

	memcpy(lsc_calc_in->last_lsc_table, cxt->std_init_lsc_table_param_buffer[DEFAULT_TAB_INDEX], cxt->init_gain_width * cxt->init_gain_height * 4 * sizeof(cmr_u16));

	lsc_calc_in->img_width = cxt->init_img_width;
	lsc_calc_in->img_height = cxt->init_img_height;
	lsc_calc_in->gain_width = cxt->init_gain_width;
	lsc_calc_in->gain_height = cxt->init_gain_height;
	lsc_calc_in->grid = cxt->init_grid;
	lsc_calc_in->main_flash_mode = 0;
	lsc_calc_in->captureFlashEnvRatio = 0;
	lsc_calc_in->captureFlash1ofAllRatio = 0;
	lsc_calc_in->preflash_guessing_mainflash_output_table = NULL;
	lsc_calc_in->ct = alsc_do_simulation->ct;
	lsc_calc_in->gridx = cxt->grid;
	lsc_calc_in->gridy = cxt->grid;
	chnl_gain_num = lsc_calc_in->gain_width*lsc_calc_in->gain_height;
	memcpy(lsc_calc_in->stat_img.r, alsc_do_simulation->stat_r, MAX_STAT_WIDTH * MAX_STAT_HEIGHT * sizeof(cmr_u32));
	memcpy(lsc_calc_in->stat_img.gr, alsc_do_simulation->stat_g, MAX_STAT_WIDTH * MAX_STAT_HEIGHT * sizeof(cmr_u32));
	memcpy(lsc_calc_in->stat_img.b, alsc_do_simulation->stat_b, MAX_STAT_WIDTH * MAX_STAT_HEIGHT * sizeof(cmr_u32));

	ISP_LOGI("do_simulation img_size[%d,%d], gain_size[%d,%d], stat_size[%d,%d] grid: %d",lsc_calc_in->img_width,lsc_calc_in->img_height,lsc_calc_in->gain_width,lsc_calc_in->gain_height,lsc_calc_in->stat_img.w,lsc_calc_in->stat_img.h,lsc_calc_in->grid);

	for (i = 0; i < 8; i++)
		lsc_calc_in->lsc_tab[i] = cxt->std_init_lsc_table_param_buffer[i];

	// first alsc calc
	ISP_LOGI("first alsc calc last_lsc_table[%d,%d,%d,%d]",lsc_calc_in->last_lsc_table[0*chnl_gain_num],lsc_calc_in->last_lsc_table[1*chnl_gain_num],lsc_calc_in->last_lsc_table[2*chnl_gain_num],lsc_calc_in->last_lsc_table[3*chnl_gain_num]);
	rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, lsc_calc_in, lsc_calc_out);

	for(i = 0; i < 10; i++){
		memcpy(lsc_calc_in->last_lsc_table, lsc_calc_out->dst_gain, cxt->init_gain_width * cxt->init_gain_height * 4 * sizeof(cmr_u16));
		ISP_LOGI("cycle %d alsc calc last_lsc_table[%d,%d,%d,%d]",i+2,lsc_calc_in->last_lsc_table[0*chnl_gain_num],lsc_calc_in->last_lsc_table[1*chnl_gain_num],lsc_calc_in->last_lsc_table[2*chnl_gain_num],lsc_calc_in->last_lsc_table[3*chnl_gain_num]);
		rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, lsc_calc_in, lsc_calc_out);
	}
	ISP_LOGI("alsc end! lsc_calc_out[%d,%d,%d,%d]",lsc_calc_out->dst_gain[0*chnl_gain_num],lsc_calc_out->dst_gain[1*chnl_gain_num],lsc_calc_out->dst_gain[2*chnl_gain_num],lsc_calc_out->dst_gain[3*chnl_gain_num]);
	// post shading gain
	lsc_sprd_do_postgain(cxt, lsc_calc_out->dst_gain, cxt->init_gain_width, cxt->init_gain_height, alsc_do_simulation->bv, alsc_do_simulation->bv_gain, 0, 0, alsc_do_simulation->sim_output_table);
	memcpy(cxt->output_lsc_table, alsc_do_simulation->sim_output_table, cxt->init_gain_width * cxt->init_gain_height * 4 * sizeof(cmr_u16));
	ISP_LOGI("post process output_lsc_table[%d,%d,%d,%d]",cxt->output_lsc_table[0*chnl_gain_num],cxt->output_lsc_table[1*chnl_gain_num],cxt->output_lsc_table[2*chnl_gain_num],cxt->output_lsc_table[3*chnl_gain_num]);

	//planar2interlace for driver use
	if(cxt->is_planar == 1){
		lsc_table_planar2interlace(alsc_do_simulation->sim_output_table, cxt->init_gain_width,
		cxt->init_gain_height, cxt->output_gain_pattern, cxt->output_gain_pattern);
	}
 EXIT:
	lsc_std_free(tmp_buffer);
	lsc_std_free(tmp_buffer_r);
	lsc_std_free(tmp_buffer_g);
	lsc_std_free(tmp_buffer_b);
	lsc_std_free(lsc_calc_out);
	lsc_std_free(lsc_calc_in);
	cxt->can_update_dest = 1;

}

static void lsc_multi_camera_sync(struct lsc_sprd_ctrl_context *cxt){
	cmr_u32 i, rtn = 0;
	cmr_u32 pre_width = proc_start_gain_w;
	cmr_u32 pre_height = proc_start_gain_h;
	cmr_u32 pre_pattern = proc_start_gain_pattern;
	cmr_u16 lsc_pre_result_scale_table[32 * 32 * 4] = { 0 };
	cmr_u16 lsc_pre_pm0_scale_table[32 * 32 * 4] = { 0 };

	cmr_u32 new_width = cxt->gain_width;
	cmr_u32 new_height = cxt->gain_height;
	cmr_u32 new_pattern = cxt->output_gain_pattern;
	cmr_u16 *lsc_result_sync_new = cxt->lsc_buffer;
	cmr_u32 channel_gain_num = new_height * new_width;

	cmr_u16 pre_pm0_r_tab[32 * 32] = { 0 };
	cmr_u16 pre_pm0_gr_tab[32 * 32] = { 0 };
	cmr_u16 pre_pm0_gb_tab[32 * 32] = { 0 };
	cmr_u16 pre_pm0_b_tab[32 * 32] = { 0 };
	cmr_u16 pre_out_r[32 * 32] = { 0 };
	cmr_u16 pre_out_gr[32 * 32] = { 0 };
	cmr_u16 pre_out_gb[32 * 32] = { 0 };
	cmr_u16 pre_out_b[32 * 32] = { 0 };

	cmr_u16 new_pm0_r_tab[32 * 32] = { 0 };
	cmr_u16 new_pm0_gr_tab[32 * 32] = { 0 };
	cmr_u16 new_pm0_gb_tab[32 * 32] = { 0 };
	cmr_u16 new_pm0_b_tab[32 * 32] = { 0 };
	cmr_u16 new_out_r[32 * 32] = { 0 };
	cmr_u16 new_out_gr[32 * 32] = { 0 };
	cmr_u16 new_out_gb[32 * 32] = { 0 };
	cmr_u16 new_out_b[32 * 32] = { 0 };

	float gain_ratio_g[32 * 32] = { 0 };
	float gain_ratio_pre_rgbg[32 * 32] = { 0 };
	float gain_ratio_new_rgbg[32 * 32] = { 0 };
	float ratio_rgbg[32 * 32] = { 0 };
	float gain_ratio_input_rgbg[32 * 32] = { 0 };
	float gain_ratio_output_rgbg[32 * 32] = { 0 };

	cmr_u8 off_gr = 0, off_r = 1, off_b = 2, off_gb = 3;
	cmr_u32 gr_chnl_off, r_chnl_off, b_chnl_off, gb_chnl_off;


	if (cxt->lsc_buffer == NULL || cxt->lsc_pm0 == NULL) {
		ISP_LOGV("Sync start fail, lsc_buffer %p, lsc_pm0 %p, camera id=%d", cxt->lsc_buffer, cxt->lsc_pm0,cxt->camera_id);
		return;
	}
	if(pre_height == 0||pre_width == 0){
		ISP_LOGD("gain width or gain height is 0,sync return,camera id =%d",cxt->camera_id );
		return;
	}

	rtn = lsc_table_linear_scaler(proc_start_output_table,pre_width,pre_height,lsc_pre_result_scale_table,new_width,new_height,cxt->is_planar);
	if(rtn)
		return;

	rtn = lsc_table_linear_scaler(proc_start_param_table,pre_width,pre_height,lsc_pre_pm0_scale_table,new_width,new_height,cxt->is_planar);
	if(rtn)
		return;

	if(cxt->is_planar == 1){
		lsc_planar_to_channel(new_width, new_height, pre_pattern, pre_pm0_r_tab, pre_pm0_gr_tab, pre_pm0_gb_tab, pre_pm0_b_tab, lsc_pre_pm0_scale_table);
		lsc_planar_to_channel(new_width, new_height, pre_pattern, pre_out_r, pre_out_gr, pre_out_gb, pre_out_b, lsc_pre_result_scale_table);
		lsc_planar_to_channel(new_width, new_height, new_pattern, new_pm0_r_tab, new_pm0_gr_tab, new_pm0_gb_tab, new_pm0_b_tab, cxt->std_lsc_table_param_buffer[0]);
	}else{
		lsc_interlace_to_channel(new_width, new_height, pre_pattern, pre_pm0_r_tab, pre_pm0_gr_tab, pre_pm0_gb_tab, pre_pm0_b_tab, lsc_pre_pm0_scale_table);
		lsc_interlace_to_channel(new_width, new_height, pre_pattern, pre_out_r, pre_out_gr, pre_out_gb, pre_out_b, lsc_pre_result_scale_table);
		lsc_interlace_to_channel(new_width, new_height, new_pattern, new_pm0_r_tab, new_pm0_gr_tab, new_pm0_gb_tab, new_pm0_b_tab, cxt->std_lsc_table_param_buffer[0]);
	}

	ISP_LOGD("pre_pm0_tab(gr_r_b_gb) = [%d, %d, %d, %d]",pre_pm0_gr_tab[0],pre_pm0_r_tab[0],pre_pm0_b_tab[0],pre_pm0_gb_tab[0]);
	ISP_LOGD("new_pm0_tab(gr_r_b_gb) = [%d, %d, %d, %d]",new_pm0_gr_tab[0],new_pm0_r_tab[0],new_pm0_b_tab[0],new_pm0_gb_tab[0]);

	/* new_out_gr & new_out_gb */
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_g[i] = (float)(new_pm0_gb_tab[i] + new_pm0_gr_tab[i]) / (float)(pre_pm0_gb_tab[i] + pre_pm0_gr_tab[i]);
	for(i = 0; i<channel_gain_num; i++){
		new_out_gb[i] = (unsigned short)(pre_out_gb[i] * gain_ratio_g[i]);
		new_out_gr[i] = (unsigned short)(pre_out_gr[i] * gain_ratio_g[i]);
	}

	/* new_out_r */
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_new_rgbg[i] = (float)(new_pm0_r_tab[i] + new_pm0_r_tab[i]) / (float)(new_pm0_gb_tab[i] + new_pm0_gr_tab[i]);
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_pre_rgbg[i] = (float)(pre_pm0_r_tab[i] + pre_pm0_r_tab[i]) / (float)(pre_pm0_gb_tab[i] + pre_pm0_gr_tab[i]);
	for(i = 0; i<channel_gain_num; i++)
		ratio_rgbg[i] = gain_ratio_new_rgbg[i] / gain_ratio_pre_rgbg[i];
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_input_rgbg[i] = (float)(pre_out_r[i] + pre_out_r[i]) / (float)(pre_out_gr[i] + pre_out_gb[i]);
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_output_rgbg[i] = gain_ratio_input_rgbg[i] * ratio_rgbg[i];
	for(i = 0; i<channel_gain_num; i++)
		new_out_r[i] = (unsigned short)((pre_out_gr[i] + pre_out_gb[i])*gain_ratio_g[i]/2.0f * gain_ratio_output_rgbg[i] + 0.5f);

	/* new_out_b */
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_new_rgbg[i] = (float)(new_pm0_b_tab[i] + new_pm0_b_tab[i]) / (float)(new_pm0_gb_tab[i] + new_pm0_gr_tab[i]);
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_pre_rgbg[i] = (float)(pre_pm0_b_tab[i] + pre_pm0_b_tab[i]) / (float)(pre_pm0_gb_tab[i] + pre_pm0_gr_tab[i]);
	for(i = 0; i<channel_gain_num; i++)
		ratio_rgbg[i] = gain_ratio_new_rgbg[i] / gain_ratio_pre_rgbg[i];
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_input_rgbg[i] = (float)(pre_out_b[i] + pre_out_b[i]) / (float)(pre_out_gr[i] + pre_out_gb[i]);
	for(i = 0; i<channel_gain_num; i++)
		gain_ratio_output_rgbg[i] = gain_ratio_input_rgbg[i] * ratio_rgbg[i];
	for(i = 0; i<channel_gain_num; i++)
		new_out_b[i] = (unsigned short)((pre_out_gr[i] + pre_out_gb[i])*gain_ratio_g[i]/2.0f * gain_ratio_output_rgbg[i] + 0.5f);

	ISP_LOGD("pre_in_tab(gr_r_b_gb) = [%d, %d, %d, %d]",pre_out_gr[0],pre_out_r[0],pre_out_b[0],pre_out_gb[0]);
	ISP_LOGD("new_out_tab(gr_r_b_gb) = [%d, %d, %d, %d]",new_out_gr[0],new_out_r[0],new_out_b[0],new_out_gb[0]);

	lsc_get_channel_index(new_pattern, &off_gr, &off_r, &off_b, &off_gb);
	gr_chnl_off = off_gr * new_width * new_height;
	r_chnl_off = off_r * new_width * new_height;
	b_chnl_off = off_b * new_width * new_height;
	gb_chnl_off = off_gb * new_width * new_height;

	if (cxt->is_planar == 0) {
		for (i = 0; i < new_width * new_height; i++) {
			lsc_result_sync_new[4 * i + off_gr] = RANGE(new_out_gr[i], 1024, 16383);
			lsc_result_sync_new[4 * i + off_r] = RANGE(new_out_r[i], 1024, 16383);
			lsc_result_sync_new[4 * i + off_b] = RANGE(new_out_b[i], 1024, 16383);
			lsc_result_sync_new[4 * i + off_gb] = RANGE(new_out_gb[i], 1024, 16383);
		}
	} else {
		for (i = 0; i < new_width * new_height; i++) {
			lsc_result_sync_new[i + gr_chnl_off] = RANGE(new_out_gr[i], 1024, 16383);
			lsc_result_sync_new[i + r_chnl_off] = RANGE(new_out_r[i], 1024, 16383);
			lsc_result_sync_new[i + b_chnl_off] = RANGE(new_out_b[i], 1024, 16383);
			lsc_result_sync_new[i + gb_chnl_off] = RANGE(new_out_gb[i], 1024, 16383);
		}
	}
	ISP_LOGD("Sync end,camra id =%d",cxt->camera_id );
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

	for (i = 0; i < 9; i++) {
		lsc_std_free(cxt->std_init_lsc_table_param_buffer[i]);
		lsc_std_free(cxt->std_lsc_table_param_buffer[i]);
	}
	lsc_std_free(cxt->dst_gain);
	lsc_std_free(cxt->fwstart_new_scaled_table);
	lsc_std_free(cxt->fwstop_output_table);
	lsc_std_free(cxt->lsc_last_info);
	lsc_std_free(cxt->lsc_flash_proc_param);
	lsc_std_free(cxt->last_lsc_table);
	lsc_std_free(cxt->output_lsc_table);
	lsc_std_free(cxt->lsc_buffer_interlace);
	lsc_std_free(cxt->lscm_info);
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

static void *lsc_sprd_init(void *in, void *out)
{
	cmr_s32 rtn = LSC_SUCCESS;
	struct lsc_sprd_ctrl_context *cxt = NULL;
	struct lsc_adv_init_param *init_param = (struct lsc_adv_init_param *)in;
	struct lsc_sprd_init_in sprd_init_param;
	struct lsc_sprd_init_out sprd_init_output_param;
	void *alsc_handle = NULL;
	cmr_s32 free_otp_flag = 1;
	cmr_s32 i;
	UNUSED(out);

	if (init_param->lsc_otp_table_addr != NULL) {
		ISP_LOGE("initparam lsc_otp_table_addr is not NULL, please check the caller");
		free_otp_flag = 0;
		goto ERR_EXIT;
	}

	//create all buffer
	cxt = (struct lsc_sprd_ctrl_context *)malloc(sizeof(struct lsc_sprd_ctrl_context));
	if (NULL == cxt) {
		ISP_LOGE("fail to alloc lsc_sprd_ctrl_context!");
		goto ERR_EXIT;
	}
	memset(cxt, 0, sizeof(*cxt));

	rtn = lsc_malloc_buffer(cxt);
	if (LSC_ALLOC_ERROR == rtn) {
		goto ERR_EXIT;
	}
	cxt->ctrl_handle = (cmr_handle)out;
	lsc_get_prop_cmd(cxt);

	lsc_parser_otp(init_param, cxt);

	// init flash ctrl param
	lsc_flash_proc_init(cxt);

	// get lsc_id
	id1_addr = cxt->lsc_buffer;
	cxt->lsc_id = 1;
	if (id1_addr != NULL && cxt->is_multi_mode == 2 /*ISP_DUAL_SBS */  && cxt->is_master == 0 /*slave */) {
		id2_addr = cxt->lsc_buffer;
		cxt->lsc_id = 2; // when lsc_id=2, lsc calculation will not work for slave camera
	}
	init_param->lsc_buffer_addr = cxt->lsc_buffer;

	//load sprd lib
	cxt->lib_info = &init_param->lib_param;
	rtn = lsc_load_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to load lsc lib");
		goto ERR_EXIT;
	}
	// alsc_init
	cxt->is_planar = 1;	// 1 -- use planar lsc table in alsc algorithm;  0 -- use interlace lsc table in alsc algorithm
	cxt->stats_inverse = 1; // set 0 if use LSCM statistic data; set 1 if use AEM statistic data
#if ((defined CONFIG_ISP_2_7)||(defined CONFIG_ISP_2_8))
	cxt->stats_inverse = 0; //sharkl5pro and sharkl6 use LSCM statistic data
#endif
	rtn = lsc_set_init_param(init_param, cxt, &sprd_init_param);

	sprd_init_output_param.lsc_buffer = NULL;
	for(i = 0; i < 9; i++){
		sprd_init_output_param.lsc_otp_std_tab[i] = cxt->std_init_lsc_table_param_buffer[i];
	}

	alsc_handle = cxt->lib_ops.alsc_init(&sprd_init_param, &sprd_init_output_param);
	if (NULL == alsc_handle) {
		ISP_LOGE("fail to do alsc init!camera_id = %d",init_param->camera_id);
		cxt->alsc_handle = NULL;
		rtn = LSC_ALLOC_ERROR;
		goto ERR_EXIT;
	}
	cxt->lsc_buffer = sprd_init_output_param.lsc_buffer;
	cxt->alsc_handle = alsc_handle;
	cxt->lib_ops.alsc_io_ctrl(alsc_handle, LSC_CMD_GET_LSC_VERSION, (void *)&cxt->LSC_SPD_VERSION, NULL);
	cxt->lib_ops.alsc_io_ctrl(alsc_handle, LSC_CMD_GET_CALCULATION_FREQ, (void *)&cxt->calc_freq, NULL);

	lsc_std_free(init_param->lsc_otp_table_addr);

	pthread_mutex_init(&cxt->status_lock, NULL);

	ISP_LOGI("lsc init success rtn = %d, camera_id = %d", rtn, init_param->camera_id);
	return (void *)cxt;

ERR_EXIT:
	if ((init_param->lsc_otp_table_addr != NULL) && (free_otp_flag == 1)) {
		ISP_LOGW("error happend free lsc_otp_table_addr:%p", init_param->lsc_otp_table_addr);
		lsc_std_free(init_param->lsc_otp_table_addr);
	}

	if (NULL != cxt) {
		rtn = lsc_sprd_deinit(cxt, NULL, NULL);
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
	struct lsc_flash_proc_param *flash_param = NULL;
	cmr_u32 alg_in = 0;
	cmr_u32 img_width, img_height, gain_width, gain_height, grid;
	cmr_u32 IIR_weight = 5;
	struct lsc_sprd_calc_in calc_in;
	struct lsc_sprd_calc_out calc_out;
	struct lsc_last_info *lsc_last_info = NULL;
	struct lsc_otp_convert_param otp_convert_param;
	int debug_index[4];

	if (cxt != NULL)
		flash_param = (struct lsc_flash_proc_param *)cxt->lsc_flash_proc_param;

	if (!handle || !in || !out) {
		ISP_LOGE("fail to check param is NULL!");
		return LSC_ERROR;
	}

	if (cxt->lsc_id == 2 /*slave */  && cxt->is_multi_mode == 2 /*ISP_DUAL_SBS */  && cxt->is_master == 0 /*slave */ ) {
		ISP_LOGV("return due to ISP_DUAL_SBS, slave, lsc_id=%d, camera_id=%d, frame_count=%d", cxt->lsc_id, cxt->camera_id, cxt->frame_count);
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
				ISP_LOGV("change mode: frame_count=%d,camera_id=%d,pre_img[%d,%d],pre_table[%d,%d,%d],new_img[%d,%d],new_table[%d,%d,%d]",
					cxt->frame_count, cxt->camera_id, cxt->img_width, cxt->img_height, cxt->gain_width, cxt->gain_height, cxt->grid, img_width, img_height, gain_width, gain_height, grid);

				cxt->img_width = img_width;
				cxt->img_height = img_height;
				cxt->gain_width = gain_width;
				cxt->gain_height = gain_height;
				cxt->grid = grid;
				cxt->lsc_pm0 = param->lsc_tab_address[0];
				memcpy(cxt->last_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
				memcpy(cxt->output_lsc_table, cxt->fwstart_new_scaled_table, gain_width * gain_height * 4 * sizeof(cmr_u16));
				memcpy(cxt->lsc_buffer, cxt->output_lsc_table, gain_width * gain_height * 4 * sizeof(unsigned short));
				// do lsc param preprocess
				if (cxt->is_planar == 1) {
					for (i = 0; i < 9; i++) {
						memcpy(cxt->std_lsc_table_param_buffer[i], param->lsc_tab_address[i], gain_width * gain_height * 4 * sizeof(cmr_u16));
						lsc_table_interlace2planar(cxt->std_lsc_table_param_buffer[i], gain_width, gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
					}
				} else {
					for (i = 0; i < 9; i++) {
						memcpy(cxt->std_lsc_table_param_buffer[i], param->lsc_tab_address[i], gain_width * gain_height * 4 * sizeof(cmr_u16));
						lsc_interlace_change_pattern(cxt->std_lsc_table_param_buffer[i], gain_width, gain_height, cxt->gain_pattern, cxt->output_gain_pattern);
					}
				}

				// regular tuning param tables by otp
				otp_convert_param.gain_width = gain_width;
				otp_convert_param.gain_height = gain_height;
				otp_convert_param.grid = grid;
				otp_convert_param.gridx = grid;
				otp_convert_param.gridy = grid;
				for (i = 0; i < 9; i++) {
					otp_convert_param.lsc_table[i] = cxt->std_lsc_table_param_buffer[i];
				}
				cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, LSC_CMD_OTP_CONVERT_TABLE, &otp_convert_param, NULL);

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
			memcpy(cxt->lsc_buffer, cxt->output_lsc_table, gain_width * gain_height * 4 * sizeof(unsigned short));
			cxt->fw_start_end = 0;
			cxt->can_update_dest = 1;
			if (cxt->frame_count == 0) {	//lunch camera with binning size or 720p both will run the fw_start
				cxt->frame_count = 1;
				cxt->alg_count = 0;	// set zero to skip iir
				ISP_LOGV("change mode END (return 0): set frame_count=1, set alg_count=0 to do quick in");
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

	debug_index[0] = 0 * gain_width * gain_height;
	debug_index[1] = 1 * gain_width * gain_height;
	debug_index[2] = 2 * gain_width * gain_height;
	debug_index[3] = 3 * gain_width * gain_height;


	if(cxt->camera_id != cxt->ref_camera_id && cxt->sync_flag == 0){
		if(lock==1){
			ISP_LOGD("Sync return,lock stat=%d,camra id =%d",lock,cxt->camera_id );
		}else{
			lsc_multi_camera_sync(cxt);
			ISP_LOGD("lsc multi shift, camera_id=%d, frame_count=%d, ref_camera_id=%d, final output cxt->lsc_buffer[%d,%d,%d,%d]",
				cxt->camera_id,cxt->frame_count,cxt->ref_camera_id,cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);
		}
		goto lsc_calc_exit;
	}else if(cxt->camera_id != cxt->ref_camera_id && cxt->sync_flag == 1 && cxt->sync_count > 0){
		if(lock==1){
			ISP_LOGD("Sync return,lock stat=%d,camra id =%d",lock,cxt->camera_id );
		}else{
			lsc_multi_camera_sync(cxt);
			memcpy(cxt->last_lsc_table, cxt->lsc_buffer, gain_width * gain_height * 4 * sizeof(cmr_u16));
			cxt->sync_count--;
			ISP_LOGD("lsc multi shift, camera_id=%d, frame_count=%d, ref_camera_id=%d, final output cxt->lsc_buffer[%d,%d,%d,%d]",
				cxt->camera_id,cxt->frame_count,cxt->ref_camera_id,cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);
		}
	}
	if(cxt->sync_flag == 1){
		alg_in = 1;
	}

	if (cxt->cmd_alsc_bypass ||  cxt->alg_bypass) {
		alg_in = 0;
	}

	// cmd set lsc output
	if (cxt->cmd_alsc_table_pattern) {
		lsc_cmd_set_output(cxt->lsc_buffer, gain_width, gain_height, cxt->output_gain_pattern);
		if(cxt->is_planar == 1){
			lsc_table_interlace2planar(cxt->lsc_buffer, gain_width, gain_height, cxt->output_gain_pattern, cxt->output_gain_pattern);
		}
		ISP_LOGD("lsc_cmd_set_output, final output cxt->lsc_buffer[%d,%d,%d,%d]",
			cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);
		goto lsc_calc_exit;
	}
	// cmd set table index
	if (cxt->cmd_alsc_table_index < 8) {
		if (cxt->LSC_SPD_VERSION <= 5) {
			memcpy(cxt->lsc_buffer, cxt->std_lsc_table_param_buffer[cxt->cmd_alsc_table_index], gain_width * gain_height * 4 * sizeof(cmr_u16));
		} else {
			memcpy(cxt->lsc_buffer, cxt->std_init_lsc_table_param_buffer[cxt->cmd_alsc_table_index], gain_width * gain_height * 4 * sizeof(cmr_u16));
		}
		ISP_LOGD("cmd set table index %d, final output cxt->lsc_buffer[%d,%d,%d,%d]", cxt->cmd_alsc_table_index,
			 cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);
		goto lsc_calc_exit;
	}

	// alsc calculation process
	if (alg_in) {
		// select std lsc param
		if (cxt->LSC_SPD_VERSION == 6 && cxt->init_gain_width == gain_width && cxt->init_gain_height == gain_height) {
			for (i = 0; i < 8; i++)
				param->std_tab_param[i] = cxt->std_init_lsc_table_param_buffer[i];	// use init lsc table with otp
		} else {
			for (i = 0; i < 8; i++)
				param->std_tab_param[i] = cxt->std_lsc_table_param_buffer[i];	 // use calculation lsc table with otp
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
		calc_in.main_flash_mode = cxt->flash_mode;
		calc_in.captureFlashEnvRatio = flash_param->captureFlashEnvRatio;
		calc_in.captureFlash1ofAllRatio = flash_param->captureFlash1ofALLRatio;
		calc_in.preflash_guessing_mainflash_output_table = flash_param->preflash_guessing_mainflash_output_table;
		calc_in.ct = param->ct;
		calc_in.gridx = param->grid;
		calc_in.gridy = param->grid;
		calc_in.smart_lsc_result = cxt->smart_result;

		calc_out.dst_gain = result->dst_gain;
		calc_out.debug_info_ptr = cxt->lsc_debug_info_ptr;
		calc_out.debug_info_size = cxt->lsc_debug_info_size;

		rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, &calc_in, &calc_out);
		memcpy(cxt->last_lsc_table, calc_out.dst_gain, gain_width * gain_height * 4 * sizeof(unsigned short));

		// post shading gain
		lsc_sprd_do_postgain(cxt, cxt->last_lsc_table, gain_width, gain_height, param->bv, param->bv_gain, cxt->flash_mode, cxt->pre_flash_mode, cxt->output_lsc_table);
		memcpy(cxt->lsc_buffer, cxt->output_lsc_table, gain_width * gain_height * 4 * sizeof(unsigned short));

		if(lock == 0){
			lock = 1;
			ISP_LOGD("sync table, camera id =%d ", cxt->camera_id);
			proc_start_gain_w = cxt->gain_width;
			proc_start_gain_h = cxt->gain_height;
			proc_start_gain_pattern = cxt->output_gain_pattern;
			memcpy(proc_start_output_table, cxt->output_lsc_table, gain_width * gain_height * 4 * sizeof(unsigned short));
			memcpy(proc_start_param_table, cxt->std_lsc_table_param_buffer[0], gain_width * gain_height * 4 * sizeof(unsigned short));
			lock = 0;
		}
		cxt->alg_count++;
	}
	// alsc_calc --

	ISP_LOGD("camera_id=%d, frame_count=%d, alg_in=%d, alg_count=%d, can_update_dest=%d, final output [%d,%d,%d,%d]",
		cxt->camera_id, cxt->frame_count, alg_in, cxt->alg_count, cxt->can_update_dest,
		cxt->lsc_buffer[debug_index[0]], cxt->lsc_buffer[debug_index[1]], cxt->lsc_buffer[debug_index[2]], cxt->lsc_buffer[debug_index[3]]);

lsc_calc_exit:
	cxt->flash_done_frame_count++;
	cxt->frame_count++;
	cxt->alsc_update_flag = 1;
	if (!cxt->can_update_dest)
		cxt->alsc_update_flag = 0;

	if (cxt->cmd_alsc_dump_table) {
		lsc_dump_gain(cxt->lsc_buffer, gain_width, gain_height, cxt->output_gain_pattern, cxt->is_planar, cxt->frame_count, cxt->camera_id);
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
	struct lsc_flash_y_gain_param flash_y_gain_param = {0};
	struct alsc_fwprocstart_info *fwprocstart_info = NULL;
	struct lsc_last_info *lsc_last_info = (struct lsc_last_info *)cxt->lsc_last_info;
	struct lsc_flash_proc_param *flash_param = (struct lsc_flash_proc_param *)cxt->lsc_flash_proc_param;
	SyncState *mult_camera_shift_param =NULL;
	//cmr_u16 *pm0_new = NULL;
	cmr_u8 is_gr, is_gb, is_r, is_b;
	cmr_u32 table_flag = 0;
	cmr_u32 chnl_gain_num = 0;
	cmr_u32 will_do_post_gain = 0;
	cmr_s32 num_read = 0;

	lsc_get_channel_index(cxt->output_gain_pattern, &is_gr, &is_r, &is_b, &is_gb);

	switch (cmd) {
	case ALSC_FW_START:	// You have to update two table in FW_START: 1.fwstart_info->lsc_result_address_new, 2.cxt->fwstart_new_scaled_table
		fwstart_info = (struct alsc_fwstart_info *)in;
		ISP_LOGD("FW_START, frame_count=%d, camera_id=%d, lsc_id=%d, LSC_SPD_VERSION=%d", cxt->frame_count, fwstart_info->camera_id, cxt->lsc_id, cxt->LSC_SPD_VERSION);
		ISP_LOGD("FW_START, old tab0 address=%p, lsc table[%d,%d,%d]", cxt->lsc_pm0, cxt->gain_width, cxt->gain_height, cxt->grid);
		ISP_LOGD("FW_START, new tab0 address=%p, lsc table[%d,%d,%d], image_size[%d,%d]", fwstart_info->lsc_tab_address_new[0],
			 fwstart_info->gain_width_new, fwstart_info->gain_height_new, fwstart_info->grid_new, fwstart_info->img_width_new, fwstart_info->img_height_new);

#if ((defined CONFIG_ISP_2_7)||(defined CONFIG_ISP_2_8))
		lsc_sprd_set_monitor(fwstart_info, cxt->lscm_info);
		lsc_set_monitor(cxt->ctrl_handle, cxt->lscm_info);
#endif

		// read last info
		if (cxt->lsc_id == 1) {
			if (cxt->init_gain_width == fwstart_info->gain_width_new && cxt->init_gain_height == fwstart_info->gain_height_new)
				table_flag = 0; //full or binning image size
			else if (fwstart_info->gain_width_new == 23 && fwstart_info->gain_height_new == 15 && (fwstart_info->grid_new == 32 || fwstart_info->grid_new == 48))
				table_flag = 2; // 1090p or 720p size
			else
				table_flag = 1;	// other size
			num_read = lsc_read_last_info(lsc_last_info, fwstart_info->camera_id, table_flag, cxt->app_mode);
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
				if (cxt->lsc_id == 1 && fwstart_info->gain_width_new == lsc_last_info->gain_width && fwstart_info->gain_height_new == lsc_last_info->gain_height && num_read !=0) {
					ISP_LOGV("FW_START, First Frame Mod, apply lsc_last_info");
					memcpy(fwstart_info->lsc_result_address_new, lsc_last_info->table, lsc_last_info->gain_height * lsc_last_info->gain_width * 4 * sizeof(cmr_u16));
				} else {
					ISP_LOGV("FW_START, First Frame Mod, Send TL84 table");
					memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[DEFAULT_TAB_INDEX], chnl_gain_num * 4 * sizeof(cmr_u16));
					lsc_table_interlace2planar(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->gain_pattern, cxt->output_gain_pattern);
					will_do_post_gain = 1;
				}

				for ( i = 0; i < 8; i++){
					memcpy(cxt->std_lsc_table_param_buffer[i], cxt->std_init_lsc_table_param_buffer[i], cxt->init_gain_height*cxt->init_gain_width*4*sizeof(cmr_u16));
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
					change_mode_rtn = lsc_fwstart_update_first_tab(cxt, fwstart_info, cxt->is_planar);
					ISP_LOGV("FW_START, Normal change mode, change_mode_rtn=%d", change_mode_rtn);
				}
			}
		} else {
			// when cxt->LSC_SPD_VERSION >= 6
			ISP_LOGV("FW_START, cxt->init_img[%d,%d],fwstart_info->img[%d,%d]",cxt->init_img_width,cxt->init_img_height,fwstart_info->img_width_new,fwstart_info->img_height_new);
			lsc_preprocess_fwstart_info(cxt, fwstart_info);

			chnl_gain_num = cxt->gain_width * cxt->gain_height;

			if (cxt->init_img_width * fwstart_info->img_height_new == cxt->init_img_height * fwstart_info->img_width_new) {
				cxt->cur_lsc_pm_mode = 0;	// common table size
			} else if (fwstart_info->img_width_new == 1280 && fwstart_info->img_height_new == 720) {
				cxt->cur_lsc_pm_mode = 1;	// 720p table size
			} else if (fwstart_info->img_width_new == 1920 && fwstart_info->img_height_new == 1080) {
				cxt->cur_lsc_pm_mode = 1;	// 1080p table size
			}

			ISP_LOGV("FW_START, new table size=[%d,%d] grid=%d,frame_count=%d, pre_lsc_pm_mode=%d, cur_lsc_pm_mode=%d", cxt->gain_width, cxt->gain_height, cxt->grid, cxt->frame_count,
				cxt->pre_lsc_pm_mode, cxt->cur_lsc_pm_mode);

			if (flash_param->main_flash_from_other_parameter == 1){
				ISP_LOGV("FW_START, Flash change mode, the same table size, don't need scale, COPY the preflash_current_output_table");
				memcpy(fwstart_info->lsc_result_address_new, flash_param->preflash_current_output_table, chnl_gain_num * 4 * sizeof(cmr_u16));
				flash_param->main_flash_from_other_parameter = 0;
				flash_param->preflash_current_lnc_table_address = NULL;
			}else if(cxt->lsc_id == 1 && cxt->gain_width == lsc_last_info->gain_width && cxt->gain_height == lsc_last_info->gain_height){
				ISP_LOGV("FW_START, the same table size, send last table");
				memcpy(fwstart_info->lsc_result_address_new, lsc_last_info->table, chnl_gain_num * 4 * sizeof(cmr_u16));
			}else{
				ISP_LOGV("FW_START, no last info, send TL84 table");
				memcpy(fwstart_info->lsc_result_address_new, fwstart_info->lsc_tab_address_new[2], chnl_gain_num * 4 * sizeof(cmr_u16));
				will_do_post_gain = 1;
			}
		}
		// main ctrl flow when fw start -----

		flash_y_gain_param.gain_height = fwstart_info->gain_height_new;
		flash_y_gain_param.gain_width = fwstart_info->gain_width_new;
		cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle,LSC_CMD_GENERATE_FLASH_Y_GAIN,&flash_y_gain_param,NULL);


		if (will_do_post_gain == 1) {
			lsc_sprd_do_postgain(cxt, fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, fwstart_info->lsc_result_address_new);
		}
		memcpy(cxt->fwstart_new_scaled_table, fwstart_info->lsc_result_address_new, chnl_gain_num * 4 * sizeof(cmr_u16));
		memcpy(cxt->lsc_buffer, fwstart_info->lsc_result_address_new, chnl_gain_num * 4 * sizeof(cmr_u16));
		//planar2interlace for driver use
		if (cxt->is_planar == 1) {
			lsc_table_planar2interlace(fwstart_info->lsc_result_address_new, fwstart_info->gain_width_new, fwstart_info->gain_height_new,
				cxt->output_gain_pattern, cxt->output_gain_pattern);
		}
		ISP_LOGV("FW_START -----, cxt->can_update_dest=%d", cxt->can_update_dest);
		break;

	case ALSC_FW_START_END:
		fwstart_info = (struct alsc_fwstart_info *)in;

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

		ISP_LOGV("FW_START_END, frame_count=%d, camera_id=%d, fw_start_end=%d, can_update_dest=%d", cxt->frame_count, cxt->camera_id,
			cxt->fw_start_end, cxt->can_update_dest);
		break;

	case ALSC_FW_STOP:
		//copy the output to last_info
		chnl_gain_num = cxt->gain_width * cxt->gain_height;

		if (cxt->lsc_id == 1) {
			if (cxt->init_gain_width == cxt->gain_width && cxt->init_gain_height == cxt->gain_height)
				table_flag = 0; // full or binning size
			else if (cxt->gain_width == 23 && cxt->gain_height == 15 && (cxt->grid == 32 || cxt->grid == 48))
				table_flag = 2; // 1080p or 720p
			else
				table_flag = 1; // other size
			memcpy(lsc_last_info->table, cxt->lsc_buffer, chnl_gain_num * 4 * sizeof(cmr_u16));
			lsc_last_info->gain_width = cxt->gain_width;
			lsc_last_info->gain_height = cxt->gain_height;
			lsc_save_last_info(lsc_last_info, cxt->camera_id, table_flag, cxt->app_mode);
		}
		// let calc not to update the result->dst_gain
		cxt->can_update_dest = 0;
		cxt->alsc_update_flag = 0;
		ISP_LOGV("FW_STOP, can_update_dest=%d, alsc_update_flag=%d, is_touch_preflash=%d", cxt->can_update_dest, cxt->alsc_update_flag, flash_param->is_touch_preflash);

		// flash mode
		if (flash_param->is_touch_preflash == 0 || flash_param->is_touch_preflash == 1) {
			memcpy(cxt->fwstop_output_table, flash_param->preflash_guessing_mainflash_output_table, chnl_gain_num * 4 * sizeof(cmr_u16));
			// normal mode
		} else {
			memcpy(cxt->fwstop_output_table, cxt->lsc_buffer, chnl_gain_num * 4 * sizeof(unsigned short));
		}

		flash_param->is_touch_preflash = -99;

		// flash capture end, prepare to back to preview, if the address not equal, means we take flash shot on different parameter file between the preflash one.
		if (cxt->flash_mode == 1 && flash_param->preflash_current_lnc_table_address != cxt->lsc_pm0 && flash_param->preflash_current_lnc_table_address != NULL) {
			// set main_flash_from_other_parameter will let the next fwstart_update_first_tab to select preflash_current_output_table when back to preview
			flash_param->main_flash_from_other_parameter = 1;
		} else {
			flash_param->main_flash_from_other_parameter = 0;
		}

		ISP_LOGV("FW_STOP, frame_count=%d, camera_id=%d, main_flash_from_other_parameter=%d", cxt->frame_count, cxt->camera_id,
			flash_param->main_flash_from_other_parameter);
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

		ISP_LOGD("FLASH_PRE_BEFORE, flash_done_frame_count=%d, pre_flash_before_ae_touch_framecount=%d, pre_flash_before_framecount=%d, is_touch_preflash=%d",
			 cxt->flash_done_frame_count, flash_param->pre_flash_before_ae_touch_framecount, flash_param->pre_flash_before_framecount, flash_param->is_touch_preflash);

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
		chnl_gain_num = cxt->gain_height * cxt->gain_width;

		// obtain the mainflash guessing table, use the output from preflash (without post gain)
		memcpy(flash_param->preflash_guessing_mainflash_output_table, cxt->output_lsc_table, cxt->gain_width * cxt->gain_height * 4 * sizeof(cmr_u16));

		// reset
		flash_param->ae_touch_framecount = -99;
		flash_param->pre_flash_before_ae_touch_framecount = -99;
		flash_param->pre_flash_before_framecount = -99;

		cxt->bv_skip_frame = 5;  // use bv_before_flash and bv_gain_before_flash some frames after pre-flash

		// force lsc_buffer to take effect in next ALSC_GET_UPDATE_INFO io-ctrl
		cxt->can_update_dest = 1;
		cxt->alsc_update_flag = 1;

		//recover the lsc table which is saved before pre_flash (with post gain)
		memcpy(cxt->lsc_buffer, flash_param->preflash_current_output_table,cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));

		ISP_LOGD("FLASH_PRE_AFTER, camera_id=%d, use preflash_current_output_table=[%d,%d,%d,%d]", cxt->camera_id, cxt->lsc_buffer[0*chnl_gain_num], cxt->lsc_buffer[1*chnl_gain_num], cxt->lsc_buffer[2*chnl_gain_num], cxt->lsc_buffer[3*chnl_gain_num]);
		break;

	case ALSC_FLASH_MAIN_LIGHTING:
		ISP_LOGV("FLASH_MAIN_LIGHTING, Do nothing, frame_count=%d, camera_id=%d", cxt->frame_count, cxt->camera_id);
		break;

	case ALSC_FLASH_PRE_LIGHTING:
		ISP_LOGV("FLASH_PRE_LIGHTING, Do nothing, frame_count=%d, camera_id=%d", cxt->frame_count, cxt->camera_id);
		break;

	case ALSC_FLASH_MAIN_BEFORE:
		if (cxt->lsc_buffer == NULL || cxt->frame_count == 0) {
			ISP_LOGV("FLASH_MAIN_BEFORE, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
			return rtn;
		}
		// stop calc update the dest buffer
		cxt->can_update_dest = 0;
		cxt->alsc_update_flag = 0;

		// set flash_mode will quick in
		cxt->flash_mode = 1;
		break;

	case ALSC_FLASH_MAIN_AFTER:
		if (cxt->lsc_buffer == NULL || cxt->frame_count == 0) {
			ISP_LOGV("FLASH_MAIN_AFTER, frame_count=0, return 0 do nothing, lsc_id=%d", cxt->lsc_id);
			return rtn;
		}
		chnl_gain_num = cxt->gain_height * cxt->gain_width;
		// set status
		cxt->flash_mode = 0;

		// quick in and set bv_skip_frame
		cxt->alg_quick_in = 1;
		cxt->quik_in_start_frame = -99;
		cxt->bv_skip_frame = 10;     // use bv_before_flash and bv_gain_before_flash some frames after main-flash
		cxt->flash_done_frame_count = 0;

		// force lsc_buffer to take effect in next ALSC_GET_UPDATE_INFO io-ctrl
		cxt->can_update_dest = 1;
		cxt->alsc_update_flag = 1;

		// reset parameter
		flash_param->captureFlashEnvRatio = 0.0;
		flash_param->captureFlash1ofALLRatio = 0.0;
		flash_param->is_touch_preflash = -99;

		//recover the lsc table which is saved before pre_flash (with post gain)
		memcpy(cxt->lsc_buffer, flash_param->preflash_current_output_table,cxt->gain_width*cxt->gain_height*4*sizeof(cmr_u16));

		ISP_LOGD("FLASH_MAIN_AFTER, camera_id=%d, use preflash_current_output_table=[%d,%d,%d,%d]", cxt->camera_id, cxt->lsc_buffer[0*chnl_gain_num], cxt->lsc_buffer[1*chnl_gain_num], cxt->lsc_buffer[2*chnl_gain_num], cxt->lsc_buffer[3*chnl_gain_num]);

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

		if (cxt->lsc_id == 1) {
			if (cxt->init_gain_width == fwprocstart_info->gain_width_new && cxt->init_gain_height == fwprocstart_info->gain_height_new)
				table_flag = 0; // full or binning size
			else if (fwprocstart_info->gain_width_new == 23 && fwprocstart_info->gain_height_new == 15 && (fwprocstart_info->grid_new == 32 || fwprocstart_info->grid_new == 48))
				table_flag = 2; //1080p or 720p
			else
				table_flag = 1;	// other size
			num_read = lsc_read_last_info(lsc_last_info, fwprocstart_info->camera_id, table_flag, cxt->app_mode);
			cxt->fw_start_bv = lsc_last_info->bv;
			cxt->fw_start_bv_gain = lsc_last_info->bv_gain;
		}

		if (cxt->LSC_SPD_VERSION <= 5) {
			//pm0_new = fwprocstart_info->lsc_tab_address_new[0];

			if (cxt->is_multi_mode == 2) {
				ISP_LOGV("FW_PROC_START, ISP_DUAL_SBS MODE, camera_id=%d", fwprocstart_info->camera_id);
				ISP_LOGV("FW_PROC_START, old lsc table[%d,%d,%d]", cxt->gain_width, cxt->gain_height, cxt->grid);
				ISP_LOGV("FW_PROC_START, new lsc table[%d,%d,%d]", fwprocstart_info->gain_width_new,fwprocstart_info->gain_height_new, fwprocstart_info->grid_new);

				chnl_gain_num = fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new;

				if (cxt->lsc_id == 1) {
					// get the master capture parameter info
					proc_start_gain_w = fwprocstart_info->gain_width_new;
					proc_start_gain_h = fwprocstart_info->gain_height_new;
					proc_start_gain_pattern = cxt->output_gain_pattern;
					memcpy(proc_start_param_table, fwprocstart_info->lsc_tab_address_new[0], proc_start_gain_w * proc_start_gain_h * 4 * sizeof(cmr_u16));
					lsc_table_interlace2planar(proc_start_param_table, fwprocstart_info->gain_width_new, fwprocstart_info->gain_height_new,
						fwprocstart_info->image_pattern_new, cxt->output_gain_pattern);
					rtn = lsc_table_linear_scaler(cxt->output_lsc_table, cxt->gain_width, cxt->gain_height, proc_start_output_table, proc_start_gain_w, proc_start_gain_h, 1);
					if(rtn)
						return -1;

					ISP_LOGV("FW_PROC_START, master table size(%d,%d)", proc_start_gain_w, proc_start_gain_h);
					//ISP_LOGV("FW_PROC_START, master output table=[%d,%d,%d,%d]", proc_start_output_table[0],
					//      proc_start_output_table[1], proc_start_output_table[2], proc_start_output_table[3]);
					ISP_LOGV("FW_PROC_START, master DNP table=[%d,%d,%d,%d]", proc_start_param_table[0*chnl_gain_num],
						 proc_start_param_table[1*chnl_gain_num], proc_start_param_table[2*chnl_gain_num], proc_start_param_table[3*chnl_gain_num]);
				} else if (cxt->lsc_id == 2) {
					ISP_LOGV("FW_PROC_START, Get master table size (%d,%d)", proc_start_gain_w, proc_start_gain_h);
					ISP_LOGV("FW_PROC_START, Get master output table=[%d,%d,%d,%d]", proc_start_output_table[0*chnl_gain_num],
						 proc_start_output_table[1*chnl_gain_num], proc_start_output_table[2*chnl_gain_num], proc_start_output_table[3*chnl_gain_num]);
					ISP_LOGV("FW_PROC_START, Get master DNP table=[%d,%d,%d,%d]", proc_start_param_table[0*chnl_gain_num],
						 proc_start_param_table[1*chnl_gain_num], proc_start_param_table[2*chnl_gain_num], proc_start_param_table[3*chnl_gain_num]);

					lsc_master_slave_sync(cxt, fwprocstart_info);
				}
			} else {
				ISP_LOGV("FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do nothing.");
				chnl_gain_num = fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new;
				if (cxt->lsc_id == 1 && num_read != 0
				    && fwprocstart_info->gain_width_new == lsc_last_info->gain_width && fwprocstart_info->gain_height_new == lsc_last_info->gain_height) {
					memcpy(fwprocstart_info->lsc_result_address_new, lsc_last_info->table, chnl_gain_num * 4 * sizeof(cmr_u16));
					ISP_LOGV("FW_PROC_START, use last_table_rgb");
				} else {
					ISP_LOGV("FW_PROC_START, use TL84 table");
					memcpy(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_tab_address_new[DEFAULT_TAB_INDEX],
					       chnl_gain_num * 4 * sizeof(cmr_u16));
					lsc_table_interlace2planar(fwprocstart_info->lsc_result_address_new, fwprocstart_info->gain_width_new, fwprocstart_info->gain_height_new,
						fwprocstart_info->image_pattern_new, cxt->output_gain_pattern);

					lsc_sprd_do_postgain(cxt, fwprocstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, fwprocstart_info->lsc_result_address_new);
				}

				//keep the update for calc to as a source for inversing static data
				memcpy(cxt->fwstart_new_scaled_table, fwprocstart_info->lsc_result_address_new, chnl_gain_num * 4 * sizeof(cmr_u16));
			}
		} else {
			// when cxt->LSC_SPD_VERSION >= 6
			ISP_LOGV("FW_PROC_START, NOT ISP_DUAL_SBS MODE, Do nothing.");
			chnl_gain_num = fwprocstart_info->gain_width_new * fwprocstart_info->gain_height_new;

			for (i = 0; i < 8; i++) {
				fwprocstart_info->lsc_tab_address_new[i] = cxt->std_init_lsc_table_param_buffer[i];
			}

			if (cxt->lsc_id == 1 && num_read != 0 && fwprocstart_info->gain_width_new == lsc_last_info->gain_width && fwprocstart_info->gain_height_new == lsc_last_info->gain_height) {
				memcpy(fwprocstart_info->lsc_result_address_new, lsc_last_info->table, chnl_gain_num * 4 * sizeof(cmr_u16));
			} else {
				memcpy(fwprocstart_info->lsc_result_address_new, fwprocstart_info->lsc_tab_address_new[DEFAULT_TAB_INDEX], chnl_gain_num * 4 * sizeof(cmr_u16));

				lsc_sprd_do_postgain(cxt, fwprocstart_info->lsc_result_address_new, cxt->gain_width, cxt->gain_height, cxt->fw_start_bv, cxt->fw_start_bv_gain, 0, 0, fwprocstart_info->lsc_result_address_new);
			}

			//keep the update for calc to as a source for inversing static data
			memcpy(cxt->fwstart_new_scaled_table, fwprocstart_info->lsc_result_address_new, chnl_gain_num * 4 * sizeof(cmr_u16));
		}

		flash_y_gain_param.gain_height = fwprocstart_info->gain_height_new;
		flash_y_gain_param.gain_width = fwprocstart_info->gain_width_new;
		cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle,LSC_CMD_GENERATE_FLASH_Y_GAIN,&flash_y_gain_param,NULL);

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
		alsc_do_simulation(handle,in);
		break;

	case LSC_SET_MULTI_SWITCH_INFO:
		if (!in) {
			ISP_LOGE("fail to check param, SyncState is NULL!");
			return LSC_ERROR;
		}
		mult_camera_shift_param = (SyncState *)in;
		cxt->ref_camera_id = mult_camera_shift_param->mRefId;
		cxt->next_camera_id = mult_camera_shift_param->mNestId;
		cxt->sync_flag = mult_camera_shift_param->mSyncFlag;
		if(cxt->sync_flag == 1){
			cxt->sync_count = 1;
		}
		ISP_LOGD("ref_camera_id=%d, next_camera_id=%d, sync_flag=%d",cxt->ref_camera_id,cxt->next_camera_id,cxt->sync_flag);
		break;

	case ALSC_SET_SMART_RESULT:
		cxt->smart_result = in;
		break;

	case ALSC_SET_APP_MODE:
		cxt->app_mode = *(cmr_u32 *) in;
		ISP_LOGD("app_mode=%d", cxt->app_mode);
		break;

	default:
		ISP_LOGV("IO_CTRL, NO cmd=%d", cmd);
		break;
	}

	return rtn;
}

struct adpt_ops_type lsc_sprd_adpt_ops_ver1 = {
	.adpt_init = lsc_sprd_init,
	.adpt_deinit = lsc_sprd_deinit,
	.adpt_process = lsc_sprd_calculation,
	.adpt_ioctrl = lsc_sprd_ioctrl,
};
