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

#include <libyuv/convert.h>
#include <libyuv/convert_from.h>
#include <libyuv/video_common.h>
#include <libyuv/scale.h>
#include <libyuv/rotate.h>

#if defined(CONFIG_LIBYUV_NEON)
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
int yuv_scale_nv21_hal(const struct img_frm *src,
                   struct img_frm *dst);

#ifdef __cplusplus
}//extern C
#endif
