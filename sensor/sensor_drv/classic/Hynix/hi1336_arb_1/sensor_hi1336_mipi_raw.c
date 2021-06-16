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
 * V1.0
 */
/*History
*Date                  Modification                                 Reason
*
*/

#define LOG_TAG "hi1336_arb_1_ums9230"


#include "sensor_hi1336_mipi_raw_4lane.h"

static char back_cam_efuse_id[64] = "0";
static bool identify_success = false;
static cmr_u8 hi1336_snsotp_pid[32] = {0};
static cmr_u8 hi1336_snsotp_pid_size = 32;
/*==============================================================================
 * Description:
 * write register value to sensor
 * please modify this function acording your spec
 *============================================================================*/


static void get_back_cam_efuse_id(cmr_handle handle);

static void hi1336_drv_write_reg2sensor(cmr_handle handle,
                                       struct sensor_i2c_reg_tab *reg_info) {
    SENSOR_IC_CHECK_PTR_VOID(reg_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_int i = 0;

    for (i = 0; i < reg_info->size; i++) {

        hw_sensor_write_reg(sns_drv_cxt->hw_handle,
                            reg_info->settings[i].reg_addr,
                            reg_info->settings[i].reg_value);
    }
}

/*==============================================================================
 * Description:
 * write gain to sensor registers buffer
 * please modify this function acording your spec
 *============================================================================*/
static void hi1336_drv_write_gain(cmr_handle handle,
                                 struct sensor_aec_i2c_tag *aec_info,
                                 cmr_u32 gain) {
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (aec_info->again->size) {
        /*TODO*/
        aec_info->again->settings[0].reg_value = (gain - 16) & 0xff;

        /*END*/
    }

    if (aec_info->dgain->size) {

        /*TODO*/

        /*END*/
    }
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers buffer
 * please modify this function acording your spec
 *============================================================================*/
static void hi1336_drv_write_frame_length(cmr_handle handle,
                                         struct sensor_aec_i2c_tag *aec_info,
                                         cmr_u32 frame_len) {
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (aec_info->frame_length->size) {
        /*TODO*/

        aec_info->frame_length->settings[0].reg_value = frame_len;

        /*END*/
    }
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers buffer
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void hi1336_drv_write_shutter(cmr_handle handle,
                                    struct sensor_aec_i2c_tag *aec_info,
                                    cmr_u32 shutter) {
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (aec_info->shutter->size) {
        /*TODO*/
        aec_info->shutter->settings[0].reg_value = shutter & 0xffff;

        /*END*/
    }
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static void hi1336_drv_calc_exposure(cmr_handle handle, cmr_u32 shutter,
                                    cmr_u32 dummy_line, cmr_u16 mode,
                                    struct sensor_aec_i2c_tag *aec_info) {
    cmr_u32 dest_fr_len = 0;
    cmr_u32 cur_fr_len = 0;
    cmr_u32 fr_len = 0;
    float fps = 0.0;
    cmr_u16 frame_interval = 0x00;

    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;
    sns_drv_cxt->line_time_def = sns_drv_cxt->trim_tab_info[mode].line_time;
    cur_fr_len = sns_drv_cxt->sensor_ev_info.preview_framelength;
    fr_len = sns_drv_cxt->frame_length_def;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    sns_drv_cxt->frame_length = dest_fr_len;

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (cur_fr_len > shutter) {
        fps = 1000000000.0 /
              (cur_fr_len * sns_drv_cxt->trim_tab_info[mode].line_time);
    } else {
        fps = 1000000000.0 / ((shutter + dummy_line) *
                              sns_drv_cxt->trim_tab_info[mode].line_time);
    }
    SENSOR_LOGI("fps = %f", fps);

    frame_interval = (uint16_t)(
        ((shutter + dummy_line) * sns_drv_cxt->line_time_def) / 1000000);
    SENSOR_LOGI(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        mode, shutter, dummy_line, frame_interval);

    if (dest_fr_len != cur_fr_len) {
        sns_drv_cxt->sensor_ev_info.preview_framelength = dest_fr_len;
        hi1336_drv_write_frame_length(handle, aec_info, dest_fr_len);
    }
    sns_drv_cxt->sensor_ev_info.preview_shutter = shutter;
    hi1336_drv_write_shutter(handle, aec_info, shutter);

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(
            sns_drv_cxt->caller_handle, SENSOR_EXIF_CTRL_EXPOSURETIME, shutter);
    }
}

static void hi1336_drv_calc_gain(cmr_handle handle, cmr_uint isp_gain,
                                struct sensor_aec_i2c_tag *aec_info) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u32 sensor_gain = 0;

    sensor_gain = isp_gain < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : isp_gain;
    sensor_gain = sensor_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;
    if (SENSOR_MAX_GAIN < sensor_gain)
        sensor_gain = SENSOR_MAX_GAIN;

    SENSOR_LOGI("isp_gain = 0x%x,sensor_gain=0x%x", (unsigned int)isp_gain,
                sensor_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = sensor_gain;
    hi1336_drv_write_gain(handle, aec_info, sensor_gain);
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi1336_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = g_hi1336_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_hi1336_mipi_raw_info.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        //hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(1 * 1000);

        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, EX_MCLK);
        hw_sensor_set_mipi_level(sns_drv_cxt->hw_handle, 1);
        //hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(1 * 1000);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(2 * 1000);
    } else {
        usleep(500);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(200);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }

    SENSOR_LOGI("(1:on, 0:off): %lu", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi1336_drv_init_fps_info(cmr_handle handle) {
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

static cmr_int hi1336_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
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
        hi1336_drv_init_fps_info(handle);
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
    ex_info->name = (cmr_s8 *)g_hi1336_mipi_raw_info.name;
    ex_info->sensor_version_info =
        (cmr_s8 *)g_hi1336_mipi_raw_info.sensor_version_info;
    memcpy(&ex_info->fov_info, &static_info->fov_info,
             sizeof(static_info->fov_info));

    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info((cmr_s8 *)SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int hi1336_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        hi1336_drv_init_fps_info(handle);
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

static cmr_int hi1336_drv_get_snsotp_pid(cmr_handle handle, cmr_u32 *param) {
    struct snsotp_pid_info *pid_info = (struct sensor_ex_info *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(pid_info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (identify_success) {
        for (int i = 0; i < hi1336_snsotp_pid_size; i++) {
                pid_info->snsotp_pid[i] = back_cam_efuse_id[i];
        }
        pid_info->snsotp_pid_size = hi1336_snsotp_pid_size;
    } else {
        return SENSOR_FAIL;
    }

    return SENSOR_SUCCESS;
}

static const cmr_u16 hi1336_pd_is_right[] = {0, 1, 0, 1};   //
static const cmr_u16 hi1336_pd_row[] = {4, 0, 8, 12};     //y
static const cmr_u16 hi1336_pd_col[] = {5, 5, 13, 13};   //x
static const struct pd_pos_info _hi1336_pd_pos_l[] = {{5, 4}, {13, 8}};
static const struct pd_pos_info _hi1336_pd_pos_r[] = {{5, 0}, {13, 12}};
static const cmr_u32 pd_sns_mode[] = {0, 0, 0, 1};
static cmr_int hi1336_drv_get_pdaf_info(cmr_handle handle, cmr_u32 *param) {

    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_pdaf_info *pdaf_info = NULL;
    cmr_u16 i = 0;
    cmr_u16 pd_pos_row_size = 0;
    cmr_u16 pd_pos_col_size = 0;
    cmr_u16 pd_pos_is_right_size = 0;
    SENSOR_IC_CHECK_PTR(param);
    SENSOR_LOGE("E\n");
    pdaf_info = (struct sensor_pdaf_info *)param;
    pd_pos_is_right_size = NUMBER_OF_ARRAY(hi1336_pd_is_right);
    pd_pos_row_size = NUMBER_OF_ARRAY(hi1336_pd_row);
    pd_pos_col_size = NUMBER_OF_ARRAY(hi1336_pd_col);

    if ((pd_pos_row_size != pd_pos_col_size) ||
        (pd_pos_row_size != pd_pos_is_right_size) ||
        (pd_pos_is_right_size != pd_pos_col_size)) {
        SENSOR_LOGE("pd_pos_row size and pd_pos_is_right size are not match");
        return SENSOR_FAIL;
    }

    pdaf_info->pd_offset_x = 56;
    pdaf_info->pd_offset_y = 24;
    pdaf_info->pd_end_x = 4152;
    pdaf_info->pd_end_y = 3096;
    pdaf_info->pd_block_w = 1;
    pdaf_info->pd_block_h = 1;
    pdaf_info->pd_block_num_x = 256;
    pdaf_info->pd_block_num_y = 192;
    pdaf_info->pd_is_right = (cmr_u16 *)hi1336_pd_is_right;
    pdaf_info->pd_pos_row = (cmr_u16 *)hi1336_pd_row;
    pdaf_info->pd_pos_col = (cmr_u16 *)hi1336_pd_col;


    cmr_u16 pd_pos_r_size = NUMBER_OF_ARRAY(_hi1336_pd_pos_r);
    cmr_u16 pd_pos_l_size = NUMBER_OF_ARRAY(_hi1336_pd_pos_l);

    if (pd_pos_r_size != pd_pos_l_size) {
        SENSOR_LOGE("pd_pos_r size not match pd_pos_l");
        return SENSOR_FAIL;
    }
    pdaf_info->pd_pitch_x = 16;
    pdaf_info->pd_pitch_y = 16;
    pdaf_info->pd_density_x = 16;
    pdaf_info->pd_density_y = 8;
    pdaf_info->pd_pos_size = pd_pos_r_size;
    pdaf_info->pd_pos_r = (struct pd_pos_info *)_hi1336_pd_pos_r;
    pdaf_info->pd_pos_l = (struct pd_pos_info *)_hi1336_pd_pos_l;
    pdaf_info->vch2_info.bypass = 0;
    pdaf_info->vch2_info.vch2_vc = 0;
    pdaf_info->vch2_info.vch2_data_type = 0x30;
    pdaf_info->vch2_info.vch2_mode = 0x03;
    pdaf_info->sns_mode = pd_sns_mode;
    pdaf_info->sns_orientation = 0; /*1: mirror+flip; 0: normal*/
    pdaf_info->pd_data_size = pdaf_info->pd_block_num_x * pdaf_info->pd_block_num_y
				* pd_pos_is_right_size * 5;
    return rtn;
}
/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi1336_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_FAIL;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("sensor hi1336: param_ptr->type=%x", param_ptr->type);

    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = hi1336_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = hi1336_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        ret = sns_drv_cxt->is_sensor_close = 1;
        break;
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        ret = hi1336_drv_get_pdaf_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
           // ret = hi1336_qunhui_identify_otp(handle, s_hi1336_otp_info_ptr, param_ptr);
    case SENSOR_VAL_TYPE_GET_SNSOTP_PID:
        ret = hi1336_drv_get_snsotp_pid(handle, param_ptr->pval);
        break;
    default:
        break;
    }
    ret = SENSOR_SUCCESS;

    return ret;
}

static void get_back_cam_efuse_id(cmr_handle handle)
{
    cmr_u16 i = 0, j = 0;
    cmr_u16 startadd = 0;
    cmr_u16 efuse_id, temp;
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hi1336_drv_write_reg2sensor(handle, &hi1336_efuse_id_tab);

    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0B02, 0x01, BITS_ADDR16_REG8);//Fast standy on
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0809, 0x00, BITS_ADDR16_REG8);//Stream off
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0B00, 0x00, BITS_ADDR16_REG8);//Stream off
    usleep(30*1000);

    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0260, 0x10, BITS_ADDR16_REG8);//OTP test mode enable
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0809, 0x00, BITS_ADDR16_REG8);//Stream on
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0B00, 0x01, BITS_ADDR16_REG8);//Stream on
    usleep(10*1000);

    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x030a, 0x00, BITS_ADDR16_REG8);
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x030b, 0x01, BITS_ADDR16_REG8);
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0302, 0x01, BITS_ADDR16_REG8);//OTP read mode
    usleep(10*1000);

    for(i=0;i<9;i++)
    {
	    efuse_id = hw_sensor_grc_read_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0308, BITS_ADDR16_REG8);
        SENSOR_LOGI("get_back_cam_efuse_id[%d] = %x", i, efuse_id);

	    sprintf(back_cam_efuse_id+2*i,"%02x",efuse_id);
	    usleep(1*1000);
    }

    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0809, 0x00, BITS_ADDR16_REG8);     // Stream off
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0B00, 0x00, BITS_ADDR16_REG8);     // Stream off
    usleep(10*1000);

    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0260, 0x00, BITS_ADDR16_REG8);
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0809, 0x01, BITS_ADDR16_REG8);     // Stream on
    hw_sensor_grc_write_i2c(sns_drv_cxt->hw_handle, 0x40>>1, 0x0B00, 0x01, BITS_ADDR16_REG8);     // Stream on
    usleep(1*1000);

}

static cmr_int hi1336_drv_save_snspid(cmr_handle handle) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u8 snspid_size = 32;
    cmr_u8 snspid[32] = {0};

    SENSOR_LOGI("E");
    for (int i = 0; i < 18; i++) {
        snspid[i] = back_cam_efuse_id[i];
    }
    if (sns_drv_cxt->ops_cb.set_snspid) {
        sns_drv_cxt->ops_cb.set_snspid(
            sns_drv_cxt->caller_handle, sns_drv_cxt->sensor_id, snspid, snspid_size);
    }

    return SENSOR_SUCCESS;
}


/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi1336_drv_identify(cmr_handle handle, cmr_uint param) {
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_u16 mid_value = 0x00;
	uint8_t cmd_val[5] = { 0x00 };
    cmr_int ret_value = SENSOR_FAIL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("mipi raw identify");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, hi1336_PID_ADDR) >> 8;

    if (hi1336_PID_VALUE == pid_value) {
        ver_value =
            hw_sensor_read_reg(sns_drv_cxt->hw_handle, hi1336_VER_ADDR) >> 8;

        mid_value = hw_sensor_grc_read_i2c(sns_drv_cxt->hw_handle, 0xA0>>1, 0x0013, BITS_ADDR16_REG8);

        SENSOR_LOGI("Identify: pid_value = %x, ver_value = %x, mid_value = %x", pid_value,
                    ver_value, mid_value);

        if (hi1336_VER_VALUE == ver_value && 0x04 == mid_value) {
            if(!identify_success){
                sensor_rid_save_sensor_name(SENSOR_HWINFOR_BACK_CAM_NAME, "0_hi1336_arb_1");
                get_back_cam_efuse_id(handle);
                sensor_rid_save_sensor_name(SENSOR_HWINFOR_BACK_CAM_EFUSE, back_cam_efuse_id);
                hi1336_drv_save_snspid(handle);
                identify_success = true;
            }
            SENSOR_LOGI("this is hi1336_arb_1 sensor, module id: %x", mid_value);
            ret_value = SENSOR_SUCCESS;
        } else {
            SENSOR_LOGE("sensor identify fail, pid_value = %x, ver_value = %x",
                        pid_value, ver_value);
        }
    } else {
        SENSOR_LOGE("sensor identify fail, pid_value = %x, ver_value = %x",
                    pid_value, ver_value);
    }

    return ret_value;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int hi1336_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u32 cap_shutter = 0;
    cmr_u32 prv_shutter = 0;
    float prv_gain = 0.0;
    float cap_gain = 0.0;
    cmr_u32 capture_mode = param & 0xffff;
    cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
    cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

    SENSOR_LOGI("preview_mode=%d,capture_mode = %d", preview_mode,
                capture_mode);
    SENSOR_LOGI("preview_shutter = 0x%x, preview_gain = 0x%x",
                sns_drv_cxt->sensor_ev_info.preview_shutter,
                (unsigned int)sns_drv_cxt->sensor_ev_info.preview_gain);

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
    prv_gain = sns_drv_cxt->sensor_ev_info.preview_gain;

    cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;
    cap_gain = prv_gain;

    SENSOR_LOGI("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter,
                cap_gain);
    hi1336_drv_calc_exposure(handle, cap_shutter, 0, capture_mode, &hi1336_aec_info);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.frame_length);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.shutter);

    sns_drv_cxt->sensor_ev_info.preview_gain = cap_gain;
    hi1336_drv_write_gain(handle, &hi1336_aec_info, cap_gain);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.again);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.dgain);

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

/*==============================================================================
 * Description:
 * get the shutter from isp and write senosr shutter register
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int hi1336_drv_write_exposure(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 size_index = 0x00;

    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_HANDLE(ex);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    hi1336_drv_calc_exposure(handle, exposure_line, dummy_line, size_index,
                            &hi1336_aec_info);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.frame_length);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.shutter);

    return ret_value;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int hi1336_drv_write_gain_value(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    hi1336_drv_calc_gain(handle, param, &hi1336_aec_info);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.again);
    hi1336_drv_write_reg2sensor(handle, hi1336_aec_info.dgain);

    return ret_value;
}

static cmr_int hi1336_drv_read_aec_info(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    info->aec_i2c_info_out = &hi1336_aec_info;
    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    hi1336_drv_calc_exposure(handle, exposure_line, dummy_line, mode,
                            &hi1336_aec_info);
    hi1336_drv_calc_gain(handle, info->gain, &hi1336_aec_info);

    return ret_value;
}

static cmr_int hi1336_drv_set_master_FrameSync(cmr_handle handle,
                                              cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    /*TODO*/

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0250, 0x0100);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0254, 0x1c00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0256, 0x0000);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0258, 0x0001);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x025A, 0x0000);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x025C, 0x0000);

    /*END*/

    return SENSOR_SUCCESS;
}

/*static cmr_int hi1336_drv_set_slave_FrameSync(cmr_handle handle,
                                              cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0250, 0x0300);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0254, 0x0011);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0256, 0x0100);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0258, 0x0002);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x025A, 0x03E8);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x025C, 0x0002);

    return SENSOR_SUCCESS;
}*/

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi1336_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

#if 0 //defined(CONFIG_DUAL_MODULE)
    char value1[PROPERTY_VALUE_MAX];
    property_get("vendor.cam.hw.framesync.on", value1, "1");
    if (!strcmp(value1, "1")) {
        if (MODE_BOKEH == sns_drv_cxt->is_multi_mode) {
            //hi1336_drv_set_master_FrameSync(handle, param);
            hi1336_drv_set_slave_FrameSync(handle, param);
        }
    }
#endif

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0b00, 0x0100);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi1336_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    cmr_u32 value = 0;
    cmr_u16 sleep_time = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0b00);
    if (value != 0x0000) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0b00, 0x0000);
        if (!sns_drv_cxt->is_sensor_close) {
            sleep_time = (sns_drv_cxt->sensor_ev_info.preview_framelength *
                          sns_drv_cxt->line_time_def / 1000000) +
                         10;
            usleep(sleep_time * 1000);
            SENSOR_LOGI("stream_off delay_ms %d", sleep_time);
        }
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0b00, 0x0000);
    }
    sns_drv_cxt->is_sensor_close = 0;

    SENSOR_LOGI("X");
    return SENSOR_SUCCESS;
}

static cmr_int
hi1336_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
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
    //s_drv_cxt->line_time_def = PREVIEW_LINE_TIME;
    hi1336_drv_write_frame_length(sns_drv_cxt,&hi1336_aec_info,sns_drv_cxt->sensor_ev_info.preview_framelength);
    hi1336_drv_write_gain(sns_drv_cxt,&hi1336_aec_info,sns_drv_cxt->sensor_ev_info.preview_gain);
    hi1336_drv_write_shutter(sns_drv_cxt ,&hi1336_aec_info,sns_drv_cxt->sensor_ev_info.preview_shutter);

    hi1336_drv_write_frame_length(
        sns_drv_cxt, &hi1336_aec_info,
        sns_drv_cxt->sensor_ev_info.preview_framelength);
    hi1336_drv_write_gain(sns_drv_cxt, &hi1336_aec_info,
                          sns_drv_cxt->sensor_ev_info.preview_gain);
    hi1336_drv_write_shutter(sns_drv_cxt, &hi1336_aec_info,
                             sns_drv_cxt->sensor_ev_info.preview_shutter);

    sensor_ic_set_match_module_info(sns_drv_cxt,
                                    ARRAY_SIZE(s_hi1336_module_info_tab),
                                    s_hi1336_module_info_tab);
    sensor_ic_set_match_resolution_info(sns_drv_cxt,
                                        ARRAY_SIZE(s_hi1336_resolution_tab_raw),
                                        s_hi1336_resolution_tab_raw);
    sensor_ic_set_match_trim_info(sns_drv_cxt,
                                  ARRAY_SIZE(s_hi1336_resolution_trim_tab),
                                  s_hi1336_resolution_trim_tab);
    sensor_ic_set_match_static_info(
        sns_drv_cxt, ARRAY_SIZE(s_hi1336_static_info), s_hi1336_static_info);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(s_hi1336_mode_fps_info),
                                 s_hi1336_mode_fps_info);

    /*init exif info,this will be deleted in the future*/
    hi1336_drv_init_fps_info(sns_drv_cxt);

    /*add private here*/
    return ret;
}

static cmr_int hi1336_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int hi1336_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                          void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

void *sensor_ic_open_lib(void)
{
     return &g_hi1336_mipi_raw_info;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = hi1336_power_on,
 *============================================================================*/
static struct sensor_ic_ops s_hi1336_ops_tab = {
    .create_handle = hi1336_drv_handle_create,
    .delete_handle = hi1336_drv_handle_delete,
    .get_data = hi1336_drv_get_private_data,
    /*---------------------------------------*/
    .power = hi1336_drv_power_on,
    .identify = hi1336_drv_identify,
    .ex_write_exp = hi1336_drv_write_exposure,
    .write_gain_value = hi1336_drv_write_gain_value,

#if defined(CONFIG_DUAL_MODULE)
    .read_aec_info = hi1336_drv_read_aec_info,
#endif

    .ext_ops = {
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = hi1336_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = hi1336_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = hi1336_drv_stream_off,
            /* expand interface,if you want to add your sub cmd ,
             *  you can add it in enum {@SENSOR_IOCTL_VAL_TYPE}
             */
            [SENSOR_IOCTL_ACCESS_VAL].ops = hi1336_drv_access_val,
    }};
