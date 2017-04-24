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

#include "sensor_ov13855_mipi_raw.h"
/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
/* frame length*/
#define SNAPSHOT_FRAME_LENGTH 3214
#define PREVIEW_FRAME_LENGTH 3216

#define IMAGE_NORMAL_MIRROR
/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static uint32_t s_current_default_frame_length = PREVIEW_FRAME_LENGTH;
static struct sensor_ev_info_t s_sensor_ev_info = {
    PREVIEW_FRAME_LENGTH - FRAME_OFFSET, SENSOR_BASE_GAIN,
    PREVIEW_FRAME_LENGTH};

static uint32_t s_ov13855_sensor_close_flag = 0;
static uint32_t s_current_frame_length = 0;
static uint32_t s_current_default_line_time = 0;

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t ov13855_get_default_frame_length(SENSOR_HW_HANDLE handle,
                                                 uint32_t mode) {
    return s_ov13855_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13855_group_hold_on(SENSOR_HW_HANDLE handle) {
    SENSOR_PRINT("E");
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13855_group_hold_off(SENSOR_HW_HANDLE handle) {
    SENSOR_PRINT("E");
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov13855_read_gain(SENSOR_HW_HANDLE handle) {
    return s_sensor_ev_info.preview_gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13855_write_gain(SENSOR_HW_HANDLE handle, float gain) {
    float gain_a = gain;
    float gain_d = 0x400; // spec p70, X1 = 15bit

    if (SENSOR_MAX_GAIN < (uint16_t)gain_a) {
        gain_a = SENSOR_MAX_GAIN;
        gain_d = gain * 0x400 / gain_a;
        if ((uint16_t)gain_d > 0x2 * 0x400 - 1)
            gain_d = 0x2 * 0x400 - 1;
    }
    // Sensor_WriteReg(0x320a, 0x01);
    // group 1:all other registers( gain)
    Sensor_WriteReg(0x3208, 0x01);
    Sensor_WriteReg(0x3508, ((uint16_t)gain_a >> 8) & 0x1f);
    Sensor_WriteReg(0x3509, (uint16_t)gain_a & 0xff);
    Sensor_WriteReg(0x5100, ((uint16_t)gain_d >> 8) & 0x7f);
    Sensor_WriteReg(0x5101, (uint16_t)gain_d & 0xff);
    Sensor_WriteReg(0x5102, ((uint16_t)gain_d >> 8) & 0x7f);
    Sensor_WriteReg(0x5103, (uint16_t)gain_d & 0xff);
    Sensor_WriteReg(0x5104, ((uint16_t)gain_d >> 8) & 0x7f);
    Sensor_WriteReg(0x5105, (uint16_t)gain_d & 0xff);
    Sensor_WriteReg(0x3208, 0x11);
    Sensor_WriteReg(0x3208, 0xA1);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov13855_read_frame_length(SENSOR_HW_HANDLE handle) {
    uint32_t frame_len;

    frame_len = Sensor_ReadReg(0x380e) & 0xff;
    frame_len = frame_len << 8 | (Sensor_ReadReg(0x380f) & 0xff);
    s_sensor_ev_info.preview_framelength = frame_len;

    return s_sensor_ev_info.preview_framelength;
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13855_write_frame_length(SENSOR_HW_HANDLE handle,
                                       uint32_t frame_len) {
    Sensor_WriteReg(0x380e, (frame_len >> 8) & 0xff);
    Sensor_WriteReg(0x380f, frame_len & 0xff);
    s_sensor_ev_info.preview_framelength = frame_len;
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13855_read_shutter(SENSOR_HW_HANDLE handle) {
    uint32_t value = 0x00;
    uint8_t shutter_l = 0x00;
    uint8_t shutter_m = 0x00;
    uint8_t shutter_h = 0x00;

    shutter_l = Sensor_ReadReg(0x3502);
    // value=(shutter>>0x04)&0x0f;
    shutter_m = Sensor_ReadReg(0x3501);
    // value+=(shutter&0xff)<<0x04;
    shutter_h = Sensor_ReadReg(0x3500);
    // value+=(shutter&0x0f)<<0x0c;
    value = ((shutter_h & 0x0f) << 12) + (shutter_m << 4) +
            ((shutter_l >> 4) & 0x0f);
    s_sensor_ev_info.preview_shutter = value; // shutter;

    return s_sensor_ev_info.preview_shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov13855_write_shutter(SENSOR_HW_HANDLE handle, uint32_t shutter) {
    uint16_t value = 0x00;

    value = (shutter << 0x04) & 0xff;
    Sensor_WriteReg(0x3502, value);
    value = (shutter >> 0x04) & 0xff;
    Sensor_WriteReg(0x3501, value);
    value = (shutter >> 0x0c) & 0x0f;
    Sensor_WriteReg(0x3500, value);
    s_sensor_ev_info.preview_shutter = shutter;
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t ov13855_write_exposure_dummy(SENSOR_HW_HANDLE handle,
                                             uint32_t shutter,
                                             uint32_t dummy_line,
                                             uint16_t size_index) {
    uint32_t dest_fr_len = 0;
    uint32_t cur_fr_len = 0;
    uint32_t fr_len = s_current_default_frame_length;

    // ov13855_group_hold_on(handle);

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    s_current_frame_length = dest_fr_len;

    cur_fr_len = ov13855_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
        ov13855_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
    /* write shutter to sensor registers */
    s_sensor_ev_info.preview_shutter = shutter;

    ov13855_write_shutter(handle, shutter);
    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, shutter);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13855_power_on(SENSOR_HW_HANDLE handle, uint32_t power_on) {
    SENSOR_AVDD_VAL_E dvdd_val = g_ov13855_mipi_raw_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_ov13855_mipi_raw_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_ov13855_mipi_raw_info.iovdd_val;
    BOOLEAN power_down = g_ov13855_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_ov13855_mipi_raw_info.reset_pulse_level;

    if (SENSOR_TRUE == power_on) {
        Sensor_PowerDown(power_down);
        Sensor_SetResetLevel(reset_level);
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
                          SENSOR_AVDD_CLOSED);
        usleep(1 * 1000);
        Sensor_SetAvddVoltage(avdd_val);
        Sensor_SetDvddVoltage(dvdd_val);
        Sensor_SetIovddVoltage(iovdd_val);
        usleep(1 * 1000);
        Sensor_PowerDown(!power_down);
        Sensor_SetResetLevel(!reset_level);
        usleep(6 * 1000);
        Sensor_SetMCLK(EX_MCLK);

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
        Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
#else
        Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
#endif
    } else {
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
        Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
#endif

        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        usleep(500);
        Sensor_SetResetLevel(reset_level);
        Sensor_PowerDown(power_down);
        usleep(200);
        Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
        Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
        Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
    }
    SENSOR_PRINT("(1:on, 0:off): %d", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13855_init_mode_fps_info(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_PRINT("ov13855_init_mode_fps_info:E");
    if (!s_ov13855_mode_fps_info.is_init) {
        uint32_t i, modn, tempfps = 0;
        SENSOR_PRINT("ov13855_init_mode_fps_info:start init");
        for (i = 0; i < NUMBER_OF_ARRAY(s_ov13855_resolution_trim_tab); i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = s_ov13855_resolution_trim_tab[i].line_time *
                      s_ov13855_resolution_trim_tab[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                s_ov13855_mode_fps_info.sensor_mode_fps[i].max_fps = modn * 30;
                if (s_ov13855_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
                    s_ov13855_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
                    s_ov13855_mode_fps_info.sensor_mode_fps[i]
                        .high_fps_skip_num =
                        s_ov13855_mode_fps_info.sensor_mode_fps[i].max_fps / 30;
                }
                if (s_ov13855_mode_fps_info.sensor_mode_fps[i].max_fps >
                    s_ov13855_static_info.max_fps) {
                    s_ov13855_static_info.max_fps =
                        s_ov13855_mode_fps_info.sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                         tempfps, s_ov13855_resolution_trim_tab[i].frame_line,
                         s_ov13855_resolution_trim_tab[i].line_time);
            SENSOR_PRINT("mode %d,max_fps: %d ", i,
                         s_ov13855_mode_fps_info.sensor_mode_fps[i].max_fps);
            SENSOR_PRINT(
                "is_high_fps: %d,highfps_skip_num %d",
                s_ov13855_mode_fps_info.sensor_mode_fps[i].is_high_fps,
                s_ov13855_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
        }
        s_ov13855_mode_fps_info.is_init = 1;
    }
    SENSOR_PRINT("ov13855_init_mode_fps_info:X");
    return rtn;
}

static uint32_t ov13855_get_static_info(SENSOR_HW_HANDLE handle,
                                        uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info;
    uint32_t up = 0;
    uint32_t down = 0;
    // make sure we have get max fps of all settings.
    if (!s_ov13855_mode_fps_info.is_init) {
        ov13855_init_mode_fps_info(handle);
    }
    ex_info = (struct sensor_ex_info *)param;
    ex_info->f_num = s_ov13855_static_info.f_num;
    ex_info->focal_length = s_ov13855_static_info.focal_length;
    ex_info->max_fps = s_ov13855_static_info.max_fps;
    ex_info->max_adgain = s_ov13855_static_info.max_adgain;
    ex_info->ois_supported = s_ov13855_static_info.ois_supported;
    ex_info->pdaf_supported = s_ov13855_static_info.pdaf_supported;
    ex_info->exp_valid_frame_num = s_ov13855_static_info.exp_valid_frame_num;
    ex_info->clamp_level = s_ov13855_static_info.clamp_level;
    ex_info->adgain_valid_frame_num =
        s_ov13855_static_info.adgain_valid_frame_num;
    ex_info->preview_skip_num = g_ov13855_mipi_raw_info.preview_skip_num;
    ex_info->capture_skip_num = g_ov13855_mipi_raw_info.capture_skip_num;
    ex_info->name = (cmr_s8 *)g_ov13855_mipi_raw_info.name;
    ex_info->sensor_version_info =
        (cmr_s8 *)g_ov13855_mipi_raw_info.sensor_version_info;
    // vcm_dw9800_get_pose_dis(handle, &up, &down);
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    SENSOR_PRINT("f_num: %d", ex_info->f_num);
    SENSOR_PRINT("max_fps: %d", ex_info->max_fps);
    SENSOR_PRINT("max_adgain: %d", ex_info->max_adgain);
    SENSOR_PRINT("ois_supported: %d", ex_info->ois_supported);
    SENSOR_PRINT("pdaf_supported: %d", ex_info->pdaf_supported);
    SENSOR_PRINT("exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
    SENSOR_PRINT("clam_level: %d", ex_info->clamp_level);
    SENSOR_PRINT("adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
    SENSOR_PRINT("sensor name is: %s", ex_info->name);
    SENSOR_PRINT("sensor version info is: %s", ex_info->sensor_version_info);

    return rtn;
}

static uint32_t ov13855_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info;
    // make sure have inited fps of every sensor mode.
    if (!s_ov13855_mode_fps_info.is_init) {
        ov13855_init_mode_fps_info(handle);
    }
    fps_info = (SENSOR_MODE_FPS_T *)param;
    uint32_t sensor_mode = fps_info->mode;
    fps_info->max_fps =
        s_ov13855_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps =
        s_ov13855_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps =
        s_ov13855_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        s_ov13855_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_PRINT("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_PRINT("min_fps: %d", fps_info->min_fps);
    SENSOR_PRINT("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_PRINT("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return rtn;
}

static uint32_t ov13855_set_sensor_close_flag(SENSOR_HW_HANDLE handle) {
    uint32_t rtn = SENSOR_SUCCESS;

    s_ov13855_sensor_close_flag = 1;

    return rtn;
}

static const cmr_u16 ov13855_pd_is_right[] = {1,0,0,1};

static const cmr_u16 ov13855_pd_row[] = {2,6,10,14};

static const cmr_u16 ov13855_pd_col[] = {14,14,6,6};
static const struct pd_pos_info _ov13855_pd_pos_l[] = {
    {14, 6}, {6, 10},
};

static const struct pd_pos_info _ov13855_pd_pos_r[] = {
	{14, 2}, {6, 14},
};

static uint32_t ov13855_get_pdaf_info(SENSOR_HW_HANDLE handle,
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
    pd_pos_is_right_size = NUMBER_OF_ARRAY(ov13855_pd_is_right);
    pd_pos_row_size = NUMBER_OF_ARRAY(ov13855_pd_row);
    pd_pos_col_size = NUMBER_OF_ARRAY(ov13855_pd_col);
    if ((pd_pos_row_size != pd_pos_col_size) || (pd_pos_row_size != pd_pos_is_right_size) ||
	(pd_pos_is_right_size != pd_pos_col_size)){
        SENSOR_PRINT_ERR(
            "ov13855 pd_pos_row size,pd_pos_row size and pd_pos_is_right size are not match");
        return -1;
    }

    pdaf_info->pd_offset_x = 0;
    pdaf_info->pd_offset_y = 0;
    pdaf_info->pd_end_x = 4224;
    pdaf_info->pd_end_y = 3136;
    pdaf_info->pd_block_w = 1;
    pdaf_info->pd_block_h = 1;
    pdaf_info->pd_block_num_x = 264;
    pdaf_info->pd_block_num_y = 196;
    pdaf_info->pd_pos_size = pd_pos_is_right_size;
    pdaf_info->pd_is_right = (cmr_u16 *)ov13855_pd_is_right;
    pdaf_info->pd_pos_row = (cmr_u16 *)ov13855_pd_row;
    pdaf_info->pd_pos_col = (cmr_u16 *)ov13855_pd_col;


	cmr_u16 pd_pos_r_size = NUMBER_OF_ARRAY(_ov13855_pd_pos_r);
	cmr_u16 pd_pos_l_size = NUMBER_OF_ARRAY(_ov13855_pd_pos_l);

	if (pd_pos_r_size != pd_pos_l_size) {
			SENSOR_PRINT_ERR(
					"ov13855_pd_pos_r size not match ov13855_pd_pos_l");
			return -1;
	}
	pdaf_info->pd_pitch_x = 16;
	pdaf_info->pd_pitch_y = 16;
	pdaf_info->pd_density_x = 16;
	pdaf_info->pd_density_y = 8;
	pdaf_info->pd_block_num_x = 264;
	pdaf_info->pd_block_num_y = 196;
	pdaf_info->pd_pos_size = pd_pos_r_size;
	pdaf_info->pd_pos_r = (struct pd_pos_info *)_ov13855_pd_pos_r;
	pdaf_info->pd_pos_l = (struct pd_pos_info *)_ov13855_pd_pos_l;

    return rtn;
}


/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long ov13855_access_val(SENSOR_HW_HANDLE handle,
                                        unsigned long param) {
    uint32_t ret = SENSOR_SUCCESS;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    if (!param_ptr) {
        return ret;
    }
    SENSOR_PRINT("sensor ov13855: param_ptr->type=%x", param_ptr->type);
    switch (param_ptr->type) {
    case SENSOR_VAL_TYPE_INIT_OTP:
        // ret =ov13855_otp_init(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP:
        // ret =ov13855_otp_read(handle,param_ptr);
        break;
    case SENSOR_VAL_TYPE_PARSE_OTP:
        // ret = ov13855_parse_otp(handle, param_ptr);
        break;
    case SENSOR_VAL_TYPE_WRITE_OTP:
        // rtn = _ov13855_write_otp(handle, (uint32_t)param_ptr->pval);
        break;

    case SENSOR_VAL_TYPE_SHUTTER:
        //		*((uint32_t*)param_ptr->pval) =
        // ov13855_read_shutter(handle);
        break;
    case SENSOR_VAL_TYPE_READ_OTP_GAIN:
        //		*((uint32_t*)param_ptr->pval) =
        // ov13855_read_gain(handle);
        break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = ov13855_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = ov13855_get_fps_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
        ret = ov13855_set_sensor_close_flag(handle);
        break;
    case SENSOR_VAL_TYPE_GET_PDAF_INFO:
        ret = ov13855_get_pdaf_info(handle, param_ptr->pval);
        break;
    default:
        break;
    }
    ret = SENSOR_SUCCESS;

    return ret;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13855_identify(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint16_t pid_value = 0x00;
    uint16_t ver_value = 0x00;
    uint32_t ret_value = SENSOR_FAIL;

    SENSOR_PRINT("mipi raw identify");

    pid_value = Sensor_ReadReg(ov13855_PID_ADDR);

    if (ov13855_PID_VALUE == pid_value) {
        ver_value = Sensor_ReadReg(ov13855_VER_ADDR);
        SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (ov13855_VER_VALUE == ver_value) {
            SENSOR_PRINT_HIGH("this is ov13855 sensor");
            //	#ifdef FEATURE_OTP
            //	/*if read otp info failed or module id mismatched ,identify
            // failed ,return SENSOR_FAIL ,exit identify*/
            //	if(PNULL!=s_ov13855_raw_param_tab_ptr->identify_otp){
            //		SENSOR_PRINT("identify
            // module_id=0x%x",s_ov13855_raw_param_tab_ptr->param_id);
            //		//set default value
            //		memset(s_ov13855_otp_info_ptr, 0x00, sizeof(struct
            // otp_info_t));
            //		ret_value =
            // s_ov13855_raw_param_tab_ptr->identify_otp(handle,s_ov13855_otp_info_ptr);
            //		if(SENSOR_SUCCESS == ret_value ){
            //			SENSOR_PRINT("identify otp sucess!
            // module_id=0x%x,
            // module_name=%s",s_ov13855_raw_param_tab_ptr->param_id,MODULE_NAME);
            //		} else{
            //			SENSOR_PRINT("identify otp fail! exit
            // identify");
            //			return ret_value;
            //		}
            //	} else{
            //	SENSOR_PRINT("no identify_otp function!");
            //	}
            //	#endif
            //	dw9718s_init(handle,2);
            ov13855_init_mode_fps_info(handle);
            ret_value = SENSOR_SUCCESS;
        } else {
            SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value,
                              ver_value);
        }
    } else {
        SENSOR_PRINT_HIGH("sensor identify fail, pid_value = %x", pid_value);
    }

    return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long ov13855_get_resolution_trim_tab(SENSOR_HW_HANDLE handle,
                                                     uint32_t param) {
    return (unsigned long)s_ov13855_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov13855_before_snapshot(SENSOR_HW_HANDLE handle,
                                        uint32_t param) {
    uint32_t cap_shutter = 0;
    uint32_t prv_shutter = 0;
    uint32_t gain = 0;
    uint32_t cap_gain = 0;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;

    uint32_t prv_linetime =
        s_ov13855_resolution_trim_tab[preview_mode].line_time;
    uint32_t cap_linetime =
        s_ov13855_resolution_trim_tab[capture_mode].line_time;

    s_current_default_frame_length =
        ov13855_get_default_frame_length(handle, capture_mode);
    SENSOR_PRINT("lxf capture_mode = %d", capture_mode);

    if (preview_mode == capture_mode) {
        cap_shutter = s_sensor_ev_info.preview_shutter;
        cap_gain = s_sensor_ev_info.preview_gain;
        goto snapshot_info;
    }

    prv_shutter = s_sensor_ev_info.preview_shutter; // ov13855_read_shutter();
    gain = s_sensor_ev_info.preview_gain;           // ov13855_read_gain();

    Sensor_SetMode(capture_mode);
    Sensor_SetMode_WaitDone();

    cap_shutter = prv_shutter * prv_linetime / cap_linetime; //* BINNING_FACTOR;
                                                             /*
                                                                     while (gain >= (2 * SENSOR_BASE_GAIN)) {
                                                                             if (cap_shutter * 2 > s_current_default_frame_length)
                                                                                     break;
                                                                             cap_shutter = cap_shutter * 2;
                                                                             gain = gain / 2;
                                                                     }
                                                             */
    cap_shutter = ov13855_write_exposure_dummy(handle, cap_shutter, 0, 0);
    cap_gain = gain;
    ov13855_write_gain(handle, cap_gain);
    SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = %f",
                 s_sensor_ev_info.preview_shutter,
                 s_sensor_ev_info.preview_gain);

    SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter,
                 cap_gain);
snapshot_info:

    Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static uint32_t ov13855_write_exposure(SENSOR_HW_HANDLE handle,
                                       unsigned long param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t size_index = 0x00;
    uint16_t frame_interval = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    if (!param) {
        SENSOR_PRINT_ERR("param is NULL !!!");
        return ret_value;
    }

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

    frame_interval =
        (uint16_t)(((exposure_line + dummy_line) *
                    (s_ov13855_resolution_trim_tab[size_index].line_time)) /
                   1000000);
    SENSOR_PRINT(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        size_index, exposure_line, dummy_line, frame_interval);
    s_current_default_frame_length =
        ov13855_get_default_frame_length(handle, size_index);
    s_current_default_line_time =
        s_ov13855_resolution_trim_tab[size_index].line_time;

    ret_value = ov13855_write_exposure_dummy(handle, exposure_line, dummy_line,
                                             size_index);

    return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(SENSOR_HW_HANDLE handle, uint32_t param) {
    uint32_t real_gain = 0;

    real_gain = param;

    return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov13855_write_gain_value(SENSOR_HW_HANDLE handle,
                                         uint32_t param) {
    uint32_t ret_value = SENSOR_SUCCESS;
    float real_gain = 0;

    // real_gain = isp_to_real_gain(handle,param);
    // SENSOR_PRINT("param = %d", param);
    param = param < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : param;

    real_gain = (float)1.0f * param * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    SENSOR_PRINT("real_gain = %f", real_gain);

    s_sensor_ev_info.preview_gain = real_gain;
    ov13855_write_gain(handle, real_gain);

    return ret_value;
}

static struct sensor_reg_tag ov13855_shutter_reg[] = {
    {0x3502, 0}, {0x3501, 0}, {0x3500, 0},
};

static struct sensor_i2c_reg_tab ov13855_shutter_tab = {
    .settings = ov13855_shutter_reg, .size = ARRAY_SIZE(ov13855_shutter_reg),
};

static struct sensor_reg_tag ov13855_again_reg[] = {
    {0x3208, 0x01}, {0x3508, 0x00}, {0x3509, 0x00},
};

static struct sensor_i2c_reg_tab ov13855_again_tab = {
    .settings = ov13855_again_reg, .size = ARRAY_SIZE(ov13855_again_reg),
};

static struct sensor_reg_tag ov13855_dgain_reg[] = {
    {0x5100, 0}, {0x5101, 0}, {0x5102, 0},    {0x5103, 0},
    {0x5104, 0}, {0x5105, 0}, {0x3208, 0x11}, {0x3208, 0xA1},
};

struct sensor_i2c_reg_tab ov13855_dgain_tab = {
    .settings = ov13855_dgain_reg, .size = ARRAY_SIZE(ov13855_dgain_reg),
};

static struct sensor_reg_tag ov13855_frame_length_reg[] = {
    {0x380e, 0}, {0x380f, 0},
};

static struct sensor_i2c_reg_tab ov13855_frame_length_tab = {
    .settings = ov13855_frame_length_reg,
    .size = ARRAY_SIZE(ov13855_frame_length_reg),
};

static struct sensor_aec_i2c_tag ov13855_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_8BIT,
    .shutter = &ov13855_shutter_tab,
    .again = &ov13855_again_tab,
    .dgain = &ov13855_dgain_tab,
    .frame_length = &ov13855_frame_length_tab,
};

static uint16_t ov13855_calc_exposure(SENSOR_HW_HANDLE handle, uint32_t shutter,
                                      uint32_t dummy_line, uint16_t mode,
                                      struct sensor_aec_i2c_tag *aec_info) {
    uint32_t dest_fr_len = 0;
    uint32_t cur_fr_len = 0;
    uint32_t fr_len = s_current_default_frame_length;
    int32_t offset = 0;
    uint16_t value = 0x00;
    float fps = 0.0;
    float line_time = 0.0;

    dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
    dest_fr_len =
        ((shutter + dummy_line) > fr_len) ? (shutter + dummy_line) : fr_len;
    s_current_frame_length = dest_fr_len;

    cur_fr_len = ov13855_read_frame_length(handle);

    if (shutter < SENSOR_MIN_SHUTTER)
        shutter = SENSOR_MIN_SHUTTER;

    line_time = s_ov13855_resolution_trim_tab[mode].line_time;
    if (cur_fr_len > shutter) {
        fps = 1000000.0 / (cur_fr_len * line_time);
    } else {
        fps = 1000000.0 / ((shutter + dummy_line) * line_time);
    }
    SENSOR_PRINT("sync fps = %f", fps);
    aec_info->frame_length->settings[0].reg_value = (dest_fr_len >> 8) & 0xff;
    aec_info->frame_length->settings[1].reg_value = dest_fr_len & 0xff;
    value = (shutter << 0x04) & 0xff;
    aec_info->shutter->settings[0].reg_value = value;
    value = (shutter >> 0x04) & 0xff;
    aec_info->shutter->settings[1].reg_value = value;
    value = (shutter >> 0x0c) & 0x0f;
    aec_info->shutter->settings[2].reg_value = value;
    return shutter;
}

static void ov13855_calc_gain(float gain, struct sensor_aec_i2c_tag *aec_info) {
    float gain_a = gain;
    float gain_d = 0x400;

    if (SENSOR_MAX_GAIN < (uint16_t)gain_a) {

        gain_a = SENSOR_MAX_GAIN;
        gain_d = gain * 0x400 / gain_a;
        if ((uint16_t)gain_d > 0x4 * 0x400 - 1)
            gain_d = 0x4 * 0x400 - 1;
    }
    // group 1:all other registers( gain)
    // Sensor_WriteReg(0x3208, 0x01);

    aec_info->again->settings[1].reg_value = ((uint16_t)gain_a >> 8) & 0x1f;
    aec_info->again->settings[2].reg_value = (uint16_t)gain_a & 0xff;

    aec_info->dgain->settings[0].reg_value = ((uint16_t)gain_d >> 8) & 0x7f;
    aec_info->dgain->settings[1].reg_value = (uint16_t)gain_d & 0xff;
    aec_info->dgain->settings[2].reg_value = ((uint16_t)gain_d >> 8) & 0x7f;
    aec_info->dgain->settings[3].reg_value = (uint16_t)gain_d & 0xff;
    aec_info->dgain->settings[4].reg_value = ((uint16_t)gain_d >> 8) & 0x7f;
    aec_info->dgain->settings[5].reg_value = (uint16_t)gain_d & 0xff;
}

static unsigned long ov13855_read_aec_info(SENSOR_HW_HANDLE handle,
                                           unsigned long param) {
    unsigned long ret_value = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t mode = 0x00;
    float real_gain = 0;
    uint32_t gain = 0;
    uint16_t frame_interval = 0x00;

    info->aec_i2c_info_out = &ov13855_aec_info;

    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    frame_interval =
        (uint16_t)(((exposure_line + dummy_line) *
                    (s_ov13855_resolution_trim_tab[mode].line_time)) /
                   1000000);
    SENSOR_PRINT(
        "mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
        mode, exposure_line, dummy_line, frame_interval);
    s_current_default_frame_length =
        ov13855_get_default_frame_length(handle, mode);
    s_current_default_line_time = s_ov13855_resolution_trim_tab[mode].line_time;

    s_sensor_ev_info.preview_shutter = ov13855_calc_exposure(
        handle, exposure_line, dummy_line, mode, &ov13855_aec_info);

    gain = info->gain < SENSOR_BASE_GAIN ? SENSOR_BASE_GAIN : info->gain;
    real_gain = (float)info->gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN * 1.0;
    ov13855_calc_gain(real_gain, &ov13855_aec_info);
    return ret_value;
}
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t ov13855_write_af(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_PRINT("E");
    // return dw9718s_write_af(handle,param);
    return 0;
}
#endif

unsigned long _ov13855_SetMaster_FrameSync(SENSOR_HW_HANDLE handle,
                                           unsigned long param) {
    // Sensor_WriteReg(0x3028, 0x20);
    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13855_stream_on(SENSOR_HW_HANDLE handle, uint32_t param) {

#if 1
    char value1[PROPERTY_VALUE_MAX];
    property_get("debug.camera.test.mode", value1, "0");
    if (!strcmp(value1, "1")) {
        Sensor_WriteReg(0x4503, 0x80);
    }
#endif

    SENSOR_PRINT("E");
    //_ov13855_SetMaster_FrameSync(handle,param);

    Sensor_WriteReg(0x0100, 0x01);
    /*delay*/
    usleep(10 * 1000);

    return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13855_stream_off(SENSOR_HW_HANDLE handle, uint32_t param) {
    SENSOR_PRINT("E");
    unsigned char value;
    unsigned int sleep_time = 0, frame_time;

    value = Sensor_ReadReg(0x0100);
    if (value != 0x00) {
        Sensor_WriteReg(0x0100, 0x00);
        if (!s_ov13855_sensor_close_flag) {
            sleep_time = 50 * 1000;
            usleep(sleep_time);
        }
    } else {
        Sensor_WriteReg(0x0100, 0x00);
    }

    s_ov13855_sensor_close_flag = 0;
    SENSOR_LOGI("X sleep_time=%dus", sleep_time);
    return 0;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = ov13855_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_ov13855_ioctl_func_tab = {
    .power = ov13855_power_on,
    .identify = ov13855_identify,
    .get_trim = ov13855_get_resolution_trim_tab,
    .before_snapshort = ov13855_before_snapshot,
    .ex_write_exp = ov13855_write_exposure,
    .write_gain_value = ov13855_write_gain_value,
    //.set_focus = ov13855_ext_func,
    //.set_video_mode = ov13855_set_video_mode,
    .stream_on = ov13855_stream_on,
    .stream_off = ov13855_stream_off,

    //.af_enable  = ov13855_write_af,
    //.group_hold_on = ov13855_group_hold_on,
    //.group_hold_of = ov13855_group_hold_off,
    //	.read_aec_info = ov13855_read_aec_info,
    .cfg_otp = ov13855_access_val,
    //.set_motor_bestmode = dw9800_set_motor_bestmode,// set vcm best mode and
    // avoid damping
};
