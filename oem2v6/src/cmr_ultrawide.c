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

#ifdef CONFIG_CAMERA_SUPPORT_ULTRA_WIDE

#define LOG_TAG "cmr_wide"
#include <cutils/trace.h>
#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#include <cutils/properties.h>
#include "isp_mw.h"
#include "sprd_img_warp.h"

struct class_ultrawide {
    struct ipm_common common;
    img_warp_param_t warp_param;
    img_warp_inst_t warp_inst;
    INST_TAG tag;
    bool is_isp_zoom;
};

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return -CMR_CAMERA_INVALID_PARAM;                                  \
        }                                                                      \
    } while (0)

#define IMAGE_FORMAT "YVU420_SEMIPLANAR"
#define CAMERA_wide_MSG_QUEUE_SIZE 5

#define CMR_EVT_wide_BASE (CMR_EVT_IPM_BASE + 0X100)
#define CMR_EVT_wide_INIT (CMR_EVT_wide_BASE + 0)
#define CMR_EVT_wide_START (CMR_EVT_wide_BASE + 1)
#define CMR_EVT_wide_EXIT (CMR_EVT_wide_BASE + 2)
#define CMR_EVT_wide_SAVE_FRAME (CMR_EVT_wide_BASE + 3)

static cmr_int ultrawide_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                              struct ipm_open_out *out,
                              cmr_handle *class_handle);
static cmr_int ultrawide_close(cmr_handle class_handle);
static cmr_int ultrawide_transfer_frame(cmr_handle class_handle,
                                        struct ipm_frame_in *in,
                                        struct ipm_frame_out *out);

static struct class_ops ultrawide_ops_tab_info = {
    ultrawide_open, ultrawide_close, ultrawide_transfer_frame, NULL, NULL};

struct class_tab_t ultrawide_tab_info = {
    &ultrawide_ops_tab_info,
};

static cmr_int ultrawide_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                              struct ipm_open_out *out,
                              cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_ultrawide *ultrawide_handle;

    ultrawide_handle =
        (struct class_ultrawide *)malloc(sizeof(struct class_ultrawide));

    if (!ultrawide_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(ultrawide_handle, sizeof(struct class_ultrawide));
    ultrawide_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    ultrawide_handle->common.class_type = IPM_TYPE_ULTRA_WIDE;
    ultrawide_handle->common.ops = &ultrawide_ops_tab_info;

    img_warp_grid_config_default(&ultrawide_handle->warp_param);
    ultrawide_handle->warp_param.otp_buf = in->otp_data.otp_ptr;
    ultrawide_handle->warp_param.otp_size = in->otp_data.otp_size;
    ultrawide_handle->warp_param.input_info.input_width = in->frame_size.width;
    ultrawide_handle->warp_param.input_info.input_height = in->frame_size.height;
    ultrawide_handle->warp_param.input_info.crop_x = in->frame_rect.start_x;
    ultrawide_handle->warp_param.input_info.crop_y = in->frame_rect.start_y;
    ultrawide_handle->warp_param.input_info.crop_width = in->frame_rect.width;
    ultrawide_handle->warp_param.input_info.crop_height = in->frame_rect.height;
    ultrawide_handle->warp_param.input_info.fullsize_width =
        in->sensor_size.width;
    ultrawide_handle->warp_param.input_info.fullsize_height =
        in->sensor_size.height;
    if (2 == in->binning_factor) {
        ultrawide_handle->warp_param.input_info.binning_mode = 1;
    } else {
        ultrawide_handle->warp_param.input_info.binning_mode = 0;
    }
    ultrawide_handle->warp_param.dst_width = in->frame_size.width;
    ultrawide_handle->warp_param.dst_height = in->frame_size.height;
    if (in->is_cap)
        ultrawide_handle->tag = WARP_CAPTURE;
    else
        ultrawide_handle->tag = WARP_PREVIEW;
    if (in->adgain) {//eis warp
        ultrawide_handle->warp_param.corr_level = 1.0f;
        ultrawide_handle->warp_param.warp_mode = WARP_PROJECTIVE;
    }

    CMR_LOGD("ultra wide open:param:%p,fullsize=%d,%d, size:%dx%d cx:%d, "
             "cy:%d,cw:%d,ch:%d,binning:%d",
             &ultrawide_handle->warp_param, in->sensor_size.width,
             in->sensor_size.height, ultrawide_handle->warp_param.dst_width,
             ultrawide_handle->warp_param.dst_height, in->frame_rect.start_x,
             in->frame_rect.start_y, in->frame_rect.width,
             in->frame_rect.height, in->binning_factor);
    sprd_warp_adapter_open(
        &ultrawide_handle->warp_inst, &ultrawide_handle->is_isp_zoom,
        &ultrawide_handle->warp_param, ultrawide_handle->tag);
    out->isp_zoom = ultrawide_handle->is_isp_zoom;
    *class_handle = (cmr_handle)ultrawide_handle;

    return ret;
}

static cmr_int ultrawide_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_ultrawide *ultrawide_handle =
        (struct class_ultrawide *)class_handle;

    CHECK_HANDLE_VALID(ultrawide_handle);

    if (ultrawide_handle->warp_inst != NULL) {
        CMR_LOGD("ultra wide close:param:%p, size:%dx%d",
                 &ultrawide_handle->warp_param,
                 ultrawide_handle->warp_param.dst_width,
                 ultrawide_handle->warp_param.dst_height);
        sprd_warp_adapter_close(&ultrawide_handle->warp_inst,
                                ultrawide_handle->tag);
    }

    free(ultrawide_handle);

    return ret;
}

static cmr_int ultrawide_transfer_frame(cmr_handle class_handle,
                                        struct ipm_frame_in *in,
                                        struct ipm_frame_out *out) {
    struct class_ultrawide *ultrawide_handle =
        (struct class_ultrawide *)class_handle;

    img_warp_buffer_t input;
    img_warp_buffer_t output;
    img_warp_undistort_param_t param;
    ipm_param_t param_t;
    struct eiswarp mat;
    struct img_frm *dst_img = NULL;
    struct img_frm *src_img = NULL;
    char value[PROPERTY_VALUE_MAX];
    struct zoom_info zoomInfo;

    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    cmr_bzero(&param, sizeof(img_warp_undistort_param_t));
    param.zoomRatio = 1.0f;
    src_img = &in->src_frame;
    dst_img = &in->dst_frame;
    property_get("debug.dump.ultrawide.frame", value, "null");
    if (!strcmp(value, "true")) {
        dump_image("before", CAM_IMG_FMT_YUV420_NV21,
                           src_img->size.width, src_img->size.height,
                           src_img->frame_number,
                           &src_img->addr_vir,
                           src_img->size.width * src_img->size.height * 3 / 2);
    }

    if (in->ae_exp_gain_info != NULL) {//eis warp
        mat = *(struct eiswarp *)(in->ae_exp_gain_info);
        for (int i =0; i< 9; i++) {
            param.warp_projective[i/3][i%3] = mat.warp[i];
        }
        CMR_LOGV("warp_projective %f %f %f %f %f %f %f %f %f",
            param.warp_projective[0][0], param.warp_projective[0][1], param.warp_projective[0][2],
            param.warp_projective[1][0], param.warp_projective[1][1], param.warp_projective[1][2],
            param.warp_projective[2][0], param.warp_projective[2][1], param.warp_projective[2][2]);
        param.zoomRatio = 1.0f;
        param.zoomCenterOffsetX = 0.0f;
        param.zoomCenterOffsetY = 0.0f;
        param.input_info = ultrawide_handle->warp_param.input_info;
    } else if ((cmr_uint)in->private_data) {
        int x, y;
        param_t = *(ipm_param_t *)(in->private_data);
        memcpy(&zoomInfo, &param_t.zoom, sizeof(struct zoom_info));

        x = zoomInfo.crop_region.start_x + zoomInfo.crop_region.width / 2;
        y = zoomInfo.crop_region.start_y + zoomInfo.crop_region.height / 2;
        x -= zoomInfo.pixel_size.width / 2;
        y -= zoomInfo.pixel_size.height / 2;

        if (zoomInfo.crop_region.width == 0) {
            zoomInfo.crop_region.width = zoomInfo.pixel_size.width;
        }
        param.zoomRatio = (float)zoomInfo.pixel_size.width / (float)zoomInfo.crop_region.width;
        param.zoomCenterOffsetX = (float)x / ((float)zoomInfo.pixel_size.width / 2);
        param.zoomCenterOffsetY = (float)y / ((float)zoomInfo.pixel_size.height / 2);
        param.input_info.fullsize_height = param_t.fullsize_height;
        param.input_info.fullsize_width = param_t.fullsize_width;
        param.input_info.input_height = param_t.input_height;
        param.input_info.input_width = param_t.input_width;
        param.input_info.crop_x = CAMERA_START(param_t.crop_x);
        param.input_info.crop_y = CAMERA_START(param_t.crop_y);
        param.input_info.crop_width = CAMERA_START(param_t.crop_width + (param_t.crop_x & 1));
        param.input_info.crop_height = CAMERA_START(param_t.crop_height + (param_t.crop_y & 1));
    } else {
        CMR_LOGE("wrong ultra wide param.");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD("tag %d, zoom info %f %f %f, %f; size %d %d, crop %d %d %d %d\n",
        ultrawide_handle->tag, zoomInfo.zoom_ratio, zoomInfo.prev_aspect_ratio,
        zoomInfo.video_aspect_ratio, zoomInfo.capture_aspect_ratio,
        zoomInfo.pixel_size.width, zoomInfo.pixel_size.height,
        zoomInfo.crop_region.start_x, zoomInfo.crop_region.start_y,
        zoomInfo.crop_region.width, zoomInfo.crop_region.height);
    CMR_LOGD("param ratio %f, cent_off (%f %f), (%d %d), (%d %d), (%d %d %d %d)\n",
        param.zoomRatio, param.zoomCenterOffsetX, param.zoomCenterOffsetY,
        param.input_info.fullsize_width, param.input_info.fullsize_height,
        param.input_info.input_width, param.input_info.input_height,
        param.input_info.crop_x, param.input_info.crop_y,
        param.input_info.crop_width, param.input_info.crop_height);

    if (ultrawide_handle->warp_inst != NULL) {
        input.width = src_img->size.width;
        input.height = src_img->size.height;
        input.stride = src_img->size.width;
        input.graphic_handle = src_img->reserved;
        input.ion_fd = src_img->fd;
        input.addr[0] = (void *)src_img->addr_vir.addr_y;

        output.width = dst_img->size.width;
        output.height = dst_img->size.height;
        output.stride = dst_img->size.width;
        output.graphic_handle = dst_img->reserved;
        output.ion_fd = dst_img->fd;
        output.addr[0] = (void *)dst_img->addr_vir.addr_y;

        CMR_LOGD("in fd=0x%x, addr %p, gpu_ptr %p, (%d %d), out fd=0x%x, addr %p, gpu_ptr %p, (%d %d)\n",
            input.ion_fd, input.addr[0], input.graphic_handle, input.width, input.height,
            output.ion_fd, output.addr[0], output.graphic_handle, output.width, output.height);

        sprd_warp_adapter_run(ultrawide_handle->warp_inst, &input, &output,
                              (void *)&param, ultrawide_handle->tag);
        if (!strcmp(value, "true")) {
            dump_image("after", CAM_IMG_FMT_YUV420_NV21,
                               src_img->size.width, src_img->size.height,
                               src_img->frame_number,
                               &dst_img->addr_vir,
                               src_img->size.width * src_img->size.height * 3 / 2);
        }
    } else {
        cmr_copy((void *)dst_img->addr_vir.addr_y,
                 (void *)src_img->addr_vir.addr_y, dst_img->buf_size);
    }

    return ret;
}
#endif
