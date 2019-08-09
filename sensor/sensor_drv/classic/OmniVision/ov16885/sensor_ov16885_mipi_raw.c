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

#define LOG_TAG "ov16885"

#include "sensor_ov16885_mipi_raw.h"
#include "otp_parser.h"

/*==============================================================================
 * Description:
 * write register value to sensor
 * please modify this function acording your spec
 *============================================================================*/

static void ov16885_drv_write_reg2sensor(cmr_handle handle,
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
static void ov16885_drv_write_gain(cmr_handle handle,
                                   struct sensor_aec_i2c_tag *aec_info,
                                   cmr_u32 gain) {
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (aec_info->again->size) {
        /*TODO*/

        aec_info->again->settings[1].reg_value = (gain >> 8) & 0x1f;
        aec_info->again->settings[2].reg_value = gain & 0xff;
        aec_info->again->settings[3].reg_value = (gain >> 8) & 0x1f;
        aec_info->again->settings[4].reg_value = gain & 0xff;

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
static void ov16885_drv_write_frame_length(cmr_handle handle,
                                           struct sensor_aec_i2c_tag *aec_info,
                                           cmr_u32 frame_len) {
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (aec_info->frame_length->size) {
        /*TODO*/

        aec_info->frame_length->settings[0].reg_value = (frame_len >> 8) & 0xff;
        aec_info->frame_length->settings[1].reg_value = frame_len & 0xff;

        /*END*/
    }
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers buffer
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov16885_drv_write_shutter(cmr_handle handle,
                                      struct sensor_aec_i2c_tag *aec_info,
                                      cmr_u32 shutter) {
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    if (aec_info->shutter->size) {
        /*TODO*/

        aec_info->shutter->settings[0].reg_value = (shutter >> 0x0c) & 0x0f;
        aec_info->shutter->settings[1].reg_value = (shutter >> 0x04) & 0xff;
        aec_info->shutter->settings[2].reg_value = (shutter << 0x04) & 0xff;

        /*END*/
    }
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static void ov16885_drv_calc_exposure(cmr_handle handle, cmr_u32 shutter,
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

    frame_interval = (uint16_t)(
        ((shutter + dummy_line) * sns_drv_cxt->line_time_def) / 1000000);
    SENSOR_LOGD("mode = %d, exposure_line = %d, dummy_line= %d, "
                "frame_interval= %d ms, fps = %f",
                mode, shutter, dummy_line, frame_interval, fps);

    if (dest_fr_len != cur_fr_len) {
        sns_drv_cxt->sensor_ev_info.preview_framelength = dest_fr_len;
        ov16885_drv_write_frame_length(handle, aec_info, dest_fr_len);
    }
    sns_drv_cxt->sensor_ev_info.preview_shutter = shutter;
    ov16885_drv_write_shutter(handle, aec_info, shutter);

    if (sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(
            sns_drv_cxt->caller_handle, SENSOR_EXIF_CTRL_EXPOSURETIME, shutter);
    }
}

static void ov16885_drv_calc_gain(cmr_handle handle, cmr_uint isp_gain,
                                  struct sensor_aec_i2c_tag *aec_info) {
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u32 sensor_gain = 0;

    sensor_gain = isp_gain < ISP_BASE_GAIN ? ISP_BASE_GAIN : isp_gain;
    sensor_gain = sensor_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    if (SENSOR_MAX_GAIN < sensor_gain)
        sensor_gain = SENSOR_MAX_GAIN;

    SENSOR_LOGD("isp_gain = 0x%x,sensor_gain=0x%x", (unsigned int)isp_gain,
                sensor_gain);

    sns_drv_cxt->sensor_ev_info.preview_gain = sensor_gain;
    ov16885_drv_write_gain(handle, aec_info, sensor_gain);
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov16885_drv_power_on(cmr_handle handle, cmr_uint power_on) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;
    cmr_uint ret = 0;
    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = g_ov16885_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_ov16885_mipi_raw_info.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);

        ret = usleep(10 * 1000);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);

        ret = usleep(10 * 1000);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        ret = usleep(5 * 1000);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, EX_MCLK);
        ret = usleep(1 * 1000);
        // hw_sensor_set_mipi_level(sns_drv_cxt->hw_handle, 0);
    } else {
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        ret = usleep(1 * 1000);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
    }

    SENSOR_LOGI("(1:on, 0:off): %lu %d", power_on, ret);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov16885_drv_init_fps_info(cmr_handle handle) {
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

static cmr_int ov16885_drv_get_static_info(cmr_handle handle, cmr_u32 *param) {
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
        ov16885_drv_init_fps_info(handle);
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
    ex_info->name = (cmr_s8 *)g_ov16885_mipi_raw_info.name;
    ex_info->sensor_version_info =
        (cmr_s8 *)g_ov16885_mipi_raw_info.sensor_version_info;
    memcpy(&ex_info->fov_info, &static_info->fov_info,
           sizeof(static_info->fov_info));
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info((cmr_s8 *)SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int ov16885_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        ov16885_drv_init_fps_info(handle);
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

#if 0
static cmr_int ov16885_drv_get_pdaf_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_pdaf_info *pdaf_info = NULL;
    SENSOR_IC_CHECK_PTR(param);
    cmr_u16 i = 0;
    cmr_u16 pd_pos_row_size = 0;
    cmr_u16 pd_pos_col_size = 0;
    cmr_u16 pd_pos_is_right_size = 0;

    SENSOR_PRINT_ERR("E\n");

    pdaf_info = (struct sensor_pdaf_info *)param;
    pd_pos_is_right_size = NUMBER_OF_ARRAY(ov16885_pd_is_right);
    pd_pos_row_size = NUMBER_OF_ARRAY(ov16885_pd_row);
    pd_pos_col_size = NUMBER_OF_ARRAY(ov16885_pd_col);
    if ((pd_pos_row_size != pd_pos_col_size) ||
        (pd_pos_row_size != pd_pos_is_right_size) ||
        (pd_pos_is_right_size != pd_pos_col_size)) {
        SENSOR_LOGE("pd_pos_row size and pd_pos_is_right size are not match");
        return SENSOR_FAIL;
    }

    pdaf_info->pd_offset_x = 24;
    pdaf_info->pd_offset_y = 24;
    pdaf_info->pd_end_x = 4184;
    pdaf_info->pd_end_y = 3096;
    pdaf_info->pd_block_w = 3;
    pdaf_info->pd_block_h = 3;
    pdaf_info->pd_block_num_x = 65;
    pdaf_info->pd_block_num_y = 48;
    pdaf_info->pd_is_right = (cmr_u16 *)ov16885_pd_is_right;
    pdaf_info->pd_pos_row = (cmr_u16 *)ov16885_pd_row;
    pdaf_info->pd_pos_col = (cmr_u16 *)ov16885_pd_col;
    // pdaf_info->vendor_type = SENSOR_VENDOR_XXX;
    pdaf_info->sns_orientation = 1; /*1: mirror+flip; 0: normal*/

    return rtn;
}
#endif

#if 1
static const cmr_u32 sns_4in1_mode[] = {0, 0, 0, 1};
static cmr_int ov16885_drv_get_4in1_info(cmr_handle handle, cmr_u32 *param) {
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_4in1_info *sn_4in1_info = NULL;
    SENSOR_IC_CHECK_PTR(param);

    SENSOR_LOGI("E\n");

    sn_4in1_info = (struct sensor_4in1_info *)param;
    sn_4in1_info->is_4in1_supported = 1;
    sn_4in1_info->limited_4in1_width = 2336;
    sn_4in1_info->limited_4in1_height = 1752;
    sn_4in1_info->sns_mode = sns_4in1_mode;

    return rtn;
}

#include "sprd_fcell.h"
#include "dlfcn.h"
#include <fcntl.h>

void *handlelib = NULL;
uint16 *pOutImage = NULL; // for mipi_raw to raw16
#define OTPDPC_MIRROR 0
#define OTPDPC_FLIP 0
static cmr_int ov16885_drv_ov4c_init(cmr_handle handle, cmr_u8 *param) {
    // #include "fcell.h"
    cmr_int rtn = SENSOR_SUCCESS;
// ov4c_init();
// unsigned short *xtalk_data, unsigned short *otp_data, int otpdpc_len;
#if 1
    int ret = E_OV_FCD_OK;
    int imgsize = IMG_WIDTH * IMG_HEIGHT * 2;
    ovfcell_init init;
    init.flag = 0;
    init.width = IMG_WIDTH;
    init.height = IMG_HEIGHT;
    // Step 1: Init Fcell Library
    init.xtalk_len = XTALK_LEN;
    const char *fcellXtalkFile = XTALK_DATA;
    struct otp_parser_section_with_version xtalk_data;
    struct otp_parser_section_with_version dpc_data;
    void *otp_parser_handle = NULL;

#if 0
   char pFcellXtalk[XTALK_LEN];
   FILE *fp1 = fopen(fcellXtalkFile, "rb");
   fread(pFcellXtalk, XTALK_LEN, 1, fp1);
   fclose(fp1);
   init.xtalk = pFcellXtalk;//xtalk_data;//pFcellXtalk;//TODO
#else
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u8 *param_ptr = (cmr_u8 *)param;
    // cmr_u8 param_ptr[0x1000] = {0x00};
    // hw_sensor_read_i2c(sns_drv_cxt->hw_handle, 0xa0 >> 1, param_ptr,
    //                    0x1000 << 16 | SENSOR_I2C_REG_16BIT);
    // for (int i = 0; i < 600; i++)
    //     param_ptr[i] = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x160f+i);
    // SENSOR_LOGI("xtalk: pid_value = %x, ver_value = %x",
    //             param_ptr[0x160f], param_ptr[0x160f+1]);

    cmr_int (*otp_parser_ptr)(
        void *raw_data, enum otp_parser_cmd cmd, cmr_uint eeprom_num,
        cmr_int camera_id, cmr_int raw_height, cmr_int raw_width, void *result);

    otp_parser_handle = dlopen("libcam_otp_parser.so", RTLD_NOW);
    if (otp_parser_handle == NULL) {
        char const *err_str = dlerror();
        SENSOR_LOGE("%s", err_str ? err_str : "unknown");
        return SENSOR_FAIL;
    } else {
        SENSOR_LOGV("dlopen libcam_otp_parser.so success");
    }

    otp_parser_ptr = dlsym(otp_parser_handle, "otp_parser");
    if (otp_parser_ptr == NULL) {
        dlclose(otp_parser_handle);
        SENSOR_LOGE(
            "load libcam_otp_parser.so: couldn't find symbol otp_parser");
        return SENSOR_FAIL;
    } else {
        SENSOR_LOGV("link libcam_otp_parser.so symbol success");
    }

    rtn = otp_parser_ptr(param_ptr, OTP_PARSER_SECTION_XTALK_4IN1,
                         OTP_EEPROM_SINGLE, sns_drv_cxt->sensor_id, IMG_HEIGHT,
                         IMG_WIDTH, &xtalk_data);
    if (rtn) {
        dlclose(otp_parser_handle);
        return SENSOR_FAIL;
    }
    rtn = otp_parser_ptr(param_ptr, OTP_PARSER_SECTION_DPC_4IN1,
                         OTP_EEPROM_SINGLE, sns_drv_cxt->sensor_id, IMG_HEIGHT,
                         IMG_WIDTH, &dpc_data);
    if (rtn) {
        dlclose(otp_parser_handle);
        return SENSOR_FAIL;
    }
    init.xtalk = xtalk_data.data_addr;
    init.otpdpc = dpc_data.data_addr;
    init.xtalk_len = xtalk_data.data_size;
    init.otpdpc_len = dpc_data.data_size;
    SENSOR_LOGD("xtalk(%p, %d), dpc(%p, %d)", init.xtalk, init.xtalk_len,
                init.otpdpc, init.otpdpc_len);

    dlclose(otp_parser_handle);
#endif

    init.ofst_xtalk[0] = XTALK_OFFSET_X;
    init.ofst_xtalk[1] = XTALK_OFFSET_Y;
    init.imgwin_xtalk[0] = XTALK_IMGWIDTH;
    init.imgwin_xtalk[1] = XTALK_IMGHEIGHT;
    if (XTALK_MIRROR)
        init.flag |= (E_OV_FCD_MIRROR_XTALK);
    else
        init.flag &= (~E_OV_FCD_MIRROR_XTALK);
    if (XTALK_FLIP)
        init.flag |= (E_OV_FCD_FLIP_XTALK);
    else
        init.flag &= (~E_OV_FCD_FLIP_XTALK);
// load_otpdpc
#if 0
   FILE *odf = fopen(OTPDPC_DATA, "rb");
   fseek(odf, 0, SEEK_END);
   init.otpdpc_len = ftell(odf);//otpdpc_len;//ftell(odf);//TODO
   char pFCellOtpdpc[init.otpdpc_len];
   fseek(odf, 0, SEEK_SET);
   fread(pFCellOtpdpc, init.otpdpc_len, 1, odf);
  fclose(odf);
   init.otpdpc = pFCellOtpdpc;//otp_data;//pFCellOtpdpc;//TODO
#endif
    if (OTPDPC_MIRROR)
        init.flag |= (E_OV_FCD_MIRROR_OTPDPC);
    else
        init.flag &= (~E_OV_FCD_MIRROR_OTPDPC);
    if (OTPDPC_FLIP)
        init.flag |= (E_OV_FCD_FLIP_OTPDPC);
    else
        init.flag &= (~E_OV_FCD_FLIP_OTPDPC);
    init.ofst_otpdpc[0] = OTPDPC_OFFSET_X;
    init.ofst_otpdpc[1] = OTPDPC_OFFSET_Y;

#endif
    handlelib = dlopen("libsprd_fcell.so", RTLD_NOW);
    if (handlelib == NULL) {
        char const *err_str = dlerror();
        SENSOR_LOGE("%s", err_str ? err_str : "unknown");
        return SENSOR_FAIL;
    }

    /* Get the address of the struct hal_module_info. */
    const char *sym = "ov4c_init";
    typedef int (*func_init)(ovfcell_init); // unsigned char *, unsigned char
                                            // *);
    func_init init_ov4c = (int *)dlsym(handlelib, sym);
    if (init_ov4c == NULL) {
        SENSOR_LOGE("load libsprd_fcell.so: couldn't find symbol %s", sym);
        return SENSOR_FAIL;
    } else {
        SENSOR_LOGI("link libsprd_fcell.so symbol success");
    }
    init_ov4c(init); //(unsigned char *)param, (unsigned char *)param);

    pOutImage = malloc(imgsize);
    return rtn;
}

static cmr_int ov16885_drv_ov4c_process(cmr_handle handle, cmr_u32 *param) {
    //      #include "fcell.h"
    cmr_int rtn = SENSOR_SUCCESS;

    struct frame_4in1_info *frame_4in1 = (struct frame_4in1_info *)param;
    cmr_uint imgsize = IMG_WIDTH * IMG_HEIGHT * 2;
    //    gettimeofday(&start, NULL);
    //    char *stdout;
    //    fprintf(stdout, "fcellprocess begin\n");
    const char *fcellImgFile = "/data/vendor/cameraserver/input.mipi_raw";
    const char *outImgFile = "/data/vendor/cameraserver/output1.raw";
    uint16 *pFcellImage = (uint16 *)frame_4in1->im_addr_in;
//  uint16 *pOutImage;
//    pFcellImage = (uint16 *)malloc(imgsize*1.25/2);
//     pOutImage = malloc(imgsize);
#if 0

        FILE *fp0 = fopen(fcellImgFile, "wb");
        if (fp0) {
           fwrite(pFcellImage, imgsize*1.25/2, 1, fp0);
            fclose(fp0);
            SENSOR_LOGI("write fcell image file %s pFcellImage %p\n", fcellImgFile,pFcellImage);
        } else {
            SENSOR_LOGI("Cannot open fcell image file %s\n", fcellImgFile);
            goto exit;
        }
#endif
    if (handlelib == NULL)
        return SENSOR_FAIL;
    const char *sym = "ov4c_process";
    typedef int (*func_proc)(unsigned short *, unsigned short *);
    func_proc proc_ov4c = (int *)dlsym(handlelib, sym);
    if (proc_ov4c == NULL) {
        SENSOR_LOGI("load: couldn't find symbol %s", sym);
        return SENSOR_FAIL;
    } else {
        SENSOR_LOGI("link symbol success");
    }

    if (pOutImage) {
        proc_ov4c((unsigned short *)pFcellImage, (unsigned short *)pOutImage);
        //  proc_ov4c((unsigned short *)img_addr, (unsigned short *)img_addr);
        SENSOR_LOGI("X %x %x", frame_4in1->im_addr_in, pOutImage[1]);
    }
    frame_4in1->im_addr_out = frame_4in1->im_addr_in;

#if 0
    fp0 = fopen(outImgFile, "wb");
    if (fp0) {
        fwrite(pOutImage, imgsize, 1, fp0);
        fclose(fp0);
    }
#if 1
    fp0 = fopen("/data/vendor/cameraserver/input-new.mipi_raw", "wb");
    if (fp0) {
        fwrite(pFcellImage, 1,imgsize * 5 /8, fp0);
        fclose(fp0);
    }
#endif
    param = (cmr_u32 *)pFcellImage;
exit:
//    free(pFcellImage);
#endif
    //    free(pOutImage);
    return rtn;
}
static cmr_int ov16885_drv_ov4c_deinit(cmr_handle handle, cmr_u32 *param) {
    //      #include "fcell.h"
    cmr_int rtn = SENSOR_SUCCESS;
    // ov4c_release();
    const char *sym = "ov4c_release";
    typedef int (*func_release)();
    func_release release_ov4c = NULL;

    if (handlelib) {
        release_ov4c = (int *)dlsym(handlelib, sym);
    }
    if (release_ov4c == NULL) {
        SENSOR_LOGI("load: couldn't find symbol %s", sym);
        if (pOutImage) {
            free(pOutImage);
            pOutImage = NULL;
        }
        return SENSOR_FAIL;
    } else {
        SENSOR_LOGI("link symbol success");
    }
    release_ov4c();
    if (handlelib) {
        dlclose(handlelib);
        handlelib = NULL;
    }

    if (pOutImage) {
        free(pOutImage);
        pOutImage = NULL;
    }

    return rtn;
}

#endif

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov16885_drv_access_val(cmr_handle handle, cmr_uint param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;

    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("sensor ov16885: param_ptr->type=%x", param_ptr->type);

    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = ov16885_drv_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = ov16885_drv_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        ret = sns_drv_cxt->is_sensor_close = 1;
        break;
#if 0
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        ret = ov16885_drv_get_pdaf_info(handle, param_ptr->pval);
        break;
#endif
#if 1
    case SENSOR_VAL_TYPE_GET_4IN1_INFO:
        ret = ov16885_drv_get_4in1_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_OTP_DATA:
        ret = ov16885_drv_ov4c_init(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_4IN1_PROC:
        ret = ov16885_drv_ov4c_process(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_4IN1_DEINIT:
        ret = ov16885_drv_ov4c_deinit(handle, param_ptr->pval);
        break;
#endif
    default:
        break;
    }

    return ret;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov16885_drv_identify(cmr_handle handle, cmr_uint param) {
    cmr_u16 pid_value = 0x00;
    cmr_u16 ver_value = 0x00;
    cmr_int ret_value = SENSOR_FAIL;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("mipi raw identify");

    pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov16885_PID_ADDR);

    if (ov16885_PID_VALUE == pid_value) {
        ver_value =
            hw_sensor_read_reg(sns_drv_cxt->hw_handle, ov16885_VER_ADDR);
        SENSOR_LOGI("Identify: pid_value = %x, ver_value = %x", pid_value,
                    ver_value);
        if (ov16885_VER_VALUE == ver_value) {
            SENSOR_LOGI("this is ov16885 sensor");
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
static cmr_int ov16885_drv_before_snapshot(cmr_handle handle, cmr_uint param) {
    cmr_u32 cap_shutter = 0;
    cmr_u32 prv_shutter = 0;
    cmr_u32 prv_gain = 0;
    cmr_u32 cap_gain = 0;
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

    ov16885_drv_calc_exposure(handle, cap_shutter, 0, capture_mode,
                              &ov16885_aec_info);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.frame_length);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.shutter);

    sns_drv_cxt->sensor_ev_info.preview_gain = cap_gain;
    ov16885_drv_write_gain(handle, &ov16885_aec_info, cap_gain);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.again);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.dgain);

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
static cmr_int ov16885_drv_write_exposure(cmr_handle handle, cmr_uint param) {
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

    ov16885_drv_calc_exposure(handle, exposure_line, dummy_line, size_index,
                              &ov16885_aec_info);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.frame_length);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.shutter);

    return ret_value;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int ov16885_drv_write_gain_value(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    ov16885_drv_calc_gain(handle, param, &ov16885_aec_info);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.again);
    ov16885_drv_write_reg2sensor(handle, ov16885_aec_info.dgain);

    return ret_value;
}

/*==============================================================================
 * Description:
 * read ae control info
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int ov16885_drv_read_aec_info(cmr_handle handle, cmr_uint param) {
    cmr_int ret_value = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(info);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    info->aec_i2c_info_out = &ov16885_aec_info;
    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    ov16885_drv_calc_exposure(handle, exposure_line, dummy_line, mode,
                              &ov16885_aec_info);
    ov16885_drv_calc_gain(handle, info->gain, &ov16885_aec_info);

    return ret_value;
}

static cmr_int ov16885_drv_set_master_FrameSync(cmr_handle handle,
                                                cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    /*TODO*/

    // hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3002, 0x40);

    /*END*/

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov16885_drv_stream_on(cmr_handle handle, cmr_uint param) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

#if 0 // defined(CONFIG_DUAL_MODULE)
	ov16885_drv_set_master_FrameSync(handle, param);
	//ov16885_drv_set_slave_FrameSync(handle, param);
#endif

    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x01);

    /*delay*/
    // usleep(50 * 1000);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int ov16885_drv_stream_off(cmr_handle handle, cmr_uint param) {
    SENSOR_LOGI("E");
    unsigned char value = 0;
    cmr_u16 sleep_time = 0, ret = 0;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x0100);
    if (value != 0x00) {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
        if (!sns_drv_cxt->is_sensor_close) {
            sleep_time = (sns_drv_cxt->sensor_ev_info.preview_framelength *
                          sns_drv_cxt->line_time_def / 1000000) +
                         10;
            ret = usleep(sleep_time * 1000);
            SENSOR_LOGI("stream_off delay_ms %d %d", sleep_time, ret);
        }
    } else {
        hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x0100, 0x00);
    }
    sns_drv_cxt->is_sensor_close = 0;

    SENSOR_LOGI("X");
    return SENSOR_SUCCESS;
}

static cmr_int
ov16885_drv_handle_create(struct sensor_ic_drv_init_para *init_param,
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
    sns_drv_cxt->line_time_def = PREVIEW_LINE_TIME;

    ov16885_drv_write_frame_length(
        sns_drv_cxt, &ov16885_aec_info,
        sns_drv_cxt->sensor_ev_info.preview_framelength);
    ov16885_drv_write_gain(sns_drv_cxt, &ov16885_aec_info,
                           sns_drv_cxt->sensor_ev_info.preview_gain);
    ov16885_drv_write_shutter(sns_drv_cxt, &ov16885_aec_info,
                              sns_drv_cxt->sensor_ev_info.preview_shutter);

    sensor_ic_set_match_module_info(sns_drv_cxt,
                                    ARRAY_SIZE(s_ov16885_module_info_tab),
                                    s_ov16885_module_info_tab);
    sensor_ic_set_match_resolution_info(
        sns_drv_cxt, ARRAY_SIZE(s_ov16885_resolution_tab_raw),
        s_ov16885_resolution_tab_raw);
    sensor_ic_set_match_trim_info(sns_drv_cxt,
                                  ARRAY_SIZE(s_ov16885_resolution_trim_tab),
                                  s_ov16885_resolution_trim_tab);
    sensor_ic_set_match_static_info(
        sns_drv_cxt, ARRAY_SIZE(s_ov16885_static_info), s_ov16885_static_info);
    sensor_ic_set_match_fps_info(sns_drv_cxt,
                                 ARRAY_SIZE(s_ov16885_mode_fps_info),
                                 s_ov16885_mode_fps_info);

    /*init exif info,this will be deleted in the future*/
    ov16885_drv_init_fps_info(sns_drv_cxt);

    /*add private here*/
    return ret;
}

static cmr_int ov16885_drv_handle_delete(cmr_handle handle, void *param) {
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    ov16885_drv_ov4c_deinit(sns_drv_cxt, &ret);
    ret = sensor_ic_drv_delete(handle, param);
    return ret;
}

static cmr_int ov16885_drv_get_private_data(cmr_handle handle, cmr_uint cmd,
                                            void **param) {
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle, cmd, param);
    return ret;
}

void *sensor_ic_open_lib(void)
{
     return &g_ov16885_mipi_raw_info;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = ov16885_power_on,
 *============================================================================*/
static struct sensor_ic_ops s_ov16885_ops_tab = {
    .create_handle = ov16885_drv_handle_create,
    .delete_handle = ov16885_drv_handle_delete,
    .get_data = ov16885_drv_get_private_data,
    /*---------------------------------------*/
    .power = ov16885_drv_power_on,
    .identify = ov16885_drv_identify,
    .ex_write_exp = ov16885_drv_write_exposure,
    .write_gain_value = ov16885_drv_write_gain_value,

#if defined(CONFIG_DUAL_MODULE)
    .read_aec_info = ov16885_drv_read_aec_info,
#endif

    .ext_ops = {
            [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = ov16885_drv_before_snapshot,
            [SENSOR_IOCTL_STREAM_ON].ops = ov16885_drv_stream_on,
            [SENSOR_IOCTL_STREAM_OFF].ops = ov16885_drv_stream_off,
            /* expand interface,if you want to add your sub cmd ,
             *  you can add it in enum {@SENSOR_IOCTL_VAL_TYPE}
             */
            [SENSOR_IOCTL_ACCESS_VAL].ops = ov16885_drv_access_val,
    }};
