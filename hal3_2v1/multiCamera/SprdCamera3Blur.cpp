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
#define LOG_TAG "SprdCamera3Blur"
#include "SprdCamera3Blur.h"

using namespace android;
namespace sprdcamera {

SprdCamera3Blur *mBlur = NULL;

// Error Check Macros
#define CHECK_BLUR()                                                           \
    if (!mBlur) {                                                              \
        HAL_LOGE("Error getting blur ");                                       \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_BLUR_ERROR()                                                     \
    if (!mBlur) {                                                              \
        HAL_LOGE("Error getting blur ");                                       \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

camera3_device_ops_t SprdCamera3Blur::mCameraCaptureOps = {
    .initialize = SprdCamera3Blur::initialize,
    .configure_streams = SprdCamera3Blur::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3Blur::construct_default_request_settings,
    .process_capture_request = SprdCamera3Blur::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3Blur::dump,
    .flush = SprdCamera3Blur::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3Blur::callback_ops_main = {
    .process_capture_result = SprdCamera3Blur::process_capture_result_main,
    .notify = SprdCamera3Blur::notifyMain};

camera3_callback_ops SprdCamera3Blur::callback_ops_aux = {
    .process_capture_result = SprdCamera3Blur::process_capture_result_aux,
    .notify = SprdCamera3Blur::notifyAux};

/*===========================================================================
 * FUNCTION   : SprdCamera3Blur
 *
 * DESCRIPTION: SprdCamera3Blur Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3Blur::SprdCamera3Blur() {
    HAL_LOGD(" E");
#if CONFIG_COVERED_SENSOR
    m_nPhyCameras = 2;
#else
    m_nPhyCameras = 1;
#endif
    m_VirtualCamera.id = CAM_BLUR_MAIN_ID;
    mStaticMetadata = NULL;
    mCaptureThread = new CaptureThread();
    mIommuEnabled = 1;
    mSavedRequestList.clear();
    setupPhysicalCameras();
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mIsCapturing = false;
    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3Blur
 *
 * DESCRIPTION: SprdCamera3Blur Desctructor
 *
 *==========================================================================*/
SprdCamera3Blur::~SprdCamera3Blur() {
    HAL_LOGD("E");
    mCaptureThread = NULL;
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }
    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION   : getCameraBlur
 *
 * DESCRIPTION: Creates Camera Blur if not created
 *
 * PARAMETERS:
 *   @pCapture               : Pointer to retrieve Camera Blur
 *
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3Blur::getCameraBlur(SprdCamera3Blur **pBlur) {
    *pBlur = NULL;

    if (!mBlur) {
        mBlur = new SprdCamera3Blur();
    }
    CHECK_BLUR();
    *pBlur = mBlur;
    HAL_LOGV("mBlur: %p ", mBlur);

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
int SprdCamera3Blur::get_camera_info(__unused int camera_id,
                                     struct camera_info *info) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    if (info) {
        rc = mBlur->getCameraInfo(info);
    }
    HAL_LOGV("X, rc: %d", rc);

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
int SprdCamera3Blur::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    HAL_LOGV("id= %d", atoi(id));
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = mBlur->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGV("id= %d, rc: %d", atoi(id), rc);

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
int SprdCamera3Blur::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mBlur->closeCameraDevice();
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
int SprdCamera3Blur::closeCameraDevice() {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;
    HAL_LOGD("E");

    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdCam = &m_pPhyCamera[i];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL) {
            continue;
        }
        HAL_LOGW("camera id:%d", i);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }

    freeLocalCapBuffer();
    mSavedRequestList.clear();
    mCaptureThread->unLoadBlurApi();

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
int SprdCamera3Blur::initialize(__unused const struct camera3_device *device,
                                const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_BLUR_ERROR();
    rc = mBlur->initialize(callback_ops);

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
const camera_metadata_t *SprdCamera3Blur::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    if (!mBlur) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mBlur->constructDefaultRequestSettings(device, type);

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
int SprdCamera3Blur::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_BLUR_ERROR();

    rc = mBlur->configureStreams(device, stream_list);

    HAL_LOGV(" X");

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
int SprdCamera3Blur::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_BLUR_ERROR();
    rc = mBlur->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3Blur
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BLUR();
    mBlur->processCaptureResultMain((camera3_capture_result_t *)result);
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
void SprdCamera3Blur::notifyMain(const struct camera3_callback_ops *ops,
                                 const camera3_notify_msg_t *msg) {
    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    CHECK_BLUR();
    mBlur->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3Blur
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BLUR();
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: Aux sensor  notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::notifyAux(const struct camera3_callback_ops *ops,
                                const camera3_notify_msg_t *msg) {
    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    CHECK_BLUR();
}

/*===========================================================================
 * FUNCTION   :allocateOne
 *
 * DESCRIPTION: deconstructor of SprdCamera3Blur
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3Blur::allocateOne(int w, int h, uint32_t is_cache,
                                 new_ion_mem_blur_t *new_mem) {
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;
    private_handle_t *buffer;

    mem_size = w * h * 3 / 2;
    // to make it page size aligned
    //  mem_size = (mem_size + 4095U) & (~4095U);

    if (!mIommuEnabled) {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM);
        }
    } else {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        HAL_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        HAL_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    buffer =
        new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION, 0x130,
                             mem_size, (unsigned char *)pHeapIon->getBase(), 0);

    buffer->share_fd = pHeapIon->getHeapID();
    buffer->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    buffer->byte_stride = w;
    buffer->internal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    buffer->width = w;
    buffer->height = h;
    buffer->stride = w;
    buffer->internalWidth = w;
    buffer->internalHeight = h;

    new_mem->native_handle = buffer;
    new_mem->pHeapIon = pHeapIon;

    HAL_LOGD("X fd=0x%x, size=0x%x, heap=%p", buffer->share_fd, mem_size,
             pHeapIon);

    return result;

getpmem_fail:
    delete pHeapIon;

    return -1;
}

/*===========================================================================
 * FUNCTION   : freeLocalCapBuffer
 *
 * DESCRIPTION: free new_ion_mem_blur_t buffer
 *
 * PARAMETERS:
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3Blur::freeLocalCapBuffer() {
    for (size_t i = 0; i < BLUR_LOCAL_CAPBUFF_NUM; i++) {
        new_ion_mem_blur_t *localBuffer = &mLocalCapBuffer[i];
        if (localBuffer->native_handle != NULL) {
            delete (private_handle_t *)*(&localBuffer->native_handle);
            localBuffer->native_handle = NULL;
        }
        if (localBuffer->pHeapIon != NULL) {
            delete localBuffer->pHeapIon;
            localBuffer->pHeapIon = NULL;
        }
    }
}

/*===========================================================================
 * FUNCTION   :validateCaptureRequest
 *
 * DESCRIPTION: validate request received from framework
 *
 * PARAMETERS :
 *  @request:   request received from framework
 *
 * RETURN     :
 *  NO_ERROR if the request is normal
 *  BAD_VALUE if the request if improper
 *==========================================================================*/
int SprdCamera3Blur::validateCaptureRequest(
    camera3_capture_request_t *request) {
    size_t idx = 0;
    const camera3_stream_buffer_t *b = NULL;

    /* Sanity check the request */
    if (request == NULL) {
        HAL_LOGE("NULL capture request");
        return BAD_VALUE;
    }

    uint32_t frameNumber = request->frame_number;
    if (request->num_output_buffers < 1 || request->output_buffers == NULL) {
        HAL_LOGE("Request %d: No output buffers provided!", frameNumber);
        return BAD_VALUE;
    }

    if (request->input_buffer != NULL) {
        b = request->input_buffer;
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %zu: Status not OK!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %zu: Has a release fence!",
                     frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %zu: NULL buffer handle!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
    }

    // Validate all output buffers
    b = request->output_buffers;
    do {
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %zu: Status not OK!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %zu: Has a release fence!",
                     frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %zu: NULL buffer handle!", frameNumber,
                     idx);
            return BAD_VALUE;
        }
        idx++;
        b = request->output_buffers + idx;
    } while (idx < (size_t)request->num_output_buffers);

    return NO_ERROR;
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
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Blur::cameraDeviceOpen(__unused int camera_id,
                                      struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGD(" E");
    hw_device_t *hw_dev[m_nPhyCameras];
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode((multiCameraMode)camera_id);
        rc = hw->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
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
    mCaptureThread->loadBlurApi();

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
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Blur::getCameraInfo(struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = m_VirtualCamera.id;
    HAL_LOGD("E, camera_id = %d", camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }
    CameraMetadata metadata = mStaticMetadata;
    SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size =
        BLUR_IMG_BUFFER_SIZE;
    metadata.update(
        ANDROID_JPEG_MAX_SIZE,
        &(SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size), 1);
    mStaticMetadata = metadata.release();

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    HAL_LOGD("X  max_size=%d",
             SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size);
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
int SprdCamera3Blur::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_BLUR_MAIN_ID;
    if (2 == m_nPhyCameras) {
        m_pPhyCamera[CAM_TYPE_AUX].id = CAM_BLUR_AUX_ID;
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   :CaptureThread
 *
 * DESCRIPTION: constructor of CaptureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Blur::CaptureThread::CaptureThread() {
    HAL_LOGD(" E");
    mCaptureMsgList.clear();
}

/*===========================================================================
 * FUNCTION   :~~CaptureThread
 *
 * DESCRIPTION: deconstructor of CaptureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Blur::CaptureThread::~CaptureThread() {
    HAL_LOGD(" E");
    mCaptureMsgList.clear();
}

/*===========================================================================
 * FUNCTION   :loadBlurApi
 *
 * DESCRIPTION: loadBlurApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Blur::CaptureThread::loadBlurApi() {
    HAL_LOGD("E");
    const char *error = NULL;
    int i = 0;

    for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
        mBlurApi[i] = (BlurAPI_t *)malloc(sizeof(BlurAPI_t));
        if (mBlurApi[i] == NULL) {
            HAL_LOGE("mBlurApi malloc failed.");
        }
        memset(mBlurApi[i], 0, sizeof(BlurAPI_t));
        HAL_LOGD("i = %d", i);
        if (i == 0) {
            mBlurApi[i]->handle = dlopen(BLUR_LIB_BOKEH_PREVIEW, RTLD_LAZY);
            if (mBlurApi[i]->handle == NULL) {
                error = dlerror();
                HAL_LOGE("open Blur API failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothInit =
                (int (*)(void **handle, int width, int height, float min_slope,
                         float max_slope, float Findex2Gamma_AdjustRatio))
                    dlsym(mBlurApi[i]->handle, "iSmoothInit");
            if (mBlurApi[i]->iSmoothInit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothInit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothDeinit = (int (*)(void *handle))dlsym(
                mBlurApi[i]->handle, "iSmoothDeinit");
            if (mBlurApi[i]->iSmoothDeinit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothDeinit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothCreateWeightMap =
                (int (*)(void *handle, int sel_x, int sel_y, int F_number,
                         int CircleSize))dlsym(mBlurApi[i]->handle,
                                               "iSmoothCreateWeightMap");
            if (mBlurApi[i]->iSmoothCreateWeightMap == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothCreateWeightMap failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothBlurImage =
                (int (*)(void *handle, unsigned char *Src_YUV,
                         unsigned char *Output_YUV))dlsym(mBlurApi[i]->handle,
                                                          "iSmoothBlurImage");
            if (mBlurApi[i]->iSmoothBlurImage == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothBlurImage failed.error = %s", error);
                return -1;
            }
        } else {
            mBlurApi[i]->handle = dlopen(BLUR_LIB_BOKEH_CAPTURE, RTLD_LAZY);
            if (mBlurApi[i]->handle == NULL) {
                error = dlerror();
                HAL_LOGE("open Blur API failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothCapInit =
                (int (*)(void **handle, int width, int height, float min_slope,
                         float max_slope, float Findex2Gamma_AdjustRatio,
                         int scaleSmoothWidth,
                         int scalerSmoothHeight))dlsym(mBlurApi[i]->handle,
                                                       "iSmoothCapInit");
            if (mBlurApi[i]->iSmoothCapInit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothCapInit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothDeinit = (int (*)(void *handle))dlsym(
                mBlurApi[i]->handle, "iSmoothCapDeinit");
            if (mBlurApi[i]->iSmoothDeinit == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothDeinit failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothCreateWeightMap =
                (int (*)(void *handle, int sel_x, int sel_y, int F_number,
                         int CircleSize))dlsym(mBlurApi[i]->handle,
                                               "iSmoothCapCreateWeightMap");
            if (mBlurApi[i]->iSmoothCreateWeightMap == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothCreateWeightMap failed.error = %s", error);
                return -1;
            }
            mBlurApi[i]->iSmoothBlurImage =
                (int (*)(void *handle, unsigned char *Src_YUV,
                         unsigned char *Output_YUV))
                    dlsym(mBlurApi[i]->handle, "iSmoothCapBlurImage");
            if (mBlurApi[i]->iSmoothBlurImage == NULL) {
                error = dlerror();
                HAL_LOGE("sym iSmoothBlurImage failed.error = %s", error);
                return -1;
            }
        }
        mBlurApi[i]->mHandle = NULL;
    }
    HAL_LOGD("load blur Api succuss.");

    return 0;
}

/*===========================================================================
 * FUNCTION   :unLoadBlurApi
 *
 * DESCRIPTION: unLoadBlurApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::unLoadBlurApi() {
    HAL_LOGD("E");
    int i = 0;
    for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
        if (mBlurApi[i] != NULL) {
            if (mBlurApi[i]->mHandle != NULL) {
                mBlurApi[i]->iSmoothDeinit(mBlurApi[i]->mHandle);
                mBlurApi[i]->mHandle = NULL;
            }
            if (mBlurApi[i]->handle != NULL) {
                dlclose(mBlurApi[i]->handle);
                mBlurApi[i]->handle = NULL;
            }
            free(mBlurApi[i]);
            mBlurApi[i] = NULL;
        }
    }
    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION   :blurHandle
 *
 * DESCRIPTION: blurHandle
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Blur::CaptureThread::blurHandle(
    struct private_handle_t *input, struct private_handle_t *output) {
    int ret = 0;
    unsigned char *srcYUV = (unsigned char *)(input->base);
    unsigned char *destYUV = NULL;
    int f_number = mBlur->mFNum;
    int CircleSize = mBlur->mCircleSize;
    int sel_x = mBlur->mSelCoordX;
    int sel_y = mBlur->mSelCoordY;
    int libid = 0;
    bool needInit = false;
    int i = 0;

    if (output != NULL) {
        destYUV = (unsigned char *)(output->base);
        libid = BLUR_LIB_BOKEH_NUM - 1;
    }
    char prop1[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.camera.blur.minslope", prop1, "0");
    if (0 != atoi(prop1) && mLastMinScope != atoi(prop1)) {
        mLastMinScope = atoi(prop1);
        needInit = true;
    }
    char prop2[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.camera.blur.maxslope", prop2, "0");
    if (0 != atoi(prop2) && mLastMaxScope != atoi(prop2)) {
        mLastMaxScope = atoi(prop2);
        needInit = true;
    }
    char prop3[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.sys.camera.blur.adjustratio", prop3, "0");
    if (0 != atoi(prop3) && mLastAdjustRati != atoi(prop3)) {
        mLastAdjustRati = atoi(prop3);
        needInit = true;
    }
    {
        Mutex::Autolock l(mMergequeueMutex);
        if (true == needInit) {
            float min_slope = (float)mLastMinScope / 1000;
            float max_slope = (float)mLastMaxScope / 1000;
            float Findex2Gamma_AdjustRatio = (float)mLastAdjustRati / 1000;
            int w = 0;
            int h = 0;
            for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
                if (mBlurApi[i]->mHandle != NULL) {
                    int64_t deinitStart = systemTime();
                    ret = mBlurApi[i]->iSmoothDeinit(mBlurApi[i]->mHandle);
                    if (ret != 0) {
                        HAL_LOGE("Deinit Err:%d", ret);
                    }
                    mBlurApi[i]->mHandle = NULL;
                    HAL_LOGD("iSmoothDeinit%d cost %lld ms", i,
                             ns2ms(systemTime() - deinitStart));
                }
                int64_t initStart = systemTime();
                if (0 == i) {
                    w = mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]
                            ->width;
                    h = mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]
                            ->height;
                    ret = mBlurApi[i]->iSmoothInit(&(mBlurApi[i]->mHandle), w,
                                                   h, min_slope, max_slope,
                                                   Findex2Gamma_AdjustRatio);
                } else {
                    w = mBlur->mCaptureWidth;
                    h = mBlur->mCaptureHeight;
                    ret = mBlurApi[i]->iSmoothCapInit(
                        &(mBlurApi[i]->mHandle), w, h, min_slope, max_slope,
                        Findex2Gamma_AdjustRatio, w / BLUR_SMOOTH_SIZE_SCALE,
                        h / BLUR_SMOOTH_SIZE_SCALE);
                }
                HAL_LOGD("iSmoothInit%d %dx%d cost %lld ms", i, w, h,
                         ns2ms(systemTime() - initStart));
                if (ret != 0) {
                    HAL_LOGE("Init Err:%d", ret);
                }
            }
        }
        if (mLastSelCoordX != sel_x || mLastSelCoordY != sel_y ||
            CircleSize != mLastCircleSize || f_number != mLastFNum) {
            mLastSelCoordX = sel_x;
            mLastSelCoordY = sel_y;
            mLastCircleSize = CircleSize;
            mLastFNum = f_number;

            for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
                int x = 0;
                int y = 0;
                int c = 0;
                int64_t creatStart = systemTime();
                if (1 == i) {
                    x = sel_x * mBlur->mCaptureWidth /
                        mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]
                            ->width;
                    y = sel_y * mBlur->mCaptureHeight /
                        mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]
                            ->height;
                    c = CircleSize * mBlur->mCaptureHeight /
                        mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]
                            ->height;
                    if (mBlur->mCaptureWidth < mBlur->mCaptureHeight) {
                        c = CircleSize * mBlur->mCaptureWidth /
                            mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]
                                ->width;
                    }
                } else {
                    x = sel_x;
                    y = sel_y;
                    c = CircleSize;
                }
                ret = mBlurApi[i]->iSmoothCreateWeightMap(mBlurApi[i]->mHandle,
                                                          x, y, f_number, c);
                if (ret != 0) {
                    HAL_LOGE("CreateWeightMap Err:%d", ret);
                }
                HAL_LOGD("iSmoothCreateWeightMap x:%d y:%d f:%d circle%d cost "
                         "%lld ms",
                         x, y, f_number, c, ns2ms(systemTime() - creatStart));
            }
        }

        int64_t blurStart = systemTime();
        ret = mBlurApi[libid]->iSmoothBlurImage(mBlurApi[libid]->mHandle,
                                                srcYUV, destYUV);
        if (ret != 0) {
            HAL_LOGE("BlurImage Err:%d", ret);
        }
        if (libid) {
            HAL_LOGD("capture iSmoothBlurImage cost %lld ms",
                     ns2ms(systemTime() - blurStart));
        } else {
            HAL_LOGD("preview BlurImage cost %lld ms",
                     ns2ms(systemTime() - blurStart));
        }
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION: threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3Blur::CaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    blur_queue_msg_t capture_msg;
    HAL_LOGD("run");

    while (!mCaptureMsgList.empty()) {
        List<blur_queue_msg_t>::iterator itor1 = mCaptureMsgList.begin();
        capture_msg = *itor1;
        mCaptureMsgList.erase(itor1);
        switch (capture_msg.msg_type) {
        case BLUR_MSG_EXIT: {
            // flush queue
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            HAL_LOGD("BLUR_MSG_EXIT");
            return false;
        } break;
        case BLUR_MSG_DATA_PROC: {
            HAL_LOGD("BLUR_MSG_DATA_PROC for frame idx:%d",
                     capture_msg.combo_buff.frame_number);
            output_buffer = &mBlur->mLocalCapBuffer[1].native_handle;

            // construct refocus data
            uint32_t MainWidethData = mBlur->mCaptureWidth;
            uint32_t MainHeightData = mBlur->mCaptureHeight;
            uint32_t MainSizeData = MainWidethData * MainHeightData * 3 / 2;
            uint32_t FNum = mBlur->mFNum;
            uint32_t CircleSize =
                mBlur->mCircleSize * mBlur->mCaptureHeight /
                (mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]->height);
            if (mBlur->mCaptureWidth < mBlur->mCaptureHeight) {
                CircleSize =
                    mBlur->mCircleSize * mBlur->mCaptureWidth /
                    (mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]->width);
            }
            uint32_t MinScope = mLastMinScope;
            uint32_t MaxScope = mLastMaxScope;
            uint32_t AdjustRati = mLastAdjustRati;
            uint32_t SelCoordX =
                mBlur->mSelCoordX * mBlur->mCaptureWidth /
                (mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]->width);
            uint32_t SelCoordY =
                mBlur->mSelCoordY * mBlur->mCaptureHeight /
                (mBlur->mSavedReqStreams[mBlur->mPreviewStreamsNum]->height);
            uint32_t scaleSmoothWidth =
                mBlur->mCaptureWidth / BLUR_SMOOTH_SIZE_SCALE;
            uint32_t scaleSmoothHeight =
                mBlur->mCaptureHeight / BLUR_SMOOTH_SIZE_SCALE;

            unsigned char BlurFlag[] = {'B', 'L', 'U', 'R'};
            unsigned char *p[] = {(unsigned char *)&MainWidethData,
                                  (unsigned char *)&MainHeightData,
                                  (unsigned char *)&MainSizeData,
                                  (unsigned char *)&FNum,
                                  (unsigned char *)&CircleSize,
                                  (unsigned char *)&MinScope,
                                  (unsigned char *)&MaxScope,
                                  (unsigned char *)&AdjustRati,
                                  (unsigned char *)&SelCoordX,
                                  (unsigned char *)&SelCoordY,
                                  (unsigned char *)&scaleSmoothWidth,
                                  (unsigned char *)&scaleSmoothHeight,
                                  (unsigned char *)&BlurFlag};

            uint32_t buffer_size =
                ((struct private_handle_t *)*mSavedResultBuff)->size;
            uint32_t jpeg_size =
                buffer_size - (MainSizeData + BLUR_REFOCUS_PARAM_NUM * 4);

            HAL_LOGD("buffer size:%d,jpeg size:%d, yuv size:%d", buffer_size,
                     jpeg_size, MainSizeData);

            unsigned char *buffer_base =
                (unsigned char *)((struct private_handle_t *)*mSavedResultBuff)
                    ->base;
            HAL_LOGD("buffer_base=%p", buffer_base);

            // cpoy refocus param to tail
            int i = 0;
            buffer_base += (buffer_size - BLUR_REFOCUS_PARAM_NUM * 4);
            HAL_LOGD("refocus param base=%p", buffer_base);
            for (i = 0; i < BLUR_REFOCUS_PARAM_NUM; i++) {
                memcpy(buffer_base + i * 4, p[i], 4);
            }

            // cpoy yuv to before refocus param
            buffer_base -= MainSizeData;
            HAL_LOGD("yuv base=%p, ", buffer_base);
            memcpy(buffer_base,
                   ((struct private_handle_t *)*capture_msg.combo_buff.buffer)
                       ->base,
                   MainSizeData);

            char prop1[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.blur1", prop1, "0");
            if (1 == atoi(prop1)) {
                mBlur->dumpData(buffer_base, 1, MainSizeData, MainWidethData,
                                MainHeightData);
            }
            if (!mBlur->mFlushing) {
                blurHandle(
                    (struct private_handle_t *)*(capture_msg.combo_buff.buffer),
                    (struct private_handle_t *)*output_buffer);
            }
            char prop2[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.blur2", prop2, "0");
            if (1 == atoi(prop2)) {
                unsigned char *buffer_base =
                    (unsigned char *)((struct private_handle_t *)*output_buffer)
                        ->base;
                mBlur->dumpData(buffer_base, 2,
                                mBlur->mCaptureWidth * mBlur->mCaptureHeight *
                                    3 / 2,
                                mBlur->mCaptureWidth, mBlur->mCaptureHeight);
            }
            camera3_capture_request_t request;
            camera3_stream_buffer_t *output_buffers = NULL;
            camera3_stream_buffer_t *input_buffer = NULL;

            memset(&request, 0x00, sizeof(camera3_capture_request_t));

            output_buffers = (camera3_stream_buffer_t *)malloc(
                sizeof(camera3_stream_buffer_t));
            if (!output_buffers) {
                HAL_LOGE("failed");
                return false;
            }
            memset(output_buffers, 0x00, sizeof(camera3_stream_buffer_t));

            input_buffer = (camera3_stream_buffer_t *)malloc(
                sizeof(camera3_stream_buffer_t));
            if (!input_buffer) {
                HAL_LOGE("failed");
                return false;
            }
            memset(input_buffer, 0x00, sizeof(camera3_stream_buffer_t));

            memcpy((void *)&request, &mSavedCapRequest,
                   sizeof(camera3_capture_request_t));
            request.settings = mSavedCapReqsettings;

            memcpy(input_buffer, &mSavedCapReqstreambuff,
                   sizeof(camera3_stream_buffer_t));
            input_buffer->stream = &mMainStreams[mCaptureStreamsNum - 1];
            input_buffer->stream->width = mBlur->mCaptureWidth;
            input_buffer->stream->height = mBlur->mCaptureHeight;
            input_buffer->buffer = output_buffer;

            memcpy((void *)&output_buffers[0], &mSavedCapReqstreambuff,
                   sizeof(camera3_stream_buffer_t));
            output_buffers[0].stream = &mMainStreams[mCaptureStreamsNum - 1];
            output_buffers[0].stream->width = mBlur->mCaptureWidth;
            output_buffers[0].stream->height = mBlur->mCaptureHeight;

            request.output_buffers = output_buffers;
            request.input_buffer = input_buffer;
            mReprocessing = true;

            ((struct private_handle_t *)(*request.output_buffers[0].buffer))
                ->size = jpeg_size;
            if (0 > mDevMain->hwi->process_capture_request(mDevMain->dev,
                                                           &request)) {
                HAL_LOGE("failed. process capture request!");
            }
            mBlur->mIsWaitSnapYuv = false;
            HAL_LOGD("jpeg request ok");
            if (input_buffer != NULL) {
                free((void *)input_buffer);
                input_buffer = NULL;
            }
            if (output_buffers != NULL) {
                free((void *)output_buffers);
                output_buffers = NULL;
            }
        } break;
        default:
            break;
        }
    };

    waitMsgAvailable();

    return true;
}

/*===========================================================================
 * FUNCTION   :requestExit
 *
 * DESCRIPTION: request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::requestExit() {
    blur_queue_msg_t blur_msg;
    blur_msg.msg_type = BLUR_MSG_EXIT;
    mCaptureMsgList.push_back(blur_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION   :getStreamType
 *
 * DESCRIPTION: return stream type
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3Blur::getStreamType(camera3_stream_t *new_stream) {
    int stream_type = 0;
    int format = new_stream->format;
    if (new_stream->stream_type == CAMERA3_STREAM_OUTPUT) {
        if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            format = HAL_PIXEL_FORMAT_YCrCb_420_SP;

        switch (format) {
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            if (new_stream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                stream_type = VIDEO_STREAM;
            } else if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                stream_type = CALLBACK_STREAM;
            } else {
                stream_type = PREVIEW_STREAM;
            }
            break;
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                stream_type = DEFAULT_STREAM;
            }
            break;
        case HAL_PIXEL_FORMAT_BLOB:
            stream_type = SNAPSHOT_STREAM;
            break;
        default:
            stream_type = DEFAULT_STREAM;
            break;
        }
    } else if (new_stream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
        stream_type = CALLBACK_STREAM;
    }

    return stream_type;
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty()) {
        if (mBlur->mFlushing) {
            Mutex::Autolock l(mBlur->mMergequeueFinishMutex);
            mBlur->mMergequeueFinishSignal.signal();
            HAL_LOGD("send mMergequeueFinishSignal.signal");
        }
        {
            Mutex::Autolock l(mMergequeueMutex);
            mMergequeueSignal.waitRelative(mMergequeueMutex,
                                           BLUR_THREAD_TIMEOUT);
        }
    }
}

/*===========================================================================
 * FUNCTION   :dumpFps
 *
 * DESCRIPTION: dump frame rate
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::CaptureThread::dumpFps() {
    mVFrameCount++;
    int64_t now = systemTime();
    int64_t diff = now - mVLastFpsTime;
    if (diff > ms2ns(250)) {
        mVFps =
            (((double)(mVFrameCount - mVLastFrameCount)) * (double)(s2ns(1))) /
            (double)diff;
        HAL_LOGD("[KPI Perf]: PROFILE_BLUR_FRAMES_PER_SECOND: %.4f ", mVFps);
        mVLastFpsTime = now;
        mVLastFrameCount = mVFrameCount;
    }
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
void SprdCamera3Blur::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    CHECK_BLUR();

    mBlur->_dump(device, fd);

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
int SprdCamera3Blur::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_BLUR_ERROR();

    rc = mBlur->_flush(device);

    HAL_LOGV(" X");

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
int SprdCamera3Blur::initialize(const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGD("E");
    CHECK_HWI_ERROR(hwiMain);

    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mCaptureThread->mCaptureStreamsNum = 0;
    mCaptureThread->mReprocessing = false;
    mCaptureThread->mLastCircleSize = 0;
    mCaptureThread->mLastFNum = 0;
    mCaptureThread->mLastMinScope = 5;
    mCaptureThread->mLastMaxScope = 50;
    mCaptureThread->mLastAdjustRati = 6000;
    mjpegSize = 0;
    mFNum = 1;
    mCircleSize = 120;
    mFlushing = false;
    mIsWaitSnapYuv = false;

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }
    if (m_nPhyCameras == 2) {
        sprdCam = m_pPhyCamera[CAM_TYPE_AUX];
        SprdCamera3HWI *hwiAux = sprdCam.hwi;
        CHECK_HWI_ERROR(hwiAux);

        rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error aux camera while initialize !! ");
            return rc;
        }
    }
    // init buffer_handle_t
    memset(mLocalCapBuffer, 0,
           sizeof(new_ion_mem_blur_t) * BLUR_LOCAL_CAPBUFF_NUM);

    mCaptureThread->mCallbackOps = callback_ops;
    mCaptureThread->mDevMain = &m_pPhyCamera[CAM_TYPE_MAIN];
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
int SprdCamera3Blur::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGV("E");

    int rc = 0;
    camera3_stream_t *pMainStreams[BLUR_MAX_NUM_STREAMS];
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;
    struct stream_info_s stream_info;
    float min_slope = (float)(mCaptureThread->mLastMinScope) / 1000;
    float max_slope = (float)(mCaptureThread->mLastMaxScope) / 1000;
    float Findex2Gamma_AdjustRatio =
        (float)(mCaptureThread->mLastAdjustRati) / 1000;

    Mutex::Autolock l(mLock);

    HAL_LOGD("configurestreams, stream num:%d", stream_list->num_streams);
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        if (requestStreamType == PREVIEW_STREAM) {
            mPreviewStreamsNum = i;
            if (mCaptureThread->mBlurApi[0]->mHandle != NULL) {
                int64_t deinitStart = systemTime();
                rc = mCaptureThread->mBlurApi[0]->iSmoothDeinit(
                    mCaptureThread->mBlurApi[0]->mHandle);
                if (rc != 0) {
                    HAL_LOGE("Deinit Err:%d", rc);
                }
                mCaptureThread->mBlurApi[0]->mHandle = NULL;
                HAL_LOGD("iSmoothDeinit0 cost %lld ms",
                         ns2ms(systemTime() - deinitStart));
            }
            int64_t initStart = systemTime();
            rc = mCaptureThread->mBlurApi[0]->iSmoothInit(
                &(mCaptureThread->mBlurApi[0]->mHandle),
                stream_list->streams[i]->width, stream_list->streams[i]->height,
                min_slope, max_slope, Findex2Gamma_AdjustRatio);
            if (rc != 0) {
                HAL_LOGD("init Err:%d", rc);
            }
            HAL_LOGD("iSmoothInit%d %dx%d cost %lld ms", i,
                     stream_list->streams[i]->width,
                     stream_list->streams[i]->height,
                     ns2ms(systemTime() - initStart));
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            char prop[PROPERTY_VALUE_MAX] = {
                0,
            };
            w = stream_list->streams[i]->width;
            h = stream_list->streams[i]->height;

            property_get("persist.sys.camera.resize5m", prop, "0");
            if (h == 1944 && 1 != atoi(prop)) {
                h = 1952;
            }

            if (mCaptureWidth != w && mCaptureHeight != h) {
                freeLocalCapBuffer();
                for (size_t j = 0; j < BLUR_LOCAL_CAPBUFF_NUM; j++) {
                    if (0 > allocateOne(w, h, 1, &(mLocalCapBuffer[j]))) {
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                }
            }
            mCaptureWidth = w;
            mCaptureHeight = h;
            mCaptureThread->mCaptureStreamsNum = stream_list->num_streams;
            mCaptureThread->mMainStreams[stream_list->num_streams].max_buffers =
                1;
            mCaptureThread->mMainStreams[stream_list->num_streams].width = w;
            mCaptureThread->mMainStreams[stream_list->num_streams].height = h;
            mCaptureThread->mMainStreams[stream_list->num_streams].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mCaptureThread->mMainStreams[stream_list->num_streams].usage =
                stream_list->streams[i]->usage;
            mCaptureThread->mMainStreams[stream_list->num_streams].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mCaptureThread->mMainStreams[stream_list->num_streams].data_space =
                stream_list->streams[i]->data_space;
            mCaptureThread->mMainStreams[stream_list->num_streams].rotation =
                stream_list->streams[i]->rotation;
            pMainStreams[stream_list->num_streams] =
                &mCaptureThread->mMainStreams[stream_list->num_streams];
            if (BLUR_LIB_BOKEH_NUM - 1) {
                if (mCaptureThread->mBlurApi[1]->mHandle != NULL) {
                    int64_t deinitStart = systemTime();
                    rc = mCaptureThread->mBlurApi[1]->iSmoothDeinit(
                        mCaptureThread->mBlurApi[1]->mHandle);
                    if (rc != 0) {
                        HAL_LOGE("Deinit Err:%d", rc);
                    }
                    mCaptureThread->mBlurApi[1]->mHandle = NULL;
                    HAL_LOGD("iSmoothDeinit1 cost %lld ms",
                             ns2ms(systemTime() - deinitStart));
                }
                int64_t initStart = systemTime();
                rc = mCaptureThread->mBlurApi[1]->iSmoothCapInit(
                    &(mCaptureThread->mBlurApi[1]->mHandle), w, h, min_slope,
                    max_slope, Findex2Gamma_AdjustRatio,
                    w / BLUR_SMOOTH_SIZE_SCALE, h / BLUR_SMOOTH_SIZE_SCALE);
                if (rc != 0) {
                    HAL_LOGD("Capinit Err:%d", rc);
                }
                HAL_LOGD("iSmoothInit%d %dx%d cost %lld ms", i, w, h,
                         ns2ms(systemTime() - initStart));
            }
        }
        mCaptureThread->mMainStreams[i] = *stream_list->streams[i];
        pMainStreams[i] = &mCaptureThread->mMainStreams[i];
    }
    mSelCoordX = stream_list->streams[mPreviewStreamsNum]->width / 2;
    mSelCoordY = stream_list->streams[mPreviewStreamsNum]->height / 2;
    mCaptureThread->mLastSelCoordX = 0;
    mCaptureThread->mLastSelCoordY = 0;

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = stream_list->num_streams + 1;
    mainconfig.streams = pMainStreams;

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    if (mainconfig.num_streams == 3) {
        HAL_LOGD("push back to streamlist");
        memcpy(stream_list->streams[0], &mCaptureThread->mMainStreams[0],
               sizeof(camera3_stream_t));
        memcpy(stream_list->streams[1], &mCaptureThread->mMainStreams[1],
               sizeof(camera3_stream_t));
        stream_list->streams[1]->width = mCaptureWidth;
        stream_list->streams[1]->height = mCaptureHeight;
    }
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD("main configurestreams, streamtype:%d, format:%d, width:%d, "
                 "height:%d",
                 stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->format,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height);
    }

    mCaptureThread->run(String8::format("Blur").string());

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
const camera_metadata_t *SprdCamera3Blur::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
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
void SprdCamera3Blur::saveRequest(camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            request_saved_blur_t currRequest;
            HAL_LOGD("save request %d", request->frame_number);
            Mutex::Autolock l(mRequestLock);
            currRequest.frame_number = request->frame_number;
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            mSavedRequestList.push_back(currRequest);
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
int SprdCamera3Blur::processCaptureRequest(const struct camera3_device *device,
                                           camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metaSettings;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_stream_buffer_t *out_streams_main = NULL;
    bool is_captureing = false;
    uint32_t tagCnt = 0;

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    metaSettings = request->settings;
    saveRequest(req);

    tagCnt = metaSettings.entryCount();
    if (tagCnt != 0) {
        uint8_t sprdBurstModeEnabled = 0;
        metaSettings.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                            &sprdBurstModeEnabled, 1);
        metaSettings.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                            &sprdBurstModeEnabled, 1);
        sprdBurstModeEnabled =
            metaSettings.find(ANDROID_SPRD_BURSTMODE_ENABLED).data.u8[0];
        uint8_t sprdZslEnabled = 1;
        metaSettings.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
        metaSettings.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
        sprdZslEnabled = metaSettings.find(ANDROID_SPRD_ZSL_ENABLED).data.u8[0];
    }

    if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
        uint32_t fnum =
            metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
        if (fnum < 1) {
            mFNum = 1;
        } else if (fnum > 20) {
            mFNum = 20;
        } else {
            mFNum = fnum;
        }
    }

    /*config main camera*/
    req_main = *req;
    out_streams_main = (camera3_stream_buffer_t *)malloc(
        sizeof(camera3_stream_buffer_t) * (req_main.num_output_buffers));
    if (!out_streams_main) {
        HAL_LOGE("failed");
        return NO_MEMORY;
    }
    memset(out_streams_main, 0x00,
           (sizeof(camera3_stream_buffer_t)) * (req_main.num_output_buffers));

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        out_streams_main[i] = req->output_buffers[i];
        HAL_LOGD("num_output_buffers:%d, streamtype:%d",
                 req->num_output_buffers, requestStreamType);
        if (requestStreamType == SNAPSHOT_STREAM) {
            HAL_LOGD("jpeg orgtype:%d w%d,h%d",
                     req->output_buffers[i].stream->format,
                     (req->output_buffers[i].stream)->width,
                     (req->output_buffers[i].stream)->height);
            mCaptureThread->mSavedResultBuff =
                request->output_buffers[i].buffer;
            mjpegSize = ((struct private_handle_t *)(*request->output_buffers[i]
                                                          .buffer))
                            ->size;
            memcpy(&mCaptureThread->mSavedCapRequest, req,
                   sizeof(camera3_capture_request_t));
            memcpy(&mCaptureThread->mSavedCapReqstreambuff,
                   &req->output_buffers[i], sizeof(camera3_stream_buffer_t));

            if (NULL != mCaptureThread->mSavedCapReqsettings) {
                free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                mCaptureThread->mSavedCapReqsettings = NULL;
            }
            mCaptureThread->mSavedCapReqsettings =
                clone_camera_metadata(req_main.settings);
            req_main.settings = mCaptureThread->mSavedCapReqsettings;

            if (mFlushing) {
                rc = hwiMain->process_capture_request(
                    m_pPhyCamera[CAM_TYPE_MAIN].dev, request);

                HAL_LOGD("mFlushing rc, d%d", rc);
                return rc;
            }

            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1] =
                req->output_buffers[i].stream;
            out_streams_main[i].stream =
                &mCaptureThread
                     ->mMainStreams[mCaptureThread->mCaptureStreamsNum];
            out_streams_main[i].stream->width = mCaptureWidth;
            out_streams_main[i].stream->height = mCaptureHeight;
            out_streams_main[i].buffer = &mLocalCapBuffer[0].native_handle;
            mIsCapturing = true;
            is_captureing = true;
            mIsWaitSnapYuv = true;
        } else {
            mSavedReqStreams[mPreviewStreamsNum] =
                req->output_buffers[i].stream;
            out_streams_main[i].stream =
                &mCaptureThread->mMainStreams[mPreviewStreamsNum];
            if (metaSettings.exists(ANDROID_CONTROL_AF_REGIONS)) {
                uint32_t x =
                    metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
                uint32_t y =
                    metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
                if (x != 0 && y != 0) {
                    mSelCoordX =
                        x * (req->output_buffers[i].stream)->width / 3692;
                    mSelCoordY =
                        y * (req->output_buffers[i].stream)->height / 2496;
                    HAL_LOGD("x=%d, y=%d  mSelCoordX=%d, mSelCoordY=%d", x, y,
                             mSelCoordX, mSelCoordY);
                }
            }
            if (metaSettings.exists(ANDROID_SPRD_BLUR_CIRCLE_SIZE)) {
                uint32_t circle =
                    metaSettings.find(ANDROID_SPRD_BLUR_CIRCLE_SIZE)
                        .data.i32[0];
                if (circle != 0) {
                    uint32_t max = (req->output_buffers[i].stream)->height /
                                   BLUR_CIRCLE_SIZE_SCALE;
                    if ((req->output_buffers[i].stream)->width <
                        (req->output_buffers[i].stream)->height) {
                        max = (req->output_buffers[i].stream)->width /
                              BLUR_CIRCLE_SIZE_SCALE;
                    }
                    mCircleSize =
                        max - (max - BLUR_CIRCLE_VALUE_MIN) * circle / 255;
                    HAL_LOGD("circle=%d, max=%d  mCircleSize=%d", circle, max,
                             mCircleSize);
                }
            }
        }
    }
    req_main.output_buffers = out_streams_main;
    req_main.settings = metaSettings.release();
    if (req_main.output_buffers[0].stream->format == HAL_PIXEL_FORMAT_BLOB) {
        HAL_LOGD("capture request, idx:%d", req_main.frame_number);
        req_main.output_buffers[0].stream->format =
            HAL_PIXEL_FORMAT_YCbCr_420_888;
    }
    HAL_LOGD("mIsCapturing:%d, framenumber=%d", mIsCapturing,
             request->frame_number);
    HAL_LOGV("start main, idx:%d", req_main.frame_number);
    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                          &req_main);
    if (rc < 0) {
        HAL_LOGE("failed, idx:%d", req_main.frame_number);
        goto req_fail;
    }

req_fail:
    HAL_LOGD("rc, d%d", rc);

    if (req_main.output_buffers != NULL) {
        free((void *)req_main.output_buffers);
        req_main.output_buffers = NULL;
    }

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
void SprdCamera3Blur::notifyMain(const camera3_notify_msg_t *msg) {
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        msg->message.shutter.frame_number ==
            mCaptureThread->mSavedCapRequest.frame_number) {
        if (mCaptureThread->mReprocessing) {
            HAL_LOGD(" hold cap notify");
            return;
        }
        HAL_LOGD(" cap notify");
    }
    mCaptureThread->mCallbackOps->notify(mCaptureThread->mCallbackOps, msg);
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
void SprdCamera3Blur::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metadata;
    metadata = result->result;

    /* Direclty pass preview buffer and meta result for Main camera */
    if (result_buffer == NULL) {
        if (result->frame_number ==
                mCaptureThread->mSavedCapRequest.frame_number &&
            0 != result->frame_number) {
            if (mCaptureThread->mReprocessing) {
                HAL_LOGD("hold yuv picture call back, framenumber:%d",
                         result->frame_number);
                if (NULL != mCaptureThread->mSavedCapReqsettings) {
                    free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                    mCaptureThread->mSavedCapReqsettings = NULL;
                }
                return;
            } else {
                mCaptureThread->mCallbackOps->process_capture_result(
                    mCaptureThread->mCallbackOps, result);
                return;
            }
        }
        if (2 == m_nPhyCameras) {
            int mCoveredValue = getCoveredValue(metadata);
            if (cur_frame_number > 100) {
                metadata.update(ANDROID_SPRD_BLUR_COVERED, &mCoveredValue, 1);
                result->result = metadata.release();
            }
        }
        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, result);

        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    if (mIsCapturing && currStreamType == DEFAULT_STREAM) {
        HAL_LOGD("framenumber:%d, currStreamType:%d, stream:%p, buffer:%p, "
                 "format:%d",
                 result->frame_number, currStreamType, result_buffer->stream,
                 result_buffer->buffer, result_buffer->stream->format);
        HAL_LOGD("main yuv picture: format:%d, width:%d, height:%d, buff:%p",
                 result_buffer->stream->format, result_buffer->stream->width,
                 result_buffer->stream->height, result_buffer->buffer);

        blur_queue_msg_t capture_msg;
        capture_msg.msg_type = BLUR_MSG_DATA_PROC;
        capture_msg.combo_buff.frame_number = result->frame_number;
        capture_msg.combo_buff.buffer = result->output_buffers->buffer;
        capture_msg.combo_buff.input_buffer = result->input_buffer;
        {
            hwiMain->setMultiCallBackYuvMode(false);
            Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
            mCaptureThread->mCaptureMsgList.push_back(capture_msg);
            mCaptureThread->mMergequeueSignal.signal();
        }
    } else if (mIsCapturing && currStreamType == SNAPSHOT_STREAM) {
        camera3_capture_result_t newResult = *result;
        camera3_stream_buffer_t newOutput_buffers = *(result->output_buffers);
        HAL_LOGD("jpegst result stream:%p, result buffer:%p, savedreqstream:%p",
                 result->output_buffers[0].stream,
                 result->output_buffers[0].buffer,
                 mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1]);
        memcpy(mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1],
               result->output_buffers[0].stream, sizeof(camera3_stream_t));
        newOutput_buffers.stream =
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1];
        newOutput_buffers.buffer = result->output_buffers->buffer;

        newResult.output_buffers = &newOutput_buffers;
        newResult.input_buffer = NULL;
        newResult.result = NULL;
        newResult.partial_result = 0;
        ((struct private_handle_t *)*result->output_buffers->buffer)->size =
            mjpegSize;
        {
            char prop3[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.blur3", prop3, "0");
            if (1 == atoi(prop3)) {
                unsigned char *buffer_base =
                    (unsigned char *)(((struct private_handle_t *)*(
                                           result->output_buffers->buffer))
                                          ->base);
                dumpData(buffer_base, 3,
                         (int)((struct private_handle_t *)*(
                                   result->output_buffers->buffer))
                             ->size,
                         mCaptureWidth, mCaptureHeight);
            }
            char prop4[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.sys.camera.blur4", prop4, "0");
            if (1 == atoi(prop4)) {
                unsigned char *buffer_base =
                    (unsigned char *)(((struct private_handle_t *)*(
                                           result->output_buffers->buffer))
                                          ->base);
                buffer_base += (int)(((struct private_handle_t *)*(
                                          result->output_buffers->buffer))
                                         ->size -
                                     BLUR_REFOCUS_PARAM_NUM * 4);
                mBlur->dumpData(buffer_base, 4, BLUR_REFOCUS_PARAM_NUM, 4, 0);
            }
        }
        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, &newResult);
        mCaptureThread->mReprocessing = false;
        mIsCapturing = false;

    } else {
        camera3_capture_result_t newResult;
        camera3_stream_buffer_t newOutput_buffers;
        memset(&newResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
        {
            Mutex::Autolock l(mRequestLock);
            List<request_saved_blur_t>::iterator i = mSavedRequestList.begin();
            while (i != mSavedRequestList.end()) {
                if (i->frame_number == result->frame_number) {
                    if (result->output_buffers->status !=
                        CAMERA3_BUFFER_STATUS_ERROR) {
                        mCaptureThread->blurHandle(
                            (struct private_handle_t *)*(
                                result->output_buffers->buffer),
                            NULL);
                    }
                    memcpy(&newResult, result,
                           sizeof(camera3_capture_result_t));
                    memcpy(&newOutput_buffers, &result->output_buffers[0],
                           sizeof(camera3_stream_buffer_t));
                    newOutput_buffers.stream = i->stream;
                    memcpy(newOutput_buffers.stream,
                           result->output_buffers[0].stream,
                           sizeof(camera3_stream_t));
                    newResult.output_buffers = &newOutput_buffers;
                    mCaptureThread->mCallbackOps->process_capture_result(
                        mCaptureThread->mCallbackOps, &newResult);
                    mSavedRequestList.erase(i);
                    HAL_LOGD("find preview frame %d", result->frame_number);
                    break;
                }
                i++;
            }
        }
    }

    return;
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
void SprdCamera3Blur::_dump(const struct camera3_device *device, int fd) {
    HAL_LOGD(" E");

    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION   :dumpData
 *
 * DESCRIPTION: dump data
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::dumpData(unsigned char *addr, int type, int size,
                               int param1, int param2) {
    HAL_LOGD(" E %p %d %d %d %d", addr, type, size, param1, param2);
    char name[128];
    FILE *fp = NULL;
    switch (type) {
    case 1:
    case 2: {
        snprintf(name, sizeof(name), "/data/misc/cameraserver/%d_%d_%d.yuv",
                 param1, param2, type);
        fp = fopen(name, "w");
        if (fp == NULL) {
            HAL_LOGE("open yuv file fail!\n");
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case 3: {
        snprintf(name, sizeof(name), "/data/misc/cameraserver/%d_%d_%d.jpg",
                 param1, param2, type);
        fp = fopen(name, "wb");
        if (fp == NULL) {
            HAL_LOGE("can not open file: %s \n", name);
            return;
        }
        fwrite((void *)addr, 1, size, fp);
        fclose(fp);
    } break;
    case 4: {
        int i = 0;
        int j = 0;
        snprintf(name, sizeof(name),
                 "/data/misc/cameraserver/refocus_%d_params.txt", size);
        fp = fopen(name, "w+");
        if (fp == NULL) {
            HAL_LOGE("open txt file fail!\n");
            return;
        }
        for (i = 0; i < size; i++) {
            int result = 0;
            if (i == size - 1) {
                for (j = 0; j < param1; j++) {
                    fprintf(fp, "%c", addr[i * param1 + j]);
                }
            } else {
                for (j = 0; j < param1; j++) {
                    result += addr[i * param1 + j] << j * 8;
                }
                fprintf(fp, "%d\n", result);
            }
        }
        fclose(fp);
    } break;
    default:
        break;
    }
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
int SprdCamera3Blur::_flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGD("E flush, mCaptureMsgList.size=%zu, mSavedRequestList.size:%zu",
             mCaptureThread->mCaptureMsgList.size(), mSavedRequestList.size());
    mFlushing = true;

    while (mIsCapturing && mIsWaitSnapYuv) {
        Mutex::Autolock l(mMergequeueFinishMutex);
        mMergequeueFinishSignal.waitRelative(mMergequeueFinishMutex,
                                             BLUR_THREAD_TIMEOUT);
    }
    HAL_LOGD("wait until mCaptureMsgList.empty");

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);
    if (2 == m_nPhyCameras) {
        SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
        rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);
    }
    if (mCaptureThread != NULL) {
        if (mCaptureThread->isRunning()) {
            mCaptureThread->requestExit();
        }
    }
    mFlushing = false;
    HAL_LOGD("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :convertToRegions
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Blur::convertToRegions(int32_t *rect, int32_t *region,
                                       int weight) {
    region[0] = rect[0];
    region[1] = rect[1];
    region[2] = rect[2];
    region[3] = rect[3];
    if (weight > -1) {
        region[4] = weight;
    }
}

/*===========================================================================
 * FUNCTION   :getCoveredValue
 *
 * DESCRIPTION: get Sub Camera Cover value
 *
 * PARAMETERS :
 *
 *
 * RETURN     : value
 *==========================================================================*/
uint8_t SprdCamera3Blur::getCoveredValue(CameraMetadata &frame_settings) {
    int rc = 0;
    uint32_t couvered_value = 0;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("debug.camera.covered", prop, "0");

    SprdCamera3HWI *hwiSub = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiSub->getCoveredValue(&couvered_value);
    if (rc < 0) {
        HAL_LOGD("read sub sensor failed");
    }
    if (0 != atoi(prop)) {
        couvered_value = atoi(prop);
    }
    if (couvered_value < MAX_CONVERED_VALURE && couvered_value) {
        couvered_value = BLUR_SELFSHOT_CONVERED;
    } else {
        couvered_value = BLUR_SELFSHOT_NO_CONVERED;
    }
    HAL_LOGD("get cover_value %u", couvered_value);
    // update face[10].score info to mean convered value when api1 is used
    {
        FACE_Tag *faceDetectionInfo = (FACE_Tag *)&(
            hwiSub->mSetting->s_setting[CAM_BLUR_AUX_ID].faceInfo);
        uint8_t numFaces = faceDetectionInfo->face_num;
        uint8_t faceScores[CAMERA3MAXFACE];
        uint8_t dataSize = CAMERA3MAXFACE;
        int32_t faceRectangles[CAMERA3MAXFACE * 4];
        int j = 0;

        numFaces = CAMERA3MAXFACE;
        for (int i = 0; i < numFaces; i++) {
            faceScores[i] = faceDetectionInfo->face[i].score;
            if (faceScores[i] == 0) {

                faceScores[i] = 1;
            }
            convertToRegions(faceDetectionInfo->face[i].rect,
                             faceRectangles + j, -1);
            j += 4;
        }
        faceScores[10] = couvered_value;

        frame_settings.update(ANDROID_STATISTICS_FACE_SCORES, faceScores,
                              dataSize);
        frame_settings.update(ANDROID_STATISTICS_FACE_RECTANGLES,
                              faceRectangles, dataSize * 4);
    }
    return couvered_value;
}
};
