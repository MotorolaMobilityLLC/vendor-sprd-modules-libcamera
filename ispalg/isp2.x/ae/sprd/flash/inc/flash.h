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
typedef  unsigned short uint16;
//typedef unsigned long uint32;
#define uint32 unsigned int
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
							/*2 * 4bytes*/
	uint32 preflash_ct;/*4bytes*/

	bool flashMask[1024];/*256 * 4bytes*/
	uint16 brightnessTable[1024];/*512 * 4bytes*/
	uint16 rTable[1024]; //g: 1024/*512 * 4bytes*/
	uint16 bTable[1024];/*512 * 4bytes*/
	uint8 reserved1[1020];/*255 * 4bytes*/
};/*2053 * 4 bypes*/

struct Flash_initInput
{
	uint8 debug_level;
	uint32 statW;/*stat block num in Width dir*/
	uint32 statH;/*stat block num in Height dir*/
	float ctTabRg[20];
	float ctTab[20];
	struct flash_tune_param *tune_info;/*flash algorithm tune param*/
};

struct Flash_initOut
{
	uint8 version;
};


enum Flash_flickerMode
{
	
	flash_flicker_50hz=0,
	flash_flicker_60hz,

};
struct Flash_pfStartInput
{
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
	uint16 *rSta; //10bit
	uint16 *gSta;
	uint16 *bSta;
};

struct Flash_pfStartOutput
{
	float nextExposure;
	uint32 nextGain;
	bool nextFlash;
	uint8 preflahLevel1;
	uint8 preflahLevel2;
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
	uint16 captureFlahLevel1;
	uint16 captureFlahLevel2;

	//next exposure
	float nextExposure;
	uint32 nextGain;
	bool nextFlash;
	bool isEnd;

	uint32 debugSize;
	void* debugBuffer;
};

int flash_get_tuning_param(struct flash_tune_param* param);/*It will be removed in the future*/
flash_handle flash_init(struct Flash_initInput* in, struct Flash_initOut *out);
int flash_deinit(flash_handle handle);
int flash_pfStart(flash_handle handle, struct Flash_pfStartInput* in, struct Flash_pfStartOutput* out);
int flash_pfOneIteration(flash_handle handle, struct Flash_pfOneIterationInput* in, struct Flash_pfOneIterationOutput* out);
int flash_pfEnd(flash_handle handle);

#ifdef __cplusplus
}
#endif
#endif//_FLASH_H_
