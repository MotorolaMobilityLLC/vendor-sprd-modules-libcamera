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
#define LOG_TAG "s5k3p9sx04_mipi_raw"
#include "sensor_s5k3p9sx04_mipi_raw.h"

#define RES_TAB_RAW s_s5k3p9sx04_resolution_Tab_RAW
#define RES_TRIM_TAB s_s5k3p9sx04_Resolution_Trim_Tab
#define STATIC_INFO s_s5k3p9sx04_static_info
#define FPS_INFO s_s5k3p9sx04_mode_fps_info
#define MIPI_RAW_INFO g_s5k3p9sx04_mipi_raw_info
#define MODULE_INFO s_s5k3p9sx04_module_info_tab

static cmr_int s5k3p9sx04_drv_init_fps_info(cmr_handle handle) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");
    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

    if (!fps_info->is_init) {
        uint32_t i, modn, tempfps = 0;
        SENSOR_LOGI("start init");
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
    SENSOR_LOGI("X");
    return rtn;
}

static cmr_int s5k3p9sx04_drv_identify(cmr_handle handle, cmr_int param) {
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_int ret_value = SENSOR_FAIL;
    cmr_u16 i = 0;
    cmr_u16 ret;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("identify E");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, s5k3p9sx04_PID_ADDR);
    ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, s5k3p9sx04_VER_ADDR);

    if (s5k3p9sx04_PID_VALUE == pid_value) {
        SENSOR_LOGI("PID = %x, VER = %x", pid_value, ver_value);
        if (s5k3p9sx04_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is s5k3p9sx04 sensor !");
            ret_value = SENSOR_SUCCESS;
            s5k3p9sx04_drv_init_fps_info(handle);
        } else {
            SENSOR_LOGE("SENSOR_s5k3p9sx04: Identify this is %x%x sensor !",
                        pid_value, ver_value);
            return ret_value;
        }
    } else {
        SENSOR_LOGE(
            "SENSOR_s5k3p9sx04: identify fail,pid_value=%x ver_value = %x",
            pid_value, ver_value);
    }

    return ret_value;
}

static cmr_int s5k3p9sx04_drv_write_exp_dummy(cmr_handle handle,
                                              cmr_u16 expsure_line,
                                              cmr_u16 dummy_line,
                                              cmr_u16 size_index) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 frame_len_cur = 0x00;
    cmr_u32 frame_len = 0x00;
    cmr_u32 max_frame_len = 0x00;
    cmr_u32 linetime = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("exposure_line:%d, dummy:%d, size_index:%d", expsure_line,
                dummy_line, size_index);
    max_frame_len = sns_drv_cxt->trim_tab_info[size_index].frame_line;
    if (expsure_line < 3) {
        expsure_line = 3;
    }

    frame_len = expsure_line + dummy_line;
    frame_len = (frame_len > (uint32_t)(expsure_line + 5))
                    ? frame_len
                    : (uint32_t)(expsure_line + 5);
    frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
    if (0x00 != (0x01 & frame_len)) {
        frame_len += 0x01;
    }

    frame_len_cur = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340);

    if (frame_len_cur != frame_len) {
        ret_value =
            hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0340, frame_len);
    }

    ret_value =
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x202, expsure_line);

    sns_drv_cxt->exp_line = expsure_line;
    linetime = sns_drv_cxt->trim_tab_info[size_index].line_time;
    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          expsure_line);
    } else {
        sns_drv_cxt->exif_info.exposure_time = expsure_line;
        sns_drv_cxt->exif_info.exposure_time = expsure_line * linetime / 1000;
    }

    return ret_value;
}

static cmr_int s5k3p9sx04_drv_write_exposure(cmr_handle handle, cmr_int param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u32 expsure_line = 0x00;
    cmr_u32 dummy_line = 0x00;
    cmr_u32 size_index = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    expsure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0x0fff;
    size_index = (param >> 0x1c) & 0x0f;

    SENSOR_LOGI("write_exposure line:%d, dummy:%d, size_index:%d", expsure_line,
                dummy_line, size_index);

    ret_value = s5k3p9sx04_drv_write_exp_dummy(handle, expsure_line, dummy_line,
                                               size_index);

    sns_drv_cxt->sensor_ev_info.preview_shutter = sns_drv_cxt->exp_line;

    return ret_value;
}

static cmr_int s5k3p9sx04_drv_ex_write_exposure(cmr_handle handle,
                                                cmr_int param) {
    cmr_int ret = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    ret = s5k3p9sx04_drv_write_exp_dummy(handle, exposure_line, dummy_line,
                                         size_index);

    sns_drv_cxt->sensor_ev_info.preview_shutter = sns_drv_cxt->exp_line;

    return ret;
}

static cmr_int s5k3p9sx04_drv_update_gain(cmr_handle handle, cmr_int param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    uint32_t real_gain = 0;
    uint32_t a_gain = 0;
    uint32_t d_gain = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    real_gain = param >> 2;

    SENSOR_LOGI("real_gain:%d, param: %ld", real_gain, param);

    if (real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (uint32_t)real_gain, (uint32_t)a_gain, (uint32_t)d_gain);
        if ((uint32_t)d_gain > 256 * 256)
            d_gain = 256 * 256; // d_gain < 256x
    }

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x204, a_gain);

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x20e, d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x210, d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x212, d_gain);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x214, d_gain);

    SENSOR_LOGI("a_gain:0x%x, d_gain: 0x%x", a_gain, d_gain);

    return ret_value;
}

static cmr_int s5k3p9sx04_drv_write_gain(cmr_handle handle, cmr_s32 param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("param: %ld", param);

    ret = s5k3p9sx04_drv_update_gain(handle, param);
    sns_drv_cxt->sensor_ev_info.preview_gain = param;

    return ret;
}

static cmr_int s5k3p9sx04_drv_read_gain(cmr_handle handle, cmr_u32 param) {
    cmr_int rtn = SENSOR_SUCCESS;
    cmr_u32 again = 0;
    cmr_u32 dgain = 0;
    cmr_u32 gain = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    again = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0204);
    dgain = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0210);
    gain = again * dgain;

    SENSOR_LOGI("gain: 0x%x", gain);
    return rtn;
}

static uint16_t s5k3p9sx04_drv_get_shutter(cmr_handle handle) {
    uint16_t shutter;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0202) & 0xffff;

    return shutter;
}

static cmr_int s5k3p9sx04_drv_before_snapshot(cmr_handle handle,
                                              cmr_int param) {
    cmr_u8 ret_l, ret_m, ret_h;
    cmr_u32 capture_exposure, preview_maxline;
    cmr_u32 capture_maxline, preview_exposure;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;
    cmr_u16 exposure_line = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;
    cmr_u32 frame_len = 0x00;
    cmr_u32 gain = 0;

    SENSOR_LOGI("mode: 0x%08lx,capture_mode:%d", param, capture_mode);

    if (sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if (sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

    if (preview_mode == capture_mode) {
        SENSOR_LOGI("prv mode equal to capmode");
        goto CFG_INFO;
    }

    preview_exposure = sns_drv_cxt->sensor_ev_info.preview_shutter;
    gain = sns_drv_cxt->sensor_ev_info.preview_gain;

    capture_exposure = preview_exposure * prv_linetime / cap_linetime;

    SENSOR_LOGI("prev_exp=%d,cap_exp=%d,gain=%d", preview_exposure,
                capture_exposure, gain);

    s5k3p9sx04_drv_write_exp_dummy(handle, capture_exposure, 0, capture_mode);
    s5k3p9sx04_drv_update_gain(handle, gain);

CFG_INFO:
    exposure_line = s5k3p9sx04_drv_get_shutter(handle);

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          exposure_line);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_APERTUREVALUE, 20);
        sns_drv_cxt->ops_cb.set_exif_info(
            sns_drv_cxt->caller_handle, SENSOR_EXIF_CTRL_MAXAPERTUREVALUE, 20);
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_FNUMBER, 200);
    } else {
        sns_drv_cxt->exif_info.exposure_line = exposure_line;
        sns_drv_cxt->exif_info.aperture_value = 29;
        sns_drv_cxt->exif_info.max_aperture_value = 20;
        sns_drv_cxt->exif_info.numerator = 20;
    }
    sns_drv_cxt->exp_line = exposure_line;
    sns_drv_cxt->exp_time = exposure_line * cap_linetime / 1000;

    return SENSOR_SUCCESS;
}

static cmr_int s5k3p9sx04_drv_init_exif_info(cmr_handle handle,
                                             void **exif_info_in /*in*/) {
    cmr_int ret = SENSOR_FAIL;
    EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = NULL;
    *exif_info_in = NULL;
    SENSOR_IC_CHECK_HANDLE(handle);

    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ret = sensor_ic_get_init_exif_info(sns_drv_cxt, &exif_ptr);
    SENSOR_IC_CHECK_PTR(exif_ptr);
    *exif_info_in = exif_ptr;

    SENSOR_LOGI("Start");
    /*aperture = numerator/denominator */
    /*fnumber = numerator/denominator */
    exif_ptr->valid.FNumber = 1;
    exif_ptr->FNumber.numerator = 14;
    exif_ptr->FNumber.denominator = 5;

    exif_ptr->valid.ApertureValue = 1;
    exif_ptr->ApertureValue.numerator = 14;
    exif_ptr->ApertureValue.denominator = 5;
    exif_ptr->valid.MaxApertureValue = 1;
    exif_ptr->MaxApertureValue.numerator = 14;
    exif_ptr->MaxApertureValue.denominator = 5;
    exif_ptr->valid.FocalLength = 1;
    exif_ptr->FocalLength.numerator = 289;
    exif_ptr->FocalLength.denominator = 100;

    exif_ptr->ExposureTime.denominator = 1000000;

    return ret;
}

static void s5k3p9sx04_calc_gain(float gain,
                                 struct sensor_aec_i2c_tag *aec_info) {
    cmr_u32 ret_value = SENSOR_SUCCESS;
    cmr_u16 value = 0x00;
    float real_gain = gain;
    float a_gain = 0;
    float d_gain = 0;
    uint8_t i = 0;

    if ((cmr_u32)real_gain <= 16 * 32) {
        a_gain = real_gain;
        d_gain = 256;
    } else {
        a_gain = 16 * 32;
        d_gain = 256.0 * real_gain / a_gain;
        SENSOR_LOGI("_s5k3p9sx04: real_gain:0x%x, a_gain: 0x%x, d_gain: 0x%x",
                    (cmr_u32)real_gain, (cmr_u32)a_gain, (cmr_u32)d_gain);
        if ((cmr_u32)d_gain > 256 * 256)
            d_gain = 256 * 256; // d_gain < 256x
    }

    aec_info->again->settings[0].reg_value = (cmr_u16)a_gain;
    for (i = 0; i < aec_info->dgain->size; i++)
        aec_info->dgain->settings[i].reg_value = (cmr_u16)d_gain;
}

static cmr_u16 s5k3p9sx04_read_frame_length(cmr_handle handle) {
    cmr_u16 frame_length = 0;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    frame_length = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0340);
    sns_drv_cxt->sensor_ev_info.preview_framelength = frame_length;

    return frame_length;
}

static cmr_u16 s5k3p9sx04_calc_exposure(cmr_handle handle, cmr_u32 shutter,
                                        cmr_u32 dummy_line, cmr_u16 mode,
                                        struct sensor_aec_i2c_tag *aec_info) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;
    cmr_int offset = 0;
    float fps = 0.0;
    float line_time = 0.0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    fr_len = sns_drv_cxt->frame_length_def;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    sns_drv_cxt->frame_length = dest_fr_len;

    cur_fr_len = s5k3p9sx04_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;
    line_time = sns_drv_cxt->trim_tab_info[mode].line_time;
    if (cur_fr_len > shutter) {
        fps = 1000000.0 / (cur_fr_len * line_time);
    } else {
        fps = 1000000.0 / ((shutter + dummy_line) * line_time);
    }
    SENSOR_LOGI("sync fps = %f", fps);

    aec_info->frame_length->settings[0].reg_value = dest_fr_len;
    aec_info->shutter->settings[0].reg_value = shutter;
    return shutter;
}

static struct sensor_reg_tag s5k3p9sx04_shutter_reg[] = {
    {0x0202, 0},
};

static struct sensor_i2c_reg_tab s5k3p9sx04_shutter_tab = {
    .settings = s5k3p9sx04_shutter_reg,
    .size = ARRAY_SIZE(s5k3p9sx04_shutter_reg),
};

static struct sensor_reg_tag s5k3p9sx04_again_reg[] = {
    {0x0204, 0},
};

static struct sensor_i2c_reg_tab s5k3p9sx04_again_tab = {
    .settings = s5k3p9sx04_again_reg, .size = ARRAY_SIZE(s5k3p9sx04_again_reg),
};

static struct sensor_reg_tag s5k3p9sx04_dgain_reg[] = {
    {0x020e, 0}, {0x0210, 0}, {0x0212, 0}, {0x0214, 0},
};

struct sensor_i2c_reg_tab s5k3p9sx04_dgain_tab = {
    .settings = s5k3p9sx04_dgain_reg, .size = ARRAY_SIZE(s5k3p9sx04_dgain_reg),
};

static struct sensor_reg_tag s5k3p9sx04_frame_length_reg[] = {
    {0x0340, 0},
};

static struct sensor_i2c_reg_tab s5k3p9sx04_frame_length_tab = {
    .settings = s5k3p9sx04_frame_length_reg,
    .size = ARRAY_SIZE(s5k3p9sx04_frame_length_reg),
};

static struct sensor_aec_i2c_tag s5k3p9sx04_aec_info = {
#ifdef SENSOR_OV8856_TELE
    .slave_addr = 0x20 >> 1,
#else
    .slave_addr = (MAJOR_I2C_SLAVE_ADDR >> 1),
#endif
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &s5k3p9sx04_shutter_tab,
    .again = &s5k3p9sx04_again_tab,
    .dgain = &s5k3p9sx04_dgain_tab,
    .frame_length = &s5k3p9sx04_frame_length_tab,
};

static cmr_int s5k3p9sx04_read_aec_info(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    float real_gain = 0;
    cmr_u32 gain = 0;
    cmr_u16 frame_interval = 0x00;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;

    info->aec_i2c_info_out = &s5k3p9sx04_aec_info;
    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    frame_interval = (cmr_u16)(
        ((exposure_line + dummy_line) * (trim_info[mode].line_time)) / 1000000);
    SENSOR_LOGI(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        mode, exposure_line, dummy_line, frame_interval);

    sns_drv_cxt->frame_length_def = trim_info[mode].frame_line;
    sns_drv_cxt->line_time_def = trim_info[mode].line_time;

    sns_drv_cxt->sensor_ev_info.preview_shutter = s5k3p9sx04_calc_exposure(
        handle, exposure_line, dummy_line, mode, &s5k3p9sx04_aec_info);

    gain = info->gain < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : info->gain;
    real_gain = (float)info->gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;
    s5k3p9sx04_calc_gain(real_gain, &s5k3p9sx04_aec_info);
    return ret;
}

static cmr_int s5k3p9sx04_drv_stream_on(cmr_handle handle, cmr_s32 param) {
    cmr_s32 ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("StreamOn E");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x0100);

    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        SENSOR_LOGD("enable test mode");
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0600, 0x0002);
    }

    SENSOR_LOGI("StreamOn out");

    return 0;
}

static cmr_int s5k3p9sx04_drv_stream_off(cmr_handle handle, cmr_int param) {
    UNUSED(param);
    cmr_s32 ret = SENSOR_SUCCESS;
    uint16_t value;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    SENSOR_LOGI("StreamOff:E");

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);
    ret = hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x0000);
    if (value == 0x0100)
        usleep(50 * 1000);

    return ret;
}

static cmr_int s5k3p9sx04_drv_power_on(cmr_handle handle, cmr_int power_on) {
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
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, dvdd_val, avdd_val,
                              iovdd_val);
        usleep(1);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(20);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DEFALUT_MCLK);
        usleep(1000);
    } else {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED,
                              SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
    }

    SENSOR_LOGI("s5k3p9sx04_drv_power_on(1:on, 0:off): %ld, "
                "reset_level %d, dvdd_val %d",
                power_on, reset_level, dvdd_val);
    return SENSOR_SUCCESS;
}

static cmr_int s5k3p9sx04_drv_get_static_info(cmr_handle handle,
                                              cmr_u32 *param) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    uint32_t up = 0;
    uint32_t down = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    if (!(fps_info && static_info && module_info)) {
        SENSOR_LOGE("error:null pointer checked.return");
        return SENSOR_FAIL;
    }

    if (!fps_info->is_init) {
        s5k3p9sx04_drv_init_fps_info(handle);
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

    memcpy(&ex_info->fov_info, &static_info->fov_info,
           sizeof(static_info->fov_info));

    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info("s5k3p9sx04", ex_info);

    return ret;
}

static cmr_int s5k3p9sx04_drv_get_fps_info(cmr_handle handle, uint32_t *param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    if (!fps_data->is_init) {
        s5k3p9sx04_drv_init_fps_info(handle);
    }
    fps_info = (SENSOR_MODE_FPS_T *)param;
    uint32_t sensor_mode = fps_info->mode;
    fps_info->max_fps = fps_data->sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps = fps_data->sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps = fps_data->sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        fps_data->sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_LOGI("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_LOGI("min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return ret;
}

static cmr_int s5k3p9sx04_drv_access_val(cmr_handle handle, cmr_int param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    uint16_t tmp;

    SENSOR_LOGI("param_ptr = %p", param_ptr);
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_SHUTTER:
        *((uint32_t *)param_ptr->pval) = s5k3p9sx04_drv_get_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_GET_AFPOSITION:
        *(uint32_t *)param_ptr->pval = 0; // cur_af_pos;
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        rtn = s5k3p9sx04_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        rtn = s5k3p9sx04_drv_get_fps_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }

    SENSOR_LOGI("X");

    return rtn;
}

static cmr_int
s5k3p9sx04_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                             cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;

    SENSOR_LOGI("E:");
    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    if (SENSOR_IC_FAILED == ret) {
        SENSOR_LOGE("sensor instance create failed!");
        return ret;
    }
    sns_drv_cxt = *sns_ic_drv_handle;

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(MODULE_INFO),
                                    MODULE_INFO);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(RES_TAB_RAW),
                                        RES_TAB_RAW);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(RES_TRIM_TAB),
                                  RES_TRIM_TAB);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(STATIC_INFO),
                                    STATIC_INFO);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(FPS_INFO), FPS_INFO);

    s5k3p9sx04_drv_init_exif_info(sns_drv_cxt, &sns_drv_cxt->exif_ptr);

    SENSOR_LOGI("X:");

    return ret;
}

static cmr_s32 s5k3p9sx04_drv_handle_delete(cmr_handle handle,
                                            uint32_t *param) {
    cmr_s32 ret = SENSOR_SUCCESS;
    /*if has private data,you must release it here*/
    SENSOR_LOGI("E:");
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ret = sensor_ic_drv_delete(handle, param);
    SENSOR_LOGI("X:");

    return ret;
}

static cmr_int s5k3p9sx04_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                               void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

void *sensor_ic_open_lib(void)
{
     return &g_s5k3p9sx04_mipi_raw_info;
}

static struct sensor_ic_ops s5k3p9sx04_ops_tab = {
    .create_handle = s5k3p9sx04_drv_handle_create,
    .delete_handle = s5k3p9sx04_drv_handle_delete,
    .get_data = s5k3p9sx04_drv_get_private_data,
    .power = s5k3p9sx04_drv_power_on,
    .identify = s5k3p9sx04_drv_identify,

    .write_exp = s5k3p9sx04_drv_write_exposure,
    .write_gain_value = s5k3p9sx04_drv_write_gain,
    .ex_write_exp = s5k3p9sx04_drv_ex_write_exposure,
    .read_aec_info = s5k3p9sx04_read_aec_info,
    .ext_ops = {
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = s5k3p9sx04_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = s5k3p9sx04_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = s5k3p9sx04_drv_stream_off,
            [SENSOR_IOCTL_ACCESS_VAL].ops = s5k3p9sx04_drv_access_val,
    }};
