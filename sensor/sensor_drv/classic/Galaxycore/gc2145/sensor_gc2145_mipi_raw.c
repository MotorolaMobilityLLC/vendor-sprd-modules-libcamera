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

#define LOG_TAG "gc2145_mipi"
#include "sensor_gc2145_mipi_raw.h"

#define FPS_INFO s_gc2145_mode_fps_info
#define STATIC_INFO s_gc2145_static_info
#define VIDEO_INFO s_gc2145_video_info
#define MODULE_INFO s_gc2145_module_info_tab
#define RES_TAB_RAW s_gc2145_resolution_tab_raw
#define RES_TRIM_TAB s_gc2145_resolution_trim_tab
#define MIPI_RAW_INFO g_gc2145_mipi_raw_info
static cmr_int gc2145_drv_set_gain16(cmr_handle handle, uint32_t gain);

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc2145_drv_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

    SENSOR_LOGI("E");
    if (!fps_info->is_init) {
        cmr_u32 i, modn, tempfps = 0;
        SENSOR_LOGI("start init");
        for (i = 0; i < SENSOR_MODE_MAX; i++) {
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
    SENSOR_LOGI("X");
    return rtn;
}

static cmr_int gc2145_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = (struct sensor_ex_info *)param;
    cmr_u32 up = 0;
    cmr_u32 down = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex_info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    if (!(fps_info && static_info && module_info)) {
        SENSOR_LOGE("error:null pointer checked.return");
        return SENSOR_FAIL;
    }

    // make sure we have get max fps of all settings.
    if (!fps_info->is_init) {
        gc2145_drv_init_fps_info(handle);
    }
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
    ex_info->name = MIPI_RAW_INFO.name;
    ex_info->sensor_version_info = MIPI_RAW_INFO.sensor_version_info;

    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info(SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int gc2145_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        gc2145_drv_init_fps_info(handle);
    }
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

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc2145_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_u32 rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    cmr_u16 tmp;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_HANDLE(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = gc2145_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = gc2145_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        rtn = sns_drv_cxt->is_sensor_close = 1;
        break;
    default:
        break;
    }

    SENSOR_LOGI("X");

    return rtn;
}
/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc2145_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    // gc2145_drv_set_init(handle);

    SENSOR_LOGI("StreamOn: E");
    //	gc2145_drv_set_gain16(handle,0xff);

    //    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
    //    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x17, 0x03);
    usleep(100000U);
#ifdef GC2145_MIPI_2Lane
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x95);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
#else
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x94);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
#endif
    usleep(20000U);

    return 0;
}
/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u32 gc2145_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("SENSOR_gc2145: StreamOff: ");

#ifdef GC2145_MIPI_2Lane
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x85);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
#else
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x10, 0x84);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
#endif
    usleep(20000U);

    return 0;
}

static cmr_int gc2145_drv_get_gain16(cmr_handle handle) {
    int gain16;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x03, 0x20);
    gain16 = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0xb0);

    return gain16;
}
#define ANALOG_GAIN_1 64  // 1.0x
#define ANALOG_GAIN_2 87  // 1.4x
#define ANALOG_GAIN_3 128 // 2.0x
#define ANALOG_GAIN_4 179 // 2.8x
#define ANALOG_GAIN_5 256 // 4.0x
// base_gain=0x40
#if 1
static cmr_int gc2145_drv_set_gain16(cmr_handle handle, uint32_t gain) {
    uint16_t temp = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("write_gain:0x%x", gain);

    if (gain < 0x40)
        gain = 0x40;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

    if ((ANALOG_GAIN_1 <= gain) && (gain < ANALOG_GAIN_2)) {
        // analog gain
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x25, 0x00);
        temp = 32 * gain / ANALOG_GAIN_1;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp);
    } else if ((ANALOG_GAIN_2 <= gain) && (gain < ANALOG_GAIN_3)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x25, 0x01);
        temp = 32 * gain / ANALOG_GAIN_2;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp);
    } else if ((ANALOG_GAIN_3 <= gain) && (gain < ANALOG_GAIN_4)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x25, 0x02);
        temp = 32 * gain / ANALOG_GAIN_3;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp);
    } else if ((ANALOG_GAIN_4 <= gain) && (gain < ANALOG_GAIN_5)) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x25, 0x03);
        temp = 32 * gain / ANALOG_GAIN_4;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp);
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x25, 0x04);
        temp = 32 * gain / ANALOG_GAIN_5;
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb1, temp);
    }

    return 0;
}
#else
static int gc2145_drv_set_gain16(cmr_handle handle, int gain16) {
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("write_gain:0x%x", gain16);

    temp = gain16 & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x03, 0x20);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xb0, temp);

    return 0;
}
#endif
static int gc2145_drv_get_shutter(cmr_handle handle) {
    // read shutter, in number of line period
    int shutter;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

    shutter = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x03);
    // shutter = (shutter << 16) + hw_sensor_read_reg(sns_drv_cxt->hw_handle,
    // 0x81);
    shutter = (shutter << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x04);
    SENSOR_LOGI("shutter:%d", shutter);

    return shutter;
}

static int gc2145_drv_set_shutter(cmr_handle handle, cmr_u16 shutter) {
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("shutter:%d", shutter);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

    // temp = (shutter >> 16)&0xff;
    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x83, temp);

    temp = (shutter >> 8) & 0x1f;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x03, temp);

    temp = shutter & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x04, temp);
    // gc2145_drv_get_shutter(handle);

    return 0;
}

static int gc2145_drv_get_vts(cmr_handle handle) {
    int VTS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
    VTS = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x07);
    VTS = (VTS << 8) + hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x07);
    SENSOR_LOGI("get_vts:%d", VTS);

    return VTS;
}

static int gc2145_drv_set_vts(cmr_handle handle, int VTS) {
    // write VTS to registers
    int temp;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("write_vts:%d", VTS);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

    temp = (VTS >> 8) & 0x1f;
    ;
    //  hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x07, temp);

    temp = VTS & 0xff;
    //  hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x08, temp);
    // gc2145_drv_get_vts(handle);

    return 0;
}

static int gc2145_drv_grouphold_on(cmr_handle handle) {
    SENSOR_LOGI("E");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    /*hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3400,0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3404,0x05);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe000,0x02);//es H  0xE002
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe001,0x02);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe003,0x02);//es L   0xE005
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe004,0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe006,0x02);//gain    0xE008
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe007,0x05);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe009,0x03);//vts H  0xE00B
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00A,0x40);*/

    //    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00B,
    //                        (gc2145_drv_get_vts(handle) >> 8) & 0xFF);

    /*hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00C,0x03);//vts L  0xE00E
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00D,0x41);*/

    //    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xe00E,
    //                        gc2145_drv_get_vts(handle) & 0xFF);
    return 0;
}
static int gc2145_drv_grouphold_off(cmr_handle handle) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    //    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x340F, 0x20); // fast
    //    write
    SENSOR_LOGI("vts:0x%x, shutter:0x%x, gain: 0x%x",
                gc2145_drv_get_vts(handle), gc2145_drv_get_shutter(handle),
                gc2145_drv_get_gain16(handle));

    return 0;
}

static void _calculate_hdr_exposure(cmr_handle handle, int capture_gain16,
                                    int capture_VTS, int capture_shutter) {
    // gc2145_drv_grouphold_on();

    // write capture gain
    gc2145_drv_set_gain16(handle, capture_gain16);

    // write capture shutter
    if (capture_shutter > (capture_VTS - gc2145_ES_OFFSET)) {
        capture_VTS = capture_shutter + gc2145_ES_OFFSET;
        gc2145_drv_set_vts(handle, capture_VTS);
    }
    gc2145_drv_set_shutter(handle, capture_shutter);

    // gc2145_drv_grouphold_off();
}

static cmr_int gc2145_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        usleep(1000U);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        usleep(1000U);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        usleep(1000U);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle,
                           SENSOR_DEFALUT_MCLK); // SENSOR_DEFALUT_MCLK);
        usleep(1000U);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(1000U);
        // step 3 reset pin high
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
    } else {
        // step 1 reset and PWDN
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        usleep(1000U);
        // step 4 xvclk
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(1000U);
        // step 5 AVDD IOVDD
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1000U);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1000U);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }
    SENSOR_LOGI("Power_On(1:on, 0:off): %d", power_on);
    return SENSOR_SUCCESS;
}

static cmr_int gc2145_drv_identify(cmr_handle handle, cmr_uint param) {
    cmr_u8 pid_value = 0x00;
    cmr_u8 ver_value = 0x00;
    cmr_u32 ret_value = SENSOR_FAIL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("E: gc2145 identify 0x%02x", pid_value);
    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, GC2145_PID_ADDR);
    if (GC2145_PID_VALUE == pid_value) {
        ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, GC2145_VER_ADDR);
        SENSOR_LOGI("PID = %x, VER = %x", pid_value, ver_value);
        if (GC2145_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is gc2145 sensor !");
            return SENSOR_SUCCESS;
        } else {
            SENSOR_LOGI("this is c%x%x sensor !", pid_value, ver_value);
        }
    } else {
        SENSOR_LOGE("identify fail,pid_value=%d", pid_value);
    }

    return ret_value;
}

static cmr_int gc2145_drv_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 expsure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    cmr_u16 frame_len = 0x00;
    cmr_u16 frame_len_cur = 0x00;
    cmr_u16 max_frame_len = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;
    expsure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("exposure line:%d, dummy:%d, size_index:%d", expsure_line,
                dummy_line, size_index);

    // group hold on
    // gc2145_drv_grouphold_on();

    max_frame_len = sns_drv_cxt->trim_tab_info[size_index].frame_line;

    if (0x00 != max_frame_len) {
        frame_len = ((expsure_line + dummy_line + 1) > max_frame_len)
                        ? (expsure_line + dummy_line + 1)
                        : max_frame_len;

        frame_len_cur = gc2145_drv_get_vts(handle) + max_frame_len;

        SENSOR_LOGI("frame_len_cur:%d, frame_len:%d,", frame_len_cur,
                    frame_len);

        if (frame_len_cur != frame_len) {

            gc2145_drv_set_vts(
                handle,
                frame_len - max_frame_len > 0 ? frame_len - max_frame_len : 0);
        }
    }

    ret_value = gc2145_drv_set_shutter(handle, expsure_line);

    return ret_value;
}

static cmr_int gc2145_drv_write_gain(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 isp_gain = 0x00;
    cmr_u32 sensor_gain = 0x00;
    SENSOR_LOGI("write_gain:0x%x", param);
    /*
        isp_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1) *
                   (((param >> 5) & 0x01) + 1);
        isp_gain = isp_gain * (((param >> 6) & 0x01) + 1) *
                   (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);

        // sensor_gain=(((isp_gain-16)/16)<<4)+isp_gain%16;
        sensor_gain = (((isp_gain - 16) / 16) << 4) + (isp_gain - 16) % 16;
    */
    sensor_gain = param * 0x40 / 0x80 * 1.0;

    ret_value = gc2145_drv_set_gain16(handle, sensor_gain);

    // group hold off
    // gc2145_drv_grouphold_off();
    return ret_value;
}

static cmr_int gc2145_drv_before_snapshot(cmr_handle handle, cmr_u32 param) {
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data = (PRIVATE_DATA *)sns_drv_cxt->privata_data.buffer;

    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;
    cmr_u32 prv_frame_line =
        sns_drv_cxt->trim_tab_info[preview_mode].frame_line;
    cmr_u32 cap_frame_line =
        sns_drv_cxt->trim_tab_info[capture_mode].frame_line;

    SENSOR_LOGI("mode: 0x%08x", param);

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if (sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("prv mode equal to capmode");
        goto CFG_INFO;
    }

    preview_exposure = gc2145_drv_get_shutter(handle);
    preview_maxline = gc2145_drv_get_vts(handle) + prv_frame_line;

    capture_maxline = gc2145_drv_get_vts(handle) + cap_frame_line;

    capture_exposure = preview_exposure * prv_linetime * 2 / cap_linetime;

    if (0 == capture_exposure) {
        capture_exposure = 1;
    }

    SENSOR_LOGI("preview_exposure:%d, capture_exposure:%d, capture_maxline: %d",
                preview_exposure, capture_exposure, capture_maxline);
    // gc2145_drv_grouphold_on();
    if (capture_exposure > (capture_maxline - gc2145_ES_OFFSET)) {
        capture_maxline = capture_exposure + gc2145_ES_OFFSET;
        capture_maxline = (capture_maxline + 1) >> 1 << 1;
        gc2145_drv_set_vts(handle, capture_maxline - cap_frame_line);
    }

    gc2145_drv_set_shutter(handle, capture_exposure);
// gc2145_drv_grouphold_off();
CFG_INFO:
    pri_data->cap_shutter = gc2145_drv_get_shutter(handle);
    pri_data->cap_vts = gc2145_drv_get_vts(handle) + cap_frame_line;
    pri_data->gain = gc2145_drv_get_gain16(handle);

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          pri_data->cap_shutter);
    } else {
        sns_drv_cxt->exif_info.exposure_time = pri_data->cap_shutter;
    }

    return SENSOR_SUCCESS;
}

static cmr_int gc2145_drv_after_snapshot(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("after_snapshot mode:%d", param);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, param);

    return SENSOR_SUCCESS;
}

static cmr_int gc2145_drv_set_ev(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ext_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data = (PRIVATE_DATA *)sns_drv_cxt->privata_data.buffer;

    cmr_int gain = pri_data->gain;
    cmr_int vts = pri_data->cap_vts;
    cmr_int cap_shutter = pri_data->cap_shutter;

    cmr_u32 ev = ext_ptr->param;

    SENSOR_LOGI("param: 0x%x", ext_ptr->param);

    switch (ev) {
    case SENSOR_HDR_EV_LEVE_0:
        _calculate_hdr_exposure(handle, gain, vts, cap_shutter / 2);
        break;
    case SENSOR_HDR_EV_LEVE_1:
        _calculate_hdr_exposure(handle, gain, vts, cap_shutter);
        break;
    case SENSOR_HDR_EV_LEVE_2:
        _calculate_hdr_exposure(handle, gain, vts, cap_shutter * 2);
        break;
    default:
        break;
    }
    return rtn;
}

static cmr_int gc2145_drv_save_load_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_EXT_FUN_PARAM_T_PTR sl_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(sl_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    PRIVATE_DATA *pri_data = sns_drv_cxt->privata_data.buffer;
    SENSOR_IC_CHECK_PTR(pri_data);

    cmr_u32 sl_param = sl_ptr->param;
    if (sl_param) {
        /*load exposure params to sensor*/
        SENSOR_LOGI("load shutter 0x%x gain 0x%x", pri_data->shutter,
                    pri_data->gain_bak);
        // gc2145_drv_grouphold_on();
        gc2145_drv_set_gain16(handle, pri_data->gain_bak);
        gc2145_drv_set_shutter(handle, pri_data->shutter);
        // gc2145_drv_grouphold_off();
    } else {
        /*ave exposure params from sensor*/
        pri_data->shutter = gc2145_drv_get_shutter(handle);
        pri_data->gain_bak = gc2145_drv_get_gain16(handle);
        SENSOR_LOGI("save shutter 0x%x gain 0x%x", pri_data->shutter,
                    pri_data->gain_bak);
    }
    return rtn;
}

static cmr_int
gc2145_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                         cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;
    cmr_u32 pri_data_size = sizeof(PRIVATE_DATA);
    void *pri_data = NULL;

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    sns_drv_cxt = *sns_ic_drv_handle;

    if (pri_data_size > 4) {
        pri_data = (PRIVATE_DATA *)malloc(pri_data_size);
        if (pri_data)
            sns_drv_cxt->privata_data.buffer = (cmr_u8 *)pri_data;
        else {
            sns_drv_cxt->privata_data.buffer = NULL;
            SENSOR_LOGE("size:%d", pri_data_size);
        }
    } else {
        sns_drv_cxt->privata_data.buffer = NULL;
    }

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO),
                                    MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW),
                                        RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB),
                                  RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO),
                                    STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    /*init exif info,this will be deleted in the future*/
    gc2145_drv_init_fps_info(sns_drv_cxt);

    /*add private here*/
    return ret;
}

static cmr_int gc2145_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (sns_drv_cxt->privata_data.buffer != NULL) {
        free(sns_drv_cxt->privata_data.buffer);
    }
    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int gc2145_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                           void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

void *sensor_ic_open_lib(void)
{
     return &g_gc2145_mipi_raw_info;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = gc2145_drv_power_on,
 *============================================================================*/
static struct sensor_ic_ops s_gc2145_ops_tab = {
    .create_handle = gc2145_drv_handle_create,
    .delete_handle = gc2145_drv_handle_delete,
    .get_data = gc2145_drv_get_private_data,

    .power = gc2145_drv_power_on,
    .identify = gc2145_drv_identify,
    .ex_write_exp = gc2145_drv_write_exposure,
    .write_gain_value = gc2145_drv_write_gain,

    .ext_ops =
        {
                [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = gc2145_drv_before_snapshot,
                [SENSOR_IOCTL_STREAM_ON].ops = gc2145_drv_stream_on,
                [SENSOR_IOCTL_STREAM_OFF].ops = gc2145_drv_stream_off,
                /* expand interface,if you want to add your sub cmd ,
                 *  you can add it in enum {@SENSOR_IOCTL_VAL_TYPE}
                 */
                [SENSOR_IOCTL_ACCESS_VAL].ops = gc2145_drv_access_val,
        }

};
