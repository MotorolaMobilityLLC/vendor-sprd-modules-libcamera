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
#include "isp_mw.h"
#include "Denoise_SPRD.h"
#include <math.h>


struct class_cnr {
    struct ipm_common common;
    cmr_uint is_inited;
    void *handle;
    sem_t sem;
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

enum nr_type { NR_ENABLE = 1, CNR_ENABLE, CNR_YNR_ENABLE };

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
    CMR_LOGI("E");

    cnr_handle = (struct class_cnr *)malloc(sizeof(struct class_cnr));
    if (!cnr_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(cnr_handle, sizeof(struct class_cnr));

    cnr_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    cnr_handle->common.class_type = IPM_TYPE_CNR;

    cnr_handle->common.ops = &cnr_ops_tab_info;


    property_get("vendor.cam.cnr.threadnum", value, "4");
    threadSet.threadNum = atoi(value);
    property_get("vendor.cam.cnr.corebundle", value, "0");
    threadSet.coreBundle = atoi(value);

    cnr_handle->handle = sprd_cnr_init(threadSet);
    if (NULL == cnr_handle->handle) {
        CMR_LOGE("failed to create");
        goto exit;
    }

    cnr_handle->is_inited = 1;
    sem_init(&cnr_handle->sem, 0, 1);

    *class_handle = (cmr_handle)cnr_handle;

    CMR_LOGI(" x ");

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
    CMR_LOGI("E");

    if (cnr_handle->is_inited) {
        sem_wait(&cnr_handle->sem);

        ret = sprd_cnr_deinit(cnr_handle->handle);
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

    CMR_LOGI("X ");

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
    denoise_buffer imgBuffer;
    Denoise_Param denoiseParam;
    YNR_Param ynrParam;
    CNR_Param cnrParam;
    denoise_mode mode = cxt->nr_flag - 1;
    cmr_bzero(&imgBuffer, sizeof(denoise_buffer));
    cmr_bzero(&denoiseParam, sizeof(Denoise_Param));
    cmr_bzero(&ynrParam, sizeof(YNR_Param));
    cmr_bzero(&cnrParam, sizeof(CNR_Param));

    if (!in || !class_handle || !cxt || !cnr_handle->handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD("E ");
    if (!cnr_handle->is_inited) {
        return ret;
    }
    sem_wait(&cnr_handle->sem);
    imgBuffer.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
    imgBuffer.bufferUV = (unsigned char *)in->src_frame.addr_vir.addr_u;
    width = in->src_frame.size.width;
    height = in->src_frame.size.height;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.before.docnr", value, "null");
    if (!strcmp(value, "true")) {
        addr = &in->src_frame.addr_vir;
        dump_image("cnr_transfer_frame_before_cnr", CAM_IMG_FMT_YUV420_NV21, width,
                   height, 0, addr, width * height * 3 / 2);
    }

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = cnr_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &cnr_handle->common.ipm_cxt->init_in;
    CMR_LOGI("cxt->nr_flag %d", cxt->nr_flag);
    if (cxt->nr_flag & NR_ENABLE) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_YNRS_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
        memcpy(&ynrParam, &isp_cmd_parm.ynr_param, sizeof(YNR_Param));
        denoiseParam.ynrParam = &ynrParam;
        if (cxt->nr_flag == CNR_YNR_ENABLE) {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                        &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp CNR param  %ld", ret);
                goto exit;
            }
            memcpy(&cnrParam, &isp_cmd_parm.cnr2_param, sizeof(CNR_Param));
            denoiseParam.cnrParam = &cnrParam;
        } else {
            denoiseParam.cnrParam = NULL;
        }
    } else {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
        memcpy(&cnrParam, &isp_cmd_parm.cnr2_param, sizeof(CNR_Param));
        denoiseParam.cnrParam = &cnrParam;
        denoiseParam.ynrParam = NULL;
    }
    char prop[PROPERTY_VALUE_MAX];
    property_get("debug.dump.nr.mode", prop, "0");
    if (atoi(prop) != 0) {
        mode = atoi(prop) - 1;
    }
    ret = sprd_cnr_process(cnr_handle->handle, &imgBuffer, &denoiseParam, mode,
                           width, height);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("failed to docnr %ld", ret);
        goto exit;
    }
    property_get("debug.dump.after.docnr", value, "null");
    if (!strcmp(value, "true")) {
        addr = &in->src_frame.addr_vir;
        dump_image("cnr_transfer_frame_after_cnr", CAM_IMG_FMT_YUV420_NV21, width,
                   height, 1, addr, width * height * 3 / 2);
    }

exit:
    CMR_LOGI("X");

    sem_post(&cnr_handle->sem);
    return ret;
}
#endif
