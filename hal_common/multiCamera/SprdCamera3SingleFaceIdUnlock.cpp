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
#define LOG_TAG "Cam3SingleFaceIdU"
#include "SprdCamera3SingleFaceIdUnlock.h"

using namespace android;
namespace sprdcamera {

SprdCamera3SingleFaceIdUnlock *mFaceIdUnlock = NULL;

// Error Check Macros
#define CHECK_FACEIDUNLOCK()                                                   \
    if (!mFaceIdUnlock) {                                                      \
        HAL_LOGE("Error getting switch");                                      \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_FACEIDUNLOCK_ERROR()                                             \
    if (!mFaceIdUnlock) {                                                      \
        HAL_LOGE("Error getting switch");                                      \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3SingleFaceIdUnlock::mCameraCaptureOps = {
    .initialize = SprdCamera3SingleFaceIdUnlock::initialize,
    .configure_streams = SprdCamera3SingleFaceIdUnlock::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3SingleFaceIdUnlock::construct_default_request_settings,
    .process_capture_request =
        SprdCamera3SingleFaceIdUnlock::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3SingleFaceIdUnlock::dump,
    .flush = SprdCamera3SingleFaceIdUnlock::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3SingleFaceIdUnlock::callback_ops_main = {
    .process_capture_result =
        SprdCamera3SingleFaceIdUnlock::process_capture_result_main,
    .notify = SprdCamera3SingleFaceIdUnlock::notifyMain};

camera3_callback_ops SprdCamera3SingleFaceIdUnlock::callback_ops_aux = {
    .process_capture_result =
        SprdCamera3SingleFaceIdUnlock::process_capture_result_aux,
    .notify = SprdCamera3SingleFaceIdUnlock::notifyAux};

/*===========================================================================
 * FUNCTION   : SprdCamera3FaceIdUnlock
 *
 * DESCRIPTION: SprdCamera3FaceIdUnlock Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3SingleFaceIdUnlock::SprdCamera3SingleFaceIdUnlock() {
    HAL_LOGI("E");

    m_pPhyCamera = NULL;
    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = CAM_FACE_MAIN_ID;
    mStaticMetadata = NULL;
    mPhyCameraNum = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mUnlockPhyaddr = 0;
    mFlushing = false;
    mSavedRequestList.clear();
    memset(&mLocalBuffer, 0, sizeof(single_faceid_unlock_alloc_mem_t) *
                                 SINGLE_FACEID_UNLOCK_BUFFER_SUM);
    memset(&mSavedReqStreams, 0,
           sizeof(camera3_stream_t *) * SINGLE_FACEID_UNLOCK_MAX_STREAMS);

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3FaceIdUnlock
 *
 * DESCRIPTION: SprdCamera3FaceIdUnlock Desctructor
 *
 *==========================================================================*/
SprdCamera3SingleFaceIdUnlock::~SprdCamera3SingleFaceIdUnlock() {
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
void SprdCamera3SingleFaceIdUnlock::getCameraFaceId(
    SprdCamera3SingleFaceIdUnlock **pFaceid) {
    if (!mFaceIdUnlock) {
        mFaceIdUnlock = new SprdCamera3SingleFaceIdUnlock();
    }

    CHECK_FACEIDUNLOCK();
    *pFaceid = mFaceIdUnlock;
    HAL_LOGV("mFaceIdUnlock=%p", mFaceIdUnlock);

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
cmr_s32
SprdCamera3SingleFaceIdUnlock::get_camera_info(__unused cmr_s32 camera_id,
                                               struct camera_info *info) {
    HAL_LOGV("E");

    cmr_s32 rc = NO_ERROR;

    if (info) {
        rc = mFaceIdUnlock->getCameraInfo(camera_id, info);
    }

    HAL_LOGV("X, rc=%d", rc);

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
cmr_s32 SprdCamera3SingleFaceIdUnlock::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    cmr_s32 rc = NO_ERROR;

    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    HAL_LOGV("id=%d", atoi(id));
    rc = mFaceIdUnlock->cameraDeviceOpen(atoi(id), hw_device);

    HAL_LOGV("id=%d, rc: %d", atoi(id), rc);

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
cmr_s32 SprdCamera3SingleFaceIdUnlock::close_camera_device(
    __unused hw_device_t *hw_dev) {
    if (NULL == hw_dev) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mFaceIdUnlock->closeCameraDevice();
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
cmr_s32 SprdCamera3SingleFaceIdUnlock::closeCameraDevice() {
    HAL_LOGD("E");

    cmr_s32 rc = NO_ERROR;
    cmr_s32 i = 0;
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
cmr_s32 SprdCamera3SingleFaceIdUnlock::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGV("E");

    cmr_s32 rc = NO_ERROR;

    CHECK_FACEIDUNLOCK_ERROR();
    rc = mFaceIdUnlock->initialize(callback_ops);

    HAL_LOGV("X");

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
SprdCamera3SingleFaceIdUnlock::construct_default_request_settings(
    const struct camera3_device *device, cmr_s32 type) {
    HAL_LOGV("E");

    const camera_metadata_t *rc;

    if (!mFaceIdUnlock) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }

    rc = mFaceIdUnlock->constructDefaultRequestSettings(device, type);

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
cmr_s32 SprdCamera3SingleFaceIdUnlock::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;

    CHECK_FACEIDUNLOCK_ERROR();
    rc = mFaceIdUnlock->configureStreams(device, stream_list);

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
cmr_s32 SprdCamera3SingleFaceIdUnlock::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;

    CHECK_FACEIDUNLOCK_ERROR();
    rc = mFaceIdUnlock->processCaptureRequest(device, request);

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
void SprdCamera3SingleFaceIdUnlock::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("frame number=%d", result->frame_number);

    CHECK_FACEIDUNLOCK();
    mFaceIdUnlock->processCaptureResultMain((camera3_capture_result_t *)result);
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
void SprdCamera3SingleFaceIdUnlock::notifyMain(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    HAL_LOGI("frame number=%d", msg->message.shutter.frame_number);

    CHECK_FACEIDUNLOCK();
    mFaceIdUnlock->notifyMain(msg);
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
void SprdCamera3SingleFaceIdUnlock::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    // This mode do not have real aux result
    CHECK_FACEIDUNLOCK();
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
void SprdCamera3SingleFaceIdUnlock::notifyAux(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    // This mode do not have real aux notify
    CHECK_FACEIDUNLOCK();
}

/*===========================================================================
 * FUNCTION   :allocateBuffer
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdUnlock
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
cmr_s32 SprdCamera3SingleFaceIdUnlock::allocateBuffer(
    cmr_s32 w, cmr_s32 h, cmr_u32 is_cache, cmr_s32 format,
    single_faceid_unlock_alloc_mem_t *new_mem) {
    HAL_LOGD("start");
    cmr_s32 result = 0;
    cmr_s32 ret = 0;
    cmr_uint phy_addr = 0;
    size_t buf_size = 0;
    sp<GraphicBuffer> graphicBuffer = NULL;
    native_handle_t *native_handle = NULL;
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;

    yuvTextUsage |= GRALLOC_USAGE_CAMERA_BUFFER;

#if defined(CONFIG_SPRD_ANDROID_8)
    graphicBuffer = new GraphicBuffer(w, h, HAL_PIXEL_FORMAT_YCrCb_420_SP, 1,
                                      yuvTextUsage, "dualcamera");
#else
    graphicBuffer =
        new GraphicBuffer(w, h, HAL_PIXEL_FORMAT_YCrCb_420_SP, yuvTextUsage);
#endif

    native_handle = (native_handle_t *)graphicBuffer->handle;

    int fd = ADP_BUFFD(native_handle);
    ret = MemIon::Get_phy_addr_from_ion(fd, &phy_addr, &buf_size);
    if (ret) {
        HAL_LOGW("get phy addr fail %d", ret);
    }
    HAL_LOGI("phy_addr=0x%lx, buf_size=%zu", phy_addr, buf_size);

    new_mem->native_handle = native_handle;
    new_mem->graphicBuffer = graphicBuffer;
    new_mem->phy_addr = phy_addr;
    new_mem->buf_size = buf_size;
    HAL_LOGD("fd=%p", fd);
    HAL_LOGD("end");
    return result;
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
void SprdCamera3SingleFaceIdUnlock::freeLocalCapBuffer() {
    HAL_LOGV("E");

    size_t buffer_num = 0;

    buffer_num = SINGLE_FACEID_UNLOCK_BUFFER_SUM;

    mPhyAddrBufferList.clear();
    mCreateBufferList.clear();

    for (size_t i = 0; i < buffer_num; i++) {
        single_faceid_unlock_alloc_mem_t *local_buffer = &mLocalBuffer[i];

        if (local_buffer->graphicBuffer != NULL) {
            local_buffer->graphicBuffer.clear();
            local_buffer->graphicBuffer = NULL;
        }
        local_buffer->native_handle = NULL;
        local_buffer->phy_addr = 0;
        local_buffer->buf_size = 0;
    }

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
 * RETURN     : cmr_s32 type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
cmr_s32 SprdCamera3SingleFaceIdUnlock::cameraDeviceOpen(
    cmr_s32 camera_id, struct hw_device_t **hw_device) {
    HAL_LOGD("E");

    cmr_s32 rc = NO_ERROR;
    cmr_s32 i = 0;
    cmr_u32 Phy_id = 0;
    struct logicalSensorInfo *logicalPtr = NULL;

    if (SPRD_SINGLE_FACEID_UNLOCK_ID == camera_id) {
        mPhyCameraNum = 1;
    } else {
        HAL_LOGW("unlock mode camera_id should not be %d", camera_id);
    }

    hw_device_t *hw_dev[mPhyCameraNum];
    setupPhysicalCameras();

    logicalPtr = sensorGetLogicaInfo4MulitCameraId(camera_id);
    if (logicalPtr) {
        if (mPhyCameraNum == logicalPtr->physicalNum) {
            for (i = 0; i < logicalPtr->physicalNum; i++) {
                m_pPhyCamera[i].id = (uint8_t)logicalPtr->phyIdGroup[i];
                HAL_LOGD("i = %d, phyId = %d", i, logicalPtr->phyIdGroup[i]);
            }
        }
    }

    // Open all physical cameras
    for (i = 0; i < mPhyCameraNum; i++) {
        Phy_id = m_pPhyCamera[i].id;

        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)Phy_id);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode(MODE_SINGLE_FACEID_UNLOCK);
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
 * RETURN     : cmr_s32 type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
cmr_s32 SprdCamera3SingleFaceIdUnlock::getCameraInfo(cmr_s32 face_camera_id,
                                                     struct camera_info *info) {
    cmr_s32 rc = NO_ERROR;
    cmr_s32 camera_id = 0;
    cmr_s32 img_size = 0;
    struct logicalSensorInfo *logicalPtr = NULL;

    HAL_LOGD("camera_id=%d", face_camera_id);

    if (SPRD_SINGLE_FACEID_UNLOCK_ID == face_camera_id) {
        m_VirtualCamera.id = CAM_FACE_MAIN_ID;
    } else {
        HAL_LOGW("unlock mode camera_id should not be %d", camera_id);
    }

    logicalPtr = sensorGetLogicaInfo4MulitCameraId(face_camera_id);
    if (logicalPtr) {
        if (1 == logicalPtr->physicalNum) {
            m_VirtualCamera.id = (uint8_t)logicalPtr->phyIdGroup[0];
            HAL_LOGD("phyId = %d", logicalPtr->phyIdGroup[0]);
        }
    }

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
cmr_s32 SprdCamera3SingleFaceIdUnlock::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[mPhyCameraNum];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }

    memset(m_pPhyCamera, 0x00,
           (mPhyCameraNum * sizeof(sprdcamera_physical_descriptor_t)));

    m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_FACE_MAIN_ID;

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
void SprdCamera3SingleFaceIdUnlock::dump(const struct camera3_device *device,
                                         cmr_s32 fd) {
    HAL_LOGV("E");
    CHECK_FACEIDUNLOCK();

    mFaceIdUnlock->_dump(device, fd);

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
cmr_s32
SprdCamera3SingleFaceIdUnlock::flush(const struct camera3_device *device) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;

    CHECK_FACEIDUNLOCK_ERROR();
    rc = mFaceIdUnlock->_flush(device);

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
cmr_s32 SprdCamera3SingleFaceIdUnlock::initialize(
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGD("E");

    cmr_s32 rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    CHECK_HWI_ERROR(hwiMain);

    mFlushing = false;
    mUnlockPhyaddr = 0;

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (NO_ERROR != rc) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    memset(mLocalBuffer, 0, sizeof(single_faceid_unlock_alloc_mem_t) *
                                SINGLE_FACEID_UNLOCK_BUFFER_SUM);
    mCallbackOps = callback_ops;

    HAL_LOGD("X");

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
cmr_s32 SprdCamera3SingleFaceIdUnlock::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;
    camera3_stream_t *pUnlockStreams[SINGLE_FACEID_UNLOCK_MAX_STREAMS];
    size_t i = 0;
    size_t j = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;

    Mutex::Autolock l(mLock);

    HAL_LOGD("FACEID_UNLOCK, num_streams=%d", stream_list->num_streams);
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD("%d: stream type=%d, width=%d, height=%d, "
                 "format=%d",
                 i, stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height,
                 stream_list->streams[i]->format);

        cmr_s32 requestStreamType = getStreamType(stream_list->streams[i]);
        if (PREVIEW_STREAM == requestStreamType) {
            mPreviewWidth = stream_list->streams[i]->width;
            mPreviewHeight = stream_list->streams[i]->height;
            mUnlockStreams[i] = *stream_list->streams[i];
        } else {
            mUnlockStreams[i].stream_type = CAMERA3_STREAM_OUTPUT;
            mUnlockStreams[i].width = mPreviewWidth;
            mUnlockStreams[i].height = mPreviewHeight;
            mUnlockStreams[i].format = HAL_PIXEL_FORMAT_YCbCr_420_888;
            mUnlockStreams[i].usage = stream_list->streams[i]->usage;
            mUnlockStreams[i].max_buffers = 1;
            mUnlockStreams[i].data_space = stream_list->streams[i]->data_space;
            mUnlockStreams[i].rotation = stream_list->streams[i]->rotation;
        }

        pUnlockStreams[i] = &mUnlockStreams[i];
    }
    // for callback buffer
    freeLocalCapBuffer();
    for (j = 0; j < SINGLE_FACEID_UNLOCK_BUFFER_SUM; j++) {
        if (0 > allocateBuffer(mPreviewWidth, mPreviewHeight, 1,
                               HAL_PIXEL_FORMAT_YCrCb_420_SP,
                               &(mLocalBuffer[j]))) {
            HAL_LOGE("request one buf failed.");
        }
        mPhyAddrBufferList.push_back(mLocalBuffer[j]);
        mCreateBufferList.push_back(&(mLocalBuffer[j].native_handle));
    }

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = SINGLE_FACEID_UNLOCK_MAX_STREAMS;
    mainconfig.streams = pUnlockStreams;

    for (i = 0; i < mainconfig.num_streams; i++) {
        HAL_LOGD("stream_type=%d, width=%d, height=%d, format=%d",
                 pUnlockStreams[i]->stream_type, pUnlockStreams[i]->width,
                 pUnlockStreams[i]->height, pUnlockStreams[i]->format);
    }

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    for (i = 0; i < mainconfig.num_streams; i++) {
        memcpy(stream_list->streams[i], &mUnlockStreams[i],
               sizeof(camera3_stream_t));
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

    HAL_LOGV("X");

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
const camera_metadata_t *
SprdCamera3SingleFaceIdUnlock::constructDefaultRequestSettings(
    const struct camera3_device *device, cmr_s32 type) {
    HAL_LOGV("E");

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

    HAL_LOGV("X");

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
void SprdCamera3SingleFaceIdUnlock::saveRequest(
    camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;

    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (CALLBACK_STREAM == getStreamType(newStream)) {
            single_faceid_unlock_saved_request_t prevRequest;
            HAL_LOGD("save request num=%d", request->frame_number);

            Mutex::Autolock l(mRequestLock);

            prevRequest.frame_number = request->frame_number;
            prevRequest.buffer = request->output_buffers[i].buffer;
            prevRequest.stream = request->output_buffers[i].stream;
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
cmr_s32 SprdCamera3SingleFaceIdUnlock::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    HAL_LOGD("E");

    cmr_s32 rc = 0;
    cmr_u32 i = 0;
    cmr_u32 tagCnt = 0;
    cmr_uint get_reg_phyaddr = 0;
    cmr_uint get_unlock_phyaddr[2] = {0, 0};
    camera3_capture_request_t *req = NULL;
    camera3_capture_request_t req_main;
    camera3_stream_t *new_stream = NULL;
    CameraMetadata metaSettings;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;

    metaSettings = request->settings;

    req = request;
    rc = validateCaptureRequest(req);
    if (NO_ERROR != rc) {
        return rc;
    }
    saveRequest(req);

    memset(&req_main, 0x00, sizeof(camera3_capture_request_t));
    req_main = *req;

    tagCnt = metaSettings.entryCount();
    if (0 != tagCnt) {
        // disable burstmode
        cmr_u8 sprdBurstModeEnabled = 0;
        metaSettings.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                            &sprdBurstModeEnabled, 1);

        // disable zsl mode
        cmr_u8 sprdZslEnabled = 0;
        metaSettings.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);

        // disable face attribute
        cmr_u8 sprdFaceAttributesEnabled = 0;
        metaSettings.update(ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE, &sprdFaceAttributesEnabled, 1);
    }

    // get phy addr from faceidservice
    if (metaSettings.exists(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)) {
        get_reg_phyaddr =
            (unsigned long)
                metaSettings.find(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)
                    .data.i64[0];
        for (i = 0; i < SINGLE_FACEID_UNLOCK_BUFFER_SUM; i++) {
            if (get_reg_phyaddr == mLocalBuffer[i].phy_addr) {
                // get phy addr
                HAL_LOGI("get phy addr from faceidservice");
                mPhyAddrBufferList.push_back(mLocalBuffer[i]);
                mCreateBufferList.push_back(&(mLocalBuffer[i].native_handle));
            }
        }
    }

    List<single_faceid_unlock_alloc_mem_t>::iterator unlock_i =
        mPhyAddrBufferList.begin();
    if (unlock_i != mPhyAddrBufferList.end()) {
        camera3_stream_buffer_t out_streams_main[2];

        // construct preview buffer
        i = 0;
        out_streams_main[i] = *req->output_buffers;
        mSavedReqStreams[i] = req->output_buffers->stream;
        out_streams_main[i].stream = &mUnlockStreams[i];

        // create callback buffer
        i++;
        out_streams_main[i].stream = &mUnlockStreams[i];
        out_streams_main[i].buffer =
            popRequestList(mCreateBufferList); // &(unlock_i->native_handle);
        mPhyAddrBufferList.erase(unlock_i);

        out_streams_main[i].status = req->output_buffers->status;
        out_streams_main[i].acquire_fence = -1;
        // req->output_buffers->acquire_fence;
        out_streams_main[i].release_fence = -1;
        // req->output_buffers->release_fence;

        // construct output_buffers buffer
        req_main.num_output_buffers = SINGLE_FACEID_UNLOCK_OUTPUT_BUFFERS;
        req_main.output_buffers = out_streams_main;
        req_main.settings = metaSettings.release();

        HAL_LOGV("num_output_buffers=%d", req_main.num_output_buffers);
        for (i = 0; i < req_main.num_output_buffers; i++) {
            HAL_LOGD("width=%d, height=%d, format=%d",
                     req_main.output_buffers[i].stream->width,
                     req_main.output_buffers[i].stream->height,
                     req_main.output_buffers[i].stream->format);
        }

        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
        }
    } else {
        camera3_stream_buffer_t out_streams_main[1];
        // just construct preview buffer
        i = 0;
        out_streams_main[i] = *req->output_buffers;
        mSavedReqStreams[i] = req->output_buffers->stream;
        out_streams_main[i].stream = &mUnlockStreams[i];

        req_main.num_output_buffers = SINGLE_FACEID_UNLOCK_OUTPUT_BUFFERS - 1;
        req_main.output_buffers = out_streams_main;
        req_main.settings = metaSettings.release();

        HAL_LOGV("num_output_buffers=%d", req_main.num_output_buffers);
        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
        }
    }

    HAL_LOGD("D, ret=%d", rc);

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
void SprdCamera3SingleFaceIdUnlock::notifyMain(
    const camera3_notify_msg_t *msg) {
    Mutex::Autolock l(mNotifyLockMain);

    HAL_LOGD("preview timestamp %lld", msg->message.shutter.timestamp);
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
void SprdCamera3SingleFaceIdUnlock::processCaptureResultMain(
    camera3_capture_result_t *result) {
    cmr_u32 cur_frame_number = result->frame_number;
    cmr_u32 searchnotifyresult = NOTIFY_NOT_FOUND;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metadata;
    metadata = result->result;
    cmr_s32 rc = 0;
    cmr_s32 i = 0;
    Mutex::Autolock l(mResultLock);
    HAL_LOGD("E");

    // meta process
    if (NULL == result_buffer && NULL != result->result) {
        if (mUnlockPhyaddr) {
            // callback phy addr
            HAL_LOGD("callback phy addr=0x%llx", mUnlockPhyaddr);
            metadata.update(ANDROID_SPRD_TO_FACEIDSERVICE_PHYADDR,
                            &mUnlockPhyaddr, 1);
            unsigned long get_phyaddr = 0;
            if (metadata.exists(ANDROID_SPRD_TO_FACEIDSERVICE_PHYADDR)) {
                get_phyaddr =
                    (unsigned long)
                        metadata.find(ANDROID_SPRD_TO_FACEIDSERVICE_PHYADDR)
                            .data.i64[0];
                HAL_LOGD("check phy addr=0x%lx", get_phyaddr);
            } else {
                HAL_LOGD("update fail");
            }
            camera3_capture_result_t new_result = *result;
            new_result.result = metadata.release();
            mCallbackOps->process_capture_result(mCallbackOps, &new_result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(new_result.result));
            mUnlockPhyaddr = 0;
        } else {
            mCallbackOps->process_capture_result(mCallbackOps, result);
        }
        return;
    }

    cmr_s32 currStreamType = getStreamType(result_buffer->stream);
    // callback process
    if (DEFAULT_STREAM == currStreamType) {
        HAL_LOGD("callback process");
        int callback_fd = ADP_BUFFD(*(result_buffer->buffer));
        for (i = 0; i < SINGLE_FACEID_UNLOCK_BUFFER_SUM; i++) {
            int saved_fd =
                ADP_BUFFD(*(&mFaceIdUnlock->mLocalBuffer[i].native_handle));
            if (callback_fd == saved_fd) {
                mUnlockPhyaddr = (cmr_s64)mLocalBuffer[i].phy_addr;
            }
        }
        return;
    }

    // preview process
    HAL_LOGD("preview process");
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
void SprdCamera3SingleFaceIdUnlock::CallBackResult(
    cmr_u32 frame_number, camera3_buffer_status_t buffer_status) {
    camera3_capture_result_t result;
    List<single_faceid_unlock_saved_request_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;

    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    {
        Mutex::Autolock l(mFaceIdUnlock->mRequestLock);
        itor = mFaceIdUnlock->mSavedRequestList.begin();
        while (itor != mFaceIdUnlock->mSavedRequestList.end()) {
            if (itor->frame_number == frame_number) {
                HAL_LOGD("erase frame_number %u", frame_number);
                result_buffers.stream = itor->stream;
                result_buffers.buffer = itor->buffer;
                mFaceIdUnlock->mSavedRequestList.erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mFaceIdUnlock->mSavedRequestList.end()) {
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

    HAL_LOGD("frame number=%d buffer status=%u", result.frame_number,
             buffer_status);
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
void SprdCamera3SingleFaceIdUnlock::_dump(const struct camera3_device *device,
                                          cmr_s32 fd) {
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
cmr_s32
SprdCamera3SingleFaceIdUnlock::_flush(const struct camera3_device *device) {
    HAL_LOGD("E");

    cmr_s32 rc = 0;

    mFlushing = true;

    // flush main camera
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);

    HAL_LOGD("X");

    return rc;
}
};
