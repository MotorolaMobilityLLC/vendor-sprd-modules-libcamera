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


namespace sprdcamera {

typedef enum {
    STATE_NOT_READY,
    STATE_IDLE,
    STATE_BUSY
}currentStatus;

typedef enum {
    /* There are valid result in both of list*/
    QUEUE_VALID = 0,
    /* There are no valid result in both of list */
    QUEUE_INVALID
} twoQuqueStatus;

typedef enum {
    MATCH_FAILED = 0,
    MATCH_SUCCESS
} matchResult;

typedef enum {
    NOTIFY_SUCCESS = 0,
    NOTIFY_ERROR,
    NOTIFY_NOT_FOUND
} notifytype;

typedef enum {
    CAMERA_LEFT = 0,
    CAMERA_RIGHT,
    MAX_CAMERA_PER_BUNDLE
} cameraIndex;

typedef enum {
    DEFAULT_DEFAULT = 0,
    PREVIEW_STREAM,
    VIDEO_STREAM,
    CALLBACK_STREAM,
    SNAPSHOT_STREAM,
} streamType_t;

typedef struct{
    uint32_t frame_number;
    int32_t vcm_steps;
    uint8_t otp_data[8192];
}depth_input_params_t;

typedef struct {
    uint32_t x_start;
    uint32_t y_start;
    uint32_t x_end;
    uint32_t y_end;
}coordinate_t;

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
    camera3_device_t* dev;
    //Camera type:main camera or aux camera
} sprdcamera_physical_descriptor_t;

typedef struct {
    // Camera Device to be shared to Frameworks
    camera3_device_t dev;
    // Camera Info
    camera_info cam_info;
    // Logical Camera Facing
    int32_t facing;
    //Main Camera Id
    uint32_t id;
} sprd_virtual_camera_t;

typedef struct {
    uint32_t frame_number;
    uint64_t timestamp;
    buffer_handle_t* buffer;
    int vcmSteps;
} hwi_frame_buffer_info_t;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    camera3_stream_t *stream;
    buffer_handle_t *buffer1;
    buffer_handle_t *buffer2;
    int vcmSteps;
} frame_matched_info_t;


typedef struct {
    uint32_t frame_number;
    buffer_handle_t*  buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t* input_buffer;
}old_video_request;

typedef struct {
    buffer_handle_t* buffer;
    MemIon *pHeapIon;
}new_mem_t;

};

#endif
