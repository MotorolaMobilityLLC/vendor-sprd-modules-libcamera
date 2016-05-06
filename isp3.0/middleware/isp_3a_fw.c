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
#define LOG_TAG "isp_3afw"

#include <stdlib.h>
#include "cutils/properties.h"
#include <unistd.h>
#include "isp_3a_fw.h"
#include <pthread.h>
#include "awb_ctrl.h"
#include "af_ctrl.h"
#include "ae_ctrl.h"
#include "afl_ctrl.h"
#include "isp_common_types.h"
#include "cmr_msg.h"
#include "cmr_sensor_info.h"
#include "isp_dev_access.h"
#include "alwrapper_3a.h"
#include "isp_3a_adpt.h"
#include "allib_3a_ver.h"
#include "allib_3a_ver_errcode.h"

/**************************************** MACRO DEFINE *****************************************/
#define ISP3A_MSG_QUEUE_SIZE                                         100
#define ISP3A_STATISTICS_BUF_NUM                                     8
#define ISP3A_STATISTICS_AFL_BUF_NUM                                 16
#define ISP3A_TURNOFF_FLASH_SKIP_NUM                                 2;

#define ISP3A_EVT_BASE                                               (1<<27)
#define ISP3A_CTRL_EVT_INIT                                          ISP3A_EVT_BASE
#define ISP3A_CTRL_EVT_DEINIT                                        (ISP3A_EVT_BASE+1)
#define ISP3A_CTRL_EVT_IOCTRL                                        (ISP3A_EVT_BASE+2)
#define ISP3A_CTRL_EVT_PROCESS                                       (ISP3A_EVT_BASE+3)
#define ISP3A_CTRL_EVT_EXIT                                          (ISP3A_EVT_BASE+4)
#define ISP3A_CTRL_EVT_START                                         (ISP3A_EVT_BASE+5)
#define ISP3A_CTRL_EVT_STOP                                          (ISP3A_EVT_BASE+6)

#define ISP3A_PROC_EVT_INIT                                          (ISP3A_EVT_BASE+20)
#define ISP3A_PROC_EVT_DEINIT                                        (ISP3A_EVT_BASE+21)
#define ISP3A_PROC_EVT_IOCTRL                                        (ISP3A_EVT_BASE+22)
#define ISP3A_PROC_EVT_PROCESS                                       (ISP3A_EVT_BASE+23)
#define ISP3A_PROC_EVT_EXIT                                          (ISP3A_EVT_BASE+24)
#define ISP3A_PROC_EVT_STATS                                         (ISP3A_EVT_BASE+25)
#define ISP3A_PROC_EVT_SENSOR_SOF                                    (ISP3A_EVT_BASE+26)
#define ISP3A_PROC_EVT_START                                         (ISP3A_EVT_BASE+27)
#define ISP3A_PROC_EVT_STOP                                          (ISP3A_EVT_BASE+28)


#define ISP3A_RECEIVER_EVT_INIT                                      (ISP3A_EVT_BASE+40)
#define ISP3A_RECEIVER_EVT_DEINIT                                    (ISP3A_EVT_BASE+41)
#define ISP3A_RECEIVER_EVT_IOCTRL                                    (ISP3A_EVT_BASE+42)
#define ISP3A_RECEIVER_EVT_PROCESS                                   (ISP3A_EVT_BASE+43)
#define ISP3A_RECEIVER_EVT_EXIT                                      (ISP3A_EVT_BASE+44)


#define ISP3A_AF_EVT_INIT                                            (ISP3A_EVT_BASE+50)
#define ISP3A_AF_EVT_DEINIT                                          (ISP3A_EVT_BASE+51)
#define ISP3A_AF_EVT_IOCTRL                                          (ISP3A_EVT_BASE+52)
#define ISP3A_AF_EVT_PROCESS                                         (ISP3A_EVT_BASE+53)
#define ISP3A_AF_EVT_EXIT                                            (ISP3A_EVT_BASE+54)

#define ISP3A_AFL_EVT_INIT                                           (ISP3A_EVT_BASE+50)
#define ISP3A_AFL_EVT_DEINIT                                         (ISP3A_EVT_BASE+51)
#define ISP3A_AFL_EVT_IOCTRL                                         (ISP3A_EVT_BASE+52)
#define ISP3A_AFL_EVT_PROCESS                                        (ISP3A_EVT_BASE+53)
#define ISP3A_AFL_EVT_EXIT                                           (ISP3A_EVT_BASE+54)
/************************************* INTERNAL DATA TYPE ***************************************/
typedef cmr_int ( *isp3a_ioctrl_fun)(cmr_handle isp_3a_handle, void *param_ptr);

struct isp3a_ctrl_io_func {
	cmr_u32 cmd;
	isp3a_ioctrl_fun ioctrl;
};

struct isp3a_thread_context {
	cmr_handle ctrl_thr_handle;
	cmr_handle process_thr_handle;//for processing unpack statics & AE & AWB
	cmr_handle af_thr_handle;
	cmr_handle afl_thr_handle;
	pthread_t receiver_thr_handle;
};

struct awb_info {
	cmr_handle handle;
	cmr_int wb_mode;
	cmr_u32 cur_ct;
	cmr_u32 skip_num;
	cmr_u32 awb_status;
	struct isp_awb_gain cur_gain;
	struct isp3a_awb_hw_cfg hw_cfg;
	struct awb_ctrl_process_out proc_out;
	struct isp3a_statistics_data statistics_buffer[ISP3A_STATISTICS_BUF_NUM];
};

struct ae_info {
	cmr_handle handle;
	struct isp3a_ae_hw_cfg hw_cfg;
	struct ae_ctrl_proc_out proc_out;
	struct isp3a_statistics_data statistics_buffer[ISP3A_STATISTICS_BUF_NUM];
};

struct af_info {
	cmr_handle handle;
	cmr_u8 af_support;
	struct isp3a_af_hw_cfg hw_cfg;
	struct isp3a_statistics_data statistics_buffer[ISP3A_STATISTICS_BUF_NUM];
};

struct afl_info {
	cmr_handle handle;
	struct isp3a_afl_hw_cfg hw_cfg;
	struct isp3a_statistics_data statistics_buffer[ISP3A_STATISTICS_AFL_BUF_NUM];
};

struct other_stats_info {
	struct isp3a_yhis_hw_cfg yhis_hw_cfg;
	struct isp3a_subsample_hw_cfg subsample_hw_cfg;
	struct isp3a_statistics_data yhis_buffer[ISP3A_STATISTICS_BUF_NUM];
	struct isp3a_statistics_data sub_sample_buffer[ISP3A_STATISTICS_BUF_NUM];
};

struct stats_buf_context {
	struct isp3a_statistics_data *ae_stats_buf_ptr;
	struct isp3a_statistics_data *awb_stats_buf_ptr;
	struct isp3a_statistics_data *af_stats_buf_ptr;
	struct isp3a_statistics_data *afl_stats_buf_ptr;
	struct isp3a_statistics_data *yhis_stats_buf_ptr;
	struct isp3a_statistics_data *subsample_stats_buf_ptr;
};

struct isp3a_fw_debug_context {
	struct debug_info1 exif_debug_info;
	struct debug_info2 debug_info;
};

struct isp3a_fw_bin_context {
	struct isp_bin_info bin_info;
	cmr_u32 is_write_to_debug_buf;
};

struct isp3a_fw_context {
	cmr_u32 camera_id;
	cmr_u32 is_inited;
	cmr_int err_code;
	cmr_handle caller_handle;
	cmr_handle dev_access_handle;
	proc_callback caller_callback;
	struct isp_ops ops;
	struct isp3a_thread_context thread_cxt;
	sem_t statistics_data_sm;
	struct awb_info awb_cxt;
	struct ae_info ae_cxt;
	struct af_info af_cxt;
	struct afl_info afl_cxt;
	struct stats_buf_context stats_buf_cxt;
	struct other_stats_info  other_stats_cxt;
	void *setting_param_ptr;
	struct sensor_raw_ioctrl *ioctrl_ptr;
	struct isp3a_fw_bin_context bin_cxt;
	cmr_uint sof_idx;
	struct isp3a_fw_debug_context debug_data;
	struct isp_sensor_ex_info ex_info;
	struct sensor_otp_cust_info *otp_data;
#ifdef CONFIG_CAMERA_RE_FOCUS
	struct sensor_raw_ioctrl *ioctrl_ptr_slv;
	struct sensor_otp_cust_info *otp_data_slv;
#endif
};
/*************************************INTERNAK DECLARATION***************************************/
static cmr_int isp3a_get_dev_time(cmr_handle handle, cmr_u32 *sec_ptr, cmr_u32 *usec_ptr);
static cmr_int isp3a_ae_callback(cmr_handle handle, enum ae_ctrl_cb_type cmd, struct ae_ctrl_callback_in *in_ptr);
static cmr_int isp3a_awb_callback(cmr_handle handle, cmr_u32 cmd, struct awb_ctrl_callback_in *in_ptr);
static cmr_int isp3a_afl_callback(cmr_handle handle, enum afl_ctrl_cb_type cmd, struct afl_ctrl_callback_in *in_ptr);
static cmr_int isp3a_set_exposure(cmr_handle handle, struct ae_ctrl_param_sensor_exposure *in_ptr);
static cmr_int isp3a_ae_set_gain(cmr_handle handle, struct ae_ctrl_param_sensor_gain *in_ptr);
#ifdef CONFIG_CAMERA_RE_FOCUS
static cmr_int isp3a_set_exposure_slv(cmr_handle handle, struct ae_ctrl_param_sensor_exposure *in_ptr);
static cmr_int isp3a_ae_set_gain_slv(cmr_handle handle, struct ae_ctrl_param_sensor_gain *in_ptr);
#endif
static cmr_int isp3a_flash_get_charge(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr);
static cmr_int isp3a_flash_get_time(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr);
static cmr_int isp3a_flash_set_charge(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr);
static cmr_int isp3a_flash_set_time(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr);
static cmr_int isp3a_set_pos(cmr_handle handle, struct af_ctrl_motor_pos * in);
static cmr_int isp3a_start_af_notify(cmr_handle handle, void *data);
static cmr_int isp3a_end_af_notify(cmr_handle handle, struct af_result_param *data);
static cmr_int isp3a_alg_init(cmr_handle isp_3a_handle, struct isp_3a_fw_init_in* input_ptr);
static cmr_int isp3a_set_cfg_otp_info(cmr_handle isp_3a_handle);
static cmr_int isp3a_alg_deinit(cmr_handle isp_3a_handle);
static cmr_int isp3a_create_ctrl_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_ctrl_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int isp3a_destroy_ctrl_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_create_process_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_process_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int isp3a_destroy_process_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_create_receiver_thread(cmr_handle isp_3a_handle);
static void *isp3a_receiver_thread_proc(void* p_data);
static cmr_int isp3a_destroy_receiver_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_create_af_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_af_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int isp3a_destroy_af_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_create_afl_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_afl_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int isp3a_destroy_afl_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_create_thread(cmr_handle isp_3a_handle);
static cmr_int isp3a_destroy_thread(cmr_handle isp_3a_handle);
static isp3a_ioctrl_fun isp3a_get_ioctrl_fun(enum isp_ctrl_cmd cmd);
static cmr_int isp3a_ioctrl(cmr_handle isp_3a_handle, enum isp_ctrl_cmd cmd, void *param_ptr);
static cmr_int isp3a_set_awb_mode(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_scene_mode(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_measure_lum(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ev(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_flicker(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_awb_bypass(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_effect(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_brightness(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_contrast(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_start_af(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_saturation(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_af_mode(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_hdr(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_iso(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_stop_af(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_touch(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_sharpness(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_awb_flash_gain(cmr_handle isp_3a_handle);
static cmr_int isp3a_set_awb_flash_off_gain(cmr_handle isp_3a_handle);
static cmr_int isp3a_set_awb_capture_gain(cmr_handle isp_3a_handle);
static cmr_int isp3a_notice_flash(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_face_area(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_start_3a(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_stop_3a(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_notice_snapshot(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_fps_range(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_fps(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_notice_burst(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_get_info(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_night_mode(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_awb_lock(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_lock(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_dzoom(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_convergence_req(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_snapshot_finished(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_get_exif_debug_info(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_get_adgain_exp_info(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_flash_mode(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_mode(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_fix_exposure_time(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_fix_sensitivity(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_fix_frame_duration(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_manual_exptime(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_ae_manual_gain(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_aux_sensor_info(cmr_handle isp_3a_handle, void *sensor_info);
static cmr_int isp3a_init_statistics_buf(cmr_handle isp_3a_handle);
static cmr_int isp3a_deinit_statistics_buf(cmr_handle isp_3a_handle);
static cmr_int isp3a_get_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data **buf_ptr);
static cmr_int isp3a_put_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data *buf_ptr);//reserved
static cmr_int isp3a_hold_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data *buf_ptr);
static cmr_int isp3a_release_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data *buf_ptr);
static cmr_int isp3a_callback_func (cmr_handle handle, cmr_u32 cb_type, void *param_ptr, cmr_u32 param_len);
static void isp3a_dev_evt_cb(cmr_int evt, void* data, cmr_u32 data_len, void* privdata);
static cmr_int isp3a_get_3a_stats_buf(cmr_handle isp_3a_handle);
static cmr_int isp3a_put_3a_stats_buf(cmr_handle isp_3a_handle);
static cmr_int isp3a_handle_stats(cmr_handle isp_3a_handle, void *data);
static cmr_int isp3a_handle_sensor_sof(cmr_handle isp_3a_handle, void *data);
static cmr_int isp3a_start_ae_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data, struct isp3a_statistics_data *awb_stats_data);
static cmr_int isp3a_start_af_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data);
static cmr_int isp3a_start_afl_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data);
static cmr_int isp3a_start_yhis_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data);
static cmr_int isp3a_start_awb_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data, struct ae_ctrl_callback_in *ae_info);
static cmr_int isp3a_handle_ae_result(cmr_handle isp_3a_handle, struct ae_ctrl_callback_in *result_ptr);
static cmr_int isp3a_start(cmr_handle isp_3a_handle, struct isp_video_start* input_ptr);
static cmr_int isp3a_stop(cmr_handle isp_3a_handle);
static cmr_int isp3a_get_yhist_info(cmr_handle isp_3a_handle, void *param_ptr);
static cmr_int isp3a_set_prev_yhist(cmr_handle isp_3a_handle, void *param_ptr);


static struct isp3a_ctrl_io_func s_isp3a_ioctrl_tab[ISP_CTRL_MAX] = {
	{ISP_CTRL_AWB_MODE,                isp3a_set_awb_mode},
	{ISP_CTRL_SCENE_MODE,              isp3a_set_scene_mode},
	{ISP_CTRL_AE_MEASURE_LUM,          isp3a_set_measure_lum},
	{ISP_CTRL_EV,                      isp3a_set_ev},
	{ISP_CTRL_FLICKER,                 isp3a_set_flicker},
	{ISP_CTRL_AEAWB_BYPASS,            isp3a_set_ae_awb_bypass},
	{ISP_CTRL_SPECIAL_EFFECT,          isp3a_set_effect},
	{ISP_CTRL_BRIGHTNESS,              isp3a_set_brightness},
	{ISP_CTRL_CONTRAST,                isp3a_set_contrast},
	{ISP_CTRL_HIST,                    NULL},
	{ISP_CTRL_AF,                      isp3a_start_af},
	{ISP_CTRL_SATURATION,              isp3a_set_saturation},
	{ISP_CTRL_AF_MODE,                 isp3a_set_af_mode},
	{ISP_CTRL_AUTO_CONTRAST,           NULL},
	{ISP_CTRL_CSS,                     NULL},
	{ISP_CTRL_HDR,                     isp3a_set_hdr},
	{ISP_CTRL_GLOBAL_GAIN,             NULL},
	{ISP_CTRL_CHN_GAIN,                NULL},
	{ISP_CTRL_GET_EXIF_INFO,           NULL},
	{ISP_CTRL_ISO,                     isp3a_set_iso},
	{ISP_CTRL_WB_TRIM,                 NULL},
	{ISP_CTRL_PARAM_UPDATE,            NULL},
	{ISP_CTRL_FLASH_EG,                NULL},
	{ISP_CTRL_VIDEO_MODE,              NULL},
	{ISP_CTRL_AF_STOP,                 isp3a_stop_af},
	{ISP_CTRL_AE_TOUCH,                isp3a_set_ae_touch},
	{ISP_CTRL_AE_INFO,                 NULL},
	{ISP_CTRL_SHARPNESS,               isp3a_set_sharpness},
	{ISP_CTRL_GET_FAST_AE_STAB,        NULL},
	{ISP_CTRL_GET_AE_STAB,             NULL},
	{ISP_CTRL_GET_AE_CHG,              NULL},
	{ISP_CTRL_GET_AWB_STAT,            NULL},
	{ISP_CTRL_GET_AF_STAT,             NULL},
	{ISP_CTRL_GAMMA,                   NULL},
	{ISP_CTRL_DENOISE,                 NULL},
	{ISP_CTRL_SMART_AE,                NULL},
	{ISP_CTRL_CONTINUE_AF,             NULL},
	{ISP_CTRL_AF_DENOISE,              NULL},
	{ISP_CTRL_FLASH_CTRL,              NULL},// for isp tool
	{ISP_CTRL_AE_CTRL,                 NULL},// for isp tool
	{ISP_CTRL_AF_CTRL,                 NULL},// for isp tool
	{ISP_CTRL_REG_CTRL,                NULL},// for isp tool
	{ISP_CTRL_DENOISE_PARAM_READ,      NULL},//for isp tool
	{ISP_CTRL_DUMP_REG,                NULL},//for isp tool
	{ISP_CTRL_AF_END_INFO,             NULL}, // for isp tool
	{ISP_CTRL_FLASH_NOTICE,            isp3a_notice_flash},
	{ISP_CTRL_AE_FORCE_CTRL,           NULL}, // for mp tool
	{ISP_CTRL_GET_AE_STATE,            NULL},// for isp tool
	{ISP_CTRL_SET_LUM,                 NULL},// for isp tool
	{ISP_CTRL_GET_LUM,                 NULL},// for isp tool
	{ISP_CTRL_SET_AF_POS,              NULL},// for isp tool
	{ISP_CTRL_GET_AF_POS,              NULL},// for isp tool
	{ISP_CTRL_GET_AF_MODE,             NULL},// for isp tool
	{ISP_CTRL_FACE_AREA,               isp3a_set_face_area},
	{ISP_CTRL_SCALER_TRIM,             NULL},
	{ISP_CTRL_START_3A,                isp3a_start_3a},
	{ISP_CTRL_STOP_3A,                 isp3a_stop_3a},
	{IST_CTRL_SNAPSHOT_NOTICE,         isp3a_notice_snapshot},//NA
	{ISP_CTRL_SFT_READ,                NULL},
	{ISP_CTRL_SFT_WRITE,               NULL},
	{ISP_CTRL_SFT_SET_PASS,            NULL},// added for sft
	{ISP_CTRL_SFT_GET_AF_VALUE,        NULL},// added for sft
	{ISP_CTRL_SFT_SET_BYPASS,          NULL},// added for sft
	{ISP_CTRL_GET_AWB_GAIN,            NULL},// for mp tool
	{ISP_CTRL_RANGE_FPS,               isp3a_set_fps_range},
	{ISP_CTRL_SET_AE_FPS,              isp3a_set_ae_fps},
	{ISP_CTRL_BURST_NOTICE,            isp3a_notice_burst},
	{ISP_CTRL_GET_INFO,                isp3a_get_info},//debug
	{ISP_CTRL_SET_AE_NIGHT_MODE,       isp3a_set_ae_night_mode},
	{ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK,  isp3a_set_ae_awb_lock},
	{ISP_CTRL_SET_AE_LOCK_UNLOCK,      isp3a_set_ae_lock},
	{ISP_CTRL_IFX_PARAM_UPDATE,        NULL},
	{ISP_CTRL_SET_DZOOM_FACTOR,        isp3a_set_dzoom},
	{ISP_CTRL_SET_CONVERGENCE_REQ,     isp3a_set_convergence_req},
	{ISP_CTRL_SET_SNAPSHOT_FINISHED,   isp3a_set_snapshot_finished},
	{ISP_CTRL_GET_EXIF_DEBUG_INFO,     isp3a_get_exif_debug_info},
	{ISP_CTRL_GET_CUR_ADGAIN_EXP,      isp3a_get_adgain_exp_info},
	{ISP_CTRL_SET_FLASH_MODE,          isp3a_set_flash_mode},
	{ISP_CTRL_SET_AE_MODE,             isp3a_set_ae_mode},
	{ISP_CTRL_SET_AE_FIX_EXP_TIME,     isp3a_set_ae_fix_exposure_time},
	{ISP_CTRL_SET_AE_FIX_SENSITIVITY,  isp3a_set_ae_fix_sensitivity},
	{ISP_CTRL_SET_AE_FIX_FRAM_DURA,    isp3a_set_ae_fix_frame_duration},
	{ISP_CTRL_SET_AE_MANUAL_EXPTIME,   isp3a_set_ae_manual_exptime},
	{ISP_CTRL_SET_AE_MANUAL_GAIN,      isp3a_set_ae_manual_gain},
	{ISP_CTRL_SET_AUX_SENSOR_INFO,     isp3a_set_aux_sensor_info},
	{ISP_CTRL_GET_YSTAT,               isp3a_get_yhist_info},
	{ISP_CTRL_SET_PREV_YHIST,          isp3a_set_prev_yhist},
};

/*************************************INTERNAK FUNCTION ***************************************/
cmr_int isp3a_get_dev_time(cmr_handle handle, cmr_u32 *sec_ptr, cmr_u32 *usec_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	union isp_dev_ctrl_cmd_out                  output_param;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_GET_TIME, NULL, &output_param);
	if (!ret) {
		*sec_ptr = output_param.time.sec;
		*usec_ptr = output_param.time.usec;
	} else {
		*sec_ptr = 0;
		*usec_ptr = 0;
	}
	return ret;
}

cmr_int isp3a_ae_callback(cmr_handle handle, enum ae_ctrl_cb_type cmd, struct ae_ctrl_callback_in *in_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;
	cmr_u32                                     callback_cmd = ISP_CALLBACK_CMD_MAX;

	ISP_LOGI("cmd =%d", cmd);
	switch (cmd) {
	case AE_CTRL_CB_CONVERGED:
	case AE_CTRL_CB_FLASHING_CONVERGED:
	case AE_CTRL_CB_CLOSE_PREFLASH:
	case AE_CTRL_CB_CLOSE_MAIN_FLASH:
	case AE_CTRL_CB_PREFLASH_PERIOD_END:
	case AE_CTRl_CB_TOUCH_CONVERGED:
		callback_cmd = ISP_AE_STAB_CALLBACK;
		break;
	case AE_CTRL_CB_QUICKMODE_DOWN:
		callback_cmd = ISP_QUICK_MODE_DOWN;
		break;
	case AE_CTRL_CB_STAB_NOTIFY:
		callback_cmd = ISP_AE_STAB_NOTIFY;
		break;
	case AE_CTRL_CB_AE_LOCK_NOTIFY:
		callback_cmd =ISP_AE_LOCK_NOTIFY;
		break;
	case AE_CTRL_CB_AE_UNLOCK_NOTIFY:
		callback_cmd = ISP_AE_UNLOCK_NOTIFY;
		break;
	case AE_CTRL_CB_PROC_OUT:
		ret = isp3a_handle_ae_result(handle, in_ptr);
		break;
	case AE_CTRL_CB_SYNC_INFO:
		callback_cmd = ISP_AE_SYNC_INFO;
		if (cxt->caller_callback) {
			ret = cxt->caller_callback(cxt->caller_handle, ISP_CALLBACK_EVT|callback_cmd
					, in_ptr->ae_sync_info, sizeof(*in_ptr->ae_sync_info));
		}
		break;
	default:
		goto exit;
		break;
	}
	if (AE_CTRL_CB_PROC_OUT == cmd || AE_CTRL_CB_SYNC_INFO == cmd) {
		goto exit;
	}
	if (cxt->caller_callback) {
		ret = cxt->caller_callback(cxt->caller_handle, ISP_CALLBACK_EVT|callback_cmd, NULL, 0);
	}
exit:
	return ret;
}

cmr_int isp3a_awb_callback(cmr_handle handle, enum ae_ctrl_cb_type cmd, struct awb_ctrl_callback_in *in_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	UNUSED(in_ptr);
	switch (cmd) {
	case AWB_CTRL_CB_CONVERGE:
		cxt->awb_cxt.awb_status = AWB_CTRL_STATUS_CONVERGE;
		break;
	default:
		break;
	}
	return ret;
}

cmr_int isp3a_afl_callback(cmr_handle handle, enum afl_ctrl_cb_type cmd, struct afl_ctrl_callback_in *in_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;
	struct ae_ctrl_param_in                     ae_in;

	if (!cxt || !in_ptr) {
		ISP_LOGE("cxt in_ptr is NULL");
		goto exit;
	}

	ISP_LOGI("cmd =%d", cmd);
	switch (cmd) {
	case AFL_CTRL_CB_FLICKER_MODE:
		ISP_LOGI("flicker_mode =%d", in_ptr->flicker_mode);
		ae_in.flicker.flicker_mode = in_ptr->flicker_mode;
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FLICKER, &ae_in, NULL);
		break;
	case AFL_CTRL_CB_STAT_DATA:
		ISP_LOGI("release stat_data =%p", in_ptr->stat_data);
		isp3a_release_statistics_buf(handle, ISP3A_AFL, in_ptr->stat_data);
		break;
	default:
		break;
	}

exit:
	return ret;
}

struct sensor_ex_exposure {
	cmr_u32 exposure;
	cmr_u32 dummy;
	cmr_u32 size_index;
};

cmr_int isp3a_set_exposure(cmr_handle handle, struct ae_ctrl_param_sensor_exposure *in_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;
	cmr_u32                                     sensor_param = 0;
	struct sensor_ex_exposure                   ex_exp;


	if (!cxt || !cxt->ioctrl_ptr || !in_ptr) {
		ISP_LOGE("don't have io interface");
		goto exit;
	}

	if (cxt->ioctrl_ptr->ex_set_exposure) {
		ex_exp.exposure = in_ptr->exp_line;
		ex_exp.dummy = in_ptr->dummy;
		ex_exp.size_index = in_ptr->size_index;

		ret = cxt->ioctrl_ptr->ex_set_exposure(cxt->ioctrl_ptr->caller_handler, (unsigned long)&ex_exp);
	} else if (cxt->ioctrl_ptr->set_exposure) {
		sensor_param = in_ptr->exp_line& 0x0000ffff;
		sensor_param |= (in_ptr->dummy << 0x10) & 0x0fff0000;
		sensor_param |= (in_ptr->size_index << 0x1c) & 0xf0000000;

		ret = cxt->ioctrl_ptr->set_exposure(cxt->ioctrl_ptr->caller_handler, sensor_param);
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_ae_set_gain(cmr_handle handle, struct ae_ctrl_param_sensor_gain *in_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt || !cxt->ioctrl_ptr || !cxt->ioctrl_ptr->set_gain || !in_ptr) {
		ISP_LOGE("don't have io interface");
		goto exit;
	}
	ret = cxt->ioctrl_ptr->set_gain(cxt->ioctrl_ptr->caller_handler, in_ptr->gain);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

#ifdef CONFIG_CAMERA_RE_FOCUS
cmr_int isp3a_set_exposure_slv(cmr_handle handle, struct ae_ctrl_param_sensor_exposure *in_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;
	cmr_u32                                     sensor_param = 0;
	struct sensor_ex_exposure                   ex_exp;


	if (!cxt || !cxt->ioctrl_ptr_slv || !in_ptr) {
		ISP_LOGE("don't have io interface");
		goto exit;
	}

	if (cxt->ioctrl_ptr_slv->ex_set_exposure) {
		ex_exp.exposure = in_ptr->exp_line;
		ex_exp.dummy = in_ptr->dummy;
		ex_exp.size_index = in_ptr->size_index;

		ret = cxt->ioctrl_ptr_slv->ex_set_exposure(cxt->ioctrl_ptr_slv->caller_handler, (unsigned long)&ex_exp);
	} else if (cxt->ioctrl_ptr_slv->set_exposure) {
		sensor_param = in_ptr->exp_line& 0x0000ffff;
		sensor_param |= (in_ptr->dummy << 0x10) & 0x0fff0000;
		sensor_param |= (in_ptr->size_index << 0x1c) & 0xf0000000;

		ret = cxt->ioctrl_ptr_slv->set_exposure(cxt->ioctrl_ptr_slv->caller_handler, sensor_param);
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_ae_set_gain_slv(cmr_handle handle, struct ae_ctrl_param_sensor_gain *in_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt || !cxt->ioctrl_ptr_slv || !cxt->ioctrl_ptr_slv->set_gain || !in_ptr) {
		ISP_LOGE("don't have io interface");
		goto exit;
	}
	ret = cxt->ioctrl_ptr_slv->set_gain(cxt->ioctrl_ptr_slv->caller_handler, in_ptr->gain);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}
#endif

cmr_int isp3a_flash_get_charge(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt->ops.flash_get_charge) {
		ISP_LOGI("failed to call flash_get_charge");
		goto exit;
	}
	ret = cxt->ops.flash_get_charge(cxt->caller_handle, cfg_ptr, cell_ptr);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_flash_get_time(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_cell *cell_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt->ops.flash_get_time) {
		ISP_LOGI("failed to call flash_get_time");
		goto exit;
	}
	ret = cxt->ops.flash_get_time(cxt->caller_handle, cfg_ptr, cell_ptr);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_flash_set_charge(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt->ops.flash_set_charge) {
		ISP_LOGI("failed to call flash_set_charge");
		goto exit;
	}
	ret = cxt->ops.flash_set_charge(cxt->caller_handle, cfg_ptr, element_ptr);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_flash_ctrl(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt->ops.flash_ctrl) {
		ISP_LOGI("failed to call flash_ctrl");
		goto exit;
	}
	ret = cxt->ops.flash_ctrl(cxt->caller_handle, cfg_ptr, element_ptr);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_flash_set_time(cmr_handle handle, struct isp_flash_cfg *cfg_ptr, struct isp_flash_element *element_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt->ops.flash_set_time) {
		ISP_LOGI("failed to call flash_set_time");
		goto exit;
	}
	ret = cxt->ops.flash_set_time(cxt->caller_handle, cfg_ptr, element_ptr);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_set_pos(cmr_handle handle, struct af_ctrl_motor_pos * in)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	if (!cxt || !cxt->ioctrl_ptr || !cxt->ioctrl_ptr->set_focus || !in) {
		ISP_LOGE("don't have io interface");
		goto exit;
	}
	ret = cxt->ioctrl_ptr->set_focus(cxt->ioctrl_ptr->caller_handler, in->motor_pos);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_cfg_af_param(cmr_handle handle, void *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;

	ISP_LOGI("E");
	memcpy(&cxt->af_cxt.hw_cfg, data, sizeof(struct isp3a_af_hw_cfg));
	ret = isp_dev_access_cfg_af_param(cxt->dev_access_handle, data);

	return ret;
}

cmr_int isp3a_start_af_notify(cmr_handle handle, void *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;
	struct isp_af_notice                        af_notice = {0x00};

	UNUSED(data);
	ISP_LOGI("move start");
	if (!cxt || !cxt->caller_callback) {
		ISP_LOGE("calllback is NULL");
		goto exit;
	}
	af_notice.mode = ISP_FOCUS_MOVE_START;
	af_notice.valid_win = 0x00;

	ret = cxt->caller_callback(cxt->caller_handle, ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK,(void*)&af_notice, sizeof(struct isp_af_notice));
exit:
	ISP_LOGI("done, %ld", ret);
	return ret;
}

cmr_int isp3a_end_af_notify(cmr_handle handle, struct af_result_param *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)handle;
	struct isp_af_notice                        af_notice = {0x00};

	ISP_LOGI("move end");
	if (!cxt || !cxt->caller_callback) {
		ISP_LOGE("calllback is NULL");
		goto exit;
	}
	af_notice.mode = ISP_FOCUS_MOVE_END;
	af_notice.valid_win = data->suc_win;
	ret = cxt->caller_callback(cxt->caller_handle,
				   ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK,
				   (void*)&af_notice,
				   sizeof(struct isp_af_notice));
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_set_cfg_otp_info(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = NULL;
	struct isp_iq_otp_info                 iq_info;
	cmr_u16 size;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	cxt = (struct isp3a_fw_context *)isp_3a_handle;
	memset(&iq_info, 0x00, sizeof(iq_info));
	if (cxt->otp_data) {
		iq_info.cali_status = cxt->otp_data->program_flag;
		iq_info.iso = cxt->otp_data->isp_awb_info.iso;
		iq_info.r_gain = cxt->otp_data->isp_awb_info.gain_r;
		iq_info.g_gain = cxt->otp_data->isp_awb_info.gain_g;
		iq_info.b_gain = cxt->otp_data->isp_awb_info.gain_b;
		if (cxt->otp_data->lsc_info.lsc_data_addr) {
			size = MIN(cxt->otp_data->lsc_info.lsc_data_size, OTP_LSC_DATA_SIZE);
			memcpy(&iq_info.lsc[0], cxt->otp_data->lsc_info.lsc_data_addr, size);
			iq_info.lsc_length = size;
		}
	} else if (cxt->bin_cxt.bin_info.otp_data_addr) {
		ISP_LOGV("use bin otp data");
		iq_info.cali_status = 1;
		iq_info.iso = cxt->bin_cxt.bin_info.otp_data_addr->iso;
		iq_info.r_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_r;
		iq_info.g_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_g;
		iq_info.b_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_b;
		if (cxt->bin_cxt.bin_info.lsc_otp_addr) {
			memcpy(&iq_info.lsc[0], cxt->bin_cxt.bin_info.lsc_otp_addr, OTP_LSC_DATA_SIZE);
			iq_info.lsc_length = OTP_LSC_DATA_SIZE;
		}
	}
	ret = isp_dev_access_set_cfg_otp_info(cxt->dev_access_handle, &iq_info);
	if (ret)
		ISP_LOGE("failed to set cfg otp info");

	return ret;
}

cmr_int isp3a_alg_init(cmr_handle isp_3a_handle, struct isp_3a_fw_init_in* input_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = NULL;
	struct af_ctrl_init_in                      af_input;
	struct af_ctrl_init_out                     af_output;
	struct awb_ctrl_init_in                     awb_input;
	struct awb_ctrl_init_out                    awb_output;
	struct ae_ctrl_init_in                      ae_input;
	struct ae_ctrl_init_out                     ae_output;
	struct afl_ctrl_init_in                     afl_input;
	struct afl_ctrl_init_out                    afl_output;
	struct sensor_raw_info                      *sensor_raw_info_ptr = (struct sensor_raw_info*)input_ptr->setting_param_ptr;
#ifdef CONFIG_CAMERA_RE_FOCUS
	struct sensor_raw_info                      *sensor_raw_info_ptr_slv = (struct sensor_raw_info*)input_ptr->setting_param_ptr_slv;
#endif
	float                                       libVersion;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	cxt = (struct isp3a_fw_context *)isp_3a_handle;

	cxt->af_cxt.af_support = !!cxt->ioctrl_ptr->set_focus;

	if (ERR_3ALIB_VER_SUCCESS == allib_3a_getversion(&libVersion)) {
		ISP_LOGI("3a version:%f", libVersion);
	}

	memset(&af_input, 0x00, sizeof(af_input));
	af_input.camera_id = input_ptr->camera_id;
	af_input.af_lib_info = input_ptr->af_config;
	af_input.caller_handle = isp_3a_handle;
	af_input.af_support = cxt->af_cxt.af_support;
	af_input.isp_info.img_width = sensor_raw_info_ptr->resolution_info_ptr->tab[1].width;
	af_input.isp_info.img_height = sensor_raw_info_ptr->resolution_info_ptr->tab[1].height;
	af_input.sensor_info.crop_info.x = sensor_raw_info_ptr->resolution_info_ptr->tab[1].start_x;
	af_input.sensor_info.crop_info.y = sensor_raw_info_ptr->resolution_info_ptr->tab[1].start_y;
	af_input.sensor_info.crop_info.width = sensor_raw_info_ptr->resolution_info_ptr->tab[1].width;
	af_input.sensor_info.crop_info.height = sensor_raw_info_ptr->resolution_info_ptr->tab[1].height;
	af_input.sensor_info.sensor_res_width = sensor_raw_info_ptr->resolution_info_ptr->tab[1].width;
	af_input.sensor_info.sensor_res_height = sensor_raw_info_ptr->resolution_info_ptr->tab[1].height;
	af_input.sensor_info.pose_dis.up2hori = input_ptr->ex_info.pos_dis.up2hori;
	af_input.sensor_info.pose_dis.hori2down = input_ptr->ex_info.pos_dis.hori2down;
	af_input.module_info.f_num = input_ptr->ex_info.f_num;
	af_input.module_info.focal_length = input_ptr->ex_info.focal_length;
	af_input.af_ctrl_cb_ops.set_pos = isp3a_set_pos;
	af_input.af_ctrl_cb_ops.start_notify = isp3a_start_af_notify;
	af_input.af_ctrl_cb_ops.end_notify = isp3a_end_af_notify;
	af_input.af_ctrl_cb_ops.lock_ae_awb = isp3a_set_ae_awb_lock;
	af_input.af_ctrl_cb_ops.cfg_af_stats = isp3a_cfg_af_param;
	af_input.af_ctrl_cb_ops.get_system_time = isp3a_get_dev_time;
	af_input.tuning_info.tuning_file = input_ptr->bin_info.af_addr;
	if (cxt->otp_data) {
		af_input.otp_info.otp_data = &cxt->otp_data->af_info;
		af_input.otp_info.size = sizeof(cxt->otp_data->af_info);
	}
	ret = af_ctrl_init(&af_input, &af_output, &cxt->af_cxt.handle);
	if (ret) {
		ISP_LOGE("failed to AF initialize");
		goto exit;
	}
	cxt->af_cxt.hw_cfg = af_output.hw_cfg;

	memset(&awb_input, 0x00, sizeof(awb_input));
	awb_input.awb_cb = isp3a_awb_callback;
	awb_input.camera_id = input_ptr->camera_id;
	awb_input.lib_config = input_ptr->awb_config;
	awb_input.wb_mode = AWB_CTRL_WB_MODE_AUTO;
	awb_input.caller_handle = isp_3a_handle;
	awb_input.awb_process_type = AWB_CTRL_RESPONSE_STABLE;
	awb_input.awb_process_level = AWB_CTRL_RESPONSE_NORMAL;
	awb_input.tuning_param = input_ptr->bin_info.awb_addr;
	if (cxt->otp_data) {
		awb_input.calibration_gain.r = cxt->otp_data->isp_awb_info.gain_r;
		awb_input.calibration_gain.g = cxt->otp_data->isp_awb_info.gain_g;
		awb_input.calibration_gain.b = cxt->otp_data->isp_awb_info.gain_b;
	} else if (input_ptr->bin_info.otp_data_addr) {
		awb_input.calibration_gain.r = input_ptr->bin_info.otp_data_addr->gain_r;
		awb_input.calibration_gain.g = input_ptr->bin_info.otp_data_addr->gain_g;
		awb_input.calibration_gain.b = input_ptr->bin_info.otp_data_addr->gain_b;
	}
	ISP_LOGI("awb bin %p", awb_input.tuning_param);
	ret = awb_ctrl_init(&awb_input, &awb_output, &cxt->awb_cxt.handle);
	if (ret) {
		ISP_LOGE("failed to AWB initialize");
		goto exit;
	}
	cxt->awb_cxt.proc_out.gain = awb_output.gain;
	cxt->awb_cxt.proc_out.gain_balanced = awb_output.gain_balanced;
	cxt->awb_cxt.proc_out.ct = awb_output.ct;
	cxt->awb_cxt.hw_cfg = awb_output.hw_cfg;

	cmr_bzero(&ae_input, sizeof(ae_input));
	ae_input.camera_id = input_ptr->camera_id;
	ae_input.caller_handle = isp_3a_handle;
	ae_input.lib_param = input_ptr->ae_config;
	ae_input.ops_in.get_system_time = isp3a_get_dev_time;
	ae_input.ops_in.ae_callback = isp3a_ae_callback;
	ae_input.ops_in.set_again = isp3a_ae_set_gain;
	ae_input.ops_in.flash_get_charge = isp3a_flash_get_charge;
	ae_input.ops_in.flash_get_time = isp3a_flash_get_time;
	ae_input.ops_in.flash_set_charge = isp3a_flash_set_charge;
	ae_input.ops_in.flash_ctrl = isp3a_flash_ctrl;
	ae_input.ops_in.flash_set_time = isp3a_flash_set_time;
	ae_input.ops_in.set_exposure = isp3a_set_exposure;
	ae_input.ops_in.release_stat_buffer = isp3a_release_statistics_buf;

	ae_input.sensor_static_info.f_num = input_ptr->ex_info.f_num;
	ae_input.sensor_static_info.exposure_valid_num = input_ptr->ex_info.exp_valid_frame_num;
	ae_input.sensor_static_info.gain_valid_num = input_ptr->ex_info.adgain_valid_frame_num;
	ae_input.sensor_static_info.preview_skip_num = input_ptr->ex_info.preview_skip_num;
	ae_input.sensor_static_info.capture_skip_num = input_ptr->ex_info.capture_skip_num;
	ae_input.sensor_static_info.max_fps = input_ptr->ex_info.max_fps;
	ae_input.sensor_static_info.max_gain = input_ptr->ex_info.max_adgain;
	ae_input.sensor_static_info.ois_supported = input_ptr->ex_info.ois_supported;

	ae_input.preview_work.work_mode = 0;
	ae_input.preview_work.resolution.frame_size.w = sensor_raw_info_ptr->resolution_info_ptr->tab[1].width;
	ae_input.preview_work.resolution.frame_size.h = sensor_raw_info_ptr->resolution_info_ptr->tab[1].height;
	ae_input.preview_work.resolution.frame_line = sensor_raw_info_ptr->resolution_info_ptr->tab[1].frame_line;
	ae_input.preview_work.resolution.line_time = sensor_raw_info_ptr->resolution_info_ptr->tab[1].line_time;;
	ae_input.preview_work.resolution.max_fps = input_ptr->ex_info.max_fps;
	ae_input.preview_work.resolution.max_gain = input_ptr->ex_info.max_adgain;
	ae_input.preview_work.resolution.sensor_size_index = 1;

#ifdef CONFIG_CAMERA_RE_FOCUS
	ae_input.ops_in.set_again_slv = isp3a_ae_set_gain_slv;
	ae_input.ops_in.set_exposure_slv = isp3a_set_exposure_slv;
	ae_input.sensor_static_info_slv.f_num = input_ptr->ex_info_slv.f_num;
	ae_input.sensor_static_info_slv.exposure_valid_num = input_ptr->ex_info_slv.exp_valid_frame_num;
	ae_input.sensor_static_info_slv.gain_valid_num = input_ptr->ex_info_slv.adgain_valid_frame_num;
	ae_input.sensor_static_info_slv.preview_skip_num = input_ptr->ex_info_slv.preview_skip_num;
	ae_input.sensor_static_info_slv.capture_skip_num = input_ptr->ex_info_slv.capture_skip_num;
	ae_input.sensor_static_info_slv.max_fps = input_ptr->ex_info_slv.max_fps;
	ae_input.sensor_static_info_slv.max_gain = input_ptr->ex_info_slv.max_adgain;
	ae_input.sensor_static_info_slv.ois_supported = input_ptr->ex_info_slv.ois_supported;
	ae_input.preview_work_slv.work_mode = 0;
	if (NULL != sensor_raw_info_ptr_slv) {
		ae_input.preview_work_slv.resolution.frame_size.w = sensor_raw_info_ptr_slv->resolution_info_ptr->tab[1].width;
		ae_input.preview_work_slv.resolution.frame_size.h = sensor_raw_info_ptr_slv->resolution_info_ptr->tab[1].height;
		ae_input.preview_work_slv.resolution.frame_line = sensor_raw_info_ptr_slv->resolution_info_ptr->tab[1].frame_line;
		ae_input.preview_work_slv.resolution.line_time = sensor_raw_info_ptr_slv->resolution_info_ptr->tab[1].line_time;;
	}
	ae_input.preview_work_slv.resolution.max_fps = input_ptr->ex_info_slv.max_fps;
	ae_input.preview_work_slv.resolution.max_gain = input_ptr->ex_info_slv.max_adgain;
	ae_input.preview_work_slv.resolution.sensor_size_index = 1;
	if (cxt->otp_data_slv) {
		ae_input.otp_data_slv.r = cxt->otp_data_slv->isp_awb_info.gain_r;
		ae_input.otp_data_slv.g = cxt->otp_data_slv->isp_awb_info.gain_g;
		ae_input.otp_data_slv.b = cxt->otp_data_slv->isp_awb_info.gain_b;
	}
#endif

	ae_input.tuning_param = input_ptr->bin_info.ae_addr;
	if (cxt->otp_data) {
		ae_input.otp_data.r = cxt->otp_data->isp_awb_info.gain_r;
		ae_input.otp_data.g = cxt->otp_data->isp_awb_info.gain_g;
		ae_input.otp_data.b = cxt->otp_data->isp_awb_info.gain_b;
	} else if (input_ptr->bin_info.otp_data_addr) {
		ae_input.otp_data.r = input_ptr->bin_info.otp_data_addr->gain_r;
		ae_input.otp_data.g = input_ptr->bin_info.otp_data_addr->gain_g;
		ae_input.otp_data.b = input_ptr->bin_info.otp_data_addr->gain_b;
	}
	ret = ae_ctrl_init(&ae_input, &ae_output, &cxt->ae_cxt.handle);
	if (ret) {
		ISP_LOGE("failed to AE initialize");
	}
	cxt->ae_cxt.hw_cfg = ae_output.hw_cfg;

	cmr_bzero(&afl_input, sizeof(afl_input));
	afl_input.camera_id = input_ptr->camera_id;
	afl_input.caller_handle = isp_3a_handle;
	afl_input.lib_param = input_ptr->afl_config;
	afl_input.ops_in.afl_callback = isp3a_afl_callback;
	afl_input.init_param.resolution.line_time = sensor_raw_info_ptr->resolution_info_ptr->tab[1].line_time;
	afl_input.init_param.resolution.frame_size.w = sensor_raw_info_ptr->resolution_info_ptr->tab[1].width;
	afl_input.init_param.resolution.frame_size.h = sensor_raw_info_ptr->resolution_info_ptr->tab[1].height;
	ret = afl_ctrl_init(&afl_input, &afl_output, &cxt->afl_cxt.handle);
	if (ret) {
		ISP_LOGE("failed to afl initialize");
		goto exit;
	}
	cxt->afl_cxt.hw_cfg = afl_output.hw_cfg;
exit:
	ISP_LOGI("done %ld", ret);
	if (ret) {
		if (cxt->af_cxt.handle) {
			af_ctrl_deinit(cxt->af_cxt.handle);
		}
		if (cxt->ae_cxt.handle) {
			ae_ctrl_deinit(cxt->ae_cxt.handle);
		}
		if (cxt->awb_cxt.handle) {
			awb_ctrl_deinit(cxt->awb_cxt.handle);
		}
		if (cxt->afl_cxt.handle) {
			afl_ctrl_deinit(cxt->afl_cxt.handle);
		}
	}
	return ret;
}

cmr_int isp3a_alg_deinit(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!cxt) {
		ISP_LOGI("input is NULL");
		goto exit;
	}

	af_ctrl_deinit(cxt->af_cxt.handle);
	awb_ctrl_deinit(cxt->awb_cxt.handle);
	ae_ctrl_deinit(cxt->ae_cxt.handle);
	afl_ctrl_deinit(cxt->afl_cxt.handle);

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}
cmr_int isp3a_create_ctrl_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.ctrl_thr_handle, ISP3A_MSG_QUEUE_SIZE,
							isp3a_ctrl_thread_proc, (void*)isp_3a_handle);
	ISP_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.ctrl_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create ctrl thread");
		ret = ISP_ERROR;
	}

	return ret;
}

cmr_int isp3a_ctrl_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_handle                                  isp_3a_handle = (cmr_handle)p_data;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGI("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP3A_CTRL_EVT_INIT:
		break;
	case ISP3A_CTRL_EVT_DEINIT:
		break;
	case ISP3A_CTRL_EVT_EXIT:
		break;
	case ISP3A_CTRL_EVT_START:
		break;
	case ISP3A_CTRL_EVT_STOP:
		break;
	case ISP3A_CTRL_EVT_IOCTRL:
		ret = isp3a_ioctrl((cmr_handle)cxt, message->sub_msg_type, message->data);
		break;
	case ISP3A_CTRL_EVT_PROCESS:
		break;
	default:
		ISP_LOGI("don't support msg");
		break;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_destroy_ctrl_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_thread_context                 *isp3a_thread_cxt;

	if (!isp_3a_handle) {
		ISP_LOGE("input is NULL");
		ret = ISP_ERROR;
		goto exit;
	}
	isp3a_thread_cxt = &cxt->thread_cxt;
	if (isp3a_thread_cxt->ctrl_thr_handle) {
		ret = cmr_thread_destroy(isp3a_thread_cxt->ctrl_thr_handle);
		if (!ret) {
			isp3a_thread_cxt->ctrl_thr_handle = (cmr_handle)NULL;
		} else {
			ISP_LOGE("failed to destroy ctrl thread");
		}
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp3a_create_process_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.process_thr_handle, ISP3A_MSG_QUEUE_SIZE,
							isp3a_process_thread_proc, (void*)isp_3a_handle);
	ISP_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.process_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create process thread");
		ret = ISP_ERROR;
	}

	return ret;
}

cmr_int isp3a_process_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_handle                                  isp_3a_handle = (cmr_handle)p_data;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGV("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP3A_PROC_EVT_INIT:
		break;
	case ISP3A_PROC_EVT_DEINIT:
		break;
	case ISP3A_PROC_EVT_EXIT:
		break;
	case ISP3A_PROC_EVT_IOCTRL:
		ret = isp3a_ioctrl((cmr_handle)cxt, message->sub_msg_type, message->data);
		break;
	case ISP3A_PROC_EVT_PROCESS:
		break;
	case ISP3A_PROC_EVT_START:
		ret = isp3a_start((cmr_handle)cxt, message->data);
		break;
	case ISP3A_PROC_EVT_STATS:
		ret = isp3a_handle_stats((cmr_handle)cxt, message->data);
		break;
	case ISP3A_PROC_EVT_SENSOR_SOF:
		ret = isp3a_handle_sensor_sof((cmr_handle)cxt, message->data);
		break;
	case ISP3A_PROC_EVT_STOP:
		ret = isp3a_stop((cmr_handle)cxt);
		break;
	default:
		ISP_LOGI("don't support msg");
		break;
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp3a_destroy_process_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_thread_context                 *isp3a_thread_cxt;

	if (!isp_3a_handle) {
		ISP_LOGE("input is NULL");
		ret = ISP_ERROR;
		goto exit;
	}

	isp3a_thread_cxt = &cxt->thread_cxt;
	if (isp3a_thread_cxt->process_thr_handle) {
		ret = cmr_thread_destroy(isp3a_thread_cxt->process_thr_handle);
		if (!ret) {
			isp3a_thread_cxt->process_thr_handle = (cmr_handle)NULL;
		} else {
			ISP_LOGE("failed to destroy process thread");
		}
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp3a_create_receiver_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	pthread_attr_t                              attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&cxt->thread_cxt.receiver_thr_handle, &attr, isp3a_receiver_thread_proc, (void*)isp_3a_handle);
	pthread_attr_destroy(&attr);

	return ret;
}

void *isp3a_receiver_thread_proc(void* p_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_handle                                  isp_3a_handle = (cmr_handle)p_data;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGI("In");

	while(1) {
	}
exit:
	ISP_LOGI("done %ld", ret);
	return NULL;
}

cmr_int isp3a_destroy_receiver_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_thread_context                 *isp3a_thread_cxt;
	void                                        *dummy;

	if (!isp_3a_handle) {
		ISP_LOGE("input is NULL");
		ret = ISP_ERROR;
		goto exit;
	}
	isp3a_thread_cxt = &cxt->thread_cxt;
	if (!isp3a_thread_cxt->receiver_thr_handle) {
		ISP_LOGI("receiver thread has been destroied");
		goto exit;
	}

	/* notice kernel destroy thread */

	isp3a_thread_cxt->receiver_thr_handle =(pthread_t)NULL;

exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_create_af_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.af_thr_handle, ISP3A_MSG_QUEUE_SIZE,
							isp3a_af_thread_proc, (void*)isp_3a_handle);
	ISP_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.af_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create af thread");
		ret = ISP_ERROR;
	}

	return ret;
}

cmr_int isp3a_af_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_handle                                  isp_3a_handle = (cmr_handle)p_data;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGI("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP3A_AF_EVT_INIT:
		break;
	case ISP3A_AF_EVT_DEINIT:
		break;
	case ISP3A_AF_EVT_EXIT:
		break;
	case ISP3A_AF_EVT_IOCTRL:
		break;
	case ISP3A_AF_EVT_PROCESS:
		break;
	default:
		ISP_LOGI("don't support msg");
		break;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_destroy_af_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_thread_context                 *isp3a_thread_cxt;

	if (!isp_3a_handle) {
		ISP_LOGE("input is NULL");
		ret = ISP_ERROR;
		goto exit;
	}
	isp3a_thread_cxt = &cxt->thread_cxt;
	if (isp3a_thread_cxt->af_thr_handle) {
		ret = cmr_thread_destroy(isp3a_thread_cxt->af_thr_handle);
		if (!ret) {
			isp3a_thread_cxt->af_thr_handle = (cmr_handle)NULL;
		} else {
			ISP_LOGE("failed to destroy af thread");
		}
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_create_afl_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.afl_thr_handle, ISP3A_MSG_QUEUE_SIZE,
							isp3a_afl_thread_proc, (void*)isp_3a_handle);
	ISP_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.afl_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		ISP_LOGE("failed to create afl thread");
		ret = ISP_ERROR;
	}

	return ret;
}

cmr_int isp3a_afl_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_handle                                  isp_3a_handle = (cmr_handle)p_data;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!message || !p_data) {
		ISP_LOGE("param error");
		goto exit;
	}
	ISP_LOGI("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	switch (message->msg_type) {
	case ISP3A_AF_EVT_INIT:
		break;
	case ISP3A_AF_EVT_DEINIT:
		break;
	case ISP3A_AF_EVT_EXIT:
		break;
	case ISP3A_AF_EVT_IOCTRL:
		break;
	case ISP3A_AF_EVT_PROCESS:
		break;
	default:
		ISP_LOGI("don't support msg");
		break;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_destroy_afl_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_thread_context                 *isp3a_thread_cxt;

	if (!isp_3a_handle) {
		ISP_LOGE("input is NULL");
		ret = ISP_ERROR;
		goto exit;
	}
	isp3a_thread_cxt = &cxt->thread_cxt;
	if (isp3a_thread_cxt->afl_thr_handle) {
		ret = cmr_thread_destroy(isp3a_thread_cxt->afl_thr_handle);
		if (!ret) {
			isp3a_thread_cxt->afl_thr_handle = (cmr_handle)NULL;
		} else {
			ISP_LOGE("failed to destroy afl thread");
		}
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_create_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

//	ret = isp3a_create_ctrl_thread(isp_3a_handle);
//	if (ret) {
//		goto exit;
//	}
	ret = isp3a_create_process_thread(isp_3a_handle);
//	if (ret) {
//		goto destroy_ctrl_thread;
//	}
//	ret = isp3a_create_receiver_thread(isp_3a_handle);
//	if (ret) {
//		goto destroy_process_thread;
//	}
//	ret = isp3a_create_af_thread(isp_3a_handle);
//	if (ret) {
//		goto destroy_process_thread;
//	}
//	ret = isp3a_create_afl_thread(isp_3a_handle);
//	if (ret) {
//		goto destroy_af_thread;
//	}
//	goto exit;
//destroy_af_thread:
//	ret = isp3a_destroy_af_thread(isp_3a_handle);
//destroy_process_thread:
//	ret = isp3a_destroy_process_thread(isp_3a_handle);
//destroy_ctrl_thread:
//	ret = isp3a_destroy_ctrl_thread(isp_3a_handle);
//exit:
	return ret;
}

cmr_int isp3a_destroy_thread(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;

	if (!isp_3a_handle) {
		ISP_LOGE("input is NULL");
		goto exit;
	}
//	isp3a_destroy_ctrl_thread(isp_3a_handle);
	ret = isp3a_destroy_process_thread(isp_3a_handle);
	if (ret)
		ISP_LOGE("destroy process thread fail %ld", ret);
//	isp3a_destroy_af_thread(isp_3a_handle);
//	isp3a_destroy_afl_thread(isp_3a_handle);
//	isp3a_destroy_receiver_thread(isp_3a_handle);
exit:
	return ret;
}

isp3a_ioctrl_fun isp3a_get_ioctrl_fun(enum isp_ctrl_cmd cmd)
{
	isp3a_ioctrl_fun                            io_ctrl = NULL;
	cmr_u32                                     total_num = ISP_CTRL_MAX;
	cmr_u32                                     i = 0;

	for (i = 0; i < total_num; i++) {
		if (cmd == s_isp3a_ioctrl_tab[i].cmd) {
			io_ctrl = s_isp3a_ioctrl_tab[i].ioctrl;
			break;
		}
	}
	return io_ctrl;
}

cmr_int isp3a_ioctrl(cmr_handle isp_3a_handle, enum isp_ctrl_cmd cmd, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	isp3a_ioctrl_fun                            ioctrl_fun = NULL;

	if (!isp_3a_handle) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ioctrl_fun = isp3a_get_ioctrl_fun(cmd);
	if (NULL != ioctrl_fun) {
		ret = ioctrl_fun(isp_3a_handle, param_ptr);
	} else {
		ISP_LOGI("ioctrl fun is NULL");
	}
exit:
	ISP_LOGV("done %ld", ret);
	return ret;
}

cmr_int isp3a_set_awb_mode(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	cmr_u32                                     input_wb_mode = *(cmr_u32*)param_ptr;
	union awb_ctrl_cmd_in                       awb_input;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}

	awb_input.wb_mode.manual_ct = 0;
	switch (input_wb_mode) {
	case ISP_AWB_INDEX0:
		break;
	case ISP_AWB_INDEX1:
		awb_input.wb_mode.wb_mode = AWB_CTRL_MWB_MODE_INCANDESCENT;
		break;
	case ISP_AWB_INDEX2:
		break;
	case ISP_AWB_INDEX3:
		break;
	case ISP_AWB_INDEX4:
		awb_input.wb_mode.wb_mode = AWB_CTRL_MWB_MODE_FLUORESCENT;
		break;
	case ISP_AWB_INDEX5:
		awb_input.wb_mode.wb_mode = AWB_CTRL_MWB_MODE_SUNNY;
		break;
	case ISP_AWB_INDEX6:
		awb_input.wb_mode.wb_mode = AWB_CTRL_MWB_MODE_CLOUDY;
		break;
	case ISP_AWB_INDEX7:
		break;
	case ISP_AWB_INDEX8:
		break;
	case ISP_AWB_AUTO:
		awb_input.wb_mode.wb_mode = AWB_CTRL_WB_MODE_AUTO;
		break;
	default:
		break;
	}
	ISP_LOGI("handle %p, set mode is %d", cxt->awb_cxt.handle, awb_input.wb_mode.wb_mode);
	ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WB_MODE, &awb_input, NULL);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_set_scene_mode(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.scene.scene_mode = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_SCENE_MODE, &ae_in, NULL);

exit:
	return ret;
}

cmr_int isp3a_set_measure_lum(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.measure_lum.lum_mode = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_MEASURE_LUM, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ev(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.exp_comp.level = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_EXP_COMP, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_flicker(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;
	struct ae_ctrl_param_out                    ae_out;
	struct afl_ctrl_param_in                    afl_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.flicker.flicker_mode = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FLICKER, &ae_in, NULL);

	if (AE_CTRL_FLICKER_AUTO == (*(cmr_u32*)param_ptr)) {
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_FLICKER_MODE, NULL, &ae_out);
		afl_in.mode.flicker_mode = ae_out.flicker_mode;
		ret = afl_ctrl_ioctrl(cxt->afl_cxt.handle, AFL_CTRL_SET_FLICKER, &afl_in, NULL);
	}

	afl_in.mode.flicker_mode = *(cmr_u32*)param_ptr;;
	ret = afl_ctrl_ioctrl(cxt->afl_cxt.handle, AFL_CTRL_SET_UI_FLICKER_MODE, &afl_in, NULL);

exit:
	return ret;
}

cmr_int isp3a_set_ae_awb_bypass(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_u32                                     type = 0xff;
	struct ae_ctrl_param_in                     ae_in;
	union awb_ctrl_cmd_in                       awb_in;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	type = *(cmr_u32*)param_ptr;
	switch (type) {
	case 0: /*ae awb normal*/
		ae_in.value = 0;
		awb_in.bypass = 0;
		ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_BYPASS, &ae_in, NULL);
		awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_BYPASS, &awb_in, NULL);
		break;
	case 1:
		break;
	case 2: /*ae by pass*/
		ae_in.value = 1;
		ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_BYPASS, &ae_in, NULL);
		break;
	case 3: /*awb by pass*/
		awb_in.bypass = 1;
		awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_BYPASS, &awb_in, NULL);
		break;
	default:
		break;
	}
exit:
	return ret;
}

cmr_int isp3a_set_effect(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	union isp_dev_ctrl_cmd_in                   dev_ioctrl_in;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	dev_ioctrl_in.value = *((cmr_u32*)param_ptr);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_SET_SPECIAL_EFFECT, &dev_ioctrl_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_brightness(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	union isp_dev_ctrl_cmd_in                   dev_ioctrl_in;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	dev_ioctrl_in.value = *((cmr_u32*)param_ptr);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_SET_BRIGHTNESS, &dev_ioctrl_in, NULL);

exit:
	return ret;
}

cmr_int isp3a_set_contrast(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	union isp_dev_ctrl_cmd_in                   dev_ioctrl_in;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	dev_ioctrl_in.value = *((cmr_u32*)param_ptr);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_SET_CONSTRACT, &dev_ioctrl_in, NULL);

exit:
	return ret;
}

cmr_int isp3a_start_af(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct af_ctrl_param_in                     af_in;
	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	af_in.af_ctrl_roi_info = *((struct isp_af_win *)param_ptr);
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CTRL_CMD_SET_AF_START, &af_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_saturation(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	union isp_dev_ctrl_cmd_in                   dev_ioctrl_in;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}

	dev_ioctrl_in.value = *((cmr_u32*)param_ptr);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_SET_SATURATION, &dev_ioctrl_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_af_mode(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct af_ctrl_param_in                     input_param;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	input_param.af_mode = *(cmr_int*)param_ptr;
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CTRL_CMD_SET_AF_MODE, &input_param, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_awb_flash_gain(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	return ret;
}

cmr_int isp3a_set_awb_flash_off_gain(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_awb_gain                         gain;
	cmr_u32                                     ct;
	union isp_dev_ctrl_cmd_in                  input_data;

	gain = cxt->awb_cxt.proc_out.gain_flash_off;
	ret = isp_dev_access_cfg_awb_gain(cxt->dev_access_handle, &gain);
	ct = cxt->awb_cxt.proc_out.ct_flash_off;
	input_data.value = ct;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_SET_COLOR_TEMP, &input_data, NULL);
	ISP_LOGI("set flash off gain %d %d %d,ct:%d", gain.r, gain.g, gain.b, ct);
	return ret;
}

cmr_int isp3a_set_awb_capture_gain(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_awb_gain                         gain;
	cmr_u32                                     ct;
	union isp_dev_ctrl_cmd_in                  input_data;

	gain = cxt->awb_cxt.proc_out.gain_capture;
	cxt->awb_cxt.proc_out.gain = cxt->awb_cxt.proc_out.gain_capture;
	ret = isp_dev_access_cfg_awb_gain(cxt->dev_access_handle, &gain);
	ct = cxt->awb_cxt.proc_out.ct_capture;
	input_data.value = ct;
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_SET_COLOR_TEMP, &input_data, NULL);
	ISP_LOGI("set capture gain %d %d %d,ct:%d", gain.r, gain.g, gain.b, ct);

	return ret;
}

cmr_int isp3a_notice_flash(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_flash_notice                     *isp_notice_ptr = NULL;
	struct ae_ctrl_param_in                     ae_in;
	union awb_ctrl_cmd_in                       awb_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	isp_notice_ptr = (struct isp_flash_notice*)param_ptr;
	ISP_LOGI("flash notice mode=%d", isp_notice_ptr->mode);
	switch (isp_notice_ptr->mode) {
	case ISP_FLASH_PRE_BEFORE:
		ae_in.flash_notice.led_info.power_0 = isp_notice_ptr->led_info.power_0;
		ae_in.flash_notice.led_info.power_1 = isp_notice_ptr->led_info.power_1;
		ae_in.flash_notice.led_info.led_tag = isp_notice_ptr->led_info.led_tag;
		ae_in.flash_notice.capture_skip_num = isp_notice_ptr->capture_skip_num;
		awb_in.flash_status = AWB_CTRL_FLASH_PRE_BEFORE;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_BEFORE_P, &awb_in, NULL);
		break;
	case ISP_FLASH_PRE_LIGHTING:
	{
		cmr_u32 ratio = isp_notice_ptr->flash_ratio;

		awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, NULL, NULL);

		ae_in.flash_notice.flash_ratio = ratio;
		awb_in.flash_status = AWB_CTRL_FLASH_PRE_LIGHTING;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_OPEN_P, &awb_in, NULL);
		ret = isp3a_set_awb_flash_gain(isp_3a_handle);
	}
		break;
	case ISP_FLASH_PRE_AFTER:
		isp3a_set_awb_flash_off_gain((cmr_handle)cxt);
		ae_in.flash_notice.will_capture = isp_notice_ptr->will_capture;
		awb_in.flash_status = AWB_CTRL_FLASH_PRE_AFTER;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_CLOSE, &awb_in, NULL);
		break;
	case ISP_FLASH_MAIN_BEFORE:
		awb_in.flash_status = AWB_CTRL_FLASH_MAIN_BEFORE;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_BEFORE_M, &awb_in, NULL);
		isp3a_set_awb_capture_gain(isp_3a_handle);
		break;
	case ISP_FLASH_MAIN_LIGHTING:
		awb_in.flash_status = AWB_CTRL_FLASH_MAIN_LIGHTING;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_OPEN_M, &awb_in, NULL);
		ret = isp3a_set_awb_flash_gain(isp_3a_handle);
		break;
	case ISP_FLASH_MAIN_AE_MEASURE:
		awb_in.flash_status = AWB_CTRL_FLASH_MAIN_MEASURE;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FLASH_STATUS, &awb_in, NULL);
		break;
	case ISP_FLASH_MAIN_AFTER_PRE:
		break;
	case ISP_FLASH_MAIN_AFTER:
		isp3a_set_awb_flash_off_gain(isp_3a_handle);
		awb_in.flash_status = AWB_CTRL_FLASH_MAIN_AFTER;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_FLASH_CLOSE, &awb_in, NULL);
		break;
	default:
		break;
	}
	ae_in.flash_notice.flash_mode = isp_notice_ptr->mode;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FLASH_NOTICE, &ae_in, NULL);

exit:
	return ret;
}

cmr_int isp3a_set_iso(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.iso.iso_mode = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_ISO, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_stop_af(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CTRL_CMD_SET_AF_STOP, NULL, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ae_touch(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;
	struct isp_pos_rect                         *rect_ptr = NULL;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	rect_ptr = (struct isp_pos_rect*)param_ptr;
	ae_in.touch_zone.zone.x = rect_ptr->start_x;
	ae_in.touch_zone.zone.y = rect_ptr->start_y;
	if ((cmr_s32)rect_ptr->end_x == rect_ptr->start_x)
		ae_in.touch_zone.zone.w = 0;
	else
		ae_in.touch_zone.zone.w = rect_ptr->end_x - rect_ptr->start_x + 1;

	if ((cmr_s32)rect_ptr->end_y == rect_ptr->start_y)
		ae_in.touch_zone.zone.h = 0;
	else
		ae_in.touch_zone.zone.h = rect_ptr->end_y - rect_ptr->start_y + 1;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_TOUCH_ZONE, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_sharpness(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	union isp_dev_ctrl_cmd_in                   dev_ioctrl_in;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	dev_ioctrl_in.value = *((cmr_u32*)param_ptr);
	ret = isp_dev_access_ioctl(cxt->dev_access_handle, ISP_DEV_ACCESS_SET_SHARPNESS, &dev_ioctrl_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_hdr(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}

	ae_in.soft_hdr_ev.level = *(cmr_s32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_HDR_EV, &ae_in, NULL);

exit:
	return ret;
}

cmr_int isp3a_set_face_area(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;
	union awb_ctrl_cmd_in                       awb_in;
	struct af_ctrl_param_in                     af_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.face_area = *(struct isp_face_area*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FD_PARAM, &ae_in, NULL);
	awb_in.face_info = *(struct isp_face_area*)param_ptr;
	ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_FACE_INFO, &awb_in, NULL);
	af_in.face_info = *(struct isp_face_area*)param_ptr;
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CTRL_CMD_SET_ROI, &af_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_start_3a(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
exit:
	return ret;
}

cmr_int isp3a_stop_3a(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
exit:
	return ret;
}

cmr_int isp3a_notice_snapshot(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
exit:
	return ret;
}

cmr_int isp3a_set_fps_range(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_range_fps                        *range_fps_ptr = NULL;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	range_fps_ptr = (struct isp_range_fps*)param_ptr;
	ae_in.range_fps.min_fps = range_fps_ptr->min_fps;
	ae_in.range_fps.max_fps = range_fps_ptr->max_fps;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FPS, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ae_fps(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_ae_fps                           *ae_fps_ptr = NULL;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_fps_ptr = (struct isp_ae_fps*)param_ptr;
	ae_in.range_fps.min_fps = ae_fps_ptr->min_fps;
	ae_in.range_fps.max_fps = ae_fps_ptr->max_fps;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FPS, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_notice_burst(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
exit:
	return ret;
}

cmr_int isp3a_get_info(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_info                             *info_ptr = (struct isp_info*)param_ptr;
	union awb_ctrl_cmd_out                      awb_out;
	struct ae_ctrl_param_out                    ae_out;
	struct afl_ctrl_param_out                   afl_out;
	struct af_ctrl_param_out                    af_out;
	cmr_u32                                     size = 0;
	struct otp_report_debug2                    *otp_info = NULL;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	info_ptr->size = 0;
	info_ptr->addr = NULL;

	memset(&awb_out, 0, sizeof(awb_out));
	memset(&ae_out, 0, sizeof(ae_out));
	memset(&afl_out, 0, sizeof(afl_out));
	memset(&af_out, 0, sizeof(af_out));
	if (0 == cxt->bin_cxt.is_write_to_debug_buf) {
		if (cxt->bin_cxt.bin_info.isp_3a_addr && cxt->bin_cxt.bin_info.isp_shading_addr) {
			size = MIN(MAX_BIN1_DEBUG_SIZE_STRUCT2, cxt->bin_cxt.bin_info.isp_3a_size);
			memcpy((void*)&cxt->debug_data.debug_info.bin_file1[0], cxt->bin_cxt.bin_info.isp_3a_addr, size);
			size = MIN(MAX_BIN2_DEBUG_SIZE_STRUCT2, cxt->bin_cxt.bin_info.isp_shading_size);
			memcpy((void*)&cxt->debug_data.debug_info.bin_file2, cxt->bin_cxt.bin_info.isp_shading_addr, size);
			cxt->bin_cxt.is_write_to_debug_buf = 1;
		}
	}
	strcpy((char*)&cxt->debug_data.debug_info.string2[0],"jpeg_str_g2v1");
	strcpy((char*)&cxt->debug_data.debug_info.end_string[0], "end_jpeg_str_g2v1");
	ret = isp_dev_access_get_debug_info(cxt->dev_access_handle, &cxt->debug_data.debug_info);
	ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_DEBUG_INFO, NULL, &awb_out);
	if (ret) {
		ISP_LOGE("failed to get awb debug info 0x%lx", ret);
		goto exit;
	}
	if (awb_out.debug_info.addr) {
		size = MIN(awb_out.debug_info.size, MAX_AWB_DEBUG_SIZE_STRUCT2);
		memcpy(&cxt->debug_data.debug_info.awb_debug_info2[0], awb_out.debug_info.addr, size);
		ISP_LOGV("awb debug size is %d", size);
	}
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_DEBUG_DATA, NULL, &ae_out);
	if (ret) {
		ISP_LOGE("failed to get ae debug info 0x%lx", ret);
		goto exit;
	}
	if (ae_out.debug_param.data) {
		size = MIN(ae_out.debug_param.size, MAX_AEFE_DEBUG_SIZE_STRUCT2);
		memcpy(cxt->debug_data.debug_info.ae_fe_debug_info2, ae_out.debug_param.data, size);
		ISP_LOGV("ae debug size is %d", size);
	}
	ret = afl_ctrl_ioctrl(cxt->afl_cxt.handle, AFL_CTRL_GET_DEBUG_DATA, NULL, &afl_out);
	if (ret) {
		ISP_LOGE("failed to get afl debug info 0x%lx", ret);
		goto exit;
	}
	if (afl_out.debug_param.data) {
		size = MIN(afl_out.debug_param.size, MAX_FLICKER_DEBUG_SIZE_STRUCT2);
		memcpy(cxt->debug_data.debug_info.flicker_debug_info2, afl_out.debug_param.data, size);
		ISP_LOGV("afl debug size is %d", size);
	}
	if (cxt->af_cxt.af_support) {
		ret = af_ctrl_ioctrl(cxt->af_cxt.handle,
				     AF_CTRL_CMD_GET_DEBUG_INFO,
				     NULL, (void *)&af_out.debug_info);
		if (ret) {
			ISP_LOGE("failed to get af debug info 0x%lx", ret);
			goto exit;
		}
		if (af_out.debug_info.addr) {
			size = MIN(MAX_AF_DEBUG_SIZE_STRUCT2, af_out.debug_info.size);
			memcpy(cxt->debug_data.debug_info.af_debug_info2, af_out.debug_info.addr, size);
			ISP_LOGI("af debug size is %d", size);
		}
	}
	otp_info = &cxt->debug_data.debug_info.otp_report_debug_info2;
	memset(otp_info, 0, sizeof(struct otp_report_debug2));
	if (cxt->otp_data) {
		otp_info->current_module_calistatus = cxt->otp_data->program_flag;
		otp_info->current_module_year = cxt->otp_data->module_info.year;
		otp_info->current_module_month = cxt->otp_data->module_info.month;
		otp_info->current_module_day = cxt->otp_data->module_info.day;
		otp_info->current_module_mid = cxt->otp_data->module_info.mid;
		otp_info->current_module_lens_id = cxt->otp_data->module_info.lens_id;
		otp_info->current_module_vcm_id = cxt->otp_data->module_info.vcm_id;
		otp_info->current_module_driver_ic = cxt->otp_data->module_info.driver_ic_id;
		otp_info->current_module_iso = cxt->otp_data->isp_awb_info.iso;
		otp_info->current_module_r_gain = cxt->otp_data->isp_awb_info.gain_r;
		otp_info->current_module_g_gain = cxt->otp_data->isp_awb_info.gain_g;
		otp_info->current_module_b_gain = cxt->otp_data->isp_awb_info.gain_b;
		if (cxt->otp_data->lsc_info.lsc_data_addr) {
			size = MIN(cxt->otp_data->lsc_info.lsc_data_size, OTP_LSC_DATA_SIZE);
			memcpy(&otp_info->current_module_lsc[0], cxt->otp_data->lsc_info.lsc_data_addr, size);
		}
		otp_info->current_module_af_flag = cxt->otp_data->af_info.flag;
		otp_info->current_module_infinity = cxt->otp_data->af_info.infinite_cali;
		otp_info->current_module_macro = cxt->otp_data->af_info.macro_cali;
		otp_info->total_check_sum = cxt->otp_data->checksum;
	} else if (cxt->bin_cxt.bin_info.otp_data_addr) {
		otp_info->current_module_iso = cxt->bin_cxt.bin_info.otp_data_addr->iso;
		otp_info->current_module_r_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_r;
		otp_info->current_module_g_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_g;
		otp_info->current_module_b_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_b;
		if (cxt->bin_cxt.bin_info.lsc_otp_addr) {
			memcpy(&otp_info->current_module_lsc[0], cxt->bin_cxt.bin_info.lsc_otp_addr, OTP_LSC_DATA_SIZE);
		}
	}
	cxt->debug_data.debug_info.structure_size2 = sizeof(struct debug_info2);
	info_ptr->size = sizeof(struct debug_info2);
	info_ptr->addr = (void*)&cxt->debug_data.debug_info;
exit:
	ISP_LOGI("debug info size %ld", info_ptr->size);
	return ret;
}

cmr_int isp3a_set_ae_night_mode(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct ae_ctrl_param_in                     ae_in;
	cmr_u32                                     night_mode;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}

exit:
	return ret;
}

cmr_int isp3a_set_ae_awb_lock(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_u32                                     mode;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	union awb_ctrl_cmd_in                       awb_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	mode = *(cmr_u32*)param_ptr;

	if (ISP_AE_AWB_LOCK == mode) {
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_PAUSE, NULL, NULL);
		awb_in.lock_param.lock_flag = 1;
		awb_in.lock_param.ct = 0;
		awb_in.lock_param.wbgain.r = 0;
		awb_in.lock_param.wbgain.g = 0;
		awb_in.lock_param.wbgain.b = 0;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, &awb_in, NULL);
	} else if (ISP_AE_AWB_UNLOCK == mode) {
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_RESTORE, NULL, NULL);
		awb_in.lock_param.lock_flag = 0;
		awb_in.lock_param.ct = 0;
		awb_in.lock_param.wbgain.r = 0;
		awb_in.lock_param.wbgain.g = 0;
		awb_in.lock_param.wbgain.b = 0;
		ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_UNLOCK, &awb_in, NULL);
	} else {
		ISP_LOGI("don't support %d", mode);
	}
exit:
	return ret;
}

cmr_int isp3a_set_ae_lock(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_u32                                     ae_mode;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_mode = *(cmr_u32*)param_ptr;
	if (ISP_AE_LOCK == ae_mode) {
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_PAUSE, NULL, NULL);
	} else if (ISP_AE_UNLOCK == ae_mode) {
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_RESTORE, NULL, NULL);
	} else {
		ISP_LOGI("don't support %d", ae_mode);
	}
exit:
	return ret;
}

cmr_int isp3a_set_ae_mode(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                    *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                    ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.value = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_BYPASS, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ae_fix_exposure_time(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                    *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                    ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.value = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FIX_EXPOSURE_TIME, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ae_fix_sensitivity(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                    *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                    ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.value = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FIX_SENSITIVITY, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ae_fix_frame_duration(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                    *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                    ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.value = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FIX_FRAME_DURATION, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ae_manual_exptime(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                    *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                    ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.value = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_MANUAL_EXPTIME, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_ae_manual_gain(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                    *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                    ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.value = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_MANUAL_GAIN, &ae_in, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_dzoom(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	union awb_ctrl_cmd_in                       input;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	input.dzoom_factor = *(float*)param_ptr;
	ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_DZOOM_FACTOR, &input, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_convergence_req(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	UNUSED(param_ptr);
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_CONVERGENCE_REQ, NULL, NULL);
exit:
	return ret;
}

cmr_int isp3a_set_snapshot_finished(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	UNUSED(param_ptr);
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_SNAPSHOT_FINISHED, NULL, NULL);
exit:
	return ret;
}

cmr_int isp3a_get_exif_debug_info(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_info                             *exif_info_ptr = (struct isp_info*)param_ptr;
	union awb_ctrl_cmd_out                      awb_out;
	struct ae_ctrl_param_out                    ae_out;
	struct afl_ctrl_param_out                   afl_out;
	struct af_ctrl_param_out                    af_out;
	cmr_u32                                      size = 0;
	struct debug_info1                          *exif_ptr = &cxt->debug_data.exif_debug_info;
	float                                       libVersion;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	exif_info_ptr->size = 0;
	exif_info_ptr->addr = NULL;

	memset(&awb_out, 0, sizeof(awb_out));
	memset(&ae_out, 0, sizeof(ae_out));
	memset(&afl_out, 0, sizeof(afl_out));
	memset(&af_out, 0, sizeof(af_out));

	strcpy((char*)&exif_ptr->string1[0], "exif_str_g2v1");
	strcpy((char*)&exif_ptr->end_string[0],"end_exif_str_g2v1");
	exif_ptr->other_debug_info1.focal_length = cxt->ex_info.focal_length;
	strcpy((char*)&exif_ptr->project_name[0], "whale2");
	exif_ptr->struct_version = DEBUG_STRUCT_VERSION;
//	exif_ptr->isp_ver_major =       //from isp drv
//	exif_ptr->isp_ver_minor =
	if (ERR_3ALIB_VER_SUCCESS == allib_3a_getversion(&libVersion)) {
		exif_ptr->main_ver_major = (uint16)libVersion;
		exif_ptr->main_ver_minor = (libVersion - exif_ptr->main_ver_major) * 10000;
	}
	memset(&exif_ptr->otp_report_debug_info1, 0, sizeof(struct otp_report_debug1));
	if (cxt->otp_data) {
		exif_ptr->otp_report_debug_info1.current_module_calistatus = cxt->otp_data->program_flag;
		exif_ptr->otp_report_debug_info1.current_module_year = cxt->otp_data->module_info.year;
		exif_ptr->otp_report_debug_info1.current_module_month = cxt->otp_data->module_info.month;
		exif_ptr->otp_report_debug_info1.current_module_day = cxt->otp_data->module_info.day;
		exif_ptr->otp_report_debug_info1.current_module_mid = cxt->otp_data->module_info.mid;
		exif_ptr->otp_report_debug_info1.current_module_lens_id = cxt->otp_data->module_info.lens_id;
		exif_ptr->otp_report_debug_info1.current_module_vcm_id = cxt->otp_data->module_info.vcm_id;
		exif_ptr->otp_report_debug_info1.current_module_driver_ic = cxt->otp_data->module_info.driver_ic_id;
		exif_ptr->otp_report_debug_info1.current_module_iso = cxt->otp_data->isp_awb_info.iso;
		exif_ptr->otp_report_debug_info1.current_module_r_gain = cxt->otp_data->isp_awb_info.gain_r;
		exif_ptr->otp_report_debug_info1.current_module_g_gain = cxt->otp_data->isp_awb_info.gain_g;
		exif_ptr->otp_report_debug_info1.current_module_b_gain = cxt->otp_data->isp_awb_info.gain_b;
		exif_ptr->otp_report_debug_info1.current_module_af_flag = cxt->otp_data->af_info.flag;
		exif_ptr->otp_report_debug_info1.current_module_infinity = cxt->otp_data->af_info.infinite_cali;
		exif_ptr->otp_report_debug_info1.current_module_macro = cxt->otp_data->af_info.macro_cali;
		exif_ptr->otp_report_debug_info1.total_check_sum = cxt->otp_data->checksum;
	} else if (cxt->bin_cxt.bin_info.otp_data_addr) {
		exif_ptr->otp_report_debug_info1.current_module_iso = cxt->bin_cxt.bin_info.otp_data_addr->iso;
		exif_ptr->otp_report_debug_info1.current_module_r_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_r;
		exif_ptr->otp_report_debug_info1.current_module_g_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_g;
		exif_ptr->otp_report_debug_info1.current_module_b_gain = cxt->bin_cxt.bin_info.otp_data_addr->gain_b;
	}

	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_EXT_DEBUG_INFO, NULL, &ae_out);
	if (ret) {
		ISP_LOGE("failed to get ae ext info 0x%lx", ret);
	}
	exif_ptr->other_debug_info1.flash_flag = ae_out.debug_info.flash_flag;
	exif_ptr->other_debug_info1.fn_value = ae_out.debug_info.fn_value;
	exif_ptr->other_debug_info1.valid_ad_gain = ae_out.debug_info.valid_ad_gain;
	exif_ptr->other_debug_info1.valid_exposure_line = ae_out.debug_info.valid_exposure_line;
	exif_ptr->other_debug_info1.valid_exposure_time = ae_out.debug_info.valid_exposure_time;
	ret = isp_dev_access_get_exif_debug_info(cxt->dev_access_handle, exif_ptr);
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_EXIF_DATA, NULL, &ae_out);
	if (ret) {
		ISP_LOGE("failed to get ae exif info 0x%lx", ret);
	}
	if (ae_out.exif_param.data) {
		size = MIN(ae_out.exif_param.size, MAX_AEFE_DEBUG_SIZE_STRUCT1);
		memcpy((void*)&exif_ptr->ae_fe_debug_info1[0], (void*)ae_out.exif_param.data, size);
		ISP_LOGV("ae exif debug size is %d", size);
	}

	ret = afl_ctrl_ioctrl(cxt->afl_cxt.handle, AFL_CTRL_GET_EXIF_DATA, NULL, &afl_out);
	if (ret) {
		ISP_LOGE("failed to get afl exif info 0x%lx", ret);
	}
	if (afl_out.exif_param.data) {
		size = MIN(afl_out.exif_param.size, MAX_FLICKER_DEBUG_SIZE_STRUCT1);
		memcpy((void*)&exif_ptr->flicker_debug_info1[0], (void*)afl_out.exif_param.data, size);
		ISP_LOGV("afl exif debug size is %d", size);
	}

	ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_GET_EXIF_DEBUG_INFO, NULL, &awb_out);
	if (ret) {
		ISP_LOGE("failed to get awb exif info 0x%lx", ret);
	}
	if (awb_out.debug_info.addr) {
		size = MIN(awb_out.debug_info.size, MAX_AEFE_DEBUG_SIZE_STRUCT1);
		memcpy((void*)&exif_ptr->awb_debug_info1[0], awb_out.debug_info.addr, size);
		ISP_LOGV("awb exif debug size is %d", size);
	}

	if (cxt->af_cxt.af_support) {
		ret= af_ctrl_ioctrl(cxt->af_cxt.handle,
				    AF_CTRL_CMD_GET_EXIF_DEBUG_INFO,
				    NULL, (void *)&af_out.exif_info);
		if (ret) {
			ISP_LOGE("failed to get af exif info 0x%lx", ret);
		}
		if (af_out.exif_info.addr) {
			size = MIN(MAX_AF_DEBUG_SIZE_STRUCT1, af_out.exif_info.size);
			memcpy((void*)&exif_ptr->af_debug_info1[0], af_out.exif_info.addr, size);
			ISP_LOGV("af exif debug size is %d", size);
		}
	}
	cxt->debug_data.exif_debug_info.structure_size1 = sizeof(cxt->debug_data.exif_debug_info);
	exif_info_ptr->size = sizeof(cxt->debug_data.exif_debug_info);
	exif_info_ptr->addr = (void*)&cxt->debug_data.exif_debug_info;
exit:
	ISP_LOGI("exif debug info size %ld", exif_info_ptr->size);

	return ret;
}

cmr_int isp3a_get_adgain_exp_info(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_adgain_exp_info                  *info_ptr = (struct isp_adgain_exp_info*)param_ptr;
	struct ae_ctrl_param_out                    ae_out;

	ae_out.exp_gain.exposure_line = 0;
	ae_out.exp_gain.exposure_time = 0;
	ae_out.exp_gain.gain = 0;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_EXP_GAIN, NULL, &ae_out);
	if (!ret) {
		info_ptr->adgain = ae_out.exp_gain.gain;
		info_ptr->exp_time = ae_out.exp_gain.exposure_time;
	}
	ISP_LOGI("adgain = %d, exp = %d", info_ptr->adgain, info_ptr->exp_time);
	return ret;
}

static cmr_int isp3a_set_flash_mode(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	ae_in.flash.flash_mode = *(cmr_u32*)param_ptr;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_FLASH_MODE, &ae_in, NULL);
exit:
	return ret;
}

static cmr_int isp3a_set_aux_sensor_info(cmr_handle isp_3a_handle, void *sensor_info)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle,
			     AF_CTRL_CMD_SET_UPDATE_AUX_SENSOR,
			     (void *)sensor_info, NULL);

	return ret;
}

cmr_int isp3a_init_statistics_buf(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_u32                                     i = 0;
	void                                        *ae_buf_addr = NULL;
	void                                        *awb_buf_addr = NULL;
	void                                        *af_buf_addr = NULL;
	void                                        *afl_buf_addr = NULL;
	void                                        *yhis_buf_addr = NULL;
	void                                        *subsample_buf_addr = NULL;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_statistics_data                *buf_head = NULL;
	struct stats_buf_size_list stats_buf_size = { 0x00 };

	sem_wait(&cxt->statistics_data_sm);
	isp_get_stats_size(&stats_buf_size);
	ae_buf_addr = malloc(ISP3A_STATISTICS_BUF_NUM * stats_buf_size.ae_stats_size);
	awb_buf_addr = malloc(ISP3A_STATISTICS_BUF_NUM * stats_buf_size.awb_stats_size);
	af_buf_addr = malloc(ISP3A_STATISTICS_BUF_NUM * stats_buf_size.af_stats_size);
	afl_buf_addr = malloc(ISP3A_STATISTICS_AFL_BUF_NUM * stats_buf_size.antif_stats_size);
	subsample_buf_addr = malloc(ISP3A_STATISTICS_BUF_NUM * stats_buf_size.subimg_stats_size);
	yhis_buf_addr = malloc(ISP3A_STATISTICS_BUF_NUM * stats_buf_size.yhist_stats_size);
	if (!ae_buf_addr || !awb_buf_addr || !af_buf_addr
			|| !afl_buf_addr || !subsample_buf_addr
			|| !yhis_buf_addr) {
		ISP_LOGE("failed to malloc stats");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}

	/* init AE stats buf */
	buf_head = &cxt->ae_cxt.statistics_buffer[0];
	for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
		cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		buf_head[i].size = stats_buf_size.ae_stats_size;
		buf_head[i].addr = (void*)((cmr_int)ae_buf_addr + i * stats_buf_size.ae_stats_size);
		ISP_LOGV("ae: i=%d size=%d addr = %lx", i, buf_head[i].size, (cmr_int)buf_head[i].addr);
	}

	/* init AWB stats buf */
	buf_head = &cxt->awb_cxt.statistics_buffer[0];
	for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
		cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		buf_head[i].size = stats_buf_size.awb_stats_size;
		buf_head[i].addr = (void*)((cmr_int)awb_buf_addr + i * stats_buf_size.awb_stats_size);
		ISP_LOGV("awb: i=%d size=%d addr = %lx", i, buf_head[i].size, (cmr_int)buf_head[i].addr);
	}

	/* init AF stats buf */
	buf_head = &cxt->af_cxt.statistics_buffer[0];
	for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
		cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		buf_head[i].size = stats_buf_size.af_stats_size;
		buf_head[i].addr = (void*)((cmr_int)af_buf_addr + i * stats_buf_size.af_stats_size);
		ISP_LOGV("af: i=%d size=%d addr = %lx", i, buf_head[i].size, (cmr_int)buf_head[i].addr);
	}

	/* init AFL stats buf */
	buf_head = &cxt->afl_cxt.statistics_buffer[0];
	for (i=0 ; i<ISP3A_STATISTICS_AFL_BUF_NUM ; i++) {
		cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		buf_head[i].size = stats_buf_size.antif_stats_size;
		buf_head[i].addr = (void*)((cmr_int)afl_buf_addr + i * stats_buf_size.antif_stats_size);
		ISP_LOGV("afl: i=%d size=%d addr = %lx", i, buf_head[i].size, (cmr_int)buf_head[i].addr);
	}

	/* init subsample stats buf */
	buf_head = &cxt->other_stats_cxt.sub_sample_buffer[0];
	for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
		cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		buf_head[i].size = stats_buf_size.subimg_stats_size;
		buf_head[i].addr = (void*)((cmr_int)subsample_buf_addr + i * stats_buf_size.subimg_stats_size);
		ISP_LOGV("sub sample: i=%d size=%d addr = %lx", i, buf_head[i].size, (cmr_int)buf_head[i].addr);
	}

	/* init yhis stats buf */
	buf_head = &cxt->other_stats_cxt.yhis_buffer[0];
	for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
		cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		buf_head[i].size = stats_buf_size.yhist_stats_size;
		buf_head[i].addr = (void*)((cmr_int)yhis_buf_addr + i * stats_buf_size.yhist_stats_size);
	}

exit:
	if (ret) {
		if (ae_buf_addr) {
			free(ae_buf_addr);
		}
		if(awb_buf_addr) {
			free(awb_buf_addr);
		}
		if (af_buf_addr) {
			free(af_buf_addr);
		}
		if (afl_buf_addr) {
			free(afl_buf_addr);
		}
		if (subsample_buf_addr) {
			free(subsample_buf_addr);
		}
		if (yhis_buf_addr) {
			free(yhis_buf_addr);
		}
	}
	sem_post(&cxt->statistics_data_sm);
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_deinit_statistics_buf(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_u32                                     i = 0;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_statistics_data                *buf_head = NULL;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);
	sem_wait(&cxt->statistics_data_sm);
	buf_head = &cxt->ae_cxt.statistics_buffer[0];
	if (buf_head[0].addr) {
		free(buf_head[0].addr);
		for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
			cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		}
	}

	buf_head = &cxt->awb_cxt.statistics_buffer[0];
	if (buf_head[0].addr) {
		free(buf_head[0].addr);
		for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
			cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		}
	}

	buf_head = &cxt->af_cxt.statistics_buffer[0];
	if (buf_head[0].addr) {
		free(buf_head[0].addr);
		for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
			cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		}
	}

	buf_head = &cxt->afl_cxt.statistics_buffer[0];
	if (buf_head[0].addr) {
		free(buf_head[0].addr);
		for (i=0 ; i<ISP3A_STATISTICS_AFL_BUF_NUM ; i++) {
			cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		}
	}

	buf_head = &cxt->other_stats_cxt.sub_sample_buffer[0];
	if (buf_head[0].addr) {
		free(buf_head[0].addr);
		for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
			cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		}
	}
	buf_head = &cxt->other_stats_cxt.yhis_buffer[0];
	if (buf_head[0].addr) {
		free(buf_head[0].addr);
		for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
			cmr_bzero((void*)&buf_head[i], sizeof(struct isp3a_statistics_data));
		}
	}
	sem_post(&cxt->statistics_data_sm);
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_get_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data **buf_ptr)
{
	cmr_int                                     ret = ISP_ERROR;
	cmr_int                                     is_get = 0;
	cmr_u32                                     i = 0;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_statistics_data                *buf_head = NULL;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	if (!buf_ptr) {
		ISP_LOGW("input is NULL");
		i = 0xFF;
		goto exit;
	}

	if (type >= ISP3A_STATICS_TYPE_MAX) {
		ISP_LOGE("input type error %ld", type);
		*buf_ptr = (struct isp3a_statistics_data *)NULL;
		i = 0xFF;
		goto exit;
	}
	switch (type) {
	case ISP3A_AE:
		buf_head = &cxt->ae_cxt.statistics_buffer[0];
		break;
	case ISP3A_AWB:
		buf_head = &cxt->awb_cxt.statistics_buffer[0];
		break;
	case ISP3A_AF:
		buf_head = &cxt->af_cxt.statistics_buffer[0];
		break;
	case ISP3A_AFL:
		buf_head = &cxt->afl_cxt.statistics_buffer[0];
		break;
	case ISP3A_YHIS:
		buf_head = &cxt->other_stats_cxt.yhis_buffer[0];
		break;
	case ISP3A_SUB_SAMPLE:
		buf_head = &cxt->other_stats_cxt.sub_sample_buffer[0];
		break;
	default:
		*buf_ptr = NULL;
		i = 0xFF;
		goto exit;
		break;
	}
	sem_wait(&cxt->statistics_data_sm);
	*buf_ptr = NULL;
	if (ISP3A_AFL != type) {
		for (i=0 ; i<ISP3A_STATISTICS_BUF_NUM ; i++) {
			if (0 == buf_head[i].used_flag) {
				*buf_ptr = &buf_head[i];
				buf_head[i].used_flag = 1;
				is_get = 1;
				break;
			}
		}
	} else {
		for (i=0 ; i<ISP3A_STATISTICS_AFL_BUF_NUM ; i++) {
			if (0 == buf_head[i].used_flag) {
				*buf_ptr = &buf_head[i];
				buf_head[i].used_flag = 1;
				is_get = 1;
				break;
			}
		}
	}
	if (0 == is_get) {
		ISP_LOGE("failed to get statistics buf ,type is %ld", type);
	} else {
		ret = ISP_SUCCESS;
	}
	sem_post(&cxt->statistics_data_sm);
exit:
	ISP_LOGI("type %ld, index %d", type, i);
	return ret;
}

cmr_int isp3a_put_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data *buf_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	cmr_u32                                     i = 0;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_statistics_data                *buf_head = NULL;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	if (!buf_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	if (type >= ISP3A_STATICS_TYPE_MAX) {
		ISP_LOGE("input type error %ld", type);
		goto exit;
	}
	sem_wait(&cxt->statistics_data_sm);
	buf_ptr->used_flag = 0;
	buf_ptr->used_num = 0;
	sem_post(&cxt->statistics_data_sm);
exit:
	return ret;
}

cmr_int isp3a_hold_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data *buf_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	UNUSED(type);
	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	if (!buf_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}
	sem_wait(&cxt->statistics_data_sm);
	if (1 == buf_ptr->used_flag) {
		buf_ptr->used_num++;
		ISP_LOGI("used num %d", buf_ptr->used_num);
	}

	sem_post(&cxt->statistics_data_sm);
exit:
	return ret;
}

cmr_int isp3a_release_statistics_buf(cmr_handle isp_3a_handle, cmr_int type, struct isp3a_statistics_data *buf_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	UNUSED(type);
	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	if (!buf_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}

	sem_wait(&cxt->statistics_data_sm);
	if (1 == buf_ptr->used_flag) {
		if (buf_ptr->used_num > 0) {
			buf_ptr->used_num--;
		}
		if (0 == buf_ptr->used_num) {
			buf_ptr->used_flag = 0;
		} else {
			ISP_LOGI("used num %d", buf_ptr->used_num);
		}
	} else {
		ISP_LOGI("release don't been used statics buffer");
	}
	sem_post(&cxt->statistics_data_sm);
exit:
	return ret;
}


cmr_int isp3a_callback_func (cmr_handle handle, cmr_u32 cb_type, void *param_ptr, cmr_u32 param_len)
{
	cmr_int                                     ret = ISP_SUCCESS;

	UNUSED(handle);
	UNUSED(cb_type);
	UNUSED(param_ptr);
	UNUSED(param_len);
	return ret;
}

void isp3a_dev_evt_cb(cmr_int evt, void *data, cmr_u32 data_len, void *privdata)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)privdata;
	void                                        *data_buffer = NULL;
	CMR_MSG_INIT(message);

	if (!cxt || !data) {
		ISP_LOGE("handle is NULL");
		return;
	}
	switch (evt) {
	case ISP_DRV_STATISTICE:
		message.msg_type = ISP3A_PROC_EVT_STATS;
		break;
	case ISP_DRV_SENSOR_SOF:
		message.msg_type = ISP3A_PROC_EVT_SENSOR_SOF;
		break;
	default:
		ISP_LOGI("don't support evt %ld", evt);
		goto err_handle;
	}
	data_buffer = malloc(data_len);
	if (NULL == data_buffer) {
		ret = ISP_ALLOC_ERROR;
		goto err_handle;
	}
	memcpy(data_buffer, data, data_len);
	message.sub_msg_type = 0;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = data_buffer;
	ret = cmr_thread_msg_send(cxt->thread_cxt.process_thr_handle, &message);
err_handle:
	if (ret) {
		ISP_LOGE("failed to send a message, evt is %ld", evt);
		if (ISP_DRV_STATISTICE == evt) {
			struct isp_statis_buf statis_buf;
			struct isp_statis_frame_output *input_buf_ptr = (struct isp_statis_frame_output*)data;
			statis_buf.buf_size = input_buf_ptr->buf_size;
			statis_buf.phy_addr = input_buf_ptr->phy_addr;
			statis_buf.vir_addr = input_buf_ptr->vir_addr;

			ret = isp_dev_access_set_stats_buf(cxt->dev_access_handle, &statis_buf);
			if (ret) {
				ISP_LOGE("failed to set statis buf");
			}
		} else {
			ISP_LOGE("failed to send sensor sof msg");
		}
	}
}

cmr_int isp3a_get_3a_stats_buf(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	cxt->stats_buf_cxt.ae_stats_buf_ptr = NULL;
	cxt->stats_buf_cxt.awb_stats_buf_ptr = NULL;
	cxt->stats_buf_cxt.af_stats_buf_ptr = NULL;
	cxt->stats_buf_cxt.afl_stats_buf_ptr = NULL;
	cxt->stats_buf_cxt.yhis_stats_buf_ptr = NULL;

	ret = isp3a_get_statistics_buf(isp_3a_handle, ISP3A_AE, &cxt->stats_buf_cxt.ae_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to get ae stats buf");
		goto exit;
	}

	ret = isp3a_get_statistics_buf(isp_3a_handle, ISP3A_AWB, &cxt->stats_buf_cxt.awb_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to get awb stats buf");
		goto exit;
	}
	ret = isp3a_get_statistics_buf(isp_3a_handle, ISP3A_AF, &cxt->stats_buf_cxt.af_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to get af stats buf");
		goto exit;
	}
	ret = isp3a_get_statistics_buf(isp_3a_handle, ISP3A_AFL, &cxt->stats_buf_cxt.afl_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to get afl stats buf");
		goto exit;
	}
	ret = isp3a_get_statistics_buf(isp_3a_handle, ISP3A_YHIS, &cxt->stats_buf_cxt.yhis_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to get yhis stats buf");
		goto exit;
	}
/*
	ret = isp3a_get_statistics_buf(isp_3a_handle, ISP3A_SUB_SAMPLE, &cxt->stats_buf_cxt.subsample_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to get ae stats buf");
		goto exit;
	}*/
	goto normal_exit;
exit:
	if (cxt->stats_buf_cxt.ae_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AE, cxt->stats_buf_cxt.ae_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.awb_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AWB, cxt->stats_buf_cxt.awb_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.af_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AF, cxt->stats_buf_cxt.af_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.afl_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AFL, cxt->stats_buf_cxt.afl_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.yhis_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_YHIS, cxt->stats_buf_cxt.yhis_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.subsample_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_SUB_SAMPLE, cxt->stats_buf_cxt.subsample_stats_buf_ptr);
	}
normal_exit:
	ISP_LOGI("done");
	return ret;
}

cmr_int isp3a_put_3a_stats_buf(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (cxt->stats_buf_cxt.ae_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AE, cxt->stats_buf_cxt.ae_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.awb_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AWB, cxt->stats_buf_cxt.awb_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.af_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AF, cxt->stats_buf_cxt.af_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.afl_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_AFL, cxt->stats_buf_cxt.afl_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.yhis_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_YHIS, cxt->stats_buf_cxt.yhis_stats_buf_ptr);
	}
	if (cxt->stats_buf_cxt.subsample_stats_buf_ptr) {
		isp3a_put_statistics_buf(isp_3a_handle, ISP3A_SUB_SAMPLE, cxt->stats_buf_cxt.subsample_stats_buf_ptr);
	}
	ISP_LOGI("done");
	return ret;
}


cmr_int isp3a_handle_stats(cmr_handle isp_3a_handle, void *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_statis_buf                       statis_buf;
	struct isp_statis_frame_output              *input_buf_ptr = (struct isp_statis_frame_output*)data;
	struct isp3a_statistics_data                *ae_stats_buf_ptr = NULL;
	struct isp3a_statistics_data                *awb_stats_buf_ptr = NULL;
	struct isp3a_statistics_data                *af_stats_buf_ptr = NULL;
	struct isp3a_statistics_data                *afl_stats_buf_ptr = NULL;
	struct isp3a_statistics_data                *yhis_stats_buf_ptr = NULL;
	struct isp_statis_frame_output              *dev_stats = (struct isp_statis_frame_output*)data;
	cmr_u32                                     is_set_stats_buf = 0;
	cmr_int                                     isp_stats;
	/* get buffer */
	ret = isp3a_get_3a_stats_buf(isp_3a_handle);
	if (ret) {
		ISP_LOGE("failed to get stats buf");
		/* free stats buf to isp drv */
		statis_buf.buf_size = input_buf_ptr->buf_size;
		statis_buf.phy_addr = input_buf_ptr->phy_addr;
		statis_buf.vir_addr = input_buf_ptr->vir_addr;

		ret = isp_dev_access_set_stats_buf(cxt->dev_access_handle, &statis_buf);
		if (ret) {
			ISP_LOGE("failed to set statis buf");
		}
		is_set_stats_buf = 1;
		goto exit;
	}
	/* dispatch stats information */
	ae_stats_buf_ptr = cxt->stats_buf_cxt.ae_stats_buf_ptr;
	awb_stats_buf_ptr = cxt->stats_buf_cxt.awb_stats_buf_ptr;
	af_stats_buf_ptr = cxt->stats_buf_cxt.af_stats_buf_ptr;
	afl_stats_buf_ptr = cxt->stats_buf_cxt.afl_stats_buf_ptr;
	yhis_stats_buf_ptr = cxt->stats_buf_cxt.yhis_stats_buf_ptr;
	af_stats_buf_ptr->timestamp.sec = dev_stats->time_stamp.sec;
	af_stats_buf_ptr->timestamp.usec = dev_stats->time_stamp.usec;
	ae_stats_buf_ptr->timestamp = af_stats_buf_ptr->timestamp;
	awb_stats_buf_ptr->timestamp = af_stats_buf_ptr->timestamp;
	isp_stats = (cmr_int)dev_stats->vir_addr;
	ret = isp_dispatch_stats((void*)isp_stats, ae_stats_buf_ptr->addr, awb_stats_buf_ptr->addr, af_stats_buf_ptr->addr,
							 yhis_stats_buf_ptr->addr, afl_stats_buf_ptr->addr, NULL, cxt->sof_idx);
	if (ret) {
		ret = isp3a_put_3a_stats_buf(isp_3a_handle);
		goto exit;
	}
	/* free stats buf to isp drv */
	statis_buf.buf_size = input_buf_ptr->buf_size;
	statis_buf.phy_addr = input_buf_ptr->phy_addr;
	statis_buf.vir_addr = input_buf_ptr->vir_addr;

	ret = isp_dev_access_set_stats_buf(cxt->dev_access_handle, &statis_buf);
	if (ret) {
		ISP_LOGE("failed to set statis buf");
	}
	is_set_stats_buf = 1;

	/* start YHIS process */
	ret= isp3a_start_yhis_process(isp_3a_handle, yhis_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to start afl process");
	}

	/* start AE process */
	ret= isp3a_start_ae_process(isp_3a_handle, ae_stats_buf_ptr, awb_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to start ae process");
	}
	/* start AF process */
	ret= isp3a_start_af_process(isp_3a_handle, af_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to start af process");
	}
	/* start AFl process */
	ret= isp3a_start_afl_process(isp_3a_handle, afl_stats_buf_ptr);
	if (ret) {
		ISP_LOGE("failed to start afl process");
	}

exit:
	if (0 == is_set_stats_buf) {
		/* free stats buf to isp drv */
		statis_buf.buf_size = input_buf_ptr->buf_size;
		statis_buf.phy_addr = input_buf_ptr->phy_addr;
		statis_buf.vir_addr = input_buf_ptr->vir_addr;
		ret = isp_dev_access_set_stats_buf(cxt->dev_access_handle, &statis_buf);
		if (ret) {
			ISP_LOGE("failed to set statis buf");
		}
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp3a_handle_sensor_sof(cmr_handle isp_3a_handle, void *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	union awb_ctrl_cmd_in                       awb_in;
	union awb_ctrl_cmd_out                      awb_out;
	struct af_ctrl_param_in                     af_in;
	struct ae_ctrl_param_in                     ae_in;
	struct ae_ctrl_param_out                    ae_out;
	struct isp_irq                              *sof_info = (struct isp_irq*)data;
	struct isp_sof_cfg_info                     sof_cfg_info;

	if (NULL == cxt) {
		ISP_LOGE("error cxt NULL");
		ret = -ISP_PARAM_NULL;
		return ret;
	}

	cxt->sof_idx = sof_info->reserved + 1;
	af_in.sof_info.sof_frame_idx = cxt->sof_idx;
	af_in.sof_info.timestamp.sec = sof_info->time_stamp.sec;
	af_in.sof_info.timestamp.usec = sof_info->time_stamp.usec;
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CTRL_CMD_SET_SOF_FRAME_IDX, &af_in, NULL);
	if (ret) {
		ISP_LOGE("failed to set af sof");
	}
	awb_in.sof_frame_idx = cxt->sof_idx;
	ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_SOF_FRAME_IDX, &awb_in, &awb_out);
	sof_cfg_info.is_update = 0;
	if (cxt->awb_cxt.proc_out.is_update) {
		sof_cfg_info.awb_gain.r = cxt->awb_cxt.proc_out.gain.r;
		sof_cfg_info.awb_gain.g = cxt->awb_cxt.proc_out.gain.g;
		sof_cfg_info.awb_gain.b = cxt->awb_cxt.proc_out.gain.b;
		sof_cfg_info.awb_b_gain.r = cxt->awb_cxt.proc_out.gain_balanced.r;
		sof_cfg_info.awb_b_gain.g = cxt->awb_cxt.proc_out.gain_balanced.g;
		sof_cfg_info.awb_b_gain.b = cxt->awb_cxt.proc_out.gain_balanced.b;
		sof_cfg_info.color_temp = cxt->awb_cxt.proc_out.ct;
		sof_cfg_info.is_update = 1;
	}

	ae_in.sof_param.frame_index = cxt->sof_idx;
	ae_in.sof_param.timestamp.sec = sof_info->time_stamp.sec;
	ae_in.sof_param.timestamp.usec = sof_info->time_stamp.usec;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_SOF, &ae_in, &ae_out);
	if (ret) {
		ISP_LOGE("failed to set ae sof");
	}
	ISP_LOGI("test msg 0");
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_HW_ISO_SPEED, NULL, &ae_out);
	if (ret) {
		ISP_LOGE("failed to get hw_iso_speed");
	}
	ISP_LOGI("test msg 1");
	sof_cfg_info.iso_val = ae_out.hw_iso_speed;
	ret = isp_dev_access_cfg_sof_info(cxt->dev_access_handle, &sof_cfg_info);
	if (ret) {
		ISP_LOGE("failed to cfg sof info");
	}
	ISP_LOGI("test msg 2");
	return ret;
}

cmr_int isp3a_start_ae_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data, struct isp3a_statistics_data *awb_stats_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_proc_in                      input;
	struct ae_ctrl_proc_out                     output;
	struct ae_ctrl_param_in                     param_in;

	param_in.awb_report.color_temp = cxt->awb_cxt.proc_out.ct;
	param_in.awb_report.normal_gain.r = cxt->awb_cxt.proc_out.gain.r;
	param_in.awb_report.normal_gain.g = cxt->awb_cxt.proc_out.gain.g;
	param_in.awb_report.normal_gain.b = cxt->awb_cxt.proc_out.gain.b;
	param_in.awb_report.balance_gain.r = cxt->awb_cxt.proc_out.gain_balanced.r;
	param_in.awb_report.balance_gain.g = cxt->awb_cxt.proc_out.gain_balanced.g;
	param_in.awb_report.balance_gain.b = cxt->awb_cxt.proc_out.gain_balanced.b;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_AWB_REPORT, &param_in, NULL);
	if (ret)
		ISP_LOGW("set awb report failed!!!");

	ret = isp3a_hold_statistics_buf(isp_3a_handle, ISP3A_AE, stats_data);
	ret = isp3a_hold_statistics_buf(isp_3a_handle, ISP3A_AWB, awb_stats_data);
	input.stat_data_ptr = stats_data;
	input.awb_stat_data_ptr = awb_stats_data;
	ret = ae_ctrl_process(cxt->ae_cxt.handle, &input, &output);
	if (ret) {
		ISP_LOGE("failed to ae process %ld", ret);
		ret = isp3a_release_statistics_buf(isp_3a_handle, ISP3A_AE, stats_data);
	}
	return ret;
}

cmr_int isp3a_start_af_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct af_ctrl_process_in                   input;
	struct af_ctrl_process_out                  output;
	union awb_ctrl_cmd_in                       awb_in;

	ret = af_ctrl_ioctrl(cxt->af_cxt.handle,
			     AF_CTRL_CMD_SET_UPDATE_AWB,
			     NULL, NULL);
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle,
			     AF_CTRL_CMD_SET_UPDATE_AE,
			     (void *)&cxt->ae_cxt.proc_out.ae_info, NULL);
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle,
			     AF_CTRL_CMD_SET_PROC_START,
			     NULL, NULL);
	ret = isp3a_hold_statistics_buf(isp_3a_handle, ISP3A_AF, stats_data);
	input.statistics_data = stats_data;
	output.data = NULL;
	output.size = 0;
	ret = af_ctrl_process(cxt->af_cxt.handle, &input, &output);
	if (ret) {
		ISP_LOGE("failed to af process %ld", ret);
	} else {
		if (output.data) {
			awb_in.af_report.data = output.data;
			awb_in.af_report.data_size = output.size;
			ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_AF_REPORT, &awb_in, NULL);
		}
	}
	ret = isp3a_release_statistics_buf(isp_3a_handle, ISP3A_AF, stats_data);
	return ret;
}

cmr_int isp3a_start_afl_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct afl_ctrl_proc_in                     input;
	struct afl_ctrl_proc_out                    output;

	input.stat_data_ptr = stats_data;
	isp3a_hold_statistics_buf(isp_3a_handle, ISP3A_AFL, stats_data);

	ret = afl_ctrl_process(cxt->afl_cxt.handle, &input, &output);
	if (ret) {
		ISP_LOGE("failed to afl process");
	}

	/*
	 * we don't exe isp3a_release_statistics_buf
	 * afl will use the buff for queue working
	 * */
#ifndef CONFIG_CAMERA_AFL_AUTO_DETECTION
	ret = isp3a_release_statistics_buf(isp_3a_handle, ISP3A_AFL, stats_data);
#endif
	return ret;
}

cmr_int isp3a_start_yhis_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     param_in;

	isp3a_hold_statistics_buf(isp_3a_handle, ISP3A_YHIS, stats_data);

	param_in.y_hist_stat = stats_data;
	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle,
			     AE_CTRL_SET_Y_HIST_STATS,
			     &param_in, NULL);

	ret = isp3a_release_statistics_buf(isp_3a_handle, ISP3A_YHIS, stats_data);
	return ret;
}

cmr_int isp3a_start_awb_process(cmr_handle isp_3a_handle, struct isp3a_statistics_data *stats_data, struct ae_ctrl_callback_in *ae_info)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct awb_ctrl_process_in                  input;
	struct awb_ctrl_process_out                 output;

	input.statistics_data = stats_data;
	memcpy(&input.ae_info, &ae_info->proc_out.ae_info, sizeof(struct isp3a_ae_info));
	ret = awb_ctrl_process(cxt->awb_cxt.handle, &input, &output);
	if (ret) {
		ISP_LOGE("failed to awb process %ld", ret);
	} else {
		if (output.is_update) {
			cxt->awb_cxt.proc_out = output;
		}
		if (AWB_CTRL_STATUS_CONVERGE == output.awb_states) {
			union awb_ctrl_cmd_in  awb_in;
			awb_in.lock_param.lock_flag = 1;
			awb_in.lock_param.ct = 0;
			awb_in.lock_param.wbgain.r = 0;
			awb_in.lock_param.wbgain.g = 0;
			awb_in.lock_param.wbgain.b = 0;
			awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_LOCK, &awb_in, NULL);
		}
	}
exit:
	ret = isp3a_release_statistics_buf(isp_3a_handle, ISP3A_AWB, stats_data);
	return ret;
}

cmr_int isp3a_handle_ae_result(cmr_handle isp_3a_handle, struct ae_ctrl_callback_in *result_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp3a_statistics_data *awb_stats_data;

	awb_stats_data = (struct isp3a_statistics_data * )result_ptr->proc_out.ae_frame.awb_stats_buff_ptr;
	ret = isp3a_release_statistics_buf(isp_3a_handle, ISP3A_AE, (struct isp3a_statistics_data * )result_ptr->proc_out.ae_frame.stats_buff_ptr);
	if (ret) {
		ISP_LOGE("failed to releae ae stats buf %ld", ret);
	}
	if (result_ptr->proc_out.ae_frame.is_skip_cur_frame) {
		ISP_LOGI("ae skip frame");
		ret = isp3a_release_statistics_buf(isp_3a_handle, ISP3A_AWB, awb_stats_data);
	} else {
		cxt->ae_cxt.proc_out = result_ptr->proc_out;
		ret = isp3a_start_awb_process(isp_3a_handle, awb_stats_data, result_ptr);
	}
	uint64_t exp_time = 0; //us
	exp_time = (uint64_t)cxt->ae_cxt.proc_out.ae_info.report_data.exp_time;
	if (cxt->caller_callback) {
		ret = cxt->caller_callback(cxt->caller_handle, ISP_CALLBACK_EVT|ISP_AE_EXP_TIME, &exp_time, 4);
	}
	return ret;
}

cmr_int isp3a_start(cmr_handle isp_3a_handle, struct isp_video_start *input_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_in                     ae_in;
	struct ae_ctrl_param_out                    ae_out;
	union awb_ctrl_cmd_in                       awb_in;
	union awb_ctrl_cmd_out                      awb_out;
	struct afl_ctrl_param_in                    afl_in;
	struct afl_ctrl_param_out                   afl_out;

	cxt->sof_idx = 0;

	if (input_ptr->sensor_fps.is_high_fps) {
		if ((input_ptr->sensor_fps.high_fps_skip_num-1) > 0) {
			ret = isp_dev_access_set_skip_num(cxt->dev_access_handle, input_ptr->sensor_fps.high_fps_skip_num-1);
			if (ret) {
				ISP_LOGE("failed to isp_dev_access_set_skip_num");
			}
		}
	}

	ae_in.work_param.work_mode = input_ptr->work_mode;
	ae_in.work_param.capture_mode = input_ptr->capture_mode;
	ae_in.work_param.is_refocus = input_ptr->is_refocus;
	ae_in.work_param.resolution.frame_size.w = input_ptr->resolution_info.crop.width;
	ae_in.work_param.resolution.frame_size.h = input_ptr->resolution_info.crop.height;
	ae_in.work_param.resolution.line_time = input_ptr->resolution_info.line_time;
	ae_in.work_param.resolution.frame_line = input_ptr->resolution_info.frame_line;
	ae_in.work_param.resolution.sensor_size_index = input_ptr->resolution_info.size_index;
	ae_in.work_param.resolution.max_fps = input_ptr->resolution_info.fps.max_fps;
	ae_in.work_param.resolution.min_fps = input_ptr->resolution_info.fps.min_fps;
	//ae_in.work_param.resolution.max_gain = input_ptr->resolution_info.max_gain;
	ae_in.work_param.sensor_fps = input_ptr->sensor_fps;

	ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_SET_WORK_MODE, &ae_in, &ae_out);
	cxt->ae_cxt.proc_out = ae_out.proc_out;
	cxt->ae_cxt.hw_cfg = ae_out.proc_out.hw_cfg;
	if (ret) {
		ISP_LOGE("failed to set work to AE");
		goto exit;
	}
	/* HW AE cfg */
	awb_in.work_param.work_mode = input_ptr->work_mode;
	awb_in.work_param.capture_mode = input_ptr->capture_mode;
	awb_in.work_param.sensor_size.w = input_ptr->resolution_info.crop.width;
	awb_in.work_param.sensor_size.h = input_ptr->resolution_info.crop.height;
	ret = awb_ctrl_ioctrl(cxt->awb_cxt.handle, AWB_CTRL_CMD_SET_WORK_MODE, &awb_in, &awb_out);
	if (ret) {
		ISP_LOGE("failed to set work mode to AWB");
		goto exit;
	}

	afl_in.work_param.work_mode = input_ptr->work_mode;
	afl_in.work_param.capture_mode = input_ptr->capture_mode;
	afl_in.work_param.resolution.line_time = input_ptr->resolution_info.line_time;
	afl_in.work_param.resolution.frame_size.w = input_ptr->resolution_info.crop.width;
	afl_in.work_param.resolution.frame_size.h = input_ptr->resolution_info.crop.height;
	ret = afl_ctrl_ioctrl(cxt->afl_cxt.handle, AFL_CTRL_SET_WORK_MODE, &afl_in, &afl_out);
	if (ret) {
		ISP_LOGE("failed to set work mode to AFL");
	} else {
		cxt->afl_cxt.hw_cfg = afl_out.hw_cfg;
	}
exit:
	cxt->err_code = ret;
	return ret;
}

cmr_int isp3a_stop(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct afl_ctrl_param_in                    afl_in;
	struct afl_ctrl_param_out                   afl_out;

	ret = afl_ctrl_ioctrl(cxt->afl_cxt.handle, AFL_CTRL_SET_STAT_QUEUE_RELEASE, &afl_in, &afl_out);
	if (ret) {
		ISP_LOGE("failed to release stat queue to AFL");
	}

	return ret;
}

/*************************************EXTERNAL FUNCTION ***************************************/
cmr_int isp_3a_fw_init(struct isp_3a_fw_init_in *input_ptr, cmr_handle *isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = NULL;
	struct sensor_raw_info                      *sensor_raw_info_ptr = NULL;
	struct sensor_raw_info                      *sensor_raw_info_ptr_slv = NULL;

	if (!input_ptr || !isp_3a_handle) {
		ISP_LOGE("input is NULL, 0x%lx", (cmr_uint)input_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	*isp_3a_handle = NULL;
	cxt = (struct isp3a_fw_context *)malloc(sizeof(struct isp3a_fw_context ));
	if (!cxt) {
		ISP_LOGE("faield to malloc");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));
	sensor_raw_info_ptr = (struct sensor_raw_info*)input_ptr->setting_param_ptr;
	cxt->camera_id = input_ptr->camera_id;
	cxt->caller_handle = input_ptr->caller_handle;
	cxt->caller_callback = input_ptr->isp_mw_callback;
	cxt->ops = input_ptr->ops;
	cxt->setting_param_ptr = input_ptr->setting_param_ptr;
	if (NULL != sensor_raw_info_ptr)
		cxt->ioctrl_ptr = sensor_raw_info_ptr->ioctrl_ptr;
	cxt->dev_access_handle = input_ptr->dev_access_handle;
	cxt->bin_cxt.bin_info = input_ptr->bin_info;
	cxt->bin_cxt.is_write_to_debug_buf = 0;
	cxt->ex_info = input_ptr->ex_info;
	if (input_ptr->otp_data) {
		cxt->otp_data = input_ptr->otp_data;
	}

#ifdef CONFIG_CAMERA_RE_FOCUS
	sensor_raw_info_ptr_slv = (struct sensor_raw_info*)input_ptr->setting_param_ptr_slv;
	if (NULL != sensor_raw_info_ptr_slv)
		cxt->ioctrl_ptr_slv = sensor_raw_info_ptr_slv->ioctrl_ptr;
	if (input_ptr->otp_data_slv) {
		cxt->otp_data_slv = input_ptr->otp_data_slv;
	}
#endif

	sem_init(&cxt->statistics_data_sm, 0, 1);

	ret = isp3a_init_statistics_buf((cmr_handle)cxt);
	if (ret) {
		goto exit;
	}

	ret = isp3a_alg_init((cmr_handle)cxt, input_ptr);
	if (ret) {
		goto exit;
	}
	ret = isp3a_set_cfg_otp_info((cmr_handle)cxt);

	ret = isp3a_create_thread((cmr_handle)cxt);
exit:
	if (ret) {
		isp3a_destroy_thread((cmr_handle)cxt);
		isp3a_alg_deinit((cmr_handle)cxt);
		isp3a_deinit_statistics_buf((cmr_handle)cxt);
		if (cxt) {
			free((void*)cxt);
		}
	} else {
		cxt->is_inited = 1;
		*isp_3a_handle = (cmr_handle)cxt;
		isp_dev_access_evt_reg(cxt->dev_access_handle, isp3a_dev_evt_cb, (cmr_handle)cxt);
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_3a_fw_deinit(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	if (!cxt) {
		ISP_LOGE("input is NULL");
		goto exit;
	}
	isp3a_destroy_thread((cmr_handle)cxt);
	isp3a_alg_deinit((cmr_handle)cxt);
	isp3a_deinit_statistics_buf((cmr_handle)cxt);
	sem_destroy(&cxt->statistics_data_sm);
	free((void*)cxt);

exit:
	return ret;
}

cmr_int isp_3a_fw_capability(cmr_handle isp_3a_handle, enum isp_capbility_cmd cmd, void* param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct ae_ctrl_param_out                    output;

	switch (cmd) {
	case ISP_LOW_LUX_EB:
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_FLASH_EB, NULL, &output);
		*((cmr_u32*)param_ptr) = output.flash_eb;
		break;
	case ISP_CUR_ISO:
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_ISO, NULL, &output);
		*((cmr_u32*)param_ptr) = output.iso_val;
		break;
	case ISP_CTRL_GET_AE_LUM:
		ret = ae_ctrl_ioctrl(cxt->ae_cxt.handle, AE_CTRL_GET_BV_BY_LUM, NULL, &output);
		*((uint32_t*)param_ptr) = output.bv;
		break;
	default:
		break;
	}

	ISP_LOGI("done %ld", ret);
	return ret;
}

static cmr_int isp_3a_fw_assemble_async_msg(enum isp_ctrl_cmd cmd, struct cmr_msg *msg, void *source)
{
	cmr_int                                     ret = ISP_SUCCESS;

	msg->msg_type = ISP3A_PROC_EVT_IOCTRL;
	msg->sub_msg_type = cmd;
	msg->alloc_flag = 1;

	switch (cmd) {
	case ISP_CTRL_SET_AUX_SENSOR_INFO:
		msg->data = malloc(sizeof(struct af_aux_sensor_info_t));
		memcpy(msg->data, source, sizeof(struct af_aux_sensor_info_t));
		break;
	default:
		ret = -ISP_ERROR;
		break;
	}

	return ret;
}

cmr_int isp_3a_fw_ioctl(cmr_handle isp_3a_handle, enum isp_ctrl_cmd cmd, void* param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	CMR_MSG_INIT(message);

	if (!isp_3a_handle) {
		ISP_LOGE("input is NULL");
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	if (!cxt->thread_cxt.process_thr_handle) {
		ISP_LOGE("ctrl thread is NULL");
		ret = ISP_ERROR;
		goto exit;
	}

	if (ISP_CTRL_SYNC_NONE_MSG_GEGIN < cmd) {
		ret = isp_3a_fw_assemble_async_msg(cmd, &message, param_ptr);
		if (ret)
			goto exit;
	} else {
		message.msg_type = ISP3A_PROC_EVT_IOCTRL;
		message.sub_msg_type = cmd;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		message.data = (void*)param_ptr;
	}
	ret = cmr_thread_msg_send(cxt->thread_cxt.process_thr_handle, &message);
exit:
	ISP_LOGV("cmd = %d done %ld", cmd, ret);
	return ret;
}

cmr_int isp_3a_fw_start(cmr_handle isp_3a_handle, struct isp_video_start *input_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	CMR_MSG_INIT(message);

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	if (!input_ptr) {
		ISP_LOGE("input is NULL");
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	message.msg_type = ISP3A_PROC_EVT_START;
	message.sub_msg_type = 0;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = (void*)input_ptr;
	ret = cmr_thread_msg_send(cxt->thread_cxt.process_thr_handle, &message);
	if(!ret) {
		ret = cxt->err_code;
	}
exit:
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_3a_fw_stop(cmr_handle isp_3a_handle)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	CMR_MSG_INIT(message);

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	message.msg_type = ISP3A_PROC_EVT_STOP;
	message.sub_msg_type = 0;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = NULL;
	ret = cmr_thread_msg_send(cxt->thread_cxt.process_thr_handle, &message);

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_3a_fw_receive_data(cmr_handle isp_3a_handle, cmr_int evt, void *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	UNUSED(evt);
	UNUSED(data);
	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_3a_fw_get_cfg(cmr_handle isp_3a_handle, struct isp_3a_cfg_param *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	data->ae_cfg = cxt->ae_cxt.hw_cfg;
	data->awb_cfg = cxt->awb_cxt.hw_cfg;
	data->af_cfg = cxt->af_cxt.hw_cfg;
	data->afl_cfg = cxt->afl_cxt.hw_cfg;
	data->yhis_cfg = cxt->other_stats_cxt.yhis_hw_cfg;
	data->subsample_cfg = cxt->other_stats_cxt.subsample_hw_cfg;
	data->awb_gain = cxt->awb_cxt.proc_out.gain;
	data->awb_gain_balanced = cxt->awb_cxt.proc_out.gain_balanced;
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_3a_fw_get_dldseq(cmr_handle isp_3a_handle, struct isp_3a_get_dld_in *input, struct isp_3a_dld_sequence *data)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	data->uc_sensor_id = cxt->camera_id;
	switch (input->op_mode) {
	case ISP3A_OPMODE_NORMALLV:
		/* W9 config */
		data->ucpreview_basic_dsdseqlen = 4;
		data->aucpreview_basic_dldseq[0] = HA3ACTRL_B_DL_TYPE_AE;
		data->aucpreview_basic_dldseq[1] = HA3ACTRL_B_DL_TYPE_AWB;
		data->aucpreview_basic_dldseq[2] = HA3ACTRL_B_DL_TYPE_AF;
		data->aucpreview_basic_dldseq[3] = HA3ACTRL_B_DL_TYPE_AWB;
		/* W10 config */
		data->ucpreview_adv_dldseqlen = 1;
		data->aucpreview_adv_dldseq[0] = HA3ACTRL_B_DL_TYPE_AntiF;
		/* W9 */
		data->ucfastcoverge_basic_dldseqlen = 0;
		data->aucfastconverge_basic_dldseq[0] = HA3ACTRL_B_DL_TYPE_NONE;
		/* W10 */
		break;
	case ISP3A_OPMODE_AF_FLASH_AF:
		/* W9 config */
		data->ucpreview_basic_dsdseqlen = 2;
		data->aucpreview_basic_dldseq[0] = HA3ACTRL_B_DL_TYPE_AF;
		data->aucpreview_basic_dldseq[1] = HA3ACTRL_B_DL_TYPE_AF;
		/* W10 config */
		data->ucpreview_adv_dldseqlen = 1;
		data->aucpreview_adv_dldseq[0] = HA3ACTRL_B_DL_TYPE_Sub;

		data->ucfastcoverge_basic_dldseqlen = 0;
		data->aucfastconverge_basic_dldseq[0] = HA3ACTRL_B_DL_TYPE_NONE;
		break;
	case ISP3A_OPMODE_FLASH_AE:
		/* W9 config */
		data->ucpreview_basic_dsdseqlen = 2;
		data->aucpreview_basic_dldseq[0] = HA3ACTRL_B_DL_TYPE_AE;
		data->aucpreview_basic_dldseq[1] = HA3ACTRL_B_DL_TYPE_AWB;
		/* W10 config */
		data->ucpreview_adv_dldseqlen = 1;
		data->aucpreview_adv_dldseq[0] = HA3ACTRL_B_DL_TYPE_AntiF;

		data->ucfastcoverge_basic_dldseqlen = 0;
		data->aucfastconverge_basic_dldseq[0] = HA3ACTRL_B_DL_TYPE_NONE;
		break;
	case ISP3A_OPMODE_MAX:
	default:
		ISP_LOGI("don't support opmode %d", input->op_mode);
		break;
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_3a_fw_get_awb_gain(cmr_handle isp_3a_handle, struct isp_awb_gain *gain, struct isp_awb_gain *gain_balanced)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;

	ISP_CHECK_HANDLE_VALID(isp_3a_handle);

	*gain = cxt->awb_cxt.proc_out.gain;
	*gain_balanced = cxt->awb_cxt.proc_out.gain_balanced;
	return ret;
}

cmr_int isp3a_get_yhist_info(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	struct isp_yhist_info                       *info_ptr = (struct isp_yhist_info*)param_ptr;
	struct af_ctrl_param_out                    af_out;

	bzero(&af_out, sizeof(af_out));
	ret = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CTRL_CMD_GET_YHIST, NULL, &af_out);
	if (!ret) {
		ISP_LOGI("%d", af_out.y_info.in_proc[0]);
		memcpy(info_ptr->yaddr, af_out.y_info.yaddr, sizeof(info_ptr->yaddr));
		memcpy(info_ptr->lock, af_out.y_info.in_proc, sizeof(info_ptr->lock));
	}

	return ret;
}

static cmr_int isp3a_set_prev_yhist(cmr_handle isp_3a_handle, void *param_ptr)
{
	cmr_int                                     ret = ISP_SUCCESS;
	struct isp3a_fw_context                     *cxt = (struct isp3a_fw_context*)isp_3a_handle;
	cmr_uint                                    *yhist = param_ptr;

	if (!param_ptr) {
		ISP_LOGW("input is NULL");
		goto exit;
	}

	ret = af_ctrl_ioctrl(cxt->af_cxt.handle, AF_CTRL_CMD_SET_Y_HIST_INFO, (void *)*yhist, NULL);


exit:
	return ret;
}
