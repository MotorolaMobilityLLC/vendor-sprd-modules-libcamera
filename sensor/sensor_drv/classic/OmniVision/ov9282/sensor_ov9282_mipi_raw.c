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
 * V6.0
 */
/*History
*Date                  Modification                                 Reason
*
*/

#include "sensor_ov9282_mipi_raw.h"

#undef LOG_TAG
#define LOG_TAG "ov9282_mipi_raw"

#define FPS_INFO s_ov9282_mode_fps_info
#define STATIC_INFO s_ov9282_static_info
#define VIDEO_INFO s_ov9282_video_info
#define MODULE_INFO s_ov9282_module_info_tab
#define RES_TAB_RAW s_ov9282_resolution_tab_raw
#define RES_TRIM_TAB s_ov9282_resolution_trim_tab
#define MIPI_RAW_INFO g_ov9282_mipi_raw_info

static cmr_int ov9282_drv_init_fps_info(cmr_handle handle) {
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

static cmr_int
ov9282_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
                          cmr_handle *sns_ic_drv_handle) {
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt *sns_drv_cxt = NULL;
    void *pri_data = NULL;

    ret = sensor_ic_drv_create(init_param, sns_ic_drv_handle);
    sns_drv_cxt = *sns_ic_drv_handle;

    sns_drv_cxt->sensor_ev_info.preview_shutter =
        PREVIEW_FRAME_LENGTH - FRAME_OFFSET;
    sns_drv_cxt->sensor_ev_info.preview_gain = SENSOR_BASE_GAIN;
    sns_drv_cxt->sensor_ev_info.preview_framelength = PREVIEW_FRAME_LENGTH;

    sns_drv_cxt->frame_length_def = PREVIEW_FRAME_LENGTH;

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
    ov9282_drv_init_fps_info(sns_drv_cxt);

    /*add private here*/
    return ret;
}

static cmr_int ov9282_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int ov9282_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                            void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

static cmr_int ov9282_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = MIPI_RAW_INFO.power_down_level;
    BOOLEAN reset_level = MIPI_RAW_INFO.reset_pulse_level;
    cmr_int ret = SENSOR_SUCCESS;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_voltage(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED,
                              SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		
        hw_sensor_set_monitor_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED); 
        usleep(1 * 1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);

        usleep(1 * 1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        hw_sensor_set_monitor_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_3300MV); 
        usleep(6 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, EX_MCLK);
    } else {
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(500);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(200);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_monitor_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED); 
    }
    SENSOR_LOGI("(1:on, 0:off): %lu", power_on);
    return SENSOR_SUCCESS;
}

static cmr_int ov9282_drv_identify(cmr_handle handle, cmr_uint param) {
    cmr_u16 chip_id_h = 0x00, chip_id_l = 0x00;
    cmr_int ret_value = SENSOR_FAIL;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("mipi raw identify");

    chip_id_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov9282_CHIP_ID_H_ADDR);
    chip_id_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov9282_CHIP_ID_L_ADDR);
    SENSOR_LOGI("Identify: CHIP_ID_H = %x, CHIP_ID_L = %x", chip_id_h, chip_id_l);
    if (chip_id_h == ov9282_CHIP_ID_H_VALUE && chip_id_l == ov9282_CHIP_ID_L_VALUE) {
        SENSOR_LOGI("this is ov9282 sensor");
	ret_value = SENSOR_SUCCESS;
    } else {
        SENSOR_LOGI("Identify this is %x%x sensor", chip_id_h, chip_id_l);
    }
    return ret_value;
}

static cmr_u16 ov9282_drv_read_gain(cmr_handle handle) {
    cmr_u32 gain;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    gain = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350a) & 0xff;
    gain = gain << 8 |
                (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x350b) & 0xff);
    sns_drv_cxt->sensor_ev_info.preview_gain = gain;

    return gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov9282_drv_write_gain(cmr_handle handle, float gain) {
    float gain_a = gain;
    float gain_d = 0x400; // spec p70, X1 = 15bit
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

 /*   if (SENSOR_MAX_GAIN < (cmr_u16)gain_a) {
        gain_a = SENSOR_MAX_GAIN;
        gain_d = gain * 0x400 / gain_a;
        if ((cmr_u16)gain_d > 0x2 * 0x400 - 1)
            gain_d = 0x2 * 0x400 - 1;
    }*/
    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x320a, 0x01);
    // group 1:all other registers( gain)
   // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3208, 0x01);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350a,
                        ((cmr_u16)gain_a >> 8) & 0x1f);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x350b, (cmr_u16)gain_a & 0xff);
  /*  hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5100,
                        ((cmr_u16)gain_d >> 8) & 0x7f);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5101, (cmr_u16)gain_d & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5102,
                        ((cmr_u16)gain_d >> 8) & 0x7f);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5103, (cmr_u16)gain_d & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5104,
                        ((cmr_u16)gain_d >> 8) & 0x7f);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x5105, (cmr_u16)gain_d & 0xff);*/
  //  hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3208, 0x11);
  //  hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3208, 0xA1);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 ov9282_drv_read_frame_length(cmr_handle handle) {
    cmr_u32 frame_len;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    frame_len = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380e) & 0xff;
    frame_len = frame_len << 8 |
                (hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x380f) & 0xff);
    sns_drv_cxt->sensor_ev_info.preview_framelength = frame_len;

    return frame_len;
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov9282_drv_write_frame_length(cmr_handle handle,
                                           cmr_u32 frame_len) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380e,
                        (frame_len >> 8) & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x380f, frame_len & 0xff);
    sns_drv_cxt->sensor_ev_info.preview_framelength = frame_len;
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u32 ov9282_drv_read_shutter(cmr_handle handle) {
    cmr_u32 value = 0x00;
    cmr_u8 shutter_l = 0x00;
    cmr_u8 shutter_m = 0x00;
    cmr_u8 shutter_h = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    shutter_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3502);
    // value=(shutter>>0x04)&0x0f;
    shutter_m = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3501);
    // value+=(shutter&0xff)<<0x04;
    shutter_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x3500);
    // value+=(shutter&0x0f)<<0x0c;
    value = ((shutter_h & 0x0f) << 12) + (shutter_m << 4) +
            ((shutter_l >> 4) & 0x0f);
    sns_drv_cxt->sensor_ev_info.preview_shutter = value;

    return value;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov9282_drv_write_shutter(cmr_handle handle, cmr_u32 shutter) {
    cmr_u16 value = 0x00;
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = (shutter) & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3502, value);
    value = (shutter >> 0x08) & 0xff;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3501, value);
    value = (shutter >> 0x10) & 0x0f;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3500, value);
    sns_drv_cxt->sensor_ev_info.preview_shutter = shutter;
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static cmr_int ov9282_drv_write_exposure_dummy(cmr_handle handle,
                                                cmr_u32 shutter,
                                                cmr_u32 dummy_line,
                                                cmr_u16 size_index) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    fr_len = sns_drv_cxt->trim_tab_info[size_index].frame_line;
    // ov9282_group_hold_on(handle);

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    sns_drv_cxt->frame_length = dest_fr_len;

    cur_fr_len = ov9282_drv_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        ov9282_drv_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    sns_drv_cxt->sensor_ev_info.preview_shutter = shutter;

    ov9282_drv_write_shutter(handle, shutter);
#if 0	
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3927, (shutter >> 8) & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3928, shutter & 0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3929, ((dest_fr_len - 8)>>8)&0xff);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x392a, (dest_fr_len - 8)&0xff);
#endif
    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(
            sns_drv_cxt->caller_handle, SENSOR_EXIF_CTRL_EXPOSURETIME, shutter);
    }

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int ov9282_drv_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;
    cmr_u16 frame_interval = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_HANDLE(ex);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    frame_interval =
        (cmr_u16)(((exposure_line + dummy_line) *
                   (sns_drv_cxt->trim_tab_info[size_index].line_time)) /
                  1000000);
    SENSOR_LOGI(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        size_index, exposure_line, dummy_line, frame_interval);
    sns_drv_cxt->frame_length_def =
        sns_drv_cxt->trim_tab_info[size_index].frame_line;
    sns_drv_cxt->line_time_def =
        sns_drv_cxt->trim_tab_info[size_index].line_time;

    ret_value = ov9282_drv_write_exposure_dummy(handle, exposure_line,
                                                 dummy_line, size_index);

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
static cmr_int ov9282_drv_write_gain_value(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    float real_gain = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    // real_gain = isp_to_real_gain(handle,param);
    // SENSOR_LOGI("param = %d", param);
    param = param < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : param;

    real_gain = (float)1.0f * param * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    SENSOR_LOGI("real_gain = %f", real_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = real_gain;
    ov9282_drv_write_gain(handle, real_gain);

    return ret_value;
}

static struct sensor_reg_tag ov9282_shutter_reg[] = {
    {0x3502, 0}, {0x3501, 0}, {0x3500, 0},
};

static struct sensor_i2c_reg_tab ov9282_shutter_tab = {
    .settings = ov9282_shutter_reg, .size = ARRAY_SIZE(ov9282_shutter_reg),
};

static struct sensor_reg_tag ov9282_again_reg[] = {
    {0x350a, 0x00}, {0x350b, 0x00},
};

static struct sensor_i2c_reg_tab ov9282_again_tab = {
    .settings = ov9282_again_reg, .size = ARRAY_SIZE(ov9282_again_reg),
};

static struct sensor_reg_tag ov9282_dgain_reg[] = {
};

struct sensor_i2c_reg_tab ov9282_dgain_tab = {
    .settings = ov9282_dgain_reg, .size = ARRAY_SIZE(ov9282_dgain_reg),
};

static struct sensor_reg_tag ov9282_frame_length_reg[] = {
    {0x380e, 0}, {0x380f, 0},
};

static struct sensor_i2c_reg_tab ov9282_frame_length_tab = {
    .settings = ov9282_frame_length_reg,
    .size = ARRAY_SIZE(ov9282_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov9282_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov9282_shutter_tab,
    .again = &ov9282_again_tab,
    .dgain = &ov9282_dgain_tab,
    .frame_length = &ov9282_frame_length_tab,
};

static cmr_u16 ov9282_drv_calc_exposure(cmr_handle handle, cmr_u32 shutter,
                                         cmr_u32 dummy_line, cmr_u16 mode,
                                         struct sensor_aec_i2c_tag *aec_info) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;
    int32_t offset = 0;
    cmr_u16 value = 0x00;
    float fps = 0.0;
    float line_time = 0.0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    fr_len = sns_drv_cxt->frame_length_def;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    sns_drv_cxt->frame_length = dest_fr_len;

    cur_fr_len = ov9282_drv_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    line_time = sns_drv_cxt->trim_tab_info[mode].line_time;
    if (cur_fr_len > shutter) {
        fps = 1000000.0 / (cur_fr_len * line_time);
    } else {
        fps = 1000000.0 / ((shutter + dummy_line) * line_time);
    }
    SENSOR_LOGI("sync fps = %f", fps);
    aec_info->frame_length->settings[0].reg_value = (dest_fr_len >> 8) & 0xff;
    aec_info->frame_length->settings[1].reg_value = dest_fr_len & 0xff;
    value = (shutter) & 0xff;
    aec_info->shutter->settings[0].reg_value = value;
    value = (shutter >> 0x08) & 0xff;
    aec_info->shutter->settings[1].reg_value = value;
    value = (shutter >> 0x10) & 0x0f;
    aec_info->shutter->settings[2].reg_value = value;
    return shutter;
}

static void ov9282_drv_calc_gain(float gain,
                                  struct sensor_aec_i2c_tag *aec_info) {
    float gain_a = gain;
    float gain_d = 0x400;

/*    if (SENSOR_MAX_GAIN < (cmr_u16)gain_a) {

        gain_a = SENSOR_MAX_GAIN;
        gain_d = gain * 0x400 / gain_a;
        if ((cmr_u16)gain_d > 0x4 * 0x400 - 1)
            gain_d = 0x4 * 0x400 - 1;
    }*/
    // group 1:all other registers( gain)
    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3208, 0x01);

    aec_info->again->settings[1].reg_value = ((cmr_u16)gain_a >> 8) & 0x1f;
    aec_info->again->settings[2].reg_value = (cmr_u16)gain_a & 0xff;
/*
    aec_info->dgain->settings[0].reg_value = ((cmr_u16)gain_d >> 8) & 0x7f;
    aec_info->dgain->settings[1].reg_value = (cmr_u16)gain_d & 0xff;
    aec_info->dgain->settings[2].reg_value = ((cmr_u16)gain_d >> 8) & 0x7f;
    aec_info->dgain->settings[3].reg_value = (cmr_u16)gain_d & 0xff;
    aec_info->dgain->settings[4].reg_value = ((cmr_u16)gain_d >> 8) & 0x7f;
    aec_info->dgain->settings[5].reg_value = (cmr_u16)gain_d & 0xff;*/
}

static cmr_int ov9282_drv_read_aec_info(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    float real_gain = 0;
    cmr_u32 gain = 0;
    cmr_u16 frame_interval = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    info->aec_i2c_info_out = &ov9282_aec_info;

    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    frame_interval = (cmr_u16)(((exposure_line + dummy_line) *
                                (sns_drv_cxt->trim_tab_info[mode].line_time)) /
                               1000000);
    SENSOR_LOGI(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        mode, exposure_line, dummy_line, frame_interval);
    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;
    //        ov9282_drv_get_default_frame_length(handle, mode);
    //    s_current_default_line_time =
    //    s_ov9282_resolution_trim_tab[mode].line_time;
    sns_drv_cxt->line_time_def = sns_drv_cxt->trim_tab_info[mode].line_time;

    sns_drv_cxt->sensor_ev_info.preview_shutter = ov9282_drv_calc_exposure(
        handle, exposure_line, dummy_line, mode, &ov9282_aec_info);

    gain = info->gain < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : info->gain;
    real_gain = (float)info->gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;
    ov9282_drv_calc_gain(real_gain, &ov9282_aec_info);
    return ret_value;
}

#if defined(CONFIG_DUAL_MODULE)
cmr_int ov9282_drv_set_master_FrameSync(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3006, 0x02);//0: slave 2:master
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3823,  0x00);//column
    return 0;
}
cmr_int ov9282_drv_set_slave_FrameSync(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3006, 0x00);//0: slave 2:master
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x38b3,  0x07);//column
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3885,  0x07);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x382b,  0x3a);//row
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3670,  0x68);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3823,  0x30);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3824,  0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3825,  0x08);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3826,  0x03);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3827,  0x8a);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3740,  0x01);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3741,  0x08);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3742,  0x01);
    return 0;
}

#endif

static cmr_int ov9282_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u32 cap_shutter = 0;
    cmr_u32 prv_shutter = 0;
    cmr_u32 gain = 0;
    cmr_u32 cap_gain = 0;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    sns_drv_cxt->frame_length_def =
        sns_drv_cxt->trim_tab_info[capture_mode].frame_line;

    SENSOR_LOGI("capture_mode = %d", capture_mode);

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

    //cap_shutter = ov13855_drv_write_exposure_dummy(handle, cap_shutter, 0, 0);
    cap_gain = gain;
    //ov13855_drv_write_gain(handle, cap_gain);
    SENSOR_LOGI("preview_shutter = 0x%x, preview_gain = %f",
                sns_drv_cxt->sensor_ev_info.preview_shutter,
                sns_drv_cxt->sensor_ev_info.preview_gain);

    SENSOR_LOGI("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter,
                cap_gain);
snapshot_info:

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                                          SENSOR_EXIF_CTRL_EXPOSURETIME,
                                          cap_shutter);
    } else {
        sns_drv_cxt->exif_info.exposure_line = cap_shutter;
    }
    return SENSOR_SUCCESS;
}

static cmr_int ov9282_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    if (1){//(sns_drv_cxt->sensor_id % 2 == 0){
         hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3820, 0x40);
         hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3821, 0x00);
    }
#if 1
	char value0[PROPERTY_VALUE_MAX];
	char value1[PROPERTY_VALUE_MAX];
	property_get("debug.camera.test.mode", value1, "0");
	if (!strcmp(value1, "1")) {
	 //   hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x4503, 0x80);
	}
	property_get("persist.vendor.cam.ae.ir.manual", value0, "0");
	if (!strcmp(value0, "1")) {
		property_get("persist.vendor.cam.ae.ir.expos", value1, "50000"); // us
		SENSOR_LOGI("value1 %d ", atoi(value1));
		cmr_u32 size_index = 1;
		cmr_u32 exposure = atoi(value1) * 1000 / sns_drv_cxt->trim_tab_info[size_index].line_time;
		cmr_u32 dummy = exposure > FRAME_LINE ? exposure - FRAME_LINE : 0;
		dummy = dummy > 0xffff ? 0xffff : dummy;
		property_get("persist.vendor.cam.ae.ir.gain", value1, "1280");
		ov9282_drv_write_exposure_dummy(handle, exposure, dummy, size_index);
		ov9282_drv_write_gain(handle, atoi(value1));
	}
#endif

    SENSOR_LOGI("E");
    ov9282_drv_set_slave_FrameSync(handle, param);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);
 //   hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x4242, 0x00);
    /*delay*/
    usleep(10 * 1000);
#if 0
                /* enable strobe pin for IR1 */
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3005, 0x0a);//8);
     //   hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b96, 0xe0);
#if 1
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b81, 0xff);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b96, 0xc0);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b8a, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b8b, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b8c, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b8d, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b8e, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3b8f, 0x80);

#endif
#if 0
         ov9282_drv_write_ir1(sns_drv_cxt->hw_handle, 0x3005, 0x0a);
    /* ir1 ir2 aec */
#if 1
        ov9282_drv_write_ir1(sns_drv_cxt->hw_handle, 0x3501, 0x80);
        ov9282_drv_write_ir2(sns_drv_cxt->hw_handle, 0x3501, 0x80);
#endif
#endif
              //  hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, SENSOR_LOW_PULSE_RESET);
              //  usleep(600);
               // hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, SENSOR_HIGH_PULSE_RESET);
              //  usleep(600);
             //   hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, SENSOR_LOW_PULSE_RESET);

                /* use this pin as IR projector reset pin for debug */
                hw_sensor_i2c_set_addr(sns_drv_cxt->hw_handle, 0x40);//c0>>1);//40);//0x90 >> 1);//0x40);
/*			for(int i=0;i<1000;i++){
    			SENSOR_LOGI("E %d", i);
				usleep(1000*1000);	
				hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x0c, 0xaa);
			}*/
	       hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x0c, 0xaa);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x0d, 0xaa);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x02, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x03, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x04, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x05, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x06, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x07, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x08, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x09, 0xff);
                hw_sensor_write_reg_8bits(sns_drv_cxt->hw_handle, 0x00, 0x01);
                hw_sensor_i2c_set_addr(sns_drv_cxt->hw_handle, 0xe0 >> 1);
#endif
#if 0
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3006, 0x08);//0a);//8); strobe
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3210, 0x04);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3007, 0x02);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x301c, 0xf0);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3020, 0x20);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3025, 0x02);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x382c, 0x05);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x382d, 0xb0);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3920, 0xff);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3923, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3924, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3925, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3926, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3927, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3928, 0x80);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x392b, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x392c, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x392d, 0x02);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x392e, 0xe8);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x392f, 0xcb);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x38b3, 0x07);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3885, 0x07);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x382b, 0x3a);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3670, 0x68);
/*
//shutter
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3927, 0x00);//strobe width
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3928, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3929, 0x00);//strobe start point is vts - Texposure-7-Tnegative
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x392a, 0x00);

//FSIN master
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3006, 0x02);//bit[1]=1 output
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3823, 0x00);//0100 01 stream on
        //slave
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3006, 0x00);//bit[1]=0 input
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3666, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x38b3, 0x07);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3885, 0x07);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x382b, 0x3a);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3670, 0x68);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3823, 0x30);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3824, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3825, 0x08);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3826, 0x03);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3827, 0x8a);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3740, 0x01);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3741, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3742, 0x08);//0100 01


*/
#endif


    return 0;
}

static cmr_int ov9282_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    cmr_u8 value;
    cmr_u32 sleep_time = 0, frame_time;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);
    if (value != 0x00) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x4242, 0x0f);
        if (!sns_drv_cxt->is_sensor_close) {
            sleep_time = 50 * 1000;
            usleep(sleep_time);
        }
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x4242, 0x0f);
    }

    sns_drv_cxt->is_sensor_close = 0;
    SENSOR_LOGI("X sleep_time=%dus", sleep_time);
    return 0;
}

static cmr_int ov9282_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = (struct sensor_ex_info *)param;
    cmr_u32 up = 0;
    cmr_u32 down = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    // make sure we have get max fps of all settings.
    if (!fps_info->is_init) {
        ov9282_drv_init_fps_info(handle);
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
    ex_info->name = (cmr_s8 *)MIPI_RAW_INFO.name;
    ex_info->sensor_version_info = (cmr_s8 *)MIPI_RAW_INFO.sensor_version_info;

    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info((cmr_s8 *)SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int ov9282_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        ov9282_drv_init_fps_info(handle);
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

static cmr_int ov9282_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = ov9282_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = ov9282_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        ret = sns_drv_cxt->is_sensor_close = 1;
        break;
    default:
        break;
    }
    ret = SENSOR_SUCCESS;

    return ret;
}

static struct sensor_ic_ops s_ov9282_ops_tab = {
    .create_handle = ov9282_drv_handle_create,
    .delete_handle = ov9282_drv_handle_delete,
    .get_data = ov9282_drv_get_private_data,

    .power = ov9282_drv_power_on,
    .identify = ov9282_drv_identify,
    .ex_write_exp = ov9282_drv_write_exposure,
    .write_gain_value = ov9282_drv_write_gain_value,

    .ext_ops = {
        [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = ov9282_drv_before_snapshot,
        [SENSOR_IOCTL_STREAM_ON].ops = ov9282_drv_stream_on,
        [SENSOR_IOCTL_STREAM_OFF].ops = ov9282_drv_stream_off,
        [SENSOR_IOCTL_ACCESS_VAL].ops = ov9282_drv_access_val,
    },
};
