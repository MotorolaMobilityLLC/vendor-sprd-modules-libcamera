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
#define LOG_TAG "sns_drv_u"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/trace.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/sockets.h>
#include <fcntl.h>
#include "dlfcn.h"
#include "sensor_cfg.h"
#include "sensor_drv_u.h"
#include "../otp_cali/otp_cali.h"
#include "ams/tcs3430/tcs_3430_drv.h"
#include "sensor_hub/color_temp/sensorhub_drv.h"

#define SENSOR_CTRL_MSG_QUEUE_SIZE 10
#define WRITE_DUAL_OTP_SIZE 230

static pthread_mutex_t cali_otp_mutex;

static char front_cam_name[64] = "Unknown";
static char frontaux_cam_name[64] = "Unknown";
static char back_cam_name[64] = "Unknown";
static char backaux_cam_name[64] = "Unknown";
static char backaux2_cam_name[64] = "Unknown";

static char front_cam_efuse[64] = "Unknown";
static char frontaux_cam_efuse[64] = "Unknown";
static char back_cam_efuse[64] = "Unknown";
static char backaux_cam_efuse[64] = "Unknown";
static char backaux2_cam_efuse[64] = "Unknown";

#define SENSOR_CTRL_EVT_BASE (CMR_EVT_SENSOR_BASE + 0x800)
#define SENSOR_CTRL_EVT_INIT (SENSOR_CTRL_EVT_BASE + 0x0)
#define SENSOR_CTRL_EVT_EXIT (SENSOR_CTRL_EVT_BASE + 0x1)
#define SENSOR_CTRL_EVT_SETMODE (SENSOR_CTRL_EVT_BASE + 0x2)
#define SENSOR_CTRL_EVT_SETMODONE (SENSOR_CTRL_EVT_BASE + 0x3)
#define SENSOR_CTRL_EVT_CFGOTP (SENSOR_CTRL_EVT_BASE + 0x4)
#define SENSOR_CTRL_EVT_STREAM_CTRL (SENSOR_CTRL_EVT_BASE + 0x5)

#define LOGICAL_SENSOR_ID_MAX 16
#define SPRD_AWB_OTP_SIZE 128
#define SPRD_AF_OTP_SIZE 128
/**---------------------------------------------------------------------------*
 **                         Local Variables                                   *
 **---------------------------------------------------------------------------*/
static struct sensor_drv_context sns_drv_cntx[SENSOR_ID_MAX];
static struct slotSensorInfo slot_sensor_info_list[SENSOR_ID_MAX] = {0};
static struct phySensorInfo phy_sensor_info_list[SENSOR_ID_MAX] = {0};
static struct logicalSensorInfo
    logical_sensor_info_list[LOGICAL_SENSOR_ID_MAX] = {0};
static struct logicalCameraInfo
    logical_camera_info_list[LOGICAL_SENSOR_ID_MAX] = {0};
static struct camera_device_manager camera_dev_manger = {0};
static struct sensor_drv_lib sensor_lib_mngr[SENSOR_ID_MAX] = {0};
static struct otp_drv_lib otp_lib_mngr[SENSOR_ID_MAX] = {0};
static struct vcm_drv_lib vcm_lib_mngr[SENSOR_ID_MAX] = {0};
static struct tuning_param_lib tuning_lib_mngr[SENSOR_ID_MAX] = {0};

static SENSOR_MATCH_T sensor_cfg_tab[SENSOR_ID_MAX] = {0};
static struct xml_camera_cfg_info xml_cfg_tab[SENSOR_ID_MAX] = {0};

static cmr_u32 sensor_is_HD_mode = 0;
static cmr_u8 otpdata[SENSOR_ID_MAX][SPRD_DUAL_OTP_SIZE] = {0};
static cmr_u8 otp_awb_data[SENSOR_ID_MAX][SPRD_AWB_OTP_SIZE] = {0};
static cmr_u8 otp_af_data[SENSOR_ID_MAX][SPRD_AF_OTP_SIZE] = {0};

/**---------------------------------------------------------------------------*
 **                         Local Functions                                   *
 **---------------------------------------------------------------------------*/
static cmr_int sensor_drv_load_library(const char *name,
                                       struct sensor_drv_lib *libPtr);
static cmr_int sensor_drv_otp_load_library(const char *name,
                                           struct otp_drv_lib *libPtr);
static cmr_int sensor_drv_vcm_load_library(const char *name,
                                           struct vcm_drv_lib *libPtr);
static cmr_int sensor_drv_unload_library(struct sensor_drv_lib *libPtr);
static cmr_int sensor_drv_otp_unload_library(struct otp_drv_lib *libPtr);

static cmr_int sensor_drv_vcm_unload_library(struct vcm_drv_lib *libPtr);
static cmr_int sensor_drv_tuning_load_library(const char *name,
                                              struct tuning_param_lib *libPtr);
static cmr_int
sensor_drv_tuning_load_default_library(param_input_t *param_input_ptr,
                                       const char *name,
                                       struct tuning_param_lib *libPtr);
static cmr_int
sensor_drv_tuning_unload_library(struct tuning_param_lib *libPtr);
static cmr_int
sensor_drv_store_version_info(struct sensor_drv_context *sensor_cxt,
                              char *sensor_info);
static cmr_int
sensor_drv_get_dynamic_info(struct sensor_drv_context *sensor_cxt);
static cmr_int
sensor_drv_get_module_otp_data(struct sensor_drv_context *sensor_cxt);
static cmr_int
sensor_drv_get_tuning_param(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_drv_get_fov_info(struct sensor_drv_context *sensor_cxt);
static cmr_int
sensor_drv_get_sensor_type(struct sensor_drv_context *sensor_cxt);

static cmr_int sensor_drv_ic_identify(struct sensor_drv_context *sensor_cxt,
                                      cmr_u32 sensor_id, cmr_u32 identify_off);
static cmr_int sensor_drv_identify(struct sensor_drv_context *sensor_cxt,
                                   cmr_u32 sensor_id);
static cmr_int sensor_drv_open(struct sensor_drv_context *sensor_cxt,
                               cmr_u32 sensor_id);
static cmr_int sensor_drv_scan_hw(void);

static cmr_int sensor_ctrl_thread_proc(struct cmr_msg *message, void *p_data);

static cmr_int sensor_set_mode_msg(struct sensor_drv_context *sensor_cxt,
                                   cmr_u32 mode, cmr_u32 is_inited);
static cmr_int sensor_set_mode(struct sensor_drv_context *sensor_cxt,
                               cmr_u32 mode, cmr_u32 is_inited);

static cmr_int sensor_stream_off(struct sensor_drv_context *sensor_cxt);

static cmr_int sensor_stream_ctrl(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 on_off);

static cmr_int sensor_init_defaul_exif(struct sensor_drv_context *sensor_cxt);

static void sensor_rid_save_sensor_info(char *sensor_info, cmr_u32 slot_id);

static cmr_int sensor_hw_read_i2c(cmr_handle sns_module_handle,
                                  cmr_u16 slave_addr, cmr_u8 *cmd,
                                  cmr_u16 cmd_length);

static cmr_int sensor_hw_write_i2c(cmr_handle sns_module_handle,
                                   cmr_u16 slave_addr, cmr_u8 *cmd,
                                   cmr_u16 cmd_length);

static cmr_int sensor_muti_i2c_write(cmr_handle sns_module_handle, void *param);

static cmr_int sensor_ic_create(struct sensor_drv_context *sensor_cxt,
                                cmr_u32 sensor_id);

static cmr_int sensor_ic_delete(struct sensor_drv_context *sensor_cxt);

static void *sensor_ic_get_data(struct sensor_drv_context *sensor_cxt,
                                cmr_uint cmd);

static cmr_int sensor_ic_ex_write_exposure(cmr_handle handle, cmr_uint param);

static cmr_int sensor_ic_write_ae_value(cmr_handle handle, cmr_u32 param);

static cmr_int sensor_ic_write_gain(cmr_handle handle, cmr_u32 param);

static cmr_int sensor_ic_read_aec_info(cmr_handle handle, void *param);

static cmr_int sensor_af_init(cmr_handle sns_module_handle);

static cmr_int sensor_af_deinit(cmr_handle sns_module_handle);

static cmr_int sensor_af_set_pos(cmr_handle sns_module_handle, cmr_u32 pos);

static cmr_int sensor_af_get_pos(cmr_handle sns_module_handle, cmr_u16 *pos);

static cmr_int sensor_af_get_pos_info(cmr_handle sns_module_handle,
                                      struct sensor_vcm_info *info);

static cmr_int sensor_otp_module_init(struct sensor_drv_context *sensor_cxt);

static cmr_int sensor_otp_module_deinit(struct sensor_drv_context *sensor_cxt);

static cmr_int sensor_otp_process(struct sensor_drv_context *sensor_cxt,
                                  uint8_t cmd, uint8_t sub_cmd, void *data);
extern uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,
                                              SENSOR_ID_E sensor_id);

LOCAL cmr_int sensor_otp_rw_ctrl(struct sensor_drv_context *sensor_cxt,
                                 uint8_t cmd, uint8_t sub_cmd, void *data);

extern uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,
                                              SENSOR_ID_E sensor_id);
cmr_int sensor_set_raw_infor(struct sensor_drv_context *sensor_cxt,
                             cmr_u8 vendor_id);
cmr_int sensor_set_otp_data(struct sensor_drv_context *sensor_cxt);

cmr_int get_sensor_pid(cmr_u8 *buf);

void sensor_set_cxt_common(struct sensor_drv_context *sensor_cxt) {
    SENSOR_ID_E slot_id = 0;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    slot_id = sensor_cxt->slot_id;
#ifndef CONFIG_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT
    sns_drv_cntx[0] = *(struct sensor_drv_context *)sensor_cxt;
#else
    if (slot_id < SENSOR_ID_MAX)
        sns_drv_cntx[slot_id] = *sensor_cxt;
#endif
    return;
}

void *sensor_get_dev_cxt(void) { return (void *)&sns_drv_cntx[0]; }

void *sensor_get_dev_cxt_Ex(cmr_u32 slot_id) {
    struct sensor_drv_context *sensor_cxt = NULL;

#ifndef CONFIG_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT
    sensor_cxt = &sns_drv_cntx[0];
#else
    if (slot_id < SENSOR_ID_MAX)
        sensor_cxt = &sns_drv_cntx[slot_id];
#endif
    return (void *)sensor_cxt;
}

static cmr_int sensor_write_pdaf_info(struct sensor_pdaf_info phasePixelMap,
                                      char *name) {
    FILE *fp = NULL;
    char pd_file_path[128];
    PhasePixel_MAP pixel_info_saved;
    memset(&pd_file_path, 0, sizeof(pd_file_path));
    memset(&pixel_info_saved, 0, sizeof(PhasePixel_MAP));
    pixel_info_saved.count = 2 * phasePixelMap.pd_pos_size;
    pixel_info_saved.block_start_col = phasePixelMap.pd_offset_x;
    pixel_info_saved.block_start_row = phasePixelMap.pd_offset_y;
    pixel_info_saved.block_end_col = phasePixelMap.pd_end_x;
    pixel_info_saved.block_end_row = phasePixelMap.pd_end_y;
    pixel_info_saved.block_width = phasePixelMap.pd_block_w;
    pixel_info_saved.block_height = phasePixelMap.pd_block_h;
    SENSOR_LOGV("number is %u", pixel_info_saved.count);
    SENSOR_LOGV("block_start_col is %u -->%u", pixel_info_saved.block_start_col,
                pixel_info_saved.block_start_row);
    SENSOR_LOGV("block_width is %u -->%u", pixel_info_saved.block_width,
                pixel_info_saved.block_height);
    for (int i = 0; i < pixel_info_saved.count; i++) {
        pixel_info_saved.pixel[i].rx = phasePixelMap.pd_pos_col[i];
        pixel_info_saved.pixel[i].ry = phasePixelMap.pd_pos_row[i];
        pixel_info_saved.pixel[i].phase_pos = phasePixelMap.pd_is_right[i];
        SENSOR_LOGV("pixel coordinate is [%u, %u, %u]",
                    pixel_info_saved.pixel[i].rx, pixel_info_saved.pixel[i].ry,
                    pixel_info_saved.pixel[i].phase_pos);
    }
    SENSOR_LOGV("finish prepare pd file with nam %s", name);
    sprintf(pd_file_path, "%s%s.bin", "/data/vendor/cameraserver/pd_", name);
    fp = fopen(pd_file_path, "wb+");
    if (!fp) {
        SENSOR_LOGE("Cannot open file");
        goto exit;
    } else {
        fwrite(&pixel_info_saved, sizeof(PhasePixel_MAP), 1, fp);
        fclose(fp);
    }
    SENSOR_LOGV("finish saving file");
    return SENSOR_SUCCESS;
exit:
    return SENSOR_FAIL;
}

static cmr_int sensor_save_pdaf_info(struct sensor_drv_context *sensor_cxt) {
    SENSOR_VAL_T val;
    char *name_of_sensor = NULL;
    struct sensor_pdaf_info phasePixelMap;
    memset(&phasePixelMap, 0, sizeof(struct sensor_pdaf_info));
    char property_value[PROPERTY_VALUE_MAX];
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    struct sensor_ic_ops *sns_ops = PNULL;
    cmr_int ret = SENSOR_SUCCESS;
    memset(&property_value, 0, sizeof(property_value));
    property_get("persist.vendor.cam.sensor.store.pdaf.file", property_value,
                 "0");
    SENSOR_LOGV("property_value is %s", property_value);
    if (atoi(property_value) && sensor_cxt && sensor_cxt->static_info &&
        sensor_cxt->static_info->pdaf_supported) {
        SENSOR_LOGV("support pdaf info is %u",
                    sensor_cxt->static_info->pdaf_supported);
        val.type = SENSOR_VAL_TYPE_GET_PDAF_INFO;
        val.pval = &phasePixelMap;
        sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
        if (sns_ops &&
            !sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                           (cmr_u32)&val)) {
            name_of_sensor = sensor_cxt->xml_info->cfgPtr->sensor_name;
            if (sensor_write_pdaf_info(phasePixelMap, name_of_sensor)) {
                goto exit;
            }
        } else
            SENSOR_LOGE("unvalid param");
    } else
        goto exit;
    return ret;
exit:
    SENSOR_LOGE("cannot save file");
    return SENSOR_FAIL;
}

cmr_int sensor_get_flash_level(struct sensor_drv_context *sensor_cxt,
                               struct sensor_flash_level *level) {
    int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    ret = hw_sensor_get_flash_level(sensor_cxt->hw_drv_handle, level);
    if (0 != ret) {
        SENSOR_LOGE("_Sensor_Device_GetFlashLevel failed, ret=%d \n", ret);
        ret = -1;
    }

    return ret;
}

cmr_int sensor_set_color_temp(cmr_handle handle, void* callback) {
    if(callback == NULL) {
        return 1;
    } else {
        color_temp_callback_func = callback;
    }

    return 0;
}

void sensor_power_on(struct sensor_drv_context *sensor_cxt, cmr_u32 power_on) {
    ATRACE_BEGIN(__FUNCTION__);

    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt->sensor_info_ptr);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

    SENSOR_LOGI("E:power_on = %d", power_on);
    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);
    if (sns_ops && sns_ops->power) {
        sns_ops->power(sensor_cxt->sns_ic_drv_handle, power_on);
    }

    ATRACE_END();
}

void sensor_set_export_Info(struct sensor_drv_context *sensor_cxt) {
    SENSOR_VIDEO_INFO_T *video_info_ptr = PNULL;
    cmr_u32 i = 0;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt->sensor_info_ptr);

    SENSOR_EXP_INFO_T *exp_info_ptr = &sensor_cxt->sensor_exp_info;

    /*get current sensor common configure information*/
    SENSOR_INFO_T *sns_info = sensor_cxt->sensor_info_ptr;
    struct sensor_ic_ops *sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    /*get curent sensor module configure information*/
    SENSOR_REG_TAB_INFO_T *res_info_ptr =
        sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_RESOLUTION);
    struct sensor_trim_tag *res_trim_ptr =
        sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_TRIM_TAB);
    struct module_cfg_info *mod_cfg_info =
        sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_MODULE_CFG);

    if (!res_info_ptr || !res_trim_ptr || !mod_cfg_info) {
        SENSOR_LOGE("res_info_ptr, res_trim_ptr or mod_cfg_info is NULL");
        goto exit;
    }

    SENSOR_MEMSET(exp_info_ptr, 0x00, sizeof(SENSOR_EXP_INFO_T));
    exp_info_ptr->name = sns_info->name;
    exp_info_ptr->image_format = sns_info->image_format;
    exp_info_ptr->image_pattern = mod_cfg_info->image_pattern;

    /*the high 3bit will be the phase(delay sel)*/
    exp_info_ptr->pclk_polarity = (sns_info->hw_signal_polarity & 0x01);
    exp_info_ptr->vsync_polarity = ((sns_info->hw_signal_polarity >> 2) & 0x1);
    exp_info_ptr->hsync_polarity = ((sns_info->hw_signal_polarity >> 4) & 0x1);
    exp_info_ptr->pclk_delay = ((sns_info->hw_signal_polarity >> 5) & 0x07);

    if (NULL != sns_info->raw_info_ptr) {
        exp_info_ptr->raw_info_ptr =
            (struct sensor_raw_info *)*sns_info->raw_info_ptr;
    }

    if ((NULL != exp_info_ptr->raw_info_ptr) &&
        (NULL != exp_info_ptr->raw_info_ptr->resolution_info_ptr)) {
        exp_info_ptr->raw_info_ptr->resolution_info_ptr->image_pattern =
            mod_cfg_info->image_pattern;
    }

    exp_info_ptr->source_width_max = sns_info->source_width_max;
    exp_info_ptr->source_height_max = sns_info->source_height_max;

    exp_info_ptr->environment_mode = sns_info->environment_mode;
    exp_info_ptr->image_effect = sns_info->image_effect;
    exp_info_ptr->wb_mode = sns_info->wb_mode;
    exp_info_ptr->step_count = sns_info->step_count;
    exp_info_ptr->ext_info_ptr = sns_info->ext_info_ptr;

    exp_info_ptr->preview_skip_num = mod_cfg_info->preview_skip_num;
    exp_info_ptr->capture_skip_num = mod_cfg_info->capture_skip_num;
    exp_info_ptr->flash_capture_skip_num = mod_cfg_info->flash_capture_skip_num;
    exp_info_ptr->mipi_cap_skip_num = mod_cfg_info->mipi_cap_skip_num;
    exp_info_ptr->preview_deci_num = mod_cfg_info->preview_deci_num;
    exp_info_ptr->change_setting_skip_num =
        mod_cfg_info->change_setting_skip_num;
    exp_info_ptr->video_preview_deci_num = mod_cfg_info->video_preview_deci_num;
    exp_info_ptr->threshold_eb = mod_cfg_info->threshold_eb;
    exp_info_ptr->threshold_mode = mod_cfg_info->threshold_mode;
    exp_info_ptr->threshold_start = mod_cfg_info->threshold_start;
    exp_info_ptr->threshold_end = mod_cfg_info->threshold_end;

    exp_info_ptr->sns_ops = sns_info->sns_ops;

    for (i = SENSOR_MODE_COMMON_INIT; i < SENSOR_MODE_MAX; i++) {
        if ((PNULL != res_info_ptr[i].sensor_reg_tab_ptr) ||
            ((0x00 != res_info_ptr[i].width) &&
             (0x00 != res_info_ptr[i].width))) {
            if (SENSOR_IMAGE_FORMAT_JPEG == res_info_ptr[i].image_format) {
                exp_info_ptr->sensor_image_type = SENSOR_IMAGE_FORMAT_JPEG;
            }
            exp_info_ptr->sensor_mode_info[i].mode = i;
            exp_info_ptr->sensor_mode_info[i].width = res_info_ptr[i].width;
            exp_info_ptr->sensor_mode_info[i].height = res_info_ptr[i].height;
            if ((PNULL != res_trim_ptr) &&
                (0x00 != res_trim_ptr[i].trim_width) &&
                (0x00 != res_trim_ptr[i].trim_height)) {
                exp_info_ptr->sensor_mode_info[i].trim_start_x =
                    res_trim_ptr[i].trim_start_x;
                exp_info_ptr->sensor_mode_info[i].trim_start_y =
                    res_trim_ptr[i].trim_start_y;
                exp_info_ptr->sensor_mode_info[i].trim_width =
                    res_trim_ptr[i].trim_width;
                exp_info_ptr->sensor_mode_info[i].trim_height =
                    res_trim_ptr[i].trim_height;
                exp_info_ptr->sensor_mode_info[i].line_time =
                    res_trim_ptr[i].line_time;
                exp_info_ptr->sensor_mode_info[i].bps_per_lane =
                    res_trim_ptr[i].bps_per_lane;
                exp_info_ptr->sensor_mode_info[i].frame_line =
                    res_trim_ptr[i].frame_line;
            } else {
                exp_info_ptr->sensor_mode_info[i].trim_start_x = 0x00;
                exp_info_ptr->sensor_mode_info[i].trim_start_y = 0x00;
                exp_info_ptr->sensor_mode_info[i].trim_width =
                    res_info_ptr[i].width;
                exp_info_ptr->sensor_mode_info[i].trim_height =
                    res_info_ptr[i].height;
            }

            /*scaler trim*/
            if ((PNULL != res_trim_ptr) &&
                (0x00 != res_trim_ptr[i].scaler_trim.w) &&
                (0x00 != res_trim_ptr[i].scaler_trim.h)) {
                exp_info_ptr->sensor_mode_info[i].scaler_trim =
                    res_trim_ptr[i].scaler_trim;
            } else {
                exp_info_ptr->sensor_mode_info[i].scaler_trim.x = 0x00;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.y = 0x00;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.w =
                    exp_info_ptr->sensor_mode_info[i].trim_width;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.h =
                    exp_info_ptr->sensor_mode_info[i].trim_height;
            }

            if (SENSOR_IMAGE_FORMAT_MAX != sns_info->image_format) {
                exp_info_ptr->sensor_mode_info[i].image_format =
                    sns_info->image_format;
            } else {
                exp_info_ptr->sensor_mode_info[i].image_format =
                    res_info_ptr[i].image_format;
            }
        } else {
            exp_info_ptr->sensor_mode_info[i].mode = SENSOR_MODE_MAX;
        }

        if (PNULL != sns_info->video_tab_info_ptr) {
            video_info_ptr = &sns_info->video_tab_info_ptr[i];
            if (PNULL != video_info_ptr) {
                cmr_copy((void *)&exp_info_ptr->sensor_video_info[i],
                         (void *)video_info_ptr, sizeof(SENSOR_VIDEO_INFO_T));
            }
        }

        if ((NULL != exp_info_ptr->raw_info_ptr) &&
            (NULL != exp_info_ptr->raw_info_ptr->resolution_info_ptr)) {
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].start_x =
                exp_info_ptr->sensor_mode_info[i].trim_start_x;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].start_y =
                exp_info_ptr->sensor_mode_info[i].trim_start_y;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].width =
                exp_info_ptr->sensor_mode_info[i].trim_width;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].height =
                exp_info_ptr->sensor_mode_info[i].trim_height;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].line_time =
                exp_info_ptr->sensor_mode_info[i].line_time;
            exp_info_ptr->raw_info_ptr->resolution_info_ptr->tab[i].frame_line =
                exp_info_ptr->sensor_mode_info[i].frame_line;
        }

        if ((NULL != exp_info_ptr->raw_info_ptr) &&
            (NULL != exp_info_ptr->raw_info_ptr->ioctrl_ptr)) {
            struct sensor_raw_ioctrl *ioctl =
                exp_info_ptr->raw_info_ptr->ioctrl_ptr;
            ioctl->caller_handler = sensor_cxt;
            ioctl->set_focus = sensor_af_set_pos;
            ioctl->get_pos = sensor_af_get_pos_info;
            ioctl->set_exposure = sensor_ic_write_ae_value;
            ioctl->set_gain = sensor_ic_write_gain;
            ioctl->ext_fuc = NULL;
#if 1
            if (module && module->af_dev_info.af_drv_entry) {
                ioctl->set_pos = sensor_af_set_pos;
                ioctl->get_otp = NULL;
                ioctl->get_motor_pos = sensor_af_get_pos;
                ioctl->set_motor_bestmode = NULL;
                ioctl->get_test_vcm_mode = NULL;
                ioctl->set_test_vcm_mode = NULL;
            } else {
                ioctl->set_pos = NULL;
                ioctl->get_motor_pos = NULL;
                SENSOR_LOGI("af_drv_entry not configured:module:%p,af_dev:%p",
                            module,
                            module ? module->af_dev_info.af_drv_entry : NULL);
            }
#endif
#ifdef SBS_MODE_SENSOR
            char value1[PROPERTY_VALUE_MAX];
            property_get("persist.vendor.cam.sbs.mode", value1, "0");
            if (!strcmp(value1, "slave")) {
                ioctl->set_pos = NULL;
                ioctl->get_motor_pos = NULL;
                ioctl->set_focus = NULL;
            }
#endif
            ioctl->write_i2c = sensor_hw_write_i2c;
            // exp_info_ptr->raw_info_ptr->ioctrl_ptr->read_i2c =
            // Sensor_ReadI2C;
            ioctl->ex_set_exposure = sensor_ic_ex_write_exposure;
            ioctl->read_aec_info = sensor_ic_read_aec_info;
            ioctl->write_aec_info = sensor_muti_i2c_write;
            ioctl->sns_ioctl = sensor_drv_ioctl;
        }
        // now we think sensor output width and height are equal to sensor
        // trim_width and trim_height
        exp_info_ptr->sensor_mode_info[i].out_width =
            exp_info_ptr->sensor_mode_info[i].trim_width;
        exp_info_ptr->sensor_mode_info[i].out_height =
            exp_info_ptr->sensor_mode_info[i].trim_height;
    }

    exp_info_ptr->sensor_interface = mod_cfg_info->sensor_interface;
    exp_info_ptr->change_setting_skip_num =
        mod_cfg_info->change_setting_skip_num;
    exp_info_ptr->horizontal_view_angle = mod_cfg_info->horizontal_view_angle;
    exp_info_ptr->vertical_view_angle = mod_cfg_info->vertical_view_angle;
    exp_info_ptr->sensor_version_info = sns_info->sensor_version_info;

exit:
    SENSOR_LOGI("X");
}

LOCAL void sensor_clean_info(struct sensor_drv_context *sensor_cxt) {

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_cxt->sensor_mode = SENSOR_MODE_MAX;
    sensor_cxt->sensor_info_ptr = PNULL;
    sensor_cxt->sensor_isInit = SENSOR_FALSE;
    sensor_cxt->sensor_index = NULL;
    SENSOR_MEMSET(&sensor_cxt->sensor_exp_info, 0x00,
                  sizeof(SENSOR_EXP_INFO_T));
    sensor_cxt->slot_id = SENSOR_ID_MAX;
    return;
}

LOCAL cmr_int sensor_set_id(struct sensor_drv_context *sensor_cxt,
                            SENSOR_ID_E sensor_id) {

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_cxt->slot_id = sensor_id;
    SENSOR_LOGV("id %d", sensor_id);
    return SENSOR_SUCCESS;
}

LOCAL SENSOR_ID_E sensor_get_cur_id(struct sensor_drv_context *sensor_cxt) {

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGV("id %d", sensor_cxt->slot_id);
    return (SENSOR_ID_E)sensor_cxt->slot_id;
}

LOCAL void sensor_i2c_init(struct sensor_drv_context *sensor_cxt,
                           SENSOR_ID_E sensor_id) {
    cmr_int ret = SENSOR_FAIL;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    if (sensor_id >= SENSOR_ID_MAX) {
        SENSOR_LOGI("invalid id=%d", sensor_id);
        goto exit;
    }

    ret = hw_sensor_i2c_init(sensor_cxt->hw_drv_handle, sensor_id);
    SENSOR_LOGI("i2c init %s", ret ? "failed" : "ok");
    sensor_cxt->i2c_init_ok = ret ? 0 : 1;

exit:
    return;
}

LOCAL cmr_int sensor_i2c_deinit(struct sensor_drv_context *sensor_cxt,
                                SENSOR_ID_E sensor_id) {
    cmr_int ret = SENSOR_FAIL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (sensor_id >= SENSOR_ID_MAX) {
        SENSOR_LOGI("invalid id=%d", sensor_id);
        goto exit;
    }

    if (!sensor_cxt->i2c_init_ok) {
        SENSOR_LOGI("i2c init failed");
        goto exit;
    }

    ret = hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, sensor_id);
    SENSOR_LOGI("i2c deinit %s", ret ? "failed" : "ok");
    sensor_cxt->i2c_init_ok = 0;

exit:
    return ret;
}

static cmr_int sensor_get_module_cfg_info(struct sensor_drv_context *sensor_cxt,
                                          cmr_u32 sensor_id,
                                          struct module_cfg_info **cfg_info) {
    cmr_int ret = SENSOR_FAIL;
    cmr_u32 i = 0, tab_size = 0;
    SENSOR_MATCH_T *module_info = NULL;
    struct sensor_module_info *mod_cfg_tab = NULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    tab_size = sensor_cxt->sensor_info_ptr->module_info_tab_size;
    mod_cfg_tab = sensor_cxt->sensor_info_ptr->module_info_tab;
    module_info = sensor_cxt->current_module;
    SENSOR_LOGI("p1:%p,p2:%p", module_info, mod_cfg_tab);

    if (mod_cfg_tab && module_info) {
        SENSOR_LOGI("tab_size:%d,%ld,%ld", tab_size, sizeof(mod_cfg_tab),
                    sizeof(mod_cfg_tab[0]));
        // tab_size = 3;
        for (i = 0; i < tab_size; i++) {
            *cfg_info = &mod_cfg_tab[i].module_info;
            SENSOR_LOGI("mid1:%d,mid2:%d,index:%d", module_info->module_id,
                        mod_cfg_tab[i].module_id, i);
            if (mod_cfg_tab[i].module_id == module_info->module_id) {
                return SENSOR_SUCCESS;
            }
        }
    } else {
        SENSOR_LOGE("ERROR:module info or configure info is NULL\n");
    }

    return ret;
}

cmr_int Sensor_set_calibration(cmr_u32 value) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sensor_get_dev_cxt();
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_cxt->is_calibration = value;
    return SENSOR_SUCCESS;
}

void _sensor_calil_lnc_param_recover(struct sensor_drv_context *sensor_cxt,
                                     SENSOR_INFO_T *sensor_info_ptr) {}

cmr_int _sensor_cali_lnc_param_update(struct sensor_drv_context *sensor_cxt,
                                      cmr_s8 *cfg_file_dir,
                                      SENSOR_INFO_T *sensor_info_ptr,
                                      SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);
    cmr_u32 rtn = SENSOR_SUCCESS;
    return rtn;
}

cmr_int _sensor_cali_awb_param_update(cmr_s8 *cfg_file_dir,
                                      SENSOR_INFO_T *sensor_info_ptr,
                                      SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);

    cmr_int rtn = 0;
    return rtn;
}

cmr_int _sensor_cali_flashlight_param_update(cmr_s8 *cfg_file_dir,
                                             SENSOR_INFO_T *sensor_info_ptr,
                                             SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);

    cmr_int rtn = 0;
    return rtn;
}

cmr_int _sensor_cali_load_param(struct sensor_drv_context *sensor_cxt,
                                cmr_s8 *cfg_file_dir,
                                SENSOR_INFO_T *sensor_info_ptr,
                                SENSOR_ID_E sensor_id) {
    return SENSOR_SUCCESS;
}

LOCAL cmr_int sensor_create_ctrl_thread(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    SENSOR_LOGI("is_inited %ld", sensor_cxt->ctrl_thread_cxt.is_inited);

    if (!sensor_cxt->ctrl_thread_cxt.is_inited) {
        pthread_mutex_init(&sensor_cxt->ctrl_thread_cxt.sensor_mutex, NULL);
        sem_init(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem, 0, 0);

        ret = cmr_thread_create(&sensor_cxt->ctrl_thread_cxt.thread_handle,
                                 SENSOR_CTRL_MSG_QUEUE_SIZE,
                                 sensor_ctrl_thread_proc, (void *)sensor_cxt,
                                 "sn_ctrl");
        if (ret) {
            SENSOR_LOGE("send msg failed!");
            ret = SENSOR_FAIL;
            goto end;
        }

        sensor_cxt->ctrl_thread_cxt.is_inited = 1;
    }

end:
    if (ret) {
        sem_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem);
        pthread_mutex_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_mutex);
        sensor_cxt->ctrl_thread_cxt.is_inited = 0;
    }

    SENSOR_LOGV("ret %ld", ret);
    return ret;
}

LOCAL cmr_int sensor_ctrl_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_u32 evt = 0, on_off = 0;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)p_data;
    otp_ctrl_cmd_t *otp_ctrl_data = NULL;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;
    SENSOR_LOGI("evt %d", evt);

    switch (evt) {
    case SENSOR_CTRL_EVT_INIT:
        /*common control info config*/
        SENSOR_LOGI("INIT DONE!");
        break;

    case SENSOR_CTRL_EVT_SETMODE: {
        cmr_u32 mode = (cmr_u32)message->sub_msg_type;
        cmr_u32 is_inited = (cmr_u32)((cmr_uint)message->data);
        usleep(100);
        sensor_set_mode(sensor_cxt, mode, is_inited);
    } break;

    case SENSOR_CTRL_EVT_SETMODONE:
        SENSOR_LOGI("SENSOR_CTRL_EVT_SETMODONE OK");
        break;

    case SENSOR_CTRL_EVT_CFGOTP:
        otp_ctrl_data = (otp_ctrl_cmd_t *)message->data;
        sensor_otp_process(sensor_cxt, otp_ctrl_data->cmd,
                           otp_ctrl_data->sub_cmd, otp_ctrl_data->data);
        break;
    case SENSOR_CTRL_EVT_STREAM_CTRL:
        on_off = (cmr_u32)message->sub_msg_type;
        sensor_stream_ctrl(sensor_cxt, on_off);
        break;

    case SENSOR_CTRL_EVT_EXIT:
        /*common control info clear*/
        SENSOR_LOGI("EXIT DONE!");
        break;

    default:
        SENSOR_LOGE("jpeg:not correct message");
        break;
    }

    return ret;
}

LOCAL cmr_int
sensor_destroy_ctrl_thread(struct sensor_drv_context *sensor_cxt) {
    CMR_MSG_INIT(message);
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    SENSOR_LOGI("is_inited %ld", sensor_cxt->ctrl_thread_cxt.is_inited);

    if (sensor_cxt->ctrl_thread_cxt.is_inited) {
        message.msg_type = SENSOR_CTRL_EVT_EXIT;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                                  &message);
        if (ret) {
            SENSOR_LOGE("send msg failed!");
        }

        ret = cmr_thread_destroy(sensor_cxt->ctrl_thread_cxt.thread_handle);
        sensor_cxt->ctrl_thread_cxt.thread_handle = 0;

        sem_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem);
        pthread_mutex_destroy(&sensor_cxt->ctrl_thread_cxt.sensor_mutex);
        sensor_cxt->ctrl_thread_cxt.is_inited = 0;
    }

    return ret;
}
/**
 *  sensor context init
 **/
cmr_int sensor_context_init(struct sensor_drv_context *sensor_cxt,
                            cmr_u32 sensor_id, cmr_uint is_autotest,
                            cmr_int open_phase) {

    cmr_int ret = SENSOR_SUCCESS;
    cmr_int fd_sensor = SENSOR_FD_INIT;
    struct hw_drv_init_para input_ptr;
    cmr_handle hw_drv_handle = NULL;
    struct camera_device_manager *devPtr = &camera_dev_manger;
    SENSOR_MATCH_T *sns_module = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    cmr_bzero((void *)sensor_cxt, sizeof(struct sensor_drv_context));

    sensor_cxt->fd_sensor = SENSOR_FD_INIT;
    sensor_cxt->i2c_addr = 0xff;
    sensor_cxt->is_autotest = is_autotest;

    sensor_clean_info(sensor_cxt);

    if (open_phase) {
        ret = sensor_create_ctrl_thread(sensor_cxt);
        if (ret) {
            SENSOR_LOGE("Failed to create sensor ctrl thread");
            goto exit;
        }
    }

    input_ptr.sensor_id = sensor_id;
    input_ptr.caller_handle = sensor_cxt;
    fd_sensor = hw_sensor_drv_create(&input_ptr, &hw_drv_handle);
    if ((SENSOR_FD_INIT == fd_sensor) || (NULL == hw_drv_handle)) {
        SENSOR_LOGE("hw sensor drv create error id = %d", sensor_id);
        hw_sensor_drv_delete(hw_drv_handle);
        ret = SENSOR_FAIL;
        goto exit;
    } else {
        sensor_cxt->fd_sensor = fd_sensor;
        sensor_cxt->hw_drv_handle = hw_drv_handle;
        sensor_cxt->sensor_hw_handler = hw_drv_handle;
    }

    if (open_phase) {
        sensor_init_log_level();
        sensor_init_defaul_exif(sensor_cxt);
    }

    sensor_cxt->is_HD_mode = sensor_is_HD_mode;
    sensor_cxt->sensor_index = &devPtr->drv_idx[sensor_id];
    sensor_cxt->xml_info = &xml_cfg_tab[sensor_id];
    sensor_cxt->current_module = &sensor_cfg_tab[sensor_id];
    sns_module = (SENSOR_INFO_T *)sensor_cxt->current_module;
    sensor_cxt->sensor_info_ptr = sns_module->sensor_info;

exit:
    return ret;
}

cmr_int sensor_context_deinit(struct sensor_drv_context *sensor_cxt,
                              cmr_int open_phase) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    if (open_phase) {
        sensor_destroy_ctrl_thread(sensor_cxt);

        sensor_stream_off(sensor_cxt);
        hw_sensor_mipi_deinit(sensor_cxt->hw_drv_handle);
        sensor_i2c_deinit(sensor_cxt, sensor_get_cur_id(sensor_cxt));

        if (SENSOR_TRUE == sensor_cxt->sensor_isInit) {
            if (SENSOR_IMAGE_FORMAT_RAW ==
                sensor_cxt->sensor_info_ptr->image_format) {
                if (0 == sensor_cxt->is_calibration) {
                    _sensor_calil_lnc_param_recover(
                        sensor_cxt, sensor_cxt->sensor_info_ptr);
                }
            }
            sensor_power_on(sensor_cxt, SENSOR_FALSE);
        }
        sensor_ic_delete(sensor_cxt);
    }
    hw_sensor_drv_delete(sensor_cxt->hw_drv_handle);
    sensor_cxt->sensor_hw_handler = NULL;
    sensor_cxt->hw_drv_handle = NULL;
    sensor_cxt->sensor_isInit = SENSOR_FALSE;
    sensor_cxt->sensor_index = NULL;
    sensor_cxt->xml_info = NULL;
    sensor_cxt->current_module = NULL;
    sensor_cxt->sensor_info_ptr = NULL;

    SENSOR_LOGI("X");
    return ret;
}

/**
 * NOTE:when open camera,you should do the following steps.
   1.register all sensor for the following first open sensor.
   2.if first open sensor failed, identify module list to save sensor index.
   3.try open sensor second time.
   4.if 3 steps failed,sensor open failed.
 **/
cmr_int sensor_open_common(struct sensor_drv_context *sensor_cxt,
                           cmr_u32 physical_id, cmr_uint is_autotest) {
    cmr_int ret = SENSOR_FAIL;
    cmr_u32 slot_id = 0;
    cmr_u8 has_identified = 0;
    struct camera_device_manager *devPtr = &camera_dev_manger;
    struct phySensorInfo *phyPtr = phy_sensor_info_list + physical_id;

    char boot_mode[PROPERTY_VALUE_MAX] = {'\0'};
    property_get("ro.bootmode", boot_mode, "0");
    if (!strcmp(boot_mode, "autotest") || !strcmp(boot_mode, "cali")) {
        sensor_drv_scan_hw();
    }

    if (phyPtr->slotId == SENSOR_ID_INVALID) {
        SENSOR_LOGE("slotId is 255.please check and connect the sensor,restart "
                    "the phone");
        return ret;
    }

    slot_id = (cmr_u32)phyPtr->slotId;
    SENSOR_LOGI("open_common physical_id %d,slotId %d,autotest %ld",
                physical_id, phyPtr->slotId, is_autotest);

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    ret = sensor_context_init(sensor_cxt, slot_id, is_autotest, 1);
    if (ret) {
        goto exit;
    }

    ret = sensor_drv_open(sensor_cxt, slot_id);

    if (ret) {
        SENSOR_LOGE("first open sensor failed, restart identify and open");
        if (SENSOR_SUCCESS == sensor_drv_identify(sensor_cxt, slot_id)) {
            ret = sensor_drv_open(sensor_cxt, slot_id);
        }
        if (ret)
            goto exit;
    }

    if (sensor_cxt->sensor_info_ptr &&
        SENSOR_IMAGE_FORMAT_RAW == sensor_cxt->sensor_info_ptr->image_format) {
        if (SENSOR_SUCCESS == ret) {
            ret = _sensor_cali_load_param(sensor_cxt, (cmr_s8 *)CALI_FILE_DIR,
                                          sensor_cxt->sensor_info_ptr, slot_id);
            if (ret) {
                SENSOR_LOGW("load cali data failed!! rtn:%ld", ret);
                goto exit;
            }
        }
    }

exit:

    if (ret) {
        sensor_destroy_ctrl_thread(sensor_cxt);
        hw_sensor_drv_delete(sensor_cxt->hw_drv_handle);
        sensor_cxt->hw_drv_handle = NULL;
        sensor_cxt->sensor_hw_handler = NULL;
    }
    return ret;
}

cmr_int sensor_is_init_common(struct sensor_drv_context *sensor_cxt) {
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    return sensor_cxt->sensor_isInit;
}

cmr_int read_txt_file(const char *file_name, void *data) {
    FILE *pf = fopen(file_name, "r");
    uint32_t read_byte = 0;
    cmr_u8 *otp_data = (cmr_u8 *)data;

    if (NULL == pf) {
        SENSOR_LOGE("dualotp read failed!");
        goto exit;
    }

    if (NULL == otp_data) {
        SENSOR_LOGE("dualotp data malloc failed!");
        fclose(pf);
        goto exit;
    }

    while (!feof(pf)) {
        if (0 > fscanf(pf, "%s\n", otp_data)){
            SENSOR_LOGE("Failed to scanf %s",otp_data);
        }
        otp_data += 4;
        read_byte += 4;
    }
    fclose(pf);
    SENSOR_LOGI("dualotp read_bytes=%d ", read_byte);

exit:
    return read_byte;
}

LOCAL cmr_int sensor_write_dualcam_otpdata(
    struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id) {

    cmr_u32 ret_val = SENSOR_FAIL;
    cmr_u16 num_byte = WRITE_DUAL_OTP_SIZE;
    char value[PROPERTY_VALUE_MAX];

    property_get("debug.dualcamera.write.otp", value, "false");
    if (!strcmp(value, "true") && (sensor_id == 0)) {
        SENSOR_LOGI("write dualotp");
        const char *psPath_OtpData = "data/vendor/cameraserver/otp.txt";

        otp_params_t pdata;
        cmr_u8 *dual_data = (cmr_u8 *)malloc(num_byte);
        int otp_ret = read_txt_file(psPath_OtpData, dual_data);
        pdata.buffer = dual_data;
        pdata.num_bytes = num_byte;
        int i = 0;
        SENSOR_LOGI("read dualotp bin,len = %d", otp_ret);
        if (otp_ret > 0) {
            int ret = sensor_otp_rw_ctrl(sensor_cxt, OTP_WRITE_DATA, 0,
                                         (void *)&pdata);
        }
        free(dual_data);
        SENSOR_LOGI("free dualotp file");
    }

    return ret_val;
}

LOCAL cmr_int sensor_set_mode_msg(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 mode, cmr_u32 is_inited) {
    CMR_MSG_INIT(message);
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    message.msg_type = SENSOR_CTRL_EVT_SETMODE;
    message.sub_msg_type = mode;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)((cmr_uint)is_inited);
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return SENSOR_FAIL;
    }

    return ret;
}

cmr_int sensor_set_mode_done_common(cmr_handle sns_module_handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_SETMODONE;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return SENSOR_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_set_mode(struct sensor_drv_context *sensor_cxt,
                              cmr_u32 mode, cmr_u32 is_inited) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u32 mclk;

    SENSOR_REG_TAB_INFO_T *res_info_ptr = PNULL;
    struct sensor_trim_tag *res_trim_ptr = PNULL;
    struct module_cfg_info *mod_cfg_info = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    mod_cfg_info = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_MODULE_CFG);
    res_trim_ptr = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_TRIM_TAB);
    res_info_ptr = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_RESOLUTION);

    SENSOR_LOGI("mode = %d.", mode);
    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        rtn = SENSOR_OP_STATUS_ERR;
        goto exit;
    }

    if(!sensor_cxt->is_long_expo) {
        if (sensor_cxt->sensor_mode == mode) {
            SENSOR_LOGI("The sensor mode as before");
            rtn = SENSOR_SUCCESS;
            goto exit;
        }
    }

    if (PNULL != res_info_ptr[mode].sensor_reg_tab_ptr) {
        mclk = res_info_ptr[mode].xclk_to_sensor;
        hw_sensor_set_mclk(sensor_cxt->hw_drv_handle, mclk);

        hw_Sensor_SendRegTabToSensor(sensor_cxt->hw_drv_handle,
                                     &res_info_ptr[mode]);
        sensor_cxt->sensor_mode = mode;
    } else {
        SENSOR_LOGI("No this resolution information !!!");
    }

#if 0
    if (is_inited) {
        if (SENSOR_INTERFACE_TYPE_CSI2 == mod_cfg_info->sensor_interface.type)
            /*stream off first for MIPI sensor switch*/
            if (SENSOR_SUCCESS == sensor_stream_off(sensor_cxt)) {
            }
    }
#endif
exit:
    ATRACE_END();
    return rtn;
}

cmr_int sensor_set_mode_common(cmr_handle sns_module_handle, cmr_u32 mode) {
    cmr_int ret = 0;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    ret = sensor_set_mode_msg(sensor_cxt, mode, 1);
    return ret;
}

cmr_int sensor_get_mode_common(cmr_handle sns_module_handle, cmr_u32 *mode) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(mode);
    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        ret = SENSOR_OP_STATUS_ERR;
        goto exit;
    }
    *mode = sensor_cxt->sensor_mode;
exit:
    return ret;
}

cmr_int sensor_stream_on(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = 0;

    cmr_int err = 0xff;
    cmr_u32 param = 0;
    SENSOR_IOCTL_FUNC_PTR stream_on_func = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_LOGI("E");

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops && sns_ops->ext_ops[SENSOR_IOCTL_STREAM_ON].ops) {
        stream_on_func = sns_ops->ext_ops[SENSOR_IOCTL_STREAM_ON].ops;
    } else {
        SENSOR_LOGE("get stream_on function failed!");
        return SENSOR_FAIL;
    }

    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("X: sensor has not been initialized");
        return SENSOR_FAIL;
    }

    if (!sensor_cxt->stream_on) {
        param = sensor_cxt->bypass_mode;
        if (param > 0) {
            struct sensor_ic_drv_cxt *sns_drv_cxt =
                (struct sensor_ic_drv_cxt *)sensor_cxt->sns_ic_drv_handle;
            sns_drv_cxt->is_multi_mode = param;
        }
        err = stream_on_func(sensor_cxt->sns_ic_drv_handle, param);
    }
    if (0 == err) {
        sensor_cxt->stream_on = 1;
        char value[PROPERTY_VALUE_MAX];

        property_get("persist.vendor.cam.sensor.info", value, "0");
        if (!strcmp(value, "trigger")) {
            SENSOR_LOGI("trigger E\n");
            char value1[255] = {0x00};
            int mode = sensor_cxt->sensor_mode;
            SENSOR_REG_TAB_INFO_T *res_info_ptr = PNULL;
            struct sensor_trim_tag *res_trim_ptr = PNULL;
            struct module_cfg_info *mod_cfg_info = PNULL;

            mod_cfg_info =
                sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_MODULE_CFG);
            res_trim_ptr =
                sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_TRIM_TAB);
            res_info_ptr =
                sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_RESOLUTION);
            sprintf(
                value1, "%s mode: %d %dlanes size: %dx%d bps per lane: %dMbps",
                sensor_cxt->sensor_info_ptr->name, sensor_cxt->sensor_mode,
                mod_cfg_info[mode].sensor_interface.bus_width,
                res_trim_ptr[mode].trim_width, res_trim_ptr[mode].trim_height,
                res_trim_ptr[mode].bps_per_lane);
            SENSOR_LOGI("trigger %s\n", value1);
            ret = property_set("persist.vendor.cam.sensor.info", value1);
        }
    }
    SENSOR_LOGI("X %ld", ret);
    ATRACE_END();
    return err;
}

cmr_int sensor_stream_ctrl_common(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 on_off) {
    CMR_MSG_INIT(message);
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_STREAM_CTRL;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.sub_msg_type = on_off;
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return SENSOR_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_stream_ctrl(struct sensor_drv_context *sensor_cxt,
                                 cmr_u32 on_off) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 mode;
    struct module_cfg_info *mod_cfg_info = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    mod_cfg_info = (struct module_cfg_info *)sensor_ic_get_data(
        sensor_cxt, SENSOR_CMD_GET_MODULE_CFG);
    SENSOR_LOGI("on_off %d", on_off);
    if (on_off == 1) {
        if (SENSOR_INTERFACE_TYPE_CSI2 == mod_cfg_info->sensor_interface.type) {
            mode = sensor_cxt->sensor_mode;
            struct hw_mipi_init_param init_param;
            init_param.lane_num =
                sensor_cxt->sensor_exp_info.sensor_interface.bus_width;
            init_param.bps_per_lane =
                sensor_cxt->sensor_exp_info.sensor_mode_info[mode].bps_per_lane;
            init_param.is_cphy =
                sensor_cxt->sensor_exp_info.sensor_interface.is_cphy == 1 ? 1
                                                                          : 0;
            if (sensor_cxt->sensor_exp_info.sensor_interface.lane_switch_eb)
                init_param.lane_seq =
                    sensor_cxt->sensor_exp_info.sensor_interface.lane_seq;
            else
                init_param.lane_seq = 0x0123;
            SENSOR_LOGI("is_cphy %d %016lx", init_param.is_cphy,
                        init_param.lane_seq);
            ret = hw_sensor_mipi_init(sensor_cxt->hw_drv_handle, init_param);
            if (ret) {
                SENSOR_LOGE("mipi initial failed ret %ld", ret);
            } else {
                SENSOR_LOGI("mipi initial ok");
            }
        }
        ret = sensor_stream_on(sensor_cxt);
    } else if (on_off == 2) {
        if (SENSOR_INTERFACE_TYPE_CSI2 == mod_cfg_info->sensor_interface.type) {
            struct hw_mipi_init_param init_param;
            mode = sensor_cxt->sensor_mode;
            init_param.lane_num =
                sensor_cxt->sensor_exp_info.sensor_interface.bus_width;
            init_param.bps_per_lane =
                sensor_cxt->sensor_exp_info.sensor_mode_info[mode].bps_per_lane;
            init_param.is_cphy =
                sensor_cxt->sensor_exp_info.sensor_interface.is_cphy == 1 ? 1
                                                                          : 0;
            if (sensor_cxt->sensor_exp_info.sensor_interface.lane_switch_eb)
                init_param.lane_seq =
                    sensor_cxt->sensor_exp_info.sensor_interface.lane_seq;
            else
                init_param.lane_seq = 0x0123;
            SENSOR_LOGI("is_cphy %d %016lx", init_param.is_cphy,
                        init_param.lane_seq);
            ret = hw_sensor_mipi_switch(sensor_cxt->hw_drv_handle, init_param);
        }
    } else {
        ret = sensor_stream_off(sensor_cxt);
        if (SENSOR_SUCCESS == ret) {
            if (SENSOR_INTERFACE_TYPE_CSI2 ==
                mod_cfg_info->sensor_interface.type) {
                hw_sensor_mipi_deinit(sensor_cxt->hw_drv_handle);
                mode = sensor_cxt->sensor_mode;
            }
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int sensor_stream_off(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int err = 0xff;
    cmr_u32 param = 0;
    SENSOR_IOCTL_FUNC_PTR stream_off_func = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops && sns_ops->ext_ops[SENSOR_IOCTL_STREAM_OFF].ops) {
        stream_off_func = sns_ops->ext_ops[SENSOR_IOCTL_STREAM_OFF].ops;
    } else {
        SENSOR_LOGE("get stream_off function failed!");
        return SENSOR_FAIL;
    }

    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("X: sensor has not been initialized");
        return SENSOR_FAIL;
    }

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    if (PNULL != stream_off_func && sensor_cxt->stream_on == 1) {
        param = sensor_cxt->bypass_mode;
        err = stream_off_func(sensor_cxt->sns_ic_drv_handle, param);
    }
    sensor_cxt->stream_on = 0;

    SENSOR_LOGI("X");
    ATRACE_END();
    return err;
}

cmr_int sensor_get_info_common(struct sensor_drv_context *sensor_cxt,
                               SENSOR_EXP_INFO_T **sensor_exp_info_pptr) {
    if (PNULL == sensor_cxt) {
        SENSOR_LOGE("zero pointer ");
        return SENSOR_FAIL;
    }
    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        return SENSOR_FAIL;
    }

    SENSOR_LOGV("info = %p ", (void *)&sensor_cxt->sensor_exp_info);
    *sensor_exp_info_pptr = &sensor_cxt->sensor_exp_info;
    return SENSOR_SUCCESS;
}

SENSOR_EXP_INFO_T *Sensor_GetInfo(void) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sensor_get_dev_cxt();

    if (PNULL == sensor_cxt) {
        SENSOR_LOGE("zero pointer ");
        return PNULL;
    }
    if (!sensor_cxt->sensor_isInit) {
        SENSOR_LOGE("sensor has not init");
        return PNULL;
    }

    SENSOR_LOGI("info=%p ", (void *)&sensor_cxt->sensor_exp_info);
    return &sensor_cxt->sensor_exp_info;
}

SENSOR_EXP_INFO_T *Sensor_GetInfo_withid(cmr_u32 id) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sensor_get_dev_cxt_Ex(id);

    if (PNULL == sensor_cxt) {
        SENSOR_LOGE("zero pointer ");
        return PNULL;
    }
    if (!sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGI("sensor has not init");
        return PNULL;
    }

    SENSOR_LOGI("info=%lx ", (void *)&sensor_cxt->sensor_exp_info);
    return &sensor_cxt->sensor_exp_info;
}

/*********************************************************************************
 todo:
 now the sensor_close_common only support close all sensor, and the function
 should be
 updated when the hardware can supported multiple sensors;
 *********************************************************************************/
cmr_int sensor_close_common(struct sensor_drv_context *sensor_cxt,
                            cmr_u32 sensor_id) {
    UNUSED(sensor_id);

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sensor_otp_module_deinit(sensor_cxt);
    sensor_af_deinit(sensor_cxt);
    sensor_context_deinit(sensor_cxt, 1);
    SENSOR_LOGI("X");

    return SENSOR_SUCCESS;
}

LOCAL cmr_int sensor_init_defaul_exif(struct sensor_drv_context *sensor_cxt) {
    EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = PNULL;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    exif_ptr = &sensor_cxt->default_exif;
    cmr_bzero(&sensor_cxt->default_exif, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

    SENSOR_LOGV("E");

    exif_ptr->valid.FNumber = 1;
    exif_ptr->FNumber.numerator = 14;
    exif_ptr->FNumber.denominator = 5;
    exif_ptr->valid.ExposureProgram = 1;
    exif_ptr->ExposureProgram = 0x04;
    exif_ptr->valid.ApertureValue = 1;
    exif_ptr->ApertureValue.numerator = 14;
    exif_ptr->ApertureValue.denominator = 5;
    exif_ptr->valid.MaxApertureValue = 1;
    exif_ptr->MaxApertureValue.numerator = 14;
    exif_ptr->MaxApertureValue.denominator = 5;
    exif_ptr->valid.FocalLength = 1;
    exif_ptr->FocalLength.numerator = 289;
    exif_ptr->FocalLength.denominator = 100;
    exif_ptr->valid.FileSource = 1;
    exif_ptr->FileSource = 0x03;
    exif_ptr->valid.ExposureMode = 1;
    exif_ptr->ExposureMode = 0x00;
    exif_ptr->valid.WhiteBalance = 1;
    exif_ptr->WhiteBalance = 0x00;
    exif_ptr->valid.Flash = 1;

    SENSOR_LOGV("X");
    return SENSOR_SUCCESS;
}

cmr_int sensor_set_exif_common(cmr_handle sns_module_handle, cmr_u32 cmdin,
                               cmr_u64 param) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_EXIF_CTRL_E cmd = (SENSOR_EXIF_CTRL_E)cmdin;
    SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
    EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = PNULL;
    void *exif_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
    SENSOR_DRV_CHECK_ZERO(sensor_info_ptr);

    exif_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)sensor_ic_get_data(
        sensor_cxt, SENSOR_CMD_GET_EXIF);
    if (exif_ptr) {
        sensor_exif_info_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)exif_ptr;
    } else {
        SENSOR_LOGV("the fun is null, set it to default");
        sensor_exif_info_ptr = &sensor_cxt->default_exif;
    }

    switch (cmd) {
    case SENSOR_EXIF_CTRL_EXPOSURETIME_BYTIME: {
        cmr_u64 exposure_time = param / 1000;
        sensor_exif_info_ptr->valid.ExposureTime = 1;

        if (0x00 == exposure_time) {
            sensor_exif_info_ptr->valid.ExposureTime = 0;
        } else if (1000000 >= exposure_time) {
            sensor_exif_info_ptr->ExposureTime.denominator =
                (1000000.00 / exposure_time) * 1000 + 0.5;
            sensor_exif_info_ptr->ExposureTime.numerator = 1000;
        } else {
            cmr_u32 second = 0x00;
            do {
                second++;
                exposure_time -= 1000000;
                if (1000000 >= exposure_time) {
                    break;
                }
            } while (1);
            sensor_exif_info_ptr->ExposureTime.denominator =
                (1000000.00 / exposure_time) * 1000 + 0.5;
            sensor_exif_info_ptr->ExposureTime.numerator =
                sensor_exif_info_ptr->ExposureTime.denominator * second  + 1000;
        }
        SENSOR_LOGV("ExposureTime = %d/%d", sensor_exif_info_ptr->ExposureTime.numerator,
                sensor_exif_info_ptr->ExposureTime.denominator);
        break;
    }
    case SENSOR_EXIF_CTRL_EXPOSURETIME: {
        enum sensor_mode img_sensor_mode = sensor_cxt->sensor_mode;
        if (img_sensor_mode == 0)
            img_sensor_mode = 1;
        cmr_u64 exposureline_time =
            sensor_info_ptr->sensor_mode_info[img_sensor_mode].line_time;
        cmr_u64 exposureline_num = param;
        cmr_u64 exposure_time = 0x00;

        exposure_time = exposureline_time * exposureline_num / 1000;
        sensor_exif_info_ptr->valid.ExposureTime = 1;

        if (0x00 == exposure_time) {
            sensor_exif_info_ptr->valid.ExposureTime = 0;
        } else if (1000000 >= exposure_time) {
            sensor_exif_info_ptr->ExposureTime.denominator =
                (1000000.00 / exposure_time) * 1000 + 0.5;
            sensor_exif_info_ptr->ExposureTime.numerator = 1000;
        } else {
            cmr_u32 second = 0x00;
            do {
                second++;
                exposure_time -= 1000000;
                if (1000000 >= exposure_time) {
                    break;
                }
            } while (1);
            sensor_exif_info_ptr->ExposureTime.denominator =
                (1000000.00 / exposure_time) * 1000 + 0.5;
            sensor_exif_info_ptr->ExposureTime.numerator =
                sensor_exif_info_ptr->ExposureTime.denominator * second  + 1000;
        }
        SENSOR_LOGV("ExposureTime = %d/%d", sensor_exif_info_ptr->ExposureTime.numerator,
                sensor_exif_info_ptr->ExposureTime.denominator);
        break;
    }
    case SENSOR_EXIF_CTRL_FNUMBER:
        sensor_exif_info_ptr->valid.FNumber = 1;
        sensor_exif_info_ptr->FNumber.numerator = param;
        sensor_exif_info_ptr->FNumber.denominator = 100;
        break;
        break;
    case SENSOR_EXIF_CTRL_EXPOSUREPROGRAM:
        break;
    case SENSOR_EXIF_CTRL_SPECTRALSENSITIVITY:
        break;
    case SENSOR_EXIF_CTRL_ISOSPEEDRATINGS:
        if (param == 6) {
            /*6 = CAMERA_ISO_MAX*/
            sensor_exif_info_ptr->valid.ISOSpeedRatings = 0;
        } else {
            sensor_exif_info_ptr->valid.ISOSpeedRatings = 1;
        }
        sensor_exif_info_ptr->ISOSpeedRatings.count = 1;
        sensor_exif_info_ptr->ISOSpeedRatings.type = EXIF_SHORT;
        sensor_exif_info_ptr->ISOSpeedRatings.size = 2;
        cmr_copy((void *)&sensor_exif_info_ptr->ISOSpeedRatings.ptr[0],
                 (void *)&param, 2);
        break;
    case SENSOR_EXIF_CTRL_OECF:
        break;
    case SENSOR_EXIF_CTRL_SHUTTERSPEEDVALUE:
        break;
    case SENSOR_EXIF_CTRL_APERTUREVALUE:
        sensor_exif_info_ptr->valid.ApertureValue = 1;
        sensor_exif_info_ptr->ApertureValue.numerator = param;
        sensor_exif_info_ptr->ApertureValue.denominator = 10;
        break;
    case SENSOR_EXIF_CTRL_BRIGHTNESSVALUE: {
        sensor_exif_info_ptr->valid.BrightnessValue = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->BrightnessValue.numerator = 1;
            sensor_exif_info_ptr->BrightnessValue.denominator = 1;
            break;
        case 3:
            sensor_exif_info_ptr->BrightnessValue.numerator = 0;
            sensor_exif_info_ptr->BrightnessValue.denominator = 0;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->BrightnessValue.numerator = 2;
            sensor_exif_info_ptr->BrightnessValue.denominator = 2;
            break;
        default:
            sensor_exif_info_ptr->BrightnessValue.numerator = 0xff;
            sensor_exif_info_ptr->BrightnessValue.denominator = 0xff;
            break;
        }
        break;
    } break;
    case SENSOR_EXIF_CTRL_EXPOSUREBIASVALUE:
        break;
    case SENSOR_EXIF_CTRL_MAXAPERTUREVALUE:
        sensor_exif_info_ptr->valid.MaxApertureValue = 1;
        sensor_exif_info_ptr->MaxApertureValue.numerator = param;
        sensor_exif_info_ptr->MaxApertureValue.denominator = 10;
        break;
    case SENSOR_EXIF_CTRL_SUBJECTDISTANCE:
        break;
    case SENSOR_EXIF_CTRL_METERINGMODE:
        break;
    case SENSOR_EXIF_CTRL_LIGHTSOURCE: {
        sensor_exif_info_ptr->valid.LightSource = 1;
        switch (param) {
        case 0:
            sensor_exif_info_ptr->LightSource = 0x00;
            break;
        case 1:
            sensor_exif_info_ptr->LightSource = 0x03;
            break;
        case 2:
            sensor_exif_info_ptr->LightSource = 0x0f;
            break;
        case 3:
            sensor_exif_info_ptr->LightSource = 0x0e;
            break;
        case 4:
            sensor_exif_info_ptr->LightSource = 0x02;
            break;
        case 5:
            sensor_exif_info_ptr->LightSource = 0x01;
            break;
        case 6:
            sensor_exif_info_ptr->LightSource = 0x0a;
            break;
        default:
            sensor_exif_info_ptr->LightSource = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_FLASH:
        sensor_exif_info_ptr->valid.Flash = 1;
        sensor_exif_info_ptr->Flash = param;
        break;
    case SENSOR_EXIF_CTRL_FOCALLENGTH:
        break;
    case SENSOR_EXIF_CTRL_SUBJECTAREA:
        break;
    case SENSOR_EXIF_CTRL_FLASHENERGY:
        break;
    case SENSOR_EXIF_CTRL_SPATIALFREQUENCYRESPONSE:
        break;
    case SENSOR_EXIF_CTRL_FOCALPLANEXRESOLUTION:
        break;
    case SENSOR_EXIF_CTRL_FOCALPLANEYRESOLUTION:
        break;
    case SENSOR_EXIF_CTRL_FOCALPLANERESOLUTIONUNIT:
        break;
    case SENSOR_EXIF_CTRL_SUBJECTLOCATION:
        break;
    case SENSOR_EXIF_CTRL_EXPOSUREINDEX:
        break;
    case SENSOR_EXIF_CTRL_SENSINGMETHOD:
        break;
    case SENSOR_EXIF_CTRL_FILESOURCE:
        break;
    case SENSOR_EXIF_CTRL_SCENETYPE:
        break;
    case SENSOR_EXIF_CTRL_CFAPATTERN:
        break;
    case SENSOR_EXIF_CTRL_CUSTOMRENDERED:
        break;
    case SENSOR_EXIF_CTRL_EXPOSUREMODE:
        break;

    case SENSOR_EXIF_CTRL_WHITEBALANCE:
        sensor_exif_info_ptr->valid.WhiteBalance = 1;
        if (param)
            sensor_exif_info_ptr->WhiteBalance = 1;
        else
            sensor_exif_info_ptr->WhiteBalance = 0;
        break;

    case SENSOR_EXIF_CTRL_DIGITALZOOMRATIO:
        break;
    case SENSOR_EXIF_CTRL_FOCALLENGTHIN35MMFILM:
        break;
    case SENSOR_EXIF_CTRL_SCENECAPTURETYPE: {
        sensor_exif_info_ptr->valid.SceneCaptureType = 1;
        switch (param) {
        case 0:
            sensor_exif_info_ptr->SceneCaptureType = 0x00;
            break;
        case 1:
            sensor_exif_info_ptr->SceneCaptureType = 0x03;
            break;
        default:
            sensor_exif_info_ptr->LightSource = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_GAINCONTROL:
        break;
    case SENSOR_EXIF_CTRL_CONTRAST: {
        sensor_exif_info_ptr->valid.Contrast = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->Contrast = 0x01;
            break;
        case 3:
            sensor_exif_info_ptr->Contrast = 0x00;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->Contrast = 0x02;
            break;
        default:
            sensor_exif_info_ptr->Contrast = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_SATURATION: {
        sensor_exif_info_ptr->valid.Saturation = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->Saturation = 0x01;
            break;
        case 3:
            sensor_exif_info_ptr->Saturation = 0x00;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->Saturation = 0x02;
            break;
        default:
            sensor_exif_info_ptr->Saturation = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_SHARPNESS: {
        sensor_exif_info_ptr->valid.Sharpness = 1;
        switch (param) {
        case 0:
        case 1:
        case 2:
            sensor_exif_info_ptr->Sharpness = 0x01;
            break;
        case 3:
            sensor_exif_info_ptr->Sharpness = 0x00;
            break;
        case 4:
        case 5:
        case 6:
            sensor_exif_info_ptr->Sharpness = 0x02;
            break;
        default:
            sensor_exif_info_ptr->Sharpness = 0xff;
            break;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_DEVICESETTINGDESCRIPTION:
        break;
    case SENSOR_EXIF_CTRL_SUBJECTDISTANCERANGE:
        break;
    default:
        break;
    }
    return SENSOR_SUCCESS;
}

cmr_int sensor_update_isparm_from_file(struct sensor_drv_context *sensor_cxt,
                                       cmr_u32 sensor_id) {
    cmr_int ret = SENSOR_SUCCESS;

    if (!sensor_cxt) {
        return SENSOR_FAIL;
    }

    ret = isp_raw_para_update_from_file(sensor_cxt->sensor_info_ptr, sensor_id);
    return ret;
}

cmr_int sensor_get_exif_common(cmr_handle sns_module_handle, void **param) {
    struct sensor_ic_ops *sns_ops = PNULL;
    void *exif_ptr = PNULL;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    EXIF_SPEC_PIC_TAKING_COND_T **sensor_exif_info_pptr =
        (EXIF_SPEC_PIC_TAKING_COND_T **)param;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_exif_info_pptr);

    exif_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)sensor_ic_get_data(
        sensor_cxt, SENSOR_CMD_GET_EXIF);
    if (exif_ptr) {
        *sensor_exif_info_pptr = (EXIF_SPEC_PIC_TAKING_COND_T *)exif_ptr;
    } else {
        SENSOR_LOGV("the fun is null, set it to default");
        *sensor_exif_info_pptr = &sensor_cxt->default_exif;
    }
    return SENSOR_SUCCESS;
}

cmr_int sensor_set_raw_infor(struct sensor_drv_context *sensor_cxt,
                             cmr_u8 vendor_id) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_VAL_T val;
    struct sensor_ic_ops *sns_ops = PNULL;
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    val.type = SENSOR_VAL_TYPE_SET_RAW_INFOR;
    val.pval = &vendor_id;

    SENSOR_LOGI("vendor id %x\n", vendor_id);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops)
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                            (cmr_uint)&val);
    return ret;
}

static cmr_int
sensor_drv_get_sensor_type(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_int sensor_type = 0;
    SENSOR_VAL_T val;
    struct sensor_4in1_info sn_4in1_info;
    struct sensor_ic_ops *sns_ops = PNULL;
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;

    cmr_bzero(&sn_4in1_info, sizeof(struct sensor_4in1_info));
    val.type = SENSOR_VAL_TYPE_GET_4IN1_INFO;
    val.pval = &sn_4in1_info;
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops) {
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                            (cmr_uint)&val);
        if (ret) {
            SENSOR_LOGI("failed to get 4in1 info");
        }
    }
    SENSOR_IMAGE_FORMAT sensor_format =
        sensor_cxt->sensor_info_ptr->image_format;
    SENSOR_LOGI("is_4in1_supported %d, sensor_format = %d",
                sn_4in1_info.is_4in1_supported, sensor_format);

    if (sn_4in1_info.is_4in1_supported) {
        sensor_type = FOURINONE_SW; // 4in1 sensor, need software remosaic
    } else if (sn_4in1_info.limited_4in1_width > 0 &&
               sn_4in1_info.limited_4in1_height > 0) {
        sensor_type = FOURINONE_HW; // 4in1 sensor, hardware remosaic
    } else {
        switch (sensor_format) {
        case SENSOR_IMAGE_FORMAT_YUV422:
            sensor_type = YUVSENSOR;
            break;
        default:
            sensor_type = RAWSENSOR;
            break;
        }
    }
    return sensor_type;
}

cmr_int sensor_set_otp_data(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_VAL_T val;
    struct sensor_ic_ops *sns_ops = PNULL;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)sensor_cxt->otp_drv_handle;

    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    val.type = SENSOR_VAL_TYPE_SET_OTP_DATA;
    val.pval = otp_cxt->otp_raw_data.buffer;

    SENSOR_LOGI("otp_raw_data:%p", val.pval);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops)
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                            (cmr_uint)&val);
    return ret;
}

LOCAL cmr_int sensor_otp_rw_ctrl(struct sensor_drv_context *sensor_cxt,
                                 uint8_t cmd, uint8_t sub_cmd, void *data) {
    CMR_MSG_INIT(message);
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    otp_ctrl_cmd_t *otp_cmd = malloc(sizeof(otp_ctrl_cmd_t));
    if (!otp_cmd) {
        SENSOR_LOGE("otp cmd buffer alloc failed!");
        return -1;
    }
    otp_cmd->cmd = cmd;
    otp_cmd->sub_cmd = sub_cmd;
    otp_cmd->data = data;
    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_CFGOTP;
    if (cmd == OTP_WRITE_DATA) {
        // message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    } else {
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    }
    message.alloc_flag = 1; /*alloc from sender*/
    message.data = (void *)otp_cmd;

    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        if (message.data) {
            free(message.data);
        }
        return SENSOR_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_otp_process(struct sensor_drv_context *sensor_cxt,
                                 uint8_t cmd, uint8_t sub_cmd, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    struct _sensor_val_tag param;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->otp_drv_info.otp_drv_entry &&
        sensor_cxt->otp_drv_handle) {
        switch (cmd) {
        case OTP_READ_RAW_DATA:
            ret = module->otp_drv_info.otp_drv_entry->otp_ops.sensor_otp_read(
                sensor_cxt->otp_drv_handle, data);
            break;
        case OTP_READ_PARSE_DATA:
            module->otp_drv_info.otp_drv_entry->otp_ops.sensor_otp_read(
                sensor_cxt->otp_drv_handle, NULL);
            ret = module->otp_drv_info.otp_drv_entry->otp_ops.sensor_otp_parse(
                sensor_cxt->otp_drv_handle, data);
            if (ret == SENSOR_SUCCESS) {
                ret = module->otp_drv_info.otp_drv_entry->otp_ops
                          .sensor_otp_calibration(sensor_cxt->otp_drv_handle);
            } else {
                SENSOR_LOGE("otp prase failed");
                return ret;
            }
            break;
        case OTP_WRITE_DATA:
            ret = module->otp_drv_info.otp_drv_entry->otp_ops.sensor_otp_write(
                sensor_cxt->otp_drv_handle, data);
            if (ret != SENSOR_SUCCESS)
                return ret;
            break;
        case OTP_IOCTL:
            ret = module->otp_drv_info.otp_drv_entry->otp_ops.sensor_otp_ioctl(
                sensor_cxt->otp_drv_handle, sub_cmd, data);
            if (ret != SENSOR_SUCCESS)
                return ret;
            break;
        default:
            break;
        }
    } else {
#if 0
        if ((NULL != sensor_cxt->sensor_info_ptr) &&
            (NULL != sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr)) {
            param.type = SENSOR_VAL_TYPE_INIT_OTP;
            if (PNULL !=
                sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cfg_otp) {
                sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cfg_otp(
                    sensor_cxt->hw_drv_handle, (cmr_uint)&param);
            }
        } else {
            SENSOR_LOGE("invalid param failed!");
            return CMR_CAMERA_INVALID_PARAM;
        }
#endif
    }
    ATRACE_END();
    return ret;
}

#include <cutils/properties.h>

void sensor_save_all_sensor_name(void) {
    const char *const front_cam_name_interface ="/sys/ontim_bootinfo/front_cam_info";
	const char *const frontaux_cam_name_interface ="/sys/ontim_bootinfo/frontaux_cam_info";
	const char *const back_cam_name_interface ="/sys/ontim_bootinfo/back_cam_info";
	const char *const backaux_cam_name_interface ="/sys/ontim_bootinfo/backaux_cam_info";
	const char *const backaux2_cam_name_interface ="/sys/ontim_bootinfo/backaux2_cam_info";

    const char *const front_cam_efuse_interface ="/sys/ontim_bootinfo/front_cam_efuse";
	const char *const frontaux_cam_efuse_interface ="/sys/ontim_bootinfo/frontaux_cam_efuse";
	const char *const back_cam_efuse_interface ="/sys/ontim_bootinfo/back_cam_efuse";
	const char *const backaux_cam_efuse_interface ="/sys/ontim_bootinfo/backaux_cam_efuse";
	const char *const backaux2_cam_efuse_interface ="/sys/ontim_bootinfo/backaux2_cam_efuse";

    ssize_t ret;
    int fd;

    SENSOR_LOGI("E");

    fd = open(back_cam_name_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, back_cam_name, strlen(back_cam_name));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
	close(fd);

    fd = open(front_cam_name_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, front_cam_name, strlen(front_cam_name));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
	close(fd);

    fd = open(backaux_cam_name_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, backaux_cam_name, strlen(backaux_cam_name));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
	close(fd);

    fd = open(backaux2_cam_name_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, backaux2_cam_name, strlen(backaux2_cam_name));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
    close(fd);

    fd = open(frontaux_cam_name_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, frontaux_cam_name, strlen(frontaux_cam_name));
    if (-1 == ret) {
        SENSOR_LOGE("write sensor_info failed \n");
        close(fd);
        goto exit;
    }
    close(fd);

//camera efuse id
    fd = open(back_cam_efuse_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, back_cam_efuse, strlen(back_cam_efuse));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
	close(fd);

    fd = open(front_cam_efuse_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, front_cam_efuse, strlen(front_cam_efuse));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
	close(fd);

    fd = open(backaux_cam_efuse_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, backaux_cam_efuse, strlen(backaux_cam_efuse));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
	close(fd);

    fd = open(backaux2_cam_efuse_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, backaux2_cam_efuse, strlen(backaux2_cam_efuse));
	if (-1 == ret) {
		SENSOR_LOGE("write sensor_info failed \n");
		close(fd);
		goto exit;
	}
    close(fd);

    fd = open(frontaux_cam_efuse_interface, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto exit;
    }
	ret = write(fd, frontaux_cam_efuse, strlen(frontaux_cam_efuse));
    if (-1 == ret) {
        SENSOR_LOGE("write sensor_info failed \n");
        close(fd);
        goto exit;
    }
    close(fd);
exit:
    SENSOR_LOGI("X");
}
static void sensor_rid_save_sensor_info(char *sensor_info, cmr_u32 slot_id) {
    const char *const sensorInterface0 =
        "/sys/devices/virtual/misc/sprd_sensor/camera_sensor_name";
    char sensor_info_with_slot_id[256];
    ssize_t ret;
    int fd;

    SENSOR_LOGV("E");

    SENSOR_LOGI("sensor_version_info info %s \n", sensor_info);

    fd = open(sensorInterface0, O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        SENSOR_LOGE("Failed to open: sensorInterface");
        goto set_property;
    }
    ret = write(fd, sensor_info, strlen(sensor_info));
    if (-1 == ret) {
        SENSOR_LOGE("write sensor_info failed \n");
        close(fd);
        goto set_property;
    }
    close(fd);
set_property:
    ret = property_set("vendor.cam.sensor.info", sensor_info);
    SENSOR_LOGI("slot id is %d", slot_id);
    if (strlen(sensor_info) < sizeof(sensor_info_with_slot_id)) {
        sprintf(sensor_info_with_slot_id, "<slot:%d>\n%s", slot_id,
                sensor_info);
        property_set("vendor.cam.sensor.slot.info", sensor_info_with_slot_id);
    }
exit:
    SENSOR_LOGV("X");
}
void sensor_rid_save_sensor_name(SENSOR_HWINFOR_E mag, char *info) {
    SENSOR_LOGI("E");
    SENSOR_LOGI("sensor_version_info info %s \n", info);

	switch (mag){
    case SENSOR_HWINFOR_BACK_CAM_NAME:
		memset(back_cam_name, 0x00, sizeof(back_cam_name));
		memcpy(back_cam_name, info, 64);
        break;
    case SENSOR_HWINFOR_FRONT_CAM_NAME:
		memset(front_cam_name, 0x00, sizeof(front_cam_name));
		memcpy(front_cam_name, info, 64);
        break;
    case SENSOR_HWINFOR_BACKAUX_CAM_NAME:
		memset(backaux_cam_name, 0x00, sizeof(backaux_cam_name));
		memcpy(backaux_cam_name, info, 64);
        break;
    case SENSOR_HWINFOR_BACKAUX2_CAM_NAME:
		memset(backaux2_cam_name, 0x00, sizeof(backaux2_cam_name));
		memcpy(backaux2_cam_name, info, 64);
        break;
    case SENSOR_HWINFOR_FRONTAUX_CAM_NAME:
		memset(frontaux_cam_name, 0x00, sizeof(frontaux_cam_name));
		memcpy(frontaux_cam_name, info, 64);
        break;
    case SENSOR_HWINFOR_BACK_CAM_EFUSE:
		memset(back_cam_efuse, 0x00, sizeof(back_cam_efuse));
		memcpy(back_cam_efuse, info, 64);
        break;
    case SENSOR_HWINFOR_FRONT_CAM_EFUSE:
		memset(front_cam_efuse, 0x00, sizeof(front_cam_efuse));
		memcpy(front_cam_efuse, info, 64);
        break;
    case SENSOR_HWINFOR_BACKAUX_CAM_EFUSE:
		memset(backaux_cam_efuse, 0x00, sizeof(backaux_cam_efuse));
		memcpy(backaux_cam_efuse, info, 64);
        break;
    case SENSOR_HWINFOR_BACKAUX2_CAM_EFUSE:
		memset(backaux2_cam_efuse, 0x00, sizeof(backaux2_cam_efuse));
		memcpy(backaux2_cam_efuse, info, 64);
        break;
    case SENSOR_HWINFOR_FRONTAUX_CAM_EFUSE:
		memset(frontaux_cam_efuse, 0x00, sizeof(frontaux_cam_efuse));
		memcpy(frontaux_cam_efuse, info, 64);
        break;
    default:
		SENSOR_LOGE("wrong mag,do not open sensorInterface");
    }
    SENSOR_LOGI("X");
}

/*--------------------------AF INTERFACE-----------------------------*/

static cmr_int sensor_af_init(cmr_handle sns_module_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct af_drv_init_para input_ptr;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;
    SENSOR_DRV_CHECK_ZERO(module);

    sensor_cxt->sensor_info_ptr->focus_eb =
        (module->af_dev_info.af_drv_entry != NULL);
    SENSOR_LOGI("sensor %s is focus enable %d",
                sensor_cxt->sensor_info_ptr->name,
                sensor_cxt->sensor_info_ptr->focus_eb);

    if (module->af_dev_info.af_drv_entry && !sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->create) {
            input_ptr.af_work_mode = module->af_dev_info.af_work_mode;
            input_ptr.hw_handle = sensor_cxt->hw_drv_handle;
            ret = af_ops->create(&input_ptr, &sensor_cxt->af_drv_handle);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGI("af_drv_entry not configured:module:%p,entry:%p,af_ops:%p",
                    module, module ? module->af_dev_info.af_drv_entry : NULL,
                    af_ops);
        return SENSOR_FAIL;
    }

exit:
    return ret;
}

static cmr_int sensor_af_deinit(cmr_handle sns_module_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->delete) {
            ret = af_ops->delete (sensor_cxt->af_drv_handle, NULL);
            sensor_cxt->af_drv_handle = NULL;
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGI("not register af driver, return directly");
        return SENSOR_FAIL;
    }
    SENSOR_LOGD("X");
    return ret;
}

static cmr_int sensor_af_set_pos(cmr_handle sns_module_handle, cmr_u32 pos) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->set_pos) {
            ret = af_ops->set_pos(sensor_cxt->af_drv_handle, (uint16_t)pos);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        } else {
            ret = SENSOR_FAIL;
            SENSOR_LOGE("set_pos is null please check your af driver.");
        }
    } else {
        SENSOR_LOGE("af driver not exist, return directly");
        return SENSOR_FAIL;
    }

    return ret;
}

static cmr_int sensor_af_get_pos(cmr_handle sns_module_handle, cmr_u16 *pos) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->get_pos) {
            ret = af_ops->get_pos(sensor_cxt->af_drv_handle, pos);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("af driver not exist, return directly");
        return SENSOR_FAIL;
    }
    return ret;
}

static cmr_int sensor_af_get_pos_info(cmr_handle sns_module_handle,
                                      struct sensor_vcm_info *info) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && module->af_dev_info.af_drv_entry &&
        sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if (af_ops->ioctl) {
            ret = af_ops->ioctl(sensor_cxt->af_drv_handle,
                                CMD_SNS_AF_GET_POS_INFO, (void *)info);
            if (SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("af driver not exist, return directly");
        return SENSOR_FAIL;
    }
    return ret;
}

/*--------------------------OTP INTERFACE-----------------------------*/
static cmr_int sensor_otp_module_init(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGD("E");
    SENSOR_MATCH_T *module = sensor_cxt->current_module;
    otp_drv_init_para_t input_para = {0, NULL, 0, 0, 0, 0, 0, 0, 0};

    if (module && (module->otp_drv_info.otp_drv_entry) &&
        (!sensor_cxt->otp_drv_handle)) {
        input_para.hw_handle = sensor_cxt->hw_drv_handle;
        input_para.sensor_name = (char *)sensor_cxt->sensor_info_ptr->name;
        input_para.sensor_id = sensor_cxt->slot_id;
        input_para.sensor_ic_addr = sensor_cxt->i2c_addr;
        input_para.eeprom_i2c_addr = module->otp_drv_info.eeprom_i2c_addr;
        input_para.eeprom_num = module->otp_drv_info.eeprom_num;
        input_para.eeprom_size = module->otp_drv_info.eeprom_size;
        input_para.sensor_max_width =
            sensor_cxt->sensor_info_ptr->source_width_max;
        input_para.sensor_max_height =
            sensor_cxt->sensor_info_ptr->source_height_max;

        ret = module->otp_drv_info.otp_drv_entry->otp_ops.sensor_otp_create(
            &input_para, &sensor_cxt->otp_drv_handle);
    } else {
        SENSOR_LOGI("otp driver is not configured");
    }
    SENSOR_LOGV("X");
    ATRACE_END();
    return ret;
}

static cmr_int sensor_otp_module_deinit(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGD("E");
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

    if (module && (module->otp_drv_info.otp_drv_entry) &&
        sensor_cxt->otp_drv_handle) {
        ret = module->otp_drv_info.otp_drv_entry->otp_ops.sensor_otp_delete(
            sensor_cxt->otp_drv_handle);
        sensor_cxt->otp_drv_handle = NULL;
    } else {
        SENSOR_LOGI("not register otp driver, no need to release");
    }
    SENSOR_LOGV("X");
    return ret;
}

/*--------------------------HARDWARE INTERFACE-----------------------------*/
cmr_int sensor_hw_read_i2c(cmr_handle sns_module_handle, cmr_u16 slave_addr,
                           cmr_u8 *cmd, cmr_u16 cmd_length) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    ret = hw_sensor_read_i2c(sensor_cxt->hw_drv_handle, slave_addr, cmd,
                             cmd_length);
    return ret;
}

cmr_int sensor_hw_write_i2c(cmr_handle sns_module_handle, cmr_u16 slave_addr,
                            cmr_u8 *cmd, cmr_u16 cmd_length) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    ret = hw_sensor_write_i2c(sensor_cxt->hw_drv_handle, slave_addr, cmd,
                              cmd_length);
    return ret;
}

cmr_int sensor_muti_i2c_write(cmr_handle sns_module_handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_muti_aec_i2c_tag *aec_i2c_info =
        (struct sensor_muti_aec_i2c_tag *)param;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    SENSOR_IC_CHECK_PTR(aec_i2c_info);
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    ret = hw_sensor_muti_i2c_write(sensor_cxt->hw_drv_handle, aec_i2c_info);
    return ret;
}

/*--------------------------SENSOR IC INTERFACE-----------------------------*/
static cmr_int sensor_ic_create(struct sensor_drv_context *sensor_cxt,
                                cmr_u32 sensor_id) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_ic_drv_init_para sns_init_para;

    SENSOR_DRV_CHECK_PTR(sensor_cxt);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->sensor_info_ptr);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->current_module);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

    if (sns_ops && sns_ops->create_handle) {
        sns_init_para.caller_handle = sensor_cxt;
        sns_init_para.hw_handle = sensor_cxt->hw_drv_handle;
        sns_init_para.module_id =
            ((SENSOR_MATCH_T *)sensor_cxt->current_module)->module_id;
        sns_init_para.sensor_id = sensor_id;
        sns_init_para.is_HD_mode = sensor_cxt->is_HD_mode;
        sns_init_para.ops_cb.set_mode = sensor_set_mode_common;
        sns_init_para.ops_cb.get_mode = sensor_get_mode_common;
        sns_init_para.ops_cb.set_exif_info = sensor_set_exif_common;
        sns_init_para.ops_cb.set_snspid = sensor_set_snspid_common;
        sns_init_para.ops_cb.get_exif_info = sensor_get_exif_common;
        sns_init_para.ops_cb.set_mode_wait_done = sensor_set_mode_done_common;
        ret = sns_ops->create_handle(&sns_init_para,
                                     &sensor_cxt->sns_ic_drv_handle);
        if (ret)
            SENSOR_LOGE("sensor ic create handle failed");
    }

    return ret;
}

static cmr_int sensor_ic_delete(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_ic_drv_init_para sns_init_para;

    SENSOR_DRV_CHECK_PTR(sensor_cxt);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->sensor_info_ptr);

    if (sensor_cxt->sns_ic_drv_handle) {
        sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
        if (sns_ops && sns_ops->delete_handle) {
            ret = sns_ops->delete_handle(sensor_cxt->sns_ic_drv_handle, NULL);
            if (ret)
                SENSOR_LOGE("sensor ic delete handle failed");
        }
        sensor_cxt->sns_ic_drv_handle = PNULL;
    }

    return ret;
}

static void *sensor_ic_get_data(struct sensor_drv_context *sensor_cxt,
                                cmr_uint cmd) {
    void *data = PNULL;
    struct sensor_ic_ops *sns_ops = PNULL;

    if (!sensor_cxt || (!sensor_cxt->sensor_info_ptr)) {
        SENSOR_LOGE("sensor_cxt is null return\n");
        return PNULL;
    }
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (!sns_ops || !sns_ops->get_data) {
        SENSOR_LOGE("error:sensor ops is null\n");
        return PNULL;
    }

    switch (cmd) {
    case SENSOR_CMD_GET_TRIM_TAB:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle,
                          SENSOR_CMD_GET_TRIM_TAB, &data);
        break;
    case SENSOR_CMD_GET_RESOLUTION:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle,
                          SENSOR_CMD_GET_RESOLUTION, &data);
        break;
    case SENSOR_CMD_GET_MODULE_CFG:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle,
                          SENSOR_CMD_GET_MODULE_CFG, &data);
        break;
    case SENSOR_CMD_GET_EXIF:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle, SENSOR_CMD_GET_EXIF,
                          &data);
        break;
    case SENSOR_CMD_GET_STATIC_INFO:
        sns_ops->get_data(sensor_cxt->sns_ic_drv_handle,
                          SENSOR_CMD_GET_STATIC_INFO, &data);
        break;
    default:
        SENSOR_LOGW("not support cmd:0x%lx", cmd);
    }

    return data;
}

cmr_int sensor_ic_write_gain(cmr_handle handle, cmr_u32 param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

#ifdef SBS_MODE_SENSOR
    char value1[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.sbs.mode", value1, "0");
    if (!strcmp(value1, "slave")) {
#ifdef SBS_SENSOR_FRONT
        sns_ops = sns_drv_cntx[3].sensor_info_ptr->sns_ops;
#else
        sns_ops = sns_drv_cntx[2].sensor_info_ptr->sns_ops;
#endif
    }
#endif
    if (!(sns_ops && sns_ops->write_gain_value)) {
        SENSOR_LOGE("write_gain is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->write_gain_value(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_ex_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
#ifdef SBS_MODE_SENSOR
    char value1[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.sbs.mode", value1, "0");
    if (!strcmp(value1, "slave")) {
#ifdef SBS_SENSOR_FRONT
        sns_ops = sns_drv_cntx[3].sensor_info_ptr->sns_ops;
#else
        sns_ops = sns_drv_cntx[2].sensor_info_ptr->sns_ops;
#endif
    }
#endif
    if (!(sns_ops && sns_ops->ex_write_exp)) {
        SENSOR_LOGE("ex_write_exp is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->ex_write_exp(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_write_ae_value(cmr_handle handle, cmr_u32 param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
#ifdef SBS_MODE_SENSOR
    char value1[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.sbs.mode", value1, "0");
    if (!strcmp(value1, "slave")) {
#ifdef SBS_SENSOR_FRONT
        sns_ops = sns_drv_cntx[3].sensor_info_ptr->sns_ops;
#else
        sns_ops = sns_drv_cntx[2].sensor_info_ptr->sns_ops;
#endif
    }
#endif
    if (!(sns_ops && sns_ops->write_ae_value)) {
        SENSOR_LOGE("write_ae_value is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->write_ae_value(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_read_aec_info(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (!(sns_ops && sns_ops->read_aec_info)) {
        SENSOR_LOGE("ex_write_exp is NULL,return");
        return SENSOR_FAIL;
    }
    ret = sns_ops->read_aec_info(sensor_cxt->sns_ic_drv_handle, param);

    return ret;
}

static cmr_int sensor_ic_write_multi_ae_info(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_handle sensor_handle;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;
    struct sensor_multi_ae_info *ae_info = (struct sensor_multi_ae_info *)param;
    struct sensor_aec_reg_info aec_reg_info[CAMERA_ID_MAX];
    struct sensor_muti_aec_i2c_tag muti_aec_info;
    struct sensor_aec_i2c_tag *aec_info;
    cmr_u32 i, k, size = 0;
    cmr_u32 count = 0;
    cmr_u32 cnt = 0;
    struct sensor_reg_tag msettings[AEC_I2C_SETTINGS_MAX];
    struct sensor_reg_tag ssettings[AEC_I2C_SETTINGS_MAX];
    // TODO optimize this later
    struct sensor_reg_tag ssettings_2[AEC_I2C_SETTINGS_MAX];
    struct sensor_reg_tag *settings = NULL;
    cmr_u16 sensor_id[AEC_I2C_SENSOR_MAX];
    cmr_u16 i2c_slave_addr[AEC_I2C_SENSOR_MAX];
    cmr_u16 addr_bits_type[AEC_I2C_SENSOR_MAX];
    cmr_u16 data_bits_type[AEC_I2C_SENSOR_MAX];
    struct phySensorInfo *phyPtr = phy_sensor_info_list;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);
    SENSOR_DRV_CHECK_ZERO(param);

    memset(&aec_reg_info, 0x00, sizeof(aec_reg_info));
    memset(&muti_aec_info, 0x00, sizeof(muti_aec_info));
    memset(&aec_info, 0x00, sizeof(aec_info));

    count = ae_info[0].count;
    for (i = 0; i < count; i++) {
        sensor_handle = ae_info[i].handle;
        aec_reg_info[i].exp.exposure = ae_info[i].exp.exposure;
        aec_reg_info[i].exp.dummy = ae_info[i].exp.dummy;
        aec_reg_info[i].exp.size_index = ae_info[i].exp.size_index;
        aec_reg_info[i].gain = ae_info[i].gain;
        aec_reg_info[i].exp.exp_time = ae_info[i].exp.exp_time;
        SENSOR_LOGV("read aec i %u count %d handle %p", i, count,
                    sensor_handle);
        ret = sensor_ic_read_aec_info(sensor_handle, (&aec_reg_info[i]));
        if (ret != SENSOR_SUCCESS)
            return ret;
    }

    muti_aec_info.sensor_id = sensor_id;
    muti_aec_info.i2c_slave_addr = i2c_slave_addr;
    muti_aec_info.addr_bits_type = addr_bits_type;
    muti_aec_info.data_bits_type = data_bits_type;

    cnt = count > AEC_I2C_SENSOR_MAX ? AEC_I2C_SENSOR_MAX : count;
    for (k = 0; k < cnt; k++) {
        size = 0;
        aec_info = aec_reg_info[k].aec_i2c_info_out;
        sensor_id[k] = (phyPtr + ae_info[k].camera_id)->slotId;
        muti_aec_info.id_size = cnt;
        i2c_slave_addr[k] = aec_info->slave_addr;
        muti_aec_info.i2c_slave_len = cnt;
        addr_bits_type[k] = (cmr_u16)aec_info->addr_bits_type;
        muti_aec_info.addr_bits_type_len = cnt;
        data_bits_type[k] = (cmr_u16)aec_info->data_bits_type;
        muti_aec_info.data_bits_type_len = cnt;
        if (k == 0) {
            muti_aec_info.master_i2c_tab = msettings;
            settings = msettings;
        } else if (k == 1) {
            muti_aec_info.slave_i2c_tab = ssettings;
            settings = ssettings;
        } else if (k == 2) {
            muti_aec_info.slave_i2c_tab_2 = ssettings_2;
            settings = ssettings_2;
        }

        for (i = 0; i < aec_info->frame_length->size; i++) {
            settings[size].reg_addr =
                aec_info->frame_length->settings[i].reg_addr;
            settings[size++].reg_value =
                aec_info->frame_length->settings[i].reg_value;
        }
        for (i = 0; i < aec_info->shutter->size; i++) {
            settings[size].reg_addr = aec_info->shutter->settings[i].reg_addr;
            settings[size++].reg_value =
                aec_info->shutter->settings[i].reg_value;
        }
        for (i = 0; i < aec_info->again->size; i++) {
            settings[size].reg_addr = aec_info->again->settings[i].reg_addr;
            settings[size++].reg_value = aec_info->again->settings[i].reg_value;
        }
        for (i = 0; i < aec_info->dgain->size; i++) {
            settings[size].reg_addr = aec_info->dgain->settings[i].reg_addr;
            settings[size++].reg_value = aec_info->dgain->settings[i].reg_value;
        }
        if (k == 0) {
            muti_aec_info.msize =
                aec_info->shutter->size + aec_info->again->size +
                aec_info->dgain->size + aec_info->frame_length->size;
        } else if (k == 1) {
            muti_aec_info.ssize =
                aec_info->shutter->size + aec_info->again->size +
                aec_info->dgain->size + aec_info->frame_length->size;
        } else if (k == 2) {
            muti_aec_info.ssize_2 =
                aec_info->shutter->size + aec_info->again->size +
                aec_info->dgain->size + aec_info->frame_length->size;
        }
    }

    ret = sensor_muti_i2c_write(handle, &muti_aec_info);
    if (ret != SENSOR_SUCCESS)
        return ret;

    return ret;
}

static cmr_int sensor_ic_parse_ebd_data(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_handle sensor_handle;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;
    SENSOR_VAL_T val;
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    val.type = SENSOR_VAL_TYPE_PARSE_EBD_DATA;
    val.pval = param;

    SENSOR_LOGV("ebd ptr %p\n", param);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops)
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                            (cmr_uint)&val);
    return ret;
}

static cmr_int sensor_ic_get_cct_data(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_handle sensor_handle;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

#ifdef TARGET_CAMERA_SENSOR_CCT_TCS3430
    tcs3430_read_data(param);
#elif  TARGET_CAMERA_SENSOR_CCT_SENSORHUB
    sensorhub_read_data(param);
#endif

    return ret;
}

static cmr_int sensor_ic_get_3dnr_threshold(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_handle sensor_handle;
    struct sensor_ic_ops *sns_ops = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;
    SENSOR_VAL_T val;
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    val.type = SENSOR_VAL_TYPE_GET_3DNR_THRESHOLD;
    val.pval = param;

    SENSOR_LOGV("3dnr threshold ptr %p\n", param);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops)
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                            (cmr_uint)&val);
    return ret;
}

cmr_int sensor_ic_ioctl(cmr_handle handle, enum sns_cmd cmd, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;

    SENSOR_DRV_CHECK_ZERO(handle);

    switch (cmd) {
    case CMD_SNS_IC_WRITE_MULTI_AE:
        ret = sensor_ic_write_multi_ae_info(handle, param);
        break;
    case CMD_SNS_IC_GET_EBD_PARSE_DATA:
        ret = sensor_ic_parse_ebd_data(handle, param);
        break;
    case CMD_SNS_IC_GET_CCT_DATA:
        ret = sensor_ic_get_cct_data(handle, param);
        break;
    case CMD_SNS_IC_GET_3DNR_THRESHOLD:
        ret = sensor_ic_get_3dnr_threshold(handle, param);
        break;
    default:
        break;
    }

    return ret;
}

/*--------------------------COMMON INTERFACE-----------------------------*/

/** sensor_drv_ioctl:
 *  @sns_module_handle: sensor module handle.
 *  @cmd:   command that you want to do.
 *  @param: parameter data you want to transmit.
 *
 * NOTE:
 * This is an extended interface which contain sensor_ic,otp_drv,af_drv
 * if you want add some sub commands.please add them in
 *common/inc/cmr_sensor_info.h
 *
 * Return:
 * SENSOR_SUCCESS : ioctl success
 * SENSOR_FAIL : ioctl failed
 **/
cmr_int sensor_drv_ioctl(cmr_handle handle, enum sns_cmd cmd, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = PNULL;
    sensor_otp_ops_t *otp_ops = PNULL;
    SENSOR_MATCH_T *module = PNULL;
    SENSOR_DRV_CHECK_ZERO(handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)handle;
    SENSOR_LOGV("E:cmd:0x%x", cmd);
    module = (SENSOR_MATCH_T *)sensor_cxt->current_module;
    SENSOR_DRV_CHECK_PTR(module);

    switch (cmd >> 8) {
    case CMD_SNS_OTP:
        if (module && (module->otp_drv_info.otp_drv_entry) &&
            sensor_cxt->otp_drv_handle) {
            otp_ops = &module->otp_drv_info.otp_drv_entry->otp_ops;
            if (otp_ops && otp_ops->sensor_otp_ioctl) {
                ret = otp_ops->sensor_otp_ioctl(sensor_cxt->otp_drv_handle, cmd,
                                                param);
                if (ret != SENSOR_SUCCESS)
                    return ret;
            }
        } else {
            ret = SENSOR_FAIL;
            SENSOR_LOGE("otp driver,return directly");
        }
        break;
    case CMD_SNS_IC:
        ret = sensor_ic_ioctl(handle, cmd, param);
        break;
    case CMD_SNS_AF:
        if (module && module->af_dev_info.af_drv_entry &&
            sensor_cxt->af_drv_handle) {
            af_ops = &module->af_dev_info.af_drv_entry->af_ops;
            if (af_ops && af_ops->ioctl) {
                ret = af_ops->ioctl(sensor_cxt->af_drv_handle, cmd, param);
                if (SENSOR_SUCCESS != ret)
                    return SENSOR_FAIL;
            }
        } else {
            ret = SENSOR_FAIL;
            SENSOR_LOGE("af driver,return directly");
        }
        break;
    default:
        break;
    }

    SENSOR_LOGV("X:");
    return ret;
}

void sensor_drv_get_camId_list_info(int *active_cam_list, int *active_cam_num) {
    SENSOR_LOGD("E:");
    int i = 0;
    struct phySensorInfo *phyPtr = NULL;
    int physical_num = 0;
    int *cam_list_pointer = active_cam_list;
    phyPtr = phy_sensor_info_list;

    for (i = 0; i < SENSOR_ID_MAX; i++) {
        if ((phyPtr + i)->slotId == SENSOR_ID_INVALID)
            continue;
        physical_num++;
        *(cam_list_pointer++) = (phyPtr + i)->slotId;
    }
    *active_cam_num = physical_num;

    SENSOR_LOGD("active_cam_num:%d", *active_cam_num);

    SENSOR_LOGD("X:");
}

/*
sensor_drv_u optimize
================================================================
*/
static void sensor_drv_print_slot_list_info(void) {
    cmr_int i = 0;
    struct slotSensorInfo *slotPtr = slot_sensor_info_list;

    for (i = 0; i < SENSOR_ID_MAX; i++) {
        if ((slotPtr + i)->slotId == SENSOR_ID_INVALID)
            continue;
        SENSOR_LOGI("slotId %d, sensor_name %s\n", (slotPtr + i)->slotId,
                    (slotPtr + i)->sensor_name);
    }
}

static void sensor_drv_print_phy_list_info(void) {
    int i = 0;
    struct phySensorInfo *phyPtr = phy_sensor_info_list;
    struct camera_device_manager *devPtr = &camera_dev_manger;

    for (i = 0; i < devPtr->physical_num; i++) {
        SENSOR_LOGI(
            "phyId %d, slotId %d, sensor_name %s, sensor_role_code 0x%x\n",
            (phyPtr + i)->phyId, (phyPtr + i)->slotId,
            (phyPtr + i)->sensor_name, (phyPtr + i)->sensor_role_code);
    }
}

static void sensor_drv_print_logical_list_info(void) {
    int i, j;
    struct logicalSensorInfo *logicalPtr = logical_sensor_info_list;
    struct camera_device_manager *devPtr = &camera_dev_manger;

    for (i = 0; i < devPtr->logical_num; i++) {
        SENSOR_LOGI("logicalId %d, multiCameraId %d, physicalNum %d\n",
                    (logicalPtr + i)->logicalId,
                    (logicalPtr + i)->multiCameraId,
                    (logicalPtr + i)->physicalNum);
        for (j = 0; j < (logicalPtr + i)->physicalNum; j++) {
            SENSOR_LOGI("----phyId %d, sensor_role %d\n",
                        (logicalPtr + i)->phyIdGroup[j].phyId,
                        (logicalPtr + i)->phyIdGroup[j].sensor_role);
        }
    }
}

static void sensor_drv_print_logical_camera_list_info(void) {
    int i, j;
    struct logicalCameraInfo *logicalCamPtr = logical_camera_info_list;
    struct camera_device_manager *devPtr = &camera_dev_manger;

    for (i = 0; i < devPtr->logical_cam_num; i++) {
        SENSOR_LOGI("logicalCameraId %d, multiCameraMode %d, sensorNum %d",
                    (logicalCamPtr + i)->logicalCameraId,
                    (logicalCamPtr + i)->multiCameraMode,
                    (logicalCamPtr + i)->sensorNum);
        for (j = 0; j < (logicalCamPtr + i)->sensorNum; j++) {
            SENSOR_LOGI("----sensorId %d",
                        (logicalCamPtr + i)->sensorIdGroup[j]);
        }
    }
}

static cmr_int sensor_drv_sensor_info_list_init(void) {
    cmr_int i = 0;
    struct slotSensorInfo *slotPtr = slot_sensor_info_list;
    struct phySensorInfo *phyPtr = phy_sensor_info_list;
    struct logicalSensorInfo *logicalPtr = logical_sensor_info_list;
    struct logicalCameraInfo *logicalCamPtr = logical_camera_info_list;

    for (i = 0; i < SENSOR_ID_MAX; i++) {
        // slot sensor list
        (slotPtr + i)->slotId = SENSOR_ID_INVALID;
        (slotPtr + i)->sensor_index = 0xff;
        memset((slotPtr + i)->sensor_name, 0, SENSOR_NAME_LEN);
        // physical sensor list
        (phyPtr + i)->phyId = SENSOR_ID_INVALID;
        (phyPtr + i)->slotId = SENSOR_ID_INVALID;
    }
    // logical sensor list
    for (i = 0; i < LOGICAL_SENSOR_ID_MAX; i++) {
        (logicalPtr + i)->logicalId = SENSOR_ID_INVALID;
        (logicalPtr + i)->multiCameraId = SENSOR_ID_INVALID;
        (logicalCamPtr + i)->logicalCameraId = SENSOR_ID_INVALID;
    }

    return 0;
}

static cmr_int
sensor_drv_create_slot_sensor_info(struct sensor_drv_context *sensor_cxt,
                                   cmr_u32 slot_id) {
    struct slotSensorInfo *slotPtr = slot_sensor_info_list + slot_id;
    SENSOR_MATCH_T *module = (SENSOR_MATCH_T *)sensor_cxt->current_module;

    slotPtr->slotId = slot_id;
    slotPtr->sensor_index = *sensor_cxt->sensor_index;
    strcpy(slotPtr->sensor_name, module->sn_name);

    return 0;
}

static void sensor_drv_special_phy_sensor_info(PHYSICAL_SENSOR_INFO_T *phyPtr,
                                               cmr_u32 slot_id) {

    if (slot_id == SENSOR_SUB2 &&
        !strcmp(phyPtr->sensor_name, "ov8856_shine")) {
        phyPtr->face_type = SNS_FACE_BACK;
        phyPtr->angle = 90;
    }
}

static cmr_int sensor_drv_get_long_exp_info(struct sensor_drv_context
                   *sensor_cxt, struct phySensorInfo *phyPtr)
{
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_VAL_T val;
    struct sensor_ic_ops *sns_ops = PNULL;
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    struct sensor_ex_info sn_ex_info_slv;
    memset(&sn_ex_info_slv, 0, sizeof(struct sensor_ex_info));

    val.type = SENSOR_VAL_TYPE_GET_STATIC_INFO;
    val.pval = &sn_ex_info_slv;

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops) {
	if(!sensor_cxt->sns_ic_drv_handle)
		sensor_ic_create(sensor_cxt, sensor_cxt->slot_id);
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                           (cmr_uint)&val);
      if(!ret) {
        for(int i = 0; i < sn_ex_info_slv.long_expose_modes_size; i++) {
            phyPtr->long_expose_modes[i] = sn_ex_info_slv.long_expose_modes[i];
        }
        phyPtr->long_expose_modes_size = sn_ex_info_slv.long_expose_modes_size;
        phyPtr->longExp_need_switch_setting = sn_ex_info_slv.longExp_need_switch_setting;
        if (phyPtr->longExp_need_switch_setting) {
            for(int i = 0; i < sn_ex_info_slv.long_exposure_setting_size; i++) {
                phyPtr->long_exposure_setting[i] = sn_ex_info_slv.long_exposure_setting[i];
            }
            phyPtr->long_exposure_threshold = sn_ex_info_slv.long_exposure_threshold;
        }
        phyPtr->longExp_valid_frame_num = sn_ex_info_slv.longExp_valid_frame_num;
      } else {
         SENSOR_LOGE("get sensor ex info failed");
         return -1;
      }
    } else {
       SENSOR_LOGE("sns_ops null");
       return -1;
    }
    return 0;
}

static cmr_int
sensor_drv_create_phy_sensor_info(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 slot_id, cmr_u32 phy_id) {
    struct slotSensorInfo *slotPtr = NULL;
    struct phySensorInfo *phyPtr = NULL;
    SENSOR_MATCH_T *module = sensor_cxt->current_module;

#ifdef CAMERA_CONFIG_SENSOR_NUM
    phy_id = slot_id;
#endif
    slotPtr = slot_sensor_info_list + slot_id;
    phyPtr = phy_sensor_info_list + phy_id;

    phyPtr->slotId = slot_id;
    phyPtr->phyId = phy_id;
    strcpy(phyPtr->sensor_name, slotPtr->sensor_name);

    SENSOR_LOGV("phy_id %d, slot_id %d, sensor_name %s", phy_id, slot_id,
                phyPtr->sensor_name);

    // config sensor attribute
    sensor_cxt->sensor_info_ptr->focus_eb =
        (module->af_dev_info.af_drv_entry != NULL);
    phyPtr->focus_eb = sensor_cxt->sensor_info_ptr->focus_eb;

    if (sensor_cxt->fov_info.physical_size[0] > 0 &&
        sensor_cxt->fov_info.physical_size[1] > 0 &&
        sensor_cxt->fov_info.focal_lengths > 0) {
        memcpy(&phyPtr->fov_info, &sensor_cxt->fov_info,
               sizeof(sensor_cxt->fov_info));
    }

    if (sensor_cxt->sensor_min_exp) {
        phyPtr->sensor_min_exp = sensor_cxt->sensor_min_exp;
        phyPtr->sensor_max_exp = sensor_cxt->sensor_max_exp;
        SENSOR_LOGD("sensor_range (%lld, %lld)",
                 phyPtr->sensor_min_exp, phyPtr->sensor_max_exp);
    }

    if (sensor_cxt->fov_angle > 0) {
        phyPtr->fov_angle = sensor_cxt->fov_angle;
    }

    phyPtr->source_width_max = sensor_cxt->sensor_info_ptr->source_width_max;
    phyPtr->source_height_max = sensor_cxt->sensor_info_ptr->source_height_max;

    phyPtr->image_format = sensor_cxt->sensor_info_ptr->image_format;
    phyPtr->sensor_type = sensor_cxt->sensor_type;
    phyPtr->data_type = 0;
    phyPtr->pdaf_supported = sensor_cxt->static_info->pdaf_supported;
    if(sensor_cxt->static_info->long_expose_supported) {
       phyPtr->long_expose_supported = sensor_cxt->static_info->long_expose_supported;
       sensor_drv_get_long_exp_info(sensor_cxt, phyPtr);
       for(int i = 0;i < phyPtr->long_expose_modes_size;i++) {
           SENSOR_LOGD("long_expose_modes[%d]=%f, long_expose_supported=%d, long_expose_modes_size=%d",
               i, phyPtr->long_expose_modes[i], phyPtr->long_expose_supported, phyPtr->long_expose_modes_size);
       }
    }
    phyPtr->sensor_role_code = sensor_cxt->xml_info->cfgPtr->sensor_role_code;
    phyPtr->face_type = sensor_cxt->xml_info->cfgPtr->facing;
    phyPtr->angle = sensor_cxt->xml_info->cfgPtr->orientation;
    phyPtr->resource_cost = sensor_cxt->xml_info->cfgPtr->resource_cost;
    memcpy(phyPtr->conflicting_devices, sensor_cxt->xml_info->cfgPtr->conflicting_devices, sizeof(sensor_cxt->xml_info->cfgPtr->conflicting_devices));
    phyPtr->conflicting_devices_length = sensor_cxt->xml_info->cfgPtr->conflicting_devices_length;
    phyPtr->mono_sensor = sensor_cxt->mono_sensor;
    phyPtr->f_num = sensor_cxt->static_info->f_num;
    phyPtr->mim_focus_distance = sensor_cxt->static_info->min_focal_distance;
    phyPtr->start_offset_time = sensor_cxt->static_info->start_offset_time;
    SENSOR_LOGD("f_num:%f,mim_focus_distance:%f,start offset time:%lld",
                 phyPtr->f_num,phyPtr->mim_focus_distance,phyPtr->start_offset_time);

    phyPtr->module_vendor_id = sensor_cxt->module_vendor_id;
    phyPtr->otp_version = sensor_cxt->otp_version;

    // special phyiscal sensor info overwrite
    sensor_drv_special_phy_sensor_info(phyPtr, slot_id);

    return 0;
}

static int sensor_drv_create_logical_sensor_info(int physical_num) {
    struct phySensorInfo *phyPtr = phy_sensor_info_list;
    struct logicalSensorInfo *logicalPtr = logical_sensor_info_list;
    int i = 0;
    int logical_num = 0;
    int phyId[SENSOR_ID_MAX];

    // single camera feature
    /* single sensor config logical list info
     * logical id is equal to physical id for single sensor
     */
    for (i = 0; i < physical_num; i++) {
        (logicalPtr + i)->logicalId = (phyPtr + i)->phyId;
        (logicalPtr + i)->multiCameraId = SENSOR_ID_INVALID;
        (logicalPtr + i)->multiCameraMode = MODE_SINGLE_CAMERA;
        (logicalPtr + i)->physicalNum = 1;
        (logicalPtr + i)->phyIdGroup[0].phyId = (phyPtr + i)->phyId;
        /* NOTE:
         * single sensor role's code_value must be equal to its enum sensor_role in cmr_common.h
         * for example,
         * single_ir's code_value = sensor_role_code & 0xf == 1,
         * in enum sensor_role, SENSOR_ROLE_SINGLE_IR == 1.
         */
        (logicalPtr + i)->phyIdGroup[0].sensor_role =
            (phyPtr + i)->sensor_role_code & 0xf;
        (logicalPtr + i)->face_type = (phyPtr + i)->face_type;
    }

    // multi camera feature
    i = physical_num;
    logicalPtr += i;

    // auto create logical info for dualcam bokeh & Portrait
    memset(phyId, SENSOR_ID_INVALID, SENSOR_ID_MAX);
    phyId[0] = sensorGetPhyId4Role(SENSOR_ROLE_DUALCAM_MASTER, SNS_FACE_BACK);
    phyId[1] = sensorGetPhyId4Role(SENSOR_ROLE_DUALCAM_SLAVE, SNS_FACE_BACK);
    if (phyId[0] != SENSOR_ID_INVALID && phyId[1] != SENSOR_ID_INVALID) {
        logicalPtr->logicalId = i;
        logicalPtr->multiCameraId = SPRD_BLUR_ID;
        logicalPtr->multiCameraMode = MODE_BOKEH;
        logicalPtr->physicalNum = 2;
        logicalPtr->phyIdGroup[0].phyId = phyId[0];
        logicalPtr->phyIdGroup[0].sensor_role = SENSOR_ROLE_DUALCAM_MASTER;
        logicalPtr->phyIdGroup[1].phyId = phyId[1];
        logicalPtr->phyIdGroup[1].sensor_role = SENSOR_ROLE_DUALCAM_SLAVE;
        logicalPtr->face_type = SNS_FACE_BACK;
        SENSOR_LOGD(
            "create back dualcam bokeh multiCameraId %d, phyId M %d, S %d",
            SPRD_BLUR_ID, phyId[0], phyId[1]);
        logicalPtr++;
        i++;
    } else {
        SENSOR_LOGD("sensor not support back dualcam bokeh, will not create "
                    "logical info, M %d, S %d",
                    phyId[0], phyId[1]);
    }

    if (phyId[0] != SENSOR_ID_INVALID && phyId[1] != SENSOR_ID_INVALID) {
        logicalPtr->logicalId = i;
        logicalPtr->multiCameraId = SPRD_PORTRAIT_ID;
        logicalPtr->multiCameraMode = MODE_PORTRAIT;
        logicalPtr->physicalNum = 2;
        logicalPtr->phyIdGroup[0].phyId = phyId[0];
        logicalPtr->phyIdGroup[0].sensor_role = SENSOR_ROLE_DUALCAM_MASTER;
        logicalPtr->phyIdGroup[1].phyId = phyId[1];
        logicalPtr->phyIdGroup[1].sensor_role = SENSOR_ROLE_DUALCAM_SLAVE;
        logicalPtr->face_type = SNS_FACE_BACK;
        SENSOR_LOGD(
            "create back dualcam portrait multiCameraId %d, phyId M %d, S %d",
            SPRD_PORTRAIT_ID, phyId[0], phyId[1]);
        logicalPtr++;
        i++;
    } else {
        SENSOR_LOGD("sensor not support back dualcam portrait, will not create "
                    "logical info, M %d, S %d",
                    phyId[0], phyId[1]);
    }

    // auto create logical info for opticszoom & & fov_fusion & dual_view_video
    memset(phyId, SENSOR_ID_INVALID, SENSOR_ID_MAX);
    phyId[0] =
        sensorGetPhyId4Role(SENSOR_ROLE_MULTICAM_SUPERWIDE, SNS_FACE_BACK);
    phyId[1] = sensorGetPhyId4Role(SENSOR_ROLE_MULTICAM_WIDE, SNS_FACE_BACK);
    phyId[2] = sensorGetPhyId4Role(SENSOR_ROLE_MULTICAM_TELE, SNS_FACE_BACK);
    if (phyId[0] != SENSOR_ID_INVALID && phyId[1] != SENSOR_ID_INVALID &&
        phyId[2] != SENSOR_ID_INVALID) {
        logicalPtr->logicalId = i;
        logicalPtr->multiCameraId = SPRD_MULTI_CAMERA_ID;
        logicalPtr->multiCameraMode = MODE_MULTI_CAMERA;
        logicalPtr->physicalNum = 3;
        logicalPtr->phyIdGroup[0].phyId = phyId[0];
        logicalPtr->phyIdGroup[0].sensor_role = SENSOR_ROLE_MULTICAM_SUPERWIDE;
        logicalPtr->phyIdGroup[1].phyId = phyId[1];
        logicalPtr->phyIdGroup[1].sensor_role = SENSOR_ROLE_MULTICAM_WIDE;
        logicalPtr->phyIdGroup[2].phyId = phyId[2];
        logicalPtr->phyIdGroup[2].sensor_role = SENSOR_ROLE_MULTICAM_TELE;
        logicalPtr->face_type = SNS_FACE_BACK;
        SENSOR_LOGD("create back SW+W+T opticszoom multiCameraId %d, phyId SW "
                    "%d, W %d, T %d",
                    SPRD_MULTI_CAMERA_ID, phyId[0], phyId[1], phyId[2]);
        logicalPtr++;
        i++;
    } else if (phyId[0] != SENSOR_ID_INVALID && phyId[1] != SENSOR_ID_INVALID &&
               phyId[2] == SENSOR_ID_INVALID) {
        logicalPtr->logicalId = i;
        logicalPtr->multiCameraId = SPRD_MULTI_CAMERA_ID;
        logicalPtr->multiCameraMode = MODE_MULTI_CAMERA;
        logicalPtr->physicalNum = 2;
        logicalPtr->phyIdGroup[0].phyId = phyId[0];
        logicalPtr->phyIdGroup[0].sensor_role = SENSOR_ROLE_MULTICAM_SUPERWIDE;
        logicalPtr->phyIdGroup[1].phyId = phyId[1];
        logicalPtr->phyIdGroup[1].sensor_role = SENSOR_ROLE_MULTICAM_WIDE;
        logicalPtr->face_type = SNS_FACE_BACK;
        SENSOR_LOGD(
            "create back SW+W opticszoom multiCameraId %d, phyId SW %d, W %d",
            SPRD_MULTI_CAMERA_ID, phyId[0], phyId[1]);
        logicalPtr++;
        i++;
    } else if (phyId[0] == SENSOR_ID_INVALID && phyId[1] != SENSOR_ID_INVALID &&
               phyId[2] != SENSOR_ID_INVALID) {
        SENSOR_LOGD(
            "hal multiCamera not support only W+T opticszoom temporarily, "
            "will not create logical info, SW %d, W %d, T %d",
            phyId[0], phyId[1], phyId[2]);
    } else {
        SENSOR_LOGD("sensor not support opticszoom, will not create logical "
                    "info, SW %d, W %d, T %d",
                    phyId[0], phyId[1], phyId[2]);
    }

    if (phyId[1] != SENSOR_ID_INVALID && phyId[2] != SENSOR_ID_INVALID) {
        logicalPtr->logicalId = i;
        logicalPtr->multiCameraId = SPRD_FOV_FUSION_ID;
        logicalPtr->multiCameraMode = MODE_FOV_FUSION;
        logicalPtr->physicalNum = 2;
        logicalPtr->phyIdGroup[1].phyId = phyId[1];
        logicalPtr->phyIdGroup[1].sensor_role = SENSOR_ROLE_MULTICAM_WIDE;
        logicalPtr->phyIdGroup[2].phyId = phyId[2];
        logicalPtr->phyIdGroup[2].sensor_role = SENSOR_ROLE_MULTICAM_TELE;
        logicalPtr->face_type = SNS_FACE_BACK;
        SENSOR_LOGD(
            "create back W+T fov_fusion multiCameraId %d, phyId W %d, T %d",
            SPRD_FOV_FUSION_ID, phyId[1], phyId[2]);
        logicalPtr++;
        i++;
    } else {
        SENSOR_LOGD("sensor not support fov_fusion, will not create logical "
                    "info, phyId W %d, T %d",
                    phyId[1], phyId[2]);
    }

    if (phyId[0] != SENSOR_ID_INVALID && phyId[2] != SENSOR_ID_INVALID) {
        logicalPtr->logicalId = i;
        logicalPtr->multiCameraId = SPRD_DUAL_VIEW_VIDEO_ID;
        logicalPtr->multiCameraMode = MODE_DUAL_VIEW_VIDEO;
        logicalPtr->physicalNum = 2;
        logicalPtr->phyIdGroup[0].phyId = phyId[0];
        logicalPtr->phyIdGroup[0].sensor_role = SENSOR_ROLE_MULTICAM_SUPERWIDE;
        logicalPtr->phyIdGroup[2].phyId = phyId[2];
        logicalPtr->phyIdGroup[2].sensor_role = SENSOR_ROLE_MULTICAM_TELE;
        logicalPtr->face_type = SNS_FACE_BACK;
        SENSOR_LOGD("create back SW+T dual_view_video multiCameraId %d, phyId "
                    "SW %d, T %d",
                    SPRD_DUAL_VIEW_VIDEO_ID, phyId[0], phyId[2]);
        logicalPtr++;
        i++;
    } else {
        SENSOR_LOGD("sensor not support dual_view_video, will not create "
                    "logical info, phyId SW %d, T %d",
                    phyId[0], phyId[2]);
    }

    // auto create logical info for 3d structure light
    memset(phyId, SENSOR_ID_INVALID, SENSOR_ID_MAX);
    phyId[0] = sensorGetPhyId4Role(SENSOR_ROLE_STL3D_RGB, SNS_FACE_FRONT);
    phyId[1] = sensorGetPhyId4Role(SENSOR_ROLE_STL3D_IR_LEFT, SNS_FACE_FRONT);
    phyId[2] = sensorGetPhyId4Role(SENSOR_ROLE_STL3D_IR_RIGHT, SNS_FACE_FRONT);
    if (phyId[0] != SENSOR_ID_INVALID && phyId[1] != SENSOR_ID_INVALID &&
        phyId[2] != SENSOR_ID_INVALID) {
        logicalPtr->logicalId = i;
        logicalPtr->multiCameraId = SPRD_3D_FACE_ID;
        logicalPtr->multiCameraMode = MODE_3D_FACE;
        logicalPtr->physicalNum = 3;
        logicalPtr->phyIdGroup[0].phyId = phyId[0];
        logicalPtr->phyIdGroup[0].sensor_role = SENSOR_ROLE_STL3D_RGB;
        logicalPtr->phyIdGroup[1].phyId = phyId[1];
        logicalPtr->phyIdGroup[1].sensor_role = SENSOR_ROLE_STL3D_IR_LEFT;
        logicalPtr->phyIdGroup[2].phyId = phyId[2];
        logicalPtr->phyIdGroup[2].sensor_role = SENSOR_ROLE_STL3D_IR_RIGHT;
        logicalPtr->face_type = SNS_FACE_FRONT;
        SENSOR_LOGD("create front 3d structure light multiCameraId %d, phyId "
                    "RGB %d, IR_L %d IR_R %d",
                    SPRD_3D_FACE_ID, phyId[0], phyId[1], phyId[2]);
        logicalPtr++;
        i++;
    } else {
        SENSOR_LOGD("sensor not support 3d structure light, will not create "
                    "logical info, phyId RGB %d, IR_L %d IR_R %d",
                    phyId[0], phyId[1], phyId[2]);
    }

    logical_num = i;

    return logical_num;
}

static int sensor_drv_create_logical_camera_info(void) {
    struct logicalSensorInfo *logicalSnsPtr = logical_sensor_info_list;
    struct logicalCameraInfo *logicalCamPtr = logical_camera_info_list;
    struct camera_device_manager *devPtr = &camera_dev_manger;
    int logical_cam_num = 0;
    int i = 0, j = 0, k = 0;

    // single camera feature
    for (i = 0; i < devPtr->physical_num; i++) {
        (logicalCamPtr + i)->logicalCameraId = (logicalSnsPtr + i)->logicalId;
        (logicalCamPtr + i)->multiCameraMode = MODE_SINGLE_CAMERA;
        (logicalCamPtr + i)->sensorNum = 1;
        (logicalCamPtr + i)->sensorIdGroup[0] =
            (logicalSnsPtr + i)->phyIdGroup[0].phyId;
        (logicalCamPtr + i)->face_type = (logicalSnsPtr + i)->face_type;
    }

    // multi camera feature bokeh & opticszoom
    for (j = devPtr->physical_num; j < devPtr->logical_num; j++) {
        if ((logicalSnsPtr + j)->multiCameraMode == MODE_BOKEH ||
            (logicalSnsPtr + j)->multiCameraMode == MODE_MULTI_CAMERA) {
            (logicalCamPtr + i)->logicalCameraId = i;
            (logicalCamPtr + i)->multiCameraMode =
                (logicalSnsPtr + j)->multiCameraMode;
            (logicalCamPtr + i)->sensorNum = (logicalSnsPtr + j)->physicalNum;
            for (k = 0; k < (logicalSnsPtr + j)->physicalNum; k++) {
                (logicalCamPtr + i)->sensorIdGroup[k] =
                    (logicalSnsPtr + j)->phyIdGroup[k].phyId;
            }
            (logicalCamPtr + i)->face_type = (logicalSnsPtr + j)->face_type;
            i++;
        }
    }

    logical_cam_num = i;
    return logical_cam_num;
}

static cmr_int
sensor_drv_get_dynamic_info(struct sensor_drv_context *sensor_cxt) {

    sensor_cxt->sensor_type = sensor_drv_get_sensor_type(sensor_cxt);
    sensor_drv_get_fov_info(sensor_cxt);
    sensor_drv_get_module_otp_data(sensor_cxt);
    sensor_cxt->static_info =
        sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_STATIC_INFO);
    sensor_drv_get_tuning_param(sensor_cxt);
    sensor_save_pdaf_info(sensor_cxt);
    return 0;
}

static cmr_int
sensor_drv_get_tuning_param(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_MATCH_T *sns_module = NULL;
    struct xml_camera_cfg_info *camera_cfg;
    struct tuning_param_lib *libTuningPtr =
        &tuning_lib_mngr[sensor_cxt->slot_id];
    sns_module = (SENSOR_MATCH_T *)sensor_cxt->current_module;
    param_input_t tuning_param_input;
    char default_tuning_para_name[SENSOR_NAME_LEN] = {0};

    camera_cfg = sensor_cxt->xml_info;
    sensor_drv_xml_parse_tuning_param_info(camera_cfg);

    if (!strcmp(camera_cfg->cfgPtr->tuning_info.tuning_para_name, "default")) {
        cmr_bzero(&tuning_param_input, sizeof(param_input_t));

        strcpy(tuning_param_input.module_cfg.sensor_basic_info.sensor_name,
               camera_cfg->cfgPtr->sensor_name);

        /* size_info[0]: fullsize */
        tuning_param_input.module_cfg.sensor_cfg.sensor_settings_info
            .size_info[0]
            .size_w = sensor_cxt->sensor_info_ptr->source_width_max;
        tuning_param_input.module_cfg.sensor_cfg.sensor_settings_info
            .size_info[0]
            .size_h = sensor_cxt->sensor_info_ptr->source_height_max;

        snprintf(default_tuning_para_name, SENSOR_NAME_LEN, "default_id_%d",
                 sensor_cxt->slot_id);
        ret = sensor_drv_tuning_load_default_library(
            &tuning_param_input, default_tuning_para_name, libTuningPtr);
    } else {
        ret = sensor_drv_tuning_load_library(
            camera_cfg->cfgPtr->tuning_info.tuning_para_name, libTuningPtr);
    }

    if (!ret) {
        sns_module->sensor_info->raw_info_ptr =
            (struct sensor_raw_info **)&libTuningPtr->raw_info_ptr;
    }

    return 0;
}

static cmr_int
sensor_drv_get_module_otp_data(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_MATCH_T *sns_module = NULL;
    struct xml_camera_cfg_info *camera_cfg;
    struct otp_drv_lib *libOtpPtr = &otp_lib_mngr[sensor_cxt->slot_id];
    struct vcm_drv_lib *libVcmPtr = &vcm_lib_mngr[sensor_cxt->slot_id];
    struct module_info_t *module_info = PNULL;
    cmr_u8 *awb_src_dat, *af_src_dat;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sns_module = (SENSOR_MATCH_T *)sensor_cxt->current_module;
    SENSOR_DRV_CHECK_ZERO(sns_module);

    camera_cfg = sensor_cxt->xml_info;
    sensor_drv_xml_parse_vcm_info(camera_cfg);
    ret = sensor_drv_vcm_load_library(camera_cfg->cfgPtr->vcm_info.af_name,
                                      libVcmPtr);
    if (!ret) {
        sns_module->af_dev_info.af_drv_entry = libVcmPtr->vcm_info_ptr;
        sns_module->af_dev_info.af_work_mode =
            camera_cfg->cfgPtr->vcm_info.work_mode;
    }

    sensor_drv_xml_parse_otp_info(camera_cfg);
    ret = sensor_drv_otp_load_library(
        camera_cfg->cfgPtr->otp_info.e2p_otp.otp_name, libOtpPtr);
    if (!ret) {
        sns_module->otp_drv_info.otp_drv_entry = libOtpPtr->otp_info_ptr;
        sns_module->otp_drv_info.eeprom_i2c_addr =
            camera_cfg->cfgPtr->otp_info.e2p_otp.eeprom_i2c_addr;
        sns_module->otp_drv_info.eeprom_num =
            camera_cfg->cfgPtr->otp_info.e2p_otp.eeprom_num;
        sns_module->otp_drv_info.eeprom_size =
            camera_cfg->cfgPtr->otp_info.e2p_otp.eeprom_size;
    }

#if defined(CONFIG_CAMERA_SENSOR_OTP)
    SENSOR_REG_TAB_INFO_T *res_info_ptr = PNULL;

    res_info_ptr = sensor_ic_get_data(sensor_cxt, SENSOR_CMD_GET_RESOLUTION);

    hw_Sensor_SendRegTabToSensor(sensor_cxt->hw_drv_handle, &res_info_ptr[0]);
    SENSOR_LOGD("CONFIG_CAMERA_SENSOR_OTP set mode SENSOR_MODE_COMMON_INIT");
#endif
    sensor_af_init(sensor_cxt);
    sensor_otp_module_init(sensor_cxt);
    if (SENSOR_IMAGE_FORMAT_RAW == sensor_cxt->sensor_info_ptr->image_format) {
        if (sns_module->otp_drv_info.otp_drv_entry) {
            sensor_otp_process(sensor_cxt, OTP_READ_PARSE_DATA, 0, NULL);

            otp_drv_cxt_t *otp_cxt =
                (otp_drv_cxt_t *)sensor_cxt->otp_drv_handle;
            module_info = &(otp_cxt->otp_module_info);
            SENSOR_DRV_CHECK_ZERO(module_info);
            awb_src_dat = otp_cxt->otp_raw_data.buffer +
                          module_info->master_awb_info.offset;
            af_src_dat = otp_cxt->otp_raw_data.buffer +
                         module_info->master_af_info.offset;
            SENSOR_DRV_CHECK_ZERO(awb_src_dat);
            SENSOR_DRV_CHECK_ZERO(af_src_dat);
            memcpy(otp_awb_data, awb_src_dat, SPRD_AWB_OTP_SIZE);
            memcpy(otp_af_data, af_src_dat, SPRD_AF_OTP_SIZE);

            sensor_otp_ops_t *otp_ops =
                &sns_module->otp_drv_info.otp_drv_entry->otp_ops;
            otp_ops->sensor_otp_ioctl(sensor_cxt->otp_drv_handle,
                                      CMD_SNS_OTP_GET_MODULE_VENDOR_ID, NULL);
            sensor_cxt->module_vendor_id = otp_cxt->module_vendor_id;
            sensor_cxt->otp_version = module_info->otp_version;
        } else {
            SENSOR_LOGI(
                "otp_drv_entry not configured:mod:%p,otp_drv:%p", sns_module,
                sns_module ? sns_module->otp_drv_info.otp_drv_entry : NULL);
        }
    }
    sensor_otp_module_deinit(sensor_cxt);
    sensor_af_deinit(sensor_cxt);

    return 0;
}

static cmr_int sensor_drv_get_fov_info(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_VAL_T val;
    struct sensor_ic_ops *sns_ops = PNULL;
    cmr_u32 sns_cmd = SENSOR_IOCTL_ACCESS_VAL;
    struct sensor_ex_info sn_ex_info_slv;
    memset(&sn_ex_info_slv, 0, sizeof(struct sensor_ex_info));

    val.type = SENSOR_VAL_TYPE_GET_STATIC_INFO;
    val.pval = &sn_ex_info_slv;

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    if (sns_ops) {
        ret = sns_ops->ext_ops[sns_cmd].ops(sensor_cxt->sns_ic_drv_handle,
                                            (cmr_uint)&val);
        if (!ret) {
            memcpy(&sensor_cxt->fov_info, &sn_ex_info_slv.fov_info,
                   sizeof(sn_ex_info_slv.fov_info));
            sensor_cxt->fov_angle = sn_ex_info_slv.fov_angle;
            sensor_cxt->mono_sensor = sn_ex_info_slv.mono_sensor;
            sensor_cxt->sensor_min_exp = sn_ex_info_slv.sensor_min_exp;
            sensor_cxt->sensor_max_exp = sn_ex_info_slv.sensor_max_exp;
            SENSOR_LOGD("sensor_range (%lld, %lld)",
                     sensor_cxt->sensor_min_exp, sensor_cxt->sensor_max_exp);
        } else {
            SENSOR_LOGE("get sensor ex info failed");
            return -1;
        }
    } else {
        SENSOR_LOGE("sns_ops null");
        return -1;
    }

    return 0;
}

static cmr_int
sensor_drv_store_version_info(struct sensor_drv_context *sensor_cxt,
                              char *sensor_info) {
    char buffer[64] = {0};
    cmr_u64 sensor_size = 0;
    SENSOR_MATCH_T *sns_module = (SENSOR_MATCH_T *)sensor_cxt->current_module;

    SENSOR_LOGI("E");
    if (sensor_cxt->sensor_info_ptr &&
        sensor_cxt->current_module &&
        sns_module && sns_module->sn_name[0]) {

        sensor_size =
            (cmr_u64)((cmr_uint)sensor_cxt->sensor_info_ptr->source_width_max *
                      (cmr_uint)sensor_cxt->sensor_info_ptr->source_height_max);
        
        if (sensor_size >= 1000000) {
            sprintf(buffer, "id%d %s %dM \n",
                    sensor_cxt->slot_id,
                    sns_module->sn_name,
                    (cmr_u32)((float)sensor_size / 1000000.0 + 0.2)
                    );
        } else {
            sprintf(buffer, "id%d %s 0.%dM \n",
                    sensor_cxt->slot_id,
                    sns_module->sn_name,
                    (cmr_u32)(sensor_size / 100000)
                    );
        }

        strcat(sensor_info, buffer);
    }
    SENSOR_LOGI("X");

    return 0;
}

static cmr_int sensor_drv_ic_identify(struct sensor_drv_context *sensor_cxt,
                                      cmr_u32 sensor_id, cmr_u32 identify_off) {
    cmr_int ret = SENSOR_SUCCESS;
    struct module_cfg_info *mod_cfg_info = PNULL;
    struct hw_drv_cfg_param hw_drv_cfg;
    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_DRV_CHECK_PTR(sensor_cxt);
    SENSOR_DRV_CHECK_PTR(sensor_cxt->sensor_info_ptr);
    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;
    SENSOR_LOGI("E");

    ret = sensor_get_module_cfg_info(sensor_cxt, sensor_id, &mod_cfg_info);
    if (ret) {
        SENSOR_LOGE("get module config info failed");
        return ret;
    }

    ret = sensor_ic_create(sensor_cxt, sensor_id);
    if (ret) {
        SENSOR_LOGE("sensor ic handle create failed!");
        goto exit;
    }

    sensor_cxt->i2c_addr = mod_cfg_info->major_i2c_addr;

    do {
        if (sns_ops && sns_ops->identify) {
            hw_drv_cfg.i2c_bus_config = mod_cfg_info->reg_addr_value_bits;
            hw_drv_cfg.i2c_burst_mode = mod_cfg_info->i2c_burst_mode;
            hw_sensor_drv_cfg(sensor_cxt->hw_drv_handle, &hw_drv_cfg);
            sensor_set_id(sensor_cxt, sensor_id);
            sensor_i2c_init(sensor_cxt, sensor_id);
            SENSOR_LOGI("i2c_addr:0x%x", sensor_cxt->i2c_addr);
            hw_sensor_i2c_set_addr(sensor_cxt->hw_drv_handle,
                                   sensor_cxt->i2c_addr);
            hw_sensor_i2c_set_clk(sensor_cxt->hw_drv_handle);
            sensor_power_on(sensor_cxt, SCI_TRUE);

            ret = sns_ops->identify(sensor_cxt->sns_ic_drv_handle,
                                    SENSOR_ZERO_I2C);
            if (ret) {
                sensor_cxt->has_register = SCI_FALSE;
                if ((sensor_cxt->i2c_addr != mod_cfg_info->minor_i2c_addr) &&
                    mod_cfg_info->minor_i2c_addr != 0x00) {
                    sensor_power_on(sensor_cxt, SCI_FALSE);
                    sensor_cxt->i2c_addr = mod_cfg_info->minor_i2c_addr;
                } else
                    goto exit;
            } else {
                SENSOR_LOGI("sensor ic identify ok");
                sensor_cxt->has_register = SCI_TRUE;
                goto exit;
            }
        } else
            goto exit;
    } while (1);

exit:
    if (identify_off || ret) {
        SENSOR_LOGI("sensor drv ic identify power off");
        if (ret == 0 && identify_off)
            sensor_drv_get_dynamic_info(sensor_cxt);
        sensor_power_on(sensor_cxt, SCI_FALSE);
        sensor_i2c_deinit(sensor_cxt, sensor_id);
        sensor_ic_delete(sensor_cxt);
    }
    return ret;
}

static cmr_int sensor_drv_load_library(const char *name,
                                       struct sensor_drv_lib *libPtr) {
    cmr_int ret = SENSOR_FAIL;
    char libso_name[SENSOR_LIB_NAME_LEN] = {0};

    void *(*sensor_ic_open_lib)(void) = NULL;
    int32_t bytes = 0;

    if (!strlen(name)) {
        SENSOR_LOGE("don't config sensor in xml file");
        goto exit;
    }

    bytes = snprintf(libso_name, SENSOR_LIB_NAME_LEN, "libsensor_%s.so", name);
    SENSOR_LOGD("sensor:libso_name %s", libso_name);
    libPtr->drv_lib_handle = dlopen(libso_name, RTLD_NOW);
    if (!libPtr->drv_lib_handle) {
        SENSOR_LOGE("sensor lib handle failed");
        goto exit;
    }

    *(void **)&sensor_ic_open_lib =
        dlsym(libPtr->drv_lib_handle, "sensor_ic_open_lib");
    if (!sensor_ic_open_lib) {
        SENSOR_LOGE("sensor ic open lib function failed");
        dlclose(libPtr->drv_lib_handle);
        libPtr->drv_lib_handle = NULL;
        goto exit;
    }
    libPtr->sensor_info_ptr = (SENSOR_INFO_T *)sensor_ic_open_lib();
    if (!libPtr->sensor_info_ptr) {
        SENSOR_LOGE("load sensor_info_ptr failed");
        dlclose(libPtr->drv_lib_handle);
        libPtr->drv_lib_handle = NULL;
        goto exit;
    }

    ret = 0;
exit:

    return ret;
}

static cmr_int sensor_drv_unload_library(struct sensor_drv_lib *libPtr) {
    if (libPtr && libPtr->drv_lib_handle)
        dlclose(libPtr->drv_lib_handle);
    return 0;
}
static cmr_int sensor_drv_otp_load_library(const char *name,
                                           struct otp_drv_lib *libPtr) {
    cmr_int ret = SENSOR_FAIL;
    char libso_name[SENSOR_LIB_NAME_LEN] = {0};

    void *(*otp_driver_open_lib)(void) = NULL;
    int32_t bytes = 0;

    if (!strlen(name)) {
        SENSOR_LOGI("don't config otp in xml file");
        goto exit;
    }

    bytes = snprintf(libso_name, SENSOR_LIB_NAME_LEN, "libotp_%s.so", name);
    SENSOR_LOGD("otp:libso_name %s", libso_name);
    libPtr->otp_lib_handle = dlopen(libso_name, RTLD_NOW);
    if (!libPtr->otp_lib_handle) {
        SENSOR_LOGE("otp lib handle failed");
        goto exit;
    }

    *(void **)&otp_driver_open_lib =
        dlsym(libPtr->otp_lib_handle, "otp_driver_open_lib");
    if (!otp_driver_open_lib) {
        SENSOR_LOGE("otp driver open lib function failed");
        dlclose(libPtr->otp_lib_handle);
        libPtr->otp_lib_handle = NULL;

        goto exit;
    }
    libPtr->otp_info_ptr = (otp_drv_entry_t *)otp_driver_open_lib();
    if (!libPtr->otp_info_ptr) {
        SENSOR_LOGE("load otp_info_ptr failed");
        dlclose(libPtr->otp_lib_handle);
        libPtr->otp_lib_handle = NULL;
        goto exit;
    }

    ret = 0;
exit:

    return ret;
}

static cmr_int sensor_drv_otp_unload_library(struct otp_drv_lib *libPtr) {
    if (libPtr && libPtr->otp_lib_handle)
        dlclose(libPtr->otp_lib_handle);
    return 0;
}

static cmr_int sensor_drv_vcm_load_library(const char *name,
                                           struct vcm_drv_lib *libPtr) {
    cmr_int ret = SENSOR_FAIL;
    char libso_name[SENSOR_LIB_NAME_LEN] = {0};

    void *(*vcm_driver_open_lib)(void) = NULL;
    int32_t bytes = 0;

    if (!strlen(name)) {
        SENSOR_LOGI("don't config vcm in xml file");
        goto exit;
    }

    bytes = snprintf(libso_name, SENSOR_LIB_NAME_LEN, "libvcm_%s.so", name);
    SENSOR_LOGD("vcm:libso_name %s", libso_name);
    libPtr->vcm_lib_handle = dlopen(libso_name, RTLD_NOW);
    if (!libPtr->vcm_lib_handle) {
        SENSOR_LOGE("vcm lib handle failed");
        goto exit;
    }

    *(void **)&vcm_driver_open_lib =
        dlsym(libPtr->vcm_lib_handle, "vcm_driver_open_lib");
    if (!vcm_driver_open_lib) {
        SENSOR_LOGE("vcm driver open lib function failed");
        dlclose(libPtr->vcm_lib_handle);
        libPtr->vcm_lib_handle = NULL;
        goto exit;
    }
    libPtr->vcm_info_ptr = (struct sns_af_drv_entry *)vcm_driver_open_lib();
    if (!libPtr->vcm_info_ptr) {
        SENSOR_LOGE("load vcm_info_ptr failed");
        dlclose(libPtr->vcm_lib_handle);
        libPtr->vcm_lib_handle = NULL;
        goto exit;
    }

    ret = 0;
exit:

    return ret;
}

static cmr_int sensor_drv_vcm_unload_library(struct vcm_drv_lib *libPtr) {
    if (libPtr && libPtr->vcm_lib_handle)
        dlclose(libPtr->vcm_lib_handle);
    return 0;
}

static cmr_int sensor_drv_tuning_load_library(const char *name,
                                              struct tuning_param_lib *libPtr) {
    cmr_int ret = SENSOR_FAIL;
    char libso_name[SENSOR_LIB_NAME_LEN] = {0};

    void *(*tuning_param_get_ptr)(void) = NULL;
    int32_t bytes = 0;

    if (!strlen(name)) {
        SENSOR_LOGI("don't config tuning in xml file");
        goto exit;
    }

    bytes = snprintf(libso_name, SENSOR_LIB_NAME_LEN, "libparam_%s.so", name);
    SENSOR_LOGD("tuning:libso_name %s", libso_name);
    libPtr->tuning_lib_handle = dlopen(libso_name, RTLD_NOW);
    if (!libPtr->tuning_lib_handle) {
        SENSOR_LOGE("tuning lib handle failed");
        goto exit;
    }

    *(void **)&tuning_param_get_ptr =
        dlsym(libPtr->tuning_lib_handle, "tuning_param_get_ptr");
    if (!tuning_param_get_ptr) {
        SENSOR_LOGE("tuning param open lib function failed");
        dlclose(libPtr->tuning_lib_handle);
        libPtr->tuning_lib_handle = NULL;
        goto exit;
    }
    libPtr->raw_info_ptr = (struct sensor_raw_info *)tuning_param_get_ptr();
    if (!libPtr->raw_info_ptr) {
        SENSOR_LOGE("load tuning_info_ptr failed");
        dlclose(libPtr->tuning_lib_handle);
        libPtr->tuning_lib_handle = NULL;
        goto exit;
    }

    ret = 0;
exit:

    return ret;
}

static cmr_int
sensor_drv_tuning_load_default_library(param_input_t *param_input_ptr,
                                       const char *name,
                                       struct tuning_param_lib *libPtr) {
    cmr_int ret = SENSOR_FAIL;
    char libso_name[SENSOR_LIB_NAME_LEN] = {0};

    void *(*default_tuning_param_get_ptr)(param_input_t *tuning_param_input) =
        NULL;
    int32_t bytes = 0;

    if (!strlen(name)) {
        SENSOR_LOGI("don't config tuning in xml file");
        goto exit;
    }

    bytes = snprintf(libso_name, SENSOR_LIB_NAME_LEN, "libparam_%s.so", name);
    SENSOR_LOGD("tuning:libso_name %s", libso_name);
    libPtr->tuning_lib_handle = dlopen(libso_name, RTLD_NOW);
    if (!libPtr->tuning_lib_handle) {
        SENSOR_LOGE("tuning lib handle failed");
        goto exit;
    }

    *(void **)&default_tuning_param_get_ptr =
        dlsym(libPtr->tuning_lib_handle, "default_tuning_param_get_ptr");
    if (!default_tuning_param_get_ptr) {
        SENSOR_LOGE("tuning param open lib function failed");
        dlclose(libPtr->tuning_lib_handle);
        libPtr->tuning_lib_handle = NULL;
        goto exit;
    }
    libPtr->raw_info_ptr =
        (struct sensor_raw_info *)default_tuning_param_get_ptr(param_input_ptr);
    if (!libPtr->raw_info_ptr) {
        SENSOR_LOGE("load tuning_info_ptr failed");
        dlclose(libPtr->tuning_lib_handle);
        libPtr->tuning_lib_handle = NULL;
        goto exit;
    }

    ret = 0;
exit:

    return ret;
}

static cmr_int
sensor_drv_tuning_unload_library(struct tuning_param_lib *libPtr) {
    if (libPtr && libPtr->tuning_lib_handle)
        dlclose(libPtr->tuning_lib_handle);
    return 0;
}

static cmr_int sensor_drv_identify(struct sensor_drv_context *sensor_cxt,
                                   cmr_u32 sensor_id) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_MATCH_T *sns_module = NULL;
    struct sensor_drv_lib *libPtr = &sensor_lib_mngr[sensor_id];
    struct sensor_module_info *mod_cfg_tab = NULL;
    struct xml_camera_cfg_info *camera_cfg = NULL;

    SENSOR_LOGI("sensor drv identify sensor_id %d", sensor_id);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_set_cxt_common(sensor_cxt);

    sns_module = (SENSOR_MATCH_T *)sensor_cxt->current_module;
    camera_cfg = sensor_cxt->xml_info;

    SENSOR_LOGI("search all sensor in the register tab for current sensor_id");

    if (sensor_id >= SENSOR_ID_MAX) {
        SENSOR_LOGI("invalid id=%d", sensor_id);
        return SENSOR_FAIL;
    }

    do {

        ret = sensor_drv_load_library(camera_cfg->cfgPtr->sensor_name, libPtr);
        if (ret) {
            SENSOR_LOGE("sensor drv load library failed");
            goto exit;
        } else {
            sns_module->sensor_info = libPtr->sensor_info_ptr;
            mod_cfg_tab = libPtr->sensor_info_ptr->module_info_tab;
            sns_module->module_id = mod_cfg_tab[0].module_id;
            strcpy(sns_module->sn_name, camera_cfg->cfgPtr->sensor_name);
        }

        sensor_cxt->sensor_info_ptr = sns_module->sensor_info;

        ret = sensor_drv_ic_identify(sensor_cxt, sensor_id, 1);

        if (ret) {
            sensor_drv_unload_library(libPtr);
        }

    } while (0);

exit:

    SENSOR_LOGI("sensor identify id %d %s", (cmr_u32)sensor_id,
                ret ? "failed" : "ok");

    return ret;
}

static cmr_int sensor_drv_open(struct sensor_drv_context *sensor_cxt,
                               cmr_u32 sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 ret = SENSOR_FAIL;
    cmr_u32 is_inited = 0;
    SENSOR_MATCH_T *module = NULL;
    struct sensor_ic_ops *sns_ops = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sns_ops = sensor_cxt->sensor_info_ptr->sns_ops;

    if (1 == sensor_cxt->is_autotest) {
        is_inited = 1;
    }

    sensor_cxt->sensor_isInit = SENSOR_TRUE;

    ret = sensor_drv_ic_identify(sensor_cxt, sensor_id, 0);
    if (ret) {
        SENSOR_LOGE("sensor drv open failed");
        sensor_cxt->sensor_isInit = SENSOR_FALSE;
        goto exit;
    }
    sensor_af_init(sensor_cxt);
    ret = sensor_set_mode_msg(sensor_cxt, SENSOR_MODE_COMMON_INIT, is_inited);
    if (ret) {
        SENSOR_LOGE("sensor init register mode failed");
        sensor_power_on(sensor_cxt, SCI_FALSE);
        sensor_i2c_deinit(sensor_cxt, sensor_id);
        sensor_ic_delete(sensor_cxt);
        goto exit;
    }

    sensor_otp_module_init(sensor_cxt);
    module = sensor_cxt->current_module;
    if ((SENSOR_IMAGE_FORMAT_RAW ==
         sensor_cxt->sensor_info_ptr->image_format) &&
        module) {
        cmr_u8 vendor_id = 0;
        if (module->otp_drv_info.otp_drv_entry) {
            // sensor_write_dualcam_otpdata(sensor_cxt, sensor_id);
            sensor_otp_rw_ctrl(sensor_cxt, OTP_READ_PARSE_DATA, 0, NULL);
            sensor_set_otp_data(sensor_cxt);
            sensor_otp_ops_t *otp_ops = PNULL;
            otp_ops = &module->otp_drv_info.otp_drv_entry->otp_ops;
            otp_ops->sensor_otp_ioctl(sensor_cxt->otp_drv_handle,
                                      CMD_SNS_OTP_GET_VENDOR_ID,
                                      (void *)&vendor_id);
        } else {
            SENSOR_LOGI("otp_drv_entry not configured:mod:%p,otp_drv:%p",
                        module,
                        module ? module->otp_drv_info.otp_drv_entry : NULL);
        }
        sensor_set_raw_infor(sensor_cxt, vendor_id);
    }
    sensor_set_export_Info(sensor_cxt);
    sensor_cxt->stream_on = 1;
    sensor_stream_off(sensor_cxt);

    if ((SENSOR_SUCCESS == ret) && (1 == sensor_cxt->is_autotest)) {
        sensor_set_mode_done_common(sensor_cxt);
    }

    SENSOR_LOGI("open success");

exit:
    ATRACE_END();
    return ret;
}

static cmr_u8 sensor_snspid[SENSOR_ID_MAX][SNSPID_SIZE]  = {0};

static cmr_u8 bokeh_snspid_size = BOKEH_SNSPID_SIZE;
static cmr_u8 bokeh_snspid[BOKEH_SNSPID_SIZE] = {0};
static cmr_u8 bokeh_module_name_size = BOKEH_MODULE_NAME_SIZE;
static cmr_u8 bokeh_module_name[BOKEH_MODULE_NAME_SIZE] = {0};
static cmr_u8 bokeh_cmei_size = BOKEH_MODULE_NAME_SIZE + BOKEH_SNSPID_SIZE;
static cmr_u8 bokeh_cmei[BOKEH_MODULE_NAME_SIZE + BOKEH_SNSPID_SIZE] = {0};

static cmr_u8 oz1_snspid_size = OZ1_SNSPID_SIZE;
static cmr_u8 oz1_snspid[OZ1_SNSPID_SIZE] = {0};
static cmr_u8 oz1_module_name_size = OZ1_MODULE_NAME_SIZE;
static cmr_u8 oz1_module_name[OZ1_MODULE_NAME_SIZE] = {0};
static cmr_u8 oz1_cmei_size = OZ1_MODULE_NAME_SIZE + OZ1_SNSPID_SIZE;
static cmr_u8 oz1_cmei[OZ1_MODULE_NAME_SIZE + OZ1_SNSPID_SIZE] = {0};

static cmr_u8 oz2_snspid_size = OZ2_SNSPID_SIZE;
static cmr_u8 oz2_snspid[OZ2_SNSPID_SIZE] = {0};
static cmr_u8 oz2_module_name_size = OZ2_MODULE_NAME_SIZE;
static cmr_u8 oz2_module_name[OZ2_MODULE_NAME_SIZE] = {0};
static cmr_u8 oz2_cmei_size = OZ2_MODULE_NAME_SIZE + OZ2_SNSPID_SIZE;
static cmr_u8 oz2_cmei[OZ2_MODULE_NAME_SIZE + OZ2_SNSPID_SIZE] = {0};

cmr_int sensor_set_snspid_common(cmr_handle sns_module_handle,
        cmr_u8 sensor_id, cmr_u8 *snspid, cmr_u8 snspid_size) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sns_module_handle;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    cmr_u8 i = 0;

    SENSOR_LOGI("E");

    for(i = 0; i < snspid_size; i++) {
        sensor_snspid[sensor_id][i] = snspid[i];
    }

    return ret;
}

static cmr_int sensor_drv_create_multicam_snspid(cmr_u8 dual_flag) {
    cmr_u32 has_scaned = SENSOR_FALSE;
    struct camera_device_manager *devPtr = &camera_dev_manger;
    struct phySensorInfo *phyPtr = phy_sensor_info_list;
    struct logicalSensorInfo *logicalPtr = logical_sensor_info_list;
    cmr_int i = 0, j =0, k=0;

    SENSOR_LOGI("E");

    switch (dual_flag) {

    case CALIBRATION_FLAG_BOKEH:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_BLUR_ID == (logicalPtr + i)->multiCameraId) {
                logicalPtr += i;
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
      	     for(i = 0; i < logicalPtr->physicalNum; i++) {
                for(j = 0; j < SNSPID_SIZE; j++)
                    bokeh_snspid[i * SNSPID_SIZE + j] =
                            sensor_snspid[logicalPtr->phyIdGroup[i].phyId][j];
      	     }
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ1:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_MULTI_CAMERA_ID == (logicalPtr + i)->multiCameraId) {
                logicalPtr += i;
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
      	     for(i = 0, k = 0; i < logicalPtr->physicalNum; i++) {
                if((SENSOR_ROLE_MULTICAM_WIDE == logicalPtr->phyIdGroup[i].sensor_role) ||
                        (SENSOR_ROLE_MULTICAM_SUPERWIDE == logicalPtr->phyIdGroup[i].sensor_role)) {
                    for(j = 0; j < SNSPID_SIZE; j++)
                        oz1_snspid[k * SNSPID_SIZE + j] = sensor_snspid[logicalPtr->phyIdGroup[i].phyId][j];
                    k++;
                }
      	     }
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ2:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_MULTI_CAMERA_ID == (logicalPtr + i)->multiCameraId) {
                logicalPtr += i;
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
      	     for(i = 0, k = 0; i < logicalPtr->physicalNum; i++) {
                if((SENSOR_ROLE_MULTICAM_WIDE == logicalPtr->phyIdGroup[i].sensor_role) ||
                        (SENSOR_ROLE_MULTICAM_TELE== logicalPtr->phyIdGroup[i].sensor_role)) {
                    for(j = 0; j < SNSPID_SIZE; j++)
                        oz2_snspid[k * SNSPID_SIZE + j] = sensor_snspid[logicalPtr->phyIdGroup[i].phyId][j];
                    k++;
                }
      	     }
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    default:
        SENSOR_LOGE("input dual_flag:%d is invalid!", dual_flag);
        return SENSOR_FAIL;
    }

    return SENSOR_SUCCESS;
}

static cmr_int sensor_drv_create_module_name(cmr_u8 dual_flag) {
    cmr_u32 has_scaned = SENSOR_FALSE;
    struct camera_device_manager *devPtr = &camera_dev_manger;
    struct phySensorInfo *phyPtr = phy_sensor_info_list;
    struct logicalSensorInfo *logicalPtr = logical_sensor_info_list;
    cmr_int i = 0, j = 0;

    SENSOR_LOGI("E");

    switch (dual_flag) {

    case CALIBRATION_FLAG_BOKEH:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_BLUR_ID == (logicalPtr + i)->multiCameraId) {
                logicalPtr += i;
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
            for(i = 0; i < logicalPtr->physicalNum; i++) {
                strcat(bokeh_module_name, 
                        (phyPtr + logicalPtr->phyIdGroup[i].phyId)->sensor_name);
                if (i != (logicalPtr->physicalNum - 1)) {
                    strcat(bokeh_module_name, "-");
                }
            }
            SENSOR_LOGI("SPRD_BLUR_ID %s", bokeh_module_name);
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ1:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_MULTI_CAMERA_ID == (logicalPtr + i)->multiCameraId) {
                logicalPtr += i;
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
            for(i = 0, j = 0; i < logicalPtr->physicalNum; i++) {
                if((SENSOR_ROLE_MULTICAM_WIDE == logicalPtr->phyIdGroup[i].sensor_role) ||
                        (SENSOR_ROLE_MULTICAM_SUPERWIDE == logicalPtr->phyIdGroup[i].sensor_role)) {
                    strcat(oz1_module_name, 
                            (phyPtr + logicalPtr->phyIdGroup[i].phyId)->sensor_name);
                    if (j < 1) {
                        strcat(oz1_module_name, "-");
                    }
                    j++;
                }
            }
            SENSOR_LOGI("CALIBRATION_FLAG_OZ1 %s", oz1_module_name);
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ2:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_MULTI_CAMERA_ID == (logicalPtr + i)->multiCameraId) {
                logicalPtr += i;
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
            for(i = 0, j = 0; i < logicalPtr->physicalNum; i++) {
                if((SENSOR_ROLE_MULTICAM_WIDE == logicalPtr->phyIdGroup[i].sensor_role) ||
                        (SENSOR_ROLE_MULTICAM_TELE == logicalPtr->phyIdGroup[i].sensor_role)) {
                    strcat(oz2_module_name, 
                            (phyPtr + logicalPtr->phyIdGroup[i].phyId)->sensor_name);
                    if (j < 1) {
                        strcat(oz2_module_name, "-");
                    }
                    j++;
                }
            }
            SENSOR_LOGI("CALIBRATION_FLAG_OZ2 %s", oz2_module_name);
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    default:
        SENSOR_LOGE("input dual_flag:%d is invalid!", dual_flag);
        return SENSOR_FAIL;
    }

    return SENSOR_SUCCESS;
}

static cmr_int sensor_drv_create_cmei(cmr_u8 dual_flag) {
    cmr_u32 has_scaned = SENSOR_FALSE;
    struct camera_device_manager *devPtr = &camera_dev_manger;
    struct phySensorInfo *phyPtr = phy_sensor_info_list;
    struct logicalSensorInfo *logicalPtr = logical_sensor_info_list;
    cmr_int i = 0;

    SENSOR_LOGI("E");

    switch (dual_flag) {

    case CALIBRATION_FLAG_BOKEH:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_BLUR_ID == (logicalPtr + i)->multiCameraId) {
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
            memcpy(bokeh_cmei, bokeh_module_name, bokeh_module_name_size);
            memcpy(bokeh_cmei+ bokeh_module_name_size, bokeh_snspid, bokeh_snspid_size);
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ1:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_MULTI_CAMERA_ID == (logicalPtr + i)->multiCameraId) {
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
            memcpy(oz1_cmei, oz1_module_name, oz1_module_name_size);
            memcpy(oz1_cmei+ oz1_module_name_size, oz1_snspid, oz1_snspid_size);
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ2:
        for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
            if ((logicalPtr + i)->logicalId == 0xff)
                continue;
            if (SPRD_MULTI_CAMERA_ID == (logicalPtr + i)->multiCameraId) {
                has_scaned = SENSOR_TRUE;
                break;
            }
        }
        if (SENSOR_TRUE == has_scaned) {
            memcpy(oz2_cmei, oz2_module_name, oz2_module_name_size);
            memcpy(oz2_cmei+ oz2_module_name_size, oz2_snspid, oz2_snspid_size);
        } else {
            SENSOR_LOGE("dual_flag:%d, can't find all configured sensors!",
                          dual_flag);
            return SENSOR_FAIL;
        }
        break;

    default:
        SENSOR_LOGE("input dual_flag:%d is invalid!", dual_flag);
        return SENSOR_FAIL;
    }

    return SENSOR_SUCCESS;
}

static cmr_int sensor_drv_create_cmei_list(void)
{
    SENSOR_LOGI("E");

    sensor_drv_create_multicam_snspid(CALIBRATION_FLAG_BOKEH);
    sensor_drv_create_module_name(CALIBRATION_FLAG_BOKEH);
    sensor_drv_create_cmei(CALIBRATION_FLAG_BOKEH);

    sensor_drv_create_multicam_snspid(CALIBRATION_FLAG_OZ1);
    sensor_drv_create_module_name(CALIBRATION_FLAG_OZ1);
    sensor_drv_create_cmei(CALIBRATION_FLAG_OZ1);

    sensor_drv_create_multicam_snspid(CALIBRATION_FLAG_OZ2);
    sensor_drv_create_module_name(CALIBRATION_FLAG_OZ2);
    sensor_drv_create_cmei(CALIBRATION_FLAG_OZ2);

    return SENSOR_SUCCESS;
}

static cmr_int sensor_drv_check_cmei(cmr_u8 dual_flag) {
    cmr_u32 ret = SENSOR_FAIL;
    cmr_u16 cmei_size = 0;
    cmr_u8 bokeh_cmei_buf[bokeh_cmei_size];
    cmr_u8 oz1_cmei_buf[oz1_cmei_size];
    cmr_u8 oz2_cmei_buf[oz2_cmei_size];
    memset(bokeh_cmei_buf, 0, bokeh_cmei_size);
    memset(oz1_cmei_buf, 0, oz1_cmei_size);
    memset(oz2_cmei_buf, 0, oz2_cmei_size);
    SENSOR_LOGI("E");

    switch (dual_flag) {

    case CALIBRATION_FLAG_BOKEH:
        cmei_size = read_calibration_cmei(CALIBRATION_FLAG_BOKEH, bokeh_cmei_buf);
        if(bokeh_cmei_size == cmei_size) {
            ret = memcmp(bokeh_cmei_buf, bokeh_cmei, bokeh_cmei_size);
            if(0 == ret) {
                SENSOR_LOGI("bokeh module hasnot changed, use calibraton data");
                return SENSOR_SUCCESS;
            } else{
                SENSOR_LOGI("bokeh module has changed, use golden data");
                return SENSOR_FAIL;
            }
        } else {
            SENSOR_LOGI("cannot get bokeh module info, use calibraton data");
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ1:
        cmei_size = read_calibration_cmei(CALIBRATION_FLAG_OZ1, oz1_cmei_buf);
        if(oz1_cmei_size == cmei_size) {
            ret = memcmp(oz1_cmei_buf, oz1_cmei, oz1_cmei_size);
            if(0 == ret) {
                SENSOR_LOGI("oz1 module hasnot changed, use calibraton data");
                return SENSOR_SUCCESS;
            } else{
                SENSOR_LOGI("oz1 module has changed, use golden data");
                return SENSOR_FAIL;
            }
        } else {
            SENSOR_LOGI("cannot get oz1 module info, use calibraton data");
            return SENSOR_FAIL;
        }
        break;

    case CALIBRATION_FLAG_OZ2:
        cmei_size = read_calibration_cmei(CALIBRATION_FLAG_OZ2, oz2_cmei_buf);
        if(oz2_cmei_size == cmei_size) {
            ret = memcmp(oz2_cmei_buf, oz2_cmei, oz2_cmei_size);
            if(0 == ret) {
                SENSOR_LOGI("oz2 module hasnot changed, use calibraton data");
                return SENSOR_SUCCESS;
            } else{
                SENSOR_LOGI("oz2 module has changed, use golden data");
                return SENSOR_FAIL;
            }
        } else {
            SENSOR_LOGI("cannot get oz2 module info, use calibraton data");
            return SENSOR_FAIL;
        }
        break;

    default:
        SENSOR_LOGE("input dual_flag:%d is invalid!", dual_flag);
        return SENSOR_FAIL;
    }

    return SENSOR_SUCCESS;
}

static cmr_int sensor_drv_scan_hw(void) {
    struct camera_device_manager *devPtr = &camera_dev_manger;
    struct sensor_drv_context sns_cxt;
    struct sensor_drv_context *sensor_cxt = &sns_cxt;
    int physical_num = 0;
    cmr_int i = 0;
    cmr_int ret = SENSOR_FAIL;
    char sensor_version_info[SENSOR_ID_MAX * 64] = {0};
    int32_t bytes = 0;
    char config_xml_name[CONFIG_XML_NAME_LEN];
    xmlDocPtr docPtr = NULL;
    xmlNodePtr rootPtr = NULL;
    xmlNodePtr nodePtr = NULL;
    int module_cfg_num = 0;
    xml_camera_module_cfg_t module_cfg;
    struct xml_camera_cfg_info camera_cfg_info;
    cmr_u32 slot_id = 0;
    cmr_u8 sensor_count[SENSOR_ID_MAX] = {0xff};
    cmr_u8 probe_identify[SENSOR_ID_MAX] = {0};
    int project_num = 0;

    if (devPtr->hasScaned) {
        SENSOR_LOGI("driver has scaned physical number %d logical_number %d",
                    devPtr->physical_num, devPtr->logical_num);
        return devPtr->physical_num;
    }

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    memset(devPtr->drv_idx, 0xff, SENSOR_ID_MAX);
    memset(sensor_count, 0xff, SENSOR_ID_MAX);
    sensor_drv_sensor_info_list_init();

    bytes = snprintf(config_xml_name, sizeof(config_xml_name), "%s%s",
                     CONFIG_XML_PATH, CONFIG_XML_FILE);
    if (access(config_xml_name, R_OK)) {
        SENSOR_LOGE("sensor_config.xml is not found");
        goto exit;
    }

    ret = sensor_drv_xml_load_file(config_xml_name, &docPtr, &rootPtr, "root");
    if (ret) {
        SENSOR_LOGE("load sensor_config.xml file failed");
        goto exit;
    }

    module_cfg_num = sensor_drv_xml_get_node_num(rootPtr, "CameraModuleCfg");
    SENSOR_LOGD("module_cfg_num %d", module_cfg_num);

    if (!module_cfg_num || module_cfg_num > MODULE_CFG_MAX_NUM) {
        SENSOR_LOGE("module_cfg_num is invaild");
        goto exit;
    }

    camera_cfg_info.docPtr = docPtr;
    camera_cfg_info.cfgPtr = &module_cfg;

    for (i = 0; i < module_cfg_num; i++) {

        nodePtr = sensor_drv_xml_get_node(rootPtr, "CameraModuleCfg", i);
        if (!nodePtr) {
            SENSOR_LOGE("CameraModuleCfg get nodePtr failed");
            goto exit;
        }

        memset(&module_cfg, 0, sizeof(xml_camera_module_cfg_t));
        camera_cfg_info.nodePtr = nodePtr;
        ret = sensor_drv_xml_parse_camera_module_info(&camera_cfg_info);
        if (ret) {
            SENSOR_LOGE("parse camera module info failed");
            goto exit;
        }

        if (module_cfg.slot_id < SENSOR_ID_MAX &&
            probe_identify[module_cfg.slot_id]) {
            SENSOR_LOGD(
                "slot_id %d has been identified successfully before, is %s",
                module_cfg.slot_id,
                slot_sensor_info_list[module_cfg.slot_id].sensor_name);
            continue;
        }
        slot_id = module_cfg.slot_id;

        sensor_count[slot_id] = slot_id;

        ret = sensor_context_init(sensor_cxt, slot_id, 0, 0);
        if (ret) {
            SENSOR_LOGE("sensor context init error id = %d", slot_id);
            sensor_context_deinit(sensor_cxt, 0);
            i++;
            probe_identify[slot_id] = 1;
            continue;
        }

        memcpy(sensor_cxt->xml_info, &camera_cfg_info,
               sizeof(struct xml_camera_cfg_info));
        if (SENSOR_SUCCESS == sensor_drv_identify(sensor_cxt, slot_id)) {
            sensor_drv_store_version_info(sensor_cxt, sensor_version_info);
            sensor_drv_create_slot_sensor_info(sensor_cxt, slot_id);
            sensor_drv_create_phy_sensor_info(sensor_cxt, slot_id,
                                              physical_num);
            physical_num++;
            probe_identify[slot_id] = 1;
            SENSOR_LOGD("slot_id %d %s is identified successfully", slot_id,
                        slot_sensor_info_list[slot_id].sensor_name);
            devPtr->identify_state[slot_id] = IDENTIFY_STATUS_PRESENT;
            SENSOR_LOGV("useful sensor identify_state[%d] = %d", slot_id,
                        devPtr->identify_state[slot_id]);
        } else {
            devPtr->identify_state[slot_id] = IDENTIFY_STATUS_NOT_PRESENT;
            SENSOR_LOGV("unuseful sensor identify_state[%d] = %d", slot_id,
                        devPtr->identify_state[slot_id]);
        }

        sensor_context_deinit(sensor_cxt, 0);
    }

    sensor_rid_save_sensor_info(sensor_version_info, slot_id);
	sensor_save_all_sensor_name();

#ifdef CAMERA_CONFIG_SENSOR_NUM
    for (i = 0; i < SENSOR_ID_MAX; i++) {
        if (sensor_count[i] != 0xff)
            project_num++;
    }
    devPtr->physical_num = project_num;
#else
    devPtr->physical_num = physical_num;
#endif

    devPtr->logical_num =
        sensor_drv_create_logical_sensor_info(devPtr->physical_num);
    devPtr->logical_cam_num = sensor_drv_create_logical_camera_info();

    devPtr->hasScaned = 1;

    sensor_drv_create_cmei_list();

exit:
    if (docPtr) {
        sensor_drv_xml_unload_file(docPtr);
    }
    SENSOR_LOGI("scan physical number %d logical number %d",
                devPtr->physical_num, devPtr->logical_num);
    sensor_drv_print_slot_list_info();
    sensor_drv_print_phy_list_info();
    sensor_drv_print_logical_list_info();
    sensor_drv_print_logical_camera_list_info();

    return devPtr->physical_num;
}

int sensorGetPhysicalSnsNum(void) {
    struct camera_device_manager *devPtr = &camera_dev_manger;

    sensor_drv_scan_hw();

    return devPtr->physical_num;
}

int sensorGetLogicalSnsNum(void) {
    struct camera_device_manager *devPtr = &camera_dev_manger;

    sensor_drv_scan_hw();

    return devPtr->logical_num;
}

int sensorGetLogicalCamNum(void) {
    struct camera_device_manager *devPtr = &camera_dev_manger;

    sensor_drv_scan_hw();

    return devPtr->logical_cam_num;
}

void *sensorGetIdentifyState(void) {

    return (void *)&camera_dev_manger;
}

PHYSICAL_SENSOR_INFO_T *sensorGetPhysicalSnsInfo(int phy_id) {

    if (phy_id >= SENSOR_ID_MAX)
        return NULL;

    return &phy_sensor_info_list[phy_id];
}

LOGICAL_SENSOR_INFO_T *sensorGetLogicalSnsInfo(int logical_id) {

    if (logical_id >= LOGICAL_SENSOR_ID_MAX)
        return NULL;

    return &logical_sensor_info_list[logical_id];
}

struct logicalCameraInfo *sensorGetLogicalCamInfo(int logical_cam_id) {

    if (logical_cam_id >= LOGICAL_SENSOR_ID_MAX)
        return NULL;

    return &logical_camera_info_list[logical_cam_id];
}

int sensorGetLogicalCameraList(struct logicalCameraInfo **logical_cam_ptr) {
    struct camera_device_manager *devPtr = &camera_dev_manger;

    *logical_cam_ptr = &logical_camera_info_list[0];

    return devPtr->logical_cam_num;
}

struct lensProperty *sensorGetlensProperty(int phy_id) {

    return sensor_get_lens_property(phy_id);
}

/**
 * NOTE: sensor_role_code in physical sensor info is a cmr_u32 digit.
 *       4 bits means a role_group, total 8 groups. Now low 16 bits have
 *       been used, high 16 bits are reserved.
 * NOTE: If 32 bits have been used up, sensor_role_code will expand to 8-bit
 *       or 32-bit array in future. Do not change cmr_u32 type to cmr_u64
 *       directly, in case of type conversion bug.
 *       (function sensor_drv_xml_str_to_integer in sensor_drv_xml_parse.c)
 *
 * cmr_u32 sensor_role_code definition:
 *       0~3 bits: single-camera role
 *           0 -- single
 *           1 -- single_ir
 *           2 -- single_macro
 *           3~f -- reserved
 *       4~7 bits: dualcamera role
 *           0 -- none
 *           1 -- dualcam_master
 *           2 -- dualcam_slave
 *           3~f -- reserved
 *       8~11 bits: multicamera opticszoom role
 *           0 -- none
 *           1 -- multicam_superwide
 *           2 -- multicam_wide
 *           3 -- multicam_tele
 *           4~f -- reserved
 *       12~15 bits: multicamera 3d structure light role
 *           0 -- none
 *           1 -- stl3d_rgb
 *           2 -- stl3d_ir_left
 *           3 -- stl3d_ir_right
 *           4~f -- reserved
 *       16~31 bits: reserved
 *
 * NOTE: single-camera superwide now multiplexes multicam_superwide, not create
 *       a single-camera role for it purposely.
 *
 * for example, sharkl5pro ums512_1h10 back master camera's sensor_role_code
 * is 0x00000210, means it's a bokeh master camera and an opticszoom wide camera
 * at the same time, no special single-camera feature.
 **/
int sensorGetPhyId4Role(enum sensor_role role, enum face_type facing) {
    int phyId = SENSOR_ID_INVALID;
    cmr_u32 code = 0;
    cmr_u32 face = 0;
    struct phySensorInfo *phyPtr = NULL;

    /* only return the first sensor that meets role,
     * and surely this sensor has been identified successfully */
    for (phyId = 0; phyId < SENSOR_ID_MAX; phyId++) {
        phyPtr = sensorGetPhysicalSnsInfo(phyId);
        code = phyPtr->sensor_role_code;
        face = phyPtr->face_type;
        switch (role) {
        case SENSOR_ROLE_DUALCAM_MASTER:
            if (((code >> 4) & 0xf) == 0x1 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_DUALCAM_SLAVE:
            if (((code >> 4) & 0xf) == 0x2 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_MULTICAM_SUPERWIDE:
            if (((code >> 8) & 0xf) == 0x1 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_MULTICAM_WIDE:
            if (((code >> 8) & 0xf) == 0x2 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_MULTICAM_TELE:
            if (((code >> 8) & 0xf) == 0x3 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_STL3D_RGB:
            if (((code >> 12) & 0xf) == 0x1 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_STL3D_IR_LEFT:
            if (((code >> 12) & 0xf) == 0x2 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_STL3D_IR_RIGHT:
            if (((code >> 12) & 0xf) == 0x3 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_SINGLE_IR:
            if ((code & 0xf) == 0x1 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_SINGLE_MACRO:
            if ((code & 0xf) == 0x2 && face == facing) {
                goto exit;
            }
            break;
        case SENSOR_ROLE_SINGLE:
            SENSOR_LOGD(
                "finding sensor_role single is meaningless, return 0xff");
            return SENSOR_ID_INVALID;
        default:
            SENSOR_LOGD("not support sensor_role %d", role);
            return SENSOR_ID_INVALID;
        }
    }
    SENSOR_LOGD("can't find sensor_role %d at facing %d", role, facing);
    return SENSOR_ID_INVALID;

exit:
    SENSOR_LOGD("find sensor_role %d at facing %d, phyId %d", role, facing,
                phyId);
    return phyId;
}

LOGICAL_SENSOR_INFO_T *sensorGetLogicaInfo4multiCameraId(int multiCameraId) {
    int i = 0;
    struct camera_device_manager *devPtr = &camera_dev_manger;
    struct logicalSensorInfo *logicalPtr = logical_sensor_info_list;

    for (i = devPtr->physical_num; i < devPtr->logical_num; i++) {
        if ((logicalPtr + i)->logicalId == SENSOR_ID_INVALID)
            continue;

        if (multiCameraId == (logicalPtr + i)->multiCameraId) {
            logicalPtr += i;
            return logicalPtr;
        }
    }

    return NULL;
}

cmr_int sensorGetZoomParam(struct sensor_zoom_param_input *zoom_param) {
    int ret = CMR_CAMERA_SUCCESS;
    char value[PROPERTY_VALUE_MAX] = {0};

    property_get("persist.vendor.cam.multi.section", value, "3");
    if (atoi(value) == 3) {
        if (zoom_param->camera_id == SPRD_DUAL_VIEW_VIDEO_ID) {
            zoom_param->PhyCameras = 2;
            zoom_param->MaxDigitalZoom = 10.0;
            zoom_param->ZoomRatioSection[0] = 2.0;
            zoom_param->ZoomRatioSection[1] = 10.0;
            zoom_param->ZoomRatioSection[2] = 0;
            zoom_param->ZoomRatioSection[3] = 0;
            zoom_param->ZoomRatioSection[4] = 0;
            zoom_param->ZoomRatioSection[5] = 0;
            zoom_param->BinningRatio = 5.0;
        } else {
            zoom_param->PhyCameras = 3;
            zoom_param->MaxDigitalZoom = 10.0;
            zoom_param->ZoomRatioSection[0] = 0.6;
            zoom_param->ZoomRatioSection[1] = 1.0;
            zoom_param->ZoomRatioSection[2] = 2.0;
            zoom_param->ZoomRatioSection[3] = 10.0;
            zoom_param->ZoomRatioSection[4] = 0;
            zoom_param->ZoomRatioSection[5] = 0;
            zoom_param->BinningRatio = 5.0;
        }
    } else if (atoi(value) == 2) {
        zoom_param->PhyCameras = 2;
        zoom_param->MaxDigitalZoom = 8.0;
        zoom_param->ZoomRatioSection[0] = 0.6;
        zoom_param->ZoomRatioSection[1] = 1.0;
        zoom_param->ZoomRatioSection[2] = 8.0;
        zoom_param->ZoomRatioSection[3] = 0;
        zoom_param->ZoomRatioSection[4] = 0;
        zoom_param->ZoomRatioSection[5] = 0;
        zoom_param->BinningRatio = 8.0;
    }

    return ret;
}

cmr_int sensor_read_calibration_otp(struct sensor_drv_context *sensor_cxt,
                                    cmr_u8 dual_flag, struct sensor_otp_cust_info *otp_data) {
    int ret = 0;
    cmr_u16 otpsize = 0;
    cmr_u8 dual_flag_tmp = 0;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("E dual_flag:%d, sensor_id:%d", dual_flag, sensor_cxt->slot_id);

    pthread_mutex_lock(&cali_otp_mutex);

    dual_flag_tmp = dual_flag;
    switch(dual_flag) {
    case CALIBRATION_FLAG_BOKEH:
#if defined(BOKEH_CALIBRATION_VERSION2) || defined(BOKEH_CALIBRATION_VERSION3)
        ret = sensor_drv_check_cmei(dual_flag);
	 if(ret)
            dual_flag_tmp = CALIBRATION_FLAG_BOKEH_GLD2;
#endif
        break;

    case CALIBRATION_FLAG_OZ1:
#if defined(OZ_CALIBRATION_VERSION2)
        ret = sensor_drv_check_cmei(dual_flag);
	 if(ret)
            dual_flag_tmp = CALIBRATION_FLAG_OZ1_GLD;
#endif
        break;

    case CALIBRATION_FLAG_OZ2:
#if defined(OZ_CALIBRATION_VERSION2)
        ret = sensor_drv_check_cmei(dual_flag);
	 if(ret)
            dual_flag_tmp = CALIBRATION_FLAG_OZ2_GLD;
#endif
        break;

    default:
        break;
    }

    otpsize = read_calibration_otp(dual_flag_tmp, otpdata[sensor_cxt->slot_id]);

    pthread_mutex_unlock(&cali_otp_mutex);
    if (otpsize > 0) {
        otp_data->total_otp.data_ptr = otpdata[sensor_cxt->slot_id];
        otp_data->total_otp.size = otpsize;
        otp_data->dual_otp.dual_flag = dual_flag;
        otp_data->dual_otp.data_3d.data_ptr = otpdata[sensor_cxt->slot_id];
        otp_data->dual_otp.data_3d.size = otpsize;
        SENSOR_LOGI("read calibration otp data success, size :%d", otpsize);
        return SENSOR_SUCCESS;
    } else {
        SENSOR_LOGE("read calibration otp data failed, size:%d", otpsize);
        return SENSOR_FAIL;
    }
}

cmr_int sensor_write_calibration_otp(struct sensor_drv_context *sensor_cxt,
	                                 cmr_u8 *buf, cmr_u8 dual_flag, cmr_u16 otp_size) {
    int ret = 0;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("E dual_flag:%d, sensor_id:%d", dual_flag, sensor_cxt->slot_id);

    pthread_mutex_lock(&cali_otp_mutex);

    switch(dual_flag) {
    case CALIBRATION_FLAG_BOKEH:
#if defined(BOKEH_CALIBRATION_VERSION2) || defined(BOKEH_CALIBRATION_VERSION3)
        ret = write_calibration_otp_with_cmei
                (dual_flag, buf, otp_size, bokeh_cmei, bokeh_cmei_size);
#else
        ret = write_calibration_otp_no_cmei(dual_flag, buf, otp_size);
#endif
        break;

    case CALIBRATION_FLAG_OZ1:
#if defined(OZ_CALIBRATION_VERSION2)
        ret = write_calibration_otp_with_cmei
                 (dual_flag, buf, otp_size, oz1_cmei, oz1_cmei_size);
#else
        ret = write_calibration_otp_no_cmei(dual_flag, buf, otp_size);
#endif
        break;

    case CALIBRATION_FLAG_OZ2:
#if defined(OZ_CALIBRATION_VERSION2)
        ret = write_calibration_otp_with_cmei
                (dual_flag, buf, otp_size, oz2_cmei, oz2_cmei_size);
#else
        ret = write_calibration_otp_no_cmei(dual_flag, buf, otp_size);
#endif
        break;

    default:
        ret = write_calibration_otp_no_cmei(dual_flag, buf, otp_size);
        break;
    }

    pthread_mutex_unlock(&cali_otp_mutex);

    if (1 == ret) {
        SENSOR_LOGI("write calibration otp data success!");
        return SENSOR_SUCCESS;
    } else {
        SENSOR_LOGE("write calibration otp data failed!");
        return SENSOR_FAIL;
    }
}

cmr_int sensor_set_HD_mode(cmr_u32 is_HD_mode) {
    int ret = SENSOR_SUCCESS;

    sensor_is_HD_mode = is_HD_mode;
    SENSOR_LOGI("is_HD_mode:%d", is_HD_mode);
    return ret;
}

cmr_int sensor_set_longExp_enable(struct sensor_drv_context *sensor_cxt,
                           cmr_u32 long_expo_enable) {
    int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sensor_cxt->is_long_expo = long_expo_enable;
    SENSOR_LOGV("is_long_expo:%d", sensor_cxt->is_long_expo);
    return ret;
}

cmr_int sensor_get_otp_tag(cmr_s32 *otp_ptr, cmr_int id) {
    if(id > SENSOR_ID_MAX) {
        SENSOR_LOGE("not valid sensor id");
        return -1;
    }
    cmr_u8 *awb_src_dat, *af_src_dat;
    cmr_int ret = SENSOR_SUCCESS;
    uint16_t RG_GAIN, BG_GAIN, RG_GOLDEN_GAIN, BG_GOLDEN_GAIN, AF_INFI, AF_MAC;
    uint16_t r_gain_current = 0, g_gain_current = 0, b_gain_current = 0, base_gain = 0;
    uint16_t r_gain = 1024, g_gain = 1024, b_gain = 1024;
    SENSOR_DRV_CHECK_ZERO(otp_ptr);

    awb_src_dat = otp_awb_data;
    af_src_dat = otp_af_data;

    SENSOR_DRV_CHECK_ZERO(awb_src_dat);
    SENSOR_DRV_CHECK_ZERO(af_src_dat);

    RG_GAIN = (awb_src_dat[0] | (((cmr_u16)awb_src_dat[1] & 0xf0) << 4)) > 0 ? (awb_src_dat[0] | (((cmr_u16)awb_src_dat[1] & 0xf0) << 4)) : 0x400;
    BG_GAIN = ((((cmr_u16)awb_src_dat[1] & 0x0f) << 8) | awb_src_dat[2]) > 0 ? ((((cmr_u16)awb_src_dat[1] & 0x0f)  << 8) | awb_src_dat[2]) : 0x400;
    RG_GOLDEN_GAIN = (awb_src_dat[0x4] | (((cmr_u16)awb_src_dat[0x5] & 0xf0) << 4)) > 0 ? (awb_src_dat[0x4] | (((cmr_u16)awb_src_dat[1] & 0xf0) << 4)) : 0x400;
    BG_GOLDEN_GAIN = ((((cmr_u16)awb_src_dat[0x5] & 0x0f) << 8) | awb_src_dat[0x6]) > 0 ? ((((cmr_u16)awb_src_dat[0x5] & 0x0f)  << 8) | awb_src_dat[0x6]) : 0x400;

    AF_INFI = (((cmr_u16)af_src_dat[0] << 8) & 0xff00) | af_src_dat[1];
    AF_MAC = (((cmr_u16)af_src_dat[4] << 8) & 0xff00) | af_src_dat[5];


    if(RG_GAIN && BG_GAIN && RG_GOLDEN_GAIN && BG_GOLDEN_GAIN) {
        r_gain_current = 2048 * RG_GOLDEN_GAIN / RG_GAIN;
        b_gain_current = 2048 * BG_GOLDEN_GAIN / BG_GAIN;
        g_gain_current = 2048;

        base_gain = (r_gain_current < b_gain_current) ? r_gain_current : b_gain_current;
        base_gain = (base_gain < g_gain_current) ? base_gain : g_gain_current;

        r_gain = 0x400 * r_gain_current / base_gain;
        g_gain = 0x400 * g_gain_current / base_gain;
        b_gain = 0x400 * b_gain_current / base_gain;

        *otp_ptr++ = (cmr_s32)RG_GAIN;
        *otp_ptr++ = (cmr_s32)BG_GAIN;
        *otp_ptr++ = (cmr_s32)r_gain;
        *otp_ptr++ = (cmr_s32)b_gain;
        *otp_ptr++ = (cmr_s32)g_gain;
        *otp_ptr++ = (cmr_s32)AF_INFI;
        *otp_ptr++ = (cmr_s32)AF_MAC;
    }

    return 0;
}
