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
static void loadUltrawideOtp(struct class_ultrawide *ultrawide_handle);

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

    loadUltrawideOtp(ultrawide_handle);

    img_warp_grid_config_default(&ultrawide_handle->warp_param);

    ultrawide_handle->warp_param.input_info.input_width = in->frame_size.width;
    ultrawide_handle->warp_param.input_info.input_height =
        in->frame_size.height;

    ultrawide_handle->warp_param.input_info.crop_x = in->frame_rect.start_x;
    ultrawide_handle->warp_param.input_info.crop_y = in->frame_rect.start_y;
    ultrawide_handle->warp_param.input_info.crop_width = in->frame_rect.width;
    ultrawide_handle->warp_param.input_info.crop_height = in->frame_rect.height;

    ultrawide_handle->warp_param.dst_width = in->frame_size.width;
    ultrawide_handle->warp_param.dst_height = in->frame_size.height;

    CMR_LOGD(
        "ultra wide open:param:%p, size:%dx%d cx:%d, cy:%d,cw:%d,ch:%d",
        &ultrawide_handle->warp_param, ultrawide_handle->warp_param.dst_width,
        ultrawide_handle->warp_param.dst_height, in->frame_rect.start_x,
        in->frame_rect.start_y, in->frame_rect.width, in->frame_rect.height);
    img_warp_grid_open(&ultrawide_handle->warp_inst,
                       &ultrawide_handle->warp_param);

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
        img_warp_grid_close(&ultrawide_handle->warp_inst);
    }

    if (NULL != ultrawide_handle)
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
    struct img_frm *dst_img = NULL;
    struct img_frm *src_img = NULL;
    char value[PROPERTY_VALUE_MAX];

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
        camera_save_yuv_to_file(0, IMG_DATA_TYPE_YUV420, src_img->size.width,
                                src_img->size.height, &src_img->addr_vir);
    }

    if ((cmr_uint)in->private_data) {
        param.zoomRatio = *((float *)(in->private_data));
    }
    CMR_LOGV("ultrawid set ratio %f", param.zoomRatio);
    if (ultrawide_handle->warp_inst != NULL) {
        input.width = src_img->size.width;
        input.height = src_img->size.height;
        input.stride = src_img->size.width;
        input.graphic_handle = src_img->reserved;
        input.addr[0] = (void *)src_img->addr_vir.addr_y;

        output.width = dst_img->size.width;
        output.height = dst_img->size.height;
        output.stride = dst_img->size.width;
        output.graphic_handle = dst_img->reserved;
        output.addr[0] = (void *)dst_img->addr_vir.addr_y;

        img_warp_grid_run(ultrawide_handle->warp_inst, &input, &output,
                          (void *)&param);
        if (!strcmp(value, "true")) {
            camera_save_yuv_to_file(1, IMG_DATA_TYPE_YUV420,
                                    src_img->size.width, src_img->size.height,
                                    &dst_img->addr_vir);
        }

        CMR_LOGD("ultra wide algo done:param:%p, size:%dx%d",
                 &ultrawide_handle->warp_param,
                 ultrawide_handle->warp_param.dst_width,
                 ultrawide_handle->warp_param.dst_height);
    } else {
        cmr_copy((void *)dst_img->addr_vir.addr_y,
                 (void *)src_img->addr_vir.addr_y, dst_img->buf_size);
    }

    return 0;
}

static void loadUltrawideOtp(struct class_ultrawide *ultrawide_handle) {
    int rc = CMR_CAMERA_SUCCESS;
    uint32_t read_byte = 0;
    cmr_u8 otp_info[256] = {0};
    cmr_u8 otp_size = 0;
    struct class_ultrawide *handle = ultrawide_handle;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    FILE *fid = fopen("/mnt/vendor/productinfo/spwOTP.txt", "rb");
    if (NULL == fid) {
        CMR_LOGD("read ultra wide otp failed!");
        rc = -1;
    } else {
        cmr_u8 *otp_data = otp_info;
        while (!feof(fid)) {
            fscanf(fid, "%d\n", otp_data);
            otp_data += 4;
            read_byte += 4;
        }
        fclose(fid);
        otp_size = read_byte;
        handle->warp_param.otp_buf = otp_info;
        handle->warp_param.otp_size = otp_size;
        CMR_LOGD("ultra wide read_bytes=%d ", read_byte);

        property_get("persist.vendor.cam.dump.ultrawide", prop, "0");
        if (atoi(prop) == 1) {
            for (int i = 0; i < otp_size; i++)
                CMR_LOGD("ultra wid data [%d] = %d", i, otp_info[i]);
        }
    }
}

#endif
