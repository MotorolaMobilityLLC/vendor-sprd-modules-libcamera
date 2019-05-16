/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#define LOG_TAG "Cam3HWI"
//#define LOG_NDEBUG 0
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <cutils/properties.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/Trace.h>
#if (MINICAMERA != 1)
#include <ui/Fence.h>
#endif
#include "SprdCamera3HWI.h"
#include <sprd_ion.h>
#include <gralloc_public.h>

extern "C" {
#include "isp_video.h"
}

using namespace android;

namespace sprdcamera {

unsigned int SprdCamera3HWI::mCameraSessionActive = 0;
multiCameraMode SprdCamera3HWI::mMultiCameraMode = MODE_SINGLE_CAMERA;

// gHALLogLevel(default is 4):
//   1 - only show ALOGE, err log is always show
//   2 - show ALOGE and ALOGW
//   3 - show ALOGE, ALOGW and ALOGI
//   4 - show ALOGE, ALOGW, ALOGI and ALOGD
//   5 - show ALOGE, ALOGW, ALOGI and ALOGD, ALOGV
// use the following command to change gHALLogLevel:
//   adb shell setprop persist.vendor.cam.hal.log 1
volatile uint32_t gHALLogLevel = 4;

camera3_device_ops_t SprdCamera3HWI::mCameraOps = {
    .initialize = SprdCamera3HWI::initialize,
    .configure_streams = SprdCamera3HWI::configure_streams,
    .register_stream_buffers = NULL, // SprdCamera3HWI::register_stream_buffers,
    .construct_default_request_settings =
        SprdCamera3HWI::construct_default_request_settings,
    .process_capture_request = SprdCamera3HWI::process_capture_request,
    .get_metadata_vendor_tag_ops =
        NULL, // SprdCamera3HWI::get_metadata_vendor_tag_ops,
    .dump = SprdCamera3HWI::dump,
    .flush = SprdCamera3HWI::flush,
    .reserved = {0},
};

static camera3_device_t *g_cam_device = NULL;

// SprdCamera3Setting *SprdCamera3HWI::mSetting = NULL;

#define SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE 0xFF

SprdCamera3HWI::SprdCamera3HWI(int cameraId)
    : mCameraId(cameraId), mOEMIf(NULL), mCameraOpened(false),
      mCameraInitialized(false), mLastFrmNum(0), mCallbackOps(NULL),
      mInputStream(NULL), mMetadataChannel(NULL), mPictureChannel(NULL),
      mDeqBufNum(0), mRecSkipNum(0), mIsSkipFrm(false), mFlush(false),
      mFirstRequestGet(false) {
    ATRACE_CALL();

    HAL_LOGI(":hal3: Constructor E camId=%d", mCameraId);

    // for camera id 2&3 debug
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.id", value, "0");
    if (mCameraId == 0) {
        if (!strcmp(value, "2"))
            mCameraId = 2;
        else if (!strcmp(value, "4"))
            mCameraId = 4;
    } else if (mCameraId == 1) {
        if (!strcmp(value, "3"))
            mCameraId = 3;
        else if (!strcmp(value, "5"))
            mCameraId = 5;
    }

    getLogLevel();
    HAL_LOGD("mCameraId %d,mCameraDevice %p", mCameraId, &mCameraDevice);
    mCameraDevice.common.tag = HARDWARE_DEVICE_TAG;
    mCameraDevice.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    mCameraDevice.common.close = close_camera_device;
    mCameraDevice.ops = &mCameraOps;
    mCameraDevice.priv = this;
    g_cam_device = &mCameraDevice;

    mPendingRequest = 0;
    mCurrentRequestId = -1;
    mCurrentCapIntent = 0;

    mMetadataChannel = NULL;

    mRegularChan = NULL;
    mFirstRegularRequest = false;

    mPicChan = NULL;
    mPictureRequest = false;

    mBurstCapCnt = 1;

    mOldCapIntent = 0;
    mOldRequesId = 0;
    mFrameNum = 0;
    mSetting = NULL;
    mSprdCameraLowpower = 0;
    mReciveQeqMax = 0;
    mCurFrameTimeStamp = 0;
    mMasterId = 0;

    HAL_LOGI(":hal3: Constructor X");
}

int SprdCamera3HWI::getNumberOfCameras() {
    int num = SprdCamera3Setting::getNumberOfCameras();

    return num;
}

SprdCamera3HWI::~SprdCamera3HWI() {
    ATRACE_CALL();

    HAL_LOGI(":hal3: Destructor E camId=%d", mCameraId);

    HAL_LOGI(":hal3: Destructor X");
}

SprdCamera3RegularChannel *SprdCamera3HWI::getRegularChan() {
    return mRegularChan;
}

SprdCamera3PicChannel *SprdCamera3HWI::getPicChan() { return mPicChan; }

SprdCamera3OEMIf *SprdCamera3HWI::getOEMif() { return mOEMIf; }

static int ispVideoStartPreview(uint32_t param1, uint32_t param2) {
    int rtn = 0x00;
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
    SprdCamera3RegularChannel *regularChannel = dev->getRegularChan();

    HAL_LOGI("START PREVIEW");

    if (regularChannel != NULL) {
        regularChannel->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW);
        rtn = regularChannel->start(dev->mFrameNum);
    }

    return rtn;
}

static int ispVideoStopPreview(uint32_t param1, uint32_t param2) {
    int rtn = 0x00;

    HAL_LOGI("STOP PREVIEW");

    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
    SprdCamera3RegularChannel *regularChannel = dev->getRegularChan();

    if (regularChannel != NULL) {
        rtn = regularChannel->stop(dev->mFrameNum);
    }

    return rtn;
}

static int ispVideoTakePicture(uint32_t param1, uint32_t param2) {
    int rtn = 0x00;
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
    SprdCamera3PicChannel *picChannel = dev->getPicChan();

    HAL_LOGI("TAKE PICTURE");

    if (NULL != picChannel) {
        // param1 = 1 for simulation case
        if (1 == param1) {
            picChannel->setCapturePara(CAMERA_CAPTURE_MODE_ISP_SIMULATION_TOOL);
        } else {
            picChannel->setCapturePara(CAMERA_CAPTURE_MODE_ISP_TUNING_TOOL);
        }
        rtn = picChannel->start(dev->mFrameNum);
    }

    return rtn;
}

static int ispVideoSetParam(uint32_t width, uint32_t height) {
    int rtn = 0x00;
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
    SprdCamera3PicChannel *picChannel = dev->getPicChan();
    SprdCamera3OEMIf *oemIf = dev->getOEMif();
    cam_dimension_t capture_size;

    HAL_LOGI("SET PARAM");

    if (NULL != picChannel) {
        // picChannel->setCaptureSize(width,height);
        capture_size.width = width;
        capture_size.height = height;
        oemIf->setCamStreamInfo(capture_size, HAL_PIXEL_FORMAT_BLOB,
                                CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
        oemIf->setCaptureRawMode(1);
    }

    return rtn;
}

static int ispVideoSetJpegQuality(uint32_t param1, uint32_t param2) {
    int rtn = 0x00;
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
    SprdCamera3Setting *setting = dev->mSetting;
    cam_dimension_t capture_size;
    JPEG_Tag jpgInfo;

    memset(&jpgInfo, 0, sizeof(JPEG_Tag));
    jpgInfo.quality = param1;
    setting->setJPEGTag(jpgInfo);
    return 0;
}

static int ispCtrlFlash(uint32_t param, uint32_t status) {
    SprdCamera3HWI *dev =
        reinterpret_cast<SprdCamera3HWI *>(g_cam_device->priv);
    SprdCamera3OEMIf *oemIf = dev->getOEMif();
    oemIf->setIspFlashMode(status);
    return 0;
}

int SprdCamera3HWI::openCamera(struct hw_device_t **hw_device) {
    ATRACE_CALL();

    int ret = 0;

    Mutex::Autolock l(mLock);

    HAL_LOGD("camera3->open E");

    // single camera mode can only open one camera .multicamera mode can only
    // open two cameras.
    if ((mCameraSessionActive == 1 && !(isMultiCameraMode(mMultiCameraMode))) ||
        mCameraSessionActive > 2) {
        HAL_LOGE("multiple simultaneous camera instance not supported");
        return -EUSERS;
    }

    if (mCameraOpened) {
        *hw_device = NULL;
        return PERMISSION_DENIED;
    }

    ret = openCamera();
    if (ret) {
        *hw_device = NULL;
        HAL_LOGE("openCamera failed");
        return -1;
    }

    *hw_device = &mCameraDevice.common;
    mCameraSessionActive++;

    HAL_LOGD("camera3->open X mCameraSessionActive %d", mCameraSessionActive);
    return ret;
}

int SprdCamera3HWI::openCamera() {
    ATRACE_CALL();

    int ret = NO_ERROR;

    HAL_LOGI(":hal3: E camId=%d", mCameraId);

    if (mOEMIf) {
        HAL_LOGE("Failure: Camera already opened");
        return ALREADY_EXISTS;
    }

    mSetting = new SprdCamera3Setting(mCameraId);
    if (mSetting == NULL) {
        HAL_LOGE("alloc setting failed.");
        return NO_MEMORY;
    }

    mOEMIf = new SprdCamera3OEMIf(mCameraId, mSetting);
    if (!mOEMIf) {
        HAL_LOGE("alloc oemif failed.");
        if (mSetting) {
            delete mSetting;
            mSetting = NULL;
        }
        return NO_MEMORY;
    }

    mOEMIf->camera_ioctrl(CAMERA_IOCTRL_SET_MULTI_CAMERAMODE, &mMultiCameraMode,
                          NULL);
    mOEMIf->camera_ioctrl(CAMERA_IOCTRL_SET_MASTER_ID, &mMasterId, NULL);
    ret = mOEMIf->openCamera();
    if (ret) {
        HAL_LOGE("mOEMIf->openCamera failed");
        if (mOEMIf) {
            delete mOEMIf;
            mOEMIf = NULL;
        }
        if (mSetting) {
            delete mSetting;
            mSetting = NULL;
        }
        return ret;
    }
    mCameraOpened = true;

    if (mOEMIf->isIspToolMode()) {
        mOEMIf->ispToolModeInit();
        startispserver(mCameraId);
        ispvideo_RegCameraFunc(1, ispVideoStartPreview);
        ispvideo_RegCameraFunc(2, ispVideoStopPreview);
        ispvideo_RegCameraFunc(3, ispVideoTakePicture);
        ispvideo_RegCameraFunc(4, ispVideoSetParam);
        ispvideo_RegCameraFunc(5, ispVideoSetJpegQuality);
    }

    HAL_LOGI(":hal3: X");
    return NO_ERROR;
}

int SprdCamera3HWI::closeCamera() {
    ATRACE_CALL();

    int ret = NO_ERROR;
    SprdCamera3RegularChannel *regularChannel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3PicChannel *picChannel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPicChan);
    int64_t timestamp = systemTime(SYSTEM_TIME_BOOTTIME);

    HAL_LOGI(":hal3: E camId=%d", mCameraId);

    if (mOEMIf == NULL) {
        HAL_LOGE("mOEMIf is NULL, this should not happen");
        return -1;
    }

    mOEMIf->setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
    // for performance: dont delay for dc/dv switch
    mOEMIf->setSensorCloseFlag();

    if (mMetadataChannel) {
        mMetadataChannel->stop(mFrameNum);
    }
    if (mRegularChan) {
        mRegularChan->stop(mFrameNum);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_PREVIEW);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_VIDEO);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_CALLBACK);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_YUV2);
    }
    if (mPicChan) {
        mPicChan->stop(mFrameNum);
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
    }

    if (mOEMIf->isIspToolMode()) {
        stopispserver();
        ispvideo_RegCameraFunc(1, NULL);
        ispvideo_RegCameraFunc(2, NULL);
        ispvideo_RegCameraFunc(3, NULL);
        ispvideo_RegCameraFunc(4, NULL);
        ispvideo_RegCameraFunc(5, NULL);
    }

    if (mCameraSessionActive == 0) {
        mMultiCameraMode = MODE_SINGLE_CAMERA;
        mOEMIf->camera_ioctrl(CAMERA_IOCTRL_SET_MULTI_CAMERAMODE,
                              &mMultiCameraMode, NULL);
    }

    mOEMIf->closeCamera();
    delete mOEMIf;
    mOEMIf = NULL;

    if (mMetadataChannel) {
        delete mMetadataChannel;
        mMetadataChannel = NULL;
    }
    if (mRegularChan) {
        delete mRegularChan;
        mRegularChan = NULL;
    }
    if (mPicChan) {
        delete mPicChan;
        mPicChan = NULL;
    }
    if (mSetting) {
        delete mSetting;
        mSetting = NULL;
    }

    mCameraOpened = false;
    mFirstRequestGet = false;

    HAL_LOGI(":hal3: X");
    return ret;
}

int SprdCamera3HWI::initialize(
    const struct camera3_callback_ops *callback_ops) {
    ATRACE_CALL();

    int ret = 0;

    Mutex::Autolock l(mLock);

    HAL_LOGI(":hal3: E camId=%d", mCameraId);

    mCallbackOps = callback_ops;
    mCameraInitialized = true;
    mFrameNum = 0;
    mCurFrameTimeStamp = 0;

    HAL_LOGI(":hal3: X");
    return ret;
}

int SprdCamera3HWI::resetVariablesToDefault() {
    int ret = 0;

    mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
    memset(&mStreamConfiguration, 0, sizeof(cam3_stream_configuration_t));

    mFrameNum = 0;
    mFirstRegularRequest = 0;
    mPictureRequest = 0;

    mOEMIf->initialize();

    return ret;
}

camera_metadata_t *SprdCamera3HWI::constructDefaultMetadata(int type) {
    ATRACE_CALL();

    camera_metadata_t *metadata = NULL;

    HAL_LOGD("type = %d", type);

    if (!mSetting) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }
    mSetting->constructDefaultMetadata(type, &metadata);

    HAL_LOGD("X");
    return metadata;
}

int SprdCamera3HWI::checkStreamList(
    camera3_stream_configuration_t *streamList) {
    int ret = NO_ERROR;

    // Sanity check stream_list
    if (streamList == NULL) {
        HAL_LOGE("NULL stream configuration");
        return BAD_VALUE;
    }
    if (streamList->streams == NULL) {
        HAL_LOGE("NULL stream list");
        return BAD_VALUE;
    } else if (streamList->streams[0]->width == 0 ||
               streamList->streams[0]->height == 0 ||
               streamList->streams[0]->width == UINT32_MAX ||
               streamList->streams[0]->height == UINT32_MAX ||
               (uint32_t)streamList->streams[0]->format == UINT32_MAX ||
               (uint32_t)streamList->streams[0]->rotation == UINT32_MAX) {
        HAL_LOGE("INVALID stream list");
        return BAD_VALUE; /*vts configureStreamsInvalidOutputs */
    }

    if (streamList->num_streams < 1) {
        HAL_LOGE("Bad number of streams requested: %d",
                 streamList->num_streams);
        return BAD_VALUE;
    }

    return ret;
}

int32_t SprdCamera3HWI::getStreamType(camera3_stream_t *stream) {
    int32_t streamType = -1;
    if (stream == mStreamConfiguration.preview.stream) {
        streamType = CAMERA_STREAM_TYPE_PREVIEW;
    } else if (stream == mStreamConfiguration.video.stream) {
        streamType = CAMERA_STREAM_TYPE_VIDEO;
    } else if (stream == mStreamConfiguration.yuvcallback.stream) {
        streamType = CAMERA_STREAM_TYPE_CALLBACK;
    } else if (stream == mStreamConfiguration.yuv2.stream) {
        streamType = CAMERA_STREAM_TYPE_YUV2;
    } else if (stream == mStreamConfiguration.snapshot.stream) {
        streamType = CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
    }

    return streamType;
}

int32_t SprdCamera3HWI::checkStreamSizeAndFormat(camera3_stream_t *new_stream) {
    SCALER_Tag scaleInfo;
    unsigned int i;
    mSetting->getSCALERTag(&scaleInfo);

    HAL_LOGD("width = %d, height = %d, format = %d, usage = %d",
             new_stream->width, new_stream->height, new_stream->format,
             new_stream->usage);
    HAL_LOGD("size = %d", sizeof(scaleInfo.available_stream_configurations));
    for (i = 0; i < sizeof(scaleInfo.available_stream_configurations) /
                        (4 * sizeof(int32_t));
         i++) {
        if (new_stream->format ==
                (int)scaleInfo.available_stream_configurations[4 * i] &&
            new_stream->width ==
                (uint32_t)
                    scaleInfo.available_stream_configurations[4 * i + 1] &&
            new_stream->height ==
                (uint32_t)scaleInfo.available_stream_configurations[4 * i + 2])
            return NO_ERROR;
    }

    return BAD_VALUE;
}

int SprdCamera3HWI::configureStreams(
    camera3_stream_configuration_t *streamList) {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock l(mLock);

    int ret = NO_ERROR;
    size_t i;
    cam_dimension_t preview_size = {0, 0}, video_size = {0, 0};
    cam_dimension_t callback_size = {0, 0}, capture_size = {0, 0};
    cam_dimension_t yuv2_size = {0, 0};
    int previewFormat = 0, videoFormat = 0;
    int callbackFormat = 0, yuv2Format = 0, captureFormat = 0;
    int previewStreamType = 0, videoStreamType = 0;
    int callbackStreamType = 0, yuv2StreamType = 0, captureStreamType = 0;
    SPRD_DEF_Tag sprddefInfo;
    memset(&sprddefInfo, 0x00, sizeof(SPRD_DEF_Tag));
    // for two HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream
    uint32_t alreadyHasPreviewStream = 0;
    // for zero HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream
    uint32_t hasImplementationDefinedOutputStream = 0;
    // for callback stream
    uint32_t hasCallbackStream = 0;
    // for yuv2 stream
    uint32_t hasYuv2Stream = 0;
    ret = checkStreamList(streamList);
    if (ret) {
        HAL_LOGE("check failed ret=%d", ret);
        return ret;
    }

    // Create metadata channel and initialize it
    if (mMetadataChannel == NULL) {
        mMetadataChannel = new SprdCamera3MetadataChannel(
            mOEMIf, captureResultCb, mSetting, this);
        if (mMetadataChannel == NULL) {
            HAL_LOGE("failed to allocate metadata channel");
        }
    } else {
        mMetadataChannel->stop(mFrameNum);
    }

    // regular channel
    if (mRegularChan == NULL) {
        mRegularChan = new SprdCamera3RegularChannel(
            mOEMIf, captureResultCb, mSetting, mMetadataChannel,
            CAMERA_CHANNEL_TYPE_REGULAR, this);
        if (mRegularChan == NULL) {
            HAL_LOGE("channel created failed");
            return INVALID_OPERATION;
        }
    } else {
        // for performance: dont delay for dc/dv switch or front/back switch
        mOEMIf->setSensorCloseFlag();
        mRegularChan->stop(mFrameNum);
    }

    // picture channel
    if (mPicChan == NULL) {
        mPicChan = new SprdCamera3PicChannel(mOEMIf, captureResultCb, mSetting,
                                             mMetadataChannel,
                                             CAMERA_CHANNEL_TYPE_PICTURE, this);
        if (mPicChan == NULL) {
            HAL_LOGE("channel created failed");
            return INVALID_OPERATION;
        }
    } else {
        mPicChan->stop(mFrameNum);
    }

    resetVariablesToDefault();

    // for zero HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream
    for (i = 0; i < streamList->num_streams; i++) {
        if (streamList->streams[i]->stream_type == CAMERA3_STREAM_OUTPUT) {
            if (streamList->streams[i]->format ==
                    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
                streamList->streams[i]->format ==
                    HAL_PIXEL_FORMAT_YCrCb_420_SP) {
                hasImplementationDefinedOutputStream = 1;
            }
        }
    }

    mStreamConfiguration.num_streams = streamList->num_streams;

    /* Allocate channel objects for the requested streams */
    for (i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        // eis use this variable, but some chip dont support eis
        newStream->reserved[0] = NULL;
        camera_stream_type_t stream_type = CAMERA_STREAM_TYPE_DEFAULT;
        camera_channel_type_t channel_type = CAMERA_CHANNEL_TYPE_DEFAULT;

        if (newStream->stream_type == CAMERA3_STREAM_OUTPUT) {
            switch (newStream->format) {
            case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                if (newStream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                    stream_type = CAMERA_STREAM_TYPE_VIDEO;
                    channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                    newStream->usage |= GRALLOC_USAGE_SW_READ_OFTEN;
                } else if (alreadyHasPreviewStream == 0) {
                    stream_type = CAMERA_STREAM_TYPE_PREVIEW;
                    channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                    newStream->usage |= GRALLOC_USAGE_SW_READ_OFTEN;
                    newStream->usage |= GRALLOC_USAGE_PRIVATE_1;
                    // for two HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED steam
                    alreadyHasPreviewStream = 1;
                } else {
                    stream_type = CAMERA_STREAM_TYPE_CALLBACK;
                    channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                }
                break;

            case HAL_PIXEL_FORMAT_YV12:
            case HAL_PIXEL_FORMAT_YCbCr_420_888:
                if (hasImplementationDefinedOutputStream == 0) {
                    stream_type = CAMERA_STREAM_TYPE_PREVIEW;
                    channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                    // for two HAL_PIXEL_FORMAT_YCBCR_420_888 steam
                    hasImplementationDefinedOutputStream = 1;
                } else if (hasCallbackStream == 0) {
                    stream_type = CAMERA_STREAM_TYPE_CALLBACK;
                    channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                    hasCallbackStream = 1;
                } else if (hasYuv2Stream == 0) {
                    stream_type = CAMERA_STREAM_TYPE_YUV2;
                    channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                    hasYuv2Stream = 1;
                }
                break;

            case HAL_PIXEL_FORMAT_BLOB:
                stream_type = CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
                channel_type = CAMERA_CHANNEL_TYPE_PICTURE;
                break;

            case HAL_PIXEL_FORMAT_RAW16:
                stream_type = CAMERA_STREAM_TYPE_PREVIEW;
                channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
                break;

            default:
                stream_type = CAMERA_STREAM_TYPE_DEFAULT;
                break;
            }
        } else if (newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
            stream_type = CAMERA_STREAM_TYPE_CALLBACK;
            channel_type = CAMERA_CHANNEL_TYPE_REGULAR;
            newStream->format = HAL_PIXEL_FORMAT_YCbCr_420_888;
            mInputStream = newStream;
        }

        if (!mOEMIf->IommuIsEnabled()) {
            newStream->usage |= GRALLOC_USAGE_VIDEO_BUFFER;
        }

        HAL_LOGD(":hal3: stream %d: stream_type=%d, chn_type=%d, w=%d, h=%d, "
                 "format=%d",
                 i, stream_type, channel_type, newStream->width,
                 newStream->height, newStream->format);

        switch (channel_type) {
        case CAMERA_CHANNEL_TYPE_REGULAR: {
            ret = mRegularChan->addStream(stream_type, newStream);
            if (ret) {
                HAL_LOGE("addStream failed");
            }

            if (stream_type == CAMERA_STREAM_TYPE_PREVIEW) {
                preview_size.width = newStream->width;
                preview_size.height = newStream->height;
                previewFormat = newStream->format;
                previewStreamType = CAMERA_STREAM_TYPE_PREVIEW;
                mStreamConfiguration.preview.status = CONFIGURED;
                mStreamConfiguration.preview.width = newStream->width;
                mStreamConfiguration.preview.height = newStream->height;
                mStreamConfiguration.preview.format = newStream->format;
                mStreamConfiguration.preview.type = CAMERA_STREAM_TYPE_PREVIEW;
                mStreamConfiguration.preview.stream = newStream;
            } else if (stream_type == CAMERA_STREAM_TYPE_VIDEO) {
                video_size.width = newStream->width;
                video_size.height = newStream->height;
                videoFormat = newStream->format;
                videoStreamType = CAMERA_STREAM_TYPE_VIDEO;
                mStreamConfiguration.video.status = CONFIGURED;
                mStreamConfiguration.video.width = newStream->width;
                mStreamConfiguration.video.height = newStream->height;
                mStreamConfiguration.video.format = newStream->format;
                mStreamConfiguration.video.type = CAMERA_STREAM_TYPE_VIDEO;
                mStreamConfiguration.video.stream = newStream;
            } else if (stream_type == CAMERA_STREAM_TYPE_CALLBACK) {
                callback_size.width = newStream->width;
                callback_size.height = newStream->height;
                callbackFormat = newStream->format;
                callbackStreamType = CAMERA_STREAM_TYPE_CALLBACK;
                mStreamConfiguration.yuvcallback.status = CONFIGURED;
                mStreamConfiguration.yuvcallback.width = newStream->width;
                mStreamConfiguration.yuvcallback.height = newStream->height;
                mStreamConfiguration.yuvcallback.format = newStream->format;
                mStreamConfiguration.yuvcallback.type =
                    CAMERA_STREAM_TYPE_CALLBACK;
                mStreamConfiguration.yuvcallback.stream = newStream;
            } else if (stream_type == CAMERA_STREAM_TYPE_YUV2) {
                yuv2_size.width = newStream->width;
                yuv2_size.height = newStream->height;
                yuv2Format = newStream->format;
                yuv2StreamType = CAMERA_STREAM_TYPE_YUV2;
                mStreamConfiguration.yuv2.status = CONFIGURED;
                mStreamConfiguration.yuv2.width = newStream->width;
                mStreamConfiguration.yuv2.height = newStream->height;
                mStreamConfiguration.yuv2.format = newStream->format;
                mStreamConfiguration.yuv2.type = CAMERA_STREAM_TYPE_YUV2;
                mStreamConfiguration.yuv2.stream = newStream;
            }

            mSetting->getSPRDDEFTag(&sprddefInfo);
            if (preview_size.width > 3264 && preview_size.height > 2448)
                SprdCamera3RegularChannel::kMaxBuffers = 2;
            else if (sprddefInfo.slowmotion > 1) {
                SprdCamera3RegularChannel::kMaxBuffers = 16;
                if (stream_type == CAMERA_STREAM_TYPE_PREVIEW)
                    SprdCamera3RegularChannel::kMaxBuffers = 4;
            } else if (sprddefInfo.sprd_eis_enabled) {
                /* for sprd_eis_enable,eis video_size=normal video_size+2*/
                SprdCamera3RegularChannel::kMaxBuffers = 16;
            } else
                SprdCamera3RegularChannel::kMaxBuffers = 4;
            HAL_LOGD("slowmotion=%d, kMaxBuffers=%d", sprddefInfo.slowmotion,
                     SprdCamera3RegularChannel::kMaxBuffers);

            newStream->max_buffers = SprdCamera3RegularChannel::kMaxBuffers;
            newStream->priv = mRegularChan;
            break;
        }

        case CAMERA_CHANNEL_TYPE_PICTURE: {
            ret = mPicChan->addStream(stream_type, newStream);
            if (ret) {
                HAL_LOGE("addStream failed");
            }

            if (stream_type == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                capture_size.width = newStream->width;
                capture_size.height = newStream->height;
                captureFormat = newStream->format;
                captureStreamType = CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
                mStreamConfiguration.snapshot.status = CONFIGURED;
                mStreamConfiguration.snapshot.width = newStream->width;
                mStreamConfiguration.snapshot.height = newStream->height;
                mStreamConfiguration.snapshot.format = newStream->format;
                mStreamConfiguration.snapshot.type =
                    CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
                mStreamConfiguration.snapshot.stream = newStream;
            }

            newStream->priv = mPicChan;
            newStream->max_buffers = SprdCamera3PicChannel::kMaxBuffers;
            mPictureRequest = false;
            break;
        }

        default:
            HAL_LOGE("channel type is invalid channel");
            break;
        }
    }

    if (mMultiCameraMode == MODE_BOKEH && mCameraId == 2 &&
        mStreamConfiguration.snapshot.width == 0 &&
        mStreamConfiguration.snapshot.height == 0) {
        capture_size.width = mStreamConfiguration.yuvcallback.width;
        capture_size.height = mStreamConfiguration.yuvcallback.height;
        captureFormat = HAL_PIXEL_FORMAT_BLOB;
        captureStreamType = CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
        mStreamConfiguration.snapshot.status = CONFIGURED;
        mStreamConfiguration.snapshot.width = capture_size.width;
        mStreamConfiguration.snapshot.height = capture_size.height;
        mStreamConfiguration.snapshot.format = captureFormat;
        mStreamConfiguration.snapshot.type =
            CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT;
        mStreamConfiguration.num_streams = 3;
    }

    mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
    mOEMIf->SetChannelHandle(mRegularChan, mPicChan);

    HAL_LOGI(":hal3: camId=%d, prev: w=%d, h=%d, video: w=%d, h=%d, callback: "
             "w=%d, h=%d, yuv2: w=%d, h=%d, cap: w=%d, h=%d",
             mCameraId, preview_size.width, preview_size.height,
             video_size.width, video_size.height, callback_size.width,
             callback_size.height, yuv2_size.width, yuv2_size.height,
             capture_size.width, capture_size.height);

#ifdef CONFIG_CAMERA_EIS
    if (sprddefInfo.sprd_eis_enabled) {
        // leave two*height*1.5 bytes space for eis parameters
        video_size.width = video_size.width - 2;
    }
#endif

    // sharkl5 or later dont need these code
    // workaround jpeg cant handle 16-noalign issue, when jpeg fix this
    // issue, we will remove these code
    // if (capture_size.height == 1944 && capture_size.width == 2592) {
    //    capture_size.height = 1952;
    //} else if (capture_size.height == 1836 && capture_size.width == 3264) {
    //    capture_size.height = 1840;
    //} else if (capture_size.height == 360 && capture_size.width == 640) {
    //    capture_size.height = 368;
    //}

    mOEMIf->setCamStreamInfo(preview_size, previewFormat, previewStreamType);
    mOEMIf->setCamStreamInfo(capture_size, captureFormat, captureStreamType);
    mOEMIf->setCamStreamInfo(video_size, videoFormat, videoStreamType);
    mOEMIf->setCamStreamInfo(callback_size, callbackFormat, callbackStreamType);
    mOEMIf->setCamStreamInfo(yuv2_size, yuv2Format, yuv2StreamType);

    // need to update crop region each time when ConfigureStreams
    mOEMIf->setCameraConvertCropRegion();

    mSetting->setPreviewSize(preview_size);
    mSetting->setVideoSize(video_size);
    mSetting->setPictureSize(capture_size);

    mReciveQeqMax = SprdCamera3RegularChannel::kMaxBuffers;
    mFirstRequestGet = false;
    mPendingRequestsList.clear();

    return ret;
}

int SprdCamera3HWI::registerStreamBuffers(
    const camera3_stream_buffer_set_t *buffer_set) {
    int ret = 0;
    Mutex::Autolock l(mLock);

    if (buffer_set == NULL) {
        HAL_LOGE("Invalid buffer_set parameter.");
        return -EINVAL;
    }
    if (buffer_set->stream == NULL) {
        HAL_LOGE("Invalid stream parameter.");
        return -EINVAL;
    }
    if (buffer_set->num_buffers < 1) {
        HAL_LOGE("Invalid num_buffers %d.", buffer_set->num_buffers);
        return -EINVAL;
    }
    if (buffer_set->buffers == NULL) {
        HAL_LOGE("Invalid buffers parameter.");
        return -EINVAL;
    }

    SprdCamera3Channel *channel = reinterpret_cast<SprdCamera3Channel *>(
        buffer_set->stream->priv); //(SprdCamera3Channel *) stream->priv;
    if (channel) {
        ret = channel->registerBuffers(buffer_set);
        if (ret < 0) {
            HAL_LOGE("registerBUffers for stream %p failed",
                     buffer_set->stream);
            return -ENODEV;
        }
    }
    return NO_ERROR;
}

int SprdCamera3HWI::validateCaptureRequest(camera3_capture_request_t *request) {
    size_t idx = 0;
    const camera3_stream_buffer_t *b = NULL;

    /* Sanity check the request */
    if (request == NULL) {
        HAL_LOGE("NULL capture request");
        return BAD_VALUE;
    }

    uint32_t frameNumber = request->frame_number;
    if (!mFirstRequestGet) {
        mFirstRequestGet = true;
        if (request->settings == NULL) {
            HAL_LOGE("NULL capture request settings");
            return BAD_VALUE;
        }
    }
    if (request->input_buffer != NULL &&
        request->input_buffer->stream ==
            NULL) { /**modified for 3d capture, enable reprocessing*/
        HAL_LOGE("Request %d: Input buffer not from input stream!",
                 frameNumber);
        return BAD_VALUE;
    }
    if (request->num_output_buffers < 1 || request->output_buffers == NULL) {
        HAL_LOGE("Request %d: No output buffers provided!", frameNumber);
        return BAD_VALUE;
    }
    if (request->input_buffer != NULL) {
        b = request->input_buffer;
        if (b == NULL || b->stream == NULL || b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %d: input_buffer parameter is NULL!",
                     frameNumber, idx);
            return BAD_VALUE;
        }
        SprdCamera3Channel *channel =
            static_cast<SprdCamera3Channel *>(b->stream->priv);
        if (channel == NULL) {
            HAL_LOGE("Request %d: Buffer %d: Unconfigured stream!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
    }
    // Validate all buffers
    b = request->output_buffers;
    do {
        if (b == NULL || b->stream == NULL || b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %d: output_buffers parameter is NULL!",
                     frameNumber, idx);
            return BAD_VALUE;
        }
        HAL_LOGV("strm=0x%lx hdl=0x%lx b=0x%lx", (cmr_uint)b->stream,
                 (cmr_uint)b->buffer, (cmr_uint)b);
        SprdCamera3Channel *channel =
            static_cast<SprdCamera3Channel *>(b->stream->priv);
        if (channel == NULL) {
            HAL_LOGE("Request %d: Buffer %d: Unconfigured stream!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        idx++;
        b = request->output_buffers + idx;
    } while (idx < (size_t)request->num_output_buffers);

    return NO_ERROR;
}

void SprdCamera3HWI::flushRequest(uint32_t frame_num) {
    ATRACE_CALL();

    HAL_LOGI(":hal3: E");

    SprdCamera3RegularChannel *regularChannel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3PicChannel *picChannel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPicChan);
    int64_t timestamp = 0;

    if (mMetadataChannel)
        mMetadataChannel->stop(mFrameNum);
    if (mRegularChan)
        mRegularChan->stop(mFrameNum);
    if (mPicChan)
        mPicChan->stop(mFrameNum);

    timestamp = systemTime(SYSTEM_TIME_BOOTTIME);
    if (regularChannel) {
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_PREVIEW);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_VIDEO);
        regularChannel->channelClearAllQBuff(timestamp,
                                             CAMERA_STREAM_TYPE_CALLBACK);
    }

    if (picChannel) {
        picChannel->channelClearAllQBuff(timestamp,
                                         CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
    }
    HAL_LOGI(":hal3: X");
}

void SprdCamera3HWI::getLogLevel() {
    char value[PROPERTY_VALUE_MAX];
    int val = 0;
    int turn_off_flag = 0;

    // to turn off camera log:
    // adb shell setprop persist.vendor.cam.log off
    property_get("persist.vendor.cam.log", value, "on");
    if (!strcmp(value, "off")) {
        turn_off_flag = 1;
    }

    // user verson camera log dont print >= LOGD
    property_get("ro.debuggable", value, "1");
    if (!strcmp(value, "0") || turn_off_flag) {
        gHALLogLevel = LEVEL_OVER_LOGI;
    }

    property_get("persist.vendor.cam.hal.log", value, "0");
    val = atoi(value);
    if (val > 0) {
        gHALLogLevel = (uint32_t)val;
    }
}

int SprdCamera3HWI::processCaptureRequest(camera3_capture_request_t *request) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    CameraMetadata meta;
    SprdCamera3Stream *pre_stream = NULL;
    int receive_req_max = mReciveQeqMax;
    int32_t width = 0, height = 0;
    int64_t timestamp = 0;
    size_t i = 0;
    int32_t streamType[4] = {0, 0, 0, 0};
    uint8_t captureIntent = 0;
    int32_t captureRequestId = 0;
    int32_t jpegOrientation = 0;
    uint32_t frameNumber = request->frame_number;
    PendingRequestInfo pendingRequest;
    FLASH_Tag flashInfo;
    CONTROL_Tag controlInfo;
    SPRD_DEF_Tag sprddefInfo;
    REQUEST_Tag requestInfo;

    Mutex::Autolock l(mLock);

    ret = validateCaptureRequest(request);
    if (ret) {
        HAL_LOGE("incoming request is not valid");
        goto exit;
    }
    mFrameNum = request->frame_number;
    // For auto tracking to save frame num
    mSetting->getREQUESTTag(&requestInfo);
    requestInfo.frame_id = mFrameNum;
    mSetting->setREQUESTTag(&requestInfo);
    HAL_LOGV("frame_id = %d", requestInfo.frame_id);

    meta = request->settings;
    mMetadataChannel->request(meta);

    mSetting->getSPRDDEFTag(&sprddefInfo);
    mSetting->getCONTROLTag(&controlInfo);
    mSetting->getFLASHTag(&flashInfo);

    captureIntent = controlInfo.capture_intent;

    if (meta.exists(ANDROID_REQUEST_ID)) {
        captureRequestId = meta.find(ANDROID_REQUEST_ID).data.i32[0];
    }

    // check if need to stop offline zsl
    mOEMIf->checkIfNeedToStopOffLineZsl();

    for (i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t &output = request->output_buffers[i];
        camera3_stream_t *stream = output.stream;
        streamType[i] = getStreamType(stream);
        HAL_LOGD("streamType[%d]=%d", i, streamType[i]);

        if (streamType[i] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
            if (meta.exists(ANDROID_JPEG_ORIENTATION)) {
                jpegOrientation =
                    meta.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
                if (jpegOrientation % 90) {
                    jpegOrientation = 0;
                }
            }
            mOEMIf->setJpegOrientation(jpegOrientation);
        }
    }
    // for cts:
    // testMandatoryOutputCombinations, testSingleCapture
    if (mStreamConfiguration.num_streams == 1 &&
        mStreamConfiguration.snapshot.status == CONFIGURED) {
        captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
    } else if (mStreamConfiguration.num_streams == 1 &&
               mStreamConfiguration.preview.status == CONFIGURED) {
        captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    } else if (mStreamConfiguration.num_streams == 1 &&
               mStreamConfiguration.yuvcallback.status == CONFIGURED) {
        captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    } else if (mStreamConfiguration.num_streams == 2 &&
               mStreamConfiguration.preview.status == CONFIGURED &&
               mStreamConfiguration.yuvcallback.status == CONFIGURED) {
        captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    } else if (mStreamConfiguration.num_streams == 3 &&
               mStreamConfiguration.preview.status == CONFIGURED &&
               mStreamConfiguration.yuvcallback.status == CONFIGURED &&
               mStreamConfiguration.snapshot.status == CONFIGURED) {
        captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    } else if (mStreamConfiguration.num_streams == 3 &&
               mStreamConfiguration.preview.status == CONFIGURED &&
               mStreamConfiguration.video.status == CONFIGURED &&
               mStreamConfiguration.snapshot.status == CONFIGURED &&
               captureIntent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE) {
        captureIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
    }

    switch (captureIntent) {
    case ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW:
        if (mStreamConfiguration.num_streams == 3 &&
            mStreamConfiguration.preview.status == CONFIGURED &&
            mStreamConfiguration.yuvcallback.status == CONFIGURED &&
            mStreamConfiguration.snapshot.status == CONFIGURED) {
            if (mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE) {
                // when sensor_rotation is 1 for volte, volte dont need capture
                if (sprddefInfo.sensor_rotation == 0) {
                    mOEMIf->setStreamOnWithZsl();
                }
                mFirstRegularRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW, mFrameNum);
                if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                    streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                    streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                    mPictureRequest = 1;
                    mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                           mFrameNum);
                }
                break;
            }

            if (mMultiCameraMode == MODE_BLUR ||
                mMultiCameraMode == MODE_3D_CAPTURE ||
                mMultiCameraMode == MODE_3D_CALIBRATION ||
                mMultiCameraMode == MODE_BOKEH) {
                if (streamType[0] == CAMERA_STREAM_TYPE_CALLBACK ||
                    streamType[1] == CAMERA_STREAM_TYPE_CALLBACK ||
                    streamType[2] == CAMERA_STREAM_TYPE_CALLBACK) {
                    HAL_LOGD("call back stream request");
                    mOEMIf->setCallBackYuvMode(1);
                    mPictureRequest = 1;
                    mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                           mFrameNum);
                    break;
                }
            }

            if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                mPictureRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                       mFrameNum);
            }
            break;
        }

        if (mStreamConfiguration.num_streams == 2 &&
            mStreamConfiguration.preview.status == CONFIGURED &&
            mStreamConfiguration.snapshot.status == CONFIGURED) {
            // raw capture need non-zsl for now
            if (mOEMIf->isRawCapture() || mOEMIf->isIspToolMode()) {
                if (mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE ||
                    mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW) {
                    mFirstRegularRequest = 1;
                    mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW,
                                           mFrameNum);
                }
                break;
            }

            if (mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE) {
                // when sensor_rotation is 1 for volte, volte dont need capture
                if (sprddefInfo.sensor_rotation == 0) {
                    mOEMIf->setStreamOnWithZsl();
                }
                mFirstRegularRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW, mFrameNum);
                if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                    streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                    streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                    mPictureRequest = 1;
                    mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                           mFrameNum);
                }
                break;
            }

            if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                mPictureRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                       mFrameNum);
            }
            break;
        }

        if (mStreamConfiguration.num_streams == 2 &&
            mStreamConfiguration.preview.status == CONFIGURED &&
            mStreamConfiguration.yuvcallback.status == CONFIGURED &&
            mMultiCameraMode == MODE_3D_CALIBRATION) {
            if (mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE) {
                mOEMIf->setStreamOnWithZsl();

                HAL_LOGD("call back stream request");
                mOEMIf->setCallBackYuvMode(1);
                if (streamType[0] == CAMERA_STREAM_TYPE_CALLBACK ||
                    streamType[1] == CAMERA_STREAM_TYPE_CALLBACK) {
                    mPictureRequest = 1;
                    mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                           mFrameNum);
                    break;
                }

                mFirstRegularRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW, mFrameNum);
                break;
            }

            if (streamType[0] == CAMERA_STREAM_TYPE_CALLBACK ||
                streamType[1] == CAMERA_STREAM_TYPE_CALLBACK) {
                mPictureRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                       mFrameNum);
            }
            break;
        }

        if (mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE) {
            if (mStreamConfiguration.snapshot.status == CONFIGURED) {
                mOEMIf->setStreamOnWithZsl();
            }
            mFirstRegularRequest = 1;
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW, mFrameNum);
            if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                mPictureRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                       mFrameNum);
            }
            break;
        }

        if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
            streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
            streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
            mPictureRequest = 1;
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                   mFrameNum);
        }
        break;

    case ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE:
        // raw capture need non-zsl for now
        if (mOEMIf->isRawCapture() || mOEMIf->isIspToolMode()) {
            mPictureRequest = 1;
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                   mFrameNum);
            break;
        }

        if (mOldCapIntent == SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE) {
            if (mStreamConfiguration.snapshot.status == CONFIGURED) {
                mOEMIf->setStreamOnWithZsl();
            }
            mFirstRegularRequest = 1;
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_PREVIEW, mFrameNum);
            if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                mPictureRequest = 1;
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                       mFrameNum);
            }
            break;
        }

        if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
            streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
            streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
            mPictureRequest = 1;
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_STILL_CAPTURE,
                                   mFrameNum);
        }
        break;

    case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD:
        if (mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD &&
            mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
            mFirstRegularRequest = true;
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO, mFrameNum);
        }

        if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
            streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
            streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
            if (mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD) {
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT,
                                       mFrameNum);
                mPictureRequest = true;
            }
        }
        break;

    case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT:
        if (mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD ||
            mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
            if (streamType[0] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[1] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT ||
                streamType[2] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT,
                                       mFrameNum);
                mPictureRequest = true;
            }
        }
        break;

    case ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
        if (mOldCapIntent != ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG) {
            mOEMIf->setCapturePara(CAMERA_CAPTURE_MODE_ZERO_SHUTTER_LAG,
                                   mFrameNum);
            mFirstRegularRequest = true;
        }
        break;

    default:
        break;
    }

    if (mOldCapIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD &&
        captureIntent == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
        mOldCapIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
    } else {
        mOldCapIntent = captureIntent;
    }

    if (captureRequestId == 0)
        captureRequestId = mOldRequesId;
    else
        mOldRequesId = captureRequestId;

    HAL_LOGD("camId=%d, bufs_num=%d, frame_num=%d, cap_intent=%d, pic_req=%d, "
             "first_regular_req=%d",
             mCameraId, request->num_output_buffers, request->frame_number,
             captureIntent, mPictureRequest, mFirstRegularRequest);

    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t &output = request->output_buffers[i];
        sp<Fence> acquireFence = new Fence(output.acquire_fence);

        ret = acquireFence->wait(Fence::TIMEOUT_NEVER);
        if (ret) {
            HAL_LOGE("fence wait failed %d", ret);
            goto exit;
        }

        acquireFence = NULL;
    }

    pendingRequest.meta_info.flash_mode = flashInfo.mode;
    memcpy(pendingRequest.meta_info.ae_regions, controlInfo.ae_regions,
           5 * sizeof(controlInfo.ae_regions[0]));
    memcpy(pendingRequest.meta_info.af_regions, controlInfo.af_regions,
           5 * sizeof(controlInfo.af_regions[0]));
    pendingRequest.frame_number = frameNumber;
    pendingRequest.num_buffers = request->num_output_buffers;
    pendingRequest.request_id = captureRequestId;
    pendingRequest.bNotified = 0;
    pendingRequest.input_buffer = request->input_buffer;
    pendingRequest.pipeline_depth = 0;

    if (mFlush) {
        for (i = 0; i < request->num_output_buffers; i++) {
            const camera3_stream_buffer_t &output = request->output_buffers[i];
            camera3_stream_t *stream = output.stream;
            RequestedBufferInfo requestedBuf;
            SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;
            if (channel == NULL) {
                HAL_LOGE("invalid channel pointer for stream");
                continue;
            }
            requestedBuf.stream = output.stream;
            requestedBuf.buffer = output.buffer;
            pendingRequest.buffers.push_back(requestedBuf);
        }
        pendingRequest.receive_req_max = receive_req_max;

        {
            Mutex::Autolock lr(mRequestLock);
            mPendingRequestsList.push_back(pendingRequest);
            mPendingRequest++;
        }

        for (i = 0; i < request->num_output_buffers; i++) {
            const camera3_stream_buffer_t &output = request->output_buffers[i];
            camera3_stream_t *stream = output.stream;
            SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;

            if (channel == NULL) {
                HAL_LOGE("invalid channel pointer for stream");
                continue;
            }

            ret = channel->request(stream, output.buffer, frameNumber);
            if (ret) {
                HAL_LOGE("channel->request failed %p (%d)", output.buffer,
                         frameNumber);
                continue;
            }
        }

        HAL_LOGI(":hal3: mFlush=1");
        goto exit;
    }

    mMetadataChannel->start(mFrameNum);

    if (mFirstRegularRequest == 1) {
        ret = mRegularChan->start(mFrameNum);
        if (ret) {
            HAL_LOGE("mRegularChan->start failed, ret=%d", ret);
            goto exit;
        }
        mFirstRegularRequest = 0;
    }

    {
        for (i = 0; i < request->num_output_buffers; i++) {
            const camera3_stream_buffer_t &output = request->output_buffers[i];
            camera3_stream_t *stream = output.stream;
            RequestedBufferInfo requestedBuf;
            SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;
            if (channel == NULL) {
                HAL_LOGE("invalid channel pointer for stream");
                continue;
            }
            requestedBuf.stream = output.stream;
            requestedBuf.buffer = output.buffer;
            pendingRequest.buffers.push_back(requestedBuf);
        }

        pendingRequest.receive_req_max = receive_req_max;

        Mutex::Autolock lr(mRequestLock);
        mPendingRequestsList.push_back(pendingRequest);
        mPendingRequest++;
    }

    for (i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t &output = request->output_buffers[i];
        camera3_stream_t *stream = output.stream;
        SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;

        if (channel == NULL) {
            HAL_LOGE("invalid channel pointer for stream");
            continue;
        }

        ret = channel->request(stream, output.buffer, frameNumber);
        if (ret) {
            HAL_LOGE("channel->request failed %p (%d)", output.buffer,
                     frameNumber);
            continue;
        }
    }

    if (request->input_buffer != NULL) {
        const camera3_stream_buffer_t *input = request->input_buffer;
        camera3_stream_t *stream = input->stream;
        SprdCamera3Channel *channel = (SprdCamera3Channel *)stream->priv;

        if (channel == NULL) {
            HAL_LOGE("invalid channel pointer for stream");
            goto exit;
        }

        HAL_LOGD("input->buffer = %p frameNumber = %d, format = %d",
                 input->buffer, frameNumber, stream->format);
        if (stream->format == HAL_PIXEL_FORMAT_BLOB) {
            mPictureRequest = true;
            mOEMIf->setCaptureReprocessMode(true, stream->width,
                                            stream->height);
        }
        ret = mRegularChan->setInputBuff(input->buffer);
        if (ret) {
            HAL_LOGE("setInputBuff failed %p (%d)", input->buffer, frameNumber);
            goto exit;
        }
    }

    if (mPictureRequest == 1) {
        ret = mPicChan->start(mFrameNum);
        if (ret) {
            HAL_LOGE("mPicChan->start failed, ret=%d", ret);
            goto exit;
        }
        mPictureRequest = 0;
    }

    {
        Mutex::Autolock lr(mRequestLock);
        size_t pendingCount = 0;
        while (mPendingRequest >= receive_req_max) {
            mRequestSignal.waitRelative(mRequestLock, kPendingTime);
            if (pendingCount > kPendingTimeOut / kPendingTime) {
                HAL_LOGE("Timeout pendingCount=%d", pendingCount);
                ret = -ENODEV;
                break;
            }
            if (mFlush) {
                HAL_LOGI("mFlush = %d", mFlush);
                break;
            }
            pendingCount++;
        }
    }

    if (ret == -ENODEV)
        flushRequest(request->frame_number);

exit:
    return ret;
}

void SprdCamera3HWI::handleCbDataWithLock(cam_result_data_info_t *result_info) {
    ATRACE_CALL();

    Mutex::Autolock l(mResultLock);

    uint32_t frame_number = result_info->frame_number;
    buffer_handle_t *buffer = result_info->buffer;
    int64_t capture_time = result_info->timestamp;
    bool is_urgent = result_info->is_urgent;
    camera3_stream_t *stream = result_info->stream;
    camera3_buffer_status_t buffer_status = result_info->buff_status;
    camera3_msg_type_t msg_type = result_info->msg_type;
    int oldrequest = mPendingRequest;
    int num_buffers = 0;
    SprdCamera3Stream *pre_stream = NULL;
    int receive_req_max = SprdCamera3RegularChannel::kMaxBuffers;
    int32_t width = 0, height = 0;

    for (List<PendingRequestInfo>::iterator i = mPendingRequestsList.begin();
         i != mPendingRequestsList.end();) {
        camera3_capture_result_t result;
        camera3_notify_msg_t notify_msg;

        if (i->frame_number < frame_number) {
            HAL_LOGD("i->frame_num=%d, frame_num=%d, i->req_id=%d",
                     i->frame_number, frame_number, i->request_id);

            /**add for 3d capture reprocessing begin   */
            HAL_LOGV("result stream format =%d", result_info->stream->format);
            if (NULL != i->input_buffer) {
                HAL_LOGI("reprocess capture request, continue search");
                i++;
                continue;
            }
            /**add for 3d capture reprocessing end   */

            if (!i->bNotified) {
                notify_msg.type = CAMERA3_MSG_SHUTTER;
                notify_msg.message.shutter.frame_number = i->frame_number;
                notify_msg.message.shutter.timestamp = capture_time;
                mCallbackOps->notify(mCallbackOps, &notify_msg);
                i->bNotified = true;
                HAL_LOGD("drop msg frame_num = %d, timestamp = %" PRId64,
                         i->frame_number, capture_time);

                SENSOR_Tag sensorInfo;
                REQUEST_Tag requestInfo;
                meta_info_t metaInfo;
                mSetting->getSENSORTag(&sensorInfo);
                sensorInfo.timestamp = capture_time;
                mSetting->setSENSORTag(sensorInfo);
                mSetting->getREQUESTTag(&requestInfo);
                requestInfo.id = i->request_id;
                requestInfo.frame_count = i->frame_number;
                requestInfo.pipeline_depth =
                    (i->pipeline_depth == 0)
                        ? 1
                        : i->pipeline_depth; // in case of 0, Burst do not fire
                                             // in testYUVBurst so use 1 as min
                mSetting->setREQUESTTag(&requestInfo);
                metaInfo.flash_mode = i->meta_info.flash_mode;
                memcpy(metaInfo.ae_regions, i->meta_info.ae_regions,
                       5 * sizeof(i->meta_info.ae_regions[0]));
                memcpy(metaInfo.af_regions, i->meta_info.af_regions,
                       5 * sizeof(i->meta_info.af_regions[0]));
                mSetting->setMETAInfo(metaInfo);

                result.result = mSetting->translateLocalToFwMetadata();
                result.frame_number = i->frame_number;
                result.num_output_buffers = 0;
                result.output_buffers = NULL;
                result.input_buffer = NULL;
                result.partial_result = 1;
                mCallbackOps->process_capture_result(mCallbackOps, &result);
                free_camera_metadata(
                    const_cast<camera_metadata_t *>(result.result));
            }
            i++;
        } else if (i->frame_number == frame_number) {
            HAL_LOGD("i->frame_num=%d, frame_num=%d, i->req_id=%d",
                     i->frame_number, frame_number, i->request_id);

            if (!i->bNotified) {
                notify_msg.type = CAMERA3_MSG_SHUTTER;
                notify_msg.message.shutter.frame_number = i->frame_number;
                notify_msg.message.shutter.timestamp = capture_time;
                mCallbackOps->notify(mCallbackOps, &notify_msg);
                i->bNotified = true;
                HAL_LOGV("notified frame_num = %d, timestamp = %" PRId64,
                         i->frame_number, notify_msg.message.shutter.timestamp);

                SENSOR_Tag sensorInfo;
                REQUEST_Tag requestInfo;
                meta_info_t metaInfo;
                mSetting->getSENSORTag(&sensorInfo);
                sensorInfo.timestamp = capture_time;
                mSetting->setSENSORTag(sensorInfo);
                mSetting->getREQUESTTag(&requestInfo);
                requestInfo.id = i->request_id;
                requestInfo.frame_count = i->frame_number;
                requestInfo.pipeline_depth =
                    (i->pipeline_depth == 0)
                        ? 1
                        : i->pipeline_depth; // in case of 0, Burst do not fire
                                             // in testYUVBurst so use 1 as min
                mSetting->setREQUESTTag(&requestInfo);
                metaInfo.flash_mode = i->meta_info.flash_mode;
                memcpy(metaInfo.ae_regions, i->meta_info.ae_regions,
                       5 * sizeof(i->meta_info.ae_regions[0]));
                memcpy(metaInfo.af_regions, i->meta_info.af_regions,
                       5 * sizeof(i->meta_info.af_regions[0]));
                mSetting->setMETAInfo(metaInfo);

                result.result = mSetting->translateLocalToFwMetadata();
                result.frame_number = i->frame_number;
                result.num_output_buffers = 0;
                result.output_buffers = NULL;
                result.input_buffer = NULL;
                result.partial_result = 1;
                mCallbackOps->process_capture_result(mCallbackOps, &result);
                free_camera_metadata(
                    const_cast<camera_metadata_t *>(result.result));
            }

            for (List<RequestedBufferInfo>::iterator j = i->buffers.begin();
                 j != i->buffers.end();) {
                if (j->stream == stream && j->buffer == buffer) {
                    camera3_stream_buffer_t *result_buffers =
                        new camera3_stream_buffer_t[1];

                    result_buffers->stream = stream;
                    result_buffers->buffer = buffer;
                    result_buffers->status = buffer_status;
                    result_buffers->acquire_fence = -1;
                    result_buffers->release_fence = -1;

                    result.result = NULL;
                    result.frame_number = i->frame_number;
                    result.num_output_buffers = 1;
                    result.output_buffers = result_buffers;
                    result.input_buffer = i->input_buffer;
                    result.partial_result = 0;
                    if (mMultiCameraMode == MODE_3D_VIDEO) {
                        setVideoBufferTimestamp(capture_time);
                    }
                    mCallbackOps->process_capture_result(mCallbackOps, &result);
                    HAL_LOGV("data frame_number = %d, input_buffer = %p",
                             result.frame_number, i->input_buffer);

                    delete[] result_buffers;

                    i->num_buffers--;
                    j = i->buffers.erase(j);

                    break;
                } else {
                    ++j;
                }
            }

            HAL_LOGD("num_bufs=%d, mPendingReq=%d", i->num_buffers,
                     mPendingRequest);

            if (0 == i->num_buffers) {
                Mutex::Autolock l(mRequestLock);
                receive_req_max = i->receive_req_max;
                i = mPendingRequestsList.erase(i);
                mPendingRequest--;
                break;
            } else {
                ++i;
            }
        } else if (i->frame_number > frame_number) {
            /**add for 3d capture reprocessing begin   */
            HAL_LOGV("result stream format =%d", result_info->stream->format);
            if (HAL_PIXEL_FORMAT_BLOB == result_info->stream->format ||
                (HAL_PIXEL_FORMAT_YCbCr_420_888 ==
                     result_info->stream->format &&
                 mMultiCameraMode == MODE_BLUR)) {
                HAL_LOGD("capture result, continue search");
                i++;
                continue;
            }
            /**add for 3d capture reprocessing end   */
            break;
        }
    }

    for (List<PendingRequestInfo>::iterator i = mPendingRequestsList.begin();
         i != mPendingRequestsList.end();) {
        i->pipeline_depth++;
        i++;
    }

    if (mPendingRequest != oldrequest && oldrequest >= receive_req_max) {
        HAL_LOGV("signal request=%d", oldrequest);
        mRequestSignal.signal();
    }
}

/**add for 3d capture, get/set needed zsl buffer's timestamp in zsl query
 * begin*/
uint64_t SprdCamera3HWI::getZslBufferTimestamp() {
    return mOEMIf->getZslBufferTimestamp();
}

/**add for 3d capture, get/set needed zsl buffer's timestamp in zsl query
 * end*/
void SprdCamera3HWI::setMultiCallBackYuvMode(bool mode) {
    mOEMIf->setMultiCallBackYuvMode(mode);
}

void SprdCamera3HWI::GetFocusPoint(cmr_s32 *point_x, cmr_s32 *point_y) {
    mOEMIf->GetFocusPoint(point_x, point_y);
    HAL_LOGD("x %d, y %d", *point_x, *point_y);
}

cmr_s32 SprdCamera3HWI::ispSwCheckBuf(cmr_uint *param_ptr) {
    return mOEMIf->ispSwCheckBuf(param_ptr);
}

void SprdCamera3HWI::getRawFrame(int64_t timestamp, cmr_u8 **y_addr) {
    cmr_u8 *addr_vir = NULL;

    mOEMIf->getRawFrame(timestamp, &addr_vir);

    *y_addr = addr_vir;

    HAL_LOGD("REAL TIME:y %p, ", *y_addr);

    return;
}

void SprdCamera3HWI::stopPreview() { mOEMIf->stopPreview(); }

void SprdCamera3HWI::startPreview() { mOEMIf->startPreview(); }

void SprdCamera3HWI::setVideoBufferTimestamp(uint64_t timestamp) {
    mCurFrameTimeStamp = timestamp;
}

uint64_t SprdCamera3HWI::getVideoBufferTimestamp() {
    return mCurFrameTimeStamp;
}

void SprdCamera3HWI::getMetadataVendorTagOps(vendor_tag_query_ops_t *ops) {
    ops->get_camera_vendor_section_name = mSetting->getVendorSectionName;
    ops->get_camera_vendor_tag_type = mSetting->getVendorTagType;
    ops->get_camera_vendor_tag_name = mSetting->getVendorTagName;
    ops->get_camera_vendor_tag_count = mSetting->getVendorTagCnt;
    ops->get_camera_vendor_tags = mSetting->getVendorTags;
    return;
}

void SprdCamera3HWI::dump(int /*fd */) {
    HAL_LOGD("dump E");
    HAL_LOGD("dump X");
    return;
}

int SprdCamera3HWI::flush() {
    ATRACE_CALL();

    int ret = NO_ERROR;
    int64_t timestamp;

    HAL_LOGI(":hal3: E camId=%d", mCameraId);
    if (mOEMIf) {
        mOEMIf->setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
    }

    {
        Mutex::Autolock l(&mLock);
        mFlush = true;
    }

    // for performance: dont delay for dc/dv switch or front/back switch
    mOEMIf->setSensorCloseFlag();

    if (mRegularChan) {
        mRegularChan->stop(mFrameNum);
    }
    if (mPicChan) {
        mPicChan->stop(mFrameNum);
    }

    {
        Mutex::Autolock l(&mLock);
        HAL_LOGI(":hal3: clear all buffers");
        timestamp = systemTime(SYSTEM_TIME_BOOTTIME);
        if (mRegularChan) {
            // TBD: will add a user-kernel interface, to return all inflight
            // buffers, then we need not to stop streams
            mRegularChan->channelClearAllQBuff(timestamp,
                                               CAMERA_STREAM_TYPE_PREVIEW);
            mRegularChan->channelClearAllQBuff(timestamp,
                                               CAMERA_STREAM_TYPE_VIDEO);
            mRegularChan->channelClearAllQBuff(timestamp,
                                               CAMERA_STREAM_TYPE_CALLBACK);
        }
        if (mPicChan) {
            // TBD: will add a user-kernel interface, to return all inflight
            // buffers, then we need not to stop streams
            mPicChan->channelClearAllQBuff(timestamp,
                                           CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
        }
    }

    // fix issue pendingList full, last request come after flush
    HAL_LOGD("clear buffers ...");
    usleep(1000);

    {
        Mutex::Autolock l(&mLock);
        HAL_LOGD(":hal3: re-clear all buffers");
        timestamp = systemTime(SYSTEM_TIME_BOOTTIME);
        if (mRegularChan) {
            // TBD: will add a user-kernel interface, to return all inflight
            // buffers, then we need not to stop streams
            mRegularChan->channelClearAllQBuff(timestamp,
                                               CAMERA_STREAM_TYPE_PREVIEW);
            mRegularChan->channelClearAllQBuff(timestamp,
                                               CAMERA_STREAM_TYPE_VIDEO);
            mRegularChan->channelClearAllQBuff(timestamp,
                                               CAMERA_STREAM_TYPE_CALLBACK);
        }
        if (mPicChan) {
            // TBD: will add a user-kernel interface, to return all inflight
            // buffers, then we need not to stop streams
            mPicChan->channelClearAllQBuff(timestamp,
                                           CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
        }
    }

    mOldCapIntent = SPRD_CONTROL_CAPTURE_INTENT_CONFIGURE;
    mFlush = false;
    if (mOEMIf) {
        mOEMIf->setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_2);
    }
    HAL_LOGI(":hal3: X");

    return 0;
}

void SprdCamera3HWI::captureResultCb(cam_result_data_info_t *result_info) {
    // Mutex::Autolock l(mLock);

    if (NULL == result_info) {
        HAL_LOGE("param invalid");
        return;
    }

    handleCbDataWithLock(result_info);

    return;
}

void SprdCamera3HWI::captureResultCb(cam_result_data_info_t *result_info,
                                     void *userdata) {
    SprdCamera3HWI *hw = (SprdCamera3HWI *)userdata;
    if (hw == NULL) {
        HAL_LOGE("Invalid hw %p", hw);
        return;
    }

    hw->captureResultCb(result_info);
    return;
}

int SprdCamera3HWI::initialize(const struct camera3_device *device,
                               const camera3_callback_ops_t *callback_ops) {
    ATRACE_CALL();

    HAL_LOGD("camera3_device_ops E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -ENODEV;
    }

    int ret = hw->initialize(callback_ops);
    HAL_LOGD("camera3_device_ops X");
    return ret;
}

int SprdCamera3HWI::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    ATRACE_CALL();

    HAL_LOGD("camera3_device_ops E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -ENODEV;
    }
    int ret = hw->configureStreams(stream_list);
    HAL_LOGD("camera3_device_ops X");
    return ret;
}

int SprdCamera3HWI::register_stream_buffers(
    const struct camera3_device *device,
    const camera3_stream_buffer_set_t *buffer_set) {
    HAL_LOGD("camera3_device_ops E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -ENODEV;
    }
    int ret = hw->registerStreamBuffers(buffer_set);
    HAL_LOGD("camera3_device_ops X");
    return ret;
}

const camera_metadata_t *SprdCamera3HWI::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    HAL_LOGD("camera3_device_ops E");
    camera_metadata_t *fwk_metadata = NULL;
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->constructDefaultMetadata(type);
    HAL_LOGD("camera3_device_ops X");
    return fwk_metadata;
}

int SprdCamera3HWI::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    ATRACE_CALL();

    HAL_LOGV("camera3_device_ops E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -EINVAL;
    }

    int ret = hw->processCaptureRequest(request);
    HAL_LOGV("camera3_device_ops X");
    return ret;
}

void SprdCamera3HWI::get_metadata_vendor_tag_ops(
    const struct camera3_device *device, vendor_tag_query_ops_t *ops) {
    HAL_LOGD("camera3_device_ops E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return;
    }

    hw->getMetadataVendorTagOps(ops);
    HAL_LOGD("camera3_device_ops X");
    return;
}

void SprdCamera3HWI::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return;
    }

    hw->dump(fd);
    HAL_LOGV("X");
    return;
}

int SprdCamera3HWI::flush(const struct camera3_device *device) {
    ATRACE_CALL();

    int ret;

    HAL_LOGD("camera3_device_ops E");
    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(device->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return -EINVAL;
    }

    ret = hw->flush();
    HAL_LOGD("camera3_device_ops X");
    return ret;
}

int SprdCamera3HWI::close_camera_device(struct hw_device_t *device) {
    ATRACE_CALL();

    int ret = NO_ERROR;

    SprdCamera3HWI *hw = reinterpret_cast<SprdCamera3HWI *>(
        reinterpret_cast<camera3_device_t *>(device)->priv);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return BAD_VALUE;
    }

    HAL_LOGI(":hal3: camId=%d camera3->close E", hw->mCameraId);

    ret = hw->closeCamera();
    if (ret) {
        HAL_LOGE("hw->closeCamera failed");
        goto exit;
    }

    delete hw;
    hw = NULL;
    device = NULL;

    g_cam_device = NULL;

    if (mCameraSessionActive > 0)
        mCameraSessionActive--;

    mMultiCameraMode = MODE_SINGLE_CAMERA;

    HAL_LOGI(":hal3: camera3->close X mCameraSessionActive %d",
             mCameraSessionActive);

exit:
    return ret;
}

void SprdCamera3HWI::setMultiCameraMode(multiCameraMode Mode) {
    mMultiCameraMode = Mode;
    HAL_LOGD("mMultiCameraMode=%d ", mMultiCameraMode);
}

void SprdCamera3HWI::setMasterId(uint8_t masterId) {
    mMasterId = masterId;
    HAL_LOGD("mMasterId=%d ", mMasterId);
}

bool SprdCamera3HWI::isMultiCameraMode(int Mode) {
    bool ret = false;
    if (Mode > MODE_SINGLE_CAMERA && Mode < MODE_CAMERA_MAX) {
        ret = true;
    }

    return ret;
}

void SprdCamera3HWI::setSprdCameraLowpower(int flag) {
    mSprdCameraLowpower = flag;
    mOEMIf->setSprdCameraLowpower(flag);
}

int SprdCamera3HWI::camera_ioctrl(int cmd, void *param1, void *param2) {

    int ret = 0;

    if (mOEMIf)
        ret = mOEMIf->camera_ioctrl(cmd, param1, param2);

    return ret;
}

int SprdCamera3HWI::setSensorStream(uint32_t on_off) {
    int ret = 0;

    HAL_LOGD("set on_off %d", on_off);

    ret = mOEMIf->setSensorStream(on_off);

    return ret;
}

int SprdCamera3HWI::setCameraClearQBuff() {
    int ret = 0;
    HAL_LOGD("E");

    ret = mOEMIf->setCameraClearQBuff();

    return ret;
}

void SprdCamera3HWI::getDualOtpData(void **addr, int *size, int *read) {
    void *otp_data = NULL;
    int otp_size = 0;
    int has_read_otp = 0;

    mOEMIf->getDualOtpData(&otp_data, &otp_size, &has_read_otp);

    *addr = otp_data;
    *size = otp_size;
    *read = has_read_otp;

    HAL_LOGD("OTP INFO:addr 0x%p, size = %d", *addr, *size);

    return;
}
void SprdCamera3HWI::getOnlineBuffer(void *cali_info) {

    mOEMIf->getOnlineBuffer(cali_info);

    HAL_LOGD("online buffer addr %p", cali_info);
    return;
}

void SprdCamera3HWI::getIspDebugInfo(void **addr, int *size) {
    void *ispInioAddr = NULL;
    int ispInfoSize = 0;

    mOEMIf->getIspDebugInfo(&ispInioAddr, &ispInfoSize);

    *addr = ispInioAddr;
    *size = ispInfoSize;
    HAL_LOGD("ISP INFO:addr 0x%p, size = %d", *addr, *size);

    return;
}

}; // end namespace sprdcamera
