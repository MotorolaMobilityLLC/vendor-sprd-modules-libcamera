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
#ifndef _STAT_BASIC_H_
#define _STAT_BASIC_H_

#ifdef CONFIG_FOR_TIZEN
#include "stdint.h"
#elif WIN32
#include "ae_porting.h"
#else
#include "ae_types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	cmr_s16 top_range;	//range of bigger value
	cmr_s16 data_len;
	cmr_s8 debug_level;
	cmr_s16 bright_thr;
	cmr_s16 dark_thr;
} basic_in;		//tuning info

typedef struct {
	cmr_s16 histogram[256];
	cmr_s16 media_1;
	cmr_s16 media_2;
	float mean;
	cmr_s16 top;	//average of bigger value
	cmr_s16 maximum;
	cmr_s16 minimum;
	float var;
	cmr_s16 bright_num;
	cmr_s16 dark_num;
	cmr_s16 normal_num;
} basic_rt;		//calc info

typedef struct {
	basic_in in_basic;
	basic_rt result_basic;
} basic_stat;		//statistic information of single channel

cmr_s32 initbasic(basic_stat * basic, cmr_u32 debug_level);
cmr_s32 deinitbasic(basic_stat * basic);
cmr_s32 calcbasic(basic_stat * basic, cmr_s32 len, cmr_u8 * data);
cmr_s32 round_ae(float iodata);

#ifdef __cplusplus
}
#endif

#endif
