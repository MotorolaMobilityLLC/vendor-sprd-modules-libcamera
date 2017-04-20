/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#define LOG_TAG "isharkl2_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "ov13855_sunny_golden_otp.h"
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
#define MODULE_INFO_END_OFFSET 0x000F
/*AWB*/
#define AWB_INFO_OFFSET 0x0016
#define AWB_INFO_END_OFFSET 0x0022
#define AWB_INFO_CHECKSUM 0x0022
#define AWB_INFO_SIZE 6
#define AWB_SECTION_NUM 2 ////////////////////
/*AF*/
#define AF_INFO_OFFSET 0x0010
#define AF_INFO_END_OFFSET 0x0015
#define AF_INFO_CHECKSUM 0x0015 //////////////////////
/*AE*/
#define AE_INFO_OFFSET 0x0D0D
#define AE_INFO_END_OFFSET 0x0D25
#define AE_INFO_CHECKSUM 0x0D25
/*Lens shading calibration*/

#define OPTICAL_INFO_OFFSET 0x0023
#define LSC_INFO_OFFSET 0x0033
#define LSC_INFO_END_OFFSET 0x0B8B
#define LSC_INFO_CHANNEL_SIZE 726
#define LSC_INFO_CHECKSUM 0x0B8B //////////////////////
#define LSC_DATA_SIZE 2920

/*dualcamera data calibration*/
#define DUAL_INFO_OFFSET 0x0D26
#define DUAL_INFO_END_OFFSET 0xE0A
#define DUAL_INFO_CHECKSUM 0xE0A
#define DUAL_DATA_SIZE 228

/*PDAF*/
#define PDAF_LEVEL_INFO_OFFSET 0x0B8C
#define PDAF_PHASE_INFO_OFFSET 0x0C8A
#define PDAF_INFO_END_OFFSET 0x0D0C
#define PDAF_INFO_CHECKSUM 0x0D0C ////////////////////
#define PDAF_LEVEL_DATA_SIZE 126
#define PDAF_PHASE_DATA_SIZE 128
/*reserve*/
#define RES_INFO_OFFSET 0x0E0B
#define RES_INFO_END_OFFSET 0x0FFF
#define RES_INFO_CHECKSUM 0x0FFE
#define RES_DATA_SIZE 499
/**/
#define TOTAL_CHECKSUM_OFFSET 0x0FFF

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

static int _ov13855_sunny_section_checksum(unsigned char *buf,
                                           unsigned int first,
                                           unsigned int last,
                                           unsigned int position);
static int _ov13855_sunny_buffer_init(void *otp_drv_handle);
static int _ov13855_sunny_parse_awb_data(void *otp_drv_handle);
static int _ov13855_sunny_parse_lsc_data(void *otp_drv_handle);
static int _ov13855_sunny_parse_af_data(void *otp_drv_handle);
static int _ov13855_sunny_parse_pdaf_data(void *otp_drv_handle);

static int _ov13855_sunny_awb_calibration(void *otp_drv_handle);
static int _ov13855_sunny_lsc_calibration(void *otp_drv_handle);
static int _ov13855_sunny_pdaf_calibration(void *otp_drv_handle);

static int ov13855_sunny_otp_create(otp_drv_init_para_t *input_para,
                                    cmr_handle *sns_af_drv_handle);
static int ov13855_sunny_otp_drv_delete(void *otp_drv_handle);
static int ov13855_sunny_otp_drv_read(void *otp_drv_handle,
                                      otp_params_t *p_data);
static int ov13855_sunny_otp_drv_write(void *otp_drv_handle,
                                       otp_params_t *p_data);
static int ov13855_sunny_otp_drv_parse(void *otp_drv_handle, void *P_params);
static int ov13855_sunny_otp_drv_calibration(void *otp_drv_handle);
static int ov13855_sunny_otp_drv_ioctl(otp_drv_cxt_t *otp_drv_handle, int cmd,
                                       void *params);

otp_drv_entry_t ov13855_sunny_drv_entry = {
    .otp_cfg =
        {
            .cali_items =
                {
                    .is_self_cal = FALSE, /*1:calibration at sensor
                                            side,0:calibration at isp*/
                    .is_dul_camc = FALSE, /* support dual camera calibration */
                    .is_awbc = FALSE,  /* support write balance calibration */
                    .is_lsc = FALSE,   /* support lens shadding calibration */
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
            .sensor_otp_create = ov13855_sunny_otp_create,
            .sensor_otp_delete = ov13855_sunny_otp_drv_delete,
            .sensor_otp_read = ov13855_sunny_otp_drv_read,
            .sensor_otp_write = ov13855_sunny_otp_drv_write,
            .sensor_otp_parse = ov13855_sunny_otp_drv_parse,
            .sensor_otp_calibration = ov13855_sunny_otp_drv_calibration,
            .sensor_otp_ioctl = ov13855_sunny_otp_drv_ioctl, /*expend*/
        },
};
