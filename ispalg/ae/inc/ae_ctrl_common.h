/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef _AE_CTRL_COMMON_H_
#define _AE_CTRL_COMMON_H_
#include "cmr_types.h"

struct ae_callback_param {
	cmr_s32 cur_bv;
	cmr_u32 face_stable;
	cmr_u32 face_num;
	cmr_u32 total_gain;
	cmr_u32 sensor_gain;
	cmr_u32 isp_gain;
	cmr_u32 exp_line;
	cmr_u64 exp_time;
	cmr_u32 ae_stable;
	cmr_u8 flash_fired;
	cmr_u32 cur_effect_sensitivity;
	cmr_u32 cur_effect_exp_time;
	cmr_s32 frame_number;
	float cur_effect_fps;
	void *debug_info;
	cmr_u32 debug_len;
	cmr_u32 frm_id;
};

#endif
