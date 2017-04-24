/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * V2.0
 */

#include "sensor_imx258_mipi_raw.h"

/*==============================================================================
 * Description:
 * Local variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

static uint32_t s_imx258_sensor_close_flag = 0;

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t imx258_set_video_mode(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_REG_T_PTR sensor_reg_ptr;
    uint16_t i = 0x00;
    uint32_t mode;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    if (PNULL == s_imx258_video_info[mode].setting_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    sensor_reg_ptr =
        (SENSOR_REG_T_PTR)&s_imx258_video_info[mode].setting_ptr[param];
    if (PNULL == sensor_reg_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr) ||
                   (0xff != sensor_reg_ptr[i].reg_value);
         i++) {
        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
                        sensor_reg_ptr[i].reg_value);
    }

    return 0;
}

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t imx258_get_default_frame_length(SENSOR_HW_HANDLE handle,
                                                uint32_t mode) {
    return s_imx258_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_group_hold_on(SENSOR_HW_HANDLE handle) {
    // Sensor_WriteReg(0x0104, 0x01);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_group_hold_off(SENSOR_HW_HANDLE handle) {
    // Sensor_WriteReg(0x0104, 0x00);
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx258_read_gain(SENSOR_HW_HANDLE handle) {
    uint16_t gain_a = 0;
    uint16_t gain_d = 0;

    gain_a = Sensor_ReadReg(0x0205);
    gain_d = Sensor_ReadReg(0x0210);

    return gain_a * gain_d;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_write_gain(SENSOR_HW_HANDLE handle, float gain) {
    uint32_t sensor_again = 0;
    uint32_t sensor_dgain = 0;
    float temp_gain;

    gain = gain / 32.0;

    temp_gain = gain;
    if (temp_gain < 1.0)
        temp_gain = 1.0;
    else if (temp_gain > 16.0)
        temp_gain = 16.0;
    sensor_again = (uint16_t)(512.0 - 512.0 / temp_gain);
    Sensor_WriteReg(0x0204, (sensor_again >> 8) & 0xFF);
    Sensor_WriteReg(0x0205, sensor_again & 0xFF);

    temp_gain = gain / 16;
    if (temp_gain > 16.0)
        temp_gain = 16.0;
    else if (temp_gain < 1.0)
        temp_gain = 1.0;
    sensor_dgain = (uint16_t)(256 * temp_gain);
    Sensor_WriteReg(0x020e, (sensor_dgain >> 8) & 0xFF);
    Sensor_WriteReg(0x020f, sensor_dgain & 0xFF);
    Sensor_WriteReg(0x0210, (sensor_dgain >> 8) & 0xFF);
    Sensor_WriteReg(0x0211, sensor_dgain & 0xFF);
    Sensor_WriteReg(0x0212, (sensor_dgain >> 8) & 0xFF);
    Sensor_WriteReg(0x0213, sensor_dgain & 0xFF);
    Sensor_WriteReg(0x0214, (sensor_dgain >> 8) & 0xFF);
    Sensor_WriteReg(0x0215, sensor_dgain & 0xFF);

    SENSOR_LOGI("realgain=%f,again=%d,dgain=%f", gain, sensor_again, temp_gain);

    // imx258_group_hold_off(handle);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx258_read_frame_length(SENSOR_HW_HANDLE handle) {
    uint16_t frame_len_h = 0;
    uint16_t frame_len_l = 0;

    frame_len_h = Sensor_ReadReg(0x0340) & 0xff;
    frame_len_l = Sensor_ReadReg(0x0341) & 0xff;

    return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_write_frame_length(SENSOR_HW_HANDLE handle,
                                      uint32_t frame_len) {
    Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
    Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_read_shutter(SENSOR_HW_HANDLE handle) {
    uint16_t shutter_h = 0;
    uint16_t shutter_l = 0;

    shutter_h = Sensor_ReadReg(0x0202) & 0xff;
    shutter_l = Sensor_ReadReg(0x0203) & 0xff;

    return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void imx258_write_shutter(SENSOR_HW_HANDLE handle, uint32_t shutter) {
    Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
    Sensor_WriteReg(0x0203, shutter & 0xff);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t imx258_update_exposure(SENSOR_HW_HANDLE handle,
                                       uint32_t shutter, uint32_t dummy_line) {
    uint32_t dest_fr_len = 0;
    uint32_t cur_fr_len = 0;
    uint32_t fr_len = s_current_default_frame_length;
    int32_t offset = 0;

    //	imx258_group_hold_on(handle);

    if (dummy_line > FRAME_OFFSET)
        offset = dummy_line;
    else
        offset = FRAME_OFFSET;
    dest_fr_len = ((shutter + offset) > fr_len) ? (shutter + offset) : fr_len;

    cur_fr_len = imx258_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        imx258_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    imx258_write_shutter(handle, shutter);
    return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_power_on(SENSOR_HW_HANDLE handle,
                                     uint32_t power_on) {
    SENSOR_AVDD_VAL_E dvdd_val = g_imx258_mipi_raw_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_imx258_mipi_raw_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_imx258_mipi_raw_info.iovdd_val;
    BOOLEAN power_down = g_imx258_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_imx258_mipi_raw_info.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        Sensor_SetResetLevel(reset_level);
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
                          SENSOR_AVDD_CLOSED);
        usleep(1 * 1000);
        Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
        usleep(1 * 1000);
        Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
        usleep(1 * 1000);
        Sensor_SetResetLevel(!reset_level);
        //Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
    } else {
        Sensor_SetResetLevel(reset_level);
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
                          SENSOR_AVDD_CLOSED);
        //Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("(1:on, 0:off): %d", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_init_mode_fps_info(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_LOGI("imx258_init_mode_fps_info:E");
    if (!s_imx258_mode_fps_info.is_init) {
        uint32_t i, modn, tempfps = 0;
        SENSOR_LOGI("imx258_init_mode_fps_info:start init");
        for (i = 0; i < NUMBER_OF_ARRAY(s_imx258_resolution_trim_tab); i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = s_imx258_resolution_trim_tab[i].line_time *
                      s_imx258_resolution_trim_tab[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps = modn * 30;
                if (s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
                    s_imx258_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
                    s_imx258_mode_fps_info.sensor_mode_fps[i]
                        .high_fps_skip_num =
                        s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps / 30;
                }
                if (s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps >
                    s_imx258_static_info.max_fps) {
                    s_imx258_static_info.max_fps =
                        s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_LOGI("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                        tempfps, s_imx258_resolution_trim_tab[i].frame_line,
                        s_imx258_resolution_trim_tab[i].line_time);
            SENSOR_LOGI("mode %d,max_fps: %d ", i,
                        s_imx258_mode_fps_info.sensor_mode_fps[i].max_fps);
            SENSOR_LOGI(
                "is_high_fps: %d,highfps_skip_num %d",
                s_imx258_mode_fps_info.sensor_mode_fps[i].is_high_fps,
                s_imx258_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
        }
        s_imx258_mode_fps_info.is_init = 1;
    }
    SENSOR_LOGI("imx258_init_mode_fps_info:X");
    return rtn;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_identify(SENSOR_HW_HANDLE handle,
                                     uint32_t param) {
    uint8_t pid_value = 0x00;
    uint8_t ver_value = 0x00;
    uint32_t ret_value = SENSOR_FAIL;
    uint8_t i = 0x00;
    UNUSED(param);

    SENSOR_LOGI("imx258 mipi raw identify");

    pid_value = Sensor_ReadReg(imx258_PID_ADDR);
    if (imx258_PID_VALUE == pid_value) {
        ver_value = Sensor_ReadReg(imx258_VER_ADDR);
        SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (imx258_VER_VALUE == ver_value) {
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("this is imx258 sensor");
#if defined(CONFIG_CAMERA_ISP_DIR_3)
            //vcm_LC898214_init(handle);
//#else
//			bu64297gwz_init(handle);
#endif
            imx258_init_mode_fps_info(handle);
        } else {
            SENSOR_LOGI("Identify this is %x%x sensor", pid_value, ver_value);
        }
    } else {
        SENSOR_LOGI("identify fail, pid_value = %x", pid_value);
    }

    return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long imx258_get_resolution_trim_tab(SENSOR_HW_HANDLE handle,
                                                    uint32_t param) {
    UNUSED(param);
    return (unsigned long)s_imx258_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx258_before_snapshot(SENSOR_HW_HANDLE handle,
                                            uint32_t param) {
    uint32_t cap_shutter = 0;
    uint32_t prv_shutter = 0;
    float gain = 0;
    float cap_gain = 0;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;

    uint32_t prv_linetime =
        s_imx258_resolution_trim_tab[preview_mode].line_time;
    uint32_t cap_linetime =
        s_imx258_resolution_trim_tab[capture_mode].line_time;

    s_current_default_frame_length =
        imx258_get_default_frame_length(handle, capture_mode);
    SENSOR_LOGI("capture_mode = %d,preview_mode=%d\n", capture_mode,
                preview_mode);

    if (preview_mode == capture_mode) {
        cap_shutter = s_sensor_ev_info.preview_shutter;
        cap_gain = s_sensor_ev_info.preview_gain;
        goto snapshot_info;
    }

    prv_shutter = s_sensor_ev_info.preview_shutter; // imx132_read_shutter();
    gain = s_sensor_ev_info.preview_gain;           // imx132_read_gain();

    Sensor_SetMode(capture_mode);
    Sensor_SetMode_WaitDone();

    cap_shutter =
        prv_shutter * prv_linetime / cap_linetime; // * BINNING_FACTOR;

    /*	while (gain >= (2 * SENSOR_BASE_GAIN)) {
                    if (cap_shutter * 2 > s_current_default_frame_length)
                            break;
                    cap_shutter = cap_shutter * 2;
                    gain = gain / 2;
            }
    */
    cap_shutter = imx258_update_exposure(handle, cap_shutter, 0);
    cap_gain = gain;
    imx258_write_gain(handle, cap_gain);
    SENSOR_LOGI("preview_shutter = %d, preview_gain = %f",
                s_sensor_ev_info.preview_shutter,
                s_sensor_ev_info.preview_gain);

    SENSOR_LOGI("capture_shutter = %d, capture_gain = %f", cap_shutter,
                cap_gain);
snapshot_info:
    s_hdr_info.capture_shutter = cap_shutter; // imx132_read_shutter();
    s_hdr_info.capture_gain = cap_gain;       // imx132_read_gain();
    /* limit HDR capture min fps to 10;
     * MaxFrameTime = 1000000*0.1us;
     */
    s_hdr_info.capture_max_shutter = 1000000 / cap_linetime;

    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static unsigned long imx258_write_exposure(SENSOR_HW_HANDLE handle,
                                           unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t mode = 0x00;

    exposure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
    mode = (param >> 0x1c) & 0x0f;

    SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line=%d", mode,
                exposure_line, dummy_line);
    s_current_default_frame_length =
        imx258_get_default_frame_length(handle, mode);

    s_sensor_ev_info.preview_shutter =
        imx258_update_exposure(handle, exposure_line, dummy_line);

    return ret_value;
}

static uint32_t imx258_ex_write_exposure(SENSOR_HW_HANDLE handle,
                                              unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t mode = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    if (!param) {
        SENSOR_LOGI("param is NULL !!!");
        return ret_value;
    }

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    mode = ex->size_index;

    SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line=%d", mode,
                exposure_line, dummy_line);
    s_current_default_frame_length =
        imx258_get_default_frame_length(handle, mode);

    s_sensor_ev_info.preview_shutter =
        imx258_update_exposure(handle, exposure_line, dummy_line);

    return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t real_gain = 0;
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
    real_gain = param;
#else
    real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1);
    real_gain =
        real_gain * (((param >> 5) & 0x01) + 1) * (((param >> 6) & 0x01) + 1);
    real_gain =
        real_gain * (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);
    real_gain =
        real_gain * (((param >> 9) & 0x01) + 1) * (((param >> 10) & 0x01) + 1);
    real_gain = real_gain * (((param >> 11) & 0x01) + 1);
#endif

    return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx258_write_gain_value(SENSOR_HW_HANDLE handle,
                                             uint32_t param) {
    unsigned long ret_value = SENSOR_SUCCESS;
    float real_gain = 0;

    real_gain = (float)param * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;

    SENSOR_LOGI("real_gain = %f", real_gain);

    s_sensor_ev_info.preview_gain = real_gain;
    imx258_write_gain(handle, real_gain);

    return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void imx258_increase_hdr_exposure(SENSOR_HW_HANDLE handle,
                                         uint8_t ev_multiplier) {
    uint32_t shutter_multiply =
        s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
    uint32_t gain = 0;

    if (0 == shutter_multiply)
        shutter_multiply = 1;

    if (shutter_multiply >= ev_multiplier) {
        imx258_update_exposure(handle,
                               s_hdr_info.capture_shutter * ev_multiplier, 0);
        imx258_write_gain(handle, s_hdr_info.capture_gain);
    } else {
        gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
        imx258_update_exposure(
            handle, s_hdr_info.capture_shutter * shutter_multiply, 0);
        imx258_write_gain(handle, gain);
    }
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void imx258_decrease_hdr_exposure(SENSOR_HW_HANDLE handle,
                                         uint8_t ev_divisor) {
    uint16_t gain_multiply = 0;
    uint32_t shutter = 0;
    gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

    if (gain_multiply >= ev_divisor) {
        imx258_update_exposure(handle, s_hdr_info.capture_shutter, 0);
        imx258_write_gain(handle, s_hdr_info.capture_gain / ev_divisor);

    } else {
        shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
        imx258_update_exposure(handle, shutter, 0);
        imx258_write_gain(handle, s_hdr_info.capture_gain / gain_multiply);
    }
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx258_set_hdr_ev(SENSOR_HW_HANDLE handle,
                                  unsigned long param) {
    uint32_t ret = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    uint32_t ev = ext_ptr->param;
    uint8_t ev_divisor, ev_multiplier;

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        ev_divisor = 2;
        imx258_decrease_hdr_exposure(handle, ev_divisor);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        ev_multiplier = 2;
        imx258_increase_hdr_exposure(handle, ev_multiplier);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        ev_multiplier = 1;
        imx258_increase_hdr_exposure(handle, ev_multiplier);
        break;
    default:
        break;
    }
    return ret;
}

/*==============================================================================
 * Description:
 * extra functoin
 * you can add functions reference SENSOR_EXT_FUNC_CMD_E which from
 *sensor_drv_u.h
 *============================================================================*/
static unsigned long imx258_ext_func(SENSOR_HW_HANDLE handle,
                                     unsigned long param) {
    unsigned long rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

    SENSOR_LOGI("ext_ptr->cmd: %d", ext_ptr->cmd);
    switch (ext_ptr->cmd) {
    case SENSOR_EXT_EV:
        rtn = imx258_set_hdr_ev(handle, param);
        break;
    default:
        break;
    }

    return rtn;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_stream_on(SENSOR_HW_HANDLE handle,
                                      uint32_t param) {
#if 1
    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_LOGI("SENSOR_imx230: enable test mode");
        Sensor_WriteReg(0x0600, 0x00);
        Sensor_WriteReg(0x0601, 0x02);
    }
#endif
    SENSOR_LOGI("E");
    UNUSED(param);
#if defined(CONFIG_CAMERA_ISP_DIR_3)
#ifndef CAMERA_SENSOR_BACK_I2C_SWITCH
    Sensor_WriteReg(0x0101, 0x03);
#endif
#endif
    Sensor_WriteReg(0x0100, 0x01);
    /*delay*/
    // usleep(10 * 1000);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx258_stream_off(SENSOR_HW_HANDLE handle,
                                       uint32_t param) {
    SENSOR_LOGI("E");
    UNUSED(param);
    unsigned char value;
    unsigned int sleep_time = 0;

    value = Sensor_ReadReg(0x0100);
    if (value != 0x00) {
        Sensor_WriteReg(0x0100, 0x00);
        if (!s_imx258_sensor_close_flag) {
            sleep_time = 100 * 1000;
            usleep(sleep_time);
        }
    } else {
        Sensor_WriteReg(0x0100, 0x00);
    }

    s_imx258_sensor_close_flag = 0;
    SENSOR_LOGI("X sleep_time=%dus", sleep_time);
    return 0;
}

static uint32_t imx258_write_af(SENSOR_HW_HANDLE handle,
                                     uint32_t param) {
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    //return vcm_LC898214_set_position(handle, param);
	return 0;
#else
    return 0; // bu64297gwz_write_af(handle, param);
#endif
}

static uint32_t imx258_get_static_info(SENSOR_HW_HANDLE handle,
                                       uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    uint32_t up = 0;
    uint32_t down = 0;
    // make sure we have get max fps of all settings.
    if (!s_imx258_mode_fps_info.is_init) {
        imx258_init_mode_fps_info(handle);
    }
    ex_info = (struct sensor_ex_info *)param;
    ex_info->f_num = s_imx258_static_info.f_num;
    ex_info->focal_length = s_imx258_static_info.focal_length;
    ex_info->max_fps = s_imx258_static_info.max_fps;
    ex_info->max_adgain = s_imx258_static_info.max_adgain;
    ex_info->ois_supported = s_imx258_static_info.ois_supported;
    ex_info->pdaf_supported = s_imx258_static_info.pdaf_supported;
    ex_info->exp_valid_frame_num = s_imx258_static_info.exp_valid_frame_num;
    ex_info->clamp_level = s_imx258_static_info.clamp_level;
    ex_info->adgain_valid_frame_num =
        s_imx258_static_info.adgain_valid_frame_num;
    ex_info->preview_skip_num = g_imx258_mipi_raw_info.preview_skip_num;
    ex_info->capture_skip_num = g_imx258_mipi_raw_info.capture_skip_num;
    ex_info->name = (cmr_s8 *)g_imx258_mipi_raw_info.name;
    ex_info->sensor_version_info = (cmr_s8 *)g_imx258_mipi_raw_info.sensor_version_info;
#if defined(CONFIG_CAMERA_ISP_DIR_3)
#if 1 // def CONFIG_AF_VCM_DW9800W
    //vcm_LC898214_get_pose_dis(handle, &up, &down);
//	vcm_dw9800_get_pose_dis(handle, &up, &down);
#else
    bu64297gwz_get_pose_dis(handle, &up, &down);
#endif
#endif
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    SENSOR_LOGI("SENSOR_imx258: f_num: %d", ex_info->f_num);
    SENSOR_LOGI("SENSOR_imx258: max_fps: %d", ex_info->max_fps);
    SENSOR_LOGI("SENSOR_imx258: max_adgain: %d", ex_info->max_adgain);
    SENSOR_LOGI("SENSOR_imx258: ois_supported: %d", ex_info->ois_supported);
    SENSOR_LOGI("SENSOR_imx258: pdaf_supported: %d", ex_info->pdaf_supported);
    SENSOR_LOGI("SENSOR_imx258: exp_valid_frame_num: %d",
                ex_info->exp_valid_frame_num);
    SENSOR_LOGI("SENSOR_imx258: clam_level: %d", ex_info->clamp_level);
    SENSOR_LOGI("SENSOR_imx258: adgain_valid_frame_num: %d",
                ex_info->adgain_valid_frame_num);
    SENSOR_LOGI("SENSOR_imx258: sensor name is: %s", ex_info->name);
    SENSOR_LOGI("SENSOR_imx258: sensor version info is: %s",
                ex_info->sensor_version_info);

    return rtn;
}

static uint32_t imx258_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    // make sure have inited fps of every sensor mode.
    if (!s_imx258_mode_fps_info.is_init) {
        imx258_init_mode_fps_info(handle);
    }
    fps_info = (SENSOR_MODE_FPS_T *)param;
    uint32_t sensor_mode = fps_info->mode;
    fps_info->max_fps =
        s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps =
        s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps =
        s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        s_imx258_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_LOGI("SENSOR_imx258: mode %d, max_fps: %d", fps_info->mode,
                fps_info->max_fps);
    SENSOR_LOGI("SENSOR_imx258: min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("SENSOR_imx258: is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("SENSOR_imx258: high_fps_skip_num: %d",
                fps_info->high_fps_skip_num);

    return rtn;
}

static uint32_t imx258_set_sensor_close_flag(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;

    s_imx258_sensor_close_flag = 1;

    return rtn;
}

static const cmr_u16 imx258_pd_is_right[] = {0,0,1,1,1,1,0,0};

static const cmr_u16 imx258_pd_row[] = {5,5,8,8,21,21,24,24};

static const cmr_u16 imx258_pd_col[] = {2,18,1,17,10,26,9,25};
static const struct pd_pos_info _imx258_pd_pos_l[] = {
    {2, 5}, {18, 5}, {9, 24}, {25, 24},
};

static const struct pd_pos_info _imx258_pd_pos_r[] = {
	{1, 8}, {17, 8}, {10, 21}, {26, 21},
};

static uint32_t imx258_get_pdaf_info(SENSOR_HW_HANDLE handle,
                                         uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct sensor_pdaf_info *pdaf_info = NULL;
    cmr_u16 i = 0;
    cmr_u16 pd_pos_row_size = 0;
    cmr_u16 pd_pos_col_size = 0;
    cmr_u16 pd_pos_is_right_size = 0;


    /*TODO*/
    if (param == NULL) {
        SENSOR_PRINT_ERR("null input");
        return -1;
    }
    pdaf_info = (struct sensor_pdaf_info *)param;
    pd_pos_is_right_size = NUMBER_OF_ARRAY(imx258_pd_is_right);
    pd_pos_row_size = NUMBER_OF_ARRAY(imx258_pd_row);
    pd_pos_col_size = NUMBER_OF_ARRAY(imx258_pd_col);
    if ((pd_pos_row_size != pd_pos_col_size) || (pd_pos_row_size != pd_pos_is_right_size) ||
	(pd_pos_is_right_size != pd_pos_col_size)){
        SENSOR_PRINT_ERR(
            "imx258 pd_pos_row size,pd_pos_row size and pd_pos_is_right size are not match");
        return -1;
    }

    pdaf_info->pd_offset_x = 24;
    pdaf_info->pd_offset_y = 24;
    pdaf_info->pd_end_x = 4184;
    pdaf_info->pd_end_y = 3096;
    pdaf_info->pd_block_w = 2;
    pdaf_info->pd_block_h = 2;
    pdaf_info->pd_block_num_x = 130;
    pdaf_info->pd_block_num_y = 96;
    pdaf_info->pd_pos_size = pd_pos_is_right_size;
    pdaf_info->pd_is_right = (cmr_u16 *)imx258_pd_is_right;
    pdaf_info->pd_pos_row = (cmr_u16 *)imx258_pd_row;
    pdaf_info->pd_pos_col = (cmr_u16 *)imx258_pd_col;

    cmr_u16 pd_pos_r_size = NUMBER_OF_ARRAY(_imx258_pd_pos_r);
    cmr_u16 pd_pos_l_size = NUMBER_OF_ARRAY(_imx258_pd_pos_l);

    if (pd_pos_r_size != pd_pos_l_size) {
        SENSOR_PRINT_ERR(
            "imx258_pd_pos_r size not match imx258_pd_pos_l");
        return -1;
    }
    pdaf_info->pd_pitch_x = 96;
    pdaf_info->pd_pitch_y = 130;
	pdaf_info->pd_density_x = 16;
    pdaf_info->pd_density_y = 32;
    pdaf_info->pd_block_num_x = 130;
    pdaf_info->pd_block_num_y = 96;
    pdaf_info->pd_pos_size = pd_pos_r_size;
    pdaf_info->pd_pos_r = (struct pd_pos_info *)_imx258_pd_pos_r;
    pdaf_info->pd_pos_l = (struct pd_pos_info *)_imx258_pd_pos_l;

    return rtn;
}

static unsigned long imx258_access_val(SENSOR_HW_HANDLE handle,
                                       unsigned long param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    uint16_t tmp;

    SENSOR_LOGI("SENSOR_imx258: _imx258_access_val E param_ptr = %p",
                param_ptr);
    if (!param_ptr) {
        return rtn;
    }

    SENSOR_LOGI("SENSOR_imx258: param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_INIT_OTP:
        // imx258_otp_init(handle);
        break;
    case SENSOR_VAL_TYPE_SHUTTER:
        //*((uint32_t*)param_ptr->pval) = imx258_get_shutter();
        break;
    case SENSOR_VAL_TYPE_READ_VCM:
        // rtn = imx258_read_vcm(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_WRITE_VCM:
        // rtn = imx258_write_vcm(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
        // imx258_otp_read(handle,param_ptr);
        break;
    case SENSOR_VAL_TYPE_PARSE_OTP:
        // imx258_parse_otp(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP:
        // rtn = _hi544_write_otp(handle, (uint32_t)param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_RELOADINFO: {
        //				struct isp_calibration_info **p= (struct
        // isp_calibration_info **)param_ptr->pval;
        //				*p=&calibration_info;
    } break;
    case SENSOR_VAL_TYPE_GET_AFPOSITION:
        //*(uint32_t*)param_ptr->pval = 0;//cur_af_pos;
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
        // rtn = imx258_write_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        // rtn = imx258_read_otp_gain(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = imx258_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = imx258_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        rtn = imx258_set_sensor_close_flag(handle);
        break;
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        rtn = imx258_get_pdaf_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("SENSOR_imx258: _imx258_access_val X");

    return rtn;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = imx132_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_imx258_ioctl_func_tab = {
    .power = imx258_power_on,
    .identify = imx258_identify,
    .get_trim = imx258_get_resolution_trim_tab,
    .before_snapshort = imx258_before_snapshot,
    .ex_write_exp = imx258_ex_write_exposure,
    .write_gain_value = imx258_write_gain_value,
    //.set_focus = imx258_ext_func,
    //.set_video_mode = imx132_set_video_mode,
    .stream_on = imx258_stream_on,
    .stream_off = imx258_stream_off,
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    //.af_enable = imx258_write_af,
#endif
    //.group_hold_on = imx132_group_hold_on,
    //.group_hold_of = imx132_group_hold_off,
    .cfg_otp = imx258_access_val,
    //	.set_motor_bestmode = dw9800_set_motor_bestmode,// set vcm best mode and
    // avoid damping
};

