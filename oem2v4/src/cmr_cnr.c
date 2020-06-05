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

struct class_tab_t cnr_tab_info = {
    &cnr_ops_tab_info,
};

#ifdef CAMERA_CNR3_ENABLE
enum nr_type {
    YNRS_ENABLE = (1 << 0),
    CNR2_ENABLE = (1 << 1),
    CNR3_ENABLE = (1 << 2),
};
#else
enum nr_type {
    YNRS_ENABLE = (1 << 0),
    CNR2_ENABLE = (1 << 1),
};
#endif

static cmr_int cnr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_cnr *cnr_handle = NULL;
    int runversion = 1;

    ThreadSet threadSet;
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };

    if (!ipm_handle || !class_handle ||!in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGI("E");

    cnr_handle = (struct class_cnr *)malloc(sizeof(struct class_cnr));
    if (!cnr_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    int width = in->frame_size.width;
    int height =in->frame_size.height;
    cmr_bzero(cnr_handle, sizeof(struct class_cnr));

    cnr_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    cnr_handle->common.class_type = IPM_TYPE_CNR;

    cnr_handle->common.ops = &cnr_ops_tab_info;

    property_get("vendor.cam.cnr.threadnum", value, "4");
    threadSet.threadNum = atoi(value);
    property_get("vendor.cam.cnr.corebundle", value, "0");
    threadSet.coreBundle = atoi(value);

    cnr_handle->handle = sprd_cnr_init(width, height, runversion);
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

#ifdef CAMERA_CNR3_ENABLE
static cmr_int cnr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 valid_nr_type = 0;
    struct class_cnr *cnr_handle = (struct class_cnr *)class_handle;
    struct img_addr *addr;
    struct camera_context *cxt = NULL;
    cmr_uint width = 0;
    cmr_uint height = 0;
    denoise_buffer imgBuffer;
    Denoise_Param denoiseParam;
    YNR_Param ynrParam;
    CNR_Parameter cnrParam;
    cnr_param_t cnr3Param;
    float ratio;
    struct setting_cmd_parameter setting_param;

    cmr_bzero(&setting_param, sizeof(setting_param));
    cmr_bzero(&imgBuffer, sizeof(denoise_buffer));
    cmr_bzero(&denoiseParam, sizeof(Denoise_Param));
    cmr_bzero(&ynrParam, sizeof(YNR_Param));
    cmr_bzero(&cnrParam, sizeof(CNR_Parameter));

    if (!in->private_data || !cnr_handle->handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!cnr_handle->is_inited) {
        return ret;
    }

    cxt = (struct camera_context *)in->private_data;
    denoise_mode mode = cxt->nr_flag;

    struct setting_context *setting_cxt = &cxt->setting_cxt;
    setting_param.camera_id = cxt->camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_APPMODE,
                            &setting_param);
    if (ret) {
        CMR_LOGE("failed to get app mode %ld", ret);
    }
    CMR_LOGD("app_mode = %d", setting_param.cmd_type_value);
    if (setting_param.cmd_type_value != CAMERA_MODE_3DNR_PHOTO) {
        cxt->nightscepro_flag = 1;
    }

    sem_wait(&cnr_handle->sem);
    imgBuffer.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
    imgBuffer.bufferUV = (unsigned char *)in->src_frame.addr_vir.addr_u;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.before.docnr", value, "null");
    // addr = in->src_frame.addr_vir;
    if (!strcmp(value, "true")) {
        width = in->src_frame.size.width;
        height = in->src_frame.size.height;
        addr = &in->src_frame.addr_vir;
        camera_save_yuv_to_file(0, IMG_DATA_TYPE_YUV420, width, height, addr);
    }

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = cnr_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &cnr_handle->common.ipm_cxt->init_in;
    CMR_LOGI("cxt->nr_flag %d", cxt->nr_flag);
    if (cxt->nr_flag & YNRS_ENABLE) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_YNRS_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
        valid_nr_type |= YNRS_ENABLE;
        memcpy(&ynrParam, &isp_cmd_parm.ynr_param, sizeof(YNR_Param));
        ratio =((float)ynrParam.ynr_Radius)/((float)ynrParam.ynr_imgCenterX*2);
        CMR_LOGD("input radius =%d,center x =%d ,center y =%d",ynrParam.ynr_Radius,ynrParam.ynr_imgCenterX,ynrParam.ynr_imgCenterY);
        ynrParam.ynr_Radius = ratio * in->src_frame.size.width*cxt->zoom_ratio;
        if(ynrParam.ynr_Radius > in->src_frame.size.width) {
            ynrParam.ynr_Radius = in->src_frame.size.width;
        }
        ynrParam.ynr_imgCenterX = in->src_frame.size.width/2;
        ynrParam.ynr_imgCenterY = in->src_frame.size.height/2;
        denoiseParam.ynrParam = &ynrParam;
        CMR_LOGD("output zoom_ratio = %f,center x =%d,center y =%d,radius =%d",cxt->zoom_ratio,ynrParam.ynr_imgCenterX,ynrParam.ynr_imgCenterY,ynrParam.ynr_Radius);
    }

    if (cxt->nr_flag & CNR2_ENABLE) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp CNR param  %ld", ret);
            goto exit;
        }
        memcpy(&cnrParam, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
        denoiseParam.cnr2Param = &cnrParam;
        valid_nr_type |= CNR2_ENABLE;
    } else {
        denoiseParam.cnr2Param = NULL;
    }

    if (cxt->nr_flag & CNR3_ENABLE) {
        if(cxt->nightscepro_flag != 1) {
            valid_nr_type |= (!CNR3_ENABLE);
        } else {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR3_PARAM,
                                        &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp CNR param  %ld", ret);
                goto exit;
            }
            memcpy(&cnr3Param, &isp_cmd_parm.cnr3_param, sizeof(cnr_param_t));
            denoiseParam.cnr3Param = &cnr3Param;
            valid_nr_type |= CNR3_ENABLE;
        }
    } else {
        denoiseParam.cnr3Param = NULL;
    }

    if (valid_nr_type == 0) {
        CMR_LOGD("valid_nr_type is 0");
        goto exit;
    }

    mode = valid_nr_type;
    char prop[PROPERTY_VALUE_MAX];
    property_get("debug.dump.nr.mode", prop, "0");
    if (atoi(prop) != 0) {
        mode = atoi(prop) - 1;
    }
    cxt->nightscepro_flag = 0;
    CMR_LOGD("the mode value is %d", mode);
    ret = sprd_cnr_process(cnr_handle->handle, &imgBuffer, &denoiseParam, mode, in->src_frame.size.width, in->src_frame.size.height);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("failed to docnr %ld", ret);
        goto exit;
    }
    property_get("debug.dump.after.docnr", value, "null");
    if (!strcmp(value, "true")) {
        width = in->src_frame.size.width;
        height = in->src_frame.size.height;
        addr = &in->src_frame.addr_vir;
        camera_save_yuv_to_file(1, IMG_DATA_TYPE_YUV420, width, height, addr);
    }

exit:
    CMR_LOGI("X");

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
    struct camera_context *cxt = NULL;
    cmr_uint width = 0;
    cmr_uint height = 0;
    denoise_buffer imgBuffer;
    Denoise_Param denoiseParam;
    YNR_Param ynrParam;
    CNR_Parameter cnrParam;
    float ratio;

    cmr_bzero(&imgBuffer, sizeof(denoise_buffer));
    cmr_bzero(&denoiseParam, sizeof(Denoise_Param));
    cmr_bzero(&ynrParam, sizeof(YNR_Param));
    cmr_bzero(&cnrParam, sizeof(CNR_Parameter));

    if (!in->private_data || !cnr_handle->handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!cnr_handle->is_inited) {
        return ret;
    }

    cxt = (struct camera_context *)in->private_data;
    denoise_mode mode = cxt->nr_flag;

    sem_wait(&cnr_handle->sem);
    imgBuffer.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
    imgBuffer.bufferUV = (unsigned char *)in->src_frame.addr_vir.addr_u;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.before.docnr", value, "null");
    // addr = in->src_frame.addr_vir;
    if (!strcmp(value, "true")) {
        width = in->src_frame.size.width;
        height = in->src_frame.size.height;
        addr = &in->src_frame.addr_vir;
        camera_save_yuv_to_file(0, IMG_DATA_TYPE_YUV420, width, height, addr);
    }

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = cnr_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &cnr_handle->common.ipm_cxt->init_in;
    CMR_LOGI("cxt->nr_flag %d", cxt->nr_flag);
    if (cxt->nr_flag & 1) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_YNRS_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
        memcpy(&ynrParam, &isp_cmd_parm.ynr_param, sizeof(YNR_Param));
        ratio =((float)ynrParam.ynr_Radius)/((float)ynrParam.ynr_imgCenterX*2);
        CMR_LOGD("input radius =%d,center x =%d ,center y =%d",ynrParam.ynr_Radius,ynrParam.ynr_imgCenterX,ynrParam.ynr_imgCenterY);
        ynrParam.ynr_Radius = ratio * in->src_frame.size.width*cxt->zoom_ratio;
        if(ynrParam.ynr_Radius > in->src_frame.size.width) {
            ynrParam.ynr_Radius = in->src_frame.size.width;
        }
        ynrParam.ynr_imgCenterX = in->src_frame.size.width/2;
        ynrParam.ynr_imgCenterY = in->src_frame.size.height/2;
        denoiseParam.ynrParam = &ynrParam;
        CMR_LOGD("output zoom_ratio = %f,center x =%d,center y =%d,radius =%d",cxt->zoom_ratio,ynrParam.ynr_imgCenterX,ynrParam.ynr_imgCenterY,ynrParam.ynr_Radius);
        if (cxt->nr_flag == 3) {
            ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                        &isp_cmd_parm);
            if (CMR_CAMERA_SUCCESS != ret) {
                CMR_LOGE("failed to get isp CNR param  %ld", ret);
                goto exit;
            }
            memcpy(&cnrParam, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
            denoiseParam.cnr2Param = &cnrParam;
        } else {
            denoiseParam.cnr2Param = NULL;
        }

    } else {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_CNR2_PARAM,
                                    &isp_cmd_parm);
        if (CMR_CAMERA_SUCCESS != ret) {
            CMR_LOGE("failed to get isp YNR param  %ld", ret);
            goto exit;
        }
        memcpy(&cnrParam, &isp_cmd_parm.cnr2_param, sizeof(CNR_Parameter));
        denoiseParam.cnr2Param = &cnrParam;
        denoiseParam.ynrParam = NULL;
    }
    char prop[PROPERTY_VALUE_MAX];
    property_get("debug.dump.nr.mode", prop, "0");
    if (atoi(prop) != 0) {
        mode = atoi(prop) - 1;
    }
    ret = sprd_cnr_process(cnr_handle->handle, &imgBuffer, &denoiseParam, mode, in->src_frame.size.width, in->src_frame.size.height);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("failed to docnr %ld", ret);
        goto exit;
    }
    property_get("debug.dump.after.docnr", value, "null");
    if (!strcmp(value, "true")) {
        width = in->src_frame.size.width;
        height = in->src_frame.size.height;
        addr = &in->src_frame.addr_vir;
        camera_save_yuv_to_file(1, IMG_DATA_TYPE_YUV420, width, height, addr);
    }

exit:
    CMR_LOGI("X");

    sem_post(&cnr_handle->sem);
    return ret;
}

#endif
#endif
