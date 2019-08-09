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

#define LOG_TAG "sensor_imx363"
#include "sensor_imx363_mipi_raw.h"

#define VIDEO_INFO s_imx363_video_info
#define STATIC_INFO s_imx363_static_info
#define FPS_INFO s_imx363_mode_fps_info
#define RES_TAB_RAW s_imx363_resolution_tab_raw
#define RES_TRIM_TAB s_imx363_resolution_trim_tab
#define MIPI_RAW_INFO g_imx363_mipi_raw_info
#define MODULE_INFO s_imx363_module_info_tab

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static cmr_int imx363_drv_set_video_mode(cmr_handle handle, cmr_u32 param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_REG_T_PTR sensor_reg_ptr;
    cmr_u16 i = 0x00;
    cmr_u32 mode = 0;

    if (param >= SENSOR_VIDEO_MODE_MAX)
        return 0;

    if (sns_drv_cxt->ops_cb.get_mode) {
        ret = sns_drv_cxt->ops_cb.get_mode(handle, &mode);
        if (SENSOR_SUCCESS != ret) {
            SENSOR_LOGI("fail.");
            return SENSOR_FAIL;
        }
    }
    if (PNULL == VIDEO_INFO[mode].setting_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    sensor_reg_ptr = (SENSOR_REG_T_PTR)&VIDEO_INFO[mode].setting_ptr[param];
    if (PNULL == sensor_reg_ptr) {
        SENSOR_LOGI("fail.");
        return SENSOR_FAIL;
    }

    for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr) ||
                   (0xff != sensor_reg_ptr[i].reg_value);
         i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, sensor_reg_ptr[i].reg_addr,
                            sensor_reg_ptr[i].reg_value);
    }

    return 0;
}
#if 0
/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static cmr_u32 imx363_drv_get_default_frame_length(cmr_handle handle,
                                                cmr_u32 mode) {
    return RES_TRIM_TAB[mode].frame_line;
}
#endif

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx363_drv_group_hold_on(cmr_handle handle) { UNUSED(handle); }

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx363_drv_group_hold_off(cmr_handle handle) { UNUSED(handle); }

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 imx363_drv_read_gain(cmr_handle handle) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u16 gain_a = 0;
    cmr_u16 gain_d = 0;

    gain_a = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0205);
    gain_d = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0210);

    return gain_a * gain_d;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx363_drv_write_gain(cmr_handle handle, float gain) {

    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 sensor_again = 0;
    cmr_u32 sensor_dgain = 0;
    float temp_gain;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0104, 0x01);

    temp_gain = gain;
    if (temp_gain < 1.0)
        temp_gain = 1.0;
    else if (temp_gain > 8.0)
        temp_gain = 8.0;
    sensor_again = (cmr_u16)(512.0 - 512.0 / temp_gain);
    // sensor_again = (cmr_u16)(1024.0 - 1024.0 / temp_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0204,
                        (sensor_again >> 8) & 0xFF);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0205, sensor_again & 0xFF);

    temp_gain = gain / 8;
    if (temp_gain > 16.0)
        temp_gain = 16.0;
    else if (temp_gain < 1.0)
        temp_gain = 1.0;
    sensor_dgain = (cmr_u16)(256 * temp_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x020e,
                        (sensor_dgain >> 8) & 0xFF);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x020f, sensor_dgain & 0xFF);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0104, 0x00);

    SENSOR_LOGD("realgain=%f,again=%d,dgain=%f", gain, sensor_again, temp_gain);

    // imx363_group_hold_off(handle);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 imx363_drv_read_frame_length(cmr_handle handle) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u16 frame_len_h = 0;
    cmr_u16 frame_len_l = 0;

    frame_len_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340) & 0xff;
    frame_len_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0341) & 0xff;

    return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx363_drv_write_frame_length(cmr_handle handle,
                                          cmr_u32 frame_len) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340,
                        (frame_len >> 8) & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u32 imx363_drv_read_shutter(cmr_handle handle) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u16 shutter_h = 0;
    cmr_u16 shutter_l = 0;

    shutter_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0202) & 0xff;
    shutter_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0203) & 0xff;

    return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void imx363_drv_write_shutter(cmr_handle handle, cmr_u32 shutter) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0202, (shutter >> 8) & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0203, shutter & 0xff);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static cmr_u16 imx363_drv_update_exposure(cmr_handle handle, cmr_u32 shutter,
                                          cmr_u32 dummy_line) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;
    cmr_u32 offset = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    fr_len = sns_drv_cxt->frame_length_def;

    if (dummy_line > FRAME_OFFSET)
        offset = dummy_line;
    else
        offset = FRAME_OFFSET;
    dest_fr_len = ((shutter + offset) > fr_len) ? (shutter + offset) : fr_len;

    cur_fr_len = imx363_drv_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        imx363_drv_write_frame_length(handle, dest_fr_len);

    sns_drv_cxt->sensor_ev_info.preview_framelength = dest_fr_len;
write_sensor_shutter:
    /* write shutter to sensor registers */
    imx363_drv_write_shutter(handle, shutter);
    return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int imx363_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    SENSOR_IC_CHECK_PTR(module_info);

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED,
                              SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
        usleep(1 * 1000);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, dvdd_val, avdd_val,
                              iovdd_val);
        usleep(1 * 1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(1 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1 * 1000);
    } else {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        usleep(10 * 1000);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED,
                              SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("(1:on, 0:off): %ld", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int imx363_drv_init_fps_info(cmr_handle handle) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

    if (!fps_info->is_init) {
        cmr_u32 i, modn, tempfps = 0;
        SENSOR_LOGI("imx363_init_mode_fps_info:start init");
        for (i = 0; i < SENSOR_MODE_MAX; i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = trim_info[i].line_time * trim_info[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                fps_info->sensor_mode_fps[i].max_fps = modn * 30;
                if (fps_info->sensor_mode_fps[i].max_fps > 30) {
                    fps_info->sensor_mode_fps[i].is_high_fps = 1;
                    fps_info->sensor_mode_fps[i].high_fps_skip_num =
                        fps_info->sensor_mode_fps[i].max_fps / 30;
                }
                if (fps_info->sensor_mode_fps[i].max_fps >
                    static_info->max_fps) {
                    static_info->max_fps = fps_info->sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_LOGI("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                        tempfps, trim_info[i].frame_line,
                        trim_info[i].line_time);
            SENSOR_LOGI("mode %d,max_fps: %d ", i,
                        fps_info->sensor_mode_fps[i].max_fps);
            SENSOR_LOGI("is_high_fps: %d,highfps_skip_num %d",
                        fps_info->sensor_mode_fps[i].is_high_fps,
                        fps_info->sensor_mode_fps[i].high_fps_skip_num);
        }
        fps_info->is_init = 1;
    }
    SENSOR_LOGV("X:");
    return ret;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int imx363_drv_identify(cmr_handle handle, cmr_uint param) {
    cmr_u8 pid_value = 0x00;
    cmr_u8 ver_value = 0x00;
    cmr_u8 test1 = 0x00;
    cmr_u8 test2 = 0x00;
    cmr_int ret_value = SENSOR_FAIL;
    cmr_u8 i = 0x00;
    UNUSED(param);

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("imx363 mipi raw identify");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, imx363_PID_ADDR);
    if (imx363_PID_VALUE == pid_value || 0x2 == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, imx363_VER_ADDR);
        SENSOR_LOGI("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (imx363_VER_VALUE == ver_value) {
            ret_value = SENSOR_SUCCESS;
            SENSOR_LOGI("this is imx363 sensor");
            imx363_drv_init_fps_info(handle);
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
static cmr_uint imx363_drv_get_trim_tab(cmr_handle handle, cmr_uint param) {
    UNUSED(param);
    UNUSED(handle);
    return (cmr_uint)RES_TRIM_TAB;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int imx363_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u32 cap_shutter = 0;
    cmr_u32 prv_shutter = 0;
    float gain = 0;
    float cap_gain = 0;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    sns_drv_cxt->frame_length_def =
        sns_drv_cxt->trim_tab_info[capture_mode].frame_line;
    SENSOR_LOGI("capture_mode = %d,preview_mode=%d\n", capture_mode,
                preview_mode);

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);

    if (sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    if (preview_mode == capture_mode) {
        cap_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
        cap_gain = sns_drv_cxt->sensor_ev_info.preview_gain;
        goto snapshot_info;
    }

    prv_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
    gain = sns_drv_cxt->sensor_ev_info.preview_gain;

    cap_shutter = prv_shutter * prv_linetime / cap_linetime;

    cap_shutter = imx363_drv_update_exposure(handle, cap_shutter, 0);
    cap_gain = gain;
    imx363_drv_write_gain(handle, cap_gain);
    SENSOR_LOGI("preview_shutter = %d, preview_gain = %f",
                sns_drv_cxt->sensor_ev_info.preview_shutter,
                sns_drv_cxt->sensor_ev_info.preview_gain);

    SENSOR_LOGI("capture_shutter = %d, capture_gain = %f", cap_shutter,
                cap_gain);
snapshot_info:
    sns_drv_cxt->hdr_info.capture_shutter = cap_shutter;
    sns_drv_cxt->hdr_info.capture_gain = cap_gain;
    /* limit HDR capture min fps to 10;
     * MaxFrameTime = 1000000*0.1us;
     */
    sns_drv_cxt->hdr_info.capture_max_shutter = 1000000 / cap_linetime;

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          cap_shutter);
    } else {
        sns_drv_cxt->exif_info.exposure_line = cap_shutter;
    }

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int imx363_drv_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0xfff;
    mode = (param >> 0x1c) & 0x0f;

    SENSOR_LOGI("current mode = %d, exposure_line = %d, dummy_line=%d", mode,
                exposure_line, dummy_line);

    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;

    sns_drv_cxt->sensor_ev_info.preview_shutter =
        imx363_drv_update_exposure(handle, exposure_line, dummy_line);
    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          exposure_line);
    } else {
        sns_drv_cxt->exif_info.exposure_line = exposure_line;
    }

    return ret_value;
}

static cmr_int imx363_drv_ex_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    mode = ex->size_index;

    SENSOR_LOGD("current mode = %d, exposure_line = %d, dummy_line=%d", mode,
                exposure_line, dummy_line);

    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;
    sns_drv_cxt->line_time_def = sns_drv_cxt->trim_tab_info[mode].line_time;
    sns_drv_cxt->sensor_ev_info.preview_shutter =
        imx363_drv_update_exposure(handle, exposure_line, dummy_line);
    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          exposure_line);
    } else {
        sns_drv_cxt->exif_info.exposure_line = exposure_line;
    }

    return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static cmr_u32 isp_to_real_gain(cmr_handle handle, cmr_u32 param) {
    cmr_u32 real_gain = 0;
    real_gain = param;

    return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int imx363_drv_write_gain_value(cmr_handle handle, cmr_uint param) {
    float real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = (float)param / ISP_BASE_GAIN * 1.0;

    SENSOR_LOGD("real_gain = %f", real_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = real_gain;
    imx363_drv_write_gain(handle, real_gain);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void imx363_drv_increase_hdr_exposure(cmr_handle handle,
                                             cmr_u8 ev_multiplier) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 shutter_multiply = sns_drv_cxt->hdr_info.capture_max_shutter /
                               sns_drv_cxt->hdr_info.capture_shutter;
    cmr_u32 gain = 0;
    struct hdr_info_t *hdr_info = &sns_drv_cxt->hdr_info;

    if (0 == shutter_multiply)
        shutter_multiply = 1;

    if (shutter_multiply >= ev_multiplier) {
        imx363_drv_update_exposure(
            handle, hdr_info->capture_shutter * ev_multiplier, 0);
        imx363_drv_write_gain(handle, hdr_info->capture_gain);
    } else {
        gain = hdr_info->capture_gain * ev_multiplier / shutter_multiply;
        imx363_drv_update_exposure(
            handle, hdr_info->capture_shutter * shutter_multiply, 0);
        imx363_drv_write_gain(handle, gain);
    }
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void imx363_drv_decrease_hdr_exposure(cmr_handle handle,
                                             cmr_u8 ev_divisor) {
    cmr_u16 gain_multiply = 0;
    cmr_u32 shutter = 0;
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct hdr_info_t *hdr_info = &sns_drv_cxt->hdr_info;

    gain_multiply = hdr_info->capture_gain / SENSOR_BASE_GAIN;

    if (gain_multiply >= ev_divisor) {
        imx363_drv_update_exposure(handle, hdr_info->capture_shutter, 0);
        imx363_drv_write_gain(handle, hdr_info->capture_gain / ev_divisor);

    } else {
        shutter = hdr_info->capture_shutter * gain_multiply / ev_divisor;
        imx363_drv_update_exposure(handle, shutter, 0);
        imx363_drv_write_gain(handle, hdr_info->capture_gain / gain_multiply);
    }
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int imx363_drv_set_hdr_ev(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);

    cmr_u32 ev = ext_ptr->param;
    cmr_u8 ev_divisor, ev_multiplier;

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        ev_divisor = 2;
        imx363_drv_decrease_hdr_exposure(handle, ev_divisor);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        ev_multiplier = 2;
        imx363_drv_increase_hdr_exposure(handle, ev_multiplier);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        ev_multiplier = 1;
        imx363_drv_increase_hdr_exposure(handle, ev_multiplier);
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
static cmr_int imx363_drv_ext_func(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);

    SENSOR_LOGI("ext_ptr->cmd: %d", ext_ptr->cmd);
    switch (ext_ptr->cmd) {
    case SENSOR_EXT_EV:
        rtn = imx363_drv_set_hdr_ev(handle, param);
        break;
    default:
        break;
    }

    return rtn;
}

cmr_int imx363_drv_write_spcdata(cmr_handle handle, cmr_u8 *param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_LOGI("E");
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u8 *param_ptr = (cmr_u8 *)param;
    // cmr_u8 param_ptr[8192] = {0x00,0x00};
    // hw_sensor_read_i2c(sns_drv_cxt->hw_handle, 0xA0 >> 1, param_ptr,8192 <<
    // 16 |SENSOR_I2C_REG_16BIT);
    // SENSOR_LOGI("otp_raw_data:%p",param_ptr);
    for (int i = 0; i < 70; i++) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x7500 + i,
                            param_ptr[0x08ba + i] & 0xFF);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x7548 + i,
                            param_ptr[0x0900 + i] & 0xFF);
        // SENSOR_LOGV("E1  0x%02x 0x%02x",
        // param_ptr[0x08ba+i],hw_sensor_grc_read_i2c(sns_drv_cxt->hw_handle,
        //             0x34 >> 1, 0x7500 + i, BITS_ADDR16_REG8) );
    }
    return 0;
}

static cmr_int imx363_drv_set_pdaf_mode(cmr_handle handle, cmr_uint param) {
    UNUSED(param);
    SENSOR_LOGI("E 0x%x", param);

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    if (param == 0x02) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a3, 0x02);
    } else if (param == 0x31) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a3,
                            0x01); // 8x6 582x582
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a8, 0x02); // 0x02
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a9, 0x44); // 0x30
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038aa, 0x02); // 0x0d
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038ab, 0x40); // 0xa8
    } else if (param == 0x30 || param == 0x21) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a3,
                            0x00); // 16x12 582x582
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a8, 0x01); // 0x02
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a9, 0x22); // 0x30
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038aa, 0x01); // 0x0d
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038ab, 0x20); // 0xa8
    } else if (param == 0x20) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a3,
                            0x01); // 8x6 582x582
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a8, 0x00); // 0x02
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038a9, 0x91); // 0x30
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038aa, 0x00); // 0x0d
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x038ab, 0x90); // 0xa8
    }

#if 0
        SENSOR_LOGI("E %x",hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x38a3));
        SENSOR_LOGI("E %x",hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3e37));

        for(int i = 0x38a3; i < 0x38f3; i++){
            SENSOR_LOGI("E1  0x%04x 0x%02x",  i, hw_sensor_read_reg(sns_drv_cxt->hw_handle, i));
         }
#endif
    return 0;
}

static cmr_int imx363_drv_set_master_FrameSync(cmr_handle handle,
                                               cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x4B68, 0xFF);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5873, 0x0C);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x30A1, 0x01);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x4B70, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x4BD0, 0x00);
    //    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0350, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5D0C, 0x01);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int imx363_drv_stream_on(cmr_handle handle, cmr_uint param) {
    UNUSED(param);

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_LOGI("E");
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    char value1[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.colorbar", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_LOGI("enable test mode");
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x780F, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0600, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0601, 0x02);
    }

    imx363_drv_set_master_FrameSync(handle, param);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0101, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int imx363_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    UNUSED(param);
    unsigned char value = 0;
    cmr_u16 sleep_time = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);
    if (value != 0x00) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
        if (!sns_drv_cxt->is_sensor_close) {
            sleep_time = (sns_drv_cxt->sensor_ev_info.preview_framelength *
                          sns_drv_cxt->line_time_def / 1000000) +
                         10;
            usleep(sleep_time * 1000);
            SENSOR_LOGI("stream_off delay_ms %d", sleep_time);
        }
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
    }
    sns_drv_cxt->is_sensor_close = 0;

    SENSOR_LOGI("X");
    return SENSOR_SUCCESS;
}

static cmr_int imx363_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = PNULL;
    cmr_u32 up = 0, down = 0, i = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;

    if (!static_info || !module_info) {
        SENSOR_LOGI("error:static_info:%p,module_info:%p", static_info,
                    module_info);
        rtn = SENSOR_FAIL;
        goto exit;
    }

    // make sure we have get max fps of all settings.
    if (!fps_info->is_init) {
        imx363_drv_init_fps_info(handle);
    }
    ex_info = (struct sensor_ex_info *)param;
    ex_info->f_num = static_info->f_num;
    ex_info->focal_length = static_info->focal_length;
    ex_info->max_fps = static_info->max_fps;
    ex_info->max_adgain = static_info->max_adgain;
    ex_info->ois_supported = static_info->ois_supported;
    ex_info->pdaf_supported = static_info->pdaf_supported;
    ex_info->exp_valid_frame_num = static_info->exp_valid_frame_num;
    ex_info->clamp_level = static_info->clamp_level;
    ex_info->adgain_valid_frame_num = static_info->adgain_valid_frame_num;
    ex_info->preview_skip_num = module_info->preview_skip_num;
    ex_info->capture_skip_num = module_info->capture_skip_num;
    ex_info->name = (cmr_s8 *)MIPI_RAW_INFO.name;
    ex_info->sensor_version_info = (cmr_s8 *)MIPI_RAW_INFO.sensor_version_info;
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;

    memcpy(&ex_info->fov_info, &static_info->fov_info,
           sizeof(static_info->fov_info));

exit:
    sensor_ic_print_static_info((cmr_s8 *)SENSOR_NAME, ex_info);
    return rtn;
}

static cmr_int imx363_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;
    SENSOR_IC_CHECK_PTR(fps_data);

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        imx363_drv_init_fps_info(handle);
    }

    fps_info = (SENSOR_MODE_FPS_T *)param;
    cmr_u32 sensor_mode = fps_info->mode;
    fps_info->max_fps = fps_data->sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps = fps_data->sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps = fps_data->sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        fps_data->sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_LOGI("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_LOGI("min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return rtn;
}

static const cmr_u16 imx363_pd_is_right[] = {0, 0, 1, 1, 1, 1, 0, 0};

static const cmr_u16 imx363_pd_row[] = {5, 5, 8, 8, 21, 21, 24, 24};

static const cmr_u16 imx363_pd_col[] = {2, 18, 1, 17, 10, 26, 9, 25};
static const struct pd_pos_info _imx363_pd_pos_l[] = {
    {2, 5}, {18, 5}, {9, 24}, {25, 24},
};

static const struct pd_pos_info _imx363_pd_pos_r[] = {
    {1, 8}, {17, 8}, {10, 21}, {26, 21},
};
static const cmr_u32 pd_sns_mode[] = {0, 1, 1, 1};

static cmr_int imx363_drv_get_pdaf_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_pdaf_info *pdaf_info = NULL;
    cmr_u16 i = 0;
    cmr_u16 pd_pos_row_size = 0;
    cmr_u16 pd_pos_col_size = 0;
    cmr_u16 pd_pos_is_right_size = 0;

    /*TODO*/
    if (param == NULL) {
        SENSOR_LOGE("null input");
        return -1;
    }
    pdaf_info = (struct sensor_pdaf_info *)param;
    pd_pos_is_right_size = NUMBER_OF_ARRAY(imx363_pd_is_right);
    pd_pos_row_size = NUMBER_OF_ARRAY(imx363_pd_row);
    pd_pos_col_size = NUMBER_OF_ARRAY(imx363_pd_col);
    if ((pd_pos_row_size != pd_pos_col_size) ||
        (pd_pos_row_size != pd_pos_is_right_size) ||
        (pd_pos_is_right_size != pd_pos_col_size)) {
        SENSOR_LOGE("pd_pos_row size,pd_pos_row size and pd_pos_is_right size "
                    "are not match");
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
    pdaf_info->pd_is_right = (cmr_u16 *)imx363_pd_is_right;
    pdaf_info->pd_pos_row = (cmr_u16 *)imx363_pd_row;
    pdaf_info->pd_pos_col = (cmr_u16 *)imx363_pd_col;

    cmr_u16 pd_pos_r_size = NUMBER_OF_ARRAY(_imx363_pd_pos_r);
    cmr_u16 pd_pos_l_size = NUMBER_OF_ARRAY(_imx363_pd_pos_l);

    if (pd_pos_r_size != pd_pos_l_size) {
        SENSOR_LOGE("pd_pos_r size not match pd_pos_l");
        return -1;
    }
    pdaf_info->pd_pitch_x = 96;
    pdaf_info->pd_pitch_y = 130;
    pdaf_info->pd_density_x = 16;
    pdaf_info->pd_density_y = 16;
    pdaf_info->pd_block_num_x = 130;
    pdaf_info->pd_block_num_y = 96;
    pdaf_info->pd_pos_size = pd_pos_r_size;
    pdaf_info->pd_pos_r = (struct pd_pos_info *)_imx363_pd_pos_r;
    pdaf_info->pd_pos_l = (struct pd_pos_info *)_imx363_pd_pos_l;
    pdaf_info->vendor_type = SENSOR_VENDOR_IMX362_DUAL_PD; // 351;
    pdaf_info->type2_info.data_type = 0x2b;
    pdaf_info->type2_info.data_format = DATA_BYTE2;
    if (DATA_BYTE2 == pdaf_info->type2_info.data_format) {
        pdaf_info->type2_info.width = 3024;      // 260 + 60;
        pdaf_info->type2_info.height = 4032 / 4; // 96;
        pdaf_info->type2_info.pd_size =
            pdaf_info->type2_info.width * pdaf_info->type2_info.height * 2;
    } else if (DATA_RAW10 == pdaf_info->type2_info.data_format) {
        pdaf_info->type2_info.width = 3024;      // 260 + 4;
        pdaf_info->type2_info.height = 4032 / 4; // 96;
        pdaf_info->type2_info.pd_size =
            pdaf_info->type2_info.width * pdaf_info->type2_info.height * 10 / 8;
    }
    pdaf_info->vch2_info.bypass = 0;
    pdaf_info->vch2_info.vch2_vc = 0;
    pdaf_info->vch2_info.vch2_data_type = 0x30;
    pdaf_info->vch2_info.vch2_mode = 0x01;
    pdaf_info->sns_mode = pd_sns_mode;

    return rtn;
}

static cmr_int imx363_drv_set_spc_data(cmr_handle handle, cmr_u8 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u8 *spc_data = (cmr_u8 *)param;
    SENSOR_LOGI("*param 0x%x 0x%x", *param, *spc_data);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    imx363_drv_write_spcdata(handle, spc_data);

    return rtn;
}

static cmr_int imx363_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("ptr:%p,type:0x%x", param_ptr, param_ptr->type);

    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = imx363_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = imx363_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        sns_drv_cxt->is_sensor_close = 1;
        break;
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        rtn = imx363_drv_get_pdaf_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_OTP_DATA:
        //   rtn = imx363_drv_set_spc_data(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGV("X");

    return rtn;
}

static cmr_u16 imx363_drv_calc_exposure(cmr_handle handle, cmr_u32 shutter,
                                        cmr_u32 dummy_line,
                                        struct sensor_aec_i2c_tag *aec_info) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;
    cmr_int offset = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(aec_info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    fr_len = sns_drv_cxt->frame_length_def;

    if (dummy_line > FRAME_OFFSET)
        offset = dummy_line;
    else
        offset = FRAME_OFFSET;
    dest_fr_len = ((shutter + offset) > fr_len) ? (shutter + offset) : fr_len;

    cur_fr_len = imx363_drv_read_frame_length(handle);
    sns_drv_cxt->frame_length = dest_fr_len;

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    cmr_u16 frame_interval = 0x00;
    frame_interval = (cmr_u16)(
        ((shutter + dummy_line) * sns_drv_cxt->line_time_def) / 1000000);
    SENSOR_LOGD("exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
                shutter, dummy_line, frame_interval);

    aec_info->frame_length->settings[0].reg_value = (dest_fr_len >> 8) & 0xff;
    aec_info->frame_length->settings[1].reg_value = dest_fr_len & 0xff;
    sns_drv_cxt->sensor_ev_info.preview_framelength = dest_fr_len;

    aec_info->shutter->settings[0].reg_value = (shutter >> 8) & 0xff;
    aec_info->shutter->settings[1].reg_value = shutter & 0xff;
    return shutter;
}

static void imx363_drv_calc_gain(float gain,
                                 struct sensor_aec_i2c_tag *aec_info) {
    uint8_t i = 0;
    cmr_u32 sensor_again = 0;
    cmr_u32 sensor_dgain = 0;
    float temp_gain;
    SENSOR_IC_CHECK_PTR_VOID(aec_info);

    gain = gain / 32.0;

    temp_gain = gain;
    if (temp_gain < 1.0)
        temp_gain = 1.0;
    else if (temp_gain > 8.0)
        temp_gain = 8.0;
    sensor_again = (cmr_u16)(512.0 - 512.0 / temp_gain);
    if (aec_info->again->size) {
        aec_info->again->settings[1].reg_value = (sensor_again >> 8) & 0xFF;
        aec_info->again->settings[2].reg_value = sensor_again & 0xFF;
    }

    temp_gain = gain / 8;
    if (temp_gain > 16.0)
        temp_gain = 16.0;
    else if (temp_gain < 1.0)
        temp_gain = 1.0;
    sensor_dgain = (cmr_u16)(256 * temp_gain);
    if (aec_info->dgain->size) {
        aec_info->dgain->settings[0].reg_value = (sensor_dgain >> 8) & 0xFF;
        aec_info->dgain->settings[1].reg_value = sensor_dgain & 0xFF;
    }
}

static unsigned long imx363_drv_read_aec_info(cmr_handle handle,
                                              cmr_uint param) {
    unsigned long ret_value = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    float real_gain = 0;
    cmr_u32 gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    info->aec_i2c_info_out = &imx363_aec_info;

    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;
    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;

    sns_drv_cxt->line_time_def = sns_drv_cxt->trim_tab_info[mode].line_time;
    sns_drv_cxt->sensor_ev_info.preview_shutter = imx363_drv_calc_exposure(
        handle, exposure_line, dummy_line, &imx363_aec_info);

    gain = info->gain < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : info->gain;
    real_gain = (float)info->gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;
    imx363_drv_calc_gain(real_gain, &imx363_aec_info);

    return ret_value;
}

static cmr_int
imx363_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                         cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    if (ret != SENSOR_SUCCESS)
        return SENSOR_FAIL;

    sns_drv_cxt = (struct sensor_ic_drv_cxt *)*sns_ic_drv_handle;

    sns_drv_cxt->sensor_ev_info.preview_framelength = PREVIEW_FRAME_LENGTH;
    sns_drv_cxt->line_time_def = PREVIEW_LINE_TIME;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO),
                                    MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW),
                                        RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB),
                                  RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO),
                                    STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    /*add private here*/

    return ret;
}

static cmr_int imx363_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    /*if has private data,you must release it here*/

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int imx363_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                           void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

void *sensor_ic_open_lib(void)
{
     return &g_imx363_mipi_raw_info;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 *============================================================================*/
static struct sensor_ic_ops s_imx363_ops_tab = {
    .create_handle = imx363_drv_handle_create,
    .delete_handle = imx363_drv_handle_delete,
    /*get privage data*/
    .get_data = imx363_drv_get_private_data,
    /*common interface*/
    .power = imx363_drv_power_on,
    .identify = imx363_drv_identify,
    .write_gain_value = imx363_drv_write_gain_value,
    .read_aec_info = imx363_drv_read_aec_info,
    .ex_write_exp = imx363_drv_ex_write_exposure,
    .ext_ops = {
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = imx363_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = imx363_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = imx363_drv_stream_off,
            [SENSOR_IOCTL_ACCESS_VAL].ops = imx363_drv_access_val,
    }};
