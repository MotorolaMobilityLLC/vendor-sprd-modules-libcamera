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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "hi846_golden_otp.h"
#include "cmr_sensor_info.h"

#define AF_OTP_SUPPORT 0

/*Module Vendor ID*/
#define MODULE_VENDOR_ID 0x57

/*I2C slave address setting*/
#define OTP_I2C_ADDR (0x42 >> 1)
#define SENSOR_I2C_ADDR (0x42 >> 1)

/*OTP space setting*/
#define OTP_START_ADDR 0x0201
#define OTP_END_ADDR 0x0ccc
#define OTP_LEN (OTP_END_ADDR - OTP_START_ADDR + 1)
/*use to upload af otp data to platform*/
#define OTP_UPLOAD_LEN 0x000a

/*module base info*/
#define MODULE_INFO_GROUP_FLAG_OFFSET 0x00
#define MODULE_INFO_GROUP1_REG 0x202
#define MODULE_INFO_GROUP2_REG 0x213
#define MODULE_INFO_GROUP3_REG 0x224
#define MODULE_INFO_SIZE 0x09

#define MODULE_INFO_CHECKSUM_GROUP1_REG 0x212
#define MODULE_INFO_CHECKSUM_GROUP2_REG 0x223
#define MODULE_INFO_CHECKSUM_GROUP3_REG 0x234

/*LSC*/
#define LSC_INFO_GROUP_FLAG_OFFSET 0x34
#define LSC_INFO_GROUP1_REG 0x236
#define LSC_INFO_GROUP2_REG 0x599
#define LSC_INFO_GROUP3_REG 0x8fc
#define LSC_INFO_SIZE 0x35a

#define LSC_INFO_CHECKSUM_GROUP1_REG 0x598
#define LSC_INFO_CHECKSUM_GROUP2_REG 0x8fb
#define LSC_INFO_CHECKSUM_GROUP3_REG 0xc5e

/*AWB*/
#define AWB_INFO_GROUP_FLAG_OFFSET 0x0a5e
#define AWB_RATIO_GROUP1_REG 0xc60
#define AWB_RATIO_GROUP2_REG 0xc7e
#define AWB_RATIO_GROUP3_REG 0xc9c
#define AWB_RATIO_SIZE 0xc

#define AWB_INFO_CHECKSUM_GROUP1_REG 0xc7d
#define AWB_INFO_CHECKSUM_GROUP2_REG 0xc9b
#define AWB_INFO_CHECKSUM_GROUP3_REG 0xcb9

#if AF_OTP_SUPPORT
/*AF*/
#define AF_INFO_GROUP_FLAG_OFFSET 0xab9
#define AF_INFO_GROUP1_REG 0xcbb
#define AF_INFO_GROUP2_REG 0xcc1
#define AF_INFO_GROUP3_REG 0xcc7
#define AF_INFO_SIZE 0x05

#define AF_INFO_CHECKSUM_GROUP1_REG 0xcc0
#define AF_INFO_CHECKSUM_GROUP2_REG 0xcc6
#define AF_INFO_CHECKSUM_GROUP3_REG 0xccc
#endif

static cmr_int hi846_otp_create(otp_drv_init_para_t *input_para,
                                cmr_handle *sns_af_drv_handle);
static cmr_int hi846_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int hi846_otp_drv_read(cmr_handle otp_drv_handle, void *p_params);
static cmr_int hi846_otp_drv_write(cmr_handle otp_drv_handle, void *p_params);
static cmr_int hi846_otp_drv_parse(cmr_handle otp_drv_handle, void *p_params);
static cmr_int hi846_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int hi846_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                   void *p_params);

otp_drv_entry_t hi846_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = FALSE,     /*TRUE: OC calibration on,FALSE:
                                                 OC calibration off*/
                    .is_awbc = TRUE,          /* TRUE: support awb calibration,
                                                 FALSE: Not support awb calibration
                                                 */
                    .is_awbc_self_cal = TRUE, /*TRUE: Sensor side
                                                 calibration, FALSE: ISP
                                                 Side calibration*/
                    .is_lsc = TRUE, /* TRUE: support lsc calibration, FALSE: Not
                                       support lsc calibration */
                    .is_lsc_self_cal =
                        TRUE, /*TRUE: Sensor side calibration, FALSE: ISP
                                 Side calibration*/
                    .is_pdafc = FALSE, /* TRUE: support pdaf calibration,
                                          FALSE: Not support pdaf
                                          calibration */
                    .is_pdaf_self_cal = FALSE, /*TRUE: Sensor side
                                                  calibration, FALSE: ISP
                                                  Side calibration*/
                    .is_afc = TRUE,
                    .is_af_self_cal = FALSE,
                    .is_dul_camc =
                        FALSE, /* TRUE: support dual camera  calibration,
                                  FALSE: Not support dual camera
                                  calibration */
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
                    .gain_width = 23,
                    .gain_height = 18,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = hi846_otp_create,
            .sensor_otp_delete = hi846_otp_drv_delete,
            .sensor_otp_read = hi846_otp_drv_read,
            .sensor_otp_write = hi846_otp_drv_write,
            .sensor_otp_parse = hi846_otp_drv_parse,
            .sensor_otp_calibration = hi846_otp_drv_calibration,
            .sensor_otp_ioctl = hi846_otp_drv_ioctl, /*expend*/
        },
};
