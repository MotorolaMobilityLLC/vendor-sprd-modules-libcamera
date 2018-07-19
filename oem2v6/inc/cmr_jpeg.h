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
#ifndef _CMR_JPEG_H_
#define _CMR_JPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "jpeg_api.h"
#include "cmr_common.h"

struct jpeg_lib_ops {
    jpg_int (*jpeg_init)(JPEG_CODEC_CALLER_T *oem_handle,
                         jpg_evt_cb_ptr jpg_evt_cb);
    jpg_int (*jpeg_deinit)(JPEG_CODEC_CALLER_T *oem_handle);
    jpg_int (*jpeg_encode)(JPEG_CODEC_CALLER_T *oem_handle,
                           struct yuvbuf_frm *src, struct yuvbuf_frm *dst,
                           struct jpg_op_mean *mean,
                           struct jpeg_enc_cb_param *enc_cb_param);
    jpg_int (*jpeg_decode)(JPEG_CODEC_CALLER_T *oem_handle,
                           struct yuvbuf_frm *src, struct yuvbuf_frm *dst,
                           struct jpg_op_mean *mean);
    jpg_int (*jpeg_stop)(JPEG_CODEC_CALLER_T *oem_handle);
    jpg_int (*jpeg_get_iommu_status)(JPEG_CODEC_CALLER_T *oem_handle);
    jpg_int (*jpeg_dec_get_resolution)(unsigned char *jpg_src,
                                       unsigned int *wdith,
                                       unsigned int *height,
                                       unsigned int *yuv_buffer_size);
    jpg_int (*jpeg_set_resolution)(void *jpg_buf, int jpg_size, int width,
                                   int height);
};

struct jpeg_lib_cxt {
    void *mLibHandle;
    struct jpeg_codec_caller_handle *codec_handle;
    struct jpeg_lib_ops ops;
};

enum cmr_jpeg_ret {
    JPEG_CODEC_SUCCESS = 0,
    JPEG_CODEC_PARAM_ERR,
    JPEG_CODEC_INVALID_HANDLE,
    JPEG_CODEC_NO_MEM,
    JPEG_CODEC_ENC_WAIT_SRC,
    JPEG_CODEC_ERROR,
    JPEG_CODEC_STOP
};

struct jpeg_wexif_cb_param {
    cmr_uint output_buf_virt_addr;
    cmr_uint output_buf_size;
};
#define JPEG_WEXIF_TEMP_MARGIN (21 * 1024)

// only support YUV slice, do not support stream slice for simplicity.
struct jpeg_enc_in_param {
    cmr_u32 src_fmt;
    cmr_u32 slice_height;
    cmr_u32 slice_mod;
    cmr_u32 quality_level;
    cmr_uint stream_buf_phy;
    cmr_uint stream_buf_vir;
    cmr_u32 stream_buf_size;
    cmr_uint stream_buf_fd;
    cmr_u32 padding;
    cmr_handle jpeg_handle;
    struct img_size size;
    struct img_size out_size;
    struct img_addr src_addr_phy;
    struct img_addr src_addr_vir;
    cmr_u32 src_fd;
    struct img_data_end src_endian;
};

struct jpeg_enc_next_param {
    cmr_uint jpeg_handle;
    cmr_u32 ready_line_num;
    cmr_u32 slice_height;
    cmr_u32 padding;
    struct img_addr src_addr_phy;
    struct img_addr src_addr_vir;
};

struct jpeg_dec_in_param {
    cmr_uint stream_buf_phy;
    cmr_uint stream_buf_vir;
    cmr_u32 stream_buf_size;
    cmr_u32 stream_buf_fd;
    cmr_u32 slice_height;
    cmr_u32 slice_mod;
    cmr_u32 dst_fmt;
    cmr_uint temp_buf_phy;
    cmr_uint temp_buf_vir;
    cmr_u32 temp_buf_size;
    cmr_u32 temp_buf_mfd;
    cmr_u32 padding;
    cmr_handle jpeg_handle;
    struct img_size size;
    struct img_addr dst_addr_phy;
    struct img_addr dst_addr_vir;
    cmr_u32 dst_fd;
    struct img_data_end dst_endian;
};

struct jpeg_dec_next_param {
    cmr_handle jpeg_handle;
    cmr_u32 slice_height;
    cmr_u32 padding;
    struct img_addr dst_addr_phy;
    struct img_addr dst_addr_vir;
};

struct jpeg_enc_exif_param {
    cmr_handle jpeg_handle;
    cmr_uint src_jpeg_addr_virt;
    cmr_uint thumbnail_addr_virt;
    cmr_uint target_addr_virt;
    cmr_u32 src_jpeg_size;
    cmr_u32 thumbnail_size;
    cmr_u32 target_size;
    cmr_u32 padding;
    JINF_EXIF_INFO_T *exif_ptr;
    EXIF_ISP_INFO_T *exif_isp_info;
    EXIF_ISP_DEBUG_INFO_T exif_isp_debug_info;
};

cmr_int cmr_jpeg_init(cmr_handle oem_handle, cmr_handle *jpeg_handle,
                      jpg_evt_cb_ptr adp_event_cb);
cmr_int cmr_jpeg_deinit(cmr_handle jpeg_handle);
cmr_int cmr_jpeg_encode(cmr_handle jpeg_handle, struct img_frm *src,
                        struct img_frm *dst, struct jpg_op_mean *mean,
                        struct jpeg_enc_cb_param *enc_cb_param);
cmr_int cmr_jpeg_decode(cmr_handle jpeg_handle, struct img_frm *src,
                        struct img_frm *dst, struct jpg_op_mean *mean);
cmr_int cmr_jpeg_enc_add_eixf(cmr_handle jpeg_handle,
                              struct jpeg_enc_exif_param *param_ptr,
                              struct jpeg_wexif_cb_param *output_ptr);
cmr_int cmr_stop_codec(cmr_handle jpeg_handle);

#ifdef __cplusplus
}
#endif

#endif // for _JPEG_CODEC_H_
