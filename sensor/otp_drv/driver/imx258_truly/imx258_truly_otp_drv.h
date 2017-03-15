/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "iwhale2_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "imx258_truly_golden_otp.h"
#include "cmr_sensor_info.h"

#define GT24C64A_I2C_ADDR 0xA0 >> 1
#define OTP_START_ADDR 0x0000
#define OTP_END_ADDR 0x0FFF

#define OTP_LEN 8192
#define GAIN_WIDTH 23
#define GAIN_HEIGHT 18

#define WB_DATA_SIZE 8 * 2 /*Don't forget golden wb data*/
#define AF_DATA_SIZE 6
#define LSC_SRC_CHANNEL_SIZE 656
#define LSC_CHANNEL_SIZE 876

/*Don't forget golden lsc otp data*/
#define FORMAT_DATA_LEN                                                        \
    WB_DATA_SIZE + AF_DATA_SIZE + GAIN_WIDTH *GAIN_WIDTH * 4 * 2 * 2
/*module base info*/
#define MODULE_INFO_OFFSET 0x0000
#define MODULE_INFO_END_OFFSET 0x0007

/**/
#define AWB_INFO_OFFSET 0x0010
#define AWB_INFO_END_OFFSET 0x0017
#define AWB_INFO_SIZE 8
#define AWB_SECTION_NUM 1
/**/
#define AF_INFO_OFFSET 0x06A0
#define AF_INFO_END_OFFSET 0x06A4
#define AF_INFO_CHECKSUM  0x06A5

/*Lens shading calibration*/
#define LSC_INFO_OFFSET 0x0020
#define LSC_INFO_END_OFFSET 0x0699
#define LSC_DATA_SIZE       1658

#define LSC_INFO_CHANNEL_SIZE 726
#define LSC_INFO_CHECKSUM 0x0b8b
/*PDAF*/
#define PDAF_INFO_OFFSET 0x0700
#define PDAF_INFO_END_OFFSET 0x0927
#define PDAF_INFO_CHECKSUM 0x0701
/*SPC*/
#define SPC_INFO_OFFSET 0x0A2B
#define SPC_INFO_END_OFFSET 0x0AA9
#define SPC_INFO_CHECKSUM 0x0AAA
/**/
#define TOTAL_CHECKSUM_OFFSET 0x0AAB
#define LSC_GRID_SIZE 726
#define LSC_FORMAT_SIZE                                                        \
    GAIN_WIDTH *GAIN_HEIGHT * 2 * 4 * 2 /*include golden and random data*/
#define OTP_COMPRESSED_FLAG OTP_COMPRESSED_14BITS

typedef struct {
    unsigned char factory_id;
    unsigned char moule_id;
    unsigned char cali_version;
    unsigned char year;
    unsigned char month;
    unsigned char day;
    unsigned char sensor_id;
    unsigned char lens_id;
    unsigned char vcm_id;
    unsigned char drvier_ic_id;
    unsigned char ir_bg_id;
    unsigned char fw_ver;
    unsigned char af_cali_dir;
} module_info_t;

static int _imx258_truly_section_checksum(unsigned char *buf, unsigned int first,
                                    unsigned int last, unsigned int position);
static int _imx258_truly_buffer_init(void *otp_drv_handle);
static int _imx258_truly_parse_awb_data(void *otp_drv_handle);
static int _imx258_truly_parse_lsc_data(void *otp_drv_handle);
static int _imx258_truly_parse_af_data(void *otp_drv_handle);
static int _imx258_truly_parse_pdaf_data(void *otp_drv_handle);

static int _imx258_truly_awb_calibration(void *otp_drv_handle);
static int _imx258_truly_lsc_calibration(void *otp_drv_handle);
static int _imx258_truly_pdaf_calibration(void *otp_drv_handle);

static int imx258_truly_otp_create(otp_drv_init_para_t *input_para,
                             cmr_handle* sns_af_drv_handle);
static int imx258_truly_otp_drv_delete(void *otp_drv_handle);
static int imx258_truly_otp_drv_read(void *otp_drv_handle, otp_params_t *p_data);
static int imx258_truly_otp_drv_write(void *otp_drv_handle, otp_params_t *p_data);
static int imx258_truly_otp_drv_parse(void *otp_drv_handle, void *P_params);
static int imx258_truly_otp_drv_calibration(void *otp_drv_handle);
static int imx258_truly_otp_drv_ioctl(otp_drv_cxt_t *otp_drv_handle, int cmd, void *params);

otp_drv_entry_t imx258_truly_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = FALSE,  /*1:calibration at sensor
                                             side,0:calibration at isp*/
                    .is_dul_camc = FALSE, /* support dual camera calibration */
                    .is_awbc = FALSE,   /* support write balance calibration */
                    .is_lsc = FALSE,    /* support lens shadding calibration */
                    .is_pdafc = FALSE, /* support pdaf calibration */
                },
            .base_info_cfg =
                {
                    /*decompression on otp driver or isp*/
                    .is_lsc_drv_decompression = FALSE,
                    /*otp data compressed format,
                      should confirm with module fae*/
                    .compress_flag = OTP_COMPRESSED_FLAG,
                    /*the width of the stream the sensor can output*/
                    .image_width = 4208,
                    /*the height of the stream the sensor can output*/
                    .image_height = 3120,
                    .grid_width = 23,
                    .grid_height = 18,
                    .gain_width = GAIN_WIDTH,
                    .gain_height = GAIN_HEIGHT,
                },
        },
    .otp_ops =
        {
            .sensor_otp_create = imx258_truly_otp_create,
            .sensor_otp_delete = imx258_truly_otp_drv_delete,
            .sensor_otp_read = imx258_truly_otp_drv_read,
            .sensor_otp_write = imx258_truly_otp_drv_write,
            .sensor_otp_parse = imx258_truly_otp_drv_parse,
            .sensor_otp_calibration = imx258_truly_otp_drv_calibration,
            .sensor_otp_ioctl = imx258_truly_otp_drv_ioctl, /*expend*/
        },
};
