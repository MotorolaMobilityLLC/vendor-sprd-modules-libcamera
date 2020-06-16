/*
 * Copyright (C) 2018 The Android Open Source Project
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
#ifndef _ISP_PARAM_FILE_UPDATE_H_
#define _ISP_PARAM_FILE_UPDATE_H_

#include <sys/types.h>
//#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "isp_blocks_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char nr_param_name[ISP_BLK_NR_MAX][20];
extern char nr_mode_name[MAX_MODE_NUM][12];
extern char nr_scene_name[MAX_SCENEMODE_NUM][12];
extern struct isp_pm_nrblk_info nr_blocks_info[ISP_BLK_NR_MAX];

cmr_u32 isp_pm_raw_para_update_from_file(struct sensor_raw_info *raw_info_ptr, char *data_path);
#if 0
#ifndef WIN32
cmr_u32 isp_raw_para_update_from_file(SENSOR_INFO_T * sensor_info_ptr, SENSOR_ID_E sensor_id);
#endif
#endif

#ifdef   __cplusplus
}
#endif

#endif
