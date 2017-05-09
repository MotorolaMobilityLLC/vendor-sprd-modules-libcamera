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
#ifndef _RANDOM_UNPACK_H_
#define _RANDOM_UNPACK_H_

#include "random_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

struct random_info {
	void *awb;
	cmr_u32 awb_size;
	void *lsc;
	cmr_u32 lsc_size;
};

cmr_s32 random_unpack(void *random_data, cmr_u32 random_size, struct random_info *info);
cmr_s32 random_lsc_unpack(void *random_lsc, cmr_u32 random_lsc_size, struct random_lsc_info *lsc_info);
cmr_s32 random_awb_unpack(void *random_awb, cmr_u32 random_awb_size, struct random_awb_info *awb_info);

#ifdef __cplusplus
}
#endif
#endif
