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
#ifndef _FAE_H_
#define _FAE_H_
/*----------------------------------------------------------------------------*
 **                              Dependencies                           *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                              Compiler Flag                          *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif				/*  */
/**---------------------------------------------------------------------------*
**                               Macro Define                           *
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**                              Data Prototype                          *
**----------------------------------------------------------------------------*/
typedef struct {
	//cmr_u8 enable;
	cmr_u8 mlog_en;
	cmr_s8 debug_level;
	struct ae1_fd_param *alg_face_info;
	struct face_tuning_param face_tuning;
	cmr_u8 *ydata;
	cmr_u32 frame_ID;
	//float face_target;
	float real_target;
	float current_lum;
} fae_in;		//tuning info

typedef struct {
	cmr_s32 fd_count;
	float face_stable[25];
	cmr_s32 no_fd_count;
	cmr_s8 fidelity;
	float face_lum;
	float face_roi_lum;
	float no_face_lum;
	float tar_offset;
	float ftar_offset;	//
	float target_offset_smooth[5];
	char *log;
} fae_rt;		//calc info

typedef struct {
	cmr_u8 mlog_en;
	cmr_u8 debug_level;
	fae_in in_fae;
	fae_rt result_fae;
	/*algorithm status */
	struct ae1_face_info cur_info;	/*297 x 4bytes */
	struct ae1_face_info prv_info;	/*297 x 4bytes */
	cmr_u32 log_buf[256];
} fae_stat;

/**---------------------------------------------------------------------------*
**                              EBD Function Prototype                          *
**----------------------------------------------------------------------------*/
//cmr_s32 fae_init(fae_stat * cxt);
cmr_s32 fae_init(fae_stat * cxt, struct face_tuning_param *face_tuning_prt);
cmr_s32 fae_calc(fae_stat * fae);
cmr_s32 fae_deinit(fae_stat * cxt);

/**----------------------------------------------------------------------------*
**                                      Compiler Flag                           **
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif				/*  */
/**---------------------------------------------------------------------------*/
#endif				/*  */
