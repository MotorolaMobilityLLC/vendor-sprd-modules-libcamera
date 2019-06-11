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
#ifndef _CMR_MM_DVFS_H_
#define _CMR_MM_DVFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cmr_common.h"

cmr_int cmr_mm_dvfs_init(cmr_handle *mm_dvfs_handle);
cmr_int cmr_mm_dvfs_deinit(cmr_handle mm_dvfs_handle);

struct dvfs_mm_status {

    uint8_t mDcamIf_index_dvfs;
    uint8_t mIsp_index_dvfs;

    bool isp_hw_dvfs_en;
    uint isp_cur_freq;

    uint dmca_if_cur_freq;
};

struct class_mm_dvfs {
    struct prev_sn_param_dvfs_type dvfs_param;
    struct dvfs_mm_status dvfs_status;
    pthread_mutex_t mm_dvfs_mutex;
};

static struct class_mm_dvfs *mm_dvfs_instance = NULL;

cmr_int cmr_set_mm_dvfs_policy(cmr_handle mm_dvfs_handle,
                               enum DVFS_MM_MODULE module,
                               enum CamProcessingState camera_state);

cmr_int cmr_set_mm_dvfs_param(cmr_handle oem_handle,
                              struct prev_sn_param_dvfs_type dvfs_param);
cmr_int do_debug_dvfs_policy(cmr_handle mm_dvfs_handle,
                             enum DVFS_MM_MODULE module,
                             enum CamProcessingState camera_state);
struct class_mm_dvfs *getInstance();

#ifdef __cplusplus
}
#endif

#endif
