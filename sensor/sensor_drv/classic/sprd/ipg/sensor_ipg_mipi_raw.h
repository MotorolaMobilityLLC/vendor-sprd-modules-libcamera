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
#ifndef _SENSOR_IPG_MIPI_RAW_H_
#define _SENSOR_IPG_MIPI_RAW_H_

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                   \
    defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_ipg_raw_param_main.c"
#else
#endif

#define ipg_I2C_ADDR_W (0xee >> 1)
#define ipg_I2C_ADDR_R (0xee >> 1)
#define SENSOR_NAME "ipg_mipi_raw"

#define VENDOR_NUM 1
LOCAL const SENSOR_REG_T ipg_com_mipi_raw[] = {};

LOCAL const SENSOR_REG_T ipg_1280X960_mipi_raw[] = {};

LOCAL const SENSOR_REG_T ipg_3264X2448_mipi_raw[] = {};

LOCAL const SENSOR_REG_T ipg_2592X1944_mipi_raw[] = {};

LOCAL const SENSOR_REG_T ipg_4608x3456_mipi_raw[] = {};

static SENSOR_STATIC_INFO_T s_ipg_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {.f_num = 200,
                     .focal_length = 354,
                     .max_fps = 0,
                     .max_adgain = 15 * 2,
                     .ois_supported = 0,
                     .pdaf_supported = 0,

#ifdef CONFIG_CAMERA_PDAF_TYPE
                     .pdaf_supported = CONFIG_CAMERA_PDAF_TYPE,
#else
                     .pdaf_supported = 0,
#endif
                     .exp_valid_frame_num = 1,
                     .clamp_level = 64,
                     .adgain_valid_frame_num = 1,
                     .fov_info = {{4.614f, 3.444f}, 4.222f}}}
    /*If there are multiple modules,please add here*/
};

static SENSOR_MODE_FPS_INFO_T s_ipg_mode_fps_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     {.is_init = 0,
      {{SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
       {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
       {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
       {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
       {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
       {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
       {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
       {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
       {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}}
    /*If there are multiple modules,please add here*/
};

static struct sensor_res_tab_info s_ipg_resolution_Tab_RAW[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .reg_tab = {{ADDR_AND_LEN_OF_ARRAY(ipg_com_mipi_raw), PNULL, 0,
                  .width = 0, .height = 0, .xclk_to_sensor = 24,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},

				  {ADDR_AND_LEN_OF_ARRAY(ipg_1280X960_mipi_raw), PNULL, 0,
                  .width = 1280, .height = 960, .xclk_to_sensor = 24,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},
                  {ADDR_AND_LEN_OF_ARRAY(ipg_4608x3456_mipi_raw), PNULL, 0,
                  .width = 4608, .height = 3456, .xclk_to_sensor = 24,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW},
                  {ADDR_AND_LEN_OF_ARRAY(ipg_3264X2448_mipi_raw), PNULL, 0,
                  .width = 3264, .height = 2448, .xclk_to_sensor = 24,
                  .image_format = SENSOR_IMAGE_FORMAT_RAW}

     }}
    /*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_ipg_Resolution_Trim_Tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .trim_info = {{0, 0, 1280, 960, 0, 0, 0, {0, 0, 1280, 960}},
                   {.trim_start_x = 0,
                    .trim_start_y = 0,
                    .trim_width = 1280,
                    .trim_height = 960,
                    .line_time = 282,
                    .bps_per_lane = 56,
                    .frame_line = 1280*4,
                    .scaler_trim = {.x = 0, .y = 0, .w = 1280, .h = 960}},
                   {.trim_start_x = 0,
                    .trim_start_y = 0,
                    .trim_width = 4608,
                    .trim_height = 3456,
                    .line_time = 282,
                    .bps_per_lane = 56,
                    .frame_line = 4608*4,
                    .scaler_trim = {.x = 0, .y = 0, .w = 4608, .h = 3456}},
                    {.trim_start_x = 0,
                    .trim_start_y = 0,
                    .trim_width = 3264,
                    .trim_height = 2448,
                    .line_time = 282,
                    .bps_per_lane = 56,
                    .frame_line = 3264*4,
                    .scaler_trim = {.x = 0, .y = 0, .w = 3264, .h = 2448}}}}
    /*If there are multiple modules,please add here*/
};

static struct sensor_module_info s_ipg_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {.major_i2c_addr = 0xee,
                     .minor_i2c_addr = 0xee,

                     .reg_addr_value_bits =
                         SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_CUSTOM,

                     .avdd_val = SENSOR_AVDD_2800MV,
                     .iovdd_val = SENSOR_AVDD_1800MV,
                     .dvdd_val = SENSOR_AVDD_1500MV,

                     .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GB,

                     .preview_skip_num = 3,
                     .capture_skip_num = 3,
                     .flash_capture_skip_num = 6,
                     .mipi_cap_skip_num = 0,
                     .preview_deci_num = 0,
                     .video_preview_deci_num = 0,

                     .sensor_interface = {.type = SENSOR_INTERFACE_TYPE_CSI2,
                                          .bus_width = 2,
                                          .pixel_width = 10,
											// .is_cphy = 1,
                                          .lane_switch_eb = 1,
                                          .lane_seq = 0xeeee, // mipi pn swap all, others:// ipmode
                                          .is_loose = 0},

                     .change_setting_skip_num = 3,
                     .horizontal_view_angle = 48,
                     .vertical_view_angle = 48}}

    /*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_ipg_ops_tab;
struct sensor_raw_info *s_ipg_mipi_raw_info_ptr = PNULL;

SENSOR_INFO_T g_ipg_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N |
                          SENSOR_HW_SIGNAL_HSYNC_P,
    .environment_mode = SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

    .image_effect = SENSOR_IMAGE_EFFECT_NORMAL |
                    SENSOR_IMAGE_EFFECT_BLACKWHITE | SENSOR_IMAGE_EFFECT_RED |
                    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE |
                    SENSOR_IMAGE_EFFECT_YELLOW | SENSOR_IMAGE_EFFECT_NEGATIVE |
                    SENSOR_IMAGE_EFFECT_CANVAS,

    .wb_mode = 0,
    .step_count = 7,

    .reset_pulse_level = SENSOR_LOW_PULSE_RESET,
    .reset_pulse_width = 50,
    .power_down_level = SENSOR_HIGH_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code = {{0x0A, 0x56}, {0x0B, 0x40}},

    .source_width_max = 4608,
    .source_height_max = 3456,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,
    //.image_format = SENSOR_IMAGE_FORMAT_YUV420,

    .resolution_tab_info_ptr = s_ipg_resolution_Tab_RAW,
    .sns_ops = &s_ipg_ops_tab,
    .raw_info_ptr = &s_ipg_mipi_raw_info_ptr,
    .ext_info_ptr = NULL,
    .module_info_tab = s_ipg_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_ipg_module_info_tab),

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"ipgv1"};
#endif

