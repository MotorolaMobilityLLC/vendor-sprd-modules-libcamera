/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _MULAES_H_
#define _MULAES_H_
/*----------------------------------------------------------------------------*
 **                              Dependencies                           *
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
#include "basic_stat.h"
/**---------------------------------------------------------------------------*
 **                              Compiler Flag                          *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"  {
#endif/*  */
/**---------------------------------------------------------------------------*
**                               Macro Define                           *
**----------------------------------------------------------------------------*/
#define MULAES_CFG_NUM AE_CFG_NUM
/**---------------------------------------------------------------------------*
**                              Data Prototype                                                                                  *
**----------------------------------------------------------------------------*/
	struct mulaes_cfg {
		cmr_s16 x_idx;
		cmr_s16 y_lum;
	};

	struct mulaes_tuning_param {
		cmr_u8 enable;
		cmr_u8 num;
		cmr_u16 reserved;	/*1 * 4bytes */
		struct mulaes_cfg cfg[MULAES_CFG_NUM];/*8 * 4bytes */
	};/*9 * 4bytes */

	typedef struct {
		cmr_u8 mlog_en;
		cmr_s16 effect_idx;
		cmr_s16 match_lv;
		float real_target;
	} mulaes_in;//tuning info

	typedef struct {
		float artifact_tar;
		cmr_s16 tar_offset;
		char *log;
	} mulaes_rt;//result info

	typedef struct {
		cmr_s8 enable;
		cmr_s8 debug_level;
		cmr_s8 mlog_en;
		cmr_s8 reserved;
		struct mulaes_tuning_param tune_param;
		cmr_s16 dynamic_table[600];
		mulaes_in in_mulaes;
		mulaes_rt result_mulaes;
		cmr_u32 log_buf[256];
	} mulaes_stat;

/**---------------------------------------------------------------------------*
**				Function Defination 		*
**---------------------------------------------------------------------------*/
	cmr_s32 mulaes_init(mulaes_stat * cxt, struct mulaes_tuning_param *tune_param_ptr);
	cmr_s32 mulaes_calc(mulaes_stat * cxt);
	cmr_s32 mulaes_deinit(mulaes_stat * cxt);

/**----------------------------------------------------------------------------*
**                                      Compiler Flag                           **
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
} 
#endif/*  */
/**---------------------------------------------------------------------------*/
#endif/*  */
