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
#define LOG_TAG "cmr_sprd_dre"
#include "cmr_common.h"
#include "cmr_oem.h"
#include "sprd_dre_adapter.h"
#include "isp_mw.h"

struct class_dre {
    struct ipm_common common;
    cmr_uint is_inited;
    cmr_uint height;
    cmr_uint width;
    void *handle;
    sem_t sem;
};
static cmr_int dre_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle);
static cmr_int dre_close(cmr_handle class_handle);
static cmr_int dre_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out);

static struct class_ops dre_ops_tab_info = {
    dre_open, dre_close, dre_transfer_frame, NULL, NULL,
};

struct class_tab_t dre_tab_info = {
    &dre_ops_tab_info,
};

static cmr_int dre_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_dre *dre_handle = NULL;
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };

    if (!ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGI("E");

    dre_handle = (struct class_dre *)malloc(sizeof(struct class_dre));
    if (!dre_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(dre_handle, sizeof(struct class_dre));

    dre_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    dre_handle->common.class_type = IPM_TYPE_DRE;

    dre_handle->common.ops = &dre_ops_tab_info;
    dre_handle->height = in->frame_size.height;
    dre_handle->width = in->frame_size.width;

    CMR_LOGI("sprd_dre_init height=%d,width = %d", dre_handle->height,
             dre_handle->width);

    ret = sprd_dre_adpt_init(&dre_handle->handle, dre_handle->width,
                        dre_handle->height, NULL);

    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("failed to create");
        goto exit;
    }

    dre_handle->is_inited = 1;
    sem_init(&dre_handle->sem, 0, 1);

    *class_handle = (cmr_handle)dre_handle;

    CMR_LOGI(" x ");

    return ret;

exit:
    if (NULL != dre_handle) {
        free(dre_handle);
        dre_handle = NULL;
    }
    return CMR_CAMERA_FAIL;
}

static cmr_int dre_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_dre *dre_handle = (struct class_dre *)class_handle;
    if (!dre_handle || !dre_handle->handle) {
        CMR_LOGE("dre_handle is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGI("E");
    if (dre_handle->is_inited) {
        sem_wait(&dre_handle->sem);
        sprd_dre_adpt_ctrl(dre_handle->handle, SPRD_DRE_FAST_STOP_CMD, NULL, NULL);
        ret = sprd_dre_adpt_deinit(dre_handle->handle);
        if (ret) {
            CMR_LOGE("failed to deinit");
        }
        sem_post(&dre_handle->sem);
        sem_destroy(&dre_handle->sem);
    }
    CMR_LOGD("deinit success");

    dre_handle->is_inited = 0;
    free(dre_handle);
    class_handle = NULL;

    CMR_LOGI("X ");

    return ret;
}

static cmr_int dre_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_dre *dre_handle = (struct class_dre *)class_handle;
    struct img_addr *addr;
    cmr_uint width = 0;
    cmr_uint height = 0;
    struct camera_context *cxt = (struct camera_context *)in->private_data;
    struct sprd_camalg_image image_in;
    if ( !dre_handle->handle || !cxt) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGI("E ");
    if (!dre_handle->is_inited) {
        return ret;
    }
    sem_wait(&dre_handle->sem);

    addr = &in->src_frame.addr_vir;
    width = in->src_frame.size.width;
    height = in->src_frame.size.height;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.dodre", value, "null");
    if (!strcmp(value, "true")) {
        dump_image("dre", CAM_IMG_FMT_YUV420_NV21, width, height, 1, addr,
                   width * height * 3 / 2);
    }
    CMR_LOGD("w=%lu,h=%lu, addr= %p", width, height, addr);

    cmr_handle oem_handle = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    oem_handle = dre_handle->common.ipm_cxt->init_in.oem_handle;

    struct ipm_init_in *ipm_in = &dre_handle->common.ipm_cxt->init_in;

    ret =
        ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_DRE_PARAM, &isp_cmd_parm);

    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGI("no parameters ,bypass dre");
        goto exit;
    } else {
        CMR_LOGI("success to get isp param  %ld", ret);
    }
    image_in.addr[0] = (void *)addr->addr_y;
    image_in.height = height;
    image_in.width = width;
    image_in.format = SPRD_CAMALG_IMG_NV21;
    CMR_LOGV("predre enable=%d ", isp_cmd_parm.dre_param.predre_param.enable);
    CMR_LOGV("predre imgKey_setting_mode=%d ",
             isp_cmd_parm.dre_param.predre_param.imgKey_setting_mode);
    CMR_LOGV("predre tarNorm_setting_mode=%d ",
             isp_cmd_parm.dre_param.predre_param.tarNorm_setting_mode);
    CMR_LOGV("predre target_norm=%d ",
             isp_cmd_parm.dre_param.predre_param.target_norm);
    CMR_LOGV("predre imagekey=%d ",
             isp_cmd_parm.dre_param.predre_param.imagekey);
    CMR_LOGV("predre min_per=%d ", isp_cmd_parm.dre_param.predre_param.min_per);
    CMR_LOGV("predre max_per=%d ", isp_cmd_parm.dre_param.predre_param.max_per);
    CMR_LOGV("predre stat_step=%d ",
             isp_cmd_parm.dre_param.predre_param.stat_step);
    CMR_LOGV("predre low_thresh=%d ",
             isp_cmd_parm.dre_param.predre_param.low_thresh);
    CMR_LOGV("predre high_thresh=%d ",
             isp_cmd_parm.dre_param.predre_param.high_thresh);
    CMR_LOGV("predre tarCoeff=%d ",
             isp_cmd_parm.dre_param.predre_param.tarCoeff);

    CMR_LOGV("post enable=%d ", isp_cmd_parm.dre_param.postdre_param.enable);
    CMR_LOGV("post strength=%d ",
             isp_cmd_parm.dre_param.postdre_param.strength);
    CMR_LOGV("post texture_counter_en=%d ",
             isp_cmd_parm.dre_param.postdre_param.texture_counter_en);
    CMR_LOGV("post text_point_thres=%d ",
             isp_cmd_parm.dre_param.postdre_param.text_point_thres);
    CMR_LOGV("post text_prop_thres=%d ",
             isp_cmd_parm.dre_param.postdre_param.text_prop_thres);
    CMR_LOGV("post tile_num_auto=%d ",
             isp_cmd_parm.dre_param.postdre_param.tile_num_auto);
    CMR_LOGV("post tile_num_x=%d ",
             isp_cmd_parm.dre_param.postdre_param.tile_num_x);
    CMR_LOGV("post tile_num_y=%d ",
             isp_cmd_parm.dre_param.postdre_param.tile_num_y);
    ret = sprd_dre_adpt_ctrl(dre_handle->handle, SPRD_DRE_PROCESS_CMD, &image_in, &isp_cmd_parm.dre_param);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("failed to do dre %ld", ret);
        goto exit;
    }

    property_get("debug.dump.dodre", value, "null");
    if (!strcmp(value, "true")) {
        dump_image("dre", CAM_IMG_FMT_YUV420_NV21, width, height, 2, addr,
                   width * height * 3 / 2);
    }

exit:
    CMR_LOGI("X");

    sem_post(&dre_handle->sem);
    return ret;
}
