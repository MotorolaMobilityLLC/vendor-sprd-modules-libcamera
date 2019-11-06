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

#define LOG_TAG "cmr_rotate"

#include <fcntl.h>
#include <sys/ioctl.h>
#include "cmr_type.h"
#include "cmr_cvt.h"
#include "sprd_cpp.h"
#include "cpp_u_dev.h"

struct rot_file {
    cmr_int fd;
};

static unsigned int cmr_rot_fmt_cvt(cmr_u32 cmr_fmt) {
    unsigned int fmt = ROT_FMT_MAX;

    switch (cmr_fmt) {
    case CAM_IMG_FMT_YUV422P:
        fmt = ROT_YUV422;
        break;

    case CAM_IMG_FMT_YUV420_NV21:
    case CAM_IMG_FMT_YUV420_NV12:
        fmt = ROT_YUV420;
        break;

    default:
        break;
    }

    return fmt;
}

cmr_int cmr_rot_open(cmr_handle *rot_handle) {
    return cpp_rot_open(rot_handle);
}

cmr_int cmr_rot(struct cmr_rot_param *rot_param) {
    struct sprd_cpp_rot_cfg_parm rot_cfg;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    enum img_angle angle;
    struct img_frm *src_img;
    struct img_frm *dst_img;
    struct cpp_rot_param cpp_rot_params;
    cmr_int fd;
    struct rot_file *file = NULL;

    cpp_rot_params.host_fd = -1;

    if (!rot_param) {
        CMR_LOGE("Invalid Param!");
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    cpp_rot_params.handle = rot_param->handle;

    angle = rot_param->angle;
    src_img = &rot_param->src_img;
    dst_img = &rot_param->dst_img;

    if (NULL == src_img || NULL == dst_img) {
        CMR_LOGE("Wrong parameter 0x%lx 0x%lx", (cmr_uint)src_img,
                 (cmr_uint)dst_img);
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    CMR_LOGI("angle %ld, src fd 0x%x, w h %ld %ld, dst fd 0x%x", (cmr_int)angle,
             src_img->fd, (cmr_int)src_img->size.width,
             (cmr_int)src_img->size.height, dst_img->fd);

    if ((cmr_u32)angle < (cmr_u32)(IMG_ANGLE_90)) {
        CMR_LOGE("Wrong angle %ld", (cmr_int)angle);
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    rot_cfg.format = cmr_rot_fmt_cvt(src_img->fmt);
    if (rot_cfg.format >= ROT_FMT_MAX) {
        CMR_LOGE("Unsupported format %d, %d", src_img->fmt, rot_cfg.format);
        ret = -CMR_CAMERA_FAIL;
        goto rot_exit;
    }

    rot_cfg.angle = angle - IMG_ANGLE_90 + ROT_90;
    rot_cfg.src_addr.y = (uint32_t)src_img->addr_phy.addr_y;
    rot_cfg.src_addr.u = (uint32_t)src_img->addr_phy.addr_u;
    rot_cfg.src_addr.v = (uint32_t)src_img->addr_phy.addr_v;
    rot_cfg.src_addr.mfd[0] = src_img->fd;
    rot_cfg.src_addr.mfd[1] = src_img->fd;
    rot_cfg.src_addr.mfd[2] = src_img->fd;
    rot_cfg.dst_addr.y = (uint32_t)dst_img->addr_phy.addr_y;
    rot_cfg.dst_addr.u = (uint32_t)dst_img->addr_phy.addr_u;
    rot_cfg.dst_addr.v = (uint32_t)dst_img->addr_phy.addr_v;
    rot_cfg.dst_addr.mfd[0] = dst_img->fd;
    rot_cfg.dst_addr.mfd[1] = dst_img->fd;
    rot_cfg.dst_addr.mfd[2] = dst_img->fd;
    rot_cfg.size.w = (cmr_u16)src_img->size.width;
    rot_cfg.size.h = (cmr_u16)src_img->size.height;
    rot_cfg.src_endian = src_img->data_end.uv_endian;
    rot_cfg.dst_endian = dst_img->data_end.uv_endian;

    cpp_rot_params.rot_cfg_param = &rot_cfg;

    ret = cpp_rot_start(&cpp_rot_params);

rot_exit:
    CMR_LOGV("X ret=%ld", ret);
    return ret;
}

cmr_int cmr_rot_close(cmr_handle rot_handle) {
    return cpp_rot_close(rot_handle);
}
