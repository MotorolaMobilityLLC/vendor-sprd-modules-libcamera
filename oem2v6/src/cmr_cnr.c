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
#include "sprd_yuv_denoise_adapter.h"
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

enum nr_type {
    YNRS_ENABLE = (1 << 0),
    CNR2_ENABLE = (1 << 1),
    CNR3_ENABLE = (1 << 2),
};

struct class_tab_t cnr_tab_info = {
    &cnr_ops_tab_info,
};

static cmr_int cnr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_cnr *cnr_handle = NULL;
    sprd_yuv_denoise_init_t param;

    param.width = 4160;
    param.height = 3120;
    param.runversion = 1;

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

    cnr_handle->handle = sprd_yuv_denoise_adpt_init((void *)&param);
    if (NULL == cnr_handle->handle) {
        CMR_LOGE("failed to create");
        goto exit;
    }

    cnr_handle->is_inited = 1;
    sem_init(&cnr_handle->sem, 0, 1);

    *class_handle = (cmr_handle)cnr_handle;

    CMR_LOGD("X");

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

        ret = sprd_yuv_denoise_adpt_deinit(cnr_handle->handle);
        if (ret) {
            CMR_LOGE("failed to deinit");
        }
        cnr_handle->is_inited = 0;
        sem_post(&cnr_handle->sem);
        sem_destroy(&cnr_handle->sem);
    }
    CMR_LOGD("deinit success");

    free(cnr_handle);
    class_handle = NULL;

    CMR_LOGD("X");

    return ret;
}

static cmr_int cnr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint max_radius = 0;
    cmr_u32 valid_nr_type = 0;
    struct img_addr *addr;
    struct class_cnr *cnr_handle = NULL;
    struct camera_context *cxt = NULL;
    YNR_Param ynrParam;
    CNR_Parameter cnr2Param;
    cnr_param_t cnr3Param;
    sprd_yuv_denoise_cmd_t mode;
    sprd_yuv_denoise_param_t denoise_param;
    cmr_bzero(&ynrParam, sizeof(YNR_Param));
    cmr_bzero(&cnr2Param, sizeof(CNR_Parameter));
    cmr_bzero(&cnr3Param, sizeof(cnr_param_t));
    cmr_bzero(&denoise_param, sizeof(sprd_yuv_denoise_param_t));

    if (!in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    cnr_handle = (struct class_cnr *)class_handle;
    cxt = (struct camera_context *)in->private_data;

    if (!cxt || !cnr_handle->handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }


    CMR_LOGV("E ");
    sem_wait(&cnr_handle->sem);
    if(!cnr_handle->is_inited) {
        CMR_LOGE("failed to transfer frame %ld",ret);
        goto exit;
    }
    denoise_param.bufferY.addr[0] = (void *)in->src_frame.addr_vir.addr_y;
    denoise_param.bufferUV.addr[0] = (void *)in->src_frame.addr_vir.addr_u;
    denoise_param.bufferY.ion_fd = (int32_t)in->src_frame.fd;
    denoise_param.bufferUV.ion_fd = (int32_t)in->src_frame.fd;
    denoise_param.width = in->src_frame.size.width;
    denoise_param.height = in->src_frame.size.height;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.before.docnr", value, "null");
    if (!strcmp(value, "true")) {
        addr = &in->src_frame.addr_vir;
        dump_image("cnr_transfer_frame_before_cnr", CAM_IMG_FMT_YUV420_NV21,
                   denoise_param.width, denoise_param.height, 0, addr,
                      (denoise_param.width) * (denoise_param.height) * 3 / 2);
    }

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = cnr_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &cnr_handle->common.ipm_cxt->init_in;
    CMR_LOGD("cnr_flag %d, night_flag %d", cxt->nr_flag, cxt->night_flag);
    if (cxt->nr_flag & YNRS_ENABLE) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_YNRS_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param %ld", ret);
        } else {
            denoise_param.ynr_ration_base = isp_cmd_parm.ynr_param.radius_base;
            memcpy(&ynrParam, &isp_cmd_parm.ynr_param.ynrs_param, sizeof(YNR_Param));
            denoise_param.ynrParam = &ynrParam;
            denoise_param.zoom_ratio = cxt->zoom_ratio;
            valid_nr_type |= YNRS_ENABLE;
        }
    }
    if (cxt->nr_flag & CNR2_ENABLE ) {
        if (cxt->night_flag) {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                         &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp CNR2 param %ld", ret);
            } else {
                memcpy(&cnr2Param, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
                denoise_param.cnr2Param = &cnr2Param;
                valid_nr_type |= CNR2_ENABLE;
            }
        }
    }
    if (cxt->nr_flag & CNR3_ENABLE) {
        if (!cxt->night_flag) {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR3_PARAM,
                            &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp CNR3 param	%ld", ret);
                goto proc;
            }
            cnr3Param.bypass = isp_cmd_parm.cnr3_param.bypass;
            memcpy(&cnr3Param.paramLayer, &isp_cmd_parm.cnr3_param.param_layer, LAYER_NUM*sizeof(multiParam));
            denoise_param.cnr3Param = &cnr3Param;
            denoise_param.cnr_ration_base = isp_cmd_parm.cnr3_param.baseRadius;
            for(i = 0;i < 3; i++) {
                CMR_LOGV("sigma[0][%d] %f", i, denoise_param.cnr3Param->paramLayer[0].sigma[i]);
                CMR_LOGV("sigma[4][%d] %f", i, denoise_param.cnr3Param->paramLayer[4].sigma[i]);
            }
            valid_nr_type |= CNR3_ENABLE;
        }
    }
proc:
    if (valid_nr_type == 0) {
        CMR_LOGD("valid_nr_type is 0");
        goto exit;
    }
    mode = valid_nr_type;
    CMR_LOGI("valid_nr_type %d, param %p, %p, %p\n", valid_nr_type,
        denoise_param.ynrParam, denoise_param.cnr2Param, denoise_param.cnr3Param);
    char prop[PROPERTY_VALUE_MAX];
    property_get("debug.dump.nr.mode", prop, "0");
    if (atoi(prop) != 0) {
        mode = atoi(prop);
    }
    if(mode >= SPRD_YUV_DENOISE_MAX_CMD){
        CMR_LOGE("cmd %d is invalid.",mode);
        goto exit;
    }
    ret = sprd_yuv_denoise_adpt_ctrl(cnr_handle->handle, mode, (void *)&denoise_param);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("failed to docnr %ld", ret);
        goto exit;
    }
    property_get("debug.dump.after.docnr", value, "null");
    if (!strcmp(value, "true")) {
        addr = &in->src_frame.addr_vir;
        dump_image("cnr_transfer_frame_after_cnr", CAM_IMG_FMT_YUV420_NV21,
            denoise_param.width, denoise_param.height, 1, addr,
                (denoise_param.width) * (denoise_param.height) * 3 / 2);
    }

exit:
    CMR_LOGV("X");
    sem_post(&cnr_handle->sem);
    return ret;
}
#endif
