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

#include <cutils/properties.h>
#ifdef CONFIG_CAMERA_CNR

#define LOG_TAG "cmr_sprd_cnr"
#include "cmr_common.h"
#include "cmr_oem.h"
#include "CNR_SPRD.h"
#include "isp_mw.h"

struct class_cnr {
    struct ipm_common common;
    cmr_uint is_inited;
    void *handle;
    sem_t sem;
    LibVersion cnr_ver;
};
static cmr_int cnr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle);
static cmr_int cnr_close(cmr_handle class_handle);
static cmr_int cnr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out);

static struct class_ops cnr_ops_tab_info = {
    cnr_open, cnr_close, cnr_transfer_frame, NULL, NULL,
};

struct class_tab_t cnr_tab_info = {
    &cnr_ops_tab_info,
};

static cmr_int cnr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_cnr *cnr_handle = NULL;

    ThreadSet threadSet;
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };

    if (!ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD("E");

    cnr_handle = (struct class_cnr *)malloc(sizeof(struct class_cnr));
    if (!cnr_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(cnr_handle, sizeof(struct class_cnr));

    cnr_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    cnr_handle->common.class_type = IPM_TYPE_CNR;

    cnr_handle->common.ops = &cnr_ops_tab_info;

    cnr_handle->cnr_ver.major = 2;
    cnr_handle->cnr_ver.middle = 0;
    cnr_handle->cnr_ver.minor = 1;

    property_get("vendor.cam.cnr.threadnum", value, "4");
    threadSet.threadNum = atoi(value);
    property_get("vendor.cam.cnr.corebundle", value, "0");
    threadSet.coreBundle = atoi(value);

    cnr_handle->handle = cnr_init(&cnr_handle->cnr_ver, threadSet);
    if (NULL == cnr_handle->handle) {
        CMR_LOGE("failed to create");
        goto exit;
    }

    cnr_handle->is_inited = 1;
    sem_init(&cnr_handle->sem, 0, 1);

    *class_handle = (cmr_handle)cnr_handle;

    CMR_LOGD(" x ");

    return ret;

exit:
    if (NULL != cnr_handle) {
        free(cnr_handle);
        cnr_handle = NULL;
    }
    return CMR_CAMERA_FAIL;
}

static cmr_int cnr_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_cnr *cnr_handle = (struct class_cnr *)class_handle;
    if (!cnr_handle || !cnr_handle->handle) {
        CMR_LOGE("cnr_handle is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD("E");

    if (cnr_handle->is_inited) {
        sem_wait(&cnr_handle->sem);

        ret = cnr_destroy(cnr_handle->handle);
        if (ret) {
            CMR_LOGE("failed to deinit");
        }
        sem_post(&cnr_handle->sem);
        sem_destroy(&cnr_handle->sem);
    }
    CMR_LOGD("deinit success");

    cnr_handle->is_inited = 0;
    free(cnr_handle);
    class_handle = NULL;

    CMR_LOGD("X ");

    return ret;
}

static cmr_int cnr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_cnr *cnr_handle = (struct class_cnr *)class_handle;
    struct img_addr *addr;
    cmr_uint width = 0;
    cmr_uint height = 0;
    struct camera_context *cxt = (struct camera_context *)in->private_data;

    if (!in || !class_handle || !cxt || !cnr_handle->handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD("E ");
    if (!cnr_handle->is_inited) {
        return ret;
    }
    sem_wait(&cnr_handle->sem);

    addr = &in->src_frame.addr_vir;
    width = in->src_frame.size.width;
    height = in->src_frame.size.height;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.before.docnr", value, "null");
    if (!strcmp(value, "true")) {
        dump_image("cnr_transfer_frame_before_cnr", CAM_IMG_FMT_YUV420_NV21, width,
                   height, 0, addr, width * height * 3 / 2);
    }
    CMR_LOGD("w=%lu,h=%lu, addr= %p", width, height, addr);

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = cnr_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &cnr_handle->common.ipm_cxt->init_in;

    ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                &isp_cmd_parm);

    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("failed to get isp param  %ld", ret);
        goto exit;
    }
    ret = cnr(cnr_handle->handle, (CNR_Parameter *)&isp_cmd_parm.cnr2_param,
              (unsigned char *)addr->addr_u, width, height);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("failed to docnr %ld", ret);
        goto exit;
    }
    property_get("debug.dump.after.docnr", value, "null");
    if (!strcmp(value, "true")) {
        dump_image("cnr_transfer_frame_after_cnr", CAM_IMG_FMT_YUV420_NV21, width,
                   height, 1, addr, width * height * 3 / 2);
    }

exit:
    CMR_LOGD("X");

    sem_post(&cnr_handle->sem);
    return ret;
}
#endif
