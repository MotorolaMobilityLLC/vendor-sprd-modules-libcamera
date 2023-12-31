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
#define LOG_TAG "cmr_setting"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#if (MINICAMERA != 1)
#include <math.h>
#endif
#include <cutils/trace.h>
#include <cutils/properties.h>
#include "cmr_setting.h"
#include "isp_app.h"
#include "cmr_msg.h"
#include "cmr_common.h"
#include "sensor_drv_u.h"
#include "cmr_exif.h"
#include "cmr_oem.h"

#define SETTING_MSG_QUEUE_SIZE 5

#define SETTING_EVT_INIT (1 << 16)
#define SETTING_EVT_DEINIT (1 << 17)
#define SETTING_EVT_ZOOM (1 << 18)

#define SETTING_EVT_MASK_BITS                                                  \
    (cmr_uint)(SETTING_EVT_INIT | SETTING_EVT_DEINIT | SETTING_EVT_ZOOM)

#define INVALID_SETTING_BYTE 0xFF

#define DV_FLASH_ON_DV_WITH_PREVIEW 1

// pre-flash interval time
#define PREFLASH_INTERVAL_TIME 3

enum exif_orientation {
    ORIENTATION_UNDEFINED = 0,
    ORIENTATION_NORMAL = 1,
    ORIENTATION_FLIP_HORIZONTAL = 2, /* left right reversed mirror */
    ORIENTATION_ROTATE_180 = 3,
    ORIENTATION_FLIP_VERTICAL = 4, /* upside down mirror */
    ORIENTATION_TRANSPOSE =
        5, /* flipped about top-left <--> bottom-right axis */
    ORIENTATION_ROTATE_90 = 6, /* rotate 90 cw to right it */
    ORIENTATION_TRANSVERSE =
        7, /* flipped about top-right <--> bottom-left axis */
    ORIENTATION_ROTATE_270 = 8 /* rotate 270 to right it */
};

enum setting_general_type {
    SETTING_GENERAL_CONTRAST,
    SETTING_GENERAL_BRIGHTNESS,
    SETTING_GENERAL_SHARPNESS,
    SETTING_GENERAL_WB,
    SETTING_GENERAL_EFFECT,
    SETTING_GENERAL_ANTIBANDING,
    SETTING_GENERAL_AUTO_EXPOSURE_MODE,
    SETTING_GENERAL_ISO,
    SETTING_GENERAL_EXPOSURE_COMPENSATION,
    SETTING_GENERAL_PREVIEW_FPS,
    SETTING_GENERAL_PREVIEW_LLS_FPS,
    SETTING_GENERAL_SATURATION,
    SETTING_GENERAL_SCENE_MODE,
    SETTING_GENERAL_SENSOR_ROTATION,
    SETTING_GENERAL_AE_LOCK_UNLOCK,
    SETTING_GENERAL_AWB_LOCK_UNLOCK,
    SETTING_GENERAL_AE_MODE,
    SETTING_GENERAL_FOCUS_DISTANCE,
    SETTING_GENERAL_AUTO_HDR,
    SETTING_GENERAL_SPRD_APP_MODE,
    SETTING_GENERAL_AI_SCENE_ENABLED,
    SETTING_GENERAL_AUTO_3DNR,
    SETTING_GENERAL_EXPOSURE_TIME,
    SETTING_GENERAL_AUTO_TRACKING_INFO_ENABLE,
    SETTING_GENERAL_AUTO_FDR,
    SETTING_GENERAL_ZOOM,
    SETTING_GENERAL_TYPE_MAX
};

struct setting_exif_unit {
    struct img_size actual_picture_size;
    struct img_size picture_size;
};
struct setting_local_param {
    cmr_uint is_dv_mode;
    struct exif_info_tag exif_all_info;
    struct sensor_exp_info sensor_static_info;
    cmr_uint is_sensor_info_store;
    struct setting_exif_unit exif_unit;
    EXIF_SPEC_PIC_TAKING_COND_T exif_pic_taking;
};

struct setting_flash_param {
    cmr_uint flash_mode;
    enum setting_flash_status flash_status;
    cmr_uint has_preflashed;
    cmr_s64 last_preflash_time;
    cmr_uint flash_hw_status;
};

struct setting_hal_common {
    cmr_uint focal_length;
    cmr_uint brightness;
    cmr_uint contrast;
    cmr_uint effect;
    cmr_uint wb_mode;
    cmr_uint exposure_time;
    cmr_uint saturation;
    cmr_uint sharpness;
    cmr_uint scene_mode;
    cmr_uint antibanding_mode;
    cmr_uint iso;
    cmr_uint video_mode;
    cmr_uint frame_rate;
    cmr_uint auto_exposure_mode;
    cmr_uint ae_mode;
    cmr_uint focus_distance;
    cmr_uint is_auto_hdr;
    cmr_uint sprd_appmode_id;
    cmr_uint ai_scene;
    cmr_uint is_auto_3dnr;
    cmr_uint is_auto_tracking;
    cmr_uint is_auto_fdr;
    struct cmr_ae_compensation_param ae_compensation_param;
};

enum zoom_status { ZOOM_IDLE, ZOOM_UPDATING };

struct setting_zoom_unit {
    struct setting_cmd_parameter in_zoom;
    cmr_int is_changed;
    cmr_int status;
    cmr_int is_sended_msg;
};

struct setting_hal_param {
    struct setting_hal_common hal_common;
    struct cmr_zoom_param zoom_value;
    struct cmr_zoom_param zoom_reprocess;
    cmr_uint ratio_value;
    cmr_uint sensor_orientation; /*screen orientation: landscape and portrait */
    cmr_uint capture_angle;

    cmr_uint jpeg_quality;
    cmr_uint thumb_quality;
    cmr_uint is_rotation_capture;
    cmr_uint encode_angle;
    cmr_uint encode_rotation;

    struct img_size preview_size;
    struct img_size raw_capture_size;
    cmr_uint preview_format;
    cmr_uint preview_angle;

    struct img_size video_size;
    cmr_uint video_format;
    struct img_size thumb_size;
    struct img_size capture_size;
    struct img_size yuv_callback_size;
    cmr_uint yuv_callback_format;
    struct img_size raw_size;
    cmr_uint raw_format;
    struct img_size yuv2_size;
    cmr_uint yuv2_format;
    struct beauty_info perfect_skinlevel;
    cmr_uint capture_format;
    cmr_uint capture_mode;
    cmr_uint shot_num;
    cmr_uint flip_on;

    struct setting_flash_param flash_param;

    struct camera_position_type position_info;
    cmr_uint is_hdr;
    cmr_uint is_fdr;
    cmr_uint is_3dnr;
    cmr_uint sprd_3dnr_type;
    cmr_uint is_cnr;
    cmr_uint is_ee;
    cmr_uint is_android_zsl;
    cmr_uint app_mode;
    struct cmr_range_fps_param range_fps;
    cmr_uint is_update_range_fps;
    cmr_uint sprd_zsl_enabled;
    cmr_uint sprd_afbc_enabled;
    cmr_uint video_slow_motion_flag;
    cmr_uint sprd_pipviv_enabled;
    cmr_uint sprd_eis_enabled;
    cmr_uint is_ae_lock;
    cmr_uint refoucs_enable;
    struct touch_coordinate touch_info;
    cmr_uint video_snapshot_type;
    cmr_uint sprd_3dcalibration_enable;
    cmr_uint sprd_yuv_callback_enable;
    cmr_uint is_awb_lock;
    cmr_uint exif_mime_type;
    cmr_uint sprd_filter_type;
    EXIF_RATIONAL_T ExposureTime;
    cmr_uint device_orientation;
    cmr_uint ot_status;
    cmr_uint face_attributes_enabled;
    cmr_uint smile_capture_enabled;
    cmr_uint sprd_logo_watermark;
    cmr_uint sprd_time_watermark;
    struct img_size originalPictureSize;
    cmr_uint is_super_macrophoto;
};

struct setting_camera_info {
    struct setting_hal_param hal_param;
    struct setting_local_param local_param;
};

struct setting_component {
    cmr_handle thread_handle;
    pthread_mutex_t status_lock;
    pthread_mutex_t ctrl_lock;
    struct setting_init_in init_in;
    struct setting_camera_info camera_info[CAMERA_ID_MAX];
    struct setting_zoom_unit zoom_unit;
    cmr_int force_set;
    uint32_t isp_is_timeout;
    sem_t isp_sem;
    pthread_mutex_t isp_mutex;
    sem_t quick_ae_sem;
    sem_t preflash_sem;
    uint32_t flash_need_quit;
};

struct setting_item {
    cmr_uint cmd_type;
    cmr_int (*setting_ioctl)(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm);
};

struct setting_general_item {
    cmr_uint general_type;
    cmr_uint *cmd_type_value;
    cmr_int isp_cmd;
    cmr_int sn_cmd;
};

struct setting_exif_cb_param {
    struct setting_component *cpt;
    struct setting_cmd_parameter *parm;
};
typedef cmr_int (*setting_ioctl_fun_ptr)(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm);

static cmr_int setting_isp_wait_notice_withtime(struct setting_component *cpt,
                                                cmr_uint timeout);
static cmr_int cmr_setting_clear_sem(struct setting_component *cpt);
/** LOCAL FUNCTION DECLARATION
 */
static cmr_int setting_set_flashdevice(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm,
                                       enum sprd_flash_status flash_status);
static cmr_int setting_isp_wait_notice(struct setting_component *cpt);
/**FUNCTION
 */
static struct setting_hal_param *get_hal_param(struct setting_component *cpt,
                                               cmr_uint camera_id) {
    return &cpt->camera_info[camera_id].hal_param;
}

static struct setting_local_param *
get_local_param(struct setting_component *cpt, cmr_uint camera_id) {
    return &cpt->camera_info[camera_id].local_param;
}

static struct setting_flash_param *
get_flash_param(struct setting_component *cpt, cmr_uint camera_id) {
    return &cpt->camera_info[camera_id].hal_param.flash_param;
}

static cmr_int
setting_get_sensor_static_info(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm,
                               struct sensor_exp_info *static_info) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    cmr_uint cmd = COM_SN_GET_INFO;
    struct common_sn_cmd_param sn_param;

    if (!local_param->is_sensor_info_store && init_in->setting_sn_ioctl) {
        sn_param.camera_id = parm->camera_id;
        ret = (*init_in->setting_sn_ioctl)(init_in->oem_handle, cmd, &sn_param);
        local_param->is_sensor_info_store = 1;
        cmr_copy(static_info, &sn_param.sensor_static_info,
                 sizeof(sn_param.sensor_static_info));
    }

    return ret;
}

static cmr_int setting_is_rawrgb_format(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int is_raw = 0;
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);

    setting_get_sensor_static_info(cpt, parm, &local_param->sensor_static_info);
    if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
        local_param->sensor_static_info.image_format) {
        is_raw = 1;
    }

    return is_raw;
}

static cmr_uint setting_is_active(struct setting_component *cpt) {
    struct setting_init_in *init_in = &cpt->init_in;
    cmr_uint is_active = 0;

    if (init_in->get_setting_activity) {
        (*init_in->get_setting_activity)(init_in->oem_handle, &is_active);
    }

    return is_active;
}

static cmr_uint setting_get_skip_number(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm,
                                        cmr_int is_check_night_mode) {
    cmr_uint skip_number = 0;
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    setting_get_sensor_static_info(cpt, parm, &local_param->sensor_static_info);

    if (!is_check_night_mode) {
        skip_number =
            (local_param->sensor_static_info.change_setting_skip_num & 0xffff);
    } else {
        if (CAMERA_SCENE_MODE_NIGHT == hal_param->hal_common.scene_mode) {
            skip_number =
                ((local_param->sensor_static_info.change_setting_skip_num >>
                  16) &
                 0xffff);
        }
    }

    return skip_number;
}

static cmr_int setting_sn_ctrl(struct setting_component *cpt, cmr_uint sn_cmd,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct common_sn_cmd_param sn_param;

    if (sn_cmd >= COM_SN_TYPE_MAX) {
        return ret;
    }

    if (init_in->setting_sn_ioctl) {
        sn_param.camera_id = parm->camera_id;
        sn_param.cmd_value = parm->cmd_type_value;
        ret = (*init_in->setting_sn_ioctl)(init_in->oem_handle, sn_cmd,
                                           &sn_param);
        if (ret) {
            CMR_LOGW("sn ctrl failed");
        }
        parm->cmd_type_value = sn_param.cmd_value;
    }

    return ret;
}

static cmr_uint camera_param_to_isp(cmr_uint cmd,
                                    struct setting_cmd_parameter *parm,
                                    struct common_isp_cmd_param *isp_param) {
    cmr_u64 in_param = parm->cmd_type_value;
    cmr_u64 out_param = in_param;

    switch (cmd) {
    case COM_ISP_SET_AE_MODE: {
        switch (in_param) {
        case CAMERA_SCENE_MODE_AUTO:
        case CAMERA_SCENE_MODE_NORMAL:
            out_param = ISP_AUTO;
            break;

        case CAMERA_SCENE_MODE_NIGHT:
            out_param = ISP_NIGHT;
            break;

        case CAMERA_SCENE_MODE_ACTION:
            out_param = ISP_SPORT;
            break;

        case CAMERA_SCENE_MODE_PORTRAIT:
            out_param = ISP_PORTRAIT;
            break;

        case CAMERA_SCENE_MODE_LANDSCAPE:
            out_param = ISP_LANDSCAPE;
            break;

        case CAMERA_SCENE_MODE_PANORAMA:
            out_param = ISP_PANORAMA;
            break;
        case CAMERA_SCENE_MODE_VIDEO:
            out_param = ISP_VIDEO;
            break;
        case CAMERA_SCENE_MODE_SLOWMOTION:
            out_param = ISP_SPORT;
            break;

        case CAMERA_SCENE_MODE_HDR:
            out_param = ISP_HDR;
            break;

        case CAMERA_SCENE_MODE_VIDEO_EIS:
            out_param = ISP_VIDEO_EIS;
            break;

        default:
            out_param = CAMERA_SCENE_MODE_AUTO;
            break;
        }
        isp_param->cmd_value = out_param;
        break;
    }

    case COM_ISP_SET_VIDEO_MODE:
        isp_param->cmd_value = parm->preview_fps_param.frame_rate;
        break;
    case COM_ISP_SET_RANGE_FPS:
        isp_param->range_fps = parm->range_fps;
        break;
    case COM_ISP_SET_EV:
        isp_param->ae_compensation_param = parm->ae_compensation_param;
        break;
    default:
        isp_param->cmd_value = out_param;
        break;
    }

    return out_param;
}

static cmr_int setting_isp_ctrl(struct setting_component *cpt, cmr_uint isp_cmd,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct common_isp_cmd_param isp_param;
    struct setting_hal_param *hal_param = NULL;

    if (isp_cmd >= COM_ISP_TYPE_MAX) {
        return ret;
    }

    if (init_in->setting_isp_ioctl) {
        isp_param.camera_id = parm->camera_id;
        if(COM_ISP_SET_EXPOSURE_TIME == isp_cmd) {
            parm->cmd_type_value = parm->cmd_type_value * 1000;
        }
	  CMR_LOGD("cmd_type_value=%"PRIu64"",parm->cmd_type_value);
        camera_param_to_isp(isp_cmd, parm, &isp_param);
        if (isp_cmd == COM_ISP_SET_AE_MODE) {
            hal_param = get_hal_param(cpt, parm->camera_id);
            CMR_LOGD("app_mode:%d, scene_mode:%d", hal_param->app_mode,
                     parm->cmd_type_value);
            if (hal_param->app_mode == CAMERA_MODE_FDR) {
                 isp_param.cmd_value = ISP_FDR;
            }
        }
        if (COM_ISP_SET_FPS_LLS_MODE == isp_cmd) {
            isp_param.fps_param.min_fps = parm->preview_fps_param.frame_rate;
            isp_param.fps_param.max_fps = parm->preview_fps_param.video_mode;
        }

        ret = (*init_in->setting_isp_ioctl)(init_in->oem_handle, isp_cmd,
                                            &isp_param);
        if (ret) {
            CMR_LOGE("sn ctrl failed");
        }
        parm->cmd_type_value = isp_param.cmd_value;
    }

    return ret;
}

static cmr_int setting_before_set_ctrl(struct setting_component *cpt,
                                       enum preview_param_mode mode) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;

    if (init_in->before_set_cb) {
        ret = (*init_in->before_set_cb)(init_in->oem_handle, mode);
        if (ret) {
            CMR_LOGE("before cb failed");
        }
    }

    return ret;
}

static cmr_int setting_after_set_ctrl(struct setting_component *cpt,
                                      struct after_set_cb_param *param) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;

    if (init_in->after_set_cb) {
        ret = (*init_in->after_set_cb)(init_in->oem_handle, param);
        if (ret) {
            CMR_LOGE("before cb failed");
        }
    }

    return ret;
}

static uint32_t setting_get_exif_orientation(int degrees) {
    uint32_t orientation = 1; /*ExifInterface.ORIENTATION_NORMAL; */

    degrees %= 360;
    if (degrees < 0)
        degrees += 360;
    if (degrees < 45) {
        orientation = ORIENTATION_NORMAL; /*ExifInterface.ORIENTATION_NORMAL; */
    } else if (degrees < 135) {
        orientation =
            ORIENTATION_ROTATE_90; /*ExifInterface.ORIENTATION_ROTATE_90; */
    } else if (degrees < 225) {
        orientation =
            ORIENTATION_ROTATE_180; /*ExifInterface.ORIENTATION_ROTATE_180; */
    } else {
        orientation =
            ORIENTATION_ROTATE_270; /*ExifInterface.ORIENTATION_ROTATE_270; */
    }
    CMR_LOGD("rotation degrees: %d, orientation: %d.", degrees, orientation);
    return orientation;
}

static cmr_int setting_set_general(struct setting_component *cpt,
                                   enum setting_general_type type,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    cmr_uint type_val = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    enum img_skip_mode skip_mode = 0;
    cmr_uint skip_number = 0;
    struct setting_general_item general_list[] = {
        {SETTING_GENERAL_CONTRAST, &hal_param->hal_common.contrast,
         COM_ISP_SET_CONTRAST, COM_SN_SET_CONTRAST},
        {SETTING_GENERAL_BRIGHTNESS, &hal_param->hal_common.brightness,
         COM_ISP_SET_BRIGHTNESS, COM_SN_SET_BRIGHTNESS},
        {SETTING_GENERAL_SHARPNESS, &hal_param->hal_common.sharpness,
         COM_ISP_SET_SHARPNESS, COM_SN_SET_SHARPNESS},
        {SETTING_GENERAL_WB, &hal_param->hal_common.wb_mode,
         COM_ISP_SET_AWB_MODE, COM_SN_SET_WB_MODE},
        {SETTING_GENERAL_EFFECT, &hal_param->hal_common.effect,
         COM_ISP_SET_SPECIAL_EFFECT, COM_SN_SET_IMAGE_EFFECT},
        {SETTING_GENERAL_ANTIBANDING, &hal_param->hal_common.antibanding_mode,
         COM_ISP_SET_ANTI_BANDING, COM_SN_SET_ANTI_BANDING},
        {SETTING_GENERAL_AUTO_EXPOSURE_MODE,
         &hal_param->hal_common.auto_exposure_mode, COM_ISP_SET_AE_MEASURE_LUM,
         COM_SN_TYPE_MAX},
        {SETTING_GENERAL_ISO, &hal_param->hal_common.iso, COM_ISP_SET_ISO,
         COM_SN_SET_ISO},
        {SETTING_GENERAL_EXPOSURE_COMPENSATION,
         (cmr_uint *)&hal_param->hal_common.ae_compensation_param
             .ae_exposure_compensation,
         COM_ISP_SET_EV, COM_SN_SET_EXPOSURE_COMPENSATION},
        {SETTING_GENERAL_PREVIEW_FPS, &hal_param->hal_common.frame_rate,
         COM_ISP_SET_VIDEO_MODE, COM_SN_SET_VIDEO_MODE},
        {SETTING_GENERAL_PREVIEW_LLS_FPS, &hal_param->hal_common.frame_rate,
         COM_ISP_SET_FPS_LLS_MODE, COM_SN_SET_FPS_LLS_MODE},
        {SETTING_GENERAL_SATURATION, &hal_param->hal_common.saturation,
         COM_ISP_SET_SATURATION, COM_SN_SET_SATURATION},
        {SETTING_GENERAL_SCENE_MODE, &hal_param->hal_common.scene_mode,
         COM_ISP_SET_AE_MODE, COM_SN_SET_PREVIEW_MODE},
        {SETTING_GENERAL_SENSOR_ROTATION, &hal_param->capture_angle,
         COM_ISP_TYPE_MAX, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AE_LOCK_UNLOCK, &hal_param->is_ae_lock,
         COM_ISP_SET_AE_LOCK_UNLOCK, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AWB_LOCK_UNLOCK, &hal_param->is_awb_lock,
         COM_ISP_SET_AWB_LOCK_UNLOCK, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AE_MODE, &hal_param->hal_common.ae_mode,
         COM_ISP_SET_AE_MODE_CONTROL, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_FOCUS_DISTANCE, &hal_param->hal_common.focus_distance,
         COM_ISP_SET_AF_POS, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AUTO_HDR, &hal_param->hal_common.is_auto_hdr,
         COM_ISP_SET_AUTO_HDR, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_SPRD_APP_MODE, &hal_param->hal_common.sprd_appmode_id,
         COM_ISP_SET_SPRD_APP_MODE, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AI_SCENE_ENABLED, &hal_param->hal_common.ai_scene,
         COM_ISP_SET_AI_SCENE_ENABLED, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AUTO_3DNR, &hal_param->hal_common.is_auto_3dnr,
         COM_ISP_SET_AUTO_3DNR, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_EXPOSURE_TIME, &hal_param->hal_common.exposure_time,
         COM_ISP_SET_EXPOSURE_TIME, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AUTO_TRACKING_INFO_ENABLE, &hal_param->hal_common.is_auto_tracking,
         COM_ISP_SET_AUTO_TRACKING_ENABLE, COM_SN_TYPE_MAX},
        {SETTING_GENERAL_AUTO_FDR, &hal_param->hal_common.is_auto_fdr,
         COM_ISP_SET_AUTO_FDR, COM_SN_TYPE_MAX}
    };
    struct setting_general_item *item = NULL;
    struct after_set_cb_param after_cb_param;
    cmr_int is_check_night_mode = 0;

    if (type >= SETTING_GENERAL_ZOOM) {
        CMR_LOGE("type is invalid");
        return -CMR_CAMERA_INVALID_PARAM;
    }

    item = &general_list[type];
    switch (type) {
    case SETTING_GENERAL_AUTO_EXPOSURE_MODE:
        type_val = parm->ae_param.mode;
        break;
    case SETTING_GENERAL_PREVIEW_FPS:
        if (setting_is_rawrgb_format(cpt, parm)) {
            type_val = parm->preview_fps_param.frame_rate;
        } else {
            hal_param->hal_common.frame_rate =
                parm->preview_fps_param.frame_rate;
            type_val = parm->preview_fps_param.video_mode;
            item->cmd_type_value = &hal_param->hal_common.video_mode;
            parm->cmd_type_value = type_val;
        }
        break;
    case SETTING_GENERAL_PREVIEW_LLS_FPS:
        if (setting_is_rawrgb_format(cpt, parm)) {
            type_val = parm->preview_fps_param.frame_rate;
        }
        break;

    case SETTING_GENERAL_AWB_LOCK_UNLOCK:
    case SETTING_GENERAL_AE_LOCK_UNLOCK:
        if (setting_is_rawrgb_format(cpt, parm)) {
            ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
        }
        break;
    case SETTING_GENERAL_EXPOSURE_COMPENSATION:
        if (setting_is_active(cpt)) {
            if (setting_is_rawrgb_format(cpt, parm)) {
                ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
            }
        }
        hal_param->hal_common.ae_compensation_param =
            parm->ae_compensation_param;
        break;
    case SETTING_GENERAL_EXPOSURE_TIME:
        *item->cmd_type_value = 0;
        type_val = parm->cmd_type_value;
        break;
    case SETTING_GENERAL_AUTO_3DNR:
        item->isp_cmd = COM_ISP_SET_AUTO_3DNR;
        ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
        type_val = parm->cmd_type_value;
        break;
    case SETTING_GENERAL_AI_SCENE_ENABLED:
        if (parm->cmd_type_value) {
            item->isp_cmd = COM_ISP_SET_AI_SCENE_START;
            ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
        } else {
            item->isp_cmd = COM_ISP_SET_AI_SCENE_STOP;
            ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
        }
        break;
    case SETTING_GENERAL_AUTO_TRACKING_INFO_ENABLE:
        item->isp_cmd = COM_ISP_SET_AUTO_TRACKING_ENABLE;
        ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
        break;

    default:
        type_val = parm->cmd_type_value;
        break;
    }

    if (SETTING_GENERAL_AE_LOCK_UNLOCK == type ||
        SETTING_GENERAL_AWB_LOCK_UNLOCK == type ||
        SETTING_GENERAL_EXPOSURE_COMPENSATION == type) {
        goto setting_out;
    }

    if (type == SETTING_GENERAL_AUTO_HDR) {
        CMR_LOGD("hdr type:%d, force_set%d, type_val:%d, cmd_type_value:%d",
                    type, cpt->force_set, type_val, *(item->cmd_type_value));
    }
    if (type == SETTING_GENERAL_AUTO_FDR) {
        CMR_LOGD("fdr type:%d, force_set%d, type_val:%d, cmd_type_value:%d",
                    type, cpt->force_set, type_val, *(item->cmd_type_value));
    }
    if ((type_val != *(item->cmd_type_value)) || (cpt->force_set)) {
        if (setting_is_active(cpt)) {
            ret = setting_before_set_ctrl(cpt, PARAM_NORMAL);
            if (ret) {
                CMR_LOGE("failed %ld", ret);
                goto setting_out;
            }

            if (setting_is_rawrgb_format(cpt, parm)) {
                ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
            } else {
                ret = setting_sn_ctrl(cpt, item->sn_cmd, parm);
            }
            if (ret) {
                CMR_LOGE("failed %ld", ret);
                goto setting_out;
            }

            if (type == SETTING_GENERAL_PREVIEW_FPS) {
                if (parm->preview_fps_param.frame_rate != 0) {
                    struct setting_cmd_parameter isoParm;
                    cmr_bzero(&isoParm, sizeof(struct setting_cmd_parameter));
                    isoParm.camera_id = parm->camera_id;
                    isoParm.cmd_type_value = 5;
                    if (setting_is_rawrgb_format(cpt, &isoParm)) {
                        // ret = setting_isp_ctrl(cpt, COM_ISP_SET_ISO,
                        // &isoParm); // remove ISO 1600 in video mode
                        if (ret) {
                            CMR_LOGE("iso set failed %ld", ret);
                            goto setting_out;
                        }
                    }
                }
                // always do
                // setting_sn_ctrl(cpt, item->sn_cmd, parm);
            }

            skip_mode = IMG_SKIP_SW_KER;
            is_check_night_mode = 0;
            if (SETTING_GENERAL_SENSOR_ROTATION == type) {
                is_check_night_mode = 1;
            }
            skip_number =
                setting_get_skip_number(cpt, parm, is_check_night_mode);
            after_cb_param.re_mode = PARAM_NORMAL;
            after_cb_param.skip_mode = skip_mode;
            after_cb_param.skip_number = skip_number;
            after_cb_param.timestamp = systemTime(CLOCK_MONOTONIC);
            ret = setting_after_set_ctrl(cpt, &after_cb_param);
        }
        *item->cmd_type_value = type_val;
    }

setting_out:
    return ret;
}

static cmr_int setting_set_encode_angle(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    cmr_uint encode_angle = 0;
    cmr_uint encode_rotation = 0;

    if (hal_param->is_rotation_capture) {
        uint32_t orientation;

        encode_rotation = 0;
        orientation = setting_get_exif_orientation((int)parm->cmd_type_value);
        switch (orientation) {
        case ORIENTATION_NORMAL:
            encode_angle = IMG_ANGLE_0;
            break;

        case ORIENTATION_ROTATE_180:
            encode_angle = IMG_ANGLE_180;
            break;

        case ORIENTATION_ROTATE_90:
            encode_angle = IMG_ANGLE_90;
            break;

        case ORIENTATION_ROTATE_270:
            encode_angle = IMG_ANGLE_270;
            break;

        default:
            encode_angle = IMG_ANGLE_0;
            break;
        }
    } else {
        encode_rotation = parm->cmd_type_value;
        encode_angle = IMG_ANGLE_0;
    }
    hal_param->encode_rotation = encode_rotation;
    hal_param->encode_angle = encode_angle;

    CMR_LOGD("encode_angle %ld, encode_rotation=%d", hal_param->encode_angle,hal_param->encode_rotation);
    return ret;
}

static cmr_int setting_get_encode_angle(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->encode_angle;
    return ret;
}

static cmr_int setting_get_encode_rotation(struct setting_component *cpt,
                                           struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->encode_rotation;
    CMR_LOGV("encode_rotation=%d", hal_param->encode_rotation);
    return ret;
}

static cmr_uint setting_flash_mode_to_status(struct setting_component *cpt,
                                             struct setting_cmd_parameter *parm,
                                             enum cmr_flash_mode f_mode) {
    cmr_int ret = 0;
    cmr_uint autoflash = 0;
    cmr_uint status = FLASH_STATUS_MAX;
    struct setting_cmd_parameter ctrl_param;
    struct setting_flash_param *flash_param =
        get_flash_param(cpt, parm->camera_id);

    switch (f_mode) {
    case CAMERA_FLASH_MODE_OFF:
        status = FLASH_CLOSE;
        break;

    case CAMERA_FLASH_MODE_ON:
        status = FLASH_OPEN;
        break;

    case CAMERA_FLASH_MODE_TORCH:
#ifdef DV_FLASH_ON_DV_WITH_PREVIEW
        status = FLASH_TORCH;
#else
        status = FLASH_OPEN_ON_RECORDING;
#endif
        break;

    case CAMERA_FLASH_MODE_AUTO:
        ctrl_param.camera_id = parm->camera_id;
        if (setting_is_rawrgb_format(cpt, parm)) {
            ret = setting_isp_ctrl(cpt, COM_ISP_GET_LOW_LUX_EB, &ctrl_param);
        } else {
            ret =
                setting_sn_ctrl(cpt, COM_SN_GET_AUTO_FLASH_STATE, &ctrl_param);
        }
        CMR_LOGD("auto flash status %ld", ctrl_param.cmd_type_value);

        if (ret) {
            ctrl_param.cmd_type_value = 1;
            CMR_LOGW("Failed to read auto flash mode %ld", ret);
        }
        if (ctrl_param.cmd_type_value) {
            status = FLASH_OPEN;
        } else {
            status = FLASH_CLOSE;
        }
        break;

    default:
        break;
    }

    return status;
}

static cmr_int setting_flash_handle(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm,
                                    cmr_uint flash_mode) {
    cmr_uint status = FLASH_STATUS_MAX;
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_flash_param *flash_param = &hal_param->flash_param;

    status = setting_flash_mode_to_status(cpt, parm, flash_mode);
    CMR_LOGD("status = %ld,flash_param->flash_status = %ld", status,
             flash_param->flash_status);
    if (status != flash_param->flash_status) {
        if (FLASH_CLOSE == status || FLASH_TORCH == status)
            ret = setting_set_flashdevice(cpt, parm, status);
        flash_param->flash_status = status;
    }

    return ret;
}

static cmr_int setting_get_HW_flash_status(struct setting_component *cpt,
                                           struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_flash_param *flash_param = &hal_param->flash_param;

    parm->cmd_type_value = flash_param->flash_hw_status;

    return ret;
}

static cmr_int setting_is_need_flash(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm);
static cmr_int setting_get_flash_status(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    parm->cmd_type_value = setting_is_need_flash(cpt, parm);
    CMR_LOGD("flash_status %ld", parm->cmd_type_value);
    return ret;
}

static cmr_int setting_set_flash_mode(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_flash_param *flash_param =
        get_flash_param(cpt, parm->camera_id);
    cmr_uint flash_mode = 0;
    cmr_uint status = 0;

    flash_mode = parm->cmd_type_value;

    CMR_LOGD("flash_mode:%lu has_preflashed:%lu", flash_mode,
             flash_param->has_preflashed);

    flash_param->flash_mode = flash_mode;
    setting_flash_handle(cpt, parm, flash_param->flash_mode);

    if (setting_is_rawrgb_format(cpt, parm)) {
        struct setting_init_in *init_in = &cpt->init_in;
        struct common_isp_cmd_param isp_param;

        if (init_in->setting_isp_ioctl) {
            isp_param.camera_id = parm->camera_id;
            isp_param.cmd_value = parm->cmd_type_value;
            ret = (*init_in->setting_isp_ioctl)(
                init_in->oem_handle, COM_ISP_SET_FLASH_MODE, &isp_param);
            // whether FRONT_CAMERA_FLASH_TYPE is torch
            bool isFrontTorch =
                (strcmp(FRONT_CAMERA_FLASH_TYPE, "led") == 0) ? true : false;
            if (parm->camera_id == 1 && isFrontTorch) {
                isp_param.camera_id = parm->camera_id;
                isp_param.cmd_value = ISP_ONLINE_FLASH_CALLBACK;
                switch (flash_mode) {
                case CAMERA_FLASH_MODE_TORCH:
                    isp_param.flash_notice.mode = ISP_FLASH_SLAVE_FLASH_TORCH;
                    break;
                case CAMERA_FLASH_MODE_AUTO:
                    isp_param.flash_notice.mode = ISP_FLASH_SLAVE_FLASH_AUTO;
                    break;
                case CAMERA_FLASH_MODE_OFF:
                    isp_param.flash_notice.mode = ISP_FLASH_SLAVE_FLASH_OFF;
                    break;
                }
                ret = (*init_in->setting_isp_ioctl)(
                    init_in->oem_handle, COM_ISP_SET_FLASH_NOTICE, &isp_param);
            }
        }
    }

    return ret;
}

static cmr_int setting_set_isp_flash_mode(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_flash_param *flash_param =
        get_flash_param(cpt, parm->camera_id);
    cmr_uint flash_mode = 0;
    cmr_uint status = 0;
    flash_mode = parm->cmd_type_value;

    ret = setting_set_flashdevice(cpt, parm, flash_mode);

    return ret;
}

static cmr_int
setting_set_auto_exposure_mode(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    CMR_LOGD("parm->ae_param.mode:%ld", parm->ae_param.mode);
    ret = setting_set_general(cpt, SETTING_GENERAL_AUTO_EXPOSURE_MODE, parm);

    return ret;
}

static cmr_int setting_set_ae_region(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct common_isp_cmd_param isp_param;

    // some isp only support touch ae on spot metering
    // if (CAMERA_AE_SPOT_METERING == parm->ae_param.mode)
    {
        if (setting_is_rawrgb_format(cpt, parm)) {
            isp_param.win_area = parm->ae_param.win_area;
            if (init_in->setting_isp_ioctl) {
                isp_param.camera_id = parm->camera_id;
                ret = (*init_in->setting_isp_ioctl)(
                    init_in->oem_handle, COM_ISP_SET_AE_METERING_AREA,
                    &isp_param);
            }
        }
    }

    return ret;
}

static cmr_int setting_set_exposure_time(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    CMR_LOGI("exposure time = %" PRIu64 "", parm->cmd_type_value);

    ret = setting_set_general(cpt, SETTING_GENERAL_EXPOSURE_TIME, parm);

    return ret;
}

static cmr_int setting_set_brightness(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_BRIGHTNESS, parm);

    return ret;
}

static cmr_int setting_set_ai_scence(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_AI_SCENE_ENABLED, parm);

    return ret;
}

static cmr_int setting_set_contrast(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_CONTRAST, parm);

    return ret;
}

static cmr_int setting_set_saturation(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_SATURATION, parm);

    return ret;
}

static cmr_int setting_set_sharpness(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_SHARPNESS, parm);

    return ret;
}

static cmr_int setting_set_effect(struct setting_component *cpt,
                                  struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_EFFECT, parm);

    return ret;
}

static cmr_int
setting_set_exposure_compensation(struct setting_component *cpt,
                                  struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_EXPOSURE_COMPENSATION, parm);

    return ret;
}

static cmr_int setting_set_wb(struct setting_component *cpt,
                              struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_WB, parm);

    return ret;
}

static cmr_int setting_set_scene_mode(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("set scene mode %ld", parm->cmd_type_value);
    if (CAMERA_SCENE_MODE_HDR == parm->cmd_type_value) {
        hal_param->is_hdr = 1;
    } else {
        hal_param->is_hdr = 0;
    }
    if (CAMERA_SCENE_MODE_FDR == parm->cmd_type_value) {
        hal_param->is_fdr = 1;
    } else {
        hal_param->is_fdr = 0;
    }
    ret = setting_set_general(cpt, SETTING_GENERAL_SCENE_MODE, parm);

    return ret;
}

static cmr_int setting_set_antibanding(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_ANTIBANDING, parm);

    return ret;
}

static cmr_int setting_set_iso(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_ISO, parm);

    return ret;
}

static cmr_int setting_set_preview_fps(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);

    if (0 != parm->preview_fps_param.frame_rate) {
        local_param->is_dv_mode = 1;
    } else {
        local_param->is_dv_mode = 0;
    }

    ret = setting_set_general(cpt, SETTING_GENERAL_PREVIEW_FPS, parm);

    return ret;
}

static cmr_int setting_set_range_fps(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);

    if (parm->range_fps.video_mode) {
        local_param->is_dv_mode = 1;
    } else {
        local_param->is_dv_mode = 0;
    }

    if (setting_is_rawrgb_format(cpt, parm)) {
        hal_param->is_update_range_fps = 1;
        hal_param->range_fps = parm->range_fps;
        /*special mode have fix fps, not update fps to isp*/
        if (hal_param->video_slow_motion_flag == 0 &&
            hal_param->hal_common.scene_mode != CAMERA_SCENE_MODE_ACTION) {
            ret = setting_isp_ctrl(cpt, COM_ISP_SET_RANGE_FPS, parm);
        }
    } else {
        hal_param->is_update_range_fps = 1;
        hal_param->range_fps = parm->range_fps;
    }

    CMR_LOGD("min_fps=%ld, max_fps=%ld", hal_param->range_fps.min_fps,
             hal_param->range_fps.max_fps);

    return ret;
}

static cmr_int setting_set_preview_lls_fps(struct setting_component *cpt,
                                           struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_PREVIEW_LLS_FPS, parm);

    return ret;
}

static cmr_int setting_set_shot_num(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->shot_num = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_shot_num(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->shot_num;
    return ret;
}

static cmr_int setting_set_focal_length(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->hal_common.focal_length = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_set_exif_mime_type(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->exif_mime_type = parm->cmd_type_value;
    CMR_LOGD("set exif mime type is %lu", parm->cmd_type_value);
    return ret;
}

static cmr_int setting_process_zoom(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    cmr_uint type_val = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    enum img_skip_mode skip_mode = 0;
    cmr_uint skip_number = 0;

    struct after_set_cb_param after_cb_param;
    struct cmr_zoom_param zoom_param;
    struct cmr_zoom_param org_zoom;
    cmr_uint is_changed = 0;

    pthread_mutex_lock(&cpt->status_lock);
    org_zoom = hal_param->zoom_value;
    pthread_mutex_unlock(&cpt->status_lock);

    zoom_param = parm->zoom_param;
    if (zoom_param.mode == ZOOM_LEVEL) {
        if (zoom_param.zoom_level != org_zoom.zoom_level)
            is_changed = 1;
    } else if (zoom_param.mode == ZOOM_INFO) {
        const float EPSINON = 0.01f;
        if (fabs(zoom_param.zoom_info.prev_aspect_ratio - org_zoom.zoom_info.prev_aspect_ratio) >= EPSINON
            || fabs(zoom_param.zoom_info.capture_aspect_ratio - org_zoom.zoom_info.capture_aspect_ratio) >= EPSINON
            || fabs(zoom_param.zoom_info.video_aspect_ratio - org_zoom.zoom_info.video_aspect_ratio) >= EPSINON)
            is_changed = 1;
    }

    if (is_changed) {
        if (setting_is_active(cpt)) {
            ret = setting_before_set_ctrl(cpt, PARAM_ZOOM);
            if (ret) {
                CMR_LOGE("failed %ld", ret);
                goto setting_out;
            }

            skip_mode = IMG_SKIP_SW_KER;
            skip_number = setting_get_skip_number(cpt, parm, 1);
            after_cb_param.re_mode = PARAM_ZOOM;
            after_cb_param.skip_mode = skip_mode;
            after_cb_param.skip_number = skip_number;
            after_cb_param.timestamp = systemTime(CLOCK_MONOTONIC);
            ret = setting_after_set_ctrl(cpt, &after_cb_param);
            if (ret) {
                CMR_LOGE("after set failed %ld", ret);
                goto setting_out;
            }
        }
        /*update zoom unit after processed or not*/
        pthread_mutex_lock(&cpt->status_lock);
        hal_param->zoom_value = zoom_param;
        pthread_mutex_unlock(&cpt->status_lock);
    }

setting_out:
    return ret;
}

static cmr_int setting_zoom_push(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    CMR_MSG_INIT(message);
    cmr_int ret = 0;

    pthread_mutex_lock(&cpt->ctrl_lock);
    cpt->zoom_unit.is_changed = 1;
    cpt->zoom_unit.in_zoom = *parm;
    if (ZOOM_IDLE == cpt->zoom_unit.status && !cpt->zoom_unit.is_sended_msg) {
        message.msg_type = SETTING_EVT_ZOOM;
        message.sync_flag = CMR_MSG_SYNC_NONE;
        ret = cmr_thread_msg_send(cpt->thread_handle, &message);
        cpt->zoom_unit.is_sended_msg = 1;
    }
    pthread_mutex_unlock(&cpt->ctrl_lock);

    return ret;
}

static cmr_int setting_set_zoom_param(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    pthread_mutex_lock(&cpt->status_lock);
    ret = setting_zoom_push(cpt, parm);
    pthread_mutex_unlock(&cpt->status_lock);

    return ret;
}

static cmr_int setting_get_zoom_param(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    pthread_mutex_lock(&cpt->status_lock);
    parm->zoom_param = hal_param->zoom_value;
    pthread_mutex_unlock(&cpt->status_lock);
    return ret;
}

static cmr_int
setting_set_reprocess_zoom_ratio(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    pthread_mutex_lock(&cpt->status_lock);
    hal_param->zoom_reprocess= parm->zoom_param;
    pthread_mutex_unlock(&cpt->status_lock);
    return ret;
}

static cmr_int
setting_get_reprocess_zoom_ratio(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    pthread_mutex_lock(&cpt->status_lock);
    parm->zoom_param = hal_param->zoom_reprocess;
    pthread_mutex_unlock(&cpt->status_lock);

    return ret;
}

static cmr_int
setting_get_sensor_orientation(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sensor_orientation;
    CMR_LOGV("get sensor_orientation %d",parm->cmd_type_value);
    return ret;
}

static cmr_int
setting_set_sensor_orientation(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sensor_orientation = parm->cmd_type_value;
    CMR_LOGV("set sensor_orientation %d",hal_param->sensor_orientation);
    return ret;
}

static void get_seconds_from_double(double d, uint32_t *numerator,
                                    uint32_t *denominator) {
    d = fabs(d);
    double degrees = (int)d;
    double remainder = d - degrees;
    double minutes = (int)(remainder * 60.0);
    double seconds = (((remainder * 60.0) - minutes) * 60.0);
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t num = 0;
    char str[20];
    double value = 1.0;

    sprintf(str, "%f", seconds);
    while (str[i++] != '.')
        ;
    j = strlen(str) - 1;
    while (str[j] == '0')
        --j;
    num = j - i + 1;
    CMR_LOGD("%s, i=%d, j=%d, num=%d \n", str, i, j, num);

    for (i = 0; i < num; i++)
        value *= 10.0;

    *numerator = seconds * value;
    *denominator = value;

    CMR_LOGD("data=%f, num=%d, denom=%d \n", seconds, *numerator, *denominator);
}

static uint32_t get_data_from_double(double d,
                                     uint32_t type) /*0: dd, 1: mm, 2: ss*/
{
    d = fabs(d);
    double degrees = (int)d;
    double remainder = d - degrees;
    double minutes = (int)(remainder * 60.0);
    double seconds = (int)(((remainder * 60.0) - minutes) * 60.0);
    uint32_t retVal = 0;
    if (0 == type) {
        retVal = (int)degrees;
    } else if (1 == type) {
        retVal = (int)minutes;
    } else if (2 == type) {
        retVal = (int)seconds;
    }
    CMR_LOGV("GPS: type: %d, ret: 0x%x", type, retVal);
    return retVal;
}

static void *setting_get_pic_taking(void *priv_data) {
    struct setting_exif_cb_param *cb_param =
        (struct setting_exif_cb_param *)priv_data;
    struct setting_local_param *local_param =
        get_local_param(cb_param->cpt, cb_param->parm->camera_id);
    struct setting_init_in *init_in = &cb_param->cpt->init_in;
    cmr_int ret = 0;

    // read exif info from sensor only
    if (init_in->setting_sn_ioctl) {
        struct common_sn_cmd_param sn_param;

        sn_param.camera_id = cb_param->parm->camera_id;
        ret = (*init_in->setting_sn_ioctl)(
            init_in->oem_handle, COM_SN_GET_EXIF_IMAGE_INFO, &sn_param);
        if (ret) {
            CMR_LOGW("sn ctrl failed");
        }
        cmr_copy(&local_param->exif_pic_taking, &sn_param.exif_pic_info,
                 sizeof(local_param->exif_pic_taking));
    }

    return (void *)&local_param->exif_pic_taking;
}

static cmr_int setting_update_gps_info(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    struct camera_position_type position = hal_param->position_info;
    JINF_EXIF_INFO_T *p_exif_info = &local_param->exif_all_info;

    uint32_t latitude_dd_numerator = get_data_from_double(position.latitude, 0);
    uint32_t latitude_dd_denominator = 1;
    uint32_t latitude_mm_numerator = get_data_from_double(position.latitude, 1);
    uint32_t latitude_mm_denominator = 1;
    uint32_t latitude_ss_numerator = 0;
    uint32_t latitude_ss_denominator = 0;
    uint32_t latitude_ref = 0;
    uint32_t longitude_ref = 0;
    uint32_t longitude_dd_numerator = 0;
    uint32_t longitude_dd_denominator = 0;
    uint32_t longitude_mm_numerator = 0;
    uint32_t longitude_mm_denominator = 0;
    uint32_t longitude_ss_numerator = 0;
    uint32_t longitude_ss_denominator = 0;
    char gps_date_buf[11] = {0};
    time_t timep;
    struct tm *p;
    char *gps_date;
    const char *gps_process_method;
    uint32_t gps_hour;
    uint32_t gps_minuter;
    uint32_t gps_second;

    get_seconds_from_double(position.latitude, &latitude_ss_numerator,
                            &latitude_ss_denominator);
    if (position.latitude < 0.0) {
        latitude_ref = 1;
    } else {
        latitude_ref = 0;
    }
    longitude_dd_numerator = get_data_from_double(position.longitude, 0);
    longitude_dd_denominator = 1;
    longitude_mm_numerator = get_data_from_double(position.longitude, 1);
    longitude_mm_denominator = 1;
    get_seconds_from_double(position.longitude, &longitude_ss_numerator,
                            &longitude_ss_denominator);
    if (position.longitude < 0.0) {
        longitude_ref = 1;
    } else {
        longitude_ref = 0;
    }

    if (0 == position.timestamp)
        time(&position.timestamp);
    p = gmtime(&position.timestamp);
    sprintf(gps_date_buf, "%4d:%02d:%02d", (1900 + p->tm_year), (1 + p->tm_mon),
            p->tm_mday);
    gps_date_buf[10] = '\0';
    gps_date = gps_date_buf;
    gps_hour = p->tm_hour;
    gps_minuter = p->tm_min;
    gps_second = p->tm_sec;
    CMR_LOGD("gps_data 2 = %s, %d:%d:%d \n", gps_date, gps_hour, gps_minuter,
             gps_second);

    gps_process_method = position.process_method;

    if (NULL != p_exif_info->gps_ptr) {
        if ((0 == latitude_dd_numerator) && (0 == latitude_mm_numerator) &&
            (0 == latitude_ss_numerator) && (0 == longitude_dd_numerator) &&
            (0 == longitude_mm_numerator) && (0 == longitude_ss_numerator)) {
            /* if no Latitude and Longitude, do not write GPS to EXIF */
            CMR_LOGD("GPS: Latitude and Longitude is 0, do not write to EXIF: "
                     "valid=%d \n",
                     *(uint32_t *)&p_exif_info->gps_ptr->valid);
            memset(&p_exif_info->gps_ptr->valid, 0, sizeof(EXIF_GPS_VALID_T));
        } else {
            p_exif_info->gps_ptr->valid.GPSLatitudeRef = 1;
            p_exif_info->gps_ptr->GPSLatitudeRef[0] =
                (0 == latitude_ref) ? 'N' : 'S';
            p_exif_info->gps_ptr->valid.GPSLongitudeRef = 1;
            p_exif_info->gps_ptr->GPSLongitudeRef[0] =
                (0 == longitude_ref) ? 'E' : 'W';

            p_exif_info->gps_ptr->valid.GPSLatitude = 1;
            p_exif_info->gps_ptr->GPSLatitude[0].numerator =
                latitude_dd_numerator;
            p_exif_info->gps_ptr->GPSLatitude[0].denominator =
                latitude_dd_denominator;
            p_exif_info->gps_ptr->GPSLatitude[1].numerator =
                latitude_mm_numerator;
            p_exif_info->gps_ptr->GPSLatitude[1].denominator =
                latitude_mm_denominator;
            p_exif_info->gps_ptr->GPSLatitude[2].numerator =
                latitude_ss_numerator;
            p_exif_info->gps_ptr->GPSLatitude[2].denominator =
                latitude_ss_denominator;

            p_exif_info->gps_ptr->valid.GPSLongitude = 1;
            p_exif_info->gps_ptr->GPSLongitude[0].numerator =
                longitude_dd_numerator;
            p_exif_info->gps_ptr->GPSLongitude[0].denominator =
                longitude_dd_denominator;
            p_exif_info->gps_ptr->GPSLongitude[1].numerator =
                longitude_mm_numerator;
            p_exif_info->gps_ptr->GPSLongitude[1].denominator =
                longitude_mm_denominator;
            p_exif_info->gps_ptr->GPSLongitude[2].numerator =
                longitude_ss_numerator;
            p_exif_info->gps_ptr->GPSLongitude[2].denominator =
                longitude_ss_denominator;

            p_exif_info->gps_ptr->valid.GPSAltitude = 1;
            p_exif_info->gps_ptr->GPSAltitude.numerator = position.altitude;
            CMR_LOGD("gps_ptr->GPSAltitude.numerator: %d.",
                     p_exif_info->gps_ptr->GPSAltitude.numerator);
            p_exif_info->gps_ptr->GPSAltitude.denominator = 1;
            p_exif_info->gps_ptr->valid.GPSAltitudeRef = 1;

            if (NULL != gps_process_method) {
                const char ascii[] = {0x41, 0x53, 0x43, 0x49, 0x49, 0, 0, 0};
                p_exif_info->gps_ptr->valid.GPSProcessingMethod = 1;
                p_exif_info->gps_ptr->GPSProcessingMethod.count =
                    strlen(gps_process_method) + sizeof(ascii) + 1;
                memcpy((char *)p_exif_info->gps_ptr->GPSProcessingMethod.ptr,
                       ascii, sizeof(ascii));
                strcpy((char *)p_exif_info->gps_ptr->GPSProcessingMethod.ptr +
                           sizeof(ascii),
                       (char *)gps_process_method);
                /*add "ASCII\0\0\0" for cts test by lyh*/
            }

            p_exif_info->gps_ptr->valid.GPSTimeStamp = 1;
            p_exif_info->gps_ptr->GPSTimeStamp[0].numerator = gps_hour;
            p_exif_info->gps_ptr->GPSTimeStamp[1].numerator = gps_minuter;
            p_exif_info->gps_ptr->GPSTimeStamp[2].numerator = gps_second;

            p_exif_info->gps_ptr->GPSTimeStamp[0].denominator = 1;
            p_exif_info->gps_ptr->GPSTimeStamp[1].denominator = 1;
            p_exif_info->gps_ptr->GPSTimeStamp[2].denominator = 1;
            p_exif_info->gps_ptr->valid.GPSDateStamp = 1;
            strcpy((char *)p_exif_info->gps_ptr->GPSDateStamp,
                   (char *)gps_date);
            CMR_LOGD("GPS: valid=%d \n",
                     *(uint32_t *)&p_exif_info->gps_ptr->valid);
        }
    }
    cmr_bzero(&hal_param->position_info, sizeof(struct camera_position_type));
    return ret;
}

static cmr_int setting_get_exif_info(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    struct setting_exif_unit *exif_unit = &local_param->exif_unit;
    struct setting_io_parameter cmd_param;
    JINF_EXIF_INFO_T *p_exif_info = &local_param->exif_all_info;
    struct setting_exif_cb_param cb_param;
    struct setting_flash_param *flash_param = &hal_param->flash_param;

    char datetime_buf[20] = {0};
    char zone_buf[10] = {0};
    time_t timep;
    struct tm *p;
    char *datetime;
    int second1 ;
    int second2;
    int hour;
    int minute;
    uint32_t focal_length_numerator;
    uint32_t focal_length_denominator;
    uint32_t rotation_angle;
    uint32_t exif_mime_type;
    char property[PROPERTY_VALUE_MAX];
    cmr_u32 is_raw_capture = 0;
    char value[PROPERTY_VALUE_MAX];

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

#ifdef CONFIG_SUPPORT_GDEPTH
#define EXIF_DEF_MAKER ""
#define EXIF_DEF_MODEL ""
    static const char image_desc[] = "";
    static const char copyright[] = "";
#else
#define EXIF_DEF_MAKER "Spreadtrum"
#define EXIF_DEF_MODEL "spxxxx"
    static const char image_desc[] = "Exif_JPEG_420";
    static const char copyright[] = "Copyright,Spreadtrum,2011";
#endif

    if (FLASH_OPEN == flash_param->flash_hw_status) {
        setting_set_flashdevice(cpt, parm, (uint32_t)FLASH_CLOSE_AFTER_OPEN);
    }
    cb_param.cpt = cpt;
    cb_param.parm = parm;
    ret = cmr_exif_init(p_exif_info, setting_get_pic_taking, (void *)&cb_param);
    if (ret) {
        CMR_LOGE("exif init failed");
        return ret;
    }

    if (init_in->io_cmd_ioctl) {
        cmd_param.camera_id = parm->camera_id;
        (*init_in->io_cmd_ioctl)(init_in->oem_handle,
                                 SETTING_IO_GET_CAPTURE_SIZE, &cmd_param);
        exif_unit->picture_size = cmd_param.size_param;

        (*init_in->io_cmd_ioctl)(init_in->oem_handle,
                                 SETTING_IO_GET_ACTUAL_CAPTURE_SIZE,
                                 &cmd_param);
        exif_unit->actual_picture_size = cmd_param.size_param;
    }

        // workaround jpeg cant handle 16-noalign issue, when jpeg fix this
        // issue, for sharkle only
#if defined(CONFIG_ISP_2_3)
    if ((is_raw_capture == 0) && (hal_param->originalPictureSize.height % 16 != 0)) {
          if (exif_unit->picture_size.width == hal_param->originalPictureSize.width &&
               exif_unit->picture_size.height != hal_param->originalPictureSize.height) {
               exif_unit->picture_size.height = hal_param->originalPictureSize.height;
          }
          if (exif_unit->actual_picture_size.width == hal_param->originalPictureSize.width &&
               exif_unit->actual_picture_size.height != hal_param->originalPictureSize.height) {
               exif_unit->actual_picture_size.height  = hal_param->originalPictureSize.height;
          }
    }
#endif

    time(&timep);
    p = localtime(&timep);
    sprintf(datetime_buf, "%4d:%02d:%02d %02d:%02d:%02d", (1900 + p->tm_year),
            (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    datetime_buf[19] = '\0';
    datetime = datetime_buf;

    CMR_LOGD("datetime %s", datetime);

    /*calculate zone info begin*/
    second1 = timep % (24 * 60 * 60);
    second2 = p->tm_hour * 3600 + p->tm_min * 60 + p->tm_sec;
    hour = (second2 - second1)/3600;
    minute =((second2 - second1)%3600)/60;

    if (hour < -12) {
        hour += 24;
    } else if (hour > 12) {
        hour -= 24;
    }

    if( hour >= 0 )
        sprintf(zone_buf, "+%02d:%02d", hour,minute);
    else
        sprintf(zone_buf, "%03d:%02d", hour,minute);

    zone_buf[6] = '\0';

    if (NULL != p_exif_info->spec_ptr) {
        strcpy(
            (char *)p_exif_info->spec_ptr->date_time_ptr->OffsetTimeOriginal,
            (char *)zone_buf);
        strcpy(
            (char *)p_exif_info->spec_ptr->date_time_ptr->OffsetTimeDigitized,
            (char *)zone_buf);
        strcpy(
            (char *)p_exif_info->spec_ptr->date_time_ptr->OffsetTime,
            (char *)zone_buf);
        CMR_LOGD("zone_buf=%s",zone_buf);
        /*calculate zone info end*/
    }

    /*update gps info*/
    setting_update_gps_info(cpt, parm);

    focal_length_numerator = hal_param->hal_common.focal_length;
    focal_length_denominator = 1000;
    exif_mime_type = hal_param->exif_mime_type;
    /* Some info is not get from the kernel */
    if (NULL != p_exif_info->spec_ptr) {
        p_exif_info->spec_ptr->basic.PixelXDimension =
            exif_unit->picture_size.width;
        p_exif_info->spec_ptr->basic.PixelYDimension =
            exif_unit->picture_size.height;
        if (IMG_ANGLE_90 == hal_param->encode_angle ||
            IMG_ANGLE_270 == hal_param->encode_angle) {
            p_exif_info->spec_ptr->basic.PixelXDimension =
                exif_unit->picture_size.height;
            p_exif_info->spec_ptr->basic.PixelYDimension =
                exif_unit->picture_size.width;
        }
        p_exif_info->spec_ptr->basic.MimeType = exif_mime_type;
    }
    rotation_angle = setting_get_exif_orientation(hal_param->encode_rotation);
    if (rotation_angle == ORIENTATION_ROTATE_90 ||
        rotation_angle == ORIENTATION_ROTATE_270) {
        p_exif_info->primary.basic.ImageWidth =
            exif_unit->actual_picture_size.height;
        p_exif_info->primary.basic.ImageLength =
            exif_unit->actual_picture_size.width;
    } else {
        p_exif_info->primary.basic.ImageWidth =
            exif_unit->actual_picture_size.width;
        p_exif_info->primary.basic.ImageLength =
            exif_unit->actual_picture_size.height;
    }

    if (NULL != p_exif_info->primary.data_struct_ptr) {
#ifdef MIRROR_FLIP_ROTATION_BY_JPEG
        /* raw capture not support mirror/flip/rotation*/
        if (is_raw_capture == 0) {
            p_exif_info->primary.data_struct_ptr->valid.Orientation = 1;
            p_exif_info->primary.data_struct_ptr->Orientation =
                ORIENTATION_UNDEFINED;
        } else {
            p_exif_info->primary.data_struct_ptr->valid.Orientation = 1;
            p_exif_info->primary.data_struct_ptr->Orientation =
                setting_get_exif_orientation(hal_param->encode_rotation);
        }
#else
        p_exif_info->primary.data_struct_ptr->valid.Orientation = 1;
        p_exif_info->primary.data_struct_ptr->Orientation =
            setting_get_exif_orientation(hal_param->encode_rotation);
#endif

#ifdef MIRROR_FLIP_BY_ISP
        /* check why put these code here later */
        if (hal_param->sprd_zsl_enabled && 1 == parm->camera_id &&
            1 == hal_param->flip_on) {
            if (270 == hal_param->encode_rotation) {
                p_exif_info->primary.data_struct_ptr->Orientation =
                    ORIENTATION_ROTATE_90;
            } else if (90 == hal_param->encode_rotation) {
                p_exif_info->primary.data_struct_ptr->Orientation =
                    ORIENTATION_ROTATE_270;
            }
        }
#endif
    }

    if (NULL != p_exif_info->primary.img_desc_ptr) {
        strcpy((char *)p_exif_info->primary.img_desc_ptr->ImageDescription,
               (char *)image_desc);
        memset(property, '\0', sizeof(property));
        property_get("ro.product.manufacturer", property, EXIF_DEF_MAKER);
        strcpy((char *)p_exif_info->primary.img_desc_ptr->Make,
               (char *)property);

        memset(property, '\0', sizeof(property));
        property_get("ro.product.model", property, EXIF_DEF_MODEL);
        strcpy((char *)p_exif_info->primary.img_desc_ptr->Model,
               (char *)property);
        strcpy((char *)p_exif_info->primary.img_desc_ptr->Copyright,
               (char *)copyright);
    }

    if ((NULL != p_exif_info->spec_ptr) &&
        (NULL != p_exif_info->spec_ptr->pic_taking_cond_ptr)) {
        p_exif_info->spec_ptr->pic_taking_cond_ptr->valid.FocalLength = 1;
        p_exif_info->spec_ptr->pic_taking_cond_ptr->FocalLength.numerator =
            focal_length_numerator;
        p_exif_info->spec_ptr->pic_taking_cond_ptr->FocalLength.denominator =
            focal_length_denominator;
    }

    /* TODO: data time is get from user space now */
    if (NULL != p_exif_info->primary.img_desc_ptr) {
        CMR_LOGV("set DateTime.");
        strcpy((char *)p_exif_info->primary.img_desc_ptr->DateTime,
               (char *)datetime);
    }

    if (NULL != p_exif_info->spec_ptr) {
        if (NULL != p_exif_info->spec_ptr->other_ptr) {
            CMR_LOGV("set ImageUniqueID.");
            memset(p_exif_info->spec_ptr->other_ptr->ImageUniqueID, 0,
                   sizeof(p_exif_info->spec_ptr->other_ptr->ImageUniqueID));
            sprintf((char *)p_exif_info->spec_ptr->other_ptr->ImageUniqueID,
                    "IMAGE %s", datetime);
        }

        if (NULL != p_exif_info->spec_ptr->date_time_ptr) {
            strcpy(
                (char *)p_exif_info->spec_ptr->date_time_ptr->DateTimeOriginal,
                (char *)datetime);
            strcpy(
                (char *)p_exif_info->spec_ptr->date_time_ptr->DateTimeDigitized,
                (char *)datetime);
            CMR_LOGV("set DateTimeOriginal.");
        }
    }

    CMR_LOGD("EXIF width=%d, height=%d, SPECEXIF width:%d, height:%d \n",
             p_exif_info->primary.basic.ImageWidth,
             p_exif_info->primary.basic.ImageLength,
             p_exif_info->spec_ptr->basic.PixelXDimension,
             p_exif_info->spec_ptr->basic.PixelYDimension);

    parm->exif_all_info_ptr = p_exif_info;

    return ret;
}

static cmr_int setting_set_video_mode(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    struct setting_cmd_parameter setting;

    if (0 != parm->preview_fps_param.frame_rate) {
        local_param->is_dv_mode = 1;
    } else {
        local_param->is_dv_mode = 0;
    }

    ret = setting_set_general(cpt, SETTING_GENERAL_PREVIEW_FPS, parm);

    return ret;
}

static cmr_int setting_get_preview_mode(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct setting_io_parameter io_param;
    cmr_bzero(&io_param, sizeof(struct setting_io_parameter));
    if (init_in->io_cmd_ioctl) {
        ret = (*init_in->io_cmd_ioctl)(init_in->oem_handle,
                                       SETTING_IO_GET_PREVIEW_MODE, &io_param);
        parm->preview_fps_param.video_mode = io_param.cmd_value;
    }

    return ret;
}

static cmr_int setting_get_video_mode(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    cmr_u32 i;
    cmr_u32 sensor_mode;
    cmr_uint frame_rate;
    struct sensor_ae_info *sensor_ae_info;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);

    setting_get_preview_mode(cpt, parm);
    frame_rate = hal_param->hal_common.frame_rate;
    sensor_mode = parm->preview_fps_param.video_mode;
    sensor_ae_info =
        &local_param->sensor_static_info.video_info[sensor_mode].ae_info[0];

    parm->preview_fps_param.video_mode = 0;
    for (i = 0; i < SENSOR_VIDEO_MODE_MAX; i++) {
        if (frame_rate <= sensor_ae_info[i].max_frate) {
            parm->preview_fps_param.video_mode = i;
            break;
        }
    }
    if (SENSOR_VIDEO_MODE_MAX == i) {
        CMR_LOGD("use default video mode");
    }

    return ret;
}

static cmr_int setting_set_jpeg_quality(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->jpeg_quality = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_jpeg_quality(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->jpeg_quality;
    return ret;
}

static cmr_int setting_set_thumb_quality(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->thumb_quality = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_thumb_quality(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->thumb_quality;
    return ret;
}

static cmr_int setting_set_position(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->position_info = parm->position_info;
    return ret;
}

static cmr_int
setting_get_raw_capture_size(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->raw_capture_size;
    return ret;
}

static cmr_int
setting_set_raw_capture_size(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->raw_capture_size = parm->size_param;

    return ret;
}

static cmr_int setting_get_preview_size(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->preview_size;
    return ret;
}

static cmr_int setting_set_preview_size(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->preview_size = parm->size_param;
    return ret;
}

static cmr_int setting_get_preview_format(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->preview_format;
    return ret;
}

static cmr_int
setting_get_sprd_zsl_enabled(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_zsl_enabled;
    return ret;
}

static cmr_int
setting_get_face_attributes_enable(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->face_attributes_enabled;
    CMR_LOGD("face_attributes_enabled=%ld", hal_param->face_attributes_enabled);
    return ret;
}

static cmr_int
setting_get_smile_capture(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->smile_capture_enabled;
    CMR_LOGD("get smile_capture_enabled=%ld", hal_param->smile_capture_enabled);
    return ret;
}

static cmr_int
setting_get_last_preflash_time(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->last_preflash_time = hal_param->flash_param.last_preflash_time;

    return ret;
}

static cmr_int
setting_set_smile_capture(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->smile_capture_enabled = parm->cmd_type_value;
    CMR_LOGD("set smile_capture_enabled=%ld", hal_param->smile_capture_enabled);
    return ret;
}

static cmr_int
setting_get_sprd_pipviv_enabled(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_pipviv_enabled;
    return ret;
}

static cmr_int
setting_get_sprd_eis_enabled(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_eis_enabled;
    return ret;
}

static cmr_int
setting_get_slow_motion_flag(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->video_slow_motion_flag;
    return ret;
}

static cmr_int setting_set_ae_lock_unlock(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_AE_LOCK_UNLOCK, parm);

    return ret;
}
static cmr_int setting_set_awb_lock_unlock(struct setting_component *cpt,
                                           struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_AWB_LOCK_UNLOCK, parm);

    return ret;
}
static cmr_int setting_get_refocus_enable(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->refoucs_enable;
    CMR_LOGD("format=%ld", hal_param->preview_format);
    return ret;
}
static cmr_int setting_get_capture_format(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->capture_format;
    return ret;
}
static cmr_int
setting_get_video_snapshot_type(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->cmd_type_value = hal_param->video_snapshot_type;
    return ret;
}

static cmr_int setting_set_preview_format(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    // deprecated after sharkl5,minicamera,test,bbat need translating format at
    // present
    if (parm->cmd_type_value == IMG_DATA_TYPE_YUV420) {
        hal_param->preview_format = CAM_IMG_FMT_YUV420_NV21;
    } else {
        hal_param->preview_format = parm->cmd_type_value;
    }
    CMR_LOGD("format=%ld", hal_param->preview_format);
    return ret;
}

static cmr_int
setting_set_sprd_zsl_enabled(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sprd_zsl_enabled = parm->cmd_type_value;
    CMR_LOGD("sprd_zsl_enabled=%ld", hal_param->sprd_zsl_enabled);
    return ret;
}

static cmr_int
setting_set_face_attributes_enable(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->face_attributes_enabled = parm->cmd_type_value;
    CMR_LOGD("face_attributes_enabled=%ld", hal_param->face_attributes_enabled);
    return ret;
}

static cmr_int
setting_set_sprd_pipviv_enabled(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sprd_pipviv_enabled = parm->cmd_type_value;

    CMR_LOGD("sprd_pipviv_enabled=%ld", hal_param->sprd_pipviv_enabled);
    return ret;
}

static cmr_int
setting_set_sprd_eis_enabled(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sprd_eis_enabled = parm->cmd_type_value;
    CMR_LOGD("sprd_eis_enabled=%ld", hal_param->sprd_eis_enabled);
    return ret;
}

static cmr_int
setting_set_slow_motion_flag(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->video_slow_motion_flag = parm->cmd_type_value;
    CMR_LOGD("video_slow_motion_flag=%ld", hal_param->video_slow_motion_flag);
    return ret;
}

static cmr_int setting_set_refocus_enable(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->refoucs_enable = parm->cmd_type_value;
    CMR_LOGD("refoucs_enable=%ld", hal_param->refoucs_enable);
    return ret;
}

static cmr_int setting_set_3dnr_enable(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->is_3dnr = parm->cmd_type_value;
    CMR_LOGD("sprd_3dnr_enable=%ld", hal_param->is_3dnr);

    return ret;
}

static cmr_int setting_set_3dnr_type(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sprd_3dnr_type = parm->cmd_type_value;
    CMR_LOGD("sprd_3dnr_type=%ld", hal_param->sprd_3dnr_type);

    return ret;
}

static cmr_int setting_set_appmode(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->app_mode = parm->cmd_type_value;

    CMR_LOGD("setting_set_appmode=%ld", hal_param->app_mode);
    ret = setting_set_general(cpt, SETTING_GENERAL_SPRD_APP_MODE, parm);
    return ret;
}

static cmr_int setting_set_cnr(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->is_cnr = parm->cmd_type_value;

    CMR_LOGD("setting_set_cnr=%ld", hal_param->is_cnr);
    return ret;
}

static cmr_int setting_set_ee(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->is_ee = parm->cmd_type_value;

    CMR_LOGD("setting_set_ee=%ld", hal_param->is_ee);
    return ret;
}


static cmr_int setting_set_touch_xy(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {

    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct setting_io_parameter io_param;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    cmr_bzero(&io_param, sizeof(struct setting_io_parameter));
    if (init_in->io_cmd_ioctl) {
        io_param.touch_xy.touchX = parm->touch_param.touchX;
        io_param.touch_xy.touchY = parm->touch_param.touchY;
        hal_param->touch_info.touchX = parm->touch_param.touchX;
        hal_param->touch_info.touchY = parm->touch_param.touchY;
        CMR_LOGD("touch_param %d %d", parm->touch_param.touchX,
                 parm->touch_param.touchY);
        ret = (*init_in->io_cmd_ioctl)(init_in->oem_handle,
                                       SETTING_IO_SET_TOUCH, &io_param);
    }
    return ret;
}

static cmr_int
setting_set_video_snapshot_type(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    hal_param->video_snapshot_type = parm->cmd_type_value;
    CMR_LOGD("video_snapshot_type=%ld", hal_param->video_snapshot_type);
    return ret;
}

static cmr_int
setting_get_3dcalibration_enable(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_3dcalibration_enable;
    CMR_LOGD("3dcalibration_enable=%ld", hal_param->sprd_3dcalibration_enable);
    return ret;
}

static cmr_int
setting_set_3dcalibration_enable(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct common_isp_cmd_param isp_param;
    struct setting_init_in *init_in = &cpt->init_in;
    cmr_bzero(&isp_param, sizeof(struct common_isp_cmd_param));
    hal_param->sprd_3dcalibration_enable = parm->cmd_type_value;

    if (init_in && init_in->setting_isp_ioctl) {
        CMR_LOGD("set COM_ISP_SET_ROI_CONVERGENCE_REQ? %d",
                 0 != init_in->setting_isp_ioctl);
        isp_param.camera_id = parm->camera_id;
        isp_param.cmd_value = parm->cmd_type_value;
        CMR_LOGD("set COM_ISP_SET_ROI_CONVERGENCE_REQ begin, ome_handle:%p",
                 init_in->oem_handle);
        ret = (*init_in->setting_isp_ioctl)(
            init_in->oem_handle, COM_ISP_SET_ROI_CONVERGENCE_REQ, &isp_param);
    }
    CMR_LOGD("3dcalibration_enable=%ld", hal_param->sprd_3dcalibration_enable);
    return ret;
}

static cmr_int
setting_set_yuv_callback_enable(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sprd_yuv_callback_enable = parm->cmd_type_value;
    CMR_LOGD("sprd_yuv_callback_enable=%ld",
             hal_param->sprd_yuv_callback_enable);

    return ret;
}

static cmr_int
setting_get_yuv_callback_enable(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_yuv_callback_enable;
    CMR_LOGD("sprd_yuv_callback_enable=%ld",
             hal_param->sprd_yuv_callback_enable);

    return ret;
}

static cmr_int
setting_set_exif_exposure_time(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->ExposureTime = *((EXIF_RATIONAL_T *)parm->cmd_type_value);
    CMR_LOGD("ExposureTime %d %d", hal_param->ExposureTime.numerator,
             hal_param->ExposureTime.denominator);

    return ret;
}

static cmr_int setting_get_touch_info(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->touch_param.touchX = hal_param->touch_info.touchX;
    parm->touch_param.touchY = hal_param->touch_info.touchY;
    CMR_LOGD("touch_param %d %d", parm->touch_param.touchX,
             parm->touch_param.touchY);
    return ret;
}

static cmr_int setting_set_capture_size(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->capture_size = parm->size_param;
    return ret;
}

static cmr_int setting_set_capture_format(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    hal_param->capture_format = parm->cmd_type_value;
    CMR_LOGD("format=%ld", hal_param->capture_format);
    return ret;
}

static cmr_int setting_get_capture_size(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->capture_size;
    return ret;
}

static cmr_uint setting_convert_rotation_to_angle(struct setting_component *cpt,
                                                  cmr_int camera_id,
                                                  cmr_uint rotation) {
    cmr_uint temp_angle = IMG_ANGLE_0;
    cmr_int camera_orientation[CAMERA_ID_MAX];
    struct setting_hal_param *hal_param = get_hal_param(cpt, camera_id);

    cmr_bzero(camera_orientation, sizeof(camera_orientation));
#ifdef CONFIG_FRONT_CAMERA_ROTATION
    camera_orientation[CAMERA_ID_1] = 1; /*need to rotate*/
#endif

#ifdef CONFIG_BACK_CAMERA_ROTATION
    camera_orientation[CAMERA_ID_0] = 1; /*need to rotate*/
#endif

    if (camera_id >= CAMERA_ID_MAX) {
        CMR_LOGE("camera id not support");
        return temp_angle;
    }
    if ((0 == camera_orientation[camera_id]) &&
        (0 == hal_param->sensor_orientation)) {
        return temp_angle;
    }
    if (camera_orientation[camera_id]) {
        switch (rotation) {
        case 0:
            temp_angle = IMG_ANGLE_90;
            break;

        case 90:
            temp_angle = IMG_ANGLE_180;
            break;

        case 180:
            temp_angle = IMG_ANGLE_270;
            break;

        case 270:
            temp_angle = 0;
            break;

        default:
            break;
        }
    } else {
        switch (rotation) {
        case 0:
            temp_angle = IMG_ANGLE_0;
            break;

        case 90:
            temp_angle = IMG_ANGLE_90;
            break;

        case 180:
            temp_angle = IMG_ANGLE_180;
            break;

        case 270:
            temp_angle = IMG_ANGLE_270;
            break;

        default:
            break;
        }
    }
    CMR_LOGD("angle=%ld", temp_angle);

    return temp_angle;
}

static cmr_int setting_set_capture_angle(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    cmr_uint angle = 0;

    angle = setting_convert_rotation_to_angle(cpt, parm->camera_id,
                                              parm->cmd_type_value);
    ret = setting_set_general(cpt, SETTING_GENERAL_SENSOR_ROTATION, parm);

    hal_param->preview_angle = angle;
    hal_param->capture_angle = angle;

    return ret;
}

static cmr_int
setting_set_perfect_skinlevel(struct setting_component *cpt,
                              struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    hal_param->perfect_skinlevel = parm->fb_param;
    return ret;
}

static cmr_int
setting_get_perfect_skinlevel(struct setting_component *cpt,
                              struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->fb_param = hal_param->perfect_skinlevel;
    return ret;
}

static cmr_int setting_set_flip_on(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    hal_param->flip_on = parm->cmd_type_value;
    CMR_LOGD("hal_param->flip_on = %ld", hal_param->flip_on);
    return ret;
}

static cmr_int setting_get_flip_on(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->cmd_type_value = hal_param->flip_on;
    CMR_LOGD("hal_param->flip_on = %ld", hal_param->flip_on);
    return ret;
}

static cmr_int setting_get_preview_angle(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->preview_angle;
    return ret;
}

static cmr_int setting_get_capture_angle(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->capture_angle;
    return ret;
}

static cmr_int setting_get_thumb_size(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->thumb_size;
    return ret;
}

static cmr_int setting_set_thumb_size(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->thumb_size = parm->size_param;
    return ret;
}
static cmr_int
setting_set_sprd_filter_type(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    hal_param->sprd_filter_type = parm->cmd_type_value;
    CMR_LOGV(" set sprd_filter_type = %ld", hal_param->sprd_filter_type);
    return ret;
}

static cmr_int
setting_get_sprd_filter_type(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->cmd_type_value = hal_param->sprd_filter_type;
    CMR_LOGD("get sprd_filter_type = %ld", hal_param->sprd_filter_type);
    return ret;
}

static cmr_int setting_set_focus_distance(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    CMR_LOGI("focus_distance = %lu", parm->cmd_type_value);
    cpt->force_set = 1;
    ret = setting_set_general(cpt, SETTING_GENERAL_FOCUS_DISTANCE, parm);
    cpt->force_set = 0;
    return ret;
}

static cmr_int
setting_set_device_orientation(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("set device_orientation %d", parm->cmd_type_value);

    hal_param->device_orientation = parm->cmd_type_value;
    return ret;
}

static cmr_int
setting_get_device_orientation(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    if (cpt == NULL || parm == NULL) {
        CMR_LOGE("input param error");
        return ret;
    }

    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("device_orientation %d", hal_param->device_orientation);

    parm->cmd_type_value = hal_param->device_orientation;
    return ret;
}

static cmr_int setting_set_ae_mode(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    ret = setting_set_general(cpt, SETTING_GENERAL_AE_MODE, parm);

    return ret;
}

static cmr_int setting_set_auto_hdr(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("set auto hdr %ld", parm->cmd_type_value);

    ret = setting_set_general(cpt, SETTING_GENERAL_AUTO_HDR, parm);

    return ret;
}

static cmr_int setting_set_auto_fdr(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("set auto fdr %ld", parm->cmd_type_value);

    ret = setting_set_general(cpt, SETTING_GENERAL_AUTO_FDR, parm);

    return ret;
}

static cmr_int setting_set_auto_3dnr(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("set auto 3dnr %ld", parm->cmd_type_value);

    ret = setting_set_general(cpt, SETTING_GENERAL_AUTO_3DNR, parm);

    return ret;
}

static cmr_int setting_set_afbc_enable(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("set AFBC enable %ld", parm->cmd_type_value);

    hal_param->sprd_afbc_enabled = parm->cmd_type_value;

    return ret;
}
static cmr_int setting_set_auto_tracking_enable(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    CMR_LOGD("set auto tracking %ld", parm->cmd_type_value);
    hal_param->hal_common.is_auto_tracking = parm->cmd_type_value;
    ret = setting_set_general(cpt, SETTING_GENERAL_AUTO_TRACKING_INFO_ENABLE,
                              parm);
    if (ret)
        CMR_LOGE("set auto tracking error");
    return ret;
}

static cmr_int setting_get_auto_tracking_enable(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->hal_common.is_auto_tracking;
    CMR_LOGD("get auto tracking enabled %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_set_auto_tracking_status(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->ot_status = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_auto_tracking_status(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->cmd_type_value = hal_param->ot_status;

    return ret;
}

static cmr_int setting_set_logo_watermark(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sprd_logo_watermark = parm->cmd_type_value;
    CMR_LOGD("logo watermark=%d", hal_param->sprd_logo_watermark);

    return ret;
}

static cmr_int setting_get_logo_watermark(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_logo_watermark;
    CMR_LOGD("logo watermark=%d", hal_param->sprd_logo_watermark);

    return ret;
}


static cmr_int setting_set_time_watermark(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->sprd_time_watermark = parm->cmd_type_value;
    CMR_LOGD("time watermark=%d", hal_param->sprd_time_watermark);

    return ret;
}

static cmr_int setting_get_time_watermark(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_time_watermark;
    CMR_LOGD("time watermark=%d", hal_param->sprd_time_watermark);

    return ret;
}

static cmr_int setting_set_environment(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    struct setting_cmd_parameter cmd_param;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    cmr_uint invalid_word = 0;

    cpt->force_set = 1;

    memset(&invalid_word, INVALID_SETTING_BYTE, sizeof(cmr_uint));
    cmd_param = *parm;
    if (invalid_word != hal_param->hal_common.brightness) {
        cmd_param.cmd_type_value = hal_param->hal_common.brightness;
        ret = setting_set_brightness(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.contrast) {
        cmd_param.cmd_type_value = hal_param->hal_common.contrast;
        ret = setting_set_contrast(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.effect) {
        cmd_param.cmd_type_value = hal_param->hal_common.effect;
        ret = setting_set_effect(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.saturation) {
        cmd_param.cmd_type_value = hal_param->hal_common.saturation;
        ret = setting_set_saturation(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.sprd_appmode_id) {
        cmd_param.cmd_type_value = hal_param->hal_common.sprd_appmode_id;
        ret = setting_set_appmode(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word !=
        (cmr_uint)hal_param->hal_common.ae_compensation_param
            .ae_exposure_compensation) {
        cmd_param.ae_compensation_param =
            hal_param->hal_common.ae_compensation_param;
        ret = setting_set_exposure_compensation(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.wb_mode) {
        cmd_param.cmd_type_value = hal_param->hal_common.wb_mode;
        ret = setting_set_wb(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.antibanding_mode) {
        cmd_param.cmd_type_value = hal_param->hal_common.antibanding_mode;
        ret = setting_set_antibanding(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.auto_exposure_mode) {
        memset(&cmd_param, 0, sizeof(cmd_param));
        cmd_param.camera_id = parm->camera_id;
        cmd_param.ae_param.mode = hal_param->hal_common.auto_exposure_mode;
        ret = setting_set_auto_exposure_mode(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.ae_mode) {
        cmd_param.cmd_type_value = hal_param->hal_common.ae_mode;
        ret = setting_set_ae_mode(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.exposure_time &&
        hal_param->hal_common.ae_mode == 0) {
        cmd_param.cmd_type_value = hal_param->hal_common.exposure_time;
        ret = setting_set_exposure_time(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.iso) {
        cmd_param.cmd_type_value = hal_param->hal_common.iso;
        ret = setting_set_iso(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.scene_mode) {
        cmd_param.cmd_type_value = hal_param->hal_common.scene_mode;
        ret = setting_set_scene_mode(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }
    /*
    if (invalid_word != hal_param->hal_common.focus_distance) {
            cmd_param.cmd_type_value = hal_param->hal_common.focus_distance;
            ret = setting_set_focus_distance(cpt, &cmd_param);
            CMR_RTN_IF_ERR(ret);
    }
    */
    if (invalid_word != hal_param->hal_common.is_auto_hdr) {
        cmd_param.cmd_type_value = hal_param->hal_common.is_auto_hdr;
        ret = setting_set_auto_hdr(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (invalid_word != hal_param->hal_common.frame_rate) {
        setting_get_video_mode(cpt, parm);
        hal_param->hal_common.video_mode = parm->preview_fps_param.video_mode;
        cmd_param.camera_id = parm->camera_id;
        cmd_param.preview_fps_param.frame_rate =
            hal_param->hal_common.frame_rate;
        cmd_param.preview_fps_param.video_mode =
            hal_param->hal_common.video_mode;
        ret = setting_set_video_mode(cpt, &cmd_param);
        CMR_RTN_IF_ERR(ret);
    }

    if (hal_param->is_update_range_fps) {
        if (setting_is_rawrgb_format(cpt, parm)) {
            if (hal_param->range_fps.video_mode) {
                struct setting_cmd_parameter isoParm;
                cmr_bzero(&isoParm, sizeof(struct setting_cmd_parameter));
                isoParm.camera_id = parm->camera_id;
                isoParm.cmd_type_value = 5;
                if (setting_is_rawrgb_format(cpt, &isoParm)) {
                    // ret = setting_isp_ctrl(cpt, COM_ISP_SET_ISO, &isoParm);
                    // // remove ISO 1600 in video mode
                    if (ret) {
                        CMR_LOGE("iso set failed %ld", ret);
                    }
                }
            }
            cmd_param.range_fps = hal_param->range_fps;
            ret = setting_isp_ctrl(cpt, COM_ISP_SET_RANGE_FPS, &cmd_param);
        } else {
            hal_param->hal_common.frame_rate = hal_param->range_fps.max_fps;
            setting_get_video_mode(cpt, parm);
            cmd_param.cmd_type_value = parm->preview_fps_param.video_mode;
            ret = setting_sn_ctrl(cpt, COM_SN_SET_VIDEO_MODE, &cmd_param);
        }
    }

exit:
    cpt->force_set = 0;
    ATRACE_END();
    return ret;
}

static cmr_int setting_get_fdr(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->is_fdr;
    CMR_LOGD("get fdr %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_get_hdr(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->is_hdr;
    CMR_LOGD("get hdr %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_get_3dnr(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->is_3dnr;
    CMR_LOGD("get 3dnr %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_get_auto_3dnr(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->hal_common.is_auto_3dnr;
    CMR_LOGD("get cmd_type_value %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_get_3dnr_type(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_3dnr_type;
    CMR_LOGD("get 3dnr type %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_get_afbc_enabled(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->sprd_afbc_enabled;
    CMR_LOGD("get afbc enabled %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_get_appmode(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->app_mode;
    CMR_LOGD("get appmode %ld", parm->cmd_type_value);

    return ret;
}

static cmr_int setting_get_cnr(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->cmd_type_value = hal_param->is_cnr;

    CMR_LOGD("get cnr %ld", parm->cmd_type_value);
    return ret;
}

static cmr_int setting_get_ee(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->cmd_type_value = hal_param->is_ee;

    CMR_LOGD("get ee %ld", parm->cmd_type_value);
    return ret;
}


static cmr_int setting_ctrl_hdr(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    cmr_setting_clear_sem(cpt);
    CMR_LOGD("start wait for hdr ev effect");
    setting_isp_wait_notice(cpt);
    CMR_LOGD("end wait for hdr ev effect");

    return ret;
}

static cmr_int setting_clear_hdr(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {

    if (!cpt) {
        CMR_LOGE("camera_context is null.");
        return -1;
    }

    pthread_mutex_lock(&cpt->isp_mutex);
    cmr_sem_post(&cpt->isp_sem);
    pthread_mutex_unlock(&cpt->isp_mutex);

    CMR_LOGD("Done");
    return 0;
}

static cmr_int setting_ctrl_fdr(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    cmr_setting_clear_sem(cpt);
    CMR_LOGD("start wait for fdr ev effect");
    ret = setting_isp_wait_notice(cpt);
    CMR_LOGD("end wait for fdr ev effect");

    return ret;
}

static cmr_int setting_clear_fdr(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {

    if (!cpt) {
        CMR_LOGE("camera_context is null.");
        return -1;
    }

    pthread_mutex_lock(&cpt->isp_mutex);
    cmr_sem_post(&cpt->isp_sem);
    pthread_mutex_unlock(&cpt->isp_mutex);

    CMR_LOGD("Done");
    return 0;
}

static cmr_int setting_ctrl_ae_adjust(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;

    CMR_LOGD("start wait for isp ev effect");
    setting_isp_wait_notice(cpt);
    CMR_LOGD("end wait for isp ev effect");

    return ret;
}

static cmr_int setting_clear_ae_adjust(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {

    cmr_setting_clear_sem(cpt);
    CMR_LOGD("Done");
    return 0;
}
static cmr_int setting_is_need_flash(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int is_need = 0;
    enum takepicture_mode capture_mode = 0;
    cmr_int shot_num = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    cmr_uint flash_status = 0;
    cmr_uint flash_mode = 0;
    cmr_uint pre_flash_status = 0;

    capture_mode = hal_param->capture_mode;
    flash_status = hal_param->flash_param.flash_status;
    flash_mode = hal_param->flash_param.flash_mode;
    shot_num = hal_param->shot_num;
    pre_flash_status = hal_param->flash_param.has_preflashed;

    CMR_LOGD("flash_mode=%ld, flash_status=%ld, capture_mode=%d, shot_num=%ld",
             flash_mode, flash_status, capture_mode, shot_num);

    if (CAMERA_FLASH_MODE_TORCH != flash_mode && flash_status &&
        (flash_status != SETTING_FLASH_MAIN_LIGHTING ||
         (flash_status == SETTING_FLASH_MAIN_LIGHTING && pre_flash_status))) {
        if (CAMERA_NORMAL_MODE == capture_mode ||
            CAMERA_ISP_SIMULATION_MODE == capture_mode ||
            CAMERA_ZSL_MODE == capture_mode || shot_num > 1) {
            is_need = 1;
        }
        if (CAMERA_ISP_TUNING_MODE == capture_mode) {
            is_need = 0;
        }
    }

    if (FLASH_NEED_QUIT == cpt->flash_need_quit) {
        is_need = 0;
    }

    return is_need;
}

static cmr_int
setting_get_flash_max_capacity(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm,
                               uint16_t *max_time, uint16_t *max_charge) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct setting_io_parameter io_param;

    if (init_in->io_cmd_ioctl) {
        io_param.camera_id = parm->camera_id;
        ret = (*init_in->io_cmd_ioctl)(
            init_in->oem_handle, SETTING_IO_GET_FLASH_MAX_CAPACITY, &io_param);
        if (0 == ret) {
            *max_time = io_param.flash_capacity.max_time;
            *max_charge = io_param.flash_capacity.max_charge;
        }
    }

    return ret;
}

static cmr_int setting_isp_flash_notify(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm,
                                        enum isp_flash_mode flash_mode) {
    struct setting_init_in *init_in = &cpt->init_in;
    struct common_isp_cmd_param isp_param;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    cmr_int ret = 0;
    cmr_int will_capture = 0;

    if (!setting_is_rawrgb_format(cpt, parm)) {
        return ret;
    }

    cmr_bzero(&isp_param, sizeof(isp_param));
    isp_param.camera_id = parm->camera_id;
    switch (flash_mode) {
    case ISP_FLASH_PRE_BEFORE: {
        uint16_t max_time = 0;
        uint16_t max_charge = 0;
        struct setting_local_param *local_param =
            get_local_param(cpt, parm->camera_id);

        setting_get_sensor_static_info(cpt, parm,
                                       &local_param->sensor_static_info);
        ret = setting_get_flash_max_capacity(cpt, parm, &max_time, &max_charge);
        isp_param.flash_notice.led_info.led_tag = camera_get_flashled_flag(
            isp_param.camera_id); // ISP_FLASH_LED_0 | ISP_FLASH_LED_1;
        isp_param.flash_notice.led_info.power_0.max_charge = max_charge;
        isp_param.flash_notice.led_info.power_0.max_time = max_time;
        isp_param.flash_notice.led_info.power_1.max_charge = max_charge;
        isp_param.flash_notice.led_info.power_1.max_time = max_time;
        isp_param.flash_notice.capture_skip_num =
            local_param->sensor_static_info.capture_skip_num;

        CMR_LOGD("max_time=%d, max_charge=%d", max_time, max_charge);
    } break;

    case ISP_FLASH_PRE_LIGHTING: {
        struct common_sn_cmd_param sn_param;

        sn_param.camera_id = parm->camera_id;
        if (init_in->setting_sn_ioctl) {
            if ((*init_in->setting_sn_ioctl)(
                    init_in->oem_handle, COM_SN_GET_FLASH_LEVEL, &sn_param)) {
                CMR_LOGE("get flash level error.");
            }
        }
        /*because hardware issue high equal to low, so use hight div high */
        if (0 != sn_param.flash_level.low_light)
            isp_param.flash_notice.flash_ratio =
                sn_param.flash_level.high_light * 256 /
                sn_param.flash_level.low_light;
    } break;
    default:
        break;
    }

    isp_param.flash_notice.mode = flash_mode;
    isp_param.flash_notice.will_capture =
        (hal_param->flash_param.flash_status == SETTING_FLASH_PRE_AFTER ||
         hal_param->flash_param.flash_status == SETTING_AF_FLASH_PRE_AFTER)
            ? 1
            : 0;

    CMR_LOGV("will_capture (%d) flash_status (%d) ",
             isp_param.flash_notice.will_capture,
             hal_param->flash_param.flash_status);

    if (init_in->setting_isp_ioctl && (FLASH_NEED_QUIT != cpt->flash_need_quit)) {
        ret = (*init_in->setting_isp_ioctl)(
            init_in->oem_handle, COM_ISP_SET_FLASH_NOTICE, &isp_param);
    }

    if (ret) {
        CMR_LOGE("setting flash error.");
    }

    return ret;
}

static cmr_int setting_set_flashdevice(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm,
                                       enum sprd_flash_status flash_hw_status) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct setting_io_parameter io_param;
    struct setting_flash_param *flash_param =
        get_flash_param(cpt, parm->camera_id);

    CMR_LOGD("flash_status=%d", flash_hw_status);
    if (init_in->io_cmd_ioctl) {
        io_param.camera_id = parm->camera_id;
        io_param.cmd_value = flash_hw_status;
        ret = (*init_in->io_cmd_ioctl)(init_in->oem_handle,
                                       SETTING_IO_CTRL_FLASH, &io_param);
    }
    flash_param->flash_hw_status = flash_hw_status;

    return ret;
}

static cmr_int setting_ctrl_flash(struct setting_component *cpt,
                                  struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    cmr_uint flash_mode = 0;
    cmr_uint flash_hw_status = 0;
    cmr_uint image_format = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    enum sprd_flash_status ctrl_flash_status = 0;
    enum setting_flash_status setting_flash_status = 0;

    cmr_int tmpVal = 0;
    cmr_s64 time1 = 0, time2 = 0;
    image_format = local_param->sensor_static_info.image_format;
    flash_mode = hal_param->flash_param.flash_mode;
    flash_hw_status = hal_param->flash_param.flash_hw_status;
    setting_flash_status = parm->setting_flash_status;
    CMR_LOGD(" flash_mode %ld, setting_flash_status %d flash_status %ld",
             flash_mode, (cmr_u32)setting_flash_status, flash_hw_status);

    if ((CAMERA_FLASH_MODE_AUTO == flash_mode) &&
        (setting_flash_status == SETTING_AF_FLASH_PRE_LIGHTING)) {
        ret = setting_flash_handle(cpt, parm, flash_mode);
    }

    if (setting_is_need_flash(cpt, parm) ||
        (FLASH_NEED_QUIT == cpt->flash_need_quit &&
         setting_flash_status == SETTING_AF_FLASH_PRE_AFTER)) {
        switch (setting_flash_status) {
        case SETTING_FLASH_PRE_LIGHTING:
        case SETTING_AF_FLASH_PRE_LIGHTING:
            ctrl_flash_status = FLASH_OPEN;
            if (flash_hw_status == FLASH_OPEN) {
                cmr_setting_clear_sem(cpt);
                hal_param->flash_param.flash_status = setting_flash_status;
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_LIGHTING);
                setting_isp_wait_notice(cpt);
                goto EXIT;
            }
            if (CAM_IMG_FMT_BAYER_MIPI_RAW == image_format) {
                CMR_LOGD("pre flash open");
                hal_param->flash_param.has_preflashed = 1;
                cmr_sem_getvalue(&cpt->preflash_sem, &tmpVal);
                while (0 < tmpVal) {
                    cmr_sem_trywait(&cpt->preflash_sem);
                    cmr_sem_getvalue(&cpt->preflash_sem, &tmpVal);
                }

                cmr_setting_clear_sem(cpt);
                time1 = systemTime(CLOCK_MONOTONIC);
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_BEFORE);
                setting_isp_wait_notice(cpt);
                time2 = systemTime(CLOCK_MONOTONIC);
                CMR_LOGI("isp_flash_pre_before cost %" PRId64 " ms",
                         (time2 - time1) / 1000000);

                if (FLASH_NEED_QUIT == cpt->flash_need_quit) {
                    goto EXIT;
                }

                setting_set_flashdevice(cpt, parm, ctrl_flash_status);
                hal_param->flash_param.last_preflash_time =
                    systemTime(CLOCK_MONOTONIC);
                hal_param->flash_param.flash_status = setting_flash_status;
                time1 = systemTime(CLOCK_MONOTONIC);
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_LIGHTING);
                setting_isp_wait_notice(cpt);
                time2 = systemTime(CLOCK_MONOTONIC);
                CMR_LOGI("isp_flash_pre_lighting cost %" PRId64 " ms",
                         (time2 - time1) / 1000000);
                hal_param->flash_param.has_preflashed = 1;

            } else {
                setting_set_flashdevice(cpt, parm, ctrl_flash_status);
            }
            break;

        case SETTING_FLASH_PRE_AFTER:
        case SETTING_FLASH_MAIN_AFTER:
        case SETTING_AF_FLASH_PRE_AFTER:
        case SETTING_FLASH_WAIT_TO_CLOSE:
            ctrl_flash_status = FLASH_CLOSE_AFTER_OPEN;
            if (CAM_IMG_FMT_BAYER_MIPI_RAW == image_format) {
                /*disable*/
                if (FLASH_CLOSE != flash_hw_status) {
                    if ((uint32_t)CAMERA_FLASH_MODE_TORCH != flash_mode) {
                        setting_set_flashdevice(cpt, parm,
                                                FLASH_CLOSE_AFTER_OPEN);
                        CMR_LOGD("flash close");
                    }

                    if (setting_flash_status == SETTING_AF_FLASH_PRE_AFTER ||
                        setting_flash_status == SETTING_FLASH_PRE_AFTER) {
                        if ((uint32_t)CAMERA_FLASH_MODE_TORCH != flash_mode) {
                            hal_param->flash_param.flash_status =
                                setting_flash_status;
                            setting_isp_flash_notify(cpt, parm,
                                                     ISP_FLASH_PRE_AFTER);
                            cmr_sem_post(&cpt->preflash_sem);
                        }
                    } else {
                        setting_isp_flash_notify(cpt, parm, ISP_FLASH_CLOSE);
                        /* no need when non-zsl, if do should timeout */
                        if (hal_param->sprd_zsl_enabled)
                            setting_isp_wait_notice(cpt);
                        hal_param->flash_param.has_preflashed = 0;
                        hal_param->flash_param.last_preflash_time = 0;
                        hal_param->flash_param.flash_status =
                            setting_flash_status;
                        setting_isp_flash_notify(cpt, parm,
                                                 ISP_FLASH_MAIN_AFTER);
                    }
                }
            } else {
                setting_set_flashdevice(cpt, parm, ctrl_flash_status);
            }
            break;

        case SETTING_FLASH_MAIN_LIGHTING: // high flash
            ctrl_flash_status = FLASH_HIGH_LIGHT;
            if (CAM_IMG_FMT_BAYER_MIPI_RAW == image_format) {
                CMR_LOGD("high flash main before");
                cmr_setting_clear_sem(cpt);
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_MAIN_BEFORE);
                setting_isp_wait_notice(cpt);

                if (FLASH_NEED_QUIT == cpt->flash_need_quit) {
                    goto EXIT;
                }

                CMR_LOGD("high flash open flash");
                setting_set_flashdevice(cpt, parm, ctrl_flash_status);
                hal_param->flash_param.flash_status = setting_flash_status;

                setting_isp_flash_notify(cpt, parm, ISP_FLASH_MAIN_LIGHTING);
                /* no need when non-zsl, if do should timeout */
                if (hal_param->sprd_zsl_enabled)
                   setting_isp_wait_notice(cpt);
                CMR_LOGD("high flash will do-capture");
            } else {
                setting_set_flashdevice(cpt, parm, ctrl_flash_status);
            }
            break;
        default:
            ctrl_flash_status = flash_hw_status;
            CMR_LOGD("reserved flash ctrl");
            break;
        }
    }

EXIT:
    return ret;
}

static cmr_int setting_get_capture_mode(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->capture_mode;

    if (hal_param->is_hdr) {
        parm->cmd_type_value = CAMERA_CAP_MODE_HDR;
    }
    return ret;
}

static cmr_int setting_set_capture_mode(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->capture_mode = parm->cmd_type_value;
    return ret;
}

static cmr_int
setting_get_rotation_capture(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->is_rotation_capture;
    return ret;
}

static cmr_int
setting_set_rotation_capture(struct setting_component *cpt,
                             struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->is_rotation_capture = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_set_android_zsl(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->is_android_zsl = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_android_zsl(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->is_android_zsl;
    return ret;
}

static cmr_int setting_set_video_size(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->video_size = parm->size_param;
    return ret;
}

static cmr_int setting_get_video_size(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->video_size;

    return ret;
}

static cmr_int setting_set_video_format(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->video_format = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_video_format(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->video_format;

    return ret;
}

static cmr_int
setting_set_yuv_callback_size(struct setting_component *cpt,
                              struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->yuv_callback_size = parm->size_param;
    return ret;
}

static cmr_int
setting_get_yuv_callback_size(struct setting_component *cpt,
                              struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->yuv_callback_size;
    return ret;
}

static cmr_int
setting_set_yuv_callback_format(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->yuv_callback_format = parm->cmd_type_value;
    return ret;
}

static cmr_int
setting_get_yuv_callback_format(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->yuv_callback_format;
    return ret;
}

static cmr_int setting_set_yuv2_size(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->yuv2_size = parm->size_param;
    return ret;
}

static cmr_int setting_get_yuv2_size(struct setting_component *cpt,
                                     struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->yuv2_size;
    return ret;
}

static cmr_int setting_set_yuv2_format(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->yuv2_format = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_yuv2_format(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->yuv2_format;
    return ret;
}

static cmr_int setting_set_raw_size(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->raw_size = parm->size_param;
    return ret;
}

static cmr_int setting_get_raw_size(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->size_param = hal_param->raw_size;
    return ret;
}

static cmr_int setting_set_raw_format(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->raw_format = parm->cmd_type_value;
    return ret;
}

static cmr_int setting_get_raw_format(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->raw_format;
    return ret;
}

static cmr_int setting_get_dv_mode(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);

    parm->cmd_type_value = local_param->is_dv_mode;
    return ret;
}

static cmr_int setting_zoom_update_status(struct setting_component *cpt,
                                          enum zoom_status status) {
    cmr_int ret = 0;

    pthread_mutex_lock(&cpt->ctrl_lock);
    cpt->zoom_unit.status = status;
    if (ZOOM_IDLE == status) {
        cpt->zoom_unit.is_sended_msg = 0;
    }
    pthread_mutex_unlock(&cpt->ctrl_lock);

    return ret;
}

static cmr_int setting_is_zoom_pull(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int is_changed = 0;

    pthread_mutex_lock(&cpt->ctrl_lock);
    is_changed = cpt->zoom_unit.is_changed;
    if (is_changed) {
        *parm = cpt->zoom_unit.in_zoom;
        cpt->zoom_unit.is_changed = 0;
    }
    pthread_mutex_unlock(&cpt->ctrl_lock);

    return is_changed;
}

static cmr_int setting_thread_proc(struct cmr_msg *message, void *data) {
    cmr_int ret = 0;
    cmr_int evt;
    struct setting_component *cpt = (struct setting_component *)data;

    if (!message || !data) {
        CMR_LOGE("param error");
        goto setting_proc_out;
    }

    CMR_LOGV("message.msg_type 0x%x", message->msg_type);
    evt = (message->msg_type & SETTING_EVT_MASK_BITS);

    switch (evt) {
    case SETTING_EVT_INIT:
        break;

    case SETTING_EVT_DEINIT:

        break;
    case SETTING_EVT_ZOOM: {
        struct setting_cmd_parameter new_zoom_param;

        while (setting_is_zoom_pull(cpt, &new_zoom_param)) {
            setting_zoom_update_status(cpt, ZOOM_UPDATING);
            ret = setting_process_zoom(cpt, &new_zoom_param);
        }
        setting_zoom_update_status(cpt, ZOOM_IDLE);
    } break;
    default:
        CMR_LOGE("not correct message");
        break;
    }

setting_proc_out:
    CMR_LOGV("X, ret %ld", ret);
    return ret;
}

static cmr_int setting_get_exif_pic_info(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_init_in *init_in = &cpt->init_in;
    struct common_sn_cmd_param sn_param;

    if (init_in->setting_sn_ioctl) {
        ret = (*init_in->setting_sn_ioctl)(
            init_in->oem_handle, COM_SN_GET_EXIF_IMAGE_INFO, &sn_param);
        if (ret) {
            CMR_LOGD("sn ctrl failed");
        }
        cmr_copy(&parm->exif_pic_cond_info, &sn_param.exif_pic_info,
                 sizeof(parm->exif_pic_cond_info));
    }
    return ret;
}

static cmr_int
setting_get_pre_lowflash_value(struct setting_component *cpt,
                               struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->flash_param.has_preflashed;
    CMR_LOGD("hal_param->flash_param.has_preflashed=%ld",
             hal_param->flash_param.has_preflashed);

    return ret;
}

static cmr_int cmr_setting_clear_sem(struct setting_component *cpt) {
    int tmpVal = 0;
    struct camera_context *cxt = NULL;

    if (!cpt) {
        CMR_LOGE("camera_context is null.");
        return -1;
    }

    cxt = (struct camera_context *)cpt->init_in.oem_handle;

    pthread_mutex_lock(&cpt->isp_mutex);
    sem_getvalue(&cpt->isp_sem, &tmpVal);
    while (0 < tmpVal && cpt->flash_need_quit != FLASH_NEED_QUIT) {
        sem_wait(&cpt->isp_sem);
        sem_getvalue(&cpt->isp_sem, &tmpVal);
    }

    if (1 == camera_get_fdr_flag(cxt)) {
        while (0 < tmpVal) {
            sem_wait(&cpt->isp_sem);
            sem_getvalue(&cpt->isp_sem, &tmpVal);
        }
    }

    sem_getvalue(&cpt->quick_ae_sem, &tmpVal);
    while (0 < tmpVal) {
        sem_wait(&cpt->quick_ae_sem);
        sem_getvalue(&cpt->quick_ae_sem, &tmpVal);
    }

    sem_getvalue(&cpt->preflash_sem, &tmpVal);
    while (0 < tmpVal) {
        sem_wait(&cpt->preflash_sem);
        sem_getvalue(&cpt->preflash_sem, &tmpVal);
    }
    pthread_mutex_unlock(&cpt->isp_mutex);

    return 0;
}

static cmr_int setting_isp_wait_notice(struct setting_component *cpt) {
    return setting_isp_wait_notice_withtime(cpt, ISP_ALG_TIMEOUT);
}

static cmr_int setting_isp_wait_notice_withtime(struct setting_component *cpt,
                                                cmr_uint timeout) {
    cmr_int rtn = 0;
    struct timespec ts;

    if (!cpt) {
        CMR_LOGE("camera_context is null.");
        return -1;
    }
    pthread_mutex_lock(&cpt->isp_mutex);
    if (clock_gettime(CLOCK_REALTIME, &ts)) {
        rtn = -1;
        CMR_LOGE("get time failed.");
    } else {
        ts.tv_sec += timeout;
        pthread_mutex_unlock(&cpt->isp_mutex);
        if (cmr_sem_timedwait((&cpt->isp_sem), &ts)) {
            pthread_mutex_lock(&cpt->isp_mutex);
            rtn = -1;
            cpt->isp_is_timeout = 1;
            CMR_LOGW("timeout.");
        } else {
            pthread_mutex_lock(&cpt->isp_mutex);
            cpt->isp_is_timeout = 0;
            CMR_LOGD("done.");
        }
    }
    pthread_mutex_unlock(&cpt->isp_mutex);

    return rtn;
}

cmr_int cmr_setting_isp_notice_done(cmr_handle setting_handle, void *data) {
    struct setting_component *cpt = (struct setting_component *)setting_handle;
    UNUSED(data);
    if (!cpt) {
        CMR_LOGE("camera_context is null.");
        return -1;
    }

    pthread_mutex_lock(&cpt->isp_mutex);
    CMR_LOGD("isp notice done.");
    // if (0 == cpt->isp_is_timeout) {
    cmr_sem_post(&cpt->isp_sem);
    //} else {
    //	cpt->isp_is_timeout = 0;
    //}
    pthread_mutex_unlock(&cpt->isp_mutex);

    return 0;
}

static cmr_int setting_quick_ae_wait_notice(struct setting_component *cpt) {
    cmr_int rtn = 0;
    struct timespec ts;
    cmr_int tmpVal = 0;

    if (!cpt) {
        CMR_LOGE("camera_context is null.");
        return -1;
    }

    pthread_mutex_lock(&cpt->isp_mutex);
    if (clock_gettime(CLOCK_REALTIME, &ts)) {
        rtn = -1;
        CMR_LOGE("get time failed.");
    } else {
        ts.tv_sec += ISP_QUICK_AE_TIMEOUT;
        pthread_mutex_unlock(&cpt->isp_mutex);
        if (cmr_sem_timedwait((&cpt->quick_ae_sem), &ts)) {
            pthread_mutex_lock(&cpt->isp_mutex);
            rtn = -1;
            CMR_LOGW("timeout.");
        } else {
            pthread_mutex_lock(&cpt->isp_mutex);
            CMR_LOGD("done.");
        }
    }
    pthread_mutex_unlock(&cpt->isp_mutex);
    return rtn;
}

cmr_int cmr_setting_quick_ae_notice_done(cmr_handle setting_handle,
                                         void *data) {
    struct setting_component *cpt = (struct setting_component *)setting_handle;
    UNUSED(data);

    if (!cpt) {
        CMR_LOGE("camera_context is null.");
        return -1;
    }

    pthread_mutex_lock(&cpt->isp_mutex);
    CMR_LOGD("isp quick ae done.");
    cmr_sem_post(&cpt->quick_ae_sem);
    pthread_mutex_unlock(&cpt->isp_mutex);
    return 0;
}

static cmr_int setting_set_pre_lowflash(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    cmr_uint flash_mode = 0;
    cmr_uint image_format = 0;
    cmr_uint been_preflash = 0;
    cmr_s64 last_preflash_time = 0, now_time = 0, diff = 0;
    struct setting_init_in *init_in = &cpt->init_in;

    image_format = local_param->sensor_static_info.image_format;
    flash_mode = hal_param->flash_param.flash_mode;

    last_preflash_time = hal_param->flash_param.last_preflash_time;
    now_time = systemTime(CLOCK_MONOTONIC);
    CMR_LOGV("last_preflash_time = %" PRId64 ", now_time=%" PRId64,
             last_preflash_time, now_time);
    if (now_time > last_preflash_time) {
        diff = (now_time - last_preflash_time) / 1000000000;
        CMR_LOGV("diff = %" PRId64, diff);
        if (diff < PREFLASH_INTERVAL_TIME) {
            CMR_LOGD("last preflash < 3s, no need do preflash again.");
            hal_param->flash_param.has_preflashed = 1;
            been_preflash = hal_param->flash_param.has_preflashed;
        }
    }

    CMR_LOGD("preflash without af, image_format %ld, flash_mode %ld, "
             "been_preflash %ld",
             image_format, flash_mode, been_preflash);

    if (!been_preflash) {
        if (CAMERA_FLASH_MODE_AUTO == flash_mode) {
            ret = setting_flash_handle(cpt, parm, flash_mode);
        }

        if (setting_is_need_flash(cpt, parm)) {
            CMR_LOGD("preflash low open");
            hal_param->flash_param.has_preflashed = 1;
            if (CAM_IMG_FMT_BAYER_MIPI_RAW == image_format) {
                cmr_setting_clear_sem(cpt);
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_BEFORE);
                setting_isp_wait_notice_withtime(cpt, ISP_PREFLASH_ALG_TIMEOUT);
                /*before flash open,if camera close,go exit*/
                if (FLASH_NEED_QUIT == cpt->flash_need_quit) {
                    goto exit;
                }
                setting_set_flashdevice(cpt, parm, (uint32_t)FLASH_OPEN);
                hal_param->flash_param.flash_status =
                    SETTING_FLASH_PRE_LIGHTING;
                /*after flash open,if camera close,go exit*/
                if (FLASH_NEED_QUIT == cpt->flash_need_quit) {
                    goto exit;
                }
                cmr_setting_clear_sem(cpt);
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_LIGHTING);
                setting_isp_wait_notice_withtime(cpt, ISP_PREFLASH_ALG_TIMEOUT);
            } else {
                setting_set_flashdevice(cpt, parm, (uint32_t)FLASH_OPEN);
            }
        }

        if (setting_is_need_flash(cpt, parm)) {
            setting_set_flashdevice(cpt, parm,
                                    (uint32_t)FLASH_CLOSE_AFTER_OPEN);
            hal_param->flash_param.flash_status = SETTING_FLASH_PRE_AFTER;

            if (CAM_IMG_FMT_BAYER_MIPI_RAW == image_format) {
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_AFTER);
            }
            CMR_LOGD("preflash low close");
        }
    }

exit:
    return ret;
}

static cmr_int
setting_set_highflash_ae_measure(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    struct setting_local_param *local_param =
        get_local_param(cpt, parm->camera_id);
    cmr_uint flash_mode = 0;
    cmr_uint image_format = 0;
    struct setting_init_in *init_in = &cpt->init_in;

    image_format = local_param->sensor_static_info.image_format;
    flash_mode = hal_param->flash_param.flash_mode;

    if (setting_is_need_flash(cpt, parm)) {
        /*open flash*/
        if (CAM_IMG_FMT_BAYER_MIPI_RAW == image_format) {
            struct sensor_raw_info *raw_info_ptr = NULL;
            struct sensor_libuse_info *libuse_info = NULL;
            cmr_int product_id = 0;

            /*for third ae*/
            setting_get_sensor_static_info(cpt, parm,
                                           &local_param->sensor_static_info);
            raw_info_ptr = local_param->sensor_static_info.raw_info_ptr;
            if (raw_info_ptr) {
                libuse_info = raw_info_ptr->libuse_info;
                if (libuse_info) {
                    product_id = libuse_info->ae_lib_info.product_id;
                }
            }

            if (!product_id) {
                setting_isp_flash_notify(cpt, parm, ISP_FLASH_MAIN_AE_MEASURE);
                setting_quick_ae_wait_notice(cpt);
            }
        }
    }

    return ret;
}

static cmr_int setting_set_original_picture_size(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    hal_param->originalPictureSize.height = parm->originalPictureSize.height;
    hal_param->originalPictureSize.width = parm->originalPictureSize.width;
    return ret;
}

static cmr_int setting_get_original_picture_size(struct setting_component *cpt,
                                    struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);
    parm->originalPictureSize.height = hal_param->originalPictureSize.height;
    parm->originalPictureSize.width = hal_param->originalPictureSize.width;
    return ret;
}

static cmr_int set_super_macrophoto(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm){

    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    hal_param->is_super_macrophoto = parm->cmd_type_value;

    return ret;
}

static cmr_int get_super_macrophoto(struct setting_component *cpt,
                                struct setting_cmd_parameter *parm){

    cmr_int ret = 0;
    struct setting_hal_param *hal_param = get_hal_param(cpt, parm->camera_id);

    parm->cmd_type_value = hal_param->is_super_macrophoto;

    return ret;
}

static setting_ioctl_fun_ptr setting_list[SETTING_TYPE_MAX] = {

    [CAMERA_PARAM_ZOOM] = setting_set_zoom_param,
    [CAMERA_PARAM_REPROCESS_ZOOM_RATIO] =
                             setting_set_reprocess_zoom_ratio,
    [CAMERA_PARAM_ENCODE_ROTATION] =
                             setting_set_encode_angle,
    [CAMERA_PARAM_CONTRAST] = setting_set_contrast,
    [CAMERA_PARAM_BRIGHTNESS] = setting_set_brightness,
    [CAMERA_PARAM_AI_SCENE_ENABLED] =
                             setting_set_ai_scence,
    [CAMERA_PARAM_SHARPNESS] = setting_set_sharpness,
    [CAMERA_PARAM_WB] = setting_set_wb,
    [CAMERA_PARAM_EFFECT] = setting_set_effect,
    [CAMERA_PARAM_FLASH] = setting_set_flash_mode,
    [CAMERA_PARAM_ANTIBANDING] = setting_set_antibanding,
    [CAMERA_PARAM_FOCUS_RECT] = NULL, /*by focus module*/
    [CAMERA_PARAM_AF_MODE] = NULL,    /*by focus module*/
    [CAMERA_PARAM_AUTO_EXPOSURE_MODE] =
                             setting_set_auto_exposure_mode,
    [CAMERA_PARAM_ISO] = setting_set_iso,
    [CAMERA_PARAM_EXPOSURE_COMPENSATION] =
                             setting_set_exposure_compensation,
    [CAMERA_PARAM_PREVIEW_FPS] = setting_set_preview_fps,
    [CAMERA_PARAM_PREVIEW_LLS_FPS] =
                             setting_set_preview_lls_fps,
    [CAMERA_PARAM_SATURATION] = setting_set_saturation,
    [CAMERA_PARAM_SCENE_MODE] = setting_set_scene_mode,
    [CAMERA_PARAM_JPEG_QUALITY] =
                             setting_set_jpeg_quality,
    [CAMERA_PARAM_THUMB_QUALITY] =
                             setting_set_thumb_quality,
    [CAMERA_PARAM_SENSOR_ORIENTATION] =
                             setting_set_sensor_orientation,
    [CAMERA_PARAM_FOCAL_LENGTH] =
                             setting_set_focal_length,
    [CAMERA_PARAM_SENSOR_ROTATION] =
                             setting_set_capture_angle,
    [CAMERA_PARAM_PERFECT_SKIN_LEVEL] =
                             setting_set_perfect_skinlevel,
    [CAMERA_PARAM_FLIP_ON] = setting_set_flip_on,
    [CAMERA_PARAM_SHOT_NUM] = setting_set_shot_num,
    [CAMERA_PARAM_ROTATION_CAPTURE] =
                             setting_set_rotation_capture,
    [CAMERA_PARAM_POSITION] = setting_set_position,
    [CAMERA_PARAM_PREVIEW_SIZE] =
                             setting_set_preview_size,
    [CAMERA_PARAM_RAW_CAPTURE_SIZE] =
                             setting_set_raw_capture_size,
    [CAMERA_PARAM_PREVIEW_FORMAT] =
                             setting_set_preview_format,
    [CAMERA_PARAM_CAPTURE_SIZE] =
                             setting_set_capture_size,
    [CAMERA_PARAM_CAPTURE_FORMAT] =
                             setting_set_capture_format,
    [CAMERA_PARAM_CAPTURE_MODE] =
                             setting_set_capture_mode,
    [CAMERA_PARAM_THUMB_SIZE] = setting_set_thumb_size,
    [CAMERA_PARAM_ANDROID_ZSL] = setting_set_android_zsl,
    [CAMERA_PARAM_VIDEO_SIZE] = setting_set_video_size,
    [CAMERA_PARAM_VIDEO_FORMAT] =
                             setting_set_video_format,
    [CAMERA_PARAM_YUV_CALLBACK_SIZE] =
                             setting_set_yuv_callback_size,
    [CAMERA_PARAM_YUV_CALLBACK_FORMAT] =
                             setting_set_yuv_callback_format,
    [CAMERA_PARAM_YUV2_SIZE] = setting_set_yuv2_size,
    [CAMERA_PARAM_YUV2_FORMAT] = setting_set_yuv2_format,
    [CAMERA_PARAM_RAW_SIZE] = setting_set_raw_size,
    [CAMERA_PARAM_RAW_FORMAT] = setting_set_raw_format,
    [CAMERA_PARAM_RANGE_FPS] = setting_set_range_fps,
    [CAMERA_PARAM_ISP_FLASH] =
                             setting_set_isp_flash_mode,
    [CAMERA_PARAM_SPRD_ZSL_ENABLED] =
                             setting_set_sprd_zsl_enabled,
    [CAMERA_PARAM_ISP_AE_LOCK_UNLOCK] =
                             setting_set_ae_lock_unlock,
    [CAMERA_PARAM_SLOW_MOTION_FLAG] =
                             setting_set_slow_motion_flag,
    [CAMERA_PARAM_SPRD_PIPVIV_ENABLED] =
                             setting_set_sprd_pipviv_enabled,

    [CAMERA_PARAM_SPRD_EIS_ENABLED] =
                             setting_set_sprd_eis_enabled,
    [CAMERA_PARAM_REFOCUS_ENABLE] =
                             setting_set_refocus_enable,
    [CAMERA_PARAM_TOUCH_XY] = setting_set_touch_xy,
    [CAMERA_PARAM_VIDEO_SNAPSHOT_TYPE] =
                             setting_set_video_snapshot_type,
    [CAMERA_PARAM_SPRD_3DCAL_ENABLE] =
                             setting_set_3dcalibration_enable,

    [CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE] =
                             setting_set_yuv_callback_enable,

    [CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK] =
                             setting_set_awb_lock_unlock,
    [CAMERA_PARAM_AE_REGION] = setting_set_ae_region,
    [CAMERA_PARAM_SPRD_SET_APPMODE] =
                             setting_set_appmode,
    [CAMERA_PARAM_SPRD_ENABLE_CNR] = setting_set_cnr,
    [CAMERA_PARAM_SPRD_3DNR_ENABLED] =
                             setting_set_3dnr_enable,
    [CAMERA_PARAM_SPRD_3DNR_TYPE] =
                             setting_set_3dnr_type,
    [SETTING_SET_EXIF_EXPOSURE_TIME] =
                             setting_set_exif_exposure_time,
    [CAMERA_PARAM_TYPE_MAX] = NULL,
    [SETTING_GET_PREVIEW_ANGLE] =
                             setting_get_preview_angle,
    [SETTING_GET_CAPTURE_ANGLE] =
                             setting_get_capture_angle,
    [SETTING_GET_ZOOM_PARAM] = setting_get_zoom_param,
    [SETTING_GET_REPROCESS_ZOOM_RATIO] =
                             setting_get_reprocess_zoom_ratio,
    [SETTING_GET_ENCODE_ANGLE] =
                             setting_get_encode_angle,
    [SETTING_GET_EXIF_INFO] = setting_get_exif_info,
    [SETTING_GET_JPEG_QUALITY] =
                             setting_get_jpeg_quality,
    [SETTING_GET_THUMB_QUALITY] =
                             setting_get_thumb_quality,
    [SETTING_GET_THUMB_SIZE] = setting_get_thumb_size,
    [SETTING_GET_ROTATION_CAPTURE] =
                             setting_get_rotation_capture,
    [SETTING_GET_SHOT_NUMBER] = setting_get_shot_num,
    [SETTING_SET_ENVIRONMENT] = setting_set_environment,
    [SETTING_GET_CAPTURE_SIZE] =
                             setting_get_capture_size,
    [SETTING_GET_CAPTURE_FORMAT] =
                             setting_get_capture_format,
    [SETTING_GET_PREVIEW_SIZE] =
                             setting_get_preview_size,
    [SETTING_GET_RAW_CAPTURE_SIZE] =
                             setting_get_raw_capture_size,
    [SETTING_GET_PREVIEW_FORMAT] =
                             setting_get_preview_format,
    [SETTING_GET_VIDEO_SIZE] = setting_get_video_size,
    [SETTING_GET_VIDEO_FORMAT] =
                             setting_get_video_format,
    [SETTING_GET_YUV_CALLBACK_SIZE] =
                             setting_get_yuv_callback_size,
    [SETTING_GET_YUV_CALLBACK_FORMAT] =
                             setting_get_yuv_callback_format,
    [SETTING_GET_YUV2_SIZE] = setting_get_yuv2_size,
    [SETTING_GET_YUV2_FORMAT] = setting_get_yuv2_format,
    [SETTING_GET_RAW_SIZE] = setting_get_raw_size,
    [SETTING_GET_RAW_FORMAT] = setting_get_raw_format,
    [SETTING_GET_HDR] = setting_get_hdr,
    [SETTING_GET_3DNR] = setting_get_3dnr,
    [SETTING_GET_AUTO_3DNR] = setting_get_auto_3dnr,
    [SETTING_GET_3DNR_TYPE] = setting_get_3dnr_type,
    [SETTING_GET_ANDROID_ZSL_FLAG] =
                             setting_get_android_zsl,
    [SETTING_CTRL_FLASH] = setting_ctrl_flash,
    [SETTING_GET_CAPTURE_MODE] =
                             setting_get_capture_mode,
    [SETTING_GET_DV_MODE] = setting_get_dv_mode,
    [SETTING_SET_PRE_LOWFLASH] =
                             setting_set_pre_lowflash,
    [SETTING_GET_FLASH_STATUS] =
                             setting_get_flash_status,
    [SETTING_SET_HIGHFLASH_AE_MEASURE] =
                             setting_set_highflash_ae_measure,
    [SETTING_GET_HW_FLASH_STATUS] =
                             setting_get_HW_flash_status,
    [SETTING_GET_PERFECT_SKINLEVEL] =
                             setting_get_perfect_skinlevel,
    [SETTING_GET_FLIP_ON] = setting_get_flip_on,
    [SETTING_GET_SPRD_ZSL_ENABLED] =
                             setting_get_sprd_zsl_enabled,
    [SETTING_GET_SLOW_MOTION_FLAG] =
                             setting_get_slow_motion_flag,
    [SETTING_GET_SPRD_PIPVIV_ENABLED] =
                             setting_get_sprd_pipviv_enabled,

    [SETTING_GET_ENCODE_ROTATION] =
                             setting_get_encode_rotation,
    [SETTING_GET_SPRD_EIS_ENABLED] =
                             setting_get_sprd_eis_enabled,
    [SETTING_GET_REFOCUS_ENABLE] =
                             setting_get_refocus_enable,
    [SETTING_GET_TOUCH_XY] = setting_get_touch_info,
    [SETTING_GET_VIDEO_SNAPSHOT_TYPE] =
                             setting_get_video_snapshot_type,
    [SETTING_GET_EXIF_PIC_INFO] =
                             setting_get_exif_pic_info,
    [SETTING_GET_PRE_LOWFLASH_VALUE] =
                             setting_get_pre_lowflash_value,
    [SETTING_GET_SPRD_3DCAL_ENABLE] =
                             setting_get_3dcalibration_enable,

    [SETTING_GET_SPRD_YUV_CALLBACK_ENABLE] =
                             setting_get_yuv_callback_enable,
    [SETTING_CTRL_HDR] = setting_ctrl_hdr,
    [SETTING_CLEAR_HDR] = setting_clear_hdr,
    [SETTING_CTRL_FDR] = setting_ctrl_fdr,
    [SETTING_CLEAR_FDR] = setting_clear_fdr,
    [CAMERA_PARAM_EXIF_MIME_TYPE] =
                             setting_set_exif_mime_type,
    [SETTING_GET_APPMODE] = setting_get_appmode,
    [SETTING_GET_CNR] = setting_get_cnr,
    [CAMERA_PARAM_FILTER_TYPE] =
                             setting_set_sprd_filter_type,
    [SETTING_GET_FILTER_TEYP] =
                             setting_get_sprd_filter_type,
    [CAMERA_PARAM_LENS_FOCUS_DISTANCE] =
                             setting_set_focus_distance,
    [CAMERA_PARAM_SPRD_AUTO_HDR_ENABLED] =
                             setting_set_auto_hdr,
    [CAMERA_PARAM_SPRD_AUTO_FDR_ENABLED] =
                             setting_set_auto_fdr,
    [CAMERA_PARAM_SET_DEVICE_ORIENTATION] =
                             setting_set_device_orientation,
    [CAMERA_PARAM_GET_DEVICE_ORIENTATION] =
                             setting_get_device_orientation,
    [CAMERA_PARAM_SPRD_AUTO_3DNR_ENABLED] =
                             setting_set_auto_3dnr,
    [CAMERA_PARAM_SPRD_AFBC_ENABLED] =
                             setting_set_afbc_enable,
    [SETTING_GET_SPRD_AFBC_ENABLED] =
                             setting_get_afbc_enabled,
    [CAMERA_PARAM_AE_MODE] =
                             setting_set_ae_mode,
    [CAMERA_PARAM_EXPOSURE_TIME] =
                             setting_set_exposure_time,
    [CAMERA_PARAM_SPRD_AUTOCHASING_REGION_ENABLE] =
                             setting_set_auto_tracking_enable,
    [SETTING_GET_SPRD_AUTOCHASING_REGION_ENABLE] =
                             setting_get_auto_tracking_enable,
    [SETTING_SET_SPRD_AUTOCHASING_STATUS] =
                             setting_set_auto_tracking_status,
    [SETTING_GET_SPRD_AUTOCHASING_STATUS] =
                             setting_get_auto_tracking_status,
    [CAMERA_PARAM_FACE_ATTRIBUTES_ENABLE] =
                             setting_set_face_attributes_enable,
    [SETTING_GET_SPRD_FACE_ATTRIBUTES_ENABLED] =
                             setting_get_face_attributes_enable,
    [CAMERA_PARAM_SPRD_LOGO_WATERMARK_ENABLED] =
                             setting_set_logo_watermark,
    [CAMERA_PARAM_SPRD_TIME_WATERMARK_ENABLED] =
                             setting_set_time_watermark,
    [SETTING_GET_SPRD_LOGO_WATERMARK] =
                             setting_get_logo_watermark,
    [SETTING_GET_SPRD_TIME_WATERMARK] =
                             setting_get_time_watermark,
    [SETTING_CTRL_AE_NOTIFY] =
                             setting_ctrl_ae_adjust,
    [SETTING_CLEAR_AE_NOTIFY] =
                             setting_clear_ae_adjust,
    [CAMERA_PARAM_GET_SENSOR_ORIENTATION] =
                             setting_get_sensor_orientation,
    [SETTING_SET_ORIGINAL_PICTURE_SIZE] =
                             setting_set_original_picture_size,
    [SETTING_GET_ORIGINAL_PICTURE_SIZE] =
                             setting_get_original_picture_size,
    [SETTING_GET_FDR] = setting_get_fdr,
    [CAMERA_PARAM_SPRD_ENABLE_POSTEE] = setting_set_ee,
    [SETTING_GET_EE] = setting_get_ee,
    [CAMERA_PARAM_SPRD_SUPER_MACROPHOTO_ENABLE] =
                             set_super_macrophoto,
    [CAMERA_PARAM_SPRD_SUPER_MACROPHOTO_PARAM] =
                             get_super_macrophoto,
    [CAMERA_PARAM_SMILE_CAPTURE_ENABLE] =
                             setting_set_smile_capture,
    [SETTING_GET_SPRD_SMILE_CAPTURE_ENABLED] =
                             setting_get_smile_capture,
    [SETTING_GET_LAST_PREFLASH_TIME] =
                             setting_get_last_preflash_time,
};

setting_ioctl_fun_ptr cmr_get_cmd_fun_from_table(cmr_uint cmd) {
    if (cmd < SETTING_TYPE_MAX) {
        return setting_list[cmd];
    } else {
        return NULL;
    }
}

cmr_int cmr_setting_init(struct setting_init_in *param_ptr,
                         cmr_handle *out_setting_handle) {
    cmr_int ret = 0;
    struct setting_component *cpt = NULL;
    enum camera_index i = 0;

    if (NULL == param_ptr) {
        return -CMR_CAMERA_INVALID_PARAM;
    }

    cpt = (struct setting_component *)malloc(sizeof(*cpt));
    if (!cpt) {
        CMR_LOGE("malloc failed");
        return -CMR_CAMERA_NO_MEM;
    }
    cmr_bzero(cpt, sizeof(*cpt));

    /*create thread */
    ret = cmr_thread_create(&cpt->thread_handle, SETTING_MSG_QUEUE_SIZE,
                            setting_thread_proc, (void *)cpt, "setting");

    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("create thread failed");
        ret = -CMR_CAMERA_NO_MEM;
        goto setting_out;
    }

    for (i = 0; i < CAMERA_ID_MAX; ++i) {
        memset(&cpt->camera_info[i].hal_param.hal_common, INVALID_SETTING_BYTE,
               sizeof(cpt->camera_info[i].hal_param.hal_common));
    }

    cpt->init_in = *param_ptr;

    pthread_mutex_init(&cpt->status_lock, NULL);
    pthread_mutex_init(&cpt->ctrl_lock, NULL);
    pthread_mutex_init(&cpt->isp_mutex, NULL);

    sem_init(&cpt->isp_sem, 0, 0);
    sem_init(&cpt->quick_ae_sem, 0, 0);
    sem_init(&cpt->preflash_sem, 0, 0);

    *out_setting_handle = (cmr_handle)cpt;
    return 0;

setting_out:
    if (ret) {
        CMR_LOGE("error ret %ld", ret);
        if (cpt) {
            free(cpt);
            cpt = NULL;
        }
    }
    return ret;
}

cmr_int cmr_setting_cancel_notice_flash(cmr_handle setting_handle) {
    cmr_int ret = 0;
    struct setting_component *cpt = (struct setting_component *)setting_handle;
    CMR_LOGD("cmr_setting_cancel_notice_flash");

    pthread_mutex_lock(&cpt->isp_mutex);
    cpt->flash_need_quit = FLASH_NEED_QUIT;
    pthread_mutex_unlock(&cpt->isp_mutex);
    sem_post(&cpt->isp_sem); // fastly quit af process when flash on
    sem_post(&cpt->isp_sem); // fastly quit af process when flash on

    return ret;
}

cmr_int cmr_pre_flash_notice_flash(cmr_handle setting_handle) {
    cmr_int ret = 0;
    struct setting_component *cpt = (struct setting_component *)setting_handle;
    CMR_LOGD("cmr_pre_flash_notice_flash");

    if (cpt == NULL) {
        CMR_LOGE("cpt is null");
        return ret;
    }

    pthread_mutex_lock(&cpt->isp_mutex);
    cpt->flash_need_quit = FLASH_OPEN;
    pthread_mutex_unlock(&cpt->isp_mutex);

    return ret;
}

cmr_int cmr_setting_deinit(cmr_handle setting_handle) {
    cmr_int ret = 0;
    struct setting_component *cpt = (struct setting_component *)setting_handle;
    CMR_MSG_INIT(message);

    if (NULL == cpt) {
        return -CMR_CAMERA_INVALID_PARAM;
    }

    if (cpt->thread_handle) {
        ret = cmr_thread_destroy(cpt->thread_handle);
        if (!ret) {
            cpt->thread_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy thr %ld", ret);
        }
    }

    sem_destroy(&cpt->isp_sem);
    sem_destroy(&cpt->quick_ae_sem);
    sem_destroy(&cpt->preflash_sem);

    pthread_mutex_destroy(&cpt->status_lock);
    pthread_mutex_destroy(&cpt->ctrl_lock);
    pthread_mutex_destroy(&cpt->isp_mutex);

    free(cpt);
    cpt = NULL;
deinit_out:
    return ret;
}

cmr_int cmr_setting_ioctl(cmr_handle setting_handle, cmr_uint cmd_type,
                          struct setting_cmd_parameter *parm) {
    cmr_int ret = 0;
    struct setting_component *cpt = (struct setting_component *)setting_handle;

    if (!cpt || !parm || cmd_type >= SETTING_TYPE_MAX) {
        CMR_LOGE("param has error cpt %p, parm %p, array_size %zu, "
                 "cmd_type %ld",
                 cpt, parm, cmr_array_size(setting_list), cmd_type);
        return -CMR_CAMERA_INVALID_PARAM;
    }

    setting_ioctl_fun_ptr fun_ptr = cmr_get_cmd_fun_from_table(cmd_type);
    if (fun_ptr) {
        ret = (*fun_ptr)(cpt, parm);
    } else {
        CMR_LOGW("ioctl is NULL  %ld", cmd_type);
    }
    return ret;
}

int camera_set_flashdevice(uint32_t param) {
    cmr_int ret = 0;
    UNUSED(param);

    return ret;
}

cmr_int camera_get_flashled_flag(cmr_int param) {
    cmr_int ret = 0;
    cmr_int led0 = 0;
    cmr_int led1 = 0;
    UNUSED(param);
#ifdef CONFIG_CAMERA_FLASH_LED_0
    led0 = ISP_FLASH_LED_0;
#endif
#ifdef CONFIG_CAMERA_FLASH_LED_1
    led1 = ISP_FLASH_LED_1;
#endif
    ret = led0 | led1;
    return ret;
}
