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
#ifndef _SENSOR_CFG_H_
#define _SENSOR_CFG_H_

#include "sns_af_drv.h"
#include "otp_info.h"

/*boardconfig.mk sensor name len*/
#define MAX_SENSOR_NAME_LEN 128

typedef struct sensor_match_tab {
    /**
     * In order to avoid user input errors and uniform format sensor
     * vendor name,we use enumeration to get sensor vendor name.So when you
     * config module_name,you should select a module_index between MODULE_SUNNY
     * and MODULE_MAX at enumeration group{@camera_vendor_name_type}
     **/
    cmr_u16 module_id;
    char sn_name[36];
    SENSOR_INFO_T *sensor_info;
    struct sns_af_drv_cfg af_dev_info;
    otp_drv_info_t otp_drv_info;
} SENSOR_MATCH_T;

SENSOR_MATCH_T *sensor_get_regist_table(cmr_u32 sensor_id);
char *sensor_get_name_list(cmr_u32 sensor_id);
SENSOR_MATCH_T *sensor_get_entry_by_idx(cmr_u32 sensor_id, cmr_u16 idx);
cmr_int sensor_check_name(cmr_u32 sensor_id, SENSOR_MATCH_T *reg_tab_ptr);
SENSOR_INFO_FOR_HAL *sensor_get_camera_info_reg_tab(cmr_u32 sensor_id);
#endif
