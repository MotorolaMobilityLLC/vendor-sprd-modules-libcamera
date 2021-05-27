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

#include "../af_drv/sns_af_drv.h"
#include "../otp_drv/otp_info.h"
#include "../ois_drv/sns_ois_drv.h"
typedef struct sensor_match_tab {
    /**
     * In order to avoid user input errors and uniform format sensor
     * vendor name,we use enumeration to get sensor vendor name.So when you
     * config module_name,you should select a module_index between MODULE_SUNNY
     * and MODULE_MAX at enumeration group{@camera_vendor_name_type}
     **/
    cmr_u16 module_id;
    char sn_name[SENSOR_NAME_LEN];
    SENSOR_INFO_T *sensor_info;
    struct sns_af_drv_cfg af_dev_info;
    struct sns_ois_drv_cfg ois_drv_info;
    otp_drv_info_t otp_drv_info;
} SENSOR_MATCH_T;

struct lensProperty *sensor_get_lens_property(int phy_id);
#endif
