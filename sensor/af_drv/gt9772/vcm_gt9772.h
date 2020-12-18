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
#ifndef GT9772_H_
#define GT9772_H_
#include "sensor_drv_u.h"
#include "sns_af_drv.h"

#define GT9772_VCM_SLAVE_ADDR (0xe4 >> 1)
#define GT9772_VCM_SLAVE_ADDR (0xe4 >> 1)
#ifndef SENSOR_SUCCESS
#define SENSOR_SUCCESS 0
#endif
/* accroding to vcm module spec */
#define POSE_UP_HORIZONTAL 32
#define POSE_DOWN_HORIZONTAL 37
#define GT9772_POWERON_DELAY 8 // ms

static int _gt9772_drv_power_on(cmr_handle sns_af_drv_handle,
                                uint16_t power_on);
static uint32_t _gt9772_set_motor_bestmode(cmr_handle sns_af_drv_handle);
static uint32_t _gt9772_get_test_vcm_mode(cmr_handle sns_af_drv_handle);
static uint32_t _gt9772_set_test_vcm_mode(cmr_handle sns_af_drv_handle,
                                          char *vcm_mode);
static int _gt9772_drv_init(cmr_handle sns_af_drv_handle);
static int gt9772_drv_create(struct af_drv_init_para *input_ptr,
                             cmr_handle *sns_af_drv_handle);
static int gt9772_drv_delete(cmr_handle sns_af_drv_handle, void *param);
static int gt9772_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos);
static int gt9772_drv_get_pos(cmr_handle sns_af_drv_handle, uint16_t *pos);
static int gt9772_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
                            void *param);
#endif