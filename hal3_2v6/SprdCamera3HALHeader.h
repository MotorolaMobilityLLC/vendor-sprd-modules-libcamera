/* Copyright (c) 2013, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*	notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*	copyright notice, this list of conditions and the following
*	disclaimer in the documentation and/or other materials provided
*	with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*	contributors may be used to endorse or promote products derived
*	from this software without specific prior written permission.
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
#ifndef __SPRDCAMERA_HALHEADER_H__
#define __SPRDCAMERA_HALHEADER_H__

#include <hardware/camera3.h>
#include "cmr_type.h"
#include "cam_log.h"

extern "C" {
#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
}
#include <utils/Mutex.h>

using namespace android;

namespace sprdcamera {

#define MAX_NUM_STREAMS 8

extern volatile uint32_t gHalLogLevel;

#define HAL_LOGE(format, args...) LOG(LOG_PRI_E, LOG_HAL, LOG_ALL, format, ##args)
#define HAL_LOGW(format, args...) LOG(LOG_PRI_W, LOG_HAL, LOG_ALL, format, ##args)
#define HAL_LOGI(format, args...) LOG(LOG_PRI_I, LOG_HAL, LOG_ALL, format, ##args)
#define HAL_LOGD(format, args...) LOG(LOG_PRI_D, LOG_HAL, LOG_ALL, format, ##args)
#define HAL_LOGV(format, args...) LOG(LOG_PRI_V, LOG_HAL, LOG_ALL, format, ##args)

#define F_HAL_LOGE(func, format, args...) LOG(LOG_PRI_E, LOG_HAL, func, format, ##args)
#define F_HAL_LOGW(func, format, args...) LOG(LOG_PRI_W, LOG_HAL, func, format, ##args)
#define F_HAL_LOGI(func, format, args...) LOG(LOG_PRI_I, LOG_HAL, func, format, ##args)
#define F_HAL_LOGD(func, format, args...) LOG(LOG_PRI_D, LOG_HAL, func, format, ##args)
#define F_HAL_LOGV(func, format, args...) LOG(LOG_PRI_V, LOG_HAL, func, format, ##args)


class SprdCamera3Channel;

typedef enum {
    UNCONFIGURED,
    CONFIGURED,
    RECONFIGURED,
} stream_status_t;

typedef struct {
    int32_t numerator;
    int32_t denominator;
} cam_rational_type_t;

typedef struct meta_info {/*   for some metadata,   result meta should be
                             corresponding with request meta */
    uint8_t flash_mode;
    int32_t ae_regions[5];
    int32_t af_regions[5];
} meta_info_t;

typedef enum {
    CAM_FLASH_FIRING_LEVEL_0,
    CAM_FLASH_FIRING_LEVEL_1,
    CAM_FLASH_FIRING_LEVEL_2,
    CAM_FLASH_FIRING_LEVEL_3,
    CAM_FLASH_FIRING_LEVEL_4,
    CAM_FLASH_FIRING_LEVEL_5,
    CAM_FLASH_FIRING_LEVEL_6,
    CAM_FLASH_FIRING_LEVEL_7,
    CAM_FLASH_FIRING_LEVEL_8,
    CAM_FLASH_FIRING_LEVEL_9,
    CAM_FLASH_FIRING_LEVEL_10,
    CAM_FLASH_FIRING_LEVEL_MAX
} cam_flash_firing_level_t;

typedef enum {
    /* applies to HAL 1 */
    CAM_STREAM_TYPE_DEFAULT,  /* default stream type */
    CAM_STREAM_TYPE_PREVIEW,  /* preview */
    CAM_STREAM_TYPE_POSTVIEW, /* postview */
    CAM_STREAM_TYPE_SNAPSHOT, /* snapshot */
    CAM_STREAM_TYPE_VIDEO,    /* video */

    /* applies to HAL 3 */
    CAM_STREAM_TYPE_CALLBACK,         /* app requested callback */
    CAM_STREAM_TYPE_NON_ZSL_SNAPSHOT, /* non zsl snapshot */
    CAM_STREAM_TYPE_IMPL_DEFINED, /* opaque format: could be display, video enc,
                                     ZSL YUV */

    /* applies to both HAL 1 and HAL 3 */
    CAM_STREAM_TYPE_METADATA,     /* meta data */
    CAM_STREAM_TYPE_RAW,          /* raw dump from camif */
    CAM_STREAM_TYPE_OFFLINE_PROC, /* offline process */
    CAM_STREAM_TYPE_MAX,
} cam_stream_type_t;

typedef enum {
    CAM_FORMAT_JPEG = 0,
    CAM_FORMAT_YUV_420_NV12 = 1,
    CAM_FORMAT_YUV_420_NV21,
    CAM_FORMAT_YUV_420_NV21_ADRENO,
    CAM_FORMAT_YUV_420_YV12,
    CAM_FORMAT_YUV_422_NV16,
    CAM_FORMAT_YUV_422_NV61,
    CAM_FORMAT_YUV_420_NV12_VENUS,

    /* Please note below are the defintions for raw image.
    * Any format other than raw image format should be declared
    * before this line!!!!!!!!!!!!! */

    /* Note: For all raw formats, each scanline needs to be 16 bytes aligned */

    /* Packed YUV/YVU raw format, 16 bpp: 8 bits Y and 8 bits UV.
    * U and V are interleaved with Y: YUYV or YVYV */
    CAM_FORMAT_YUV_RAW_8BIT_YUYV,
    CAM_FORMAT_YUV_RAW_8BIT_YVYU,
    CAM_FORMAT_YUV_RAW_8BIT_UYVY,
    CAM_FORMAT_YUV_RAW_8BIT_VYUY,

    /* MIPI RAW formats based on MIPI CSI-2 specifiction.
    * 8BPP: Each pixel occupies one bytes, starting at LSB.
    *       Output with of image has no restrictons.
    * 10BPP: Four pixels are held in every 5 bytes. The output
    *       with of image must be a multiple of 4 pixels.
    * 12BPP: Two pixels are held in every 3 bytes. The output
    *       width of image must be a multiple of 2 pixels. */
    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GBRG,
    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_GRBG,
    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_RGGB,
    CAM_FORMAT_BAYER_MIPI_RAW_8BPP_BGGR,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GBRG,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_GRBG,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_RGGB,
    CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GBRG,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_GRBG,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_RGGB,
    CAM_FORMAT_BAYER_MIPI_RAW_12BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_8BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_10BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_MIPI_12BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN8_8BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_8BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_10BPP_BGGR,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_GBRG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_GRBG,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_RGGB,
    CAM_FORMAT_BAYER_IDEAL_RAW_PLAIN16_12BPP_BGGR,

    /* generic 8-bit raw */
    CAM_FORMAT_JPEG_RAW_8BIT,
    CAM_FORMAT_META_RAW_8BIT,

    CAM_FORMAT_MAX
} cam_format_t;

typedef struct {
    uint32_t num_streams;
    uint32_t streamID[MAX_NUM_STREAMS];
} cam_stream_ID_t;

typedef struct {
    camera3_stream_t *stream;
    buffer_handle_t *buffer;
} RequestedBufferInfo;

typedef struct {
    cam_stream_type_t type;
    cam_format_t format;
    struct img_size dimension;
    uint32_t num_buffers;
} hal_stream_info_t;

typedef struct {
    void *addr_phy;
    void *addr_vir;
    uint32_t index;
    uint64_t timestamp;
} hal3_frame_info_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    int format;
    int number;
    int phyAdd;
    int virAdd;
    int index;
} notify_frame_info_t;

enum {
    HAL3_NOTIFY_FRAME,
    HAL3_NOTIFY_EVENT,
};

enum { HAL3_EVENT_TYPE_FOCUS, HAL3_EVENT_TYPE_AE, HAL3_EVENT_TYPE_AWB };

typedef struct {
    int type;
    int result;
} notify_event_info_t;

typedef struct {
    cam_stream_type_t stream_type;
    uint64_t timestamp;
    union {
        notify_frame_info_t frame;
        notify_event_info_t event;
    };
    cmr_uint user_data;
} hal3_trans_info_t;

static const int64_t kBurstCapWaitTime =
    3000000000LL; /*be shorter than framework(4s)*/

typedef enum {
    CAMERA_STREAM_TYPE_DEFAULT,
    CAMERA_STREAM_TYPE_PREVIEW,
    CAMERA_STREAM_TYPE_VIDEO,
    CAMERA_STREAM_TYPE_CALLBACK,
    CAMERA_STREAM_TYPE_YUV2,
    CAMERA_STREAM_TYPE_ZSL_PREVIEW,
    CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT,
    CAMERA_STREAM_TYPE_MAX,
} camera_stream_type_t;

typedef enum {
    CAMERA_CHANNEL_TYPE_DEFAULT,
    CAMERA_CHANNEL_TYPE_REGULAR,
    CAMERA_CHANNEL_TYPE_PICTURE,
    CAMERA_CHANNEL_TYPE_MAX,
} camera_channel_type_t;

typedef enum {
    CAMERA_CAPTURE_MODE_DEFAULT,
    CAMERA_CAPTURE_MODE_PREVIEW,
    CAMERA_CAPTURE_MODE_STILL_CAPTURE,
    CAMERA_CAPTURE_MODE_VIDEO,
    CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT,
    CAMERA_CAPTURE_MODE_ZERO_SHUTTER_LAG,
    CAMERA_CAPTURE_MODE_CTS_YUVCALLBACK_AND_SNAPSHOT,
    CAMERA_CAPTURE_MODE_ISP_TUNING_TOOL,
    CAMERA_CAPTURE_MODE_ISP_SIMULATION_TOOL,
    CAMERA_CAPTURE_MODE_MAX,
} camera_capture_mode_t;

typedef enum {
    CAMERA_PREVIEW_IDLE,
    CAMERA_PREVIEW_IN_PROC,
    CAMERA_SNAPSHOT_IN_PROC,
    CAMERA_SNAPSHOT_IDLE,
} camera_status_t;

typedef enum {
    CAMERA_STATUS_PREVIEW,
    CAMERA_STATUS_SNAPSHOT,
} camera_status_type_t;

typedef struct {
    int32_t width;
    int32_t height;
} camera_dimension_t;

typedef struct {
    struct img_size stream_size;
    camera_stream_type_t stream_type;
    camera_channel_type_t channel_type;
} camera_stream_configure_t;

typedef enum {
    CAMERA_PREVIEW_FORMAT_DC = 0,
    CAMERA_PREVIEW_FORMAT_DV,
} camera_preview_mode_t;

typedef struct {
    bool is_urgent; /*for 3a notify*/
    uint32_t frame_number;
    // uint32_t index;
    // camera3_stream_buffer_t *buffer;
    int64_t timestamp; /**/
    camera3_stream_t *stream;
    // cam_stream_type_t strm_type;
    buffer_handle_t *buffer;
    camera3_buffer_status_t buff_status;
    camera3_msg_type_t msg_type;
    camera_metadata_t *result;
    uint8_t result_status;
} cam_result_data_info_t;

typedef enum {
    SPRD_CAMERA_MSG_INVAILED = 0,
    SPRD_CAMERA_MSG_ERROR,
    SPRD_CAMERA_MSG_SHUTTER,
} camera_msg_type_t;
}; // namespace sprdcamera

#endif
