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
 #ifndef _OTP_PACK_H_
#define _OTP_PACK_H_

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the OTPDLLV01_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// OTPDLLV01_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef OTPDLLV01_EXPORTS
#define OTPDLLV01_API __declspec(dllexport)
#else
#define OTPDLLV01_API __declspec(dllimport)
#endif

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "sci_types.h"
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
#define MAX_NONSTD_IMAGE 9
#define LSC_CHN_NUM 4
#define DIFF_FIXPOINT 10
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct otp_pack_module_param {
	cmr_u32 core_version;
	cmr_u32 sensor_maker;
	cmr_u32 year;
	cmr_u32 month;
	cmr_u32 module_version;
	cmr_u32 release_number;
	cmr_u32 cal_dll_version;
	cmr_u32 cal_map_version;
};


struct otp_pack_lsc_param {
	/* 1: tshark algorithm; 2: sharkl/tshark2 algorithm  */
	cmr_u32 alg_version;
	/* 1: use 1d diff; 2: use 2d diff */
	cmr_u32 alg_type;
	/* 0: normal gain (16 bits for one gain); 1: compress gain (14 bit for one gain)  */
	cmr_u32 compress;
	/* correction percent: 1-100 */
	cmr_u32 percent;
	/*lsc grid size*/
	cmr_u32 grid_width;
	cmr_u32 grid_height;
	/*0: physcial type; 1: optical type*/
	cmr_u32 center_type;
};

struct otp_pack_awb_param {
	/*awb algorithm version, must be 1*/
	cmr_u32 alg_version;
	/*region of interest*/
	cmr_u32 roi_x;
	cmr_u32 roi_y;
	cmr_u32 roi_width;
	cmr_u32 roi_height;
};

struct otp_pack_golden_param {
	struct otp_pack_module_param module;
	struct otp_pack_lsc_param lsc;
	struct otp_pack_awb_param awb;

	cmr_u32 base_gain;

	/*image basic info*/
	cmr_u32 image_width;
	cmr_u32 image_height;
	/*0: gr, 1: r, 2: b, 3: gb*/
	cmr_u32 image_pattern;
	/*0: raw image; 1: mipi raw image*/
	cmr_u32 image_format;

	/*standard image*/
	void *std_img;
	cmr_u32 std_ct;

	/*nonstandard image*/
	void *nonstd_img[MAX_NONSTD_IMAGE];
	cmr_u32 nonstd_ct[MAX_NONSTD_IMAGE];
	cmr_u32 nonstd_num;

	/*buffer to receive output data*/
	void *target_buf;
	cmr_u32 target_buf_size;
};

struct otp_pack_random_param {
	struct otp_pack_lsc_param lsc;
	struct otp_pack_awb_param awb;

	cmr_u32 base_gain;

	/*standard image basic info*/
	cmr_u32 image_width;
	cmr_u32 image_height;
	/*0: gr, 1: r, 2: b, 3: gb*/
	cmr_u32 image_pattern;
	/*0: raw image; 1: mipi raw image*/
	cmr_u32 image_format;
	void *image_data;
	cmr_u32 image_ct;

	/*buffer to receive output data*/
	void *target_buf;
	cmr_u32 target_buf_size;
};

/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/
OTPDLLV01_API cmr_s32 otp_pack_golden(struct otp_pack_golden_param *param, cmr_u32 *real_size);
OTPDLLV01_API cmr_s32 otp_golden_size(struct otp_pack_golden_param *param, cmr_u32 *size);
OTPDLLV01_API cmr_s32 otp_pack_random(struct otp_pack_random_param *param, cmr_u32 *real_size);
OTPDLLV01_API cmr_s32 otp_random_size(struct otp_pack_random_param *param, cmr_u32 *size);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End