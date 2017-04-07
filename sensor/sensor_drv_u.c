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
#define LOG_TAG "sensor_drv_u"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/trace.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "hw_sensor_drv.h"
#include "sensor_cfg.h"
#include "sensor_drv_u.h"

#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
#include "isp_cali_interface.h"
#include "isp_param_file_update.h"
#endif

#define SENSOR_CTRL_MSG_QUEUE_SIZE 10

#define SENSOR_CTRL_EVT_BASE (CMR_EVT_SENSOR_BASE + 0x800)
#define SENSOR_CTRL_EVT_INIT (SENSOR_CTRL_EVT_BASE + 0x0)
#define SENSOR_CTRL_EVT_EXIT (SENSOR_CTRL_EVT_BASE + 0x1)
#define SENSOR_CTRL_EVT_SETMODE (SENSOR_CTRL_EVT_BASE + 0x2)
#define SENSOR_CTRL_EVT_SETMODONE (SENSOR_CTRL_EVT_BASE + 0x3)
#define SENSOR_CTRL_EVT_CFGOTP (SENSOR_CTRL_EVT_BASE + 0x4)
#define SENSOR_CTRL_EVT_STREAM_CTRL (SENSOR_CTRL_EVT_BASE + 0x5)

/**---------------------------------------------------------------------------*
 **                         Local Variables                                   *
 **---------------------------------------------------------------------------*/
struct sensor_drv_context s_local_sensor_cxt[4];
/**---------------------------------------------------------------------------*
 **                         Local Functions                                   *
 **---------------------------------------------------------------------------*/
static cmr_int sensor_init_defaul_exif(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_post_set_mode(struct sensor_drv_context *sensor_cxt,
                               cmr_u32 mode, cmr_u32 is_inited);
static cmr_int sensor_handle_set_mode(struct sensor_drv_context *sensor_cxt, cmr_u32 mode,
                            cmr_u32 is_inited);
static cmr_int sensor_set_modone(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_stream_on(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_stream_off(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_set_id(struct sensor_drv_context *sensor_cxt,
                          SENSOR_ID_E sensor_id);
static cmr_int sensor_otp_process(struct sensor_drv_context *sensor_cxt,
                                  uint8_t cmd, uint8_t sub_cmd, void *data);
static cmr_int sensor_open(struct sensor_drv_context *sensor_cxt,
                        cmr_u32 sensor_id);
static cmr_int sns_dev_pwdn(struct sensor_drv_context *sensor_cxt,
                            cmr_u32 power_level);
static SENSOR_ID_E sensor_get_cur_id(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_set_mark(struct sensor_drv_context *sensor_cxt, cmr_u8 *buf);
static cmr_int sensor_get_mark(struct sensor_drv_context *sensor_cxt, cmr_u8 *buf,
                            cmr_u8 *is_saved_ptr);
static cmr_int snr_chk_snr_mode(SENSOR_MODE_INFO_T *mode_info);
static cmr_u32 sns_get_mipi_phy_id(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_create_ctrl_thread(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_ctrl_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int sensor_destroy_ctrl_thread(struct sensor_drv_context *sensor_cxt);
static cmr_int sensor_handle_stream_ctrl_common(struct sensor_drv_context *sensor_cxt,
                                      cmr_u32 on_off);
extern uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,
                                              SENSOR_ID_E sensor_id);
void sensor_rid_save_sensor_info(struct sensor_drv_context *sensor_cxt);

/***---------------------------------------------------------------------------*
 **                       Local function contents                              *
 **----------------------------------------------------------------------------*/
 
  /*the following interface will be deleted later*/
cmr_handle sensor_get_module_handle(cmr_handle handle){
	/*will be delete later.*/
	struct hw_drv_cxt
	{
		/*sensor device file descriptor*/
		cmr_s32 fd_sensor;
		/*sensor_id will be used mipi init*/
		cmr_u32 sensor_id;
		/*0-bit:reg value width,1-bit:reg addre width*/
		/*5,6,7-bit:i2c frequency*/
		cmr_u8 i2c_bus_config;
		/*sensor module handle,will be deleted later*/
		cmr_handle caller_handle;/**/
		/**/
	};
	struct hw_drv_cxt *hw_cxt = (struct hw_drv_cxt*)handle;
	struct sensor_drv_context *sensor_cxt =
			(struct sensor_drv_context *)hw_cxt->caller_handle;
	return (cmr_handle)sensor_cxt;
}

void sensor_set_cxt_common(struct sensor_drv_context *sensor_cxt) {
    SENSOR_ID_E sensor_id = 0;
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_id = sensor_cxt->sensor_register_info.cur_id;
#ifndef CONFIG_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT
    s_local_sensor_cxt[0] = *(struct sensor_drv_context *)sensor_cxt;
#else
    if (sensor_id == 0)
        s_local_sensor_cxt[0] = *(struct sensor_drv_context *)sensor_cxt;
    else if (sensor_id == 1)
        s_local_sensor_cxt[1] = *(struct sensor_drv_context *)sensor_cxt;
    else if (sensor_id == 2)
        s_local_sensor_cxt[2] = *(struct sensor_drv_context *)sensor_cxt;

    else if (sensor_id == 3)
        s_local_sensor_cxt[3] = *(struct sensor_drv_context *)sensor_cxt;
#endif

    return;
}

void *sensor_get_dev_cxt(void) { return (void *)&s_local_sensor_cxt[0]; }

void *sensor_get_dev_cxt_Ex(cmr_u32 camera_id) {
    struct sensor_drv_context *p_sensor_cxt = NULL;
    if (camera_id > CAMERA_ID_MAX)
        return NULL;
#ifndef CONFIG_CAMERA_SENSOR_MULTI_INSTANCE_SUPPORT
    p_sensor_cxt = &s_local_sensor_cxt[0];
#else
    if (camera_id == 0)
        p_sensor_cxt = &s_local_sensor_cxt[0];
    else if (camera_id == 1)
        p_sensor_cxt = &s_local_sensor_cxt[1];
    else if (camera_id == 2)
        p_sensor_cxt = &s_local_sensor_cxt[2];
    else if (camera_id == 3)
        p_sensor_cxt = &s_local_sensor_cxt[3];
#endif

    return (void *)p_sensor_cxt;
}

cmr_int sns_dev_get_flash_level(struct sensor_drv_context *sensor_cxt,
                                struct sensor_flash_level *level) {
    int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    ret = hw_sensor_get_flash_level(sensor_cxt->hw_drv_handle,level);
    if (0 != ret) {
        SENSOR_LOGE("_Sensor_Device_GetFlashLevel failed, ret=%d \n", ret);
        ret = -1;
    }

    return ret;
}

void Sensor_PowerOn(struct sensor_drv_context *sensor_cxt, cmr_u32 power_on) {
    ATRACE_BEGIN(__FUNCTION__);

    struct sensor_power_info_tag power_cfg;
    cmr_u32 power_down;
    SENSOR_IOCTL_FUNC_PTR power_func;
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

    if (PNULL == sensor_cxt->sensor_info_ptr) {
        SENSOR_LOGE("No sensor info!");
        return;
    }
    power_down = (cmr_u32)sensor_cxt->sensor_info_ptr->power_down_level;
    power_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->power;

    SENSOR_LOGI("power_on = %d, power_down_level = %d", power_on,
                power_down);

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    if (PNULL != power_func) {
        power_func(sensor_cxt->hw_drv_handle, power_on);
    }
    ATRACE_END();
}

void Sensor_PowerOn_Ex(struct sensor_drv_context *sensor_cxt,
                       cmr_u32 sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    struct sensor_power_info_tag power_cfg;
    cmr_u32 power_down;
    SENSOR_AVDD_VAL_E dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val;
    SENSOR_IOCTL_FUNC_PTR power_func;
    cmr_u32 rst_lvl = 0;
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

    sensor_set_id(sensor_cxt, (SENSOR_ID_E)sensor_id);

    sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[sensor_id];

    power_down = (cmr_u32)sensor_cxt->sensor_info_ptr->power_down_level;
    dvdd_val = sensor_cxt->sensor_info_ptr->dvdd_val;
    avdd_val = sensor_cxt->sensor_info_ptr->avdd_val;
    iovdd_val = sensor_cxt->sensor_info_ptr->iovdd_val;
    power_func = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->power;

    SENSOR_LOGI("power_down_level = %d, avdd_val = %d", power_down, avdd_val);
    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    if (PNULL != power_func) {
        power_func(sensor_cxt->hw_drv_handle, 1);
    } else {
        memset(&power_cfg, 0, sizeof(struct sensor_power_info_tag));
        power_cfg.is_on = 1;
        power_cfg.op_sensor_id = sensor_id;
        hw_sensor_power_config(sensor_cxt->hw_drv_handle, power_cfg);
    }

    ATRACE_END();
}

void sensor_set_export_Info(struct sensor_drv_context *sensor_cxt) {
    SENSOR_REG_TAB_INFO_T *resolution_info_ptr = PNULL;
    SENSOR_TRIM_T_PTR resolution_trim_ptr = PNULL;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;

    SENSOR_VIDEO_INFO_T *video_info_ptr = PNULL;
    cmr_u32 i = 0;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

    SENSOR_EXP_INFO_T *exp_info_ptr = &sensor_cxt->sensor_exp_info;
    if(PNULL == exp_info_ptr) {
        CMR_LOGE("X. sensor_exp_info is null");
        return;
    }
    SENSOR_MATCH_T *module = sensor_cxt->module_cxt;
    if (PNULL == sensor_cxt->sensor_info_ptr) {
        SENSOR_LOGE("X. sensor_info_ptr is null.");
        return;
    } else {
        sensor_info_ptr = sensor_cxt->sensor_info_ptr;
    }

    SENSOR_MEMSET(exp_info_ptr, 0x00, sizeof(SENSOR_EXP_INFO_T));
    exp_info_ptr->name = sensor_info_ptr->name;
    exp_info_ptr->image_format = sensor_info_ptr->image_format;
    exp_info_ptr->image_pattern = sensor_info_ptr->image_pattern;

    exp_info_ptr->pclk_polarity =
        (sensor_info_ptr->hw_signal_polarity &
         0x01); /*the high 3bit will be the phase(delay sel)*/
    exp_info_ptr->vsync_polarity =
        ((sensor_info_ptr->hw_signal_polarity >> 2) & 0x1);
    exp_info_ptr->hsync_polarity =
        ((sensor_info_ptr->hw_signal_polarity >> 4) & 0x1);
    exp_info_ptr->pclk_delay =
        ((sensor_info_ptr->hw_signal_polarity >> 5) & 0x07);

    if (NULL != sensor_info_ptr->raw_info_ptr) {
        exp_info_ptr->raw_info_ptr =
            (struct sensor_raw_info *)*sensor_info_ptr->raw_info_ptr;
    }

    if ((NULL != exp_info_ptr->raw_info_ptr) &&
        (NULL != exp_info_ptr->raw_info_ptr->resolution_info_ptr)) {
        exp_info_ptr->raw_info_ptr->resolution_info_ptr->image_pattern =
            sensor_info_ptr->image_pattern;
    }

    exp_info_ptr->source_width_max = sensor_info_ptr->source_width_max;
    exp_info_ptr->source_height_max = sensor_info_ptr->source_height_max;

    exp_info_ptr->environment_mode = sensor_info_ptr->environment_mode;
    exp_info_ptr->image_effect = sensor_info_ptr->image_effect;
    exp_info_ptr->wb_mode = sensor_info_ptr->wb_mode;
    exp_info_ptr->step_count = sensor_info_ptr->step_count;

    exp_info_ptr->ext_info_ptr = sensor_info_ptr->ext_info_ptr;

    exp_info_ptr->preview_skip_num = sensor_info_ptr->preview_skip_num;
    exp_info_ptr->capture_skip_num = sensor_info_ptr->capture_skip_num;
    exp_info_ptr->flash_capture_skip_num =
        sensor_info_ptr->flash_capture_skip_num;
    exp_info_ptr->mipi_cap_skip_num = sensor_info_ptr->mipi_cap_skip_num;
    exp_info_ptr->preview_deci_num = sensor_info_ptr->preview_deci_num;
    exp_info_ptr->change_setting_skip_num =
        sensor_info_ptr->change_setting_skip_num;
    exp_info_ptr->video_preview_deci_num =
        sensor_info_ptr->video_preview_deci_num;

    exp_info_ptr->threshold_eb = sensor_info_ptr->threshold_eb;
    exp_info_ptr->threshold_mode = sensor_info_ptr->threshold_mode;
    exp_info_ptr->threshold_start = sensor_info_ptr->threshold_start;
    exp_info_ptr->threshold_end = sensor_info_ptr->threshold_end;

    exp_info_ptr->ioctl_func_ptr = sensor_info_ptr->ioctl_func_tab_ptr;
    if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->get_trim) {
        /*the get trim function need not the sensor_cxt(the fd)*/
        resolution_trim_ptr =
            (SENSOR_TRIM_T_PTR)sensor_info_ptr->ioctl_func_tab_ptr->get_trim(
                sensor_cxt->hw_drv_handle, 0x00);
    }
    for (i = SENSOR_MODE_COMMON_INIT; i < SENSOR_MODE_MAX; i++) {
        resolution_info_ptr = &(sensor_info_ptr->resolution_tab_info_ptr[i]);

        if (SENSOR_IMAGE_FORMAT_JPEG == resolution_info_ptr->image_format) {
            exp_info_ptr->sensor_image_type = SENSOR_IMAGE_FORMAT_JPEG;
        }

        if ((PNULL != resolution_info_ptr->sensor_reg_tab_ptr) ||
            ((0x00 != resolution_info_ptr->width) &&
             (0x00 != resolution_info_ptr->width))) {
            exp_info_ptr->sensor_mode_info[i].mode = i;
            exp_info_ptr->sensor_mode_info[i].width =
                resolution_info_ptr->width;
            exp_info_ptr->sensor_mode_info[i].height =
                resolution_info_ptr->height;
            if ((PNULL != resolution_trim_ptr) &&
                (0x00 != resolution_trim_ptr[i].trim_width) &&
                (0x00 != resolution_trim_ptr[i].trim_height)) {
                exp_info_ptr->sensor_mode_info[i].trim_start_x =
                    resolution_trim_ptr[i].trim_start_x;
                exp_info_ptr->sensor_mode_info[i].trim_start_y =
                    resolution_trim_ptr[i].trim_start_y;
                exp_info_ptr->sensor_mode_info[i].trim_width =
                    resolution_trim_ptr[i].trim_width;
                exp_info_ptr->sensor_mode_info[i].trim_height =
                    resolution_trim_ptr[i].trim_height;
                exp_info_ptr->sensor_mode_info[i].line_time =
                    resolution_trim_ptr[i].line_time;
                exp_info_ptr->sensor_mode_info[i].bps_per_lane =
                    resolution_trim_ptr[i].bps_per_lane;
                exp_info_ptr->sensor_mode_info[i].frame_line =
                    resolution_trim_ptr[i].frame_line;
            } else {
                exp_info_ptr->sensor_mode_info[i].trim_start_x = 0x00;
                exp_info_ptr->sensor_mode_info[i].trim_start_y = 0x00;
                exp_info_ptr->sensor_mode_info[i].trim_width =
                    resolution_info_ptr->width;
                exp_info_ptr->sensor_mode_info[i].trim_height =
                    resolution_info_ptr->height;
            }

            /*scaler trim*/
            if ((PNULL != resolution_trim_ptr) &&
                (0x00 != resolution_trim_ptr[i].scaler_trim.w) &&
                (0x00 != resolution_trim_ptr[i].scaler_trim.h)) {
                exp_info_ptr->sensor_mode_info[i].scaler_trim =
                    resolution_trim_ptr[i].scaler_trim;
            } else {
                exp_info_ptr->sensor_mode_info[i].scaler_trim.x = 0x00;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.y = 0x00;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.w =
                    exp_info_ptr->sensor_mode_info[i].trim_width;
                exp_info_ptr->sensor_mode_info[i].scaler_trim.h =
                    exp_info_ptr->sensor_mode_info[i].trim_height;
            }

            if (SENSOR_IMAGE_FORMAT_MAX != sensor_info_ptr->image_format) {
                exp_info_ptr->sensor_mode_info[i].image_format =
                    sensor_info_ptr->image_format;
            } else {
                exp_info_ptr->sensor_mode_info[i].image_format =
                    resolution_info_ptr->image_format;
            }
        } else {
            exp_info_ptr->sensor_mode_info[i].mode = SENSOR_MODE_MAX;
        }
        if (PNULL != sensor_info_ptr->video_tab_info_ptr) {
            video_info_ptr = &sensor_info_ptr->video_tab_info_ptr[i];
            if (PNULL != video_info_ptr) {
                cmr_copy((void *)&exp_info_ptr->sensor_video_info[i],
                         (void *)video_info_ptr, sizeof(SENSOR_VIDEO_INFO_T));
            }
        }

        if ((NULL != exp_info_ptr) && (NULL != exp_info_ptr->raw_info_ptr) &&
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

        if ((NULL != exp_info_ptr) && (NULL != exp_info_ptr->raw_info_ptr) &&
            (NULL != exp_info_ptr->raw_info_ptr->ioctrl_ptr)) {
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->caller_handler =
                              sensor_cxt;
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_focus =
                              sensor_af_set_pos;
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_exposure =
                              sensor_ic_write_ae_value;
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_gain =
                              sensor_ic_write_gain;
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->ext_fuc = NULL;
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
            if (module && module->af_dev_info.af_drv_entry) {
                exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_pos =
                              sensor_af_set_pos;
                exp_info_ptr->raw_info_ptr->ioctrl_ptr->get_otp = NULL;
                exp_info_ptr->raw_info_ptr->ioctrl_ptr->get_motor_pos =
                              sensor_af_get_pos;
                exp_info_ptr->raw_info_ptr->ioctrl_ptr->sns_ioctl =
                              sensor_drv_ioctl;
                exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_motor_bestmode = NULL;
                exp_info_ptr->raw_info_ptr->ioctrl_ptr->get_test_vcm_mode = NULL;
                exp_info_ptr->raw_info_ptr->ioctrl_ptr->set_test_vcm_mode = NULL;
            } else {
                CMR_LOGE("AF device driver has problem,please double check "
                         "it.module:0x%x,af_dev:0x%x",
                         module, module ? module->af_dev_info.af_drv_entry : NULL);
            }
#endif
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->write_i2c =
                                   sensor_hw_WriteI2C;
            // exp_info_ptr->raw_info_ptr->ioctrl_ptr->read_i2c =
            // Sensor_ReadI2C;
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->ex_set_exposure =
                                   sensor_ic_ex_write_exposure;
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->read_aec_info =
                                   sensor_ic_read_aec_info;
            exp_info_ptr->raw_info_ptr->ioctrl_ptr->write_aec_info =
                                   sensor_muti_i2c_write;
        }
        // now we think sensor output width and height are equal to sensor
        // trim_width and trim_height
        exp_info_ptr->sensor_mode_info[i].out_width =
            exp_info_ptr->sensor_mode_info[i].trim_width;
        exp_info_ptr->sensor_mode_info[i].out_height =
            exp_info_ptr->sensor_mode_info[i].trim_height;
    }
    exp_info_ptr->sensor_interface = sensor_info_ptr->sensor_interface;
    exp_info_ptr->change_setting_skip_num =
        sensor_info_ptr->change_setting_skip_num;
    exp_info_ptr->horizontal_view_angle =
        sensor_info_ptr->horizontal_view_angle;
    exp_info_ptr->vertical_view_angle = sensor_info_ptr->vertical_view_angle;
    exp_info_ptr->sensor_version_info = sensor_info_ptr->sensor_version_info;

    SENSOR_LOGI("X");
}

LOCAL void sensor_clean_info(struct sensor_drv_context *sensor_cxt) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_cxt->sensor_mode[SENSOR_MAIN] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_SUB] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE2] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE3] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_info_ptr = PNULL;
    sensor_cxt->sensor_isInit = SENSOR_FALSE;
    sensor_cxt->sensor_index[SENSOR_MAIN] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_SUB] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_DEVICE2] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_DEVICE3] = 0xFF;
    sensor_cxt->sensor_index[SENSOR_ATV] = 0xFF;
    SENSOR_MEMSET(&sensor_cxt->sensor_exp_info, 0x00,
                  sizeof(SENSOR_EXP_INFO_T));
    sensor_register_info_ptr->cur_id = SENSOR_ID_MAX;
    return;
}

LOCAL cmr_int sensor_set_id(struct sensor_drv_context *sensor_cxt,
                   SENSOR_ID_E sensor_id) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_register_info_ptr->cur_id = sensor_id;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("id %d,is_register_sensor %ld", sensor_id,
                sensor_cxt->is_register_sensor);

    if (1 == sensor_cxt->is_register_sensor) {
        if ((SENSOR_MAIN == sensor_id) && (1 == sensor_cxt->is_main_sensor))
            return SENSOR_SUCCESS;
        if ((SENSOR_SUB == sensor_id) && (0 == sensor_cxt->is_main_sensor))
            return SENSOR_SUCCESS;
        if ((SENSOR_DEVICE2 == sensor_id) && (1 == sensor_cxt->is_main2_sensor))
            return SENSOR_SUCCESS;
        if ((SENSOR_DEVICE3 == sensor_id) && (0 == sensor_cxt->is_main2_sensor))
            return SENSOR_SUCCESS;
    }
    if ((SENSOR_MAIN <= sensor_id) || (SENSOR_DEVICE3 >= sensor_id)) {
        if (SENSOR_SUB == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (1 == sensor_cxt->is_main_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_MAIN);
            }
            sensor_cxt->is_main_sensor = 0;
        } else if (SENSOR_MAIN == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (0 == sensor_cxt->is_main_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_SUB);
            }
            sensor_cxt->is_main_sensor = 1;
        } else if (SENSOR_DEVICE3 == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (1 == sensor_cxt->is_main2_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE2);
            }
            sensor_cxt->is_main2_sensor = 0;
        } else if (SENSOR_DEVICE2 == sensor_id) {
            if ((1 == sensor_cxt->is_register_sensor) &&
                (0 == sensor_cxt->is_main2_sensor)) {
                hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE3);
            }
            sensor_cxt->is_main2_sensor = 1;
        }
        sensor_cxt->is_register_sensor = 0;
        if (hw_sensor_i2c_init(sensor_cxt->hw_drv_handle, sensor_id)) {
            if (SENSOR_MAIN == sensor_id) {
                sensor_cxt->is_main_sensor = 0;
            } else if (SENSOR_DEVICE2 == sensor_id) {
                sensor_cxt->is_main2_sensor = 0;
            }
            SENSOR_LOGI("add I2C error");
            return SENSOR_FAIL;
        } else {
            SENSOR_LOGV("add I2C OK.");
            sensor_cxt->is_register_sensor = 1;
        }
    }

    return SENSOR_SUCCESS;
}

LOCAL SENSOR_ID_E sensor_get_cur_id(struct sensor_drv_context *sensor_cxt) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    SENSOR_LOGV("id %d", sensor_register_info_ptr->cur_id);
    return (SENSOR_ID_E)sensor_register_info_ptr->cur_id;
}

LOCAL void sensor_i2c_init(struct sensor_drv_context *sensor_cxt,
                  SENSOR_ID_E sensor_id) {
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    sensor_register_info_ptr = PNULL;
    SENSOR_INFO_T **sensor_info_tab_ptr = PNULL;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 i2c_clock = 100000;
    cmr_u32 set_i2c_clock = 0;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_register_info_ptr->cur_id = sensor_id;

	if (hw_sensor_i2c_init(sensor_cxt->hw_drv_handle, sensor_id)) {
								SENSOR_LOGE("SENSOR: add I2C driver error");
								return;
		} else {
				SENSOR_LOGI("SENSOR: add I2C driver OK");
				sensor_cxt->is_register_sensor = 1;
		}
#if 0
    if (0 == sensor_cxt->is_register_sensor) {
        if ((SENSOR_MAIN <= sensor_id) || (SENSOR_DEVICE3 >= sensor_id)) {

            if (hw_sensor_i2c_init(sensor_cxt->hw_drv_handle, sensor_id)) {
                SENSOR_LOGE("SENSOR: add I2C driver error");
                return;
            } else {
                SENSOR_LOGI("SENSOR: add I2C driver OK");
                sensor_cxt->is_register_sensor = 1;
            }
        }
    } else {
        SENSOR_LOGI("Sensor: Init I2c %d ternimal! exits", sensor_id);
    }
#endif
    SENSOR_LOGI("sensor_id=%d, is_register_sensor=%ld", sensor_id,
                sensor_cxt->is_register_sensor);
}

LOCAL cmr_int sensor_i2c_deinit(struct sensor_drv_context *sensor_cxt,
                       SENSOR_ID_E sensor_id) {
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (1 == sensor_cxt->is_register_sensor) {
        if ((SENSOR_MAIN <= sensor_id) || (SENSOR_DEVICE3 >= sensor_id)) {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, sensor_id);
            sensor_cxt->is_register_sensor = 0;
            SENSOR_LOGI("delete I2C %d driver OK", sensor_id);
        }
    } else {
        SENSOR_LOGI("delete I2C %d driver OK", SENSOR_ID_MAX);
    }

    return SENSOR_SUCCESS;
}

LOCAL cmr_u32 sensor_identify_search(struct sensor_drv_context *sensor_cxt,
                            SENSOR_ID_E sensor_id) {
    cmr_u32 sensor_index = 0;
    SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;
    cmr_u32 valid_tab_index_max = 0x00;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 retValue = SCI_FALSE;
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr =
        &sensor_cxt->sensor_register_info;

    SENSOR_LOGI("search all sensor...");
    sensor_strinfo_tab_ptr =
        (SENSOR_MATCH_T *)Sensor_GetInforTab(sensor_cxt, sensor_id);
    valid_tab_index_max =
        Sensor_GetInforTabLenght(sensor_cxt, sensor_id) - SENSOR_ONE_I2C;
    sensor_i2c_init(sensor_cxt, sensor_id);

    // search the sensor in the table
    for (sensor_index = 0x00; sensor_index < valid_tab_index_max;
         sensor_index++) {
        sensor_info_ptr = sensor_strinfo_tab_ptr[sensor_index].sensor_info;
        if (NULL == sensor_info_ptr) {
            SENSOR_LOGW("%d info of Sensor table %d is null", sensor_index,
                        sensor_id);
            continue;
        }
        sensor_cxt->sensor_info_ptr = sensor_info_ptr;
        struct hw_drv_cfg_param hw_drv_cfg;
        hw_drv_cfg.i2c_bus_config = sensor_info_ptr->reg_addr_value_bits;
        hw_sensor_drv_cfg(sensor_cxt->hw_drv_handle, &hw_drv_cfg);

        Sensor_PowerOn(sensor_cxt, SCI_TRUE);

        if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->identify) {
            if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt)) {
                sensor_cxt->i2c_addr =
                    (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
                hw_sensor_i2c_set_addr(sensor_cxt->hw_drv_handle, sensor_cxt->i2c_addr);
            }
            SENSOR_LOGI("identify  Sensor 01");
            if (SENSOR_SUCCESS ==
                sensor_info_ptr->ioctl_func_tab_ptr->identify(
                    sensor_cxt->hw_drv_handle, SENSOR_ZERO_I2C)) {
                sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
                sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
                if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt))
                    sensor_cxt->sensor_index[sensor_id] = sensor_index;
                sensor_register_info_ptr->img_sensor_num++;
                Sensor_PowerOn(sensor_cxt, SCI_FALSE);
                retValue = SCI_TRUE;
                SENSOR_LOGI("sensor_id :%d,img_sensor_num=%d", sensor_id,
                            sensor_register_info_ptr->img_sensor_num);
                break;
            }
        }
        Sensor_PowerOn(sensor_cxt, SCI_FALSE);
    }
    sensor_i2c_deinit(sensor_cxt, sensor_id);
    if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
        SENSOR_LOGI("SENSOR TYPE of %d indentify OK", (cmr_u32)sensor_id);
        sensor_cxt->sensor_param_saved = SCI_TRUE;
    } else {
        SENSOR_LOGI("SENSOR TYPE of %d indentify failed!", (cmr_u32)sensor_id);
    }

    return retValue;
}

LOCAL cmr_u32 sensor_identify_strsearch(struct sensor_drv_context *sensor_cxt,
                               SENSOR_ID_E sensor_id) {
    cmr_u32 sensor_index = 0;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 retValue = SCI_FALSE;
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr =
        &sensor_cxt->sensor_register_info;
    cmr_u32 index_get = 0;
    SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;

    index_get = Sensor_IndexGet(sensor_cxt, sensor_id);
    if (index_get == 0xFF) {
        return retValue;
    }

    sensor_i2c_init(sensor_cxt, sensor_id);
    sensor_strinfo_tab_ptr =
        (SENSOR_MATCH_T *)Sensor_GetInforTab(sensor_cxt, sensor_id);
    sensor_info_ptr = sensor_strinfo_tab_ptr[index_get].sensor_info;

    if (NULL == sensor_info_ptr) {
        SENSOR_LOGW("%d info of Sensor table %d is null", index_get, sensor_id);
        return retValue;
    }
    sensor_cxt->sensor_info_ptr = sensor_info_ptr;
    struct hw_drv_cfg_param hw_drv_cfg;
    hw_drv_cfg.i2c_bus_config = sensor_info_ptr->reg_addr_value_bits;
    hw_sensor_drv_cfg(sensor_cxt->hw_drv_handle, &hw_drv_cfg);

    Sensor_PowerOn(sensor_cxt, SCI_TRUE);
    if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->identify) {
        if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt)) {
            sensor_cxt->i2c_addr =
                (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
           hw_sensor_i2c_set_addr(sensor_cxt->hw_drv_handle, sensor_cxt->i2c_addr);
        }
        SENSOR_LOGI("identify  Sensor 01");
        if (SENSOR_SUCCESS ==
            sensor_info_ptr->ioctl_func_tab_ptr->identify(
                sensor_cxt->hw_drv_handle, SENSOR_ZERO_I2C)) {
            sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
            sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
            if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt))
                sensor_cxt->sensor_index[sensor_id] = index_get;
            sensor_register_info_ptr->img_sensor_num++;
            // Sensor_PowerOn(sensor_cxt, SCI_FALSE);
            retValue = SCI_TRUE;
            SENSOR_LOGI("sensor_id :%d,img_sensor_num=%d", sensor_id,
                        sensor_register_info_ptr->img_sensor_num);
        }
    }
    Sensor_PowerOn(sensor_cxt, SCI_FALSE);
    sensor_i2c_deinit(sensor_cxt, sensor_id);
    if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
        SENSOR_LOGI("SENSOR TYPE of %d indentify OK", (cmr_u32)sensor_id);
        sensor_cxt->sensor_param_saved = SCI_TRUE;
    } else {
        SENSOR_LOGI("SENSOR TYPE of %d indentify failed!", (cmr_u32)sensor_id);
    }

    return retValue;
}

LOCAL cmr_int sensor_identify(struct sensor_drv_context *sensor_cxt,
                     SENSOR_ID_E sensor_id) {
    cmr_u32 sensor_index = 0;
    SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;
    cmr_u32 valid_tab_index_max = 0x00;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    cmr_u32 retValue = SCI_FALSE;
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_LOGI("sensor identifing %d", sensor_id);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    // if already identified
    if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
        SENSOR_LOGI("sensor identified");
        return SCI_TRUE;
    }
    if (sensor_cxt->sensor_identified && (SENSOR_ATV != sensor_id)) {
        sensor_index = sensor_cxt->sensor_index[sensor_id];
        SENSOR_LOGI("sensor_index = %d", sensor_index);
        if (0xFF != sensor_index) {
            sensor_strinfo_tab_ptr =
                (SENSOR_MATCH_T *)Sensor_GetInforTab(sensor_cxt, sensor_id);
            sensor_i2c_init(sensor_cxt, sensor_id);
            sensor_info_ptr = sensor_strinfo_tab_ptr[sensor_index].sensor_info;
            if (NULL == sensor_info_ptr) {
                SENSOR_LOGW("%d info of Sensor table %d is null", sensor_index,
                            sensor_id);
                sensor_i2c_deinit(sensor_cxt, sensor_id);
                goto IDENTIFY_SEARCH;
            }
            sensor_cxt->sensor_info_ptr = sensor_info_ptr;
            Sensor_PowerOn(sensor_cxt, SCI_TRUE);
            if (PNULL != sensor_info_ptr->ioctl_func_tab_ptr->identify) {
                sensor_cxt->i2c_addr =
                    (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
                hw_sensor_i2c_set_addr(sensor_cxt->hw_drv_handle, sensor_cxt->i2c_addr);

                SENSOR_LOGI("identify  Sensor 01");
                if (SENSOR_SUCCESS ==
                    sensor_info_ptr->ioctl_func_tab_ptr->identify(
                        sensor_cxt->hw_drv_handle, SENSOR_ZERO_I2C)) {
                    sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
                    sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
                    sensor_register_info_ptr->img_sensor_num++;
                    retValue = SCI_TRUE;
                    SENSOR_LOGI("sensor_id :%d,img_sensor_num=%d", sensor_id,
                                sensor_register_info_ptr->img_sensor_num);
                } else {
                    Sensor_PowerOn(sensor_cxt, SCI_FALSE);
                    sensor_i2c_deinit(sensor_cxt, sensor_id);
                    SENSOR_LOGI("identify failed!");
                    goto IDENTIFY_SEARCH;
                }
            }
            Sensor_PowerOn(sensor_cxt, SCI_FALSE);
            sensor_i2c_deinit(sensor_cxt, sensor_id);
            return retValue;
        }
    }

IDENTIFY_SEARCH:
    if (sensor_cxt->sensor_identified == SCI_FALSE &&
        ((strlen(CAMERA_SENSOR_TYPE_BACK)) ||
         (strlen(CAMERA_SENSOR_TYPE_FRONT)) ||
         strlen(CAMERA_SENSOR_TYPE_BACK_EXT) ||
         (strlen(AT_CAMERA_SENSOR_TYPE_BACK)) ||
         (strlen(AT_CAMERA_SENSOR_TYPE_FRONT)))) {
        retValue = sensor_identify_strsearch(sensor_cxt, sensor_id);
        if (retValue == SCI_TRUE) {
            return retValue;
        }
    }
    retValue = sensor_identify_search(sensor_cxt, sensor_id);
    return retValue;
}

LOCAL void sensor_set_status(struct sensor_drv_context *sensor_cxt,
                    SENSOR_ID_E sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 i = 0;
    cmr_u32 rst_lvl = 0;
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

    /*pwdn all the sensor to avoid confilct as the sensor output*/
    SENSOR_LOGV("1");
    for (i = 0; i <= SENSOR_DEVICE3; i++) {
        if (i == sensor_id) {
            continue;
        }
        if (SENSOR_TRUE == sensor_register_info_ptr->is_register[i]) {
            sensor_set_id(sensor_cxt, i);
            sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[i];
            if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt)) {
                sensor_cxt->i2c_addr =
                    (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
                hw_sensor_i2c_set_addr(sensor_cxt->hw_drv_handle, sensor_cxt->i2c_addr);
            }

            /*when use the camera vendor functions, the sensor_cxt should be set
             * at first */
            sensor_set_cxt_common(sensor_cxt);
#ifdef CONFIG_CAMERA_RT_REFOCUS
            if (i == SENSOR_SUB) {
                hw_sensor_set_reset_level(
                    sensor_cxt->hw_drv_handle,
                    (cmr_u32)sensor_cxt->sensor_info_ptr->reset_pulse_level);
                usleep(10 * 1000);
                SENSOR_LOGI("Sensor_sleep of id %d", i);
            }
#endif
        }
    }

    /*Give votage according the target sensor*/
    /*For dual sensor solution, the dual sensor should share all the power*/
    SENSOR_LOGV("1_1");

    Sensor_PowerOn_Ex(sensor_cxt, sensor_id);

    SENSOR_LOGI("2");

    sensor_set_id(sensor_cxt, sensor_id);
    sensor_cxt->sensor_info_ptr = sensor_cxt->sensor_list_ptr[sensor_id];
    SENSOR_LOGI("3");
    // reset target sensor. and make normal.
    sensor_set_export_Info(sensor_cxt);
    SENSOR_LOGI("4");

    ATRACE_END();
}

cmr_int sensor_register(struct sensor_drv_context *sensor_cxt,
                     SENSOR_ID_E sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 sensor_index = 0;
    SENSOR_MATCH_T *sensor_strinfo_tab_ptr = PNULL;
    cmr_u32 valid_tab_index_max = 0x00;
    SENSOR_INFO_T *sensor_info_ptr = PNULL;
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

    SENSOR_LOGI("id %d ", sensor_id);
    // if already identified
    if (SCI_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
        SENSOR_LOGI("identified ");
        return SENSOR_SUCCESS;
    }
    if (sensor_cxt->sensor_identified && (SENSOR_ATV != sensor_id)) {
        sensor_index = sensor_cxt->sensor_index[sensor_id];
        SENSOR_LOGI("sensor_index = %d.", sensor_index);
        if (0xFF != sensor_index) {
            valid_tab_index_max =
                Sensor_GetInforTabLenght(sensor_cxt, sensor_id) -
                SENSOR_ONE_I2C;
            if (sensor_index >= valid_tab_index_max) {
                SENSOR_LOGE("saved index is larger than sensor sum.");
                return SENSOR_FAIL;
            }

            sensor_strinfo_tab_ptr =
                (SENSOR_MATCH_T *)Sensor_GetInforTab(sensor_cxt, sensor_id);
            sensor_info_ptr = sensor_strinfo_tab_ptr[sensor_index].sensor_info;
            if (NULL == sensor_info_ptr) {
                SENSOR_LOGE("index %d info of Sensor table %d is null",
                            sensor_index, sensor_id);
                return SENSOR_FAIL;
            }

            if (NULL == sensor_info_ptr->raw_info_ptr) {
                SENSOR_LOGE(
                    "index %d info of Sensor table %d raw_info_ptr  is null",
                    sensor_index, sensor_id);
                return SENSOR_FAIL;
            }
            sensor_cxt->sensor_info_ptr = sensor_info_ptr;
            sensor_cxt->sensor_list_ptr[sensor_id] = sensor_info_ptr;
            sensor_register_info_ptr->is_register[sensor_id] = SCI_TRUE;
            sensor_register_info_ptr->img_sensor_num++;
            struct hw_drv_cfg_param hw_drv_cfg;
            hw_drv_cfg.i2c_bus_config = sensor_info_ptr->reg_addr_value_bits;
            hw_sensor_drv_cfg(sensor_cxt->hw_drv_handle, &hw_drv_cfg);
        }
    }

    ATRACE_END();
    return SENSOR_SUCCESS;
}

LOCAL void sensor_load_sensor_info(struct sensor_drv_context *sensor_cxt) {
    FILE *fp;
    cmr_u8 sensor_param[SENSOR_PARAM_NUM];
    cmr_u32 len = 0;

    cmr_bzero(&sensor_param[0], SENSOR_PARAM_NUM);

    fp = fopen(SENSOR_PARA, "rb+");
    if (NULL == fp) {
        fp = fopen(SENSOR_PARA, "wb+");
        if (NULL == fp) {
            SENSOR_LOGE("sensor_load_sensor_info: file %s open error:%s",
                        SENSOR_PARA, strerror(errno));
        }
        cmr_bzero(&sensor_param[0], SENSOR_PARAM_NUM);
    } else {
        len = fread(sensor_param, 1, SENSOR_PARAM_NUM, fp);
        SENSOR_LOGI("rd sns param len %d %x,%x,%x,%x,%x,%x,%x,%x ", len,
                    sensor_param[0], sensor_param[1], sensor_param[2],
                    sensor_param[3], sensor_param[4], sensor_param[5],
                    sensor_param[6], sensor_param[7]);
    }

    if (NULL != fp)
        fclose(fp);

    sensor_set_mark(sensor_cxt, sensor_param);
}

LOCAL void sensor_save_info2file(struct sensor_drv_context *sensor_cxt) {
    FILE *fp;
    cmr_u8 is_saved = 0;
    cmr_u8 sensor_param[SENSOR_PARAM_NUM];

    cmr_bzero(&sensor_param[0], SENSOR_PARAM_NUM);
    sensor_get_mark(sensor_cxt, sensor_param, &is_saved);

    if (is_saved) {
        fp = fopen(SENSOR_PARA, "wb+");
        if (NULL == fp) {
            SENSOR_LOGI("file %s open error:%s ", SENSOR_PARA, strerror(errno));
        } else {
            fwrite(sensor_param, 1, SENSOR_PARAM_NUM, fp);
            fclose(fp);
        }
    }
}

cmr_int Sensor_set_calibration(cmr_u32 value) {
    struct sensor_drv_context *sensor_cxt =
        (struct sensor_drv_context *)sensor_get_dev_cxt();
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_cxt->is_calibration = value;
    return SENSOR_SUCCESS;
}

void _sensor_calil_lnc_param_recover(struct sensor_drv_context *sensor_cxt,
                                     SENSOR_INFO_T *sensor_info_ptr) {
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    cmr_u32 i = 0;
    cmr_u32 index = 0;
    cmr_u32 length = 0;
    cmr_uint addr = 0;
    struct sensor_raw_fix_info *raw_fix_info_ptr = PNULL;
    struct sensor_raw_info *raw_info_ptr = PNULL;

    raw_info_ptr = (struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr));

    SENSOR_DRV_CHECK_ZERO_VOID(sensor_cxt);

    if (PNULL != raw_info_ptr) {
        raw_fix_info_ptr = raw_info_ptr->fix_ptr;
        if (PNULL != raw_fix_info_ptr) {
            for (i = 0; i < 8; i++) {
                if (sensor_cxt->lnc_addr_bakup[i][1]) {

                    free((void *)sensor_cxt->lnc_addr_bakup[i][1]);
                    sensor_cxt->lnc_addr_bakup[i][1] = 0;
                    index = sensor_cxt->lnc_addr_bakup[i][0];  /*index*/
                    length = sensor_cxt->lnc_addr_bakup[i][3]; /*length*/
                    addr =
                        sensor_cxt->lnc_addr_bakup[i][2]; /*original address*/

                    raw_fix_info_ptr->lnc.map[index][0].param_addr =
                        (cmr_u16 *)addr;
                    raw_fix_info_ptr->lnc.map[index][0].len = length;
                    sensor_cxt->lnc_addr_bakup[i][0] = 0;
                    sensor_cxt->lnc_addr_bakup[i][1] = 0;
                    sensor_cxt->lnc_addr_bakup[i][2] = 0;
                    sensor_cxt->lnc_addr_bakup[i][3] = 0;
                }
            }
        } else {
            for (i = 0; i < 8; i++) {
                if (sensor_cxt->lnc_addr_bakup[i][1]) {
                    free((void *)sensor_cxt->lnc_addr_bakup[i][1]);
                }
            }
            cmr_bzero((void *)&sensor_cxt->lnc_addr_bakup[0][0],
                      sizeof(sensor_cxt->lnc_addr_bakup));
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (sensor_cxt->lnc_addr_bakup[i][1]) {
                free((void *)sensor_cxt->lnc_addr_bakup[i][1]);
            }
        }
        cmr_bzero((void *)&sensor_cxt->lnc_addr_bakup[0][0],
                  sizeof(sensor_cxt->lnc_addr_bakup));
    }

    sensor_cxt->is_calibration = 0;
    SENSOR_LOGI("test: is_calibration: %d", sensor_cxt->is_calibration);
#endif
}

cmr_int _sensor_cali_lnc_param_update(struct sensor_drv_context *sensor_cxt,
                                      cmr_s8 *cfg_file_dir,
                                      SENSOR_INFO_T *sensor_info_ptr,
                                      SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);
    cmr_u32 rtn = SENSOR_SUCCESS;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    const cmr_s8 *sensor_name = sensor_info_ptr->name;
    FILE *fp = PNULL;
    cmr_s8 file_name[80] = {0};
    cmr_s8 *file_name_ptr = 0;
    cmr_u32 str_len = 0;
    cmr_int file_pos = 0;
    cmr_u32 file_size = 0;
    cmr_s8 *data_ptr;
    cmr_s32 i, j;
    cmr_u16 *temp_buf_16 = PNULL;
    cmr_u32 width;
    cmr_u32 height;
    cmr_u32 index = 0;
    SENSOR_TRIM_T *trim_ptr = 0;
    struct sensor_raw_fix_info *raw_fix_info_ptr = PNULL;

    if (SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format) {
        rtn = SENSOR_FAIL;
        goto cali_lnc_param_update_exit;
    }

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (PNULL == sensor_cxt->sensor_info_ptr) {
        SENSOR_LOGE("No sensor info!");
        return -1;
    }

    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    /*LNC DATA Table*/
    temp_buf_16 = (cmr_u16 *)malloc(128 * 1024 * 2);
    if (!temp_buf_16) {
        rtn = SENSOR_FAIL;
        goto cali_lnc_param_update_exit;
    }
    /*the get trim function need not set the sensor_cxt to work(mainly fd)*/
    trim_ptr = (SENSOR_TRIM_T *)(sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr
                                     ->get_trim(0));
    raw_fix_info_ptr =
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))->fix_ptr;
    i = 1;
    while (1) {
        height = trim_ptr[i].trim_height;
        width = trim_ptr[i].trim_width;
        if ((0 == height) || (0 == width)) {
            break;
        }

        sprintf(file_name_ptr, "_lnc_%d_%d_%d_rdm.dat", width, height, (i - 1));

        fp = fopen(file_name, "rb");
        if (0 == fp) {
            SENSOR_LOGI("does not find calibration file");
            i++;
            continue;
        }

        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            free(temp_buf_16);
            temp_buf_16 = NULL;
            SENSOR_LOGI("file pointers error!");
            rtn = SENSOR_FAIL;
            goto cali_lnc_param_update_exit;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(temp_buf_16, 1, file_size, fp);
        fclose(fp);

        if (file_size != raw_fix_info_ptr->lnc.map[i - 1][0].len) {
            SENSOR_LOGI("file size dis-match, do not replace, w:%d, h:%d, ori: "
                        "%d, now:%d/n",
                        width, height, raw_fix_info_ptr->lnc.map[i - 1][0].len,
                        file_size);
        } else {
            if (sensor_cxt->lnc_addr_bakup[index][1]) {
                free((void *)sensor_cxt->lnc_addr_bakup[index][1]);
                sensor_cxt->lnc_addr_bakup[index][1] = 0;
            }
            sensor_cxt->lnc_addr_bakup[index][1] = (cmr_uint)malloc(file_size);
            if (0 == sensor_cxt->lnc_addr_bakup[index][1]) {
                rtn = SENSOR_FAIL;
                SENSOR_LOGI("malloc failed i = %d", i);
                goto cali_lnc_param_update_exit;
            }
            cmr_bzero((void *)sensor_cxt->lnc_addr_bakup[index][1], file_size);

            sensor_cxt->lnc_addr_bakup[index][0] = i - 1;
            sensor_cxt->lnc_addr_bakup[index][2] =
                (cmr_uint)raw_fix_info_ptr->lnc.map[i - 1][0]
                    .param_addr; /*save the original address*/
            sensor_cxt->lnc_addr_bakup[index][3] = file_size;
            data_ptr = (cmr_s8 *)sensor_cxt->lnc_addr_bakup[index][1];
            raw_fix_info_ptr->lnc.map[i - 1][0].param_addr =
                (cmr_u16 *)data_ptr;
            cmr_copy(data_ptr, temp_buf_16, file_size);
            index++;
            SENSOR_LOGI("replace finished");
        }
        i++;
    }

    if (temp_buf_16) {
        free((void *)temp_buf_16);
        temp_buf_16 = 0;
    }
    return rtn;

cali_lnc_param_update_exit:

    if (temp_buf_16) {
        free((void *)temp_buf_16);
        temp_buf_16 = 0;
    }

    _sensor_calil_lnc_param_recover(sensor_cxt, sensor_info_ptr);
#endif
    return rtn;
}

cmr_int _sensor_cali_awb_param_update(cmr_s8 *cfg_file_dir,
                                      SENSOR_INFO_T *sensor_info_ptr,
                                      SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);

    cmr_int rtn = 0;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    const cmr_s8 *sensor_name = sensor_info_ptr->name;
    FILE *fp = PNULL;
    cmr_s8 file_name[80] = {0};
    cmr_s8 buf[256] = {0x00};
    cmr_s8 *file_name_ptr = 0;
    cmr_u32 str_len = 0;
    cmr_int file_pos = 0;
    cmr_u32 file_size = 0;
    struct isp_bayer_ptn_stat_t *stat_ptr = PNULL;
    struct sensor_cali_info *cali_info_ptr = PNULL;
    struct sensor_raw_tune_info *raw_tune_info_ptr = PNULL;

    if (SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format) {
        return SENSOR_FAIL;
    }
    raw_tune_info_ptr =
        (struct sensor_raw_tune_info *)(((struct sensor_raw_info *)(*(
                                             sensor_info_ptr->raw_info_ptr)))
                                            ->tune_ptr);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->awb.cali_info);

    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_awb_rdm.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;
        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGI("file pointers error!");
            return SENSOR_FAIL;
        }

        fseek(fp, 0L, SEEK_SET);
        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }

    cmr_bzero(&file_name[0], sizeof(file_name));
    cmr_bzero(&buf[0], sizeof(buf));
    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_awb_gldn.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->awb.golden_cali_info);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGI("file pointers error!");
            return SENSOR_FAIL;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }
#endif
    return rtn;
}

cmr_int _sensor_cali_flashlight_param_update(cmr_s8 *cfg_file_dir,
                                             SENSOR_INFO_T *sensor_info_ptr,
                                             SENSOR_ID_E sensor_id) {
    UNUSED(sensor_id);

    cmr_int rtn = 0;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    const cmr_s8 *sensor_name = sensor_info_ptr->name;
    FILE *fp = PNULL;
    cmr_s8 file_name[80] = {0};
    cmr_s8 buf[256] = {0x00};
    cmr_s8 *file_name_ptr = 0;
    cmr_u32 str_len = 0;
    cmr_int file_pos = 0;
    cmr_u32 file_size = 0;
    struct isp_bayer_ptn_stat_t *stat_ptr = PNULL;
    struct sensor_cali_info *cali_info_ptr = PNULL;
    struct sensor_raw_tune_info *raw_tune_info_ptr = PNULL;

    if (SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format) {
        return SENSOR_FAIL;
    }
    raw_tune_info_ptr =
        (struct sensor_raw_tune_info *)(((struct sensor_raw_info *)(*(
                                             sensor_info_ptr->raw_info_ptr)))
                                            ->tune_ptr);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->flashlight.cali_info);

    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_flashlight_rdm.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);
        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGI("file pointers error!");
            return SENSOR_FAIL;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }

    cmr_bzero(&file_name[0], sizeof(file_name));
    cmr_bzero(&buf[0], sizeof(buf));
    str_len = sprintf(file_name, "%ssensor_%s", cfg_file_dir, sensor_name);
    file_name_ptr = (cmr_s8 *)&file_name[0] + str_len;

    sprintf(file_name_ptr, "_flashlight_gldn.dat");

    SENSOR_LOGI("file_name: %s", file_name);
    cali_info_ptr = (struct sensor_cali_info *)&(
        ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
            ->cali_ptr->flashlight.golden_cali_info);
    fp = fopen(file_name, "rb");
    if (0 == fp) {
        SENSOR_LOGI("does not find calibration file");

        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        rtn = SENSOR_SUCCESS;
        return rtn;
    } else {
        fseek(fp, 0L, SEEK_END);

        file_pos = ftell(fp);
        if (file_pos >= 0) {
            file_size = (cmr_u32)file_pos;
        } else {
            fclose(fp);
            SENSOR_LOGI("file pointers error!");
            return SENSOR_FAIL;
        }
        fseek(fp, 0L, SEEK_SET);

        fread(buf, 1, file_size, fp);
        fclose(fp);

        stat_ptr = (struct isp_bayer_ptn_stat_t *)&buf[0];
        cali_info_ptr->r_sum = stat_ptr->r_stat;
        cali_info_ptr->b_sum = stat_ptr->b_stat;
        cali_info_ptr->gr_sum = stat_ptr->gr_stat;
        cali_info_ptr->gb_sum = stat_ptr->gb_stat;

        rtn = SENSOR_SUCCESS;
    }
#endif
    return rtn;
}

cmr_int _sensor_cali_load_param(struct sensor_drv_context *sensor_cxt,
                                cmr_s8 *cfg_file_dir,
                                SENSOR_INFO_T *sensor_info_ptr,
                                SENSOR_ID_E sensor_id) {
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
    cmr_int rtn = 0;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (1 != sensor_cxt->is_calibration) { /*for normal*/

        rtn = _sensor_cali_lnc_param_update(sensor_cxt, cfg_file_dir,
                                            sensor_info_ptr, sensor_id);
        if (rtn) {

            return SENSOR_FAIL;
        }
        rtn = _sensor_cali_flashlight_param_update(cfg_file_dir,
                                                   sensor_info_ptr, sensor_id);
        if (rtn) {
            return SENSOR_FAIL;
        }
        rtn = _sensor_cali_awb_param_update(cfg_file_dir, sensor_info_ptr,
                                            sensor_id);
        if (rtn) {
            return SENSOR_FAIL;
        }
    } else { /*for calibration*/
        struct sensor_cali_info *cali_info_ptr = PNULL;

        /*for awb calibration*/
        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->awb.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        /*for flash  calibration*/
        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;

        cali_info_ptr = (struct sensor_cali_info *)&(
            ((struct sensor_raw_info *)(*(sensor_info_ptr->raw_info_ptr)))
                ->cali_ptr->flashlight.golden_cali_info);
        cali_info_ptr->r_sum = 1024;
        cali_info_ptr->b_sum = 1024;
        cali_info_ptr->gr_sum = 1024;
        cali_info_ptr->gb_sum = 1024;
    }
#endif
    return SENSOR_SUCCESS;
}

LOCAL cmr_int sensor_create_ctrl_thread(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    SENSOR_LOGI("is_inited %ld", sensor_cxt->ctrl_thread_cxt.is_inited);

    if (!sensor_cxt->ctrl_thread_cxt.is_inited) {
        pthread_mutex_init(&sensor_cxt->ctrl_thread_cxt.sensor_mutex, NULL);
        sem_init(&sensor_cxt->ctrl_thread_cxt.sensor_sync_sem, 0, 0);

        ret = cmr_thread_create(&sensor_cxt->ctrl_thread_cxt.thread_handle,
                                SENSOR_CTRL_MSG_QUEUE_SIZE,
                                sensor_ctrl_thread_proc, (void *)sensor_cxt);
        if (ret) {
            SENSOR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
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

    SENSOR_LOGI("ret %ld", ret);
    return ret;
}

LOCAL cmr_int sensor_ctrl_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 evt = 0, on_off = 0;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)p_data;

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
        sensor_handle_set_mode(sensor_cxt, mode, is_inited);
    } break;

    case SENSOR_CTRL_EVT_SETMODONE:
        SENSOR_LOGI("SENSOR_CTRL_EVT_SETMODONE OK");
        break;

    case SENSOR_CTRL_EVT_CFGOTP:
        camera_id = camera_id;
        otp_ctrl_cmd_t *otp_ctrl_data = (otp_ctrl_cmd_t *)message->data;
        sensor_otp_process(sensor_cxt, otp_ctrl_data->cmd,
                           otp_ctrl_data->sub_cmd, otp_ctrl_data->data);
        break;
    case SENSOR_CTRL_EVT_STREAM_CTRL:
        on_off = (cmr_u32)message->sub_msg_type;
        sensor_handle_stream_ctrl_common(sensor_cxt, on_off);
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

LOCAL cmr_int sensor_destroy_ctrl_thread(struct sensor_drv_context *sensor_cxt) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

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

/*********************************************************************************
 todo:
 now the sensor_open_common only support open one sensor on 1 times, and the
 function
 should be updated when the hardware can supported multiple sensors;
 *********************************************************************************/
cmr_int sensor_open_common(struct sensor_drv_context *sensor_cxt,
                           cmr_u32 sensor_id, cmr_uint is_autotest) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret_val = SENSOR_FAIL;
    cmr_u32 sensor_num = 0;
    sensor_init_log_level();
    struct hw_drv_init_para input_ptr;

    SENSOR_LOGI("0, start,id %d autotest %ld", sensor_id, is_autotest);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    if (NULL != sensor_cxt) {
        if (SENSOR_TRUE == sensor_is_init_common(sensor_cxt)) {
            SENSOR_LOGI("sensor close.");
            sensor_close_common(sensor_cxt, SENSOR_MAIN);
        }
    }

    cmr_bzero((void *)sensor_cxt, sizeof(struct sensor_drv_context));
    sensor_cxt->fd_sensor = SENSOR_FD_INIT;
    sensor_cxt->i2c_addr = 0xff;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    {
        sensor_clean_info(sensor_cxt);
        sensor_init_defaul_exif(sensor_cxt);
        sensor_load_sensor_info(sensor_cxt);
        sensor_cxt->is_autotest = is_autotest;
        if (sensor_create_ctrl_thread(sensor_cxt)) {
            SENSOR_LOGE("Failed to create sensor ctrl thread");
            goto init_exit;
            ;
        }
        /*now device opened set with ID*/
        /*hw handle instance should be created as soon as possible */
        input_ptr.sensor_id = sensor_id;
		input_ptr.caller_handle = sensor_cxt;
        sensor_cxt->fd_sensor = hw_sensor_drv_create(&input_ptr,&sensor_cxt->hw_drv_handle);
        if (SENSOR_FD_INIT == sensor_cxt->fd_sensor) {
            SENSOR_LOGE("sns_device_init %d error, return", sensor_id);
            ret_val = SENSOR_FAIL;
            goto init_exit;
        }
        sensor_cxt->sensor_hw_handler = sensor_cxt->hw_drv_handle;
        if (SCI_TRUE == sensor_cxt->sensor_identified) {
            if (SENSOR_SUCCESS == sensor_register(sensor_cxt, SENSOR_MAIN)) {
                sensor_num++;
            }
#ifndef CONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
            if (SENSOR_SUCCESS == sensor_register(sensor_cxt, SENSOR_SUB)) {
                sensor_num++;
            }
#endif
#ifdef CONFIG_DCAM_SENSOR2_SUPPORT
            if (SENSOR_SUCCESS == sensor_register(sensor_cxt, SENSOR_DEVICE2)) {
                sensor_num++;
            }
#endif

#ifdef CONFIG_DCAM_SENSOR3_SUPPORT
            if (SENSOR_SUCCESS == sensor_register(sensor_cxt, SENSOR_DEVICE3)) {
                sensor_num++;
            }
#endif
            SENSOR_LOGI("1 is identify, register OK");
            ret_val = sensor_open(sensor_cxt, sensor_id);
            if (ret_val != SENSOR_SUCCESS) {
                Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
            }
        }

        if (ret_val != SENSOR_SUCCESS) {
            sensor_num = 0;
            SENSOR_LOGI("register sensor fail, start identify");
            if (sensor_identify(sensor_cxt, SENSOR_MAIN))
                sensor_num++;
#ifndef CONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
            if (sensor_identify(sensor_cxt, SENSOR_SUB))
                sensor_num++;
#endif
#ifdef CONFIG_DCAM_SENSOR2_SUPPORT
            if (sensor_identify(sensor_cxt, SENSOR_DEVICE2))
                sensor_num++;
#endif
#ifdef CONFIG_DCAM_SENSOR3_SUPPORT
            if (sensor_identify(sensor_cxt, SENSOR_DEVICE3))
                sensor_num++;
#endif

            ret_val = sensor_open(sensor_cxt, sensor_id);
        }
        sensor_cxt->sensor_identified = SCI_TRUE;
    }

    sensor_save_info2file(sensor_cxt);
    sensor_rid_save_sensor_info(sensor_cxt);
    SENSOR_LOGI("1 debug %p", sensor_cxt->sensor_info_ptr); // for debug
    //	SENSOR_LOGI("2 debug %d", sensor_cxt->sensor_info_ptr->image_format);
    ////for debug
    if (sensor_cxt->sensor_info_ptr &&
        SENSOR_IMAGE_FORMAT_RAW == sensor_cxt->sensor_info_ptr->image_format) {
        if (SENSOR_SUCCESS == ret_val) {
            ret_val =
                _sensor_cali_load_param(sensor_cxt, CALI_FILE_DIR,
                                        sensor_cxt->sensor_info_ptr, sensor_id);
            if (ret_val) {
                SENSOR_LOGI("load cali data failed!! rtn:%ld", ret_val);
                goto init_exit;
            }
        }
    }

    SENSOR_LOGI("total camera number %d", sensor_num);

init_exit:
    if (SENSOR_SUCCESS != ret_val) {
        sensor_destroy_ctrl_thread(sensor_cxt);
        //sns_device_deinit(sensor_cxt);
    }

    SENSOR_LOGI("2 init OK!");
    ATRACE_END();
    return ret_val;
}

cmr_int sensor_is_init_common(struct sensor_drv_context *sensor_cxt) {
    return sensor_cxt->sensor_isInit;
}

LOCAL cmr_int sensor_open(struct sensor_drv_context *sensor_cxt, cmr_u32 sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 ret_val = SENSOR_FAIL;
    cmr_u32 is_inited = 0;
    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;
    SENSOR_MATCH_T *module = NULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;

    if (1 == sensor_cxt->is_autotest) {
        is_inited = 1;
    }
    if (SENSOR_TRUE == sensor_register_info_ptr->is_register[sensor_id]) {
        SENSOR_LOGI("1, sensor register ok");
        sensor_set_status(sensor_cxt, sensor_id);
        SENSOR_LOGI("2, sensor set status");
        sensor_cxt->sensor_isInit = SENSOR_TRUE;

        sensor_i2c_init(sensor_cxt, sensor_id);
        if (SENSOR_ATV != sensor_get_cur_id(sensor_cxt)) {
            sensor_cxt->i2c_addr =
                (sensor_cxt->sensor_info_ptr->salve_i2c_addr_w & 0xFF);
            hw_sensor_i2c_set_addr(sensor_cxt->hw_drv_handle, sensor_cxt->i2c_addr);
        }

        SENSOR_LOGI("3:sensor_id: %d, addr = 0x%x", sensor_id,
                    sensor_cxt->i2c_addr);
        hw_sensor_i2c_set_clk(sensor_cxt->hw_drv_handle);

        /*when use the camera vendor functions, the sensor_cxt should be set at
         * first */
        sensor_set_cxt_common(sensor_cxt);

        ATRACE_BEGIN("sensor_identify");
        // confirm camera identify OK
        if (SENSOR_SUCCESS !=
            sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->identify(
                sensor_cxt->hw_drv_handle, SENSOR_ZERO_I2C)) {
            sensor_register_info_ptr->is_register[sensor_id] = SENSOR_FALSE;
            sensor_i2c_deinit(sensor_cxt, sensor_id);
            SENSOR_LOGI("sensor identify not correct!!");
            return SENSOR_FAIL;
        }
        ATRACE_END();
        module = (SENSOR_MATCH_T *)Sensor_GetInforTab(sensor_cxt, sensor_id) +
                 sensor_cxt->sensor_index[sensor_id];
        sensor_cxt->module_cxt = module;
        sensor_set_export_Info(sensor_cxt);

        ret_val = SENSOR_SUCCESS;
        if (SENSOR_SUCCESS !=
            sensor_post_set_mode(sensor_cxt, SENSOR_MODE_COMMON_INIT, is_inited)) {
            SENSOR_LOGE("Sensor set init mode error!");
            sensor_i2c_deinit(sensor_cxt, sensor_id);
            ret_val = SENSOR_FAIL;
        }
        if (SENSOR_SUCCESS == ret_val) {
            if (SENSOR_SUCCESS != sensor_post_set_mode(sensor_cxt,
                                                  SENSOR_MODE_PREVIEW_ONE,
                                                  is_inited)) {
                SENSOR_LOGE("Sensor set init mode error!");
                sensor_i2c_deinit(sensor_cxt, sensor_id);
                ret_val = SENSOR_FAIL;
            }
        }
        sensor_otp_module_init(sensor_cxt);
        if ((SENSOR_IMAGE_FORMAT_RAW ==
             sensor_cxt->sensor_info_ptr->image_format) &&
            module && module->otp_drv_info) {
            sensor_otp_rw_ctrl(sensor_cxt, OTP_READ_PARSE_DATA, 0, NULL);
        }
        sensor_af_init(sensor_cxt);
        sensor_cxt->stream_on = 1;
        sensor_handle_stream_ctrl_common(sensor_cxt, 0);
        SENSOR_LOGI("4 open success");
    } else {
        SENSOR_LOGE("Sensor not register, open failed, sensor_id = %d",
                    sensor_id);
    }

    if ((SENSOR_SUCCESS == ret_val) && (1 == sensor_cxt->is_autotest)) {
        hw_Sensor_SetMode_WaitDone(sensor_cxt->hw_drv_handle);
    }

    ATRACE_END();
    return ret_val;
}

LOCAL cmr_int sensor_post_set_mode(struct sensor_drv_context *sensor_cxt, cmr_u32 mode,
                        cmr_u32 is_inited) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    message.msg_type = SENSOR_CTRL_EVT_SETMODE;
    message.sub_msg_type = mode;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)((cmr_uint)is_inited);
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    return ret;
}

static cmr_int sensor_set_modone(struct sensor_drv_context *sensor_cxt) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_SETMODONE;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_handle_set_mode(struct sensor_drv_context *sensor_cxt, cmr_u32 mode,
                     cmr_u32 is_inited) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int rtn;
    cmr_u32 mclk;
    SENSOR_IOCTL_FUNC_PTR set_reg_tab_func = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (PNULL == sensor_cxt->sensor_info_ptr) {
        SENSOR_LOGE("No sensor info!");
        return -1;
    }

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    set_reg_tab_func =
        sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->cus_func_1;

    SENSOR_LOGI("mode = %d.", mode);
    if (SENSOR_FALSE == sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGI("sensor has not init");
        return SENSOR_OP_STATUS_ERR;
    }

    if (sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)] == mode) {
        SENSOR_LOGI("The sensor mode as before");
    } else {
        if (PNULL !=
            sensor_cxt->sensor_info_ptr->resolution_tab_info_ptr[mode]
                .sensor_reg_tab_ptr) {
            mclk = sensor_cxt->sensor_info_ptr->resolution_tab_info_ptr[mode]
                       .xclk_to_sensor;
            hw_sensor_set_mclk(sensor_cxt->hw_drv_handle, mclk);
            sensor_cxt->sensor_exp_info.image_format =
                sensor_cxt->sensor_exp_info.sensor_mode_info[mode].image_format;

            if ((SENSOR_MODE_COMMON_INIT == mode) && set_reg_tab_func) {
                set_reg_tab_func(sensor_cxt->hw_drv_handle,
                                 SENSOR_MODE_COMMON_INIT);
            } else {
                hw_Sensor_SendRegTabToSensor(sensor_cxt->hw_drv_handle,
                    &sensor_cxt->sensor_info_ptr->resolution_tab_info_ptr[mode]);
            }
            sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)] = mode;
        } else {
            if (set_reg_tab_func)
                set_reg_tab_func(sensor_cxt->hw_drv_handle, 0);
            SENSOR_LOGI("No this resolution information !!!");
        }
    }

    if (is_inited) {
        if (SENSOR_INTERFACE_TYPE_CSI2 ==
            sensor_cxt->sensor_info_ptr->sensor_interface.type) {
            rtn = sensor_stream_off(
                sensor_cxt); /*stream off first for MIPI sensor switch*/
            if (SENSOR_SUCCESS == rtn) {
                //				sns_dev_mipi_deinit(sensor_cxt);
            }
        }
    }

    ATRACE_END();
    return SENSOR_SUCCESS;
}

cmr_int sensor_set_mode_common(struct sensor_drv_context *sensor_cxt,
                               cmr_uint mode) {
    cmr_int ret = 0;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    ret = sensor_post_set_mode(sensor_cxt, mode, 1);
    return ret;
}

cmr_int hw_Sensor_SetMode(cmr_handle handle, cmr_u32 mode) {
    cmr_int ret = 0;

	struct sensor_drv_context *sensor_cxt = 
		sensor_get_module_handle(handle);

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    ret = sensor_post_set_mode(sensor_cxt, mode, 1);
    return ret;
}

cmr_int sensor_set_modone_common(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = 0;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    ret = sensor_set_modone(sensor_cxt);
    return ret;
}

cmr_int hw_Sensor_SetMode_WaitDone(cmr_handle handle) {
    cmr_int ret = 0;
	struct sensor_drv_context *sensor_cxt = 
		sensor_get_module_handle(handle);

    sensor_set_modone(sensor_cxt);
    return ret;
}

cmr_int sensor_get_mode_common(struct sensor_drv_context *sensor_cxt,
                               cmr_uint *mode) {
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (SENSOR_FALSE == sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGI("sensor has not init");
        return SENSOR_OP_STATUS_ERR;
    }
    *mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
    return SENSOR_SUCCESS;
}

cmr_int hw_Sensor_GetMode(cmr_handle handle, cmr_u32 *mode) {
	struct sensor_drv_context *sensor_cxt = 
		sensor_get_module_handle(handle);

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (SENSOR_FALSE == sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGI("sensor has not init");
        return SENSOR_OP_STATUS_ERR;
    }
    *mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
    return SENSOR_SUCCESS;
}

LOCAL cmr_int sensor_stream_on(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int err = 0xff;
    cmr_u32 param = 0;
    SENSOR_IOCTL_FUNC_PTR stream_on_func;

    SENSOR_LOGI("E");

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (!sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGE("X: sensor has not been initialized");
        return SENSOR_FAIL;
    }

    if (PNULL == sensor_cxt->sensor_info_ptr) {
        SENSOR_LOGE("X: No sensor info!");
        return -1;
    }

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    // sensor_set_cxt_common(sensor_cxt);

    if (!sensor_cxt->stream_on) {
        stream_on_func =
            sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->stream_on;

        if (PNULL != stream_on_func) {
            param = sensor_cxt->bypass_mode;
            err = stream_on_func(sensor_cxt->hw_drv_handle, param);
        }

        if (0 == err) {
            sensor_cxt->stream_on = 1;
        }
    }

    SENSOR_LOGI("X");
    ATRACE_END();
    return err;
}

cmr_int sensor_stream_ctrl_common(struct sensor_drv_context *sensor_cxt,
                                  cmr_u32 on_off) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    /*the set mode function can be async control*/
    message.msg_type = SENSOR_CTRL_EVT_STREAM_CTRL;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.sub_msg_type = on_off;
    ret = cmr_thread_msg_send(sensor_cxt->ctrl_thread_cxt.thread_handle,
                              &message);
    if (ret) {
        SENSOR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_handle_stream_ctrl_common(struct sensor_drv_context *sensor_cxt,
                               cmr_u32 on_off) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    cmr_u32 mode;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    if (on_off) {
        if (SENSOR_INTERFACE_TYPE_CSI2 ==
            sensor_cxt->sensor_info_ptr->sensor_interface.type) {
            mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
            struct hw_mipi_init_param init_param;
            init_param.lane_num = sensor_cxt->sensor_exp_info.sensor_interface.bus_width;
            init_param.bps_per_lane =
                         sensor_cxt->sensor_exp_info.sensor_mode_info[mode].bps_per_lane;
            ret = hw_sensor_mipi_init(sensor_cxt->hw_drv_handle, init_param);
            if (ret) {
                SENSOR_LOGE("mipi initial failed ret %d", ret);
            } else {
                SENSOR_LOGE("mipi initial ok");
            }
        }
        ret = sensor_stream_on(sensor_cxt);
    } else {
        ret = sensor_stream_off(sensor_cxt);
        if (SENSOR_SUCCESS == ret) {
            if (SENSOR_INTERFACE_TYPE_CSI2 ==
                sensor_cxt->sensor_info_ptr->sensor_interface.type) {
                hw_sensor_mipi_deinit(sensor_cxt->hw_drv_handle);
                mode = sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
            }
        }
    }

    ATRACE_END();
    return ret;
}

LOCAL cmr_int sensor_stream_off(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int err = 0xff;
    cmr_u32 param = 0;
    SENSOR_IOCTL_FUNC_PTR stream_off_func;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    if (!sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGE("X: sensor has not been initialized");
        return SENSOR_FAIL;
    }

    if (PNULL == sensor_cxt->sensor_info_ptr) {
        SENSOR_LOGE("X: No sensor info!");
        return -1;
    }

    /*when use the camera vendor functions, the sensor_cxt should be set at
     * first */
    sensor_set_cxt_common(sensor_cxt);

    stream_off_func =
        sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr->stream_off;

    if (PNULL != stream_off_func && sensor_cxt->stream_on == 1) {
        err = stream_off_func(sensor_cxt->hw_drv_handle, param);
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
    if (!sensor_is_init_common(sensor_cxt)) {
        SENSOR_LOGE("sensor has not init");
        return SENSOR_FAIL;
    }

    SENSOR_LOGV("info = 0x%lx ", (void *)&sensor_cxt->sensor_exp_info);
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
    ATRACE_BEGIN(__FUNCTION__);

    UNUSED(sensor_id);

    SENSOR_REGISTER_INFO_T_PTR sensor_register_info_ptr = PNULL;

    SENSOR_LOGI("E");
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sensor_register_info_ptr = &sensor_cxt->sensor_register_info;
    sensor_otp_module_deinit(sensor_cxt);
    sensor_af_deinit(sensor_cxt);
    sensor_destroy_ctrl_thread(sensor_cxt);

    sensor_stream_off(sensor_cxt);
    hw_sensor_mipi_deinit(sensor_cxt->hw_drv_handle);
    if (1 == sensor_cxt->is_register_sensor) {
        if (1 == sensor_cxt->is_main_sensor) {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_MAIN);
        } else {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_SUB);
        }
        if (1 == sensor_cxt->is_main2_sensor) {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE2);
        } else {
            hw_sensor_i2c_deinit(sensor_cxt->hw_drv_handle, SENSOR_DEVICE3);
        }
        sensor_cxt->is_register_sensor = 0;
        sensor_cxt->is_main_sensor = 0;
        sensor_cxt->is_main2_sensor = 0;
    }

    if (SENSOR_TRUE == sensor_is_init_common(sensor_cxt)) {
        if (SENSOR_IMAGE_FORMAT_RAW ==
            sensor_cxt->sensor_info_ptr->image_format) {
            if (0 == sensor_cxt->is_calibration) {
                _sensor_calil_lnc_param_recover(sensor_cxt,
                                                sensor_cxt->sensor_info_ptr);
            }
        }
        Sensor_PowerOn(sensor_cxt, SENSOR_FALSE);
    }
    SENSOR_LOGI("9.");
    hw_sensor_drv_delete(sensor_cxt->hw_drv_handle);
    sensor_cxt->sensor_hw_handler = NULL;
    sensor_cxt->hw_drv_handle = NULL;
    sensor_cxt->sensor_isInit = SENSOR_FALSE;
    sensor_cxt->sensor_mode[SENSOR_MAIN] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_SUB] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE2] = SENSOR_MODE_MAX;
    sensor_cxt->sensor_mode[SENSOR_DEVICE3] = SENSOR_MODE_MAX;
    SENSOR_LOGI("X");

    ATRACE_END();
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

cmr_int sensor_set_exif_common(struct sensor_drv_context *sensor_cxt,
                               SENSOR_EXIF_CTRL_E cmd, cmr_u32 param) {
    SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
    EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = PNULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
    if (PNULL == sensor_info_ptr) {
        SENSOR_LOGW("sensor not ready yet, direct return");
        return SENSOR_FAIL;
    } else if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
        sensor_exif_info_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)
                                   sensor_info_ptr->ioctl_func_ptr->get_exif(
                                       sensor_cxt->hw_drv_handle, 0x00);
    } else {
        SENSOR_LOGV("the fun is null, set it to default");
        sensor_exif_info_ptr = &sensor_cxt->default_exif;
    }
    switch (cmd) {
    case SENSOR_EXIF_CTRL_EXPOSURETIME: {
        enum sensor_mode img_sensor_mode =
            sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
        cmr_u32 exposureline_time =
            sensor_info_ptr->sensor_mode_info[img_sensor_mode].line_time;
        cmr_u32 exposureline_num = param;
        cmr_u32 exposure_time = 0x00;

        exposure_time = exposureline_time * exposureline_num / 10;
        sensor_exif_info_ptr->valid.ExposureTime = 1;

        if (0x00 == exposure_time) {
            sensor_exif_info_ptr->valid.ExposureTime = 0;
        } else if (1000000 >= exposure_time) {
            sensor_exif_info_ptr->ExposureTime.numerator = 0x01;
            sensor_exif_info_ptr->ExposureTime.denominator =
                1000000 / exposure_time;
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
                1000000 / exposure_time;
            sensor_exif_info_ptr->ExposureTime.numerator =
                sensor_exif_info_ptr->ExposureTime.denominator * second;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_FNUMBER:
        sensor_exif_info_ptr->valid.FNumber = 1;
        sensor_exif_info_ptr->FNumber.numerator = param;
        sensor_exif_info_ptr->FNumber.denominator = 10;
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
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!sensor_cxt) {
        return CMR_CAMERA_FAIL;
    }

    ret = isp_raw_para_update_from_file(sensor_cxt->sensor_info_ptr, sensor_id);
    return ret;
}

cmr_int hw_Sensor_SetSensorExifInfo(cmr_handle handle,
                                    SENSOR_EXIF_CTRL_E cmd, cmr_u32 param) {
	struct sensor_drv_context *sensor_cxt = 
		sensor_get_module_handle(handle);

    SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
    EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = PNULL;

    if(!sensor_cxt) {
        SENSOR_LOGE("sensor cxt is null,do nothing");
        return SENSOR_FAIL;
    }
    sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
    if (PNULL == sensor_info_ptr) {
        SENSOR_LOGW("sensor not ready yet, direct return");
        return SENSOR_FAIL;
    } else if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
        SENSOR_LOGI("get_exif_info enter ioctol");
        sensor_exif_info_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)
                                   sensor_info_ptr->ioctl_func_ptr->get_exif(
                                       sensor_cxt->hw_drv_handle, 0x00);
    } else {
        SENSOR_LOGI("the fun is null, set it to default");
        sensor_exif_info_ptr = &sensor_cxt->default_exif;
    }

    switch (cmd) {
    case SENSOR_EXIF_CTRL_EXPOSURETIME: {
        enum sensor_mode img_sensor_mode =
            sensor_cxt->sensor_mode[sensor_get_cur_id(sensor_cxt)];
        cmr_u32 exposureline_time =
            sensor_info_ptr->sensor_mode_info[img_sensor_mode].line_time;
        cmr_u32 exposureline_num = param;
        cmr_u32 exposure_time = 0x00;

        exposure_time = exposureline_time * exposureline_num / 1000;
        sensor_exif_info_ptr->valid.ExposureTime = 1;

        if (0x00 == exposure_time) {
            sensor_exif_info_ptr->valid.ExposureTime = 0;
        } else if (1000000 >= exposure_time) {
            sensor_exif_info_ptr->ExposureTime.numerator = 0x01;
            sensor_exif_info_ptr->ExposureTime.denominator =
                ((10000000 / exposure_time) + 5) / 10;
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
                1000000 / exposure_time;
            sensor_exif_info_ptr->ExposureTime.numerator =
                sensor_exif_info_ptr->ExposureTime.denominator * second;
        }
        break;
    }
    case SENSOR_EXIF_CTRL_FNUMBER:
        sensor_exif_info_ptr->valid.FNumber = 1;
        sensor_exif_info_ptr->FNumber.numerator = param;
        sensor_exif_info_ptr->FNumber.denominator = 10;
        break;
    case SENSOR_EXIF_CTRL_EXPOSUREPROGRAM:
        break;
    case SENSOR_EXIF_CTRL_SPECTRALSENSITIVITY:
        break;
    case SENSOR_EXIF_CTRL_ISOSPEEDRATINGS:
        sensor_exif_info_ptr->valid.ISOSpeedRatings = 1;
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
            sensor_exif_info_ptr->LightSource = 0x03;
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

cmr_int
sensor_get_exif_common(struct sensor_drv_context *sensor_cxt,
                       EXIF_SPEC_PIC_TAKING_COND_T **sensor_exif_info_pptr) {
    SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    sensor_get_info_common(sensor_cxt, &sensor_info_ptr);
    if (!sensor_info_ptr) {
        SENSOR_LOGE("ZERO poiner sensor_cxt %p sensor_info_ptr %p", sensor_cxt,
                    sensor_info_ptr);
        return SENSOR_SUCCESS;
    }

    if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
        *sensor_exif_info_pptr = (EXIF_SPEC_PIC_TAKING_COND_T *)
                                     sensor_info_ptr->ioctl_func_ptr->get_exif(
                                         sensor_cxt->hw_drv_handle, 0x00);
        SENSOR_LOGI("get_exif.");
    } else {
        // struct sensor_drv_context *cur_cxt = (struct
        // sensor_drv_context*)sensor_get_dev_cxt();
        *sensor_exif_info_pptr = &sensor_cxt->default_exif;
        // memcpy((void*)&sensor_cxt->default_exif,
        // (void*)&cur_cxt->default_exif,
        //	sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
    }
    return SENSOR_SUCCESS;
}

EXIF_SPEC_PIC_TAKING_COND_T *
hw_Sensor_GetSensorExifInfo(cmr_handle handle) {

	struct sensor_drv_context *sensor_cxt = 
		sensor_get_module_handle(handle);

    if(!sensor_cxt) {
        SENSOR_LOGE("sensor cxt is null,do nothing");
        return NULL;
    }

    SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
    EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = PNULL;

    if (!sensor_cxt) {
        SENSOR_LOGE("ZERO poiner sensor_cxt %p", sensor_cxt);
        return NULL;
    }

    sensor_get_info_common(sensor_cxt, &sensor_info_ptr);

    if (!sensor_info_ptr) {
        SENSOR_LOGE("ZERO poiner sensor_info_ptr %p", sensor_info_ptr);
        return NULL;
    }

    if (PNULL != sensor_info_ptr->ioctl_func_ptr->get_exif) {
        sensor_exif_info_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)
                                   sensor_info_ptr->ioctl_func_ptr->get_exif(
                                       sensor_cxt->hw_drv_handle, 0x00);
        SENSOR_LOGI("get_exif.");
    } else {
        sensor_exif_info_ptr = &sensor_cxt->default_exif;
        SENSOR_LOGI("fun null, so use the default");
    }
    return sensor_exif_info_ptr;
}

LOCAL cmr_int sensor_set_mark(struct sensor_drv_context *sensor_cxt, cmr_u8 *buf) {
    cmr_u32 i;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    if ((SIGN_0 != buf[0]) && (SIGN_1 != buf[1]) && (SIGN_2 != buf[2]) &&
        (SIGN_3 != buf[3])) {
        sensor_cxt->sensor_identified = SCI_FALSE;
    } else {
        sensor_cxt->sensor_identified = SCI_TRUE;
        for (i = 0; i < 4; i++) {
            sensor_cxt->sensor_index[i] = buf[4 + i];
        }
    }
    SENSOR_LOGI("%d,idex %d,%d.", sensor_cxt->sensor_identified,
                sensor_cxt->sensor_index[SENSOR_MAIN],
                sensor_cxt->sensor_index[SENSOR_SUB]);
    return 0;
}

cmr_int sensor_get_mark(struct sensor_drv_context *sensor_cxt, cmr_u8 *buf,
                     cmr_u8 *is_saved_ptr) {
    cmr_u32 i, j = 0;
    cmr_u8 *ptr = buf;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    if (SCI_TRUE == sensor_cxt->sensor_param_saved) {
        *is_saved_ptr = 1;
        *ptr++ = SIGN_0;
        *ptr++ = SIGN_1;
        *ptr++ = SIGN_2;
        *ptr++ = SIGN_3;
        for (i = 0; i < 4; i++) {
            *ptr++ = sensor_cxt->sensor_index[i];
        }
        SENSOR_LOGI("index is %d,%d.", sensor_cxt->sensor_index[SENSOR_MAIN],
                    sensor_cxt->sensor_index[SENSOR_SUB]);
    } else {
        *is_saved_ptr = 0;
    }
    return 0;
}

LOCAL cmr_int sensor_otp_rw_ctrl(struct sensor_drv_context *sensor_cxt,
                                  uint8_t cmd,uint8_t sub_cmd,void* data)
{
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    otp_ctrl_cmd_t *otp_cmd = malloc(sizeof(otp_ctrl_cmd_t));
    if (!otp_cmd) {
        CMR_LOGE("otp cmd buffer alloc failed!");
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
        CMR_LOGE("send msg failed!");
        if (message.data) {
            free(message.data);
        }
        return CMR_CAMERA_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_otp_process(struct sensor_drv_context *sensor_cxt, uint8_t cmd,
                           uint8_t sub_cmd, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = 0;
    struct _sensor_val_tag param;
    SENSOR_MATCH_T *module = sensor_cxt->module_cxt;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    if (module && module->otp_drv_info && sensor_cxt->otp_drv_handle) {
        switch (cmd) {
        case OTP_READ_RAW_DATA:
            ret = module->otp_drv_info->otp_ops.sensor_otp_read(
                sensor_cxt->otp_drv_handle, data);
            break;
        case OTP_READ_PARSE_DATA:
            module->otp_drv_info->otp_ops.sensor_otp_read(
                          sensor_cxt->otp_drv_handle, NULL);
            ret = module->otp_drv_info->otp_ops.sensor_otp_parse(
                               sensor_cxt->otp_drv_handle, data);
            if (ret == CMR_CAMERA_SUCCESS) {
                ret = module->otp_drv_info->otp_ops.sensor_otp_calibration(
                        sensor_cxt->otp_drv_handle);
            } else {
                SENSOR_LOGE("otp prase failed");
                return ret;
            }
            break;
        case OTP_WRITE_DATA:
            ret = module->otp_drv_info->otp_ops.sensor_otp_write(
                sensor_cxt->otp_drv_handle, NULL);
            if (ret != CMR_CAMERA_SUCCESS)
                return ret;
            break;
        case OTP_IOCTL:
            ret = module->otp_drv_info->otp_ops.sensor_otp_ioctl(
                sensor_cxt->otp_drv_handle, sub_cmd, data);
            if (ret != CMR_CAMERA_SUCCESS)
                return ret;
            break;
        default:
            break;
        }
    } else {
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
    }
    ATRACE_END();
    return ret;
}

cmr_int snr_chk_snr_mode(SENSOR_MODE_INFO_T *mode_info) {
    cmr_int ret = 0;
    if (mode_info) {
        /*jpeg format do not crop*/
        if (SENSOR_IMAGE_FORMAT_JPEG == mode_info->image_format) {
            if ((0 != mode_info->trim_start_x) ||
                (0 != mode_info->trim_start_y)) {
                ret = -1;
                goto out;
            }
        }
        if (((mode_info->trim_start_x + mode_info->trim_width) >
             mode_info->width) ||
            ((mode_info->trim_start_y + mode_info->trim_height) >
             mode_info->height) ||
            ((mode_info->scaler_trim.x + mode_info->scaler_trim.w) >
             mode_info->trim_width) ||
            ((mode_info->scaler_trim.y + mode_info->scaler_trim.h) >
             mode_info->trim_height)) {
            ret = -1;
        }
    }

out:
    return ret;
}

cmr_int hw_Sensor_SetFlash(SENSOR_HW_HANDLE handle, uint32_t is_open) {
    UNUSED(is_open);

    // now some NULL process
    return 0;
}

#include <cutils/properties.h>

void sensor_rid_save_sensor_info(struct sensor_drv_context *sensor_cxt) {
    char sensor_info[80] = {0};
    const char *const sensorInterface0 =
        "/sys/devices/virtual/misc/sprd_sensor/camera_sensor_name";
    ssize_t wr_ret;
    cmr_u32 fd;
    cmr_s8 value[PROPERTY_VALUE_MAX];

    property_get("persist.sys.sensor.id", value, "0");
    if (!strcmp(value, "trigger_srid")) {
        SENSOR_LOGI("srid E\n");
        if (sensor_cxt->sensor_list_ptr[SENSOR_MAIN] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->sensor_version_info !=
                NULL &&
            strlen(
                sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->sensor_version_info) >
                1)
            sprintf(
                sensor_info, "%s ",
                sensor_cxt->sensor_list_ptr[SENSOR_MAIN]->sensor_version_info);
        if (sensor_cxt->sensor_list_ptr[SENSOR_SUB] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_SUB]->sensor_version_info !=
                NULL &&
            strlen(
                sensor_cxt->sensor_list_ptr[SENSOR_SUB]->sensor_version_info) >
                1)
            sprintf(
                sensor_info, "%s %s ", sensor_info,
                sensor_cxt->sensor_list_ptr[SENSOR_SUB]->sensor_version_info);
        if (sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]->sensor_version_info !=
                NULL &&
            strlen(sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]
                       ->sensor_version_info) > 1)
            sprintf(sensor_info, "%s %s ", sensor_info,
                    sensor_cxt->sensor_list_ptr[SENSOR_DEVICE2]
                        ->sensor_version_info);
        if (sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3] != NULL &&
            sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]->sensor_version_info !=
                NULL &&
            strlen(sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]
                       ->sensor_version_info) > 1)
            sprintf(sensor_info, "%s %s ", sensor_info,
                    sensor_cxt->sensor_list_ptr[SENSOR_DEVICE3]
                        ->sensor_version_info);
        SENSOR_LOGE("WRITE srid %s \n", sensor_info);

        fd = open(sensorInterface0, O_WRONLY | O_TRUNC);
        if (-1 == fd) {
            SENSOR_LOGE("Failed to open: sensorInterface");
            goto ERROR;
        }
        wr_ret = write(fd, sensor_info, strlen(sensor_info));
        if (-1 == wr_ret) {
            SENSOR_LOGE("WRITE FAILED \n");
            goto ERROR;
        }
        property_set("sys.sensor.info", sensor_info);
        property_set("persist.sys.sensor.id", "0");
    ERROR:
        close(fd);
    }
}

/*--------------------------AF INTERFACE-----------------------------*/

LOCAL cmr_int sensor_af_init(cmr_handle sns_module_handle)
{
    cmr_int ret = SENSOR_SUCCESS;
    struct af_drv_init_para input_ptr;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = (SENSOR_MATCH_T *)sensor_cxt->module_cxt;

    if (module && module->af_dev_info.af_drv_entry && (!sensor_cxt->af_drv_handle)) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        hw_sensor_set_monitor_val(sensor_cxt->hw_drv_handle,
                                  module->af_dev_info.af_drv_entry->motor_avdd_val);
        SENSOR_LOGI("af power is %d",module->af_dev_info.af_drv_entry->motor_avdd_val);
        if(af_ops->create) {
            input_ptr.af_work_mode = module->af_dev_info.af_work_mode;
            input_ptr.hw_handle = sensor_cxt->hw_drv_handle;
            ret = af_ops->create(&input_ptr,&sensor_cxt->af_drv_handle);
            if(SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("ERROR:module:0x%x,entry:0x%x,af_ops:0x%x",
                           module, module?module->af_dev_info.af_drv_entry:NULL, af_ops);
        return SENSOR_FAIL;
    }
    return ret;
}

LOCAL cmr_int sensor_af_deinit(cmr_handle sns_module_handle)
{
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = (SENSOR_MATCH_T *)sensor_cxt->module_cxt;

    SENSOR_LOGI("E:");
    if (module && module->af_dev_info.af_drv_entry && sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        hw_sensor_set_monitor_val(sensor_cxt->hw_drv_handle,SENSOR_AVDD_CLOSED);
        if(af_ops->delete) {
            ret = af_ops->delete(sensor_cxt->af_drv_handle,NULL);
            sensor_cxt->af_drv_handle = NULL;
            if(SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("AF_DEINIT:Don't register af driver,return directly'");
        return SENSOR_FAIL;
    }
    SENSOR_LOGI("X:");
    return ret;
}

LOCAL cmr_int sensor_af_set_pos(cmr_handle sns_module_handle, uint16_t pos)
{
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = (SENSOR_MATCH_T *)sensor_cxt->module_cxt;

    if (module && module->af_dev_info.af_drv_entry && sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if(af_ops->set_pos) {
            ret = af_ops->set_pos(sensor_cxt->af_drv_handle, pos);
            if(SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        } else {
            ret = SENSOR_FAIL;
            SENSOR_LOGE("set_pos is null please check your af driver.");
        }
    } else {
        SENSOR_LOGE("af driver not exist,return directly");
        return SENSOR_FAIL;
    }

    return ret;
}

LOCAL cmr_int sensor_af_get_pos(cmr_handle sns_module_handle, uint16_t *pos)
{
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = NULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    SENSOR_MATCH_T *module = (SENSOR_MATCH_T *)sensor_cxt->module_cxt;

    if (module && module->af_dev_info.af_drv_entry && sensor_cxt->af_drv_handle) {
        af_ops = &module->af_dev_info.af_drv_entry->af_ops;
        if(af_ops->get_pos) {
            ret = af_ops->get_pos(sensor_cxt->af_drv_handle, pos);
            if(SENSOR_SUCCESS != ret)
                return SENSOR_FAIL;
        }
    } else {
        SENSOR_LOGE("af driver not exist,return directly");
        return SENSOR_FAIL;
    }
    return ret;
}

/*--------------------------OTP INTERFACE-----------------------------*/

LOCAL cmr_int sensor_otp_module_init(struct sensor_drv_context *sensor_cxt) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("in");
    SENSOR_MATCH_T *module = sensor_cxt->module_cxt;
    otp_drv_init_para_t input_para = {0};

    if (module && (module->otp_drv_info) && (!sensor_cxt->otp_drv_handle)) {
        input_para.hw_handle = sensor_cxt->hw_drv_handle;
        input_para.sensor_name = sensor_cxt->sensor_info_ptr->name;
        ret = module->otp_drv_info->otp_ops.sensor_otp_create(&input_para,&sensor_cxt->otp_drv_handle);
    } else {
        SENSOR_LOGE("error:Don't register otp_driver please double check! ");
    }
    SENSOR_LOGI("out");
    ATRACE_END();
    return ret;
}

LOCAL cmr_int sensor_otp_module_deinit(struct sensor_drv_context *sensor_cxt) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_LOGI("in");
    SENSOR_MATCH_T *module = sensor_cxt->module_cxt;

    if (module && (module->otp_drv_info) && sensor_cxt->otp_drv_handle) {
        ret = module->otp_drv_info->otp_ops.sensor_otp_delete(
            sensor_cxt->otp_drv_handle);
        sensor_cxt->otp_drv_handle = NULL;
    } else {
        SENSOR_LOGE("Don't has otp instance,no need to release.");
    }
    SENSOR_LOGI("out");
    return ret;
}

/*--------------------------HARDWARE INTERFACE-----------------------------*/
cmr_int sensor_hw_ReadI2C(cmr_handle sns_module_handle, cmr_u16 slave_addr,
                           cmr_u8 *cmd, cmr_u16 cmd_length) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    ret = hw_Sensor_ReadI2C(sensor_cxt->hw_drv_handle,slave_addr,cmd,cmd_length);
    return ret;
}

cmr_int sensor_hw_WriteI2C(cmr_handle sns_module_handle, cmr_u16 slave_addr,
                           cmr_u8 *cmd, cmr_u16 cmd_length) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    ret = hw_Sensor_WriteI2C(sensor_cxt->hw_drv_handle,slave_addr,cmd,cmd_length);
    return ret;
}

cmr_int sensor_muti_i2c_write(cmr_handle sns_module_handle,
                        struct sensor_muti_aec_i2c_tag *aec_i2c_info) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    ret = hw_sensor_muti_i2c_write(sensor_cxt->hw_drv_handle, aec_i2c_info);
    return ret;

}
/*--------------------------SENSOR IC INTERFACE-----------------------------*/

cmr_uint sensor_ic_write_gain(cmr_handle sns_module_handle, cmr_uint param){
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IOCTL_FUNC_TAB_T_PTR ioctl_func_ptr = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    ioctl_func_ptr = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr;

    if(!(ioctl_func_ptr && ioctl_func_ptr->write_gain_value)) {
        CMR_LOGE("write_gain is NULL,return");
        return SENSOR_FAIL;
    }
    ret = ioctl_func_ptr->write_gain_value(sensor_cxt->hw_drv_handle,param);

    return ret;

}

unsigned long sensor_ic_ex_write_exposure(cmr_handle sns_module_handle, unsigned long param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IOCTL_FUNC_TAB_T_PTR ioctl_func_ptr = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    ioctl_func_ptr = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr;

    if(!(ioctl_func_ptr && ioctl_func_ptr->ex_write_exp)) {
        CMR_LOGE("ex_write_exp is NULL,return");
        return SENSOR_FAIL;
    }
    ret = ioctl_func_ptr->ex_write_exp(sensor_cxt->hw_drv_handle,param);

    return ret;
}

unsigned long sensor_ic_write_ae_value(cmr_handle sns_module_handle, unsigned long param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IOCTL_FUNC_TAB_T_PTR ioctl_func_ptr = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    ioctl_func_ptr = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr;

    if(!(ioctl_func_ptr && ioctl_func_ptr->write_ae_value)) {
        CMR_LOGE("write_ae_value is NULL,return");
        return SENSOR_FAIL;
    }
    ret = ioctl_func_ptr->write_ae_value(sensor_cxt->hw_drv_handle,param);

    return ret;
}

unsigned long sensor_ic_read_aec_info(cmr_handle sns_module_handle, unsigned long param){
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IOCTL_FUNC_TAB_T_PTR ioctl_func_ptr = PNULL;
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;

    SENSOR_DRV_CHECK_ZERO(sensor_cxt);
    SENSOR_DRV_CHECK_ZERO(sensor_cxt->sensor_info_ptr);

    ioctl_func_ptr = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr;

    if(!(ioctl_func_ptr && ioctl_func_ptr->read_aec_info)) {
        CMR_LOGE("ex_write_exp is NULL,return");
        return SENSOR_FAIL;
    }
    ret = ioctl_func_ptr->read_aec_info(sensor_cxt->hw_drv_handle,param);

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
 * if you want add some sub commands.please add them in common/inc/cmr_sensor_info.h
 *
 * Return:
 * SENSOR_SUCCESS : ioctl success
 * SENSOR_FAIL : ioctl failed
 **/
cmr_int sensor_drv_ioctl(cmr_handle sns_module_handle,enum sns_cmd cmd,void* param)
{
    cmr_int ret = SENSOR_SUCCESS;
    struct sns_af_drv_ops *af_ops = PNULL;
    sensor_otp_ops_t *otp_ops = PNULL;
    SENSOR_MATCH_T *module = PNULL;
    SENSOR_DRV_CHECK_ZERO(sns_module_handle);
    struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)sns_module_handle;
    SENSOR_LOGI("E:cmd:0x%x",cmd);
    module = (SENSOR_MATCH_T *)sensor_cxt->module_cxt;

    switch (cmd >> 8)
    {
        case CMD_SNS_OTP_START:
            if (module && (module->otp_drv_info) && sensor_cxt->otp_drv_handle) {
                otp_ops = &module->otp_drv_info->otp_ops;
                if(otp_ops && otp_ops->sensor_otp_ioctl) {
                    ret = otp_ops->sensor_otp_ioctl(sensor_cxt->otp_drv_handle,cmd,param);
                    if (ret != CMR_CAMERA_SUCCESS)
                        return ret;
                }
            } else {
                ret = SENSOR_FAIL;
                SENSOR_LOGE("otp driver,return directly");
            }
            break;
        case CMD_SNS_IC_START:

            break;
        case CMD_SNS_AF_START:
            if (module && module->af_dev_info.af_drv_entry && sensor_cxt->af_drv_handle) {
                af_ops = &module->af_dev_info.af_drv_entry->af_ops;
                if(af_ops && af_ops->ioctl) {
                    ret = af_ops->ioctl(sensor_cxt->af_drv_handle,cmd,param);
                    if(SENSOR_SUCCESS != ret)
                        return SENSOR_FAIL;
                }
            } else {
                ret = SENSOR_FAIL;
                SENSOR_LOGE("af driver,return directly");
            }
            break;
        default :
            break;
    };
    SENSOR_LOGI("X:");
    return ret;
}

