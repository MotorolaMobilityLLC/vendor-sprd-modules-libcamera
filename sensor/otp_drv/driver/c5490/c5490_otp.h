/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
//#include "sensor_drv_k.h"
#include "otp_parse.h"
#include "c5490_golden_otp.h"

#define C5490_I2C_ADDR    0xff
#define OTP_START_ADDR    0xffff

#define OTP_LEN           8192
#define GAIN_WIDTH         20
#define GAIN_HEIGHT        17

#define WB_DATA_SIZE      8 * 2 /*Don't forget golden wb data*/
#define AF_DATA_SIZE        6
#define LSC_SRC_CHANNEL_SIZE 656
#define LSC_CHANNEL_SIZE  876

/*Don't forget golden lsc otp data*/
#define FORMAT_DATA_LEN     WB_DATA_SIZE+AF_DATA_SIZE+GAIN_WIDTH*GAIN_WIDTH*4*2*2 
/*module base info*/
#define MODULE_INFO_OFFSET      0x01
#define MODULE_INFO_CHECK_SUM   0x0F
/**/
#define WB_INFO_OFFSET          0x15
#define WB_INFO_CHECK_SUM       0x21
/**/
#define AF_INFO_OFFSET          0x15
#define AF_INFO_CHECK_SUM       0x21
/**/
#define LSC_INFO_OFFSET         0x32
#define LSC_INFO_CHECK_SUM      0x21

#define LSC_GRID_SIZE           656
#define LSC_FORMAT_SIZE         GAIN_WIDTH * GAIN_HEIGHT * 2 * 4 * 2 /*include golden and random data*/
#define OTP_COMPRESSED_FLAG     OTP_COMPRESSED_14BITS

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

uint32_t c5490_section_checksum(unsigned char *buf,
unsigned int first, unsigned int last, unsigned int position);
uint32_t c5490_formatdata_buffer_init(void *ptr);
uint32_t c5490_format_wbdata(void *ptr);
uint32_t c5490_format_lensdata(void *ptr);
uint32_t c5490_wbc_calibration (void *ptr);
uint32_t c5490_lsc_calibration(void *ptr);
uint32_t c5490_pdaf_calibration(void *ptr);
uint32_t c5490_format_calibration_data(void *ptr);
uint32_t c5490_read_otp_data(void *ptr);
uint32_t c5490_write_otp_data(void *ptr);

otp_drv_cxt_t c5490_drv_cxt = {
	.otp_cfg =
	{
		.cali_items =
		{
			.is_self_cal = TRUE,    /*1:calibration at sensor side,0:calibration at isp*/
			.is_dul_camc = FALSE,   /* support dual camera calibration */
			.is_awbc     = TRUE,    /* support write balance calibration */
			.is_lsc      = TRUE,    /* support lens shadding calibration */
			.is_pdafc    = FALSE,   /* support pdaf calibration */
		},
		.base_info_cfg =
		{
			.compress_flag = OTP_COMPRESSED_FLAG,/*otp data compressed format ,should confirm with sensor fae*/
			.image_width   = 2592, /*the width of the stream the sensor can output*/
			.image_height  = 1944, /*the height of the stream the sensor can output*/
			.grid_width    = 96,   /*the height of the stream the sensor can output*/
			.grid_height   = 96,
			.gain_width    = GAIN_WIDTH,
			.gain_height   = GAIN_HEIGHT,
		},
	},
	.otp_ops =
	{
		.format_calibration_data = c5490_format_calibration_data,
		.awb_calibration          = c5490_wbc_calibration,
		.lsc_calibration         = c5490_lsc_calibration,
		.pdaf_calibration        = NULL,
		.dul_cam_calibration     = NULL,
		.read_otp_data           = c5490_read_otp_data,
		.write_otp_data          = c5490_write_otp_data,
	},
};
