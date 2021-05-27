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
#ifndef _SNS_OIS_DRV_H_
#define _SNS_OIS_DRV_H_

#include <utils/Log.h>
#include "jpeg_exif_header.h"
#include "cmr_type.h"
#include "cmr_common.h"
#include "cmr_sensor_info.h"
#include "sensor_raw.h"
#include "sensor_drv_u.h"

#define OIS_SUCCESS CMR_CAMERA_SUCCESS
#define OIS_FAIL CMR_CAMERA_FAIL

/*OIS bool type*/
#define OIS_FALSE 0
#define OIS_TRUE 1

#ifndef CHECK_PTR
#define CHECK_PTR(expr)                                                        \
    if ((expr) == NULL) {                                                      \
        ALOGE("ERROR: NULL pointer detected " #expr);                          \
        return OIS_FAIL;                                                       \
    }
#endif

struct ois_drv_init_para {
    SENSOR_HW_HANDLE hw_handle;
    uint32_t ois_work_mode;
    /*you can add your param here*/
};

struct sns_ois_drv_ops {
    /*used in sensor_drv_u*/
    int (*create)(struct ois_drv_init_para *input_ptr,
                  cmr_handle *sns_ois_drv_handle); /*create ois handle*/
    int (*delete)(cmr_handle sns_ois_drv_handle,
                  void *param); /*delete ois handle*/

    /*export to isp*/
    int (*ois_enforce)(cmr_handle sns_ois_drv_handle); /*customer must realize*/
    int (*ois_calibrate)(cmr_handle sns_ois_drv_handle); /*can be NULL*/
};

struct sns_ois_drv_entry {
    uint32_t default_work_mode;
    struct sns_ois_drv_ops ois_ops;
};

struct sns_ois_drv_cxt {
    cmr_handle hw_handle;  /*hardware handle*/
    uint32_t ois_work_mode; /*Actual working mode*/
    uint16_t i2c_addr;
    void *private;         /*ois private data*/
};

struct sns_ois_drv_cfg {
    struct sns_ois_drv_entry *ois_drv_entry;
    uint32_t ois_work_mode; /*config by user*/
};

struct ois_drv_lib {
    void *ois_lib_handle;
    struct sns_ois_drv_entry *ois_info_ptr;
};

int ois_drv_create(struct ois_drv_init_para *input_ptr,
                  cmr_handle *sns_ois_drv_handle);
int ois_drv_delete(cmr_handle sns_ois_drv_handle, void *param);
#endif
