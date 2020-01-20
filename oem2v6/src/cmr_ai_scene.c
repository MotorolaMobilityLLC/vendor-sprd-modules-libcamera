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

#define LOG_TAG "cmr_ai_scene"

#include <cutils/trace.h>
#include "cmr_oem.h"
#include "cmr_ipm.h"
#include <time.h>

#define SMALL_PIC_SIZE 228

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

struct class_ai_scene {
    struct ipm_common common;
    cmr_uint small_width;
    cmr_uint small_height;
    cmr_uint small_buf_phy;
    cmr_uint small_buf_vir;
    cmr_s32 small_buf_fd;
    cmr_uint is_closed;
    sem_t sem_ai_scene;
};

static cmr_int ai_scene_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *out_class_handle);
static cmr_int ai_scene_close(cmr_handle class_handle);
static cmr_int ai_scene_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out);
static cmr_int ai_scene_pre_proc(cmr_handle class_handle);
static cmr_int ai_scene_post_proc(cmr_handle class_handle);

static struct class_ops ai_scene_ops_tab_info = {
    ai_scene_open,     ai_scene_close,     ai_scene_transfer_frame,
    ai_scene_pre_proc, ai_scene_post_proc,
};

struct class_tab_t ai_scene_tab_info = {
    &ai_scene_ops_tab_info,
};

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            CMR_LOGE("invalid handle");                                        \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

static cmr_int ai_scene_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *out_class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle oem_handle = NULL;
    struct class_ai_scene *ai_scene_handle = NULL;
    struct ipm_context_t *ipm_cxt = (struct ipm_context_t *)ipm_handle;
    struct ipm_init_in *ipm_in = (struct ipm_init_in *)&ipm_cxt->init_in;
    cmr_u32 small_buf_size;
    cmr_u32 small_buf_num;

    CMR_LOGD("E");
    if (!out || !in || !ipm_handle || !out_class_handle) {
        CMR_LOGE("Invalid Param!");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ai_scene_handle =
        (struct class_ai_scene *)malloc(sizeof(struct class_ai_scene));
    if (!ai_scene_handle) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_bzero(ai_scene_handle, sizeof(struct class_ai_scene));
    sem_init(&ai_scene_handle->sem_ai_scene, 0, 1);

    ai_scene_handle->small_height = SMALL_PIC_SIZE;
    ai_scene_handle->small_width = SMALL_PIC_SIZE;
    small_buf_size =
        ai_scene_handle->small_width * ai_scene_handle->small_height * 3 / 2;
    small_buf_num = 1;

    ai_scene_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    ai_scene_handle->common.class_type = IPM_TYPE_AI_SCENE;
    ai_scene_handle->common.ops = &ai_scene_ops_tab_info;
    oem_handle = ai_scene_handle->common.ipm_cxt->init_in.oem_handle;
    ret = ipm_in->ops.mem_malloc(
        CAMERA_PREVIEW_SCALE_AI_SCENE, oem_handle, &small_buf_size,
        &small_buf_num, &ai_scene_handle->small_buf_phy,
        &ai_scene_handle->small_buf_vir, &ai_scene_handle->small_buf_fd);
    if (ret) {
        CMR_LOGE("Fail to malloc buffers for small image");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    *out_class_handle = ai_scene_handle;
exit:
    if (ret) {
        if (ai_scene_handle) {
            if (ai_scene_handle->small_buf_phy) {
                ipm_in->ops.mem_free(CAMERA_PREVIEW_SCALE_AI_SCENE, oem_handle,
                                     &ai_scene_handle->small_buf_phy,
                                     &ai_scene_handle->small_buf_vir,
                                     &ai_scene_handle->small_buf_fd, 1);
            }
            sem_destroy(&ai_scene_handle->sem_ai_scene);
            free(ai_scene_handle);
            ai_scene_handle = NULL;
        }
    }
    CMR_LOGD("X");
    return ret;
}

static cmr_int ai_scene_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_ai_scene *ai_scene_handle =
        (struct class_ai_scene *)class_handle;
    cmr_int i;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;
    struct ipm_init_in *ipm_in = NULL;
    CHECK_HANDLE_VALID(ai_scene_handle);

    CMR_LOGD("E");
    oem_handle = ai_scene_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    ipm_in = &ai_scene_handle->common.ipm_cxt->init_in;

    sem_wait(&ai_scene_handle->sem_ai_scene);
    ai_scene_handle->is_closed = 1;

    ret = ipm_in->ops.mem_free(CAMERA_PREVIEW_SCALE_AI_SCENE, oem_handle,
                               &ai_scene_handle->small_buf_phy,
                               &ai_scene_handle->small_buf_vir,
                               &ai_scene_handle->small_buf_fd, 1);
    if (ret) {
        CMR_LOGE("Fail to free the small image buffers");
    }

    sem_post(&ai_scene_handle->sem_ai_scene);
    sem_destroy(&ai_scene_handle->sem_ai_scene);
    free(ai_scene_handle);
    ai_scene_handle = NULL;

    CMR_LOGD("X");
    return ret;
}

static cmr_int ai_scene_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    struct img_frm src, dst;
    struct cmr_op_mean mean;
    cmr_u32 cur_frm_idx;
    cmr_u32 crop_size;
    struct class_ai_scene *ai_scene_handle =
        (struct class_ai_scene *)class_handle;
    struct prev_ai_scene_info *info = NULL;
    cmr_handle oem_handle = NULL;
    struct ipm_init_in *ipm_in = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    struct setting_cmd_parameter setting_param;
    cmr_bzero(&src, sizeof(struct img_frm));
    cmr_bzero(&dst, sizeof(struct img_frm));
    cmr_bzero(&mean, sizeof(struct cmr_op_mean));
    cmr_bzero(&isp_cmd_parm, sizeof(struct common_isp_cmd_param));
    cmr_bzero(&setting_param, sizeof(struct setting_cmd_parameter));
    cmr_bzero(&setting_param, sizeof(setting_param));

    CHECK_HANDLE_VALID(ai_scene_handle);
    ipm_in = &ai_scene_handle->common.ipm_cxt->init_in;

    info = (struct prev_ai_scene_info *)(in->private_data);
    if (!info) {
        CMR_LOGE("get prev_ai_scene_info error");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    sem_wait(&ai_scene_handle->sem_ai_scene);
    if (ai_scene_handle->is_closed) {
        goto exit;
    }
    oem_handle = ai_scene_handle->common.ipm_cxt->init_in.oem_handle;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    isp_cmd_parm.ai_img_status.frame_id = info->data.frame_num;
    ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_AI_SCENE_IMAGE_REQ_FLAG,
                                &isp_cmd_parm);

    setting_param.camera_id = cxt->camera_id;
    ret =
        cmr_setting_ioctl(setting_cxt->setting_handle,
                          CAMERA_PARAM_GET_DEVICE_ORIENTATION, &setting_param);
    if (setting_param.cmd_type_value == 90) {
        setting_param.cmd_type_value = 1;
    } else if (setting_param.cmd_type_value == 180) {
        setting_param.cmd_type_value = 2;
    } else if (setting_param.cmd_type_value == 270) {
        setting_param.cmd_type_value = 3;
    }

    if (ret) {
        CMR_LOGE("get image flag fail, ret:%d", ret);
        goto exit;
    }
    if (isp_cmd_parm.ai_img_status.img_flag != ISP_IMAGE_DATA_REQUIRED) {
        CMR_LOGV("ai engine doesn't need image.");
        goto exit;
    }

    /*1. scale to 228x228*/
    src = in->src_frame;
    src.data_end.y_endian = 0;
    src.data_end.uv_endian = 0;
    crop_size = min(in->src_frame.size.width, in->src_frame.size.height);
    src.rect.start_x = (in->src_frame.size.width - crop_size + 1) >> 1;
    src.rect.start_y = (in->src_frame.size.height - crop_size + 1) >> 1;
    src.rect.width = crop_size;
    src.rect.height = crop_size;
    memcpy(&dst, &src, sizeof(struct img_frm));
    dst.buf_size =
        ai_scene_handle->small_width * ai_scene_handle->small_height * 3 / 2;
    dst.rect.start_x = 0;
    dst.rect.start_y = 0;
    dst.rect.width = ai_scene_handle->small_width;
    dst.rect.height = ai_scene_handle->small_height;
    dst.size.width = ai_scene_handle->small_width;
    dst.size.height = ai_scene_handle->small_height;
    dst.addr_phy.addr_y = ai_scene_handle->small_buf_phy;
    dst.addr_phy.addr_u =
        dst.addr_phy.addr_y +
        ai_scene_handle->small_width * ai_scene_handle->small_height;
    dst.addr_phy.addr_v = dst.addr_phy.addr_u;
    dst.addr_vir.addr_y = ai_scene_handle->small_buf_vir;
    dst.addr_vir.addr_u =
        dst.addr_vir.addr_y +
        ai_scene_handle->small_width * ai_scene_handle->small_height;
    dst.addr_vir.addr_v = dst.addr_vir.addr_u;
    dst.fd = ai_scene_handle->small_buf_fd;
    mean.is_sync = 1;

    int64_t time = systemTime(CLOCK_MONOTONIC);
    ret = ipm_in->ops.img_scale(oem_handle, ai_scene_handle, &src, &dst, &mean);
    if (ret) {
        CMR_LOGE("Fail to scale image, ret:%d", ret);
        goto exit;
    }
    time = systemTime(CLOCK_MONOTONIC) - time;
    CMR_LOGV("scale src(wxh:%dx%d) to dst(wxh:%dx%d) cost  :%lld ms",
             src.rect.width, src.rect.height, dst.size.width, dst.size.height,
             ns2ms(time));

    /*2. send image to isp*/
    isp_cmd_parm.ai_img_param.frame_id = info->data.frame_num;
    isp_cmd_parm.ai_img_param.img_buf.img_y = dst.addr_vir.addr_y;
    isp_cmd_parm.ai_img_param.img_buf.img_uv = dst.addr_vir.addr_u;
    isp_cmd_parm.ai_img_param.timestamp =
        info->data.sec * 1000000000LL + info->data.usec * 1000;
    isp_cmd_parm.ai_img_param.height = SMALL_PIC_SIZE;
    isp_cmd_parm.ai_img_param.width = SMALL_PIC_SIZE;
    isp_cmd_parm.ai_img_param.img_y_pitch = SMALL_PIC_SIZE;
    isp_cmd_parm.ai_img_param.img_uv_pitch = SMALL_PIC_SIZE;
    isp_cmd_parm.ai_img_param.is_continuous = 1;
    isp_cmd_parm.ai_img_param.orientation = setting_param.cmd_type_value;
    ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_SET_AI_SCENE_IMAGE,
                                &isp_cmd_parm);
    if (ret) {
        CMR_LOGE("Fail to set image, ret:%d", ret);
        goto exit;
    }
    CMR_LOGD(
        "img_param.timestamp:%lld  img_param.frame_id :%u, ai_orientation: %d",
        isp_cmd_parm.ai_img_param.timestamp, isp_cmd_parm.ai_img_param.frame_id,
        isp_cmd_parm.ai_img_param.orientation);
exit:
    sem_post(&ai_scene_handle->sem_ai_scene);
    return ret;
}

static cmr_int ai_scene_pre_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/
    (void)class_handle;

    return ret;
}

static cmr_int ai_scene_post_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/
    (void)class_handle;

    return ret;
}
