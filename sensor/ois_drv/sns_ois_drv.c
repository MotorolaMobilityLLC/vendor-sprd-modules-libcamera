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
#define LOG_TAG "sns_ois_drv"
#include "sns_ois_drv.h"

int ois_drv_create(struct ois_drv_init_para *input_ptr,
                  cmr_handle *sns_ois_drv_handle) {
    cmr_int ret = OIS_SUCCESS;
    struct sns_ois_drv_cxt *ois_drv_cxt = NULL;
    CHECK_PTR(input_ptr);
    CMR_LOGV("in");
    *sns_ois_drv_handle = NULL;
    ois_drv_cxt = (struct sns_ois_drv_cxt *)malloc(sizeof(struct sns_ois_drv_cxt));
    if (!ois_drv_cxt) {
        ret = OIS_FAIL;
    } else {
        cmr_bzero(ois_drv_cxt, sizeof(*ois_drv_cxt));
        ois_drv_cxt->hw_handle = input_ptr->hw_handle;
        ois_drv_cxt->i2c_addr = input_ptr->ois_work_mode;
        *sns_ois_drv_handle = (cmr_handle)ois_drv_cxt;
    }
    CMR_LOGV("out");
    return ret;
}

int ois_drv_delete(cmr_handle sns_ois_drv_handle, void *param) {
    cmr_int ret = OIS_SUCCESS;
    CMR_LOGV("in");
    if (sns_ois_drv_handle) {
        free(sns_ois_drv_handle);
    } else {
        CMR_LOGE("ois_drv_handle is NULL,no need release.");
    }
    CMR_LOGV("out");
    return ret;
}
