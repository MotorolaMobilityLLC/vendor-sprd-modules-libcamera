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
#define LOG_TAG "cmr_snp"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <stdlib.h>
#include "cmr_snapshot.h"
#include "cmr_msg.h"
#include "isp_video.h"
#include "cmr_ipm.h"
#include "cmr_oem.h"
#include <cutils/trace.h>
#include "cutils/properties.h"
#include <unistd.h>

//#include "isp_otp.h"

#define HW_SIMULATION_SLICE_WIDTH 1280
#define SNP_MSG_QUEUE_SIZE 50
#define SNP_INVALIDE_VALUE 0xFFFFFFFF
#define SENSOR_DEFAULT_PIX_WIDTH 0x0a

#define SNP_EVT_BASE (CMR_EVT_SNAPSHOT_BASE + SNPASHOT_EVT_MAX)

// for main thread
#define SNP_EVT_MAIN_INIT (SNP_EVT_BASE)
#define SNP_EVT_MAIN_DEINIT (SNP_EVT_BASE + 1)
#define SNP_EVT_MAIN_START (SNP_EVT_BASE + 2)
#define SNP_EVT_MAIN_STOP (SNP_EVT_BASE + 3)

// for cvt thread
#define SNP_EVT_CVT_INIT (SNP_EVT_BASE + 10)
#define SNP_EVT_CVT_DEINIT (SNP_EVT_BASE + 11)
#define SNP_EVT_CVT_START (SNP_EVT_BASE + 12)
#define SNP_EVT_CVT_STOP (SNP_EVT_BASE + 13)

// for postproc thread
#define SNP_EVT_POSTPROC_INIT (SNP_EVT_BASE + 20)
#define SNP_EVT_POSTPROC_DEINIT (SNP_EVT_BASE + 21)
#define SNP_EVT_POSTPROC_START (SNP_EVT_BASE + 22)
#define SNP_EVT_POSTPROC_FREE_FRM (SNP_EVT_BASE + 23)
#define SNP_EVT_POSTPROC_STOP (SNP_EVT_BASE + 24)

// for proc_cb thread
#define SNP_EVT_PROC_CB_RAW_DONE (SNP_EVT_BASE + 30)
#define SNP_EVT_PROC_CB_SCALE_DONE (SNP_EVT_BASE + 31)
#define SNP_EVT_PROC_CB_JPEG_ENC_DONE (SNP_EVT_BASE + 32)
#define SNP_EVT_PROC_CB_JPEG_DEC_DONE (SNP_EVT_BASE + 33)
#define SNP_EVT_PROC_CB_JPEG_ENC_ERR (SNP_EVT_BASE + 34)
#define SNP_EVT_PROC_CB_JPEG_DEC_ERR (SNP_EVT_BASE + 35)

// for write_exif thread
#define SNP_EVT_WRITE_EXIF_INIT (SNP_EVT_BASE + 40)
#define SNP_EVT_WRITE_EXIF_DEINIT (SNP_EVT_BASE + 41)
#define SNP_EVT_WRITE_EXIF_START (SNP_EVT_BASE + 42)
#define SNP_EVT_WRITE_EXIF_STOP (SNP_EVT_BASE + 43)

// for redisplay thread
#define SNP_EVT_REDISPLAY_START (SNP_EVT_BASE + 50)

// for thumbnail thread
#define SNP_EVT_THUMB_START (SNP_EVT_BASE + 60)

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            CMR_LOGE("err handle");                                            \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

#define IS_CAP_FRM(id, v)                                                      \
    ((((id) & (v)) == (v)) && (((id) - (v)) < CMR_CAPTURE_MEM_SUM))
#define SRC_MIPI_RAW "/data/isptool_src_mipi_raw_file.raw"

/********************************* internal data type
 * *********************************/

enum cvt_trigger_src { SNP_TRIGGER = 0, NON_SNP_TRIGGER, CVT_TRIGGER_SRC_MAX };

struct snp_thread_context {
    cmr_handle main_thr_handle;
    cmr_handle post_proc_thr_handle;
    cmr_handle notify_thr_handle;
    cmr_handle proc_cb_thr_handle;
    cmr_handle secondary_thr_handle;
    cmr_handle cvt_thr_handle;
    cmr_handle write_exif_thr_handle;
    cmr_handle proc_redisplay_handle;
    cmr_handle proc_thumb_handle;
    sem_t writte_exif_access_sm;
};

struct process_status {
    cmr_u32 slice_height_in;
    cmr_u32 slice_height_out;
    cmr_u32 is_encoding;
    cmr_u32 slice_num;
    struct frm_info frame_info;
};

struct snp_rot_param {
    cmr_uint angle;
    struct img_frm src_img;
    struct img_frm dst_img;
};

struct snp_scale_param {
    cmr_uint slice_height;
    struct img_frm src_img;
    struct img_frm dst_img;
};

struct snp_jpeg_param {
    struct img_frm src;
    struct img_frm dst;
    struct img_frm super;
    struct cmr_op_mean mean;
};

struct snp_ipm_param {
    struct img_frm src;
};

struct snp_exif_param {
    struct img_frm big_pic_stream_src;
    struct img_frm thumb_stream_src;
    struct img_frm dst;
};

struct snp_exif_out_param {
    cmr_uint virt_addr;
    cmr_uint stream_size;
};

struct snp_raw_proc_param {
    struct img_frm src_frame;
    struct img_frm dst_frame;
    cmr_u32 src_avail_height;
    cmr_u32 src_slice_height;
    cmr_u32 dst_slice_height;
    cmr_u32 lice_num;
};

struct snp_channel_param {
    cmr_u32 is_scaling;
    cmr_u32 is_rot;
    struct img_frm chn_frm[CMR_CAPTURE_MEM_SUM];
    struct img_frm hdr_src_frm[CMR_CAPTURE_MEM_SUM];
    struct snp_scale_param scale[CMR_CAPTURE_MEM_SUM];
    struct snp_scale_param convert_thumb[CMR_CAPTURE_MEM_SUM];
    struct snp_rot_param rot[CMR_CAPTURE_MEM_SUM];
    /*jpeg encode param*/
    cmr_u32 thumb_stream_size[CMR_CAPTURE_MEM_SUM];
    struct snp_jpeg_param jpeg_in[CMR_CAPTURE_MEM_SUM];
    struct snp_ipm_param ipm[CMR_CAPTURE_MEM_SUM];
    struct snp_jpeg_param thumb_in[CMR_CAPTURE_MEM_SUM];
    struct snp_exif_param exif_in[CMR_CAPTURE_MEM_SUM];
    struct snp_exif_out_param exif_out[CMR_CAPTURE_MEM_SUM];
    /*jpeg decode param*/
    struct snp_jpeg_param jpeg_dec_in[CMR_CAPTURE_MEM_SUM];
    /*isp proc*/
    struct raw_proc_param isp_proc_in[CMR_CAPTURE_MEM_SUM];
    struct process_status isp_process[CMR_CAPTURE_MEM_SUM];
};

struct snp_cvt_context {
    cmr_u32 proc_height;
    cmr_u32 is_first_slice;
    cmr_u32 slice_height;
    cmr_u32 trigger;
    sem_t cvt_sync_sm;
    struct frm_info frame_info;
    struct img_frm out_frame;
};

struct snp_context {
    cmr_handle oem_handle;
    cmr_uint is_inited;
    cmr_uint camera_id;
    cmr_uint status;
    cmr_int err_code;
    cmr_u32 is_request;
    cmr_u32 index;
    sem_t access_sm;
    snapshot_cb_of_state oem_cb;
    struct snapshot_md_ops ops;
    struct snp_thread_context thread_cxt;
    void *caller_privdata;
    struct snapshot_param req_param;
    struct sensor_exp_info sensor_info;
    /*channel param*/
    struct snp_channel_param chn_param;
    /*post proc contrl*/
    struct frm_info cur_frame_info;
    cmr_u32 cap_cnt;
    cmr_u32 is_frame_done;
    cmr_u32 jpeg_stream_size;
    cmr_u32 thumb_stream_size;
    cmr_u32 jpeg_exif_stream_size;
    cmr_u32 isptool_saved_file_count;
    cmr_u32 hdr_src_num;
    cmr_u32 padding;
    sem_t proc_done_sm;
    sem_t jpeg_sync_sm;
    sem_t scaler_sync_sm;
    sem_t takepic_callback_sem;
    sem_t hdr_sync_sm;
    sem_t ipm_sync_sm;
    sem_t redisplay_sm;
    sem_t writer_exif_sm;
    sem_t pre_start_encode_sync_sm;
    struct snp_cvt_context cvt;
};
/********************************* internal data type
 * *********************************/
static cmr_int snp_main_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_postproc_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_notify_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_proc_cb_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_secondary_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_cvt_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_write_exif_thread_proc(struct cmr_msg *message,
                                          void *p_data);
static cmr_int snp_redisplay_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_thumb_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int snp_create_main_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_main_thread(cmr_handle snp_handle);
static cmr_int snp_create_postproc_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_postproc_thread(cmr_handle snp_handle);
static cmr_int snp_create_notify_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_notify_thread(cmr_handle snp_handle);
static cmr_int snp_create_proc_cb_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_proc_cb_thread(cmr_handle snp_handle);
static cmr_int snp_create_secondary_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_secondary_thread(cmr_handle snp_handle);
static cmr_int snp_create_cvt_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_cvt_thread(cmr_handle snp_handle);
static cmr_int snp_create_write_exif_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_write_exif_thread(cmr_handle snp_handle);
static cmr_int snp_create_redisplay_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_redisplay_thread(cmr_handle snp_handle);
static cmr_int snp_create_thumb_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_thumb_thread(cmr_handle snp_handle);
static cmr_int snp_create_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_thread(cmr_handle snp_handle);
static void snp_local_init(cmr_handle snp_handle);
static void snp_local_deinit(cmr_handle snp_handle);
static cmr_int snp_check_post_proc_param(struct snapshot_param *param_ptr);
static cmr_int snp_set_channel_out_param(cmr_handle snp_handle);
static cmr_int snp_set_hdr_param(cmr_handle snp_handle);
static cmr_int snp_set_scale_param(cmr_handle snp_handle);
static cmr_int snp_set_convert_thumb_param(cmr_handle snp_handle);
static cmr_int snp_set_rot_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_enc_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_thumb_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_exif_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_dec_param(cmr_handle snp_handle);
static cmr_int snp_set_isp_proc_param(cmr_handle snp_handle);
static void snp_get_is_scaling(cmr_handle snp_handle,
                               cmr_s32 is_normal_snapshot);
static cmr_int snp_clean_thumb_param(cmr_handle snp_handle);
static cmr_int snp_set_post_proc_param(cmr_handle snp_handle,
                                       struct snapshot_param *param_ptr);
static void snp_set_request(cmr_handle snp_handle, cmr_u32 is_request);
static cmr_u32 snp_get_request(cmr_handle snp_handle);
static cmr_int snp_check_frame_is_valid(cmr_handle snp_handle,
                                        struct frm_info *frm_ptr);
static cmr_int snp_post_proc_for_yuv(cmr_handle snp_handle, void *data);
static cmr_int snp_post_proc_for_jpeg(cmr_handle snp_handle, void *data);
static cmr_int snp_post_proc_for_raw(cmr_handle snp_handle, void *data);
static void snp_reset_ctrl_condition(cmr_handle snp_handle);
static cmr_int snp_post_proc(cmr_handle snp_handle, void *data);
static cmr_int snp_stop_proc(cmr_handle snp_handle);
static cmr_int snp_start_encode(cmr_handle snp_handle, void *data);
static cmr_int snp_start_encode_thumb(cmr_handle snp_handle);
static cmr_int snp_start_isp_proc(cmr_handle snp_handle, void *data);
static cmr_int snp_start_isp_next_proc(cmr_handle snp_handle, void *data);
static cmr_int snp_start_rot(cmr_handle snp_handle, void *data);
static cmr_int snp_start_scale(cmr_handle snp_handle, void *data);
static cmr_int snp_start_convet_thumb(cmr_handle snp_handle, void *data);
static cmr_int snp_start_decode_sync(cmr_handle snp_handle, void *data);
static cmr_int snp_start_decode(cmr_handle snp_handle, void *data);
static void snp_wait_cvt_done(cmr_handle snp_handle);
static void snp_cvt_done(cmr_handle snp_handle);
static cmr_int snp_start_cvt(cmr_handle snp_handle, void *data);
static void snp_post_proc_exit(struct snp_context *snp_cxt);
/*static void snp_main_thr_proc_exit(struct snp_context *snp_cxt);*/
static cmr_int snp_send_msg_notify_thr(cmr_handle snp_handle, cmr_int func_type,
                                       cmr_int evt, void *data,
                                       cmr_uint data_len);
static cmr_int snp_send_msg_secondary_thr(cmr_handle snp_handle,
                                          cmr_int func_type, cmr_int evt,
                                          void *data);
static cmr_int snp_send_msg_write_exif_thr(cmr_handle snp_handle, cmr_int evt,
                                           void *data);
static cmr_int snp_send_msg_redisplay_thr(cmr_handle snp_handle, cmr_int evt,
                                          void *data);
static cmr_int snp_send_msg_thumb_thr(cmr_handle snp_handle, cmr_int evt,
                                      void *data);
static cmr_int snp_checkout_exit(cmr_handle snp_handle);
static cmr_int snp_take_picture_done(cmr_handle snp_handle,
                                     struct frm_info *data);
static cmr_int snp_yuv_callback_take_picture_done(
    cmr_handle snp_handle,
    struct frm_info *data); /**modified for 3d calibration*/
static cmr_int snp_thumbnail(cmr_handle snp_handle, struct frm_info *data);
static cmr_int camera_set_frame_type(cmr_handle snp_handle,
                                     struct camera_frame_type *frame_type,
                                     struct frm_info *info);
static void snp_takepic_callback_done(cmr_handle snp_handle);
static void snp_takepic_callback_wait(cmr_handle snp_handle);
static cmr_int snp_redisplay(cmr_handle snp_handle, struct frm_info *data);
static cmr_int snp_start_thumb_proc(cmr_handle snp_handle,
                                    struct frm_info *data);
static cmr_int snp_notify_redisplay_proc(cmr_handle snp_handle,
                                         cmr_int func_type, cmr_int evt,
                                         void *data);
static cmr_int snp_scale_cb_handle(cmr_handle snp_handle, void *data);
static cmr_int snp_jpeg_enc_cb_handle(cmr_handle snp_handle, void *data);
static cmr_int snp_jpeg_dec_cb_handle(cmr_handle snp_handle, void *data);
static void snp_post_proc_done(cmr_handle snp_handle);
static void snp_post_proc_err_exit(cmr_handle snp_handle, cmr_int err_code);
static cmr_int isp_is_have_src_data_from_picture(void);
static void isp_set_saved_file_count(cmr_handle snp_handle);
static cmr_u32 isp_get_saved_file_count(cmr_handle snp_handle);
static cmr_int isp_overwrite_cap_mem(cmr_handle snp_handle);
static void snp_set_status(cmr_handle snp_handle, cmr_uint status);
static cmr_uint snp_get_status(cmr_handle snp_handle);
static void snp_jpeg_enc_err_handle(cmr_handle snp_handle);
/*static void snp_jpeg_dec_err_handle(cmr_handle snp_handle);*/
static void snp_autotest_proc(cmr_handle snp_handle, void *data);

static cmr_int camera_open_uvde(struct camera_context *cxt,
                                struct ipm_open_in *in_ptr,
                                struct ipm_open_out *out_ptr);
static cmr_int camera_close_uvde(struct camera_context *cxt);
static cmr_int camera_start_uvde(struct camera_context *cxt,
                                 struct img_frm *src);
static cmr_int camera_start_yde(struct camera_context *cxt,
                                struct img_frm *src);
static cmr_int camera_start_refocus(struct camera_context *cxt,
                                    struct img_frm *src);
static int32_t snp_img_padding(struct img_frm *src, struct img_frm *dst,
                               struct cmr_op_mean *mean);
static cmr_int snp_ipm_process(cmr_handle snp_handle, void *data);

cmr_int snp_main_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle snp_handle = (cmr_handle)p_data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data %p", message->msg_type,
             message->data);
    switch (message->msg_type) {
    case SNP_EVT_MAIN_INIT:
        break;
    case SNP_EVT_MAIN_DEINIT:
        break;
    case SNP_EVT_MAIN_START:
        ret = snp_set_post_proc_param(snp_handle,
                                      (struct snapshot_param *)message->data);
        cxt->err_code = ret;
        if (!ret) {
            snp_set_request(snp_handle, TAKE_PICTURE_NEEDED);
            snp_set_status(snp_handle, IDLE);
        } else {
            CMR_LOGE("failed to set param %ld", ret);
        }
        break;
    case SNP_EVT_MAIN_STOP:
        CMR_LOGD("need to stop snp");
        ret = snp_stop_proc(snp_handle);
        cxt->err_code = ret;
        if (ret && ret != CMR_CAMERA_NORNAL_EXIT) {
            CMR_LOGE("fail to stop %ld", ret);
        }
        break;
    default:
        CMR_LOGD("don't support msg");
        break;
    }
exit:
    CMR_LOGV("X");
    return ret;
}

/*void snp_main_thr_proc_exit(struct snp_context * snp_cxt)
{
        CMR_LOGD("done");
}*/
cmr_int snp_proc_copy_frame(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    return ret;
}
cmr_int snp_postproc_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)p_data;
    struct buffer_cfg buf_cfg;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(p_data)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data 0x%lx, fd 0x%x", message->msg_type,
             (cmr_uint)message->data, ((struct frm_info *)message->data)->fd);
    switch (message->msg_type) {
    case SNP_EVT_POSTPROC_INIT:
        break;
    case SNP_EVT_POSTPROC_START:
        ret = snp_post_proc((cmr_handle)p_data, message->data);
        break;
    case SNP_EVT_POSTPROC_FREE_FRM:
        if (cxt->ops.channel_buff_cfg) {
            struct frm_info frame = *(struct frm_info *)message->data;
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = frame.channel_id;
            buf_cfg.base_id = CMR_BASE_ID(frame.frame_id);
            buf_cfg.count = 1;
            buf_cfg.flag = BUF_FLAG_RUNNING;
            buf_cfg.addr[0].addr_y = frame.yaddr;
            buf_cfg.addr[0].addr_u = frame.uaddr;
            buf_cfg.addr[0].addr_v = frame.vaddr;
            buf_cfg.addr_vir[0].addr_y = frame.yaddr_vir;
            buf_cfg.addr_vir[0].addr_u = frame.uaddr_vir;
            buf_cfg.addr_vir[0].addr_v = frame.vaddr_vir;
            buf_cfg.is_4in1 = frame.is_4in1_frame;
            buf_cfg.fd[0] = frame.fd;
            cxt->ops.channel_buff_cfg(cxt->oem_handle, &buf_cfg);
            CMR_LOGD("free frame");
        }
        break;
    case SNP_EVT_POSTPROC_STOP:
        break;
    case SNP_EVT_POSTPROC_DEINIT:
        break;
    default:
        break;
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int snp_notify_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)p_data;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (cxt == NULL || cxt->oem_cb == NULL) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGD("message.msg_type 0x%x, data %p", message->msg_type,
             message->data);

    if (message->msg_type >= SNAPSHOT_CB_EVT_MAX) {
        CMR_LOGE("don't support this 0x%x", message->msg_type);
        goto exit;
    }

    cxt->oem_cb(cxt->oem_handle, message->msg_type, message->sub_msg_type,
                message->data);

exit:
    return ret;
}

cmr_int snp_scale_cb_handle(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct img_frm *scale_out_ptr = (struct img_frm *)data;
    if (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr ||
        cxt->req_param.is_pipviv_mode || cxt->req_param.is_3dcalibration_mode ||
        cxt->req_param.is_yuv_callback_mode) {
        if (cxt->req_param.is_3dcalibration_mode ||
            cxt->req_param.is_yuv_callback_mode) {
            ret = snp_yuv_callback_take_picture_done(snp_handle,
                                                     &cxt->cur_frame_info);
        } else {
            ret = snp_take_picture_done(snp_handle, &cxt->cur_frame_info);
        }
        snp_post_proc_done(snp_handle);
        return ret;
    }
    if (cxt->err_code) {
        CMR_LOGE("err exit");
        sem_post(&cxt->scaler_sync_sm);
        sem_post(&cxt->jpeg_sync_sm);
        return ret;
    }
    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        sem_post(&cxt->scaler_sync_sm);
        sem_post(&cxt->jpeg_sync_sm);
        goto exit;
    }
    if (scale_out_ptr->size.height ==
        cxt->req_param.post_proc_setting.actual_snp_size.height) {
        ret = snp_start_encode(snp_handle, &cxt->cur_frame_info);
        if (ret) {
            CMR_LOGE("failed to start encode %ld", ret);
            sem_post(&cxt->scaler_sync_sm);
            goto exit;
        }
        if ((0 != cxt->req_param.jpeg_setting.thum_size.width) &&
            (0 != cxt->req_param.jpeg_setting.thum_size.height)) {
            ret = snp_start_thumb_proc(snp_handle, &cxt->cur_frame_info);
            if (ret) {
                CMR_LOGE("failed to start thumb %ld", ret);
                goto exit;
            }
        }
        ret = snp_redisplay(snp_handle, &cxt->cur_frame_info);
        if (ret) {
            CMR_LOGE("failed to take pic done %ld", ret);
            goto exit;
        }
    } else {
        if (0 != scale_out_ptr->size.width) {
            ret = CMR_CAMERA_NO_SUPPORT;
        } else {
            ret = CMR_CAMERA_FAIL;
        }
        sem_post(&cxt->jpeg_sync_sm);
        CMR_LOGD("don't support %ld", ret);
    }
exit:
    CMR_LOGD("post jpeg sync sm");
    if ((0 == cxt->req_param.jpeg_setting.thum_size.width) ||
        (0 == cxt->req_param.jpeg_setting.thum_size.height)) {
        sem_post(&cxt->scaler_sync_sm);
    }
    if (ret) {
        snp_post_proc_err_exit(snp_handle, ret);
        cxt->err_code = ret;
    }
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

void snp_post_proc_err_exit(cmr_handle snp_handle, cmr_int err_code) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cxt->cap_cnt = 0;
    cmr_bzero(&cxt->cur_frame_info, sizeof(cxt->cur_frame_info));
    snp_set_request(snp_handle, TAKE_PICTURE_NO);
    sem_post(&cxt->proc_done_sm);
    if (CODEC_WORKING == snp_get_status(snp_handle)) {
        sem_post(&cxt->jpeg_sync_sm);
    }
    snp_set_status(snp_handle, IDLE);
    if (CMR_CAMERA_NORNAL_EXIT != err_code) {
        ret = snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                      SNAPSHOT_CB_EVT_FAILED, NULL, 0);
    }
    CMR_LOGV("X");
}

void snp_post_proc_done(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        cxt->cap_cnt = 0;
        goto exit;
    }
    CMR_LOGD("wait beging for redisplay sm");
    sem_wait(&cxt->redisplay_sm);
    CMR_LOGD("wait end for redisplay sm");

    if (isp_video_get_simulation_flag()) {
        CMR_LOGI("hwsim:snp_set_request\n");
        snp_set_request(snp_handle, TAKE_PICTURE_NO);
    } else {
        if (cxt->cap_cnt >= cxt->req_param.total_num) {
            cxt->cap_cnt = 0;
            snp_set_request(snp_handle, TAKE_PICTURE_NO);
        }
    }

exit:
    CMR_LOGV("post");
    snp_set_status(snp_handle, IDLE);
    sem_post(&cxt->proc_done_sm);
    CMR_LOGV("done");
}

cmr_int snp_jpeg_enc_cb_handle(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct jpeg_enc_cb_param *enc_out_ptr = (struct jpeg_enc_cb_param *)data;
    if (cxt == NULL){
        return CMR_CAMERA_FAIL;
    }
    struct cmr_cap_mem *mem_ptr =
        &cxt->req_param.post_proc_setting.mem[cxt->index];
    char value[PROPERTY_VALUE_MAX];
    struct camera_frame_type frame_type;
    void *isp_info_addr = NULL;
    int isp_info_size = 0;

    if (cxt->err_code) {
        CMR_LOGE("error exit");
        sem_post(&cxt->jpeg_sync_sm);
        return ret;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 5 || atoi(value) & (1 << 5)) {
        struct camera_context *cam_ctx = cxt->oem_handle;
        ret =
            dump_image("snp_jpeg_enc_cb_handle", CAM_IMG_FMT_JPEG,
                       cxt->req_param.post_proc_setting.actual_snp_size.width,
                       cxt->req_param.post_proc_setting.actual_snp_size.height,
                       cam_ctx->dump_cnt, &mem_ptr->target_jpeg.addr_vir,
                       enc_out_ptr->stream_size);
    }

    if (enc_out_ptr->total_height ==
        cxt->req_param.post_proc_setting.actual_snp_size.height) {
        property_get("persist.vendor.cam.isptool.mode.enable", value, "false");
        if ((!strcmp(value, "true")) ||
            (CAMERA_ISP_SIMULATION_MODE == cxt->req_param.mode)) {
            if (isp_video_get_simulation_flag()) {
                struct img_addr jpeg_addr;
                cmr_bzero((void *)&jpeg_addr, sizeof(jpeg_addr));
                jpeg_addr.addr_y = mem_ptr->target_jpeg.addr_vir.addr_y;
                if (isp_video_get_simulation_loop_count() == 1) {
                    ret = camera_local_get_isp_info(
                        cxt->oem_handle, &isp_info_addr, &isp_info_size);
                    if (ret == 0 && isp_info_size > 0) {
                        memcpy(((char *)jpeg_addr.addr_y +
                                enc_out_ptr->stream_size),
                               (char *)isp_info_addr, isp_info_size);
                        cxt->ops.dump_image_with_3a_info(
                            cxt->oem_handle, CAM_IMG_FMT_JPEG,
                            cxt->req_param.post_proc_setting.actual_snp_size
                                .width,
                            cxt->req_param.post_proc_setting.actual_snp_size
                                .height,
                            enc_out_ptr->stream_size + isp_info_size,
                            &jpeg_addr);
                    } else {
                        CMR_LOGD("save jpg without isp debug info.");
                        cxt->ops.dump_image_with_3a_info(
                            cxt->oem_handle, CAM_IMG_FMT_JPEG,
                            cxt->req_param.post_proc_setting.actual_snp_size
                                .width,
                            cxt->req_param.post_proc_setting.actual_snp_size
                                .height,
                            enc_out_ptr->stream_size, &jpeg_addr);
                    }
                }
                isp_video_set_capture_complete_flag();
            } else {
                int u_ret = usleep(500 * 1000);
                if (u_ret)
                    CMR_LOGE("usleep failed !");
                if((cxt != NULL) && (mem_ptr != NULL)){
                    send_capture_data(
                        0x10, /* jpg */
                        cxt->req_param.post_proc_setting.actual_snp_size.width,
                        cxt->req_param.post_proc_setting.actual_snp_size.height,
                        (char *)mem_ptr->target_jpeg.addr_vir.addr_y,
                        enc_out_ptr->stream_size, 0, 0, 0, 0);
                }
            }
        }
        cxt->jpeg_stream_size = enc_out_ptr->stream_size;
        CMR_LOGD("jpeg_stream_size %d", cxt->jpeg_stream_size);

        sem_post(&cxt->jpeg_sync_sm);
        CMR_LOGD("post jpeg_sync_sm");

        snp_set_status(snp_handle, POST_PROCESSING);
        frame_type.status = ret;
        snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE,
                                SNAPSHOT_CB_EVT_ENC_DONE, (void *)&frame_type,
                                sizeof(struct camera_frame_type));
    } else {
        ret = CMR_CAMERA_NO_SUPPORT;
        CMR_LOGD("don't support");
        goto exit;
    }

exit:
    CMR_LOGD("done %ld", ret);
    if (ret) {
        sem_post(&cxt->jpeg_sync_sm);
        CMR_LOGV("post");
        snp_post_proc_err_exit(snp_handle, ret);
        cxt->err_code = ret;
    }
    ATRACE_END();
    return ret;
}
void snp_jpeg_enc_err_handle(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    CMR_LOGD("err code %ld", cxt->err_code);
    sem_post(&cxt->jpeg_sync_sm);
    snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                            SNAPSHOT_CB_EVT_FAILED, NULL, 0);
}

cmr_int snp_proc_cb_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)p_data;

    CMR_LOGD("done %ld", ret);

    if (!message || !p_data) {
        CMR_LOGE("param error");
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data %p", message->msg_type,
             message->data);
    switch (message->msg_type) {
    case SNP_EVT_PROC_CB_RAW_DONE:
        snp_cvt_done((cmr_handle)cxt);
        ret = CMR_CAMERA_SUCCESS;
        CMR_LOGD("ret %ld", ret);
        break;
    case SNP_EVT_PROC_CB_SCALE_DONE:
        ret = snp_scale_cb_handle((cmr_handle)cxt, message->data);
        cxt->err_code = ret;
        break;
    case SNP_EVT_PROC_CB_JPEG_ENC_DONE:
        ret = snp_jpeg_enc_cb_handle((cmr_handle)cxt, message->data);
        cxt->err_code = ret;
        break;
    case SNP_EVT_PROC_CB_JPEG_DEC_DONE:
        break;
    case SNP_EVT_PROC_CB_JPEG_ENC_ERR:
        CMR_LOGE("jpeg enc error");
        cxt->err_code = CMR_CAMERA_FAIL;
        snp_jpeg_enc_err_handle((cmr_handle)cxt);
        break;
    case SNP_EVT_PROC_CB_JPEG_DEC_ERR:
        CMR_LOGE("jpeg dec error");
        cxt->err_code = CMR_CAMERA_FAIL;
        break;
    default:
        CMR_LOGD("don't support this evt 0x%x", message->msg_type);
        break;
    }
exit:
    return ret;
}

cmr_int snp_secondary_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)p_data;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data %p", message->msg_type,
             message->data);

    if (SNAPSHOT_CB_EVT_MAX > message->msg_type) {
        if (cxt->oem_cb) {
            cxt->oem_cb(cxt->oem_handle, message->msg_type,
                        message->sub_msg_type, message->data);
        } else {
            CMR_LOGE("err, oem cb is null");
        }
    } else {
        CMR_LOGE("don't support this 0x%x", message->msg_type);
    }
exit:
    CMR_LOGD("X %ld", ret);
    return ret;
}

#ifdef SUPER_MACRO
cmr_int snp_save_yuv_for_macro(cmr_handle snp_handle, struct snp_jpeg_param *jpeg_in)
{
    cmr_u32 tmp = 0;
    cmr_int ret = 0;
    cmr_uint tmp_dst_addr = 0;
    cmr_uint tmp_src_addr = 0;
    cmr_uint rotation = 0;
    char name[20] = {0};
    char value[PROPERTY_VALUE_MAX] = {0};
    struct camera_context *cxt = NULL;
    struct setting_context *setting_cxt = NULL;
    struct snp_context *snp_cxt = NULL;
    struct setting_cmd_parameter setting_param = {0};
    struct cmr_op_mean op_mean = {0};
    struct img_frm *src = NULL;
    struct img_frm *dst = NULL;

    snp_cxt = (struct snp_context *)snp_handle;
    cxt = (struct camera_context *)snp_cxt->oem_handle;
    setting_cxt = &cxt->setting_cxt;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                SETTING_GET_ENCODE_ROTATION, &setting_param);
    rotation = setting_param.cmd_type_value;
    ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                SETTING_GET_FLIP_ON, &setting_param);
    op_mean.flip = setting_param.cmd_type_value;

    CMR_LOGV("flip %d rotation %d", op_mean.flip, rotation);

    if (rotation != 0) {
        if (90 == rotation)
            op_mean.rot = IMG_ANGLE_90;
        else if (180 == rotation) {
            op_mean.rot = IMG_ANGLE_180;
        } else if (270 == rotation) {
            op_mean.rot = IMG_ANGLE_270;
        }

        src = &jpeg_in->src;
        dst = &jpeg_in->super;
        dst->addr_phy.addr_u = src->addr_phy.addr_u;

        CMR_LOGV("src fd 0x%x, phy Y 0x%x, U 0x%x, V 0x%x",
            src->fd, src->addr_phy.addr_y, src->addr_phy.addr_u, src->addr_phy.addr_v);
        CMR_LOGV("dst fd 0x%x, phy Y 0x%x, U 0x%x, v 0x%x",
            dst->fd, dst->addr_phy.addr_y, dst->addr_phy.addr_u, dst->addr_phy.addr_v);

        src->data_end = snp_cxt->req_param.post_proc_setting.data_endian;
        dst->data_end = snp_cxt->req_param.post_proc_setting.data_endian;
        if (rotation == 90 || rotation == 270) {
            tmp = dst->size.width;
            dst->size.width = dst->size.height;
            dst->size.height = tmp;
        }

        if (snp_cxt->ops.start_rot != NULL)
            ret = snp_cxt->ops.start_rot(snp_cxt->oem_handle,
                        (cmr_handle)snp_cxt,src, dst, &op_mean);
        else
            CMR_LOGW("cannot do rotation");
        tmp_dst_addr = jpeg_in->super.addr_vir.addr_y;
    } else {
        tmp_dst_addr = jpeg_in->super.addr_vir.addr_y;
        tmp_src_addr = jpeg_in->src.addr_vir.addr_y;
        memcpy((char *)tmp_dst_addr, (char *)tmp_src_addr, jpeg_in->src.buf_size);
    }
    tmp_src_addr = jpeg_in->super.size.width;
    tmp_dst_addr += jpeg_in->src.buf_size;
    memcpy((char *)tmp_dst_addr, (char *)&tmp_src_addr, sizeof(int));
    tmp_src_addr = jpeg_in->super.size.height;
    tmp_dst_addr += sizeof(int);
    memcpy((char *)tmp_dst_addr, (char *)&tmp_src_addr, sizeof(int));
    tmp_src_addr = jpeg_in->super.buf_size;
    tmp_dst_addr += sizeof(int);
    memcpy((char *)tmp_dst_addr, (char *)&tmp_src_addr, sizeof(int));
    property_get("persist.vendor.cam.macro.dump", value, "0");
    if (atoi(value) == 1) {
        sprintf(name, "sp_yuv_rot_%d", rotation);
        dump_image(name, CAM_IMG_FMT_YUV420_NV21,
            jpeg_in->super.size.width, jpeg_in->super.size.height,
            FORM_DUMPINDEX(SNP_ENCODE_SRC_DATA, 1, 0),
            &jpeg_in->super.addr_vir,
            jpeg_in->super.size.width * jpeg_in->super.size.height *
            3 / 2);
    }

    return ret;
}
#endif
/* For: not free when encoding
 * wait:0~post,1~waitv
 * return: 0: ok, -1: fail
 */
cmr_int cmr_snpshot_encode_semaphore(cmr_handle snp_handle, int wait)
{
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;

    if (!snp_cxt)
        return -1;
    if (snp_cxt->is_inited == 0)
        return -1;
    if (wait) {
        sem_wait(&snp_cxt->pre_start_encode_sync_sm);
    } else {
        sem_post(&snp_cxt->pre_start_encode_sync_sm);
    }
    return 0;
}

cmr_int snp_start_encode(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    struct frm_info *frm_ptr = (struct frm_info *)data;
    struct snp_jpeg_param *jpeg_in_ptr;
    cmr_u32 index = frm_ptr->frame_id - frm_ptr->base;
    struct jpeg_param quality_param;

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        sem_post(&snp_cxt->jpeg_sync_sm);
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }
    CMR_PRINT_TIME;

    snp_cxt->ops.get_jpeg_param_info(snp_cxt->oem_handle, &quality_param);

    snp_cxt->index = index;
    jpeg_in_ptr = &chn_param_ptr->jpeg_in[index];
    jpeg_in_ptr->src.data_end =
        snp_cxt->req_param.post_proc_setting.data_endian;
    jpeg_in_ptr->mean.quality_level = quality_param.quality;
    CMR_LOGD("jpeg_in_ptr->mean.quality_level=%d",
             jpeg_in_ptr->mean.quality_level);

    if (snp_cxt->ops.start_encode == NULL) {
        CMR_LOGE("snp_cxt->ops.start_encode is null");
        ret = -CMR_CAMERA_FAIL;
        goto exit;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 4 || atoi(value) == 100 || (atoi(value) & (1 << 4))) {
        struct camera_context *cam_ctx = snp_cxt->oem_handle;
        dump_image("snp_start_encode", CAM_IMG_FMT_YUV420_NV21,
                   jpeg_in_ptr->src.size.width, jpeg_in_ptr->src.size.height,
                   FORM_DUMPINDEX(SNP_ENCODE_SRC_DATA, cam_ctx->dump_cnt, 0),
                   &jpeg_in_ptr->src.addr_vir,
                   jpeg_in_ptr->src.size.width * jpeg_in_ptr->src.size.height *
                       3 / 2);
    }

#ifdef CONFIG_CAPTURE_DENOISE
    struct camera_context *cxt = (struct camera_context *)snp_cxt->oem_handle;
    if (cxt->is_multi_mode == MODE_MULTI_CAMERA || cxt->camera_id == 0 || cxt->camera_id == 4) {
        ret = camera_start_uvde(cxt, &jpeg_in_ptr->src);
        if (ret != CMR_CAMERA_SUCCESS) {
            CMR_LOGE("camera_start_uvde fail");
            goto exit;
        }
    }
#endif

#ifdef CONFIG_CAMERA_Y_DENOISE
    struct camera_context *cxt = (struct camera_context *)snp_cxt->oem_handle;
    ret = camera_start_yde(cxt, &jpeg_in_ptr->src);
    if (ret != CMR_CAMERA_SUCCESS) {
        CMR_LOGE("camera_start_yde fail");
        goto exit;
    }
#endif

    if ((!snp_cxt->req_param.is_video_snapshot) &&
        (!snp_cxt->req_param.is_zsl_snapshot) &&
        (snp_cxt->req_param.mode != CAMERA_ISP_TUNING_MODE)) {
        snp_img_padding(&jpeg_in_ptr->src, &jpeg_in_ptr->dst, NULL);
    }

    camera_take_snapshot_step(CMR_STEP_JPG_ENC_S);
    sem_wait(&snp_cxt->pre_start_encode_sync_sm);
    ret = snp_cxt->ops.start_encode(snp_cxt->oem_handle, snp_handle,
                                &jpeg_in_ptr->src, &jpeg_in_ptr->dst,
                                &jpeg_in_ptr->mean, NULL);

#ifdef SUPER_MACRO
    if (jpeg_in_ptr->super.addr_vir.addr_y != 0 && snp_cxt->req_param.is_super) {
        snp_save_yuv_for_macro(snp_handle, jpeg_in_ptr);
    }
#endif
    sem_post(&snp_cxt->pre_start_encode_sync_sm);
    if (ret) {
        CMR_LOGE("failed to start enc %ld", ret);
        goto exit;
    }
#ifdef SUPER_MACRO
    if (snp_cxt->req_param.is_super == 1) {
        camera_take_snapshot_step(CMR_STEP_JPG_ENC_E);
        sem_post(&snp_cxt->jpeg_sync_sm);
        snp_set_status(snp_handle, POST_PROCESSING);
        snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE,
                                SNAPSHOT_CB_EVT_ENC_DONE, NULL,
                                sizeof(struct camera_frame_type));
    }
#endif
    snp_set_status(snp_handle, CODEC_WORKING);

exit:
    CMR_LOGD("X, ret = %ld", ret);
    if (ret) {
        sem_post(&snp_cxt->jpeg_sync_sm);
        snp_cxt->err_code = ret;
    }
    ATRACE_END();
    return ret;
}

cmr_int snp_start_encode_thumb(cmr_handle snp_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    struct snp_jpeg_param *jpeg_in_ptr;
    cmr_u32 index = snp_cxt->index;
    struct jpeg_param quality_param;
    struct jpeg_enc_cb_param out_enc_cb_param;

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    if (0 == snp_cxt->req_param.jpeg_setting.thum_size.width ||
        0 == snp_cxt->req_param.jpeg_setting.thum_size.height) {
        CMR_LOGD("don't include thumb");
        goto exit;
    }

    if (snp_cxt->ops.start_encode == NULL) {
        CMR_LOGE("start_encode is null");
        ret = -CMR_CAMERA_FAIL;
        goto exit;
    }

    snp_cxt->ops.get_jpeg_param_info(snp_cxt->oem_handle, &quality_param);

    camera_take_snapshot_step(CMR_STEP_THUM_ENC_S);
    jpeg_in_ptr = &chn_param_ptr->thumb_in[index];
    jpeg_in_ptr->src.data_end =
        snp_cxt->req_param.post_proc_setting.data_endian;
    jpeg_in_ptr->dst.data_end =
        snp_cxt->req_param.post_proc_setting.data_endian;
    jpeg_in_ptr->mean.quality_level = quality_param.thumb_quality;
    CMR_LOGD("jpeg_in_ptr->mean.quality_level=%d",
             jpeg_in_ptr->mean.quality_level);

    ret = snp_cxt->ops.start_encode(snp_cxt->oem_handle, snp_handle,
                                    &jpeg_in_ptr->src, &jpeg_in_ptr->dst,
                                    &jpeg_in_ptr->mean, &out_enc_cb_param);
    if (ret) {
        CMR_LOGE("failed to start thumb enc %ld", ret);
        goto exit;
    }
    camera_take_snapshot_step(CMR_STEP_THUM_ENC_E);

    snp_cxt->thumb_stream_size = (cmr_u32)(out_enc_cb_param.stream_size);
    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 7 || atoi(value) & (1 << 7)) {
        struct camera_context *cam_ctx = snp_cxt->oem_handle;
        dump_image("snp_start_encode_thumb", CAM_IMG_FMT_JPEG,
                   jpeg_in_ptr->src.size.width, jpeg_in_ptr->src.size.height,
                   FORM_DUMPINDEX(SNP_THUMB_STREAM, cam_ctx->dump_cnt, 0),
                   &jpeg_in_ptr->dst.addr_vir, snp_cxt->thumb_stream_size);
    }

exit:
    CMR_LOGD("X, done %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int snp_start_decode_sync(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr = (struct frm_info *)data;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    cmr_u32 index = frm_ptr->frame_id - frm_ptr->base;
    struct img_frm src, dst;
    struct cmr_op_mean mean;
    char value[PROPERTY_VALUE_MAX];

    CMR_LOGD("index %d phy addr 0x%lx 0x%lx 0x%lx vrit addr 0x%lx 0x%lx 0x%lx "
             "fd 0x%x, length %d",
             index, chn_param_ptr->jpeg_dec_in[index].src.addr_phy.addr_y,
             chn_param_ptr->jpeg_dec_in[index].src.addr_phy.addr_u,
             chn_param_ptr->jpeg_dec_in[index].src.addr_phy.addr_v,
             chn_param_ptr->jpeg_dec_in[index].src.addr_vir.addr_y,
             chn_param_ptr->jpeg_dec_in[index].src.addr_vir.addr_u,
             chn_param_ptr->jpeg_dec_in[index].src.addr_vir.addr_v,
             chn_param_ptr->jpeg_dec_in[index].src.fd, frm_ptr->length);

    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 1 || atoi(value) & (1 << 1)) {
        struct camera_context *cam_ctx = snp_cxt->oem_handle;
        dump_image("snp_start_decode_sync", CAM_IMG_FMT_JPEG,
                   chn_param_ptr->chn_frm[frm_ptr->frame_id - frm_ptr->base]
                       .size.width,
                   chn_param_ptr->chn_frm[frm_ptr->frame_id - frm_ptr->base]
                       .size.height,
                   FORM_DUMPINDEX(SNP_CHN_OUT_DATA, cam_ctx->dump_cnt, 0),
                   &chn_param_ptr->jpeg_dec_in[index].src.addr_vir,
                   frm_ptr->length);
    }
    if (snp_cxt->ops.start_decode) {
        src = chn_param_ptr->jpeg_dec_in[index].src;
        dst = chn_param_ptr->jpeg_dec_in[index].dst;
        mean = chn_param_ptr->jpeg_dec_in[index].mean;
        mean.is_sync = 1;
        src.reserved = (void *)((unsigned long)frm_ptr->length);
        ret = snp_cxt->ops.start_decode(snp_cxt->oem_handle, snp_handle, &src,
                                        &dst, &mean);
        if (ret) {
            CMR_LOGE("failed to start decode proc %ld", ret);
        }
        snp_cxt->cvt.out_frame = dst;
        snp_cxt->index = index;
    } else {
        CMR_LOGE("err start_decode is null");
        ret = -CMR_CAMERA_FAIL;
    }
    return ret;
}

cmr_int snp_start_decode(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr = (struct frm_info *)data;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    cmr_u32 index = frm_ptr->frame_id - frm_ptr->base;
    struct img_frm src, dst;
    struct cmr_op_mean mean;

    if (snp_cxt->ops.start_decode) {
        src = chn_param_ptr->jpeg_dec_in[index].src;
        dst = chn_param_ptr->jpeg_dec_in[index].dst;
        mean = chn_param_ptr->jpeg_dec_in[index].mean;
        mean.is_sync = 0;
        ret = snp_cxt->ops.start_decode(snp_cxt->oem_handle, snp_handle, &src,
                                        &dst, &mean);
        if (ret) {
            CMR_LOGE("failed to start decode proc %ld", ret);
        }
        snp_cxt->cvt.out_frame = dst;
        snp_cxt->index = index;
    } else {
        CMR_LOGE("err start_decode is null");
        ret = -CMR_CAMERA_FAIL;
    }
    CMR_LOGV("X, ret %ld", ret);
    return ret;
}

cmr_int snp_start_rot(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr = (struct frm_info *)data;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    cmr_u32 index = frm_ptr->frame_id - frm_ptr->base;
    struct img_frm src, dst;
    struct cmr_op_mean mean;
    char value[PROPERTY_VALUE_MAX];

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    if (snp_cxt->ops.start_rot) {
        src = chn_param_ptr->rot[index].src_img;
        dst = chn_param_ptr->rot[index].dst_img;
        src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
        dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
        mean.rot = chn_param_ptr->rot[index].angle;
        ret = snp_cxt->ops.start_rot(snp_cxt->oem_handle, snp_handle, &src,
                                     &dst, &mean);
    } else {
        CMR_LOGE("err start_rot is null");
        ret = -CMR_CAMERA_FAIL;
    }
    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 2 || (atoi(value) & (1 << 2))) {
        struct camera_context *cam_ctx = snp_cxt->oem_handle;
        dump_image("snp_start_rot", CAM_IMG_FMT_YUV420_NV21, dst.size.width,
                   dst.size.height,
                   FORM_DUMPINDEX(SNP_ROT_DATA, cam_ctx->dump_cnt, 0),
                   &dst.addr_vir, dst.size.width * dst.size.height * 3 / 2);
    }

exit:
    CMR_LOGV("X, ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int snp_start_scale(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr = (struct frm_info *)data;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    cmr_u32 index = frm_ptr->frame_id - frm_ptr->base;
    struct img_frm src, dst;
    struct cmr_op_mean mean;

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }
    if (snp_cxt->ops.start_scale == NULL) {
        CMR_LOGE("snp_cxt->ops.start_scale is null");
        ret = -CMR_CAMERA_FAIL;
        goto exit;
    }

    camera_take_snapshot_step(CMR_STEP_SC_S);

    src = chn_param_ptr->scale[index].src_img;
    dst = chn_param_ptr->scale[index].dst_img;
    mean.slice_height = chn_param_ptr->scale[index].slice_height;
    mean.is_sync = 0;
    src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
    dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
    ret = snp_cxt->ops.start_scale(snp_cxt->oem_handle, snp_handle, &src, &dst,
                                   &mean);
    if (ret) {
        CMR_LOGE("snp_cxt->ops.start_scale failed");
    }

exit:
    CMR_LOGV("ret = %ld", ret);
    if (ret) {
        snp_cxt->err_code = ret;
        sem_post(&snp_cxt->scaler_sync_sm);
    }
    ATRACE_END();
    return ret;
}

static int32_t snp_img_padding(struct img_frm *src, struct img_frm *dst,
                               struct cmr_op_mean *mean) {
    UNUSED(mean);

    cmr_u8 *last_y_buf;
    cmr_u8 *last_uv_buf;
    cmr_u8 *dst_y_buf;
    cmr_u8 *dst_uv_buf;
    cmr_u8 *src_y_buf;
    cmr_u8 *src_uv_buf;
    cmr_u32 src_w;
    cmr_u32 src_h;
    uint32_t i;
    cmr_u32 padding_lines = 0;

    if (NULL == dst || NULL == src) {
        return -1;
    }

    src_y_buf = (cmr_u8 *)src->addr_vir.addr_y;
    src_uv_buf = (cmr_u8 *)src->addr_vir.addr_u;
    src_w = src->size.width;
    src_h = src->size.height;

    if (0 == (src_h & 0xf)) {
        CMR_LOGD("no padding");
        return 0;
    }

    last_y_buf = src_y_buf + src_w * (src_h - 1);
    dst_y_buf = src_y_buf + src_w * src_h;

    // for y
    padding_lines = (src_h % 8) ? (8 - (src_h % 8)): 0;
    CMR_LOGD("y padding_lines %d src_h %d", padding_lines, src_h);
    for (i = 0; i < padding_lines; i++) {
        CMR_LOGD("dst_y_buf %p last_y_buf %p", dst_y_buf, last_y_buf);
        memcpy(dst_y_buf, last_y_buf, src_w);
        dst_y_buf += src_w;
    }

    last_uv_buf = src_uv_buf + (src_w) * (src_h / 2 - 1);
    dst_uv_buf = src_uv_buf + src_w * src_h / 2;

    padding_lines = 8 - ((src_h / 2) % 8);
    CMR_LOGD("uv padding_lines %d src_h %d", padding_lines, src_h);
    for (i = 0; i < padding_lines; i++) {
        CMR_LOGD("dst_uv_buf %p last_uv_buf %p", dst_uv_buf, last_uv_buf);
        memcpy(dst_uv_buf, last_uv_buf, src_w);
        dst_uv_buf += src_w;
    }

    CMR_LOGV("X");
    return 0;
}

cmr_int snp_start_convet_thumb(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr = (struct frm_info *)data;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    cmr_u32 index = frm_ptr->frame_id - frm_ptr->base;
    struct img_frm src, dst;
    struct cmr_op_mean mean;
    char value[PROPERTY_VALUE_MAX];

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    if (snp_cxt->ops.start_scale == NULL) {
        CMR_LOGE("start_scale is null");
        ret = -CMR_CAMERA_FAIL;
        goto exit;
    }

    src = chn_param_ptr->convert_thumb[index].src_img;
    dst = chn_param_ptr->convert_thumb[index].dst_img;
    if (src.size.width == dst.size.width &&
        src.size.height == dst.size.height) {
        CMR_LOGD("don't need to scale");
        goto exit;
    }
    mean.slice_height = chn_param_ptr->convert_thumb[index].slice_height;
    mean.is_sync = 1;
    src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
    dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;

    camera_take_snapshot_step(CMR_STEP_CVT_THUMB_S);

#ifdef CAMERA_BRINGUP
    camera_scale_down_software(&src, &dst);
#else
    ret = snp_cxt->ops.start_scale(snp_cxt->oem_handle, snp_handle, &src, &dst,
                                   &mean);
    if (ret) {
        CMR_LOGE("snp_cxt->ops.start_scale failed");
        goto exit;
    }
#endif

    camera_take_snapshot_step(CMR_STEP_CVT_THUMB_E);

    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 6 || (atoi(value) & (1 << 6))) {
        struct camera_context *cam_ctx = snp_cxt->oem_handle;
        dump_image("snp_start_convet_thumb", CAM_IMG_FMT_YUV420_NV21,
                   dst.size.width, dst.size.height,
                   FORM_DUMPINDEX(SNP_THUMB_DATA, cam_ctx->dump_cnt, 0),
                   &dst.addr_vir, dst.size.width * dst.size.height * 3 / 2);
    }

exit:
    CMR_LOGD("X, ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int snp_start_isp_proc(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr = (struct frm_info *)data;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    struct raw_proc_param isp_in_param;
    struct common_sn_cmd_param sn_param;
    struct img_frm cap_raw_small;
    struct img_frm cap_raw_big;
    struct img_addr raw_buff;
    struct img_addr isp_raw_buff;
    cmr_u32 small_w, small_h;
    cmr_u32 index = frm_ptr->frame_id - frm_ptr->base;
    bool flag_loose = false;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 loose_flag = snp_cxt->sensor_info.sn_interface.is_loose;
    void *buff_for14bit = NULL;
    void *buff_forisp14bit = NULL;

    if (snp_cxt->ops.raw_proc == NULL) {
        CMR_LOGE("raw_proc is null");
        ret = -CMR_CAMERA_FAIL;
        goto exit;
    }

    struct cmr_cap_mem *mem_ptr =
        &snp_cxt->req_param.post_proc_setting.mem[snp_cxt->index];
    if(mem_ptr == NULL)
        return CMR_CAMERA_FD_INIT;

    isp_in_param = chn_param_ptr->isp_proc_in[index];
    chn_param_ptr->isp_process[index].slice_num = 1;

    flag_loose = (loose_flag == ISP_RAW_HALF14 || loose_flag == ISP_RAW_HALF10);
    isp_in_param.src_frame = mem_ptr->cap_raw;
    if (flag_loose) {
        isp_in_param.src_frame.fmt = ISP_DATA_NORMAL_RAW10;
    } else {
        isp_in_param.src_frame.fmt = ISP_DATA_CSI2_RAW10;
    }
    isp_in_param.dst_frame = mem_ptr->target_yuv;
    isp_in_param.dst2_frame = mem_ptr->cap_raw2;
    isp_in_param.src_avail_height = mem_ptr->cap_raw.size.height;
    isp_in_param.src_slice_height = isp_in_param.src_avail_height;
    isp_in_param.dst_slice_height = isp_in_param.src_avail_height;
    isp_in_param.dst2_slice_height = isp_in_param.src_avail_height;
    isp_in_param.slice_num = 1;

    CMR_LOGI("is_4in1_frame: %d", frm_ptr->is_4in1_frame);
    if (frm_ptr->is_4in1_frame) {
        if (flag_loose) {
            isp_in_param.src_frame.buf_size =
            mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 2;

            small_w = mem_ptr->cap_raw.size.width >> 1;
            small_h = mem_ptr->cap_raw.size.height >> 1;
            cap_raw_big = mem_ptr->cap_raw;
            cap_raw_big.buf_size =
                mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 2;

            cap_raw_small.addr_phy.addr_y =
                mem_ptr->cap_raw.addr_phy.addr_y +
                (cap_raw_big.buf_size + PAGE_SIZE - 1) &
                ~(PAGE_SIZE - 1);
            cap_raw_small.addr_vir.addr_y =
                mem_ptr->cap_raw.addr_vir.addr_y +
                (cap_raw_big.buf_size + PAGE_SIZE - 1) &
                ~(PAGE_SIZE - 1);
            cap_raw_small.fd = mem_ptr->cap_raw.fd;
            cap_raw_small.buf_size = small_w * small_h * RAWRGB_RAW14BIT_WIDTH / 8;
            cap_raw_small.size.width = small_w;
            cap_raw_small.size.height = small_h;
            cap_raw_small.fmt = ISP_DATA_NORMAL_RAW10;
        } else {
            isp_in_param.src_frame.buf_size =
            mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 5 / 4;

            small_w = mem_ptr->cap_raw.size.width >> 1;
            small_h = mem_ptr->cap_raw.size.height >> 1;
            cap_raw_big = mem_ptr->cap_raw;
            cap_raw_big.buf_size =
                mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 5 / 4;

            cap_raw_small.addr_phy.addr_y =
                mem_ptr->cap_raw.addr_phy.addr_y +
                (cap_raw_big.buf_size + PAGE_SIZE - 1) &
                ~(PAGE_SIZE - 1);
            cap_raw_small.addr_vir.addr_y =
                mem_ptr->cap_raw.addr_vir.addr_y +
                (cap_raw_big.buf_size + PAGE_SIZE - 1) &
                ~(PAGE_SIZE - 1);
            cap_raw_small.fd = mem_ptr->cap_raw.fd;
            cap_raw_small.buf_size = small_w * small_h * RAWRGB_BIT_WIDTH / 8;
            cap_raw_small.size.width = small_w;
            cap_raw_small.size.height = small_h;
            cap_raw_small.fmt = ISP_DATA_CSI2_RAW10;
        }
    }

    property_get("persist.vendor.cam.isptool.mode.enable", value, "false");
    if ((!strcmp(value, "true")) ||
        (CAMERA_ISP_SIMULATION_MODE == snp_cxt->req_param.mode)) {
        cmr_u32 raw_format, raw_pixel_width;
        raw_pixel_width = snp_cxt->sensor_info.sn_interface.pixel_width;
        if (raw_pixel_width < SENSOR_DEFAULT_PIX_WIDTH)
            raw_pixel_width = SENSOR_DEFAULT_PIX_WIDTH;
        if (flag_loose) {
            isp_in_param.src_frame.fmt = ISP_DATA_NORMAL_RAW10;
            raw_format = 0x04;
        } else {
            isp_in_param.src_frame.fmt = ISP_DATA_CSI2_RAW10;
            raw_format = 0x08;
        }
        CMR_LOGI("fetch_pitch raw buf size, %d", mem_ptr->cap_raw.buf_size);
        if (frm_ptr->is_4in1_frame) {
            if (flag_loose) {
                buff_forisp14bit =
                    malloc(mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 2);
                isp_raw_buff.addr_y = (cmr_uint)buff_forisp14bit;
                CMR_LOGI("fetch_4ini1_pitch raw buf size, %d", cap_raw_big.buf_size);
                if (isp_raw_buff.addr_y != 0) {
                    raw14bit_process(&mem_ptr->cap_raw.addr_vir, &isp_raw_buff,
                        mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height);
                }
                send_capture_data(raw_format, mem_ptr->cap_raw.size.width,
                          mem_ptr->cap_raw.size.height,
                          (char *)isp_raw_buff.addr_y,
                          cap_raw_big.buf_size, 0, 0, 0, 0);
                free(buff_forisp14bit);
            } else {
                send_capture_data(raw_format, mem_ptr->cap_raw.size.width,
                          mem_ptr->cap_raw.size.height,
                          (char *)mem_ptr->cap_raw.addr_vir.addr_y,
                          cap_raw_big.buf_size, 0, 0, 0, 0);
            }
        }else {
            send_capture_data(raw_format, mem_ptr->cap_raw.size.width,
                          mem_ptr->cap_raw.size.height,
                          (char *)mem_ptr->cap_raw.addr_vir.addr_y,
                          mem_ptr->cap_raw.buf_size, 0, 0, 0, 0);
        }
    }

    if (isp_video_get_raw_images_info()) {
        if (CAMERA_ISP_TUNING_MODE == snp_cxt->req_param.mode) {
            if (frm_ptr->is_4in1_frame) {
                if (flag_loose) {
                    /*
                    dump mipi raw remosaic before
                    snp_cxt->ops.dump_image_with_3a_info(
                        snp_cxt->oem_handle, REMOSAIC_CAM_IMG_FMT_BAYER_MIPI_RAW,
                        mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height,
                        mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 5 / 4,
                        &mem_ptr->cap_raw.addr_vir);
                    */
                    cmr_bzero(&sn_param, sizeof(struct common_sn_cmd_param));
                    sn_param.postproc_info.src = cap_raw_big;
                    sn_param.postproc_info.dst = cap_raw_big;
                    ret = snp_cxt->ops.sensor_ioctl(
                        snp_cxt->oem_handle, COM_SN_GET_4IN1_FORMAT_CONVERT, &sn_param);
                    if (ret) {
                        CMR_LOGE("failed to sensor ioctl");
                    }
                    CMR_LOGD("dump 4in1 raw14bit after remosaic");
                    buff_for14bit =
                        malloc(mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 2);
                    raw_buff.addr_y = (cmr_uint)buff_for14bit;
                    if (raw_buff.addr_y != 0) {
                        raw14bit_process(&mem_ptr->cap_raw.addr_vir, &raw_buff,
                            mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height);
                        CMR_LOGI("raw_process after");
                        snp_cxt->ops.dump_image_with_3a_info(
                            snp_cxt->oem_handle, REMOSAIC_CAM_IMG_FMT_RAW14BIT,
                            mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height,
                            cap_raw_big.buf_size,
                            &raw_buff);
                    }
                    free(buff_for14bit);

                } else {
                    CMR_LOGD("dump mipi raw");
                    snp_cxt->ops.dump_image_with_3a_info(
                        snp_cxt->oem_handle, CAM_IMG_FMT_BAYER_MIPI_RAW,
                        mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height,
                        ((mem_ptr->cap_raw.size.width * 5 / 4 + 3) & ~3) * mem_ptr->cap_raw.size.height,
                        &mem_ptr->cap_raw.addr_vir);
                }

            } else {
                if (flag_loose){
                    CMR_LOGD("dump raw14bit tuning mode");
                    snp_cxt->ops.dump_image_with_3a_info(
                        snp_cxt->oem_handle, CAM_IMG_FMT_RAW14BIT,
                        mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height,
                        mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * 2,
                        &mem_ptr->cap_raw.addr_vir);
                } else {
                    CMR_LOGD("dump mipi raw tuning mode");
                    snp_cxt->ops.dump_image_with_3a_info(
                        snp_cxt->oem_handle, CAM_IMG_FMT_BAYER_MIPI_RAW,
                        mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height,
                        ((mem_ptr->cap_raw.size.width * 5 / 4 + 3) & ~3) * mem_ptr->cap_raw.size.height,
                        &mem_ptr->cap_raw.addr_vir);
                }
            }
        }
    } else {
        CMR_LOGD("dump mipi raw");
        snp_cxt->ops.dump_image_with_3a_info(
            snp_cxt->oem_handle, CAM_IMG_FMT_BAYER_MIPI_RAW,
            mem_ptr->cap_raw.size.width, mem_ptr->cap_raw.size.height,
            ((mem_ptr->cap_raw.size.width * 5 / 4 + 3) & ~3) * mem_ptr->cap_raw.size.height,
            &mem_ptr->cap_raw.addr_vir);
    }
    ret = snp_cxt->ops.raw_proc(snp_cxt->oem_handle, snp_handle, &isp_in_param);
    if (ret) {
        CMR_LOGE("failed to start isp proc %ld", ret);
    }

    chn_param_ptr->isp_process[index].frame_info = *frm_ptr;
    snp_cxt->cvt.out_frame = chn_param_ptr->isp_proc_in[index].dst_frame;

    CMR_LOGD("done %ld", ret);
exit:
    return ret;
}

cmr_int snp_start_isp_next_proc(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct ips_out_param *isp_out_ptr = (struct ips_out_param *)data;
    struct raw_proc_param isp_in_param;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    struct process_status *process_ptr;
    struct img_frm src, dst;
    struct cmr_op_mean mean;
    cmr_u32 index = snp_cxt->index;
    cmr_u32 slice_out, dst_height, dst_width;
    cmr_u32 offset;

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        snp_set_status(snp_handle, IDLE);
        ret = CMR_CAMERA_NORNAL_EXIT;
        return ret;
    }
    if (index >= CMR_CAPTURE_MEM_SUM) {
        CMR_LOGD("index over than CMR_CAPTURE_MEM_SUM %d,index:%d",
                 CMR_CAPTURE_MEM_SUM, index);
        return CMR_CAMERA_FAIL;
    }
    cmr_copy((void *)&isp_in_param, (void *)&chn_param_ptr->isp_proc_in[index],
             sizeof(struct raw_proc_param));
    process_ptr = &chn_param_ptr->isp_process[index];
    process_ptr->slice_height_out += isp_out_ptr->output_height;
    slice_out = process_ptr->slice_height_out;
    dst_width = chn_param_ptr->isp_proc_in[index].dst_frame.size.width;
    dst_height = chn_param_ptr->isp_proc_in[index].dst_frame.size.height;
    chn_param_ptr->isp_process[index].slice_num++;
    if (slice_out == dst_height) {
        CMR_LOGD("isp proc done");
        return CMR_CAMERA_NORNAL_EXIT;
    } else if (slice_out + process_ptr->slice_height_in < dst_height) {
        isp_in_param.src_slice_height = process_ptr->slice_height_in;
    } else {
        isp_in_param.src_slice_height = dst_height - slice_out;
    }
    offset = process_ptr->slice_height_out * dst_width;
    isp_in_param.src_frame.addr_phy.addr_y +=
        ((offset * snp_cxt->sensor_info.sn_interface.pixel_width) >> 3);
    isp_in_param.slice_num = chn_param_ptr->isp_process[index].slice_num;
    isp_in_param.dst_frame.addr_phy.addr_y =
        chn_param_ptr->isp_proc_in[index].dst_frame.addr_phy.addr_y + offset;
    isp_in_param.dst_frame.addr_phy.addr_u =
        chn_param_ptr->isp_proc_in[index].dst_frame.addr_phy.addr_u +
        (offset >> 1);
    if (snp_cxt->ops.raw_proc) {
        ret = snp_cxt->ops.raw_proc(snp_cxt->oem_handle, snp_handle,
                                    &isp_in_param);
        if (ret) {
            CMR_LOGE("failed to start isp proc %ld", ret);
        }
    } else {
        CMR_LOGE("err raw_proc is null");
        ret = -CMR_CAMERA_FAIL;
    }

    CMR_LOGV("X, ret %ld", ret);
    return ret;
}

void snp_wait_cvt_done(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    CMR_LOGD("waiting begin");
    sem_wait(&cxt->cvt.cvt_sync_sm);
    CMR_LOGD("wating end");
}

void snp_cvt_done(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    CMR_MSG_INIT(message);

    sem_post(&cxt->cvt.cvt_sync_sm);
    CMR_LOGV("cvt done");
}

cmr_int snp_start_cvt(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr = (struct frm_info *)data;

    cxt->cur_frame_info = *frm_ptr;
    if (CAM_IMG_FMT_JPEG == frm_ptr->fmt) {
        ret = snp_start_decode_sync(snp_handle, data);
        snp_cvt_done(snp_handle);
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW == frm_ptr->fmt) {
        ret = snp_start_isp_proc(snp_handle, data);
        // for raw capture brinup, skip isp postprocess
        // snp_cvt_done(snp_handle);
    }

    if (ret) {
        CMR_LOGE("failed to start cvt %ld", ret);
        goto exit;
    }
    cxt->index = frm_ptr->frame_id - frm_ptr->base;
exit:
    return ret;
}

cmr_int snp_cvt_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle snp_handle = (cmr_handle)p_data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    // struct frm_info *frm_ptr;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data 0x%lx", message->msg_type,
             (cmr_uint)message->data);
    // frm_ptr = (struct frm_info *) message->data;
    switch (message->msg_type) {
    case SNP_EVT_CVT_INIT:
        break;
    case SNP_EVT_CVT_DEINIT:
        break;
    case SNP_EVT_CVT_START:
        cxt->cvt.trigger = message->sub_msg_type;
        cxt->err_code = snp_start_cvt(snp_handle, message->data);
        snp_wait_cvt_done(snp_handle);
        break;
    case SNP_EVT_CVT_STOP:
        break;
    default:
        CMR_LOGD("don't support msg");
        break;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_write_exif(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct frm_info *frame = (struct frm_info *)data;
    cmr_u32 index = 0;
    struct camera_frame_type frame_type;
    cmr_s32 sm_val = 0;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct camera_frame_type zsl_frame;
    struct img_frm rot_src, scale_src;
    struct snp_jpeg_param *jpeg_in_ptr;
    struct camera_context *cmr_cxt;
    cmr_cxt = (struct camera_context *)cxt->oem_handle;

    if (!data) {
        CMR_LOGE("param error");
        return -CMR_CAMERA_INVALID_PARAM;
    }

    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    cmr_bzero(&zsl_frame, sizeof(struct camera_frame_type));

    ATRACE_BEGIN("sem_wait(scaler_sync_sm)");
    CMR_LOGD("wait scaler_sync_sm");
    sem_wait(&cxt->scaler_sync_sm);
    ATRACE_END();

    ATRACE_BEGIN("sem_wait(jpeg_sync_sm)");
    CMR_LOGD("wait jpeg_sync_sm");
    sem_wait(&cxt->jpeg_sync_sm);
    ATRACE_END();

    CMR_LOGD("cxt->req_param.is_zsl_snapshot=%d",
             cxt->req_param.is_zsl_snapshot);
    CMR_LOGD("cxt->req_param.is_3dnr %d", cxt->req_param.is_3dnr);
    if (cxt->req_param.is_zsl_snapshot) {
        if (cxt->req_param.is_3dnr == 1 || cxt->req_param.is_hdr == 1 ||
            cxt->req_param.is_3dnr == 5) {
            // bokeh + hdr RETURN_SW_ALGORITHM_ZSL_BUF message is in
            // snp_yuv_callback_take_picture_done
            if (cmr_cxt->is_multi_mode != MODE_BOKEH &&
                cmr_cxt->is_multi_mode != MODE_MULTI_CAMERA) {
                CMR_LOGI(
                    "send SNAPSHOT_CB_EVT_RETURN_SW_ALGORITHM_ZSL_BUF here ");
                snp_send_msg_notify_thr(
                    snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                    SNAPSHOT_CB_EVT_RETURN_SW_ALGORITHM_ZSL_BUF, NULL,
                    sizeof(struct camera_frame_type));
            }
        } else if (chn_param_ptr->is_rot) {
            rot_src = chn_param_ptr->rot[0].src_img;
            CMR_LOGD("fd=0x%x", rot_src.fd);
            zsl_frame.fd = rot_src.fd;
            snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_ENCODE_PICTURE,
                                    SNAPSHOT_CB_EVT_RETURN_ZSL_BUF,
                                    (void *)&zsl_frame,
                                    sizeof(struct camera_frame_type));
        } else if (chn_param_ptr->is_scaling) {
            scale_src = chn_param_ptr->scale[0].src_img;
            CMR_LOGD("fd=0x%x", scale_src.fd);
            zsl_frame.fd = scale_src.fd;
            snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_ENCODE_PICTURE,
                                    SNAPSHOT_CB_EVT_RETURN_ZSL_BUF,
                                    (void *)&zsl_frame,
                                    sizeof(struct camera_frame_type));
        } else {
            CMR_LOGD("fd=0x%x", chn_param_ptr->jpeg_in[0].src.fd);
            zsl_frame.fd = chn_param_ptr->jpeg_in[0].src.fd;
            snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_ENCODE_PICTURE,
                                    SNAPSHOT_CB_EVT_RETURN_ZSL_BUF,
                                    (void *)&zsl_frame,
                                    sizeof(struct camera_frame_type));
        }
    }

    if (cxt->ops.channel_free_frame) {
        index += cxt->cur_frame_info.base;
        ret = cxt->ops.channel_free_frame(cxt->oem_handle,
                                          cxt->req_param.channel_id, index);
    }

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    if (cxt->err_code) {
        CMR_LOGE("error exit");
        sem_getvalue(&cxt->proc_done_sm, &sm_val);
        if (0 == sm_val) {
            sem_post(&cxt->proc_done_sm);
        } else {
            CMR_LOGD("cxt->proc_done_sm should be 1, actual is %d ", sm_val);
        }
        send_capture_complete_msg();
        return ret;
    }

    jpeg_in_ptr = &chn_param_ptr->jpeg_in[0];
    cmr_snapshot_invalidate_cache(cmr_cxt->snp_cxt.snapshot_handle,
                                  &jpeg_in_ptr->dst);

    if (cxt->ops.start_exif_encode == NULL) {
        CMR_LOGE("start_exif_encode is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    index = cxt->index;
    struct camera_jpeg_param enc_param;
    struct snp_exif_param *exif_in_ptr = &cxt->chn_param.exif_in[index];
    struct img_size image_size = cxt->req_param.req_size;
    struct jpeg_wexif_cb_param enc_out_param;
    memset(&enc_param, 0, sizeof(struct camera_jpeg_param));
    snp_set_status(snp_handle, WRITE_EXIF);
    exif_in_ptr->big_pic_stream_src.buf_size = cxt->jpeg_stream_size;
    exif_in_ptr->thumb_stream_src.buf_size = cxt->thumb_stream_size;
    camera_take_snapshot_step(CMR_STEP_WR_EXIF_S);
    ATRACE_BEGIN("start_exif_encode");
    ret = cxt->ops.start_exif_encode(cxt->oem_handle, snp_handle,
                                     &exif_in_ptr->big_pic_stream_src,
                                     &exif_in_ptr->thumb_stream_src, NULL,
                                     &exif_in_ptr->dst, &enc_out_param);
    if (ret) {
        CMR_LOGE("start_exif_encode failed, ret=%ld", ret);
    }
    ATRACE_END();
    camera_take_snapshot_step(CMR_STEP_WR_EXIF_E);
    sem_post(&cxt->writer_exif_sm);
    snp_set_status(snp_handle, POST_PROCESSING);
    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("snp has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }
    enc_param.need_free = 0;
    enc_param.index = index;
    enc_param.outPtr = (void *)enc_out_param.output_buf_virt_addr;
    enc_param.size = enc_out_param.output_buf_size;
    if (0 == snp_get_request(snp_handle) ||
        (cxt->cap_cnt == cxt->req_param.total_num)) {
        enc_param.need_free = 1;
    }
    CMR_LOGD("need free %d stream %d cap cnt %d total num %d",
             enc_param.need_free, enc_param.size, cxt->cap_cnt,
             cxt->req_param.total_num);
#ifdef SUPER_MACRO
    /* copy super info to camera_jpeg_param->super */
    if (jpeg_in_ptr->super.addr_vir.addr_y != 0 && cxt->req_param.is_super) {
        enc_param.super.addr = jpeg_in_ptr->super.addr_vir.addr_y;
        enc_param.super.width = jpeg_in_ptr->super.size.width;
        enc_param.super.height = jpeg_in_ptr->super.size.height;
        enc_param.super.size = jpeg_in_ptr->super.buf_size + sizeof(int)*3;

        CMR_LOGV("super: addr 0x%x, (%d, %d), size %d", enc_param.super.addr, enc_param.super.width,
            enc_param.super.height, enc_param.super.size);
    }
#endif
    camera_take_snapshot_step(CMR_STEP_CALL_BACK);
    // just for perf tuning
    // camera_snapshot_step_statisic(&image_size);
    frame_type.timestamp = frame->sec * 1000000000LL + frame->usec * 1000;
    frame_type.monoboottime = frame->monoboottime;
    memcpy((void *)&frame_type.jpeg_param, (void *)&enc_param,
           sizeof(struct camera_jpeg_param));
    snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_ENCODE_PICTURE,
                            SNAPSHOT_CB_EVT_DONE, (void *)&frame_type,
                            sizeof(struct camera_frame_type));

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("snp has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    {
        struct img_addr jpeg_addr;
        char value[PROPERTY_VALUE_MAX];
        struct camera_context *cam_ctx = cxt->oem_handle;
        cmr_bzero((void *)&jpeg_addr, sizeof(jpeg_addr));
        property_get("debug.camera.save.snpfile", value, "0");
        if (atoi(value) == 8 || atoi(value) == 100 || atoi(value) & (1 << 8)) {
            jpeg_addr.addr_y = enc_out_param.output_buf_virt_addr;
            dump_image("snp_write_exif", CAM_IMG_FMT_JPEG,
                       cxt->req_param.post_proc_setting.actual_snp_size.width,
                       cxt->req_param.post_proc_setting.actual_snp_size.height,
                       FORM_DUMPINDEX(SNP_JPEG_STREAM, cam_ctx->dump_cnt, 0),
                       &jpeg_addr, enc_out_param.output_buf_size);
        }
        CMR_LOGV("debug.camera.save.snpfile 0x%x\n", atoi(value));
        if (atoi(value) & 0x3ff) {
            cam_ctx->dump_cnt++;
            CMR_LOGD("dump_cnt 0x%lx\n", cam_ctx->dump_cnt);
        }
    }

    if (!ret) {
        snp_post_proc_done(snp_handle);
    }

exit:
    send_capture_complete_msg();
    if (ret) {
        snp_post_proc_err_exit(snp_handle, ret);
    }
    CMR_LOGD("x, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int snp_write_exif_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle snp_handle = (cmr_handle)p_data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct frm_info *frm_ptr;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data 0x%lx", message->msg_type,
             (cmr_uint)message->data);
    switch (message->msg_type) {
    case SNP_EVT_WRITE_EXIF_INIT:
        break;
    case SNP_EVT_WRITE_EXIF_DEINIT:
        break;
    case SNP_EVT_WRITE_EXIF_START:
        ret = snp_write_exif(snp_handle, message->data);
        break;
    case SNP_EVT_WRITE_EXIF_STOP:
        break;
    default:
        CMR_LOGD("don't support msg");
        break;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_redisplay_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle snp_handle = (cmr_handle)p_data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    //struct frm_info *frm_ptr;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data 0x%lx", message->msg_type,
             (cmr_uint)message->data);
    switch (message->msg_type) {
    case SNP_EVT_REDISPLAY_START:
        //frm_ptr = (struct frm_info *)message->data;
        ret = snp_take_picture_done(snp_handle, message->data);
        break;
    default:
        CMR_LOGD("don't support msg");
        break;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_thumb_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_handle snp_handle = (cmr_handle)p_data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    //struct frm_info frm_ptr;

    if (!message || !p_data) {
        CMR_LOGE("param error");
        goto exit;
    }
    CMR_LOGD("message.msg_type 0x%x, data 0x%lx", message->msg_type,
             (cmr_uint)message->data);
    switch (message->msg_type) {
    case SNP_EVT_THUMB_START:
        //frm_ptr = *(struct frm_info *)message->data;
        ret = snp_thumbnail(snp_handle, message->data);
        break;
    default:
        CMR_LOGD("don't support msg");
        break;
    }
exit:
    CMR_LOGD("done %ld", ret);
    return ret;
}

cmr_int snp_create_main_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret =
        cmr_thread_create(&cxt->thread_cxt.main_thr_handle, SNP_MSG_QUEUE_SIZE,
                          snp_main_thread_proc, (void *)snp_handle,
                          "snp_main");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.main_thr_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create main thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_main_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;
    CMR_MSG_INIT(message);

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->main_thr_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->main_thr_handle);
        if (!ret) {
            snp_thread_cxt->main_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy main thread %ld", ret);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_postproc_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.post_proc_thr_handle,
                            SNP_MSG_QUEUE_SIZE, snp_postproc_thread_proc,
                            (void *)snp_handle, "snp_post");

    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.post_proc_thr_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create post proc thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_postproc_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;
    CMR_MSG_INIT(message);

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->post_proc_thr_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->post_proc_thr_handle);
        if (!ret) {
            snp_thread_cxt->post_proc_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy post proc thread %ld", ret);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_notify_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.notify_thr_handle,
                            SNP_MSG_QUEUE_SIZE, snp_notify_thread_proc,
                            (void *)snp_handle, "snp_notify");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.notify_thr_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create notify thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_notify_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->notify_thr_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->notify_thr_handle);
        if (!ret) {
            snp_thread_cxt->notify_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy notify thread %ld", ret);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_proc_cb_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.proc_cb_thr_handle,
                            SNP_MSG_QUEUE_SIZE, snp_proc_cb_thread_proc,
                            (void *)snp_handle, "snp_proc_cb");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.proc_cb_thr_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create proc cb thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_proc_cb_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->proc_cb_thr_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->proc_cb_thr_handle);
        if (!ret) {
            snp_thread_cxt->proc_cb_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy proc cb %ld", ret);
        }
    }
exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_secondary_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.secondary_thr_handle,
                            SNP_MSG_QUEUE_SIZE, snp_secondary_thread_proc,
                            (void *)snp_handle, "snp_sec");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.secondary_thr_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create secondary thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_secondary_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->secondary_thr_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->secondary_thr_handle);
        if (!ret) {
            snp_thread_cxt->secondary_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy secondary %ld", ret);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_cvt_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.cvt_thr_handle, SNP_MSG_QUEUE_SIZE,
                            snp_cvt_thread_proc, (void *)snp_handle, "snp_cvt");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.cvt_thr_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create cvt thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_cvt_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;
    CMR_MSG_INIT(message);

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;

    if (snp_thread_cxt->cvt_thr_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->cvt_thr_handle);
        if (!ret) {
            snp_thread_cxt->cvt_thr_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy cvt thread %ld", ret);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_write_exif_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.write_exif_thr_handle,
                            SNP_MSG_QUEUE_SIZE, snp_write_exif_thread_proc,
                            (void *)snp_handle, "snp_exif");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.write_exif_thr_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create write exif thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    } else {
        sem_init(&cxt->thread_cxt.writte_exif_access_sm, 0, 1);
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_write_exif_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    sem_wait(&cxt->thread_cxt.writte_exif_access_sm);
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->write_exif_thr_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->write_exif_thr_handle);
        if (!ret) {
            snp_thread_cxt->write_exif_thr_handle = (cmr_handle)0;
            sem_post(&cxt->thread_cxt.writte_exif_access_sm);
            sem_destroy(&cxt->thread_cxt.writte_exif_access_sm);
        } else {
            sem_post(&cxt->thread_cxt.writte_exif_access_sm);
            CMR_LOGE("failed to destroy write exif %ld", ret);
        }
    } else {
        sem_post(&cxt->thread_cxt.writte_exif_access_sm);
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_redisplay_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.proc_redisplay_handle,
                            SNP_MSG_QUEUE_SIZE, snp_redisplay_thread_proc,
                            (void *)snp_handle, "snp_redisplay");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.proc_redisplay_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create redisplay thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_redisplay_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->proc_redisplay_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->proc_redisplay_handle);
        if (!ret) {
            snp_thread_cxt->proc_redisplay_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy redisplay %ld", ret);
        }
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_thumb_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = cmr_thread_create(&cxt->thread_cxt.proc_thumb_handle,
                            SNP_MSG_QUEUE_SIZE, snp_thumb_thread_proc,
                            (void *)snp_handle, "snp_thumb");
    CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.proc_thumb_handle);
    if (CMR_MSG_SUCCESS != ret) {
        CMR_LOGE("failed to create thumb thread %ld", ret);
        ret = -CMR_CAMERA_NO_SUPPORT;
        goto exit;
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_destroy_thumb_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_thread_context *snp_thread_cxt;

    if (!snp_handle) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    snp_thread_cxt = &cxt->thread_cxt;
    if (snp_thread_cxt->proc_redisplay_handle) {
        ret = cmr_thread_destroy(snp_thread_cxt->proc_thumb_handle);
        if (!ret) {
            snp_thread_cxt->proc_thumb_handle = (cmr_handle)0;
        } else {
            CMR_LOGE("failed to destroy thumb %ld", ret);
        }
    }
exit:
    CMR_LOGD("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_create_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGV("E");

    ret = snp_create_main_thread(snp_handle);
    if (ret) {
        goto exit;
    }
    ret = snp_create_postproc_thread(snp_handle);
    if (ret) {
        goto destroy_main_thr;
    }
    ret = snp_create_notify_thread(snp_handle);
    if (ret) {
        goto destroy_postproc_thr;
    }
    ret = snp_create_proc_cb_thread(snp_handle);
    if (ret) {
        goto destroy_notify_thr;
    }
    ret = snp_create_secondary_thread(snp_handle);
    if (ret) {
        goto destroy_proc_cb_thr;
    }
    ret = snp_create_cvt_thread(snp_handle);
    if (ret) {
        goto destroy_sencondary_thr;
    }
    ret = snp_create_redisplay_thread(snp_handle);
    if (ret) {
        goto destroy_cvt_thr;
    }
    ret = snp_create_thumb_thread(snp_handle);
    if (ret) {
        goto destroy_redisplay_thr;
    }
    ret = snp_create_write_exif_thread(snp_handle);
    if (ret) {
        goto destroy_thumb_thr;
    } else {
        goto exit;
    }
destroy_thumb_thr:
    snp_destroy_thumb_thread(snp_handle);
destroy_redisplay_thr:
    snp_destroy_redisplay_thread(snp_handle);
destroy_cvt_thr:
    snp_destroy_write_exif_thread(snp_handle);
destroy_sencondary_thr:
    snp_destroy_secondary_thread(snp_handle);
destroy_proc_cb_thr:
    snp_destroy_proc_cb_thread(snp_handle);
destroy_notify_thr:
    snp_destroy_notify_thread(snp_handle);
destroy_postproc_thr:
    snp_destroy_postproc_thread(snp_handle);
destroy_main_thr:
    snp_destroy_main_thread(snp_handle);
exit:
    CMR_LOGV("X, ret = %ld", ret);
    return ret;
}

cmr_int snp_destroy_thread(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGV("E");

    ret = snp_destroy_write_exif_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy write exif thread %ld", ret);
    }
    ret = snp_destroy_thumb_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy thumb thread %ld", ret);
    }
    ret = snp_destroy_redisplay_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy redisplay thread %ld", ret);
    }
    ret = snp_destroy_cvt_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy cvt thread %ld", ret);
    }
    ret = snp_destroy_secondary_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy secondary thread %ld", ret);
    }
    ret = snp_destroy_proc_cb_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy proc cb thread %ld", ret);
    }
    ret = snp_destroy_notify_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy notify thread %ld", ret);
    }
    ret = snp_destroy_postproc_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy post proc thread %ld", ret);
    }
    ret = snp_destroy_main_thread(snp_handle);
    if (ret) {
        CMR_LOGE("failed to destroy main thread %ld", ret);
    }
    CMR_LOGV("X");
    return ret;
}

void snp_local_init(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    sem_init(&cxt->access_sm, 0, 1);
    sem_init(&cxt->proc_done_sm, 0, 1);
    sem_init(&cxt->cvt.cvt_sync_sm, 0, 0);
    sem_init(&cxt->takepic_callback_sem, 0, 0);
    sem_init(&cxt->jpeg_sync_sm, 0, 0);
    sem_init(&cxt->hdr_sync_sm, 0, 0);
    sem_init(&cxt->scaler_sync_sm, 0, 0);
    sem_init(&cxt->redisplay_sm, 0, 0);
    sem_init(&cxt->writer_exif_sm, 0, 0);
    sem_init(&cxt->ipm_sync_sm, 0, 1);
    sem_init(&cxt->pre_start_encode_sync_sm, 0, 1);
}

void snp_local_deinit(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    sem_destroy(&cxt->access_sm);
    sem_destroy(&cxt->proc_done_sm);
    sem_destroy(&cxt->cvt.cvt_sync_sm);
    sem_destroy(&cxt->takepic_callback_sem);
    sem_destroy(&cxt->jpeg_sync_sm);
    sem_destroy(&cxt->scaler_sync_sm);
    sem_destroy(&cxt->hdr_sync_sm);
    sem_destroy(&cxt->redisplay_sm);
    sem_destroy(&cxt->writer_exif_sm);
    sem_destroy(&cxt->ipm_sync_sm);
    sem_destroy(&cxt->pre_start_encode_sync_sm);
    cxt->is_inited = 0;
}

cmr_int snp_check_post_proc_param(struct snapshot_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct img_size actual_size;
    struct img_size snp_size;
    struct img_frm chn_out;
    struct img_frm rot_frm;
    struct snp_proc_param *proc_param_ptr;
    cmr_u32 channel_id = param_ptr->channel_id;

    proc_param_ptr = &param_ptr->post_proc_setting;
    actual_size = proc_param_ptr->actual_snp_size;
    if (0 == actual_size.width || 0 == actual_size.height) {
        CMR_LOGE("err, actual size is zero");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (proc_param_ptr->cap_org_size.width == 0 ||
        proc_param_ptr->cap_org_size.height == 0) {
        CMR_LOGW("cap_org_size[%d %d], set as [%d %d]",
                proc_param_ptr->cap_org_size.width,
                proc_param_ptr->cap_org_size.height,
                proc_param_ptr->actual_snp_size.width,
                proc_param_ptr->actual_snp_size.height);
        proc_param_ptr->cap_org_size = proc_param_ptr->actual_snp_size;
    }
    snp_size = proc_param_ptr->snp_size;
    if (0 == snp_size.width || 0 == snp_size.height) {
        CMR_LOGE("err, snp size is zero");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (param_ptr->channel_id >= GRAB_CHANNEL_MAX) {
        CMR_LOGE("err, channel id %d", param_ptr->channel_id);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_set_channel_out_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *channel_param_ptr = &cxt->req_param;
    struct img_frm *chn_out_frm_ptr;
    cmr_uint i;

    chn_out_frm_ptr = &channel_param_ptr->post_proc_setting.chn_out_frm[0];
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        if (0 == chn_out_frm_ptr->size.width ||
            0 == chn_out_frm_ptr->size.height) {
            CMR_LOGE("err, frm size %d %d", chn_out_frm_ptr->size.width,
                     chn_out_frm_ptr->size.height);
            ret = -CMR_CAMERA_INVALID_PARAM;
            break;
        }
        if ((0 == chn_out_frm_ptr->addr_vir.addr_y ||
             0 == chn_out_frm_ptr->addr_vir.addr_u) ||
            (0 == chn_out_frm_ptr->fd)) {
            CMR_LOGE("err, frm fd 0x%x addr_vir 0x%lx 0x%lx",
                     chn_out_frm_ptr->fd, chn_out_frm_ptr->addr_vir.addr_y,
                     chn_out_frm_ptr->addr_vir.addr_u);
            ret = -CMR_CAMERA_INVALID_PARAM;
            break;
        }
        chn_out_frm_ptr++;
    }
    if (CMR_CAMERA_SUCCESS == ret) {
        chn_out_frm_ptr = &channel_param_ptr->post_proc_setting.chn_out_frm[0];
        CMR_LOGD("data format %d", chn_out_frm_ptr->fmt);
        if (CAM_IMG_FMT_JPEG != chn_out_frm_ptr->fmt) {
            if (CAM_IMG_FMT_BAYER_MIPI_RAW != chn_out_frm_ptr->fmt) {
                cmr_copy((void *)&cxt->chn_param.chn_frm[0],
                         (void *)chn_out_frm_ptr,
                         CMR_CAPTURE_MEM_SUM * sizeof(struct img_frm));
            } else {
                if (CAMERA_AUTOTEST_MODE != cxt->req_param.mode) {
                    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                        cxt->chn_param.chn_frm[i] =
                            cxt->chn_param.isp_proc_in[i].dst_frame;
                        cxt->chn_param.chn_frm[i].fmt = CAM_IMG_FMT_YUV420_NV21;
                    }
                } else {
                    cmr_copy((void *)&cxt->chn_param.chn_frm[0],
                             (void *)chn_out_frm_ptr,
                             CMR_CAPTURE_MEM_SUM * sizeof(struct img_frm));
                }
            }
        } else {
            for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                cxt->chn_param.chn_frm[i] = cxt->chn_param.jpeg_dec_in[i].dst;
            }
        }
    }
    if (!ret) {
        chn_out_frm_ptr = &cxt->chn_param.chn_frm[0];
        for (i = 0; i < 1 /*CMR_CAPTURE_MEM_SUM*/; i++) {
            CMR_LOGD("phy fd 0x%x vir addr 0x%lx size %d %d",
                     chn_out_frm_ptr->fd, chn_out_frm_ptr->addr_vir.addr_u,
                     chn_out_frm_ptr->size.width, chn_out_frm_ptr->size.height);
        }
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_set_hdr_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    cmr_copy((void *)&cxt->chn_param.hdr_src_frm[0],
             (void *)&cxt->chn_param.chn_frm[0],
             sizeof(cxt->chn_param.hdr_src_frm));

    return ret;
}

cmr_int snp_set_ipm_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_ipm_param *ipm_ptr = &chn_param_ptr->ipm[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        ipm_ptr->src = req_param_ptr->post_proc_setting.mem[i].target_yuv;
        // ipm_ptr->src.size = req_param_ptr->post_proc_setting.actual_snp_size;
        // [4in1 && zsl]scale after post process
        ipm_ptr->src.size = req_param_ptr->post_proc_setting.cap_org_size;
        ipm_ptr++;
    }
    ipm_ptr = &chn_param_ptr->ipm[0];

    CMR_LOGD("src addr 0x%lx 0x%lx ", ipm_ptr->src.addr_phy.addr_y,
             ipm_ptr->src.addr_phy.addr_u);
    CMR_LOGD("src size %d %d ", ipm_ptr->src.size.width,
             ipm_ptr->src.size.height);
    return ret;
}

cmr_int snp_set_scale_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct img_rect rect[CMR_CAPTURE_MEM_SUM];
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct img_frm *frm_ptr;
    cmr_uint i;
    cmr_u32 offset[CMR_CAPTURE_MEM_SUM] = {0};

    cmr_copy(&rect[0], &req_param_ptr->post_proc_setting.scaler_src_rect[0],
             CMR_CAPTURE_MEM_SUM * sizeof(struct img_rect));

    CMR_LOGD("scaler_src_rect %d %d %d %d", rect[0].start_x, rect[0].start_y,
             rect[0].width, rect[0].height);

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        chn_param_ptr->scale[i].slice_height = rect[i].height;
        chn_param_ptr->scale[i].src_img.fmt = CAM_IMG_FMT_YUV420_NV21;
        chn_param_ptr->scale[i].dst_img.fmt = CAM_IMG_FMT_YUV420_NV21;
        chn_param_ptr->scale[i].src_img.data_end =
            req_param_ptr->post_proc_setting.data_endian;
        chn_param_ptr->scale[i].dst_img.data_end =
            req_param_ptr->post_proc_setting.data_endian;
        chn_param_ptr->scale[i].src_img.rect = rect[i];
        chn_param_ptr->scale[i].dst_img.size =
            req_param_ptr->post_proc_setting.actual_snp_size;
        chn_param_ptr->scale[i].dst_img.fd =
            req_param_ptr->post_proc_setting.mem[i].target_yuv.fd;
        cmr_copy((void *)&chn_param_ptr->scale[i].dst_img.addr_phy,
                 (void *)&req_param_ptr->post_proc_setting.mem[i]
                     .target_yuv.addr_phy,
                 sizeof(struct img_addr));
        cmr_copy((void *)&chn_param_ptr->scale[i].dst_img.addr_vir,
                 (void *)&req_param_ptr->post_proc_setting.mem[i]
                     .target_yuv.addr_vir,
                 sizeof(struct img_addr));
    }

    if (IMG_ANGLE_0 == req_param_ptr->post_proc_setting.rot_angle ||
        (cxt->req_param.is_cfg_rot_cap &&
         (IMG_ANGLE_0 == cxt->req_param.rot_angle))) {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            chn_param_ptr->scale[i].src_img.addr_phy.addr_y =
                chn_param_ptr->chn_frm[i].addr_phy.addr_y + offset[i];
            chn_param_ptr->scale[i].src_img.addr_phy.addr_u =
                chn_param_ptr->chn_frm[i].addr_phy.addr_u + (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.addr_phy.addr_v =
                chn_param_ptr->chn_frm[i].addr_phy.addr_v + (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.size =
                req_param_ptr->post_proc_setting.chn_out_frm[i].size;
            chn_param_ptr->scale[i].src_img.addr_vir.addr_y =
                chn_param_ptr->chn_frm[i].addr_vir.addr_y + offset[i];
            chn_param_ptr->scale[i].src_img.addr_vir.addr_u =
                chn_param_ptr->chn_frm[i].addr_vir.addr_u + (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.addr_vir.addr_v =
                chn_param_ptr->chn_frm[i].addr_vir.addr_v + (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.fd = chn_param_ptr->chn_frm[i].fd;
        }
    } else { // rotation case
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            chn_param_ptr->scale[i].src_img.addr_phy.addr_y =
                chn_param_ptr->rot[i].dst_img.addr_phy.addr_y + offset[i];
            chn_param_ptr->scale[i].src_img.addr_phy.addr_u =
                chn_param_ptr->rot[i].dst_img.addr_phy.addr_u +
                (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.addr_phy.addr_v =
                chn_param_ptr->rot[i].dst_img.addr_phy.addr_v +
                (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.size =
                chn_param_ptr->rot[i].dst_img.size;
            chn_param_ptr->scale[i].src_img.addr_vir.addr_y =
                chn_param_ptr->rot[i].dst_img.addr_phy.addr_y + offset[i];
            chn_param_ptr->scale[i].src_img.addr_vir.addr_u =
                chn_param_ptr->rot[i].dst_img.addr_phy.addr_u +
                (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.addr_vir.addr_v =
                chn_param_ptr->rot[i].dst_img.addr_phy.addr_v +
                (offset[i] >> 1);
            chn_param_ptr->scale[i].src_img.fd =
                chn_param_ptr->rot[i].dst_img.fd;
        }
    }

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        CMR_LOGD("src addr 0x%lx 0x%lx dst add 0x%lx 0x%lx fd 0x%x",
                 chn_param_ptr->scale[i].src_img.addr_phy.addr_y,
                 chn_param_ptr->scale[i].src_img.addr_phy.addr_u,
                 chn_param_ptr->scale[i].dst_img.addr_phy.addr_y,
                 chn_param_ptr->scale[i].dst_img.addr_phy.addr_u,
                 chn_param_ptr->scale[i].dst_img.fd);

        CMR_LOGD("src size %d %d dst size %d %d",
                 chn_param_ptr->scale[i].src_img.size.width,
                 chn_param_ptr->scale[i].src_img.size.height,
                 chn_param_ptr->scale[i].dst_img.size.width,
                 chn_param_ptr->scale[i].dst_img.size.height);
    }
    return ret;
}

cmr_int snp_set_convert_thumb_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct img_rect rect;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_scale_param *thumb_ptr = &cxt->chn_param.convert_thumb[0];
    struct jpeg_param *jpeg_ptr = &req_param_ptr->jpeg_setting;
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        thumb_ptr->src_img = req_param_ptr->post_proc_setting.mem[i].target_yuv;
        thumb_ptr->dst_img = req_param_ptr->post_proc_setting.mem[i].thum_yuv;
        thumb_ptr->src_img.size =
            req_param_ptr->post_proc_setting.actual_snp_size;
        thumb_ptr->src_img.rect.start_x = 0;
        thumb_ptr->src_img.rect.start_y = 0;
        thumb_ptr->src_img.rect.width = thumb_ptr->src_img.size.width;
        thumb_ptr->src_img.rect.height = thumb_ptr->src_img.size.height;
        thumb_ptr->src_img.data_end =
            req_param_ptr->post_proc_setting.data_endian;
        thumb_ptr->dst_img.data_end =
            req_param_ptr->post_proc_setting.data_endian;
        thumb_ptr->src_img.fmt = CAM_IMG_FMT_YUV420_NV21;
        thumb_ptr->dst_img.fmt = CAM_IMG_FMT_YUV420_NV21;
        thumb_ptr++;
    }
    thumb_ptr = &cxt->chn_param.convert_thumb[0];
    if (req_param_ptr->is_cfg_rot_cap &&
        (IMG_ANGLE_90 == jpeg_ptr->set_encode_rotation ||
         IMG_ANGLE_270 == jpeg_ptr->set_encode_rotation)) {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            thumb_ptr->dst_img.size.width = jpeg_ptr->thum_size.height;
            thumb_ptr->dst_img.size.height = jpeg_ptr->thum_size.width;
            thumb_ptr->slice_height = thumb_ptr->dst_img.size.height;
            thumb_ptr++;
        }
    } else {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            thumb_ptr->dst_img.size = jpeg_ptr->thum_size;
            thumb_ptr->slice_height = thumb_ptr->dst_img.size.height;
            thumb_ptr++;
        }
    }
    thumb_ptr = &cxt->chn_param.convert_thumb[0];
    CMR_LOGD("src addr 0x%lx 0x%lx dst addr 0x%lx 0x%lx, fd 0x%x",
             thumb_ptr->src_img.addr_phy.addr_y,
             thumb_ptr->src_img.addr_phy.addr_u,
             thumb_ptr->dst_img.addr_phy.addr_y,
             thumb_ptr->dst_img.addr_phy.addr_u, thumb_ptr->dst_img.fd);
    CMR_LOGD("src size %d %d dst size %d %d slice height %d",
             thumb_ptr->src_img.size.width, thumb_ptr->src_img.size.height,
             thumb_ptr->dst_img.size.width, thumb_ptr->dst_img.size.height,
             (cmr_u32)thumb_ptr->slice_height);
    thumb_ptr++;
    return ret;
}

cmr_int snp_set_rot_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_rot_param *rot_ptr = &cxt->chn_param.rot[0];
    cmr_uint i;

    if (IMG_ANGLE_0 == req_param_ptr->post_proc_setting.rot_angle) {
        CMR_LOGD("don't need to rotate");
        goto exit;
    }
    chn_param_ptr->is_rot = 1;
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        if (CAM_IMG_FMT_JPEG !=
            req_param_ptr->post_proc_setting.chn_out_frm[i].fmt) {
            rot_ptr->src_img = chn_param_ptr->chn_frm[i];
        } else {
            rot_ptr->src_img = chn_param_ptr->jpeg_dec_in[i].dst;
        }
        rot_ptr->angle = req_param_ptr->post_proc_setting.rot_angle;
        rot_ptr++;
    }
    rot_ptr = &cxt->chn_param.rot[0];
    if (chn_param_ptr->is_scaling) {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            rot_ptr->dst_img = req_param_ptr->post_proc_setting.mem[i].cap_yuv;
            rot_ptr++;
        }
    } else {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            rot_ptr->dst_img =
                req_param_ptr->post_proc_setting.mem[i].target_yuv;
            rot_ptr++;
        }
    }
    rot_ptr = &cxt->chn_param.rot[0];
    if (IMG_ANGLE_90 == req_param_ptr->post_proc_setting.rot_angle ||
        IMG_ANGLE_270 == req_param_ptr->post_proc_setting.rot_angle) {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            rot_ptr->dst_img.size.width =
                req_param_ptr->post_proc_setting.chn_out_frm[i].size.height;
            rot_ptr->dst_img.size.height =
                req_param_ptr->post_proc_setting.chn_out_frm[i].size.width;
            rot_ptr++;
        }
    } else {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            rot_ptr->dst_img.size =
                req_param_ptr->post_proc_setting.chn_out_frm[i].size;
            rot_ptr++;
        }
    }
    rot_ptr = &cxt->chn_param.rot[0];
    CMR_LOGD("src addr 0x%lx 0x%lx dst 0x%lx 0x%lx, fd 0x%x",
             rot_ptr->src_img.addr_phy.addr_y, rot_ptr->src_img.addr_phy.addr_u,
             rot_ptr->dst_img.addr_phy.addr_y, rot_ptr->dst_img.addr_phy.addr_u,
             rot_ptr->dst_img.fd);
    CMR_LOGD("src size %d %d dst size %d %d", rot_ptr->src_img.size.width,
             rot_ptr->src_img.size.height, rot_ptr->dst_img.size.width,
             rot_ptr->dst_img.size.height);
    rot_ptr++;
exit:
    return ret;
}

cmr_int snp_set_jpeg_enc_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_jpeg_param *jpeg_ptr = &chn_param_ptr->jpeg_in[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        jpeg_ptr->src = req_param_ptr->post_proc_setting.mem[i].target_yuv;
        jpeg_ptr->dst = req_param_ptr->post_proc_setting.mem[i].target_jpeg;
        jpeg_ptr->src.size = req_param_ptr->post_proc_setting.actual_snp_size;
        jpeg_ptr->super = req_param_ptr->post_proc_setting.mem[i].super_macro;
        if (req_param_ptr->is_cfg_rot_cap &&
            (IMG_ANGLE_0 != req_param_ptr->jpeg_setting.set_encode_rotation) &&
            (IMG_ANGLE_180 !=
             req_param_ptr->jpeg_setting.set_encode_rotation)) {
            jpeg_ptr->dst.size.width = req_param_ptr->req_size.height;
            jpeg_ptr->dst.size.height = req_param_ptr->req_size.width;
        } else {
            jpeg_ptr->dst.size = req_param_ptr->req_size;
        }
        jpeg_ptr->dst.size =
            req_param_ptr->post_proc_setting.dealign_actual_snp_size;
        jpeg_ptr->mean.is_sync = 0;
        jpeg_ptr->mean.is_thumb = 0;
        jpeg_ptr->mean.slice_num = 1;
        jpeg_ptr->mean.quality_level = req_param_ptr->jpeg_setting.quality;
        jpeg_ptr->mean.slice_height =
            req_param_ptr->post_proc_setting.actual_snp_size.height;
        jpeg_ptr->mean.slice_mode = JPEG_YUV_SLICE_ONE_BUF;
        if (req_param_ptr->is_cfg_rot_cap) {
            jpeg_ptr->mean.rot = 0;
        } else {
            jpeg_ptr->mean.rot =
                req_param_ptr->jpeg_setting.set_encode_rotation;
        }
        jpeg_ptr++;
    }
    jpeg_ptr = &chn_param_ptr->jpeg_in[0];
    CMR_LOGD("src addr 0x%lx 0x%lx dst 0x%lx, fd 0x%x",
             jpeg_ptr->src.addr_phy.addr_y, jpeg_ptr->src.addr_phy.addr_u,
             jpeg_ptr->dst.addr_phy.addr_y, jpeg_ptr->dst.fd);
    CMR_LOGD("src size %d %d out size %d %d", jpeg_ptr->src.size.width,
             jpeg_ptr->src.size.height, jpeg_ptr->dst.size.width,
             jpeg_ptr->dst.size.height);
    CMR_LOGD("quality %d slice height %d", jpeg_ptr->mean.quality_level,
             jpeg_ptr->mean.slice_height);
    return ret;
}

cmr_int snp_set_jpeg_thumb_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_jpeg_param *jpeg_ptr = &chn_param_ptr->thumb_in[0];
    struct snp_scale_param *thumb_ptr = &cxt->chn_param.convert_thumb[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        if ((thumb_ptr->src_img.size.width != thumb_ptr->dst_img.size.width) ||
            (thumb_ptr->src_img.size.height !=
             thumb_ptr->dst_img.size.height)) {
            jpeg_ptr->src = req_param_ptr->post_proc_setting.mem[i].thum_yuv;
        } else {
            jpeg_ptr->src = req_param_ptr->post_proc_setting.mem[i].target_yuv;
        }
        jpeg_ptr->dst = req_param_ptr->post_proc_setting.mem[i].thum_jpeg;
        jpeg_ptr->src.size = cxt->chn_param.convert_thumb[i].dst_img.size;
        jpeg_ptr->mean.is_sync = 1;
        jpeg_ptr->mean.is_thumb = 1;
        jpeg_ptr->mean.quality_level =
            req_param_ptr->jpeg_setting.thumb_quality;
        jpeg_ptr->mean.slice_height = jpeg_ptr->src.size.height;
        jpeg_ptr->mean.slice_mode = JPEG_YUV_SLICE_ONE_BUF;
        jpeg_ptr->src.rect.start_x = 0;
        jpeg_ptr->src.rect.start_y = 0;
        jpeg_ptr->src.rect.width = jpeg_ptr->src.size.width;
        jpeg_ptr->src.rect.height = jpeg_ptr->src.size.height;
        jpeg_ptr->dst.size = jpeg_ptr->src.size;
        if (req_param_ptr->is_cfg_rot_cap) {
            jpeg_ptr->mean.rot = 0;
        } else {
            jpeg_ptr->mean.rot =
                req_param_ptr->jpeg_setting.set_encode_rotation;
        }
        jpeg_ptr++;
        thumb_ptr++;
    }
    jpeg_ptr = &chn_param_ptr->thumb_in[0];
    CMR_LOGD("src addr 0x%lx 0x%lx dst 0x%lx, fd 0x%x",
             jpeg_ptr->src.addr_phy.addr_y, jpeg_ptr->src.addr_phy.addr_u,
             jpeg_ptr->dst.addr_phy.addr_y, jpeg_ptr->dst.fd);
    CMR_LOGD("src size %d %d out size %d %d", jpeg_ptr->src.size.width,
             jpeg_ptr->src.size.height, jpeg_ptr->dst.size.width,
             jpeg_ptr->dst.size.height);
    CMR_LOGD("quality %d slice height %d", jpeg_ptr->mean.quality_level,
             jpeg_ptr->mean.slice_height);
    return ret;
}

cmr_int snp_set_jpeg_exif_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_exif_param *exif_in_ptr = &chn_param_ptr->exif_in[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        exif_in_ptr->big_pic_stream_src = chn_param_ptr->jpeg_in[i].dst;
        exif_in_ptr->thumb_stream_src = chn_param_ptr->thumb_in[i].dst;
        exif_in_ptr->dst.addr_vir.addr_y =
            exif_in_ptr->big_pic_stream_src.addr_vir.addr_y - JPEG_EXIF_SIZE;
        exif_in_ptr->dst.buf_size =
            JPEG_EXIF_SIZE + exif_in_ptr->big_pic_stream_src.buf_size;
        exif_in_ptr++;
    }
    exif_in_ptr = &chn_param_ptr->exif_in[0];
    CMR_LOGD("src addr 0x%lx 0x%lx dst addr 0x%lx, fd 0x%x",
             exif_in_ptr->big_pic_stream_src.addr_vir.addr_y,
             exif_in_ptr->thumb_stream_src.addr_vir.addr_y,
             exif_in_ptr->dst.addr_vir.addr_y, exif_in_ptr->dst.fd);
    CMR_LOGD("dst buf size %d", exif_in_ptr->dst.buf_size);
    return ret;
}

cmr_int snp_set_jpeg_dec_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_jpeg_param *dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        dec_in_ptr->src = req_param_ptr->post_proc_setting.chn_out_frm[i];
        dec_in_ptr->mean.temp_buf =
            req_param_ptr->post_proc_setting.mem[i].jpeg_tmp;
        dec_in_ptr++;
    }
    dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
    if (IMG_ANGLE_0 != req_param_ptr->post_proc_setting.rot_angle) {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            if (req_param_ptr->post_proc_setting.is_need_scaling) {
                dec_in_ptr->dst.addr_phy =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_phy;
                dec_in_ptr->dst.addr_vir =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_vir;
                dec_in_ptr->dst.fd =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv.fd;
            } else {
                dec_in_ptr->dst.addr_phy =
                    req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_phy;
                dec_in_ptr->dst.addr_vir =
                    req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_vir;
                dec_in_ptr->dst.fd =
                    req_param_ptr->post_proc_setting.mem[i].target_yuv.fd;
            }
            dec_in_ptr++;
        }
    } else {
        if (req_param_ptr->is_cfg_rot_cap) {
            for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                if (req_param_ptr->post_proc_setting.is_need_scaling) {
                    dec_in_ptr->dst.addr_phy =
                        req_param_ptr->post_proc_setting.mem[i]
                            .cap_yuv.addr_phy;
                    dec_in_ptr->dst.addr_vir =
                        req_param_ptr->post_proc_setting.mem[i]
                            .cap_yuv.addr_vir;
                    dec_in_ptr->dst.fd =
                        req_param_ptr->post_proc_setting.mem[i].cap_yuv.fd;
                } else {
                    dec_in_ptr->dst.addr_phy =
                        req_param_ptr->post_proc_setting.mem[i]
                            .target_yuv.addr_phy;
                    dec_in_ptr->dst.addr_vir =
                        req_param_ptr->post_proc_setting.mem[i]
                            .target_yuv.addr_vir;
                    dec_in_ptr->dst.fd =
                        req_param_ptr->post_proc_setting.mem[i].target_yuv.fd;
                }
                dec_in_ptr++;
            }
        } else if (!chn_param_ptr->is_scaling) {
            for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                dec_in_ptr->dst.addr_phy =
                    req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_phy;
                dec_in_ptr->dst.addr_vir =
                    req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_vir;
                dec_in_ptr->dst.fd =
                    req_param_ptr->post_proc_setting.mem[i].target_yuv.fd;
                dec_in_ptr++;
            }
        } else {
            for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                dec_in_ptr->dst.addr_phy =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_phy;
                dec_in_ptr->dst.addr_vir =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_vir;
                dec_in_ptr->dst.fd =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv.fd;
                dec_in_ptr++;
            }
        }
    }
    dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        dec_in_ptr->dst.size =
            req_param_ptr->post_proc_setting.chn_out_frm[i].size;
        dec_in_ptr->dst.fmt = CAM_IMG_FMT_YUV420_NV21;
        dec_in_ptr->dst.data_end = cxt->req_param.post_proc_setting.data_endian;
        dec_in_ptr->mean.slice_mode = JPEG_YUV_SLICE_ONE_BUF;
        dec_in_ptr->mean.slice_num = 1;
        dec_in_ptr->mean.slice_height = dec_in_ptr->dst.size.height;
        dec_in_ptr++;
    }
    dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
    /*	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
                    CMR_LOGD("src addr 0x%lx dst 0x%lx 0x%lx",
       dec_in_ptr->src.addr_phy.addr_y,
                                    dec_in_ptr->dst.addr_phy.addr_y,
       dec_in_ptr->dst.addr_phy.addr_u);
                    CMR_LOGD("dst size %d %d", dec_in_ptr->dst.size.width,
       dec_in_ptr->dst.size.height);
            }*/
    return ret;
}

cmr_int snp_set_isp_proc_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct raw_proc_param *isp_proc_in_ptr = &chn_param_ptr->isp_proc_in[0];
    cmr_uint i;

    cmr_bzero(chn_param_ptr->isp_proc_in, sizeof(chn_param_ptr->isp_proc_in));
    cmr_bzero(chn_param_ptr->isp_process, sizeof(chn_param_ptr->isp_process));
    if (IMG_ANGLE_0 == req_param_ptr->post_proc_setting.rot_angle) {
        if (!req_param_ptr->post_proc_setting.is_need_scaling) {
            for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                chn_param_ptr->isp_proc_in[i].dst_frame =
                    req_param_ptr->post_proc_setting.mem[i].target_yuv;
            }
        } else if (req_param_ptr->is_cfg_rot_cap) {
            for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                chn_param_ptr->isp_proc_in[i].dst_frame =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv_rot;
            }
        } else {
            for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                chn_param_ptr->isp_proc_in[i].dst_frame =
                    req_param_ptr->post_proc_setting.mem[i].cap_yuv;
            }
        }
    } else {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            chn_param_ptr->isp_proc_in[i].dst_frame =
                req_param_ptr->post_proc_setting.mem[i].cap_yuv_rot;
        }
    }

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        /*chn_param_ptr->isp_proc_in[i].dst_frame.size =
         * req_param_ptr->post_proc_setting.snp_size;*/
        chn_param_ptr->isp_proc_in[i].dst_frame.size =
            req_param_ptr->post_proc_setting.dealign_actual_snp_size;
        chn_param_ptr->isp_proc_in[i].src_frame =
            req_param_ptr->post_proc_setting.mem[i].cap_raw;
        chn_param_ptr->isp_proc_in[i].src_avail_height =
            req_param_ptr->post_proc_setting.mem[i].cap_raw.size.height;
        chn_param_ptr->isp_proc_in[i].dst2_frame =
            req_param_ptr->post_proc_setting.mem[i].cap_raw2;
        chn_param_ptr->isp_proc_in[i].dst2_frame.size =
            req_param_ptr->post_proc_setting.dealign_actual_snp_size;

        chn_param_ptr->isp_proc_in[i].src_slice_height =
            chn_param_ptr->isp_proc_in[i].src_avail_height;
        chn_param_ptr->isp_proc_in[i].dst_slice_height =
            chn_param_ptr->isp_proc_in[i].src_avail_height;
        chn_param_ptr->isp_proc_in[i].dst2_slice_height =
            chn_param_ptr->isp_proc_in[i].src_avail_height;
        chn_param_ptr->isp_process[i].slice_height_in =
            chn_param_ptr->isp_proc_in[i].src_avail_height;

        if (CAMERA_ISP_SIMULATION_MODE == req_param_ptr->mode) {
            // chn_param_ptr->isp_proc_in[i].src_frame.format_pattern =
            // tool_fmt_pattern;
        } else {
            chn_param_ptr->isp_proc_in[i].src_frame.format_pattern =
                INVALID_FORMAT_PATTERN;
        }
        chn_param_ptr->isp_proc_in[i].src_frame.fmt = ISP_DATA_CSI2_RAW10;
        chn_param_ptr->isp_proc_in[i].dst_frame.fmt = ISP_DATA_YVU420_2FRAME;
        chn_param_ptr->isp_proc_in[i].dst2_frame.fmt = ISP_DATA_ALTEK_RAW10;
    }

    if (!(chn_param_ptr->is_scaling || chn_param_ptr->is_rot)) {
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            chn_param_ptr->isp_process[i].is_encoding = 1;
        }
    }

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        CMR_LOGD("src: fd 0x%x addr_y 0x%lx dst: fd 0x%x addr_y 0x%lx 0x%lx "
                 "dst2: fd 0x%x addr_y 0x%lx w=%d h=%d",
                 chn_param_ptr->isp_proc_in[i].src_frame.fd,
                 chn_param_ptr->isp_proc_in[i].src_frame.addr_phy.addr_y,
                 chn_param_ptr->isp_proc_in[i].dst_frame.fd,
                 chn_param_ptr->isp_proc_in[i].dst_frame.addr_phy.addr_y,
                 chn_param_ptr->isp_proc_in[i].dst_frame.addr_phy.addr_u,
                 chn_param_ptr->isp_proc_in[i].dst2_frame.fd,
                 chn_param_ptr->isp_proc_in[i].dst2_frame.addr_phy.addr_y,
                 req_param_ptr->post_proc_setting.chn_out_frm[i].size.width,
                 req_param_ptr->post_proc_setting.chn_out_frm[i].size.height);
    }
    return ret;
}

void snp_get_is_scaling(cmr_handle snp_handle, cmr_s32 is_normal_snapshot) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_proc_param *proc_param_ptr = &cxt->req_param.post_proc_setting;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct sensor_mode_info *sensor_mode_ptr;
    struct sensor_exp_info *sensor_info_ptr = &cxt->sensor_info;
    struct img_size tmp_size;
    cmr_u32 is_scaling = 0;
    cmr_uint i;

    cxt->chn_param.is_scaling =
        cxt->req_param.post_proc_setting.is_need_scaling;

    CMR_LOGD("is_need_scaling %d, rot_angle %d, is_normal_snapshot %d",
             cxt->req_param.post_proc_setting.is_need_scaling,
             cxt->req_param.post_proc_setting.rot_angle, is_normal_snapshot);

    if ((0 == cxt->chn_param.is_scaling) &&
        (cxt->req_param.mode != CAMERA_ISP_TUNING_MODE) &&
        (cxt->req_param.mode != CAMERA_ISP_SIMULATION_MODE) &&
        (cxt->req_param.mode != CAMERA_UTEST_MODE)) {
        if (is_normal_snapshot) {
            if (cxt->req_param.is_cfg_rot_cap &&
                (IMG_ANGLE_0 == cxt->req_param.post_proc_setting.rot_angle)) {
                cxt->chn_param.is_scaling = 1;
                for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
                    cxt->req_param.post_proc_setting.scaler_src_rect[i]
                        .start_x = 0;
                    cxt->req_param.post_proc_setting.scaler_src_rect[i]
                        .start_y = 0;
                    cxt->req_param.post_proc_setting.scaler_src_rect[i].width =
                        cxt->req_param.post_proc_setting.chn_out_frm[i]
                            .size.width;
                    cxt->req_param.post_proc_setting.scaler_src_rect[i].height =
                        cxt->req_param.post_proc_setting.chn_out_frm[i]
                            .size.height;
                }
            }
        }
    }
    CMR_LOGD("channel id %d is scaling %d", cxt->req_param.channel_id,
             cxt->chn_param.is_scaling);
}

cmr_int snp_clean_thumb_param(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_jpeg_param *jpeg_ptr = &cxt->chn_param.thumb_in[0];

    CMR_LOGD("clean thumb param");
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        cmr_bzero(&jpeg_ptr->src, sizeof(struct img_frm));
        cmr_bzero(&jpeg_ptr->dst, sizeof(struct img_frm));
        jpeg_ptr++;
    }
    cxt->thumb_stream_size = 0;
    return ret;
}

/* start */
/* performance optimization, use zsl buf to postprocess, not copy */
cmr_int snp_update_postproc_src_size(cmr_handle snp_handle,
                                     struct img_frm *chn_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_proc_param *proc_param_ptr = &cxt->req_param.post_proc_setting;

    cmr_u32 width = proc_param_ptr->chn_out_frm[0].size.width;
    cmr_u32 height = proc_param_ptr->chn_out_frm[0].size.height;
    //[4in1 && zsl]scale after post process
    cmr_u32 act_width = proc_param_ptr->cap_org_size.width;
    cmr_u32 act_height = proc_param_ptr->cap_org_size.height;

    if (ZOOM_POST_PROCESS == proc_param_ptr->channel_zoom_mode) {
        chn_data->addr_vir.addr_u =
            (unsigned long)chn_data->addr_vir.addr_y + width * height;
        chn_data->addr_phy.addr_u =
            (unsigned long)chn_data->addr_phy.addr_y + width * height;
    } else {
        chn_data->addr_vir.addr_u =
            (unsigned long)chn_data->addr_vir.addr_y + act_width * act_height;
        chn_data->addr_phy.addr_u =
            (unsigned long)chn_data->addr_phy.addr_y + act_width * act_height;
    }

    return ret;
}

cmr_int snp_update_scale_param(cmr_handle snp_handle, struct img_frm chn_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct img_rect rect[CMR_CAPTURE_MEM_SUM];
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct img_frm *frm_ptr;
    cmr_uint i;
    cmr_u32 offset[CMR_CAPTURE_MEM_SUM] = {0};

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        chn_param_ptr->scale[i].src_img.addr_phy.addr_y =
            chn_data.addr_phy.addr_y + offset[i];
        chn_param_ptr->scale[i].src_img.addr_phy.addr_u =
            chn_data.addr_phy.addr_u + (offset[i] >> 1);
        chn_param_ptr->scale[i].src_img.addr_phy.addr_v =
            chn_data.addr_phy.addr_v + (offset[i] >> 1);
        chn_param_ptr->scale[i].src_img.size.width = chn_data.size.width;
        chn_param_ptr->scale[i].src_img.size.height = chn_data.size.height;
        chn_param_ptr->scale[i].src_img.addr_vir.addr_y =
            chn_data.addr_vir.addr_y + offset[i];
        chn_param_ptr->scale[i].src_img.addr_vir.addr_u =
            chn_data.addr_vir.addr_u + (offset[i] >> 1);
        chn_param_ptr->scale[i].src_img.addr_vir.addr_v =
            chn_data.addr_vir.addr_v + (offset[i] >> 1);
        chn_param_ptr->scale[i].src_img.fd = chn_data.fd;
    }

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        CMR_LOGD("src addr 0x%lx 0x%lx dst add 0x%lx 0x%lx, fd 0x%x",
                 chn_param_ptr->scale[i].src_img.addr_phy.addr_y,
                 chn_param_ptr->scale[i].src_img.addr_phy.addr_u,
                 chn_param_ptr->scale[i].dst_img.addr_phy.addr_y,
                 chn_param_ptr->scale[i].dst_img.addr_phy.addr_u,
                 chn_param_ptr->scale[i].src_img.fd);

        CMR_LOGD("src size %d %d dst size %d %d",
                 chn_param_ptr->scale[i].src_img.size.width,
                 chn_param_ptr->scale[i].src_img.size.height,
                 chn_param_ptr->scale[i].dst_img.size.width,
                 chn_param_ptr->scale[i].dst_img.size.height);
    }
    return ret;
}

cmr_int snp_update_convert_thumb_param(cmr_handle snp_handle,
                                       struct img_frm chn_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct img_rect rect;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_scale_param *thumb_ptr = &cxt->chn_param.convert_thumb[0];
    struct jpeg_param *jpeg_ptr = &req_param_ptr->jpeg_setting;
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        thumb_ptr->src_img = chn_data;
        thumb_ptr->src_img.size =
            req_param_ptr->post_proc_setting.actual_snp_size;
        thumb_ptr->src_img.rect.start_x = 0;
        thumb_ptr->src_img.rect.start_y = 0;
        thumb_ptr->src_img.rect.width = thumb_ptr->src_img.size.width;
        thumb_ptr->src_img.rect.height = thumb_ptr->src_img.size.height;
        thumb_ptr->src_img.data_end =
            req_param_ptr->post_proc_setting.data_endian;
        thumb_ptr->src_img.fmt = CAM_IMG_FMT_YUV420_NV21;
        thumb_ptr++;
    }

    return ret;
}

cmr_int snp_update_rot_param(cmr_handle snp_handle, struct img_frm chn_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_rot_param *rot_ptr = &cxt->chn_param.rot[0];
    cmr_uint i;

    chn_param_ptr->is_rot = 1;
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        rot_ptr->src_img = chn_data;
        rot_ptr++;
    }

exit:
    return ret;
}

cmr_int snp_update_jpeg_enc_param(cmr_handle snp_handle,
                                  struct img_frm chn_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_jpeg_param *jpeg_ptr = &chn_param_ptr->jpeg_in[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        jpeg_ptr->src = chn_data;
        jpeg_ptr->src.size = req_param_ptr->post_proc_setting.actual_snp_size;
        jpeg_ptr++;
    }

    return ret;
}

cmr_int snp_update_ipm_param(cmr_handle snp_handle, struct img_frm chn_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_ipm_param *ipm_ptr = &chn_param_ptr->ipm[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        ipm_ptr->src = chn_data;
        // ipm_ptr->src.size = req_param_ptr->post_proc_setting.actual_snp_size;
        // [4in1 && zsl]scale after post process
        ipm_ptr->src.size = req_param_ptr->post_proc_setting.cap_org_size;
        ipm_ptr++;
    }

    return ret;
}

cmr_int snp_update_jpeg_thumb_param(cmr_handle snp_handle,
                                    struct img_frm chn_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct snp_jpeg_param *jpeg_ptr = &chn_param_ptr->thumb_in[0];
    struct snp_scale_param *thumb_ptr = &cxt->chn_param.convert_thumb[0];
    cmr_uint i;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        if ((thumb_ptr->src_img.size.width == thumb_ptr->dst_img.size.width) &&
            (thumb_ptr->src_img.size.height ==
             thumb_ptr->dst_img.size.height)) {
            jpeg_ptr->src = chn_data;
            jpeg_ptr->src.size = cxt->chn_param.convert_thumb[i].dst_img.size;
            jpeg_ptr->src.rect.start_x = 0;
            jpeg_ptr->src.rect.start_y = 0;
            jpeg_ptr->src.rect.width = jpeg_ptr->src.size.width;
            jpeg_ptr->src.rect.height = jpeg_ptr->src.size.height;
            jpeg_ptr->dst.size = jpeg_ptr->src.size;
        }
        jpeg_ptr++;
        thumb_ptr++;
    }

    return ret;
}

cmr_int zsl_snp_update_post_proc_param(cmr_handle snp_handle,
                                       struct img_frm *img_frame) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &cxt->req_param;

    // update src width and height
    snp_update_postproc_src_size(snp_handle, img_frame);

    if (IMG_ANGLE_0 != req_param_ptr->post_proc_setting.rot_angle) {
        ret = snp_update_rot_param(snp_handle, *img_frame);
        if (ret) {
            CMR_LOGE("failed to set rot param %ld", ret);
            goto exit;
        }
        CMR_LOGV("dont need to update other params");
        goto exit;
    }

    ret = snp_update_ipm_param(snp_handle, *img_frame);
    if (ret) {
        CMR_LOGE("failed to set ipm param %ld", ret);
        goto exit;
    }
    if (cxt->chn_param.is_scaling) {
        ret = snp_update_scale_param(snp_handle, *img_frame);
        if (ret) {
            CMR_LOGE("failed to set scale param %ld", ret);
            goto exit;
        }
        CMR_LOGV("dont need to update other params");
        goto exit;
    }

    ret = snp_update_jpeg_enc_param(snp_handle, *img_frame);
    if (ret) {
        CMR_LOGE("failed to set rot param %ld", ret);
        goto exit;
    }

    if (0 != cxt->req_param.jpeg_setting.thum_size.width &&
        0 != cxt->req_param.jpeg_setting.thum_size.height) {
        ret = snp_update_convert_thumb_param(snp_handle, *img_frame);
        if (ret) {
            CMR_LOGE("failed to set convert thumb param %ld", ret);
            goto exit;
        }
        ret = snp_update_jpeg_thumb_param(snp_handle, *img_frame);
        if (ret) {
            CMR_LOGE("failed to set jpeg thumb param %ld", ret);
            goto exit;
        }
    } else {
        ret = snp_clean_thumb_param(snp_handle);
    }

exit:
    return ret;
}
/* performance optimization, use zsl buf to postprocess, not copy */
/* end */

cmr_int snp_set_post_proc_param(cmr_handle snp_handle,
                                struct snapshot_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_proc_param *proc_param_ptr = &cxt->req_param.post_proc_setting;
    cmr_s32 sm_val = 0;
    cmr_s32 is_normal_cap = 0;

    cxt->cap_cnt = 0;
    cxt->chn_param.is_rot = 0;
    cxt->req_param = *param_ptr;
    CMR_LOGD("@xin chn %d total num %d", cxt->req_param.channel_id,
             cxt->req_param.total_num);
    if (cxt->req_param.is_video_snapshot || cxt->req_param.is_zsl_snapshot) {
        is_normal_cap = 0;
    } else {
        is_normal_cap = 1;
    }
    ret = cxt->ops.get_sensor_info(cxt->oem_handle, cxt->req_param.camera_id,
                                   &cxt->sensor_info);
    if (ret) {
        CMR_LOGE("failed to get sn info %ld", ret);
        goto exit;
    }

    ret = snp_set_jpeg_dec_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set rot param %ld", ret);
        goto exit;
    }

    ret = snp_set_isp_proc_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set rot param %ld", ret);
        goto exit;
    }

    ret = snp_set_channel_out_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set out %ld", ret);
        goto exit;
    }
    ret = snp_set_hdr_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set hdr param %ld", ret);
        goto exit;
    }
    ret = snp_set_ipm_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set ipm param %ld", ret);
        goto exit;
    }
    snp_get_is_scaling(snp_handle, is_normal_cap);

    ret = snp_set_rot_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set rot param %ld", ret);
        goto exit;
    }

    if (cxt->chn_param.is_scaling) {
        ret = snp_set_scale_param(snp_handle);
        if (ret) {
            CMR_LOGE("failed to set scale param %ld", ret);
            goto exit;
        }
    }

    ret = snp_set_jpeg_enc_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set rot param %ld", ret);
        goto exit;
    }
    if (0 != cxt->req_param.jpeg_setting.thum_size.width &&
        0 != cxt->req_param.jpeg_setting.thum_size.height) {
        ret = snp_set_convert_thumb_param(snp_handle);
        if (ret) {
            CMR_LOGE("failed to set convert thumb param %ld", ret);
            goto exit;
        }
        ret = snp_set_jpeg_thumb_param(snp_handle);
        if (ret) {
            CMR_LOGE("failed to set jpeg thumb param %ld", ret);
            goto exit;
        }
    } else {
        ret = snp_clean_thumb_param(snp_handle);
        if (ret) {
            CMR_LOGE("failed to clean thumb param %ld", ret);
            goto exit;
        }
    }

    ret = snp_set_jpeg_exif_param(snp_handle);
    if (ret) {
        CMR_LOGE("failed to set exif param %ld", ret);
    } else {
        sem_getvalue(&cxt->redisplay_sm, &sm_val);
        if (0 != sm_val) {
            sem_destroy(&cxt->redisplay_sm);
            sem_init(&cxt->redisplay_sm, 0, 0);
            CMR_LOGD("re-initialize redisplay sm");
        }
        sem_getvalue(&cxt->writer_exif_sm, &sm_val);
        if (0 != sm_val) {
            sem_destroy(&cxt->writer_exif_sm);
            sem_init(&cxt->writer_exif_sm, 0, 0);
            CMR_LOGD("re-initialize write exif sm");
        }
    }

exit:
    return ret;
}

void snp_set_request(cmr_handle snp_handle, cmr_u32 is_request) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    sem_wait(&cxt->access_sm);
    cxt->is_request = is_request;
    sem_post(&cxt->access_sm);
    CMR_LOGD("request %d", cxt->is_request);
}

cmr_u32 snp_get_request(cmr_handle snp_handle) {
    cmr_u32 is_request = 0;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    sem_wait(&cxt->access_sm);
    is_request = cxt->is_request;
    sem_post(&cxt->access_sm);
    CMR_LOGD("req %d", is_request);
    return is_request;
}

cmr_int snp_checkout_exit(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 is_request = 0;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (0 == snp_get_request(snp_handle)) {
        if (IPM_WORKING == snp_get_status(snp_handle)) {
            if (cxt->req_param.filter_type || cxt->req_param.nr_flag   || cxt->req_param.ee_flag) {
                sem_wait(&cxt->ipm_sync_sm);
            } else {
                sem_post(&cxt->hdr_sync_sm);
                CMR_LOGD("post hdr sm");
            }
        }
        if (POST_PROCESSING == snp_get_status(snp_handle)) {
            sem_wait(&cxt->pre_start_encode_sync_sm);
            sem_post(&cxt->pre_start_encode_sync_sm);
        }
        if (CODEC_WORKING == snp_get_status(snp_handle)) {
            if (cxt->ops.stop_codec) {
                sem_post(&cxt->jpeg_sync_sm);
                ret = cxt->ops.stop_codec(cxt->oem_handle);
                if (ret) {
                    CMR_LOGE("failed to stop codec %ld", ret);
                }
            }
        }
        if (WRITE_EXIF == snp_get_status(snp_handle)) {
            sem_wait(&cxt->writer_exif_sm);
        }
        CMR_LOGD("cxt->oem_cb is 0x%lx", (cmr_uint)cxt->oem_cb);
        if (cxt->oem_cb) {
            snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_RELEASE_PICTURE,
                                    SNAPSHOT_CB_EVT_RSP_SUCCESS, NULL, 0);
        }
        ret = CMR_CAMERA_NORNAL_EXIT;
        if (cxt->ops.channel_free_frame) {
            cxt->ops.channel_free_frame(cxt->oem_handle,
                                        cxt->cur_frame_info.channel_id,
                                        cxt->cur_frame_info.frame_id);
            CMR_LOGD("free frame");
        }
    }
    return ret;
}

cmr_int snp_check_frame_is_valid(cmr_handle snp_handle,
                                 struct frm_info *frm_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (frm_ptr->channel_id != cxt->req_param.channel_id) {
        CMR_LOGD("don't need to process this frame %d %d", frm_ptr->channel_id,
                 cxt->req_param.channel_id);
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    if ((cxt->cap_cnt > cxt->req_param.total_num) ||
        (!IS_CAP_FRM(frm_ptr->frame_id, frm_ptr->base))) {
        CMR_LOGD("this is invalid frame %d %d %d %d", cxt->cap_cnt,
                 cxt->req_param.total_num, frm_ptr->frame_id, frm_ptr->base);
        ret = CMR_CAMERA_INVALID_FRAME;
    }

exit:
    return ret;
}
cmr_int snp_send_msg_notify_thr(cmr_handle snp_handle, cmr_int func_type,
                                cmr_int evt, void *data, cmr_uint data_len) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    cmr_u32 sync_flag = CMR_MSG_SYNC_PROCESSED;
    CMR_MSG_INIT(message);

    CMR_LOGD("evt %ld", evt);
    if (SNAPSHOT_FUNC_STATE == func_type) {
        sync_flag = CMR_MSG_SYNC_NONE;
    } else if (SNAPSHOT_FUNC_TAKE_PICTURE == func_type ||
               SNAPSHOT_FUNC_ENCODE_PICTURE == func_type) {
        switch (evt) {
        case SNAPSHOT_CB_EVT_DONE:
        case SNAPSHOT_CB_EVT_SNAPSHOT_DONE:
        case SNAPSHOT_CB_EVT_CAPTURE_FRAME_DONE:
            sync_flag = CMR_MSG_SYNC_NONE;
            break;
        default:
            sync_flag = CMR_MSG_SYNC_PROCESSED;
            break;
        }
    }

    if (data) {
        message.data = malloc(data_len);
        if (!message.data) {
            ret = CMR_CAMERA_NO_MEM;
            goto exit;
        }
        memcpy(message.data, data, data_len);
        message.alloc_flag = 1;
    } else {
        message.alloc_flag = 0;
    }
    message.msg_type = evt;
    message.sub_msg_type = func_type;
    message.sync_flag = sync_flag;
    ret = cmr_thread_msg_send(cxt->thread_cxt.notify_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send exit msg to notify thr %ld", ret);
        if (message.data) {
            free(message.data);
            message.data = NULL;
            goto exit;
        }
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int snp_send_msg_secondary_thr(cmr_handle snp_handle, cmr_int func_type,
                                   cmr_int evt, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    CMR_MSG_INIT(message);

    CMR_LOGD("evt 0x%ld", evt);
    message.msg_type = evt;
    message.sub_msg_type = func_type;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 0;
    message.data = data;
    ret = cmr_thread_msg_send(cxt->thread_cxt.secondary_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send exit msg to notify thr %ld", ret);
    }
    CMR_LOGV("X");
    return ret;
}

cmr_int snp_send_msg_write_exif_thr(cmr_handle snp_handle, cmr_int evt,
                                    void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    CMR_MSG_INIT(message);

    CMR_LOGD("evt 0x%ld", evt);
    sem_wait(&cxt->thread_cxt.writte_exif_access_sm);
    message.msg_type = evt;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    message.alloc_flag = 1;
    message.data = malloc(sizeof(struct frm_info));
    if (!message.data) {
        CMR_LOGE("failed to malloc");
        sem_post(&cxt->thread_cxt.writte_exif_access_sm);
        return -CMR_CAMERA_NO_MEM;
    } else {
        cmr_copy(message.data, data, sizeof(struct frm_info));
    }
    ret = cmr_thread_msg_send(cxt->thread_cxt.write_exif_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send msg to write thr %ld", ret);
	free(message.data);
	message.data = NULL;
    }
    CMR_LOGV("done %ld", ret);
    sem_post(&cxt->thread_cxt.writte_exif_access_sm);
    ATRACE_END();
    return ret;
}

cmr_int snp_send_msg_redisplay_thr(cmr_handle snp_handle, cmr_int evt,
                                   void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    CMR_MSG_INIT(message);

    CMR_LOGD("evt 0x%ld", evt);
    message.msg_type = evt;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.alloc_flag = 1;
    message.data = malloc(sizeof(struct frm_info));
    if (!message.data) {
        CMR_LOGE("failed to malloc");
        return -CMR_CAMERA_NO_MEM;
    } else {
        cmr_copy(message.data, data, sizeof(struct frm_info));
    }
    ret = cmr_thread_msg_send(cxt->thread_cxt.proc_redisplay_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send msg to redisplay thr %ld", ret);
	free(message.data);
	message.data = NULL;
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_send_msg_thumb_thr(cmr_handle snp_handle, cmr_int evt, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    CMR_MSG_INIT(message);

    CMR_LOGD("evt 0x%ld", evt);
    message.msg_type = evt;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.alloc_flag = 1;
    message.data = malloc(sizeof(struct frm_info));
    if (!message.data) {
        CMR_LOGE("failed to malloc");
        return -CMR_CAMERA_NO_MEM;
    } else {
        cmr_copy(message.data, data, sizeof(struct frm_info));
    }
    ret = cmr_thread_msg_send(cxt->thread_cxt.proc_thumb_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send msg to thumb thr %ld", ret);
	free(message.data);
	message.data = NULL;
    }
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int camera_set_frame_type(cmr_handle snp_handle,
                              struct camera_frame_type *frame_type,
                              struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr;
    struct cmr_cap_mem *mem_ptr;
    bool select_mode = false;
    cmr_u32 frm_id = info->frame_id - info->base;
    char value[PROPERTY_VALUE_MAX];

    req_param_ptr = &cxt->req_param;
    mem_ptr = &req_param_ptr->post_proc_setting.mem[frm_id];
    if (mem_ptr == NULL){
        return CMR_CAMERA_FAIL;
    }
    frame_type->buf_id = info->frame_real_id;
    select_mode = (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr ||
        cxt->req_param.is_pipviv_mode);
    if (select_mode) {
        frame_type->width =
            req_param_ptr->post_proc_setting.dealign_actual_snp_size.width;
        frame_type->height =
            req_param_ptr->post_proc_setting.dealign_actual_snp_size.height;
    } else {
        frame_type->width =
            req_param_ptr->post_proc_setting.actual_snp_size.width;
        frame_type->height =
            req_param_ptr->post_proc_setting.actual_snp_size.height;
    }
    frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
    frame_type->monoboottime = info->monoboottime;

    switch (req_param_ptr->mode) {
    case CAMERA_UTEST_MODE:
        frame_type->y_vir_addr = mem_ptr->target_yuv.addr_vir.addr_y;
        frame_type->format = info->fmt;
        break;
    case CAMERA_AUTOTEST_MODE:
        frame_type->y_vir_addr =
            req_param_ptr->post_proc_setting.chn_out_frm[frm_id]
                .addr_vir.addr_y;
        frame_type->format = info->fmt;
        break;
    default: {
        cmr_u32 size =
            cxt->req_param.post_proc_setting.actual_snp_size.width *
                cxt->req_param.post_proc_setting.actual_snp_size.height;

        struct camera_context *oem_cxt =
            (struct camera_context *)cxt->oem_handle;
        CMR_LOGD("camera id %d, refocus_mode %d", oem_cxt->camera_id,
                 oem_cxt->is_refocus_mode);
        if (oem_cxt->is_refocus_mode == 2)
            frame_type->y_vir_addr = info->yaddr_vir;
        else
            frame_type->y_vir_addr = mem_ptr->target_yuv.addr_vir.addr_y;

        frame_type->fd = mem_ptr->target_yuv.fd;
        frame_type->y_phy_addr = mem_ptr->target_yuv.addr_phy.addr_y;
        frame_type->uv_vir_addr = mem_ptr->target_yuv.addr_vir.addr_u;
        frame_type->uv_phy_addr = mem_ptr->target_yuv.addr_phy.addr_u;
        frame_type->format = CAMERA_YCBCR_4_2_0;
        property_get("persist.vendor.cam.isptool.mode.enable", value, "false");
        if ((!strcmp(value, "true")) ||
            (CAMERA_ISP_SIMULATION_MODE == cxt->req_param.mode)) {
            if((cxt != NULL) && (mem_ptr != NULL)){
                send_capture_data(
                    0x40, /* yuv420 */
                    cxt->req_param.post_proc_setting.actual_snp_size.width,
                    cxt->req_param.post_proc_setting.actual_snp_size.height,
                    (char *)mem_ptr->target_yuv.addr_vir.addr_y, size,
                    (char *)mem_ptr->target_yuv.addr_vir.addr_u, size / 2, 0, 0);
            }
        }
    } break;
    }

    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 3 || (atoi(value) & (1 << 3))) {
        struct camera_context *cam_ctx = cxt->oem_handle;
        dump_image("camera_set_frame_type", CAM_IMG_FMT_YUV420_NV21,
                   frame_type->width, frame_type->height,
                   FORM_DUMPINDEX(SNP_REDISPLAY_DATA, cam_ctx->dump_cnt, 0),
                   &mem_ptr->target_yuv.addr_vir,
                   frame_type->width * frame_type->height * 3 / 2);
    }

    if (select_mode) {
        CMR_LOGD(
            "vendor capture cap_cnt = %d, total_num = %d, is_pipviv_mode %d",
            cxt->cap_cnt, cxt->req_param.total_num,
            cxt->req_param.is_pipviv_mode);
        if (0 == snp_get_request(snp_handle) ||
            (cxt->cap_cnt == cxt->req_param.total_num) ||
            cxt->req_param.is_pipviv_mode) {
            frame_type->need_free = 1;
        } else {
            frame_type->need_free = 0;
        }
    }

    CMR_LOGD(
        "index 0x%x, addr 0x%lx 0x%lx, w h %d %d, format %d, sec %ld usec %ld",
        info->frame_id, frame_type->y_vir_addr, frame_type->y_phy_addr,
        frame_type->width, frame_type->height, frame_type->format, info->sec,
        info->usec);
    return ret;
}

void snp_takepic_callback_done(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    sem_post(&cxt->takepic_callback_sem);
    CMR_LOGV("X");
}

void snp_takepic_callback_wait(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    CMR_LOGV("wait beging");
    sem_wait(&cxt->takepic_callback_sem);
    CMR_LOGV("wait end");
}

cmr_int snp_redisplay(cmr_handle snp_handle, struct frm_info *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (!data) {
        CMR_LOGE("param error");
        goto exit;
    }
    ret = snp_send_msg_redisplay_thr(snp_handle, SNP_EVT_REDISPLAY_START, data);
exit:
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int snp_start_thumb_proc(cmr_handle snp_handle, struct frm_info *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (!data) {
        CMR_LOGE("param error");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if ((0 != cxt->req_param.jpeg_setting.thum_size.width) &&
        (0 != cxt->req_param.jpeg_setting.thum_size.height)) {
        ret = snp_send_msg_thumb_thr(snp_handle, SNP_EVT_THUMB_START, data);
        if (ret) {
            CMR_LOGE("failed to send msg thumb %ld", ret);
            goto exit;
        }
    }
exit:
    ATRACE_END();
    if ((0 != cxt->req_param.jpeg_setting.thum_size.width) &&
        (0 != cxt->req_param.jpeg_setting.thum_size.height) &&
        (CMR_CAMERA_SUCCESS != ret)) {
        sem_post(&cxt->scaler_sync_sm);
    }
    return ret;
}

cmr_int snp_notify_redisplay_proc(cmr_handle snp_handle, cmr_int func_type,
                                  cmr_int evt, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (!snp_handle) {
        CMR_LOGE("param error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGD("evt 0x%lx, func 0x%lx", evt, func_type);

    if (SNAPSHOT_CB_EVT_MAX > evt) {
        if (cxt->oem_cb) {
            cxt->oem_cb(cxt->oem_handle, evt, func_type, data);
        } else {
            CMR_LOGE("err, oem cb is null");
        }
    } else {
        CMR_LOGE("don't support this 0x%lx", evt);
    }
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}
/**modified for 3d calibration begin   */
cmr_int snp_yuv_callback_take_picture_done(cmr_handle snp_handle,
                                           struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct camera_frame_type frame_type;
    struct camera_context *cmr_cxt;
    cmr_cxt = (struct camera_context *)cxt->oem_handle;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }
    if (!data) {
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    CMR_PRINT_TIME;
    CMR_LOGD(
        "3dcalibration yuv take done in picture: width:%d, height:%d, fd:%d, "
        "yaddp:%d, uaddp:%d, vaddp:%d, yaddv:%d, uaddv:%d, vaddv:%d",
        cxt->req_param.post_proc_setting.dealign_actual_snp_size.width,
        data->height, data->fd, data->yaddr, data->uaddr, data->vaddr,
        data->yaddr_vir, data->uaddr_vir, data->vaddr_vir);
    frame_type.fd = data->fd;
    frame_type.buf_id = data->frame_real_id;
    frame_type.width =
        cxt->req_param.post_proc_setting.dealign_actual_snp_size.width;
    frame_type.height =
        cxt->req_param.post_proc_setting.dealign_actual_snp_size.height;
    frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000LL;
    frame_type.monoboottime = data->monoboottime;
    frame_type.y_vir_addr = data->yaddr_vir;
    frame_type.uv_vir_addr = data->uaddr_vir;
    frame_type.y_phy_addr = data->yaddr;
    frame_type.uv_phy_addr = data->uaddr;
    frame_type.format = data->fmt;
    property_get("debug.camera.save.3dcalfile", prop, "0");
    if (atoi(prop) == 1) {
        struct img_addr imgadd = {0, 0, 0};
        imgadd.addr_y = data->yaddr_vir;
        imgadd.addr_u = data->uaddr_vir;
        imgadd.addr_v = data->vaddr_vir;
        dump_image("snp_yuv_callback_take_picture_done",
                   CAM_IMG_FMT_YUV420_NV21, frame_type.width, frame_type.height,
                   8881, &imgadd, frame_type.width * frame_type.height * 3 / 2);
    }
    snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                            SNAPSHOT_CB_EVT_SNAPSHOT_DONE, (void *)&frame_type,
                            sizeof(struct camera_frame_type));
    snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                            SNAPSHOT_CB_EVT_DONE, (void *)&frame_type,
                            sizeof(struct camera_frame_type));
    if (cxt->req_param.is_hdr == 1 &&
        (cmr_cxt->is_multi_mode == MODE_BOKEH ||
         cmr_cxt->is_multi_mode == MODE_MULTI_CAMERA)) {
        snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                SNAPSHOT_CB_EVT_RETURN_SW_ALGORITHM_ZSL_BUF,
                                NULL, sizeof(struct camera_frame_type));
    } else {
        snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                SNAPSHOT_CB_EVT_RETURN_ZSL_BUF,
                                (void *)&frame_type,
                                sizeof(struct camera_frame_type));
    }
    sem_post(&cxt->redisplay_sm);
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}
/**modified for 3d calibration end   */
cmr_int snp_take_picture_done(cmr_handle snp_handle, struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct camera_frame_type frame_type;

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        sem_post(&cxt->redisplay_sm);
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }
    if (!data) {
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    cmr_bzero(&frame_type, sizeof(frame_type));
    CMR_PRINT_TIME;
    ret = camera_set_frame_type(snp_handle, &frame_type, data);
    if (CMR_CAMERA_SUCCESS == ret) {
        snp_notify_redisplay_proc(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                  SNAPSHOT_CB_EVT_SNAPSHOT_DONE,
                                  (void *)&frame_type);
        snp_notify_redisplay_proc(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                  SNAPSHOT_CB_EVT_DONE, (void *)&frame_type);
    } else {
        snp_notify_redisplay_proc(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                  SNAPSHOT_CB_EVT_FAILED, NULL);
    }

    if (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr ||
        cxt->req_param.is_pipviv_mode) {
        snp_notify_redisplay_proc(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                  SNAPSHOT_CB_EVT_SNAPSHOT_JPEG_DONE,
                                  (void *)&frame_type);
    } else {
        snp_notify_redisplay_proc(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                  SNAPSHOT_CB_EVT_SNAPSHOT_JPEG_DONE, NULL);
    }
    sem_post(&cxt->redisplay_sm);
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_thumbnail(cmr_handle snp_handle, struct frm_info *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    if (!data) {
        CMR_LOGE("param error");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ret = snp_start_convet_thumb(snp_handle, data);
    if (ret) {
        CMR_LOGE("failed to convert thumb %ld", ret);
        goto exit;
    }
    ret = snp_start_encode_thumb(snp_handle);
    if (ret) {
        CMR_LOGE("failed to start encode thumb %ld", ret);
        goto exit;
    }
exit:
    ATRACE_END();
    sem_post(&cxt->scaler_sync_sm);
    return ret;
}

static cmr_int snp_ipm_process(cmr_handle snp_handle, void *data) {

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *snap_cxt = (struct snp_context *)snp_handle;
    struct camera_context *oem_cxt = snap_cxt->oem_handle;
    struct snp_channel_param *chn_param_ptr = &snap_cxt->chn_param;
    struct frm_info *chn_data_ptr = (struct frm_info *)data;
    cmr_u32 index = chn_data_ptr->frame_id - chn_data_ptr->base;

    struct img_frm *src = NULL;

    sem_wait(&snap_cxt->ipm_sync_sm);
    snp_set_status(snp_handle, IPM_WORKING);

    src = &chn_param_ptr->ipm[index].src;

    if (snap_cxt->ops.ipm_process == NULL) {

        CMR_LOGE("err ipm_process is null");
        ret = -CMR_CAMERA_FAIL;
        return ret;
    }

    snap_cxt->ops.ipm_process(oem_cxt, src);

    sem_post(&snap_cxt->ipm_sync_sm);
    snp_set_status(snp_handle, POST_PROCESSING);
    return ret;
}

cmr_int snp_post_proc_for_yuv(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct frm_info *chn_data_ptr = (struct frm_info *)data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    struct camera_context *cam_cxt = cxt->oem_handle;
    cmr_u32 index;
    cmr_uint cnr_type;
    char value[PROPERTY_VALUE_MAX];

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    cxt->cap_cnt++;

    snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                            SNAPSHOT_CB_EVT_CAPTURE_FRAME_DONE, NULL, 0);
    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                            SNAPSHOT_CB_EVT_RSP_SUCCESS, NULL, 0);
    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    cnr_type = cxt->ops.get_cnr_realtime_flag(cxt->oem_handle);
    cxt->req_param.nr_flag = cnr_type;
    cam_cxt->nr_flag = cnr_type;
    CMR_LOGV("cnr_type %d", cnr_type);

    snp_set_status(snp_handle, POST_PROCESSING);
    if ((cxt->req_param.filter_type || cxt->req_param.nr_flag
        || cxt->req_param.dre_flag || cxt->req_param.ee_flag)
        && !cxt->req_param.is_yuv_callback_mode) {
        CMR_LOGI("before snp_ipm_process cameraid = %d ",cxt->camera_id);
        snp_ipm_process(snp_handle, data);
    }

    if (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr ||
        cxt->req_param.is_pipviv_mode || cxt->req_param.is_3dcalibration_mode ||
        cxt->req_param.is_yuv_callback_mode) {
        if (chn_param_ptr->is_rot) {
            CMR_LOGD("need rotate");
            ret = snp_start_rot(snp_handle, data);
            if (ret) {
                CMR_LOGE("failed to start rot %ld", ret);
                goto exit;
            }
        }

        cxt->cur_frame_info = *chn_data_ptr;
        if (chn_param_ptr->is_scaling) {
            ret = snp_start_scale(snp_handle, data);
            if (ret) {
                CMR_LOGE("failed to start scale %ld", ret);
                goto exit;
            }
        } else {
            if (cxt->req_param.is_3dcalibration_mode ||
                cxt->req_param.is_yuv_callback_mode) {
                ret = snp_yuv_callback_take_picture_done(snp_handle,
                                                         chn_data_ptr);
            } else {
                ret = snp_take_picture_done(snp_handle, chn_data_ptr);
            }
            snp_post_proc_done(snp_handle);
            CMR_LOGV("is_pipviv_mode %d chn_data_ptr %p",
                     cxt->req_param.is_pipviv_mode, chn_data_ptr);
        }
        goto exit;
    }

    CMR_LOGD(
        "w %d, h %d, yaddr 0x%lx, fd 0x%x",
        chn_param_ptr->chn_frm[chn_data_ptr->frame_id - chn_data_ptr->base]
            .size.width,
        chn_param_ptr->chn_frm[chn_data_ptr->frame_id - chn_data_ptr->base]
            .size.height,
        chn_param_ptr->chn_frm[chn_data_ptr->frame_id - chn_data_ptr->base]
            .addr_vir.addr_y,
        chn_param_ptr->chn_frm[chn_data_ptr->frame_id - chn_data_ptr->base].fd);
    camera_take_snapshot_step(CMR_STEP_CAP_E);
    property_get("debug.camera.save.snpfile", value, "0");
    if (atoi(value) == 1 || atoi(value) == 100 || (atoi(value) & (1 << 1))) {
        struct camera_context *cam_ctx = cxt->oem_handle;
        cmr_u32 image_width = 0;
        cmr_u32 image_height = 0;
        image_width =
            chn_param_ptr->chn_frm[chn_data_ptr->frame_id - chn_data_ptr->base]
                .size.width;
        image_height =
            chn_param_ptr->chn_frm[chn_data_ptr->frame_id - chn_data_ptr->base]
                .size.height;
        dump_image(
            "snp_post_proc_for_yuv", CAM_IMG_FMT_YUV420_NV21, image_width,
            image_height, SNP_CHN_OUT_DATA,
            &chn_param_ptr->chn_frm[chn_data_ptr->frame_id - chn_data_ptr->base]
                 .addr_vir,
            image_width * image_height * 3 / 2);
    }

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    ret =
        snp_send_msg_write_exif_thr(snp_handle, SNP_EVT_WRITE_EXIF_START, data);
    if (ret) {
        CMR_LOGE("snp_send_msg_write_exif_thr failed, ret=%ld", ret);
        goto exit;
    }

    if (chn_param_ptr->is_rot) {
        CMR_LOGD("need rotate");
        ret = snp_start_rot(snp_handle, data);
        if (ret) {
            CMR_LOGE("failed to start rot %ld", ret);
            sem_post(&cxt->scaler_sync_sm);
            sem_post(&cxt->jpeg_sync_sm);
            goto exit;
        }
    }

    cxt->cur_frame_info = *chn_data_ptr;
    if (chn_param_ptr->is_scaling) {
        ret = snp_start_scale(snp_handle, data);
        if (ret) {
            CMR_LOGE("failed to start scale %ld", ret);
            sem_post(&cxt->scaler_sync_sm);
            sem_post(&cxt->jpeg_sync_sm);
            goto exit;
        }
    } else { //!chn_param_ptr->is_scaling
        ret = snp_start_encode(snp_handle, data);
        if (ret) {
            CMR_LOGE("failed to start encode %ld", ret);
            sem_post(&cxt->scaler_sync_sm);
            goto exit;
        }
        if ((0 != cxt->req_param.jpeg_setting.thum_size.width) &&
            (0 != cxt->req_param.jpeg_setting.thum_size.height)) {
            ret = snp_start_thumb_proc(snp_handle, data);
            if (ret) {
                CMR_LOGE("failed to start thumb proc %ld", ret);
                goto exit;
            }
        }
        ret = snp_redisplay(snp_handle, data);
        if (ret) {
            CMR_LOGE("failed to take pic done %ld", ret);
            sem_post(&cxt->scaler_sync_sm);
            goto exit;
        }
        if ((0 == cxt->req_param.jpeg_setting.thum_size.width) ||
            (0 == cxt->req_param.jpeg_setting.thum_size.height)) {
            sem_post(&cxt->scaler_sync_sm);
        }
    }

exit:
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int snp_post_proc_for_jpeg(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct frm_info *chn_data_ptr = (struct frm_info *)data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;

    camera_take_snapshot_step(CMR_STEP_JPG_DEC_S);
    ret = snp_start_decode_sync(snp_handle, data);
    camera_take_snapshot_step(CMR_STEP_JPG_DEC_E);
    if (ret) {
        return ret;
    }
    chn_data_ptr->fmt = CAM_IMG_FMT_YUV420_NV21;
    ret = snp_post_proc_for_yuv(snp_handle, data);
    return ret;
}

cmr_int isp_is_have_src_data_from_picture(void) {
    FILE *fp = 0;
    char isptool_src_mipi_raw[] = SRC_MIPI_RAW;

    fp = fopen(isptool_src_mipi_raw, "r");
    if (fp != NULL) {
        fclose(fp);
        CMR_LOGD("have input_raw source file");
    } else {
        CMR_LOGD("no input_raw source file");
        return -CMR_CAMERA_FAIL;
    }

    return CMR_CAMERA_SUCCESS;
}
void isp_set_saved_file_count(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    cxt->isptool_saved_file_count++;
}

cmr_u32 isp_get_saved_file_count(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    return cxt->isptool_saved_file_count;
}

cmr_int isp_overwrite_cap_mem(cmr_handle snp_handle) {
    FILE *fp = NULL;
    char isptool_src_mipi_raw[] = SRC_MIPI_RAW;
    struct camera_frame_type frame_type;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct cmr_cap_mem *mem_ptr = &cxt->req_param.post_proc_setting.mem[0];
    cmr_u32 pixel_width = cxt->sensor_info.sn_interface.pixel_width;
    cmr_u32 memsize;

    memsize = mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height *
              pixel_width / 8;
    fp = fopen(isptool_src_mipi_raw, "r");
    if (fp != NULL) {
        size_t f_ret = fread((void *)mem_ptr->cap_raw.addr_vir.addr_y, 1, memsize, fp);
        fclose(fp);
    } else {
        CMR_LOGE("no input_raw source file");
        return -CMR_CAMERA_FAIL;
    }

    isp_set_saved_file_count(snp_handle);

    frame_type.y_vir_addr = mem_ptr->cap_raw.addr_vir.addr_y;
    frame_type.y_phy_addr = mem_ptr->cap_raw.addr_phy.addr_y;
    frame_type.fd = mem_ptr->cap_raw.fd;
    frame_type.width = mem_ptr->cap_raw.size.width;
    frame_type.height = mem_ptr->cap_raw.size.height * pixel_width / 8;

    snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                            SNAPSHOT_CB_EVT_FLUSH_CACHE, (void *)&frame_type,
                            sizeof(struct camera_frame_type));

    return CMR_CAMERA_SUCCESS;
}

cmr_int snp_post_proc_for_isp_tuning(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct frm_info *chn_data_ptr = (struct frm_info *)data;
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct cmr_cap_mem *mem_ptr =
        &snp_cxt->req_param.post_proc_setting.mem[snp_cxt->index];
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 loose_flag = snp_cxt->sensor_info.sn_interface.is_loose;
    CMR_MSG_INIT(message);

    if (!isp_is_have_src_data_from_picture()) {
        isp_overwrite_cap_mem(snp_handle);
    }

    message.msg_type = SNP_EVT_CVT_START;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 0;
    message.sub_msg_type = SNP_TRIGGER;
    message.data = data;
    ret = cmr_thread_msg_send(snp_cxt->thread_cxt.cvt_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send start cvt msg to cvt thr %ld", ret);
    }

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        cmr_u32 width, height;
        CMR_LOGI("is_loose %d", loose_flag);
        width = mem_ptr->cap_raw2.size.width;
        height = mem_ptr->cap_raw2.size.height;
        if (loose_flag == ISP_RAW_HALF14 || loose_flag == ISP_RAW_HALF10) {
            CMR_LOGD("dump raw14bit dcam raw");
            snp_cxt->ops.dump_image_with_3a_info(
            snp_cxt->oem_handle, CAM_IMG_FMT_DCAM_RAW14BIT, width, height,
            mem_ptr->cap_raw2.buf_size, &mem_ptr->cap_raw2.addr_vir);
        } else {
            CMR_LOGD("dump dcam raw");
            snp_cxt->ops.dump_image_with_3a_info(
            snp_cxt->oem_handle, CAM_IMG_FMT_BAYER_SPRD_DCAM_RAW, width, height,
            ((width * 5 / 4 + 3) & ~3) * height, &mem_ptr->cap_raw2.addr_vir);
        }

        CMR_LOGD("dump yuv");
        width = mem_ptr->target_yuv.size.width;
        height = mem_ptr->target_yuv.size.height;
        snp_cxt->ops.dump_image_with_3a_info(
            snp_cxt->oem_handle, CAM_IMG_FMT_YUV420_NV21, width, height,
            width * height * 3 / 2, &mem_ptr->target_yuv.addr_vir);
    }

    chn_data_ptr->fmt = CAM_IMG_FMT_YUV420_NV21;
    ret = snp_post_proc_for_yuv(snp_handle, data);
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_post_proc_for_raw(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct frm_info *chn_data_ptr = (struct frm_info *)data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    CMR_MSG_INIT(message);

    CMR_LOGD("snapshot mode %d", cxt->req_param.mode);

    switch (cxt->req_param.mode) {
    case CAMERA_UTEST_MODE:
        ret = snp_take_picture_done(snp_handle, (struct frm_info *)data);
        break;
    case CAMERA_ISP_TUNING_MODE:
    case CAMERA_ISP_SIMULATION_MODE:
        ret = snp_post_proc_for_isp_tuning(snp_handle, data);
        break;
    default: {
        message.msg_type = SNP_EVT_CVT_START;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        message.alloc_flag = 0;
        message.sub_msg_type = SNP_TRIGGER;
        message.data = data;
        ret = cmr_thread_msg_send(cxt->thread_cxt.cvt_thr_handle, &message);
        if (ret) {
            CMR_LOGE("failed to send stop msg to cvt thr %ld", ret);
            break;
        }
        if (cxt->err_code) {
            CMR_LOGE("failed to convert format %ld", cxt->err_code);
            ret = cxt->err_code;
            break;
        }
        chn_data_ptr->fmt = CAM_IMG_FMT_YUV420_NV21;
        ret = snp_post_proc_for_yuv(snp_handle, data);
    } break;
    }

    return ret;
}

void snp_autotest_proc(cmr_handle snp_handle, void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct frm_info *chn_data_ptr = (struct frm_info *)data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    ret = snp_take_picture_done(snp_handle, data);
    if (ret) {
        CMR_LOGE("failed to take pic done %ld", ret);
    }
    sem_post(&cxt->proc_done_sm);
    CMR_LOGV("X, ret=%ld", ret);
}

void snp_reset_ctrl_condition(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    cmr_s32 sm_val = 0;

    if (!snp_handle) {
        CMR_LOGE("handle is null");
        return;
    }

    sem_getvalue(&cxt->writer_exif_sm, &sm_val);
    if (0 != sm_val) {
        sem_destroy(&cxt->writer_exif_sm);
        sem_init(&cxt->writer_exif_sm, 0, 0);
        CMR_LOGD("re-initialize writer exif sm");
    }

    sem_getvalue(&cxt->jpeg_sync_sm, &sm_val);
    if (0 != sm_val) {
        sem_destroy(&cxt->jpeg_sync_sm);
        sem_init(&cxt->jpeg_sync_sm, 0, 0);
        CMR_LOGD("re-initialize jpeg sync sm");
    }

    sem_getvalue(&cxt->scaler_sync_sm, &sm_val);
    if (0 != sm_val) {
        sem_destroy(&cxt->scaler_sync_sm);
        sem_init(&cxt->scaler_sync_sm, 0, 0);
        CMR_LOGD("re-initialize scaler sync sm");
    }
}

cmr_int snp_post_proc(cmr_handle snp_handle, void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (!snp_handle || !data) {
        CMR_LOGE("error param 0x%lx, 0x%lx", (cmr_uint)snp_handle,
                 (cmr_uint)data);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    struct frm_info *chn_data_ptr = (struct frm_info *)data;
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct camera_context *cam_cxt = (struct camera_context *)cxt->oem_handle;
    struct snp_channel_param *chn_param_ptr = &cxt->chn_param;
    cmr_u32 fmt;
    struct buffer_cfg buf_cfg;

    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        return ret;
    }
    CMR_LOGD("waiting begin");
    sem_wait(&cxt->proc_done_sm);
    CMR_LOGD("waiting end");

    snp_reset_ctrl_condition(snp_handle);
    cxt->err_code = 0;
    if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
        CMR_LOGD("post proc has been cancel");
        ret = CMR_CAMERA_NORNAL_EXIT;
        goto exit;
    }

    ret = snp_check_frame_is_valid(snp_handle, chn_data_ptr);
    if (CMR_CAMERA_NORNAL_EXIT == ret) {
        goto exit;
    } else if (CMR_CAMERA_INVALID_FRAME == ret) {
        if (cxt->ops.channel_buff_cfg && !cxt->req_param.is_video_snapshot) {
            buf_cfg.channel_id = chn_data_ptr->channel_id;
            buf_cfg.base_id = CMR_BASE_ID(chn_data_ptr->frame_id);
            buf_cfg.count = 1;
            buf_cfg.flag = BUF_FLAG_RUNNING;
            buf_cfg.addr[0].addr_y = chn_data_ptr->yaddr;
            buf_cfg.addr[0].addr_u = chn_data_ptr->uaddr;
            buf_cfg.addr[0].addr_v = chn_data_ptr->vaddr;
            buf_cfg.addr_vir[0].addr_y = chn_data_ptr->yaddr_vir;
            buf_cfg.addr_vir[0].addr_u = chn_data_ptr->uaddr_vir;
            buf_cfg.addr_vir[0].addr_v = chn_data_ptr->vaddr_vir;
            buf_cfg.fd[0] = chn_data_ptr->fd;
            cxt->ops.channel_buff_cfg(cxt->oem_handle, &buf_cfg);
            CMR_LOGD("free frame");
        }
        goto exit;
    }
    CMR_LOGD("path data index 0x%x 0x%x",
             chn_data_ptr->frame_id, chn_data_ptr->base);
    cxt->index = chn_data_ptr->frame_id - chn_data_ptr->base;
    fmt = chn_data_ptr->fmt;
    CMR_LOGD("index %d fmt %d", cxt->index, fmt);
    if (CAMERA_AUTOTEST_MODE == cxt->req_param.mode) {
        snp_autotest_proc(snp_handle, data);
        return ret;
    } else {
        switch (fmt) {
        case CAM_IMG_FMT_JPEG:
            ret = snp_post_proc_for_jpeg(snp_handle, data);
            break;
        case CAM_IMG_FMT_YUV420_NV21:
        case CAM_IMG_FMT_YUV420_NV12:
        case CAM_IMG_FMT_YUV422P:
            ret = snp_post_proc_for_yuv(snp_handle, data);
            break;
        case CAM_IMG_FMT_BAYER_MIPI_RAW:

            ret = snp_post_proc_for_raw(snp_handle, data);

            break;
        default:
            CMR_LOGE("err, fmt %d", fmt);
            ret = CMR_CAMERA_INVALID_FORMAT;
            break;
        }
    }
exit:
    if (ret) {
        cxt->err_code = ret;
        snp_post_proc_err_exit(snp_handle, ret);
    }
    CMR_LOGV("X, ret=%ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int snp_stop_proc(cmr_handle snp_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snp_handle;

    CMR_LOGV("E");
    ret = snp_checkout_exit(snp_handle);
    snp_set_status(snp_handle, IDLE);
    CMR_LOGV("X");
    return ret;
}

cmr_int cmr_snapshot_thumb_yuv_proc(cmr_handle snp_handle,
                                    struct snp_thumb_yuv_param *thumb_parm) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if ((NULL == snp_handle) || (NULL == thumb_parm)) {
        CMR_LOGE("param invalid");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("E");
    struct snp_context *snp_cxt = (struct snp_context *)snp_handle;
    struct snapshot_param *req_param_ptr = &snp_cxt->req_param;
    struct jpeg_param *jpeg_ptr = &req_param_ptr->jpeg_setting;
    struct snp_channel_param *chn_param_ptr = &snp_cxt->chn_param;
    struct img_frm src, dst, rot_img;
    struct cmr_op_mean mean;
    bool is_scale = false;

    src = thumb_parm->src_img;
    dst = thumb_parm->dst_img;
    mean.slice_height = dst.rect.height;
    mean.is_sync = 1;
    mean.rot = thumb_parm->angle;
    src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
    src.rect.start_x = 0;
    src.rect.start_y = 0;
    src.fmt = CAM_IMG_FMT_YUV420_NV21;
    dst.fmt = CAM_IMG_FMT_YUV420_NV21;
    dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
    src.data_end.uv_endian = IMG_DATA_ENDIAN_3PLANE;
    dst.data_end.uv_endian = IMG_DATA_ENDIAN_3PLANE;

    if (src.size.width != dst.size.width ||
        src.size.height != dst.size.height) {
        is_scale = true;
    }
    if (chn_param_ptr->is_scaling) {
        rot_img = req_param_ptr->post_proc_setting.mem[0].cap_yuv;
    } else {
        rot_img = req_param_ptr->post_proc_setting.mem[0].target_yuv;
    }
    if (IMG_ANGLE_90 == mean.rot || IMG_ANGLE_270 == mean.rot) {
        rot_img.size.width = src.size.height;
        rot_img.size.height = src.size.width;
        rot_img.rect.width = src.size.height;
        rot_img.rect.height = src.size.width;
        dst.size.width = thumb_parm->dst_img.size.height;
        dst.size.height = thumb_parm->dst_img.size.width;
        dst.rect.width = thumb_parm->dst_img.size.height;
        dst.rect.height = thumb_parm->dst_img.size.width;
    } else {
        rot_img.size.width = src.size.width;
        rot_img.size.height = src.size.height;
        rot_img.rect.width = src.size.width;
        rot_img.rect.height = src.size.height;
    }
    if (mean.rot) {
        if (snp_cxt->ops.start_rot == NULL) {
            CMR_LOGE("snp_cxt->ops.start_rot is null");
            goto exit;
        }

        CMR_LOGD("rot start");
        ret = snp_cxt->ops.start_rot(snp_cxt->oem_handle, snp_handle, &src,
                                     &rot_img, &mean);
        if (ret) {
            CMR_LOGE("snp_cxt->ops.start_rot failed");
            goto exit;
        }
        src = rot_img;
        mean.slice_height = dst.rect.height;
        mean.rot = 0;
    }
    if (is_scale) {
        if (snp_cxt->ops.start_scale == NULL) {
            CMR_LOGE("snp_cxt->ops.start_scale is null");
            ret = -CMR_CAMERA_FAIL;
            goto exit;
        }
        CMR_LOGD("scale start");

        ret = snp_cxt->ops.start_scale(snp_cxt->oem_handle, snp_handle, &src,
                                       &dst, &mean);
        if (ret) {
            CMR_LOGE("snp_cxt->ops.start_scale failed");
            goto exit;
        }
    }

exit:

    CMR_LOGD("src addr 0x%lx 0x%lx 0x%lx 0x%lx fd 0x%x size %d %d %d",
             thumb_parm->src_img.addr_phy.addr_y,
             thumb_parm->src_img.addr_phy.addr_u,
             thumb_parm->src_img.addr_vir.addr_y,
             thumb_parm->src_img.addr_vir.addr_u, thumb_parm->src_img.fd,
             thumb_parm->src_img.size.width, thumb_parm->src_img.size.height,
             thumb_parm->src_img.buf_size);

    CMR_LOGD("rot addr 0x%lx 0x%lx 0x%lx 0x%lx fd 0x%x size %d %d %d",
             rot_img.addr_phy.addr_y, rot_img.addr_phy.addr_u,
             rot_img.addr_vir.addr_y, rot_img.addr_vir.addr_u, rot_img.fd,
             rot_img.rect.width, rot_img.rect.height, rot_img.buf_size);

    CMR_LOGD("dst addr 0x%lx 0x%lx add 0x%lx 0x%lx fd 0x%x size %d %d %d",
             dst.addr_phy.addr_y, dst.addr_phy.addr_u, dst.addr_vir.addr_y,
             dst.addr_vir.addr_u, dst.fd, dst.rect.width, dst.rect.height,
             dst.buf_size);

    CMR_LOGI("X slice_height=%d rot=%d", mean.slice_height, mean.rot);
    return ret;
}

void snp_set_status(cmr_handle snp_handle, cmr_uint status) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    struct camera_frame_type frame_type;
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    CMR_LOGD("set %ld", status);
    sem_wait(&cxt->access_sm);
    cxt->status = status;
    sem_post(&cxt->access_sm);
    if (IDLE == status) {
        if (cxt->oem_cb) {
            cxt->oem_cb(cxt->oem_handle, SNAPSHOT_CB_EVT_STATE,
                        SNAPSHOT_FUNC_STATE, (void *)&status);
        }
    } else {
        frame_type.status = status;
        snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE,
                                SNAPSHOT_CB_EVT_STATE, (void *)&frame_type,
                                sizeof(struct camera_frame_type));
    }
    CMR_LOGV("X");
}

cmr_uint snp_get_status(cmr_handle snp_handle) {
    struct snp_context *cxt = (struct snp_context *)snp_handle;
    cmr_uint status;

    CMR_LOGV("E");
    sem_wait(&cxt->access_sm);
    status = cxt->status;
    sem_post(&cxt->access_sm);
    CMR_LOGV("X, ret=%ld", status);
    return status;
}
/********************************* external data type
 * *********************************/

cmr_int cmr_snapshot_init(struct snapshot_init_param *param_ptr,
                          cmr_handle *snapshot_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = NULL;
    CMR_MSG_INIT(message);

    if (!param_ptr || !snapshot_handle || !param_ptr->ipm_handle ||
        !param_ptr->oem_handle || !param_ptr->oem_cb) {
        if (param_ptr) {
            CMR_LOGE("err param 0x%lx 0x%lx 0x%lx 0x%lx", (cmr_uint)param_ptr,
                     (cmr_uint)snapshot_handle, (cmr_uint)param_ptr->ipm_handle,
                     (cmr_uint)param_ptr->oem_handle);
        }
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    *snapshot_handle = (cmr_handle)0;
    cxt = (struct snp_context *)malloc(sizeof(*cxt));
    if (NULL == cxt) {
        CMR_LOGE("failed to create snp context");
        ret = -CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_bzero(cxt, sizeof(*cxt));
    cxt->camera_id = param_ptr->id;
    cxt->oem_handle = param_ptr->oem_handle;
    /*	cxt->ipm_handle = param_ptr->ipm_handle;*/
    cxt->oem_cb = param_ptr->oem_cb;
    cxt->ops = param_ptr->ops;
    cxt->caller_privdata = param_ptr->private_data;
    cxt->req_param.channel_id = SNP_INVALIDE_VALUE;
    snp_local_init((cmr_handle)cxt);
    ret = snp_create_thread((cmr_handle)cxt);
    if (ret) {
        CMR_LOGE("failed to create thread %ld", ret);
    }
exit:
    CMR_LOGD("done %ld", ret);
    if (CMR_CAMERA_SUCCESS == ret) {
        *snapshot_handle = (cmr_handle)cxt;
        cxt->is_inited = 1;
    } else {
        if (cxt) {
            free((void *)cxt);
            cxt = NULL;
        }
    }
    return ret;
}

cmr_int cmr_snapshot_deinit(cmr_handle snapshot_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(snapshot_handle);

    if (0 == cxt->is_inited) {
        CMR_LOGD("has been de-initialized");
        goto exit;
    }
    if (IDLE != snp_get_status(snapshot_handle)) {
        snp_set_request(snapshot_handle, TAKE_PICTURE_NO);
        message.msg_type = SNP_EVT_MAIN_STOP;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        message.alloc_flag = 0;
        ret = cmr_thread_msg_send(cxt->thread_cxt.main_thr_handle, &message);
        if (ret) {
            CMR_LOGE("failed to send stop msg to main thr %ld", ret);
        }
    }
    ret = snp_destroy_thread(snapshot_handle);
    if (ret) {
        CMR_LOGE("failed to destroy thr %ld", ret);
    }
    snp_local_deinit(snapshot_handle);
    free((void *)snapshot_handle);
    snapshot_handle = NULL;
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int cmr_snapshot_post_proc(cmr_handle snapshot_handle,
                               struct snapshot_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_int cnt = 0;
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    CMR_MSG_INIT(message);

    if (!snapshot_handle || !param_ptr) {
        CMR_LOGE("error param 0x%lx 0x%lx", (cmr_uint)snapshot_handle,
                 (cmr_uint)param_ptr);
        goto exit;
    }
    if (TAKE_PICTURE_NEEDED == snp_get_request((cmr_handle)cxt)) {
        CMR_LOGE("post proc is going");
        while (cnt < 50) {
            cnt++;
            if (TAKE_PICTURE_NO == snp_get_request((cmr_handle)cxt)) {
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                ret = -CMR_CAMERA_FAIL;
            }
            int u_ret = usleep(10000);
            if (u_ret)
                CMR_LOGE("usleep failed !");
        }
        if (ret == -CMR_CAMERA_FAIL) {
            CMR_LOGE("post proc is going, is_request  is abnormal");
            goto exit;
        }
    }
    ret = snp_check_post_proc_param(param_ptr);
    if (ret) {
        goto exit;
    }
    CMR_LOGD("chn id %d android zsl %d rotation snp %d, cfg cap mode %d hdr %d "
             "snp mode angle %d total num %d thumb size %d %d",
             param_ptr->channel_id, param_ptr->is_android_zsl,
             param_ptr->is_cfg_rot_cap, param_ptr->mode, param_ptr->is_hdr,
             param_ptr->rot_angle, param_ptr->total_num,
             param_ptr->jpeg_setting.thum_size.width,
             param_ptr->jpeg_setting.thum_size.height);
    message.msg_type = SNP_EVT_MAIN_START;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 0;
    message.data = param_ptr;
    ret = cmr_thread_msg_send(cxt->thread_cxt.main_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send msg to main thr %ld", ret);
        goto exit;
    }
    ret = cxt->err_code;
exit:
    CMR_LOGV("X, ret=%ld", ret);
    return ret;
}

cmr_int snp_get_buffer_id(cmr_handle snapshot_handle, void *data) {
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    struct frm_info *frame_info_ptr = (struct frm_info *)data;
    cmr_int id = 0xFFFFFFFF;
    int i = 0;

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        CMR_LOGD("chn fd 0x%x data fd 0x%x",
                 cxt->req_param.post_proc_setting.chn_out_frm[i].fd,
                 frame_info_ptr->fd);
        if (cxt->req_param.post_proc_setting.chn_out_frm[i].fd ==
            (cmr_s32)frame_info_ptr->fd) {
            id = i;
            break;
        }
    }
    CMR_LOGV("X,id=%ld", id);

    return id;
}

cmr_int cmr_snapshot_receive_data(cmr_handle snapshot_handle, cmr_int evt,
                                  void *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    struct frm_info *frame_info_ptr = (struct frm_info *)data;
    struct snp_proc_param *proc_param_ptr = &cxt->req_param.post_proc_setting;
    cmr_u32 malloc_len = 0;
    cmr_handle send_thr_handle;
    cmr_int snp_evt;
    cmr_int buffer_id;
    cmr_u32 flag = 0;
    struct frm_info chn_data;
    unsigned long src_vir, dst_vir;
    cmr_u32 width = 0, height = 0, act_width = 0, act_height = 0;

    CMR_MSG_INIT(message);
    CHECK_HANDLE_VALID(snapshot_handle);

    CMR_LOGD("evt %ld", evt);
    if ((evt >= SNPASHOT_EVT_MAX) || !data) {
        CMR_LOGE("err param %ld %p", evt, data);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    switch (evt) {
    case SNAPSHOT_EVT_CHANNEL_DONE:
        snp_evt = SNP_EVT_POSTPROC_START;
        malloc_len = sizeof(struct frm_info);
        send_thr_handle = cxt->thread_cxt.post_proc_thr_handle;

        CMR_LOGD("video %d zsl %d yaddr_vir 0x%x",
                 cxt->req_param.is_video_snapshot,
                 cxt->req_param.is_zsl_snapshot, frame_info_ptr->yaddr_vir);
        buffer_id = snp_get_buffer_id(snapshot_handle, data);
        buffer_id += frame_info_ptr->base;

        if (1 == cxt->req_param.is_video_snapshot ||
            1 == cxt->req_param.is_zsl_snapshot) {
            flag = 1;
            width =
                cxt->req_param.post_proc_setting.chn_out_frm[0].size.width;
            height =
                cxt->req_param.post_proc_setting.chn_out_frm[0].size.height;
            act_width =
                cxt->req_param.post_proc_setting.actual_snp_size.width;
            act_height =
                cxt->req_param.post_proc_setting.actual_snp_size.height;

            memcpy(&chn_data, data, sizeof(struct frm_info));
            chn_data.base = CMR_CAP0_ID_BASE;
            chn_data.frame_id = CMR_CAP0_ID_BASE;
            if (1 == cxt->req_param.is_zsl_snapshot) {
                chn_data.base = CMR_CAP1_ID_BASE;
                chn_data.frame_id = CMR_CAP1_ID_BASE;
            }
        }

        if (1 == cxt->req_param.is_video_snapshot) {
            chn_data.yaddr =
                cxt->req_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_y;
            chn_data.uaddr =
                cxt->req_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_u;
            chn_data.fd =
                cxt->req_param.post_proc_setting.chn_out_frm[0].fd;

            src_vir =
                (unsigned long)chn_data.yaddr_vir;
            dst_vir =
                (unsigned long)cxt->req_param.post_proc_setting.chn_out_frm[0]
                    .addr_vir.addr_y;
            CMR_LOGD("y src_vir = %lx dst_vir = %lx width %d height %d "
                     "channel_zoom_mode %ld",
                     src_vir, dst_vir, width, height,
                     proc_param_ptr->channel_zoom_mode);
            cmr_copy((void *)dst_vir, (void *)src_vir, width * height);

            src_vir =
                (unsigned long)chn_data.yaddr_vir + width * height;
            dst_vir =
                (unsigned long)cxt->req_param.post_proc_setting.chn_out_frm[0]
                    .addr_vir.addr_u;
            CMR_LOGD("uv src_vir = %lx dst_vir = %lx width %d height "
                     "%d act_width %d act_height %d max_size.width %d "
                     "max_size.height %d",
                     src_vir, dst_vir, width, height, act_width, act_height,
                     proc_param_ptr->max_size.width,
                     proc_param_ptr->max_size.height);

            cmr_copy((void *)dst_vir, (void *)src_vir, width * height / 2);

            // for cache coherency
            cmr_snapshot_memory_flush(
                snapshot_handle,
                &cxt->req_param.post_proc_setting.chn_out_frm[0]);
        } else if (1 == cxt->req_param.is_zsl_snapshot) {
#ifndef PERFORMANCE_OPTIMIZATION
            chn_data.yaddr =
                cxt->req_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_y;
            chn_data.uaddr =
                cxt->req_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_u;
            chn_data.fd =
                cxt->req_param.post_proc_setting.chn_out_frm[0].fd;

            src_vir =
                (unsigned long)chn_data.yaddr_vir;
            dst_vir =
                (unsigned long)cxt->req_param.post_proc_setting.chn_out_frm[0]
                    .addr_vir.addr_y;
            CMR_LOGD("y src_vir = %lx dst_vir = %lx width %d height %d "
                     "channel_zoom_mode %d",
                     src_vir, dst_vir, width, height,
                     proc_param_ptr->channel_zoom_mode);
            cmr_copy((void *)dst_vir, (void *)src_vir, width * height);

            if (ZOOM_POST_PROCESS == proc_param_ptr->channel_zoom_mode) {
                src_vir = (unsigned long)chn_data.yaddr_vir + width * height;
            } else if (ZOOM_POST_PROCESS_WITH_TRIM ==
                       proc_param_ptr->channel_zoom_mode) {
                src_vir = (unsigned long)chn_data.yaddr_vir +
                          proc_param_ptr->max_size.width *
                              proc_param_ptr->max_size.height;
            } else {
                src_vir =
                    (unsigned long)chn_data.yaddr_vir + act_width * act_height;
            }

            dst_vir =
                (unsigned long)cxt->req_param.post_proc_setting.chn_out_frm[0]
                    .addr_vir.addr_u;
            CMR_LOGD("uv src_vir = %lx dst_vir = %lx width %d height "
                     "%d act_width %d act_height %d max_size.width %d "
                     "max_size.height %d",
                     src_vir, dst_vir, width, height, act_width, act_height,
                     proc_param_ptr->max_size.width,
                     proc_param_ptr->max_size.height);

            cmr_copy((void *)dst_vir, (void *)src_vir, width * height / 2);

            // for cache coherency
            cmr_snapshot_memory_flush(
                snapshot_handle,
                &cxt->req_param.post_proc_setting.chn_out_frm[0]);
#endif
        }
        break;
    case SNAPSHOT_EVT_POSTPROC_START:
        snp_evt = SNP_EVT_POSTPROC_START;
        malloc_len = sizeof(struct frm_info);
        send_thr_handle = cxt->thread_cxt.post_proc_thr_handle;
        break;
    case SNAPSHOT_EVT_FREE_FRM:
        snp_evt = SNP_EVT_POSTPROC_FREE_FRM;
        malloc_len = sizeof(struct frm_info);
        send_thr_handle = cxt->thread_cxt.post_proc_thr_handle;
        break;
    case SNAPSHOT_EVT_RAW_PROC:
        snp_evt = SNP_EVT_PROC_CB_RAW_DONE;
        malloc_len = sizeof(struct ips_out_param);
        send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
        break;
    case SNAPSHOT_EVT_CVT_RAW_DATA:
        snp_evt = SNP_EVT_PROC_CB_RAW_DONE;
        malloc_len = sizeof(struct ips_out_param);
        send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
        break;
    case SNAPSHOT_EVT_SCALE_DONE:
        snp_evt = SNP_EVT_PROC_CB_SCALE_DONE;
        malloc_len = sizeof(struct img_frm);
        send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
        break;
    case SNAPSHOT_EVT_JPEG_ENC_DONE: {
        struct jpeg_enc_cb_param test_param = *(struct jpeg_enc_cb_param *)data;
        snp_evt = SNP_EVT_PROC_CB_JPEG_ENC_DONE;
        malloc_len = sizeof(struct jpeg_enc_cb_param);
        send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
    } break;
    case SNAPSHOT_EVT_JPEG_DEC_DONE:
        snp_evt = SNP_EVT_PROC_CB_JPEG_DEC_DONE;
        malloc_len = sizeof(struct jpeg_dec_cb_param);
        send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
        break;
    case SNAPSHOT_EVT_JPEG_ENC_ERR:
        snp_evt = SNP_EVT_PROC_CB_JPEG_ENC_ERR;
        send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
        break;
    case SNAPSHOT_EVT_JPEG_DEC_ERR:
        snp_evt = SNP_EVT_PROC_CB_JPEG_DEC_ERR;
        send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
        break;
    default:
        CMR_LOGD("don't support evt 0x%lx", evt);
        goto exit;
    }

    if (malloc_len) {
        message.data = malloc(malloc_len);
        if (!message.data) {
            CMR_LOGE("failed to malloc");
            ret = -CMR_CAMERA_NO_MEM;
            goto exit;
        }
        if (1 == flag) {
            cmr_copy(message.data, &chn_data, malloc_len);
        } else {
            cmr_copy(message.data, data, malloc_len);
        }
        message.alloc_flag = 1;
    } else {
        message.alloc_flag = 0;
    }
    message.msg_type = snp_evt;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    ret = cmr_thread_msg_send(send_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send stop msg to main thr %ld", ret);
	if (message.alloc_flag) {
		free(message.data);
		message.data = NULL;
	}
        goto exit;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int cmr_snapshot_stop(cmr_handle snapshot_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    CMR_MSG_INIT(message);

    CHECK_HANDLE_VALID(snapshot_handle);

    snp_set_request(snapshot_handle, TAKE_PICTURE_NO);
    message.msg_type = SNP_EVT_MAIN_STOP;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 0;
    ret = cmr_thread_msg_send(cxt->thread_cxt.main_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send stop msg to main thr %ld", ret);
    }
    return ret;
}

cmr_int cmr_snapshot_release_frame(cmr_handle snapshot_handle, cmr_uint index) {
    UNUSED(snapshot_handle);
    UNUSED(index);

    cmr_int ret = CMR_CAMERA_SUCCESS;
#if 0
	cmr_u32                         buf_base;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;

	if (cxt->ops.channel_free_frame) {
		buf_base = cxt->cur_frame_info.base;
		index += buf_base;
		ret = cxt->ops.channel_free_frame(cxt->oem_handle, cxt->req_param.channel_id, index);
	} else {
		CMR_LOGE("err,channel_free_frame is null");
	}
#endif
    return ret;
}

cmr_int cmr_snapshot_format_convert(cmr_handle snapshot_handle, void *data,
                                    struct img_frm *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    struct frm_info *frame_info_ptr = (struct frm_info *)data;
    CMR_MSG_INIT(message);

    if (!snapshot_handle || !data || !out_ptr) {
        CMR_LOGE("err param 0x%lx 0x%lx 0x%lx", (cmr_uint)snapshot_handle,
                 (cmr_uint)data, (cmr_uint)out_ptr);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    if (!IS_CAP_FRM(frame_info_ptr->frame_id, frame_info_ptr->base)) {
        CMR_LOGE("error frame index 0x%x", frame_info_ptr->frame_id);
        ret = -CMR_CAMERA_INVALID_FRAME;
        goto exit;
    }
    if (frame_info_ptr->channel_id != cxt->req_param.channel_id) {
        CMR_LOGE("error channel data %d %d", frame_info_ptr->channel_id,
                 cxt->req_param.channel_id);
        ret = -CMR_CAMERA_INVALID_FRAME;
        goto exit;
    }
    message.msg_type = SNP_EVT_CVT_START;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.alloc_flag = 0;
    message.sub_msg_type = NON_SNP_TRIGGER;
    message.data = data;
    ret = cmr_thread_msg_send(cxt->thread_cxt.cvt_thr_handle, &message);
    if (ret) {
        CMR_LOGE("failed to send stop msg to main thr %ld", ret);
    } else {
        snp_set_status(snapshot_handle, POST_PROCESSING);
    }
    *out_ptr = cxt->cvt.out_frame;
exit:
    return ret;
}

cmr_int cmr_snapshot_memory_flush(cmr_handle snapshot_handle,
                                  struct img_frm *img) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    cam_ion_buffer_t ion_buf;

    CMR_LOGV("E");

    cmr_bzero(&ion_buf, sizeof(cam_ion_buffer_t));
    CHECK_HANDLE_VALID(snapshot_handle);

    if (img == NULL) {
        CMR_LOGE("img = %p", img);
        goto exit;
    }

    ion_buf.fd = img->fd;
    ion_buf.addr_phy = (void *)img->addr_phy.addr_y;
    ion_buf.addr_vir = (void *)img->addr_vir.addr_y;
    if (img->fmt == CAM_IMG_FMT_BAYER_MIPI_RAW) {
        ion_buf.size = img->size.width * img->size.height * 5 / 4;
    } else {
        ion_buf.size = img->size.width * img->size.height * 3 / 2;
    }
    CMR_LOGD("flush bug info: fd=%d, addr_vir=%p, size=%ld", ion_buf.fd,
             ion_buf.addr_vir, ion_buf.size);
    ret = snp_send_msg_notify_thr(snapshot_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                  SNAPSHOT_CB_EVT_FLUSH_CACHE, (void *)&ion_buf,
                                  sizeof(cam_ion_buffer_t));
    if (ret) {
        CMR_LOGE("ret = %ld", ret);
        goto exit;
    }

    CMR_LOGV("X");
exit:
    return ret;
}

cmr_int cmr_snapshot_invalidate_cache(cmr_handle snapshot_handle,
                                      struct img_frm *img) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct snp_context *cxt = (struct snp_context *)snapshot_handle;
    cam_ion_buffer_t ion_buf;

    cmr_bzero(&ion_buf, sizeof(cam_ion_buffer_t));
    CHECK_HANDLE_VALID(snapshot_handle);

    if (img == NULL) {
        CMR_LOGE("img = %p", img);
        goto exit;
    }

    ion_buf.fd = img->fd;
    ion_buf.addr_phy = (void *)img->addr_phy.addr_y;
    ion_buf.addr_vir = (void *)img->addr_vir.addr_y;
    ion_buf.size = img->size.width * img->size.height * 3 / 2;

    ret = snp_send_msg_notify_thr(snapshot_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
                                  SNAPSHOT_CB_EVT_INVALIDATE_CACHE,
                                  (void *)&ion_buf, sizeof(cam_ion_buffer_t));
    if (ret) {
        CMR_LOGE("SNAPSHOT_CB_EVT_INVALIDATE_CACHE failed, ret = %ld", ret);
        goto exit;
    }

exit:
    return ret;
}

cmr_int camera_open_uvde(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                         struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if ((NULL == cxt) || (NULL == cxt->ipm_cxt.ipm_handle)) {
        CMR_LOGE("param invalid");
        return CMR_CAMERA_INVALID_PARAM;
    }
    ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_UVDE, in_ptr, out_ptr,
                       &cxt->ipm_cxt.uvde_handle);
    return ret;
}

cmr_int camera_close_uvde(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (cxt->ipm_cxt.uvde_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.uvde_handle);
        cxt->ipm_cxt.uvde_handle = 0;
    }
    return ret;
}

cmr_int camera_start_uvde(struct camera_context *cxt, struct img_frm *src) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct ipm_frame_in ipm_in_param;

    CMR_LOGD("uvdenoise start");
    if ((NULL == cxt) || (NULL == src)) {
        CMR_LOGE("param invalid");
        return CMR_CAMERA_INVALID_PARAM;
    }
    camera_take_snapshot_step(CMR_STEP_UVDENOISE_S);
    ret = camera_open_uvde(cxt, NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to open uvdenoise %ld", ret);
        return ret;
    }

    ipm_in_param.src_frame = *src;
    ret = ipm_transfer_frame(cxt->ipm_cxt.uvde_handle, &ipm_in_param, NULL);
    if (ret) {
        CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        return ret;
    }

    ret = camera_close_uvde(cxt);
    if (ret) {
        CMR_LOGE("fail to close uvdenoise %ld", ret);
        return ret;
    }
    camera_take_snapshot_step(CMR_STEP_UVDENOISE_E);
    CMR_LOGV("X");
    return ret;
}

cmr_int camera_open_yde(struct camera_context *cxt, struct ipm_open_in *in_ptr,
                        struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if ((NULL == cxt) || (NULL == cxt->ipm_cxt.ipm_handle)) {
        CMR_LOGE("param invalid");
        return CMR_CAMERA_INVALID_PARAM;
    }
    ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_YDE, in_ptr, out_ptr,
                       &cxt->ipm_cxt.yde_handle);

    return ret;
}

cmr_int camera_close_yde(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (cxt->ipm_cxt.yde_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.yde_handle);
        cxt->ipm_cxt.yde_handle = 0;
    }
    return ret;
}

static cmr_int camera_start_yde(struct camera_context *cxt,
                                struct img_frm *src) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct ipm_frame_in ipm_in_param;

    CMR_LOGV("E");
    if ((NULL == cxt) || (NULL == src)) {
        CMR_LOGE("param invalid");
        return CMR_CAMERA_INVALID_PARAM;
    }
    camera_take_snapshot_step(CMR_STEP_YDENOISE_S);
    ret = camera_open_yde(cxt, NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to open ydenoise %ld", ret);
        return ret;
    }

    ipm_in_param.src_frame = *src;
    ret = ipm_transfer_frame(cxt->ipm_cxt.yde_handle, &ipm_in_param, NULL);
    if (ret) {
        CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        return ret;
    }

    ret = camera_close_yde(cxt);
    if (ret) {
        CMR_LOGE("fail to close ydenoise %ld", ret);
        return ret;
    }
    camera_take_snapshot_step(CMR_STEP_YDENOISE_E);
    CMR_LOGV("X");
    return ret;
}

cmr_int camera_open_refocus(struct camera_context *cxt,
                            struct ipm_open_in *in_ptr,
                            struct ipm_open_out *out_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct ipm_open_in in_ptr1;
    struct ipm_open_out out_ptr1;
    if ((NULL == cxt) || (NULL == cxt->ipm_cxt.ipm_handle)) {
        CMR_LOGE("param invalid");
        return CMR_CAMERA_INVALID_PARAM;
    }
    in_ptr1.frame_size.width =
        cxt->snp_cxt.post_proc_setting.actual_snp_size.width;
    in_ptr1.frame_size.height =
        cxt->snp_cxt.post_proc_setting.actual_snp_size.height;
    in_ptr1.otp_data.otp_size = 8192; // otp_data->size;//TBD
    in_ptr1.otp_data.otp_ptr = NULL;  // TBD
    in_ptr1.frame_cnt = 0;
    ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_REFOCUS, &in_ptr1,
                       &out_ptr1, &cxt->ipm_cxt.refocus_handle);

    return ret;
}

cmr_int camera_close_refocus(struct camera_context *cxt) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (cxt->ipm_cxt.refocus_handle) {
        ret = cmr_ipm_close(cxt->ipm_cxt.refocus_handle);
        cxt->ipm_cxt.refocus_handle = 0;
    }
    return ret;
}

static cmr_int camera_start_refocus(struct camera_context *cxt,
                                    struct img_frm *src) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct ipm_frame_in ipm_in_param;

    CMR_LOGV("E");
    if ((NULL == cxt) || (NULL == src)) {
        CMR_LOGE("param invalid");
        return CMR_CAMERA_INVALID_PARAM;
    }
    camera_take_snapshot_step(CMR_STEP_REFOCUS_S);
    ret = camera_open_refocus(cxt, NULL, NULL);
    if (ret) {
        CMR_LOGE("failed to open ydenoise %ld", ret);
        return ret;
    }

    ipm_in_param.src_frame = *src;
    ipm_in_param.touch_x = cxt->snp_cxt.touch_xy.touchX;
    ipm_in_param.touch_y = cxt->snp_cxt.touch_xy.touchY;
    ipm_in_param.depth_map.width = 480;          // TBD
    ipm_in_param.depth_map.height = 360;         // TBD
    ipm_in_param.depth_map.depth_map_ptr = NULL; // TBD
    ret = ipm_transfer_frame(cxt->ipm_cxt.refocus_handle, &ipm_in_param, NULL);
    if (ret) {
        CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        return ret;
    }

    ret = camera_close_refocus(cxt);
    if (ret) {
        CMR_LOGE("fail to close ydenoise %ld", ret);
        return ret;
    }
    camera_take_snapshot_step(CMR_STEP_REFOCUS_E);
    CMR_LOGV("X");
    return ret;
}
