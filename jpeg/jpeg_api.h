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
#ifndef _JPEG_API_H_
//#define _CMR_OEM_H_
#define _JPEG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long jpg_uint;
typedef long jpg_int;
typedef uint64_t jpg_u64;
typedef int64_t jpg_s64;
typedef unsigned int jpg_u32;
typedef int jpg_s32;
typedef unsigned short jpg_u16;
typedef short jpg_s16;
typedef unsigned char jpg_u8;
typedef char jpg_s8;
typedef void *jpg_handle;

typedef struct jpeg_codec_caller_handle {
    /*for the device OEM layer owned*/

    void *jpeg_cxt;

    void *reserved;

} JPEG_CODEC_CALLER_T;

typedef enum {
    JPEGENC_YUV_420 = 0,
    JPEGENC_YUV_422,
    JPEGENC_YUV_400 = 4,
    JPEGENC_YUV_MAX
} JPEGENC_FORMAT_E;
typedef enum {
    JPEGENC_QUALITY_LOW = 0,     // bad
    JPEGENC_QUALITY_MIDDLE_LOW,  // poor
    JPEGENC_QUALITY_MIDDLE,      // good
    JPEGENC_QUALITY_MIDDLE_HIGH, // excellent
    JPEGENC_QUALITY_HIGH,        // oustanding
    JPEGENC_QUALITY_MAX
} JPEGENC_QUALITY_E;

/***********************/
struct yuvbuf_rect {
    jpg_u32 start_x;
    jpg_u32 start_y;
    jpg_u32 width;
    jpg_u32 height;
};

struct yuvbuf_size {
    jpg_u32 width;
    jpg_u32 height;
};

struct yuvbuf_addr {
    jpg_uint addr_y;
    jpg_uint addr_u;
    jpg_uint addr_v;
};

struct yuvbuf_data_end {
    jpg_u8 y_endian;
    jpg_u8 uv_endian;
    jpg_u8 reserved0;
    jpg_u8 reserved1;
    // jpg_u32                                 padding;
};

struct yuvbuf_info {
    jpg_u32 channel_id;
    jpg_u32 frame_id;
    jpg_u32 frame_real_id;
    jpg_u32 height;
    jpg_uint sec;
    jpg_uint usec;
    jpg_u32 length;
    jpg_u32 free;
    jpg_u32 base;
    jpg_u32 fmt;
};

struct yuvbuf_frm {
    jpg_u32 fmt;
    jpg_u32 buf_size;
    struct yuvbuf_rect rect;
    struct yuvbuf_size size;
    struct yuvbuf_addr addr_phy;
    struct yuvbuf_addr addr_vir;
    jpg_u32 fd;
    struct yuvbuf_data_end data_end;
    jpg_u32 format_pattern;
    void *reserved;
};

/***********************/
struct jpg_op_mean {
    jpg_u32 slice_height;
    jpg_u32 slice_mode;
    jpg_u32 ready_line_num;
    jpg_u32 slice_num;
    jpg_u32 is_sync;
    jpg_u32 is_thumb;
    jpg_u8 mirror;
    jpg_u8 flip;
    jpg_u8 rotation;
    jpg_u32 quality_level;
    jpg_uint out_param;
    struct yuvbuf_frm temp_buf;
};
/******************************* jpeg data type *******************************/
struct jpeg_enc_cb_param {
    jpg_uint stream_buf_phy;
    jpg_uint stream_buf_vir;
    jpg_u32 stream_size;
    jpg_u32 slice_height;
    jpg_u32 total_height;
    jpg_u32 is_thumbnail;
};

struct jpeg_dec_cb_param {
    jpg_u32 slice_height;
    jpg_u32 total_height;
    struct yuvbuf_frm *src_img;
    struct yuvbuf_data_end data_endian;
};

#define CMR_EVT_JPEG_BASE (1 << 20)
enum jpg_jpeg_evt {
    CMR_JPEG_ENC_DONE = CMR_EVT_JPEG_BASE,
    CMR_JPEG_DEC_DONE,
    CMR_JPEG_WEXIF_DONE,
    CMR_JPEG_ENC_ERR,
    CMR_JPEG_DEC_ERR,
    CMR_JPEG_ERR,
};

typedef void (*jpg_evt_cb_ptr)(enum jpg_jpeg_evt evt, void *data,
                               void *privdata);

jpg_int sprd_jpeg_init(JPEG_CODEC_CALLER_T *oem_handle,
                       jpg_evt_cb_ptr jpg_evt_cb);
jpg_int sprd_jpeg_deinit(JPEG_CODEC_CALLER_T *oem_handle);
jpg_int sprd_jpg_encode(JPEG_CODEC_CALLER_T *oem_handle, struct yuvbuf_frm *src,
                        struct yuvbuf_frm *dst, struct jpg_op_mean *mean, struct jpeg_enc_cb_param *enc_cb_param);
jpg_int sprd_jpg_decode(JPEG_CODEC_CALLER_T *oem_handle, struct yuvbuf_frm *src,
                        struct yuvbuf_frm *dst, struct jpg_op_mean *mean);
jpg_int sprd_stop_codec(JPEG_CODEC_CALLER_T *oem_handle);
jpg_int sprd_jpg_get_Iommu_status(JPEG_CODEC_CALLER_T *oem_handle);
jpg_int sprd_jpg_dec_get_resolution(unsigned char *jpg_src, int jpg_size,
                                    unsigned int *wdith, unsigned int *height,
                                    unsigned int *yuv_buffer_size);
jpg_int sprd_jpg_set_resolution(void *jpg_buf, int jpg_size, int width,
                                int height);

#ifdef __cplusplus
}
#endif

#endif
