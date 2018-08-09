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

#ifdef CONFIG_CAMERA_HDR_CAPTURE

#define LOG_TAG "cmr_hdr"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#ifdef CONFIG_SPRD_HDR_LIB
#include "HDR_SPRD.h"
#endif
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
#include "sprd_hdr_api.h"
#endif
#include <cutils/properties.h>

#ifndef CONFIG_SPRD_HDR_LIB_VERSION_2
#define HDR_NEED_FRAME_NUM (HDR_CAP_NUM)
#else
#define HDR_NEED_FRAME_NUM ((HDR_CAP_NUM)-1)
#endif

struct class_hdr_lib_context {
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
    hdr_inst_t lib_handle;
#endif
    float ev[HDR_CAP_NUM];
    struct ipm_version version;
};

struct class_hdr {
    struct ipm_common common;
    cmr_u8 *alloc_addr[HDR_CAP_NUM];
    cmr_uint is_plus;
    cmr_uint mem_size;
    cmr_uint width;
    cmr_uint height;
    cmr_uint is_inited;
    cmr_handle hdr_thread;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    struct class_hdr_lib_context lib_cxt;
    cmr_uint ev_effect_frame_interval;
};

enum oem_ev_level { OEM_EV_LEVEL_1, OEM_EV_LEVEL_2, OEM_EV_LEVEL_3 };

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return -CMR_CAMERA_INVALID_PARAM;                                  \
        }                                                                      \
    } while (0)

#define IMAGE_FORMAT "YVU420_SEMIPLANAR"
#define CAMERA_HDR_MSG_QUEUE_SIZE 5

#define CMR_EVT_HDR_BASE (CMR_EVT_IPM_BASE + 0X100)
#define CMR_EVT_HDR_INIT (CMR_EVT_HDR_BASE + 0)
#define CMR_EVT_HDR_START (CMR_EVT_HDR_BASE + 1)
#define CMR_EVT_HDR_EXIT (CMR_EVT_HDR_BASE + 2)
#define CMR_EVT_HDR_SAVE_FRAME (CMR_EVT_HDR_BASE + 3)
#define CMR_EVT_HDR_PRE_PROC (CMR_EVT_HDR_BASE + 4)
#define CMR_EVT_HDR_SEND_NORMAL (CMR_EVT_HDR_BASE + 5)

typedef cmr_int (*ipm_get_sensor_info)(cmr_handle oem_handle,
                                       cmr_uint sensor_id,
                                       struct sensor_exp_info *sensor_info);
typedef cmr_int (*ipm_sensor_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                                    struct common_sn_cmd_param *parm);
typedef cmr_int (*ipm_isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                                 struct common_isp_cmd_param *parm);

static cmr_int hdr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle);
static cmr_int hdr_close(cmr_handle class_handle);
static cmr_int hdr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out);
static cmr_int hdr_pre_proc(cmr_handle class_handle);
static cmr_int hdr_post_proc(cmr_handle class_handle);
static cmr_int req_hdr_do(cmr_handle class_handle, struct img_addr *dst_addr,
                          struct img_size frame_size);
static cmr_int hdr_arithmetic(cmr_handle class_handle,
                              struct img_addr *dst_addr, cmr_u32 width,
                              cmr_u32 height);
static cmr_int hdr_thread_proc(struct cmr_msg *message, void *private_data);
static cmr_int hdr_thread_create(struct class_hdr *class_handle);
static cmr_int hdr_thread_destroy(struct class_hdr *class_handle);
static cmr_int hdr_save_frame(cmr_handle class_handle, struct ipm_frame_in *in);
static cmr_int req_hdr_save_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in);
static cmr_int hdr_frame_proc(cmr_handle class_handle);
static cmr_int hdr_save_yuv(cmr_handle class_handle, cmr_u32 width,
                            cmr_u32 height);
static cmr_int req_hdr_send_normal_frame(cmr_handle class_handle,
                                         struct ipm_frame_in *in);
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
static cmr_int hdr_sprd_version_init(struct class_hdr *hdr_handle);
static cmr_int hdr_sprd_version_deinit(struct class_hdr *hdr_handle);
static cmr_int hdr_sprd_version_process(struct class_hdr *hdr_handle,
                                        cmr_u32 width, cmr_u32 height);
static cmr_int hdr_sprd_version_detect(struct class_hdr *hdr_handle,
                                       void *param, float *ev);
#endif

static struct class_ops hdr_ops_tab_info = {
    hdr_open, hdr_close, hdr_transfer_frame, hdr_pre_proc, hdr_post_proc,
};

struct class_tab_t hdr_tab_info = {
    &hdr_ops_tab_info,
};

static cmr_int hdr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                        struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = NULL;
    cmr_uint size;
    cmr_int i = 0;

    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    hdr_handle = (struct class_hdr *)malloc(sizeof(struct class_hdr));
    if (!hdr_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(hdr_handle, sizeof(struct class_hdr));
    /*ev effect ctrl by isp, this palce set to zero*/
    hdr_handle->ev_effect_frame_interval = 0; // in->adgain_valid_frame_num + 1;

    out->format = IMG_FMT_YCBCR420;
    out->total_frame_number =
        HDR_CAP_NUM + hdr_handle->ev_effect_frame_interval;

    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);

    CMR_LOGI("in->frame_size.width = %d,in->frame_size.height = %d",
             in->frame_size.width, in->frame_size.height);

    hdr_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    hdr_handle->common.class_type = IPM_TYPE_HDR;
    hdr_handle->common.receive_frame_count = 0;
    hdr_handle->common.save_frame_count = 0;
    hdr_handle->common.ops = &hdr_ops_tab_info;
    hdr_handle->is_plus = in->is_plus;

    hdr_handle->mem_size = size;

    CMR_LOGI("hdr_handle->mem_size = 0x%lx", hdr_handle->mem_size);

    hdr_handle->height = in->frame_size.height;
    hdr_handle->width = in->frame_size.width;
    hdr_handle->reg_cb = in->reg_cb;

    ret = hdr_thread_create(hdr_handle);
    if (ret) {
        CMR_LOGE("HDR error: create thread.");
        goto free_all;
    }

    *class_handle = (cmr_handle)hdr_handle;
#ifdef CONFIG_SPRD_HDR_LIB
    sprd_hdr_pool_init();
    sprd_hdr_set_stop_flag(HDR_NORMAL);
#endif

    out->version.major = 1;
    out->version.minor = 0;
    out->version.micro = 0;
    out->version.nano = 0;

#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
    ret = hdr_sprd_version_init(hdr_handle);
    if (ret) {
        goto free_all;
    }
    out->version.major = hdr_handle->lib_cxt.version.major;
    out->version.minor = hdr_handle->lib_cxt.version.minor;
    out->version.micro = hdr_handle->lib_cxt.version.micro;
    out->version.nano = hdr_handle->lib_cxt.version.nano;
#endif
    return ret;

free_all:
    if (NULL != hdr_handle)
        free(hdr_handle);
    return CMR_CAMERA_NO_MEM;
}

static cmr_int hdr_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;
    cmr_int i;
    CHECK_HANDLE_VALID(hdr_handle);

    CMR_LOGD("E");

#ifdef CONFIG_SPRD_HDR_LIB
    sprd_hdr_set_stop_flag(HDR_STOP);
#endif

#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
    ret = hdr_sprd_version_deinit(hdr_handle);
#endif
    ret = hdr_thread_destroy(hdr_handle);
    if (ret) {
        CMR_LOGE("HDR failed to destroy hdr thread.");
    }
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
    CMR_LOGI("close hdr");
    ret = sprd_hdr_close(hdr_handle->lib_cxt.lib_handle);
#endif

    if (NULL != hdr_handle)
        free(hdr_handle);

#ifdef CONFIG_SPRD_HDR_LIB
    sprd_hdr_pool_destroy();
#endif

    CMR_LOGD("X");
    return ret;
}

static cmr_int req_hdr_save_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;

    CMR_MSG_INIT(message);

    if (!class_handle || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    hdr_handle->common.save_frame_count++;
    if (hdr_handle->common.save_frame_count <=
        hdr_handle->ev_effect_frame_interval) {
        CMR_LOGD("ev_effect_frame_interval is %ld, donot need save this frame, "
                 "just for set ev in pipeline.frame count:%ld",
                 hdr_handle->ev_effect_frame_interval,
                 hdr_handle->common.save_frame_count);
        return CMR_CAMERA_SUCCESS;
    }

    message.data = (struct ipm_frame_in *)malloc(sizeof(struct ipm_frame_in));
    if (!message.data) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        return ret;
    }
    memcpy(message.data, in, sizeof(struct ipm_frame_in));
    message.msg_type = CMR_EVT_HDR_SAVE_FRAME;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(hdr_handle->hdr_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to hdr thread.");
        if (message.data) {
            free(message.data);
        }
    }
    return ret;
}

static cmr_int hdr_transfer_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in,
                                  struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;
    cmr_u32 frame_in_cnt;
    struct img_addr *addr;
    struct img_size size;
    cmr_handle oem_handle;
    ipm_sensor_ioctl sensor_ioctl;
    ipm_isp_ioctl isp_ioctl;
    struct common_sn_cmd_param sn_param;
    struct common_isp_cmd_param isp_param1;
    struct common_isp_cmd_param isp_param2;
    cmr_u32 sensor_id = 0;
    ipm_get_sensor_info get_sensor_info;
    cmr_u32 hdr_enable = 0;

    if (!out || !in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGI("ipm_frame_in.private_data 0x%lx", (cmr_int)in->private_data);
    addr = &in->dst_frame.addr_vir;
    size = in->src_frame.size;

    ret = req_hdr_save_frame(class_handle, in);
    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("req_hdr_save_frame fail");
        return CMR_CAMERA_FAIL;
    }

    // save normal pic start
    if (hdr_handle->is_plus &&
        hdr_handle->common.save_frame_count ==
            (HDR_CAP_NUM + hdr_handle->ev_effect_frame_interval)) {
        // will send normal cap via callback to save
        req_hdr_send_normal_frame(hdr_handle, in);
    }

    if (hdr_handle->common.save_frame_count ==
        (HDR_CAP_NUM + hdr_handle->ev_effect_frame_interval)) {
        CMR_LOGD("HDR enable = %d", hdr_enable);
        cmr_bzero(&out->dst_frame, sizeof(struct img_frm));
        sensor_ioctl = hdr_handle->common.ipm_cxt->init_in.ipm_sensor_ioctl;
        isp_ioctl = hdr_handle->common.ipm_cxt->init_in.ipm_isp_ioctl;
        oem_handle = hdr_handle->common.ipm_cxt->init_in.oem_handle;
        hdr_handle->frame_in = *in;
        ret = req_hdr_do(class_handle, addr, size);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("req_hdr_do fail");
        }
        out->dst_frame = in->dst_frame;
        out->private_data = in->private_data;
        struct sensor_exp_info sensor_info;
        sensor_id = hdr_handle->common.ipm_cxt->init_in.sensor_id;
        get_sensor_info = hdr_handle->common.ipm_cxt->init_in.get_sensor_info;
        get_sensor_info(oem_handle, sensor_id, &sensor_info);

        if (SENSOR_IMAGE_FORMAT_RAW == sensor_info.image_format) {
            isp_param1.cmd_value = hdr_enable;
            ret = isp_ioctl(oem_handle, COM_ISP_SET_HDR, (void *)&isp_param1);
        } else {
            sn_param.cmd_value = OEM_EV_LEVEL_2;
            ret =
                sensor_ioctl(oem_handle, COM_SN_SET_HDR_EV, (void *)&sn_param);
        }
        if (ret) {
            CMR_LOGE("HDR failed to set ev.");
        }
    }

    return ret;
}

static cmr_int req_hdr_send_normal_frame(cmr_handle class_handle,
                                         struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;

    CMR_MSG_INIT(message);

    if (!class_handle || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    message.data = (struct ipm_frame_in *)malloc(sizeof(struct ipm_frame_in));
    if (!message.data) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        return ret;
    }
    memcpy(message.data, in, sizeof(struct ipm_frame_in));
    message.msg_type = CMR_EVT_HDR_SEND_NORMAL;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(hdr_handle->hdr_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to hdr thread.");
        if (message.data) {
            free(message.data);
        }
    }
    return ret;
}

static cmr_int hdr_pre_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;
    CMR_MSG_INIT(message);

    if (!class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    message.msg_type = CMR_EVT_HDR_PRE_PROC;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;

    ret = cmr_thread_msg_send(hdr_handle->hdr_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to hdr thread.");
    }

    return ret;
}

static cmr_int hdr_frame_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 img_fmt = 0;
    cmr_u32 frame_in_cnt = 0;
    cmr_handle oem_handle = NULL;
    enum oem_ev_level ev_level = OEM_EV_LEVEL_1;
    ipm_get_sensor_info get_sensor_info;
    ipm_sensor_ioctl sensor_ioctl;
    ipm_isp_ioctl isp_ioctl;
    struct common_sn_cmd_param sn_param;
    struct common_isp_cmd_param isp_param1;
    struct common_isp_cmd_param isp_param2;
    struct sensor_exp_info sensor_info;
    cmr_u32 hdr_enable = 1;

    if (!hdr_handle) {
        CMR_LOGE("hdr_handle is NULL");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    frame_in_cnt = ++hdr_handle->common.receive_frame_count;
    sensor_id = hdr_handle->common.ipm_cxt->init_in.sensor_id;
    get_sensor_info = hdr_handle->common.ipm_cxt->init_in.get_sensor_info;
    sensor_ioctl = hdr_handle->common.ipm_cxt->init_in.ipm_sensor_ioctl;
    isp_ioctl = hdr_handle->common.ipm_cxt->init_in.ipm_isp_ioctl;
    oem_handle = hdr_handle->common.ipm_cxt->init_in.oem_handle;

    CMR_LOGV("frame cnt %d", frame_in_cnt);
    switch (frame_in_cnt) {
    case 1:
        ev_level = OEM_EV_LEVEL_1;
        break;
    case 2:
        ev_level = OEM_EV_LEVEL_2;
        break;
    case 3:
        ev_level = OEM_EV_LEVEL_3;
        break;
    default:
        ev_level = OEM_EV_LEVEL_2;
        break;
    }
    get_sensor_info(oem_handle, sensor_id, &sensor_info);
    CMR_LOGI("HDR enable = %d", hdr_enable);

    if (SENSOR_IMAGE_FORMAT_RAW == sensor_info.image_format) {
        isp_param1.cmd_value = (cmr_u32)hdr_enable;
        ret = isp_ioctl(oem_handle, COM_ISP_SET_HDR, (void *)&isp_param1);
    } else {
        sn_param.cmd_value = (cmr_u32)ev_level;
        ret = sensor_ioctl(oem_handle, COM_SN_SET_HDR_EV, (void *)&sn_param);
    }

    if (ret) {
        CMR_LOGE("HDR failed to set ev.");
    }

    return ret;
}

static cmr_int hdr_post_proc(cmr_handle class_handle) {
    UNUSED(class_handle);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/

    return ret;
}

static cmr_int req_hdr_do(cmr_handle class_handle, struct img_addr *dst_addr,
                          struct img_size frame_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;

    CMR_MSG_INIT(message);

    if (!dst_addr || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    hdr_handle->dst_addr = *dst_addr;
    hdr_handle->width = frame_size.width;
    hdr_handle->height = frame_size.height;

    message.msg_type = CMR_EVT_HDR_START;
    if (NULL != hdr_handle->reg_cb)
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    else
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(hdr_handle->hdr_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to hdr thread.");
    }

    return ret;
}

static cmr_int hdr_arithmetic(cmr_handle class_handle,
                              struct img_addr *dst_addr, cmr_u32 width,
                              cmr_u32 height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 size = width * height;
    cmr_u8 *temp_addr0 = NULL;
    cmr_u8 *temp_addr1 = NULL;
    cmr_u8 *temp_addr2 = NULL;
    char *p_format = IMAGE_FORMAT;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;

    if (!class_handle || !dst_addr) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.dump.hdr.frame", value, "0");
    if (!strcmp(value, "1")) {
        ret = hdr_save_yuv(class_handle, width, height);
        if (ret != 0) {
            CMR_LOGE("hdr save yuv failed!");
        }
    }

    temp_addr0 = hdr_handle->alloc_addr[0];
    temp_addr1 = hdr_handle->alloc_addr[1];
    temp_addr2 = hdr_handle->alloc_addr[2];

    CMR_LOGD("width %d,height %d.", width, height);
    /*save_input_data(width,height);*/
    if ((NULL != temp_addr0) && (NULL != temp_addr1) && (NULL != temp_addr2)) {
        LAUNCHLOGE(CMR_CAPTURE_RECEIVE_FRAME_T);
        LAUNCHLOGS(CMR_HDR_DO_T);
#ifndef CONFIG_SPRD_HDR_LIB_VERSION_2
        ret = HDR_Function(temp_addr0, temp_addr1, temp_addr2, temp_addr0,
                           height, width, p_format);
#else
        ret = hdr_sprd_version_process(hdr_handle, width, height);
#endif
        if (ret != 0) {
            if (ret == 1) {
                CMR_LOGI("hdr not executed completely");
                ret = CMR_CAMERA_SUCCESS;
            } else {
                CMR_LOGE("hdr error!");
                ret = CMR_CAMERA_FAIL;
            }
        }
    } else {
        CMR_LOGE("can't handle hdr.");
        ret = CMR_CAMERA_FAIL;
    }

    if (NULL != temp_addr0) {
        memcpy((void *)dst_addr->addr_y, (void *)temp_addr0, size);
        memcpy((void *)dst_addr->addr_u, (void *)(temp_addr0 + size), size / 2);
    }

    if (CMR_CAMERA_SUCCESS == ret) {
        CMR_LOGD("hdr done.");
    }

    LAUNCHLOGE(CMR_HDR_DO_T);
    return ret;
}

static cmr_int hdr_save_frame(cmr_handle class_handle,
                              struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint y_size = 0;
    cmr_uint uv_size = 0;
    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;
    cmr_int frame_sn = 0;
    if (!class_handle || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (hdr_handle->common.save_frame_count >
        (HDR_CAP_NUM + hdr_handle->ev_effect_frame_interval)) {
        CMR_LOGE("cap cnt error,%ld.", hdr_handle->common.save_frame_count);
        return CMR_CAMERA_FAIL;
    }

    hdr_handle->lib_cxt.ev[0] = in->ev[0];
    hdr_handle->lib_cxt.ev[1] = in->ev[1];
    CMR_LOGI("ev: %f, %f", hdr_handle->lib_cxt.ev[0],
             hdr_handle->lib_cxt.ev[1]);
    y_size = in->src_frame.size.height * in->src_frame.size.width;
    uv_size = in->src_frame.size.height * in->src_frame.size.width / 2;
    frame_sn = hdr_handle->common.save_frame_count - 1 -
               hdr_handle->ev_effect_frame_interval;
    if (frame_sn < 0) {
        CMR_LOGE("frame_sn error,%ld.", frame_sn);
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGI(" HDR frame_sn %ld, y_addr 0x%lx", frame_sn,
             in->src_frame.addr_vir.addr_y);
    if (hdr_handle->mem_size >= in->src_frame.buf_size &&
        NULL != (void *)in->src_frame.addr_vir.addr_y)
        hdr_handle->alloc_addr[frame_sn] =
            (cmr_u8 *)(in->src_frame.addr_vir.addr_y);
    else
        CMR_LOGE(" HDR:mem size:0x%lx,data y_size:0x%lx. 0x%lx",
                 hdr_handle->mem_size, y_size, in->src_frame.addr_vir.addr_y);

    return ret;
}

static cmr_int hdr_thread_proc(struct cmr_msg *message, void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_hdr *class_handle = (struct class_hdr *)private_data;
    cmr_u32 evt = 0;
    struct ipm_frame_out out;
    struct ipm_frame_in *in;

    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_HDR_INIT:
        CMR_LOGI("HDR thread inited.");
        break;
    case CMR_EVT_HDR_PRE_PROC:
        CMR_LOGI("HDR pre_proc");
        hdr_frame_proc(class_handle);
        break;
    case CMR_EVT_HDR_SAVE_FRAME:
        CMR_LOGI("HDR save frame");
        in = message->data;
        ret = hdr_save_frame(class_handle, in);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("HDR save frame failed.");
        }
        break;
    case CMR_EVT_HDR_START:
        class_handle->common.receive_frame_count = 0;
        class_handle->common.save_frame_count = 0;
        out.dst_frame = class_handle->frame_in.dst_frame;
        out.private_data = class_handle->frame_in.private_data;
        out.is_plus = 0;
        CMR_LOGI("out private_data 0x%lx", (cmr_int)out.private_data);
        CMR_LOGI("CMR_EVT_HDR_START addr 0x%lx %ld %ld",
                 class_handle->dst_addr.addr_y, class_handle->width,
                 class_handle->height);
        CMR_LOGI("HDR thread proc start ");
        //  modify "-1 , +1 , 0" to "-1 , 0 , +1" , need modify index "0-1-2" to
        //  "0-2-1"
        cmr_u8 *p = class_handle->alloc_addr[1];
        class_handle->alloc_addr[1] = class_handle->alloc_addr[2];
        class_handle->alloc_addr[2] = p;

        hdr_arithmetic(class_handle, &class_handle->dst_addr,
                       class_handle->width, class_handle->height);
        CMR_LOGI("HDR thread proc done ");

        if (class_handle->reg_cb) {
            (class_handle->reg_cb)(IPM_TYPE_HDR, &out);
        }

        break;
    case CMR_EVT_HDR_SEND_NORMAL:
        CMR_LOGI("HDR thread send normal pic");
        in = (struct ipm_frame_in *)message->data;
        out.dst_frame = in->dst_frame;
        out.private_data = in->private_data;
        out.is_plus = 1;
        if (out.private_data == NULL) {
            CMR_LOGI("HDR thread send normal pic private_data = NULL");
        }
        if (class_handle->reg_cb) {
            (class_handle->reg_cb)(IPM_TYPE_HDR, &out);
        }
        break;
    case CMR_EVT_HDR_EXIT:
        CMR_LOGD("HDR thread exit.");
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int hdr_thread_create(struct class_hdr *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(&class_handle->hdr_thread,
                                CAMERA_HDR_MSG_QUEUE_SIZE, hdr_thread_proc,
                                (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        ret = cmr_thread_set_name(class_handle->hdr_thread, "hdr");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set thr name");
            ret = CMR_MSG_SUCCESS;
        }

        class_handle->is_inited = 1;
    }

    return ret;
}

static cmr_int hdr_thread_destroy(struct class_hdr *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    CMR_LOGD("E");

    if (class_handle->is_inited) {
        ret = cmr_thread_destroy(class_handle->hdr_thread);
        class_handle->hdr_thread = 0;

        class_handle->is_inited = 0;
    }

    CMR_LOGD("X");
    return ret;
}

static cmr_int hdr_save_yuv(cmr_handle class_handle, cmr_u32 width,
                            cmr_u32 height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    struct class_hdr *hdr_handle = (struct class_hdr *)class_handle;

    if (!class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    FILE *fp = NULL;
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;

    char file_name[100];
    char file_name1[100];
    char file_name2[100];
    char tmp_str[10];
    char datetime[15] = {0};
    time_t timep;
    struct tm *p;

    cmr_bzero(file_name, 40);
    cmr_bzero(file_name1, 40);
    cmr_bzero(file_name2, 40);

    time(&timep);
    p = localtime(&timep);
    sprintf(datetime, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
            (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

    strcpy(file_name, CAMERA_DUMP_PATH);
    strcpy(file_name1, CAMERA_DUMP_PATH);
    strcpy(file_name2, CAMERA_DUMP_PATH);

    strcat(file_name, "hdr_");
    strcat(file_name1, "hdr_");
    strcat(file_name2, "hdr_");
    strcat(file_name, datetime);
    strcat(file_name1, datetime);
    strcat(file_name2, datetime);

    strcat(file_name, "_");
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);
    strcat(file_name, "_1");
    strcat(file_name, ".NV21");
    fp = fopen(file_name, "wb");
    fwrite((void *)hdr_handle->alloc_addr[0], 1, width * height * 3 / 2, fp);
    fclose(fp);
    fp = NULL;

    strcat(file_name1, "_");
    sprintf(tmp_str, "%d", width);
    strcat(file_name1, tmp_str);
    strcat(file_name1, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name1, tmp_str);
    strcat(file_name1, "_2");
    strcat(file_name1, ".NV21");
    fp1 = fopen(file_name1, "wb");
    fwrite((void *)hdr_handle->alloc_addr[1], 1, width * height * 3 / 2, fp1);
    fclose(fp1);
    fp1 = NULL;

    strcat(file_name2, "_");
    sprintf(tmp_str, "%d", width);
    strcat(file_name2, tmp_str);
    strcat(file_name2, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name2, tmp_str);
    strcat(file_name2, "_3");
    strcat(file_name2, ".NV21");
    fp2 = fopen(file_name2, "wb");
    fwrite((void *)hdr_handle->alloc_addr[2], 1, width * height * 3 / 2, fp2);
    fclose(fp2);
    fp2 = NULL;

    return ret;
}
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
static cmr_int hdr_sprd_version_init(struct class_hdr *hdr_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    hdr_version_t lib_version;
    hdr_config_t cfg;

    if (!hdr_handle) {
        CMR_LOGE("error:hdr handle is NULL\n");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    memset(&lib_version, 0, sizeof(hdr_version_t));
    memset(&cfg, 0, sizeof(hdr_config_t));

    if (!sprd_hdr_version(&lib_version)) {
        hdr_handle->lib_cxt.version.major = lib_version.major;
        hdr_handle->lib_cxt.version.minor = lib_version.minor;
        hdr_handle->lib_cxt.version.micro = lib_version.micro;
        hdr_handle->lib_cxt.version.nano = lib_version.nano;
        CMR_LOGD("major:%d minor:%d micro:%d nano:%d \n", lib_version.major,
                 lib_version.minor, lib_version.micro, lib_version.nano);
        CMR_LOGD("buiid date:%s build time:%s build rev:%s \n",
                 lib_version.built_date, lib_version.built_time,
                 lib_version.built_rev);
    } else {
        CMR_LOGE("failed to get verion!");
    }

    ret = sprd_hdr_config_default(&cfg);
    if (!ret) {
        cfg.img_width = hdr_handle->width;
        cfg.img_height = hdr_handle->height;
        cfg.img_stride = hdr_handle->width;
        cfg.img_num = HDR_NEED_FRAME_NUM;
        cfg.max_width = hdr_handle->width;
        cfg.max_height = hdr_handle->height;
        cfg.core_str = "_4_0_1_2_3";
        ret = sprd_hdr_open(&hdr_handle->lib_cxt.lib_handle, &cfg);
        if (ret) {
            CMR_LOGE("failed to open lib!\n");
            goto exit;
        }
    } else {
        CMR_LOGE("failed to get cfg!");
        goto exit;
    }
exit:
    CMR_LOGD("done %ld", ret);
    return ret;
}

static cmr_int hdr_sprd_version_deinit(struct class_hdr *hdr_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!hdr_handle) {
        CMR_LOGE("error:hdr handle is NULL\n");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto deinit_exit;
    }
    CMR_LOGI("stop hdr");
    ret = sprd_hdr_fast_stop(hdr_handle->lib_cxt.lib_handle);

deinit_exit:
    CMR_LOGD("done %ld", ret);
    return ret;
}

static cmr_int hdr_sprd_version_process(struct class_hdr *hdr_handle,
                                        cmr_u32 width, cmr_u32 height) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    ldr_image_t input_img[HDR_NEED_FRAME_NUM];
    cmr_u8 *out_img;

    if (!hdr_handle) {
        CMR_LOGE("error:hdr handle is NULL\n");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto process_exit;
    }
    input_img[0].data = hdr_handle->alloc_addr[0];
    input_img[0].ev = hdr_handle->lib_cxt.ev[0];
    input_img[0].width = width;
    input_img[0].height = height;
    input_img[0].stride = width;
    input_img[1].data = hdr_handle->alloc_addr[2];
    input_img[1].ev = hdr_handle->lib_cxt.ev[1];
    input_img[1].width = width;
    input_img[1].height = height;
    input_img[1].stride = width;
    out_img = hdr_handle->alloc_addr[0];
    CMR_LOGI("addr: 0x%lx, 0x%lx, ev: %f, %f", input_img[0].data,
             input_img[1].data, input_img[0].ev, input_img[1].ev);
    ret = sprd_hdr_process(hdr_handle->lib_cxt.lib_handle, &input_img[0],
                           out_img);
    if (ret) {
        CMR_LOGE("faild to hdr process\n");
    }
process_exit:
    CMR_LOGD("done %ld", ret);
    return ret;
}

static cmr_int hdr_sprd_version_detect(struct class_hdr *hdr_handle,
                                       void *param, float *ev) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!hdr_handle || !param || !ev) {
        CMR_LOGE("input param is NULL\n");
        ret = -CMR_CAMERA_INVALID_PARAM;
        return ret;
    }
    ret = sprd_hdr_detect(hdr_handle->lib_cxt.lib_handle, (hdr_stat_t *)param,
                          ev);
    if (ret) {
        CMR_LOGE("failed to detect\n");
    }
    CMR_LOGI("done %ld", ret);
    return ret;
}
#endif
#endif
