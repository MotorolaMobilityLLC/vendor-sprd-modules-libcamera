#include "sci_types.h"
#include <stdio.h>

void write_data_uint16(const char *file_name, cmr_u16 *data, cmr_u32 size)
{
	FILE *pf = NULL;
	cmr_u32 items = size / sizeof(cmr_u16);
	cmr_u32 item_per_line = 16;
	cmr_u32 lines = items / item_per_line;
	cmr_u32 i, j;

	pf = fopen(file_name, "w+t");
	if (NULL == pf)
		return;

	for (i=0; i<lines; i++) {

		for (j=0; j<item_per_line; j++) {
			fprintf(pf, "0x%04x, ", *data++);
		}

		fprintf(pf, "\n");

		items -= item_per_line;
	}

	if (items > 0) {
		for (j=0; j<items; j++) {
			fprintf(pf, "0x%04x, ", *data++);
		}
		fprintf(pf, "\n");
	}

	fclose(pf);
	pf = NULL;

}

void write_data_uint16_dec(const char *file_name, cmr_u16 *data, cmr_u32 item_per_line, cmr_u32 size)
{
	FILE *pf = NULL;
	cmr_u32 items = size / sizeof(cmr_u16);
	cmr_u32 lines = items / item_per_line;
	cmr_u32 i, j;

	pf = fopen(file_name, "w+t");
	if (NULL == pf)
		return;

	for (i=0; i<lines; i++) {

		for (j=0; j<item_per_line; j++) {
			fprintf(pf, "%04d, ", *data++);
		}

		fprintf(pf, "\n");

		items -= item_per_line;
	}

	if (items > 0) {
		for (j=0; j<items; j++) {
			fprintf(pf, "%04d, ", *data++);
		}
		fprintf(pf, "\n");
	}

	fclose(pf);
	pf = NULL;

}

void write_data_double(const char *file_name, double *data, cmr_u32 item_per_line, cmr_u32 items)
{
	FILE *pf = NULL;
	cmr_u32 lines = items / item_per_line;
	cmr_u32 i, j;

	pf = fopen(file_name, "w+t");
	if (NULL == pf)
		return;

	for (i=0; i<lines; i++) {

		for (j=0; j<item_per_line; j++) {
			fprintf(pf, "%.4f, ", *data++);
		}

		fprintf(pf, "\n");

		items -= item_per_line;
	}

	if (items > 0) {
		for (j=0; j<items; j++) {
			fprintf(pf, "%.4f, ", *data++);
		}
		fprintf(pf, "\n");
	}

	fclose(pf);
	pf = NULL;

}

void split_bayer_raw(cmr_u16 *dst[4], cmr_u16 *src, cmr_u32 w, cmr_u32 h)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;

	cmr_u16 *chn[4] = {NULL};

	chn[0] = dst[0];
	chn[1] = dst[1];
	chn[2] = dst[2];
	chn[3] = dst[3];

	for (i=0; i<h * w; i++) {

		*chn[0]++ = *src++;
		*chn[1]++ = *src++;
		*chn[2]++ = *src++;
		*chn[3]++ = *src++;
	}
}

void merge_bayer_raw(cmr_u16 *dst, cmr_u16 *src[4], cmr_u32 w, cmr_u32 h)
{
	cmr_u32 i = 0;
	cmr_u32 j = 0;

	for (i=0; i<h; i++) {

		for (j=0; j<w; j++) {

			cmr_u32 idx = (i & 1) * 2 + (j & 1);

			*dst++ = *src[idx];
			src[idx]++;
		}
	}
}

cmr_u32 read_data_uint16(const char *file_name, cmr_u16 *data, cmr_u32 max_size)
{
	FILE *pf = NULL;
	char c0 = 0, c1 = 0;
	cmr_u32 val = 0;
	cmr_u32 data_num = 0;
	cmr_s32 rtn = 0;

	pf = fopen(file_name, "rt");
	if (NULL == pf)
		return;


	while (EOF != (c0 = getc(pf))) {

		if ('0' == c0) {

			if (EOF == (c1 = getc(pf)))
				break;

			if ('x' == c1) {

				if (EOF == fscanf(pf, "%x", &val))
					break;

				*data++ = val;
				data_num++;
			}
		}
	}

	fclose(pf);
	pf = NULL;

	return data_num * sizeof(cmr_u16);
}

void write_data_uint16_interlace(const char *file_name, cmr_u16 *data, cmr_u32 size)
{
	FILE *pf = NULL;
	cmr_u32 items = size / sizeof(cmr_u16);
	cmr_u32 item_per_line = 16;
	cmr_u32 lines = items / item_per_line;
	cmr_u32 i, j;

	uint16* tmp0 = data + 0 * items/4;
	uint16* tmp1 = data + 1 * items/4;
	uint16* tmp2 = data + 2 * items/4;
	uint16* tmp3 = data + 3 * items/4;


	pf = fopen(file_name, "w+t");
	if (NULL == pf)
		return;

	for (i=0; i<lines; i++) {

		for (j=0; j<item_per_line/4; j++) {
			fprintf(pf, "0x%04x, ", *tmp1++);
			fprintf(pf, "0x%04x, ", *tmp0++);

			fprintf(pf, "0x%04x, ", *tmp3++);
			fprintf(pf, "0x%04x, ", *tmp2++);
		}

		fprintf(pf, "\n");

		items -= item_per_line;
	}

	if (items > 0) {
		for (j=0; j<items/4; j++) {
			fprintf(pf, "0x%04x, ", *tmp0++);
			fprintf(pf, "0x%04x, ", *tmp1++);
			fprintf(pf, "0x%04x, ", *tmp2++);
			fprintf(pf, "0x%04x, ", *tmp3++);
		}
		fprintf(pf, "\n");
	}

	fclose(pf);
	pf = NULL;

}

cmr_u32 read_file(const char* file_name, void *data_buf,cmr_u32 buf_size)
{
	FILE* pf = NULL;
	cmr_u32 file_len = 0;
	pf = fopen(file_name,"rb");

	if (NULL == pf || NULL == data_buf)
		return 0;

	fseek(pf,0,SEEK_END);
	file_len = ftell(pf);
	fseek(pf,0,SEEK_SET);

	if (buf_size >= file_len)
		file_len = fread(data_buf, 1, file_len, pf);
	else
		file_len = 0;

	fclose(pf);

	return file_len;
}

cmr_s32 save_file(const char *file_name, void *data, cmr_u32 data_size)
{
	FILE *pf = fopen(file_name, "wb");
	cmr_u32 write_bytes = 0;

	if (NULL == pf)
		return 0;

	write_bytes = fwrite(data, 1, data_size, pf);
	fclose(pf);

	return write_bytes;
}

cmr_s32 image_blc(cmr_u16 *dst, cmr_u16 *src, cmr_u32 width, cmr_u32 height, cmr_u32 bayer_pattern,
				 cmr_u16 blc_gr, cmr_u16 blc_r, cmr_u16 blc_b, cmr_u16 blc_gb)
{
	cmr_u16 blc[4] = {0};
	cmr_u16 i = 0;
	cmr_u16 j = 0;

	switch (bayer_pattern) {
	case 0:
		blc[0] = blc_gr;
		blc[1] = blc_r;
		blc[2] = blc_b;
		blc[3] = blc_gb;
		break;

	case 1:
		blc[0] = blc_r;
		blc[1] = blc_gr;
		blc[2] = blc_gb;
		blc[3] = blc_b;
		break;

	case 2:
		blc[0] = blc_b;
		blc[1] = blc_gb;
		blc[2] = blc_gr;
		blc[3] = blc_r;
		break;

	case 3:
		blc[0] = blc_gb;
		blc[1] = blc_b;
		blc[2] = blc_r;
		blc[3] = blc_gr;
		break;

	default:
		break;
	}

	for (i=0; i<height; i++) {
		for (j=0; j<width; j++) {

			cmr_u32 pattern_index = (i & 1) * 2 + (j & 1);
			cmr_u16 src_value = *src++;
			cmr_u16 dst_value = 0;

			if (src_value > blc[pattern_index])
				dst_value = src_value - blc[pattern_index];
			else
				dst_value = 0;

			*dst++ = dst_value;
		}
	}

	return 0;
}

