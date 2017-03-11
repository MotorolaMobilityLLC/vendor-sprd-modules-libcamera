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
#ifndef _AF_DRV_H_
#define _AF_DRV_H_

#include <utils/Log.h>
#include "jpeg_exif_header.h"
#include "cmr_type.h"
#include "cmr_common.h"
#include "sensor_raw.h"
#include "sensor_drv_u.h"

#define VCM_UNIDENTIFY 0xFFFF

#ifndef CHECK_PTR
#define CHECK_PTR(expr)                                                        \
    if ((expr) == NULL) {                                                      \
        ALOGE("ERROR: NULL pointer detected " #expr);                          \
        return FALSE;                                                          \
    }
#endif

#define AF_LOGI CMR_LOGI
#define AF_LOGD CMR_LOGD
#define AF_LOGE CMR_LOGE

#define AF_SUCCESS CMR_CAMERA_SUCCESS
#define AF_FAIL CMR_CAMERA_FAIL

typedef struct af_ioctl_func_tab_tag {
    cmr_uint (*set_motor_bestmode)(SENSOR_HW_HANDLE handle);
    // cmr_uint (*motor_init)(SENSOR_HW_HANDLE handle,uint32_t mode);
    cmr_uint (*motor_deinit)(SENSOR_HW_HANDLE handle,
                             uint32_t mode); /*can be NULL*/

    cmr_uint (*set_motor_pos)(SENSOR_HW_HANDLE handle,
                              uint32_t pos); /*customer must realize*/
    cmr_uint (*get_motor_pos)(SENSOR_HW_HANDLE handle,
                              uint16_t *pos); /*can be NULL*/
    /*the fllowing interface just for debug*/
    cmr_uint (*get_test_motor_mode)(SENSOR_HW_HANDLE handle);
    cmr_uint (*set_test_motor_mode)(SENSOR_HW_HANDLE handle, char *vcm_mode);
    /*expend*/
    cmr_uint (*motor_process)(SENSOR_HW_HANDLE handle, uint32_t cmd,
                              void *param);
} af_ioctl_func_tab_t;

typedef struct af_info_tab_tag {
    uint32_t af_work_mode;
    af_ioctl_func_tab_t af_ops;
} af_drv_info_t;
#endif
