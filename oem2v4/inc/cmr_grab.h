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
#ifndef _CMR_V4L2_H_
#define _CMR_V4L2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cmr_common.h"

enum channel_status { CHN_IDLE = 0, CHN_BUSY };

enum grab_sensor_format {
    GRAB_SENSOR_FORMAT_YUV = 0,
    GRAB_SENSOR_FORMAT_SPI,
    GRAB_SENSOR_FORMAT_JPEG,
    GRAB_SENSOR_FORMAT_RAWRGB,
    GRAB_SENSOR_FORMAT_MAX
};

enum cmr_grab_rtn {
    CMR_GRAB_RET_OK = 0,
    CMR_GRAB_RET_RESTART,
    CMR_GRAB_RET_MAX,
};

#if 0
enum cmr_grab_wtite_cmd_id {
	CMR_GRAB_WRITE_STOP = 0x5AA5,
	CMR_GRAB_WRITE_FREE_FRAME = 0xA55A,
	CMR_GRAB_WRITE_MAX
};
#endif

enum {
    PREV_TRACE = 0,
    CAP_TRACE,
    TRACE_MAX,
};

struct grab_init_param {
    cmr_handle oem_handle;
    cmr_u32 sensor_id;
    struct img_size sensor_max_size;
};

struct sn_cfg {
    cmr_u32 sbs_mode;
    struct img_size sn_size;
    struct img_rect sn_trim;
    cmr_u32 frm_num;
    // sensor_max_size is for isp alloc memory use
    struct img_size sensor_max_size;
    //for sensor interpolation use
    struct img_size actual_pic_size;
};

struct grab_flash_opt {
    cmr_u32 led0_enable;
    cmr_u32 led1_enable;
    cmr_u32 flash_mode;
    cmr_u32 flash_index;
};

typedef cmr_int (*grab_stream_on)(cmr_u32 is_on, void *privdata);

struct cmr_grab {
    cmr_s32 fd;
    cmr_evt_cb grab_evt_cb;
    cmr_evt_cb isp_statis_evt_cb;
    cmr_u32 isp_cb_enable;
    cmr_evt_cb isp_irq_proc_evt_cb;
    cmr_evt_cb grab_3dnr_evt_cb;
    cmr_evt_cb grab_post_ynr_evt_cb;
    sem_t close_sem;
    pthread_mutex_t cb_mutex;
    pthread_mutex_t dcam_mutex;
    pthread_mutex_t status_mutex;
    pthread_mutex_t path_mutex[CHN_MAX];
    cmr_u32 is_on;
    pthread_t thread_handle;
    cmr_u32 chn_status[CHN_MAX];
    cmr_u32 is_prev_trace;
    cmr_u32 is_cap_trace;
    cmr_u32 is_rt_refocus;
    struct grab_init_param init_param;
    grab_stream_on stream_on_cb;
    cmr_u8 mode_enable;
    cmr_u8 res;
    cmr_u32 is_sidebyside;
    cmr_u32 offline_mode;
};

cmr_int cmr_grab_init(struct grab_init_param *init_param_ptr,
                      cmr_handle *grab_handle);
cmr_int cmr_grab_deinit(cmr_handle grab_handle);
cmr_s32 cmr_grab_get_iommu_status(cmr_handle grab_handle);
void cmr_grab_evt_reg(cmr_handle grab_handle, cmr_evt_cb v4l2_event_cb);
void cmr_grab_isp_statis_evt_reg(cmr_handle grab_handle,
                                 cmr_evt_cb isp_statis_event_cb);
void cmr_grab_isp_irq_proc_evt_reg(cmr_handle grab_handle,
                                   cmr_evt_cb isp_irq_proc_event_cb);
void cmr_grab_post_ynr_evt_reg(cmr_handle grab_handle, cmr_evt_cb grab_post_ynr_evt_cb);
void cmr_grab_3dnr_evt_reg(cmr_handle grab_handle,
                           cmr_evt_cb grab_3dnr_event_cb);
cmr_int cmr_grab_if_cfg(cmr_handle grab_handle, struct sensor_if *sn_if);
cmr_int cmr_grab_if_decfg(cmr_handle grab_handle, struct sensor_if *sn_if);
cmr_int cmr_grab_sn_cfg(cmr_handle grab_handle, struct sn_cfg *config);
cmr_int cmr_grab_cap_cfg(cmr_handle grab_handle, struct cap_cfg *config,
                         cmr_u32 *channel_id, struct img_data_end *endian);
cmr_int cmr_grab_cap_cfg_lightly(cmr_handle grab_handle, struct cap_cfg *config,
                                 cmr_u32 channel_id);
cmr_int cmr_grab_buff_cfg(cmr_handle grab_handle, struct buffer_cfg *buf_cfg);
cmr_int cmr_grab_sw_3dnr_cfg(cmr_handle grab_handle,
                             struct sprd_img_3dnr_param *threednr);
cmr_int cmr_grab_cap_start(cmr_handle grab_handle, cmr_u32 skip_num);
cmr_int cmr_grab_cap_stop(cmr_handle grab_handle);
cmr_int cmr_grab_cap_resume(cmr_handle grab_handle, cmr_u32 channel_id,
                            cmr_u32 skip_number, cmr_u32 deci_factor,
                            cmr_s32 frm_num);
cmr_int cmr_grab_cap_pause(cmr_handle grab_handle, cmr_u32 channel_id,
                           cmr_u32 reconfig_flag);
cmr_int cmr_grab_free_frame(cmr_handle grab_handle, cmr_u32 channel_id,
                            cmr_u32 index);
cmr_int cmr_grab_scale_capability(cmr_handle grab_handle, cmr_u32 *width,
                                  cmr_u32 *sc_factor, cmr_u32 *sc_threshold);
cmr_int cmr_grab_get_cap_time(cmr_handle grab_handle, cmr_u32 *sec,
                              cmr_u32 *usec);
cmr_int cmr_grab_flash_cb(cmr_handle grab_handle,
                          struct grab_flash_opt *flash_opt);
cmr_int cmr_grab_stream_cb(cmr_handle grab_handle, grab_stream_on str_on);
cmr_int cmr_grab_set_trace_flag(cmr_handle grab_handle, cmr_u32 trace_owner,
                                cmr_u32 val);
cmr_int cmr_grab_set_zoom_mode(cmr_handle grab_handle, cmr_u32 opt);
cmr_u32 cmr_grab_get_dcam_endian(struct img_data_end *in_endian,
                                 struct img_data_end *out_endian);
cmr_int cmr_grab_path_capability(cmr_handle grab_handle,
                                 struct cmr_path_capability *capability);
cmr_int cmr_grab_cfg_flash(cmr_handle grab_handle,
                           struct sprd_flash_cfg_param *cfg);
cmr_s32 cmr_grab_get_cpp_fd(cmr_handle grab_handle);
cmr_int cmr_grab_set_rt_refocus(cmr_handle grab_handle, cmr_u32 rt_refocus);
cmr_int cmr_grab_deinit_notice(cmr_handle grab_handle);

// for sharkl2-like capture path
cmr_int cmr_grab_start_capture(cmr_handle grab_handle,
                               struct sprd_img_capture_param capture_param);
cmr_int cmr_grab_stop_capture(cmr_handle grab_handle);
cmr_int cmr_grab_set_sidebyside_flag(cmr_handle grab_handle,
                                     cmr_u32 is_sidebyside);
cmr_int cmr_grab_get_sg(cmr_handle grab_handle,
                           struct sprd_img_iova *iommu_map_data);
cmr_int cmr_grab_map_iommu(cmr_handle grab_handle,
                           struct sprd_img_iova *iommu_map_data);
cmr_int cmr_grab_unmap_iommu(cmr_handle grab_handle,
                           struct sprd_img_iova *iommu_map_data);
cmr_int cmr_grab_set_alloc_size(cmr_handle grab_handle,
                                 cmr_u16 weight, cmr_u16 height);
#ifdef __cplusplus
}
#endif

#endif // for _CMR_V4L2_H_
