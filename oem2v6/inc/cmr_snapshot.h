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
#ifndef _CMR_SNAPSHOT_H_
#define _CMR_SNAPSHOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cmr_common.h"
#include "SprdOEMCamera.h"
#include "cmr_jpeg.h"

// other module need snapshot do something
enum snapshot_receive_evt_type {
    SNAPSHOT_EVT_CHANNEL_DONE = CMR_EVT_SNAPSHOT_BASE,
    SNAPSHOT_EVT_RAW_PROC,
    SNAPSHOT_EVT_SCALE_DONE,
    SNAPSHOT_EVT_POSTPROC_START,
    SNAPSHOT_EVT_3DNR_DONE,
    SNAPSHOT_EVT_CVT_RAW_DATA,
    SNAPSHOT_EVT_JPEG_ENC_DONE,
    SNAPSHOT_EVT_JPEG_DEC_DONE,
    SNAPSHOT_EVT_JPEG_ENC_ERR,
    SNAPSHOT_EVT_JPEG_DEC_ERR,
    SNAPSHOT_EVT_FREE_FRM,
    SNAPSHOT_EVT_FDR_PROC,
    SNPASHOT_EVT_MAX
};

// snapshot send msg to other module
enum snapshot_cb_type {
    SNAPSHOT_CB_EVT_RSP_SUCCESS,
    SNAPSHOT_CB_EVT_DONE,
    SNAPSHOT_CB_EVT_FAILED,
    SNAPSHOT_CB_EVT_CAPTURE_FRAME_DONE,
    SNAPSHOT_CB_EVT_SNAPSHOT_DONE,
    SNAPSHOT_CB_EVT_SNAPSHOT_JPEG_DONE,
    SNAPSHOT_CB_EVT_ENC_DONE,
    SNAPSHOT_CB_EVT_STATE,
    SNAPSHOT_CB_EVT_RETURN_ZSL_BUF,
    SNAPSHOT_CB_EVT_PREPARE,
    SNAPSHOT_CB_EVT_FLUSH_CACHE,
    SNAPSHOT_CB_EVT_INVALIDATE_CACHE,
    SNAPSHOT_CB_EVT_RETURN_SW_ALGORITHM_ZSL_BUF,
    SNAPSHOT_CB_EVT_MAX
};

enum snapshot_func_type {
    SNAPSHOT_FUNC_RELEASE_PICTURE,
    SNAPSHOT_FUNC_TAKE_PICTURE,
    SNAPSHOT_FUNC_ENCODE_PICTURE,
    SNAPSHOT_FUNC_STATE,
    SNAPSHOT_FUNC_RECOVERY,
    SNAPSHOT_FUNC_MAX
};


enum snap_frame_type {
	FRAME_COMMON = 0,
	FRAME_4IN1,
	FDR_FRAME_RAW,
	FDR_FRAME_YUVL,
	FDR_FRAME_YUVH,
	FDR_FRAME_MAX,
};

#define IS_FDR_FRAME(frame_type) \
	(((frame_type >= FDR_FRAME_RAW) && (frame_type <= FDR_FRAME_YUVH)) ? 1 : 0)

typedef void (*snapshot_cb_of_state)(cmr_handle oem_handle,
                                     enum snapshot_cb_type cb,
                                     enum snapshot_func_type func, void *parm);

struct raw_proc_param {
    struct img_frm src_frame;
    struct img_frm dst_frame;
    struct img_frm dst2_frame;
    cmr_u32 src_avail_height;
    cmr_u32 src_slice_height;
    cmr_u32 dst_slice_height;
    cmr_u32 dst2_slice_height;
    cmr_u32 slice_num;
    struct img_sbs_info sbs_info;
};

struct raw_proc_out_param {
    cmr_uint output_height;
};

struct jpeg_param {
    cmr_u32 quality;
    cmr_u32 thumb_quality;
    cmr_u32 set_encode_rotation;
    cmr_u32 padding;
    struct img_size thum_size;
};

struct snapshot_md_ops {
    cmr_int (*start_encode)(cmr_handle oem_handle, cmr_handle caller_handle,
                            struct img_frm *src, struct img_frm *dst,
                            struct cmr_op_mean *mean,
                            struct jpeg_enc_cb_param *enc_cb_param);
    cmr_int (*start_decode)(cmr_handle oem_handle, cmr_handle caller_handle,
                            struct img_frm *src, struct img_frm *dst,
                            struct cmr_op_mean *mean);
    cmr_int (*start_exif_encode)(cmr_handle oem_handle,
                                 cmr_handle caller_handle,
                                 struct img_frm *big_pic_src,
                                 struct img_frm *thumb_src, void *exif,
                                 struct img_frm *dst,
                                 struct jpeg_wexif_cb_param *out_ptr);
    cmr_int (*stop_codec)(cmr_handle oem_handle);
    cmr_int (*start_scale)(cmr_handle oem_handle, cmr_handle caller_handle,
                           struct img_frm *src, struct img_frm *dst,
                           struct cmr_op_mean *mean);
    cmr_int (*start_rot)(cmr_handle oem_handle, cmr_handle caller_handle,
                         struct img_frm *src, struct img_frm *dst,
                         struct cmr_op_mean *mean);
    cmr_int (*capture_pre_proc)(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 preview_sn_mode,
                                cmr_u32 capture_sn_mode, cmr_u32 is_restart,
                                cmr_u32 is_sn_reopen);
    cmr_int (*capture_post_proc)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*raw_proc)(cmr_handle oem_handle, cmr_handle caller_handle,
                        struct raw_proc_param *param_ptr);
    cmr_int (*isp_start_video)(cmr_handle oem_handle,
                               struct video_start_param *param_ptr);
    cmr_int (*isp_stop_video)(cmr_handle oem_handle);
    cmr_int (*channel_cfg)(cmr_handle oem_handle, cmr_handle caller_handle,
                           cmr_u32 camera_id,
                           struct channel_start_param *param_ptr,
                           cmr_u32 *channel_id);
    cmr_int (*channel_start)(cmr_handle oem_handle, cmr_u32 channel_bits,
                             cmr_uint skip_bumber);
    cmr_int (*channel_pause)(cmr_handle oem_handle, cmr_uint channel_id,
                             cmr_u32 reconfig_flag);
    cmr_int (*channel_resume)(cmr_handle oem_handle, cmr_uint channel_id,
                              cmr_u32 skip_number, cmr_u32 deci_factor,
                              cmr_u32 frm_num);
    cmr_int (*channel_free_frame)(cmr_handle oem_handle, cmr_u32 channel_id,
                                  cmr_u32 index);
    cmr_int (*channel_stop)(cmr_handle oem_handle, cmr_u32 channel_bits);
    cmr_int (*channel_buff_cfg)(cmr_handle oem_handle,
                                struct buffer_cfg *buf_cfg);
    cmr_int (*channel_cap_cfg)(cmr_handle oem_handle, cmr_handle caller_handle,
                               cmr_u32 camera_id, struct cap_cfg *cap_cfg,
                               cmr_u32 *channel_id,
                               struct img_data_end *endian);
    cmr_int (*get_sensor_info)(cmr_handle oem_handle, cmr_uint sensor_id,
                               struct sensor_exp_info *exp_info_ptr);
    cmr_int (*sensor_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                            struct common_sn_cmd_param *parm);
    cmr_int (*get_tuning_info)(cmr_handle oem_handle,
                               struct isp_adgain_exp_info *adgain_exp_info_ptr);
    void (*face_makeup)(cmr_handle oem_handle, struct img_frm *src);
    cmr_int (*get_jpeg_param_info)(cmr_handle oem_handle,
                                   struct jpeg_param *param);
    cmr_int (*ipm_process)(cmr_handle oem_handle, void *data);
    int (*dump_image_with_3a_info)(cmr_handle oem_handle, uint32_t img_fmt,
                                   uint32_t width, uint32_t height,
                                   uint32_t dump_size, struct img_addr *addr);
    cmr_u32 (*get_cnr_realtime_flag)(cmr_handle oem_handle);
};

struct snapshot_init_param {
    cmr_uint id;
    cmr_handle oem_handle;
    cmr_handle ipm_handle;
    snapshot_cb_of_state oem_cb;
    struct snapshot_md_ops ops;
    struct memory_param memory_setting;
    void *private_data;
};

struct snapshot_param {
    cmr_u32 camera_id;
    cmr_u32 mode;
    cmr_u32 is_ultrawide;
    cmr_u32 filter_type;
    cmr_u32 fb_on;
    cmr_u32 watermark;
    cmr_u32 is_hdr;
    cmr_u32 is_fdr;
    cmr_u32 is_3dnr;
    cmr_u32 is_mfsr;
    cmr_u32 is_video_snapshot;
    cmr_u32 is_zsl_snapshot;
    cmr_u32 snap_cnt;
    cmr_u32 total_num;
    cmr_u32 rot_angle;
    cmr_u32 is_android_zsl;
    cmr_u32 is_cfg_rot_cap;
    cmr_u32 channel_id;
    cmr_u32 sn_mode;
    cmr_u32 hdr_need_frm_num;
    cmr_u32 threednr_need_frm_num;
    cmr_u32 lls_shot_mode;
    cmr_u32 is_vendor_hdr;
    cmr_u32 is_pipviv_mode;
    cmr_u32 is_3dcalibration_mode;
    cmr_u32 is_yuv_callback_mode;
    cmr_u32 is_super;
    cmr_u32 nr_flag;
    cmr_u32 ee_flag;
    cmr_u32 dre_flag;
    cmr_handle threednr_handle;
    cmr_handle hdr_handle;
    struct img_size req_size;
    struct cmr_zoom_param zoom_param;
    struct jpeg_param jpeg_setting;
    struct snp_proc_param post_proc_setting;
};

struct encode_cb_param {
    cmr_uint stream_buf_phy;
    cmr_uint stream_buf_vir;
    cmr_u32 stream_size;
    cmr_u32 total_height;
    cmr_u32 is_thumbnail;
    cmr_u32 padding;
};

struct decode_cb_param {
    struct img_frm dst_img;
    cmr_u32 total_height;
    cmr_u32 padding;
};

struct snp_zsl_data {
    cmr_uint src_phy_addr;
    cmr_uint src_vir_addr;
    cmr_u32 width;
    cmr_u32 height;
};

cmr_int cmr_snapshot_init(struct snapshot_init_param *param_ptr,
                          cmr_handle *snapshot_handle);

cmr_int cmr_snapshot_deinit(cmr_handle snapshot_handle);

cmr_int cmr_snapshot_prepare(cmr_handle snapshot_handle,
                               struct snapshot_param *param_ptr);
cmr_int cmr_snapshot_post_proc(cmr_handle snapshot_handle,
                               struct snapshot_param *param_ptr);

cmr_int cmr_snapshot_receive_data(cmr_handle snapshot_handle, cmr_int evt,
                                  void *data);

cmr_int cmr_snapshot_stop(cmr_handle snapshot_handle);

cmr_int cmr_snapshot_release_frame(cmr_handle snapshot_handle, cmr_uint index);

cmr_int cmr_snapshot_format_convert(cmr_handle snapshot_handle, void *data,
                                    struct img_frm *out_ptr);

cmr_int cmr_snapshot_memory_flush(cmr_handle snapshot_handle,
                                  struct img_frm *img);

cmr_int cmr_snapshot_invalidate_cache(cmr_handle snapshot_handle,
                                      struct img_frm *img);

/* performance optimization, use zsl buf to postprocess, not copy */
cmr_int zsl_snp_update_post_proc_param(cmr_handle snp_handle,
                                       struct img_frm *img_frame);
cmr_int cmr_snapshot_thumb_yuv_proc(cmr_handle snp_handle,
                                    struct snp_thumb_yuv_param *thumb_parm);
cmr_int cmr_snpshot_encode_semaphore(cmr_handle snp_handle, int wait); //wait:0~post,1~wait
cmr_int cmr_snp_update_reprocess_param(cmr_handle snp_handle,
                                       struct snapshot_param *param_ptr, struct img_frm *img_frame);
#ifdef __cplusplus
}
#endif

#endif
