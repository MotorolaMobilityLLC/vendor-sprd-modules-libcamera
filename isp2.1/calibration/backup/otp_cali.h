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
#ifndef _OTP_CALI_H_
#define _OTP_CALI_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "sci_types.h"
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
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

#define GR_PATTERN_RAW 		0
#define R_PATTERN_RAW 		1
#define B_PATTERN_RAW 		2
#define GB_PATTERN_RAW 		3

#define PIXEL_CENTER        0
#define GRID_CENTER         1
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/

//calc Optical Center by pixel or grid, choose type by setting center_type
typedef cmr_s32(*FNUC_OTP_calcOpticalCenter) (void *raw_image, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 data_pattern, cmr_u32 data_type, cmr_u32 version_id, cmr_u32 grid, cmr_u32 base_gain, cmr_u32 corr_per, cmr_u32 center_type, cmr_u16 * oc_centerx, cmr_u16 * oc_centery);

OTPDLLV01_API cmr_s32 calcOpticalCenter(void *raw_image, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 data_pattern, cmr_u32 data_type, cmr_u32 version_id, cmr_u32 grid, cmr_u32 base_gain, cmr_u32 corr_per, cmr_u32 center_type, cmr_u16 * oc_centerx, cmr_u16 * oc_centery);

//get the memory size which one channel needs to malloc
typedef cmr_s32(*FNUC_OTP_getLSCOneChannelSize) (cmr_u32 grid, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 lsc_algid, cmr_u32 compress, cmr_u32 * chnnel_size);

OTPDLLV01_API cmr_s32 getLSCOneChannelSize(cmr_u32 grid, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 lsc_algid, cmr_u32 compress, cmr_u32 * chnnel_size);

//calc gains of four channels
typedef cmr_s32(*FNUC_OTP_calcLSC) (void *raw_image, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 data_pattern, cmr_u32 data_type, cmr_u32 lsc_algid, cmr_u32 compress, cmr_u32 grid, cmr_u32 base_gain, cmr_u32 corr_per, cmr_u16 * lsc_r_gain, cmr_u16 * lsc_gr_gain, cmr_u16 * lsc_gb_gain, cmr_u16 * lsc_b_gain, cmr_u32 * lsc_versionid);

OTPDLLV01_API cmr_s32 calcLSC(void *raw_image, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 data_pattern, cmr_u32 data_type, cmr_u32 lsc_algid, cmr_u32 compress, cmr_u32 grid, cmr_u32 base_gain, cmr_u32 corr_per, cmr_u16 * lsc_r_gain, cmr_u16 * lsc_gr_gain, cmr_u16 * lsc_gb_gain, cmr_u16 * lsc_b_gain, cmr_u32 * lsc_versionid);

//calc mean r/g/b value of the trim area
typedef cmr_s32(*FNUC_OTP_calcAWB) (void *raw_image, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 data_pattern, cmr_u32 data_type, cmr_u32 trim_width, cmr_u32 trim_height, int32 trim_x, int32 trim_y, cmr_u16 * awb_r_Mean, cmr_u16 * awb_g_Mean, cmr_u16 * awb_b_Mean, cmr_u32 * awb_versionid);

OTPDLLV01_API cmr_s32 calcAWB(void *raw_image, cmr_u32 raw_width, cmr_u32 raw_height, cmr_u32 data_pattern, cmr_u32 data_type, cmr_u32 trim_width, cmr_u32 trim_height, int32 trim_x, int32 trim_y, cmr_u16 * awb_r_Mean, cmr_u16 * awb_g_Mean, cmr_u16 * awb_b_Mean, cmr_u32 * awb_versionid);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
