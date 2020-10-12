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

#define LOG_TAG "cmr_prev"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include "cmr_preview.h"
#include "SprdOEMCamera.h"
#include "cmr_grab.h"
#include "cmr_ipm.h"
#include "cmr_mem.h"
#include "cmr_msg.h"
#include "cmr_oem.h"
#include "cmr_sensor.h"
#include "isp_simulation.h"
#include "isp_video.h"
#ifdef CONFIG_CAMERA_SUPPORT_ULTRA_WIDE
#include "sprd_img_warp.h"
#endif
#include <cutils/properties.h>
#include <cutils/trace.h>
#include <math.h>
#include <stdlib.h>
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
#include "cmr_mm_dvfs.h"
#endif
#ifdef CONFIG_CAMERA_FDR
#include "fdr_interface.h"
#endif

#undef YUV_TO_ISP
/**************************MCARO
 * DEFINITION********************************************************************/
// abilty, max support buf num
#define PREV_FRM_CNT GRAB_BUF_MAX
#define PREV_ROT_FRM_CNT GRAB_BUF_MAX
#define ZSL_FRM_CNT GRAB_BUF_MAX
#define ZSL_ROT_FRM_CNT GRAB_BUF_MAX

// actually, the num alloced for preview/video/zsl, hal1.0 will use this
#define PREV_FRM_ALLOC_CNT 8
#define PREV_ROT_FRM_ALLOC_CNT 8
#define PREV_ULTRA_WIDE_ALLOC_CNT 8
#define VIDEO_ULTRA_WIDE_ALLOC_CNT 8
#define ZSL_ULTRA_WIDE_ALLOC_CNT 3
#define ZSL_FRM_ALLOC_CNT 8
#define FDR_FRM_ALLOC_CNT 10
#define ZSL_ROT_FRM_ALLOC_CNT 8
#define CHANNEL0_BUF_CNT 8
#define CHANNEL1_BUF_CNT 8
#define CHANNEL1_BUF_CNT_ROT 8
#define CHANNEL2_BUF_CNT 8
#define CHANNEL2_BUF_CNT_ROT 8
#define CHANNEL3_BUF_CNT 8
#define CHANNEL3_BUF_CNT_ROT 8
#define CHANNEL4_BUF_CNT 8
#define CHANNEL4_BUF_CNT_ROT 8

#define PREV_MSG_QUEUE_SIZE 50
#define PREV_RECOVERY_CNT 3

#define SIDEBYSIDE_WIDTH 3200
#define SIDEBYSIDE_MAIN_WIDTH 1600
#define SIDEBYSIDE_HEIGH 1200

#define ISP_HW_SUPPORT_MAX_ZOOM_RATIO 4

#define PREV_EVT_BASE (CMR_EVT_PREVIEW_BASE + 0x100)
#define PREV_EVT_INIT (PREV_EVT_BASE + 0x0)
#define PREV_EVT_EXIT (PREV_EVT_BASE + 0x1)
#define PREV_EVT_SET_PARAM (PREV_EVT_BASE + 0x2)
#define PREV_EVT_START (PREV_EVT_BASE + 0x3)
#define PREV_EVT_STOP (PREV_EVT_BASE + 0x4)
#define PREV_EVT_UPDATE_ZOOM (PREV_EVT_BASE + 0x5)
#define PREV_EVT_BEFORE_SET (PREV_EVT_BASE + 0x6)
#define PREV_EVT_AFTER_SET (PREV_EVT_BASE + 0x7)
#define PREV_EVT_RECEIVE_DATA (PREV_EVT_BASE + 0x8)
#define PREV_EVT_GET_POST_PROC_PARAM (PREV_EVT_BASE + 0x9)
#define PREV_EVT_FD_CTRL (PREV_EVT_BASE + 0xA)
#define PREV_EVT_CANCEL_SNP (PREV_EVT_BASE + 0xB)
#define PREV_EVT_SET_PREV_BUFFER (PREV_EVT_BASE + 0xC)
#define PREV_EVT_SET_VIDEO_BUFFER (PREV_EVT_BASE + 0xD)
#define PREV_EVT_SET_ZSL_BUFFER (PREV_EVT_BASE + 0xE)

#define PREV_EVT_CB_INIT (PREV_EVT_BASE + 0x10)
#define PREV_EVT_CB_START (PREV_EVT_BASE + 0x11)
#define PREV_EVT_CB_EXIT (PREV_EVT_BASE + 0x12)
#define PREV_EVT_ASSIST_START (PREV_EVT_BASE + 0x13)
#define PREV_EVT_ASSIST_STOP (PREV_EVT_BASE + 0x14)


#define PREV_EVT_CHANNEL0_QUEUE_BUFFER (PREV_EVT_BASE + 0x15)
#define PREV_EVT_CHANNEL1_QUEUE_BUFFER (PREV_EVT_BASE + 0x16)
#define PREV_EVT_CHANNEL2_QUEUE_BUFFER (PREV_EVT_BASE + 0x17)
#define PREV_EVT_CHANNEL3_QUEUE_BUFFER (PREV_EVT_BASE + 0x18)
#define PREV_EVT_CHANNEL4_QUEUE_BUFFER (PREV_EVT_BASE + 0x19)
#define PREV_EVT_3DNR_CALLBACK (PREV_EVT_BASE + 0x20)

#define ALIGN_16_PIXEL(x) (((x) + 15) & (~15))

#define IS_PREVIEW(handle, cam_id)                                             \
    (PREVIEWING ==                                                             \
     (cmr_uint)((struct prev_handle *)handle->prev_cxt[cam_id].prev_status))
#define IS_PREVIEW_FRM(id) ((id & CMR_PREV_ID_BASE) == CMR_PREV_ID_BASE)
#define IS_VIDEO_FRM(id) ((id & CMR_VIDEO_ID_BASE) == CMR_VIDEO_ID_BASE)
#define IS_ZSL_FRM(id) ((id & CMR_CAP1_ID_BASE) == CMR_CAP1_ID_BASE)
#define CAP_SIM_ROT(handle, cam_id)                                            \
    (((struct prev_handle *)handle)                                            \
         ->prev_cxt[cam_id]                                                    \
         .prev_param.is_cfg_rot_cap &&                                         \
     (IMG_ANGLE_0 ==                                                           \
      ((struct prev_handle *)handle)->prev_cxt[cam_id].prev_status))

#define IS_RESEARCH(search_h, h)                                               \
    (search_h != h && h >= search_h * 3 / 2 && search_h >= 1088)

#define CHECK_HANDLE_VALID(handle)                                             \
    do {                                                                       \
        if (!handle) {                                                         \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

#define CHECK_CAMERA_ID(id)                                                    \
    do {                                                                       \
        if (id >= CAMERA_ID_MAX) {                                             \
            return CMR_CAMERA_INVALID_PARAM;                                   \
        }                                                                      \
    } while (0)

/**************************LOCAL FUNCTION
 * DECLEARATION*********************************************************/
enum isp_status {
    PREV_ISP_IDLE = 0,
    PREV_ISP_COWORK,
    PREV_ISP_POST_PROCESS,
    PREV_ISP_ERR,
    PREV_ISP_MAX,
};

enum cvt_status {
    PREV_CVT_IDLE = 0,
    PREV_CVT_ROTATING,
    PREV_CVT_ROT_DONE,
    PREV_CVT_MAX,
};

enum chn_status { PREV_CHN_IDLE = 0, PREV_CHN_BUSY };

enum recovery_status {
    PREV_RECOVERY_IDLE = 0,
    PREV_RECOVERING,
    PREV_RECOVERY_DONE
};

enum recovery_mode {
    RECOVERY_LIGHTLY = 0,
    RECOVERY_MIDDLE,
    RECOVERY_HEAVY
};

struct rot_param {
    cmr_uint angle;
    struct img_frm *src_img;
    struct img_frm *dst_img;
};

// for channel0
typedef struct channel0 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL0_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    cmr_s32 fd[CHANNEL0_BUF_CNT];
    unsigned long addr_phy[CHANNEL0_BUF_CNT];
    unsigned long addr_vir[CHANNEL0_BUF_CNT];
} channel0_t;

// for channel1
typedef struct channel1 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL1_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    struct img_frm rot_frm[CHANNEL1_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL1_BUF_CNT_ROT];
    cmr_u32 rot_index;

    cmr_s32 fd[CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT];
} channel1_t;

// for channel2
typedef struct channel2 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL2_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    struct img_frm rot_frm[CHANNEL2_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL2_BUF_CNT_ROT];
    cmr_u32 rot_index;

    cmr_s32 fd[CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT];
} channel2_t;

// for channel3
typedef struct channel3 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL3_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    struct img_frm rot_frm[CHANNEL3_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL3_BUF_CNT_ROT];
    cmr_u32 rot_index;

    cmr_s32 fd[CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT];
} channel3_t;

// for channel4
typedef struct channel4 {
    cmr_u32 enable;
    struct img_size size;
    cmr_u32 buf_size;
    cmr_u32 buf_cnt;
    // valid_buf_cnt means how many buffers set to kernel driver
    cmr_u32 valid_buf_cnt;
    cmr_u32 shrink;
    cmr_u32 chn_id;
    cmr_u32 chn_status;
    cmr_u32 format;
    cmr_u32 skip_num;
    cmr_u32 skip_mode;
    unsigned long frm_cnt;
    struct img_data_end endian;
    struct img_frm frm[CHANNEL4_BUF_CNT];
    struct img_frm frm_reserved;
    cmr_u32 frm_valid;

    struct img_frm rot_frm[CHANNEL4_BUF_CNT_ROT];
    cmr_u32 rot_frm_lock_flag[CHANNEL4_BUF_CNT_ROT];
    cmr_u32 rot_index;

    cmr_s32 fd[CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT];
    unsigned long addr_phy[CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT];
    unsigned long addr_vir[CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT];
} channel4_t;

struct prev_context {
    cmr_uint camera_id;
    struct preview_param prev_param;

    cmr_int out_ret_val; /*for external function get return value*/

    /*preview*/
    struct img_size actual_prev_size;
    cmr_uint prev_status;
    cmr_uint prev_mode;
    cmr_uint latest_prev_mode;
    struct img_rect prev_rect;
    cmr_uint skip_mode;
    cmr_uint prev_channel_deci;
    cmr_uint prev_preflash_skip_en;
    cmr_uint prev_skip_num;
    cmr_uint prev_channel_id;
    cmr_uint prev_channel_status;
    struct img_data_end prev_data_endian;
    cmr_uint prev_frm_cnt;
    struct rot_param rot_param;
    cmr_s64 restart_timestamp;
    cmr_uint restart_skip_cnt;
    cmr_uint restart_skip_en;
    struct img_size lv_size;    /*isp lv size*/
    struct img_size video_size; /*isp video and scl size*/

    cmr_uint prev_self_restart;
    cmr_uint prev_buf_id;
    struct img_frm prev_frm[PREV_FRM_CNT];
    struct img_frm prev_reserved_frm;
    cmr_uint prev_rot_index;
    cmr_uint prev_rot_frm_is_lock[PREV_ROT_FRM_CNT];
    cmr_uint prev_ultra_wide_index;
    cmr_uint prev_ultra_wide_frm_is_lock[PREV_ULTRA_WIDE_ALLOC_CNT];
    struct img_frm prev_rot_frm[PREV_ROT_FRM_CNT];
    struct img_frm prev_ultra_wide_frm[PREV_ULTRA_WIDE_ALLOC_CNT];
    cmr_uint prev_phys_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint prev_virt_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_s32 prev_fd_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    void *prev_ultra_wide_handle_array[PREV_ULTRA_WIDE_ALLOC_CNT];
    cmr_uint prev_reserved_phys_addr;
    cmr_uint prev_reserved_virt_addr;
    cmr_s32 prev_reserved_fd;
    cmr_uint prev_mem_size;
    cmr_uint prev_mem_num;
    cmr_int prev_mem_valid_num;
#ifdef CONFIG_Y_IMG_TO_ISP
    cmr_uint prev_mem_y_size;
    cmr_uint prev_mem_y_num;
    cmr_uint prev_phys_y_addr_array[2];
    cmr_uint prev_virt_y_addr_array[2];
    cmr_s32 prev_mfd_y_array[2];
#endif
    cmr_uint prev_mem_yuv_size;
    cmr_uint prev_mem_yuv_num;
    cmr_uint prev_phys_yuv_addr;
    cmr_uint prev_virt_yuv_addr;
    cmr_s32 prev_mfd_yuv;
    /*video*/
    struct img_size actual_video_size;
    cmr_uint video_status;
    cmr_uint video_mode;
    struct img_rect video_rect;
    // cmr_uint                        video_skip_mode;
    // cmr_uint                        prev_skip_num;
    cmr_uint video_channel_id;
    cmr_uint video_channel_status;
    struct img_data_end video_data_endian;
    cmr_uint video_frm_cnt;
    struct rot_param video_rot_param;
    cmr_s64 video_restart_timestamp;
    cmr_uint video_restart_skip_cnt;
    cmr_uint video_restart_skip_en;

    cmr_uint video_self_restart;
    cmr_uint video_buf_id;
    struct img_frm video_frm[PREV_FRM_CNT];
    struct img_frm video_reserved_frm;
    cmr_uint video_rot_index;
    cmr_uint video_rot_frm_is_lock[PREV_ROT_FRM_CNT];
    cmr_uint video_ultra_wide_index;
    cmr_uint video_ultra_wide_frm_is_lock[VIDEO_ULTRA_WIDE_ALLOC_CNT];
    struct img_frm video_rot_frm[PREV_ROT_FRM_CNT];
    struct img_frm video_ultra_wide_frm[VIDEO_ULTRA_WIDE_ALLOC_CNT];
    cmr_uint video_phys_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_uint video_virt_addr_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    cmr_s32 video_fd_array[PREV_FRM_CNT + PREV_ROT_FRM_CNT];
    void *video_ultra_wide_handle_array[VIDEO_ULTRA_WIDE_ALLOC_CNT];
    cmr_uint video_reserved_phys_addr;
    cmr_uint video_reserved_virt_addr;
    cmr_s32 video_reserved_fd;
    cmr_uint video_mem_size;
    cmr_uint video_mem_num;
    cmr_int video_mem_valid_num;
    cmr_int cache_buffer_cont;

    cmr_u32 eis_video_mem_size;
    cmr_u32 eis_video_ultra_wide_mem_num;
    cmr_uint eis_video_phys_addr;
    cmr_uint eis_video_virt_addr;
    cmr_s32 eis_video_fd;
    void *dst_eis_video_buffer_handle;
    sem_t ultra_video;

    // for channel0
    channel0_t channel0;
    cmr_uint channel0_work_mode;

    // for channel1
    channel1_t channel1;
    struct img_size channel1_actual_pic_size;
    cmr_uint channel1_work_mode;

    // for channel2
    channel2_t channel2;
    struct img_size channel2_actual_pic_size;
    cmr_uint channel2_work_mode;

    // for channel3
    channel3_t channel3;
    struct img_size channel3_actual_pic_size;
    cmr_uint channel3_work_mode;

    // for channel4
    channel4_t channel4;
    struct img_size channel4_actual_pic_size;
    cmr_uint channel4_work_mode;

    /*capture*/
    cmr_uint cap_mode;
    struct img_size max_size;
    struct img_size aligned_pic_size;
    struct img_size actual_pic_size;
    struct img_size dealign_actual_pic_size;
    struct channel_start_param restart_chn_param;
    cmr_uint cap_channel_id;
    cmr_uint cap_channel_status;
    cmr_uint zsl_channel_status;
    struct img_data_end cap_data_endian;
    cmr_uint cap_frm_cnt;
    cmr_uint cap_skip_num;
    cmr_uint cap_org_fmt;
    struct img_size cap_org_size;
    cmr_uint cap_zoom_mode;
    struct img_rect cap_sn_trim_rect;
    struct img_size cap_sn_size;
    struct img_rect cap_scale_src_rect;

    cmr_uint cap_phys_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_virt_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 cap_fd_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_phys_addr_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_virt_addr_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 cap_fd_path_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint cap_hdr_phys_addr_path_array[HDR_CAP_NUM];
    cmr_uint cap_hdr_virt_addr_path_array[HDR_CAP_NUM];
    cmr_s32 cap_hdr_fd_path_array[HDR_CAP_NUM];
    cmr_uint cap_3dnr_phys_addr_path_array[CAP_3DNR_NUM];
    cmr_uint cap_3dnr_virt_addr_path_array[CAP_3DNR_NUM];
    cmr_s32 cap_3dnr_fd_path_array[CAP_3DNR_NUM];
    cmr_uint super_phys_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_uint super_virt_addr_array[CMR_CAPTURE_MEM_SUM];
    cmr_s32 super_fd_array[CMR_CAPTURE_MEM_SUM];

    struct cmr_cap_mem cap_mem[CMR_CAPTURE_MEM_SUM];
    struct img_frm cap_frm[CMR_CAPTURE_MEM_SUM];
    cmr_uint is_zsl_frm;

    cmr_s64 cap_zsl_restart_timestamp;
    cmr_uint cap_zsl_restart_skip_cnt;
    cmr_uint cap_zsl_restart_skip_en;
    cmr_uint cap_zsl_frm_cnt;
    struct img_frm cap_zsl_frm[ZSL_FRM_CNT];
    struct img_frm cap_zsl_reserved_frm;
    cmr_uint cap_zsl_rot_index;
    cmr_uint cap_zsl_rot_frm_is_lock[ZSL_ROT_FRM_CNT];
    struct img_frm cap_zsl_rot_frm[ZSL_ROT_FRM_CNT];
    cmr_uint cap_zsl_ultra_wide_index;
    cmr_uint cap_zsl_ultra_wide_frm_is_lock[ZSL_ULTRA_WIDE_ALLOC_CNT];
    struct img_frm cap_zsl_ultra_wide_frm[ZSL_ULTRA_WIDE_ALLOC_CNT];

    cmr_uint cap_zsl_phys_addr_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_uint cap_zsl_virt_addr_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    cmr_s32 cap_zsl_fd_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    void *cap_zsl_ultra_wide_handle_array[ZSL_ROT_FRM_CNT];
    void *cap_zsl_3dnr_handle_array[ZSL_ROT_FRM_CNT];
    cmr_s32 cap_zsl_dst_fd_array[ZSL_FRM_CNT + ZSL_ROT_FRM_CNT];
    void *cap_zsl_dst_handle_array[ZSL_FRM_CNT];
    cmr_uint cap_zsl_reserved_phys_addr;
    cmr_uint cap_zsl_reserved_virt_addr;
    cmr_s32 cap_zsl_reserved_fd;

    cmr_uint cap_zsl_mem_size;
    cmr_uint cap_zsl_mem_num;
    cmr_int cap_zsl_mem_valid_num;

    cmr_uint cap_4in1_phys_addr_array[CAP_4IN1_NUM];
    cmr_uint cap_4in1_virt_addr_array[CAP_4IN1_NUM];
    cmr_s32 cap_4in1_fd_array[CAP_4IN1_NUM];
    cmr_uint cap_4in1_mem_size;
    cmr_uint cap_4in1_mem_num;
    cmr_int cap_4in1_mem_valid_num;

    cmr_uint cap_fdr_phys_addr_array[FDR_FRM_ALLOC_CNT];
    cmr_uint cap_fdr_virt_addr_array[FDR_FRM_ALLOC_CNT];
    cmr_s32 cap_fdr_fd_array[FDR_FRM_ALLOC_CNT];
    cmr_uint cap_fdr_mem_size;
    cmr_u32 cap_fdr_mem_num;
    cmr_int cap_fdr_mem_valid_num;
    struct buffer_cfg cap_used_fdr_buf_cfg;

    cmr_uint is_reprocessing;
    cmr_uint capture_scene_mode;

    struct touch_coordinate touch_info;

    /*common*/
    struct img_size dcam_output_size;
    cmr_u32 need_isp;
    cmr_u32 need_binning;
    cmr_handle fd_handle;
    cmr_handle ultra_wide_handle;
    cmr_handle zsl_ultra_wide_handle;
    cmr_handle video_ultra_wide_handle;
    cmr_handle refocus_handle;
    cmr_handle prev_3dnr_handle;
    cmr_handle ai_scence_handle;
    cmr_handle auto_tracking_handle;
    cmr_uint recovery_status;
    cmr_uint recovery_cnt;
    cmr_uint recovery_en;
    cmr_uint isp_status;
    struct sensor_exp_info sensor_info;
    cmr_uint ae_time;
    void *private_data;
    cmr_uint vcm_step;
    cmr_u32 auto_tracking_cnt;
    cmr_u32 auto_tracking_inited;
    cmr_s32 auto_tracking_start_x;
    cmr_s32 auto_tracking_start_y;
    cmr_s32 auto_tracking_status;
    cmr_s32 auto_tracking_frame_id;
    /* face detect */
    cmr_s32 auto_tracking_last_frame;
    cmr_u32 ae_stab[AE_CB_MAX_INDEX];
    cmr_u32 hist[CAMERA_ISP_HIST_ITEMS];
    cmr_u32 af_status;
    cmr_uint threednr_cap_smallwidth;
    cmr_uint threednr_cap_smallheight;
    bool prev_zoom;
    bool cap_zoom;
    bool video_zoom;

    //20191030
    cmr_u32 sensor_out_width;
    cmr_u32 sensor_out_height;
    /* super macro */
    cmr_uint is_super;
};

struct prev_thread_cxt {
    cmr_uint is_inited;
    cmr_handle thread_handle;
    sem_t prev_sync_sem;
    sem_t prev_recovery_sem;
    pthread_mutex_t prev_mutex;
    pthread_mutex_t prev_stop_mutex;
    /*callback thread*/
    cmr_handle cb_thread_handle;
    cmr_handle assist_thread_handle;
};

struct prev_handle {
    cmr_handle oem_handle;
    cmr_handle ipm_handle;
    cmr_uint sensor_bits; // multi-sensors need multi mem ? channel_cfg
    preview_cb_func oem_cb;
    struct preview_md_ops ops;
    void *private_data;
    struct prev_thread_cxt thread_cxt;
    struct prev_context prev_cxt[CAMERA_ID_MAX];
    cmr_uint frame_active;
};

struct prev_cb_info {
    enum preview_cb_type cb_type;
    enum preview_func_type func_type;
    struct camera_frame_type *frame_data;
};

struct internal_param {
    void *param1;
    void *param2;
    void *param3;
    void *param4;
};

/**************************LOCAL FUNCTION
 * DECLEARATION*********************************************************/
static cmr_int prev_create_thread(struct prev_handle *handle);

static cmr_int prev_destroy_thread(struct prev_handle *handle);

static cmr_int prev_create_cb_thread(struct prev_handle *handle);

static cmr_int prev_destroy_cb_thread(struct prev_handle *handle);

static cmr_int prev_assist_thread_proc(struct cmr_msg *message, void *p_data);

static cmr_int prev_thread_proc(struct cmr_msg *message, void *p_data);

static cmr_int prev_cb_thread_proc(struct cmr_msg *message, void *p_data);

static cmr_int prev_cb_start(struct prev_handle *handle,
                             struct prev_cb_info *cb_info);

static cmr_int prev_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *data);

static cmr_int prev_preview_frame_handle(struct prev_handle *handle,
                                         cmr_u32 camera_id,
                                         struct frm_info *data);

static cmr_int prev_video_frame_handle(struct prev_handle *handle,
                                       cmr_u32 camera_id,
                                       struct frm_info *data);

static cmr_int prev_capture_frame_handle(struct prev_handle *handle,
                                         cmr_u32 camera_id,
                                         struct frm_info *data);

static cmr_int prev_zsl_frame_handle(struct prev_handle *handle,
                                     cmr_u32 camera_id, struct frm_info *data);

static cmr_int prev_error_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_uint evt_type);

static cmr_int prev_recovery_pre_proc(struct prev_handle *handle,
                                      cmr_u32 camera_id,
                                      enum recovery_mode mode);

static cmr_int prev_recovery_post_proc(struct prev_handle *handle,
                                       cmr_u32 camera_id,
                                       enum recovery_mode mode);

static cmr_int prev_recovery_reset(struct prev_handle *handle,
                                   cmr_u32 camera_id);

static cmr_int prev_local_init(struct prev_handle *handle);

static cmr_int prev_local_deinit(struct prev_handle *handle);

static cmr_int prev_pre_set(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_post_set(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_start(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_u32 is_restart, cmr_u32 is_sn_reopen);

static cmr_int prev_stop(struct prev_handle *handle, cmr_u32 camera_id,
                         cmr_u32 is_restart);

static cmr_int prev_cancel_snapshot(struct prev_handle *handle,
                                    cmr_u32 camera_id);

static cmr_int prev_alloc_prev_buf(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart,
                                   struct buffer_cfg *buffer);

static cmr_int prev_free_prev_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart);

static cmr_int prev_alloc_video_buf(struct prev_handle *handle,
                                    cmr_u32 camera_id, cmr_u32 is_restart,
                                    struct buffer_cfg *buffer);

static cmr_int prev_free_video_buf(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart);

static cmr_int prev_alloc_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart,
                                  struct buffer_cfg *buffer);

static cmr_int prev_free_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_u32 is_restart);

static cmr_int prev_alloc_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart,
                                  struct buffer_cfg *buffer);

static cmr_int prev_free_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_u32 is_restart);

static cmr_int prev_free_zsl_raw_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_u32 is_restart);

static cmr_int prev_alloc_4in1_buf(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart);

static cmr_int check_software_remosaic(struct prev_context *prev_cxt);

static cmr_int prev_free_4in1_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart);

static cmr_int prev_free_cap_reserve_buf(struct prev_handle *handle,
                                         cmr_u32 camera_id, cmr_u32 is_restart);

static cmr_int prev_get_sensor_mode(struct prev_handle *handle,
                                    cmr_u32 camera_id);

static cmr_int prev_get_sn_preview_mode(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct sensor_exp_info *sensor_info,
                                        struct img_size *target_size,
                                        cmr_uint *work_mode);
cmr_int prev_get_sn_capture_mode(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct sensor_exp_info *sensor_info,
                                 struct img_size *target_size,
                                 cmr_uint *work_mode);

static cmr_int prev_get_sn_inf(struct prev_handle *handle, cmr_u32 camera_id,
                               cmr_u32 frm_deci, struct sensor_if *sn_if);

static cmr_int prev_get_cap_max_size(struct prev_handle *handle,
                                     cmr_u32 camera_id,
                                     struct sensor_mode_info *sn_mode,
                                     struct img_size *max_size);

static cmr_int prev_construct_frame(struct prev_handle *handle,
                                    cmr_u32 camera_id, struct frm_info *info,
                                    struct camera_frame_type *frame_type);

static cmr_int prev_construct_video_frame(struct prev_handle *handle,
                                          cmr_u32 camera_id,
                                          struct frm_info *info,
                                          struct camera_frame_type *frame_type);

static cmr_int prev_construct_zsl_frame(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct frm_info *info,
                                        struct camera_frame_type *frame_type);

static cmr_int prev_set_param_internal(struct prev_handle *handle,
                                       cmr_u32 camera_id, cmr_u32 is_restart,
                                       struct preview_out_param *out_param_ptr);

static cmr_int prev_set_prev_param(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart,
                                   struct preview_out_param *out_param_ptr);

static cmr_int prev_set_video_param(struct prev_handle *handle,
                                    cmr_u32 camera_id, cmr_u32 is_restart,
                                    struct preview_out_param *out_param_ptr);

static cmr_int prev_set_prev_param_lightly(struct prev_handle *handle,
                                           cmr_u32 camera_id);

cmr_int channel0_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr);
cmr_int channel0_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer);
cmr_int channel0_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart);
cmr_s32 channel0_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer);
int channel0_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info);

cmr_int channel1_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr);
cmr_int channel1_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer);
cmr_int channel1_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart);
cmr_s32 channel1_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer);
int channel1_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info);
cmr_int channel1_update_params(struct prev_handle *handle, cmr_u32 camera_id);
cmr_s32 channel1_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index);

cmr_int channel2_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr);
cmr_int channel2_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer);
cmr_int channel2_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart);
cmr_s32 channel2_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer);
int channel2_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info);
cmr_int channel2_update_params(struct prev_handle *handle, cmr_u32 camera_id);
cmr_s32 channel2_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index);

cmr_int channel3_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr);
cmr_int channel3_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer);
cmr_int channel3_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart);
cmr_s32 channel3_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer);
int channel3_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info);
cmr_int channel3_update_params(struct prev_handle *handle, cmr_u32 camera_id);
cmr_s32 channel3_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index);

cmr_int channel4_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr);
cmr_int channel4_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer);
cmr_int channel4_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart);
cmr_s32 channel4_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer);
int channel4_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info);
cmr_int channel4_update_params(struct prev_handle *handle, cmr_u32 camera_id);
cmr_s32 channel4_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index);
cmr_int threednr_sw_prev_callback_process(struct ipm_frame_out *cb_parm);
static cmr_int prev_set_cap_param(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart, cmr_u32 is_lightly,
                                  struct preview_out_param *out_param_ptr);

static cmr_int prev_update_cap_param(struct prev_handle *handle,
                                     cmr_u32 camera_id, cmr_u32 encode_angle,
                                     struct snp_proc_param *out_param_ptr);

static cmr_int prev_set_zsl_param_lightly(struct prev_handle *handle,
                                          cmr_u32 camera_id);

static cmr_int prev_set_cap_param_raw(struct prev_handle *handle,
                                      cmr_u32 camera_id, cmr_u32 is_restart,
                                      struct preview_out_param *out_param_ptr);

static cmr_int prev_cap_ability(struct prev_handle *handle, cmr_u32 camera_id,
                                struct img_size *cap_size,
                                struct img_frm_cap *img_cap);

static cmr_int prev_get_scale_rect(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 rot,
                                   struct snp_proc_param *cap_post_proc_param);

static cmr_int prev_before_set_param(struct prev_handle *handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode);

static cmr_int prev_after_set_param(struct prev_handle *handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number);

static cmr_uint prev_get_rot_val(cmr_uint rot_enum);

static cmr_uint prev_get_rot_enum(cmr_uint rot_val);

static cmr_uint prev_set_rot_buffer_flag(struct prev_context *prev_cxt,
                                         cmr_uint type, cmr_int index,
                                         cmr_uint flag);
static cmr_uint prev_set_ultra_wide_buffer_flag(struct prev_context *prev_cxt,
                                                cmr_uint type, cmr_int index,
                                                cmr_uint flag);

static cmr_uint prev_search_rot_buffer(struct prev_context *prev_cxt,
                                       cmr_uint type);
static cmr_uint prev_search_ultra_wide_buffer(struct prev_context *prev_cxt,
                                              cmr_uint type);
static cmr_uint prev_get_src_rot_buffer(struct prev_context *prev_cxt,
                                        struct frm_info *data, cmr_uint *index);

cmr_uint prev_get_src_ultra_wide_buffer(struct prev_context *prev_cxt,
                                        struct frm_info *data, cmr_uint *index);

static cmr_int prev_start_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *data);

static cmr_int
prev_get_cap_post_proc_param(struct prev_handle *handle, cmr_u32 camera_id,
                             cmr_u32 encode_angle,
                             struct snp_proc_param *out_param_ptr);

static cmr_int prev_receive_data(struct prev_handle *handle, cmr_u32 camera_id,
                                 cmr_uint evt, struct frm_info *data);

static cmr_int prev_pause_cap_channel(struct prev_handle *handle,
                                      cmr_u32 camera_id, struct frm_info *data);

static cmr_int prev_resume_cap_channel(struct prev_handle *handle,
                                       cmr_u32 camera_id,
                                       struct frm_info *data);

static cmr_int prev_restart_cap_channel(struct prev_handle *handle,
                                        cmr_u32 camera_id,
                                        struct frm_info *data);

static cmr_int prev_fd_open(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_fd_close(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_fd_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct img_frm *frm);
static cmr_int prev_ultra_wide_open(struct prev_handle *handle,
                                    cmr_u32 camera_id);

static cmr_int prev_ultra_wide_close(struct prev_handle *handle,
                                     cmr_u32 camera_id);

static cmr_int prev_ultra_wide_send_data(struct prev_handle *handle,
                                         cmr_u32 camera_id,
                                         struct frm_info *data);
static cmr_int prev_fd_cb(cmr_u32 class_type, struct ipm_frame_out *cb_param);

static cmr_int prev_fd_ctrl(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 on_off);

static cmr_int prev_3dnr_open(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_3dnr_close(struct prev_handle *handle, cmr_u32 camera_id);

static cmr_int prev_3dnr_send_data(struct prev_handle *handle,
                                   cmr_u32 camera_id, struct frm_info *frm_info,
                                   struct camera_frame_type *frame_type,
                                   struct img_frm *frm,
                                   struct img_frm *video_frm);


static cmr_int prev_ai_scene_send_data(struct prev_handle *handle,
                                       cmr_u32 camera_id, struct img_frm *frm,
                                       struct frm_info *frm_info);

static cmr_int prev_auto_tracking_open(struct prev_handle *handle,
                                       cmr_u32 camera_id);
static cmr_int prev_auto_tracking_close(struct prev_handle *handle,
                                        cmr_u32 camera_id);
static cmr_int prev_auto_tracking_send_data(struct prev_handle *handle,
                                            cmr_u32 camera_id,
                                            struct img_frm *frm,
                                            struct frm_info *frm_info);
static cmr_int prev_auto_tracking_cb(cmr_u32 class_type,
                                     struct ipm_frame_out *cb_param);

static cmr_int prev_set_preview_buffer(struct prev_handle *handle,
                                       cmr_u32 camera_id,
                                       cam_buffer_info_t *buffer);

static cmr_int prev_pop_preview_buffer(struct prev_handle *handle,
                                       cmr_u32 camera_id, struct frm_info *data,
                                       cmr_u32 is_to_hal);
static cmr_int prev_pop_video_buffer_sw_3dnr(struct prev_handle *handle,
                                             cmr_u32 camera_id,
                                             struct frm_info *data,
                                             cmr_u32 is_to_hal);
static cmr_int prev_clear_preview_buffers(struct prev_handle *handle,
                                          cmr_u32 camera_id);

static cmr_int prev_set_video_buffer(struct prev_handle *handle,
                                     cmr_u32 camera_id,
                                     cam_buffer_info_t *buffer);

static cmr_int prev_pop_video_buffer(struct prev_handle *handle,
                                     cmr_u32 camera_id, struct frm_info *data,
                                     cmr_u32 is_to_hal);

static cmr_int prev_set_zsl_buffer(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_uint src_phy_addr,
                                   cmr_uint src_vir_addr, cmr_s32 fd);

static cmr_int prev_pop_zsl_buffer(struct prev_handle *handle,
                                   cmr_u32 camera_id, struct frm_info *data,
                                   cmr_u32 is_to_hal);

static cmr_int prev_capture_zoom_post_cap(struct prev_handle *handle,
                                          cmr_int *flag, cmr_u32 camera_id);
cmr_int prev_get_frm_index(struct img_frm *frame, struct frm_info *data);
cmr_int prev_is_need_scaling(cmr_handle preview_handle, cmr_u32 camera_id);
static cmr_int camera_get_mem_size(cmr_handle preview_handle, cmr_u32 camera_id);

/**************************FUNCTION
 * ***************************************************************************/
cmr_int cmr_preview_init(struct preview_init_param *init_param_ptr,
                         cmr_handle *preview_handle_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = NULL;

    if (!preview_handle_ptr || !init_param_ptr) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p", preview_handle_ptr,
                 init_param_ptr);
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)malloc(sizeof(struct prev_handle));
    if (!handle) {
        CMR_LOGE("No mem!");
        return CMR_CAMERA_NO_MEM;
    }
    cmr_bzero(handle, sizeof(struct prev_handle));

    /*save init param*/
    handle->oem_handle = init_param_ptr->oem_handle;
    handle->ipm_handle = init_param_ptr->ipm_handle;
    handle->sensor_bits = init_param_ptr->sensor_bits;
    handle->oem_cb = init_param_ptr->oem_cb;
    handle->ops = init_param_ptr->ops;
    handle->private_data = init_param_ptr->private_data;
    CMR_LOGI("oem_handle: %p, sensor_bits: %ld, private_data: %p",
             handle->oem_handle, handle->sensor_bits, handle->private_data);

    /*create thread*/
    ret = prev_create_thread(handle);
    if (ret) {
        CMR_LOGE("create thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto init_end;
    }

    /*create callback thread*/
    ret = prev_create_cb_thread(handle);
    if (ret) {
        CMR_LOGE("create cb thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto init_end;
    }

    /*return handle*/
    *preview_handle_ptr = (cmr_handle)handle;
    CMR_LOGD("preview_handle_ptr=%p created", handle);

init_end:
    if (ret) {
        if (handle) {
            free(handle);
            handle = NULL;
        }
    }

    return ret;
}

cmr_int cmr_preview_deinit(cmr_handle preview_handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 i = 0;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    if (!preview_handle) {
        CMR_LOGE("Invalid param!");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("E");
    /*check every device, if previewing, stop it*/
    for (i = 0; i < CAMERA_ID_MAX; i++) {
        CMR_LOGV("id %d, prev_status %ld,4in1_mem_num=%d",
                 i, handle->prev_cxt[i].prev_status,handle->prev_cxt[i].cap_4in1_mem_num);
        if (handle->prev_cxt[i].cap_4in1_mem_num != 0) {
           prev_free_4in1_buf(handle,i,0);
        }
        if (PREVIEWING == handle->prev_cxt[i].prev_status) {
            /*prev_stop(handle, i, 0);*/
            cmr_preview_stop(preview_handle, i);
        }
    }

    ret = prev_destroy_thread(handle);
    if (ret) {
        CMR_LOGE("destory thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto deinit_end;
    }

    ret = prev_destroy_cb_thread(handle);
    if (ret) {
        CMR_LOGE("destory cb thread failed!");
        ret = CMR_CAMERA_FAIL;
        goto deinit_end;
    }

    if (handle) {
        free(handle);
        handle = NULL;
    }

    CMR_LOGD("X");

deinit_end:
    return ret;
}

cmr_int cmr_preview_set_param(cmr_handle preview_handle, cmr_u32 camera_id,
                              struct preview_param *param_ptr,
                              struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_int call_ret = CMR_CAMERA_SUCCESS;
    struct internal_param *inter_param = NULL;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    if (!preview_handle || !param_ptr || !out_param_ptr) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", preview_handle, param_ptr,
                 out_param_ptr);
        return CMR_CAMERA_INVALID_PARAM;
    }

    CHECK_CAMERA_ID(camera_id);
    CMR_LOGD("camera_id %d frame count %d", camera_id, param_ptr->frame_count);

    /*save the preview param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)param_ptr;
    inter_param->param3 = (void *)out_param_ptr;

    message.msg_type = PREV_EVT_SET_PARAM;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    } else {
        call_ret = handle->prev_cxt[camera_id].out_ret_val;
        CMR_LOGV("call ret %ld", call_ret);
    }

    ATRACE_END();
    return ret | call_ret;
}

cmr_int cmr_preview_start(cmr_handle preview_handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGD("E");

    message.msg_type = PREV_EVT_ASSIST_START;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    message.msg_type = PREV_EVT_START;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)camera_id);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int cmr_preview_stop(cmr_handle preview_handle, cmr_u32 camera_id) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGD("E");

    message.msg_type = PREV_EVT_ASSIST_STOP;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    message.msg_type = PREV_EVT_STOP;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)camera_id);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGD("X");
    return ret;
}

cmr_int cmr_preview_cancel_snapshot(cmr_handle preview_handle,
                                    cmr_u32 camera_id) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGD("E");

    message.msg_type = PREV_EVT_CANCEL_SNP;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)((unsigned long)camera_id);
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGD("X");
    return ret;
}

cmr_int cmr_preview_get_status(cmr_handle preview_handle, cmr_u32 camera_id) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    if (!handle || (camera_id >= CAMERA_ID_MAX)) {
        CMR_LOGE("invalid param, handle %p, camera_id %d", handle, camera_id);
        return ERROR;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    /*CMR_LOGD("prev_status %ld", prev_cxt->prev_status); */
    return (cmr_int)prev_cxt->prev_status;
}

void cmr_preview_wait_recovery(cmr_handle preview_handle, cmr_u32 camera_id){
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    if (!handle || (camera_id >= CAMERA_ID_MAX)) {
        CMR_LOGE("invalid param, handle %p, camera_id %d", handle, camera_id);
        return;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    sem_wait(&handle->thread_cxt.prev_recovery_sem);
    prev_cxt->recovery_en = 0;
    sem_post(&handle->thread_cxt.prev_recovery_sem);
}

cmr_int cmr_preview_get_prev_rect(cmr_handle preview_handle, cmr_u32 camera_id,
                                  struct img_rect *rect) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!rect) {
        CMR_LOGE("rect is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    cmr_copy(rect, &prev_cxt->prev_rect, sizeof(struct img_rect));

    return ret;
}

cmr_int cmr_preview_get_fdr_zsl_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                 struct img_frm *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    //struct img_frm src_param;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    if (prev_cxt->prev_param.is_ultra_wide) {
        if(prev_cxt->cap_zsl_ultra_wide_frm[0].fd != 0) {
            cmr_copy(data, &(prev_cxt->cap_zsl_ultra_wide_frm[0]), sizeof(struct img_frm));
            CMR_LOGD("ultra wide after copy, buf fd=%d, width=%d, height=%d ",
                  data->fd,
                  data->size.width,
                  data->size.height);
        } else {
            CMR_LOGE("No valid ultra wide buffer for fdr yuv postproc. ");
        }

    } else if(prev_cxt->cap_zsl_mem_valid_num>0) {
       CMR_LOGD("zsl frm, buf fd=%d, width=%d, height=%d, vir_yaddr=0x%lx",
                  prev_cxt->cap_zsl_frm[0].fd,
                  prev_cxt->cap_zsl_frm[0].size.width,
                  prev_cxt->cap_zsl_frm[0].size.height,
                  prev_cxt->cap_zsl_frm[0].addr_vir.addr_y);

       //data = &(prev_cxt->cap_zsl_frm[0]);
        cmr_copy(data, &(prev_cxt->cap_zsl_frm[0]), sizeof(struct img_frm));
        CMR_LOGD("after copy, buf fd=%d, width=%d, height=%d ",
                  data->fd,
                  data->size.width,
                  data->size.height);
   } else {
      CMR_LOGE("No valid zsl buffer for fdr yuv postproc. ");
   }

   return ret;
}

cmr_int cmr_preview_get_fdr_sn_size(cmr_handle preview_handle, cmr_u32 camera_id,
                                 struct img_size *pic_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    //struct img_frm src_param;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    pic_size->width = prev_cxt->cap_sn_size.width;
    pic_size->height = prev_cxt->cap_sn_size.height;

    CMR_LOGD("get capture sensor size: w:%d, h:%d", pic_size->width, pic_size->height);

    return ret;
}

cmr_int cmr_preview_get_fdr_free_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                 struct buffer_cfg *free_buf_cfg) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    struct buffer_cfg *used_buf_cfg = NULL;
    cmr_u32 i = 0, j = 0, k = 0;
    cmr_u32 fd = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(free_buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    used_buf_cfg = &prev_cxt->cap_used_fdr_buf_cfg;

    CMR_LOGD("E cap_fdr_mem_num:%d", prev_cxt->cap_fdr_mem_num);
    for(i = 0; i < prev_cxt->cap_fdr_mem_num; i++) {
        fd = prev_cxt->cap_fdr_fd_array[i];
        for (j = 0; j < used_buf_cfg->count; j++) {
            if(fd == used_buf_cfg->fd[j]) {
                CMR_LOGD("E used buff fd:%d", used_buf_cfg->fd[j]);
                break;
            }
        }
        if (j >= used_buf_cfg->count) {
            CMR_LOGD("get free buffer: fd:%d", fd);
            k = free_buf_cfg->count;
            free_buf_cfg->count++;
            free_buf_cfg->length = (cmr_u32)prev_cxt->cap_fdr_mem_size;
            free_buf_cfg->is_fdr = 1;
            free_buf_cfg->addr[k].addr_y = prev_cxt->cap_fdr_phys_addr_array[i];
            free_buf_cfg->addr_vir[k].addr_y = prev_cxt->cap_fdr_virt_addr_array[i];
            free_buf_cfg->fd[k] = prev_cxt->cap_fdr_fd_array[i];
            CMR_LOGD("get free buffer: count:%d fd:%d, length%ld, addr:0x%x, 0x%x",
                        free_buf_cfg->count, fd, free_buf_cfg->length,
                        free_buf_cfg->addr[k].addr_y, free_buf_cfg->addr_vir[k].addr_y);
        }
    }

    return ret;
}

cmr_int cmr_preview_set_fdr_used_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                 cmr_s32 fd) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    struct buffer_cfg *buf_cfg = NULL;
    cmr_u32 i = 0, j = 0;
    //struct img_frm src_param;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGD("E fd=%d", fd);
    prev_cxt = &handle->prev_cxt[camera_id];
    buf_cfg = &prev_cxt->cap_used_fdr_buf_cfg;
    for(i=0; i< prev_cxt->cap_fdr_mem_num; i++) {
        if (fd == prev_cxt->cap_fdr_fd_array[i]) {
            CMR_LOGD("get correct fd i:%d", i);
            break;
        }
    }

    buf_cfg->channel_id = prev_cxt->cap_channel_id;
    j = buf_cfg->count;
    buf_cfg->count++;
    buf_cfg->length = (cmr_u32)prev_cxt->cap_fdr_mem_size;
    buf_cfg->is_fdr = 1;
    buf_cfg->addr[j].addr_y = prev_cxt->cap_fdr_phys_addr_array[i];
    buf_cfg->addr_vir[j].addr_y = prev_cxt->cap_fdr_virt_addr_array[i];
    buf_cfg->fd[j] = prev_cxt->cap_fdr_fd_array[i];

    CMR_LOGD("used buffer count:%d, length:%ld, channel_id:%d, "
              "addr_y: 0x%x, vir_addry: 0x%x, fd: %d", buf_cfg->count,
              buf_cfg->length, buf_cfg->channel_id, buf_cfg->addr[j].addr_y,
              buf_cfg->addr_vir[j].addr_y, buf_cfg->fd[j]);

    return ret;
}

cmr_int cmr_preview_cfg_fdr_buffer(cmr_handle preview_handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    struct buffer_cfg buf_cfg;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    CMR_LOGD("fdr prev_mem_size %ld, mem_num %ld",
               prev_cxt->cap_fdr_mem_size, prev_cxt->cap_fdr_mem_num);

    buf_cfg.channel_id = prev_cxt->cap_channel_id;
    buf_cfg.base_id = CMR_CAP1_ID_BASE;
    buf_cfg.count = prev_cxt->cap_fdr_mem_num;
    buf_cfg.length = (cmr_u32)prev_cxt->cap_fdr_mem_size;
    buf_cfg.is_fdr = 1;
    for (cmr_u32 i = 0; i < buf_cfg.count; i++) {
        buf_cfg.addr[i].addr_y = prev_cxt->cap_fdr_phys_addr_array[i];
        buf_cfg.addr_vir[i].addr_y = prev_cxt->cap_fdr_virt_addr_array[i];
        buf_cfg.fd[i] = prev_cxt->cap_fdr_fd_array[i];
        CMR_LOGD("alloc addr fdr i=%d, addr_y:0x%x, vir_addr_y:0x%x, fd: %d",
                  i, buf_cfg.addr[i].addr_y, buf_cfg.addr_vir[i].addr_y,
                  buf_cfg.fd[i]);
    }
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
    }

    //set used fdr buf cfg to zero
    cmr_bzero(&prev_cxt->cap_used_fdr_buf_cfg, sizeof(struct buffer_cfg));
    return ret;
}

cmr_int cmr_camera_isp_stop_video(cmr_handle preview_handle,
                                  cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    CHECK_HANDLE_VALID(handle);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    /*stop isp*/
    if (PREV_ISP_COWORK == prev_cxt->isp_status) {
        ret = handle->ops.isp_stop_video(handle->oem_handle);
        prev_cxt->isp_status = PREV_ISP_IDLE;
        if (ret) {
            CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
        }
    }
exit:
    return ret;
}

cmr_int cmr_preview_receive_data(cmr_handle preview_handle, cmr_u32 camera_id,
                                 cmr_uint evt, void *data) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct frm_info *frm_data = NULL;
    struct internal_param *inter_param = NULL;

    CMR_LOGV("handle 0x%p camera id %d evt 0x%lx", preview_handle, camera_id,
             evt);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    /*copy the frame info*/

    if (data) {
        frm_data = (struct frm_info *)malloc(sizeof(struct frm_info));
        if (!frm_data) {
            CMR_LOGE("alloc frm mem failed!");
            ret = CMR_CAMERA_NO_MEM;
            goto exit;
        }
        cmr_copy(frm_data, data, sizeof(struct frm_info));
    }

    /*deliver the evt and data via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)evt;
    inter_param->param3 = (void *)frm_data;

    message.msg_type = PREV_EVT_RECEIVE_DATA;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        goto exit;
    }

exit:
    if (ret) {
        if (frm_data) {
            free(frm_data);
            frm_data = NULL;
        }

        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    return ret;
}

cmr_int cmr_preview_update_zoom(cmr_handle preview_handle, cmr_u32 camera_id,
                                struct cmr_zoom_param *param) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!param) {
        CMR_LOGE("zoom param is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)param;

    if (ZOOM_LEVEL == param->mode) {
        CMR_LOGD("update zoom, zoom_level %ld", param->zoom_level);
    } else {
        CMR_LOGD("update zoom, zoom_ratio=%f prev=%f, video=%f, cap=%f",
                 param->zoom_info.zoom_ratio,
                 param->zoom_info.prev_aspect_ratio,
                 param->zoom_info.video_aspect_ratio,
                 param->zoom_info.capture_aspect_ratio);
    }

    message.msg_type = PREV_EVT_UPDATE_ZOOM;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGD("ret %ld", ret);
    return ret;
}

cmr_int cmr_preview_release_frame(cmr_handle preview_handle, cmr_u32 camera_id,
                                  cmr_uint index) {
    UNUSED(preview_handle);
    UNUSED(camera_id);
    UNUSED(index);

    cmr_int ret = CMR_CAMERA_SUCCESS;

    return ret;
}

cmr_int cmr_preview_ctrl_facedetect(cmr_handle preview_handle,
                                    cmr_u32 camera_id, cmr_uint on_off) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)on_off;

    message.msg_type = PREV_EVT_FD_CTRL;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    return ret;
}

cmr_int cmr_preview_facedetect_set_ae_stab(cmr_handle preview_handle,
                                           cmr_u32 camera_id, cmr_u32 *ae_stab) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    if (!ae_stab) {
        CMR_LOGE("ae_stab is NULL");
        return CMR_CAMERA_FAIL;
    }
    for (int i = 0; i < AE_CB_MAX_INDEX; i++) {
        prev_cxt->ae_stab[i] = ae_stab[i];
    }

    return ret;
}

cmr_int cmr_preview_facedetect_set_hist(cmr_handle preview_handle,
                                        cmr_u32 camera_id, const cmr_u32 *data) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    memcpy(&prev_cxt->hist, data, sizeof(prev_cxt->hist));

    return ret;
}

cmr_int cmr_preview_is_support_zsl(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint *is_support) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct sensor_exp_info *sensor_info = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!is_support) {
        CMR_LOGE("invalid param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    sensor_info =
        (struct sensor_exp_info *)malloc(sizeof(struct sensor_exp_info));
    if (!sensor_info) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    if (!handle->ops.get_sensor_info) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret =
        handle->ops.get_sensor_info(handle->oem_handle, camera_id, sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (CAM_IMG_FMT_JPEG == sensor_info->sensor_image_type) {
        *is_support = 0;
    } else {
        *is_support = 1;
    }

exit:
    if (sensor_info) {
        free(sensor_info);
        sensor_info = NULL;
    }
    return ret;
}

cmr_int cmr_preview_get_max_cap_size(cmr_handle preview_handle,
                                     cmr_u32 camera_id, cmr_uint *max_w,
                                     cmr_uint *max_h) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!max_w || !max_h) {
        CMR_LOGE("invalid param, 0x%p, 0x%p", max_w, max_h);
        return CMR_CAMERA_INVALID_PARAM;
    }

    *max_w = handle->prev_cxt[camera_id].max_size.width;
    *max_h = handle->prev_cxt[camera_id].max_size.height;

    CMR_LOGD("max size %ld, %ld", *max_w, *max_h);
    return ret;
}
/**add for 3d capture to reset reprocessing capture size begin*/
cmr_int cmr_preview_set_cap_size(cmr_handle preview_handle,
                                 cmr_u32 is_reprocessing, cmr_u32 camera_id,
                                 cmr_u32 width, cmr_u32 height) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    CMR_LOGD("before set cap_org_size:%d %d",
             handle->prev_cxt[camera_id].cap_org_size.width,
             handle->prev_cxt[camera_id].cap_org_size.height);
    CMR_LOGD("before set actual_pic_size:%d %d",
             handle->prev_cxt[camera_id].actual_pic_size.width,
             handle->prev_cxt[camera_id].actual_pic_size.height);
    CMR_LOGD("before set aligned_pic_size:%d %d",
             handle->prev_cxt[camera_id].aligned_pic_size.width,
             handle->prev_cxt[camera_id].aligned_pic_size.height);
    CMR_LOGD("before set dealign_actual_pic_size:%d %d",
             handle->prev_cxt[camera_id].dealign_actual_pic_size.width,
             handle->prev_cxt[camera_id].dealign_actual_pic_size.height);
    CMR_LOGD("before set picture_size:%d %d",
             handle->prev_cxt[camera_id].prev_param.picture_size.width,
             handle->prev_cxt[camera_id].prev_param.picture_size.height);
    handle->prev_cxt[camera_id].actual_pic_size.width = width;
    handle->prev_cxt[camera_id].actual_pic_size.height = height;
    handle->prev_cxt[camera_id].aligned_pic_size.width = width;
    handle->prev_cxt[camera_id].aligned_pic_size.height = height;
    handle->prev_cxt[camera_id].dealign_actual_pic_size.width = width;
    handle->prev_cxt[camera_id].dealign_actual_pic_size.height = height;
    handle->prev_cxt[camera_id].cap_org_size.width = width;
    handle->prev_cxt[camera_id].cap_org_size.height = height;
    handle->prev_cxt[camera_id].max_size.width = width;
    handle->prev_cxt[camera_id].max_size.height = height;
    handle->prev_cxt[camera_id].prev_param.picture_size.width = width;
    handle->prev_cxt[camera_id].prev_param.picture_size.height = height;
    handle->prev_cxt[camera_id].is_reprocessing = is_reprocessing;

    return 0;
}
/**add for 3d capture to reset reprocessing capture size end*/

cmr_int cmr_preview_get_post_proc_param(cmr_handle preview_handle,
                                        cmr_u32 camera_id, cmr_u32 encode_angle,
                                        struct snp_proc_param *out_param_ptr) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGD("in");

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)((unsigned long)encode_angle);
    inter_param->param3 = (void *)out_param_ptr;

    message.msg_type = PREV_EVT_GET_POST_PROC_PARAM;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
    }

exit:
    CMR_LOGD("out, ret %ld", ret);
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    return ret;
}

cmr_int cmr_preview_before_set_param(cmr_handle preview_handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)mode;

    message.msg_type = PREV_EVT_BEFORE_SET;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_after_set_param(cmr_handle preview_handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)mode;
    inter_param->param3 = (void *)skip_mode;
    inter_param->param4 = (void *)((unsigned long)skip_number);

    message.msg_type = PREV_EVT_AFTER_SET;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_set_preview_buffer(cmr_handle preview_handle,
                                       cmr_u32 camera_id,
                                       cam_buffer_info_t buffer) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("in");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)&buffer;

    message.msg_type = PREV_EVT_SET_PREV_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("out");
    return ret;
}

cmr_int cmr_preview_set_video_buffer(cmr_handle preview_handle,
                                     cmr_u32 camera_id,
                                     cam_buffer_info_t buffer) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("E");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)&buffer;

    message.msg_type = PREV_EVT_SET_VIDEO_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int cmr_preview_set_zsl_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                                   cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                                   cmr_s32 fd) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("E");

    /*deliver the zoom param via internal msg*/
    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)src_phy_addr;
    inter_param->param3 = (void *)src_vir_addr;
    inter_param->param4 = (void *)(unsigned long)fd;

    message.msg_type = PREV_EVT_SET_ZSL_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int cmr_preview_get_hdr_buf(cmr_handle handle, cmr_u32 camera_id,
                                struct frm_info *in, cmr_uint *addr_vir_y) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int i = 0;
    CHECK_HANDLE_VALID(handle);
    struct prev_handle *pre_handle = (struct prev_handle *)handle;
    struct prev_context *prev_cxt = &pre_handle->prev_cxt[camera_id];

    if (!in) {
        CMR_LOGE("input parameters is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    for (i = 0; i < HDR_CAP_NUM; i++) {
        if (in->fd == prev_cxt->cap_hdr_fd_path_array[i])
            break;
    }

    if (i == HDR_CAP_NUM) {
        CMR_LOGE("search hdr buffer failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    *addr_vir_y = prev_cxt->cap_hdr_virt_addr_path_array[i];

    CMR_LOGI("fd:%d", i);

exit:
    return ret;
}

/**************************LOCAL FUNCTION
 * ***************************************************************************/
cmr_int prev_create_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("is_inited %ld", handle->thread_cxt.is_inited);

    if (!handle->thread_cxt.is_inited) {
        pthread_mutex_init(&handle->thread_cxt.prev_mutex, NULL);
        pthread_mutex_init(&handle->thread_cxt.prev_stop_mutex, NULL);
        sem_init(&handle->thread_cxt.prev_sync_sem, 0, 0);
        sem_init(&handle->thread_cxt.prev_recovery_sem, 0, 1);

        ret = cmr_thread_create(&handle->thread_cxt.assist_thread_handle,
                                PREV_MSG_QUEUE_SIZE, prev_assist_thread_proc,
                                (void *)handle, "prev_assist");
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }

        ret = cmr_thread_create(&handle->thread_cxt.thread_handle,
                                PREV_MSG_QUEUE_SIZE, prev_thread_proc,
                                (void *)handle, "prev");
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }

        handle->thread_cxt.is_inited = 1;

        message.msg_type = PREV_EVT_INIT;
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
        if (ret) {
            CMR_LOGE("send msg failed!");
            ret = CMR_CAMERA_FAIL;
            goto end;
        }
    }

end:
    if (ret) {
        sem_destroy(&handle->thread_cxt.prev_sync_sem);
        sem_destroy(&handle->thread_cxt.prev_recovery_sem);
        pthread_mutex_destroy(&handle->thread_cxt.prev_mutex);
        pthread_mutex_destroy(&handle->thread_cxt.prev_stop_mutex);
        handle->thread_cxt.is_inited = 0;
    }

    CMR_LOGI("ret %ld", ret);
    return ret;
}

cmr_int prev_destroy_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("E is_inited %ld", handle->thread_cxt.is_inited);

    if (handle->thread_cxt.is_inited) {
        message.msg_type = PREV_EVT_EXIT;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        ret = cmr_thread_msg_send(handle->thread_cxt.thread_handle, &message);
        if (ret) {
            CMR_LOGE("send msg failed!");
        }

        CMR_LOGV("destory prev thread");
        ret = cmr_thread_destroy(handle->thread_cxt.thread_handle);
        handle->thread_cxt.thread_handle = 0;

        CMR_LOGI("destory prev assist thread");
        ret = cmr_thread_destroy(handle->thread_cxt.assist_thread_handle);
        handle->thread_cxt.assist_thread_handle = 0;

        sem_destroy(&handle->thread_cxt.prev_sync_sem);
        sem_destroy(&handle->thread_cxt.prev_recovery_sem);
        pthread_mutex_destroy(&handle->thread_cxt.prev_mutex);
        pthread_mutex_destroy(&handle->thread_cxt.prev_stop_mutex);
        handle->thread_cxt.is_inited = 0;
    }

    CMR_LOGV("X ret %ld", ret);
    return ret;
}

cmr_int prev_create_cb_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGI("E");

    ret = cmr_thread_create(&handle->thread_cxt.cb_thread_handle,
                            PREV_MSG_QUEUE_SIZE, prev_cb_thread_proc,
                            (void *)handle, "prev_cb");
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto end;
    }

    message.msg_type = PREV_EVT_CB_INIT;
    message.sync_flag = CMR_MSG_SYNC_RECEIVED;
    ret = cmr_thread_msg_send(handle->thread_cxt.cb_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto end;
    }

end:
    CMR_LOGV("ret %ld", ret);
    return ret;
}

cmr_int threednr_sw_prev_callback_process(struct ipm_frame_out *cb_param) {
    struct prev_handle *prev_handle;
    struct prev_threednr_info *threednr_info;
    struct prev_context *prev_cxt;
    int ret;
    int camera_id;
    struct prev_cb_info cb_data_info;
    threednr_info = cb_param->private_data;
    prev_handle = cb_param->caller_handle;
    prev_cxt = &prev_handle->prev_cxt[threednr_info->camera_id];
    camera_id = threednr_info->camera_id;

    prev_pop_video_buffer_sw_3dnr(prev_handle, threednr_info->camera_id,
                                  &threednr_info->data, 0);
    if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
        cmr_uint rot_index = CMR_CAMERA_SUCCESS;
        ret =
            prev_get_src_rot_buffer(prev_cxt, &threednr_info->data, &rot_index);
        CMR_LOGD("rot_index %ld", rot_index);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            return ret;
        }
        ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW, rot_index, 0);
        if (ret) {
            CMR_LOGE("prev_set_rot_buffer_flag failed");
            return ret;
        }
    } else {
        prev_cxt->prev_buf_id =
            ((struct camera_frame_type)(threednr_info->framtype)).buf_id;
    }
    ret = prev_pop_preview_buffer(prev_handle, camera_id, &threednr_info->data,
                                  0);
    if (ret) {
        CMR_LOGE("pop frm 0x%x err",
                 ((struct frm_info)(threednr_info->data)).channel_id);
        return ret;
    }
    /*notify frame via callback*/
    cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &threednr_info->framtype;
    prev_cb_start(prev_handle, &cb_data_info);
    return CMR_CAMERA_SUCCESS;
}


cmr_int prev_destroy_cb_thread(struct prev_handle *handle) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    CMR_LOGV("E");

    message.msg_type = PREV_EVT_CB_EXIT;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    ret = cmr_thread_msg_send(handle->thread_cxt.cb_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
    }

    ret = cmr_thread_destroy(handle->thread_cxt.cb_thread_handle);
    handle->thread_cxt.cb_thread_handle = 0;

    CMR_LOGV("X ret %ld", ret);
    return ret;
}

cmr_int prev_assist_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 msg_type = 0;
    cmr_uint evt = 0;
    cmr_uint src_phy_addr, src_vir_addr, zsl_private;
    cmr_s32 fd;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    struct internal_param *inter_param = NULL;
    struct frm_info *frm_data = NULL;
    struct prev_handle *handle = (struct prev_handle *)p_data;
    cam_buffer_info_t *buffer = NULL;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    msg_type = (cmr_u32)message->msg_type;
    CMR_LOGV("msg_type 0x%x", msg_type);
    switch (msg_type) {
    case PREV_EVT_ASSIST_START:
        handle->frame_active = 1;
        break;

    case PREV_EVT_RECEIVE_DATA:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        evt = (cmr_uint)inter_param->param2;
        frm_data = (struct frm_info *)inter_param->param3;
        if (handle->frame_active == 1) {
            ret = prev_receive_data(handle, camera_id, evt,frm_data);
        }
        if (frm_data) {
            free(frm_data);
            frm_data = NULL;
        }
        break;

    case PREV_EVT_FD_CTRL:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);

        ret = prev_fd_ctrl(handle, camera_id,
                           (cmr_u32)((cmr_uint)inter_param->param2));
        break;

    case PREV_EVT_ASSIST_STOP:
        handle->frame_active = 0;
        break;

    case PREV_EVT_SET_PREV_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        buffer = (cam_buffer_info_t *)inter_param->param2;

        ret = prev_set_preview_buffer(handle, camera_id, buffer);
        break;

    case PREV_EVT_SET_VIDEO_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        buffer = (cam_buffer_info_t *)inter_param->param2;
        ret = prev_set_video_buffer(handle, camera_id, buffer);
        break;

    case PREV_EVT_SET_ZSL_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        src_phy_addr = (cmr_uint)inter_param->param2;
        src_vir_addr = (cmr_uint)inter_param->param3;
        fd = (cmr_s32)(unsigned long)inter_param->param4;

        ret = prev_set_zsl_buffer(handle, camera_id, src_phy_addr, src_vir_addr,
                                  fd);
        break;

    case PREV_EVT_CHANNEL0_QUEUE_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        buffer = (cam_buffer_info_t *)inter_param->param2;
        ret = channel0_queue_buffer(handle, camera_id, buffer);
        break;

    case PREV_EVT_CHANNEL1_QUEUE_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        buffer = (cam_buffer_info_t *)inter_param->param2;
        ret = channel1_queue_buffer(handle, camera_id, buffer);
        break;

    case PREV_EVT_CHANNEL2_QUEUE_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        buffer = (cam_buffer_info_t *)inter_param->param2;
        ret = channel2_queue_buffer(handle, camera_id, buffer);
        break;

    case PREV_EVT_CHANNEL3_QUEUE_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        buffer = (cam_buffer_info_t *)inter_param->param2;
        ret = channel3_queue_buffer(handle, camera_id, buffer);
        break;

    case PREV_EVT_CHANNEL4_QUEUE_BUFFER:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((cmr_uint)inter_param->param1);
        buffer = (cam_buffer_info_t *)inter_param->param2;
        ret = channel4_queue_buffer(handle, camera_id, buffer);
        break;
    case PREV_EVT_3DNR_CALLBACK:
        ret = threednr_sw_prev_callback_process(
            (struct ipm_frame_out *)message->data);
        break;
    default:
        CMR_LOGE("unknown message");
        break;
    }

    return ret;
}

cmr_int prev_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 msg_type = 0;
    cmr_uint evt = 0;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    enum preview_param_mode mode = PARAM_MODE_MAX;
    struct internal_param *inter_param = NULL;
    struct frm_info *frm_data = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    struct prev_handle *handle = (struct prev_handle *)p_data;
    struct prev_cb_info cb_data_info;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    msg_type = (cmr_u32)message->msg_type;
    // CMR_LOGD("msg_type 0x%x", msg_type);

    switch (msg_type) {
    case PREV_EVT_INIT:
        ret = prev_local_init(handle);
        break;

    case PREV_EVT_SET_PARAM:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);

        /*save the preview param first*/
        cmr_copy(&handle->prev_cxt[camera_id].prev_param, inter_param->param2,
                 sizeof(struct preview_param));

        /*handle the param*/
        ret = prev_set_param_internal(
            handle, camera_id, 0,
            (struct preview_out_param *)inter_param->param3);
        break;

    case PREV_EVT_UPDATE_ZOOM:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);
        zoom_param = (struct cmr_zoom_param *)inter_param->param2;
        CMR_LOGV("camera_id %d,  PREV_EVT_UPDATE_ZOOM", camera_id);

        if (ZOOM_LEVEL == zoom_param->mode) {
            CMR_LOGD("update zoom, zoom_level %ld", zoom_param->zoom_level);
        } else {
            CMR_LOGD("update zoom, zoom_ratio=%f, prev=%f, video=%f, cap=%f",
                     zoom_param->zoom_info.zoom_ratio,
                     zoom_param->zoom_info.prev_aspect_ratio,
                     zoom_param->zoom_info.video_aspect_ratio,
                     zoom_param->zoom_info.capture_aspect_ratio);
        }

        /*save zoom param*/
        cmr_copy(&handle->prev_cxt[camera_id].prev_param.zoom_setting,
                 zoom_param, sizeof(struct cmr_zoom_param));
        break;

    case PREV_EVT_BEFORE_SET:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);
        mode = (enum preview_param_mode)inter_param->param2;

        ret = prev_before_set_param(handle, camera_id, mode);
        break;

    case PREV_EVT_AFTER_SET:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);
        mode = (enum preview_param_mode)inter_param->param2;

        ret = prev_after_set_param(
            handle, camera_id, mode, (enum img_skip_mode)inter_param->param3,
            (cmr_u32)((unsigned long)inter_param->param4));
        break;

    case PREV_EVT_START:
        camera_id = (cmr_u32)((unsigned long)message->data);

        prev_recovery_reset(handle, camera_id);
        ret = prev_start(handle, camera_id, 0, 0);
        /*Notify preview started*/
        cb_data_info.cb_type = PREVIEW_EXIT_CB_PREPARE;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
        break;

    case PREV_EVT_STOP:
        camera_id = (cmr_u32)((unsigned long)message->data);
        ret = prev_stop(handle, camera_id, 0);
        break;

    case PREV_EVT_CANCEL_SNP:
        camera_id = (cmr_u32)((unsigned long)message->data);
        ret = prev_cancel_snapshot(handle, camera_id);
        break;

    case PREV_EVT_GET_POST_PROC_PARAM:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);

        ret = prev_update_cap_param(
            handle, camera_id, (cmr_u32)((unsigned long)inter_param->param2),
            (struct snp_proc_param *)inter_param->param3);

        ret = prev_get_cap_post_proc_param(
            handle, camera_id, (cmr_u32)((unsigned long)inter_param->param2),
            (struct snp_proc_param *)inter_param->param3);
        break;

    case PREV_EVT_FD_CTRL:
        inter_param = (struct internal_param *)message->data;
        camera_id = (cmr_u32)((unsigned long)inter_param->param1);

        ret = prev_fd_ctrl(handle, camera_id,
                           (cmr_u32)((unsigned long)inter_param->param2));
        break;

    case PREV_EVT_EXIT:
        ret = prev_local_deinit(handle);
        break;

    default:
        CMR_LOGE("unknown message");
        break;
    }

    return ret;
}

static cmr_int prev_cb_thread_proc(struct cmr_msg *message, void *p_data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 msg_type = 0;
    cmr_uint evt = 0;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    struct prev_cb_info *cb_data_info = NULL;
    struct prev_handle *handle = (struct prev_handle *)p_data;

    if (!message || !p_data) {
        return CMR_CAMERA_INVALID_PARAM;
    }

    msg_type = (cmr_u32)message->msg_type;
    /*CMR_LOGD("msg_type 0x%x", msg_type); */

    switch (msg_type) {
    case PREV_EVT_CB_INIT:
        CMR_LOGD("cb thread inited");
        break;

    case PREV_EVT_CB_START:
        cb_data_info = (struct prev_cb_info *)message->data;

        if (!handle->oem_cb) {
            CMR_LOGE("oem_cb is null");
            break;
        }

        ret = handle->oem_cb(handle->oem_handle, cb_data_info->cb_type,
                             cb_data_info->func_type, cb_data_info->frame_data);

        if (cb_data_info->frame_data) {
            free(cb_data_info->frame_data);
            cb_data_info->frame_data = NULL;
        }
        break;

    case PREV_EVT_CB_EXIT:
        CMR_LOGD("cb thread exit");
        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_cb_start(struct prev_handle *handle,
                      struct prev_cb_info *cb_info) {
    ATRACE_BEGIN(__FUNCTION__);

    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_cb_info *cb_data_info = NULL;
    struct camera_frame_type *frame_type_data = NULL;

    cb_data_info = (struct prev_cb_info *)malloc(sizeof(struct prev_cb_info));
    if (!cb_data_info) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }
    cmr_copy(cb_data_info, cb_info, sizeof(struct prev_cb_info));

    if (cb_info->frame_data) {
        cb_data_info->frame_data = (struct camera_frame_type *)malloc(
            sizeof(struct camera_frame_type));
        if (!cb_data_info->frame_data) {
            CMR_LOGE("No mem!");
            ret = CMR_CAMERA_NO_MEM;
            goto exit;
        }
        cmr_copy(cb_data_info->frame_data, cb_info->frame_data,
                 sizeof(struct camera_frame_type));
    }

    CMR_LOGV("cb_type %d, func_type %d, frame_data 0x%p", cb_data_info->cb_type,
             cb_data_info->func_type, cb_data_info->frame_data);

    /*send to callback thread*/
    message.msg_type = PREV_EVT_CB_START;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)cb_data_info;
    message.alloc_flag = 1;
    ret = cmr_thread_msg_send(handle->thread_cxt.cb_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (cb_data_info) {
            if (cb_data_info->frame_data) {
                free(cb_data_info->frame_data);
                cb_data_info->frame_data = NULL;
            }

            free(cb_data_info);
            cb_data_info = NULL;
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_receive_data(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_uint evt, struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    struct prev_context *prev_cxt = NULL;

    switch (evt) {
    case CMR_GRAB_TX_DONE:
        /*got one frame*/
        ret = prev_frame_handle(handle, camera_id, data);
        break;

    case CMR_GRAB_CANCELED_BUF:
        prev_cxt = &handle->prev_cxt[camera_id];
        preview_enable = prev_cxt->prev_param.preview_eb;
        snapshot_enable = prev_cxt->prev_param.snapshot_eb;
        video_enable = prev_cxt->prev_param.video_eb;
        if (preview_enable && (data->channel_id == prev_cxt->prev_channel_id)) {
            ret = prev_pop_preview_buffer(handle, camera_id, data, 1);
        }

        if (video_enable && (data->channel_id == prev_cxt->video_channel_id)) {
            ret = prev_pop_video_buffer(handle, camera_id, data, 1);
        }

        if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
            ret = prev_pop_zsl_buffer(handle, camera_id, data, 1);
        }

        break;

    case CMR_GRAB_TX_ERROR:
    case CMR_GRAB_TX_NO_MEM:
    case CMR_GRAB_CSI2_ERR:
    case CMR_GRAB_TIME_OUT:
    case CMR_SENSOR_ERROR: {
        char value1[PROPERTY_VALUE_MAX];
        property_get("vendor.cam.recovery.on", value1, "1");
        if (!strcmp(value1, "1")) {
            ret = prev_error_handle(handle, camera_id, evt);
        }
        break;
    }

    case PREVIEW_CHN_PAUSE:
        ret = prev_pause_cap_channel(handle, camera_id, data);
        break;

    case PREVIEW_CHN_RESUME:
        ret = prev_resume_cap_channel(handle, camera_id, data);
        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                          struct frm_info *data) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    cmr_u32 channel4_eb = 0, channel3_eb = 0, channel2_eb = 0, channel1_eb = 0,
            channel0_eb = 0;
    struct prev_context *prev_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;
    channel0_eb = prev_cxt->prev_param.channel0_eb;
    channel1_eb = prev_cxt->prev_param.channel1_eb;
    channel2_eb = prev_cxt->prev_param.channel2_eb;
    channel3_eb = prev_cxt->prev_param.channel3_eb;
    channel4_eb = prev_cxt->prev_param.channel4_eb;

    CMR_LOGV("preview_enable %d, snapshot_enable %d, channel_id %d, "
             "prev_channel_id %ld, cap_channel_id %ld",
             preview_enable, snapshot_enable, data->channel_id,
             prev_cxt->prev_channel_id, prev_cxt->cap_channel_id);

    if (preview_enable && (data->channel_id == prev_cxt->prev_channel_id)) {
        if (prev_cxt->recovery_status != PREV_RECOVERY_IDLE &&
            data->fd != prev_cxt->prev_frm[0].fd) {
            CMR_LOGD("recoverying");
            return ret;
        }
        ret = prev_preview_frame_handle(handle, camera_id, data);
    }

    if (video_enable && (data->channel_id == prev_cxt->video_channel_id)) {
        ret = prev_video_frame_handle(handle, camera_id, data);
    }

    if (channel0_eb && (data->channel_id == prev_cxt->channel0.chn_id)) {
        ret = channel0_dequeue_buffer(handle, camera_id, data);
    }

    if (channel1_eb && (data->channel_id == prev_cxt->channel1.chn_id)) {
        ret = channel1_dequeue_buffer(handle, camera_id, data);
    }

    if (channel2_eb && (data->channel_id == prev_cxt->channel2.chn_id)) {
        ret = channel2_dequeue_buffer(handle, camera_id, data);
    }

    if (channel3_eb && (data->channel_id == prev_cxt->channel3.chn_id)) {
        ret = channel3_dequeue_buffer(handle, camera_id, data);
    }

    if (channel4_eb && (data->channel_id == prev_cxt->channel4.chn_id)) {
        ret = channel4_dequeue_buffer(handle, camera_id, data);
    }

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
        if (prev_cxt->is_zsl_frm) {
            ret = prev_zsl_frame_handle(handle, camera_id, data);
        } else {
            ret = prev_capture_frame_handle(handle, camera_id, data);
        }
    }

    if (prev_cxt->recovery_status != PREV_RECOVERY_IDLE) {
        CMR_LOGD("reset the recover status");
        prev_cxt->recovery_status = PREV_RECOVERY_IDLE;
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_preview_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                  struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    cmr_s64 timestamp = 0;
    struct prev_cb_info cb_data_info;
    cmr_uint rot_index = 0;
    cmr_uint ultra_wide_index = 0;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame");
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGV("frame_id=0x%x, frame_real_id=%d, channel_id=%d fd=0x%x",
             data->frame_id, data->frame_real_id, data->channel_id, data->fd);

    if (0 == prev_cxt->prev_frm_cnt) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    prev_cxt->prev_frm_cnt++;

    if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
        ret = prev_get_src_rot_buffer(prev_cxt, data, &rot_index);
        CMR_LOGD("rot_index %ld", rot_index);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            return ret;
        }
    }
    if (prev_cxt->prev_param.is_ultra_wide) {
        ret = prev_get_src_ultra_wide_buffer(prev_cxt, data, &ultra_wide_index);
        CMR_LOGD("ultra_wide_index %ld", ultra_wide_index);
        if (ret) {
            CMR_LOGE("get src ultra_wide buffer failed");
            return ret;
        }
    }
    /* skip num frames for pre-flash, because the frame is black*/
    if (prev_cxt->prev_preflash_skip_en &&
        IMG_SKIP_SW_KER == prev_cxt->skip_mode) {
        if (prev_cxt->prev_frm_cnt <= prev_cxt->prev_skip_num) {
            CMR_LOGD("ignore this frame, preview cnt %ld, total skip num %ld, "
                     "channed_id %d",
                     prev_cxt->prev_frm_cnt, prev_cxt->prev_skip_num,
                     data->channel_id);

            if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
                ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                               rot_index, 0);
                if (ret) {
                    CMR_LOGE("prev_set_rot_buffer_flag failed");
                    goto exit;
                }
                CMR_LOGD("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                         prev_cxt->prev_rot_frm_is_lock[rot_index]);
                data->fd = prev_cxt->prev_frm[0].fd;
                data->yaddr = prev_cxt->prev_frm[0].addr_phy.addr_y;
                data->uaddr = prev_cxt->prev_frm[0].addr_phy.addr_u;
                data->vaddr = prev_cxt->prev_frm[0].addr_phy.addr_v;
                data->yaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_y;
                data->uaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_u;
                data->vaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_v;
            }
            if (prev_cxt->prev_param.is_ultra_wide) {
                ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                                      ultra_wide_index, 0);
                if (ret) {
                    CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                    goto exit;
                }
                CMR_LOGD(
                    "ultra_wide_index %ld prev_ultra_wide_frm_is_lock %ld",
                    ultra_wide_index,
                    prev_cxt->prev_ultra_wide_frm_is_lock[ultra_wide_index]);
                data->fd = prev_cxt->prev_frm[0].fd;
                data->yaddr = prev_cxt->prev_frm[0].addr_phy.addr_y;
                data->uaddr = prev_cxt->prev_frm[0].addr_phy.addr_u;
                data->vaddr = prev_cxt->prev_frm[0].addr_phy.addr_v;
                data->yaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_y;
                data->uaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_u;
                data->vaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_v;
            }
            ret = prev_pop_preview_buffer(handle, camera_id, data, 1);
            if (ret) {
                CMR_LOGE("pop frm failed");
            }

            return ret;
        }
    }

    if (prev_cxt->prev_param.is_ultra_wide) {
        if (prev_cxt->prev_mem_valid_num > 0) {
            pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
            ret = prev_ultra_wide_send_data(handle, camera_id, data);
            pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

            if (ret) {
                CMR_LOGE("ultra_wide failed, skip this frm");
                ret = CMR_CAMERA_SUCCESS;
                goto exit;
            }
            CMR_LOGV("ultra wide done");

            ret = prev_construct_frame(handle, camera_id, data, &frame_type);
            if (ret) {
                CMR_LOGE("construct frm 0x%x err", data->frame_id);
                goto exit;
            }

            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                                  ultra_wide_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
            /*notify frame*/
            ret = prev_pop_preview_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        } else {
            CMR_LOGW("no available buf, drop! channel_id 0x%x",
                     data->channel_id);
            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                                  ultra_wide_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
        }
    } else if (IMG_ANGLE_0 == prev_cxt->prev_param.prev_rot) {
        ret = prev_construct_frame(handle, camera_id, data, &frame_type);
        if (ret) {
            CMR_LOGE("construct frm err");
            goto exit;
        }

        if (prev_cxt->prev_param.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW &&
            prev_cxt->prev_param.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW)
        {
            prev_cxt->prev_buf_id = frame_type.buf_id;
            ret = prev_pop_preview_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
                }
            /*notify frame via callback*/
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
         }

    } else {
        if (prev_cxt->prev_param.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW &&
            prev_cxt->prev_param.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_CAP_SW)
        {
            /*need rotation*/
            if (prev_cxt->prev_mem_valid_num > 0) {
                ret = prev_start_rotate(handle, camera_id, data);
                if (ret) {
                    CMR_LOGE("rot failed, skip this frm");
                    ret = CMR_CAMERA_SUCCESS;
                    goto exit;
                }
                CMR_LOGD("rot done");

            /*construct frame*/
                ret = prev_construct_frame(handle, camera_id, data, &frame_type);
                if (ret) {
                    CMR_LOGE("construct frm 0x%x err", data->frame_id);
                    goto exit;
                }

                ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW, rot_index,
                                           0);
                if (ret) {
                    CMR_LOGE("prev_set_rot_buffer_flag failed");
                    goto exit;
                }
                /*notify frame*/
                ret = prev_pop_preview_buffer(handle, camera_id, data, 0);
                if (ret) {
                    CMR_LOGE("pop frm 0x%x err", data->channel_id);
                    goto exit;
                }
                cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
                cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
                cb_data_info.frame_data = &frame_type;
                prev_cb_start(handle, &cb_data_info);

                } else {
                CMR_LOGW("no available buf, drop! channel_id 0x%x",
                         data->channel_id);
                ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW, rot_index,
                                           0);
                if (ret) {
                     CMR_LOGE("prev_set_rot_buffer_flag failed");
                    goto exit;
                 }
             }
        }
    }

exit:
    if (ret) {
        cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    return ret;
}

cmr_int prev_video_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    cmr_s64 timestamp = 0;
    struct prev_cb_info cb_data_info;
    cmr_uint rot_index = 0;
    cmr_uint ultra_wide_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame");
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGV("got one frame, frame_id 0x%x, frame_real_id %d, channel_id %d",
             data->frame_id, data->frame_real_id, data->channel_id);

    if (0 == prev_cxt->video_frm_cnt) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->video_frm_cnt++;

    if (IMG_ANGLE_0 != prev_cxt->prev_param.prev_rot) {
        ret = prev_get_src_rot_buffer(prev_cxt, data, &rot_index);
        CMR_LOGD("rot_index %ld", rot_index);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            return ret;
        }
    }

    if (prev_cxt->prev_param.is_ultra_wide) {
        ret = prev_get_src_ultra_wide_buffer(prev_cxt, data, &ultra_wide_index);
        CMR_LOGD("ultra_wide_index %ld", ultra_wide_index);
        if (ret) {
            CMR_LOGE("get src ultra_wide buffer failed");
            return ret;
        }
    }

    if (prev_cxt->prev_param.is_ultra_wide) {
        if (prev_cxt->video_mem_valid_num > 0) {
            pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
            ret = prev_ultra_wide_send_data(handle, camera_id, data);
            pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

            if (ret) {
                CMR_LOGE("ultra_wide failed, skip this frm");
                ret = CMR_CAMERA_SUCCESS;
                goto exit;
            }
            CMR_LOGV("video ultra wide done");

            ret = prev_construct_video_frame(handle, camera_id, data, &frame_type);
            if (ret) {
                CMR_LOGE("construct frm 0x%x err", data->frame_id);
                goto exit;
            }

            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_VIDEO,
                                                  ultra_wide_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
            /*notify frame*/
            ret = prev_pop_video_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        } else {
            CMR_LOGW("no available buf, drop! channel_id 0x%x",
                     data->channel_id);
            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_VIDEO,
                                                  ultra_wide_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
        }
    } else if (IMG_ANGLE_0 == prev_cxt->prev_param.prev_rot) {
        ret = prev_construct_video_frame(handle, camera_id, data, &frame_type);
        if (ret) {
            CMR_LOGE("construct frm err");
            goto exit;
        }
        prev_cxt->video_buf_id = frame_type.buf_id;

        /*notify frame via callback*/
        /*need rotation*/
        /*construct frame*/
        /*notify frame*/

        ret = prev_pop_video_buffer(handle, camera_id, data, 0);
        if (ret) {
            CMR_LOGE("pop frm 0x%x err", data->channel_id);
            goto exit;
        }
        cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);
    } else {
        if (prev_cxt->video_mem_valid_num > 0) {
            ret = prev_start_rotate(handle, camera_id, data);
            if (ret) {
                CMR_LOGE("rot failed, skip this frm");
                ret = CMR_CAMERA_SUCCESS;
                goto exit;
            }
            CMR_LOGD("rot done");
            ret = prev_construct_video_frame(handle, camera_id, data,
                                             &frame_type);
            if (ret) {
                CMR_LOGE("construct frm 0x%x err", data->frame_id);
                goto exit;
            }
            ret =
                prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO, rot_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            ret = prev_pop_video_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        } else {
            CMR_LOGW("no available buf, drop! channel_id 0x%x",
                     data->channel_id);
            ret =
                prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO, rot_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
        }
    }
exit:
    if (ret) {
        cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    return ret;
}
/* Reduce Cyclomatic Complexity for prev_zsl_frame_handle */
static int prev_zsl_frame_handle_wide(struct prev_handle *handle,
                                      cmr_u32 camera_id, struct frm_info *data,
                                      struct prev_cb_info *cb_info_p,
                                      struct camera_frame_type *frame_type) {

    int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 is_fdr = 0;
    cmr_uint ultra_wide_index = 0;

    prev_cxt = &handle->prev_cxt[camera_id];
    ret = prev_get_src_ultra_wide_buffer(prev_cxt, data, &ultra_wide_index);
    if (ret) {
        CMR_LOGE("zsl get src ultra_wide buffer failed");
        return ret;
    }
    if (prev_cxt->cap_zsl_mem_valid_num > 0) {
        ret = prev_ultra_wide_send_data(handle, camera_id, data);
        if (ret) {
            CMR_LOGE("ultra_wide failed, skip this frm");
            ret = CMR_CAMERA_SUCCESS;
            goto exit;
        }
        CMR_LOGV("ultra wide done");

        ret = prev_construct_zsl_frame(handle, camera_id, data, frame_type);
        if (ret) {
            CMR_LOGE("construct frm 0x%x err", data->frame_id);
            goto exit;
        }

        if (handle->ops.get_fdr_enable) {
            CMR_LOGV("get fdr flag handle");
            ret = handle->ops.get_fdr_enable(handle->oem_handle, &is_fdr);
        } else {
            CMR_LOGE("get fdr flag handle failed");
        }
        CMR_LOGD("fdr skip pop zsl buffer, and set ultra wide flag,is_fdr:%d",
                 is_fdr);
        if (!is_fdr) {
            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                                  ultra_wide_index, 0);
            if (ret) {
                CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
            ret = prev_pop_zsl_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
        }
        cb_info_p->cb_type = PREVIEW_EVT_CB_FRAME;
        cb_info_p->func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_info_p->frame_data = frame_type;
        prev_cb_start(handle, cb_info_p);
    } else {
        CMR_LOGW("no available buf, drop! channel_id 0x%x", data->channel_id);
        ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                              ultra_wide_index, 0);
        if (ret) {
            CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
            goto exit;
        }
    }

exit:
    return ret;
}

static int prev_zsl_frame_hangle_rot(struct prev_handle *handle,
                                     cmr_u32 camera_id, struct frm_info *data,
                                     struct prev_cb_info *cb_info_p,
                                     struct camera_frame_type *frame_type) {

    int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_uint rot_index = 0;

    prev_cxt = &handle->prev_cxt[camera_id];
    ret = prev_get_src_rot_buffer(prev_cxt, data, &rot_index);
    if (ret) {
        CMR_LOGE("get src rot buffer failed");
        return ret;
    }
    if (prev_cxt->cap_zsl_mem_valid_num > 0) {
        ret = prev_start_rotate(handle, camera_id, data);
        if (ret) {
            CMR_LOGE("rot failed, skip this frm");
            ret = CMR_CAMERA_SUCCESS;
            goto exit;
        }
        ret = prev_construct_zsl_frame(handle, camera_id, data, frame_type);
        if (ret) {
            CMR_LOGE("construct frm 0x%x err", data->frame_id);
            goto exit;
        }
        ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL, rot_index,
                                       0);
        if (ret) {
            CMR_LOGE("prev_set_rot_buffer_flag failed");
            goto exit;
        }
        ret = prev_pop_zsl_buffer(handle, camera_id, data, 0);
        if (ret) {
            CMR_LOGE("pop frm 0x%x err", data->channel_id);
            goto exit;
        }
        cb_info_p->cb_type = PREVIEW_EVT_CB_FRAME;
        cb_info_p->func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_info_p->frame_data = frame_type;
        prev_cb_start(handle, cb_info_p);
    } else {
        CMR_LOGW("no available buf, drop! channel_id 0x%x", data->channel_id);
        ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL, rot_index,
                                       0);
        if (ret) {
            CMR_LOGE("prev_set_rot_buffer_flag failed");
            goto exit;
        }
    }
exit:
    return ret;
}
cmr_int prev_zsl_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                              struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    cmr_s64 timestamp = 0;
    struct prev_cb_info cb_data_info;
    cmr_uint rot_index = 0;
    cmr_uint ultra_wide_index = 0;
    cmr_u32 is_fdr = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!IS_PREVIEW(handle, camera_id)) {
        CMR_LOGE("preview stopped, skip this frame");
        return ret;
    }

    if (!handle->oem_cb || !handle->ops.channel_free_frame) {
        CMR_LOGE("ops oem_cb or channel_free_frame is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("frame_id=0x%x, frame_real_id=%d, channel_id=%d, fd=0x%x",
             data->frame_id, data->frame_real_id, data->channel_id, data->fd);
    CMR_LOGV("cap_zsl_frm_cnt %ld", prev_cxt->cap_zsl_frm_cnt);
    if (0 == prev_cxt->cap_zsl_frm_cnt) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->cap_zsl_frm_cnt++;

    CMR_LOGV("fdr is_ultra_wide:%d, cap_rot:%d",
             prev_cxt->prev_param.is_ultra_wide, prev_cxt->prev_param.cap_rot);
    if (prev_cxt->prev_param.is_ultra_wide) {
        ret = prev_zsl_frame_handle_wide(handle, camera_id, data, &cb_data_info,
                                         &frame_type);
    } else if (IMG_ANGLE_0 == prev_cxt->prev_param.cap_rot) {

        ret = prev_construct_zsl_frame(handle, camera_id, data, &frame_type);
        if (ret) {
            CMR_LOGE("construct frm err");
            goto exit;
        }

        if (handle->ops.get_fdr_enable) {
            CMR_LOGV("get fdr flag handle");
            ret = handle->ops.get_fdr_enable(handle->oem_handle, &is_fdr);
        } else {
            CMR_LOGE("get fdr flag handle failed");
        }
        CMR_LOGD("fdr skip pop zsl buffer,is_fdr:%d", is_fdr);
        if (!is_fdr) {

            ret = prev_pop_zsl_buffer(handle, camera_id, data, 0);
            if (ret) {
                CMR_LOGE("pop frm 0x%x err", data->channel_id);
                goto exit;
            }
        }
        /*notify frame via callback*/
        cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);
    } else {
        ret = prev_zsl_frame_hangle_rot(handle, camera_id, data, &cb_data_info,
                                        &frame_type);
    }
exit:
    if (ret) {
        cb_data_info.cb_type = PREVIEW_EXIT_CB_FAILED;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

    return ret;
}

cmr_int prev_capture_frame_handle(struct prev_handle *handle, cmr_u32 camera_id,
                                  struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 channel_bit = 0;
    struct buffer_cfg buf_cfg;
    cmr_uint i;
    cmr_uint hdr_num = HDR_CAP_NUM;
    cmr_uint threednr_num = CAP_3DNR_NUM;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    struct timespec ts;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!handle->ops.channel_stop || !handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    channel_bit = 1 << prev_cxt->cap_channel_id;

    CMR_LOGD("frame_id 0x%x", data->frame_id);
    if (!(CMR_CAP0_ID_BASE == (data->frame_id & CMR_CAP0_ID_BASE) ||
          CMR_CAP1_ID_BASE == (data->frame_id & CMR_CAP1_ID_BASE))) {
        CMR_LOGE("0x%x not capture frame, drop it", data->frame_id);
        return ret;
    }

    prev_cxt->cap_frm_cnt++;

    CMR_LOGD("frame_ctrl %d, frame_count %d, cap_frm_cnt %ld, isp_status %ld",
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             prev_cxt->cap_frm_cnt, prev_cxt->isp_status);

    /*capture done, stop isp and channel*/
    CMR_LOGD("cap_frm_cnt %ld, frame_count %d", prev_cxt->cap_frm_cnt,
             prev_cxt->prev_param.frame_count);
    if (prev_cxt->cap_frm_cnt == prev_cxt->prev_param.frame_count ||
        (FRAME_FLASH_MAX == prev_cxt->prev_param.frame_count)) {

        /*post proc*/
        CMR_LOGD("post proc");
        ret = handle->ops.capture_post_proc(handle->oem_handle, camera_id);
        if (ret) {
            CMR_LOGE("post proc failed");
        }

        /*stop channel*/
        ret = handle->ops.channel_stop(handle->oem_handle, channel_bit);
        if (ret) {
            CMR_LOGE("channel_stop failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (CMR_CAP0_ID_BASE == (data->frame_id & CMR_CAP0_ID_BASE)) {
            prev_cxt->cap_channel_status = PREV_CHN_IDLE;
        } else if (CMR_CAP1_ID_BASE == (data->frame_id & CMR_CAP1_ID_BASE)) {
            prev_cxt->zsl_channel_status = PREV_CHN_IDLE;
        }
        /*stop isp*/
        if (PREV_ISP_COWORK == prev_cxt->isp_status) {
            ret = handle->ops.isp_stop_video(handle->oem_handle);
            prev_cxt->isp_status = PREV_ISP_IDLE;
            if (ret) {
                CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
            }
        }
    }

exit:
    return ret;
}

cmr_int prev_error_handle(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_uint evt_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    enum recovery_mode mode = 0;
    struct prev_context *prev_cxt = NULL;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!handle->oem_cb) {
        CMR_LOGE("oem_cb is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&cb_data_info, sizeof(struct prev_cb_info));

    CMR_LOGE("error type 0x%lx, camera_id %d", evt_type, camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    sem_wait(&handle->thread_cxt.prev_recovery_sem);
    if(prev_cxt->recovery_en == 0) {
        CMR_LOGW("exit recovery");
        goto exit;
    }

    CMR_LOGD("prev_status %ld, preview_eb %d, snapshot_eb %d",
             prev_cxt->prev_status, prev_cxt->prev_param.preview_eb,
             prev_cxt->prev_param.snapshot_eb);

    CMR_LOGD("recovery_status %ld, mode %d", prev_cxt->recovery_status, mode);

    if (PREV_RECOVERY_DONE == prev_cxt->recovery_status) {
        CMR_LOGE("recovery failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    switch (evt_type) {
    case CMR_GRAB_TX_ERROR:
    case CMR_GRAB_CSI2_ERR:
        if (PREV_RECOVERING == prev_cxt->recovery_status) {
            prev_cxt->recovery_cnt--;
            CMR_LOGD("recovery_cnt, %ld", prev_cxt->recovery_cnt);
            if (prev_cxt->recovery_cnt) {
                /* try once more */
                mode = RECOVERY_MIDDLE;
            } else {
                /* tried three times, it hasn't recovered yet, restart */
                mode = RECOVERY_HEAVY;
                prev_cxt->recovery_status = PREV_RECOVERY_DONE;
            }
        } else {
            /* now in recovering, start to recover three times */
            mode = RECOVERY_MIDDLE;
            prev_cxt->recovery_status = PREV_RECOVERING;
            prev_cxt->recovery_cnt = PREV_RECOVERY_CNT;
            CMR_LOGD("need recover, recovery_cnt=%ld", prev_cxt->recovery_cnt);
        }
        break;

    case CMR_SENSOR_ERROR:
    case CMR_GRAB_TIME_OUT:
        mode = RECOVERY_HEAVY;
        prev_cxt->recovery_status = PREV_RECOVERY_DONE;
        CMR_LOGD("sensor error, restart preview");
        break;

    default:
        CMR_LOGE("invalid evt_type");
        break;
    }

    ret = prev_recovery_pre_proc(handle, camera_id, mode);
    if (ret) {
        CMR_LOGE("stop failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = prev_recovery_post_proc(handle, camera_id, mode);
    if (ret) {
        CMR_LOGE("start failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    sem_post(&handle->thread_cxt.prev_recovery_sem);
    return 0;
}

cmr_int prev_recovery_pre_proc(struct prev_handle *handle, cmr_u32 camera_id,
                               enum recovery_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    if (!handle->ops.sensor_close) {
        CMR_LOGE("ops is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("mode %d, camera_id %d", mode, camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    prev_clear_preview_buffers(handle, camera_id);

    switch (mode) {
    case RECOVERY_HEAVY:
    case RECOVERY_MIDDLE:
        ret = prev_stop(handle, camera_id, 1);
        if (RECOVERY_HEAVY == mode) {
            handle->ops.sensor_close(handle->oem_handle, camera_id);
        }
        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_recovery_post_proc(struct prev_handle *handle, cmr_u32 camera_id,
                                enum recovery_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (!handle->ops.sensor_open) {
        CMR_LOGE("ops is null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("mode %d, camera_id %d", mode, camera_id);

    switch (mode) {
    case RECOVERY_HEAVY:
    case RECOVERY_MIDDLE:
        if (RECOVERY_HEAVY == mode) {
            handle->ops.sensor_open(handle->oem_handle, camera_id);
        }

        ret = prev_set_param_internal(handle, camera_id, 1, NULL);
        if (ret) {
            CMR_LOGE("prev_set_param_internal failed");
            return ret;
        }

        ret = prev_start(handle, camera_id, 1, 1);
        if (ret) {
            CMR_LOGE("prev_start failed");
            return ret;
        }
        break;

    default:
        break;
    }

    return ret;
}

cmr_int prev_recovery_reset(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    prev_cxt = &handle->prev_cxt[camera_id];

    /*reset recovery status*/
    prev_cxt->recovery_status = PREV_RECOVERY_IDLE;

    return ret;
}

cmr_int prev_local_init(struct prev_handle *handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    CMR_LOGV("X");

    return ret;
}

cmr_int prev_local_deinit(struct prev_handle *handle) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CMR_LOGV("E");

    CHECK_HANDLE_VALID(handle);

    CMR_LOGV("X");

    return ret;
}

cmr_int prev_pre_set(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint sensor_mode = 0;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);

    prev_cxt = &handle->prev_cxt[camera_id];

    sensor_mode = MAX(prev_cxt->prev_mode, prev_cxt->video_mode);
    sensor_mode = MAX(sensor_mode, prev_cxt->cap_mode);
    sensor_mode = MAX(sensor_mode, prev_cxt->channel0_work_mode);
    sensor_mode = MAX(sensor_mode, prev_cxt->channel1_work_mode);
    sensor_mode = MAX(sensor_mode, prev_cxt->channel2_work_mode);
    sensor_mode = MAX(sensor_mode, prev_cxt->channel3_work_mode);
    sensor_mode = MAX(sensor_mode, prev_cxt->channel4_work_mode);

    if (handle->ops.preview_pre_proc == NULL) {
        CMR_LOGE("preview_pre_proc is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("sensor_mode = %ld", sensor_mode);
    ret = handle->ops.preview_pre_proc(handle->oem_handle, camera_id,
                                       (cmr_u32)sensor_mode);
    if (ret) {
        CMR_LOGE("preview_pre_proc failed, ret=%ld", ret);
        goto exit;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int prev_post_set(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    CHECK_HANDLE_VALID(handle);

    if (handle->ops.preview_post_proc) {
        handle->ops.preview_post_proc(handle->oem_handle, camera_id);
    } else {
        CMR_LOGE("post proc is null");
        ret = CMR_CAMERA_FAIL;
    }

    return ret;
}

cmr_int prev_start(struct prev_handle *handle, cmr_u32 camera_id,
                   cmr_u32 is_restart, cmr_u32 is_sn_reopen) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    cmr_u32 tool_eb = 0;
    cmr_u32 channel_bits = 0;
    cmr_uint work_mode = 0;
    cmr_uint skip_num = 0;
    cmr_u8 pdaf_type = 0;
    struct video_start_param isp_param;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct sensor_context *sn_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    sn_cxt = &(cxt->sn_cxt);

    CHECK_HANDLE_VALID(sn_cxt);
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&isp_param, sizeof(struct video_start_param));

    prev_cxt = &handle->prev_cxt[camera_id];
    sem_init(&prev_cxt->ultra_video, 0, 1);
    work_mode = MAX(prev_cxt->prev_mode, prev_cxt->video_mode);
    work_mode = MAX(work_mode, prev_cxt->cap_mode);
    work_mode = MAX(work_mode, prev_cxt->channel0_work_mode);
    work_mode = MAX(work_mode, prev_cxt->channel1_work_mode);
    work_mode = MAX(work_mode, prev_cxt->channel2_work_mode);
    work_mode = MAX(work_mode, prev_cxt->channel3_work_mode);
    work_mode = MAX(work_mode, prev_cxt->channel4_work_mode);

    sensor_mode_info = &prev_cxt->sensor_info.mode_info[work_mode];

    if (!handle->ops.channel_start || !handle->ops.capture_pre_proc) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    pdaf_type = sn_cxt->cur_sns_ex_info.pdaf_supported;
    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;
    tool_eb = prev_cxt->prev_param.tool_eb;
    CMR_LOGD("camera_id %d, prev_status %ld, preview_eb %d, snapshot_eb %d, "
             "need_isp %d,pdaf_type %d",
             camera_id, prev_cxt->prev_status, preview_enable, snapshot_enable,
             prev_cxt->need_isp, pdaf_type);

    if (preview_enable && PREVIEWING == prev_cxt->prev_status) {
        CMR_LOGE("is previewing now, do nothing");
        return ret;
    }

    if(0 == is_restart)
        prev_cxt->recovery_en = 1;

    if (preview_enable)
        channel_bits |= 1 << prev_cxt->prev_channel_id;
    if (video_enable)
        channel_bits |= 1 << prev_cxt->video_channel_id;
    if (snapshot_enable)
        channel_bits |= 1 << prev_cxt->cap_channel_id;
    if (prev_cxt->prev_param.channel0_eb)
        channel_bits |= 1 << prev_cxt->channel0.chn_id;
    if (prev_cxt->prev_param.channel1_eb)
        channel_bits |= 1 << prev_cxt->channel1.chn_id;
    if (prev_cxt->prev_param.channel2_eb)
        channel_bits |= 1 << prev_cxt->channel2.chn_id;
    if (prev_cxt->prev_param.channel3_eb)
        channel_bits |= 1 << prev_cxt->channel3.chn_id;
    if (prev_cxt->prev_param.channel4_eb)
        channel_bits |= 1 << prev_cxt->channel4.chn_id;

    if (snapshot_enable && !preview_enable) {
        handle->ops.capture_pre_proc(handle->oem_handle, camera_id,
                                     prev_cxt->latest_prev_mode, work_mode,
                                     is_restart, is_sn_reopen);
    }

    if (prev_cxt->need_isp) {
        isp_param.size.width = sensor_mode_info->trim_width;
        if (prev_cxt->need_binning) {
            isp_param.size.width = isp_param.size.width >> 1;
        }
        isp_param.size.height = sensor_mode_info->trim_height;
        if (snapshot_enable && !preview_enable) {
            isp_param.is_snapshot = 1;
            isp_param.work_mode = 1;
        } else {
            isp_param.is_snapshot = 0;
            isp_param.work_mode = 0;
        }

// TBD: isp dont need to know zsl or not
#ifdef CONFIG_CAMERA_OFFLINE
#if defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_7)
        if (prev_cxt->prev_param.sprd_zsl_enabled ||
            ((channel_bits & (1 << CHN_3)) && (preview_enable == 1))) {
            isp_param.sprd_zsl_flag = 1;
        }
#else
        if (prev_cxt->prev_param.sprd_zsl_enabled) {
            isp_param.sprd_zsl_flag = 1;
        }
#endif
        else {
            isp_param.sprd_zsl_flag = 0;
        }

        if (snapshot_enable && !preview_enable && !tool_eb) {
            isp_param.dcam_size.width = sensor_mode_info->trim_width;
            isp_param.dcam_size.height = sensor_mode_info->trim_height;
        } else {
            isp_param.dcam_size.width = prev_cxt->dcam_output_size.width;
            isp_param.dcam_size.height = prev_cxt->dcam_output_size.height;
        }
#endif
        prev_cxt->prev_param.remosaic_type = camera_get_remosaic_type(
                &(cxt->sn_cxt.info_4in1),
                sensor_mode_info->trim_width, sensor_mode_info->trim_height);

        isp_param.remosaic_type = prev_cxt->prev_param.remosaic_type;
        ret = handle->ops.isp_start_video(handle->oem_handle, &isp_param);
        if (ret) {
            CMR_LOGE("isp start video failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->isp_status = PREV_ISP_COWORK;
    }

        if (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
            prev_cxt->prev_param.is_auto_3dnr == CAMERA_3DNR_AUTO)
         {
              struct sprd_img_3dnr_param stream_info;
              stream_info.w = prev_cxt->threednr_cap_smallwidth;
              stream_info.h = prev_cxt->threednr_cap_smallheight;
              stream_info.is_3dnr = 1;
              CMR_LOGD("sw_3dnr_info_cfg, %d, %d, %d", stream_info.w,
                   stream_info.h, stream_info.is_3dnr);
              ret =
                   handle->ops.sw_3dnr_info_cfg(handle->oem_handle, &stream_info);
              if (ret) {
                    CMR_LOGE("sw 3dnr info cfg failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                   }
            }

    skip_num = prev_cxt->sensor_info.mipi_cap_skip_num;
    CMR_LOGD("channel_bits %d, skip_num %ld", channel_bits, skip_num);
    ret = handle->ops.channel_start(handle->oem_handle, channel_bits, skip_num);
    if (ret) {
        CMR_LOGE("channel_start failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    prev_cxt->latest_prev_mode = prev_cxt->prev_mode;

    /*update preview status*/
    if (preview_enable) {
        prev_cxt->prev_status = PREVIEWING;
        /*init fd*/
        CMR_LOGD("is_support_fd %lu", prev_cxt->prev_param.is_support_fd);
        if (prev_cxt->prev_param.is_support_fd) {
            prev_fd_open(handle, camera_id);
        }
        /*init ultra_wide*/
        if (prev_cxt->prev_param.is_ultra_wide) {
            prev_ultra_wide_open(handle, camera_id);
        }
        /*init 3dnr*/
       if (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW ||
            prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {
            prev_3dnr_open(handle, camera_id);
        }
        /*init at, dual pd sensor default open 4d auto tracking */
        if (property_get_bool("persist.vendor.cam.auto.tracking.enable", 0) || pdaf_type == DUAL_PD) {
            CMR_LOGD("enable auto tracking");
            /*init auto tracking*/
            if (!prev_cxt->auto_tracking_inited) {
                prev_auto_tracking_open(handle, camera_id);
                prev_cxt->auto_tracking_inited = 1;
            }
        }
    }

    if (video_enable) {
        /*init ultra_wide*/
        if (prev_cxt->prev_param.is_ultra_wide) {
            prev_ultra_wide_open(handle, camera_id);
        }
    }

exit:
    if (ret) {
        if (preview_enable) {
            prev_post_set(handle, camera_id);
            prev_free_prev_buf(handle, camera_id, 0);
        }

        if (video_enable) {
            prev_free_video_buf(handle, camera_id, 0);
        }

        if (prev_cxt->prev_param.channel0_eb) {
            channel0_free_bufs(handle, camera_id, 0);
        }

        if (prev_cxt->prev_param.channel1_eb) {
            channel1_free_bufs(handle, camera_id, 0);
        }

        if (prev_cxt->prev_param.channel2_eb) {
            channel2_free_bufs(handle, camera_id, 0);
        }

        if (prev_cxt->prev_param.channel3_eb) {
            channel3_free_bufs(handle, camera_id, 0);
        }

        if (prev_cxt->prev_param.channel4_eb) {
            channel4_free_bufs(handle, camera_id, 0);
        }

        if (snapshot_enable) {
            prev_free_cap_buf(handle, camera_id, 0);
            prev_free_cap_reserve_buf(handle, camera_id, 0);
            prev_free_zsl_buf(handle, camera_id, 0);
            prev_free_4in1_buf(handle, camera_id, 0);
            prev_free_zsl_raw_buf(handle, camera_id, 0);
            cmr_bzero(&prev_cxt->cap_used_fdr_buf_cfg, sizeof(struct buffer_cfg));
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_stop(struct prev_handle *handle, cmr_u32 camera_id,
                  cmr_u32 is_restart) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 video_enable = 0;
    cmr_u32 pdaf_enable = 0;
    cmr_u32 channel_bits = 0;
    cmr_u32 valid_num;
    struct prev_cb_info cb_data_info;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    sem_destroy(&prev_cxt->ultra_video);
    if (!handle->ops.channel_stop || !handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    video_enable = prev_cxt->prev_param.video_eb;

    CMR_LOGD("E camera_id %d, prev_status %ld, isp_status %ld, preview_eb %d, "
             "snapshot_eb %d",
             camera_id, prev_cxt->prev_status, prev_cxt->isp_status,
             preview_enable, snapshot_enable);

    if (IDLE == prev_cxt->prev_status && preview_enable) {
        CMR_LOGE("is idle now, do nothing");
        return ret;
    }

    if (preview_enable)
        channel_bits |= 1 << prev_cxt->prev_channel_id;
    if (video_enable)
        channel_bits |= 1 << prev_cxt->video_channel_id;
    if (snapshot_enable)
        channel_bits |= 1 << prev_cxt->cap_channel_id;
    if (prev_cxt->prev_param.channel1_eb)
        channel_bits |= 1 << prev_cxt->channel1.chn_id;
    if (prev_cxt->prev_param.channel2_eb)
        channel_bits |= 1 << prev_cxt->channel2.chn_id;
    if (prev_cxt->prev_param.channel3_eb)
        channel_bits |= 1 << prev_cxt->channel3.chn_id;
    if (prev_cxt->prev_param.channel4_eb)
        channel_bits |= 1 << prev_cxt->channel4.chn_id;

    prev_cxt->prev_channel_status = PREV_CHN_IDLE;
    prev_cxt->cap_channel_status = PREV_CHN_IDLE;

#if defined(CONFIG_ISP_2_3)  //just for sharkle
    pthread_mutex_lock(&handle->thread_cxt.prev_stop_mutex);
    /*stop channel*/
    CMR_LOGD("channel_bits %d", channel_bits);
    ret = handle->ops.channel_stop(handle->oem_handle, channel_bits);
    if (ret) {
        CMR_LOGE("channel_stop failed");
        ret = CMR_CAMERA_FAIL;
        pthread_mutex_unlock(&handle->thread_cxt.prev_stop_mutex);
        goto exit;
    }
    if (preview_enable) {
        if (is_restart && PREV_RECOVERY_IDLE != prev_cxt->recovery_status) {
            prev_cxt->prev_status = RECOVERING_IDLE;
        } else {
            prev_cxt->prev_status = IDLE;
        }
    }
    pthread_mutex_unlock(&handle->thread_cxt.prev_stop_mutex);
#else
  /*stop channel*/
    CMR_LOGD("channel_bits %d", channel_bits);
    ret = handle->ops.channel_stop(handle->oem_handle, channel_bits);
    if (ret) {
        CMR_LOGE("channel_stop failed");
        ret = CMR_CAMERA_FAIL;
        pthread_mutex_unlock(&handle->thread_cxt.prev_stop_mutex);
        goto exit;
    }
#endif

    if (preview_enable) {
        if (is_restart && PREV_RECOVERY_IDLE != prev_cxt->recovery_status) {
            prev_cxt->prev_status = RECOVERING_IDLE;
        } else {
            prev_cxt->prev_status = IDLE;
        }

        /*deinit fd*/
        if (prev_cxt->prev_param.is_support_fd) {
            prev_fd_close(handle, camera_id);
        }
        /*deinit ultra wide*/
        if (prev_cxt->prev_param.is_ultra_wide) {
            prev_ultra_wide_close(handle, camera_id);
        }

        /*deinit 3dnr_preview*/
        if (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
                  prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
                  prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW) {
            prev_3dnr_close(handle, camera_id);
        }
        /*stop auto tracking*/
        if (prev_cxt->auto_tracking_inited) {
            prev_auto_tracking_close(handle, camera_id);
        }
    }

    if (video_enable) {
        /*deinit ultra_wide*/
        if (prev_cxt->prev_param.is_ultra_wide) {
            prev_ultra_wide_close(handle, camera_id);
        }
    }

    /*stop isp*/
    if (PREV_ISP_COWORK == prev_cxt->isp_status) {
        ret = handle->ops.isp_stop_video(handle->oem_handle);
        prev_cxt->isp_status = PREV_ISP_IDLE;
        if (ret) {
            CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
        }
    }

    if (preview_enable) {
        prev_post_set(handle, camera_id);
        prev_free_prev_buf(handle, camera_id, is_restart);

        CMR_LOGD("is_restart %d, recovery_status %ld", is_restart,
                 prev_cxt->recovery_status);
        if (!is_restart) {
            /*stop response*/
            cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
            cb_data_info.func_type = PREVIEW_FUNC_STOP_PREVIEW;
            cb_data_info.frame_data = NULL;
            prev_cb_start(handle, &cb_data_info);
        }
    }

    if (video_enable) {
        prev_free_video_buf(handle, camera_id, is_restart);
        valid_num = prev_cxt->video_mem_valid_num;

#if defined(CONFIG_ISP_2_3)
// just for sharkle in 3dnr video mode to clear video buffer
        while(valid_num > 0){
            cmr_bzero(&prev_cxt->video_frm[valid_num-1], sizeof(struct img_frm));
            valid_num --;
        }
        prev_cxt->video_mem_valid_num = 0;
        CMR_LOGI("clear video buffer");
#endif
    }

    if (prev_cxt->prev_param.channel1_eb) {
        channel1_free_bufs(handle, camera_id, 0);
    }

    if (prev_cxt->prev_param.channel2_eb) {
        channel2_free_bufs(handle, camera_id, 0);
    }

    if (prev_cxt->prev_param.channel3_eb) {
        channel3_free_bufs(handle, camera_id, 0);
    }

    if (prev_cxt->prev_param.channel4_eb) {
        channel4_free_bufs(handle, camera_id, 0);
    }

    if (snapshot_enable) {
        /*capture post proc*/
        ret = handle->ops.capture_post_proc(handle->oem_handle, camera_id);
        if (ret) {
            CMR_LOGE("post proc failed");
        }
        /* if encoding, need wait encode end, or memory be freed when using */
        cmr_snpshot_encode_semaphore(cxt->snp_cxt.snapshot_handle, 1);
        cmr_snpshot_encode_semaphore(cxt->snp_cxt.snapshot_handle, 0);
        prev_free_cap_buf(handle, camera_id, is_restart);
    }

    prev_free_cap_reserve_buf(handle, camera_id, is_restart);
    prev_free_zsl_buf(handle, camera_id, is_restart);
    CMR_LOGD("fdr free zsl raw buf");
    prev_free_zsl_raw_buf(handle, camera_id, is_restart);
    cmr_bzero(&prev_cxt->cap_used_fdr_buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt->prev_frm_cnt = 0;
    prev_cxt->video_frm_cnt = 0;
    prev_cxt->cap_zsl_frm_cnt = 0;
    prev_cxt->capture_scene_mode = 0;
    if (!is_restart) {
        prev_cxt->cap_frm_cnt = 0;
    }

    prev_cxt->need_isp = 0;
    prev_cxt->need_binning = 0;

    prev_cxt->prev_mem_valid_num = 0;
    prev_cxt->video_mem_valid_num = 0;
    prev_cxt->cap_zsl_mem_valid_num = 0;

    pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
    prev_cxt->prev_preflash_skip_en = 0;
    prev_cxt->restart_skip_cnt = 0;
    prev_cxt->restart_skip_en = 0;
    prev_cxt->video_restart_skip_cnt = 0;
    prev_cxt->video_restart_skip_en = 0;
    prev_cxt->cap_zsl_restart_skip_cnt = 0;
    prev_cxt->cap_zsl_restart_skip_en = 0;
    pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);

exit:
    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_cancel_snapshot(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 channel_bit = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!handle->ops.channel_stop || !handle->ops.isp_stop_video) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    channel_bit = 1 << prev_cxt->cap_channel_id;

    CMR_LOGD("channel_bit %d, channel_status %ld", channel_bit,
             prev_cxt->cap_channel_status);

    /*capture done, stop isp and channel*/
    if (PREV_CHN_BUSY == prev_cxt->cap_channel_status) {

        /*stop channel*/
        ret = handle->ops.channel_stop(handle->oem_handle, channel_bit);
        if (ret) {
            CMR_LOGE("channel_stop failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->cap_channel_status = PREV_CHN_IDLE;

        /*stop isp*/
        if (PREV_ISP_COWORK == prev_cxt->isp_status) {
            ret = handle->ops.isp_stop_video(handle->oem_handle);
            prev_cxt->isp_status = PREV_ISP_IDLE;
            if (ret) {
                CMR_LOGE("Failed to stop ISP video mode, %ld", ret);
            }
        }
    }

    /*post proc*/
    ret = handle->ops.capture_post_proc(handle->oem_handle, camera_id);
    if (ret) {
        CMR_LOGE("post proc failed");
    }

exit:
    return ret;
}

cmr_int prev_alloc_prev_buf(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_uint reserved_count = 1;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->actual_prev_size.width;
    height = prev_cxt->actual_prev_size.height;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init preview memory info*/
    buffer_size = width * height;
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->prev_param.preview_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->prev_param.preview_fmt ||
        CAM_IMG_FMT_BAYER_MIPI_RAW == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->prev_mem_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->prev_mem_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->prev_param.preview_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot) {
            prev_cxt->prev_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->prev_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->prev_param.preview_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->prev_param.preview_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->prev_mem_num = PREV_FRM_ALLOC_CNT;
    if (prev_cxt->prev_param.prev_rot) {
        CMR_LOGD("need increase buf for rotation");
        prev_cxt->prev_mem_num += PREV_ROT_FRM_ALLOC_CNT;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        cmr_uint prev_mem_num = prev_cxt->prev_mem_num;
        cmr_u32 ultra_wide_mem_num = 0;
        cmr_uint real_width = width;
        cmr_uint real_height = height;
        prev_cxt->prev_mem_valid_num = 0;
        cmr_bzero(prev_cxt->prev_phys_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_virt_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_fd_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_s32));
        mem_ops->alloc_mem(
            CAMERA_PREVIEW, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->prev_mem_size,
            (cmr_u32 *)&prev_cxt->prev_mem_num, prev_cxt->prev_phys_addr_array,
            prev_cxt->prev_virt_addr_array, prev_cxt->prev_fd_array);
        if (prev_cxt->prev_param.is_ultra_wide) {
            cmr_u32 prev_mem_size = prev_cxt->prev_mem_size;
            ultra_wide_mem_num = PREV_ULTRA_WIDE_ALLOC_CNT;
            CMR_LOGD("ultra wide gpu alloc 0x%lx, mem_num %ld",
                     prev_cxt->prev_mem_size, ultra_wide_mem_num);
            mem_ops->gpu_alloc_mem(
                CAMERA_PREVIEW_ULTRA_WIDE, handle->oem_handle,
                (cmr_u32 *)&prev_mem_size, &ultra_wide_mem_num,
                prev_cxt->prev_phys_addr_array + prev_mem_num,
                prev_cxt->prev_virt_addr_array + prev_mem_num,
                prev_cxt->prev_fd_array + prev_mem_num,
                prev_cxt->prev_ultra_wide_handle_array, &real_width,
                &real_height);
        }
        /*check memory valid*/
        CMR_LOGD("prev_mem_size 0x%lx, mem_num %ld", prev_cxt->prev_mem_size,
                 prev_cxt->prev_mem_num);
#ifdef CONFIG_Y_IMG_TO_ISP
        prev_cxt->prev_mem_y_size = prev_cxt->prev_mem_size * 2 / 3;
        prev_cxt->prev_mem_y_num = 2;
        mem_ops->alloc_mem(CAMERA_ISP_PREVIEW_Y, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->prev_mem_y_size,
                           (cmr_u32 *)&prev_cxt->prev_mem_y_num,
                           prev_cxt->prev_phys_y_addr_array,
                           prev_cxt->prev_virt_y_addr_array,
                           prev_cxt->prev_mfd_y_array);
        CMR_LOGD("phys 0x%lx, virt %lx", prev_cxt->prev_phys_y_addr_array[0],
                 prev_cxt->prev_virt_y_addr_array[0]);
        CMR_LOGD("phys 0x%lx, virt %lx", prev_cxt->prev_phys_y_addr_array[1],
                 prev_cxt->prev_virt_y_addr_array[1]);
#endif
#ifdef YUV_TO_ISP
        prev_cxt->prev_mem_yuv_size = prev_cxt->prev_mem_size;
        prev_cxt->prev_mem_yuv_num = 1;
        mem_ops->alloc_mem(CAMERA_ISP_PREVIEW_YUV, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->prev_mem_yuv_size,
                           (cmr_u32 *)&prev_cxt->prev_mem_yuv_num,
                           &prev_cxt->prev_phys_yuv_addr,
                           &prev_cxt->prev_virt_yuv_addr,
                           &prev_cxt->prev_mfd_yuv);
#endif

        for (i = 0; i < (prev_mem_num + ultra_wide_mem_num); i++) {
            CMR_LOGD("%ld, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->prev_virt_addr_array[i],
                     prev_cxt->prev_fd_array[i]);

            if ((0 == prev_cxt->prev_virt_addr_array[i]) ||
                (0 == prev_cxt->prev_fd_array[i])) {
                if (i >= PREV_FRM_ALLOC_CNT) {
                    CMR_LOGE("memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            } else {
                if (i < PREV_FRM_ALLOC_CNT) {
                    prev_cxt->prev_mem_valid_num++;
                }
            }
        }

        mem_ops->alloc_mem(
            CAMERA_PREVIEW_RESERVED, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->prev_mem_size, (cmr_u32 *)&reserved_count,
            &prev_cxt->prev_reserved_phys_addr,
            &prev_cxt->prev_reserved_virt_addr, &prev_cxt->prev_reserved_fd);
        CMR_LOGD("reserved, virt_addr 0x%lx, fd 0x%x",
                 prev_cxt->prev_reserved_virt_addr, prev_cxt->prev_reserved_fd);
    }

    frame_size = prev_cxt->prev_mem_size;
    prev_num = prev_cxt->prev_mem_num;
    if (prev_cxt->prev_param.prev_rot) {
        prev_num = prev_cxt->prev_mem_num - PREV_ROT_FRM_ALLOC_CNT;
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->prev_mem_valid_num;
    buffer->length = frame_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->prev_mem_valid_num; i++) {
        prev_cxt->prev_frm[i].buf_size = frame_size;
        prev_cxt->prev_frm[i].addr_vir.addr_y =
            prev_cxt->prev_virt_addr_array[i];
        prev_cxt->prev_frm[i].addr_vir.addr_u =
            prev_cxt->prev_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->prev_frm[i].addr_phy.addr_y =
            prev_cxt->prev_phys_addr_array[i];
        prev_cxt->prev_frm[i].addr_phy.addr_u =
            prev_cxt->prev_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->prev_frm[i].fd = prev_cxt->prev_fd_array[i];
        prev_cxt->prev_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->prev_frm[i].size.width = prev_cxt->actual_prev_size.width;
        prev_cxt->prev_frm[i].size.height = prev_cxt->actual_prev_size.height;

        buffer->addr[i].addr_y = prev_cxt->prev_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->prev_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->prev_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->prev_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->prev_frm[i].fd;
    }
    prev_cxt->prev_reserved_frm.buf_size = frame_size;
    prev_cxt->prev_reserved_frm.addr_vir.addr_y =
        prev_cxt->prev_reserved_virt_addr;
    prev_cxt->prev_reserved_frm.addr_vir.addr_u =
        prev_cxt->prev_reserved_frm.addr_vir.addr_y + buffer_size;
    prev_cxt->prev_reserved_frm.addr_phy.addr_y =
        prev_cxt->prev_reserved_phys_addr;
    prev_cxt->prev_reserved_frm.addr_phy.addr_u =
        prev_cxt->prev_reserved_frm.addr_phy.addr_y + buffer_size;
    prev_cxt->prev_reserved_frm.fd = prev_cxt->prev_reserved_fd;

    prev_cxt->prev_reserved_frm.fmt = prev_cxt->prev_param.preview_fmt;
    prev_cxt->prev_reserved_frm.size.width = prev_cxt->actual_prev_size.width;
    prev_cxt->prev_reserved_frm.size.height = prev_cxt->actual_prev_size.height;

    prev_cxt->prev_frm[i].addr_phy.addr_v = 0;
    prev_cxt->prev_reserved_frm.addr_phy.addr_v = 0;

    if (prev_cxt->prev_param.prev_rot) {
        for (i = 0; i < PREV_ROT_FRM_ALLOC_CNT; i++) {
            prev_cxt->prev_rot_frm[i].buf_size = frame_size;
            prev_cxt->prev_rot_frm[i].addr_vir.addr_y =
                prev_cxt->prev_virt_addr_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].addr_vir.addr_u =
                prev_cxt->prev_rot_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->prev_rot_frm[i].addr_phy.addr_y =
                prev_cxt->prev_phys_addr_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].addr_phy.addr_u =
                prev_cxt->prev_rot_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->prev_rot_frm[i].addr_phy.addr_v = 0;
            prev_cxt->prev_rot_frm[i].fd =
                prev_cxt->prev_fd_array[prev_num + i];
            prev_cxt->prev_rot_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
            prev_cxt->prev_rot_frm[i].size.width =
                prev_cxt->actual_prev_size.width;
            prev_cxt->prev_rot_frm[i].size.height =
                prev_cxt->actual_prev_size.height;
        }
    }
    if (prev_cxt->prev_param.is_ultra_wide) {
        for (i = 0; i < PREV_ULTRA_WIDE_ALLOC_CNT; i++) {
            prev_cxt->prev_ultra_wide_frm[i].buf_size = frame_size;
            prev_cxt->prev_ultra_wide_frm[i].addr_vir.addr_y =
                prev_cxt->prev_virt_addr_array[prev_num + i];
            prev_cxt->prev_ultra_wide_frm[i].addr_vir.addr_u =
                prev_cxt->prev_ultra_wide_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->prev_ultra_wide_frm[i].addr_phy.addr_y =
                prev_cxt->prev_phys_addr_array[prev_num + i];
            prev_cxt->prev_ultra_wide_frm[i].addr_phy.addr_u =
                prev_cxt->prev_ultra_wide_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->prev_ultra_wide_frm[i].addr_phy.addr_v = 0;
            prev_cxt->prev_ultra_wide_frm[i].fd =
                prev_cxt->prev_fd_array[prev_num + i];
            prev_cxt->prev_ultra_wide_frm[i].fmt =
                prev_cxt->prev_param.preview_fmt;
            prev_cxt->prev_ultra_wide_frm[i].size.width =
                prev_cxt->actual_prev_size.width;
            prev_cxt->prev_ultra_wide_frm[i].size.height =
                prev_cxt->actual_prev_size.height;
        }

        if (is_restart) {
            buffer->count = 0;
            for (i = 0; i < PREV_ULTRA_WIDE_ALLOC_CNT; i++) {
                if (1 == *(&prev_cxt->prev_ultra_wide_frm_is_lock[0] + i)) {
                    buffer->addr[buffer->count].addr_y =
                        prev_cxt->prev_ultra_wide_frm[i].addr_phy.addr_y;
                    buffer->addr[buffer->count].addr_u =
                        prev_cxt->prev_ultra_wide_frm[i].addr_phy.addr_u;
                    buffer->addr_vir[buffer->count].addr_y =
                        prev_cxt->prev_ultra_wide_frm[i].addr_vir.addr_y;
                    buffer->addr_vir[buffer->count].addr_u =
                        prev_cxt->prev_ultra_wide_frm[i].addr_vir.addr_u;
                    buffer->fd[buffer->count] =
                        prev_cxt->prev_ultra_wide_frm[i].fd;
                    buffer->count++;
                }
            }
        }
    }
    CMR_LOGV("X %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_free_prev_buf(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        cmr_uint prev_mem_num = prev_cxt->prev_mem_num;
        cmr_uint ultra_wide_mem_num = PREV_ULTRA_WIDE_ALLOC_CNT;

        mem_ops->free_mem(CAMERA_PREVIEW, handle->oem_handle,
                          prev_cxt->prev_phys_addr_array,
                          prev_cxt->prev_virt_addr_array,
                          prev_cxt->prev_fd_array, prev_cxt->prev_mem_num);
        if (prev_cxt->prev_param.is_ultra_wide) {
            mem_ops->free_mem(CAMERA_PREVIEW_ULTRA_WIDE, handle->oem_handle,
                              prev_cxt->prev_phys_addr_array + prev_mem_num,
                              prev_cxt->prev_virt_addr_array + prev_mem_num,
                              prev_cxt->prev_fd_array + prev_mem_num,
                              ultra_wide_mem_num);
            cmr_bzero(prev_cxt->prev_ultra_wide_handle_array,
                      (ultra_wide_mem_num) * sizeof(void *));
        }
        cmr_bzero(prev_cxt->prev_phys_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_virt_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_fd_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_s32));

#ifdef CONFIG_Y_IMG_TO_ISP
        mem_ops->free_mem(CAMERA_ISP_PREVIEW_Y, handle->oem_handle,
                          prev_cxt->prev_phys_y_addr_array,
                          prev_cxt->prev_virt_y_addr_array,
                          prev_cxt->prev_mfd_y_array, prev_cxt->prev_mem_y_num);
        cmr_bzero(prev_cxt->prev_phys_y_addr_array, 2 * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_virt_y_addr_array, 2 * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->prev_mfd_y_array, 2 * sizeof(cmr_s32));
#endif

#ifdef YUV_TO_ISP
        mem_ops->free_mem(CAMERA_ISP_PREVIEW_YUV, handle->oem_handle,
                          (cmr_uint *)prev_cxt->prev_phys_yuv_addr,
                          (cmr_uint *)prev_cxt->prev_virt_yuv_addr,
                          &prev_cxt->prev_mfd_yuv, prev_cxt->prev_mem_yuv_num);
        prev_cxt->prev_phys_yuv_addr = 0;
        prev_cxt->prev_virt_yuv_addr = 0;
        prev_cxt->prev_mfd_yuv = 0;
#endif

        mem_ops->free_mem(CAMERA_PREVIEW_RESERVED, handle->oem_handle,
                          (cmr_uint *)prev_cxt->prev_reserved_phys_addr,
                          (cmr_uint *)prev_cxt->prev_reserved_virt_addr,
                          &prev_cxt->prev_reserved_fd, (cmr_u32)1);

        prev_cxt->prev_reserved_phys_addr = 0;
        prev_cxt->prev_reserved_virt_addr = 0;
        prev_cxt->prev_reserved_fd = 0;
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int prev_alloc_video_buf(struct prev_handle *handle, cmr_u32 camera_id,
                             cmr_u32 is_restart, struct buffer_cfg *buffer) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_uint reserved_count = 1;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->actual_video_size.width;
    height = prev_cxt->actual_video_size.height;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init video memory info*/
    buffer_size = width * height;
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->prev_param.preview_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->video_mem_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->prev_param.preview_fmt) {
        prev_cxt->video_mem_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->prev_param.preview_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot) {
            prev_cxt->video_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->video_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->prev_param.preview_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->prev_param.preview_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->video_mem_num = PREV_FRM_ALLOC_CNT;
    if (prev_cxt->prev_param.prev_rot) {
        CMR_LOGD("need increase buf for rotation");
        prev_cxt->video_mem_num += PREV_ROT_FRM_ALLOC_CNT;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!is_restart) {
        prev_cxt->video_mem_valid_num = 0;
        prev_cxt->cache_buffer_cont = 0;

        cmr_uint video_mem_num = prev_cxt->video_mem_num;
        cmr_u32 ultra_wide_mem_num = 0;
        cmr_uint real_width = width;
        cmr_uint real_height = height;

        for (i = 0; i < prev_cxt->video_mem_num; i++) {
            prev_cxt->video_phys_addr_array[i] = 0;
            prev_cxt->video_virt_addr_array[i] = 0;
            prev_cxt->video_fd_array[i] = 0;
        }
        mem_ops->alloc_mem(CAMERA_VIDEO, handle->oem_handle,
                           (cmr_u32 *)&prev_cxt->video_mem_size,
                           (cmr_u32 *)&prev_cxt->video_mem_num,
                           prev_cxt->video_phys_addr_array,
                           prev_cxt->video_virt_addr_array,
                           prev_cxt->video_fd_array);

        if (prev_cxt->prev_param.is_ultra_wide) {
            cmr_u32 video_mem_size = prev_cxt->video_mem_size;
            ultra_wide_mem_num = VIDEO_ULTRA_WIDE_ALLOC_CNT;
            CMR_LOGD("video ultra wide gpu alloc 0x%lx, mem_num %ld",
                     prev_cxt->video_mem_size, ultra_wide_mem_num);
            mem_ops->gpu_alloc_mem(
                CAMERA_VIDEO_ULTRA_WIDE, handle->oem_handle,
                (cmr_u32 *)&video_mem_size, &ultra_wide_mem_num,
                prev_cxt->video_phys_addr_array + video_mem_num,
                prev_cxt->video_virt_addr_array + video_mem_num,
                prev_cxt->video_fd_array + video_mem_num,
                prev_cxt->video_ultra_wide_handle_array, &real_width,
                &real_height);
            if (prev_cxt->prev_param.sprd_eis_enabled == 1) {
                prev_cxt->eis_video_ultra_wide_mem_num = 0;
                prev_cxt->eis_video_phys_addr = 0;
                prev_cxt->eis_video_virt_addr = 0;
                prev_cxt->eis_video_fd = 0;
                prev_cxt->dst_eis_video_buffer_handle = NULL;

                prev_cxt->eis_video_mem_size = prev_cxt->video_mem_size;
                prev_cxt->eis_video_ultra_wide_mem_num = 1;
                mem_ops->gpu_alloc_mem(
                    CAMERA_VIDEO_EIS_ULTRA_WIDE, handle->oem_handle,
                    (cmr_u32 *)&prev_cxt->eis_video_mem_size,
                    &prev_cxt->eis_video_ultra_wide_mem_num,
                    &prev_cxt->eis_video_phys_addr,
                    &prev_cxt->eis_video_virt_addr,
                    &prev_cxt->eis_video_fd,
                    &prev_cxt->dst_eis_video_buffer_handle,
                    &real_width, &real_height);
                CMR_LOGD("eis vir addr=0x%x, fd=0x%x,size=0x%x",
                    prev_cxt->eis_video_virt_addr, prev_cxt->eis_video_fd,
                    prev_cxt->eis_video_mem_size);
            }
        }

        /*check memory valid*/
        CMR_LOGD("video_mem_size 0x%lx, mem_num %ld", prev_cxt->video_mem_size,
                 prev_cxt->video_mem_num);
        for (i = 0; i < (video_mem_num + ultra_wide_mem_num); i++) {
            CMR_LOGV("%ld, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->video_virt_addr_array[i],
                     prev_cxt->video_fd_array[i]);

            if ((0 == prev_cxt->video_virt_addr_array[i]) ||
                (0 == prev_cxt->video_fd_array[i])) {
                if (i >= PREV_FRM_ALLOC_CNT) {
                    CMR_LOGE("memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            } else {
                if (i < PREV_FRM_ALLOC_CNT) {
                    prev_cxt->video_mem_valid_num++;
                }
            }
        }

        mem_ops->alloc_mem(
            CAMERA_VIDEO_RESERVED, handle->oem_handle,
            (cmr_u32 *)&prev_cxt->video_mem_size, (cmr_u32 *)&reserved_count,
            &prev_cxt->video_reserved_phys_addr,
            &prev_cxt->video_reserved_virt_addr, &prev_cxt->video_reserved_fd);
    }

    frame_size = prev_cxt->video_mem_size;
    prev_num = prev_cxt->video_mem_num;
    if (prev_cxt->prev_param.prev_rot) {
        prev_num = prev_cxt->video_mem_num - PREV_ROT_FRM_ALLOC_CNT;
    }

    /*arrange the buffer*/

    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_VIDEO_ID_BASE;
    buffer->count = prev_cxt->video_mem_valid_num;
    buffer->length = frame_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; (cmr_int)i < prev_cxt->video_mem_valid_num; i++) {
        prev_cxt->video_frm[i].buf_size = frame_size;
        prev_cxt->video_frm[i].addr_vir.addr_y =
            prev_cxt->video_virt_addr_array[i];
        prev_cxt->video_frm[i].addr_vir.addr_u =
            prev_cxt->video_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->video_frm[i].addr_phy.addr_y =
            prev_cxt->video_phys_addr_array[i];
        prev_cxt->video_frm[i].addr_phy.addr_u =
            prev_cxt->video_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->video_frm[i].fd = prev_cxt->video_fd_array[i];
        prev_cxt->video_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
        prev_cxt->video_frm[i].size.width = prev_cxt->actual_video_size.width;
        prev_cxt->video_frm[i].size.height = prev_cxt->actual_video_size.height;

        buffer->addr[i].addr_y = prev_cxt->video_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->video_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->video_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->video_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->video_frm[i].fd;
    }

    prev_cxt->video_reserved_frm.buf_size = frame_size;
    prev_cxt->video_reserved_frm.addr_vir.addr_y =
        prev_cxt->video_reserved_virt_addr;
    prev_cxt->video_reserved_frm.addr_vir.addr_u =
        prev_cxt->video_reserved_frm.addr_vir.addr_y + buffer_size;
    prev_cxt->video_reserved_frm.addr_phy.addr_y =
        prev_cxt->video_reserved_phys_addr;
    prev_cxt->video_reserved_frm.addr_phy.addr_u =
        prev_cxt->video_reserved_frm.addr_phy.addr_y + buffer_size;
    prev_cxt->video_reserved_frm.fd = prev_cxt->video_reserved_fd;
    prev_cxt->video_reserved_frm.fmt = prev_cxt->prev_param.preview_fmt;
    prev_cxt->video_reserved_frm.size.width = prev_cxt->actual_video_size.width;
    prev_cxt->video_reserved_frm.size.height =
        prev_cxt->actual_video_size.height;

    prev_cxt->video_frm[i].addr_phy.addr_v = 0;
    prev_cxt->video_reserved_frm.addr_phy.addr_v = 0;

    if (prev_cxt->prev_param.prev_rot) {
        for (i = 0; i < PREV_ROT_FRM_ALLOC_CNT; i++) {
            prev_cxt->video_rot_frm[i].buf_size = frame_size;
            prev_cxt->video_rot_frm[i].addr_vir.addr_y =
                prev_cxt->video_virt_addr_array[prev_num + i];
            prev_cxt->video_rot_frm[i].addr_vir.addr_u =
                prev_cxt->video_rot_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->video_rot_frm[i].addr_phy.addr_y =
                prev_cxt->video_phys_addr_array[prev_num + i];
            prev_cxt->video_rot_frm[i].addr_phy.addr_u =
                prev_cxt->video_rot_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->video_rot_frm[i].addr_phy.addr_v = 0;
            prev_cxt->video_rot_frm[i].fd =
                prev_cxt->video_fd_array[prev_num + i];
            prev_cxt->video_rot_frm[i].fmt = prev_cxt->prev_param.preview_fmt;
            prev_cxt->video_rot_frm[i].size.width =
                prev_cxt->actual_video_size.width;
            prev_cxt->video_rot_frm[i].size.height =
                prev_cxt->actual_video_size.height;
        }
    }

    if (prev_cxt->prev_param.is_ultra_wide) {
        for (i = 0; i < VIDEO_ULTRA_WIDE_ALLOC_CNT; i++) {
            prev_cxt->video_ultra_wide_frm[i].buf_size = frame_size;
            prev_cxt->video_ultra_wide_frm[i].addr_vir.addr_y =
                prev_cxt->video_virt_addr_array[prev_num + i];
            prev_cxt->video_ultra_wide_frm[i].addr_vir.addr_u =
                prev_cxt->video_ultra_wide_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->video_ultra_wide_frm[i].addr_phy.addr_y =
                prev_cxt->video_phys_addr_array[prev_num + i];
            prev_cxt->video_ultra_wide_frm[i].addr_phy.addr_u =
                prev_cxt->video_ultra_wide_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->video_ultra_wide_frm[i].addr_phy.addr_v = 0;
            prev_cxt->video_ultra_wide_frm[i].fd =
                prev_cxt->video_fd_array[prev_num + i];
            prev_cxt->video_ultra_wide_frm[i].fmt =
                prev_cxt->prev_param.preview_fmt;
            prev_cxt->video_ultra_wide_frm[i].size.width =
                prev_cxt->actual_video_size.width;
            prev_cxt->video_ultra_wide_frm[i].size.height =
                prev_cxt->actual_video_size.height;
        }
        if (is_restart) {
            buffer->count = 0;
            for (i = 0; i < VIDEO_ULTRA_WIDE_ALLOC_CNT; i++) {
                if (1 == *(&prev_cxt->video_ultra_wide_frm_is_lock[0] + i)) {
                    buffer->addr[buffer->count].addr_y =
                        prev_cxt->video_ultra_wide_frm[i].addr_phy.addr_y;
                    buffer->addr[buffer->count].addr_u =
                        prev_cxt->video_ultra_wide_frm[i].addr_phy.addr_u;
                    buffer->addr_vir[buffer->count].addr_y =
                        prev_cxt->video_ultra_wide_frm[i].addr_vir.addr_y;
                    buffer->addr_vir[buffer->count].addr_u =
                        prev_cxt->video_ultra_wide_frm[i].addr_vir.addr_u;
                    buffer->fd[buffer->count] =
                        prev_cxt->video_ultra_wide_frm[i].fd;
                    buffer->count++;
                }
            }
        }
    }

    CMR_LOGV("X %ld", ret);
    return ret;
}

cmr_int prev_free_video_buf(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    cmr_uint video_mem_num = prev_cxt->video_mem_num;
    cmr_u32 ultra_wide_mem_num = PREV_ULTRA_WIDE_ALLOC_CNT;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_VIDEO, handle->oem_handle,
                          prev_cxt->video_phys_addr_array,
                          prev_cxt->video_virt_addr_array,
                          prev_cxt->video_fd_array, prev_cxt->video_mem_num);

        if (prev_cxt->prev_param.is_ultra_wide) {
            mem_ops->free_mem(CAMERA_VIDEO_ULTRA_WIDE, handle->oem_handle,
                              prev_cxt->video_phys_addr_array + video_mem_num,
                              prev_cxt->video_virt_addr_array + video_mem_num,
                              prev_cxt->video_fd_array + video_mem_num,
                              ultra_wide_mem_num);
            cmr_bzero(prev_cxt->video_ultra_wide_handle_array,
                      (ultra_wide_mem_num) * sizeof(void *));
            mem_ops->free_mem(
                CAMERA_VIDEO_EIS_ULTRA_WIDE, handle->oem_handle,
                &prev_cxt->eis_video_phys_addr, &prev_cxt->eis_video_virt_addr,
                &prev_cxt->eis_video_fd, prev_cxt->eis_video_ultra_wide_mem_num);
            prev_cxt->dst_eis_video_buffer_handle = NULL;
        }

        cmr_bzero(prev_cxt->video_phys_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->video_virt_addr_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->video_fd_array,
                  (PREV_FRM_CNT + PREV_ROT_FRM_CNT) * sizeof(cmr_s32));

        mem_ops->free_mem(CAMERA_VIDEO_RESERVED, handle->oem_handle,
                          (cmr_uint *)prev_cxt->video_reserved_phys_addr,
                          (cmr_uint *)prev_cxt->video_reserved_virt_addr,
                          &prev_cxt->video_reserved_fd, (cmr_u32)1);
        prev_cxt->video_reserved_phys_addr = 0;
        prev_cxt->video_reserved_virt_addr = 0;
        prev_cxt->video_reserved_fd = 0;
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int prev_alloc_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 i = 0;
    cmr_u32 total_mem_size = 0;
    cmr_u32 mem_size, buffer_size, frame_size, y_addr, u_addr = 0;
    cmr_s32 fd = 0;
    cmr_uint y_addr_vir, u_addr_vir = 0;
    cmr_u32 no_scaling = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    struct img_size *cap_max_size = NULL;
    struct sensor_mode_info *sensor_mode = NULL;
    struct cmr_cap_2_frm cap_2_mems;
    struct img_frm *cur_img_frm = NULL;
    struct cmr_zoom_param *zoom_param = NULL;

    cmr_u32 sum = 0;
    cmr_u32 is_normal_cap = 0;
    cmr_int zoom_post_proc = 0;
    cmr_u32 channel_buffer_size = 0;
    cmr_u32 cap_sum = 0;
    int32_t buffer_id = 0;
    cmr_int is_need_scaling = 1;
    cmr_u32 hdr_cap_sum = HDR_CAP_NUM - 1;
    cmr_u32 super_macro_size = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    sum = 1;
    cap_sum = CMR_CAPTURE_MEM_SUM;
    CMR_LOGD("camera_id %d, is_restart %d", camera_id, is_restart);
    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    cap_max_size = &prev_cxt->max_size;
    sensor_mode = &prev_cxt->sensor_info.mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    if (!prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        is_normal_cap = 1;
    } else {
        is_normal_cap = 0;
    }

    if (prev_cxt->cap_org_size.width * prev_cxt->cap_org_size.height >
        cap_max_size->width * cap_max_size->height) {
        cap_max_size = &prev_cxt->cap_org_size;
    }

    is_need_scaling = prev_is_need_scaling(handle, camera_id);
    CMR_LOGD("capture format: %d", prev_cxt->cap_org_fmt);
    /*caculate memory size for capture*/
    total_mem_size = camera_get_mem_size(handle, camera_id);
    if (total_mem_size == 0) {
        CMR_LOGE("get total mem size fail!");
        return CMR_CAMERA_NO_MEM;
    }
    sum = 1;
    /*alloc capture buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!is_restart) {
        mem_ops->alloc_mem(CAMERA_SNAPSHOT, handle->oem_handle, &total_mem_size,
                           &sum, prev_cxt->cap_phys_addr_array,
                           prev_cxt->cap_virt_addr_array,
                           prev_cxt->cap_fd_array);
#if 0 // for coverity 181595
        for (i = 1; i < CMR_CAPTURE_MEM_SUM; i++) {
            prev_cxt->cap_phys_addr_array[i] = prev_cxt->cap_phys_addr_array[0];
            prev_cxt->cap_virt_addr_array[i] = prev_cxt->cap_virt_addr_array[0];
            prev_cxt->cap_fd_array[i] = prev_cxt->cap_fd_array[0];
        }
#endif
        CMR_LOGD("virt_addr 0x%lx, fd 0x%x", prev_cxt->cap_virt_addr_array[0],
                 prev_cxt->cap_fd_array[0]);

        /*check memory valid*/
        CMR_LOGD("cap mem size 0x%x, mem_num %d", total_mem_size,
                 CMR_CAPTURE_MEM_SUM);
        for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
            if ((0 == prev_cxt->cap_virt_addr_array[i]) ||
                (0 == prev_cxt->cap_fd_array[i])) {
                CMR_LOGE("memory is invalid");
                return CMR_CAMERA_NO_MEM;
            }
        }
    }
#ifdef SUPER_MACRO
    if (prev_cxt->is_super == 1 && !is_restart) {
        CMR_LOGI("macro photo, w*h %d, %d, size %d", cap_max_size->width, cap_max_size->height,
            cap_max_size->width*cap_max_size->height*3/2);
        super_macro_size += cap_max_size->width*cap_max_size->height*3/2;
        super_macro_size += sizeof(int)*3;
        mem_ops->alloc_mem(CAMERA_SNAPSHOT, handle->oem_handle, &super_macro_size,
                           &sum, prev_cxt->super_phys_addr_array,
                           prev_cxt->super_virt_addr_array,
                           prev_cxt->super_fd_array);

        CMR_LOGV("super mem size 0x%x, addr 0x%x", super_macro_size,
            prev_cxt->super_virt_addr_array[0]);
    }
#endif

    /*arrange the buffer*/
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        int buf_4in1_flag = 0;

        cmr_bzero(&cap_2_mems, sizeof(struct cmr_cap_2_frm));
        cap_2_mems.mem_frm.buf_size = total_mem_size;
        cap_2_mems.mem_frm.addr_phy.addr_y = prev_cxt->cap_phys_addr_array[i];
        cap_2_mems.mem_frm.addr_vir.addr_y = prev_cxt->cap_virt_addr_array[i];
        cap_2_mems.mem_frm.fd = prev_cxt->cap_fd_array[i];
        cap_2_mems.type = CAMERA_MEM_NO_ALIGNED;
        cap_2_mems.zoom_post_proc = zoom_post_proc;

        if (check_software_remosaic(prev_cxt))
            buf_4in1_flag = 1;
        if (is_normal_cap) {
            ret = ((IMG_ANGLE_0 != prev_cxt->prev_param.cap_rot) ||
                 prev_cxt->prev_param.is_cfg_rot_cap);
        } else {
            ret = (prev_cxt->prev_param.is_cfg_rot_cap &&
                 (IMG_ANGLE_0 != prev_cxt->prev_param.encode_angle));
        }

        ret = camera_arrange_capture_buf(
                &cap_2_mems, &prev_cxt->cap_sn_size,
                &prev_cxt->cap_sn_trim_rect, &prev_cxt->max_size,
                prev_cxt->cap_org_fmt, &prev_cxt->cap_org_size,
                &prev_cxt->prev_param.thumb_size, &prev_cxt->cap_mem[i],
                &prev_cxt->sensor_info,
                ret, is_need_scaling, 1, buf_4in1_flag);
    }

#ifdef SUPER_MACRO
    if (prev_cxt->is_super == 1) {
        prev_cxt->cap_mem[0].super_macro.fd = prev_cxt->super_fd_array[0];
        prev_cxt->cap_mem[0].super_macro.buf_size = cap_max_size->width*cap_max_size->height*3/2;
        prev_cxt->cap_mem[0].super_macro.size.width = cap_max_size->width;
        prev_cxt->cap_mem[0].super_macro.size.height = cap_max_size->height;
        prev_cxt->cap_mem[0].super_macro.addr_vir.addr_y = prev_cxt->super_virt_addr_array[0];
    }
#endif
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_CAP0_ID_BASE;
    buffer->count = CMR_CAPTURE_MEM_SUM;
    buffer->flag = BUF_FLAG_INIT;
    buffer_size =
        prev_cxt->actual_pic_size.width * prev_cxt->actual_pic_size.height;
    frame_size = buffer_size * 3 / 2;
    CMR_LOGD("prev_cxt->cap_org_fmt: %ld, encode_angle %d",
             prev_cxt->cap_org_fmt, prev_cxt->prev_param.encode_angle);
    for (i = 0; i < buffer->count; i++) {
        if (CAM_IMG_FMT_BAYER_MIPI_RAW == prev_cxt->cap_org_fmt) {
            if ((IMG_ANGLE_0 != prev_cxt->prev_param.cap_rot) ||
                prev_cxt->prev_param.is_cfg_rot_cap) {
                if (prev_cxt->cap_mem[i].cap_yuv_rot.fd ==
                    prev_cxt->cap_mem[i].cap_yuv.fd) {
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].target_yuv.fd;
                } else {
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].cap_yuv.fd;
                }
            } else {
                if (!is_need_scaling) {
                    CMR_LOGD("raw no scale, no rotation");
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].cap_yuv.fd;
                } else {
                    CMR_LOGD("raw has scale, no rotation");
                    prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y =
                        prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                    prev_cxt->cap_mem[i].cap_raw.fd =
                        prev_cxt->cap_mem[i].target_yuv.fd;
                }
            }

            buffer_size = sensor_mode->trim_width * sensor_mode->trim_height;
            mem_size = prev_cxt->cap_mem[i].cap_raw.buf_size;
            fd = prev_cxt->cap_mem[i].cap_raw.fd;
            y_addr = prev_cxt->cap_mem[i].cap_raw.addr_phy.addr_y;
            u_addr = y_addr;
            y_addr_vir = prev_cxt->cap_mem[i].cap_raw.addr_vir.addr_y;
            u_addr_vir = y_addr_vir;
            frame_size = buffer_size * RAWRGB_BIT_WIDTH / 8;
            cur_img_frm = &prev_cxt->cap_mem[i].cap_raw;
        } else if (CAM_IMG_FMT_JPEG == prev_cxt->cap_org_fmt) {
            mem_size = prev_cxt->cap_mem[i].target_jpeg.buf_size;
            if (CAP_SIM_ROT(handle, camera_id)) {
                fd = prev_cxt->cap_mem[i].cap_yuv.fd;
                y_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                y_addr_vir = prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv;
            } else {
                fd = prev_cxt->cap_mem[i].target_jpeg.fd;
                y_addr = prev_cxt->cap_mem[i].target_jpeg.addr_phy.addr_y;
                y_addr_vir = prev_cxt->cap_mem[i].target_jpeg.addr_vir.addr_y;
                cur_img_frm = &prev_cxt->cap_mem[i].target_jpeg;
            }
            u_addr = y_addr;
            u_addr_vir = y_addr_vir;
            frame_size = CMR_JPEG_SZIE(prev_cxt->actual_pic_size.width,
                                       prev_cxt->actual_pic_size.height);
        } else if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->cap_org_fmt ||
                   CAM_IMG_FMT_YUV420_NV12 == prev_cxt->cap_org_fmt) {
            if (is_normal_cap) {
                if ((IMG_ANGLE_0 != prev_cxt->prev_param.cap_rot) ||
                    (prev_cxt->prev_param.is_cfg_rot_cap &&
                     (IMG_ANGLE_0 != prev_cxt->prev_param.encode_angle))) {
                    mem_size = prev_cxt->cap_mem[i].cap_yuv_rot.buf_size;
                    fd = prev_cxt->cap_mem[i].cap_yuv_rot.fd;
                    y_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_y;
                    u_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_u;
                    y_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_y;
                    u_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_u;
                    frame_size = prev_cxt->cap_org_size.width *
                                 prev_cxt->cap_org_size.height * 3 / 2;
                    cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv_rot;
                } else {
                    if (!is_need_scaling) {
                        mem_size = prev_cxt->cap_mem[i].target_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].target_yuv.fd;
                        y_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                        u_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].target_yuv;
                    } else {
                        mem_size = prev_cxt->cap_mem[i].cap_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].cap_yuv.fd;
                        y_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                        u_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv;
                    }
                    frame_size = buffer_size * 3 / 2;
                }
            } else {
                if (prev_cxt->prev_param.is_cfg_rot_cap &&
                    (IMG_ANGLE_0 != prev_cxt->prev_param.encode_angle)) {
                    mem_size = prev_cxt->cap_mem[i].cap_yuv_rot.buf_size;
                    fd = prev_cxt->cap_mem[i].cap_yuv_rot.fd;
                    y_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_y;
                    u_addr = prev_cxt->cap_mem[i].cap_yuv_rot.addr_phy.addr_u;
                    y_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_y;
                    u_addr_vir =
                        prev_cxt->cap_mem[i].cap_yuv_rot.addr_vir.addr_u;
                    frame_size = prev_cxt->cap_org_size.width *
                                 prev_cxt->cap_org_size.height * 3 / 2;
                    cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv_rot;

                } else {
                    if (!is_need_scaling) {
                        mem_size = prev_cxt->cap_mem[i].target_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].target_yuv.fd;
                        y_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_y;
                        u_addr =
                            prev_cxt->cap_mem[i].target_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].target_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].target_yuv;
                    } else {
                        mem_size = prev_cxt->cap_mem[i].cap_yuv.buf_size;
                        fd = prev_cxt->cap_mem[i].cap_yuv.fd;
                        y_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_y;
                        u_addr = prev_cxt->cap_mem[i].cap_yuv.addr_phy.addr_u;
                        y_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_y;
                        u_addr_vir =
                            prev_cxt->cap_mem[i].cap_yuv.addr_vir.addr_u;
                        cur_img_frm = &prev_cxt->cap_mem[i].cap_yuv;
                    }
                    frame_size = buffer_size * 3 / 2;
                }
            }
        } else {
            CMR_LOGE("Unsupported capture format!");
            ret = CMR_CAMERA_NO_SUPPORT;
            break;
        }

        CMR_LOGD("fd 0x%x", cur_img_frm->fd);
        if (0 == cur_img_frm->fd) {
            ret = CMR_CAMERA_FAIL;
            CMR_LOGD("cur_img_frm->fd is null");
            break;
        }

        prev_cxt->cap_frm[i].size.width = prev_cxt->cap_org_size.width;
        prev_cxt->cap_frm[i].size.height = prev_cxt->cap_org_size.height;
        prev_cxt->cap_frm[i].fmt = prev_cxt->cap_org_fmt;
        prev_cxt->cap_frm[i].buf_size = cur_img_frm->buf_size;
        prev_cxt->cap_frm[i].addr_phy.addr_y = y_addr;
        prev_cxt->cap_frm[i].addr_phy.addr_u = u_addr;
        prev_cxt->cap_frm[i].addr_vir.addr_y = y_addr_vir;
        prev_cxt->cap_frm[i].addr_vir.addr_u = u_addr_vir;
        prev_cxt->cap_frm[i].fd = fd;
        buffer->addr[i].addr_y = prev_cxt->cap_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->cap_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->cap_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->cap_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->cap_frm[i].fd;
    }

    buffer->length = frame_size;

    ATRACE_END();
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_free_cap_buf(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_u32 sum = 0;
    cmr_u32 is_pre_alloc_cap_mem = 0;
    cmr_u32 cap_sum = 0;
    cmr_u32 hdr_cap_sum = HDR_CAP_NUM - 1;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

#ifdef CONFIG_PRE_ALLOC_CAPTURE_MEM
    is_pre_alloc_cap_mem = 1;
#endif

    sum = 1;
    cap_sum = CMR_CAPTURE_MEM_SUM;
    CMR_LOGD("camera_id %d, is_restart %d, is_pre_alloc_cap_mem %d", camera_id,
             is_restart, is_pre_alloc_cap_mem);
    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (0 == prev_cxt->cap_fd_array[0]) {
        CMR_LOGE("already freed");
        return ret;
    }

    if (!is_pre_alloc_cap_mem && !is_restart) {
        CMR_LOGD("fre cap mem really");
        mem_ops->free_mem(
            CAMERA_SNAPSHOT, handle->oem_handle, prev_cxt->cap_phys_addr_array,
            prev_cxt->cap_virt_addr_array, prev_cxt->cap_fd_array, sum);

        cmr_bzero(prev_cxt->cap_phys_addr_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_virt_addr_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_fd_array,
                  CMR_CAPTURE_MEM_SUM * sizeof(cmr_s32));
#ifdef SUPER_MACRO
        if (prev_cxt->is_super == 1) {
            CMR_LOGD("free super buffer");
            mem_ops->free_mem(
                CAMERA_SNAPSHOT, handle->oem_handle, prev_cxt->super_phys_addr_array,
                prev_cxt->super_virt_addr_array, prev_cxt->super_fd_array, sum);

            cmr_bzero(prev_cxt->super_phys_addr_array,
                      CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
            cmr_bzero(prev_cxt->super_virt_addr_array,
                      CMR_CAPTURE_MEM_SUM * sizeof(cmr_uint));
            cmr_bzero(prev_cxt->super_fd_array,
                      CMR_CAPTURE_MEM_SUM * sizeof(cmr_s32));
        }
#endif
    }

    CMR_LOGD("X");
    return ret;
}

cmr_int prev_alloc_cap_reserve_buf(struct prev_handle *handle,
                                   cmr_u32 camera_id, cmr_u32 is_restart) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_u32 i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 cap_rot = 0;
    cmr_uint reserved_count = 1;
    cmr_u32 aligned_type = 0;
    cmr_u32 small_w, small_h, reserved_buf_size;
    cmr_u32 mipi_raw;
    struct prev_context *prev_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;
    struct memory_param *mem_ops = NULL;
    cmr_int zoom_post_proc = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    mem_ops = &prev_cxt->prev_param.memory_setting;
    if (ZOOM_POST_PROCESS == zoom_post_proc) {
        width = prev_cxt->max_size.width;
        height = prev_cxt->max_size.height;
    } else {
//        width = prev_cxt->actual_pic_size.width;
//        height = prev_cxt->actual_pic_size.height;
        width = prev_cxt->cap_org_size.width;
        height = prev_cxt->cap_org_size.height;
    }

    small_w = width >> 1;
    small_h = height >> 1;

    CMR_LOGD("is_restart %d, size[%d,%d], small_size[%d,%d], fmt 0x%x",
             is_restart, width, height, small_w, small_h, prev_cxt->cap_org_fmt);
    cap_rot = 0; // prev_cxt->prev_param.cap_rot;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init preview memory info*/
    buffer_size = width * height;
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->cap_org_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->cap_org_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.cap_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.cap_rot) {
            prev_cxt->cap_zsl_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->cap_zsl_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->cap_org_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height * 2);
        if (check_software_remosaic(prev_cxt))
            prev_cxt->cap_zsl_mem_size += (small_w * small_h * 2);

        CMR_LOGI("cap_zsl_mem_size = %d", prev_cxt->cap_zsl_mem_size);
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->cap_org_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }
    prev_cxt->cap_zsl_mem_num = ZSL_FRM_ALLOC_CNT;

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mipi_raw = prev_cxt->sensor_info.sn_interface.is_loose ? 0 : 1;
        if (mipi_raw)
            reserved_buf_size = ((width + 15) & ~15) * height * 10 / 8;
        else
            reserved_buf_size = ((width + 15) & ~15) * height * 2;

        if (reserved_buf_size < prev_cxt->cap_zsl_mem_size)
            reserved_buf_size = prev_cxt->cap_zsl_mem_size;

        mem_ops->alloc_mem(CAMERA_SNAPSHOT_ZSL_RESERVED, handle->oem_handle,
                           (cmr_u32 *)&reserved_buf_size,
                           (cmr_u32 *)&reserved_count,
                           &prev_cxt->cap_zsl_reserved_phys_addr,
                           &prev_cxt->cap_zsl_reserved_virt_addr,
                           &prev_cxt->cap_zsl_reserved_fd);
        CMR_LOGD("reserved_fd %d, mipi_raw %d, w %d h %d, size 0x%x  zsl_size 0x%x\n",
                          prev_cxt->cap_zsl_reserved_fd,  mipi_raw, width, height,
                          reserved_buf_size, (cmr_u32)prev_cxt->cap_zsl_mem_size);
    }

    frame_size = prev_cxt->cap_zsl_mem_size;

    prev_cxt->cap_zsl_reserved_frm.buf_size = frame_size;
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y =
        prev_cxt->cap_zsl_reserved_virt_addr;
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u =
        prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y + buffer_size;
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y =
        prev_cxt->cap_zsl_reserved_phys_addr;
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u =
        prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y + buffer_size;
    prev_cxt->cap_zsl_reserved_frm.fd = prev_cxt->cap_zsl_reserved_fd;
    prev_cxt->cap_zsl_reserved_frm.fmt = prev_cxt->cap_org_fmt;
    prev_cxt->cap_zsl_reserved_frm.size.width = width;
    prev_cxt->cap_zsl_reserved_frm.size.height = height;

    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_v = 0;

    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_free_cap_reserve_buf(struct prev_handle *handle, cmr_u32 camera_id,
                                  cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_SNAPSHOT_ZSL_RESERVED, handle->oem_handle,
                          &prev_cxt->cap_zsl_reserved_phys_addr,
                          &prev_cxt->cap_zsl_reserved_virt_addr,
                          &prev_cxt->cap_zsl_reserved_fd, (cmr_u32)1);
        prev_cxt->cap_zsl_reserved_phys_addr = 0;
        prev_cxt->cap_zsl_reserved_virt_addr = 0;
        prev_cxt->cap_zsl_reserved_fd = 0;
    }

    CMR_LOGV("X");
    return ret;
}

void prev_cal_3dnr_smallsize(struct prev_handle *handle,cmr_u32 camera_id)
{
   struct prev_context *prev_cxt = NULL;
   struct camera_context *cxt = (struct camera_context *)handle->oem_handle;
       prev_cxt = &handle->prev_cxt[camera_id];
   if ((cxt->snp_cxt.request_size.width * 10) / cxt->snp_cxt.request_size.height <= 10) {
       prev_cxt->threednr_cap_smallheight= CMR_3DNR_1_1_SMALL_HEIGHT;
        prev_cxt->threednr_cap_smallwidth = CMR_3DNR_1_1_SMALL_WIDTH;
    } else if ((cxt->snp_cxt.request_size.width * 10) / cxt->snp_cxt.request_size.height <= 13) {
       prev_cxt->threednr_cap_smallheight = CMR_3DNR_4_3_SMALL_HEIGHT;
         prev_cxt->threednr_cap_smallwidth = CMR_3DNR_4_3_SMALL_WIDTH;
    } else if ((cxt->snp_cxt.request_size.width * 10) / cxt->snp_cxt.request_size.height <= 18) {
       prev_cxt->threednr_cap_smallheight = CMR_3DNR_16_9_SMALL_HEIGHT;
       prev_cxt->threednr_cap_smallwidth = CMR_3DNR_16_9_SMALL_WIDTH;
    } else if ((cxt->snp_cxt.request_size.width * 10) / cxt->snp_cxt.request_size.height <= 20) {
        prev_cxt->threednr_cap_smallheight = CMR_3DNR_18_9_SMALL_HEIGHT;
        prev_cxt->threednr_cap_smallwidth = CMR_3DNR_18_9_SMALL_WIDTH;
    } else if ((cxt->snp_cxt.request_size.width * 10) / cxt->snp_cxt.request_size.height <= 22) {
       prev_cxt->threednr_cap_smallheight = CMR_3DNR_19_9_SMALL_HEIGHT;
        prev_cxt->threednr_cap_smallwidth = CMR_3DNR_19_9_SMALL_WIDTH;
    } else {
        CMR_LOGE("incorrect 3dnr small image mapping, using 16*9 as the "
                 "default setting");
       prev_cxt->threednr_cap_smallheight = CMR_3DNR_16_9_SMALL_HEIGHT;
       prev_cxt->threednr_cap_smallwidth = CMR_3DNR_16_9_SMALL_WIDTH;
    }

}

cmr_int prev_alloc_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_u32 i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 cap_rot = 0;
    cmr_uint reserved_count = 1;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;
    struct memory_param *mem_ops = NULL;
    cmr_int zoom_post_proc = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    CMR_LOGD("is_restart %d,zoom_post_proc %d", is_restart, zoom_post_proc);

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    mem_ops = &prev_cxt->prev_param.memory_setting;
    if (ZOOM_POST_PROCESS == zoom_post_proc) {
        width = prev_cxt->cap_sn_size.width;
        height = prev_cxt->cap_sn_size.height;
    } else if (ZOOM_POST_PROCESS_WITH_TRIM == zoom_post_proc) {
        width = prev_cxt->max_size.width;
        height = prev_cxt->max_size.height;
    } else {
//        width = prev_cxt->actual_pic_size.width;
//        height = prev_cxt->actual_pic_size.height;
        width = prev_cxt->cap_org_size.width;
        height = prev_cxt->cap_org_size.height;
    }

    CMR_LOGD("width %d height %d", width, height);
    cap_rot = 0; // prev_cxt->prev_param.cap_rot;
    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /*init preview memory info*/
    buffer_size = width * height;
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->cap_org_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->cap_org_fmt) {
        if (IMG_ANGLE_90 == prev_cxt->prev_param.cap_rot ||
            IMG_ANGLE_270 == prev_cxt->prev_param.cap_rot) {
            prev_cxt->cap_zsl_mem_size =
                (height + camera_get_aligned_size(aligned_type, height / 2)) *
                width;
        } else {
            prev_cxt->cap_zsl_mem_size =
                (width + camera_get_aligned_size(aligned_type, width / 2)) *
                height;
        }
        prev_cxt->cap_org_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW == prev_cxt->cap_org_fmt) {
        prev_cxt->cap_zsl_mem_size = (width * height * 2);
    } else {
        CMR_LOGE("unsupprot fmt %ld", prev_cxt->cap_org_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->cap_zsl_mem_num = ZSL_FRM_ALLOC_CNT;
    if (prev_cxt->prev_param.cap_rot) {
        CMR_LOGD("need increase buf for rotation");
        prev_cxt->cap_zsl_mem_num += PREV_ROT_FRM_CNT;
    }

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!is_restart) {
        cmr_uint cap_zsl_mem_num = prev_cxt->cap_zsl_mem_num;
        cmr_u32 ultra_wide_mem_num = 0;
        cmr_uint real_width = width;
        cmr_uint real_height = height;

        prev_cxt->cap_zsl_mem_valid_num = 0;
        if (prev_cxt->prev_param.is_ultra_wide) {
            ultra_wide_mem_num = ZSL_ULTRA_WIDE_ALLOC_CNT;
            mem_ops->gpu_alloc_mem(
                CAMERA_SNAPSHOT_ULTRA_WIDE, handle->oem_handle,
                (cmr_u32 *)&prev_cxt->cap_zsl_mem_size,
                (cmr_u32 *)&prev_cxt->cap_zsl_mem_num,
                prev_cxt->cap_zsl_phys_addr_array,
                prev_cxt->cap_zsl_virt_addr_array, prev_cxt->cap_zsl_fd_array,
                prev_cxt->cap_zsl_dst_handle_array, &real_width, &real_height);

            memcpy(prev_cxt->cap_zsl_dst_fd_array, prev_cxt->cap_zsl_fd_array,
                   sizeof(prev_cxt->cap_zsl_dst_fd_array));

            mem_ops->gpu_alloc_mem(
                CAMERA_PREVIEW_ULTRA_WIDE, handle->oem_handle,
                (cmr_u32 *)&prev_cxt->cap_zsl_mem_size, &ultra_wide_mem_num,
                prev_cxt->cap_zsl_phys_addr_array + cap_zsl_mem_num,
                prev_cxt->cap_zsl_virt_addr_array + cap_zsl_mem_num,
                prev_cxt->cap_zsl_fd_array + cap_zsl_mem_num,
                prev_cxt->cap_zsl_ultra_wide_handle_array, &real_width,
                &real_height);
        } else {

        if (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
            prev_cxt->prev_param.is_auto_3dnr == CAMERA_3DNR_AUTO)
        {
                prev_cal_3dnr_smallsize (handle,camera_id);
                prev_cxt->cap_zsl_mem_size += (prev_cxt->threednr_cap_smallwidth *
                                   prev_cxt->threednr_cap_smallheight * 3 / 2);
                CMR_LOGI("3dnr type ,add  small buffer to zsl buffer,smallwith=%d,height=%d",prev_cxt->threednr_cap_smallwidth,
                                     prev_cxt->threednr_cap_smallheight);
                mem_ops->gpu_alloc_mem(
                CAMERA_SNAPSHOT_SW3DNR, handle->oem_handle,
                (cmr_u32 *)&prev_cxt->cap_zsl_mem_size, (cmr_u32 *)&prev_cxt->cap_zsl_mem_num,
                prev_cxt->cap_zsl_phys_addr_array ,
                prev_cxt->cap_zsl_virt_addr_array ,
                prev_cxt->cap_zsl_fd_array ,
                prev_cxt->cap_zsl_3dnr_handle_array, &real_width,
                &real_height);
                CMR_LOGI("real_width=%d,real_height=%d",real_width,real_height);
        }
        else
        {
                mem_ops->alloc_mem(CAMERA_SNAPSHOT_ZSL, handle->oem_handle,
                               (cmr_u32 *)&prev_cxt->cap_zsl_mem_size,
                               (cmr_u32 *)&prev_cxt->cap_zsl_mem_num,
                               prev_cxt->cap_zsl_phys_addr_array,
                               prev_cxt->cap_zsl_virt_addr_array,
                               prev_cxt->cap_zsl_fd_array);
         }
      }

        /*check memory valid*/
        CMR_LOGD("prev_mem_size 0x%lx, mem_num %ld", prev_cxt->cap_zsl_mem_size,
                 prev_cxt->cap_zsl_mem_num);
        for (i = 0; i < (cap_zsl_mem_num + ultra_wide_mem_num); i++) {
            CMR_LOGD("%d, virt_addr 0x%lx, fd 0x%x", i,
                     prev_cxt->cap_zsl_virt_addr_array[i],
                     prev_cxt->cap_zsl_fd_array[i]);

            if ((0 == prev_cxt->cap_zsl_virt_addr_array[i]) ||
                0 == prev_cxt->cap_zsl_fd_array[i]) {
                if (i >= ZSL_FRM_ALLOC_CNT) {
                    CMR_LOGE("memory is invalid");
                    return CMR_CAMERA_NO_MEM;
                }
            } else {
                if (i < ZSL_FRM_ALLOC_CNT) {
                    CMR_LOGV("%d, graphic handle:%p", i, prev_cxt->cap_zsl_dst_handle_array[i]);
                    prev_cxt->cap_zsl_mem_valid_num++;
                }
            }
        }
    }

    frame_size = prev_cxt->cap_zsl_mem_size;
    prev_num = prev_cxt->cap_zsl_mem_num;
    if (prev_cxt->prev_param.cap_rot) {
        prev_num = prev_cxt->cap_zsl_mem_num - PREV_ROT_FRM_CNT;
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_CAP1_ID_BASE;
    buffer->count = prev_cxt->cap_zsl_mem_valid_num;
    buffer->length = frame_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_u32)prev_cxt->cap_zsl_mem_valid_num; i++) {
        prev_cxt->cap_zsl_frm[i].buf_size = frame_size;
        prev_cxt->cap_zsl_frm[i].addr_vir.addr_y =
            prev_cxt->cap_zsl_virt_addr_array[i];
        prev_cxt->cap_zsl_frm[i].addr_vir.addr_u =
            prev_cxt->cap_zsl_frm[i].addr_vir.addr_y + buffer_size;
        prev_cxt->cap_zsl_frm[i].addr_phy.addr_y =
            prev_cxt->cap_zsl_phys_addr_array[i];
        prev_cxt->cap_zsl_frm[i].addr_phy.addr_u =
            prev_cxt->cap_zsl_frm[i].addr_phy.addr_y + buffer_size;
        prev_cxt->cap_zsl_frm[i].fd = prev_cxt->cap_zsl_fd_array[i];
        prev_cxt->cap_zsl_frm[i].fmt = prev_cxt->cap_org_fmt;
        prev_cxt->cap_zsl_frm[i].size.width = width;
        prev_cxt->cap_zsl_frm[i].size.height = height;

        buffer->addr[i].addr_y = prev_cxt->cap_zsl_frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->cap_zsl_frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->cap_zsl_frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->cap_zsl_frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->cap_zsl_frm[i].fd;
    }
    /*prev_cxt->cap_zsl_reserved_frm.buf_size        = frame_size;
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y =
    prev_cxt->cap_zsl_reserved_virt_addr;
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u =
    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y + buffer_size;
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y =
    prev_cxt->cap_zsl_reserved_phys_addr;
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u =
    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y + buffer_size;
    prev_cxt->cap_zsl_reserved_frm.fmt             = prev_cxt->cap_org_fmt;
    prev_cxt->cap_zsl_reserved_frm.size.width      = width;
    prev_cxt->cap_zsl_reserved_frm.size.height     = height;*/

    prev_cxt->cap_zsl_frm[i].addr_phy.addr_v = 0;
    // prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_v = 0;

    if (prev_cxt->prev_param.cap_rot) {
        for (i = 0; i < PREV_ROT_FRM_CNT; i++) {
            prev_cxt->cap_zsl_rot_frm[i].buf_size = frame_size;
            prev_cxt->cap_zsl_rot_frm[i].addr_vir.addr_y =
                prev_cxt->cap_zsl_virt_addr_array[prev_num + i];
            prev_cxt->cap_zsl_rot_frm[i].addr_vir.addr_u =
                prev_cxt->cap_zsl_rot_frm[i].addr_vir.addr_y + buffer_size;
            prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_y =
                prev_cxt->cap_zsl_phys_addr_array[prev_num + i];
            prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_u =
                prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_y + buffer_size;
            prev_cxt->cap_zsl_rot_frm[i].addr_phy.addr_v = 0;
            prev_cxt->cap_zsl_rot_frm[i].fd =
                prev_cxt->cap_zsl_fd_array[prev_num + i];
            prev_cxt->cap_zsl_rot_frm[i].fmt = prev_cxt->cap_org_fmt;
            prev_cxt->cap_zsl_rot_frm[i].size.width = width;
            prev_cxt->cap_zsl_rot_frm[i].size.height = height;
        }
    }
    if (prev_cxt->prev_param.is_ultra_wide) {
        for (i = 0; i < ZSL_ULTRA_WIDE_ALLOC_CNT; i++) {
            prev_cxt->cap_zsl_ultra_wide_frm[i].buf_size = frame_size;
            prev_cxt->cap_zsl_ultra_wide_frm[i].addr_vir.addr_y =
                prev_cxt->cap_zsl_virt_addr_array[prev_num + i];
            prev_cxt->cap_zsl_ultra_wide_frm[i].addr_vir.addr_u =
                prev_cxt->cap_zsl_ultra_wide_frm[i].addr_vir.addr_y +
                buffer_size;
            prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_y =
                prev_cxt->cap_zsl_phys_addr_array[prev_num + i];
            prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_u =
                prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_y +
                buffer_size;
            prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_v = 0;
            prev_cxt->cap_zsl_ultra_wide_frm[i].fd =
                prev_cxt->cap_zsl_fd_array[prev_num + i];
            prev_cxt->cap_zsl_ultra_wide_frm[i].fmt =
                prev_cxt->prev_param.preview_fmt;
            prev_cxt->cap_zsl_ultra_wide_frm[i].size.width = width;
            prev_cxt->cap_zsl_ultra_wide_frm[i].size.height = height;
        }
        if (is_restart) {
            buffer->count = 0;
            for (i = 0; i < ZSL_ULTRA_WIDE_ALLOC_CNT; i++) {
                if (1 == *(&prev_cxt->cap_zsl_ultra_wide_frm_is_lock[0] + i)) {
                    buffer->addr[buffer->count].addr_y =
                        prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_y;
                    buffer->addr[buffer->count].addr_u =
                        prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_u;
                    buffer->addr_vir[buffer->count].addr_y =
                        prev_cxt->cap_zsl_ultra_wide_frm[i].addr_vir.addr_y;
                    buffer->addr_vir[buffer->count].addr_u =
                        prev_cxt->cap_zsl_ultra_wide_frm[i].addr_vir.addr_u;
                    buffer->fd[buffer->count] =
                        prev_cxt->cap_zsl_ultra_wide_frm[i].fd;
                    buffer->count++;
                }
            }
        } else {
            CMR_LOGD("set ultra_wide flag!");
            buffer->count = ZSL_ULTRA_WIDE_ALLOC_CNT;
            for (i = 0; i < buffer->count; i++) {
                buffer->addr[i].addr_y =
                    prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_y;
                buffer->addr[i].addr_u =
                    prev_cxt->cap_zsl_ultra_wide_frm[i].addr_phy.addr_u;
                buffer->addr_vir[i].addr_y =
                    prev_cxt->cap_zsl_ultra_wide_frm[i].addr_vir.addr_y;
                buffer->addr_vir[i].addr_u =
                    prev_cxt->cap_zsl_ultra_wide_frm[i].addr_vir.addr_u;
                buffer->fd[i] = prev_cxt->cap_zsl_ultra_wide_frm[i].fd;
                prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                                i, 1);
            }
        }
    }

    ATRACE_END();
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_free_zsl_buf(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        cmr_uint cap_zsl_mem_num = prev_cxt->cap_zsl_mem_num;
        cmr_uint ultra_wide_mem_num = ZSL_ULTRA_WIDE_ALLOC_CNT;

        if (prev_cxt->prev_param.is_ultra_wide) {
            mem_ops->free_mem(CAMERA_SNAPSHOT_ULTRA_WIDE, handle->oem_handle,
                              prev_cxt->cap_zsl_phys_addr_array,
                              prev_cxt->cap_zsl_virt_addr_array,
                              prev_cxt->cap_zsl_fd_array, ultra_wide_mem_num);
            cmr_bzero(prev_cxt->cap_zsl_ultra_wide_handle_array,
                      (ultra_wide_mem_num) * sizeof(void *));
        } else {

        if (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
            prev_cxt->prev_param.is_auto_3dnr == CAMERA_3DNR_AUTO) {
             CMR_LOGI("free 3dnr memory");
              mem_ops->free_mem(CAMERA_SNAPSHOT_SW3DNR, handle->oem_handle,
                              prev_cxt->cap_zsl_phys_addr_array,
                              prev_cxt->cap_zsl_virt_addr_array,
                              prev_cxt->cap_zsl_fd_array,
                              prev_cxt->cap_zsl_mem_num);
        } else {
               mem_ops->free_mem(CAMERA_SNAPSHOT_ZSL, handle->oem_handle,
                              prev_cxt->cap_zsl_phys_addr_array,
                              prev_cxt->cap_zsl_virt_addr_array,
                              prev_cxt->cap_zsl_fd_array,
                              prev_cxt->cap_zsl_mem_num);
                   }
        }

        cmr_bzero(prev_cxt->cap_zsl_phys_addr_array,
                  (ZSL_FRM_CNT + ZSL_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_zsl_virt_addr_array,
                  (ZSL_FRM_CNT + ZSL_ROT_FRM_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_zsl_fd_array,
                  (ZSL_FRM_CNT + ZSL_ROT_FRM_CNT) * sizeof(cmr_s32));
        cmr_bzero(&prev_cxt->cap_zsl_frm[0],
                  sizeof(struct img_frm) * ZSL_FRM_CNT);
        prev_cxt->cap_zsl_reserved_phys_addr = 0;
        prev_cxt->cap_zsl_reserved_virt_addr = 0;
        prev_cxt->cap_zsl_reserved_fd = 0;
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int prev_free_zsl_raw_buf(struct prev_handle *handle, cmr_u32 camera_id,
                          cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_SNAPSHOT_ZSL_RAW, handle->oem_handle,
                      prev_cxt->cap_fdr_phys_addr_array,
                      prev_cxt->cap_fdr_virt_addr_array,
                      prev_cxt->cap_fdr_fd_array,
                      prev_cxt->cap_fdr_mem_num);

        cmr_bzero(prev_cxt->cap_fdr_phys_addr_array,
                  (FDR_FRM_ALLOC_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_fdr_virt_addr_array,
                  (FDR_FRM_ALLOC_CNT) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_fdr_fd_array,
                  (FDR_FRM_ALLOC_CNT) * sizeof(cmr_s32));
    }

    prev_cxt->cap_fdr_mem_num = 0;
    //set used fdr buf cfg to zero
    cmr_bzero(&prev_cxt->cap_used_fdr_buf_cfg, sizeof(struct buffer_cfg));
    CMR_LOGD("X, cap_fdr_mem_num: %d", prev_cxt->cap_fdr_mem_num);
    return ret;
}

cmr_int prev_free_4in1_buf(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_4IN1_PROC, handle->oem_handle,
                          prev_cxt->cap_4in1_phys_addr_array,
                          prev_cxt->cap_4in1_virt_addr_array,
                          prev_cxt->cap_4in1_fd_array,
                          prev_cxt->cap_4in1_mem_num);

        cmr_bzero(prev_cxt->cap_4in1_phys_addr_array,
                  (CAP_4IN1_NUM) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_4in1_virt_addr_array,
                  (CAP_4IN1_NUM) * sizeof(cmr_uint));
        cmr_bzero(prev_cxt->cap_4in1_fd_array,
                  (CAP_4IN1_NUM) * sizeof(cmr_s32));
    }
    prev_cxt->cap_4in1_mem_num = 0;
    CMR_LOGD("X");
    return ret;
}

cmr_int prev_alloc_4in1_buf(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_size = 0;
    cmr_u32 frame_num = 0;
    cmr_u32 i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 cap_rot = 0;
    cmr_uint reserved_count = 1;
    cmr_u32 aligned_type = 0;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;
    cmr_int zoom_post_proc = 0;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    CMR_LOGD("is_restart %d", is_restart);

    cmr_bzero(prev_cxt->cap_4in1_phys_addr_array,
              (CAP_4IN1_NUM) * sizeof(cmr_uint));
    cmr_bzero(prev_cxt->cap_4in1_virt_addr_array,
              (CAP_4IN1_NUM) * sizeof(cmr_uint));
    cmr_bzero(prev_cxt->cap_4in1_fd_array, (CAP_4IN1_NUM) * sizeof(cmr_s32));

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    mem_ops = &prev_cxt->prev_param.memory_setting;

    width = cxt->sn_cxt.sensor_info.source_width_max;
    height = cxt->sn_cxt.sensor_info.source_height_max;
    CMR_LOGD("4in1 width %d height %d", width, height);

    /*init  memory info*/
    prev_cxt->cap_4in1_mem_size = (width * height * 5) / 4;
    /*alloc  buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (!is_restart) {
        prev_cxt->cap_4in1_mem_num = CAP_4IN1_NUM;
        prev_cxt->cap_4in1_mem_valid_num = 0;
        ret = mem_ops->alloc_mem(CAMERA_4IN1_PROC, handle->oem_handle,
                                 (cmr_u32 *)&prev_cxt->cap_4in1_mem_size,
                                 (cmr_u32 *)&prev_cxt->cap_4in1_mem_num,
                                 prev_cxt->cap_4in1_phys_addr_array,
                                 prev_cxt->cap_4in1_virt_addr_array,
                                 prev_cxt->cap_4in1_fd_array);
        if (ret) {
            CMR_LOGE("alloc 4in1 memory failed");
            prev_cxt->cap_4in1_mem_num = 0;
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        /*check memory valid*/
        CMR_LOGD("4in1 prev_mem_size 0x%lx, mem_num %ld",
                 prev_cxt->cap_4in1_mem_size, prev_cxt->cap_4in1_mem_num);
    }
exit:

    ATRACE_END();
    CMR_LOGV("X");
    return ret;
}

/* return: 1: 4in1,need software remosaic,0:no need
 */
cmr_int check_software_remosaic(struct prev_context *prev_cxt)
{
    if (1 == prev_cxt->prev_param.remosaic_type)
        return 1;
    return 0;
}

cmr_int prev_get_sensor_mode(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct img_size *prev_size = NULL;
    struct img_size *act_prev_size = NULL;
    struct img_size *video_size = NULL;
    struct img_size *act_video_size = NULL;
    struct img_size *channel0_size = NULL;
    struct img_size *channel1_size = NULL;
    struct img_size *channel2_size = NULL;
    struct img_size *channel3_size = NULL;
    struct img_size *channel4_size = NULL;
    struct img_size *org_pic_size = NULL;
    struct img_size *act_pic_size = NULL;
    struct img_size *alg_pic_size = NULL;
    struct img_size *channel1_act_pic_size = NULL;
    struct img_size *channel2_act_pic_size = NULL;
    struct img_size *channel3_act_pic_size = NULL;
    struct img_size *channel4_act_pic_size = NULL;
    struct sensor_exp_info *sensor_info = NULL;
    cmr_u32 prev_rot = 0;
    cmr_u32 cap_rot = 0;
    cmr_u32 cfg_cap_rot = 0;
    cmr_u32 is_cfg_rot_cap = 0;
    cmr_u32 aligned_type = 0;
    cmr_u32 mode_flag = 0;
    cmr_int sn_mode = 0;
    cmr_uint valid_max_sn_mode = 0;
    struct sensor_mode_fps_tag fps_info;
    struct camera_context *cam_cxt = (struct camera_context *)(handle->oem_handle);
    struct setting_context *setting_cxt = &cam_cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;

    cmr_bzero(&fps_info, sizeof(struct sensor_mode_fps_tag));

#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
    struct prev_sn_param_dvfs_type dvfs_param;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;
#endif

    CMR_LOGD("E");

    CHECK_HANDLE_VALID(handle);

    prev_size =
        &handle->prev_cxt[camera_id].prev_param.preview_size;
    act_prev_size =
        &handle->prev_cxt[camera_id].actual_prev_size;
    video_size =
        &handle->prev_cxt[camera_id].prev_param.video_size;
    act_video_size =
        &handle->prev_cxt[camera_id].actual_video_size;
    channel0_size =
        &handle->prev_cxt[camera_id].prev_param.channel0_size;
    channel1_size =
        &handle->prev_cxt[camera_id].prev_param.channel1_size;
    channel1_act_pic_size =
        &handle->prev_cxt[camera_id].channel1_actual_pic_size;
    channel2_size =
        &handle->prev_cxt[camera_id].prev_param.channel2_size;
    channel2_act_pic_size =
        &handle->prev_cxt[camera_id].channel2_actual_pic_size;
    channel3_size =
        &handle->prev_cxt[camera_id].prev_param.channel3_size;
    channel3_act_pic_size =
        &handle->prev_cxt[camera_id].channel3_actual_pic_size;
    channel4_size =
        &handle->prev_cxt[camera_id].prev_param.channel4_size;
    channel4_act_pic_size =
        &handle->prev_cxt[camera_id].channel4_actual_pic_size;
    org_pic_size =
        &handle->prev_cxt[camera_id].prev_param.picture_size;
    alg_pic_size =
        &handle->prev_cxt[camera_id].aligned_pic_size;
    act_pic_size =
        &handle->prev_cxt[camera_id].actual_pic_size;
    prev_rot =
        handle->prev_cxt[camera_id].prev_param.prev_rot;
    cap_rot =
        handle->prev_cxt[camera_id].prev_param.cap_rot;
    cfg_cap_rot =
        handle->prev_cxt[camera_id].prev_param.encode_angle;
    is_cfg_rot_cap =
        handle->prev_cxt[camera_id].prev_param.is_cfg_rot_cap;
    sensor_info =
        &handle->prev_cxt[camera_id].sensor_info;

    CMR_LOGD("preview_eb %d, video_eb %d, snapshot_eb %d, sprd_zsl_enabled %d",
             handle->prev_cxt[camera_id].prev_param.preview_eb,
             handle->prev_cxt[camera_id].prev_param.video_eb,
             handle->prev_cxt[camera_id].prev_param.snapshot_eb,
             handle->prev_cxt[camera_id].prev_param.sprd_zsl_enabled);

    CMR_LOGD("camera_id %d, prev size %d %d, cap size %d %d",
        camera_id, prev_size->width, prev_size->height,
        org_pic_size->width, org_pic_size->height);

    CMR_LOGD("prev_rot %d, cap_rot %d, is_cfg_rot_cap %d, cfg_cap_rot %d",
        prev_rot, cap_rot, is_cfg_rot_cap, cfg_cap_rot);

    aligned_type = CAMERA_MEM_NO_ALIGNED;

    /* w/h aligned by 16 */
    alg_pic_size->width =
        camera_get_aligned_size(aligned_type, org_pic_size->width);
    alg_pic_size->height =
        camera_get_aligned_size(aligned_type, org_pic_size->height);

    /*consider preview and capture rotation*/
    if (IMG_ANGLE_90 == prev_rot || IMG_ANGLE_270 == prev_rot) {
        act_prev_size->width = prev_size->height;
        act_prev_size->height = prev_size->width;
        act_video_size->width = video_size->height;
        act_video_size->height = video_size->width;
        act_pic_size->width = alg_pic_size->height;
        act_pic_size->height = alg_pic_size->width;
        channel1_act_pic_size->width = channel1_size->height;
        channel1_act_pic_size->height = channel1_size->width;
        channel2_act_pic_size->width = channel2_size->height;
        channel2_act_pic_size->height = channel2_size->width;
        channel3_act_pic_size->width = channel3_size->height;
        channel3_act_pic_size->height = channel3_size->width;
        channel4_act_pic_size->width = channel4_size->height;
        channel4_act_pic_size->height = channel4_size->width;
    } else {
        act_prev_size->width = prev_size->width;
        act_prev_size->height = prev_size->height;
        act_video_size->width = video_size->width;
        act_video_size->height = video_size->height;
        act_pic_size->width = alg_pic_size->width;
        act_pic_size->height = alg_pic_size->height;
        channel1_act_pic_size->width = channel1_size->width;
        channel1_act_pic_size->height = channel1_size->height;
        channel2_act_pic_size->width = channel2_size->width;
        channel2_act_pic_size->height = channel2_size->height;
        channel3_act_pic_size->width = channel3_size->width;
        channel3_act_pic_size->height = channel3_size->height;
        channel4_act_pic_size->width = channel4_size->width;
        channel4_act_pic_size->height = channel4_size->height;
    }

    CMR_LOGD(
        "org_pic_size %d %d, aligned_pic_size %d %d, actual_pic_size %d %d",
        org_pic_size->width, org_pic_size->height, alg_pic_size->width,
        alg_pic_size->height, act_pic_size->width, act_pic_size->height);

    if (!handle->ops.get_sensor_info) {
        CMR_LOGE("ops is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret =
        handle->ops.get_sensor_info(handle->oem_handle, camera_id, sensor_info);
    if (ret) {
        CMR_LOGE("get_sensor info failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    handle->prev_cxt[camera_id].prev_mode = 0;
    handle->prev_cxt[camera_id].video_mode = 0;
    handle->prev_cxt[camera_id].cap_mode = 0;
    handle->prev_cxt[camera_id].channel0_work_mode = 0;
    handle->prev_cxt[camera_id].channel1_work_mode = 0;
    handle->prev_cxt[camera_id].channel2_work_mode = 0;
    handle->prev_cxt[camera_id].channel3_work_mode = 0;
    handle->prev_cxt[camera_id].channel4_work_mode = 0;

    /* set is_4in1_sensor: after get_sensor_info
    * 1: is_4in1_support || limit_w > 0
    */
    do {
        struct camera_context *cxt = handle->oem_handle;
        cxt->is_4in1_sensor = camera_get_is_4in1_sensor(&(cxt->sn_cxt.info_4in1));
    } while(0);

    /*get sensor preview work mode*/
    if (handle->prev_cxt[camera_id].prev_param.preview_eb) {
        ret = prev_get_sn_preview_mode(
            handle, camera_id, sensor_info, act_prev_size,
            &handle->prev_cxt[camera_id].prev_mode);
        if (ret) {
            CMR_LOGE("get preview mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor video work mode*/
    if (handle->prev_cxt[camera_id].prev_param.video_eb) {
        ret = prev_get_sn_preview_mode(
            handle, camera_id, sensor_info, act_video_size,
            &handle->prev_cxt[camera_id].video_mode);
        if (ret) {
            CMR_LOGE("get video mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor channel0 work mode*/
    if (handle->prev_cxt[camera_id].prev_param.channel0_eb) {
        ret = prev_get_sn_preview_mode(
            handle, camera_id, sensor_info, channel0_size,
            &handle->prev_cxt[camera_id].channel0_work_mode);
        if (ret) {
            CMR_LOGE("get preview mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor channel1 work mode*/
    if (handle->prev_cxt[camera_id].prev_param.channel1_eb) {
        ret = prev_get_sn_preview_mode(
            handle, camera_id, sensor_info, channel1_size,
            &handle->prev_cxt[camera_id].channel1_work_mode);
        if (ret) {
            CMR_LOGE("get preview mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor channel2 work mode*/
    if (handle->prev_cxt[camera_id].prev_param.channel2_eb) {
        ret = prev_get_sn_preview_mode(
            handle, camera_id, sensor_info, channel2_size,
            &handle->prev_cxt[camera_id].channel2_work_mode);
        if (ret) {
            CMR_LOGE("get preview mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor channel3 work mode*/
    if (handle->prev_cxt[camera_id].prev_param.channel3_eb) {
        ret = prev_get_sn_preview_mode(
            handle, camera_id, sensor_info, channel3_size,
            &handle->prev_cxt[camera_id].channel3_work_mode);
        if (ret) {
            CMR_LOGE("get preview mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor channel4 work mode*/
    if (handle->prev_cxt[camera_id].prev_param.channel4_eb) {
        ret = prev_get_sn_preview_mode(
            handle, camera_id, sensor_info, channel4_size,
            &handle->prev_cxt[camera_id].channel4_work_mode);
        if (ret) {
            CMR_LOGE("get preview mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    /*get sensor snapshot work mode*/
    if (handle->prev_cxt[camera_id].prev_param.snapshot_eb) {
        ret = prev_get_sn_capture_mode(
            handle, camera_id, sensor_info, act_pic_size,
            &handle->prev_cxt[camera_id].cap_mode);
        if (ret) {
            CMR_LOGE("get capture mode failed!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                    SETTING_GET_APPMODE, &setting_param);
    if (ret) {
        CMR_LOGE("failed to get app mode %ld", ret);
        goto exit;
    }
    CMR_LOGV("app_mode = %d", setting_param.cmd_type_value);
    /*get sensor video_slowmotion work mode*/
    if (handle->prev_cxt[camera_id].prev_param.video_slowmotion_eb ||
        setting_param.cmd_type_value == CAMERA_MODE_SLOWMOTION) {
        for (sn_mode = SENSOR_MODE_PREVIEW_ONE; sn_mode < SENSOR_MODE_MAX;
             sn_mode++) {
            ret = handle->ops.get_sensor_fps_info(
                handle->oem_handle, camera_id,
                sn_mode, &fps_info);
            if (ret) {
                CMR_LOGE("get_sensor info failed!");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }

            CMR_LOGD("mode=%d, max_fps=%d, min_fps=%d, is_high_fps=%d, "
                     "high_fps_skip_num=%d",
                     fps_info.mode, fps_info.max_fps, fps_info.min_fps,
                     fps_info.is_high_fps, fps_info.high_fps_skip_num);

            CMR_LOGD("trim_height=%d, video_height=%d, prev_height=%d",
                     sensor_info->mode_info[fps_info.mode].trim_height,
                     act_video_size->height, act_prev_size->height);

            /* we want to make sure that high fps setting is bigger than preview
             * and video size */
            if (fps_info.is_high_fps &&
                sensor_info->mode_info[fps_info.mode].trim_height >=
                    act_prev_size->height &&
                sensor_info->mode_info[fps_info.mode].trim_height >=
                    act_video_size->height &&
                sensor_info->mode_info[fps_info.mode].trim_height >=
                    act_pic_size->height) {
                CMR_LOGD("HFPS: sensor mode=%d, prev_channel_deci=%d",
                         fps_info.mode, fps_info.high_fps_skip_num);
                handle->prev_cxt[camera_id].prev_mode = fps_info.mode;
                if (handle->prev_cxt[camera_id].prev_param.video_eb)
                    handle->prev_cxt[camera_id].video_mode = fps_info.mode;
                if (handle->prev_cxt[camera_id].prev_param.snapshot_eb)
                    handle->prev_cxt[camera_id].cap_mode = fps_info.mode;
                handle->prev_cxt[camera_id].prev_channel_deci =
                    fps_info.high_fps_skip_num - 1;
                break;
            }
        }
    }

    valid_max_sn_mode =
        MAX(handle->prev_cxt[camera_id].prev_mode,
        handle->prev_cxt[camera_id].video_mode);
    valid_max_sn_mode =
        MAX(valid_max_sn_mode, handle->prev_cxt[camera_id].channel0_work_mode);
    valid_max_sn_mode =
        MAX(valid_max_sn_mode, handle->prev_cxt[camera_id].channel1_work_mode);
    valid_max_sn_mode =
        MAX(valid_max_sn_mode, handle->prev_cxt[camera_id].channel2_work_mode);
    valid_max_sn_mode =
        MAX(valid_max_sn_mode, handle->prev_cxt[camera_id].channel3_work_mode);
    valid_max_sn_mode =
        MAX(valid_max_sn_mode, handle->prev_cxt[camera_id].channel4_work_mode);
    valid_max_sn_mode =
        MAX(valid_max_sn_mode, handle->prev_cxt[camera_id].cap_mode);

    if (handle->prev_cxt[camera_id].prev_param.preview_eb) {
        handle->prev_cxt[camera_id].prev_mode = valid_max_sn_mode;
    }

    if (handle->prev_cxt[camera_id].prev_param.video_eb) {
        handle->prev_cxt[camera_id].video_mode = valid_max_sn_mode;
    }

    if (handle->prev_cxt[camera_id].prev_param.channel0_eb) {
        handle->prev_cxt[camera_id].channel0_work_mode = valid_max_sn_mode;
    }

    if (handle->prev_cxt[camera_id].prev_param.channel1_eb) {
        handle->prev_cxt[camera_id].channel1_work_mode = valid_max_sn_mode;
    }

    if (handle->prev_cxt[camera_id].prev_param.channel2_eb) {
        handle->prev_cxt[camera_id].channel2_work_mode = valid_max_sn_mode;
    }

    if (handle->prev_cxt[camera_id].prev_param.channel3_eb) {
        handle->prev_cxt[camera_id].channel3_work_mode = valid_max_sn_mode;
    }

    if (handle->prev_cxt[camera_id].prev_param.channel4_eb) {
        handle->prev_cxt[camera_id].channel4_work_mode = valid_max_sn_mode;
    }

    if (handle->prev_cxt[camera_id].prev_param.snapshot_eb) {
        handle->prev_cxt[camera_id].cap_mode = valid_max_sn_mode;

        /*caculate max size for capture*/
        handle->prev_cxt[camera_id].max_size.width = alg_pic_size->width;
        handle->prev_cxt[camera_id].max_size.height = alg_pic_size->height;
        ret = prev_get_cap_max_size(
            handle, camera_id,
            &sensor_info->mode_info[handle->prev_cxt[camera_id].cap_mode],
            &handle->prev_cxt[camera_id].max_size);
    }

    CMR_LOGD("prev_mode %ld, video_mode %ld, channel0_work_mode %ld "
             "chn1_work_mode %ld,chn2_work_mode %ld, chn3_work_mode %ld, "
             "channel4_work_mode %ld, cap_mode %ld",
             handle->prev_cxt[camera_id].prev_mode,
             handle->prev_cxt[camera_id].video_mode,
             handle->prev_cxt[camera_id].channel0_work_mode,
             handle->prev_cxt[camera_id].channel1_work_mode,
             handle->prev_cxt[camera_id].channel2_work_mode,
             handle->prev_cxt[camera_id].channel3_work_mode,
             handle->prev_cxt[camera_id].channel4_work_mode,
             handle->prev_cxt[camera_id].cap_mode);

// for mm dvfs
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
    if (handle->prev_cxt[camera_id].prev_param.channel0_eb ||
        handle->prev_cxt[camera_id].prev_param.channel1_eb ||
        handle->prev_cxt[camera_id].prev_param.channel2_eb ||
        handle->prev_cxt[camera_id].prev_param.channel3_eb ||
        handle->prev_cxt[camera_id].prev_param.channel4_eb) {
        dvfs_param.channel_x_enble = 1;
    } else {
        dvfs_param.channel_x_enble = 0;
    }
    if (cxt->is_multi_mode == MODE_BOKEH ||
        cxt->is_multi_mode == MODE_3D_CAPTURE ||
        cxt->is_multi_mode == MODE_3D_VIDEO ||
        cxt->is_multi_mode == MODE_3D_CALIBRATION ||
                cxt->is_multi_mode == MODE_BOKEH_CALI_GOLDEN ||
        cxt->is_multi_mode == MODE_3D_PREVIEW ||
        cxt->is_multi_mode == MODE_TUNING) {
        dvfs_param.cam_mode = 1;
    } else {
        dvfs_param.cam_mode = 0;
    }
    dvfs_param.lane_num = sensor_info->sn_interface.bus_width;
    sensor_mode_info = &sensor_info->mode_info[valid_max_sn_mode];
    dvfs_param.bps_per_lane = sensor_mode_info->bps_per_lane;
    dvfs_param.sn_max_w = sensor_mode_info->width;
    dvfs_param.sn_max_h = sensor_mode_info->height;
    dvfs_param.is_high_fps = fps_info.is_high_fps;
    cmr_set_mm_dvfs_param(handle->oem_handle, dvfs_param);
    cmr_set_mm_dvfs_policy(handle->oem_handle, DVFS_DCAM_IF, IS_PREVIEW_BEGIN);
    cmr_set_mm_dvfs_policy(handle->oem_handle, DVFS_ISP, IS_PREVIEW_BEGIN);
#endif

    handle->prev_cxt[camera_id].sensor_out_width =
        sensor_info->mode_info[valid_max_sn_mode].width;
    handle->prev_cxt[camera_id].sensor_out_height =
        sensor_info->mode_info[valid_max_sn_mode].height;

exit:
    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_get_sn_preview_mode(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct sensor_exp_info *sensor_info,
                                 struct img_size *target_size,
                                 cmr_uint *work_mode) {
    cmr_int ret = CMR_CAMERA_FAIL;
    cmr_u32 width = 0, height = 0, i = 0, last_one = 0;
    cmr_u32 prv_width = 0, prv_height = 0;
    cmr_u32 search_height;
    cmr_u32 search_width;
    cmr_u32 target_mode = SENSOR_MODE_MAX;
    cmr_int offset1 = 0, offset2 = 0;
    struct sensor_mode_fps_tag fps_info;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 is_raw_capture = 0;
    cmr_u32 is_3D_video = 0;
    cmr_u32 is_3D_caputre = 0;
    cmr_u32 is_3D_preview = 0;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    if (!sensor_info) {
        CMR_LOGE("sn info is null!");
        return CMR_CAMERA_FAIL;
    }

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if ((!strcmp(value, "raw")) && (!cxt->is_4in1_sensor)) {
        is_raw_capture = 1;
    }

    float max_binning_ratio = 0;
    struct sensor_zoom_param_input ZoomInputParam;
    ZoomInputParam.camera_id = cxt->is_multi_camera_id;
    ret = sensorGetZoomParam(&ZoomInputParam);
    ret = CMR_CAMERA_SUCCESS;
    max_binning_ratio = ZoomInputParam.BinningRatio;

    if (cxt->is_multi_mode == MODE_3D_VIDEO) {
        is_3D_video = 1;
    } else if (cxt->is_multi_mode == MODE_3D_CAPTURE) {
        is_3D_caputre = 1;
    } else if (cxt->is_multi_mode == MODE_3D_PREVIEW) {
        is_3D_preview = 1;
    }
    if (1 == is_3D_video || 1 == is_3D_caputre || 1 == is_3D_preview) {
        search_width = sensor_info->source_width_max / 2;
        search_height = sensor_info->source_height_max / 2;
    } else if (cxt->is_4in1_sensor == 1 && cxt->is_high_res_mode == 0) {
        /* 4in1 sensor: not high resolution mode, sensor output
        * max size is binning size, but output can digital zoom
        */
        if (target_size->width <= sensor_info->source_width_max / 2 &&
            target_size->height <= sensor_info->source_height_max / 2) {
            search_width = target_size->width;
            search_height = target_size->height;
        } else { /* less than binning size */
            search_width = sensor_info->source_width_max / 2;
            search_height = sensor_info->source_height_max / 2;
        }
    } else {
        search_width = target_size->width;
        search_height = target_size->height;
    }

    prv_width =
        handle->prev_cxt[camera_id].prev_param.preview_size.width;
    prv_height =
        handle->prev_cxt[camera_id].prev_param.preview_size.height;

    if (is_raw_capture == 1) {
        search_height =
            handle->prev_cxt[camera_id].prev_param.raw_capture_size.height;
        search_width =
            handle->prev_cxt[camera_id].prev_param.raw_capture_size.width;
        CMR_LOGD("search_height = %d", search_height);
        for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
            if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
                height = sensor_info->mode_info[i].trim_height;
                width = sensor_info->mode_info[i].trim_width;
                CMR_LOGD("candidate height = %d, width = %d", height, width);
                if (search_height == height && search_width == width) {
                    target_mode = i;
                    ret = CMR_CAMERA_SUCCESS;
                    break;
                } else {
                    last_one = i;
                }
            }
        }
    } else {
        CMR_LOGD("search_height = %d", search_height);
        for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
            if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
                height = sensor_info->mode_info[i].trim_height;
                width = sensor_info->mode_info[i].trim_width;
                CMR_LOGD("candidate height = %d, width = %d", height, width);
                if (CAM_IMG_FMT_JPEG !=
                    sensor_info->mode_info[i].image_format) {
                    if (search_height <= height && search_width <= width) {
                        if (cxt->is_multi_mode == MODE_MULTI_CAMERA) {
                            if ((float)width / max_binning_ratio >=
                                (float)prv_width / ISP_HW_SUPPORT_MAX_ZOOM_RATIO) {
                                /* dont choose high fps setting for no-slowmotion */
                                ret = handle->ops.get_sensor_fps_info(
                                    handle->oem_handle, camera_id, i, &fps_info);
                                CMR_LOGD("mode=%d, is_high_fps=%d", i,
                                    fps_info.is_high_fps);
                                if (fps_info.is_high_fps) {
                                    CMR_LOGD("dont choose high fps setting");
                                    continue;
                                }
                                target_mode = i;
                                ret = CMR_CAMERA_SUCCESS;
                                break;
                            }
                        } else {
                            /* dont choose high fps setting for no-slowmotion */
                            ret = handle->ops.get_sensor_fps_info(
                                handle->oem_handle, camera_id, i, &fps_info);
                            CMR_LOGD("mode=%d, is_high_fps=%d", i,
                                fps_info.is_high_fps);
                            if (fps_info.is_high_fps) {
                                CMR_LOGD("dont choose high fps setting");
                                continue;
                            }
                            target_mode = i;
                            ret = CMR_CAMERA_SUCCESS;
                            break;
                        }
                    } else {
                        last_one = i;
                    }
                }
            }
        }
    }

    if (i == SENSOR_MODE_MAX) {
        CMR_LOGD("can't find the right mode, %d", i);
        target_mode = last_one;
        ret = CMR_CAMERA_SUCCESS;
    }

    CMR_LOGD("target_mode %d", target_mode);
    *work_mode = target_mode;

    return ret;
}

cmr_int prev_get_sn_capture_mode(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct sensor_exp_info *sensor_info,
                                 struct img_size *target_size,
                                 cmr_uint *work_mode) {
    cmr_int ret = CMR_CAMERA_FAIL;
    cmr_u32 width = 0, height = 0, i = 0;
    cmr_u32 prv_width = 0, prv_height = 0;
    cmr_u32 search_width;
    cmr_u32 search_height;
    cmr_u32 target_mode = SENSOR_MODE_MAX;
    cmr_u32 last_mode = SENSOR_MODE_PREVIEW_ONE;
    struct sensor_mode_fps_tag fps_info;
    char value[PROPERTY_VALUE_MAX];
    cmr_u32 is_raw_capture = 0;
    cmr_u32 is_3D_video = 0;
    cmr_u32 is_3D_caputre = 0;
    cmr_u32 is_3D_preview = 0;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;

    if (!sensor_info) {
        CMR_LOGE("sn info is null!");
        return CMR_CAMERA_FAIL;
    }

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    float max_binning_ratio = 0;
    struct sensor_zoom_param_input ZoomInputParam;
    ZoomInputParam.camera_id = cxt->is_multi_camera_id;
    ret = sensorGetZoomParam(&ZoomInputParam);
    ret = CMR_CAMERA_SUCCESS;
    max_binning_ratio = ZoomInputParam.BinningRatio;

    if (cxt->is_multi_mode == MODE_3D_VIDEO) {
        is_3D_video = 1;
    }
    CMR_LOGD("camera_id = %d, mode = %d, 4in1 = %d",
        camera_id, cxt->is_multi_mode, cxt->is_4in1_sensor);
    if (1 == is_3D_video) {
        search_width = sensor_info->source_width_max / 2;
        search_height = sensor_info->source_height_max / 2;
    } else if (cxt->is_4in1_sensor == 1 && cxt->is_high_res_mode == 0
               && !handle->prev_cxt[camera_id].prev_param.tool_eb) {
        /* 4in1 sensor: not high resolution mode, sensor output
        * max size is binning size, but output can digital zoom
        */
        if (target_size->width <= sensor_info->source_width_max / 2 &&
            target_size->height <= sensor_info->source_height_max / 2) {
            search_width = target_size->width;
            search_height = target_size->height;
        } else { /* less than binning size */
            search_width = sensor_info->source_width_max / 2;
            search_height = sensor_info->source_height_max / 2;
        }
    } else {
        search_width = target_size->width;
        search_height = target_size->height;
    }

    prv_width =
        handle->prev_cxt[camera_id].prev_param.preview_size.width;
    prv_height =
        handle->prev_cxt[camera_id].prev_param.preview_size.height;

    if (is_raw_capture == 1 || handle->prev_cxt[camera_id].prev_param.tool_eb) {
        CMR_LOGD("search_height = %d", search_height);
        for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
            if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
                height = sensor_info->mode_info[i].trim_height;
                width = sensor_info->mode_info[i].trim_width;
                CMR_LOGD("height = %d, width = %d", height, width);
                if (CAM_IMG_FMT_JPEG !=
                    sensor_info->mode_info[i].image_format) {
                    if (search_height == height && search_width == width) {
                        target_mode = i;
                        ret = CMR_CAMERA_SUCCESS;
                        break;
                    } else {
                        last_mode = i;
                    }
                }
            }
        }
    } else {
        CMR_LOGD("search_height = %d, is_high_res_mode = %d, ambient_highlight = %d",
            search_height, cxt->is_high_res_mode, cxt->ambient_highlight);
        if (cxt->is_high_res_mode == 1 && cxt->ambient_highlight == 0){
            search_height = search_height>> 1;
            search_width = search_width >> 1;
            CMR_LOGD("search_height = %d, search_width = %d", search_height, search_width);
            for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
                if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
                    height = sensor_info->mode_info[i].trim_height;
                    width = sensor_info->mode_info[i].trim_width;
                    CMR_LOGD("height = %d, width = %d", height, width);
                    if (CAM_IMG_FMT_JPEG !=
                        sensor_info->mode_info[i].image_format) {
                        if (search_height <= height && search_width <= width) {
                            /* dont choose high fps setting for no-slowmotion */
                            ret = handle->ops.get_sensor_fps_info(
                                handle->oem_handle, camera_id, i, &fps_info);
                            CMR_LOGV("mode=%d, is_high_fps=%d", i,
                                fps_info.is_high_fps);
                            if (fps_info.is_high_fps) {
                                CMR_LOGD("dont choose high fps setting");
                                continue;
                            }
                            target_mode = i;
                            ret = CMR_CAMERA_SUCCESS;
                            break;
                        } else {
                            last_mode = i;
                        }
                    }
               }
           }
        }else {
            for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
                if (SENSOR_MODE_MAX != sensor_info->mode_info[i].mode) {
                    height = sensor_info->mode_info[i].trim_height;
                    width = sensor_info->mode_info[i].trim_width;
                    CMR_LOGD("height = %d, width = %d", height, width);
                    if (CAM_IMG_FMT_JPEG !=
                        sensor_info->mode_info[i].image_format) {
                        if (search_height <= height && search_width <= width) {
                            if (cxt->is_multi_mode == MODE_MULTI_CAMERA) {
                                if ((float)width / max_binning_ratio >=
                                    (float)prv_width / ISP_HW_SUPPORT_MAX_ZOOM_RATIO) {
                                    /* dont choose high fps setting for no-slowmotion */
                                    ret = handle->ops.get_sensor_fps_info(
                                        handle->oem_handle, camera_id, i, &fps_info);
                                    CMR_LOGD("mode=%d, is_high_fps=%d", i,
                                         fps_info.is_high_fps);
                                    if (fps_info.is_high_fps) {
                                    CMR_LOGD("dont choose high fps setting");
                                    continue;
                                    }
                                    target_mode = i;
                                    ret = CMR_CAMERA_SUCCESS;
                                    break;
                                }
                            } else {
                                /* dont choose high fps setting for no-slowmotion */
                                ret = handle->ops.get_sensor_fps_info(
                                    handle->oem_handle, camera_id, i, &fps_info);
                                CMR_LOGV("mode=%d, is_high_fps=%d", i,
                                    fps_info.is_high_fps);
                                if (fps_info.is_high_fps) {
                                    CMR_LOGD("dont choose high fps setting");
                                    continue;
                                }
                                target_mode = i;
                                ret = CMR_CAMERA_SUCCESS;
                                break;
                            }
                        } else {
                            last_mode = i;
                        }
                    }
                }
            }
        }
    }

    if (i == SENSOR_MODE_MAX) {
        CMR_LOGD("can't find the right mode, use last available mode %d",
                 last_mode);
        i = last_mode;
        target_mode = last_mode;
        ret = CMR_CAMERA_SUCCESS;
    }

    CMR_LOGD("mode %d, width %d height %d", target_mode,
             sensor_info->mode_info[i].trim_width,
             sensor_info->mode_info[i].trim_height);

    *work_mode = target_mode;

    return ret;
}

cmr_int prev_get_sn_inf(struct prev_handle *handle, cmr_u32 camera_id,
                        cmr_u32 frm_deci, struct sensor_if *sn_if) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    sensor_info = &handle->prev_cxt[camera_id].sensor_info;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info->image_format) {
        sn_if->img_fmt = GRAB_SENSOR_FORMAT_RAWRGB;
        CMR_LOGD("this is RAW sensor");
    } else {
        sn_if->img_fmt = GRAB_SENSOR_FORMAT_YUV;
    }

    if (SENSOR_INTERFACE_TYPE_CSI2 == sensor_info->sn_interface.type) {
        sn_if->if_type = 1;
        sn_if->if_spec.mipi.is_cphy = sensor_info->sn_interface.is_cphy;
        sn_if->if_spec.mipi.lane_num = sensor_info->sn_interface.bus_width;
        sn_if->if_spec.mipi.bits_per_pxl =
            sensor_info->sn_interface.pixel_width;
        sn_if->if_spec.mipi.is_loose = sensor_info->sn_interface.is_loose;
        sn_if->if_spec.mipi.use_href = 0;
        CMR_LOGD("lane_num %d, bits_per_pxl %d, is_loose %d, is_cphy",
                 sn_if->if_spec.mipi.lane_num, sn_if->if_spec.mipi.bits_per_pxl,
                 sn_if->if_spec.mipi.is_loose, sn_if->if_spec.mipi.is_cphy);
    } else {
        sn_if->if_type = 0;
        sn_if->if_spec.ccir.v_sync_pol = sensor_info->vsync_polarity;
        sn_if->if_spec.ccir.h_sync_pol = sensor_info->hsync_polarity;
        sn_if->if_spec.ccir.pclk_pol = sensor_info->pclk_polarity;
    }

    sn_if->img_ptn = sensor_info->image_pattern;
    sn_if->frm_deci = frm_deci;

exit:
    return ret;
}

cmr_int prev_get_cap_max_size(struct prev_handle *handle, cmr_u32 camera_id,
                              struct sensor_mode_info *sn_mode,
                              struct img_size *max_size) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 zoom_proc_mode = ZOOM_BY_CAP;
    cmr_u32 original_fmt = CAM_IMG_FMT_YUV420_NV21;
    cmr_u32 need_isp = 0;
    struct img_rect img_rc;
    struct img_size img_sz;
    struct img_size trim_sz;
    cmr_u32 tmp_width;
    cmr_u32 isp_width_limit = 0;
    cmr_u32 sc_factor = 0, sc_capability = 0, sc_threshold = 0;
    struct cmr_zoom_param *zoom_param = NULL;
    struct img_size *cap_size = NULL;
    cmr_int zoom_post_proc = 0;

    img_sz.width = max_size->width;
    img_sz.height = max_size->height;
    CMR_LOGD("camera_id %d", camera_id);
    isp_width_limit = handle->prev_cxt[camera_id].prev_param.isp_width_limit;
    zoom_param = &handle->prev_cxt[camera_id].prev_param.zoom_setting;
    cap_size = &handle->prev_cxt[camera_id].actual_pic_size;

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    if (CAM_IMG_FMT_YUV422P == sn_mode->image_format) {
        original_fmt = CAM_IMG_FMT_YUV420_NV21;
        zoom_proc_mode = zoom_post_proc;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW == sn_mode->image_format) {
        if (sn_mode->trim_width <= isp_width_limit) {
            CMR_LOGD("need ISP");
            need_isp = 1;
            original_fmt = CAM_IMG_FMT_YUV420_NV21;
            zoom_proc_mode = zoom_post_proc;
        } else {
            CMR_LOGD("Need to process raw data");
            need_isp = 0;
            original_fmt = CAM_IMG_FMT_BAYER_MIPI_RAW;
            zoom_proc_mode = ZOOM_POST_PROCESS;
        }
    } else if (CAM_IMG_FMT_JPEG == sn_mode->image_format) {
        original_fmt = CAM_IMG_FMT_JPEG;
        zoom_proc_mode = ZOOM_POST_PROCESS;
    } else {
        CMR_LOGE("Unsupported sensor format %d for capture",
                 sn_mode->image_format);
        ret = -CMR_CAMERA_INVALID_FORMAT;
        goto exit;
    }

    img_rc.start_x = sn_mode->trim_start_x;
    img_rc.start_y = sn_mode->trim_start_y;
    img_rc.width = sn_mode->trim_width;
    img_rc.height = sn_mode->trim_height;

    trim_sz.width = sn_mode->trim_width;
    trim_sz.height = sn_mode->trim_height;
    CMR_LOGD("rect %d %d %d %d", img_rc.start_x, img_rc.start_y, img_rc.width,
             img_rc.height);
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&img_rc, zoom_param->zoom_level, &trim_sz);
        if (ret) {
            CMR_LOGE("Failed to get trimming window for %ld zoom level ",
                     zoom_param->zoom_level);
            goto exit;
        }
    } else {
        float aspect_ratio = 1.0 * cap_size->width / cap_size->height;
        ret = camera_get_trim_rect2(
            &img_rc, zoom_param->zoom_info.zoom_ratio, aspect_ratio,
            sn_mode->trim_width, sn_mode->trim_height,
            handle->prev_cxt[camera_id].prev_param.cap_rot);
        if (ret) {
            CMR_LOGE("Failed to get trimming window");
            goto exit;
        }
    }
    CMR_LOGD("after rect %d %d %d %d", img_rc.start_x, img_rc.start_y,
             img_rc.width, img_rc.height);

    if (ZOOM_POST_PROCESS == zoom_proc_mode) {
        if (zoom_post_proc) {
            if ((max_size->width < sn_mode->trim_width) ||
                (max_size->height < sn_mode->trim_height)) {
                max_size->width = sn_mode->trim_width;
                max_size->height = sn_mode->trim_height;
            }
        } else {
            if (max_size->width < sn_mode->trim_width) {
                max_size->width = sn_mode->trim_width;
                max_size->height = sn_mode->trim_height;
            }
        }
    } else {
        if (handle->ops.channel_scale_capability) {
            ret = handle->ops.channel_scale_capability(
                handle->oem_handle, &sc_capability, &sc_factor, &sc_threshold);
            if (ret) {
                CMR_LOGE("ops return %ld", ret);
                goto exit;
            }
        } else {
            CMR_LOGE("ops is null");
            goto exit;
        }

        tmp_width = (cmr_u32)(sc_factor * img_rc.width);
        if ((img_rc.width >= cap_size->width ||
             cap_size->width <= sc_threshold) &&
            ZOOM_BY_CAP == zoom_proc_mode) {
            /*if the out size is smaller than the in size, try to use scaler on
             * the fly*/
            if (cap_size->width > tmp_width) {
                if (tmp_width > sc_capability) {
                    img_sz.width = sc_capability;
                } else {
                    img_sz.width = tmp_width;
                }
                img_sz.height = (cmr_u32)(img_rc.height * sc_factor);
            } else {
                /*just use scaler on the fly*/
                img_sz.width = cap_size->width;
                img_sz.height = cap_size->height;
            }
        } else {
            /*if the out size is larger than the in size*/
            img_sz.width = img_rc.width;
            img_sz.height = img_rc.height;
        }

        if (!(max_size->width == img_sz.height &&
              max_size->height == img_sz.width)) {
            max_size->width = MAX(max_size->width, img_sz.width);
            max_size->height = MAX(max_size->height, img_sz.height);
        }
    }

    CMR_LOGD("max_size width %d, height %d", max_size->width, max_size->height);

exit:
    return ret;
}

cmr_int prev_get_frm_index(struct img_frm *frame, struct frm_info *data) {
    cmr_int i;

    for (i = 0; i < PREV_FRM_CNT; i++) {
        if (data->fd == (frame + i)->fd) {
            break;
        }
    }
    CMR_LOGV("frm id %ld", i);

    return i;
}

int get_frame_index(struct img_frm *frame, int total_num, struct frm_info *data,
                    int *index) {
    int ret = -1;
    int i;

    for (i = 0; i < total_num; i++) {
        if (data->fd == (frame + i)->fd) {
            ret = 0;
            *index = i;
            break;
        }
    }
    CMR_LOGV("frm id %d", i);

    return ret;
}

cmr_int prev_zsl_get_frm_index(struct img_frm *frame, struct frm_info *data) {
    cmr_int i;

    for (i = 0; i < ZSL_FRM_CNT; i++) {
        if (data->fd == (frame + i)->fd) {
            break;
        }
    }
    CMR_LOGV("frm id %ld", i);

    return i;
}

#ifdef CONFIG_Y_IMG_TO_ISP
cmr_int prev_y_info_copy_to_isp(struct prev_handle *handle, cmr_uint camera_id,
                                struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint index = 0xff;
    struct prev_context *prev_cxt = NULL;
    struct yimg_info y_info = {0};
    struct isp_yimg_info isp_yimg = {0};

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.get_isp_yimg || !handle->ops.set_preview_yimg) {
        CMR_LOGE("ops null");
        goto exit;
    }
    /* get isp user buffer status */
    ret = handle->ops.get_isp_yimg(handle->oem_handle, camera_id, &isp_yimg);
    if (ret) {
        CMR_LOGE("get isp yimg error");
        goto exit;
    }

    for (i = 0; i < 2; i++) {
        if (0 == isp_yimg.lock[i]) {
            index = i;
            break;
        }
    }
    if (0xff == index) {
        CMR_LOGE("two buffer all in lock");
        goto exit;
    }
    CMR_LOGD("index %ld", index);
    /* set buffer to isp */
    cmr_bzero(prev_cxt->prev_virt_y_addr_array[index],
              prev_cxt->prev_mem_y_size);
    y_info.camera_id = camera_id;
    y_info.y_size = prev_cxt->prev_mem_y_size;
    y_info.sec = info->sec;
    y_info.usec = info->usec;
    memcpy(prev_cxt->prev_virt_y_addr_array[index], info->yaddr_vir,
           prev_cxt->prev_mem_y_size);
    y_info.ready[index] = 1;
    for (i = 0; i < 2; i++)
        y_info.y_addr[i] = prev_cxt->prev_virt_y_addr_array[i];
    ret = handle->ops.set_preview_yimg(handle->oem_handle, camera_id, &y_info);
    if (ret)
        CMR_LOGE("set_preview_yimg err %d", ret);

exit:
    return ret;
}
#endif

static cmr_int prev_yuv_info_copy_to_isp(struct prev_handle *handle,
                                         cmr_uint camera_id,
                                         struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_uint i = 0;
    cmr_uint index = 0xff;
    struct prev_context *prev_cxt = NULL;
    struct yuv_info_t yuv_info;
    cmr_u8 *info_yaddr = NULL;
    cmr_u8 *info_uaddr = NULL;
    cmr_u8 *uv_addr = NULL;
    cmr_uint uv_size = 0;

    prev_cxt = &handle->prev_cxt[camera_id];

    if (0 != prev_cxt->prev_frm_cnt % 30)
        goto exit;

    cmr_bzero(&yuv_info, sizeof(struct yuv_info_t));
    /* set buffer to isp */
    yuv_info.camera_id = camera_id;
    yuv_info.yuv_addr = (cmr_u8 *)prev_cxt->prev_virt_yuv_addr;
    yuv_info.width = prev_cxt->actual_prev_size.width;
    yuv_info.height = prev_cxt->actual_prev_size.height;
    info_yaddr = (cmr_u8 *)((cmr_uint)info->yaddr_vir);
    info_uaddr = (cmr_u8 *)((cmr_uint)info->uaddr_vir);
    memcpy(yuv_info.yuv_addr, info_yaddr, yuv_info.width * yuv_info.height);
    uv_addr = yuv_info.yuv_addr + yuv_info.width * yuv_info.height;
    uv_size = yuv_info.width * yuv_info.height / 2;
    memcpy(uv_addr, info_uaddr, uv_size);

    ret = handle->ops.set_preview_yuv(handle->oem_handle, camera_id, &yuv_info);
    if (ret)
        CMR_LOGE("set_preview_yimg err %ld", ret);

exit:
    return ret;
}

cmr_uint g_preview_frame_dump_cnt = 0;
cmr_int prev_construct_frame(struct prev_handle *handle, cmr_u32 camera_id,
                             struct frm_info *info,
                             struct camera_frame_type *frame_type) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 is_ultra_wide = 0;
    cmr_u32 prev_rot = 0;
    cmr_u32 video_frm_id = 0;
    struct prev_context *prev_cxt = NULL;
    struct img_frm *frm_ptr = NULL;
    struct img_frm *video_frm_ptr = NULL;
    cmr_s64 ae_time = 0;

    if (!handle || !frame_type || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", handle, frame_type, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    prev_chn_id = handle->prev_cxt[camera_id].prev_channel_id;
    cap_chn_id = handle->prev_cxt[camera_id].cap_channel_id;
    prev_rot = handle->prev_cxt[camera_id].prev_param.prev_rot;
    is_ultra_wide = handle->prev_cxt[camera_id].prev_param.is_ultra_wide;
    prev_cxt = &handle->prev_cxt[camera_id];
    ae_time = prev_cxt->ae_time;
    if (prev_chn_id == info->channel_id) {
        if (prev_rot || is_ultra_wide) {
            /*prev_num = prev_cxt->prev_mem_num - PREV_ROT_FRM_CNT;
            frm_id   = prev_cxt->prev_rot_index % PREV_ROT_FRM_CNT;
            frm_ptr  = &prev_cxt->prev_rot_frm[frm_id];

            frame_type->buf_id       = frm_id;
            frame_type->order_buf_id = frm_id + prev_num;
            frame_type->y_vir_addr   =
            prev_cxt->prev_rot_frm[frm_id].addr_vir.addr_y;
            frame_type->y_phy_addr   =
            prev_cxt->prev_rot_frm[frm_id].addr_phy.addr_y;

            CMR_LOGE("[prev_rot] lock %d", frm_id);
            prev_cxt->prev_rot_frm_is_lock[frm_id] = 1;*/

            info->fd = prev_cxt->prev_frm[0].fd;
            info->yaddr = prev_cxt->prev_frm[0].addr_phy.addr_y;
            info->uaddr = prev_cxt->prev_frm[0].addr_phy.addr_u;
            info->vaddr = prev_cxt->prev_frm[0].addr_phy.addr_v;
            info->yaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_y;
            info->uaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_u;
            info->vaddr_vir = prev_cxt->prev_frm[0].addr_vir.addr_v;
        }
        // frm_id = info->frame_id - CMR_PREV_ID_BASE;
        frm_id = prev_get_frm_index(prev_cxt->prev_frm, info);
        frm_ptr = &prev_cxt->prev_frm[frm_id];

        frame_type->buf_id = frm_id;
        frame_type->order_buf_id = frm_id;
        frame_type->y_vir_addr = prev_cxt->prev_frm[frm_id].addr_vir.addr_y;
        frame_type->y_phy_addr = prev_cxt->prev_frm[frm_id].addr_phy.addr_y;
        frame_type->fd = prev_cxt->prev_frm[frm_id].fd;

        frame_type->width = prev_cxt->prev_param.preview_size.width;
        frame_type->height = prev_cxt->prev_param.preview_size.height;
        frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type->monoboottime = info->monoboottime;
        frame_type->zoom_ratio =
            prev_cxt->prev_param.zoom_setting.zoom_info.zoom_ratio;
        frame_type->ae_time = ae_time;
        frame_type->vcm_step = (cmr_u32)prev_cxt->vcm_step;
        frame_type->type = PREVIEW_FRAME;
        CMR_LOGV("ae_time: %" PRId64
                 ", zoom_ratio: %f, vcm_step: %d, timestamp: %" PRId64,
                 frame_type->ae_time, frame_type->zoom_ratio,
                 frame_type->vcm_step, frame_type->timestamp);

        if (prev_cxt->prev_param.is_support_fd && prev_cxt->prev_param.is_fd_on &&
            ((cxt->ref_camera_id == camera_id && cxt->is_fov_fusion != 1) ||
            cxt->is_fov_fusion ==1 || cxt->is_multi_mode != MODE_MULTI_CAMERA)) {
                /*after image processed, timestamp and zoom ratio will be needed*/
                frm_ptr->reserved = (void*)info;
                prev_fd_send_data(handle, camera_id, frm_ptr);
        }
        if (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW ||
            prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW)
        {
            struct frm_info data;
            if (((void *)(frm_ptr->addr_vir.addr_y) != NULL) &&
                (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW||
                prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW)) {
                video_frm_id = frm_id;
                video_frm_ptr = &prev_cxt->video_frm[frm_id];
            }
            ret = prev_3dnr_send_data(handle, camera_id, info,
                    frame_type, frm_ptr, video_frm_ptr);
        }

        if (cxt->ipm_cxt.ai_scene_inited && cxt->ai_scene_enable == 1 && cxt->ref_camera_id == camera_id) {
            prev_ai_scene_send_data(handle, camera_id, frm_ptr, info);
        }

        // auto tracking
        if (prev_cxt->auto_tracking_inited) {
            prev_auto_tracking_send_data(handle, camera_id, frm_ptr, info);
        }

        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.preview.dump.count", value, "null");
        cmr_uint dump_num = atoi(value);
        if (strcmp(value, "null")) {
            if (g_preview_frame_dump_cnt < dump_num) {
                char tag_name[30]={0};
                char cameraId[2] ={0};
                strcpy(tag_name, "prev_construct_frame_id");
                sprintf(cameraId, "%d", camera_id);
                strcat(tag_name, cameraId);
                dump_image(tag_name, CAM_IMG_FMT_YUV420_NV21,
                           frame_type->width, frame_type->height,
                           prev_cxt->prev_frm_cnt,
                           &prev_cxt->prev_frm[frm_id].addr_vir,
                           frame_type->width * frame_type->height * 3 / 2);
                g_preview_frame_dump_cnt++;
            }
        }

#ifdef CONFIG_Y_IMG_TO_ISP
        prev_y_info_copy_to_isp(handle, camera_id, info);
#endif
#ifdef YUV_TO_ISP
        prev_yuv_info_copy_to_isp(handle, camera_id, info);
#endif
    } else {
        CMR_LOGE("ignored, channel id %d, frame id %d", info->channel_id,
                 info->frame_id);
    }

    ATRACE_END();
    return ret;
}

cmr_uint g_video_frame_dump_cnt = 0;
cmr_int prev_construct_video_frame(struct prev_handle *handle,
                                   cmr_u32 camera_id, struct frm_info *info,
                                   struct camera_frame_type *frame_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 video_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 is_ultra_wide = 0;
    cmr_u32 prev_rot = 0;
    struct prev_context *prev_cxt = NULL;
    struct img_frm *frm_ptr = NULL;
    cmr_s64 ae_time = 0;

    if (!handle || !frame_type || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", handle, frame_type, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    video_chn_id = handle->prev_cxt[camera_id].video_channel_id;
    cap_chn_id = handle->prev_cxt[camera_id].cap_channel_id;
    prev_rot = handle->prev_cxt[camera_id].prev_param.prev_rot;
    is_ultra_wide = handle->prev_cxt[camera_id].prev_param.is_ultra_wide;
    prev_cxt = &handle->prev_cxt[camera_id];
    ae_time = prev_cxt->ae_time;

    if (video_chn_id == info->channel_id) {
        if (prev_rot || is_ultra_wide) {
            /*prev_num = prev_cxt->video_mem_num - PREV_ROT_FRM_CNT;
            frm_id   = prev_cxt->video_rot_index % PREV_ROT_FRM_CNT;
            frm_ptr  = &prev_cxt->video_rot_frm[frm_id];

            frame_type->buf_id       = frm_id;
            frame_type->order_buf_id = frm_id + prev_num;
            frame_type->y_vir_addr   =
            prev_cxt->video_rot_frm[frm_id].addr_vir.addr_y;
            frame_type->y_phy_addr   =
            prev_cxt->video_rot_frm[frm_id].addr_phy.addr_y;

            CMR_LOGE("[prev_rot] lock %d", frm_id);
            prev_cxt->video_rot_frm_is_lock[frm_id] = 1;*/

            info->fd = prev_cxt->video_frm[0].fd;
            info->yaddr = prev_cxt->video_frm[0].addr_phy.addr_y;
            info->uaddr = prev_cxt->video_frm[0].addr_phy.addr_u;
            info->vaddr = prev_cxt->video_frm[0].addr_phy.addr_v;
            info->yaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_y;
            info->uaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_u;
            info->vaddr_vir = prev_cxt->video_frm[0].addr_vir.addr_v;
        }
        // frm_id = info->frame_id - CMR_PREV_ID_BASE;
        frm_id = prev_get_frm_index(prev_cxt->video_frm, info);
        frm_ptr = &prev_cxt->video_frm[frm_id];

        frame_type->buf_id = frm_id;
        frame_type->order_buf_id = frm_id;
        frame_type->y_vir_addr = prev_cxt->video_frm[frm_id].addr_vir.addr_y;
        frame_type->uv_vir_addr = prev_cxt->video_frm[frm_id].addr_vir.addr_u;
        frame_type->fd = prev_cxt->video_frm[frm_id].fd;
        frame_type->y_phy_addr = prev_cxt->video_frm[frm_id].addr_phy.addr_y;
        frame_type->uv_phy_addr = prev_cxt->video_frm[frm_id].addr_phy.addr_u;
        frame_type->width = prev_cxt->prev_param.video_size.width;
        frame_type->height = prev_cxt->prev_param.video_size.height;
        frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type->monoboottime = info->monoboottime;
        frame_type->zoom_ratio =
            prev_cxt->prev_param.zoom_setting.zoom_info.zoom_ratio;
        frame_type->ae_time = ae_time;
        CMR_LOGV("ae_time: %" PRId64 ", zoom_ratio: %f", frame_type->ae_time,
                 frame_type->zoom_ratio);
        frame_type->type = PREVIEW_VIDEO_FRAME;

        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.video.dump.count", value, "null");
        cmr_uint dump_num = atoi(value);
        if (strcmp(value, "null")) {
            if (g_video_frame_dump_cnt < dump_num) {
                char tag_name[30]={0};
                char cameraId[2] ={0};
                strcpy(tag_name, "prev_construct_video_frame");
                sprintf(cameraId, "%d", camera_id);
                strcat(tag_name, cameraId);
                dump_image(tag_name,
                           CAM_IMG_FMT_YUV420_NV21, frame_type->width,
                           frame_type->height, prev_cxt->prev_frm_cnt,
                           &prev_cxt->video_frm[frm_id].addr_vir,
                           frame_type->width * frame_type->height * 3 / 2);
                g_video_frame_dump_cnt++;
            }
        }
    } else {
        CMR_LOGE("ignored, channel id %d, frame id %d", info->channel_id,
                 info->frame_id);
    }

    return ret;
}

cmr_uint g_zsl_frame_dump_cnt = 0;
cmr_int prev_construct_zsl_frame(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *info,
                                 struct camera_frame_type *frame_type) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0;
    struct prev_context *prev_cxt = NULL;
    struct img_frm *frm_ptr = NULL;
    cmr_int zoom_post_proc = 0;
    cmr_u32 is_ultra_wide = 0;

    if (!handle || !frame_type || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p, 0x%p", handle, frame_type, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    prev_chn_id = handle->prev_cxt[camera_id].prev_channel_id;
    cap_chn_id = handle->prev_cxt[camera_id].cap_channel_id;
    prev_rot = handle->prev_cxt[camera_id].prev_param.cap_rot;
    is_ultra_wide = handle->prev_cxt[camera_id].prev_param.is_ultra_wide;
    prev_cxt = &handle->prev_cxt[camera_id];
    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    if (cap_chn_id == info->channel_id) {
        if (prev_rot || is_ultra_wide) {
            info->fd = prev_cxt->cap_zsl_frm[0].fd;
            info->yaddr = prev_cxt->cap_zsl_frm[0].addr_phy.addr_y;
            info->uaddr = prev_cxt->cap_zsl_frm[0].addr_phy.addr_u;
            info->vaddr = prev_cxt->cap_zsl_frm[0].addr_phy.addr_v;
            info->yaddr_vir = prev_cxt->cap_zsl_frm[0].addr_vir.addr_y;
            info->uaddr_vir = prev_cxt->cap_zsl_frm[0].addr_vir.addr_u;
            info->vaddr_vir = prev_cxt->cap_zsl_frm[0].addr_vir.addr_v;
        }
        frm_id = prev_zsl_get_frm_index(prev_cxt->cap_zsl_frm, info);
        frm_ptr = &prev_cxt->cap_zsl_frm[frm_id];

        frame_type->buf_id = frm_id;
        frame_type->order_buf_id = frm_id;
        frame_type->y_vir_addr = prev_cxt->cap_zsl_frm[frm_id].addr_vir.addr_y;
        frame_type->uv_vir_addr = prev_cxt->cap_zsl_frm[frm_id].addr_vir.addr_u;
        frame_type->fd = prev_cxt->cap_zsl_frm[frm_id].fd;
        frame_type->y_phy_addr = prev_cxt->cap_zsl_frm[frm_id].addr_phy.addr_y;
        frame_type->uv_phy_addr = prev_cxt->cap_zsl_frm[frm_id].addr_phy.addr_u;
        frame_type->width = prev_cxt->cap_zsl_frm[frm_id].size.width;
        frame_type->height = prev_cxt->cap_zsl_frm[frm_id].size.height;
        frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type->monoboottime = info->monoboottime;
        frame_type->format = info->fmt;
        frame_type->type = PREVIEW_ZSL_FRAME;
        CMR_LOGV("timestamp=%" PRId64 ", width=%d, height=%d, fd=0x%x",
                 frame_type->timestamp, frame_type->width, frame_type->height,
                 frame_type->fd);

        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.zsl.dump.count", value, "null");
        cmr_uint dump_num = atoi(value);
        if (strcmp(value, "null")) {
            if (g_zsl_frame_dump_cnt < dump_num) {
                char tag_name[30]={0};
                char cameraId[2] ={0};
                strcpy(tag_name, "prev_construct_zsl_frame");
                sprintf(cameraId, "%d", camera_id);
                strcat(tag_name, cameraId);
                dump_image(tag_name, CAM_IMG_FMT_YUV420_NV21,
                           frame_type->width, frame_type->height,
                           prev_cxt->prev_frm_cnt,
                           &prev_cxt->cap_zsl_frm[frm_id].addr_vir,
                           frame_type->width * frame_type->height * 3 / 2);
                g_zsl_frame_dump_cnt++;
            }
        }

    } else {
        CMR_LOGE("ignored, channel id %d, frame id %d", info->channel_id,
                 info->frame_id);
    }

    return ret;
}

cmr_int prev_set_param_internal(struct prev_handle *handle, cmr_u32 camera_id,
                                cmr_u32 is_restart,
                                struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 is_raw_capture = 0;
    char value[PROPERTY_VALUE_MAX];
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!out_param_ptr) {
        CMR_LOGD("out_param_ptr is null");
    }

    /*cmr_bzero(out_param_ptr, sizeof(struct preview_out_param));*/

    handle->prev_cxt[camera_id].camera_id = camera_id;
    handle->prev_cxt[camera_id].out_ret_val = CMR_CAMERA_SUCCESS;

    CMR_LOGD("camera_id %ld, preview_eb %d, snapshot_eb %d, video_eb %d, "
             "tool_eb %d, prev_status %ld",
             handle->prev_cxt[camera_id].camera_id,
             handle->prev_cxt[camera_id].prev_param.preview_eb,
             handle->prev_cxt[camera_id].prev_param.snapshot_eb,
             handle->prev_cxt[camera_id].prev_param.video_eb,
             handle->prev_cxt[camera_id].prev_param.tool_eb,
             handle->prev_cxt[camera_id].prev_status);

    CMR_LOGD("preview_size %d %d, picture_size %d %d, video_size %d %d",
             handle->prev_cxt[camera_id].prev_param.preview_size.width,
             handle->prev_cxt[camera_id].prev_param.preview_size.height,
             handle->prev_cxt[camera_id].prev_param.picture_size.width,
             handle->prev_cxt[camera_id].prev_param.picture_size.height,
             handle->prev_cxt[camera_id].prev_param.video_size.width,
             handle->prev_cxt[camera_id].prev_param.video_size.height);

    ret = prev_get_sensor_mode(handle, camera_id);
    if (ret) {
        CMR_LOGE("get sensor mode failed");
        goto exit;
    }

    ret = prev_pre_set(handle, camera_id);
    if (ret) {
        CMR_LOGE("pre set failed");
        goto exit;
    }

    do { /* check 4in1 type */
        struct prev_context *prev_cxt = &(handle->prev_cxt[camera_id]);
        prev_cxt->prev_param.remosaic_type =
            camera_get_remosaic_type(&(cxt->sn_cxt.info_4in1),
            prev_cxt->sensor_out_width, prev_cxt->sensor_out_height);
        cxt->remosaic_type = prev_cxt->prev_param.remosaic_type;
    } while (0);

    if (handle->prev_cxt[camera_id].prev_param.preview_eb) {
        ret =prev_set_prev_param(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("set prev param failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.video_eb) {
        ret = prev_set_video_param(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("set video param failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.channel0_eb) {
        ret = channel0_configure(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("channel0_configure failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.channel1_eb) {
        ret = channel1_configure(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("channel1_configure failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.channel2_eb) {
        ret = channel2_configure(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("channel2_configure failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.channel3_eb) {
        ret = channel3_configure(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("channel3_configure failed");
            goto exit;
        }
    }

    if (handle->prev_cxt[camera_id].prev_param.channel4_eb) {
        ret = channel4_configure(handle, camera_id, is_restart, out_param_ptr);
        if (ret) {
            CMR_LOGE("channel4_configure failed");
            goto exit;
        }
    }

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

#if defined(CONFIG_ISP_2_3)
    if (handle->prev_cxt[camera_id].prev_param.snapshot_eb) {
        if (handle->prev_cxt[camera_id].prev_param.tool_eb) {
#else
    if (handle->prev_cxt[camera_id].prev_param.snapshot_eb) {
        if ((handle->prev_cxt[camera_id].prev_param.tool_eb &&
             is_raw_capture == 1) ||
            isp_video_get_simulation_flag()) {
#endif
            ret = prev_set_cap_param_raw(handle, camera_id, is_restart,
                                         out_param_ptr);
        } else {
            ret = prev_set_cap_param(handle, camera_id, is_restart, 0,
                                     out_param_ptr);
        }
        if (ret) {
            CMR_LOGE("set cap param failed");
            goto exit;
        }
    }

exit:
    CMR_LOGV("X");
    handle->prev_cxt[camera_id].out_ret_val = ret;
    ATRACE_END();
    return ret;
}

cmr_int prev_set_prev_param(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart,
                            struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));
    CMR_LOGD("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];

    chn_param.sensor_mode = prev_cxt->prev_mode;
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->prev_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    cmr_bzero(prev_cxt->prev_ultra_wide_frm_is_lock,
              PREV_ULTRA_WIDE_ALLOC_CNT * sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->prev_ultra_wide_index = 0;
    prev_cxt->prev_frm_cnt = 0;
    prev_cxt->prev_preflash_skip_en = 0;
    prev_cxt->prev_skip_num = sensor_info->preview_skip_num;
    prev_cxt->skip_mode = IMG_SKIP_SW_KER;

    prev_cxt->prev_zoom = true;
    prev_cxt->cap_zoom = true;
#ifdef CONFIG_CAMERA_SUPPORT_ULTRA_WIDE
    if (camera_id == sensorGetPhyId4Role(SENSOR_ROLE_MULTICAM_SUPERWIDE,
                                         SNS_FACE_BACK) && cxt->is_ultra_wide) {
        prev_cxt->prev_zoom = sprd_warp_adapter_get_isISPZoom(WARP_PREVIEW);
        if (!(prev_cxt->prev_zoom))
            zoom_param->zoom_info.prev_aspect_ratio = 1.0f;
        prev_cxt->cap_zoom = sprd_warp_adapter_get_isISPZoom(WARP_CAPTURE);
        if (!(prev_cxt->cap_zoom))
            zoom_param->zoom_info.capture_aspect_ratio = 1.0f;
        CMR_LOGV("ID=%d,pre %d,cap %d", camera_id, prev_cxt->prev_zoom,
                 prev_cxt->cap_zoom);
    }
#endif
    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;

    /* in slowmotion, we want do decimation in preview channel,
    bug video buffer and preview buffer is in one request, so we cant
    do decimation for now, otherwise the fps is low */

    chn_param.cap_inf_cfg.chn_deci_factor = 0;

#ifdef SPRD_SLOWMOTION_OPTIMIZE
    if (prev_cxt->prev_param.video_eb &&
        prev_cxt->prev_param.video_slowmotion_eb) {
        chn_param.cap_inf_cfg.chn_deci_factor = 3;
        prev_cxt->prev_skip_num = 2;
    }
#endif

    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    if (prev_cxt->prev_param.video_slowmotion_eb) {
        chn_param.cap_inf_cfg.cfg.slowmotion = 1;
    }

    if (sensor_mode_info->image_format == CAM_IMG_FMT_BAYER_MIPI_RAW &&
        chn_param.cap_inf_cfg.cfg.dst_img_fmt != CAM_IMG_FMT_BAYER_MIPI_RAW) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_prev_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_prev_size.height;

    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_PREVIEW;

    CMR_LOGD("skip_mode %ld, skip_num %ld, image_format %d",
             prev_cxt->skip_mode, prev_cxt->prev_skip_num,
             sensor_mode_info->image_format);

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    prev_cxt->video_size.width = sensor_mode_info->scaler_trim.width;
    prev_cxt->video_size.height = sensor_mode_info->scaler_trim.height;

    if (handle->ops.isp_ioctl) {
        struct common_isp_cmd_param param;

        param.camera_id = camera_id;
        param.size_param.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
        param.size_param.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;
        ret = handle->ops.isp_ioctl(handle->oem_handle,
                COM_ISP_SET_SENSOR_SIZE, &param);
        if (ret < 0) {
            CMR_LOGW("fail to set sensor size");
        }
    }

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_info->image_format)
    {
        /*caculate trim rect*/
        if (ZOOM_INFO != zoom_param->mode) {
            CMR_LOGD("zoom level %ld, dst_img_size %d %d", zoom_param->zoom_level,
                     chn_param.cap_inf_cfg.cfg.dst_img_size.width,
                     chn_param.cap_inf_cfg.cfg.dst_img_size.height);
            ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                       zoom_param->zoom_level,
                                       &chn_param.cap_inf_cfg.cfg.dst_img_size);
        } else {
            float real_ratio = zoom_param->zoom_info.prev_aspect_ratio;
            float aspect_ratio = 1.0 * prev_cxt->actual_prev_size.width /
                                 prev_cxt->actual_prev_size.height;

            if (zoom_param->zoom_info.crop_region.width > 0 && PLATFORM_ID == 0x0401 ) {
                chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                        zoom_param->zoom_info.pixel_size, zoom_param->zoom_info.crop_region,
                        chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
            } else {
                ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                        real_ratio, aspect_ratio,
                        sensor_mode_info->scaler_trim.width,
                        sensor_mode_info->scaler_trim.height,
                        prev_cxt->prev_param.prev_rot);
            }
        }
        if (ret) {
            CMR_LOGE("prev get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    CMR_LOGD("camera %u after src_img_rect %d %d %d %d", camera_id,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    if (handle->ops.isp_ioctl) {
        struct common_isp_cmd_param param;

        param.camera_id = camera_id;
        param.ae_target_region = chn_param.cap_inf_cfg.cfg.src_img_rect;
        ret = handle->ops.isp_ioctl(handle->oem_handle,
                COM_ISP_SET_AE_TARGET_REGION, &param);
        if (ret < 0) {
            CMR_LOGW("fail to set AE roi");
        }
    }

    /*save the rect*/
    prev_cxt->prev_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->prev_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->prev_rect.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->prev_rect.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc preview buffer*/
    ret = prev_alloc_prev_buf(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    chn_param.cap_inf_cfg.cfg.flip_on = 0;

    if (check_software_remosaic(prev_cxt))
        chn_param.cap_inf_cfg.cfg.need_4in1 = 1;

    /*config channel*/
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->prev_channel_id = channel_id;
    CMR_LOGD("prev chn id is %ld", prev_cxt->prev_channel_id);
    prev_cxt->prev_channel_status = PREV_CHN_BUSY;
    prev_cxt->prev_data_endian = endian;
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;
    if (prev_cxt->skip_mode == IMG_SKIP_SW_KER) {
        /*config skip num buffer*/
        for (i = 0; i < prev_cxt->prev_skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->prev_channel_id;
            buf_cfg.base_id = CMR_PREV_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->prev_mem_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->prev_reserved_frm.addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->prev_reserved_frm.addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->prev_reserved_frm.addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->prev_reserved_frm.addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->prev_reserved_frm.fd;
            buf_cfg.frame_number = 0xFFFFFFFF;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->prev_channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->prev_channel_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->prev_mem_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->prev_reserved_frm.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->prev_reserved_frm.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y = prev_cxt->prev_reserved_frm.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u = prev_cxt->prev_reserved_frm.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->prev_reserved_frm.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

// TBD: move this to channel_cfg
#ifdef CONFIG_CAMERA_OFFLINE
    struct sprd_dcam_path_size dcam_cfg;
    dcam_cfg.dcam_in_w = sensor_mode_info->trim_width;
    dcam_cfg.dcam_in_h = sensor_mode_info->trim_height;
    dcam_cfg.pre_dst_w = prev_cxt->actual_prev_size.width;
    dcam_cfg.pre_dst_h = prev_cxt->actual_prev_size.height;
    dcam_cfg.vid_dst_w = prev_cxt->actual_video_size.width;
    dcam_cfg.vid_dst_h = prev_cxt->actual_video_size.height;
    ret = handle->ops.channel_dcam_size(handle->oem_handle, &dcam_cfg);
    if (ret) {
        CMR_LOGE("get dcam output size failed.");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->dcam_output_size.width = dcam_cfg.dcam_out_w;
    prev_cxt->dcam_output_size.height = dcam_cfg.dcam_out_h;
#endif

    /*return preview out params*/
    if (out_param_ptr) {
        out_param_ptr->preview_chn_id = prev_cxt->prev_channel_id;
        out_param_ptr->preview_sn_mode = chn_param.sensor_mode;
        out_param_ptr->preview_data_endian = prev_cxt->prev_data_endian;
    }

exit:
    if (ret) {
        prev_free_prev_buf(handle, camera_id, 0);
    }

    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_set_prev_param_lightly(struct prev_handle *handle,
                                    cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("E");

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    prev_cxt = &handle->prev_cxt[camera_id];

    chn_param.sensor_mode = prev_cxt->prev_mode;
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    // cmr_bzero(prev_cxt->prev_rot_frm_is_lock, PREV_ROT_FRM_CNT *
    // sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->skip_mode = IMG_SKIP_SW_KER;
    prev_cxt->prev_ultra_wide_index = 0;

    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_prev_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_prev_size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    prev_cxt->video_size.width = sensor_mode_info->scaler_trim.width;
    prev_cxt->video_size.height = sensor_mode_info->scaler_trim.height;

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
    } else {
        float aspect_ratio = 1.0 * prev_cxt->actual_prev_size.width /
                    prev_cxt->actual_prev_size.height;
        CMR_LOGD("preview aspect_ratio %f", aspect_ratio);
        if (zoom_param->zoom_info.crop_region.width > 0 && PLATFORM_ID == 0x0401) {
            chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                    zoom_param->zoom_info.pixel_size, zoom_param->zoom_info.crop_region,
                    chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
        } else {
            ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                    zoom_param->zoom_info.prev_aspect_ratio,
                    aspect_ratio,
                    sensor_mode_info->scaler_trim.width,
                    sensor_mode_info->scaler_trim.height,
                    prev_cxt->prev_param.prev_rot);
        }
    }

    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("camera %u after src_img_rect %d %d %d %d", camera_id,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    if (handle->ops.isp_ioctl) {
        struct common_isp_cmd_param param;

        param.camera_id = camera_id;
        param.ae_target_region = chn_param.cap_inf_cfg.cfg.src_img_rect;
        ret = handle->ops.isp_ioctl(handle->oem_handle,
                COM_ISP_SET_AE_TARGET_REGION, &param);
        if (ret < 0) {
            CMR_LOGW("fail to set AE roi");
        }
    }

    /*save the rect*/
    prev_cxt->prev_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->prev_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->prev_rect.width =
        chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->prev_rect.height =
        chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    channel_id = prev_cxt->prev_channel_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("returned chn id is %d", channel_id);

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_set_video_param(struct prev_handle *handle, cmr_u32 camera_id,
                             cmr_u32 is_restart,
                             struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct camera_context *cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;
    cmr_u32 i;
    cxt = (struct camera_context *)handle->oem_handle;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));
    CMR_LOGD("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];

    chn_param.sensor_mode = prev_cxt->video_mode;
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->video_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    cmr_bzero(prev_cxt->video_ultra_wide_frm_is_lock,
              VIDEO_ULTRA_WIDE_ALLOC_CNT * sizeof(cmr_uint));
    prev_cxt->video_rot_index = 0;
    prev_cxt->video_ultra_wide_index = 0;
    prev_cxt->video_frm_cnt = 0;
    prev_cxt->prev_skip_num = sensor_info->preview_skip_num;
    prev_cxt->skip_mode = IMG_SKIP_SW_KER;
    prev_cxt->video_zoom = true;

#ifdef CONFIG_CAMERA_SUPPORT_ULTRA_WIDE
    if (camera_id == sensorGetPhyId4Role(SENSOR_ROLE_MULTICAM_SUPERWIDE,
                                         SNS_FACE_BACK) && cxt->is_ultra_wide) {
        prev_cxt->video_zoom = sprd_warp_adapter_get_isISPZoom(WARP_PREVIEW);
        if (!(prev_cxt->video_zoom))
            zoom_param->zoom_info.video_aspect_ratio = 1.0f;
        CMR_LOGV("ID=%d,video %d", camera_id, prev_cxt->video_zoom);
    }
#endif

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 1;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_RECORDING;
    chn_param.cap_inf_cfg.cfg.afbc_enable =
        prev_cxt->prev_param.sprd_afbc_enabled;
    if (prev_cxt->prev_param.video_slowmotion_eb &&
        prev_cxt->prev_param.video_eb) {
        chn_param.cap_inf_cfg.cfg.slowmotion = 1;
#ifdef FMCU_SUPPORT
        chn_param.cap_inf_cfg.cfg.slowmotion = 2;
#endif
    }

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_video_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_video_size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

#ifdef SPRD_SLOWMOTION_OPTIMIZE
    if (prev_cxt->prev_param.video_slowmotion_eb &&
        prev_cxt->prev_param.video_eb)
        prev_cxt->prev_skip_num = 2 * prev_cxt->prev_param.video_slowmotion_eb;
#endif

    CMR_LOGD("skip_mode %ld, skip_num %ld, image_format %d w=%d h=%d",
             prev_cxt->skip_mode, prev_cxt->prev_skip_num,
             sensor_mode_info->image_format, prev_cxt->actual_video_size.width,
             prev_cxt->actual_video_size.height);

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        CMR_LOGD("zoom level %ld, dst_img_size %d %d", zoom_param->zoom_level,
                 chn_param.cap_inf_cfg.cfg.dst_img_size.width,
                 chn_param.cap_inf_cfg.cfg.dst_img_size.height);
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
    } else {
        float real_ratio = zoom_param->zoom_info.video_aspect_ratio;
        float aspect_ratio = 1.0 * prev_cxt->actual_video_size.width /
                             prev_cxt->actual_video_size.height;

        // TODO WORKAROUND
        if (camera_id == sensorGetPhyId4Role(SENSOR_ROLE_MULTICAM_SUPERWIDE, SNS_FACE_BACK))
            real_ratio = 1.0f;

        if (zoom_param->zoom_info.crop_region.width > 0) {
            chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                    zoom_param->zoom_info.pixel_size, zoom_param->zoom_info.crop_region,
                    chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
        } else {
            ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                    real_ratio, aspect_ratio,
                    sensor_mode_info->scaler_trim.width,
                    sensor_mode_info->scaler_trim.height,
                    prev_cxt->prev_param.prev_rot);
        }
    }
    if (ret) {
        CMR_LOGE("video get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*save the rect*/
    prev_cxt->video_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->video_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->video_rect.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->video_rect.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc preview buffer*/
    ret =
        prev_alloc_video_buf(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if ((handle->prev_cxt[camera_id].prev_param.flip_on) && (1 == camera_id)) {
        chn_param.cap_inf_cfg.cfg.flip_on =
            handle->prev_cxt[camera_id].prev_param.flip_on;
    } else {
        chn_param.cap_inf_cfg.cfg.flip_on = 0;
    }
    CMR_LOGD("channel config flip:%d", chn_param.cap_inf_cfg.cfg.flip_on);

    // config 4in1 flag
    if (check_software_remosaic(prev_cxt)) {
        CMR_LOGD("set 4in1 mode to 1");
        chn_param.cap_inf_cfg.cfg.need_4in1 = 1;
    }

    /*config channel*/
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->video_channel_id = channel_id;
    CMR_LOGD("video chn id is %ld", prev_cxt->video_channel_id);
    prev_cxt->video_channel_status = PREV_CHN_BUSY;
    prev_cxt->video_data_endian = endian;
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    if (prev_cxt->skip_mode == IMG_SKIP_SW_KER) {
        /*config skip buffer*/
        for (i = 0; i < prev_cxt->prev_skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->video_channel_id;
            buf_cfg.base_id = CMR_VIDEO_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->video_mem_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->video_reserved_frm.addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->video_reserved_frm.addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->video_reserved_frm.addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->video_reserved_frm.addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->video_reserved_frm.fd;
            buf_cfg.frame_number = 0xFFFFFFFF;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->video_channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->video_channel_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->video_mem_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->video_reserved_frm.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->video_reserved_frm.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y = prev_cxt->video_reserved_frm.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u = prev_cxt->video_reserved_frm.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->video_reserved_frm.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*start isp*/
    CMR_LOGD("need_isp %d, isp_status %ld", chn_param.cap_inf_cfg.cfg.need_isp,
             prev_cxt->isp_status);

    /*return preview out params*/
    if (out_param_ptr) {
        out_param_ptr->video_chn_id = prev_cxt->video_channel_id;
        out_param_ptr->video_sn_mode = chn_param.sensor_mode;
        out_param_ptr->video_data_endian = prev_cxt->video_data_endian;
        out_param_ptr->snapshot_data_endian = prev_cxt->video_data_endian;
        out_param_ptr->actual_video_size = prev_cxt->actual_video_size;
    }

exit:
    CMR_LOGD("ret %ld", ret);
    if (ret) {
        prev_free_video_buf(handle, camera_id, 0);
    }

    ATRACE_END();
    return ret;
}

cmr_int prev_set_video_param_lightly(struct prev_handle *handle,
                                     cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct img_size trim_sz;
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("E");

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    prev_cxt = &handle->prev_cxt[camera_id];

    chn_param.sensor_mode = prev_cxt->video_mode;
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->video_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->skip_mode = IMG_SKIP_SW_KER;
    prev_cxt->video_ultra_wide_index = 0;

    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 1;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->actual_video_size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->actual_video_size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
    } else {
        float aspect_ratio = 1.0 * prev_cxt->actual_video_size.width /
                             prev_cxt->actual_video_size.height;
        if (zoom_param->zoom_info.crop_region.width > 0) {
            chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                    zoom_param->zoom_info.pixel_size, zoom_param->zoom_info.crop_region,
                    chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
        } else {
            ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                    zoom_param->zoom_info.video_aspect_ratio,
                    aspect_ratio,
                    sensor_mode_info->scaler_trim.width,
                    sensor_mode_info->scaler_trim.height,
                    prev_cxt->prev_param.prev_rot);
        }
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*save the rect*/
    prev_cxt->video_rect.start_x =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
    prev_cxt->video_rect.start_y =
        chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
    prev_cxt->video_rect.width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;
    prev_cxt->video_rect.height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    channel_id = prev_cxt->video_channel_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("returned chn id is %d", channel_id);

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int channel0_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 reserved_count = 1;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->channel0.size.width;
    height = prev_cxt->channel0.size.height;

    CMR_LOGD("channel0_fmt=%d, w=%d, h=%d", prev_cxt->prev_param.channel0_fmt,
             width, height);
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->prev_param.channel0_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->prev_param.channel0_fmt) {
        prev_cxt->channel0.buf_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->prev_param.channel0_fmt) {
        prev_cxt->channel0.buf_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
               prev_cxt->prev_param.channel0_fmt) {
        prev_cxt->channel0.buf_size = width * height * 2;
    } else {
        CMR_LOGE("unsupprot fmt %d", prev_cxt->prev_param.channel0_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt->channel0.buf_cnt = 0;
    prev_cxt->channel0.valid_buf_cnt = 0;
    if (!is_restart) {
        mem_ops->alloc_mem(CAMERA_CHANNEL_0_RESERVED, handle->oem_handle,
                           &prev_cxt->channel0.buf_size, &reserved_count,
                           &prev_cxt->channel0.frm_reserved.addr_phy.addr_y,
                           &prev_cxt->channel0.frm_reserved.addr_vir.addr_y,
                           &prev_cxt->channel0.frm_reserved.fd);
        if (prev_cxt->channel0.frm_reserved.fd <= 0 ||
            prev_cxt->channel0.frm_reserved.addr_vir.addr_y == 0) {
            CMR_LOGE("alloc failed, fd=%d, addr_vir_y=%ld",
                     prev_cxt->channel0.frm_reserved.fd,
                     prev_cxt->channel0.frm_reserved.addr_vir.addr_y);
            return -1;
        }
    }

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->channel0.valid_buf_cnt;
    buffer->length = prev_cxt->channel0.buf_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->channel0.valid_buf_cnt; i++) {
        prev_cxt->channel0.frm[i].fmt = prev_cxt->prev_param.channel0_fmt;
        prev_cxt->channel0.frm[i].buf_size = prev_cxt->channel0.buf_size;
        prev_cxt->channel0.frm[i].size.width = prev_cxt->channel0.size.width;
        prev_cxt->channel0.frm[i].size.height = prev_cxt->channel0.size.height;
        prev_cxt->channel0.frm[i].addr_vir.addr_y =
            prev_cxt->channel0.addr_vir[i];
        prev_cxt->channel0.frm[i].addr_phy.addr_y =
            prev_cxt->channel0.addr_phy[i];
        prev_cxt->channel0.frm[i].fd = prev_cxt->channel0.fd[i];

        buffer->addr[i].addr_y = prev_cxt->channel0.frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->channel0.frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->channel0.frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->channel0.frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->channel0.frm[i].fd;
    }

    prev_cxt->channel0.frm_reserved.fmt = prev_cxt->prev_param.channel0_fmt;
    prev_cxt->channel0.frm_reserved.buf_size = prev_cxt->channel0.buf_size;
    prev_cxt->channel0.frm_reserved.size.width = width;
    prev_cxt->channel0.frm_reserved.size.height = height;

    CMR_LOGD("reserved_buf: fd=%d, vir_addr_y=%lx",
             prev_cxt->channel0.frm_reserved.fd,
             prev_cxt->channel0.frm_reserved.addr_vir.addr_y);

    ATRACE_END();
    return ret;
}

cmr_int channel0_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        cmr_bzero(prev_cxt->channel0.addr_phy,
                  CHANNEL0_BUF_CNT * sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel0.addr_vir,
                  CHANNEL0_BUF_CNT * sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel0.fd, CHANNEL0_BUF_CNT * sizeof(cmr_s32));

        mem_ops->free_mem(
            CAMERA_CHANNEL_0_RESERVED, handle->oem_handle,
            (cmr_uint *)prev_cxt->channel0.frm_reserved.addr_phy.addr_y,
            (cmr_uint *)prev_cxt->channel0.frm_reserved.addr_vir.addr_y,
            &prev_cxt->channel0.frm_reserved.fd, 1);

        prev_cxt->channel0.frm_reserved.addr_phy.addr_y = 0;
        prev_cxt->channel0.frm_reserved.addr_vir.addr_y = 0;
        prev_cxt->channel0.frm_reserved.fd = 0;
    }

    return ret;
}

cmr_int channel0_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    int32_t ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&endian, sizeof(struct img_data_end));
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    cmr_bzero(&prev_cxt->channel0, sizeof(channel0_t));

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel0_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    prev_cxt->channel0.size.width = prev_cxt->prev_param.channel0_size.width;
    prev_cxt->channel0.size.height = prev_cxt->prev_param.channel0_size.height;
    prev_cxt->channel0.frm_cnt = 0;
    prev_cxt->channel0.skip_num = 0; // TBD
    prev_cxt->channel0.skip_mode = IMG_SKIP_SW_KER;

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    chn_param.sensor_mode = prev_cxt->channel0_work_mode;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    chn_param.cap_inf_cfg.cfg.need_isp_tool = 1;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel0_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_size.width = sensor_mode_info->trim_width;
    chn_param.cap_inf_cfg.cfg.src_img_size.height =
        sensor_mode_info->trim_height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x = 0;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y = 0;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width = sensor_mode_info->trim_width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->trim_height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        sensor_mode_info->trim_height;
    chn_param.cap_inf_cfg.cfg.dst_img_size.width = sensor_mode_info->trim_width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        sensor_mode_info->trim_height;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_PREVIEW;

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    ret = channel0_alloc_bufs(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }

    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    prev_cxt->channel0.chn_id = channel_id;
    prev_cxt->channel0.chn_status = PREV_CHN_BUSY;
    prev_cxt->channel0.endian = endian;
    if (prev_cxt->prev_param.preview_eb == 0 &&
        prev_cxt->prev_param.video_eb == 0 &&
        prev_cxt->prev_param.channel1_eb == 0 &&
        prev_cxt->prev_param.channel2_eb == 0 &&
        prev_cxt->prev_param.channel3_eb == 0 &&
        prev_cxt->prev_param.channel4_eb == 0 &&
        prev_cxt->prev_param.snapshot_eb == 0) {
        prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
        prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;
    }

    /* skip frame in dcam driver */
    if (prev_cxt->channel0.skip_mode == IMG_SKIP_SW_KER) {
        for (i = 0; i < prev_cxt->channel0.skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->channel0.chn_id;
            buf_cfg.base_id = CMR_VIDEO_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->channel0.frm_reserved.buf_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->channel0.frm_reserved.addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->channel0.frm_reserved.addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->channel0.frm_reserved.addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->channel0.frm_reserved.addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->channel0.frm_reserved.fd;
            buf_cfg.frame_number = 0xFFFFFFFF;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->channel0.chn_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->channel0.chn_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel0.frm_reserved.buf_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->channel0.frm_reserved.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->channel0.frm_reserved.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->channel0.frm_reserved.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->channel0.frm_reserved.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->channel0.frm_reserved.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (out_param_ptr) {
        out_param_ptr->channel0_chn_id = prev_cxt->channel0.chn_id;
        out_param_ptr->channel0_sn_mode = prev_cxt->channel0_work_mode;
        out_param_ptr->channel0_endian = prev_cxt->channel0.endian;
        out_param_ptr->channel0_size = prev_cxt->channel0.size;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_s32 channel0_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_num = 0;
    cmr_u32 width, height;
    struct buffer_cfg buf_cfg;
    cmr_u32 rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->channel0.valid_buf_cnt;

    if (valid_num >= CHANNEL0_BUF_CNT) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    width = prev_cxt->channel0.size.width;
    height = prev_cxt->channel0.size.height;

    prev_cxt->channel0.fd[valid_num] = buffer->fd;
    prev_cxt->channel0.addr_phy[valid_num] = (unsigned long)buffer->addr_phy;
    prev_cxt->channel0.addr_vir[valid_num] = (unsigned long)buffer->addr_vir;

    prev_cxt->channel0.frm[valid_num].fmt = prev_cxt->prev_param.channel0_fmt;
    prev_cxt->channel0.frm[valid_num].buf_size = prev_cxt->channel0.buf_size;
    prev_cxt->channel0.frm[valid_num].size.width = width;
    prev_cxt->channel0.frm[valid_num].size.height = height;
    prev_cxt->channel0.frm[valid_num].fd = prev_cxt->channel0.fd[valid_num];
    prev_cxt->channel0.frm[valid_num].addr_phy.addr_y =
        prev_cxt->channel0.addr_phy[valid_num];
    prev_cxt->channel0.frm[valid_num].addr_vir.addr_y =
        prev_cxt->channel0.addr_vir[valid_num];

    prev_cxt->channel0.valid_buf_cnt++;

    buf_cfg.channel_id = prev_cxt->channel0.chn_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE; // no use, will remove it
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel0.buf_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;
    buf_cfg.frame_number = buffer->frame_number;

    buf_cfg.addr[0].addr_y = prev_cxt->channel0.frm[valid_num].addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->channel0.frm[valid_num].addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->channel0.frm[valid_num].addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->channel0.frm[valid_num].addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->channel0.frm[valid_num].fd;

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d",
             prev_cxt->camera_id, prev_cxt->channel0.frm[valid_num].fd,
             prev_cxt->channel0.chn_id, prev_cxt->channel0.valid_buf_cnt);
    ATRACE_END();
    return ret;
}

int channel0_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int frm_index = 0;
    cmr_u32 channel0_chn_id = 0;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_buf_cnt;
    struct prev_cb_info cb_data_info;
    struct camera_frame_type frame_type;
    int i;

    if (!handle || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p", handle, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    channel0_chn_id = prev_cxt->channel0.chn_id;

    if (info->channel_id != channel0_chn_id) {
        CMR_LOGE("ignored, channel id %d", info->channel_id);
        goto exit;
    }

    if (prev_cxt->channel0.valid_buf_cnt <= 0) {
        ret = -1;
        CMR_LOGE("chn2 valid_buf_cnt=%d", prev_cxt->channel0.valid_buf_cnt);
        goto exit;
    }

    if (prev_cxt->channel0.frm[0].fd != (cmr_s32)info->fd) {
        ret = -1;
        CMR_LOGE("frame sequence error from kernel driver: info->fd=0x%x, "
                 "prev_cxt->channel0.frm[0].fd=0x%x",
                 info->fd, prev_cxt->channel0.frm[0].fd);
        goto exit;
    }

    frame_type.buf_id = 0;
    frame_type.order_buf_id = 0;
    frame_type.y_vir_addr = prev_cxt->channel0.frm[0].addr_vir.addr_y;
    frame_type.uv_vir_addr = prev_cxt->channel0.frm[0].addr_vir.addr_u;
    frame_type.fd = prev_cxt->channel0.frm[0].fd;
    frame_type.y_phy_addr = prev_cxt->channel0.frm[0].addr_phy.addr_y;
    frame_type.uv_phy_addr = prev_cxt->channel0.frm[0].addr_phy.addr_u;
    frame_type.width = prev_cxt->channel0.size.width;
    frame_type.height = prev_cxt->channel0.size.height;
    frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
    frame_type.monoboottime = info->monoboottime;
    frame_type.type = CHANNEL0_FRAME;

    valid_buf_cnt = prev_cxt->channel0.valid_buf_cnt;
    for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
        prev_cxt->channel0.fd[i] = prev_cxt->channel0.fd[i + 1];
        prev_cxt->channel0.addr_phy[i] = prev_cxt->channel0.addr_phy[i + i];
        prev_cxt->channel0.addr_vir[i] = prev_cxt->channel0.addr_vir[i + 1];
        prev_cxt->channel0.frm[i] = prev_cxt->channel0.frm[i + 1];
    }

    prev_cxt->channel0.fd[valid_buf_cnt - 1] = 0;
    prev_cxt->channel0.addr_phy[valid_buf_cnt - 1] = 0;
    prev_cxt->channel0.addr_vir[valid_buf_cnt - 1] = 0;
    cmr_bzero(&prev_cxt->channel0.frm[valid_buf_cnt - 1],
              sizeof(struct img_frm));

    prev_cxt->channel0.valid_buf_cnt--;

    cb_data_info.cb_type = PREVIEW_EVT_CB_RAW_FRAME;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d, frame_index=%d",
             prev_cxt->camera_id, frame_type.fd, info->channel_id,
             prev_cxt->channel0.valid_buf_cnt, info->frame_real_id);

exit:
    return ret;
}

int cmr_channel0_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)&buffer;

    message.msg_type = PREV_EVT_CHANNEL0_QUEUE_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int channel1_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 rot_frm_start_num = 0;
    cmr_u32 reserved_count = 1;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->channel1.size.width;
    height = prev_cxt->channel1.size.height;

    CMR_LOGD("channel1_fmt=%d, w=%d, h=%d", prev_cxt->prev_param.channel1_fmt,
             width, height);
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->prev_param.channel1_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->prev_param.channel1_fmt) {
        prev_cxt->channel1.buf_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->prev_param.channel1_fmt) {
        prev_cxt->channel1.buf_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->prev_param.channel1_fmt) {
        prev_cxt->channel1.buf_size = (width * height * 3) >> 1;
        prev_cxt->prev_param.channel1_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
               prev_cxt->prev_param.channel1_fmt) {
        prev_cxt->channel1.buf_size = (width * height) << 1;
    } else {
        CMR_LOGE("unsupprot fmt %d", prev_cxt->prev_param.channel1_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

#ifdef CONFIG_CAMERA_HAL_VERSION_1
    prev_cxt->channel1.buf_cnt = CHANNEL1_BUF_CNT;
    if (prev_cxt->prev_param.channel1_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel1.buf_cnt += CHANNEL1_BUF_CNT_ROT;
    }

    if (!is_restart) {
        mem_ops->alloc_mem(
            CAMERA_CHANNEL_1, handle->oem_handle, &prev_cxt->channel1.buf_size,
            &prev_cxt->channel1.buf_cnt, prev_cxt->channel1.addr_phy,
            prev_cxt->channel1.addr_vir, prev_cxt->channel1.fd);

        prev_cxt->channel1.valid_buf_cnt = CHANNEL1_BUF_CNT;
        rot_frm_start_num = CHANNEL1_BUF_CNT;

        mem_ops->alloc_mem(CAMERA_CHANNEL_1_RESERVED, handle->oem_handle,
                           &prev_cxt->channel1.buf_size, &reserved_count,
                           &prev_cxt->channel1.frm_reserved.addr_phy,
                           &prev_cxt->channel1.frm_reserved.addr_vir,
                           &prev_cxt->channel1.frm_reserved.fd);
    }
#else
    prev_cxt->channel1.buf_cnt = 0;
    if (prev_cxt->prev_param.channel1_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel1.buf_cnt += CHANNEL1_BUF_CNT_ROT;
        if (!is_restart) {
            mem_ops->alloc_mem(CAMERA_CHANNEL_1, handle->oem_handle,
                               &prev_cxt->channel1.buf_size,
                               &prev_cxt->channel1.buf_cnt,
                               &prev_cxt->channel1.addr_phy[CHANNEL1_BUF_CNT],
                               &prev_cxt->channel1.addr_vir[CHANNEL1_BUF_CNT],
                               &prev_cxt->channel1.fd[CHANNEL1_BUF_CNT]);
        }
    }

    if (!is_restart) {
        prev_cxt->channel1.valid_buf_cnt = 0;
        rot_frm_start_num = CHANNEL1_BUF_CNT;
        mem_ops->alloc_mem(CAMERA_CHANNEL_1_RESERVED, handle->oem_handle,
                           &prev_cxt->channel1.buf_size, &reserved_count,
                           &prev_cxt->channel1.frm_reserved.addr_phy.addr_y,
                           &prev_cxt->channel1.frm_reserved.addr_vir.addr_y,
                           &prev_cxt->channel1.frm_reserved.fd);
    }
#endif

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->channel1.valid_buf_cnt;
    buffer->length = prev_cxt->channel1.buf_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->channel1.valid_buf_cnt; i++) {
        prev_cxt->channel1.frm[i].fmt = prev_cxt->prev_param.channel1_fmt;
        prev_cxt->channel1.frm[i].buf_size = prev_cxt->channel1.buf_size;
        prev_cxt->channel1.frm[i].size.width = prev_cxt->channel1.size.width;
        prev_cxt->channel1.frm[i].size.height = prev_cxt->channel1.size.height;
        prev_cxt->channel1.frm[i].addr_vir.addr_y =
            prev_cxt->channel1.addr_vir[i];
        prev_cxt->channel1.frm[i].addr_vir.addr_u =
            prev_cxt->channel1.frm[i].addr_vir.addr_y + width * height;
        prev_cxt->channel1.frm[i].addr_phy.addr_y =
            prev_cxt->channel1.addr_phy[i];
        prev_cxt->channel1.frm[i].addr_phy.addr_u =
            prev_cxt->channel1.frm[i].addr_phy.addr_y + width * height;
        prev_cxt->channel1.frm[i].fd = prev_cxt->channel1.fd[i];

        buffer->addr[i].addr_y = prev_cxt->channel1.frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->channel1.frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->channel1.frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->channel1.frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->channel1.frm[i].fd;
    }

    prev_cxt->channel1.frm_reserved.fmt = prev_cxt->prev_param.channel1_fmt;
    prev_cxt->channel1.frm_reserved.buf_size = prev_cxt->channel1.buf_size;
    prev_cxt->channel1.frm_reserved.size.width = width;
    prev_cxt->channel1.frm_reserved.size.height = height;
    //prev_cxt->channel1.frm_reserved.addr_vir.addr_y =
        //prev_cxt->channel1.frm_reserved.addr_vir.addr_y;
    prev_cxt->channel1.frm_reserved.addr_vir.addr_u =
        prev_cxt->channel1.frm_reserved.addr_vir.addr_y + width * height;
    //prev_cxt->channel1.frm_reserved.addr_phy.addr_y =
        //prev_cxt->channel1.frm_reserved.addr_phy.addr_y;
    prev_cxt->channel1.frm_reserved.addr_phy.addr_u =
        prev_cxt->channel1.frm_reserved.addr_phy.addr_y + width * height;
    //prev_cxt->channel1.frm_reserved.fd = prev_cxt->channel1.frm_reserved.fd;

    if (prev_cxt->prev_param.channel1_rot_angle) {
        for (i = 0; i < CHANNEL1_BUF_CNT_ROT; i++) {
            prev_cxt->channel1.rot_frm[i].fmt =
                prev_cxt->prev_param.channel1_fmt;
            prev_cxt->channel1.rot_frm[i].buf_size =
                prev_cxt->channel1.buf_size;
            prev_cxt->channel1.rot_frm[i].size.width =
                prev_cxt->channel1.size.width;
            prev_cxt->channel1.rot_frm[i].size.height =
                prev_cxt->channel1.size.height;
            prev_cxt->channel1.rot_frm[i].addr_vir.addr_y =
                prev_cxt->channel1.addr_vir[rot_frm_start_num + i];
            prev_cxt->channel1.rot_frm[i].addr_vir.addr_u =
                prev_cxt->channel1.rot_frm[i].addr_vir.addr_y + width * height;
            prev_cxt->channel1.rot_frm[i].addr_phy.addr_y =
                prev_cxt->channel1.addr_phy[rot_frm_start_num + i];
            prev_cxt->channel1.rot_frm[i].addr_phy.addr_u =
                prev_cxt->channel1.rot_frm[i].addr_phy.addr_y + width * height;
            prev_cxt->channel1.rot_frm[i].fd =
                prev_cxt->channel1.fd[rot_frm_start_num + i];
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int channel1_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_CHANNEL_1, handle->oem_handle,
                          prev_cxt->channel1.addr_phy,
                          prev_cxt->channel1.addr_vir, prev_cxt->channel1.fd,
                          prev_cxt->channel1.buf_cnt);
        cmr_bzero(prev_cxt->channel1.addr_phy,
                  (CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel1.addr_vir,
                  (CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel1.fd,
                  (CHANNEL1_BUF_CNT + CHANNEL1_BUF_CNT_ROT) * sizeof(cmr_s32));

        mem_ops->free_mem(
            CAMERA_CHANNEL_1_RESERVED, handle->oem_handle,
            (cmr_uint *)prev_cxt->channel1.frm_reserved.addr_phy.addr_y,
            (cmr_uint *)prev_cxt->channel1.frm_reserved.addr_vir.addr_y,
            &prev_cxt->channel1.frm_reserved.fd, 1);

        prev_cxt->channel1.frm_reserved.addr_phy.addr_y = 0;
        prev_cxt->channel1.frm_reserved.addr_vir.addr_y = 0;
        prev_cxt->channel1.frm_reserved.fd = 0;
    }

    return ret;
}

cmr_int channel1_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&endian, sizeof(struct img_data_end));
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    cmr_bzero(&prev_cxt->channel1, sizeof(channel1_t));

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel1_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    prev_cxt->channel1.size.width = prev_cxt->channel1_actual_pic_size.width;
    prev_cxt->channel1.size.height = prev_cxt->channel1_actual_pic_size.height;
    prev_cxt->channel1.frm_cnt = 0;
    prev_cxt->channel1.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel1.skip_mode = IMG_SKIP_SW_KER;

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    chn_param.sensor_mode = prev_cxt->channel1_work_mode;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel1_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.flip_on =
        handle->prev_cxt[camera_id].prev_param.channel1_flip_on;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.sence_mode =
        DCAM_SCENE_MODE_CAPTURE_CALLBACK; // TBD
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    } else {
        chn_param.cap_inf_cfg.cfg.need_isp = 0;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel1.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel1.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel1.size.width /
                             prev_cxt->channel1.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel1_rot_angle);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    ret = channel1_alloc_bufs(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }

    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    prev_cxt->channel1.chn_id = channel_id;
    prev_cxt->channel1.chn_status = PREV_CHN_BUSY;
    prev_cxt->channel1.endian = endian;
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /* skip frame in dcam driver */
    if (prev_cxt->channel1.skip_mode == IMG_SKIP_SW_KER) {
        for (i = 0; i < prev_cxt->channel1.skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->channel1.chn_id;
            buf_cfg.base_id = CMR_VIDEO_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->channel1.frm_reserved.buf_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->channel1.frm_reserved.addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->channel1.frm_reserved.addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->channel1.frm_reserved.addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->channel1.frm_reserved.addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->channel1.frm_reserved.fd;
            buf_cfg.frame_number = 0xFFFFFFFF;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->channel1.chn_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->channel1.chn_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel1.frm_reserved.buf_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->channel1.frm_reserved.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->channel1.frm_reserved.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->channel1.frm_reserved.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->channel1.frm_reserved.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->channel1.frm_reserved.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (out_param_ptr) {
        out_param_ptr->channel1_chn_id = prev_cxt->channel1.chn_id;
        out_param_ptr->channel1_sn_mode = prev_cxt->channel1_work_mode;
        out_param_ptr->channel1_endian = prev_cxt->channel1.endian;
        out_param_ptr->channel1_size = prev_cxt->channel1.size;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int channel1_update_params(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));

    prev_cxt = &handle->prev_cxt[camera_id];

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel1_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    //	cmr_bzero(prev_cxt->prev_rot_frm_is_lock, PREV_ROT_FRM_CNT *
    // sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->channel1.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel1.skip_mode = IMG_SKIP_SW_KER;
    chn_param.sensor_mode = prev_cxt->channel1_work_mode;
    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel1_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format)
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    else
        chn_param.cap_inf_cfg.cfg.need_isp = 0;

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel1.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel1.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel1.size.width /
                             prev_cxt->channel1.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel1_rot_angle);
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    channel_id = prev_cxt->channel1.chn_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    return ret;
}

cmr_s32 channel1_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_num = 0;
    cmr_u32 width, height;
    struct buffer_cfg buf_cfg;
    cmr_u32 rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->channel1.valid_buf_cnt;

    if (valid_num >= CHANNEL1_BUF_CNT) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    width = prev_cxt->channel1.size.width;
    height = prev_cxt->channel1.size.height;

    prev_cxt->channel1.fd[valid_num] = buffer->fd;
    prev_cxt->channel1.addr_phy[valid_num] = (unsigned long)buffer->addr_phy;
    prev_cxt->channel1.addr_vir[valid_num] = (unsigned long)buffer->addr_vir;

    prev_cxt->channel1.frm[valid_num].fmt = prev_cxt->prev_param.channel1_fmt;
    prev_cxt->channel1.frm[valid_num].buf_size = prev_cxt->channel1.buf_size;
    prev_cxt->channel1.frm[valid_num].size.width = width;
    prev_cxt->channel1.frm[valid_num].size.height = height;
    prev_cxt->channel1.frm[valid_num].fd = prev_cxt->channel1.fd[valid_num];
    prev_cxt->channel1.frm[valid_num].addr_phy.addr_y =
        prev_cxt->channel1.addr_phy[valid_num];
    prev_cxt->channel1.frm[valid_num].addr_phy.addr_u =
        prev_cxt->channel1.frm[valid_num].addr_phy.addr_y + width * height;
    prev_cxt->channel1.frm[valid_num].addr_vir.addr_y =
        prev_cxt->channel1.addr_vir[valid_num];
    prev_cxt->channel1.frm[valid_num].addr_vir.addr_u =
        prev_cxt->channel1.frm[valid_num].addr_vir.addr_y + width * height;

    prev_cxt->channel1.valid_buf_cnt++;

    buf_cfg.channel_id = prev_cxt->channel1.chn_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE; // no use, will remove it
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel1.buf_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;
    buf_cfg.frame_number = buffer->frame_number;
    if (prev_cxt->prev_param.channel1_rot_angle) {
        ret = channel1_find_free_rot_buffer(prev_cxt, &rot_index);
        if (ret) {
            CMR_LOGE("prev_search_rot_buffer failed");
            goto exit;
        }

        buf_cfg.fd[0] = prev_cxt->channel1.rot_frm[rot_index].fd;
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel1.rot_frm[rot_index].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel1.rot_frm[rot_index].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel1.rot_frm[rot_index].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel1.rot_frm[rot_index].addr_vir.addr_u;

        prev_cxt->channel1.rot_frm_lock_flag[rot_index] = 1;
    } else {
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel1.frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel1.frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel1.frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel1.frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->channel1.frm[valid_num].fd;
    }

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d",
             prev_cxt->camera_id, prev_cxt->channel1.frm[valid_num].fd,
             prev_cxt->channel1.chn_id, prev_cxt->channel1.valid_buf_cnt);
    ATRACE_END();
    return ret;
}

int channel1_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int frm_index = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0, channel1_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0, channel1_rot_angle = 0;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_buf_cnt;
    struct prev_cb_info cb_data_info;
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;
    int rot_index;
    struct camera_frame_type frame_type;
    int i;

    if (!handle || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p", handle, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    channel1_chn_id = handle->prev_cxt[camera_id].channel1.chn_id;
    channel1_rot_angle =
        handle->prev_cxt[camera_id].prev_param.channel1_rot_angle;
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->channel1.frm_cnt == 0) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->channel1.frm_cnt++;

    if (info->channel_id != channel1_chn_id) {
        CMR_LOGE("ignored, channel id %d", info->channel_id);
        goto exit;
    }

    if (prev_cxt->channel1.valid_buf_cnt <= 0) {
        ret = -1;
        CMR_LOGE("chn2 valid_buf_cnt=%d", prev_cxt->channel1.valid_buf_cnt);
        goto exit;
    }

    // no rotation
    if (channel1_rot_angle == 0) {
        if (prev_cxt->channel1.frm[0].fd != (cmr_s32)info->fd) {
            ret = -1;
            CMR_LOGE("frame sequence error from kernel driver: info->fd=0x%x, "
                     "prev_cxt->channel1.frm[0].fd=0x%x",
                     info->fd, prev_cxt->channel1.frm[0].fd);
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel1.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel1.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel1.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel1.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel1.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel1.size.width;
        frame_type.height = prev_cxt->channel1.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL1_FRAME;

        valid_buf_cnt = prev_cxt->channel1.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel1.fd[i] = prev_cxt->channel1.fd[i + 1];
            prev_cxt->channel1.addr_phy[i] = prev_cxt->channel1.addr_phy[i + i];
            prev_cxt->channel1.addr_vir[i] = prev_cxt->channel1.addr_vir[i + 1];
            prev_cxt->channel1.frm[i] = prev_cxt->channel1.frm[i + 1];
        }

        prev_cxt->channel1.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel1.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel1.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel1.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));
    } else {
        // rotation case
        ret = get_frame_index(prev_cxt->channel1.rot_frm, CHANNEL1_BUF_CNT_ROT,
                              info, &rot_index);
        if (ret) {
            CMR_LOGE("frame is not match");
            goto exit;
        }

        rot_param.src_img = &prev_cxt->channel1.rot_frm[rot_index];
        rot_param.src_img->data_end = prev_cxt->channel1.endian;
        rot_param.dst_img = &prev_cxt->channel1.frm[0];
        rot_param.dst_img->data_end = prev_cxt->channel1.endian;
        rot_param.angle = channel1_rot_angle;
        op_mean.rot = channel1_rot_angle;
        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel1.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel1.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel1.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel1.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel1.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel1.size.width;
        frame_type.height = prev_cxt->channel1.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL1_FRAME;

        valid_buf_cnt = prev_cxt->channel1.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel1.fd[i] = prev_cxt->channel1.fd[i + 1];
            prev_cxt->channel1.addr_phy[i] = prev_cxt->channel1.addr_phy[i + i];
            prev_cxt->channel1.addr_vir[i] = prev_cxt->channel1.addr_vir[i + 1];
            prev_cxt->channel1.frm[i] = prev_cxt->channel1.frm[i + 1];
        }

        prev_cxt->channel1.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel1.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel1.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel1.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));

        prev_cxt->channel1.rot_frm_lock_flag[rot_index] = 0;
    }

    prev_cxt->channel1.valid_buf_cnt--;

    cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d, frame_index=%d",
             prev_cxt->camera_id, frame_type.fd, info->channel_id,
             prev_cxt->channel1.valid_buf_cnt, info->frame_real_id);

exit:
    return ret;
}

cmr_s32 channel1_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index) {
    cmr_s32 ret = -CMR_CAMERA_FAIL;
    cmr_u32 search_index;
    cmr_u32 i = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->channel1.rot_index;

    for (i = 0; i < CHANNEL1_BUF_CNT_ROT; i++) {
        search_index += i;
        search_index %= CHANNEL1_BUF_CNT_ROT;
        if (0 == prev_cxt->channel1.rot_frm_lock_flag[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->channel1.rot_index = search_index;
            *index = search_index;
            CMR_LOGD("find %d", search_index);
            break;
        }
    }

    return ret;
}

int cmr_channel1_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)&buffer;

    message.msg_type = PREV_EVT_CHANNEL1_QUEUE_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int channel2_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 rot_frm_start_num = 0;
    cmr_u32 reserved_count = 1;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->channel2.size.width;
    height = prev_cxt->channel2.size.height;

    CMR_LOGD("channel2_fmt=%d, w=%d, h=%d", prev_cxt->prev_param.channel2_fmt,
             width, height);
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->prev_param.channel2_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->prev_param.channel2_fmt) {
        prev_cxt->channel2.buf_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->prev_param.channel2_fmt) {
        prev_cxt->channel2.buf_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->prev_param.channel2_fmt) {
        prev_cxt->channel2.buf_size = (width * height * 3) >> 1;
        prev_cxt->prev_param.channel2_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
               prev_cxt->prev_param.channel2_fmt) {
        prev_cxt->channel2.buf_size = (width * height) << 1;
    } else {
        CMR_LOGE("unsupprot fmt %d", prev_cxt->prev_param.channel2_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

#ifdef CONFIG_CAMERA_HAL_VERSION_1
    prev_cxt->channel2.buf_cnt = CHANNEL2_BUF_CNT;
    if (prev_cxt->prev_param.channel2_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel2.buf_cnt += CHANNEL2_BUF_CNT_ROT;
    }

    if (!is_restart) {
        mem_ops->alloc_mem(
            CAMERA_CHANNEL_2, handle->oem_handle, &prev_cxt->channel2.buf_size,
            &prev_cxt->channel2.buf_cnt, prev_cxt->channel2.addr_phy,
            prev_cxt->channel2.addr_vir, prev_cxt->channel2.fd);

        prev_cxt->channel2.valid_buf_cnt = CHANNEL2_BUF_CNT;
        rot_frm_start_num = CHANNEL2_BUF_CNT;

        mem_ops->alloc_mem(CAMERA_CHANNEL_2_RESERVED, handle->oem_handle,
                           &prev_cxt->channel2.buf_size, &reserved_count,
                           &prev_cxt->channel2.frm_reserved.addr_phy,
                           &prev_cxt->channel2.frm_reserved.addr_vir,
                           &prev_cxt->channel2.frm_reserved.fd);
    }
#else
    prev_cxt->channel2.buf_cnt = 0;
    if (prev_cxt->prev_param.channel2_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel2.buf_cnt += CHANNEL2_BUF_CNT_ROT;
        if (!is_restart) {
            mem_ops->alloc_mem(CAMERA_CHANNEL_2, handle->oem_handle,
                               &prev_cxt->channel2.buf_size,
                               &prev_cxt->channel2.buf_cnt,
                               &prev_cxt->channel2.addr_phy[CHANNEL2_BUF_CNT],
                               &prev_cxt->channel2.addr_vir[CHANNEL2_BUF_CNT],
                               &prev_cxt->channel2.fd[CHANNEL2_BUF_CNT]);
        }
    }

    if (!is_restart) {
        prev_cxt->channel2.valid_buf_cnt = 0;
        rot_frm_start_num = CHANNEL2_BUF_CNT;
        mem_ops->alloc_mem(CAMERA_CHANNEL_2_RESERVED, handle->oem_handle,
                           &prev_cxt->channel2.buf_size, &reserved_count,
                           &prev_cxt->channel2.frm_reserved.addr_phy.addr_y,
                           &prev_cxt->channel2.frm_reserved.addr_vir.addr_y,
                           &prev_cxt->channel2.frm_reserved.fd);
    }
#endif

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->channel2.valid_buf_cnt;
    buffer->length = prev_cxt->channel2.buf_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->channel2.valid_buf_cnt; i++) {
        prev_cxt->channel2.frm[i].fmt = prev_cxt->prev_param.channel2_fmt;
        prev_cxt->channel2.frm[i].buf_size = prev_cxt->channel2.buf_size;
        prev_cxt->channel2.frm[i].size.width = prev_cxt->channel2.size.width;
        prev_cxt->channel2.frm[i].size.height = prev_cxt->channel2.size.height;
        prev_cxt->channel2.frm[i].addr_vir.addr_y =
            prev_cxt->channel2.addr_vir[i];
        prev_cxt->channel2.frm[i].addr_vir.addr_u =
            prev_cxt->channel2.frm[i].addr_vir.addr_y + width * height;
        prev_cxt->channel2.frm[i].addr_phy.addr_y =
            prev_cxt->channel2.addr_phy[i];
        prev_cxt->channel2.frm[i].addr_phy.addr_u =
            prev_cxt->channel2.frm[i].addr_phy.addr_y + width * height;
        prev_cxt->channel2.frm[i].fd = prev_cxt->channel2.fd[i];

        buffer->addr[i].addr_y = prev_cxt->channel2.frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->channel2.frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->channel2.frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->channel2.frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->channel2.frm[i].fd;
    }

    prev_cxt->channel2.frm_reserved.fmt = prev_cxt->prev_param.channel2_fmt;
    prev_cxt->channel2.frm_reserved.buf_size = prev_cxt->channel2.buf_size;
    prev_cxt->channel2.frm_reserved.size.width = width;
    prev_cxt->channel2.frm_reserved.size.height = height;
    //prev_cxt->channel2.frm_reserved.addr_vir.addr_y =
        //prev_cxt->channel2.frm_reserved.addr_vir.addr_y;
    prev_cxt->channel2.frm_reserved.addr_vir.addr_u =
        prev_cxt->channel2.frm_reserved.addr_vir.addr_y + width * height;
    //prev_cxt->channel2.frm_reserved.addr_phy.addr_y =
        //prev_cxt->channel2.frm_reserved.addr_phy.addr_y;
    prev_cxt->channel2.frm_reserved.addr_phy.addr_u =
        prev_cxt->channel2.frm_reserved.addr_phy.addr_y + width * height;
    //prev_cxt->channel2.frm_reserved.fd = prev_cxt->channel2.frm_reserved.fd;

    if (prev_cxt->prev_param.channel2_rot_angle) {
        for (i = 0; i < CHANNEL2_BUF_CNT_ROT; i++) {
            prev_cxt->channel2.rot_frm[i].fmt =
                prev_cxt->prev_param.channel2_fmt;
            prev_cxt->channel2.rot_frm[i].buf_size =
                prev_cxt->channel2.buf_size;
            prev_cxt->channel2.rot_frm[i].size.width =
                prev_cxt->channel2.size.width;
            prev_cxt->channel2.rot_frm[i].size.height =
                prev_cxt->channel2.size.height;
            prev_cxt->channel2.rot_frm[i].addr_vir.addr_y =
                prev_cxt->channel2.addr_vir[rot_frm_start_num + i];
            prev_cxt->channel2.rot_frm[i].addr_vir.addr_u =
                prev_cxt->channel2.rot_frm[i].addr_vir.addr_y + width * height;
            prev_cxt->channel2.rot_frm[i].addr_phy.addr_y =
                prev_cxt->channel2.addr_phy[rot_frm_start_num + i];
            prev_cxt->channel2.rot_frm[i].addr_phy.addr_u =
                prev_cxt->channel2.rot_frm[i].addr_phy.addr_y + width * height;
            prev_cxt->channel2.rot_frm[i].fd =
                prev_cxt->channel2.fd[rot_frm_start_num + i];
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int channel2_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_CHANNEL_2, handle->oem_handle,
                          prev_cxt->channel2.addr_phy,
                          prev_cxt->channel2.addr_vir, prev_cxt->channel2.fd,
                          prev_cxt->channel2.buf_cnt);
        cmr_bzero(prev_cxt->channel2.addr_phy,
                  (CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel2.addr_vir,
                  (CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel2.fd,
                  (CHANNEL2_BUF_CNT + CHANNEL2_BUF_CNT_ROT) * sizeof(cmr_s32));

        mem_ops->free_mem(
            CAMERA_CHANNEL_2_RESERVED, handle->oem_handle,
            (cmr_uint *)prev_cxt->channel2.frm_reserved.addr_phy.addr_y,
            (cmr_uint *)prev_cxt->channel2.frm_reserved.addr_vir.addr_y,
            &prev_cxt->channel2.frm_reserved.fd, 1);

        prev_cxt->channel2.frm_reserved.addr_phy.addr_y = 0;
        prev_cxt->channel2.frm_reserved.addr_vir.addr_y = 0;
        prev_cxt->channel2.frm_reserved.fd = 0;
    }

    return ret;
}

cmr_int channel2_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&endian, sizeof(struct img_data_end));
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    cmr_bzero(&prev_cxt->channel2, sizeof(channel2_t));

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel2_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    prev_cxt->channel2.size.width = prev_cxt->channel2_actual_pic_size.width;
    prev_cxt->channel2.size.height = prev_cxt->channel2_actual_pic_size.height;
    prev_cxt->channel2.frm_cnt = 0;
    prev_cxt->channel2.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel2.skip_mode = IMG_SKIP_SW_KER;

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    chn_param.sensor_mode = prev_cxt->channel2_work_mode;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel2_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.flip_on =
        handle->prev_cxt[camera_id].prev_param.channel2_flip_on;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.sence_mode =
        DCAM_SCENE_MODE_CAPTURE_CALLBACK; // TBD
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    } else {
        chn_param.cap_inf_cfg.cfg.need_isp = 0;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel2.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel2.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel2.size.width /
                             prev_cxt->channel2.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel2_rot_angle);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    ret = channel2_alloc_bufs(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }

    // config 4in1 flag
    if (check_software_remosaic(prev_cxt)) {
        CMR_LOGD("set 4in1 mode to 1");
        chn_param.cap_inf_cfg.cfg.need_4in1 = 1;
    }

    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    prev_cxt->channel2.chn_id = channel_id;
    prev_cxt->channel2.chn_status = PREV_CHN_BUSY;
    prev_cxt->channel2.endian = endian;
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /* skip frame in dcam driver */
    if (prev_cxt->channel2.skip_mode == IMG_SKIP_SW_KER) {
        for (i = 0; i < prev_cxt->channel2.skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->channel2.chn_id;
            buf_cfg.base_id = CMR_VIDEO_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->channel2.frm_reserved.buf_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->channel2.frm_reserved.addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->channel2.frm_reserved.addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->channel2.frm_reserved.addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->channel2.frm_reserved.addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->channel2.frm_reserved.fd;
            buf_cfg.frame_number = 0xFFFFFFFF;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->channel2.chn_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->channel2.chn_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel2.frm_reserved.buf_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->channel2.frm_reserved.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->channel2.frm_reserved.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->channel2.frm_reserved.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->channel2.frm_reserved.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->channel2.frm_reserved.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (out_param_ptr) {
        out_param_ptr->channel2_chn_id = prev_cxt->channel2.chn_id;
        out_param_ptr->channel2_sn_mode = prev_cxt->channel2_work_mode;
        out_param_ptr->channel2_endian = prev_cxt->channel2.endian;
        out_param_ptr->channel2_size = prev_cxt->channel2.size;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int channel2_update_params(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));

    prev_cxt = &handle->prev_cxt[camera_id];

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel2_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    //	cmr_bzero(prev_cxt->prev_rot_frm_is_lock, PREV_ROT_FRM_CNT *
    // sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->channel2.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel2.skip_mode = IMG_SKIP_SW_KER;
    chn_param.sensor_mode = prev_cxt->channel2_work_mode;
    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel2_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format)
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    else
        chn_param.cap_inf_cfg.cfg.need_isp = 0;

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel2.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel2.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel2.size.width /
                             prev_cxt->channel2.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel2_rot_angle);
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    channel_id = prev_cxt->channel2.chn_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    return ret;
}

cmr_s32 channel2_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_num = 0;
    cmr_u32 width, height;
    struct buffer_cfg buf_cfg;
    cmr_u32 rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->channel2.valid_buf_cnt;

    if (valid_num >= CHANNEL2_BUF_CNT) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    width = prev_cxt->channel2.size.width;
    height = prev_cxt->channel2.size.height;

    prev_cxt->channel2.fd[valid_num] = buffer->fd;
    prev_cxt->channel2.addr_phy[valid_num] = (unsigned long)buffer->addr_phy;
    prev_cxt->channel2.addr_vir[valid_num] = (unsigned long)buffer->addr_vir;

    prev_cxt->channel2.frm[valid_num].fmt = prev_cxt->prev_param.channel2_fmt;
    prev_cxt->channel2.frm[valid_num].buf_size = prev_cxt->channel2.buf_size;
    prev_cxt->channel2.frm[valid_num].size.width = width;
    prev_cxt->channel2.frm[valid_num].size.height = height;
    prev_cxt->channel2.frm[valid_num].fd = prev_cxt->channel2.fd[valid_num];
    prev_cxt->channel2.frm[valid_num].addr_phy.addr_y =
        prev_cxt->channel2.addr_phy[valid_num];
    prev_cxt->channel2.frm[valid_num].addr_phy.addr_u =
        prev_cxt->channel2.frm[valid_num].addr_phy.addr_y + width * height;
    prev_cxt->channel2.frm[valid_num].addr_vir.addr_y =
        prev_cxt->channel2.addr_vir[valid_num];
    prev_cxt->channel2.frm[valid_num].addr_vir.addr_u =
        prev_cxt->channel2.frm[valid_num].addr_vir.addr_y + width * height;

    prev_cxt->channel2.valid_buf_cnt++;

    buf_cfg.channel_id = prev_cxt->channel2.chn_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE; // no use, will remove it
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel2.buf_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;
    buf_cfg.frame_number = buffer->frame_number;
    if (prev_cxt->prev_param.channel2_rot_angle) {
        ret = channel2_find_free_rot_buffer(prev_cxt, &rot_index);
        if (ret) {
            CMR_LOGE("prev_search_rot_buffer failed");
            goto exit;
        }

        buf_cfg.fd[0] = prev_cxt->channel2.rot_frm[rot_index].fd;
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel2.rot_frm[rot_index].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel2.rot_frm[rot_index].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel2.rot_frm[rot_index].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel2.rot_frm[rot_index].addr_vir.addr_u;

        prev_cxt->channel2.rot_frm_lock_flag[rot_index] = 1;
    } else {
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel2.frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel2.frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel2.frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel2.frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->channel2.frm[valid_num].fd;
    }

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d",
             prev_cxt->camera_id, prev_cxt->channel2.frm[valid_num].fd,
             prev_cxt->channel2.chn_id, prev_cxt->channel2.valid_buf_cnt);
    ATRACE_END();
    return ret;
}

cmr_uint g_channel2_frame_dump_cnt = 0;
int channel2_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int frm_index = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0, channel2_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0, channel2_rot_angle = 0;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_buf_cnt;
    struct prev_cb_info cb_data_info;
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;
    int rot_index;
    struct camera_frame_type frame_type;
    int i;

    if (!handle || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p", handle, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    channel2_chn_id = handle->prev_cxt[camera_id].channel2.chn_id;
    channel2_rot_angle =
        handle->prev_cxt[camera_id].prev_param.channel2_rot_angle;
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->channel2.frm_cnt == 0) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->channel2.frm_cnt++;

    if (info->channel_id != channel2_chn_id) {
        CMR_LOGE("ignored, channel id %d", info->channel_id);
        goto exit;
    }

    if (prev_cxt->channel2.valid_buf_cnt <= 0) {
        ret = -1;
        CMR_LOGE("chn2 valid_buf_cnt=%d", prev_cxt->channel2.valid_buf_cnt);
        goto exit;
    }

    // no rotation
    if (channel2_rot_angle == 0) {
        if (prev_cxt->channel2.frm[0].fd != (cmr_s32)info->fd) {
            ret = -1;
            CMR_LOGE("frame sequence error from kernel driver: info->fd=0x%x, "
                     "prev_cxt->channel2.frm[0].fd=0x%x",
                     info->fd, prev_cxt->channel2.frm[0].fd);
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel2.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel2.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel2.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel2.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel2.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel2.size.width;
        frame_type.height = prev_cxt->channel2.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL2_FRAME;

        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.channel2.dump.count", value, "null");
        cmr_uint dump_num = atoi(value);
        if (strcmp(value, "null")) {
            if (g_channel2_frame_dump_cnt < dump_num) {
                char tag_name[30]={0};
                char cameraId[2] ={0};
                strcpy(tag_name, "dump_channel2_frame");
                sprintf(cameraId, "%d", camera_id);
                strcat(tag_name, cameraId);
                dump_image(tag_name, CAM_IMG_FMT_YUV420_NV21,
                           frame_type.width, frame_type.height,
                           prev_cxt->channel2.frm_cnt,
                           &prev_cxt->channel2.frm[0].addr_vir,
                           frame_type.width * frame_type.height * 3 / 2);
                g_channel2_frame_dump_cnt++;
            }
        }

        valid_buf_cnt = prev_cxt->channel2.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel2.fd[i] = prev_cxt->channel2.fd[i + 1];
            prev_cxt->channel2.addr_phy[i] = prev_cxt->channel2.addr_phy[i + i];
            prev_cxt->channel2.addr_vir[i] = prev_cxt->channel2.addr_vir[i + 1];
            prev_cxt->channel2.frm[i] = prev_cxt->channel2.frm[i + 1];
        }

        prev_cxt->channel2.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel2.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel2.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel2.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));
    } else {
        // rotation case
        ret = get_frame_index(prev_cxt->channel2.rot_frm, CHANNEL2_BUF_CNT_ROT,
                              info, &rot_index);
        if (ret) {
            CMR_LOGE("frame is not match");
            goto exit;
        }

        rot_param.src_img = &prev_cxt->channel2.rot_frm[rot_index];
        rot_param.src_img->data_end = prev_cxt->channel2.endian;
        rot_param.dst_img = &prev_cxt->channel2.frm[0];
        rot_param.dst_img->data_end = prev_cxt->channel2.endian;
        rot_param.angle = channel2_rot_angle;
        op_mean.rot = channel2_rot_angle;
        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel2.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel2.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel2.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel2.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel2.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel2.size.width;
        frame_type.height = prev_cxt->channel2.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL2_FRAME;

        valid_buf_cnt = prev_cxt->channel2.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel2.fd[i] = prev_cxt->channel2.fd[i + 1];
            prev_cxt->channel2.addr_phy[i] = prev_cxt->channel2.addr_phy[i + i];
            prev_cxt->channel2.addr_vir[i] = prev_cxt->channel2.addr_vir[i + 1];
            prev_cxt->channel2.frm[i] = prev_cxt->channel2.frm[i + 1];
        }

        prev_cxt->channel2.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel2.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel2.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel2.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));

        prev_cxt->channel2.rot_frm_lock_flag[rot_index] = 0;
    }

    prev_cxt->channel2.valid_buf_cnt--;

    cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d, frame_index=%d",
             prev_cxt->camera_id, frame_type.fd, info->channel_id,
             prev_cxt->channel2.valid_buf_cnt, info->frame_real_id);

exit:
    return ret;
}

cmr_s32 channel2_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index) {
    cmr_s32 ret = -CMR_CAMERA_FAIL;
    cmr_u32 search_index;
    cmr_u32 i = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->channel2.rot_index;

    for (i = 0; i < CHANNEL2_BUF_CNT_ROT; i++) {
        search_index += i;
        search_index %= CHANNEL2_BUF_CNT_ROT;
        if (0 == prev_cxt->channel2.rot_frm_lock_flag[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->channel2.rot_index = search_index;
            *index = search_index;
            CMR_LOGD("find %d", search_index);
            break;
        }
    }

    return ret;
}

int cmr_channel2_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)&buffer;

    message.msg_type = PREV_EVT_CHANNEL2_QUEUE_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int channel3_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 rot_frm_start_num = 0;
    cmr_u32 reserved_count = 1;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->channel3.size.width;
    height = prev_cxt->channel3.size.height;

    CMR_LOGD("channel3_fmt=%d, w=%d, h=%d", prev_cxt->prev_param.channel3_fmt,
             width, height);
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->prev_param.channel3_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->prev_param.channel3_fmt) {
        prev_cxt->channel3.buf_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->prev_param.channel3_fmt) {
        prev_cxt->channel3.buf_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->prev_param.channel3_fmt) {
        prev_cxt->channel3.buf_size = (width * height * 3) >> 1;
        prev_cxt->prev_param.channel3_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
               prev_cxt->prev_param.channel3_fmt) {
        prev_cxt->channel3.buf_size = (width * height) << 1;
    } else {
        CMR_LOGE("unsupprot fmt %d", prev_cxt->prev_param.channel3_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

#ifdef CONFIG_CAMERA_HAL_VERSION_1
    prev_cxt->channel3.buf_cnt = CHANNEL3_BUF_CNT;
    if (prev_cxt->prev_param.channel3_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel3.buf_cnt += CHANNEL3_BUF_CNT_ROT;
    }

    if (!is_restart) {
        mem_ops->alloc_mem(
            CAMERA_CHANNEL_3, handle->oem_handle, &prev_cxt->channel3.buf_size,
            &prev_cxt->channel3.buf_cnt, prev_cxt->channel3.addr_phy,
            prev_cxt->channel3.addr_vir, prev_cxt->channel3.fd);

        prev_cxt->channel3.valid_buf_cnt = CHANNEL3_BUF_CNT;
        rot_frm_start_num = CHANNEL3_BUF_CNT;

        mem_ops->alloc_mem(CAMERA_CHANNEL_3_RESERVED, handle->oem_handle,
                           &prev_cxt->channel3.buf_size, &reserved_count,
                           &prev_cxt->channel3.frm_reserved.addr_phy,
                           &prev_cxt->channel3.frm_reserved.addr_vir,
                           &prev_cxt->channel3.frm_reserved.fd);
    }
#else
    prev_cxt->channel3.buf_cnt = 0;
    if (prev_cxt->prev_param.channel3_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel3.buf_cnt += CHANNEL3_BUF_CNT_ROT;
        if (!is_restart) {
            mem_ops->alloc_mem(CAMERA_CHANNEL_3, handle->oem_handle,
                               &prev_cxt->channel3.buf_size,
                               &prev_cxt->channel3.buf_cnt,
                               &prev_cxt->channel3.addr_phy[CHANNEL3_BUF_CNT],
                               &prev_cxt->channel3.addr_vir[CHANNEL3_BUF_CNT],
                               &prev_cxt->channel3.fd[CHANNEL3_BUF_CNT]);
        }
    }

    if (!is_restart) {
        prev_cxt->channel3.valid_buf_cnt = 0;
        rot_frm_start_num = CHANNEL3_BUF_CNT;
        mem_ops->alloc_mem(CAMERA_CHANNEL_3_RESERVED, handle->oem_handle,
                           &prev_cxt->channel3.buf_size, &reserved_count,
                           &prev_cxt->channel3.frm_reserved.addr_phy.addr_y,
                           &prev_cxt->channel3.frm_reserved.addr_vir.addr_y,
                           &prev_cxt->channel3.frm_reserved.fd);
    }
#endif

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->channel3.valid_buf_cnt;
    buffer->length = prev_cxt->channel3.buf_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->channel3.valid_buf_cnt; i++) {
        prev_cxt->channel3.frm[i].fmt = prev_cxt->prev_param.channel3_fmt;
        prev_cxt->channel3.frm[i].buf_size = prev_cxt->channel3.buf_size;
        prev_cxt->channel3.frm[i].size.width = prev_cxt->channel3.size.width;
        prev_cxt->channel3.frm[i].size.height = prev_cxt->channel3.size.height;
        prev_cxt->channel3.frm[i].addr_vir.addr_y =
            prev_cxt->channel3.addr_vir[i];
        prev_cxt->channel3.frm[i].addr_vir.addr_u =
            prev_cxt->channel3.frm[i].addr_vir.addr_y + width * height;
        prev_cxt->channel3.frm[i].addr_phy.addr_y =
            prev_cxt->channel3.addr_phy[i];
        prev_cxt->channel3.frm[i].addr_phy.addr_u =
            prev_cxt->channel3.frm[i].addr_phy.addr_y + width * height;
        prev_cxt->channel3.frm[i].fd = prev_cxt->channel3.fd[i];

        buffer->addr[i].addr_y = prev_cxt->channel3.frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->channel3.frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->channel3.frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->channel3.frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->channel3.frm[i].fd;
    }

    prev_cxt->channel3.frm_reserved.fmt = prev_cxt->prev_param.channel3_fmt;
    prev_cxt->channel3.frm_reserved.buf_size = prev_cxt->channel3.buf_size;
    prev_cxt->channel3.frm_reserved.size.width = width;
    prev_cxt->channel3.frm_reserved.size.height = height;
    //prev_cxt->channel3.frm_reserved.addr_vir.addr_y =
        //prev_cxt->channel3.frm_reserved.addr_vir.addr_y;
    prev_cxt->channel3.frm_reserved.addr_vir.addr_u =
        prev_cxt->channel3.frm_reserved.addr_vir.addr_y + width * height;
    //prev_cxt->channel3.frm_reserved.addr_phy.addr_y =
        //prev_cxt->channel3.frm_reserved.addr_phy.addr_y;
    prev_cxt->channel3.frm_reserved.addr_phy.addr_u =
        prev_cxt->channel3.frm_reserved.addr_phy.addr_y + width * height;
    //prev_cxt->channel3.frm_reserved.fd = prev_cxt->channel3.frm_reserved.fd;

    if (prev_cxt->prev_param.channel3_rot_angle) {
        for (i = 0; i < CHANNEL3_BUF_CNT_ROT; i++) {
            prev_cxt->channel3.rot_frm[i].fmt =
                prev_cxt->prev_param.channel3_fmt;
            prev_cxt->channel3.rot_frm[i].buf_size =
                prev_cxt->channel3.buf_size;
            prev_cxt->channel3.rot_frm[i].size.width =
                prev_cxt->channel3.size.width;
            prev_cxt->channel3.rot_frm[i].size.height =
                prev_cxt->channel3.size.height;
            prev_cxt->channel3.rot_frm[i].addr_vir.addr_y =
                prev_cxt->channel3.addr_vir[rot_frm_start_num + i];
            prev_cxt->channel3.rot_frm[i].addr_vir.addr_u =
                prev_cxt->channel3.rot_frm[i].addr_vir.addr_y + width * height;
            prev_cxt->channel3.rot_frm[i].addr_phy.addr_y =
                prev_cxt->channel3.addr_phy[rot_frm_start_num + i];
            prev_cxt->channel3.rot_frm[i].addr_phy.addr_u =
                prev_cxt->channel3.rot_frm[i].addr_phy.addr_y + width * height;
            prev_cxt->channel3.rot_frm[i].fd =
                prev_cxt->channel3.fd[rot_frm_start_num + i];
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int channel3_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_CHANNEL_3, handle->oem_handle,
                          prev_cxt->channel3.addr_phy,
                          prev_cxt->channel3.addr_vir, prev_cxt->channel3.fd,
                          prev_cxt->channel3.buf_cnt);
        cmr_bzero(prev_cxt->channel3.addr_phy,
                  (CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel3.addr_vir,
                  (CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel3.fd,
                  (CHANNEL3_BUF_CNT + CHANNEL3_BUF_CNT_ROT) * sizeof(cmr_s32));

        mem_ops->free_mem(
            CAMERA_CHANNEL_3_RESERVED, handle->oem_handle,
            (cmr_uint *)prev_cxt->channel3.frm_reserved.addr_phy.addr_y,
            (cmr_uint *)prev_cxt->channel3.frm_reserved.addr_vir.addr_y,
            &prev_cxt->channel3.frm_reserved.fd, 1);

        prev_cxt->channel3.frm_reserved.addr_phy.addr_y = 0;
        prev_cxt->channel3.frm_reserved.addr_vir.addr_y = 0;
        prev_cxt->channel3.frm_reserved.fd = 0;
    }

    return ret;
}

cmr_int channel3_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&endian, sizeof(struct img_data_end));
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    cmr_bzero(&prev_cxt->channel3, sizeof(channel3_t));

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel3_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    prev_cxt->channel3.size.width = prev_cxt->channel3_actual_pic_size.width;
    prev_cxt->channel3.size.height = prev_cxt->channel3_actual_pic_size.height;
    prev_cxt->channel3.frm_cnt = 0;
    prev_cxt->channel3.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel3.skip_mode = IMG_SKIP_SW_KER;

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    chn_param.sensor_mode = prev_cxt->channel3_work_mode;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel3_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.flip_on =
        handle->prev_cxt[camera_id].prev_param.channel3_flip_on;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.sence_mode =
        DCAM_SCENE_MODE_CAPTURE_CALLBACK; // TBD
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    } else {
        chn_param.cap_inf_cfg.cfg.need_isp = 0;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel3.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel3.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel3.size.width /
                             prev_cxt->channel3.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel3_rot_angle);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    ret = channel3_alloc_bufs(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }

    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    prev_cxt->channel3.chn_id = channel_id;
    prev_cxt->channel3.chn_status = PREV_CHN_BUSY;
    prev_cxt->channel3.endian = endian;
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /* skip frame in dcam driver */
    if (prev_cxt->channel3.skip_mode == IMG_SKIP_SW_KER) {
        for (i = 0; i < prev_cxt->channel3.skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->channel3.chn_id;
            buf_cfg.base_id = CMR_VIDEO_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->channel3.frm_reserved.buf_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->channel3.frm_reserved.addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->channel3.frm_reserved.addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->channel3.frm_reserved.addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->channel3.frm_reserved.addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->channel3.frm_reserved.fd;
            buf_cfg.frame_number = 0xFFFFFFFF;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->channel3.chn_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->channel3.chn_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel3.frm_reserved.buf_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->channel3.frm_reserved.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->channel3.frm_reserved.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->channel3.frm_reserved.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->channel3.frm_reserved.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->channel3.frm_reserved.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (out_param_ptr) {
        out_param_ptr->channel3_chn_id = prev_cxt->channel3.chn_id;
        out_param_ptr->channel3_sn_mode = prev_cxt->channel3_work_mode;
        out_param_ptr->channel3_endian = prev_cxt->channel3.endian;
        out_param_ptr->channel3_size = prev_cxt->channel3.size;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int channel3_update_params(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));

    prev_cxt = &handle->prev_cxt[camera_id];

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel3_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    //	cmr_bzero(prev_cxt->prev_rot_frm_is_lock, PREV_ROT_FRM_CNT *
    // sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->channel3.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel3.skip_mode = IMG_SKIP_SW_KER;
    chn_param.sensor_mode = prev_cxt->channel3_work_mode;
    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel3_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format)
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    else
        chn_param.cap_inf_cfg.cfg.need_isp = 0;

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel3.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel3.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel3.size.width /
                             prev_cxt->channel3.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel3_rot_angle);
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    channel_id = prev_cxt->channel3.chn_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    return ret;
}

cmr_s32 channel3_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_num = 0;
    cmr_u32 width, height;
    struct buffer_cfg buf_cfg;
    cmr_u32 rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->channel3.valid_buf_cnt;

    if (valid_num >= CHANNEL3_BUF_CNT) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    width = prev_cxt->channel3.size.width;
    height = prev_cxt->channel3.size.height;

    prev_cxt->channel3.fd[valid_num] = buffer->fd;
    prev_cxt->channel3.addr_phy[valid_num] = (unsigned long)buffer->addr_phy;
    prev_cxt->channel3.addr_vir[valid_num] = (unsigned long)buffer->addr_vir;

    prev_cxt->channel3.frm[valid_num].fmt = prev_cxt->prev_param.channel3_fmt;
    prev_cxt->channel3.frm[valid_num].buf_size = prev_cxt->channel3.buf_size;
    prev_cxt->channel3.frm[valid_num].size.width = width;
    prev_cxt->channel3.frm[valid_num].size.height = height;
    prev_cxt->channel3.frm[valid_num].fd = prev_cxt->channel3.fd[valid_num];
    prev_cxt->channel3.frm[valid_num].addr_phy.addr_y =
        prev_cxt->channel3.addr_phy[valid_num];
    prev_cxt->channel3.frm[valid_num].addr_phy.addr_u =
        prev_cxt->channel3.frm[valid_num].addr_phy.addr_y + width * height;
    prev_cxt->channel3.frm[valid_num].addr_vir.addr_y =
        prev_cxt->channel3.addr_vir[valid_num];
    prev_cxt->channel3.frm[valid_num].addr_vir.addr_u =
        prev_cxt->channel3.frm[valid_num].addr_vir.addr_y + width * height;

    prev_cxt->channel3.valid_buf_cnt++;

    buf_cfg.channel_id = prev_cxt->channel3.chn_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE; // no use, will remove it
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel3.buf_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;
    buf_cfg.frame_number = buffer->frame_number;
    if (prev_cxt->prev_param.channel3_rot_angle) {
        ret = channel3_find_free_rot_buffer(prev_cxt, &rot_index);
        if (ret) {
            CMR_LOGE("prev_search_rot_buffer failed");
            goto exit;
        }

        buf_cfg.fd[0] = prev_cxt->channel3.rot_frm[rot_index].fd;
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel3.rot_frm[rot_index].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel3.rot_frm[rot_index].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel3.rot_frm[rot_index].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel3.rot_frm[rot_index].addr_vir.addr_u;

        prev_cxt->channel3.rot_frm_lock_flag[rot_index] = 1;
    } else {
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel3.frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel3.frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel3.frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel3.frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->channel3.frm[valid_num].fd;
    }

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d",
             prev_cxt->camera_id, prev_cxt->channel3.frm[valid_num].fd,
             prev_cxt->channel3.chn_id, prev_cxt->channel3.valid_buf_cnt);
    ATRACE_END();
    return ret;
}

int channel3_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int frm_index = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0, channel3_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0, channel3_rot_angle = 0;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_buf_cnt;
    struct prev_cb_info cb_data_info;
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;
    int rot_index;
    struct camera_frame_type frame_type;
    int i;

    if (!handle || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p", handle, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    channel3_chn_id = handle->prev_cxt[camera_id].channel3.chn_id;
    channel3_rot_angle =
        handle->prev_cxt[camera_id].prev_param.channel3_rot_angle;
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->channel3.frm_cnt == 0) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->channel3.frm_cnt++;

    if (info->channel_id != channel3_chn_id) {
        CMR_LOGE("ignored, channel id %d", info->channel_id);
        goto exit;
    }

    if (prev_cxt->channel3.valid_buf_cnt <= 0) {
        ret = -1;
        CMR_LOGE("chn3 valid_buf_cnt=%d", prev_cxt->channel3.valid_buf_cnt);
        goto exit;
    }

    // no rotation
    if (channel3_rot_angle == 0) {
        if (prev_cxt->channel3.frm[0].fd != (cmr_s32)info->fd) {
            ret = -1;
            CMR_LOGE("frame sequence error from kernel driver: info->fd=0x%x, "
                     "prev_cxt->channel3.frm[0].fd=0x%x",
                     info->fd, prev_cxt->channel3.frm[0].fd);
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel3.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel3.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel3.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel3.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel3.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel3.size.width;
        frame_type.height = prev_cxt->channel3.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL3_FRAME;

        valid_buf_cnt = prev_cxt->channel3.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel3.fd[i] = prev_cxt->channel3.fd[i + 1];
            prev_cxt->channel3.addr_phy[i] = prev_cxt->channel3.addr_phy[i + i];
            prev_cxt->channel3.addr_vir[i] = prev_cxt->channel3.addr_vir[i + 1];
            prev_cxt->channel3.frm[i] = prev_cxt->channel3.frm[i + 1];
        }

        prev_cxt->channel3.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel3.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel3.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel3.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));
    } else {
        // rotation case
        ret = get_frame_index(prev_cxt->channel3.rot_frm, CHANNEL3_BUF_CNT_ROT,
                              info, &rot_index);
        if (ret) {
            CMR_LOGE("frame is not match");
            goto exit;
        }

        rot_param.src_img = &prev_cxt->channel3.rot_frm[rot_index];
        rot_param.src_img->data_end = prev_cxt->channel3.endian;
        rot_param.dst_img = &prev_cxt->channel3.frm[0];
        rot_param.dst_img->data_end = prev_cxt->channel3.endian;
        rot_param.angle = channel3_rot_angle;
        op_mean.rot = channel3_rot_angle;
        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel3.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel3.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel3.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel3.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel3.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel3.size.width;
        frame_type.height = prev_cxt->channel3.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL3_FRAME;

        valid_buf_cnt = prev_cxt->channel3.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel3.fd[i] = prev_cxt->channel3.fd[i + 1];
            prev_cxt->channel3.addr_phy[i] = prev_cxt->channel3.addr_phy[i + i];
            prev_cxt->channel3.addr_vir[i] = prev_cxt->channel3.addr_vir[i + 1];
            prev_cxt->channel3.frm[i] = prev_cxt->channel3.frm[i + 1];
        }

        prev_cxt->channel3.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel3.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel3.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel3.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));

        prev_cxt->channel3.rot_frm_lock_flag[rot_index] = 0;
    }

    prev_cxt->channel3.valid_buf_cnt--;

    cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d, frame_index=%d",
             prev_cxt->camera_id, frame_type.fd, info->channel_id,
             prev_cxt->channel3.valid_buf_cnt, info->frame_real_id);

exit:
    return ret;
}

cmr_s32 channel3_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index) {
    cmr_s32 ret = -CMR_CAMERA_FAIL;
    cmr_u32 search_index;
    cmr_u32 i = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->channel3.rot_index;

    for (i = 0; i < CHANNEL3_BUF_CNT_ROT; i++) {
        search_index += i;
        search_index %= CHANNEL3_BUF_CNT_ROT;
        if (0 == prev_cxt->channel3.rot_frm_lock_flag[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->channel3.rot_index = search_index;
            *index = search_index;
            CMR_LOGD("find %d", search_index);
            break;
        }
    }

    return ret;
}

int cmr_channel3_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)&buffer;

    message.msg_type = PREV_EVT_CHANNEL3_QUEUE_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int channel4_alloc_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 is_restart, struct buffer_cfg *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 buffer_size = 0;
    cmr_u32 frame_num = 0;
    cmr_uint i = 0;
    cmr_u32 width, height = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 rot_frm_start_num = 0;
    cmr_u32 reserved_count = 1;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!buffer) {
        CMR_LOGE("null param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;
    width = prev_cxt->channel4.size.width;
    height = prev_cxt->channel4.size.height;

    CMR_LOGD("channel4_fmt=%d, w=%d, h=%d", prev_cxt->prev_param.channel4_fmt,
             width, height);
    if (CAM_IMG_FMT_YUV420_NV21 == prev_cxt->prev_param.channel4_fmt ||
        CAM_IMG_FMT_YUV420_NV12 == prev_cxt->prev_param.channel4_fmt) {
        prev_cxt->channel4.buf_size = (width * height * 3) >> 1;
    } else if (CAM_IMG_FMT_YUV422P == prev_cxt->prev_param.channel4_fmt) {
        prev_cxt->channel4.buf_size = (width * height) << 1;
    } else if (CAM_IMG_FMT_YUV420_YV12 == prev_cxt->prev_param.channel4_fmt) {
        prev_cxt->channel4.buf_size = (width * height * 3) >> 1;
        prev_cxt->prev_param.channel4_fmt = CAM_IMG_FMT_YUV420_NV21;
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW ==
               prev_cxt->prev_param.channel4_fmt) {
        prev_cxt->channel4.buf_size = (width * height) << 1;
    } else {
        CMR_LOGE("unsupprot fmt %d", prev_cxt->prev_param.channel4_fmt);
        return CMR_CAMERA_INVALID_PARAM;
    }

#ifdef CONFIG_CAMERA_HAL_VERSION_1
    prev_cxt->channel4.buf_cnt = CHANNEL4_BUF_CNT;
    if (prev_cxt->prev_param.channel4_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel4.buf_cnt += CHANNEL4_BUF_CNT_ROT;
    }

    if (!is_restart) {
        mem_ops->alloc_mem(
            CAMERA_CHANNEL_4, handle->oem_handle, &prev_cxt->channel4.buf_size,
            &prev_cxt->channel4.buf_cnt, prev_cxt->channel4.addr_phy,
            prev_cxt->channel4.addr_vir, prev_cxt->channel4.fd);

        prev_cxt->channel4.valid_buf_cnt = CHANNEL4_BUF_CNT;
        rot_frm_start_num = CHANNEL4_BUF_CNT;

        mem_ops->alloc_mem(CAMERA_CHANNEL_4_RESERVED, handle->oem_handle,
                           &prev_cxt->channel4.buf_size, &reserved_count,
                           &prev_cxt->channel4.frm_reserved.addr_phy,
                           &prev_cxt->channel4.frm_reserved.addr_vir,
                           &prev_cxt->channel4.frm_reserved.fd);
    }
#else
    prev_cxt->channel4.buf_cnt = 0;
    if (prev_cxt->prev_param.channel4_rot_angle) {
        CMR_LOGD("rotation need more buffer");
        prev_cxt->channel4.buf_cnt += CHANNEL4_BUF_CNT_ROT;
        if (!is_restart) {
            mem_ops->alloc_mem(CAMERA_CHANNEL_4, handle->oem_handle,
                               &prev_cxt->channel4.buf_size,
                               &prev_cxt->channel4.buf_cnt,
                               &prev_cxt->channel4.addr_phy[CHANNEL4_BUF_CNT],
                               &prev_cxt->channel4.addr_vir[CHANNEL4_BUF_CNT],
                               &prev_cxt->channel4.fd[CHANNEL4_BUF_CNT]);
        }
    }

    if (!is_restart) {
        prev_cxt->channel4.valid_buf_cnt = 0;
        rot_frm_start_num = CHANNEL4_BUF_CNT;
        mem_ops->alloc_mem(CAMERA_CHANNEL_4_RESERVED, handle->oem_handle,
                           &prev_cxt->channel4.buf_size, &reserved_count,
                           &prev_cxt->channel4.frm_reserved.addr_phy.addr_y,
                           &prev_cxt->channel4.frm_reserved.addr_vir.addr_y,
                           &prev_cxt->channel4.frm_reserved.fd);
    }
#endif

    /*arrange the buffer*/
    buffer->channel_id = 0; /*should be update when channel cfg complete*/
    buffer->base_id = CMR_PREV_ID_BASE;
    buffer->count = prev_cxt->channel4.valid_buf_cnt;
    buffer->length = prev_cxt->channel4.buf_size;
    buffer->flag = BUF_FLAG_INIT;

    for (i = 0; i < (cmr_uint)prev_cxt->channel4.valid_buf_cnt; i++) {
        prev_cxt->channel4.frm[i].fmt = prev_cxt->prev_param.channel4_fmt;
        prev_cxt->channel4.frm[i].buf_size = prev_cxt->channel4.buf_size;
        prev_cxt->channel4.frm[i].size.width = prev_cxt->channel4.size.width;
        prev_cxt->channel4.frm[i].size.height = prev_cxt->channel4.size.height;
        prev_cxt->channel4.frm[i].addr_vir.addr_y =
            prev_cxt->channel4.addr_vir[i];
        prev_cxt->channel4.frm[i].addr_vir.addr_u =
            prev_cxt->channel4.frm[i].addr_vir.addr_y + width * height;
        prev_cxt->channel4.frm[i].addr_phy.addr_y =
            prev_cxt->channel4.addr_phy[i];
        prev_cxt->channel4.frm[i].addr_phy.addr_u =
            prev_cxt->channel4.frm[i].addr_phy.addr_y + width * height;
        prev_cxt->channel4.frm[i].fd = prev_cxt->channel4.fd[i];

        buffer->addr[i].addr_y = prev_cxt->channel4.frm[i].addr_phy.addr_y;
        buffer->addr[i].addr_u = prev_cxt->channel4.frm[i].addr_phy.addr_u;
        buffer->addr_vir[i].addr_y = prev_cxt->channel4.frm[i].addr_vir.addr_y;
        buffer->addr_vir[i].addr_u = prev_cxt->channel4.frm[i].addr_vir.addr_u;
        buffer->fd[i] = prev_cxt->channel4.frm[i].fd;
    }

    prev_cxt->channel4.frm_reserved.fmt = prev_cxt->prev_param.channel4_fmt;
    prev_cxt->channel4.frm_reserved.buf_size = prev_cxt->channel4.buf_size;
    prev_cxt->channel4.frm_reserved.size.width = width;
    prev_cxt->channel4.frm_reserved.size.height = height;
    //prev_cxt->channel4.frm_reserved.addr_vir.addr_y =
        //prev_cxt->channel4.frm_reserved.addr_vir.addr_y;
    prev_cxt->channel4.frm_reserved.addr_vir.addr_u =
        prev_cxt->channel4.frm_reserved.addr_vir.addr_y + width * height;
    //prev_cxt->channel4.frm_reserved.addr_phy.addr_y =
        //prev_cxt->channel4.frm_reserved.addr_phy.addr_y;
    prev_cxt->channel4.frm_reserved.addr_phy.addr_u =
        prev_cxt->channel4.frm_reserved.addr_phy.addr_y + width * height;
    //prev_cxt->channel4.frm_reserved.fd = prev_cxt->channel4.frm_reserved.fd;

    if (prev_cxt->prev_param.channel4_rot_angle) {
        for (i = 0; i < CHANNEL4_BUF_CNT_ROT; i++) {
            prev_cxt->channel4.rot_frm[i].fmt =
                prev_cxt->prev_param.channel4_fmt;
            prev_cxt->channel4.rot_frm[i].buf_size =
                prev_cxt->channel4.buf_size;
            prev_cxt->channel4.rot_frm[i].size.width =
                prev_cxt->channel4.size.width;
            prev_cxt->channel4.rot_frm[i].size.height =
                prev_cxt->channel4.size.height;
            prev_cxt->channel4.rot_frm[i].addr_vir.addr_y =
                prev_cxt->channel4.addr_vir[rot_frm_start_num + i];
            prev_cxt->channel4.rot_frm[i].addr_vir.addr_u =
                prev_cxt->channel4.rot_frm[i].addr_vir.addr_y + width * height;
            prev_cxt->channel4.rot_frm[i].addr_phy.addr_y =
                prev_cxt->channel4.addr_phy[rot_frm_start_num + i];
            prev_cxt->channel4.rot_frm[i].addr_phy.addr_u =
                prev_cxt->channel4.rot_frm[i].addr_phy.addr_y + width * height;
            prev_cxt->channel4.rot_frm[i].fd =
                prev_cxt->channel4.fd[rot_frm_start_num + i];
        }
    }

    ATRACE_END();
    return ret;
}

cmr_int channel4_free_bufs(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct memory_param *mem_ops = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    mem_ops = &prev_cxt->prev_param.memory_setting;

    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if (!is_restart) {
        mem_ops->free_mem(CAMERA_CHANNEL_4, handle->oem_handle,
                          prev_cxt->channel4.addr_phy,
                          prev_cxt->channel4.addr_vir, prev_cxt->channel4.fd,
                          prev_cxt->channel4.buf_cnt);
        cmr_bzero(prev_cxt->channel4.addr_phy,
                  (CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel4.addr_vir,
                  (CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT) *
                      sizeof(unsigned long));
        cmr_bzero(prev_cxt->channel4.fd,
                  (CHANNEL4_BUF_CNT + CHANNEL4_BUF_CNT_ROT) * sizeof(cmr_s32));

        mem_ops->free_mem(
            CAMERA_CHANNEL_4_RESERVED, handle->oem_handle,
            (cmr_uint *)prev_cxt->channel4.frm_reserved.addr_phy.addr_y,
            (cmr_uint *)prev_cxt->channel4.frm_reserved.addr_vir.addr_y,
            &prev_cxt->channel4.frm_reserved.fd, 1);

        prev_cxt->channel4.frm_reserved.addr_phy.addr_y = 0;
        prev_cxt->channel4.frm_reserved.addr_vir.addr_y = 0;
        prev_cxt->channel4.frm_reserved.fd = 0;
    }

    return ret;
}

cmr_int channel4_configure(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart,
                           struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    struct img_size trim_sz;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&endian, sizeof(struct img_data_end));
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    cmr_bzero(&prev_cxt->channel4, sizeof(channel4_t));

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel4_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    prev_cxt->channel4.size.width = prev_cxt->channel4_actual_pic_size.width;
    prev_cxt->channel4.size.height = prev_cxt->channel4_actual_pic_size.height;
    prev_cxt->channel4.frm_cnt = 0;
    prev_cxt->channel4.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel4.skip_mode = IMG_SKIP_SW_KER;

    chn_param.is_lightly = 0;
    chn_param.frm_num = -1;
    chn_param.sensor_mode = prev_cxt->channel4_work_mode;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel4_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.flip_on =
        handle->prev_cxt[camera_id].prev_param.channel4_flip_on;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.sence_mode =
        DCAM_SCENE_MODE_CAPTURE_CALLBACK; // TBD
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    } else {
        chn_param.cap_inf_cfg.cfg.need_isp = 0;
    }

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel4.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel4.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    trim_sz.width = sensor_mode_info->scaler_trim.width;
    trim_sz.height = sensor_mode_info->scaler_trim.height;
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel4.size.width /
                             prev_cxt->channel4.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel4_rot_angle);
        if (ret) {
            CMR_LOGE("get trim failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    ret = channel4_alloc_bufs(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc prev buf failed");
        goto exit;
    }

    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    prev_cxt->channel4.chn_id = channel_id;
    prev_cxt->channel4.chn_status = PREV_CHN_BUSY;
    prev_cxt->channel4.endian = endian;
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /* skip frame in dcam driver */
    if (prev_cxt->channel4.skip_mode == IMG_SKIP_SW_KER) {
        for (i = 0; i < prev_cxt->channel4.skip_num; i++) {
            cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
            buf_cfg.channel_id = prev_cxt->channel4.chn_id;
            buf_cfg.base_id = CMR_VIDEO_ID_BASE;
            buf_cfg.count = 1;
            buf_cfg.length = prev_cxt->channel4.frm_reserved.buf_size;
            buf_cfg.is_reserved_buf = 0;
            buf_cfg.flag = BUF_FLAG_INIT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->channel4.frm_reserved.addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->channel4.frm_reserved.addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->channel4.frm_reserved.addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->channel4.frm_reserved.addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->channel4.frm_reserved.fd;
            buf_cfg.frame_number = 0xFFFFFFFF;
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
            if (ret) {
                CMR_LOGE("channel buff config failed");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    chn_param.buffer.channel_id = prev_cxt->channel4.chn_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->channel4.chn_id;
    buf_cfg.base_id = CMR_VIDEO_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel4.frm_reserved.buf_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->channel4.frm_reserved.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->channel4.frm_reserved.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y =
        prev_cxt->channel4.frm_reserved.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u =
        prev_cxt->channel4.frm_reserved.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->channel4.frm_reserved.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (out_param_ptr) {
        out_param_ptr->channel4_chn_id = prev_cxt->channel4.chn_id;
        out_param_ptr->channel4_sn_mode = prev_cxt->channel4_work_mode;
        out_param_ptr->channel4_endian = prev_cxt->channel4.endian;
        out_param_ptr->channel4_size = prev_cxt->channel4.size;
    }

exit:
    ATRACE_END();
    return ret;
}

cmr_int channel4_update_params(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));

    prev_cxt = &handle->prev_cxt[camera_id];

    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->channel4_work_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    //	cmr_bzero(prev_cxt->prev_rot_frm_is_lock, PREV_ROT_FRM_CNT *
    // sizeof(cmr_uint));
    prev_cxt->prev_rot_index = 0;
    prev_cxt->channel4.skip_num = sensor_info->preview_skip_num; // TBD
    prev_cxt->channel4.skip_mode = IMG_SKIP_SW_KER;
    chn_param.sensor_mode = prev_cxt->channel4_work_mode;
    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.channel4_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;

    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format)
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    else
        chn_param.cap_inf_cfg.cfg.need_isp = 0;

    chn_param.cap_inf_cfg.cfg.dst_img_size.width =
        prev_cxt->channel4.size.width;
    chn_param.cap_inf_cfg.cfg.dst_img_size.height =
        prev_cxt->channel4.size.height;
    chn_param.cap_inf_cfg.cfg.notice_slice_height =
        chn_param.cap_inf_cfg.cfg.dst_img_size.height;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
        sensor_mode_info->scaler_trim.start_x;
    chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
        sensor_mode_info->scaler_trim.start_y;
    chn_param.cap_inf_cfg.cfg.src_img_rect.width =
        sensor_mode_info->scaler_trim.width;
    chn_param.cap_inf_cfg.cfg.src_img_rect.height =
        sensor_mode_info->scaler_trim.height;

    CMR_LOGD("src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   zoom_param->zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
    } else {
        float aspect_ratio = 1.0 * prev_cxt->channel4.size.width /
                             prev_cxt->channel4.size.height;
        ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    zoom_param->zoom_info.zoom_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.channel4_rot_angle);
    }
    if (ret) {
        CMR_LOGE("prev get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("after src_img_rect %d %d %d %d",
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
             chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
             chn_param.cap_inf_cfg.cfg.src_img_rect.width,
             chn_param.cap_inf_cfg.cfg.src_img_rect.height);

    channel_id = prev_cxt->channel4.chn_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    return ret;
}

cmr_s32 channel4_queue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_num = 0;
    cmr_u32 width, height;
    struct buffer_cfg buf_cfg;
    cmr_u32 rot_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));

    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->channel4.valid_buf_cnt;

    if (valid_num >= CHANNEL4_BUF_CNT) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    width = prev_cxt->channel4.size.width;
    height = prev_cxt->channel4.size.height;

    prev_cxt->channel4.fd[valid_num] = buffer->fd;
    prev_cxt->channel4.addr_phy[valid_num] = (unsigned long)buffer->addr_phy;
    prev_cxt->channel4.addr_vir[valid_num] = (unsigned long)buffer->addr_vir;

    prev_cxt->channel4.frm[valid_num].fmt = prev_cxt->prev_param.channel4_fmt;
    prev_cxt->channel4.frm[valid_num].buf_size = prev_cxt->channel4.buf_size;
    prev_cxt->channel4.frm[valid_num].size.width = width;
    prev_cxt->channel4.frm[valid_num].size.height = height;
    prev_cxt->channel4.frm[valid_num].fd = prev_cxt->channel4.fd[valid_num];
    prev_cxt->channel4.frm[valid_num].addr_phy.addr_y =
        prev_cxt->channel4.addr_phy[valid_num];
    prev_cxt->channel4.frm[valid_num].addr_phy.addr_u =
        prev_cxt->channel4.frm[valid_num].addr_phy.addr_y + width * height;
    prev_cxt->channel4.frm[valid_num].addr_vir.addr_y =
        prev_cxt->channel4.addr_vir[valid_num];
    prev_cxt->channel4.frm[valid_num].addr_vir.addr_u =
        prev_cxt->channel4.frm[valid_num].addr_vir.addr_y + width * height;

    prev_cxt->channel4.valid_buf_cnt++;

    buf_cfg.channel_id = prev_cxt->channel4.chn_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE; // no use, will remove it
    buf_cfg.count = 1;
    buf_cfg.length = prev_cxt->channel4.buf_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;
    buf_cfg.frame_number = buffer->frame_number;
    if (prev_cxt->prev_param.channel4_rot_angle) {
        ret = channel4_find_free_rot_buffer(prev_cxt, &rot_index);
        if (ret) {
            CMR_LOGE("prev_search_rot_buffer failed");
            goto exit;
        }

        buf_cfg.fd[0] = prev_cxt->channel4.rot_frm[rot_index].fd;
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel4.rot_frm[rot_index].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel4.rot_frm[rot_index].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel4.rot_frm[rot_index].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel4.rot_frm[rot_index].addr_vir.addr_u;

        prev_cxt->channel4.rot_frm_lock_flag[rot_index] = 1;
    } else {
        buf_cfg.addr[0].addr_y =
            prev_cxt->channel4.frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->channel4.frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->channel4.frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->channel4.frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->channel4.frm[valid_num].fd;
    }

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d",
             prev_cxt->camera_id, prev_cxt->channel4.frm[valid_num].fd,
             prev_cxt->channel4.chn_id, prev_cxt->channel4.valid_buf_cnt);
    ATRACE_END();
    return ret;
}

int channel4_dequeue_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *info) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    int frm_index = 0;
    cmr_u32 prev_num = 0;
    cmr_u32 prev_chn_id = 0, channel4_chn_id = 0;
    cmr_u32 cap_chn_id = 0;
    cmr_u32 prev_rot = 0, channel4_rot_angle = 0;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 valid_buf_cnt;
    struct prev_cb_info cb_data_info;
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;
    int rot_index;
    struct camera_frame_type frame_type;
    int i;

    if (!handle || !info) {
        CMR_LOGE("Invalid param! 0x%p, 0x%p", handle, info);
        ret = CMR_CAMERA_FAIL;
        return ret;
    }

    channel4_chn_id = handle->prev_cxt[camera_id].channel4.chn_id;
    channel4_rot_angle =
        handle->prev_cxt[camera_id].prev_param.channel4_rot_angle;
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->channel4.frm_cnt == 0) {
        /*response*/
        cb_data_info.cb_type = PREVIEW_RSP_CB_SUCCESS;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }
    prev_cxt->channel4.frm_cnt++;

    if (info->channel_id != channel4_chn_id) {
        CMR_LOGE("ignored, channel id %d", info->channel_id);
        goto exit;
    }

    if (prev_cxt->channel4.valid_buf_cnt <= 0) {
        ret = -1;
        CMR_LOGE("chn4 valid_buf_cnt=%d", prev_cxt->channel4.valid_buf_cnt);
        goto exit;
    }

    // no rotation
    if (channel4_rot_angle == 0) {
        if (prev_cxt->channel4.frm[0].fd != (cmr_s32)info->fd) {
            ret = -1;
            CMR_LOGE("frame sequence error from kernel driver: info->fd=0x%x, "
                     "prev_cxt->channel4.frm[0].fd=0x%x",
                     info->fd, prev_cxt->channel4.frm[0].fd);
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel4.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel4.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel4.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel4.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel4.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel4.size.width;
        frame_type.height = prev_cxt->channel4.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL4_FRAME;

        valid_buf_cnt = prev_cxt->channel4.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel4.fd[i] = prev_cxt->channel4.fd[i + 1];
            prev_cxt->channel4.addr_phy[i] = prev_cxt->channel4.addr_phy[i + i];
            prev_cxt->channel4.addr_vir[i] = prev_cxt->channel4.addr_vir[i + 1];
            prev_cxt->channel4.frm[i] = prev_cxt->channel4.frm[i + 1];
        }

        prev_cxt->channel4.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel4.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel4.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel4.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));
    } else {
        // rotation case
        ret = get_frame_index(prev_cxt->channel4.rot_frm, CHANNEL4_BUF_CNT_ROT,
                              info, &rot_index);
        if (ret) {
            CMR_LOGE("frame is not match");
            goto exit;
        }

        rot_param.src_img = &prev_cxt->channel4.rot_frm[rot_index];
        rot_param.src_img->data_end = prev_cxt->channel4.endian;
        rot_param.dst_img = &prev_cxt->channel4.frm[0];
        rot_param.dst_img->data_end = prev_cxt->channel4.endian;
        rot_param.angle = channel4_rot_angle;
        op_mean.rot = channel4_rot_angle;
        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        frame_type.buf_id = 0;
        frame_type.order_buf_id = 0;
        frame_type.y_vir_addr = prev_cxt->channel4.frm[0].addr_vir.addr_y;
        frame_type.uv_vir_addr = prev_cxt->channel4.frm[0].addr_vir.addr_u;
        frame_type.fd = prev_cxt->channel4.frm[0].fd;
        frame_type.y_phy_addr = prev_cxt->channel4.frm[0].addr_phy.addr_y;
        frame_type.uv_phy_addr = prev_cxt->channel4.frm[0].addr_phy.addr_u;
        frame_type.width = prev_cxt->channel4.size.width;
        frame_type.height = prev_cxt->channel4.size.height;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
        frame_type.type = CHANNEL4_FRAME;

        valid_buf_cnt = prev_cxt->channel4.valid_buf_cnt;
        for (i = 0; i < (int)valid_buf_cnt - 1; i++) {
            prev_cxt->channel4.fd[i] = prev_cxt->channel4.fd[i + 1];
            prev_cxt->channel4.addr_phy[i] = prev_cxt->channel4.addr_phy[i + i];
            prev_cxt->channel4.addr_vir[i] = prev_cxt->channel4.addr_vir[i + 1];
            prev_cxt->channel4.frm[i] = prev_cxt->channel4.frm[i + 1];
        }

        prev_cxt->channel4.fd[valid_buf_cnt - 1] = 0;
        prev_cxt->channel4.addr_phy[valid_buf_cnt - 1] = 0;
        prev_cxt->channel4.addr_vir[valid_buf_cnt - 1] = 0;
        cmr_bzero(&prev_cxt->channel4.frm[valid_buf_cnt - 1],
                  sizeof(struct img_frm));

        prev_cxt->channel4.rot_frm_lock_flag[rot_index] = 0;
    }

    prev_cxt->channel4.valid_buf_cnt--;

    cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%d, frame_index=%d",
             prev_cxt->camera_id, frame_type.fd, info->channel_id,
             prev_cxt->channel4.valid_buf_cnt, info->frame_real_id);

exit:
    return ret;
}

cmr_s32 channel4_find_free_rot_buffer(struct prev_context *prev_cxt,
                                      cmr_u32 *index) {
    cmr_s32 ret = -CMR_CAMERA_FAIL;
    cmr_u32 search_index;
    cmr_u32 i = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->channel4.rot_index;

    for (i = 0; i < CHANNEL4_BUF_CNT_ROT; i++) {
        search_index += i;
        search_index %= CHANNEL4_BUF_CNT_ROT;
        if (0 == prev_cxt->channel4.rot_frm_lock_flag[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->channel4.rot_index = search_index;
            *index = search_index;
            CMR_LOGD("find %d", search_index);
            break;
        }
    }

    return ret;
}

int cmr_channel4_queue_buffer(cmr_handle preview_handle, cmr_u32 camera_id,
                              cam_buffer_info_t buffer) {
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct internal_param *inter_param = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    inter_param =
        (struct internal_param *)malloc(sizeof(struct internal_param));
    if (!inter_param) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(inter_param, sizeof(struct internal_param));
    inter_param->param1 = (void *)((unsigned long)camera_id);
    inter_param->param2 = (void *)&buffer;

    message.msg_type = PREV_EVT_CHANNEL4_QUEUE_BUFFER;
    message.sync_flag = CMR_MSG_SYNC_PROCESSED;
    message.data = (void *)inter_param;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (inter_param) {
            free(inter_param);
            inter_param = NULL;
        }
    }

    CMR_LOGV("X");
    return ret;
}

cmr_int prev_set_cap_param(struct prev_handle *handle, cmr_u32 camera_id,
                           cmr_u32 is_restart, cmr_u32 is_lightly,
                           struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    struct img_data_end endian;
    struct cmr_path_capability capability;
    struct buffer_cfg buf_cfg;
    struct camera_context *cxt = NULL;
    struct setting_cmd_parameter setting_param = {0};
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));

    CMR_LOGD("cam_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    if (!is_restart) {
        prev_cxt->cap_frm_cnt = 0;
        prev_cxt->cap_zsl_frm_cnt = 0;
        cmr_bzero(prev_cxt->cap_zsl_ultra_wide_frm_is_lock,
                  ZSL_ULTRA_WIDE_ALLOC_CNT * sizeof(cmr_uint));
    }

    chn_param.is_lightly = is_lightly;
    chn_param.sensor_mode = prev_cxt->cap_mode;
    prev_cxt->skip_mode = IMG_SKIP_SW_KER;
    prev_cxt->cap_skip_num = sensor_info->capture_skip_num;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;

    cxt = (struct camera_context *)handle->oem_handle;
    setting_param.camera_id = camera_id;
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
        CAMERA_PARAM_SPRD_SUPER_MACROPHOTO_PARAM, &setting_param);
    prev_cxt->is_super = setting_param.cmd_type_value;
    CMR_LOGI("preview_eb %d , snapshot_eb %d, frame_ctrl %d, frame_count %d, "
             "is_restart %d, is super %d",
             prev_cxt->prev_param.preview_eb, prev_cxt->prev_param.snapshot_eb,
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             is_restart, prev_cxt->is_super);

    chn_param.frm_num = -1;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = prev_cxt->cap_skip_num;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_CAPTURE;

    prev_cxt->capture_scene_mode = chn_param.cap_inf_cfg.cfg.sence_mode;
    // config 4in1 flag
    if (check_software_remosaic(prev_cxt))
        chn_param.cap_inf_cfg.cfg.need_4in1 = 1;
    /*config capture ability*/
    ret = prev_cap_ability(handle, camera_id, &prev_cxt->actual_pic_size,
                           &chn_param.cap_inf_cfg.cfg);
    if (ret) {
        CMR_LOGE("calc ability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id,
                          chn_param.cap_inf_cfg.chn_deci_factor,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc capture buffer*/
    ret = prev_alloc_cap_buf(handle, camera_id, is_restart, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc cap buf failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (!handle->ops.channel_path_capability) {
        CMR_LOGE("ops channel_path_capability is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (prev_cxt->prev_param.sprd_zsl_enabled == 1) {
        ret = prev_alloc_zsl_buf(handle, camera_id, is_restart,
                                 &chn_param.buffer);
        if (ret) {
            CMR_LOGE("alloc zsl buf failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }
    if (check_software_remosaic(prev_cxt) && prev_cxt->cap_4in1_mem_num == 0) {
        ret = prev_alloc_4in1_buf(handle, camera_id, is_restart);
        if (ret) {
            CMR_LOGE("alloc 4in1 buf failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }
    ret = prev_alloc_cap_reserve_buf(handle, camera_id, is_restart);
    if (ret) {
        CMR_LOGE("alloc cap reserve buf failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = handle->ops.channel_path_capability(handle->oem_handle, &capability);
    if (ret) {
        CMR_LOGE("channel_path_capability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (capability.yuv_available_cnt && !chn_param.is_lightly &&
        prev_cxt->prev_param.video_snapshot_type != VIDEO_SNAPSHOT_VIDEO) {
        /*config channel*/
        if (!handle->ops.channel_cfg) {
            CMR_LOGE("ops channel_cfg is null");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        if ((handle->prev_cxt[camera_id].prev_param.flip_on) &&
            (1 == camera_id)) {
            chn_param.cap_inf_cfg.cfg.flip_on =
                handle->prev_cxt[camera_id].prev_param.flip_on;
        } else {
            chn_param.cap_inf_cfg.cfg.flip_on = 0;
        }
        CMR_LOGD("channel config flip:%d", chn_param.cap_inf_cfg.cfg.flip_on);

        ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                      &chn_param, &channel_id, &endian);
        if (ret) {
            CMR_LOGE("channel config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->cap_channel_id = channel_id;
        CMR_LOGD("cap chn id is %ld", prev_cxt->cap_channel_id);
        if (chn_param.cap_inf_cfg.cfg.sence_mode == DCAM_SCENE_MODE_CAPTURE) {
            prev_cxt->cap_channel_status = PREV_CHN_BUSY;
        } else if (chn_param.cap_inf_cfg.cfg.sence_mode ==
                   DCAM_SCENE_MODE_CAPTURE_CALLBACK) {
            prev_cxt->zsl_channel_status = PREV_CHN_BUSY;
        }

        prev_cxt->cap_data_endian = endian;

        /* for capture, not skip frame for now, for cts
         * testMandatoryOutputCombinations issue */
        if ((prev_cxt->skip_mode == IMG_SKIP_SW_KER) && 0) {
            /*config skip num buffer*/
            for (i = 0; i < prev_cxt->cap_skip_num; i++) {
                cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
                buf_cfg.channel_id = prev_cxt->cap_channel_id;
                if (prev_cxt->prev_param.sprd_zsl_enabled == 1) {
                    buf_cfg.base_id = CMR_CAP1_ID_BASE;
                } else {
                    buf_cfg.base_id = CMR_CAP0_ID_BASE;
                }
                buf_cfg.count = 1;
                buf_cfg.length = prev_cxt->cap_zsl_mem_size;
                buf_cfg.is_reserved_buf = 0;
                buf_cfg.flag = BUF_FLAG_INIT;
                buf_cfg.addr[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y;
                buf_cfg.addr[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u;
                buf_cfg.addr_vir[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y;
                buf_cfg.addr_vir[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u;
                buf_cfg.fd[0] = prev_cxt->cap_zsl_reserved_frm.fd;
                buf_cfg.frame_number = 0xFFFFFFFF;
                ret =
                    handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
                if (ret) {
                    CMR_LOGE("channel buff config failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
        }

        chn_param.buffer.channel_id = prev_cxt->cap_channel_id;
        ret =
            handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        /*config reserved buffer*/
        cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
        buf_cfg.channel_id = prev_cxt->cap_channel_id;
        if (prev_cxt->prev_param.sprd_zsl_enabled == 1) {
            buf_cfg.base_id = CMR_CAP1_ID_BASE;
        } else {
            buf_cfg.base_id = CMR_CAP0_ID_BASE;
        }
        buf_cfg.count = 1;
        buf_cfg.length = prev_cxt->cap_zsl_mem_size;
        buf_cfg.is_reserved_buf = 1;
        buf_cfg.flag = BUF_FLAG_INIT;
        buf_cfg.addr[0].addr_y = prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y;
        buf_cfg.addr[0].addr_u = prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->cap_zsl_reserved_frm.fd;
        buf_cfg.frame_number = 0xFFFFFFFF;
        ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    } else {
        if (!prev_cxt->prev_param.sprd_zsl_enabled) {
            prev_cxt->cap_channel_id = CHN_MAX;
            prev_cxt->cap_data_endian = prev_cxt->video_data_endian;
        }
    }

    if (check_software_remosaic(prev_cxt)) {
        CMR_LOGD("4in1 buffer cfg, channel id = %d", prev_cxt->cap_channel_id);
        cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
        buf_cfg.channel_id = prev_cxt->cap_channel_id;
        buf_cfg.count = prev_cxt->cap_4in1_mem_num;
        buf_cfg.length = (cmr_u32)prev_cxt->cap_4in1_mem_size;
        buf_cfg.is_4in1 = 1;
        for (cmr_u32 i = 0; i < buf_cfg.count; i++) {
            buf_cfg.addr[i].addr_y = prev_cxt->cap_4in1_phys_addr_array[i];
            buf_cfg.addr_vir[i].addr_y = prev_cxt->cap_4in1_virt_addr_array[i];
            buf_cfg.fd[i] = prev_cxt->cap_4in1_fd_array[i];
        }
        ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    }

    /*save channel start param for restart*/
    cmr_copy(&prev_cxt->restart_chn_param, &chn_param,
             sizeof(struct channel_start_param));

    /*start isp*/
    CMR_LOGD("need_isp %d, isp_status %ld", chn_param.cap_inf_cfg.cfg.need_isp,
             prev_cxt->isp_status);
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /*return capture out params*/
    if (out_param_ptr) {
        out_param_ptr->snapshot_chn_id = prev_cxt->cap_channel_id;
        out_param_ptr->snapshot_sn_mode = prev_cxt->cap_mode;
        out_param_ptr->snapshot_data_endian = prev_cxt->cap_data_endian;
        out_param_ptr->actual_snapshot_size = prev_cxt->actual_pic_size;
        if (prev_cxt->prev_param.sprd_zsl_enabled == 1) {
            out_param_ptr->zsl_frame = 1;
            prev_cxt->is_zsl_frm = 1;
        } else {
            out_param_ptr->zsl_frame = 0;
            prev_cxt->is_zsl_frm = 0;
        }

        CMR_LOGI("chn_id 0x%x, cap_mode %d, encode_angle %d",
                 out_param_ptr->snapshot_chn_id,
                 out_param_ptr->snapshot_sn_mode,
                 prev_cxt->prev_param.encode_angle);

        ret = prev_get_cap_post_proc_param(handle, camera_id,
                                           prev_cxt->prev_param.encode_angle,
                                           &out_param_ptr->post_proc_setting);
        if (ret) {
            CMR_LOGE("get cap post proc param failed");
        }
    }

exit:
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

static cmr_int prev_update_cap_param(struct prev_handle *handle,
                                     cmr_u32 camera_id, cmr_u32 encode_angle,
                                     struct snp_proc_param *out_param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;
    struct cmr_path_capability capability;
    struct buffer_cfg buf_cfg;

    /*for new video snapshot or zsl snapshot, update cap mem info via encode
     * angle*/
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    CMR_LOGD("preview_eb %d , snapshot_eb %d, frame_ctrl %d, frame_count "
             "%d, encode_angle %d",
             prev_cxt->prev_param.preview_eb, prev_cxt->prev_param.snapshot_eb,
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             encode_angle);

    if (prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        /*normal cap ignore this*/
        cmr_bzero(&chn_param, sizeof(struct channel_start_param));
        prev_cxt->prev_param.encode_angle = encode_angle;
        if (out_param_ptr->thumb_size.width !=
                prev_cxt->prev_param.thumb_size.width ||
            out_param_ptr->thumb_size.height !=
                prev_cxt->prev_param.thumb_size.height) {
            prev_cxt->prev_param.thumb_size = out_param_ptr->thumb_size;
        }

        /*trigger cap mem re-arrange*/
        ret = prev_alloc_cap_buf(handle, camera_id, 1, &chn_param.buffer);
        if (ret) {
            CMR_LOGE("update cap buf failed");
            ret = CMR_CAMERA_FAIL;
        }
    }
    return ret;
}

cmr_int prev_set_zsl_param_lightly(struct prev_handle *handle,
                                   cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct img_data_end endian;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("E");

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    prev_cxt = &handle->prev_cxt[camera_id];

    chn_param.sensor_mode = prev_cxt->cap_mode;
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    cmr_bzero(prev_cxt->cap_zsl_rot_frm_is_lock,
              PREV_ROT_FRM_CNT * sizeof(cmr_uint));
    /*cmr_bzero(prev_cxt->cap_zsl_ultra_wide_frm_is_lock,
              ZSL_ULTRA_WIDE_ALLOC_CNT * sizeof(cmr_uint));*/
    prev_cxt->prev_rot_index = 0;
    prev_cxt->prev_ultra_wide_index = 0;
    prev_cxt->skip_mode = IMG_SKIP_SW_KER;

    chn_param.is_lightly = 1; /*config channel lightly*/
    chn_param.frm_num = -1;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;

    chn_param.buffer.base_id = CMR_CAP1_ID_BASE;
    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = -1;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;
    chn_param.cap_inf_cfg.cfg.need_binning = 0;
    chn_param.cap_inf_cfg.cfg.need_isp = 0;
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.cap_fmt;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = prev_cxt->cap_skip_num;
    if (CAM_IMG_FMT_BAYER_MIPI_RAW == sensor_mode_info->image_format) {
        chn_param.cap_inf_cfg.cfg.need_isp = 1;
    }

    /*config capture ability*/
    ret = prev_cap_ability(handle, camera_id, &prev_cxt->actual_pic_size,
                           &chn_param.cap_inf_cfg.cfg);
    if (ret) {
        CMR_LOGE("calc ability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    channel_id = prev_cxt->cap_channel_id;
    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("returned chn id is %d", channel_id);

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_set_cap_param_raw(struct prev_handle *handle, cmr_u32 camera_id,
                               cmr_u32 is_restart,
                               struct preview_out_param *out_param_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct prev_context *prev_cxt = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    cmr_u32 channel_id = 0;
    struct channel_start_param chn_param;
    struct video_start_param video_param;
    cmr_uint is_autotest = 0;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!handle->ops.get_sensor_autotest_mode) {
        CMR_LOGE("ops get_sensor_autotest_mode null");
        return CMR_CAMERA_INVALID_PARAM;
    }

    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    cmr_bzero(&video_param, sizeof(struct video_start_param));
    CMR_LOGD("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_info = &prev_cxt->sensor_info;
    sensor_mode_info = &sensor_info->mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    ret = handle->ops.get_sensor_autotest_mode(handle->oem_handle, camera_id,
                                               &is_autotest);
    if (ret) {
        CMR_LOGE("get mode err");
    }
    if (is_autotest) {
        CMR_LOGE("0 sensor_mode->image_format =%d \n",
                 sensor_mode_info->image_format);
        CMR_LOGE("inorde to out yuv raw data ,so force set yuv to "
                 "SENSOR_IMAGE_FORMAT_RAW \n");
        sensor_mode_info->image_format = CAM_IMG_FMT_BAYER_MIPI_RAW;
        CMR_LOGE("1 sensor_mode->image_format =%d \n",
                 sensor_mode_info->image_format);
    }

    if (!is_restart) {
        prev_cxt->cap_frm_cnt = 0;
    }

    prev_cxt->cap_skip_num = sensor_info->capture_skip_num;
    chn_param.is_lightly = 0;
    chn_param.sensor_mode = prev_cxt->cap_mode;
    chn_param.skip_num = sensor_info->mipi_cap_skip_num;

    CMR_LOGD("preview_eb %d , snapshot_eb %d, frame_ctrl %d, frame_count %d, "
             "is_restart %d",
             prev_cxt->prev_param.preview_eb, prev_cxt->prev_param.snapshot_eb,
             prev_cxt->prev_param.frame_ctrl, prev_cxt->prev_param.frame_count,
             is_restart);

    chn_param.frm_num = 1;
    CMR_LOGD("frm_num 0x%x", chn_param.frm_num);

    chn_param.cap_inf_cfg.chn_deci_factor = 0;
    chn_param.cap_inf_cfg.frm_num = chn_param.frm_num;
    chn_param.cap_inf_cfg.buffer_cfg_isp = 0;

    /*config capture ability*/
    chn_param.cap_inf_cfg.cfg.dst_img_fmt = CAM_IMG_FMT_BAYER_MIPI_RAW;
    chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
    chn_param.cap_inf_cfg.cfg.need_isp_tool = 1;
    chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;
    chn_param.cap_inf_cfg.cfg.sence_mode = DCAM_SCENE_MODE_CAPTURE;
    ret = prev_cap_ability(handle, camera_id, &prev_cxt->actual_pic_size,
                           &chn_param.cap_inf_cfg.cfg);
    if (ret) {
        CMR_LOGE("calc ability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*get sensor interface info*/
    ret = prev_get_sn_inf(handle, camera_id, chn_param.skip_num,
                          &chn_param.sn_if);
    if (ret) {
        CMR_LOGE("get sn inf failed");
        goto exit;
    }

    /*alloc capture buffer*/
    ret = prev_alloc_cap_buf(handle, camera_id, 0, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("alloc cap buf failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = prev_alloc_cap_reserve_buf(handle, camera_id, is_restart);
    if (ret) {
        CMR_LOGE("alloc cap reserve buf failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config channel*/
    if (!handle->ops.channel_cfg) {
        CMR_LOGE("ops channel_cfg is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    if (check_software_remosaic(prev_cxt))
        chn_param.cap_inf_cfg.cfg.need_4in1 = 1;

    ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                  &chn_param, &channel_id, &endian);
    if (ret) {
        CMR_LOGE("channel config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    prev_cxt->cap_channel_id = channel_id;
    CMR_LOGD("cap chn id is %ld", prev_cxt->cap_channel_id);
    prev_cxt->cap_channel_status = PREV_CHN_BUSY;
    if (prev_cxt->prev_param.tool_eb) {
        prev_cxt->cap_data_endian = prev_cxt->prev_data_endian;
    } else {
        prev_cxt->cap_data_endian = endian;
    }

    chn_param.buffer.channel_id = prev_cxt->cap_channel_id;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &chn_param.buffer);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*config reserved buffer*/
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    buf_cfg.channel_id = prev_cxt->cap_channel_id;
    buf_cfg.base_id = CMR_CAP0_ID_BASE;
    buf_cfg.count = 1;
    // buf_cfg.length = prev_cxt->cap_zsl_mem_size;
    buf_cfg.length = prev_cxt->cap_zsl_reserved_frm.buf_size;
    buf_cfg.is_reserved_buf = 1;
    buf_cfg.flag = BUF_FLAG_INIT;
    buf_cfg.addr[0].addr_y = prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y;
    buf_cfg.addr[0].addr_u = prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u;
    buf_cfg.addr_vir[0].addr_y = prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y;
    buf_cfg.addr_vir[0].addr_u = prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u;
    buf_cfg.fd[0] = prev_cxt->cap_zsl_reserved_frm.fd;
    buf_cfg.frame_number = 0xFFFFFFFF;
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel buff config failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*save channel start param for restart*/
    cmr_copy(&prev_cxt->restart_chn_param, &chn_param,
             sizeof(struct channel_start_param));

    /*start isp*/
    CMR_LOGD("need_isp %d, isp_status %ld", chn_param.cap_inf_cfg.cfg.need_isp,
             prev_cxt->isp_status);
    prev_cxt->need_isp = chn_param.cap_inf_cfg.cfg.need_isp;
    prev_cxt->need_binning = chn_param.cap_inf_cfg.cfg.need_binning;

    /*return capture out params*/
    if (out_param_ptr) {
        out_param_ptr->snapshot_chn_id = prev_cxt->cap_channel_id;
        out_param_ptr->snapshot_sn_mode = prev_cxt->cap_mode;
        out_param_ptr->snapshot_data_endian = prev_cxt->cap_data_endian;
        CMR_LOGD("chn_id 0x%x, cap_mode %d", out_param_ptr->snapshot_chn_id,
                 out_param_ptr->snapshot_sn_mode);

        ret = prev_get_cap_post_proc_param(handle, camera_id,
                                           prev_cxt->prev_param.encode_angle,
                                           &out_param_ptr->post_proc_setting);
        if (ret) {
            CMR_LOGE("get cap post proc param failed");
        }
    }

exit:
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_cap_ability(struct prev_handle *handle, cmr_u32 camera_id,
                         struct img_size *cap_size,
                         struct img_frm_cap *img_cap) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 tmp_width = 0;
    struct img_size *sensor_size = NULL;
    struct img_rect *sn_trim_rect = NULL;
    struct sensor_mode_info *sn_mode_info = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    struct prev_context *prev_cxt = NULL;
    struct camera_context *cxt = NULL;
    cmr_u32 sc_factor = 0, sc_capability = 0, sc_threshold = 0;
    cmr_int zoom_post_proc = 0;
    struct img_size trim_sz;

    if (!handle || !cap_size || !img_cap) {
        CMR_LOGE("invalid param, 0x%p, 0x%p, 0x%p", handle, cap_size, img_cap);
        return CMR_CAMERA_FAIL;
    }

    CMR_LOGD("camera_id %d", camera_id);
    cxt = (struct camera_context *)handle->oem_handle;
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_size = &prev_cxt->cap_sn_size;
    sn_trim_rect = &prev_cxt->cap_sn_trim_rect;
    sn_mode_info = &prev_cxt->sensor_info.mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;
    CMR_LOGD("image_format %d, dst_img_fmt %d", sn_mode_info->image_format,
             img_cap->dst_img_fmt);
    CMR_LOGD("isp_to_dram %d", prev_cxt->prev_param.isp_to_dram);
    img_cap->need_isp = 0;
    sensor_size->width = sn_mode_info->trim_width;
    sensor_size->height = sn_mode_info->trim_height;
    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);

    switch (sn_mode_info->image_format) {
    case CAM_IMG_FMT_YUV422P:
        prev_cxt->cap_org_fmt = prev_cxt->prev_param.cap_fmt;
        if (ZOOM_POST_PROCESS == zoom_post_proc) {
            prev_cxt->cap_zoom_mode = zoom_post_proc;
            sensor_size->width = sn_mode_info->width;
            sensor_size->height = sn_mode_info->height;
        } else {
            prev_cxt->cap_zoom_mode = zoom_post_proc;
        }
        break;

    case CAM_IMG_FMT_BAYER_MIPI_RAW:
        if (CAM_IMG_FMT_BAYER_MIPI_RAW == img_cap->dst_img_fmt) {
            CMR_LOGD("Get RawData From RawRGB senosr");
            img_cap->need_isp = 0;
            prev_cxt->cap_org_fmt = CAM_IMG_FMT_BAYER_MIPI_RAW;
            prev_cxt->cap_zoom_mode = ZOOM_POST_PROCESS;
            sensor_size->width = sn_mode_info->width;
            sensor_size->height = sn_mode_info->height;
        } else {
            if (sn_mode_info->width <= prev_cxt->prev_param.isp_width_limit) {
                CMR_LOGD("Need ISP");
                img_cap->need_isp = 1;
                prev_cxt->cap_org_fmt = prev_cxt->prev_param.cap_fmt;
                if (ZOOM_POST_PROCESS == zoom_post_proc) {
                    prev_cxt->cap_zoom_mode = zoom_post_proc;
                    sensor_size->width = sn_mode_info->width;
                    sensor_size->height = sn_mode_info->height;
                } else {
                    prev_cxt->cap_zoom_mode = zoom_post_proc;
                }
            } else {
                CMR_LOGD("change to rgbraw type");
                img_cap->need_isp = 0;
                prev_cxt->cap_org_fmt = CAM_IMG_FMT_BAYER_MIPI_RAW;
                prev_cxt->cap_zoom_mode = ZOOM_POST_PROCESS;
                sensor_size->width = sn_mode_info->width;
                sensor_size->height = sn_mode_info->height;
            }
        }
        break;

    case CAM_IMG_FMT_JPEG:
        prev_cxt->cap_org_fmt = CAM_IMG_FMT_JPEG;
        prev_cxt->cap_zoom_mode = ZOOM_POST_PROCESS;
        break;

    default:
        CMR_LOGE("Unsupport sn format %d", sn_mode_info->image_format);
        return CMR_CAMERA_NO_SUPPORT;
        break;
    }

    CMR_LOGD("sn_image_format %d, dst_img_fmt %d,"
             "cap_org_fmt %ld, cap_zoom_mode %ld",
             sn_mode_info->image_format, img_cap->dst_img_fmt,
             prev_cxt->cap_org_fmt, prev_cxt->cap_zoom_mode);

    img_cap->dst_img_fmt = prev_cxt->cap_org_fmt;
    img_cap->src_img_fmt = sn_mode_info->image_format;
    img_cap->notice_slice_height = sn_mode_info->scaler_trim.height;
    img_cap->src_img_rect.start_x = sn_mode_info->scaler_trim.start_x;
    img_cap->src_img_rect.start_y = sn_mode_info->scaler_trim.start_y;
    img_cap->src_img_rect.width = sn_mode_info->scaler_trim.width;
    img_cap->src_img_rect.height = sn_mode_info->scaler_trim.height;

    CMR_LOGD("src_img_rect %d %d %d %d",
             img_cap->src_img_rect.start_x, img_cap->src_img_rect.start_y,
             img_cap->src_img_rect.width, img_cap->src_img_rect.height);

    trim_sz.width = sn_mode_info->scaler_trim.width;
    trim_sz.height = sn_mode_info->scaler_trim.height;

    /*caculate trim rect*/
    if (ZOOM_INFO != zoom_param->mode) {
        ret = camera_get_trim_rect(&img_cap->src_img_rect,
                                   zoom_param->zoom_level, &trim_sz);
    } else {
        float aspect_ratio = 1.0 * cap_size->width / cap_size->height;
        CMR_LOGD("aspect_ratio %f, is_fov_fusion %d, app_ratio %f",
            aspect_ratio, cxt->is_fov_fusion, cxt->app_ratio);
        if (PLATFORM_ID == 0x0401) {
            if (zoom_param->zoom_info.crop_region.width == 0 ||
                (cxt->is_fov_fusion == 1 && cxt->app_ratio < 2.0)) {
                CMR_LOGD("camera_get_trim_rect2");
                ret = camera_get_trim_rect2(
                            &img_cap->src_img_rect, zoom_param->zoom_info.capture_aspect_ratio,
                            aspect_ratio, sn_mode_info->scaler_trim.width,
                            sn_mode_info->scaler_trim.height, prev_cxt->prev_param.cap_rot);
            } else {
                CMR_LOGD("camera_apply_rect_and_ratio");
                img_cap->src_img_rect = camera_apply_rect_and_ratio(
                            zoom_param->zoom_info.pixel_size, zoom_param->zoom_info.crop_region,
                            img_cap->src_img_rect, aspect_ratio);
            }
        } else {
            ret = camera_get_trim_rect2(
                        &img_cap->src_img_rect, zoom_param->zoom_info.capture_aspect_ratio,
                        aspect_ratio, sn_mode_info->scaler_trim.width,
                        sn_mode_info->scaler_trim.height, prev_cxt->prev_param.cap_rot);
        }
    }

    if (ret) {
        CMR_LOGE("cap get trim failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGD("after src_img_rect %d %d %d %d", img_cap->src_img_rect.start_x,
             img_cap->src_img_rect.start_y, img_cap->src_img_rect.width,
             img_cap->src_img_rect.height);

    /*save sensor trim rect*/
    sn_trim_rect->start_x = img_cap->src_img_rect.start_x;
    sn_trim_rect->start_y = img_cap->src_img_rect.start_y;
    sn_trim_rect->width = img_cap->src_img_rect.width;
    sn_trim_rect->height = img_cap->src_img_rect.height;

    trim_sz.width = sn_trim_rect->width;
    trim_sz.height = sn_trim_rect->height;

    /*handle zoom process mode*/
    if (ZOOM_POST_PROCESS == prev_cxt->cap_zoom_mode) {
        img_cap->src_img_rect.start_x = sn_mode_info->trim_start_x;
        img_cap->src_img_rect.start_y = sn_mode_info->trim_start_y;
        img_cap->src_img_rect.width = sn_mode_info->trim_width;
        img_cap->src_img_rect.height = sn_mode_info->trim_height;
        img_cap->dst_img_size.width = sn_mode_info->trim_width;
        img_cap->dst_img_size.height = sn_mode_info->trim_height;

        if (CAM_IMG_FMT_BAYER_MIPI_RAW == prev_cxt->cap_org_fmt ||
            ZOOM_POST_PROCESS == zoom_post_proc) {
            sn_trim_rect->start_x = img_cap->src_img_rect.start_x;
            sn_trim_rect->start_y = img_cap->src_img_rect.start_y;
            sn_trim_rect->width = img_cap->src_img_rect.width;
            sn_trim_rect->height = img_cap->src_img_rect.height;
            if (ZOOM_INFO != zoom_param->mode) {
                ret = camera_get_trim_rect(sn_trim_rect, zoom_param->zoom_level,
                                           &trim_sz);
            } else {
                float aspect_ratio = 1.0 * cap_size->width / cap_size->height;
                ret = camera_get_trim_rect2(
                    sn_trim_rect, zoom_param->zoom_info.capture_aspect_ratio,
                    aspect_ratio, sn_trim_rect->width, sn_trim_rect->height,
                    prev_cxt->prev_param.cap_rot);
            }
            if (ret) {
                CMR_LOGE("Failed to get trimming window for %ld zoom level ",
                         zoom_param->zoom_level);
                goto exit;
            }
        }
    } else {
        if (handle->ops.channel_scale_capability) {
            ret = handle->ops.channel_scale_capability(
                handle->oem_handle, &sc_capability, &sc_factor, &sc_threshold);
            if (ret) {
                CMR_LOGE("ops return %ld", ret);
                goto exit;
            }
        } else {
            CMR_LOGE("ops channel_scale_capability is null");
            goto exit;
        }

        tmp_width = (cmr_u32)(sc_factor * img_cap->src_img_rect.width);
        CMR_LOGD("%d, %d, %d, %d, %d", tmp_width, img_cap->src_img_rect.width,
                 cap_size->width, cap_size->height, sc_threshold);
        if ((img_cap->src_img_rect.width >= cap_size->width ||
             cap_size->width <= sc_threshold) &&
            ZOOM_BY_CAP == prev_cxt->cap_zoom_mode) {
            /* if the out size is smaller than the in size, try to
             * use scaler on the fly
             */
            if (cap_size->width > tmp_width) {
                if (tmp_width > sc_capability) {
                    img_cap->dst_img_size.width = sc_capability;
                } else {
                    img_cap->dst_img_size.width = tmp_width;
                }
                img_cap->dst_img_size.height =
                    (cmr_u32)(img_cap->src_img_rect.height * sc_factor);
            } else {
                /* just use scaler on the fly */
                img_cap->dst_img_size.width = cap_size->width;
                img_cap->dst_img_size.height = cap_size->height;
            }
        } else {
            img_cap->dst_img_size.width = cap_size->width;
            img_cap->dst_img_size.height = cap_size->height;
        }
    }
    /* If 4in1 high resolution && zsl
     * sensor output binning size, isp hardware output binning size
     * scale up after post process
     * so cap_org_size used as the isp hardware output size, post process size
     */
    if (cxt->is_high_res_mode == 1 && cxt->zsl_enabled == 1) {
        /* for set to kernel by output_size(IOCTL),
         * and set this size to cap_org_size below
         */
        img_cap->dst_img_size.width /= 2;
        img_cap->dst_img_size.height /= 2;
    }


    /*save original cap size*/
    if (prev_cxt->prev_param.video_snapshot_type == VIDEO_SNAPSHOT_VIDEO) {
        prev_cxt->cap_org_size.width = prev_cxt->actual_video_size.width;
        prev_cxt->cap_org_size.height = prev_cxt->actual_video_size.height;
    } else {
        prev_cxt->cap_org_size.width = img_cap->dst_img_size.width;
        prev_cxt->cap_org_size.height = img_cap->dst_img_size.height;
    }
    CMR_LOGD("cap_orig_size %d %d", prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height);

exit:
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_get_scale_rect(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_u32 rot,
                            struct snp_proc_param *cap_post_proc_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 i = 0;
    cmr_int is_need_scaling = 1;
    struct img_rect rect;
    struct prev_context *prev_cxt = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct cmr_zoom_param *zoom_param = NULL;
    struct img_size trim_sz;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!cap_post_proc_param) {
        CMR_LOGE("out param is null");
        return ret;
    }
    CMR_LOGD("camera_id %d", camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    sensor_mode_info = &prev_cxt->sensor_info.mode_info[prev_cxt->cap_mode];
    zoom_param = &prev_cxt->prev_param.zoom_setting;

    CMR_LOGD("rot %d, cap_zoom_mode %ld, cap_org_size %d %d, aligned_pic_size "
             "%d %d, actual_pic_size %d %d",
             rot, prev_cxt->cap_zoom_mode, prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height, prev_cxt->aligned_pic_size.width,
             prev_cxt->aligned_pic_size.height, prev_cxt->actual_pic_size.width,
             prev_cxt->actual_pic_size.height);

    is_need_scaling = prev_is_need_scaling(handle, camera_id);
    if (is_need_scaling) {
        cap_post_proc_param->is_need_scaling = 1;
    } else {
        cap_post_proc_param->is_need_scaling = 0;
    }

    if (!cap_post_proc_param->is_need_scaling) {
        CMR_LOGD("no scaling");
        return ret;
    }

    CMR_LOGD("cap_zoom_mode %ld, is_need_scaling %ld", prev_cxt->cap_zoom_mode,
             cap_post_proc_param->is_need_scaling);

    if (ZOOM_BY_CAP == prev_cxt->cap_zoom_mode ||
        ZOOM_POST_PROCESS_WITH_TRIM == prev_cxt->cap_zoom_mode) {
        rect.start_x = 0;
        rect.start_y = 0;

        if (IMG_ANGLE_90 == rot || IMG_ANGLE_270 == rot) {
            rect.width = prev_cxt->cap_org_size.height;
            rect.height = prev_cxt->cap_org_size.width;
        } else {
            rect.width = prev_cxt->cap_org_size.width;
            rect.height = prev_cxt->cap_org_size.height;
        }
    } else {
        switch (rot) {
        case IMG_ANGLE_MIRROR:
            rect.start_x = sensor_mode_info->trim_width -
                           sensor_mode_info->scaler_trim.start_x -
                           sensor_mode_info->scaler_trim.width;
            rect.start_y = sensor_mode_info->scaler_trim.start_y;
            rect.width = sensor_mode_info->scaler_trim.width;
            rect.height = sensor_mode_info->scaler_trim.height;
            break;

        case IMG_ANGLE_90:
            rect.start_x = sensor_mode_info->trim_height -
                           sensor_mode_info->scaler_trim.start_y -
                           sensor_mode_info->scaler_trim.height;
            rect.start_y = sensor_mode_info->scaler_trim.start_x;
            rect.width = sensor_mode_info->scaler_trim.height;
            rect.height = sensor_mode_info->scaler_trim.width;
            break;

        case IMG_ANGLE_180:
            rect.start_x = sensor_mode_info->trim_width -
                           sensor_mode_info->scaler_trim.start_x -
                           sensor_mode_info->scaler_trim.width;
            rect.start_y = sensor_mode_info->trim_height -
                           sensor_mode_info->scaler_trim.start_y -
                           sensor_mode_info->scaler_trim.height;
            rect.width = sensor_mode_info->scaler_trim.width;
            rect.height = sensor_mode_info->scaler_trim.height;
            break;

        case IMG_ANGLE_270:
            rect.start_x = sensor_mode_info->scaler_trim.start_y;
            rect.start_y = sensor_mode_info->trim_width -
                           sensor_mode_info->scaler_trim.start_x -
                           sensor_mode_info->scaler_trim.width;
            rect.width = sensor_mode_info->scaler_trim.height;
            rect.height = sensor_mode_info->scaler_trim.width;
            break;

        case IMG_ANGLE_0:
        default:
            rect.start_x = sensor_mode_info->scaler_trim.start_x;
            rect.start_y = sensor_mode_info->scaler_trim.start_y;
            rect.width = sensor_mode_info->scaler_trim.width;
            rect.height = sensor_mode_info->scaler_trim.height;
            break;
        }

        CMR_LOGD("src rect %d %d %d %d", rect.start_x, rect.start_y, rect.width,
                 rect.height);

        trim_sz.width = rect.width;
        trim_sz.height = rect.height;
        /*caculate trim rect*/
        float aspect_ratio = 1.0 * prev_cxt->actual_pic_size.width /
                             prev_cxt->actual_pic_size.height;
        if (ZOOM_INFO != zoom_param->mode) {
            ret = camera_get_trim_rect(&rect, zoom_param->zoom_level, &trim_sz);
        } else {
            ret = camera_get_trim_rect2(&rect, zoom_param->zoom_info.zoom_ratio,
                                        aspect_ratio, rect.width, rect.height,
                                        IMG_ANGLE_0);
        }

        if (ret) {
            CMR_LOGE("get scale trim failed");
            return CMR_CAMERA_FAIL;
        }

        CMR_LOGD("after rect %d %d %d %d", rect.start_x, rect.start_y,
                 rect.width, rect.height);
    }

    CMR_LOGD("out rect %d %d %d %d", rect.start_x, rect.start_y, rect.width,
             rect.height);

    /*return scale rect*/
    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        cmr_copy(&cap_post_proc_param->scaler_src_rect[i], &rect,
                 sizeof(struct img_rect));
    }

    return ret;
}

static cmr_int prev_before_set_param(struct prev_handle *handle,
                                     cmr_u32 camera_id,
                                     enum preview_param_mode mode) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 sec = 0;
    cmr_u32 usec = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (mode >= PARAM_MODE_MAX) {
        CMR_LOGE("invalid mode");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    if (PREVIEWING != prev_cxt->prev_status) {
        CMR_LOGD("not in previewing, return directly");
        return CMR_CAMERA_SUCCESS; /*return directly without error*/
    }

    CMR_LOGD("mode %d, prev_status %ld, preview_eb %d, snapshot_eb %d", mode,
             prev_cxt->prev_status, prev_cxt->prev_param.preview_eb,
             prev_cxt->prev_param.snapshot_eb);

    if (PARAM_NORMAL == mode) {
        /*normal param, get timestamp to skip frame*/
        if (handle->ops.channel_get_cap_time) {
            ret = handle->ops.channel_get_cap_time(handle->oem_handle, &sec,
                                                   &usec);
        } else {
            CMR_LOGE("ops is null");
            return CMR_CAMERA_INVALID_PARAM;
        }
        prev_cxt->restart_timestamp = sec * 1000000000LL + usec * 1000;
        prev_cxt->video_restart_timestamp = prev_cxt->restart_timestamp;
    }

exit:
    return ret;
}

static cmr_int prev_after_set_param(struct prev_handle *handle,
                                    cmr_u32 camera_id,
                                    enum preview_param_mode mode,
                                    enum img_skip_mode skip_mode,
                                    cmr_u32 skip_number) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 skip_num = 0;
    cmr_u32 frm_num = 0;
    struct cmr_path_capability capability;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (mode >= PARAM_MODE_MAX) {
        CMR_LOGE("invalid mode");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD(
        "mode %d, prev_status %ld, preview_eb %d, snapshot_eb %d, video_eb %d",
        mode, prev_cxt->prev_status, prev_cxt->prev_param.preview_eb,
        prev_cxt->prev_param.snapshot_eb, prev_cxt->prev_param.video_eb);

    if (PREVIEWING != prev_cxt->prev_status) {
        return CMR_CAMERA_SUCCESS; /*directly return without error*/
    }

    prev_cxt->skip_mode = skip_mode;
    prev_cxt->prev_skip_num = skip_number;

    if (PARAM_NORMAL == mode) {
        /*normal param*/
        pthread_mutex_lock(&handle->thread_cxt.prev_mutex);
        prev_cxt->restart_skip_cnt = 0;
        prev_cxt->restart_skip_en = 1;
        prev_cxt->video_restart_skip_cnt = 0;
        prev_cxt->video_restart_skip_en = 1;
        prev_cxt->cap_zsl_restart_skip_cnt = 0;
        prev_cxt->cap_zsl_restart_skip_en = 1;
        pthread_mutex_unlock(&handle->thread_cxt.prev_mutex);
    } else {
        /*zoom*/
        ret = prev_set_prev_param_lightly(handle, camera_id);
        if (ret) {
            CMR_LOGE("failed to update prev param when previewing");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (prev_cxt->prev_param.video_eb) {
            ret = prev_set_video_param_lightly(handle, camera_id);
            if (ret) {
                CMR_LOGE("failed to update video param when previewing");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }

        if (prev_cxt->prev_param.channel1_eb) {
            ret = channel1_update_params(handle, camera_id);
            if (ret) {
                CMR_LOGE("failed to update video param when previewing");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }

        if (prev_cxt->prev_param.channel2_eb) {
            ret = channel2_update_params(handle, camera_id);
            if (ret) {
                CMR_LOGE("failed to update video param when previewing");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }

        if (prev_cxt->prev_param.channel3_eb) {
            ret = channel3_update_params(handle, camera_id);
            if (ret) {
                CMR_LOGE("failed to update video param when previewing");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }

        if (prev_cxt->prev_param.channel4_eb) {
            ret = channel4_update_params(handle, camera_id);
            if (ret) {
                CMR_LOGE("failed to update video param when previewing");
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
        }

        if (prev_cxt->prev_param.preview_eb &&
            prev_cxt->prev_param.snapshot_eb) {
            if (prev_cxt->is_zsl_frm) {
                ret = prev_set_zsl_param_lightly(handle, camera_id);
                if (ret) {
                    CMR_LOGE("failed to update zsl param when previewing");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
        }
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_set_preview_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                                cam_buffer_info_t *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 width, height, buffer_size, frame_size;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;
    cmr_uint ultra_wide_index = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->prev_mem_valid_num;
    width = prev_cxt->actual_prev_size.width;
    height = prev_cxt->actual_prev_size.height;

    buffer_size = width * height;
    frame_size = prev_cxt->prev_mem_size;

    if (valid_num >= PREV_FRM_CNT || valid_num < 0) {
        CMR_LOGE("cnt error valid_num %ld", valid_num);
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    prev_cxt->prev_fd_array[valid_num] = buffer->fd;
    prev_cxt->prev_phys_addr_array[valid_num] = (unsigned long)buffer->addr_phy;
    prev_cxt->prev_virt_addr_array[valid_num] = (unsigned long)buffer->addr_vir;
    prev_cxt->prev_frm[valid_num].buf_size = frame_size;
    prev_cxt->prev_frm[valid_num].addr_vir.addr_y =
        prev_cxt->prev_virt_addr_array[valid_num];
    prev_cxt->prev_frm[valid_num].addr_vir.addr_u =
        prev_cxt->prev_frm[valid_num].addr_vir.addr_y + buffer_size;
    prev_cxt->prev_frm[valid_num].addr_phy.addr_y =
        prev_cxt->prev_phys_addr_array[valid_num];
    prev_cxt->prev_frm[valid_num].addr_phy.addr_u =
        prev_cxt->prev_frm[valid_num].addr_phy.addr_y + buffer_size;
    prev_cxt->prev_frm[valid_num].fd = prev_cxt->prev_fd_array[valid_num];

    prev_cxt->prev_frm[valid_num].fmt = prev_cxt->prev_param.preview_fmt;
    prev_cxt->prev_frm[valid_num].size.width = prev_cxt->actual_prev_size.width;
    prev_cxt->prev_frm[valid_num].size.height =
        prev_cxt->actual_prev_size.height;
    prev_cxt->prev_mem_valid_num++;

    buf_cfg.channel_id = prev_cxt->prev_channel_id;
    buf_cfg.base_id = CMR_PREV_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = frame_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;
    buf_cfg.frame_number = buffer->frame_number;
    if (prev_cxt->prev_param.prev_rot) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_rot_buffer(prev_cxt, CAMERA_PREVIEW)) {
            rot_index = prev_cxt->prev_rot_index % PREV_ROT_FRM_ALLOC_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->prev_rot_frm[rot_index].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->prev_rot_frm[rot_index].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->prev_rot_frm[rot_index].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->prev_rot_frm[rot_index].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->prev_rot_frm[rot_index].fd;
            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_PREVIEW, rot_index,
                                           1);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            CMR_LOGD("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                     prev_cxt->prev_rot_frm_is_lock[rot_index]);
        } else {
            CMR_LOGE("error no rot buffer");
            goto exit;
        }
    } else if (prev_cxt->prev_param.is_ultra_wide) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_ultra_wide_buffer(prev_cxt, CAMERA_PREVIEW)) {
            ultra_wide_index =
                prev_cxt->prev_ultra_wide_index % PREV_ULTRA_WIDE_ALLOC_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->prev_ultra_wide_frm[ultra_wide_index].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->prev_ultra_wide_frm[ultra_wide_index].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->prev_ultra_wide_frm[ultra_wide_index].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->prev_ultra_wide_frm[ultra_wide_index].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->prev_ultra_wide_frm[ultra_wide_index].fd;
            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_PREVIEW,
                                                  ultra_wide_index, 1);
            if (ret) {
                CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
            CMR_LOGD("ultra_wide_index %ld prev_ultra_wide_frm_is_lock %ld",
                     ultra_wide_index,
                     prev_cxt->prev_ultra_wide_frm_is_lock[ultra_wide_index]);
        } else {
            CMR_LOGE("error no ultra wide buffer");
            goto exit;
        }
    } else {
        buf_cfg.addr[0].addr_y = prev_cxt->prev_frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u = prev_cxt->prev_frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->prev_frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->prev_frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->prev_frm[valid_num].fd;
    }

#if defined(CONFIG_ISP_2_3)   //just for sharkle
    pthread_mutex_lock(&handle->thread_cxt.prev_stop_mutex);
    if (prev_cxt->prev_status !=IDLE){
        ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    }
    pthread_mutex_unlock(&handle->thread_cxt.prev_stop_mutex);
#else
        ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
#endif

    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%lx, valid_num=%ld"
             ", phy_y=0x%x,vir_y=0x%x,fmt=%d,buffer_size=0x%x",
             prev_cxt->camera_id, prev_cxt->prev_frm[valid_num].fd,
             prev_cxt->prev_channel_id, prev_cxt->prev_mem_valid_num,
             prev_cxt->prev_frm[valid_num].addr_phy.addr_y,
             prev_cxt->prev_frm[valid_num].addr_vir.addr_y,
             prev_cxt->prev_frm[valid_num].fmt,prev_cxt->prev_frm[valid_num].buf_size);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_preview_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data, cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->prev_mem_valid_num;

    if (valid_num > PREV_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if ((prev_cxt->prev_frm[0].fd == (cmr_s32)data->fd) && valid_num > 0) {
        frame_type.y_phy_addr = prev_cxt->prev_frm[0].addr_phy.addr_y;
        frame_type.y_vir_addr = prev_cxt->prev_frm[0].addr_vir.addr_y;
        frame_type.fd = prev_cxt->prev_frm[0].fd;
        frame_type.type = PREVIEW_CANCELED_FRAME;

        CMR_LOGV("fd addr 0x%x addr 0x%lx", frame_type.fd,
                 frame_type.y_vir_addr);

        for (i = 0; i < (cmr_u32)(valid_num - 1); i++) {
            prev_cxt->prev_phys_addr_array[i] =
                prev_cxt->prev_phys_addr_array[i + 1];
            prev_cxt->prev_virt_addr_array[i] =
                prev_cxt->prev_virt_addr_array[i + 1];
            prev_cxt->prev_fd_array[i] = prev_cxt->prev_fd_array[i + 1];
            memcpy(&prev_cxt->prev_frm[i], &prev_cxt->prev_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->prev_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->prev_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->prev_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->prev_frm[valid_num - 1], sizeof(struct img_frm));
        prev_cxt->prev_mem_valid_num--;

        if (is_to_hal) {
            frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000LL;
            frame_type.monoboottime = data->monoboottime;
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
    } else {
        CMR_LOGE(
            "got wrong buf: data->fd=0x%x, prev_frm[0].fd=0x%x, valid_num=%ld",
            data->fd, prev_cxt->prev_frm[0].fd, valid_num);
        return CMR_CAMERA_INVALID_FRAME;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%ld, frame_index=%d",
             prev_cxt->camera_id, data->fd, data->channel_id,
             prev_cxt->prev_mem_valid_num, data->frame_real_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_video_buffer_sw_3dnr(struct prev_handle *handle,
                                      cmr_u32 camera_id, struct frm_info *data,
                                      cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->video_mem_valid_num;

    if (valid_num > PREV_FRM_CNT || valid_num <= 0) {
        if (prev_cxt->prev_param.video_eb)
            CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }
    if (/*(prev_cxt->video_frm[0].fd == (cmr_s32)data->fd) &&*/ valid_num > 0) {
        for (i = 0; i < (cmr_u32)valid_num - 1; i++) {
            prev_cxt->video_phys_addr_array[i] =
                prev_cxt->video_phys_addr_array[i + 1];
            prev_cxt->video_virt_addr_array[i] =
                prev_cxt->video_virt_addr_array[i + 1];
            prev_cxt->video_fd_array[i] = prev_cxt->video_fd_array[i + 1];
            memcpy(&prev_cxt->video_frm[i], &prev_cxt->video_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->video_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->video_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->video_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->video_frm[valid_num - 1], sizeof(struct img_frm));
        prev_cxt->video_mem_valid_num--;
        if (is_to_hal) {
            cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
            frame_type.y_phy_addr = prev_cxt->video_phys_addr_array[0];
            frame_type.y_vir_addr = prev_cxt->video_virt_addr_array[0];
            frame_type.fd = prev_cxt->video_fd_array[0];
            frame_type.type = PREVIEW_VIDEO_CANCELED_FRAME;
            frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000LL;
            frame_type.monoboottime = data->monoboottime;
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
    } else {
        CMR_LOGE(
            "got wrong buf: data->fd=0x%x, video_frm[0].fd=0x%x, valid_num=%ld",
            data->fd, prev_cxt->video_frm[0].fd, valid_num);
        return CMR_CAMERA_INVALID_FRAME;
    }

exit:
    CMR_LOGD("fd=0x%x, chn_id=0x%x, valid_num=%ld, cam_id=%ld", data->fd,
             data->channel_id, prev_cxt->video_mem_valid_num,
             prev_cxt->camera_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_clear_preview_buffers(struct prev_handle *handle,
                                   cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->prev_mem_valid_num;

    if (valid_num > PREV_FRM_CNT || valid_num < 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }

    while (valid_num > 0) {
        frame_type.y_phy_addr = prev_cxt->prev_frm[0].addr_phy.addr_y;
        frame_type.y_vir_addr = prev_cxt->prev_frm[0].addr_vir.addr_y;
        frame_type.fd = prev_cxt->prev_frm[0].fd;
        frame_type.type = PREVIEW_CANCELED_FRAME;
        frame_type.timestamp = systemTime(CLOCK_MONOTONIC);
        frame_type.monoboottime = systemTime(SYSTEM_TIME_BOOTTIME);

        CMR_LOGI("fd=%x vir_addr=%lx", frame_type.fd, frame_type.y_vir_addr);

        for (i = 0; i < (cmr_u32)(valid_num - 1); i++) {
            prev_cxt->prev_phys_addr_array[i] =
                prev_cxt->prev_phys_addr_array[i + 1];
            prev_cxt->prev_virt_addr_array[i] =
                prev_cxt->prev_virt_addr_array[i + 1];
            prev_cxt->prev_fd_array[i] = prev_cxt->prev_fd_array[i + 1];
            memcpy(&prev_cxt->prev_frm[i], &prev_cxt->prev_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->prev_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->prev_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->prev_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->prev_frm[valid_num - 1], sizeof(struct img_frm));

        cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = &frame_type;
        prev_cb_start(handle, &cb_data_info);

        prev_cxt->prev_mem_valid_num--;
        valid_num = prev_cxt->prev_mem_valid_num;
    }

exit:
    CMR_LOGV("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_set_video_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              cam_buffer_info_t *buffer) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 width, height, buffer_size, frame_size;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;
    cmr_uint ultra_wide_index = 0;
    cmr_int i = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->video_mem_valid_num;

    if (valid_num >= PREV_FRM_CNT || valid_num < 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }

    width = prev_cxt->actual_video_size.width;
    height = prev_cxt->actual_video_size.height;

    buffer_size = width * height;
    frame_size = prev_cxt->video_mem_size;

    prev_cxt->video_fd_array[valid_num] = buffer->fd;
    prev_cxt->video_phys_addr_array[valid_num] =
        (unsigned long)buffer->addr_phy;
    prev_cxt->video_virt_addr_array[valid_num] =
        (unsigned long)buffer->addr_vir;
    prev_cxt->video_frm[valid_num].buf_size = frame_size;
    prev_cxt->video_frm[valid_num].addr_vir.addr_y =
        prev_cxt->video_virt_addr_array[valid_num];
    prev_cxt->video_frm[valid_num].addr_vir.addr_u =
        prev_cxt->video_frm[valid_num].addr_vir.addr_y + buffer_size;
    prev_cxt->video_frm[valid_num].addr_phy.addr_y =
        prev_cxt->video_phys_addr_array[valid_num];
    prev_cxt->video_frm[valid_num].addr_phy.addr_u =
        prev_cxt->video_frm[valid_num].addr_phy.addr_y + buffer_size;
    prev_cxt->video_frm[valid_num].fd = prev_cxt->video_fd_array[valid_num];
    prev_cxt->video_frm[valid_num].fmt = prev_cxt->prev_param.preview_fmt;
    prev_cxt->video_frm[valid_num].size.width =
        prev_cxt->actual_video_size.width;
    prev_cxt->video_frm[valid_num].size.height =
        prev_cxt->actual_video_size.height;
    prev_cxt->video_mem_valid_num++;

    if (prev_cxt->prev_param.video_slowmotion_eb == ISP_SLW_VIDEO) {
        prev_cxt->cache_buffer_cont++;
        if (prev_cxt->cache_buffer_cont <
                CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE ||
            prev_cxt->video_mem_valid_num <
                CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE)
            goto exit;

        buf_cfg.channel_id = prev_cxt->video_channel_id;
        buf_cfg.base_id = CMR_VIDEO_ID_BASE;
        buf_cfg.count = CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE;
        buf_cfg.length = frame_size;
        buf_cfg.flag = BUF_FLAG_RUNNING;
        buf_cfg.frame_number = buffer->frame_number;

        CMR_LOGD("camera_id = %ld, buffer_cont=%ld, valid_num=%ld",
                 prev_cxt->camera_id, prev_cxt->cache_buffer_cont,
                 prev_cxt->video_mem_valid_num);
        for (i = 0; i < CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE; i++) {
            buf_cfg.addr[i].addr_y =
                prev_cxt
                    ->video_frm[prev_cxt->video_mem_valid_num -
                                CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE + i]
                    .addr_phy.addr_y;
            buf_cfg.addr[i].addr_u =
                prev_cxt
                    ->video_frm[prev_cxt->video_mem_valid_num -
                                CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE + i]
                    .addr_phy.addr_u;
            buf_cfg.addr_vir[i].addr_y =
                prev_cxt
                    ->video_frm[prev_cxt->video_mem_valid_num -
                                CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE + i]
                    .addr_vir.addr_y;
            buf_cfg.addr_vir[i].addr_u =
                prev_cxt
                    ->video_frm[prev_cxt->video_mem_valid_num -
                                CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE + i]
                    .addr_vir.addr_u;
            buf_cfg.fd[i] =
                prev_cxt
                    ->video_frm[prev_cxt->video_mem_valid_num -
                                CAMERA_CONFIG_BUFFER_TO_KERNAL_ARRAY_SIZE + i]
                    .fd;
        }
        prev_cxt->cache_buffer_cont = 0;
    } else {
        buf_cfg.channel_id = prev_cxt->video_channel_id;
        buf_cfg.base_id = CMR_VIDEO_ID_BASE;
        buf_cfg.count = 1;
        buf_cfg.length = frame_size;
        buf_cfg.flag = BUF_FLAG_RUNNING;
        buf_cfg.frame_number = buffer->frame_number;

        if (prev_cxt->prev_param.prev_rot) {
            if (CMR_CAMERA_SUCCESS ==
                prev_search_rot_buffer(prev_cxt, CAMERA_VIDEO)) {
                rot_index = prev_cxt->video_rot_index % PREV_ROT_FRM_CNT;
                buf_cfg.addr[0].addr_y =
                    prev_cxt->video_rot_frm[rot_index].addr_phy.addr_y;
                buf_cfg.addr[0].addr_u =
                    prev_cxt->video_rot_frm[rot_index].addr_phy.addr_u;
                buf_cfg.addr_vir[0].addr_y =
                    prev_cxt->video_rot_frm[rot_index].addr_vir.addr_y;
                buf_cfg.addr_vir[0].addr_u =
                    prev_cxt->video_rot_frm[rot_index].addr_vir.addr_u;
                buf_cfg.fd[0] = prev_cxt->video_rot_frm[rot_index].fd;
                ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_VIDEO,
                                               rot_index, 1);
                if (ret) {
                    CMR_LOGE("prev_set_rot_buffer_flag failed");
                    goto exit;
                }
                CMR_LOGD("rot_index %ld video_rot_frm_is_lock %ld", rot_index,
                         prev_cxt->video_rot_frm_is_lock[rot_index]);
            } else {
                CMR_LOGE("error no rot buffer");
                goto exit;
            }
        }else if (prev_cxt->prev_param.is_ultra_wide) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_ultra_wide_buffer(prev_cxt, CAMERA_VIDEO)) {
            ultra_wide_index =
                prev_cxt->video_ultra_wide_index % VIDEO_ULTRA_WIDE_ALLOC_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->video_ultra_wide_frm[ultra_wide_index].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->video_ultra_wide_frm[ultra_wide_index].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->video_ultra_wide_frm[ultra_wide_index].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->video_ultra_wide_frm[ultra_wide_index].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->video_ultra_wide_frm[ultra_wide_index].fd;
            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_VIDEO,
                                                  ultra_wide_index, 1);
            if (ret) {
                CMR_LOGE("prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
            CMR_LOGD("ultra_wide_index %ld video_ultra_wide_frm_is_lock %ld",
                     ultra_wide_index,
                     prev_cxt->video_ultra_wide_frm_is_lock[ultra_wide_index]);
        } else {
            CMR_LOGE("error no ultra wide buffer");
            goto exit;
        }
        }else {
            buf_cfg.addr[0].addr_y =
                prev_cxt->video_frm[valid_num].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->video_frm[valid_num].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->video_frm[valid_num].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->video_frm[valid_num].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->video_frm[valid_num].fd;
        }
    }

#if defined(CONFIG_ISP_2_3)   //just for sharkle
    pthread_mutex_lock(&handle->thread_cxt.prev_stop_mutex);
    if (prev_cxt->prev_status !=IDLE) {
        if (prev_cxt->prev_param.sprd_3dnr_type != CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW)
            ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
        else {
            CMR_LOGV("skip set video buffer for CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW");
            pthread_mutex_unlock(&handle->thread_cxt.prev_stop_mutex);
            return 0 ;
        }
    } else {
        cmr_bzero(&prev_cxt->video_frm[valid_num], sizeof(struct img_frm));
    }
    pthread_mutex_unlock(&handle->thread_cxt.prev_stop_mutex);
#else
    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
#endif

    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=%ld, valid_num=%ld"
             ", phy_y=0x%x,vir_y=0x%x,fmt=%d,buffer_size=0x%x",
             prev_cxt->camera_id, prev_cxt->video_frm[valid_num].fd,
             prev_cxt->video_channel_id, prev_cxt->video_mem_valid_num,
             prev_cxt->video_frm[valid_num].addr_phy.addr_y,
             prev_cxt->video_frm[valid_num].addr_vir.addr_y,
             prev_cxt->video_frm[valid_num].fmt,prev_cxt->video_frm[valid_num].buf_size);

    ATRACE_END();
    return ret;
}

cmr_int prev_pop_video_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                              struct frm_info *data, cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->video_mem_valid_num;

    if (valid_num > PREV_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if ((prev_cxt->video_frm[0].fd == (cmr_s32)data->fd) && valid_num > 0) {
        frame_type.y_phy_addr = prev_cxt->video_phys_addr_array[0];
        frame_type.y_vir_addr = prev_cxt->video_virt_addr_array[0];
        frame_type.fd = prev_cxt->video_fd_array[0];
        frame_type.type = PREVIEW_VIDEO_CANCELED_FRAME;

        for (i = 0; i < (cmr_u32)valid_num - 1; i++) {
            prev_cxt->video_phys_addr_array[i] =
                prev_cxt->video_phys_addr_array[i + 1];
            prev_cxt->video_virt_addr_array[i] =
                prev_cxt->video_virt_addr_array[i + 1];
            prev_cxt->video_fd_array[i] = prev_cxt->video_fd_array[i + 1];
            memcpy(&prev_cxt->video_frm[i], &prev_cxt->video_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->video_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->video_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->video_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->video_frm[valid_num - 1], sizeof(struct img_frm));
        prev_cxt->video_mem_valid_num--;
        if (is_to_hal) {
            frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000LL;
            frame_type.monoboottime = data->monoboottime;
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
    } else {
        CMR_LOGE(
            "got wrong buf: data->fd=0x%x, video_frm[0].fd=0x%x, valid_num=%ld",
            data->fd, prev_cxt->video_frm[0].fd, valid_num);
        return CMR_CAMERA_INVALID_FRAME;
    }

exit:
    CMR_LOGD("cam_id=%ld, fd=0x%x, chn_id=0x%x, valid_num=%ld, frame_index=%d",
             prev_cxt->camera_id, data->fd, data->channel_id,
             prev_cxt->video_mem_valid_num, data->frame_real_id);
    ATRACE_END();
    return ret;
}

cmr_int prev_set_zsl_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            cmr_uint src_phy_addr, cmr_uint src_vir_addr,
                            cmr_s32 fd) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 width, height, buffer_size, frame_size;
    struct buffer_cfg buf_cfg;
    cmr_uint rot_index = 0;
    cmr_uint ultra_wide_index = 0;
    cmr_int zoom_post_proc = 0;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!src_vir_addr) {
        CMR_LOGE("in parm error");
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    prev_cxt = &handle->prev_cxt[camera_id];
    if (IDLE == prev_cxt->prev_status) {
        CMR_LOGD("don't need to set buffer");
        return ret;
    }
    if ((PREV_CHN_IDLE == prev_cxt->zsl_channel_status) &&
        (prev_cxt->capture_scene_mode == DCAM_SCENE_MODE_CAPTURE_CALLBACK)) {
        return ret;
    } else if ((PREV_CHN_IDLE == prev_cxt->cap_channel_status) &&
               (prev_cxt->capture_scene_mode == DCAM_SCENE_MODE_CAPTURE)) {
        return ret;
    }
    valid_num = prev_cxt->cap_zsl_mem_valid_num;
    if (ZOOM_POST_PROCESS == zoom_post_proc) {
        width = prev_cxt->cap_sn_size.width;
        height = prev_cxt->cap_sn_size.height;
    } else if (ZOOM_POST_PROCESS_WITH_TRIM == zoom_post_proc) {
        width = prev_cxt->max_size.width;
        height = prev_cxt->max_size.height;
    } else {
//        width = prev_cxt->actual_pic_size.width;
//        height = prev_cxt->actual_pic_size.height;
        width = prev_cxt->cap_org_size.width;
        height = prev_cxt->cap_org_size.height;
    }
    if (prev_cxt->is_reprocessing && cxt->is_multi_mode == MODE_3D_CAPTURE) {
        width = prev_cxt->cap_sn_size.width;
        height = prev_cxt->cap_sn_size.height;
    }
    buffer_size = width * height;
    frame_size = prev_cxt->cap_zsl_mem_size;

    prev_cxt->cap_zsl_fd_array[valid_num] = fd;
    prev_cxt->cap_zsl_phys_addr_array[valid_num] = src_phy_addr;
    prev_cxt->cap_zsl_virt_addr_array[valid_num] = src_vir_addr;
    prev_cxt->cap_zsl_frm[valid_num].buf_size = frame_size;
    prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_y =
        prev_cxt->cap_zsl_virt_addr_array[valid_num];
    prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_u =
        prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_y + buffer_size;
    prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_y =
        prev_cxt->cap_zsl_phys_addr_array[valid_num];
    prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_u =
        prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_y + buffer_size;
    prev_cxt->cap_zsl_frm[valid_num].fd = prev_cxt->cap_zsl_fd_array[valid_num];
    prev_cxt->cap_zsl_frm[valid_num].fmt = prev_cxt->cap_org_fmt;
    prev_cxt->cap_zsl_frm[valid_num].size.width = width;
    prev_cxt->cap_zsl_frm[valid_num].size.height = height;
    prev_cxt->cap_zsl_mem_valid_num++;

    buf_cfg.channel_id = prev_cxt->cap_channel_id;
    buf_cfg.base_id = CMR_CAP1_ID_BASE;
    buf_cfg.count = 1;
    buf_cfg.length = frame_size;
    buf_cfg.flag = BUF_FLAG_RUNNING;

    if (prev_cxt->prev_param.prev_rot) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_rot_buffer(prev_cxt, CAMERA_SNAPSHOT_ZSL)) {
            rot_index = prev_cxt->cap_zsl_rot_index % PREV_ROT_FRM_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->video_rot_frm[rot_index].addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->cap_zsl_rot_frm[rot_index].addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->cap_zsl_rot_frm[rot_index].addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->cap_zsl_rot_frm[rot_index].addr_vir.addr_u;
            buf_cfg.fd[0] = prev_cxt->cap_zsl_rot_frm[rot_index].fd;
            ret = prev_set_rot_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                           rot_index, 1);
            if (ret) {
                CMR_LOGE("prev_set_rot_buffer_flag failed");
                goto exit;
            }
            CMR_LOGD("rot_index %ld prev_rot_frm_is_lock %ld", rot_index,
                     prev_cxt->video_rot_frm_is_lock[rot_index]);
        } else {
            CMR_LOGE("error no rot buffer");
            goto exit;
        }
    } else if (prev_cxt->prev_param.is_ultra_wide) {
        if (CMR_CAMERA_SUCCESS ==
            prev_search_ultra_wide_buffer(prev_cxt, CAMERA_SNAPSHOT_ZSL)) {
            ultra_wide_index =
                prev_cxt->cap_zsl_ultra_wide_index % ZSL_ULTRA_WIDE_ALLOC_CNT;
            buf_cfg.addr[0].addr_y =
                prev_cxt->cap_zsl_ultra_wide_frm[ultra_wide_index]
                    .addr_phy.addr_y;
            buf_cfg.addr[0].addr_u =
                prev_cxt->cap_zsl_ultra_wide_frm[ultra_wide_index]
                    .addr_phy.addr_u;
            buf_cfg.addr_vir[0].addr_y =
                prev_cxt->cap_zsl_ultra_wide_frm[ultra_wide_index]
                    .addr_vir.addr_y;
            buf_cfg.addr_vir[0].addr_u =
                prev_cxt->cap_zsl_ultra_wide_frm[ultra_wide_index]
                    .addr_vir.addr_u;
            buf_cfg.fd[0] =
                prev_cxt->cap_zsl_ultra_wide_frm[ultra_wide_index].fd;
            ret = prev_set_ultra_wide_buffer_flag(prev_cxt, CAMERA_SNAPSHOT_ZSL,
                                                  ultra_wide_index, 1);
            if (ret) {
                CMR_LOGE("zsl prev_set_ultra_wide_buffer_flag failed");
                goto exit;
            }
            CMR_LOGD("ultra_wide_index %ld cap_zsl_ultra_wide_frm_is_lock %ld",
                     ultra_wide_index,
                     prev_cxt->prev_ultra_wide_frm_is_lock[ultra_wide_index]);
        } else {
            CMR_LOGE("error no ultra wide buffer");
            goto exit;
        }
    } else {
        buf_cfg.addr[0].addr_y =
            prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_y;
        buf_cfg.addr[0].addr_u =
            prev_cxt->cap_zsl_frm[valid_num].addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->cap_zsl_frm[valid_num].addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->cap_zsl_frm[valid_num].fd;
    }

    ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
    if (ret) {
        CMR_LOGE("channel_buff_cfg failed");
        goto exit;
    }

exit:
    CMR_LOGD("cam_id = %ld, fd=0x%x, chn_id=0x%lx, valid_num=%ld",
             prev_cxt->camera_id, prev_cxt->cap_zsl_frm[valid_num].fd,
             prev_cxt->cap_channel_id, prev_cxt->cap_zsl_mem_valid_num);
    ATRACE_END();
    return ret;
}

cmr_int prev_pop_zsl_buffer(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *data, cmr_u32 is_to_hal) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_int valid_num = 0;
    cmr_u32 i;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    if (!data) {
        CMR_LOGE("frm data is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    prev_cxt = &handle->prev_cxt[camera_id];
    valid_num = prev_cxt->cap_zsl_mem_valid_num;

    if (valid_num > ZSL_FRM_CNT || valid_num <= 0) {
        CMR_LOGE("wrong valid_num %ld", valid_num);
        return CMR_CAMERA_INVALID_PARAM;
    }

    if ((prev_cxt->cap_zsl_frm[0].fd == (cmr_s32)data->fd) && valid_num > 0) {
        frame_type.y_phy_addr = prev_cxt->cap_zsl_phys_addr_array[0];
        frame_type.y_vir_addr = prev_cxt->cap_zsl_virt_addr_array[0];
        frame_type.fd = prev_cxt->cap_zsl_fd_array[0];
        frame_type.type = PREVIEW_ZSL_CANCELED_FRAME;
        for (i = 0; i < (cmr_u32)valid_num - 1; i++) {
            prev_cxt->cap_zsl_phys_addr_array[i] =
                prev_cxt->cap_zsl_phys_addr_array[i + 1];
            prev_cxt->cap_zsl_virt_addr_array[i] =
                prev_cxt->cap_zsl_virt_addr_array[i + 1];
            prev_cxt->cap_zsl_fd_array[i] = prev_cxt->cap_zsl_fd_array[i + 1];
            memcpy(&prev_cxt->cap_zsl_frm[i], &prev_cxt->cap_zsl_frm[i + 1],
                   sizeof(struct img_frm));
        }
        prev_cxt->cap_zsl_phys_addr_array[valid_num - 1] = 0;
        prev_cxt->cap_zsl_virt_addr_array[valid_num - 1] = 0;
        prev_cxt->cap_zsl_fd_array[valid_num - 1] = 0;
        cmr_bzero(&prev_cxt->cap_zsl_frm[valid_num - 1],
                  sizeof(struct img_frm));
        prev_cxt->cap_zsl_mem_valid_num--;

        if (is_to_hal) {
            frame_type.timestamp = data->sec * 1000000000LL + data->usec * 1000LL;
            frame_type.monoboottime = data->monoboottime;
            cb_data_info.cb_type = PREVIEW_EVT_CB_FRAME;
            cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
            cb_data_info.frame_data = &frame_type;
            prev_cb_start(handle, &cb_data_info);
        }
    } else {
        CMR_LOGE("got wrong buf: data->fd=0x%x, cap_zsl_frm[0].fd=0x%x, "
                 "valid_num=%ld",
                 data->fd, prev_cxt->cap_zsl_frm[0].fd, valid_num);
        return CMR_CAMERA_INVALID_FRAME;
    }

exit:
    CMR_LOGD(
        "cam_id = %ld, fd=0x%x, chn_id=0x%x, valid_num=%ld, frame_index=%d",
        prev_cxt->camera_id, data->fd, data->channel_id,
        prev_cxt->cap_zsl_mem_valid_num, data->frame_real_id);
    ATRACE_END();
    return ret;
}

cmr_uint prev_get_rot_val(cmr_uint rot_enum) {
    cmr_uint rot_val = 0;

    switch (rot_enum) {
    case IMG_ANGLE_0:
        rot_val = 0;
        break;

    case IMG_ANGLE_90:
        rot_val = 1;
        break;

    case IMG_ANGLE_180:
        rot_val = 2;
        break;

    case IMG_ANGLE_270:
        rot_val = 3;
        break;

    default:
        CMR_LOGE("uncorrect params!");
        break;
    }

    CMR_LOGD("in angle %ld, out val %ld", rot_enum, rot_val);

    return rot_val;
}

cmr_uint prev_get_rot_enum(cmr_uint rot_val) {
    cmr_uint rot_enum = IMG_ANGLE_0;

    switch (rot_val) {
    case 0:
        rot_enum = IMG_ANGLE_0;
        break;

    case 1:
        rot_enum = IMG_ANGLE_90;
        break;

    case 2:
        rot_enum = IMG_ANGLE_180;
        break;

    case 3:
        rot_enum = IMG_ANGLE_270;
        break;

    default:
        CMR_LOGE("uncorrect params!");
        break;
    }

    CMR_LOGD("in val %ld, out enum %ld", rot_val, rot_enum);

    return rot_enum;
}

cmr_uint prev_set_rot_buffer_flag(struct prev_context *prev_cxt, cmr_uint type,
                                  cmr_int index, cmr_uint flag) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint *frm_is_lock = NULL;

    if (!prev_cxt) {
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (CAMERA_PREVIEW == type) {
            frm_is_lock = &prev_cxt->prev_rot_frm_is_lock[0];
        } else if (CAMERA_VIDEO == type) {
            frm_is_lock = &prev_cxt->video_rot_frm_is_lock[0];
        } else if (CAMERA_SNAPSHOT_ZSL == type) {
            frm_is_lock = &prev_cxt->cap_zsl_rot_frm_is_lock[0];
        } else {
            CMR_LOGW("ignored  prev_status %ld, index %ld",
                     prev_cxt->prev_status, index);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }
    if (!ret && (index >= 0 && index < PREV_ROT_FRM_ALLOC_CNT) &&
        (NULL != frm_is_lock)) {
        *(frm_is_lock + index) = flag;
    } else {
        CMR_LOGE("error index %ld", index);
        ret = CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGD("[prev_rot] done ret %ld flag %ld", ret, flag);
    return ret;
}

cmr_uint prev_set_ultra_wide_buffer_flag(struct prev_context *prev_cxt,
                                         cmr_uint type, cmr_int index,
                                         cmr_uint flag) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint *frm_is_lock = NULL;
    cmr_uint alloc_cnt = 0;
    char *debug_str = NULL;

    if (!prev_cxt) {
        return ret;
    }

    if (CAMERA_PREVIEW == type) {
        frm_is_lock = &prev_cxt->prev_ultra_wide_frm_is_lock[0];
        alloc_cnt = PREV_ULTRA_WIDE_ALLOC_CNT;
        debug_str = "preview";
    } else  if (CAMERA_VIDEO == type) {
        frm_is_lock = &prev_cxt->video_ultra_wide_frm_is_lock[0];
        alloc_cnt = VIDEO_ULTRA_WIDE_ALLOC_CNT;
        debug_str = "video";
    }else if (CAMERA_SNAPSHOT_ZSL == type) {
        frm_is_lock = &prev_cxt->cap_zsl_ultra_wide_frm_is_lock[0];
        alloc_cnt = ZSL_ULTRA_WIDE_ALLOC_CNT;
        debug_str = "ZSL";
    } else {
        debug_str = "none";
        CMR_LOGW("ignored  prev_status %ld, index %ld", prev_cxt->prev_status,
                 index);
        ret = CMR_CAMERA_INVALID_STATE;
    }

    if (!ret && (index >= 0 && index < (cmr_int)alloc_cnt) &&
        (NULL != frm_is_lock)) {
        *(frm_is_lock + index) = flag;
    } else {
        CMR_LOGE("error index %ld", index);
        ret = CMR_CAMERA_INVALID_PARAM;
    }

    CMR_LOGV("[prev_ultra_wide] %s done ret %ld flag %ld",
             debug_str, ret, flag);
    return ret;
}

cmr_uint prev_search_rot_buffer(struct prev_context *prev_cxt, cmr_uint type) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint search_index = 0;
    cmr_uint count = 0;
    cmr_uint *rot_index = NULL;
    cmr_uint *frm_is_lock = NULL;

    if (!prev_cxt) {
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (CAMERA_PREVIEW == type) {
            search_index = prev_cxt->prev_rot_index;
            rot_index = &prev_cxt->prev_rot_index;
            frm_is_lock = &prev_cxt->prev_rot_frm_is_lock[0];
        } else if (CAMERA_VIDEO == type) {
            search_index = prev_cxt->video_rot_index;
            rot_index = &prev_cxt->video_rot_index;
            frm_is_lock = &prev_cxt->video_rot_frm_is_lock[0];
        } else if (CAMERA_SNAPSHOT_ZSL == type) {
            search_index = prev_cxt->cap_zsl_rot_index;
            rot_index = &prev_cxt->cap_zsl_rot_index;
            frm_is_lock = &prev_cxt->cap_zsl_rot_frm_is_lock[0];
        } else {
            CMR_LOGW("ignored  prev_status %ld, type %ld",
                     prev_cxt->prev_status, type);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }
    if (!ret && (NULL != frm_is_lock)) {
        for (count = 0; count < PREV_ROT_FRM_ALLOC_CNT; count++) {
            search_index += count;
            search_index %= PREV_ROT_FRM_ALLOC_CNT;
            if (0 == *(frm_is_lock + search_index)) {
                *rot_index = search_index;
                CMR_LOGD("[prev_rot] find %ld", search_index);
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                ret = CMR_CAMERA_INVALID_PARAM;
                CMR_LOGV("[prev_rot] rot buffer %ld is locked", search_index);
            }
        }
    }
    /*	search_index = 0;//prev_cxt->prev_rot_index;

            for (count = 0; count < PREV_ROT_FRM_CNT; count++){
                    search_index = count;
                    //search_index %= PREV_ROT_FRM_CNT;
                    CMR_LOGD("[prev_rot] index %d lock %d",
                                    search_index,
                                    prev_cxt->prev_rot_frm_is_lock[search_index]);
                    if (!data) {
                            if (0 ==
       prev_cxt->prev_rot_frm_is_lock[search_index]) {
                                    ret = CMR_CAMERA_SUCCESS;
                                    prev_cxt->prev_rot_index = search_index;
                                    CMR_LOGD("[prev_rot] find %d",
       search_index);
                                    break;
                            } else {
                                    CMR_LOGD("[prev_rot] rot buffer %ld is
       locked", search_index);
                            }
                    } else {
                            CMR_LOGD("[prev_rot] index %d lock %d frame 0x%x
       rot_frm 0x%x",
                                    search_index,
                                    prev_cxt->prev_rot_frm_is_lock[search_index],
                                    data->yaddr,
                                    prev_cxt->prev_rot_frm[search_index].addr_phy.addr_y);
                            if (1 ==
       prev_cxt->prev_rot_frm_is_lock[search_index] && data->yaddr ==
       prev_cxt->prev_rot_frm[search_index].addr_phy.addr_y) {
                                    ret = CMR_CAMERA_SUCCESS;
                                    prev_cxt->prev_rot_index = search_index;
                                    CMR_LOGD("[prev_rot] match %d",
       search_index);
                                    break;
                            } else {
                                    CMR_LOGD("[prev_rot] no match rot buffer %ld
       is locked", search_index);
                            }
                    }
            }*/
    CMR_LOGD("[prev_rot] done ret %ld search_index %ld", ret, search_index);
    return ret;
}

cmr_uint prev_search_ultra_wide_buffer(struct prev_context *prev_cxt,
                                       cmr_uint type) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint search_index = 0;
    cmr_uint count = 0;
    cmr_uint *ultra_wide_index = NULL;
    cmr_uint *frm_is_lock = NULL;
    cmr_uint alloc_cnt = 0;
    char *debug_str = NULL;;

    if (!prev_cxt) {
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (CAMERA_PREVIEW == type) {
            search_index = prev_cxt->prev_ultra_wide_index;
            ultra_wide_index = &prev_cxt->prev_ultra_wide_index;
            frm_is_lock = &prev_cxt->prev_ultra_wide_frm_is_lock[0];
            alloc_cnt = PREV_ULTRA_WIDE_ALLOC_CNT;
            debug_str = "preview";
        } else if (CAMERA_VIDEO == type) {
            search_index = prev_cxt->video_ultra_wide_index;
            ultra_wide_index = &prev_cxt->video_ultra_wide_index;
            frm_is_lock = &prev_cxt->video_ultra_wide_frm_is_lock[0];
            alloc_cnt = VIDEO_ULTRA_WIDE_ALLOC_CNT;
            debug_str = "video";
        } else if (CAMERA_SNAPSHOT_ZSL == type) {
            search_index = prev_cxt->cap_zsl_ultra_wide_index;
            ultra_wide_index = &prev_cxt->cap_zsl_ultra_wide_index;
            frm_is_lock = &prev_cxt->cap_zsl_ultra_wide_frm_is_lock[0];
            alloc_cnt = ZSL_ULTRA_WIDE_ALLOC_CNT;
            debug_str = "ZSL";
        } else {
            debug_str = "none";
            CMR_LOGW("ignored  prev_status %ld, type %ld",
                     prev_cxt->prev_status, type);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }

    if (!ret && (NULL != frm_is_lock)) {
        for (count = 0; count < alloc_cnt; count++) {
            search_index += count;
            search_index %= alloc_cnt;
            if (0 == *(frm_is_lock + search_index)) {
                *ultra_wide_index = search_index;
                CMR_LOGV("[prev_ultra_wide] find %ld", search_index);
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                ret = CMR_CAMERA_INVALID_PARAM;
                CMR_LOGV("[prev_ultra_wide] ultra_wide buffer %ld is locked",
                         search_index);
            }
        }
    }
    CMR_LOGV("[prev_ultra_wide] %s done ret %ld search_index %ld",
             debug_str, ret, search_index);
    return ret;
}

cmr_uint prev_get_src_rot_buffer(struct prev_context *prev_cxt,
                                 struct frm_info *data, cmr_uint *index) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint count = 0;
    cmr_uint *frm_is_lock;
    struct img_frm *frm_ptr = NULL;

    if (!prev_cxt || !index) {
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (IS_PREVIEW_FRM(data->frame_id)) {
            CMR_LOGD("PREVIEW");
            frm_is_lock = &prev_cxt->prev_rot_frm_is_lock[0];
            frm_ptr = &prev_cxt->prev_rot_frm[0];
        } else if (IS_VIDEO_FRM(data->frame_id)) {
            CMR_LOGD("VIDEO");
            frm_is_lock = &prev_cxt->video_rot_frm_is_lock[0];
            frm_ptr = &prev_cxt->video_rot_frm[0];
        } else if (IS_ZSL_FRM(data->frame_id)) {
            CMR_LOGD("ZSL");
            frm_is_lock = &prev_cxt->cap_zsl_rot_frm_is_lock[0];
            frm_ptr = &prev_cxt->cap_zsl_rot_frm[0];
        } else {
            CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                     prev_cxt->prev_status, data->frame_id);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }

    if (!ret && (frm_ptr != NULL)) {
        for (count = 0; count < PREV_ROT_FRM_ALLOC_CNT; count++) {
            CMR_LOGD("[prev_rot] fd 0x%x 0x%x %ld", (frm_ptr + count)->fd,
                     data->fd, *(frm_is_lock + count));
            if (1 == *(frm_is_lock + count) &&
                (cmr_s32)data->fd == (frm_ptr + count)->fd) {
                *index = count;
                //*rot_frm = *(frm_ptr + count);
                CMR_LOGD("[prev_rot] find %ld", count);
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                ret = CMR_CAMERA_INVALID_PARAM;
                CMR_LOGV("[prev_rot] rot buffer %ld is locked", count);
            }
        }
    }

    CMR_LOGD("[prev_rot] done ret %ld count %ld", ret, count);
    return ret;
}

cmr_uint prev_get_src_ultra_wide_buffer(struct prev_context *prev_cxt,
                                        struct frm_info *data,
                                        cmr_uint *index) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_uint count = 0;
    cmr_uint *frm_is_lock;
    cmr_uint alloc_cnt = 0;
    struct img_frm *frm_ptr = NULL;

    if (!prev_cxt || !index) {
        ret = CMR_CAMERA_INVALID_PARAM;
        return ret;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        if (IS_PREVIEW_FRM(data->frame_id)) {
            frm_is_lock = &prev_cxt->prev_ultra_wide_frm_is_lock[0];
            frm_ptr = &prev_cxt->prev_ultra_wide_frm[0];
            alloc_cnt = PREV_ULTRA_WIDE_ALLOC_CNT;
        } else if (IS_VIDEO_FRM(data->frame_id)) {
            frm_is_lock = &prev_cxt->video_ultra_wide_frm_is_lock[0];
            frm_ptr = &prev_cxt->video_ultra_wide_frm[0];
            alloc_cnt = VIDEO_ULTRA_WIDE_ALLOC_CNT;
        } else if (IS_ZSL_FRM(data->frame_id)) {
            frm_is_lock = &prev_cxt->cap_zsl_ultra_wide_frm_is_lock[0];
            frm_ptr = &prev_cxt->cap_zsl_ultra_wide_frm[0];
            alloc_cnt = ZSL_ULTRA_WIDE_ALLOC_CNT;
        } else {
            CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                     prev_cxt->prev_status, data->frame_id);
            ret = CMR_CAMERA_INVALID_STATE;
        }
    }

    if (!ret && (frm_ptr != NULL)) {
        for (count = 0; count < alloc_cnt; count++) {
            CMR_LOGV("[prev_ultra_wide] fd 0x%x 0x%x %ld",
                     (frm_ptr + count)->fd, data->fd, *(frm_is_lock + count));
            if (1 == *(frm_is_lock + count) &&
                (cmr_s32)data->fd == (frm_ptr + count)->fd) {
                *index = count;
                CMR_LOGV("[prev_ultra_wide] find %ld", count);
                ret = CMR_CAMERA_SUCCESS;
                break;
            } else {
                ret = CMR_CAMERA_INVALID_PARAM;
                CMR_LOGV("[prev_ultra_wide] ultra_wide buffer %ld is locked",
                         count);
            }
        }
    }

    CMR_LOGV("[prev_ultra_wide] done ret %ld count %ld", ret, count);
    return ret;
}

cmr_uint prev_search_rot_video_buffer(struct prev_context *prev_cxt) {
    cmr_uint ret = CMR_CAMERA_FAIL;
    cmr_uint search_index;
    cmr_uint count = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->video_rot_index;

    for (count = 0; count < PREV_ROT_FRM_CNT; count++) {
        search_index += count;
        search_index %= PREV_ROT_FRM_CNT;
        if (0 == prev_cxt->video_rot_frm_is_lock[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->video_rot_index = search_index;
            CMR_LOGD("[prev_rot] find %ld", search_index);
            break;
        } else {
            CMR_LOGV("[prev_rot] rot buffer %ld is locked", search_index);
        }
    }

    return ret;
}

cmr_uint prev_search_rot_zsl_buffer(struct prev_context *prev_cxt) {
    cmr_uint ret = CMR_CAMERA_FAIL;
    cmr_uint search_index;
    cmr_uint count = 0;

    if (!prev_cxt) {
        return ret;
    }
    search_index = prev_cxt->cap_zsl_rot_index;

    for (count = 0; count < PREV_ROT_FRM_CNT; count++) {
        search_index += count;
        search_index %= PREV_ROT_FRM_CNT;
        if (0 == prev_cxt->cap_zsl_rot_frm_is_lock[search_index]) {
            ret = CMR_CAMERA_SUCCESS;
            prev_cxt->cap_zsl_rot_index = search_index;
            CMR_LOGD("[prev_rot] find %ld", search_index);
            break;
        } else {
            CMR_LOGW("[prev_rot] rot buffer %ld is locked", search_index);
        }
    }

    return ret;
}

cmr_int prev_start_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                          struct frm_info *data) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_uint rot_frm_id = 0;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;

    if (!handle->ops.start_rot) {
        CMR_LOGE("ops start_rot is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&rot_param, sizeof(struct rot_param));
    cmr_bzero(&op_mean, sizeof(struct cmr_op_mean));
    /*check preview status and frame id*/
    if (PREVIEWING == prev_cxt->prev_status) {
        ret = prev_get_src_rot_buffer(prev_cxt, data, &rot_frm_id);
        if (ret) {
            CMR_LOGE("get src rot buffer failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        if (IS_PREVIEW_FRM(data->frame_id)) {
            frm_id = data->frame_id - CMR_PREV_ID_BASE;
            rot_param.dst_img = &prev_cxt->prev_frm[frm_id];
            rot_param.src_img = &prev_cxt->prev_rot_frm[rot_frm_id];
            rot_param.src_img->data_end = prev_cxt->prev_data_endian;
            rot_param.dst_img->data_end = prev_cxt->prev_data_endian;
        } else if (IS_VIDEO_FRM(data->frame_id)) {
            frm_id = data->frame_id - CMR_VIDEO_ID_BASE;
            rot_param.dst_img = &prev_cxt->video_frm[frm_id];
            rot_param.src_img = &prev_cxt->video_rot_frm[rot_frm_id];
            rot_param.src_img->data_end = prev_cxt->video_data_endian;
            rot_param.dst_img->data_end = prev_cxt->video_data_endian;
        } else if (IS_ZSL_FRM(data->frame_id)) {
            frm_id = data->frame_id - CMR_CAP1_ID_BASE;
            rot_param.dst_img = &prev_cxt->cap_zsl_frm[frm_id];
            rot_param.src_img = &prev_cxt->cap_zsl_rot_frm[rot_frm_id];
            rot_param.src_img->data_end = prev_cxt->cap_data_endian;
            rot_param.dst_img->data_end = prev_cxt->cap_data_endian;
        } else {
            CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                     prev_cxt->prev_status, data->frame_id);
            ret = CMR_CAMERA_INVALID_STATE;
            goto exit;
        }

        rot_param.angle = prev_cxt->prev_param.prev_rot;
        ;

        CMR_LOGD("frm_id %d, rot_frm_id %ld", frm_id, rot_frm_id);

        op_mean.rot = rot_param.angle;

        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

    } else {
        CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                 prev_cxt->prev_status, data->frame_id);
        ret = CMR_CAMERA_INVALID_STATE;
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_start_video_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 rot_frm_id = 0;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;

    if (!handle->ops.start_rot) {
        CMR_LOGE("ops start_rot is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&rot_param, sizeof(struct rot_param));
    cmr_bzero(&op_mean, sizeof(struct cmr_op_mean));
    /*check preview status and frame id*/
    if (PREVIEWING == prev_cxt->prev_status && IS_VIDEO_FRM(data->frame_id)) {

        frm_id = data->frame_id - CMR_PREV_ID_BASE;
        rot_frm_id = prev_cxt->video_rot_index % PREV_ROT_FRM_CNT;

        CMR_LOGD("frm_id %d, rot_frm_id %d", frm_id, rot_frm_id);

        rot_param.angle = prev_cxt->prev_param.prev_rot;
        rot_param.src_img = &prev_cxt->video_frm[frm_id];
        rot_param.dst_img = &prev_cxt->video_rot_frm[rot_frm_id];
        rot_param.src_img->data_end = prev_cxt->video_data_endian;
        rot_param.dst_img->data_end = prev_cxt->video_data_endian;

        op_mean.rot = rot_param.angle;

        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

    } else {
        CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                 prev_cxt->prev_status, data->frame_id);
        ret = CMR_CAMERA_INVALID_STATE;
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_start_zsl_rotate(struct prev_handle *handle, cmr_u32 camera_id,
                              struct frm_info *data) {
    cmr_uint ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_u32 rot_frm_id = 0;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    struct rot_param rot_param;
    struct cmr_op_mean op_mean;

    if (!handle->ops.start_rot) {
        CMR_LOGE("ops start_rot is null");
        return CMR_CAMERA_INVALID_PARAM;
    }
    cmr_bzero(&rot_param, sizeof(struct rot_param));
    cmr_bzero(&op_mean, sizeof(struct cmr_op_mean));
    /*check preview status and frame id*/
    if (PREVIEWING == prev_cxt->prev_status && IS_ZSL_FRM(data->frame_id)) {

        frm_id = data->frame_id - CMR_PREV_ID_BASE;
        rot_frm_id = prev_cxt->cap_zsl_rot_index % PREV_ROT_FRM_CNT;

        CMR_LOGD("frm_id %d, rot_frm_id %d", frm_id, rot_frm_id);

        rot_param.angle = prev_cxt->prev_param.cap_rot;
        rot_param.src_img = &prev_cxt->cap_zsl_frm[frm_id];
        rot_param.dst_img = &prev_cxt->cap_zsl_rot_frm[rot_frm_id];
        rot_param.src_img->data_end = prev_cxt->cap_data_endian;
        rot_param.dst_img->data_end = prev_cxt->cap_data_endian;

        op_mean.rot = rot_param.angle;

        ret = handle->ops.start_rot(handle->oem_handle, (cmr_handle)handle,
                                    rot_param.src_img, rot_param.dst_img,
                                    &op_mean);
        if (ret) {
            CMR_LOGE("rot failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

    } else {
        CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                 prev_cxt->prev_status, data->frame_id);
        ret = CMR_CAMERA_INVALID_STATE;
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_get_cap_post_proc_param(struct prev_handle *handle,
                                     cmr_u32 camera_id, cmr_u32 encode_angle,
                                     struct snp_proc_param *out_param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 cap_rot = 0;
    cmr_u32 cfg_cap_rot = 0;
    cmr_u32 is_cfg_rot_cap = 0;
    cmr_u32 tmp_refer_rot = 0;
    cmr_u32 tmp_req_rot = 0;
    cmr_u32 i = 0;
    cmr_u32 is_normal_cap = 0;
    struct img_size *org_pic_size = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    if (!out_param_ptr) {
        CMR_LOGE("invalid param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    prev_cxt = &handle->prev_cxt[camera_id];
    cap_rot = prev_cxt->prev_param.cap_rot;
    is_cfg_rot_cap = prev_cxt->prev_param.is_cfg_rot_cap;
    cfg_cap_rot = encode_angle;
    org_pic_size = &prev_cxt->prev_param.picture_size;

    if (!prev_cxt->prev_param.preview_eb && prev_cxt->prev_param.snapshot_eb) {
        is_normal_cap = 1;
    } else {
        is_normal_cap = 0;
    }

    CMR_LOGD("cap_rot %d, is_cfg_rot_cap %d, cfg_cap_rot %d, is_normal_cap %d",
             cap_rot, is_cfg_rot_cap, cfg_cap_rot, is_normal_cap);

    if (is_normal_cap) {
        if ((IMG_ANGLE_0 != cap_rot) ||
            (is_cfg_rot_cap && (IMG_ANGLE_0 != cfg_cap_rot))) {

            if (IMG_ANGLE_0 != cfg_cap_rot && IMG_ANGLE_180 != cfg_cap_rot) {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.height;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.width;

                prev_cxt->dealign_actual_pic_size.width = org_pic_size->height;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->width;
            } else if (IMG_ANGLE_180 != cap_rot) {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.width;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.height;

                prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
            } else {
                CMR_LOGD("default");
            }

            CMR_LOGD(
                "now actual_pic_size %d %d,  dealign_actual_pic_size %d %d",
                prev_cxt->actual_pic_size.width,
                prev_cxt->actual_pic_size.height,
                prev_cxt->dealign_actual_pic_size.width,
                prev_cxt->dealign_actual_pic_size.height);

            tmp_req_rot = prev_get_rot_val(cfg_cap_rot);
            tmp_refer_rot = prev_get_rot_val(cap_rot);
            tmp_req_rot += tmp_refer_rot;
            if (tmp_req_rot >= IMG_ANGLE_MIRROR) {
                tmp_req_rot -= IMG_ANGLE_MIRROR;
            }
            cap_rot = prev_get_rot_enum(tmp_req_rot);
        } else {
            prev_cxt->actual_pic_size.width =
                prev_cxt->aligned_pic_size.width;
            prev_cxt->actual_pic_size.height =
                prev_cxt->aligned_pic_size.height;
            prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
            prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
        }
    } else {
        if (is_cfg_rot_cap && (IMG_ANGLE_0 != cfg_cap_rot)) {

            if (IMG_ANGLE_0 != cfg_cap_rot && IMG_ANGLE_180 != cfg_cap_rot) {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.height;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.width;

                prev_cxt->dealign_actual_pic_size.width = org_pic_size->height;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->width;
            } else {
                prev_cxt->actual_pic_size.width =
                    prev_cxt->aligned_pic_size.width;
                prev_cxt->actual_pic_size.height =
                    prev_cxt->aligned_pic_size.height;
                prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
                prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
            }

            CMR_LOGD(
                "2 now actual_pic_size %d %d,  dealign_actual_pic_size %d %d",
                prev_cxt->actual_pic_size.width,
                prev_cxt->actual_pic_size.height,
                prev_cxt->dealign_actual_pic_size.width,
                prev_cxt->dealign_actual_pic_size.height);

        } else {
            prev_cxt->actual_pic_size.width =
                prev_cxt->aligned_pic_size.width;
            prev_cxt->actual_pic_size.height =
                prev_cxt->aligned_pic_size.height;
            prev_cxt->dealign_actual_pic_size.width = org_pic_size->width;
            prev_cxt->dealign_actual_pic_size.height = org_pic_size->height;
        }
        cap_rot = cfg_cap_rot;
    }

    CMR_LOGD("now cap_rot %d", cap_rot);

    /*if (prev_cxt->prev_param.video_eb) {
            cap_rot = 0;
    }*/
    ret = prev_get_scale_rect(handle, camera_id, cap_rot, out_param_ptr);
    if (ret) {
        CMR_LOGE("get scale rect failed");
    }

    out_param_ptr->rot_angle = cap_rot;
    out_param_ptr->channel_zoom_mode = prev_cxt->cap_zoom_mode;
    out_param_ptr->snp_size = prev_cxt->aligned_pic_size;
    out_param_ptr->actual_snp_size = prev_cxt->actual_pic_size;
    out_param_ptr->cap_org_size = prev_cxt->cap_org_size;
    out_param_ptr->dealign_actual_snp_size = prev_cxt->dealign_actual_pic_size;
    out_param_ptr->max_size = prev_cxt->max_size;

    cmr_copy(&out_param_ptr->chn_out_frm[0], &prev_cxt->cap_frm[0],
             CMR_CAPTURE_MEM_SUM * sizeof(struct img_frm));

    cmr_copy(&out_param_ptr->mem[0], &prev_cxt->cap_mem[0],
             CMR_CAPTURE_MEM_SUM * sizeof(struct cmr_cap_mem));

    CMR_LOGD("rot_angle %ld, channel_zoom_mode %ld, is_need_scaling %ld",
             out_param_ptr->rot_angle, out_param_ptr->channel_zoom_mode,
             out_param_ptr->is_need_scaling);

    CMR_LOGD("cap_org_size %d, %d", prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height);

    CMR_LOGD(
        "snp_size %d %d, actual_snp_size %d %d, dealign_actual_pic_size %d %d",
        out_param_ptr->snp_size.width, out_param_ptr->snp_size.height,
        out_param_ptr->actual_snp_size.width,
        out_param_ptr->actual_snp_size.height,
        out_param_ptr->dealign_actual_snp_size.width,
        out_param_ptr->dealign_actual_snp_size.height);

    for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
        CMR_LOGD(
            "chn_out_frm[%d], format %d, size %d %d, fd 0x%x, buf_size 0x%x", i,
            out_param_ptr->chn_out_frm[i].fmt,
            out_param_ptr->chn_out_frm[i].size.width,
            out_param_ptr->chn_out_frm[i].size.height,
            out_param_ptr->chn_out_frm[i].fd,
            out_param_ptr->chn_out_frm[i].buf_size);
    }

    return ret;
}

cmr_int prev_pause_cap_channel(struct prev_handle *handle, cmr_u32 camera_id,
                               struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 snapshot_enable = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGV("E");

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.channel_pause) {
        CMR_LOGE("ops channel_start is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    CMR_LOGD("snapshot_eb %d, channel_id %d, %ld", snapshot_enable,
             data->channel_id, prev_cxt->cap_channel_id);

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
        /*pause channel*/
        ret = handle->ops.channel_pause(handle->oem_handle,
                                        prev_cxt->cap_channel_id, 1);
        if (ret) {
            CMR_LOGE("channel_pause failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        prev_cxt->cap_zsl_mem_valid_num = 0;
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_resume_cap_channel(struct prev_handle *handle, cmr_u32 camera_id,
                                struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 snapshot_enable = 0;
    struct preview_out_param out_param;
    struct buffer_cfg buf_cfg;
    cmr_u32 i;
    struct prev_cb_info cb_data_info;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGV("E");

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.channel_resume) {
        CMR_LOGE("ops channel_resume is null");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    CMR_LOGD("snapshot_eb %d, channel_id %d, %ld", snapshot_enable,
             data->channel_id, prev_cxt->cap_channel_id);

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {
        if (prev_cxt->prev_param.preview_eb &&
            prev_cxt->prev_param.snapshot_eb) {
            CMR_LOGD("do nothing");
        } else {
            if (1 != prev_cxt->prev_param.frame_count) {
                buf_cfg.channel_id = prev_cxt->cap_channel_id;
                buf_cfg.base_id = CMR_CAP0_ID_BASE;
                buf_cfg.count = CMR_CAPTURE_MEM_SUM;
                buf_cfg.flag = BUF_FLAG_INIT;
                buf_cfg.length = prev_cxt->actual_pic_size.width *
                                 prev_cxt->actual_pic_size.height * 3 / 2;
                for (i = 0; i < buf_cfg.count; i++) {
                    buf_cfg.addr[i].addr_y =
                        prev_cxt->cap_frm[i].addr_phy.addr_y;
                    buf_cfg.addr[i].addr_u =
                        prev_cxt->cap_frm[i].addr_phy.addr_u;
                    buf_cfg.addr_vir[i].addr_y =
                        prev_cxt->cap_frm[i].addr_vir.addr_y;
                    buf_cfg.addr_vir[i].addr_u =
                        prev_cxt->cap_frm[i].addr_vir.addr_u;
                    buf_cfg.fd[i] = prev_cxt->cap_frm[i].fd;
                }
                ret =
                    handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
                if (ret) {
                    CMR_LOGE("channel_buff_cfg failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
        }

        /*resume channel*/
        ret = handle->ops.channel_resume(
            handle->oem_handle, prev_cxt->cap_channel_id,
            prev_cxt->cap_skip_num, 0, prev_cxt->prev_param.frame_count);
        if (ret) {
            CMR_LOGE("channel_resume failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        cb_data_info.cb_type = PREVIEW_EVT_CB_RESUME;
        cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
        cb_data_info.frame_data = NULL;
        prev_cb_start(handle, &cb_data_info);
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_restart_cap_channel(struct prev_handle *handle, cmr_u32 camera_id,
                                 struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    cmr_u32 preview_enable = 0;
    cmr_u32 snapshot_enable = 0;
    cmr_u32 channel_id = 0;
    struct video_start_param video_param;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct img_data_end endian;
    struct buffer_cfg buf_cfg;
    cmr_u32 i;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    CMR_LOGV("E");

    prev_cxt = &handle->prev_cxt[camera_id];

    preview_enable = prev_cxt->prev_param.preview_eb;
    snapshot_enable = prev_cxt->prev_param.snapshot_eb;
    CMR_LOGD("preview_eb%d, snapshot_eb %d, channel_id %d, %ld, isp_status %ld",
             preview_enable, snapshot_enable, data->channel_id,
             prev_cxt->cap_channel_id, prev_cxt->isp_status);

    if (snapshot_enable && (data->channel_id == prev_cxt->cap_channel_id)) {

        /*reconfig the channel with the params saved before*/
        ret = handle->ops.channel_cfg(handle->oem_handle, handle, camera_id,
                                      &prev_cxt->restart_chn_param, &channel_id,
                                      &endian);
        if (ret) {
            CMR_LOGE("channel config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        CMR_LOGD("cap chn id is %ld", prev_cxt->cap_channel_id);
        if (prev_cxt->capture_scene_mode == DCAM_SCENE_MODE_CAPTURE) {
            prev_cxt->cap_channel_status = PREV_CHN_BUSY;
        } else if (prev_cxt->capture_scene_mode ==
                   DCAM_SCENE_MODE_CAPTURE_CALLBACK) {
            prev_cxt->zsl_channel_status = PREV_CHN_BUSY;
        }

        /* for capture, not skip frame for now, for cts
         * testMandatoryOutputCombinations issue */
        if ((prev_cxt->skip_mode == IMG_SKIP_SW_KER) && 0) {
            /*config skip num buffer*/
            for (i = 0; i < prev_cxt->cap_skip_num; i++) {
                cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
                buf_cfg.channel_id = prev_cxt->cap_channel_id;
                if (prev_cxt->prev_param.sprd_zsl_enabled) {
                    buf_cfg.base_id = CMR_CAP1_ID_BASE;
                } else {
                    buf_cfg.base_id = CMR_CAP0_ID_BASE;
                }
                buf_cfg.count = 1;
                buf_cfg.length = prev_cxt->cap_zsl_mem_size;
                buf_cfg.is_reserved_buf = 0;
                buf_cfg.flag = BUF_FLAG_INIT;
                buf_cfg.addr[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y;
                buf_cfg.addr[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u;
                buf_cfg.addr_vir[0].addr_y =
                    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y;
                buf_cfg.addr_vir[0].addr_u =
                    prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u;
                buf_cfg.fd[0] = prev_cxt->cap_zsl_reserved_frm.fd;
                ret =
                    handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
                if (ret) {
                    CMR_LOGE("channel buff config failed");
                    ret = CMR_CAMERA_FAIL;
                    goto exit;
                }
            }
        }

        prev_cxt->restart_chn_param.buffer.channel_id = channel_id;
        ret = handle->ops.channel_buff_cfg(handle->oem_handle,
                                           &prev_cxt->restart_chn_param.buffer);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        /*config reserved buffer*/
        cmr_bzero(&buf_cfg, sizeof(struct buffer_cfg));
        buf_cfg.channel_id = prev_cxt->cap_channel_id;
        if (prev_cxt->prev_param.sprd_zsl_enabled) {
            buf_cfg.base_id = CMR_CAP1_ID_BASE;
        } else {
            buf_cfg.base_id = CMR_CAP0_ID_BASE;
        }
        buf_cfg.count = 1;
        buf_cfg.length = prev_cxt->cap_zsl_mem_size;
        buf_cfg.is_reserved_buf = 1;
        buf_cfg.flag = BUF_FLAG_INIT;
        buf_cfg.addr[0].addr_y = prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_y;
        buf_cfg.addr[0].addr_u = prev_cxt->cap_zsl_reserved_frm.addr_phy.addr_u;
        buf_cfg.addr_vir[0].addr_y =
            prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_y;
        buf_cfg.addr_vir[0].addr_u =
            prev_cxt->cap_zsl_reserved_frm.addr_vir.addr_u;
        buf_cfg.fd[0] = prev_cxt->cap_zsl_reserved_frm.fd;
        ret = handle->ops.channel_buff_cfg(handle->oem_handle, &buf_cfg);
        if (ret) {
            CMR_LOGE("channel buff config failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

        ret = prev_start(handle, camera_id, 1, 0);
        if (ret) {
            CMR_LOGE("prev start failed");
            goto exit;
        }
    }

exit:
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_fd_open(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;
    struct common_isp_cmd_param isp_cmd_parm;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    isp_cmd_parm.cmd_value = 1;

    CMR_LOGD("is_support_fd %ld, is_fd_on %ld",
             prev_cxt->prev_param.is_support_fd, prev_cxt->prev_param.is_fd_on);

    if (!prev_cxt->prev_param.is_support_fd) {
        CMR_LOGD("not support fd");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGD("fd_handle 0x%p", prev_cxt->fd_handle);
    if (prev_cxt->fd_handle) {
        CMR_LOGD("fd inited already");
        ret = handle->ops.isp_ioctl(
            handle->oem_handle, COM_ISP_SET_AI_SET_FD_ON_OFF, &isp_cmd_parm);
        goto exit;
    }

    in_param.frame_cnt = 1;
    if ((IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot) ||
        (IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot)) {
        in_param.frame_size.width = prev_cxt->actual_prev_size.height;
        in_param.frame_size.height = prev_cxt->actual_prev_size.width;
        in_param.frame_rect.start_x = 0;
        in_param.frame_rect.start_y = 0;
        in_param.frame_rect.width = in_param.frame_size.height;
        in_param.frame_rect.height = in_param.frame_size.width;
    } else {
        in_param.frame_size.width = prev_cxt->actual_prev_size.width;
        in_param.frame_size.height = prev_cxt->actual_prev_size.height;
        in_param.frame_rect.start_x = 0;
        in_param.frame_rect.start_y = 0;
        in_param.frame_rect.width = in_param.frame_size.width;
        in_param.frame_rect.height = in_param.frame_size.height;
    }

    in_param.multi_mode = cxt->is_multi_mode;
    in_param.reg_cb = prev_fd_cb;
    ret = cmr_ipm_open(handle->ipm_handle, IPM_TYPE_FD, &in_param, &out_param,
                       &prev_cxt->fd_handle);

    if (ret) {
        CMR_LOGE("cmr_ipm_open failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    if (prev_cxt->fd_handle) {
        ret = handle->ops.isp_ioctl(
            handle->oem_handle, COM_ISP_SET_AI_SET_FD_ON_OFF, &isp_cmd_parm);
    }
    CMR_LOGD("fd_handle 0x%p", prev_cxt->fd_handle);

exit:
    CMR_LOGD("X");
    ATRACE_END();
    return ret;
}

cmr_int prev_fd_close(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct common_isp_cmd_param isp_cmd_parm;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD("is_support_fd %ld, is_fd_on %ld",
             prev_cxt->prev_param.is_support_fd, prev_cxt->prev_param.is_fd_on);
    CMR_LOGV("fd_handle 0x%p", prev_cxt->fd_handle);

    isp_cmd_parm.cmd_value = 0;
    if (prev_cxt->fd_handle) {
        ret = handle->ops.isp_ioctl(
            handle->oem_handle, COM_ISP_SET_AI_SET_FD_ON_OFF, &isp_cmd_parm);
        ret = cmr_ipm_close(prev_cxt->fd_handle);
        prev_cxt->fd_handle = 0;
    }

    CMR_LOGV("ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_fd_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                          struct img_frm *frm) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct frm_info *info = NULL;
    struct fd_auxiliary_data private_data;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out imp_out_param;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!prev_cxt->fd_handle || !cxt) {
        CMR_LOGE("fd closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    CMR_LOGV("is_support_fd %ld, is_fd_on %ld",
             prev_cxt->prev_param.is_support_fd, prev_cxt->prev_param.is_fd_on);

    if (!prev_cxt->prev_param.is_support_fd || !prev_cxt->prev_param.is_fd_on) {
        CMR_LOGE("fd unsupport or closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_FACE_ATTRIBUTES_ENABLED,
                            &setting_param);
    ipm_in_param.face_attribute_on = setting_param.cmd_type_value;

    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_SMILE_CAPTURE_ENABLED,
                            &setting_param);
    ipm_in_param.smile_capture_on = setting_param.cmd_type_value;

    /* collect face detect private data */
    private_data.camera_id = camera_id;
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = camera_id;
    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                      CAMERA_PARAM_GET_SENSOR_ORIENTATION, &setting_param);
    private_data.sensorOrientation = (cmr_u32)setting_param.cmd_type_value;
    CMR_LOGD("sensorOrientation = %d", private_data.sensorOrientation);
    cmr_setting_ioctl(cxt->setting_cxt.setting_handle,
                      CAMERA_PARAM_GET_DEVICE_ORIENTATION, &setting_param);
    private_data.orientation = (cmr_u32)setting_param.cmd_type_value;
    if((private_data.sensorOrientation == 90) && (camera_id == 1)) {
        if(private_data.orientation == 0)
            private_data.orientation = 180;
        else if(private_data.orientation == 90)
            private_data.orientation = 270;
        else if(private_data.orientation == 180)
            private_data.orientation = 0;
        else if(private_data.orientation == 270)
            private_data.orientation = 90;
    }
    CMR_LOGD("orientation = %d", private_data.orientation);
    private_data.bright_value = prev_cxt->ae_stab[AE_CB_BV_INDEX];
    private_data.ae_stable = prev_cxt->ae_stab[AE_CB_STABLE_INDEX];
    private_data.backlight_pro = prev_cxt->ae_stab[AE_CB_BLS_INDEX];
    info = (struct frm_info *)frm->reserved;
    private_data.zoom_ratio = info ? info->zoom_ratio : (cmr_u32)(-1);
    memcpy(&private_data.hist, prev_cxt->hist, sizeof(private_data.hist));

    ipm_in_param.src_frame = *frm;
    ipm_in_param.dst_frame = *frm;
    ipm_in_param.touch_x = prev_cxt->touch_info.touchX;
    ipm_in_param.touch_y = prev_cxt->touch_info.touchY;
    ipm_in_param.caller_handle = (void *)handle;
    ipm_in_param.private_data = &private_data;

    ret = ipm_transfer_frame(prev_cxt->fd_handle, &ipm_in_param, NULL);
    if (ret) {
        CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        goto exit;
    }

exit:
    CMR_LOGV("ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_fd_cb(cmr_u32 class_type, struct ipm_frame_out *cb_param) {
    UNUSED(class_type);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = NULL;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;
    cmr_u32 camera_id = CAMERA_ID_MAX;
    cmr_u32 i = 0;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = (cmr_u32)((unsigned long)cb_param->private_data);
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!prev_cxt->prev_param.is_support_fd || !prev_cxt->prev_param.is_fd_on) {
        CMR_LOGW("fd closed");
        return CMR_CAMERA_INVALID_PARAM;
    }

    /*copy face-detect info*/
    cmr_bzero(&frame_type, sizeof(struct camera_frame_type));
    struct frm_info *info = cb_param->dst_frame.reserved;
    if (info != NULL) {
        frame_type.frame_num = info->frame_num;
        frame_type.timestamp = info->sec * 1000000000LL + info->usec * 1000LL;
        frame_type.monoboottime = info->monoboottime;
    }
    frame_type.is_update_isp = cb_param->is_plus;
    frame_type.width = cb_param->dst_frame.size.width;
    frame_type.height = cb_param->dst_frame.size.height;
    frame_type.face_num = (cmr_u32)cb_param->face_area.face_count;
    CMR_LOGV("face_num %d", frame_type.face_num);
    for (i = 0; i < frame_type.face_num; i++) {
        frame_type.face_info[i].face_id = cb_param->face_area.range[i].face_id;
        frame_type.face_info[i].sx = cb_param->face_area.range[i].sx;
        frame_type.face_info[i].sy = cb_param->face_area.range[i].sy;
        frame_type.face_info[i].srx = cb_param->face_area.range[i].srx;
        frame_type.face_info[i].sry = cb_param->face_area.range[i].sry;
        frame_type.face_info[i].ex = cb_param->face_area.range[i].ex;
        frame_type.face_info[i].ey = cb_param->face_area.range[i].ey;
        frame_type.face_info[i].elx = cb_param->face_area.range[i].elx;
        frame_type.face_info[i].ely = cb_param->face_area.range[i].ely;
        frame_type.face_info[i].brightness =
            cb_param->face_area.range[i].brightness;
        frame_type.face_info[i].angle = cb_param->face_area.range[i].angle;
        frame_type.face_info[i].score = cb_param->face_area.range[i].score;
        frame_type.face_info[i].pose = cb_param->face_area.range[i].pose;
        frame_type.face_info[i].smile_level =
            cb_param->face_area.range[i].smile_level;
        frame_type.face_info[i].blink_level =
            cb_param->face_area.range[i].blink_level;
        frame_type.face_info[i].gender_age_race =
            cb_param->face_area.range[i].gender_age_race;
    }

    CMR_LOGD(" frame_type.timestamp:%lld   frame_type.face_num  :%u,  "
             "frame_type.frame_num:%u ",
             frame_type.timestamp, frame_type.face_num, frame_type.frame_num);

    /*notify fd info directly*/
    cb_data_info.cb_type = PREVIEW_EVT_CB_FD;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    return ret;
}


cmr_int threednr_sw_prev_callback(cmr_u32 class_type,
                                  struct ipm_frame_out *cb_param) {
    // send message to prev assist
    CMR_MSG_INIT(message);
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = cb_param->caller_handle;
    struct prev_threednr_info *threednr_info;
    struct prev_cb_info cb_data_info;
    threednr_info = cb_param->private_data;
    cmr_u32 camera_id = threednr_info->camera_id;
    struct ipm_frame_out *threednr_preparam = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    CMR_LOGV("E");
    /*deliver the zoom param via internal msg*/
    threednr_preparam = (struct ipm_frame_out *)malloc(
        sizeof(struct ipm_frame_out) + sizeof(struct prev_threednr_info));
    if (!threednr_preparam) {
        CMR_LOGE("No mem!");
        ret = CMR_CAMERA_NO_MEM;
        goto exit;
    }

    cmr_bzero(threednr_preparam, sizeof(struct ipm_frame_out));
    *threednr_preparam = *cb_param;
    threednr_preparam->private_data =
        ((uint8_t *)threednr_preparam) + sizeof(struct ipm_frame_out);
    *((struct prev_threednr_info *)(threednr_preparam->private_data)) =
        *((struct prev_threednr_info *)(cb_param->private_data));
    threednr_preparam->caller_handle = cb_param->caller_handle;

    message.msg_type = PREV_EVT_3DNR_CALLBACK;
    message.sync_flag = CMR_MSG_SYNC_NONE;
    message.data = (void *)threednr_preparam;
    message.alloc_flag = 1;
    ret =
        cmr_thread_msg_send(handle->thread_cxt.assist_thread_handle, &message);
    if (ret) {
        CMR_LOGE("send msg failed!");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:
    if (ret) {
        if (threednr_preparam) {
            free(threednr_preparam);
        }
    }
    CMR_LOGV("X");
    return ret;
}

cmr_int prev_fd_ctrl(struct prev_handle *handle, cmr_u32 camera_id,
                     cmr_u32 on_off) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD(" %d", on_off);

    prev_cxt->prev_param.is_fd_on = on_off;

    if (0 == on_off) {
        prev_fd_close(handle, camera_id);
    } else {
        prev_fd_open(handle, camera_id);
    }

    return ret;
}

cmr_int prev_3dnr_open(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD("is_support_3dnr %u", prev_cxt->prev_param.sprd_3dnr_type);

    if (prev_cxt->prev_param.sprd_3dnr_type == 0) {
        CMR_LOGD("not support preview 3dnr");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (prev_cxt->prev_3dnr_handle) {
        CMR_LOGD("3dnr preview  inited already");
        goto exit;
    }

    in_param.frame_cnt = 1;
    if ((IMG_ANGLE_90 == prev_cxt->prev_param.prev_rot) ||
        (IMG_ANGLE_270 == prev_cxt->prev_param.prev_rot)) {
        in_param.frame_size.width = prev_cxt->actual_prev_size.height;
        in_param.frame_size.height = prev_cxt->actual_prev_size.width;
        in_param.frame_rect.start_x = 0;
        in_param.frame_rect.start_y = 0;
        in_param.frame_rect.width = in_param.frame_size.height;
        in_param.frame_rect.height = in_param.frame_size.width;
    } else {
        in_param.frame_size.width = prev_cxt->actual_prev_size.width;
        in_param.frame_size.height = prev_cxt->actual_prev_size.height;
        in_param.frame_rect.start_x = 0;
        in_param.frame_rect.start_y = 0;
        in_param.frame_rect.width = in_param.frame_size.width;
        in_param.frame_rect.height = in_param.frame_size.height;
    }
CMR_LOGI("snapshot width size =%d ,height =%d",cxt->snp_cxt.request_size.width,cxt->snp_cxt.request_size.height);

    in_param.reg_cb = threednr_sw_prev_callback;
    ret = cmr_ipm_open(handle->ipm_handle, IPM_TYPE_3DNR_PRE, &in_param,
                       &out_param, &prev_cxt->prev_3dnr_handle);
    if (ret) {
        CMR_LOGE("cmr_ipm_open failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }
    CMR_LOGD("prev_3dnr_handle 0x%p, out", prev_cxt->prev_3dnr_handle);

exit:
    ATRACE_END();
    return ret;
}


cmr_int prev_3dnr_close(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD("is_support_3dnr%u", prev_cxt->prev_param.sprd_3dnr_type);

    CMR_LOGV("3dnr_handle 0x%p", prev_cxt->prev_3dnr_handle);
    if (prev_cxt->prev_3dnr_handle) {
        ret = cmr_ipm_close(prev_cxt->prev_3dnr_handle);
        prev_cxt->prev_3dnr_handle = 0;
    }

    CMR_LOGV("ret %ld", ret);
    ATRACE_END();
    return ret;
}


cmr_int prev_3dnr_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                            struct frm_info *frm_info,
                            struct camera_frame_type *frame_type,
                            struct img_frm *frm, struct img_frm *video_frm) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out imp_out_param;
    struct prev_threednr_info threednr_info;
    struct isp_adgain_exp_info adgain_exp_info;

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!prev_cxt->prev_3dnr_handle) {
        CMR_LOGE("3dnr closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (prev_cxt->prev_param.sprd_3dnr_type == 0) {
        CMR_LOGE("3dnr unsupport or closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    ipm_in_param.src_frame = *frm;
    ipm_in_param.caller_handle = (void *)handle;
    if (prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW||
                prev_cxt->prev_param.sprd_3dnr_type == CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW) {
        CMR_LOGI("3DNR Using sw path");
        imp_out_param.dst_frame = *video_frm;
        ipm_in_param.private_data = (void *)(&threednr_info);
        threednr_info.frm_preview = *frm;
        threednr_info.frm_video = *video_frm;
        threednr_info.caller_handle = (void *)handle;
        threednr_info.camera_id = (unsigned long)camera_id;
        threednr_info.data = *frm_info;
        threednr_info.framtype = *frame_type;
        ipm_in_param.adgain = 16;
#if 0
        ret = handle->ops.get_tuning_info(handle->oem_handle, &adgain_exp_info);
        if (ret)
            CMR_LOGE("failed to get ae info, using default!");
        else
            ipm_in_param.adgain = adgain_exp_info.adgain / 128;
        CMR_LOGI("SW 3DRN, Get Gain from ISP: %d", ipm_in_param.adgain);
#endif
        ret = ipm_transfer_frame(prev_cxt->prev_3dnr_handle, &ipm_in_param,
                                 &imp_out_param);
       }
else
      {
    CMR_LOGI("3dnr using hw path");
    ipm_in_param.src_frame = *frm;
    ipm_in_param.dst_frame = *frm;
    ipm_in_param.caller_handle = (void *)handle;
    ipm_in_param.private_data = (void *)((unsigned long)camera_id);

    ret = ipm_transfer_frame(prev_cxt->prev_3dnr_handle, &ipm_in_param, NULL);
    if (ret) {
        CMR_LOGE("failed to transfer frame to 3dnr preview %ld", ret);
        goto exit;
         }
    }

    //ret = cmr_preview_flush_cache(handle, &ipm_in_param.src_frame);
    if (ret) {
        goto exit;
    }

    CMR_LOGV("ret %ld", ret);

exit:
    ATRACE_END();
    return ret;
}

cmr_int prev_ultra_wide_open(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    struct sprd_img_path_rect sn_trim;
    struct sensor_exp_info *sensor_info = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.is_ultra_wide == 0) {
        CMR_LOGD("not support preview ultra wide");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (prev_cxt->ultra_wide_handle) {
        CMR_LOGD("ultra wide handle preview inited already");
        goto exit;
    }
    cmr_bzero(&out_param, sizeof(out_param));
    cmr_grab_get_dcam_path_trim(cxt->grab_cxt.grab_handle, &sn_trim);
    sensor_info = &handle->prev_cxt[camera_id].sensor_info;

    in_param.frame_rect.start_x = sn_trim.trim_valid_rect.x;
    in_param.frame_rect.start_y = sn_trim.trim_valid_rect.y;
    in_param.frame_rect.width = sn_trim.trim_valid_rect.w;
    in_param.frame_rect.height = sn_trim.trim_valid_rect.h;
    in_param.sensor_size.width = sensor_info->source_width_max;
    in_param.sensor_size.height = sensor_info->source_height_max;

    // for prev
    if (prev_cxt->prev_param.preview_eb && (!prev_cxt->ultra_wide_handle)) {
        in_param.binning_factor =
            cxt->sn_cxt.sensor_info.mode_info[prev_cxt->prev_mode]
                .binning_factor;
        in_param.frame_size.width = prev_cxt->actual_prev_size.width;
        in_param.frame_size.height = prev_cxt->actual_prev_size.height;
        in_param.is_cap = false;
        ret = cmr_ipm_open(handle->ipm_handle, IPM_TYPE_ULTRA_WIDE, &in_param,
                           &out_param, &prev_cxt->ultra_wide_handle);
        prev_cxt->prev_zoom = out_param.isp_zoom;
        if (ret) {
            CMR_LOGE("cmr_ipm_open failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    // for video
    if (prev_cxt->prev_param.video_eb && (!prev_cxt->video_ultra_wide_handle)) {
        in_param.binning_factor =
            cxt->sn_cxt.sensor_info.mode_info[prev_cxt->video_mode]
                .binning_factor;
        in_param.frame_size.width = prev_cxt->actual_video_size.width;
        in_param.frame_size.height = prev_cxt->actual_video_size.height;
        in_param.is_cap = false;
        ret = cmr_ipm_open(handle->ipm_handle, IPM_TYPE_ULTRA_WIDE, &in_param,
                           &out_param, &prev_cxt->video_ultra_wide_handle);
        prev_cxt->video_zoom = out_param.isp_zoom;
        if (ret) {
            CMR_LOGE("cmr_ipm_open failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    // for cap
    if (prev_cxt->prev_param.snapshot_eb &&
        (!prev_cxt->zsl_ultra_wide_handle)) {
        in_param.binning_factor =
            cxt->sn_cxt.sensor_info.mode_info[prev_cxt->cap_mode]
                .binning_factor;
        in_param.frame_size.width = prev_cxt->actual_pic_size.width;
        in_param.frame_size.height = prev_cxt->actual_pic_size.height;
        in_param.is_cap = true;
        ret = cmr_ipm_open(handle->ipm_handle, IPM_TYPE_ULTRA_WIDE, &in_param,
                           &out_param, &prev_cxt->zsl_ultra_wide_handle);
        prev_cxt->cap_zoom = out_param.isp_zoom;
        if (ret) {
            CMR_LOGE("cmr_ipm_open failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

exit:
    return ret;
}

cmr_int prev_ultra_wide_close(struct prev_handle *handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    prev_cxt = &handle->prev_cxt[camera_id];

    CMR_LOGD("is_support_ultra_wide%d", prev_cxt->prev_param.is_ultra_wide);

    CMR_LOGI("ultra_wide_handle 0x%p", prev_cxt->ultra_wide_handle);
    if (prev_cxt->ultra_wide_handle) {
        ret = cmr_ipm_close(prev_cxt->ultra_wide_handle);
        prev_cxt->ultra_wide_handle = 0;
    }

    CMR_LOGI("video_ultra_wide_handle 0x%p", prev_cxt->video_ultra_wide_handle);
    if (prev_cxt->video_ultra_wide_handle) {
        ret = cmr_ipm_close(prev_cxt->video_ultra_wide_handle);
        prev_cxt->video_ultra_wide_handle = 0;
    }

    CMR_LOGI("zsl_ultra_wide_handle 0x%p", prev_cxt->zsl_ultra_wide_handle);
    if (prev_cxt->zsl_ultra_wide_handle) {
        ret = cmr_ipm_close(prev_cxt->zsl_ultra_wide_handle);
        prev_cxt->zsl_ultra_wide_handle = 0;
    }
    CMR_LOGV("ret %ld", ret);
    return ret;
}

cmr_int prev_ultra_wide_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                                  struct frm_info *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 frm_id = 0;
    cmr_uint ultra_wide_frm_id = 0;
    int frame_type = PREVIEW_FRAME;
    int zoom_changed = 1, cap_zoom_changed = 1;
    float org_zoom = 1.0f;
    float cap_org_zoom =1.0f;
    const float EPSINON = 0.0001f;
    ipm_param_t param_info;
    struct img_frm src, dst;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct prev_context *prev_cxt = NULL;
    struct ipm_frame_in ipm_in_param;
    struct setting_cmd_parameter setting_param;
    struct channel_start_param chn_param;
    struct sensor_exp_info *sensor_info = NULL;
    struct sensor_mode_info *sensor_mode_info = NULL;
    struct img_frm *dst_img = NULL;
    struct img_frm *src_img = NULL;
    struct img_frm *dst_eis_img = NULL;
    void *src_buffer_handle = NULL;
    void *dst_buffer_handle = NULL;
    void * dst_eis_video_buffer_handle = NULL;
    cmr_handle *ultra_wide_handle = NULL;

    cmr_bzero(&setting_param, sizeof(setting_param));
    cmr_bzero(&chn_param, sizeof(struct channel_start_param));
    dst_eis_img = (struct img_frm *)malloc (sizeof(struct img_frm));
    setting_param.camera_id = camera_id;
    prev_cxt = &handle->prev_cxt[camera_id];

    if (prev_cxt->prev_param.is_ultra_wide == 0) {
        CMR_LOGD("not support preview ultra wide");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    if (PREVIEWING == prev_cxt->prev_status) {
        ret =
            prev_get_src_ultra_wide_buffer(prev_cxt, data, &ultra_wide_frm_id);
        if (ret) {
            CMR_LOGE("get src ultra_wide buffer failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                                    SETTING_GET_REPROCESS_ZOOM_RATIO,
                                    &setting_param);
        if(ret) {
            CMR_LOGE("fail to get reprocess zoom ratio.");
            goto exit;
        }
        memcpy(&param_info.zoom,&setting_param.zoom_param.zoom_info,sizeof(struct zoom_info));

        if (IS_PREVIEW_FRM(data->frame_id)) {
            frame_type = PREVIEW_FRAME;
            src_img = &prev_cxt->prev_ultra_wide_frm[ultra_wide_frm_id];
            src_buffer_handle =
                prev_cxt->prev_ultra_wide_handle_array[ultra_wide_frm_id];
            frm_id = data->frame_id - CMR_PREV_ID_BASE;
            dst_img = &prev_cxt->prev_frm[frm_id];
            ultra_wide_handle = prev_cxt->ultra_wide_handle;
            param_info.zoomRatio = setting_param.zoom_param.zoom_info.prev_aspect_ratio;

            /*float new_zoom = setting_param.zoom_param.zoom_info.prev_aspect_ratio;
            if(fabs(org_zoom - new_zoom) >= EPSINON)
                  zoom_changed = 1;
            else
                  zoom_changed = 0;
            org_zoom = new_zoom;
            if(zoom_changed) {*/      /*set AE ROI*/
                  chn_param.sensor_mode = prev_cxt->prev_mode;
                  sensor_info = &prev_cxt->sensor_info;
                  sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];

                  chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
                  chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
                  chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
                  chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;

                  chn_param.cap_inf_cfg.cfg.dst_img_size.width =
                           prev_cxt->actual_prev_size.width;
                  chn_param.cap_inf_cfg.cfg.dst_img_size.height =
                           prev_cxt->actual_prev_size.height;
                  chn_param.cap_inf_cfg.cfg.notice_slice_height =
                           chn_param.cap_inf_cfg.cfg.dst_img_size.height;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
                           sensor_mode_info->scaler_trim.start_x;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
                           sensor_mode_info->scaler_trim.start_y;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.width =
                           sensor_mode_info->scaler_trim.width;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.height =
                           sensor_mode_info->scaler_trim.height;

                  CMR_LOGV("preview src_img_rect %d %d %d %d",
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.width,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.height);

                  param_info.fullsize_height = sensor_info->source_height_max;
                  param_info.fullsize_width = sensor_info->source_width_max;
                  if(src_img != NULL){
                      param_info.input_height = src_img->size.height;
                      param_info.input_width = src_img->size.width;
                  }
                  /*caculate trim rect*/
                  if (ZOOM_INFO != setting_param.zoom_param.mode) {
                           ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   setting_param.zoom_param.zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
                  } else {
                           float real_ratio = setting_param.zoom_param.zoom_info.prev_aspect_ratio;
                           float aspect_ratio = 1.0 * prev_cxt->actual_prev_size.width /
                                  prev_cxt->actual_prev_size.height;

                           if (setting_param.zoom_param.zoom_info.crop_region.width > 0) {
                                 chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                                         setting_param.zoom_param.zoom_info.pixel_size, 
                                         setting_param.zoom_param.zoom_info.crop_region,
                                         chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
                           } else {
                           float aspect_ratio = 1.0 * prev_cxt->actual_prev_size.width /
                                    prev_cxt->actual_prev_size.height;
                           ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    setting_param.zoom_param.zoom_info.prev_aspect_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.cap_rot);
                           }
                  }
                  if (ret) {
                           CMR_LOGE("prev get trim failed, %d",ret);
                  }

                  CMR_LOGV("camera %u preview after src_img_rect %d %d %d %d", camera_id,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.width,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.height);

                  param_info.crop_x= chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
                  param_info.crop_y = chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
                  param_info.crop_height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;
                  param_info.crop_width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;

                  CMR_LOGD("prev ultrawid set ratio=%f, fullsize_height=%d, "
                      "fullsize_width=%d, input_height=%d, input_width=%d, "
                      "crop_x=%d, crop_y=%d, crop_width=%d, crop_height=%d",
                      param_info.zoomRatio, param_info.fullsize_height,
                      param_info.fullsize_width, param_info.input_height,
                      param_info.input_width, param_info.crop_x, param_info.crop_y,
                      param_info.crop_width, param_info.crop_height);
                  cxt->zoom_ratio = param_info.zoomRatio;
                  if (handle->ops.isp_ioctl) {
                           struct common_isp_cmd_param param;
                           param.camera_id = camera_id;
                           param.ae_target_region = chn_param.cap_inf_cfg.cfg.src_img_rect;
                           ret = handle->ops.isp_ioctl(handle->oem_handle,
                                   COM_ISP_SET_AE_TARGET_REGION, &param);
                      if (ret < 0) {
                            CMR_LOGW("fail to set AE roi");
                      }
                 }
            //}
        } else if (IS_VIDEO_FRM(data->frame_id)) {
            frame_type = PREVIEW_VIDEO_FRAME;
            src_img = &prev_cxt->video_ultra_wide_frm[ultra_wide_frm_id];
            src_buffer_handle =
                prev_cxt->video_ultra_wide_handle_array[ultra_wide_frm_id];
            frm_id = data->frame_id - CMR_VIDEO_ID_BASE;
            dst_img = &prev_cxt->video_frm[frm_id];
            ultra_wide_handle = prev_cxt->video_ultra_wide_handle;
            param_info.zoomRatio = setting_param.zoom_param.zoom_info.video_aspect_ratio;

            /*float new_zoom = setting_param.zoom_param.zoom_info.video_aspect_ratio;
            if(fabs(org_zoom - new_zoom) >= EPSINON)
                  zoom_changed = 1;
            else
                  zoom_changed = 0;
            org_zoom = new_zoom;
            if(zoom_changed) {*/      /*set AE ROI*/
                  chn_param.sensor_mode = prev_cxt->video_mode;
                  sensor_info = &prev_cxt->sensor_info;
                  sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];

                  chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
                  chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
                  chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
                  chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;

                  chn_param.cap_inf_cfg.cfg.dst_img_size.width =
                           prev_cxt->actual_video_size.width;
                  chn_param.cap_inf_cfg.cfg.dst_img_size.height =
                           prev_cxt->actual_video_size.height;
                  chn_param.cap_inf_cfg.cfg.notice_slice_height =
                           chn_param.cap_inf_cfg.cfg.dst_img_size.height;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
                           sensor_mode_info->scaler_trim.start_x;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
                           sensor_mode_info->scaler_trim.start_y;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.width =
                           sensor_mode_info->scaler_trim.width;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.height =
                           sensor_mode_info->scaler_trim.height;

                  CMR_LOGV("video src_img_rect %d %d %d %d",
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.width,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.height);

                  param_info.fullsize_height = sensor_info->source_height_max;
                  param_info.fullsize_width = sensor_info->source_width_max;
                  if(src_img != NULL){
                      param_info.input_height = src_img->size.height;
                      param_info.input_width = src_img->size.width;
                  }

                  /*caculate trim rect*/
                  if (ZOOM_INFO != setting_param.zoom_param.mode) {
                           ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   setting_param.zoom_param.zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
                  } else {
                           float real_ratio = setting_param.zoom_param.zoom_info.video_aspect_ratio;
                           float aspect_ratio = 1.0 * prev_cxt->actual_video_size.width /
                                  prev_cxt->actual_video_size.height;

                           if (setting_param.zoom_param.zoom_info.crop_region.width > 0) {
                                 chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                                         setting_param.zoom_param.zoom_info.pixel_size,
                                         setting_param.zoom_param.zoom_info.crop_region,
                                         chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
                           } else {
                           float aspect_ratio = 1.0 * prev_cxt->actual_video_size.width /
                                    prev_cxt->actual_video_size.height;
                           ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    setting_param.zoom_param.zoom_info.video_aspect_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.cap_rot);
                           }
                  }
                  if (ret) {
                           CMR_LOGE("video get trim failed, %d",ret);
                  }

                  CMR_LOGV("camera %u video after src_img_rect %d %d %d %d", camera_id,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.width,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.height);

                  param_info.crop_x= chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
                  param_info.crop_y = chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
                  param_info.crop_height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;
                  param_info.crop_width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;

                  CMR_LOGD("video ultrawid set ratio=%f, fullsize_height=%d"
                      ", fullsize_width=%d, input_height=%d, input_width=%d",
                      ", crop_x=%d, crop_y=%d, crop_width=%d, crop_height=%d",
                      param_info.zoomRatio, param_info.fullsize_height,
                      param_info.fullsize_width, param_info.input_height, param_info.input_width,
                      param_info.crop_x,param_info.crop_y,param_info.crop_width,param_info.crop_height);

                  if (handle->ops.isp_ioctl) {
                           struct common_isp_cmd_param param;
                           param.camera_id = camera_id;
                           param.ae_target_region = chn_param.cap_inf_cfg.cfg.src_img_rect;
                           ret = handle->ops.isp_ioctl(handle->oem_handle,
                                   COM_ISP_SET_AE_TARGET_REGION, &param);
                      if (ret < 0) {
                            CMR_LOGW("fail to set AE roi");
                      }
                 }
            //}
        } else if (IS_VIDEO_FRM(data->frame_id)) {
            frame_type = PREVIEW_VIDEO_FRAME;
            src_img = &prev_cxt->video_ultra_wide_frm[ultra_wide_frm_id];
            src_buffer_handle =
                prev_cxt->video_ultra_wide_handle_array[ultra_wide_frm_id];
            frm_id = data->frame_id - CMR_VIDEO_ID_BASE;
            dst_img = &prev_cxt->video_frm[frm_id];
            ultra_wide_handle = prev_cxt->video_ultra_wide_handle;
            param_info.zoomRatio = setting_param.zoom_param.zoom_info.video_aspect_ratio;

            /*float new_zoom = setting_param.zoom_param.zoom_info.video_aspect_ratio;
            if(fabs(org_zoom - new_zoom) >= EPSINON)
                  zoom_changed = 1;
            else
                  zoom_changed = 0;
            org_zoom = new_zoom;
            if(zoom_changed) {*/      /*set AE ROI*/
                  chn_param.sensor_mode = prev_cxt->video_mode;
                  sensor_info = &prev_cxt->sensor_info;
                  sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];

                  chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
                  chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
                  chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
                  chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;

                  chn_param.cap_inf_cfg.cfg.dst_img_size.width =
                           prev_cxt->actual_video_size.width;
                  chn_param.cap_inf_cfg.cfg.dst_img_size.height =
                           prev_cxt->actual_video_size.height;
                  chn_param.cap_inf_cfg.cfg.notice_slice_height =
                           chn_param.cap_inf_cfg.cfg.dst_img_size.height;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
                           sensor_mode_info->scaler_trim.start_x;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
                           sensor_mode_info->scaler_trim.start_y;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.width =
                           sensor_mode_info->scaler_trim.width;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.height =
                           sensor_mode_info->scaler_trim.height;

                  CMR_LOGV("video src_img_rect %d %d %d %d",
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.width,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.height);

                  param_info.fullsize_height = sensor_info->source_height_max;
                  param_info.fullsize_width = sensor_info->source_width_max;
                  if(src_img != NULL){
                      param_info.input_height = src_img->size.height;
                      param_info.input_width = src_img->size.width;
                  }

                  /*caculate trim rect*/
                  if (ZOOM_INFO != setting_param.zoom_param.mode) {
                           ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   setting_param.zoom_param.zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
                  } else {
                           float real_ratio = setting_param.zoom_param.zoom_info.video_aspect_ratio;
                           float aspect_ratio = 1.0 * prev_cxt->actual_video_size.width /
                                  prev_cxt->actual_video_size.height;

                           if (setting_param.zoom_param.zoom_info.crop_region.width > 0) {
                                 chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                                         setting_param.zoom_param.zoom_info.pixel_size,
                                         setting_param.zoom_param.zoom_info.crop_region,
                                         chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
                           } else {
                           float aspect_ratio = 1.0 * prev_cxt->actual_video_size.width /
                                    prev_cxt->actual_video_size.height;
                           ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                    setting_param.zoom_param.zoom_info.video_aspect_ratio,
                                    aspect_ratio,
                                    sensor_mode_info->scaler_trim.width,
                                    sensor_mode_info->scaler_trim.height,
                                    prev_cxt->prev_param.cap_rot);
                           }
                  }
                  if (ret) {
                           CMR_LOGE("video get trim failed, %d",ret);
                  }

                  CMR_LOGV("camera %u video after src_img_rect %d %d %d %d", camera_id,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.width,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.height);

                  param_info.crop_x= chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
                  param_info.crop_y = chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
                  param_info.crop_height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;
                  param_info.crop_width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;

                  CMR_LOGD("video ultrawid set ratio %f", param_info.zoomRatio);
                  CMR_LOGD("video fullsize_height=%d,fullsize_width=%d",param_info.fullsize_height, param_info.fullsize_width);
                  CMR_LOGD("video input_height=%d,input_width=%d", param_info.input_height, param_info.input_width);
                  CMR_LOGD("video crop_x=%d,crop_y=%d,crop_width=%d,crop_height=%d",
                           param_info.crop_x,param_info.crop_y,param_info.crop_width,param_info.crop_height);
                  if (handle->ops.isp_ioctl) {
                           struct common_isp_cmd_param param;
                           param.camera_id = camera_id;
                           param.ae_target_region = chn_param.cap_inf_cfg.cfg.src_img_rect;
                           ret = handle->ops.isp_ioctl(handle->oem_handle,
                                   COM_ISP_SET_AE_TARGET_REGION, &param);
                      if (ret < 0) {
                            CMR_LOGW("fail to set AE roi");
                      }
                 }
            //}
        } else if (IS_ZSL_FRM(data->frame_id)) {
            cmr_s32 dst_fd;
            frame_type = PREVIEW_ZSL_FRAME;
            src_img = &prev_cxt->cap_zsl_ultra_wide_frm[ultra_wide_frm_id];
            src_buffer_handle =
                prev_cxt->cap_zsl_ultra_wide_handle_array[ultra_wide_frm_id];
            frm_id = data->frame_id - CMR_CAP1_ID_BASE;
            dst_img = &prev_cxt->cap_zsl_frm[frm_id];
            dst_fd = prev_cxt->cap_zsl_fd_array[frm_id];
            for (int i = 0; i < ZSL_FRM_CNT; i++) {
                if (dst_fd == prev_cxt->cap_zsl_dst_fd_array[i]) {
                    dst_buffer_handle = prev_cxt->cap_zsl_dst_handle_array[i];
                }
            }
            ultra_wide_handle = prev_cxt->zsl_ultra_wide_handle;
            param_info.zoomRatio = setting_param.zoom_param.zoom_info.capture_aspect_ratio;

            /*float cap_new_zoom = setting_param.zoom_param.zoom_info.capture_aspect_ratio;
            if(fabs(cap_org_zoom - cap_new_zoom) >= EPSINON)
                  cap_zoom_changed = 1;
            else   cap_zoom_changed = 0;
            cap_org_zoom = cap_new_zoom;
            if(zoom_changed) {   */   /*set AE ROI*/
                  chn_param.sensor_mode = prev_cxt->cap_mode;
                  sensor_info = &prev_cxt->sensor_info;
                  sensor_mode_info = &sensor_info->mode_info[chn_param.sensor_mode];

                  chn_param.cap_inf_cfg.cfg.dst_img_fmt = prev_cxt->prev_param.preview_fmt;
                  chn_param.cap_inf_cfg.cfg.src_img_fmt = sensor_mode_info->image_format;
                  chn_param.cap_inf_cfg.cfg.regular_desc.regular_mode = 0;
                  chn_param.cap_inf_cfg.cfg.chn_skip_num = 0;

                  chn_param.cap_inf_cfg.cfg.dst_img_size.width =
                           prev_cxt->actual_prev_size.width;
                  chn_param.cap_inf_cfg.cfg.dst_img_size.height =
                           prev_cxt->actual_prev_size.height;
                  chn_param.cap_inf_cfg.cfg.notice_slice_height =
                           chn_param.cap_inf_cfg.cfg.dst_img_size.height;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_x =
                           sensor_mode_info->scaler_trim.start_x;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.start_y =
                           sensor_mode_info->scaler_trim.start_y;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.width =
                           sensor_mode_info->scaler_trim.width;
                  chn_param.cap_inf_cfg.cfg.src_img_rect.height =
                           sensor_mode_info->scaler_trim.height;

                  CMR_LOGV("capture src_img_rect %d %d %d %d",
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_x,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.start_y,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.width,
                           chn_param.cap_inf_cfg.cfg.src_img_rect.height);

                  param_info.fullsize_height = sensor_info->source_height_max;
                  param_info.fullsize_width = sensor_info->source_width_max;
                  if(src_img != NULL){
                      param_info.input_height = src_img->size.height;
                      param_info.input_width = src_img->size.width;
                  }

                  /*caculate trim rect*/
                  if (ZOOM_INFO != setting_param.zoom_param.mode) {
                           ret = camera_get_trim_rect(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                   setting_param.zoom_param.zoom_level,
                                   &chn_param.cap_inf_cfg.cfg.dst_img_size);
                  } else {
                           float aspect_ratio = 1.0 * prev_cxt->actual_pic_size.width /
                                  prev_cxt->actual_pic_size.height;
                           if (setting_param.zoom_param.zoom_info.crop_region.width > 0) {
                                 chn_param.cap_inf_cfg.cfg.src_img_rect = camera_apply_rect_and_ratio(
                                         setting_param.zoom_param.zoom_info.pixel_size, 
                                         setting_param.zoom_param.zoom_info.crop_region,
                                         chn_param.cap_inf_cfg.cfg.src_img_rect, aspect_ratio);
                           } else {
                                 ret = camera_get_trim_rect2(&chn_param.cap_inf_cfg.cfg.src_img_rect,
                                         setting_param.zoom_param.zoom_info.capture_aspect_ratio,
                                         aspect_ratio,
                                         sensor_mode_info->scaler_trim.width,
                                         sensor_mode_info->scaler_trim.height,
                                         prev_cxt->prev_param.prev_rot);
                           }
                  }
                  if (ret) {
                           CMR_LOGE("capture get trim failed, %d",ret);
                  }
                  param_info.crop_x= chn_param.cap_inf_cfg.cfg.src_img_rect.start_x;
                  param_info.crop_y = chn_param.cap_inf_cfg.cfg.src_img_rect.start_y;
                  param_info.crop_height = chn_param.cap_inf_cfg.cfg.src_img_rect.height;
                  param_info.crop_width = chn_param.cap_inf_cfg.cfg.src_img_rect.width;

                  CMR_LOGD("cap ultrawid set ratio=%f, fullsize_height=%d"
                      ", fullsize_width=%d, input_height=%d, input_width=%d",
                      ", crop_x=%d, crop_y=%d, crop_width=%d, crop_height=%d",
                      param_info.zoomRatio, param_info.fullsize_height,
                      param_info.fullsize_width, param_info.input_height, param_info.input_width,
                      param_info.crop_x,param_info.crop_y,param_info.crop_width,param_info.crop_height);
            //}
        } else {
            CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                     prev_cxt->prev_status, data->frame_id);
            ret = CMR_CAMERA_INVALID_STATE;
            goto exit;
        }

        if (ultra_wide_handle != NULL && dst_img != NULL && src_img != NULL) {
            cam_graphic_buffer_info_t buf_info;
            cmr_bzero(&buf_info, sizeof(buf_info));

            buf_info.fd = dst_img->fd;
            buf_info.addr_vir = dst_img->addr_vir.addr_y;
            buf_info.addr_phy = dst_img->addr_phy.addr_y;
            buf_info.width = dst_img->size.width;
            buf_info.height = dst_img->size.height;
            buf_info.buf_size = dst_img->buf_size;
            if (dst_buffer_handle == NULL) {
                handle->ops.get_buff_handle(handle->oem_handle, frame_type,
                                            &buf_info);
                dst_buffer_handle = buf_info.graphic_buffer;
            }

            if (ret) {
                CMR_LOGE("failed to get zoom ratio %ld", ret);
                ret = CMR_CAMERA_FAIL;
                goto exit;
            }
            CMR_LOGD("ultra wide src:%p, dst:%p size:%ld"
                     ", src_buf_hd:%p, dst_buf_hd:%p\n",
                     (void *)src_img->addr_vir.addr_y,
                     (void *)dst_img->addr_vir.addr_y,
                     dst_img->buf_size, src_buffer_handle, dst_buffer_handle);
            cmr_bzero(&ipm_in_param, sizeof(struct ipm_frame_in));
            ipm_in_param.src_frame = *src_img;
            ipm_in_param.src_frame.frame_number = prev_cxt->prev_frm_cnt;
            ipm_in_param.src_frame.reserved = src_buffer_handle;

            if (prev_cxt->prev_param.sprd_eis_enabled == 1 && frame_type == PREVIEW_VIDEO_FRAME) {
                    dst_eis_img->fd = prev_cxt->eis_video_fd;
                    dst_eis_img->addr_vir.addr_y = prev_cxt->eis_video_virt_addr;
                    dst_eis_img->addr_phy.addr_y = prev_cxt->eis_video_phys_addr;
                    dst_eis_img->size.width = prev_cxt->actual_video_size.width;
                    dst_eis_img->size.height = prev_cxt->actual_video_size.height;
                    dst_eis_img->buf_size = prev_cxt->eis_video_mem_size;
                    ipm_in_param.dst_frame = *dst_eis_img;
                    ipm_in_param.dst_frame.reserved = prev_cxt->dst_eis_video_buffer_handle;
                    ipm_in_param.private_data = (void *)&param_info;
                    CMR_LOGD("dst vir addr=0x%x, eis virt addr=0x%x, fd=%d, size=0x%x",
                        dst_img->addr_vir.addr_y, prev_cxt->eis_video_virt_addr,
                        prev_cxt->eis_video_fd, prev_cxt->eis_video_mem_size);
            } else {
                ipm_in_param.dst_frame = *dst_img;
                ipm_in_param.dst_frame.reserved = dst_buffer_handle;
                ipm_in_param.private_data = (void *)&param_info;
            }
            sem_wait(&prev_cxt->ultra_video);
            if (src_buffer_handle != NULL && dst_buffer_handle != NULL) {
                ret =
                    ipm_transfer_frame(ultra_wide_handle, &ipm_in_param, NULL);
            }
            if (prev_cxt->prev_param.sprd_eis_enabled == 1 && frame_type == PREVIEW_VIDEO_FRAME) {
                memcpy((void *)dst_img->addr_vir.addr_y, (void*)dst_eis_img->addr_vir.addr_y, prev_cxt->eis_video_mem_size);
            }
            sem_post(&prev_cxt->ultra_video);
            handle->ops.release_buff_handle(handle->oem_handle, frame_type,
                                            &buf_info);
        }
        if (ret) {
            CMR_LOGE("ultra_wide failed");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    } else {
        CMR_LOGW("ignored  prev_status %ld, frame_id 0x%x",
                 prev_cxt->prev_status, data->frame_id);
        ret = CMR_CAMERA_INVALID_STATE;
    }

exit:
    free(dst_eis_img);
    dst_eis_img = NULL;
    return ret;
}
cmr_int prev_ai_scene_send_data(struct prev_handle *handle, cmr_u32 camera_id,
                                struct img_frm *frm,
                                struct frm_info *frm_info) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = NULL;
    struct ipm_context *ipm_cxt = NULL;
    struct ipm_frame_in ipm_in_param;
    struct prev_ai_scene_info ai_scene_info;
    cmr_bzero(&ipm_in_param, sizeof(struct ipm_frame_in));
    cmr_bzero(&ai_scene_info, sizeof(struct prev_ai_scene_info));
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cxt = (struct camera_context *)handle->oem_handle;
    ipm_cxt = (struct ipm_context *)(&cxt->ipm_cxt);
    if (!ipm_cxt->ai_scene_handle) {
        CMR_LOGE("ai scene closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ipm_in_param.private_data = (void *)(&ai_scene_info);
    ai_scene_info.caller_handle = (void *)handle;
    ai_scene_info.camera_id = camera_id;
    ai_scene_info.data = *frm_info;
    ipm_in_param.src_frame = *frm;
    ipm_in_param.dst_frame = *frm;
    ipm_in_param.caller_handle = (void *)handle;
    ret = ipm_transfer_frame(ipm_cxt->ai_scene_handle, &ipm_in_param, NULL);
    if (ret) {
        CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        goto exit;
    }
exit:
    CMR_LOGV("ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_auto_tracking_open(struct prev_handle *handle, cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct ipm_open_in in_param;
    struct ipm_open_out out_param;
    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->auto_tracking_cnt = 0;

    in_param.frame_full_size.width = cxt->sn_cxt.sensor_info.source_width_max;
    in_param.frame_full_size.height = cxt->sn_cxt.sensor_info.source_height_max;
    in_param.reg_cb = prev_auto_tracking_cb;
    if (NULL == prev_cxt->auto_tracking_handle) {
        ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_AUTO_TRACKING,
                           &in_param, &out_param,
                           &prev_cxt->auto_tracking_handle);
    }
    CMR_LOGI("end prev_cxt.auto_tracking_handle:%p",prev_cxt->auto_tracking_handle);

exit:
    ATRACE_END();
    return ret;
}

cmr_int prev_auto_tracking_close(struct prev_handle *handle,
                                 cmr_u32 camera_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->auto_tracking_cnt = 0;
    prev_cxt->auto_tracking_inited = 0;
    if (prev_cxt->auto_tracking_handle) {
        ret = cmr_ipm_close(prev_cxt->auto_tracking_handle);
        prev_cxt->auto_tracking_handle = 0;
    }
    CMR_LOGI("close auto tracking done %ld", ret);

    ATRACE_END();
    return ret;
}

cmr_int prev_auto_tracking_send_data(struct prev_handle *handle,
                                     cmr_u32 camera_id, struct img_frm *frm,
                                     struct frm_info *frm_info) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_s32 tmp_X = 0;
    cmr_s32 tmp_Y = 0;
    struct camera_context *cxt = NULL;
    struct ipm_frame_in ipm_in_param;
    struct ipm_frame_out ipm_out_param;
    struct prev_auto_tracking_info at_info;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    cmr_bzero(&ipm_in_param, sizeof(struct ipm_frame_in));
    cmr_bzero(&at_info, sizeof(struct prev_auto_tracking_info));
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    cxt = (struct camera_context *)handle->oem_handle;
    if (!prev_cxt->auto_tracking_handle) {
        CMR_LOGE("auto tracking closed");
        ret = CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }

    ipm_in_param.private_data = (void *)(&at_info);
    at_info.caller_handle = (void *)handle;
    at_info.camera_id = camera_id;
    at_info.data = *frm_info;
    prev_cxt->auto_tracking_cnt++;
    at_info.frm_cnt = prev_cxt->auto_tracking_cnt;
    ipm_in_param.src_frame = *frm;
    ipm_in_param.dst_frame = *frm;
    ipm_in_param.caller_handle = (void *)handle;
    ipm_in_param.input.ot_af_status = prev_cxt->af_status;

    CMR_LOGV("in param: x=%d, y=%d", prev_cxt->auto_tracking_start_x,
             prev_cxt->auto_tracking_start_y);

    // send touch coordinate to ipm
    ipm_in_param.input.objectX = prev_cxt->auto_tracking_start_x;
    ipm_in_param.input.objectY = prev_cxt->auto_tracking_start_y;
    ipm_in_param.input.status = prev_cxt->auto_tracking_status;
    ipm_in_param.input.frame_id = prev_cxt->auto_tracking_frame_id;
    ipm_in_param.input.imageW = cxt->sn_cxt.sensor_info.source_width_max;
    ipm_in_param.input.imageH = cxt->sn_cxt.sensor_info.source_height_max;
    if (ipm_in_param.input.frame_id != prev_cxt->auto_tracking_last_frame
        && prev_cxt->auto_tracking_frame_id != 0)
        ipm_in_param.input.first_frame = 1;
    else
        ipm_in_param.input.first_frame = 0;
    CMR_LOGV("is_first_frame =%d",ipm_in_param.input.first_frame);

    ret = ipm_transfer_frame(prev_cxt->auto_tracking_handle, &ipm_in_param,
                             &ipm_out_param);
    if (ret) {
        CMR_LOGE("failed to transfer frame to ipm %ld", ret);
        goto exit;
    }
    CMR_LOGD("out param:x=%d, y=%d, status=%d, f_id=%d,",
             ipm_out_param.output.objectX, ipm_out_param.output.objectY,
             ipm_out_param.output.status, ipm_out_param.output.frame_id);
    prev_cxt->auto_tracking_last_frame = ipm_in_param.input.frame_id;
    if(prev_cxt->af_status == 0) {
       ipm_out_param.output.objectX = 0;
       ipm_out_param.output.objectY = 0;
       ipm_out_param.output.status = 0;
       ipm_out_param.output.frame_id = 0;
       CMR_LOGV("ot_af_statu_clearn");
    }

exit:
    CMR_LOGV("ret %ld", ret);
    ATRACE_END();
    return ret;
}

cmr_int prev_auto_tracking_cb(cmr_u32 class_type,
                              struct ipm_frame_out *cb_param) {
    UNUSED(class_type);

    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = NULL;
    struct prev_context *prev_cxt = NULL;
    struct camera_frame_type frame_type;
    struct prev_cb_info cb_data_info;
    cmr_u32 camera_id = CAMERA_ID_MAX;

    if (!cb_param || !cb_param->caller_handle) {
        CMR_LOGE("error param");
        return CMR_CAMERA_INVALID_PARAM;
    }

    handle = (struct prev_handle *)cb_param->caller_handle;
    camera_id = (cmr_u32)((unsigned long)cb_param->private_data);
    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    memcpy(&(frame_type.at_cb_info), &(cb_param->output),
           sizeof(struct auto_tracking_info));
    /*notify fd info directly*/
    cb_data_info.cb_type = PREVIEW_EVT_CB_AT;
    cb_data_info.func_type = PREVIEW_FUNC_START_PREVIEW;
    cb_data_info.frame_data = &frame_type;
    prev_cb_start(handle, &cb_data_info);

    return ret;
}

cmr_int prev_capture_zoom_post_cap(struct prev_handle *handle, cmr_int *flag,
                                   cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cmr_path_capability capability;
    struct prev_context *prev_cxt = NULL;

    if (!handle || !flag) {
        CMR_LOGE("in parm error");
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGV("E");

    prev_cxt = &handle->prev_cxt[camera_id];

    if (!handle->ops.channel_path_capability) {
        CMR_LOGE("ops channel_path_capability is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = handle->ops.channel_path_capability(handle->oem_handle, &capability);
    if (ret) {
        CMR_LOGE("channel_path_capability failed");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    *flag = capability.zoom_post_proc;

    CMR_LOGV("out flag %ld", *flag);
exit:
    return ret;
}

cmr_int prev_set_preview_skip_frame_num(cmr_handle preview_handle,
                                        cmr_u32 camera_id, cmr_uint skip_num,
                                        cmr_uint has_preflashed) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->prev_skip_num = prev_cxt->prev_frm_cnt + skip_num;
    prev_cxt->prev_preflash_skip_en = has_preflashed;

    return ret;
}

cmr_int prev_set_ae_time(cmr_handle preview_handle, cmr_u32 camera_id,
                         void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_uint ae_time = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    ae_time = *(cmr_uint *)data; // ns

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->ae_time = ae_time;

    return ret;
}

cmr_int prev_set_preview_touch_info(cmr_handle preview_handle,
                                    cmr_u32 camera_id,
                                    struct touch_coordinate *touch_xy) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->touch_info.touchX = (*touch_xy).touchX;
    prev_cxt->touch_info.touchY = (*touch_xy).touchY;
    CMR_LOGD("touchX %d touchY %d", prev_cxt->touch_info.touchX,
             prev_cxt->touch_info.touchY);

    return ret;
}

cmr_int cmr_preview_get_zoom_factor(cmr_handle preview_handle,
                                    cmr_u32 camera_id, struct cmr_zoom *zoom_factor) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;

    CHECK_HANDLE_VALID(handle);
    CHECK_HANDLE_VALID(zoom_factor);
    CHECK_CAMERA_ID(camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    zoom_factor->zoom_setting = prev_cxt->prev_param.zoom_setting;
    zoom_factor->prev_zoom = prev_cxt->prev_zoom;
    zoom_factor->cap_zoom = prev_cxt->cap_zoom;
    zoom_factor->video_zoom = prev_cxt->video_zoom;
    CMR_LOGD("zoom_factor is %f ", zoom_factor->zoom_setting.zoom_info.zoom_ratio);
    return ret;
}
cmr_int prev_set_vcm_step(cmr_handle preview_handle, cmr_u32 camera_id,
                          void *data) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_uint vcm_step = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    vcm_step = *(cmr_uint *)data;

    prev_cxt = &handle->prev_cxt[camera_id];
    prev_cxt->vcm_step = vcm_step;

    return ret;
}
cmr_int prev_is_need_scaling(cmr_handle preview_handle, cmr_u32 camera_id) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_int is_need_scaling = 1;
    cmr_u32 is_raw_capture = 0;
    char value[PROPERTY_VALUE_MAX];

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);
    prev_cxt = &handle->prev_cxt[camera_id];
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    // yuv no scale condition
    if ((ZOOM_BY_CAP == prev_cxt->cap_zoom_mode) &&
        (prev_cxt->cap_org_size.width == prev_cxt->actual_pic_size.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->actual_pic_size.height)) {
        is_need_scaling = 0;
        return is_need_scaling;
    }

    // raw data no scale condition
    if ((is_raw_capture) && (ZOOM_POST_PROCESS == prev_cxt->cap_zoom_mode) &&
        (prev_cxt->cap_org_size.width == prev_cxt->actual_pic_size.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->actual_pic_size.height)) {
        is_need_scaling = 0;
        return is_need_scaling;
    }

    // isp tool no scale condition
    if ((!is_raw_capture) && (ZOOM_POST_PROCESS == prev_cxt->cap_zoom_mode) &&
        (prev_cxt->cap_org_size.width == prev_cxt->actual_pic_size.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->actual_pic_size.height) &&
        prev_cxt->prev_param.tool_eb) {
        is_need_scaling = 0;
        return is_need_scaling;
    }

    // no raw data no scale condition
    if ((!is_raw_capture) && (ZOOM_POST_PROCESS == prev_cxt->cap_zoom_mode) &&
        (prev_cxt->cap_org_size.width == prev_cxt->actual_pic_size.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->actual_pic_size.height) &&
        (prev_cxt->cap_org_size.width == prev_cxt->cap_sn_trim_rect.width) &&
        (prev_cxt->cap_org_size.height == prev_cxt->cap_sn_trim_rect.height)) {
        is_need_scaling = 0;
        return is_need_scaling;
    }

    CMR_LOGD("cap_zoom_mode %ld, cap_org_size %d %d, cap_sn_trim_rect %d %d, "
             "actual_pic_size %d %d, is_raw_capture %d, is_need_scaling %ld",
             prev_cxt->cap_zoom_mode, prev_cxt->cap_org_size.width,
             prev_cxt->cap_org_size.height, prev_cxt->cap_sn_trim_rect.width,
             prev_cxt->cap_sn_trim_rect.height, prev_cxt->actual_pic_size.width,
             prev_cxt->actual_pic_size.height, is_raw_capture, is_need_scaling);

    return is_need_scaling;
}

cmr_int camera_get_mem_size(cmr_handle preview_handle, cmr_u32 camera_id) {
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = NULL;
    cmr_int ret = CMR_CAMERA_SUCCESS;
    cmr_u32 is_raw_capture = 0;
    cmr_u32 total_mem_size = 0;
    cmr_u32 channel_size = 0;
    cmr_int zoom_post_proc = 0;
    char value[PROPERTY_VALUE_MAX];
    prev_cxt = &handle->prev_cxt[camera_id];

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    if (ZOOM_POST_PROCESS == zoom_post_proc ||
        ZOOM_POST_PROCESS_WITH_TRIM == zoom_post_proc) {
        channel_size = prev_cxt->max_size.width * prev_cxt->max_size.height;
    } else {
        channel_size =
            prev_cxt->actual_pic_size.width * prev_cxt->actual_pic_size.height;
    }

    if (check_software_remosaic(prev_cxt) &&
        CAM_IMG_FMT_BAYER_MIPI_RAW == prev_cxt->cap_org_fmt) {
        ret = camera_get_4in1_postproc_capture_size(camera_id, &total_mem_size,
                                                prev_cxt->sensor_info.sn_interface.is_loose);
    } else {
        if (is_raw_capture == 1 || isp_video_get_simulation_flag()) {
            ret = camera_get_raw_postproc_capture_size(camera_id, &total_mem_size,
                                                 prev_cxt->sensor_info.sn_interface.is_loose);
        } else {
            ret = camera_get_postproc_capture_size(camera_id, &total_mem_size, channel_size);
        }
    }
    if (ret) {
        CMR_LOGE("get mem size err");
        return 0;
    }
    CMR_LOGD("cap mem size 0x%x", total_mem_size);
    return total_mem_size;
}

cmr_int
cmr_preview_set_autotracking_param(cmr_handle preview_handle, cmr_u32 camera_id,
                                   struct auto_tracking_info *input_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];
    struct common_isp_cmd_param isp_cmd_parm;
    cmr_bzero(&isp_cmd_parm, sizeof(struct common_isp_cmd_param));

    struct camera_context *cxt = (struct camera_context *)(handle->oem_handle);
    struct setting_context *setting_cxt = &cxt->setting_cxt;
    struct setting_cmd_parameter setting_param;
    cmr_uint is_autochasing_enable = 0;
    cmr_uint ot_status = 0;
    cmr_bzero(&setting_param, sizeof(setting_param));
    setting_param.camera_id = camera_id;

    ret = cmr_setting_ioctl(setting_cxt->setting_handle,
                            SETTING_GET_SPRD_AUTOCHASING_REGION_ENABLE,
                            &setting_param);
    is_autochasing_enable = setting_param.cmd_type_value;

    ret =
        cmr_setting_ioctl(setting_cxt->setting_handle,
                          SETTING_GET_SPRD_AUTOCHASING_STATUS, &setting_param);
    ot_status = setting_param.cmd_type_value;
    prev_cxt->auto_tracking_start_x = input_param->objectX;
    prev_cxt->auto_tracking_start_y = input_param->objectY;
    prev_cxt->auto_tracking_status = input_param->status;
    prev_cxt->auto_tracking_frame_id = input_param->frame_id;
    CMR_LOGD(
        "start_x=%d, start_y=%d, status=%d, f_id=%d enable=%d ot_status= %d",
        prev_cxt->auto_tracking_start_x, prev_cxt->auto_tracking_start_y,
        prev_cxt->auto_tracking_status, prev_cxt->auto_tracking_frame_id,
        is_autochasing_enable, ot_status);
    if ((prev_cxt->auto_tracking_start_x == 0) &&
        (prev_cxt->auto_tracking_start_y == 0) &&
        (is_autochasing_enable == 1) && (ot_status != 2)) {
        isp_cmd_parm.af_ot_info.status = 3;
        ret = handle->ops.isp_ioctl(
            handle->oem_handle, COM_ISP_SET_AUTO_TRACKING_INFO, &isp_cmd_parm);
        if (ret)
            CMR_LOGE("SET_AUTO_TRACKING_INFO_STATUS ERROR : %d", ret);
    }

    return ret;
}

cmr_int
cmr_preview_realloc_buffer_for_fdr(cmr_handle preview_handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    cmr_u32 width, height = 0;
    struct prev_context *prev_cxt = NULL;
    struct camera_context *cxt = (struct camera_context *)handle->oem_handle;
    struct memory_param *mem_ops = NULL;
    cmr_int zoom_post_proc = 0;
    int fdr_total_frame_num = 0;
    int fdr_ref_frame_num = 0;

    CHECK_HANDLE_VALID(handle);
    CHECK_CAMERA_ID(camera_id);

    prev_cxt = &handle->prev_cxt[camera_id];
    CMR_LOGD("E, camera_id:%d", camera_id);
    if (prev_cxt->cap_fdr_mem_num > 0) {
        CMR_LOGD("FDR raw memory is already exist!");
        goto exit;
    }

    CMR_LOGD("sensor width %d sensor height %d", prev_cxt->cap_sn_size.width, prev_cxt->cap_sn_size.height);
    cmr_bzero(prev_cxt->cap_fdr_phys_addr_array,
              (FDR_FRM_ALLOC_CNT) * sizeof(cmr_uint));
    cmr_bzero(prev_cxt->cap_fdr_virt_addr_array,
              (FDR_FRM_ALLOC_CNT) * sizeof(cmr_uint));
    cmr_bzero(prev_cxt->cap_fdr_fd_array, (FDR_FRM_ALLOC_CNT) * sizeof(cmr_s32));
    prev_capture_zoom_post_cap(handle, &zoom_post_proc, camera_id);
    mem_ops = &prev_cxt->prev_param.memory_setting;
    height = prev_cxt->cap_sn_size.height;
    width = prev_cxt->cap_sn_size.width;

    CMR_LOGD("width %d height %d", width, height);

    /*init memory info*/
    prev_cxt->cap_fdr_mem_size = (width+2) * (height+2) * 2;
#ifdef CONFIG_CAMERA_FDR
    sprd_fdr_get_max_frame_num(&fdr_total_frame_num, &fdr_ref_frame_num);
#endif
    if(fdr_total_frame_num <= FDR_FRM_ALLOC_CNT) {
        prev_cxt->cap_fdr_mem_num = fdr_total_frame_num;
    } else {
        CMR_LOGE("max fdr mem num is exceed 10");
    }
    prev_cxt->cap_fdr_mem_valid_num = 0;

    /*alloc preview buffer*/
    if (!mem_ops->alloc_mem || !mem_ops->free_mem) {
        CMR_LOGE("mem ops is null, 0x%p, 0x%p", mem_ops->alloc_mem,
                 mem_ops->free_mem);
        return CMR_CAMERA_INVALID_PARAM;
    }

    //prev_cxt->cap_zsl_mem_valid_num = 0;
    ret = mem_ops->alloc_mem(CAMERA_SNAPSHOT_ZSL_RAW, handle->oem_handle,
               (cmr_u32 *)&prev_cxt->cap_fdr_mem_size,
               (cmr_u32 *)&prev_cxt->cap_fdr_mem_num,
                prev_cxt->cap_fdr_phys_addr_array,
                prev_cxt->cap_fdr_virt_addr_array,
                prev_cxt->cap_fdr_fd_array);

    if (ret) {
        CMR_LOGE("alloc fdr memory failed");
        prev_cxt->cap_fdr_mem_num = 0;
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

exit:

    return ret;
}

cmr_int
cmr_preview_free_fdr_buffer(cmr_handle preview_handle, cmr_u32 camera_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_context *prev_cxt = NULL;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    prev_cxt = &handle->prev_cxt[camera_id];
    prev_free_zsl_raw_buf(handle, camera_id, false);
    cmr_bzero(&prev_cxt->cap_used_fdr_buf_cfg, sizeof(struct buffer_cfg));
    CMR_LOGD("free fdr buffer, camera_id:%d", camera_id);
    return ret;
}

cmr_int cmr_preview_af_status_set_to_autotracking(cmr_handle preview_handle, cmr_u32 camera_id,
                                  cmr_uint af_status) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];

    prev_cxt->af_status = af_status;
    CMR_LOGD("af_status=%d", prev_cxt->af_status);

    return ret;
}

cmr_int cmr_preview_set_fd_touch_param(cmr_handle preview_handle,
                                       cmr_u32 camera_id,
                                       struct fd_touch_info *input_param) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;
    struct prev_context *prev_cxt = &handle->prev_cxt[camera_id];

    prev_cxt->touch_info.touchX = input_param->fd_touchX;
    prev_cxt->touch_info.touchY = input_param->fd_touchY;
    return ret;
}

cmr_int cmr_preview_get_prev_aspect_ratio(cmr_handle preview_handle,
                                          cmr_u32 camera_id,
                                          float *ratio) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct prev_handle *handle = (struct prev_handle *)preview_handle;

    CHECK_HANDLE_VALID(handle);
    if (ratio) {
        struct img_size *size = &handle->prev_cxt[camera_id].actual_prev_size;

        *ratio = (float)size->width / (float)size->height;
    }

    return ret;
}
