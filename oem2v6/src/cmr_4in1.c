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
#ifdef CONFIG_CAMERA_4IN1
#define LOG_TAG "cmr_4in1"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#include <cutils/properties.h>

struct class_4in1 {
    struct ipm_common common;
    cmr_uint width;
    cmr_uint height;
    cmr_uint is_inited;
    cmr_handle thread_4in1;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    sem_t sem_4in1;
};

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return -CMR_CAMERA_INVALID_PARAM;                                  \
        }                                                                      \
    } while (0)

#define CAMERA_4IN1_MSG_QUEUE_SIZE 10

#define CMR_EVT_4IN1_BASE (CMR_EVT_IPM_BASE + 0X100)
#define CMR_EVT_4IN1_INIT (CMR_EVT_4IN1_BASE + 0)
#define CMR_EVT_4IN1_START (CMR_EVT_4IN1_BASE + 1)
#define CMR_EVT_4IN1_EXIT (CMR_EVT_4IN1_BASE + 2)

typedef cmr_int (*ipm_get_sensor_info)(cmr_handle oem_handle,
                                       cmr_uint sensor_id,
                                       struct sensor_exp_info *sensor_info);
typedef cmr_int (*ipm_sensor_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                                    struct common_sn_cmd_param *parm);
typedef cmr_int (*ipm_isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                                 struct common_isp_cmd_param *parm);

static cmr_int open_4in1(cmr_handle ipm_handle, struct ipm_open_in *in,
                         struct ipm_open_out *out, cmr_handle *class_handle);
static cmr_int close_4in1(cmr_handle class_handle);
static cmr_int transfer_frame_4in1(cmr_handle class_handle,
                                   struct ipm_frame_in *in,
                                   struct ipm_frame_out *out);
static cmr_int thread_proc_4in1(struct cmr_msg *message, void *private_data);
static cmr_int thread_create_4in1(struct class_4in1 *class_handle);
static cmr_int thread_destroy_4in1(struct class_4in1 *class_handle);

static struct class_ops ops_tab_info_4in1 = {
    open_4in1, close_4in1, transfer_frame_4in1, NULL, NULL,
};

struct class_tab_t tab_info_4in1 = {
    &ops_tab_info_4in1,
};

static cmr_int open_4in1(cmr_handle ipm_handle, struct ipm_open_in *in,
                         struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_4in1 *handle = NULL;

    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct class_4in1 *)malloc(sizeof(struct class_4in1));
    if (!handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(handle, sizeof(struct class_4in1));

    sem_init(&handle->sem_4in1, 0, 1);

    handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    handle->common.class_type = IPM_TYPE_4IN1;
    handle->common.ops = &ops_tab_info_4in1;
    handle->reg_cb = in->reg_cb;

    ret = thread_create_4in1(handle);
    if (ret) {
        CMR_LOGE("4in1 error: create thread.");
        goto free_all;
    }
    *class_handle = (cmr_handle)handle;

    return ret;

free_all:
    if (NULL != handle) {
        free(handle);
        handle = NULL;
    }

    return CMR_CAMERA_FAIL;
}

static cmr_int close_4in1(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_4in1 *handle = (struct class_4in1 *)class_handle;
    CHECK_HANDLE_VALID(handle);

    CMR_LOGD("E");
    sem_wait(&handle->sem_4in1);
    if (handle->is_inited) {
        handle->is_inited = 0;
    }

    ret = thread_destroy_4in1(handle);
    if (ret) {
        CMR_LOGE("4in1 failed to destroy 4IN1 thread.");
    }
    sem_destroy(&handle->sem_4in1);

    if (NULL != handle) {
        free(handle);
        handle = NULL;
    }
    CMR_LOGD("X");
    return ret;
}

static cmr_int transfer_frame_4in1(cmr_handle class_handle,
                                   struct ipm_frame_in *in,
                                   struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_4in1 *handle = (struct class_4in1 *)class_handle;

    CMR_MSG_INIT(message);

    if (!out || !in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!handle->is_inited) {
        CMR_LOGD("is_inited is 0,exit");
        return 0;
    }

    CMR_LOGD("ipm_frame_in.private_data 0x%lx", (cmr_int)in->private_data);
    message.msg_type = CMR_EVT_4IN1_START;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)malloc(sizeof(struct ipm_frame_in));
    if (!message.data) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_FAIL;
    } else {
        message.alloc_flag = 1;
    }
    cmr_copy(message.data, in, sizeof(struct ipm_frame_in));
    ret = cmr_thread_msg_send(handle->thread_4in1, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        free(message.data);
        message.data = NULL;
        return CMR_CAMERA_FAIL;
    }
    return ret;
}

static cmr_int frame_transform_4in1(cmr_handle class_handle,
                                    struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_4in1 *handle = (struct class_4in1 *)class_handle;
    struct img_frm sensor_out_frame;
    cmr_handle oem_handle = NULL;
    struct buffer_cfg buf_cfg;
    struct common_sn_cmd_param sn_param;
    cmr_u32 is_raw_capture = 0;

    if (!in || !handle || !handle->common.ipm_cxt) {
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (handle->is_inited == 0) {
        goto exit;
    }
    if (in->src_frame.fmt == CAM_IMG_FMT_BAYER_MIPI_RAW) {
        is_raw_capture = 1;
    }
    sem_wait(&handle->sem_4in1);
    CMR_LOGD("dst_frame 0x%x,addr=0x%x,is_raw_capture=%d", in->dst_frame.fd,
             in->src_frame.addr_vir.addr_y, is_raw_capture);

    oem_handle = handle->common.ipm_cxt->init_in.oem_handle;
    if (handle->common.ipm_cxt->init_in.ipm_sensor_ioctl) {
        cmr_bzero(&sn_param, sizeof(struct common_sn_cmd_param));
        sn_param.postproc_info.src = in->src_frame;
        sn_param.postproc_info.dst = in->src_frame;

        ret = handle->common.ipm_cxt->init_in.ipm_sensor_ioctl(
            oem_handle, COM_SN_GET_4IN1_FORMAT_CONVERT, &sn_param);
        if (ret) {
            CMR_LOGE("failed to sensor ioctl");
        }
    } else {
        CMR_LOGE("sensor ioctl is NULL");
    }
    sem_post(&handle->sem_4in1);
    if (handle->is_inited == 0) {
        CMR_LOGD("stop snap.exit");
        goto exit;
    }
    sem_wait(&handle->sem_4in1);
    if (handle->common.ipm_cxt->init_in.ops.channel_reproc && !is_raw_capture) {
        cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
        buf_cfg.count = 1;
        buf_cfg.length =
            in->src_frame.size.width * in->src_frame.size.height * 5 / 4;
        buf_cfg.slice_height = in->src_frame.size.height;
        buf_cfg.addr[0].addr_y = in->src_frame.addr_phy.addr_y;
        buf_cfg.addr_vir[0].addr_y = in->src_frame.addr_vir.addr_y;
        buf_cfg.fd[0] = in->src_frame.fd;
        buf_cfg.is_4in1 = 1;
        buf_cfg.monoboottime = in->src_frame.monoboottime;
        ret = handle->common.ipm_cxt->init_in.ops.channel_reproc(oem_handle,
                                                                 &buf_cfg);
    }
    if (is_raw_capture) {
        struct ipm_frame_out out;
        out.dst_frame = in->src_frame;
        out.private_data = handle->frame_in.private_data;
        out.is_plus = 0;
        if (handle->reg_cb) {
            (handle->reg_cb)(IPM_TYPE_4IN1, &out);
        }
    }
    sem_post(&handle->sem_4in1);

exit:
    return ret;
}

static cmr_int thread_proc_4in1(struct cmr_msg *message, void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_4in1 *class_handle = (struct class_4in1 *)private_data;
    cmr_u32 evt = 0;
    struct ipm_frame_out out;
    struct ipm_frame_in *in;

    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_4IN1_START:
        in = message->data;
        frame_transform_4in1(class_handle, in);
        break;

    case CMR_EVT_4IN1_EXIT:
        CMR_LOGD("4IN1 thread exit.");
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int thread_create_4in1(struct class_4in1 *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(&class_handle->thread_4in1,
                                CAMERA_4IN1_MSG_QUEUE_SIZE, thread_proc_4in1,
                                (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        ret = cmr_thread_set_name(class_handle->thread_4in1, "4in1");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set thr name");
            ret = CMR_MSG_SUCCESS;
        }

        class_handle->is_inited = 1;
    }

    return ret;
}

static cmr_int thread_destroy_4in1(struct class_4in1 *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    CMR_LOGV("E");

    ret = cmr_thread_destroy(class_handle->thread_4in1);
    class_handle->thread_4in1 = 0;

    CMR_LOGV("X");
    return ret;
}
#endif
