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

#ifndef _AE1_FACE_H_
#define _AE1_FACE_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
struct face_tuning_param {
	cmr_u8 face_tuning_enable;
	cmr_u8 face_target;	//except to get the face lum
	cmr_u8 face_tuning_lum1;	// scope is [0,256]
	cmr_u8 face_tuning_lum2;	//if face lum > this value, offset will set to be 0
	cmr_u16 cur_offset_weight;	//10~100 will trans 0~1
	cmr_u16 reserved[1];	//?
};
struct ae1_face {
	cmr_u8 start_x;
	cmr_u8 start_y;
	cmr_u8 end_x;
	cmr_u8 end_y;	/*1 x 4bytes */
	cmr_s32 pose;	/* face pose: frontal, half-profile, full-profile */
};

struct ae1_face_info {
	cmr_u32 face_num;	/*1 x 4bytes */
	cmr_u8 rect[1024];	/*256 x 4bytes */
	struct ae1_face face_area[20];	/*20 x 2 x 4bytes */
};			/*297 x 4bytes */

struct ae1_fd_param {
	struct ae1_face_info cur_info;	/*297 x 4bytes */
	//struct ae1_face_info pre_info;/*297 x 4bytes*/
	cmr_u8 update_flag;
	cmr_u8 enable_flag;
	cmr_u16 reserved;	/*1 x 4bytes */
};			/*595 x 4bytes */
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
