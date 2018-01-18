/**
 *      Copyright (c) 2018 Spreadtrum Technologies, Inc.
 *      All Rights Reserved.
 *     Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "ov13855_sunny_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "ov13855_sunny_golden_otp.h"
#include "cmr_sensor_info.h"

#define GT24C64A_I2C_ADDR 0xA0 >> 1
#define GT24C64A_I2C_WR_ADDR 0x80 >> 1
#define GT24C64A_I2C_RD_ADDR 0xF0 >> 1
#define OTP_START_ADDR 0x0000
#define OTP_END_ADDR 0x0FFF

#define OTP_LEN 8192
#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18

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
#define AF_INFO_OFFSET_1V0 0x0051
#define AF_INFO_CHECKSUM_1V0 0x005B

/*AWB*/
#define AWB_INFO_OFFSET 0x0016
#define AWB_INFO_CHECKSUM 0x0022
#define AWB_INFO_SIZE 6
#define AWB_SECTION_NUM 1
#define AWB_INFO_OFFSET_1V0 0x005C
#define AWB_INFO_CHECKSUM_1V0 0x0069

/*LSC*/
#define OPTICAL_INFO_OFFSET 0x0023
#define LSC_INFO_OFFSET 0x0033
#define LSC_INFO_CHECKSUM 0x0B8B
#define LSC_INFO_CHANNEL_SIZE 726
#define LSC_INFO_OFFSET_1V0 0x006A
#define LSC_INFO_CHECKSUM_1V0 0x0BD8

/*PDAF*/
#define PDAF_INFO_OFFSET 0x0B8C
#define PDAF_INFO_CHECKSUM 0x0D0C
#define PDAF_INFO_OFFSET_1V0 0x0BD9
#define PDAF_INFO_CHECKSUM_1V0 0x0D5A

/*AE*/
#define AE_INFO_OFFSET 0x0D0D
#define AE_INFO_CHECKSUM 0x0D25
#define AE_INFO_OFFSET_1V0 0x0DDC
#define AE_INFO_CHECKSUM_1V0 0x0DF5

/*dualcamera data calibration*/
#define DUAL_INFO_OFFSET 0x0D26
#define VCM_INFO_OFFSET 0x0E0A
#define DUAL_INFO_CHECKSUM 0xE0C
#define DUAL_DATA_SIZE 230
#define VCM_DATA_SIZE 2
#define DUAL_INFO_OFFSET_1V0 0x0DF6
#define DUAL_INFO_CHECKSUM_1V0 0xEF6

/**/
#define TOTAL_CHECKSUM_OFFSET 0x0FFF

/*ARCSOFT*/
#define ARCSOFT_INFO_RESERVE_SIZE 255
#define ARCSOFT_INFO_OFFSET 0x1000
#define ARCSOFT_INFO_CHECKSUM 0x18FF

#define LSC_GRID_SIZE 96
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

static cmr_int _ov13855_sunny_section_checksum(cmr_u8 *buffer, cmr_uint offset,
                                               cmr_uint size,
                                               cmr_uint checksum_offset,
                                               enum otp_version_t otp_version);
static cmr_int _ov13855_sunny_buffer_init(cmr_handle otp_drv_handle);
static cmr_int _ov13855_sunny_parse_awb_data(cmr_handle otp_drv_handle);
static cmr_int _ov13855_sunny_parse_lsc_data(cmr_handle otp_drv_handle);
static cmr_int _ov13855_sunny_parse_af_data(cmr_handle otp_drv_handle);
static cmr_int _ov13855_sunny_parse_pdaf_data(cmr_handle otp_drv_handle);

static cmr_int _ov13855_sunny_awb_calibration(cmr_handle otp_drv_handle);
static cmr_int _ov13855_sunny_lsc_calibration(cmr_handle otp_drv_handle);
static cmr_int _ov13855_sunny_pdaf_calibration(cmr_handle otp_drv_handle);

static cmr_int ov13855_sunny_otp_drv_create(otp_drv_init_para_t *input_para,
                                            cmr_handle *sns_af_drv_handle);
static cmr_int ov13855_sunny_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int ov13855_sunny_otp_drv_read(cmr_handle otp_drv_handle,
                                          void *p_data);
static cmr_int ov13855_sunny_otp_drv_write(cmr_handle otp_drv_handle,
                                           void *p_data);
static cmr_int ov13855_sunny_otp_drv_parse(cmr_handle otp_drv_handle,
                                           void *params);
static cmr_int ov13855_sunny_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int ov13855_sunny_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                           cmr_uint cmd, void *params);

otp_drv_entry_t ov13855_sunny_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = FALSE,
                    .is_dul_camc = TRUE,
                    .is_awbc = TRUE,
                    .is_lsc = TRUE,
                    .is_pdafc = TRUE,
                },
            .base_info_cfg =
                {
                    .is_lsc_drv_decompression = FALSE,
                    .compress_flag = OTP_COMPRESSED_FLAG,
                    .full_img_width = 4224,
                    .full_img_height = 3136,
                    .lsc_otp_grid = 96,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = ov13855_sunny_otp_drv_create,
            .sensor_otp_delete = ov13855_sunny_otp_drv_delete,
            .sensor_otp_read = ov13855_sunny_otp_drv_read,
            .sensor_otp_write = ov13855_sunny_otp_drv_write,
            .sensor_otp_parse = ov13855_sunny_otp_drv_parse,
            .sensor_otp_calibration = ov13855_sunny_otp_drv_calibration,
            .sensor_otp_ioctl = ov13855_sunny_otp_drv_ioctl, /*expend*/
        },
};
