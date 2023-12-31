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
#define LOG_TAG "Cam3SingleFaceIdR"
#include "SprdCamera3SingleFaceIdRegister.h"

using namespace android;
namespace sprdcamera {

SprdCamera3SingleFaceIdRegister *mFaceIdRegister = NULL;

// Error Check Macros
#define CHECK_FACEIDREGISTER()                                                 \
    if (!mFaceIdRegister) {                                                    \
        HAL_LOGE("Error getting switch");                                      \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_FACEIDREGISTER_ERROR()                                           \
    if (!mFaceIdRegister) {                                                    \
        HAL_LOGE("Error getting switch");                                      \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3SingleFaceIdRegister::mCameraCaptureOps = {
    .initialize = SprdCamera3SingleFaceIdRegister::initialize,
    .configure_streams = SprdCamera3SingleFaceIdRegister::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3SingleFaceIdRegister::construct_default_request_settings,
    .process_capture_request =
        SprdCamera3SingleFaceIdRegister::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3SingleFaceIdRegister::dump,
    .flush = SprdCamera3SingleFaceIdRegister::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3SingleFaceIdRegister::callback_ops_main = {
    .process_capture_result =
        SprdCamera3SingleFaceIdRegister::process_capture_result_main,
    .notify = SprdCamera3SingleFaceIdRegister::notifyMain};

camera3_callback_ops SprdCamera3SingleFaceIdRegister::callback_ops_aux = {
    .process_capture_result =
        SprdCamera3SingleFaceIdRegister::process_capture_result_aux,
    .notify = SprdCamera3SingleFaceIdRegister::notifyAux};

/*===========================================================================
 * FUNCTION   : SprdCamera3FaceIdRegister
 *
 * DESCRIPTION: SprdCamera3FaceIdRegister Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3SingleFaceIdRegister::SprdCamera3SingleFaceIdRegister() {
    HAL_LOGI("E");

    m_pPhyCamera = NULL;
    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = CAM_FACE_MAIN_ID;
    mStaticMetadata = NULL;
    mPhyCameraNum = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mRegisterPhyaddr = 0;
    mFlushing = false;
    mCallbackOps = NULL;
    mSavedRequestList.clear();
    memset(&mLocalBuffer, 0, sizeof(single_faceid_register_alloc_mem_t) *
                                 SINGLE_FACEID_REGISTER_BUFFER_SUM);
    memset(&mSavedReqStreams, 0,
           sizeof(camera3_stream_t *) * SINGLE_FACEID_REGISTER_MAX_STREAMS);
    bzero(&mRegisterStreams, sizeof(mRegisterStreams));

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3FaceIdRegister
 *
 * DESCRIPTION: SprdCamera3FaceIdRegister Desctructor
 *
 *==========================================================================*/
SprdCamera3SingleFaceIdRegister::~SprdCamera3SingleFaceIdRegister() {
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
void SprdCamera3SingleFaceIdRegister::getCameraFaceId(
    SprdCamera3SingleFaceIdRegister **pFaceid) {
    if (!mFaceIdRegister) {
        mFaceIdRegister = new SprdCamera3SingleFaceIdRegister();
    }

    CHECK_FACEIDREGISTER();
    *pFaceid = mFaceIdRegister;
    HAL_LOGV("mFaceIdRegister=%p", mFaceIdRegister);

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
SprdCamera3SingleFaceIdRegister::get_camera_info(__unused cmr_s32 camera_id,
                                                 struct camera_info *info) {
    HAL_LOGV("E");

    cmr_s32 rc = NO_ERROR;

    if (info) {
        rc = mFaceIdRegister->getCameraInfo(camera_id, info);
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
cmr_s32 SprdCamera3SingleFaceIdRegister::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    cmr_s32 rc = NO_ERROR;

    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    HAL_LOGV("id=%d", atoi(id));
    rc = mFaceIdRegister->cameraDeviceOpen(atoi(id), hw_device);

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
cmr_s32 SprdCamera3SingleFaceIdRegister::close_camera_device(
    __unused hw_device_t *hw_dev) {
    if (NULL == hw_dev) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mFaceIdRegister->closeCameraDevice();
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
cmr_s32 SprdCamera3SingleFaceIdRegister::closeCameraDevice() {
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
cmr_s32 SprdCamera3SingleFaceIdRegister::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGV("E");

    cmr_s32 rc = NO_ERROR;

    CHECK_FACEIDREGISTER_ERROR();
    rc = mFaceIdRegister->initialize(callback_ops);

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
SprdCamera3SingleFaceIdRegister::construct_default_request_settings(
    const struct camera3_device *device, cmr_s32 type) {
    HAL_LOGV("E");

    const camera_metadata_t *rc;

    if (!mFaceIdRegister) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }

    rc = mFaceIdRegister->constructDefaultRequestSettings(device, type);

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
cmr_s32 SprdCamera3SingleFaceIdRegister::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;

    CHECK_FACEIDREGISTER_ERROR();
    rc = mFaceIdRegister->configureStreams(device, stream_list);

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
cmr_s32 SprdCamera3SingleFaceIdRegister::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;

    CHECK_FACEIDREGISTER_ERROR();
    rc = mFaceIdRegister->processCaptureRequest(device, request);

    HAL_LOGV("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdRegister
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SingleFaceIdRegister::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("frame number=%d", result->frame_number);

    CHECK_FACEIDREGISTER();
    mFaceIdRegister->processCaptureResultMain(
        (camera3_capture_result_t *)result);
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
void SprdCamera3SingleFaceIdRegister::notifyMain(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    HAL_LOGI("frame number=%d", msg->message.shutter.frame_number);

    CHECK_FACEIDREGISTER();
    mFaceIdRegister->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdRegister
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SingleFaceIdRegister::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    // This mode do not have real aux result
    CHECK_FACEIDREGISTER();
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
void SprdCamera3SingleFaceIdRegister::notifyAux(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    // This mode do not have real aux notify
    CHECK_FACEIDREGISTER();
}

/*===========================================================================
 * FUNCTION   :allocateBuffer
 *
 * DESCRIPTION: deconstructor of SprdCamera3FaceIdRegister
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
cmr_s32 SprdCamera3SingleFaceIdRegister::allocateBuffer(
    cmr_s32 w, cmr_s32 h, cmr_u32 is_cache, cmr_s32 format,
    single_faceid_register_alloc_mem_t *new_mem) {
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
    HAL_LOGD("fd=%d", fd);
    HAL_LOGD("end");
    return result;
}

/*===========================================================================
 * FUNCTION   : freeLocalCapBuffer
 *
 * DESCRIPTION: free faceid_register_alloc_mem_t buffer
 *
 * PARAMETERS:
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3SingleFaceIdRegister::freeLocalCapBuffer() {
    HAL_LOGV("E");

    size_t buffer_num = 0;

    buffer_num = SINGLE_FACEID_REGISTER_BUFFER_SUM;

    mPhyAddrBufferList.clear();
    mCreateBufferList.clear();

    for (size_t i = 0; i < buffer_num; i++) {
        single_faceid_register_alloc_mem_t *local_buffer = &mLocalBuffer[i];

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
cmr_s32 SprdCamera3SingleFaceIdRegister::cameraDeviceOpen(
    cmr_s32 camera_id, struct hw_device_t **hw_device) {
    HAL_LOGD("E");

    cmr_s32 rc = NO_ERROR;
    cmr_s32 i = 0;
    cmr_u32 Phy_id = 0;

    mPhyCameraNum = 1;
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

        hw->setMultiCameraMode(MODE_SINGLE_FACEID_REGISTER);
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
cmr_s32
SprdCamera3SingleFaceIdRegister::getCameraInfo(cmr_s32 face_camera_id,
                                               struct camera_info *info) {
    cmr_s32 rc = NO_ERROR;
    cmr_s32 camera_id = 0;
    cmr_s32 img_size = 0;

    HAL_LOGD("camera_id=%d", face_camera_id);
    m_VirtualCamera.id = CAM_FACE_MAIN_ID;

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
cmr_s32 SprdCamera3SingleFaceIdRegister::setupPhysicalCameras() {
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
void SprdCamera3SingleFaceIdRegister::dump(const struct camera3_device *device,
                                           cmr_s32 fd) {
    HAL_LOGV("E");
    CHECK_FACEIDREGISTER();

    mFaceIdRegister->_dump(device, fd);

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
SprdCamera3SingleFaceIdRegister::flush(const struct camera3_device *device) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;

    CHECK_FACEIDREGISTER_ERROR();
    rc = mFaceIdRegister->_flush(device);

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
cmr_s32 SprdCamera3SingleFaceIdRegister::initialize(
    const camera3_callback_ops_t *callback_ops) {
    HAL_LOGD("E");

    cmr_s32 rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    CHECK_HWI_ERROR(hwiMain);

    mFlushing = false;
    mRegisterPhyaddr = 0;

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (NO_ERROR != rc) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    memset(mLocalBuffer, 0, sizeof(single_faceid_register_alloc_mem_t) *
                                SINGLE_FACEID_REGISTER_BUFFER_SUM);
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
cmr_s32 SprdCamera3SingleFaceIdRegister::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    cmr_s32 rc = 0;
    camera3_stream_t *pRegisterStreams[SINGLE_FACEID_REGISTER_MAX_STREAMS];
    size_t i = 0;
    size_t j = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;

    Mutex::Autolock l(mLock);

    HAL_LOGD("FACEID_REGISTER, num_streams=%d", stream_list->num_streams);
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
            mRegisterStreams[i] = *stream_list->streams[i];
        } else {
            mRegisterStreams[i].stream_type = CAMERA3_STREAM_OUTPUT;
            mRegisterStreams[i].width = mPreviewWidth;
            mRegisterStreams[i].height = mPreviewHeight;
            mRegisterStreams[i].format = HAL_PIXEL_FORMAT_YCbCr_420_888;
            mRegisterStreams[i].usage = stream_list->streams[i]->usage;
            mRegisterStreams[i].max_buffers = 1;
            mRegisterStreams[i].data_space =
                stream_list->streams[i]->data_space;
            mRegisterStreams[i].rotation = stream_list->streams[i]->rotation;
        }

        pRegisterStreams[i] = &mRegisterStreams[i];
    }
    // for callback buffer
    freeLocalCapBuffer();
    for (j = 0; j < SINGLE_FACEID_REGISTER_BUFFER_SUM; j++) {
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
    mainconfig.num_streams = SINGLE_FACEID_REGISTER_MAX_STREAMS;
    mainconfig.streams = pRegisterStreams;

    for (i = 0; i < mainconfig.num_streams; i++) {
        HAL_LOGD("stream_type=%d, width=%d, height=%d, format=%d",
                 pRegisterStreams[i]->stream_type, pRegisterStreams[i]->width,
                 pRegisterStreams[i]->height, pRegisterStreams[i]->format);
    }

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    for (i = 0; i < mainconfig.num_streams; i++) {
        memcpy(stream_list->streams[i], &mRegisterStreams[i],
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
SprdCamera3SingleFaceIdRegister::constructDefaultRequestSettings(
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
void SprdCamera3SingleFaceIdRegister::saveRequest(
    camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;

    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (CALLBACK_STREAM == getStreamType(newStream)) {
            single_faceid_register_saved_request_t prevRequest;
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
cmr_s32 SprdCamera3SingleFaceIdRegister::processCaptureRequest(
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

	cmr_u8 sprdSmileCaptureEnabled = 0;
	metaSettings.update(ANDROID_SPRD_SMILE_CAPTURE_ENABLE, &sprdSmileCaptureEnabled, 1);
    }

    // get phy addr from faceidservice
    if (metaSettings.exists(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)) {
        get_reg_phyaddr =
            (unsigned long)
                metaSettings.find(ANDROID_SPRD_FROM_FACEIDSERVICE_PHYADDR)
                    .data.i64[0];
        for (i = 0; i < SINGLE_FACEID_REGISTER_BUFFER_SUM; i++) {
            if (get_reg_phyaddr == mLocalBuffer[i].phy_addr) {
                // get phy addr
                HAL_LOGI("get phy addr from faceidservice");
                mPhyAddrBufferList.push_back(mLocalBuffer[i]);
                mCreateBufferList.push_back(&(mLocalBuffer[i].native_handle));
            }
        }
    }

    List<single_faceid_register_alloc_mem_t>::iterator register_i =
        mPhyAddrBufferList.begin();
    if (register_i != mPhyAddrBufferList.end()) {
        camera3_stream_buffer_t out_streams_main[2];

        // construct preview buffer
        i = 0;
        out_streams_main[i] = *req->output_buffers;
        mSavedReqStreams[i] = req->output_buffers->stream;
        out_streams_main[i].stream = &mRegisterStreams[i];

        // create callback buffer
        i++;
        out_streams_main[i].stream = &mRegisterStreams[i];
        out_streams_main[i].buffer =
            popRequestList(mCreateBufferList); // &(register_i->native_handle);
        mPhyAddrBufferList.erase(register_i);

        out_streams_main[i].status = req->output_buffers->status;
        out_streams_main[i].acquire_fence = -1;
        // req->output_buffers->acquire_fence;
        out_streams_main[i].release_fence = -1;
        // req->output_buffers->release_fence;

        // construct output_buffers buffer
        req_main.num_output_buffers = SINGLE_FACEID_REGISTER_OUTPUT_BUFFERS;
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
        out_streams_main[i].stream = &mRegisterStreams[i];

        req_main.num_output_buffers = SINGLE_FACEID_REGISTER_OUTPUT_BUFFERS - 1;
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
void SprdCamera3SingleFaceIdRegister::notifyMain(
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
void SprdCamera3SingleFaceIdRegister::processCaptureResultMain(
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
        if (mRegisterPhyaddr) {
            // callback phy addr
            HAL_LOGD("callback phy addr=0x%llx", mRegisterPhyaddr);
            metadata.update(ANDROID_SPRD_TO_FACEIDSERVICE_PHYADDR,
                            &mRegisterPhyaddr, 1);
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
            mRegisterPhyaddr = 0;
        } else {
            mCallbackOps->process_capture_result(mCallbackOps, result);
        }
        return;
    }

    if (result_buffer == NULL) {
        HAL_LOGE("result_buffer = result->output_buffers is NULL");
        return;
    }
    cmr_s32 currStreamType = getStreamType(result_buffer->stream);
    // callback process
    if (DEFAULT_STREAM == currStreamType) {
        HAL_LOGD("callback process");
        int callback_fd = ADP_BUFFD(*(result_buffer->buffer));
        for (i = 0; i < SINGLE_FACEID_REGISTER_BUFFER_SUM; i++) {
            int saved_fd =
                ADP_BUFFD(*(&mFaceIdRegister->mLocalBuffer[i].native_handle));
            if (callback_fd == saved_fd) {
                mRegisterPhyaddr = (cmr_s64)mLocalBuffer[i].phy_addr;
            }
        }
        return;
    }
    // preview process
    HAL_LOGD("preview process buffer_status = %d",result_buffer->status);
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        CallBackResult(result->frame_number, CAMERA3_BUFFER_STATUS_ERROR);
        return;
    }
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
void SprdCamera3SingleFaceIdRegister::CallBackResult(
    cmr_u32 frame_number, camera3_buffer_status_t buffer_status) {
    camera3_capture_result_t result;
    List<single_faceid_register_saved_request_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;

    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    {
        Mutex::Autolock l(mFaceIdRegister->mRequestLock);
        itor = mFaceIdRegister->mSavedRequestList.begin();
        while (itor != mFaceIdRegister->mSavedRequestList.end()) {
            if (itor->frame_number == frame_number) {
                HAL_LOGD("erase frame_number %u", frame_number);
                result_buffers.stream = itor->stream;
                result_buffers.buffer = itor->buffer;
                mFaceIdRegister->mSavedRequestList.erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mFaceIdRegister->mSavedRequestList.end()) {
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
void SprdCamera3SingleFaceIdRegister::_dump(const struct camera3_device *device,
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
SprdCamera3SingleFaceIdRegister::_flush(const struct camera3_device *device) {
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
