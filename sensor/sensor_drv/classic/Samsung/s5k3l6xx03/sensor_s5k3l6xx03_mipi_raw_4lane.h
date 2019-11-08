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

#ifndef _SENSOR_S5K3L6XX03_MIPI_RAW_H_
#define _SENSOR_S5K3L6XX03_MIPI_RAW_H_


#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"


//#define FEATURE_OTP

#define VENDOR_NUM 				1
#define SENSOR_NAME 		"s5k3l6xx03"
#define I2C_SLAVE_ADDR 		0x5A

#define s5k3l6xx03_PID_ADDR 		0x0000
#define s5k3l6xx03_PID_VALUE 		0x30c6
#define s5k3l6xx03_VER_ADDR 		0x0002
#define s5k3l6xx03_VER_VALUE 		0xb000

/* sensor parameters begin */

/* effective sensor output image size */
#define VIDEO_WIDTH				1920
#define VIDEO_HEIGHT			1080
#define PREVIEW_WIDTH 			2104
#define PREVIEW_HEIGHT 			1560
#define SNAPSHOT_WIDTH 			4208
#define SNAPSHOT_HEIGHT 		3120

/*Raw Trim parameters*/
#define VIDEO_TRIM_X			0
#define VIDEO_TRIM_Y			0
#define VIDEO_TRIM_W			VIDEO_WIDTH
#define VIDEO_TRIM_H			VIDEO_HEIGHT
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			PREVIEW_WIDTH
#define PREVIEW_TRIM_H			PREVIEW_HEIGHT
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			SNAPSHOT_WIDTH
#define SNAPSHOT_TRIM_H			SNAPSHOT_HEIGHT

/*Mipi output*/
#define LANE_NUM			4
#define RAW_BITS			10

#define VIDEO_MIPI_PER_LANE_BPS 		552    /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS 		568  /* 2*Mipi clk */
#define SNAPSHOT_MIPI_PER_LANE_BPS 		1200 /* 2*Mipi clk */

/*line time unit: 1ns*/
#define VIDEO_LINE_TIME		 	9660
#define PREVIEW_LINE_TIME 		10200
#define SNAPSHOT_LINE_TIME 		10200

/* frame length*/
#define VIDEO_FRAME_LENGTH 			1146
#define PREVIEW_FRAME_LENGTH 		3282
#define SNAPSHOT_FRAME_LENGTH 		3282

/* please ref your spec */
#define FRAME_OFFSET 			8
#define SENSOR_MAX_GAIN 		0x200
#define SENSOR_BASE_GAIN 		0x20
#define SENSOR_MIN_SHUTTER 		4

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR			1

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN			0x80

/* please don't change it */
#define EX_MCLK				24

/*==============================================================================
 * Description:
 * register setting
 *============================================================================*/

static const SENSOR_REG_T s5k3l6xx03_init_setting[] = {
    {0x3084, 0x1314}, 
    {0x3266, 0x0001},
    {0x3242, 0x2020}, 
    {0x306A, 0x2F4C}, 
    {0x306C, 0xCA01}, 
    {0x307A, 0x0D20}, 
    {0x309E, 0x002D}, 
    {0x3072, 0x0013}, 
    {0x3074, 0x0977}, 
    {0x3076, 0x9411}, 
    {0x3024, 0x0016}, 
    {0x3070, 0x3D00}, 
    {0x3002, 0x0E00}, 
    {0x3006, 0x1000}, 
    {0x300A, 0x0C00}, 
    {0x3010, 0x0400}, 
    {0x3018, 0xC500}, 
    {0x303A, 0x0204}, 
    {0x3452, 0x0001}, 
    {0x3454, 0x0001}, 
    {0x3456, 0x0001}, 
    {0x3458, 0x0001}, 
    {0x345a, 0x0002}, 
    {0x345C, 0x0014}, 
    {0x345E, 0x0002}, 
    {0x3460, 0x0014}, 
    {0x3464, 0x0006}, 
    {0x3466, 0x0012}, 
    {0x3468, 0x0012}, 
    {0x346A, 0x0012}, 
    {0x346C, 0x0012}, 
    {0x346E, 0x0012}, 
    {0x3470, 0x0012}, 
    {0x3472, 0x0008}, 
    {0x3474, 0x0004}, 
    {0x3476, 0x0044}, 
    {0x3478, 0x0004}, 
    {0x347A, 0x0044}, 
    {0x347E, 0x0006}, 
    {0x3480, 0x0010}, 
    {0x3482, 0x0010}, 
    {0x3484, 0x0010}, 
    {0x3486, 0x0010}, 
    {0x3488, 0x0010}, 
    {0x348A, 0x0010}, 
    {0x348E, 0x000C}, 
    {0x3490, 0x004C}, 
    {0x3492, 0x000C}, 
    {0x3494, 0x004C}, 
    {0x3496, 0x0020}, 
    {0x3498, 0x0006}, 
    {0x349A, 0x0008}, 
    {0x349C, 0x0008}, 
    {0x349E, 0x0008}, 
    {0x34A0, 0x0008}, 
    {0x34A2, 0x0008}, 
    {0x34A4, 0x0008}, 
    {0x34A8, 0x001A}, 
    {0x34AA, 0x002A}, 
    {0x34AC, 0x001A}, 
    {0x34AE, 0x002A}, 
    {0x34B0, 0x0080}, 
    {0x34B2, 0x0006}, 
    {0x32A2, 0x0000}, 
    {0x32A4, 0x0000}, 
    {0x32A6, 0x0000}, 
    {0x32A8, 0x0000}, 
    {0x3066, 0x7E00}, 
    {0x3004, 0x0800}, 
    {0x0A02, 0x3400}, 
};

static const SENSOR_REG_T s5k3l6xx03_preview_setting[] = {
    {0x0344, 0x0008}, 
    {0x0346, 0x0008}, 
    {0x0348, 0x1077}, 
    {0x034A, 0x0C37}, 
    {0x034C, 0x0838}, 
    {0x034E, 0x0618}, 
    {0x0900, 0x0122}, 
    {0x0380, 0x0001}, 
    {0x0382, 0x0001}, 
    {0x0384, 0x0001}, 
    {0x0386, 0x0003}, 
    {0x0114, 0x0330}, 
    {0x0110, 0x0002}, 
    {0x0136, 0x1800}, 
    {0x0304, 0x0004}, 
    {0x0306, 0x0078}, 
    {0x3C1E, 0x0000}, 
    {0x030C, 0x0003}, 
    {0x030E, 0x0047}, 
    {0x3C16, 0x0001}, 
    {0x0300, 0x0006}, 
    {0x0342, 0x1320}, 
    {0x0340, 0x0CD2},   //cbc-3260
    {0x38C4, 0x0004}, 
    {0x38D8, 0x0011}, 
    {0x38DA, 0x0005}, 
    {0x38DC, 0x0005}, 
    {0x38C2, 0x0005}, 
    {0x38C0, 0x0004}, 
    {0x38D6, 0x0004}, 
    {0x38D4, 0x0004}, 
    {0x38B0, 0x0007}, 
    {0x3932, 0x1000}, 
    {0x3938, 0x000C}, 
    {0x0820, 0x0238}, 
    {0x380C, 0x0049}, 
    {0x3064, 0xFFCF}, 
    {0x309C, 0x0640}, 
    {0x3090, 0x8000}, 
    {0x3238, 0x000B}, 
    {0x314A, 0x5F02}, 
    {0x3300, 0x0000}, 
    {0x3400, 0x0000}, 
    {0x3402, 0x4E46}, 
    {0x32B2, 0x0008}, 
    {0x32B4, 0x0008}, 
    {0x32B6, 0x0008}, 
    {0x32B8, 0x0008}, 
    {0x3C34, 0x0048}, 
    {0x3C36, 0x3000}, 
    {0x3C38, 0x0020}, 
    {0x393E, 0x4000}, 
    {0x303A, 0x0204}, 
    {0x3034, 0x4B01}, 
    {0x3036, 0x0029}, 
    {0x3032, 0x4800}, 
    {0x320E, 0x049E}, 
};

static const SENSOR_REG_T s5k3l6xx03_snapshot_setting[] = {
    {0x0344, 0x0008}, 
    {0x0346, 0x0008}, 
    {0x0348, 0x1077}, 
    {0x034A, 0x0C37}, 
    {0x034C, 0x1070}, 
    {0x034E, 0x0C30}, 
    {0x0900, 0x0000}, 
    {0x0380, 0x0001}, 
    {0x0382, 0x0001}, 
    {0x0384, 0x0001}, 
    {0x0386, 0x0001}, 
    {0x0114, 0x0330}, 
    {0x0110, 0x0002}, 
    {0x0136, 0x1800}, 
    {0x0304, 0x0004}, 
    {0x0306, 0x0078}, 
    {0x3C1E, 0x0000}, 
    {0x030C, 0x0003}, 
    {0x030E, 0x004B}, 
    {0x3C16, 0x0000}, 
    {0x0300, 0x0006}, 
    {0x0342, 0x1320}, 
    {0x0340, 0x0CD2},   //rumble_0x0CBC
    {0x38C4, 0x0009}, 
    {0x38D8, 0x002A}, 
    {0x38DA, 0x000A}, 
    {0x38DC, 0x000B}, 
    {0x38C2, 0x000A}, 
    {0x38C0, 0x000F}, 
    {0x38D6, 0x000A}, 
    {0x38D4, 0x0009}, 
    {0x38B0, 0x000F}, 
    {0x3932, 0x1800}, 
    {0x3938, 0x000C}, 
    {0x0820, 0x04b0}, 
    {0x380C, 0x0090}, 
    {0x3064, 0xFFCF}, 
    {0x309C, 0x0640}, 
    {0x3090, 0x8800}, 
    {0x3238, 0x000C}, 
    {0x314A, 0x5F00}, 
    {0x3300, 0x0000}, 
    {0x3400, 0x0000}, 
    {0x3402, 0x4E42},
    {0x32B2, 0x0006}, 
    {0x32B4, 0x0006}, 
    {0x32B6, 0x0006}, 
    {0x32B8, 0x0006}, 
    {0x3C34, 0x0048}, 
    {0x3C36, 0x3000}, 
    {0x3C38, 0x0020}, 
    {0x393E, 0x4000}, 
    {0x303A, 0x0204}, 
    {0x3034, 0x4B01}, 
    {0x3036, 0x0029}, 
    {0x3032, 0x4800}, 
    {0x320E, 0x049E}, 
};

static const SENSOR_REG_T s5k3l6xx03_video_setting[] = {
    {0x0344, 0x00C0}, {0x0346, 0x01E8}, {0x0348, 0x0FBF}, {0x034A, 0x0A57},
    {0x034C, 0x0780}, {0x034E, 0x0438}, {0x0900, 0x0122}, {0x0380, 0x0001},
    {0x0382, 0x0001}, {0x0384, 0x0001}, {0x0386, 0x0003}, {0x0114, 0x0330},
    {0x0110, 0x0002}, {0x0136, 0x1800}, {0x0304, 0x0004}, {0x0306, 0x0078},
    {0x3C1E, 0x0000}, {0x030C, 0x0003}, {0x030E, 0x0045}, {0x3C16, 0x0001},
    {0x0300, 0x0006}, {0x0342, 0x1220}, {0x0340, 0x047A}, {0x38C4, 0x0004},
    {0x38D8, 0x0010}, {0x38DA, 0x0005}, {0x38DC, 0x0005}, {0x38C2, 0x0005},
    {0x38C0, 0x0004}, {0x38D6, 0x0004}, {0x38D4, 0x0004}, {0x38B0, 0x0007},
    {0x3932, 0x1000}, {0x3938, 0x000C}, {0x0820, 0x0228}, {0x380C, 0x003B},
    {0x3064, 0xEFCF}, {0x309C, 0x0640}, {0x3090, 0x8000}, {0x3238, 0x000B},
    {0x314A, 0x5F02}, {0x3300, 0x0000}, {0x3400, 0x0000}, {0x3402, 0x4E46},
    {0x32B2, 0x0008}, {0x32B4, 0x0008}, {0x32B6, 0x0008}, {0x32B8, 0x0008},
    // 1920*1080 90fpsfps, 24Mclk,lane,4lanes
    {0x3C34, 0x0048}, {0x3C36, 0x3000}, {0x3C38, 0x0020}, {0x393E, 0x4000},

    {0x303A, 0x0202}, {0x3034, 0x4B00}, {0x3036, 0xF729}, {0x3032, 0x3500},
    {0x320E, 0x0480},
};

static struct sensor_res_tab_info s_s5k3l6xx03_resolution_tab_raw[VENDOR_NUM] = {
	{
      .module_id = MODULE_SUNNY,
      .reg_tab = {
        {ADDR_AND_LEN_OF_ARRAY(s5k3l6xx03_init_setting), PNULL, 0,
        .width = 0, .height = 0,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

		{ADDR_AND_LEN_OF_ARRAY(s5k3l6xx03_video_setting), PNULL, 0,
        .width = VIDEO_WIDTH, .height = VIDEO_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},
		
        {ADDR_AND_LEN_OF_ARRAY(s5k3l6xx03_preview_setting), PNULL, 0,
        .width = PREVIEW_WIDTH, .height = PREVIEW_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW},

        {ADDR_AND_LEN_OF_ARRAY(s5k3l6xx03_snapshot_setting), PNULL, 0,
        .width = SNAPSHOT_WIDTH, .height = SNAPSHOT_HEIGHT,
        .xclk_to_sensor = EX_MCLK, .image_format = SENSOR_IMAGE_FORMAT_RAW}
		}
	}

	/*If there are multiple modules,please add here*/
};

static SENSOR_TRIM_T s_s5k3l6xx03_resolution_trim_tab[VENDOR_NUM] = {
	{
     .module_id = MODULE_SUNNY,
     .trim_info = {
       {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	   
	   {.trim_start_x = VIDEO_TRIM_X, .trim_start_y = VIDEO_TRIM_Y,
        .trim_width = VIDEO_TRIM_W,   .trim_height = VIDEO_TRIM_H,
        .line_time = VIDEO_LINE_TIME, .bps_per_lane = VIDEO_MIPI_PER_LANE_BPS,
        .frame_line = VIDEO_FRAME_LENGTH,
        .scaler_trim = {.x = VIDEO_TRIM_X, .y = VIDEO_TRIM_Y, .w = VIDEO_TRIM_W, .h = VIDEO_TRIM_H}},
	   
	   {.trim_start_x = PREVIEW_TRIM_X, .trim_start_y = PREVIEW_TRIM_Y,
        .trim_width = PREVIEW_TRIM_W,   .trim_height = PREVIEW_TRIM_H,
        .line_time = PREVIEW_LINE_TIME, .bps_per_lane = PREVIEW_MIPI_PER_LANE_BPS,
        .frame_line = PREVIEW_FRAME_LENGTH,
        .scaler_trim = {.x = PREVIEW_TRIM_X, .y = PREVIEW_TRIM_Y, .w = PREVIEW_TRIM_W, .h = PREVIEW_TRIM_H}},
       
	   {
        .trim_start_x = SNAPSHOT_TRIM_X, .trim_start_y = SNAPSHOT_TRIM_Y,
        .trim_width = SNAPSHOT_TRIM_W,   .trim_height = SNAPSHOT_TRIM_H,
        .line_time = SNAPSHOT_LINE_TIME, .bps_per_lane = SNAPSHOT_MIPI_PER_LANE_BPS,
        .frame_line = SNAPSHOT_FRAME_LENGTH,
        .scaler_trim = {.x = SNAPSHOT_TRIM_X, .y = SNAPSHOT_TRIM_Y, .w = SNAPSHOT_TRIM_W, .h = SNAPSHOT_TRIM_H}},
		}
	}
    /*If there are multiple modules,please add here*/

};

static SENSOR_REG_T s5k3l6xx03_shutter_reg[] = {
    {0x0202, SENSOR_MIN_SHUTTER},
};

static struct sensor_i2c_reg_tab s5k3l6xx03_shutter_tab = {
    .settings = s5k3l6xx03_shutter_reg,
    .size = ARRAY_SIZE(s5k3l6xx03_shutter_reg),
};

static SENSOR_REG_T s5k3l6xx03_again_reg[] = {
    {0x0204, SENSOR_BASE_GAIN},
};

static struct sensor_i2c_reg_tab s5k3l6xx03_again_tab = {
    .settings = s5k3l6xx03_again_reg,
    .size = ARRAY_SIZE(s5k3l6xx03_again_reg),
};

static SENSOR_REG_T s5k3l6xx03_dgain_reg[] = {

};

static struct sensor_i2c_reg_tab s5k3l6xx03_dgain_tab = {
    .settings = s5k3l6xx03_dgain_reg,
    .size = ARRAY_SIZE(s5k3l6xx03_dgain_reg),
};

static SENSOR_REG_T s5k3l6xx03_frame_length_reg[] = {
    {0x0340, PREVIEW_FRAME_LENGTH},
};

static struct sensor_i2c_reg_tab s5k3l6xx03_frame_length_tab = {
    .settings = s5k3l6xx03_frame_length_reg,
    .size = ARRAY_SIZE(s5k3l6xx03_frame_length_reg),
};

static struct sensor_aec_i2c_tag s5k3l6xx03_aec_info = {
    .slave_addr = (I2C_SLAVE_ADDR >> 1),
    .addr_bits_type = SENSOR_I2C_REG_16BIT,
    .data_bits_type = SENSOR_I2C_VAL_16BIT,
    .shutter = &s5k3l6xx03_shutter_tab,
    .again = &s5k3l6xx03_again_tab,
    .dgain = &s5k3l6xx03_dgain_tab,
    .frame_length = &s5k3l6xx03_frame_length_tab,
};

static SENSOR_STATIC_INFO_T s_s5k3l6xx03_static_info[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .static_info = {
        .f_num = 200,
        .focal_length = 354,
        .max_fps = 30,
        .max_adgain = 8,
        .ois_supported = 0,
		.pdaf_supported = 3,
        .exp_valid_frame_num = 1,
        .clamp_level = 64,
        .adgain_valid_frame_num = 0,
        .fov_info = {{4.713f, 3.494f}, 3.774f}}
    }
    /*If there are multiple modules,please add here*/
};


static SENSOR_MODE_FPS_INFO_T s_s5k3l6xx03_mode_fps_info[VENDOR_NUM] = {
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
         {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}}
    }
    /*If there are multiple modules,please add here*/
};


static struct sensor_module_info s_s5k3l6xx03_module_info_tab[VENDOR_NUM] = {
    {.module_id = MODULE_SUNNY,
     .module_info = {
         .major_i2c_addr = I2C_SLAVE_ADDR >> 1,
         .minor_i2c_addr = I2C_SLAVE_ADDR >> 1,

         .reg_addr_value_bits = SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT |
                                SENSOR_I2C_FREQ_400,

         .avdd_val = SENSOR_AVDD_2800MV,
         .iovdd_val = SENSOR_AVDD_1800MV,
         .dvdd_val = SENSOR_AVDD_1200MV,

         .image_pattern = SENSOR_IMAGE_PATTERN_RAWRGB_GR,

         .preview_skip_num = 1,
         .capture_skip_num = 1,
         .flash_capture_skip_num = 6,
         .mipi_cap_skip_num = 0,
         .preview_deci_num = 0,
         .video_preview_deci_num = 0,

         .threshold_eb = 0,
         .threshold_mode = 0,
         .threshold_start = 0,
         .threshold_end = 0,

         .sensor_interface = {
              .type = SENSOR_INTERFACE_TYPE_CSI2,
              .bus_width = LANE_NUM,
              .pixel_width = RAW_BITS,
              .is_loose = 0,
          },
         .change_setting_skip_num = 1,
         .horizontal_view_angle = 65,
         .vertical_view_angle = 60
      }
    }

/*If there are multiple modules,please add here*/
};

static struct sensor_ic_ops s_s5k3l6xx03_ops_tab;
struct sensor_raw_info *s_s5k3l6xx03_mipi_raw_info_ptr = PNULL;


/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_s5k3l6xx03_mipi_raw_info = {
    .hw_signal_polarity = SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P |
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
    .power_down_level = SENSOR_LOW_LEVEL_PWDN,
    .identify_count = 1,
    .identify_code =
        {{ .reg_addr = s5k3l6xx03_PID_ADDR, .reg_value = s5k3l6xx03_PID_VALUE},
         { .reg_addr = s5k3l6xx03_VER_ADDR, .reg_value = s5k3l6xx03_VER_VALUE}},

    .source_width_max = SNAPSHOT_WIDTH,
    .source_height_max = SNAPSHOT_HEIGHT,
    .name = (cmr_s8 *)SENSOR_NAME,
    .image_format = SENSOR_IMAGE_FORMAT_RAW,

    .module_info_tab = s_s5k3l6xx03_module_info_tab,
    .module_info_tab_size = ARRAY_SIZE(s_s5k3l6xx03_module_info_tab),

    .resolution_tab_info_ptr = s_s5k3l6xx03_resolution_tab_raw,
    .sns_ops = &s_s5k3l6xx03_ops_tab,
    .raw_info_ptr = &s_s5k3l6xx03_mipi_raw_info_ptr,

    .video_tab_info_ptr = NULL,
    .sensor_version_info = (cmr_s8 *)"s5k3l6xx03_v1",
};

#endif
