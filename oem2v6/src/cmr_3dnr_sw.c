
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
#include "mfnr_adapt_interface.h"
#include <cutils/properties.h>
#include "isp_mw.h"
#include "sw_3dnr_param.h"
#include <string.h>

typedef struct c3dn_io_info {
    mfnr_buffer_t image[3];
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

struct small_buf_info {
    cmr_uint small_buf_phy[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint small_buf_vir[PRE_SW_3DNR_RESERVE_NUM];
    cmr_s32 small_buf_fd[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint used[PRE_SW_3DNR_RESERVE_NUM];
};
typedef struct preview_smallbuf_node {
    uint8_t *buf_vir;
    uint8_t *buf_phy;
    cmr_int buf_fd;
    struct preview_smallbuf_node *next;
} preview_smallbuf_node_t;

typedef struct preview_smallbuf_queue {
    struct preview_smallbuf_node *head;
    struct preview_smallbuf_node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} preview_smallbuf_queue_t;

typedef struct process_pre_3dnr_info {
    struct ipm_frame_in in;
    struct ipm_frame_out out;
    struct preview_smallbuf_node smallbuf_node;
    struct prev_threednr_info threednr_info;
} process_pre_3dnr_info_t;

struct class_3dnr_pre { // 3dnr pre
    struct ipm_common common;
    cmr_uint mem_size;
    cmr_uint width;
    cmr_uint height;
    cmr_uint small_width;
    cmr_uint small_height;
    cmr_uint small_pic_width;
    cmr_uint small_pic_height;
    cmr_uint is_inited;
    cmr_handle threednr_prevthread;
    struct img_addr dst_addr;
    ipm_callback reg_cb;
    struct ipm_frame_in frame_in;
    // ***** for output data **************
    cmr_uint pre_buf_phy[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint pre_buf_vir[PRE_SW_3DNR_RESERVE_NUM];
    cmr_s32 pre_buf_fd[PRE_SW_3DNR_RESERVE_NUM];
// ************************************
// ***** for preview small image ******
#if 0
    cmr_uint small_buf_phy[PRE_SW_3DNR_RESERVE_NUM];
    cmr_uint small_buf_vir[PRE_SW_3DNR_RESERVE_NUM];
    cmr_s32 small_buf_fd[PRE_SW_3DNR_RESERVE_NUM];
#else
    struct preview_smallbuf_queue small_buf_queue;
    struct small_buf_info small_buf_info;
#endif
    // ************************************
    cmr_u32 g_num;
    cmr_uint is_stop;
    mfnr_buffer_t *out_image;
    sem_t sem_3dnr;
    void *proc_handle;
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
    union mfnr_buffer out_buf;
    cmr_uint out_buf_phy;
    cmr_uint out_buf_vir;
    cmr_uint small_buf_phy[CAP_3DNR_NUM];
    cmr_uint small_buf_vir[CAP_3DNR_NUM];
    cmr_s32 out_buf_fd;
    cmr_s32 small_buf_fd[CAP_3DNR_NUM];
    cmr_u32 g_num;
    cmr_u32 g_totalnum;
    cmr_uint is_stop;
    void *proc_handle;
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

#define CMR_EVT_3DNR_PREV_INIT (CMR_EVT_3DNR_BASE + 5)
#define CMR_EVT_3DNR_PREV_START (CMR_EVT_3DNR_BASE + 6)
#define CMR_EVT_3DNR_PREV_EXIT (CMR_EVT_3DNR_BASE + 7)

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
static cmr_int threednr_process_thread_proc(struct cmr_msg *message,void *private_data);
static cmr_int threednr_open_prev(cmr_handle ipm_handle, struct ipm_open_in *in,
                                  struct ipm_open_out *out,
                                  cmr_handle *class_handle);

static cmr_int threednr_close_prev(cmr_handle class_handle);
static cmr_int threednr_transfer_prev_frame(cmr_handle class_handle,
                                            struct ipm_frame_in *in,
                                            struct ipm_frame_out *out);
static cmr_int threednr_thread_create(struct class_3dnr *class_handle);
static cmr_int threednr_thread_destroy(struct class_3dnr *class_handle);
static cmr_int threednr_prevthread_create(struct class_3dnr_pre *class_handle);
static cmr_int threednr_prevthread_destroy(struct class_3dnr_pre *class_handle);
static cmr_int threednr_save_frame(cmr_handle class_handle,
                                   struct ipm_frame_in *in);
cmr_int threednr_start_scale(cmr_handle oem_handle, struct img_frm *src,
                             struct img_frm *dst);
static cmr_int save_yuv(char *filename, char *buffer, uint32_t width,
                        uint32_t height);
static cmr_int threadnr_scaler_process(cmr_handle class_handle,
                                       struct thread_3dnr_info *p_data);
static cmr_int
queue_preview_smallbufer(struct preview_smallbuf_queue *psmall_buf_queue,
                         struct preview_smallbuf_node *pnode);
static cmr_int
init_queue_preview_smallbuffer(struct preview_smallbuf_queue *psmall_buf_queue);
static struct class_ops threednr_ops_tab_info = {threednr_open, threednr_close,
                                                 threednr_transfer_frame, NULL,
                                                 threednr_post_proc};
static cmr_int deinit_queue_preview_smallbufer(
    struct preview_smallbuf_queue *psmall_buf_queue);
static cmr_int
dequeue_preview_smallbuffer(struct preview_smallbuf_queue *psmall_buf_queue,
                            struct preview_smallbuf_node *pnode);
static cmr_int threednr_process_prevthread_proc(struct cmr_msg *message,
                                                void *private_data);
static cmr_int
req_3dnr_preview_frame(cmr_handle class_handle, struct ipm_frame_in *in,
                       struct ipm_frame_out *out,
                       struct prev_threednr_info *threednr_info,
                       struct preview_smallbuf_node *small_buf_node);
struct class_tab_t threednr_tab_info = {
    &threednr_ops_tab_info,
};
static struct class_ops threednr_prev_ops_tab_info = {
    threednr_open_prev, threednr_close_prev, threednr_transfer_prev_frame, NULL,
    threednr_post_proc};

 struct class_tab_t threednr_prev_tab_info = {
    &threednr_prev_ops_tab_info,
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
    mfnr_param_info_t param;
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
    mfnr_cmd_proc_t process_param;

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
    if (cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {

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
    }
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
    CMR_LOGD("nightscepro_flag %d", cam_cxt->nightscepro_flag);
    if (cam_cxt->nightscepro_flag) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle,
                   COM_ISP_GET_MFNR_PARAM, &isp_cmd_parm);
        if (ret) {
            CMR_LOGE("failed to get isp param for mfnr %ld", ret);
        } else {
            param.SearchWindow_x = isp_cmd_parm.mfnr_param.searchWindow_x;
            param.SearchWindow_y = isp_cmd_parm.mfnr_param.searchWindow_y;
            param.recur_str = isp_cmd_parm.mfnr_param.recur_str;
            param.match_ratio_sad = isp_cmd_parm.mfnr_param.match_ratio_sad;
            param.match_ratio_pro = isp_cmd_parm.mfnr_param.match_ratio_pro;
            param.feat_thr = isp_cmd_parm.mfnr_param.feat_thr;
            param.luma_ratio_high = isp_cmd_parm.mfnr_param.luma_ratio_high;
            param.luma_ratio_low = isp_cmd_parm.mfnr_param.luma_ratio_low;
            param.zone_size = isp_cmd_parm.mfnr_param.zone_size;
            memcpy(param.reserverd, isp_cmd_parm.mfnr_param.reserverd,(16 * sizeof(int)));
        }
    } else if (cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {

          ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_SW3DNR_PARAM,
                                &isp_cmd_parm);
          if (ret) {
              CMR_LOGE("failed to get isp param for sw3dnr %ld", ret);
          } else {
              param.SearchWindow_x = isp_cmd_parm.threednr_param.searchWindow_x;
              param.SearchWindow_y = isp_cmd_parm.threednr_param.searchWindow_y;
              param.recur_str = isp_cmd_parm.threednr_param.recur_str;
              param.match_ratio_sad = isp_cmd_parm.threednr_param.match_ratio_sad;
              param.match_ratio_pro = isp_cmd_parm.threednr_param.match_ratio_pro;
              param.feat_thr = isp_cmd_parm.threednr_param.feat_thr;
              param.luma_ratio_high = isp_cmd_parm.threednr_param.luma_ratio_high;
              param.luma_ratio_low = isp_cmd_parm.threednr_param.luma_ratio_low;
              param.zone_size = isp_cmd_parm.threednr_param.zone_size;
              memcpy(param.reserverd, isp_cmd_parm.threednr_param.reserverd,(16 * sizeof(int)));
          }
    }
    param.productInfo = PLATFORM_ID;
    ret = sprd_mfnr_adpt_init((void **)&(threednr_handle->proc_handle),  &param, (void *)cap_3dnr_param);
    if (ret != 0 || !threednr_handle->proc_handle) {
        CMR_LOGE("Fail to call threednr_init, ret = %d",ret);
    } else {
        CMR_LOGD("ok to call threednr_init");
    }
    if (cam_cxt->nightscepro_flag) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_MFNR_PARAM, &isp_cmd_parm);
        if (ret) {
            CMR_LOGE("failed to get isp param	%ld", ret);
        }
        memcpy(process_param.proc_param.setpara_param.thr, isp_cmd_parm.mfnr_param.threshold, (4*sizeof(int)));
        memcpy(process_param.proc_param.setpara_param.slp, isp_cmd_parm.mfnr_param.slope, (4 * sizeof(int)));
        ret = sprd_mfnr_adpt_ctrl((void *)(threednr_handle->proc_handle), SPRD_MFNR_PROC_SET_PARAMS_CMD, (void *)&process_param);
        if(ret != 0){
            CMR_LOGE("failed to set 3dnr init params. ret = %d",ret);
        }
    } else if (cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {
        ret = ipm_in->ipm_isp_ioctl(oem_handle, COM_ISP_GET_SW3DNR_PARAM,
                                    &isp_cmd_parm);
        if (ret) {
            CMR_LOGE("failed to get isp param  %ld", ret);
        }

        memcpy(process_param.proc_param.setpara_param.thr, isp_cmd_parm.threednr_param.threshold, (4*sizeof(int)));
        memcpy(process_param.proc_param.setpara_param.slp, isp_cmd_parm.threednr_param.slope, (4 * sizeof(int)));
        ret = sprd_mfnr_adpt_ctrl((void *)(threednr_handle->proc_handle), SPRD_MFNR_PROC_SET_PARAMS_CMD, (void *)&process_param);
        if(ret != 0){
            CMR_LOGE("failed to set 3dnr init params. ret = %d",ret);
        }
    }

    *class_handle = threednr_handle;
    CMR_LOGD("X");
    return ret;

free_all:
    if (NULL != threednr_handle) {
        free(threednr_handle);
        threednr_handle = NULL;
    }
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

    ret = threednr_thread_destroy(threednr_handle);
    if (ret) {
        CMR_LOGE("3dnr failed to destroy 3dnr thread");
    }

    ret = sprd_mfnr_adpt_deinit((void **)&(threednr_handle->proc_handle));
    if (ret != 0) {
        CMR_LOGE("3dnr failed to threednr_deinit, ret = %d", ret);
    }

    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if((cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) || cam_cxt->nightscepro_flag)
    {
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
    }
exit:

    if (NULL != threednr_handle) {
        free(threednr_handle);
        threednr_handle = NULL;
    }
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

    message.data =
        (struct thread_3dnr_info *)malloc(sizeof(struct thread_3dnr_info));
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
            message.data = NULL;
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

    message.data =
        (struct thread_3dnr_info *)malloc(sizeof(struct thread_3dnr_info));
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
            message.data = NULL;
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
    mfnr_buffer_t small_image,big_image, big_buf,small_buf;
    mfnr_cap_gpu_buffer_t orig_image;
    cmr_u32 cur_frm = info->cur_frame_num;
    cmr_handle oem_handle;
    char filename[128];
    struct camera_context *cam_cxt = NULL;
    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    mfnr_cmd_proc_t process_param;
    sprd_camalg_device_type run_type;
    memset(&orig_image, 0, sizeof(mfnr_cap_gpu_buffer_t));
    ret = sprd_mfnr_get_devicetype(&run_type);
    // call 3dnr function
    CMR_LOGD("Call the threednr_function() yaddr 0x%x cur_frm: %d",
             in->src_frame.addr_vir.addr_y, cur_frm);

    if(run_type != SPRD_CAMALG_RUN_TYPE_VDSP) {
        orig_image.gpuHandle = out->private_data;
        orig_image.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
        orig_image.bufferU =
            orig_image.bufferY + threednr_handle->width * threednr_handle->height;
        orig_image.bufferV = orig_image.bufferU;
    }else {
        big_image.cpu_buffer.bufferY = (unsigned char *)in->src_frame.addr_vir.addr_y;
        big_image.cpu_buffer.bufferU = big_image.cpu_buffer.bufferY +
            threednr_handle->width * threednr_handle->height;
        big_image.cpu_buffer.bufferV = big_image.cpu_buffer.bufferU;
        big_image.cpu_buffer.fd = (int32_t)in->src_frame.fd;
    }
    CMR_LOGI("cam_cxt->snp_cxt.sprd_3dnr_type=%d",cam_cxt->snp_cxt.sprd_3dnr_type);

    if(cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW )
    {
        small_image.cpu_buffer.bufferY =
            (unsigned char *)threednr_handle->small_buf_vir[cur_frm];
        small_image.cpu_buffer.bufferU =
        small_image.cpu_buffer.bufferY +
            threednr_handle->small_width * threednr_handle->small_height;
        small_image.cpu_buffer.bufferV = small_image.cpu_buffer.bufferU;
        small_image.cpu_buffer.fd = threednr_handle->small_buf_fd[cur_frm];
    }
    else
    {
        big_buf.gpu_buffer.handle =
        out->private_data; //(unsigned char *)in->src_frame.addr_vir.addr_y;
        small_image.cpu_buffer.bufferY =
            (unsigned char *)in->src_frame.addr_vir.addr_y +
            threednr_handle->width * threednr_handle->height * 3 / 2;
        small_image.cpu_buffer.bufferU =
            small_image.cpu_buffer.bufferY +
            threednr_handle->small_width * threednr_handle->small_height;
        small_image.cpu_buffer.bufferV = small_image.cpu_buffer.bufferU;
    }
    CMR_LOGV("Call the threednr_function().big Y: %p, small %p."
             " ,threednr_handle->is_stop %ld",
             orig_image.bufferY, small_image.cpu_buffer.bufferY,
             threednr_handle->is_stop);

    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }
    CMR_LOGD("big_buf.gpu_buffer.handle %p", big_buf.gpu_buffer.handle);

    if(run_type != SPRD_CAMALG_RUN_TYPE_VDSP) {
        process_param.proc_param.cap_new_param.small_image = &small_image;
        process_param.proc_param.cap_new_param.orig_image = &orig_image;
        process_param.callWay = 1;
    }else {
        process_param.proc_param.cap_param.small_image = &small_image;
        process_param.proc_param.cap_param.orig_image = &big_image;
        process_param.callWay = 0;
    }
    ret = sprd_mfnr_adpt_ctrl(threednr_handle->proc_handle, SPRD_MFNR_PROC_CAPTURE_CMD, (void *)&process_param);
    if (ret != 0) {
        CMR_LOGE("Fail to call the threednr_function, ret =%d",ret);
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
    CMR_LOGV("X");
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
        ret = threadnr_scaler_process(class_handle,
                                      (struct thread_3dnr_info *)p_data);
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
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;
    struct class_3dnr *threednr_handle =
        (struct class_3dnr *)info->class_handle;
    struct img_frm *src, dst;
    cmr_u32 cur_frm = 0;
    char filename[128];
    if (threednr_handle->is_stop) {
        CMR_LOGE("threednr_handle is stop");
        goto exit;
    }
    oem_handle = threednr_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    cur_frm = info->cur_frame_num;
    CMR_LOGD("E. yaddr 0x %x cur_frm: %d", in->src_frame.addr_vir.addr_y,
             cur_frm);
    if (NULL == in->private_data) {
        CMR_LOGE("private_data is ptr of camera_context, now is null");
        goto exit;
    }

    if(cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW )
    {
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
    }

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
            if(cam_cxt->snp_cxt.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ){
                save_yuv(filename, (char *)dst.addr_vir.addr_y,
                         threednr_handle->small_width,
                         threednr_handle->small_height);
             }else{
                save_yuv(filename, (char *)(in->src_frame.addr_vir.addr_y+threednr_handle->width*threednr_handle->height*3/2),
                         threednr_handle->small_width,
                         threednr_handle->small_height);
             }
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

cmr_int threednr_process_prev_frame(cmr_handle class_handle,
                                    struct ipm_frame_in *in,
                                    struct ipm_frame_out *out)

{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    cmr_u32 frame_in_cnt;
    struct img_addr *addr;
    struct img_size size;
    cmr_handle oem_handle;
    cmr_u32 sensor_id = 0;
    cmr_u32 threednr_enable = 0;
    union mfnr_buffer big_buf, small_buf, video_buf;
    struct img_frm *src, dst;
    cmr_u32 cur_frm_idx;
    char filename[128];
    char tmp_name[64];
    struct mfnr_pre_inparam preview_param;
    mfnr_cmd_proc_t process_param;

    if (!class_handle || !in || !out) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    sem_wait(&threednr_prev_handle->sem_3dnr);
    int64_t time_1 = systemTime(CLOCK_MONOTONIC);

    addr = &in->dst_frame.addr_vir;
    size = in->src_frame.size;

    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    if (threednr_prev_handle->is_stop) {
        return CMR_CAMERA_FAIL;
    }
    int64_t time = systemTime(CLOCK_MONOTONIC);

    {
        char flag[PROPERTY_VALUE_MAX];
        static int index = 0;
        property_get("vendor.cam.post_3dnr_save_scl_data", flag, "0");
        if (!strcmp(flag, "1")) { // save input image.
            CMR_LOGI("save pic: %d, threednr_prev_handle->g_num: %d.", index,
                     threednr_prev_handle->g_num);
            sprintf(filename, "scl_in_%ldx%ld_index_%d.yuv",
                    threednr_prev_handle->width, threednr_prev_handle->height,
                    index);
            save_yuv(filename, (char *)in->src_frame.addr_vir.addr_y,
                     threednr_prev_handle->width, threednr_prev_handle->height);
            sprintf(filename, "scl_out_%ldx%ld_index_%d.yuv",
                    threednr_prev_handle->small_width,
                    threednr_prev_handle->small_height, index);
            save_yuv(filename, (char *)in->dst_frame.addr_vir.addr_y,
                     threednr_prev_handle->small_width,
                     threednr_prev_handle->small_height);
            index++;
        }
    }

    // call 3dnr function
    CMR_LOGV("Call the threednr_function(). before. cnt: %ld, fd: 0x%x",
             threednr_prev_handle->common.save_frame_count, in->src_frame.fd);

    big_buf.cpu_buffer.bufferY =
        (unsigned char *)(in->src_frame.addr_vir.addr_y);
    big_buf.cpu_buffer.bufferU =
        big_buf.cpu_buffer.bufferY +
        threednr_prev_handle->width * threednr_prev_handle->height;
    big_buf.cpu_buffer.bufferV = big_buf.cpu_buffer.bufferU;
    big_buf.cpu_buffer.fd = in->src_frame.fd;

    small_buf.cpu_buffer.bufferY =
        (unsigned char *)
            in->dst_frame.addr_vir.addr_y; // small_buf_vir[cur_frm_idx];
    small_buf.cpu_buffer.bufferU =
        small_buf.cpu_buffer.bufferY +
        threednr_prev_handle->small_width * threednr_prev_handle->small_height;
    small_buf.cpu_buffer.bufferV = small_buf.cpu_buffer.bufferU;
    small_buf.cpu_buffer.fd =
        0; // threednr_prev_handle->small_buf_fd[cur_frm_idx];
    video_buf.cpu_buffer.bufferY =
        (unsigned char *)out->dst_frame.addr_vir.addr_y;
    video_buf.cpu_buffer.bufferU =
        video_buf.cpu_buffer.bufferY +
        threednr_prev_handle->width * threednr_prev_handle->height;
    video_buf.cpu_buffer.bufferV = video_buf.cpu_buffer.bufferU;
    preview_param.gain = in->adgain;
    CMR_LOGD("Call the threednr_function().big Y: %p, 0x%x, small Y: %p, 0x%x, "
             "video Y: %p, 0x%x, adgain: %d"
             " ,threednr_prev_handle->is_stop %ld",
             big_buf.cpu_buffer.bufferY, big_buf.cpu_buffer.fd,
             small_buf.cpu_buffer.bufferY, small_buf.cpu_buffer.fd,
             video_buf.cpu_buffer.bufferY, video_buf.cpu_buffer.fd,
             preview_param.gain, threednr_prev_handle->is_stop);

    process_param.proc_param.pre_param.small_image = &small_buf;
    process_param.proc_param.pre_param.orig_image = &big_buf;
    process_param.proc_param.pre_param.out_image = &video_buf;
    process_param.proc_param.pre_param.gain = preview_param.gain;

    if (threednr_prev_handle->is_stop) {
        return CMR_CAMERA_FAIL;
    }
    int64_t time_2;

    static int index_total = 0;
    int cycle_index;
    static int64_t time_total = 0;
    cycle_index = index_total % 100;
    if (cycle_index == 0)
        time_total = systemTime(CLOCK_MONOTONIC);
    if (cycle_index == 99) {
        CMR_LOGI("3dnr effect costtime:%lld ms , index_total:%d",
                 (systemTime(CLOCK_MONOTONIC) - time_total) / 1000000,
                 index_total);
    }
    index_total++;
    time_2 = systemTime(CLOCK_MONOTONIC);

    char value[128];
    property_get("vendor.cam.3dnrclose", value, "0");
    if (!strcmp(value, "0")) {
        if (video_buf.cpu_buffer.bufferY != NULL) {
            CMR_LOGV("add threednr_function_pre previewbuffer with video :%p , "
                     "small:%p , video buffer:%p",
                     big_buf.cpu_buffer.bufferY, small_buf.cpu_buffer.bufferY,
                     video_buf.cpu_buffer.bufferY);

            if ((small_buf.cpu_buffer.bufferY != NULL) &&
                (big_buf.cpu_buffer.bufferY != NULL)){
                ret = sprd_mfnr_adpt_ctrl(threednr_prev_handle->proc_handle,SPRD_MFNR_PROC_PREVIEW_CMD,(void *)&process_param);		
            }
            else {
                CMR_LOGE(
                    "preview or scale image is null, direct copy video buffer");
                memcpy(video_buf.cpu_buffer.bufferY, big_buf.cpu_buffer.bufferY,
                       threednr_prev_handle->width *
                           threednr_prev_handle->height * 3 / 2);
            }
        } else {
           CMR_LOGI("video buff is null");
            static int index = 0;
            property_get("vendor.cam.3dnr_save", value, "0");
            FILE *fp;
            sprintf(tmp_name, "%ldx%ld_preview_index%d.yuv",
                    threednr_prev_handle->width, threednr_prev_handle->height,
                    index);
            if (!strcmp(value, "1")) {
                sprintf(tmp_name, "%ldx%ld_preview_index%d.yuv",
                        threednr_prev_handle->width,
                        threednr_prev_handle->height, index);
                strcpy(filename, CAMERA_DUMP_PATH);
                strcat(filename, tmp_name);
                fp = fopen(filename, "wb");
                if (fp) {
                    fwrite(big_buf.cpu_buffer.bufferY, 1,
                           threednr_prev_handle->width *
                               threednr_prev_handle->height * 3 / 2,
                           fp);
                    fclose(fp);
                }
                sprintf(tmp_name, "%ldx%ld_preview_index%d.yuv",
                        threednr_prev_handle->small_width,
                        threednr_prev_handle->small_height, index);
                strcpy(filename, CAMERA_DUMP_PATH);
                strcat(filename, tmp_name);
                fp = fopen(filename, "wb");

                if (fp) {
                    fwrite(small_buf.cpu_buffer.bufferY, 1,
                           threednr_prev_handle->small_width *
                               threednr_prev_handle->small_height * 3 / 2,
                           fp);
                    fclose(fp);
                }
            }
            if ((small_buf.cpu_buffer.bufferY != NULL) &&
                (big_buf.cpu_buffer.bufferY != NULL)){
                process_param.proc_param.pre_param.out_image = NULL;
                ret = sprd_mfnr_adpt_ctrl(threednr_prev_handle->proc_handle,SPRD_MFNR_PROC_PREVIEW_CMD,(void *)&process_param);
            }
            if (!strcmp(value, "1")) {
                sprintf(tmp_name, "%ldx%ld_preview_result_index%d.yuv",
                        threednr_prev_handle->width,
                        threednr_prev_handle->height, index);
                strcpy(filename, CAMERA_DUMP_PATH);
                strcat(filename, tmp_name);
                fp = fopen(filename, "wb");
                if (fp) {
                    fwrite(big_buf.cpu_buffer.bufferY, 1,
                           threednr_prev_handle->width *
                               threednr_prev_handle->height * 3 / 2,
                           fp);
                    fclose(fp);
                }
            }
            CMR_LOGV("add threednr_function_pre previewbuffer:%p , small:%p",
                     big_buf.cpu_buffer.bufferY, small_buf.cpu_buffer.bufferY);
            index++;
        }
    } else {
        CMR_LOGI("force to close 3dnr, only by pass");
        if (video_buf.cpu_buffer.bufferY != NULL)
            memcpy(video_buf.cpu_buffer.bufferY, big_buf.cpu_buffer.bufferY,
                   threednr_prev_handle->width * threednr_prev_handle->height *
                       3 / 2);
    }

    if (ret != 0) {
        CMR_LOGE("Fail to call the threednr_function, %d",ret);
    }
    sem_post(&threednr_prev_handle->sem_3dnr);
exit:
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

    CMR_LOGV("src fd: 0x%x, yaddr: 0x%lx, fmt: %d dst fd: 0x%x, yaddr: 0x%lx, "
             "fmt: %d",
             src->fd, src->addr_vir.addr_y, src->fmt, dst->fd,
             dst->addr_vir.addr_y, dst->fmt);
    ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst,
                          (cmr_evt_cb)NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to start scaler, ret %ld", ret);
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);

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

cmr_int threednr_open_prev(cmr_handle ipm_handle, struct ipm_open_in *in,
                           struct ipm_open_out *out, cmr_handle *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle = NULL;
    cmr_uint size;
    cmr_int i = 0;
    struct mfnr_param_info param;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;
    struct preview_context *prev_cxt = NULL;
    struct ipm_context_t *handle = (struct ipm_context_t *)ipm_handle;
    // struct isp_context *isp_cxt = NULL;
    cmr_u32 buf_size;
    cmr_u32 buf_num;
    cmr_u32 small_buf_size;
    cmr_u32 small_buf_num;
    struct preview_smallbuf_node smallbuff_node;
    cmr_uint sensor_id;
    struct threednr_tuning_param *prev_3dnr_param;
    char flag[PROPERTY_VALUE_MAX];

    CMR_LOGV("E");
    if (!out || !in || !ipm_handle || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    threednr_prev_handle =
        (struct class_3dnr_pre *)malloc(sizeof(struct class_3dnr_pre));
    if (!threednr_prev_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }

    cmr_bzero(threednr_prev_handle, sizeof(struct class_3dnr_pre));
    cmr_bzero(&smallbuff_node, sizeof(struct preview_smallbuf_node));
    ret = threednr_prevthread_create(threednr_prev_handle);
    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("threednr_prevthread_create failed");
        return CMR_CAMERA_FAIL;
    }
    size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);

    CMR_LOGD("in->frame_size.width = %d,in->frame_size.height = %d",
             in->frame_size.width, in->frame_size.height);

    sem_init(&threednr_prev_handle->sem_3dnr, 0, 1);
    threednr_prev_handle->reg_cb = in->reg_cb;
    threednr_prev_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    threednr_prev_handle->common.class_type = IPM_TYPE_3DNR_PRE;
    threednr_prev_handle->common.ops = &threednr_prev_ops_tab_info;

    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;
    prev_cxt = (struct preview_context *)&cam_cxt->prev_cxt;

    init_queue_preview_smallbuffer(&(threednr_prev_handle->small_buf_queue));

    threednr_prev_handle->mem_size = size;
    threednr_prev_handle->height = in->frame_size.height;
    threednr_prev_handle->width = in->frame_size.width;
    threednr_prev_handle->small_height = in->frame_size.height / 4;
    threednr_prev_handle->small_width = in->frame_size.width / 4;
    threednr_prev_handle->is_stop = 0;
    threednr_prev_handle->g_num = 0;
    for (int i = 0; i < PRE_SW_3DNR_RESERVE_NUM; i++) {
        threednr_prev_handle->small_buf_info.used[i] = 0;
    }

    buf_size =
        threednr_prev_handle->width * threednr_prev_handle->height * 3 / 2;
    buf_num = PRE_SW_3DNR_RESERVE_NUM;
    small_buf_size = threednr_prev_handle->small_width *
                     threednr_prev_handle->small_height * 3 / 2;
    small_buf_num = PRE_SW_3DNR_RESERVE_NUM;

    if (NULL != cam_cxt->hal_malloc) {
        if (0 !=
            cam_cxt->hal_malloc(CAMERA_PREVIEW_3DNR, &buf_size, &buf_num,
                                threednr_prev_handle->pre_buf_phy,
                                threednr_prev_handle->pre_buf_vir,
                                threednr_prev_handle->pre_buf_fd,
                                cam_cxt->client_data)) {
            CMR_LOGE("Fail to alloc mem for 3DNR preview");
        } else {
            CMR_LOGD("OK to alloc mem for 3DNR preview");
        }
        if (0 !=
            cam_cxt->hal_malloc(
                CAMERA_PREVIEW_SCALE_3DNR, &small_buf_size, &small_buf_num,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_phy,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_vir,
                threednr_prev_handle->small_buf_info.small_buf_fd,
                cam_cxt->client_data)) {
            CMR_LOGE("Fail to malloc buffers for small image");
        } else {
            CMR_LOGD("OK to malloc buffers for small image");
        }
    } else {
        CMR_LOGE("cam_cxt->hal_malloc is NULL");
    }

    for (int i = 0; i < PRE_SW_3DNR_RESERVE_NUM; i++) {
        smallbuff_node.buf_vir =
            (uint8_t *)threednr_prev_handle->small_buf_info.small_buf_vir[i];
        smallbuff_node.buf_phy =
            (uint8_t *)threednr_prev_handle->small_buf_info.small_buf_phy[i];
        smallbuff_node.buf_fd =
            threednr_prev_handle->small_buf_info.small_buf_fd[i];
        queue_preview_smallbufer(&(threednr_prev_handle->small_buf_queue),
                                 &(smallbuff_node));
    }
    param.orig_width = threednr_prev_handle->width;
    param.orig_height = threednr_prev_handle->height;
    param.small_width = threednr_prev_handle->small_width;
    param.small_height = threednr_prev_handle->small_height;
    param.total_frame_num = PRE_3DNR_NUM;
    // need release
    threednr_prev_handle->out_image =
        (union mfnr_buffer *)malloc(sizeof(union mfnr_buffer));
    threednr_prev_handle->out_image->cpu_buffer.bufferY =
        (uint8_t *)threednr_prev_handle->pre_buf_vir[0];
    threednr_prev_handle->out_image->cpu_buffer.bufferU =
        threednr_prev_handle->out_image->cpu_buffer.bufferY +
        threednr_prev_handle->width * threednr_prev_handle->height;
    threednr_prev_handle->out_image->cpu_buffer.bufferV =
        threednr_prev_handle->out_image->cpu_buffer.bufferU;
    threednr_prev_handle->out_image->cpu_buffer.fd =
        threednr_prev_handle->pre_buf_fd[0];
    param.poutimg = NULL; // threednr_prev_handle->out_image;
    param.gain = 16;
    param.low_thr = 100;
    param.ratio = 2;
    param.sigma_tmp = sigma_tmp;
    param.slope_tmp = slope_tmp;
    param.yuv_mode = 0; // NV12?
    param.control_en = 0x0;
    param.thread_num_acc = 2 | (1 << 4) | (2 << 6) | (1 << 12);
    param.thread_num = 2 | (1 << 4) | (2 << 6) | (1 << 12);
    param.preview_cpyBuf = 1;

    cmr_u32 param_3dnr_index = 0;
    sensor_id = handle->init_in.sensor_id;
    param_3dnr_index = threednr_get_sns_match_index(sensor_id);
    prev_3dnr_param = sns_3dnr_param_tab[param_3dnr_index].prev_param;

    param.SearchWindow_x = prev_3dnr_param->searchWindow_x;
    param.SearchWindow_y = prev_3dnr_param->searchWindow_y;
    param.threthold = prev_3dnr_param->threshold;
    param.slope = prev_3dnr_param->slope;
    param.recur_str = prev_3dnr_param->recur_str;
    param.match_ratio_sad = prev_3dnr_param->match_ratio_sad;
    param.match_ratio_pro = prev_3dnr_param->match_ratio_pro;
    param.feat_thr = prev_3dnr_param->feat_thr;
    param.luma_ratio_high = prev_3dnr_param->luma_ratio_high;
    param.luma_ratio_low = prev_3dnr_param->luma_ratio_low;
    param.zone_size = prev_3dnr_param->zone_size;
    memcpy(param.gain_thr, prev_3dnr_param->gain_thr, (6 * sizeof(int)));
    memcpy(param.reserverd, prev_3dnr_param->reserverd, (16 * sizeof(int)));
    CMR_LOGD("sensor_id %ld index %d search window %hdx%hd threthold[3][2] %d",
             sensor_id, param_3dnr_index, param.SearchWindow_x,
             param.SearchWindow_y, param.threthold[3][2]);

    param.productInfo = PLATFORM_ID;
    ret = sprd_mfnr_adpt_init((void **)&(threednr_prev_handle->proc_handle), &param, (void *)prev_3dnr_param);
    if (ret != 0) {
        CMR_LOGE("Fail to call preview threednr_init");
    } else {
        CMR_LOGD("ok to call preview threednr_init");
    }
    *class_handle = (cmr_handle)threednr_prev_handle;

    CMR_LOGV("X");
    return ret;
}

static cmr_int threednr_prevthread_create(struct class_3dnr_pre *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(&class_handle->threednr_prevthread,
                                /*CAMERA_3DNR_MSG_QUEUE_SIZE*/ 8,
                                threednr_process_prevthread_proc,
                                (void *)class_handle, "threednr_prv");
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }
        message.msg_type = CMR_EVT_3DNR_PREV_INIT;
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        ret = cmr_thread_msg_send(class_handle->threednr_prevthread, &message);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;

        } else
            class_handle->is_inited = 1;
    }

    return ret;
}

static cmr_int threednr_prevthread_destroy(struct class_3dnr_pre *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (class_handle->is_inited) {

        ret = cmr_thread_destroy(class_handle->threednr_prevthread);
        class_handle->threednr_prevthread = 0;

        class_handle->is_inited = 0;
    }

    return ret;
}

cmr_int threednr_close_prev(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    cmr_int i;
    cmr_handle oem_handle = NULL;
    struct camera_context *cam_cxt = NULL;

    CMR_LOGD("E");
    CHECK_HANDLE_VALID(threednr_prev_handle);

    threednr_prev_handle->is_stop = 1;
    ret = threednr_prevthread_destroy(threednr_prev_handle);
    if (ret) {
        CMR_LOGE("3dnr failed to destroy 3dnr prev thread");
    }

    CMR_LOGI("OK to threednr_cancel for preview");
    ret = sprd_mfnr_adpt_deinit((void **)&(threednr_prev_handle->proc_handle));
    if (ret != 0) {
        CMR_LOGE("3dnr preview failed to threednr_deinit");
    }

    sem_destroy(&threednr_prev_handle->sem_3dnr);
    oem_handle = threednr_prev_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if (NULL != cam_cxt->hal_malloc) {
        if (0 !=
            cam_cxt->hal_free(CAMERA_PREVIEW_3DNR,
                              threednr_prev_handle->pre_buf_phy,
                              threednr_prev_handle->pre_buf_vir,
                              threednr_prev_handle->pre_buf_fd,
                              PRE_SW_3DNR_RESERVE_NUM, cam_cxt->client_data)) {
            CMR_LOGE("Fail to free the preview output buffer");
        } else {
            CMR_LOGD("OK to free thepreview  output buffer");
        }

        if (0 !=
            cam_cxt->hal_free(
                CAMERA_PREVIEW_SCALE_3DNR,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_phy,
                (cmr_uint *)threednr_prev_handle->small_buf_info.small_buf_vir,
                threednr_prev_handle->small_buf_info.small_buf_fd,
                PRE_SW_3DNR_RESERVE_NUM, cam_cxt->client_data)) {
            CMR_LOGE("Fail to free the small image buffers");
        }
    } else {
        CMR_LOGD("cam_cxt->hal_free is NULL");
    }

    if (threednr_prev_handle->out_image)
        free(threednr_prev_handle->out_image);

    deinit_queue_preview_smallbufer(&(threednr_prev_handle->small_buf_queue));
    if (NULL != threednr_prev_handle)
        free(threednr_prev_handle);
    CMR_LOGD("X");

    return ret;
}

static cmr_int
queue_preview_smallbufer(struct preview_smallbuf_queue *psmall_buf_queue,
                         struct preview_smallbuf_node *pnode) {
    struct preview_smallbuf_node *pnewnode =
        (struct preview_smallbuf_node *)malloc(
            sizeof(struct preview_smallbuf_node));

    CMR_LOGV("add new node:%p , smallbffqueue:%p", pnewnode, psmall_buf_queue);
    pthread_mutex_lock(&psmall_buf_queue->mutex);
    if ((NULL == pnewnode) || (NULL == pnode)) {
        if(NULL != pnewnode) {
           CMR_LOGE("alloc pnewnode failed");
           free(pnewnode);
        } else {
           CMR_LOGE("alloc pnewnode failed");
        }
        pthread_mutex_unlock(&psmall_buf_queue->mutex);
        return -1;
    }
    *pnewnode = *pnode;
    pnewnode->next = NULL;
    if (NULL == psmall_buf_queue->head) {
        psmall_buf_queue->head = pnewnode;
        psmall_buf_queue->tail = pnewnode;
    } else {
        psmall_buf_queue->tail->next = pnewnode;
        psmall_buf_queue->tail = pnewnode;
    }
    CMR_LOGV("queue small buff vir addr:%p, fd:0x%lx", pnode->buf_vir,
             pnode->buf_fd);
    pthread_cond_signal(&psmall_buf_queue->cond);
    pthread_mutex_unlock(&psmall_buf_queue->mutex);
    return 0;
}

static cmr_int deinit_queue_preview_smallbufer(
    struct preview_smallbuf_queue *psmall_buf_queue) {
    struct preview_smallbuf_node *pnode;
    pthread_mutex_lock(&psmall_buf_queue->mutex);
    pnode = psmall_buf_queue->head;
    while (pnode != NULL) {
        psmall_buf_queue->head = psmall_buf_queue->head->next;
        CMR_LOGV("add free pnode:%p , smallbffqueue:%p", pnode,
                 psmall_buf_queue);
        free(pnode);
        pnode = psmall_buf_queue->head;
    }

    psmall_buf_queue->tail = NULL;
    pthread_mutex_unlock(&psmall_buf_queue->mutex);
    pthread_mutex_destroy(&psmall_buf_queue->mutex);
    pthread_cond_destroy(&psmall_buf_queue->cond);
    return 0;
}

static cmr_int
dequeue_preview_smallbuffer(struct preview_smallbuf_queue *psmall_buf_queue,
                            struct preview_smallbuf_node *pnode) {
    struct preview_smallbuf_node *ptemp;
    pthread_mutex_lock(&psmall_buf_queue->mutex);
    // CMR_LOGE("no free small buffer");
    while (psmall_buf_queue->head == NULL) {
        CMR_LOGE("no free small buffer");
        pthread_cond_wait(&psmall_buf_queue->cond, &psmall_buf_queue->mutex);
    }
    {
        ptemp = psmall_buf_queue->head;
        *pnode = *(psmall_buf_queue->head);
        psmall_buf_queue->head = psmall_buf_queue->head->next;
        free(ptemp);
        if (NULL == psmall_buf_queue->head) {
            psmall_buf_queue->tail = NULL;
        }
        CMR_LOGV("dequeue small buff vir addr:%p, fd:0x%lx", pnode->buf_vir,
                 pnode->buf_fd);
        pthread_mutex_unlock(&psmall_buf_queue->mutex);
        return 0;
    }
}

static cmr_int init_queue_preview_smallbuffer(
    struct preview_smallbuf_queue *psmall_buf_queue) {
    psmall_buf_queue->head = NULL;
    psmall_buf_queue->tail = NULL;
    pthread_mutex_init(&psmall_buf_queue->mutex, NULL);
    pthread_cond_init(&psmall_buf_queue->cond, NULL);
    return 0;
}

static cmr_int
req_3dnr_preview_frame(cmr_handle class_handle, struct ipm_frame_in *in,
                       struct ipm_frame_out *out,
                       struct prev_threednr_info *threednr_info,
                       struct preview_smallbuf_node *psmall_buf_node) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    struct process_pre_3dnr_info *ptemp;
    CMR_MSG_INIT(message);

    if (!class_handle || !in || !out) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    message.data = (struct process_pre_3dnr_info *)malloc(
        sizeof(struct process_pre_3dnr_info));
    if (!message.data) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        return ret;
    }
    ptemp = message.data;
    memcpy(&ptemp->in, in, sizeof(struct ipm_frame_in));
    memcpy(&ptemp->out, out, sizeof(struct ipm_frame_out));
    ptemp->smallbuf_node = *psmall_buf_node;
    ptemp->threednr_info.frm_preview = in->src_frame;
    ptemp->threednr_info.frm_smallpreview = in->dst_frame;
    ptemp->threednr_info.frm_video = out->dst_frame;
    ptemp->threednr_info.framtype = threednr_info->framtype;
    ptemp->threednr_info.data = threednr_info->data;
    ptemp->threednr_info.camera_id = threednr_info->camera_id;
    ptemp->threednr_info.caller_handle = threednr_info->caller_handle;

    message.msg_type = CMR_EVT_3DNR_PREV_START;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(threednr_prev_handle->threednr_prevthread,
                              &message);
    if (ret) {
        CMR_LOGE("Failed to send one msg to 3dnr thread");
        if (message.data) {
            free(message.data);
        }
    }
    return ret;
}

cmr_int threednr_transfer_prev_frame(cmr_handle class_handle,
                                     struct ipm_frame_in *in,
                                     struct ipm_frame_out *out) {

    struct img_frm src, dst;
    cmr_u32 cur_frm_idx;
    int ret;
    struct class_3dnr_pre *threednr_prev_handle =
        (struct class_3dnr_pre *)class_handle;
    struct prev_threednr_info *pthreednr_info;
    struct preview_smallbuf_node smallbuf_node;

    if (!in) {
        CMR_LOGE("incorrect parameter");
        return -1;
    }

    ret = dequeue_preview_smallbuffer(&(threednr_prev_handle->small_buf_queue),
                                      &smallbuf_node);
    if (ret != 0) {
        CMR_LOGE(
            "add threednr_transfer_prev_frame failed no free small buffer");
        return -1;
    }

    src = in->src_frame;
    src.data_end.y_endian = 0;
    src.data_end.uv_endian = 0;
    src.rect.start_x = 0;
    src.rect.start_y = 0;
    src.rect.width = threednr_prev_handle->width;
    src.rect.height = threednr_prev_handle->height;
    memcpy(&dst, &src, sizeof(struct img_frm));
    dst.buf_size = threednr_prev_handle->small_width *
                   threednr_prev_handle->small_height * 3 / 2;
    dst.rect.start_x = 0;
    dst.rect.start_y = 0;
    dst.rect.width = threednr_prev_handle->small_width;
    dst.rect.height = threednr_prev_handle->small_height;
    dst.size.width = threednr_prev_handle->small_width;
    dst.size.height = threednr_prev_handle->small_height;
    dst.addr_phy.addr_y =
        (cmr_uint)smallbuf_node
            .buf_phy; // threednr_prev_handle->small_buf_info.small_buf_phy[cur_frm_idx];
    dst.addr_phy.addr_u =
        dst.addr_phy.addr_y +
        threednr_prev_handle->small_width * threednr_prev_handle->small_height;
    dst.addr_phy.addr_v = dst.addr_phy.addr_u;
    dst.addr_vir.addr_y =
        (cmr_uint)smallbuf_node
            .buf_vir; // threednr_prev_handle->small_buf_info.small_buf_vir[cur_frm_idx];
    dst.addr_vir.addr_u =
        dst.addr_vir.addr_y +
        threednr_prev_handle->small_width * threednr_prev_handle->small_height;
    dst.addr_vir.addr_v = dst.addr_vir.addr_u;
    dst.fd =
        smallbuf_node
            .buf_fd; // threednr_prev_handle->small_buf_info.small_buf_fd[cur_frm_idx];
    int64_t time = systemTime(CLOCK_MONOTONIC);

    ret = threednr_start_scale(
        threednr_prev_handle->common.ipm_cxt->init_in.oem_handle, &src, &dst);
    if (ret) {
        CMR_LOGE("Fail to call threednr_start_scale");
        goto exit;
    }
    in->dst_frame = dst;

    ((struct prev_threednr_info *)(in->private_data))->frm_smallpreview = dst;

     pthreednr_info = in->private_data;
     ret = req_3dnr_preview_frame(class_handle, in, out, pthreednr_info,
                                 &smallbuf_node /*cur_frm_idx*/);
exit:
    return ret;
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
    union mfnr_buffer small_image;
    mfnr_cap_gpu_buffer_t orig_image;
    cmr_u32 cur_num = threednr_handle->g_num;

    if (!out) {
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

static cmr_int threednr_process_prevthread_proc(struct cmr_msg *message,
                                                void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_3dnr *class_handle = (struct class_3dnr *)private_data;
    cmr_u32 evt = 0;
    struct ipm_frame_out out;
    struct ipm_frame_in *in;
    struct process_pre_3dnr_info info;
    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)message->msg_type;

    switch (evt) {
    case CMR_EVT_3DNR_PREV_INIT:
        CMR_LOGI("3dnr thread inited.");
        break;
    case CMR_EVT_3DNR_PREV_START:
        CMR_LOGV("CMR_EVT_3DNR_PREV_START.");
        struct class_3dnr_pre *threednr_prev_handle =
            (struct class_3dnr_pre *)class_handle;
        struct ipm_frame_out frame_out;
        info = *((struct process_pre_3dnr_info *)(message->data));
        threednr_process_prev_frame(class_handle, &info.in, &info.out);
        // threednr_prev_handle->small_buf_info.used[info.small_buf_index] = 0;
        queue_preview_smallbufer(&(threednr_prev_handle->small_buf_queue),
                                 &(info.smallbuf_node));
        struct prev_threednr_info threednr_info;
        // threednr_info.frm_preview = info.in.src_frame;
        threednr_info = info.threednr_info;
        frame_out.private_data = &threednr_info;
        frame_out.caller_handle = threednr_info.caller_handle;
        threednr_prev_handle->reg_cb(0, &frame_out);
        break;
    case CMR_EVT_3DNR_PREV_EXIT:
        CMR_LOGD("3dnr prev thread exit.");
        break;
    default:
        break;
    }

    return ret;
}

static cmr_int threednr_thread_create(struct class_3dnr *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(
            &class_handle->threednr_thread, CAMERA_3DNR_MSG_QUEUE_SIZE,
            threednr_process_thread_proc, (void *)class_handle, "threednr");
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
        }

        ret = cmr_thread_create(
            &class_handle->scaler_thread, CAMERA_3DNR_MSG_QUEUE_SIZE,
            threednr_scaler_thread_proc, (void *)class_handle, "scaler_3dnr");
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            return ret;
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

        ret = cmr_thread_destroy(class_handle->scaler_thread);
        class_handle->scaler_thread = 0;

        ret = cmr_thread_destroy(class_handle->threednr_thread);
        class_handle->threednr_thread = 0;



        class_handle->is_inited = 0;
    }

    return ret;
}

#endif
