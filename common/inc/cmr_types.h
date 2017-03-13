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
    CAMERA_ISP_RAW_DATA,
    CAMERA_ISP_ANTI_FLICKER,
    CAMERA_ISP_RAWAE,
    CAMERA_ISP_PREVIEW_Y,
    CAMERA_ISP_PREVIEW_YUV,
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    CAMERA_DEPTH_MAP,
    CAMERA_DEPTH_MAP_RESERVED,
#else
    CAMERA_SENSOR_DATATYPE_MAP, // CAMERA_DEPTH_MAP,
    CAMERA_SENSOR_DATATYPE_MAP_RESERVED, // CAMERA_DEPTH_MAP_RESERVED,
#endif
    CAMERA_PDAF_RAW,
    CAMERA_PDAF_RAW_RESERVED,
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    CAMERA_ISP_STATIS,
#endif
    CAMERA_MEM_CB_TYPE_MAX
};

typedef unsigned long cmr_uint;
typedef long cmr_int;
typedef uint64_t cmr_u64;
typedef int64_t cmr_s64;
typedef unsigned int cmr_u32;
typedef int cmr_s32;
typedef unsigned short cmr_u16;
typedef short cmr_s16;
typedef unsigned char cmr_u8;
typedef signed char cmr_s8;
typedef void *cmr_handle;

#ifndef bzero
#define bzero(p, len) memset(p, 0, len);
#endif

#ifndef UNUSED
#define UNUSED(x) (void) x
#endif
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef cmr_int (*cmr_malloc)(cmr_u32 mem_type, cmr_handle oem_handle,
                              cmr_u32 *size, cmr_u32 *sum, cmr_uint *phy_addr,
                              cmr_uint *vir_addr, cmr_s32 *fd);
typedef cmr_int (*cmr_free)(cmr_u32 mem_type, cmr_handle oem_handle,
                            cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                            cmr_u32 sum);

#endif
