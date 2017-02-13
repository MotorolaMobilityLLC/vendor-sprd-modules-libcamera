/**
 *	Copyright (c) 2016 Spreadtrum Technologies, Inc.
 *	All Rights Reserved.
 *	Confidential and Proprietary - Spreadtrum Technologies, Inc.
 **/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_parse.h"
#include "imx258_golden_otp.h"

#define GT24C64A_I2C_ADDR 0xA0 >> 1
#define OTP_START_ADDR    0x0000
#define OTP_END_ADDR      0x0FFF

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
#define MODULE_INFO_OFFSET      0x0000
#define MODULE_INFO_CHECK_SUM   0x000F
/**/
#define AWB_INFO_OFFSET          0x0016
#define AWB_INFO_SIZE			  6     /*byte*/
#define AWB_SECTION_NUM			  2
#define AWB_INFO_CHECKSUM       0x0022
/**/
#define AF_INFO_OFFSET          0x0010
#define AF_INFO_SIZE              5     /*byte*/
#define AF_INFO_CHECKSUM        0x0015
/*optical center*/
#define OPTICAL_INFO_OFFSET    0x0023
/*Lens shading calibration*/
#define LSC_INFO_OFFSET         0x0033
#define LSC_INFO_CHANNEL_SIZE	 726
#define LSC_INFO_CHECKSUM       0x0b8b
/*PDAF*/
#define PDAF_INFO_OFFSET        0x0b8c
#define PDAF_INFO_SIZE			 384
#define PDAF_INFO_CHECKSUM      0x0d0c

#define LSC_GRID_SIZE           726
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

uint32_t imx258_section_checksum(unsigned char *buf,
unsigned int first, unsigned int last, unsigned int position);
uint32_t imx258_formatdata_buffer_init(SENSOR_HW_HANDLE handle);
uint32_t imx258_format_wbdata(SENSOR_HW_HANDLE handle);
uint32_t imx258_format_lensdata(SENSOR_HW_HANDLE handle);
uint32_t imx258_awbc_calibration (SENSOR_HW_HANDLE handle);
uint32_t imx258_lsc_calibration(SENSOR_HW_HANDLE handle);
uint32_t imx258_pdaf_calibration(SENSOR_HW_HANDLE handle);
uint32_t imx258_format_calibration_data(SENSOR_HW_HANDLE handle);
uint32_t imx258_read_otp_data(SENSOR_HW_HANDLE handle);
uint32_t imx258_write_otp_data(SENSOR_HW_HANDLE handle);


