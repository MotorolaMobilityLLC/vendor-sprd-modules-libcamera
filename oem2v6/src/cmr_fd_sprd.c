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

#if defined(CONFIG_CAMERA_FACE_DETECT) &&                                      \
    defined(CONFIG_CAMERA_FACE_DETECT_SPRD)

#define LOG_TAG "cmr_fd_sprd"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "SprdOEMCamera.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "sprdfdapi.h"
#include "facealignapi.h"
#include "faceattributeapi.h"
#include "faceattrcnnapi.h"
#include "cmr_oem.h"
#define FD_MAX_FACE_NUM 10
#define FD_RUN_FAR_INTERVAL                                                    \
    8 /* The frame interval to run FAR. For reducing computation cost */
#define FD_THREAD_NUM 2
#define FD_MAX_CNN_FACE_NUM 1

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : -(x))
#endif

struct class_faceattr {
    FA_SHAPE shape;
    FAR_ATTRIBUTE attr;
    int face_id; /* face id gotten from face detection */
};

struct class_faceattr_v2 {
    FA_SHAPE shape;
    FAR_ATTRIBUTE_V2 attr_v2;
    int face_id; /* face id gotten from face detection */
};

struct class_faceattr_array {
    int count;          /* face count      */
    cmr_uint frame_idx; /* The frame when the face attributes are updated */
    struct class_faceattr face[FD_MAX_FACE_NUM + 1]; /* face attricutes */
    struct class_faceattr_v2 face_v2[FD_MAX_FACE_NUM + 1];
};

struct class_fd {
    struct ipm_common common;
    cmr_handle thread_handle;
    cmr_uint is_busy;
    cmr_uint is_inited;
    void *alloc_addr;
    void *alloc_addr_u;
    cmr_uint mem_size;
    cmr_uint frame_cnt;
    cmr_uint frame_total_num;
    struct ipm_frame_in frame_in;
    struct ipm_frame_out frame_out;
    ipm_callback frame_cb;
    struct img_size fd_img_size;
    /* The previous frame is used to directly callback if face detect is busying
     */
    struct img_face_area face_area_prev;
    /*The previous frame is used to make face detection results more stable */
    struct img_face_area face_small_area;
    struct class_faceattr_array faceattr_arr; /* face attributes */
    cmr_uint curr_frame_idx;
    cmr_uint is_get_result;
    FD_HANDLE hDT;              /* Face Detection Handle */
    FA_ALIGN_HANDLE hFaceAlign; /* Handle for face alignment */
    FAR_RECOGNIZER_HANDLE hFAR; /* Handle for face attribute recognition */
    FAR_HANDLE hFAR_v2;
    struct img_frm fd_small;
    cmr_uint is_face_attribute;
    cmr_uint is_smile_capture;
    cmr_uint face_attributes_off;
    struct frm_info trans_frm;
    cmr_uint fd_x;
    cmr_uint fd_y;
    cmr_uint is_face_attribute_init;
    cmr_int work_mode;
    sem_t sem_facearea;
};

struct fd_start_parameter {
    void *frame_data;
    ipm_callback frame_cb;
    cmr_handle caller_handle;
    void *private_data;

    /* FD_IMAGE_CONTEXT */
    cmr_u32 orientation;
    cmr_u32 bright_value;
    cmr_u32 ae_stable;
    cmr_u32 backlight_pro;
    cmr_u32 hist[CAMERA_ISP_HIST_ITEMS];
    cmr_u32 zoom_ratio;
    cmr_u32 frame_id;
};

typedef enum { SPRD_API_MODE = 0, SPRD_API_MODE_V2 } ApiMode;

static cmr_int sprd_fd_api = 0;

static cmr_int fd_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                       struct ipm_open_out *out, cmr_handle *out_class_handle);
static cmr_int fd_close(cmr_handle class_handle);
static cmr_int fd_transfer_frame(cmr_handle class_handle,
                                 struct ipm_frame_in *in,
                                 struct ipm_frame_out *out);
static cmr_int fd_pre_proc(cmr_handle class_handle);
static cmr_int fd_post_proc(cmr_handle class_handle);
static cmr_int fd_start(cmr_handle class_handle,
                        struct fd_start_parameter *param);
static cmr_uint check_size_data_invalid(struct img_size *fd_img_size);
static cmr_int fd_call_init(struct class_fd *class_handle,
                            const struct img_size *fd_img_size);
static cmr_uint fd_is_busy(struct class_fd *class_handle);
static void fd_set_busy(struct class_fd *class_handle, cmr_uint is_busy);
static cmr_int fd_thread_create(struct class_fd *class_handle);
static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data);
static void fd_face_attribute_detect(struct class_fd *class_handle);
cmr_int fd_start_scale(cmr_handle oem_handle, struct img_frm *src,
                       struct img_frm *dst);
static struct class_ops fd_ops_tab_info = {
    fd_open, fd_close, fd_transfer_frame, fd_pre_proc, fd_post_proc,
};

struct class_tab_t fd_tab_info = {
    &fd_ops_tab_info,
};

#define CMR_EVT_FD_START (1 << 16)
#define CMR_EVT_FD_EXIT (1 << 17)
#define CMR_EVT_FD_INIT (1 << 18)
#define CMR__EVT_FD_MASK_BITS                                                  \
    (cmr_u32)(CMR_EVT_FD_START | CMR_EVT_FD_EXIT | CMR_EVT_FD_INIT)

#define CAMERA_FD_MSG_QUEUE_SIZE 5
#define IMAGE_FORMAT "YVU420_SEMIPLANAR"

#define FD_SMALL_SIZE_4_3_WIDTH 640
#define FD_SMALL_SIZE_4_3_HEIGHT 480

#define FD_SMALL_SIZE_16_9_WIDTH 640
#define FD_SMALL_SIZE_16_9_HEIGHT 360

#define FD_SMALL_SIZE_2_1_WIDTH 640
#define FD_SMALL_SIZE_2_1_HEIGHT 320

#define FD_SMALL_SIZE_1_1_WIDTH 480
#define FD_SMALL_SIZE_1_1_HEIGHT 480

#define FD_SMALL_SIZE_20_9_WIDTH 640
#define FD_SMALL_SIZE_20_9_HEIGHT 290

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

static cmr_int fd_open(cmr_handle ipm_handle, struct ipm_open_in *in,
                       struct ipm_open_out *out, cmr_handle *out_class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_fd *fd_handle = NULL;
    struct img_size *fd_img_size;

#ifdef CONFIG_SPRD_FD_LIB_VERSION_2
    sprd_fd_api = SPRD_API_MODE_V2;
#else
    sprd_fd_api = SPRD_API_MODE;
#endif

    if (!out || !in || !ipm_handle || !out_class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    fd_handle = (struct class_fd *)malloc(sizeof(struct class_fd));
    if (!fd_handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }
    cmr_bzero(fd_handle, sizeof(struct class_fd));

    fd_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
    fd_handle->common.class_type = IPM_TYPE_FD;
    fd_handle->common.ops = &fd_ops_tab_info;
    fd_handle->frame_cb = in->reg_cb;
    fd_handle->mem_size =
        in->frame_size.height * in->frame_size.width; // * 3 / 2;
    fd_handle->frame_total_num = in->frame_cnt;
    fd_handle->frame_cnt = 0;
    fd_handle->fd_img_size = in->frame_size;
    fd_handle->face_area_prev.face_count = 0;
    fd_handle->curr_frame_idx = 0;
    fd_handle->faceattr_arr.count = 0;

    CMR_LOGD("mem_size = %ld", fd_handle->mem_size);

    fd_handle->alloc_addr = malloc(fd_handle->mem_size);
    if (!fd_handle->alloc_addr) {
        CMR_LOGE("mem alloc failed");
        goto free_fd_handle;
    }

    fd_handle->alloc_addr_u = malloc(fd_handle->mem_size / 2);
    if (!fd_handle->alloc_addr_u) {
        CMR_LOGE("mem alloc failed");
        goto free_fd_handle;
    }

    ret = fd_thread_create(fd_handle);
    if (ret) {
        CMR_LOGE("failed to create thread.");
        goto free_fd_handle;
    }

    if (in->multi_mode == MODE_SINGLE_FACEID_REGISTER ||
        in->multi_mode == MODE_DUAL_FACEID_REGISTER)
        fd_handle->work_mode = FD_WORKMODE_FACEENROLL;
    else if (in->multi_mode == MODE_SINGLE_FACEID_UNLOCK ||
             in->multi_mode == MODE_DUAL_FACEID_UNLOCK)
        fd_handle->work_mode = FD_WORKMODE_FACEAUTH;
    else
        fd_handle->work_mode = FD_WORKMODE_MOVIE;

#ifdef CONFIG_SPRD_FD_HW_SUPPORT
    struct camera_context *cam_cxt = NULL;
    cmr_handle oem_handle = NULL;
    cmr_u32 fd_small_buf_num = 1;
    float src_ratio;
    float fd_small_4_3_ratio, fd_small_16_9_ratio;
    float fd_small_2_1_ratio, fd_small_1_1_ratio;
    float fd_small_20_9_ratio = 0;

    src_ratio = (float)in->frame_size.width / (float)in->frame_size.height;
    fd_small_4_3_ratio = (float)4 / 3;
    fd_small_16_9_ratio = (float)16 / 9;
    fd_small_2_1_ratio = (float)2 / 1;
    fd_small_1_1_ratio = (float)1 / 1;
    fd_small_20_9_ratio = (float)20 / 9;
    CMR_LOGD("src_ratio = %f", src_ratio);

    if (fabsf(src_ratio - fd_small_4_3_ratio) < 0.001) {
        fd_handle->fd_small.size.width = FD_SMALL_SIZE_4_3_WIDTH;
        fd_handle->fd_small.size.height = FD_SMALL_SIZE_4_3_HEIGHT;
    } else if (fabsf(src_ratio - fd_small_16_9_ratio) < 0.001) {
        fd_handle->fd_small.size.width = FD_SMALL_SIZE_16_9_WIDTH;
        fd_handle->fd_small.size.height = FD_SMALL_SIZE_16_9_HEIGHT;
    } else if (fabsf(src_ratio - fd_small_2_1_ratio) < 0.001) {
        fd_handle->fd_small.size.width = FD_SMALL_SIZE_2_1_WIDTH;
        fd_handle->fd_small.size.height = FD_SMALL_SIZE_2_1_HEIGHT;
    } else if (fabsf(src_ratio - fd_small_1_1_ratio) < 0.001) {
        fd_handle->fd_small.size.width = FD_SMALL_SIZE_1_1_WIDTH;
        fd_handle->fd_small.size.height = FD_SMALL_SIZE_1_1_HEIGHT;
    } else if (fabsf(src_ratio - fd_small_20_9_ratio) < 0.06){
        fd_handle->fd_small.size.width = FD_SMALL_SIZE_20_9_WIDTH;
        fd_handle->fd_small.size.height = FD_SMALL_SIZE_20_9_HEIGHT;
    } else {
        fd_handle->fd_small.size.width = FD_SMALL_SIZE_4_3_WIDTH;
        fd_handle->fd_small.size.height = FD_SMALL_SIZE_4_3_HEIGHT;
        CMR_LOGW("fd_small size w / h was set defalut size.");
    }
    fd_handle->fd_small.buf_size =
        fd_handle->fd_small.size.width * fd_handle->fd_small.size.height * 3;

    oem_handle = fd_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if (cam_cxt->hal_malloc == NULL) {
        CMR_LOGE("cam_cxt->hal_malloc is NULL");
        goto free_fd_handle;
    }
    ret = cam_cxt->hal_malloc(CAMERA_FD_SMALL, &fd_handle->fd_small.buf_size,
                              &fd_small_buf_num,
                              &fd_handle->fd_small.addr_phy.addr_y,
                              &fd_handle->fd_small.addr_vir.addr_y,
                              &fd_handle->fd_small.fd, cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to malloc buffers for fd_small image");
        goto free_fd_handle;
    }
    CMR_LOGD("OK to malloc buffers for fd_small image%ld",
             fd_handle->fd_small.buf_size);

    CMR_LOGD("sprd_fd_api %d fd_small size %dx%d work_mode %d", sprd_fd_api,
             fd_handle->fd_small.size.width, fd_handle->fd_small.size.height,
             fd_handle->work_mode);
    ret = fd_call_init(fd_handle, &fd_handle->fd_small.size);
#else
    fd_img_size = &in->frame_size;
    CMR_LOGD("sprd_fd_api %d fd_img_size %dx%d work_mode %d", sprd_fd_api,
             fd_img_size->width, fd_img_size->height, fd_handle->work_mode);
    ret = fd_call_init(fd_handle, fd_img_size);
#endif

    sem_init(&fd_handle->sem_facearea, 0 , 1);

    if (ret) {
        CMR_LOGE("failed to init fd");
        fd_close(fd_handle);
    } else {
        *out_class_handle = (cmr_handle)fd_handle;
    }

    return ret;

free_fd_handle:
    if (fd_handle->alloc_addr) {
        free(fd_handle->alloc_addr);
        fd_handle->alloc_addr = NULL;
    }
    if (fd_handle->alloc_addr_u) {
        free(fd_handle->alloc_addr_u);
        fd_handle->alloc_addr_u = NULL;
    }
    free(fd_handle);
    fd_handle = NULL;
    return ret;
}

static cmr_int fd_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_fd *fd_handle = (struct class_fd *)class_handle;

    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(fd_handle);

    message.msg_type = CMR_EVT_FD_EXIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(fd_handle->thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg fail");
        goto out;
    }

    if (fd_handle->thread_handle) {
        if (cmr_thread_destroy(fd_handle->thread_handle) == 0){
            fd_handle->thread_handle = 0;
            fd_handle->is_inited = 0;
        }
        sem_destroy(&fd_handle->sem_facearea);
    }

#ifdef CONFIG_SPRD_FD_HW_SUPPORT
    struct camera_context *cam_cxt = NULL;
    cmr_handle oem_handle = NULL;
    oem_handle = fd_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if (cam_cxt->hal_free == NULL) {
        CMR_LOGE("cam_cxt->hal_free is NULL");
        goto out;
    }
    ret =
        cam_cxt->hal_free(CAMERA_FD_SMALL, &fd_handle->fd_small.addr_phy.addr_y,
                          &fd_handle->fd_small.addr_vir.addr_y,
                          &fd_handle->fd_small.fd, 1, cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to free the fd_small image buffers");
        goto out;
    }
    CMR_LOGD("Ok to free the fd_small image buffers");
#endif

out:
    if (fd_handle->alloc_addr) {
        free(fd_handle->alloc_addr);
        fd_handle->alloc_addr = NULL;
    }

    if (fd_handle->alloc_addr_u) {
        free(fd_handle->alloc_addr_u);
        fd_handle->alloc_addr_u = NULL;
    }

    if (fd_handle) {
        free(fd_handle);
        fd_handle = NULL;
    }

    return ret;
}

/*
 * in->private is a (struct fd_auxiliary_data) variable
 * while out->private is a camera_id converted to (void *)
 */
static cmr_int fd_transfer_frame(cmr_handle class_handle,
                                 struct ipm_frame_in *in,
                                 struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_fd *fd_handle = (struct class_fd *)class_handle;
    struct fd_auxiliary_data *auxiliary = NULL;
    cmr_uint frame_cnt;
    cmr_u32 is_busy = 0;
    struct fd_start_parameter param;

    frame_cnt = ++fd_handle->frame_cnt;
    auxiliary = (struct fd_auxiliary_data *)in->private_data;

    if (frame_cnt < fd_handle->frame_total_num) {
        CMR_LOGD("This is fd 0x%ld frame. need the 0x%ld frame,", frame_cnt,
                 fd_handle->frame_total_num);
        return ret;
    }

    fd_handle->curr_frame_idx++;

    fd_handle->frame_in.touch_x = in->touch_x;
    fd_handle->frame_in.touch_y = in->touch_y;
    fd_handle->frame_in.face_attribute_on = in->face_attribute_on;
    fd_handle->frame_in.smile_capture_on = in->smile_capture_on;

    // reduce the frame rate, because the current face detection (tracking mode)
    // is too fast!!
    /*{
        const static cmr_uint DROP_RATE = 2;
        if ((fd_handle->curr_frame_idx % DROP_RATE) != 0) {
            return ret;
        }
    }*/

    is_busy = fd_is_busy(fd_handle);
    // CMR_LOGI("fd is_busy =%d", is_busy);
    if (!is_busy) {
        fd_handle->frame_cnt = 0;
        fd_handle->frame_in = *in;
        /* don't hold pointer to variable in caller stack */
        fd_handle->frame_in.private_data = NULL;
        if (in->dst_frame.reserved) {
            memcpy((void *)&fd_handle->trans_frm, in->dst_frame.reserved,
                   sizeof(struct frm_info));
            fd_handle->frame_in.dst_frame.reserved =
                (void *)&fd_handle->trans_frm;
        }
        param.frame_data = (void *)in->src_frame.addr_vir.addr_y;
        param.frame_cb = fd_handle->frame_cb;
        param.caller_handle = in->caller_handle;

        if (auxiliary) {
            param.orientation = auxiliary->orientation;
            param.bright_value = auxiliary->bright_value;
            param.ae_stable = auxiliary->ae_stable;
            param.backlight_pro = auxiliary->backlight_pro;
            memcpy(param.hist, auxiliary->hist, sizeof(param.hist));
            param.zoom_ratio = auxiliary->zoom_ratio;
            param.frame_id = fd_handle->curr_frame_idx;
            /* callback needs this */
            param.private_data = (void *)((unsigned long)auxiliary->camera_id);
        } else {
            param.orientation = (cmr_u32)(-1);
            param.bright_value = (cmr_u32)(-1);
            param.ae_stable = 0;
            param.backlight_pro = (cmr_u32)(-1);
            memset(param.hist, 0, sizeof(param.hist));
            param.zoom_ratio = (cmr_u32)(-1);
            param.frame_id = (cmr_u32)(-1);
            /* callback needs this */
            param.private_data = NULL;
        }

        memcpy(fd_handle->alloc_addr, (void *)in->src_frame.addr_vir.addr_y,
               fd_handle->mem_size);
        memcpy(fd_handle->alloc_addr_u, (void *)in->src_frame.addr_vir.addr_u,
               fd_handle->mem_size / 2);

        ret = fd_start(class_handle, &param);
        if (ret) {
            CMR_LOGE("send msg fail");
            goto out;
        }

        if (fd_handle->frame_cb) {
            if (out != NULL) {
                cmr_bzero(out, sizeof(struct ipm_frame_out));
            }
        } else {
            if (out != NULL) {
                out = &fd_handle->frame_out;
            } else {
                CMR_LOGE("sync err,out parm can't NULL.");
            }
        }
    } else if (!fd_handle->is_get_result) {
        /*!!Warning: The following codes are not thread-safe */
        sem_wait(&fd_handle->sem_facearea);
        memcpy(&fd_handle->frame_out.face_area, &fd_handle->face_area_prev,
               sizeof(struct img_face_area));
        sem_post(&fd_handle->sem_facearea);
        fd_handle->frame_out.dst_frame.size.width =
            fd_handle->frame_in.src_frame.size.width;
        fd_handle->frame_out.dst_frame.size.height =
            fd_handle->frame_in.src_frame.size.height;
        fd_handle->frame_out.dst_frame.reserved =
            fd_handle->frame_in.dst_frame.reserved;

        /*callback*/
        if (fd_handle->frame_cb) {
            if (auxiliary) {
                /* callback needs this */
                fd_handle->frame_out.private_data =
                    (void *)((unsigned long)auxiliary->camera_id);
            } else {
                fd_handle->frame_out.private_data = NULL;
            }
            fd_handle->frame_out.caller_handle = in->caller_handle;
            fd_handle->frame_out.is_plus = 0;
            fd_handle->frame_cb(IPM_TYPE_FD, &fd_handle->frame_out);
        }
        if (fd_handle->frame_cb) {
            if (out != NULL) {
                cmr_bzero(out, sizeof(struct ipm_frame_out));
            }
        } else {
            if (out != NULL) {
                out = &fd_handle->frame_out;
            } else {
                CMR_LOGE("sync err,out parm can't NULL.");
            }
        }
    }

out:
    return ret;
}

static cmr_int fd_pre_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/
    (void)class_handle;

    return ret;
}

static cmr_int fd_post_proc(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    /*no need to do*/
    (void)class_handle;

    return ret;
}

static cmr_int fd_start(cmr_handle class_handle,
                        struct fd_start_parameter *param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_fd *fd_handle = (struct class_fd *)class_handle;
    cmr_u32 is_busy = 0;
    CMR_MSG_INIT(message);

    if (!class_handle || !param) {
        CMR_LOGE("parameter is NULL. fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!param->frame_data) {
        CMR_LOGE("frame_data is NULL. fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    message.data = (void *)malloc(sizeof(struct fd_start_parameter));
    if (NULL == message.data) {
        CMR_LOGE("NO mem, Fail to alloc memory for msg data");
        return CMR_CAMERA_NO_MEM;
    }

    memcpy(message.data, param, sizeof(struct fd_start_parameter));

    message.msg_type = CMR_EVT_FD_START;
    message.alloc_flag = 1;

    if (fd_handle->frame_cb) {
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        // message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    } else {
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    }

    ret = cmr_thread_msg_send(fd_handle->thread_handle, &message);
    if (ret) {
	free(message.data);
	message.data = NULL;
        CMR_LOGE("send msg fail");
        ret = CMR_CAMERA_FAIL;
    }

    return ret;
}

static cmr_uint check_size_data_invalid(struct img_size *fd_img_size) {
    cmr_int ret = -CMR_CAMERA_FAIL;

    if (NULL != fd_img_size) {
        if ((fd_img_size->width) && (fd_img_size->height)) {
            ret = CMR_CAMERA_SUCCESS;
        }
    }

    return ret;
}

static cmr_int fd_call_init(struct class_fd *class_handle,
                            const struct img_size *fd_img_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    message.data = malloc(sizeof(struct img_size));
    if (NULL == message.data) {
        CMR_LOGE("NO mem, Fail to alloc memory for msg data");
        ret = CMR_CAMERA_NO_MEM;
        goto out;
    }

    message.alloc_flag = 1;
    memcpy(message.data, fd_img_size, sizeof(struct img_size));

    message.msg_type = CMR_EVT_FD_INIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(class_handle->thread_handle, &message);
    if (CMR_CAMERA_SUCCESS != ret) {
        CMR_LOGE("msg send fail");
        ret = CMR_CAMERA_FAIL;
        goto free_all;
    }

    return ret;

free_all:
    free(message.data);
    message.data = NULL;
out:
    return ret;
}

static cmr_uint fd_is_busy(struct class_fd *class_handle) {
    cmr_int is_busy = 0;

    if (NULL == class_handle) {
        return is_busy;
    }

    is_busy = class_handle->is_busy;

    return is_busy;
}

static void fd_set_busy(struct class_fd *class_handle, cmr_uint is_busy) {
    if (NULL == class_handle) {
        return;
    }

    class_handle->is_busy = is_busy;
}

static cmr_int fd_thread_create(struct class_fd *class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(class_handle);

    if (!class_handle->is_inited) {
        ret = cmr_thread_create(&class_handle->thread_handle,
                                CAMERA_FD_MSG_QUEUE_SIZE, fd_thread_proc,
                                (void *)class_handle, "fd_oem");
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }

        class_handle->is_inited = 1;
    } else {
        CMR_LOGI("fd is inited already");
    }

end:
    return ret;
}

static void fd_recognize_face_attribute(FD_HANDLE hDT,
                                        FA_ALIGN_HANDLE hFaceAlign,
                                        FAR_RECOGNIZER_HANDLE hFAR,
                                        FAR_HANDLE hFAR_v2,
                                        struct class_fd *class_handle) {
    cmr_int face_count = 0;
    cmr_int fd_idx = 0;
    cmr_int i = 0;
    cmr_int ret = 0;
    struct class_faceattr_array new_attr_array;
    FA_IMAGE img;
    FAR_IMAGE_YUV420SP img_420sp;
    FA_SHAPE fattr_shape;
    FAR_FACEINFO_V2 faface_v2;

    struct class_faceattr_array *io_faceattr_arr;
    cmr_u8 *i_image_data;
    cmr_u8 *i_image_data_u;
    struct img_size i_image_size;
    cmr_uint i_curr_frame_idx;
    struct ipm_frame_in *frame_in_touch;

#ifdef CONFIG_SPRD_FD_HW_SUPPORT
    i_image_data = (void *)class_handle->fd_small.addr_vir.addr_y;
    i_image_data_u = (void *)class_handle->fd_small.addr_vir.addr_u;
    i_image_size = class_handle->fd_small.size;
    frame_in_touch = &(class_handle->frame_in);
    CMR_LOGV("get touch point before scale:(%d, %d)", frame_in_touch->touch_x, frame_in_touch->touch_y);
    frame_in_touch->touch_x = frame_in_touch->touch_x * class_handle->fd_small.size.width/class_handle->fd_img_size.width;
    frame_in_touch->touch_y = frame_in_touch->touch_y * class_handle->fd_small.size.height/class_handle->fd_img_size.height;
    CMR_LOGV("get touch point after scale:(%d, %d)", frame_in_touch->touch_x, frame_in_touch->touch_y);
#else
    i_image_data = class_handle->alloc_addr;
    i_image_data_u = class_handle->alloc_addr_u;
    i_image_size = class_handle->fd_img_size;
    frame_in_touch = &(class_handle->frame_in);
#endif

    io_faceattr_arr = &(class_handle->faceattr_arr);
    i_curr_frame_idx = class_handle->curr_frame_idx;
    frame_in_touch = &(class_handle->frame_in);

    face_count = FdGetFaceCount(hDT);
    if (face_count <= 0) {
        if (sprd_fd_api == SPRD_API_MODE_V2) {
            ret = FarResetFaceArray(hFAR_v2);
        }
        return;
    }

    FAR_ATTRIBUTE_V2 faceAtt[face_count];
    FAR_ATTRIBUTE_VEC fattr_v2_vec;
    FAR_FACEINFO_V2 faceInfo[face_count];
    FAR_FACEINFO_VEC farface_v2_vec;
    FAR_TOUCH_POINT touchPoint;
    /* Don't update face attribute, if the frame interval is not enough. For
     * reducing computation cost */
    if (sprd_fd_api == SPRD_API_MODE) {
        if ((io_faceattr_arr->count > 0) &&
            (i_curr_frame_idx - io_faceattr_arr->frame_idx) <
                FD_RUN_FAR_INTERVAL) {
            return;
        }
    }

    cmr_bzero(&new_attr_array, sizeof(struct class_faceattr_array));
    cmr_bzero(&img, sizeof(FA_IMAGE));
    cmr_bzero(&img_420sp, sizeof(FAR_IMAGE_YUV420SP));
    cmr_bzero(faceAtt, sizeof(FAR_ATTRIBUTE_V2) * face_count);
    cmr_bzero(faceInfo, sizeof(FAR_FACEINFO_V2) * face_count);
    cmr_bzero(&touchPoint, sizeof(FAR_TOUCH_POINT));

    img.data = (unsigned char *)i_image_data;
    img.width = i_image_size.width;
    img.height = i_image_size.height;
    img.step = img.width;
    if (class_handle->fd_x == frame_in_touch->touch_x &&
        class_handle->fd_y == frame_in_touch->touch_y) {
        touchPoint.x = 0;
        touchPoint.y = 0;
    } else {
        touchPoint.x = frame_in_touch->touch_x;
        touchPoint.y = frame_in_touch->touch_y;
        class_handle->fd_x = frame_in_touch->touch_x;
        class_handle->fd_y = frame_in_touch->touch_y;
    }

    // When there are many faces, process every face will be too slow.
    // Limit face count to 2
    if (sprd_fd_api == SPRD_API_MODE)
        face_count = MIN(face_count, 2);
    if (sprd_fd_api == SPRD_API_MODE_V2) {
        fattr_v2_vec.faceAtt = faceAtt;
        farface_v2_vec.faceInfo = faceInfo;
    }

    for (fd_idx = 0; fd_idx < face_count; fd_idx++) {
        struct class_faceattr *fattr = &(new_attr_array.face[fd_idx]);

        FD_FACEINFO info;
        FdGetFaceInfo(hDT, fd_idx, &info);

        /* Assign the same face id with FD */
        fattr->face_id = info.hid;
        fattr->attr.smile = 0;
        fattr->attr.eyeClose = 0;

        /* Run face alignment */
        {
            FA_FACEINFO faface;
            faface.x = info.x;
            faface.y = info.y;
            faface.width = info.width;
            faface.height = info.height;
            faface.yawAngle = info.yawAngle;
            faface.rollAngle = info.rollAngle;
            ret = FaFaceAlign(hFaceAlign, &img, &faface, &(fattr->shape));
        }

        for (i = 0; i < 7; i++) {
            fattr_shape.data[i * 2] = fattr->shape.data[i * 2];
            fattr_shape.data[i * 2 + 1] = fattr->shape.data[i * 2 + 1];
        }
        fattr_shape.score = fattr->shape.score;

        /* Run face attribute recognition */
        {
            FAR_OPTION opt;
            FAR_FACEINFO farface;

            /* set option: only do smile detection */
            opt.smileOn = 1;
            opt.eyeOn = 0;
            opt.infantOn = 0;
            opt.genderOn = 1;

            {
                if (sprd_fd_api == SPRD_API_MODE) {
                    for (i = 0; i < 7; i++) {
                        farface.landmarks[i].x = fattr->shape.data[i * 2];
                        farface.landmarks[i].y = fattr->shape.data[i * 2 + 1];
                    }
                    ret = FarRecognize(hFAR, (const FAR_IMAGE *)&img, &farface,
                                       &opt, &(fattr->attr));
                } else if (sprd_fd_api == SPRD_API_MODE_V2) {
                    struct class_faceattr_v2 *fattr =
                        &(new_attr_array.face_v2[fd_idx]);
                    fattr->attr_v2.smile = 0;
                    fattr->attr_v2.eyeClose = 0;
                    fattr->attr_v2.infant = 0;
                    fattr->attr_v2.genderPre = 0;
                    fattr->attr_v2.agePre = 0;
                    fattr->attr_v2.genderCnn = 0;
                    fattr->attr_v2.race = 0;
                    fattr->attr_v2.raceScore[0] = 0;
                    fattr->attr_v2.raceScore[1] = 0;
                    fattr->attr_v2.raceScore[2] = 0;
                    fattr->attr_v2.raceScore[3] = 0;
                    fattr->attr_v2.faceIdx = info.hid;
                    fattr->face_id = info.hid;
                    faface_v2.x = info.x;
                    faface_v2.y = info.y;
                    faface_v2.width = info.width;
                    faface_v2.height = info.height;
                    faface_v2.yawAngle = info.yawAngle;
                    faface_v2.rollAngle = info.rollAngle;
                    faface_v2.faceIdx = info.hid;
                    for (i = 0; i < 7; i++) {
                        fattr->shape.data[i * 2] = fattr_shape.data[i * 2];
                        fattr->shape.data[i * 2 + 1] =
                            fattr_shape.data[i * 2 + 1];
                    }
                    fattr->shape.score = fattr_shape.score;
                    farface_v2_vec.faceNum = face_count;
                    memcpy(&faceInfo[fd_idx], &faface_v2,
                           sizeof(FAR_FACEINFO_V2));
                    fattr_v2_vec.faceNum = face_count;
                    memcpy(&faceAtt[fd_idx], &fattr->attr_v2,
                           sizeof(FAR_ATTRIBUTE_V2));
                    if (fd_idx == (face_count - 1)) {
                        img_420sp.yData = img.data;
                        img_420sp.uvData = (unsigned char *)i_image_data_u;
                        img_420sp.width = img.width;
                        img_420sp.height = img.height;
                        img_420sp.format = YUV420_FORMAT_CRCB;
                        if (touchPoint.x != 0 || touchPoint.y != 0) {
                            CMR_LOGD("TOUCHPOINT %d %d ", touchPoint.x,
                                     touchPoint.y);
                            ret = FarSelectFace(
                                hFAR_v2, (const FAR_TOUCH_POINT *)&touchPoint,
                                (const FAR_FACEINFO_VEC *)&farface_v2_vec);
                        }
                        ret = FarRun_YUV420SP(
                            hFAR_v2, (const FAR_IMAGE_YUV420SP *)&img_420sp,
                            (const FAR_FACEINFO_VEC *)&farface_v2_vec,
                            &fattr_v2_vec);
                        for (fd_idx = 0; fd_idx < face_count; fd_idx++) {
                            CMR_LOGD("face num %d", fattr_v2_vec.faceNum);
                            for (int i = 0; i < fattr_v2_vec.faceNum; i++) {
                                if (new_attr_array.face_v2[fd_idx].face_id ==
                                    faceAtt[i].faceIdx) {
                                    memcpy(&(new_attr_array.face_v2[fd_idx]
                                                 .attr_v2),
                                           &faceAtt[i],
                                           sizeof(FAR_ATTRIBUTE_V2));
                                    break;
                                }
                            }
                            CMR_LOGV(
                                "face num %d fattr_v2_vec smile%d eyeClose%d infant%d genderPre%d genderCnn%d race%d"
                                "raceScore0=%d raceScore1=%d raceScore2=%d raceScore3=%d faceIdx=%d",
                                fattr_v2_vec.faceNum,
                                new_attr_array.face_v2[fd_idx].attr_v2.smile,
                                new_attr_array.face_v2[fd_idx].attr_v2.eyeClose,
                                new_attr_array.face_v2[fd_idx].attr_v2.infant,
                                new_attr_array.face_v2[fd_idx].attr_v2.genderPre,
                                new_attr_array.face_v2[fd_idx].attr_v2.genderCnn,
                                new_attr_array.face_v2[fd_idx].attr_v2.race,
                                new_attr_array.face_v2[fd_idx].attr_v2.raceScore[0],
                                new_attr_array.face_v2[fd_idx].attr_v2.raceScore[1],
                                new_attr_array.face_v2[fd_idx].attr_v2.raceScore[2],
                                new_attr_array.face_v2[fd_idx].attr_v2.raceScore[3],
                                new_attr_array.face_v2[fd_idx].attr_v2.faceIdx);
                        }
                    }
                }
            }
        }
    }

    new_attr_array.count = face_count;
    new_attr_array.frame_idx = i_curr_frame_idx;
    memcpy(io_faceattr_arr, &new_attr_array,
           sizeof(struct class_faceattr_array));
}

static cmr_int fd_get_face_overlap(const struct face_finder_data *i_face1,
                                   const struct face_finder_data *i_face2) {
    cmr_int percent = 0;

    /* get the overlapped region */
    cmr_int sx = MAX(i_face1->sx, i_face2->sx);
    cmr_int ex = MIN(i_face1->ex, i_face2->ex);
    cmr_int sy = MAX(i_face1->sy, i_face2->sy);
    cmr_int ey = MIN(i_face1->ey, i_face2->ey);

    if (ex >= sx && ey >= sy) {
        cmr_int overlap_area = (ex - sx + 1) * (ey - sy + 1);
        cmr_int area1 =
            (i_face1->ex - i_face1->sx + 1) * (i_face1->ey - i_face1->sy + 1);
        cmr_int area2 =
            (i_face2->ex - i_face2->sx + 1) * (i_face2->ey - i_face2->sy + 1);
        percent = (100 * overlap_area * 2) / (area1 + area2);
    }

    return percent;
}

static void
fd_smooth_face_rect(const struct img_face_area *i_face_area_prev,
                    const struct class_faceattr_array *i_faceattr_arr,
                    struct face_finder_data *io_curr_face) {
    cmr_int overlap_thr = 0;
    cmr_uint trust_curr_face = 0;
    cmr_uint prevIdx = 0;
    FA_SHAPE fattr_shape;
    cmr_bzero(&fattr_shape, sizeof(FA_SHAPE));

    // Try to correct the face rectangle by the face shape which is often more
    // accurate
    /*if (i_faceattr_arr != NULL) {
        // find the face shape with the same face ID
        const cmr_int shape_score_thr = 200;
        cmr_int i = 0;
        if (sprd_fd_api == SPRD_API_MODE) {
            const struct class_faceattr *fattr = NULL;
            for (i = 0; i < i_faceattr_arr->count; i++)
                if (i_faceattr_arr->face[i].face_id == io_curr_face->face_id) {
                    fattr = &(i_faceattr_arr->face[i]);
                    break;
                }
            if (fattr != NULL)
                fattr_shape = fattr->shape;
        } else if (sprd_fd_api == SPRD_API_MODE_V2) {
            const struct class_faceattr_v2 *fattr = NULL;
            for (i = 0; i < i_faceattr_arr->count; i++)
                if (i_faceattr_arr->face_v2[i].face_id ==
                    io_curr_face->face_id) {
                    fattr = &(i_faceattr_arr->face_v2[i]);
                    break;
                }
            if (fattr != NULL)
                fattr_shape = fattr->shape;
        }

        // Calculate the new face rectangle according to the face shape
        if (fattr_shape.score >= shape_score_thr) {
            const int *sdata = (const int *)(fattr_shape.data);
            cmr_int eye_cx = (sdata[0] + sdata[2] + sdata[4] + sdata[6]) / 4;
            cmr_int eye_cy = (sdata[1] + sdata[3] + sdata[5] + sdata[7]) / 4;
            cmr_int mouth_cx = (sdata[10] + sdata[12]) / 2;
            cmr_int mouth_cy = (sdata[11] + sdata[13]) / 2;
            cmr_int cx = (eye_cx + mouth_cx) / 2;
            cmr_int cy = (eye_cy + mouth_cy) / 2;
            cmr_int curr_cx = (io_curr_face->sx + io_curr_face->ex) / 2;
            cmr_int curr_cy = (io_curr_face->sy + io_curr_face->ey) / 2;
            cmr_int x_shift = cx - curr_cx;
            cmr_int y_shift = cy - curr_cy;
            cmr_int max_shift = (io_curr_face->ex - io_curr_face->sx) / 4;

            // If the shift calculted by face shape is not too large,
            // we're very sure the face shape is correct.
            if ((ABS(x_shift) + ABS(y_shift)) < max_shift) {
                // only correct the face center; size is not changed.
                io_curr_face->sx += x_shift;
                io_curr_face->sy += y_shift;
                io_curr_face->srx += x_shift;
                io_curr_face->sry += y_shift;
                io_curr_face->elx += x_shift;
                io_curr_face->ely += y_shift;
                io_curr_face->ex += x_shift;
                io_curr_face->ey += y_shift;

                trust_curr_face = 1;
            }
        }
    }*/
    //fd_ptr save fd result to isp
    io_curr_face->fd_ptr.sx = io_curr_face->sx;
    io_curr_face->fd_ptr.sy = io_curr_face->sy;
    io_curr_face->fd_ptr.srx = io_curr_face->srx;
    io_curr_face->fd_ptr.sry = io_curr_face->sry;
    io_curr_face->fd_ptr.elx = io_curr_face->elx;
    io_curr_face->fd_ptr.ely = io_curr_face->ely;
    io_curr_face->fd_ptr.ex = io_curr_face->ex;
    io_curr_face->fd_ptr.ey = io_curr_face->ey;
    // Try to keep the face rectangle to be the same with the previous
    // frame (for stable looks)
    overlap_thr = 90;
    for (prevIdx = 0; prevIdx < i_face_area_prev->face_count; prevIdx++) {
        const struct face_finder_data *prev_face =
            &(i_face_area_prev->range[prevIdx]);
        cmr_int overlap_percent = fd_get_face_overlap(prev_face, io_curr_face);

        if (overlap_percent >= overlap_thr) {
            io_curr_face->sx = prev_face->sx;
            io_curr_face->sy = prev_face->sy;
            io_curr_face->srx = prev_face->srx;
            io_curr_face->sry = prev_face->sry;
            io_curr_face->elx = prev_face->elx;
            io_curr_face->ely = prev_face->ely;
            io_curr_face->ex = prev_face->ex;
            io_curr_face->ey = prev_face->ey;
            break;
        }
    }
}

static void fd_get_fd_results(FD_HANDLE hDT,
                              const struct class_faceattr_array *i_faceattr_arr,
                              const struct img_face_area *i_face_area_prev,
                              struct img_face_area *o_face_area,
                              struct img_size image_size,
                              const cmr_uint i_curr_frame_idx) {
    cmr_int face_num = 0;
    FD_FACEINFO info;
    cmr_int face_idx = 0;
    cmr_int ret = FD_OK;
    cmr_int sx = 0, ex = 0, sy = 0, ey = 0;
    cmr_int valid_count = 0;
    struct face_finder_data *face_ptr = NULL;
    const struct class_faceattr_array *curr_faceattr =
        (i_curr_frame_idx == i_faceattr_arr->frame_idx) ? i_faceattr_arr : NULL;

    face_num = FdGetFaceCount(hDT);
    for (face_idx = 0; face_idx < face_num; face_idx++) {
        /* Gets the detection result for each face */
        ret = FdGetFaceInfo(hDT, face_idx, &info);
        if (ret != FD_OK) {
            CMR_LOGW("FdGetFaceInfo(%ld) Error : %ld", face_idx, ret);
            continue;
        }

        sx = info.x;
        sy = info.y;
        ex = info.x + info.width - 1;
        ey = info.y + info.height - 1;

        /* enlarge face size a little.
        !TODO: maybe it is also necessary to adjust the face center
        */
        /*
        {
            cmr_int delta = info.width / 20;
            sx -= delta;
            sy -= delta;
            ex += delta;
            ey += delta;
        }
        */
        face_ptr = &(o_face_area->range[valid_count]);
        valid_count++;

        face_ptr->sx = sx;
        face_ptr->sy = sy;
        face_ptr->srx = ex;
        face_ptr->sry = sy;
        face_ptr->elx = sx;
        face_ptr->ely = ey;
        face_ptr->ex = ex;
        face_ptr->ey = ey;
        face_ptr->face_id = info.hid;
        face_ptr->pose = info.yawAngle;
        face_ptr->angle = info.rollAngle;
        /* Make it in [0,100]. HAL1.0 requires so */
        face_ptr->score = info.score / 10;
        face_ptr->smile_level = 1;
        face_ptr->blink_level = 0;
        face_ptr->brightness = 128;
        CMR_LOGV("fd result %d %d %d %d", sx, sy, ex,ey);
        fd_smooth_face_rect(i_face_area_prev, curr_faceattr, face_ptr);

        /* Ensure the face coordinates are in image region */
        if (face_ptr->sx < 0)
            face_ptr->sx = face_ptr->elx = 0;
        if (face_ptr->sy < 0)
            face_ptr->sy = face_ptr->sry = 0;
        if (face_ptr->ex >= (cmr_int)image_size.width)
            face_ptr->ex = face_ptr->srx = image_size.width - 1;
        if (face_ptr->ey >= (cmr_int)image_size.height)
            face_ptr->ey = face_ptr->ely = image_size.height - 1;

        /* set smile detection result */
        {
            const cmr_int app_smile_thr = 30;  // smile threshold in APP
            const cmr_int algo_smile_thr = 10; // smile threshold by algorithm;
            // it is a tuning parameter, must be in [1, 50]
            cmr_int i = 0;
            cmr_int gender_age = 0;
            for (i = 0; i < i_faceattr_arr->count; i++) {
                if (sprd_fd_api == SPRD_API_MODE) {
                    const struct class_faceattr *fattr =
                        &(i_faceattr_arr->face[i]);
                    if (fattr->face_id == info.hid) {
                        /* Note: The original smile score is in [-100, 100].
                            But the Camera APP needs a score in [0, 100],
                            and also the definitions for smile degree are
                           different
                            with the algorithm.
                            So, we need to adjust the smile score to fit the
                           APP.
                        */
                        cmr_int smile_score = MAX(0, fattr->attr.smile);
                        if (smile_score >= algo_smile_thr) {
                            /* norm_score is in [0, 70] */
                            cmr_int norm_score =
                                ((smile_score - algo_smile_thr) *
                                 (100 - app_smile_thr)) /
                                (100 - algo_smile_thr);
                            /* scale the smile score to be in [30, 100] */
                            smile_score = norm_score + app_smile_thr;
                        } else {
                            /* scale the smile score to be in [0, 30) */
                            smile_score =
                                (smile_score * app_smile_thr) / algo_smile_thr;
                        }

                        face_ptr->smile_level = MAX(1, smile_score);
                        face_ptr->blink_level = MAX(0, fattr->attr.eyeClose);
                    }
                } else if (sprd_fd_api == SPRD_API_MODE_V2) {
                    const struct class_faceattr_v2 *fattr =
                        &(i_faceattr_arr->face_v2[i]);
                    if (fattr->face_id == info.hid) {
                        /* Note: The original smile score is in [-100, 100].
                            But the Camera APP needs a score in [0, 100],
                            and also the definitions for smile degree are
                           different
                            with the algorithm.
                            So, we need to adjust the smile score to fit the
                           APP.
                        */
                        cmr_int smile_score = MAX(0, fattr->attr_v2.smile);
                        if (smile_score >= algo_smile_thr) {
                            /* norm_score is in [0, 70] */
                            cmr_int norm_score =
                                ((smile_score - algo_smile_thr) *
                                 (100 - app_smile_thr)) /
                                (100 - algo_smile_thr);
                            /* scale the smile score to be in [30, 100] */
                            smile_score = norm_score + app_smile_thr;
                        } else {
                            /* scale the smile score to be in [0, 30) */
                            smile_score =
                                (smile_score * app_smile_thr) / algo_smile_thr;
                        }
                        face_ptr->smile_level = MAX(1, smile_score);
                        face_ptr->blink_level = MAX(0, fattr->attr_v2.eyeClose);
                        /*The first digit of gender_age represents gender:
                        * Male(1);Female(2);unknown(0)*/
                        if (fattr->attr_v2.genderCnn == 0 &&
                            fattr->attr_v2.race == 0 &&
                            fattr->attr_v2.agePre == 0)
                            face_ptr->gender_age_race = 0;
                        else {
                            gender_age =
                                fattr->attr_v2.genderCnn > 0
                                    ? fattr->attr_v2.agePre + 100
                                    : ((fattr->attr_v2.genderCnn < 0)
                                           ? fattr->attr_v2.agePre + 200
                                           : fattr->attr_v2.agePre + 0);
                            switch (fattr->attr_v2.race) {
                            case 0:
                                face_ptr->gender_age_race = gender_age + 1000;
                                break;
                            case 1:
                                face_ptr->gender_age_race = gender_age + 2000;
                                break;
                            case 2:
                                face_ptr->gender_age_race = gender_age + 3000;
                                break;
                            case 3:
                                face_ptr->gender_age_race = gender_age + 4000;
                                break;
                            default:
                                face_ptr->gender_age_race = gender_age;
                                break;
                            }
                            CMR_LOGV("gender_age_race %d",face_ptr->gender_age_race);
                        }
                        break;
                    }
                }
            }
        }
    }

    o_face_area->face_count = valid_count;
}

static cmr_int fd_create_detector(FD_HANDLE *hDT,
                                  const struct img_size *fd_img_size,
                                  cmr_int work_mode) {
    FD_OPTION opt;
    FD_VERSION version;
    FdGetVersion(&version);
#ifdef CONFIG_SPRD_FD_HW_SUPPORT
    opt.fdEnv = FD_ENV_HW;
#else
    opt.fdEnv = FD_ENV_SW;
#endif
    FdInitOption(&opt);
#ifdef PLATFORM_ID
    opt.platform = PLATFORM_ID;
#else
    opt.platform = PLATFORM_ID_GENERIC;
#endif
    opt.workMode = work_mode;
    opt.maxFaceNum = FACE_DETECT_NUM;
    opt.minFaceSize = MIN(fd_img_size->width, fd_img_size->height) / 12;
    opt.directions = FD_DIRECTION_ALL;
    opt.angleFrontal = FD_ANGLE_RANGE_90;
    opt.angleHalfProfile = FD_ANGLE_RANGE_30;
    opt.angleFullProfile = FD_ANGLE_RANGE_30; // FD_ANGLE_NONE; for Bug 636739;
    opt.detectDensity = 5;
    opt.scoreThreshold = 0;
    opt.detectInterval = 6;
    opt.trackDensity = 5;
    opt.lostRetryCount = 1;
    opt.lostHoldCount = 1;
    opt.holdPositionRate = 5;
    opt.holdSizeRate = 4;
    opt.swapFaceRate = 200;
    opt.guessFaceDirection = 1;

    CMR_LOGI("SPRD FD version: \"%s\", platform: 0x%04x", version.built_rev,
             opt.platform);

    /* For tuning FD parameter: read parameter from file */
    /*
    {
            FILE *fp = fopen("/data/sprd_fd_param.txt", "rt");
            if (!fp) {
                    CMR_LOGI("failed to open /data/sprd_fd_param.txt");
            } else {
                    CMR_LOGI("read /data/sprd_fd_param.txt");
            }

            if (fp) {
                    unsigned int v[17];
                    int i = 0;
                    for (i = 0; i < 17; i++){
                            fscanf(fp, "%d ", &(v[i]));
                            CMR_LOGI("sprd_fd_param: v[%d]=%d", i, v[i]);
                    }
                    fclose(fp);

                    opt.minFaceSize     = MIN(fd_img_size->width,
    fd_img_size->height) / v[0];
                    opt.directions      = v[1];
                    opt.angleFrontal    = v[2];
                    opt.angleHalfProfile= v[3];
                    opt.angleFullProfile= v[4];
                    opt.detectDensity   = v[5];
                    opt.scoreThreshold  = v[6];
                    opt.initFrames      = v[7];
                    opt.detectFrames    = v[8];
                    opt.detectInterval  = v[9];
                    opt.trackDensity    = v[10];
                    opt.lostRetryCount  = v[11];
                    opt.lostHoldCount   = v[12];
                    opt.holdPositionRate = v[13];
                    opt.holdSizeRate    = v[14];
                    opt.swapFaceRate    = v[15];
                    opt.guessFaceDirection = v[16];
            }
    }
    */
    return FdCreateDetector(hDT, &opt);
}

cmr_int fd_start_scale(cmr_handle oem_handle, struct img_frm *src,
                       struct img_frm *dst) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;

    if (!oem_handle || !src || !dst) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGD(
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
    CMR_LOGV("X, ret=%ld", ret);

    return ret;
}

static cmr_int fd_thread_proc(struct cmr_msg *message, void *private_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_fd *class_handle = (struct class_fd *)private_data;
    cmr_int evt = 0;
    struct fd_start_parameter *start_param = NULL;
    struct img_size *fd_img_size = NULL;
    FD_IMAGE fd_img;
    clock_t start_time, end_time;
    int duration;
    float ratio;
    cmr_u32 face_square = 0;
    int w = 0;
    int h = 0;
    cmr_u32 i = 0;
    cmr_bzero(&fd_img, sizeof(fd_img));
    if (!message || !class_handle) {
        CMR_LOGE("parameter is fail");
        return CMR_CAMERA_INVALID_PARAM;
    }

    evt = (cmr_u32)(message->msg_type & CMR__EVT_FD_MASK_BITS);

    switch (evt) {
    case CMR_EVT_FD_INIT:
        /* Create face alignment and face attribute recognition handle */
        if (FA_OK != FaCreateAlignHandle(&(class_handle->hFaceAlign))) {
            CMR_LOGE("FaCreateAlignHandle() Error");
            break;
        }

        if (sprd_fd_api == SPRD_API_MODE) {
            if (FAR_OK != FarCreateRecognizerHandle(&(class_handle->hFAR))) {
                CMR_LOGE("FarCreateRecognizerHandle() Error");
                break;
            }
        }

        /* Creates Face Detection handle */
        fd_img_size = (struct img_size *)message->data;
        ret = fd_create_detector(&(class_handle->hDT), fd_img_size,
                                 class_handle->work_mode);
        if (ret != FD_OK) {
            CMR_LOGE("fd_create_detector() Error: %d", ret);
            break;
        }

        break;

    case CMR_EVT_FD_START:
        start_param = (struct fd_start_parameter *)message->data;

        if (NULL == start_param) {
            CMR_LOGE("parameter fail");
            break;
        }

        fd_set_busy(class_handle, 1);

        fd_img.context.orientation = (int)start_param->orientation;
        fd_img.context.brightValue = (int)start_param->bright_value;
        fd_img.context.aeStable = !!start_param->ae_stable;
        fd_img.context.backlightPro = (unsigned int)start_param->backlight_pro;
        for (int i = 0; i < CAMERA_ISP_HIST_ITEMS; i++)
            fd_img.context.hist[i] = (unsigned int)start_param->hist[i];
        fd_img.context.zoomRatio = (int)start_param->zoom_ratio;
        fd_img.context.frameID = (int)start_param->frame_id;

        CMR_LOGD("orientation %d, brightValue %d, aeStable %d, backlightPro "
                 "%u, hist[0~3] %u %u %u %u, zoomRatio %d frameID %d",
                 fd_img.context.orientation, fd_img.context.brightValue,
                 fd_img.context.aeStable, fd_img.context.backlightPro,
                 fd_img.context.hist[0], fd_img.context.hist[1],
                 fd_img.context.hist[2], fd_img.context.hist[3],
                 fd_img.context.zoomRatio, fd_img.context.frameID);

#ifdef CONFIG_SPRD_FD_HW_SUPPORT
        class_handle->frame_in.src_frame.data_end.y_endian = 0;
        class_handle->frame_in.src_frame.data_end.uv_endian = 0;
        class_handle->frame_in.src_frame.rect.start_x = 0;
        class_handle->frame_in.src_frame.rect.start_y = 0;
        class_handle->frame_in.src_frame.rect.width =
            class_handle->frame_in.src_frame.size.width;
        class_handle->frame_in.src_frame.rect.height =
            class_handle->frame_in.src_frame.size.height;

        class_handle->frame_in.src_frame.addr_phy.addr_u =
            class_handle->frame_in.src_frame.addr_phy.addr_y +
            class_handle->frame_in.src_frame.size.width *
                class_handle->frame_in.src_frame.size.height;
        class_handle->frame_in.src_frame.addr_phy.addr_v =
            class_handle->frame_in.src_frame.addr_vir.addr_u;

        class_handle->frame_in.src_frame.addr_vir.addr_u =
            class_handle->frame_in.src_frame.addr_vir.addr_y +
            class_handle->frame_in.src_frame.size.width *
                class_handle->frame_in.src_frame.size.height;
        class_handle->frame_in.src_frame.addr_vir.addr_v =
            class_handle->frame_in.src_frame.addr_vir.addr_u;

        class_handle->fd_small.fmt = CAM_IMG_FMT_YUV420_NV21;
        class_handle->fd_small.data_end.y_endian = 0;
        class_handle->fd_small.data_end.uv_endian = 0;
        class_handle->fd_small.rect.start_x = 0;
        class_handle->fd_small.rect.start_y = 0;
        class_handle->fd_small.rect.width = class_handle->fd_small.size.width;
        class_handle->fd_small.rect.height = class_handle->fd_small.size.height;

        class_handle->fd_small.addr_phy.addr_u =
            class_handle->fd_small.addr_phy.addr_y +
            class_handle->fd_small.size.width *
                class_handle->fd_small.size.height;
        class_handle->fd_small.addr_phy.addr_v =
            class_handle->fd_small.addr_phy.addr_u;

        class_handle->fd_small.addr_vir.addr_u =
            class_handle->fd_small.addr_vir.addr_y +
            class_handle->fd_small.size.width *
                class_handle->fd_small.size.height;
        class_handle->fd_small.addr_vir.addr_v =
            class_handle->fd_small.addr_vir.addr_u;

        ret = fd_start_scale(class_handle->common.ipm_cxt->init_in.oem_handle,
                             &class_handle->frame_in.src_frame,
                             &class_handle->fd_small);
        if (ret) {
            CMR_LOGE("Fail to call fd_start_scale");
        }

        /* Executes Face Detection */
        fd_img.data = (unsigned char *)class_handle->fd_small.addr_vir.addr_y;
        fd_img.width = class_handle->fd_small.size.width;
        fd_img.height = class_handle->fd_small.size.height;
        fd_img.step = class_handle->fd_small.size.width;
        fd_img.data_handle = class_handle->fd_small.fd;

        start_time = clock();
        ret = FdDetectFace(class_handle->hDT, &fd_img);
        end_time = clock();

        if (ret != FD_OK) {
            CMR_LOGE("FdDetectFace() Error : %ld", ret);
            fd_set_busy(class_handle, 0);
            break;
        }

        fd_face_attribute_detect(class_handle);

        class_handle->is_get_result = 1;
        /* extract face detection results */
        fd_get_fd_results(class_handle->hDT, &(class_handle->faceattr_arr),
                          &(class_handle->face_small_area),
                          &(class_handle->frame_out.face_area),
                          class_handle->fd_small.size,
                          class_handle->curr_frame_idx);
        class_handle->frame_out.dst_frame.size.width =
            class_handle->frame_in.src_frame.size.width;
        class_handle->frame_out.dst_frame.size.height =
            class_handle->frame_in.src_frame.size.height;
        class_handle->frame_out.dst_frame.reserved =
            class_handle->frame_in.dst_frame.reserved;
        ratio = (float)class_handle->frame_in.src_frame.size.width /
                (float)class_handle->fd_small.size.width;
        /* save a copy face_small_area for next frame to smooth */
        memcpy(&(class_handle->face_small_area),
               &(class_handle->frame_out.face_area),
               sizeof(struct img_face_area));

        for (i = 0; i < class_handle->frame_out.face_area.face_count; i++) {
            //640*480 t0 prev plane
            class_handle->frame_out.face_area.range[i].fd_ptr.sx =
                class_handle->frame_out.face_area.range[i].fd_ptr.sx  * ratio;
            class_handle->frame_out.face_area.range[i].fd_ptr.sy =
                class_handle->frame_out.face_area.range[i].fd_ptr.sy * ratio;
            class_handle->frame_out.face_area.range[i].fd_ptr.srx =
                class_handle->frame_out.face_area.range[i].fd_ptr.srx * ratio;
            class_handle->frame_out.face_area.range[i].fd_ptr.sry =
                class_handle->frame_out.face_area.range[i].fd_ptr.sry * ratio;
            class_handle->frame_out.face_area.range[i].fd_ptr.ex =
                class_handle->frame_out.face_area.range[i].fd_ptr.ex * ratio;
            class_handle->frame_out.face_area.range[i].fd_ptr.ey =
                class_handle->frame_out.face_area.range[i].fd_ptr.ey * ratio;
            class_handle->frame_out.face_area.range[i].fd_ptr.elx =
                class_handle->frame_out.face_area.range[i].elx * ratio;
            class_handle->frame_out.face_area.range[i].fd_ptr.ely =
                class_handle->frame_out.face_area.range[i].fd_ptr.ely * ratio;
            /* coherence of coordinates */
            w = class_handle->frame_out.face_area.range[i].ex -
                class_handle->frame_out.face_area.range[i].sx;
            h = class_handle->frame_out.face_area.range[i].ey -
            class_handle->frame_out.face_area.range[i].sy;
            if (w == h)
                face_square = 1;
            class_handle->frame_out.face_area.range[i].sx =
                class_handle->frame_out.face_area.range[i].sx * ratio;
            class_handle->frame_out.face_area.range[i].sy =
                class_handle->frame_out.face_area.range[i].sy * ratio;
            class_handle->frame_out.face_area.range[i].srx =
                class_handle->frame_out.face_area.range[i].srx * ratio;
            class_handle->frame_out.face_area.range[i].sry =
                class_handle->frame_out.face_area.range[i].sry * ratio;
            class_handle->frame_out.face_area.range[i].ex =
                class_handle->frame_out.face_area.range[i].ex * ratio;
            class_handle->frame_out.face_area.range[i].ey =
                class_handle->frame_out.face_area.range[i].ey * ratio;
            class_handle->frame_out.face_area.range[i].elx =
                class_handle->frame_out.face_area.range[i].elx * ratio;
            class_handle->frame_out.face_area.range[i].ely =
                class_handle->frame_out.face_area.range[i].ely * ratio;
            w = class_handle->frame_out.face_area.range[i].ex -
                class_handle->frame_out.face_area.range[i].sx;
            h = class_handle->frame_out.face_area.range[i].ey -
                class_handle->frame_out.face_area.range[i].sy;
            if(w != h)
                CMR_LOGV("sx %d,ex %d, sy %d ey %d,srx %d,elx %d, sry %d ely %d",
                    class_handle->frame_out.face_area.range[i].sx,
                    class_handle->frame_out.face_area.range[i].ex,
                    class_handle->frame_out.face_area.range[i].sy,
                    class_handle->frame_out.face_area.range[i].ey,
                    class_handle->frame_out.face_area.range[i].srx,
                    class_handle->frame_out.face_area.range[i].elx,
                    class_handle->frame_out.face_area.range[i].sry,
                    class_handle->frame_out.face_area.range[i].ely);
            if (face_square == 1) {
                if(w > h){
                    class_handle->frame_out.face_area.range[i].ex =
                        class_handle->frame_out.face_area.range[i].sx + h;
                    class_handle->frame_out.face_area.range[i].srx =
                        class_handle->frame_out.face_area.range[i].elx + h;
               }else if (w < h) {
                    class_handle->frame_out.face_area.range[i].ey =
                        class_handle->frame_out.face_area.range[i].sy + w;
                    class_handle->frame_out.face_area.range[i].ely =
                        class_handle->frame_out.face_area.range[i].sry + w;
               }

               face_square = 0;
            }
        }

        /* save a copy for next frame to directly callback if busying */
        sem_wait(&class_handle->sem_facearea);
        memcpy(&(class_handle->face_area_prev),
               &(class_handle->frame_out.face_area),
               sizeof(struct img_face_area));
        sem_post(&class_handle->sem_facearea);

        duration = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;
        CMR_LOGD("%dx%d, face_num=%ld, time=%d ms",
                 class_handle->fd_small.size.width,
                 class_handle->fd_small.size.height,
                 class_handle->frame_out.face_area.face_count, duration);
#else
        /* Executes Face Detection */
        fd_img.data = (unsigned char *)class_handle->alloc_addr;
        fd_img.width = class_handle->fd_img_size.width;
        fd_img.height = class_handle->fd_img_size.height;
        fd_img.step = fd_img.width;
        fd_img.data_handle = 0;

        start_time = clock();
        ret = FdDetectFace(class_handle->hDT, &fd_img);
        end_time = clock();

        if (ret != FD_OK) {
            CMR_LOGE("FdDetectFace() Error : %ld", ret);
            fd_set_busy(class_handle, 0);
            break;
        }
        fd_face_attribute_detect(class_handle);

        class_handle->is_get_result = 1;
        /* extract face detection results */
        sem_wait(&class_handle->sem_facearea);
        fd_get_fd_results(class_handle->hDT, &(class_handle->faceattr_arr),
                          &(class_handle->face_area_prev),
                          &(class_handle->frame_out.face_area),
                          class_handle->fd_img_size,
                          class_handle->curr_frame_idx);
        /* save a copy for next frame to directly callback if busying */
        memcpy(&(class_handle->face_area_prev),
               &(class_handle->frame_out.face_area),
               sizeof(struct img_face_area));
        sem_post(&class_handle->sem_facearea);

        class_handle->frame_out.dst_frame.size.width =
            class_handle->frame_in.src_frame.size.width;
        class_handle->frame_out.dst_frame.size.height =
            class_handle->frame_in.src_frame.size.height;
        class_handle->frame_out.dst_frame.reserved =
            class_handle->frame_in.dst_frame.reserved;

        duration = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;
        CMR_LOGD("%dx%d, face_num=%ld, time=%d ms",
                 class_handle->frame_in.src_frame.size.width,
                 class_handle->frame_in.src_frame.size.height,
                 class_handle->frame_out.face_area.face_count, duration);
#endif

        /*callback*/
        if (class_handle->frame_cb) {
            class_handle->frame_out.private_data = start_param->private_data;
            class_handle->frame_out.caller_handle = start_param->caller_handle;
            class_handle->frame_out.is_plus = 1;
            class_handle->frame_cb(IPM_TYPE_FD, &class_handle->frame_out);
        }

        fd_set_busy(class_handle, 0);
        class_handle->is_get_result = 0;
        break;

    case CMR_EVT_FD_EXIT:
        /* Deletes Face Detection handle */
        FaDeleteAlignHandle(&(class_handle->hFaceAlign));
        if (sprd_fd_api == SPRD_API_MODE)
            FarDeleteRecognizerHandle(&(class_handle->hFAR));
        else if (sprd_fd_api == SPRD_API_MODE_V2 &&
                 class_handle->is_face_attribute_init == 1) {
            FarDeleteRecognizerHandle_V2(&(class_handle->hFAR_v2));
            class_handle->is_face_attribute_init = 0;
            CMR_LOGD("FarRecognizerHandle_V2: Delete");
        }
        FdDeleteDetector(&(class_handle->hDT));
        break;

    default:
        break;
    }

    return ret;
}

void fd_face_attribute_detect(struct class_fd *class_handle) {
    bool updateFlag = false;
    if ((class_handle->frame_in.face_attribute_on == 1 ||
            class_handle->frame_in.smile_capture_on == 1) &&
            (class_handle->is_face_attribute_init == 0)) {
        if (sprd_fd_api == SPRD_API_MODE_V2) {
            FAR_OPTION_V2 opt_v2;
            FAR_InitOption(&opt_v2);
            opt_v2.trackInterval = 16;
            int threadNum = FD_THREAD_NUM;
            opt_v2.workMode = FAR_WORKMODE_MOVIE;
            opt_v2.maxFaceNum = FD_MAX_CNN_FACE_NUM;
            /* set option: only do smile detection */
            if (FAR_OK !=
                FarCreateRecognizerHandle_V2(&(class_handle->hFAR_v2),
                                             threadNum, &opt_v2)) {
                CMR_LOGE("FarCreateRecognizerHandle_V2() Error");
            } else {
                class_handle->is_face_attribute_init = 1;
                CMR_LOGD("FarRecognizerHandle_V2: Create");
            }
        }
    }
    updateFlag = (class_handle->is_face_attribute != class_handle->frame_in.face_attribute_on)
        ||  (class_handle->is_smile_capture != class_handle->frame_in.smile_capture_on);
    if(updateFlag && sprd_fd_api == SPRD_API_MODE_V2) {
        CMR_LOGD("FarRecognizerHandle_V2: Update race %d, smile %d",
            class_handle->frame_in.face_attribute_on, class_handle->frame_in.smile_capture_on);
        FAR_OPTION_V2 opt_v2;
        FAR_InitOption(&opt_v2);
        if(class_handle->frame_in.smile_capture_on == 1)
            opt_v2.smileOn = 1;
        else opt_v2.smileOn = 0;
        if(class_handle->frame_in.face_attribute_on == 1) {
            opt_v2.genderCnnOn = 1;
            opt_v2.raceOn = 1;
        } else {
            opt_v2.genderCnnOn = 0;
            opt_v2.raceOn = 0;
        }
        FAR_updateOption(class_handle->hFAR_v2, &opt_v2);
        FarResetFaceArray(class_handle->hFAR_v2);
        class_handle->is_face_attribute = class_handle->frame_in.face_attribute_on;
        class_handle->is_smile_capture = class_handle->frame_in.smile_capture_on;
    }

    /* recognize face attribute (smile detection) */
    if(class_handle->frame_in.face_attribute_on == 1 ||
        class_handle->frame_in.smile_capture_on == 1 ||
        sprd_fd_api == SPRD_API_MODE)
        fd_recognize_face_attribute(class_handle->hDT, class_handle->hFaceAlign,
                                    class_handle->hFAR, class_handle->hFAR_v2,
                                    class_handle);

    if ((class_handle->frame_in.face_attribute_on == 0 &&
        class_handle->frame_in.smile_capture_on == 0) &&
        (class_handle->is_face_attribute_init == 1)) {
        if (sprd_fd_api == SPRD_API_MODE_V2) {
            FarDeleteRecognizerHandle_V2(&(class_handle->hFAR_v2));
            CMR_LOGD("FarRecognizerHandle_V2: Delete");
        }
        class_handle->is_face_attribute_init = 0;
    }
    class_handle->face_attributes_off =
            class_handle->frame_in.face_attribute_on;
}
#endif
