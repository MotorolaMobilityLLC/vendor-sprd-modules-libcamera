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
#ifndef _SENSOR_RAW_H_
#define _SENSOR_RAW_H_

enum ISP_BLK_ID {
	ISP_BLK_SHARKL2_BASE = 0x4000,
	ISP_BLK_PRE_GBL_GAIN = 0x4001,
	ISP_BLK_BLC = 0x4002,
	ISP_BLK_PDAF_EXTRACT = 0x4003,
	ISP_BLK_PDAF_CORRECT = 0x4004,
	ISP_BLK_NLM = 0x4005,		// + ISP_BLK_VST + ISP_BLK_IVST
	ISP_BLK_POSTBLC = 0x4006,
	ISP_BLK_RGB_GAIN = 0x4007,
	ISP_BLK_RGB_DITHER = 0x4008,	//RGBgain yrandom
	ISP_BLK_NLC = 0x4009,
	ISP_BLK_2D_LSC = 0x400A,
	ISP_BLK_1D_LSC = 0x400B,	//radial lens
	ISP_BLK_BINNING4AWB = 0x400C,
	ISP_BLK_AWBC = 0x400D,
	ISP_BLK_RGB_AEM = 0x400E,
	ISP_BLK_BPC = 0x400F,
	ISP_BLK_GRGB = 0x4010,
	ISP_BLK_CFA = 0x4011,		//include CFAI intplt and CFAI CSS
	ISP_BLK_RGB_AFM = 0x4012,
	ISP_BLK_CMC10 = 0x4013,
	ISP_BLK_RGB_GAMC = 0x4014,
	ISP_BLK_HSV = 0x4015,
	ISP_BLK_POSTERIZE = 0x4016,
	ISP_BLK_CCE = 0x4017,
	ISP_BLK_UVDIV = 0x4018,
	ISP_BLK_YIQ_AFL_V1 = 0x4019,
	ISP_BLK_YIQ_AFL_V3 = 0x401A,
	ISP_BLK_3DNR_PRE = 0x401B,
	ISP_BLK_3DNR_CAP = 0x401C,
	ISP_BLK_YUV_PRECDN = 0x401D,
	ISP_BLK_YNR = 0x401E,
	ISP_BLK_BRIGHT = 0x401F,
	ISP_BLK_CONTRAST = 0x4020,
	ISP_BLK_HIST = 0x4021,
	ISP_BLK_HIST2 = 0x4022,
	ISP_BLK_EDGE = 0x4023,
	ISP_BLK_Y_GAMMC = 0x4024,
	ISP_BLK_UV_CDN = 0x4025,
	ISP_BLK_UV_POSTCDN = 0x4026,
	ISP_BLK_Y_DELAY = 0x4027,
	ISP_BLK_SATURATION = 0x4028,
	ISP_BLK_HUE = 0x4029,
	ISP_BLK_IIRCNR_IIR = 0x402A,
	ISP_BLK_IIRCNR_YRANDOM = 0x402B,
	ISP_BLK_YUV_NOISEFILTER = 0X402C,
//3A and smart
	ISP_BLK_SMART = 0x402D,
	ISP_BLK_SFT_AF = 0x402E,
	ISP_BLK_ALSC = 0x402F,
	ISP_BLK_AFT = 0x4030,
	ISP_BLK_AE_NEW = 0X4031,
	ISP_BLK_AWB_NEW = 0x4032,
	ISP_BLK_AF_NEW = 0x4033,
//Flash and envi
	ISP_BLK_FLASH_CALI = 0x4034,
	ISP_BLK_ENVI_DETECT = 0x4035,
	ISP_BLK_DUAL_FLASH = 0x4036,
// DCAM raw block
	DCAM_BLK_BLC = 0x4037,
	DCAM_BLK_RAW_AEM = 0x4038,
	DCAM_BLK_2D_LSC = 0x4039,
	ISP_BLK_BOKEH_MICRO_DEPTH = 0x403A,
	ISP_BLK_ANTI_FLICKER = 0x403B,
	ISP_BLK_PDAF_TUNE = 0x403C,
	ISP_BLK_ATM_TUNE = 0x403D,
	ISP_BLK_TOF_TUNE = 0x403E,

	// pike2
	ISP_BLK_BDN = 0x4051,
	ISP_BLK_RGB_PRECDN = 0x4052,
	ISP_BLK_CSS = 0x4053,
	ISP_BLK_UV_PREFILTER = 0x4054,	// ISP_BLK_PREF , prfy

	// sharkl3
	DCAM_BLK_PDAF_EXTRACT = 0x5000,
	DCAM_BLK_BPC = 0x5001,
	DCAM_BLK_RGB_AFM = 0x5002,
	DCAM_BLK_NLM = 0x5003,
	DCAM_BLK_3DNR_PRE = 0x5004,
	DCAM_BLK_3DNR_CAP = 0x5005,
	ISP_BLK_CNR2 = 0x5006,
	ISP_BLK_AE_SYNC = 0x5007,
	ISP_BLK_4IN1_PARAM = 0x5008,
	ISP_BLK_AE_ADAPT_PARAM = 0x5009,
	//sharkl5
	//DCAM Block
	DCAM_BLK_RGB_DITHER = 0x5051,
	DCAM_BLK_PPE = 0x5052,
	DCAM_BLK_HIST = 0x5053,
	DCAM_BLK_BPC_V1 = 0x5054,
	DCAM_BLK_RGB_AFM_V1 = 0x5055,

	//ISP Block
	ISP_BLK_GRGB_V1 = 0x5056,
	ISP_BLK_NLM_V1 = 0x5057,
	ISP_BLK_IMBALANCE = 0x5058,
	ISP_BLK_CFA_V1 = 0x5059,
	ISP_BLK_UVDIV_V1 = 0x505A,
	ISP_BLK_3DNR = 0x505B,
	ISP_BLK_LTM = 0x505C,
	ISP_BLK_YUV_PRECDN_V1 = 0x505D,
	ISP_BLK_UV_CDN_V1 = 0x505E,
	ISP_BLK_UV_POSTCDN_V1 = 0x505F,
	ISP_BLK_YNR_V1 = 0x5060,
	ISP_BLK_EE_V1 = 0x5061,
	ISP_BLK_IIRCNR_IIR_V1 = 0x5062,
	ISP_BLK_YUV_NOISEFILTER_V1 = 0x5063,
	ISP_BLK_CNR2_V1 = 0x5064,
	ISP_BLK_BCHS = 0x5065,
	ISP_BLK_EXT,
	ISP_BLK_ID_MAX,
};

enum isp_smart_id {
	ISP_SMART_LNC = 0,
	ISP_SMART_COLOR_CAST = 1,
	ISP_SMART_CMC = 2,
	ISP_SMART_SATURATION_DEPRESS = 3,
	ISP_SMART_HSV = 4,
	ISP_SMART_GAMMA = 5,
	ISP_SMART_GAIN_OFFSET = 6,
	ISP_SMART_BLC = 7,
	ISP_SMART_POSTBLC = 8,
//NR
	ISP_SMART_PDAF_CORRECT = 9,
	ISP_SMART_NLM = 10,
	ISP_SMART_RGB_DITHER = 11,
	ISP_SMART_BPC = 12,
	ISP_SMART_GRGB = 13,
	ISP_SMART_CFAE = 14,
	ISP_SMART_RGB_AFM = 15,
	ISP_SMART_UVDIV = 16,
	ISP_SMART_3DNR_PRE = 17,
	ISP_SMART_3DNR_CAP = 18,
	ISP_SMART_EDGE = 19,
	ISP_SMART_YUV_PRECDN = 20,
	ISP_SMART_YNR = 21,
	ISP_SMART_UVCDN = 22,
	ISP_SMART_UV_POSTCDN = 23,
	ISP_SMART_IIRCNR_IIR = 24,
	ISP_SMART_IIR_YRANDOM = 25,
	ISP_SMART_YUV_NOISEFILTER = 26,
	ISP_SMART_RGB_PRECDN = 27,
	ISP_SMART_BDN = 28,
	ISP_SMART_PRFY = 29,		// UV_PREFILTER
	ISP_SMART_CNR2 = 30,
	ISP_SMART_IMBALANCE = 31,
	ISP_SMART_LTM = 32,
	ISP_SMART_3DNR = 33,
	ISP_SMART_MAX
};

#ifdef CONFIG_ISP_2_4
#include "sensor_raw_pike2.h"
#elif defined CONFIG_ISP_2_5
#include "sensor_raw_sharkl3.h"
#elif defined CONFIG_ISP_2_6
#include "sensor_raw_sharkl5.h"
#else
#include "sensor_raw_isp2.1.h"
#endif

#endif
