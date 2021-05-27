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
#ifndef _DW9781B_H_
#define _DW9781B_H_
#include <utils/Log.h>
#include "sensor.h"
#include "sns_ois_drv.h"

#define DW9781C_VCM_SLAVE_ADDR (0xE4 >> 1)
#define MOVE_CODE_STEP_MAX 20
#define WAIT_STABLE_TIME 10     // ms
#define DW9781B_POWERON_DELAY 3 // ms

static int dw9781c_drv_calibrate(cmr_handle sns_ois_drv_handle);
static int dw9781c_drv_create(struct ois_drv_init_para *input_ptr, cmr_handle *sns_ois_drv_handle);
static int dw9781c_drv_delete(cmr_handle sns_ois_drv_handle, void *param);
static int dw9781c_drv_enforce(cmr_handle sns_ois_drv_handle);

#endif
