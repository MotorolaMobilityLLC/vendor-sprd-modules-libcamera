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
#define LOG_TAG "cmr_sensor"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/trace.h>
#include <utils/Log.h>
#include <fcntl.h> /* low-level i/o */
#include <errno.h>
#include <sys/ioctl.h>

#include "sensor_cfg.h"
#include "cmr_msg.h"
#include "cmr_sensor.h"

/**---------------------------------------------------------------------------*
 **                         MACRO definition                                  *
 **---------------------------------------------------------------------------*/
#define SENSOR_MSG_QUEUE_SIZE 10
#define CMR_ISP_OTP_MAX_SIZE (100 * 1024)

#define CMR_SENSOR_EVT_BASE (CMR_EVT_SENSOR_BASE + 0x100)
#define CMR_SENSOR_EVT_INIT (CMR_SENSOR_EVT_BASE + 0x0)
#define CMR_SENSOR_EVT_EXIT (CMR_SENSOR_EVT_BASE + 0x1)
#define CMR_SENSOR_EVT_OPEN (CMR_SENSOR_EVT_BASE + 0x2)
#define CMR_SENSOR_EVT_CLOSE (CMR_SENSOR_EVT_BASE + 0x3)
#define CMR_SENSOR_EVT_IOCTL (CMR_SENSOR_EVT_BASE + 0x4)
#define CMR_SENSOR_EVT_SETMODE (CMR_SENSOR_EVT_BASE + 0x5)
#define CMR_SENSOR_EVT_SETMODONE (CMR_SENSOR_EVT_BASE + 0x6)
#define CMR_SENSOR_EVT_AFINIT (CMR_SENSOR_EVT_BASE + 0x7)
#define CMR_SENSOR_EVT_STREAM (CMR_SENSOR_EVT_BASE + 0x8)
#define CMR_SENSOR_EVT_SETEXIF (CMR_SENSOR_EVT_BASE + 0x9)
#define CMR_SENSOR_EVT_ISPARAM_FROM_FILE (CMR_SENSOR_EVT_BASE + 0xa)
#define CMR_SENSOR_EVT_BYPASS_MODE (CMR_SENSOR_EVT_BASE + 0xb)

#define CMR_SENSOR_MONITOR_BASE (CMR_EVT_SENSOR_BASE + 0x200)
#define CMR_SENSOR_MONITOR_INIT (CMR_SENSOR_EVT_BASE + 0x0)
#define CMR_SENSOR_MONITOR_EXIT (CMR_SENSOR_EVT_BASE + 0x1)

#define CMR_SENSOR_FMOVE_BASE (CMR_EVT_SENSOR_BASE + 0x300)
#define CMR_SENSOR_FMOVE_INIT (CMR_SENSOR_EVT_BASE + 0x0)
#define CMR_SENSOR_FMOVE_EXIT (CMR_SENSOR_EVT_BASE + 0x1)

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

#define CHECK_HANDLE_VALID_VOID(handle)                                        \
    do {                                                                       \
        if (!handle) {                                                         \
            return;                                                            \
        }                                                                      \
    } while (0)

/**---------------------------------------------------------------------------*
 **                         type definition                                   *
 **---------------------------------------------------------------------------*/
struct cmr_sns_ioctl_param {
    cmr_uint cmd;
    cmr_uint arg;
};

struct cmr_exif_param {
    SENSOR_EXIF_CTRL_E cmd;
    cmr_uint param;
};

struct cmr_sns_thread_cxt {
    cmr_uint is_inited;
    cmr_handle thread_handle;
};

struct cmr_sensor_handle {
    struct sensor_drv_context sensor_cxt[CAMERA_ID_MAX];
    cmr_handle oem_handle;
    cmr_uint sensor_bits;
    cmr_evt_cb sensor_event_cb;
    struct cmr_sns_thread_cxt thread_cxt;
    void *private_data;
    cmr_uint is_autotest;
};

/**---------------------------------------------------------------------------*
 **                         Local Functions declaration                       *
 **---------------------------------------------------------------------------*/
static cmr_int cmr_sns_create_thread(struct cmr_sensor_handle *handle);
static cmr_int cmr_sns_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int cmr_sns_destroy_thread(struct cmr_sensor_handle *handle);
static cmr_int cmr_sns_open(struct cmr_sensor_handle *handle,
                            cmr_u32 sensor_id_bits);
static cmr_int cmr_sns_close(struct cmr_sensor_handle *handle,
                             cmr_u32 sensor_id_bits);
static cmr_int cmr_sns_ioctl(struct sensor_drv_context *sensor_cxt,
                             cmr_uint cmd, cmr_uint arg);
static cmr_int cmr_sns_af_init(struct sensor_drv_context *sensor_cxt);
static cmr_int cmr_sns_copy_mode_info(struct sensor_mode_info *out_mode_info,
                                      SENSOR_MODE_INFO_T *in_mode_info);
static cmr_int cmr_sns_copy_video_info(struct sensor_video_info *out_video_info,
                                       SENSOR_VIDEO_INFO_T *in_video_info);
static cmr_int cmr_sns_copy_info(struct sensor_exp_info *out_sensor_info,
                                 SENSOR_EXP_INFO_T *in_sensor_info);
cmr_int cmr_sns_get_ioctl_cmd(SENSOR_IOCTL_CMD_E *sns_cmd,
                              enum sensor_cmd in_cmd);

/**---------------------------------------------------------------------------*
 **                         External Functions                                *
 **---------------------------------------------------------------------------*/
cmr_int cmr_sensor_init(struct sensor_init_param *init_param_ptr,
                        cmr_handle *sensor_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle = NULL;
    CMR_LOGV("E");

    if (!init_param_ptr) {
        CMR_LOGE("Invalid param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle =
        (struct cmr_sensor_handle *)malloc(sizeof(struct cmr_sensor_handle));
    if (!handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }
    cmr_bzero(handle, sizeof(struct cmr_sensor_handle));

    /*save init param*/
    handle->oem_handle = init_param_ptr->oem_handle;
    handle->sensor_bits = init_param_ptr->sensor_bits;
    handle->private_data = init_param_ptr->private_data;
    handle->is_autotest = init_param_ptr->is_autotest;
    CMR_LOGD(
        "oem_handle: 0x%lx, sensor_bits: %ld, private_data: 0x%lx autotest %ld",
        (cmr_uint)handle->oem_handle, handle->sensor_bits,
        (cmr_uint)handle->private_data, handle->is_autotest);

    /*create thread*/
    ret = cmr_sns_create_thread(handle);
    if (ret) {
        CMR_LOGE("create thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto init_end;
    }

    /*return handle*/
    *sensor_handle = (cmr_handle)handle;
    CMR_LOGI("handle 0x%lx created!", (cmr_uint)handle);

init_end:
    if (ret) {
        if (handle) {
            free(handle);
            handle = NULL;
        }
    }

    CMR_LOGV("X ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int cmr_sensor_deinit(cmr_handle sensor_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_LOGV("E");
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;

    if (!sensor_handle) {
        CMR_LOGE("X Invalid param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    /*destory thread*/
    ret = cmr_sns_destroy_thread(handle);
    if (ret) {
        CMR_LOGE("destory thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto deinit_end;
    }

    /*release handle*/
    if (handle) {
        free(handle);
        handle = NULL;
    }

deinit_end:
    CMR_LOGV("X ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int cmr_sensor_open(cmr_handle sensor_handle, cmr_u32 sensor_id_bits) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    cmr_u32 cameraId = 0;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    /*the open&close function should be sync*/
    message.msg_type = CMR_SENSOR_EVT_OPEN;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)sensor_id_bits);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("X send msg failed!");
        return CMR_CAMERA_FAIL;
    } else {
        for (cameraId = 0; cameraId < CAMERA_ID_MAX; cameraId++) {
            if (sensor_id_bits & (1 << cameraId)) {
                break;
            }
        }
        if (cameraId == CAMERA_ID_MAX ||
            handle->sensor_cxt[cameraId].fd_sensor == CMR_CAMERA_FD_INIT ||
            handle->sensor_cxt[cameraId].sensor_isInit != SENSOR_TRUE) {
            CMR_LOGE("camera %d open fail!", cameraId);
            ret = CMR_CAMERA_FAIL;
        }
    }

    CMR_LOGV("X ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int cmr_sensor_close(cmr_handle sensor_handle, cmr_u32 sensor_id_bits) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    /*the open&close function should be sync*/
    message.msg_type = CMR_SENSOR_EVT_CLOSE;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)sensor_id_bits);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("X send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGV("X ret %ld", ret);
    ATRACE_END();
    return ret;
}

void cmr_sensor_event_reg(cmr_handle sensor_handle, cmr_uint sensor_id,
                          cmr_evt_cb event_cb) {
    UNUSED(sensor_id);

    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("event_cb %p", (void *)event_cb);
    CHECK_HANDLE_VALID_VOID(handle);
    handle->sensor_event_cb = event_cb;
    return;
}

cmr_int cmr_sensor_stream_ctrl(cmr_handle sensor_handle, cmr_uint sensor_id,
                               cmr_uint on_off) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    /*the open&close function should be sync*/
    message.msg_type = CMR_SENSOR_EVT_STREAM;
    message.sub_msg_type = sensor_id;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)on_off;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("X send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGV("X ret %ld", ret);
    return ret;
}

cmr_int cmr_sensor_set_bypass_mode(cmr_handle sensor_handle, cmr_uint sensor_id,
                                   cmr_u32 bypass_mode) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    cmr_u32 cameraId = 0;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    handle->sensor_cxt[sensor_id].bypass_mode = bypass_mode;

    return ret;
}

cmr_int cmr_sns_copy_mode_info(struct sensor_mode_info *out_mode_info,
                               SENSOR_MODE_INFO_T *in_mode_info) {
    out_mode_info->mode = (cmr_u16)in_mode_info->mode;
    out_mode_info->width = in_mode_info->width;
    out_mode_info->height = in_mode_info->height;
    out_mode_info->trim_start_x = in_mode_info->trim_start_x;
    out_mode_info->trim_start_y = in_mode_info->trim_start_y;
    out_mode_info->trim_width = in_mode_info->trim_width;
    out_mode_info->trim_height = in_mode_info->trim_height;
    out_mode_info->image_format = (cmr_u16)in_mode_info->image_format;
    out_mode_info->line_time = in_mode_info->line_time;
    out_mode_info->bps_per_lane = in_mode_info->bps_per_lane;
    out_mode_info->frame_line = in_mode_info->frame_line;
    out_mode_info->scaler_trim.start_x = (cmr_u32)in_mode_info->scaler_trim.x;
    out_mode_info->scaler_trim.start_y = (cmr_u32)in_mode_info->scaler_trim.y;
    out_mode_info->scaler_trim.width = (cmr_u32)in_mode_info->scaler_trim.w;
    out_mode_info->scaler_trim.height = (cmr_u32)in_mode_info->scaler_trim.h;
    out_mode_info->out_width = in_mode_info->out_width;
    out_mode_info->out_height = in_mode_info->out_height;

    return CMR_CAMERA_SUCCESS;
}

cmr_int cmr_sns_copy_video_info(struct sensor_video_info *out_video_info,
                                SENSOR_VIDEO_INFO_T *in_video_info) {
    cmr_u32 i = 0;
    for (i = 0; i < SENSOR_VIDEO_MODE_MAX; i++) {
        out_video_info->ae_info[i].min_frate =
            in_video_info->ae_info[i].min_frate;
        out_video_info->ae_info[i].max_frate =
            in_video_info->ae_info[i].max_frate;
        out_video_info->ae_info[i].line_time =
            in_video_info->ae_info[i].line_time;
        out_video_info->ae_info[i].gain = in_video_info->ae_info[i].gain;
    }
    out_video_info->setting_pptr = (void *)in_video_info->setting_ptr;
    return CMR_CAMERA_SUCCESS;
}

cmr_int cmr_sns_copy_info(struct sensor_exp_info *out_sensor_info,
                          SENSOR_EXP_INFO_T *in_sensor_info) {
    cmr_u32 i = 0;
    out_sensor_info->image_format = in_sensor_info->image_format;
    out_sensor_info->image_pattern = in_sensor_info->image_pattern;
    out_sensor_info->change_setting_skip_num =
        in_sensor_info->change_setting_skip_num;
    out_sensor_info->sensor_image_type = in_sensor_info->sensor_image_type;
    out_sensor_info->pclk_polarity = in_sensor_info->pclk_polarity;
    out_sensor_info->vsync_polarity = in_sensor_info->vsync_polarity;
    out_sensor_info->hsync_polarity = in_sensor_info->hsync_polarity;
    out_sensor_info->source_width_max = in_sensor_info->source_width_max;
    out_sensor_info->source_height_max = in_sensor_info->source_height_max;
    out_sensor_info->image_effect = in_sensor_info->image_effect;
    out_sensor_info->step_count = in_sensor_info->step_count;
    out_sensor_info->preview_skip_num = in_sensor_info->preview_skip_num;
    out_sensor_info->capture_skip_num = in_sensor_info->capture_skip_num;
    out_sensor_info->flash_capture_skip_num =
        in_sensor_info->flash_capture_skip_num;
    out_sensor_info->mipi_cap_skip_num = in_sensor_info->mipi_cap_skip_num;
    out_sensor_info->video_preview_deci_num =
        in_sensor_info->video_preview_deci_num;
    out_sensor_info->threshold_eb = in_sensor_info->threshold_eb;
    out_sensor_info->threshold_mode = in_sensor_info->threshold_mode;
    out_sensor_info->threshold_start = in_sensor_info->threshold_start;
    out_sensor_info->threshold_end = in_sensor_info->threshold_end;
    out_sensor_info->raw_info_ptr = in_sensor_info->raw_info_ptr;
    out_sensor_info->sn_interface.type =
        (cmr_u32)in_sensor_info->sensor_interface.type;
    out_sensor_info->sn_interface.bus_width =
        in_sensor_info->sensor_interface.bus_width;
    out_sensor_info->sn_interface.pixel_width =
        in_sensor_info->sensor_interface.pixel_width;
    out_sensor_info->sn_interface.is_loose =
        in_sensor_info->sensor_interface.is_loose;
    if (in_sensor_info->sensor_interface.lane_switch_eb != 0) {
        out_sensor_info->sn_interface.lane_switch_eb = 1;
        out_sensor_info->sn_interface.lane_seq =
            in_sensor_info->sensor_interface.lane_seq; /*default 0x0123*/
    } else {
        out_sensor_info->sn_interface.lane_switch_eb = 0;
        cmr_u64 lane_seq = 0x0123;
        in_sensor_info->sensor_interface.lane_seq = lane_seq; /*default 0x0123*/
    }
    out_sensor_info->sn_interface.is_cphy =
        in_sensor_info->sensor_interface.is_cphy;
    if (out_sensor_info->image_format == SENSOR_IMAGE_FORMAT_YUV422) {
        out_sensor_info->image_format = CAM_IMG_FMT_YUV422P;
    } else if (out_sensor_info->image_format == SENSOR_IMAGE_FORMAT_RAW) {
        out_sensor_info->image_format = CAM_IMG_FMT_BAYER_MIPI_RAW;
    }
    for (i = 0; i < SENSOR_MODE_MAX; i++) {
        cmr_sns_copy_mode_info(&out_sensor_info->mode_info[i],
                               &in_sensor_info->sensor_mode_info[i]);
        if (out_sensor_info->mode_info[i].image_format ==
            SENSOR_IMAGE_FORMAT_YUV422) {
            out_sensor_info->mode_info[i].image_format = CAM_IMG_FMT_YUV422P;
        } else if (out_sensor_info->mode_info[i].image_format ==
                   SENSOR_IMAGE_FORMAT_RAW) {
            out_sensor_info->mode_info[i].image_format =
                CAM_IMG_FMT_BAYER_MIPI_RAW;
        }
        cmr_sns_copy_video_info(&out_sensor_info->video_info[i],
                                &in_sensor_info->sensor_video_info[i]);
    }
    out_sensor_info->view_angle.horizontal_val =
        in_sensor_info->horizontal_view_angle;
    out_sensor_info->view_angle.vertical_val =
        in_sensor_info->vertical_view_angle;
    out_sensor_info->name = (cmr_s8 *)in_sensor_info->name;
    out_sensor_info->sensor_version_info =
        (cmr_s8 *)in_sensor_info->sensor_version_info;

    return CMR_CAMERA_SUCCESS;
}

/*because use copy methods, we should get info every time when use that*/
cmr_int cmr_sensor_get_info(cmr_handle sensor_handle, cmr_uint sensor_id,
                            struct sensor_exp_info *sensor_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CHECK_HANDLE_VALID(handle);
    SENSOR_EXP_INFO_T *cur_sensor_info = NULL;
    CMR_LOGV("E");


    ret = sensor_get_info_common(&(handle->sensor_cxt[sensor_id]),
                                 &cur_sensor_info);
    if (ret) {
        CMR_LOGE("X fail, bypass this!");
        return ret;
    }

    CHECK_HANDLE_VALID(sensor_info);

    cmr_sns_copy_info(sensor_info, cur_sensor_info);
    CMR_LOGV("X");

    return ret;
}

cmr_s64 cmr_sensor_get_shutter_skew(cmr_handle sensor_handle,
        cmr_uint sensor_work_mode, cmr_uint sensor_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CHECK_HANDLE_VALID(sensor_handle);
    struct sensor_shutter_skew_info shutter_skew_info;
    memset(&shutter_skew_info, 0, sizeof(struct sensor_shutter_skew_info));
    shutter_skew_info.sns_mode = sensor_work_mode;
    shutter_skew_info.shutter_skew = -1;
    SENSOR_VAL_T val;
    val.type = SENSOR_VAL_TYPE_GET_SHUTTER_SKEW_DATA;
    val.pval = &shutter_skew_info;
    ret = cmr_sensor_ioctl(sensor_handle, sensor_id, SENSOR_ACCESS_VAL,
                        (cmr_uint)&val);
    if(ret == CMR_CAMERA_SUCCESS)
        return shutter_skew_info.shutter_skew;

    return ret;
}

cmr_int cmr_sensor_set_mode(cmr_handle sensor_handle, cmr_uint sensor_id,
                            cmr_uint mode) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    /*the set mode function can be async control*/
    message.msg_type = CMR_SENSOR_EVT_SETMODE;
    message.sub_msg_type = sensor_id;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    message.data = (void *)mode;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("X send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGV("X ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int cmr_sensor_set_mode_done(cmr_handle sensor_handle, cmr_uint sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_MSG_INIT(message);
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    message.msg_type = CMR_SENSOR_EVT_SETMODONE;
    message.sub_msg_type = sensor_id;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("X send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGV("X ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int cmr_sensor_get_mode(cmr_handle sensor_handle, cmr_uint sensor_id,
                            cmr_u32 *mode_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;

    CHECK_HANDLE_VALID(handle);
    sensor_get_mode_common(&handle->sensor_cxt[sensor_id], mode_ptr);

    return ret;
}

cmr_int cmr_sensor_update_isparm_from_file(cmr_handle sensor_handle,
                                           cmr_uint sensor_id) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    ret = sensor_update_isparm_from_file(&handle->sensor_cxt[sensor_id],
                                         sensor_id);

#if 0
	message.msg_type     = CMR_SENSOR_EVT_ISPARAM_FROM_FILE;
	message.sub_msg_type = sensor_id;
	message.sync_flag    = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
	if (ret) {
		CMR_LOGE("X send msg failed!");
		return CMR_CAMERA_FAIL;
	}
#endif
    CMR_LOGV("X ret %ld", ret);

    return ret;
}

cmr_int cmr_sensor_read_calibration_otp(cmr_handle sensor_handle, cmr_u8 dual_flag,
                                    struct sensor_otp_cust_info *otp_data, cmr_u32 camera_id){
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
            (struct cmr_sensor_handle *)sensor_handle;
    CHECK_HANDLE_VALID(handle);

    ret = sensor_read_calibration_otp(&handle->sensor_cxt[camera_id], dual_flag, otp_data);
    if (ret) {
        CMR_LOGE("cmr_sensor_read_calibration_otp failed!");
        ret = CMR_CAMERA_FAIL;
    }
    return ret;
}

cmr_int cmr_sensor_write_calibration_otp(cmr_handle sensor_handle, cmr_u8 *buf, cmr_u8 dual_flag,
	cmr_u16 otp_size,cmr_u32 camera_id){

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CHECK_HANDLE_VALID(handle);

    ret =  sensor_write_calibration_otp(&handle->sensor_cxt[camera_id],
        buf, dual_flag, otp_size);
    if (ret) {
        CMR_LOGE("cmr_sensor_write_calibration_otp failed!");
        ret = CMR_CAMERA_FAIL;
    }
    return ret;
}

cmr_int cmr_sensor_set_exif(cmr_handle sensor_handle, cmr_uint sensor_id,
                            SENSOR_EXIF_CTRL_E cmd, cmr_uint param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    // struct cmr_exif_param    exif_param = {0, 0};
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;

    return sensor_set_exif_common(&handle->sensor_cxt[sensor_id], cmd, param);

    /*
            CMR_MSG_INIT(message);
            CMR_LOGI("E");

            CHECK_HANDLE_VALID(handle);

            exif_param.cmd = cmd;
            exif_param.param = param;
            message.msg_type     = CMR_SENSOR_EVT_SETEXIF;
            message.sub_msg_type = sensor_id;
            message.sync_flag    = CMR_MSG_SYNC_RECEIVED;
            message.data         = (struct cmr_exif_param *)malloc(sizeof(struct
       cmr_exif_param));
            if (!message.data) {
                    CMR_LOGE("No mem!");
                    return CMR_CAMERA_NO_MEM;
            } else {
                    message.alloc_flag = 1;
            }
            cmr_copy(message.data, &exif_param, sizeof(struct cmr_exif_param));
            ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle,
       &message);
            if (ret) {
                    CMR_LOGE("X send msg failed!");
                    return CMR_CAMERA_FAIL;
            }
            CMR_LOGI("X ret %ld", ret);
            return ret;
    */
}

cmr_int cmr_sensor_get_exif(cmr_handle sensor_handle, cmr_uint sensor_id,
                            EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_ptr) {
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    EXIF_SPEC_PIC_TAKING_COND_T *cur_sensor_exif_ptr = NULL;

    CHECK_HANDLE_VALID(handle);
    sensor_get_exif_common(&handle->sensor_cxt[sensor_id],
                           (void **)&cur_sensor_exif_ptr);

    CHECK_HANDLE_VALID(sensor_exif_ptr);

    if (cur_sensor_exif_ptr) {
        CMR_LOGD("get sensor context exif addr %p", cur_sensor_exif_ptr);
        cmr_copy(sensor_exif_ptr, cur_sensor_exif_ptr,
                 sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
    }

    return CMR_CAMERA_SUCCESS;
}

cmr_int cmr_sensor_get_gain_thrs(cmr_handle sensor_handle, cmr_uint sensor_id,
                                 cmr_u32 *gain_thrs) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_gain_thrs_tag *gain_thrs_param = NULL;
    cmr_uint sensor_param = 0;

    gain_thrs_param = malloc(sizeof(struct sensor_gain_thrs_tag));
    if (!gain_thrs_param) {
        CMR_LOGE("fail to malloc");
        return CMR_CAMERA_NO_MEM;
    }
    gain_thrs_param->cmd = SENSOR_EXT_GAIN_OVER_THRS;
    // gain_thrs_param->param = param_ptr->cmd_value;
    sensor_param = (cmr_uint)gain_thrs_param;
    ret = cmr_sensor_ioctl(sensor_handle, sensor_id, SENSOR_GET_GAIN_THRS,
                           sensor_param);
    *gain_thrs = gain_thrs_param->gain_thrs;
    free(gain_thrs_param);
    gain_thrs_param = NULL;

    return ret;
}

cmr_int cmr_sensor_get_raw_settings(cmr_handle sensor_handle, void *raw_setting,
                                    cmr_u32 camera_id) {
    struct sensor_drv_context *sensor_cxt = NULL;
    CHECK_HANDLE_VALID(sensor_handle);
    sensor_cxt =
        &(((struct cmr_sensor_handle *)sensor_handle)->sensor_cxt[camera_id]);

    if (PNULL == sensor_cxt->sensor_info_ptr) {
        CMR_LOGE("No sensor info!");
        return -1;
    }

    /*the current function of raw setting refer is a dummy*/
    CHECK_HANDLE_VALID(raw_setting);

    if (raw_setting) {
        CMR_LOGI("get sensor_info_ptr addr %p", sensor_cxt->sensor_info_ptr);
        cmr_copy(raw_setting, sensor_cxt->sensor_info_ptr,
                 sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
    }
    return CMR_CAMERA_SUCCESS;
}

cmr_int cmr_sensor_get_fps_info(cmr_handle sensor_handle, cmr_u32 camera_id,
                                cmr_u32 sn_mode,
                                SENSOR_MODE_FPS_T_PTR fps_info_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CHECK_HANDLE_VALID(sensor_handle);
    if (NULL == fps_info_ptr) {
        CMR_LOGE("out param-sensor fps info ptr is null!You should input valid "
                 "address for sensor fps.");
        return CMR_CAMERA_INVALID_PARAM;
    }
    fps_info_ptr->mode = sn_mode;
    SENSOR_VAL_T val;
    val.type = SENSOR_VAL_TYPE_GET_FPS_INFO;
    val.pval = fps_info_ptr;
    ret = cmr_sensor_ioctl(sensor_handle, camera_id, SENSOR_ACCESS_VAL,
                           (cmr_uint)&val);
    return ret;
}

cmr_int cmr_sensor_ioctl(cmr_handle sensor_handle, cmr_u32 sensor_id,
                         cmr_uint cmd, cmr_uint arg) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sns_ioctl_param ioctl_param;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    cmr_bzero(&ioctl_param, sizeof(ioctl_param));
    ioctl_param.cmd = cmd;
    ioctl_param.arg = arg;

    /*the set mode function can be async control*/
    message.msg_type = CMR_SENSOR_EVT_IOCTL;
    message.sub_msg_type = sensor_id;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)malloc(sizeof(struct cmr_sns_ioctl_param));
    if (!message.data) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    } else {
        message.alloc_flag = 1;
    }
    cmr_copy(message.data, &ioctl_param, sizeof(struct cmr_sns_ioctl_param));

    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
	free(message.data);
	message.data = NULL;
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGV("X ret %ld", ret);
    return ret;
}

cmr_int cmr_sensor_focus_init(cmr_handle sensor_handle, cmr_u32 sensor_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_MSG_INIT(message);
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    message.msg_type = CMR_SENSOR_EVT_AFINIT;
    message.sub_msg_type = sensor_id;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    CMR_LOGV("x ret= %ld", ret);
    return ret;
}

cmr_int cmr_sensor_get_autotest_mode(cmr_handle sensor_handle,
                                     cmr_u32 sensor_id, cmr_uint *is_autotest) {
    UNUSED(sensor_id);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;

    if (!handle || !is_autotest) {
        CMR_LOGE("param err 0x%lx", (cmr_uint)sensor_handle);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    *is_autotest = handle->is_autotest;
    CMR_LOGI("auto test %d", (cmr_s32)*is_autotest);
exit:
    return ret;
}

cmr_int cmr_sensor_get_flash_info(cmr_handle sensor_handle, cmr_u32 sensor_id,
                                  struct sensor_flash_level *level) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_sensor_handle *handle =
        (struct cmr_sensor_handle *)sensor_handle;
    CHECK_HANDLE_VALID(handle);

    if (!handle || !level) {
        CMR_LOGE("param err 0x%lx", (cmr_uint)sensor_handle);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ret = sensor_get_flash_level(&handle->sensor_cxt[sensor_id], level);
    if (ret) {
        CMR_LOGE("get falsh level failed!");
        ret = CMR_CAMERA_FAIL;
    }

exit:
    CMR_LOGV("ret= %ld", ret);
    return ret;
}

/* for get sensor stream status
 * return: 1:on, 0:off
 */
cmr_int cmr_sensor_get_stream_status(cmr_handle sensor_handle,
                                     cmr_u32 sensor_id) {
    struct cmr_sensor_handle *handle =
                (struct cmr_sensor_handle *)sensor_handle;
    struct sensor_drv_context *sensor_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    sensor_cxt = &(handle->sensor_cxt[sensor_id]);

    return sensor_cxt->stream_on;
}

/**---------------------------------------------------------------------------*
 **                         Local Functions Contents                          *
 **---------------------------------------------------------------------------*/
cmr_int cmr_sns_create_thread(struct cmr_sensor_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("is_inited %ld", handle->thread_cxt.is_inited);

    if (!handle->thread_cxt.is_inited) {
        ret = cmr_thread_create(&handle->thread_cxt.thread_handle,
                                SENSOR_MSG_QUEUE_SIZE, cmr_sns_thread_proc,
                                (void *)handle, "sensor_oem");
    }

end:
    if (CMR_MSG_SUCCESS != ret) {
        handle->thread_cxt.is_inited = 0;
    } else {
        handle->thread_cxt.is_inited = 1;
    }

    CMR_LOGV("ret %ld", ret);
    return ret;
}

cmr_int cmr_sns_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 evt = 0;
    cmr_u32 ops_param = 0;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    struct internal_param *inter_param = NULL;
    struct cmr_sensor_handle *handle = (struct cmr_sensor_handle *)p_data;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;
    CMR_LOGI("evt %d", evt);

    switch (evt) {
    case CMR_SENSOR_EVT_INIT:
        /*common control info config*/
        CMR_LOGI("INIT DONE!");
        break;

    case CMR_SENSOR_EVT_OPEN:
        /*camera sensor open for every bits*/
        ops_param = (cmr_u32)((unsigned long)message->data);
        ret = cmr_sns_open(handle, ops_param);
        if (ret) {
            /* notify oem through fd_sensor */
            CMR_LOGE("cmr_sns_open failed!");
        }
        break;

    case CMR_SENSOR_EVT_CLOSE:
        /*camera sensor close for every bits*/
        ops_param = (cmr_u32)((unsigned long)message->data);
        ret = cmr_sns_close(handle, ops_param);
        if (ret) {
            /*todo, need to notify OEM that close camera sensor fail*/

            CMR_LOGE("cmr_sns_close failed!");
        }
        break;

    case CMR_SENSOR_EVT_IOCTL: {
        struct cmr_sns_ioctl_param *p_ioctl_param =
            (struct cmr_sns_ioctl_param *)message->data;
        camera_id = (cmr_u32)message->sub_msg_type;
        CMR_LOGV("camera_id=%d", camera_id);
        sensor_set_cxt_common(&handle->sensor_cxt[camera_id]);
        cmr_sns_ioctl(&handle->sensor_cxt[camera_id], p_ioctl_param->cmd,
                      p_ioctl_param->arg);
    } break;

    case CMR_SENSOR_EVT_SETEXIF: {
        struct cmr_exif_param *p_exif_param =
            (struct cmr_exif_param *)message->data;
        camera_id = (cmr_u32)message->sub_msg_type;
        sensor_set_exif_common(&handle->sensor_cxt[camera_id],
                               p_exif_param->cmd, p_exif_param->param);
    } break;

    case CMR_SENSOR_EVT_SETMODE:
        camera_id = (cmr_u32)message->sub_msg_type;
        sensor_set_mode_common(&handle->sensor_cxt[camera_id],
                               (cmr_u32)((unsigned long)message->data));
        break;

    case CMR_SENSOR_EVT_SETMODONE:
        CMR_LOGI("SENSOR_EVT_SET_MODE_DONE_OK");
        camera_id = (cmr_u32)message->sub_msg_type;
        sensor_set_mode_done_common(
            &handle->sensor_cxt[camera_id]); // for debug
        break;

    case CMR_SENSOR_EVT_STREAM:
        camera_id = (cmr_u32)message->sub_msg_type;
        ops_param = (cmr_u32)((unsigned long)message->data);
        ret = sensor_stream_ctrl_common(&handle->sensor_cxt[camera_id],
                                        ops_param);
        if (ret) {
            CMR_LOGE("sensor_stream_ctrl_common failed!");
        }
        break;

    case CMR_SENSOR_EVT_AFINIT:
        camera_id = (cmr_u32)message->sub_msg_type;
        CMR_LOGI("SENSOR_EVT_AF_INIT");
        ret = cmr_sns_af_init(&handle->sensor_cxt[camera_id]);
        CMR_LOGI("SENSOR_EVT_AF_INIT, Done");
        break;

    case CMR_SENSOR_EVT_ISPARAM_FROM_FILE:
        camera_id = (cmr_u32)message->sub_msg_type;
        ret = sensor_update_isparm_from_file(&handle->sensor_cxt[camera_id],
                                             camera_id);
        break;

    case CMR_SENSOR_EVT_EXIT:
        /*common control info clear*/
        CMR_LOGI("EXIT DONE!");
        break;

    default:
        CMR_LOGE("jpeg:not correct message");
        break;
    }

    return ret;
}

cmr_int cmr_sns_destroy_thread(struct cmr_sensor_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("is_inited %ld", handle->thread_cxt.is_inited);

    if (handle->thread_cxt.is_inited) {
        ret = cmr_thread_destroy(handle->thread_cxt.thread_handle);
        handle->thread_cxt.thread_handle = 0;
        handle->thread_cxt.is_inited = 0;
    }

    return ret;
}

cmr_int cmr_sns_open(struct cmr_sensor_handle *handle, cmr_u32 sensor_id_bits) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 cameraId = 0;
    cmr_u32 cameraCnt = 0;

    CHECK_HANDLE_VALID(handle);

    /*open all signed camera sensor*/
    for (cameraId = 0; cameraId < CAMERA_ID_MAX; cameraId++) {
        if (0 != (sensor_id_bits & (1 << cameraId))) {
            ret = sensor_open_common(&handle->sensor_cxt[cameraId], cameraId,
                                     handle->is_autotest);
            if (ret) {
                CMR_LOGE("camera %u open failed!", cameraId);
                goto exit;
            }
            handle->sensor_bits |= (1 << cameraId);
        }
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int cmr_sns_close(struct cmr_sensor_handle *handle,
                      cmr_u32 sensor_id_bits) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 cameraId = 0;
    cmr_u32 cameraCnt = 0;

    CHECK_HANDLE_VALID(handle);

    /*close all signed camera sensor*/
    for (cameraId = 0; cameraId < CAMERA_ID_MAX; cameraId++) {
        if (0 != (sensor_id_bits & (1 << cameraId))) {
            ret = sensor_close_common(&handle->sensor_cxt[cameraId], cameraId);
            if (ret) {
                CMR_LOGE("camera %u open failed!", cameraId);
            } else {
                handle->sensor_bits &= ~(1 << cameraId);
                CMR_LOGI("close sensor ok");
                /*cmr_sns_af_init(&handle->sensor_cxt[cameraId]);*/
            }
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int cmr_sns_get_ioctl_cmd(SENSOR_IOCTL_CMD_E *sns_cmd,
                              enum sensor_cmd in_cmd) {
    cmr_u32 ret = CMR_CAMERA_SUCCESS;

    switch (in_cmd) {
    case SENSOR_WRITE_REG:
        *sns_cmd = SENSOR_IOCTL_WRITE_REG;
        break;

    case SENSOR_READ_REG:
        *sns_cmd = SENSOR_IOCTL_READ_REG;
        break;

    case SENSOR_BRIGHTNESS:
        *sns_cmd = SENSOR_IOCTL_BRIGHTNESS;
        break;

    case SENSOR_CONTRAST:
        *sns_cmd = SENSOR_IOCTL_CONTRAST;
        break;

    case SENSOR_SHARPNESS:
        *sns_cmd = SENSOR_IOCTL_SHARPNESS;
        break;

    case SENSOR_SATURATION:
        *sns_cmd = SENSOR_IOCTL_SATURATION;
        break;

    case SENSOR_SCENE:
        *sns_cmd = SENSOR_IOCTL_PREVIEWMODE;
        break;

    case SENSOR_IMAGE_EFFECT:
        *sns_cmd = SENSOR_IOCTL_IMAGE_EFFECT;
        break;

    case SENSOR_BEFORE_SNAPSHOT:
        *sns_cmd = SENSOR_IOCTL_BEFORE_SNAPSHOT;
        break;

    case SENSOR_AFTER_SNAPSHOT:
        *sns_cmd = SENSOR_IOCTL_AFTER_SNAPSHOT;
        break;

    case SENSOR_CHECK_NEED_FLASH:
        *sns_cmd = SENSOR_IOCTL_FLASH;
        break;

    case SENSOR_WRITE_EV:
        *sns_cmd = SENSOR_IOCTL_WRITE_EV;
        break;

    case SENSOR_WRITE_GAIN:
        *sns_cmd = SENSOR_IOCTL_WRITE_GAIN;
        break;

    case SENSOR_SET_AF_POS:
        *sns_cmd = SENSOR_IOCTL_AF_ENABLE;
        break;

    case SENSOR_SET_WB_MODE:
        *sns_cmd = SENSOR_IOCTL_SET_WB_MODE;
        break;

    case SENSOR_ISO:
        *sns_cmd = SENSOR_IOCTL_ISO;
        break;

    case SENSOR_EXPOSURE_COMPENSATION:
        *sns_cmd = SENSOR_IOCTL_EXPOSURE_COMPENSATION;
        break;

    case SENSOR_GET_EXIF:
        /*this function is complete in other function, use a dummy function for
         * instead*/
        *sns_cmd = SENSOR_IOCTL_GET_STATUS;
        break;

    case SENSOR_FOCUS:
        *sns_cmd = SENSOR_IOCTL_FOCUS;
        CMR_LOGD("SENSOR_FOCUS --> SENSOR_IOCTL_FOCUS ");
        break;

    case SENSOR_ANTI_BANDING:
        *sns_cmd = SENSOR_IOCTL_ANTI_BANDING_FLICKER;
        break;

    case SENSOR_VIDEO_MODE:
        *sns_cmd = SENSOR_IOCTL_VIDEO_MODE;
        break;

    case SENSOR_STREAM_ON:
        *sns_cmd = SENSOR_IOCTL_STREAM_ON;
        break;

    case SENSOR_STREAM_OFF:
        *sns_cmd = SENSOR_IOCTL_STREAM_OFF;
        break;

    case SENSOR_SET_HDR_EV:
        *sns_cmd = SENSOR_IOCTL_FOCUS;
        break;

    case SENSOR_GET_GAIN_THRS:
        *sns_cmd = SENSOR_IOCTL_FOCUS;
        break;

    case SENSOR_RESTORE:
        /*this function is not used yet, use a dummy function for instead*/
        *sns_cmd = SENSOR_IOCTL_GET_STATUS;
        break;

    case SENSOR_WRITE:
        /*this function is not used yet, use a dummy function for instead*/
        *sns_cmd = SENSOR_IOCTL_GET_STATUS;
        break;

    case SENSOR_READ:
        /*this function is not used yet, use a dummy function for instead*/
        *sns_cmd = SENSOR_IOCTL_GET_STATUS;
        break;

    case SENSOR_MIRROR:
        *sns_cmd = SENSOR_IOCTL_HMIRROR_ENABLE;
        break;

    case SENSOR_FLIP:
        *sns_cmd = SENSOR_IOCTL_VMIRROR_ENABLE;
        break;

    case SENSOR_MONITOR:
        /*this function is not used yet, use a dummy function for instead*/
        *sns_cmd = SENSOR_IOCTL_GET_STATUS;
        break;

    case SENSOR_GET_STATUS:
        *sns_cmd = SENSOR_IOCTL_GET_STATUS;
        break;

    case SENSOR_ACCESS_VAL:
        *sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
        break;

    case SENSOR_YUV_FPS:
        *sns_cmd = SENSOR_IOCTL_FOCUS;
        CMR_LOGE("SENSOR_YUV_FPS --> SENSOR_IOCTL_FOCUS ");
        break;
    default:
        ret = CMR_CAMERA_INVALID_PARAM;
        break;
    }
    return ret;
}

cmr_int cmr_get_otp_from_kernel(struct sensor_drv_context *sensor_cxt,
                                cmr_uint cmd, cmr_uint arg,
                                SENSOR_IOCTL_FUNC_PTR func_ptr,
                                cmr_u8 *read_flag) {
    cmr_u32 ret = CMR_CAMERA_SUCCESS;
    struct _sensor_otp_param_tag param_ptr;
    struct sensor_data_info sensor_otp;
    uint32_t otp_data_len = 0;
    SENSOR_VAL_T *val = (SENSOR_VAL_T *)arg;
    cmr_int fd_sensor = sensor_cxt->fd_sensor;

    *read_flag = 0;
    if ((SENSOR_ACCESS_VAL == cmd) && SENSOR_VAL_TYPE_READ_OTP == val->type) {
        cmr_bzero(&param_ptr, sizeof(param_ptr));
        cmr_bzero(&sensor_otp, sizeof(sensor_otp));
        otp_data_len = CMR_ISP_OTP_MAX_SIZE;
        sensor_otp.data_ptr = malloc(otp_data_len);
        if (NULL == sensor_otp.data_ptr) {
            CMR_LOGE("malloc file failed");
            return -1;
        }

        cmr_bzero(sensor_otp.data_ptr, otp_data_len);
        if (SENSOR_VAL_TYPE_READ_OTP == val->type) {
            param_ptr.type = SENSOR_OTP_PARAM_NORMAL;
            param_ptr.start_addr = 0;
            param_ptr.len = 0;
            param_ptr.buff = sensor_otp.data_ptr;

            ret = ioctl(fd_sensor, SENSOR_IO_READ_OTPDATA, &param_ptr);
            if (!ret && 0 != param_ptr.buff[0]) {
                CMR_LOGD("SENSOR_IO_READ_OTPDATA  OK");
                val->pval = param_ptr.buff;
                val->type = SENSOR_VAL_TYPE_PARSE_OTP;
            }
        } else if (SENSOR_VAL_TYPE_READ_DUAL_OTP == val->type) {
            param_ptr.type = SENSOR_OTP_PARAM_NORMAL;
            param_ptr.start_addr = 0;
            param_ptr.len = 0;
            param_ptr.buff = sensor_otp.data_ptr;

            ret = ioctl(fd_sensor, SENSOR_IO_READ_OTPDATA, &param_ptr);
            if (!ret && 0 != param_ptr.buff[0]) {
                CMR_LOGD("SENSOR_IO_READ_OTPDATA dual OK,param_ptr.buff %p",
                         param_ptr.buff);
                val->pval = param_ptr.buff;
                val->type = SENSOR_VAL_TYPE_PARSE_DUAL_OTP;
            }
        }
        if (PNULL != func_ptr) {
            ret = func_ptr(sensor_cxt->sensor_hw_handler, arg);
            *read_flag = 1;
        }
        if (NULL != sensor_otp.data_ptr) {
            CMR_LOGD("SENSOR_IO_READ_OTPDATA  free buff,val->pval %p",
                     val->pval);
            free(sensor_otp.data_ptr);
            sensor_otp.data_ptr = NULL;
        }
    }
    return ret;
}

cmr_int cmr_sns_ioctl(struct sensor_drv_context *sensor_cxt, cmr_uint cmd,
                      cmr_uint arg) {
    SENSOR_IOCTL_FUNC_PTR func_ptr = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;
    cmr_uint temp;
    cmr_u32 ret = CMR_CAMERA_SUCCESS;
    cmr_u32 sns_cmd = SENSOR_IOCTL_GET_STATUS;
    cmr_u8 read_flag = 0;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    ret = cmr_sns_get_ioctl_cmd(&sns_cmd, cmd);
    if (ret) {
        CMR_LOGE("can't get correct command !\n");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (SENSOR_IOCTL_GET_STATUS != sns_cmd) {
        CMR_LOGI("cmd = %d, arg = 0x%lx.\n", sns_cmd, arg);
    }

    if (!sensor_is_init_common(sensor_cxt)) {
        CMR_LOGE("sensor has not init.\n");
        return SENSOR_OP_STATUS_ERR;
    }

    if (SENSOR_IOCTL_CUS_FUNC_1 > sns_cmd) {
        CMR_LOGW("can't access internal command !\n");
        return SENSOR_SUCCESS;
    }

    if (PNULL == sensor_cxt->sensor_info_ptr) {
        CMR_LOGE("No sensor info!");
        return -1;
    }

#if 1
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    // func_ptr = sns_ops->ext_ops[sns_cmd].ops;
    if (!module->otp_drv_info.otp_drv_entry) {
        /*ret =
            cmr_get_otp_from_kernel(sensor_cxt, cmd, arg, func_ptr, &read_flag);
        if (read_flag) {
            return ret;
        }*/
    } else {
        if (cmd == SENSOR_ACCESS_VAL) {
            SENSOR_VAL_T *val = (SENSOR_VAL_T *)arg;
            if (val->type == SENSOR_VAL_TYPE_READ_OTP ||
                val->type == SENSOR_VAL_TYPE_READ_DUAL_OTP)
                sensor_drv_ioctl(sensor_cxt,
                                 CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT,
                                 (void *)arg);
        }
    }

    if (PNULL != sns_ops->ext_ops[sns_cmd].ops) {
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle, arg);
    }
#endif
    return ret;
}

static cmr_int cmr_sns_af_init(struct sensor_drv_context *sensor_cxt) {
    SENSOR_EXT_FUN_PARAM_T af_param;
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGV("E");

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    af_param.cmd = SENSOR_EXT_FUNC_INIT;
    af_param.param = SENSOR_EXT_FOCUS_TRIG;
    ret = cmr_sns_ioctl(sensor_cxt, SENSOR_FOCUS, (cmr_uint)&af_param);
    if (ret) {
        CMR_LOGE("Failed to init AF");
    } else {
        CMR_LOGI("OK to init auto focus");
    }

    return ret;
}
