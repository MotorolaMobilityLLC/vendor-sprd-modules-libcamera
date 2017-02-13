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
#ifndef _AK7371_H_
#define _AK7371_H_
#include "sensor_drv_u.h"

uint32_t vcm_ak7371_init(SENSOR_HW_HANDLE handle, int mode);
uint32_t vcm_ak7371_set_position(SENSOR_HW_HANDLE handle, uint32_t pos);
uint32_t vcm_ak7371_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h, uint32_t *h2down);
uint32_t vcm_ak7371_deinit(SENSOR_HW_HANDLE handle);

#endif

