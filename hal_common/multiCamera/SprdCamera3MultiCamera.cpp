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
#define LOG_TAG "Cam3MultiCamera"
//#define LOG_NDEBUG 0
#include "SprdCamera3MultiCamera.h"

#if (MINICAMERA != 1)
#include <ui/Fence.h>
#endif

using namespace android;

namespace sprdcamera {

SprdCamera3MultiCamera *gMultiCam = NULL;

#define MAX_DIGITAL_ZOOM_RATIO_OPTICS 8
#define SWITCH_WIDE_SW_REQUEST_RATIO (1.0)
#define SWITCH_WIDE_TELE_REQUEST_RATIO (2.0)
// Error Check Macros
#define CHECK_MUXER()                                                          \
    if (!gMultiCam) {                                                          \
        HAL_LOGE("Error getting muxer ");                                      \
        return;                                                                \
    }
/*
// Error Check Macros
#define CHECK_BASE()                                                           \
    if (!gMultiCam) {                                                         \
        HAL_LOGE("Error getting blur ");                                       \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_BASE_ERROR()                                                     \
    if (!gMultiCam) {                                                         \
        HAL_LOGE("Error getting blur ");                                       \
        return -ENODEV;                                                        \
    }
*/
#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

#define CHECK_STREAM_ERROR(stream)                                             \
    if (!stream) {                                                             \
        HAL_LOGE("Error !! stream not found!! check config file ");            \
        return -ENODEV;                                                        \
    }

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#endif

#ifdef MAX_SAVE_QUEUE_SIZE
#undef MAX_SAVE_QUEUE_SIZE
#endif
#define MAX_SAVE_QUEUE_SIZE 20

#ifdef MAX_NOTIFY_QUEUE_SIZE
#undef MAX_NOTIFY_QUEUE_SIZE
#endif
#define MAX_NOTIFY_QUEUE_SIZE 50

camera3_device_ops_t SprdCamera3MultiCamera::mCameraCaptureOps = {
    .initialize = SprdCamera3MultiCamera::initialize,
    .configure_streams = SprdCamera3MultiCamera::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3MultiCamera::construct_default_request_settings,
    .process_capture_request = SprdCamera3MultiCamera::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3MultiCamera::dump,
    .flush = SprdCamera3MultiCamera::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3MultiCamera::callback_ops_multi[] = {
    {.process_capture_result =
         SprdCamera3MultiCamera::process_capture_result_main,
     .notify = SprdCamera3MultiCamera::notifyMain},
    {.process_capture_result =
         SprdCamera3MultiCamera::process_capture_result_aux1,
     .notify = SprdCamera3MultiCamera::notify_Aux1},
    {.process_capture_result =
         SprdCamera3MultiCamera::process_capture_result_aux2,
     .notify = SprdCamera3MultiCamera::notify_Aux2},
    //{.process_capture_result =
    // SprdCamera3MultiCamera::process_capture_result_aux3,
    //.notify = SprdCamera3MultiCamera::notify_Aux3}

};

camera_metadata_t *SprdCamera3MultiCamera::mStaticCameraCharacteristics = NULL;

/*
 * HAL API: camera_module_t.get_camera_info
 *
 * TODO: currently this function reports metadata only for MultiCamera
 */
int SprdCamera3MultiCamera::get_camera_info(int id, struct camera_info *info) {
    if (info) {
        SprdCamera3Setting::getCameraInfo(0, info);
        info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
        info->conflicting_devices_length = 0;
        if (!mStaticCameraCharacteristics) {
            camera_metadata_t *metadata_0;
            HAL_LOGD("get_camera_info id 36");
            SprdCamera3Setting::getSensorStaticInfo(0);
            SprdCamera3Setting::initDefaultParameters(0);
            SprdCamera3Setting::getStaticMetadata(0, &metadata_0);

            CameraMetadata metadata = clone_camera_metadata(metadata_0);

            float max_digital_zoom = MAX_DIGITAL_ZOOM_RATIO_OPTICS;
            metadata.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
                            &max_digital_zoom, 1);

#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT
            float zoom_ratio_section[6] = {0.6, 1.0, 8.0, 0, 0, 0};
#else
            float zoom_ratio_section[6] = {0.6, 1.0, 2.0, 8.0, 0, 0};
#endif

            addAvailableStreamSize(metadata, "RES_MULTI");
            metadata.update(ANDROID_SPRD_ZOOM_RATIO_SECTION, zoom_ratio_section,
                            6);

            cmr_u16 cropW, cropH;
            int32_t active_array_size[4] = {0};
            SprdCamera3Setting::getLargestSensorSize(0, &cropW, &cropH);
            active_array_size[0] = 0;
            active_array_size[1] = 0;
            active_array_size[2] = cropW;
            active_array_size[3] = cropH;
            metadata.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                            active_array_size, ARRAY_SIZE(active_array_size));

            mStaticCameraCharacteristics = metadata.release();
        }
        info->static_camera_characteristics = mStaticCameraCharacteristics;
    }

    return 0;
}

/*===========================================================================
 * FUNCTION     : SprdCamera3MultiCamera
 *
 * DESCRIPTION : SprdCamera3MultiCamera Constructor
 *
 * PARAMETERS :
 *    @num_of_cameras : Number of Physical Cameras on device
 *
 * RETURN     :
 *==========================================================================*/
SprdCamera3MultiCamera::SprdCamera3MultiCamera() {
    m_nPhyCameras = 3;
    mIsCapturing = false;
    mIsVideoMode = false;
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    mMultiMode = MODE_MULTI_CAMERA;
    mLocalBufferNumber = 0;
    mCapFrameNum = -1;
    mCurFrameNum = 0;
    mRequstState = PREVIEW_REQUEST_STATE;
    mFlushing = false;
    mReqConfigNum = 0;
    mFirstFrameCount = 0;
    mMetaNotifyIndex = -1;
    mWaitFrameNum = -1;
    mSendFrameNum = -1;
    mSavedCapReqsettings = NULL;
    mIsSyncFirstFrame = false;
    mMetadataList.clear();
    mLocalBufferList.clear();
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    mNotifyListMain.clear();
    mNotifyListAux1.clear();
    mNotifyListAux2.clear();
    mNotifyListAux3.clear();
    mNotifyListVideoMain.clear();
    mNotifyListVideoAux1.clear();
    mNotifyListVideoAux2.clear();
    mNotifyListVideoAux3.clear();
    bzero(&m_pPhyCamera,
          sizeof(sprdcamera_physical_descriptor_t) * MAX_MULTI_NUM_CAMERA);
    bzero(&mHalReqConfigStreamInfo,
          sizeof(hal_req_stream_config_total) * MAX_MULTI_NUM_STREAMS);
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    bzero(&mBufferInfo, sizeof(hal_buffer_info) * MAX_MULTI_NUM_BUFFER);
    bzero(&mLocalBuffer,
          sizeof(new_mem_t) * MAX_MULTI_NUM_BUFFER * MAX_MULTI_NUM_BUFFER);
    bzero(&mMainSize, sizeof(stream_size_t));

    mCapInputbuffer = NULL;
    mTeleMaxWidth = 0;
    mTeleMaxHeight = 0;
    mWideMaxWidth = 0;
    mWideMaxHeight = 0;
    mSwMaxWidth = 0;
    mSwMaxHeight = 0;
    mVcmSteps = 0;
    mAlgoStatus = 0;
    mPrevAlgoHandle = NULL;
    mCapAlgoHandle = NULL;
    mPreviewMuxerThread = new TWPreviewMuxerThread();
    mTWCaptureThread = new TWCaptureThread();
    mVideoThread = new VideoThread();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux1.clear();
    mUnmatchedFrameListAux2.clear();
    mUnmatchedFrameListVideoMain.clear();
    mUnmatchedFrameListVideoAux1.clear();
    mUnmatchedFrameListVideoAux2.clear();
    mRefIdex = 0;
    mRefCameraId = 0;
    mLastRefCameraId = 0xff;
    mCameraIdWide = 0;
    mCameraIdSw = 0;
    mCameraIdTele = 0;
    aux1_af_state= 0;
    aux2_af_state= 0;
    aux1_face_number = 0;
    aux2_face_number = 0;
    aux1_ai_scene_type = 0;
    aux2_ai_scene_type = 0;
    memset(&mOtpData, 0, sizeof(OtpData));
    memset(main_FaceRect, 0, sizeof(int32_t) * 10 * 4);
    memset(main_FaceRect, 0, sizeof(int32_t) * 10 * 4);
    memset(aux1_FaceRect, 0, sizeof(int32_t) * 10 * 4);
    memset(aux2_FaceRect, 0, sizeof(int32_t) * 10 * 4);
    memset(aux1_FaceGenderRaceAge, 0, sizeof(int32_t) * 10);
    memset(aux2_FaceGenderRaceAge, 0, sizeof(int32_t) * 10);
    memset(MultiConfigId, 0, sizeof(uint32_t) * 5);

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : ~SprdCamera3MultiCamera
 *
 * DESCRIPTION : SprdCamera3MultiCamera Desctructor
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
SprdCamera3MultiCamera::~SprdCamera3MultiCamera() {
    HAL_LOGI("E");
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : getCameraMultiCamera
 *
 * DESCRIPTION : Creates Camera Muxer if not created
 *
 * PARAMETERS :
 *    @pMuxer : Pointer to retrieve Camera Muxer
 *
 * RETURN     : NONE
 *==========================================================================*/
/*
void SprdCamera3MultiCamera::getCameraMultiCamera(SprdCamera3MultiCamera
**pMuxer) {
    *pMuxer = NULL;
    if (!gMultiCam) {
        gMultiCam = new SprdCamera3MultiCamera();
    }
    CHECK_MUXER();
    *pMuxer = gMultiCam;
    HAL_LOGD("gMultiCam: %p ", gMultiCam);

    return;
}
*/

static config_multi_camera multi_camera_config = {
#include "multi_camera_config_.h"
};

static config_multi_camera wide_ultra_config = {
#include "wide_ultrawide_config.h"
};

/*===========================================================================
 * FUNCTION     : camera_device_open
 *
 * DESCRIPTION : static function to open a camera device by its ID
 *
 * PARAMETERS :
 *    @modue: hw module
 *    @id : camera ID
 *    @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     :
 *                NO_ERROR  : success
 *                BAD_VALUE : Invalid Camera ID
 *                other: non-zero failure code
 *==========================================================================*/
int SprdCamera3MultiCamera::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint8_t phyId = 0;
    unsigned int on_off = 1;

    HAL_LOGD("E id = %d", atoi(id));
    if (!gMultiCam) {
        gMultiCam = new SprdCamera3MultiCamera();
    }

    config_multi_camera *config_info = gMultiCam->load_config_file();
    if (!config_info) {
        HAL_LOGE("failed to get common config file ");
        return BAD_VALUE;
    }
    gMultiCam->parse_configure_info(config_info);

    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    hw_device_t *hw_dev[gMultiCam->m_nPhyCameras];
    // Open all physical cameras
    for (uint32_t i = 0; i < gMultiCam->m_nPhyCameras; i++) {
        phyId = gMultiCam->m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((int)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }

        hw_dev[i] = NULL;
        hw->setMultiCameraMode(gMultiCam->mMultiMode);
        hw->setMasterId(gMultiCam->m_VirtualCamera.id);
        rc = hw->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hw;
            gMultiCam->closeCameraDevice();
            return rc;
        }

        if (phyId == findSensorRole(MODULE_SPW_NONE_BACK)) {
            hw->camera_ioctrl(CAMERA_IOCTRL_ULTRA_WIDE_MODE, &on_off, NULL);
        }

        HAL_LOGD("open id=%d success", phyId);
        gMultiCam->m_pPhyCamera[i].dev =
            reinterpret_cast<camera3_device_t *>(hw_dev[i]);
        gMultiCam->m_pPhyCamera[i].hwi = hw;
    }

    gMultiCam->m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    gMultiCam->m_VirtualCamera.dev.common.version =
        CAMERA_DEVICE_API_VERSION_3_2;
    gMultiCam->m_VirtualCamera.dev.common.close = close_camera_device;
    gMultiCam->m_VirtualCamera.dev.ops = &mCameraCaptureOps;
    gMultiCam->m_VirtualCamera.dev.priv = gMultiCam;
    *hw_device = &(gMultiCam->m_VirtualCamera.dev.common);

    // reConfigOpen();
    HAL_LOGI("id= %d, rc: %d", atoi(id), rc);
    return rc;
}

/*===========================================================================
 * FUNCTION     : close_camera_device
 *
 * DESCRIPTION : Close the camera
 *
 * PARAMETERS :
 *    @hw_dev : camera hardware device info
 *
 * RETURN     :
 *                NO_ERROR  : success
 *                other: non-zero failure code
 *==========================================================================*/
int SprdCamera3MultiCamera::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }
    return gMultiCam->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION     : close  amera device
 *
 * DESCRIPTION : Close the camera
 *
 * PARAMETERS :
 *    @hw_dev : camera hardware device info
 *
 * RETURN     :
 *                NO_ERROR : success
 *                other : non-zero failure code
 *==========================================================================*/
int SprdCamera3MultiCamera::closeCameraDevice() {

    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    HAL_LOGI("E");

    reConfigClose();
    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = m_nPhyCameras; i > 0; i--) {
        sprdCam = &m_pPhyCamera[i - 1];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL)
            continue;

        HAL_LOGI("camera id:%d", sprdCam->id);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", sprdCam->id);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }
    // clear all list
    freeLocalBuffer();
    mIsSyncFirstFrame = false;
    mIsVideoMode = false;
    mFirstFrameCount = 0;
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    mMetadataList.clear();
    mNotifyListMain.clear();
    mNotifyListAux1.clear();
    mNotifyListAux2.clear();
    mNotifyListAux3.clear();
    mNotifyListVideoMain.clear();
    mNotifyListVideoAux1.clear();
    mNotifyListVideoAux2.clear();
    mNotifyListVideoAux3.clear();
    HAL_LOGI("X, rc: %d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION     : initialize
 *
 * DESCRIPTION : initialize camera device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    // CHECK_BASE_ERROR();
    rc = gMultiCam->initialize(callback_ops);
    HAL_LOGV("X");
    return rc;
}

/*===========================================================================
 * FUNCTION     : initialize
 *
 * DESCRIPTION :
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = 0;
    mCallbackOps = callback_ops;
    SprdCamera3HWI *hwi = NULL;
    SprdCamera3MultiBase::initialize(mMultiMode, m_pPhyCamera[0].hwi);

    for (uint32_t i = 0; i < m_nPhyCameras; i++) {

        sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[i];
        SprdCamera3HWI *hwi = sprdCam.hwi;
        CHECK_HWI_ERROR(hwi);

        rc = hwi->initialize(sprdCam.dev, &callback_ops_multi[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error  camera while initialize !! ");
            return rc;
        }
    }

    mLocalBufferNumber = 0;
    mCapFrameNum = -1;
    mCurFrameNum = -1;
    aux1_face_number = 0;
    aux2_face_number = 0;
    aux1_ai_scene_type = 0;
    aux2_ai_scene_type = 0;
    mRequstState = PREVIEW_REQUEST_STATE;
    mSwitch_W_Sw_Threshold = SWITCH_WIDE_SW_REQUEST_RATIO;
    mSwitch_W_T_Threshold = SWITCH_WIDE_TELE_REQUEST_RATIO;
    mFlushing = false;
    mReqConfigNum = 0;
    mMetaNotifyIndex = -1;
    mWaitFrameNum = -1;
    mSendFrameNum = -1;
    mFirstFrameCount = 0;
    mSavedCapReqsettings = NULL;
    mIsSyncFirstFrame = false;
    mMetadataList.clear();
    mLocalBufferList.clear();
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    memset(MultiConfigId, 0, sizeof(uint32_t) * 5);
    memset(aux1_FaceRect, 0, sizeof(int32_t) * 10 * 4);
    memset(aux2_FaceRect, 0, sizeof(int32_t) * 10 * 4);
    memset(aux1_FaceGenderRaceAge, 0, sizeof(int32_t) * 10);
    memset(aux2_FaceGenderRaceAge, 0, sizeof(int32_t) * 10);
    bzero(&mLocalBuffer,
          sizeof(new_mem_t) * MAX_MULTI_NUM_BUFFER * MAX_MULTI_NUM_BUFFER);
    reConfigInit();

    return rc;
}

/*===========================================================================
 * FUNCTION     : construct_default_request_settings
 *
 * DESCRIPTION : construc default request settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t *
SprdCamera3MultiCamera::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    rc = gMultiCam->constructDefaultRequestSettings(device, type);
    HAL_LOGV("X");
    return rc;
}

/*===========================================================================
 * FUNCTION     : constructDefaultRequestSettings
 *
 * DESCRIPTION : construct Default Request Settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t *
SprdCamera3MultiCamera::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *fwk_metadata = NULL;

    const camera_metadata_t *metadata = NULL;
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[i];
        SprdCamera3HWI *hwi = sprdCam.hwi;
        if (m_VirtualCamera.id == sprdCam.id) {
            fwk_metadata =
                hwi->construct_default_request_settings(sprdCam.dev, type);
            if (!fwk_metadata) {
                HAL_LOGE("Error  camera while initialize !! ");
                return NULL;
            }
        } else {
            metadata =
                hwi->construct_default_request_settings(sprdCam.dev, type);
            if (!metadata) {
                HAL_LOGE("Error  camera while initialize !! ");
                return NULL;
            }
        }
        HAL_LOGI("X,fwk_metadata=%p", fwk_metadata);
    }

    return fwk_metadata;
}

/*===========================================================================
 * FUNCTION     : createHalStream
 *
 * DESCRIPTION : create stream
 *
 * PARAMETERS : Logical Main Camera Info
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::createHalStream(
    camera3_stream_t *preview_stream, camera3_stream_t *callback_stream,
    camera3_stream_t *snap_stream, camera3_stream_t *video_stream,
    camera3_stream_t *output_list,
    sprdcamera_physical_descriptor_t *camera_phy_info) {
    if (output_list == NULL || camera_phy_info == NULL) {
        HAL_LOGE("param is null");
        return UNKNOWN_ERROR;
    }

    for (int i = 0; i < camera_phy_info->stream_num; i++) {
        camera3_stream_t *output = (camera3_stream_t *)&output_list[i];
        hal_stream_info *hal_stream =
            (hal_stream_info *)&camera_phy_info->hal_stream[i];
        switch (hal_stream->type) {
        case PREVIEW_STREAM:
            if (preview_stream) {
                memcpy(output, preview_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            }
            break;
        case SNAPSHOT_STREAM:
            if (snap_stream) {
                memcpy(output, snap_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_BLOB;
            }

            break;
        case CALLBACK_STREAM:
            if (callback_stream) {
                memcpy(output, callback_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_YCbCr_420_888;
                output->usage = GRALLOC_USAGE_SW_READ_OFTEN;
            }

            break;
        case VIDEO_STREAM:
            if (video_stream) {
                memcpy(output, video_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
                output->usage = GRALLOC_USAGE_HW_VIDEO_ENCODER;
            }

            break;
        case DEFAULT_STREAM:
            break;

        default:
            HAL_LOGI("unknown type %d", hal_stream->type);
            break;
            output->max_buffers = 1;
        }
        if (hal_stream->width != 0 || hal_stream->height != 0) {
            output->width = hal_stream->width;
            output->height = hal_stream->height;
        }
        if (hal_stream->format) {
            output->format = hal_stream->format;
        }
        output->stream_type = CAMERA3_STREAM_OUTPUT;
        output->rotation = 0;

        HAL_LOGD("hal_stream %p, hal_stream width %d, height %d,",
            "hal_stream stream type is %d, format % d,",
            "output width %d, height %d, format %d",
            hal_stream, hal_stream->width, hal_stream->height,
            hal_stream->type, hal_stream->format,
            output->width, output->height, output->format);
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION     : configureStreams
 *
 * DESCRIPTION : configure stream
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");
    int rc = 0;
    int total_stream = 0;
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *snapStream = NULL;
    camera3_stream_t *callbackStream = NULL;
    camera3_stream_t *videoStream = NULL;
    camera3_stream_t *defaultStream = NULL;
    camera3_stream_t *configstreamList[MAX_MULTI_NUM_STREAMS];
    camera3_stream_t *newStream = NULL;
    mFlushing = false;
    mMetadataList.clear();
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    mNotifyListMain.clear();
    mNotifyListAux1.clear();
    mNotifyListAux2.clear();
    mNotifyListAux3.clear();
    mNotifyListVideoMain.clear();
    mNotifyListVideoAux1.clear();
    mNotifyListVideoAux2.clear();
    mNotifyListVideoAux3.clear();
    mMetaNotifyIndex = -1;
    mCapFrameNum = -1;
    mWaitFrameNum = -1;
    mSendFrameNum = -1;
    bzero(&mMainSize, sizeof(stream_size_t));

    memset(configstreamList, 0,
           sizeof(camera3_stream_t *) * MAX_MULTI_NUM_STREAMS);

    // first.get stream from fw.
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        camera3_stream_t *newStream = stream_list->streams[i];

        if (newStream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
            config_multi_camera *config_info = gMultiCam->load_config_file();
            if (!config_info) {
                HAL_LOGE("failed to get video config file ");
                return BAD_VALUE;
            }
            gMultiCam->video_parse_configure_info(config_info);
            mIsVideoMode = true;
        } else {
            config_multi_camera *config_info = gMultiCam->load_config_file();
            if (!config_info) {
                HAL_LOGE("failed to get common config file ");
                return BAD_VALUE;
            }
            gMultiCam->parse_configure_info(config_info);
        }
        HAL_LOGV("mIsVideoMode=%d", mIsVideoMode);

        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGI("stream num %d, requestStreamType %d",
            stream_list->num_streams, requestStreamType);
        if (requestStreamType == PREVIEW_STREAM) {
            previewStream = stream_list->streams[i];
            previewStream->max_buffers = 4;
            mMainSize.preview_w = previewStream->width;
            mMainSize.preview_h = previewStream->height;
        } else if (requestStreamType == CALLBACK_STREAM) {
            callbackStream = stream_list->streams[i];
            callbackStream->max_buffers = 4;
            mMainSize.callback_w = callbackStream->width;
            mMainSize.callback_h = callbackStream->height;
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            snapStream = stream_list->streams[i];
            snapStream->max_buffers = 1;
            mMainSize.capture_w = snapStream->width;
            mMainSize.capture_h = snapStream->height;
        } else if (requestStreamType == VIDEO_STREAM) {
            videoStream = stream_list->streams[i];
            videoStream->max_buffers = 4;
            mMainSize.video_w = videoStream->width;
            mMainSize.video_h = videoStream->height;
        } else if (requestStreamType == DEFAULT_STREAM) {
            defaultStream = stream_list->streams[i];
            defaultStream->max_buffers = 4;
        }
    }
    // second.config streamsList.
    for (size_t i = 0; i < m_nPhyCameras; i++) {
        sprdcamera_physical_descriptor_t *camera_phy_info =
            (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[i];
        camera3_stream_t *config_stream =
            (camera3_stream_t *)&(m_pPhyCamera[i].streams);

        createHalStream(previewStream, callbackStream, snapStream, videoStream,
                        config_stream, camera_phy_info);

        for (int j = 0; j < m_pPhyCamera[i].stream_num; j++) {
            int follow_type = camera_phy_info->hal_stream[j].follow_type;
            if (follow_type != 0) {
                int camera_index =
                    camera_phy_info->hal_stream[j].follow_camera_index;
                HAL_LOGD("camera=%d,follw_type=%d,index=%d",
                    i, follow_type, camera_index);
                camera3_stream_t *find_stream =
                    findStream(follow_type, (sprdcamera_physical_descriptor_t
                                                 *)&m_pPhyCamera[camera_index]);
                CHECK_STREAM_ERROR(find_stream);
                m_pPhyCamera[i].streams[j].width = find_stream->width;
                m_pPhyCamera[i].streams[j].height = find_stream->height;
            }
            configstreamList[j] = &(m_pPhyCamera[i].streams[j]);
        }

        camera3_stream_configuration config;
        config = *stream_list;
        config.num_streams = m_pPhyCamera[i].stream_num;
        config.streams = configstreamList;
        rc = (m_pPhyCamera[i].hwi)
                 ->configure_streams(m_pPhyCamera[i].dev, &config);
        if (rc < 0) {
            HAL_LOGE("failed. configure %d streams!!", i);
            return rc;
        }
    }
    sprdcamera_physical_descriptor_t *main_camera =
        (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[0];

    for (int i = 0; i < main_camera->stream_num; i++) {
        newStream = (camera3_stream_t *)&main_camera->streams[i];
        int stream_type = main_camera->hal_stream[i].type;
        if (stream_type == PREVIEW_STREAM && (previewStream != NULL)) {
            memcpy(previewStream, newStream, sizeof(camera3_stream_t));
            previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
            HAL_LOGV("previewStream  max buffers %d",
                previewStream->max_buffers);
        } else if (stream_type == SNAPSHOT_STREAM && (snapStream != NULL)) {
            memcpy(snapStream, newStream, sizeof(camera3_stream_t));
        } else if (stream_type == CALLBACK_STREAM && (callbackStream != NULL)) {
            memcpy(callbackStream, newStream, sizeof(camera3_stream_t));
            callbackStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        } else if (stream_type == VIDEO_STREAM && (videoStream != NULL)) {
            memcpy(videoStream, newStream, sizeof(camera3_stream_t));
            videoStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        } else if (stream_type == DEFAULT_STREAM && (defaultStream != NULL)) {
            memcpy(defaultStream, newStream, sizeof(camera3_stream_t));
            defaultStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        }
    }

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGI("main configurestreams, streamtype:%d, format:%d,"
            "width:%d, height:%d, %p, requestStreamType %d",
            stream_list->streams[i]->stream_type,
            stream_list->streams[i]->format,
            stream_list->streams[i]->width,
            stream_list->streams[i]->height,
            stream_list->streams[i]->priv, requestStreamType);
    }
    rc = allocateBuff();
    if (rc == -1) {
        HAL_LOGE("allocateBuff failed. rc%d.", rc);
    }
    reConfigStream();

    HAL_LOGI("x rc%d.", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION     : configure_streams
 *
 * DESCRIPTION : configure streams
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV(" E");
    // CHECK_BASE_ERROR();
    rc = gMultiCam->configureStreams(device, stream_list);
    HAL_LOGV(" X");
    return rc;
}

/*===========================================================================
 * FUNCTION     : process_capture_request
 *
 * DESCRIPTION : process capture request
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV(" E idx:%d", request->frame_number);
    // CHECK_BASE_ERROR();
    rc = gMultiCam->processCaptureRequest(device, request);
    HAL_LOGV(" X");
    return rc;
}

/*===========================================================================
 * FUNCTION     : saveRequest
 *
 * DESCRIPTION : save Request
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::saveRequest(
    camera3_capture_request_t *request,
    camera3_stream_buffer_t fw_buffer[MAX_MULTI_NUM_STREAMS + 1], int index) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    multi_request_saved_t currRequest;
    bzero(&currRequest, sizeof(currRequest));
    Mutex::Autolock l(mRequestLock);
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        newStream->reserved[0] = NULL; // for eis crash
        currRequest.buffer = request->output_buffers[i].buffer;
        currRequest.input_buffer = request->input_buffer;
        currRequest.frame_number = request->frame_number;
        currRequest.metaNotifyIndex = index;
        if (getStreamType(newStream) == CALLBACK_STREAM) { // preview
            currRequest.metaNotifyIndex = 0;
            currRequest.preview_stream = request->output_buffers[i].stream;
            HAL_LOGV("save prev request:id %d,to list ", request->frame_number);
            mSavedPrevRequestList.push_back(currRequest);
            memcpy(&fw_buffer[PREVIEW_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
        } else if (getStreamType(newStream) == DEFAULT_STREAM) { // callback
            currRequest.callback_stream = request->output_buffers[i].stream;
            HAL_LOGV("save callck request:id %d,to list ",
                request->frame_number);
            currRequest.metaNotifyIndex = 0;
            mSavedCallbackRequestList.push_back(currRequest);
            memcpy(&fw_buffer[CALLBACK_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
        } else if (getStreamType(newStream) == SNAPSHOT_STREAM) { // snapshot
            if (mIsVideoMode == true) {
                currRequest.metaNotifyIndex = 0;
            }
            mCapFrameNum = request->frame_number;
            currRequest.snap_stream = request->output_buffers[i].stream;
            HAL_LOGV("save snapshot request:id %d,to list ", mCapFrameNum);
            memcpy(&fw_buffer[SNAPSHOT_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
            memcpy(&mSavedSnapRequest, &currRequest,
                   sizeof(multi_request_saved_t));
            // mSavedCapReqsettings = clone_camera_metadata(request->settings);
        } else if (getStreamType(newStream) == VIDEO_STREAM) { // video
            currRequest.metaNotifyIndex = 0;
            currRequest.video_stream = request->output_buffers[i].stream;
            HAL_LOGV("save video request:id %d,to list ",
                request->frame_number);
            mSavedVideoRequestList.push_back(currRequest);
            memcpy(&fw_buffer[VIDEO_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
        }
    }
}

/*===========================================================================
 * FUNCTION     : createOutputBufferStream
 *
 * DESCRIPTION : createOutputBufferStream
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::createOutputBufferStream(
    camera3_stream_buffer_t *outputBuffer, int req_stream_mask,
    const sprdcamera_physical_descriptor_t *camera_phy_info,
    camera3_stream_buffer_t fw_buffer[MAX_MULTI_NUM_STREAMS + 1],
    int *buffer_num) {
    int i = 0;
    int ret = 0;
    int stream_type = 0;
    int stream_num = 0;
    int cur_stream_mask = req_stream_mask;
    camera3_stream_buffer_t *curOutputBuffer = outputBuffer;

    while (cur_stream_mask) {
        stream_type = (req_stream_mask >> i++) & 0x1;
        cur_stream_mask >>= 1;
        if (!stream_type) {
            continue;
        }
        stream_num++;
        stream_type = 0x1 << (i - 1);
        curOutputBuffer->release_fence = -1;
        curOutputBuffer->acquire_fence = -1;
        curOutputBuffer->status = CAMERA3_BUFFER_STATUS_OK;
        HAL_LOGV("BufferStreamType %d", stream_type);
        switch (stream_type) {
        case DEFAULT_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(DEFAULT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            break;
        case DEFAULT_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(DEFAULT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[DEFAULT_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[DEFAULT_STREAM].acquire_fence;
            break;
        case PREVIEW_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(PREVIEW_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            break;
        case PREVIEW_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(PREVIEW_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[PREVIEW_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[PREVIEW_STREAM].acquire_fence;
            break;
        case VIDEO_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(VIDEO_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            break;
        case VIDEO_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(VIDEO_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[VIDEO_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[VIDEO_STREAM].acquire_fence;
            break;
        case CALLBACK_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(CALLBACK_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            break;
        case CALLBACK_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(CALLBACK_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[CALLBACK_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[CALLBACK_STREAM].acquire_fence;
            break;
        case SNAPSHOT_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(SNAPSHOT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            HAL_LOGD("start take_picture");
            break;
        case SNAPSHOT_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(SNAPSHOT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[SNAPSHOT_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[SNAPSHOT_STREAM].acquire_fence;
            HAL_LOGD("start take_picture");
            break;

        default:
            HAL_LOGE("don't kown stream,error");
        }
        if (!curOutputBuffer->buffer) {
            HAL_LOGE("buffer get failed.");
            return -1;
        }
        curOutputBuffer++;
    }
    *buffer_num = stream_num;
    HAL_LOGV("buffer_num %d", *buffer_num);
    return ret;
}

/*===========================================================================
 * FUNCTION     : reprocessReq
 *
 * DESCRIPTION : reprocessReq
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::reprocessReq(buffer_handle_t *input_buffers) {
    int ret = NO_ERROR;
    int capture_w = 0;
    int capture_h = 0;
    int64_t align = 0;
    camera3_stream_t *snap_stream = NULL;
#ifdef BOKEH_YUV_DATA_TRANSFORM
    int transform_w = 0;
    int transform_h = 0;
    buffer_handle_t *transform_buffer = NULL;
    buffer_handle_t *output_handle = NULL;
    void *transform_buffer_addr = NULL;
    void *output_handle_addr = NULL;
#endif

    if (!input_buffers) {
        HAL_LOGE("reprocess para is null");
        return UNKNOWN_ERROR;
    }

    camera3_capture_request_t request;
    camera3_stream_buffer_t output_buffer;
    camera3_stream_buffer_t input_buffer;

    memset(&output_buffer, 0x00, sizeof(camera3_stream_buffer_t));
    memset(&input_buffer, 0x00, sizeof(camera3_stream_buffer_t));

    if (mSavedSnapRequest.buffer != NULL) {
        output_buffer.buffer = mSavedSnapRequest.buffer;
    } else {
        HAL_LOGE("reprocess buffer is null");
        ret = UNKNOWN_ERROR;
        goto exit;
    }
    HAL_LOGD("find snap_stream mRefIdex %d", mRefIdex);
    snap_stream =
        findStream(SNAPSHOT_STREAM,
                   (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[mRefIdex]);
    capture_w = snap_stream->width;
    capture_h = snap_stream->height;
    HAL_LOGD("capture frame num %lld, size w=%d x h=%d",
        mCapFrameNum, capture_w, capture_h);

    input_buffer.stream = snap_stream;
    input_buffer.status = CAMERA3_BUFFER_STATUS_OK;
    input_buffer.acquire_fence = -1;
    input_buffer.release_fence = -1;

    output_buffer.stream = snap_stream;
    output_buffer.status = CAMERA3_BUFFER_STATUS_OK;
    output_buffer.acquire_fence = -1;
    output_buffer.release_fence = -1;

#ifdef BOKEH_YUV_DATA_TRANSFORM
    // workaround jpeg cant handle 16-noalign issue, when jpeg fix
    // this
    // issue, we will remove these code
    if (capture_h == 1944 && capture_w == 2592) {
        transform_w = 2592;
        transform_h = 1952;
    } else if (capture_h == 1836 && capture_w == 3264) {
        transform_w = 3264;
        transform_h = 1840;
    } else if (capture_h == 360 && capture_w == 640) {
        transform_w = 640;
        transform_h = 368;
    } else if (capture_h == 1080 && capture_w == 1920) {
        transform_w = 1920;
        transform_h = 1088;
    } else {
        transform_w = 0;
        transform_h = 0;
    }

    if (transform_w != 0) {
        transform_buffer =
            popBufferList(mLocalBufferList, transform_w, transform_h);
        output_handle = input_buffers;
        map(output_handle, &output_handle_addr);
        map(transform_buffer, &transform_buffer_addr);
        align = systemTime();
        alignTransform(output_handle_addr, capture_w, capture_h, transform_w,
                       transform_h, transform_buffer_addr);
        HAL_LOGD("alignTransform run cost %lld ms",
            ns2ms(systemTime() - align));

        input_buffer.stream->width = transform_w;
        input_buffer.stream->height = transform_h;
        input_buffer.buffer = transform_buffer;
        output_buffer.stream->width = transform_w;
        output_buffer.stream->height = transform_h;
        unmap(output_handle);
        unmap(transform_buffer);
    } else {
        input_buffer.stream->width = capture_w;
        input_buffer.stream->height = capture_h;
        input_buffer.buffer = input_buffers;
        output_buffer.stream->width = capture_w;
        output_buffer.stream->height = capture_h;
    }
#else
    input_buffer.stream->width = capture_w;
    input_buffer.stream->height = capture_h;
    input_buffer.buffer = input_buffers;
    output_buffer.stream->width = capture_w;
    output_buffer.stream->height = capture_h;
#endif

    HAL_LOGD("mSavedCapReqsettings %p", mSavedCapReqsettings);

    if (mSavedCapReqsettings != NULL) {
        request.settings = mSavedCapReqsettings;
    } else {
        HAL_LOGE("reprocess metadata is null,failed.");
    }
    request.num_output_buffers = 1;
    request.frame_number = mCapFrameNum;
    request.output_buffers = &output_buffer;
    request.input_buffer = &input_buffer;

    if (mFlushing) {
        CallBackResult(mCapFrameNum, CAMERA3_BUFFER_STATUS_ERROR,
                       SNAPSHOT_STREAM, 0);
        goto exit;
    }

    mRequstState = REPROCESS_STATE;
    HAL_LOGD("reprocessReq request.frame_number %d",
        request.frame_number);

    if (0 > m_pPhyCamera[mRefIdex].hwi->process_capture_request(
                m_pPhyCamera[mRefIdex].dev, &request)) {
        HAL_LOGE("failed to  reprocess capture request!");
        ret = UNKNOWN_ERROR;
        goto exit;
    }

#ifdef BOKEH_YUV_DATA_TRANSFORM
    pushBufferList(mLocalBuffer, transform_buffer, mLocalBufferNumber,
                   mLocalBufferList);
#endif

exit:
    if (NULL != mSavedCapReqsettings) {
        free_camera_metadata(mSavedCapReqsettings);
        mSavedCapReqsettings = NULL;
    }

    return ret;
}

/*===========================================================================
 * FUNCTION     : sendWaitFrameSingle
 *
 * DESCRIPTION : send single
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::sendWaitFrameSingle() {
    Mutex::Autolock l(mWaitFrameLock);
    mFirstFrameCount++;
    if (mFirstFrameCount == m_nPhyCameras) {
        mWaitFrameSignal.signal();
        HAL_LOGD("wake up first sync");
    }
}

/*===========================================================================
 * FUNCTION     : processCaptureRequest
 *
 * DESCRIPTION : processCaptureRequest
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {

    int rc = 0;
    int cameraMNIndex = 0;
    Mutex::Autolock l(mMultiLock);
    rc = validateCaptureRequest(request);
    if (rc != NO_ERROR) {
        return rc;
    }

    mCurFrameNum = request->frame_number;
    int total_stream_mask = 0;
    int req_stream_mak[MAX_MULTI_NUM_CAMERA];
    CameraMetadata
        metaSettings[MAX_MULTI_NUM_CAMERA];
    camera3_stream_buffer_t
        fw_buffer[MAX_MULTI_NUM_STREAMS + 1];
    camera3_stream_buffer_t
        out_buffer[MAX_MULTI_NUM_CAMERA][MAX_MULTI_NUM_STREAMS];
    camera3_capture_request_t
        newReq[MAX_MULTI_NUM_CAMERA];

    bzero(&req_stream_mak, sizeof(int) * MAX_MULTI_NUM_CAMERA);
    bzero(&fw_buffer,
        sizeof(camera3_stream_buffer_t) * (MAX_MULTI_NUM_STREAMS + 1));
    bzero(&out_buffer,
        sizeof(camera3_stream_buffer_t) * MAX_MULTI_NUM_CAMERA *
        MAX_MULTI_NUM_STREAMS);
    bzero(&newReq,
        sizeof(camera3_capture_request_t) * MAX_MULTI_NUM_CAMERA);

    // 1. config metadata for all camera
    for (int i = 0; i < MAX_MULTI_NUM_CAMERA; i++) {
        metaSettings[i] = request->settings;
    }
    reReqConfig(request, (CameraMetadata *)&metaSettings);

    // 2. get configure info from mHalReqStreamInfo;
    // map
    HAL_LOGD("num_output_buffers %d frame_number %d",
        request->num_output_buffers, request->frame_number);
    for (uint32_t i = 0; i < request->num_output_buffers; i++) {
        int stream_type = 0;
        hal_req_stream_config *req_stream_config = NULL;
        camera3_stream_t *newStream = (request->output_buffers[i]).stream;
        stream_type = getStreamType(newStream);
        // get config stream info
        if (stream_type == CALLBACK_STREAM)
            stream_type = PREVIEW_STREAM;
        else if (stream_type == DEFAULT_STREAM)
            stream_type = CALLBACK_STREAM;

        req_stream_config =
            findHalReq(stream_type, &mHalReqConfigStreamInfo[mReqConfigNum]);
        if (!req_stream_config) {
            continue;
        }
        cameraMNIndex = req_stream_config->mn_index;
        // get stream_mask for everyone
        for (int j = 0; j < req_stream_config->total_camera; j++) {
            int camera_index = req_stream_config->camera_index[j];
            HAL_LOGV("req_stream_mak=%d",
                req_stream_mak[camera_index]);
            req_stream_mak[camera_index] |=
                req_stream_config->stream_type_mask[j];
            total_stream_mask |=
                req_stream_config->stream_type_mask[j];
            HAL_LOGV("camera=%d, camera_index=%d"
                ", req_stream_config->stream_type_mask=%d"
                ", req_stream_mak=%d, total_stream_mask=%d",
                j, camera_index, req_stream_config->stream_type_mask[j],
                req_stream_mak[camera_index], total_stream_mask);
        }
        if (stream_type == SNAPSHOT_STREAM &&
            !(total_stream_mask &
              (SNAPSHOT_STREAM_FW_BUFFER | SNAPSHOT_STREAM_HAL_BUFFER))) {
            mRequstState = WAIT_FIRST_YUV_STATE;
            HAL_LOGD("jpeg ->callback,id=%lld", mCurFrameNum);
        } else if (stream_type == SNAPSHOT_STREAM) {
            HAL_LOGD("jpeg ->start,id=%lld", mCurFrameNum);
            mRequstState = WAIT_JPEG_STATE;
        }
    }

    {
#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT
        if (mZoomValue < 1.0) {
            mRefIdex = 1;
        } else {
            mRefIdex = 0;
        }

#else
        if (mZoomValue < 1.0) {
            mRefIdex = 1;
        } else if (mZoomValue >= 2.0) {
            mRefIdex = 2;
        } else {
            mRefIdex = 0;
        }

#endif
    }

    // 3.  save request;
    saveRequest(request, fw_buffer, mRefIdex);
    HAL_LOGV("frame id=%lld, capture id=%lld, sendF=%lld"
        ", cameraMNIndex=%d, mMetaNotifyIndex=%d",
        mCurFrameNum, mCapFrameNum, mSendFrameNum,
        cameraMNIndex, mMetaNotifyIndex);

    // 5. wait untill switch finish
    if (cameraMNIndex != mMetaNotifyIndex && mMetaNotifyIndex != -1 &&
        mSendFrameNum < request->frame_number - 1 &&
        request->frame_number > 0) {
        mWaitFrameNum = request->frame_number - 1;
        HAL_LOGV("change cameraMNIndex %d to %d,mWaitFrameNum=%lld",
            mMetaNotifyIndex, cameraMNIndex, mWaitFrameNum);
        Mutex::Autolock l(mWaitFrameLock);
        mWaitFrameSignal.waitRelative(mWaitFrameLock, WAIT_FRAME_TIMEOUT);
        HAL_LOGV("wait succeed.");
    } else if (request->frame_number == 1 && mIsSyncFirstFrame) {
        HAL_LOGV("wait first frame sync.start.");
        Mutex::Autolock l(mWaitFrameLock);
        mWaitFrameSignal.waitRelative(mWaitFrameLock, 500e6);
        HAL_LOGV("wait first frame sync. succeed.");
    }
    mMetaNotifyIndex = cameraMNIndex;

#ifndef MINICAMERA
    int ret;
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t &output = request->output_buffers[i];
        sp<Fence> acquireFence = new Fence(output.acquire_fence);

        ret = acquireFence->wait(Fence::TIMEOUT_NEVER);
        if (ret) {
            HAL_LOGE("fence wait failed %d", ret);
            goto req_fail;
        }

        acquireFence = NULL;
    }
#endif

    // 4. DO request.
    for (int i = 0; i < m_nPhyCameras; i++) {
        SprdCamera3HWI *hwi = m_pPhyCamera[i].hwi;
        int buffer_num = 0;
        int streamConfig = req_stream_mak[i];
        HAL_LOGD("streamConfig = %d, i = %d", streamConfig, i);
        if ((mIsVideoMode == true) && (i != 0 && i != mRefIdex) &&
            (streamConfig == SNAPSHOT_STREAM_HAL_BUFFER)) {
            HAL_LOGV("skip_video_capture");
            continue;
        } else if ((i != mRefIdex) &&
            (streamConfig == CALLBACK_STREAM_HAL_BUFFER)) {
            HAL_LOGV("skip_dc_capture");
            continue;
        }

        if (!streamConfig) {
            HAL_LOGD("no need to config camera:%d", i);
            continue;
        } else if (streamConfig == 3 || streamConfig == (3 << 2) ||
                   streamConfig == (3 << 4) || streamConfig == (3 << 6) ||
                   streamConfig == (3 << 8)) {
            HAL_LOGE("error config camera:%d,please check stream config", i);
            goto req_fail;
        }

        camera3_capture_request_t *tempReq = &newReq[i];
        camera3_stream_buffer_t *tempOutBuffer =
            (camera3_stream_buffer_t *)out_buffer[i];

        rc = createOutputBufferStream(
            tempOutBuffer, streamConfig,
            (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[i], fw_buffer,
            &buffer_num);
        if (rc) {
            HAL_LOGE("failed to createOutputBufferStream, idx:%d,index=%d",
                tempReq->frame_number, i);
            goto req_fail;
        }
        if (!buffer_num) {
            HAL_LOGI("check num_output_buffers is 0");
            continue;
        }

        memcpy(tempReq, request, sizeof(camera3_capture_request_t));
        tempReq->num_output_buffers = buffer_num;
        tempReq->settings = metaSettings[i].release();
        tempReq->output_buffers = tempOutBuffer;
        HAL_LOGV("process request camera[%d] ,buffer_num %d,frame=%u",
            i, buffer_num, tempReq->frame_number);
        if (getStreamType(tempOutBuffer->stream) ==
            DEFAULT_STREAM /*&& i == 0*/) {
            mSavedCapReqsettings = clone_camera_metadata(tempReq->settings);
            HAL_LOGD("save main camera meta setting mSavedCapReqsettings %p",
                mSavedCapReqsettings);
        }
        if (mIsCapturing) {
            if (mRequstState == WAIT_FIRST_YUV_STATE) {
                struct timespec t1;
                clock_gettime(CLOCK_BOOTTIME, &t1);
                uint64_t currentmainTimestamp =
                    (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
                hwi->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            }
        }

        rc = hwi->process_capture_request(m_pPhyCamera[i].dev, tempReq);
        if (rc < 0) {
            HAL_LOGE("failed to process_capture_request, idx:%d,index=%d",
                tempReq->frame_number, i);
            goto req_fail;
        }
        if (mRefCameraId != mLastRefCameraId) {
            HAL_LOGI("cur %u last %u", mRefCameraId, mLastRefCameraId);
            mLastRefCameraId = mRefCameraId;
            hwi->setRefCameraId(mRefCameraId);
        }
        if (tempReq->settings)
            free_camera_metadata((camera_metadata_t *)(tempReq->settings));
    }

req_fail:

    return rc;
}

/*===========================================================================
 * FUNCTION     : process_capture_result_main
 *
 * DESCRIPTION : result of SprdCamera3MultiCamera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    // CHECK_BASE();
    gMultiCam->processCaptureResultMain((camera3_capture_result_t *)result);
}

/*===========================================================================
 * FUNCTION     : process_capture_result_aux1
 *
 * DESCRIPTION : result of SprdCamera3MultiCamera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::process_capture_result_aux1(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    // CHECK_BASE();
    gMultiCam->processCaptureResultAux1(result);
}

/*===========================================================================
 * FUNCTION     : process_capture_result_aux2
 *
 * DESCRIPTION : result of SprdCamera3MultiCamera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::process_capture_result_aux2(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    // CHECK_BASE();
    gMultiCam->processCaptureResultAux2(result);
}

/*===========================================================================
 * FUNCTION     : process_capture_result_aux3
 *
 * DESCRIPTION : result of SprdCamera3MultiCamera
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
/*
void SprdCamera3MultiCamera::process_capture_result_aux3(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    //CHECK_BASE();
    gMultiCam->processCaptureResultAux3(result);
}
*/

/*===========================================================================
 * FUNCTION     : notifyMain
 *
 * DESCRIPTION : main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notifyMain(const struct camera3_callback_ops *ops,
                                        const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%u", msg->message.shutter.frame_number);
    // CHECK_BASE();
    gMultiCam->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION     : notify_Aux1
 *
 * DESCRIPTION : Aux sensor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notify_Aux1(const struct camera3_callback_ops *ops,
                                         const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%u", msg->message.shutter.frame_number);
    // CHECK_BASE();
    gMultiCam->notifyAux1(msg);
}

/*===========================================================================
 * FUNCTION     : notifyAux1
 *
 * DESCRIPTION : process notify of camera index 1
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notifyAux1(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    // CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }

    int index = findMNIndex(cur_frame_number);
    HAL_LOGD("notifyAux1_index=%d", index);
     if (index == 1){
        mCallbackOps->notify(mCallbackOps, msg);
    }
    {
        HAL_LOGV("mNotifyListAux1.push_back");
        mNotifyListAux1.push_back(*msg);
        if ((mNotifyListAux1.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListAux1.begin();
            mNotifyListAux1.erase(itor);
        }
        mNotifyListVideoAux1.push_back(*msg);
        if ((mNotifyListVideoAux1.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor_video = mNotifyListVideoAux1.begin();
            mNotifyListVideoAux1.erase(itor_video);
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION     : notify_Aux2
 *
 * DESCRIPTION : Aux sensor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notify_Aux2(const struct camera3_callback_ops *ops,
                                         const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%u", msg->message.shutter.frame_number);
    // CHECK_BASE();
    gMultiCam->notifyAux2(msg);
}

/*===========================================================================
 * FUNCTION     : notifyAux2
 *
 * DESCRIPTION : process notify of camera index 2
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notifyAux2(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    // CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }

    int index = findMNIndex(cur_frame_number);
    HAL_LOGD("notifyAux2_index=%d", index);
    if (index == 2){
        mCallbackOps->notify(mCallbackOps, msg);
    }

    {
        HAL_LOGV("mNotifyListAux2.push_back");
        mNotifyListAux2.push_back(*msg);
        if ((mNotifyListAux2.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListAux2.begin();
            mNotifyListAux2.erase(itor);
        }
        mNotifyListVideoAux2.push_back(*msg);
        if ((mNotifyListVideoAux2.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor_video = mNotifyListVideoAux2.begin();
            mNotifyListVideoAux2.erase(itor_video);
        }
    }
    return;
}

/*===========================================================================
 * FUNCTION     : notify_Aux3
 *
 * DESCRIPTION : Aux sensor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notify_Aux3(const struct camera3_callback_ops *ops,
                                         const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%u", msg->message.shutter.frame_number);
    // CHECK_BASE();
    gMultiCam->notifyAux3(msg);
}

/*===========================================================================
 * FUNCTION     : notifyAux3
 *
 * DESCRIPTION : process notify of camera index 3
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notifyAux3(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    // CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }

    int index = findMNIndex(cur_frame_number);
    HAL_LOGV("notifyAux3_index=%d", index);
    if (index == 3) {
        mCallbackOps->notify(mCallbackOps, msg);
    }
    {
        HAL_LOGV("mNotifyListAux3.push_back");
        mNotifyListAux3.push_back(*msg);
        if ((mNotifyListAux3.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListAux3.begin();
            mNotifyListAux3.erase(itor);
        }
        mNotifyListVideoAux3.push_back(*msg);
        if ((mNotifyListVideoAux3.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor_video = mNotifyListVideoAux3.begin();
            mNotifyListVideoAux3.erase(itor_video);
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION     : notifyMain
 *
 * DESCRIPTION : process notify of camera index 0
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::notifyMain(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    // CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }
    int index = findMNIndex(cur_frame_number);
    HAL_LOGD("notifyMain_index=%d", index);
    if (index == 0) {
        mCallbackOps->notify(mCallbackOps, msg);
    }
    {
        HAL_LOGV("mNotifyListMain.push_back");
        mNotifyListMain.push_back(*msg);
        if ((mNotifyListMain.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListMain.begin();
            mNotifyListMain.erase(itor);
        }
        mNotifyListVideoMain.push_back(*msg);
        if ((mNotifyListVideoMain.size() == MAX_NOTIFY_QUEUE_SIZE)) {
            List<camera3_notify_msg_t>::iterator itor_video = mNotifyListVideoMain.begin();
            mNotifyListVideoMain.erase(itor_video);
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION     : dump
 *
 * DESCRIPTION :
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    // CHECK_BASE();
    gMultiCam->_dump(device, fd);
    HAL_LOGV("X");
}

/*===========================================================================
 * FUNCTION     : _dump
 *
 * DESCRIPTION :
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::_dump(const struct camera3_device *device,
                                   int fd) {
    HAL_LOGI("E");

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : flush
 *
 * DESCRIPTION :
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::flush(const struct camera3_device *device) {
    int rc = 0;
    HAL_LOGV(" E");
    // CHECK_BASE_ERROR();
    gMultiCam->mFlushing = true;

    rc = gMultiCam->_flush(device);

    HAL_LOGV(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION     : _flush
 *
 * DESCRIPTION : flush
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3MultiCamera::_flush(const struct camera3_device *device) {
    HAL_LOGI("E");

    int rc = 0;

    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[i];
        SprdCamera3HWI *hwi = sprdCam.hwi;
        rc = hwi->flush(sprdCam.dev);
        if (rc < 0) {
            HAL_LOGE("flush err !! ");
            return -1;
        }
    }
    reConfigFlush();

    HAL_LOGI("X");
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION     : CallBackMetadata
 *
 * DESCRIPTION : CallBackMetadata
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3MultiCamera::CallBackMetadata() {

    camera3_capture_result_t result;
    bzero(&result, sizeof(camera3_capture_result_t));
    List<meta_save_t>::iterator itor;
    {
        Mutex::Autolock l(mMetatLock);
        itor = mMetadataList.begin();
        while (itor != mMetadataList.end()) {
            result.frame_number = itor->frame_number;
            result.result = reConfigResultMeta(itor->metadata);
            result.num_output_buffers = 0;
            result.output_buffers = NULL;
            result.input_buffer = NULL;
            result.partial_result = 1;
            HAL_LOGD("send meta id=%d, %p", result.frame_number, result.result);
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(result.result));
            mMetadataList.erase(itor);
            itor++;
            {
                Mutex::Autolock l(mWaitFrameLock);
                if (result.frame_number) {
                    mSendFrameNum = result.frame_number;
                }
                if (result.frame_number == mWaitFrameNum) {
                    mWaitFrameSignal.signal();
                    HAL_LOGD("wake up request");
                }
            }
        }
    }
}

/*===========================================================================
 * FUNCTION     : CallBackResult
 *
 * DESCRIPTION : CallBackResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3MultiCamera::CallBackResult(uint32_t frame_number,
                                            int buffer_status, int stream_type,
                                            int camera_index) {
    int rc = 0;
    bool callbackflag = false;
    bool previewflag = false;
    void *output_buf1_addr = NULL;
    void *output_callback_addr = NULL;
    camera3_capture_result_t result;
    List<multi_request_saved_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;
    camera3_capture_result_t result_callback;
    List<multi_request_saved_t>::iterator itor_callback;
    camera3_stream_buffer_t result_callback_buffers;

    bzero(&result_callback, sizeof(camera3_capture_result_t));
    bzero(&result_callback_buffers, sizeof(camera3_stream_buffer_t));
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));
    List<multi_request_saved_t> *mSavedRequestList = NULL;
    List<multi_request_saved_t> *mSavedCallRequestList = NULL;

    if (stream_type == CALLBACK_STREAM) {
        mSavedRequestList = &mSavedPrevRequestList;
        mSavedCallRequestList = &mSavedCallbackRequestList;
    } else if (stream_type == DEFAULT_STREAM) {
        mSavedRequestList = &mSavedCallbackRequestList;
    } else if (stream_type == VIDEO_STREAM) {
        mSavedRequestList = &mSavedVideoRequestList;
    }

    if (stream_type != SNAPSHOT_STREAM) {
        Mutex::Autolock l(mRequestLock);
        if (stream_type == CALLBACK_STREAM) {
            itor_callback = mSavedCallRequestList->begin();
            while (itor_callback != mSavedCallRequestList->end()) {
                if (itor_callback->frame_number == frame_number) {
                    if (camera_index != itor_callback->metaNotifyIndex) {
                        return;
                    }
                    result_callback_buffers.stream = itor_callback->stream;
                    result_callback_buffers.buffer = itor_callback->buffer;
                    mSavedCallRequestList->erase(itor_callback);
                    callbackflag = true;
                    break;
                }
                itor_callback++;
            }
        }
        itor = mSavedRequestList->begin();
        while (itor != mSavedRequestList->end()) {
            if (itor->frame_number == frame_number) {
                if (camera_index != itor->metaNotifyIndex) {
                    return;
                }
                result_buffers.stream = itor->stream;
                result_buffers.buffer = itor->buffer;
                mSavedRequestList->erase(itor);
                previewflag = true;
                break;
            }
            itor++;
        }
        if (itor == mSavedRequestList->end()) {
            HAL_LOGV("can't find frame:%u", frame_number);
            return;
        }
    } else {
        result_buffers.stream = mSavedSnapRequest.snap_stream;
        result_buffers.buffer = mSavedSnapRequest.buffer;
    }

    HAL_LOGD("send frame %u, status.%d, result_buffers.buffer %p,"
        "camera_index %d, stream_type %d callbackflag %d previewflag %d",
        frame_number, buffer_status, &result_buffers.buffer,
        camera_index, stream_type, callbackflag, previewflag);

    CallBackMetadata();
    if (callbackflag && previewflag) {
        rc = gMultiCam->map(result_callback_buffers.buffer,
                            &output_callback_addr);
        rc = gMultiCam->map(result_buffers.buffer, &output_buf1_addr);
        memcpy(output_callback_addr, output_buf1_addr,
               ADP_BUFSIZE(*result_callback_buffers.buffer));
        gMultiCam->unmap(result_buffers.buffer);
        gMultiCam->unmap(result_callback_buffers.buffer);

        result_callback_buffers.status = buffer_status;
        result_callback_buffers.acquire_fence = -1;
        result_callback_buffers.release_fence = -1;
        result_callback.result = NULL;
        result_callback.frame_number = frame_number;
        result_callback.num_output_buffers = 1;
        result_callback.output_buffers = &result_callback_buffers;
        result_callback.input_buffer = NULL;
        result_callback.partial_result = 0;
        mCallbackOps->process_capture_result(mCallbackOps, &result_callback);
    }

    result_buffers.status = buffer_status;
    result_buffers.acquire_fence = -1;
    result_buffers.release_fence = -1;
    result.result = NULL;
    result.frame_number = frame_number;
    result.num_output_buffers = 1;
    result.output_buffers = &result_buffers;
    result.input_buffer = NULL;
    result.partial_result = 0;
    mCallbackOps->process_capture_result(mCallbackOps, &result);
}

/*===========================================================================
 * FUNCTION     : parse_configure_info
 *
 * DESCRIPTION : load configure file
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::parse_configure_info(
    config_multi_camera *config_info) {

    if (!config_info) {
        HAL_LOGE("failed to get common configure info");
        return;
    }

    mMultiMode = config_info->mode;
    m_nPhyCameras = config_info->total_config_camera;
    m_VirtualCamera.id = config_info->virtual_camera_id;

    mCameraIdWide =
        findSensorRole(MODULE_OPTICSZOOM_WIDE_BACK);
    mCameraIdSw =
        findSensorRole(MODULE_SPW_NONE_BACK);
    mCameraIdTele =
        findSensorRole(MODULE_OPTICSZOOM_TELE_BACK);
    MultiConfigId[0] = mCameraIdWide;
    MultiConfigId[1] = mCameraIdSw;
    MultiConfigId[2] = mCameraIdTele;

    // mBufferInfo
    memcpy(&mBufferInfo, config_info->buffer_info,
           sizeof(hal_buffer_info) * MAX_MULTI_NUM_BUFFER);

    // init m_nPhyCameras
    for (int i = 0; i < m_nPhyCameras; i++) {
        config_physical_descriptor *phy_cam_info =
            (config_physical_descriptor *)&config_info->multi_phy_info[i];
        m_pPhyCamera[i].id = MultiConfigId[i];
        m_pPhyCamera[i].stream_num = phy_cam_info->config_stream_num;
        memcpy(&m_pPhyCamera[i].hal_stream, &(phy_cam_info->hal_stream),
               sizeof(hal_stream_info) * MAX_MULTI_NUM_STREAMS);
    }
    // init mHalReqConfigStreamInfo

    memcpy(&mHalReqConfigStreamInfo, &(config_info->hal_req_config_stream),
           sizeof(hal_req_stream_config_total) * MAX_MULTI_NUM_STREAMS);
}

/*===========================================================================
 * FUNCTION     : freeLocalBuffer
 *
 * DESCRIPTION : free buffer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::freeLocalBuffer() {
    for (int i = 0; i < mLocalBufferNumber; i++) {
        freeOneBuffer(&mLocalBuffer[i]);
    }

    mLocalBufferNumber = 0;
    mLocalBufferList.clear();
}

 /*===========================================================================
 * FUNCTION     : video_parse_configure_info
 *
 * DESCRIPTION : load configure file
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::video_parse_configure_info(
    config_multi_camera *config_info) {

    if (!config_info) {
        HAL_LOGE("failed to get video configure info");
        return;
    }

    mMultiMode = config_info->mode;
    m_nPhyCameras = config_info->total_config_camera;
    m_VirtualCamera.id = config_info->virtual_camera_id;

    // video mBufferInfo
    memcpy(&mBufferInfo, config_info->video_buffer_info,
           sizeof(hal_buffer_info) * MAX_MULTI_NUM_BUFFER);

    // init video m_nPhyCameras
    for (int i = 0; i < m_nPhyCameras; i++) {
        config_physical_descriptor *phy_cam_info =
            (config_physical_descriptor *)&config_info->video_multi_phy_info[i];
        m_pPhyCamera[i].id = MultiConfigId[i];
        m_pPhyCamera[i].stream_num = phy_cam_info->config_stream_num;
        memcpy(&m_pPhyCamera[i].hal_stream, &(phy_cam_info->hal_stream),
               sizeof(hal_stream_info) * MAX_MULTI_NUM_STREAMS);
    }

    // init video mHalReqConfigStreamInfo
    memcpy(&mHalReqConfigStreamInfo, &(config_info->video_hal_req_config_stream),
           sizeof(hal_req_stream_config_total) * MAX_MULTI_NUM_STREAMS);
}

/*===========================================================================
 * FUNCTION     : allocateBuff
 *
 * DESCRIPTION : allocate buffer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::allocateBuff() {
    int ret = 0;
    freeLocalBuffer();

    for (int i = 0; i < MAX_MULTI_NUM_BUFFER; i++) {
        int width = mBufferInfo[i].width;
        int height = mBufferInfo[i].height;
        int follow_type = mBufferInfo[i].follow_type;
        HAL_LOGV("i = %d, number = %d, follow_type = %d",
            i, mBufferInfo[i].number, follow_type);
        if (!mBufferInfo[i].number) {
            return ret;
        }

        if (follow_type != 0) {
            int camera_index = mBufferInfo[i].follow_camera_index;
            for (int j = 0; j < m_pPhyCamera[i].stream_num; j++) {
                camera3_stream_t *find_stream =
                    findStream(follow_type, (sprdcamera_physical_descriptor_t
                                                 *)&m_pPhyCamera[camera_index]);
                CHECK_STREAM_ERROR(find_stream);
                mBufferInfo[i].width = find_stream->width;
                mBufferInfo[i].height = find_stream->height;
            HAL_LOGV("buffer. j=%d,i=%d,w%d,h%d", j,i,
                mBufferInfo[i].width, mBufferInfo[i].height);
            }
        }

        for (int j = 0; j < mBufferInfo[i].number; j++) {
            HAL_LOGD("buffer. j=%d,num=%d,w%d,h%d", j,
                mBufferInfo[i].number, mBufferInfo[i].width, mBufferInfo[i].height);
            if (0 > allocateOne(mBufferInfo[i].width, mBufferInfo[i].height,
                                &(mLocalBuffer[mLocalBufferNumber]), YUV420)) {
                HAL_LOGE("request one buf failed.");
                return -1;
            }
            mLocalBufferList.push_back(&(mLocalBuffer[mLocalBufferNumber]));
            mLocalBufferNumber++;
        }
    }
    HAL_LOGD(" mLocalBufferNumber %d", mLocalBufferNumber);

    return ret;
}

/*===========================================================================
 * FUNCTION     : findStream
 *
 * DESCRIPTION : find a match stream from sprdcamera_physical_descriptor_t
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
camera3_stream_t *SprdCamera3MultiCamera::findStream(
    int type, const sprdcamera_physical_descriptor_t *phy_cam_info) {
    camera3_stream_t *ret_stream = NULL;
    for (int i = 0; i < phy_cam_info->stream_num; i++) {
        HAL_LOGV("hal_stream[i : %d].type %d, type %d, stream_num %d", i,
            phy_cam_info->hal_stream[i].type, type, phy_cam_info->stream_num);
        if (phy_cam_info->hal_stream[i].type == type) {
            ret_stream = (camera3_stream_t *)&(phy_cam_info->streams[i]);
            break;
        }
    }
    if (!ret_stream) {
        HAL_LOGE("failed.find stream");
    }
    HAL_LOGV("width %d height %d", ret_stream->width, ret_stream->height);
    return ret_stream;
}

/*===========================================================================
 * FUNCTION     : findHalReq
 *
 * DESCRIPTION : find a match hal_req_stream_config from
 *    hal_req_stream_config_total
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
hal_req_stream_config *SprdCamera3MultiCamera::findHalReq(
    int type, const hal_req_stream_config_total *total_config) {
    hal_req_stream_config *hal_req = NULL;
    int i = 0;
    for (i = 0; i < total_config->total_config_stream; i++) {

        hal_req_stream_config *cur_req_config =
            (hal_req_stream_config *)&(total_config->hal_req_config[i]);

        HAL_LOGV("type %d, cur_req_config->roi_stream_type %d",
            type, cur_req_config->roi_stream_type);

        if (cur_req_config->roi_stream_type == type) {
            hal_req = cur_req_config;
            break;
        }
    }

    if (!hal_req) {
        HAL_LOGD("Cann't find config request for stream=%d", type);
    }
    return hal_req;
}

/*===========================================================================
 * FUNCTION     : findMNIndex
 *
 * DESCRIPTION : calculate meatadata and notify index
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::findMNIndex(uint32_t frame_number) {
    int index = -1;

    HAL_LOGV("frame_number %d mCapFrameNum %d",
        frame_number, mCapFrameNum);

    if (frame_number == mCapFrameNum) {
        if (mRequstState != REPROCESS_STATE) {
            index = mSavedSnapRequest.metaNotifyIndex;
        } else {
            index = -1;
        }
        HAL_LOGV("capture frame index = %d", index);
    } else {
        List<multi_request_saved_t>::iterator itor;
        Mutex::Autolock l(mRequestLock);
        itor = mSavedPrevRequestList.begin();
        while (itor != mSavedPrevRequestList.end()) {
            if (itor->frame_number == frame_number) {
                index = itor->metaNotifyIndex;
                break;
            }
            itor++;
        }
        if (itor == mSavedPrevRequestList.end()) {
            HAL_LOGV("can't find frame:%u", frame_number);
            index = -1;
        }
    }

    HAL_LOGV("MNIndex = %d", index);

    return index;
}

/*===========================================================================
 * FUNCTION     : isFWBuffer
 *
 * DESCRIPTION : verify FW Buffer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
bool SprdCamera3MultiCamera::isFWBuffer(buffer_handle_t *MatchBuffer) {
    Mutex::Autolock l(mRequestLock);
    List<multi_request_saved_t>::iterator itor;
    if (mSavedPrevRequestList.empty()) {
        HAL_LOGE("mSavedPrevRequestList is empty ");
        return false;
    } else {
        itor = mSavedPrevRequestList.begin();
        while (itor != mSavedPrevRequestList.end()) {
            if (MatchBuffer == (itor->buffer)) {
                return true;
            }
            itor++;
        }
        return false;
    }
}

/*===========================================================================
 * FUNCTION     : load_config_file
 *
 * DESCRIPTION : load multi_camera configure file
 *
  * PARAMETERS :
 *
 * RETURN     : NONE
 *==========================================================================*/
config_multi_camera *SprdCamera3MultiCamera::load_config_file(void) {

    HAL_LOGD("load_config_file ");
#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT
    return &wide_ultra_config;
#else
    return &multi_camera_config;
#endif
}

/*===========================================================================
 * FUNCTION     : reConfigResultMeta
 *
 * DESCRIPTION : reConfigResultMeta
 *
  * PARAMETERS :
 *
 * RETURN     : meta
 *==========================================================================*/
camera_metadata_t *
SprdCamera3MultiCamera::reConfigResultMeta(camera_metadata_t *meta) {
    CameraMetadata *camMetadata = new CameraMetadata(meta);
    uint8_t face_num = 0;
    uint8_t real_face_num = 0;
    int32_t crop_Region[4] = {0};
    camera_metadata_entry_t entry;
    entry = camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES);
    if (entry.count != 0) {
        face_num = entry.count / 4;
        real_face_num = entry.count / 4;
    } else {
        face_num = 0;
        real_face_num = 0;
    }
    HAL_LOGV("face_num1 = %d, real_face_num1 = %d", face_num, real_face_num);

    if (mZoomValue < mSwitch_W_Sw_Threshold) {
        face_num = gMultiCam->aux1_face_number;
    }

    if (mZoomValue >= mSwitch_W_T_Threshold) {
        face_num = gMultiCam->aux2_face_number;
    }

    HAL_LOGV("face_num2 = %d, real_face_num2 = %d", face_num, real_face_num);

    if (mZoomValue < mSwitch_W_Sw_Threshold) { // super wide
        float left_offest = 0, top_offest = 0;
        left_offest = ((float)mWideMaxWidth / 0.6 - (float)mSwMaxWidth) / 2;
        top_offest = ((float)mWideMaxHeight / 0.6 - (float)mSwMaxHeight) / 2;
        crop_Region[2] = static_cast<int32_t>(
            static_cast<float>(mSwMaxWidth * 0.6f) / (mZoomValue));
        crop_Region[3] = static_cast<int32_t>(
            static_cast<float>(mSwMaxHeight * 0.6f) / (mZoomValue));
        crop_Region[0] = ((mSwMaxWidth - crop_Region[2]) >> 1) + left_offest;
        crop_Region[1] = ((mSwMaxHeight - crop_Region[3]) >> 1) + top_offest;
        camMetadata->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                            ARRAY_SIZE(crop_Region));
        camMetadata->update(ANDROID_SPRD_AI_SCENE_TYPE_CURRENT,
                            &(gMultiCam->aux1_ai_scene_type), 1);
        camMetadata->update(ANDROID_CONTROL_AF_STATE,
                            &(gMultiCam->aux1_af_state), 1);
        HAL_LOGD("sw crop=%d,%d,%d,%d ai_scene_type %d,aux1_af_state %d",
            crop_Region[0], crop_Region[1], crop_Region[2],
            crop_Region[3], gMultiCam->aux1_ai_scene_type, gMultiCam->aux1_af_state);
    } else if (mZoomValue >= mSwitch_W_T_Threshold) { // tele
        float inputRatio = mZoomValue / mSwitch_W_T_Threshold;
        HAL_LOGD("tele inputRatio = %f", inputRatio);
        inputRatio = inputRatio > 4 ? 4 : inputRatio;
        float left_offest = 0, top_offest = 0;
        left_offest = ((float)mWideMaxWidth / 0.6 - (float)mTeleMaxWidth) / 2;
        top_offest = ((float)mWideMaxHeight / 0.6 - (float)mTeleMaxHeight) / 2;
        crop_Region[2] = static_cast<int32_t>(
            static_cast<float>(mTeleMaxWidth) / (inputRatio));
        crop_Region[3] = static_cast<int32_t>(
            static_cast<float>(mTeleMaxHeight) / (inputRatio));
        crop_Region[0] = ((mTeleMaxWidth - crop_Region[2]) >> 1) + left_offest;
        crop_Region[1] = ((mTeleMaxHeight - crop_Region[3]) >> 1) + top_offest;
        camMetadata->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                            ARRAY_SIZE(crop_Region));
        camMetadata->update(ANDROID_SPRD_AI_SCENE_TYPE_CURRENT,
                            &(gMultiCam->aux2_ai_scene_type), 1);
        camMetadata->update(ANDROID_CONTROL_AF_STATE,
                            &(gMultiCam->aux2_af_state), 1);
        HAL_LOGD("tele crop=%d,%d,%d,%d ai_scene_type %d,aux2_af_state %d",
            crop_Region[0], crop_Region[1], crop_Region[2],
            crop_Region[3], gMultiCam->aux2_ai_scene_type, gMultiCam->aux2_af_state);
    } else { // wide
        float left_offest = 0, top_offest = 0;
        left_offest = ((float)mWideMaxWidth / 0.6 - (float)mWideMaxWidth) / 2;
        top_offest = ((float)mWideMaxHeight / 0.6 - (float)mWideMaxHeight) / 2;
        crop_Region[2] = static_cast<int32_t>(
            static_cast<float>(mWideMaxWidth) / (mZoomValue));
        crop_Region[3] = static_cast<int32_t>(
            static_cast<float>(mWideMaxHeight) / (mZoomValue));
        crop_Region[0] = ((mWideMaxWidth - crop_Region[2]) >> 1) + left_offest;
        crop_Region[1] = ((mWideMaxHeight - crop_Region[3]) >> 1) + top_offest;
        camMetadata->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                            ARRAY_SIZE(crop_Region));
        HAL_LOGV("wide crop=%d,%d,%d,%d",
            crop_Region[0], crop_Region[1], crop_Region[2], crop_Region[3]);
    }

    HAL_LOGV("face_num=%d,camMetadata=%p", face_num, camMetadata);
    if (face_num) {
        if (mZoomValue >= mSwitch_W_Sw_Threshold &&
            mZoomValue < mSwitch_W_T_Threshold && real_face_num) { // wide
            for (int i = 0; i < real_face_num; i++) {
                float left_offest = 0, top_offest = 0;
                left_offest =
                    ((float)mWideMaxWidth / 0.6 - (float)mWideMaxWidth) / 2;
                top_offest =
                    ((float)mWideMaxHeight / 0.6 - (float)mWideMaxHeight) / 2;
                main_FaceRect[i * 4 + 0] =
                    camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                        .data.i32[i * 4 + 0] +
                    left_offest;
                main_FaceRect[i * 4 + 1] =
                    camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                        .data.i32[i * 4 + 1] +
                    top_offest;
                main_FaceRect[i * 4 + 2] =
                    camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                        .data.i32[i * 4 + 2] +
                    left_offest;
                main_FaceRect[i * 4 + 3] =
                    camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                        .data.i32[i * 4 + 3] +
                    top_offest;
                HAL_LOGD("the wide face %d, main_FaceRect = %d, %d, %d, %d ", i,
                    main_FaceRect[i * 4], main_FaceRect[i * 4 + 1],
                    main_FaceRect[i * 4 + 2], main_FaceRect[i * 4 + 3]);
            }
            camMetadata->update(ANDROID_STATISTICS_FACE_RECTANGLES,
                                main_FaceRect, face_num * 4);
        } else if (mZoomValue < mSwitch_W_Sw_Threshold) { // super wide
            camMetadata->update(ANDROID_STATISTICS_FACE_RECTANGLES,
                                gMultiCam->aux1_FaceRect, face_num * 4);
            camMetadata->update(ANDROID_SPRD_FACE_ATTRIBUTES,
                                gMultiCam->aux1_FaceGenderRaceAge, face_num);
        } else { // tele
            camMetadata->update(ANDROID_STATISTICS_FACE_RECTANGLES,
                                gMultiCam->aux2_FaceRect, face_num * 4);
            camMetadata->update(ANDROID_SPRD_FACE_ATTRIBUTES,
                                gMultiCam->aux2_FaceGenderRaceAge, face_num);
        }
    }

    meta = camMetadata->release();
    delete camMetadata;
    return meta;
}

/*===========================================================================
 * FUNCTION     : setZoomCropRegion
 *
 * DESCRIPTION : set zoom crop region meta info
 *
  * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::setZoomCropRegion(CameraMetadata *WideSettings,
                                               CameraMetadata *TeleSettings,
                                               CameraMetadata *SwSettings,
                                               float multi_zoom_ratio) {
    int ret = 0;
    mIsSyncFirstFrame = true;
    int32_t crop_Region[4] = {0};
    int32_t crop_Region_sw[4] = {0};

    HAL_LOGD("multi_zoom_ratio = %f", multi_zoom_ratio);

    if (multi_zoom_ratio < mSwitch_W_Sw_Threshold) { // super wide
        crop_Region[2] = static_cast<int32_t>(
            static_cast<float>(mSwMaxWidth * 0.6f) / (multi_zoom_ratio));
        crop_Region[3] = static_cast<int32_t>(
            static_cast<float>(mSwMaxHeight * 0.6f) / (multi_zoom_ratio));
        crop_Region[0] = (mSwMaxWidth - crop_Region[2]) >> 1;
        crop_Region[1] = (mSwMaxHeight - crop_Region[3]) >> 1;
        SwSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                           ARRAY_SIZE(crop_Region));
        HAL_LOGD("sw crop=%d,%d,%d,%d",
            crop_Region[0], crop_Region[1], crop_Region[2], crop_Region[3]);
        mRefCameraId = 2;
    } else if (multi_zoom_ratio >= mSwitch_W_T_Threshold) { // tele
        float inputRatio = multi_zoom_ratio / mSwitch_W_T_Threshold;
        HAL_LOGD("tele inputRatio = %f", inputRatio);
        inputRatio = inputRatio > 4 ? 4 : inputRatio;
        crop_Region[2] = static_cast<int32_t>(
            static_cast<float>(mTeleMaxWidth) / (inputRatio));
        crop_Region[3] = static_cast<int32_t>(
            static_cast<float>(mTeleMaxHeight) / (inputRatio));
        crop_Region[0] = (mTeleMaxWidth - crop_Region[2]) >> 1;
        crop_Region[1] = (mTeleMaxHeight - crop_Region[3]) >> 1;
        TeleSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                             ARRAY_SIZE(crop_Region));
        HAL_LOGD("tele crop=%d,%d,%d,%d",
            crop_Region[0], crop_Region[1], crop_Region[2], crop_Region[3]);
        mRefCameraId = 3;
    } else { // wide
        crop_Region_sw[2] = static_cast<int32_t>(
            static_cast<float>(mSwMaxWidth * 0.6f) / (0.9999));
        crop_Region_sw[3] = static_cast<int32_t>(
            static_cast<float>(mSwMaxHeight * 0.6f) / (0.9999));
        crop_Region_sw[0] = (mSwMaxWidth - crop_Region_sw[2]) >> 1;
        crop_Region_sw[1] = (mSwMaxHeight - crop_Region_sw[3]) >> 1;
        SwSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region_sw,
                           ARRAY_SIZE(crop_Region_sw));

        crop_Region[2] = static_cast<int32_t>(
            static_cast<float>(mWideMaxWidth) / (multi_zoom_ratio));
        crop_Region[3] = static_cast<int32_t>(
            static_cast<float>(mWideMaxHeight) / (multi_zoom_ratio));
        crop_Region[0] = (mWideMaxWidth - crop_Region[2]) >> 1;
        crop_Region[1] = (mWideMaxHeight - crop_Region[3]) >> 1;
        WideSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                             ARRAY_SIZE(crop_Region));
        HAL_LOGD("wide crop=%d,%d,%d,%d",
            crop_Region[0], crop_Region[1], crop_Region[2], crop_Region[3]);
        mRefCameraId = 0;
    }
}

/*===========================================================================
 * FUNCTION     : reReqConfig
 *
 * DESCRIPTION : reconfigure request
 *
 * PARAMETERS :
 *
 * RETURN     : NONE
 *==========================================================================*/
void SprdCamera3MultiCamera::reReqConfig(camera3_capture_request_t *request,
                                         CameraMetadata *meta) {
    int tagCnt = 0;
    CameraMetadata *metaSettingsWide = &meta[CAM_TYPE_MAIN];
    CameraMetadata *metaSettingsSw = &meta[CAM_TYPE_AUX1];
    CameraMetadata *metaSettingsTele = &meta[CAM_TYPE_AUX2];
    cmr_u16 snsW, snsH;
    int32_t crop_region[4] = {0};
    int32_t touch_area[5] = {0};
    int32_t af_area[5] = {0};
    int32_t ae_area[5] = {0};
    uint8_t afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;

    // meta config
    if (!meta) {
        return;
    }
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };
    /*
        if (meta->exists(ANDROID_SPRD_ZOOM_RATIO)) {
            mZoomValue =
                meta->find(ANDROID_SPRD_ZOOM_RATIO).data.f[0];
            HAL_LOGD("mZoomValue %f", mZoomValue);
        }
        HAL_LOGD("mZoomValue %f", mZoomValue);
    */
    int tagCnt1 = meta[CAM_TYPE_MAIN].entryCount();
    if (tagCnt1 != 0) {
        if (meta[CAM_TYPE_MAIN].exists(ANDROID_SPRD_BURSTMODE_ENABLED)) {
            uint8_t sprdBurstModeEnabled = 0;
            meta[CAM_TYPE_MAIN].update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                       &sprdBurstModeEnabled, 1);
            meta[CAM_TYPE_AUX1].update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                       &sprdBurstModeEnabled, 1);
            meta[CAM_TYPE_AUX2].update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                       &sprdBurstModeEnabled, 1);
        }

        uint8_t sprdZslEnabled = 1;
        meta[CAM_TYPE_MAIN].update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled,
                                   1);
        meta[CAM_TYPE_AUX1].update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled,
                                   1);
        meta[CAM_TYPE_AUX2].update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled,
                                   1);
    }

    SprdCamera3Setting::getLargestSensorSize(mCameraIdWide, &snsW, &snsH);
    mWideMaxWidth = snsW;
    mWideMaxHeight = snsH;

    SprdCamera3Setting::getLargestSensorSize(mCameraIdSw, &snsW, &snsH);
    mSwMaxWidth = snsW;
    mSwMaxHeight = snsH;

    SprdCamera3Setting::getLargestSensorSize(mCameraIdTele, &snsW, &snsH);
    mTeleMaxWidth = snsW;
    mTeleMaxHeight = snsH;


    HAL_LOGV("mWideMaxWidth=%d, mWideMaxHeight=%d, mTeleMaxWidth=%d, "
        "mTeleMaxHeight=%d, mSwMaxWidth=%d, mSwMaxHeight=%d",
        mWideMaxWidth, mWideMaxHeight, mTeleMaxWidth,
        mTeleMaxHeight, mSwMaxWidth, mSwMaxHeight);

    if (meta->exists(ANDROID_SCALER_CROP_REGION)) {
        crop_region[0] = meta->find(ANDROID_SCALER_CROP_REGION).data.i32[0];
        crop_region[1] = meta->find(ANDROID_SCALER_CROP_REGION).data.i32[1];
        crop_region[2] = meta->find(ANDROID_SCALER_CROP_REGION).data.i32[2];
        crop_region[3] = meta->find(ANDROID_SCALER_CROP_REGION).data.i32[3];
        HAL_LOGV("app crop=%d,%d,%d,%d",
            crop_region[0], crop_region[1], crop_region[2], crop_region[3]);
        mZoomValue = (float)mWideMaxWidth / (float)crop_region[2];
    }
    HAL_LOGV("app_mZoomValue=%f", mZoomValue);

    tagCnt = metaSettingsWide->entryCount();
    if (tagCnt != 0) {
        setZoomCropRegion(metaSettingsWide, metaSettingsTele, metaSettingsSw,
                          mZoomValue);
        if (mZoomValue == -1) {
        } else if (mZoomValue < mSwitch_W_T_Threshold &&
                   mZoomValue >= mSwitch_W_Sw_Threshold && mAlgoStatus == 1) {
            mReqConfigNum = 0;
        } else {
            mReqConfigNum = 1;
        }
        /*
        if (metaSettingsWide->exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
            int32_t aeTargetFpsRange[2] = {20, 20};
            metaSettingsWide->update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                aeTargetFpsRange,
                ARRAY_SIZE(aeTargetFpsRange));
            metaSettingsSw->update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                aeTargetFpsRange,
                ARRAY_SIZE(aeTargetFpsRange));
            metaSettingsTele->update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                aeTargetFpsRange,
                ARRAY_SIZE(aeTargetFpsRange));
        }
        */

        if (meta->exists(ANDROID_SPRD_TOUCH_INFO)) {
            touch_area[0] = meta->find(ANDROID_SPRD_TOUCH_INFO).data.i32[0];
            touch_area[1] = meta->find(ANDROID_SPRD_TOUCH_INFO).data.i32[1];
            touch_area[2] = meta->find(ANDROID_SPRD_TOUCH_INFO).data.i32[2];
            touch_area[3] = meta->find(ANDROID_SPRD_TOUCH_INFO).data.i32[3];
            touch_area[4] = meta->find(ANDROID_SPRD_TOUCH_INFO).data.i32[4];
            HAL_LOGV("app touch_area = %d, %d, %d, %d, %d",
                touch_area[0], touch_area[1], touch_area[2],
                touch_area[3], touch_area[4]);
        }

        if (mZoomValue < mSwitch_W_Sw_Threshold) { // super wide
            float left_offest = 0, top_offest = 0;
            float left_dst = 0, top_dst = 0;
            left_offest = ((float)mWideMaxWidth - (float)mSwMaxWidth) / 2;
            top_offest = ((float)mWideMaxHeight - (float)mSwMaxHeight) / 2;
            left_dst = touch_area[2] - touch_area[0];
            top_dst = touch_area[3] - touch_area[1];
            // touch_area[0] = touch_area[0] * mZoomValue - left_offest;
            // touch_area[1] = touch_area[1] * mZoomValue - top_offest;
            touch_area[0] = touch_area[0] *
                            ((float)mSwMaxWidth / (float)mWideMaxWidth) * 0.6;
            touch_area[1] = touch_area[1] *
                            ((float)mSwMaxHeight / (float)mWideMaxHeight) * 0.6;
            touch_area[2] = touch_area[0] + left_dst;
            touch_area[3] = touch_area[1] + top_dst;
            metaSettingsSw->update(ANDROID_SPRD_TOUCH_INFO, touch_area,
                                   ARRAY_SIZE(touch_area));
            if (metaSettingsTele->exists(ANDROID_CONTROL_AF_TRIGGER)) {
                metaSettingsTele->update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
            }
            if (metaSettingsWide->exists(ANDROID_CONTROL_AF_TRIGGER)) {
                metaSettingsWide->update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
            }
            HAL_LOGD("sw touch_area = %d, %d, %d, %d, %d",
                touch_area[0], touch_area[1], touch_area[2],
                touch_area[3], touch_area[4]);
        } else if (mZoomValue >= mSwitch_W_T_Threshold) { // tele
            float left_offest = 0, top_offest = 0;
            left_offest =
                ((float)mWideMaxWidth / 0.6 - (float)mTeleMaxWidth) / 2;
            top_offest =
                ((float)mWideMaxHeight / 0.6 - (float)mTeleMaxHeight) / 2;
            touch_area[0] = touch_area[0] - left_offest;
            touch_area[1] = touch_area[1] - top_offest;
            touch_area[2] = touch_area[2] - left_offest;
            touch_area[3] = touch_area[3] - top_offest;
            metaSettingsTele->update(ANDROID_SPRD_TOUCH_INFO, touch_area,
                                     ARRAY_SIZE(touch_area));
            if (metaSettingsSw->exists(ANDROID_CONTROL_AF_TRIGGER)) {
                metaSettingsSw->update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
            }
            if (metaSettingsWide->exists(ANDROID_CONTROL_AF_TRIGGER)) {
                metaSettingsWide->update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
            }
            HAL_LOGD("tele touch_area = %d, %d, %d, %d, %d",
                touch_area[0], touch_area[1], touch_area[2],
                touch_area[3], touch_area[4]);
        } else { // wide
            float left_offest = 0, top_offest = 0;
            left_offest =
                ((float)mWideMaxWidth / 0.6 - (float)mWideMaxWidth) / 2;
            top_offest =
                ((float)mWideMaxHeight / 0.6 - (float)mWideMaxHeight) / 2;
            touch_area[0] = touch_area[0] - left_offest;
            touch_area[1] = touch_area[1] - top_offest;
            touch_area[2] = touch_area[2] - left_offest;
            touch_area[3] = touch_area[3] - top_offest;
            metaSettingsWide->update(ANDROID_SPRD_TOUCH_INFO, touch_area,
                                     ARRAY_SIZE(touch_area));
            if (metaSettingsTele->exists(ANDROID_CONTROL_AF_TRIGGER)) {
                metaSettingsTele->update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
            }
            if (metaSettingsSw->exists(ANDROID_CONTROL_AF_TRIGGER)) {
                metaSettingsSw->update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
            }
            HAL_LOGD("wide touch_area = %d, %d, %d, %d, %d",
                touch_area[0], touch_area[1], touch_area[2],
                touch_area[3], touch_area[4]);
        }

        if (meta->exists(ANDROID_CONTROL_AF_REGIONS)) {
            af_area[0] = meta->find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
            af_area[1] = meta->find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
            af_area[2] = meta->find(ANDROID_CONTROL_AF_REGIONS).data.i32[2];
            af_area[3] = meta->find(ANDROID_CONTROL_AF_REGIONS).data.i32[3];
            af_area[4] = meta->find(ANDROID_CONTROL_AF_REGIONS).data.i32[4];
            HAL_LOGD("app af_area = %d, %d, %d, %d, %d",
                af_area[0], af_area[1], af_area[2], af_area[3], af_area[4]);
        }

        if (mZoomValue < mSwitch_W_Sw_Threshold) { // super wide
            float left_offest = 0, top_offest = 0;
            float left_dst = 0, top_dst = 0;
            left_offest = ((float)mWideMaxWidth - (float)mSwMaxWidth) / 2;
            top_offest = ((float)mWideMaxHeight - (float)mSwMaxHeight) / 2;
            left_dst = af_area[2] - af_area[0];
            top_dst = af_area[3] - af_area[1];
            // af_area[0] = af_area[0] * mZoomValue - left_offest;
            // af_area[1] = af_area[1] * mZoomValue - top_offest;
            af_area[0] =
                af_area[0] * ((float)mSwMaxWidth / (float)mWideMaxWidth) * 0.6;
            af_area[1] = af_area[1] *
                         ((float)mSwMaxHeight / (float)mWideMaxHeight) * 0.6;
            af_area[2] = af_area[0] + left_dst;
            af_area[3] = af_area[1] + top_dst;
            metaSettingsSw->update(ANDROID_CONTROL_AF_REGIONS, af_area,
                                   ARRAY_SIZE(af_area));
            HAL_LOGD("sw af_area = %d, %d, %d, %d, %d",
                af_area[0], af_area[1], af_area[2], af_area[3], af_area[4]);
        } else if (mZoomValue >= mSwitch_W_T_Threshold) { // tele
            float left_offest = 0, top_offest = 0;
            left_offest =
                ((float)mWideMaxWidth / 0.6 - (float)mTeleMaxWidth) / 2;
            top_offest =
                ((float)mWideMaxHeight / 0.6 - (float)mTeleMaxHeight) / 2;
            af_area[0] = af_area[0] - left_offest;
            af_area[1] = af_area[1] - top_offest + 1;
            af_area[2] = af_area[2] - left_offest;
            af_area[3] = af_area[3] - top_offest + 1;
            metaSettingsTele->update(ANDROID_CONTROL_AF_REGIONS, af_area,
                                     ARRAY_SIZE(af_area));
            HAL_LOGD("tele af_area = %d, %d, %d, %d, %d",
                af_area[0], af_area[1], af_area[2], af_area[3], af_area[4]);
        } else { // wide
            float left_offest = 0, top_offest = 0;
            left_offest =
                ((float)mWideMaxWidth / 0.6 - (float)mWideMaxWidth) / 2;
            top_offest =
                ((float)mWideMaxHeight / 0.6 - (float)mWideMaxHeight) / 2;
            af_area[0] = af_area[0] - left_offest;
            af_area[1] = af_area[1] - top_offest + 1;
            af_area[2] = af_area[2] - left_offest;
            af_area[3] = af_area[3] - top_offest + 1;
            metaSettingsWide->update(ANDROID_CONTROL_AF_REGIONS, af_area,
                                     ARRAY_SIZE(af_area));
            HAL_LOGD("wide af_area = %d, %d, %d, %d, %d",
                af_area[0], af_area[1], af_area[2], af_area[3], af_area[4]);
        }

        if (meta->exists(ANDROID_CONTROL_AE_REGIONS)) {
            ae_area[0] = meta->find(ANDROID_CONTROL_AE_REGIONS).data.i32[0];
            ae_area[1] = meta->find(ANDROID_CONTROL_AE_REGIONS).data.i32[1];
            ae_area[2] = meta->find(ANDROID_CONTROL_AE_REGIONS).data.i32[2];
            ae_area[3] = meta->find(ANDROID_CONTROL_AE_REGIONS).data.i32[3];
            ae_area[4] = meta->find(ANDROID_CONTROL_AE_REGIONS).data.i32[4];
            HAL_LOGD("app ae_area = %d, %d, %d, %d, %d",
                ae_area[0], ae_area[1], ae_area[2], ae_area[3], ae_area[4]);
        }

        if (mZoomValue < mSwitch_W_Sw_Threshold) { // super wide
            float left_offest = 0, top_offest = 0;
            float left_dst = 0, top_dst = 0;
            left_offest = ((float)mWideMaxWidth - (float)mSwMaxWidth) / 2;
            top_offest = ((float)mWideMaxHeight - (float)mSwMaxHeight) / 2;
            left_dst = ae_area[2] - ae_area[0];
            top_dst = ae_area[3] - ae_area[1];
            // ae_area[0] = ae_area[0] * mZoomValue - left_offest;
            // ae_area[1] = ae_area[1] * mZoomValue - top_offest;
            ae_area[0] =
                ae_area[0] * ((float)mSwMaxWidth / (float)mWideMaxWidth) * 0.6;
            ae_area[1] = ae_area[1] *
                         ((float)mSwMaxHeight / (float)mWideMaxHeight) * 0.6;
            ae_area[2] = ae_area[0] + left_dst;
            ae_area[3] = ae_area[1] + top_dst;
            metaSettingsSw->update(ANDROID_CONTROL_AE_REGIONS, ae_area,
                                   ARRAY_SIZE(ae_area));
            HAL_LOGD("sw ae_area = %d, %d, %d, %d, %d",
                ae_area[0], ae_area[1], ae_area[2], ae_area[3], ae_area[4]);
        } else if (mZoomValue >= mSwitch_W_T_Threshold) { // tele
            float left_offest = 0, top_offest = 0;
            left_offest =
                ((float)mWideMaxWidth / 0.6 - (float)mTeleMaxWidth) / 2;
            top_offest =
                ((float)mWideMaxHeight / 0.6 - (float)mTeleMaxHeight) / 2;
            ae_area[0] = ae_area[0] - left_offest;
            ae_area[1] = ae_area[1] - top_offest + 1;
            ae_area[2] = ae_area[2] - left_offest;
            ae_area[3] = ae_area[3] - top_offest + 1;
            metaSettingsTele->update(ANDROID_CONTROL_AE_REGIONS, ae_area,
                                     ARRAY_SIZE(ae_area));
            HAL_LOGD("tele ae_area = %d, %d, %d, %d, %d",
                ae_area[0], ae_area[1], ae_area[2], ae_area[3], ae_area[4]);
        } else { // wide
            float left_offest = 0, top_offest = 0;
            left_offest =
                ((float)mWideMaxWidth / 0.6 - (float)mWideMaxWidth) / 2;
            top_offest =
                ((float)mWideMaxHeight / 0.6 - (float)mWideMaxHeight) / 2;
            ae_area[0] = ae_area[0] - left_offest;
            ae_area[1] = ae_area[1] - top_offest + 1;
            ae_area[2] = ae_area[2] - left_offest;
            ae_area[3] = ae_area[3] - top_offest + 1;
            metaSettingsWide->update(ANDROID_CONTROL_AE_REGIONS, ae_area,
                                     ARRAY_SIZE(ae_area));
            HAL_LOGD("wide ae_area = %d, %d, %d, %d, %d",
                ae_area[0], ae_area[1], ae_area[2], ae_area[3], ae_area[4]);
        }
    }

    if (mZoomValue < mSwitch_W_T_Threshold &&
        mZoomValue >= mSwitch_W_Sw_Threshold) {
        mReqConfigNum = 0;
    } else {
        mReqConfigNum = 1;
    }
    HAL_LOGV("switchRequest = %d ,exit frame_number %u",
        mReqConfigNum, request->frame_number);
}

/*===========================================================================
 * FUNCTION     : reConfigInit
 *
 * DESCRIPTION : set init
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3MultiCamera::reConfigInit() {
    mIsCapturing = true;
    mCapInputbuffer = NULL;
    mPrevAlgoHandle = NULL;
    mCapAlgoHandle = NULL;
    mOtpData.otp_size = 0;
    mOtpData.otp_type = 0;
    mOtpData.otp_exist = false;
    memset(&mTWCaptureThread->mSavedOneResultBuff, 0,
           sizeof(camera3_stream_buffer_t));
}

/*===========================================================================
 * FUNCTION     : reConfigFlush
 *
 * DESCRIPTION : set flush
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3MultiCamera::reConfigFlush() {
    HAL_LOGD("E");

    TWThreadExit();
    mIsVideoMode = false;
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux1.clear();
    mUnmatchedFrameListAux2.clear();
    mUnmatchedFrameListVideoMain.clear();
    mUnmatchedFrameListVideoAux1.clear();
    mUnmatchedFrameListVideoAux2.clear();

    // gMultiCam->deinitAlgo();
    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION     : TWThreadExit
 *
 * DESCRIPTION :
 *
 * PARAMETERS : none
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::TWThreadExit() {
    HAL_LOGI("E");

    if (mPreviewMuxerThread != NULL) {
        if (mPreviewMuxerThread->isRunning()) {
            mPreviewMuxerThread->requestExit();
        }
    }
    if (mTWCaptureThread != NULL) {
        if (mTWCaptureThread->isRunning()) {
            mTWCaptureThread->requestExit();
        }
    }
    if (mVideoThread != NULL) {
        if (mVideoThread->isRunning()) {
            mVideoThread->requestExit();
        }
    }
    // wait threads quit to relese object
    mTWCaptureThread->join();
    mPreviewMuxerThread->join();
    mVideoThread->join();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : reConfigStream
 *
 * DESCRIPTION :
 *
 * PARAMETERS : none
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::reConfigStream() {
    int rc = NO_ERROR;
#define READ_OTP_SIZE 700
    FILE *fid = fopen("/data/vendor/cameraserver/wt_otp.bin", "rb");
    if (fid != NULL) {
        HAL_LOGD("open depth_config_parameter.bin file success");
        rc = fread(mOtpData.otp_data, sizeof(uint8_t), READ_OTP_SIZE, fid);
        fclose(fid);
        mOtpData.otp_size = rc;
        mOtpData.otp_exist = true;
        HAL_LOGD("dualotp otp_size=%d", mOtpData.otp_size);
    }
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.dump.calibration.data", prop, "0");
    if (atoi(prop) == 1) {
        for (int i = 0; i < mOtpData.otp_size; i++)
            HAL_LOGD("calibraion data [%d] = %d", i, mOtpData.otp_data[i]);
    }
    gMultiCam->mPreviewMuxerThread->run(String8::format("TW_Preview").string());
    gMultiCam->mTWCaptureThread->run(String8::format("TW_Capture").string());
    if (mIsVideoMode == true) {
        gMultiCam->mVideoThread->run(String8::format("TW_Video").string());
    }
    // initAlgo();
}

/*===========================================================================
 * FUNCTION     : reConfigClose
 *
 * DESCRIPTION :
 *
 * PARAMETERS : none
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::reConfigClose() {
    HAL_LOGD("E");
    if (!mFlushing) {
        mFlushing = true;
        TWThreadExit();
        mUnmatchedFrameListMain.clear();
        mUnmatchedFrameListAux1.clear();
        mUnmatchedFrameListAux2.clear();
        mUnmatchedFrameListVideoMain.clear();
        mUnmatchedFrameListVideoAux1.clear();
        mUnmatchedFrameListVideoAux2.clear();
        // gMultiCam->deinitAlgo();
    }
    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION     :TWPreviewMuxerThread
 *
 * DESCRIPTION : constructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3MultiCamera::TWPreviewMuxerThread::TWPreviewMuxerThread() {
    HAL_LOGI("E");
    mPreviewMuxerMsgList.clear();
    // mMsgList.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : ~TWPreviewMuxerThread
 *
 * DESCRIPTION : deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3MultiCamera::TWPreviewMuxerThread::~TWPreviewMuxerThread() {
    HAL_LOGI(" E");
    mPreviewMuxerMsgList.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : TWPreviewMuxerThread threadLoop
 *
 * DESCRIPTION : TWPreviewMuxerThread threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3MultiCamera::TWPreviewMuxerThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    void *output_buf_addr = NULL;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    void *input_buf3_addr = NULL;
    muxer_queue_msg_t muxer_msg;
    uint32_t frame_number = 0;
    int rc;

    waitMsgAvailable();
    // while (!mPreviewMuxerMsgList.empty()) {
    List<muxer_queue_msg_t>::iterator it;
    {
        Mutex::Autolock l(mMergequeueMutex);
        it = mPreviewMuxerMsgList.begin();
        muxer_msg = *it;
        mPreviewMuxerMsgList.erase(it);
    }
    switch (muxer_msg.msg_type) {
    case MUXER_MSG_EXIT: {
        List<multi_request_saved_t>::iterator itor =
            gMultiCam->mSavedPrevRequestList.begin();
        HAL_LOGD("exit frame_number %u", itor->frame_number);
        while (itor != gMultiCam->mSavedPrevRequestList.end()) {
            frame_number = itor->frame_number;
            itor++;
            gMultiCam->CallBackResult(frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                CALLBACK_STREAM, CAM_TYPE_MAIN);
        }
        return false;
    } break;
    case MUXER_MSG_DATA_PROC: {
        int IsNeedreProcess;
        camera3_stream_t *preview_stream = NULL;
        {
            Mutex::Autolock l(gMultiCam->mRequestLock);
            List<multi_request_saved_t>::iterator itor =
                gMultiCam->mSavedPrevRequestList.begin();
            while (itor != gMultiCam->mSavedPrevRequestList.end()) {
                if (itor->frame_number == muxer_msg.combo_frame.frame_number) {
                    output_buffer = itor->buffer;
                    preview_stream = itor->preview_stream;
                    frame_number = muxer_msg.combo_frame.frame_number;
                    break;
                }
                itor++;
            }
        }

        if (output_buffer != NULL) {
            HAL_LOGV("preview thread gMultiCam->mZoomValue %f",
                gMultiCam->mZoomValue);
            {
                char prop2[PROPERTY_VALUE_MAX] = {
                    0,
                };
                property_get("persist.vendor.cam.twv1.zoomratio", prop2, "0");
#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT
                if (gMultiCam->mZoomValue >=
                        gMultiCam->mSwitch_W_Sw_Threshold) { // wide
                    rc = gMultiCam->map(output_buffer, &output_buf_addr);
                    rc = gMultiCam->map(muxer_msg.combo_frame.buffer1,
                        &input_buf1_addr);
                    memcpy(output_buf_addr, input_buf1_addr,
                        ADP_BUFSIZE(*output_buffer));
                    gMultiCam->unmap(muxer_msg.combo_frame.buffer1);
                    gMultiCam->unmap(output_buffer);
                } else { // super wide
                    rc = gMultiCam->map(output_buffer, &output_buf_addr);
                    rc = gMultiCam->map(muxer_msg.combo_frame.buffer2,
                        &input_buf2_addr);
                    memcpy(output_buf_addr, input_buf2_addr,
                        ADP_BUFSIZE(*output_buffer));
                    gMultiCam->unmap(muxer_msg.combo_frame.buffer2);
                    gMultiCam->unmap(output_buffer);
                }

#else
                if (gMultiCam->mZoomValue >=
                        gMultiCam->mSwitch_W_Sw_Threshold &&
                    gMultiCam->mZoomValue <
                        gMultiCam->mSwitch_W_T_Threshold) { // wide
                    rc = gMultiCam->map(output_buffer, &output_buf_addr);
                    rc = gMultiCam->map(muxer_msg.combo_frame.buffer1,
                        &input_buf1_addr);
                    memcpy(output_buf_addr, input_buf1_addr,
                        ADP_BUFSIZE(*output_buffer));
                    gMultiCam->unmap(muxer_msg.combo_frame.buffer1);
                    gMultiCam->unmap(output_buffer);
                } else if (gMultiCam->mZoomValue <
                        gMultiCam->mSwitch_W_Sw_Threshold) { // super wide
                    rc = gMultiCam->map(output_buffer, &output_buf_addr);
                    rc = gMultiCam->map(muxer_msg.combo_frame.buffer2,
                        &input_buf2_addr);
                    memcpy(output_buf_addr, input_buf2_addr,
                        ADP_BUFSIZE(*output_buffer));
                    gMultiCam->unmap(muxer_msg.combo_frame.buffer2);
                    gMultiCam->unmap(output_buffer);
                } else if (gMultiCam->mZoomValue >=
                        gMultiCam->mSwitch_W_T_Threshold &&
                        gMultiCam->mZoomValue <= 8.0) { // tele
                    rc = gMultiCam->map(output_buffer, &output_buf_addr);
                    rc = gMultiCam->map(muxer_msg.combo_frame.buffer3,
                        &input_buf3_addr);
                    memcpy(output_buf_addr, input_buf3_addr,
                        ADP_BUFSIZE(*output_buffer));
                    gMultiCam->unmap(muxer_msg.combo_frame.buffer3);
                    gMultiCam->unmap(output_buffer);
                }
#endif
                // dump prev data
                {
                    char prop[PROPERTY_VALUE_MAX] = {
                        0,
                    };
                    property_get("persist.vendor.cam.multi.dump", prop, "0");
                    if (!strcmp(prop, "pre_in") || !strcmp(prop, "all")) {
                        rc = gMultiCam->map(muxer_msg.combo_frame.buffer1,
                            &input_buf1_addr);
                        rc = gMultiCam->map(muxer_msg.combo_frame.buffer2,
                            &input_buf2_addr);
                        rc = gMultiCam->map(muxer_msg.combo_frame.buffer3,
                            &input_buf3_addr);
                        char tmp_str[64] = {0};
                        sprintf(tmp_str, "_zoom=%f", gMultiCam->mZoomValue);
                        char MainPrev[80] = "MainPrev";
                        char Sub1Prev[80] = "Sub1Prev";
                        char Sub2Prev[80] = "Sub2Prev";
                        strcat(MainPrev, tmp_str);
                        strcat(Sub1Prev, tmp_str);
                        strcat(Sub2Prev, tmp_str);
                        // input_buf1 or left image
                        gMultiCam->dumpData((unsigned char *)input_buf1_addr, 1,
                            ADP_BUFSIZE(*output_buffer),
                            gMultiCam->mMainSize.preview_w,
                            gMultiCam->mMainSize.preview_h,
                            frame_number, MainPrev);
                        // input_buf2 or right image
                        gMultiCam->dumpData((unsigned char *)input_buf2_addr, 1,
                            ADP_BUFSIZE(*output_buffer),
                            gMultiCam->mMainSize.preview_w,
                            gMultiCam->mMainSize.preview_h,
                            frame_number, Sub1Prev);
                        gMultiCam->dumpData((unsigned char *)input_buf3_addr, 1,
                            ADP_BUFSIZE(*output_buffer),
                            gMultiCam->mMainSize.preview_w,
                            gMultiCam->mMainSize.preview_h,
                            frame_number, Sub2Prev);
                        gMultiCam->unmap(muxer_msg.combo_frame.buffer1);
                        gMultiCam->unmap(muxer_msg.combo_frame.buffer2);
                        gMultiCam->unmap(muxer_msg.combo_frame.buffer3);
                    }
                    if (!strcmp(prop, "pre_out") || !strcmp(prop, "all")) {
                        rc = gMultiCam->map(output_buffer, &output_buf_addr);
                        char tmp_str[64] = {0};
                        sprintf(tmp_str, "_zoom=%f", gMultiCam->mZoomValue);
                        char MultiPrev[80] = "MultiPrev";
                        strcat(MultiPrev, tmp_str);
                        // output_buffer
                        gMultiCam->dumpData((unsigned char *)output_buf_addr, 1,
                            ADP_BUFSIZE(*output_buffer), preview_stream->width,
                            preview_stream->height, frame_number, MultiPrev);
                        gMultiCam->unmap(output_buffer);
                    }
                }
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, muxer_msg.combo_frame.buffer1,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, muxer_msg.combo_frame.buffer2,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
#ifndef CONFIG_WIDE_ULTRAWIDE_SUPPORT
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, muxer_msg.combo_frame.buffer3,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
#endif
                gMultiCam->CallBackResult(muxer_msg.combo_frame.frame_number,
                    CAMERA3_BUFFER_STATUS_OK,
                    CALLBACK_STREAM, CAM_TYPE_MAIN);
            }
        }
    } break;
    default:
        break;
    }
//};

    return true;
}

/*===========================================================================
 * FUNCTION     : requestExit
 *
 * DESCRIPTION : request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3MultiCamera::TWPreviewMuxerThread::requestExit() {

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mPreviewMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION     : waitMsgAvailable
 *
 * DESCRIPTION : wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::TWPreviewMuxerThread::waitMsgAvailable() {
    while (mPreviewMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION     : TWCaptureThread
 *
 * DESCRIPTION : constructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3MultiCamera::TWCaptureThread::TWCaptureThread() {
    HAL_LOGI("E");
    mSavedOneResultBuff = NULL;

    mCaptureMsgList.clear();
    // mMsgList.clear();
    memset(&mSavedOneResultBuff, 0, sizeof(camera3_stream_buffer_t));
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : ~TWCaptureThread
 *
 * DESCRIPTION : deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3MultiCamera::TWCaptureThread::~TWCaptureThread() {
    HAL_LOGI(" E");
    mCaptureMsgList.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : TWCaptureThread threadLoop
 *
 * DESCRIPTION : TWCaptureThread threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3MultiCamera::TWCaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    void *output_buf_addr = NULL;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    void *input_buf3_addr = NULL;
    uint32_t frame_number = 0;
    muxer_queue_msg_t capture_msg;
    int rc = 0;

    waitMsgAvailable();
    // while (!mCaptureMsgList.empty()) {
    List<muxer_queue_msg_t>::iterator itor1;
    {
        Mutex::Autolock l(mMergequeueMutex);
        itor1 = mCaptureMsgList.begin();
        capture_msg = *itor1;
        mCaptureMsgList.erase(itor1);
    }
    switch (capture_msg.msg_type) {
    case MUXER_MSG_EXIT: {
        // flush queue
        HAL_LOGD("TW_MSG_EXIT,mCapFrameNum=%lld", gMultiCam->mCapFrameNum);
        if ((gMultiCam->mCapFrameNum) > 0) {
            HAL_LOGE("TW_MSG_EXIT.HAL don't send capture frame");
            gMultiCam->CallBackResult(gMultiCam->mCapFrameNum,
                CAMERA3_BUFFER_STATUS_ERROR,
                SNAPSHOT_STREAM, CAM_TYPE_MAIN);
        }
        memset(&(gMultiCam->mSavedSnapRequest), 0,
            sizeof(multi_request_saved_t));
        memset(&mSavedOneResultBuff, 0, sizeof(camera3_stream_buffer_t));
        return false;
    }
    case MUXER_MSG_DATA_PROC: {
        HAL_LOGD("capture thread mZoomValue %f", gMultiCam->mZoomValue);

        if (mSavedOneResultBuff) {
            memset(&mSavedOneResultBuff, 0, sizeof(camera3_stream_buffer_t));
        }
        output_buffer = (gMultiCam->popBufferList(
            gMultiCam->mLocalBufferList,
            gMultiCam->mSavedSnapRequest.snap_stream->width,
            gMultiCam->mSavedSnapRequest.snap_stream->height));
#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT
        if (gMultiCam->mZoomValue >=
            gMultiCam->mSwitch_W_Sw_Threshold) { // wide
            rc = gMultiCam->map(capture_msg.combo_frame.buffer1,
                &input_buf1_addr);
            rc = gMultiCam->map(output_buffer, &output_buf_addr);
            memcpy(output_buf_addr, input_buf1_addr,
                ADP_BUFSIZE(*output_buffer));
            gMultiCam->unmap(capture_msg.combo_frame.buffer1);
            gMultiCam->unmap(output_buffer);
            gMultiCam->pushBufferList(
                gMultiCam->mLocalBuffer, capture_msg.combo_frame.buffer1,
                gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
        } else { // super wide
            rc = gMultiCam->map(capture_msg.combo_frame.buffer2,
                &input_buf2_addr);
            rc = gMultiCam->map(output_buffer, &output_buf_addr);
            memcpy(output_buf_addr, input_buf2_addr,
                ADP_BUFSIZE(*output_buffer));
            gMultiCam->unmap(capture_msg.combo_frame.buffer2);
            gMultiCam->unmap(output_buffer);
            gMultiCam->pushBufferList(
                gMultiCam->mLocalBuffer, capture_msg.combo_frame.buffer2,
                gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
        }
#else
        if (gMultiCam->mZoomValue >= gMultiCam->mSwitch_W_Sw_Threshold &&
            gMultiCam->mZoomValue < gMultiCam->mSwitch_W_T_Threshold) { // wide
            rc = gMultiCam->map(capture_msg.combo_frame.buffer1,
                &input_buf1_addr);
            rc = gMultiCam->map(output_buffer, &output_buf_addr);
            memcpy(output_buf_addr, input_buf1_addr,
                ADP_BUFSIZE(*output_buffer));
            gMultiCam->unmap(capture_msg.combo_frame.buffer1);
            gMultiCam->unmap(output_buffer);
            gMultiCam->pushBufferList(
                gMultiCam->mLocalBuffer, capture_msg.combo_frame.buffer1,
                gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
        } else if (gMultiCam->mZoomValue <
                gMultiCam->mSwitch_W_Sw_Threshold) { // super wide
            rc = gMultiCam->map(capture_msg.combo_frame.buffer2,
                &input_buf2_addr);
            rc = gMultiCam->map(output_buffer, &output_buf_addr);
            memcpy(output_buf_addr, input_buf2_addr,
                ADP_BUFSIZE(*output_buffer));
            gMultiCam->unmap(capture_msg.combo_frame.buffer2);
            gMultiCam->unmap(output_buffer);
            gMultiCam->pushBufferList(
                gMultiCam->mLocalBuffer, capture_msg.combo_frame.buffer2,
                gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
        } else if (gMultiCam->mZoomValue >= gMultiCam->mSwitch_W_T_Threshold &&
                gMultiCam->mZoomValue <= 8.0) { // tele
            rc = gMultiCam->map(capture_msg.combo_frame.buffer3,
                &input_buf3_addr);
            rc = gMultiCam->map(output_buffer, &output_buf_addr);
            memcpy(output_buf_addr, input_buf3_addr,
                ADP_BUFSIZE(*output_buffer));
            gMultiCam->unmap(capture_msg.combo_frame.buffer3);
            gMultiCam->unmap(output_buffer);
            gMultiCam->pushBufferList(
                gMultiCam->mLocalBuffer, capture_msg.combo_frame.buffer3,
                gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
        }
#endif
        // dump capture data
        {
            char prop[PROPERTY_VALUE_MAX] = {
                0,
            };

            property_get("persist.vendor.cam.multi.dump", prop, "0");
            if (!strcmp(prop, "capyuv") || !strcmp(prop, "all")) {
                rc = gMultiCam->map(output_buffer, &output_buf_addr);
                // input_buf1 or left image
                char tmp_str[64] = {0};
                sprintf(tmp_str, "_zoom=%f", gMultiCam->mZoomValue);
                char Capture[80] = "MultiCapture";
                strcat(Capture, tmp_str);
                // output_buffer
                gMultiCam->dumpData(
                    (unsigned char *)output_buf_addr, 1,
                    ADP_BUFSIZE(*output_buffer),
                    gMultiCam->mSavedSnapRequest.snap_stream->width,
                    gMultiCam->mSavedSnapRequest.snap_stream->height,
                    frame_number, Capture);
                gMultiCam->unmap(output_buffer);
            }
        }
        gMultiCam->mCapInputbuffer = output_buffer;
        gMultiCam->reprocessReq(output_buffer);
        break;
    }
    default:
        break;
    };
    //}
    return true;
}

/*===========================================================================
 * FUNCTION     : requestExit
 *
 * DESCRIPTION : request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     :None
 *==========================================================================*/
void SprdCamera3MultiCamera::TWCaptureThread::requestExit() {

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t capture_msg;
    capture_msg.msg_type = MUXER_MSG_EXIT;
    mCaptureMsgList.push_back(capture_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION     : waitMsgAvailable
 *
 * DESCRIPTION : wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::TWCaptureThread::waitMsgAvailable() {
    while (mCaptureMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}

 /*===========================================================================
 * FUNCTION     : VideoThread
 *
 * DESCRIPTION : deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3MultiCamera::VideoThread::VideoThread() {
    HAL_LOGI("E");
    mVideoMuxerMsgList.clear();
    // mMsgList.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : ~VideoThread
 *
 * DESCRIPTION : deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3MultiCamera::VideoThread::~VideoThread() {
    HAL_LOGI(" E");
    mVideoMuxerMsgList.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION     : threadLoop
 *
 * DESCRIPTION : threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3MultiCamera::VideoThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    void *output_buf_addr = NULL;
    void *output_buf_addr_aux1 = NULL;
    void *output_buf_addr_aux2 = NULL;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    void *input_buf3_addr = NULL;
    muxer_queue_msg_t muxer_msg;
    uint32_t frame_number = 0;
    int rc;

    waitMsgAvailable();
    //while (!mVideoMuxerMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mVideoMuxerMsgList.begin();
            muxer_msg = *it;
            mVideoMuxerMsgList.erase(it);
         }

        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT: {
            List<multi_request_saved_t>::iterator itor =
                gMultiCam->mSavedVideoRequestList.begin();
            HAL_LOGD("exit frame_number %u", itor->frame_number);
            while (itor != gMultiCam->mSavedVideoRequestList.end()) {
                frame_number = itor->frame_number;
                itor++;
                gMultiCam->CallBackResult(frame_number,
                                          CAMERA3_BUFFER_STATUS_ERROR,
                                          VIDEO_STREAM, CAM_TYPE_MAIN);
             }
            return false;
        } break;
        case MUXER_MSG_DATA_PROC: {
            int IsNeedreProcess;
            camera3_stream_t *video_stream = NULL;
             {
                Mutex::Autolock l(gMultiCam->mRequestLock);
                List<multi_request_saved_t>::iterator itor =
                    gMultiCam->mSavedVideoRequestList.begin();
                while (itor != gMultiCam->mSavedVideoRequestList.end()) {
                    if (itor->frame_number ==
                        muxer_msg.combo_frame.frame_number) {
                        output_buffer = itor->buffer;
                        video_stream = itor->video_stream;
                        frame_number = muxer_msg.combo_frame.frame_number;
                        break;
                     }
                    itor++;
                 }
             }

            if (output_buffer != NULL) {
                HAL_LOGD("video_threadloop gMultiCam->mZoomValue = %f",
                    gMultiCam->mZoomValue);
                {
                    if (gMultiCam->mZoomValue >= gMultiCam->mSwitch_W_Sw_Threshold &&
                        gMultiCam->mZoomValue < gMultiCam->mSwitch_W_T_Threshold) {// wide
                        rc = gMultiCam->map(output_buffer, &output_buf_addr);
                        rc = gMultiCam->map(muxer_msg.combo_frame.buffer1,
                            &input_buf1_addr);
                        memcpy(output_buf_addr, input_buf1_addr,
                            ADP_BUFSIZE(*output_buffer));
                        gMultiCam->unmap(muxer_msg.combo_frame.buffer1);
                        gMultiCam->unmap(output_buffer);
                    } else if (gMultiCam->mZoomValue < gMultiCam->mSwitch_W_Sw_Threshold) {// super wide
                        rc = gMultiCam->map(output_buffer, &output_buf_addr);
                        rc = gMultiCam->map(muxer_msg.combo_frame.buffer2,
                            &input_buf2_addr);
                        memcpy(output_buf_addr, input_buf2_addr,
                            ADP_BUFSIZE(*output_buffer));
                        gMultiCam->unmap(muxer_msg.combo_frame.buffer2);
                        gMultiCam->unmap(output_buffer);
                    } else if (gMultiCam->mZoomValue >= gMultiCam->mSwitch_W_T_Threshold &&
                            gMultiCam->mZoomValue <= 8.0) {// tele
                        rc = gMultiCam->map(output_buffer, &output_buf_addr);
                        rc = gMultiCam->map(muxer_msg.combo_frame.buffer3,
                            &input_buf3_addr);
                        memcpy(output_buf_addr, input_buf3_addr,
                            ADP_BUFSIZE(*output_buffer));
                        gMultiCam->unmap(muxer_msg.combo_frame.buffer3);
                        gMultiCam->unmap(output_buffer);
                    }
                    // dump prev data
                    {
                        char prop[PROPERTY_VALUE_MAX] = {
                            0,
                        };
                        property_get("persist.vendor.cam.twv1.dump", prop, "0");
                        if (!strcmp(prop, "videopreyuv") || !strcmp(prop, "all")) {
                            rc = gMultiCam->map(output_buffer, &output_buf_addr);
                            rc = gMultiCam->map(muxer_msg.combo_frame.buffer1,
                                &input_buf1_addr);
                            rc = gMultiCam->map(muxer_msg.combo_frame.buffer2,
                                &input_buf2_addr);
                            char tmp_str[64] = {0};
                            sprintf(tmp_str, "_zoom=%f", gMultiCam->mZoomValue);
                            char MainPrev[80] = "MainPrev";
                            char SubPrev[80] = "SubPrev";
                            char TWPrev[80] = "TWPrev";
                            strcat(MainPrev, tmp_str);
                            strcat(SubPrev, tmp_str);
                            strcat(TWPrev, tmp_str);
                            // input_buf1 or left image
                            gMultiCam->dumpData(
                                (unsigned char *)input_buf1_addr, 1,
                                ADP_BUFSIZE(*output_buffer),
                                gMultiCam->mWideMaxWidth,
                                gMultiCam->mWideMaxHeight, frame_number,
                                MainPrev);
                            // input_buf2 or right image
                            gMultiCam->dumpData(
                                (unsigned char *)input_buf2_addr, 1,
                                ADP_BUFSIZE(*output_buffer),
                                gMultiCam->mTeleMaxWidth,
                                gMultiCam->mTeleMaxHeight, frame_number,
                                SubPrev);
                            // output_buffer
                            gMultiCam->dumpData(
                                (unsigned char *)output_buf_addr, 1,
                                ADP_BUFSIZE(*output_buffer),
                                video_stream->width, video_stream->height,
                                frame_number, TWPrev);
                            gMultiCam->unmap(muxer_msg.combo_frame.buffer1);
                            gMultiCam->unmap(muxer_msg.combo_frame.buffer2);
                            gMultiCam->unmap(output_buffer);
                        }
                    }
                    gMultiCam->pushBufferList(gMultiCam->mLocalBuffer,
                        muxer_msg.combo_frame.buffer1,
                        gMultiCam->mLocalBufferNumber,
                        gMultiCam->mLocalBufferList);
                    gMultiCam->pushBufferList(gMultiCam->mLocalBuffer,
                        muxer_msg.combo_frame.buffer2,
                        gMultiCam->mLocalBufferNumber,
                        gMultiCam->mLocalBufferList);
                    gMultiCam->pushBufferList(gMultiCam->mLocalBuffer,
                        muxer_msg.combo_frame.buffer3,
                        gMultiCam->mLocalBufferNumber,
                        gMultiCam->mLocalBufferList);
                    gMultiCam->CallBackResult(
                        muxer_msg.combo_frame.frame_number,
                        CAMERA3_BUFFER_STATUS_OK,
                        VIDEO_STREAM, CAM_TYPE_MAIN);
                }
            }
        } break;
        default:
            break;
        }
    //};

    return true;
}

/*===========================================================================
 * FUNCTION     : requestExit
 *
 * DESCRIPTION : request video thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3MultiCamera::VideoThread::requestExit() {

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mVideoMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION     : waitMsgAvailable
 *
 * DESCRIPTION : wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::VideoThread::waitMsgAvailable() {
    while (mVideoMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION     : processCaptureResultMain
 *
 * DESCRIPTION : process frame of camera index 0
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::processCaptureResultMain(
    const camera3_capture_result_t *result) {

    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    meta_save_t metadata_t;
    int vcmSteps = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;

    if (result_buffer == NULL) {
        // main meta process
        metadata = result->result;
        if (metadata.exists(ANDROID_SPRD_VCM_STEP) & cur_frame_number) {
            vcmSteps = metadata.find(ANDROID_SPRD_VCM_STEP).data.i32[0];
            setAlgoTrigger(vcmSteps);
        }

        if (cur_frame_number == mCapFrameNum && cur_frame_number != 0) {
            if (gMultiCam->mRequstState == REPROCESS_STATE) {
                HAL_LOGD("main hold jpeg picture call back, framenumber:%d",
                    result->frame_number);
            } else {
                {
                    HAL_LOGD("main CallBackMetadata, framenumber:%d",
                        result->frame_number);
                    Mutex::Autolock(gMultiCam->mMetatLock);
                    metadata_t.frame_number = cur_frame_number;
                    metadata_t.metadata = clone_camera_metadata(result->result);
                    mMetadataList.push_back(metadata_t);
                }
                CallBackMetadata();
            }
            return;
        } else {
            HAL_LOGD("main send preview meta, frame number:%d", cur_frame_number);
            metadata_t.frame_number = cur_frame_number;
            metadata_t.metadata = clone_camera_metadata(result->result);
            Mutex::Autolock l(gMultiCam->mMetatLock);
            mMetadataList.push_back(metadata_t);
            return;
        }
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGD("main frame %d, stream type %d", cur_frame_number, currStreamType);
    /* Process error buffer for Main camera*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGD("main return local buffer:%d caused by error Buffer status",
            result->frame_number);
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                          mLocalBufferNumber, mLocalBufferList);
        if (currStreamType == DEFAULT_STREAM) {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                           SNAPSHOT_STREAM, CAM_TYPE_MAIN);
        } else {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                           currStreamType, CAM_TYPE_MAIN);
        }
        return;
    }
    if (currStreamType == DEFAULT_STREAM) {
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                           DEFAULT_STREAM, CAM_TYPE_MAIN);
            return;
        }
        Mutex::Autolock l(mDefaultStreamLock);
        muxer_queue_msg_t capture_msg;
        capture_msg.msg_type = MUXER_MSG_DATA_PROC;
        capture_msg.combo_frame.frame_number = result->frame_number;
        capture_msg.combo_frame.buffer1 = result->output_buffers->buffer;
        // capture_msg.combo_frame.buffer3 =
        // mTWCaptureThread->mSavedOneResultBuff;
        capture_msg.combo_frame.input_buffer = result->input_buffer;
        {
            Mutex::Autolock l(mTWCaptureThread->mMergequeueMutex);
            HAL_LOGV("main capture enqueue combo frame capture:%d for frame merge!",
                capture_msg.combo_frame.frame_number);
            mTWCaptureThread->mCaptureMsgList.push_back(capture_msg);
            mTWCaptureThread->mMergequeueSignal.signal();
        }
    } else if (currStreamType == SNAPSHOT_STREAM) {
        if (mIsVideoMode == true) {
            if ((gMultiCam->mZoomValue >=
                    gMultiCam->mSwitch_W_Sw_Threshold &&
                gMultiCam->mZoomValue <
                    gMultiCam->mSwitch_W_T_Threshold)) {
                void *output_buf_addr = NULL;
                void *input_buf_addr = NULL;
                int rc = 0;

                rc = gMultiCam->map((result->output_buffers)->buffer,
                    &input_buf_addr);
                rc = gMultiCam->map(mSavedSnapRequest.buffer,
                    &output_buf_addr);
                memcpy(output_buf_addr, input_buf_addr,
                    ADP_BUFSIZE(*mSavedSnapRequest.buffer));
                gMultiCam->unmap((result->output_buffers)->buffer);
                gMultiCam->unmap(mSavedSnapRequest.buffer);
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, (result->output_buffers)->buffer,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);

                CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                    currStreamType, CAM_TYPE_MAIN);
                // dump capture data
                {
                    char prop[PROPERTY_VALUE_MAX] = {0};
                    property_get("persist.vendor.cam.multi.dump.video.jpeg", prop, "0");
                    if (!strcmp(prop, "capjpeg") || !strcmp(prop, "all")) {
                        // input_buf1 or left image
                        char tmp_str[64] = {0};
                        sprintf(tmp_str, "_zoom=%f", gMultiCam->mZoomValue);
                        char Capture[80] = "MultiCapture";
                        strcat(Capture, tmp_str);
                        rc = gMultiCam->map((result->output_buffers)->buffer,
                            &input_buf_addr);
                        // output_buffer
                        gMultiCam->dumpData(
                            (unsigned char *)input_buf_addr, 2,
                            ADP_BUFSIZE(*(result->output_buffers->buffer)),
                            gMultiCam->mSavedSnapRequest.snap_stream->width,
                            gMultiCam->mSavedSnapRequest.snap_stream->height,
                            cur_frame_number, Capture);
                        gMultiCam->unmap((result->output_buffers)->buffer);
                    }
                }
            } else {
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, (result->output_buffers)->buffer,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
            }
        } else {
            if (mCapInputbuffer) {
                gMultiCam->pushBufferList(gMultiCam->mLocalBuffer, mCapInputbuffer,
                                      gMultiCam->mLocalBufferNumber,
                                      gMultiCam->mLocalBufferList);
                mCapInputbuffer = NULL;
            }
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                       currStreamType, CAM_TYPE_MAIN);
        }
    } else if (currStreamType == CALLBACK_STREAM) {
        {
            Mutex::Autolock l(mNotifyLockMain);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListMain.begin();
                 i != mNotifyListMain.end(); i++) {
                HAL_LOGD("mNotifyListMain_preview");
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        mNotifyListMain.erase(i);
                        HAL_LOGD("mNotifyListMain_NOTIFY_SUCCESS_preview");
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("main return local buffer:%d caused by error "
                            "Notify status", result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        CallBackResult(cur_frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR,
                                       currStreamType, CAM_TYPE_MAIN);
                        mNotifyListMain.erase(i);
                        return;
                    }
                }
            }
        }

        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("main found no corresponding notify");
            return;
        }

        hwi_frame_buffer_info_t cur_frame;
        hwi_frame_buffer_info_t matched_frame1;
        hwi_frame_buffer_info_t matched_frame2;
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame1, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame2, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListAux1,
                                               &matched_frame1)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = cur_frame.buffer;

                muxer_msg.combo_frame.buffer2 = matched_frame1.buffer;

                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("main preview combo frame:%d for frame merge!",
                        muxer_msg.combo_frame.frame_number);
                    if (cur_frame.frame_number > 5)
                        clearFrameNeverMatched(cur_frame.frame_number,
                                               matched_frame1.frame_number, 0);

                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            }
#else
            if (MATCH_SUCCESS ==
                matchThreeFrame(cur_frame, mUnmatchedFrameListAux1,
                                mUnmatchedFrameListAux2, &matched_frame1,
                                &matched_frame2)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = cur_frame.buffer;
                muxer_msg.combo_frame.buffer2 = matched_frame1.buffer;
                muxer_msg.combo_frame.buffer3 = matched_frame2.buffer;
                Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                HAL_LOGV("main preview enqueue combo frame:%d for frame merge!",
                    muxer_msg.combo_frame.frame_number);
                if (cur_frame.frame_number > 5) {
                    HAL_LOGV("main preview cur_frame.frame_number > 5");
                    clearFrameNeverMatched(cur_frame.frame_number,
                                           matched_frame1.frame_number,
                                           matched_frame2.frame_number);
                }
                mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(muxer_msg);
                mPreviewMuxerThread->mMergequeueSignal.signal();
            }
#endif
            else {
                HAL_LOGV("main preview enqueue newest unmatched frame:%d",
                    cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
                if (discard_frame != NULL) {
                    HAL_LOGV("main preview discard frame_number %u", cur_frame_number);
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    if (cur_frame.frame_number > 5)
                        CallBackResult(discard_frame->frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR,
                                       currStreamType, CAM_TYPE_MAIN);
                    delete discard_frame;
                }
            }
        }
    } else if (currStreamType == VIDEO_STREAM) {
        {
            Mutex::Autolock l(mNotifyLockVideoMain);
            for (List<camera3_notify_msg_t>::iterator i = mNotifyListVideoMain.begin();
                 i != mNotifyListVideoMain.end(); i++) {
                HAL_LOGD("mNotifyListVideoMain_video");
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        mNotifyListVideoMain.erase(i);
                        HAL_LOGD("mNotifyListVideoMain_NOTIFY_SUCCESS_video");
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("main return local buffer:%d caused by error "
                            "Notify status", result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        CallBackResult(cur_frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR,
                                       currStreamType, CAM_TYPE_MAIN);
                        mNotifyListVideoMain.erase(i);
                        return;
                    }
                }
            }
        }

        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("main found no corresponding notify");
            return;
        }

        hwi_frame_buffer_info_t cur_frame;
        hwi_frame_buffer_info_t matched_frame1;
        hwi_frame_buffer_info_t matched_frame2;
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame1, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame2, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;

        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS ==
                matchThreeFrame(cur_frame, mUnmatchedFrameListVideoAux1,
                                mUnmatchedFrameListVideoAux2, &matched_frame1,
                                &matched_frame2)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = cur_frame.buffer;
                muxer_msg.combo_frame.buffer2 = matched_frame1.buffer;
                muxer_msg.combo_frame.buffer3 = matched_frame2.buffer;
                Mutex::Autolock l(mVideoThread->mMergequeueMutex);
                HAL_LOGD("main video enqueue combo frame:%d for frame merge!",
                    muxer_msg.combo_frame.frame_number);
                if (cur_frame.frame_number > 5) {
                    HAL_LOGD("main video cur_frame.frame_number > 5");
                    clearFrameNeverMatched(cur_frame.frame_number,
                                           matched_frame1.frame_number,
                                           matched_frame2.frame_number);
                }
                mVideoThread->mVideoMuxerMsgList.push_back(muxer_msg);
                mVideoThread->mMergequeueSignal.signal();
            } else {
                HAL_LOGD("main video enqueue newest unmatched frame:%d",
                    cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListVideoMain);
                if (discard_frame != NULL) {
                    HAL_LOGD("main video discard frame_number %u", cur_frame_number);
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    if (cur_frame.frame_number > 5)
                        CallBackResult(discard_frame->frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR,
                                       currStreamType, CAM_TYPE_MAIN);
                    delete discard_frame;
                }
            }
        }

    }
    return;
}

/*===========================================================================
 * FUNCTION     : processCaptureResultAux1
 *
 * DESCRIPTION : process frame of camera index 1
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::processCaptureResultAux1(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    CameraMetadata metadata;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    metadata = result->result;
    uint64_t result_timestamp = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    meta_save_t metadata_t;
    camera_metadata_entry_t entry;
    float zoomWidth, zoomHeight;

    if (result_buffer == NULL) {
        // aux1 meta process
        if (metadata.exists(ANDROID_SPRD_AI_SCENE_TYPE_CURRENT)) {
            gMultiCam->aux1_ai_scene_type =
                metadata.find(ANDROID_SPRD_AI_SCENE_TYPE_CURRENT).data.u8[0];
        }
        if (metadata.exists(ANDROID_CONTROL_AF_STATE)) {
            gMultiCam->aux1_af_state=
                metadata.find(ANDROID_CONTROL_AF_STATE).data.u8[0];
        }
        entry = metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES);
        if (entry.count != 0) {
            gMultiCam->aux1_face_number = entry.count / 4;
        } else {
            gMultiCam->aux1_face_number = 0;
        }

        if (gMultiCam->aux1_face_number) {
            for (int i = 0; i < gMultiCam->aux1_face_number; i++) {
                // zoomWidth = (float)mWideMaxWidth / ((float)mSwMaxWidth);
                // zoomHeight = (float)mWideMaxHeight / ((float)mSwMaxHeight);
                zoomWidth = 1.0;
                zoomHeight = 1.0;
                float left_offest = 0, top_offest = 0;
                left_offest =
                    ((float)mWideMaxWidth / 0.6 - (float)mSwMaxWidth) / 2;
                top_offest =
                    ((float)mWideMaxHeight / 0.6 - (float)mSwMaxHeight) / 2;
                gMultiCam->aux1_FaceRect[i * 4 + 0] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 0] *
                        zoomWidth +
                    left_offest;
                gMultiCam->aux1_FaceRect[i * 4 + 1] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 1] *
                        zoomHeight +
                    top_offest;
                gMultiCam->aux1_FaceRect[i * 4 + 2] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 2] *
                        zoomWidth +
                    left_offest;
                gMultiCam->aux1_FaceRect[i * 4 + 3] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 3] *
                        zoomHeight +
                    top_offest;

                gMultiCam->aux1_FaceGenderRaceAge[i] =
                    metadata.find(ANDROID_SPRD_FACE_ATTRIBUTES).data.i32[i];

                HAL_LOGV("aux1_FaceRect =  %d, %d, %d, %d zoomWidth %f "
                    "zoomHeight %f gMultiCam->aux1_face_number %d",
                    gMultiCam->aux1_FaceRect[i * 4 + 0],
                    gMultiCam->aux1_FaceRect[i * 4 + 1],
                    gMultiCam->aux1_FaceRect[i * 4 + 2],
                    gMultiCam->aux1_FaceRect[i * 4 + 3], zoomWidth,
                    zoomHeight, gMultiCam->aux1_face_number);
            }
        }

        if (cur_frame_number == mCapFrameNum && cur_frame_number != 0) {
            if (gMultiCam->mRequstState == REPROCESS_STATE) {
                HAL_LOGD("aux1 hold jpeg picture call back, framenumber:%d",
                    result->frame_number);
            } else {
                if (mIsVideoMode == false) {
                    {
                        HAL_LOGD("aux1 CallBackMetadata, framenumber:%d",
                            result->frame_number);
                        Mutex::Autolock(gMultiCam->mMetatLock);
                        metadata_t.frame_number = cur_frame_number;
                        metadata_t.metadata = clone_camera_metadata(result->result);
                        mMetadataList.push_back(metadata_t);
                    }
                    CallBackMetadata();
                }
            }
                return;
        }

            return;
    }

    if (mFlushing) {
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                       mLocalBufferNumber, mLocalBufferList);
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGD("aux1 frame %d, stream type %d", cur_frame_number, currStreamType);

    /* Process error buffer for Aux1 camera*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGD("aux1 return local buffer:%d caused by error Buffer status",
            result->frame_number);
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
        if (currStreamType == DEFAULT_STREAM) {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                           SNAPSHOT_STREAM, CAM_TYPE_AUX1);
        }
        return;
    }
    if (currStreamType == DEFAULT_STREAM) {

        Mutex::Autolock l(mDefaultStreamLock);
        result_buffer = result->output_buffers;
        /*if (NULL == mTWCaptureThread->mSavedOneResultBuff) {
            mTWCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {*/
        muxer_queue_msg_t capture_msg;

        capture_msg.msg_type = MUXER_MSG_DATA_PROC;
        capture_msg.combo_frame.frame_number = result->frame_number;
        capture_msg.combo_frame.buffer2 = result->output_buffers->buffer;
        capture_msg.combo_frame.input_buffer = result->input_buffer;
        {
            Mutex::Autolock l(mTWCaptureThread->mMergequeueMutex);
            HAL_LOGD("aux1 capture enqueue combo frame:%d for frame merge!",
                capture_msg.combo_frame.frame_number);
            mTWCaptureThread->mCaptureMsgList.push_back(capture_msg);
            mTWCaptureThread->mMergequeueSignal.signal();
        }
        //}
    } else if (currStreamType == SNAPSHOT_STREAM) {
        if (mIsVideoMode == true) {
            if ((gMultiCam->mZoomValue <
                    gMultiCam->mSwitch_W_Sw_Threshold)) {
                void *output_buf_addr1 = NULL;
                void *input_buf_addr1 = NULL;
                int rc = 0;

                rc = gMultiCam->map((result->output_buffers)->buffer,
                    &input_buf_addr1);
                rc = gMultiCam->map(mSavedSnapRequest.buffer,
                    &output_buf_addr1);
                memcpy(output_buf_addr1, input_buf_addr1,
                    ADP_BUFSIZE(*mSavedSnapRequest.buffer));
                gMultiCam->unmap((result->output_buffers)->buffer);
                gMultiCam->unmap(mSavedSnapRequest.buffer);
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, (result->output_buffers)->buffer,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);

                CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                    currStreamType, CAM_TYPE_MAIN);

                // dump capture data
                {
                    char prop[PROPERTY_VALUE_MAX] = {0};
                    property_get("persist.vendor.cam.multi.dump.video.jpeg1", prop, "0");
                    if (!strcmp(prop, "capjpeg") || !strcmp(prop, "all")) {
                        // input_buf1 or left image
                        char tmp_str[64] = {0};
                        sprintf(tmp_str, "_zoom=%f", gMultiCam->mZoomValue);
                        char Capture[80] = "MultiCapture";
                        strcat(Capture, tmp_str);
                        rc = gMultiCam->map((result->output_buffers)->buffer,
                            &input_buf_addr1);
                        // output_buffer
                        gMultiCam->dumpData(
                            (unsigned char *)input_buf_addr1, 2,
                            ADP_BUFSIZE(*(result->output_buffers->buffer)),
                            gMultiCam->mSavedSnapRequest.snap_stream->width,
                            gMultiCam->mSavedSnapRequest.snap_stream->height,
                            cur_frame_number, Capture);
                        gMultiCam->unmap((result->output_buffers)->buffer);
                    }
                }
            } else {
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, (result->output_buffers)->buffer,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
            }
        } else {
            if (mCapInputbuffer) {
                gMultiCam->pushBufferList(gMultiCam->mLocalBuffer, mCapInputbuffer,
                                      gMultiCam->mLocalBufferNumber,
                                      gMultiCam->mLocalBufferList);
                mCapInputbuffer = NULL;
            }
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                currStreamType, CAM_TYPE_MAIN);
        }
    } else if (currStreamType == CALLBACK_STREAM) {
        // process aux1 preview buffer
        {
            Mutex::Autolock l(mNotifyLockAux1);
            for (List<camera3_notify_msg_t>::iterator i =
                     gMultiCam->mNotifyListAux1.begin();
                 i != gMultiCam->mNotifyListAux1.end(); i++) {
                HAL_LOGD("mNotifyListAux1_preview");
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        gMultiCam->mNotifyListAux1.erase(i);
                        HAL_LOGD("mNotifyListAux1_NOTIFY_SUCCESS_preview");
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("aux1 return local buffer:%d caused by error "
                            "Notify status", result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        gMultiCam->mNotifyListAux1.erase(i);
                        return;
                    }
                }
            }
        }

        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("aux1 found no corresponding notify");
            return;
        }

        /* Process error buffer for Aux1 camera: just return local buffer*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGV("aux1 return local buffer:%d caused by error Buffer status",
                result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }

        hwi_frame_buffer_info_t cur_frame;
        hwi_frame_buffer_info_t matched_frame1;
        hwi_frame_buffer_info_t matched_frame2;

        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame1, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame2, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
#ifdef CONFIG_WIDE_ULTRAWIDE_SUPPORT
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListMain,
                                               &matched_frame1)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number =
                    matched_frame1.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame1.buffer;
                muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("aux1 preview enqueue combo frame:%d for frame merge!",
                        muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five
                    // frame.
                    // for first frame meta and ok status buffer update at
                    // the
                    // same time.
                    // app need the point that the first meta updated to
                    // hide
                    // image cover
                    if (cur_frame.frame_number > 5)
                        clearFrameNeverMatched(matched_frame1.frame_number,
                                               cur_frame.frame_number, 0);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            }
#else
            if (MATCH_SUCCESS ==
                matchThreeFrame(cur_frame, mUnmatchedFrameListMain,
                                mUnmatchedFrameListAux2, &matched_frame1,
                                &matched_frame2)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number =
                    matched_frame1.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame1.buffer;
                muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
                muxer_msg.combo_frame.buffer3 = matched_frame2.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("aux1 preview enqueue combo frame:%d for frame merge!",
                        muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five
                    // frame.
                    // for first frame meta and ok status buffer update at
                    // the
                    // same time.
                    // app need the point that the first meta updated to
                    // hide
                    // image cover
                    if (cur_frame.frame_number > 5) {
                        HAL_LOGV("aux1 preview cur_frame.frame_number > 5");
                        clearFrameNeverMatched(matched_frame1.frame_number,
                                               cur_frame.frame_number,
                                               matched_frame2.frame_number);
                    }
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            }
#endif
            else {
                HAL_LOGV("aux1 preview enqueue newest unmatched frame:%d",
                    cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux1);
                if (discard_frame != NULL) {
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    delete discard_frame;
                }
            }
        }
    } else if (currStreamType == VIDEO_STREAM) {
        // process aux1 video buffer
        {
            Mutex::Autolock l(mNotifyLockVideoAux1);
            for (List<camera3_notify_msg_t>::iterator i =
                     gMultiCam->mNotifyListVideoAux1.begin();
                 i != gMultiCam->mNotifyListVideoAux1.end(); i++) {
                HAL_LOGD("mNotifyListVideoAux1_video");
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        gMultiCam->mNotifyListVideoAux1.erase(i);
                        HAL_LOGD("mNotifyListVideoAux1_NOTIFY_SUCCESS_video");
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("aux1 return local buffer:%d caused by error "
                            "Notify status", result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        gMultiCam->mNotifyListVideoAux1.erase(i);
                        return;
                    }
                }
            }
        }

        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("aux1 found no corresponding notify");
            return;
        }

        /* Process error buffer for Aux1 camera: just return local buffer*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGV("aux1 return local buffer:%d caused by error Buffer status",
                result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }

        hwi_frame_buffer_info_t cur_frame;
        hwi_frame_buffer_info_t matched_frame1;
        hwi_frame_buffer_info_t matched_frame2;

        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame1, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame2, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;

        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS ==
                matchThreeFrame(cur_frame, mUnmatchedFrameListVideoMain,
                                mUnmatchedFrameListVideoAux2, &matched_frame1,
                                &matched_frame2)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number =
                    matched_frame1.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame1.buffer;
                muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
                muxer_msg.combo_frame.buffer3 = matched_frame2.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mVideoThread->mMergequeueMutex);
                    HAL_LOGD("aux1 video enqueue combo frame:%d for frame merge!",
                        muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five
                    // frame.
                    // for first frame meta and ok status buffer update at
                    // the
                    // same time.
                    // app need the point that the first meta updated to
                    // hide
                    // image cover
                    if (cur_frame.frame_number > 5) {
                        HAL_LOGD("aux1 video cur_frame.frame_number > 5");
                        clearFrameNeverMatched(matched_frame1.frame_number,
                                               cur_frame.frame_number,
                                               matched_frame2.frame_number);
                    }
                    mVideoThread->mVideoMuxerMsgList.push_back(
                        muxer_msg);
                    mVideoThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("aux1 video enqueue newest unmatched frame:%d",
                    cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListVideoAux1);
                if (discard_frame != NULL) {
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    delete discard_frame;
                }
            }
        }

    }


    return;
}

/*===========================================================================
 * FUNCTION     : processCaptureResultAux2
 *
 * DESCRIPTION : process frame of camera index 2
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::processCaptureResultAux2(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    CameraMetadata metadata;
    meta_save_t metadata_t;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    metadata = result->result;
    uint64_t result_timestamp = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    camera_metadata_entry_t entry;
    float zoomWidth, zoomHeight;

    if (result_buffer == NULL) {
        // aux2 meta process
        if (metadata.exists(ANDROID_SPRD_AI_SCENE_TYPE_CURRENT)) {
            gMultiCam->aux2_ai_scene_type =
                metadata.find(ANDROID_SPRD_AI_SCENE_TYPE_CURRENT).data.u8[0];
        }
        if (metadata.exists(ANDROID_CONTROL_AF_STATE)) {
            gMultiCam->aux2_af_state=
                metadata.find(ANDROID_CONTROL_AF_STATE).data.u8[0];
        }

        entry = metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES);
        if (entry.count != 0) {
            gMultiCam->aux2_face_number = entry.count / 4;
        } else {
            gMultiCam->aux2_face_number = 0;
        }
        if (gMultiCam->aux2_face_number) {
            for (int i = 0; i < gMultiCam->aux2_face_number; i++) {
                // zoomWidth = ((float)mWideMaxWidth) /
                // ((float)mTeleMaxWidth)/0.6;
                // zoomHeight = ((float)mWideMaxHeight) /
                // ((float)mTeleMaxHeight)/0.6;
                zoomWidth = 1.0;
                zoomHeight = 1.0;
                float left_offest = 0, top_offest = 0;
                left_offest =
                    ((float)mWideMaxWidth / 0.6 - (float)mTeleMaxWidth) / 2;
                top_offest =
                    ((float)mWideMaxHeight / 0.6 - (float)mTeleMaxHeight) / 2;
                // left_offest = 0;
                // top_offest  = 0;
                gMultiCam->aux2_FaceRect[i * 4 + 0] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 0] *
                        zoomWidth +
                    left_offest;
                gMultiCam->aux2_FaceRect[i * 4 + 1] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 1] *
                        zoomHeight +
                    top_offest;
                gMultiCam->aux2_FaceRect[i * 4 + 2] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 2] *
                        zoomWidth +
                    left_offest;
                gMultiCam->aux2_FaceRect[i * 4 + 3] =
                    metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 3] *
                        zoomHeight +
                    top_offest;

                gMultiCam->aux2_FaceGenderRaceAge[i] =
                    metadata.find(ANDROID_SPRD_FACE_ATTRIBUTES).data.i32[i];

                HAL_LOGV("aux2_FaceRect =  %d, %d, %d, %d zoomWidth %f "
                    "zoomHeight %f gMultiCam->aux2_face_number %d",
                    gMultiCam->aux2_FaceRect[i * 4 + 0],
                    gMultiCam->aux2_FaceRect[i * 4 + 1],
                    gMultiCam->aux2_FaceRect[i * 4 + 2],
                    gMultiCam->aux2_FaceRect[i * 4 + 3], zoomWidth,
                    zoomHeight, gMultiCam->aux2_face_number);
            }
        }

        if (cur_frame_number == mCapFrameNum && cur_frame_number != 0) {
            if (gMultiCam->mRequstState == REPROCESS_STATE) {
                HAL_LOGD("aux2 hold jpeg picture call back, framenumber:%d",
                    result->frame_number);
            } else {
                if (mIsVideoMode == false) {
                    {
                        HAL_LOGD("aux2 CallBackMetadata, framenumber:%d",
                            result->frame_number);
                        Mutex::Autolock(gMultiCam->mMetatLock);
                        metadata_t.frame_number = cur_frame_number;
                        metadata_t.metadata = clone_camera_metadata(result->result);
                        mMetadataList.push_back(metadata_t);
                    }
                    CallBackMetadata();
                }
            }
                return;
        }

            return;
    }

    if (mFlushing) {
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                       mLocalBufferNumber, mLocalBufferList);
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGD("aux2 frame %d, stream type %d", cur_frame_number, currStreamType);
    /* Process error buffer for Aux2 camera*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGD("aux2 return local buffer:%d caused by error Buffer status",
            result->frame_number);
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
        if (currStreamType == DEFAULT_STREAM) {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                           SNAPSHOT_STREAM, CAM_TYPE_AUX2);
        }
        return;
    }
    if (currStreamType == DEFAULT_STREAM) {

        Mutex::Autolock l(mDefaultStreamLock);
        result_buffer = result->output_buffers;
        /*if (NULL == mTWCaptureThread->mSavedOneResultBuff) {
            mTWCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {*/
        muxer_queue_msg_t capture_msg;

        capture_msg.msg_type = MUXER_MSG_DATA_PROC;
        capture_msg.combo_frame.frame_number = result->frame_number;
        capture_msg.combo_frame.buffer3 = result->output_buffers->buffer;
        capture_msg.combo_frame.input_buffer = result->input_buffer;
        {
            Mutex::Autolock l(mTWCaptureThread->mMergequeueMutex);
            HAL_LOGV("aux2 capture enqueue combo frame:%d for frame merge!",
                capture_msg.combo_frame.frame_number);
            mTWCaptureThread->mCaptureMsgList.push_back(capture_msg);
            mTWCaptureThread->mMergequeueSignal.signal();
        }
        //}
    } else if (currStreamType == SNAPSHOT_STREAM) {
    if (mIsVideoMode == true) {
            if ((gMultiCam->mZoomValue >=
                    gMultiCam->mSwitch_W_T_Threshold)) {
                void *output_buf_addr2 = NULL;
                void *input_buf_addr2 = NULL;
                int rc = 0;

                rc = gMultiCam->map((result->output_buffers)->buffer,
                    &input_buf_addr2);
                rc = gMultiCam->map(mSavedSnapRequest.buffer,
                    &output_buf_addr2);
                memcpy(output_buf_addr2, input_buf_addr2,
                    ADP_BUFSIZE(*mSavedSnapRequest.buffer));
                gMultiCam->unmap((result->output_buffers)->buffer);
                gMultiCam->unmap(mSavedSnapRequest.buffer);
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, (result->output_buffers)->buffer,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);

                CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                    currStreamType, CAM_TYPE_MAIN);

                // dump capture data
                {
                    char prop[PROPERTY_VALUE_MAX] = {0};
                    property_get("persist.vendor.cam.multi.dump.video.jpeg2", prop, "0");
                    if (!strcmp(prop, "capjpeg") || !strcmp(prop, "all")) {
                        // input_buf1 or left image
                        char tmp_str[64] = {0};
                        sprintf(tmp_str, "_zoom=%f", gMultiCam->mZoomValue);
                        char Capture[80] = "MultiCapture";
                        strcat(Capture, tmp_str);
                        rc = gMultiCam->map((result->output_buffers)->buffer,
                            &input_buf_addr2);
                        // output_buffer
                        gMultiCam->dumpData(
                            (unsigned char *)input_buf_addr2, 2,
                            ADP_BUFSIZE(*(result->output_buffers->buffer)),
                            gMultiCam->mSavedSnapRequest.snap_stream->width,
                            gMultiCam->mSavedSnapRequest.snap_stream->height,
                            cur_frame_number, Capture);
                        gMultiCam->unmap((result->output_buffers)->buffer);
                    }
                }
            }else {
                gMultiCam->pushBufferList(
                    gMultiCam->mLocalBuffer, (result->output_buffers)->buffer,
                    gMultiCam->mLocalBufferNumber, gMultiCam->mLocalBufferList);
            }
        } else {
            if (mCapInputbuffer) {
                gMultiCam->pushBufferList(gMultiCam->mLocalBuffer, mCapInputbuffer,
                                      gMultiCam->mLocalBufferNumber,
                                      gMultiCam->mLocalBufferList);
                mCapInputbuffer = NULL;
            }
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                currStreamType, CAM_TYPE_MAIN);
        }
    } else if (currStreamType == CALLBACK_STREAM) {
        // process aux2 preview buffer
        {
            Mutex::Autolock l(mNotifyLockAux2);
            for (List<camera3_notify_msg_t>::iterator i =
                     gMultiCam->mNotifyListAux2.begin();
                 i != gMultiCam->mNotifyListAux2.end(); i++) {
                HAL_LOGD("mNotifyListAux2_preview");
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        gMultiCam->mNotifyListAux2.erase(i);
                        HAL_LOGD("mNotifyListAux2_NOTIFY_SUCCESS_preview");
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("aux2 return local buffer:%d caused by error "
                             "Notify status", result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        gMultiCam->mNotifyListAux2.erase(i);
                        return;
                    }
                }
            }
        }

        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("aux2 found no corresponding notify");
            return;
        }

        /* Process error buffer for Aux2 camera: just return local buffer*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGV("aux2 return local buffer:%d caused by error Buffer status",
                result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }

        hwi_frame_buffer_info_t cur_frame;
        hwi_frame_buffer_info_t matched_frame1;
        hwi_frame_buffer_info_t matched_frame2;

        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame1, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame2, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS ==
                matchThreeFrame(cur_frame, mUnmatchedFrameListMain,
                                mUnmatchedFrameListAux1, &matched_frame1,
                                &matched_frame2)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number =
                    matched_frame1.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame1.buffer;
                muxer_msg.combo_frame.buffer2 = matched_frame2.buffer;
                muxer_msg.combo_frame.buffer3 = cur_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("aux2 preview enqueue combo frame:%d for frame merge!",
                        muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five
                    // frame.
                    // for first frame meta and ok status buffer update at
                    // the
                    // same time.
                    // app need the point that the first meta updated to
                    // hide
                    // image cover
                    if (cur_frame.frame_number > 5) {
                        HAL_LOGD("aux2 preview cur_frame.frame_number > 5");
                        clearFrameNeverMatched(matched_frame1.frame_number,
                                               matched_frame2.frame_number,
                                               cur_frame.frame_number);
                    }
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("aux2 preview enqueue newest unmatched frame:%d",
                    cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux2);
                if (discard_frame != NULL) {
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    delete discard_frame;
                }
            }
        }
    } else if (currStreamType == VIDEO_STREAM) {
        // process aux2 video buffer
        {
            Mutex::Autolock l(mNotifyLockVideoAux2);
            for (List<camera3_notify_msg_t>::iterator i =
                     gMultiCam->mNotifyListVideoAux2.begin();
                 i != gMultiCam->mNotifyListVideoAux2.end(); i++) {
                  HAL_LOGD("mNotifyListVideoAux2_video");
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        gMultiCam->mNotifyListVideoAux2.erase(i);
                        HAL_LOGD("mNotifyListVideoAux2_NOTIFY_SUCCESS_video");
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("aux2 return local buffer:%d caused by error "
                            "Notify status", result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        gMultiCam->mNotifyListVideoAux2.erase(i);
                        return;
                    }
                }
            }
        }

        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("found no corresponding notify");
            return;
        }

        /* Process error buffer for Aux2 camera: just return local buffer*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGV("Return local buffer:%d caused by error Buffer status",
                result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }

        hwi_frame_buffer_info_t cur_frame;
        hwi_frame_buffer_info_t matched_frame1;
        hwi_frame_buffer_info_t matched_frame2;

        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame1, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&matched_frame2, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;

        {
            Mutex::Autolock l(mUnmatchedQueueLock);
           if (MATCH_SUCCESS ==
                matchThreeFrame(cur_frame, mUnmatchedFrameListVideoMain,
                                mUnmatchedFrameListVideoAux1, &matched_frame1,
                                &matched_frame2)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number =
                    matched_frame1.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame1.buffer;
                muxer_msg.combo_frame.buffer2 = matched_frame2.buffer;
                muxer_msg.combo_frame.buffer3 = cur_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mVideoThread->mMergequeueMutex);
                    HAL_LOGV("aux2 video enqueue combo frame:%d for frame merge!",
                        muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five
                    // frame.
                    // for first frame meta and ok status buffer update at
                    // the
                    // same time.
                    // app need the point that the first meta updated to
                    // hide
                    // image cover
                    if (cur_frame.frame_number > 5) {
                        HAL_LOGD("aux2 video cur_frame.frame_number > 5");
                        clearFrameNeverMatched(matched_frame1.frame_number,
                                               matched_frame2.frame_number,
                                               cur_frame.frame_number);
                    }
                    mVideoThread->mVideoMuxerMsgList.push_back(
                        muxer_msg);
                    mVideoThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("aux2 video enqueue newest unmatched frame:%d",
                    cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListVideoAux2);
                if (discard_frame != NULL) {
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    delete discard_frame;
                }
            }
        }

    }

    return;
}

/*===========================================================================
 * FUNCTION     : setAlgoTrigger
 *
 * DESCRIPTION : set algo status
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::setAlgoTrigger(int vcm) {

    if (mVcmSteps == vcm && mVcmSteps) {

        mAlgoStatus = 1;
    } else {
        mAlgoStatus = 0;
    }
    HAL_LOGV("mAlgoStatus %d,trigger=%d,vcm=%d", mAlgoStatus, mVcmSteps, vcm);

    mVcmSteps = vcm;
}

/*===========================================================================
 * FUNCTION     : clearFrameNeverMatched
 *
 * DESCRIPTION : clear earlier frame which will never be matched any more
 *
 * PARAMETERS : which camera queue to be clear
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3MultiCamera::clearFrameNeverMatched(
    uint32_t main_frame_number, uint32_t sub_frame_number,
    uint32_t sub2_frame_number) {
    List<hwi_frame_buffer_info_t>::iterator itor;
    uint32_t frame_num = 0;
    Mutex::Autolock l(mClearBufferLock);
    HAL_LOGV("clearFrameNeverMatched:%d", main_frame_number);

    itor = mUnmatchedFrameListMain.begin();
    while (itor != mUnmatchedFrameListMain.end()) {
        if (itor->frame_number < main_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame main idx:%d", itor->frame_number);
            frame_num = itor->frame_number;
            mUnmatchedFrameListMain.erase(itor);
            CallBackResult(frame_num, CAMERA3_BUFFER_STATUS_ERROR,
                           CALLBACK_STREAM, CAM_TYPE_MAIN);
        }
        itor++;
    }

    itor = mUnmatchedFrameListAux1.begin();
    while (itor != mUnmatchedFrameListAux1.end()) {
        if (itor->frame_number < sub_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux1 idx:%d", itor->frame_number);
            mUnmatchedFrameListAux1.erase(itor);
        }
        itor++;
    }

    itor = mUnmatchedFrameListAux2.begin();
    while (itor != mUnmatchedFrameListAux2.end()) {
        if (itor->frame_number < sub2_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux2 idx:%d", itor->frame_number);
            mUnmatchedFrameListAux2.erase(itor);
        }
        itor++;
    }

    itor = mUnmatchedFrameListVideoMain.begin();
    while (itor != mUnmatchedFrameListVideoMain.end()) {
        if (itor->frame_number < main_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame main video idx:%d", itor->frame_number);
            frame_num = itor->frame_number;
            mUnmatchedFrameListVideoMain.erase(itor);
            CallBackResult(frame_num, CAMERA3_BUFFER_STATUS_ERROR,
                           CALLBACK_STREAM, CAM_TYPE_MAIN);
        }
        itor++;
    }

    itor = mUnmatchedFrameListVideoAux1.begin();
    while (itor != mUnmatchedFrameListVideoAux1.end()) {
        if (itor->frame_number < sub_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux1 video idx:%d", itor->frame_number);
            mUnmatchedFrameListVideoAux1.erase(itor);
        }
        itor++;
    }

    itor = mUnmatchedFrameListVideoAux2.begin();
    while (itor != mUnmatchedFrameListVideoAux2.end()) {
        if (itor->frame_number < sub2_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux2 video idx:%d", itor->frame_number);
            mUnmatchedFrameListVideoAux2.erase(itor);
        }
        itor++;
    }

}

/*===========================================================================
 * FUNCTION     : initAlgo
 *
 * DESCRIPTION :
 *
 * PARAMETERS : none
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::initAlgo() {
    int ret;
    WT_inparam algo_info;
    algo_info.yuv_mode = 1;
    algo_info.is_preview = 1;
    algo_info.image_width_wide = mMainSize.preview_w;
    algo_info.image_height_wide = mMainSize.preview_h;
    algo_info.image_width_tele = mMainSize.preview_w;
    algo_info.image_height_tele = mMainSize.preview_h;
    algo_info.otpbuf = &mOtpData.otp_data;
    algo_info.otpsize = mOtpData.otp_size;
    algo_info.VCMup = 0;
    algo_info.VCMdown = 1000;
    HAL_LOGD("otpType %d, otpSize %d, otpbuf =%p ", mOtpData.otp_type,
        mOtpData.otp_size, &mOtpData.otp_data);
    HAL_LOGD(
        "yuv_mode=%d,is_preview=%d,image_height_tele = %d,image_width_tele "
        "=%d,image_height_wide =%d,image_width_wide =%d,VCMup=%d,",
        algo_info.yuv_mode, algo_info.is_preview, algo_info.image_height_tele,
        algo_info.image_width_tele, algo_info.image_height_wide,
        algo_info.image_width_wide, algo_info.VCMup);
    ret = WTprocess_Init(&mPrevAlgoHandle, &algo_info);
    if (ret) {
        HAL_LOGE("WT Algo Prev Init failed");
    }
    algo_info.is_preview = 0;
    algo_info.image_width_wide = mMainSize.capture_w;
    algo_info.image_height_wide = mMainSize.capture_h;
    algo_info.image_width_tele = mMainSize.capture_w;
    algo_info.image_height_tele = mMainSize.capture_h;
    ret = WTprocess_Init(&mCapAlgoHandle, &algo_info);
    if (ret) {
        HAL_LOGE("WT Algo Cap Init failed");
    }
}

/*===========================================================================
 * FUNCTION     : runAlgo
 *
 * DESCRIPTION :
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3MultiCamera::runAlgo(void *handle,
                                    buffer_handle_t *input_image_wide,
                                    buffer_handle_t *input_image_tele,
                                    buffer_handle_t *output_image) {
    int ret;
    WT_inparam *algoInfo = (WT_inparam *)handle;
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;
    int32_t yuvTextFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    uint32_t inWidth = 0, inHeight = 0, inStride = 0;
#if defined(CONFIG_SPRD_ANDROID_8)
    uint32_t inLayCount = 1;
#endif
    HAL_LOGD("algoInfo->is_preview =%d", algoInfo->is_preview);
    if (algoInfo->is_preview) {
        inWidth = mMainSize.preview_w;
        inHeight = mMainSize.preview_h;
        inStride = mMainSize.preview_w;
    } else if (algoInfo->is_preview == 0) {
        inWidth = mMainSize.capture_w;
        inHeight = mMainSize.capture_h;
        inStride = mMainSize.capture_w;
    }
    HAL_LOGD(
        "yuvTextUsage "
        "=%d,yuvTextFormat=%d,inWidth=%d,inHeight=%d,inStride=%d,inLayCount=%d",
        yuvTextUsage, yuvTextFormat, inWidth, inHeight, inStride, inLayCount);
#if defined(CONFIG_SPRD_ANDROID_8)
    sp<GraphicBuffer> inputWideBuffer = new GraphicBuffer(
        (native_handle_t *)(*input_image_wide),
        GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
        yuvTextFormat, inLayCount, yuvTextUsage, inStride);
    sp<GraphicBuffer> inputTeleBuffer = new GraphicBuffer(
        (native_handle_t *)(*input_image_tele),
        GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
        yuvTextFormat, inLayCount, yuvTextUsage, inStride);
    sp<GraphicBuffer> outputBuffer = new GraphicBuffer(
        (native_handle_t *)(*output_image),
        GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
        yuvTextFormat, inLayCount, yuvTextUsage, inStride);
#else

    sp<GraphicBuffer> inputWideBuffer =
        new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                          inStride, (native_handle_t *)(*input_image_wide), 0);
    sp<GraphicBuffer> inputTeleBuffer =
        new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                          inStride, (native_handle_t *)(*input_image_tele), 0);
    sp<GraphicBuffer> outputBuffer =
        new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                          inStride, (native_handle_t *)(*output_image), 0);
#endif

    ret = WTprocess_function(handle, &(*inputWideBuffer), &(*inputTeleBuffer),
                             &(*outputBuffer), mVcmSteps, mZoomValue);

    if (ret) {
        HAL_LOGE("WT Algo Run failed");
    }
    return ret;
}

/*===========================================================================
 * FUNCTION     : deinitAlgo
 *
 * DESCRIPTION :
 *
 * PARAMETERS : none
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3MultiCamera::deinitAlgo() {
    int ret;
    HAL_LOGV("E");
    if (mPrevAlgoHandle) {
        ret = WTprocess_deinit(mPrevAlgoHandle);
        if (ret) {
            HAL_LOGE("WT Algo Prev deinit failed");
        }
        mPrevAlgoHandle = NULL;
    }
    if (mCapAlgoHandle) {
        ret = WTprocess_deinit(mCapAlgoHandle);
        if (ret) {
            HAL_LOGE("WT Algo Cap deinit failed");
        }
        mCapAlgoHandle = NULL;
    }
    HAL_LOGV("X");
}
}
