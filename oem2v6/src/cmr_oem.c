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
#define LOG_TAG "cmr_oem"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include "cmr_oem.h"
#include "cmr_common.h"
#include "isp_otp_calibration.h"
#include "isp_simulation.h"
#include "isp_video.h"
#include "cmr_watermark.h"
#include "pthread.h"
#include <cutils/properties.h>
#include <cutils/trace.h>
#include <dlfcn.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <dlfcn.h>
#include <libloader.h>

#ifdef CONFIG_CAMERA_FDR
#include "fdr_interface.h"
#endif

#include "swa_fdr_api.h"

#ifdef CONFIG_FACE_BEAUTY
#include "sprd_facebeauty_adapter.h"
#endif
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
#include "cmr_mm_dvfs.h"
#endif

#define FILE_NAME_LEN 200
#define PREVIEW_MSG_QUEUE_SIZE 100
#define SNAPSHOT_MSG_QUEUE_SIZE 50
#define CMR_EVT_INIT (CMR_EVT_OEM_BASE)
#define CMR_EVT_WAIT (CMR_EVT_OEM_BASE + 1)
#define CMR_EVT_EXIT (CMR_EVT_OEM_BASE + 2)

#define CAMERA_OEM_MSG_QUEUE_SIZE 10
#define CAMERA_RECOVER_CNT 3

#define OEM_HANDLE_HDR 1
#define OEM_HANDLE_3DNR 1
#define OEM_HANDLE_FILTER 1

#define CAMERA_PATH_SHARE 1
#define OEM_RESTART_SUM 2
#define POWER2(x) (1 << (x))
#define ONE_HUNDRED 100
#define MS_TO_NANOSEC 1000LL
#define SEC_TO_NANOSEC 1000000000LL
#define BUF_BLOCK_SIZE (1024 * 1024)
#define PICTURE_W 5664

#define OFFLINE_CHANNEL_BIT 0x8
#define OFFLINE_CHANNEL 3

#define HDR_SKIP_FRAME_NUM 2
#define FDR_SKIP_FRAME_NUM 7

enum oem_ev_level { OEM_EV_LEVEL_1, OEM_EV_LEVEL_2, OEM_EV_LEVEL_3 };


#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            CMR_LOGE("err handle");                                            \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

#define CONVERED_CAMERA_INIT                                                   \
    ((cxt->camera_id >= 2 && (is_multi_camera_mode_oem == MODE_BLUR ||         \
                              is_multi_camera_mode_oem == MODE_SELF_SHOT ||    \
                              is_multi_camera_mode_oem == MODE_PAGE_TURN)))


static cmr_int camera_nightpro_init(cmr_handle oem_handle);
static cmr_int camera_nightpro_deinit(cmr_handle oem_handle);

static pthread_mutex_t close_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
static pthread_mutex_t mm_dvfs_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static pthread_cond_t close_cond = PTHREAD_COND_INITIALIZER;
static uint32_t closing = 0;
static uint32_t active_camera_num = 0;
static multiCameraMode is_multi_camera_mode_oem;
static uint8_t master_id_oem = 0;
static int flag_night_dns_init = 0; // 1:using,0:not using
static float s_mfsr_ratiol = 1.99999999;
static float s_mfsr_ratioh = 15.99999999;

static void camera_send_channel_data(cmr_handle oem_handle,
                                     cmr_handle receiver_handle, cmr_uint evt,
                                     void *data);
static cmr_int camera_sensor_streamctrl(cmr_u32 on_off, void *privdata);
static cmr_int camera_isp_ctrl_done(cmr_u32 cmd, void *data);
static cmr_uint camera_isp_af_param(cmr_uint cmd, cmr_uint cmd_value);
static void camera_sensor_evt_cb(cmr_int evt, void *data, void *privdata);
static cmr_int camera_is_need_change_fmt(cmr_handle oem_handle,
                                         struct frm_info *data_ptr);
static void camera_grab_evt_cb(cmr_int evt, void *data, void *privdata);
static void camera_scaler_evt_cb(cmr_int evt, void *data, void *privdata);
static void camera_jpeg_evt_cb(enum jpg_jpeg_evt evt, void *data,
                               void *privdata);
static cmr_int camera_isp_evt_cb(cmr_handle oem_handle, cmr_u32 evt, void *data,
                                 cmr_u32 data_len);
static void camera_focus_evt_cb(enum af_cb_type cb, cmr_uint param,
                                void *privdata);
static cmr_int camera_preview_cb(cmr_handle oem_handle,
                                 enum preview_cb_type cb_type,
                                 enum preview_func_type func, void *param);
static cmr_int camera_ipm_cb(cmr_u32 class_type,
                             struct ipm_frame_out *cb_param);
static void camera_snapshot_cb_to_hal(cmr_handle oem_handle,
                                      enum snapshot_cb_type cb,
                                      enum snapshot_func_type func,
                                      void *param);
static void camera_snapshot_state_handle(cmr_handle oem_handle,
                                         enum snapshot_cb_type cb,
                                         enum snapshot_func_type func,
                                         void *param);
static void camera_snapshot_cb(cmr_handle oem_handle, enum snapshot_cb_type cb,
                               enum snapshot_func_type func, void *param);
static cmr_int camera_before_set(cmr_handle oem_handle,
                                 enum preview_param_mode mode);
static cmr_int camera_after_set(cmr_handle oem_handle,
                                struct after_set_cb_param *param);
static cmr_int camera_focus_pre_proc(cmr_handle oem_handle);
static cmr_int camera_focus_post_proc(cmr_handle oem_handle);
static cmr_int camera_get_preview_status(cmr_handle oem_handle);
static cmr_int camera_sensor_init(cmr_handle oem_handle, cmr_uint is_autotest);
static cmr_int camera_sensor_deinit(cmr_handle oem_handle);
static cmr_int camera_grab_init(cmr_handle oem_handle);
static cmr_int camera_grab_deinit(cmr_handle oem_handle);
static cmr_int camera_jpeg_init(cmr_handle oem_handle);
static cmr_int camera_jpeg_init_async(cmr_handle oem_handle);
static cmr_int camera_jpeg_init_wait(cmr_handle oem_handle);
static cmr_int camera_jpeg_deinit(cmr_handle oem_handle);
static cmr_int camera_jpeg_deinit_async(cmr_handle oem_handle);
static cmr_int camera_scaler_init(cmr_handle oem_handle);
static cmr_int camera_scaler_deinit(cmr_handle oem_handle);
static cmr_int camera_rotation_init(cmr_handle oem_handle);
static cmr_int camera_rotation_deinit(cmr_handle oem_handle);
static cmr_int camera_isp_init(cmr_handle oem_handle);
static cmr_int camera_isp_deinit_notice(cmr_handle oem_handle);
static cmr_int camera_isp_deinit(cmr_handle oem_handle);
static cmr_int camera_preview_init(cmr_handle oem_handle);
static cmr_int camera_preview_deinit(cmr_handle oem_handle);
static cmr_int camera_snapshot_init(cmr_handle oem_handle);
static cmr_int camera_snapshot_deinit(cmr_handle oem_handle);
static cmr_int camera_ipm_init(cmr_handle oem_handle);
static cmr_int camera_ipm_deinit(cmr_handle oem_handle);
static cmr_int camera_ipm_open_sw_algorithm(cmr_handle oem_handle);
static cmr_int camera_setting_init(cmr_handle oem_handle);
static cmr_int camera_setting_deinit(cmr_handle oem_handle);
static cmr_int camera_focus_init(cmr_handle oem_handle);
static cmr_int camera_focus_deinit(cmr_handle oem_handle);
static cmr_int camera_preview_cb_thread_proc(struct cmr_msg *message,
                                             void *data);
static cmr_int camera_snapshot_cb_thread_proc(struct cmr_msg *message,
                                              void *data);
static cmr_int camera_snapshot_secondary_thread_proc(struct cmr_msg *message,
                                                     void *data);
static cmr_int camera_snapshot_send_raw_thread_proc(struct cmr_msg *message,
                                                    void *data);
static cmr_int camera_create_prev_thread(cmr_handle oem_handle);
static cmr_int camera_destroy_prev_thread(cmr_handle oem_handle);
static cmr_int camera_create_video_thread(cmr_handle oem_handle);
static cmr_int camera_destroy_video_thread(cmr_handle oem_handle);
static cmr_int camera_video_cb_thread_proc(struct cmr_msg *message, void *data);
static cmr_int camera_create_snp_thread(cmr_handle oem_handle);
static cmr_int camera_destroy_snp_thread(cmr_handle oem_handle);
static cmr_int camera_init_thread(cmr_handle oem_handle);
static cmr_int camera_deinit_thread(cmr_handle oem_handle);
static cmr_int camera_res_init(cmr_handle);
static cmr_int camera_res_deinit(cmr_handle);
static cmr_int camera_res_init_internal(cmr_handle oem_handle);
static cmr_int camera_res_deinit_internal(cmr_handle oem_handle);
static cmr_int camera_init_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int camera_res_init_done(cmr_handle oem_handle);
static cmr_int camera_init_internal(cmr_handle oem_handle,
                                    cmr_uint is_autotest);
static cmr_int camera_deinit_internal(cmr_handle oem_handle);
static cmr_int camera_preview_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id,
                                       cmr_u32 preview_sn_mode);
static cmr_int camera_preview_post_proc(cmr_handle oem_handle,
                                        cmr_u32 camera_id);
static cmr_int camera_start_encode(cmr_handle oem_handle,
                                   cmr_handle caller_handle,
                                   struct img_frm *src, struct img_frm *dst,
                                   struct cmr_op_mean *mean,
                                   struct jpeg_enc_cb_param *enc_cb_param);
static cmr_int
camera_preview_face_beauty_handle(void *data,
                                  struct camera_frame_type *preview_frame);

static cmr_int
camera_video_face_beauty_handle(void *data,
                                struct camera_frame_type *video_frame);
static void camera_frame_facebeauty(void *data, struct camera_frame_type *frame,
                            struct faceBeautyLevels *beautyLevels, char *value);
static cmr_int is_video_face_beauty_on(void *data);
static cmr_int camera_get_logo_data(unsigned char *logo, int Width, int Height);
static cmr_int camera_start_decode(cmr_handle oem_handle,
                                   cmr_handle caller_handle,
                                   struct img_frm *src, struct img_frm *dst,
                                   struct cmr_op_mean *mean);
static cmr_int camera_start_exif_encode(cmr_handle oem_handle,
                                        cmr_handle caller_handle,
                                        struct img_frm *pic_src,
                                        struct img_frm *thumb_src, void *exif,
                                        struct img_frm *dst,
                                        struct jpeg_wexif_cb_param *out_ptr);
static cmr_int
camera_sw_3dnr_info_cfg(cmr_handle oem_handle,
                        struct sprd_img_3dnr_param *threednr_info);
static cmr_int
camera_start_exif_encode_simplify(cmr_handle oem_handle,
                                  struct img_frm *pic_src, struct img_frm *dst,
                                  struct jpeg_wexif_cb_param *out_ptr);
static cmr_int camera_stop_codec(cmr_handle oem_handle);
static cmr_int camera_start_scale(cmr_handle oem_handle,
                                  cmr_handle caller_handle, struct img_frm *src,
                                  struct img_frm *dst,
                                  struct cmr_op_mean *mean);
static cmr_int camera_start_rot(cmr_handle oem_handle, cmr_handle caller_handle,
                                struct img_frm *src, struct img_frm *dst,
                                struct cmr_op_mean *mean);
static cmr_int camera_ipm_pre_proc(cmr_handle oem_handle, void *private_data);
static cmr_int camera_capture_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id,
                                       cmr_u32 preview_mode,
                                       cmr_u32 capture_mode, cmr_u32 is_restart,
                                       cmr_u32 is_sn_reopen);
static cmr_int camera_capture_post_proc(cmr_handle oem_handle,
                                        cmr_u32 camera_id);
static cmr_int camera_open_sensor(cmr_handle oem_handle, cmr_u32 camera_id);
static cmr_int camera_close_sensor(cmr_handle oem_handle, cmr_u32 camera_id);
static cmr_int camera_raw_proc(cmr_handle oem_handle, cmr_handle caller_handle,
                               struct raw_proc_param *param_ptr);
static cmr_int camera_isp_start_video(cmr_handle oem_handle,
                                      struct video_start_param *param_ptr);
static cmr_int camera_isp_stop_video(cmr_handle oem_handle);
static cmr_int camera_channel_cfg(cmr_handle oem_handle,
                                  cmr_handle caller_handle, cmr_u32 camera_id,
                                  struct channel_start_param *param_ptr,
                                  cmr_u32 *channel_id,
                                  struct img_data_end *endian);
static cmr_int camera_channel_start(cmr_handle oem_handle, cmr_u32 channel_bits,
                                    cmr_uint skip_number);
static cmr_int camera_channel_pause(cmr_handle oem_handle, cmr_uint channel_id,
                                    cmr_u32 reconfig_flag);
static cmr_int camera_channel_resume(cmr_handle oem_handle, cmr_uint channel_id,
                                     cmr_u32 skip_number, cmr_u32 deci_factor,
                                     cmr_u32 frm_num);
static cmr_int camera_channel_free_frame(cmr_handle oem_handle,
                                         cmr_u32 channel_id, cmr_u32 index);
static cmr_int camera_channel_stop(cmr_handle oem_handle, cmr_u32 channel_bits);
static cmr_int camera_channel_buff_cfg(cmr_handle oem_handle,
                                       struct buffer_cfg *buf_cfg);
static cmr_int
camera_channel_cap_cfg(cmr_handle oem_handle, cmr_handle caller_handle,
                       cmr_u32 camera_id, struct cap_cfg *cap_cfg,
                       cmr_u32 *channel_id, struct img_data_end *endian);

#ifdef CONFIG_CAMERA_OFFLINE
static cmr_int camera_channel_dcam_size(cmr_handle oem_handle,
                                        struct sprd_dcam_path_size *dcam_cfg);
#endif

static cmr_int camera_get_adgain_exp_info(cmr_handle oem_handle,
                                   struct isp_adgain_exp_info *isp_adgain);
static cmr_int camera_hdr_set_ev(cmr_handle oem_handle);

static cmr_int camera_fdr_ctrl_3A(cmr_handle oem_handle,
                                   cmr_u32 enable);
static cmr_int camera_fdr_set_ev(cmr_handle oem_handle, cmr_u32 value);
static cmr_int camera_get_fdr_enable(cmr_handle oem_handle, cmr_u32 *value);
static cmr_int camera_fdr_get_frame_cnt(cmr_handle oem_handle,
                                   int *total_frame_num, int *ref_frame_num);
static cmr_int camera_get_fdr_tuning_param(cmr_handle oem_handle,
                                   struct isp_blkpm_t *tuning_param);

static cmr_int camera_channel_scale_capability(cmr_handle oem_handle,
                                               cmr_u32 *width,
                                               cmr_u32 *sc_factor,
                                               cmr_u32 *sc_threshold);
static cmr_int
camera_channel_path_capability(cmr_handle oem_handle,
                               struct cmr_path_capability *capability);
static cmr_int camera_channel_get_cap_time(cmr_handle oem_handle, cmr_u32 *sec,
                                           cmr_u32 *usec);
static cmr_int camera_set_hal_cb(cmr_handle oem_handle,
                                 camera_cb_of_type hal_cb);
static cmr_int camera_ioctl_for_setting(cmr_handle oem_handle,
                                        cmr_uint cmd_type,
                                        struct setting_io_parameter *param_ptr);
static void camera_get_iso_value(cmr_handle oem_handle);
static cmr_int camera_get_ae_lum_value(cmr_handle oem_handle);

static cmr_int camera_get_setting_activity(cmr_handle oem_handle,
                                           cmr_uint *is_active);
static cmr_int camera_set_preview_param(cmr_handle oem_handle,
                                        enum takepicture_mode mode,
                                        cmr_uint is_snapshot);
static cmr_int camera_get_preview_param(cmr_handle oem_handle,
                                        enum takepicture_mode mode,
                                        cmr_uint is_snapshot,
                                        struct preview_param *out_param_ptr);
static cmr_int camera_get_snapshot_param(cmr_handle oem_handle,
                                         struct snapshot_param *out_ptr);
static cmr_int camera_get_sensor_fps_info(cmr_handle oem_handle,
                                          cmr_uint sensor_id, cmr_u32 sn_mode,
                                          struct sensor_mode_fps_tag *fps_info);
cmr_int camera_get_jpeg_param_info(cmr_handle oem_handle,
                                   struct jpeg_param *param);
static cmr_int camera_get_sensor_autotest_mode(cmr_handle oem_handle,
                                               cmr_uint sensor_id,
                                               cmr_uint *is_autotest);
static cmr_int camera_set_setting(cmr_handle oem_handle,
                                  enum camera_param_type id, cmr_u64 param);
static void camera_set_3dnr_flag(struct camera_context *cxt,
                                 cmr_u32 threednr_flag);
static cmr_u32 camera_get_3dnr_flag(struct camera_context *cxt);
//static cmr_u32 camera_get_cnr_flag(cmr_handle oem_handle);
static cmr_u32 camera_set_ee_flag(struct camera_context *cxt,
                                 cmr_u32 ee_flag);
static cmr_u32 camera_get_cnr_realtime_flag(cmr_handle oem_handle);
static cmr_int camera_open_mfsr(struct camera_context *cxt,
                               struct ipm_open_in *in_ptr,
                               struct ipm_open_out *out_ptr);
static void camera_close_mfsr(struct camera_context *cxt);
static cmr_int camera_open_3dnr(struct camera_context *cxt,
                                struct ipm_open_in *in_ptr,
                                struct ipm_open_out *out_ptr);
static cmr_int camera_close_3dnr(struct camera_context *cxt);
static cmr_int camera_open_night_dns(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                         struct ipm_open_out *out_ptr);
static cmr_int camera_close_night_dns(struct camera_context *cxt);
static cmr_int camera_open_cnr(struct camera_context *cxt,
                               struct ipm_open_in *in_ptr,
                               struct ipm_open_out *out_ptr);
static cmr_int camera_close_cnr(struct camera_context *cxt);
static cmr_int camera_open_dre(struct camera_context *cxt,
                               struct ipm_open_in *in_ptr,
                               struct ipm_open_out *out_ptr);
static cmr_int camera_close_dre(struct camera_context *cxt);
static cmr_int camera_open_ai_scene(struct camera_context *cxt,
                                    struct ipm_open_in *in_ptr,
                                    struct ipm_open_out *out_ptr);
static cmr_int camera_close_ai_scene(struct camera_context *cxt);
static void camera_set_fdr_flag(struct camera_context *cxt, cmr_u32 fdr_flag);
static void camera_set_hdr_flag(struct camera_context *cxt, cmr_u32 hdr_flag);
static cmr_u32 camera_get_hdr_flag(struct camera_context *cxt);

static cmr_int camera_open_hdr(struct camera_context *cxt,
                               struct ipm_open_in *in_ptr,
                               struct ipm_open_out *out_ptr);
static cmr_int camera_close_hdr(struct camera_context *cxt);

static void camera_set_filter_flag(struct camera_context *cxt,
                                   cmr_u32 filter_flag);
static cmr_uint camera_get_filter_flag(struct camera_context *cxt);
static cmr_int camera_open_filter(struct camera_context *cxt,
                                  struct ipm_open_in *in_ptr,
                                  struct ipm_open_out *out_ptr);
static cmr_int camera_close_filter(struct camera_context *cxt);

static void camera_set_discard_frame(cmr_handle oem_handle,
                                     cmr_uint is_discard);
static cmr_uint camera_get_is_discard_frame(cmr_handle oem_handle,
                                            struct frm_info *data);
static void camera_set_snp_req(cmr_handle oem_handle, cmr_uint is_req);
static cmr_uint camera_get_snp_req(cmr_handle oem_handle);
static cmr_int camera_get_cap_time(cmr_handle snp_handle);
static cmr_int camera_check_cap_time(cmr_handle snp_handle,
                                     struct frm_info *data);
static void camera_snapshot_started(cmr_handle oem_handle);
static cmr_uint camera_param_to_isp(cmr_uint cmd,
                                    struct common_isp_cmd_param *parm);
static cmr_int camera_isp_ev_switch(struct common_isp_cmd_param *parm);
static cmr_int camera_restart_rot(cmr_handle oem_handle);
static cmr_uint camera_set_vendor_hdr_ev(cmr_handle oem_handle);
static cmr_uint
camera_copy_sensor_fps_info_to_isp(struct isp_sensor_fps_info *out_isp_fps,
                                   SENSOR_MODE_FPS_T *in_fps);
static cmr_uint
camera_copy_sensor_ex_info_to_isp(struct isp_sensor_ex_info *out_isp_sn_ex_info,
                                  struct sensor_ex_info *in_sn_ex_info,
                                  cmr_u32 sensor_id);
static cmr_uint camera_sensor_color_to_isp_color(cmr_u32 *isp_color,
                                                 cmr_u32 sensor_color);
static cmr_int camera_preview_get_isp_yimg(cmr_handle oem_handle,
                                           cmr_u32 camera_id,
                                           struct isp_yimg_info *yimg);
static cmr_int camera_preview_set_yimg_to_isp(cmr_handle oem_handle,
                                              cmr_u32 camera_id,
                                              struct yimg_info *yimg);
static cmr_int camera_preview_set_yuv_to_isp(cmr_handle oem_handle,
                                             cmr_u32 camera_id,
                                             struct yuv_info_t *yuv);
static void camera_filter_doeffect(cmr_handle oem_handle, struct img_frm *src,
                                   cmr_s32 type);
static void camera_set_exif_exposure_time(cmr_handle oem_handle);
static cmr_int camera_ipm_process(cmr_handle oem_handle, void *data);

static cmr_int camera_set_hdr_ev(cmr_handle oem_handle, void *data);

static cmr_uint camera_hal_ai_scene_type(cmr_u8 isp_scene_type);

static cmr_int camera_open_4in1(cmr_handle oem_handle);
static cmr_int camera_close_4in1(cmr_handle oem_handle);
static cmr_int camera_4in1_handle(cmr_int evt, void *data, void *privdata);

#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
static cmr_int camera_mm_dvfs_init(cmr_handle oem_handle);
static cmr_int camera_mm_dvfs_deinit(cmr_handle oem_handle);
#endif

cmr_int camera_invalidate_buf(cmr_handle oem_handle, cmr_s32 buf_fd,
                              cmr_u32 size, cmr_uint phy_addr,
                              cmr_uint vir_addr);
cmr_int camera_flush_buf(cmr_handle oem_handle, cmr_s32 buf_fd, cmr_u32 size,
                         cmr_uint phy_addr, cmr_uint vir_addr);


static cmr_int camera_open_fdr(struct camera_context *cxt) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 log_level;
	char value[PROPERTY_VALUE_MAX];
	struct ipmpro_context *swa_cxt = &cxt->swa_cxt_fdr;
	struct swa_fdr_init_param init_data;
	struct img_size sn_size;

	if (swa_cxt->inited == 0) {
		CMR_LOGE("fdr context is not inited\n");
		return CMR_CAMERA_NO_SUPPORT;
	}
	ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_INIT_FDR, NULL);
	if (ret) {
		CMR_LOGE("fail to init isp fdr \n");
		return CMR_CAMERA_NO_SUPPORT;
	}

	camera_fdr_get_frame_cnt(cxt, &cxt->fdr_total_frame_cnt, &cxt->fdr_ref_frame_num);
	CMR_LOGD("total frame %d, ref %d\n", cxt->fdr_total_frame_cnt, cxt->fdr_ref_frame_num);

	//get capture sensor size
	cmr_preview_get_fdr_sn_size(cxt->prev_cxt.preview_handle,
                            cxt->camera_id, &sn_size);

	init_data.zoom_ratio = cxt->zoom_ratio;
	init_data.caller_hanlde = (void *)cxt;
	init_data.cb = NULL; // do not use callback for fdr process except debug
	init_data.fdr_version = swa_cxt->version;
	init_data.total_frame_num = cxt->fdr_total_frame_cnt;
	init_data.ref_frame_num = cxt->fdr_ref_frame_num;
	init_data.sensor_fmt = SWA_IMG_FMT_MIPI_RAW14;
	init_data.sensor_pattern = SWA_RAW_PATTERN_BGGR;
	init_data.dst_fmt = SWA_IMG_FMT_NV12;
	init_data.sensor_size.width = sn_size.width;
	init_data.sensor_size.height = sn_size.height;
	init_data.dst_size.width = cxt->snp_cxt.request_size.width;
	init_data.dst_size.height = cxt->snp_cxt.request_size.height;
	init_data.param_ptr = cxt->snp_cxt.fdr_tuning_param.param_ptr;
	init_data.param_size = cxt->snp_cxt.fdr_tuning_param.param_size;
	CMR_LOGD("fdr version %d\n", init_data.fdr_version, swa_cxt->version);

	property_get("debug.cam.fdr.loglevel", value, "4");
	log_level = atoi(value);
	if (log_level < SWA_LOGLEVEL_E || log_level > SWA_LOGLEVEL_V)
		log_level = SWA_LOGLEVEL_D;

	sem_wait(&swa_cxt->sem);
	swa_cxt->swa_handle = malloc(swa_cxt->swa_handle_size);
	if (swa_cxt->swa_handle == NULL) {
		sem_post(&swa_cxt->sem);
		CMR_LOGE("failed to malloc handle\n");
		ret = CMR_CAMERA_NO_MEM;
		goto exit;
	}

	ret = swa_cxt->swa_open(swa_cxt->swa_handle, &init_data, log_level);
	sem_post(&swa_cxt->sem);

	if (ret) {
		CMR_LOGE("failed to open fdr\n");
		goto exit;
	}

	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
		time_t timep;
		struct tm *p;
		char datetime[15];

		time(&timep);
		p = localtime(&timep);
		sprintf(datetime, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
		            (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
		sprintf(cxt->dbg_cxt.tags, "fdr_%s", datetime);
		isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_DBG_TAG, cxt->dbg_cxt.tags);
		if (cxt->dbg_cxt.dbg_data) {
			free(cxt->dbg_cxt.dbg_data);
			cxt->dbg_cxt.dbg_data = NULL;
		}
		cxt->dbg_cxt.dbg_data = malloc(sizeof(struct isp_fdr_dbgdata));
	}

	return ret;

exit:
	if (swa_cxt->swa_handle) {
		free(swa_cxt->swa_handle);
		swa_cxt->swa_handle = NULL;
	}
	isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_DEINIT_FDR, NULL);

	CMR_LOGD("X. ret = %ld\n", ret);
	return ret;
}

static cmr_int camera_close_fdr(struct camera_context *cxt) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct ipmpro_context *cur = &cxt->swa_cxt_fdr;

	CMR_LOGD("E");
	sem_wait(&cur->sem);
	CMR_LOGD("start");
	if (cxt->dbg_cxt.dbg_data) {
		free(cxt->dbg_cxt.dbg_data);
		cxt->dbg_cxt.dbg_data = NULL;
	}

	if (cur->swa_handle && cur->swa_close)
		cur->swa_close(cur->swa_handle, NULL);
	if (cur->swa_handle)
		free(cur->swa_handle);
	cur->swa_handle = (cmr_handle)0;
	sem_post(&cur->sem);

	isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_DEINIT_FDR, NULL);
	CMR_LOGD("X,ret=%ld", ret);
	return ret;
}

/* send to hw reproc merged raw frame */
static cmr_int camera_fdr_reproc_v1raw(struct camera_context *cxt,
	struct swa_frame *src_frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 buf_size = 0;
	struct buffer_cfg buf_cfg, free_buf;
	struct img_size sn_size;
	struct img_addr vir_addr;
	struct isp_nlm_factor nlm_data, *nlm_factor;

	cmr_preview_get_fdr_sn_size(cxt->prev_cxt.preview_handle,
		cxt->camera_id, &sn_size);

	buf_size = sn_size.width * sn_size.height * 2;
	camera_flush_buf(cxt, src_frame->fd, buf_size,
		src_frame->addr_phy[0], src_frame->addr_vir[0]);

	CMR_LOGD("flush merged raw frame fd:%d,  buf_size:%ld, addr:0x%x, v_addr:0x%x",
		src_frame->fd, buf_size, src_frame->addr_phy[0], src_frame->addr_vir[0]);

	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
		vir_addr.addr_y = src_frame->addr_vir[0];
		dump_image_tags(cxt->dbg_cxt.tags, "merged",
			CAM_IMG_FMT_RAW14BIT, sn_size.width, sn_size.height,
			-1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr merged raw, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
			cxt->dbg_cxt.tags,
			src_frame->fd, sn_size.width, sn_size.height, vir_addr.addr_y);
	}

	memset(&free_buf, 0, sizeof(free_buf));
	ret = cmr_preview_get_fdr_free_buffer(
			cxt->prev_cxt.preview_handle, cxt->camera_id, 1, &free_buf);
	if (ret || (free_buf.count < 1)) {
		CMR_LOGE("fail to get fdr post proc rgb buffers\n");
		return CMR_CAMERA_FAIL;
	}
	cmr_preview_set_fdr_used_buffer(cxt->prev_cxt.preview_handle,
			cxt->camera_id, 1, (cmr_s32)free_buf.fd[0]);

	memset(&buf_cfg, 0, sizeof(buf_cfg));
	buf_cfg.fd[0] = src_frame->fd;
	buf_cfg.addr_vir[0].addr_y = src_frame->addr_vir[0];
	buf_cfg.addr_vir[0].addr_u = src_frame->addr_vir[1];
	buf_cfg.addr_vir[0].addr_v = src_frame->addr_vir[2];
	buf_cfg.addr[0].addr_y = src_frame->addr_phy[0];
	buf_cfg.addr[0].addr_u = src_frame->addr_phy[1];
	buf_cfg.addr[0].addr_v = src_frame->addr_phy[2];

	/* for hw output RGB buffer */
	buf_cfg.fd[1] = free_buf.fd[0];
	buf_cfg.addr_vir[1] = free_buf.addr_vir[0];
	buf_cfg.addr[1] = free_buf.addr[0];

	buf_cfg.channel_id = cxt->snp_cxt.channel_id;
	buf_cfg.base_id = src_frame->base_id;
	buf_cfg.count = 2;
	buf_cfg.slice_height = src_frame->sec;
	buf_cfg.start_buf_id = src_frame->usec;
	buf_cfg.monoboottime = src_frame->monoboottime;
	buf_cfg.is_fdr = 1;

	cxt->dbg_cxt.buff_cfg = buf_cfg;
	CMR_LOGD("fdr post proc raw, channel id %d", cxt->snp_cxt.channel_id);
	ret = cmr_grab_fdr_postproc_v1(cxt->grab_cxt.grab_handle, &buf_cfg, 0);
	if (ret) {
		CMR_LOGE("failed to buf cfg %ld", ret);
	}

	CMR_LOGD("X, ret=%ld", ret);
	return ret;
}

/* send to hw reproc fusion ouput rgb frame */
static cmr_int camera_fdr_reproc_v1rgb(struct camera_context *cxt,
	struct swa_frame *src_frame)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 buf_size = 0;
	struct buffer_cfg buf_cfg;
	struct img_size sn_size;
	struct img_addr vir_addr;

	cmr_preview_get_fdr_sn_size(cxt->prev_cxt.preview_handle,
		cxt->camera_id, &sn_size);

	buf_size = sn_size.width * sn_size.height * 3;
	camera_flush_buf(cxt, src_frame->fd, buf_size,
		src_frame->addr_phy[0], src_frame->addr_vir[0]);

	CMR_LOGD("flush fusion rgb frame fd:%d,  buf_size:%ld, addr:0x%x, v_addr:0x%x",
		src_frame->fd, buf_size, src_frame->addr_phy[0], src_frame->addr_vir[0]);

	memset(&buf_cfg, 0, sizeof(buf_cfg));
	buf_cfg.fd[0] = src_frame->fd;
	buf_cfg.addr_vir[0].addr_y = src_frame->addr_vir[0];
	buf_cfg.addr_vir[0].addr_u = src_frame->addr_vir[1];
	buf_cfg.addr_vir[0].addr_v = src_frame->addr_vir[2];
	buf_cfg.addr[0].addr_y = src_frame->addr_phy[0];
	buf_cfg.addr[0].addr_u = src_frame->addr_phy[1];
	buf_cfg.addr[0].addr_v = src_frame->addr_phy[2];

	/* for hw output yuv buffer: reuse merged out raw buffer */
	buf_cfg.fd[1] = cxt->dbg_cxt.buff_cfg.fd[0];
	buf_cfg.addr_vir[1] = cxt->dbg_cxt.buff_cfg.addr_vir[0];
	buf_cfg.addr[1] = cxt->dbg_cxt.buff_cfg.addr[0];

	buf_cfg.channel_id = cxt->snp_cxt.channel_id;
	buf_cfg.base_id = src_frame->base_id;
	buf_cfg.count = 2;
	buf_cfg.slice_height = src_frame->sec;
	buf_cfg.start_buf_id = src_frame->usec;
	buf_cfg.monoboottime = src_frame->monoboottime;
	buf_cfg.is_fdr = 1;

	cxt->dbg_cxt.buff_cfg = buf_cfg;
	CMR_LOGD("fdr post proc fusion rgb, channel id %d", cxt->snp_cxt.channel_id);
	ret = cmr_grab_fdr_postproc_v1(cxt->grab_cxt.grab_handle, &buf_cfg, 1);

	CMR_LOGD("X, ret=%ld", ret);
	return ret;
}

static cmr_int camera_fdr_reproc(struct camera_context *cxt,
	struct swa_frames_inout *out)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 buf_size = 0;
	struct buffer_cfg buf_cfg, free_buf;
	struct img_size sn_size;
	struct img_addr vir_addr;
	struct swa_frame *cur_frame;
	int i, j, n, idx_map[6] = { 0, 3, 1, 2, 4, 5 };

	if (cxt->swa_cxt_fdr.version == FDR_VERSION_1) {
		ret = camera_fdr_reproc_v1raw(cxt, &out->frms[0]);
		return ret;
	}

	if (out->frame_num < 2) {
		CMR_LOGE("fail to get fdr out frames\n");
		return CMR_CAMERA_FAIL;
	}

	cmr_preview_get_fdr_sn_size(cxt->prev_cxt.preview_handle,
		cxt->camera_id, &sn_size);

	memset(&buf_cfg, 0, sizeof(buf_cfg));
	if (out->frame_num < 6) {
		memset(&free_buf, 0, sizeof(free_buf));
		ret = cmr_preview_get_fdr_free_buffer(
				cxt->prev_cxt.preview_handle, cxt->camera_id, 0, &free_buf);
		if (ret || (free_buf.count < (6 - out->frame_num))) {
			CMR_LOGE("fail to get fdr post proc buffers \n");
			return CMR_CAMERA_FAIL;
		}
	}

	for (i = 0; i < 2; i++) {
		n = idx_map[i];
		cur_frame = &out->frms[i];

		CMR_LOGD("flush buff, buf i:%d,  fd:%d,  addr:0x%x, v_addr:0x%x",
			i, cur_frame->fd, cur_frame->addr_phy[0], cur_frame->addr_vir[0]);

		camera_flush_buf(cxt, cur_frame->fd, buf_size,
			cur_frame->addr_phy[0], cur_frame->addr_vir[0]);

		if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
			buf_size = sn_size.width * sn_size.height * 2;
			vir_addr.addr_y = cur_frame->addr_vir[0];
			dump_image_tags(cxt->dbg_cxt.tags, (i == 0) ? "low" : "high",
				CAM_IMG_FMT_RAW14BIT, sn_size.width, sn_size.height,
				-1, &vir_addr, buf_size);
			CMR_LOGD("dump fdr merged raw %s, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
				((i == 0) ? "low" : "high"), cxt->dbg_cxt.tags,
				cur_frame->fd, sn_size.width, sn_size.height, vir_addr.addr_y);
		}

		buf_cfg.fd[n] = cur_frame->fd;
		buf_cfg.addr_vir[n].addr_y = cur_frame->addr_vir[0];
		buf_cfg.addr_vir[n].addr_u = cur_frame->addr_vir[1];
		buf_cfg.addr_vir[n].addr_v = cur_frame->addr_vir[2];
		buf_cfg.addr[n].addr_y = cur_frame->addr_phy[0];
		buf_cfg.addr[n].addr_u = cur_frame->addr_phy[1];
		buf_cfg.addr[n].addr_v = cur_frame->addr_phy[2];
	}

	for (i = 2, j = 0; i < 6; i++) {
		n = idx_map[i];
		if (i < out->frame_num ) {
			cur_frame = &out->frms[i];
			buf_cfg.fd[n] = cur_frame->fd;
			buf_cfg.addr_vir[n].addr_y = cur_frame->addr_vir[0];
			buf_cfg.addr_vir[n].addr_u = cur_frame->addr_vir[1];
			buf_cfg.addr_vir[n].addr_v = cur_frame->addr_vir[2];
			buf_cfg.addr[n].addr_y = cur_frame->addr_phy[0];
			buf_cfg.addr[n].addr_u = cur_frame->addr_phy[1];
			buf_cfg.addr[n].addr_v = cur_frame->addr_phy[2];
		} else {
			buf_cfg.fd[n] = free_buf.fd[j];
			buf_cfg.addr_vir[n] = free_buf.addr_vir[j];
			buf_cfg.addr[n] = free_buf.addr[j];
			j++;
		}
	}

	cur_frame = &out->frms[0];
	buf_cfg.channel_id = cxt->snp_cxt.channel_id;
	buf_cfg.base_id = cur_frame->base_id;
	buf_cfg.count = 6;
	buf_cfg.slice_height = cur_frame->sec;
	buf_cfg.start_buf_id = cur_frame->usec;
	buf_cfg.monoboottime = cur_frame->monoboottime;
	buf_cfg.is_fdr = 1;

	cxt->dbg_cxt.buff_cfg = buf_cfg;
	CMR_LOGD("fdr post proc, channel id %d", cxt->snp_cxt.channel_id);
	ret = cmr_grab_fdr_postproc(cxt->grab_cxt.grab_handle, &buf_cfg);
	if (ret) {
		CMR_LOGE("failed to buf cfg %ld", ret);
	}

	CMR_LOGD("X, ret=%ld", ret);
	return ret;
}

static cmr_int camera_prepare_fdrparam(
	struct camera_context *cxt, struct swa_fdr_proc_param * fdr_param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct isp_blc_data *blc, blc_data;
       struct isp_fdr_dbgdata *dbg_data = cxt->dbg_cxt.dbg_data;

	memset(fdr_param, 0, sizeof(struct swa_fdr_proc_param));
	fdr_param->ae_common_info = cxt->snp_cxt.ae_common_info;
	fdr_param->ae_fdr_info = cxt->snp_cxt.fdr_ae_info;

	/* process last one frame, should input/output merge & fusion param */
	blc = &blc_data;
	ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_BLC, blc);
	CMR_LOGD("ret %d , blc %d %d %d %d\n",  ret, blc->r, blc->b, blc->gr, blc->gb);
	CMR_LOGD("ae common info ptr %p, fdr_ae_info %p\n",
		cxt->snp_cxt.ae_common_info, cxt->snp_cxt.fdr_ae_info);

	fdr_param->r = blc->r;
	fdr_param->b = blc->b;
	fdr_param->gr = blc->gr;
	fdr_param->gb = blc->gb;

	return ret;
}

static cmr_int camera_prepare_postparam(
	struct camera_context *cxt, struct swa_fdr_proc_param * post_param,
	struct common_isp_cmd_param *isp_cnr_param,
	struct common_isp_cmd_param *isp_ynr_param,
	struct swa_nrinfo_t *ee_info)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 cnr_type = 0;
	struct isp_blkpm_t ee_pm;

	cmr_bzero(isp_cnr_param, sizeof(struct common_isp_cmd_param));
	ret = camera_isp_ioctl(cxt, COM_ISP_GET_CNR2CNR3_YNR_EN, isp_cnr_param);
	if (ret)
		CMR_LOGE("isp get COM_ISP_GET_CNR2CNR3_YNR_EN  failed");
	else
		cnr_type = isp_cnr_param->cnr2cnr3_ynr_en;

	if (cnr_type & (1 << 0)) {
		cmr_bzero(isp_cnr_param, sizeof(struct common_isp_cmd_param));
		ret = camera_isp_ioctl(cxt, COM_ISP_GET_YNRS_PARAM, isp_cnr_param);
		if (ret)
			CMR_LOGE("fail to get ynrs param\n");
		else {
#ifdef CAMERA_RADIUS_ENABLE
			post_param->ynr_radius_base = isp_cnr_param->ynr_param.Radius;
			post_param->ynr_param = &isp_cnr_param->ynr_param.ynrs_param;
#else
			post_param->ynr_radius_base = 1024;
			post_param->ynr_param = &isp_cnr_param->ynr_param;
#endif
		}
	}

	if (cnr_type & (1 << 2)) {
		cmr_bzero(isp_ynr_param, sizeof(struct common_isp_cmd_param));
		ret = camera_isp_ioctl(cxt, COM_ISP_GET_CNR3_PARAM, isp_ynr_param);
		if (ret)
			CMR_LOGE("fail to get cnr3 param\n");
		else {
#ifdef CAMERA_RADIUS_ENABLE
			post_param->cnr_radius_base = isp_ynr_param->cnr3_param.baseRadius;
			post_param->cnr_param = &isp_ynr_param->cnr3_param.param_layer;
#else
			post_param->cnr_radius_base = 1024;
			post_param->cnr_param = &isp_ynr_param->cnr3_param;
#endif
		}
	}

	ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_POSTEE, &ee_pm);
	if (ret) {
		CMR_LOGE("fail to get ee param\n");
	} else {
		ee_info->param_ptr = ee_pm.param_ptr;
		ee_info->param_size = ee_pm.param_size;
		ee_info->multi_nr_map = ee_pm.multi_nr_map;
		ee_info->mode_num = ee_pm.mode_num;
		ee_info->scene_num = ee_pm.scene_num;
		ee_info->level_num = ee_pm.level_num;
		ee_info->mode_id = ee_pm.mode_id;
		ee_info->scene_id = ee_pm.scene_id;
		ee_info->ai_scene_id = ee_pm.ai_scene_id;
		ee_info->idx0 = ee_pm.idx0;
		ee_info->idx1 = ee_pm.idx1;
		ee_info->weight0 = ee_pm.weight0;
		ee_info->weight1 = ee_pm.weight1;
		post_param->ee_info = ee_info;
	}

	return ret;
}

static cmr_int camera_prepare_fdr_output(struct camera_context *cxt,
	struct frm_info *frame, struct swa_frame *src_param,
	struct img_frm *yuv_frame, struct swa_frames_inout *out)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;

	bzero(yuv_frame, sizeof(struct img_frm));
	cmr_preview_get_fdr_zsl_buffer(cxt->prev_cxt.preview_handle,
		cxt->camera_id, yuv_frame);

	memset(out, 0, sizeof(struct swa_frames_inout));
	out->frame_num = 1;
	out->frms[0] = *src_param;
	out->frms[0].fd = yuv_frame->fd;
	out->frms[0].addr_vir[0] = yuv_frame->addr_vir.addr_y;
	out->frms[0].addr_vir[1] = yuv_frame->addr_vir.addr_u;
	out->frms[0].addr_vir[2] = yuv_frame->addr_vir.addr_v;
	out->frms[0].addr_phy[0] = yuv_frame->addr_phy.addr_y;
	out->frms[0].addr_phy[1] = yuv_frame->addr_phy.addr_u;
	out->frms[0].addr_phy[2] = yuv_frame->addr_phy.addr_v;
	out->frms[0].rect.start_x = cxt->prev_cxt.rect.start_x;
	out->frms[0].rect.start_y = cxt->prev_cxt.rect.start_y;
	out->frms[0].rect.width = cxt->prev_cxt.rect.width;
	out->frms[0].rect.height = cxt->prev_cxt.rect.height;
	cxt->snp_cxt.cur_frm_info = *frame;

	CMR_LOGD("zoom ratio = %f, crop %d %d %d %d\n", cxt->zoom_ratio,
		cxt->prev_cxt.rect.start_x, cxt->prev_cxt.rect.start_y,
		cxt->prev_cxt.rect.width, cxt->prev_cxt.rect.height);

	return ret;
}

/* post proc hw output rgb */
static cmr_int camera_fdr_handle_postv1rgb(struct camera_context *cxt,
	struct frm_info *frame, struct swa_frame *src_param) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 buf_size = 0;
	struct ipmpro_context *swa_cxt = &cxt->swa_cxt_fdr;
	struct img_size pic_size;
	struct buffer_cfg free_buf;
	struct swa_frame *dst_frame;
	struct swa_frames_inout in, out;

	pic_size.width = frame->length;
	pic_size.height = frame->height;

	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
		struct img_addr vir_addr;

		vir_addr.addr_y = frame->yaddr_vir;
		buf_size = pic_size.width * pic_size.height * 3 * 2;
		dump_image_tags(cxt->dbg_cxt.tags, "rgb_hw",
		        CAM_IMG_FMT_RGB14, pic_size.width, pic_size.height,
		        -1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr rgb from hw, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
		        cxt->dbg_cxt.tags,
		        frame->fd, pic_size.width, pic_size.height, vir_addr.addr_y);
	}

	memset(&free_buf, 0, sizeof(free_buf));
	ret = cmr_preview_get_fdr_free_buffer(
			cxt->prev_cxt.preview_handle, cxt->camera_id, 1, &free_buf);
	if (ret || (free_buf.count < 1)) {
		CMR_LOGE("fail to get fdr post proc rgb buffers\n");
		return CMR_CAMERA_FAIL;
	}

	memset(&in, 0, sizeof(struct swa_frames_inout));
	in.frame_num = 1;
	in.frms[0] = *src_param;

	memset(&out, 0, sizeof(struct swa_frames_inout));
	out.frame_num = 1;
	dst_frame = &out.frms[0];
	*dst_frame = *src_param;
	dst_frame->fd = free_buf.fd[0];
	dst_frame->addr_vir[0] = free_buf.addr_vir[0].addr_y;
	dst_frame->addr_phy[0] = free_buf.addr[0].addr_y;

	ret = swa_cxt->swa_process(swa_cxt->swa_handle, &in, &out, NULL);
	if (ret) {
		CMR_LOGE("fdr swa_process failed\n");
	}

	//dump image
	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
		struct img_addr vir_addr;

		pic_size.width = frame->length;
		pic_size.height = frame->height;
		vir_addr.addr_y = dst_frame->addr_vir[0];
		buf_size = frame->height * frame->length * 3 * 2;

		dump_image_tags(cxt->dbg_cxt.tags, "rgb_sw",
		        CAM_IMG_FMT_RGB14, pic_size.width, pic_size.height, -1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr sw fusion out rgb prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
		        cxt->dbg_cxt.tags,
		        dst_frame->fd, pic_size.width, pic_size.height, vir_addr.addr_y);
	}

	ret = camera_fdr_reproc_v1rgb(cxt, dst_frame);

	CMR_LOGI("X, ret=%ld\n", ret);
	return ret;
}

static cmr_int camera_fdr_handle_postv1yuv(struct camera_context *cxt,
	struct frm_info *frame, struct swa_frame *src_param) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 buf_size = 0;
	struct ipmpro_context *swa_cxt = &cxt->swa_cxt_fdr;
	struct img_size pic_size;
	struct img_frm yuv_frame;
	struct frm_info dst_frame;
	struct buffer_cfg buf_cfg, free_buf;
	struct swa_frames_inout in, out;

	pic_size.width = frame->length;
	pic_size.height = frame->height;

	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
		struct img_addr vir_addr;

		vir_addr.addr_y = frame->yaddr_vir;
		buf_size = pic_size.width * pic_size.height * 3 / 2;
		dump_image_tags(cxt->dbg_cxt.tags, "yuv",
		        frame->fmt, pic_size.width, pic_size.height,
		        -1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr hw yuv, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
		        cxt->dbg_cxt.tags,
		        frame->fd, pic_size.width, pic_size.height, vir_addr.addr_y);
	}

	memset(&in, 0, sizeof(struct swa_frames_inout));
	in.frame_num = 1;
	in.frms[0] = *src_param;

	//Config the final yuv buffer and yuv post proc parameters
	camera_prepare_fdr_output(cxt, frame, src_param, &yuv_frame, &out);

	ret = swa_cxt->swa_process(swa_cxt->swa_handle, &in, &out, NULL);
	if (ret) {
		CMR_LOGE("fdr swa_process failed\n");
	}

	//dump image
	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
		struct img_addr vir_addr;

		pic_size.width = frame->length;
		pic_size.height = frame->height;
		vir_addr.addr_y = yuv_frame.addr_vir.addr_y;
		buf_size = frame->height * frame->length * 3 / 2;

		dump_image_tags(cxt->dbg_cxt.tags, "final",
		        frame->fmt, pic_size.width, pic_size.height, -1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr final yuv, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
		        cxt->dbg_cxt.tags,
		        yuv_frame.fd, pic_size.width, pic_size.height, vir_addr.addr_y);
	}

	dst_frame = cxt->snp_cxt.cur_frm_info;
	frame = &dst_frame;
	frame->frame_type = FRAME_COMMON;
	frame->fd = yuv_frame.fd;
	frame->yaddr = yuv_frame.addr_phy.addr_y;
	frame->uaddr = yuv_frame.addr_phy.addr_u;
	frame->vaddr = yuv_frame.addr_phy.addr_v;
	frame->yaddr_vir = yuv_frame.addr_vir.addr_y;
	frame->uaddr_vir = yuv_frame.addr_vir.addr_u;
	frame->vaddr_vir = yuv_frame.addr_vir.addr_v;
	camera_send_channel_data((cmr_handle)cxt,
		cxt->grab_cxt.caller_handle[frame->channel_id], CMR_GRAB_TX_DONE, (void *)frame);
	CMR_LOGD("send final yuv fd %d for FDR, mono time %lld\n", frame->fd, frame->monoboottime);

	return ret;
}

static cmr_int camera_fdr_handle_post(struct camera_context *cxt, struct frm_info *frame) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 buf_size = 0;
	struct ipmpro_context *swa_cxt = &cxt->swa_cxt_fdr;
	struct img_frm yuv_frame;
	struct swa_frames_inout in, out;
	struct swa_frame src_param;

	sem_wait(&swa_cxt->sem);
	if (swa_cxt->swa_handle == NULL) {
		sem_post(&swa_cxt->sem);
		CMR_LOGD("X. close_fdr may called\n");
		return ret;
	}

	/* FDR offline all done */
	if ((is_multi_camera_mode_oem == MODE_MULTI_CAMERA) &&
		(frame->frame_type == FDR_FRAME_YUVH)) {
		CMR_LOGD("multi fdr resume stream,  is stream off %d\n", cxt->fdr_call_stream_off);
		if (cxt->fdr_call_stream_off) {
			camera_stream_ctrl(cxt, 1); // sensor stream on
			ret = cmr_grab_stream_resume(cxt->grab_cxt.grab_handle);
			cxt->fdr_call_stream_off = 0;
		}
	}

	bzero(&src_param, sizeof(struct swa_frame));
	src_param.fd = frame->fd;
	src_param.fmt = frame->fmt;
	src_param.base_id = frame->frame_real_id;
	src_param.sec = frame->sec;
	src_param.usec = frame->usec;
	src_param.addr_vir[0] = frame->yaddr_vir;
	src_param.addr_vir[1] = frame->uaddr_vir;
	src_param.addr_vir[1] = frame->vaddr_vir;
	src_param.addr_phy[0] = frame->yaddr;
	src_param.addr_phy[1] = frame->uaddr;
	src_param.addr_phy[2] = frame->vaddr;
	src_param.size.height = frame->height;
	src_param.size.width = frame->length;
	src_param.monoboottime = frame->monoboottime;

	 //invalidate buf
	buf_size = frame->height * frame->length * 2;
	CMR_LOGD("fdr fd: %d, mono time %lld buf_size:%ld, w %d h %d",
			frame->fd, frame->monoboottime, buf_size, frame->length, frame->height);
	CMR_LOGD("fdr addr: 0x%lx, 0x%lx, 0x%lx, vir_addr: 0x%lx, 0x%lx, 0x%lx",
	             frame->yaddr, frame->uaddr, frame->vaddr,
	             frame->yaddr_vir, frame->uaddr_vir, frame->vaddr_vir);
	ret = camera_invalidate_buf(cxt, frame->fd, buf_size, frame->yaddr, frame->yaddr_vir);

	if (swa_cxt->version == FDR_VERSION_1) {
		if (frame->frame_type == FDR_FRAME_YUVL)
			ret = camera_fdr_handle_postv1rgb(cxt, frame, &src_param);
		else
			ret = camera_fdr_handle_postv1yuv(cxt, frame, &src_param);

		sem_post(&swa_cxt->sem);
		return ret;
	}

	//dump image
	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits ) {
		struct img_addr vir_addr;
		struct buffer_cfg *buf_cfg = &cxt->dbg_cxt.buff_cfg;
		struct sensor_mode_info  *mode_info;
		cmr_u32 i, sn_mode = 0;
		struct img_size pic_size;

		pic_size.width = frame->length;
		pic_size.height = frame->height;

		i = (frame->frame_type == FDR_FRAME_YUVL) ? 0 : 3;
		vir_addr.addr_y = frame->yaddr_vir;
		buf_size = pic_size.width * pic_size.height * 3 / 2;
		dump_image_tags(cxt->dbg_cxt.tags, (i == 0) ? "low" : "high",
		        frame->fmt, pic_size.width, pic_size.height,
		        -1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr yuv %s, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
		        ((i == 0) ? "low" : "high"), cxt->dbg_cxt.tags,
		        frame->fd, pic_size.width, pic_size.height, vir_addr.addr_y);

		cmr_preview_get_fdr_sn_size(cxt->prev_cxt.preview_handle, cxt->camera_id, &pic_size);
		vir_addr.addr_y = buf_cfg->addr_vir[i + 1].addr_y;
		buf_size = pic_size.width * pic_size.height * 2;
		ret = camera_invalidate_buf(cxt, buf_cfg->fd[i + 1], buf_size,
		                buf_cfg->addr[0].addr_y, buf_cfg->addr_vir[0].addr_y);

		dump_image_tags(cxt->dbg_cxt.tags, (i == 0) ? "low" : "high",
		        CAM_IMG_FMT_DCAM_RAW14BIT, pic_size.width, pic_size.height,
		        -1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr dcam raw %s, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
		        ((i == 0) ? "low" : "high"), cxt->dbg_cxt.tags,
		        buf_cfg->fd[i + 1], pic_size.width, pic_size.height, vir_addr.addr_y);
	}

	memset(&in, 0, sizeof(struct swa_frames_inout));
	in.frame_num = 1;
	in.frms[0] = src_param;

	//Config the final yuv buffer
	if (frame->frame_type == FDR_FRAME_YUVH)
		camera_prepare_fdr_output(cxt, frame, &src_param, &yuv_frame, &out);

	ret = swa_cxt->swa_process(swa_cxt->swa_handle, &in, &out, NULL);
	if (ret) {
		CMR_LOGE("fdr swa_process failed\n");
	}

	//dump image
	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits && frame->frame_type == FDR_FRAME_YUVH) {
		struct img_addr vir_addr;
		struct img_size pic_size;

		pic_size.width = frame->length;
		pic_size.height = frame->height;
		vir_addr = yuv_frame.addr_vir;
		buf_size = frame->height * frame->length * 3 /2;

		dump_image_tags(cxt->dbg_cxt.tags, "fusion",
		        frame->fmt, pic_size.width, pic_size.height, -1, &vir_addr, buf_size);
		CMR_LOGD("dump fdr fusion yuv, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n",
		        cxt->dbg_cxt.tags, yuv_frame.fd, pic_size.width, pic_size.height, vir_addr.addr_y);
	}

	if (frame->frame_type == FDR_FRAME_YUVH) {
		cmr_handle receiver_handle;
		cmr_int evt = CMR_GRAB_TX_DONE;
		struct frm_info dst_frame;

		dst_frame = cxt->snp_cxt.cur_frm_info;
		frame = &dst_frame;
		frame->frame_type = FRAME_COMMON;
		frame->fd = yuv_frame.fd;
		frame->yaddr = yuv_frame.addr_phy.addr_y;
		frame->uaddr = yuv_frame.addr_phy.addr_u;
		frame->vaddr = yuv_frame.addr_phy.addr_v;
		frame->yaddr_vir = yuv_frame.addr_vir.addr_y;
		frame->uaddr_vir = yuv_frame.addr_vir.addr_u;
		frame->vaddr_vir = yuv_frame.addr_vir.addr_v;
		CMR_LOGD("send final yuv fd %d for FDR, mono time %lld\n", frame->fd, frame->monoboottime);
		receiver_handle = cxt->grab_cxt.caller_handle[frame->channel_id];
		camera_send_channel_data((cmr_handle)cxt, receiver_handle, evt, (void *)frame);
	}

	sem_post(&swa_cxt->sem);
	CMR_LOGI("X, ret=%ld\n", ret);
	return ret;
}

cmr_int camera_fdr_handle(void *data, void *privdata) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	cmr_u32 buf_size = 0;
	struct camera_context *cxt = (struct camera_context *)privdata;
	struct frm_info *frame = (struct frm_info *)data;
	struct img_size sn_size;
	struct ipmpro_context *swa_cxt = &cxt->swa_cxt_fdr;
	struct swa_frames_inout in, out;
	struct swa_frame *cur_frm;
	struct swa_frame src_param;
	struct swa_fdr_proc_param fdr_param;
	struct isp_nlm_factor nlm_data, *nlm_factor;
	struct isp_info fdr_dbg_info;
	struct isp_fdr_dbgdata *dbg_data = cxt->dbg_cxt.dbg_data;
	struct swa_nrinfo_t ee_info;
	struct common_isp_cmd_param isp_param;
	struct common_isp_cmd_param isp_cnr_param, isp_ynr_param;

	CMR_LOGD("type %d ch_id:%d, fd=%d  size %d %d, mono time %lld addr: 0x%lx, vir_addr: 0x%lx",
		frame->frame_type, frame->channel_id, frame->fd, frame->length, frame->height,
		frame->monoboottime, frame->yaddr, frame->yaddr_vir);

	if (frame->frame_type == FDR_FRAME_YUVL ||  frame->frame_type == FDR_FRAME_YUVH) {
		ret = camera_fdr_handle_post(cxt, frame);
		return ret;
	}

	if (frame->monoboottime < cxt->fdr_capture_timestamp) {
		CMR_LOGD("skip early frame fd %d, time %llu\n", frame->fd, frame->monoboottime);
		cmr_preview_set_fdr_used_buffer(cxt->prev_cxt.preview_handle,
		                    cxt->camera_id, 0, frame->fd);
		return ret;
	}

	sem_wait(&swa_cxt->sem);
	if (swa_cxt->swa_handle == NULL) {
		sem_post(&swa_cxt->sem);
		CMR_LOGD("X. close_fdr may called\n");
		return ret;
	}

	//get capture sensor size
	cmr_preview_get_fdr_sn_size(cxt->prev_cxt.preview_handle,
	                    cxt->camera_id, &sn_size);
	frame->height = sn_size.height;
	frame->length = sn_size.width;
	CMR_LOGD("fdr sn size %d %d, \n", frame->length, frame->height);

	memset(&src_param, 0, sizeof(src_param));
	src_param.fd = frame->fd;
	src_param.fmt = frame->fmt;
	src_param.base_id = frame->frame_real_id;
	src_param.sec = frame->sec;
	src_param.usec = frame->usec;
	src_param.addr_vir[0] = frame->yaddr_vir;
	src_param.addr_vir[1] = frame->uaddr_vir;
	src_param.addr_vir[1] = frame->vaddr_vir;
	src_param.addr_phy[0] = frame->yaddr;
	src_param.addr_phy[1] = frame->uaddr;
	src_param.addr_phy[2] = frame->vaddr;
	src_param.size.height = frame->height;
	src_param.size.width = frame->length;
	src_param.monoboottime = frame->monoboottime;
	cxt->fdr_capture_frame_cnt++;

	if (cxt->fdr_capture_frame_cnt == cxt->fdr_ref_frame_num ||
        cxt->fdr_capture_frame_cnt == cxt->fdr_ref_frame_num + 1) {
		camera_prepare_fdrparam(cxt, &fdr_param);
		camera_prepare_postparam(cxt,
			&fdr_param, &isp_cnr_param, &isp_ynr_param, &ee_info);
	}

	//invalidate cache
	buf_size = frame->height * frame->length * 2;
	ret = camera_invalidate_buf(cxt, frame->fd, buf_size, frame->yaddr, frame->yaddr_vir);
	if (ret) {
		CMR_LOGE("fdr invalidate buf %d cache failed\n", frame->fd);
	}

	//dump image
	if (cxt->dbg_cxt.inited && cxt->dbg_cxt.dump_bits) {
		struct img_addr addr_vir;
		addr_vir.addr_y = frame->yaddr_vir;
		dump_image_tags(cxt->dbg_cxt.tags, NULL,
		        CAM_IMG_FMT_RAW14BIT, frame->length, frame->height,
		        frame->frame_real_id, &addr_vir,
		        frame->height * frame->length * 2);
		CMR_LOGD("dump fdr raw, prefix %s, fd %d,  w %d h %d,  vaddr %lx\n", cxt->dbg_cxt.tags,
		        frame->fd, sn_size.width, sn_size.height, addr_vir.addr_y);
	}

	//record fdr used buffer
	cmr_preview_set_fdr_used_buffer(cxt->prev_cxt.preview_handle,
	                    cxt->camera_id, 0, frame->fd);

	if (cxt->fdr_capture_frame_cnt >= cxt->fdr_total_frame_cnt) {
		CMR_LOGD("fdr stop capture");
		ret = cmr_grab_stop_capture(cxt->grab_cxt.grab_handle);
		if (ret) {
			CMR_LOGE("cmr_grab_stop_capture failed");
		}

		if (is_multi_camera_mode_oem == MODE_MULTI_CAMERA) {
			CMR_LOGD("multi fdr mode, call stream off");
			camera_stream_ctrl(cxt, 0); // sensor stream off
			ret = cmr_grab_stream_pause(cxt->grab_cxt.grab_handle);
			cxt->fdr_call_stream_off = 1;
		}
	}

	memset(&in, 0, sizeof(struct swa_frames_inout));
	memset(&out, 0, sizeof(struct swa_frames_inout));
	in.frame_num = 1;
	in.frms[0] = src_param;

	ret = swa_cxt->swa_process(swa_cxt->swa_handle, &in, &out, &fdr_param);
	if (ret) {
		CMR_LOGE("fdr swa_process failed\n");
		goto exit;
	}

	/* no output should be processed if it is not the last frame */
	if (cxt->fdr_capture_frame_cnt < cxt->fdr_total_frame_cnt)
		goto exit;

	fdr_dbg_info.addr = fdr_param.dbg_info_addr;
	fdr_dbg_info.size = fdr_param.dbg_info_size;
	isp_param.cmd_ptr = (void *)&fdr_dbg_info;
	CMR_LOGD("fdr_dbg_info addr %p, size %d", fdr_dbg_info.addr, fdr_dbg_info.size);
	if (fdr_dbg_info.addr) {
		ret = camera_isp_ioctl(cxt, COM_ISP_SET_FDR_LOG, (void *)&isp_param);
		if (ret) {
			CMR_LOGE("failed to set FDR exif debug log");
		}
	}

	if (swa_cxt->version == FDR_VERSION_0) {
		nlm_factor = &nlm_data;
		nlm_factor->nlm_out_ratio0 = fdr_param.nlm_out_ratio0;
		nlm_factor->nlm_out_ratio1 = fdr_param.nlm_out_ratio1;
		nlm_factor->nlm_out_ratio2 = fdr_param.nlm_out_ratio2;
		nlm_factor->nlm_out_ratio3 = fdr_param.nlm_out_ratio3;
		nlm_factor->nlm_out_ratio4 = fdr_param.nlm_out_ratio4;
		CMR_LOGD("nlm_factor = %d %d %d %d %d\n",
		         nlm_factor->nlm_out_ratio0, nlm_factor->nlm_out_ratio1,
		         nlm_factor->nlm_out_ratio2, nlm_factor->nlm_out_ratio3,
		         nlm_factor->nlm_out_ratio4);
	} else {
		nlm_factor = NULL;
	}

	/* ISP_CTRL_UPDATE_FDR called after merge and before SPRD_IMG_IO_POST_FDR */
	ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_UPDATE_FDR, nlm_factor);
	if (ret) {
		CMR_LOGE("ISP_CTRL_UPDATE_FDR");
	}

	if (dbg_data) {
		dbg_data->total_frm_num = cxt->fdr_total_frame_cnt;
		dbg_data->ref_frm_num = cxt->fdr_ref_frame_num;
		dbg_data->nlm_out_ratio0 = fdr_param.nlm_out_ratio0;
		dbg_data->nlm_out_ratio1 = fdr_param.nlm_out_ratio1;
		dbg_data->nlm_out_ratio2 = fdr_param.nlm_out_ratio2;
		dbg_data->nlm_out_ratio3 = fdr_param.nlm_out_ratio3;
		dbg_data->nlm_out_ratio4 = fdr_param.nlm_out_ratio4;
		ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_FDR_DBG_DATA, dbg_data);
		if (ret) {
			CMR_LOGE("fail to set fdr dbg data\n");
		}
	}

	ret = camera_fdr_reproc(cxt, &out);

exit:
	sem_post(&swa_cxt->sem);
	CMR_LOGI("X, ret=%ld\n", ret);
	return ret;
}


static cmr_int camera_ipmpro_init(cmr_handle oem_handle) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	void *sw_handle = NULL;
	struct camera_context *cxt = (struct camera_context *)oem_handle;
	struct ipmpro_context *swa_cxt = &cxt->swa_cxt_fdr;;
	ipmpro_get_handle_size swa_get_size;

	swa_cxt->inited = 0;
#ifndef CONFIG_CAMERA_FDR
	CMR_LOGV("fdr is not supported\n");
	return ret;
#endif

	sw_handle = NULL;
	sw_handle = get_lib_handle("libfdr.so");
	if (sw_handle)
		swa_cxt->get_flag = 1;
	else
		sw_handle = dlopen("libfdr.so", RTLD_NOW);

	if (sw_handle == NULL) {
		CMR_LOGE("fail to open libfdr.so\n");
		return ret;
	}

	swa_get_size = dlsym(sw_handle, "swa_get_handle_size");
	if (swa_get_size)
		swa_cxt->swa_handle_size = swa_get_size();
	if (swa_get_size == NULL || swa_cxt->swa_handle_size <= 0) {
		CMR_LOGE("fail to get swa_get_handle_size %p, %d from libfdr.so",
			swa_get_size, swa_cxt->swa_handle_size);
		goto exit;
	}

	swa_cxt->swa_open = dlsym(sw_handle, "swa_open");
	swa_cxt->swa_process = dlsym(sw_handle, "swa_process");
	swa_cxt->swa_close = dlsym(sw_handle, "swa_close");
	if (!swa_cxt->swa_open || !swa_cxt->swa_process || !swa_cxt->swa_close) {
		CMR_LOGE("fail to get libfdr, for api %p %p %p\n",
			swa_cxt->swa_open, swa_cxt->swa_process, swa_cxt->swa_close);
		goto exit;
	}

	CMR_LOGD("libfdr.so inited, handle %p api %p %p %p, data size %d\n", sw_handle,
		swa_cxt->swa_open, swa_cxt->swa_process, swa_cxt->swa_close, swa_cxt->swa_handle_size);
	sem_init(&swa_cxt->sem_flag, 0, 1);
	sem_init(&swa_cxt->sem, 0, 1);
	swa_cxt->lib_handle = sw_handle;
	swa_cxt->inited = 1;
#if defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_7)
	swa_cxt->version = FDR_VERSION_0;
#else
	swa_cxt->version = FDR_VERSION_1;
#endif

	return 0;

exit:
	swa_cxt->swa_open = NULL;
	swa_cxt->swa_process = NULL;
	swa_cxt->swa_close = NULL;
	if (swa_cxt->get_flag)
		put_lib_handle(sw_handle);
	else
		dlclose(sw_handle);

	return ret;
}

static cmr_int camera_ipmpro_deinit(cmr_handle oem_handle) {
	cmr_int ret = CMR_CAMERA_SUCCESS;
	void *sw_handle = NULL;
	struct camera_context *cxt = (struct camera_context *)oem_handle;
	struct ipmpro_context *swa_cxt;

	swa_cxt = &cxt->swa_cxt_fdr;
	if (swa_cxt->inited == 0)
		return ret;

	swa_cxt->inited = 0;
	sem_wait(&swa_cxt->sem);
	if (swa_cxt->swa_handle && swa_cxt->swa_close)
		swa_cxt->swa_close(swa_cxt->swa_handle, NULL);
	if (swa_cxt->swa_handle)
		free(swa_cxt->swa_handle);
	swa_cxt->swa_handle = (cmr_handle)0;

	swa_cxt->swa_handle_size = 0;
	swa_cxt->swa_open = NULL;
	swa_cxt->swa_process = NULL;
	swa_cxt->swa_close = NULL;
	sem_destroy(&swa_cxt->sem_flag);
	sem_destroy(&swa_cxt->sem);
	if (swa_cxt->get_flag)
		put_lib_handle(swa_cxt->lib_handle);
	else
		dlclose(swa_cxt->lib_handle);
	swa_cxt->lib_handle = (cmr_handle)0;
	swa_cxt->get_flag = 0;

	return ret;
}

cmr_int camera_malloc(cmr_u32 mem_type, cmr_handle oem_handle,
                      cmr_u32 *size_ptr, cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                      cmr_uint *vir_addr, cmr_s32 *fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !vir_addr || !size_ptr || !sum_ptr || !fd) {
        CMR_LOGE("error param mem_type 0x%x, oem_handle %p vir_addr %p, fd %p",
                 mem_type, oem_handle, vir_addr, fd);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (cxt->hal_malloc == NULL) {
        CMR_LOGE("cxt->hal_malloc is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGD("mem_type=%d size=%d sum=%d", mem_type, *size_ptr, *sum_ptr);

    ret = cxt->hal_malloc(mem_type, size_ptr, sum_ptr, phy_addr, vir_addr, fd,
                          cxt->client_data);
    if (ret) {
        CMR_LOGE("cxt->hal_malloc failed, ret=%d", ret);
    }

exit:
    return ret;
}

cmr_int camera_gpu_malloc(cmr_u32 mem_type, cmr_handle oem_handle,
                          cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
                          cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                          void **handle, cmr_uint *width, cmr_uint *height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !vir_addr || !size_ptr || !sum_ptr || !fd || !handle) {
        CMR_LOGE("error param mem_type 0x%x, oem_handle %p vir_addr %p, fd %p "
                 "handle %p",
                 mem_type, oem_handle, vir_addr, fd, handle);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (cxt->hal_gpu_malloc == NULL) {
        CMR_LOGE("cxt->hal_gpu_malloc is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGD("mem_type=%d size=%d sum=%d", mem_type, *size_ptr, *sum_ptr);

    ret = cxt->hal_gpu_malloc(mem_type, size_ptr, sum_ptr, phy_addr, vir_addr,
                              fd, handle, width, height, cxt->client_data);
    if (ret) {
        CMR_LOGE("cxt->hal_gpu_malloc failed, ret=%d", ret);
    }

exit:
    return ret;
}

cmr_int camera_free(cmr_u32 mem_type, cmr_handle oem_handle, cmr_uint *phy_addr,
                    cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !vir_addr || !fd) {
        CMR_LOGE("error param mem_type=0x%x,oem_handle=%p,fd=0x%x,vir_addr=%p",
                 mem_type, oem_handle, (cmr_u32)fd, vir_addr);
        return -CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD("mem_type=%d sum=%d", mem_type, sum);
    ret =
        cxt->hal_free(mem_type, phy_addr, vir_addr, fd, sum, cxt->client_data);

    return ret;
}

cmr_int camera_invalidate_buf(cmr_handle oem_handle, cmr_s32 buf_fd,
                              cmr_u32 size, cmr_uint phy_addr,
                              cmr_uint vir_addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cam_ion_buffer_t ion_buf;

    if (!oem_handle || buf_fd < 0) {
        CMR_LOGE("error param oem_handle %p fd %d", oem_handle, buf_fd);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (cxt->camera_cb == NULL) {
        CMR_LOGE("cxt->camera_cb is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ion_buf.fd = (int)buf_fd;
    ion_buf.size = (size_t)size;
    ion_buf.addr_phy = (void *)&phy_addr;
    ion_buf.addr_vir = (void *)&vir_addr;
    cxt->camera_cb(CAMERA_EVT_CB_INVALIDATE_BUF, cxt->client_data,
                   CAMERA_FUNC_BUFCACHE, (void *)&ion_buf);
exit:
    return ret;
}

cmr_int camera_flush_buf(cmr_handle oem_handle, cmr_s32 buf_fd, cmr_u32 size,
                         cmr_uint phy_addr, cmr_uint vir_addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cam_ion_buffer_t ion_buf;

    if (!oem_handle || buf_fd < 0) {
        CMR_LOGE("error param oem_handle %p fd %d", oem_handle, buf_fd);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (cxt->camera_cb == NULL) {
        CMR_LOGE("cxt->camera_cb is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ion_buf.fd = (int)buf_fd;
    ion_buf.size = (size_t)size;
    ion_buf.addr_phy = (void *)&phy_addr;
    ion_buf.addr_vir = (void *)&vir_addr;
    cxt->camera_cb(CAMERA_EVT_CB_FLUSH_BUF, cxt->client_data,
                   CAMERA_FUNC_BUFCACHE, (void *)&ion_buf);
exit:
    return ret;
}

cmr_int camera_write_sysfs_file(const char *filename, cmr_u32 value) {
    int32_t bytes = 0;
    char buffer[16];
    int ret = 0;
    int fd;
    CMR_LOGV("E");

    fd = open(filename, O_WRONLY);

    if (-1 == fd) {
        CMR_LOGE("Failed to open: sysfs_file %s", filename);
        return -EINVAL;
    }

    bytes = snprintf(buffer, sizeof(buffer), "0x%x", value);
    if (write(fd, buffer, bytes) != bytes) {
        CMR_LOGE("write failed\n");
        ret = -EINVAL;
    }

    close(fd);
    CMR_LOGV("X");

    return ret;
}

cmr_int camera_read_sysfs_file(const char *filename, cmr_u8 *value) {
    int32_t bytes = 0;
    int ret = 0;
    int fd;
    char buffer[4] = {0};

    CMR_LOGV("E");

    fd = open(filename, O_RDONLY);

    if (-1 == fd) {
        CMR_LOGE("Failed to open: sysfs_file %s", filename);
        return -EINVAL;
    }

    ret = read(fd, buffer, sizeof(buffer));
    if (ret <= 0) {
        CMR_LOGE("read failed\n");
        ret = -EINVAL;
    }
    CMR_LOGV("buffer %s", buffer);

    close(fd);
    *value = atoi(buffer);

    CMR_LOGV("X");

    return ret;
}

cmr_int camera_front_lcd_flash_activie(cmr_u32 face_type) {
    bool isFrontLCD =
        (strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") == 0) ? true : false;

    if (face_type == 1 && isFrontLCD)
        return 1;

    return 0;
}

cmr_int camera_front_lcd_set_color_temperature(struct camera_context *cxt) {

    if (cxt->enhance == NULL) {
        CMR_LOGE("enhance object invalid");
        return -1;
    }

    if (cxt->enhance->set_value(cxt->color_temp))
        CMR_LOGE("set temperature %ld failed\n", cxt->color_temp);

    return 0;
}

cmr_int camera_front_lcd_enhance_module_init(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    const hw_module_t *module;

    if (!camera_front_lcd_flash_activie(cxt->facing)) {
        CMR_LOGI("flash is not lcd type");
        return -1;
    }

    if (hw_get_module(ENHANCE_HARDWARE_MODULE_ID, &module)) {
        CMR_LOGE("load enhance.so failed");
        return -1;
    }

    if (module->methods->open(module, "flash",
                              (struct hw_device_t **)&(cxt->enhance))) {
        CMR_LOGE("open enhance.so failed");
        return -1;
    }

    return 0;
}

cmr_int camera_front_lcd_enhance_module_deinit(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_u8 flip = 0;
    const char *brightness = "/sys/class/backlight/sprd_backlight/brightness";
    const char *refresh = "/sys/class/display/dispc0/refresh";
    const char *disable_flip = "/sys/class/display/dispc0/disable_flip";

    if (!camera_front_lcd_flash_activie(cxt->facing)) {
        CMR_LOGI("flash is not lcd type");
        return -1;
    }

    camera_read_sysfs_file(disable_flip, &flip);
    CMR_LOGI("disable_flip %d", flip);

    if (flip) {
        camera_write_sysfs_file(brightness, cxt->backup_brightness);
        camera_write_sysfs_file(refresh, 0x01);
        cxt->color_temp = 0;
        camera_front_lcd_set_color_temperature(cxt);

        cxt->bg_color = 0;
        cxt->backlight_brightness = 0;
        cxt->lcd_flash_highlight = 0;
    }

    if (cxt->enhance) {
        cxt->enhance->common.close((struct hw_device_t *)cxt->enhance);
    }

    return 0;
}

cmr_int camera_front_lcd_flash_cfg(struct camera_context *cxt,
                                   struct sprd_flash_cfg_param *cfg) {

    cxt->lcd_flash_highlight = cfg->real_cell.type;
    cxt->backlight_brightness = cfg->real_cell.element[0].brightness;
    cxt->color_temp = cfg->real_cell.element[0].color_temp;
    cxt->bg_color = cfg->real_cell.element[0].bg_color;

    CMR_LOGI("lcd_flash_highlight:%d,brightness %d,color_temp %d,bg_color 0x%x",
             cxt->lcd_flash_highlight, cxt->backlight_brightness,
             cxt->color_temp, cxt->bg_color);

    return 0;
}

cmr_int camera_front_lcd_flash_callback(struct camera_context *cxt,
                                        cmr_u32 flash_mode) {
    const char *bg_color = "/sys/class/display/dispc0/bg_color";
    const char *brightness = "/sys/class/backlight/sprd_backlight/brightness";
    const char *refresh = "/sys/class/display/dispc0/refresh";
    const char *disable_flip = "/sys/class/display/dispc0/disable_flip";
    cmr_u8 flip = 0;

    switch (flash_mode) {
    case FLASH_OPEN:
        cxt->color_temp = cxt->color_temp ? cxt->color_temp : 0;
        camera_front_lcd_set_color_temperature(cxt);

        cxt->bg_color = cxt->bg_color ? cxt->bg_color : 0xffffff;
        camera_write_sysfs_file(bg_color, cxt->bg_color);

        camera_read_sysfs_file(brightness, &(cxt->backup_brightness));
        CMR_LOGI("backup_brightness:%d", cxt->backup_brightness);

        cxt->backlight_brightness =
            cxt->backlight_brightness ? cxt->backlight_brightness : 0xff;
        camera_write_sysfs_file(brightness, cxt->backlight_brightness);

        break;
    case FLASH_HIGH_LIGHT:
        CMR_LOGI("flash highlight");

        // cxt->color_temp = cxt->color_temp ? cxt->color_temp : 0;
        camera_front_lcd_set_color_temperature(cxt);

        // cxt->bg_color = cxt->bg_color ? cxt->bg_color : 0xffffff;
        camera_write_sysfs_file(bg_color, cxt->bg_color);

        // cxt->backlight_brightness =
        //    cxt->backlight_brightness ? cxt->backlight_brightness : 0xff;
        camera_write_sysfs_file(brightness, cxt->backlight_brightness);

        break;
    case FLASH_CLOSE_AFTER_OPEN:

        camera_read_sysfs_file(disable_flip, &flip);
        CMR_LOGI("lcd_flash_highlight %d, disable_flip %d", cxt->lcd_flash_highlight, flip);

        if (cxt->lcd_flash_highlight && flip) {
            camera_write_sysfs_file(brightness, cxt->backup_brightness);
            camera_write_sysfs_file(refresh, 0x01);
            cxt->color_temp = 0;
            camera_front_lcd_set_color_temperature(cxt);

            cxt->bg_color = 0;
            cxt->backlight_brightness = 0;
            cxt->lcd_flash_highlight = 0;
        }
        break;
    }

    return 0;
}

void camera_snapshot_started(cmr_handle oem_handle) {
    camera_snapshot_cb_to_hal(oem_handle, SNAPSHOT_CB_EVT_PREPARE,
                              SNAPSHOT_FUNC_TAKE_PICTURE, 0);
}

void camera_set_discard_frame(cmr_handle oem_handle, cmr_uint is_discard) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    sem_wait(&cxt->access_sm);
    cxt->is_discard_frm = is_discard;
    sem_post(&cxt->access_sm);
    CMR_LOGD("%ld", cxt->is_discard_frm);
}

cmr_uint camera_get_is_discard_frame(cmr_handle oem_handle,
                                     struct frm_info *data) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;
    cmr_uint is_discard = 0;

    if (snp_cxt->channel_id == data->channel_id) {
        sem_wait(&cxt->access_sm);
        is_discard = cxt->is_discard_frm;
        sem_post(&cxt->access_sm);
    }
    CMR_LOGV("%ld", is_discard);
    return is_discard;
}

void camera_set_snp_req(cmr_handle oem_handle, cmr_uint is_req) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    sem_wait(&cxt->access_sm);
    cxt->snp_cxt.is_req_snp = is_req;
    sem_post(&cxt->access_sm);
    CMR_LOGV("%ld", cxt->snp_cxt.is_req_snp);
}

cmr_uint camera_get_snp_req(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_uint is_req;

    is_req = cxt->snp_cxt.is_req_snp;
    CMR_LOGV("%ld", is_req);

    return is_req;
}

cmr_uint camera_set_vendor_hdr_ev(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct common_sn_cmd_param sn_param;
    struct common_isp_cmd_param isp_param;
    cmr_uint ev_value;

    if (!(cxt->is_vendor_hdr))
        return ret;

    cxt->cap_cnt++;
    if (cxt->cap_cnt == 1) {
        /* set ev = 1 */
        ev_value = OEM_EV_LEVEL_2;
    } else if (cxt->cap_cnt == 2) {
        /*set ev = 2 */
        ev_value = OEM_EV_LEVEL_3;
    } else {
        /* set ev = 1 as default */
        cxt->cap_cnt = 0;
        ev_value = OEM_EV_LEVEL_2;
    }

    struct sensor_exp_info sensor_info;
    ret = camera_get_sensor_info(cxt, cxt->camera_id, &sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info.image_format) {
        isp_param.cmd_value = ev_value;
        ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_HDR, (void *)&isp_param);
    } else {
        sn_param.cmd_value = ev_value;
        ret = camera_sensor_ioctl(oem_handle, COM_SN_SET_HDR_EV,
                                  (void *)&sn_param);
    }

exit:
    return ret;
}

static cmr_int camera_ips_get_params(struct camera_context *cxt,
	struct swa_frame_param *frm_param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct swa_common_info *com_info;
	struct common_isp_cmd_param isp_param;

	memset(frm_param, 0, sizeof(struct swa_frame_param));

	/*get common info */
	com_info = &frm_param->common_param;

	ret = camera_isp_ioctl(cxt, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
	com_info->again = isp_param.isp_adgain.adgain;
	com_info->exp_time = isp_param.isp_adgain.exp_time;
	com_info->bv = isp_param.isp_adgain.bv;

	ret = camera_isp_ioctl(cxt, COM_ISP_GET_CUR_COL_TEM, &isp_param);
	com_info->ct = isp_param.isp_cur_ct;
	ret = camera_isp_ioctl(cxt, COM_ISP_GET_CUR_SENS, &isp_param);
	com_info->iso = isp_param.isp_cur_iso;

	com_info->zoom_ratio = cxt->zoom_ratio;
	com_info->is_front = (cxt->camera_id == 1);

	/* get mfsr param */
	if (cxt->snp_cxt.is_mfsr) {
		struct img_rect frame_crop, crop_x, crop1;
		float x, y, w, h, ratio;

		ret = camera_isp_ioctl(cxt, COM_ISP_GET_MFSR_PARAM, &isp_param);
		if (ret) {
			CMR_LOGE("fail to get mfsr param\n");
		} else {
			frm_param->mfsr_param.data = isp_param.isp_blk_param.param_ptr;
			frm_param->mfsr_param.data_size = isp_param.isp_blk_param.param_size;
		}
		frm_param->mfsr_param.frame_size = cxt->snp_cxt.request_size;

		crop_x = cxt->snp_cxt.zoom_param.zoom_info.crop_region;
		crop1 = cxt->snp_cxt.zoom_param_1x.zoom_info.crop_region;
		ratio = 1.0 * frm_param->mfsr_param.frame_size.width / crop1.width;
		x = (crop_x.start_x - crop1.start_x) * 1.0 * ratio;
		w = crop_x.width * ratio;

		ratio = 1.0 * frm_param->mfsr_param.frame_size.height / crop1.height;
		y = (crop_x.start_y - crop1.start_y) * 1.0 * ratio;
		h = crop_x.height * ratio;

		frame_crop.start_x = ((cmr_u32)(x + 0.5) & ~1);
		frame_crop.start_y = ((cmr_u32)(y + 0.5) & ~1);
		frame_crop.width = ((cmr_u32)(w + 0.5) & ~1);
		frame_crop.height = ((cmr_u32)(h + 0.5) & ~1);

		frm_param->mfsr_param.frame_crop = frame_crop;
		CMR_LOGD("frame size %d %d, crop (%d %d %d %d) 1x crop(%d %d %d %d), cur crop(%d %d %d %d) ratio %f\n",
			frm_param->mfsr_param.frame_size.width, frm_param->mfsr_param.frame_size.height,
			frm_param->mfsr_param.frame_crop.start_x, frm_param->mfsr_param.frame_crop.start_y,
			frm_param->mfsr_param.frame_crop.width, frm_param->mfsr_param.frame_crop.height,
			crop1.start_x, crop1.start_y, crop1.width, crop1.height,
			crop_x.start_x, crop_x.start_y, crop_x.width, crop_x.height, ratio);

		while ((frame_crop.width * 2) > frm_param->mfsr_param.frame_size.width) {
			frame_crop.width -= 2;
		}
		while ((frame_crop.width * 10) < frm_param->mfsr_param.frame_size.width) {
			frame_crop.width += 2;
		}

		while ((frame_crop.height * 2) > frm_param->mfsr_param.frame_size.height) {
			frame_crop.height -= 2;
		}
		while ((frame_crop.height * 10) < frm_param->mfsr_param.frame_size.height) {
			frame_crop.height += 2;
		}

		frm_param->mfsr_param.frame_crop = frame_crop;
		CMR_LOGD("frame size %d %d, crop (%d %d %d %d) 1x crop(%d %d %d %d), cur crop(%d %d %d %d) ratio %f\n",
			frm_param->mfsr_param.frame_size.width, frm_param->mfsr_param.frame_size.height,
			frm_param->mfsr_param.frame_crop.start_x, frm_param->mfsr_param.frame_crop.start_y,
			frm_param->mfsr_param.frame_crop.width, frm_param->mfsr_param.frame_crop.height,
			crop1.start_x, crop1.start_y, crop1.width, crop1.height,
			crop_x.start_x, crop_x.start_y, crop_x.width, crop_x.height, ratio);
	}

	return ret;
}

static cmr_int camera_mfsr_proc(struct camera_context *cxt, struct frm_info *frm)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct swa_frame_param *frm_param = NULL;

	if (cxt->snp_cxt.snap_cnt == 0) {
		frm_param = (struct swa_frame_param *)malloc(sizeof(struct swa_frame_param));
		if (frm_param == NULL) {
			CMR_LOGE("fail to malloc mem for frame params\n");
		} else {
			camera_ips_get_params(cxt, frm_param);
			frm->zsl_private = (cmr_uint)frm_param;
		}
	}
	CMR_LOGD("frame No.%d, fd=0x%x, vaddr 0x%lx, fid %d, param %p\n",
		cxt->snp_cxt.snap_cnt, frm->fd, (cmr_u32)frm->yaddr_vir,
		frm->frame_real_id, frm_param);

	cxt->snp_cxt.snap_cnt++;
	if (cxt->snp_cxt.snap_cnt >= cxt->snp_cxt.total_num) {
		cmr_u32 isp_scene = ISP_AUTO;
		struct img_rect crop_x;
		struct cmr_zoom zoom_factor;
		float aspect_ratio = 1.0 * cxt->snp_cxt.request_size.width / cxt->snp_cxt.request_size.height;

		ret = cmr_grab_stop_capture(cxt->grab_cxt.grab_handle);
		if (ret)
			CMR_LOGE("fail to stop capture\n");
		cxt->snp_cxt.start_capture_flag = 0;
		cxt->snp_cxt.snap_cnt = 0;
		CMR_LOGD("mfst total frm %d is enough, stop cap\n", cxt->snp_cxt.total_num);

		ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SCENE_MODE, &isp_scene);
		if (ret)
			CMR_LOGE("fail to isp set auto scene\n");

		cmr_preview_get_zoom_factor(cxt->prev_cxt.preview_handle,
					cxt->camera_id, &zoom_factor);
		crop_x.start_x = 0;
		crop_x.start_y = 0;
		crop_x.width = cxt->sn_cxt.cur_sn_size.width;
		crop_x.height = cxt->sn_cxt.cur_sn_size.height;
		ret = camera_get_trim_rect2(&crop_x, zoom_factor.zoom_setting.zoom_info.capture_aspect_ratio,
					aspect_ratio, crop_x.width, crop_x.height, 0);
		if (ret)
			CMR_LOGE("fail to get trim rect2\n");

		CMR_LOGD("restore crop for %f (%d %d %d %d)\n",
			zoom_factor.zoom_setting.zoom_info.capture_aspect_ratio,
			crop_x.start_x, crop_x.start_y, crop_x.width, crop_x.height);
		ret = cmr_grab_cfg_ch_crop(cxt->grab_cxt.grab_handle, cxt->snp_cxt.channel_id, &crop_x);
		if (ret)
			CMR_LOGE("fail to cmr_grab_cfg_ch_crop\n");
	}

	camera_invalidate_buf(cxt, frm->fd,
		frm->length, frm->yaddr, frm->yaddr_vir);
	ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
			SNAPSHOT_EVT_POSTPROC_START, frm);

	return ret;
}

void camera_send_channel_data(cmr_handle oem_handle, cmr_handle receiver_handle,
                              cmr_uint evt, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct frm_info *frm_ptr = data;
    cmr_u32 chn_bit;
    struct buffer_cfg buf_cfg;
    cmr_int need_pause;

    (cmr_handle)(receiver_handle);
    if (!frm_ptr) {
        CMR_LOGE("err, frame is null");
        goto exit;
    }

    if(camera_get_fdr_flag(cxt) && cxt->prev_cxt.preview_eb &&
        cxt->prev_cxt.preview_channel_id == frm_ptr->channel_id) {
        int fdr_total_frame_cnt = 0;
        int fdr_ref_frame_num = 0;
        camera_fdr_get_frame_cnt(cxt, &fdr_total_frame_cnt, &fdr_ref_frame_num);
    }

    camera_local_normal_snapshot_need_pause(oem_handle, &need_pause);
    chn_bit = 1 << frm_ptr->channel_id;
    CMR_LOGV("camera_id=%d,chn_id=%d,pre_chn_id=%d,video_chn_id=%d,snp_chn_id=%d,total=%d,zsl_frame %d",
        cxt->camera_id,frm_ptr->channel_id, cxt->prev_cxt.preview_channel_id, cxt->prev_cxt.video_channel_id,
        cxt->snp_cxt.channel_id, cxt->snp_cxt.total_num,
        cxt->snp_cxt.zsl_frame);

    CMR_LOGD("ch_id %d, frame_id %d, fd 0x%x, time %04d.%06ds, monotime 0x%llx\n",
		frm_ptr->channel_id, frm_ptr->frame_real_id, frm_ptr->fd,
		frm_ptr->sec, frm_ptr->usec, frm_ptr->monoboottime);

    if (cxt->prev_cxt.preview_eb &&
        cxt->prev_cxt.preview_channel_id == frm_ptr->channel_id) {
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
    }

    if (cxt->prev_cxt.video_eb &&
        cxt->prev_cxt.video_channel_id == frm_ptr->channel_id) {
        cmr_copy(&cxt->prev_cxt.video_cur_chn_data, frm_ptr,
                 sizeof(struct frm_info));
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
    }

    if (cxt->prev_cxt.channel0_eb &&
        cxt->prev_cxt.channel0_chn_id == frm_ptr->channel_id) {
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
    }

    if (cxt->prev_cxt.channel1_eb &&
        cxt->prev_cxt.channel1_chn_id == frm_ptr->channel_id) {
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
    }

    if (cxt->prev_cxt.channel2_eb &&
        cxt->prev_cxt.channel2_chn_id == frm_ptr->channel_id) {
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
    }

    if (cxt->prev_cxt.channel3_eb &&
        cxt->prev_cxt.channel3_chn_id == frm_ptr->channel_id) {
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
    }

    if (cxt->prev_cxt.channel4_eb &&
        cxt->prev_cxt.channel4_chn_id == frm_ptr->channel_id) {
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
    }

    if (cxt->prev_cxt.snapshot_eb &&
        cxt->snp_cxt.channel_id == frm_ptr->channel_id) {
         cxt->is_capture_face = true;
         if((1 == camera_get_fdr_flag(cxt)) && IS_FDR_FRAME(frm_ptr->frame_type)) {
            if (frm_ptr->frame_type != FDR_FRAME_RAW)
                 cxt->is_capture_face = false;
            ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                            SNAPSHOT_EVT_FDR_PROC, data);
             if (ret)
                 CMR_LOGE("failed to send fdr data");
        } else if (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt) &&
            CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
            ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                            SNAPSHOT_EVT_CHANNEL_DONE, data);
            if (ret) {
                CMR_LOGE("failed");
            }
            ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                           cxt->camera_id, evt, data);
            if (ret) {
                CMR_LOGE("failed");
            }
        } else if (cxt->snp_cxt.is_mfsr) {
            cmr_copy(&cxt->snp_cxt.cur_chn_data, frm_ptr, sizeof(struct frm_info));
            ret = camera_mfsr_proc(cxt, frm_ptr);

        } else {
            if (cxt->snp_cxt.zsl_frame) {
                cmr_copy(&cxt->snp_cxt.cur_chn_data, frm_ptr,
                         sizeof(struct frm_info));
                ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                               cxt->camera_id, evt, data);
                CMR_LOGV("camera id=%d, cur_chn_data.yaddr_vir 0x%x, "
                         "yaddr_vir 0x%x",
                         cxt->camera_id, cxt->snp_cxt.cur_chn_data.yaddr_vir,
                         frm_ptr->yaddr_vir);
            } else {
                ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                                SNAPSHOT_EVT_FREE_FRM, data);
            }
        }
    }

exit:
    if (ret) {
        CMR_LOGE("failed to send channel data %ld", ret);
    }
    ATRACE_END();
}

/*
 *privdata:oem handle
 */
cmr_int camera_sensor_streamctrl(cmr_u32 on_off, void *privdata) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;

    if (!cxt) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (cxt->is_multi_mode > 0) {
        ret = cmr_sensor_set_bypass_mode(cxt->sn_cxt.sensor_handle,
                                         cxt->camera_id, cxt->is_multi_mode);
        CMR_LOGV("cmr_sensor_set_bypass_mode %d", ret);
    }
    ret = cmr_sensor_stream_ctrl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                 on_off);
    if (ret) {
        CMR_LOGE("err to set stream %ld", ret);
    }
exit:
    CMR_LOGI("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_isp_ctrl_done(cmr_u32 cmd, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    (void *)data;
    if (cmd >= ISP_CTRL_MAX) {
        CMR_LOGE("isp wrong cmd %d", cmd);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGV("isp cmd, 0x%x, ret %ld", cmd, ret);
exit:
    return ret;
}

void camera_sensor_evt_cb(cmr_int evt, void *data, void *privdata) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;

    if (!cxt || !data || CMR_EVT_SENSOR_BASE != (CMR_EVT_SENSOR_BASE & evt)) {
        CMR_LOGE("error param, handle 0x%lx data 0x%lx evt 0x%lx",
                 (cmr_uint)cxt, (cmr_uint)data, evt);
        goto exit;
    }
    CMR_LOGI("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);
    switch (evt) {
    case CMR_SENSOR_FOCUS_MOVE:
        if (1 == cxt->focus_cxt.inited) {
            ret = cmr_focus_sensor_handle(cxt->focus_cxt.focus_handle,
                                          CMR_SENSOR_FOCUS_MOVE, cxt->camera_id,
                                          data);
        } else {
            CMR_LOGE("err, focus hasn't been initialized");
        }
        break;
    case CMR_SENSOR_ERROR:
        if (1 == cxt->prev_cxt.inited) {
            ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                           cxt->camera_id, evt, data);
        } else {
            CMR_LOGE("err, preview hasn't been initialized");
        }
        break;
    default:
        CMR_LOGE("can't handle ths evt, 0x%lx", evt);
    }
exit:
    if (ret) {
        CMR_LOGE("failed %ld", ret);
    }
}

cmr_int camera_is_need_change_fmt(cmr_handle oem_handle,
                                  struct frm_info *data_ptr) {
    cmr_int is_change_fmt = 0;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;
    cmr_uint is_snp_frm = 0;

    is_snp_frm = (data_ptr->channel_id == snp_cxt->channel_id);
    if (is_snp_frm) {
        if (CAM_IMG_FMT_JPEG == data_ptr->fmt ||
            CAM_IMG_FMT_BAYER_MIPI_RAW == data_ptr->fmt) {
            is_change_fmt = 1;
        }
    }
    return is_change_fmt;
}

cmr_int camera_get_cap_time(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)snp_handle;
    cmr_u32 sec = 0;
    cmr_u32 usec = 0;

    sem_wait(&cxt->access_sm);
    ret = cmr_grab_get_cap_time(cxt->grab_cxt.grab_handle, &sec, &usec);
    CMR_LOGD("cap time %d %d", sec, usec);
    cxt->snp_cxt.cap_time_stamp = sec * SEC_TO_NANOSEC + usec * MS_TO_NANOSEC;
    sem_post(&cxt->access_sm);
    return ret;
}

cmr_int camera_check_cap_time(cmr_handle snp_handle, struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)snp_handle;
    cmr_u64 frame_time =
        data->sec * SEC_TO_NANOSEC + data->usec * MS_TO_NANOSEC;

    CMR_LOGV("time %ld, %ld", data->sec, data->usec);
    sem_wait(&cxt->access_sm);
    if (TAKE_PICTURE_NEEDED == cxt->snp_cxt.is_req_snp &&
        (cxt->snp_cxt.channel_id == data->channel_id)) {
        if (frame_time <= cxt->snp_cxt.cap_time_stamp) {
            CMR_LOGW("frame is earlier than picture, drop!");
            ret = CMR_CAMERA_FAIL;
        } else {
            CMR_LOGV("frame time OK!");
        }
    }
    sem_post(&cxt->access_sm);
    return ret;
}

cmr_int camera_get_post_proc_chn_out_frm_id(struct img_frm *frame,
                                            struct frm_info *data) {
    cmr_int i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        if (frame[i].fd == data->fd)
            break;
    }
    CMR_LOGI("frm id %ld", i);
    return i;
}

void camera_grab_handle(cmr_int evt, void *data, void *privdata) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;
    struct ipm_context *ipm_cxt = &cxt->ipm_cxt;
    struct frm_info *frame = (struct frm_info *)data;
    cmr_u32 channel_id;
    cmr_handle receiver_handle;
    cmr_u32 chn_bits = (1 << frame->channel_id);
    cmr_u32 frm_id;
    struct buffer_cfg buf_cfg;

    if (cxt->snp_cxt.snp_mode != CAMERA_ZSL_MODE) {
        if (camera_get_is_discard_frame((cmr_handle)cxt, frame) ||
            camera_check_cap_time((cmr_handle)cxt, frame)) {
            memset(&buf_cfg, 0, sizeof(buf_cfg));
            buf_cfg.channel_id = frame->channel_id;
            buf_cfg.base_id = CMR_BASE_ID(frame->frame_id);
            buf_cfg.count = 1;
            buf_cfg.flag = BUF_FLAG_RUNNING;
            buf_cfg.addr[0].addr_y = frame->yaddr;
            buf_cfg.addr[0].addr_u = frame->uaddr;
            buf_cfg.addr[0].addr_v = frame->vaddr;
            buf_cfg.addr_vir[0].addr_y = frame->yaddr_vir;
            buf_cfg.addr_vir[0].addr_u = frame->uaddr_vir;
            buf_cfg.addr_vir[0].addr_v = frame->vaddr_vir;
            buf_cfg.fd[0] = frame->fd;
            camera_channel_buff_cfg(cxt, &buf_cfg);
            return;
        }
    }

    receiver_handle = cxt->grab_cxt.caller_handle[frame->channel_id];
    camera_send_channel_data((cmr_handle)cxt, receiver_handle, evt, data);
exit:
    ATRACE_END();
    return;
}

void camera_grab_evt_cb(cmr_int evt, void *data, void *privdata) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;
    struct frm_info *frame = (struct frm_info *)data;
    cmr_u32 channel_id;
    cmr_handle receiver_handle;

    if (!cxt || !data || !privdata ||
        CMR_EVT_GRAB_BASE != (CMR_EVT_GRAB_BASE & evt)) {
        CMR_LOGE("error param, handle 0x%lx data 0x%lx evt 0x%lx",
                 (cmr_uint)cxt, (cmr_uint)data, evt);
        return;
    }
    CMR_LOGV("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);

    channel_id = frame->channel_id;
    if (channel_id >= GRAB_CHANNEL_MAX) {
        CMR_LOGE("error param, channel id %d", channel_id);
        return;
    }

    switch (evt) {
    case CMR_GRAB_TX_DONE:
        if (frame->is_4in1_frame && frame->fmt != CAM_IMG_FMT_BAYER_MIPI_RAW) {
            camera_4in1_handle(evt, data, privdata);
        }
#if defined OEM_HANDLE_HDR || defined OEM_HANDLE_3DNR
        camera_grab_handle(evt, data, privdata);
#else
        camera_send_channel_data((cmr_handle)cxt, receiver_handle, evt, data);
#endif
        break;
    case CMR_GRAB_CANCELED_BUF:
    case CMR_GRAB_TX_ERROR:
    case CMR_GRAB_TX_NO_MEM:
    case CMR_GRAB_CSI2_ERR:
    case CMR_GRAB_TIME_OUT:
        ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, evt, data);
        if (ret) {
            CMR_LOGE("fail to handle error, ret %ld", ret);
        }
        break;
    default:
        CMR_LOGE("don't support evt 0x%lx", evt);
        break;
    }
}

cmr_uint camera_isp_af_param(cmr_uint cmd, cmr_uint cmd_value) {
    cmr_uint in_param = cmd_value;
    cmr_uint out_param = 0;

    switch (cmd) {
    case ISP_AF_NOTICE_CALLBACK: {
        switch (in_param) {
        case ISP_FOCUS_TRIG:
            out_param = CAMERA_FOCUS_MODE_AUTO;
            break;
        case ISP_FOCUS_MULTI_ZONE:
            out_param = CAMERA_FOCUS_MODE_AUTO_MULTI;
            break;
        case ISP_FOCUS_MACRO:
            out_param = CAMERA_FOCUS_MODE_MACRO;
            break;
        case ISP_FOCUS_NONE:
            out_param = CAMERA_FOCUS_MODE_INFINITY;
            break;
        case ISP_FOCUS_CONTINUE:
            out_param = CAMERA_FOCUS_MODE_CAF;
            break;
        case ISP_FOCUS_VIDEO:
            out_param = CAMERA_FOCUS_MODE_CAF_VIDEO;
            break;
        case ISP_FOCUS_MACRO_FIXED:
            out_param = CAMERA_FOCUS_MODE_MACRO_FIXED;
            break;
        case ISP_FOCUS_PICTURE:
            out_param = CAMERA_FOCUS_MODE_PICTURE;
            break;
        case ISP_FOCUS_FULLSCAN:
            out_param = CAMERA_FOCUS_MODE_FULLSCAN;
            break;
        default:
            break;
        }
    } break;
    default:
        break;
    }
    CMR_LOGV("[PFC] out_param %d X", out_param);
    return out_param;
}

void camera_isp_dev_evt_cb(cmr_int evt, void *data, cmr_u32 data_len,
                           void *privdata) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;
    struct frm_info *frame = (struct frm_info *)data;
    cmr_u32 channel_id;
    cmr_handle receiver_handle;
    cmr_int flash_status = FLASH_CLOSE;
    struct setting_cmd_parameter setting_param;
}

void camera_grab_post_ynr_evt_cb(cmr_int evt, void *data, void *privdata) {
    struct camera_context *cxt = NULL;

    if (!privdata) {
        CMR_LOGE("err oem post ynr param, privdata is %p", privdata);
        return;
    }
    cxt = (struct camera_context *)privdata;

    /*
        if (cxt->isp_cxt.isp_handle) {
            isp_ynr_post_proc(cxt->isp_cxt.isp_handle);
        }
    */
    return;
}

void camera_scaler_evt_cb(cmr_int evt, void *data, void *privdata) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;

    if (!cxt || !data || !privdata) {
        CMR_LOGE("err, scale callback param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        return;
    }
    CMR_LOGI("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);

    if (CMR_IMG_CVT_SC_DONE == evt) {
        camera_take_snapshot_step(CMR_STEP_SC_E);
        cmr_snapshot_receive_data((cmr_handle)privdata, SNAPSHOT_EVT_SCALE_DONE,
                                  data);
    } else {
        CMR_LOGE("err, don't support evt 0x%lx", evt);
    }
    ATRACE_END();
}

void camera_jpeg_evt_cb(enum jpg_jpeg_evt evt, void *data, void *privdata) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;
    cmr_u32 temp_evt;

    if (NULL == data || !privdata ||
        CMR_EVT_JPEG_BASE != (CMR_EVT_JPEG_BASE & evt)) {
        CMR_LOGE("err, param, 0x%lx 0x%lx", (cmr_uint)data, evt);
        return;
    }
    CMR_LOGD("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);

    switch (evt) {
    case CMR_JPEG_ENC_DONE:
        camera_take_snapshot_step(CMR_STEP_JPG_ENC_E);
        temp_evt = SNAPSHOT_EVT_JPEG_ENC_DONE;
        cxt->jpg_encode = JPEG_ENCODE_DONE;
        break;
    case CMR_JPEG_DEC_DONE:
        temp_evt = SNAPSHOT_EVT_JPEG_DEC_DONE;
        break;
    case CMR_JPEG_ENC_ERR:
        temp_evt = SNAPSHOT_EVT_JPEG_ENC_ERR;
        break;
    case CMR_JPEG_DEC_ERR:
        temp_evt = SNAPSHOT_EVT_JPEG_DEC_ERR;
        break;
    case CMR_JPEG_DEC_STOP:
        cxt->jpg_encode = JPEG_ENCODE_STOP;
        break;
    default:
        ret = -CMR_CAMERA_NO_SUPPORT;
        CMR_LOGE("err, don't support evt 0x%lx", evt);
    }
    if (ret) {
        CMR_LOGE("done %ld", ret);
        return;
    }
    if(evt != CMR_JPEG_DEC_STOP) {
        ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, temp_evt, data);
        if (ret) {
            CMR_LOGE("failed %ld", ret);
        }
    }
}
static cmr_int camera_isp_ctrl_flash(cmr_handle setting_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct setting_cmd_parameter setting_param;
    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));
    cmr_int flash_type = 0;
    if (NULL == data) {
        flash_type = FLASH_CLOSE;
        goto ctrl_flash;
    }

    switch (*(cmr_int *)data) {
    case 0:
        flash_type = FLASH_CLOSE;
        break;
    case 1:
        flash_type = FLASH_OPEN;
        break;
    case 2:
        flash_type = FLASH_HIGH_LIGHT;
        break;
    default:
        flash_type = FLASH_CLOSE;
        break;
    }

ctrl_flash:
    setting_param.cmd_type_value = flash_type;
    ret = cmr_setting_ioctl(setting_handle, CAMERA_PARAM_ISP_FLASH,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to open flash");
    }

    return ret;
}

cmr_int camera_convert_coor_sensor_to_capture(const struct img_rect *sn_trim,
        const struct img_size *capture_size, cmr_u32 *cropRegion) {

        float left = 0, top = 0, width = 0, height = 0, zoomWidth = 0, zoomHeight = 0;
        int ret = 0;
        int flag_square = 0;
        float sensorAspect = 0, captureAspect = 0;
        cmr_u32 roiW = 0, roiH = 0;
        cmr_u32 capture_width = 0, capture_height = 0;
        cmr_u32 sensor_width = 0, sensor_height = 0;

        roiW = cropRegion[2] - cropRegion[0];
        roiH = cropRegion[3] - cropRegion[1];
        if (roiW == 0 || roiH == 0) {
            CMR_LOGE("parameters error.");
            return 1;
        }
        if (roiW == roiH)
            flag_square = 1;

        capture_width = capture_size->width;
        capture_height = capture_size->height;
        sensor_width = sn_trim->width;
        sensor_height = sn_trim->height;
        sensorAspect = (float)sensor_width / sensor_height;
        captureAspect = (float)capture_width / capture_height;
        if (sensorAspect > captureAspect) {
            width = capture_width;
            height = captureAspect * capture_height / sensorAspect;
            left = 0;
            top = 0 + (capture_height - height) / 2;
        } else {
            width = sensorAspect * capture_width / captureAspect;
            height = capture_height;
            left = 0 + (capture_width - width) / 2;
            top = 0;
        }

        zoomWidth = width / (float)sensor_width;
        zoomHeight = height / (float)sensor_height;
        CMR_LOGD("before_crop_rect_calculated: (xs=%d,sy=%d,ex=%d,ey=%d)",
                 cropRegion[0], cropRegion[1], cropRegion[2], cropRegion[3]);
        cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth + left);
        cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight + top);
        cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth + left);
        cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight + top);
        //If it was originally square, it should be square after conversion
        roiW = cropRegion[2] - cropRegion[0];
        roiH = cropRegion[3] - cropRegion[1];
        if (flag_square == 1){
            if (roiW > roiH){
               cropRegion[2] = cropRegion[0] +roiH;
            } else if (roiH > roiW){
               cropRegion[3] = cropRegion[1]+ roiW;
            }
            flag_square = 0;
        }
        CMR_LOGD("after_crop_rect_calculated: (sx=%d,sy=%d,ex=%d,ey=%d)",
                 cropRegion[0], cropRegion[1], cropRegion[2], cropRegion[3]);
        return ret;
}

cmr_int camera_isp_evt_cb(cmr_handle oem_handle, cmr_u32 evt, void *data,
                          cmr_u32 data_len) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_u32 sub_type;
    cmr_u32 cmd = evt & 0xFF;
    cmr_int oem_cb;
    cmr_u32 *ae_info = NULL;
    struct cmr_focus_status focus_status;
    struct isp_af_notice *isp_af = NULL;
    cmr_u32 rect[4] = {0};
    struct img_size capture_size;
    struct img_rect sn_trim;

    struct isp_ae_adjust_param *ae_aux_info = NULL;
    if (!oem_handle || CMR_EVT_ISP_BASE != (CMR_EVT_ISP_BASE & evt)) {
        CMR_LOGE("err param, 0x%lx 0x%x 0x%lx", (cmr_uint)data, evt,
                 (cmr_uint)oem_handle);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGV("evt 0x%x, handle 0x%lx len %d", evt, (cmr_uint)oem_handle,
             data_len);

    sub_type = (~CMR_EVT_ISP_BASE) & evt;
    CMR_LOGV("sub_type %0x", sub_type);

    if ((sub_type & ISP_EVT_MASK) == 0) {
        ret = camera_isp_ctrl_done(cmd, data);
        goto exit;
    }

    switch (sub_type & ISP_EVT_MASK) {
    case ISP_PROC_CALLBACK:
        if (camera_is_need_change_fmt((cmr_handle)cxt, data)) {
            ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                            SNAPSHOT_EVT_CVT_RAW_DATA, data);
        } else {
            ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                            SNAPSHOT_EVT_RAW_PROC, data);
        }
        break;
    case ISP_AF_NOTICE_CALLBACK:
        CMR_LOGV("ISP_AF_NOTICE_CALLBACK come here");
        if (1 == cxt->focus_cxt.inited) {
            ret = cmr_focus_isp_handle(cxt->focus_cxt.focus_handle,
                                       FOCUS_EVT_ISP_AF_NOTICE, cxt->camera_id,
                                       data);
        }
        isp_af = (struct isp_af_notice *)data;
        
        /*get capture size*/
        capture_size.width = cxt->snp_cxt.request_size.width;
        capture_size.height = cxt->snp_cxt.request_size.height;

        /*get sensor size*/
        ret = camera_get_sensor_mode_trim(oem_handle, &sn_trim);
        if (ret) {
            CMR_LOGE("get_sensor info failed!");
            ret = CMR_CAMERA_FAIL;
        }

        rect[0] = isp_af->af_roi.sx;
        rect[1] = isp_af->af_roi.sy;
        rect[2] = isp_af->af_roi.ex;
        rect[3] = isp_af->af_roi.ey;
        ret = camera_convert_coor_sensor_to_capture(&sn_trim, &capture_size, rect);
        if (ret) {
            CMR_LOGE("convert coor sensor to capture failed!");
            ret = CMR_CAMERA_FAIL;
        }
        cxt->af_roi.sx = rect[0];
        cxt->af_roi.sy = rect[1];
        cxt->af_roi.ex = rect[2];
        cxt->af_roi.ey = rect[3];

        if (cxt->camera_cb && (isp_af->motor_pos != 0)) {
            oem_cb = CAMERA_EVT_CB_FOCUS_END;
            memset(&focus_status, 0, sizeof(struct cmr_focus_status));

            focus_status.af_motor_pos = isp_af->motor_pos;
            focus_status.af_mode =
                camera_isp_af_param(ISP_AF_NOTICE_CALLBACK, isp_af->af_mode);
            focus_status.af_state_result =
                af_state_focus_to_hal(isp_af->valid_win);
            if (!(focus_status.af_state_result)) {
                focus_status.af_motor_pos = FOCUS_FAIL;
            }
            CMR_LOGD("ISP_AF_NOTICE_CALLBACK isp_af->af_mode %d "
                     "focus_status.af_mode %d af_state_result %d",
                     isp_af->af_mode, focus_status.af_mode,
                     focus_status.af_state_result);
            cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_START_FOCUS,
                           (void *)&focus_status);
        } else {
            CMR_LOGD("cxt->camera_cb warning");
        }
        break;
    /*	case ISP_FLASH_AE_CALLBACK:
                    ret = cmr_setting_isp_alg_done(oem_handle, data);
                    break;
            case ISP_AE_BAPASS_CALLBACK:
                    ret = cmr_setting_isp_alg_done(oem_handle, data);
                    break;
            case ISP_AF_STAT_CALLBACK:
                    ret = camera_isp_af_stat(data);
                    break;*/
    case ISP_AE_STAB_CALLBACK:
        ret =
            cmr_setting_isp_notice_done(cxt->setting_cxt.setting_handle, data);
        break;
    case ISP_QUICK_MODE_DOWN:
        cmr_setting_quick_ae_notice_done(cxt->setting_cxt.setting_handle, data);
        break;
    case ISP_ONLINE_FLASH_CALLBACK:
        camera_isp_ctrl_flash(cxt->setting_cxt.setting_handle, data);
        break;
    case ISP_AE_STAB_NOTIFY:
        oem_cb = CAMERA_EVT_CB_AE_STAB_NOTIFY;
        if (data != NULL) {
            ae_info = (cmr_u32 *)data;
            cxt->long_expo_enable = ae_info[AE_CB_LONG_EXP_INDEX];
            CMR_LOGD("cxt->long_expo_enable %d", cxt->long_expo_enable);
            cxt->camera_cb(oem_cb, cxt->client_data,
                           CAMERA_FUNC_AE_STATE_CALLBACK, ae_info);
            cmr_preview_facedetect_set_ae_stab(cxt->prev_cxt.preview_handle,
                                               cxt->camera_id, ae_info);
        } else
            cxt->camera_cb(oem_cb, cxt->client_data,
                           CAMERA_FUNC_AE_STATE_CALLBACK, NULL);
        break;
    case ISP_AE_LOCK_NOTIFY:
        oem_cb = CAMERA_EVT_CB_AE_LOCK_NOTIFY;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       NULL);
        break;
    case ISP_AE_UNLOCK_NOTIFY:
        oem_cb = CAMERA_EVT_CB_AE_UNLOCK_NOTIFY;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       NULL);
        break;
    case ISP_AE_SYNC_INFO: {
        struct ispae_sync_info_output *info_output =
            (struct ispae_sync_info_output *)data;
        // TBD write info to al3200
    }
    case ISP_AE_EXP_TIME:
        CMR_LOGV("ISP_AE_EXP_TIME,data %" PRId64, *(uint64_t *)data);
        prev_set_ae_time(cxt->prev_cxt.preview_handle, cxt->camera_id, data);
        break;
    /*    case ISP_VCM_STEP:
            CMR_LOGI("ISP_VCM_STEP,data %d", *(uint32_t *)data);
            prev_set_vcm_step(cxt->prev_cxt.preview_handle, cxt->camera_id,
       data);
            break;*/
    case ISP_EV_EFFECT_CALLBACK:
        CMR_LOGD("under exposure scene=%d", *(cmr_u8 *)data);
        cxt->skipframe = *(cmr_u8 *)data;
        if (cxt->gtm_flag) {
            *(cmr_u8 *)data = 1;
            cxt->skipframe = 1;
        }
        if (camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_NIGHT_DNS) {
            *(cmr_u8 *)data = 1;
            cxt->skipframe = 1;
        }

        cmr_setting_isp_notice_done(cxt->setting_cxt.setting_handle, data);
        oem_cb = CAMERA_EVT_CB_EV_ADJUST_SCENE;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        break;
    case ISP_AE_AUX_EFFECT_CALLBACK: //exp,iso,gain etc
            ae_aux_info = (struct isp_ae_adjust_param *)data;
            cmr_u32 cnt = cxt->ae_aux_info.cnt;
            CMR_LOGI("cnt %d, ev %1.3f", cnt, ae_aux_info->ev);
            if (cnt < NIGHTDNS_CAP_NUM) {
            cxt->ae_aux_info.param[cnt].exp_time = ae_aux_info->exp_time;
            cxt->ae_aux_info.param[cnt].total_gain = ae_aux_info->total_gain;
            cxt->ae_aux_info.param[cnt].isp_gain = ae_aux_info->isp_gain;
            cxt->ae_aux_info.param[cnt].iso = ae_aux_info->iso;
            cxt->ae_aux_info.param[cnt].exposure = ae_aux_info->exposure;
            cxt->ae_aux_info.param[cnt].ev = ae_aux_info->ev;
            cxt->ae_aux_info.cnt++;
            CMR_LOGI("cnt %d exp_time total_gain iso isp_gain exposure ev(%d %d %d %d %d %f)",cnt,
                cxt->ae_aux_info.param[cnt].exp_time,
                cxt->ae_aux_info.param[cnt].total_gain,
                cxt->ae_aux_info.param[cnt].iso,
                cxt->ae_aux_info.param[cnt].isp_gain,
                cxt->ae_aux_info.param[cnt].exposure,
                cxt->ae_aux_info.param[cnt].ev);
            }
        break;
    case ISP_FDR_EV_EFFECT_CALLBACK:
        cmr_setting_isp_notice_done(cxt->setting_cxt.setting_handle, data);
        cxt->snp_cxt.fdr_ae_info = data;
        CMR_LOGD("notice to capture, fdr_ae_info addr = %p", cxt->snp_cxt.fdr_ae_info);
        break;
    case ISP_HDR_EV_EFFECT_CALLBACK:
        camera_set_hdr_ev(oem_handle, data);
        cmr_setting_isp_notice_done(cxt->setting_cxt.setting_handle, data);
        break;
    case ISP_AE_CB_FLASH_FIRED:
        oem_cb = CAMERA_EVT_CB_AE_FLASH_FIRED;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        break;
    case ISP_AUTO_HDR_STATUS_CALLBACK:
        oem_cb = CAMERA_EVT_CB_HDR_SCENE;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        break;
    case ISP_AUTO_FDR_STATUS_CALLBACK:
        oem_cb = CAMERA_EVT_CB_FDR_SCENE;
        CMR_LOGD("auto fdr is_fdr_scene:%d", *(cmr_u8 *)data);
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        break;
    case ISP_AE_SYNC_STATUS_CALLBACK:
        oem_cb = CAMERA_EVT_CB_AE_SYNC;
        CMR_LOGD("ae sync stable:%d", *(cmr_u8 *)data);
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        break;
    case ISP_LONGEXP_SKIPNUM_CALLBACK:
        oem_cb = CAMERA_EVT_CB_LONGEXP_SKIPNUM;
#if 0
        CMR_LOGD("long exp skip num:%d", *(cmr_u8 *)data);
        cxt->longexp_skipnum = *(cmr_u8 *)data;
#endif
        break;
    case ISP_AI_SCENE_TYPE_CALLBACK:
        oem_cb = CAMERA_EVT_CB_AI_SCENE;
        CMR_LOGV("isp ai scene type:%u", *(cmr_u8 *)data);
        cmr_u8 hal_scene_type = camera_hal_ai_scene_type(*(cmr_uint *)data);
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       &hal_scene_type);
        break;
    case ISP_AF_VCM_NOTICE_CALLBACK:
        oem_cb = CAMERA_EVT_CB_VCM_RESULT;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        break;
    case ISP_HIST_REPORT_CALLBACK:
        oem_cb = CAMERA_EVT_CB_HIST_REPORT;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        cmr_preview_facedetect_set_hist(cxt->prev_cxt.preview_handle,
                                        cxt->camera_id, data);
        break;
    case ISP_3DNR_CALLBACK:
        oem_cb = CAMERA_EVT_CB_3DNR_SCENE;
        CMR_LOGD("3dnr scene=%d", *(cmr_u8 *)data);
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_STATE_CALLBACK,
                       data);
        break;
    case ISP_AE_PARAM_CALLBACK:
        oem_cb = CAMERA_EVT_CB_AE_PARAMS;
        cxt->snp_cxt.ae_common_info = data;
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AE_PARAMS_CALLBACK,
                       data);
        {
          cmr_int oem_cb = 0xffff;
          unsigned short  af = 0;
          camera_get_af_support(oem_handle,&af);
          CMR_LOGD("af sup %d, af_bypass:%d", af, cxt->isp_cxt.is_af_bypass);
          if (!af || cxt->isp_cxt.is_af_bypass) {
              struct af_callback_params tmp_af_callback;
              memset (&tmp_af_callback, 0, sizeof(struct af_callback_params));
              tmp_af_callback.frame_number = -1;
              oem_cb = CAMERA_EVT_CB_AF_PARAMS;
              cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AF_PARAMS_CALLBACK,
                         &tmp_af_callback);
          }
        }
        CMR_LOGV("ae common info cb to oem ,addr = %p", cxt->snp_cxt.ae_common_info);
        break;
    case ISP_AF_PARAMS_CALLBACK:
       oem_cb = CAMERA_EVT_CB_AF_PARAMS;
       cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_AF_PARAMS_CALLBACK,
                      data);
       break;
    case ISP_AE_CB_HDR_EXP_GAIN: {
        struct ae_hdr_exp_gain_info info;
        cxt->snp_cxt.ae_exp_gain_info = data;
        info = *(struct ae_hdr_exp_gain_info *)data;
        CMR_LOGD("exp[%d %d %d], gain[%d %d %d]", info.exp_time[0],
                 info.exp_time[1], info.exp_time[2], info.total_gain[0],
                 info.total_gain[1], info.total_gain[2]);
        // todo
    }
    break;
    case ISP_AE_CB_HDR_TUNING_PARAM_INDEX: {
        // got index(cmr_u32)
        void *hdr_callback = data;
        CMR_LOGD("hdr_callback %p", hdr_callback);
        cxt->snp_cxt.hdr_callback = hdr_callback;
        //todo
    }
    break;
    default:
        break;
    }
exit:
    return ret;
}

int af_state_focus_to_hal(cmr_u32 valid_win) {
    int af_state_result = AF_STATE_FOCUSE_MAX;

    switch (valid_win) {
    case 0:
        af_state_result = AF_STATE_NOT_FOCUSED_LOCKED;
        break;
    case 1:
        af_state_result = AF_STATE_FOCUSED_LOCKED;
        break;
    default:
        CMR_LOGD("unknown value");
    }
    return af_state_result;
}

void camera_focus_evt_cb(enum af_cb_type cb, cmr_uint param, void *privdata) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;
    cmr_int oem_cb;
    struct cmr_focus_status *focus_status;

    if (!privdata) {
        CMR_LOGE("err, handle for callback");
        return;
    }
    switch (cb) {
    case AF_CB_DONE:
        oem_cb = CAMERA_EXIT_CB_DONE;
        cxt->is_focus = param;
        if (param) {
            return;
        }
        break;
    case AF_CB_FAILED:
        oem_cb = CAMERA_EXIT_CB_FAILED;
        cxt->is_focus = 0;
        break;
    case AF_CB_ABORT:
        oem_cb = CAMERA_EXIT_CB_ABORT;
        break;
    case AF_CB_FOCUS_MOVE:
        oem_cb = CAMERA_EVT_CB_FOCUS_MOVE;
        focus_status = (struct cmr_focus_status *)param;
        cxt->is_focus = focus_status->is_in_focus;
        if (cxt->is_focus) {
            cxt->focus_rect.x = 0;
            cxt->focus_rect.y = 0;
        }
        break;
    case AF_CB_ROI:
        oem_cb = CAMERA_EXIT_CB_DONE;
        break;
    default:
        CMR_LOGE("failed focus cb %d", cb);
        ret = -CMR_CAMERA_NO_SUPPORT;
    }
    if (ret) {
        CMR_LOGE("done %ld", ret);
        return;
    }
    CMR_LOGV("param =0x%lx camera_cb 0x%lx focus cb %ld, oem cb 0x%lx",
             (cmr_uint)param, (cmr_uint)cb, (cmr_uint)oem_cb,
             (cmr_uint)cxt->camera_cb);
    if (cxt->camera_cb) {
        CMR_LOGV("cxt->camera_cb run");
        cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_START_FOCUS,
                       (void *)param);
    } else {
        CMR_LOGD("cxt->camera_cb null error");
    }
}

static void camera_facealign_conversion(cmr_handle oem_handle,
                                struct camera_frame_type *frame_param,
                                struct isp_face_area *face_area,
                                struct sprd_img_path_rect *sn_trim) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    cmr_s32 i = 0;
    for (i = 0; i < 1; i++) {
        float left = 0, top = 0, width = 0, height = 0, zoomWidth = 0,
              zoomHeight = 0;
        struct sprd_img_rect scalerCrop;

        scalerCrop.x = sn_trim->trim_valid_rect.x;
        scalerCrop.y = sn_trim->trim_valid_rect.y;
        scalerCrop.w = sn_trim->trim_valid_rect.w;
        scalerCrop.h = sn_trim->trim_valid_rect.h;

        /* for crop region center not at sensor center */
        struct setting_cmd_parameter setting_param;
        int ret = 0, tag;

        cmr_bzero(&setting_param, sizeof(setting_param));
        setting_param.camera_id = cxt->camera_id;
        tag = cxt->is_ultra_wide ? SETTING_GET_REPROCESS_ZOOM_RATIO
                                 : SETTING_GET_ZOOM_PARAM;
        ret = cmr_setting_ioctl(setting_cxt->setting_handle, tag,
                                &setting_param);
        if (ret) {
            CMR_LOGW("failed to get zoom param %ld", ret);
        } else {
            struct zoom_info *info = &setting_param.zoom_param.zoom_info;
            struct img_rect *rect = &info->crop_region;

            /* hal_param is bzero-ed on init, this check should be enough...
             */
            if (rect->start_x || rect->start_y || rect->width ||
                rect->height) {
                struct img_rect src, dst;

                src.start_x = 0;
                src.start_y = 0;
                src.width = face_area->frame_width;
                src.height = face_area->frame_height;

                dst = camera_apply_rect_and_ratio(
                    info->pixel_size, info->crop_region, src,
                    (float)src.width / (float)src.height);

                CMR_LOGV("fix rect from %u %u %u %u to %u %u %u %u",
                         scalerCrop.x, scalerCrop.y, scalerCrop.w,
                         scalerCrop.h, dst.start_x, dst.start_y, dst.width,
                         dst.height);

                scalerCrop.x = dst.start_x;
                scalerCrop.y = dst.start_y;
                scalerCrop.w = dst.width;
                scalerCrop.h = dst.height;
            }
        }

        float previewAspect = (float)frame_param->width / frame_param->height;
        float cropAspect = (float)scalerCrop.w / scalerCrop.h;
        if (previewAspect > cropAspect) {
            width = scalerCrop.w;
            height = scalerCrop.w / previewAspect;
            left = scalerCrop.x;
            top = scalerCrop.y + (scalerCrop.h - height) / 2;
        } else {
            width = previewAspect * scalerCrop.h;
            height = scalerCrop.h;
            left = scalerCrop.x + (scalerCrop.w - width) / 2;
            top = scalerCrop.y;
        }
        zoomWidth = width / (float)frame_param->width;
        zoomHeight = height / (float)frame_param->height;

        CMR_LOGV("zoomWidth %f zoomHeight %f  left %f  top %f", zoomWidth, zoomHeight, left, top);
        CMR_LOGV("frame_param->width x height: %f x %f",(float)frame_param->width, (float)frame_param->height);
        CMR_LOGV("scalerCrop.w x h: %f x %f",(float)scalerCrop.w, (float)scalerCrop.h);

        face_area->face_info[i].fascore = frame_param->face_info[i].fascore;
        for(int j = 0; j < FA_SHAPE_POINTNUM * 2; j += 2) {
            face_area->face_info[i].data[j] = (int)((float)(frame_param->face_info[i].data[j]) * zoomWidth + left);
        }
        for(int j = 1; j < FA_SHAPE_POINTNUM * 2; j += 2) {
            face_area->face_info[i].data[j] = (int)((float)(frame_param->face_info[i].data[j]) * zoomHeight + top);
        }

        for(int j=0; j < FA_SHAPE_POINTNUM * 2; j++) {
            CMR_LOGD("toispface%d: fa_shape.data point %d = %d", i, j, face_area->face_info[i].data[j]);
        }
        CMR_LOGD("toispface%d: fa_shape.fascore = %d", i, face_area->face_info[i].fascore);
    }
}

static void camera_cfg_face_roi(cmr_handle oem_handle,
                                struct camera_frame_type *frame_param,
                                struct isp_face_area *face_area,
                                struct sprd_img_path_rect *sn_trim) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    cmr_s32 sx = 0;
    cmr_s32 sy = 0;
    cmr_s32 ex = 0;
    cmr_s32 ey = 0;
    cmr_s32 fd_sx =0;
    cmr_s32 fd_sy =0;
    cmr_s32 fd_ex =0;
    cmr_s32 fd_ey = 0;
    cmr_s32 i = 0;
    cmr_uint face_info_max_num =
        sizeof(face_area->face_info) / sizeof(struct isp_face_info);

    if (face_info_max_num < face_area->face_num)
        face_area->face_num = face_info_max_num;

    for (i = 0; i < face_area->face_num; i++) {
        sx = MIN(
            MIN(frame_param->face_info[i].sx, frame_param->face_info[i].srx),
            MIN(frame_param->face_info[i].ex, frame_param->face_info[i].elx));
        sy = MIN(
            MIN(frame_param->face_info[i].sy, frame_param->face_info[i].sry),
            MIN(frame_param->face_info[i].ey, frame_param->face_info[i].ely));
        ex = MAX(
            MAX(frame_param->face_info[i].sx, frame_param->face_info[i].srx),
            MAX(frame_param->face_info[i].ex, frame_param->face_info[i].elx));
        ey = MAX(
            MAX(frame_param->face_info[i].sy, frame_param->face_info[i].sry),
            MAX(frame_param->face_info[i].ey, frame_param->face_info[i].ely));
        // save face info in cmr cxt for other case.such as face beauty
        // takepicture
        cxt->fd_face_area.face_info[i].sx = sx;
        cxt->fd_face_area.face_info[i].sy = sy;
        cxt->fd_face_area.face_info[i].ex = ex;
        cxt->fd_face_area.face_info[i].ey = ey;
        //fd out
        fd_sx = MIN(
            MIN(frame_param->face_info[i].fd_cb_ptr.sx, frame_param->face_info[i].fd_cb_ptr.srx),
            MIN(frame_param->face_info[i].fd_cb_ptr.ex, frame_param->face_info[i].fd_cb_ptr.elx));
        fd_sy = MIN(
            MIN(frame_param->face_info[i].fd_cb_ptr.sy, frame_param->face_info[i].fd_cb_ptr.sry),
            MIN(frame_param->face_info[i].fd_cb_ptr.ey, frame_param->face_info[i].fd_cb_ptr.ely));
        fd_ex = MAX(
            MAX(frame_param->face_info[i].fd_cb_ptr.sx, frame_param->face_info[i].fd_cb_ptr.srx),
            MAX(frame_param->face_info[i].fd_cb_ptr.ex, frame_param->face_info[i].fd_cb_ptr.elx));
        fd_ey = MAX(
            MAX(frame_param->face_info[i].fd_cb_ptr.sy, frame_param->face_info[i].fd_cb_ptr.sry),
            MAX(frame_param->face_info[i].fd_cb_ptr.ey, frame_param->face_info[i].fd_cb_ptr.ely));
        CMR_LOGV("mPreviewWidth = %d, mPreviewHeight = %d, crop %d %d %d %d fd %d %d %d %d",
                 frame_param->width, frame_param->height, sx, sy, ex, ey, fd_sx, fd_sy, fd_ex, fd_ey);

#ifdef CONFIG_CAMERA_FACE_ROI
        float left = 0, top = 0, width = 0, height = 0, zoomWidth = 0,
              zoomHeight = 0;
        struct sprd_img_rect scalerCrop;
        cmr_bzero(&scalerCrop, sizeof(scalerCrop));
        CMR_LOGV("mPreviewWidth = %d, mPreviewHeight = %d, crop %d %d %d %d",
                 frame_param->width, frame_param->height, sx, sy, ex, ey);
        {
            /* for crop region center not at sensor center */
            struct setting_cmd_parameter setting_param;
            int ret = 0, tag;

            cmr_bzero(&setting_param, sizeof(setting_param));
            setting_param.camera_id = cxt->camera_id;
            tag = cxt->is_ultra_wide ? SETTING_GET_REPROCESS_ZOOM_RATIO
                                     : SETTING_GET_ZOOM_PARAM;
            ret = cmr_setting_ioctl(setting_cxt->setting_handle, tag,
                                    &setting_param);
            if (ret) {
                CMR_LOGW("failed to get zoom param %ld", ret);
            } else {
                struct zoom_info *info = &setting_param.zoom_param.zoom_info;
                struct img_rect *rect = &info->crop_region;

                /* hal_param is bzero-ed on init, this check should be enough...
                 */
                if (rect->start_x || rect->start_y || rect->width ||
                    rect->height) {
                    struct img_rect src, dst;

                    src.start_x = 0;
                    src.start_y = 0;
                    src.width = face_area->frame_width;
                    src.height = face_area->frame_height;

                    if (src.height != 0) {
                        dst = camera_apply_rect_and_ratio(
                        info->pixel_size, info->crop_region, src,
                        (float)src.width / (float)src.height);
                    }

                    CMR_LOGV("fix rect from %u %u %u %u to %u %u %u %u",
                             scalerCrop.x, scalerCrop.y, scalerCrop.w,
                             scalerCrop.h, dst.start_x, dst.start_y, dst.width,
                             dst.height);

                    scalerCrop.x = dst.start_x;
                    scalerCrop.y = dst.start_y;
                    scalerCrop.w = dst.width;
                    scalerCrop.h = dst.height;
                }
            }
        }
        CMR_LOGD("scalerCrop x =%d ,y =%d, w = %d , h =%d",scalerCrop.x, scalerCrop.y,scalerCrop.w,scalerCrop.h );

        if (scalerCrop.x != 0 && scalerCrop.h != 0) {
            float previewAspect = (float)frame_param->width / frame_param->height;
            float cropAspect = (float)scalerCrop.w / scalerCrop.h;
            if (previewAspect > cropAspect) {
                width = scalerCrop.w;
                height = cropAspect * scalerCrop.h / previewAspect;
                left = scalerCrop.x;
                top = scalerCrop.y+ (scalerCrop.h - height) / 2;
           } else {
                width = previewAspect * scalerCrop.w / cropAspect;
                height = scalerCrop.h;
                left = scalerCrop.x+ (scalerCrop.w - width) / 2;
                top = scalerCrop.y;
            }
            zoomWidth = width / (float)frame_param->width;
            zoomHeight = height / (float)frame_param->height;
            face_area->face_info[i].sx = (cmr_u32)((float)fd_sx * zoomWidth + left);
            face_area->face_info[i].sy = (cmr_u32)((float)fd_sy * zoomHeight + top);
            face_area->face_info[i].ex = (cmr_u32)((float)fd_ex * zoomWidth + left);
            face_area->face_info[i].ey = (cmr_u32)((float)fd_ey * zoomHeight + top);
            CMR_LOGD("zoomX Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)", face_area->face_info[i].sx,
                     face_area->face_info[i].sy, face_area->face_info[i].ex, face_area->face_info[i].ey);
        } else {
            face_area->face_info[i].sx = 1.0 * fd_sx * (float)face_area->frame_width /
                                     (float)frame_param->width;
            face_area->face_info[i].sy = 1.0 * fd_sy * (float)face_area->frame_height /
                                     (float)frame_param->height;
            face_area->face_info[i].ex = 1.0 * fd_ex * (float)face_area->frame_width /
                                     (float)frame_param->width;
            face_area->face_info[i].ey = 1.0 * fd_ey * (float)face_area->frame_height /
                                     (float)frame_param->height;
            CMR_LOGD("1.0X Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)", face_area->face_info[i].sx,
                face_area->face_info[i].sy, face_area->face_info[i].ex, face_area->face_info[i].ey);
    }
#else
        float left = 0, top = 0, width = 0, height = 0, zoomWidth = 0,
              zoomHeight = 0;
        struct sprd_img_rect scalerCrop;
        CMR_LOGV("mPreviewWidth = %d, mPreviewHeight = %d, crop %d %d %d %d",
                 frame_param->width, frame_param->height, sx, sy, ex, ey);

        scalerCrop.x = sn_trim->trim_valid_rect.x;
        scalerCrop.y = sn_trim->trim_valid_rect.y;
        scalerCrop.w = sn_trim->trim_valid_rect.w;
        scalerCrop.h = sn_trim->trim_valid_rect.h;

        {
            /* for crop region center not at sensor center */
            struct setting_cmd_parameter setting_param;
            int ret = 0, tag;

            cmr_bzero(&setting_param, sizeof(setting_param));
            setting_param.camera_id = cxt->camera_id;
            tag = cxt->is_ultra_wide ? SETTING_GET_REPROCESS_ZOOM_RATIO
                                     : SETTING_GET_ZOOM_PARAM;
            ret = cmr_setting_ioctl(setting_cxt->setting_handle, tag,
                                    &setting_param);
            if (ret) {
                CMR_LOGW("failed to get zoom param %ld", ret);
            } else {
                struct zoom_info *info = &setting_param.zoom_param.zoom_info;
                struct img_rect *rect = &info->crop_region;

                /* hal_param is bzero-ed on init, this check should be enough...
                 */
                if (rect->start_x || rect->start_y || rect->width ||
                    rect->height) {
                    struct img_rect src, dst;

                    src.start_x = 0;
                    src.start_y = 0;
                    src.width = face_area->frame_width;
                    src.height = face_area->frame_height;

                    dst = camera_apply_rect_and_ratio(
                        info->pixel_size, info->crop_region, src,
                        (float)src.width / (float)src.height);

                    CMR_LOGV("fix rect from %u %u %u %u to %u %u %u %u",
                             scalerCrop.x, scalerCrop.y, scalerCrop.w,
                             scalerCrop.h, dst.start_x, dst.start_y, dst.width,
                             dst.height);

                    scalerCrop.x = dst.start_x;
                    scalerCrop.y = dst.start_y;
                    scalerCrop.w = dst.width;
                    scalerCrop.h = dst.height;
                }
            }
        }

        float previewAspect = (float)frame_param->width / frame_param->height;
        float cropAspect = (float)scalerCrop.w / scalerCrop.h;
        if (previewAspect > cropAspect) {
            width = scalerCrop.w;
            height = scalerCrop.w / previewAspect;
            left = scalerCrop.x;
            top = scalerCrop.y + (scalerCrop.h - height) / 2;
        } else {
            width = previewAspect * scalerCrop.h;
            height = scalerCrop.h;
            left = scalerCrop.x + (scalerCrop.w - width) / 2;
            top = scalerCrop.y;
        }
        zoomWidth = width / (float)frame_param->width;
        zoomHeight = height / (float)frame_param->height;
        face_area->face_info[i].sx = (cmr_s32)((float)fd_sx * zoomWidth + left);
        face_area->face_info[i].sy = (cmr_s32)((float)fd_sy * zoomHeight + top);
        face_area->face_info[i].ex = (cmr_s32)((float)fd_ex * zoomWidth + left);
        face_area->face_info[i].ey = (cmr_s32)((float)fd_ey * zoomHeight + top);
#endif
        cxt->fd_face_area.face_info[i].angle = frame_param->face_info[i].angle;
        cxt->fd_face_area.face_info[i].pose = frame_param->face_info[i].pose;
        face_area->face_info[i].brightness =
            frame_param->face_info[i].brightness;
        face_area->face_info[i].angle = frame_param->face_info[i].angle;
        face_area->face_info[i].pose = frame_param->face_info[i].pose;
        face_area->face_info[i].yaw_angle = frame_param->face_info[i].pose;
        face_area->face_info[i].roll_angle = frame_param->face_info[i].angle;
        face_area->face_info[i].score = frame_param->face_info[i].score;
        face_area->face_info[i].id = frame_param->face_info[i].face_id;
        CMR_LOGV("id %u, s %d %d, e %d %d, center (%d, %d)", cxt->camera_id,
                 face_area->face_info[i].sx, face_area->face_info[i].sy,
                 face_area->face_info[i].ex, face_area->face_info[i].ey,
                 (face_area->face_info[i].sx + face_area->face_info[i].ex) / 2,
                 (face_area->face_info[i].sy + face_area->face_info[i].ey) / 2);
    }
    if (cxt->is_capture_face == true) {
        cmr_copy(&cxt->fd_face_area_capture, &cxt->fd_face_area,
            sizeof(struct isp_face_area));
        cxt->is_capture_face = false;
    }

}

cmr_int camera_preview_cb(cmr_handle oem_handle, enum preview_cb_type cb_type,
                          enum preview_func_type func, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint oem_func;
    cmr_uint oem_cb_type;
    struct setting_cmd_parameter setting_param;
    CHECK_HANDLE_VALID(oem_handle);
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    cmr_handle send_thr_handle = cxt->prev_cb_thr_handle;
    CMR_MSG_INIT(message);
    if (PREVIEW_FUNC_START_PREVIEW == func) {
        oem_func = CAMERA_FUNC_START_PREVIEW;
    } else if (PREVIEW_FUNC_STOP_PREVIEW == func) {
        oem_func = CAMERA_FUNC_STOP_PREVIEW;
    } else if (PREVIEW_FUNC_START_CAPTURE == func) {
        oem_func = CAMERA_FUNC_TAKE_PICTURE;
    } else {
        CMR_LOGE("err, %d", func);
        goto exit;
    }
    CMR_LOGV("%ld %d", oem_func, cb_type);
    switch (cb_type) {
    case PREVIEW_RSP_CB_SUCCESS:
        oem_cb_type = CAMERA_RSP_CB_SUCCESS;
        break;
    case PREVIEW_EVT_CB_FRAME:
        oem_cb_type = CAMERA_EVT_CB_FRAME;
        break;
    case PREVIEW_EVT_CB_RAW_FRAME:
        oem_cb_type = CAMERA_EVT_CB_RAW_FRAME;
        break;
    case PREVIEW_EXIT_CB_FAILED:
        oem_cb_type = CAMERA_EXIT_CB_FAILED;
        break;
    case PREVIEW_EVT_CB_FD:
        oem_cb_type = CAMERA_EVT_CB_FD;
        if (param) {
            struct camera_frame_type *frame_param =
                (struct camera_frame_type *)param;
            struct isp_face_area face_area;//for isp fd info
            int32_t sx = 0;
            int32_t sy = 0;
            int32_t ex = 0;
            int32_t ey = 0;
            int32_t i = 0;
            struct img_rect src_prev_rect;
            struct sensor_mode_info *sensor_mode_info = NULL;
            cmr_u32 sn_mode = 0;
            struct sprd_img_path_rect sn_trim;
            bzero(&sn_trim, sizeof(struct sprd_img_path_rect));
#ifdef CONFIG_ISP_2_3
            struct img_rect af_trim = {0, 0, 0, 0};
#endif
            cmr_uint face_info_max_num =
                sizeof(face_area.face_info) / sizeof(struct isp_face_info);

            cxt->fd_face_area.frame_width = frame_param->width;
            cxt->fd_face_area.frame_height = frame_param->height;
            cxt->fd_face_area.face_num = frame_param->face_num;

            cmr_bzero(&face_area, sizeof(struct isp_face_area));
            // note:now we get the preview face crop.but ISP need sensor's
            // crop.so we need recovery crop.
            cmr_preview_get_prev_rect(cxt->prev_cxt.preview_handle,
                                      cxt->camera_id, &src_prev_rect);

            cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                &sn_mode);
#ifndef CONFIG_CAMERA_FACE_ROI
            cmr_grab_get_dcam_path_trim(cxt->grab_cxt.grab_handle, &sn_trim);
#endif
            sensor_mode_info = &cxt->sn_cxt.sensor_info.mode_info[sn_mode];
            face_area.frame_width = sensor_mode_info->trim_width;
            face_area.frame_height = sensor_mode_info->trim_height;
            face_area.face_num = frame_param->face_num;

            face_area.frame_id = frame_param->frame_num;
            face_area.timestamp = frame_param->timestamp;
            CMR_LOGV("face_area.frame_id:%u,  "
                     "face_area.timestamp:%lld ",
                     face_area.frame_id, face_area.timestamp);

            CMR_LOGV("face_num %d, size:%dx%d", face_area.face_num,
                     face_area.frame_width, face_area.frame_height);

            if (face_info_max_num < face_area.face_num) {
                face_area.face_num = face_info_max_num;
            }

            if(face_area.face_num) {
                camera_facealign_conversion(cxt, frame_param, &face_area, &sn_trim);
            }
            camera_cfg_face_roi(cxt, frame_param, &face_area, &sn_trim);
            /* SS requires to disable FD when HDR is on */
            if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
                    cxt->sn_cxt.sensor_info.image_format &&
                (!cxt->is_vendor_hdr) && frame_param->is_update_isp) {
                isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_FACE_AREA,
                          (void *)&face_area);
            }

#ifdef CONFIG_ISP_2_3
            cmr_bzero(&face_area, sizeof(struct isp_face_area));

            camera_get_senor_mode_trim2(cxt, &af_trim);
            face_area.frame_width = af_trim.width;
            face_area.frame_height = af_trim.height;
            face_area.face_num = frame_param->face_num;
            CMR_LOGV("af face_num %d, size:%dx%d", face_area.face_num,
                     face_area.frame_width, face_area.frame_height);

            camera_cfg_face_roi(cxt, frame_param, &face_area, &sn_trim);

            if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
                    cxt->sn_cxt.sensor_info.image_format &&
                (!cxt->is_vendor_hdr) && /* SS requires to disable FD when HDR
                                            is on */
                frame_param->is_update_isp) {
                isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AF_FACE_AREA,
                          (void *)&face_area);
            }
#endif
        }

        break;
    case PREVIEW_EXIT_CB_PREPARE:
        oem_cb_type = CAMERA_EXIT_CB_PREPARE;
        break;
    case PREVIEW_EVT_CB_RESUME:
        oem_cb_type = CAMERA_EVT_CB_RESUME;
        break;
    case PREVIEW_EVT_CB_AT:
        oem_cb_type = CAMERA_EVT_CB_AUTO_TRACKING;
        CMR_LOGV("PREVIEW_EVT_CB_AT");
        struct camera_frame_type *frame_param_one =
            (struct camera_frame_type *)param;
        struct common_isp_cmd_param isp_cmd_parm;
        cmr_bzero(&isp_cmd_parm, sizeof(struct common_isp_cmd_param));
        struct setting_context *setting_cxt = &cxt->setting_cxt;
        struct setting_cmd_parameter setting_param;
        cmr_bzero(&setting_param, sizeof(setting_param));

        isp_cmd_parm.camera_id = cxt->camera_id;
        memcpy(&(isp_cmd_parm.af_ot_info), &(frame_param_one->at_cb_info),
               sizeof(struct auto_tracking_info));
        ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_AUTO_TRACKING_INFO,
                         &isp_cmd_parm);

        setting_param.camera_id = cxt->camera_id;
        setting_param.cmd_type_value = isp_cmd_parm.af_ot_info.status;
        ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                SETTING_SET_SPRD_AUTOCHASING_STATUS,
                                &setting_param);
        break;

    default:
        CMR_LOGE("err, %d", cb_type);
        ret = -CMR_CAMERA_NO_SUPPORT;
    }
    if (ret) {
        CMR_LOGE("CAMERA_EVT_CB_AUTO_TRACKING ERROR");
        goto exit;
    }

    /*if (CAMERA_FUNC_STOP_PREVIEW == oem_func && CAMERA_RSP_CB_SUCCESS ==
    oem_cb_type) {
            CMR_LOGV("stop preview response, notify directly");
            if (cxt->camera_cb) {
                    cxt->camera_cb(oem_cb_type, cxt->client_data, oem_func,
    param);
            }
            return ret;
    }*/

    if (param) {
        message.data = malloc(sizeof(struct camera_frame_type));
        if (!message.data) {
            CMR_LOGE("failed to malloc msg");
            ret = -CMR_CAMERA_NO_MEM;
            goto exit;
        }
        message.alloc_flag = 1;

        if ((cxt->is_lls_enable) && (PREVIEW_EVT_CB_FRAME == cb_type)) {
            struct camera_frame_type *prev_frame =
                (struct camera_frame_type *)param;
            prev_frame->lls_info = camera_get_ae_lum_value(oem_handle);
        }

        // skip preview frames for hdr effect
        if (camera_get_hdr_flag(cxt) == 1 && cxt->hdr_skip_frame_enable == 1 &&
            cb_type == PREVIEW_EVT_CB_FRAME) {
            struct camera_frame_type *prev_frame =
                (struct camera_frame_type *)param;

            if (prev_frame->type == PREVIEW_FRAME) {
                CMR_LOGD("monoboottime %llu hdr_capture_timestamp %llu",
                         prev_frame->monoboottime, cxt->hdr_capture_timestamp);

                if (prev_frame->monoboottime > cxt->hdr_capture_timestamp) {
                    prev_frame->type = PREVIEW_CANCELED_FRAME;
                    cxt->hdr_skip_frame_cnt++;
                    CMR_LOGD("hdr_skip_frame_cnt %d", cxt->hdr_skip_frame_cnt);
                }
            }
            if (cxt->hdr_skip_frame_cnt == HDR_SKIP_FRAME_NUM) {
                cxt->hdr_skip_frame_enable = 0;
                CMR_LOGD("hdr_skip_frame_cnt done");
            }
        }

        // skip preview frames for fdr effect
        if (camera_get_fdr_flag(cxt) == 1 && cxt->fdr_skip_frame_enable == 1 &&
            cb_type == PREVIEW_EVT_CB_FRAME) {
            struct camera_frame_type *prev_frame =
                (struct camera_frame_type *)param;

            if (prev_frame->type == PREVIEW_FRAME) {
                CMR_LOGD("monoboottime %llu fdr_capture_timestamp %llu",
                         prev_frame->monoboottime, cxt->fdr_capture_timestamp);
                prev_frame->type = PREVIEW_CANCELED_FRAME;
                if (prev_frame->monoboottime > cxt->fdr_capture_timestamp) {
                    cxt->fdr_skip_frame_cnt++;
                    CMR_LOGD("fdr_skip_frame_cnt %d", cxt->fdr_skip_frame_cnt);
                }
            }
            if (cxt->fdr_skip_frame_cnt == cxt->fdr_total_frame_cnt - 1) {
                CMR_LOGD("fdr_total_frame_cnt: %d", cxt->fdr_total_frame_cnt +
                          cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1);
                //unlock AE
               ret = camera_fdr_set_ev(cxt, 0);
               if(ret) {
                  CMR_LOGE("fdr failed to unlock ae");
               }
            }
            if (cxt->fdr_skip_frame_cnt == cxt->fdr_total_frame_cnt +
                 cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1) {
                cxt->fdr_skip_frame_enable = 0;
                CMR_LOGD("fdr_skip_total_frame_cnt: %d", cxt->fdr_total_frame_cnt +
                           cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1);
                //unlock 3A
                ret = camera_fdr_ctrl_3A(cxt, 0);
                if(ret) {
                CMR_LOGE("fdr failed to unlock 3a");
                }
            }
        }

        if (cxt->skip_frame_enable == 1 && cb_type == PREVIEW_EVT_CB_FRAME) {
            struct camera_frame_type *prev_frame =
                (struct camera_frame_type *)param;
            if (prev_frame->type == PREVIEW_FRAME) {
                cmr_int offettime = 0;
                CMR_LOGD("monoboottime %llu dre_capture_timestamp %llu",
                         prev_frame->monoboottime, cxt->capture_timestamp);
                struct phySensorInfo *phyPtr = sensorGetPhysicalSnsInfo(cxt->camera_id);
                struct tmp_cts_ae_params tmp_ae_params;

                struct setting_cmd_parameter setting_param;
                struct setting_context *setting_cxt = &cxt->setting_cxt;
                cmr_uint app_mode;
                cmr_bzero(&setting_param, sizeof(setting_param));
                setting_param.camera_id = cxt->camera_id;
                ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                        SETTING_GET_APPMODE, &setting_param);
                if (ret) {
                    CMR_LOGE("failed to get app mode %ld", ret);
                    goto exit;
                }
                app_mode = setting_param.cmd_type_value;
                memcpy(&tmp_ae_params, cxt->snp_cxt.ae_common_info, sizeof(struct tmp_cts_ae_params));
                if(tmp_ae_params.exp_time)
                    offettime = tmp_ae_params.exp_time - phyPtr->start_offset_time;
                CMR_LOGD("offset %d current capture_timestamp %d start_offset_time %lld",
                offettime, tmp_ae_params.exp_time, phyPtr->start_offset_time);
                if (app_mode != -1 && (prev_frame->monoboottime > cxt->capture_timestamp - offettime)) {
                    prev_frame->type = PREVIEW_CANCELED_FRAME;
                    cxt->skip_frame_cnt--;
                    CMR_LOGD("current skip_frame_cnt %d", cxt->skip_frame_cnt);
                }
            }
            if (cxt->skip_frame_cnt == 0) {
                cxt->skip_frame_enable = 0;
                // need disable ev adjust, or disable too late
                if (CAMERA_3DNR_TYPE_NIGHT_DNS == camera_get_3dnr_flag(cxt))
                    camera_snapshot_set_ev(oem_handle, 0, SNAPSHOT_NIGHT_DNS);
            }
        }

        // preflash and highflash shutdown skip preview frame
        if (cxt->flash_skip_frame_enable && cb_type == PREVIEW_EVT_CB_FRAME) {
            struct camera_frame_type *prev_frame =
                (struct camera_frame_type *)param;
            if (prev_frame->type == PREVIEW_FRAME) {
                CMR_LOGD("monoboottime %llu flash_handle_timestamp %llu",
                         prev_frame->monoboottime, cxt->flash_handle_timestamp);
                if (prev_frame->monoboottime > cxt->flash_handle_timestamp) {
                    prev_frame->type = PREVIEW_CANCELED_FRAME;
                    cxt->flash_skip_frame_cnt++;
                    CMR_LOGD("flash_skip_frame_cnt %d",
                             cxt->flash_skip_frame_cnt);
                }
            }
            if (cxt->flash_skip_frame_cnt == cxt->flash_skip_frame_num) {
                cxt->flash_skip_frame_enable = 0;
                CMR_LOGD("flash_skip_frame_cnt done");
            }
        }

        memcpy(message.data, param, sizeof(struct camera_frame_type));
    }
    message.msg_type = oem_func;
    message.sub_msg_type = oem_cb_type;
    message.sync_flag = CMR_MSG_SYNC_NONE;

    if (param && cxt->video_face_beauty_en) {
        struct camera_frame_type *video_frame =
                  (struct camera_frame_type *)param;
        if(video_frame->type == PREVIEW_VIDEO_FRAME) {
           if (!is_video_face_beauty_on((void *)oem_handle)) {
#ifdef CONFIG_FACE_BEAUTY
              send_thr_handle = cxt->video_cb_thr_handle;
#endif
              CMR_LOGV("send msg to video thread type=%d ",
                         video_frame->type);
            } else {
                send_thr_handle = cxt->prev_cb_thr_handle;
                CMR_LOGV("send msg to preview thread face off type=%d ",
                         video_frame->type);
            }
        } else {
            send_thr_handle = cxt->prev_cb_thr_handle;
            CMR_LOGV("send msg to preview thread type=%d ", video_frame->type);
        }
    }

    ret = cmr_thread_msg_send(send_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send msg, ret %ld", ret);
        free(message.data);
        message.data = NULL;
    }
exit:
    return ret;
}

cmr_int camera_ipm_cb(cmr_u32 class_type, struct ipm_frame_out *cb_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    struct frm_info frame;
    struct img_frm *dst_frm;
    cmr_u32 width = 0, height = 0;

    if (!cb_param || !cb_param->private_data) {
        CMR_LOGE("error param");
        return -CMR_CAMERA_INVALID_PARAM;
    }
    cxt = (struct camera_context *)cb_param->private_data;

    dst_frm = &cb_param->dst_frame;
    CMR_LOGD("cb type %d, dst frm id %d, fd 0x%x, vaddr_y %lx, fmt %d, w %d h %d, buf size %d, rect %d %d %d %d",
            class_type, dst_frm->base_id, dst_frm->fd, dst_frm->addr_vir.addr_y, dst_frm->fmt,
            dst_frm->size.width, dst_frm->size.height, dst_frm->buf_size,
            dst_frm->rect.start_x, dst_frm->rect.start_y, dst_frm->rect.width, dst_frm->rect.height);

    // for cache coherency
    cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle,
                              &cb_param->dst_frame);

    cxt->ipm_cxt.frm_num = 0;

    frame = cxt->snp_cxt.cur_frm_info;
    frame.channel_id = cxt->snp_cxt.channel_id;

    if (1 == camera_get_hdr_flag(cxt)) {
        camera_local_set_zsl_snapshot_buffer(
            cxt, cb_param->dst_frame.addr_phy.addr_y,
            cb_param->dst_frame.addr_vir.addr_y, cb_param->dst_frame.fd);
    } else if (1 == camera_get_3dnr_flag(cxt) ||
               5 == camera_get_3dnr_flag(cxt) ||
               8 == camera_get_3dnr_flag(cxt) ||
               CAMERA_3DNR_TYPE_NIGHT_DNS == camera_get_3dnr_flag(cxt)) {
        ret = camera_3dnr_set_ev((cmr_handle)cb_param->private_data, 0);
        if (ret)
            CMR_LOGE("fail to set 3dnr ev");
        camera_local_set_zsl_snapshot_buffer(
            cxt, cb_param->dst_frame.addr_phy.addr_y,
            cb_param->dst_frame.addr_vir.addr_y, cb_param->dst_frame.fd);
    } else if (1 == cxt->sn_cxt.info_4in1.is_4in1_supported) {
        cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle,
                                  &cb_param->dst_frame);
        ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                        SNAPSHOT_EVT_CHANNEL_DONE, &frame);
    }

    if (ret) {
        CMR_LOGE("fail to send frame to snp %ld", ret);
        goto exit;
    }

    CMR_LOGV("X");

exit:
    return ret;
}

void camera_snapshot_cb_to_hal(cmr_handle oem_handle, enum snapshot_cb_type cb,
                               enum snapshot_func_type func, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_uint oem_func = CAMERA_FUNC_TYPE_MAX;
    cmr_uint oem_cb_type = CAMERA_CB_TYPE_MAX;
    cmr_handle send_thr_handle = cxt->snp_cb_thr_handle;
    struct camera_frame_type *frame_ptr = NULL;
    struct isp_af_ts af_ts;

    CMR_MSG_INIT(message);

    switch (func) {
    case SNAPSHOT_FUNC_RELEASE_PICTURE:
        oem_func = CAMERA_FUNC_RELEASE_PICTURE;
        break;
    case SNAPSHOT_FUNC_TAKE_PICTURE:
        oem_func = CAMERA_FUNC_TAKE_PICTURE;
        break;
    case SNAPSHOT_FUNC_ENCODE_PICTURE:
        oem_func = CAMERA_FUNC_ENCODE_PICTURE;
        break;
    default:
        oem_func = func;
        break;
    }

    switch (cb) {
    case SNAPSHOT_CB_EVT_RSP_SUCCESS:
        oem_cb_type = CAMERA_RSP_CB_SUCCESS;
        break;
    case SNAPSHOT_CB_EVT_FLUSH_CACHE:
        oem_cb_type = CAMERA_EVT_CB_FLUSH;
        break;
    case SNAPSHOT_CB_EVT_INVALIDATE_CACHE:
        oem_cb_type = CAMERA_EVT_CB_INVALIDATE_CACHE;
        break;
    case SNAPSHOT_CB_EVT_FAILED:
        oem_cb_type = CAMERA_EXIT_CB_FAILED;
        break;
    case SNAPSHOT_CB_EVT_SNAPSHOT_DONE:
        oem_cb_type = CAMERA_EVT_CB_SNAPSHOT_DONE;
        send_thr_handle = cxt->snp_send_raw_image_handle;
        break;
    case SNAPSHOT_CB_EVT_DONE:
        if (cxt->is_multi_mode == MODE_MULTI_CAMERA || 0 == cxt->camera_id ||
            4 == cxt->camera_id) {
            af_ts.timestamp = ((struct camera_frame_type *)param)->timestamp;
            af_ts.capture = 1;
            isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_DCAM_TIMESTAMP,
                      &af_ts);
        }
        if (oem_func == CAMERA_FUNC_ENCODE_PICTURE) {
            ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_DONE_FDR, NULL);
            if (ret)
                CMR_LOGE("done fdr failed");
        }
        oem_cb_type = CAMERA_EXIT_CB_DONE;
        if (CAMERA_FUNC_TAKE_PICTURE == oem_func) {
            send_thr_handle = cxt->snp_send_raw_image_handle;
        }
        break;
    case SNAPSHOT_CB_EVT_CAPTURE_FRAME_DONE:
        oem_cb_type = CAMERA_EVT_CB_CAPTURE_FRAME_DONE;
        send_thr_handle = cxt->snp_secondary_thr_handle;
        break;
    case SNAPSHOT_CB_EVT_SNAPSHOT_JPEG_DONE:
        oem_cb_type = CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE;
        break;
    case SNAPSHOT_CB_EVT_RETURN_ZSL_BUF:
        oem_cb_type = CAMERA_EVT_CB_RETURN_ZSL_BUF;
        break;
    case SNAPSHOT_CB_EVT_PREPARE:
        oem_cb_type = CAMERA_EXIT_CB_PREPARE;
        break;
    case SNAPSHOT_CB_EVT_RETURN_SW_ALGORITHM_ZSL_BUF:
        oem_cb_type = CAMERA_EVT_CB_RETURN_SW_ALGORITHM_ZSL_BUF;
        break;
    default:
        oem_cb_type = cb;
        break;
    }

    CMR_LOGD("camera_cb %ld %ld", oem_cb_type, oem_func);

    // TBD: remove camera_frame_type and cam_ion_buffer_t, only data and size
    if (param) {
        if (oem_cb_type == CAMERA_EVT_CB_FLUSH ||
            oem_cb_type == CAMERA_EVT_CB_INVALIDATE_CACHE) {
            message.data = malloc(sizeof(cam_ion_buffer_t));
            if (!message.data) {
                CMR_LOGE("malloc failed");
                ret = -CMR_CAMERA_NO_MEM;
                goto exit;
            }
            message.alloc_flag = 1;
            memcpy(message.data, param, sizeof(cam_ion_buffer_t));
        } else {
            message.data = malloc(sizeof(struct camera_frame_type));
            if (!message.data) {
                CMR_LOGE("failed to malloc msg");
                ret = -CMR_CAMERA_NO_MEM;
                goto exit;
            }
            message.alloc_flag = 1;
            frame_ptr = (struct camera_frame_type *)message.data;
            memcpy(message.data, param, sizeof(struct camera_frame_type));
            frame_ptr->sensor_info.exposure_time_numerator =
                cxt->sn_cxt.exif_info.ExposureTime.numerator;
            frame_ptr->sensor_info.exposure_time_denominator =
                cxt->sn_cxt.exif_info.ExposureTime.denominator;
        }
    }
    message.msg_type = oem_func;
    message.sub_msg_type = oem_cb_type;
    if (CAMERA_EVT_CB_CAPTURE_FRAME_DONE == oem_cb_type) {
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    } else if (CAMERA_EXIT_CB_PREPARE == oem_cb_type ||
               CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE == oem_cb_type ||
               CAMERA_EVT_CB_SNAPSHOT_DONE == oem_cb_type ||
               CAMERA_EVT_CB_RETURN_ZSL_BUF == oem_cb_type ||
               CAMERA_EVT_CB_RETURN_SW_ALGORITHM_ZSL_BUF == oem_cb_type) {
        message.sync_flag = CMR_MSG_SYNC_NONE;
    } else {
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    }
    ret = cmr_thread_msg_send(send_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send msg %ld", ret);
        if (message.data) {
            free(message.data);
            message.data = NULL;
        }
        goto exit;
    }

exit:
    CMR_LOGV("X");
}

void camera_set_3dnr_flag(struct camera_context *cxt, cmr_u32 threednr_flag) {
    char value[PROPERTY_VALUE_MAX];
    CMR_LOGD("3dnr flag %d", threednr_flag);
    sem_wait(&cxt->threednr_flag_sm);
    property_get("debug.camera.3dnr.capture", value, "true");
    if (!strcmp(value, "false")) {
        threednr_flag = CAMERA_3DNR_TYPE_NULL;
    }

    // the flag is set from user, and need reassign the
    // value by the hw capability
    if (threednr_flag != CAMERA_3DNR_TYPE_NULL) {
        struct cmr_path_capability capability;
        cmr_bzero(&capability, sizeof(capability));
        cmr_grab_path_capability(cxt->grab_cxt.grab_handle, &capability);

        CMR_LOGD("'capability.support_3dnr_modes:%d",
                 capability.support_3dnr_mode);

#ifndef CONFIG_ISP_2_3
        if (capability.support_3dnr_mode != SPRD_3DNR_HW) {
            threednr_flag = CAMERA_3DNR_TYPE_NULL;
        }
#endif
    }

    cxt->snp_cxt.sprd_3dnr_type = threednr_flag;
    sem_post(&cxt->threednr_flag_sm);
}

cmr_u32 camera_get_3dnr_flag(struct camera_context *cxt) {
    cmr_u32 threednr_flag = 0;
    // sem_wait(&cxt->threednr_flag_sm);
    threednr_flag = cxt->snp_cxt.sprd_3dnr_type;
    // sem_post(&cxt->threednr_flag_sm);
    return threednr_flag;
}

cmr_u32 camera_get_cnr_realtime_flag(cmr_handle oem_handle) {

    int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;
    cmr_u32 cnr_ynr_flag = 0;

    cnr_ynr_flag = camera_get_cnr_flag(oem_handle);

#ifdef CAMERA_CNR3_ENABLE
    if (cnr_ynr_flag) {
        cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
        ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CNR2CNR3_YNR_EN,
                               &isp_param);
        if (ret) {
            CMR_LOGE("isp get COM_ISP_GET_CNR2CNR3_YNR_EN  failed");
            return false;
        }
        CMR_LOGD("isp cnr enable %d", isp_param.cnr2cnr3_ynr_en);
        return isp_param.cnr2cnr3_ynr_en;
    }
#else
    if (cnr_ynr_flag) {
        cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
        ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_YNR_EN, &isp_param);
        if (ret) {
            CMR_LOGE("isp get COM_ISP_GET_CNR2_YNR_EN  failed");
            return false;
        }
        CMR_LOGD("isp cnr enable %d", isp_param.cnr2_ynr_en);
        return isp_param.cnr2_ynr_en;
    }
#endif

    return false;
}

cmr_u32 camera_get_cnr_flag(cmr_handle oem_handle) {

    int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 cnr_flag = 0;
    struct setting_cmd_parameter setting_param;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;

#ifndef CONFIG_CAMERA_CNR
    return ret;
#endif
    char value[PROPERTY_VALUE_MAX];

    property_get("persist.vendor.cam.cnr.mode", value, "0");
    if (atoi(value)) {
        ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_CNR,
                                &setting_param);
        cnr_flag = setting_param.cmd_type_value;
        CMR_LOGV("cnr_flag is : %d", cnr_flag);
    }
    return cnr_flag;
}

cmr_u32 camera_set_ee_flag(struct camera_context *cxt, cmr_u32 ee_flag) {
    int ret = CMR_CAMERA_SUCCESS;

    struct setting_cmd_parameter setting_param;
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    setting_param.cmd_type_value = ee_flag;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle, CAMERA_PARAM_SPRD_ENABLE_POSTEE,
                            &setting_param);
	CMR_LOGD("set ee flag: %d", ee_flag);
    return ret;
}

cmr_u32 camera_get_ee_flag(cmr_handle oem_handle) {
    int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 ee_flag = 0;
    struct setting_cmd_parameter setting_param;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_EE,
                            &setting_param);
    ee_flag = setting_param.cmd_type_value;
    CMR_LOGD("ee_flag is : %d", ee_flag);

    return ee_flag;
}
/* camera_get_watermark_flag
 * return: 0: none, 1:logo,2:time,3:both
 */
cmr_u32 camera_get_watermark_flag(cmr_handle oem_handle) {

    int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 watermark_flag = 0;
    struct setting_cmd_parameter setting_param;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    /* watermark test */
    do {
        char chr[PROPERTY_VALUE_MAX];

        property_get("debug.vendor.cam.watermark.test", chr, "0");
        ret = atoi(chr);
        if (ret)
            return ret;
    } while (0);

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_LOGO_WATERMARK, &setting_param);
    if (setting_param.cmd_type_value)
        watermark_flag |= WATERMARK_LOGO;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_TIME_WATERMARK, &setting_param);
    if (setting_param.cmd_type_value)
        watermark_flag |= WATERMARK_TIME;

    return watermark_flag;
}

cmr_int camera_open_3dnr(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                         struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGD("E");
    sem_wait(&cxt->threednr_flag_sm);
    ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_3DNR, in_ptr, out_ptr,
                       &cxt->ipm_cxt.threednr_handle);
    sem_post(&cxt->threednr_flag_sm);
    CMR_LOGD("X");
    return ret;
}

cmr_int camera_close_3dnr(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->threednr_flag_sm);
    if (cxt->ipm_cxt.threednr_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.threednr_handle);
        cxt->ipm_cxt.threednr_handle = 0;
    }
    sem_post(&cxt->threednr_flag_sm);
    sem_destroy(&cxt->threednr_proc_sm);
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_open_night_dns(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                         struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int i = 0;

    CMR_LOGV("E");
    // if another night_dns not exit, wait 2s
    while (flag_night_dns_init == 1 && i < 200) {
        usleep(10000);
        i++;
    }
    if (i)
        CMR_LOGI("wait %dms", 10 * i);
    sem_wait(&cxt->threednr_flag_sm);
    ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_NIGHTDNS, in_ptr, out_ptr,
                       &cxt->ipm_cxt.threednr_handle);
    sem_post(&cxt->threednr_flag_sm);
    flag_night_dns_init = 1;
    CMR_LOGD("X");
    return ret;
}

cmr_int camera_close_night_dns(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->threednr_flag_sm);
    if (cxt->ipm_cxt.threednr_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.threednr_handle);
        cxt->ipm_cxt.threednr_handle = 0;
    }
    sem_post(&cxt->threednr_flag_sm);
    sem_destroy(&cxt->threednr_proc_sm);
    flag_night_dns_init = 0;
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_open_cnr(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                        struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGD("E");
    sem_wait(&cxt->cnr_flag_sm);
    if (NULL == cxt->ipm_cxt.cnr_handle) {
        ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_CNR, in_ptr,
                           out_ptr, &cxt->ipm_cxt.cnr_handle);
    }
    sem_post(&cxt->cnr_flag_sm);
    CMR_LOGD("X");

    return ret;
}

cmr_int camera_close_cnr(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->cnr_flag_sm);
    if (cxt->ipm_cxt.cnr_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.cnr_handle);
        cxt->ipm_cxt.cnr_handle = 0;
    }
    sem_post(&cxt->cnr_flag_sm);
    CMR_LOGI("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_open_dre(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                        struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->dre_flag_sm);
    if (NULL == cxt->ipm_cxt.dre_handle) {
        ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_DRE, in_ptr,
                           out_ptr, &cxt->ipm_cxt.dre_handle);
    }
    sem_post(&cxt->dre_flag_sm);

    return ret;
}

cmr_int camera_close_dre(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->dre_flag_sm);
    if (cxt->ipm_cxt.dre_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.dre_handle);
        cxt->ipm_cxt.dre_handle = 0;
    }
    sem_post(&cxt->dre_flag_sm);
    CMR_LOGI("close dre done %ld", ret);
    return ret;
}
cmr_int camera_open_ai_scene(struct camera_context *cxt,
                             struct ipm_open_in *in_ptr,
                             struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGD("E");
    sem_wait(&cxt->ai_scene_flag_sm);
    if (NULL == cxt->ipm_cxt.ai_scene_handle) {
        ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_AI_SCENE, in_ptr,
                           out_ptr, &cxt->ipm_cxt.ai_scene_handle);
    }
    sem_post(&cxt->ai_scene_flag_sm);
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_close_ai_scene(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->ai_scene_flag_sm);
    if (cxt->ipm_cxt.ai_scene_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.ai_scene_handle);
        cxt->ipm_cxt.ai_scene_handle = 0;
    }
    sem_post(&cxt->ai_scene_flag_sm);
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

void camera_set_fdr_flag(struct camera_context *cxt, cmr_u32 fdr_flag) {
	struct ipmpro_context *cur;

	cur = &cxt->swa_cxt_fdr;
	if (cur->inited == 0) {
		cxt->snp_cxt.is_fdr = 0;
		CMR_LOGD("fdr is not supported\n");
		return;
	}

	CMR_LOGV("E. flag %d, camera_id:%d", fdr_flag, cxt->camera_id);
	sem_wait(&cur->sem_flag);
	if(is_multi_camera_mode_oem == MODE_MULTI_CAMERA &&
		cxt->ref_camera_id != cxt->camera_id) {
		CMR_LOGD("multi camera mode and set background cam fdr flag to 0");
		fdr_flag = 0;
	}
	cxt->snp_cxt.is_fdr = fdr_flag;
	sem_post(&cur->sem_flag);
	CMR_LOGD("X. flag %d, camera_id:%d", fdr_flag, cxt->camera_id);
}

cmr_u32 camera_get_fdr_flag(struct camera_context *cxt) {
	cmr_u32 fdr_flag = 0;

	fdr_flag = cxt->swa_cxt_fdr.inited ? cxt->snp_cxt.is_fdr : 0;
	return fdr_flag;
}

void camera_set_hdr_flag(struct camera_context *cxt, cmr_u32 hdr_flag) {
    CMR_LOGV("flag %d", hdr_flag);
    sem_wait(&cxt->hdr_flag_sm);
    cxt->snp_cxt.is_hdr = hdr_flag;
    sem_post(&cxt->hdr_flag_sm);
}

cmr_u32 camera_get_hdr_flag(struct camera_context *cxt) {
    cmr_u32 hdr_flag = 0;
    // sem_wait(&cxt->hdr_flag_sm);
    hdr_flag = cxt->snp_cxt.is_hdr;
    // sem_post(&cxt->hdr_flag_sm);
    CMR_LOGV("%d", hdr_flag);
    return hdr_flag;
}

cmr_int camera_open_hdr(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                        struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGD("E");
    sem_wait(&cxt->hdr_flag_sm);
    ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_HDR, in_ptr, out_ptr,
                       &cxt->ipm_cxt.hdr_handle);
    sem_post(&cxt->hdr_flag_sm);
    CMR_LOGD("X");
    return ret;
}

cmr_int camera_close_hdr(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->hdr_flag_sm);
    if (cxt->ipm_cxt.hdr_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.hdr_handle);
        cxt->ipm_cxt.hdr_handle = 0;
    }
    sem_post(&cxt->hdr_flag_sm);
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_open_mfsr(struct camera_context *cxt,
		struct ipm_open_in *in_ptr,
		struct ipm_open_out *out_ptr)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct common_isp_cmd_param isp_param;

	memset(&isp_param, 0, sizeof(struct common_isp_cmd_param));
	ret = camera_isp_ioctl(cxt, COM_ISP_GET_MFSR_PARAM, &isp_param);
	if (ret) {
		CMR_LOGE("fail to get mfsr param\n");
		in_ptr->otp_data.otp_ptr = NULL;
		in_ptr->otp_data.otp_size = 0;
	} else {
		in_ptr->otp_data.otp_ptr = isp_param.isp_blk_param.param_ptr;
		in_ptr->otp_data.otp_size = isp_param.isp_blk_param.param_size;
	}
	in_ptr->frame_cnt = 5;
	in_ptr->frame_size = cxt->snp_cxt.request_size;

	sem_wait(&cxt->mfsr_sm);
	ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_MFSR, in_ptr, out_ptr,
					&cxt->ipm_cxt.mfsr_handle);
	if (ret)
		goto exit;

	ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_MFSR_POST, in_ptr, out_ptr,
					&cxt->ipm_cxt.mfsr_post_handle);
exit:
	sem_post(&cxt->mfsr_sm);
	return ret;
}

void camera_close_mfsr(struct camera_context *cxt)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;

	sem_wait(&cxt->mfsr_sm);
	if (cxt->ipm_cxt.mfsr_handle) {
		ret = cmr_ipm_close(cxt->ipm_cxt.mfsr_handle);
		cxt->ipm_cxt.mfsr_handle = NULL;
	}
	if (cxt->ipm_cxt.mfsr_post_handle) {
		ret = cmr_ipm_close(cxt->ipm_cxt.mfsr_post_handle);
		cxt->ipm_cxt.mfsr_post_handle = NULL;
	}
	sem_post(&cxt->mfsr_sm);
}


void camera_set_filter_flag(struct camera_context *cxt, cmr_u32 filter_flag) {
    CMR_LOGV("flag %d", filter_flag);

    sem_wait(&cxt->filter_sm);
    cxt->snp_cxt.filter_type = filter_flag;
    sem_post(&cxt->filter_sm);
}

cmr_uint camera_get_filter_flag(struct camera_context *cxt) {
    cmr_u32 filter_flag = 0;

    sem_wait(&cxt->filter_sm);
    filter_flag = cxt->snp_cxt.filter_type;
    sem_post(&cxt->filter_sm);
    CMR_LOGD("%d", filter_flag);

    return filter_flag;
}

cmr_int camera_open_filter(struct camera_context *cxt,
                           struct ipm_open_in *in_ptr,
                           struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_LOGD("E");
    sem_wait(&cxt->filter_sm);
    ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_FILTER, in_ptr,
                       out_ptr, &cxt->ipm_cxt.filter_handle);
    sem_post(&cxt->filter_sm);
    CMR_LOGD("X");
    return ret;
}

cmr_int camera_close_filter(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    sem_wait(&cxt->filter_sm);
    if (cxt->ipm_cxt.filter_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.filter_handle);
        cxt->ipm_cxt.filter_handle = 0;
    }
    sem_post(&cxt->filter_sm);
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

void camera_set_touch_xy(struct camera_context *cxt,
                         struct touch_coordinate touch_info) {
    CMR_LOGD("touch_info.touchX %d touch_info.touchY %d", touch_info.touchX,
             touch_info.touchY);
    cxt->snp_cxt.touch_xy.touchX = touch_info.touchX;
    cxt->snp_cxt.touch_xy.touchY = touch_info.touchY;
}

void camera_snapshot_state_handle(cmr_handle oem_handle,
                                  enum snapshot_cb_type cb,
                                  enum snapshot_func_type func, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (SNAPSHOT_FUNC_STATE == func) {
        switch (cb) {
        case SNAPSHOT_CB_EVT_ENC_DONE:
            CMR_LOGD("close hdr before jpeg enc done");
            struct setting_context *setting_cxt = &cxt->setting_cxt;
            struct setting_cmd_parameter setting_param;
            setting_param.camera_id = cxt->camera_id;
            ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                    SETTING_GET_APPMODE, &setting_param);
            if (ret)
                CMR_LOGE("failed to get app mode");
            if (CAMERA_3DNR_TYPE_NIGHT_DNS == camera_get_3dnr_flag(cxt)) {
                ret = camera_close_night_dns(cxt);
            } else if ((setting_param.cmd_type_value == CAMERA_MODE_NIGHT_PHOTO) &&
                cxt->night_cxt.is_authorized && cxt->night_cxt.mfnr_on_off) {
                cxt->night_cxt.mfnr_on_off = 0;
                ret = cxt->night_cxt.sw_close(cxt);
            } else if (1 == camera_get_hdr_flag(cxt)) {
                ret = camera_close_hdr(cxt);
            } else if (1 == camera_get_3dnr_flag(cxt) ||
                       5 == camera_get_3dnr_flag(cxt) ||
                       8 == camera_get_3dnr_flag(cxt)) {
                ret = camera_close_3dnr(cxt);
            } else if(camera_get_fdr_flag(cxt)) {
                ret = camera_close_fdr(cxt);
                if (ret) {
                    CMR_LOGE("failed to close hdr");
                    goto exit;
                }
            } else if (cxt->snp_cxt.is_mfsr) {
                camera_close_mfsr(cxt);
            }
            CMR_LOGD("jpeg enc done");
            break;
        case SNAPSHOT_CB_EVT_STATE:
            cxt->snp_cxt.status = ((struct camera_frame_type *)param)->status;
            CMR_LOGD("snapshot state is %d", cxt->snp_cxt.status);
            break;
        default:
            CMR_LOGE("don't support cb %d", cb);
            break;
        }
    }
exit:
    if (SNAPSHOT_FUNC_RECOVERY == func) {
        // to do
    }
}

void camera_snapshot_cb(cmr_handle oem_handle, enum snapshot_cb_type cb,
                        enum snapshot_func_type func, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_uint oem_func;
    cmr_uint oem_cb_type;
    struct camera_jpeg_param enc_param;

    if (!oem_handle) {
        CMR_LOGE("error handle");
        return;
    }
    CMR_LOGD("func %d cb_type %d\n", func, cb);
    if (SNAPSHOT_FUNC_TAKE_PICTURE == func ||
        SNAPSHOT_FUNC_ENCODE_PICTURE == func) {
        if ((SNAPSHOT_FUNC_ENCODE_PICTURE == func) &&
            (SNAPSHOT_CB_EVT_DONE == cb) && param) {
            enc_param = ((struct camera_frame_type *)param)->jpeg_param;
            if (enc_param.need_free) {
                camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
                camera_set_discard_frame(cxt, 0);
            }
        }
        camera_snapshot_cb_to_hal(oem_handle, cb, func, param);

        if (cxt->lls_shot_mode || cxt->is_vendor_hdr || cxt->is_pipviv_mode) {
            if ((SNAPSHOT_FUNC_TAKE_PICTURE == func) &&
                (SNAPSHOT_CB_EVT_SNAPSHOT_JPEG_DONE == cb) && param) {
                struct camera_frame_type *frame_type =
                    (struct camera_frame_type *)param;

                if (frame_type->need_free) {
                    camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
                    camera_set_discard_frame(cxt, 0);
                }
            }
        }
    } else if (SNAPSHOT_FUNC_RELEASE_PICTURE == func) {
        if (cxt->camera_cb) {
            cxt->camera_cb(CAMERA_RSP_CB_SUCCESS, cxt->client_data,
                           CAMERA_FUNC_RELEASE_PICTURE, NULL);
        }
    } else {
        camera_snapshot_state_handle(oem_handle, cb, func, param);
    }
}

cmr_int camera_before_set(cmr_handle oem_handle, enum preview_param_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("error handle");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = cmr_preview_before_set_param(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, mode);
exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_int camera_after_set(cmr_handle oem_handle,
                         struct after_set_cb_param *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_u32 skip_num = 0;

    if (!oem_handle) {
        CMR_LOGE("error handle");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (PREVIEWING == cmr_preview_get_status(cxt->prev_cxt.preview_handle,
                                             cxt->camera_id) &&
        (CAM_IMG_FMT_BAYER_MIPI_RAW == cxt->sn_cxt.sensor_info.image_format)) {
        skip_num = 0;
    } else {
        skip_num = param->skip_number;
    }
    CMR_LOGD("sensor fmt %d, skip num %d", cxt->sn_cxt.sensor_info.image_format,
             skip_num);

    ret = cmr_preview_after_set_param(cxt->prev_cxt.preview_handle,
                                      cxt->camera_id, param->re_mode,
                                      param->skip_mode, skip_num);

exit:
    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_int camera_focus_visit_flash_info(cmr_handle oem_handle,
                                      cmr_uint camera_id) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;

    setting_param.camera_id = camera_id;
    setting_param.cmd_type_value = 2;
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      SETTING_GET_HW_FLASH_STATUS, &setting_param);

    return setting_param.cmd_type_value; // 1 stands for pre-flash turned on
}

cmr_int camera_focus_pre_proc(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));

    CMR_LOGV("E");
    if (cxt->camera_id == 1)
        goto exit;

    setting_param.camera_id = cxt->camera_id;
    setting_param.setting_flash_status = SETTING_AF_FLASH_PRE_LIGHTING;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_CTRL_FLASH,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to open flash");
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int camera_focus_post_proc(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    cmr_int need_close_flash = 1;
    struct common_isp_cmd_param isp_param;
    cmr_uint video_snapshot_type;

    CMR_LOGV("E");

    if (cxt->camera_id == 1)
        goto exit;

    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));

    /*close flash*/

    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_VIDEO_SNAPSHOT_TYPE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get video snapshot enabled flag %ld", ret);
    } else {
        video_snapshot_type = setting_param.cmd_type_value;
        if (video_snapshot_type == VIDEO_SNAPSHOT_VIDEO)
            need_close_flash = 0;
    }

    if (need_close_flash) {
        setting_param.setting_flash_status = SETTING_AF_FLASH_PRE_AFTER;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_CTRL_FLASH, &setting_param);
        if (ret) {
            CMR_LOGE("failed to open flash %ld", ret);
        }
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int camera_get_preview_status(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt = &cxt->prev_cxt;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    if (1 != prev_cxt->inited) {
        CMR_LOGE("err, don't init preview");
        return ERROR;
    }

    return cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id);
}

cmr_int camera_sensor_init(cmr_handle oem_handle, cmr_uint is_autotest) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt = NULL;
    struct sensor_init_param init_param;
    cmr_handle sensor_handle;
    cmr_u32 camera_id_bits = 0;
    struct sensor_ex_info *sns_ex_info_ptr = NULL;

    CHECK_HANDLE_VALID(oem_handle);
    sn_cxt = &cxt->sn_cxt;
    CHECK_HANDLE_VALID(sn_cxt);

    if (1 == sn_cxt->inited) {
        CMR_LOGD("sensor has been intialized");
        goto exit;
    }
    cmr_bzero(&init_param, sizeof(init_param));
    init_param.oem_handle = oem_handle;
    init_param.is_autotest = is_autotest;

    ret = cmr_sensor_init(&init_param, &sensor_handle);
    if (ret) {
        CMR_LOGE("failed to init sensor %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
    sn_cxt->sensor_handle = sensor_handle;
    sn_cxt->inited = 1;
    camera_id_bits = 1 << cxt->camera_id;
    ret = cmr_sensor_open(sensor_handle, camera_id_bits);
    if (ret) {
        CMR_LOGE("open %d sensor failed %ld", cxt->camera_id, ret);
        goto sensor_exit;
    }

    cmr_sensor_event_reg(sensor_handle, cxt->camera_id, camera_sensor_evt_cb);
    cxt->sn_cxt.sensor_handle = sensor_handle;

    sns_ex_info_ptr = &sn_cxt->cur_sns_ex_info;
    ret = cmr_sensor_init_static_info(cxt);
    if (ret) {
        CMR_LOGE("init static info of  %d sensor failed %ld", cxt->camera_id,
                 ret);
        goto sensor_exit;
    }
    goto exit;

sensor_exit:
    cmr_sensor_deinit_static_info(cxt);
    cmr_sensor_deinit(sn_cxt->sensor_handle);
    sn_cxt->inited = 0;

exit:
    CMR_LOGV("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_sensor_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle sensor_handle;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt;

    CHECK_HANDLE_VALID(oem_handle);

    sn_cxt = &cxt->sn_cxt;
    if (0 == sn_cxt->inited) {
        CMR_LOGI("sensor has been de-intialized");
        goto exit;
    }
    sensor_handle = sn_cxt->sensor_handle;
    cmr_sensor_close(sensor_handle, (1 << cxt->camera_id));
    cmr_sensor_deinit_static_info(cxt);
    cmr_sensor_deinit(sensor_handle);
    cmr_bzero(sn_cxt, sizeof(*sn_cxt));

exit:
    ATRACE_END();
    return ret;
}
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
cmr_int camera_mm_dvfs_init(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct mm_dvfs_context *mm_dvfs_cxt = NULL;
    cmr_handle mm_dvfs_handle = NULL;
    CHECK_HANDLE_VALID(oem_handle);

    mm_dvfs_cxt = &cxt->mm_dvfs_cxt;

    if (0 == mm_dvfs_cxt->inited) {
        ret = cmr_mm_dvfs_init(&mm_dvfs_handle);
        if (ret) {
            CMR_LOGE("failed to init dvfs %ld", ret);
            ret = -CMR_CAMERA_NO_SUPPORT;
            goto exit;
        }
        mm_dvfs_cxt->inited = 1;
        mm_dvfs_cxt->mm_dvfs_handle = mm_dvfs_handle;
    }
exit:
    CMR_LOGD("X,ret=%ld, mm_dvfs_handle=%p", ret, mm_dvfs_handle);
    return ret;
}

cmr_int camera_mm_dvfs_deinit(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct mm_dvfs_context *mm_dvfs_cxt = NULL;

    CHECK_HANDLE_VALID(oem_handle);
    mm_dvfs_cxt = &cxt->mm_dvfs_cxt;
    if (0 == mm_dvfs_cxt->inited) {
        CMR_LOGD("mm_dvfs_ has been de-intialized");
        goto exit;
    }

    ret = cmr_mm_dvfs_deinit(mm_dvfs_cxt->mm_dvfs_handle);
    if (ret) {
        CMR_LOGE("failed to de-init mm_dvfs %ld", ret);
        goto exit;
    }
    cmr_bzero(mm_dvfs_cxt, sizeof(*mm_dvfs_cxt));
exit:
    CMR_LOGV("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_local_set_mm_dvfs_policy(cmr_handle oem_handle,
                                        enum DVFS_MM_MODULE module,
                                        enum CamProcessingState camera_state) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CHECK_HANDLE_VALID(oem_handle);
    ret = cmr_set_mm_dvfs_policy(oem_handle, module, camera_state);
    CMR_LOGV("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_set_mm_dvfs_param(cmr_handle oem_handle,
                                 struct prev_sn_param_dvfs_type dvfs_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CHECK_HANDLE_VALID(oem_handle);
    ret = cmr_set_mm_dvfs_param(oem_handle, dvfs_param);
    // CMR_LOGD("done %ld", ret);
    return ret;
}
#endif

cmr_int camera_grab_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);
    struct phySensorInfo *phyPtr = NULL;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct grab_context *grab_cxt = NULL;
    struct sensor_context *sn_cxt = NULL;
    cmr_handle grab_handle = NULL;
    struct grab_init_param grab_param;
    struct sensor_exp_info sensor_info;
    cmr_bzero(&sensor_info, sizeof(struct sensor_exp_info));
    CHECK_HANDLE_VALID(oem_handle);
    grab_cxt = &cxt->grab_cxt;
    CHECK_HANDLE_VALID(grab_cxt);
    sn_cxt = &(cxt->sn_cxt);
    CHECK_HANDLE_VALID(sn_cxt);

    ret = cmr_sensor_get_info(sn_cxt->sensor_handle, cxt->camera_id,
                              &(sn_cxt->sensor_info));
    if (ret) {
        CMR_LOGE("fail to get sensor info ret %ld", ret);
        goto exit;
    }
    phyPtr = sensorGetPhysicalSnsInfo(cxt->camera_id);

    if (0 == grab_cxt->inited) {
        grab_param.oem_handle = oem_handle;
        grab_param.sensor_id = phyPtr->slotId;
        ret = cmr_grab_init(&grab_param, &grab_handle);
        if (ret) {
            CMR_LOGE("failed to init grab %ld", ret);
            ret = -CMR_CAMERA_NO_SUPPORT;
            goto exit;
        }
        cmr_grab_evt_reg(grab_handle, camera_grab_evt_cb);
        cmr_grab_stream_cb(grab_handle, camera_sensor_streamctrl);
        /*only raw sensor should init isp*/
        if (CAM_IMG_FMT_BAYER_MIPI_RAW == sn_cxt->sensor_info.image_format) {
            cmr_grab_isp_statis_evt_reg(grab_handle, isp_statis_evt_cb);
            cmr_grab_isp_irq_proc_evt_reg(grab_handle, isp_irq_proc_evt_cb);
        }
        cmr_grab_post_ynr_evt_reg(grab_handle, camera_grab_post_ynr_evt_cb);
        grab_cxt->inited = 1;
        grab_cxt->grab_handle = grab_handle;
    }

exit:
    CMR_LOGD("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_grab_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct grab_context *grab_cxt = NULL;

    CHECK_HANDLE_VALID(oem_handle);
    grab_cxt = &cxt->grab_cxt;
    if (0 == grab_cxt->inited) {
        CMR_LOGD("GRAB has been de-intialized");
        goto exit;
    }

    ret = cmr_grab_deinit(grab_cxt->grab_handle);
    if (ret) {
        CMR_LOGE("failed to de-init grab %ld", ret);
        goto exit;
    }
    cmr_bzero(grab_cxt, sizeof(*grab_cxt));
exit:
    CMR_LOGD("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_s32 camera_local_get_iommu_status(cmr_handle oem_handle) {
    cmr_s32 ret;
    struct grab_context *grab_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    grab_cxt = &cxt->grab_cxt;

    ret = cmr_grab_get_iommu_status(grab_cxt->grab_handle);

    return ret;
}

cmr_int camera_set_security(cmr_handle oem_handle,
                            struct sprd_cam_sec_cfg *sec_cfg) {
    cmr_int ret;
    struct grab_context *grab_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    grab_cxt = &cxt->grab_cxt;

    ret = cmr_grab_set_security(grab_cxt->grab_handle, sec_cfg);
    return ret;
}

cmr_int camera_set_zsl_param(cmr_handle oem_handle,
                            struct sprd_cap_zsl_param *zsl_param) {
    cmr_int ret;
    struct grab_context *grab_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    grab_cxt = &cxt->grab_cxt;

    ret = cmr_grab_set_zsl_param(grab_cxt->grab_handle, zsl_param);
    return ret;
}
cmr_int camera_set_hdr_disable(cmr_handle oem_handle, cmr_u32 param) {
    cmr_int ret;
    struct grab_context *grab_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_exp_info sensor_info;
    struct common_isp_cmd_param isp_param;

    ret = camera_get_sensor_info(cxt, cxt->camera_id, &sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info.image_format) {
        isp_param.cmd_value = param;
        ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_HDR, (void *)&isp_param);
        ret = camera_close_hdr(cxt);
    } else {
        isp_param.cmd_value = param;
        ret = camera_sensor_ioctl(oem_handle, COM_SN_SET_HDR_EV,
                                  (void *)&isp_param);
    }

exit:
    return ret;
}

static cmr_int camera_jpeg_init_async_message_handler(struct cmr_msg *message, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)data;
    cmr_u32 msg_type = 0;

    msg_type = (cmr_u32)message->msg_type;
    switch (msg_type) {
    case CMR_EVT_INIT:
        ret = camera_jpeg_init(cxt);
        if (ret) {
            CMR_LOGE("failed to init jpeg async %ld", ret);
        } else {
            CMR_LOGI("jpeg async init ready");
        }
        break;

    case CMR_EVT_WAIT:
        CMR_LOGI("wait here");
        break;

    case CMR_EVT_EXIT:
        CMR_LOGI("exit here");
        break;

    default:
        break;
    }
    return ret;
}

cmr_int camera_jpeg_init_async(cmr_handle oem_handle) {
    int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    ret = cmr_thread_create(&cxt->jpeg_async_init_handle, 10,
            camera_jpeg_init_async_message_handler, (void *)oem_handle,
            "jpeg_async");
    if (ret) {
        CMR_LOGE("fail to create jpeg async init thread, %d", ret);
        goto exit;
    }
    CMR_LOGI("init jpeg async");

    CMR_MSG_INIT(message);
    message.msg_type = CMR_EVT_INIT;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    ret = cmr_thread_msg_send(cxt->jpeg_async_init_handle, &message);
    if (ret) {
        CMR_LOGE("fail to send message to jpeg async init thread, %d", ret);
        goto exit;
    }
    CMR_LOGI("send message to jpeg async init thread");

exit:
    return ret;
}

cmr_int camera_jpeg_init_wait(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (cxt->jpeg_async_init_handle != NULL) {
        CMR_MSG_INIT(message);
        message.msg_type = CMR_EVT_WAIT;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        ret = cmr_thread_msg_send(cxt->jpeg_async_init_handle, &message);
        if (ret) {
            CMR_LOGE("fail to send wait message to jpeg async init thread, %d", ret);
            goto exit;
        }
        CMR_LOGI("send wait message to jpeg async init thread");
    } else {
        ret = CMR_CAMERA_INVALID_PARAM;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_jpeg_deinit_async(cmr_handle oem_handle) {
    int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_MSG_INIT(message);
    message.msg_type = CMR_EVT_EXIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(cxt->jpeg_async_init_handle, &message);
    if (ret) {
        CMR_LOGE("fail to send exit message to jpeg async init thread, %d", ret);
        goto exit;
    }
    CMR_LOGI("send exit message to jpeg async init thread");

    ret = cmr_thread_destroy(cxt->jpeg_async_init_handle);
    if (ret) {
        CMR_LOGE("fail to destroy jpeg async thread");
        goto exit;
    }

    ret = camera_jpeg_deinit(oem_handle);

exit:
    return ret;
}

cmr_int camera_jpeg_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct jpeg_context *jpeg_cxt = NULL;

    CHECK_HANDLE_VALID(oem_handle);
    jpeg_cxt = &cxt->jpeg_cxt;
    CHECK_HANDLE_VALID(jpeg_cxt);

    if (cxt->camera_id >= 2 && is_multi_camera_mode_oem == MODE_BOKEH) {
        CMR_LOGD("secondary camera dont need jpeg alone");
        goto exit;
    }

    if (0 == jpeg_cxt->inited) {
        ret = cmr_jpeg_init(oem_handle, &jpeg_cxt->jpeg_handle,
                            camera_jpeg_evt_cb);
        if (ret) {
            CMR_LOGE("failed to init jpeg codec %ld", ret);
            ret = -CMR_CAMERA_NO_SUPPORT;
            goto exit;
        }
        jpeg_cxt->inited = 1;
    }

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_jpeg_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct jpeg_context *jpeg_cxt = NULL;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    jpeg_cxt = &cxt->jpeg_cxt;
    CHECK_HANDLE_VALID(jpeg_cxt);

    if (cxt->camera_id >= 2 && is_multi_camera_mode_oem == MODE_BOKEH) {
        CMR_LOGD("secondary camera dont need jpeg alone");
        goto exit;
    }

    if (0 == jpeg_cxt->inited) {
        CMR_LOGD("jpeg codec has been de-intialized");
        goto exit;
    }

    ret = cmr_jpeg_deinit(jpeg_cxt->jpeg_handle);
    if (ret) {
        CMR_LOGE("failed to de-init jpeg codec %ld", ret);
        goto exit;
    }
    cmr_bzero(jpeg_cxt, sizeof(*jpeg_cxt));

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_scaler_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct scaler_context *scaler_cxt = NULL;
    cmr_handle scale_handle;

    CHECK_HANDLE_VALID(oem_handle);
    scaler_cxt = &cxt->scaler_cxt;
    CHECK_HANDLE_VALID(scaler_cxt);

    if (1 == scaler_cxt->inited) {
        CMR_LOGD("scaler has been intialized");
        goto exit;
    }

    scale_handle = cxt->grab_cxt.grab_handle;
    ret = cmr_scale_open(&scale_handle);
    if (ret) {
        CMR_LOGE("failed to init scaler %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
    } else {
        scaler_cxt->scaler_handle = scale_handle;
        scaler_cxt->inited = 1;
    }

exit:
    CMR_LOGD("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_scaler_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct scaler_context *scaler_cxt;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    scaler_cxt = &cxt->scaler_cxt;
    CHECK_HANDLE_VALID(scaler_cxt);
    if (0 == scaler_cxt->inited) {
        CMR_LOGD("scaler has been de-intialized");
        goto exit;
    }

    ret = cmr_scale_close(scaler_cxt->scaler_handle);
    if (ret) {
        CMR_LOGE("failed to de-init scaler %ld", ret);
        goto exit;
    }
    cmr_bzero(scaler_cxt, sizeof(*scaler_cxt));

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_rotation_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct rotation_context *rot_cxt = NULL;
    cmr_handle rot_handle;

    CHECK_HANDLE_VALID(oem_handle);
    rot_cxt = &cxt->rot_cxt;
    CHECK_HANDLE_VALID(rot_cxt);

    if (1 == rot_cxt->inited) {
        CMR_LOGD("rot has been intialized");
        goto exit;
    }

    rot_handle = cxt->grab_cxt.grab_handle;
    ret = cmr_rot_open(&rot_handle);
    if (ret) {
        CMR_LOGE("failed to init rot %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
    } else {
        rot_cxt->rotation_handle = rot_handle;
        rot_cxt->inited = 1;
    }

exit:
    CMR_LOGD("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_rotation_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct rotation_context *rot_cxt;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    rot_cxt = &cxt->rot_cxt;
    CHECK_HANDLE_VALID(rot_cxt);

    if (0 == rot_cxt->inited) {
        CMR_LOGD("rot has been de-intialized");
        goto exit;
    }

    ret = cmr_rot_close(rot_cxt->rotation_handle);
    if (ret) {
        CMR_LOGE("failed to de-init rot %ld", ret);
        goto exit;
    }
    cmr_bzero(rot_cxt, sizeof(*rot_cxt));

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

void camera_calibrationconfigure_save(uint32_t start_addr, uint32_t data_size) {
    const char configfile[] = "/data/otpconfig.bin";

    FILE *configfile_handle = fopen(configfile, "wb");
    if (NULL == configfile_handle) {
        CMR_LOGE("failed");
        return;
    }
    fwrite(&start_addr, 1, 4, configfile_handle);
    fwrite(&data_size, 1, 4, configfile_handle);
    fclose(configfile_handle);
    CMR_LOGV("X");
}

void camera_calibrationconfigure_load(uint32_t *start_addr,
                                      uint32_t *data_size) {
    const char configfile[] = "/data/otpconfig.bin";
    cmr_u32 ret =0;

    FILE *configfile_handle = fopen(configfile, "rb");
    if (NULL == configfile_handle) {
        CMR_LOGE("failed");
        return;
    }
    ret = fread(&start_addr, 1, 4, configfile_handle);
    if (ret < 4) {
        CMR_LOGW("read start_addr calibrationconfigure file fail, len %d", ret);
    }
    ret = fread(&data_size, 1, 4, configfile_handle);
    if (ret < 4) {
        CMR_LOGW("read data_size calibrationconfigure file fail, len %d", ret);
    }
    fclose(configfile_handle);
    CMR_LOGV("X");
}

cmr_int camera_get_otpinfo(cmr_handle oem_handle, cmr_u8 dual_flag,
                           struct sensor_otp_cust_info *otp_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    SENSOR_VAL_T val;
    cmr_u8 dualflag = dual_flag;
    struct sensor_otp_cust_info *otp_ptr = NULL;

    if (NULL == oem_handle || NULL == otp_data) {
        ret = -CMR_CAMERA_INVALID_PARAM;
        CMR_LOGE("in parm error");
        goto exit;
    }

    ret = cmr_sensor_read_calibration_otp(cxt->sn_cxt.sensor_handle, dual_flag, otp_data, cxt->camera_id);
    if (CMR_CAMERA_SUCCESS == ret) {
        CMR_LOGD("dual_otp data from calibration bin");
    } else {
        val.type = SENSOR_VAL_TYPE_READ_DUAL_OTP;
        val.pval = &dualflag;
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                               SENSOR_ACCESS_VAL, (cmr_uint)&val);
        if ((val.pval != &dualflag) && (val.pval != NULL)) {
            otp_ptr = (struct sensor_otp_cust_info *)val.pval;
            if (otp_ptr->dual_otp.data_3d.size > 0) {
                memcpy(otp_data, val.pval, sizeof(struct sensor_otp_cust_info));
                CMR_LOGD("dual_otp data in eeprom");
            } else {
                ret = CMR_CAMERA_FAIL;
                CMR_LOGI("no dual_otp data from bin or eeprom");
                goto exit;
            }
        } else {
            ret = CMR_CAMERA_FAIL;
            CMR_LOGI("no dual_otp data from bin or eeprom");
            goto exit;
        }
    }

    CMR_LOGD("total_otp raw buffer %p, total_otp size %d, dual_flag %d, "
             "dual_otp ptr %p, dual_otp size %d",
             otp_data->total_otp.data_ptr, otp_data->total_otp.size,
             otp_data->dual_otp.dual_flag, otp_data->dual_otp.data_3d.data_ptr,
             otp_data->dual_otp.data_3d.size);

exit:
    return ret;
}

cmr_int camera_get_onlinebuffer(cmr_handle oem_handle, void *cali_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (NULL == oem_handle || NULL == cali_info) {
        ret = -CMR_CAMERA_INVALID_PARAM;
        CMR_LOGE("in parm error");
        goto exit;
    }

    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;

/*
    ret = isp_sw_get_depth_cali_info(isp_cxt->isp_handle, cali_info);
    if (ret) {
        CMR_LOGE("get sensor dual otp info failed %ld", ret);
        goto exit;
    }
*/
exit:
    return ret;
}

cmr_int camera_get_buff_handle(cmr_handle oem_handle, int frame_type,
                               cam_graphic_buffer_info_t *buf_cfg) {
    int32_t ret = 0;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (cxt->camera_cb) {
        enum camera_cb_type cb = CAMERA_EVT_PREVIEW_BUF_HANDLE;
        if (frame_type == PREVIEW_FRAME) {
            cb = CAMERA_EVT_PREVIEW_BUF_HANDLE;
        } else if (frame_type == PREVIEW_ZSL_FRAME) {
            cb = CAMERA_EVT_CAPTURE_BUF_HANDLE;
        } else if (frame_type == PREVIEW_VIDEO_FRAME) {
            cb = CAMERA_EVT_VIDEO_BUF_HANDLE;
        }
        cxt->camera_cb(cb, cxt->client_data, CAMERA_FUNC_GET_BUF_HANDLE,
                       buf_cfg);
    }
    return ret;
}

cmr_int camera_release_buff_handle(cmr_handle oem_handle, int frame_type,
                                   cam_graphic_buffer_info_t *buf_cfg) {
    int32_t ret = 0;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (cxt->camera_cb) {
        enum camera_cb_type cb = CAMERA_EVT_PREVIEW_BUF_HANDLE;
        if (frame_type == PREVIEW_FRAME) {
            cb = CAMERA_EVT_PREVIEW_BUF_HANDLE;
        } else if (frame_type == PREVIEW_ZSL_FRAME) {
            cb = CAMERA_EVT_CAPTURE_BUF_HANDLE;
        } else if (frame_type == PREVIEW_VIDEO_FRAME) {
            cb = CAMERA_EVT_VIDEO_BUF_HANDLE;
        }
        cxt->camera_cb(cb, cxt->client_data, CAMERA_FUNC_RELEASE_BUF_HANDLE,
                       buf_cfg);
    }
    return ret;
}

int32_t camera_isp_flash_get_charge(void *handler,
                                    struct isp_flash_cfg *cfg_ptr,
                                    struct isp_flash_cell *cell) {
    int32_t ret = 0;
    struct camera_context *cxt = (struct camera_context *)handler;
    return ret;
}

int32_t camera_isp_flash_get_time(void *handler, struct isp_flash_cfg *cfg_ptr,
                                  struct isp_flash_cell *cell) {
    int32_t ret = 0;
    struct camera_context *cxt = (struct camera_context *)handler;
    return ret;
}

int32_t camera_isp_flash_set_charge(void *handler,
                                    struct isp_flash_cfg *cfg_ptr,
                                    struct isp_flash_element *element) {
    cmr_s32 ret = 0;
    struct camera_context *cxt = (struct camera_context *)handler;

    struct sprd_flash_cfg_param cfg;
    cmr_u8 real_type = 0;
    struct sprd_flash_cell real_cell;

    if (!cxt || !element) {
        CMR_LOGE("err param, %p %p", cxt, element);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto out;
    }

    switch (cfg_ptr->type) {
    case ISP_FLASH_TYPE_PREFLASH:
        real_type = FLASH_TYPE_PREFLASH;
        break;
    case ISP_FLASH_TYPE_MAIN:
        real_type = FLASH_TYPE_MAIN;
        break;
    default:
        CMR_LOGE("not support the type");
        goto out;
        break;
    }
    cmr_bzero(&real_cell, sizeof(real_cell));
    cfg.real_cell.type = real_type;
    cfg.real_cell.count = 1;
    cfg.real_cell.led_idx = cfg_ptr->led_idx;
    cfg.real_cell.element[0].index = element->index;
    cfg.real_cell.element[0].val = element->val;
    cfg.io_id = FLASH_IOID_SET_CHARGE;
    cfg.flash_idx = cxt->facing % 2;
    cfg.real_cell.element[0].brightness = element->brightness;
    cfg.real_cell.element[0].color_temp = element->color_temp;
    cfg.real_cell.element[0].bg_color = element->bg_color;

    CMR_LOGD("led_idx=%d, flash_type=%d, idx=%d", cfg_ptr->led_idx, real_type,
             element->index);
    if (camera_front_lcd_flash_activie(cfg.flash_idx))
        ret = camera_front_lcd_flash_cfg(cxt, &cfg);
    else
        ret = cmr_grab_cfg_flash(cxt->grab_cxt.grab_handle, &cfg);
out:
    return ret;
}

int32_t camera_isp_flash_ctrl(void *handler, struct isp_flash_cfg *cfg_ptr,
                              struct isp_flash_element *element) {

    int32_t ret = 0;
    struct camera_context *cxt = (struct camera_context *)handler;
    cmr_u8 real_type;
    struct grab_flash_opt flash_opt;
    bzero(&flash_opt, sizeof(struct grab_flash_opt));
    if (!cxt || !cfg_ptr) {
        CMR_LOGE("err param, %p %p", cxt, cfg_ptr);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto out;
    }
    CMR_LOGD("led0_enable=%d,led1_enable=%d, led_type=%d", cfg_ptr->led0_enable,
             cfg_ptr->led1_enable, cfg_ptr->type);
    switch (cfg_ptr->type) {
    case ISP_FLASH_TYPE_PREFLASH:
        if (cfg_ptr->led0_enable || cfg_ptr->led1_enable) {
            real_type = FLASH_OPEN;
        } else {
            real_type = FLASH_CLOSE_AFTER_OPEN;
            cfg_ptr->led0_enable = 1;
            cfg_ptr->led1_enable = 1;
        }
        break;
    case ISP_FLASH_TYPE_MAIN:
        if (cfg_ptr->led0_enable || cfg_ptr->led1_enable) {
            real_type = FLASH_HIGH_LIGHT;
        } else {
            real_type = FLASH_CLOSE_AFTER_OPEN;
            cfg_ptr->led0_enable = 1;
            cfg_ptr->led1_enable = 1;
        }
        break;
    default:
        CMR_LOGE("not support the type");
        goto out;
        break;
    }

    flash_opt.led0_enable = cfg_ptr->led0_enable;
    flash_opt.led1_enable = cfg_ptr->led1_enable;
    flash_opt.flash_mode = real_type;
    flash_opt.flash_index = cxt->facing % 2;

    if (camera_front_lcd_flash_activie(flash_opt.flash_index))
        ret = camera_front_lcd_flash_callback(cxt, flash_opt.flash_mode);
    else
        ret = cmr_grab_flash_cb(cxt->grab_cxt.grab_handle, &flash_opt);
out:
    return ret;
}

int32_t camera_isp_flash_set_time(void *handler, struct isp_flash_cfg *cfg_ptr,
                                  struct isp_flash_element *element) {
    int32_t ret = 0;
    struct camera_context *cxt = (struct camera_context *)handler;
    return ret;
}

cmr_s32 camera_get_pos_info(cmr_handle oem_handle,
                            struct sensor_vcm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt = NULL;
    struct sensor_exp_info *sensor_info_ptr;
    struct sensor_raw_ioctrl *ioctrl_ptr;

    sn_cxt = &(cxt->sn_cxt);

    ret = cmr_sensor_get_info(sn_cxt->sensor_handle, cxt->camera_id,
                              &(sn_cxt->sensor_info));
    if (ret) {
        CMR_LOGE("fail to get sensor info ret %ld", ret);
        return ret;
    }

    sensor_info_ptr = &(sn_cxt->sensor_info);
    CHECK_HANDLE_VALID(sensor_info_ptr);
    CHECK_HANDLE_VALID(sensor_info_ptr->raw_info_ptr);
    ioctrl_ptr = sensor_info_ptr->raw_info_ptr->ioctrl_ptr;
    CHECK_HANDLE_VALID(ioctrl_ptr);

    if (sensor_info_ptr->raw_info_ptr &&
        sensor_info_ptr->raw_info_ptr->ioctrl_ptr &&
        sensor_info_ptr->raw_info_ptr->ioctrl_ptr->get_pos) {
        sensor_info_ptr->raw_info_ptr->ioctrl_ptr->get_pos(
            ioctrl_ptr->caller_handler, info);
    }

    return ret;
}

cmr_s32 camera_isp_set_pulse_line(void *handler, cmr_u32 line) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

#ifdef CONFIG_ISP_2_3
    struct camera_context *cxt = (struct camera_context *)handler;

    ret = cmr_grab_set_pulse_line(cxt->grab_cxt.grab_handle, line);
    return ret;
#else
    // CMR_LOGI("sharkl5 TBD");
    return ret;
#endif
}

cmr_s32 camera_isp_set_next_vcm_pos(void *handler, cmr_s32 pos) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

#ifdef CONFIG_ISP_2_3
    struct camera_context *cxt = (struct camera_context *)handler;
    struct sensor_vcm_info info;
    struct sprd_img_vcm_param vcm_param;

    cmr_bzero(&info, sizeof(info));
    cmr_bzero(&vcm_param, sizeof(vcm_param));

    if (0 <= pos) {
        info.pos = pos;
        camera_get_pos_info(handler, &info);
        vcm_param.next_vcm_pos = pos;
        vcm_param.vcm_i2c_count = info.cmd_len;
        memcpy(vcm_param.vcm_i2c_data, info.cmd_val, info.cmd_len);
        vcm_param.vcm_slave_addr = info.slave_addr;

        if ((0 == info.slave_addr) && (0 == info.cmd_len)) {
            CMR_LOGE("warning! fail to get vcm info");
            goto exit;
        }
    } else {
        vcm_param.next_vcm_pos = pos;
    }

    ret = cmr_grab_set_next_vcm_pos(cxt->grab_cxt.grab_handle, &vcm_param);
exit:
    return ret;
#else
    // CMR_LOGI("sharkl5 TBD");
    return ret;
#endif
}

cmr_s32 camera_isp_set_pulse_log(void *handler, cmr_u32 enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

#ifdef CONFIG_ISP_2_3
    struct camera_context *cxt = (struct camera_context *)handler;

    ret = cmr_grab_set_pulse_log(cxt->grab_cxt.grab_handle, enable);
    return ret;
#else
    // CMR_LOGI("sharkl5 TBD");
    return ret;
#endif
}

cmr_int camera_isp_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    char value[PROPERTY_VALUE_MAX];
    char ba_portrait[PROPERTY_VALUE_MAX];
    char fr_portrait[PROPERTY_VALUE_MAX];
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = NULL;
    struct sensor_context *sn_cxt = NULL;
    struct sensor_exp_info *sensor_info_ptr;
    struct sensor_ex_info *sns_ex_info_ptr = NULL;
    struct isp_init_param isp_param;
    struct isp_video_limit isp_limit;
    SENSOR_VAL_T val;
    struct sensor_pdaf_info pdaf_info;
    struct grab_context *grab_cxt = NULL;
    struct cmr_grab *p_grab = NULL;
    struct isp_cali_param cali_param;
    struct isp_data_info cali_result;
    struct sensor_4in1_info sn_4in1_info;

    cmr_bzero(&cali_param, sizeof(cali_param));
    cmr_bzero(&cali_result, sizeof(cali_result));
    cmr_bzero(&sn_4in1_info, sizeof(sn_4in1_info));
    cmr_bzero(&isp_param, sizeof(isp_param));
    cmr_bzero(&pdaf_info, sizeof(pdaf_info));

    CHECK_HANDLE_VALID(oem_handle);
    isp_cxt = &(cxt->isp_cxt);
    CHECK_HANDLE_VALID(isp_cxt);
    sn_cxt = &(cxt->sn_cxt);
    CHECK_HANDLE_VALID(sn_cxt);
    grab_cxt = &(cxt->grab_cxt);
    CHECK_HANDLE_VALID(grab_cxt);

    if (1 == isp_cxt->inited) {
        CMR_LOGD("isp has been intialized");
        goto exit;
    }

    ret = cmr_sensor_get_info(sn_cxt->sensor_handle, cxt->camera_id,
                              &(sn_cxt->sensor_info));
    if (ret) {
        CMR_LOGE("fail to get sensor info ret %ld", ret);
        goto exit;
    }
    CMR_LOGV("sensor_info.name=%s, version=%s", sn_cxt->sensor_info.name,
             sn_cxt->sensor_info.sensor_version_info);

    if (CAM_IMG_FMT_BAYER_MIPI_RAW != sn_cxt->sensor_info.image_format) {
        CMR_LOGD("no need to init isp %d ", sn_cxt->sensor_info.image_format);
        goto exit;
    }

    cxt->lsc_malloc_flag = 0;

    sensor_info_ptr = &(sn_cxt->sensor_info);
    CHECK_HANDLE_VALID(sensor_info_ptr);

    isp_param.setting_param_ptr = sensor_info_ptr->raw_info_ptr;
    if (0 != sensor_info_ptr->mode_info[SENSOR_MODE_COMMON_INIT].width) {
        isp_param.size.w =
            sensor_info_ptr->mode_info[SENSOR_MODE_COMMON_INIT].width;
        isp_param.size.h =
            sensor_info_ptr->mode_info[SENSOR_MODE_COMMON_INIT].height;
    } else {
        isp_param.size.w =
            sensor_info_ptr->mode_info[SENSOR_MODE_PREVIEW_ONE].width;
        isp_param.size.h =
            sensor_info_ptr->mode_info[SENSOR_MODE_PREVIEW_ONE].height;
    }
    isp_param.sensor_max_size.h = sensor_info_ptr->source_height_max;
    isp_param.sensor_max_size.w = sensor_info_ptr->source_width_max;
    isp_param.ops.flash_get_charge = camera_isp_flash_get_charge;
    isp_param.ops.flash_set_charge = camera_isp_flash_set_charge;
    isp_param.ops.flash_ctrl = camera_isp_flash_ctrl;
    isp_param.ops.flash_get_time = camera_isp_flash_get_time;
    isp_param.ops.flash_set_time = camera_isp_flash_set_time;

    isp_param.ops.set_pulse_line = camera_isp_set_pulse_line;
    isp_param.ops.set_next_vcm_pos = camera_isp_set_next_vcm_pos;
    isp_param.ops.set_pulse_log = camera_isp_set_pulse_log;

    isp_param.ctrl_callback = camera_isp_evt_cb;
    isp_param.oem_handle = oem_handle;
    isp_param.camera_id = cxt->camera_id;
    isp_param.facing = cxt->facing;
    isp_param.alloc_cb = camera_malloc;
    isp_param.free_cb = camera_free;

    camera_sensor_color_to_isp_color(&isp_param.image_pattern,
                                     sensor_info_ptr->image_pattern);

    p_grab = (struct cmr_grab *)(grab_cxt->grab_handle);
    if (NULL != p_grab)
        isp_param.dcam_fd = p_grab->fd;

    sns_ex_info_ptr = &sn_cxt->cur_sns_ex_info;
    if (sns_ex_info_ptr == NULL) {
        CMR_LOGE("sns_ex_info_ptr is null,it is impossible error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if ((NULL == sns_ex_info_ptr->name) ||
        (NULL == sns_ex_info_ptr->sensor_version_info)) {
        ret = cmr_sensor_init_static_info(cxt);
        if (ret) {
            cmr_sensor_deinit_static_info(cxt);
            CMR_LOGE("get sensor static info failed %ld", ret);
            goto exit;
        }
    } else {
        camera_copy_sensor_ex_info_to_isp(&isp_param.ex_info, sns_ex_info_ptr,
                                          cxt->camera_id);
    }

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sn_cxt->sensor_info.image_format) {
        isp_param.ex_info.preview_skip_num = 0;
        isp_param.ex_info.capture_skip_num = 0;
    }

    if ((NULL != sns_ex_info_ptr->name) &&
        (NULL != sns_ex_info_ptr->sensor_version_info)) {
        CMR_LOGD("sensorname=%s, version=%s", isp_param.ex_info.name,
                 isp_param.ex_info.sensor_version_info);
    } else {
        CMR_LOGE("fail to get static info");
    }

    property_get("persist.vendor.cam.pdaf.off", value, "0");
    if (atoi(value)) {
        isp_param.ex_info.pdaf_supported = 0;
    }

    if (isp_param.ex_info.pdaf_supported > SENSOR_PDAF_DISABLED &&
        isp_param.ex_info.pdaf_supported < SENSOR_PDAF_MAX) {

        val.type = SENSOR_VAL_TYPE_GET_PDAF_INFO;
        val.pval = &pdaf_info;
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                               SENSOR_ACCESS_VAL, (cmr_uint)&val);
        if (ret) {
            CMR_LOGE("get sensor pdaf info failed %ld", ret);
            goto exit;
        }
        isp_param.pdaf_info = &pdaf_info;
    }

    val.type = SENSOR_VAL_TYPE_READ_OTP;
    val.pval = NULL;
    ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                           SENSOR_ACCESS_VAL, (cmr_uint)&val);
    if (ret) {
        CMR_LOGE("get sensor otp failed %ld", ret);
        goto exit;
    }
    if (val.pval) {
        isp_param.otp_data = val.pval;
    }

    if (sensor_info_ptr->raw_info_ptr &&
        sensor_info_ptr->raw_info_ptr->ioctrl_ptr &&
        sensor_info_ptr->raw_info_ptr->ioctrl_ptr->set_focus) {
        isp_param.ex_info.af_supported = 1;
    } else {
        isp_param.ex_info.af_supported = 0;
    }

    property_get("persist.vendor.cam.ba.blur.version", value, "0");
    property_get("persist.vendor.cam.ba.portrait.enable", ba_portrait, "0");
    property_get("persist.vendor.cam.fr.portrait.enable", fr_portrait, "0");

    if (cxt->is_multi_mode == MODE_3D_CAPTURE ||
        cxt->is_multi_mode == MODE_3D_VIDEO ||
        cxt->is_multi_mode == MODE_3D_CALIBRATION ||
        cxt->is_multi_mode == MODE_BOKEH_CALI_GOLDEN ||
        cxt->is_multi_mode == MODE_3D_PREVIEW ||
        cxt->is_multi_mode == MODE_TUNING ||
        cxt->is_multi_mode == MODE_DUAL_FACEID_UNLOCK ||
        cxt->is_multi_mode == MODE_DUAL_FACEID_REGISTER) {
        isp_param.multi_mode = ISP_DUAL_NORMAL;
    } else if (cxt->is_multi_mode == MODE_BLUR && atoi(value) == 3) {
        isp_param.multi_mode = ISP_BLUR_REAR;
    } else if (cxt->is_multi_mode == MODE_BLUR &&
               (atoi(fr_portrait) == 1 || atoi(ba_portrait) == 1)) {
        isp_param.multi_mode = ISP_BLUR_PORTRAIT;
    } else if (cxt->is_multi_mode == MODE_SOFY_OPTICAL_ZOOM) {
        isp_param.multi_mode = ISP_WIDETELE;
    } else if (cxt->is_multi_mode == MODE_BOKEH) {
        isp_param.multi_mode = ISP_BOKEH;
    } else if (cxt->is_multi_mode == MODE_MULTI_CAMERA) {
        isp_param.multi_mode = ISP_WIDETELEULTRAWIDE;
    } else {
        isp_param.multi_mode = ISP_SINGLE;
    }

    if (cxt->is_multi_mode == MODE_DUAL_FACEID_UNLOCK ||
        cxt->is_multi_mode == MODE_SINGLE_FACEID_UNLOCK ||
        cxt->is_multi_mode == MODE_3D_FACEID_UNLOCK) {
        isp_param.is_faceId_unlock = 1;
    }

    // need modify,initialized by app,Similar to multi_mode
    if (cxt->camera_id == cxt->master_id)
        isp_param.is_master = 1;
    CMR_LOGI("multi_mode = %d, isp mode %d", cxt->is_multi_mode,
             isp_param.multi_mode);
#ifdef CONFIG_CAMERA_4IN1
    val.type = SENSOR_VAL_TYPE_GET_4IN1_INFO;
    val.pval = &sn_4in1_info;
    ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                           SENSOR_ACCESS_VAL, (cmr_uint)&val);
    if (ret) {
        CMR_LOGE("get sensor 4ini1 failed %ld", ret);
        goto exit;
    }
    if (isp_video_get_simulation_flag())
        cxt->sn_cxt.info_4in1.is_4in1_supported = 0;
    else
        cxt->sn_cxt.info_4in1.is_4in1_supported =
            sn_4in1_info.is_4in1_supported;
#endif

    isp_param.is_4in1_sensor = camera_get_is_4in1_sensor(&sn_4in1_info);

    CMR_LOGI(
        "multi_mode=%d, f_num=%d, focal_length=%d, max_fps=%d, "
        "max_adgain=%d, ois_supported=%d, pdaf_supported=%d, "
        "exp_valid_frame_num=%d, clamp_level=%d, adgain_valid_frame_num=%d, "
        "prev_skip_num=%d, cap_skip_num=%d, w=%d, h=%d, "
        "sensor_info_ptr->image_pattern=%d, isp_param.image_pattern=%d, "
        "is_4in1_sensor=%d, is_faceId_unlock=%d, long_expose_supported=%d",
        isp_param.multi_mode, isp_param.ex_info.f_num,
        isp_param.ex_info.focal_length, isp_param.ex_info.max_fps,
        isp_param.ex_info.max_adgain, isp_param.ex_info.ois_supported,
        isp_param.ex_info.pdaf_supported, isp_param.ex_info.exp_valid_frame_num,
        isp_param.ex_info.clamp_level, isp_param.ex_info.adgain_valid_frame_num,
        isp_param.ex_info.preview_skip_num, isp_param.ex_info.capture_skip_num,
        isp_param.size.w, isp_param.size.h, isp_param.image_pattern,
        sensor_info_ptr->image_pattern, isp_param.is_4in1_sensor,
        isp_param.is_faceId_unlock, isp_param.ex_info.long_expose_supported);

    ret = isp_init(&isp_param, &isp_cxt->isp_handle);
    if (ret) {
        CMR_LOGE("failed to init isp %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    ret = isp_capability(isp_cxt->isp_handle, ISP_VIDEO_SIZE, &isp_limit);
    if (ret) {
        CMR_LOGE("failed to get the limitation of isp %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        isp_deinit(isp_cxt->isp_handle);
        goto exit;
    }
    isp_cxt->width_limit = isp_limit.width;
    isp_cxt->inited = 1;
    CMR_LOGD("width_limit=%d", isp_cxt->width_limit);

exit:
    if (NULL != cali_result.data_ptr) {
        free(cali_result.data_ptr);
        cali_result.data_ptr = NULL;
    }

    ATRACE_END();
    return ret;
}

cmr_int camera_isp_deinit_notice(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_LOGV("E");

    ret = cmr_setting_cancel_notice_flash(cxt->setting_cxt.setting_handle);
    ret = cmr_focus_deinit_notice(cxt->focus_cxt.focus_handle);
    ret = cmr_grab_deinit_notice(cxt->grab_cxt.grab_handle);

    CMR_LOGV("X, ret=%ld", ret);

    return ret;
}

cmr_int camera_isp_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt;
    cmr_s32 fd = 0;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    isp_cxt = &cxt->isp_cxt;
    CHECK_HANDLE_VALID(isp_cxt);

    if (0 == isp_cxt->inited) {
        CMR_LOGD("isp has been de-intialized");
        goto exit;
    }

    ret = isp_deinit(isp_cxt->isp_handle);
    if (ret) {
        CMR_LOGE("failed to de-init isp %ld", ret);
        goto exit;
    }
    cmr_bzero(isp_cxt, sizeof(*isp_cxt));

#if defined(CONFIG_ISP_2_3)
    if (cxt->lsc_malloc_flag == 1) {
        if (cxt->hal_free) {
            cxt->hal_free(CAMERA_ISP_LSC, &cxt->isp_lsc_phys_addr,
                          &cxt->isp_lsc_virt_addr, &cxt->lsc_mfd,
                          ISP_LSC_BUF_NUM, cxt->client_data);
        }
        cxt->lsc_malloc_flag = 0;
    }
#endif

    if (cxt->is_real_bokeh) {
        // DEPTH
        if (cxt->swisp_depth_malloc_flag == 1) {
            cmr_uint depth_size = 960 * 720 * 3 / 2;
            cmr_uint depth_num = 1;
            if (cxt->hal_free) {
                cxt->hal_free(CAMERA_PREVIEW_DEPTH, &cxt->swisp_depth_phys_addr,
                              &cxt->swisp_depth_virt_addr,
                              &cxt->swisp_depth_mfd, depth_num,
                              cxt->client_data);
                cxt->swisp_depth_malloc_flag = 0;
            }
        }

        // SW_OUT
        if (cxt->swisp_out_malloc_flag == 1) {
            cmr_uint sw_out_size = 960 * 720 * 3 / 2;
            cmr_uint sw_out_num = 1;
            if (cxt->hal_free) {
                cxt->hal_free(CAMERA_PREVIEW_SW_OUT, &cxt->swisp_out_phys_addr,
                              &cxt->swisp_out_virt_addr, &cxt->swisp_out_mfd,
                              sw_out_num, cxt->client_data);
                cxt->swisp_out_malloc_flag = 0;
            }
        }
    }
    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_preview_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt = NULL;
    struct preview_init_param init_param;

    CHECK_HANDLE_VALID(oem_handle);
    prev_cxt = &cxt->prev_cxt;
    CHECK_HANDLE_VALID(prev_cxt);

    if (1 == prev_cxt->inited) {
        CMR_LOGD("preview has been intialized");
        goto exit;
    }
    init_param.oem_handle = oem_handle;
    init_param.ipm_handle = cxt->ipm_cxt.ipm_handle;
    init_param.ops.channel_cfg = camera_channel_cfg;
    init_param.ops.channel_start = camera_channel_start;
    init_param.ops.channel_pause = camera_channel_pause;
    init_param.ops.channel_resume = camera_channel_resume;
    init_param.ops.channel_free_frame = camera_channel_free_frame;
    init_param.ops.channel_stop = camera_channel_stop;
    init_param.ops.channel_buff_cfg = camera_channel_buff_cfg;
    init_param.ops.channel_cap_cfg = camera_channel_cap_cfg;
#ifdef CONFIG_CAMERA_OFFLINE
    init_param.ops.channel_dcam_size = camera_channel_dcam_size;
#endif
    init_param.ops.isp_start_video = camera_isp_start_video;
    init_param.ops.isp_stop_video = camera_isp_stop_video;
    init_param.ops.start_rot = camera_start_rot;
    init_param.ops.preview_pre_proc = camera_preview_pre_proc;
    init_param.ops.preview_post_proc = camera_preview_post_proc;
    init_param.ops.get_sensor_info = camera_get_sensor_info;
    init_param.ops.get_sensor_autotest_mode = camera_get_sensor_autotest_mode;
    init_param.ops.channel_scale_capability = camera_channel_scale_capability;
    init_param.ops.channel_path_capability = camera_channel_path_capability;
    init_param.ops.channel_get_cap_time = camera_channel_get_cap_time;
    init_param.ops.capture_pre_proc = camera_capture_pre_proc;
    init_param.ops.capture_post_proc = camera_capture_post_proc;
    init_param.ops.sensor_open = camera_open_sensor;
    init_param.ops.sensor_close = camera_close_sensor;
    init_param.ops.get_isp_yimg = camera_preview_get_isp_yimg;
    init_param.ops.set_preview_yimg = camera_preview_set_yimg_to_isp;
    init_param.ops.set_preview_yuv = camera_preview_set_yuv_to_isp;
    init_param.ops.get_sensor_fps_info = camera_get_sensor_fps_info;
    init_param.ops.get_sensor_otp = camera_get_otpinfo;
    init_param.ops.get_buff_handle = camera_get_buff_handle;
    init_param.ops.release_buff_handle = camera_release_buff_handle;
    init_param.ops.hdr_set_ev = camera_hdr_set_ev;
    init_param.ops.fdr_set_ev = camera_fdr_set_ev;
    init_param.ops.get_fdr_enable = camera_get_fdr_enable;
    init_param.ops.set_3dnr_ev = camera_3dnr_set_ev;
    init_param.ops.sw_3dnr_info_cfg = camera_sw_3dnr_info_cfg;
    init_param.ops.isp_ioctl = camera_isp_ioctl;
    init_param.oem_cb = camera_preview_cb;
    init_param.private_data = NULL;
    init_param.sensor_bits = (1 << cxt->camera_id);
    ret = cmr_preview_init(&init_param, &prev_cxt->preview_handle);
    if (ret) {
        CMR_LOGE("failed to init preview,ret %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
    prev_cxt->inited = 1;

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_preview_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt;

    CHECK_HANDLE_VALID(oem_handle);
    prev_cxt = &cxt->prev_cxt;
    CHECK_HANDLE_VALID(prev_cxt);

    if (0 == prev_cxt->inited) {
        CMR_LOGD("preview has been de-intialized");
        goto exit;
    }

    ret = cmr_preview_deinit(prev_cxt->preview_handle);
    if (ret) {
        CMR_LOGE("failed to de-init preview %ld", ret);
        goto exit;
    }
    cmr_bzero(prev_cxt, sizeof(*prev_cxt));

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_snapshot_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt = NULL;
    struct snapshot_init_param init_param;

    CHECK_HANDLE_VALID(oem_handle);
    snp_cxt = &cxt->snp_cxt;
    CHECK_HANDLE_VALID(snp_cxt);

    if (1 == snp_cxt->inited) {
        CMR_LOGD("snp has been intialized");
        goto exit;
    }
    init_param.id = cxt->camera_id;
    init_param.oem_handle = oem_handle;
    init_param.ipm_handle = cxt->ipm_cxt.ipm_handle;
    init_param.oem_cb = camera_snapshot_cb;
    init_param.ops.start_encode = camera_start_encode;
    init_param.ops.start_decode = camera_start_decode;
    init_param.ops.start_exif_encode = camera_start_exif_encode;
    init_param.ops.start_scale = camera_start_scale;
    init_param.ops.start_rot = camera_start_rot;
    init_param.ops.capture_pre_proc = camera_capture_pre_proc;
    init_param.ops.capture_post_proc = camera_capture_post_proc;
    init_param.ops.raw_proc = camera_raw_proc;
    init_param.ops.isp_start_video = camera_isp_start_video;
    init_param.ops.isp_stop_video = camera_isp_stop_video;
    init_param.ops.channel_start = camera_channel_start;
    init_param.ops.channel_pause = camera_channel_pause;
    init_param.ops.channel_resume = camera_channel_resume;
    init_param.ops.channel_free_frame = camera_channel_free_frame;
    init_param.ops.channel_stop = camera_channel_stop;
    init_param.ops.channel_buff_cfg = camera_channel_buff_cfg;
    init_param.ops.channel_cap_cfg = camera_channel_cap_cfg;
    init_param.ops.get_sensor_info = camera_get_sensor_info;
    init_param.ops.sensor_ioctl = camera_sensor_ioctl;
    init_param.ops.get_tuning_info = camera_get_tuning_info;
    init_param.ops.get_jpeg_param_info = camera_get_jpeg_param_info;
    init_param.ops.stop_codec = camera_stop_codec;
    init_param.ops.ipm_process = camera_ipm_process;
    init_param.ops.dump_image_with_3a_info = dump_image_with_3a_info;
    init_param.ops.get_cnr_realtime_flag = camera_get_cnr_realtime_flag;
    init_param.private_data = NULL;
    init_param.memory_setting.oem_handle = oem_handle;
    init_param.memory_setting.alloc_mem = camera_malloc;
    init_param.memory_setting.free_mem = camera_free;
    init_param.memory_setting.gpu_alloc_mem = camera_gpu_malloc;
    init_param.memory_setting.invalidate_cb = camera_invalidate_buf;
    init_param.memory_setting.flush_cb = camera_flush_buf;
    ret = cmr_snapshot_init(&init_param, &snp_cxt->snapshot_handle);
    if (ret) {
        CMR_LOGE("failed to init snapshot,ret %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
    snp_cxt->inited = 1;
    snp_cxt->start_capture_flag = 0;
    CMR_LOGD("default fdr tuing param to null");
    snp_cxt->fdr_tuning_param.param_ptr = NULL;
    snp_cxt->fdr_tuning_param.param_size = 0;

exit:
    CMR_LOGD("X,Snapshot:%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_snapshot_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    snp_cxt = &cxt->snp_cxt;
    CHECK_HANDLE_VALID(snp_cxt);

    if (0 == snp_cxt->inited) {
        CMR_LOGD("snp has been de-intialized");
        goto exit;
    }

    if (cxt->ipm_cxt.filter_inited) {
        ret = camera_close_filter(cxt);
        cxt->ipm_cxt.filter_inited = 0;
    }

    ret = cmr_snapshot_deinit(snp_cxt->snapshot_handle);
    if (ret) {
        CMR_LOGE("failed to de-init snapshot %ld", ret);
        goto exit;
    }
    cmr_bzero(snp_cxt, sizeof(*snp_cxt));

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_ipm_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct ipm_context *ipm_cxt = NULL;
    struct ipm_init_in init_param;

    CHECK_HANDLE_VALID(oem_handle);
    ipm_cxt = &cxt->ipm_cxt;
    CHECK_HANDLE_VALID(ipm_cxt);

    if (1 == ipm_cxt->inited) {
        CMR_LOGD("ipm has been intialized");
        goto exit;
    }

    init_param.oem_handle = oem_handle;
    init_param.sensor_id = cxt->camera_id;
    init_param.get_sensor_info = camera_get_sensor_info;
    init_param.ipm_sensor_ioctl = camera_sensor_ioctl;
    init_param.ipm_isp_ioctl = camera_isp_ioctl;
    init_param.ops.channel_reproc = camera_channel_reproc;
    init_param.ops.mem_malloc = camera_malloc;
    init_param.ops.mem_free = camera_free;
    init_param.ops.img_scale = camera_start_scale;
    ret = cmr_ipm_init(&init_param, &ipm_cxt->ipm_handle);
    if (ret) {
        CMR_LOGE("failed to init ipm,ret %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
    ipm_cxt->inited = 1;

exit:
    CMR_LOGD("X,done %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_ipm_open_sw_algorithm(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;
    struct img_size pic_size;
    cmr_int ret = CMR_CAMERA_SUCCESS;

    cmr_bzero(&in_param, sizeof(struct ipm_open_in));
    cmr_bzero(&out_param, sizeof(struct ipm_open_out));

    in_param.frame_rect.start_x = 0;
    in_param.frame_rect.start_y = 0;
    in_param.reg_cb = camera_ipm_cb;

    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                            &setting_param);
    if (ret)
        CMR_LOGE("failed to get app mode");
    if(setting_param.cmd_type_value == CAMERA_MODE_NIGHT_PHOTO &&
        camera_get_3dnr_flag(cxt) != CAMERA_3DNR_TYPE_NIGHT_DNS) {
          if (cxt->night_cxt.is_authorized) {
            ret = cxt->night_cxt.sw_open(cxt);
            cxt->night_cxt.mfnr_on_off = 1;
            if (ret)
                CMR_LOGE("failed to open sw %ld", ret);
        }
        return ret;
    }

    if(camera_get_fdr_flag(cxt)) {
         ret = camera_open_fdr(cxt);
          if(!ret) {
             CMR_LOGV("fdr total frame cnt %d", cxt->fdr_total_frame_cnt);
             return ret;
          }
          CMR_LOGE("failed to open fdr %ld, fail through to non-fdr capture", ret);
          camera_set_fdr_flag(cxt, 0);
    }

    if (1 == camera_get_hdr_flag(cxt)) {
        if (cxt->is_high_res_mode || cxt->multicam_highres_mode) {
            in_param.frame_size.width = cxt->snp_cxt.request_size.width / 2;
            in_param.frame_size.height = cxt->snp_cxt.request_size.height / 2;
        } else {
            in_param.frame_size.width = cxt->snp_cxt.request_size.width;
            in_param.frame_size.height = cxt->snp_cxt.request_size.height;
        }
        CMR_LOGD("hdr %d, %d", in_param.frame_size.width, in_param.frame_size.height);
        in_param.frame_rect.width = in_param.frame_size.width;
        in_param.frame_rect.height = in_param.frame_size.height;
        in_param.adgain_valid_frame_num =
            cxt->sn_cxt.cur_sns_ex_info.adgain_valid_frame_num;
        ret = camera_open_hdr(cxt, &in_param, &out_param);
        if (ret) {
            CMR_LOGE("failed to open hdr %ld", ret);
            return ret;
        }
        cxt->ipm_cxt.hdr_num = out_param.total_frame_number;
        cxt->ipm_cxt.hdr_version.major = out_param.version.major;
        CMR_LOGD("get hdr num %d", cxt->ipm_cxt.hdr_num);
    }

    if (cxt->snp_cxt.is_mfsr) {
        ret = camera_open_mfsr(cxt, &in_param, &out_param);
        if (ret) {
            CMR_LOGE("fail to open mfsr\n");
        }
    }

    if (camera_get_filter_flag(cxt) && !cxt->ipm_cxt.filter_inited) {
        in_param.frame_size.width = cxt->snp_cxt.request_size.width;
        in_param.frame_size.height = cxt->snp_cxt.request_size.height;
        in_param.frame_rect.start_x = 0;
        in_param.frame_rect.start_y = 0;
        in_param.frame_rect.width = in_param.frame_size.width;
        in_param.frame_rect.height = in_param.frame_size.height;
        in_param.reg_cb = camera_ipm_cb;
        ret = camera_open_filter(cxt, &in_param, &out_param);
        if (ret)
            CMR_LOGE("failed to open filter %ld", ret);
        else
            cxt->ipm_cxt.filter_inited = 1;
    }

    if (1 != cxt->is_3dnr_video &&
        (camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
         camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
         camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_PREV_NULL_CAP_SW ||
         camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_NIGHT_DNS)) {
        struct isp_adgain_exp_info adgain_exp_info;
        if (cxt->is_high_res_mode || cxt->multicam_highres_mode) {
            in_param.frame_size.width = cxt->snp_cxt.request_size.width / 2;
            in_param.frame_size.height = cxt->snp_cxt.request_size.height / 2;
        } else {
            in_param.frame_size.width = cxt->snp_cxt.request_size.width;
            in_param.frame_size.height = cxt->snp_cxt.request_size.height;
        }
        CMR_LOGD("3dnr %d, %d", in_param.frame_size.width, in_param.frame_size.height);
        in_param.frame_rect.width = in_param.frame_size.width;
        in_param.frame_rect.height = in_param.frame_size.height;
        in_param.reg_cb = camera_ipm_cb;
        in_param.adgain = 16;
        in_param.af_ctrl_roi.start_x = cxt->af_roi.sx;
        in_param.af_ctrl_roi.start_y = cxt->af_roi.sy;
        in_param.af_ctrl_roi.width = cxt->af_roi.ex - cxt->af_roi.sx;
        in_param.af_ctrl_roi.height = cxt->af_roi.ey - cxt->af_roi.sy;
        
        ret = camera_get_tuning_info((cmr_handle)cxt, &adgain_exp_info);
        if (ret) {
            CMR_LOGE("failed to get gain %ld, and using default gain", ret);
            return ret;
        } else {
            in_param.adgain = adgain_exp_info.adgain / 128;
        }
        CMR_LOGD("SW 3DRN, Get Gain from ISP: %d", in_param.adgain);

        if (camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_NIGHT_DNS) {
            ret = camera_open_night_dns(cxt, &in_param, &out_param);
        } else {
            ret = camera_open_3dnr(cxt, &in_param, &out_param);
        }
        if (ret) {
            CMR_LOGE("failed to open 3dnr %ld", ret);
            return ret;
        }
        cxt->ipm_cxt.threednr_num = out_param.total_frame_number;
        CMR_LOGD("get 3dnr num %d", cxt->ipm_cxt.threednr_num);
    }

    if (camera_get_cnr_flag(oem_handle) && !cxt->ipm_cxt.cnr_inited) {
        ret = camera_open_cnr(cxt, NULL, NULL);
        if (ret) {
            CMR_LOGE("failed to open cnr %ld", ret);
            return ret;
        }
        cxt->ipm_cxt.cnr_inited = 1;
    }

#ifdef CONFIG_CAMERA_DRE_PRO
    CMR_LOGD("dre_inited = %ld, dre_flag = %ld, camera_get_hdr_flag = %d",
                        cxt->ipm_cxt.dre_inited, cxt->dre_flag, camera_get_hdr_flag(cxt));
    if (!cxt->ipm_cxt.dre_inited && cxt->dre_flag && (1 != camera_get_hdr_flag(cxt))) {
        in_param.frame_size.width = cxt->sn_cxt.sensor_info.source_width_max;
        in_param.frame_size.height = cxt->sn_cxt.sensor_info.source_height_max;
        ret = camera_open_dre(cxt, &in_param, NULL);
        cxt->ipm_cxt.dre_inited = 1;
    }
#endif

    return ret;
}

cmr_int camera_ipm_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct ipm_context *ipm_cxt;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    ipm_cxt = &cxt->ipm_cxt;
    CHECK_HANDLE_VALID(ipm_cxt);

    camera_close_4in1(oem_handle);

    if (cxt->ipm_cxt.cnr_inited) {
        ret = camera_close_cnr(cxt);
        if (ret) {
            CMR_LOGE("failed to close cnr");
        }
        cxt->ipm_cxt.cnr_inited = 0;
    }

#ifdef CONFIG_CAMERA_DRE_PRO
    if (cxt->ipm_cxt.dre_inited && cxt->dre_flag) {
        ret = camera_close_dre(cxt);
        if (ret) {
            CMR_LOGE("failed to close cnr");
        }
        cxt->ipm_cxt.dre_inited = 0;
    }
#endif

    camera_close_mfsr(cxt);

    if (0 == ipm_cxt->inited) {
        CMR_LOGD("ipm has been de-intialized");
        goto exit;
    }

    ret = cmr_ipm_deinit(ipm_cxt->ipm_handle);
    if (ret) {
        CMR_LOGE("failed to de-init ipm %ld", ret);
        goto exit;
    }
    cmr_bzero(ipm_cxt, sizeof(*ipm_cxt));

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_setting_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = NULL;
    struct setting_init_in init_param;

    CHECK_HANDLE_VALID(oem_handle);
    setting_cxt = &cxt->setting_cxt;
    CHECK_HANDLE_VALID(setting_cxt);

    if (1 == setting_cxt->inited) {
        CMR_LOGD("setting has been de-intialized");
        goto exit;
    }

    init_param.oem_handle = oem_handle;
    init_param.camera_id_bits = (1 << cxt->camera_id);
    init_param.io_cmd_ioctl = camera_ioctl_for_setting;
    init_param.setting_sn_ioctl = camera_sensor_ioctl;
    init_param.setting_isp_ioctl = camera_isp_ioctl;
    init_param.get_setting_activity = camera_get_setting_activity;
    init_param.before_set_cb = camera_before_set;
    init_param.after_set_cb = camera_after_set;
    init_param.padding = 0;
    ret = cmr_setting_init(&init_param, &setting_cxt->setting_handle);
    if (ret) {
        CMR_LOGE("failed to init setting %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
    setting_cxt->inited = 1;

exit:
    CMR_LOGD("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_ipm_process(cmr_handle oem_handle, void *data) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct ipm_context *ipm_cxt;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out imp_out_param;
    struct img_frm *img_frame = (struct img_frm *)data;
    struct setting_context *setting_cxt;
    struct setting_cmd_parameter setting_param;
    cmr_uint is_filter;

    CMR_LOGI("E");
    CHECK_HANDLE_VALID(oem_handle);
    CHECK_HANDLE_VALID(data);

    ipm_cxt = &cxt->ipm_cxt;
    setting_cxt = &cxt->setting_cxt;

    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                            &setting_param);
    if (ret)
        CMR_LOGE("failed to get app mode");
    if(setting_param.cmd_type_value == CAMERA_MODE_NIGHT_PHOTO &&
        cxt->night_cxt.is_authorized && cxt->night_cxt.mfnr_on_off) {
        cxt->night_cxt.mfnr_on_off = 0;
        ret = cxt->night_cxt.sw_close(cxt);
        ret = cxt->night_cxt.ipmpro_process(oem_handle, data);
        if(ret) {
            CMR_LOGE("failed to process ipmpro %ld",ret);
        }
        goto exit;
    }

    if (camera_get_fdr_flag(cxt)) {
	CMR_LOGD("no ipm process for fdr\n");
	return ret;
    }

    is_filter = cxt->snp_cxt.filter_type;
    if (cxt->skipframe) {
        camera_snapshot_set_ev(oem_handle, 0, SNAPSHOT_NULL);
        cxt->skipframe = 0;
        CMR_LOGD("set ev to 0 and reset skipframe");
    }

    if ((cxt->nr_flag && cxt->is_multi_mode != MODE_BOKEH && cxt->is_multi_mode != MODE_BLUR) ||
        is_filter || cxt->dre_flag || cxt->ee_flag || cxt->snp_cxt.is_mfsr) {
        cmr_bzero(&ipm_in_param, sizeof(ipm_in_param));
        cmr_bzero(&imp_out_param, sizeof(imp_out_param));

        ipm_cxt->frm_num++;
        ipm_in_param.src_frame = *img_frame;
        ipm_in_param.private_data = (void *)cxt;
        imp_out_param.dst_frame = *img_frame;

        // do cnr
        if (cxt->nr_flag)
            ret = ipm_transfer_frame(ipm_cxt->cnr_handle, &ipm_in_param, NULL);
        if (ret) {
            CMR_LOGE("failed to do cnr process %ld", ret);
            goto exit;
        }

        CMR_LOGD("in frame[%d,%d],fd 0x%x,vir[0x%x,0x%x]",
                 imp_out_param.dst_frame.size.width,
                 imp_out_param.dst_frame.size.height,
                 imp_out_param.dst_frame.fd,
                 imp_out_param.dst_frame.addr_vir.addr_y,
                 imp_out_param.dst_frame.addr_vir.addr_u);
        if (cxt->ee_flag) {
            ret = ipm_transfer_frame(ipm_cxt->ee_handle, &ipm_in_param, NULL);
            if (ret) {
                CMR_LOGE("failed to do ee process %ld", ret);
                goto exit;
            }
        }

        /* mfsr post-processing */
        if (cxt->snp_cxt.is_mfsr) {
            struct img_frm *detail_frm = (struct img_frm *)img_frame->reserved;
            ipm_in_param.src_frame.reserved = detail_frm->reserved;
            detail_frm->reserved = NULL;
            ipm_in_param.dst_frame = *detail_frm;
            ret = ipm_transfer_frame(ipm_cxt->mfsr_post_handle, &ipm_in_param, NULL);
            if (ret)
                CMR_LOGE("failed to do mfsr post process %ld", ret);
        }

        // do dre
        if (cxt->dre_flag) {
            // do dre except burst mode
            struct setting_cmd_parameter setting_param;
            struct setting_context *setting_cxt = &cxt->setting_cxt;
            cmr_uint app_mode;

            cmr_bzero(&setting_param, sizeof(setting_param));
            setting_param.camera_id = cxt->camera_id;
            ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                    SETTING_GET_APPMODE, &setting_param);
            if (ret) {
                CMR_LOGE("failed to get app mode %ld", ret);
                goto exit;
            }
            app_mode = setting_param.cmd_type_value;
            if (app_mode != CAMERA_MODE_CONTINUE) {
                ret = ipm_transfer_frame(ipm_cxt->dre_handle, &ipm_in_param,
                                         NULL);
                if (ret) {
                    CMR_LOGE("failed to do dre process %ld", ret);
                    goto exit;
                }
            }
        }

        // do filter:
        if (is_filter) {
            struct setting_cmd_parameter setting_param;
            struct setting_context *setting_cxt = &cxt->setting_cxt;

            /*get orientation*/
            cmr_bzero(&setting_param, sizeof(setting_param));
            setting_param.camera_id = cxt->camera_id;
            ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                    CAMERA_PARAM_GET_SENSOR_ORIENTATION,
                                    &setting_param);
            if (ret) {
                CMR_LOGE("failed to get enc rotation %ld", ret);
            }
            ipm_in_param.orientation = setting_param.cmd_type_value;
            /*get flip*/
            cmr_bzero(&setting_param, sizeof(setting_param));
            setting_param.camera_id = cxt->camera_id;
            ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                    SETTING_GET_FLIP_ON, &setting_param);
            if (ret) {
                CMR_LOGE("failed to get flip %ld", ret);
            }
            ipm_in_param.flip_on = setting_param.cmd_type_value;
            imp_out_param.private_data = (void *)(cxt->snp_cxt.filter_type);
            ret = ipm_transfer_frame(ipm_cxt->filter_handle, &ipm_in_param,
                                     &imp_out_param);
        }

        cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle, img_frame);
        if (ret) {
            CMR_LOGE("failed to do filter process %ld", ret);
            goto exit;
        }
    }
exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int camera_setting_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    setting_cxt = &cxt->setting_cxt;
    CHECK_HANDLE_VALID(setting_cxt);

    if (0 == setting_cxt->inited) {
        CMR_LOGD("setting has been de-intialized");
        goto exit;
    }

    ret = cmr_setting_deinit(setting_cxt->setting_handle);
    if (ret) {
        CMR_LOGE("failed to de-init setting %ld", ret);
        goto exit;
    }
    cmr_bzero(setting_cxt, sizeof(*setting_cxt));

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_focus_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct focus_context *focus_cxt;
    struct af_init_param init_param;

    CHECK_HANDLE_VALID(oem_handle);
    focus_cxt = &cxt->focus_cxt;
    CHECK_HANDLE_VALID(focus_cxt);

    if (1 == focus_cxt->inited) {
        CMR_LOGD("focus has been intialized");
        goto exit;
    }
    init_param.oem_handle = oem_handle;
    init_param.evt_cb = camera_focus_evt_cb;
    init_param.ops.af_pre_proc = camera_focus_pre_proc;
    init_param.ops.af_post_proc = camera_focus_post_proc;
    init_param.ops.af_isp_ioctrl = camera_isp_ioctl;
    init_param.ops.get_preview_status = camera_get_preview_status;
    init_param.ops.af_sensor_ioctrl = camera_sensor_ioctl;
    init_param.ops.get_sensor_info = camera_get_sensor_info;
    init_param.ops.get_flash_info = camera_focus_visit_flash_info;

    ret = cmr_focus_init(&init_param, cxt->camera_id, &focus_cxt->focus_handle);
    if (ret) {
        CMR_LOGE("failed to init focus,ret %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
    focus_cxt->inited = 1;

exit:
    CMR_LOGD("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_focus_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct focus_context *focus_cxt;

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(oem_handle);
    focus_cxt = &cxt->focus_cxt;
    CHECK_HANDLE_VALID(focus_cxt);

    if (0 == focus_cxt->inited) {
        CMR_LOGD("focus has been de-intialized");
        goto exit;
    }

    ret = cmr_focus_deinit(focus_cxt->focus_handle);
    if (ret) {
        CMR_LOGE("failed to de-init focus %ld", ret);
        goto exit;
    }
    cmr_bzero(focus_cxt, sizeof(*focus_cxt));

    CMR_LOGD("X");

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_preview_cb_thread_proc(struct cmr_msg *message, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)data;
    camera_cb_of_type callback;
    struct setting_cmd_parameter setting_param;
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    if (!message || !data) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGV("msg_type 0x%x, sub msg type 0x%x client_data 0x%lx",
             message->msg_type, message->sub_msg_type,
             (cmr_uint)cxt->client_data);

    if (cxt->video_face_beauty_en) {
        struct camera_frame_type *preview_frame =
            (struct camera_frame_type *)message->data;
        cmr_bzero(&setting_param, sizeof(setting_param));
        setting_param.camera_id = cxt->camera_id;
        ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                SETTING_GET_APPMODE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get app mode %ld", ret);
            goto exit;
        }

        if ((cxt->ref_camera_id == cxt->camera_id ||
             cxt->is_multi_mode == MODE_SINGLE_CAMERA) &&
            (preview_frame) && (preview_frame->y_vir_addr) &&
            (setting_param.cmd_type_value == CAMERA_MODE_AUTO_VIDEO)) {
            ret = camera_preview_face_beauty_handle(data, preview_frame);
        }
        if (ret) {
            CMR_LOGE("failed to preview face beauty %ld", ret);
            goto exit;
        }
    }
    callback = cxt->camera_cb;
    /*CMR_PRINT_TIME;*/
    if (callback) {
        callback(message->sub_msg_type, cxt->client_data, message->msg_type,
                 message->data);
    } else {
        CMR_LOGE("err, camera cb is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
/*CMR_PRINT_TIME;*/
exit:
    CMR_LOGV("X,ret=%ld", ret);
    return ret;
}

cmr_int is_video_face_beauty_on(void *data) {
#ifdef CONFIG_FACE_BEAUTY
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int face_beauty_on = 0;
    int SPRD_FACE_BEAUTY_PARAM_NUM = 9;
    cmr_s32 perfect_skin_level[SPRD_FACE_BEAUTY_PARAM_NUM];
    struct camera_context *cxt = (struct camera_context *)data;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    struct faceBeautyLevels beautyLevels;
    cmr_bzero((void *)&setting_param, sizeof(setting_param));
    memset(&beautyLevels, 0, sizeof(struct faceBeautyLevels));
    setting_param.camera_id = cxt->camera_id;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PERFECT_SKINLEVEL, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get perfect skinlevel %ld", ret);
        goto exit;
    }

    perfect_skin_level[0] = setting_param.fb_param.blemishLevel;
    perfect_skin_level[1] = setting_param.fb_param.smoothLevel;
    perfect_skin_level[2] = setting_param.fb_param.skinColor;
    perfect_skin_level[3] = setting_param.fb_param.skinLevel;
    perfect_skin_level[4] = setting_param.fb_param.brightLevel;
    perfect_skin_level[5] = setting_param.fb_param.lipColor;
    perfect_skin_level[6] = setting_param.fb_param.lipLevel;
    perfect_skin_level[7] = setting_param.fb_param.slimLevel;
    perfect_skin_level[8] = setting_param.fb_param.largeLevel;

    for (int i = 0; i < SPRD_FACE_BEAUTY_PARAM_NUM; i++) {
        if (perfect_skin_level[i] != 0)
            return ret;
    }
    return CMR_CAMERA_FAIL;
exit:
    CMR_LOGV("X,ret=%ld", ret);
    return ret;
#else
    return CMR_CAMERA_FAIL;
#endif
}

/*
    when video face beauty enable,do the preview and video frame face beauty.
    parameters:
    name is preview or video
*/
#ifdef CONFIG_FACE_BEAUTY
void camera_frame_facebeauty(void *data, struct camera_frame_type *frame,
                            struct faceBeautyLevels *beautyLevels, char *name) {
    struct camera_context *cxt = (struct camera_context *)data;
    fb_beauty_image_t beauty_image;
    fbBeautyFacetT beauty_face;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int facecount = cxt->fd_face_area.face_num;

    for (int i = 0; i < facecount; i++) {
         beauty_face.idx = i;
         beauty_face.startX =
                     (cxt->fd_face_area.face_info[i].sx * frame->width) /
                     (cxt->fd_face_area.frame_width);
         beauty_face.startY =
                     (cxt->fd_face_area.face_info[i].sy * frame->height) /
                     (cxt->fd_face_area.frame_height);
         beauty_face.endX =
                     (cxt->fd_face_area.face_info[i].ex * frame->width) /
                     (cxt->fd_face_area.frame_width);
         beauty_face.endY =
                     (cxt->fd_face_area.face_info[i].ey * frame->height) /
                     (cxt->fd_face_area.frame_height);
         beauty_face.angle = cxt->fd_face_area.face_info[i].angle;
         beauty_face.pose = cxt->fd_face_area.face_info[i].pose;
         if (!strcmp(name, "prev")) {
             ret = face_beauty_ctrl(&(cxt->prev_face_beauty),
                                    FB_BEAUTY_CONSTRUCT_FACE_CMD,
                                    (void *)&beauty_face);
         } else {
            ret = face_beauty_ctrl(&(cxt->video_face_beauty),
                                   FB_BEAUTY_CONSTRUCT_FACE_CMD,
                                   (void *)&beauty_face);
         }
    }

    if (!(cxt->mflagfb) && !strcmp(name, "prev")) {
        face_beauty_set_devicetype(&(cxt->prev_face_beauty),
                                   SPRD_CAMALG_RUN_TYPE_CPU);
        fb_chipinfo chipinfo = SHARKLE;
#if defined(CONFIG_ISP_2_3)
        chipinfo = SHARKLE;
#elif defined(CONFIG_ISP_2_4)
        chipinfo = PIKE2;
#elif defined(CONFIG_ISP_2_5)
        chipinfo = SHARKL3;
#elif defined(CONFIG_ISP_2_6)
        chipinfo = SHARKL5;
#elif defined(CONFIG_ISP_2_7)
        chipinfo = SHARKL5PRO;
#endif
        face_beauty_init(&(cxt->prev_face_beauty), 1, 2, chipinfo);
        if (cxt->prev_face_beauty.hSprdFB != NULL)
            cxt->mflagfb = true;
    }

    if (!(cxt->mvideofb) && !strcmp(name, "video")) {
        face_beauty_set_devicetype(&(cxt->video_face_beauty),
                                   SPRD_CAMALG_RUN_TYPE_VDSP);
        fb_chipinfo chipinfo = SHARKLE;
#if defined(CONFIG_ISP_2_3)
        chipinfo = SHARKLE;
#elif defined(CONFIG_ISP_2_4)
        chipinfo = PIKE2;
#elif defined(CONFIG_ISP_2_5)
        chipinfo = SHARKL3;
#elif defined(CONFIG_ISP_2_6)
        chipinfo = SHARKL5;
#elif defined(CONFIG_ISP_2_7)
        chipinfo = SHARKL5PRO;
#endif

        face_beauty_init(&(cxt->video_face_beauty), 1, 2, chipinfo);
        if (cxt->video_face_beauty.hSprdFB != NULL) {
            cxt->mvideofb = true;
        }
    }

    camera_invalidate_buf(data, frame->fd,
                          frame->width * frame->height * 3 / 2,
                          0, frame->y_vir_addr);
    beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
    beauty_image.inputImage.addr[0] = (void *)frame->y_vir_addr;
    beauty_image.inputImage.addr[1] = (void *)frame->uv_vir_addr;
    beauty_image.inputImage.addr[2] = (void *)frame->uv_vir_addr;
    beauty_image.inputImage.ion_fd = frame->fd;
    beauty_image.inputImage.offset[0] = 0;
    beauty_image.inputImage.offset[1] = frame->width * frame->height;
    beauty_image.inputImage.width = frame->width;
    beauty_image.inputImage.height = frame->height;
    beauty_image.inputImage.stride = frame->width;
    beauty_image.inputImage.size = frame->width * frame->height * 3 / 2;
    if (!strcmp(name, "prev")) {
        ret = face_beauty_ctrl(&(cxt->prev_face_beauty),
                               FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                               (void *)&beauty_image);
        ret = face_beauty_ctrl(&(cxt->prev_face_beauty),
                               FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                               beautyLevels);
        ret = face_beauty_ctrl(&(cxt->prev_face_beauty), FB_BEAUTY_PROCESS_CMD,
                               (void *)&facecount);
     } else {
        ret = face_beauty_ctrl(&(cxt->video_face_beauty),
                               FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                               (void *)&beauty_image);
        ret = face_beauty_ctrl(&(cxt->video_face_beauty),
                               FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                               beautyLevels);
        ret = face_beauty_ctrl(&(cxt->video_face_beauty), FB_BEAUTY_PROCESS_CMD,
                               (void *)&facecount);
    }
    camera_flush_buf(data, frame->fd,
                               frame->width * frame->height * 3 / 2, 0,
                               frame->y_vir_addr);
}
#endif

/*
    when video face beauty enable,the preview frame is handled.
*/
cmr_int camera_preview_face_beauty_handle(void *data,
                                  struct camera_frame_type *preview_frame) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
#ifdef CONFIG_FACE_BEAUTY
    struct camera_context *cxt = (struct camera_context *)data;

    int face_beauty_on = 0;
    int facecount = cxt->fd_face_area.face_num;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    struct faceBeautyLevels beautyLevels;
    struct common_isp_cmd_param isp_param;
    memset(&beautyLevels, 0, sizeof(struct faceBeautyLevels));
    cmr_bzero((void *)&setting_param, sizeof(setting_param));

    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PERFECT_SKINLEVEL, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get perfect skinlevel %ld", ret);
        goto exit;
    }
    ret = camera_isp_ioctl(data, COM_ISP_GET_CUR_COL_TEM, &isp_param);
    ret = camera_isp_ioctl(data, COM_ISP_GET_CUR_SENS, &isp_param);
    ret = camera_isp_ioctl(data, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    if (ret) {
        CMR_LOGE("failed to get camera BV %ld", ret);
        goto exit;
    }

    beautyLevels.blemishLevel =
        (unsigned char)setting_param.fb_param.blemishLevel;
    beautyLevels.smoothLevel =
        (unsigned char)setting_param.fb_param.smoothLevel;
    beautyLevels.skinColor = (unsigned char)setting_param.fb_param.skinColor;
    beautyLevels.skinLevel = (unsigned char)setting_param.fb_param.skinLevel;
    beautyLevels.brightLevel =
        (unsigned char)setting_param.fb_param.brightLevel;
    beautyLevels.lipColor = (unsigned char)setting_param.fb_param.lipColor;
    beautyLevels.lipLevel = (unsigned char)setting_param.fb_param.lipLevel;
    beautyLevels.slimLevel = (unsigned char)setting_param.fb_param.slimLevel;
    beautyLevels.largeLevel = (unsigned char)setting_param.fb_param.largeLevel;
    beautyLevels.cameraBV = (int)isp_param.isp_adgain.bv;
    beautyLevels.cameraCT = (int)isp_param.isp_cur_ct;
    beautyLevels.cameraISO = (int)isp_param.isp_cur_iso;
    beautyLevels.cameraWork = (int)cxt->camera_id;

    face_beauty_on = beautyLevels.blemishLevel || beautyLevels.smoothLevel ||
                     beautyLevels.skinColor || beautyLevels.skinLevel ||
                     beautyLevels.brightLevel || beautyLevels.lipColor ||
                     beautyLevels.lipLevel || beautyLevels.slimLevel ||
                     beautyLevels.largeLevel;
    CMR_LOGV(" face_beauty_on %d facecount %d", face_beauty_on, facecount);

    if (face_beauty_on &&
       (PREVIEWING == cmr_preview_get_status(cxt->prev_cxt.preview_handle,
        cxt->camera_id)) && (preview_frame->type == PREVIEW_FRAME)) {
        char value[6] = "prev";
        camera_frame_facebeauty(data, preview_frame, &beautyLevels, value);
    } else {
        if (cxt->mflagfb) {
            cxt->mflagfb = false;
            ret = face_beauty_ctrl(&cxt->prev_face_beauty,
                                   FB_BEAUTY_FAST_STOP_CMD,NULL);
            face_beauty_deinit(&cxt->prev_face_beauty);
        }
    }
exit:
    CMR_LOGV("X,ret=%ld", ret);
#endif
    return ret;
}

/*
    when video face beauty enable,the video frame is handled.
*/
cmr_int camera_video_face_beauty_handle(void *data,
                                        struct camera_frame_type *video_frame) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
#ifdef CONFIG_FACE_BEAUTY
    struct camera_context *cxt = (struct camera_context *)data;

    int face_beauty_on = 0;
    int facecount = cxt->fd_face_area.face_num;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    struct faceBeautyLevels beautyLevels;
    struct common_isp_cmd_param isp_param;
    memset(&beautyLevels, 0, sizeof(struct faceBeautyLevels));
    cmr_bzero((void *)&setting_param, sizeof(setting_param));

    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PERFECT_SKINLEVEL, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get perfect skinlevel %ld", ret);
        goto exit;
    }
    ret = camera_isp_ioctl(data, COM_ISP_GET_CUR_COL_TEM, &isp_param);
    ret = camera_isp_ioctl(data, COM_ISP_GET_CUR_SENS, &isp_param);
    ret = camera_isp_ioctl(data, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    if (ret) {
        CMR_LOGE("failed to get camera BV %ld", ret);
        goto exit;
    }

    beautyLevels.blemishLevel =
        (unsigned char)setting_param.fb_param.blemishLevel;
    beautyLevels.smoothLevel =
        (unsigned char)setting_param.fb_param.smoothLevel;
    beautyLevels.skinColor = (unsigned char)setting_param.fb_param.skinColor;
    beautyLevels.skinLevel = (unsigned char)setting_param.fb_param.skinLevel;
    beautyLevels.brightLevel =
        (unsigned char)setting_param.fb_param.brightLevel;
    beautyLevels.lipColor = (unsigned char)setting_param.fb_param.lipColor;
    beautyLevels.lipLevel = (unsigned char)setting_param.fb_param.lipLevel;
    beautyLevels.slimLevel = (unsigned char)setting_param.fb_param.slimLevel;
    beautyLevels.largeLevel = (unsigned char)setting_param.fb_param.largeLevel;
    beautyLevels.cameraBV = (int)isp_param.isp_adgain.bv;
    beautyLevels.cameraCT = (int)isp_param.isp_cur_ct;
    beautyLevels.cameraISO = (int)isp_param.isp_cur_iso;
    beautyLevels.cameraWork = (int)cxt->camera_id;

    face_beauty_on = beautyLevels.blemishLevel || beautyLevels.smoothLevel ||
                     beautyLevels.skinColor || beautyLevels.skinLevel ||
                     beautyLevels.brightLevel || beautyLevels.lipColor ||
                     beautyLevels.lipLevel || beautyLevels.slimLevel ||
                     beautyLevels.largeLevel;
    CMR_LOGV(" face_beauty_on %d,facecount %d", face_beauty_on, facecount);

    if (face_beauty_on && (cxt->start_video_face_beauty)) {
        char value[6] = "video";
        camera_frame_facebeauty(data, video_frame, &beautyLevels, value);
    } else {
        if (cxt->mvideofb) {
            cxt->mvideofb = false;
            cxt->start_video_face_beauty = false;
            ret = face_beauty_ctrl(&cxt->video_face_beauty,
                                   FB_BEAUTY_FAST_STOP_CMD,NULL);
            face_beauty_deinit(&cxt->video_face_beauty);
        }
    }
exit:
    CMR_LOGV("X,ret=%ld", ret);
#endif
    return ret;
}

cmr_int camera_video_cb_thread_proc(struct cmr_msg *message, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
#ifdef CONFIG_FACE_BEAUTY
    struct camera_context *cxt = (struct camera_context *)data;
    camera_cb_of_type callback;

    if (!message || !data) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGV("msg_type 0x%x, sub msg type 0x%x client_data 0x%lx",
             message->msg_type, message->sub_msg_type,
             (cmr_uint)cxt->client_data);
    struct camera_frame_type *video_frame =
        (struct camera_frame_type *)message->data;
    if ((cxt->ref_camera_id == cxt->camera_id ||
         cxt->is_multi_mode == MODE_SINGLE_CAMERA) &&
        (video_frame) && (video_frame->y_vir_addr)) {
        ret = camera_video_face_beauty_handle(data, video_frame);
    }
    if (ret) {
        CMR_LOGE("failed to video face beauty%ld", ret);
        goto exit;
    }
    callback = cxt->camera_cb;
    /*CMR_PRINT_TIME;*/
    if (callback) {
        callback(message->sub_msg_type, cxt->client_data, message->msg_type,
                 message->data);
    } else {
        CMR_LOGE("err, camera cb is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
/*CMR_PRINT_TIME;*/
exit:
    CMR_LOGV("X,ret=%ld", ret);
#endif
    return ret;
}

cmr_int camera_snapshot_cb_thread_proc(struct cmr_msg *message, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)data;
    camera_cb_of_type callback;

    if (!message || !data) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGV("msg_type 0x%x, sub msg type 0x%x", message->msg_type,
             message->sub_msg_type);

    callback = cxt->camera_cb;
    if (callback) {
        callback(message->sub_msg_type, cxt->client_data, message->msg_type,
                 message->data);
    } else {
        CMR_LOGE("err, camera cb is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }

exit:
    return ret;
}

cmr_int camera_snapshot_secondary_thread_proc(struct cmr_msg *message,
                                              void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)data;
    camera_cb_of_type callback;

    if (!message || !data) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGD("msg_type 0x%x, sub msg type 0x%x", message->msg_type,
             message->sub_msg_type);
    callback = cxt->camera_cb;
    if (callback) {
        callback(message->sub_msg_type, cxt->client_data, message->msg_type,
                 message->data);
    } else {
        CMR_LOGE("err, camera cb is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
exit:
    return ret;
}

cmr_int camera_snapshot_send_raw_thread_proc(struct cmr_msg *message,
                                             void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)data;
    camera_cb_of_type callback;

    if (!message || !data) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGD("msg_type 0x%x, sub msg type 0x%x", message->msg_type,
             message->sub_msg_type);
    callback = cxt->camera_cb;
    if (callback) {
        callback(message->sub_msg_type, cxt->client_data, message->msg_type,
                 message->data);
    } else {
        CMR_LOGE("err, camera cb is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
    }
exit:
    return ret;
}

cmr_int camera_create_prev_thread(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    ret = cmr_thread_create(&cxt->prev_cb_thr_handle, PREVIEW_MSG_QUEUE_SIZE,
                            camera_preview_cb_thread_proc, (void *)oem_handle,
                            "preview_cb");

    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("create preview thread fail");
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

#ifdef CONFIG_FACE_BEAUTY
    if (cxt->video_face_beauty_en) {
        memset(&cxt->face_beauty, 0, sizeof(cxt->prev_face_beauty));
        cxt->mflagfb = false;
    }
#endif

exit:
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_create_video_thread(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
#ifdef CONFIG_FACE_BEAUTY
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    ret = cmr_thread_create(&cxt->video_cb_thr_handle, PREVIEW_MSG_QUEUE_SIZE,
                            camera_video_cb_thread_proc, (void *)oem_handle,
                            "video_cb");

    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("create video thread fail");
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    memset(&cxt->video_face_beauty, 0, sizeof(cxt->video_face_beauty));
    cxt->mvideofb = false;

exit:
    CMR_LOGD("X,ret=%ld", ret);
#endif
    return ret;
}

cmr_int camera_destroy_prev_thread(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

#ifdef CONFIG_FACE_BEAUTY
    if (cxt->video_face_beauty_en) {
       if (cxt->mflagfb) {
          cxt->mflagfb = false;
          ret = face_beauty_ctrl(&cxt->prev_face_beauty, FB_BEAUTY_FAST_STOP_CMD,NULL);
          face_beauty_deinit(&cxt->prev_face_beauty);
      }
   }
#endif
    if (cxt->prev_cb_thr_handle) {
        ret = cmr_thread_destroy(cxt->prev_cb_thr_handle);
        if (!ret) {
            cxt->prev_cb_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy prev thr");
        }
    }
exit:
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_destroy_video_thread(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
#ifdef CONFIG_FACE_BEAUTY
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (cxt->mvideofb) {
        cxt->mvideofb = false;
        cxt->start_video_face_beauty = false;
        ret = face_beauty_ctrl(&cxt->video_face_beauty, FB_BEAUTY_FAST_STOP_CMD,
                               NULL);
        face_beauty_deinit(&cxt->video_face_beauty);
    }

    if (cxt->video_cb_thr_handle) {
        ret = cmr_thread_destroy(cxt->video_cb_thr_handle);
        if (!ret) {
            cxt->video_cb_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy video thr");
        }
    }
exit:
    CMR_LOGD("X,ret=%ld", ret);
#endif
    return ret;
}

cmr_int camera_create_snp_thread(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    ret = cmr_thread_create(&cxt->snp_cb_thr_handle, SNAPSHOT_MSG_QUEUE_SIZE,
                            camera_snapshot_cb_thread_proc, (void *)oem_handle,
                            "snap_cb");

    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create snapshot thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    ret = cmr_thread_create(&cxt->snp_secondary_thr_handle,
            SNAPSHOT_MSG_QUEUE_SIZE, camera_snapshot_secondary_thread_proc,
            (void *)oem_handle, "snap_sec");

    if (CMR_MSG_SUCCESS != ret) {
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto destroy_cb_thr;
    }

    ret = cmr_thread_create(&cxt->snp_send_raw_image_handle,
            SNAPSHOT_MSG_QUEUE_SIZE, camera_snapshot_send_raw_thread_proc,
            (void *)oem_handle, "snap_raw");

    if (CMR_MSG_SUCCESS != ret) {
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto destroy_secondary_thr;
    }

    CMR_LOGD("X");
    return CMR_CAMERA_SUCCESS;

destroy_secondary_thr:
    cmr_thread_destroy(cxt->snp_secondary_thr_handle);
    cxt->snp_secondary_thr_handle = (cmr_handle)0;
destroy_cb_thr:
    cmr_thread_destroy(cxt->snp_cb_thr_handle);
    cxt->snp_cb_thr_handle = (cmr_handle)0;
exit:
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_destroy_snp_thread(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (cxt->snp_cb_thr_handle) {
        ret = cmr_thread_destroy(cxt->snp_cb_thr_handle);
        if (!ret) {
            cxt->snp_cb_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy snp cb thr %ld", ret);
        }
    }
    if (cxt->snp_secondary_thr_handle) {
        ret = cmr_thread_destroy(cxt->snp_secondary_thr_handle);
        if (!ret) {
            cxt->snp_secondary_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy snp secondary thr %ld", ret);
        }
    }
    if (cxt->snp_send_raw_image_handle) {
        ret = cmr_thread_destroy(cxt->snp_send_raw_image_handle);
        if (!ret) {
            cxt->snp_send_raw_image_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy snp raw thr %ld", ret);
        }
    }
exit:
    CMR_LOGD("X,ret=%ld", ret);
    return ret;
}

cmr_int camera_init_thread(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    cxt->video_face_beauty_en = 0;

#ifdef CONFIG_FACE_BEAUTY
    do {
        char video_face_en[PROPERTY_VALUE_MAX];

        property_get("persist.vendor.cam.video.face.beauty.enable", video_face_en, "0");
        if (atoi(video_face_en) == 1) {
            cxt->video_face_beauty_en = 1;
        }
    } while (0);
#endif
    /*create preview thread*/
    ret = camera_create_prev_thread(oem_handle);
    if (ret) {
        CMR_LOGE("failed to create preview thread");
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }

    /*create snapshot thread*/
    ret = camera_create_snp_thread(oem_handle);
    if (ret) {
        CMR_LOGE("failed to create snapshot thread");
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto prev_thr_destroy;
    } else {
        /*create video thread*/
        if (cxt->video_face_beauty_en) {
            ret = camera_create_video_thread(oem_handle);
            if (ret) {
                CMR_LOGE("failed to create video thread");
                ret = -CMR_CAMERA_NO_SUPPORT;
                goto prev_thr_destroy;
            } else {
                goto exit;
            }
        }
        goto exit;
    }

prev_thr_destroy:
    camera_destroy_prev_thread(oem_handle);

exit:
    return ret;
}

cmr_int camera_deinit_thread(cmr_handle oem_handle) {
    CMR_LOGD("E");
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    camera_destroy_prev_thread(oem_handle);
    if (cxt->video_face_beauty_en) {
        camera_destroy_video_thread(oem_handle);
    }
    camera_destroy_snp_thread(oem_handle);
    CMR_LOGD("X");

    return CMR_CAMERA_SUCCESS;
}

static cmr_int camera_res_init_internal(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_PRINT_TIME;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    /* for multicamera mode,when open convered sensor,only need to init setting
     */
    if (CONVERED_CAMERA_INIT) {
        ret = camera_setting_init(oem_handle);
        if (ret) {
            CMR_LOGE("failed to init setting %ld", ret);
        }
        goto exit;
    }

    ret = camera_nightpro_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init nightpro %ld", ret);
    }

    ret = camera_ipmpro_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init ipmpro %ld", ret);
        goto exit;
    }

    ret = camera_ipm_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init ipm %ld", ret);
        goto exit;
    }

    ret = camera_setting_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init setting %ld", ret);
        goto exit;
    }

    ret = camera_focus_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init focus %ld", ret);
        goto exit;
    }

    ret = camera_scaler_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init scaler %ld", ret);
        goto exit;
    }

    ret = camera_rotation_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init rotation %ld", ret);
        goto exit;
    }

    ret = camera_preview_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init preview %ld", ret);
        goto exit;
    }

    ret = camera_snapshot_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init snapshot %ld", ret);
        goto exit;
    } else {
        // CMR_LOGI("init mds ok");
        ret = camera_init_thread(oem_handle);
    }

exit:
    if (ret) {
        camera_res_deinit_internal(oem_handle);
    }

    return ret;
}

static cmr_int camera_res_deinit_internal(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_LOGD("E");
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    /*for multicamera mode,when close convered sensor,only need to deinit
     * setting*/
    if (CONVERED_CAMERA_INIT) {
        camera_setting_deinit(oem_handle);
        camera_deinit_thread(oem_handle);
        goto exit;
    }

    camera_snapshot_deinit(oem_handle);

    camera_preview_deinit(oem_handle);

    camera_focus_deinit(oem_handle);

    camera_setting_deinit(oem_handle);

    camera_rotation_deinit(oem_handle);

    camera_scaler_deinit(oem_handle);

    camera_ipm_deinit(oem_handle);
    if (cxt->night_cxt.is_authorized) {
        camera_nightpro_deinit(oem_handle);
    }
    camera_ipmpro_deinit(oem_handle);

    camera_deinit_thread(oem_handle);

exit:
    CMR_LOGD("X");

    ATRACE_END();
    return CMR_CAMERA_SUCCESS;
}

static cmr_int camera_init_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 msg_type = 0;
    cmr_uint evt = 0;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    struct camera_context *cxt = (struct camera_context *)p_data;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    msg_type = (cmr_u32)message->msg_type;

    switch (msg_type) {
    case CMR_EVT_INIT:
        cxt->err_code = camera_res_init_internal((cmr_handle)cxt);
        if (cxt->err_code) {
            CMR_LOGI("cb thread has deinited");
        }
        CMR_LOGI("cb thread inited");
        break;

    case CMR_EVT_WAIT:
        CMR_LOGI("wait here");
        break;

    case CMR_EVT_EXIT:
        camera_res_deinit_internal((cmr_handle)cxt);
        CMR_LOGI("camera exit");
        break;

    default:
        break;
    }

    return ret;
}
cmr_int camera_res_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_MSG_INIT(message);

    CMR_PRINT_TIME;
    sem_init(&cxt->mfsr_sm, 0, 1);
#ifdef OEM_HANDLE_HDR
    sem_init(&cxt->hdr_sync_sm, 0, 0);
    sem_init(&cxt->hdr_flag_sm, 0, 1);
#endif
#ifdef OEM_HANDLE_3DNR
    sem_init(&cxt->threednr_flag_sm, 0, 1);
#endif
    sem_init(&cxt->cnr_flag_sm, 0, 1);
    sem_init(&cxt->dre_flag_sm, 0, 1);
    sem_init(&cxt->dre_pro_flag_sm, 0, 1);
    sem_init(&cxt->ai_scene_flag_sm, 0, 1);
    sem_init(&cxt->filter_sm, 0, 1);
    sem_init(&cxt->eisWarp_sm, 0, 1);
    sem_init(&cxt->share_path_sm, 0, 0);
    sem_init(&cxt->access_sm, 0, 1);
    sem_init(&cxt->sbs_sync_sm, 0, 0);
    sem_init(&cxt->snapshot_sm, 0, 1);

    cxt->err_code = CMR_CAMERA_SUCCESS;
    /*create thread*/
    ret = cmr_thread_create((cmr_handle *)&cxt->init_thread,
                            CAMERA_OEM_MSG_QUEUE_SIZE, camera_init_thread_proc,
                            (void *)cxt, "res_init");
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("create thread fail");
    }

    CMR_LOGI("init thread created");
    message.msg_type = CMR_EVT_INIT;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    ret = cmr_thread_msg_send(cxt->init_thread, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
    }
    CMR_LOGI("async msg sent");
    ATRACE_END();
    return ret;
}

static cmr_int camera_res_init_done(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_MSG_INIT(message);

    message.msg_type = CMR_EVT_WAIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(cxt->init_thread, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
    }
    ret = cxt->err_code;
    CMR_LOGV("res init-ed");
    ATRACE_END();
    return ret;
}
static cmr_int camera_res_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_MSG_INIT(message);

    CMR_LOGD("E");

    message.msg_type = CMR_EVT_EXIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(cxt->init_thread, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
    }

    if (cxt->camera_cb) {
        cxt->camera_cb(CAMERA_EXIT_CB_DONE, cxt->client_data, CAMERA_FUNC_STOP,
                       0);
    }

    if (cxt->init_thread) {
        ret = cmr_thread_destroy(cxt->init_thread);
        if (!ret) {
            cxt->init_thread = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy init thr %ld", ret);
        }
    }
    sem_destroy(&cxt->mfsr_sm);
#ifdef OEM_HANDLE_HDR
    sem_destroy(&cxt->hdr_sync_sm);
    sem_destroy(&cxt->hdr_flag_sm);
#endif

#ifdef OEM_HANDLE_3DNR
    sem_destroy(&cxt->threednr_flag_sm);
#endif
    sem_destroy(&cxt->cnr_flag_sm);
    sem_destroy(&cxt->dre_flag_sm);
    sem_destroy(&cxt->ai_scene_flag_sm);

    sem_destroy(&cxt->filter_sm);
    sem_destroy(&cxt->eisWarp_sm);

    sem_destroy(&cxt->share_path_sm);
    sem_destroy(&cxt->access_sm);
    // for sbs mode capture
    sem_destroy(&cxt->sbs_sync_sm);
    sem_destroy(&cxt->snapshot_sm);

    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int camera_init_internal(cmr_handle oem_handle, cmr_uint is_autotest) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_int ret_deinit = CMR_CAMERA_SUCCESS;
    char value[PROPERTY_VALUE_MAX];
    pthread_mutex_lock(&close_mutex);
    while (closing != 0) {
        pthread_cond_wait(&close_cond, &close_mutex);
    }
    pthread_mutex_unlock(&close_mutex);
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_LOGD("E");

    /* for multicamera mode,when open convered sensor,only need to init sensor
     * and res
     */
    if (CONVERED_CAMERA_INIT) {
        ret = camera_sensor_init(oem_handle, is_autotest);
        if (ret) {
            CMR_LOGE("failed to init sensor %ld", ret);
            goto exit;
        }
        ret = camera_res_init(oem_handle);
        if (ret) {
            CMR_LOGE("failed to init res %ld", ret);
            goto grab_deinit;
        }
        ret = camera_res_init_done(oem_handle);
        goto exit;
    }

    ret = camera_sensor_init(oem_handle, is_autotest);
    if (ret) {
        CMR_LOGE("failed to init sensor %ld", ret);
        goto exit;
    }

    ret = camera_grab_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init grab %ld", ret);
        goto sensor_deinit;
    }

    ret = camera_res_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init res %ld", ret);
        goto grab_deinit;
    }

    ret = camera_isp_init(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init isp %ld", ret);
        goto res_deinit;
    }

    ret = camera_res_init_done(oem_handle);
    if (ret) {
        CMR_LOGE("failed to wait res done %ld", ret);
        goto isp_deinit;
    }

    ret = camera_jpeg_init_async(oem_handle);
    if (ret) {
        CMR_LOGE("failed to init jpeg %ld", ret);
        goto isp_deinit;
    }

#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
    pthread_mutex_lock(&mm_dvfs_mutex);
    ret = camera_mm_dvfs_init(oem_handle);
    pthread_mutex_unlock(&mm_dvfs_mutex);
    if (ret)
        CMR_LOGE("failed to init mm dvfs %ld", ret);
    ret = CMR_CAMERA_SUCCESS;
#endif
    camera_front_lcd_enhance_module_init(oem_handle);
    property_get("persist.vendor.cam.all_data.mode", value, "0");
    if (!strcmp(value, "cap_all")) {
        cxt->dbg_cxt.inited = 1;
        cxt->dbg_cxt.dump_bits = 1;
    }
    goto exit;

isp_deinit:
    ret_deinit = camera_isp_deinit_notice(oem_handle);
    if (ret_deinit) {
        CMR_LOGE("failed to camera_isp_deinit_notice %ld", ret_deinit);
    }

    ret_deinit = camera_isp_deinit(oem_handle);
    if (ret_deinit) {
        CMR_LOGE("failed to camera_isp_deinit %ld", ret_deinit);
    }

res_deinit:
    ret_deinit = camera_res_deinit(oem_handle);
    if (ret_deinit) {
        CMR_LOGE("failed to camera_res_deinit %ld", ret_deinit);
    }

grab_deinit:
    ret_deinit = camera_grab_deinit(oem_handle);
    if (ret_deinit) {
        CMR_LOGE("failed to camera_grab_deinit %ld", ret_deinit);
    }

sensor_deinit:
    ret_deinit = camera_sensor_deinit(oem_handle);
    if (ret_deinit) {
        CMR_LOGE("failed to camera_sensor_deinit %ld", ret_deinit);
    }

exit:
    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int camera_deinit_internal(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    pthread_mutex_lock(&close_mutex);
    closing++;
    pthread_mutex_unlock(&close_mutex);
    CMR_LOGD("E");
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    /*for multicamera mode,when close convered sensor,only need to deinit sensor
     * and res*/
    if (CONVERED_CAMERA_INIT) {
        camera_res_deinit(oem_handle);
        camera_sensor_deinit(oem_handle);
        pthread_mutex_lock(&close_mutex);
        closing--;
        if (closing == 0) {
            pthread_cond_signal(&close_cond);
        }
        pthread_mutex_unlock(&close_mutex);
        goto exit;
    }

    camera_front_lcd_enhance_module_deinit(oem_handle);
    camera_jpeg_deinit_async(oem_handle);
    camera_isp_deinit_notice(oem_handle);
    camera_isp_deinit(oem_handle);
    camera_res_deinit(oem_handle);
    camera_sensor_deinit(oem_handle);
    camera_grab_deinit(oem_handle);
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
    if (active_camera_num > 0)
        active_camera_num--;

    if (active_camera_num == 0) {
        pthread_mutex_lock(&mm_dvfs_mutex);
        camera_mm_dvfs_deinit(oem_handle);
        pthread_mutex_unlock(&mm_dvfs_mutex);
    }
#endif
    pthread_mutex_lock(&close_mutex);
    closing--;
    if (closing == 0) {
        pthread_cond_signal(&close_cond);
    }
    pthread_mutex_unlock(&close_mutex);

    CMR_LOGD("active_camera_num =%d", active_camera_num);
exit:

    ATRACE_END();
    CMR_LOGD("X");
    return ret;
}

cmr_int camera_nightpro_init(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGD("E");
#if USE_LEGACY_DLFCN
    cxt->night_cxt.sw_handle = dlopen("libnight.so", RTLD_NOW);
#else
    cxt->night_cxt.sw_handle = get_lib_handle("libnight.so");
#endif
    if (!cxt->night_cxt.sw_handle) {
        ret = -CMR_CAMERA_INVALID_PARAM;
        CMR_LOGD("libnight open failed with %s", dlerror());
        goto exit;
    } else {
        cxt->night_cxt.sw_open = dlsym(cxt->night_cxt.sw_handle,
                        "camera_sw_open");
        cxt->night_cxt.sw_process = dlsym(cxt->night_cxt.sw_handle,
                        "camera_sw_process");
        cxt->night_cxt.sw_close = dlsym(cxt->night_cxt.sw_handle,
                        "camera_sw_close");
        cxt->night_cxt.ipmpro_init = dlsym(cxt->night_cxt.sw_handle,
                        "camera_ipm_pro_init");
        cxt->night_cxt.ipmpro_deinit = dlsym(cxt->night_cxt.sw_handle,
                        "camera_ipm_pro_deinit");
        cxt->night_cxt.ipmpro_process = dlsym(cxt->night_cxt.sw_handle,
                        "camera_ipm_pro_process");
        if ((!cxt->night_cxt.sw_open) || (!cxt->night_cxt.sw_process) ||
                (!cxt->night_cxt.sw_close) || (!cxt->night_cxt.ipmpro_init) ||
                (!cxt->night_cxt.ipmpro_deinit) || (!cxt->night_cxt.ipmpro_process)) {
            CMR_LOGD("func analyzing failed with %s", dlerror());
#if USE_LEGACY_DLFCN
            dlclose(cxt->night_cxt.sw_handle);
#else
            put_lib_handle(cxt->night_cxt.sw_handle);
#endif
            cxt->night_cxt.is_authorized = 0;
        } else {
            cxt->night_cxt.is_authorized = 1;
        }
    }
    CMR_LOGV("is_authorized %d", cxt->night_cxt.is_authorized);

    if (cxt->night_cxt.is_authorized) {
        ret = cxt->night_cxt.ipmpro_init(oem_handle);
        if (ret) {
#if USE_LEGACY_DLFCN
            dlclose(cxt->night_cxt.sw_handle);
#else
            put_lib_handle(cxt->night_cxt.sw_handle);
#endif
            cxt->night_cxt.is_authorized = 0;
            CMR_LOGE("failed to init ipm pro %ld", ret);
        }
    }

exit:
    ATRACE_END();
    CMR_LOGD("X");
    return ret;
}

cmr_int camera_nightpro_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->night_flag = true;
    CMR_LOGD("D");
    if(cxt->night_cxt.ipmpro_deinit)
        cxt->night_cxt.ipmpro_deinit(oem_handle);
#if USE_LEGACY_DLFCN
    dlclose(cxt->night_cxt.sw_handle);
#else
    put_lib_handle(cxt->night_cxt.sw_handle);
#endif
    cxt->night_cxt.sw_open = NULL;
    cxt->night_cxt.sw_process = NULL;
    cxt->night_cxt.sw_close = NULL;
    cxt->night_cxt.ipmpro_init = NULL;
    cxt->night_cxt.ipmpro_deinit = NULL;
    cxt->night_cxt.ipmpro_process = NULL;
    cxt->night_cxt.is_authorized = 0;
    ATRACE_END();
    CMR_LOGD("X");
    return ret;
}

cmr_int camera_do_face_beauty_body(cmr_handle oem_handle,
                                   struct setting_context *setting_cxt,
                                   struct setting_cmd_parameter *_setting_param,
                                   struct img_frm *_src) {
    int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param = *_setting_param;
    struct img_frm src = *_src;

#ifdef CONFIG_FACE_BEAUTY
    int face_beauty_on = 0;
    cmr_u32 w = 0, h = 0;
    float ratio;
    int facecount;
    struct isp_face_area fd_face_area;
    fbBeautyFacetT beauty_face;
    fb_beauty_image_t beauty_image;
    struct faceBeautyLevels beautyLevels;
    struct facebeauty_param_info fb_param_map_cap;
    struct common_isp_cmd_param isp_param;
    int sx, sy, ex, ey, angle, pose;

    memset(&beautyLevels, 0, sizeof(struct faceBeautyLevels));
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PERFECT_SKINLEVEL, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get perfect skinlevel %ld", ret);
        return ret;
    }

    beautyLevels.blemishLevel =
        (unsigned char)setting_param.fb_param.blemishLevel;
    beautyLevels.smoothLevel =
        (unsigned char)setting_param.fb_param.smoothLevel;
    beautyLevels.skinColor = (unsigned char)setting_param.fb_param.skinColor;
    beautyLevels.skinLevel = (unsigned char)setting_param.fb_param.skinLevel;
    beautyLevels.brightLevel =
        (unsigned char)setting_param.fb_param.brightLevel;
    beautyLevels.lipColor = (unsigned char)setting_param.fb_param.lipColor;
    beautyLevels.lipLevel = (unsigned char)setting_param.fb_param.lipLevel;
    beautyLevels.slimLevel = (unsigned char)setting_param.fb_param.slimLevel;
    beautyLevels.largeLevel = (unsigned char)setting_param.fb_param.largeLevel;

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    beautyLevels.cameraBV = (int)isp_param.isp_adgain.bv;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_COL_TEM, &isp_param);
    beautyLevels.cameraCT = (int)isp_param.isp_cur_ct;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_SENS, &isp_param);
    beautyLevels.cameraISO = (int)isp_param.isp_cur_iso;
    beautyLevels.cameraWork = (int)cxt->camera_id;

    CMR_LOGD("Cam%d, bv %d, ct %d, iso %d, smooth %d, bright %d, slim %d, large %d\n",
             beautyLevels.cameraWork, beautyLevels.cameraBV,
             beautyLevels.cameraCT, beautyLevels.cameraISO,
             beautyLevels.smoothLevel, beautyLevels.brightLevel,
             beautyLevels.slimLevel, beautyLevels.largeLevel);
    face_beauty_on = beautyLevels.blemishLevel || beautyLevels.smoothLevel ||
                     beautyLevels.skinColor || beautyLevels.skinLevel ||
                     beautyLevels.brightLevel || beautyLevels.lipColor ||
                     beautyLevels.lipLevel || beautyLevels.slimLevel ||
                     beautyLevels.largeLevel;

    if (cxt->snp_cxt.snp_mode == CAMERA_ZSL_MODE)
        memcpy(&fd_face_area, &cxt->fd_face_area_capture,
                     sizeof(struct isp_face_area));
    else
        memcpy(&fd_face_area, &cxt->fd_face_area,
                     sizeof(struct isp_face_area));
    facecount = fd_face_area.face_num;

    if (face_beauty_on==0 || facecount == 0)
       goto exit;

    face_beauty_set_devicetype(&(cxt->face_beauty), SPRD_CAMALG_RUN_TYPE_CPU);
    face_beauty_init(&(cxt->face_beauty), 0, 2, 0);
    for (int i = 0; i < fd_face_area.face_num; i++) {
            beauty_face.idx = i;
            w = fd_face_area.face_info[i].ex - fd_face_area.face_info[i].sx;
            h = fd_face_area.face_info[i].ey -fd_face_area.face_info[i].sy;
            ratio = (float)src.size.width / (float)(fd_face_area.frame_width);
            beauty_face.startX = (int)(fd_face_area.face_info[i].sx * ratio);
            beauty_face.startY = (int)(fd_face_area.face_info[i].sy * ratio);
            beauty_face.endX = (int)(beauty_face.startX + w * ratio);
            beauty_face.endY = (int)(beauty_face.startY + h * ratio);
            beauty_face.angle = fd_face_area.face_info[i].angle;
            beauty_face.pose = fd_face_area.face_info[i].pose;
            CMR_LOGD("src face_info(idx[%d]:sx,ex(%d, %d) sy,ey(%d, %d)", i,
                fd_face_area.face_info[i].sx, fd_face_area.face_info[i].ex,
                fd_face_area.face_info[i].sy, fd_face_area.face_info[i].ey);
            CMR_LOGD("dst face_info(idx[%d]:w,h(%d, %d) sx,ex(%d, %d) sy,ey(%d, %d)", i,
                beauty_face.endX - beauty_face.startX,
                beauty_face.endY - beauty_face.startY,
                beauty_face.startX, beauty_face.endX,
                beauty_face.startY, beauty_face.endY);
            ret = face_beauty_ctrl(&(cxt->face_beauty),
                                   FB_BEAUTY_CONSTRUCT_FACE_CMD,
                                   (void *)&beauty_face);
    }

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_FB_CAP_PARAM, &isp_param);
    if (ret == ISP_SUCCESS) {
        struct facebeauty_param_info fb_param_map_cap;
        memcpy(&fb_param_map_cap, &isp_param.fb_param, sizeof(struct isp_fb_param_info));
        ret = face_beauty_ctrl(&(cxt->face_beauty),
                               FB_BEAUTY_CONSTRUCT_FACEMAP_CMD,
                               (void *)&fb_param_map_cap);
    }

    beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
    beauty_image.inputImage.addr[0] = (void *)(src.addr_vir.addr_y);
    beauty_image.inputImage.addr[1] = (void *)(src.addr_vir.addr_u);
    beauty_image.inputImage.addr[2] = (void *)(src.addr_vir.addr_v);
    beauty_image.inputImage.ion_fd = src.fd;
    beauty_image.inputImage.offset[0] = 0;
    beauty_image.inputImage.offset[1] = src.size.width * src.size.height;
    beauty_image.inputImage.width = src.size.width;
    beauty_image.inputImage.height = src.size.height;
    beauty_image.inputImage.stride = src.size.width;
    beauty_image.inputImage.size = src.size.width * src.size.height * 3 / 2;
    ret = face_beauty_ctrl(&(cxt->face_beauty), FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                         (void *)&beauty_image);
    ret = face_beauty_ctrl(&(cxt->face_beauty), FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                         (void *)&beautyLevels);
    ret = face_beauty_ctrl(&(cxt->face_beauty), FB_BEAUTY_PROCESS_CMD,
                           (void *)&facecount);
    face_beauty_deinit(&(cxt->face_beauty));

#endif
exit:
    return ret;
}

cmr_int camera_yuv_do_face_beauty_simplify(cmr_handle oem_handle,
                                           struct img_frm *src) {
    ATRACE_BEGIN(__FUNCTION__);
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    cmr_s32 filter_type = 0;
    int ret = CMR_CAMERA_SUCCESS;

    if (!oem_handle || !src->addr_vir.addr_y) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    sem_wait(&cxt->access_sm);

    cmr_bzero((void *)&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    src->data_end.y_endian = 0;
    src->data_end.uv_endian = 2;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_FILTER_TEYP, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get filtertype %ld", ret);
        goto exit;
    }
    filter_type = setting_param.cmd_type_value;
    if ((filter_type == 0) && (!cxt->prev_cxt.video_eb)) {
        ret = camera_do_face_beauty_body(oem_handle, setting_cxt,
                                         &setting_param, src);
    }
exit:
    sem_post(&cxt->access_sm);
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_jpeg_encode_exif_simplify(cmr_handle oem_handle,
                                         struct enc_exif_param *param) {
    ATRACE_BEGIN(__FUNCTION__);

    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct img_frm src = param->src;
    struct img_frm pic_enc = param->pic_enc;
    struct img_frm dst = param->last_dst;
    struct cmr_op_mean mean;
    struct jpeg_enc_cb_param enc_cb_param;
    struct jpeg_context *jpeg_cxt = NULL;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    cmr_s32 filter_type = 0;
    int ret = CMR_CAMERA_SUCCESS;
    int need_exif_flag = (dst.addr_vir.addr_y == 0) ? 0 : 1;
    cmr_u32 SUPER_FINE = 95;
    cmr_u32 FINE = 80;
    cmr_u32 NORMAL = 70;
    sizeParam_t sizeparam;
    cmr_bzero(&sizeparam, sizeof(sizeparam));
    cmr_bzero(&enc_cb_param, sizeof(enc_cb_param));

    if (!oem_handle || !src.addr_vir.addr_y || !pic_enc.addr_vir.addr_y) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    jpeg_cxt = &cxt->jpeg_cxt;

    sem_wait(&cxt->access_sm);
    // 1.construct param
    cmr_bzero((void *)&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    memset(&mean, 0, sizeof(struct cmr_op_mean));
    mean.quality_level = SUPER_FINE;
    mean.slice_mode = JPEG_YUV_SLICE_ONE_BUF;
    mean.slice_height = pic_enc.size.height;
    mean.is_sync = 1;
    src.data_end.y_endian = 0;
    src.data_end.uv_endian = 2;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_FILTER_TEYP, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get filtertype %ld", ret);
        goto exit;
    }
    filter_type = setting_param.cmd_type_value;
    if ((cxt->is_multi_mode == MODE_SINGLE_CAMERA ||
         cxt->is_multi_mode == MODE_SELF_SHOT ||
         cxt->is_multi_mode == MODE_MULTI_CAMERA ||
         cxt->is_multi_mode == MODE_BOKEH ||
         (cxt->is_multi_mode == MODE_BLUR && cxt->blur_facebeauty_flag == 1)) &&
        (filter_type == 0) && (!cxt->prev_cxt.video_eb)) {
        ret = camera_do_face_beauty_body(oem_handle, setting_cxt,
                                         &setting_param, &src);
        if (ret)
            goto exit;
    }
    /* add watermark: logo, or time */
    sizeparam.imgW = src.size.width;
    sizeparam.imgH = src.size.height;
    sizeparam.angle = param->rotation;
    sizeparam.isMirror = 0;
    cmr_uint flip_on = param->flip_on;
    cmr_u32 tmp = 0;

    if (cxt->is_multi_mode == MODE_BOKEH) {
        if (0 != sizeparam.angle) {
            if (90 == sizeparam.angle)
                mean.rot = 1;
            else if (180 == sizeparam.angle) {
                mean.flip = 1;
                mean.mirror = 1;
            } else if (270 == sizeparam.angle) {
                mean.rot = 1;
                mean.flip = 1;
                mean.mirror = 1;
            }
        }
        if (flip_on) {
            if (mean.mirror)
                mean.mirror = 0;
            else
                mean.mirror = 1;
        }
        if ((90 == sizeparam.angle || 270 == sizeparam.angle)) {
            tmp = pic_enc.size.height;
            pic_enc.size.height = pic_enc.size.width;
            pic_enc.size.width = tmp;
        }
    }

//    watermark_add_yuv(oem_handle, (cmr_u8 *)src.addr_vir.addr_y, &sizeparam);
    // for cache coherency
    cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle, &src);

    // 2.call jpeg interface
    ret = cmr_jpeg_encode(jpeg_cxt->jpeg_handle, &src, &pic_enc,
                          (struct jpg_op_mean *)&mean, &enc_cb_param);

    if (ret) {
        cxt->jpeg_cxt.enc_caller_handle = (cmr_handle)0;
        CMR_LOGE("failed to jpeg enc %ld", ret);
        goto exit;
    }

    param->stream_real_size = enc_cb_param.stream_size;
    CMR_LOGD("need_exif_flag:%d, param->stream_real_size:%u,is_sync=%d",
             need_exif_flag, param->stream_real_size, mean.is_sync);
    if (need_exif_flag) {
        struct jpeg_wexif_cb_param enc_out_param;
        ret = camera_start_exif_encode_simplify(oem_handle, &pic_enc, &dst,
                                                &enc_out_param);
        if (ret) {
            CMR_LOGE("failed to camera_start_exif_encode %ld", ret);
            goto exit;
        }
    }

exit:
    sem_post(&cxt->access_sm);
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_jpeg_decode_simplify(cmr_handle oem_handle,
                                         struct enc_exif_param *param) {
    ATRACE_BEGIN(__FUNCTION__);

    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct img_frm src_img_frm = param->src;
    struct img_frm dst_img_frm = param->pic_enc;
    struct yuvbuf_frm src_yuvbuf_frm;
    struct yuvbuf_frm dst_yuvbuf_frm;
    struct jpg_op_mean mean;
    struct jpeg_lib_cxt *jcxt = NULL;
    struct jpeg_context *jpeg_cxt = NULL;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 SUPER_FINE = 95;
    cmr_u32 FINE = 80;
    cmr_u32 NORMAL = 70;
    sizeParam_t sizeparam;
    cmr_bzero(&sizeparam, sizeof(sizeparam));
	CMR_LOGD("E");

    if (!oem_handle || !src_img_frm.addr_vir.addr_y || !dst_img_frm.addr_vir.addr_y) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    jpeg_cxt = &cxt->jpeg_cxt;
    jcxt = (struct jpeg_lib_cxt *)jpeg_cxt->jpeg_handle;

    sem_wait(&cxt->access_sm);
    // 1.construct param
    memset(&mean, 0, sizeof(struct jpg_op_mean));
    mean.slice_mode = 1;
    mean.is_sync = 1;

    src_yuvbuf_frm.fd = src_img_frm.fd;
    src_yuvbuf_frm.addr_phy.addr_y = src_img_frm.addr_phy.addr_y;
    src_yuvbuf_frm.addr_vir.addr_y=src_img_frm.addr_vir.addr_y;
    src_yuvbuf_frm.buf_size = src_img_frm.buf_size;
    CMR_LOGD("fd=0x%x, phy.addr_y=%lx, vir.addr_y=%lx, buf_size=%d",
             (cmr_u32)src_yuvbuf_frm.fd,
             src_yuvbuf_frm.addr_phy.addr_y,
             src_yuvbuf_frm.addr_vir.addr_y,
             src_yuvbuf_frm.buf_size);


    dst_yuvbuf_frm.fd = dst_img_frm.fd;
    dst_yuvbuf_frm.addr_phy.addr_y = dst_img_frm.addr_phy.addr_y;
    dst_yuvbuf_frm.addr_vir.addr_y = dst_img_frm.addr_vir.addr_y;
    dst_yuvbuf_frm.buf_size = dst_img_frm.buf_size;
    dst_yuvbuf_frm.size.width = dst_img_frm.size.width;
    dst_yuvbuf_frm.size.height = dst_img_frm.size.height;
    dst_yuvbuf_frm.data_end.y_endian = 1;
    dst_yuvbuf_frm.data_end.uv_endian = 2;
    CMR_LOGD("fd=0x%x, phy.addr_y=%lx, vir.add_y=%lx, buf_size=%d, size.width=%d,"
             "size.height=%d, y_endian=%d, uv_endian=%d",
             (cmr_u32)dst_yuvbuf_frm.fd,
             dst_yuvbuf_frm.addr_phy.addr_y,
             dst_yuvbuf_frm.addr_vir.addr_y,
             dst_yuvbuf_frm.buf_size,
             dst_yuvbuf_frm.size.width,
             dst_yuvbuf_frm.size.height,
             dst_yuvbuf_frm.data_end.y_endian,
             dst_yuvbuf_frm.data_end.uv_endian);

    ret = jcxt->ops.jpeg_decode(jcxt->codec_handle, &src_yuvbuf_frm, &dst_yuvbuf_frm, &mean);
    if (ret) {
        cxt->jpeg_cxt.enc_caller_handle = (cmr_handle)0;
        CMR_LOGE("failed to jpeg decode %ld", ret);
        goto exit;
    }

exit:
    sem_post(&cxt->access_sm);
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_start_encode(cmr_handle oem_handle, cmr_handle caller_handle,
                            struct img_frm *src, struct img_frm *dst,
                            struct cmr_op_mean *mean,
                            struct jpeg_enc_cb_param *enc_cb_param) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        return ret;
    }
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct img_frm enc_src, enc_dst;
    struct cmr_op_mean enc_mean;
    cmr_uint stream_size;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    cmr_uint rotation = 0;
    cmr_uint flip_on = 0;
    cmr_u32 is_raw_capture = 0;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 tmp = 0;
    struct timespec start_time, end_time;
    unsigned int duration;
    cmr_s32 filter_type = 0;
    struct jpeg_context *jpeg_cxt = NULL;
    cmr_u32 watermark_flush = 0;
    cmr_u32 facebeauty_flush = 0;
    sizeParam_t sizeparam;
    cmr_bzero(&sizeparam, sizeof(sizeparam));

    jpeg_cxt = &cxt->jpeg_cxt;
    CHECK_HANDLE_VALID(jpeg_cxt);
    cxt->jpeg_cxt.enc_caller_handle = caller_handle;
    sem_wait(&cxt->access_sm);

    if (!caller_handle || !oem_handle || !src || !dst || !mean) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    cmr_bzero((void *)&setting_param, sizeof(setting_param));
    cmr_copy(&enc_src, src, sizeof(struct img_frm));
    cmr_copy(&enc_dst, dst, sizeof(struct img_frm));
    cmr_copy(&enc_mean, mean, sizeof(struct cmr_op_mean));

    setting_param.camera_id = cxt->camera_id;

#if defined(CONFIG_ISP_2_3)
     if (is_raw_capture == 0) {
           ret = cmr_setting_ioctl(setting_cxt->setting_handle,SETTING_GET_ORIGINAL_PICTURE_SIZE, &setting_param);
           if ( (setting_param.originalPictureSize.height % 16 != 0) &&
                (setting_param.originalPictureSize.height !=  dst->size.height &&
                 setting_param.originalPictureSize.width == dst->size.width)) {
                 enc_dst.size.height = setting_param.originalPictureSize.height;
            }
     }
#endif

/* from sharkl2, jpeg support mirror/flip/rotation, mirror feature always use
 * jpeg if jpeg support it */
#ifdef MIRROR_FLIP_ROTATION_BY_JPEG
    /* raw capture not support mirror/flip/rotation*/
    enc_mean.flip = 0;
    enc_mean.rot = 0;
    enc_mean.mirror = 0;

    if (is_raw_capture == 0) {
        ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                SETTING_GET_ENCODE_ROTATION, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get enc rotation %ld", ret);
            goto exit;
        }
        rotation = setting_param.cmd_type_value;

        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_GET_FLIP_ON, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get preview sprd flip_on enabled flag %ld",
                     ret);
            goto exit;
        }
        flip_on = setting_param.cmd_type_value;
        CMR_LOGI("encode_rotation:%ld, flip:%ld", rotation, flip_on);

        if (0 != rotation) {
            if (90 == rotation)
                enc_mean.rot = 1;
            else if (180 == rotation) {
                enc_mean.flip = 1;
                enc_mean.mirror = 1;
            } else if (270 == rotation) {
                enc_mean.rot = 1;
                enc_mean.flip = 1;
                enc_mean.mirror = 1;
            }
        }

        if (flip_on) {
            if (enc_mean.mirror)
                enc_mean.mirror = 0;
            else
                enc_mean.mirror = 1;
        }

        if ((90 == rotation || 270 == rotation)) {
            tmp = enc_dst.size.height;
            enc_dst.size.height = enc_dst.size.width;
            enc_dst.size.width = tmp;
        }
    }
#endif
    cxt->jpeg_cxt.enc_caller_handle = caller_handle;

    CMR_LOGD("src: fd=0x%x, y_offset=0x%lx, u_offset=0x%lx, virt_y=0x%lx, "
             "virt_u=0x%lx, width=%d, height=%d, y_endian=%d, uv_endian=%d",
             src->fd, src->addr_phy.addr_y, src->addr_phy.addr_u,
             src->addr_vir.addr_y, src->addr_vir.addr_u, src->size.width,
             src->size.height, src->data_end.y_endian, src->data_end.uv_endian);
    CMR_LOGD("dst: fd=0x%x, stream_offset=0x%lx, stream_vir=0x%lx, width=%d, "
             "height=%d",
             enc_dst.fd, enc_dst.addr_phy.addr_y, enc_dst.addr_vir.addr_y,
             enc_dst.size.width, enc_dst.size.height);
    CMR_LOGD("enc_mean: mirror=%d, flip=%d, rot=%d, is_sync=%d, is_thumb=%d",
             enc_mean.mirror, enc_mean.flip, enc_mean.rot, enc_mean.is_sync,
             enc_mean.is_thumb);

    cmr_snapshot_invalidate_cache(cxt->snp_cxt.snapshot_handle, dst);

    if (1 != mean->is_thumb) {
        ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                SETTING_GET_FILTER_TEYP, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get filtertype %ld", ret);
            goto exit;
        }
        filter_type = setting_param.cmd_type_value;
        if ((cxt->is_multi_mode == MODE_SINGLE_CAMERA ||
             cxt->is_multi_mode == MODE_SELF_SHOT ||
             cxt->is_multi_mode == MODE_MULTI_CAMERA ||
             (cxt->is_multi_mode == MODE_BLUR &&
              cxt->blur_facebeauty_flag == 1)) &&
            (filter_type == 0) && (!cxt->prev_cxt.video_eb)) {
            if (camera_do_face_beauty_body(oem_handle, setting_cxt,
                                           &setting_param, src))
                goto exit;
            else
                facebeauty_flush = 1;
        }
        /* add watermark: logo, or time */
        sizeparam.imgW = enc_src.size.width;
        sizeparam.imgH = enc_src.size.height;
        sizeparam.angle = rotation;
        sizeparam.isMirror = flip_on;
        ret = watermark_add_yuv(oem_handle, (cmr_u8 *)enc_src.addr_vir.addr_y,
                                &sizeparam);
        watermark_flush |= ret;
        watermark_flush |= facebeauty_flush;
        if (watermark_flush) // for cache coherency
            cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle, src);

        /* if super cap, encode will skip and return success */
        if (cxt->snp_cxt.is_super == 0)
            ret = cmr_jpeg_encode(jpeg_cxt->jpeg_handle, &enc_src, &enc_dst,
                                  (struct jpg_op_mean *)&enc_mean, enc_cb_param);
        else
            ret = CMR_CAMERA_SUCCESS;

        if (ret) {
            CMR_LOGE("failed to jpeg codec %ld", ret);
            goto exit;
        }
        if (mean->is_sync != 0) {
            dst->buf_size = enc_dst.buf_size;
        }
        cxt->jpg_encode = JPEG_ENCODING;
    } else {
        enc_dst.buf_size = (enc_dst.size.height * enc_dst.size.width) * 3;
        enc_dst.buf_size = enc_dst.buf_size / 2;
        ret = cmr_jpeg_encode(jpeg_cxt->jpeg_handle, &enc_src, &enc_dst,
                              (struct jpg_op_mean *)&enc_mean, enc_cb_param);
        if (ret) {
            CMR_LOGE("failed to jpeg codec %ld", ret);
            goto exit;
        }
        if (mean->is_sync != 0) {
            dst->reserved = enc_dst.reserved;
            dst->buf_size = enc_dst.buf_size;
        }
    }

exit:
    sem_post(&cxt->access_sm);
    ATRACE_END();
    return ret;
}

cmr_int camera_start_decode(cmr_handle oem_handle, cmr_handle caller_handle,
                            struct img_frm *src, struct img_frm *dst,
                            struct cmr_op_mean *mean) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct jpeg_dec_in_param dec_in_param;
    struct jpeg_context *jpeg_cxt = NULL;

    jpeg_cxt = &cxt->jpeg_cxt;
    CHECK_HANDLE_VALID(jpeg_cxt);

    if (!caller_handle || !oem_handle || !src || !dst || !mean) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGI("src phy addr 0x%lx src vir addr 0x%lx", src->addr_phy.addr_y,
             src->addr_vir.addr_y);
    CMR_LOGI("dst phr addr 0x%lx  0x%lx vir addr 0x%lx 0x%lx",
             dst->addr_phy.addr_y, dst->addr_phy.addr_u, dst->addr_vir.addr_y,
             dst->addr_vir.addr_u);
    CMR_LOGI("src size %d %d", src->size.width, src->size.height);
    CMR_LOGI("out size %d %d", dst->size.width, dst->size.height);
    CMR_LOGI("temp size %d", mean->temp_buf.buf_size);
    CMR_LOGD("caller_handle 0x%lx", (cmr_uint)caller_handle);

    cxt->jpeg_cxt.dec_caller_handle = caller_handle;

    ret = cmr_jpeg_decode(jpeg_cxt->jpeg_handle, src, dst,
                          (struct jpg_op_mean *)mean);

    if (ret) {
        cxt->jpeg_cxt.dec_caller_handle = (cmr_handle)0;
        CMR_LOGE("dec start fail ret %ld", ret);
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_stop_codec(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct jpeg_context *jpeg_cxt = NULL;

    CHECK_HANDLE_VALID(oem_handle);
    jpeg_cxt = &cxt->jpeg_cxt;
    CHECK_HANDLE_VALID(jpeg_cxt);

    ret = cmr_stop_codec(jpeg_cxt->jpeg_handle);
    if (ret) {
        CMR_LOGE("failed to stop codec %ld", ret);
    }

    return ret;
}

cmr_int camera_start_exif_encode(cmr_handle oem_handle,
                                 cmr_handle caller_handle,
                                 struct img_frm *pic_src,
                                 struct img_frm *thumb_src, void *exif,
                                 struct img_frm *dst,
                                 struct jpeg_wexif_cb_param *out_ptr) {
    UNUSED(exif);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;

    struct jpeg_enc_exif_param enc_exif_param;
    struct jpeg_wexif_cb_param out_pram;
    struct setting_cmd_parameter setting_param;
    struct common_isp_cmd_param isp_param;
    struct jpeg_context *jpeg_cxt = NULL;

    if (!caller_handle || !oem_handle || !pic_src || !dst || !thumb_src ||
        !out_ptr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    jpeg_cxt = &cxt->jpeg_cxt;
    CHECK_HANDLE_VALID(jpeg_cxt);

    cmr_bzero(&enc_exif_param, sizeof(struct jpeg_enc_exif_param));
    cmr_bzero(&out_pram, sizeof(struct jpeg_wexif_cb_param));
    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));

    if (1 == camera_get_hdr_flag(cxt)) {
        //camera_set_exif_exposure_time(oem_handle);
    }

    enc_exif_param.jpeg_handle = cxt->jpeg_cxt.jpeg_handle;
    enc_exif_param.src_jpeg_addr_virt = pic_src->addr_vir.addr_y;
    enc_exif_param.thumbnail_addr_virt = thumb_src->addr_vir.addr_y;
    enc_exif_param.target_addr_virt = dst->addr_vir.addr_y;
    enc_exif_param.src_jpeg_size = pic_src->buf_size;
    enc_exif_param.thumbnail_size = thumb_src->buf_size;
    enc_exif_param.target_size = dst->buf_size;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_EXIF_INFO, &setting_param);
    enc_exif_param.exif_ptr = setting_param.exif_all_info_ptr;
    enc_exif_param.exif_isp_info = NULL;
    if (cxt->is_multi_mode != MODE_BOKEH) {
        if (enc_exif_param.exif_ptr != NULL) {
            enc_exif_param.exif_ptr->spec_ptr->pic_taking_cond_ptr->FNumber
                .numerator = cxt->sn_cxt.cur_sns_ex_info.f_num;
            enc_exif_param.exif_ptr->spec_ptr->pic_taking_cond_ptr->FNumber
                .denominator = 100;
        }
    }

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_EXIF_DEBUG_INFO, &isp_param);
    if (ret) {
        CMR_LOGW("isp get exif debug info failed");
        enc_exif_param.exif_isp_debug_info.addr = NULL;
        enc_exif_param.exif_isp_debug_info.size = 0;
    } else {
        enc_exif_param.exif_isp_debug_info.addr = isp_param.isp_dbg_info.addr;
        enc_exif_param.exif_isp_debug_info.size = isp_param.isp_dbg_info.size;
    }

    CMR_LOGV("exif_isp_debug_info: addr=%p, size=%d",
             enc_exif_param.exif_isp_debug_info.addr,
             enc_exif_param.exif_isp_debug_info.size);

    enc_exif_param.padding = 0;
    out_pram.output_buf_virt_addr = 0;
    out_pram.output_buf_size = 0;
    ret = cmr_jpeg_enc_add_eixf(jpeg_cxt->jpeg_handle, &enc_exif_param,
                                &out_pram);
    if (!ret) {
        *out_ptr = out_pram;
        CMR_LOGD("out addr 0x%lx size %ld", out_ptr->output_buf_virt_addr,
                 out_ptr->output_buf_size);
    } else {
        CMR_LOGE("failed to compund exif jpeg %ld", ret);
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_start_exif_encode_simplify(cmr_handle oem_handle,
                                          struct img_frm *pic_src,
                                          struct img_frm *dst,
                                          struct jpeg_wexif_cb_param *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct jpeg_context *jpeg_cxt = NULL;
    struct isp_context *isp_cxt = &cxt->isp_cxt;

    struct jpeg_enc_exif_param enc_exif_param;
    struct jpeg_wexif_cb_param out_pram;
    struct setting_cmd_parameter setting_param;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle || !pic_src || !dst || !out_ptr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    jpeg_cxt = &cxt->jpeg_cxt;
    CHECK_HANDLE_VALID(jpeg_cxt);

    cmr_bzero((void *)&enc_exif_param, sizeof(struct jpeg_enc_exif_param));
    cmr_bzero((void *)&out_pram, sizeof(struct jpeg_wexif_cb_param));
    cmr_bzero((void *)&setting_param, sizeof(struct setting_cmd_parameter));
    cmr_bzero((void *)&isp_param, sizeof(struct common_isp_cmd_param));

    enc_exif_param.jpeg_handle = cxt->jpeg_cxt.jpeg_handle;
    enc_exif_param.src_jpeg_addr_virt = pic_src->addr_vir.addr_y;
    // enc_exif_param.thumbnail_addr_virt = thumb_src->addr_vir.addr_y;
    enc_exif_param.target_addr_virt = dst->addr_vir.addr_y;
    enc_exif_param.src_jpeg_size = pic_src->buf_size;
    // enc_exif_param.thumbnail_size = thumb_src->buf_size;
    enc_exif_param.target_size = dst->buf_size;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_EXIF_INFO, &setting_param);
    enc_exif_param.exif_ptr = setting_param.exif_all_info_ptr;
    enc_exif_param.exif_isp_info = NULL;
    if (enc_exif_param.exif_ptr != NULL) {
        enc_exif_param.exif_ptr->spec_ptr->pic_taking_cond_ptr->FNumber
            .numerator = cxt->sn_cxt.cur_sns_ex_info.f_num;
        enc_exif_param.exif_ptr->spec_ptr->pic_taking_cond_ptr->FNumber
            .denominator = 100;
    }

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_EXIF_DEBUG_INFO, &isp_param);
    if (ret) {
        CMR_LOGW("isp get exif debug info failed");
        enc_exif_param.exif_isp_debug_info.addr = NULL;
        enc_exif_param.exif_isp_debug_info.size = 0;
    } else {
        enc_exif_param.exif_isp_debug_info.addr = isp_param.isp_dbg_info.addr;
        enc_exif_param.exif_isp_debug_info.size = isp_param.isp_dbg_info.size;
    }

    CMR_LOGV("exif_isp_debug_info: addr=%p, size=%d",
             enc_exif_param.exif_isp_debug_info.addr,
             enc_exif_param.exif_isp_debug_info.size);

    enc_exif_param.padding = 0;
    out_pram.output_buf_virt_addr = 0;
    out_pram.output_buf_size = 0;
    ret = cmr_jpeg_enc_add_eixf(jpeg_cxt->jpeg_handle, &enc_exif_param,
                                &out_pram);
    if (!ret) {
        *out_ptr = out_pram;
        CMR_LOGI("out addr 0x%lx size %ld", out_ptr->output_buf_virt_addr,
                 out_ptr->output_buf_size);
    } else {
        CMR_LOGE("failed to compund exif jpeg %ld", ret);
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_start_scale(cmr_handle oem_handle, cmr_handle caller_handle,
                           struct img_frm *src, struct img_frm *dst,
                           struct cmr_op_mean *mean) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !src || !dst || !mean) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGD(
        "caller %p sync %d, src fd 0x%x, paddr 0x%lx 0x%lx , dst fd 0x%x, paddr 0x%lx 0x%lx",
        caller_handle, mean->is_sync,
        src->fd, src->addr_phy.addr_y, src->addr_phy.addr_u,
        dst->fd, dst->addr_phy.addr_y, dst->addr_phy.addr_u);
    CMR_LOGD(
        "src size %d %d dst size %d %d rect %d %d %d %d endian %d %d %d %d",
        src->size.width, src->size.height, dst->size.width, dst->size.height,
        src->rect.start_x, src->rect.start_y, src->rect.width, src->rect.height,
        src->data_end.y_endian, src->data_end.uv_endian, dst->data_end.y_endian,
        dst->data_end.uv_endian);

    if (1 != mean->is_sync) {
        ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst,
                              camera_scaler_evt_cb, (void *)caller_handle);
    } else {
        ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst,
                              (cmr_evt_cb)NULL, NULL);
    }
    if (ret) {
        CMR_LOGE("failed to start scaler, ret %ld", ret);
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);

    ATRACE_END();
    return ret;
}

cmr_int camera_start_rot(cmr_handle oem_handle, cmr_handle caller_handle,
                         struct img_frm *src, struct img_frm *dst,
                         struct cmr_op_mean *mean) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct cmr_rot_param rot_param;
    cmr_uint restart_cnt = 0;

    if (!oem_handle || !src || !dst || !mean) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (IMG_ANGLE_0 == mean->rot) {
        CMR_LOGV("in parm angle 0, no need rotate");
        goto exit;
    }
    camera_take_snapshot_step(CMR_STEP_ROT_S);
    do {
        rot_param.angle = mean->rot;
        rot_param.handle = cxt->rot_cxt.rotation_handle;
        rot_param.src_img = *src;
        rot_param.dst_img = *dst;
        ret = cmr_rot(&rot_param);
        if (ret) {
            CMR_LOGE("failed to rotate %ld", ret);
            ret = camera_restart_rot(oem_handle);
        } else {
            goto rot_end;
        }
        restart_cnt++;
    } while (restart_cnt < OEM_RESTART_SUM);
rot_end:
    camera_take_snapshot_step(CMR_STEP_ROT_E);
exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_preview_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 preview_sn_mode) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_u32 sensor_mode;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGI("camera id %d", camera_id);

    sensor_mode = preview_sn_mode;
    cxt->prev_cxt.preview_sn_mode = preview_sn_mode;

    CMR_LOGI("sensor work mode %d", sensor_mode);
    ret =
        cmr_sensor_set_mode(cxt->sn_cxt.sensor_handle, camera_id, sensor_mode);
    if (ret) {
        CMR_LOGE("failed to set sensor work mode %ld", ret);
        goto exit;
    }
    ret = cmr_sensor_set_mode_done(cxt->sn_cxt.sensor_handle, camera_id);
    if (ret) {
        CMR_LOGE("failed to wait done %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_preview_post_proc(cmr_handle oem_handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    cmr_int flash_status;

    // whether FRONT_CAMERA_FLASH_TYPE is lcd
    bool isFrontLcd =
        (strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") == 0) ? true : false;
    // whether FRONT_CAMERA_FLASH_TYPE is flash
    bool isFrontFlash =
        (strcmp(FRONT_CAMERA_FLASH_TYPE, "flash") == 0) ? true : false;

    if (1 == camera_id && !isFrontLcd && !isFrontFlash)
        goto exit;

    setting_param.camera_id = camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_HW_FLASH_STATUS, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get flash mode %ld", ret);
        goto exit;
    }
    flash_status = setting_param.cmd_type_value;
    CMR_LOGD("HW flash_status=%ld", flash_status);
    /*close flash*/
    if ((CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) &&
        ((FLASH_OPEN == flash_status) || (FLASH_HIGH_LIGHT == flash_status))) {
        memset(&setting_param, 0, sizeof(setting_param));
        setting_param.camera_id = camera_id;
        setting_param.setting_flash_status = SETTING_FLASH_PRE_AFTER;

        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_CTRL_FLASH, &setting_param);
        if (ret) {
            CMR_LOGE("failed to open flash %ld", ret);
        }
    }

exit:
    return ret;
}

cmr_int camera_capture_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 preview_mode, cmr_u32 capture_mode,
                                cmr_u32 is_restart, cmr_u32 is_sn_reopen) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt;
    struct setting_cmd_parameter setting_param;
    struct common_sn_cmd_param sn_param;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle || (camera_id != cxt->camera_id)) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_cxt = &cxt->snp_cxt;

    CMR_LOGI("camera id %d, capture mode %d preview mode %d,  is_restart %d "
             "is_sn_reopen %d, snapshot_sn_mode %d",
             camera_id, capture_mode, preview_mode, is_restart, is_sn_reopen,
             snp_cxt->snp_mode);

    if ((CAMERA_ZSL_MODE != snp_cxt->snp_mode) &&
        (!is_restart || is_sn_reopen)) {
        snp_cxt = &cxt->snp_cxt;
        snp_cxt->snapshot_sn_mode = capture_mode;
        /*set sensor before snapshot*/
        ret = cmr_sensor_ioctl(
            cxt->sn_cxt.sensor_handle, camera_id, SENSOR_BEFORE_SNAPSHOT,
            (capture_mode | (preview_mode << CAP_MODE_BITS)));
        if (ret) {
            CMR_LOGE("failed to set sensor %ld", ret);
        }
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_capture_post_proc(cmr_handle oem_handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt;
    struct setting_cmd_parameter setting_param;
    struct common_isp_cmd_param isp_param;
    cmr_int flash_status = FLASH_CLOSE;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    snp_cxt = &cxt->snp_cxt;

    // whether FRONT_CAMERA_FLASH_TYPE is lcd
    bool isFrontLcd =
        (strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") == 0) ? true : false;
    // whether FRONT_CAMERA_FLASH_TYPE is flash
    bool isFrontFlash =
        (strcmp(FRONT_CAMERA_FLASH_TYPE, "flash") == 0) ? true : false;
    if (cxt->is_multi_mode == MODE_MULTI_CAMERA || camera_id == 0 ||
        isFrontLcd || isFrontFlash || camera_id == 4) {
        setting_param.camera_id = camera_id;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_GET_HW_FLASH_STATUS, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get flash mode %ld", ret);
            // goto exit;
        }
        flash_status = setting_param.cmd_type_value;
        CMR_LOGD("HW flash_status=%ld", flash_status);
        CMR_LOGD("start");
        if (FLASH_OPEN == flash_status || FLASH_HIGH_LIGHT == flash_status) {
            /*close flash*/
            memset(&setting_param, 0, sizeof(setting_param));
            setting_param.camera_id = camera_id;
            setting_param.setting_flash_status = SETTING_FLASH_MAIN_AFTER;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                    SETTING_CTRL_FLASH, &setting_param);
            if (ret) {
                CMR_LOGE("failed to open flash");
            }
        }
    }

    // notify isp after snapshot
    ret =
        camera_isp_ioctl(oem_handle, COM_ISP_SET_SNAPSHOT_FINISHED, &isp_param);
    if (ret) {
        CMR_LOGE("failed to set snapshot finished %ld", ret);
    }

exit:
    CMR_LOGD("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_local_snapshot_is_need_flash(cmr_handle oem_handle,
                                            cmr_u32 camera_id,
                                            cmr_u32 *is_need_flash) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt;
    struct setting_cmd_parameter setting_param;
    if (!oem_handle || (camera_id != cxt->camera_id)) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    // whether FRONT_CAMERA_FLASH_TYPE is lcd
    bool isFrontLcd =
        (strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") == 0) ? true : false;
    // whether FRONT_CAMERA_FLASH_TYPE is flash
    bool isFrontFlash =
        (strcmp(FRONT_CAMERA_FLASH_TYPE, "flash") == 0) ? true : false;
    if (camera_id == 1 && !isFrontLcd && !isFrontFlash)
        goto exit;

    snp_cxt = &cxt->snp_cxt;

    CMR_LOGD("camera id %d, capture mode %d", camera_id, snp_cxt->snp_mode);

    memset(&setting_param, 0, sizeof(setting_param));
    setting_param.camera_id = camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_FLASH_STATUS, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get flash mode %ld", ret);
        goto exit;
    }
    *is_need_flash = setting_param.cmd_type_value;
    CMR_LOGD("is_need_flash = %d", *is_need_flash);
exit:
    return ret;
}

cmr_int camera_capture_highflash(cmr_handle oem_handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt;
    struct setting_cmd_parameter setting_param;
    cmr_uint has_preflashed;
    cmr_uint flash_status;

    if (!oem_handle || (camera_id != cxt->camera_id)) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    snp_cxt = &cxt->snp_cxt;

    CMR_LOGD("camera id %d, capture mode %d", camera_id, snp_cxt->snp_mode);

    if ((1 != camera_get_hdr_flag(cxt)) && (1 != camera_get_3dnr_flag(cxt))) {
        /*open flash*/
        memset(&setting_param, 0, sizeof(setting_param));
        setting_param.camera_id = camera_id;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_GET_FLASH_STATUS, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get flash mode %ld", ret);
            goto exit;
        }
        flash_status = setting_param.cmd_type_value;

        memset(&setting_param, 0, sizeof(setting_param));
        setting_param.camera_id = camera_id;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_GET_PRE_LOWFLASH_VALUE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get preflashed flag %ld", ret);
        }
        has_preflashed = setting_param.cmd_type_value;
        CMR_LOGD("flash_status = %ld, has_preflashed=%ld", flash_status,
                 has_preflashed);

        if (!ret && flash_status && has_preflashed) {
            memset(&setting_param, 0, sizeof(setting_param));
            setting_param.camera_id = camera_id;
            setting_param.setting_flash_status = SETTING_FLASH_MAIN_LIGHTING;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                    SETTING_CTRL_FLASH, &setting_param);
            if (ret) {
                CMR_LOGE("failed to open flash");
            }
        }
    }

exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_set_3dnr_video(cmr_handle oem_handle, cmr_uint is_3dnr_video) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->is_3dnr_video = is_3dnr_video;
    CMR_LOGI("is_3dnr_video %ld", cxt->is_3dnr_video);
    return ret;
}

cmr_int camera_get_af_support(cmr_handle oem_handle, cmr_u16 *af_support) {

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt = NULL;
    struct sensor_exp_info *sensor_info_ptr;

    sn_cxt = &(cxt->sn_cxt);
    CHECK_HANDLE_VALID(sn_cxt);

    ret = cmr_sensor_get_info(sn_cxt->sensor_handle, cxt->camera_id,
                              &(sn_cxt->sensor_info));
    if (ret) {
        CMR_LOGE("fail to get sensor info ret %ld", ret);
        goto exit;
    }
    sensor_info_ptr = &(sn_cxt->sensor_info);
    CHECK_HANDLE_VALID(sensor_info_ptr);

    if (sensor_info_ptr->raw_info_ptr &&
        sensor_info_ptr->raw_info_ptr->ioctrl_ptr &&
        sensor_info_ptr->raw_info_ptr->ioctrl_ptr->set_pos) {
        *af_support = 1;
    } else {
        *af_support = 0;
    }
    // af cb when fdr capturing
    if (cxt->fdr_skip_frame_enable)
        *af_support = 0;


    CMR_LOGI("af_support %ld", *af_support);
    return ret;
exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_set_ultra_wide_mode(cmr_handle oem_handle,
                                   cmr_uint is_ultra_wide) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->is_ultra_wide = is_ultra_wide;
    CMR_LOGI("is_ultra_wide %ld", cxt->is_ultra_wide);
    return ret;
}

cmr_int camera_set_fov_fusion_mode(cmr_handle oem_handle,
                                   cmr_uint is_fov_fusion) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->is_fov_fusion = is_fov_fusion;
    CMR_LOGI("is_fov_fusion %ld", cxt->is_fov_fusion);
    return ret;
}

cmr_int camera_set_fov_fusion_app_ratio(cmr_handle oem_handle,
                                   float app_ratio) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->app_ratio = app_ratio;
    CMR_LOGI("app_ratio %f", cxt->app_ratio);
    return ret;
}

cmr_int camera_set_multi_camera_id(cmr_handle oem_handle,
                                   cmr_uint is_multi_camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->is_multi_camera_id = is_multi_camera_id;
    CMR_LOGI("is_multi_camera_id %ld", cxt->is_multi_camera_id);
    return ret;
}

cmr_int camera_open_sensor(cmr_handle oem_handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt = &cxt->sn_cxt;
    cmr_handle sensor_handle;
    cmr_u32 camera_id_bits = 0;

    (void)camera_id;

    sensor_handle = sn_cxt->sensor_handle;
    camera_id_bits = 1 << cxt->camera_id;
    ret = cmr_sensor_open(sensor_handle, camera_id_bits);
    if (ret) {
        CMR_LOGE("open %d sensor failed %ld", cxt->camera_id, ret);
        return CMR_CAMERA_FAIL;
    }
    ATRACE_END();
    return ret;
}

cmr_int camera_close_sensor(cmr_handle oem_handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt = &cxt->sn_cxt;
    cmr_handle sensor_handle;
    cmr_u32 camera_id_bits = 0;

    (void)camera_id;
    sensor_handle = sn_cxt->sensor_handle;
    camera_id_bits = 1 << cxt->camera_id;
    ret = cmr_sensor_close(sensor_handle, camera_id_bits);
    if (ret) {
        CMR_LOGE("open %d sensor failed %ld", cxt->camera_id, ret);
        return CMR_CAMERA_FAIL;
    }
    ATRACE_END();
    return ret;
}

cmr_int camera_raw_proc(cmr_handle oem_handle, cmr_handle caller_handle,
                        struct raw_proc_param *param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct sensor_ex_info *sns_ex_info_ptr = NULL;
    char value[PROPERTY_VALUE_MAX];
    struct sensor_mode_info *sensor_mode_info;
    cmr_u32 sn_mode = 0;

    if (!oem_handle || !param_ptr || !caller_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGI("slice num %d avail height %d slice height %d",
             param_ptr->slice_num, param_ptr->src_avail_height,
             param_ptr->src_slice_height);
    CMR_LOGI("src addr 0x%lx 0x%lx pattern %d,dst addr 0x%lx 0x%lx, dst2 addr "
             "0x%lx 0x%lx",
             param_ptr->src_frame.addr_phy.addr_y,
             param_ptr->src_frame.addr_phy.addr_u,
             param_ptr->src_frame.format_pattern,
             param_ptr->dst_frame.addr_phy.addr_y,
             param_ptr->dst_frame.addr_phy.addr_u,
             param_ptr->dst2_frame.addr_phy.addr_y,
             param_ptr->dst2_frame.addr_phy.addr_u);
    CMR_LOGI("fd: src.fd=0x%x, dst.fd=0x%x, dst2.fd=0x%x",
             param_ptr->src_frame.fd, param_ptr->dst_frame.fd,
             param_ptr->dst2_frame.fd);

    if (1 == param_ptr->slice_num) {
        struct ips_in_param in_param;
        struct ips_out_param out_param;

        in_param.oem_handle = oem_handle;
        in_param.alloc_cb = camera_malloc;
        in_param.free_cb = camera_free;
        in_param.invalidate_cb = camera_invalidate_buf;
        in_param.flush_cb = camera_flush_buf;

        in_param.src_frame.img_fmt = param_ptr->src_frame.fmt;
        in_param.src_frame.img_size.w = param_ptr->src_frame.size.width;
        in_param.src_frame.img_size.h = param_ptr->src_frame.size.height;
        in_param.src_frame.img_fd.y = param_ptr->src_frame.fd;
        in_param.src_frame.img_fd.u = param_ptr->src_frame.fd;
        in_param.src_frame.img_addr_phy.chn0 =
            param_ptr->src_frame.addr_phy.addr_y;
        in_param.src_frame.img_addr_phy.chn1 =
            param_ptr->src_frame.addr_phy.addr_u;
        in_param.src_frame.img_addr_vir.chn0 =
            param_ptr->src_frame.addr_vir.addr_y;
        in_param.src_frame.img_addr_vir.chn1 =
            param_ptr->src_frame.addr_vir.addr_u;
        //		in_param.src_frame.format_pattern =
        // param_ptr->src_frame.format_pattern;
        in_param.src_avail_height = param_ptr->src_avail_height;
        in_param.src_slice_height = param_ptr->src_slice_height;
        in_param.dst_frame.img_fmt = param_ptr->dst_frame.fmt;
        in_param.dst_frame.img_size.w = param_ptr->dst_frame.size.width;
        in_param.dst_frame.img_size.h = param_ptr->dst_frame.size.height;
        in_param.dst_frame.img_fd.y = param_ptr->dst_frame.fd;
        in_param.dst_frame.img_fd.u = param_ptr->dst_frame.fd;
        in_param.dst_frame.img_addr_phy.chn0 =
            param_ptr->dst_frame.addr_phy.addr_y;
        in_param.dst_frame.img_addr_phy.chn1 =
            param_ptr->dst_frame.addr_phy.addr_u;
        in_param.dst_frame.img_addr_vir.chn0 =
            param_ptr->dst_frame.addr_vir.addr_y;
        in_param.dst_frame.img_addr_vir.chn1 =
            param_ptr->dst_frame.addr_vir.addr_u;
        in_param.dst_slice_height = param_ptr->dst_slice_height;

        in_param.dst2_frame.img_fmt = param_ptr->dst2_frame.fmt;
        in_param.dst2_frame.img_size.w = param_ptr->dst2_frame.size.width;
        in_param.dst2_frame.img_size.h = param_ptr->dst2_frame.size.height;
        in_param.dst2_frame.img_fd.y = param_ptr->dst2_frame.fd;
        in_param.dst2_frame.img_fd.u = param_ptr->dst2_frame.fd;
        in_param.dst2_frame.img_addr_phy.chn0 =
            param_ptr->dst2_frame.addr_phy.addr_y;
        in_param.dst2_frame.img_addr_phy.chn1 =
            param_ptr->dst2_frame.addr_phy.addr_u;
        in_param.dst2_frame.img_addr_vir.chn0 =
            param_ptr->dst2_frame.addr_vir.addr_y;
        in_param.dst2_frame.img_addr_vir.chn1 =
            param_ptr->dst2_frame.addr_vir.addr_u;
        in_param.dst2_slice_height = param_ptr->dst2_slice_height;

        ret = cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                  &sn_mode);
        // we think OEM has get sensor info and save it into sensor context,so
        // we get mode info from cxt
        sensor_mode_info = &cxt->sn_cxt.sensor_info.mode_info[sn_mode];
        SENSOR_MODE_FPS_T fps_info;
        ret = cmr_sensor_get_fps_info(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                      sn_mode, &fps_info);
        camera_copy_sensor_fps_info_to_isp(&in_param.sensor_fps, &fps_info);
        if (ret) {
            CMR_LOGE("get sensor fps info failed %ld", ret);
            goto exit;
        }
        in_param.resolution_info.sensor_size.w = sensor_mode_info->width;
        in_param.resolution_info.sensor_size.h = sensor_mode_info->height;
        in_param.resolution_info.line_time = sensor_mode_info->line_time;
        in_param.resolution_info.frame_line = sensor_mode_info->frame_line;
        in_param.resolution_info.size_index = sn_mode;
        in_param.resolution_info.crop.st_x = sensor_mode_info->trim_start_x;
        in_param.resolution_info.crop.st_y = sensor_mode_info->trim_start_y;
        in_param.resolution_info.crop.width = sensor_mode_info->trim_width;
        in_param.resolution_info.crop.height = sensor_mode_info->trim_height;
        in_param.resolution_info.fps.max_fps = in_param.sensor_fps.max_fps;
        in_param.resolution_info.fps.min_fps = in_param.sensor_fps.min_fps;
        in_param.resolution_info.sensor_max_size.w =
            cxt->sn_cxt.sensor_info.source_width_max;
        in_param.resolution_info.sensor_max_size.h =
            cxt->sn_cxt.sensor_info.source_height_max;
        in_param.resolution_info.sensor_output_size.w =
            sensor_mode_info->out_width;
        in_param.resolution_info.sensor_output_size.h =
            sensor_mode_info->out_height;
        CMR_LOGI("ips_in_param:sensor fps info: is_high_fps %d, "
                 "high_fps_skip_num is %d",
                 in_param.sensor_fps.is_high_fps,
                 in_param.sensor_fps.high_fps_skip_num);
        CMR_LOGI("ips_in_param:sensor fps info:	max_fps is %d, min_fps is %d",
                 in_param.sensor_fps.max_fps, in_param.sensor_fps.min_fps);
        CMR_LOGI("ips_in_param: sensor max w h, %d %d",
                 in_param.resolution_info.sensor_max_size.w,
                 in_param.resolution_info.sensor_max_size.h);
        CMR_LOGI("ips_in_param sensor output w h, %d %d",
                 in_param.resolution_info.sensor_output_size.w,
                 in_param.resolution_info.sensor_output_size.h);
        CMR_LOGI("ips_in_param: sensor crop startx start w h, %d %d %d %d",
                 in_param.resolution_info.crop.st_x,
                 in_param.resolution_info.crop.st_y,
                 in_param.resolution_info.crop.width,
                 in_param.resolution_info.crop.height);

        sns_ex_info_ptr = &cxt->sn_cxt.cur_sns_ex_info;
        if (NULL == sns_ex_info_ptr) {
            CMR_LOGE("sns_ex_info_ptr is null, It is impossible error!");
            ret = -CMR_CAMERA_INVALID_PARAM;
            goto exit;
        }

        if ((NULL == sns_ex_info_ptr->name) ||
            (NULL == sns_ex_info_ptr->sensor_version_info)) {
            ret = cmr_sensor_init_static_info(cxt);
            if (ret) {
                cmr_sensor_deinit_static_info(cxt);
                CMR_LOGE("init sensor static info failed %ld", ret);
                goto exit;
            }
        }

        in_param.resolution_info.max_gain = sns_ex_info_ptr->max_adgain;
        CMR_LOGI("ips_in_param:max_gain:%d ",
                 in_param.resolution_info.max_gain);

        property_get("debug.camera.save.snpfile", value, "0");
        if (atoi(value) == 1 || atoi(value) == 100 ||
            (atoi(value) & (1 << 1))) {
            dump_image("camera_raw_proc", CAM_IMG_FMT_BAYER_MIPI_RAW,
                       param_ptr->src_frame.size.width,
                       param_ptr->src_frame.size.height,
                       FORM_DUMPINDEX(0x4000, cxt->dump_cnt, 0),
                       &param_ptr->src_frame.addr_vir,
                       param_ptr->src_frame.size.width *
                           param_ptr->src_frame.size.height * 5 / 4);
        }

#if defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6) ||                      \
    defined(CONFIG_ISP_2_7)
        if (isp_video_get_simulation_flag())
            in_param.hwsim_4in1_width =
                cxt->sn_cxt.info_4in1.limited_4in1_width;
#endif
        usleep(500 * 1000);
        in_param.remosaic_type = cxt->remosaic_type;
        ret = isp_proc_start(isp_cxt->isp_handle, &in_param, &out_param);
        if (ret) {
            CMR_LOGE("failed to start proc %ld", ret);
            goto exit;
        }

        property_get("debug.camera.save.snpfile", value, "0");
        if (atoi(value) == 1 || atoi(value) == 100 ||
            (atoi(value) & (1 << 1))) {
            dump_image("camera_raw_proc", CAM_IMG_FMT_YUV420_NV21,
                       param_ptr->dst_frame.size.width,
                       param_ptr->dst_frame.size.height, 0x6000,
                       &param_ptr->dst_frame.addr_vir,
                       param_ptr->dst_frame.size.width *
                           param_ptr->dst_frame.size.height * 3 / 2);
        }
    } else {
        struct ipn_in_param in_param;
        struct ips_out_param out_param;
        in_param.src_avail_height = param_ptr->src_avail_height;
        in_param.src_slice_height = param_ptr->src_slice_height;
        in_param.src_addr_phy.chn0 = param_ptr->src_frame.addr_phy.addr_y;
        in_param.src_addr_phy.chn1 = param_ptr->src_frame.addr_phy.addr_u;
        in_param.dst_addr_phy.chn0 = param_ptr->dst_frame.addr_phy.addr_y;
        in_param.dst_addr_phy.chn1 = param_ptr->dst_frame.addr_phy.addr_u;
        ret = isp_proc_next(isp_cxt->isp_handle, &in_param, &out_param);
        if (ret) {
            CMR_LOGE("failed to start proc %ld", ret);
            goto exit;
        }
    }

    if (CMR_CAMERA_SUCCESS == ret) {
        cxt->isp_cxt.caller_handle = caller_handle;
        CMR_LOGD("caller handle 0x%lx", (cmr_uint)caller_handle);
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_isp_start_video(cmr_handle oem_handle,
                               struct video_start_param *param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct isp_video_start isp_param;
    cmr_int work_mode = 0;
    cmr_int dv_mode = 0;
    cmr_int raw_buf_size;
    cmr_int highiso_buf_size;
    cmr_int tmp_buf_size;
    cmr_int num = 0;
    cmr_int highiso_buf_num = 0;
    cmr_int isp_raw_buf_num = 0;
    struct setting_cmd_parameter setting_param;
    struct sensor_ex_info *sns_ex_info_ptr;
    struct sensor_mode_info *sensor_mode_info = NULL;
    cmr_u32 sn_mode = 0;
    struct sensor_exp_info exp_info;
    struct sensor_exp_info *sensor_info_ptr;
    SENSOR_VAL_T val;
    struct sensor_pdaf_info pdaf_info;
    char prop[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.dual.preview", prop, "0");

    if (!param_ptr || !oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGV("E");

    cmr_bzero(&isp_param, sizeof(struct isp_video_start));
    cmr_bzero(&exp_info, sizeof(struct sensor_exp_info));
    cmr_bzero(&pdaf_info, sizeof(struct sensor_pdaf_info));

    isp_param.size.w = param_ptr->size.width;
    isp_param.size.h = param_ptr->size.height;

#ifdef CONFIG_CAMERA_OFFLINE
    isp_param.dcam_size.w = param_ptr->dcam_size.width;
    isp_param.dcam_size.h = param_ptr->dcam_size.height;
    isp_param.zsl_flag = param_ptr->sprd_zsl_flag;
#endif

    isp_param.format = ISP_DATA_NORMAL_RAW10;
    isp_param.mode = ISP_VIDEO_MODE_CONTINUE;

#if defined(CONFIG_ISP_2_3)
    if (cxt->lsc_malloc_flag == 0) {
        cmr_u32 lsc_buf_size = 0;
        cmr_u32 lsc_buf_num = 0;
        isp_param.lsc_buf_num = ISP_LSC_BUF_NUM;
        isp_param.lsc_buf_size = ISP_LSC_BUF_SIZE;
        lsc_buf_size = isp_param.lsc_buf_size;
        lsc_buf_num = isp_param.lsc_buf_num;

        if (cxt->hal_malloc == NULL) {
            ret = -1;
            CMR_LOGE("hal_malloc is null");
            goto exit;
        }

        cxt->hal_malloc(CAMERA_ISP_LSC, &lsc_buf_size, &lsc_buf_num,
                        &cxt->isp_lsc_phys_addr, &cxt->isp_lsc_virt_addr,
                        &cxt->lsc_mfd, cxt->client_data);
        cxt->lsc_malloc_flag = 1;
    }
#endif

    isp_param.lsc_phys_addr = cxt->isp_lsc_phys_addr;
    isp_param.lsc_virt_addr = cxt->isp_lsc_virt_addr;
    isp_param.lsc_mfd = cxt->lsc_mfd;
    isp_param.cb_of_malloc = cxt->hal_malloc;
    isp_param.cb_of_free = cxt->hal_free;
    isp_param.buffer_client_data = cxt->client_data;
    isp_param.oem_handle = oem_handle;
    isp_param.alloc_cb = camera_malloc;
    isp_param.free_cb = camera_free;
    isp_param.invalidate_cb = camera_invalidate_buf;
    isp_param.flush_cb = camera_flush_buf;

    if (0 == param_ptr->work_mode) {
        work_mode = param_ptr->work_mode;

        cmr_bzero(&setting_param, sizeof(setting_param));
        setting_param.camera_id = cxt->camera_id;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_GET_DV_MODE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get dv mode %ld", ret);
            goto exit;
        }
        if (setting_param.cmd_type_value) {
            work_mode = 0;
        }
        dv_mode = setting_param.cmd_type_value;
    } else {
        work_mode = param_ptr->work_mode;
    }

    isp_param.work_mode = work_mode;
    isp_param.is_snapshot = param_ptr->is_snapshot;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_CAPTURE_MODE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get dv mode %ld", ret);
        goto exit;
    }
    // TBD: check this
    isp_param.capture_mode = 0; // setting_param.cmd_type_value;
    isp_param.dv_mode = dv_mode;

    ret = cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                              &sn_mode);
    if (ret) {
        CMR_LOGE("cmr_sensor_get_mode failed");
        goto exit;
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_APPMODE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get app mode %ld", ret);
        goto exit;
    }
    isp_param.app_mode = setting_param.cmd_type_value;
        CMR_LOGD("isp_param.app_mode %d", isp_param.app_mode);

    ret = cmr_sensor_get_info(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                              &exp_info);
    if (ret) {
        CMR_LOGE("cmr_sensor_get_info failed ret=%ld", ret);
        goto exit;
    }
    sensor_mode_info = &exp_info.mode_info[sn_mode];

    isp_param.resolution_info.sensor_size.w = sensor_mode_info->width;
    isp_param.resolution_info.sensor_size.h = sensor_mode_info->height;
    isp_param.resolution_info.crop.width = sensor_mode_info->trim_width;
    isp_param.resolution_info.crop.height = sensor_mode_info->trim_height;
    isp_param.resolution_info.sensor_output_size.w =
        sensor_mode_info->out_width;
    isp_param.resolution_info.sensor_output_size.h =
        sensor_mode_info->out_height;
    isp_param.resolution_info.line_time = sensor_mode_info->line_time;
    isp_param.resolution_info.frame_line = sensor_mode_info->frame_line;
    isp_param.resolution_info.size_index = sn_mode;
    isp_param.resolution_info.crop.st_x = sensor_mode_info->trim_start_x;
    isp_param.resolution_info.crop.st_y = sensor_mode_info->trim_start_y;
    isp_param.resolution_info.sensor_max_size.w =
        cxt->sn_cxt.sensor_info.source_width_max;
    isp_param.resolution_info.sensor_max_size.h =
        cxt->sn_cxt.sensor_info.source_height_max;
    isp_param.is_refocus = cxt->is_refocus_mode;

    if(cxt->sn_cxt.cur_sns_ex_info.pdaf_supported) {
        val.type = SENSOR_VAL_TYPE_GET_PDAF_INFO;
        val.pval = &pdaf_info;
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                SENSOR_ACCESS_VAL, (cmr_uint)&val);
        CMR_LOGI("output ret id %d", ret);
        if (!ret && sn_mode < SENSOR_PDAF_MODE && pdaf_info.sns_mode)
            if (pdaf_info.sns_mode[sn_mode])
                isp_param.pdaf_enable = 1;
    }

    SENSOR_MODE_FPS_T fps_info;
    ret = cmr_sensor_get_fps_info(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                  sn_mode, &fps_info);
    if (ret) {
        CMR_LOGE("get sensor fps info failed %ld", ret);
        goto exit;
    }
    camera_copy_sensor_fps_info_to_isp(&isp_param.sensor_fps, &fps_info);

    isp_param.resolution_info.fps.max_fps = isp_param.sensor_fps.max_fps;
    isp_param.resolution_info.fps.min_fps = isp_param.sensor_fps.min_fps;

    sns_ex_info_ptr = &cxt->sn_cxt.cur_sns_ex_info;
    if (NULL == sns_ex_info_ptr) {
        CMR_LOGE("sns_ex_info_ptr is null, It is impossible error!");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if ((NULL == sns_ex_info_ptr->name) ||
        (NULL == sns_ex_info_ptr->sensor_version_info)) {
        ret = cmr_sensor_init_static_info(cxt);
        if (ret) {
            cmr_sensor_deinit_static_info(cxt);
            CMR_LOGE("init sensor static info failed %ld", ret);
            goto exit;
        }
    }
    sensor_info_ptr = &cxt->sn_cxt.sensor_info;
    if (NULL != sns_ex_info_ptr->sns_binning_factor &&
        sns_ex_info_ptr->sns_binning_factor[sn_mode] != 0)
        sensor_info_ptr->mode_info[sn_mode].binning_factor =
            sns_ex_info_ptr->sns_binning_factor[sn_mode];
    else
        sensor_info_ptr->mode_info[sn_mode].binning_factor = 1;
    isp_param.resolution_info.max_gain = sns_ex_info_ptr->max_adgain;

    ispmw_dev_buf_cfg_evt_cb(isp_cxt->isp_handle, camera_isp_dev_evt_cb);

    if (1 == cxt->isp_to_dram) {
        isp_param.capture_mode = ISP_CAP_MODE_DRAM;
    }

    if (cxt->is_real_bokeh) {
        CMR_LOGD("real bokeh");
        // sharkl5 TBD remove these code
        // isp_param.is_real_bokeh = cxt->is_real_bokeh;
        // DEPTH
        if (cxt->swisp_depth_malloc_flag == 0) {
            cmr_u32 depth_size = 960 * 720 * 3 / 2;
            cmr_u32 depth_num = 1;
            if (cxt->hal_malloc) {
                cxt->hal_malloc(CAMERA_PREVIEW_DEPTH, &depth_size, &depth_num,
                                &cxt->swisp_depth_phys_addr,
                                &cxt->swisp_depth_virt_addr,
                                &cxt->swisp_depth_mfd, cxt->client_data);
                cxt->swisp_depth_malloc_flag = 1;
            } else {
                ret = -CMR_CAMERA_NO_MEM;
                CMR_LOGE("failed to malloc isp lsc buffer");
                goto exit;
            }
            // sharkl5 TBD remove these code
            // isp_param.s_yuv_depth.img_size.w = 960;
            // isp_param.s_yuv_depth.img_size.h = 720;
            // isp_param.s_yuv_depth.img_fd.y = cxt->swisp_depth_mfd;
            // isp_param.s_yuv_depth.img_addr_vir.chn0 =
            //    cxt->swisp_depth_virt_addr;
        }

        // SW_OUT
        if (cxt->swisp_out_malloc_flag == 0) {
            cmr_u32 sw_out_size = 960 * 720 * 3 / 2;
            cmr_u32 sw_out_num = 1;
            if (cxt->hal_malloc) {
                cxt->hal_malloc(CAMERA_PREVIEW_SW_OUT, &sw_out_size,
                                &sw_out_num, &cxt->swisp_out_phys_addr,
                                &cxt->swisp_out_virt_addr, &cxt->swisp_out_mfd,
                                cxt->client_data);
                cxt->swisp_out_malloc_flag = 1;
            } else {
                ret = -CMR_CAMERA_NO_MEM;
                CMR_LOGE("failed to malloc isp lsc buffer");
                goto exit;
            }
            // sharkl5 TBD remove these code
            // isp_param.s_yuv_sw_out.img_size.w = 800;
            // isp_param.s_yuv_sw_out.img_size.h = 600;
            // isp_param.s_yuv_sw_out.img_fd.y = cxt->swisp_out_mfd;
            // isp_param.s_yuv_sw_out.img_addr_vir.chn0 =
            // cxt->swisp_out_virt_addr;
        }
    }

    CMR_LOGI(
        "sensor_max_size: w=%d, h=%d, sensor_output_size: w=%d,h=%d, "
        "sn_mode=%d, crop: st_x=%d, st_y=%d, w=%d, h=%d, max_gain=%d, "
        "sensor_fps: is_high_fps=%d, high_fps_skip_num=%d, max_fps=%d, "
        "min_fps=%d, isp_param.size: w=%d, h=%d, lsc_phys_addr=0x%lx, "
        "lsc_virt_addr=0x%lx, work_mode=%d, dv_mode=%d, capture_mode=%d, "
        "is_snapshot=%d",
        isp_param.resolution_info.sensor_max_size.w,
        isp_param.resolution_info.sensor_max_size.h,
        isp_param.resolution_info.sensor_output_size.w,
        isp_param.resolution_info.sensor_output_size.h, sn_mode,
        isp_param.resolution_info.crop.st_x,
        isp_param.resolution_info.crop.st_y,
        isp_param.resolution_info.crop.width,
        isp_param.resolution_info.crop.height,
        isp_param.resolution_info.max_gain, isp_param.sensor_fps.is_high_fps,
        isp_param.sensor_fps.high_fps_skip_num, isp_param.sensor_fps.max_fps,
        isp_param.sensor_fps.min_fps, isp_param.size.w, isp_param.size.h,
        isp_param.lsc_phys_addr, isp_param.lsc_virt_addr, isp_param.work_mode,
        isp_param.dv_mode, isp_param.capture_mode, isp_param.is_snapshot);

    /* set remosaic_type */
    isp_param.remosaic_type = camera_get_remosaic_type(
        &(cxt->sn_cxt.info_4in1),
        isp_param.resolution_info.sensor_output_size.w,
        isp_param.resolution_info.sensor_output_size.h);
    isp_param.is_4in1_sensor =
        camera_get_is_4in1_sensor(&(cxt->sn_cxt.info_4in1));
    if (isp_video_get_simulation_flag()) {
        isp_param.is_4in1_sensor = 0;
        isp_param.remosaic_type = 0;
    }
    isp_param.is_high_res_mode = cxt->is_high_res_mode;

    ret = isp_video_start(isp_cxt->isp_handle, &isp_param);
    if (ret) {
        isp_cxt->is_work = 0;
        CMR_LOGE("failed to start isp, ret %ld", ret);
        goto exit;
    }
    isp_cxt->is_work = 1;

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_isp_stop_video(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (cxt->isp_cxt.is_work) {
        ret = isp_video_stop(isp_cxt->isp_handle);
        if (ret) {
            CMR_LOGE("failed to stop isp %ld", ret);
        } else {
            cxt->isp_cxt.is_work = 0;
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_channel_cfg(cmr_handle oem_handle, cmr_handle caller_handle,
                           cmr_u32 camera_id,
                           struct channel_start_param *param_ptr,
                           cmr_u32 *channel_id, struct img_data_end *endian) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt = &cxt->sn_cxt;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    struct sensor_exp_info sensor_info;
    struct sensor_mode_info *sensor_mode_info, *tmp;
    struct sn_cfg sensor_cfg;
    cmr_u32 sn_mode = 0;
    cmr_u32 sprd_3dnr_type = 0;
    SENSOR_MODE_FPS_T fps_info;

    if (!oem_handle || !caller_handle || !param_ptr || !channel_id || !endian) {
        CMR_LOGE("in parm error 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx",
                 (cmr_uint)oem_handle, (cmr_uint)caller_handle,
                 (cmr_uint)param_ptr, (cmr_uint)channel_id, (cmr_uint)endian);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (cxt->app_mode == CAMERA_MODE_FDR && cxt->camera_id == 0)
        param_ptr->cap_inf_cfg.cfg.need_fdr = 1;
    else
        param_ptr->cap_inf_cfg.cfg.need_fdr = 0;

    cmr_bzero(&sensor_cfg, sizeof(struct sn_cfg));
    param_ptr->cap_inf_cfg.buffer_cfg_isp = 0;
    CMR_LOGV("param_ptr->cap_inf_cfg.buffer_cfg_isp %d",
             param_ptr->cap_inf_cfg.buffer_cfg_isp);
    if (param_ptr->is_lightly) {
        CMR_LOGD("channel id %d, caller_handle 0x%lx, skip num %d", *channel_id,
                 (cmr_uint)caller_handle, param_ptr->skip_num);
        ret = cmr_grab_cap_cfg_lightly(cxt->grab_cxt.grab_handle,
                                       &param_ptr->cap_inf_cfg, *channel_id);
        if (ret) {
            CMR_LOGE("failed to cap cfg %ld", ret);
            goto exit;
        }
        goto exit;
    }

    ret = cmr_sensor_get_info(sn_cxt->sensor_handle, camera_id, &sensor_info);
    if (ret) {
        CMR_LOGE("failed to get sensor info %ld", ret);
        goto exit;
    }

    ret = cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                              &sn_mode);
    if (ret)
        goto exit;

    ret = cmr_sensor_get_fps_info(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                  sn_mode, &fps_info);
    if (ret) {
        CMR_LOGE("get sensor fps info failed %ld", ret);
        goto exit;
    }

    CMR_LOGD("frm_num 0x%x", param_ptr->frm_num);

    sensor_mode_info = &sensor_info.mode_info[param_ptr->sensor_mode];
    sensor_cfg.sn_size.width = sensor_mode_info->width;
    sensor_cfg.sn_size.height = sensor_mode_info->height;
    sensor_cfg.frm_num = param_ptr->frm_num;
    sensor_cfg.sn_trim.start_x = sensor_mode_info->trim_start_x;
    sensor_cfg.sn_trim.start_y = sensor_mode_info->trim_start_y;
    sensor_cfg.sn_trim.width = sensor_mode_info->trim_width;
    sensor_cfg.sn_trim.height = sensor_mode_info->trim_height;
    CMR_LOGD("sn_size: width=%d, height=%d, camera_id=%d",
             sensor_cfg.sn_size.width, sensor_cfg.sn_size.height,
             cxt->camera_id);
    CMR_LOGD("sn_trim: width=%d, height=%d", sensor_cfg.sn_trim.width,
             sensor_cfg.sn_trim.height);
    CMR_LOGD("scaler_trim: width=%d, height=%d",
             sensor_mode_info->scaler_trim.width,
             sensor_mode_info->scaler_trim.height);
    param_ptr->sn_if.sensor_width = sensor_mode_info->width;
    param_ptr->sn_if.sensor_height = sensor_mode_info->height;
    param_ptr->sn_if.if_spec.mipi.pclk = sensor_mode_info->bps_per_lane;
    param_ptr->sn_if.if_spec.mipi.is_cphy = sensor_info.sn_interface.is_cphy;
    if (cxt->isp_to_dram)
        param_ptr->sn_if.res[0] = 1;
    else
        param_ptr->sn_if.res[0] = 0;

    sensor_cfg.sensor_max_size.width = sensor_info.source_width_max;
    sensor_cfg.sensor_max_size.height = sensor_info.source_height_max;
    CMR_LOGD("sensor_max_size: width=%d, height=%d",
             sensor_cfg.sensor_max_size.width,
             sensor_cfg.sensor_max_size.height);

    ret = cmr_grab_if_cfg(cxt->grab_cxt.grab_handle, &param_ptr->sn_if);
    if (ret) {
        CMR_LOGE("failed interface cfg %ld", ret);
        goto exit;
    }

    ret = cmr_grab_sn_cfg(cxt->grab_cxt.grab_handle, &sensor_cfg);
    if (ret) {
        CMR_LOGE("failed to sn cfg %ld", ret);
        goto exit;
    }

    if (caller_handle == cxt->prev_cxt.preview_handle) {
        cxt->prev_cxt.rect = param_ptr->cap_inf_cfg.cfg.src_img_rect;
        CMR_LOGD("prev rect %d %d %d %d", cxt->prev_cxt.rect.start_x,
                 cxt->prev_cxt.rect.start_y, cxt->prev_cxt.rect.width,
                 cxt->prev_cxt.rect.height);
    }
#ifdef CONFIG_CAMERA_DCAM_PDAF
    if (sensor_info.source_width_max == sensor_mode_info->width) {
        param_ptr->cap_inf_cfg.cfg.pdaf_ctrl.mode =
            sn_cxt->cur_sns_ex_info.pdaf_supported;
    } else
        param_ptr->cap_inf_cfg.cfg.pdaf_ctrl.mode = 0;
#endif
    param_ptr->cap_inf_cfg.sensor_id = camera_id;

    sprd_3dnr_type = camera_get_3dnr_flag(cxt);

    if ((((sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW) ||
          (sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_HW_CAP_HW) ||
          (sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_HW_CAP_NULL)) &&
         param_ptr->cap_inf_cfg.cfg.sence_mode == DCAM_SCENE_MODE_PREVIEW) ||
        (((sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_NULL_CAP_HW) ||
          (sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_HW_CAP_HW)) &&
         param_ptr->cap_inf_cfg.cfg.sence_mode == DCAM_SCENE_MODE_CAPTURE) ||
        (sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_HW_VIDEO_HW) ||
        sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
        sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW) {
        // hardware 3dnr
        param_ptr->cap_inf_cfg.cfg.need_3dnr = 1;
    } else {
        param_ptr->cap_inf_cfg.cfg.need_3dnr = 0;
    }

#ifdef CONFIG_SUPPROT_AUTO_3DNR
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get app mode %ld", ret);
    }
    CMR_LOGD("app_mode = %d", setting_param.cmd_type_value);
    if ((setting_param.cmd_type_value == CAMERA_MODE_AUTO_PHOTO) || cxt->_3rd_3dnr_flag
        || (setting_param.cmd_type_value == CAMERA_MODE_TIMELAPSE)) {
        char value[PROPERTY_VALUE_MAX];
        property_get("persist.vendor.cam.preview.3dnr_enable", value, "true");
        if(!strcmp(value,"true") && (param_ptr->cap_inf_cfg.cfg.sence_mode == DCAM_SCENE_MODE_PREVIEW
            || param_ptr->cap_inf_cfg.cfg.sence_mode == DCAM_SCENE_MODE_CAPTURE_CALLBACK)) {
            param_ptr->cap_inf_cfg.cfg.need_3dnr = 1;
        }
    }

#ifdef CONFIG_SUPPROT_FRONT_4IN1_DATA_INTERPOLATION
    if (cxt->camera_id == 1) {
        if (setting_param.cmd_type_value == CAMERA_MODE_PORTRAIT_PHOTO ||
             setting_param.cmd_type_value == CAMERA_MODE_HIGH_RES_PHOTO) {
            // for L5Pro front picture size from 4M to 16M
            cxt->is_support_front_16M = 0;
        } else {
            cxt->is_support_front_16M = 1;
        }
    }
#endif

#ifdef CONFIG_SUPPROT_AUTO_3DNR_IN_HIGH_RES
    if (setting_param.cmd_type_value == CAMERA_MODE_HIGH_RES_PHOTO) {
        // auto 3dnr available in high_res mode
        ret = cmr_grab_auto_3dnr_cfg(cxt->grab_cxt.grab_handle, 1);
        if (ret) {
            CMR_LOGE("failed to cap cfg %ld", ret);
        }
    }
#endif
#endif

    if (1 == sn_cxt->cur_sns_ex_info.embedded_line_enable) {
        SENSOR_VAL_T val;
        struct sensor_embedded_info ebd_info;
        cmr_bzero(&ebd_info, sizeof(ebd_info));

        val.type = SENSOR_VAL_TYPE_GET_EBDLINE_INFO;
        val.pval = &ebd_info;
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                               SENSOR_ACCESS_VAL, (cmr_uint)&val);
        if (ret) {
            CMR_LOGE("get sensor embed line info failed %ld", ret);
            goto exit;
        }

        param_ptr->cap_inf_cfg.cfg.ebd_ctrl.mode = ebd_info.vc_info.vch_mode;
        param_ptr->cap_inf_cfg.cfg.ebd_ctrl.image_vc = ebd_info.vc_info.vch_id;
        param_ptr->cap_inf_cfg.cfg.ebd_ctrl.image_dt =
            ebd_info.vc_info.vch_data_type;
    }

    if (cxt->is_multi_mode == MODE_BOKEH ||
        cxt->is_multi_mode == MODE_3D_CAPTURE ||
        cxt->is_multi_mode == MODE_3D_VIDEO ||
        cxt->is_multi_mode == MODE_3D_CALIBRATION ||
        cxt->is_multi_mode == MODE_BOKEH_CALI_GOLDEN ||
        cxt->is_multi_mode == MODE_3D_PREVIEW ||
        cxt->is_multi_mode == MODE_TUNING) {
        param_ptr->cap_inf_cfg.cfg.dual_cam = 1;
    } else {
        param_ptr->cap_inf_cfg.cfg.dual_cam = 0;
    }

    param_ptr->cap_inf_cfg.cfg.is_high_fps = fps_info.is_high_fps;
    param_ptr->cap_inf_cfg.cfg.high_fps_skip_num = fps_info.high_fps_skip_num;

    /* set 4in1 remosaic_type */
    do {
        cxt->remosaic_type = camera_get_remosaic_type(&(sn_cxt->info_4in1),
                                                      sensor_mode_info->width,
                                                      sensor_mode_info->height);
        if (cxt->remosaic_type == 1)
            param_ptr->cap_inf_cfg.cfg.need_4in1 = 1;
    } while (0);
    if (!param_ptr->is_lightly) {
        ret = cmr_grab_cap_cfg(cxt->grab_cxt.grab_handle,
                               &param_ptr->cap_inf_cfg, channel_id, endian);
        if (ret) {
            CMR_LOGE("failed to cap cfg %ld", ret);
            goto exit;
        }
    }

    cxt->grab_cxt.caller_handle[*channel_id] = caller_handle;
    cxt->grab_cxt.skip_number[*channel_id] = param_ptr->skip_num;
    CMR_LOGD("channel id %d, caller_handle 0x%lx, skip num %d", *channel_id,
             (cmr_uint)caller_handle, param_ptr->skip_num);

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_channel_buff_cfg(cmr_handle oem_handle,
                                struct buffer_cfg *buf_cfg) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = cmr_grab_buff_cfg(cxt->grab_cxt.grab_handle, buf_cfg);
    if (ret) {
        CMR_LOGE("failed to buf cfg %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_channel_cap_cfg(cmr_handle oem_handle, cmr_handle caller_handle,
                               cmr_u32 camera_id, struct cap_cfg *cap_cfg,
                               cmr_u32 *channel_id,
                               struct img_data_end *endian) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (cxt->app_mode == CAMERA_MODE_FDR && cxt->camera_id == 0)
        cap_cfg->cfg.need_fdr = 1;
    else
        cap_cfg->cfg.need_fdr = 0;

    cap_cfg->buffer_cfg_isp = 0;
    cap_cfg->sensor_id = camera_id;
    ret = cmr_grab_cap_cfg(cxt->grab_cxt.grab_handle, cap_cfg, channel_id,
                           endian);
    if (ret) {
        CMR_LOGE("failed to buf cfg %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

// TBD: move this to channel_cfg
#ifdef CONFIG_CAMERA_OFFLINE
cmr_int camera_channel_dcam_size(cmr_handle oem_handle,
                                 struct sprd_dcam_path_size *dcam_cfg) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = cmr_grab_dcam_size(cxt->grab_cxt.grab_handle, dcam_cfg);

    if (ret) {
        CMR_LOGE("failed to buf cfg %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}
#endif

// param channel_bits is used for offline channel(sharkl2-like)
cmr_int camera_channel_start(cmr_handle oem_handle, cmr_u32 channel_bits,
                             cmr_uint skip_number) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    cmr_uint video_snapshot_type = VIDEO_SNAPSHOT_NONE;
    struct sprd_img_capture_param capture_param;
    struct sprd_img_function_mode img_func_mode;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGI("skip num %ld %d", skip_number, channel_bits);
    camera_take_snapshot_step(CMR_STEP_CAP_S);

    if (cxt->remosaic_type) {
        /* 4in1 capture need 3 frame previous skip */
        skip_number = 3;
        CMR_LOGI("skip_num %ld", skip_number);
    }

    ret = cmr_grab_cap_start(cxt->grab_cxt.grab_handle, skip_number);
    if (ret) {
        CMR_LOGE("failed to start cap %ld", ret);
        goto exit;
    }

    if (cxt->is_start_snapshot != 1) {
        cmr_bzero(&setting_param, sizeof(setting_param));
        setting_param.camera_id = cxt->camera_id;
        ret =
            cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                              SETTING_GET_VIDEO_SNAPSHOT_TYPE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get preview sprd eis enabled flag %ld", ret);
            goto exit;
        }
        video_snapshot_type = setting_param.cmd_type_value;

        /* for sharkl2 offline path */
        if ((channel_bits & OFFLINE_CHANNEL_BIT) && cxt->zsl_enabled == 0 &&
            video_snapshot_type != VIDEO_SNAPSHOT_VIDEO) {
            cmr_bzero(&capture_param, sizeof(capture_param));
            capture_param.type = 1;
            ret = cmr_grab_start_capture(cxt->grab_cxt.grab_handle,
                                         capture_param);
            if (ret) {
                CMR_LOGE("failed to start off the fly path,ret %ld", ret);
            }
        }
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    cxt->is_start_snapshot = 0;
    return ret;
}

cmr_int camera_channel_pause(cmr_handle oem_handle, cmr_uint channel_id,
                             cmr_u32 reconfig_flag) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        return ret;
    }
    CMR_LOGI("channel id %ld, reconfig flag %d", channel_id, reconfig_flag);

    /* for sharkl2 offline path */
    if (channel_id == OFFLINE_CHANNEL) {
        ret = cmr_grab_stop_capture(cxt->grab_cxt.grab_handle);
        if (ret) {
            CMR_LOGE("failed to stop off the fly path %ld", ret);
        }
    }

    ret = cmr_grab_cap_pause(cxt->grab_cxt.grab_handle, channel_id,
                             reconfig_flag);
    if (ret) {
        CMR_LOGE("failed to pause channel %ld", ret);
        goto exit;
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_channel_resume(cmr_handle oem_handle, cmr_uint channel_id,
                              cmr_u32 skip_number, cmr_u32 deci_factor,
                              cmr_u32 frm_num) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_sn_cmd_param param;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGI("channel id %ld, skip num %d, deci %d, frm num %d", channel_id,
             skip_number, deci_factor, frm_num);
    camera_set_discard_frame(cxt, 0);
    camera_set_vendor_hdr_ev(oem_handle);

    /* for sharkl2 offline path */
    if (channel_id == OFFLINE_CHANNEL) {
        ret = camera_local_start_capture(oem_handle);
    }

    ret = cmr_grab_cap_resume(cxt->grab_cxt.grab_handle, channel_id,
                              skip_number, deci_factor, frm_num);
    if (ret) {
        CMR_LOGE("failed to resume channel,ret %ld", ret);
        goto exit;
    }

exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_channel_free_frame(cmr_handle oem_handle, cmr_u32 channel_id,
                                  cmr_u32 index) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = cmr_grab_free_frame(cxt->grab_cxt.grab_handle, channel_id, index);
    if (ret) {
        CMR_LOGE("failed to free frame %d %ld", channel_id, ret);
        goto exit;
    }
exit:
    return ret;
}

// channel_bits is used for offline channel(sharkl2-like)
cmr_int camera_channel_stop(cmr_handle oem_handle, cmr_u32 channel_bits) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    /* for sharkl2 offline path */
    if (channel_bits & OFFLINE_CHANNEL_BIT) {
        ret = cmr_grab_stop_capture(cxt->grab_cxt.grab_handle);
        if (ret) {
            CMR_LOGE("failed to stop off the fly path %ld", ret);
        }
    }
    cxt->snp_cxt.start_capture_flag = 0;
    // sharkl5 TBD remove these code
    /*
        ret = isp_sw_stop(isp_cxt->isp_handle);
        if (ret) {
            CMR_LOGE("failed to isp sw stop %ld", ret);
        }
    */
    ret = cmr_grab_cap_stop(cxt->grab_cxt.grab_handle);
    if (ret) {
        CMR_LOGE("failed to stop channel %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_channel_scale_capability(cmr_handle oem_handle, cmr_u32 *width,
                                        cmr_u32 *sc_factor,
                                        cmr_u32 *sc_threshold) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = cmr_grab_scale_capability(cxt->grab_cxt.grab_handle, width, sc_factor,
                                    sc_threshold);
exit:
    return ret;
}

cmr_int camera_channel_path_capability(cmr_handle oem_handle,
                                       struct cmr_path_capability *capability) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    if (!oem_handle || !capability) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = cmr_grab_path_capability(cxt->grab_cxt.grab_handle, capability);
exit:
    return ret;
}
cmr_int camera_channel_get_cap_time(cmr_handle oem_handle, cmr_u32 *sec,
                                    cmr_u32 *usec) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !sec | !usec) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = cmr_grab_get_cap_time(cxt->grab_cxt.grab_handle, sec, usec);
exit:
    return ret;
}

cmr_int camera_get_sensor_info(cmr_handle oem_handle, cmr_uint sensor_id,
                               struct sensor_exp_info *exp_info_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !exp_info_ptr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret =
        cmr_sensor_get_info(cxt->sn_cxt.sensor_handle, sensor_id, exp_info_ptr);
    if (ret) {
        CMR_LOGE("failed to get sensor info %ld", ret);
        goto exit;
    }

exit:
    return ret;
}

cmr_int camera_get_sensor_fps_info(cmr_handle oem_handle, cmr_uint sensor_id,
                                   cmr_u32 sn_mode,
                                   struct sensor_mode_fps_tag *fps_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !fps_info) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = cmr_sensor_get_fps_info(cxt->sn_cxt.sensor_handle, sensor_id, sn_mode,
                                  fps_info);
    if (ret) {
        CMR_LOGE("failed to get sensor info %ld", ret);
        goto exit;
    }
exit:
    return ret;
}

cmr_int
camera_get_tuning_info(cmr_handle oem_handle,
                       struct isp_adgain_exp_info *adgain_exp_info_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct common_isp_cmd_param isp_param;

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    adgain_exp_info_ptr->adgain = isp_param.isp_adgain.adgain;
    adgain_exp_info_ptr->exp_time = isp_param.isp_adgain.exp_time;
    adgain_exp_info_ptr->bv = isp_param.isp_adgain.bv;
    CMR_LOGV("adgain=%d, exp_time=%d, bv=%d", adgain_exp_info_ptr->adgain,
             adgain_exp_info_ptr->exp_time, adgain_exp_info_ptr->bv);
    return ret;
}

cmr_int
camera_get_adgain_exp_info(cmr_handle oem_handle,
                       struct isp_adgain_exp_info *adgain_exp_info_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct common_isp_cmd_param isp_param;

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    adgain_exp_info_ptr->adgain = isp_param.isp_adgain.adgain;
    adgain_exp_info_ptr->exp_time = isp_param.isp_adgain.exp_time;
    adgain_exp_info_ptr->bv = isp_param.isp_adgain.bv;
    CMR_LOGD("adgain=%d, exp_time=%d, bv=%d", adgain_exp_info_ptr->adgain,
             adgain_exp_info_ptr->exp_time, adgain_exp_info_ptr->bv);
    return ret;
}

cmr_int camera_get_sensor_autotest_mode(cmr_handle oem_handle,
                                        cmr_uint sensor_id,
                                        cmr_uint *is_autotest) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !is_autotest) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        return ret;
    }
    CMR_LOGI("sensor id %ld", sensor_id);
    ret = cmr_sensor_get_autotest_mode(cxt->sn_cxt.sensor_handle, sensor_id,
                                       is_autotest);
    if (ret) {
        CMR_LOGE("failed to get sensor info %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("ret %ld, is_autotest %ld", ret, *is_autotest);
    return ret;
}

cmr_int cmr_get_leds_ctrl(cmr_handle oem_handle, cmr_u32 *led0_enable,
                          cmr_u32 *led1_enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct common_isp_cmd_param isp_param;

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_LEDS_CTRL, &isp_param);
    if (ret) {
        CMR_LOGE("failed to get leds_ctrl %ld", ret);
        goto exit;
    }

    *led0_enable = isp_param.leds_ctrl.led0_ctrl;
    *led1_enable = isp_param.leds_ctrl.led1_ctrl;

exit:
    return ret;
}

cmr_int camera_ioctl_for_setting(cmr_handle oem_handle, cmr_uint cmd_type,
                                 struct setting_io_parameter *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_handle grab_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    grab_handle = cxt->grab_cxt.grab_handle;

    switch (cmd_type) {
    case SETTING_IO_GET_CAPTURE_SIZE:
        param_ptr->size_param = cxt->snp_cxt.post_proc_setting.snp_size;
        break;
    case SETTING_IO_GET_ACTUAL_CAPTURE_SIZE:
        param_ptr->size_param =
            cxt->snp_cxt.post_proc_setting.dealign_actual_snp_size;
        break;
    case SETTING_IO_CTRL_FLASH: {
        struct grab_flash_opt flash_opt;

        if (FLASH_OPEN == param_ptr->cmd_value ||
            FLASH_HIGH_LIGHT == param_ptr->cmd_value ||
            FLASH_TORCH == param_ptr->cmd_value) {
            cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                SENSOR_EXIF_CTRL_FLASH, 1);
        }
        char value1[PROPERTY_VALUE_MAX];
        property_get("persist.vendor.cam.flash.torch.cfg", value1, "02");

        /*cfg torch value*/
        if (param_ptr->cmd_value == FLASH_TORCH) {
            struct sprd_flash_cfg_param cfg;
            cfg.real_cell.type = FLASH_TYPE_TORCH;
            cfg.real_cell.count = 1;
            cfg.real_cell.led_idx = 0x01;                  /*main*/
            cfg.real_cell.element[0].index = atoi(value1); // 0x06;
            cfg.real_cell.element[0].val = 0;
            cfg.io_id = FLASH_IOID_SET_CHARGE;
            cfg.flash_idx = cxt->facing % 2;
            if (!camera_front_lcd_flash_activie(cfg.flash_idx))
                ret = cmr_grab_cfg_flash(grab_handle, &cfg);
        }
        cmr_bzero(&flash_opt, sizeof(flash_opt));

        if ((param_ptr->cmd_value == FLASH_OPEN ||
             param_ptr->cmd_value == FLASH_HIGH_LIGHT) &&
            (cxt->is_multi_mode == MODE_MULTI_CAMERA || cxt->camera_id == 0 ||
             cxt->camera_id == 4)) {
            cmr_get_leds_ctrl(oem_handle, &flash_opt.led0_enable,
                              &flash_opt.led1_enable);
        } else {
            flash_opt.led0_enable = 1;
            flash_opt.led1_enable = 1;
        }

        flash_opt.flash_mode = param_ptr->cmd_value;
        flash_opt.flash_index = cxt->facing % 2;
        CMR_LOGV("led0_enable=%d, led1_enable=%d", flash_opt.led0_enable,
                 flash_opt.led1_enable);
        if (camera_front_lcd_flash_activie(flash_opt.flash_index)) {
            camera_front_lcd_flash_callback(cxt, flash_opt.flash_mode);
        } else {

#if defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_4) ||                      \
    defined(CONFIG_ISP_2_6) || defined(CONFIG_ISP_2_5) ||                      \
    defined(CONFIG_ISP_2_7)
            if (param_ptr->cmd_value == FLASH_CLOSE_AFTER_OPEN ||
                param_ptr->cmd_value == FLASH_OPEN) {
                cmr_u32 flash_capture_skip_num = 0;
                bool isFrontFlash =
                    (strcmp(FRONT_CAMERA_FLASH_TYPE, "flash") == 0) ? true
                                                                    : false;
                ret = isp_ioctl(cxt->isp_cxt.isp_handle,
                                ISP_CTRL_GET_FLASH_SKIP_FRAME_NUM,
                                &flash_capture_skip_num);
                if (ret) {
                    CMR_LOGE("failed to get preflash skip number %ld", ret);
                }
                CMR_LOGD("preflash_skip_num = %d", flash_capture_skip_num);
                cxt->flash_skip_frame_num =
                    (flash_capture_skip_num == 0) ? 1 : flash_capture_skip_num;
                cxt->flash_skip_frame_enable = 1;
                cxt->flash_skip_frame_cnt = 0;
                cxt->flash_handle_timestamp = systemTime(SYSTEM_TIME_BOOTTIME);
                CMR_LOGD("flash_skip_frame_num %d flash_skip_frame_cnt %d",
                         cxt->flash_skip_frame_num, cxt->flash_skip_frame_cnt);
            }
#endif
            cmr_grab_flash_cb(grab_handle, &flash_opt);
        }
    } break;
    case SETTING_IO_GET_PREVIEW_MODE:
        param_ptr->cmd_value = cxt->prev_cxt.preview_sn_mode;
        break;
    case SETTING_IO_GET_FLASH_MAX_CAPACITY: {
#if 0
        struct sprd_flash_capacity capacity;
        struct sprd_flash_cfg_param cfg;
        cfg.io_id = FLASH_IOID_GET_MAX_CAPACITY;
        cfg.data = &capacity;
        ret = cmr_grab_cfg_flash(cxt->grab_cxt.grab_handle, &cfg);
        if (0 == ret) {
            param_ptr->flash_capacity.max_charge = capacity.max_charge;
            param_ptr->flash_capacity.max_time = capacity.max_time;
        }
#endif
    }
    case SETTING_IO_SET_TOUCH: {
        prev_set_preview_touch_info(cxt->prev_cxt.preview_handle,
                                    cxt->camera_id, &param_ptr->touch_xy);
    } break;

    default:
        CMR_LOGE("don't support cmd %ld", cmd_type);
        ret = CMR_CAMERA_NO_SUPPORT;
        break;
    }
exit:
    return ret;
}

cmr_int camera_sensor_ioctl(cmr_handle oem_handle, cmr_uint cmd_type,
                            struct common_sn_cmd_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_uint cmd = SENSOR_CMD_MAX;
    cmr_uint sensor_param = 0;
    cmr_uint set_exif_flag = 0;
    SENSOR_EXIF_CTRL_E exif_cmd;
    SENSOR_EXT_FUN_PARAM_T_PTR hdr_ev_param_ptr = 0;
    SENSOR_EXT_FUN_PARAM_T_PTR fdr_ev_param_ptr = 0;
    SENSOR_VAL_T val;
    struct frame_4in1_info img_addr;

    if (!oem_handle || !param_ptr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGV("cmd_type =%ld", cmd_type);

    switch (cmd_type) {
    case COM_SN_GET_AUTO_FLASH_STATE:
        cmd = SENSOR_CHECK_NEED_FLASH;
        sensor_param = (cmr_uint)&param_ptr->cmd_value;
        break;
    case COM_SN_SET_BRIGHTNESS:
        cmd = SENSOR_BRIGHTNESS;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_BRIGHTNESSVALUE;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_CONTRAST:
        cmd = SENSOR_CONTRAST;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_CONTRAST;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_SATURATION:
        cmd = SENSOR_SATURATION;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_SATURATION;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_SHARPNESS:
        cmd = SENSOR_SHARPNESS;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_SHARPNESS;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_IMAGE_EFFECT:
        cmd = SENSOR_IMAGE_EFFECT;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_EXPOSURE_COMPENSATION:
        cmd = SENSOR_EXPOSURE_COMPENSATION;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_EXPOSURE_LEVEL:
        cmd = SENSOR_WRITE_EV;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_WB_MODE:
        cmd = SENSOR_SET_WB_MODE;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_LIGHTSOURCE;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_PREVIEW_MODE:
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_SCENECAPTURETYPE;
        break;
    case COM_SN_SET_ANTI_BANDING:
        cmd = SENSOR_FLIP;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_ISO:
        cmd = SENSOR_ISO;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_VIDEO_MODE:
        cmd = SENSOR_VIDEO_MODE;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_FPS_LLS_MODE:
        break;
    case COM_SN_SET_BEFORE_SNAPSHOT:
        cmd = SENSOR_BEFORE_SNAPSHOT;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_AFTER_SNAPSHOT:
        cmd = SENSOR_AFTER_SNAPSHOT;
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_EXT_FUNC:
        break;
    case COM_SN_SET_AE_ENABLE: // don't support
        break;
    case COM_SN_SET_EXIF_FOCUS:
        sensor_param = param_ptr->cmd_value;
        break;
    case COM_SN_SET_FOCUS:
        cmd = SENSOR_FOCUS;
        sensor_param = (cmr_uint) & (param_ptr->yuv_sn_af_param);
        CMR_LOGE("COM_SN_SET_FOCUS cmd =%ld ", cmd);
        break;
    case COM_SN_GET_PREVIEW_MODE:
        break;
    case COM_SN_GET_CAPTURE_MODE:
        break;
    case COM_SN_GET_SENSOR_ID:
        break;
    case COM_SN_GET_VIDEO_MODE:
        break;
    case COM_SN_GET_EXIF_IMAGE_INFO:
        ret = cmr_sensor_get_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                  &param_ptr->exif_pic_info);
        if (ret) {
            CMR_LOGE("sn get exif image info failed!");
        } else {
            memcpy((void *)&cxt->sn_cxt.exif_info,
                   (void *)&param_ptr->exif_pic_info,
                   sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
        }
        return ret;
    case COM_SN_SET_HDR_EV: {
        cmd = SENSOR_SET_HDR_EV;
        hdr_ev_param_ptr = malloc(sizeof(SENSOR_EXT_FUN_PARAM_T));
        if (!hdr_ev_param_ptr) {
            CMR_LOGE("fail to malloc");
            return CMR_CAMERA_NO_MEM;
        }
        hdr_ev_param_ptr->cmd = SENSOR_EXT_EV;
        hdr_ev_param_ptr->param = param_ptr->cmd_value;
        sensor_param = (cmr_uint)hdr_ev_param_ptr;
        CMR_LOGI("vendor hdr ev_val = %d", param_ptr->cmd_value);
        break;
    }
    case COM_SN_GET_INFO:
        ret = cmr_sensor_get_info(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                  &param_ptr->sensor_static_info);
        if (ret) {
            CMR_LOGE("sn get info failed!");
        }
        return ret;
    case COM_SN_GET_FLASH_LEVEL:
        ret = cmr_sensor_get_flash_info(
            cxt->sn_cxt.sensor_handle, cxt->camera_id, &param_ptr->flash_level);
        if (ret) {
            CMR_LOGE("sn get flash level failed!");
        }
        CMR_LOGI("flash level low_light = %d, high_light = %d",
                 param_ptr->flash_level.low_light,
                 param_ptr->flash_level.high_light);
        return ret;
    case COM_SN_GET_4IN1_FORMAT_CONVERT:
        bzero(&img_addr, sizeof(struct frame_4in1_info));
#if 1
        struct tuning_param_info tuning_info;
        bzero(&tuning_info, sizeof(struct tuning_param_info));
        camera_local_get_tuning_param(oem_handle, &tuning_info);

        img_addr.awb_gain.r_gain = tuning_info.awb_info.r_gain;
        img_addr.awb_gain.b_gain = tuning_info.awb_info.b_gain;
        img_addr.awb_gain.g_gain = tuning_info.awb_info.g_gain;
#endif
        //        img_addr.awb_gain.r_gain= tuning_info.awb_info.r_gain;
        //        camera_local_get_tuning_param(oem_handle,
        //        img_addr.tuning_info);
        img_addr.im_addr_in = param_ptr->postproc_info.src.addr_vir.addr_y;
        img_addr.im_addr_out = param_ptr->postproc_info.dst.addr_vir.addr_y;
        cmd = SENSOR_ACCESS_VAL;
        val.type = SENSOR_VAL_TYPE_4IN1_PROC;
        val.pval = &img_addr;
        sensor_param = (cmr_uint)&val;
        CMR_LOGD("4in1 raw convert:0x%lx,0x%lx", img_addr.im_addr_in,
                 img_addr.im_addr_out);
        break;
    default:
        CMR_LOGE("don't support cmd %ld", cmd_type);
        ret = CMR_CAMERA_NO_SUPPORT;
        break;
    }
    if (!ret) {
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id, cmd,
                               sensor_param);
        if ((COM_SN_SET_HDR_EV == cmd_type) && hdr_ev_param_ptr) {
            free(hdr_ev_param_ptr);
            hdr_ev_param_ptr = 0;
        }
        if (ret) {
            CMR_LOGE("failed to sn ioctrl %ld", ret);
        }
        if (set_exif_flag) {
            CMR_LOGD("ERIC set exif");
            if (cmd_type == COM_SN_SET_WB_MODE) {
                cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                    exif_cmd, param_ptr->cmd_value);
                cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                    SENSOR_EXIF_CTRL_WHITEBALANCE,
                                    param_ptr->cmd_value);
            } else
                cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                    exif_cmd, sensor_param);
        }
    }
exit:
    return ret;
}

cmr_uint camera_param_to_isp(cmr_uint cmd, struct common_isp_cmd_param *parm) {
    cmr_uint in_param = parm->cmd_value;
    cmr_uint out_param = in_param;

    switch (cmd) {
    case COM_ISP_SET_AWB_MODE: {
        switch (in_param) {
        case CAMERA_WB_AUTO:
            out_param = ISP_AWB_AUTO;
            break;

        case CAMERA_WB_INCANDESCENT:
            out_param = ISP_AWB_INDEX1;
            break;

        case CAMERA_WB_FLUORESCENT:
            out_param = ISP_AWB_INDEX4;
            break;

        case CAMERA_WB_DAYLIGHT:
            out_param = ISP_AWB_INDEX5;
            break;

        case CAMERA_WB_CLOUDY_DAYLIGHT:
            out_param = ISP_AWB_INDEX6;
            break;

        default:
            break;
        }
        break;
    }
    case COM_ISP_SET_EV: {
        switch (in_param) {
        case 0:
            out_param = 1;
            break;

        case 1:
            out_param = 2;
            break;

        case 2:
            out_param = 3;
            break;

        case 3:
            out_param = 4;
            break;

        case 4:
            out_param = 5;
            break;

        case 5:
            out_param = 6;
            break;

        case 6:
            out_param = 7;
            break;

        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    return out_param;
}

cmr_int camera_isp_ev_switch(struct common_isp_cmd_param *parm) {
    cmr_int in_param = parm->ae_compensation_param.ae_exposure_compensation;
    cmr_int out_param = in_param;
    switch (in_param) {
    case -6:
        out_param = 0;
        break;

    case -4:
        out_param = 1;
        break;

    case -2:
        out_param = 2;
        break;

    case 0:
        out_param = 3;
        break;

    case 2:
        out_param = 4;
        break;

    case 4:
        out_param = 5;
        break;

    case 6:
        out_param = 6;
        break;

    default:
        break;
    }
    return out_param;
}

cmr_int camera_local_get_last_preflash_time(cmr_handle oem_handle, cmr_s64 *time){
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));

    setting_param.camera_id = cxt->camera_id;
    //setting_param.setting_flash_status = SETTING_AF_FLASH_PRE_LIGHTING;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_LAST_PREFLASH_TIME,
                            &setting_param);
    *time = setting_param.last_preflash_time;

    return ret;
}

cmr_int camera_local_get_isp_info(cmr_handle oem_handle, void **addr,
                                  int *size, cmr_s32 frame_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct isp_info isp_info;

    if (!oem_handle || !addr || !size) {
        CMR_LOGE("err,invlid param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    *addr = NULL;
    *size = 0;
    cmr_bzero(&isp_info, sizeof(isp_info));
    isp_info.frame_id = frame_id;
    if (CAM_IMG_FMT_BAYER_MIPI_RAW == cxt->sn_cxt.sensor_info.image_format) {
        ret = isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_GET_INFO,
                        (void *)&isp_info);
        if (!ret) {
            *addr = isp_info.addr;
            *size = isp_info.size;
        } else {
            CMR_LOGE("fail to get isp information");
        }
    }

    CMR_LOGD("%p %d", *addr, *size);
    return ret;
}

cmr_int camera_isp_ioctl(cmr_handle oem_handle, cmr_uint cmd_type,
                         struct common_isp_cmd_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_u32 isp_cmd = ISP_CTRL_MAX;
    cmr_u64 isp_param = 0;
    void *isp_param_ptr = NULL;
    cmr_u32 ptr_flag = 0;
    cmr_uint set_exif_flag = 0;
    cmr_uint set_isp_flag = 1;
    SENSOR_EXIF_CTRL_E exif_cmd = SENSOR_EXIF_CTRL_MAX;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct isp_pos_rect trim;
    struct isp_range_fps isp_fps;
    struct isp_ae_fps ae_fps;
    struct isp_hdr_param hdr_param;
    struct isp_snp_ae_param snp_ae_param;
    struct isp_fdr_param fdr_param;

    struct isp_3dnr_ctrl_param param_3dnr;
    struct isp_exp_compensation ae_compensation;
#ifdef CONFIG_ISP_2_7
    struct isp_gtm_switch_param gtm_switch;
#endif
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    if (!oem_handle || !param_ptr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    switch (cmd_type) {
    case COM_ISP_SET_AE_MODE:
        if (cxt->is_lls_enable) {
            isp_cmd = ISP_CTRL_SET_AE_NIGHT_MODE;
        } else {
            isp_cmd = ISP_CTRL_SCENE_MODE;
        }

        {
            struct sensor_raw_info *raw_info_ptr = NULL;
            struct sensor_libuse_info *libuse_info = NULL;
            cmr_int product_id = 0;

            /*for third ae*/
            raw_info_ptr = cxt->sn_cxt.sensor_info.raw_info_ptr;
            if (raw_info_ptr) {
                libuse_info = raw_info_ptr->libuse_info;
                if (libuse_info) {
                    product_id = libuse_info->ae_lib_info.product_id;
                }
            }
            if (product_id) {
                isp_cmd = ISP_CTRL_SCENE_MODE;
            }
        }
        struct setting_cmd_parameter setting_param;
        cmr_bzero(&setting_param, sizeof(setting_param));
        setting_param.camera_id = cxt->camera_id;
        ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                                &setting_param);
        if (ret) {
            CMR_LOGE("failed to get app mode %ld", ret);
            ret = 0;
        }
        CMR_LOGD("setting_param.cmd_type_value %d", setting_param.cmd_type_value);

        if (setting_param.cmd_type_value == CAMERA_MODE_MANUAL) {
            param_ptr->cmd_value = ISP_PROFESSION;
        } else if (setting_param.cmd_type_value == CAMERA_MODE_CONTINUE) {
            param_ptr->cmd_value = ISP_CONTINUOUSPICTURE;
        } else if (setting_param.cmd_type_value == CAMERA_MODE_FILTER) {
            param_ptr->cmd_value = ISP_FILTER;
        } else if (setting_param.cmd_type_value == CAMERA_MODE_TIMELAPSE) {
            param_ptr->cmd_value = ISP_DELAYVIDEO;
        } else if (setting_param.cmd_type_value == CAMERA_MODE_INTERVAL) {
            param_ptr->cmd_value = ISP_INTERVAL;
        } else if(setting_param.cmd_type_value == CAMERA_MODE_REFOCUS) {
            param_ptr->cmd_value = ISP_BOKEHMODE;
        } else if (setting_param.cmd_type_value == CAMERA_MODE_PORTRAIT_PHOTO){
            param_ptr->cmd_value = ISP_PORTRAIT;
        } else if (setting_param.cmd_type_value == -1) {
            switch (cxt->app_id) {
            case TOP_APP_FACEBOOK:
                param_ptr->cmd_value = ISP_FACEBOOK;
                break;
            case TOP_APP_MESSENGER:
                param_ptr->cmd_value = ISP_MESSENGER;
                break;
            case TOP_APP_QQ:
                param_ptr->cmd_value = ISP_QQ;
                break;
            case TOP_APP_SNAPCHAT:
                param_ptr->cmd_value = ISP_SNAPCHAT;
                break;
            case TOP_APP_WHATSAPP:
                param_ptr->cmd_value = ISP_WHATAPPS;
                break;
            case TOP_APP_WECHAT:
                param_ptr->cmd_value = ISP_WECHAT;
                break;
            default:
                param_ptr->cmd_value = ISP_THIRDPARTY;
                break;
           }
        }

        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_SCENECAPTURETYPE;
        isp_param = param_ptr->cmd_value;
        if (ISP_AE_MODE_MAX == isp_param) {
            set_isp_flag = 0;
        }
        CMR_LOGD("ae scene mode %ld", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_EXPOSURE_TIME:
        CMR_LOGD("exposure time %" PRIu64 "", param_ptr->cmd_value);
        isp_cmd = ISP_CTRL_SET_AE_EXP_TIME;
        isp_param = param_ptr->cmd_value;
        cxt->exp_time = isp_param;
        break;
    case COM_ISP_SET_AE_MEASURE_LUM:
        isp_cmd = ISP_CTRL_AE_MEASURE_LUM;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("aw measure lum %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_AE_METERING_AREA:
        isp_cmd = ISP_CTRL_AE_TOUCH;
        trim.start_x = param_ptr->win_area.rect[0].start_x;
        trim.start_y = param_ptr->win_area.rect[0].start_y;
        if (0 == param_ptr->win_area.rect[0].width)
            trim.end_x = 0;
        else
            trim.end_x = param_ptr->win_area.rect[0].start_x +
                         param_ptr->win_area.rect[0].width - 1;

        if (0 == param_ptr->win_area.rect[0].height)
            trim.end_y = 0;
        else
            trim.end_y = param_ptr->win_area.rect[0].start_y +
                         param_ptr->win_area.rect[0].height - 1;

        CMR_LOGD("AE ROI (%d,%d,%d,%d)", trim.start_x, trim.start_y, trim.end_x,
                 trim.end_y);
        ptr_flag = 1;
        isp_param_ptr = (void *)&trim;
        break;
    case COM_ISP_SET_BRIGHTNESS:
        isp_cmd = ISP_CTRL_BRIGHTNESS;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_BRIGHTNESSVALUE;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("brightness %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_CONTRAST:
        isp_cmd = ISP_CTRL_CONTRAST;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_CONTRAST;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("contrast %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_SATURATION:
        isp_cmd = ISP_CTRL_SATURATION;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_SATURATION;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("saturation %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_SHARPNESS:
        isp_cmd = ISP_CTRL_SHARPNESS;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_SHARPNESS;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("sharpness %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_SPECIAL_EFFECT:
        isp_cmd = ISP_CTRL_SPECIAL_EFFECT;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_SET_EV:
        isp_cmd = ISP_CTRL_AE_EXP_COMPENSATION;
        ptr_flag = 1;
        ae_compensation.comp_val =
            param_ptr->ae_compensation_param.ae_exposure_compensation;
        ae_compensation.comp_range.min =
            param_ptr->ae_compensation_param.ae_compensation_range[0];
        ae_compensation.comp_range.max =
            param_ptr->ae_compensation_param.ae_compensation_range[1];
        ae_compensation.step_numerator =
            param_ptr->ae_compensation_param.ae_compensation_step_numerator;
        ae_compensation.step_denominator =
            param_ptr->ae_compensation_param.ae_compensation_step_denominator;
        isp_param_ptr = (void *)&ae_compensation;
        CMR_LOGD("ae compensation: comp_val=%d, range.min=%d, range.max=%d, "
                 "step_numerator=%d, step_denominator=%d",
                 ae_compensation.comp_val, ae_compensation.comp_range.min,
                 ae_compensation.comp_range.max, ae_compensation.step_numerator,
                 ae_compensation.step_denominator);
        break;
    case COM_ISP_SET_AWB_MODE:
        CMR_LOGD("awb mode 00 %d isp param %d", param_ptr->cmd_value,
                 isp_param);
        isp_cmd = ISP_CTRL_AWB_MODE;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_LIGHTSOURCE;
        isp_param = camera_param_to_isp(COM_ISP_SET_AWB_MODE, param_ptr);
        CMR_LOGD("awb mode %d isp param %d", param_ptr->cmd_value, isp_param);
        break;
    case COM_ISP_SET_ANTI_BANDING:
        isp_cmd = ISP_CTRL_FLICKER;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("flicker %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_ISO:
        isp_cmd = ISP_CTRL_ISO;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_ISOSPEEDRATINGS;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("iso %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_VIDEO_MODE:
        isp_cmd = ISP_CTRL_VIDEO_MODE;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("isp video mode %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_FLASH_EG:
        isp_cmd = ISP_CTRL_FLASH_EG;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_GET_EXIF_IMAGE_INFO:
        isp_cmd = ISP_CTRL_GET_EXIF_INFO;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->exif_pic_info;
        break;
    case COM_ISP_GET_EXIF_DEBUG_INFO:
        isp_cmd = ISP_CTRL_GET_EXIF_DEBUG_INFO;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->isp_dbg_info;
        break;
    case COM_ISP_GET_CUR_ADGAIN_EXP:
        isp_cmd = ISP_CTRL_GET_CUR_ADGAIN_EXP;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->isp_adgain;
        break;
    case COM_ISP_GET_CUR_COL_TEM:
        isp_cmd = ISP_CTRL_GET_AWB_CT;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->isp_cur_ct;
        break;
    case COM_ISP_GET_CUR_SENS:
        isp_cmd = ISP_CUR_ISO;
        isp_param_ptr = (void *)&param_ptr->isp_cur_iso;
        ret = isp_capability(isp_cxt->isp_handle, isp_cmd, isp_param_ptr);
        if (ret) {
            CMR_LOGE("Failed to read isp capability ret = %ld", ret);
        }
        return ret;
    case COM_ISP_SET_AF:
        isp_cmd = ISP_CTRL_AF;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->af_param;
        CMR_LOGD("isp_cmd =%d", isp_cmd);
        break;
    case COM_ISP_SET_AF_MODE:
        isp_cmd = ISP_CTRL_AF_MODE;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("af mode %d", param_ptr->cmd_value);
        break;
    case COM_ISP_SET_AF_STOP:
        isp_cmd = ISP_CTRL_AF_STOP;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("isp_cmd =%d af mode %d", isp_cmd, param_ptr->cmd_value);
        break;
    case COM_ISP_GET_LOW_LUX_EB:
        isp_cmd = ISP_LOW_LUX_EB;
        isp_param_ptr = (void *)&param_ptr->cmd_value;

        ret = isp_capability(isp_cxt->isp_handle, isp_cmd, isp_param_ptr);
        if (ret) {
            CMR_LOGE("Failed to read isp capability ret = %ld", ret);
        }
        return ret;
    case COM_ISP_SET_FLASH_NOTICE:
        isp_cmd = ISP_CTRL_FLASH_NOTICE;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->flash_notice;
        CMR_LOGD("isp_cmd = %d, mode = %d", isp_cmd,
                 param_ptr->flash_notice.mode);
        break;
    case COM_ISP_SET_FACE_AREA:
        isp_cmd = ISP_CTRL_FACE_AREA;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->fd_param;
        CMR_LOGD("isp_cmd = %d, face_num = %d", isp_cmd,
                 param_ptr->fd_param.face_num);
        break;
    case COM_ISP_SET_RANGE_FPS:
        isp_cmd = ISP_CTRL_RANGE_FPS;
        ptr_flag = 1;
        isp_fps.min_fps = param_ptr->range_fps.min_fps;
        isp_fps.max_fps = param_ptr->range_fps.max_fps;
        isp_param_ptr = (void *)&isp_fps;
        break;

    case COM_ISP_SET_FPS_LLS_MODE:
        isp_cmd = ISP_CTRL_SET_AE_FPS;
        ae_fps = param_ptr->fps_param;
        ptr_flag = 1;
        isp_param_ptr = (void *)&ae_fps;
        break;

    case COM_ISP_SET_HDR:
        isp_cmd = ISP_CTRL_HDR;
        hdr_param.hdr_enable = param_ptr->cmd_value;
        hdr_param.ev_effect_valid_num =
            cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1;
        ptr_flag = 1;
        CMR_LOGD("set HDR enable %d, ev_effect_valid_num %d",
                 hdr_param.hdr_enable, hdr_param.ev_effect_valid_num);
        isp_param_ptr = (void *)&hdr_param;
        break;
    case COM_ISP_LOCK_NR_SMART: // as COM_ISP_SET_HDR, but no AE_HDR_START
        isp_cmd = ISP_CTRL_LOCK_NR_SMART;
        hdr_param.hdr_enable = param_ptr->cmd_value;
        hdr_param.ev_effect_valid_num =
            cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1;
        ptr_flag = 1;
        CMR_LOGD("set lock %d, ev_effect_valid_num %d",
                 hdr_param.hdr_enable, hdr_param.ev_effect_valid_num);
        isp_param_ptr = (void *)&hdr_param;
        break;
    case COM_ISP_SET_FDR:
        fdr_param.fdr_enable = param_ptr->cmd_value;
        isp_cmd = fdr_param.fdr_enable ? ISP_CTRL_START_FDR : ISP_CTRL_STOP_FDR;
        fdr_param.ev_effect_valid_num =
            cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1;
        fdr_param.ev_effect_cnt = cxt->fdr_total_frame_cnt;  //2~10 frames;
        ptr_flag = 1;
        CMR_LOGD("set fdr enable %d, ev_effect_valid_num %d, fdr frame cnt:%d",
                 fdr_param.fdr_enable, fdr_param.ev_effect_valid_num, fdr_param.ev_effect_cnt);
        isp_param_ptr = (void *)&fdr_param;
        break;

    case COM_ISP_SET_AE_ADJUST:
        isp_cmd = ISP_CTRL_SET_AE_ADJUST;
        snp_ae_param = param_ptr->snp_ae_param;
        if (snp_ae_param.type == SNAPSHOT_GTM) {
            snp_ae_param.ev_effect_valid_num =
                cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1;
            snp_ae_param.ev_adjust_count = 2;
            snp_ae_param.type = SNAPSHOT_GTM;
        } else if (snp_ae_param.type == SNAPSHOT_DRE) {
            snp_ae_param.ev_effect_valid_num =
                cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1;
            snp_ae_param.ev_adjust_count = 1;
            snp_ae_param.type = SNAPSHOT_DRE;
        } else if (snp_ae_param.type == SNAPSHOT_NIGHT_DNS) {
            CMR_LOGD("NIGHT_DNS");
        }
        ptr_flag = 1;
        CMR_LOGD("set ae enable %d", snp_ae_param.enable);
        isp_param_ptr = (void *)&snp_ae_param;
        break;

#ifdef CONFIG_ISP_2_7
    case COM_ISP_SET_GTM_ONFF:
        isp_cmd = ISP_CTRL_SET_GTM_ONFF;
        gtm_switch.enable = param_ptr->cmd_value;
        ptr_flag = 1;
        isp_param_ptr = (void *)&gtm_switch;
        CMR_LOGD("set gtm on off value =%d", gtm_switch.enable);
        break;
#endif

    case COM_ISP_SET_3DNR:
        isp_cmd = ISP_CTRL_3DNR;
        param_3dnr.enable = param_ptr->cmd_value;
        ptr_flag = 1;
        CMR_LOGD("set 3dnr enable %d", param_3dnr.enable);
        isp_param_ptr = (void *)&param_3dnr;
        break;

    case COM_ISP_SET_AE_LOCK_UNLOCK:
        isp_cmd = ISP_CTRL_SET_AE_LOCK_UNLOCK;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("set AE Lock & Unlock %d", isp_param);
        break;

    case COM_ISP_SET_ROI_CONVERGENCE_REQ:
        isp_cmd = ISP_CTRL_SET_CONVERGENCE_REQ;
        ptr_flag = 1;
        /**add for 3d calibration set 3d calibration flag begin*/
        CMR_LOGD("ISP_CTRL_SET_CONVERGENCE_REQ, is 3dcalibration enable:%d, "
                 "params value:%d",
                 cxt->is_3dcalibration_mode, param_ptr->cmd_value);
        if (cxt->is_3dcalibration_mode && 1 == param_ptr->cmd_value) {
            ptr_flag = 0;
            isp_param = param_ptr->cmd_value;
            CMR_LOGD(
                "ISP_CTRL_SET_CONVERGENCE_REQ, isp_param:%d, &isp_param:%p",
                isp_param, &isp_param);
        }
        /**add for 3d calibration set 3d calibration flag end*/
        CMR_LOGD("ISP_CTRL_SET_CONVERGENCE_REQ");
        break;

    case COM_ISP_SET_SNAPSHOT_FINISHED:
        isp_cmd = ISP_CTRL_SET_SNAPSHOT_FINISHED;
        ptr_flag = 1;
        CMR_LOGD("ISP_CTRL_SET_SNAPSHOT_FINISHED");
        break;

    case COM_ISP_SET_FLASH_MODE:
        isp_cmd = ISP_CTRL_SET_FLASH_MODE;
        isp_param = param_ptr->cmd_value;
        cxt->flash_mode = param_ptr->cmd_value;
        CMR_LOGD("flash mode %d", param_ptr->cmd_value);
        break;

    case COM_ISP_GET_YIMG_INFO:
        isp_cmd = ISP_CTRL_GET_YIMG_INFO;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->isp_yimg;
        break;

    case COM_ISP_SET_PREVIEW_YIMG:
        isp_cmd = ISP_CTRL_SET_PREV_YIMG;
        isp_param = param_ptr->cmd_value;
        break;

    case COM_ISP_SET_PREVIEW_YUV:
        isp_cmd = ISP_CTRL_SET_PREV_YUV;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_GET_VCM_INFO:
        isp_cmd = ISP_CTRL_GET_AF_POS;
        // isp_param = param_ptr->cmd_value;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->vcm_step;
        break;
    case COM_ISP_GET_REBOKEH_DATA:
        isp_cmd = ISP_CTRL_GET_REBOKEH_DATA;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->relbokeh_info;
        break;
    case COM_ISP_SET_PREVIEW_PDAF_RAW:
        isp_cmd = ISP_CTRL_SET_PREV_PDAF_RAW;
        ptr_flag = 1;
        isp_param_ptr = param_ptr->cmd_ptr;
        break;

    case COM_ISP_SET_AWB_LOCK_UNLOCK:
        isp_cmd = ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK;
        isp_param = param_ptr->cmd_value;
        if (isp_param == ISP_AWB_UNLOCK)
            isp_param = ISP_AWB_UNLOCK;
        else if (isp_param == ISP_AWB_LOCK)
            isp_param = ISP_AWB_LOCK;
        break;

    case COM_ISP_GET_LEDS_CTRL:
        ptr_flag = 1;
        isp_cmd = ISP_CTRL_GET_LEDS_CTRL;
        isp_param_ptr = (void *)&param_ptr->leds_ctrl;
        break;

    case COM_ISP_SET_AE_MODE_CONTROL:
        isp_cmd = ISP_CTRL_SET_AE_MODE;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("ae_mode %d", param_ptr->cmd_value);
        break;

    case COM_ISP_SET_AF_POS:
        isp_cmd = ISP_CTRL_SET_AF_POS;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("af_pos %d", param_ptr->cmd_value);
        break;

    case COM_ISP_SET_SPRD_APP_MODE:
        isp_cmd = ISP_CTRL_SET_APP_MODE;
        isp_param = param_ptr->cmd_value;
        CMR_LOGD("set app mode id = %d", param_ptr->cmd_value);
        break;

    case COM_ISP_GET_MFSR_PARAM:
        isp_cmd = ISP_CTRL_GET_MFSR_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->isp_blk_param;
        break;

    case COM_ISP_SET_MFSR_LOG:
        isp_cmd = ISP_CTRL_SET_MFSR_LOG;
        ptr_flag = 1;
        isp_param_ptr = (void *)param_ptr->cmd_ptr;
        break;

    case COM_ISP_GET_CNR3_PARAM:
        isp_cmd = ISP_CTRL_GET_CNR3_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cnr3_param;
        break;

    case COM_ISP_GET_CNR2_PARAM:
        isp_cmd = ISP_CTRL_GET_CNR2_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cnr2_param;
        break;

    case COM_ISP_GET_HDR_PARAM:
        isp_cmd = ISP_CTRL_GET_HDR_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->hdr_param;
        break;

    case COM_ISP_GET_DRE_PARAM:
#ifdef CONFIG_CAMERA_DRE
        isp_cmd = ISP_CTRL_GET_DRE_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->dre_param;
#else
        isp_cmd = ISP_CTRL_MAX;
#endif
        break;

    case COM_ISP_GET_FB_PREV_PARAM:
        isp_cmd = ISP_CTRL_GET_FB_PREV_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->fb_param;
        break;

    case COM_ISP_GET_FB_CAP_PARAM:
        isp_cmd = ISP_CTRL_GET_FB_CAP_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->fb_param;
        break;

    case COM_ISP_GET_DRE_PRO_PARAM:
#ifdef CONFIG_CAMERA_DRE_PRO
        isp_cmd = ISP_CTRL_GET_DRE_PRO_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->dre_pro_param;
#else
        isp_cmd = ISP_CTRL_MAX;
#endif
        break;

    case COM_ISP_GET_YNRS_PARAM:
        isp_cmd = ISP_CTRL_GET_YNRS_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->ynr_param;
        break;

    case COM_ISP_SET_ZOOM_SCENE:
        isp_cmd = ISP_CTRL_SET_ZOOM_SCENE;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cmd_value;
        break;

    case COM_ISP_GET_CNR2CNR3_YNR_EN:
        isp_cmd = ISP_CTRL_GET_CNR2CNR3_YNR_EN;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cnr2cnr3_ynr_en;
        break;

    case COM_ISP_GET_CNR2_YNR_EN:
        isp_cmd = ISP_CTRL_GET_CNR2_YNR_EN;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cnr2_ynr_en;
        break;

    case COM_ISP_SET_AUTO_HDR:
        isp_cmd = ISP_CTRL_AUTO_HDR_MODE;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AUTO_FDR:
        CMR_LOGD("set auto fdr enabled %d", param_ptr->cmd_value);
        isp_cmd = ISP_CTRL_AUTO_FDR_MODE;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AUTO_3DNR:
        CMR_LOGV("set auto 3dnr %d", param_ptr->cmd_value);
        isp_cmd = ISP_CTRL_SET_3DNR_MODE;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AI_SCENE_START:
        isp_cmd = ISP_CTRL_AI_PROCESS_START;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AI_SCENE_STOP:
        isp_cmd = ISP_CTRL_AI_PROCESS_STOP;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AI_SCENE_IMAGE:
        isp_cmd = ISP_CTRL_AI_SET_IMG_PARAM;
        isp_param_ptr = (void *)&param_ptr->ai_img_param;
        ptr_flag = 1;
        break;
    case COM_ISP_GET_AI_SCENE_IMAGE_REQ_FLAG:
        isp_cmd = ISP_CTRL_AI_GET_IMG_FLAG;
        isp_param_ptr = (void *)&param_ptr->ai_img_status;
        ptr_flag = 1;
        break;
    case COM_ISP_SET_CAP_FLAG:
        CMR_LOGD("set cap flag %d", param_ptr->cmd_value);
        isp_cmd = ISP_CTRL_SET_CAP_FLAG;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AUTO_FLASH_CAP:
        isp_cmd = ISP_CTRL_SET_AUTO_FLASH_CAP;
        isp_param = param_ptr->cmd_value;
        break;
    case COM_ISP_GET_SW3DNR_PARAM:
        isp_cmd = ISP_CTRL_GET_SW3DNR_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->threednr_param;
        break;
    case COM_ISP_GET_MFNR_PARAM:
        isp_cmd = ISP_CTRL_GET_MFNR_PARAM;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->mfnr_param;
        break;
    case COM_ISP_GET_CALIBRATION_VCMINFO:
        isp_cmd = ISP_CTRL_GET_BOKEH_RANGE;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->vcm_range;
        break;
    case COM_ISP_GET_AE_FPS_RANGE:
        isp_cmd = ISP_CTRL_GET_AE_FPS_RANGE;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->ae_fps_range;
        break;
    case COM_ISP_SET_AI_SET_FD_ON_OFF:
        isp_cmd = ISP_CTRL_AI_SET_FD_STATUS;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AE_SET_FD_ON_OFF:
        isp_cmd = ISP_CTRL_AE_SET_FD_STATUS;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cmd_value;
        break;

#if defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6) ||                      \
    defined(CONFIG_ISP_2_7)
    case COM_ISP_SET_AUTO_TRACKING_ENABLE:
        CMR_LOGD("set auto tracking enable %d", param_ptr->cmd_value);
        isp_cmd = ISP_CTRL_SET_AF_OT_SWITH;
        ptr_flag = 1;
        isp_param_ptr = (void *)&param_ptr->cmd_value;
        break;
    case COM_ISP_SET_AUTO_TRACKING_INFO:
        isp_cmd = ISP_CTRL_SET_AF_OT_INFO;
        ptr_flag = 1;
        isp_param_ptr = (void *)&(param_ptr->af_ot_info);
        CMR_LOGD("set auto tracking info X %d Y %d status %d",
                 param_ptr->af_ot_info.objectX, param_ptr->af_ot_info.objectY,
                 param_ptr->af_ot_info.status);
        break;
#endif
    case COM_ISP_SET_CALIBRATION_VCMDISC:
        isp_cmd = ISP_CTRL_SET_VCM_DIST;
        ptr_flag = 1;
        isp_param_ptr = (void *)&(param_ptr->vcm_disc);
        break;
    case COM_ISP_SET_AE_TARGET_REGION:
        isp_cmd = ISP_CTRL_AE_SET_TARGET_REGION;
        ptr_flag = 1;
        isp_param_ptr = (void *)&(param_ptr->ae_target_region);
        break;
    case COM_ISP_SET_SENSOR_SIZE:
        isp_cmd = ISP_CTRL_SET_SENSOR_SIZE;
        ptr_flag = 1;
        isp_param_ptr = (void *)&(param_ptr->size_param);
        break;
    case COM_ISP_SET_FDR_LOG:
         isp_cmd = ISP_CTRL_SET_FDR_LOG;
         ptr_flag = 1;
         isp_param_ptr = param_ptr->cmd_ptr;
         break;

    default:
        CMR_LOGE("don't support cmd %ld", cmd_type);
        ret = CMR_CAMERA_NO_SUPPORT;
        break;
    }

    if (ptr_flag) {
        ret = isp_ioctl(isp_cxt->isp_handle, isp_cmd, isp_param_ptr);
        if (ret) {
            CMR_LOGE("failed isp ioctl %ld", ret);
        }
        CMR_LOGV("done %ld and direct return", ret);
        return ret;
    }

    if (set_isp_flag) {
        ret = isp_ioctl(isp_cxt->isp_handle, isp_cmd, (void *)&isp_param);
        if (ret) {
            CMR_LOGE("failed isp ioctl %ld", ret);
        } else {
            if (COM_ISP_SET_ISO == cmd_type) {
                if (0 == param_ptr->cmd_value) {
                    isp_capability(isp_cxt->isp_handle, ISP_CUR_ISO,
                                   (void *)&isp_param);
                    cxt->setting_cxt.is_auto_iso = 1;
                } else {
                    cxt->setting_cxt.is_auto_iso = 0;
                }
                isp_param = POWER2(isp_param - 1) * ONE_HUNDRED;
                CMR_LOGI("auto iso %d, exif iso %d",
                         cxt->setting_cxt.is_auto_iso, isp_param);
            }
        }
    }

    if (set_exif_flag) {
        CMR_LOGD("ERIC set exif");
        if (COM_ISP_SET_AWB_MODE == cmd_type) {
            cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                exif_cmd, param_ptr->cmd_value);
            cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                SENSOR_EXIF_CTRL_WHITEBALANCE,
                                param_ptr->cmd_value);
        } else {
            cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                                exif_cmd, isp_param);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

void camera_get_iso_value(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_u32 isp_param = 0;
    if (cxt->isp_cxt.is_work && (1 == cxt->setting_cxt.is_auto_iso)) {
        struct isp_context *isp_cxt = &cxt->isp_cxt;
        isp_capability(isp_cxt->isp_handle, ISP_CUR_ISO, (void *)&isp_param);
        CMR_LOGD("iso value is %d", isp_param);
        cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                            SENSOR_EXIF_CTRL_ISOSPEEDRATINGS, isp_param);
    }
}

cmr_int camera_get_fb_param(cmr_handle handle,
                            struct isp_fb_param_info *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)handle;
    struct common_isp_cmd_param isp_param;
    if (!handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = camera_isp_ioctl(handle, COM_ISP_GET_FB_PREV_PARAM, &isp_param);
    if (ret) {
        goto exit;
    }
    memcpy(param, &isp_param.fb_param, sizeof(struct isp_fb_param_info));

exit:
    CMR_LOGV("done ret = %d", ret);
    return ret;
}

void camera_set_exif_exposure_time(cmr_handle oem_handle) {

    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    struct exif_spec_pic_taking_cond_tag exif_pic_info;
    EXIF_RATIONAL_T exposure_time;
    cmr_sensor_get_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                        &exif_pic_info);
    exposure_time.numerator = exif_pic_info.ExposureTime.numerator;
    exposure_time.denominator = exif_pic_info.ExposureTime.denominator;

    setting_param.camera_id = cxt->camera_id;
    setting_param.cmd_type_value = (cmr_uint)&exposure_time;
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      SETTING_SET_EXIF_EXPOSURE_TIME, &setting_param);

    CMR_LOGD("exposure_time_numerator %d exposure_time_denominator %d",
             exposure_time.numerator, exposure_time.denominator);
}

cmr_int camera_get_ae_lum_value(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_int lum_val = 0;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    isp_capability(isp_cxt->isp_handle, ISP_CTRL_GET_AE_LUM, (void *)&lum_val);

    return lum_val;
}

cmr_int camera_get_setting_activity(cmr_handle oem_handle,
                                    cmr_uint *is_active) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !is_active) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    *is_active = cxt->setting_cxt.is_active;
    CMR_LOGV("%ld", (cmr_uint)*is_active);
exit:
    return ret;
}

bool prev_get_appmode(cmr_handle oem_handle, cmr_u32 camera_id) {
    struct camera_context *cxt = (struct camera_context *)(oem_handle);
    struct setting_cmd_parameter setting_param;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    bool is_autophoto = false;
    bool ret;
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get app mode %ld", ret);
    }
    CMR_LOGD("app_mode = %d", setting_param.cmd_type_value);
    if (setting_param.cmd_type_value == CAMERA_MODE_AUTO_PHOTO) {
        // auto 3dnr available
        is_autophoto = true;
        if (ret) {
            CMR_LOGE("failed to cap cfg %ld", ret);
        }
    }
    return is_autophoto;
}

cmr_int camera_get_preview_param(cmr_handle oem_handle,
                                 enum takepicture_mode mode,
                                 cmr_uint is_snapshot,
                                 struct preview_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt = &cxt->prev_cxt;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct jpeg_context *jpeg_cxt = &cxt->jpeg_cxt;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;
    struct setting_cmd_parameter setting_param;
    struct sensor_context *sn_cxt = &cxt->sn_cxt;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 is_cfg_snp = 0;
    cmr_u32 rotation = 0;
    cmr_u8 haf_enable = 0;
    cmr_u32 is_raw_capture = 0;
    struct sensor_4in1_info sn_4in1_info;
    SENSOR_VAL_T val;

    cmr_bzero(&sn_4in1_info, sizeof(struct sensor_4in1_info));

    cmr_bzero((void *)out_param_ptr, sizeof(*out_param_ptr));

    out_param_ptr->memory_setting.alloc_mem = camera_malloc;
    out_param_ptr->memory_setting.free_mem = camera_free;
    out_param_ptr->memory_setting.gpu_alloc_mem = camera_gpu_malloc;
    out_param_ptr->memory_setting.invalidate_cb = camera_invalidate_buf;
    out_param_ptr->memory_setting.flush_cb = camera_flush_buf;

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PREVIEW_SIZE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get prev size %ld", ret);
        goto exit;
    }
    prev_cxt->size = setting_param.size_param;
    out_param_ptr->preview_size = prev_cxt->size;
    if (out_param_ptr->preview_size.width > 0 &&
        out_param_ptr->preview_size.height > 0) {
        out_param_ptr->preview_eb = 1;
        CMR_LOGD("preview_eb=%d", out_param_ptr->preview_eb);
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PREVIEW_FORMAT, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview fmt %ld", ret);
        goto exit;
    }
    out_param_ptr->preview_fmt = setting_param.cmd_type_value;

    out_param_ptr->is_fd_on = cxt->fd_on_off;
    out_param_ptr->is_support_fd = cxt->is_support_fd;
    out_param_ptr->is_lls_enable = cxt->is_lls_enable;
    out_param_ptr->is_ultra_wide = cxt->is_ultra_wide;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PREVIEW_ANGLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview angle %ld", ret);
        goto exit;
    }
    out_param_ptr->prev_rot = setting_param.cmd_type_value;

    /*get raw capture size, this is only for choosing sensor mode*/
    if (is_raw_capture == 1) {
        ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                SETTING_GET_RAW_CAPTURE_SIZE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get prev size %ld", ret);
            goto exit;
        }
        out_param_ptr->raw_capture_size = setting_param.size_param;
        CMR_LOGD("raw_capture height = %d, width = %d",
                 out_param_ptr->raw_capture_size.height,
                 out_param_ptr->raw_capture_size.width);
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_VIDEO_SIZE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get video size %ld", ret);
        goto exit;
    }
    prev_cxt->video_size = setting_param.size_param;
    out_param_ptr->video_size = prev_cxt->video_size;
    if (out_param_ptr->video_size.width > 0 &&
        out_param_ptr->video_size.height > 0) {
        out_param_ptr->video_eb = 1;
        CMR_LOGD("video_eb=%d", out_param_ptr->video_eb);
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_VIDEO_FORMAT, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get video fmt %ld", ret);
        goto exit;
    }
    out_param_ptr->video_fmt = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_DV_MODE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get dv mode %ld", ret);
        goto exit;
    }
    out_param_ptr->is_dv = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SLOW_MOTION_FLAG, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get dv mode %ld", ret);
        goto exit;
    }
    out_param_ptr->video_slowmotion_eb = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_REFOCUS_ENABLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview refocus enabled flag %ld", ret);
        goto exit;
    }
    cxt->is_refocus_mode = setting_param.cmd_type_value;

/*for sharkle get auto 3dnr flag*/
#ifdef CONFIG_ISP_2_3
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_AUTO_3DNR,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get 3dnr type %ld", ret);
        goto exit;
    }
    camera_set_3dnr_flag(cxt, setting_param.cmd_type_value);
    out_param_ptr->is_auto_3dnr = setting_param.cmd_type_value;
    if (prev_get_appmode(oem_handle, cxt->camera_id)) {
        out_param_ptr->is_auto_3dnr = CAMERA_3DNR_AUTO;
    }
#endif

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_3DNR_TYPE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get 3dnr type %ld", ret);
        goto exit;
    }
    camera_set_3dnr_flag(cxt, setting_param.cmd_type_value);
    property_get("debug.camera.3dnr.preview", value, "true");
    if (!strcmp(value, "false")) {
        out_param_ptr->sprd_3dnr_type = CAMERA_3DNR_TYPE_NULL;
    } else {
        out_param_ptr->sprd_3dnr_type = camera_get_3dnr_flag(cxt);
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_AFBC_ENABLED, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get dv mode %ld", ret);
        goto exit;
    }
    out_param_ptr->sprd_afbc_enabled = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_ZOOM_PARAM,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get zoom param %ld", ret);
        goto exit;
    }

#ifdef CONFIG_CAMERA_4IN1
    val.type = SENSOR_VAL_TYPE_GET_4IN1_INFO;
    val.pval = &sn_4in1_info;
    ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                           SENSOR_ACCESS_VAL, (cmr_uint)&val);
    if (ret) {
        CMR_LOGE("get sensor 4ini1 failed %ld", ret);
        goto exit;
    }
    if (isp_video_get_simulation_flag())
        cxt->sn_cxt.info_4in1.is_4in1_supported = 0;
    else
        cxt->sn_cxt.info_4in1.is_4in1_supported =
            sn_4in1_info.is_4in1_supported;
    cxt->sn_cxt.info_4in1.limited_4in1_width = sn_4in1_info.limited_4in1_width;
    cxt->sn_cxt.info_4in1.limited_4in1_height =
        sn_4in1_info.limited_4in1_height;

    if (1 == cxt->sn_cxt.info_4in1.is_4in1_supported) {
        struct cmr_path_capability capability;
        CMR_LOGD(" 4in1 path capability");
        camera_channel_path_capability(oem_handle, &capability);
        if (1 != capability.support_4in1) {
            CMR_LOGE("isp hw not support_4in1");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }
    out_param_ptr->limited_4in1_width =
        cxt->sn_cxt.info_4in1.limited_4in1_width;
    out_param_ptr->limited_4in1_height =
        cxt->sn_cxt.info_4in1.limited_4in1_height;
    if (is_raw_capture == 1) {
        out_param_ptr->remosaic_type = camera_get_remosaic_type(
            &(cxt->sn_cxt.info_4in1), out_param_ptr->raw_capture_size.width,
            out_param_ptr->raw_capture_size.height);
    } else {
        /* TODO, if capture run to here, maybe need call
         * camera_get_remosaic_type */
        out_param_ptr->remosaic_type = 0;
    }
    if (cxt->remosaic_type == 1) {
        camera_open_4in1((cmr_handle)cxt);
        CMR_LOGD("prepare software remosaic");
    }
#endif

    out_param_ptr->zoom_setting = setting_param.zoom_param;
    CMR_LOGV("aspect ratio prev=%f, video=%f, cap=%f",
             out_param_ptr->zoom_setting.zoom_info.prev_aspect_ratio,
             out_param_ptr->zoom_setting.zoom_info.video_aspect_ratio,
             out_param_ptr->zoom_setting.zoom_info.capture_aspect_ratio);

    cxt->snp_cxt.snp_mode = mode;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_YUV_CALLBACK_SIZE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get capture size %ld", ret);
        goto exit;
    }

    //      channel1 will be used later
    //        out_param_ptr->channel1_size = setting_param.size_param;
    //        if (out_param_ptr->channel1_size.width > 0 &&
    //            out_param_ptr->channel1_size.height > 0) {
    //            out_param_ptr->channel1_eb = 1;
    //       }

    out_param_ptr->channel2_size = setting_param.size_param;
    if (out_param_ptr->channel2_size.width > 0 &&
        out_param_ptr->channel2_size.height > 0) {
        out_param_ptr->channel2_eb = 1;
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_YUV2_SIZE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get capture size %ld", ret);
        goto exit;
    }
    out_param_ptr->channel3_size = setting_param.size_param;
    if (out_param_ptr->channel3_size.width > 0 &&
        out_param_ptr->channel3_size.height > 0) {
        out_param_ptr->channel3_eb = 1;
    }

    //     channel4 will be used later
    //        out_param_ptr->channel4_size = setting_param.size_param;
    //        if (out_param_ptr->channel4_size.width > 0 &&
    //            out_param_ptr->channel4_size.height > 0) {
    //            out_param_ptr->channel4_eb = 1;
    //        }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_YUV_CALLBACK_FORMAT, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get cap fmt %ld", ret);
        goto exit;
    }

    //    channel1 will be used later
    //        out_param_ptr->channel1_fmt = setting_param.cmd_type_value;
    //        out_param_ptr->channel1_rot_angle = out_param_ptr->prev_rot;
    //        CMR_LOGD("channel1_eb=%d, fmt=%d, rot_angle=%d",
    //        out_param_ptr->channel1_eb, out_param_ptr->channel1_fmt,
    //        out_param_ptr->channel1_rot_angle);

    out_param_ptr->channel2_fmt = setting_param.cmd_type_value;
    out_param_ptr->channel2_rot_angle = out_param_ptr->prev_rot;
    CMR_LOGD("channel2_eb=%d, fmt=%d, rot_angle=%d", out_param_ptr->channel2_eb,
             out_param_ptr->channel2_fmt, out_param_ptr->channel2_rot_angle);

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_YUV2_FORMAT, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get cap fmt %ld", ret);
        goto exit;
    }
    out_param_ptr->channel3_fmt = setting_param.cmd_type_value;
    out_param_ptr->channel3_rot_angle = out_param_ptr->prev_rot;
    CMR_LOGD("channel3_eb=%d, fmt=%d, rot_angle=%d", out_param_ptr->channel3_eb,
             out_param_ptr->channel3_fmt, out_param_ptr->channel3_rot_angle);

    //    channel4 will be used later
    //        out_param_ptr->channel4_fmt = setting_param.cmd_type_value;
    //        out_param_ptr->channel4_rot_angle = out_param_ptr->prev_rot;
    //        CMR_LOGD("channel4_eb=%d, fmt=%d, rot_angle=%d",
    //        out_param_ptr->channel4_eb, out_param_ptr->channel4_fmt,
    //        out_param_ptr->channel4_rot_angle);

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_RAW_SIZE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get raw size %ld", ret);
        goto exit;
    }
    out_param_ptr->channel0_size = setting_param.size_param;
    if (out_param_ptr->channel0_size.width > 0 &&
        out_param_ptr->channel0_size.height > 0) {
        out_param_ptr->channel0_eb = 1;
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_RAW_FORMAT,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get cap fmt %ld", ret);
        goto exit;
    }
    out_param_ptr->channel0_fmt = setting_param.cmd_type_value;
    CMR_LOGV("channel0_eb=%d, fmt=%d", out_param_ptr->channel0_eb,
             out_param_ptr->channel0_fmt);

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_CAPTURE_SIZE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get capture size %ld", ret);
        goto exit;
    }
    out_param_ptr->picture_size = setting_param.size_param;
    cxt->snp_cxt.request_size = setting_param.size_param;
    if (out_param_ptr->picture_size.width > 0 &&
        out_param_ptr->picture_size.height > 0 &&
        out_param_ptr->video_slowmotion_eb == 0) {
        out_param_ptr->snapshot_eb = 1;
        CMR_LOGD("snapshot_eb=%d", out_param_ptr->snapshot_eb);
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_CAPTURE_FORMAT, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get cap fmt %ld", ret);
        goto exit;
    }
    out_param_ptr->cap_fmt = setting_param.cmd_type_value;

    /*get snapshot angle*/
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_CAPTURE_ANGLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get capture rot %ld", ret);
        goto exit;
    }
    out_param_ptr->cap_rot = setting_param.cmd_type_value;

    /*get rotation snapshot cfg*/
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_ROTATION_CAPTURE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get capture rot %ld", ret);
        goto exit;
    }
    out_param_ptr->is_cfg_rot_cap = setting_param.cmd_type_value;
    cxt->snp_cxt.is_cfg_rot_cap = out_param_ptr->is_cfg_rot_cap;

    out_param_ptr->isp_width_limit = cxt->isp_cxt.width_limit;
    out_param_ptr->tool_eb = 0;
    if (CAMERA_ISP_TUNING_MODE == mode || CAMERA_UTEST_MODE == mode ||
        CAMERA_AUTOTEST_MODE == mode || CAMERA_ISP_SIMULATION_MODE == mode) {
        out_param_ptr->tool_eb = 1;
    }

    /*get hdr flag*/
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_HDR,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get envir %ld", ret);
        goto exit;
    }
    camera_set_hdr_flag(cxt, setting_param.cmd_type_value);
    out_param_ptr->is_hdr = setting_param.cmd_type_value;

     /*get fdr flag*/
     cmr_bzero(&setting_param, sizeof(setting_param));
     setting_param.camera_id = cxt->camera_id;
     ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_FDR,
                             &setting_param);
     if (ret) {
         CMR_LOGE("failed to get envir %ld", ret);
         goto exit;
     }
     camera_set_fdr_flag(cxt, setting_param.cmd_type_value);
     CMR_LOGD("get fdr flag %d", camera_get_fdr_flag(cxt));
     out_param_ptr->is_fdr = camera_get_fdr_flag(cxt);

    /*get android zsl flag*/
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_ANDROID_ZSL_FLAG, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get android zsl %ld", ret);
        goto exit;
    }
    cxt->is_android_zsl = setting_param.cmd_type_value;

    /*get jpeg param*/
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_JPEG_QUALITY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get image quality %ld", ret);
        goto exit;
    }
    jpeg_cxt->param.quality = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_THUMB_QUALITY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get thumb quality %ld", ret);
        goto exit;
    }
    jpeg_cxt->param.thumb_quality = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_THUMB_SIZE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get thumb size %ld", ret);
        goto exit;
    }
    jpeg_cxt->param.thum_size = setting_param.size_param;
    out_param_ptr->thumb_size = jpeg_cxt->param.thum_size;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_ENCODE_ANGLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get enc angle %ld", ret);
        goto exit;
    }
    out_param_ptr->encode_angle = setting_param.cmd_type_value;
    jpeg_cxt->param.set_encode_rotation = setting_param.cmd_type_value;
    if (cxt->snp_cxt.is_cfg_rot_cap) {
        cxt->snp_cxt.cfg_cap_rot = jpeg_cxt->param.set_encode_rotation;
    } else {
        cxt->snp_cxt.cfg_cap_rot = 0;
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SHOT_NUMBER, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get shot num %ld", ret);
        goto exit;
    }
    out_param_ptr->frame_count = setting_param.cmd_type_value;
    cxt->snp_cxt.total_num = setting_param.cmd_type_value;

    CMR_LOGD("out_param_ptr->frame_count =%d", out_param_ptr->frame_count);

/* from sharkl2, jpeg support mirror/flip, so mirror feature use jpeg, not use
 * isp */
#ifdef MIRROR_FLIP_BY_ISP
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_ENCODE_ROTATION, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get enc rotation %ld", ret);
        goto exit;
    }
    rotation = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_FLIP_ON, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview sprd flip_on enabled flag %ld", ret);
        goto exit;
    }
    out_param_ptr->flip_on = setting_param.cmd_type_value;
    if (out_param_ptr->flip_on) {
        CMR_LOGI("encode_rotation:%d, flip:%d", rotation,
                 out_param_ptr->flip_on);
        if (90 == rotation || 270 == rotation) {
            out_param_ptr->flip_on = 0x1; // flip
        } else if (0 == rotation || 180 == rotation) {
            out_param_ptr->flip_on = 0x3; // mirror
        }
    }
#endif

    out_param_ptr->sprd_zsl_enabled = cxt->zsl_enabled;
    CMR_LOGD("sprd zsl_enabled flag %d", out_param_ptr->sprd_zsl_enabled);

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_VIDEO_SNAPSHOT_TYPE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview sprd eis enabled flag %ld", ret);
        goto exit;
    }
    out_param_ptr->video_snapshot_type = setting_param.cmd_type_value;
    CMR_LOGI("video_snapshot_type=%d", out_param_ptr->video_snapshot_type);

    if (out_param_ptr->sprd_zsl_enabled) {
        out_param_ptr->frame_count = FRAME_NUM_MAX;
        out_param_ptr->frame_ctrl = FRAME_CONTINUE;
        if (camera_get_3dnr_flag(cxt) == 5 || camera_get_3dnr_flag(cxt) == 8) {
            out_param_ptr->frame_count = cxt->ipm_cxt.threednr_num;
            out_param_ptr->frame_ctrl = FRAME_3DNR_PROC;
            CMR_LOGI("set FRAME_3DNR_PROC for 3dnr");
        }
    } else {
        if (camera_get_hdr_flag(cxt)) {
            out_param_ptr->frame_count = cxt->ipm_cxt.hdr_num;
            out_param_ptr->frame_ctrl = FRAME_HDR_PROC;
        } else if (out_param_ptr->video_snapshot_type == 1) {
            out_param_ptr->frame_ctrl = FRAME_CONTINUE;
        }
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_PIPVIV_ENABLED, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview sprd pipviv enabled flag %ld", ret);
        goto exit;
    }
    out_param_ptr->sprd_pipviv_enabled = setting_param.cmd_type_value;
    cxt->is_pipviv_mode = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_EIS_ENABLED, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview sprd eis enabled flag %ld", ret);
        goto exit;
    }
    out_param_ptr->sprd_eis_enabled = setting_param.cmd_type_value;
    CMR_LOGV("sprd eis_enabled flag %d", out_param_ptr->sprd_eis_enabled);

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_3DCAL_ENABLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview sprd 3dcal enabled flag %ld", ret);
        goto exit;
    }
    cxt->is_3dcalibration_mode = setting_param.cmd_type_value;
    out_param_ptr->sprd_3dcalibration_enabled = cxt->is_3dcalibration_mode;
    CMR_LOGV("sprd_3dcalibration_enabled flag %d",
             out_param_ptr->sprd_3dcalibration_enabled);

exit:
    CMR_LOGD(
        "prev size %d %d, pic size %d %d, video size %d %d, android zsl flag "
        "%d, prev rot %ld snp rot %d rot snp %d, zoom mode %ld fd %ld is dv %d "
        "tool eb %d, q %d thumb q %d enc angle %d thumb size %d %d, frame cnt "
        "%d, out_param_ptr->flip_on %d, sprd_3dnr_type %d, 4in1 support %d,"
        "limited_4in1_width %d, limited_4in1_height %d",
        out_param_ptr->preview_size.width, out_param_ptr->preview_size.height,
        out_param_ptr->picture_size.width, out_param_ptr->picture_size.height,
        out_param_ptr->video_size.width, out_param_ptr->video_size.height,
        cxt->is_android_zsl, out_param_ptr->prev_rot, out_param_ptr->cap_rot,
        out_param_ptr->is_cfg_rot_cap, out_param_ptr->zoom_setting.mode,
        out_param_ptr->is_fd_on, out_param_ptr->is_dv, out_param_ptr->tool_eb,
        jpeg_cxt->param.quality, jpeg_cxt->param.thumb_quality,
        jpeg_cxt->param.set_encode_rotation, jpeg_cxt->param.thum_size.width,
        jpeg_cxt->param.thum_size.height, out_param_ptr->frame_count,
        out_param_ptr->flip_on, out_param_ptr->sprd_3dnr_type,
        cxt->sn_cxt.info_4in1.is_4in1_supported,
        cxt->sn_cxt.info_4in1.limited_4in1_width,
        cxt->sn_cxt.info_4in1.limited_4in1_height);

    ATRACE_END();
    return ret;
}

cmr_int camera_set_preview_param(cmr_handle oem_handle,
                                 enum takepicture_mode mode,
                                 cmr_uint is_snapshot) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt = &cxt->prev_cxt;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct preview_param start_param;
    struct preview_out_param preview_out;

    if (1 != prev_cxt->inited) {
        CMR_LOGE("err, don't init preview");
        ret = -CMR_CAMERA_INVALID_STATE;
        goto exit;
    }
    if (PREVIEWING ==
        cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id)) {
        CMR_LOGI("prev has been started");
        goto exit;
    }

    cmr_bzero(&start_param, sizeof(struct preview_param));
    ret = camera_get_preview_param(oem_handle, mode, is_snapshot, &start_param);
    if (ret) {
        CMR_LOGE("failed to get prev param %ld", ret);
        goto exit;
    }
    prev_cxt->preview_eb = start_param.preview_eb;
    prev_cxt->video_eb = start_param.video_eb;
    prev_cxt->channel0_eb = start_param.channel0_eb;
    prev_cxt->channel1_eb = start_param.channel1_eb;
    prev_cxt->channel2_eb = start_param.channel2_eb;
    prev_cxt->channel3_eb = start_param.channel3_eb;
    prev_cxt->channel4_eb = start_param.channel4_eb;
    prev_cxt->snapshot_eb = start_param.snapshot_eb;

    cmr_bzero(&preview_out, sizeof(struct preview_out_param));
    ret = cmr_preview_set_param(prev_cxt->preview_handle, cxt->camera_id,
                                &start_param, &preview_out);
    if (ret) {
        CMR_LOGE("failed to set prev param %ld", ret);
        goto exit;
    }
    prev_cxt->preview_sn_mode = preview_out.preview_sn_mode;
    prev_cxt->preview_channel_id = preview_out.preview_chn_id;
    prev_cxt->data_endian = preview_out.preview_data_endian;
    prev_cxt->video_sn_mode = preview_out.video_sn_mode;
    prev_cxt->video_channel_id = preview_out.video_chn_id;
    prev_cxt->actual_video_size = preview_out.actual_video_size;
    prev_cxt->video_data_endian = preview_out.video_data_endian;

    prev_cxt->channel0_chn_id = preview_out.channel0_chn_id;
    prev_cxt->channel0_sn_mode = preview_out.channel0_sn_mode;
    prev_cxt->channel0_endian = preview_out.channel0_endian;

    prev_cxt->channel1_chn_id = preview_out.channel1_chn_id;
    prev_cxt->channel1_sn_mode = preview_out.channel1_sn_mode;
    prev_cxt->channel1_endian = preview_out.channel1_endian;

    prev_cxt->channel2_chn_id = preview_out.channel2_chn_id;
    prev_cxt->channel2_sn_mode = preview_out.channel2_sn_mode;
    prev_cxt->channel2_endian = preview_out.channel2_endian;

    prev_cxt->channel3_chn_id = preview_out.channel3_chn_id;
    prev_cxt->channel3_sn_mode = preview_out.channel3_sn_mode;
    prev_cxt->channel3_endian = preview_out.channel3_endian;

    prev_cxt->channel4_chn_id = preview_out.channel4_chn_id;
    prev_cxt->channel4_sn_mode = preview_out.channel4_sn_mode;
    prev_cxt->channel4_endian = preview_out.channel4_endian;

    snp_cxt->snapshot_sn_mode = preview_out.snapshot_sn_mode;
    snp_cxt->channel_id = preview_out.snapshot_chn_id;
    snp_cxt->post_proc_setting = preview_out.post_proc_setting;
    snp_cxt->data_endian = preview_out.snapshot_data_endian;
    snp_cxt->zsl_frame = preview_out.zsl_frame;
    snp_cxt->actual_capture_size = preview_out.actual_snapshot_size;
    cmr_copy((void *)&snp_cxt->post_proc_setting,
             (void *)&preview_out.post_proc_setting,
             sizeof(snp_cxt->post_proc_setting));
    CMR_LOGI("prev mode %d chn id %d snp mode %d snp_chn_id %d",
             prev_cxt->preview_sn_mode, prev_cxt->preview_channel_id,
             snp_cxt->snapshot_sn_mode, snp_cxt->channel_id);
    CMR_LOGI("rot angle %ld", snp_cxt->post_proc_setting.rot_angle);

exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_get_snapshot_param(cmr_handle oem_handle,
                                  struct snapshot_param *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct jpeg_context *jpeg_cxt = &cxt->jpeg_cxt;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    struct common_isp_cmd_param isp_cmd_parm;
    cmr_u32 gtm_on = 0, app_mode = CAMERA_MODE_AUTO_PHOTO;
    cmr_u32 is_need_flash = 0;

    out_ptr->snap_cnt = 0;
    out_ptr->total_num = 0;
    out_ptr->rot_angle = 0;
    out_ptr->hdr_handle = cxt->ipm_cxt.hdr_handle;
    out_ptr->hdr_need_frm_num = cxt->ipm_cxt.hdr_num;
    out_ptr->threednr_handle = cxt->ipm_cxt.threednr_handle;
    out_ptr->threednr_need_frm_num = cxt->ipm_cxt.threednr_num;
    out_ptr->post_proc_setting.data_endian = cxt->snp_cxt.data_endian;
    out_ptr->lls_shot_mode = cxt->lls_shot_mode;
    out_ptr->is_vendor_hdr = cxt->is_vendor_hdr;
    out_ptr->is_pipviv_mode = cxt->is_pipviv_mode;
    out_ptr->is_3dcalibration_mode = cxt->is_3dcalibration_mode;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                            &setting_param);
    if (ret)
        CMR_LOGE("failed to get app mode %ld", ret);
    else
        app_mode = (cmr_u32)setting_param.cmd_type_value;
    CMR_LOGD("app_mode = %d", app_mode);

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            CAMERA_PARAM_SPRD_SUPER_MACROPHOTO_PARAM, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get macrophoto flag %ld", ret);
        goto exit;
    }
    out_ptr->is_super = setting_param.cmd_type_value;
    cxt->snp_cxt.is_super = out_ptr->is_super;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_3DCAL_ENABLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get 3dcal flag %ld", ret);
        goto exit;
    }
    cxt->is_3dcalibration_mode = setting_param.cmd_type_value;
    out_ptr->is_3dcalibration_mode = setting_param.cmd_type_value;
    ret =
        cmr_setting_ioctl(setting_cxt->setting_handle,
                          SETTING_GET_SPRD_YUV_CALLBACK_ENABLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get yuv callback flag %ld", ret);
        goto exit;
    }
    cxt->is_yuv_callback_mode = setting_param.cmd_type_value;
    out_ptr->is_yuv_callback_mode = setting_param.cmd_type_value;

    //get whether is_need_flash, flash has priority over HDR/FDR/3DNR
    camera_local_snapshot_is_need_flash(oem_handle, cxt->camera_id,
                                        &is_need_flash);
    if (!is_need_flash) {
        ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_HDR,
                                &setting_param);
        if (ret) {
            CMR_LOGE("failed to get hdr %ld", ret);
        } else {
            camera_set_hdr_flag(cxt, (cmr_u32)setting_param.cmd_type_value);
            out_ptr->is_hdr = camera_get_hdr_flag(cxt);
        }
    } else {
        camera_set_hdr_flag(cxt, 0);
        out_ptr->is_hdr = 0;
    }

    if (!is_need_flash && out_ptr->is_hdr == 0) {
        ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_FDR,
                            &setting_param);
        if (ret) {
            CMR_LOGE("failed to get FDR %ld", ret);
        } else {
            camera_set_fdr_flag(cxt, setting_param.cmd_type_value);
            out_ptr->is_fdr = camera_get_fdr_flag(cxt);
        }
    } else {
        camera_set_fdr_flag(cxt, 0);
        out_ptr->is_fdr = 0;
    }

	out_ptr->is_mfsr = 0;
	cxt->snp_cxt.is_mfsr = 0;
	if ((cxt->mfsr_force_off == 0) && !is_need_flash &&
		(out_ptr->is_fdr == 0) && (out_ptr->is_hdr == 0)) {
		struct cmr_zoom zoom_factor;
		float zoom_ratio;

		cmr_preview_get_zoom_factor(cxt->prev_cxt.preview_handle,
				cxt->camera_id, &zoom_factor);
		out_ptr->zoom_param = zoom_factor.zoom_setting;
		zoom_ratio = zoom_factor.zoom_setting.zoom_info.capture_aspect_ratio;
		cxt->snp_cxt.zoom_param = out_ptr->zoom_param;

		if ((zoom_ratio > s_mfsr_ratiol) && (zoom_ratio < s_mfsr_ratioh)) {
			out_ptr->is_mfsr = 1;
			cxt->snp_cxt.is_mfsr = 1;
		}
		CMR_LOGD("set ratio range (%f ~ %f), cur_ratio %f, is_mfsr %d",
				s_mfsr_ratiol, s_mfsr_ratioh, zoom_ratio, out_ptr->is_mfsr);
	}

    if (!is_need_flash && out_ptr->is_fdr == 0 && out_ptr->is_hdr == 0 && !out_ptr->is_mfsr) {
        ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_3DNR_TYPE,
                            &setting_param);
        if (ret) {
            CMR_LOGE("failed to get 3dnr type %ld", ret);
        } else {
            camera_set_3dnr_flag(cxt, setting_param.cmd_type_value);
            out_ptr->is_3dnr = camera_get_3dnr_flag(cxt);
        }
    } else {
        camera_set_3dnr_flag(cxt, 0);
        out_ptr->is_3dnr = 0;
    }

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SHOT_NUMBER, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get shot num %ld", ret);
        goto exit;
    }
    out_ptr->total_num = setting_param.cmd_type_value;
    cxt->snp_cxt.total_num = out_ptr->total_num;

    /*get jpeg param*/
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_JPEG_QUALITY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get image quality %ld", ret);
        goto exit;
    }
    jpeg_cxt->param.quality = setting_param.cmd_type_value;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_THUMB_QUALITY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get thumb quality %ld", ret);
        goto exit;
    }
    jpeg_cxt->param.thumb_quality = setting_param.cmd_type_value;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_THUMB_SIZE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get thumb size %ld", ret);
        goto exit;
    }
    jpeg_cxt->param.thum_size = setting_param.size_param;
    out_ptr->post_proc_setting.thumb_size = jpeg_cxt->param.thum_size;

    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_TOUCH_XY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to open flash");
    }
    camera_set_touch_xy(cxt, setting_param.touch_param);
    /*get filter*/
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_FILTER_TEYP, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get filtertype %ld", ret);
        goto exit;
    }
    camera_set_filter_flag(cxt, setting_param.cmd_type_value);
    out_ptr->filter_type = (cmr_u32)setting_param.cmd_type_value;

    out_ptr->ee_flag = camera_get_ee_flag(oem_handle);
    cxt->ee_flag = out_ptr->ee_flag;

    cxt->gtm_flag = 0;
#ifdef CONFIG_ISP_2_7
    ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_GTM_STATUS, &gtm_on);
    if (ret) {
        CMR_LOGE("failed isp ioctl %ld", ret);
    }
    cxt->gtm_flag = !!gtm_on;
    CMR_LOGD("gtm on %d\n", cxt->gtm_flag);
#endif

    out_ptr->dre_flag = 0;
    cxt->dre_flag = 0;
#ifdef CONFIG_CAMERA_DRE
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_DRE_PARAM, &isp_cmd_parm);
    if (ret) {
        CMR_LOGE("there is no dre parameters for camera id =%d",
                 cxt->camera_id);
    } else if ((app_mode == CAMERA_MODE_AUTO_PHOTO ||
         app_mode == CAMERA_MODE_BACK_ULTRA_WIDE ||
         app_mode == CAMERA_MODE_3DNR_PHOTO || app_mode == CAMERA_MODE_HIGH_RES_PHOTO) &&
         !cxt->snp_cxt.is_mfsr && !cxt->snp_cxt.is_fdr && !cxt->snp_cxt.is_hdr){
        // dre available
        out_ptr->dre_flag = 1;
        cxt->dre_flag = 1;
        cxt->predre_flag = isp_cmd_parm.dre_param.predre_param.enable;
    }
#endif

#ifdef CONFIG_CAMERA_DRE_PRO
    if (app_mode == CAMERA_MODE_NIGHT_PHOTO ||
        ((app_mode == CAMERA_MODE_AUTO_PHOTO || app_mode == CAMERA_MODE_HIGH_RES_PHOTO)
            && !cxt->snp_cxt.is_hdr && !cxt->snp_cxt.is_mfsr && !cxt->snp_cxt.is_fdr)) {
        out_ptr->dre_flag = 1;
        cxt->dre_flag = 1;
    }
#endif

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_ENCODE_ANGLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get enc angle %ld", ret);
        goto exit;
    }
    out_ptr->rot_angle = setting_param.cmd_type_value;
    jpeg_cxt->param.set_encode_rotation = setting_param.cmd_type_value;

    ret = cmr_preview_get_post_proc_param(
        cxt->prev_cxt.preview_handle, cxt->camera_id,
        (cmr_u32)setting_param.cmd_type_value, &out_ptr->post_proc_setting);
    if (ret) {
        CMR_LOGE("failed to get rot angle %ld", ret);
        goto exit;
    }
    if (0 == out_ptr->post_proc_setting.chn_out_frm[0].fd) {
        ret = CMR_CAMERA_FAIL;
        CMR_LOGE("failed to get cap mem(fd is NULL) %ld", ret);
        goto exit;
    }
    out_ptr->camera_id = cxt->camera_id;
    out_ptr->is_android_zsl = cxt->is_android_zsl;
    out_ptr->mode = cxt->snp_cxt.snp_mode;
    out_ptr->is_cfg_rot_cap = cxt->snp_cxt.is_cfg_rot_cap;
    out_ptr->jpeg_setting = jpeg_cxt->param;
    out_ptr->req_size = cxt->snp_cxt.request_size;
    out_ptr->channel_id = cxt->snp_cxt.channel_id;

    if (is_need_flash || out_ptr->is_hdr || out_ptr->is_fdr ||
            out_ptr->is_3dnr || out_ptr->is_mfsr)
        cxt->gtm_flag = 0;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_VIDEO_SNAPSHOT_TYPE, &setting_param);

    if (ret) {
        CMR_LOGE("failed to get preview sprd eis enabled flag %ld", ret);
        goto exit;
    }
    if (setting_param.cmd_type_value == VIDEO_SNAPSHOT_VIDEO) {
        out_ptr->channel_id = cxt->prev_cxt.video_channel_id;
        out_ptr->is_video_snapshot = 1;
    } else if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode) {
        out_ptr->is_zsl_snapshot = 1;
    }
    CMR_LOGD("actual size %d %d",
             out_ptr->post_proc_setting.actual_snp_size.width,
             out_ptr->post_proc_setting.actual_snp_size.height);

exit:
    CMR_LOGD("done,total num %d enc angle %d", out_ptr->total_num,
             out_ptr->rot_angle);
    return ret;
}

cmr_int camera_set_setting(cmr_handle oem_handle, enum camera_param_type id,
                           cmr_u64 param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;

    setting_param.camera_id = cxt->camera_id;
    switch (id) {
    case CAMERA_PARAM_ZOOM:
        if (param) {
            setting_param.zoom_param = *((struct cmr_zoom_param *)param);
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, zoom param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_REPROCESS_ZOOM_RATIO:
        if (param) {
            setting_param.zoom_param = *((struct cmr_zoom_param *)param);
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, zoom ratio param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_ENCODE_ROTATION:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_CONTRAST:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_BRIGHTNESS:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_AI_SCENE_ENABLED:
        cxt->ai_scene_enable = param;
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SHARPNESS:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_WB:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_EFFECT:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_FLASH:
    case CAMERA_PARAM_ISP_FLASH:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_ANTIBANDING:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_AUTO_EXPOSURE_MODE:
        setting_param.ae_param = *(struct cmr_ae_param *)param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_AE_REGION:
        setting_param.ae_param = *(struct cmr_ae_param *)param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_ISO:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_EXPOSURE_COMPENSATION:
        setting_param.ae_compensation_param =
            *(struct cmr_ae_compensation_param *)param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_PREVIEW_FPS:
        if (param) {
            setting_param.preview_fps_param =
                *(struct cmr_preview_fps_param *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, fps param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;

    case CAMERA_PARAM_PREVIEW_LLS_FPS:
        if (param) {
            /*to do */
            // use the cmr_preview_fps_param temporaily, to be replaced later
            setting_param.preview_fps_param =
                *(struct cmr_preview_fps_param *)param;
            setting_param.cmd_type_value =
                setting_param.preview_fps_param.frame_rate;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, fps param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;

    case CAMERA_PARAM_AE_MODE:
        setting_param.cmd_type_value = param;
        CMR_LOGI("ae_mode=%lu", param);
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;

    case CAMERA_PARAM_EXPOSURE_TIME:
        setting_param.cmd_type_value = param;
        CMR_LOGI("param=%"PRIu64"", param);
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;

    case CAMERA_PARAM_EXIF_EXPOSURE_TIME:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                SETTING_SET_EXIF_EXPOSURE_TIME, &setting_param);
        break;


    case CAMERA_PARAM_SPRD_3DNR_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;

    case CAMERA_PARAM_SPRD_3DNR_TYPE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;

    case CAMERA_PARAM_SPRD_SET_APPMODE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;

    case CAMERA_PARAM_SPRD_ENABLE_CNR:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;

    case CAMERA_PARAM_SATURATION:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SCENE_MODE:
        setting_param.cmd_type_value = param;
        CMR_LOGD("fdr scene mode:%d, cam_id:%d, ref_cam_id:%d, is_multi_camera_mode_oem:%d",
                 param, cxt->camera_id, cxt->ref_camera_id, is_multi_camera_mode_oem);
        if(CAMERA_SCENE_MODE_FDR == param) {
            if(is_multi_camera_mode_oem == MODE_SINGLE_CAMERA ||
                (is_multi_camera_mode_oem == MODE_MULTI_CAMERA && cxt->camera_id == cxt->ref_camera_id)) {
                camera_get_fdr_tuning_param(cxt, NULL);
                camera_set_fdr_flag(cxt, 1);
                cmr_preview_realloc_buffer_for_fdr(cxt->prev_cxt.preview_handle,
					cxt->camera_id, (cxt->swa_cxt_fdr.version == FDR_VERSION_1) ? 1 : 0);
            } else {
                camera_set_fdr_flag(cxt, 0);
            }
        } else {
            camera_set_fdr_flag(cxt, 0);
        }
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
   case CAMERA_PARAM_SPRD_ENABLE_POSTEE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_JPEG_QUALITY:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_THUMB_QUALITY:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SENSOR_ORIENTATION:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_FOCAL_LENGTH:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SENSOR_ROTATION:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_PERFECT_SKIN_LEVEL:
        setting_param.fb_param = *(struct beauty_info *)param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_FLIP_ON:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SHOT_NUM:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_ROTATION_CAPTURE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_POSITION:
        if (param) {
            setting_param.position_info = *(struct camera_position_type *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, postition param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_RAW_CAPTURE_SIZE:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, raw capture size param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_PREVIEW_SIZE:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, prev size param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_PREVIEW_FORMAT:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_CAPTURE_SIZE:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, capture size param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_CAPTURE_FORMAT:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_CAPTURE_MODE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_THUMB_SIZE:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, thumb size param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_VIDEO_SIZE:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, video size param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_VIDEO_FORMAT:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_YUV_CALLBACK_SIZE:
    case CAMERA_PARAM_YUV2_SIZE:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_RAW_SIZE:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, callback size param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_YUV_CALLBACK_FORMAT:
    case CAMERA_PARAM_YUV2_FORMAT:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_RAW_FORMAT:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_RANGE_FPS:
        if (param) {
            setting_param.range_fps = *(struct cmr_range_fps_param *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, range fps param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_SPRD_ZSL_ENABLED:
        cxt->zsl_enabled = param;
        CMR_LOGD("zsl %d", cxt->zsl_enabled);
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SLOW_MOTION_FLAG:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_ISP_AE_LOCK_UNLOCK:
        CMR_LOGD("CAMERA_PARAM_ISP_AE_LOCK_UNLOCK");
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_PIPVIV_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_EIS_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_REFOCUS_ENABLE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_TOUCH_XY:
        if (param) {
            setting_param.size_param = *(struct img_size *)param;
            ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                    &setting_param);
        } else {
            CMR_LOGE("err, video size param is null");
            ret = -CMR_CAMERA_INVALID_PARAM;
        }
        break;
    case CAMERA_PARAM_VIDEO_SNAPSHOT_TYPE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_3DCAL_ENABLE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_EXIF_MIME_TYPE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_FILTER_TYPE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_LENS_FOCUS_DISTANCE:
        setting_param.cmd_type_value = param;
        CMR_LOGI("focus_distance=%lu", param);
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_AUTO_HDR_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_AUTO_FDR_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SET_DEVICE_ORIENTATION:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_AUTO_TRACKING_INFO:
        ret = cmr_preview_set_autotracking_param(
            cxt->prev_cxt.preview_handle, cxt->camera_id,
            (struct auto_tracking_info *)param);
        break;
    case CAMERA_PARAM_AF_STATUS_NOTIFY_TRACKING:
        ret = cmr_preview_af_status_set_to_autotracking(
            cxt->prev_cxt.preview_handle, cxt->camera_id, param);
        break;
    case CAMERA_PARAM_SPRD_AUTO_3DNR_ENABLED:
        cxt->auto3dnr_flag = param;
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_AFBC_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_LOGO_WATERMARK_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_TIME_WATERMARK_ENABLED:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_AUTOCHASING_REGION_ENABLE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_FACE_ATTRIBUTES_ENABLE:
        setting_param.cmd_type_value = param;
        CMR_LOGI(" face attributes enable =%lu", param);
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    case CAMERA_PARAM_SPRD_SUPER_MACROPHOTO_ENABLE:
        setting_param.cmd_type_value = param;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
        break;
    case CAMERA_PARAM_SMILE_CAPTURE_ENABLE:
        setting_param.cmd_type_value = param;
        CMR_LOGI(" smile capture =%lu", param);
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id,
                                &setting_param);
        break;
    default:
        CMR_LOGI("don't support %d", id);
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_restart_rot(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_rotation_deinit(oem_handle);
    if (ret) {
        CMR_LOGE("failed to de-init rotate %ld", ret);
    } else {
        ret = camera_rotation_init(oem_handle);
        if (ret) {
            CMR_LOGE("failed to initizalize rot");
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

/**************************external function*****************************/

cmr_int camera_local_int(cmr_u32 camera_id, camera_cb_of_type callback,
                         void *client_data, cmr_uint is_autotest,
                         cmr_handle *oem_handle, void *cb_of_malloc,
                         void *cb_of_free) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    struct phySensorInfo *phyPtr = NULL;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    *oem_handle = (cmr_handle)0;
    cxt = (struct camera_context *)malloc(sizeof(struct camera_context));
    if (NULL == cxt) {
        CMR_LOGE("failed to create context");
        ret = -CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_bzero(cxt, sizeof(*cxt));
    cxt->camera_id = camera_id;
    cxt->master_id = master_id_oem;
    cxt->camera_cb = callback;
    cxt->client_data = client_data;
    cxt->hal_malloc = cb_of_malloc;
    cxt->hal_free = cb_of_free;
    cxt->is_multi_mode = is_multi_camera_mode_oem;
    cxt->blur_facebeauty_flag = 0;
    phyPtr = sensorGetPhysicalSnsInfo(cxt->camera_id);
    cxt->facing = phyPtr->face_type;

    CMR_LOGI("cxt=%p, client_data=%p, master_id=%d", cxt, cxt->client_data, cxt->master_id);
    ret = camera_init_internal((cmr_handle)cxt, is_autotest);
    if (ret) {
        CMR_LOGE("camera_init_internal failed");
        goto exit;
    }

exit:
    if (CMR_CAMERA_SUCCESS == ret) {
        cxt->inited = 1;
        *oem_handle = (cmr_handle)cxt;
        active_camera_num++;
        CMR_LOGD("active_camera_num = %d", active_camera_num);
    } else {
        if (cxt) {
            free((void *)cxt);
            cxt = NULL;
        }
    }
    return ret;
}

cmr_int camera_local_set_gpu_mem_ops(cmr_handle oem_handle, void *cb_of_malloc,
                                     void *cb_of_free) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->hal_gpu_malloc = cb_of_malloc;
    return ret;
}

cmr_int camera_local_deinit(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    camera_deinit_internal(oem_handle);
    free((void *)oem_handle);
    oem_handle = NULL;
    ATRACE_END();
    return ret;
}

static void show_snap_param(struct snapshot_param *p)
{

	struct snp_proc_param *pp = &p->post_proc_setting;
	CMR_LOGI("cam %d, filter %d mode %d hdr %d fdr %d, 3dnr %d, vid %d zsl %d tot_num %d\n",
		    p->camera_id,////
		    p->filter_type,
		    p->mode,
		    p->is_hdr, p->is_fdr,
		    p->is_3dnr, ////
		    p->is_video_snapshot,
		    p->is_zsl_snapshot,
		    p->total_num);

	CMR_LOGI("rot ang %d an_zsl %d,   rot_cap %d ch_id %d  sn_mod %d hdrn %d 3dnr_n %d\n",
		    p->rot_angle,
		    p->is_android_zsl,
		    p->is_cfg_rot_cap,
		    p->channel_id,
		    p->sn_mode,
		    p->hdr_need_frm_num,
		    p->threednr_need_frm_num);
	CMR_LOGI("%d %d %d %d yuv_cb %d, sup %d  nr %d ee %d dre %d\n",
		    p->lls_shot_mode,
		    p->is_vendor_hdr,
		    p->is_pipviv_mode,
		    p->is_3dcalibration_mode,
		    p->is_yuv_callback_mode, ////
		    p->is_super,
		    p->nr_flag,
		    p->ee_flag,
		    p->dre_flag);

	CMR_LOGI("req size %d %d, mfsr %d, zoom info %d %d.  %f %f %f %f\n",
		p->req_size.width, p->req_size.height, p->is_mfsr,
		p->zoom_param.mode, p->zoom_param.zoom_level,
		p->zoom_param.zoom_info.zoom_ratio,
		p->zoom_param.zoom_info.prev_aspect_ratio,
		p->zoom_param.zoom_info.video_aspect_ratio,
		p->zoom_param.zoom_info.capture_aspect_ratio);

	CMR_LOGI("pix %d %d,  crop %d %d %d %d, jpegq %d %d, rot %d, padding %d,  thm size %d %d\n",
		p->zoom_param.zoom_info.pixel_size.width, p->zoom_param.zoom_info.pixel_size.height,
		p->zoom_param.zoom_info.crop_region.start_x, p->zoom_param.zoom_info.crop_region.start_y,
		p->zoom_param.zoom_info.crop_region.width, p->zoom_param.zoom_info.crop_region.height,
		p->jpeg_setting.quality,
		p->jpeg_setting.thumb_quality,
		p->jpeg_setting.set_encode_rotation,
		p->jpeg_setting.padding,
		p->jpeg_setting.thum_size.width,
		p->jpeg_setting.thum_size.height);

	CMR_LOGI("setting %ld %ld, scale %d, de align actual (%d %d), cap_org (%d %d)\n",
		pp->rot_angle, pp->channel_zoom_mode, pp->is_need_scaling,
		pp->dealign_actual_snp_size.width,
		pp->dealign_actual_snp_size.height,
		pp->cap_org_size.width, pp->cap_org_size.height);

	CMR_LOGI("actual(%d %d), snp (%d %d), max(%d %d), thm(%d %d), rect (%d %d %d %d)\n",
		pp->actual_snp_size.width,
		pp->actual_snp_size.height,
		pp->snp_size.width, pp->snp_size.height,
		pp->max_size.width, pp->max_size.height,
		pp->thumb_size.width, pp->thumb_size.height,
		pp->scaler_src_rect[0].start_x,
		pp->scaler_src_rect[0].start_y,
		pp->scaler_src_rect[0].width,
		pp->scaler_src_rect[0].height);

	CMR_LOGI("out 0x%x, 0x%08x.  cap raw 0x%x, 0x%08x, cap yuv 0x%x, 0x%08x\n",
		pp->chn_out_frm[0].fd,
		pp->chn_out_frm[0].addr_vir.addr_y,
		pp->mem[0].cap_raw.fd,
		pp->mem[0].cap_raw.addr_vir.addr_y,
		pp->mem[0].cap_yuv.fd,
		pp->mem[0].cap_yuv.addr_vir.addr_y);

	CMR_LOGI("target 0x%x, 0x%08x, jpeg 0x%x, 0x%08x, tmyuv 0x%x, 0x%08x, thmjpeg 0x%x, 0x%08x\n",
		pp->mem[0].target_yuv.fd,
		pp->mem[0].target_yuv.addr_vir.addr_y,
		pp->mem[0].target_jpeg.fd,
		pp->mem[0].target_jpeg.addr_vir.addr_y,
		pp->mem[0].thum_yuv.fd,
		pp->mem[0].thum_yuv.addr_vir.addr_y,
		pp->mem[0].thum_jpeg.fd,
		pp->mem[0].thum_jpeg.addr_vir.addr_y);

	return;
}



cmr_int camera_local_start_preview(cmr_handle oem_handle,
                                   enum takepicture_mode mode,
                                   cmr_uint is_snapshot) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt = &cxt->prev_cxt;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    struct common_isp_cmd_param isp_param;
    struct snapshot_param snp_param;
    cmr_u32 app_mode = CAMERA_MODE_AUTO_PHOTO;
    char value[PROPERTY_VALUE_MAX] = {0};


    cxt->camera_mode = mode;
    cxt->snp_cxt.start_capture_flag = 0;
    cxt->ambient_highlight = 0; /* default 0 when start preview */
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get app mode %ld", ret);
    } else {
        app_mode = (cmr_u32) setting_param.cmd_type_value;
    }
    CMR_LOGI("cam%d app_mode = %d, takepic mode %d, multi mode %d\n",
        cxt->camera_id, app_mode, mode, cxt->is_multi_mode);

    cxt->app_mode = app_mode;
    cxt->nightscepro_flag = 0;
    if (app_mode != CAMERA_MODE_3DNR_PHOTO) {
        cxt->nightscepro_flag = 1;
    }
    ret = camera_set_preview_param(oem_handle, mode, is_snapshot);
    if (ret) {
        CMR_LOGE("failed to set prev param %ld", ret);
        goto exit;
    }

    if (property_get_bool("persist.vendor.cam.ai.scence.enable", 0)) {
        if ((app_mode == CAMERA_MODE_AUTO_PHOTO ||
            app_mode == CAMERA_MODE_3DNR_PHOTO) &&
            (setting_param.camera_id == 0 ||
             cxt->is_multi_mode == MODE_MULTI_CAMERA ||
             cxt->is_multi_mode == MODE_FOV_FUSION)) {
            if (cxt->ipm_cxt.ai_scene_inited == 0) {
                struct ipm_open_in in_param;
                struct ipm_open_out out_param;
                cmr_bzero(&in_param, sizeof(struct ipm_open_in));
                cmr_bzero(&out_param, sizeof(struct ipm_open_out));
                camera_open_ai_scene(oem_handle, &in_param, &out_param);
                cxt->ipm_cxt.ai_scene_inited = 1;
            }
        }
    }

    cxt->setting_cxt.is_active = 1;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_SET_ENVIRONMENT, &setting_param);

    ret = cmr_preview_start(prev_cxt->preview_handle, cxt->camera_id);
    if (ret) {
        CMR_LOGE("failed to start prev %ld", ret);
    }
#ifdef CONFIG_ISP_2_7
    if (app_mode == CAMERA_MODE_AUTO_PHOTO ||
        app_mode == CAMERA_MODE_BACK_ULTRA_WIDE) {
        isp_param.cmd_value = 1;
    } else {
        isp_param.cmd_value = 0;
    }
    ret =
        camera_isp_ioctl(oem_handle, COM_ISP_SET_GTM_ONFF, (void *)&isp_param);
#endif

    ret = cmr_sensor_focus_init(cxt->sn_cxt.sensor_handle, cxt->camera_id);

#ifdef CONFIG_FACE_BEAUTY
    if (cxt->video_face_beauty_en) {
        cxt->start_video_face_beauty = true;
    }
#endif

    cmr_bzero(&snp_param, sizeof(struct snapshot_param));
    camera_get_snapshot_param(oem_handle, &snp_param);
    show_snap_param(&snp_param);
    if (snp_param.is_fdr)
        cmr_preview_realloc_buffer_for_fdr(cxt->prev_cxt.preview_handle,
                cxt->camera_id, (cxt->swa_cxt_fdr.version == FDR_VERSION_1) ? 1 : 0);

    /* init debug ctrl */
    cxt->mfsr_force_off = 1;
#ifdef CONFIG_CAMERA_MFSR_CAPTURE
    if (app_mode == CAMERA_MODE_AUTO_PHOTO && (cxt->is_multi_mode == MODE_SINGLE_CAMERA)) {
        cxt->mfsr_force_off = 0;
        ret = camera_isp_ioctl(cxt, COM_ISP_GET_MFSR_PARAM, &isp_param);
        if (ret || isp_param.isp_blk_param.param_ptr == NULL) {
            CMR_LOGE("fail to get mfsr param\n");
            cxt->mfsr_force_off = 1;
        }
        property_get("debug.camera.mfsr.force_off", value, "2");
        if (atoi(value) < 2)
            cxt->mfsr_force_off = atoi(value);
        property_get("debug.camera.mfsr.ratio.low", value, "0");
        if (atoi(value) != 0)
            s_mfsr_ratiol = (atoi(value)) / 10.0 - 0.00000001;
        property_get("debug.camera.mfsr.ratio.high", value, "0");
        if (atoi(value) != 0)
            s_mfsr_ratioh = (atoi(value)) / 10.0 + 0.00000001;
        CMR_LOGD("mfsr_force_off %d, ratio (%f ~ %f)\n", cxt->mfsr_force_off, s_mfsr_ratiol, s_mfsr_ratioh);
    }
#endif

    cmr_snapshot_prepare(cxt->snp_cxt.snapshot_handle, &snp_param);
exit:
    ATRACE_END();
    return ret;
}

cmr_int cmr_preview_waitencode(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int time = 0;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    if (cxt->jpg_encode == JPEG_ENCODING) {
        CMR_LOGI("wait for jpeg encode");
        while (1) {
            if (time > JPG_ENCODE_WAIT_TIMEOUT) {
                CMR_LOGE("wait jpg encode timeout");
                ret = -1;
                goto exit;
            }
            time++;
            if (cxt->jpg_encode == JPEG_ENCODE_DONE ||
                !cxt->jpeg_cxt.jpeg_handle ||
                cxt->jpg_encode == JPEG_ENCODE_STOP) {
                CMR_LOGI("jpeg encode ok wait up jpg_encode %d", cxt->jpg_encode);
                break;
            }
            usleep(1000);
        }
   }
exit:
    ATRACE_END();
    return ret;
}

cmr_int camera_local_stop_preview(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_int prev_ret = CMR_CAMERA_SUCCESS;
    cmr_int snp_ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param param;
    struct setting_cmd_parameter setting_param;

    CMR_LOGD("E");

    cmr_preview_wait_recovery(cxt->prev_cxt.preview_handle, cxt->camera_id);

    if (PREVIEWING !=
        cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id)) {
        CMR_LOGI("don't previewing");
        goto exit;
    }

    if (CAMERA_ZSL_MODE == cxt->camera_mode) {
        if (IDLE != cxt->snp_cxt.status) {
            snp_ret = cmr_snapshot_stop(cxt->snp_cxt.snapshot_handle);
            if (snp_ret) {
                CMR_LOGE("failed to stop snp %ld", ret);
            }
        }
    }

    camera_get_iso_value(oem_handle);
    if (cxt->jpg_encode == JPEG_ENCODING)
        cmr_preview_waitencode(oem_handle);
    prev_ret = cmr_preview_stop(cxt->prev_cxt.preview_handle, cxt->camera_id);
    if (prev_ret) {
        CMR_LOGE("failed to stop prev %ld", ret);
    }
    ret = prev_ret | snp_ret;
    cxt->setting_cxt.is_active = 0;
    cxt->prev_cxt.video_size.width = 0;
    cxt->prev_cxt.video_size.height = 0;
    cxt->prev_cxt.preview_eb = 0;
    cxt->prev_cxt.preview_channel_id = 0;
    cxt->prev_cxt.preview_sn_mode = 0;
    cxt->prev_cxt.video_eb = 0;
    cxt->prev_cxt.video_channel_id = 0;
    cxt->prev_cxt.video_sn_mode = 0;
    cxt->prev_cxt.channel1_eb = 0;
    cxt->prev_cxt.channel1_chn_id = 0;
    cxt->prev_cxt.channel1_sn_mode = 0;
    cxt->prev_cxt.channel2_eb = 0;
    cxt->prev_cxt.channel2_chn_id = 0;
    cxt->prev_cxt.channel2_sn_mode = 0;
    cxt->prev_cxt.channel3_eb = 0;
    cxt->prev_cxt.channel3_chn_id = 0;
    cxt->prev_cxt.channel3_sn_mode = 0;
    cxt->prev_cxt.channel4_eb = 0;
    cxt->prev_cxt.channel4_chn_id = 0;
    cxt->prev_cxt.channel4_sn_mode = 0;
    cxt->prev_cxt.snapshot_eb = 0;

    cxt->snp_cxt.start_capture_flag = 0;

    cxt->is_start_snapshot = 0;

#ifdef CONFIG_FACE_BEAUTY
    if (cxt->video_face_beauty_en) {
       if (cxt->mvideofb) {
          cxt->mvideofb = false;
	   cxt->start_video_face_beauty = false;
          ret = face_beauty_ctrl(&cxt->video_face_beauty, FB_BEAUTY_FAST_STOP_CMD,NULL);
          face_beauty_deinit(&cxt->video_face_beauty);
      }
       if (cxt->mflagfb) {
          cxt->mflagfb = false;
          ret = face_beauty_ctrl(&cxt->prev_face_beauty, FB_BEAUTY_FAST_STOP_CMD,NULL);
          face_beauty_deinit(&cxt->prev_face_beauty);
      }
    }
#endif
    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      CAMERA_PARAM_PREVIEW_SIZE, &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle, CAMERA_PARAM_VIDEO_SIZE,
                      &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      CAMERA_PARAM_YUV_CALLBACK_SIZE, &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle, CAMERA_PARAM_YUV2_SIZE,
                      &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle, CAMERA_PARAM_RAW_SIZE,
                      &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      CAMERA_PARAM_CAPTURE_SIZE, &setting_param);

    if (cxt->ipm_cxt.ai_scene_inited) {
        ret = camera_close_ai_scene(cxt);
        if (ret) {
            CMR_LOGE("failed to close ai scene");
        }
        cxt->ipm_cxt.ai_scene_inited = 0;
    }

    if (cxt->night_cxt.is_authorized) {
        cxt->night_flag = false;
        cxt->night_cxt.ipmpro_deinit(oem_handle);
    }

    if (cxt->ipm_cxt.warp_handle != NULL) {
        sem_wait(&cxt->eisWarp_sm);
        ret = cmr_ipm_close(cxt->ipm_cxt.warp_handle);
        cxt->ipm_cxt.warp_handle = 0;
        sem_post(&cxt->eisWarp_sm);
    }

exit:
    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int camera_local_highflash_ae_measure(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;

    if (CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
        setting_param.camera_id = cxt->camera_id;
        ret =
            cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                              SETTING_SET_HIGHFLASH_AE_MEASURE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to open flash");
        }
    }

    return ret;
}

/**add for 3d capture to reset reprocessing capture size begin*/
cmr_int camera_local_set_cap_size(cmr_handle oem_handle,
                                  cmr_u32 is_reprocessing, cmr_u32 camera_id,
                                  cmr_u32 width, cmr_u32 height) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prv_cxt = &cxt->prev_cxt;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;

    if (cxt->multicam_highres_mode) {
        width = width * 2;
        height = height * 2;
    }

    cmr_preview_set_cap_size(prv_cxt->preview_handle, is_reprocessing,
                             camera_id, width, height);
    snp_cxt->request_size.width = width;
    snp_cxt->request_size.height = height;

    return 0;
}
/**add for 3d capture to reset reprocessing capture size end*/

void camera_adjust_ev_before_snapshot(cmr_handle oem_handle,
                                      enum camera_snapshot_tpye type) {
    cmr_int ret;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    // clear to 0 for cb(transfer parameter back)
    cxt->ae_aux_info.cnt = 0;
    ret = camera_snapshot_set_ev(oem_handle, 1, type);
    if (ret) {
        CMR_LOGE("fail to set dre ev");
    }
    cxt->capture_timestamp = systemTime(SYSTEM_TIME_BOOTTIME);
    if (cxt->skipframe) {
        cxt->skip_frame_enable = 1;
        if (type == SNAPSHOT_GTM)
            cxt->skip_frame_cnt = 2;
        else if (type == SNAPSHOT_DRE)
            cxt->skip_frame_cnt = 1;
        else if (type == SNAPSHOT_NIGHT_DNS)
            cxt->skip_frame_cnt = 8;
    } else {
        cxt->skip_frame_enable = 0;
        camera_snapshot_set_ev(oem_handle, 0, type);
    }
    CMR_LOGI("skip_frame_enable =%d,type =%d", cxt->skip_frame_enable, type);
}

cmr_int camera_local_start_snapshot(cmr_handle oem_handle,
                                    enum takepicture_mode mode,
                                    cmr_uint is_snapshot) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt;
    struct snapshot_param snp_param;
    struct common_sn_cmd_param param;
    struct setting_cmd_parameter setting_param;
    struct img_rect prev_rect;
    cmr_u32 is_need_flash = 0;
    cmr_uint flash_hw_status = 0;
    cmr_s32 sm_val = 0;
    cmr_u32 need_3dnr = 0;
    cmr_u32 lock_af = 0;
    cmr_u32 lock_awb = 0;
    cmr_u32 is_raw_capture = 0;
    cmr_u32 is_isptool_flag = 0;
    char raw_value[PROPERTY_VALUE_MAX];
    cmr_uint video_snapshot_type;
    cmr_uint has_preflashed;
    struct common_isp_cmd_param isp_param;
    CMR_LOGI("E.\n");

    sem_wait(&cxt->snapshot_sm);

    ret = camera_jpeg_init_wait(oem_handle);
    if (ret) {
        CMR_LOGD("jpeg_async_init_handle already release!");
        goto exit;
    }

    camera_take_snapshot_step(CMR_STEP_TAKE_PIC);
    prev_cxt = &cxt->prev_cxt;

    sem_getvalue(&cxt->share_path_sm, &sm_val);
    if (0 != sm_val) {
        sem_destroy(&cxt->share_path_sm);
        sem_init(&cxt->share_path_sm, 0, 0);
        CMR_LOGI("re-initialize share_path_sm");
    }
    if (CAMERA_ZSL_MODE != mode) {
        ret = camera_set_preview_param(oem_handle, mode, is_snapshot);
        if (ret) {
            CMR_LOGE("failed to set preview param %ld", ret);
            goto exit;
        }
        if (cxt->remosaic_type == 1)
            camera_open_4in1(oem_handle);
    } else {
        camera_get_iso_value(oem_handle);
    }

    cmr_bzero(&snp_param, sizeof(struct snapshot_param));
    ret = camera_get_snapshot_param(oem_handle, &snp_param);
    if (ret) {
        CMR_LOGE("failed to get snp num %ld", ret);
        goto exit;
    }
    if ((snp_param.is_3dnr == CAMERA_3DNR_TYPE_PREV_NULL_CAP_HW) ||
        (snp_param.is_3dnr == CAMERA_3DNR_TYPE_PREV_HW_CAP_HW) ||
        (snp_param.is_3dnr == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW)) {
        need_3dnr = 1;
    }
    ret = cmr_grab_3dnr_cfg(cxt->grab_cxt.grab_handle, snp_param.channel_id,
                            need_3dnr);
    if (ret) {
        CMR_LOGE("failed to config 3dnr  %ld", ret);
    }

    cmr_bzero(&prev_rect, sizeof(struct img_rect));
    ret = cmr_grab_longexp_cfg(cxt->grab_cxt.grab_handle, cxt->long_expo_enable);
    if (ret) {
        CMR_LOGE("failed to config long_expo_enable  %ld", ret);
    }

    ret = cmr_preview_get_prev_rect(cxt->prev_cxt.preview_handle,
                                    cxt->camera_id, &prev_rect);
    cxt->prev_cxt.rect = prev_rect;
    CMR_LOGD("cam%d, zoom_ratio = %f, prev_rect %d %d %d %d",
            cxt->camera_id, cxt->zoom_ratio,
            prev_rect.start_x, prev_rect.start_y, prev_rect.width, prev_rect.height);

    ret = camera_ipm_open_sw_algorithm((cmr_handle)cxt);
    if (ret) {
        CMR_LOGE("failed to open ipm module %ld", ret);
        goto exit;
    }

    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_HW_FLASH_STATUS, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get flash mode %ld", ret);
    } else {
        flash_hw_status = setting_param.cmd_type_value;
    }
    CMR_LOGD("HW flash_status=%ld", flash_hw_status);
    if (flash_hw_status != FLASH_TORCH) {
        cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                            SENSOR_EXIF_CTRL_FLASH, 0);
    }

    cxt->snp_cxt.snap_cnt = 0;
    if (cxt->snp_cxt.is_mfsr) {
        cmr_u32 isp_scene = ISP_ZOOM;
        snp_param.total_num = 5;
        cxt->snp_cxt.total_num = 5;
        ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SCENE_MODE, &isp_scene);
        if (ret)
            CMR_LOGE("fail to isp set zoom scene\n");
    }
	if (cxt->snp_cxt.is_mfsr) {
		struct img_rect crop_x, crop_1;
		float zoom_ratio, aspect_ratio;
		cxt->snp_cxt.zoom_param_1x = cxt->snp_cxt.zoom_param;
		cxt->snp_cxt.zoom_param_1x.zoom_info.capture_aspect_ratio = 1.0;
		crop_1.start_x = 0;
		crop_1.start_y = 0;
		crop_1.width = cxt->sn_cxt.cur_sn_size.width;
		crop_1.height = cxt->sn_cxt.cur_sn_size.height;
		crop_x = crop_1;

		aspect_ratio = 1.0 * snp_param.req_size.width / snp_param.req_size.height;
		zoom_ratio = 1.0;
		ret = camera_get_trim_rect2(&crop_1, zoom_ratio, aspect_ratio, crop_1.width, crop_1.height, 0);
		if (ret)
			CMR_LOGE("fail to get trim rect2\n");

		zoom_ratio = cxt->snp_cxt.zoom_param.zoom_info.capture_aspect_ratio;
		ret = camera_get_trim_rect2(&crop_x, zoom_ratio, aspect_ratio, crop_x.width, crop_x.height, 0);
		if (ret)
			CMR_LOGE("fail to get trim rect2\n");

		cxt->snp_cxt.zoom_param.zoom_info.crop_region = crop_x;
		cxt->snp_cxt.zoom_param_1x.zoom_info.crop_region = crop_1;
		CMR_LOGD("crop for 1.0 (%d %d %d %d), for %f (%d %d %d %d)\n",
			crop_1.start_x, crop_1.start_y, crop_1.width, crop_1.height, zoom_ratio,
			crop_x.start_x, crop_x.start_y, crop_x.width, crop_x.height);

		ret = cmr_grab_cfg_ch_crop(cxt->grab_cxt.grab_handle, cxt->snp_cxt.channel_id, &crop_1);
		if (ret)
			CMR_LOGE("fail to cmr_grab_cfg_ch_crop\n");
	}

    show_snap_param(&snp_param);

    ret = cmr_snapshot_post_proc(cxt->snp_cxt.snapshot_handle, &snp_param);
    if (ret) {
        CMR_LOGE("failed to snp post proc %ld", ret);
        goto exit;
    }
    cxt->is_start_snapshot = 1;
#ifdef CONFIG_FACE_BEAUTY
    memset(&(cxt->face_beauty), 0, sizeof(struct fb_beauty_param));
#endif
    cxt->snp_cxt.actual_capture_size =
        snp_param.post_proc_setting.chn_out_frm[0].size;
    if (CAMERA_ISP_SIMULATION_MODE == mode) {
        cxt->camera_mode = mode;
        // clean up the buffer had set to kernel
        cmr_preview_stop(prev_cxt->preview_handle, cxt->camera_id);
    } else if (CAMERA_ZSL_MODE != mode) {
        ret = cmr_preview_start(prev_cxt->preview_handle, cxt->camera_id);
        if (ret) {
            CMR_LOGE("failed to start prev %ld", ret);
        }
        cxt->camera_mode = mode;
    }

    camera_local_snapshot_is_need_flash(oem_handle, cxt->camera_id,
                                        &is_need_flash);
    property_get("persist.vendor.cam.raw.mode", raw_value, "jpeg");
    if (!strcmp(raw_value, "raw")) {
        is_raw_capture = 1;
    }

    property_get("persist.vendor.cam.isptool.mode.enable", raw_value, "false");
    if ((!strcmp(raw_value, "true")) ||
        (CAMERA_ISP_SIMULATION_MODE == mode)) {
        is_isptool_flag = 1;
    }
    CMR_LOGD("is_raw_capture %d, is_isptool_flag %d", is_raw_capture, is_isptool_flag);
    //GTM set ev
    if(camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_NULL && cxt->gtm_flag &&
        0 == camera_get_hdr_flag(cxt) && !is_need_flash && 1 != camera_get_fdr_flag(cxt))
    {
        camera_adjust_ev_before_snapshot(oem_handle,SNAPSHOT_GTM);
    } else {
        CMR_LOGD("GTM No need to lower exposure ,3dnr flag =%d GTM =%d",
                 camera_get_3dnr_flag(cxt), cxt->gtm_flag);
    }

    if (1 == cxt->dre_flag &&!is_need_flash && cxt->predre_flag && !cxt->gtm_flag &&
          camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_NULL && 1 != camera_get_fdr_flag(cxt)) {
        camera_adjust_ev_before_snapshot(oem_handle,SNAPSHOT_DRE);
    } else {
        CMR_LOGD("DRE No need to lower exposure,dre_flag=%d,predre=%d,flash "
                 "=%d,gtm =%d",
                 cxt->dre_flag, cxt->predre_flag, is_need_flash, cxt->gtm_flag);
    }

    if (1 == camera_get_hdr_flag(cxt)) {
        ret = camera_hdr_set_ev(oem_handle);
        if (ret) {
            CMR_LOGE("fail to set hdr ev");
        }
        cxt->hdr_capture_timestamp = systemTime(SYSTEM_TIME_BOOTTIME);
        cxt->hdr_skip_frame_enable = 1;
        cxt->hdr_skip_frame_cnt = 0;
    } else if (1 == camera_get_fdr_flag(cxt)) {
        //configure fdr buffer
        cmr_preview_cfg_fdr_buffer(cxt->prev_cxt.preview_handle,
                                    cxt->camera_id);
        //lock 3A
        ret = camera_fdr_ctrl_3A(oem_handle, 1);
        if(ret) {
            CMR_LOGE("fdr failed to lock 3A");
        }

        //lock AE
        ret = camera_fdr_set_ev(oem_handle, 1);
        if (ret) {
            CMR_LOGE("fail to set fdr ev");
        }
        cxt->fdr_capture_timestamp = systemTime(SYSTEM_TIME_BOOTTIME);
        cxt->fdr_skip_frame_enable = 1;
        cxt->fdr_skip_frame_cnt = 0;
        cxt->fdr_capture_frame_cnt = 0;

    } else if (CAMERA_3DNR_TYPE_PREV_HW_CAP_SW == camera_get_3dnr_flag(cxt) ||
               CAMERA_3DNR_TYPE_PREV_SW_CAP_SW == camera_get_3dnr_flag(cxt) ||
                CAMERA_3DNR_TYPE_PREV_NULL_CAP_SW == camera_get_3dnr_flag(cxt)) {
        sem_init(&cxt->threednr_proc_sm, 0, 0);
        if (!(is_raw_capture || is_isptool_flag)) {
            ret = camera_3dnr_set_ev(oem_handle, 1);
            if (ret)
                CMR_LOGE("fail to set 3dnr ev");
        }
    } else if (CAMERA_3DNR_TYPE_NIGHT_DNS == camera_get_3dnr_flag(cxt)) {
        sem_init(&cxt->threednr_proc_sm, 0, 0);
        if (!(is_raw_capture || is_isptool_flag)) { // not (raw capture, isptool)
            camera_adjust_ev_before_snapshot(oem_handle, SNAPSHOT_NIGHT_DNS);
        }
    } else {
        // whether FRONT_CAMERA_FLASH_TYPE is lcd
        bool isFrontLcd =
            (strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") == 0) ? true : false;
        // whether FRONT_CAMERA_FLASH_TYPE is flash
        bool isFrontFlash =
            (strcmp(FRONT_CAMERA_FLASH_TYPE, "flash") == 0) ? true : false;
        if (cxt->is_multi_mode == MODE_MULTI_CAMERA || cxt->camera_id == 0 ||
            isFrontLcd || isFrontFlash || cxt->camera_id == 4) {
            ret = camera_capture_highflash(oem_handle, cxt->camera_id);
            if (ret)
                CMR_LOGE("open high flash fail");
        }
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_PRE_LOWFLASH_VALUE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preflashed flag %ld", ret);
    }
    has_preflashed = setting_param.cmd_type_value;

    if (has_preflashed)
        camera_get_iso_value(oem_handle);
    if (cxt->flash_mode == 3){
        if (setting_param.cmd_type_value == 1){
            isp_param.cmd_value = 1;
            ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_AUTO_FLASH_CAP, (void *)&isp_param);
        }
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_VIDEO_SNAPSHOT_TYPE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get VIDEO_SNAPSHOT_TYPE %ld", ret);
        goto exit;
    }

    video_snapshot_type = setting_param.cmd_type_value;
    if (video_snapshot_type != VIDEO_SNAPSHOT_VIDEO &&
        cxt->snp_cxt.start_capture_flag == 0) {
        ret = camera_local_start_capture(oem_handle);
        if (ret) {
            CMR_LOGE("camera_start_capture failed");
            goto exit;
        }
        cxt->snp_cxt.start_capture_flag = 1;
    }

    camera_set_snp_req((cmr_handle)cxt, TAKE_PICTURE_NEEDED);
    camera_snapshot_started((cmr_handle)cxt);
    //camera_set_exif_exposure_time(oem_handle);
    ret = camera_get_cap_time((cmr_handle)cxt);
    cxt->snp_cxt.status = SNAPSHOTING;
    cxt->snp_cxt.post_proc_setting.actual_snp_size =
        snp_param.post_proc_setting.actual_snp_size;
    cmr_copy(cxt->snp_cxt.post_proc_setting.chn_out_frm,
             snp_param.post_proc_setting.chn_out_frm,
             sizeof(cxt->snp_cxt.post_proc_setting.chn_out_frm));
    cxt->snp_cxt.post_proc_setting.actual_snp_size =
        snp_param.post_proc_setting.actual_snp_size;
    cxt->snp_cxt.post_proc_setting.dealign_actual_snp_size =
        snp_param.post_proc_setting.dealign_actual_snp_size; /**add for 3D Capture, modify reprocess
                                         request exif size*/
    cxt->snp_cxt.post_proc_setting.snp_size =
        snp_param.post_proc_setting.snp_size; /**add for 3D Capture, modify
                                                 reprocess request exif size*/

    camera_set_discard_frame(cxt, 0);

    if (CAMERA_ISP_SIMULATION_MODE == mode) {
        ret = cmr_isp_simulation_proc(cxt, &snp_param);
        if (ret) {
            CMR_LOGE("camera isp simulation mode failed");
            goto exit;
        }
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    sem_post(&cxt->snapshot_sm);
    ATRACE_END();
    return ret;
}

cmr_int camera_local_stop_snapshot(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    memset(&setting_param, 0, sizeof(setting_param));

    sem_wait(&cxt->snapshot_sm);
    if (cxt->remosaic_type == 1)
        camera_close_4in1(oem_handle);
    if (camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
        camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
        camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_PREV_NULL_CAP_SW) {
#ifdef OEM_HANDLE_3DNR
        if (0 != cxt->ipm_cxt.frm_num) {
            cxt->ipm_cxt.frm_num = 0;
        }
#endif
        setting_param.camera_id = cxt->camera_id;
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_APPMODE,
                            &setting_param);

        if((setting_param.cmd_type_value == CAMERA_MODE_NIGHT_PHOTO)
                 && cxt->night_cxt.is_authorized && cxt->night_cxt.mfnr_on_off) {
                cxt->night_cxt.mfnr_on_off = 0;
                ret = cxt->night_cxt.sw_close(cxt);
        } else {
             ret = camera_close_3dnr(cxt);
             if (ret)
                CMR_LOGE("failed to close 3dnr");
        }
    }else if (camera_get_3dnr_flag(cxt) == CAMERA_3DNR_TYPE_NIGHT_DNS) {
         ret = camera_close_night_dns(cxt);
         if (ret)
            CMR_LOGE("failed to close night_dns");
    }

    if (cxt->ipm_cxt.cnr_inited) {
        ret = camera_close_cnr(cxt);
        if (ret) {
            CMR_LOGE("failed to close cnr");
        }
        cxt->ipm_cxt.cnr_inited = 0;
    }

#ifdef CONFIG_CAMERA_DRE_PRO
    if (cxt->ipm_cxt.dre_inited && cxt->dre_flag) {
        ret = camera_close_dre(cxt);
        if (ret) {
            CMR_LOGE("failed to close dre");
        }
        cxt->ipm_cxt.dre_inited = 0;
    }
#endif

    ret = cmr_snapshot_stop(cxt->snp_cxt.snapshot_handle);
    if (ret) {
        CMR_LOGE("failed to stop snp %ld", ret);
        goto exit;
    }

    if (TAKE_PICTURE_NEEDED == camera_get_snp_req(oem_handle)) {
        camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
        camera_channel_stop(oem_handle, 1 << cxt->snp_cxt.channel_id);
    } else {
        camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
    }

    ret = cmr_preview_cancel_snapshot(cxt->prev_cxt.preview_handle,
                                      cxt->camera_id);
    if (ret) {
        CMR_LOGE("failed to cancel %ld", ret);
        goto exit;
    }

    if (camera_get_hdr_flag(cxt)) {
#ifdef OEM_HANDLE_HDR
        if (0 != cxt->ipm_cxt.frm_num) {
            cxt->ipm_cxt.frm_num = 0;
            //		sem_post(&cxt->hdr_sync_sm);
        }
#endif
        ret = camera_close_hdr(cxt);
        if (ret) {
            CMR_LOGE("failed to close hdr");
        }

        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_CLEAR_HDR, &setting_param);
    }

    if (cxt->snp_cxt.is_mfsr)
       camera_close_mfsr(cxt);

    if (camera_get_fdr_flag(cxt)) {
	CMR_LOGD("close_fdr\n");
        ret = camera_close_fdr(cxt);
        if(ret) {
           CMR_LOGE("failed to close fdr");
        }
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_CLEAR_FDR, &setting_param);
        if (ret) {
            CMR_LOGE("failed to clear fdr sem");
        }
    }

    cxt->snp_cxt.status = IDLE;
    cxt->snp_cxt.start_capture_flag = 0;

    cxt->is_start_snapshot = 0;
    cxt->prev_cxt.snapshot_eb = 0;

    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      CAMERA_PARAM_PREVIEW_SIZE, &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle, CAMERA_PARAM_VIDEO_SIZE,
                      &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      CAMERA_PARAM_YUV_CALLBACK_SIZE, &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle, CAMERA_PARAM_YUV2_SIZE,
                      &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle, CAMERA_PARAM_RAW_SIZE,
                      &setting_param);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      CAMERA_PARAM_CAPTURE_SIZE, &setting_param);

exit:
    CMR_LOGD("X, ret=%ld", ret);
    ATRACE_END();
    sem_post(&cxt->snapshot_sm);
    return ret;
}

cmr_int camera_local_get_prev_rect(cmr_handle oem_handle,
                                   struct img_rect *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!param_ptr) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = cmr_preview_get_prev_rect(cxt->prev_cxt.preview_handle,
                                    cxt->camera_id, param_ptr);
    CMR_LOGI("%d %d %d %d", param_ptr->start_x, param_ptr->start_y,
             param_ptr->width, param_ptr->height);
exit:
    return ret;
}

cmr_int camera_get_sensor_mode_info(cmr_handle oem_handle,
                                    struct sensor_mode_info *mode_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_exp_info *sensor_info = NULL;
    cmr_uint sensor_mode = SENSOR_MODE_MAX;
    cmr_uint i;

    if (!oem_handle || !mode_info) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    sensor_info =
        (struct sensor_exp_info *)malloc(sizeof(struct sensor_exp_info));
    if (!sensor_info) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    ret = camera_get_sensor_info(cxt, cxt->camera_id, sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    for (i = SENSOR_MODE_COMMON_INIT; i < SENSOR_MODE_MAX; i++) {
        mode_info[i].trim_start_x = sensor_info->mode_info[i].trim_start_x;
        mode_info[i].trim_start_y = sensor_info->mode_info[i].trim_start_y;
        mode_info[i].trim_width = sensor_info->mode_info[i].trim_width;
        mode_info[i].trim_height = sensor_info->mode_info[i].trim_height;
    }

exit:
    if (sensor_info) {
        free(sensor_info);
        sensor_info = NULL;
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_get_sensor_mode_trim(cmr_handle oem_handle,
                                    struct img_rect *sn_trim) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_exp_info *sensor_info = NULL;
    cmr_u32 sensor_mode = SENSOR_MODE_MAX;

    if (!oem_handle || !sn_trim) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    sensor_info =
        (struct sensor_exp_info *)malloc(sizeof(struct sensor_exp_info));
    if (!sensor_info) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    ret = camera_get_sensor_info(cxt, cxt->camera_id, sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                              &sensor_mode);
    CMR_LOGD("camera_id =%d sns mode =%d", cxt->camera_id, sensor_mode);
    if (sensor_mode >= SENSOR_MODE_MAX) {
        // note:cmr_sensor_get_mode would not set parameter.so sensor_mode
        // ==SENSOR_MODE_MAX
        // then mode_info[sensor_mode] would overflow.
        CMR_LOGE("cmr_sensor_get_mode failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    if (sensor_info->mode_info[sensor_mode].trim_width > PICTURE_W) {
        sn_trim->start_x = sensor_info->mode_info[sensor_mode].trim_start_x;
        sn_trim->start_y = sensor_info->mode_info[sensor_mode].trim_start_y;
        sn_trim->width = sensor_info->mode_info[sensor_mode].trim_width >> 1;
        sn_trim->height = sensor_info->mode_info[sensor_mode].trim_height >> 1;
    } else {
        sn_trim->start_x = sensor_info->mode_info[sensor_mode].trim_start_x;
        sn_trim->start_y = sensor_info->mode_info[sensor_mode].trim_start_y;
        sn_trim->width = sensor_info->mode_info[sensor_mode].trim_width;
        sn_trim->height = sensor_info->mode_info[sensor_mode].trim_height;
    }
    CMR_LOGD("sensor x=%d y=%d w=%d h=%d", sn_trim->start_x, sn_trim->start_y,
             sn_trim->width, sn_trim->height);
exit:
    if (sensor_info) {
        free(sensor_info);
        sensor_info = NULL;
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_get_senor_mode_trim2(cmr_handle oem_handle,
                                    struct img_rect *sn_trim) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt = &cxt->prev_cxt;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct sprd_dcam_path_size dcam_cfg;
    cmr_u32 sensor_mode = SENSOR_MODE_MAX;

    if (!oem_handle || !sn_trim) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    sensor_info =
        (struct sensor_exp_info *)malloc(sizeof(struct sensor_exp_info));
    if (!sensor_info) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    ret = camera_get_sensor_info(cxt, cxt->camera_id, sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                              &sensor_mode);
    CMR_LOGD("camera_id =%d sns mode =%d", cxt->camera_id, sensor_mode);
    if (sensor_mode >= SENSOR_MODE_MAX) {
        // note:cmr_sensor_get_mode would not set parameter.so sensor_mode
        // ==SENSOR_MODE_MAX
        // then mode_info[sensor_mode] would overflow.
        CMR_LOGE("cmr_sensor_get_mode failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    sensor_mode_info = &sensor_info->mode_info[sensor_mode];
    dcam_cfg.dcam_in_w = sensor_mode_info->trim_width;
    dcam_cfg.dcam_in_h = sensor_mode_info->trim_height;
    dcam_cfg.pre_dst_w = prev_cxt->size.width;
    dcam_cfg.pre_dst_h = prev_cxt->size.height;
    dcam_cfg.vid_dst_w = prev_cxt->actual_video_size.width;
    dcam_cfg.vid_dst_h = prev_cxt->actual_video_size.height;

    ret = camera_channel_dcam_size(oem_handle, &dcam_cfg);
    if (ret) {
        CMR_LOGE("get dcam output size failed.");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    sn_trim->width = dcam_cfg.dcam_out_w;
    sn_trim->height = dcam_cfg.dcam_out_h;

    CMR_LOGD("sensor x=%d y=%d w=%d h=%d", sn_trim->start_x, sn_trim->start_y,
             sn_trim->width, sn_trim->height);
exit:
    if (sensor_info) {
        free(sensor_info);
        sensor_info = NULL;
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_uint camera_get_preview_angle(cmr_handle oem_handle) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = NULL;
    struct setting_cmd_parameter setting_param;

    if (!oem_handle) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    setting_cxt = &cxt->setting_cxt;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_PREVIEW_ANGLE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get preview angle %ld", ret);
        goto exit;
    }
    ret = setting_param.cmd_type_value;
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_uint camera_get_exif_info(cmr_handle oem_handle,
                              struct exif_info *exif_info) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = NULL;
    struct setting_cmd_parameter setting_param;
    EXIF_SPEC_PIC_TAKING_COND_T *exif_spec = NULL;

    if (!oem_handle) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    setting_cxt = &cxt->setting_cxt;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_EXIF_INFO, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get exif %ld", ret);
        goto exit;
    }
    exif_spec = setting_param.exif_all_info_ptr->spec_ptr->pic_taking_cond_ptr;
    exif_info->aperture = (float)cxt->sn_cxt.cur_sns_ex_info.f_num / 100.0;
    exif_info->focus_distance = (float)exif_spec->FocalLength.numerator /
                                (float)exif_spec->FocalLength.denominator;

exit:
    CMR_LOGI("apet %f focus dist %f", exif_info->aperture,
             exif_info->focus_distance);
    return ret;
}

cmr_uint camera_get_result_exif_info(
    cmr_handle oem_handle,
    struct exif_spec_pic_taking_cond_tag *exif_pic_info) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = NULL;
    struct setting_cmd_parameter setting_param;

    if (!oem_handle || !exif_pic_info) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    setting_cxt = &cxt->setting_cxt;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_GET_EXIF_PIC_INFO, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get exif %ld", ret);
        goto exit;
    }
    memcpy(exif_pic_info, &setting_param.exif_pic_cond_info,
           sizeof(struct exif_spec_pic_taking_cond_tag));
    exif_pic_info->ApertureValue.numerator = cxt->sn_cxt.cur_sns_ex_info.f_num;
    exif_pic_info->ApertureValue.denominator = 100;
    CMR_LOGI("apet:%d,%d.\n", exif_pic_info->ApertureValue.numerator,
             exif_pic_info->ApertureValue.denominator);
exit:
    return ret;
}

cmr_int camera_local_get_zsl_info(cmr_handle oem_handle, cmr_uint *is_support,
                                  cmr_uint *max_width, cmr_uint *max_height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct preview_zsl_info zsl_info;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !is_support || !max_width || !max_height) {
        CMR_LOGE("error param");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = cmr_preview_is_support_zsl(cxt->prev_cxt.preview_handle,
                                     cxt->camera_id, is_support);
    if (ret) {
        CMR_LOGE("failed to get zsl info %ld", ret);
        goto exit;
    }

    ret = cmr_preview_get_max_cap_size(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, max_width, max_height);
    if (ret) {
        CMR_LOGE("failed to get max cap size %ld", ret);
        goto exit;
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_local_redisplay_data(
    cmr_handle oem_handle, cmr_s32 output_fd, cmr_uint output_addr,
    cmr_uint output_vir_addr, cmr_uint output_width, cmr_uint output_height,
    cmr_s32 input_fd, cmr_uint input_addr_y, cmr_uint input_addr_uv,
    cmr_uint input_vir_addr, cmr_uint input_width, cmr_uint input_height) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct img_frm src_img;
    struct img_frm dst_img;
    struct cmr_rot_param rot_param;
    struct img_rect rect;
    enum img_angle angle = IMG_ANGLE_0;
    cmr_uint img_len = (cmr_uint)(output_width * output_height);
    struct setting_context *setting_cxt;
    struct setting_cmd_parameter setting_param;

    if (!oem_handle) {
        CMR_LOGE("err, handle is null");
        goto exit;
    }

    cmr_bzero(&src_img, sizeof(struct img_frm));
    cmr_bzero(&dst_img, sizeof(struct img_frm));
    rect.start_x = 0;
    rect.start_y = 0;
    rect.width = input_width;
    rect.height = input_height;
    src_img.rect = rect;
    src_img.size.width = input_width;
    src_img.size.height = input_height;
    src_img.fd = input_fd;
    src_img.addr_phy.addr_y = input_addr_y;
    src_img.addr_phy.addr_u = input_addr_uv;
    src_img.fmt = CAM_IMG_FMT_YUV420_NV21;
    cmr_grab_get_dcam_endian(&cxt->snp_cxt.data_endian, &src_img.data_end);

    dst_img.size.width = output_width;
    dst_img.size.height = output_height;
    dst_img.fd = output_fd;
    dst_img.addr_phy.addr_y = output_addr;
    dst_img.addr_phy.addr_u =
        dst_img.addr_phy.addr_y + output_width * output_height;
    dst_img.fmt = CAM_IMG_FMT_YUV420_NV21;
    cmr_grab_get_dcam_endian(&cxt->snp_cxt.data_endian, &dst_img.data_end);

#ifdef CAMERA_BRINGUP
    CMR_LOGD("cpp is not ok, sw scale crashed, so dont do scale here");
    camera_scale_down_software(&src_img, &dst_img);
#else
    CMR_LOGD("src_img with %d, height %d, dst_img with %d, height %d",
             src_img.size.width, src_img.size.height, dst_img.size.width,
             dst_img.size.height);
    ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, &src_img, &dst_img,
                          (cmr_evt_cb)NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to start start %ld", ret);
        ret = -CMR_CAMERA_FAIL;
        goto exit;
    }
#endif

exit:
    ATRACE_END();
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_local_fd_start(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_LOGV("fd %d", cxt->fd_on_off);
    if (PREVIEWING ==
        cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id)) {
        ret = cmr_preview_ctrl_facedetect(cxt->prev_cxt.preview_handle,
                                          cxt->camera_id, cxt->fd_on_off);
        if (ret) {
            CMR_LOGE("failed to fd ctrl %ld", ret);
        }
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_isp_set_params(cmr_handle oem_handle, enum camera_param_type id,
                              cmr_uint param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    cmr_uint isp_cmd = ISP_CTRL_MAX;
    cmr_u32 isp_param = 0;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct cmr_ae_param ae_param;
    struct isp_pos_rect trim;
    struct cmr_ae_compensation_param ae_compensation_param;
    struct isp_exp_compensation ae_compensation;

    cmr_u32 ptr_flag = 0;
    cmr_uint set_exif_flag = 0;
    cmr_uint set_isp_flag = 1;

    void *isp_param_ptr = NULL;
    SENSOR_EXIF_CTRL_E exif_cmd = SENSOR_EXIF_CTRL_MAX;

    switch (id) {
    case CAMERA_PARAM_AUTO_EXPOSURE_MODE:
        ae_param = *(struct cmr_ae_param *)param;
        isp_cmd = ISP_CTRL_AE_MEASURE_LUM;
        isp_param = ae_param.mode;
        CMR_LOGD("set AE mode %d", isp_param);
        break;

    case CAMERA_PARAM_ISP_AE_LOCK_UNLOCK:
        isp_cmd = ISP_CTRL_SET_AE_LOCK_UNLOCK;
        isp_param = param;
        CMR_LOGD("set AE Lock & Unlock %d", isp_param);
        break;

    case CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK:
        isp_cmd = ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK;
        isp_param = param;
        if (isp_param == ISP_AWB_UNLOCK)
            isp_param = ISP_AWB_UNLOCK;
        else if (isp_param == ISP_AWB_LOCK)
            isp_param = ISP_AWB_LOCK;
        break;

    case CAMERA_PARAM_ANTIBANDING:
        isp_cmd = ISP_CTRL_FLICKER;
        isp_param = param;
        CMR_LOGD("flicker %d", isp_param);
        break;

    case CAMERA_PARAM_ISO:
        isp_cmd = ISP_CTRL_ISO;
        set_exif_flag = 1;
        exif_cmd = SENSOR_EXIF_CTRL_ISOSPEEDRATINGS;
        isp_param = param;
        CMR_LOGD("iso %d", param);
        break;

    case CAMERA_PARAM_AE_REGION:
        isp_cmd = ISP_CTRL_AE_TOUCH;
        ae_param = *(struct cmr_ae_param *)param;

        trim.start_x = ae_param.win_area.rect[0].start_x;
        trim.start_y = ae_param.win_area.rect[0].start_y;
        if (0 == ae_param.win_area.rect[0].width)
            trim.end_x = 0;
        else
            trim.end_x = ae_param.win_area.rect[0].start_x +
                         ae_param.win_area.rect[0].width - 1;

        if (0 == ae_param.win_area.rect[0].height)
            trim.end_y = 0;
        else
            trim.end_y = ae_param.win_area.rect[0].start_y +
                         ae_param.win_area.rect[0].height - 1;

        CMR_LOGD("AE ROI camera_id %d, x, y , w, h, (%d,%d,%d,%d)",
                 cxt->camera_id, trim.start_x, trim.start_y, trim.end_x,
                 trim.end_y);
        ptr_flag = 1;
        isp_param_ptr = (void *)&trim;
        break;

    case CAMERA_PARAM_EXPOSURE_COMPENSATION:
        isp_cmd = ISP_CTRL_AE_EXP_COMPENSATION;
        ptr_flag = 1;
        ae_compensation_param = *(struct cmr_ae_compensation_param *)param;

        ae_compensation.comp_val =
            ae_compensation_param.ae_exposure_compensation;
        ae_compensation.comp_range.min =
            ae_compensation_param.ae_compensation_range[0];
        ae_compensation.comp_range.max =
            ae_compensation_param.ae_compensation_range[1];
        ae_compensation.step_numerator =
            ae_compensation_param.ae_compensation_step_numerator;
        ae_compensation.step_denominator =
            ae_compensation_param.ae_compensation_step_denominator;
        isp_param_ptr = (void *)&ae_compensation;
        CMR_LOGD("ae compensation: comp_val=%d, range.min=%d, range.max=%d, "
                 "step_numerator=%d, step_denominator=%d",
                 ae_compensation.comp_val, ae_compensation.comp_range.min,
                 ae_compensation.comp_range.max, ae_compensation.step_numerator,
                 ae_compensation.step_denominator);
        break;

    case CAMERA_PARAM_EFFECT:
        isp_cmd = ISP_CTRL_SPECIAL_EFFECT;
        isp_param = param;
        break;

    case CAMERA_PARAM_BRIGHTNESS:
        isp_cmd = ISP_CTRL_BRIGHTNESS;
        isp_param = param;
        break;

    case CAMERA_PARAM_CONTRAST:
        isp_cmd = ISP_CTRL_CONTRAST;
        isp_param = param;
        break;

    case CAMERA_PARAM_SATURATION:
        isp_cmd = ISP_CTRL_SATURATION;
        isp_param = param;
        break;

    case CAMERA_PARAM_F_NUMBER:
        set_exif_flag = 1;
        set_isp_flag = 0;
        exif_cmd = SENSOR_EXIF_CTRL_FNUMBER;
        isp_param = param;
        CMR_LOGD("aperture %d", param);
        break;
     case CAMERA_PARAM_FACE_BEAUTY_ENABLE:
        set_isp_flag = 1;
        isp_cmd = ISP_CTRL_SET_FACEBEAUTY_ENABLE;
        isp_param = param;
        CMR_LOGD("face beauty %d", param);
        break;

    default:
        CMR_LOGE("don't support cmd %ld", id);
        ret = CMR_CAMERA_NO_SUPPORT;
        break;
    }

    if (ptr_flag) {
        ret = isp_ioctl(isp_cxt->isp_handle, isp_cmd, isp_param_ptr);
        if (ret) {
            CMR_LOGE("failed isp ioctl %ld", ret);
        }
        CMR_LOGV("done %ld and direct return", ret);
        return ret;
    }

    if (set_isp_flag) {
        ret = isp_ioctl(isp_cxt->isp_handle, isp_cmd, (void *)&isp_param);
        if (ret) {
            CMR_LOGE("failed isp ioctl %ld", ret);
        } else {
            if (CAMERA_PARAM_ISO == id) {
                if (0 == param) {
                    isp_capability(isp_cxt->isp_handle, ISP_CUR_ISO,
                                   (void *)&isp_param);
                    cxt->setting_cxt.is_auto_iso = 1;
                } else {
                    cxt->setting_cxt.is_auto_iso = 0;
                }
                if (cxt->setting_cxt.is_auto_iso != 1) {
                    isp_param = POWER2(isp_param - 1) * ONE_HUNDRED;
                }
                CMR_LOGD("auto iso %d, exif iso %d",
                         cxt->setting_cxt.is_auto_iso, isp_param);
            }
        }
    }

    if (set_exif_flag) {
        cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, exif_cmd,
                            isp_param);
    }

    return ret;
}

cmr_int camera_local_set_param(cmr_handle oem_handle, enum camera_param_type id,
                               uint64_t param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CHECK_HANDLE_VALID(oem_handle);

    switch (id) {
    case CAMERA_PARAM_FOCUS_RECT:
        CMR_LOGD("set focus rect 0x%lx", param);
        ret = cmr_focus_set_param(cxt->focus_cxt.focus_handle, cxt->camera_id,
                                  id, (void *)param);
        break;
    case CAMERA_PARAM_AF_MODE:
        CMR_LOGD("set focus af mode 0x%lx", param);
        ret = cmr_focus_set_param(cxt->focus_cxt.focus_handle, cxt->camera_id,
                                  id, (void *)param);
        break;
    case CAMERA_PARAM_ZOOM: {
        struct cmr_zoom zoom_factor;
        const float EPSINON = 0.0001f;
        struct cmr_zoom_param *zoom_param =
            (struct cmr_zoom_param *)malloc(sizeof(struct cmr_zoom_param));
        struct cmr_zoom_param *zoom_reprocess =
            (struct cmr_zoom_param *)malloc(sizeof(struct cmr_zoom_param));
        cmr_uint zoom_factor_changed = 0;
        struct common_isp_cmd_param isp_cmd_parm;
        struct setting_context *setting_cxt = &cxt->setting_cxt;
        struct setting_cmd_parameter setting_param;

        cmr_bzero(&setting_param, sizeof(setting_param));
        cmr_bzero(&isp_cmd_parm, sizeof(struct common_isp_cmd_param));
        if (param) {
            cmr_bzero(zoom_param, sizeof(struct cmr_zoom_param));
            cmr_bzero(zoom_reprocess, sizeof(struct cmr_zoom_param));
            memcpy(zoom_param, (struct cmr_zoom_param *)param,
                   sizeof(struct cmr_zoom_param));
            memcpy(zoom_reprocess, (struct cmr_zoom_param *)param,
                   sizeof(struct cmr_zoom_param));

            if (zoom_param->mode == ZOOM_INFO) {
                ret = cmr_preview_get_zoom_factor(cxt->prev_cxt.preview_handle,
                                                  cxt->camera_id, &zoom_factor);
                if (ret) {
                    CMR_LOGE("fail to get zoom factor  %ld", ret);
                } else {
                    if (!zoom_factor.prev_zoom) {
                        zoom_param->zoom_info.prev_aspect_ratio = 1.0f;
                    }
                    if (!zoom_factor.cap_zoom) {
                        zoom_param->zoom_info.capture_aspect_ratio = 1.0f;
                    }
                    if (!zoom_factor.video_zoom) {
                        zoom_param->zoom_info.video_aspect_ratio = 1.0f;
                    }
                    if (fabs(zoom_param->zoom_info.prev_aspect_ratio -
                             zoom_factor.zoom_setting.zoom_info
                                 .prev_aspect_ratio) > EPSINON ||
                        fabs(zoom_param->zoom_info.capture_aspect_ratio -
                             zoom_factor.zoom_setting.zoom_info
                                 .capture_aspect_ratio) > EPSINON ||
                        fabs(zoom_param->zoom_info.video_aspect_ratio -
                             zoom_factor.zoom_setting.zoom_info
                                 .video_aspect_ratio) > EPSINON) {
                        zoom_factor_changed = 1;
                    }
                    cxt->zoom_ratio =
                        zoom_factor.zoom_setting.zoom_info.prev_aspect_ratio;
                    CMR_LOGD(
                        "id=%d, zoom_factor_changed=%d, zoom=%f, orgZoom=%f, "
                        "prev %d, cap %d, video %d",
                        cxt->camera_id, zoom_factor_changed,
                        zoom_param->zoom_info.prev_aspect_ratio,
                        zoom_factor.zoom_setting.zoom_info.prev_aspect_ratio,
                        zoom_factor.prev_zoom, zoom_factor.cap_zoom,
                        zoom_factor.video_zoom);
                }
            }
        }
        ret = cmr_preview_update_zoom(cxt->prev_cxt.preview_handle,
                                      cxt->camera_id, zoom_param);
        if (ret) {
            CMR_LOGE("failed to update zoom %ld", ret);
        }

        setting_param.camera_id = cxt->camera_id;
        ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                SETTING_GET_APPMODE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get app mode %ld", ret);
        }
        if (setting_param.cmd_type_value == CAMERA_MODE_AUTO_PHOTO){
            if (cxt->need_sr) {
                    isp_cmd_parm.cmd_value = ISP_SPORT;
                    ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_ZOOM_SCENE, &isp_cmd_parm);
                    if (ret)
                        CMR_LOGE("isp set MFSR  failed");
            } else {
                if (cxt->zoom_ratio - 1.0f > 0.0001f){
                    isp_cmd_parm.cmd_value = ISP_ZOOM;
                    ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_ZOOM_SCENE, &isp_cmd_parm);
                    if (ret)
                        CMR_LOGE("isp set ISP_ZOOM  failed");
                }
                if (cxt->zoom_ratio - 1.0f <= 0.0001f){
                    isp_cmd_parm.cmd_value = ISP_AUTO;
                    ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_ZOOM_SCENE, &isp_cmd_parm);
                    if (ret)
                        CMR_LOGE("isp set ISP_AUTO  failed");
                }
            }
        }


        ret = camera_set_setting(oem_handle, id, (cmr_uint)zoom_param);
        if (ret) {
            CMR_LOGE("failed to set camera setting of zoom %ld", ret);
        }
        ret = camera_set_setting(oem_handle, CAMERA_PARAM_REPROCESS_ZOOM_RATIO,
                                 (cmr_uint)zoom_reprocess);
        if (zoom_factor_changed) {
            ret = cmr_set_zoom_factor_to_isp(oem_handle,
                                             &zoom_param->zoom_info.zoom_ratio);
            if (ret) {
                CMR_LOGE("failed to set zoom factor to isp  %ld", ret);
            }
        }
        free(zoom_param);
        free(zoom_reprocess);
        break;
    case CAMERA_PARAM_REPROCESS_ZOOM_RATIO:
        ret = camera_set_setting(oem_handle, id, param);
        if (ret) {
            CMR_LOGE("failed to set camera setting of zoom ratio %ld", ret);
        }
        break;
    }
    /*
    case CAMERA_PARAM_ISO:
        cxt->setting_cxt.iso_value = param;
        ret = camera_set_setting(oem_handle, id, param);
        break;
    */
    case CAMERA_PARAM_AUTO_EXPOSURE_MODE:
    case CAMERA_PARAM_ISP_AE_LOCK_UNLOCK:
    case CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK:
    case CAMERA_PARAM_ANTIBANDING:
    // case CAMERA_PARAM_ISO:
    case CAMERA_PARAM_AE_REGION:
    // case CAMERA_PARAM_EXPOSURE_COMPENSATION:
    case CAMERA_PARAM_EFFECT:
    case CAMERA_PARAM_BRIGHTNESS:
    case CAMERA_PARAM_CONTRAST:
    case CAMERA_PARAM_SATURATION:
    case CAMERA_PARAM_F_NUMBER:
    case CAMERA_PARAM_FACE_BEAUTY_ENABLE:
        ret = camera_isp_set_params(oem_handle, id, param);
        break;

    case CAMERA_PARAM_TOUCH_INFO_TO_FD:
        ret = cmr_preview_set_fd_touch_param(cxt->prev_cxt.preview_handle,
                                             cxt->camera_id,
                                             (struct fd_touch_info *)param);
        break;

    case CAMERA_PARAM_WRITE_CALIBRATION_OTP_DATA: {
        struct cal_otp_info *temp_info = (struct cal_otp_info *)param;
        ret = camera_write_calibration_otp(oem_handle,(struct cal_otp_info *)param);
        if (0 == ret) {
            temp_info->cal_otp_result = CAMERA_CALIC_OTP_SUCCESS;
        } else {
            temp_info->cal_otp_result = CAMERA_CALIC_OTP_FAIL;
        }
        break;
    }

    case CAMERA_PARAM_SPRD_SUPER_MACROPHOTO_ENABLE: {
        ret = camera_set_setting(oem_handle, id, param);
        if (ret) {
            CMR_LOGE("failed to set super macrophoto enable %ld", ret);
        }
        break;
    }

    case CAMERA_PARAM_3RD_3DNR_ENABLED: {
        cxt->_3rd_3dnr_flag = param;
        break;
    }

    case CAMERA_PARAM_SET_TOP_APP_ID: {
        cxt->app_id = (enum top_app_id)param;
        break;
    }

    default:
        ret = camera_set_setting(oem_handle, id, param);
        break;
    }
    if (ret) {
        CMR_LOGE("failed to set param %ld", ret);
    }
exit:
    return ret;
}

cmr_int camera_local_start_focus(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    ret = cmr_pre_flash_notice_flash(cxt->setting_cxt.setting_handle);
    ret = cmr_af_start_notice_focus(cxt->focus_cxt.focus_handle);
    ret = cmr_focus_start(cxt->focus_cxt.focus_handle, cxt->camera_id);

    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_int camera_local_cancel_focus(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if(1 != camera_get_fdr_flag(cxt)) {
        ret = cmr_setting_cancel_notice_flash(cxt->setting_cxt.setting_handle);
    }
    ret = cmr_af_cancel_notice_focus(cxt->focus_cxt.focus_handle);
    ret = cmr_focus_stop(cxt->focus_cxt.focus_handle, cxt->camera_id, 1);

    CMR_LOGV("done %ld", ret);
    return ret;
}

cmr_int camera_local_pre_flash(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    CMR_LOGV("E");

    setting_param.camera_id = cxt->camera_id;

    if(1 != camera_get_fdr_flag(cxt)) {
        ret = cmr_pre_flash_notice_flash(cxt->setting_cxt.setting_handle);
    }
    setting_param.setting_flash_status = SETTING_FLASH_PRE_LIGHTING;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                            SETTING_SET_PRE_LOWFLASH, &setting_param);
    if (ret) {
        CMR_LOGE("failed to open flash");
    }

    return ret;
}

cmr_int camera_local_get_viewangle(cmr_handle oem_handle,
                                   struct sensor_view_angle *view_angle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct sensor_exp_info exp_info;

    if (!oem_handle || !view_angle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    cxt = (struct camera_context *)oem_handle;
    ret = cmr_sensor_get_info(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                              &exp_info);
    if (ret) {
        CMR_LOGE("failed to get sensor info %ld", ret);
        goto exit;
    }

    view_angle->horizontal_val = exp_info.view_angle.horizontal_val;
    view_angle->vertical_val = exp_info.view_angle.vertical_val;
exit:
    return ret;
}
cmr_int camera_local_set_preview_buffer(cmr_handle oem_handle,
                                        cmr_uint src_phy_addr,
                                        cmr_uint src_vir_addr, cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct sensor_exp_info exp_info;
    cam_buffer_info_t buffer;
    if (!oem_handle || !fd || !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cxt = (struct camera_context *)oem_handle;

    cmr_bzero(&buffer, sizeof(cam_buffer_info_t));
    buffer.fd = fd;
    buffer.addr_phy = (void *)src_phy_addr;
    buffer.addr_vir = (void *)src_vir_addr;

    ret = cmr_preview_set_preview_buffer(cxt->prev_cxt.preview_handle,
                                         cxt->camera_id, buffer);
    if (ret) {
        CMR_LOGE("failed to set preview buffer %ld", ret);
        goto exit;
    }
exit:
    return ret;
}
cmr_int camera_local_set_video_buffer(cmr_handle oem_handle,
                                      cmr_uint src_phy_addr,
                                      cmr_uint src_vir_addr, cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct sensor_exp_info exp_info;
    cam_buffer_info_t buffer;
    memset(&buffer, 0, sizeof(cam_buffer_info_t));

    if (!oem_handle || !fd || !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cxt = (struct camera_context *)oem_handle;
    buffer.fd = fd;
    buffer.addr_phy = (void *)src_phy_addr;
    buffer.addr_vir = (void *)src_vir_addr;

    ret = cmr_preview_set_video_buffer(cxt->prev_cxt.preview_handle,
                                       cxt->camera_id, buffer);
    if (ret) {
        CMR_LOGE("failed to set video buffer %ld", ret);
        goto exit;
    }
exit:
    return ret;
}

cmr_int camera_local_set_zsl_buffer(cmr_handle oem_handle,
                                    cmr_uint src_phy_addr,
                                    cmr_uint src_vir_addr, cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct sensor_exp_info exp_info;
    if (!oem_handle || !fd || !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cxt = (struct camera_context *)oem_handle;
    ret =
        cmr_preview_set_zsl_buffer(cxt->prev_cxt.preview_handle, cxt->camera_id,
                                   src_phy_addr, src_vir_addr, fd);
    if (ret) {
        CMR_LOGE("failed to set zsl buffer %ld", ret);
        goto exit;
    }
exit:
    return ret;
}

cmr_s32 local_queue_buffer(cmr_handle oem_handle, cam_buffer_info_t buffer,
                           int steam_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct sensor_exp_info exp_info;
    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cxt = (struct camera_context *)oem_handle;

    switch (steam_type) {
    case SPRD_CAM_STREAM_PREVIEW:
        ret = cmr_preview_set_preview_buffer(cxt->prev_cxt.preview_handle,
                                             cxt->camera_id, buffer);
        if (ret) {
            CMR_LOGE("cmr_preview_set_preview_buffer failed");
            goto exit;
        }
        break;

    case SPRD_CAM_STREAM_VIDEO:
        ret = cmr_preview_set_video_buffer(cxt->prev_cxt.preview_handle,
                                           cxt->camera_id, buffer);
        if (ret) {
            CMR_LOGE("cmr_preview_set_video_buffer failed");
            goto exit;
        }
        break;

    case SPRD_CAM_STREAM_CALLBACK:
        ret = cmr_channel2_queue_buffer(cxt->prev_cxt.preview_handle,
                                        cxt->camera_id, buffer);
        if (ret) {
            CMR_LOGE("cmr_channel2_queue_buffer failed");
            goto exit;
        }
        break;

    case SPRD_CAM_STREAM_YUV2:
        ret = cmr_channel3_queue_buffer(cxt->prev_cxt.preview_handle,
                                        cxt->camera_id, buffer);
        if (ret) {
            CMR_LOGE("cmr_channel3_queue_buffer failed");
            goto exit;
        }
        break;

    case SPRD_CAM_STREAM_RAW:
        ret = cmr_channel0_queue_buffer(cxt->prev_cxt.preview_handle,
                                        cxt->camera_id, buffer);
        if (ret) {
            CMR_LOGE("cmr_channel0_queue_buffer failed");
            goto exit;
        }
        break;

    // channel1 will be used later
    //    case FOR_CHANNEL1:
    //        cmr_channel1_queue_buffer(cxt->prev_cxt.preview_handle,
    //        cxt->camera_id, buffer);
    //        break;

    // channel4 will be used later
    //    case FOR_CHANNEL4:
    //        cmr_channel4_queue_buffer(cxt->prev_cxt.preview_handle,
    //        cxt->camera_id, buffer);
    //        break;
    default:
        break;
    }

exit:
    return ret;
}

cmr_int camera_local_set_video_snapshot_buffer(cmr_handle oem_handle,
                                               cmr_uint src_phy_addr,
                                               cmr_uint src_vir_addr,
                                               cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct frm_info chn_data;
    cmr_u32 buffer_size = 0;
    if (!oem_handle || !fd || !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    camera_get_iso_value(oem_handle);

    cxt = (struct camera_context *)oem_handle;
    CMR_LOGI("in video w=%d h=%d cap w=%d h=%d",
             cxt->prev_cxt.actual_video_size.width,
             cxt->prev_cxt.actual_video_size.height,
             cxt->snp_cxt.actual_capture_size.width,
             cxt->snp_cxt.actual_capture_size.height);

    if (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt)) {
        if (cxt->snp_cxt.actual_capture_size.width *
                cxt->snp_cxt.actual_capture_size.height <
            cxt->prev_cxt.actual_video_size.width *
                cxt->prev_cxt.actual_video_size.height)
            buffer_size = cxt->snp_cxt.actual_capture_size.width *
                          cxt->snp_cxt.actual_capture_size.height;
        else
            buffer_size = cxt->prev_cxt.actual_video_size.width *
                          cxt->prev_cxt.actual_video_size.height;
        cmr_bzero(&chn_data, sizeof(struct frm_info));
        cmr_copy(&chn_data, &cxt->prev_cxt.video_cur_chn_data,
                 sizeof(struct frm_info));
        chn_data.fd = fd;
        chn_data.yaddr = src_phy_addr;
        chn_data.uaddr = src_phy_addr + buffer_size;
        chn_data.vaddr = 0;
        chn_data.yaddr_vir = src_vir_addr;
        chn_data.uaddr_vir = src_vir_addr + buffer_size;
        chn_data.vaddr_vir = 0;
        ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                        SNAPSHOT_EVT_CHANNEL_DONE,
                                        (void *)&chn_data);
    } else {
        CMR_LOGE("snapshot is not ready");
    }
    if (ret) {
        CMR_LOGE("failed to set snapshot buffer %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int camera_local_set_zsl_snapshot_buffer(cmr_handle oem_handle,
                                             cmr_uint src_phy_addr,
                                             cmr_uint src_vir_addr,
                                             cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct frm_info chn_data;
    cmr_u32 buffer_size = 0;
    cmr_int need_pause = 0;

    if (!oem_handle || !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    cxt = (struct camera_context *)oem_handle;
    camera_local_zsl_snapshot_need_pause(oem_handle, &need_pause);
    if (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt)) {
        camera_take_snapshot_step(CMR_STEP_CAP_S);
        buffer_size = cxt->snp_cxt.actual_capture_size.width *
                      cxt->snp_cxt.actual_capture_size.height;
        cmr_bzero(&chn_data, sizeof(struct frm_info));
        cmr_copy(&chn_data, &cxt->snp_cxt.cur_chn_data,
                 sizeof(struct frm_info));
        chn_data.yaddr = src_phy_addr;
        chn_data.uaddr = src_phy_addr + buffer_size;
        chn_data.vaddr = 0;
        chn_data.yaddr_vir = src_vir_addr;
        chn_data.uaddr_vir = src_vir_addr + buffer_size;
        chn_data.vaddr_vir = 0;
        chn_data.fd = fd;

#ifdef PERFORMANCE_OPTIMIZATION
        // update postprocess params
        struct img_frm img_frame;
        memset(&img_frame, 0, sizeof(struct img_frm));
        img_frame.fmt = chn_data.fmt;
        img_frame.buf_size = buffer_size * 3 / 2; // ? or buffer_size * 3 /2
        img_frame.rect.start_x = 0;
        img_frame.rect.start_y = 0;
        img_frame.rect.width = cxt->snp_cxt.actual_capture_size.width;
        img_frame.rect.height = cxt->snp_cxt.actual_capture_size.height;
        img_frame.size.width = cxt->snp_cxt.actual_capture_size.width;
        img_frame.size.height = cxt->snp_cxt.actual_capture_size.height;
        img_frame.addr_phy.addr_y = src_phy_addr;
        img_frame.addr_phy.addr_u = src_phy_addr + buffer_size;
        img_frame.addr_phy.addr_v = 0;
        img_frame.addr_vir.addr_y = src_vir_addr;
        img_frame.addr_vir.addr_u = src_vir_addr + buffer_size;
        img_frame.addr_vir.addr_v = 0;
        img_frame.data_end.y_endian = 1;  // ?
        img_frame.data_end.uv_endian = 2; // ?
        img_frame.fd = fd;
        CMR_LOGD("in src vir addr[0x%lx, 0x%lx], img_frame.fd 0x%x",
                 img_frame.addr_vir.addr_y, img_frame.addr_vir.addr_u,
                 img_frame.fd);
        zsl_snp_update_post_proc_param(cxt->snp_cxt.snapshot_handle,
                                       (void *)&img_frame);
#endif
        ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                        SNAPSHOT_EVT_CHANNEL_DONE,
                                        (void *)&chn_data);
        if (need_pause) {
            camera_set_discard_frame(cxt, 1);
            ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle,
                                           cxt->camera_id, PREVIEW_CHN_PAUSE,
                                           (void *)&chn_data);
        }
        camera_capture_post_proc(cxt, cxt->camera_id);
    } else {
        CMR_LOGE("snapshot is not ready");
    }
    if (ret) {
        CMR_LOGE("failed to set snapshot buffer %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int camera_local_zsl_snapshot_need_pause(cmr_handle oem_handle,
                                             cmr_int *flag) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct cmr_path_capability capability;

    if (!oem_handle || !flag) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGV("E");
    cxt = (struct camera_context *)oem_handle;

    camera_channel_path_capability(oem_handle, &capability);
    //	*flag = capability.capture_pause;
    *flag = 0;
    CMR_LOGV("out flag %ld", *flag);
exit:
    return ret;
}

cmr_int camera_local_normal_snapshot_need_pause(cmr_handle oem_handle,
                                                cmr_int *flag) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt;
    struct cmr_path_capability capability;

    if (!oem_handle || !flag) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGV("in");
    cxt = (struct camera_context *)oem_handle;

    camera_channel_path_capability(oem_handle, &capability);
    *flag = capability.capture_pause;
    CMR_LOGV("out flag %ld", *flag);
exit:
    return ret;
}

void camera_local_start_burst_notice(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    uint32_t caf_switch = 0;

    isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_BURST_NOTICE, &caf_switch);
}

void camera_local_end_burst_notice(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    uint32_t caf_switch = 1;

    isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_BURST_NOTICE, &caf_switch);
}

cmr_int
camera_isp_set_sensor_info_to_af(cmr_handle oem_handle,
                                 struct cmr_af_aux_sensor_info *sensor_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct af_aux_sensor_info_t aux_sensor_info;
    if (cxt && isp_cxt && sensor_info) {
        memset(&aux_sensor_info, 0, sizeof(aux_sensor_info));
        switch (sensor_info->type) {
        case CAMERA_AF_ACCELEROMETER:
            aux_sensor_info.type = AF_ACCELEROMETER;
            aux_sensor_info.gsensor_info.timestamp =
                sensor_info->gsensor_info.timestamp;
            aux_sensor_info.gsensor_info.vertical_up =
                sensor_info->gsensor_info.vertical_up;
            aux_sensor_info.gsensor_info.vertical_down =
                sensor_info->gsensor_info.vertical_down;
            aux_sensor_info.gsensor_info.horizontal =
                sensor_info->gsensor_info.horizontal;
            ret = isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_SET_AUX_SENSOR_INFO,
                            &aux_sensor_info);
            break;
        case CAMERA_AF_MAGNETIC_FIELD:
            aux_sensor_info.type = AF_MAGNETIC_FIELD;
            break;
        case CAMERA_AF_GYROSCOPE:
            aux_sensor_info.type = AF_GYROSCOPE;
            aux_sensor_info.gyro_info.timestamp =
                sensor_info->gyro_info.timestamp;
            aux_sensor_info.gyro_info.x = sensor_info->gyro_info.x;
            aux_sensor_info.gyro_info.y = sensor_info->gyro_info.y;
            aux_sensor_info.gyro_info.z = sensor_info->gyro_info.z;
            ret = isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_SET_AUX_SENSOR_INFO,
                            &aux_sensor_info);
            break;
        case CAMERA_AF_LIGHT:
            aux_sensor_info.type = AF_LIGHT;
            break;
        case CAMERA_AF_PROXIMITY:
            aux_sensor_info.type = AF_PROXIMITY;
            break;
        default:
            CMR_LOGE("NO this sensor type: %d ", sensor_info->type);
            ret = CMR_CAMERA_INVALID_PARAM;
        }
    } else {
        CMR_LOGE("input param is null!");
        ret = CMR_CAMERA_INVALID_PARAM;
    }
    return ret;
}

static cmr_uint
camera_copy_sensor_fps_info_to_isp(struct isp_sensor_fps_info *out_isp_fps,
                                   SENSOR_MODE_FPS_T *in_fps) {
    if (NULL == in_fps || NULL == out_isp_fps) {
        CMR_LOGE("input param or out param is null!");
        return CMR_CAMERA_FAIL;
    }
    out_isp_fps->mode = in_fps->mode;
    out_isp_fps->max_fps = in_fps->max_fps;
    out_isp_fps->min_fps = in_fps->min_fps;
    out_isp_fps->is_high_fps = in_fps->is_high_fps;
    out_isp_fps->high_fps_skip_num = in_fps->high_fps_skip_num;

    return CMR_CAMERA_SUCCESS;
}

static cmr_uint
camera_copy_sensor_ex_info_to_isp(struct isp_sensor_ex_info *out_isp_sn_ex_info,
                                  struct sensor_ex_info *in_sn_ex_info,
                                  cmr_u32 sensor_id) {
    if (NULL == in_sn_ex_info || NULL == out_isp_sn_ex_info) {
        CMR_LOGE("input param or out param is null!");
        return CMR_CAMERA_FAIL;
    }
    out_isp_sn_ex_info->f_num = in_sn_ex_info->f_num;
    out_isp_sn_ex_info->focal_length = in_sn_ex_info->focal_length;
    out_isp_sn_ex_info->max_fps = in_sn_ex_info->max_fps;
    out_isp_sn_ex_info->max_adgain = in_sn_ex_info->max_adgain;
    out_isp_sn_ex_info->ois_supported = in_sn_ex_info->ois_supported;
    out_isp_sn_ex_info->pdaf_supported = in_sn_ex_info->pdaf_supported;
    out_isp_sn_ex_info->ebd_supported = in_sn_ex_info->embedded_line_enable;
    out_isp_sn_ex_info->long_expose_supported = in_sn_ex_info ->long_expose_supported;
    out_isp_sn_ex_info->exp_valid_frame_num =
        in_sn_ex_info->exp_valid_frame_num;
    out_isp_sn_ex_info->clamp_level = in_sn_ex_info->clamp_level;
    out_isp_sn_ex_info->adgain_valid_frame_num =
        in_sn_ex_info->adgain_valid_frame_num;
    out_isp_sn_ex_info->preview_skip_num = in_sn_ex_info->preview_skip_num;
    out_isp_sn_ex_info->capture_skip_num = in_sn_ex_info->capture_skip_num;
    out_isp_sn_ex_info->name = in_sn_ex_info->name;
    out_isp_sn_ex_info->sensor_version_info =
        in_sn_ex_info->sensor_version_info;
    out_isp_sn_ex_info->pos_dis.up2hori = in_sn_ex_info->pos_dis.up2hori;
    out_isp_sn_ex_info->pos_dis.hori2down = in_sn_ex_info->pos_dis.hori2down;
#ifdef TARGET_CAMERA_SENSOR_CCT_TCS3430
    out_isp_sn_ex_info->color_support = 1;
#else
    out_isp_sn_ex_info->color_support = 0;
#endif

    if (0 == sensor_id) {
#ifdef TARGET_CAMERA_SENSOR_TOF_VL53L0
        out_isp_sn_ex_info->tof_support = 1;
#else
        out_isp_sn_ex_info->tof_support = 0;
#endif
    } else {
        out_isp_sn_ex_info->tof_support = 0;
    }

    out_isp_sn_ex_info->fov_info.physical_size[0] =
        in_sn_ex_info->fov_info.physical_size[0];
    out_isp_sn_ex_info->fov_info.physical_size[1] =
        in_sn_ex_info->fov_info.physical_size[1];
    out_isp_sn_ex_info->fov_info.focal_lengths =
        in_sn_ex_info->fov_info.focal_lengths;

    return CMR_CAMERA_SUCCESS;
}

static cmr_uint camera_sensor_color_to_isp_color(cmr_u32 *isp_color,
                                                 cmr_u32 sensor_color) {
    switch (sensor_color) {
    case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
        *isp_color = COLOR_ORDER_GR;
        break;
    case SENSOR_IMAGE_PATTERN_RAWRGB_R:
        *isp_color = COLOR_ORDER_RG;
        break;
    case SENSOR_IMAGE_PATTERN_RAWRGB_B:
        *isp_color = COLOR_ORDER_BG;
        break;
    case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
        *isp_color = COLOR_ORDER_GB;
        break;
    default:
        CMR_LOGE("sensor color maybe error,not found in definition.set isp "
                 "color to COLOR_ORDER_GR");
        *isp_color = COLOR_ORDER_GR;
        break;
    }
    return CMR_CAMERA_SUCCESS;
}

cmr_int camera_preview_get_isp_yimg(cmr_handle oem_handle, cmr_u32 camera_id,
                                    struct isp_yimg_info *yimg) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle || NULL == yimg) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    isp_param.camera_id = camera_id;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_YIMG_INFO, &isp_param);
    if (ret) {
        CMR_LOGE("get isp y stat error %ld", ret);
        goto exit;
    }
    memcpy(yimg, &isp_param.isp_yimg, sizeof(struct isp_yimg_info));
    CMR_LOGI("%d", isp_param.isp_yimg.lock[0]);

exit:
    return ret;
}

cmr_int camera_preview_set_yimg_to_isp(cmr_handle oem_handle, cmr_u32 camera_id,
                                       struct yimg_info *yimg) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle || NULL == yimg) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    isp_param.camera_id = camera_id;
    isp_param.cmd_value = (cmr_uint)yimg;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_PREVIEW_YIMG, &isp_param);

exit:
    return ret;
}

cmr_int camera_preview_set_yuv_to_isp(cmr_handle oem_handle, cmr_u32 camera_id,
                                      struct yuv_info_t *yuv) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle || NULL == yuv) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    isp_param.camera_id = camera_id;
    isp_param.cmd_value = (cmr_uint)yuv;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_PREVIEW_YUV, &isp_param);

exit:
    return ret;
}

cmr_int cmr_sensor_init_static_info(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    struct sensor_ex_info *sns_ex_info_ptr = NULL;
    SENSOR_VAL_T val;
    CHECK_HANDLE_VALID(oem_handle);
    cxt = (struct camera_context *)oem_handle;
    sns_ex_info_ptr = &cxt->sn_cxt.cur_sns_ex_info;
    cmr_bzero(sns_ex_info_ptr, sizeof(struct sensor_ex_info));
    if (cxt->sn_cxt.inited && cxt->sn_cxt.sensor_handle) {
        val.type = SENSOR_VAL_TYPE_GET_STATIC_INFO;
        val.pval = sns_ex_info_ptr;
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                               SENSOR_ACCESS_VAL, (cmr_uint)&val);

    } else {
        ret = CMR_CAMERA_FAIL;
    }
    if (ret) {
        CMR_LOGE("oem not init or get sensor static info failed, we set "
                 "sn_ex_info to zero. ");
    }
    return ret;
}

cmr_int cmr_sensor_deinit_static_info(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    SENSOR_VAL_T val;
    CHECK_HANDLE_VALID(oem_handle);
    cxt = (struct camera_context *)oem_handle;
    cmr_bzero(&cxt->sn_cxt.cur_sns_ex_info, sizeof(struct sensor_ex_info));
    return ret;
}

cmr_int cmr_get_sensor_max_fps(cmr_handle oem_handle, cmr_u32 camera_id,
                               cmr_u32 *max_fps) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    struct sensor_ex_info *sns_ex_info_ptr = NULL;
    CHECK_HANDLE_VALID(oem_handle);
    CHECK_HANDLE_VALID(max_fps);
    cxt = (struct camera_context *)oem_handle;
    sns_ex_info_ptr = &cxt->sn_cxt.cur_sns_ex_info;
    if (NULL == sns_ex_info_ptr) {
        CMR_LOGE("sns_ex_info_ptr is null, It is impossible error!");
        return -CMR_CAMERA_INVALID_PARAM;
    }

    if ((NULL == sns_ex_info_ptr->name) ||
        (NULL == sns_ex_info_ptr->sensor_version_info)) {
        ret = cmr_sensor_init_static_info(cxt);
    }
    if (ret) {
        CMR_LOGE(
            "failed to init sensor static info,we set max fps to default: %d ",
            *max_fps);
        *max_fps = 30;
    } else {
        *max_fps = sns_ex_info_ptr->max_fps;
    }
    return ret;
}

cmr_int cmr_get_blur_covered_type(cmr_handle oem_handle, cmr_s32 *type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct common_isp_cmd_param isp_param;
    cmr_u32 bv;
    cmr_s32 isp_rt;

    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    bv = isp_param.isp_adgain.bv;
    CMR_LOGD("bv=%d", bv);

    // Just 4 bv test tmp
    char min_bv[PROPERTY_VALUE_MAX];
    char max_bv[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.minbv.test", min_bv, "500");
    property_get("persist.vendor.cam.maxbv.test", max_bv, "700");

    cmr_u32 Min_bv;
    cmr_u32 Max_bv;
    Min_bv = atoi(min_bv);
    Max_bv = atoi(max_bv);

    if (bv < Min_bv) {
        *type = BLUR_TIPS_NEED_LIGHT;
        CMR_LOGD("depth cannot work under this bv, (%d~%d), type=%d", Min_bv,
                 Max_bv, *type);
        return ret;
    }
    if (bv < Max_bv && BLUR_TIPS_NEED_LIGHT == *type) {
        CMR_LOGD("still need more light");
        return ret;
    }

    if (cxt->is_focus) {
        *type = BLUR_TIPS_UNABLED;
        CMR_LOGD("is focusing");
        return ret;
    }

    // sharkl5 TBD remove these code
    /*
        isp_rt = isp_sw_get_bokeh_status(isp_cxt->isp_handle);
        if (0 == isp_rt) {
            *type = BLUR_TIPS_OK;
        } else if (1 == isp_rt) {
            *type = BLUR_TIPS_FURTHER;
        } else if (2 == isp_rt) {
            *type = BLUR_TIPS_CLOSE;
        } else {
            *type = BLUR_TIPS_UNABLED;
        }
    */
    CMR_LOGD("type =%d", *type);

    return ret;
}

cmr_int cmr_set_zoom_factor_to_isp(cmr_handle oem_handle, float *zoomFactor) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    struct isp_context *isp_cxt = NULL;
    CHECK_HANDLE_VALID(oem_handle);

    cxt = (struct camera_context *)oem_handle;
    isp_cxt = &cxt->isp_cxt;
    if (NULL == isp_cxt->isp_handle || NULL == zoomFactor) {
        CMR_LOGE("isp handle or zoomFactor is null!");
        ret = -CMR_CAMERA_INVALID_PARAM;
        return ret;
    }
    ret = isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_SET_DZOOM_FACTOR, zoomFactor);
    if (ret) {
        CMR_LOGE("isp_ioctl-ISP_CTRL_SET_DZOOM_FACTOR failed:zoomFactor is %f",
                 *zoomFactor);
    }
    return ret;
}

cmr_int cmr_get_sensor_vcm_step(cmr_handle oem_handle, cmr_u32 camera_id,
                                cmr_u32 *vcm_step) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    isp_param.camera_id = cxt->camera_id;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_VCM_INFO, &isp_param);
    if (ret) {
        CMR_LOGE("get isp vcm step error %ld", ret);
        goto exit;
    }
    CMR_LOGD("isp_param.vcm_step = %d", isp_param.vcm_step);
    *vcm_step = isp_param.vcm_step;

exit:
    return ret;
}

cmr_int cmr_get_vcm_range(cmr_handle oem_handle, cmr_u32 camera_id,
                          struct vcm_range_info *vcm_range) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    isp_param.camera_id = cxt->camera_id;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CALIBRATION_VCMINFO,
                           &isp_param);
    if (ret) {
        CMR_LOGE("get isp vcm range error %ld", ret);
        goto exit;
    }

    memcpy(vcm_range, &isp_param.vcm_range, sizeof(struct vcm_range_info));
    CMR_LOGD("VCM_INFO:isp_param.range [%d, %d]", vcm_range->limited_infi,
             vcm_range->limited_macro);

exit:
    return ret;
}

cmr_int cmr_get_ae_fps_range(cmr_handle oem_handle, cmr_u32 camera_id,
                             struct ae_fps_range_info *ae_fps_range) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    isp_param.camera_id = cxt->camera_id;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_AE_FPS_RANGE, &isp_param);
    if (ret) {
        CMR_LOGE("get isp vcm range error %ld", ret);
        goto exit;
    }

    memcpy(ae_fps_range, &isp_param.ae_fps_range,
           sizeof(struct ae_fps_range_info));
    CMR_LOGV("AE_FPS_RANGE_INFO in DV mode:isp_param.range [%d, %d]",
             ae_fps_range->dv_fps_min, ae_fps_range->dv_fps_max);

exit:
    return ret;
}

cmr_int cmr_set_vcm_disc(cmr_handle oem_handle, cmr_u32 camera_id,
                         struct vcm_disc_info *vcm_disc) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    isp_param.camera_id = cxt->camera_id;
    CMR_LOGD("Total_seg %d disc[0] %d disc[3] %d", vcm_disc->total_seg,
             vcm_disc->distance[0], vcm_disc->distance[3]);
    memcpy(&isp_param.vcm_disc, vcm_disc, sizeof(struct vcm_disc_info));
    ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_CALIBRATION_VCMDISC,
                           &isp_param);
    if (ret) {
        CMR_LOGE("set isp vcm_disc error %ld", ret);
        goto exit;
    }

exit:
    return ret;
}

cmr_int cmr_get_reboke_data(cmr_handle oem_handle,
                            struct af_relbokeh_oem_data *golden_distance) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct common_isp_cmd_param isp_param;

    if (!oem_handle || golden_distance == NULL) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_REBOKEH_DATA, &isp_param);
    golden_distance->golden_count = isp_param.relbokeh_info.golden_count;
    golden_distance->golden_infinity = isp_param.relbokeh_info.golden_infinity;
    golden_distance->golden_macro = isp_param.relbokeh_info.golden_macro;
    for (int i = 0; i < golden_distance->golden_count; i++) {
        golden_distance->golden_distance[i] =
            isp_param.relbokeh_info.golden_distance[i];
        golden_distance->golden_vcm[i] = isp_param.relbokeh_info.golden_vcm[i];
        CMR_LOGD(" -------golden_vcm[%d] = %d ", i,
                 golden_distance->golden_vcm[i]);
    }
    if (ret) {
        CMR_LOGE("get isp vcm data error %ld", ret);
        goto exit;
    }

exit:
    return ret;
}

cmr_int camera_local_set_sensor_close_flag(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    SENSOR_VAL_T val;

    val.type = SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG;
    val.pval = NULL;
    ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                           SENSOR_ACCESS_VAL, (cmr_uint)&val);
    if (ret) {
        CMR_LOGE("failed to set sensor close flag");
    }
    return ret;
}

cmr_int camera_snapshot_set_ev(cmr_handle oem_handle, cmr_u32 value,
                               enum camera_snapshot_tpye type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    struct common_isp_cmd_param isp_param;

//    isp_param.ev_setting.cmd_value = value;
//    isp_param.ev_setting.snapshot_type = type;
    if (value != 0)
        cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                          SETTING_CLEAR_AE_NOTIFY, &setting_param);

    if (type == SNAPSHOT_NIGHT_DNS) {
        float ev[7];
        char ch_ev[PROPERTY_VALUE_MAX];

        //lock or unlock af
        isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_AF_BYPASS,
                  (void *)&value);
        //lock,unlock nr block by smart, used like hdr interface
        // lock with ev_adj start, but unlock later by then got all frame
        if (value) { //enable ev_adj
            isp_param.cmd_value = !!value;
            ret = camera_isp_ioctl(oem_handle, COM_ISP_LOCK_NR_SMART, (void *)&isp_param);
            if (ret)
                CMR_LOGW("Lock nr block fail");
        }

        property_get("persist.vendor.cam.night.b01.ev", ch_ev, "-4,-2,0,0.5,0.5,0.5,0.5");
        sscanf(ch_ev, "%f,%f,%f,%f,%f,%f,%f", &ev[0], &ev[1], &ev[2], &ev[3], &ev[4],&ev[5], &ev[6]);
        CMR_LOGD("ev_effect_valid_num %d", isp_param.snp_ae_param.ev_effect_valid_num);
        isp_param.snp_ae_param.ev_effect_valid_num =
            cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 1;
        isp_param.snp_ae_param.ev_adjust_count = 8;	//last is 0
        isp_param.snp_ae_param.ev_value[0] = ev[0];
        isp_param.snp_ae_param.ev_value[1] = ev[1];
        isp_param.snp_ae_param.ev_value[2] = ev[2];
        isp_param.snp_ae_param.ev_value[3] = ev[3];
        isp_param.snp_ae_param.ev_value[4] = ev[4];
        isp_param.snp_ae_param.ev_value[5] = ev[5];
        isp_param.snp_ae_param.ev_value[6] = ev[6];
        isp_param.snp_ae_param.ev_value[7] = 0.0;
    }
    ISP_LOGI("ae_adj: cnt %d (%f, %f, %f, %f, %f, %f, %f, %f)",
            isp_param.snp_ae_param.ev_adjust_count,
            isp_param.snp_ae_param.ev_value[0], isp_param.snp_ae_param.ev_value[1],
            isp_param.snp_ae_param.ev_value[2], isp_param.snp_ae_param.ev_value[3],
            isp_param.snp_ae_param.ev_value[4], isp_param.snp_ae_param.ev_value[5],
            isp_param.snp_ae_param.ev_value[6], isp_param.snp_ae_param.ev_value[7]);

    isp_param.snp_ae_param.type = type;
    isp_param.snp_ae_param.enable = value;
    ret =
        camera_isp_ioctl(oem_handle, COM_ISP_SET_AE_ADJUST, (void *)&isp_param);
    if (value != 0)
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_CTRL_AE_NOTIFY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to wait ev effect");
        goto exit;
    }
exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}

//lock awb and af
/* function: lock or unlock awb and af
 * param: af_lock: 0:unlock, 1:lock
 *        awb_lock: 0:unlock, 1:lock
 */
cmr_int camera_fdr_ctrl_3A (cmr_handle oem_handle, cmr_u32 lock) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_int prof_mode = 0;

    if(lock) {
       ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_STOP_3A, NULL);
       CMR_LOGD("fdr lock 3A");
	   if(ret) {
          CMR_LOGE("failed to lock 3A");
          goto exit;
	   }
	   prof_mode = 1;
	   ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_PROF_MODE, &prof_mode);
	   if(ret) {
		  CMR_LOGE("failed to prof mode set 1");
          goto exit;
	   }
    }else {
       prof_mode = 0;
	   ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_PROF_MODE, &prof_mode);
	   if(ret) {
		  CMR_LOGE("failed to prof mode set 0");
          goto exit;
	   }
       ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_START_3A, NULL);
       CMR_LOGD("fdr unlock 3A");
       if(ret) {
          CMR_LOGE("failed to unlock 3A");
          goto exit;
       }
    }

exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}


cmr_int camera_hdr_set_ev(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_sn_cmd_param sn_param;
    struct common_isp_cmd_param isp_param;
    struct setting_cmd_parameter setting_param;
    memset(&setting_param, 0, sizeof(setting_param));

    if (1 == camera_get_hdr_flag(cxt)) {
        ret = cmr_ipm_pre_proc(cxt->ipm_cxt.hdr_handle);
        if (ret) {
            CMR_LOGE("failed to ipm pre proc, %ld", ret);
            goto exit;
        }
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_CTRL_HDR, &setting_param);
        if (ret) {
            CMR_LOGE("failed to wait hdr ev effect");
            goto exit;
        }
    }

    if (cxt->is_vendor_hdr) {
        cxt->cap_cnt = 0;
        struct sensor_exp_info sensor_info;
        ret = camera_get_sensor_info(cxt, cxt->camera_id, &sensor_info);
        if (ret) {
            CMR_LOGE("get_sensor info failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info.image_format) {
            isp_param.cmd_value = OEM_EV_LEVEL_1;
            ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_HDR,
                                   (void *)&isp_param);
        } else {
            sn_param.cmd_value = OEM_EV_LEVEL_1;
            ret = camera_sensor_ioctl(oem_handle, COM_SN_SET_HDR_EV,
                                      (void *)&sn_param);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_fdr_get_frame_cnt(cmr_handle oem_handle, int *total_frame_num, int *ref_frame_num) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_adgain_exp_info adgain_exp_info = {0, 0, 0, 0};
#ifdef CONFIG_CAMERA_FDR
    fdr_calc_frame_status_t calc_status;
    fdr_calc_frame_param_in_t param_in;
    fdr_calc_frame_param_out_t param_out;

    cmr_bzero(&calc_status, sizeof(fdr_calc_frame_status_t));
    cmr_bzero(&param_out, sizeof(fdr_calc_frame_param_out_t));
    camera_get_adgain_exp_info(oem_handle, &adgain_exp_info);

    param_in.tuning_param = cxt->snp_cxt.fdr_tuning_param.param_ptr;
    param_in.BV = adgain_exp_info.bv;
    ret = sprd_fdr_get_frame_num(&param_in, &param_out, &calc_status);
    if(ret == CMR_CAMERA_FAIL) {
       CMR_LOGE("can not get ref frame");
       sprd_fdr_get_max_frame_num(total_frame_num, ref_frame_num);
    } else {
	    *total_frame_num = param_out.cur_total_frame_num;
	    *ref_frame_num = param_out.ref_frame_num;
    }
#endif
    CMR_LOGD("X, frame_cnt:%d, ref_frame_num:%d, tuning_param:0x%x, bv:%d",
             *total_frame_num, *ref_frame_num, cxt->snp_cxt.fdr_tuning_param.param_ptr, adgain_exp_info.bv);
    return ret;
}

cmr_int camera_fdr_set_ev(cmr_handle oem_handle, cmr_u32 fdr_enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    struct common_isp_cmd_param isp_param;

    CMR_LOGD("E, fdr ev=%d", fdr_enable);

    isp_param.cmd_value = fdr_enable;
    memset(&setting_param, 0, sizeof(setting_param));
    ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_FDR, (void *)&isp_param);
    CMR_LOGD("wait for fdr ev effect");
    if (fdr_enable != 0)
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_CTRL_FDR, &setting_param);
    if (ret) {
        CMR_LOGE("failed to wait fdr ev effect");
        goto exit;
    }
exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_get_fdr_enable(cmr_handle oem_handle, cmr_u32 *fdr_enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    *fdr_enable = camera_get_fdr_flag(cxt);
    CMR_LOGD("E, fdr enable=%d", *fdr_enable);

exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_3dnr_set_ev(cmr_handle oem_handle, cmr_u32 enable) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;
    struct sensor_exp_info sensor_info;
    ret = camera_get_sensor_info(cxt, cxt->camera_id, &sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info.image_format) {
        isp_param.cmd_value = enable;
        ret =
            camera_isp_ioctl(oem_handle, COM_ISP_SET_3DNR, (void *)&isp_param);
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_sw_3dnr_info_cfg(cmr_handle oem_handle,
                                struct sprd_img_3dnr_param *threednr_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGD("call camera_sw_3dnr_info_cfg, %d, %d", camera_get_3dnr_flag(cxt),
             camera_get_3dnr_flag(cxt));

    ret = cmr_grab_sw_3dnr_cfg(cxt->grab_cxt.grab_handle, threednr_info);
    return ret;
}

cmr_int camera_get_jpeg_param_info(cmr_handle oem_handle,
                                   struct jpeg_param *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;

    setting_param.camera_id = cxt->camera_id;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_JPEG_QUALITY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get image quality %ld", ret);
        goto exit;
    }
    param->quality = setting_param.cmd_type_value;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_THUMB_QUALITY, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get thumb quality %ld", ret);
        goto exit;
    }
    param->thumb_quality = setting_param.cmd_type_value;

    // thum_size is not use for now
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_THUMB_SIZE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get thumb size %ld", ret);
        goto exit;
    }
    param->thum_size = setting_param.size_param;

    CMR_LOGV("quality=%d, thumb_quality=%d, thum_size: %d %d", param->quality,
             param->thumb_quality, param->thum_size.width,
             param->thum_size.height);
exit:
    return ret;
}

cmr_int camera_local_start_capture(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;
    cmr_u32 flash_status = 0;
    struct sprd_img_capture_param capture_param;
    struct common_isp_cmd_param isp_param;

    struct sensor_exp_info sensor_info;

    ret = camera_get_sensor_info(cxt, cxt->camera_id, &sensor_info);

    cmr_bzero(&capture_param, sizeof(capture_param));
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    capture_param.type = DCAM_CAPTURE_START;
    camera_local_snapshot_is_need_flash(oem_handle, cxt->camera_id,
                                        &flash_status);
    if (1 == camera_get_fdr_flag(cxt)) {
        // num of continuous frames start from next sof interrupt
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = cxt->fdr_total_frame_cnt;
        capture_param.cap_scene = CAPTURE_FDR;
    } else if (1 == camera_get_hdr_flag(cxt)) {
        // 3 continuous frames start from next sof interrupt
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = 3;
        capture_param.cap_scene = CAPTURE_HDR;
    } else if (cxt->snp_cxt.is_mfsr) {
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = cxt->snp_cxt.total_num;
        capture_param.cap_scene = CAPTURE_COMMON;
    } else if ((CAMERA_3DNR_TYPE_PREV_HW_CAP_SW == camera_get_3dnr_flag(cxt))) {
        // 5 continuous frames start from next sof interrupt
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = 5;
        capture_param.cap_scene = CAPTURE_SW3DNR;
        capture_param.timestamp = snp_cxt->cap_need_time_stamp;
    } else if (CAMERA_3DNR_TYPE_NIGHT_DNS == camera_get_3dnr_flag(cxt)) {
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = 7;
        capture_param.cap_scene = CAPTURE_SW3DNR;
    } else if ((CAMERA_3DNR_TYPE_PREV_NULL_CAP_SW == camera_get_3dnr_flag(cxt))) {
        // 5 continuous frames start from next sof interrupt
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = 5;
        capture_param.cap_scene = CAPTURE_SW3DNR;
    } else if ((CAMERA_3DNR_TYPE_PREV_NULL_CAP_HW ==
                camera_get_3dnr_flag(cxt)) ||
               (CAMERA_3DNR_TYPE_PREV_HW_CAP_HW == camera_get_3dnr_flag(cxt)) ||
               (CAMERA_3DNR_TYPE_PREV_SW_CAP_SW == camera_get_3dnr_flag(cxt))) {
        // start hardware 3dnr capture
        CMR_LOGI("set cap_param type to DCAM_CAPTURE_START_3DNR");
        capture_param.type = DCAM_CAPTURE_START_3DNR;
        capture_param.cap_scene = CAPTURE_HW3DNR;
    } else if (cxt->is_multi_mode == MODE_BOKEH ||
               cxt->is_multi_mode == MODE_3D_CALIBRATION ||
               cxt->is_multi_mode == MODE_BOKEH_CALI_GOLDEN) {
        if (cxt->is_yuv_callback_mode || cxt->is_3dcalibration_mode) {
            capture_param.type = DCAM_CAPTURE_START_WITH_TIMESTAMP;
            capture_param.timestamp = snp_cxt->cap_need_time_stamp;
        } else {
            return ret;
        }
    } else if (cxt->skipframe == 1) {
        // need get 1 frame start from next sof interrupt
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = 1;
        CMR_LOGD("DRE or GTM case");
    } else if (flash_status > 0) {
        // need get 1 frame start from next sof interrupt
        capture_param.type = DCAM_CAPTURE_START_FROM_NEXT_SOF;
        capture_param.cap_cnt = 1;
        capture_param.cap_scene = CAPTURE_FLASH;
    }

    if (capture_param.type == DCAM_CAPTURE_START)
        capture_param.timestamp = snp_cxt->cap_need_time_stamp;
    CMR_LOGD(
        "type %d, cnt %d, scene %d,  dre_flag %d dre_skipframe %d, flash %d, cxt->flash_mode %d\n",
        capture_param.type, capture_param.cap_cnt, capture_param.cap_scene,
        cxt->dre_flag, cxt->skipframe, flash_status, cxt->flash_mode);

    isp_param.cmd_value = 1;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info.image_format)
        ret =
            camera_isp_ioctl(oem_handle, COM_ISP_SET_CAP_FLAG, (void *)&isp_param);

    ret = cmr_grab_start_capture(cxt->grab_cxt.grab_handle, capture_param);
    if (ret) {
        CMR_LOGE("cmr_grab_start_capture failed");
        goto exit;
    }
    cxt->is_capture_face = true;
    isp_param.cmd_value = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info.image_format)
        ret =
            camera_isp_ioctl(oem_handle, COM_ISP_SET_CAP_FLAG, (void *)&isp_param);

exit:
    return ret;
}

cmr_int camera_local_stop_capture(cmr_handle oem_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct common_isp_cmd_param isp_param;

    ret = cmr_grab_stop_capture(cxt->grab_cxt.grab_handle);
    if (ret) {
        CMR_LOGE("cmr_grab_stop_capture failed");
        goto exit;
    }
    cxt->snp_cxt.start_capture_flag = 0;
exit:
    return ret;
}
cmr_int camera_local_set_alloc_size(cmr_handle oem_handle, cmr_u16 width, cmr_u16 height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    ret = cmr_grab_set_alloc_size(cxt->grab_cxt.grab_handle, width , height);
    if (ret) {
        CMR_LOGE("cmr_grab_stop_capture failed");
        goto exit;
    }

exit:
    return ret;
}

void camera_set_oem_multimode(multiCameraMode camera_mode) {
    CMR_LOGD("camera_mode %d", camera_mode);
    is_multi_camera_mode_oem = camera_mode;
}

void camera_set_oem_masterid(uint8_t master_id) {
    CMR_LOGD("master id %d", master_id);
    master_id_oem = master_id;
}

cmr_int camera_local_set_ref_camera_id(cmr_handle oem_handle,
                                       cmr_u32 *ref_camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_cmd_parameter setting_param;
    cmr_u32 is_fdr_on = 0;
    struct setting_context *setting_cxt = &cxt->setting_cxt;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_FDR,
                             &setting_param);
    is_fdr_on = setting_param.cmd_type_value;

    CMR_LOGD("ref_camera_id id %d, camera_id:%d, old_ref_id:%d",
              *ref_camera_id, cxt->camera_id, cxt->ref_camera_id);
    CMR_LOGD("is_fdr_on %d, is_multi_camera_mode_oem:%d",
              is_fdr_on, is_multi_camera_mode_oem);

    if(is_fdr_on && is_multi_camera_mode_oem == MODE_MULTI_CAMERA &&
            cxt->ref_camera_id != *ref_camera_id) {
        if(*ref_camera_id == cxt->camera_id) {
            camera_get_fdr_tuning_param(cxt, NULL);
            //allocate fdr buffer for new ref camera id
            cmr_preview_realloc_buffer_for_fdr(cxt->prev_cxt.preview_handle,
                        cxt->camera_id, (cxt->swa_cxt_fdr.version == FDR_VERSION_1) ? 1 : 0);
        } else if(cxt->ref_camera_id == cxt->camera_id) {
            // free fdr buffer for old ref camera id
            cmr_preview_free_fdr_buffer(cxt->prev_cxt.preview_handle, cxt->camera_id);
        }
    }

    cxt->ref_camera_id = *ref_camera_id;
    // reset fdr flag
    camera_set_fdr_flag(cxt, is_fdr_on);
    ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AE_SET_REF_CAMERA_ID,
                     ref_camera_id);
    return ret;
}

cmr_int camera_local_set_visible_region(cmr_handle oem_handle,
                                        struct visible_region_info *info) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_mode_info *mode_info = NULL;
    struct img_rect cur_rect;
    struct img_size cur_size;
    struct img_rect dst_rect;
    float ratio = 0;
    cmr_u32 sn_mode = 0;

    cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id, &sn_mode);
    mode_info = &cxt->sn_cxt.sensor_info.mode_info[sn_mode];
    cur_size.width = mode_info->scaler_trim.width;
    cur_size.height = mode_info->scaler_trim.height;
    cur_rect.start_x = 0;
    cur_rect.start_y = 0;
    cur_rect.width = cur_size.width;
    cur_rect.height = cur_size.height;

    cmr_preview_get_prev_aspect_ratio(cxt->prev_cxt.preview_handle,
                                      cxt->camera_id, &ratio);
    dst_rect = camera_apply_rect_and_ratio(info->max_size, info->region,
                                           cur_rect, ratio);

    CMR_LOGV("src: size %u %u rect %u %u %u %u, dst: size %u %u rect %u %u %u %u, ratio: %f, camera_id %d",
            info->max_size.width, info->max_size.height,
            info->region.start_x, info->region.start_y,
            info->region.width, info->region.height,
            cur_size.width, cur_size.height,
            dst_rect.start_x, dst_rect.start_y,
            dst_rect.width, dst_rect.height, ratio, cxt->camera_id);

    info->max_size = cur_size;
    info->region = dst_rect;

    return isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AE_SET_VISIBLE_REGION,
                     info);
}

cmr_int camera_local_set_global_zoom_ratio(cmr_handle oem_handle,
                                           float *ratio) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    return isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AE_SET_GLOBAL_ZOOM_RATIO,
                     ratio);
}

cmr_int camera_set_eis_move_info(cmr_handle oem_handle,
                                           cmr_u8 *move_info) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    return isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AE_SET_EIS_MOVE_INFO,
                     move_info);
}

cmr_int camera_write_calibration_otp(cmr_handle oem_handle, struct cal_otp_info * param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGD("dual_otp_flag %d size %d", param->dual_otp_flag, param->otp_size);

    ret = cmr_sensor_write_calibration_otp(cxt->sn_cxt.sensor_handle, param->otp_data,
    param->dual_otp_flag, param->otp_size, cxt->camera_id);
    if (0 == ret) {
        param->cal_otp_result = CAMERA_CALIC_OTP_SUCCESS;
    } else {
        param->cal_otp_result = CAMERA_CALIC_OTP_FAIL;
    }
    return ret;
}

cmr_int camera_set_multicam_highres_mode(cmr_handle oem_handle , bool param) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->multicam_highres_mode = param;
    CMR_LOGD("multicam_highres_mode %d", cxt->multicam_highres_mode);
    return CMR_CAMERA_SUCCESS;
}

cmr_int camera_local_cap_state(cmr_handle oem_handle, bool *flag) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGD("*flag %d cxt->camera_id %d", *flag, cxt->camera_id);
    cmr_u32 channel_id = 0;
    cmr_u32 skip_number = 0;
    cmr_u32 deci_factor = 0;
    cmr_s32 frm_num = 0;
    cmr_u32 reconfig_flag = 0;
    if (*flag)
        ret = cmr_grab_cap_resume(cxt->grab_cxt.grab_handle, channel_id,
                                  skip_number, deci_factor, frm_num);
    else
        ret = cmr_grab_cap_pause(cxt->grab_cxt.grab_handle, channel_id,
                                 reconfig_flag);
    if (ret) {
        CMR_LOGE("failed to resume or pause ,ret %ld", ret);
    }
    return ret;
}

cmr_int camera_local_get_cover(cmr_handle oem_handle,
                               struct dual_sensor_luma_info *luma_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    SENSOR_VAL_T val;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_LOGD("E id=%d", cxt->camera_id);
    if (cxt->camera_id < 2) {

        struct camera_context *cxt = (struct camera_context *)oem_handle;
        struct isp_context *isp_cxt = &cxt->isp_cxt;
        struct common_isp_cmd_param isp_param;

        ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_ADGAIN_EXP,
                               &isp_param);
        luma_info->main_gain = isp_param.isp_adgain.adgain;
        cmr_u32 exp_time = isp_param.isp_adgain.exp_time;
        luma_info->main_luma = isp_param.isp_adgain.bv;
        CMR_LOGD("tadgain=%d, exp_time=%d, bv=%d", luma_info->main_gain,
                 exp_time, luma_info->main_luma);
    } else {
        val.type = SENSOR_VAL_TYPE_GET_BV;
        val.pval = (void *)&luma_info->sub_luma;
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                               SENSOR_ACCESS_VAL, (cmr_uint)&val);
    }
    CMR_LOGV("X");
    return ret;
}

cmr_int camera_stream_ctrl(cmr_handle oem_handle, cmr_u32 on_off) {
    cmr_int ret = 0;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct video_start_param param_ptr;

    memset(&param_ptr, 0, sizeof(struct video_start_param));
    CMR_LOGD("stream on/off(1/0) %u, multi_mode %d", on_off,
             cxt->is_multi_mode);
    if (cxt->camera_id != SENSOR_MAIN2) {
        CMR_LOGW("should not direct open sensors except for SENSOR_MAIN2");
    }

    if (on_off == 0) {
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                               SENSOR_STREAM_OFF, 0);
    } else {
        ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                               SENSOR_STREAM_ON, 0);
    }
    CMR_LOGD("done");
    return ret;
}

cmr_int cmr_get_isp_af_fullscan(cmr_handle oem_handle,
                                struct isp_af_fullscan_info *af_fullscan_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    void *isp_param_ptr = (void *)af_fullscan_info;

    CMR_LOGV("E");
    ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_FULLSCAN_INFO,
                    isp_param_ptr);

    return ret;
}

cmr_int cmr_set_af_pos(cmr_handle oem_handle, cmr_u32 af_pos) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_LOGI("E af_pos =%d", af_pos);

    ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_AF_POS,
                    (void *)&af_pos);

    return ret;
}

cmr_int cmr_set_3a_bypass(cmr_handle oem_handle, cmr_u32 value) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_LOGI("E af_pos =%d", value);
    ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_AF_BYPASS,
                    (void *)&value);
    if (value == 1) {
        cmr_u32 ae = 2;
        cmr_u32 awb = 3;
        ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AEAWB_BYPASS,
                        (void *)&ae);
        ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AEAWB_BYPASS,
                        (void *)&awb);
    } else {
        ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_AEAWB_BYPASS,
                        (void *)&value);
    }
    return ret;
}

cmr_int cmr_set_af_bypass(cmr_handle oem_handle, cmr_u32 value) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CMR_LOGI("af_bypass =%d", value);
    cxt->isp_cxt.is_af_bypass = value;
    ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_AF_BYPASS,
                    (void *)&value);
    return ret;
}

cmr_int cmr_get_ae_fps(cmr_handle oem_handle, cmr_u32 *ae_fps) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    void *isp_param_ptr = (void *)ae_fps;

    ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_FPS, isp_param_ptr);

    return ret;
}

cmr_int camera_local_get_tuning_param(cmr_handle oem_handle,
                                      struct tuning_param_info *tuning_info) {
    CMR_LOGV("E");

    cmr_int ret = CMR_CAMERA_SUCCESS;
    tuning_info->gain = 0;
    tuning_info->shutter = 0;
    tuning_info->bv = 0;
    tuning_info->pos = 0;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_adgain_exp_info adgain_exp_info;
    (struct isp_awb_info) tuning_info->awb_info;

    camera_get_tuning_info(oem_handle, &adgain_exp_info);

    tuning_info->gain = adgain_exp_info.adgain;
    tuning_info->shutter = adgain_exp_info.exp_time;
    tuning_info->bv = adgain_exp_info.bv;
    isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_AWB_GAIN,
              (void *)&(tuning_info->awb_info));
    isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_AF_POS,
              (void *)&(tuning_info->pos));

    CMR_LOGD("X");
    return ret;
}

cmr_int camera_get_fdr_tuning_param(cmr_handle oem_handle, struct isp_blkpm_t *tuning_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if(cxt->snp_cxt.fdr_tuning_param.param_ptr == NULL) {
        ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_GET_FDR_PARAM, &cxt->snp_cxt.fdr_tuning_param);
        if (ret) {
            CMR_LOGE("cam%d get fdr param failed", cxt->camera_id);
            cxt->snp_cxt.fdr_tuning_param.param_ptr = NULL;
            cxt->snp_cxt.fdr_tuning_param.param_size = 0;
        }
    }

    if (cxt->snp_cxt.fdr_tuning_param.param_ptr != NULL && tuning_param) {
       tuning_param->param_size = cxt->snp_cxt.fdr_tuning_param.param_size;
       tuning_param->param_ptr = cxt->snp_cxt.fdr_tuning_param.param_ptr;
    }

    CMR_LOGD("cam%d, fdr tuning param, size: %d, ptr:0x%x", cxt->camera_id,
                cxt->snp_cxt.fdr_tuning_param.param_size,
                cxt->snp_cxt.fdr_tuning_param.param_ptr);
    return ret;
}

cmr_int camera_get_fdr_tuning_flag(cmr_handle oem_handle, cmr_int *tuning_flag) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct isp_blkpm_t fdr_tuning_param;

    cmr_bzero(&fdr_tuning_param, sizeof(struct isp_blkpm_t));
    camera_get_fdr_tuning_param (oem_handle, &fdr_tuning_param);
    if (fdr_tuning_param.param_ptr != NULL && fdr_tuning_param.param_size > 0) {
        *tuning_flag = 1;
    } else {
        *tuning_flag = 0;
    }

    return ret;
}

cmr_int cmr_set_snapshot_timestamp(cmr_handle oem_handle, int64_t timestamp) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct snapshot_context *snp_cxt = &cxt->snp_cxt;

    snp_cxt->cap_need_time_stamp = timestamp;

    CMR_LOGD("timestamp=%" PRId64, snp_cxt->cap_need_time_stamp);

    return ret;
}

cmr_int cmr_get_microdepth_param(cmr_handle oem_handle, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    // sharkl5 TBD remove these code
    //    ret = isp_ioctl(cxt->isp_cxt.isp_handle,
    //    ISP_CTRL_GET_MICRODEPTH_PARAM,
    //                    param);

    return ret;
}

cmr_int cmr_set_microdepth_debug_info(cmr_handle oem_handle, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    // sharkl5 TBD remove these code

    //    ret = isp_ioctl(cxt->isp_cxt.isp_handle,
    //    ISP_CTRL_SET_MICRODEPTH_DEBUG_INFO,
    //                    param);
    return ret;
}

cmr_int camera_local_get_sensor_format(cmr_handle oem_handle,
                                       cmr_u32 *sensor_format) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CHECK_HANDLE_VALID(cxt);
    struct sensor_context *sn_cxt = NULL;
    sn_cxt = &(cxt->sn_cxt);
    CHECK_HANDLE_VALID(sn_cxt);

    ret = cmr_sensor_get_info(sn_cxt->sensor_handle, cxt->camera_id,
                              &(sn_cxt->sensor_info));
    if (ret)
        CMR_LOGE("fail to get sensor info ret %ld", ret);
    else
        *sensor_format = sn_cxt->sensor_info.image_format;
    CMR_LOGD("sensor_format is %d", *sensor_format);
    return ret;
}

cmr_int camera_local_set_capture_fb(cmr_handle oem_handle, cmr_u32 *on) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    CHECK_HANDLE_VALID(cxt);
    cxt->blur_facebeauty_flag = *on;
    CMR_LOGD("blur_facebeauty_flag %d", cxt->blur_facebeauty_flag);

    return ret;
}

cmr_int camera_set_multicam_reprocess_buffer(cmr_handle oem_handle,
                                             struct snapshot_param *param_ptr,
                                             cmr_uint src_phy_addr,
                                             cmr_uint src_vir_addr,
                                             cmr_s32 fd, cmr_u32 width, cmr_u32 height) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_u32 buffer_size = 0;
    struct img_frm img_frame;

    CMR_LOGD("E ");
    if (!oem_handle || !src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    buffer_size = width * height;
    memset(&img_frame, 0, sizeof(struct img_frm));
    img_frame.fmt = cxt->snp_cxt.cur_chn_data.fmt;
    img_frame.buf_size = buffer_size * 3 / 2;
    img_frame.rect.start_x = 0;
    img_frame.rect.start_y = 0;
    img_frame.rect.width = width;
    img_frame.rect.height = height;
    img_frame.size.width = width;
    img_frame.size.height = height;
    img_frame.addr_phy.addr_y = src_phy_addr;
    img_frame.addr_phy.addr_u = src_phy_addr + buffer_size;
    img_frame.addr_phy.addr_v = 0;
    img_frame.addr_vir.addr_y = src_vir_addr;
    img_frame.addr_vir.addr_u = src_vir_addr + buffer_size;
    img_frame.addr_vir.addr_v = 0;
    img_frame.data_end.y_endian = 1;
    img_frame.data_end.uv_endian = 2;
    img_frame.fd = fd;
    CMR_LOGD("multicam post proc src vir addr[0x%lx, 0x%lx], fd 0x%x",
                 img_frame.addr_vir.addr_y, img_frame.addr_vir.addr_u,
                 img_frame.fd);
    ret = cmr_snp_update_reprocess_param(cxt->snp_cxt.snapshot_handle, param_ptr,
                                       &img_frame);
exit:
    CMR_LOGD("X. ret = %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_local_reprocess_yuv_for_jpeg(cmr_handle oem_handle,
                                            enum takepicture_mode cap_mode,
                                            cmr_uint yaddr, cmr_uint yaddr_vir,
                                            cmr_uint fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct preview_context *prev_cxt;
    struct snapshot_param snp_param;
    struct common_sn_cmd_param param;
    struct setting_cmd_parameter setting_param;
    struct frm_info frame_info;
    struct frm_info *frm_data = &frame_info;
    cmr_uint buffer_size;
    cmr_int flash_status = FLASH_CLOSE;
    cmr_u32 sec = 0;
    cmr_u32 usec = 0;
    cmr_s32 sm_val = 0;
    cmr_u32 width = cxt->snp_cxt.request_size.width;
    cmr_u32 height = cxt->snp_cxt.request_size.height;

    camera_take_snapshot_step(CMR_STEP_TAKE_PIC);
    prev_cxt = &cxt->prev_cxt;

    sem_getvalue(&cxt->share_path_sm, &sm_val);
    if (0 != sm_val) {
        sem_destroy(&cxt->share_path_sm);
        sem_init(&cxt->share_path_sm, 0, 0);
        CMR_LOGI("re-initialize share_path_sm");
    }

    sem_wait(&cxt->access_sm);
    ret = cmr_grab_get_cap_time(cxt->grab_cxt.grab_handle, &sec, &usec);
    sem_post(&cxt->access_sm);

    if (cxt->multicam_highres_mode) {
        width /= 2;
        height /= 2;
    }

    cmr_bzero(frm_data, sizeof(struct frm_info));
    frm_data->channel_id = cxt->snp_cxt.channel_id;
    frm_data->frame_id = CMR_CAP0_ID_BASE;
    frm_data->frame_real_id = 0;
    frm_data->base = CMR_CAP0_ID_BASE;
    frm_data->height = height;
    frm_data->base = CMR_CAP0_ID_BASE;
    frm_data->fmt = CAM_IMG_FMT_YUV420_NV21;
    frm_data->yaddr = yaddr;
    frm_data->yaddr_vir = yaddr_vir;
    frm_data->fd = fd;
    frm_data->sec = sec;
    frm_data->usec = usec;

    CMR_LOGD("%dx%d, fd=0x%x, vir=0x%x", width, height, fd, yaddr_vir);
    buffer_size = width * height;
    frm_data->uaddr = yaddr + buffer_size;
    frm_data->vaddr = 0;
    frm_data->uaddr_vir = yaddr_vir + buffer_size;
    frm_data->vaddr_vir = 0;
    bzero(&snp_param, sizeof(struct snapshot_param));
    ret = camera_get_snapshot_param(oem_handle, &snp_param);

    // check snp size
    if (snp_param.post_proc_setting.snp_size.height == 0 ||
        snp_param.post_proc_setting.snp_size.width == 0 ||
        snp_param.post_proc_setting.actual_snp_size.height == 0 ||
        snp_param.post_proc_setting.actual_snp_size.width == 0) {
        cmr_bzero(&setting_param, sizeof(setting_param));
        setting_param.camera_id = cxt->camera_id;
        CMR_LOGI("camera id: %ld", setting_param.camera_id);

        /*get snapshot size*/
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                                SETTING_GET_CAPTURE_SIZE, &setting_param);
        if (ret) {
            CMR_LOGE("failed to get capture size %ld", ret);
            goto exit;
        }
        snp_param.post_proc_setting.actual_snp_size = setting_param.size_param;
        snp_param.post_proc_setting.snp_size = setting_param.size_param;
    }
    if (snp_param.channel_id == 0 || snp_param.channel_id >= GRAB_CHANNEL_MAX) {
        snp_param.channel_id = frm_data->channel_id;
    }
    CMR_LOGD("chn id: %d ,actual_snp_size: width:%d, height:%d", snp_param.channel_id,
             cxt->snp_cxt.post_proc_setting.actual_snp_size.width,
             cxt->snp_cxt.post_proc_setting.actual_snp_size.height);

    snp_param.post_proc_setting.chn_out_frm[0].addr_vir.addr_y =
        frm_data->yaddr_vir;
    snp_param.post_proc_setting.chn_out_frm[0].addr_vir.addr_u =
        frm_data->uaddr_vir;
    snp_param.post_proc_setting.chn_out_frm[0].addr_vir.addr_v =
        frm_data->vaddr_vir;
    snp_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_y =
        frm_data->yaddr;
    snp_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_u =
        frm_data->uaddr;
    snp_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_v =
        frm_data->vaddr;
    snp_param.post_proc_setting.chn_out_frm[0].fd = frm_data->fd;
    snp_param.post_proc_setting.chn_out_frm[0].fmt = frm_data->fmt;
    snp_param.post_proc_setting.chn_out_frm[0].size.width = width;
    snp_param.post_proc_setting.chn_out_frm[0].size.height = height;

    if(!cxt->multicam_highres_mode) {
      snp_param.post_proc_setting.mem[0].target_yuv.addr_vir.addr_y =
        frm_data->yaddr_vir;
      snp_param.post_proc_setting.mem[0].target_yuv.addr_vir.addr_u =
        frm_data->uaddr_vir;
      snp_param.post_proc_setting.mem[0].target_yuv.addr_vir.addr_v =
        frm_data->vaddr_vir;
      snp_param.post_proc_setting.mem[0].target_yuv.addr_phy.addr_y =
        frm_data->yaddr;
      snp_param.post_proc_setting.mem[0].target_yuv.addr_phy.addr_u =
        frm_data->uaddr;
      snp_param.post_proc_setting.mem[0].target_yuv.addr_phy.addr_v =
        frm_data->vaddr;
      snp_param.post_proc_setting.mem[0].target_yuv.fd = frm_data->fd;
      snp_param.post_proc_setting.mem[0].target_yuv.fmt = frm_data->fmt;
      snp_param.post_proc_setting.mem[0].target_yuv.size.width = width;
      snp_param.post_proc_setting.mem[0].target_yuv.size.height = height;
    }
    camera_set_snp_req((cmr_handle)cxt, TAKE_PICTURE_NEEDED);

    if (!cxt->multicam_highres_mode) {
        ret = cmr_snapshot_post_proc(cxt->snp_cxt.snapshot_handle, &snp_param);
        if (ret) {
            CMR_LOGE("snapshot_post_proc failed!");
        }
    } else {
        ret = camera_set_multicam_reprocess_buffer(oem_handle, &snp_param,
           yaddr, yaddr_vir, (cmr_s32)fd, width, height);
        if (ret) {
            CMR_LOGE("set_multicam_reprocess_buffer failed!");
        }
    }
    ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                    SNAPSHOT_EVT_POSTPROC_START, frm_data);

exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int camera_set_thumb_yuv_proc(cmr_handle oem_handle,
                                  struct snp_thumb_yuv_param *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGD("E");

    ret = cmr_snapshot_thumb_yuv_proc(cxt->snp_cxt.snapshot_handle, param);
    if (ret) {
        CMR_LOGE("snp_thumb_yuv_proc failed.");
    }

    return ret;
}

cmr_int camera_set_eis_warp_yuv_proc(cmr_handle oem_handle,
                                  struct eis_warp_yuv_param *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    ipm_param_t param_info;
    struct ipm_frame_in ipm_in_param;
    cmr_bzero(&param_info, sizeof(ipm_param_t));
    cmr_bzero(&ipm_in_param, sizeof(struct ipm_frame_in));
    CMR_LOGV("E");
    param_info.zoomRatio = 1.0;
    param_info.fullsize_width = param->src_img.rect.width;
    param_info.fullsize_height = param->src_img.rect.height;
    param_info.input_width = param->src_img.rect.width;
    param_info.input_height = param->src_img.rect.height;
    param_info.crop_x = param->src_img.rect.start_x;
    param_info.crop_y = param->src_img.rect.start_y;
    param_info.crop_width = param->src_img.rect.width;
    param_info.crop_height = param->src_img.rect.height;

    ipm_in_param.src_frame = param->src_img;
    ipm_in_param.src_frame.frame_number = param->src_img.frame_number;
    ipm_in_param.src_frame.reserved = param->src_img.reserved;
    ipm_in_param.dst_frame = param->dst_img;
    ipm_in_param.dst_frame.reserved = param->dst_img.reserved;
    ipm_in_param.private_data = (void *)&param_info;
    ipm_in_param.ae_exp_gain_info = (void *)&(param->eiswarp);

    if (NULL == cxt->ipm_cxt.warp_handle && cxt->prev_cxt.preview_eb) {
        struct ipm_open_in in_param;
        struct ipm_open_out out_param;
        cmr_bzero(&in_param, sizeof(struct ipm_open_in));
        cmr_bzero(&out_param, sizeof(struct ipm_open_out));
        in_param.frame_rect.start_x = param->src_img.rect.start_x;
        in_param.frame_rect.start_y = param->src_img.rect.start_y;
        in_param.frame_rect.width = param->src_img.rect.width;
        in_param.frame_rect.height = param->src_img.rect.height;
        in_param.sensor_size.width = param->src_img.rect.width;
        in_param.sensor_size.height = param->src_img.rect.height;
        in_param.binning_factor = 0;
        in_param.frame_size.width = param->src_img.rect.width;
        in_param.frame_size.height = param->src_img.rect.height;
        in_param.is_cap = false;
        in_param.adgain = 1;
        sem_wait(&cxt->eisWarp_sm);
        ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_ULTRA_WIDE, &in_param,
                           &out_param, &cxt->ipm_cxt.warp_handle);
        sem_post(&cxt->eisWarp_sm);
    }
    if (cxt->ipm_cxt.warp_handle != NULL && cxt->prev_cxt.preview_eb) {
        sem_wait(&cxt->eisWarp_sm);
        ret = ipm_transfer_frame(cxt->ipm_cxt.warp_handle, &ipm_in_param, NULL);
        sem_post(&cxt->eisWarp_sm);
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int camera_local_start_scale(cmr_handle oem_handle,
                                 struct img_frm **scale_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct img_frm *dst = NULL;
    struct img_frm *src = NULL;

    CMR_LOGV("E");
    if (!scale_param) {
        CMR_LOGI("invalid param,return");
        return CMR_CAMERA_INVALID_PARAM;
    }
    dst = scale_param[0];
    src = scale_param[1];
    if (!src || !dst) {
        CMR_LOGI("invalid param,return");
        return CMR_CAMERA_INVALID_PARAM;
    }
    ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst,
                          (cmr_evt_cb)NULL, NULL);
    if (ret) {
        CMR_LOGE("scale failed.");
    }

    CMR_LOGV("X ret=%d", ret);
    return ret;
}

cmr_int camera_local_start_rotate(cmr_handle oem_handle,
                                  struct rotate_param *rotate_param) {

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct cmr_op_mean mean;
    mean.rot = rotate_param->angle;

    ret = camera_start_rot(oem_handle, NULL, &(rotate_param->src_img),
                           &(rotate_param->dst_img), &mean);
    if (ret) {
        CMR_LOGE("failed to start start %ld", ret);
        ret = -CMR_CAMERA_FAIL;
    }

    return ret;
}

cmr_int camera_get_blur_covered_type(cmr_handle oem_handle, cmr_s32 *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGV("E");

    ret = cmr_get_blur_covered_type(cxt, param);

    if (ret) {
        CMR_LOGE("get blur covered type failed.");
    }

    return ret;
}

cmr_int camera_get_grab_capability(cmr_handle oem_handle,
                                   struct cmr_path_capability *capability) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    ret = cmr_grab_path_capability(cxt->grab_cxt.grab_handle, capability);

    return ret;
}

cmr_int camera_4in1_handle(cmr_int evt, void *data, void *privdata) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)privdata;
    struct frm_info *frame = (struct frm_info *)data;
    struct ipm_context *ipm_cxt = &cxt->ipm_cxt;
    struct img_frm src_param;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out imp_out_param;

    bzero(&src_param, sizeof(struct img_frm));
    bzero(&ipm_in_param, sizeof(struct ipm_frame_in));
    bzero(&imp_out_param, sizeof(struct ipm_frame_out));

    src_param.fd = frame->fd;
    src_param.fmt = frame->fmt;
    src_param.addr_vir.addr_y = frame->yaddr_vir;
    src_param.addr_phy.addr_y = frame->yaddr;
    src_param.addr_phy.addr_u = frame->uaddr;
    src_param.addr_phy.addr_v = frame->vaddr;
    src_param.size.height = frame->height;
    src_param.size.width = frame->length;
    src_param.monoboottime = frame->monoboottime;
    ipm_in_param.src_frame = src_param;
    ipm_in_param.private_data = privdata;
    cxt->snp_cxt.cur_frm_info = *frame;

    ret =
        ipm_transfer_frame(ipm_cxt->handle_4in1, &ipm_in_param, &imp_out_param);

    ATRACE_END();
    return ret;
}

cmr_int camera_open_4in1(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;

    if (!cxt) {
        CMR_LOGE("camera_open_4in1 cxt is null ");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (cxt->ipm_cxt.four_in_one_inited == 1) {
        CMR_LOGE("camera_open_4in1 has init ,go exit ");
        goto exit;
    }

    bzero(&in_param, sizeof(struct ipm_open_in));
    bzero(&out_param, sizeof(struct ipm_open_out));
    in_param.reg_cb = camera_ipm_cb;
    if (!cxt->ipm_cxt.handle_4in1) {
        ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_4IN1, &in_param,
                           &out_param, &cxt->ipm_cxt.handle_4in1);
        if (!ret) {
            cxt->ipm_cxt.four_in_one_inited = 1;
        } else {
            cxt->ipm_cxt.four_in_one_inited = 0;
        }
    }
exit:
    CMR_LOGD("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

static cmr_int camera_close_4in1(cmr_handle oem_handle) {
    ATRACE_BEGIN(__FUNCTION__);
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!cxt) {
        CMR_LOGE("camera_close_4in1 cxt is null");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (cxt->ipm_cxt.handle_4in1 && (1 == cxt->ipm_cxt.four_in_one_inited)) {
        ret = cmr_ipm_close(cxt->ipm_cxt.handle_4in1);
        cxt->ipm_cxt.handle_4in1 = NULL;
        cxt->ipm_cxt.four_in_one_inited = 0;
    }

exit:
    CMR_LOGV("X,ret=%ld", ret);
    ATRACE_END();
    return ret;
}

/* for solution2
 * is_4in1_sensor: 1: 4in1 senor(contain software remosaic, hardware remosai)
 *               0: normal sensor
 */
cmr_int camera_get_is_4in1_sensor(struct sensor_4in1_info *p) {
    cmr_int ret = 0;

    ret = (p->is_4in1_supported == 1) || (p->limited_4in1_width > 0);

    return ret;
}

/* oem use
 * input struct sensor_4in1_info *p, sensor_w,sensor_h:sensor output size
 * return: remosaic_type
 * 0:no need, 1:software, 2:hardware.
 * 0: default,or limited_4in1_width == 0 && limited_4in1_height == 0
 * 1: .is_4in1_supported == 1 && sensor current size > limited
 * 2: .sensor current size > limited
 */
cmr_int camera_get_remosaic_type(struct sensor_4in1_info *p, cmr_u32 sensor_w,
                                 cmr_u32 sensor_h) {
    cmr_int remosaic_type = 0;

    CMR_LOGD("[%d %d %d], {%d %d]", p->is_4in1_supported, p->limited_4in1_width,
             p->limited_4in1_height, sensor_w, sensor_h);
    if (p->limited_4in1_width == 0 || p->limited_4in1_height == 0)
        remosaic_type = 0;
    else if (p->is_4in1_supported && sensor_w > p->limited_4in1_width &&
             sensor_h > p->limited_4in1_height)
        remosaic_type = 1;
    else if (sensor_w > p->limited_4in1_width &&
             sensor_h > p->limited_4in1_height)
        remosaic_type = 2;

    CMR_LOGI("remosaic_type=%d", remosaic_type);

    return remosaic_type;
}

/* report some 4in1 info to hal
 *
 */
cmr_int camera_get_4in1_info(cmr_handle handle, struct fin1_info *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)handle;
    struct common_isp_cmd_param isp_param;

    if (!handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = camera_isp_ioctl(handle, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    if (ret) {
        goto exit;
    }
    cxt->ambient_highlight = isp_param.isp_adgain.ambient_highlight;

    param->remosaic_type = cxt->remosaic_type;
    param->ambient_highlight = cxt->ambient_highlight;

exit:
    CMR_LOGV("done ret = %d", ret);

    return ret;
}

cmr_int camera_set_high_res_mode(cmr_handle oem_handle,
                                 cmr_uint is_high_res_mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    cxt->is_high_res_mode = is_high_res_mode;
    CMR_LOGI("is_high_res_mode %ld", cxt->is_high_res_mode);

    return ret;
}

cmr_int camera_channel_reproc(cmr_handle oem_handle,
                              struct buffer_cfg *buf_cfg) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct img_frm img;
    char prop_value[PROPERTY_VALUE_MAX];

    struct camera_context *cxt = (struct camera_context *)oem_handle;
    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    cmr_bzero(&img, sizeof(struct img_frm));
    img.fd = buf_cfg->fd[0];
    img.addr_phy.addr_y = buf_cfg->addr[0].addr_y;
    img.addr_vir.addr_y = buf_cfg->addr_vir[0].addr_y;
    img.size.height = buf_cfg->slice_height;
    img.size.width = (buf_cfg->length * 4) / (buf_cfg->slice_height * 5);
    img.fmt = CAM_IMG_FMT_BAYER_MIPI_RAW;

    cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle, &img);

    ret = cmr_grab_buff_reproc(cxt->grab_cxt.grab_handle, buf_cfg);
    if (ret) {
        CMR_LOGE("failed to buf cfg %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_uint camera_hal_ai_scene_type(cmr_u8 isp_scene_type) {
    cmr_u8 hal_scene_type = HAL_AI_SCENE_MAX;
    switch (isp_scene_type) {
    case ISP_AI_SCENE_DEFAULT:
        hal_scene_type = HAL_AI_SCENE_DEFAULT;
        break;
    case ISP_AI_SCENE_FOOD:
        hal_scene_type = HAL_AI_SCENE_FOOD;
        break;
    case ISP_AI_SCENE_PORTRAIT:
        hal_scene_type = HAL_AI_SCENE_PORTRAIT;
        break;
    case ISP_AI_SCENE_FOLIAGE:
        hal_scene_type = HAL_AI_SCENE_FOLIAGE;
        break;
    case ISP_AI_SCENE_SKY:
        hal_scene_type = HAL_AI_SCENE_SKY;
        break;
    case ISP_AI_SCENE_NIGHT:
        hal_scene_type = HAL_AI_SCENE_NIGHT;
        break;
    case ISP_AI_SCENE_BACKLIGHT:
        hal_scene_type = HAL_AI_SCENE_BACKLIGHT;
        break;
    case ISP_AI_SCENE_TEXT:
        hal_scene_type = HAL_AI_SCENE_TEXT;
        break;
    case ISP_AI_SCENE_SUNRISE:
        hal_scene_type = HAL_AI_SCENE_SUNRISE;
        break;
    case ISP_AI_SCENE_BUILDING:
        hal_scene_type = HAL_AI_SCENE_BUILDING;
        break;
    case ISP_AI_SCENE_LANDSCAPE:
        hal_scene_type = HAL_AI_SCENE_LANDSCAPE;
        break;
    case ISP_AI_SCENE_SNOW:
        hal_scene_type = HAL_AI_SCENE_SNOW;
        break;
    case ISP_AI_SCENE_FIREWORK:
        hal_scene_type = HAL_AI_SCENE_FIREWORK;
        break;
    case ISP_AI_SCENE_BEACH:
        hal_scene_type = HAL_AI_SCENE_BEACH;
        break;
    case ISP_AI_SCENE_PET:
        hal_scene_type = HAL_AI_SCENE_PET;
        break;
    case ISP_AI_SCENE_FLOWER:
        hal_scene_type = HAL_AI_SCENE_FLOWER;
        break;
    case ISP_AI_SCENE_MUlTI_PORTRAIT:
        hal_scene_type = HAL_AI_SCENE_MUlTI_PORTRAIT;
        break;
    default:
        CMR_LOGE("invalid isp ai scene type:%u", isp_scene_type);
        break;
    }

    return hal_scene_type;
}

cmr_int camera_set_hdr_ev(cmr_handle oem_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!data || !cxt) {
        CMR_LOGI("don't need set hdr ev");
        return ret;
    }
    memcpy((void *)&cxt->snp_cxt.hdr_ev[0], data,
           ((HDR_CAP_NUM)-1) * sizeof(float));
    CMR_LOGD("hdr ev %f, %f", cxt->snp_cxt.hdr_ev[0], cxt->snp_cxt.hdr_ev[1]);
    return ret;
}

cmr_int camera_local_image_sw_algorithm_processing(
    cmr_handle oem_handle, struct image_sw_algorithm_buf *src_sw_algorithm_buf,
    struct image_sw_algorithm_buf *dst_sw_algorithm_buf,
    sprd_cam_image_sw_algorithm_type_t sw_algorithm_type,
    cam_img_format_t format) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_exp_info exp_info;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out imp_out_param;
    struct ipm_context *ipm_cxt = &cxt->ipm_cxt;

    ipm_in_param.private_data = (void *)cxt;
    ipm_in_param.src_frame.size.width = src_sw_algorithm_buf->width;
    ipm_in_param.src_frame.size.height = src_sw_algorithm_buf->height;
    ipm_in_param.src_frame.fd = src_sw_algorithm_buf->fd;
    ipm_in_param.src_frame.fmt = src_sw_algorithm_buf->format;
    ipm_in_param.src_frame.buf_size = ipm_in_param.src_frame.size.width *
                                      ipm_in_param.src_frame.size.height * 3 /
                                      2;
    ipm_in_param.src_frame.addr_vir.addr_y = src_sw_algorithm_buf->y_vir_addr;
    ipm_in_param.src_frame.addr_vir.addr_u =
        src_sw_algorithm_buf->y_vir_addr +
        ipm_in_param.src_frame.size.width * ipm_in_param.src_frame.size.height;
    ipm_in_param.src_frame.addr_phy.addr_y = src_sw_algorithm_buf->y_phy_addr;
    imp_out_param.private_data = src_sw_algorithm_buf->reserved;

    ipm_in_param.dst_frame.size.width = dst_sw_algorithm_buf->width;
    ipm_in_param.dst_frame.size.height = dst_sw_algorithm_buf->height;
    ipm_in_param.dst_frame.fd = dst_sw_algorithm_buf->fd;
    ipm_in_param.dst_frame.fmt = dst_sw_algorithm_buf->format;
    ipm_in_param.dst_frame.buf_size = ipm_in_param.dst_frame.size.width *
                                      ipm_in_param.dst_frame.size.height * 3 /
                                      2;
    ipm_in_param.dst_frame.addr_vir.addr_y = dst_sw_algorithm_buf->y_vir_addr;
    ipm_in_param.dst_frame.addr_vir.addr_u =
        dst_sw_algorithm_buf->y_vir_addr +
        ipm_in_param.dst_frame.size.width * ipm_in_param.dst_frame.size.height;
    ipm_in_param.dst_frame.addr_phy.addr_y = dst_sw_algorithm_buf->y_phy_addr;

    if (!oem_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    cmr_snapshot_invalidate_cache(cxt->snp_cxt.snapshot_handle,
                                  &ipm_in_param.src_frame);
    CMR_LOGD("sw type %d, is ultra %d\n", sw_algorithm_type, cxt->is_ultra_wide);
    if (sw_algorithm_type == SPRD_CAM_IMAGE_SW_ALGORITHM_3DNR) {
        if (cxt->is_ultra_wide) {
            void *ghandle = NULL;
            ret = cmr_preview_get_ultra_wide_handle(cxt->prev_cxt.preview_handle,
                cxt->camera_id, src_sw_algorithm_buf->fd, &ghandle);
            if (!ret)
                imp_out_param.private_data = ghandle;
        }
        ret = ipm_transfer_frame(ipm_cxt->threednr_handle, &ipm_in_param,
                                 &imp_out_param);
    } else if (sw_algorithm_type == SPRD_CAM_IMAGE_SW_ALGORITHM_HDR) {
        if (cxt->ipm_cxt.hdr_version.major != 1) {
            memcpy(&ipm_in_param.ev[0], &cxt->snp_cxt.hdr_ev[0],
                   ((HDR_CAP_NUM)-1) * sizeof(float));
            ipm_in_param.hdr_callback = cxt->snp_cxt.hdr_callback;
            ipm_in_param.ae_exp_gain_info = cxt->snp_cxt.ae_exp_gain_info;
            CMR_LOGD("ev: %f, %f, %p, %p", ipm_in_param.ev[0],
             ipm_in_param.ev[1], ipm_in_param.ae_exp_gain_info, ipm_in_param.hdr_callback);
        }
        ret = ipm_transfer_frame(ipm_cxt->hdr_handle, &ipm_in_param,
                                 &imp_out_param);
    } else if (sw_algorithm_type == SPRD_CAM_IMAGE_SW_ALGORITHM_NIGHT &&
               cxt->night_cxt.is_authorized) {
        ret = cxt->night_cxt.sw_process(oem_handle, src_sw_algorithm_buf,
                        dst_sw_algorithm_buf);
    } else if (sw_algorithm_type == SPRD_CAM_IMAGE_SW_ALGORITHM_CNR_YNR) {

        cxt->nr_flag = camera_get_cnr_realtime_flag(cxt);
        if (cxt->nr_flag) {
			// do cnr
            if (cxt->nr_flag)
                ret = ipm_transfer_frame(ipm_cxt->cnr_handle, &ipm_in_param, NULL);
            if (ret) {
                CMR_LOGE("failed to do cnr process %ld", ret);
                goto exit;
            }
        }else{
            ret=CMR_CAMERA_FAIL;
            CMR_LOGD("not need to nr");
        }
    } else if (sw_algorithm_type == SPRD_CAM_IMAGE_SW_ALGORITHM_NIGHT_DNS) {
        ret = ipm_transfer_frame(ipm_cxt->threednr_handle, &ipm_in_param,
                                 &imp_out_param);
    } else {
        CMR_LOGV("sw_type %d", sw_algorithm_type);
    }

exit:
    return ret;
}

cmr_int camera_get_bv_info(cmr_handle oem_handle, cmr_u32 *bv_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct common_isp_cmd_param isp_param;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_ADGAIN_EXP, &isp_param);
    if (ret) {
        CMR_LOGE("failed to get isp param %d", ret);
        return ret;
    }
    *bv_info = isp_param.isp_adgain.bv;
    return ret;
}

cmr_int camera_get_ct_info(cmr_handle oem_handle, cmr_u32 *ct_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct common_isp_cmd_param isp_param;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_COL_TEM, &isp_param);
    if (ret) {
        CMR_LOGE("failed to get isp param %d", ret);
        return ret;
    }
    *ct_info = isp_param.isp_cur_ct;
    return ret;
}

cmr_int camera_get_iso_info(cmr_handle oem_handle, cmr_u32 *iso_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct common_isp_cmd_param isp_param;
    ret = camera_isp_ioctl(oem_handle, COM_ISP_GET_CUR_SENS, &isp_param);
    if (ret) {
        CMR_LOGE("failed to get isp param %d", ret);
        return ret;
    }
    *iso_info = isp_param.isp_cur_iso;
    return ret;
}

struct awbc_cfg {
    cmr_u32 r_gain;
    cmr_u32 g_gain;
    cmr_u32 b_gain;
    cmr_u32 r_offset;
    cmr_u32 g_offset;
    cmr_u32 b_offset;
};
extern cmr_u32 isp_cur_bv;
extern cmr_u32 isp_cur_ct;
/* dump image with 3a info */
int dump_image_with_3a_info(cmr_handle oem_handle, uint32_t img_fmt,
                            uint32_t width, uint32_t height, uint32_t dump_size,
                            struct img_addr *addr) {
    int ret = CMR_CAMERA_SUCCESS;
    char file_name[FILE_NAME_LEN] = {0};
    char tmp_str[20] = {0};
    FILE *fp = NULL;
    uint32_t size = 0;
    uint32_t gain = 0;
    uint32_t shutter = 0;
    int32_t bv = 0;
    uint32_t pos = 0;
    cmr_u32 glb_gain = 0;
    struct isp_adgain_exp_info adgain_exp_info;
    struct awbc_cfg awbc;
    char datetime[18];

    cmr_bzero(&awbc, sizeof(awbc));
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    if (isp_cxt->isp_handle == NULL) {
        CMR_LOGE("isp handle null!!");
        ret = CMR_CAMERA_FAIL;
        return -1;
    }

    if (isp_video_get_simulation_flag()) {
        ret = isp_sim_get_mipi_raw_file_name(file_name);
    } else {

        // get millisecond of the time
        struct timeval tv;
        gettimeofday(&tv, NULL);

        time_t timep;
        struct tm *p;
        time(&timep);
        p = localtime(&timep);
        sprintf(datetime, "%04d%02d%02d%02d%02d%02d%03d", (1900 + p->tm_year),
                (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
                tv.tv_usec / 1000);

        camera_get_tuning_info(oem_handle, &adgain_exp_info);
        gain = adgain_exp_info.adgain;
        shutter = adgain_exp_info.exp_time;
        bv = adgain_exp_info.bv;

        isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_GET_AWB_GAIN, (void *)&awbc);
        isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_GET_GLB_GAIN,
                  (void *)&glb_gain);
        isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_GET_AWB_CT,
                  (void *)&isp_cur_ct);
        isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_GET_AF_POS, (void *)&pos);

        strcpy(file_name, CAMERA_DUMP_PATH);
        sprintf(tmp_str, "%d", width);
        strcat(file_name, tmp_str);
        strcat(file_name, "X");
        sprintf(tmp_str, "%d", height);
        strcat(file_name, tmp_str);

        strcat(file_name, "_");
        sprintf(tmp_str, "%s", datetime);
        strcat(file_name, tmp_str);

        strcat(file_name, "_");
        strcat(file_name, "gain");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", gain);
        strcat(file_name, tmp_str);

        strcat(file_name, "_");
        strcat(file_name, "ispdgain");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", glb_gain);
        strcat(file_name, tmp_str);

        strcat(file_name, "_");
        strcat(file_name, "shutter");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", shutter);
        strcat(file_name, tmp_str);

        strcat(file_name, "_");
        strcat(file_name, "awbgain");
        strcat(file_name, "_");
        strcat(file_name, "r");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", awbc.r_gain);
        strcat(file_name, tmp_str);
        strcat(file_name, "_");
        strcat(file_name, "g");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", awbc.g_gain);
        strcat(file_name, tmp_str);
        strcat(file_name, "_");
        strcat(file_name, "b");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", awbc.b_gain);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        strcat(file_name, "_");
        strcat(file_name, "afpos");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", pos);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        strcat(file_name, "_");
        strcat(file_name, "ct");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", isp_cur_ct);
        strcat(file_name, tmp_str);

        memset(tmp_str, 0, sizeof(tmp_str));
        strcat(file_name, "_");
        strcat(file_name, "bv");
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", isp_cur_bv);
        strcat(file_name, tmp_str);
    }

    if (img_fmt == CAM_IMG_FMT_BAYER_MIPI_RAW) {
        strcat(file_name, ".mipi_raw");
        size = dump_size;
    } else if (img_fmt == REMOSAIC_CAM_IMG_FMT_BAYER_MIPI_RAW) {
        strcat(file_name, "_remosaic.mipi_raw");
        size = dump_size;
    } else if (img_fmt == REMOSAIC_CAM_IMG_FMT_RAW14BIT) {
        strcat(file_name, "_remosaic.raw");
        size = dump_size;
    } else if (img_fmt == CAM_IMG_FMT_BAYER_SPRD_DCAM_RAW) {
        strcat(file_name, "_dcam.mipi_raw");
        size = dump_size;
    } else if (img_fmt == CAM_IMG_FMT_RAW14BIT) {
        strcat(file_name, ".raw");
        size = width * height * 2;
    } else if (img_fmt == CAM_IMG_FMT_YUV420_NV21) {
        strcat(file_name, ".yuv");
        size = width * height * 3 / 2;
    } else if (img_fmt == CAM_IMG_FMT_JPEG) {
        strcat(file_name, ".jpg");
        size = dump_size;
    } else if (img_fmt == CAM_IMG_FMT_DCAM_RAW14BIT) {
        strcat(file_name, "_dcam.raw");
        size = width * height * 2;
    }

    CMR_LOGD("file name %s", file_name);
    fp = fopen(file_name, "wb");
    if (NULL == fp) {
        CMR_LOGE("can not open file: %s errno = %d", file_name, errno);
        return -1;
    }

    fwrite((void *)addr->addr_y, 1, size, fp);
    fclose(fp);

    return 0;
}

cmr_int cmr_get_bokeh_sn_trim(cmr_handle handle,
                              struct sprd_img_path_rect *trim_param) {
    struct camera_context *cxt = (struct camera_context *)handle;
    cmr_int ret = CMR_CAMERA_SUCCESS;
#ifndef CONFIG_CAMERA_FACE_ROI
    ret = cmr_grab_get_dcam_path_trim(cxt->grab_cxt.grab_handle, trim_param);
#else
    cmr_u32 sn_mode = 0;
    cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id, &sn_mode);
    struct sensor_mode_info *sensor_mode_info = NULL;
    sensor_mode_info = &cxt->sn_cxt.sensor_info.mode_info[sn_mode];
    trim_param->trim_valid_rect.w = sensor_mode_info->trim_width;
    trim_param->trim_valid_rect.h = sensor_mode_info->trim_height;
#endif

    return ret;
}

void camera_local_set_original_picture_size(cmr_handle oem_handle , int32_t width , int32_t height ){
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    setting_param.originalPictureSize.width = width;
    setting_param.originalPictureSize.height = height;
    setting_param.camera_id = cxt->camera_id;
    cmr_setting_ioctl(setting_cxt->setting_handle,SETTING_SET_ORIGINAL_PICTURE_SIZE, &setting_param);
}

cmr_int camera_set_ae_params(cmr_handle oem_handle, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct ae_params *temp_ae_params = (struct ae_params *)param;
    struct tmp_cts_ae_params tmp_cts_ae;
    tmp_cts_ae.ae_mode = temp_ae_params->ae_mode;
    tmp_cts_ae.exp_time = temp_ae_params->exp_time;
    tmp_cts_ae.frame_number = temp_ae_params->frame_number;
    tmp_cts_ae.sensitivity = temp_ae_params->sensitivity;
    cxt->exp_time = tmp_cts_ae.exp_time = temp_ae_params->exp_time;
    if (cxt->isp_cxt.is_work) {
        struct isp_context *isp_cxt = &cxt->isp_cxt;
        ret = isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_SET_AE_PARAMS, (void *)&tmp_cts_ae);
        CMR_LOGD("exp_time:%lld, sensitivity:%d, frame_number:%d", temp_ae_params->exp_time, temp_ae_params->sensitivity, temp_ae_params->frame_number);
    }
    return ret;
}

cmr_int camera_set_af_params(cmr_handle oem_handle, void *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct af_params *temp_af_params = (struct af_params *)param;
    if (cxt->isp_cxt.is_work) {
        struct isp_context *isp_cxt = &cxt->isp_cxt;
        isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_SET_FOCUS_DISTANCE, (void *)temp_af_params);
        CMR_LOGD("focus_distance:%f, frame_number:%d", temp_af_params->focus_distance, temp_af_params->frame_number);
    }
    return ret;
}

void camera_local_set_exif_iso_value(cmr_handle oem_handle, cmr_u32 iso_value) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGD("exif iso_value:%d", iso_value);
    cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                        SENSOR_EXIF_CTRL_ISOSPEEDRATINGS,
                        iso_value);
    cmr_u32 white_balance = 1;
    cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                        SENSOR_EXIF_CTRL_WHITEBALANCE,
                        white_balance);
}

void camera_local_set_exif_exp_time(cmr_handle oem_handle, cmr_s64 exp_time) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    CMR_LOGD("exif exp_time:%lld", exp_time);
    cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id,
                        SENSOR_EXIF_CTRL_EXPOSURETIME_BYTIME,
                        exp_time);
}

cmr_s64 camera_local_get_shutter_skew(cmr_handle oem_handle) {
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct sensor_context *sn_cxt = &cxt->sn_cxt;
    struct preview_context *prev_cxt = &cxt->prev_cxt;
    cmr_uint sensor_work_mode;
    cmr_s64 rolling_shutter_skew;
    cmr_u8 camerId = cxt->camera_id;
    sensor_work_mode = cmr_preview_get_sn_work_mode(cxt->prev_cxt.preview_handle, camerId);
    rolling_shutter_skew = cmr_sensor_get_shutter_skew(sn_cxt->sensor_handle, sensor_work_mode, camerId);
    CMR_LOGD("camera id:%d work mode:%d rolling shutter time:%lld",
             camerId, sensor_work_mode, rolling_shutter_skew);
    return rolling_shutter_skew;
}

