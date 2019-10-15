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
#define LOG_TAG "Cam33DFaceId"
#include "SprdCamera33DFaceId.h"

using namespace android;
namespace sprdcamera {

#define PENDINGTIME (1000000)
#define PENDINGTIMEOUT (5000000000)

#define MASTER_ID 0

SprdCamera33DFaceId *m3DFaceId = NULL;
static int g_raw_frame_dump_cnt = 0;
static int g_yuv_frame_dump_cnt = 0;

// Error Check Macros
#define CHECK_FACEID()                                                         \
    if (!m3DFaceId) {                                                            \
        HAL_LOGE("Error getting faceid");                                      \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_FACEID_ERROR()                                                   \
    if (!m3DFaceId) {                                                            \
        HAL_LOGE("Error getting m3DFaceId");                                     \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera33DFaceId::mCameraCaptureOps = {
    .initialize = SprdCamera33DFaceId::initialize,
    .configure_streams = SprdCamera33DFaceId::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera33DFaceId::construct_default_request_settings,
    .process_capture_request = SprdCamera33DFaceId::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera33DFaceId::dump,
    .flush = SprdCamera33DFaceId::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera33DFaceId::callback_ops_main = {
    .process_capture_result =
        SprdCamera33DFaceId::process_capture_result_main,
    .notify = SprdCamera33DFaceId::notifyMain};

camera3_callback_ops SprdCamera33DFaceId::callback_ops_aux_L = {
    .process_capture_result = SprdCamera33DFaceId::process_capture_result_aux_L,
    .notify = SprdCamera33DFaceId::notifyAuxL};

camera3_callback_ops SprdCamera33DFaceId::callback_ops_aux_R = {
    .process_capture_result = SprdCamera33DFaceId::process_capture_result_aux_R,
    .notify = SprdCamera33DFaceId::notifyAuxR};

/*===========================================================================
 * FUNCTION   : SprdCamera3FaceIdUnlock
 *
 * DESCRIPTION: SprdCamera3FaceIdUnlock Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera33DFaceId::SprdCamera33DFaceId() {
    HAL_LOGV("E");

    m_pPhyCamera = NULL;
    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = (uint8_t)CAM_FACE_MAIN_ID;
    mStaticMetadata = NULL;
    mPhyCameraNum = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mIRWidth = 0;
    mIRHeight = 0;
    mReqTimestamp = 0;
    mMaxPendingCount = 0;
    mPendingRequest = 0;
    mFlushing = false;
    mSavedRequestList.clear();
    mLocalBufferListMain.clear();
    mLocalBufferListAuxL.clear();
    mLocalBufferListAuxR.clear();
    mResultBufferListMain.clear();
    mResultBufferListAuxL.clear();
    mResultBufferListAuxR.clear();
    mLocalBufferListToFaceAddr.clear();
    memset(mLocalBufferMain, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);
    memset(mLocalBufferAuxL, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);
    memset(mLocalBufferAuxR, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);
    memset(mMainStreams, 0, sizeof(camera3_stream_t) * THREE_D_FACEID_MAX_STREAMS);
    memset(mAuxStreamsL, 0, sizeof(camera3_stream_t) * (THREE_D_FACEID_MAX_STREAMS - 1));
    memset(mAuxStreamsR, 0, sizeof(camera3_stream_t) * (THREE_D_FACEID_MAX_STREAMS - 1));
    memset(&mOtpData, 0, sizeof(OtpData));
    memset(&mOtpLocalBuffer, 0, sizeof(new_mem_t));
    memset(&mToFaceIDServiceBuffer, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);

    HAL_LOGV("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3FaceIdUnlock
 *
 * DESCRIPTION: SprdCamera3FaceIdUnlock Desctructor
 *
 *==========================================================================*/
SprdCamera33DFaceId::~SprdCamera33DFaceId() {
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
void SprdCamera33DFaceId::getCameraFaceId(SprdCamera33DFaceId **pFaceid) {
    if (!m3DFaceId) {
        m3DFaceId = new SprdCamera33DFaceId();
    }

    CHECK_FACEID();
    *pFaceid = m3DFaceId;
    HAL_LOGV("m3DFaceId=%p", m3DFaceId);

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
int SprdCamera33DFaceId::get_camera_info(__unused int camera_id,
                                           struct camera_info *info) {
    HAL_LOGD("E");

    int rc = NO_ERROR;

    if (info) {
        rc = m3DFaceId->getCameraInfo(camera_id, info);
    }

    HAL_LOGD("X, rc=%d", rc);

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
int SprdCamera33DFaceId::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    HAL_LOGD("id=%d", atoi(id));

    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = m3DFaceId->cameraDeviceOpen(atoi(id), hw_device);

    HAL_LOGD("id=%d, rc: %d", atoi(id), rc);

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
int SprdCamera33DFaceId::close_camera_device(__unused hw_device_t *hw_dev) {
    if (NULL == hw_dev) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return m3DFaceId->closeCameraDevice();
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
int SprdCamera33DFaceId::closeCameraDevice() {
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
        HAL_LOGD("camera id:%d", i);
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

    mLocalBufferListAuxL.clear();
    mLocalBufferListAuxR.clear();
    mResultBufferListMain.clear();
    mResultBufferListAuxL.clear();
    mResultBufferListAuxR.clear();
    mLocalBufferListToFaceAddr.clear();
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
int SprdCamera33DFaceId::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGD("E");

    int rc = NO_ERROR;

    CHECK_FACEID_ERROR();
    rc = m3DFaceId->initialize(callback_ops);

    HAL_LOGD("X");

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
SprdCamera33DFaceId::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    HAL_LOGV("E");

    const camera_metadata_t *rc;

    if (!m3DFaceId) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }

    rc = m3DFaceId->constructDefaultRequestSettings(device, type);

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
int SprdCamera33DFaceId::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    int rc = 0;

    CHECK_FACEID_ERROR();
    rc = m3DFaceId->configureStreams(device, stream_list);

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
int SprdCamera33DFaceId::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    HAL_LOGV("E");

    int rc = 0;

    CHECK_FACEID_ERROR();
    rc = m3DFaceId->processCaptureRequest(device, request);

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
void SprdCamera33DFaceId::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("frame number=%d", result->frame_number);

    CHECK_FACEID();
    m3DFaceId->processCaptureResultMain((camera3_capture_result_t *)result);
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
void SprdCamera33DFaceId::notifyMain(const struct camera3_callback_ops *ops,
                                       const camera3_notify_msg_t *msg) {
    HAL_LOGV("frame number=%d", msg->message.shutter.frame_number);

    CHECK_FACEID();
    m3DFaceId->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_auxL
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdUnlock
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera33DFaceId::process_capture_result_aux_L(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("frame number=%d", result->frame_number);

    CHECK_FACEID();
    m3DFaceId->processCaptureResultAuxL((camera3_capture_result_t *)result);
}

/*===========================================================================
 * FUNCTION   :notifyAuxL
 *
 * DESCRIPTION:  main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera33DFaceId::notifyAuxL(const struct camera3_callback_ops *ops,
                                      const camera3_notify_msg_t *msg) {
    // This mode do not have real aux notify
    CHECK_FACEID();
}


/*===========================================================================
 * FUNCTION   :process_capture_result_aux_R
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdUnlock
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera33DFaceId::process_capture_result_aux_R(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("frame number=%d", result->frame_number);

    CHECK_FACEID();
    m3DFaceId->processCaptureResultAuxR((camera3_capture_result_t *)result);
}

/*===========================================================================
 * FUNCTION   :notifyAuxR
 *
 * DESCRIPTION:  main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera33DFaceId::notifyAuxR(const struct camera3_callback_ops *ops,
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
void SprdCamera33DFaceId::freeLocalCapBuffer() {
    HAL_LOGV("E");

    size_t buffer_num = THREE_D_FACEID_BUFFER_SUM;
    for (size_t i = 0; i < buffer_num; i++) {
        freeOneBuffer(&mLocalBufferMain[i]);
        freeOneBuffer(&mLocalBufferAuxL[i]);
        freeOneBuffer(&mLocalBufferAuxR[i]);
        freeOneBuffer(&mToFaceIDServiceBuffer[i]);
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
int SprdCamera33DFaceId::cameraDeviceOpen(int camera_id,
                                            struct hw_device_t **hw_device) {
    HAL_LOGD("E");

    int rc = NO_ERROR;
    int i = 0;
    uint32_t Phy_id = 0;

    if ((SPRD_3D_FACEID_REGISTER_ID == camera_id) ||
        (SPRD_3D_FACEID_UNLOCK_ID == camera_id)) {
        mPhyCameraNum = 3;
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

        if (SPRD_3D_FACEID_REGISTER_ID == camera_id) {
            hw->setMultiCameraMode(MODE_3D_FACEID_REGISTER);
            mFaceMode = MODE_3D_FACEID_REGISTER;
        } else if (SPRD_3D_FACEID_UNLOCK_ID == camera_id) {
            hw->setMultiCameraMode(MODE_3D_FACEID_UNLOCK);
            mFaceMode = MODE_3D_FACEID_UNLOCK;
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

    g_raw_frame_dump_cnt = 0;
    g_yuv_frame_dump_cnt = 0;
    HAL_LOGD("X");
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
int SprdCamera33DFaceId::getCameraInfo(int face_camera_id,
                                         struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;

    HAL_LOGD("camera_id=%d", face_camera_id);

    m_VirtualCamera.id = CAM_MAIN_3D_FACE_ID;
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
int SprdCamera33DFaceId::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[mPhyCameraNum];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }

    memset(m_pPhyCamera, 0x00,
           (mPhyCameraNum * sizeof(sprdcamera_physical_descriptor_t)));

    m_pPhyCamera[CAM_TYPE_MAIN].id = (uint8_t)CAM_MAIN_3D_FACE_ID;
    m_pPhyCamera[CAM_TYPE_AUX1].id = (uint8_t)CAM_AUX_FACE_ID_L;
    m_pPhyCamera[CAM_TYPE_AUX2].id = (uint8_t)CAM_AUX_FACE_ID_R;

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
void SprdCamera33DFaceId::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    CHECK_FACEID();

    m3DFaceId->_dump(device, fd);

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
int SprdCamera33DFaceId::flush(const struct camera3_device *device) {
    HAL_LOGV("E");

    int rc = 0;

    CHECK_FACEID_ERROR();
    rc = m3DFaceId->_flush(device);

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
int SprdCamera33DFaceId::initialize(
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGV("E");

    int rc = NO_ERROR;
    mFlushing = false;
    mLocalBufferListMain.clear();
    mLocalBufferListAuxL.clear();
    mLocalBufferListAuxR.clear();
    mResultBufferListMain.clear();
    mResultBufferListAuxL.clear();
    mResultBufferListAuxR.clear();
    mLocalBufferListToFaceAddr.clear();
    memset(&mOtpLocalBuffer, 0, sizeof(new_mem_t));
    memset(&mToFaceIDServiceBuffer, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);

    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiMain);
    SprdCamera3MultiBase::initialize(MODE_3D_FACEID_UNLOCK, hwiMain);

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (NO_ERROR != rc) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_TYPE_AUX1];
    SprdCamera3HWI *hwiAuxL = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAuxL);

    rc = hwiAuxL->initialize(sprdCam.dev, &callback_ops_aux_L);
    if (NO_ERROR != rc) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }


    sprdCam = m_pPhyCamera[CAM_TYPE_AUX2];
    SprdCamera3HWI *hwiAuxR = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAuxR);
    rc = hwiAuxR->initialize(sprdCam.dev, &callback_ops_aux_R);
    if (NO_ERROR != rc) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    memset(mLocalBufferMain, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);
    memset(mLocalBufferAuxL, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);
    memset(mLocalBufferAuxR, 0, sizeof(new_mem_t) * THREE_D_FACEID_BUFFER_SUM);
    memset(mMainStreams, 0, sizeof(camera3_stream_t) * THREE_D_FACEID_MAX_STREAMS);
    memset(mAuxStreamsL, 0, sizeof(camera3_stream_t) * (THREE_D_FACEID_MAX_STREAMS - 1));
    memset(mAuxStreamsR, 0, sizeof(camera3_stream_t) * (THREE_D_FACEID_MAX_STREAMS - 1));
    mCallbackOps = callback_ops;

    HAL_LOGV("X");

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
int SprdCamera33DFaceId::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGD("E");

    int rc = 0;
    camera3_stream_t *pmainStreams[THREE_D_FACEID_MAX_STREAMS];
    camera3_stream_t *pauxStreamsL[THREE_D_FACEID_MAX_STREAMS - 1];
    camera3_stream_t *pauxStreamsR[THREE_D_FACEID_MAX_STREAMS - 1];
    camera3_stream_t *previewStream = NULL;
    size_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAuxL = m_pPhyCamera[CAM_TYPE_AUX1].hwi;
    SprdCamera3HWI *hwiAuxR = m_pPhyCamera[CAM_TYPE_AUX2].hwi;
    mReqTimestamp = 0;
    mMaxPendingCount = 0;
    mPendingRequest = 0;
    Mutex::Autolock l(mLock);

    HAL_LOGD("FACEID , num_streams=%d", stream_list->num_streams);
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD("%d: stream type=%d, width=%d, height=%d, "
                 "format=%d usage = 0x%x",
                 i, stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height,
                 stream_list->streams[i]->format,
                 stream_list->streams[i]->usage);

        int requestStreamType = getStreamType(stream_list->streams[i]);
        if (PREVIEW_STREAM == requestStreamType) {
            previewStream = stream_list->streams[i];
            mPreviewWidth = stream_list->streams[i]->width;
            mPreviewHeight = stream_list->streams[i]->height;
            mMainStreams[i] = *stream_list->streams[i];

            mIRWidth = THREE_D_FACEID_WIDTH;
            mIRHeight = THREE_D_FACEID_HEIGHT;
            mAuxStreamsL[i] = *stream_list->streams[i];
            mAuxStreamsL[i].format = HAL_PIXEL_FORMAT_RAW16;
            mAuxStreamsL[i].width = mIRWidth;
            mAuxStreamsL[i].height = mIRHeight;

            mAuxStreamsR[i] = *stream_list->streams[i];
            mAuxStreamsR[i].format = HAL_PIXEL_FORMAT_RAW16;
            mAuxStreamsR[i].width = mIRWidth;
            mAuxStreamsR[i].height = mIRHeight;

            pmainStreams[i] = &mMainStreams[i];
            pauxStreamsL[i] = &mAuxStreamsL[i];
            pauxStreamsR[i] = &mAuxStreamsR[i];
        } else {
           // for callback buffer
           mMainStreams[i] = *stream_list->streams[i];
           mMainStreams[i].stream_type = CAMERA3_STREAM_OUTPUT;
           mMainStreams[i].width = mPreviewWidth;
           mMainStreams[i].height = mPreviewHeight;
           mMainStreams[i].format = HAL_PIXEL_FORMAT_YCbCr_420_888;
           mMainStreams[i].usage = stream_list->streams[i]->usage;
           mMainStreams[i].max_buffers = 1;
           mMainStreams[i].data_space = stream_list->streams[i]->data_space;
           mMainStreams[i].rotation = stream_list->streams[i]->rotation;
        }
           pmainStreams[i] = &mMainStreams[i];
    }

    freeLocalCapBuffer();
    for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
        if (0 > allocateOne(mPreviewWidth, mPreviewHeight,
                            &(mLocalBufferMain[i]), YUV420)) {

            HAL_LOGE("request one buf main failed.");
        }
        mLocalBufferMain[i].type = (camera_buffer_type_t)MAIN_BUFFER_3D;
        mLocalBufferListMain.push_back(&mLocalBufferMain[i]);

        if (0 > allocateOne(mIRWidth, mIRHeight,
                            &(mLocalBufferAuxL[i]), YUV420)) {
            HAL_LOGE("request one buf AuxL failed.");
        }
        mLocalBufferAuxL[i].type = (camera_buffer_type_t)AUX_BUFFER_L;
        mLocalBufferListAuxL.push_back(&mLocalBufferAuxL[i]);

        if (0 > allocateOne(mIRWidth, mIRHeight,
                            &(mLocalBufferAuxR[i]), YUV420)) {
            HAL_LOGE("request one buf AuxR failed.");
        }
        mLocalBufferAuxR[i].type = (camera_buffer_type_t)AUX_BUFFER_R;
        mLocalBufferListAuxR.push_back(&mLocalBufferAuxR[i]);
    }

    if (0 > allocateOne(mIRWidth, mIRHeight, &mOtpLocalBuffer, YUV420)) {
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

    for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
        if (0 > allocateOne(mIRWidth/4, mIRHeight/4, &mToFaceIDServiceBuffer[i], YUV420)) {
            HAL_LOGE("request one buf failed.");
        }
        mToFaceIDServiceBuffer[i].type = (camera_buffer_type_t)FACE_ADDR_BUFFER;
        mLocalBufferListToFaceAddr.push_back(&mToFaceIDServiceBuffer[i]);
    }

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;

    mainconfig.num_streams = THREE_D_FACEID_MAX_STREAMS;

    mainconfig.streams = pmainStreams;
    for (i = 0; i < mainconfig.num_streams; i++) {
        HAL_LOGD("stream_type=%d, width=%d, height=%d, format=%d",
                 pmainStreams[i]->stream_type, pmainStreams[i]->width,
                 pmainStreams[i]->height, pmainStreams[i]->format);
    }

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    camera3_stream_configuration auxconfigL;
    auxconfigL = *stream_list;
    auxconfigL.num_streams = THREE_D_FACEID_MAX_STREAMS - 1;
    auxconfigL.streams = pauxStreamsL;

    rc = hwiAuxL->configure_streams(m_pPhyCamera[CAM_TYPE_AUX1].dev, &auxconfigL);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    camera3_stream_configuration auxconfigR;
    auxconfigR = *stream_list;
    auxconfigR.num_streams = THREE_D_FACEID_MAX_STREAMS - 1;
    auxconfigR.streams = pauxStreamsR;

    rc = hwiAuxR->configure_streams(m_pPhyCamera[CAM_TYPE_AUX2].dev, &auxconfigR);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    for (i = 0; i < mainconfig.num_streams; i++) {

        memcpy(stream_list->streams[i], &mMainStreams[i], sizeof(camera3_stream_t));

        HAL_LOGD(
            "atfer configure treams: stream_type=%d, "
            "width=%d, height=%d, format=%d, usage=0x%x, "
            "max_buffers=%d, data_space=%d, rotation=%d",
            stream_list->streams[i]->stream_type,
            stream_list->streams[i]->width, stream_list->streams[i]->height,
            stream_list->streams[i]->format, stream_list->streams[i]->usage,
            stream_list->streams[i]->max_buffers,
            stream_list->streams[i]->data_space,
            stream_list->streams[i]->rotation);
    }
    HAL_LOGD("X");

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
const camera_metadata_t *SprdCamera33DFaceId::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGD("E");

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
        HAL_LOGD("otpType %d, otpSize %d", otpType, otpSize);
        mOtpData.otp_exist = true;
        mOtpData.otp_type = otpType;
        mOtpData.otp_size = otpSize;
        memcpy(mOtpData.otp_data, metadata.find(ANDROID_SPRD_OTP_DATA).data.u8,
               otpSize);
    }

    HAL_LOGD("X");

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
void SprdCamera33DFaceId::saveRequest(camera3_capture_request_t *request) {
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
int SprdCamera33DFaceId::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    uint32_t tagCnt = 0;
    uint64_t get_reg_phyaddr = 0;
    uint64_t get_unlock_phyaddr[2] = {0, 0};
    camera3_capture_request_t *req = NULL;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux_L;
    camera3_capture_request_t req_aux_R;
    camera3_stream_t *new_stream = NULL;
    void * arrAddr = NULL;
    uint64_t phyaddr[5] = {0};

    CameraMetadata metaSettingsMain, metaSettingsAuxL, metaSettingsAuxR;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAuxL = m_pPhyCamera[CAM_TYPE_AUX1].hwi;
    SprdCamera3HWI *hwiAuxR = m_pPhyCamera[CAM_TYPE_AUX2].hwi;

    metaSettingsMain = request->settings;
    metaSettingsAuxL = request->settings;
    metaSettingsAuxR = request->settings;

    req = request;
    rc = validateCaptureRequest(req);
    if (NO_ERROR != rc) {
        return rc;
    }
    saveRequest(req);

    memset(&req_main, 0x00, sizeof(camera3_capture_request_t));
    memset(&req_aux_L, 0x00, sizeof(camera3_capture_request_t));
    memset(&req_aux_R, 0x00, sizeof(camera3_capture_request_t));

    tagCnt = metaSettingsMain.entryCount();
    if (0 != tagCnt) {
        if (metaSettingsMain.exists(ANDROID_SPRD_BURSTMODE_ENABLED)) {
            uint8_t sprdBurstModeEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                    &sprdBurstModeEnabled, 1);
            metaSettingsAuxL.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                   &sprdBurstModeEnabled, 1);
            metaSettingsAuxR.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                   &sprdBurstModeEnabled, 1);
        }
        if (metaSettingsMain.exists(ANDROID_SPRD_ZSL_ENABLED)) {
            uint8_t sprdZslEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
            metaSettingsAuxL.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
            metaSettingsAuxR.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
        }
        if (metaSettingsMain.exists(ANDROID_STATISTICS_FACE_DETECT_MODE)) {
            uint8_t fd_mode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
            metaSettingsAuxL.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &fd_mode, 1);
            metaSettingsAuxR.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &fd_mode, 1);
        } else {
            HAL_LOGW("ANDROID_STATISTICS_FACE_DETECT_MODE ### not exist");
        }
    }
    // get phy addr from faceidservice
    if (metaSettingsMain.exists(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)) {
        int j = 0;
        get_reg_phyaddr =
            (uint64_t)
                metaSettingsMain.find(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)
                    .data.i64[0];
        for (j = 0; j < THREE_D_FACEID_BUFFER_SUM; j++) {
            if (get_reg_phyaddr == (uint64_t)mToFaceIDServiceBuffer[j].phy_addr) {
                break;
            }
        }

        map(&mToFaceIDServiceBuffer[j].native_handle, &arrAddr);
        memcpy(phyaddr, arrAddr, 5 * sizeof(int64_t));

        for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
            if (phyaddr[0] == (uint64_t)mLocalBufferAuxL[i].phy_addr) {
                mLocalBufferAuxL[i].type = (camera_buffer_type_t)AUX_BUFFER_L;
                mLocalBufferListAuxL.push_back(&mLocalBufferAuxL[i]);

            }
        }
        for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
            if (phyaddr[1] == (uint64_t)mLocalBufferAuxR[i].phy_addr) {
                mLocalBufferAuxR[i].type = (camera_buffer_type_t)AUX_BUFFER_R;
                mLocalBufferListAuxR.push_back(&mLocalBufferAuxR[i]);
            }
        }
        for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
            if (phyaddr[2] == (uint64_t)mLocalBufferMain[i].phy_addr) {
                mLocalBufferMain[i].type = (camera_buffer_type_t)MAIN_BUFFER_3D;
                mLocalBufferListMain.push_back(&mLocalBufferMain[i]);
            }
        }
        unmap(&mToFaceIDServiceBuffer[j].native_handle);
        {
            Mutex::Autolock l(mToFaceLock);
            mToFaceIDServiceBuffer[j].type = (camera_buffer_type_t)FACE_ADDR_BUFFER;
            mLocalBufferListToFaceAddr.push_back(&mToFaceIDServiceBuffer[j]);
        }
    }
    req_main = *req;
    req_aux_L = *req;
    req_aux_R = *req;
    req_main.settings = metaSettingsMain.release();
    req_aux_L.settings = metaSettingsAuxL.release();
    req_aux_R.settings = metaSettingsAuxR.release();

    if (!mLocalBufferListMain.empty()) {
        camera3_stream_buffer_t out_streams_main[2];
        camera3_stream_buffer_t out_streams_aux_L[1];
        camera3_stream_buffer_t out_streams_aux_R[1];
        memset(out_streams_main, 0, sizeof(camera3_stream_buffer_t) * 2);
        memset(out_streams_aux_L, 0, sizeof(camera3_stream_buffer_t) * 1);
        memset(out_streams_aux_R, 0, sizeof(camera3_stream_buffer_t) * 1);
        // construct preview buffer main camera
        i = 0;
        out_streams_main[i] = *req->output_buffers;
        out_streams_main[i].stream = &mMainStreams[i];

        out_streams_main[i].status = req->output_buffers->status;
        out_streams_main[i].release_fence = -1;
        // create callback buffer main camera
        i++;
        out_streams_main[i].stream = &mMainStreams[i];
        out_streams_main[i].buffer = popBufferList(
                        mLocalBufferListMain, (camera_buffer_type_t)MAIN_BUFFER_3D);
        if (NULL == out_streams_main[i].buffer) {
            HAL_LOGE("failed, mLocalBufferListMain is empty");
            goto req_fail;
        }
        out_streams_main[i].status = req->output_buffers->status;
        out_streams_main[i].acquire_fence = -1;
        out_streams_main[i].release_fence = -1;

        // construct main output_buffers buffer
        req_main.num_output_buffers = i + 1;
        req_main.output_buffers = out_streams_main;

        i = 0;
        out_streams_aux_L[i] = *req->output_buffers;
        out_streams_aux_L[i].stream = &mAuxStreamsL[i];
        out_streams_aux_L[i].status = req->output_buffers->status;
        out_streams_aux_L[i].acquire_fence = -1;
        out_streams_aux_L[i].release_fence = -1;

        out_streams_aux_L[i].buffer =
            popBufferList(mLocalBufferListAuxL, (camera_buffer_type_t)AUX_BUFFER_L);
        if (NULL == out_streams_aux_L[i].buffer) {
            HAL_LOGE("failed, mLocalBufferListAuxL is empty");
            goto req_fail;
        }

        out_streams_aux_R[i] = *req->output_buffers;
        out_streams_aux_R[i].stream = &mAuxStreamsR[i];
        out_streams_aux_R[i].status = req->output_buffers->status;
        out_streams_aux_R[i].acquire_fence = -1;
        out_streams_aux_R[i].release_fence = -1;

        out_streams_aux_R[i].buffer =
            popBufferList(mLocalBufferListAuxR, (camera_buffer_type_t)AUX_BUFFER_R);
        if (NULL == out_streams_aux_R[i].buffer) {
            HAL_LOGE("failed, mLocalBufferListAuxR is empty");
            goto req_fail;
        }
        // construct aux output_buffers buffer L
        req_aux_L.num_output_buffers = 1;
        req_aux_L.output_buffers = out_streams_aux_L;

        rc = hwiAuxL->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX1].dev,
                                             &req_aux_L);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_aux_L.frame_number);
            goto req_fail;
        }
        // construct aux output_buffers buffer R
        req_aux_R.num_output_buffers = 1;
        req_aux_R.output_buffers = out_streams_aux_R;

        rc = hwiAuxR->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX2].dev,
                                             &req_aux_R);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_aux_R.frame_number);
            goto req_fail;
        }

        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }


    } else {
        camera3_stream_buffer_t out_streams_main[1];
        // just construct main preview buffer
        i = 0;
        out_streams_main[i] = *req->output_buffers;
        out_streams_main[i].stream = &mMainStreams[i];

        req_main.num_output_buffers = 1;
        req_main.output_buffers = out_streams_main;

        HAL_LOGV("frame:%d only preview", request->frame_number);
        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }

    }

    struct timespec t1;
    clock_gettime(CLOCK_BOOTTIME, &t1);
    mReqTimestamp = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
    mMaxPendingCount = THREE_D_FACEID_BUFFER_SUM + 1;
    {
        Mutex::Autolock l(mPendingLock);
        size_t pendingCount = 0;
        mPendingRequest++;
        HAL_LOGV("mPendingRequest=%d, mMaxPendingCount=%d", mPendingRequest,
                 mMaxPendingCount);
        while (mPendingRequest >= mMaxPendingCount) {
            mRequestSignal.waitRelative(mPendingLock, PENDINGTIME);
            if (pendingCount > (PENDINGTIMEOUT / PENDINGTIME)) {
                HAL_LOGV("m_PendingRequest=%d", mPendingRequest);
                rc = -ENODEV;
                break;
            }
            pendingCount++;
        }
    }

req_fail:
    if (req_main.settings)
        free_camera_metadata((camera_metadata_t *)req_main.settings);

    if (req_aux_L.settings)
        free_camera_metadata((camera_metadata_t *)req_aux_L.settings);


    if (req_aux_R.settings)
        free_camera_metadata((camera_metadata_t *)req_aux_R.settings);
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
void SprdCamera33DFaceId::notifyMain(const camera3_notify_msg_t *msg) {
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
void SprdCamera33DFaceId::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    metadata = result->result;
    int i = 0;
    void * buffer_addr = NULL;

    // meta process
    if (NULL == result_buffer && NULL != result->result) {
        if (!mResultBufferListMain.empty() && !mResultBufferListAuxL.empty() &&
		    !mResultBufferListAuxR.empty()) {
            uint64_t main_phyaddr;
            uint64_t aux_phyaddr_L;
            uint64_t aux_phyaddr_R;
            List<new_mem_t *>::iterator it;
            new_mem_t *main_mem = NULL;
            new_mem_t *aux_mem_L = NULL;
            new_mem_t *aux_mem_R = NULL;
            new_mem_t *toface_addr_mem = NULL;
            {
                Mutex::Autolock l(mMainLock);
                it = mResultBufferListMain.begin();
                main_mem = *it;
                mResultBufferListMain.erase(it);
            }
            main_phyaddr = (int64_t)main_mem->phy_addr;
            {
                Mutex::Autolock l(mAuxLockL);
                it = mResultBufferListAuxL.begin();
                aux_mem_L = *it;
                mResultBufferListAuxL.erase(it);
            }
            aux_phyaddr_L = (int64_t)aux_mem_L->phy_addr;
            {
                Mutex::Autolock l(mAuxLockR);
                it = mResultBufferListAuxR.begin();
                aux_mem_R = *it;
                mResultBufferListAuxR.erase(it);
            }
            aux_phyaddr_R = (int64_t)aux_mem_R->phy_addr;


            //update  ANDROID_SPRD_3D_PARAM
            FACEID_IR_PARAM_T irParams;
            irParams.capture_width = mIRWidth;
            irParams.capture_height = mIRHeight;
            irParams.depth_width = mIRWidth;
            irParams.depth_height = mIRHeight;
            irParams.calib_width = mIRWidth;
            irParams.calib_height = mIRHeight;
            irParams.color_raw_width = mPreviewWidth;
            irParams.color_raw_height = mPreviewHeight;

            HAL_LOGD("IR size:(%d x %d), Color size:(%d x %d)", irParams.capture_width, irParams.capture_height,
                                                    irParams.color_raw_width, irParams.color_raw_height);
            metadata.update(ANDROID_SPRD_3D_PARAM, (unsigned char *)&irParams, sizeof(FACEID_IR_PARAM_T));
            if (!mLocalBufferListToFaceAddr.empty()) {
                {
                    Mutex::Autolock l(mToFaceLock);
                    it = mLocalBufferListToFaceAddr.begin();
                    toface_addr_mem = *it;
                    mLocalBufferListToFaceAddr.erase(it);
                }

                //fill mToFaceIDServiceBuffer
                map(&toface_addr_mem->native_handle, &buffer_addr);
                memcpy((void *)(char *)buffer_addr, (void*)&aux_phyaddr_L, sizeof(int64_t));
                memcpy((void *)((char *)buffer_addr + sizeof(int64_t)), (void*)&aux_phyaddr_R, sizeof(int64_t));
                memcpy((void *)((char *)buffer_addr + 2*sizeof(int64_t)), (void*)&main_phyaddr, sizeof(int64_t));
                memcpy((void *)((char *)buffer_addr + 3*sizeof(int64_t)), (void*)&(mOtpLocalBuffer.phy_addr), sizeof(int64_t));
                unmap(&toface_addr_mem->native_handle);

                int64_t phyaddr[3] = {(int64_t)toface_addr_mem->phy_addr, 0, 0};
                HAL_LOGV("frame(%d):phyaddr 0x%lx, 0x%lx, 0x%lx", cur_frame_number,
                     (uint64_t)main_phyaddr, (uint64_t)aux_phyaddr_L, (uint64_t)aux_phyaddr_R);
                metadata.update(ANDROID_SPRD_TO_FACEIDSERVICE_PHYADDR, phyaddr, 1);
            }

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

    int currStreamType = getStreamType(result_buffer->stream);
    int result_buf_fd = ADP_BUFFD(*result_buffer->buffer);
    if (DEFAULT_STREAM == currStreamType) {
        HAL_LOGV("callback process");
        for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
            int saved_buf_fd = ADP_BUFFD(mLocalBufferMain[i].native_handle);
            if (result_buf_fd == saved_buf_fd) {
                Mutex::Autolock l(mMainLock);
                processYUVData(&mLocalBufferMain[i],result->frame_number);
                mResultBufferListMain.push_back(&mLocalBufferMain[i]);
                break;
            }
        }
        return;
    }

    // preview process
    HAL_LOGV("preview process");
    CallBackResult(result->frame_number, CAMERA3_BUFFER_STATUS_OK);

    return;
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
void SprdCamera33DFaceId::CallBackResult(
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
 * FUNCTION   :raw10ToRaw8
 *
 * DESCRIPTION: process raw10 data to raw8
 *
 * PARAMETERS : src with data,and dest to store data translated.
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera33DFaceId::raw10ToRaw8(void *dest, void *src, int size) {
    if (dest == NULL || src == NULL || size == 0) {
        HAL_LOGE("para is null. dst %p, src %p, size %d", dest, src, size);
        return;
    }
    int i;
    char *p1 = (char *)src;
    char *p2 = (char *)dest;
    for (i = 0; i < size / 5; i++) {
        memcpy(p2, p1, 4 * sizeof(char));

        p1 += 5;
        p2 += 4;
    }

    HAL_LOGD(" success %d", i);
}

bool SprdCamera33DFaceId::Raw8Rotate90AndMirror(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                                      uint16_t a_uwSrcWidth,
                                      uint16_t a_uwSrcHeight, uint8_t angle) {
    if (a_ucDstBuf == NULL || a_ucSrcBuf == NULL) {
        HAL_LOGE("addr is null");
        return false;
    }

    int i;
    uint8_t *dst = a_ucDstBuf;
    uint8_t *src = a_ucSrcBuf;
    int wh = a_uwSrcWidth * a_uwSrcHeight;
    i = 0;

    for (i = 0; i < wh; i++) {
        dst[i] = src[(i%a_uwSrcHeight)* a_uwSrcWidth + (i/a_uwSrcHeight)];
    }

    return true;
}

bool SprdCamera33DFaceId::NV21Rotate90AndMirror(uint8_t *a_ucDstBuf,
                                        uint8_t *a_ucSrcBuf,
                                        uint16_t a_uwSrcWidth,
                                        uint16_t a_uwSrcHeight,
                                        uint32_t a_udFileSize) {
    int k, x, nw, nh, uvw, uvh;

    nw = a_uwSrcHeight;
    nh = a_uwSrcWidth;
    uvw = nw/2;
    uvh = nh/2;
    // rotate Y
    k = 0;
    for (k = 0; k < nw * nh; k++) {
        a_ucDstBuf[k] = a_ucSrcBuf[(k%nw)* nh + (k/nw)];
    }

    // rotate cbcr
    k = nw * nh;
    for (x = 0; x < uvw * uvh; x++) {
        a_ucDstBuf[k+2*x] = a_ucSrcBuf[k + (x%uvw)* uvh*2 + (x/uvw)*2];
        a_ucDstBuf[k+2*x+1] = a_ucSrcBuf[k + (x%uvw)* uvh*2 + (x/uvw)*2 + 1];
    }

    return true;
}

void SprdCamera33DFaceId::processYUVData(new_mem_t *buffer, uint32_t frame_number) {
    char prop[PROPERTY_VALUE_MAX] = {0};
    char frame_num[PROPERTY_VALUE_MAX] = {0};
    void * src_addr = NULL;
    int src_size = 0;

    uint8_t *temp_addr = (uint8_t *)malloc(mPreviewWidth * mPreviewHeight * 3 / 2);
    memset(temp_addr, 0, sizeof(mPreviewWidth * mPreviewHeight * 3 / 2));
    map(&buffer->native_handle,&src_addr);

    src_size = mPreviewWidth * mPreviewHeight * 3 / 2; // yuv size;

    property_get("persist.vendor.dump.3dface", prop, "null");
    property_get("debug.camera.dump.frame_num", frame_num, "10");
    int dump_num = atoi(frame_num);

    if (!strcmp(prop, "yuv") || !strcmp(prop, "all")) {
        if (g_yuv_frame_dump_cnt < dump_num) {
            m3DFaceId->dumpData((unsigned char *)src_addr, 5,
                               src_size, mPreviewHeight, mPreviewWidth, frame_number,
                               "yuv");
            g_yuv_frame_dump_cnt++;
        }
    }

    unmap(&buffer->native_handle);
    free(temp_addr);

}

/*===========================================================================
 * FUNCTION   :processRawData
 *
 * DESCRIPTION: 1.process raw10 data to raw8 2.doing rotate 3.dump data.
 *
 * PARAMETERS : buffer is the data will be deal with.
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera33DFaceId::processRawData(new_mem_t *buffer, buffer_type_3D type, uint32_t frame_number)
{
    int src_size = 0, convert_size = 0;      // raw8 size;
    void * src_addr = NULL;
    char prop[PROPERTY_VALUE_MAX] = {0};
    char frame_num[PROPERTY_VALUE_MAX] = {0};

    map(&buffer->native_handle,&src_addr);
    uint8_t *temp_addr = (uint8_t *)malloc(mIRWidth * mIRHeight * 3 / 2);
    memset(temp_addr, 0, sizeof(mIRWidth * mIRHeight * 3 / 2));

    src_size = mIRWidth * mIRHeight * 10 / 8; // raw10 size;
    convert_size = mIRWidth * mIRHeight;      // raw8 size

    raw10ToRaw8((void *)temp_addr, (void *)src_addr, src_size);

    m3DFaceId->Raw8Rotate((uint8_t *)src_addr, (uint8_t *)temp_addr, THREE_D_FACEID_WIDTH, THREE_D_FACEID_HEIGHT, IMG_ANGLE_0);

    property_get("persist.vendor.dump.3dface", prop, "null");
    property_get("debug.camera.dump.frame_num", frame_num, "10");
    int dump_num = atoi(frame_num);

    if (!strcmp(prop, "aux1_raw8") || !strcmp(prop, "aux2_raw8") ||
            !strcmp(prop, "all")) {
        if (g_raw_frame_dump_cnt < dump_num) {
            m3DFaceId->dumpData((unsigned char *)src_addr, 4,
                               convert_size, mIRWidth, mIRHeight, frame_number,
                               ((type == AUX_BUFFER_L) ? "aux1_raw8" : "aux2_raw8"));
            g_raw_frame_dump_cnt++;
        }
    }
    unmap(&buffer->native_handle);
    free(temp_addr);
}

/*===========================================================================
 * FUNCTION   :processCaptureResultAuxL
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera33DFaceId::processCaptureResultAuxL(
    camera3_capture_result_t *result) {
    if (result->output_buffers == NULL) {
        return;
    }
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    int i = 0;
    int currStreamType = getStreamType(result_buffer->stream);
    int result_buf_fd = ADP_BUFFD(*result_buffer->buffer);
    HAL_LOGD("result process frameid %d, currStreamType = %d", cur_frame_number, currStreamType);

    // callback process
    if (0 == currStreamType) {
        for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
            int saved_buf_fd = ADP_BUFFD(mLocalBufferAuxL[i].native_handle);
            if (result_buf_fd == saved_buf_fd) {
                Mutex::Autolock l(mAuxLockL);
                processRawData(&mLocalBufferAuxL[i], AUX_BUFFER_L, cur_frame_number);
                HAL_LOGV("mResultBufferListAuxL ++,i = %d",i);
                mResultBufferListAuxL.push_back(&mLocalBufferAuxL[i]);
            }
        }
    }
}



/*===========================================================================
 * FUNCTION   :processCaptureResultAuxR
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera33DFaceId::processCaptureResultAuxR(
    camera3_capture_result_t *result) {
    if (result->output_buffers == NULL) {
        return;
    }
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    int i = 0;
    int currStreamType = getStreamType(result_buffer->stream);
    int result_buf_fd = ADP_BUFFD(*result_buffer->buffer);
    HAL_LOGD("result process frameid %d, currStreamType = %d", cur_frame_number, currStreamType);

    // callback process
    if (0 == currStreamType) {
        for (i = 0; i < THREE_D_FACEID_BUFFER_SUM; i++) {
            int saved_buf_fd = ADP_BUFFD(mLocalBufferAuxR[i].native_handle);
            if (result_buf_fd == saved_buf_fd) {
                Mutex::Autolock l(mAuxLockR);
                processRawData(&mLocalBufferAuxR[i], AUX_BUFFER_R, cur_frame_number);
                HAL_LOGV("mResultBufferListAuxR ++,i = %d",i);
                mResultBufferListAuxR.push_back(&mLocalBufferAuxR[i]);
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
void SprdCamera33DFaceId::_dump(const struct camera3_device *device, int fd) {
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
int SprdCamera33DFaceId::_flush(const struct camera3_device *device) {
    HAL_LOGD("E");

    int rc = 0;
    mFlushing = true;
    // flush main camera
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);
    SprdCamera3HWI *hwiAuxL = m_pPhyCamera[CAM_TYPE_AUX1].hwi;
    rc = hwiAuxL->flush(m_pPhyCamera[CAM_TYPE_AUX1].dev);
    SprdCamera3HWI *hwiAuxR = m_pPhyCamera[CAM_TYPE_AUX2].hwi;
    rc = hwiAuxR->flush(m_pPhyCamera[CAM_TYPE_AUX2].dev);

    HAL_LOGD("X");

    return rc;
}
};
