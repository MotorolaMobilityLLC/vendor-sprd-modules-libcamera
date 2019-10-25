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
#ifndef _CMR_PREVIEW_H_
#define _CMR_PREVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cmr_common.h"
#include "sensor_drv_u.h"

// just for fix build warning, pdaf owner shoud be add it
struct pd_raw_info {
    int dummy;
};
// just for fix build warning, pdaf owner shoud be add it
struct pd_raw_open {
    int dummy;
};

#define CMR_3DNR_1_1_SMALL_WIDTH 960
#define CMR_3DNR_1_1_SMALL_HEIGHT 960
#define CMR_3DNR_4_3_SMALL_WIDTH 1280
#define CMR_3DNR_4_3_SMALL_HEIGHT 960
#define CMR_3DNR_16_9_SMALL_WIDTH 1280
#define CMR_3DNR_16_9_SMALL_HEIGHT 720
#define CMR_3DNR_18_9_SMALL_WIDTH 1280
#define CMR_3DNR_18_9_SMALL_HEIGHT 640
#define CMR_3DNR_19_9_SMALL_WIDTH 1280
#define CMR_3DNR_19_9_SMALL_HEIGHT 608

enum preview_func_type {
    PREVIEW_FUNC_START_PREVIEW = 0,
    PREVIEW_FUNC_STOP_PREVIEW,
    PREVIEW_FUNC_START_CAPTURE,
    PREVIEW_FUNC_MAX
};

enum preview_cb_type {
    PREVIEW_RSP_CB_SUCCESS = 0,
    PREVIEW_EVT_CB_FRAME,
    PREVIEW_EXIT_CB_FAILED,
    PREVIEW_EVT_CB_FLUSH,
    PREVIEW_EVT_CB_FD,
    PREVIEW_EVT_CB_RESUME,
    PREVIEW_EXIT_CB_PREPARE,
    PREVIEW_EVT_CB_RAW_FRAME,
    PREVIEW_EVT_CB_AT,
    PREVIEW_EVT_MAX
};

enum preview_op_evt {
    PREVIEW_CHN_PAUSE = CMR_GRAB_MAX + 1,
    PREVIEW_CHN_RESUME,
    PREVIEW_OP_MAX
};

enum preview_frame_type {
    PREVIEW_FRAME = 0,
    PREVIEW_VIDEO_FRAME,
    PREVIEW_ZSL_FRAME,
    PREVIEW_CANCELED_FRAME,
    PREVIEW_VIDEO_CANCELED_FRAME,
    PREVIEW_ZSL_CANCELED_FRAME,
    CHANNEL0_FRAME,
    CHANNEL1_FRAME,
    CHANNEL2_FRAME,
    CHANNEL3_FRAME,
    CHANNEL4_FRAME,
    PREVIEW_FRAME_TYPE_MAX
};

enum preview_4in1_mode {
    PREVIEW_4IN1_OFF,
    PREVIEW_4IN1_BINNING,
    PREVIEW_4IN1_FULL,
    PREVIEW_4IN1_MAX
};

typedef cmr_int (*preview_cb_func)(cmr_handle oem_handle,
                                   enum preview_cb_type cb_type,
                                   enum preview_func_type func_type,
                                   void *parm);

struct preview_md_ops {
    cmr_int (*channel_cfg)(cmr_handle oem_handle, cmr_handle caller_handle,
                           cmr_u32 camera_id,
                           struct channel_start_param *param_ptr,
                           cmr_u32 *channel_id, struct img_data_end *endian);
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
#ifdef CONFIG_CAMERA_OFFLINE
    cmr_int (*channel_dcam_size)(cmr_handle oem_handle,
                                 struct sprd_dcam_path_size *dcam_cfg);
#endif
    cmr_int (*channel_scale_capability)(cmr_handle oem_handle, cmr_u32 *width,
                                        cmr_u32 *sc_factor,
                                        cmr_u32 *sc_threshold);
    cmr_int (*channel_path_capability)(cmr_handle oem_handle,
                                       struct cmr_path_capability *capability);
    cmr_int (*channel_get_cap_time)(cmr_handle oem_handle, cmr_u32 *sec,
                                    cmr_u32 *usec);
    cmr_int (*isp_start_video)(cmr_handle oem_handle,
                               struct video_start_param *param_ptr);
    cmr_int (*isp_stop_video)(cmr_handle oem_handle);
    cmr_int (*sensor_open)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*sensor_close)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*start_rot)(cmr_handle oem_handle, cmr_handle caller_handle,
                         struct img_frm *src, struct img_frm *dst,
                         struct cmr_op_mean *mean);
    cmr_int (*preview_pre_proc)(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 preview_sn_mode);
    cmr_int (*preview_post_proc)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*capture_pre_proc)(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 preview_sn_mode,
                                cmr_u32 capture_sn_mode, cmr_u32 is_restart,
                                cmr_u32 is_sn_reopen);
    cmr_int (*capture_post_proc)(cmr_handle oem_handle, cmr_u32 camera_id);
    cmr_int (*get_sensor_info)(cmr_handle oem_handle, cmr_uint sensor_id,
                               struct sensor_exp_info *sensor_info);
    cmr_int (*get_sensor_autotest_mode)(cmr_handle oem_handle,
                                        cmr_uint sensor_id,
                                        cmr_uint *is_autotest);
    cmr_int (*get_isp_yimg)(cmr_handle oem_handle, cmr_u32 sensor_id,
                            struct isp_yimg_info *yimg);
    cmr_int (*set_preview_yimg)(cmr_handle oem_handle, cmr_u32 sensor_id,
                                struct yimg_info *yimg);
    cmr_int (*set_preview_yuv)(cmr_handle oem_handle, cmr_u32 sensor_id,
                               struct yuv_info_t *yuv);
    cmr_int (*set_preview_pd_raw)(cmr_handle oem_handle,
                                  struct pd_raw_info *pd_raw);
    cmr_int (*set_preview_pd_open)(cmr_handle oem_handle,
                                   struct pd_raw_open *pd_open);
    cmr_int (*get_sensor_fps_info)(cmr_handle oem_handle, cmr_uint sensor_id,
                                   cmr_u32 sn_mode,
                                   struct sensor_mode_fps_tag *fps_info);
    cmr_int (*get_sensor_otp)(cmr_handle oem_handle, cmr_u8 dual_flag,
                              struct sensor_otp_cust_info *dual_otp_data);
    cmr_int (*get_buff_handle)(cmr_handle oem_handle, int frame_type,
                               cam_graphic_buffer_info_t *buf_info);
    cmr_int (*release_buff_handle)(cmr_handle oem_handle, int frame_type,
                                   cam_graphic_buffer_info_t *buf_info);
    cmr_int (*isp_buff_cfg)(cmr_handle oem_handle, struct buffer_cfg *buf_cfg);
    cmr_int (*hdr_set_ev)(cmr_handle oem_handle);
    cmr_int (*set_3dnr_ev)(cmr_handle oem_handle, cmr_u32 enable);
    cmr_int (*sw_3dnr_info_cfg)(cmr_handle oem_handle,
                                struct sprd_img_3dnr_param *threednr_info);
    cmr_int (*isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                         struct common_isp_cmd_param *parm);
};

struct preview_init_param {
    cmr_handle oem_handle;
    cmr_handle ipm_handle;
    cmr_uint sensor_bits;
    preview_cb_func oem_cb;
    struct preview_md_ops ops;
    void *private_data;
};

struct preview_param {
    cmr_uint is_fd_on;
    cmr_uint is_support_fd;
    cmr_u32 preview_eb;
    cmr_uint preview_fmt;
    cmr_uint prev_rot;
    struct img_size preview_size;

    cmr_u32 video_eb;
    struct img_size video_size;
    cmr_u32 video_fmt;

    cmr_u32 channel0_eb;
    cmr_u32 channel0_fmt;
    struct img_size channel0_size;

    cmr_u32 channel1_eb;
    cmr_u32 channel1_fmt;
    cmr_u32 channel1_rot_angle;
    struct img_size channel1_size;
    cmr_u32 channel1_flip_on;

    cmr_u32 channel2_eb;
    cmr_u32 channel2_fmt;
    cmr_u32 channel2_rot_angle;
    struct img_size channel2_size;
    cmr_u32 channel2_flip_on;

    cmr_u32 channel3_eb;
    cmr_u32 channel3_fmt;
    cmr_u32 channel3_rot_angle;
    struct img_size channel3_size;
    cmr_u32 channel3_flip_on;

    cmr_u32 channel4_eb;
    cmr_u32 channel4_fmt;
    cmr_u32 channel4_rot_angle;
    struct img_size channel4_size;
    cmr_u32 channel4_flip_on;

    cmr_u32 snapshot_eb;
    struct img_size picture_size;
    struct img_size raw_capture_size;
    struct img_size thumb_size;
    cmr_uint cap_fmt;
    cmr_u32 cap_rot;
    cmr_u32 is_cfg_rot_cap;
    cmr_u32 encode_angle;

    cmr_u32 is_dv;
    cmr_u32 is_hdr;
    cmr_u32 sprd_3dnr_type;
    cmr_u32 isp_width_limit;
    cmr_u32 frame_ctrl;  // 0:stop,1:continue
    cmr_u32 frame_count; // 0xffffffff for zsl
    cmr_u32 flip_on;
    cmr_u32 tool_eb;
    void *private_data;
    cmr_u32 is_lls_enable;
    cmr_u32 is_ultra_wide;
    cmr_u32 sprd_zsl_enabled;
    cmr_u32 sprd_afbc_enabled;
    cmr_u32 video_slowmotion_eb;
    cmr_u32 sprd_pipviv_enabled;
    cmr_u32 sprd_eis_enabled;
    cmr_u32 isp_to_dram;
    cmr_u32 video_snapshot_type;
    cmr_u32 sprd_3dcalibration_enabled;
    cmr_u32 mode_4in1;
    cmr_u32 limited_4in1_width;
    cmr_u32 limited_4in1_height;
    struct cmr_zoom_param zoom_setting;
    struct memory_param memory_setting;
};

struct preview_out_param {
    cmr_u32 preview_chn_id;
    cmr_u32 preview_sn_mode;
    struct img_data_end preview_data_endian;
    cmr_u32 snapshot_chn_id;
    cmr_u32 snapshot_sn_mode;
    struct img_data_end snapshot_data_endian;
    cmr_u32 zsl_frame;
    struct img_size actual_snapshot_size;
    cmr_u32 video_chn_id;
    cmr_u32 video_sn_mode;
    struct img_data_end video_data_endian;
    struct img_size actual_video_size;

    cmr_u32 channel0_chn_id;
    cmr_u32 channel0_sn_mode;
    struct img_data_end channel0_endian;
    struct img_size channel0_size;

    cmr_u32 channel1_chn_id;
    cmr_u32 channel1_sn_mode;
    struct img_data_end channel1_endian;
    struct img_size channel1_size;

    cmr_u32 channel2_chn_id;
    cmr_u32 channel2_sn_mode;
    struct img_data_end channel2_endian;
    struct img_size channel2_size;

    cmr_u32 channel3_chn_id;
    cmr_u32 channel3_sn_mode;
    struct img_data_end channel3_endian;
    struct img_size channel3_size;

    cmr_u32 channel4_chn_id;
    cmr_u32 channel4_sn_mode;
    struct img_data_end channel4_endian;
    struct img_size channel4_size;
    struct snp_proc_param post_proc_setting;
};

struct preview_zsl_info {
    cmr_u32 is_support;
    cmr_u32 max_width;
    cmr_u32 max_height;
};

cmr_int cmr_preview_init(struct preview_init_param *init_param_ptr,
                         cmr_handle *preview_handle_ptr);

cmr_int cmr_preview_deinit(cmr_handle preview_handle);

cmr_int cmr_preview_set_param(cmr_handle preview_handle, cmr_u32 camera_id,
                              struct preview_param *param_ptr,
                              struct preview_out_param *out_param_ptr);

cmr_int cmr_preview_start(cmr_handle preview_handle, cmr_u32 camera_id);

cmr_int cmr_preview_stop(cmr_handle preview_handle, cmr_u32 camera_id);

cmr_int cmr_preview_cancel_snapshot(cmr_handle preview_handle,
                                    cmr_u32 camera_id);

cmr_int cmr_preview_get_status(cmr_handle preview_handle, cmr_u32 camera_id);

cmr_int cmr_preview_get_prev_rect(cmr_handle preview_handle, cmr_u32 camera_id,
                                  struct img_rect *rect);

cmr_int cmr_preview_receive_data(cmr_handle preview_handle, cmr_u32 camera_id,
                                 cmr_uint evt, void *data);

cmr_int cmr_preview_update_zoom(cmr_handle preview_handle, cmr_u32 camera_id,
                                struct cmr_zoom_param *param);

cmr_int cmr_preview_release_frame(cmr_handle preview_handle, cmr_u32 camera_id,
                                  cmr_uint index);

cmr_int cmr_preview_ctrl_facedetect(cmr_handle preview_handle,
                                    cmr_u32 camera_id, cmr_uint on_off);

cmr_int cmr_preview_facedetect_set_ae_stab(cmr_handle preview_handle,
                                           cmr_u32 camera_id, cmr_u32 *ae_stab);

cmr_int cmr_preview_facedetect_set_hist(cmr_handle preview_handle,
                                        cmr_u32 camera_id, const cmr_u32 *ae_stab);

cmr_int cmr_preview_is_support_zsl(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint *is_support);

cmr_int cmr_preview_get_max_cap_size(cmr_handle preview_handle,
                                     cmr_u32 camera_id, cmr_uint *max_w,
                                     cmr_uint *max_h);
cmr_int cmr_preview_set_cap_size(
    cmr_handle preview_handle, cmr_u32 is_reprocessing, cmr_u32 camera_id,
    cmr_u32 width,
    cmr_u32 height); /**add for 3d capture to reset reprocessing capture size*/

cmr_int cmr_preview_get_post_proc_param(cmr_handle preview_handle,
                                        cmr_u32 camera_id, cmr_u32 encode_angle,
                                        struct snp_proc_param *out_param_ptr);

cmr_int cmr_preview_before_set_param(cmr_handle preview_handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode);

cmr_int cmr_preview_after_set_param(cmr_handle preview_handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number);

cmr_int cmr_preview_set_preview_buffer(cmr_handle preview_handle,
                                       cmr_u32 camera_id,
                                       cam_buffer_info_t buffer);

cmr_int cmr_preview_set_video_buffer(cmr_handle preview_handle,
                                     cmr_u32 camera_id,
                                     cam_buffer_info_t buffer);

cmr_int cmr_preview_set_zsl_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                   cmr_s32 fd);

int cmr_channel0_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel1_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel2_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel3_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

int cmr_channel4_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer);

cmr_int prev_set_ae_time(cmr_handle preview_handle, cmr_u32 camera_id,
                         void *data);

cmr_int cmr_preview_get_zoom_factor(cmr_handle preview_handle,
                                    cmr_u32 camera_id, struct cmr_zoom *zoom_factor);
cmr_int cmr_camera_isp_stop_video(cmr_handle preview_handle, cmr_u32 camera_id);
cmr_int cmr_preview_get_hdr_buf(cmr_handle handle, cmr_u32 camera_id,
                                struct frm_info *in, cmr_uint *vir_addr_y);
cmr_int
cmr_preview_set_autotracking_param(cmr_handle preview_handle, cmr_u32 camera_id,
                                   struct auto_tracking_info *input_param);

cmr_int cmr_preview_set_fd_touch_param(cmr_handle preview_handle,
                                       cmr_u32 camera_id,
                                       struct fd_touch_info *input_param);

#ifdef __cplusplus
}
#endif

#endif
