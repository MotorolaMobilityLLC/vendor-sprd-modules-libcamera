
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

#ifdef CONFIG_CAMERA_3DNR_CAPTURE
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#define LOG_TAG "cmr_3dnr_sw"
#include <dlfcn.h>
#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#include "3dnr_interface.h"
#include <cutils/properties.h>
#include "isp_mw.h"
#include "sw_3dnr_param.h"
#include <string.h>

typedef struct c3dn_io_info {
    c3dnr_buffer_t image[3];
    cmr_u32 width;
    cmr_u32 height;
    cmr_s8 mv_x;
    cmr_s8 mv_y;

    cmr_u8 blending_no;
} c3dnr_io_info_t;

struct thread_3dnr_info {
    cmr_handle class_handle;
    struct ipm_frame_in in;
    struct ipm_frame_out out;
    cmr_u32 cur_frame_num;
};

struct class_3dnr {
    struct ipm_common common;
    cmr_u8 *alloc_addr[CAP_3DNR_NUM];
    struct thread_3dnr_info g_info_3dnr[CAP_3DNR_NUM];
    cmr_uint mem_size;
    cmr_uint width;
    cmr_uint height;
    cmr_uint small_width;
    cmr_uint small_height;
    cmr_uint is_inited;
    cmr_handle threednr_thread;
    cmr_handle scaler_thread;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    union c3dnr_buffer out_buf;
    cmr_uint out_buf_phy;
    cmr_uint out_buf_vir;
    cmr_uint small_buf_phy[CAP_3DNR_NUM];
    cmr_uint small_buf_vir[CAP_3DNR_NUM];
    cmr_s32 out_buf_fd;
    cmr_s32 small_buf_fd[CAP_3DNR_NUM];
    cmr_u32 g_num;
    cmr_u32 g_totalnum;
    cmr_uint is_stop;
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
#define CMR_EVT_3DNR_PROCESS (CMR_EVT_3DNR_BASE + 4)

#define CMR_EVT_3DNR_SCALER_BASE (CMR_EVT_IPM_BASE + 0X200)
#define CMR_EVT_3DNR_SCALER_INIT (CMR_EVT_3DNR_SCALER_BASE + 0)
#define CMR_EVT_3DNR_SCALER_START (CMR_EVT_3DNR_SCALER_BASE + 1)
#define CMR_EVT_3DNR_SCALER_DEINIT (CMR_EVT_3DNR_SCALER_BASE + 2)

static int slope_tmp[6] = {4};
static int sigma_tmp[6] = {4};

static struct threednr_tuning_param prev_param, cap_param;
#if 0
static int pre_threthold[4][6] = {{0, 2, 4, 9, 9, 9},
                              {0, 1, 5, 9, 9, 9},
                              {0, 1, 5, 9, 9, 9},
                              {0, 1, 6, 9, 9, 9}};

static int pre_slope[4][6] = {
    {255, 5, 6, 9, 9, 9},
    {255, 5, 6, 9, 9, 9},
    {255, 5, 6, 9, 9, 9},
    {255, 4, 5, 9, 9, 9},
};
uint16_t pre_SearchWindow_x = 11;
uint16_t pre_SearchWindow_y = 11;

static int cap_threthold[4][6] = {{3, 4, 6, 9, 9, 9},
                              {3, 5, 6, 9, 9, 9},
                              {3, 5, 6, 9, 9, 9},
                              {2, 6, 7, 9, 9, 9}};

static int cap_slope[4][6] = {
    {5, 6, 7, 9, 9, 9},
    {5, 6, 7, 9, 9, 9},
    {5, 6, 7, 9, 9, 9},
    {5, 6, 6, 9, 9, 9},
};
uint16_t cap_SearchWindow_x = 11;
uint16_t cap_SearchWindow_y = 11;
#endif

static cmr_int threednr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *class_handle);
static cmr_int threednr_close(cmr_handle class_handle);
static cmr_int threednr_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out);
static cmr_int threednr_post_proc(cmr_handle class_handle);
static cmr_int threednr_process_thread_proc(struct cmr_msg *message,
                                            void *private_data);
static cmr_int threednr_thread_create(struct class_3dnr *class_handle);
static cmr_int threednr_thread_destroy(struct class_3dnr *class_handle);
static cmr_int threednr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in);
cmr_int threednr_start_scale(cmr_handle oem_handle, struct img_frm *src,
                             struct img_frm *dst);
static cmr_int save_yuv(char *filename, char *buffer, uint32_t width,
                        uint32_t height);
static cmr_int threadnr_scaler_process(cmr_handle class_handle,
                                                    struct thread_3dnr_info *p_data);

static struct class_ops threednr_ops_tab_info = {threednr_open, threednr_close,
                                                 threednr_transfer_frame, NULL,
                                                 threednr_post_proc};

struct class_tab_t threednr_tab_info = {
    &threednr_ops_tab_info,
};

cmr_int isp_ioctl_for_3dnr(cmr_handle isp_handle, c3dnr_io_info_t *io_info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}

static cmr_int read_threednr_param_parser(char *parafile,
                                          struct threednr_tuning_param *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    FILE *pFile = fopen(parafile, "rt");
    char line[256];

    if (pFile == NULL || param == NULL) {
        CMR_LOGE("open 3dnr setting file  %s failed. param %p\n", parafile,
                 param);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        memset(param, 0, sizeof(struct threednr_tuning_param));
        fgets(line, 256, pFile);
        char ss[256];

        while (!feof(pFile)) {
            sscanf(line, "%s", ss);

            if (!strcmp(ss, "-th0")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss,
                       &param->threshold[0][0], &param->threshold[0][1],
                       &param->threshold[0][2], &param->threshold[0][3],
                       &param->threshold[0][4], &param->threshold[0][5]);
            } else if (!strcmp(ss, "-th1")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss,
                       &param->threshold[1][0], &param->threshold[1][1],
                       &param->threshold[1][2], &param->threshold[1][3],
                       &param->threshold[1][4], &param->threshold[1][5]);
            } else if (!strcmp(ss, "-th2")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss,
                       &param->threshold[2][0], &param->threshold[2][1],
                       &param->threshold[2][2], &param->threshold[2][3],
                       &param->threshold[2][4], &param->threshold[2][5]);
            } else if (!strcmp(ss, "-th3")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss,
                       &param->threshold[3][0], &param->threshold[3][1],
                       &param->threshold[3][2], &param->threshold[3][3],
                       &param->threshold[3][4], &param->threshold[3][5]);
            } else if (!strcmp(ss, "-sl0")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &param->slope[0][0],
                       &param->slope[0][1], &param->slope[0][2],
                       &param->slope[0][3], &param->slope[0][4],
                       &param->slope[0][5]);
            } else if (!strcmp(ss, "-sl1")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &param->slope[1][0],
                       &param->slope[1][1], &param->slope[1][2],
                       &param->slope[1][3], &param->slope[1][4],
                       &param->slope[1][5]);
            } else if (!strcmp(ss, "-sl2")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &param->slope[2][0],
                       &param->slope[2][1], &param->slope[2][2],
                       &param->slope[2][3], &param->slope[2][4],
                       &param->slope[2][5]);
            } else if (!strcmp(ss, "-sl3")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &param->slope[3][0],
                       &param->slope[3][1], &param->slope[3][2],
                       &param->slope[3][3], &param->slope[3][4],
                       &param->slope[3][5]);
            } else if (!strcmp(ss, "-srx"))
                sscanf(line, "%s %hd", ss, &param->searchWindow_x);
            else if (!strcmp(ss, "-sry"))
                sscanf(line, "%s %hd", ss, &param->searchWindow_y);
            else if (!strcmp(ss, "-gain_thr")) {
                sscanf(line, "%s %d %d %d %d %d %d", ss, &param->gain_thr[0],
                       &param->gain_thr[1], &param->gain_thr[2],
                       &param->gain_thr[3], &param->gain_thr[4],
                       &param->gain_thr[5]);
            } else if (!strcmp(ss, "-recur_str")) {
                sscanf(line, "%s %d", ss, &param->recur_str);
            } else if (!strcmp(ss, "-match_ratio")) {
                sscanf(line, "%s %d %d", ss, &param->match_ratio_sad,
                       &param->match_ratio_pro);
            } else if (!strcmp(ss, "-feat_thr")) {
                sscanf(line, "%s %d", ss, &param->feat_thr);
            } else if (!strcmp(ss, "-zone_size")) {
                sscanf(line, "%s %d", ss, &param->zone_size);
            } else if (!strcmp(ss, "-luma_ratio")) {
                sscanf(line, "%s %d %d", ss, &param->luma_ratio_high,
                       &param->luma_ratio_low);
            }

            fgets(line, 256, pFile);
        }
    }

exit:
    if (pFile != NULL)
        fclose(pFile);

    return ret;
}

static cmr_int read_cap_param_from_file() {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char parafile[] =
        "/data/vendor/cameraserver/bst_tdns_settings_image_cap.txt";
    ret = read_threednr_param_parser(parafile, &cap_param);
    if (ret) {
        CMR_LOGD("failed read 3dnr cap param,use default param, ret %ld", ret);
    } else {
        CMR_LOGD("read 3dnr cap param success!");
    }

    return ret;
}

static cmr_int threednr_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                             struct ipm_open_out *out,
                             cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = NULL;
    cmr_uint size;
    cmr_int i = 0;
    struct c3dnr_param_info param;
    struct ipm_context_t *ipm_cxt = (struct ipm_context_t *)ipm_handle;
    cmr_handle oem_handle = NULL;
    struct ipm_init_in *ipm_in = (struct ipm_init_in *)&ipm_cxt->init_in;
    struct camera_context *cam_cxt = NULL;
    struct isp_context *isp_cxt = NULL;
    cmr_u32 buf_size;
    cmr_u32 buf_num;
    cmr_u32 small_buf_size;
    cmr_u32 small_buf_num;
    cmr_uint sensor_id;
    struct threednr_tuning_param *cap_3dnr_param;
    struct common_isp_cmd_param isp_cmd_parm;
    char flag[PROPERTY_VALUE_MAX];

    CMR_LOGD("E");
    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    threednr_handle = (struct class_3dnr *)malloc(sizeof(struct class_3dnr));
    if (!threednr_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    out->total_frame_number = CAP_3DNR_NUM;

    cmr_bzero(threednr_handle, sizeof(struct class_3dnr));
    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);
    threednr_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    threednr_handle->common.class_type = IPM_TYPE_3DNR;
    threednr_handle->common.ops = &threednr_ops_tab_info;
    threednr_handle->common.receive_frame_count = 0;
    threednr_handle->common.save_frame_count = 0;
    threednr_handle->common.ops = &threednr_ops_tab_info;

    threednr_handle->mem_size = size;

    threednr_handle->height = in->frame_size.height;
    threednr_handle->width = in->frame_size.width;
    if ((threednr_handle->width * 10) / threednr_handle->height <= 10) {
        threednr_handle->small_height = CMR_3DNR_1_1_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_1_1_SMALL_WIDTH;
    } else if ((threednr_handle->width * 10) / threednr_handle->height <= 13) {
        threednr_handle->small_height = CMR_3DNR_4_3_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_4_3_SMALL_WIDTH;
    } else if ((threednr_handle->width * 10) / threednr_handle->height <= 18) {
        threednr_handle->small_height = CMR_3DNR_16_9_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_16_9_SMALL_WIDTH;
    } else if ((threednr_handle->width * 10) / threednr_handle->height <= 20) {
        threednr_handle->small_height = CMR_3DNR_18_9_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_18_9_SMALL_WIDTH;
    } else if ((threednr_handle->width * 10) / threednr_handle->height <= 22) {
        threednr_handle->small_height = CMR_3DNR_19_9_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_19_9_SMALL_WIDTH;
    } else {
        CMR_LOGE("incorrect 3dnr small image mapping, using 16*9 as the "
                 "default setting");
        threednr_handle->small_height = CMR_3DNR_16_9_SMALL_HEIGHT;
        threednr_handle->small_width = CMR_3DNR_16_9_SMALL_WIDTH;
    }
    threednr_handle->reg_cb = in->reg_cb;
    threednr_handle->g_num = 0;
    threednr_handle->is_stop = 0;
    threednr_handle->g_totalnum = 0;
    ret = threednr_thread_create(threednr_handle);
    if (ret) {
        CMR_LOGE("3dnr error: create thread");
        goto free_all;
    }

    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    isp_cxt = (struct isp_context *)&(cam_cxt->isp_cxt);

    buf_size = threednr_handle->width * threednr_handle->height * 3 / 2;
    buf_num = 1;
    small_buf_size =
        threednr_handle->small_width * threednr_handle->small_height * 3 / 2;
    small_buf_num = CAP_3DNR_NUM;

    if (cam_cxt->hal_malloc == NULL) {
        CMR_LOGE("cam_cxt->hal_malloc is NULL");
        goto free_all;
    }

    ret = cam_cxt->hal_malloc(
        CAMERA_SNAPSHOT_3DNR, &small_buf_size, &small_buf_num,
        (cmr_uint *)threednr_handle->small_buf_phy,
        (cmr_uint *)threednr_handle->small_buf_vir,
        threednr_handle->small_buf_fd, cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to malloc buffers for small image");
        goto free_all;
    }
    CMR_LOGD("OK to malloc buffers for small image");

    param.orig_width = threednr_handle->width;
    param.orig_height = threednr_handle->height;
    param.small_width = threednr_handle->small_width;
    param.small_height = threednr_handle->small_height;
    param.total_frame_num = CAP_3DNR_NUM;
    param.poutimg = NULL;
    param.gain = in->adgain;
    param.low_thr = 100;
    param.ratio = 2;
    param.sigma_tmp = sigma_tmp;
    param.slope_tmp = slope_tmp;
    param.yuv_mode = 1; // NV21
    param.control_en = 0x0;
    param.thread_num_acc = 4; // 2 | (1 << 4) | (2 << 6) |(1<<12);
    param.thread_num = 4;     // 2 | (1<<4) | (2<<6) | (1<<12);
    param.preview_cpyBuf = 1;

#if 0
    if (!strcmp(flag, "1")) {
        param.SearchWindow_x = cap_SearchWindow_x;
        param.SearchWindow_y = cap_SearchWindow_y;
    } else {
        param.SearchWindow_x = 21;
        param.SearchWindow_y = 21;
    }
    CMR_LOGD("set SearWindow : %d, %d", param.SearchWindow_x,
             param.SearchWindow_y);

    param.threthold = cap_threthold;
    param.slope = cap_slope;
    param.recur_str = -1;
#endif

    cmr_u32 param_3dnr_index = 0;
    sensor_id = ipm_cxt->init_in.sensor_id;
    param_3dnr_index = threednr_get_sns_match_index(sensor_id);
    cap_3dnr_param = sns_3dnr_param_tab[param_3dnr_index].cap_param;

    property_get("vendor.cam.3dnr_setting_from_file", flag, "0");
    if (!strcmp(flag, "1")) {
        ret = read_cap_param_from_file();
        if (!ret) {
            cap_3dnr_param = &cap_param;
        } else {
            ret = CMR_CAMERA_SUCCESS;
            CMR_LOGD("read 3dnr cap param file failed,using default param.");
        }
    }

    param.SearchWindow_x = cap_3dnr_param->searchWindow_x;
    param.SearchWindow_y = cap_3dnr_param->searchWindow_y;
    param.threthold = cap_3dnr_param->threshold;
    param.slope = cap_3dnr_param->slope;
    param.recur_str = cap_3dnr_param->recur_str;
    param.match_ratio_sad = cap_3dnr_param->match_ratio_sad;
    param.match_ratio_pro = cap_3dnr_param->match_ratio_pro;
    param.feat_thr = cap_3dnr_param->feat_thr;
    param.luma_ratio_high = cap_3dnr_param->luma_ratio_high;
    param.luma_ratio_low = cap_3dnr_param->luma_ratio_low;
    param.zone_size = cap_3dnr_param->zone_size;
    memcpy(param.gain_thr, cap_3dnr_param->gain_thr, (6 * sizeof(int)));
    memcpy(param.reserverd, cap_3dnr_param->reserverd, (16 * sizeof(int)));
    CMR_LOGD("sensor_id %ld index %d search window %hdx%hd threthold[3][2] %d",
             sensor_id, param_3dnr_index, param.SearchWindow_x,
             param.SearchWindow_y, param.threthold[3][2]);

    threednr_set_platform_flag(SPECIAL);

    ret = threednr_init(&param);
    if (ret < 0) {
        CMR_LOGE("Fail to call threednr_init");
    } else {
        CMR_LOGD("ok to call threednr_init");
    }

    ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_SW3DNR_PARAM,
                                &isp_cmd_parm);
    if (ret) {
        CMR_LOGE("failed to get isp param  %ld", ret);
    }

    threednr_setparams(isp_cmd_parm.threednr_param.threshold,
                       isp_cmd_parm.threednr_param.slope);
    *class_handle = threednr_handle;
    CMR_LOGD("X");
    return ret;

free_all:
    if (NULL != threednr_handle)
        free(threednr_handle);
    return CMR_CAMERA_NO_MEM;
}

static cmr_int threednr_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;
    cmr_int i;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;

    CMR_LOGD("E");
    CHECK_HANDLE_VALID(threednr_handle);

    threednr_handle->is_stop = 1;
    // threednr_cancel();
    CMR_LOGD("OK to threednr_cancel");

    ret = threednr_deinit();
    if (ret) {
        CMR_LOGE("3dnr failed to threednr_deinit");
    }

    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if (cam_cxt->hal_free == NULL) {
        CMR_LOGE("cam_cxt->hal_free is NULL");
        goto exit;
    }
    ret = cam_cxt->hal_free(
        CAMERA_SNAPSHOT_3DNR, (cmr_uint *)threednr_handle->small_buf_phy,
        (cmr_uint *)threednr_handle->small_buf_vir,
        threednr_handle->small_buf_fd, CAP_3DNR_NUM, cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to free the small image buffers");
    }

exit:
    ret = threednr_thread_destroy(threednr_handle);
    if (ret) {
        CMR_LOGE("3dnr failed to destroy 3dnr thread");
    }

    if (NULL != threednr_handle)
        free(threednr_handle);

    CMR_LOGD("X");
    return ret;
}

static cmr_int threednr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint y_size = 0;
    cmr_uint uv_size = 0;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;
    cmr_int frame_sn = 0;
    if (!class_handle || !in) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    threednr_handle->common.save_frame_count++;
    if (threednr_handle->common.save_frame_count > CAP_3DNR_NUM) {
        CMR_LOGE("cap cnt error,%ld", threednr_handle->common.save_frame_count);
        return CMR_CAMERA_FAIL;
    }

    y_size = in->src_frame.size.height * in->src_frame.size.width;
    uv_size = in->src_frame.size.height * in->src_frame.size.width / 2;
    frame_sn = threednr_handle->common.save_frame_count - 1;
    if (frame_sn < 0) {
        CMR_LOGE("frame_sn error,%ld.", frame_sn);
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGV(" 3dnr frame_sn %ld, y_addr 0x%lx", frame_sn,
             in->src_frame.addr_vir.addr_y);
    if (threednr_handle->mem_size >= in->src_frame.buf_size &&
        NULL != (void *)in->src_frame.addr_vir.addr_y) {
        threednr_handle->alloc_addr[frame_sn] =
            (cmr_u8 *)(in->src_frame.addr_vir.addr_y);
    } else {
        CMR_LOGE(" 3dnr:mem size:0x%lx,data y_size:0x%lx. 0x%lx",
                 threednr_handle->mem_size, y_size,
                 in->src_frame.addr_vir.addr_y);
    }
    return ret;
}

static cmr_int req_3dnr_process_frame(cmr_handle class_handle,
                                   struct thread_3dnr_info *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;

    CMR_MSG_INIT(message);

    if (!class_handle || !p_data) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    message.data = (struct thread_3dnr_info *)malloc(sizeof(struct thread_3dnr_info));
    if (!message.data) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        return ret;
    }
    memcpy(message.data, p_data, sizeof(struct thread_3dnr_info));
    message.msg_type = CMR_EVT_3DNR_PROCESS;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(threednr_handle->threednr_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to 3dnr thread");
        if (message.data) {
            free(message.data);
        }
    }
    return ret;
}

static cmr_int req_3dnr_scaler_frame(cmr_handle class_handle,
                                   struct thread_3dnr_info *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;

    CMR_MSG_INIT(message);

    if (!class_handle || !p_data) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    message.data = (struct thread_3dnr_info *)malloc(sizeof(struct thread_3dnr_info));
    if (!message.data) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        return ret;
    }
    memcpy(message.data, p_data, sizeof(struct thread_3dnr_info));
    message.msg_type = CMR_EVT_3DNR_SCALER_START;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(threednr_handle->scaler_thread, &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to 3dnr thread");
        if (message.data) {
            free(message.data);
        }
    }
    return ret;
}

static cmr_int threednr_process_frame(cmr_handle class_handle,
                                   struct thread_3dnr_info *p_data) {
    struct thread_3dnr_info *info = (struct thread_3dnr_info *)p_data;
    struct ipm_frame_in *in = &info->in;
    struct ipm_frame_out *out = &info->out;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle =
        (struct class_3dnr *)info->class_handle;
    union c3dnr_buffer small_image;
    c3dnr_cap_gpu_buffer_t orig_image;
    cmr_u32 cur_frm = info->cur_frame_num;
    cmr_handle oem_handle;
    char filename[128];
    // call 3dnr function
    CMR_LOGD("Call the threednr_function() yaddr 0x%x cur_frm: %d",
                    in->src_frame.addr_vir.addr_y, cur_frm);
    orig_image.gpuHandle = out->private_data;
    orig_image.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
    orig_image.bufferU =
        orig_image.bufferY + threednr_handle->width * threednr_handle->height;
    orig_image.bufferV = orig_image.bufferU;

    small_image.cpu_buffer.bufferY =
        (unsigned char *)threednr_handle->small_buf_vir[cur_frm];
    small_image.cpu_buffer.bufferU =
        small_image.cpu_buffer.bufferY +
        threednr_handle->small_width * threednr_handle->small_height;
    small_image.cpu_buffer.bufferV = small_image.cpu_buffer.bufferU;
    small_image.cpu_buffer.fd = threednr_handle->small_buf_fd[cur_frm];
    CMR_LOGV("Call the threednr_function().big Y: %p, small Y: %p."
             " ,threednr_handle->is_stop %ld",
             orig_image.bufferY, small_image.cpu_buffer.bufferY,
             threednr_handle->is_stop);

    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }

    ret = threednr_function_new(&small_image, &orig_image);
    if (ret < 0) {
        CMR_LOGE("Fail to call the threednr_function");
    }

    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }

    {
        char flag[PROPERTY_VALUE_MAX];
        property_get("vendor.cam.3dnr_save_capture_frame", flag, "0");
        if (!strcmp(flag, "1")) { // save output image.
            sprintf(filename, "%ldx%ld_3dnr_handle_frame_index%d.yuv",
                    threednr_handle->width, threednr_handle->height, cur_frm);
            save_yuv(filename, (char *)in->dst_frame.addr_vir.addr_y,
                     threednr_handle->width, threednr_handle->height);
        }
    }

    if ((CAP_3DNR_NUM - 1) == cur_frm) {
        cmr_bzero(&out->dst_frame, sizeof(struct img_frm));
        CMR_LOGD("cur_frame %d", cur_frm);
        oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
        threednr_handle->frame_in = *in;

        threednr_handle->common.receive_frame_count = 0;
        threednr_handle->common.save_frame_count = 0;
        out->private_data = threednr_handle->common.ipm_cxt->init_in.oem_handle;
        out->dst_frame = threednr_handle->frame_in.dst_frame;
        CMR_LOGD("3dnr process done, addr 0x%lx   %ld %ld",
                 threednr_handle->dst_addr.addr_y, threednr_handle->width,
                 threednr_handle->height);

        if (threednr_handle->reg_cb) {
            (threednr_handle->reg_cb)(IPM_TYPE_3DNR, out);
        }
    }

exit:
    CMR_LOGD("X");
    return ret;

}

static cmr_int threednr_process_thread_proc(struct cmr_msg *message,
                                            void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *class_handle = (struct class_3dnr *)private_data;
    cmr_u32 evt = 0;
    struct ipm_frame_out out;
    struct ipm_frame_in *in;
    struct thread_3dnr_info *p_data;

    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_3DNR_INIT:
        CMR_LOGD("3dnr thread inited.");
        break;

    case CMR_EVT_3DNR_PROCESS:
        CMR_LOGD("CMR_EVT_3DNR_PROCESS");
        p_data = message->data;
        ret = threednr_process_frame(class_handle, p_data);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("3dnr process frame failed.");
        }
        break;
    case CMR_EVT_3DNR_DEINIT:
        CMR_LOGD("3dnr thread exit.");
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int threednr_scaler_thread_proc(struct cmr_msg *message,
                                            void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *class_handle = (struct class_3dnr *)private_data;
    cmr_u32 evt = 0;
    struct thread_3dnr_info *p_data;

    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_3DNR_SCALER_INIT:
        CMR_LOGD("3dnr scaler thread inited.");
        break;
    case CMR_EVT_3DNR_SCALER_START:
        p_data = message->data;
        ret = threadnr_scaler_process(class_handle, (struct thread_3dnr_info *)p_data);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("3dnr sclaer process failed.");
        }
        break;
    case CMR_EVT_3DNR_SCALER_DEINIT:
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int threadnr_scaler_process(cmr_handle class_handle,
                                                    struct thread_3dnr_info *p_data) {
    struct thread_3dnr_info *info = (struct thread_3dnr_info *)p_data;
    struct ipm_frame_in *in = &info->in;
    struct ipm_frame_out *out = &info->out;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle =
        (struct class_3dnr *)info->class_handle;
    cmr_handle oem_handle;
    struct img_frm *src, dst;
    cmr_u32 cur_frm;
    char filename[128];

    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }

    cur_frm = info->cur_frame_num;
    CMR_LOGD("E. yaddr 0x %x cur_frm: %d", in->src_frame.addr_vir.addr_y, cur_frm);
    if (NULL == in->private_data) {
        CMR_LOGE("private_data is ptr of camera_context, now is null");
        goto exit;
    }

    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    src = &in->src_frame;
    src->addr_vir.addr_u = in->src_frame.addr_vir.addr_y +
                           threednr_handle->width * threednr_handle->height;
    src->addr_vir.addr_v = src->addr_vir.addr_u;
    src->addr_phy.addr_u = in->src_frame.addr_phy.addr_y +
                           threednr_handle->width * threednr_handle->height;
    src->addr_phy.addr_v = src->addr_vir.addr_u;
    src->data_end.y_endian = 0;
    src->data_end.uv_endian = 0;
    src->rect.start_x = 0;
    src->rect.start_y = 0;
    src->rect.width = threednr_handle->width;
    src->rect.height = threednr_handle->height;
    memcpy(&dst, &in->src_frame, sizeof(struct img_frm));
    dst.buf_size =
        threednr_handle->small_width * threednr_handle->small_height * 3 / 2;
    dst.rect.start_x = 0;
    dst.rect.start_y = 0;
    dst.rect.width = threednr_handle->small_width;
    dst.rect.height = threednr_handle->small_height;
    dst.size.width = threednr_handle->small_width;
    dst.size.height = threednr_handle->small_height;
    dst.addr_phy.addr_y = threednr_handle->small_buf_phy[cur_frm];
    dst.addr_phy.addr_u =
        dst.addr_phy.addr_y +
        threednr_handle->small_width * threednr_handle->small_height;
    dst.addr_phy.addr_v = dst.addr_phy.addr_u;
    dst.addr_vir.addr_y = threednr_handle->small_buf_vir[cur_frm];
    dst.addr_vir.addr_u =
        dst.addr_vir.addr_y +
        threednr_handle->small_width * threednr_handle->small_height;
    dst.addr_vir.addr_v = dst.addr_vir.addr_u;
    dst.fd = threednr_handle->small_buf_fd[cur_frm];
    CMR_LOGD("Call the threednr_start_scale().src Y: 0x%lx, 0x%x, dst Y: "
             "0x%lx, 0x%x",
             src->addr_vir.addr_y, src->fd, dst.addr_vir.addr_y, dst.fd);

    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }

    ret = threednr_start_scale(oem_handle, src, &dst);
    if (ret) {
        CMR_LOGE("Fail to call threednr_start_scale");
        goto exit;
    }

#if 0
    // in->dst_frame.addr_vir      in->src_frame.addr_vir.addr_y +
    // threednr_handle->width * threednr_handle->height * 3 / 2
    if (cur_frm == 0) {
        memcpy((void *)in->dst_frame.addr_vir.addr_y,
               (void *)in->src_frame.addr_vir.addr_y,
               threednr_handle->width * threednr_handle->height * 3 / 2);
    }
#endif

    ret = threednr_save_frame(threednr_handle, in);
    if (ret) {
        CMR_LOGE("failed save 3dnr process");
        goto exit;
    }

    {
        char flag[PROPERTY_VALUE_MAX];
        property_get("vendor.cam.3dnr_save_capture_frame", flag, "0");
        if (!strcmp(flag, "1")) { // save input image.
            CMR_LOGI("save pic: %d, threednr_handle->g_num: %d.", cur_frm,
                     threednr_handle->g_num);
            sprintf(filename, "big_in_%ldx%ld_index_%d.yuv",
                    threednr_handle->width, threednr_handle->height, cur_frm);
            save_yuv(filename, (char *)in->src_frame.addr_vir.addr_y,
                     threednr_handle->width, threednr_handle->height);
            sprintf(filename, "small_in_%ldx%ld_index_%d.yuv",
                    threednr_handle->small_width, threednr_handle->small_height,
                    cur_frm);
            save_yuv(filename, (char *)dst.addr_vir.addr_y,
                     threednr_handle->small_width,
                     threednr_handle->small_height);
        }
    }

    ret = req_3dnr_process_frame(threednr_handle, info);
    if (ret) {
        CMR_LOGE("failed request 3dnr process");
        goto exit;
    }

exit:
    CMR_LOGD("X cur_frm: %d", cur_frm);

    return ret;
}

static cmr_int threednr_post_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}

cmr_int threednr_start_scale(cmr_handle oem_handle, struct img_frm *src,
                             struct img_frm *dst) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !src || !dst) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGV(
        "src size %d %d dst size %d %d rect %d %d %d %d endian %d %d %d %d",
        src->size.width, src->size.height, dst->size.width, dst->size.height,
        src->rect.start_x, src->rect.start_y, src->rect.width, src->rect.height,
        src->data_end.y_endian, src->data_end.uv_endian, dst->data_end.y_endian,
        dst->data_end.uv_endian);

    CMR_LOGD("src fd: 0x%x, yaddr: 0x%lx, fmt: %d dst fd: 0x%x, yaddr: 0x%lx, "
             "fmt: %d",
             src->fd, src->addr_vir.addr_y, src->fmt, dst->fd,
             dst->addr_vir.addr_y, dst->fmt);
    ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst,
                          (cmr_evt_cb)NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to start scaler, ret %ld", ret);
    }
exit:
    CMR_LOGV("done %ld", ret);

    return ret;
}

static cmr_int save_yuv(char *filename, char *buffer, uint32_t width,
                        uint32_t height) {
    char tmp_name[128];
    strcpy(tmp_name, CAMERA_DUMP_PATH);
    strcat(tmp_name, filename);
    FILE *fp;
    fp = fopen(tmp_name, "wb");
    if (fp) {
        fwrite(buffer, 1, width * height * 3 / 2, fp);
        fclose(fp);
        return 0;
    } else
        return -1;
}

static cmr_int threednr_transfer_frame(cmr_handle class_handle,
                                       struct ipm_frame_in *in,
                                       struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *threednr_handle = (struct class_3dnr *)class_handle;
    cmr_u32 frame_in_cnt;
    struct img_addr *addr;
    struct img_size size;
    cmr_handle oem_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 dnr_enable = 0;
    union c3dnr_buffer small_image;
    c3dnr_cap_gpu_buffer_t orig_image;
    cmr_u32 cur_num = threednr_handle->g_num;

    if (!out || !in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (threednr_handle->is_stop) {
        return 0;
    }

    CMR_LOGD("get one frame, num %d, %d", cur_num, threednr_handle->g_totalnum);
    if (threednr_handle->g_totalnum < CAP_3DNR_NUM) {
        threednr_handle->g_info_3dnr[cur_num].class_handle = class_handle;
        memcpy(&threednr_handle->g_info_3dnr[cur_num].in, in,
               sizeof(struct ipm_frame_in));
        memcpy(&threednr_handle->g_info_3dnr[cur_num].out, out,
               sizeof(struct ipm_frame_out));
        threednr_handle->g_info_3dnr[cur_num].cur_frame_num = cur_num;
        CMR_LOGD("yaddr 0x%x", in->src_frame.addr_vir.addr_y);
        ret = req_3dnr_scaler_frame(threednr_handle,
                                            &threednr_handle->g_info_3dnr[cur_num]);
        if (ret) {
            CMR_LOGE("failed to sensor scaler frame");
        }
    } else {
        CMR_LOGE("got more than %d 3dnr capture images, now got %d images",
                 CAP_3DNR_NUM, threednr_handle->g_totalnum);
    }
    threednr_handle->g_num++;
    threednr_handle->g_num = threednr_handle->g_num % CAP_3DNR_NUM;
    threednr_handle->g_totalnum++;

    return ret;
}
static cmr_int threednr_thread_create(struct class_3dnr *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(
            &class_handle->threednr_thread, CAMERA_3DNR_MSG_QUEUE_SIZE,
            threednr_process_thread_proc, (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        ret = cmr_thread_set_name(class_handle->threednr_thread, "threednr");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set 3dnr name");
            ret = CMR_MSG_SUCCESS;
        }

        ret = cmr_thread_create(
            &class_handle->scaler_thread, CAMERA_3DNR_MSG_QUEUE_SIZE,
            threednr_scaler_thread_proc, (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        ret = cmr_thread_set_name(class_handle->scaler_thread, "scaler_3dnr");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set 3dnr name");
            ret = CMR_MSG_SUCCESS;
        }
        class_handle->is_inited = 1;
    }

    return ret;
}

static cmr_int threednr_thread_destroy(struct class_3dnr *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (class_handle->is_inited) {

        ret = cmr_thread_destroy(class_handle->threednr_thread);
        class_handle->threednr_thread = 0;

        ret = cmr_thread_destroy(class_handle->scaler_thread);
        class_handle->scaler_thread = 0;

        class_handle->is_inited = 0;
    }

    return ret;
}

#endif
