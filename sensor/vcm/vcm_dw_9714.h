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
#ifdef CONFIG_AF_VCM_9714
#include "sensor_drv_u.h"
#else
#include "vcm_drv.h"
#endif
#define DW9714_VCM_SLAVE_ADDR (0x18>>1)
uint32_t vcm_dw9714_init(SENSOR_HW_HANDLE handle,uint32_t mode);
uint32_t vcm_dw9714_set_position(SENSOR_HW_HANDLE handle,uint32_t pos,uint32_t slewrate);
/*cmr_uint dw9714_set_pos(SENSOR_HW_HANDLE handle,uint16_t pos);
cmr_uint dw9714_get_otp(SENSOR_HW_HANDLE handle,uint16_t *inf,uint16_t *macro);
cmr_uint dw9714_get_motor_pos(SENSOR_HW_HANDLE handle,uint16_t *pos);
cmr_uint dw9714_set_motor_bestmode(SENSOR_HW_HANDLE handle);
cmr_uint dw9714_get_test_vcm_mode(SENSOR_HW_HANDLE handle);
cmr_uint dw9714_set_test_vcm_mode(SENSOR_HW_HANDLE handle,char* vcm_mode);
*/
