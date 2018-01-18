/**
 *      Copyright (c) 2018 Spreadtrum Technologies, Inc.
 *      All Rights Reserved.
 *     Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "ov5675_sunny_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "ov5675_sunny_golden_otp.h"
#include "cmr_sensor_info.h"

#define GT24C64A_I2C_ADDR 0xA0 >> 1
#define OTP_START_ADDR 0x0000
#define OTP_END_ADDR 0x0FFF

#define OTP_LEN 8192
#define GAIN_WIDTH 22
#define GAIN_HEIGHT 17

#define WB_DATA_SIZE 8 * 2 /*Don't forget truly wb data*/
#define AF_DATA_SIZE 6
#define LSC_SRC_CHANNEL_SIZE 656
#define LSC_CHANNEL_SIZE 876

/*Don't forget truly lsc otp data*/
#define FORMAT_DATA_LEN                                                        \
    WB_DATA_SIZE + AF_DATA_SIZE + GAIN_WIDTH *GAIN_WIDTH * 4 * 2 * 2

/*module base info*/
#define MODULE_INFO_OFFSET 0x0000
#define MODULE_INFO_CHECKSUM 0x000F
#define MODULE_INFO_CHECKSUM_1V0 0x0050

/*AF*/
#define AF_INFO_OFFSET 0x0010
#define AF_INFO_CHECKSUM 0x0015
#define AF_INFO_OFFSET_1V0 0x1000
#define AF_INFO_CHECKSUM_1V0 0x100A

/*AWB*/
#define AWB_INFO_OFFSET 0x0016
#define AWB_INFO_CHECKSUM 0x0022
#define AWB_INFO_OFFSET_1V0 0x100B
#define AWB_INFO_CHECKSUM_1V0 0x1018

/*Lens shading calibration*/
#define OPTICAL_INFO_OFFSET 0x0023
#define LSC_INFO_OFFSET 0x0033
#define LSC_INFO_CHECKSUM 0xA73
#define LSC_INFO_CHANNEL_SIZE 656
#define LSC_INFO_OFFSET_1V0 0x1019
#define LSC_INFO_CHECKSUM_1V0 0x1A6F

/*AE*/
#define AE_INFO_OFFSET 0x0A74
#define AE_INFO_CHECKSUM 0x0A8C
#define AE_INFO_OFFSET_1V0 0x1A70
#define AE_INFO_CHECKSUM_1V0 0x1A89

/**/
#define TOTAL_CHECKSUM_OFFSET 0x0FFF

#define LSC_GRID_SIZE 64

#define LSC_FORMAT_SIZE                                                        \
    GAIN_WIDTH *GAIN_HEIGHT * 2 * 4 * 2 /*include truly and random data*/
#define OTP_COMPRESSED_FLAG OTP_COMPRESSED_14BITS

typedef struct {
    cmr_u16 calib_version;
    enum otp_version_t otp_version;
    cmr_u8 year;
    cmr_u8 month;
    cmr_u8 day;
} module_info_t;

static cmr_int _ov5675_sunny_section_checksum(cmr_u8 *buffer, cmr_uint offset,
                                              cmr_uint size,
                                              cmr_uint checksum_offset,
                                              enum otp_version_t otp_version);
static cmr_int _ov5675_sunny_buffer_init(cmr_handle otp_drv_handle);
static cmr_int _ov5675_sunny_parse_awb_data(cmr_handle otp_drv_handle);
static cmr_int _ov5675_sunny_parse_lsc_data(cmr_handle otp_drv_handle);
static cmr_int _ov5675_sunny_parse_af_data(cmr_handle otp_drv_handle);
static cmr_int _ov5675_sunny_parse_pdaf_data(cmr_handle otp_drv_handle);

static cmr_int _ov5675_sunny_awb_calibration(cmr_handle otp_drv_handle);
static cmr_int _ov5675_sunny_lsc_calibration(cmr_handle otp_drv_handle);
static cmr_int _ov5675_sunny_pdaf_calibration(cmr_handle otp_drv_handle);

static cmr_int ov5675_sunny_otp_drv_create(otp_drv_init_para_t *input_para,
                                           cmr_handle *sns_af_drv_handle);
static cmr_int ov5675_sunny_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int ov5675_sunny_otp_drv_read(cmr_handle otp_drv_handle,
                                         void *p_data);
static cmr_int ov5675_sunny_otp_drv_write(cmr_handle otp_drv_handle,
                                          void *p_data);
static cmr_int ov5675_sunny_otp_drv_parse(cmr_handle otp_drv_handle,
                                          void *params);
static cmr_int ov5675_sunny_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int ov5675_sunny_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                          cmr_uint cmd, void *params);

otp_drv_entry_t ov5675_sunny_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = FALSE,
                    .is_dul_camc = FALSE,
                    .is_awbc = TRUE,
                    .is_lsc = TRUE,
                    .is_pdafc = FALSE,
                },
            .base_info_cfg =
                {
                    .is_lsc_drv_decompression = FALSE,
                    .compress_flag = OTP_COMPRESSED_FLAG,
                    .full_img_width = 2592,
                    .full_img_height = 1944,
                    .lsc_otp_grid = 64,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = ov5675_sunny_otp_drv_create,
            .sensor_otp_delete = ov5675_sunny_otp_drv_delete,
            .sensor_otp_read = ov5675_sunny_otp_drv_read,
            .sensor_otp_write = ov5675_sunny_otp_drv_write,
            .sensor_otp_parse = ov5675_sunny_otp_drv_parse,
            .sensor_otp_calibration = ov5675_sunny_otp_drv_calibration,
            .sensor_otp_ioctl = ov5675_sunny_otp_drv_ioctl, /*expend*/
        },
};
