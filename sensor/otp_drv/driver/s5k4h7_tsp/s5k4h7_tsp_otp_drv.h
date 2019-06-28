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
#define LOG_TAG "s5k4h7_tsp_otp"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "s5k4h7_tsp_golden_otp.h"
#include "cmr_sensor_info.h"

/*Module Vendor ID*/
#define MODULE_VENDOR_ID  0x08

/*I2C slave address setting*/
#define OTP_I2C_ADDR (0x20 >> 1)
#define SENSOR_I2C_ADDR (0x20 >> 1)

/*OTP space setting*/
#define OTP_START_ADDR 0x0a04
#define OTP_END_ADDR 0x0a37
#define OTP_LEN   (OTP_END_ADDR-OTP_START_ADDR+1)

/*module base info*/
#define MODULE_INFO_GROUP_FLAG_OFFSET  0  /*for Sensor OTP memory*/
#define MODULE_INFO_START_OFFSET 1
#define MODULE_INFO_SIZE 18
#define MODULE_INFO_CHECKSUM_OFFSET 19

/*AF*/
#define AF_INFO_GROUP_FLAG_OFFSET  0 /*for Sensor OTP memory*/
#define AF_INFO_START_OFFSET 0
#define AF_INFO_SIZE 0
#define AF_INFO_CHECKSUM_OFFSET 0

/*AWB*/
#define AWB_INFO_GROUP_FLAG_OFFSET  0 /*for Sensor OTP memory*/
#define AWB_DATA_SIZE 4
#define AWB_SECTION_NUM 1
#define AWB_INFO_START_OFFSET 1
#define AWB_INFO_SIZE 18
#define AWB_INFO_CHECKSUM_OFFSET 19 

#define AWB_GOLDEN_INFO_GROUP_FLAG_OFFSET  0x0009 /* for Sensor OTP memory */
#define AWB_GOLDEN_INFO_OFFSET 0x000a
#define AWB_GOLDEN_INFO_SIZE 4
#define AWB_GOLDEN_SECTION_NUM 1
#define AWB_GOLDEN_INFO_CHECKSUM 0x000d

/*Lens shading calibration*/
#define LSC_INFO_GROUP_FLAG_OFFSET  0x0000 /*for Sensor OTP memory*/
#define OPTICAL_INFO_OFFSET 0x0000
#define LSC_INFO_OFFSET 0x0000
#define LSC_INFO_CHANNEL_SIZE 442
#define LSC_INFO_CHECKSUM 0x0000

/*PDAF*/
#define PDAF_INFO_OFFSET 0x0000
#define PDAF_INFO_SIZE 0x0000
#define PDAF_INFO_CHECKSUM 0x0000

/*reserved data*/
#define RES_INFO_OFFSET 0x0000
#define RES_INFO_SIZE 0x0000
#define RES_INFO_CHECKSUM 0x0000
/**/

#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18
#define LSC_FORMAT_SIZE  (GAIN_WIDTH *GAIN_HEIGHT * 2 * 4 * 2 )/*include golden and random data*/

static cmr_int s5k4h7_tsp_otp_create(otp_drv_init_para_t *input_para, cmr_handle *sns_af_drv_handle);
static cmr_int s5k4h7_tsp_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int s5k4h7_tsp_otp_drv_read(cmr_handle otp_drv_handle,void *p_params);
static cmr_int s5k4h7_tsp_otp_drv_write(cmr_handle otp_drv_handle, void *p_params);
static cmr_int s5k4h7_tsp_otp_drv_parse(cmr_handle otp_drv_handle, void *p_params);
static cmr_int s5k4h7_tsp_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int s5k4h7_tsp_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd, void *p_params);

otp_drv_entry_t s5k4h7_tsp_drv_entry = {
	.otp_cfg =
		{
			.cali_items =
				{
					.is_self_cal = FALSE, /*TRUE: OC calibration on,FALSE: OC calibration off*/
					.is_awbc = TRUE,  /* TRUE: support awb calibration, FALSE: Not support awb calibration */
					.is_awbc_self_cal= TRUE,/*TRUE: Sensor side calibration, FALSE: ISP Side calibration*/
					.is_lsc = TRUE,   /* TRUE: support lsc calibration, FALSE: Not support lsc calibration */
					.is_lsc_self_cal= TRUE, /*TRUE: Sensor side calibration, FALSE: ISP Side calibration*/
					.is_pdafc = FALSE, /* TRUE: support pdaf calibration, FALSE: Not support pdaf calibration */
					.is_pdaf_self_cal= FALSE, /*TRUE: Sensor side calibration, FALSE: ISP Side calibration*/
					.is_afc = FALSE,
					.is_af_self_cal = FALSE,
					.is_dul_camc = FALSE, /* TRUE: support dual camera  calibration, FALSE: Not support dual camera  calibration */
				},
			.base_info_cfg =
				{
					/*decompression on otp driver or isp*/
					.is_lsc_drv_decompression = FALSE,
					/*otp data compressed format,
					should confirm with module fae*/
					.compress_flag = GAIN_ORIGIN_BITS,
					/*the width of the stream the sensor can output*/
					.image_width = 3264,
					/*the height of the stream the sensor can output*/
					.image_height = 2448,
					.grid_width = 23,
					.grid_height = 18,
					.gain_width = GAIN_WIDTH,
					.gain_height = GAIN_HEIGHT,
				},
		},
	.otp_ops =
		{
            .sensor_otp_create = s5k4h7_tsp_otp_create,
            .sensor_otp_delete = s5k4h7_tsp_otp_drv_delete,
            .sensor_otp_read = s5k4h7_tsp_otp_drv_read,
            .sensor_otp_write = s5k4h7_tsp_otp_drv_write,
            .sensor_otp_parse = s5k4h7_tsp_otp_drv_parse,
            .sensor_otp_calibration = s5k4h7_tsp_otp_drv_calibration,
            .sensor_otp_ioctl = s5k4h7_tsp_otp_drv_ioctl, /*expend*/
        },
};
