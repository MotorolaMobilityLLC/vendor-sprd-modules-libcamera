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
#ifndef ANDROID_HARDWARE_SPRD_OEM_CAMERA_H
#define ANDROID_HARDWARE_SPRD_OEM_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "cmr_type.h"

#define FACE_DETECT_NUM 10

cmr_int camera_init(cmr_u32 camera_id, camera_cb_of_type callback,
                    void *client_data, cmr_uint is_autotest,
                    cmr_handle *camera_handle, void *cb_of_malloc,
                    void *cb_of_free);

cmr_int camera_deinit(cmr_handle camera_handle);

cmr_int camera_release_frame(cmr_handle camera_handle, enum camera_data data,
                             cmr_uint index);

cmr_int camera_set_param(cmr_handle camera_handle, enum camera_param_type id,
                         uint64_t param);

cmr_int camera_start_preview(cmr_handle camera_handle,
                             enum takepicture_mode mode);

cmr_int camera_stop_preview(cmr_handle camera_handle);

cmr_int camera_start_autofocus(cmr_handle camera_handle);

cmr_int camera_cancel_autofocus(cmr_handle camera_handle);

cmr_int camera_cancel_takepicture(cmr_handle camera_handle);

uint32_t camera_safe_scale_th(void);

cmr_int camera_take_picture(cmr_handle camera_handle,
                            enum takepicture_mode cap_mode);

cmr_int camera_get_sn_trim(cmr_handle camera_handle, cmr_u32 mode,
                           cmr_uint *trim_x, cmr_uint *trim_y, cmr_uint *trim_w,
                           cmr_uint *trim_h, cmr_uint *width, cmr_uint *height);

cmr_int camera_set_mem_func(cmr_handle camera_handle, void *cb_of_malloc,
                            void *cb_of_free, void *private_data);

cmr_int camera_get_redisplay_data(
    cmr_handle camera_handle, cmr_s32 output_fd, cmr_uint output_addr,
    cmr_uint output_vir_addr, cmr_uint output_width, cmr_uint output_height,
    cmr_s32 input_fd, cmr_uint input_addr_y, cmr_uint input_addr_uv,
    cmr_uint input_vir_addr, cmr_uint input_width, cmr_uint input_height);

cmr_int camera_is_change_size(cmr_handle camera_handle, cmr_u32 cap_width,
                              cmr_u32 cap_height, cmr_u32 preview_width,
                              cmr_u32 preview_height, cmr_u32 video_width,
                              cmr_u32 video_height, cmr_uint *is_change);

cmr_int camera_get_preview_rect(cmr_handle camera_handle, cmr_uint *rect_x,
                                cmr_uint *rect_y, cmr_uint *rect_width,
                                cmr_uint *rect_height);

cmr_int camera_get_zsl_capability(cmr_handle camera_handle,
                                  cmr_uint *is_support, cmr_uint *max_widht,
                                  cmr_uint *max_height);

cmr_int camera_get_sensor_info_for_raw(cmr_handle camera_handle,
                                       struct sensor_mode_info *mode_info);

cmr_int camera_get_sensor_trim(cmr_handle camera_handle,
                               struct img_rect *sn_trim);

cmr_int camera_get_sensor_trim2(cmr_handle camera_handle,
                               struct img_rect *sn_trim);

cmr_uint camera_get_preview_rot_angle(cmr_handle camera_handle);

void camera_fd_enable(cmr_handle camera_handle, cmr_u32 is_enable);
void camera_flip_enable(cmr_handle camera_handle, cmr_u32 param);

void camera_fd_start(cmr_handle camera_handle, cmr_u32 param);

cmr_int camera_is_need_stop_preview(cmr_handle camera_handle);

cmr_int camera_takepicture_process(cmr_handle camera_handle,
                                   cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                   cmr_u32 width, cmr_u32 height);

uint32_t camera_get_size_align_page(uint32_t size);

cmr_int camera_fast_ctrl(cmr_handle camera_handle, enum fast_ctrl_mode mode,
                         cmr_u32 param);

cmr_int camera_start_preflash(cmr_handle camera_handle);

cmr_int camera_get_viewangle(cmr_handle camera_handle,
                             struct sensor_view_angle *view_angle);

cmr_uint camera_get_sensor_exif_info(cmr_handle camera_handle,
                                     struct exif_info *exif_info);
cmr_uint camera_get_sensor_result_exif_info(
    cmr_handle camera_handle,
    struct exif_spec_pic_taking_cond_tag *exif_pic_info);
cmr_s32 camera_get_iommu_status(cmr_handle camera_handle);
cmr_int camera_set_preview_buffer(cmr_handle camera_handle,
                                  cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                  cmr_s32 fd);
cmr_int camera_set_video_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr,
                                cmr_uint src_vir_addr, cmr_s32 fd);
cmr_int camera_set_zsl_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr,
                              cmr_uint src_vir_addr, cmr_s32 fd);

cmr_s32 queue_buffer(cmr_handle camera_handle, cam_buffer_info_t buffer,
                     int steam_type);

cmr_int camera_set_video_snapshot_buffer(cmr_handle camera_handle,
                                         cmr_uint src_phy_addr,
                                         cmr_uint src_vir_addr, cmr_s32 fd);
cmr_int camera_set_zsl_snapshot_buffer(cmr_handle camera_handle,
                                       cmr_uint src_phy_addr,
                                       cmr_uint src_vir_addr, cmr_s32 fd);
cmr_int camera_zsl_snapshot_need_pause(cmr_handle camera_handle, cmr_int *flag);
cmr_int camera_get_isp_handle(cmr_handle camera_handle, cmr_handle *isp_handle);
void camera_lls_enable(cmr_handle camera_handle, cmr_u32 is_enable);
cmr_int camera_is_lls_enabled(cmr_handle camera_handle);
void camera_vendor_hdr_enable(cmr_handle camera_handle, cmr_u32 is_enable);
cmr_int camera_is_vendor_hdr(cmr_handle camera_handle);

void camera_set_lls_shot_mode(cmr_handle camera_handle, cmr_u32 is_enable);
cmr_int camera_get_lls_shot_mode(cmr_handle camera_handle);

cmr_int camera_get_isp_info(cmr_handle camera_handle, void **addr, int *size, cmr_s32 frame_id);

cmr_int camera_get_last_preflash_time(cmr_handle camera_handle, cmr_s64 *time);

void camera_start_burst_notice(cmr_handle camera_handle);
void camera_end_burst_notice(cmr_handle camera_handle);

cmr_int dump_jpeg_file(void *virt_addr, unsigned int size, int width,
                       int height);

cmr_int camera_get_gain_thrs(cmr_handle camera_handle, cmr_u32 *is_over_thrs);

cmr_int
camera_set_sensor_info_to_af(cmr_handle camera_handle,
                             struct cmr_af_aux_sensor_info *sensor_info);
cmr_int camera_get_sensor_max_fps(cmr_handle camera_handle, cmr_u32 camera_id,
                                  cmr_u32 *max_fps);

cmr_int camera_snapshot_is_need_flash(cmr_handle oem_handle, cmr_u32 camera_id,
                                      cmr_u32 *is_need_flash);
cmr_uint camera_get_sensor_otp_info(cmr_handle camera_handle, cmr_u8 dual_flag,
                                    struct sensor_otp_cust_info *otp_info);
cmr_uint camera_get_sensor_vcm_step(cmr_handle camera_handle, cmr_u32 camera_id,
                                    cmr_u32 *vcm_step);

cmr_int camera_set_reprocess_picture_size(
    cmr_handle camera_handle, cmr_uint is_reprocessing, cmr_u32 camera_id,
    cmr_u32 width,
    cmr_u32 height); /**add for 3d capture to reset reprocessing capture size*/

cmr_int camera_set_largest_picture_size(cmr_u32 camera_id, cmr_u16 width,
                                        cmr_u16 height);

cmr_int camera_start_capture(cmr_handle camera_handle);
cmr_int camera_stop_capture(cmr_handle camera_handle);

cmr_int camera_ioctrl(cmr_handle handle, int cmd, void *param);

cmr_int camera_reprocess_yuv_for_jpeg(cmr_handle camera_handle,
                                      enum takepicture_mode cap_mode,
                                      cmr_uint yaddr, cmr_uint yaddr_vir,
                                      cmr_uint fd);

cmr_int camera_get_focus_point(cmr_handle camera_handle, cmr_s32 *point_x,
                               cmr_s32 *point_y);

cmr_s32 camera_isp_sw_check_buf(cmr_handle camera_handle, cmr_uint *param_ptr);

cmr_int camera_raw_post_proc(cmr_handle camera_handle, struct img_frm *raw_buff,
                             struct img_frm *yuv_buff,
                             struct img_sbs_info *sbs_info);

cmr_int camera_get_tuning_param(cmr_handle camera_handle,
                                struct tuning_param_info *tuning_info);
cmr_int image_sw_algorithm_processing(
    cmr_handle camera_handle,
    struct image_sw_algorithm_buf *src_sw_algorithm_buf,
    struct image_sw_algorithm_buf *dst_sw_algorithm_buf,
    sprd_cam_image_sw_algorithm_type_t sw_algorithm_type,
    cam_img_format_t format);
int dump_image_with_isp_info(cmr_handle camera_handle, uint32_t img_fmt,
                             uint32_t width, uint32_t height,
                             uint32_t dump_size, struct img_addr *addr);
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
cmr_int camera_set_mm_dvfs_policy(cmr_handle camera_handle,
                                  enum DVFS_MM_MODULE module,
                                  enum CamProcessingState camera_state);
#endif
void camera_set_original_picture_size(cmr_handle camera_handle,
                                   int32_t width,int32_t height);

void camera_set_exif_iso_value(cmr_handle handle,cmr_u32 iso_value);
void camera_set_exif_exp_time(cmr_handle handle,cmr_s64 exp_time);

#ifdef __cplusplus
}
#endif

#endif // ANDROID_HARDWARE_SPRD_OEM_CAMERA_H
