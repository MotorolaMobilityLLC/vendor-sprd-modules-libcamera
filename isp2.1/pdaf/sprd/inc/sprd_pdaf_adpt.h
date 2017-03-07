/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _SPRD_PDAF_ADPT_H
#define _SPRD_PDAF_ADPT_H

#include <utils/Timers.h>
#include <pthread.h>
#include "pd_algo.h"


#define IMAGE_WIDTH 4106
#define IMAGE_HEIGHT 3172
#define BEGIN_X 26
#define BEGIN_Y 26
#define ROI_X 1048
#define ROI_Y 792
#define ROI_Width 2048
#define ROI_Height 1536
#define AREA_LOOP 4
#define SENSOR_ID 0
#define PD_REG_OUT_SIZE	352
#define PD_OTP_PACK_SIZE	550

typedef struct{
	short int   m_wLeft;
	short int   m_wTop;
	short int   m_wWidth;
	short int   m_wHeight;
} alGE_RECT;

typedef struct {
	short int   m_wLeft;
	short int   m_wTop;
	short int   m_wWidth;
	short int   m_wHeight;
} alPD_RECT;

static uint16_t Reg[256];

typedef struct
{
	int dSensorID;//0:for SamSung 1: for Sony
	double uwInfVCM;
	double uwMacroVCM;
}SensorInfo;


typedef struct {
	/*sensor information*/
	SensorInfo tSensorInfo;
	int dcurrentVCM;
	// BV value
	float dBv;
	// black offset
	int dBlackOffset;
	unsigned char ucPrecision;
} PDInReg;

struct pdaf_timestamp {
	uint32_t sec;
	uint32_t usec;
};

struct sprd_pdaf_report_t {
	uint32_t token_id;
	uint32_t frame_id;
	char enable;
	struct pdaf_timestamp time_stamp;
	float pd_value;
	void *pd_reg_out;
	uint32_t pd_reg_size;
};

struct pd_result {
	/*TBD get reset from */
	int pdConf[AREA_LOOP+1];
	double pdPhaseDiff[AREA_LOOP+1];
	int pdGetFrameID;
};

#endif
