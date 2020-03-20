/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _FLASH_H_
#define _FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <stdbool.h>

#ifdef WIN32
typedef  unsigned char uint8;
typedef  short int16;
typedef  unsigned short uint16;
//typedef unsigned long uint32;
#define uint32 unsigned int
#define int32 int
#else
typedef  unsigned char uint8;
typedef  unsigned short uint16;
typedef  unsigned int uint32;

typedef  signed char int8;
typedef  signed short int16;
typedef  signed int int32;
#endif

typedef void* flash_handle;

struct flash_tune_param {
	uint8 version;/*version 0: just for old flash controlled by AE algorithm, and Dual Flash must be 1*/
	uint8 alg_id;
	uint8 flashLevelNum1;
	uint8 flashLevelNum2;/*1 * 4bytes*/

	uint8 preflahLevel1;
	uint8 preflahLevel2;
	uint16 preflashBrightness;/*1 * 4bytes*/

	uint16 brightnessTarget; //10bit
	uint16 brightnessTargetMax; //10bit
								/*1 * 4bytes*/

	uint32 foregroundRatioHigh;/*fix data: 1x-->100*/
	uint32 foregroundRatioLow;/*fix data: 1x-->100*/

	bool flashMask[1024];/*256 * 4bytes*/
	uint16 brightnessTable[1024];/*512 * 4bytes*/
	uint16 rTable[1024]; //g: 1024/*512 * 4bytes*/
	uint16 bTable[1024];/*512 * 4bytes*/
	uint32 preflash_ct;/*4bytes*/
	int16 driverId1[32];
	int16 driverId2[32];
	int16 preflahLevel1_new;
	int16 preflahLevel2_new;

	//v5
	float rgTab[20];
	float ctTab[20];
	//uint8 reserved1[60]; /*255 * 4bytes*/
	int16 otp_golden_r_cali;
	int16 otp_golden_g_cali;
	int16 otp_golden_b_cali;
	int16 otp_random_r_cali;
	int16 otp_random_g_cali;
	int16 otp_random_b_cali;
	int16 otp_golden_r_ct;
	int16 otp_golden_g_ct;
	int16 otp_golden_b_ct;
	int16 otp_random_r_ct;
	int16 otp_random_g_ct;
	int16 otp_random_b_ct;
	int16 main_flash_en; //0: main flash awb¤£°µ
	int16 with_ct_tab;  //1: tuning with ct table,  0: ct table from awb
	uint32 max_pf_shutter; //0: from ae, others: assign max preflash shutter by tuning data
	//v5 end

	uint32 alg_id_sub;

	//
	uint32 version_cali;
	uint32 version_sub_cali;
	uint32 alg_id_preParam;
	uint32 alg_id_sub_preParam;

	///

	int16 preflash_max_ma;
	int16 mainflash_max_ma;

	int16 driverIndexP1[32];
	int16 driverIndexP2[32];
	uint16 maP1[32];
	uint16 maP2[32];
	uint16 maM1[32];
	uint16 maM2[32];
	uint16 numP1_hwSample;
	uint16 numP2_hwSample;
	uint16 numM1_hwSample;
	uint16 numM2_hwSample;
	uint16 mAMaxP1;
	uint16 mAMaxP2;
	uint16 mAMaxP12;
	uint16 mAMaxM1;
	uint16 mAMaxM2;
	uint16 mAMaxM12;

	float rgRatPf;
	uint16 rPf1[32];
	uint16 gPf1[32];
	uint16 rPf2[32];
	uint16 gPf2[32];
	uint32 awbWithCtTab;

	uint32 multiColorLcdEn;
	float capGainRMul;
	float capGainBMul;
	uint32 maxMfShutter;
	uint32 customFlashIntensityEn;
	int32 flashIntensityBv1;
	int32 flashIntensityBv2;
	int16 flashIntensityLevel1;
	int16 flashIntensityLevel2;
	uint8 reserved1[1760];
};/*2500 * 4 bypes*/


struct Flash_initInput
{
	uint8 debug_level;
	uint32 statW;/*stat block num in Width dir*/
	uint32 statH;/*stat block num in Height dir*/
#if defined(__i386)
	uint32 tmpForPack1;
#endif
	double tmp4pack;
	struct flash_tune_param *tune_info;/*flash algorithm tune param*/
#if defined(__i386)
	uint32 tmpForPack2;
#endif
};

struct Flash_initOut
{
	uint8 version;
	uint8 mainFlashEn;
};


enum Flash_flickerMode
{

	flash_flicker_50hz=0,
	flash_flicker_60hz,

};

#define AE_EXP_GAIN_TABLE_SIZE 512

struct ae_exp_gain_table_copy {
	int32 min_index;
	int32 max_index;
	uint32 exposure[AE_EXP_GAIN_TABLE_SIZE]; //exposure_line
	uint32 dummy[AE_EXP_GAIN_TABLE_SIZE];/*unused*/
	uint16 again[AE_EXP_GAIN_TABLE_SIZE]; //128->1x
	uint16 dgain[AE_EXP_GAIN_TABLE_SIZE]; /*unused*/
};


struct Flash_pfStartInput
{
	float ctTabRg[20];
	float ctTab[20];
	float minExposure;
	float maxExposure;
	uint32 minGain;  //x128
	uint32 maxGain;  //x128
	float minCapExposure;
	float maxCapExposure;
	uint32 minCapGain;  //x128
	uint32 maxCapGain;  //x128
	uint32 rGain;  //x1024
	uint32 gGain;  //x1024
	uint32 bGain;  //x1024
	float aeExposure; //unit  0.1us
	uint32 aeGain; //unit? x128
	bool isFlash;   //no flash, 0
	uint8 flickerMode; //50hz:0, 60hz:1
	uint16 staW;
	uint16 staH;
	double tmp4pack;
	uint16 *rSta; //10bit
	double tmp4pack2;
	uint16 *gSta;
	double tmp4pack3;
	uint16 *bSta;
	uint32 otp_gldn_r;
	uint32 otp_gldn_g;
	uint32 otp_gldn_b;
	uint32 otp_rdm_r;
	uint32 otp_rdm_g;
	uint32 otp_rdm_b;
#if defined(__i386)
	uint32 tmp4pack4[4];
#endif
	int32 bv;
	uint32 expsure_line_ns;
	void* ae_table;
};

struct Flash_pfStartOutput
{
	float nextExposure;
	uint32 nextGain;
	bool nextFlash;
	int16 preflahLevel1;
	int16 preflahLevel2;
};

struct Flash_pfOneIterationInput
{
	float aeExposure;
	uint32 aeGain;
	bool isFlash; //with flash:1
	uint16 staW;
	uint16 staH;
	uint16 rSta[1024];
	uint16 gSta[1024];
	uint16 bSta[1024];
};

struct Flash_pfOneIterationOutput
{
	//for capture parameters
	float captureExposure;
	uint32 captureGain;
	float captureFlashRatio; //0-1
	float captureFlash1Ratio; //0-1
	uint16 captureRGain;
	uint16 captureGGain;
	uint16 captureBGain;
	int16 captureFlahLevel1;
	int16 captureFlahLevel2;

	//next exposure
	float nextExposure;
	uint32 nextGain;
	bool nextFlash;
	bool isEnd;

	uint32 debugSize;
#if defined(__i386)
	uint32 tmp4pack2;
#endif
	double tmp4pack;
	void* debugBuffer;
#if defined(__i386)
	uint32 tmp4pack3;
#endif
};


struct Flash_mfCalcInput
{
	uint16 staW;
	uint16 staH;
	uint16 rSta[1024];
	uint16 gSta[1024];
	uint16 bSta[1024];
	uint32 rGain;
	uint32 gGain;
	uint32 bGain;
	uint32 wb_mode;
};

struct Flash_mfCalcOutput
{
	uint16 isEnd;
	uint16 captureRGain;
	uint16 captureGGain;
	uint16 captureBGain;
};

int flash_get_tuning_param(struct flash_tune_param* param);/*It will be removed in the future*/
flash_handle flash_init(struct Flash_initInput* in, struct Flash_initOut *out);
int flash_deinit(flash_handle handle);
int flash_pfStart(flash_handle handle, struct Flash_pfStartInput* in, struct Flash_pfStartOutput* out);
int flash_pfOneIteration(flash_handle handle, struct Flash_pfOneIterationInput* in, struct Flash_pfOneIterationOutput* out);
int flash_mfCalc(flash_handle handle, struct Flash_mfCalcInput* in, struct Flash_mfCalcOutput* out);
int flash_pfEnd(flash_handle handle);

#ifdef __cplusplus
}
#endif
#endif//_FLASH_H_
