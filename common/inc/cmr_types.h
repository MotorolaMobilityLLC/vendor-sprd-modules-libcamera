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

#ifndef _CMR_TYPES_H_
#define _CMR_TYPES_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include <sys/types.h>
#include <utils/Log.h>

enum camera_mem_cb_type {
	CAMERA_PREVIEW = 0,
	CAMERA_SNAPSHOT,
	CAMERA_SNAPSHOT_ZSL,
	CAMERA_VIDEO,
	CAMERA_PREVIEW_RESERVED,
	CAMERA_SNAPSHOT_ZSL_RESERVED_MEM,
	CAMERA_SNAPSHOT_ZSL_RESERVED,
	CAMERA_VIDEO_RESERVED,
	CAMERA_ISP_LSC,
	CAMERA_ISP_BINGING4AWB,
	CAMERA_SNAPSHOT_PATH,
	CAMERA_ISP_FIRMWARE,
	CAMERA_SNAPSHOT_HIGHISO,
	CAMERA_ISP_ANTI_FLICKER,
	CAMERA_ISP_RAWAE,
	CAMERA_MEM_CB_TYPE_MAX
};


typedef unsigned long   cmr_uint;
typedef long            cmr_int;
typedef uint64_t        cmr_u64;
typedef int64_t         cmr_s64;
typedef unsigned int    cmr_u32;
typedef int             cmr_s32;
typedef unsigned short  cmr_u16;
typedef short           cmr_s16;
typedef unsigned char   cmr_u8;
typedef char            cmr_s8;
typedef void*           cmr_handle;


#define bzero(p, len) memset(p, 0, len);

#define UNUSED(x) (void)x
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__


#if 1//(SC_FPGA == 0)
#define CMR_LOGE(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGW(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGI(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGD(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGV(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
//#warnning  "SC_FPGA is not set"
#else
#include "ylog.h"
#define CMR_LOGE(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGW(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGI(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGD(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define CMR_LOGV(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#endif


typedef void (*cmr_malloc)(cmr_u32 mem_type, cmr_handle oem_handle, cmr_u32 *size,
	                         cmr_u32 *sum, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *mfd);
typedef void (*cmr_free)(cmr_u32 mem_type, cmr_handle oem_handle, cmr_uint *phy_addr,
	                       cmr_uint *vir_addr, cmr_u32 sum);

#endif
