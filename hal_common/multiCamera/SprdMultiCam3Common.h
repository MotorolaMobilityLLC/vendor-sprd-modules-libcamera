/* Copyright (c) 2016, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef SPRDMULTICAMERACOMMON_H_HEADER
#define SPRDMULTICAMERACOMMON_H_HEADER

#include "../SprdCamera3HWI.h"
#ifdef ANDROID_VERSION_KK
#include <cstring>
#else
#include <string>
#endif
#include "../../external/drivers/gpu/gralloc_public.h"

namespace sprdcamera {

#define WAIT_FRAME_TIMEOUT 2000e6
#define THREAD_TIMEOUT 30e6
#define LIB_GPU_PATH "libimagestitcher.so"
#define CONTEXT_SUCCESS 1
#define CONTEXT_FAIL 0
#define TIME_DIFF (15e6)
#define MAX_CONVERED_VALURE 10
#define SFACTOR 100
#define AR4_3 133
#define AR16_9 177
#ifndef MAX_UNMATCHED_QUEUE_SIZE
#define MAX_UNMATCHED_QUEUE_SIZE 3
#endif
#ifdef CONFIG_COVERED_SENSOR
#define CAMERA3MAXFACE 11
#else
#define CAMERA3MAXFACE 10
#endif
#define MAX_F_FUMBER (10)
#define MIN_F_FUMBER (1)
#define MAX_BLUR_F_FUMBER (20)
#define MIN_BLUR_F_FUMBER (1)

#undef MAX_MULTI_NUM_STREAMS
#define MAX_MULTI_NUM_STREAMS 5
#undef MAX_MULTI_NUM_CAMERA
#define MAX_MULTI_NUM_CAMERA 4
#undef MAX_MULTI_NUM_BUFFER
#define MAX_MULTI_NUM_BUFFER 4

typedef signed int MInt32;
typedef unsigned int MUInt32;
typedef enum { STATE_NOT_READY, STATE_IDLE, STATE_BUSY } currentStatus;

typedef enum {
    /* There are valid result in both of list*/
    QUEUE_VALID = 0,
    /* There are no valid result in both of list */
    QUEUE_INVALID
} twoQuqueStatus;

typedef struct __tag_rect {
    MInt32 left;
    MInt32 top;
    MInt32 right;
    MInt32 bottom;
} MRECT, *PMRECT;

typedef enum {
    MATCH_FAILED = 0,
    MATCH_SUCCESS = 1,
} matchResult;

typedef enum {
    NOTIFY_SUCCESS = 0,
    NOTIFY_ERROR = 1,
    NOTIFY_NOT_FOUND = 2,
} notifytype;

typedef enum {
    PREVIEW_MAIN_BUFFER = 0,
    PREVIEW_DEPTH_BUFFER,
    SNAPSHOT_MAIN_BUFFER,
    SNAPSHOT_DEPTH_BUFFER,
    SNAPSHOT_TRANSFORM_BUFFER,
    DEPTH_OUT_BUFFER,
    DEPTH_OUT_WEIGHTMAP,
    YUV420,
    SNAPSHOT_SCALE_BUFFER,
    SNAPSHOT_GDEPTH_BUFFER,
    SNAP_GDEPTHJPEG_BUFFER
} camera_buffer_type_t;

typedef enum {
    DEFAULT_STREAM = 1,
    PREVIEW_STREAM = 2,
    VIDEO_STREAM = 3,
    CALLBACK_STREAM = 4,
    SNAPSHOT_STREAM = 5,
} streamType_t;

typedef enum {
    DEFAULT_STREAM_HAL_BUFFER = 1,
    DEFAULT_STREAM_FW_BUFFER = 1 << 1,
    PREVIEW_STREAM_HAL_BUFFER = 1 << 2,
    PREVIEW_STREAM_FW_BUFFER = 1 << 3,
    VIDEO_STREAM_HAL_BUFFER = 1 << 4,
    VIDEO_STREAM_FW_BUFFER = 1 << 5,
    CALLBACK_STREAM_HAL_BUFFER = 1 << 6,
    CALLBACK_STREAM_FW_BUFFER = 1 << 7,
    SNAPSHOT_STREAM_HAL_BUFFER = 1 << 8,
    SNAPSHOT_STREAM_FW_BUFFER = 1 << 9,
} streamTypeMask_t;

typedef struct {
    int type;
    int width;
    int height;
    int format;
    int follow_type;
    int follow_camera_index;
} hal_stream_info;

typedef struct {
    int number;
    int width;
    int height;
    int follow_type;
    int follow_camera_index;
} hal_buffer_info;

typedef struct {
    int roi_stream_type;
    int total_camera;
    int mn_index;
    int camera_index[MAX_MULTI_NUM_CAMERA];
    int stream_type_mask[MAX_MULTI_NUM_CAMERA];
} hal_req_stream_config;

typedef struct {
    int total_config_stream;
    hal_req_stream_config hal_req_config[MAX_MULTI_NUM_STREAMS];
} hal_req_stream_config_total;

/* Struct@ sprdcamera_physical_descriptor_t
 *
 *  Description@ This structure specifies various attributes
 *      physical cameras enumerated on the device
 */
typedef struct {
    // Userspace Physical Camera ID
    uint8_t id;
    // Camera Info
    camera_info cam_info;
    // Reference to HWI
    SprdCamera3HWI *hwi;
    // Reference to camera device structure
    camera3_device_t *dev;
    int stream_num;
    hal_stream_info hal_stream[MAX_MULTI_NUM_STREAMS];
    camera3_stream_t streams[MAX_MULTI_NUM_STREAMS];
} sprdcamera_physical_descriptor_t;

typedef struct {
    // Camera Device to be shared to Frameworks
    camera3_device_t dev;
    // Camera Info
    camera_info cam_info;
    // Logical Camera Facing
    int32_t facing;
    // Main Camera Id
    uint32_t id;
} sprd_virtual_camera_t;

/*configure mulit camera*/

typedef struct {
    uint8_t id;
    int config_stream_num;
    hal_stream_info hal_stream[MAX_MULTI_NUM_STREAMS];
} config_physical_descriptor;

typedef struct {
    int type;
    Size size;
} config_hal_stream_info;

typedef struct {
    multiCameraMode mode;
    int virtual_camera_id;
    int total_config_camera;
    hal_buffer_info buffer_info[MAX_MULTI_NUM_BUFFER];
    hal_buffer_info video_buffer_info[MAX_MULTI_NUM_BUFFER];
    config_physical_descriptor multi_phy_info[MAX_MULTI_NUM_CAMERA];
    config_physical_descriptor video_multi_phy_info[MAX_MULTI_NUM_CAMERA];
    int total_config_number;
    hal_req_stream_config_total hal_req_config_stream[MAX_MULTI_NUM_STREAMS];
    hal_req_stream_config_total video_hal_req_config_stream[MAX_MULTI_NUM_STREAMS];
} config_multi_camera;
/*configure mulit camera*/

typedef enum {
    CAMERA_LEFT = 0,
    CAMERA_RIGHT,
    MAX_CAMERA_PER_BUNDLE
} cameraIndex;

typedef struct {
    bool otp_exist;
    int otp_size;
    int otp_type;
    // uint8_t otp_data[SPRD_DUAL_OTP_SIZE];
    uint8_t otp_data[THIRD_OTP_SIZE];
} OtpData;

enum sensor_stream_ctrl {
    STREAM_OFF = 0,
    STREAM_ON = 1,
};

typedef struct {
    int preview_w;
    int preview_h;
    int callback_w;
    int callback_h;
    int capture_w;
    int capture_h;
    int transform_w;
    int transform_h;
    int depth_prev_out_w;
    int depth_prev_out_h;
    int depth_snap_out_w;
    int depth_snap_out_h;
    int depth_prev_sub_w;
    int depth_prev_sub_h;
    int depth_snap_sub_w;
    int depth_snap_sub_h;
    int depth_snap_main_w;
    int depth_snap_main_h;
    int depth_prev_size;
    int depth_weight_map_size;
    int depth_snap_size;
    int depth_prev_scale_size;
    cmr_uint depth_jepg_size;
    cmr_uint depth_yuv_normalize_size;
    cmr_uint depth_confidence_map_size;
} BokehSize;

typedef struct {
    int preview_w;
    int preview_h;
    int callback_w;
    int callback_h;
    int capture_w;
    int capture_h;
    int video_w;
    int video_h;
} stream_size_t;

typedef struct {
    uint32_t frame_number;
    int32_t vcm_steps;
    uint8_t otp_data[8192];
} depth_input_params_t;

typedef struct {
    uint32_t x_start;
    uint32_t y_start;
    uint32_t x_end;
    uint32_t y_end;
} coordinate_t;

typedef struct {
    uint32_t frame_number;
    uint64_t timestamp;
    buffer_handle_t *buffer;
    int status;
    int vcmSteps;
} hwi_frame_buffer_info_t;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    camera3_stream_t *stream;
    buffer_handle_t *buffer1; // main sensor
    int status1;
    buffer_handle_t *buffer2; // aux sensor

    buffer_handle_t *buffer3; // aux2 sensor
    buffer_handle_t *buffer4; // main sensor capture
    int status2;
    int vcmSteps;
} frame_matched_info_t;

 typedef struct {
    uint32_t frame_number;
    int x;
    int y;
} faceaf_frame_buffer_info_t;

typedef enum {
    MUXER_MSG_DATA_PROC = 1,
    MUXER_MSG_EXIT = 2,
    MUXER_MSG_INIT = 3,
} muxerMsgType;

typedef struct {
    muxerMsgType msg_type;
    frame_matched_info_t combo_frame;
} muxer_queue_msg_t;

typedef struct {
    uint32_t frame_number;
    int32_t request_id;
    bool invalid;
    int showPreviewDeviceId;
    int32_t perfectskinlevel;
    int32_t g_face_info[4];
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_t *callback_stream;
    buffer_handle_t *callback_buffer;
    camera3_stream_buffer_t *input_buffer;
    int rotation;
} old_request;

typedef struct {
    const native_handle_t *native_handle;
    sp<GraphicBuffer> graphicBuffer;
    int width;
    int height;
    void *vir_addr;
    void *phy_addr;
    camera_buffer_type_t type;
    void *private_handle;
} new_mem_t;

typedef struct {
    buffer_handle_t *left_buf;
    buffer_handle_t *right_buf;
    buffer_handle_t *dst_buf;
    int rot_angle;
} dcam_info_t;

typedef struct {
    void *handle;
    int (*initRenderContext)(struct stream_info_s *resolution,
                             float *homography_matrix, int matrix_size);
    void (*imageStitchingWithGPU)(dcam_info_t *dcam);
    void (*destroyRenderContext)(void);
} GPUAPI_t;

typedef enum {
    /* Main camera device id*/
    CAM_STEREO_MAIN_ID = 1,
    /* Aux camera device id*/
    CAM_STEREO_AUX_ID = 3
} CameraStereoID;

typedef enum {
    /* Main camera device id*/
    CAM_FACE_MAIN_ID = 1,
    /* Aux camera device id*/
    CAM_FACE_AUX_ID = 3
} CameraFaceLockID;

typedef enum {
    /* Main camera device id*/
    CAM_BLUR_MAIN_ID = 0,
    /* Main front camera device id*/
    CAM_BLUR_MAIN_ID_2 = 1,
    /* Aux camera device id*/
    CAM_BLUR_AUX_ID = 2,
    /* Aux front camera device id*/
    CAM_BLUR_AUX_ID_2 = 3,
} CameraBlurID;

typedef enum {
    /* Main camera device id*/
    CAM_PBRP_MAIN_ID = 0,
    /* Main front camera device id*/
    CAM_PBRP_MAIN_ID_2 = 1,
    /* Aux camera device id*/
    CAM_PBRP_AUX_ID = 2,
    /* Aux front camera device id*/
    CAM_PBRP_AUX_ID_2 = 3,
} CameraPortraitSceneID;


typedef enum {
    /* Main camera of the related cam subsystem which controls*/
    CAM_TYPE_MAIN = 0,
    /* Aux camera of the related cam subsystem */
    CAM_TYPE_AUX,
    CAM_TYPE_AUX1 = CAM_TYPE_AUX,
    CAM_TYPE_AUX2,
    CAM_TYPE_AUX3,
} CameraType;

struct stream_info_s {
    int src_width;
    int src_height;
    int dst_width;
    int dst_height;
    int rot_angle;
}; // stream_info_t;

typedef struct line_buf_s {
    float homography_matrix[18];
    float warp_matrix[18];
    int points[32];
} line_buf_t;

typedef enum {
    BLUR_SELFSHOT_LOWLIGHT_DISABLE = 0,
    BLUR_SELFSHOT_NO_CONVERED,
    BLUR_SELFSHOT_CONVERED,
    NORMAL_LIGHT_VALUE = 3,
    LOW_LIGHT_VALUE,
    MAX_EXIT
} sprd_convered_info_t;

typedef struct {
    uint32_t frame_number;
    buffer_handle_t *buffer;
    union {
        camera3_stream_t *stream;
        camera3_stream_t *callback_stream;
        camera3_stream_t *preview_stream;
        camera3_stream_t *snap_stream;
        camera3_stream_t *video_stream;
    };
    camera3_stream_buffer_t *input_buffer;
    int metaNotifyIndex;
} multi_request_saved_t;

enum rot_angle { ROT_0 = 0, ROT_90, ROT_180, ROT_270, ROT_MAX };
typedef enum { DARK_LIGHT = 0, LOW_LIGHT, BRIGHT_LIGHT } scene_Light;

typedef struct {
    camera_metadata_t *metadata;
    uint32_t frame_number;
} meta_save_t;

typedef enum {
    MULTI_ZOOM_RATIO_SECTION,
    MULTI_ZOOM_RATIO,
    MULTI_BURSTMODE_ENABLED,
    MULTI_ZSL_ENABLED,
    MULTI_TOUCH_INFO,
    MULTI_VCM_STEP,
    MULTI_AI_SCENE_TYPE_CURRENT,
    MULTI_FACE_ATTRIBUTES,
    MULTI_OTP_DATA,
    MULTI_APP_MODE_ID,
}multi_tag;

};
#endif
