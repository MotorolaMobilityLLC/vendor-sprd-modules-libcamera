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
#ifndef _DEFLICKER_H_
#define _DEFLICKER_H_

#ifdef __cplusplus
extern "C" {
#endif

cmr_s32 antiflcker_sw_init();
cmr_s32 antiflcker_sw_deinit();
cmr_s32 antiflcker_sw_process(cmr_s32 input_img_width, cmr_s32 input_img_height, cmr_s32 * debug_sat_img_H_scaling,
			      cmr_s32 exposure_time, cmr_s32 reg_mflicker_frame_thrd, cmr_s32 reg_mflicker_video_thrd,
			      cmr_s32 reg_sflicker_frame_thrd, cmr_s32 reg_mflicker_long_thrd, cmr_s32 reg_flat_thrd, cmr_s32 reg_flat_count_thrd, cmr_s32 reg_fflicker_frame_thrd, cmr_s32 reg_fflicker_video_thrd, cmr_s32 reg_fflicker_length_thrd, cmr_s32 * R_window, cmr_s32 * G_window, cmr_s32 * B_window);

#ifdef __cplusplus
}
#endif
#endif
