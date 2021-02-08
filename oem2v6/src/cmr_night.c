
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

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#define LOG_TAG "cmr_night"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#include "B01YUVNightDNS.h"
#include <cutils/properties.h>
#include "isp_mw.h"
#include <string.h>

struct nightdns_info {
    cmr_handle class_handle;
    struct ipm_frame_in in;
    struct ipm_frame_out out;
    cmr_u32 cur_frame_num;
};

struct class_nightdns {
    struct ipm_common common;
    struct nightdns_info g_info_3dnr[NIGHTDNS_CAP_NUM];
    cmr_uint mem_size;
    cmr_uint width;
    cmr_uint height;
    cmr_uint is_inited;
    cmr_handle nightdns_thread;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    cmr_u32 g_num;
    cmr_u32 g_totalnum;
    cmr_uint is_stop;
    cmr_uint ev_effect_frame_interval;
    time_t timep;
};

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return -CMR_CAMERA_INVALID_PARAM;                                  \
        }                                                                      \
    } while (0)

#define CAMERA_3DNR_MSG_QUEUE_SIZE 5

#define CMR_EVT_3DNR_BASE (CMR_EVT_IPM_BASE + 0X100)
#define CMR_EVT_3DNR_INIT (CMR_EVT_3DNR_BASE + 0)
#define CMR_EVT_3DNR_START (CMR_EVT_3DNR_BASE + 1)
#define CMR_EVT_3DNR_DEINIT (CMR_EVT_3DNR_BASE + 2)
#define CMR_EVT_3DNR_PROCESS (CMR_EVT_3DNR_BASE + 3)
#define CMR_EVT_3DR_PRE_PROC (CMR_EVT_3DNR_BASE + 4)
#define CMR_EVT_3DNR_SAVE_FRAME (CMR_EVT_3DNR_BASE + 5)

typedef cmr_int (*ipm_get_sensor_info)(cmr_handle oem_handle,
                                       cmr_uint sensor_id,
                                       struct sensor_exp_info *sensor_info);
typedef cmr_int (*ipm_sensor_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                                    struct common_sn_cmd_param *parm);
typedef cmr_int (*ipm_isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,
                                 struct common_isp_cmd_param *parm);

static cmr_int nightdns_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *class_handle);
static cmr_int nightdns_close(cmr_handle class_handle);
static cmr_int nightdns_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out);
static cmr_int nightdns_pre_proc(cmr_handle class_handle);
static cmr_int nightdns_post_proc(cmr_handle class_handle);
static cmr_int nightdns_process_thread_proc(struct cmr_msg *message,void *private_data);
static cmr_int nightdns_thread_create(struct class_nightdns *class_handle);
static cmr_int nightdns_thread_destroy(struct class_nightdns *class_handle);
static cmr_int nightdns_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in);
static cmr_int nightdns_frame_proc(cmr_handle class_handle);
static struct class_ops nightb01_ops_tab_info = {nightdns_open, nightdns_close,
                                    nightdns_transfer_frame, nightdns_pre_proc,
                                    nightdns_post_proc};

struct class_tab_t nightb01_tab_info = {
    &nightb01_ops_tab_info,
};

// idx: [0,n] input, [-n, -1]output
static cmr_int save_yuv_file(cmr_handle class_handle, int idx) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char flag[PROPERTY_VALUE_MAX];
    struct class_nightdns *nightdns_handle = (struct class_nightdns *)class_handle;
    struct tm *p;
    char file_name[256];
    char tmp[64];
    FILE *fp = NULL;
    int w, h;
    struct camera_context *cxt = NULL;

    if (!class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (idx == 0)   // first get time
        time(&(nightdns_handle->timep));

    property_get("persist.vendor.cam.night.b01.dump", flag, "0");
   if (!atoi(flag)) { // save output image.
        return 1;
    }
    p = localtime(&(nightdns_handle->timep));
    snprintf(tmp, sizeof(tmp), "%04d%02d%02d%02d%02d%02d_",
            (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
            p->tm_hour, p->tm_min, p->tm_sec);

    strcpy(file_name, CAMERA_DUMP_PATH);
    strcat(file_name, "night_");
    strcat(file_name, tmp);
    w =nightdns_handle->width;
    h = nightdns_handle->height;
    snprintf(tmp, sizeof(tmp), "%dx%d_", w, h);
    strcat(file_name, tmp);
    cxt = (struct camera_context *)nightdns_handle->common.ipm_cxt->init_in.oem_handle;
    if (idx >= 0) {
        snprintf(tmp, sizeof(tmp), "input_%d_ev%f_iso%d_exp%d_gain%d",
            idx,
            cxt->ae_aux_info.param[idx].ev,
            cxt->ae_aux_info.param[idx].iso,
            cxt->ae_aux_info.param[idx].exposure,
            cxt->ae_aux_info.param[idx].total_gain);
        strcat(file_name, tmp);
        strcat(file_name, "_face1_0_0_0_0");
    } else {
        strcat(file_name, "output");
    }
    strcat(file_name, ".yuv");
    fp = fopen(file_name, "wb");
    if (idx >= 0)
        fwrite((void *)nightdns_handle->g_info_3dnr[idx].in.src_frame.addr_vir.addr_y, 1,
               w * h * 3 / 2, fp);
    else
        fwrite((void *)nightdns_handle->dst_addr.addr_y, 1,
               w * h * 3 / 2, fp);
//        fwrite((void *)threednr_handle->frame_in.dst_frame.addr_vir.addr_y, 1,
//               w * h * 3 / 2, fp);

    fclose(fp);
    fp = NULL;

    return ret;
}

static cmr_int nightdns_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_nightdns *nightdns_handle = NULL;
    cmr_uint size = 0;
    int nRefFrameNum = 0; //input num-1
    int nExpNum = 0; //need adjust ev num
    BSTYUVNIGHTDNSConfig config;
    struct ipm_context_t *ipm_cxt = (struct ipm_context_t *)ipm_handle;

    CMR_LOGD("E");
    if (!out || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    nightdns_handle = (struct class_nightdns *)malloc(sizeof(struct class_nightdns));
    if (!nightdns_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    out->total_frame_number = NIGHTDNS_CAP_NUM;

    cmr_bzero(nightdns_handle, sizeof(struct class_nightdns));
    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);
    nightdns_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    nightdns_handle->common.class_type = IPM_TYPE_NIGHTDNS;
    nightdns_handle->common.ops = &nightb01_ops_tab_info;
    nightdns_handle->common.receive_frame_count = 0;
    nightdns_handle->common.save_frame_count = 0;
    nightdns_handle->mem_size = size;

    nightdns_handle->height = in->frame_size.height;
    nightdns_handle->width = in->frame_size.width;
    nightdns_handle->reg_cb = in->reg_cb;
    nightdns_handle->g_num = 0;
    nightdns_handle->is_stop = 0;
    nightdns_handle->g_totalnum = 0;
    ret = nightdns_thread_create(nightdns_handle);
    if (ret) {
        ("3dnr error: create thread");
        goto free_all;
    }

    //init
    nRefFrameNum = 6;
    nExpNum = 4;
    CMR_LOGD("nRefFrameNum nExpNum %d %d", nRefFrameNum, nExpNum);
    config.cfg.pConfigFile = "/vendor/cfg/control_param_YUVProcFlow_yuvnight_4dns9_colorkill.cfg";
    config.nConfigType = BST_YUVNIGHTDNS_CONFIG_TYPE_FILE;
    ret = bstYUVNIGHTDNSInit(config, nRefFrameNum, nExpNum);
    if(ret != 0){
        CMR_LOGE("failed to init nightdns. ret = %d",ret);
    }

    *class_handle = nightdns_handle;
    CMR_LOGD("X");
    return ret;

free_all:
    if (NULL != nightdns_handle) {
        free(nightdns_handle);
        nightdns_handle = NULL;
    }
    return CMR_CAMERA_NO_MEM;
}

static cmr_int nightdns_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_nightdns *nightdns_handle = (struct class_nightdns *)class_handle;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;

    CMR_LOGD("E");
    CHECK_HANDLE_VALID(nightdns_handle);

    nightdns_handle->is_stop = 1;
    // nightdns_cancel();
    CMR_LOGD("OK to nightdns_cancel");

    ret = nightdns_thread_destroy(nightdns_handle);
    if (ret) {
        CMR_LOGE("nightdns failed to destroy 3dnr thread");
    }

    ret = bstYUVNIGHTDNSUninit();
    if (ret != 0) {
        CMR_LOGE("nightdns failed to nightdns_deinit, ret = %d", ret);
    }

    oem_handle = nightdns_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if(cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW)
    {
        if (cam_cxt->hal_free == NULL) {
            CMR_LOGE("cam_cxt->hal_free is NULL");
            goto exit;
         }
    }
exit:

    if (NULL != nightdns_handle) {
        free(nightdns_handle);
        nightdns_handle = NULL;
    }
    CMR_LOGD("X");
    return ret;
}

static cmr_int req_3dnr_save_frame(cmr_handle class_handle,
                                  struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_nightdns *nightdns_handle = (struct class_nightdns *)class_handle;

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
    message.msg_type = CMR_EVT_3DNR_SAVE_FRAME;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(nightdns_handle->nightdns_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to 3dnr thread.");
        if (message.data) {
            free(message.data);
            message.data = NULL;
        }
    }
    return ret;
}

/* add frame one by one */
static cmr_int nightdns_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    BSTImage InImage;
    memset(&InImage, 0, sizeof(BSTImage));
    cmr_int frame_sn = 0;
    if (!class_handle || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    struct class_nightdns *nightdns_handle = (struct class_nightdns *)class_handle;
    cxt = (struct camera_context *)in->private_data;
    nightdns_handle->common.save_frame_count++;
    if (nightdns_handle->common.save_frame_count > NIGHTDNS_CAP_NUM) {
        CMR_LOGE("cap cnt error,%ld", nightdns_handle->common.save_frame_count);
        return CMR_CAMERA_FAIL;
    }

    frame_sn = nightdns_handle->common.save_frame_count - 1;
    if (frame_sn < 0) {
        CMR_LOGE("frame_sn error,%ld.", frame_sn);
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGD("3dnr frame_sn %ld, y_addr 0x%lx", frame_sn,
             in->src_frame.addr_vir.addr_y);
    if (nightdns_handle->mem_size >= in->src_frame.buf_size &&
        NULL != (void *)in->src_frame.addr_vir.addr_y) {
        save_yuv_file(nightdns_handle, frame_sn);
        InImage.imgData.nFormat = BST_IMAGE_TYPE_NV21;
        InImage.imgData.nWidth = in->src_frame.size.width;
        InImage.imgData.nHeight = in->src_frame.size.height;
        InImage.imgData.pPlanePtr[0] = (cmr_u8 *)(in->src_frame.addr_vir.addr_y);
        InImage.imgData.pPlanePtr[1] = (cmr_u8 *)(in->src_frame.addr_vir.addr_u);
        InImage.imgData.nRowPitch[0] = in->src_frame.size.width;
        InImage.imgData.nRowPitch[1] = in->src_frame.size.width;

        InImage.metaData.nExpo = cxt->ae_aux_info.param[frame_sn].exp_time / 1000000;//ms
        InImage.metaData.nISO = cxt->ae_aux_info.param[frame_sn].iso;
        CMR_LOGD("%p frame_sn %d w h[%d %d] pPlanePtr[%d %d] nExpo %d nISO %d ev %f ",
            cxt, frame_sn,
            InImage.imgData.nWidth,
            InImage.imgData.nHeight,
            InImage.imgData.pPlanePtr[0],
            InImage.imgData.pPlanePtr[1],
            InImage.metaData.nExpo, InImage.metaData.nISO,
            cxt->ae_aux_info.param[frame_sn].ev);
        ret =  bstYUVNIGHTDNSAddFrame(frame_sn, &InImage);
        if (ret != 0) {
            CMR_LOGE("nightdns addframe fails!!!\n");
            //goto EXIT;
        }
    } else {
        CMR_LOGE(" 3dnr:mem size:0x%lx,data w_h:%d %d. 0x%lx",
                 nightdns_handle->mem_size, in->src_frame.size.width, in->src_frame.size.height,
                 in->src_frame.addr_vir.addr_y);
    }
    return ret;
}

static cmr_int nightdns_process_frame(cmr_handle class_handle,
                          struct img_addr *dst_addr, struct img_size frame_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_LOGD("E");
    struct camera_context *cxt = NULL;
    struct class_nightdns *nightdns_handle =
        (struct class_nightdns *)class_handle;
    struct ipm_frame_out *out = &nightdns_handle->g_info_3dnr[0].out;
    BSTImage nightDNSImg;
    int nWidth = 0;
    char filename[128];
    cmr_handle oem_handle;
    struct ipm_frame_in *in = &nightdns_handle->frame_in;

    cxt = (struct camera_context *)in->private_data;
    nWidth = in->src_frame.size.width;
    nightDNSImg.imgData.pPlanePtr[0] = (unsigned char *)dst_addr->addr_y;//in->src_frame.addr_vir.addr_y;
    nightDNSImg.imgData.pPlanePtr[1] = (unsigned char *)dst_addr->addr_u;//in->src_frame.addr_vir.addr_u;
    nightDNSImg.imgData.nWidth = in->src_frame.size.width;
    nightDNSImg.imgData.nHeight = in->src_frame.size.height;
    nightDNSImg.imgData.nFormat = BST_IMAGE_TYPE_NV21;
    nightDNSImg.imgData.nRowPitch[0] = nWidth;
    nightDNSImg.imgData.nRowPitch[1] = nWidth;
    CMR_LOGI("EXP_ISO, %d %d",
        cxt->ae_aux_info.param[0].exp_time / 1000000,
        cxt->ae_aux_info.param[0].iso);
    nightDNSImg.metaData.nExpo =
        cxt->ae_aux_info.param[0].exp_time /1000000;
    nightDNSImg.metaData.nISO = cxt->ae_aux_info.param[0].iso;
    CMR_LOGD("process: w h[%d %d] pPlanePtr[%x %x] nExpo %d nISO %d",
        nightDNSImg.imgData.nWidth,
        nightDNSImg.imgData.nHeight,
        nightDNSImg.imgData.pPlanePtr[0],
        nightDNSImg.imgData.pPlanePtr[1],
        nightDNSImg.metaData.nExpo, nightDNSImg.metaData.nISO);
    ret =  bstYUVNIGHTDNSRender(&nightDNSImg);
    if (ret != 0) {
        CMR_LOGE("Fail to call the nightdns_function, ret =%d",ret);
    }
    nightdns_handle->dst_addr = *dst_addr;
    save_yuv_file(class_handle, -1);
    if (nightdns_handle->is_stop) {
        CMR_LOGE("nightdns_handle is stop");
        goto exit;
    }

    CMR_LOGI("save_frame_count %d", nightdns_handle->common.save_frame_count);

    if (NIGHTDNS_CAP_NUM == nightdns_handle->common.save_frame_count) {
        cmr_bzero(&out->dst_frame, sizeof(struct img_frm));
        oem_handle = nightdns_handle->common.ipm_cxt->init_in.oem_handle;
        nightdns_handle->frame_in = *in;

        nightdns_handle->common.receive_frame_count = 0;
        nightdns_handle->common.save_frame_count = 0;
        out->private_data = nightdns_handle->common.ipm_cxt->init_in.oem_handle;
        out->dst_frame = nightdns_handle->frame_in.dst_frame;
        CMR_LOGI("nightdns process done, addr 0x%lx   %ld %ld dst_frame 0x%lx",
                 nightdns_handle->dst_addr.addr_y, nightdns_handle->width,
                 nightdns_handle->height, nightdns_handle->frame_in.dst_frame);

        if (nightdns_handle->reg_cb) {
            (nightdns_handle->reg_cb)(IPM_TYPE_NIGHTDNS, out);
        }
    }

exit:
    CMR_LOGD("X");
    return ret;
}

static cmr_int nightdns_process_thread_proc(struct cmr_msg *message,
                                            void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_nightdns *class_handle = (struct class_nightdns *)private_data;
    cmr_u32 evt = 0;
    struct ipm_frame_out out;
    struct ipm_frame_in *in;
    struct nightdns_info *p_data;

    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_3DNR_INIT:
        CMR_LOGD("nightdns thread inited.");
        break;
    case CMR_EVT_3DR_PRE_PROC:
        CMR_LOGI("nightdns pre_proc");
        nightdns_frame_proc(class_handle);
        break;
    case CMR_EVT_3DNR_SAVE_FRAME:
        CMR_LOGI("nightdns save frame");
        in = message->data;
        ret = nightdns_save_frame(class_handle, in);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("nightdns save frame failed.");
        }
        break;
/*
    case CMR_EVT_3DNR_PROCESS:
        CMR_LOGD("CMR_EVT_3DNR_PROCESS");
        p_data = message->data;
        ret = threednr_process_frame(class_handle, p_data);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("3dnr process frame failed.");
        }
        break;
*/
    case CMR_EVT_3DNR_DEINIT:
        CMR_LOGD("nightdns thread exit.");
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int nightdns_pre_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_nightdns *nightdns_handle = (struct class_nightdns *)class_handle;
    CMR_MSG_INIT(message);

    if (!class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    message.msg_type = CMR_EVT_3DR_PRE_PROC;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;

    ret = cmr_thread_msg_send(nightdns_handle->nightdns_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to nightdns thread.");
    }

    return ret;
}

static cmr_int nightdns_frame_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_nightdns *nightdns_handle = (struct class_nightdns *)class_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 frame_in_cnt = 0;
    cmr_handle oem_handle = NULL;
    ipm_get_sensor_info get_sensor_info;
    ipm_sensor_ioctl sensor_ioctl;
    ipm_isp_ioctl isp_ioctl;
    struct common_sn_cmd_param sn_param;
    struct sensor_exp_info sensor_info;
    cmr_u32 dnr_enable = 1;
    struct common_isp_cmd_param isp_param;
    if (!nightdns_handle) {
        CMR_LOGE("nightdns_handlee is NULL");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }
    frame_in_cnt = ++nightdns_handle->common.receive_frame_count;
    sensor_id = nightdns_handle->common.ipm_cxt->init_in.sensor_id;
    get_sensor_info = nightdns_handle->common.ipm_cxt->init_in.get_sensor_info;
    sensor_ioctl = nightdns_handle->common.ipm_cxt->init_in.ipm_sensor_ioctl;
    isp_ioctl = nightdns_handle->common.ipm_cxt->init_in.ipm_isp_ioctl;
    oem_handle = nightdns_handle->common.ipm_cxt->init_in.oem_handle;
    CMR_LOGD("frame cnt %d", frame_in_cnt);
    get_sensor_info(oem_handle, sensor_id, &sensor_info);

    CMR_LOGD("nightdns enable = %d", dnr_enable);
    return ret;
}

static cmr_int nightdns_post_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}

static cmr_int nightdns_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_nightdns *nightdns_handle = (struct class_nightdns *)class_handle;
    struct camera_context *cxt = NULL;
    ipm_sensor_ioctl sensor_ioctl;
    struct common_sn_cmd_param sn_param;
    struct img_addr *addr;
    cmr_u32 frame_in_cnt;
    struct img_size size;
    cmr_handle oem_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 dnr_enable = 0;
    ipm_get_sensor_info get_sensor_info;
    cmr_u32 cur_num = nightdns_handle->g_num;
    BSTImage nightDNSImg;
    int nWidth = 0;
    cxt = (struct camera_context *)in->private_data;
    if (!out) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }
    CMR_LOGD("ipm_frame_in.private_data 0x%lx", (cmr_int)in->private_data);
    addr = &in->dst_frame.addr_vir;
    size = in->src_frame.size;

    CMR_LOGD("get one frame, num %d, %d", cur_num, nightdns_handle->g_totalnum);
    if (nightdns_handle->g_totalnum < NIGHTDNS_CAP_NUM) {
        nightdns_handle->g_info_3dnr[cur_num].class_handle = class_handle;
        memcpy(&nightdns_handle->g_info_3dnr[cur_num].in, in,
               sizeof(struct ipm_frame_in));
        nightdns_handle->g_info_3dnr[cur_num].cur_frame_num = cur_num;
        CMR_LOGD("fd 0x%x yaddr 0x%x", in->src_frame.fd, in->src_frame.addr_vir.addr_y);
    } else {
        CMR_LOGE("got more than %d nightdns capture images, now got %d images",
                 NIGHTDNS_CAP_NUM, nightdns_handle->g_totalnum);
    }

    ret = req_3dnr_save_frame(nightdns_handle, in);
    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("req save frame fail");
    }

    nightdns_handle->g_num++;
    nightdns_handle->g_num = nightdns_handle->g_num % NIGHTDNS_CAP_NUM;
    nightdns_handle->g_totalnum++;
    CMR_LOGD("g_num g_totalnum(%d %d)",
        nightdns_handle->g_num, nightdns_handle->g_totalnum);
    CMR_LOGD("save_frame_count ev_effect_frame_interval %d %d",
        nightdns_handle->common.save_frame_count,
        nightdns_handle->ev_effect_frame_interval);
    //nightnds  process
    if (nightdns_handle->common.save_frame_count ==
        (NIGHTDNS_CAP_NUM + nightdns_handle->ev_effect_frame_interval)) {
        cmr_bzero(&out->dst_frame, sizeof(struct img_frm));
        sensor_ioctl = nightdns_handle->common.ipm_cxt->init_in.ipm_sensor_ioctl;
        //isp_ioctl = nightdns_handle->common.ipm_cxt->init_in.ipm_isp_ioctl;
        oem_handle = nightdns_handle->common.ipm_cxt->init_in.oem_handle;
        nightdns_handle->frame_in = nightdns_handle->g_info_3dnr[0].in;
        CMR_LOGD("frame_in = %p", &(nightdns_handle->frame_in));
        ret = nightdns_process_frame(class_handle, addr, size);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("nightdns process fail");
        }
        out->dst_frame = in->dst_frame;
        out->private_data = in->private_data;
        // ev adj end
        //cxt->skipframe = 0;
        struct sensor_exp_info sensor_info;
        sensor_id = nightdns_handle->common.ipm_cxt->init_in.sensor_id;
        get_sensor_info = nightdns_handle->common.ipm_cxt->init_in.get_sensor_info;
/*
        if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info.image_format) {
            struct camera_context *cxt = (struct camera_context *)oem_handle;
            //stop ev adjust
            struct common_isp_cmd_param parm;
            parm.snp_ae_param.enable = dnr_enable;
            parm.snp_ae_param.ev_effect_valid_num =
            cxt->sn_cxt.cur_sns_ex_info.exp_valid_frame_num + 4;
            parm.snp_ae_param.ev_adjust_count = 5;
            parm.snp_ae_param.type = SNAPSHOT_3DNR;
            parm.snp_ae_param.ev_value[0] = 0;
            parm.snp_ae_param.ev_value[1] = 0;
            parm.snp_ae_param.ev_value[2] = 0;
            parm.snp_ae_param.ev_value[3] = 0;
            parm.snp_ae_param.ev_value[4] = 0;
            parm.snp_ae_param.ev_value[5] = 0;
            parm.snp_ae_param.ev_value[6] = 0;
            parm.snp_ae_param.ev_value[7] = 0;
            ret = camera_isp_ioctl(oem_handle, COM_ISP_SET_AE_ADJUST, &parm);
            if (ret) {
                CMR_LOGE("dnsnight failed to set ev.");
            }
        } else {
            sn_param.cmd_value = 1;
           //TODO
           // ret = sensor_ioctl(oem_handle, COM_SN_SET_HDR_EV, (void *)&sn_param);
            }
		*/
    }

    return ret;
}

static cmr_int nightdns_thread_create(struct class_nightdns *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(
            &class_handle->nightdns_thread, CAMERA_3DNR_MSG_QUEUE_SIZE,
            nightdns_process_thread_proc, (void *)class_handle, "nightdns");
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        class_handle->is_inited = 1;
    }

    return ret;
}

static cmr_int nightdns_thread_destroy(struct class_nightdns *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (class_handle->is_inited) {
        ret = cmr_thread_destroy(class_handle->nightdns_thread);
        class_handle->nightdns_thread = 0;
        class_handle->is_inited = 0;
    }

    return ret;
}

