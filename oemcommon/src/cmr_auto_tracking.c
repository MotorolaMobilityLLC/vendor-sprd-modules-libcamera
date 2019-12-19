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

#define LOG_TAG "cmr_auto_tracking"

#include <cutils/trace.h>
#include "cmr_oem.h"
#include "cmr_ipm.h"
#include <time.h>
#include "4D_Tracking_algo.h"

#define QVGA_WIDTH 320
#define QVGA_HEIGHT 240

struct class_auto_tracking {
    struct ipm_common common;
    cmr_uint small_width;
    cmr_uint small_height;
    cmr_uint small_buf_phy;
    cmr_uint small_buf_vir;
    cmr_s32 small_buf_fd;
    cmr_uint is_closed;
    sem_t sem_auto_tracking;
    ipm_callback reg_cb;
    cmr_u32 save_frame_id;
    cmr_s32 pre_status;
    struct img_addr addr_vir;
    cmr_s32 alg_num;
    cmr_s32 do_with_coordinate;
};

static cmr_int auto_tracking_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                                  struct ipm_open_out *out,
                                  cmr_handle *out_class_handle);
static cmr_int auto_tracking_close(cmr_handle class_handle);
static cmr_int auto_tracking_scaling(cmr_handle class_handle,
                                     struct ipm_frame_in *in, cmr_uint *addr);
static cmr_int auto_tracking_dump(cmr_handle class_handle, cmr_uint addr);
static cmr_int auto_tracking_transfer_frame(cmr_handle class_handle,
                                            struct ipm_frame_in *in,
                                            struct ipm_frame_out *out);
static cmr_int auto_tracking_pre_proc(cmr_handle class_handle);
static cmr_int auto_tracking_post_proc(cmr_handle class_handle);

static struct class_ops auto_tracking_ops_tab_info = {
    auto_tracking_open,           auto_tracking_close,
    auto_tracking_transfer_frame, auto_tracking_pre_proc,
    auto_tracking_post_proc,
};

struct class_tab_t auto_tracking_tab_info = {
    &auto_tracking_ops_tab_info,
};

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            CMR_LOGE("invalid handle");                                        \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

static cmr_int auto_tracking_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                                  struct ipm_open_out *out,
                                  cmr_handle *out_class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle oem_handle = NULL;
    struct class_auto_tracking *auto_tracking_handle = NULL;
    struct ipm_context_t *ipm_cxt = (struct ipm_context_t *)ipm_handle;
    struct ipm_init_in *ipm_in = (struct ipm_init_in *)&ipm_cxt->init_in;
    cmr_u32 small_buf_size;
    cmr_u32 small_buf_num;

    CMR_LOGI("E");
    if (!out || !in || !ipm_handle || !out_class_handle) {
        CMR_LOGE("Invalid Param!");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    auto_tracking_handle = (struct class_auto_tracking *)malloc(
        sizeof(struct class_auto_tracking));
    if (!auto_tracking_handle) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_bzero(auto_tracking_handle, sizeof(struct class_auto_tracking));
    sem_init(&auto_tracking_handle->sem_auto_tracking, 0, 1);

    auto_tracking_handle->small_width = QVGA_WIDTH;
    auto_tracking_handle->small_height = QVGA_HEIGHT;
    small_buf_size = auto_tracking_handle->small_width *
                     auto_tracking_handle->small_height * 3 / 2;
    small_buf_num = 1;

    auto_tracking_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    auto_tracking_handle->common.class_type = IPM_TYPE_AUTO_TRACKING;
    auto_tracking_handle->common.ops = &auto_tracking_ops_tab_info;
    oem_handle = auto_tracking_handle->common.ipm_cxt->init_in.oem_handle;
    ret = ipm_in->ops.mem_malloc(CAMERA_PREVIEW_SCALE_AUTO_TRACKING, oem_handle,
                                 &small_buf_size, &small_buf_num,
                                 &auto_tracking_handle->small_buf_phy,
                                 &auto_tracking_handle->small_buf_vir,
                                 &auto_tracking_handle->small_buf_fd);
    if (ret) {
        CMR_LOGE("Fail to malloc buffers for small image");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    auto_tracking_handle->reg_cb = in->reg_cb;

    // Alg init
    OT_GlobalSetting *a_OTSetting = NULL;
    a_OTSetting =
        (OT_GlobalSetting *)malloc(sizeof(OT_GlobalSetting));
    cmr_bzero(a_OTSetting, sizeof(OT_GlobalSetting));
    a_OTSetting->dImageW = in->frame_full_size.width;
    a_OTSetting->dImageH = in->frame_full_size.height;
    a_OTSetting->dScalingW = QVGA_WIDTH;
    a_OTSetting->dScalingH = QVGA_HEIGHT;
    a_OTSetting->dColorFormat = 0;

    //test Ratio[1-2048]
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.set.ratio", value, "0");
    if (!atoi(value)) {
        a_OTSetting->dOT_Ratio = 1280;
    } else {
        a_OTSetting->dOT_Ratio = atoi(value);
    }
    CMR_LOGD("OT_Init");
    ret = OT_Init(a_OTSetting);

    *out_class_handle = (cmr_handle)auto_tracking_handle;

exit:
    if (ret) {
        if (auto_tracking_handle) {
            if (auto_tracking_handle->small_buf_phy) {
                ipm_in->ops.mem_free(CAMERA_PREVIEW_SCALE_AUTO_TRACKING,
                                     oem_handle,
                                     &auto_tracking_handle->small_buf_phy,
                                     &auto_tracking_handle->small_buf_vir,
                                     &auto_tracking_handle->small_buf_fd, 1);
            }
            sem_destroy(&auto_tracking_handle->sem_auto_tracking);
            free(auto_tracking_handle);
        }
    }
    CMR_LOGI("X");
    return ret;
}

static cmr_int auto_tracking_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_auto_tracking *auto_tracking_handle =
        (struct class_auto_tracking *)class_handle;
    cmr_int i;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;
    struct ipm_init_in *ipm_in = NULL;
    CHECK_HANDLE_VALID(auto_tracking_handle);

    CMR_LOGI("E");
    oem_handle = auto_tracking_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    ipm_in = &auto_tracking_handle->common.ipm_cxt->init_in;

    sem_wait(&auto_tracking_handle->sem_auto_tracking);
    auto_tracking_handle->is_closed = 1;

    ret = ipm_in->ops.mem_free(CAMERA_PREVIEW_SCALE_AUTO_TRACKING, oem_handle,
                               &auto_tracking_handle->small_buf_phy,
                               &auto_tracking_handle->small_buf_vir,
                               &auto_tracking_handle->small_buf_fd, 1);
    if (ret) {
        CMR_LOGE("Fail to free the small image buffers");
    }

    sem_post(&auto_tracking_handle->sem_auto_tracking);
    sem_destroy(&auto_tracking_handle->sem_auto_tracking);
    free(auto_tracking_handle);

    // Alg deinit
    CMR_LOGD("OT_Deinit");
    ret = OT_Deinit();

    CMR_LOGI("X");
    return ret;
}

static cmr_int auto_tracking_scaling(cmr_handle class_handle,
                                     struct ipm_frame_in *in, cmr_uint *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    struct img_frm src, dst;
    struct cmr_op_mean mean;
    struct class_auto_tracking *auto_tracking_handle =
        (struct class_auto_tracking *)class_handle;
    cmr_handle oem_handle = NULL;
    struct ipm_init_in *ipm_in = NULL;
    cmr_bzero(&src, sizeof(struct img_frm));
    cmr_bzero(&dst, sizeof(struct img_frm));
    cmr_bzero(&mean, sizeof(struct cmr_op_mean));

    oem_handle = auto_tracking_handle->common.ipm_cxt->init_in.oem_handle;
    ipm_in = &auto_tracking_handle->common.ipm_cxt->init_in;

    src = in->src_frame;
    src.data_end.y_endian = 0;
    src.data_end.uv_endian = 0;
    src.rect.start_x = 0;
    src.rect.start_y = 0;
    src.rect.width = in->src_frame.size.width;
    src.rect.height = in->src_frame.size.height;
    memcpy(&dst, &src, sizeof(struct img_frm));
    dst.buf_size = auto_tracking_handle->small_width *
                   auto_tracking_handle->small_height * 3 / 2;
    dst.rect.start_x = 0;
    dst.rect.start_y = 0;
    dst.rect.width = auto_tracking_handle->small_width;
    dst.rect.height = auto_tracking_handle->small_height;
    dst.size.width = auto_tracking_handle->small_width;
    dst.size.height = auto_tracking_handle->small_height;
    dst.addr_phy.addr_y = auto_tracking_handle->small_buf_phy;
    dst.addr_phy.addr_u =
        dst.addr_phy.addr_y +
        auto_tracking_handle->small_width * auto_tracking_handle->small_height;
    dst.addr_phy.addr_v = dst.addr_phy.addr_u;
    dst.addr_vir.addr_y = auto_tracking_handle->small_buf_vir;
    dst.addr_vir.addr_u =
        dst.addr_vir.addr_y +
        auto_tracking_handle->small_width * auto_tracking_handle->small_height;
    dst.addr_vir.addr_v = dst.addr_vir.addr_u;
    dst.fd = auto_tracking_handle->small_buf_fd;
    mean.is_sync = 1;

    int64_t t_time = systemTime(CLOCK_MONOTONIC);
    ret = ipm_in->ops.img_scale(oem_handle, auto_tracking_handle, &src, &dst,
                                &mean);
    if (ret) {
        CMR_LOGE("Fail to scale image, ret:%ld", ret);
        goto exit;
    }
    t_time = systemTime(CLOCK_MONOTONIC) - t_time;
    CMR_LOGV("scale src(wxh:%dx%d) to dst(wxh:%dx%d) cost  :%lld ms",
             src.rect.width, src.rect.height, dst.size.width, dst.size.height,
             ns2ms(t_time));

    *addr = dst.addr_vir.addr_y;

exit:
    return ret;
}

static cmr_int auto_tracking_dump(cmr_handle class_handle, cmr_uint addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    FILE *fp = NULL;
    char file_name[100];
    char tmp_str[10];
    char datetime[15] = {0};
    time_t timep;
    struct tm *p;
    cmr_bzero(file_name, 40);
    time(&timep);
    p = localtime(&timep);
    sprintf(datetime, "%04d%02d%02d%02d%02d%02d", (1900 + p->tm_year),
            (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    strcpy(file_name, CAMERA_DUMP_PATH);
    strcat(file_name, "Auto_tracking_");
    strcat(file_name, datetime);
    strcat(file_name, ".NV21");
    fp = fopen(file_name, "wb");
    fwrite((cmr_handle)addr, 1, QVGA_WIDTH * QVGA_HEIGHT * 3 / 2, fp);
    fclose(fp);
    fp = NULL;

    return ret;
}

static cmr_int auto_tracking_transfer_frame(cmr_handle class_handle,
                                            struct ipm_frame_in *in,
                                            struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_auto_tracking *auto_tracking_handle =
        (struct class_auto_tracking *)class_handle;
    struct prev_auto_tracking_info *info = NULL;
    cmr_handle a_pScalingBuf;
    cmr_uint scaling_addr;
    cmr_s32 x_point = 0;
    cmr_s32 y_point = 0;
    cmr_s32 af_status = 0;
    OT_Result a_OTResult;

    CHECK_HANDLE_VALID(auto_tracking_handle);

    if (!in) {
        CMR_LOGE("invalid parameters");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    info = (struct prev_auto_tracking_info *)(in->private_data);
    if (!info) {
        CMR_LOGE("get prev_auto_tracking_info error");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    sem_wait(&auto_tracking_handle->sem_auto_tracking);
    if (auto_tracking_handle->is_closed) {
        goto exit;
    }

    if (0 == in->input.status) {
        auto_tracking_handle->pre_status = in->input.status;
        goto exit;
    }

    //check status
    if (0 == auto_tracking_handle->pre_status || in->input.first_frame) {
        auto_tracking_handle->save_frame_id = in->input.frame_id;
        ret = OT_Stop();
        CMR_LOGD("OT_Stop first");
        auto_tracking_handle->alg_num = 0;
        auto_tracking_handle->pre_status = in->input.status;
    }

    CMR_LOGD("check if miss data:param_frame_id=%d, data_frame_id=%d frm_cnt %d",
        auto_tracking_handle->save_frame_id,
        info->data.frame_num,info->frm_cnt);

    // param frame should be equal to data frame
    if (auto_tracking_handle->save_frame_id == info->frm_cnt) {
        auto_tracking_handle->do_with_coordinate = 1;
    } else {
        auto_tracking_handle->do_with_coordinate = 0;
    }
    CMR_LOGV("input param x_point=%d, y_point=%d,status=%d", in->input.objectX, in->input.objectY,in->input.ot_af_status);

    // get frame id
    ret = OT_GetResultN1(&a_OTResult);
    CMR_LOGD("Get process result dOTFrameID = %d, alg_num = %d",
        a_OTResult.dOTFrameID, auto_tracking_handle->alg_num);
    // if alg is processing, don't send data
    if (a_OTResult.dOTFrameID == auto_tracking_handle->alg_num) {
        /**1. scale to QVGA 320*240 **/
        ret = auto_tracking_scaling(class_handle, in, &scaling_addr);

        // dump
        char prop[PROPERTY_VALUE_MAX];
        property_get("persist.vendor.cam.at.dump.QVGA", prop, "0");
        if (atoi(prop)) {
            ret = auto_tracking_dump(class_handle, scaling_addr);
        }

        /**2. send image to Alg **/
        // Alg do
        a_pScalingBuf = (cmr_handle)scaling_addr;

        if (auto_tracking_handle->do_with_coordinate) {
            CMR_LOGD("OT_Do_With COORDINATE");
            x_point = in->input.objectX;
            y_point = in->input.objectY;
            af_status = in->input.ot_af_status;
            // Alg do with coordinate[x, y]
            CMR_LOGD("input param x_point=%d, y_point=%d,af_status=%d", x_point, y_point, af_status);
            ret = OT_Do(a_pScalingBuf, x_point, y_point, af_status);
            auto_tracking_handle->alg_num++;

            // Alg getresult
            CMR_LOGD("OT_GetResult");
            ret = OT_GetResultN1(&a_OTResult);
            // Callback coordinate
            out->output.objectX = a_OTResult.dMovingX;
            out->output.objectY = a_OTResult.dMovingY;
            out->output.status = a_OTResult.dOTStatus;
            out->output.frame_id = a_OTResult.dOTFrameID;
            out->output.objectSize_X = a_OTResult.dSize_X;
            out->output.objectSize_Y = a_OTResult.dSize_Y;
            out->output.objectAxis1 = a_OTResult.dAxis1;
            out->output.objectAxis2 = a_OTResult.dAxis2;
        }else {
            x_point = 0;
            y_point = 0;
            af_status = in->input.ot_af_status;
            if (info->frm_cnt> auto_tracking_handle->save_frame_id) {
                CMR_LOGD("OT_Do_Without coordinate");
                // Alg do with [0, 0]
                CMR_LOGV("input param x_point=%d, y_point=%d,af_status=%d", x_point, y_point, af_status);
                ret = OT_Do(a_pScalingBuf, x_point, y_point, af_status);
                auto_tracking_handle->alg_num++;

                // Alg getresult
                CMR_LOGD("OT_GetResult");
                ret = OT_GetResultN1(&a_OTResult);
                // Callback coordinate
                out->output.objectX = a_OTResult.dMovingX;
                out->output.objectY = a_OTResult.dMovingY;
                out->output.status = a_OTResult.dOTStatus;
                out->output.frame_id = a_OTResult.dOTFrameID;
                out->output.objectSize_X = a_OTResult.dSize_X;
                out->output.objectSize_Y = a_OTResult.dSize_Y;
                out->output.objectAxis1 = a_OTResult.dAxis1;
                out->output.objectAxis2 = a_OTResult.dAxis2;
            } else {
                CMR_LOGD("Do not Track");
                out->output.objectX = 0;
                out->output.objectY = 0;
                out->output.status = 0;
                out->output.frame_id = 0;
                out->output.objectSize_X = 0;
                out->output.objectSize_Y = 0;
                out->output.objectAxis1 = 0;
                out->output.objectAxis2 = 0;
            }
        }
    } else {
        out->output.objectX = a_OTResult.dMovingX;
        out->output.objectY = a_OTResult.dMovingY;
        out->output.status = a_OTResult.dOTStatus;
        out->output.frame_id = a_OTResult.dOTFrameID;
        out->output.objectSize_X = a_OTResult.dSize_X;
        out->output.objectSize_Y = a_OTResult.dSize_Y;
        out->output.objectAxis1 = a_OTResult.dAxis1;
        out->output.objectAxis2 = a_OTResult.dAxis2;
    }
    CMR_LOGD("auto tracking Callback param:%d,%d,%d,%d SIZEX SIZEY axis1 axis2 "
             "%d %d %d %d",
             out->output.objectX, out->output.objectY, out->output.status,
             out->output.frame_id, out->output.objectSize_X,
             out->output.objectSize_Y, out->output.objectAxis1,
             out->output.objectAxis2);
    out->private_data = (void *)info->camera_id;
    out->caller_handle = in->caller_handle;
    out->output.imageW = in->input.imageW;
    out->output.imageH = in->input.imageH;

    //reset pre_status when failure
    if (AUTO_TRACKING_FAILURE == out->output.status) {
        auto_tracking_handle->pre_status = 0;
    }

    CMR_LOGD("auto tracking Callback param:%d,%d,%d,%d SIZEX SIZEY axis1 axis2 "
             "%d %d %d %d",
             out->output.objectX, out->output.objectY, out->output.status,
             out->output.frame_id, out->output.objectSize_X,
             out->output.objectSize_Y, out->output.objectAxis1,
             out->output.objectAxis2);


    if (auto_tracking_handle->reg_cb) {
        (auto_tracking_handle->reg_cb)(IPM_TYPE_AUTO_TRACKING, out);
    }

exit:
    sem_post(&auto_tracking_handle->sem_auto_tracking);
    return ret;
}

static cmr_int auto_tracking_pre_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/
    (void)class_handle;

    return ret;
}

static cmr_int auto_tracking_post_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/
    (void)class_handle;

    return ret;
}
