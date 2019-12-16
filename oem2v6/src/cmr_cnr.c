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

#ifdef CAMERA_CNR3_ENABLE
enum nr_type { NR_ENABLE = 1, CNR2_ENABLE, CNR2_YNR_ENABLE,CNR3_ENABLE, CNR3_YNR_ENABLE , CNR2_CNR3_ENABLE, CNR2_CNR3_YNR_ENABLE };
#else
enum nr_type { NR_ENABLE = 1, CNR_ENABLE, CNR_YNR_ENABLE };
#endif

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
        sem_post(&cnr_handle->sem);
        sem_destroy(&cnr_handle->sem);
    }
    CMR_LOGD("deinit success");

    cnr_handle->is_inited = 0;
    free(cnr_handle);
    class_handle = NULL;

    CMR_LOGD("X");

    return ret;
}

#ifdef CAMERA_CNR3_ENABLE
static cmr_int cnr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint max_radius = 0;
    struct class_cnr *cnr_handle = (struct class_cnr *)class_handle;
    struct img_addr *addr;
    struct camera_context *cxt = (struct camera_context *)in->private_data;
    YNR_Param ynrParam;
    CNR_Parameter cnr2Param;
    cnr_param_t cnr3Param;
    sprd_yuv_denoise_param_t denoise_param;
    sprd_yuv_denoise_cmd_t mode;
    cmr_bzero(&ynrParam, sizeof(YNR_Param));
    cmr_bzero(&cnr2Param, sizeof(CNR_Parameter));
    cmr_bzero(&cnr3Param, sizeof(cnr_param_t));
    cmr_bzero(&denoise_param, sizeof(sprd_yuv_denoise_param_t));

    if (!cxt || !cnr_handle->handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGV("E ");
    if (!cnr_handle->is_inited) {
        return ret;
    }
    mode = cxt->nr_flag - 1;
    sem_wait(&cnr_handle->sem);
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
        dump_image("cnr_transfer_frame_before_cnr", CAM_IMG_FMT_YUV420_NV21, denoise_param.width,
                   denoise_param.height, 0, addr, (denoise_param.width) * (denoise_param.height) * 3 / 2);
    }

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = cnr_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &cnr_handle->common.ipm_cxt->init_in;
    CMR_LOGI("cxt->nr_flag %d", cxt->nr_flag);
    if (cxt->nr_flag & NR_ENABLE) {  /*YNR or YNR_CNR*/
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_YNRS_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
#ifdef CAMERA_RADIUS_ENABLE
        isp_cmd_parm.ynr_param.ynrs_param.imgCenterX = denoise_param.width / 2;
        isp_cmd_parm.ynr_param.ynrs_param.imgCenterY = denoise_param.height / 2;
        isp_cmd_parm.ynr_param.ynrs_param.Radius_factor =
                                    (isp_cmd_parm.ynr_param.ynrs_param.Radius_factor / isp_cmd_parm.ynr_param.Radius) * denoise_param.width;
        ISP_LOGI("isp_cmd_parm.ynr_param.ynrs_param.imgCenterX = 0x%x isp_cmd_parm.ynr_param.ynrs_param.imgCenterY = 0x%x isp_cmd_parm.ynr_param.ynrs_param.Radius_factor = 0x%x\n",
                    isp_cmd_parm.ynr_param.ynrs_param.imgCenterX,isp_cmd_parm.ynr_param.ynrs_param.imgCenterY, isp_cmd_parm.ynr_param.ynrs_param.Radius_factor);
        memcpy(&ynrParam, &isp_cmd_parm.ynr_param.ynrs_param, sizeof(YNR_Param));
#else
        memcpy(&ynrParam, &isp_cmd_parm.ynr_param, sizeof(YNR_Param));
#endif
        denoise_param.ynrParam = &ynrParam;
        if (cxt->nr_flag == CNR2_YNR_ENABLE) {    /*YNR_CNR2*/
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                        &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp CNR param  %ld", ret);
                goto exit;
            }
            memcpy(&cnr2Param, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
            denoise_param.cnr2Param = &cnr2Param;
        } else {
            denoise_param.cnr2Param = NULL;
        }
        /*YNR_CNR2 or YNR_CNR2_CNR3*/
        if (cxt->nr_flag == CNR3_YNR_ENABLE || cxt->nr_flag == CNR2_CNR3_YNR_ENABLE) {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR3_PARAM,
                                        &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp CNR param  %ld", ret);
                goto exit;
            }
#ifdef CAMERA_RADIUS_ENABLE
            for (i = 0 ;i < LAYER_NUM; i++){
                isp_cmd_parm.cnr3_param.param_layer[i].imgCenterX = denoise_param.width/pow(2, (i+1));
                isp_cmd_parm.cnr3_param.param_layer[i].imgCenterY = denoise_param.height/pow(2, (i+1));
                max_radius = (denoise_param.width + denoise_param.height)/pow(2, (i+1));
                isp_cmd_parm.cnr3_param.param_layer[i].baseRadius =
                                         (isp_cmd_parm.cnr3_param.param_layer[i].baseRadius/isp_cmd_parm.cnr3_param.baseRadius)*max_radius;
            }
#endif
            cnr3Param.bypass = isp_cmd_parm.cnr3_param.bypass;
            memcpy(&cnr3Param.paramLayer, &isp_cmd_parm.cnr3_param.param_layer, LAYER_NUM*sizeof(multiParam));
            denoise_param.cnr3Param = &cnr3Param;
        } else {
            denoise_param.cnr3Param = NULL;
        }
    } else {   /*CNR*/
        if (cxt->nr_flag == CNR2_ENABLE) {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                    &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp YNR param  %ld", ret);
                goto exit;
            }
            memcpy(&cnr2Param, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
            denoise_param.cnr2Param = &cnr2Param;
            denoise_param.ynrParam = NULL;
            denoise_param.cnr3Param = NULL;
    }
    if (cxt->nr_flag == CNR3_ENABLE || cxt->nr_flag == CNR2_CNR3_ENABLE) {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR3_PARAM,
                                    &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp YNR param  %ld", ret);
                goto exit;
            }
#ifdef CAMERA_RADIUS_ENABLE
            for (i = 0 ;i < LAYER_NUM; i++){
                isp_cmd_parm.cnr3_param.param_layer[i].imgCenterX = denoise_param.width/pow(2, (i+1));
                isp_cmd_parm.cnr3_param.param_layer[i].imgCenterY = denoise_param.height/pow(2, (i+1));
                max_radius = (denoise_param.width + denoise_param.height)/pow(2, (i+1));
                isp_cmd_parm.cnr3_param.param_layer[i].baseRadius =
                                        (isp_cmd_parm.cnr3_param.param_layer[i].baseRadius/isp_cmd_parm.cnr3_param.baseRadius)*max_radius;
            }
#endif
            cnr3Param.bypass = isp_cmd_parm.cnr3_param.bypass;
            memcpy(&cnr3Param.paramLayer, &isp_cmd_parm.cnr3_param.param_layer, LAYER_NUM*sizeof(multiParam));
            denoise_param.cnr3Param = &cnr3Param;
            denoise_param.ynrParam = NULL;
            denoise_param.cnr2Param = NULL;
    }
    }
    char prop[PROPERTY_VALUE_MAX];
    property_get("debug.dump.nr.mode", prop, "0");
    if (atoi(prop) != 0) {
        mode = atoi(prop) - 1;
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
        dump_image("cnr_transfer_frame_after_cnr", CAM_IMG_FMT_YUV420_NV21, denoise_param.width,
                   denoise_param.height, 1, addr, (denoise_param.width) * (denoise_param.height) * 3 / 2);
    }

exit:
    CMR_LOGV("X");

    sem_post(&cnr_handle->sem);
    return ret;
}
#else
static cmr_int cnr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_cnr *cnr_handle = (struct class_cnr *)class_handle;
    struct img_addr *addr;
    struct camera_context *cxt = (struct camera_context *)in->private_data;
    YNR_Param ynrParam;
    CNR_Parameter cnr2Param;
    cnr_param_t cnr3Param;
    sprd_yuv_denoise_param_t denoise_param;
    sprd_yuv_denoise_cmd_t mode;
    cmr_bzero(&ynrParam, sizeof(YNR_Param));
    cmr_bzero(&cnr2Param, sizeof(CNR_Parameter));
    cmr_bzero(&cnr3Param, sizeof(cnr_param_t));
    cmr_bzero(&denoise_param, sizeof(sprd_yuv_denoise_param_t));

    if (!cxt || !cnr_handle->handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGV("E ");
    if (!cnr_handle->is_inited) {
        return ret;
    }
    mode = cxt->nr_flag - 1;
    sem_wait(&cnr_handle->sem);
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
        dump_image("cnr_transfer_frame_before_cnr", CAM_IMG_FMT_YUV420_NV21, denoise_param.width,
                   denoise_param.height, 0, addr, (denoise_param.width) * (denoise_param.height) * 3 / 2);
    }

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = cnr_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &cnr_handle->common.ipm_cxt->init_in;
    CMR_LOGI("cxt->nr_flag %d", cxt->nr_flag);
    if (cxt->nr_flag & NR_ENABLE) {  /*YNR or YNR_CNR*/
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_YNRS_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
        memcpy(&ynrParam, &isp_cmd_parm.ynr_param, sizeof(YNR_Param));
        denoise_param.ynrParam = &ynrParam;
        if (cxt->nr_flag == CNR_YNR_ENABLE) {    /*YNR_CNR*/
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                        &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {   
                CMR_LOGE("failed to get isp CNR param  %ld", ret);
                goto exit;
            }
            memcpy(&cnr2Param, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
            denoise_param.cnr2Param = &cnr2Param;
            denoise_param.cnr3Param = &cnr3Param;
        } else {
            denoise_param.cnr2Param = NULL;
        }
    } else {   /*CNR*/
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
        memcpy(&cnr2Param, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
        denoise_param.cnr2Param = &cnr2Param;
        denoise_param.cnr3Param = &cnr3Param;
        denoise_param.ynrParam = NULL;
    }
    char prop[PROPERTY_VALUE_MAX];
    property_get("debug.dump.nr.mode", prop, "0");
    if (atoi(prop) != 0) {
        mode = atoi(prop) - 1;
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
        dump_image("cnr_transfer_frame_after_cnr", CAM_IMG_FMT_YUV420_NV21, denoise_param.width,
                   denoise_param.height, 1, addr, (denoise_param.width) * (denoise_param.height) * 3 / 2);
    }

exit:
    CMR_LOGV("X");

    sem_post(&cnr_handle->sem);
    return ret;
}

#endif
#endif
