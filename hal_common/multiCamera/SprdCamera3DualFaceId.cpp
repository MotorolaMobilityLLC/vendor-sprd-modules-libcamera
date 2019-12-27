/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
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
#define LOG_TAG "Cam3DualFaceId"
#include "SprdCamera3DualFaceId.h"

using namespace android;
namespace sprdcamera {

#define PENDINGTIME (1000000)
#define PENDINGTIMEOUT (5000000000)

#define MASTER_ID 0

SprdCamera3DualFaceId *mFaceId = NULL;

// Error Check Macros
#define CHECK_FACEID()                                                         \
    if (!mFaceId) {                                                            \
        HAL_LOGE("Error getting faceid");                                      \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_FACEID_ERROR()                                                   \
    if (!mFaceId) {                                                            \
        HAL_LOGE("Error getting mFaceId");                                     \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3DualFaceId::mCameraCaptureOps = {
    .initialize = SprdCamera3DualFaceId::initialize,
    .configure_streams = SprdCamera3DualFaceId::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3DualFaceId::construct_default_request_settings,
    .process_capture_request = SprdCamera3DualFaceId::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3DualFaceId::dump,
    .flush = SprdCamera3DualFaceId::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3DualFaceId::callback_ops_main = {
    .process_capture_result =
        SprdCamera3DualFaceId::process_capture_result_main,
    .notify = SprdCamera3DualFaceId::notifyMain};

camera3_callback_ops SprdCamera3DualFaceId::callback_ops_aux = {
    .process_capture_result = SprdCamera3DualFaceId::process_capture_result_aux,
    .notify = SprdCamera3DualFaceId::notifyAux};

/*===========================================================================
 * FUNCTION   : SprdCamera3FaceIdUnlock
 *
 * DESCRIPTION: SprdCamera3FaceIdUnlock Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3DualFaceId::SprdCamera3DualFaceId() {
    HAL_LOGI("E");

    m_pPhyCamera = NULL;
    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = (uint8_t)CAM_FACE_MAIN_ID;
    mStaticMetadata = NULL;
    mPhyCameraNum = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mReqTimestamp = 0;
    mMaxPendingCount = 0;
    mPendingRequest = 0;
    mFlushing = false;
    mSavedRequestList.clear();
    mLocalBufferListMain.clear();
    mLocalBufferListAux.clear();
    mResultBufferListMain.clear();
    mResultBufferListAux.clear();
    memset(mLocalBufferMain, 0, sizeof(new_mem_t) * DUAL_FACEID_BUFFER_SUM);
    memset(mLocalBufferAux, 0, sizeof(new_mem_t) * DUAL_FACEID_BUFFER_SUM);
    memset(mMainStreams, 0, sizeof(camera3_stream_t) * DUAL_FACEID_MAX_STREAMS);
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * (DUAL_FACEID_MAX_STREAMS - 1));
    memset(&mOtpData, 0, sizeof(OtpData));
    memset(&mOtpLocalBuffer, 0, sizeof(new_mem_t));

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3FaceIdUnlock
 *
 * DESCRIPTION: SprdCamera3FaceIdUnlock Desctructor
 *
 *==========================================================================*/
SprdCamera3DualFaceId::~SprdCamera3DualFaceId() {
    HAL_LOGV("E");

    HAL_LOGV("X");
}

/*===========================================================================
 * FUNCTION   : getCameraSidebyside
 *
 * DESCRIPTION: Creates Camera Blur if not created
 *
 * PARAMETERS:
 *   @pCapture               : Pointer to retrieve Camera Switch
 *
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3DualFaceId::getCameraFaceId(SprdCamera3DualFaceId **pFaceid) {
    if (!mFaceId) {
        mFaceId = new SprdCamera3DualFaceId();
    }

    CHECK_FACEID();
    *pFaceid = mFaceId;
    HAL_LOGV("mFaceId=%p", mFaceId);

    return;
}

/*===========================================================================
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: get logical camera info
 *
 * PARAMETERS:
 *   @camera_id     : Logical Camera ID
 *   @info              : Logical Main Camera Info
 *
 * RETURN    :
 *              NO_ERROR  : success
 *              ENODEV : Camera not found
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3DualFaceId::get_camera_info(__unused int camera_id,
                                           struct camera_info *info) {
    HAL_LOGI("E");

    int rc = NO_ERROR;

    if (info) {
        rc = mFaceId->getCameraInfo(camera_id, info);
    }

    HAL_LOGI("X, rc=%d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   : camera_device_open
 *
 * DESCRIPTION: static function to open a camera device by its ID
 *
 * PARAMETERS :
 *   @modue: hw module
 *   @id : camera ID
 *   @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              BAD_VALUE : Invalid Camera ID
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3DualFaceId::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    HAL_LOGI("id=%d", atoi(id));
    rc = mFaceId->cameraDeviceOpen(atoi(id), hw_device);

    HAL_LOGI("id=%d, rc: %d", atoi(id), rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   : close_camera_device
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *   @hw_dev : camera hardware device info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3DualFaceId::close_camera_device(__unused hw_device_t *hw_dev) {
    if (NULL == hw_dev) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mFaceId->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION   : closeCameraDevice
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3DualFaceId::closeCameraDevice() {
    HAL_LOGD("E");

    int rc = NO_ERROR;
    int i = 0;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    // Attempt to close all cameras regardless of unbundle results
    for (i = 0; i < mPhyCameraNum; i++) {
        sprdCam = &m_pPhyCamera[i];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (NULL == dev) {
            continue;
        }
        HAL_LOGI("camera id:%d", i);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (NO_ERROR != rc) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }

    freeLocalCapBuffer();
    mSavedRequestList.clear();
    mLocalBufferListMain.clear();
    mLocalBufferListAux.clear();
    mResultBufferListMain.clear();
    mResultBufferListAux.clear();
    mReqTimestamp = 0;
    mMaxPendingCount = 0;
    mPendingRequest = 0;
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }

    HAL_LOGD("X, rc: %d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   :initialize
 *
 * DESCRIPTION: initialize camera device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3DualFaceId::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGI("E");

    int rc = NO_ERROR;

    CHECK_FACEID_ERROR();
    rc = mFaceId->initialize(callback_ops);

    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :construct_default_request_settings
 *
 * DESCRIPTION: construc default request settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t *
SprdCamera3DualFaceId::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    HAL_LOGV("E");

    const camera_metadata_t *rc;

    if (!mFaceId) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }

    rc = mFaceId->constructDefaultRequestSettings(device, type);

    HAL_LOGV("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :configure_streams
 *
 * DESCRIPTION: configure streams
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3DualFaceId::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    int rc = 0;

    CHECK_FACEID_ERROR();
    rc = mFaceId->configureStreams(device, stream_list);

    HAL_LOGV("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_request
 *
 * DESCRIPTION: process capture request
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3DualFaceId::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    HAL_LOGV("E");

    int rc = 0;

    CHECK_FACEID_ERROR();
    rc = mFaceId->processCaptureRequest(device, request);

    HAL_LOGV("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdUnlock
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3DualFaceId::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("frame number=%d", result->frame_number);

    CHECK_FACEID();
    mFaceId->processCaptureResultMain((camera3_capture_result_t *)result);
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION:  main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3DualFaceId::notifyMain(const struct camera3_callback_ops *ops,
                                       const camera3_notify_msg_t *msg) {
    HAL_LOGI("frame number=%d", msg->message.shutter.frame_number);

    CHECK_FACEID();
    mFaceId->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdUnlock
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3DualFaceId::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("frame number=%d", result->frame_number);

    CHECK_FACEID();
    mFaceId->processCaptureResultAux((camera3_capture_result_t *)result);
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION:  main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3DualFaceId::notifyAux(const struct camera3_callback_ops *ops,
                                      const camera3_notify_msg_t *msg) {
    // This mode do not have real aux notify
    CHECK_FACEID();
}

/*===========================================================================
 * FUNCTION   : freeLocalCapBuffer
 *
 * DESCRIPTION: free faceid_unlock_alloc_mem_t buffer
 *
 * PARAMETERS:
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3DualFaceId::freeLocalCapBuffer() {
    HAL_LOGV("E");

    size_t buffer_num = DUAL_FACEID_BUFFER_SUM;
    for (size_t i = 0; i < buffer_num; i++) {
        freeOneBuffer(&mLocalBufferMain[i]);
        freeOneBuffer(&mLocalBufferAux[i]);
    }
    freeOneBuffer(&mOtpLocalBuffer);

    HAL_LOGV("X");
}

/*===========================================================================
 * FUNCTION   : cameraDeviceOpen
 *
 * DESCRIPTION: open a camera device with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     : int type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3DualFaceId::cameraDeviceOpen(int camera_id,
                                            struct hw_device_t **hw_device) {
    HAL_LOGI("E");

    int rc = NO_ERROR;
    int i = 0;
    uint32_t Phy_id = 0;

    if ((SPRD_DUAL_FACEID_REGISTER_ID == camera_id) ||
        (SPRD_DUAL_FACEID_UNLOCK_ID == camera_id)) {
        mPhyCameraNum = 2;
    } else {
        HAL_LOGW("face id mode camera_id should not be %d", camera_id);
    }

    hw_device_t *hw_dev[mPhyCameraNum];
    setupPhysicalCameras();

    // Open all physical cameras
    for (i = 0; i < mPhyCameraNum; i++) {
        Phy_id = m_pPhyCamera[i].id;

        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)Phy_id);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        if (SPRD_DUAL_FACEID_REGISTER_ID == camera_id) {
            hw->setMultiCameraMode(MODE_DUAL_FACEID_REGISTER);
            mFaceMode = MODE_DUAL_FACEID_REGISTER;
        } else if (SPRD_DUAL_FACEID_UNLOCK_ID == camera_id) {
            hw->setMultiCameraMode(MODE_DUAL_FACEID_UNLOCK);
            mFaceMode = MODE_DUAL_FACEID_UNLOCK;
        }
        hw->setMasterId(MASTER_ID);
        rc = hw->openCamera(&hw_dev[i]);
        if (NO_ERROR != rc) {
            HAL_LOGE("failed, camera id:%d", Phy_id);
            delete hw;
            closeCameraDevice();
            return rc;
        }

        m_pPhyCamera[i].dev = reinterpret_cast<camera3_device_t *>(hw_dev[i]);
        m_pPhyCamera[i].hwi = hw;
    }

    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraCaptureOps;
    m_VirtualCamera.dev.priv = (void *)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;

    HAL_LOGI("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   : getCameraInfo
 *
 * DESCRIPTION: query camera information with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3DualFaceId::getCameraInfo(int face_camera_id,
                                         struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;

    HAL_LOGI("camera_id=%d", face_camera_id);

    m_VirtualCamera.id = CAM_MAIN_FACE_ID;
    camera_id = m_VirtualCamera.id;
    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        HAL_LOGE("getStaticMetadata failed");
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(camera_id, info);
    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;

    return rc;
}

/*===========================================================================
 * FUNCTION   : setupPhysicalCameras
 *
 * DESCRIPTION: Creates Camera Capture if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3DualFaceId::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[mPhyCameraNum];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }

    memset(m_pPhyCamera, 0x00,
           (mPhyCameraNum * sizeof(sprdcamera_physical_descriptor_t)));

    m_pPhyCamera[CAM_TYPE_MAIN].id = (uint8_t)CAM_MAIN_FACE_ID;
    m_pPhyCamera[CAM_TYPE_AUX].id = (uint8_t)CAM_AUX_FACE_ID;

    return NO_ERROR;
}

/*======================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump fd
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3DualFaceId::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    CHECK_FACEID();

    mFaceId->_dump(device, fd);

    HAL_LOGV("X");
}

/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3DualFaceId::flush(const struct camera3_device *device) {
    HAL_LOGV("E");

    int rc = 0;

    CHECK_FACEID_ERROR();
    rc = mFaceId->_flush(device);

    HAL_LOGV("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :initialize
 *
 * DESCRIPTION: initialize device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3DualFaceId::initialize(
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGI("E");

    int rc = NO_ERROR;
    mFlushing = false;
    mLocalBufferListMain.clear();
    mLocalBufferListAux.clear();
    mResultBufferListMain.clear();
    mResultBufferListAux.clear();
    memset(&mOtpLocalBuffer, 0, sizeof(new_mem_t));

    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiMain);
    SprdCamera3MultiBase::initialize(MODE_DUAL_FACEID_UNLOCK, hwiMain);

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (NO_ERROR != rc) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_TYPE_AUX];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (NO_ERROR != rc) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    memset(mLocalBufferMain, 0, sizeof(new_mem_t) * DUAL_FACEID_BUFFER_SUM);
    memset(mLocalBufferAux, 0, sizeof(new_mem_t) * DUAL_FACEID_BUFFER_SUM);
    memset(mMainStreams, 0, sizeof(camera3_stream_t) * DUAL_FACEID_MAX_STREAMS);
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * (DUAL_FACEID_MAX_STREAMS - 1));
    mCallbackOps = callback_ops;

    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :configureStreams
 *
 * DESCRIPTION: configureStreams
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3DualFaceId::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");

    int rc = 0;
    camera3_stream_t *pmainStreams[DUAL_FACEID_MAX_STREAMS];
    camera3_stream_t *pauxStreams[DUAL_FACEID_MAX_STREAMS - 1];
    camera3_stream_t *previewStream = NULL;
    size_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    mReqTimestamp = 0;
    mMaxPendingCount = 0;
    mPendingRequest = 0;

    Mutex::Autolock l(mLock);

    HAL_LOGD("FACEID , num_streams=%d", stream_list->num_streams);
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD("%d: stream type=%d, width=%d, height=%d, "
                 "format=%d",
                 i, stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height,
                 stream_list->streams[i]->format);

        int requestStreamType = getStreamType(stream_list->streams[i]);
        if (PREVIEW_STREAM == requestStreamType) {
            previewStream = stream_list->streams[i];
            mPreviewWidth = stream_list->streams[i]->width;
            mPreviewHeight = stream_list->streams[i]->height;
            mMainStreams[i] = *stream_list->streams[i];
            mAuxStreams[i] = *stream_list->streams[i];
            pmainStreams[i] = &mMainStreams[i];
            pauxStreams[i] = &mAuxStreams[i];
        } else if (SNAPSHOT_STREAM == requestStreamType) {
            stream_list->streams[i]->max_buffers = 1;
        } else {
            stream_list->streams[i]->max_buffers = 1;
        }
    }
    // for callback buffer
    if (mFaceMode == MODE_DUAL_FACEID_REGISTER) {
        i = 1;
        mMainStreams[i] = *stream_list->streams[i];
        mMainStreams[i].stream_type = CAMERA3_STREAM_OUTPUT;
        mMainStreams[i].width = mPreviewWidth;
        mMainStreams[i].height = mPreviewHeight;
        mMainStreams[i].format = HAL_PIXEL_FORMAT_YCbCr_420_888;
        mMainStreams[i].usage = stream_list->streams[i]->usage;
        mMainStreams[i].max_buffers = 1;
        mMainStreams[i].data_space = stream_list->streams[i]->data_space;
        mMainStreams[i].rotation = stream_list->streams[i]->rotation;
        pmainStreams[i] = &mMainStreams[i];
    }
    freeLocalCapBuffer();
    for (i = 0; i < DUAL_FACEID_BUFFER_SUM; i++) {
        if (0 > allocateOne(mPreviewWidth, mPreviewHeight,
                            &(mLocalBufferMain[i]), YUV420)) {
            HAL_LOGE("request one buf failed.");
        }
        mLocalBufferMain[i].type = (camera_buffer_type_t)MAIN_BUFFER;
        mLocalBufferListMain.push_back(&mLocalBufferMain[i]);

        if (0 > allocateOne(mPreviewWidth, mPreviewHeight,
                            &(mLocalBufferAux[i]), YUV420)) {
            HAL_LOGE("request one buf failed.");
        }
        mLocalBufferAux[i].type = (camera_buffer_type_t)AUX_BUFFER;
        mLocalBufferListAux.push_back(&mLocalBufferAux[i]);
    }
    if (0 >
        allocateOne(mPreviewWidth, mPreviewHeight, &mOtpLocalBuffer, YUV420)) {
        HAL_LOGE("request one buf failed.");
    } else {
        void *otpAddr = NULL;
        map(&mOtpLocalBuffer.native_handle, &otpAddr);
        int otp_size = mOtpData.otp_size;
        memcpy(otpAddr, (void *)&otp_size, sizeof(int));
        memcpy((void *)((char *)otpAddr + sizeof(int)), mOtpData.otp_data,
               mOtpData.otp_size);
        unmap(&mOtpLocalBuffer.native_handle);
    }

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    if (mFaceMode == MODE_DUAL_FACEID_REGISTER) {
        mainconfig.num_streams = DUAL_FACEID_MAX_STREAMS;
    } else {
        mainconfig.num_streams = DUAL_FACEID_MAX_STREAMS - 1;
    }
    mainconfig.streams = pmainStreams;

    camera3_stream_configuration auxconfig;
    auxconfig = *stream_list;
    auxconfig.num_streams = DUAL_FACEID_MAX_STREAMS - 1;
    auxconfig.streams = pauxStreams;

    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_AUX].dev, &auxconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }
    if (previewStream != NULL) {
        memcpy(previewStream, &mMainStreams[0], sizeof(camera3_stream_t));
        HAL_LOGD("previewStream max buffer %d", previewStream->max_buffers);
        previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
    }
    for (i = 0; i < DUAL_FACEID_MAX_STREAMS; i++) {
        HAL_LOGV(
            "atfer configure treams: stream_type=%d, "
            "width=%d, height=%d, format=%d, usage=%d, "
            "max_buffers=%d, data_space=%d, rotation=%d",
            stream_list->streams[i]->stream_type,
            stream_list->streams[i]->width, stream_list->streams[i]->height,
            stream_list->streams[i]->format, stream_list->streams[i]->usage,
            stream_list->streams[i]->max_buffers,
            stream_list->streams[i]->data_space,
            stream_list->streams[i]->rotation);
    }
    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :constructDefaultRequestSettings
 *
 * DESCRIPTION: construct Default Request Settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t *SprdCamera3DualFaceId::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGI("E");

    const camera_metadata_t *fwk_metadata = NULL;
    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_MAIN].hwi;

    Mutex::Autolock l(mLock);

    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_MAIN].dev, type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }
    memset(&mOtpData, 0, sizeof(OtpData));

    CameraMetadata metadata;
    metadata = fwk_metadata;
    mOtpData.otp_exist = false;
    if (metadata.exists(ANDROID_SPRD_OTP_DATA)) {
        uint8_t otpType;
        int otpSize;
        otpType = SprdCamera3Setting::s_setting[m_pPhyCamera[CAM_TYPE_MAIN].id]
                      .otpInfo.otp_type;
        otpSize = SprdCamera3Setting::s_setting[m_pPhyCamera[CAM_TYPE_MAIN].id]
                      .otpInfo.otp_size;
        HAL_LOGI("otpType %d, otpSize %d", otpType, otpSize);
        mOtpData.otp_exist = true;
        mOtpData.otp_type = otpType;
        mOtpData.otp_size = otpSize;
        memcpy(mOtpData.otp_data, metadata.find(ANDROID_SPRD_OTP_DATA).data.u8,
               otpSize);
    }

    HAL_LOGI("X");

    return fwk_metadata;
}

/*===========================================================================
 * FUNCTION   :saveRequest
 *
 * DESCRIPTION: save buffer in request
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3DualFaceId::saveRequest(camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;

    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (CALLBACK_STREAM == getStreamType(newStream)) {
            multi_request_saved_t prevRequest;
            HAL_LOGV("save request num=%d", request->frame_number);

            Mutex::Autolock l(mRequestLock);
            prevRequest.frame_number = request->frame_number;
            prevRequest.buffer = request->output_buffers[i].buffer;
            prevRequest.preview_stream = request->output_buffers[i].stream;
            prevRequest.input_buffer = request->input_buffer;
            mSavedRequestList.push_back(prevRequest);
        }
    }
}

/*===========================================================================
 * FUNCTION   :processCaptureRequest
 *
 * DESCRIPTION: process Capture Request
 *
 * PARAMETERS :
 *    @device: camera3 device
 *    @request:camera3 request
 * RETURN     :
 *==========================================================================*/
int SprdCamera3DualFaceId::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    uint32_t tagCnt = 0;
    uint64_t get_reg_phyaddr = 0;
    uint64_t get_unlock_phyaddr[2] = {0, 0};
    camera3_capture_request_t *req = NULL;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    camera3_stream_t *preview_stream = NULL;
    CameraMetadata metaSettingsMain, metaSettingsAux;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;

    metaSettingsMain = request->settings;
    metaSettingsAux = request->settings;

    req = request;
    rc = validateCaptureRequest(req);
    if (NO_ERROR != rc) {
        return rc;
    }
    saveRequest(req);

    memset(&req_main, 0x00, sizeof(camera3_capture_request_t));
    memset(&req_aux, 0x00, sizeof(camera3_capture_request_t));

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType = getStreamType(req->output_buffers[i].stream);
        if (requestStreamType == CALLBACK_STREAM) {
            preview_stream = (req->output_buffers[i]).stream;
        }
    }

    tagCnt = metaSettingsMain.entryCount();
    if (0 != tagCnt) {
        if (metaSettingsMain.exists(ANDROID_SPRD_BURSTMODE_ENABLED)) {
            uint8_t sprdBurstModeEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                    &sprdBurstModeEnabled, 1);
            metaSettingsAux.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                   &sprdBurstModeEnabled, 1);
        }
        if (metaSettingsMain.exists(ANDROID_SPRD_ZSL_ENABLED)) {
            uint8_t sprdZslEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled,
                                    1);
            metaSettingsAux.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled,
                                   1);
        }
        if (metaSettingsMain.exists(ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE)) {
            uint8_t sprdFaceAttributesEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE, &sprdFaceAttributesEnabled,
                                    1);
            metaSettingsAux.update(ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE, &sprdFaceAttributesEnabled,
                                   1);
        }
    }
    // get phy addr from faceidservice
    if (metaSettingsMain.exists(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)) {
        get_reg_phyaddr =
            (uint64_t)
                metaSettingsMain.find(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)
                    .data.i64[0];
        for (i = 0; i < DUAL_FACEID_BUFFER_SUM; i++) {
            if (get_reg_phyaddr == (uint64_t)mLocalBufferMain[i].phy_addr) {
                // get phy addr
                HAL_LOGD("frame:%d,main get phy addr from faceidservice,0x%x",
                         request->frame_number, get_reg_phyaddr);
                mLocalBufferMain[i].type = (camera_buffer_type_t)MAIN_BUFFER;
                mLocalBufferListMain.push_back(&mLocalBufferMain[i]);
            }
        }
        get_reg_phyaddr =
            (uint64_t)
                metaSettingsMain.find(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)
                    .data.i64[1];
        for (i = 0; i < DUAL_FACEID_BUFFER_SUM; i++) {
            if (get_reg_phyaddr == (uint64_t)mLocalBufferAux[i].phy_addr) {
                // get phy addr
                HAL_LOGD("frame:%d,sub get phy addr from faceidservice,0x%x",
                         request->frame_number, get_reg_phyaddr);
                mLocalBufferAux[i].type = (camera_buffer_type_t)AUX_BUFFER;
                mLocalBufferListAux.push_back(&mLocalBufferAux[i]);
            }
        }
    }
    req_main = *req;
    req_aux = *req;
    req_main.settings = metaSettingsMain.release();
    req_aux.settings = metaSettingsAux.release();

    if (!mLocalBufferListMain.empty()) {
        camera3_stream_buffer_t out_streams_main[2];
        camera3_stream_buffer_t out_streams_aux[1];
        memset(out_streams_main, 0, sizeof(camera3_stream_buffer_t) * 2);
        memset(out_streams_aux, 0, sizeof(camera3_stream_buffer_t) * 1);
        // construct preview buffer
        i = 0;
        out_streams_main[i] = req->output_buffers[0];
        out_streams_main[i].stream = &mMainStreams[i];
        if (mFaceMode == MODE_DUAL_FACEID_UNLOCK) {
            out_streams_main[i].buffer = popBufferList(
                mLocalBufferListMain, (camera_buffer_type_t)MAIN_BUFFER);
        }
        out_streams_main[i].status = req->output_buffers->status;
        out_streams_main[i].release_fence = -1;
        // create callback buffer
        if (mFaceMode == MODE_DUAL_FACEID_REGISTER) {
            i++;
            out_streams_main[i].stream = &mMainStreams[i];
            out_streams_main[i].buffer = popBufferList(
                mLocalBufferListMain, (camera_buffer_type_t)MAIN_BUFFER);
            if (NULL == out_streams_main[i].buffer) {
                HAL_LOGE("failed, mLocalBufferListMain is empty");
                goto req_fail;
            }
            out_streams_main[i].status = req->output_buffers->status;
            out_streams_main[i].acquire_fence = -1;
            out_streams_main[i].release_fence = -1;
        }

        // construct main output_buffers buffer
        req_main.num_output_buffers = i + 1;
        req_main.output_buffers = out_streams_main;
        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }
        i = 0;
        out_streams_aux[i] = *req->output_buffers;
        out_streams_aux[i].stream = &mAuxStreams[i];
        out_streams_aux[i].status = req->output_buffers->status;
        out_streams_aux[i].acquire_fence = -1;
        out_streams_aux[i].release_fence = -1;

        out_streams_aux[i].buffer = popBufferList(
            mLocalBufferListAux, (camera_buffer_type_t)AUX_BUFFER);
        if (NULL == out_streams_aux[i].buffer) {
            HAL_LOGE("failed, mLocalBufferListAux is empty");
            goto req_fail;
        }
        // construct aux output_buffers buffer
        req_aux.num_output_buffers = 1;
        req_aux.output_buffers = out_streams_aux;

        rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,
                                             &req_aux);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_aux.frame_number);
            goto req_fail;
        }

        struct timespec t1;
        clock_gettime(CLOCK_BOOTTIME, &t1);
        mReqTimestamp = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
        mMaxPendingCount =
            preview_stream->max_buffers - MAX_UNMATCHED_QUEUE_SIZE + 1;
        {
            Mutex::Autolock l(mPendingLock);
            size_t pendingCount = 0;
            mPendingRequest++;
            HAL_LOGV("mPendingRequest=%d, mMaxPendingCount=%d", mPendingRequest,
                     mMaxPendingCount);
            while (mPendingRequest >= mMaxPendingCount) {
                mRequestSignal.waitRelative(mPendingLock, PENDINGTIME);
                if (pendingCount > (PENDINGTIMEOUT / PENDINGTIME)) {
                    HAL_LOGD("m_PendingRequest=%d", mPendingRequest);
                    rc = -ENODEV;
                    break;
                }
                pendingCount++;
            }
        }
    } else {
        camera3_stream_buffer_t out_streams_main[1];
        camera3_stream_buffer_t out_streams_aux[1];
        // just construct main preview buffer
        i = 0;
        out_streams_main[i] = *req->output_buffers;
        out_streams_main[i].stream = &mMainStreams[i];

        req_main.num_output_buffers = 1;
        req_main.output_buffers = out_streams_main;
        HAL_LOGD("frame:%d only preview", request->frame_number);

        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }
    }

req_fail:
    if (req_main.settings)
        free_camera_metadata((camera_metadata_t *)req_main.settings);

    if (req_aux.settings)
        free_camera_metadata((camera_metadata_t *)req_aux.settings);

    return rc;
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION: device notify
 *
 * PARAMETERS : notify message
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3DualFaceId::notifyMain(const camera3_notify_msg_t *msg) {
    mCallbackOps->notify(mCallbackOps, msg);
}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the main hwi
 *
 * PARAMETERS : capture result structure from hwi
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3DualFaceId::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    metadata = result->result;
    int i = 0;
    // meta process
    if (NULL == result_buffer && NULL != result->result) {
        if (!mResultBufferListMain.empty() && !mResultBufferListAux.empty()) {
            int64_t main_phyaddr;
            int64_t aux_phyaddr;
            List<new_mem_t *>::iterator it;
            new_mem_t *main_mem = NULL;
            new_mem_t *aux_mem = NULL;
            {
                Mutex::Autolock l(mMainLock);
                it = mResultBufferListMain.begin();
                main_mem = *it;
                mResultBufferListMain.erase(it);
            }
            main_phyaddr = (int64_t)main_mem->phy_addr;
            {
                Mutex::Autolock l(mAuxLock);
                it = mResultBufferListAux.begin();
                aux_mem = *it;
                mResultBufferListAux.erase(it);
            }
            aux_phyaddr = (int64_t)aux_mem->phy_addr;
            int64_t phyaddr[3] = {main_phyaddr, aux_phyaddr,
                                  (int64_t)mOtpLocalBuffer.phy_addr};
            HAL_LOGD("frame(%d):phyaddr 0x%lx,0x%lx", cur_frame_number,
                     (uint64_t)main_phyaddr, (uint64_t)aux_phyaddr);
            metadata.update(ANDROID_SPRD_TO_FACEIDSERVICE_PHYADDR, phyaddr, 3);

            camera3_capture_result_t new_result = *result;
            new_result.result = metadata.release();
            mCallbackOps->process_capture_result(mCallbackOps, &new_result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(new_result.result));
        } else {
            mCallbackOps->process_capture_result(mCallbackOps, result);
        }
        return;
    }
    if (result_buffer == NULL) {
        HAL_LOGE("result_buffer = result->output_buffers is NULL");
        return;
    }
    int currStreamType = getStreamType(result_buffer->stream);
    int result_buf_fd = ADP_BUFFD(*result_buffer->buffer);
    if ((DEFAULT_STREAM == currStreamType) ||
        (mFaceMode == MODE_DUAL_FACEID_UNLOCK &&
         CALLBACK_STREAM == currStreamType)) {
        for (i = 0; i < DUAL_FACEID_BUFFER_SUM; i++) {
            int saved_buf_fd = ADP_BUFFD(mLocalBufferMain[i].native_handle);
            if (result_buf_fd == saved_buf_fd) {
                Mutex::Autolock l(mMainLock);
                mResultBufferListMain.push_back(&mLocalBufferMain[i]);
            }
        }
    }
    if (DEFAULT_STREAM == currStreamType) {
        HAL_LOGV("return default");
        return;
    } else if (CALLBACK_STREAM == currStreamType) {
        // preview process
        HAL_LOGV("preview process");
        CallBackResult(result->frame_number, (camera3_buffer_status_t)(result->output_buffers->status));
    }
}

/*===========================================================================
 * FUNCTION   :CallBackResult
 *
 * DESCRIPTION: CallBackResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3DualFaceId::CallBackResult(
    uint32_t frame_number, camera3_buffer_status_t buffer_status) {
    camera3_capture_result_t result;
    List<multi_request_saved_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;

    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    {
        Mutex::Autolock l(mRequestLock);
        itor = mSavedRequestList.begin();
        while (itor != mSavedRequestList.end()) {
            if (itor->frame_number == frame_number) {
                HAL_LOGD("erase frame_number %u", frame_number);
                result_buffers.stream = itor->preview_stream;
                result_buffers.buffer = itor->buffer;
                mSavedRequestList.erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mSavedRequestList.end()) {
            HAL_LOGE("can't find frame in mSavedRequestList %u:", frame_number);
            return;
        }
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

    {
        Mutex::Autolock l(mPendingLock);
        mPendingRequest--;
        if (mPendingRequest < mMaxPendingCount) {
            HAL_LOGV("signal request m_PendingRequest = %d", mPendingRequest);
            mRequestSignal.signal();
        }
    }

    HAL_LOGD("frame number=%d buffer status=%u", result.frame_number,
             buffer_status);
}

/*===========================================================================
 * FUNCTION   :processCaptureResultAux
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3DualFaceId::processCaptureResultAux(
    camera3_capture_result_t *result) {
    if (result->output_buffers == NULL) {
        return;
    }
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    int i = 0;
    int currStreamType = getStreamType(result_buffer->stream);
    int result_buf_fd = ADP_BUFFD(*result_buffer->buffer);
    HAL_LOGD("result process frameid %d", cur_frame_number);
    // callback process
    if (CALLBACK_STREAM == currStreamType) {
        for (i = 0; i < DUAL_FACEID_BUFFER_SUM; i++) {
            int saved_buf_fd = ADP_BUFFD(mLocalBufferAux[i].native_handle);
            if (result_buf_fd == saved_buf_fd) {
                Mutex::Autolock l(mAuxLock);
                mResultBufferListAux.push_back(&mLocalBufferAux[i]);
            }
        }
    }
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3DualFaceId::_dump(const struct camera3_device *device, int fd) {
    HAL_LOGD(" E");

    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION: flush
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3DualFaceId::_flush(const struct camera3_device *device) {
    HAL_LOGD("E");

    int rc = 0;

    mFlushing = true;

    // flush main camera
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);

    HAL_LOGD("X");

    return rc;
}
};
