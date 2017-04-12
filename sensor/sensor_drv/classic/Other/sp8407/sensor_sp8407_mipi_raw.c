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
 * V5.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "parameters/sensor_sp8407_raw_param_main.c"
#else
#include "parameters/sensor_sp8407_raw_param.c"
#endif


#define CAMERA_IMAGE_180
#define SENSOR_NAME			"sp8407"
#define I2C_SLAVE_ADDR		0x6c //0x20    /* 8bit slave address*/

#define sp8407_PID_ADDR		0x300B
#define sp8407_PID_VALUE		0x88
#define sp8407_VER_ADDR		0x300C
#define sp8407_VER_VALUE		0x5A

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH		3264 
#define SNAPSHOT_HEIGHT		2448
#define PREVIEW_WIDTH		1632
#define PREVIEW_HEIGHT		1224

/*Raw Trim parameters*/
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			3264
#define SNAPSHOT_TRIM_H			2448
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			1632
#define PREVIEW_TRIM_H			1224
/*Mipi output*/
#define LANE_NUM			2 //4
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	720 
#define PREVIEW_MIPI_PER_LANE_BPS	720 

/*line time unit: 1ns*/
#define SNAPSHOT_LINE_TIME		26900//13400 
#define PREVIEW_LINE_TIME		26000//13400 

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH	 2482//2482
#define PREVIEW_FRAME_LENGTH	 1246//2482

/* please ref your spec */
#define FRAME_OFFSET			6
#define SENSOR_MAX_GAIN		0x800//0x7c0              //15.5 multiple
#define SENSOR_BASE_GAIN		0x80
#define SENSOR_MIN_SHUTTER		6

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
#define SUPPORT_AUTO_FRAME_LENGTH	0
/*delay 1 frame to write sensor gain*/
//#define GAIN_DELAY_1_FRAME
/* sensor parameters end */

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN			0x80
#else
#define ISP_BASE_GAIN			0x10
#endif
/* please don't change it */
#define EX_MCLK				24

#define IMAGE_NORMAL_MIRROR 
//#define IMAGE_H_MIRROR 
//#define IMAGE_V_MIRROR 
//#define IMAGE_HV_MIRROR 


/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info={
	2000000/SNAPSHOT_LINE_TIME,/*min 5fps*/
	SNAPSHOT_FRAME_LENGTH-FRAME_OFFSET,
	SENSOR_BASE_GAIN
	};
static uint32_t s_current_default_frame_length=PREVIEW_FRAME_LENGTH;
static struct sensor_ev_info_t s_sensor_ev_info={
	PREVIEW_FRAME_LENGTH-FRAME_OFFSET,
	SENSOR_BASE_GAIN
	};

//#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#include "sensor_sp8407_lightarray_otp.c"
static struct otp_info_t *s_sp8407_otp_info_ptr=&s_sp8407_lightarray_otp_info;
static struct raw_param_info_tab *s_sp8407_raw_param_tab_ptr=&s_sp8407_lightarray_raw_param_tab;  /*otp function interface*/
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_sp8407_ioctl_func_tab;
static struct sensor_raw_info *s_sp8407_mipi_raw_info_ptr = &s_sp8407_mipi_raw_info;
static EXIF_SPEC_PIC_TAKING_COND_T s_sp8407_exif_info;

static const SENSOR_REG_T sp8407_init_setting1[] ={
	//lsy 20160616
 {0x0100,0x00},
 {0x031e,0x0c},
 {0x3000,0x00},
 {0x300e,0x00},
 {0x3010,0x00},
 {0x3015,0x84},
 {0x3018,0x32},
 {0x3021,0x23},
 {0x3033,0x24},
 {0x3500,0x00},
 {0x3503,0x08},
 {0x3505,0x83},
 {0x3508,0x01},
 {0x3509,0x80},
 {0x350c,0x00},
 {0x350d,0x80},
 {0x350e,0x04},
 {0x350f,0x00},
 {0x3510,0x00},
 {0x3511,0x02},
 {0x3512,0x00},
 {0x3600,0x72},
 {0x3601,0x40},
 {0x3602,0x30},
 {0x3610,0xc5},
 {0x3611,0x58},
 {0x3612,0x5c},
 {0x3613,0x5a},
 {0x3614,0x60},
 {0x3628,0xff},
 {0x3629,0xff},
 {0x362a,0xff},
 {0x3633,0x10},
 {0x3634,0x10},
 {0x3635,0x10},
 {0x3636,0x10},
 {0x3663,0x08},
 {0x3669,0x34},
 {0x3706,0x86},
 {0x370b,0x7e},
 {0x3730,0x12},
 {0x3733,0x10},
 {0x3764,0x00},
 {0x3765,0x00},
 {0x3769,0x62},
 {0x376a,0x2a},
 {0x376b,0x30},
 {0x3780,0x00},
 {0x3781,0x24},
 {0x3782,0x00},
 {0x3783,0x23},
 {0x3798,0x2f},//add
 {0x37a1,0x60},
 {0x37a8,0x6a},
 {0x37ab,0x3f},
 {0x37c3,0xf1},
 {0x37c9,0x80},
 {0x37cb,0x03},
 {0x37cc,0x0a},
 {0x37cd,0x16},
 {0x37ce,0x1f}, 
 {0x3800,0x00},
 {0x3801,0x00},
 {0x3802,0x00},
 {0x3803,0x0c},
 {0x3804,0x0c},
 {0x3805,0xdf},
 {0x3806,0x09},
 {0x3807,0xa3},
 {0x3810,0x00},
 {0x3812,0x00},
 {0x3815,0x01},
 {0x3816,0x00},
 {0x3817,0x00},
 {0x3818,0x00},
 {0x3819,0x00},
 {0x382b,0x01},
 {0x3830,0x06},
 {0x3836,0x02},
 {0x3862,0x04},
 {0x3863,0x08},
 {0x3cc0,0x33},
 {0x3d85,0x17},
 {0x3d8c,0x73},
 {0x3d8d,0xde},
 {0x4001,0xe0},
 {0x4003,0x40},
 {0x4008,0x00},
 {0x400a,0x00},
 {0x400b,0x84},
 {0x400f,0x80},
 {0x4010,0xf0},
 {0x4011,0xff},
 {0x4012,0x02},//add
 {0x4013,0x01},//add
 {0x4014,0x01},
 {0x4015,0x01},
 {0x4042,0x00},
 {0x4043,0x80},
 {0x4044,0x00},
 {0x4045,0x80},
 {0x4046,0x00},
 {0x4047,0x80},
 {0x4048,0x00},
 {0x4049,0x80},
 {0x4041,0x03},
 {0x404c,0x20},
 {0x404d,0x00},
 {0x404e,0x20},
 {0x4203,0x80},
 {0x4307,0x30},
 {0x4317,0x00},
 {0x4503,0x08},
 {0x4601,0x80},
 {0x4800,0x44},
 {0x4816,0x53},
 {0x481b,0x58},
 {0x481f,0x27},
 {0x483c,0x0f},
 {0x484b,0x05},
 {0x5000,0x77},
 {0x5001,0x0a},
 {0x5004,0x04},
 {0x502e,0x03},
 {0x5030,0x41},
 {0x579a,0x00},
 {0x579c,0x00},
 {0x5780,0x14},
 {0x5781,0x0f},
 {0x5782,0x44},
 {0x5783,0x02},
 {0x5784,0x01},
 {0x5785,0x01},
 {0x5786,0x00},
 {0x5787,0x04},
 {0x5788,0x02},
 {0x5789,0x0f},
 {0x578a,0xfd},
 {0x578b,0xf5},//add
 {0x578c,0xf5},//add
 {0x578d,0x03},//add
 {0x578e,0x08},  
 {0x578f,0x0c},
 {0x5790,0x08},
 {0x5791,0x04},
 {0x5792,0x00},
 {0x5793,0x52},
 {0x5794,0xa3},
 {0x59f8,0x3d},
 {0x5a08,0x02},
 {0x5b00,0x02},
 {0x5b01,0x10},
 {0x5b02,0x03},
 {0x5b03,0xcf},
 {0x5b05,0x6c},
 {0x5e00,0x00},


};

static const SENSOR_REG_T sp8407_preview_setting1[] = {
	//1632 x1224 720Mbps 30fps 2lane lsy 20160616
   {0x0100,0x00},
   {0x0302,0x3c},     
   {0x0303,0x01},     
   {0x3501,0x4c},      
   {0x3502,0xe0},     
   {0x366e,0x08},     
   {0x3714,0x27},     
   {0x37c2,0x14},     
   {0x3808,0x06},     
   {0x3809,0x60},     
   {0x380a,0x04},     
   {0x380b,0xc8},     
   {0x380c,0x0e},     
   {0x380d,0xa0},     
   {0x380e,0x04}, //02   
   {0x380f,0xde},     
   {0x3811,0x08},     
   {0x3813,0x02},//add  
   {0x3814,0x03},//add 
   {0x3820,0x90},     
   {0x3821,0x67},     
   {0x382a,0x03},  
   {0x4009,0x05},    
   {0x4837,0x16},   
   {0x5795,0x00},     
   {0x5796,0x10},     
   {0x5797,0x10},     
   {0x5798,0x73},     
   {0x5799,0x73},
   {0x579b,0x28},    
   {0x579d,0x16},     
   {0x579e,0x06},     
   {0x579f,0x20},     
   {0x57a0,0x04},
   {0x57a1,0xa0},
   {0x3820,0xd6},// flip    
   {0x3821,0x61},     
   {0x502e,0x00},     
   {0x5001,0x0e},     
   {0x5004,0x00},
   {0x376b,0x36},

};

static const SENSOR_REG_T sp8407_snapshot_setting1[] = {
	//3264x2448 15fps 720Mbps 2lane lsy 20160616
{0x0100,0x00},
{0x0302,0x3c},
{0x0303,0x01},
{0x3501,0x9a},
{0x3502,0x20},
{0x366e,0x10},
{0x3714,0x23},
{0x37c2,0x04},
{0x3808,0x0c},
{0x3809,0xc0},
{0x380a,0x09},
{0x380b,0x90},
{0x380c,0x0f},
{0x380d,0x20},
{0x380e,0x09},//04
{0x380f,0xb2},//02
{0x3811,0x10},
{0x3813,0x04},//add
{0x3814,0x01},//add
{0x3820,0x80},
{0x3821,0x46},
{0x382a,0x01},
{0x4009,0x0b},
{0x4837,0x16},
{0x5795,0x02},
{0x5796,0x20},
{0x5797,0x20},
{0x5798,0xd5},
{0x5799,0xd5},
{0x579b,0x50},
{0x579d,0x2c},
{0x579e,0x0c},
{0x579f,0x40},
{0x57a0,0x09},
{0x57a1,0x40},
{0x3820,0xc6},// flip    
{0x3821,0x40},     
{0x502e,0x00},     
{0x5001,0x0e},     
{0x5004,0x00},
{0x376b,0x36},

};     

/*//delay 200ms
{SENSOR_WRITE_DELAY, 200},
*/
static const SENSOR_REG_T sp8407_init_setting[] = {
	  //100 99 1632x1224
		{0x0103, 0x01}, 
		{0x0100, 0x00}, 
		{0x0302, 0x3c}, 
		{0x0303, 0x01}, 
		{0x031e, 0x0c}, 
		{0x3000, 0x00}, 
		{0x300e, 0x00}, 
		{0x3010, 0x00}, 
		{0x3015, 0x84}, 
		{0x3018, 0x72}, 
		{0x3021, 0x23}, 
		{0x3033, 0x24}, 
		{0x3500, 0x00}, 
		{0x3501, 0x4c}, 
		{0x3502, 0xe0}, 
		{0x3503, 0x08}, 
		{0x3505, 0x83}, 
		{0x3508, 0x01}, 
		{0x3509, 0x80}, 
		{0x350c, 0x00}, 
		{0x350d, 0x80}, 
		{0x350e, 0x04}, 
		{0x350f, 0x00}, 
		{0x3510, 0x00}, 
		{0x3511, 0x02}, 
		{0x3512, 0x00}, 
		{0x3600, 0x72}, 
		{0x3601, 0x40}, 
		{0x3602, 0x30}, 
		{0x3610, 0xc5}, 
		{0x3611, 0x58}, 
		{0x3612, 0x5c}, 
		{0x3613, 0x5a}, 
		{0x3614, 0x60}, 
		{0x3628, 0xff}, 
		{0x3629, 0xff}, 
		{0x362a, 0xff}, 
		{0x3633, 0x10}, 
		{0x3634, 0x10}, 
		{0x3635, 0x10}, 
		{0x3636, 0x10}, 
		{0x3663, 0x08}, 
		{0x3669, 0x34}, 
		{0x366e, 0x08}, 
		{0x3706, 0x86}, 
		{0x370b, 0x7e}, 
		{0x3714, 0x27}, 
		{0x3730, 0x12}, 
		{0x3733, 0x10}, 
		{0x3764, 0x00}, 
		{0x3765, 0x00}, 
		{0x3769, 0x62}, 
		{0x376a, 0x2a}, 
		{0x376b, 0x30}, 
		{0x3780, 0x00}, 
		{0x3781, 0x24}, 
		{0x3782, 0x00}, 
		{0x3783, 0x23}, 
		{0x3798, 0x2f}, 
		{0x37a1, 0x60}, 
		{0x37a8, 0x6a}, 
		{0x37ab, 0x3f}, 
		{0x37c2, 0x14}, 
		{0x37c3, 0xf1}, 
		{0x37c9, 0x80}, 
		{0x37cb, 0x03}, 
		{0x37cc, 0x0a}, 
		{0x37cd, 0x16}, 
		{0x37ce, 0x1f}, 
		{0x3800, 0x00}, 
		{0x3801, 0x00}, 
		{0x3802, 0x00}, 
		{0x3803, 0x0c}, 
		{0x3804, 0x0c}, 
		{0x3805, 0xdf}, 
		{0x3806, 0x09}, 
		{0x3807, 0xa3}, 
		{0x3808, 0x06}, 
		{0x3809, 0x60}, 
		{0x380a, 0x04}, 
		{0x380b, 0xc8}, 
		{0x380c, 0x07}, 
		{0x380d, 0x8c}, 
		{0x380e, 0x09}, //
		{0x380f, 0xb2}, //
		{0x3810, 0x00}, 
		{0x3811, 0x08}, 
		{0x3812, 0x00}, 
		{0x3813, 0x02}, 
		{0x3814, 0x03}, 
		{0x3815, 0x01}, 
		{0x3816, 0x00}, 
		{0x3817, 0x00}, 
		{0x3818, 0x00}, 
		{0x3819, 0x00}, 
		{0x3820, 0x90}, 
		{0x3821, 0x67}, 
		{0x382a, 0x03}, 
		{0x382b, 0x01}, 
		{0x3830, 0x06}, 
		{0x3836, 0x02}, 
		{0x3862, 0x04}, 
		{0x3863, 0x08}, 
		{0x3cc0, 0x33}, 
		{0x3d85, 0x17}, 
		{0x3d8c, 0x73}, 
		{0x3d8d, 0xde}, 
		{0x4001, 0xe0}, 
		{0x4003, 0x40}, 
		{0x4008, 0x00}, 
		{0x4009, 0x05}, 
		{0x400a, 0x00},
		{0x400b, 0x84},
		{0x400f, 0x80}, 
		{0x4010, 0xf0}, 
		{0x4011, 0xff}, 
		{0x4012, 0x02}, 
		{0x4013, 0x01}, 
		{0x4014, 0x01}, 
		{0x4015, 0x01}, 
		{0x4042, 0x00}, 
		{0x4043, 0x80}, 
		{0x4044, 0x00}, 
		{0x4045, 0x80}, 
		{0x4046, 0x00}, 
		{0x4047, 0x80}, 
		{0x4048, 0x00}, 
		{0x4049, 0x80}, 
		{0x4041, 0x03}, 
		{0x404c, 0x20}, 
		{0x404d, 0x00}, 
		{0x404e, 0x20}, 
		{0x4203, 0x80}, 
		{0x4307, 0x30}, 
		{0x4317, 0x00}, 
		{0x4503, 0x08}, 
		{0x4601, 0x80},
		{0x4800, 0x44},  
		{0x4816, 0x53}, 
		{0x481b, 0x58}, 
		{0x481f, 0x27}, 
		{0x4837, 0x16}, 
		{0x483c, 0x0f}, 
		{0x484b, 0x05}, 
		{0x5000, 0x77}, 
	//	{0x5001, 0x0a}, 
		//{0x5004, 0x04}, 
	//	{0x502e, 0x03}, 
		{0x5030, 0x41}, 
		{0x5795, 0x00}, 
		{0x5796, 0x10}, 
		{0x5797, 0x10}, 
		{0x5798, 0x73}, 
		{0x5799, 0x73}, 
		{0x579a, 0x00}, 
		{0x579b, 0x28}, 
		{0x579c, 0x00}, 
		{0x579d, 0x16}, 
		{0x579e, 0x06}, 
		{0x579f, 0x20}, 
		{0x57a0, 0x04}, 
		{0x57a1, 0xa0}, 
		{0x5780, 0x14}, 
		{0x5781, 0x0f}, 
		{0x5782, 0x44}, 
		{0x5783, 0x02}, 
		{0x5784, 0x01}, 
		{0x5785, 0x01}, 
		{0x5786, 0x00}, 
		{0x5787, 0x04}, 
		{0x5788, 0x02}, 
		{0x5789, 0x0f}, 
		{0x578a, 0xfd}, 
		{0x578b, 0xf5}, 
		{0x578c, 0xf5}, 
		{0x578d, 0x03}, 
		{0x578e, 0x08}, 
		{0x578f, 0x0c}, 
		{0x5790, 0x08}, 
		{0x5791, 0x04}, 
		{0x5792, 0x00}, 
		{0x5793, 0x52}, 
		{0x5794, 0xa3}, 
		{0x59f8, 0x3d}, //update from AM11a, for LENC h-band issue
		{0x5a08, 0x02}, 
		{0x5b00, 0x02}, 
		{0x5b01, 0x10}, 
		{0x5b02, 0x03}, 
		{0x5b03, 0xcf}, 
		{0x5b05, 0x6c}, 
		{0x5e00, 0x00}, 
		//{0x0100, 0x01}, 

};

static const SENSOR_REG_T sp8407_preview_setting[] = {
		//mipi bit rate 720Mbps, //1632x1224, 30fps, frame_length 2482,line time 13.41us
		
		{0x0100, 0x00},
		{0x3501, 0x4c}, 
		{0x3502, 0xe0},
		{0x366e, 0x08}, 
		{0x3714, 0x27},
		{0x37c2, 0x14}, 
		{0x3800, 0x00},
		{0x3801, 0x00},
		{0x3802, 0x00},
		{0x3803, 0x0c}, 
		{0x3804, 0x0c},  
		{0x3805, 0xdf},
		{0x3806, 0x09}, 
		{0x3807, 0xa3}, 
		{0x3808, 0x06}, 
		{0x3809, 0x60},  
		{0x380a, 0x04}, 
		{0x380c, 0x07},
		{0x380d, 0x8c},   
		{0x380b, 0xc8},  
		{0x380e, 0x09},
		{0x380f, 0xb2},
		{0x3810, 0x00},
		{0x3811, 0x02}, 
		{0x3813, 0x02},  
		{0x3814, 0x03}, 		
	
 #ifdef IMAGE_NORMAL_MIRROR
		{0x3820, 0x90},  
		{0x3821, 0x67}, 
		{0x502e, 0x03},  
		{0x5001, 0x0a}, 		
		{0x5004, 0x04}, 
		{0x376b, 0x30},   
#endif
#ifdef IMAGE_H_MIRROR
		{0x3820, 0x90},  
		{0x3821, 0x61}, 
		{0x502e, 0x03},  
		{0x5001, 0x0a}, 		
		{0x5004, 0x00}, 
		{0x376b, 0x30},  
#endif
#ifdef IMAGE_V_MIRROR
		{0x3820, 0x96},  
		{0x3821, 0x67}, 
		{0x502e, 0x00},  
		{0x5001, 0x0e}, 		
		{0x5004, 0x04}, 
		{0x376b, 0x36},   
#endif
#ifdef IMAGE_HV_MIRROR
		{0x3820, 0x96},  
		{0x3821, 0x61}, 
		{0x502e, 0x00},  
		{0x5001, 0x0e}, 		
		{0x5004, 0x00}, 
		{0x376b, 0x36}, 
#endif 
/*=============flip mirror end=====================*/  
		{0x382a, 0x03},    
		{0x4009, 0x05},
		{0x4601, 0x80},  
		{0x5795, 0x00},    
		{0x5796, 0x10},  
		{0x5797, 0x10},  
		{0x5798, 0x73},  
		{0x5799, 0x73}, 
		{0x579a, 0x00}, 
		{0x579b, 0x28},
		{0x579c, 0x00},    
		{0x579d, 0x16},  
		{0x579e, 0x06},    
		{0x579f, 0x20},  
		{0x57a0, 0x04},    
		{0x57a1, 0xa0},  
		//{0x0100, 0x01},
};

static const SENSOR_REG_T sp8407_snapshot_setting[] = {
	
	//mipi bit rate 720Mbps,3264x2448,  30fps, frame_length 2482,line time 13.41us
        {0x0100,0x00},
        {0x3501,0x9a},
        {0x3502,0x20},       
        {0x366e,0x10},
        {0x3714,0x23},
        {0x37c2,0x04},
        {0x3800, 0x00},
		{0x3801, 0x00},
		{0x3802, 0x00},
		{0x3803, 0x0c}, 
		{0x3804, 0x0c},  
		{0x3805, 0xdf},
		{0x3806, 0x09}, 
	 	{0x3807, 0xa3}, 
        {0x3808,0x0c},
        {0x3809,0xc0},
        {0x380a,0x09},
        {0x380b,0x90},
        {0x380e,0x09},
        {0x380f,0xb2},
        {0x3810,0x00}, 
        {0x3811,0x04},
        {0x3812,0x00}, 
        {0x3813,0x02},
        {0x3814,0x01},
        
/*=============flip mirror start=====================*/ 
	
#ifdef IMAGE_NORMAL_MIRROR
		{0x3820, 0x80},  
		{0x3821, 0x46}, 
		{0x502e, 0x03},  
		{0x5001, 0x0a}, 		
		{0x5004, 0x04}, 
		{0x376b, 0x30},   
#endif
#ifdef IMAGE_H_MIRROR
		{0x3820, 0x80},  
		{0x3821, 0x40}, 
		{0x502e, 0x03},  
		{0x5001, 0x0a}, 		
		{0x5004, 0x00}, 
		{0x376b, 0x30}, 
#endif
#ifdef IMAGE_V_MIRROR
		{0x3820, 0x86},  
		{0x3821, 0x46}, 
		{0x502e, 0x00},  
		{0x5001, 0x0e}, 		
		{0x5004, 0x04}, 
		{0x376b, 0x36}, 
#endif
#ifdef IMAGE_HV_MIRROR
		{0x3820, 0x86},  
		{0x3821, 0x40}, 
		{0x502e, 0x00},  
		{0x5001, 0x0e}, 		
		{0x5004, 0x00}, 
		{0x376b, 0x36}, 
#endif 

 /*=============flip mirror end=====================*/  

        {0x382a,0x01},
        {0x4009,0x0b},
        {0x4601,0x80}, 
        {0x5795,0x02},
        {0x5796,0x20},
        {0x5797,0x20},
        {0x5798,0xd5},
        {0x5799,0xd5},    
        {0x579a,0x00}, 
        {0x579b,0x50}, 
        {0x579c,0x00}, 
        {0x579d,0x2c},
        {0x579e,0x0c},
        {0x579f,0x40},
        {0x57a0,0x09},
        {0x57a1,0x40},
        //{0x0100,0x01},

};

static SENSOR_REG_TAB_INFO_T s_sp8407_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(sp8407_init_setting1), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(sp8407_preview_setting1),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(sp8407_snapshot_setting1),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_sp8407_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{PREVIEW_TRIM_X, PREVIEW_TRIM_Y, PREVIEW_TRIM_W, PREVIEW_TRIM_H,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_TRIM_W, PREVIEW_TRIM_H}},
	{SNAPSHOT_TRIM_X, SNAPSHOT_TRIM_Y, SNAPSHOT_TRIM_W, SNAPSHOT_TRIM_H,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_TRIM_W, SNAPSHOT_TRIM_H}},
};

static const SENSOR_REG_T s_sp8407_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static const SENSOR_REG_T s_sp8407_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static SENSOR_VIDEO_INFO_T s_sp8407_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_sp8407_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_sp8407_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t sp8407_set_video_mode(SENSOR_HW_HANDLE handle, uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	uint32_t mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_sp8407_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_sp8407_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr)
	     || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_sp8407_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
	/* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	 * bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	 * other bit: reseved
	 */
	SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,
	/* preview mode */
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
	/* image effect */
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	/* while balance mode */
	0,
	/* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	 * bit[8:31] reseved
	 */
	7,
	/* reset pulse level */
	SENSOR_LOW_PULSE_RESET,
	/* reset pulse width(ms) */
	50,
	/* 1: high level valid; 0: low level valid */
	SENSOR_LOW_LEVEL_PWDN,
	/* count of identify code */
	1,
	/* supply two code to identify sensor.
	 * for Example: index = 0-> Device id, index = 1 -> version id
	 * customer could ignore it.
	 */
	{{sp8407_PID_ADDR, sp8407_PID_VALUE}
	 ,
	 {sp8407_VER_ADDR, sp8407_VER_VALUE}
	 }
	,
	/* voltage of avdd */
	SENSOR_AVDD_2800MV,
	/* max width of source image */
	SNAPSHOT_WIDTH,
	/* max height of source image */
	SNAPSHOT_HEIGHT,
	/* name of sensor */
	SENSOR_NAME,
	/* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	 * if set to SENSOR_IMAGE_FORMAT_MAX here,
	 * image format depent on SENSOR_REG_TAB_INFO_T
	 */
	SENSOR_IMAGE_FORMAT_RAW,
	/*  pattern of input image form sensor */
	SENSOR_IMAGE_PATTERN_RAWRGB_B,
	/* point to resolution table information structure */
	s_sp8407_resolution_tab_raw,
	/* point to ioctl function table */
	&s_sp8407_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_sp8407_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_sp8407_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	1,
	/* skip frame num before capture */
	1,
    /* skip frame num for flash capture */
    6,
    /* skip frame num on mipi cap */
    0,
	/* deci frame num during preview */
	0,
	/* deci frame num during video preview */
	0,
	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0}
	,
	0,
	/* skip frame num while change setting */
	1,
	/* horizontal  view angle*/
	65,
	/* vertical view angle*/
	60,
	(cmr_s8 *) "sp8407_v1",
};

static SENSOR_STATIC_INFO_T s_sp8407_static_info = {
		220, // f-number,focal ratio
		346, // focal_length;
		0,	 // max_fps,max fps of sensor's all settings,it will be calculated from
				 // sensor mode fps
		8,	 // max_adgain,AD-gain
		0,	 // ois_supported;
		0,	 // pdaf_supported;
		1,	 // exp_valid_frame_num;N+2-1
		64,  // clamp_level,black level
		1,	 // adgain_valid_frame_num;N+1-1
};

static SENSOR_MODE_FPS_INFO_T s_sp8407_mode_fps_info = {
		0, // is_init;
		{{SENSOR_MODE_COMMON_INIT, 0, 1, 0, 0},
		 {SENSOR_MODE_PREVIEW_ONE, 0, 1, 0, 0},
		 {SENSOR_MODE_SNAPSHOT_ONE_FIRST, 0, 1, 0, 0},
		 {SENSOR_MODE_SNAPSHOT_ONE_SECOND, 0, 1, 0, 0},
		 {SENSOR_MODE_SNAPSHOT_ONE_THIRD, 0, 1, 0, 0},
		 {SENSOR_MODE_PREVIEW_TWO, 0, 1, 0, 0},
		 {SENSOR_MODE_SNAPSHOT_TWO_FIRST, 0, 1, 0, 0},
		 {SENSOR_MODE_SNAPSHOT_TWO_SECOND, 0, 1, 0, 0},
		 {SENSOR_MODE_SNAPSHOT_TWO_THIRD, 0, 1, 0, 0}}};
/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp8407_init_mode_fps_info(SENSOR_HW_HANDLE handle) {
		uint32_t rtn = SENSOR_SUCCESS;
		SENSOR_PRINT("sp8407_init_mode_fps_info:E");
		if (!s_sp8407_mode_fps_info.is_init) {
				uint32_t i, modn, tempfps = 0;
				SENSOR_PRINT("sp8407_init_mode_fps_info:start init");
				for (i = 0; i < NUMBER_OF_ARRAY(s_sp8407_resolution_trim_tab); i++) {
						// max fps should be multiple of 30,it calulated from line_time and
						// frame_line
						tempfps = s_sp8407_resolution_trim_tab[i].line_time *
											s_sp8407_resolution_trim_tab[i].frame_line;
						if (0 != tempfps) {
								tempfps = 1000000000 / tempfps;
								modn = tempfps / 30;
								if (tempfps > modn * 30)
										modn++;
								s_sp8407_mode_fps_info.sensor_mode_fps[i].max_fps = modn * 30;
								if (s_sp8407_mode_fps_info.sensor_mode_fps[i].max_fps > 30) {
										s_sp8407_mode_fps_info.sensor_mode_fps[i].is_high_fps = 1;
										s_sp8407_mode_fps_info.sensor_mode_fps[i]
												.high_fps_skip_num =
												s_sp8407_mode_fps_info.sensor_mode_fps[i].max_fps / 30;
								}
								if (s_sp8407_mode_fps_info.sensor_mode_fps[i].max_fps >
										s_sp8407_static_info.max_fps) {
										s_sp8407_static_info.max_fps =
												s_sp8407_mode_fps_info.sensor_mode_fps[i].max_fps;
								}
						}
						SENSOR_PRINT("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
												 tempfps, s_sp8407_resolution_trim_tab[i].frame_line,
												 s_sp8407_resolution_trim_tab[i].line_time);
						SENSOR_PRINT("mode %d,max_fps: %d ", i,
												 s_sp8407_mode_fps_info.sensor_mode_fps[i].max_fps);
						SENSOR_PRINT(
								"is_high_fps: %d,highfps_skip_num %d",
								s_sp8407_mode_fps_info.sensor_mode_fps[i].is_high_fps,
								s_sp8407_mode_fps_info.sensor_mode_fps[i].high_fps_skip_num);
				}
				s_sp8407_mode_fps_info.is_init = 1;
		}
		SENSOR_PRINT("sp8407_init_mode_fps_info:X");
		return rtn;
}

static uint32_t sp8407_get_static_info(SENSOR_HW_HANDLE handle,
																			 uint32_t *param) {
		uint32_t rtn = SENSOR_SUCCESS;
		struct sensor_ex_info *ex_info;
		uint32_t up = 0;
		uint32_t down = 0;
		// make sure we have get max fps of all settings.
		if (!s_sp8407_mode_fps_info.is_init) {
				sp8407_init_mode_fps_info(handle);
		}
		ex_info = (struct sensor_ex_info *)param;
		ex_info->f_num = s_sp8407_static_info.f_num;
		ex_info->focal_length = s_sp8407_static_info.focal_length;
		ex_info->max_fps = s_sp8407_static_info.max_fps;
		ex_info->max_adgain = s_sp8407_static_info.max_adgain;
		ex_info->ois_supported = s_sp8407_static_info.ois_supported;
		ex_info->pdaf_supported = s_sp8407_static_info.pdaf_supported;
		ex_info->exp_valid_frame_num = s_sp8407_static_info.exp_valid_frame_num;
		ex_info->clamp_level = s_sp8407_static_info.clamp_level;
		ex_info->adgain_valid_frame_num =
				s_sp8407_static_info.adgain_valid_frame_num;
		ex_info->preview_skip_num = g_sp8407_mipi_raw_info.preview_skip_num;
		ex_info->capture_skip_num = g_sp8407_mipi_raw_info.capture_skip_num;
		ex_info->name = (cmr_s8 *)g_sp8407_mipi_raw_info.name;
		ex_info->sensor_version_info = (cmr_s8 *)g_sp8407_mipi_raw_info.sensor_version_info;
		// vcm_ak7371_get_pose_dis(handle, &up, &down);
		ex_info->pos_dis.up2hori = up;
		ex_info->pos_dis.hori2down = down;
		SENSOR_PRINT("f_num: %d", ex_info->f_num);
		SENSOR_PRINT("max_fps: %d", ex_info->max_fps);
		SENSOR_PRINT("max_adgain: %d", ex_info->max_adgain);
		SENSOR_PRINT("ois_supported: %d", ex_info->ois_supported);
		SENSOR_PRINT("pdaf_supported: %d", ex_info->pdaf_supported);
		SENSOR_PRINT("exp_valid_frame_num: %d", ex_info->exp_valid_frame_num);
		SENSOR_PRINT("clam_level: %d", ex_info->clamp_level);
		SENSOR_PRINT("adgain_valid_frame_num: %d", ex_info->adgain_valid_frame_num);
		SENSOR_PRINT("sensor name is: %s", ex_info->name);
		SENSOR_PRINT("sensor version info is: %s", ex_info->sensor_version_info);

		return rtn;
}

static uint32_t sp8407_get_fps_info(SENSOR_HW_HANDLE handle, uint32_t *param) {
		uint32_t rtn = SENSOR_SUCCESS;
		SENSOR_MODE_FPS_T *fps_info;
		// make sure have inited fps of every sensor mode.
		if (!s_sp8407_mode_fps_info.is_init) {
				sp8407_init_mode_fps_info(handle);
		}
		fps_info = (SENSOR_MODE_FPS_T *)param;
		uint32_t sensor_mode = fps_info->mode;
		fps_info->max_fps =
				s_sp8407_mode_fps_info.sensor_mode_fps[sensor_mode].max_fps;
		fps_info->min_fps =
				s_sp8407_mode_fps_info.sensor_mode_fps[sensor_mode].min_fps;
		fps_info->is_high_fps =
				s_sp8407_mode_fps_info.sensor_mode_fps[sensor_mode].is_high_fps;
		fps_info->high_fps_skip_num =
				s_sp8407_mode_fps_info.sensor_mode_fps[sensor_mode].high_fps_skip_num;
		SENSOR_PRINT("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
		SENSOR_PRINT("min_fps: %d", fps_info->min_fps);
		SENSOR_PRINT("is_high_fps: %d", fps_info->is_high_fps);
		SENSOR_PRINT("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

		return rtn;
}
	

#if 0//defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)

#define param_update(x1,x2) sprintf(name,"/data/misc/media/sp8407_%s.bin",x1);\
				if(0==access(name,R_OK))\
				{\
					FILE* fp = NULL;\
					SENSOR_PRINT("param file %s exists",name);\
					if( NULL!=(fp=fopen(name,"rb")) ){\
						fread((void*)x2,1,sizeof(x2),fp);\
						fclose(fp);\
					}else{\
						SENSOR_PRINT("param open %s failure",name);\
					}\
				}else{\
					SENSOR_PRINT("access %s failure",name);\
				}\
				memset(name,0,sizeof(name))

static uint32_t sp8407_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	isp_raw_para_update_from_file(&g_sp8407_mipi_raw_info,0);

	struct sensor_raw_info* raw_sensor_ptr=s_sp8407_mipi_raw_info_ptr;
	struct isp_mode_param* mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	int i;
	char name[100] = {'\0'};

	for (i=0; i<mode_common_ptr->block_num; i++) {
		struct isp_block_header* header = &(mode_common_ptr->block_header[i]);
		uint8_t* data = (uint8_t*)mode_common_ptr + header->offset;
		switch (header->block_id)
		{
		case	ISP_BLK_PRE_WAVELET_V1: {
				/* modify block data */
				struct sensor_pwd_param* block = (struct sensor_pwd_param*)data;

				static struct sensor_pwd_level pwd_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/pwd_param.h"
				};

				param_update("pwd_param",pwd_param);

				block->param_ptr = pwd_param;
			}
			break;

		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;

				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/bpc_param.h"
				};

				param_update("bpc_param",bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/bdn_param.h"
				};

				param_update("bdn_param",bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/grgb_param.h"
				};

				param_update("grgb_param",grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				static struct sensor_nlm_level nlm_param[32] = {
					#include "NR/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[32] = {
					#include "NR/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
					#include "NR/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
					#include "NR/flat_offset_param.h"
				};

				param_update("flat_offset_param",flat_offset_param);

				block->param_nlm_ptr = nlm_param;
				block->param_vst_ptr = vst_param;
				block->param_ivst_ptr = ivst_param;
				block->param_flat_offset_ptr = flat_offset_param;
			}
			break;

		case	ISP_BLK_CFA_V1: {
				/* modify block data */
				struct sensor_cfa_param_v1* block = (struct sensor_cfa_param_v1*)data;
				static struct sensor_cfae_level cfae_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/cfae_param.h"
				};

				param_update("cfae_param",cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param",precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param",yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/prfy_param.h"
				};

				param_update("prfy_param",prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param",uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {
				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/edge_param.h"
				};

				param_update("edge_param",edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param",uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/iircnr_param.h"
				};

				param_update("iircnr_param",iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param",iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/cce_uv_param.h"
				};

				param_update("cce_uv_param",cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;

			static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/y_afm_param.h"
				};

				param_update("y_afm_param",y_afm_param);

				block->param_ptr = y_afm_param;
			}
			break;

		default:
			break;
		}
	}


	return rtn;
}
#endif

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t sp8407_get_default_frame_length(SENSOR_HW_HANDLE handle,uint32_t mode)
{
	return s_sp8407_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp8407_group_hold_on(SENSOR_HW_HANDLE handle)
{
	SENSOR_PRINT("E");

}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp8407_group_hold_off(SENSOR_HW_HANDLE handle)
{
	SENSOR_PRINT("E");

}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/

static uint16_t sp8407_read_gain(SENSOR_HW_HANDLE handle)
{
	return s_sensor_ev_info.preview_gain;
}





/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp8407_write_gain(SENSOR_HW_HANDLE handle,uint32_t gain)
{
	if (SENSOR_MAX_GAIN < gain)
			gain = SENSOR_MAX_GAIN;

	Sensor_WriteReg(0x320a, 0x01);
	
	//group 1:all other registers( gain)
	Sensor_WriteReg(0x3208, 0x01);
	
	Sensor_WriteReg(0x3508, (gain >> 8) & 0x07);//1f);
	Sensor_WriteReg(0x3509, gain & 0xff);
	
	Sensor_WriteReg(0x3208, 0x11);
	Sensor_WriteReg(0x3208, 0xA1);

	//sp8407_group_hold_off();

}



/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t sp8407_read_frame_length(SENSOR_HW_HANDLE handle)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x380e) & 0xff;
	frame_len_l = Sensor_ReadReg(0x380f) & 0xff;

	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void sp8407_write_frame_length(SENSOR_HW_HANDLE handle,uint32_t frame_len)
{
	Sensor_WriteReg(0x380e, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x380f, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp8407_read_shutter(SENSOR_HW_HANDLE handle)
{
	return s_sensor_ev_info.preview_shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void sp8407_write_shutter(SENSOR_HW_HANDLE handle,uint32_t shutter)
{
	uint16_t value=0x00;
	value=(shutter<<0x04)&0xff;
	Sensor_WriteReg(0x3502,value);
	value=(shutter>>0x04)&0xff;
	Sensor_WriteReg(0x3501,value);
	value=(shutter>>0x0c)&0x0f;
	Sensor_WriteReg(0x3500,value);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t sp8407_update_exposure(SENSOR_HW_HANDLE handle,uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	//sp8407_group_hold_on();

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = sp8407_read_frame_length(handle);

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		sp8407_write_frame_length(handle,dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	sp8407_write_shutter(handle,shutter);
	#ifdef GAIN_DELAY_1_FRAME
	usleep(dest_fr_len*PREVIEW_LINE_TIME/10);
	#endif
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp8407_power_on(SENSOR_HW_HANDLE handle,uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_sp8407_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_sp8407_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_sp8407_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_sp8407_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_sp8407_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
//dfsl af start
		usleep(1* 1000);
//dfsl end
		//Sensor_SetResetLevel(reset_level);
		usleep(1 * 1000);
		Sensor_SetAvddVoltage(avdd_val);
		Sensor_SetDvddVoltage(dvdd_val);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1 * 1000);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		usleep(1 * 1000);
		Sensor_SetMCLK(EX_MCLK);

	} else {

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1 * 1000);
		//Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(1 * 1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);

	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}


/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long sp8407_access_val(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret = SENSOR_FAIL;
    SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	
	if(!param_ptr){
		#ifdef FEATURE_OTP
		if(PNULL!=s_sp8407_raw_param_tab_ptr->cfg_otp){
			ret = s_sp8407_raw_param_tab_ptr->cfg_otp(s_sp8407_otp_info_ptr);
			//checking OTP apply result
			if (SENSOR_SUCCESS != ret) {
				SENSOR_PRINT("apply otp failed");
			}
		} else {
			SENSOR_PRINT("no update otp function!");
	}
	#endif
		return ret;
	}
	
	SENSOR_PRINT("sensor sp8407: param_ptr->type=%x", param_ptr->type);
	
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = sp8407_read_shutter(handle);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			*((uint32_t*)param_ptr->pval) = sp8407_read_gain(handle);
      break;
    case SENSOR_VAL_TYPE_GET_STATIC_INFO:
        ret = sp8407_get_static_info(handle, param_ptr->pval);
        break;
    case SENSOR_VAL_TYPE_GET_FPS_INFO:
        ret = sp8407_get_fps_info(handle, param_ptr->pval);
        break;
   // case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
   //     ret = sp8407_set_sensor_close_flag(handle);
			break;
		default:
			break;
	}
    ret = SENSOR_SUCCESS;

	return ret;
}

/*==============================================================================
 * Description:
 * Initialize Exif Info
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp8407_InitExifInfo(void)
{
	EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = &s_sp8407_exif_info;

	memset(&s_sp8407_exif_info, 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

	SENSOR_PRINT("Start");

	/*aperture = numerator/denominator */
	/*fnumber = numerator/denominator */
	exif_ptr->valid.FNumber=1;
	exif_ptr->FNumber.numerator=14;
	exif_ptr->FNumber.denominator=5;
	
	exif_ptr->valid.ApertureValue=1;
	exif_ptr->ApertureValue.numerator=14;
	exif_ptr->ApertureValue.denominator=5;
	exif_ptr->valid.MaxApertureValue=1;
	exif_ptr->MaxApertureValue.numerator=14;
	exif_ptr->MaxApertureValue.denominator=5;
	exif_ptr->valid.FocalLength=1;
	exif_ptr->FocalLength.numerator=289;
	exif_ptr->FocalLength.denominator=100;

	return SENSOR_SUCCESS;
}

static uint32_t  sp8407_get_exif_info(unsigned long param)
{
	return (unsigned long) & s_sp8407_exif_info;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp8407_identify(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint16_t pid_value = 0x00;
	uint16_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(sp8407_PID_ADDR);

	if (sp8407_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(sp8407_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (sp8407_VER_VALUE == ver_value) {
			SENSOR_PRINT_HIGH("this is sp8407 sensor");
			#ifdef FEATURE_OTP
			/*if read otp info failed or module id mismatched ,identify failed ,return SENSOR_FAIL ,exit identify*/
			if(PNULL!=s_sp8407_raw_param_tab_ptr->identify_otp){
				SENSOR_PRINT("identify module_id=0x%x",s_sp8407_raw_param_tab_ptr->param_id);
				//set default value
				memset(s_sp8407_otp_info_ptr, 0x00, sizeof(struct otp_info_t));
				ret_value = s_sp8407_raw_param_tab_ptr->identify_otp(s_sp8407_otp_info_ptr);
				
				if(SENSOR_SUCCESS == ret_value ){
					SENSOR_PRINT("identify otp sucess! module_id=0x%x, module_name=%s",s_sp8407_raw_param_tab_ptr->param_id,MODULE_NAME);
				} else{
					SENSOR_PRINT("identify otp fail! exit identify");
					return ret_value;
				}
			} else{
			SENSOR_PRINT("no identify_otp function!");
			}

			#endif
			sp8407_init_mode_fps_info(handle);
			sp8407_InitExifInfo();
			#if 0//defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
			sp8407_InitRawTuneInfo();
			#endif
			ret_value = SENSOR_SUCCESS;
			
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_HIGH("identify fail, pid_value = %x", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long sp8407_get_resolution_trim_tab(SENSOR_HW_HANDLE handle,uint32_t param)
{
	return (unsigned long) s_sp8407_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t sp8407_before_snapshot(SENSOR_HW_HANDLE handle,uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_sp8407_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_sp8407_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = sp8407_get_default_frame_length(handle,capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//sp8407_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//sp8407_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime ;//* BINNING_FACTOR;
/*
	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}
*/
	cap_shutter = sp8407_update_exposure(handle, cap_shutter,0);
	cap_gain = gain;
	sp8407_write_gain(handle,cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //sp8407_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //sp8407_read_gain();
	/* limit HDR capture min fps to 10;
	 * MaxFrameTime = 1000000*0.1us;
	 */
	//s_hdr_info.capture_max_shutter = 1000000/ cap_linetime;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static uint32_t sp8407_write_exposure(SENSOR_HW_HANDLE handle,unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;
    struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;

    if (!param) {
        SENSOR_PRINT_ERR("param is NULL !!!");
        return ret_value;
    }

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    mode = ex->size_index;


	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = sp8407_get_default_frame_length(handle, mode);

	s_sensor_ev_info.preview_shutter = sp8407_update_exposure(handle, exposure_line,dummy_line);

	return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(uint32_t param)
{
	uint32_t real_gain = 0;

	
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	real_gain=param;
#else
	real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1);
	real_gain = real_gain * (((param >> 5) & 0x01) + 1) * (((param >> 6) & 0x01) + 1);
	real_gain = real_gain * (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);
	real_gain = real_gain * (((param >> 9) & 0x01) + 1) * (((param >> 10) & 0x01) + 1);
	real_gain = real_gain * (((param >> 11) & 0x01) + 1);
#endif

	return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t sp8407_write_gain_value(SENSOR_HW_HANDLE handle, uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	sp8407_write_gain(handle, real_gain);

	return ret_value;
}


/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void sp8407_increase_hdr_exposure(SENSOR_HW_HANDLE handle, uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		sp8407_update_exposure(handle, s_hdr_info.capture_shutter * ev_multiplier,0);
		sp8407_write_gain(handle, s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		sp8407_update_exposure(handle, s_hdr_info.capture_shutter * shutter_multiply,0);
		sp8407_write_gain(handle, gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void sp8407_decrease_hdr_exposure(SENSOR_HW_HANDLE handle, uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		sp8407_update_exposure(handle, s_hdr_info.capture_shutter,0);
		sp8407_write_gain(handle, s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		sp8407_update_exposure(handle, shutter,0);
		sp8407_write_gain(handle, s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t sp8407_set_hdr_ev(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;
	SENSOR_PRINT("sp8407_set_hdr_ev: %d",ev);
	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		sp8407_decrease_hdr_exposure(handle, ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		sp8407_increase_hdr_exposure(handle, ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		sp8407_increase_hdr_exposure(handle, ev_multiplier);
		break;
	default:
		break;
	}
	return ret;
}

/*==============================================================================
 * Description:
 * extra functoin
 * you can add functions reference SENSOR_EXT_FUNC_CMD_E which from sensor_drv_u.h
 *============================================================================*/
static uint32_t sp8407_ext_func(SENSOR_HW_HANDLE handle, unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = sp8407_set_hdr_ev(handle, param);
		break;
	default:
		break;
	}

	return rtn;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp8407_stream_on(SENSOR_HW_HANDLE handle, uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x01);
	/*delay*/
	//usleep(10 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t sp8407_stream_off(SENSOR_HW_HANDLE handle, uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x00);
	/*delay*/
	usleep(50 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = sp8407_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_sp8407_ioctl_func_tab = {
	.power = sp8407_power_on,
	.identify = sp8407_identify,
	.get_trim = sp8407_get_resolution_trim_tab,
	.before_snapshort = sp8407_before_snapshot,
	.ex_write_exp = sp8407_write_exposure,
	.write_gain_value = sp8407_write_gain_value,
	.get_exif = sp8407_get_exif_info,
	.set_focus = sp8407_ext_func,
	//.set_video_mode = sp8407_set_video_mode,
	.stream_on = sp8407_stream_on,
	.stream_off = sp8407_stream_off,
	.cfg_otp = sp8407_access_val,	

	//.group_hold_on = sp8407_group_hold_on,
	//.group_hold_of = sp8407_group_hold_off,
};
