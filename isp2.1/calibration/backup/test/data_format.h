#ifndef _DATA_FORMAT_H_
#define _DATA_FORMAT_H_

#include "sci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void write_data_uint16(const char *file_name, cmr_u16 * data, cmr_u32 size);
void write_data_double(const char *file_name, double *data, cmr_u32 item_per_line, cmr_u32 items);
void write_data_uint16_dec(const char *file_name, cmr_u16 * data, cmr_u32 item_per_line, cmr_u32 size);
void write_data_uint16_interlace(const char *file_name, cmr_u16 * data, cmr_u32 size);
cmr_u32 read_data_uint16(const char *file_name, cmr_u16 * data, cmr_u32 max_size);
cmr_u32 read_file(const char *file_name, void *data_buf, cmr_u32 buf_size);
cmr_s32 save_file(const char *file_name, void *data, cmr_u32 data_size);
cmr_s32 image_blc(cmr_u16 * dst, cmr_u16 * src, cmr_u32 width, cmr_u32 height, cmr_u32 bayer_pattern, cmr_u16 blc_gr, cmr_u16 blc_r, cmr_u16 blc_b, cmr_u16 blc_gb);
void split_bayer_raw(cmr_u16 * dst[4], cmr_u16 * src, cmr_u32 w, cmr_u32 h);

#ifdef __cplusplus
}
#endif
#endif
