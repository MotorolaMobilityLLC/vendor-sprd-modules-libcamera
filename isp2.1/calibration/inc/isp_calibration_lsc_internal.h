/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 #ifndef _ISP_CALIBRATION_LSC_INTERNAL_H_
#define _ISP_CALIBRATION_LSC_INTERNAL_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "isp_calibration_lsc.h"

#ifdef WIN32
#include <stdio.h>
#define PRINTF printf
#else
#include "isp_type.h"
#define LSC_DEBUG_STR     "OTP LSC: %d, %s: "
#define LSC_DEBUG_ARGS    __LINE__,__FUNCTION__
#define PRINTF(format,...) ALOGE(LSC_DEBUG_STR format, LSC_DEBUG_ARGS, ##__VA_ARGS__)
#endif






/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
#define CHANNAL_NUM 4
#define LSC_GOLDEN_MIN_BLOCK 	2
#define LSC_GOLDEN_MAX_BLOCK 	16
#define LSC_GOLDEN_VERSION		1

#define BLOCK_ID_BASIC		0x00020001
#define BLOCK_ID_STD_GAIN	0x00020002
#define BLOCK_ID_DIFF_GAIN	0x00020003

struct lsc_golden_header {
	cmr_u32 version;
	cmr_u32 length;
	cmr_u32 block_num;
};

struct lsc_golden_basic_info {
	cmr_u16 base_gain;
	cmr_u16 algorithm_version;
	cmr_u16 compress_flag;
	cmr_u16 image_width;
	cmr_u16 image_height;
	cmr_u16 gain_width;
	cmr_u16 gain_height;
	cmr_u16 optical_x;
	cmr_u16 optical_y;
	cmr_u16 grid_width;
	cmr_u16 grid_height;
	cmr_u16 percent;
	cmr_u16 bayer_pattern;
	cmr_u16 reserved;
};

struct lsc_golden_block_info {
	cmr_u32 id;
	cmr_u32 offset;
	cmr_u32 size;
};

struct lsc_golden_gain_info {
	cmr_u32 ct;
	cmr_u32 gain_num;
	void *gain;
};

struct lsc_random_info {
	cmr_u32 version;
	cmr_u16 data_length;
	cmr_u16 algorithm_version;
	cmr_u16 compress_flag;
	cmr_u16 image_width;
	cmr_u16 image_height;
	cmr_u16 gain_width;
	cmr_u16 gain_height;
	cmr_u16 optical_x;
	cmr_u16 optical_y;
	cmr_u16 grid_width;
	cmr_u16 grid_height;
	cmr_u16 percent;
	cmr_u16 bayer_pattern;
	cmr_u16 gain_num;
	cmr_u16 *gain;
};
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
enum isp_calibration_lsc_random_cali_info_table{
	R_VERSION_ID = 0,
	R_IMG_WIDTH = 2,
	R_IMG_HEIGHT = 4,
	R_DATALEN = 6,
	R_CENTER_X = 8,
	R_CENTER_Y = 10,
	R_STANDARD = 12
};

enum isp_calibration_lsc_golden_cali_info_table{
	G_VERSION_LOW_16BITS = 0,
	G_VERSION_HIGH_16BITS = 2,
	G_IMG_WIDTH = 4,
	G_IMG_HEIGHT = 6,
	G_DATALEN_LOW_16BITS = 8,
	G_DATALEN_HIGH_16BITS = 10,
	G_SUB_VERSION_ID = 12,
	G_MODULE_NUM = 16,
	G_DATALEN = 20,
	G_MODULE_INFO = 24,
	G_MODULE_INFO_DELTA = 12,

	G_GRID_SIZE_MASK = 0xfe00,
	G_GRID_SIZE_SHIFT = 9,
	G_COMPRESS_FLAG_MASK = 0x0100,
	G_COMPRESS_FLAG_SHIFT = 8,
	G_DIM_FLAG_MASK = 0x0080,
	G_DIM_FLAG_SHIFT = 7,
	G_MODULE_INFO_MASK = 0x0003,
	G_MODULE_INFO_SHIFT = 0,


	G_PARA_INFO = 0,
	G_STANDARD_INFO = 1,

	G_STANDARD_DATA = 0,
	G_DIFF_DATA = 1,

	G_STANDARDOFFSET = 1,
	G_DIFFOFFSET = 2,

	G_LSC_MODULE = 0x01,
	G_COMPRESS = 1,
	G_NONCOMPRESS = 0,
	G_DIM_1D = 1,
	G_DIM_2D = 2

};

enum isp_calibration_lsc_calc_out_table{
	OUT_STANDARD = 0,
	OUT_NONSTANDARD = 1,
};


struct isp_calibration_lsc_random_cali_info{
	cmr_u16 img_width;
	cmr_u16 img_height;
	cmr_u16 center_x;
	cmr_u16 center_y;
	cmr_u16* std_gain;
	cmr_u32 lsc_pattern;
	cmr_u32 compress;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
};

struct isp_calibration_lsc_info_from_golden{
	/*color temperature of the light*/
	cmr_u32 standard_light_ct;
	cmr_u8 grid_size;
	cmr_u8 diff_num;
	cmr_u16 grid_width;
	cmr_u16 grid_height;
	cmr_u32 std_gain_golden_size;
	cmr_u16* std_gain_golden;
};

struct isp_calibration_lsc_flags{
	cmr_u32 version;
	cmr_u32 compress_flag;
	cmr_u32 alg_type;		// 1: 1D; 2: 2D
	cmr_u32 alg_version;
	cmr_u32 base_gain;
	cmr_u32 percent;		//correction percent
};

struct isp_calibration_lsc_module_info{
	cmr_u32 id;
	cmr_u32 datalen;
	cmr_u32 offset;
};

struct isp_calibration_lsc_predictor_param{
	cmr_u16* std_gain;
	cmr_u16* diff;
	cmr_u16* nonstd_gain;
	cmr_u32 difflen;// eyery channel keep the same difflen,so the data in diffptr keep the length of 4*difflen
	cmr_u16 grid_width;
	cmr_u16 grid_height;
	cmr_u16 center_x;
	cmr_u16 center_y;
};

struct isp_calibration_lsc_golden_cali_info{
	cmr_u16 img_width;
	cmr_u16 img_height;
	cmr_u8 grid_size;
	cmr_u16 center_x;
	cmr_u16 center_y;
	cmr_u16 grid_width;
	cmr_u16 grid_height;
	cmr_u8 diff_num;
	struct isp_calibration_lsc_param gain[ISP_CALIBRATION_MAX_LSC_NUM];// includes the standard data and nonstandard data
};

cmr_s32 isp_calibration_lsc_golden_parse(void *golden_data, cmr_u32 golden_size,
					struct isp_calibration_lsc_flags *flag,
					struct isp_calibration_lsc_golden_cali_info *golden_info);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End
