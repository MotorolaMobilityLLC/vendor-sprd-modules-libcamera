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
#ifndef _CPP_U_SLICE_H_
#define _CPP_U_SLICE_H_
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "sprd_cpp.h"
#include "cmr_types.h"
#include "slice_drv.h"
#include "cmr_common.h"

#define MOD(x, a) (x % a)
#define MOD2(x)	(x % 2)
#define MOD8(x)	(x % 8)
#define OSIDE(x, a, b) ((x < a) || (x > b))
#define CMP(x, a, b) (x < (a + b))
#define ALIGN_UP(size, align) ((size + (align - 1)) & (~(align - 1)))
#define ALIGN_DOWN(size, align) ((size) & ~((align) - 1))

int cpp_u_input_param_check(
		struct sprd_cpp_scale_cfg_parm *cfg_parm);
#ifdef CPP_LITE_R5P0
	void convert_param_to_calc(
			struct sprd_cpp_scale_cfg_parm *cfg_parm,
			slice_drv_param_t *slice_parm);
#else 
#ifdef CPP_LITE_R6P0
void convert_param_to_calc(
		struct sprd_cpp_scale_cfg_parm *cfg_parm,
		struct sprd_cpp_scale_slice_parm *slice_parm);
#endif
#endif
#endif

