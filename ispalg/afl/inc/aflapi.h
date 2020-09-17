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
#ifndef _AFLAPI_H_
#define _AFLAPI_H_

typedef void* afl_handle_t;

typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t micro;
	uint8_t nano;
	char built_date[0x20];
	char built_time[0x20];
	char built_rev[0x100];
} afl_version_t;

struct afl_input_image_size {
	cmr_u32 width;
	cmr_u32 height;
};

struct afl_ev_setting_t
{
	cmr_u32 max_fps;
	cmr_u32 app_mode;
	cmr_u32 cameraId;
	cmr_u32 cur_flicker;
	cmr_u32 cur_exp_flag;
	cmr_s32 ae_exp_flag;
	cmr_int bypass;
	struct afl_input_image_size input_image_size;
};


#ifdef __cplusplus
extern "C" {
#endif

int AFL_GetVersion(struct afl_version_t *o_version);
int AFL_CreateHandle(afl_handle_t *handle);
int AFL_DeleteHandle(afl_handle_t *handle);
int AFL_Process(afl_handle_t handle, int *debug_sat_img_H_scaling, int exposure_time, cmr_s32 *thr, int *window, struct afl_ev_setting_t ev_setting);

#ifdef __cplusplus
}
#endif

#endif
