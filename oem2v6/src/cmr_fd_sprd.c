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
#include "cmr_oem.h"
#define FD_MAX_FACE_NUM 10
#define FD_RUN_FAR_INTERVAL                                                    \
    8 /* The frame interval to run FAR. For reducing computation cost */

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : -(x))
#endif

struct class_faceattr {
    FA_SHAPE shape;
    FAR_ATTRIBUTE attr;
    int face_id; /* face id gotten from face detection */
};

struct class_faceattr_array {
    int count;          /* face count      */
    cmr_uint frame_idx; /* The frame when the face attributes are updated */
    struct class_faceattr face[FD_MAX_FACE_NUM + 1]; /* face attricutes */
};

struct class_fd {
    struct ipm_common common;
    cmr_handle thread_handle;
    cmr_uint is_busy;
    cmr_uint is_inited;
    void *alloc_addr;
    cmr_uint mem_size;
    cmr_uint frame_cnt;
    cmr_uint frame_total_num;
    struct ipm_frame_in frame_in;
    struct ipm_frame_out frame_out;
    ipm_callback frame_cb;
    struct img_size fd_img_size;
    struct img_face_area
        face_area_prev; /* The faces detected from the previous frame;
                            It is used to directly callback if face detect is busying */
    struct img_face_area face_small_area;
    struct class_faceattr_array faceattr_arr; /* face attributes */
    cmr_uint curr_frame_idx;
    cmr_uint is_get_result;
    FD_DETECTOR_HANDLE hDT;     /* Face Detection Handle */
    FA_ALIGN_HANDLE hFaceAlign; /* Handle for face alignment */
    FAR_RECOGNIZER_HANDLE hFAR; /* Handle for face attribute recognition */
    struct img_frm fd_small;
};

struct fd_start_parameter {
    void *frame_data;
    ipm_callback frame_cb;
    cmr_handle caller_handle;
    void *private_data;
};

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
    struct camera_context *cam_cxt = NULL;
    cmr_handle oem_handle = NULL;
    cmr_u32 fd_small_buf_num = 1;
    float src_ratio;
    float fd_small_4_3_ratio, fd_small_16_9_ratio;
    float fd_small_2_1_ratio, fd_small_1_1_ratio;
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

    src_ratio = (float)in->frame_size.width / (float)in->frame_size.height;
    fd_small_4_3_ratio = (float)4 / 3;
    fd_small_16_9_ratio = (float)16 / 9;
    fd_small_2_1_ratio = (float)2 / 1;
    fd_small_1_1_ratio = (float)1 / 1;
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
    ret = cam_cxt->hal_malloc(CAMERA_FD_SMALL,
                &fd_handle->fd_small.buf_size, &fd_small_buf_num,
                &fd_handle->fd_small.addr_phy.addr_y,
                &fd_handle->fd_small.addr_vir.addr_y,
                &fd_handle->fd_small.fd, cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to malloc buffers for fd_small image");
        goto free_fd_handle;
    }
    CMR_LOGD("OK to malloc buffers for fd_small image");

    CMR_LOGD("mem_size = %ld, fd_small.buf_size = %ld",
                        fd_handle->mem_size, fd_handle->fd_small.buf_size);
    fd_handle->alloc_addr = malloc(fd_handle->mem_size);
    if (!fd_handle->alloc_addr) {
        CMR_LOGE("mem alloc failed");
        goto free_fd_handle;
    }

    ret = fd_thread_create(fd_handle);
    if (ret) {
        CMR_LOGE("failed to create thread.");
        goto free_fd_handle;
    }

    fd_img_size = &in->frame_size;
    CMR_LOGI("fd_img_size height = %d, width = %d", fd_img_size->height,
             fd_img_size->width);
    ret = fd_call_init(fd_handle, &fd_handle->fd_small.size);
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
    }
    free(fd_handle);
    return ret;
}

static cmr_int fd_close(cmr_handle class_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_fd *fd_handle = (struct class_fd *)class_handle;
    struct camera_context *cam_cxt = NULL;
    cmr_handle oem_handle = NULL;
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
        cmr_thread_destroy(fd_handle->thread_handle);
        fd_handle->thread_handle = 0;
        fd_handle->is_inited = 0;
    }

    if (fd_handle->alloc_addr) {
        free(fd_handle->alloc_addr);
    }

    oem_handle = fd_handle->common.ipm_cxt->init_in.oem_handle;
    cam_cxt = (struct camera_context *)oem_handle;

    if (cam_cxt->hal_free == NULL) {
        CMR_LOGE("cam_cxt->hal_free is NULL");
        goto out;
    }
    ret = cam_cxt->hal_free(CAMERA_FD_SMALL,
                &fd_handle->fd_small.addr_phy.addr_y,
                &fd_handle->fd_small.addr_vir.addr_y,
                &fd_handle->fd_small.fd, 1, cam_cxt->client_data);
    if (ret) {
        CMR_LOGE("Fail to free the fd_small image buffers");
        goto out;
    }
    CMR_LOGD("Ok to free the fd_small image buffers");

    free(fd_handle);

out:
    return ret;
}

static cmr_int fd_transfer_frame(cmr_handle class_handle,
                                 struct ipm_frame_in *in,
                                 struct ipm_frame_out *out) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct class_fd *fd_handle = (struct class_fd *)class_handle;
    cmr_uint frame_cnt;
    cmr_u32 is_busy = 0;
    struct fd_start_parameter param;

    if (!in || !class_handle) {
        CMR_LOGE("Invalid Param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    frame_cnt = ++fd_handle->frame_cnt;

    if (frame_cnt < fd_handle->frame_total_num) {
        CMR_LOGD("This is fd 0x%ld frame. need the 0x%ld frame,", frame_cnt,
                 fd_handle->frame_total_num);
        return ret;
    }

    fd_handle->curr_frame_idx++;

    // reduce the frame rate, because the current face detection (tracking mode)
    // is too fast!!
    {
        const static cmr_uint DROP_RATE = 2;
        if ((fd_handle->curr_frame_idx % DROP_RATE) != 0) {
            return ret;
        }
    }

    is_busy = fd_is_busy(fd_handle);
    // CMR_LOGI("fd is_busy =%d", is_busy);

    if (!is_busy) {
        fd_handle->frame_cnt = 0;
        fd_handle->frame_in = *in;

        param.frame_data = (void *)in->src_frame.addr_vir.addr_y;
        param.frame_cb = fd_handle->frame_cb;
        param.caller_handle = in->caller_handle;
        param.private_data = in->private_data;

        memcpy(fd_handle->alloc_addr, (void *)in->src_frame.addr_vir.addr_y,
               fd_handle->mem_size);

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
        memcpy(&fd_handle->frame_out.face_area, &fd_handle->face_area_prev,
               sizeof(struct img_face_area));
        fd_handle->frame_out.dst_frame.size.width =
            fd_handle->frame_in.src_frame.size.width;
        fd_handle->frame_out.dst_frame.size.height =
            fd_handle->frame_in.src_frame.size.height;
        fd_handle->frame_out.dst_frame.reserved =
            fd_handle->frame_in.dst_frame.reserved;

        /*callback*/
        if (fd_handle->frame_cb) {
            fd_handle->frame_out.private_data = in->private_data;
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
                                (void *)class_handle);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }
        ret = cmr_thread_set_name(class_handle->thread_handle, "fd");
        if (CMR_MSG_SUCCESS != ret) {
            CMR_LOGE("fail to set thr name");
            ret = CMR_MSG_SUCCESS;
        }

        class_handle->is_inited = 1;
    } else {
        CMR_LOGI("fd is inited already");
    }

end:
    return ret;
}

static void fd_recognize_face_attribute(
    FD_DETECTOR_HANDLE hDT, FA_ALIGN_HANDLE hFaceAlign,
    FAR_RECOGNIZER_HANDLE hFAR, struct class_faceattr_array *io_faceattr_arr,
    const cmr_u8 *i_image_data, struct img_size i_image_size,
    const cmr_uint i_curr_frame_idx) {
    cmr_int face_count = 0;
    cmr_int fd_idx = 0;
    cmr_int i = 0;
    struct class_faceattr_array new_attr_array;
    FA_IMAGE img;

    face_count = FdGetFaceCount(hDT);
    if (face_count <= 0) {
        return;
    }

    /* Don't update face attribute, if the frame interval is not enough. For
     * reducing computation cost */
    if ((io_faceattr_arr->count > 0) &&
        (i_curr_frame_idx - io_faceattr_arr->frame_idx) < FD_RUN_FAR_INTERVAL) {
        return;
    }

    cmr_bzero(&new_attr_array, sizeof(struct class_faceattr_array));
    cmr_bzero(&img, sizeof(FA_IMAGE));

    img.data = (unsigned char *)i_image_data;
    img.width = i_image_size.width;
    img.height = i_image_size.height;
    img.step = img.width;

    // When there are many faces, process every face will be too slow.
    // Limit face count to 2
    face_count = MIN(face_count, 2);

    for (fd_idx = 0; fd_idx < face_count; fd_idx++) {
        struct class_faceattr *fattr = &(new_attr_array.face[fd_idx]);
        FD_FACEINFO info;
        FdGetFaceInfo(hDT, fd_idx, &info);

        /* Assign the same face id with FD */
        fattr->face_id = info.id;
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
            FaFaceAlign(hFaceAlign, &img, &faface, &(fattr->shape));
        }

        /* Run face attribute recognition */
        {
            FAR_OPTION opt;
            FAR_FACEINFO farface;

            /* set option: only do smile detection */
            opt.smileOn = 1;
            opt.eyeOn = 0;
            opt.infantOn = 0;
            opt.genderOn = 0;

            /* Set the eye locations */
            for (i = 0; i < 7; i++) {
                farface.landmarks[i].x = fattr->shape.data[i * 2];
                farface.landmarks[i].y = fattr->shape.data[i * 2 + 1];
            }

            {
                int err = FarRecognize(hFAR, (const FAR_IMAGE *)&img, &farface,
                                       &opt, &(fattr->attr));
                // CMR_LOGI("FarRecognize: err=%d, smile=%d", err,
                // fattr->attr.smile);
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

    // Try to correct the face rectangle by the face shape which is often more
    // accurate
    if (i_faceattr_arr != NULL) {
        // find the face shape with the same face ID
        const cmr_int shape_score_thr = 200;
        const struct class_faceattr *fattr = NULL;
        cmr_int i = 0;
        for (i = 0; i < i_faceattr_arr->count; i++) {
            if (i_faceattr_arr->face[i].face_id == io_curr_face->face_id) {
                fattr = &(i_faceattr_arr->face[i]);
                break;
            }
        }

        // Calculate the new face rectangle according to the face shape
        if (fattr != NULL && fattr->shape.score >= shape_score_thr) {
            const int *sdata = (const int *)(fattr->shape.data);
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
    }

    // Try to keep the face rectangle to be the same with the previous
    // frame (for stable looks)
    overlap_thr = trust_curr_face ? 90 : 80;
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

static void fd_get_fd_results(FD_DETECTOR_HANDLE hDT,
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
        {
            cmr_int delta = info.width / 20;
            sx -= delta;
            sy -= delta;
            ex += delta;
            ey += delta;
        }

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
        face_ptr->face_id = info.id;
        face_ptr->pose = info.yawAngle;
        face_ptr->angle = info.rollAngle;
        face_ptr->score =
            info.score / 10; /* Make it in [0,100]. HAL1.0 requires so */
        face_ptr->smile_level = 1;
        face_ptr->blink_level = 0;
        face_ptr->brightness = 128;

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
                                               // it is a tuning parameter, must
                                               // be in [1, 50]
            cmr_int i = 0;
            for (i = 0; i < i_faceattr_arr->count; i++) {
                const struct class_faceattr *fattr = &(i_faceattr_arr->face[i]);
                if (fattr->face_id == info.id) {
                    /* Note: The original smile score is in [-100, 100].
                       But the Camera APP needs a score in [0, 100], and also
                       the definitions for smile degree are different
                       with the algorithm. So, we need to adjust the smile score
                       to fit the APP.
                    */
                    cmr_int smile_score = MAX(0, fattr->attr.smile);
                    if (smile_score >= algo_smile_thr) {
                        /* norm_score is in [0, 70] */
                        cmr_int norm_score = ((smile_score - algo_smile_thr) *
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
                    break;
                }
            }
        }
    }

    o_face_area->face_count = valid_count;
}

static cmr_int fd_create_detector(FD_DETECTOR_HANDLE *hDT,
                                  const struct img_size *fd_img_size) {
    FD_OPTION opt;
    FD_VERSION_T version;
    FdGetVersion(&version);
    CMR_LOGI("SPRD FD version: %s .", version.built_rev);
#ifdef CONFIG_SPRD_FD_HW_SUPPORT
    opt.fdEnv = FD_ENV_HW;
#else
    opt.fdEnv = FD_ENV_SW;
#endif
    FdInitOption(&opt);
    opt.workMode = FD_WORKMODE_MOVIE;
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
    opt.cfgPoolingMode = 1;
    opt.cfgPoolingFrameNum = 5;

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
    CMR_LOGV("done %ld", ret);

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
        if (FAR_OK != FarCreateRecognizerHandle(&(class_handle->hFAR))) {
            CMR_LOGE("FarCreateRecognizerHandle() Error");
            break;
        }

        /* Creates Face Detection handle */
        fd_img_size = (struct img_size *)message->data;
        ret = fd_create_detector(&(class_handle->hDT), fd_img_size);
        if (ret != FD_OK) {
            CMR_LOGE("fd_create_detector() Error");
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

        class_handle->frame_in.src_frame.data_end.y_endian = 1;
        class_handle->frame_in.src_frame.data_end.uv_endian = 2;
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
        class_handle->fd_small.data_end.y_endian = 1;
        class_handle->fd_small.data_end.uv_endian = 2;
        class_handle->fd_small.rect.start_x = 0;
        class_handle->fd_small.rect.start_y = 0;
        class_handle->fd_small.rect.width = class_handle->fd_small.size.width;
        class_handle->fd_small.rect.height = class_handle->fd_small.size.height;

        class_handle->fd_small.addr_phy.addr_u = class_handle->fd_small.addr_phy.addr_y +
            class_handle->fd_small.size.width * class_handle->fd_small.size.height;
        class_handle->fd_small.addr_phy.addr_v = class_handle->fd_small.addr_phy.addr_u;

        class_handle->fd_small.addr_vir.addr_u = class_handle->fd_small.addr_vir.addr_y +
            class_handle->fd_small.size.width * class_handle->fd_small.size.height;
        class_handle->fd_small.addr_vir.addr_v = class_handle->fd_small.addr_vir.addr_u;

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

        /* recognize face attribute (smile detection) */
        fd_recognize_face_attribute(
            class_handle->hDT, class_handle->hFaceAlign, class_handle->hFAR,
            &(class_handle->faceattr_arr), (cmr_u8 *)class_handle->fd_small.addr_vir.addr_y,
            class_handle->fd_small.size, class_handle->curr_frame_idx);

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

        /* coherence of coordinates */
        for (i = 0; i < class_handle->frame_out.face_area.face_count; i++) {
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
        }

        /* save a copy for next frame to directly callback if busying */
        memcpy(&(class_handle->face_area_prev),
               &(class_handle->frame_out.face_area),
               sizeof(struct img_face_area));

        duration = (end_time - start_time) * 1000 / CLOCKS_PER_SEC;
        CMR_LOGD("%dx%d, face_num=%ld, time=%d ms",
                 class_handle->fd_small.size.width,
                 class_handle->fd_small.size.height,
                 class_handle->frame_out.face_area.face_count, duration);
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
        FarDeleteRecognizerHandle(&(class_handle->hFAR));
        FdDeleteDetector(&(class_handle->hDT));
        break;

    default:
        break;
    }

    return ret;
}

#endif
