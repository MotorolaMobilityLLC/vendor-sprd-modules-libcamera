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

#include "sensor_s5k5e8yx_mipi_raw.h"
/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
/* frame length*/
#define IMAGE_NORMAL_MIRROR
#define CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static uint32_t s_current_default_frame_length = PREVIEW_FRAME_LENGTH;
static struct hdr_info_t s_hdr_info;
static struct sensor_ev_info_t s_sensor_ev_info = {
    PREVIEW_FRAME_LENGTH - FRAME_OFFSET, SENSOR_BASE_GAIN,
    PREVIEW_FRAME_LENGTH};

static uint32_t s_s5k5e8yx_sensor_close_flag = 0;
static uint32_t s_current_frame_length = 0;
static uint32_t s_current_default_line_time = 0;

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t s5k5e8yx_get_default_frame_length(SENSOR_HW_HANDLE handle,
                                                  uint32_t mode) {
  return s_s5k5e8yx_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k5e8yx_init_mode_fps_info(SENSOR_HW_HANDLE handle) {
  uint32_t rtn = SENSOR_SUCCESS;
  SENSOR_PRINT("s5k5e8yx_init_mode_fps_info:E");
  if (!s_s5k5e8yx_mode_fps_info.is_init) {
    uint32_t i, modn, tempfps = 0;
    SENSOR_PRINT("s5k5e8yx_init_mode_fps_info:start init");
    for (i = 0; i < NUMBER_OF_ARRAY(s_s5k5e8yx_resolution_trim_tab); i++) {
      // max fps should be multiple of 30,it calulated from line_time and
      // frame_line
      tempfps = s_s5k5e8yx_resolution_trim_tab[i].line_time *
                s_s5k5e8yx_resolution_trim_tab[i].frame_line;
      if (0 != tempfps) {
        tempfps = 1000000000 / tempfps;
        modn = tempfps / 30;
        if (tempfps > modn * 30)
          modn++;
        s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].max_fps = modn * 30;
        if (s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
          s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
          s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num =
              s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].max_fps / 30;
        }
        if (s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].max_fps >
            s_s5k5e8yx_static_info.max_fps) {
          s_s5k5e8yx_static_info.max_fps =
              s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].max_fps;
        }
      }
      SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ", i, tempfps,
                   s_s5k5e8yx_resolution_trim_tab[i].frame_line,
                   s_s5k5e8yx_resolution_trim_tab[i].line_time);
      SENSOR_PRINT("mode %d,max_fps: %d ", i,
                   s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].max_fps);
      SENSOR_PRINT(
          "is_high_fps: %d,highfps_skip_num %d",
          s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].is_high_fps,
          s_s5k5e8yx_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
    }
    s_s5k5e8yx_mode_fps_info.is_init = 1;
  }
  SENSOR_PRINT("s5k5e8yx_init_mode_fps_info:X");
  return rtn;
}

static uint32_t s5k5e8yx_get_fps_info(SENSOR_HW_HANDLE handle,
                                      uint32_t *param) {
  uint32_t rtn = SENSOR_SUCCESS;
  SENSOR_MODE_FPS_T *fps_info;
  // make sure have inited fps of every sensor mode.
  if (!s_s5k5e8yx_mode_fps_info.is_init) {
    s5k5e8yx_init_mode_fps_info(handle);
  }
  fps_info = (SENSOR_MODE_FPS_T *)param;
  uint32_t sensor_mode = fps_info->mode;
  fps_info->max_fps =
      s_s5k5e8yx_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
  fps_info->min_fps =
      s_s5k5e8yx_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
  fps_info->is_high_fps =
      s_s5k5e8yx_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
  fps_info->high_fps_skip_num =
      s_s5k5e8yx_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
  SENSOR_PRINT("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
  SENSOR_PRINT("min_fps: %d", fps_info->min_fps);
  SENSOR_PRINT("is_high_fps: %d", fps_info->is_high_fps);
  SENSOR_PRINT("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

  return rtn;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k5e8yx_group_hold_on(SENSOR_HW_HANDLE handle) {
  SENSOR_PRINT("E");
  Sensor_WriteReg(0x104, 0x01);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k5e8yx_group_hold_off(SENSOR_HW_HANDLE handle) {
  SENSOR_PRINT("E");
  Sensor_WriteReg(0x104, 0x00);
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t s5k5e8yx_read_gain(SENSOR_HW_HANDLE handle) {
  uint16_t gain_h = 0;
  uint16_t gain_l = 0;

  gain_h = Sensor_ReadReg(0x204) & 0xff;
  gain_l = Sensor_ReadReg(0x205) & 0xff;

  return ((gain_h << 8) | gain_l);
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k5e8yx_write_gain(SENSOR_HW_HANDLE handle, uint32_t gain) {
  if (SENSOR_MAX_GAIN < gain)
    gain = SENSOR_MAX_GAIN;

  Sensor_WriteReg(0x204, (gain >> 8) & 0xff);
  Sensor_WriteReg(0x205, gain & 0xff);

  s5k5e8yx_group_hold_off(handle);
}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t s5k5e8yx_read_frame_length(SENSOR_HW_HANDLE handle) {
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
static void s5k5e8yx_write_frame_length(SENSOR_HW_HANDLE handle,
                                        uint32_t frame_len) {
  Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
  Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k5e8yx_read_shutter(SENSOR_HW_HANDLE handle) {
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
static void s5k5e8yx_write_shutter(SENSOR_HW_HANDLE handle, uint32_t shutter) {
  Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
  Sensor_WriteReg(0x0203, shutter & 0xff);
}

/*==============================================================================
 *  * Description:
 *   * write exposure to sensor registers and get current shutter
 *    * please pay attention to the frame length
 *     * please don't change this function if it's necessary
 *      *============================================================================*/
static uint16_t s5k5e8yx_update_exposure(SENSOR_HW_HANDLE handle,
                                         uint32_t shutter,
                                         uint32_t dummy_line) {
  uint32_t dest_fr_len = 0;
  uint32_t cur_fr_len = 0;
  uint32_t fr_len = s_current_default_frame_length;

  s5k5e8yx_group_hold_on(handle);

  if (1 == SUPPORT_AUTO_FRAME_LENGTH)
    goto write_sensor_shutter;

  dest_fr_len = ((shutter + dummy_line + FRAME_OFFSET) > fr_len)
                    ? (shutter + dummy_line + FRAME_OFFSET)
                    : fr_len;

  cur_fr_len = s5k5e8yx_read_frame_length(handle);

  if (shutter < SENSOR_MIN_SHUTTER)
    shutter = SENSOR_MIN_SHUTTER;

  if (dest_fr_len != cur_fr_len)
    s5k5e8yx_write_frame_length(handle, dest_fr_len);
write_sensor_shutter:
  /* write shutter to sensor registers */
  s5k5e8yx_write_shutter(handle, shutter);
  return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k5e8yx_power_on(SENSOR_HW_HANDLE handle, uint32_t power_on) {
  SENSOR_AVDD_VAL_E dvdd_val = g_s5k5e8yx_mipi_raw_info.dvdd_val;
  SENSOR_AVDD_VAL_E avdd_val = g_s5k5e8yx_mipi_raw_info.avdd_val;
  SENSOR_AVDD_VAL_E iovdd_val = g_s5k5e8yx_mipi_raw_info.iovdd_val;
  BOOLEAN power_down = g_s5k5e8yx_mipi_raw_info.power_down_level;
  BOOLEAN reset_level = g_s5k5e8yx_mipi_raw_info.reset_pulse_level;

  if (SENSOR_TRUE == power_on) {
    Sensor_PowerDown(power_down);
    // Open power
    Sensor_SetResetLevel(reset_level);
    usleep(10 * 1000);
    Sensor_SetAvddVoltage(avdd_val);
    Sensor_SetDvddVoltage(dvdd_val);
    Sensor_SetIovddVoltage(iovdd_val);
    usleep(10 * 1000);
    Sensor_PowerDown(!power_down);
    Sensor_SetResetLevel(!reset_level);
    usleep(10 * 1000);
    Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
// Sensor_SetMIPILevel(0);     // for bringup

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
    Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
    usleep(5 * 1000);
    dw9714_init(2);
#endif

  } else {

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
    dw9714_deinit(2);
    Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
#endif

    Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
    Sensor_SetResetLevel(reset_level);
    Sensor_PowerDown(power_down);
    Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
    Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
    Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
    // Sensor_SetMIPILevel(1);     // for bringup
  }
  SENSOR_PRINT("(1:on, 0:off): %d", power_on);
  return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k5e8yx_identify(SENSOR_HW_HANDLE handle, uint32_t param) {
  uint16_t pid_value = 0x00;
  uint16_t ver_value = 0x00;
  uint32_t ret_value = SENSOR_FAIL;

  SENSOR_PRINT("mipi raw identify");

  pid_value = Sensor_ReadReg(s5k5e8yx_PID_ADDR);

  if (s5k5e8yx_PID_VALUE == pid_value) {
    ver_value = Sensor_ReadReg(s5k5e8yx_VER_ADDR);
    SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
    if (s5k5e8yx_VER_VALUE == ver_value) {
      ret_value = SENSOR_SUCCESS;
      SENSOR_PRINT_HIGH("this is s5k5e8yx sensor,shinetech module");
    } else {
      SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
    }
  } else {
    SENSOR_PRINT_HIGH("identify fail, pid_value = %x", pid_value);
  }

  return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long s5k5e8yx_get_resolution_trim_tab(SENSOR_HW_HANDLE handle,
                                                      uint32_t param) {
  return (unsigned long)s_s5k5e8yx_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t s5k5e8yx_before_snapshot(SENSOR_HW_HANDLE handle,
                                         uint32_t param) {
  uint32_t cap_shutter = 0;
  uint32_t prv_shutter = 0;
  uint32_t gain = 0;
  uint32_t cap_gain = 0;
  uint32_t capture_mode = param & 0xffff;
  uint32_t preview_mode = (param >> 0x10) & 0xffff;

  uint32_t prv_linetime =
      s_s5k5e8yx_resolution_trim_tab[preview_mode].line_time;
  uint32_t cap_linetime =
      s_s5k5e8yx_resolution_trim_tab[capture_mode].line_time;

  s_current_default_frame_length =
      s5k5e8yx_get_default_frame_length(handle, capture_mode);
  SENSOR_PRINT("capture_mode = %d", capture_mode);

  if (preview_mode == capture_mode) {
    cap_shutter = s_sensor_ev_info.preview_shutter;
    cap_gain = s_sensor_ev_info.preview_gain;
    goto snapshot_info;
  }

  prv_shutter = s_sensor_ev_info.preview_shutter; // s5k5e8yx_read_shutter();
  gain = s_sensor_ev_info.preview_gain;           // s5k5e8yx_read_gain();

  Sensor_SetMode(capture_mode);
  Sensor_SetMode_WaitDone();

  cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

  while (gain >= (2 * SENSOR_BASE_GAIN)) {
    if (cap_shutter * 2 > s_current_default_frame_length)
      break;
    cap_shutter = cap_shutter * 2;
    gain = gain / 2;
  }

  cap_shutter = s5k5e8yx_update_exposure(handle, cap_shutter, 0);
  cap_gain = gain;
  s5k5e8yx_write_gain(handle, cap_gain);
  SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
               s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

  SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter,
               cap_gain);
snapshot_info:
  s_hdr_info.capture_shutter = cap_shutter; // s5k5e8yx_read_shutter();
  s_hdr_info.capture_gain = cap_gain;       // s5k5e8yx_read_gain();
                                            /* limit HDR capture min fps to 10;
                                             *    * MaxFrameTime = 1000000*0.1us;
                                             *       */
  s_hdr_info.capture_max_shutter = 1000000 / cap_linetime;

  return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static uint32_t s5k5e8yx_write_exposure(SENSOR_HW_HANDLE handle,
                                        uint32_t param) {
  uint32_t ret_value = SENSOR_SUCCESS;
  uint16_t exposure_line = 0x00;
  uint16_t dummy_line = 0x00;
  uint16_t mode = 0x00;

  exposure_line = param & 0xffff;
  dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
  mode = (param >> 0x1c) & 0x0f;

  SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode,
               exposure_line, dummy_line);
  s_current_default_frame_length =
      s5k5e8yx_get_default_frame_length(handle, mode);

  s_sensor_ev_info.preview_shutter =
      s5k5e8yx_update_exposure(handle, exposure_line, dummy_line);

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
static uint32_t s5k5e8yx_write_gain_value(SENSOR_HW_HANDLE handle,
                                          uint32_t param) {
  uint32_t ret_value = SENSOR_SUCCESS;
  uint32_t real_gain = 0;

  real_gain = isp_to_real_gain(handle, param);

  real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

  SENSOR_PRINT("real_gain = 0x%x", real_gain);

  s_sensor_ev_info.preview_gain = real_gain;
  s5k5e8yx_write_gain(handle, real_gain);

  return ret_value;
}

static uint32_t s5k5e8yx_get_static_info(SENSOR_HW_HANDLE handle,
                                         uint32_t *param) {
  uint32_t rtn = SENSOR_SUCCESS;
  struct sensor_ex_info *ex_info;
  uint32_t up = 0;
  uint32_t down = 0;
  // make sure we have get max fps of all settings.
  if (!s_s5k5e8yx_mode_fps_info.is_init) {
    s5k5e8yx_init_mode_fps_info(handle);
  }
  ex_info = (struct sensor_ex_info *)param;
  ex_info->f_num = s_s5k5e8yx_static_info.f_num;
  ex_info->focal_length = s_s5k5e8yx_static_info.focal_length;
  ex_info->max_fps = s_s5k5e8yx_static_info.max_fps;
  ex_info->max_adgain = s_s5k5e8yx_static_info.max_adgain;
  ex_info->ois_supported = s_s5k5e8yx_static_info.ois_supported;
  ex_info->pdaf_supported = s_s5k5e8yx_static_info.pdaf_supported;
  ex_info->exp_valid_frame_num = s_s5k5e8yx_static_info.exp_valid_frame_num;
  ex_info->clamp_level = s_s5k5e8yx_static_info.clamp_level;
  ex_info->adgain_valid_frame_num =
      s_s5k5e8yx_static_info.adgain_valid_frame_num;
  ex_info->preview_skip_num = g_s5k5e8yx_mipi_raw_info.preview_skip_num;
  ex_info->capture_skip_num = g_s5k5e8yx_mipi_raw_info.capture_skip_num;
  ex_info->name = (cmr_s8 *)g_s5k5e8yx_mipi_raw_info.name;
  ex_info->sensor_version_info =
      (cmr_s8 *)g_s5k5e8yx_mipi_raw_info.sensor_version_info;
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

static uint32_t s5k5e8yx_set_sensor_close_flag(SENSOR_HW_HANDLE handle) {
  uint32_t rtn = SENSOR_SUCCESS;

  s_s5k5e8yx_sensor_close_flag = 1;

  return rtn;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t s5k5e8yx_write_af(SENSOR_HW_HANDLE handle, uint32_t param) {
  SENSOR_PRINT("E");
  // return dw9718s_write_af(handle,param);
  return 0;
}
#endif

unsigned long _s5k5e8yx_SetMaster_FrameSync(SENSOR_HW_HANDLE handle,
                                            unsigned long param) {
  // Sensor_WriteReg(0x3028, 0x20);
  return 0;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k5e8yx_stream_on(SENSOR_HW_HANDLE handle, uint32_t param) {

  SENSOR_PRINT("E");
  Sensor_WriteReg(0x3c16, 0x00);
  Sensor_WriteReg(0x3C0D, 0x04);
  Sensor_WriteReg(0x0100, 0x01);
  // Sensor_WriteReg(0x3C22, 0x00);
  // Sensor_WriteReg(0x3C22, 0x00);
  usleep(500);
  Sensor_WriteReg(0x3C0D, 0x00);
  SENSOR_PRINT("gx 1");

  return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k5e8yx_stream_off(SENSOR_HW_HANDLE handle, uint32_t param) {

  SENSOR_PRINT("gx 1");
  Sensor_WriteReg(0x0100, 0x00);
  return 0;
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long s5k5e8yx_access_val(SENSOR_HW_HANDLE handle,
                                         unsigned long param) {
  uint32_t ret = SENSOR_SUCCESS;
  SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
  if (!param_ptr) {
    return ret;
  }
  SENSOR_PRINT("sensor s5k5e8yx: param_ptr->type=%x", param_ptr->type);
  switch (param_ptr->type) {
  case SENSOR_VAL_TYPE_INIT_OTP:
    // ret =s5k5e8yx_otp_init(handle);
    break;
  case SENSOR_VAL_TYPE_READ_OTP:
    // ret =s5k5e8yx_otp_read(handle,param_ptr);
    break;
  case SENSOR_VAL_TYPE_PARSE_OTP:
    // ret = s5k5e8yx_parse_otp(handle, param_ptr);
    break;
  case SENSOR_VAL_TYPE_WRITE_OTP:
    // rtn = _s5k5e8yx_write_otp(handle, (uint32_t)param_ptr->pval);
    break;

  case SENSOR_VAL_TYPE_SHUTTER:
    //       *((uint32_t*)param_ptr->pval) =
    // s5k5e8yx_read_shutter(handle);
    break;
  case SENSOR_VAL_TYPE_READ_OTP_GAIN:
    //       *((uint32_t*)param_ptr->pval) =
    // s5k5e8yx_read_gain(handle);
    break;
  case SENSOR_VAL_TYPE_GET_STATIC_INFO:
    ret = s5k5e8yx_get_static_info(handle, param_ptr->pval);
    break;
  case SENSOR_VAL_TYPE_GET_FPS_INFO:
    ret = s5k5e8yx_get_fps_info(handle, param_ptr->pval);
    break;
  case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
    // ret = s5k5e8yx_set_sensor_close_flag(handle);
    break;
  default:
    break;
  }
  ret = SENSOR_SUCCESS;

  return ret;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = s5k5e8yx_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_s5k5e8yx_ioctl_func_tab = {
    .power = s5k5e8yx_power_on,
    .identify = s5k5e8yx_identify,
    .get_trim = s5k5e8yx_get_resolution_trim_tab,
    .before_snapshort = s5k5e8yx_before_snapshot,
    .write_ae_value = s5k5e8yx_write_exposure,
    .write_gain_value = s5k5e8yx_write_gain_value,
    .stream_on = s5k5e8yx_stream_on,
    .stream_off = s5k5e8yx_stream_off,
    .cfg_otp = s5k5e8yx_access_val,
};
