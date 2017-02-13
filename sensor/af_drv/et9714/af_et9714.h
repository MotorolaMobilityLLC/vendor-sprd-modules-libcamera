/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ET9714_H_
#define _ET9714_H_
#include "af_drv.h"

#define et9714_VCM_SLAVE_ADDR (0x18 >> 1)
#define MOVE_CODE_STEP_MAX 40
#define WAIT_STABLE_TIME  20    //ms

uint32_t et9714_init(SENSOR_HW_HANDLE handle,uint32_t mode);
uint32_t et9714_write_af(SENSOR_HW_HANDLE handle,uint32_t param);
uint32_t et9714_deinit(SENSOR_HW_HANDLE handle,uint32_t mode);

#endif
