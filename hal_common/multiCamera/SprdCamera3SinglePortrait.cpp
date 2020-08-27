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
#define LOG_TAG "Cam3SinglePortrait"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL) 
#include "SprdCamera3SinglePortrait.h"
#include <cutils/trace.h> 
using namespace android;

namespace sprdcamera {

#define FRONT_MASTER_ID 1
#define BACK_MASTER_ID 0

SprdCamera3SinglePortrait *mSinglePortrait = NULL;

// Error Check Macros
#define CHECK_BLUR()                                                           \
    if (!mSinglePortrait) {                                                    \
        HAL_LOGE("Error getting blur ");                                       \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_BLUR_ERROR()                                                     \
    if (!mSinglePortrait) {                                                    \
        HAL_LOGE("Error getting blur ");                                       \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : -(x))
#endif

#ifndef FACE_INFO_SCALE
#define FACE_SCREEN_EFFECT_FACTOR 5
#define FACE_EFFECT_FACTOR 5
#define FACE_INFO_SCALE(initwidth, initheight, x1, y1, x2, y2)                 \
    initwidth *initheight *FACE_SCREEN_EFFECT_FACTOR /                         \
        ((ABS(x2 - x1) * ABS(x2 - x1) + ABS(y2 - y1) * ABS(y2 - y1)) *         \
         FACE_EFFECT_FACTOR)
#endif

#define MAX_UPDATE_TIME 3000
#define Mask_DepthW 800
#define Mask_DepthH 600

camera3_device_ops_t SprdCamera3SinglePortrait::mCameraCaptureOps = {
    .initialize = SprdCamera3SinglePortrait::initialize,
    .configure_streams = SprdCamera3SinglePortrait::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3SinglePortrait::construct_default_request_settings,
    .process_capture_request =
        SprdCamera3SinglePortrait::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3SinglePortrait::dump,
    .flush = SprdCamera3SinglePortrait::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3SinglePortrait::callback_ops_main = {
    .process_capture_result =
        SprdCamera3SinglePortrait::process_capture_result_main,
    .notify = SprdCamera3SinglePortrait::notifyMain};

camera3_callback_ops SprdCamera3SinglePortrait::callback_ops_aux = {
    .process_capture_result =
        SprdCamera3SinglePortrait::process_capture_result_aux,
    .notify = SprdCamera3SinglePortrait::notifyAux};

/*===========================================================================
 * FUNCTION   : SprdCamera3SinglePortrait
 *
 * DESCRIPTION: SprdCamera3SinglePortrait Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3SinglePortrait::SprdCamera3SinglePortrait() {
    HAL_LOGI(" E");

    memset(&m_VirtualCamera, 0, sizeof(sprd_virtual_camera_t));
    memset(&mLocalCapBuffer, 0, sizeof(new_mem_t) * BLUR_LOCAL_CAPBUFF_NUM);
    memset(&mSavedReqStreams, 0,
           sizeof(camera3_stream_t *) * BLUR_MAX_NUM_STREAMS);
#ifdef CONFIG_FACE_BEAUTY
    memset(&fbLevels, 0, sizeof(faceBeautyLevels));
    memset(&fbLevels_cap, 0, sizeof(faceBeautyLevels));
    memset(&fb_prev,0,sizeof(fb_beauty_param_t));
    memset(&fb_cap,0,sizeof(fb_beauty_param_t));
    mFaceBeautyFlag = false;
#endif
    mBlurMode = 0;
    mFirstSprdFB = 0;
    bzero(&beauty_image, sizeof(beauty_image));
    bzero(&beauty_face, sizeof(beauty_face));
    m_VirtualCamera.id = CAM_BLUR_MAIN_ID;
    mStaticMetadata = NULL;
    mCaptureThread = new CaptureThread();
    mSavedRequestList.clear();
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mjpegSize = 0;
    mNearJpegSize = 0;
    mFarJpegSize = 0;
    m_pNearJpegBuffer = NULL;
    m_pFarJpegBuffer = NULL;
    m_pNearYuvBuffer = NULL;
    weight_map = NULL;
    mCameraId = CAM_BLUR_MAIN_ID;
    mFlushing = false;
    mInitThread = false;
    mReqState = PREVIEW_REQUEST_STATE;
    mPerfectskinlevel = 0;
    mCoverValue = 1;
    m_pPhyCamera = NULL;
    m_nPhyCameras = 0;
    mSnapshotResultReturn = false;
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3SinglePortrait
 *
 * DESCRIPTION: SprdCamera3SinglePortrait Desctructor
 *
 *==========================================================================*/
SprdCamera3SinglePortrait::~SprdCamera3SinglePortrait() {
    HAL_LOGI("E");
    mCaptureThread = NULL;
    if (mStaticMetadata)
        free_camera_metadata(mStaticMetadata);
    HAL_LOGI("X");
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
void SprdCamera3SinglePortrait::getCameraBlur(
    SprdCamera3SinglePortrait **pBlur) {
    *pBlur = NULL;

    if (!mSinglePortrait) {
        mSinglePortrait = new SprdCamera3SinglePortrait();
    }
    CHECK_BLUR();
    *pBlur = mSinglePortrait;
    HAL_LOGV("mSinglePortrait: %p ", mSinglePortrait);

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
int SprdCamera3SinglePortrait::get_camera_info(__unused int camera_id,
                                               struct camera_info *info) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    if (info) {
        rc = mSinglePortrait->getCameraInfo(camera_id, info);
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
int SprdCamera3SinglePortrait::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    HAL_LOGV("id= %d", atoi(id));
    rc = mSinglePortrait->cameraDeviceOpen(atoi(id), hw_device);
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
int SprdCamera3SinglePortrait::close_camera_device(
    __unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mSinglePortrait->closeCameraDevice();
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
int SprdCamera3SinglePortrait::closeCameraDevice() {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;
    HAL_LOGI("E");

    mFlushing = true;
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

    if (mCaptureThread != NULL) {
        if (mCaptureThread->isRunning()) {
            mCaptureThread->requestExit();
        }
        mInitThread = false;
    }

    freeLocalCapBuffer();
    mSavedRequestList.clear();
    mCaptureThread->unLoadBlurApi();
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }

    HAL_LOGI("X, rc: %d", rc);

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
int SprdCamera3SinglePortrait::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_BLUR_ERROR();
    rc = mSinglePortrait->initialize(callback_ops);

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
SprdCamera3SinglePortrait::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    if (!mSinglePortrait) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mSinglePortrait->constructDefaultRequestSettings(device, type);

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
int SprdCamera3SinglePortrait::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_BLUR_ERROR();

    rc = mSinglePortrait->configureStreams(device, stream_list);

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
int SprdCamera3SinglePortrait::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_BLUR_ERROR();
    rc = mSinglePortrait->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3SinglePortrait
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SinglePortrait::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BLUR();
    mSinglePortrait->processCaptureResultMain(
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
void SprdCamera3SinglePortrait::notifyMain(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_BLUR();
    mSinglePortrait->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3SinglePortrait
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SinglePortrait::process_capture_result_aux(
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
void SprdCamera3SinglePortrait::notifyAux(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {
    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    CHECK_BLUR();
}

/*===========================================================================
 * FUNCTION   : freeLocalCapBuffer
 *
 * DESCRIPTION: free new_mem_t buffer
 *
 * PARAMETERS:
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3SinglePortrait::freeLocalCapBuffer() {
    for (size_t i = 0; i < BLUR_LOCAL_CAPBUFF_NUM; i++) {
        new_mem_t *localBuffer = &mLocalCapBuffer[i];
        freeOneBuffer(localBuffer);
    }

    if (mCaptureThread->mOutWeightBuff != NULL) {
        free(mCaptureThread->mOutWeightBuff);
        mCaptureThread->mOutWeightBuff = NULL;
    }
    if (weight_map != NULL) {
        free(weight_map);
        weight_map = NULL;
    }
    if (m_pNearYuvBuffer != NULL) {
        free(m_pNearYuvBuffer);
        m_pNearYuvBuffer = NULL;
    }

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
int SprdCamera3SinglePortrait::cameraDeviceOpen(
    __unused int camera_id, struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;
    uint8_t master_id = 0;

    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.blur.cov.id", prop, "3");

    if (camera_id == SPRD_PORTRAIT_SINGLE_ID) {
        mCameraId = CAM_BLUR_MAIN_ID_2;
        master_id = FRONT_MASTER_ID;
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID_2;
        if (atoi(prop) == 1 || atoi(prop) == 2) {
            m_nPhyCameras = 2;
        } else {
            m_nPhyCameras = 1;
        }
    } else {
        mCameraId = CAM_BLUR_MAIN_ID;
        master_id = BACK_MASTER_ID;
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID;
        if (atoi(prop) == 0 || atoi(prop) == 2) {
            m_nPhyCameras = 2;
        } else {
            m_nPhyCameras = 1;
        }
    }
    hw_device_t *hw_dev[m_nPhyCameras];
    setupPhysicalCameras();

    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        HAL_LOGI("open %d", phyId);
        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode(MODE_BLUR);
        hw->setMasterId(master_id);
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
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3SinglePortrait::getCameraInfo(int blur_camera_id,
                                             struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;
    int32_t img_size = 0;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    char mPropSize[PROPERTY_VALUE_MAX] = {
        0,
    };
    int32_t jpeg_stream_size = 0;
    if (mStaticMetadata)
        free_camera_metadata(mStaticMetadata);

    if (blur_camera_id == SPRD_PORTRAIT_SINGLE_ID) {
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID_2;
        property_get("persist.vendor.cam.fr.blur.version", prop, "0");
    } else {
        m_VirtualCamera.id = CAM_BLUR_MAIN_ID;
        property_get("persist.vendor.cam.ba.blur.version", prop, "0");
    }

    camera_id = m_VirtualCamera.id;
    mCaptureThread->mVersion = atoi(prop);
    HAL_LOGI("E, camera_id = %d version:%d", camera_id,
             mCaptureThread->mVersion);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }
    CameraMetadata metadata = clone_camera_metadata(mStaticMetadata);
    if (blur_camera_id == SPRD_PORTRAIT_SINGLE_ID) {
        property_get("persist.vendor.cam.res.blur.fr", mPropSize, "RES_5M");
        HAL_LOGI("blur front support cap resolution %s", mPropSize);
    } else {
        property_get("persist.vendor.cam.res.blur.ba", mPropSize, "RES_5M");
        HAL_LOGI("blur back support cap resolution %s", mPropSize);
    }
    jpeg_stream_size = getJpegStreamSize(mPropSize);
    if (atoi(prop) == 3) {
        property_get("persist.vendor.cam.gallery.blur", prop, "1");
        if (atoi(prop) == 1) {
            mCaptureThread->mIsGalleryBlur = true;
            img_size =
                jpeg_stream_size * 3 +
                (BLUR_REFOCUS_PARAM2_NUM * 4) + 1024;
        } else {
            mCaptureThread->mIsGalleryBlur = false;
            img_size =
                jpeg_stream_size * 2 +
                jpeg_stream_size / 6 +
                (BLUR_REFOCUS_PARAM2_NUM * 4) + 1024;
        }

    } else {
        property_get("persist.vendor.cam.fr.blur.type", prop, "2");
        if (atoi(prop) == 2) {
            img_size =
                jpeg_stream_size * 2 +
                jpeg_stream_size / 48 +
                (BLUR_REFOCUS_PARAM_NUM * 4) + 1024;

        } else {
            img_size =
                jpeg_stream_size * 2 +
                (BLUR_REFOCUS_PARAM_NUM * 4) + 1024;
        }
    }
    SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size = img_size;
    metadata.update(
        ANDROID_JPEG_MAX_SIZE,
        &img_size, 1);

    addAvailableStreamSize(metadata, mPropSize);

    mStaticMetadata = metadata.release();

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;

    HAL_LOGI("X  max_size=%d", img_size);
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
int SprdCamera3SinglePortrait::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    if (mCameraId == 0) {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_BLUR_MAIN_ID;
        if (2 == m_nPhyCameras) {
            m_pPhyCamera[CAM_TYPE_AUX].id = CAM_BLUR_AUX_ID;
        }
    } else {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_BLUR_MAIN_ID_2;
        if (2 == m_nPhyCameras) {
            m_pPhyCamera[CAM_TYPE_AUX].id = CAM_BLUR_AUX_ID_2;
        }
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
SprdCamera3SinglePortrait::CaptureThread::CaptureThread()
    : mCallbackOps(NULL), mDevMain(NULL), mSavedResultBuff(NULL),
      mSavedCapReqsettings(NULL), mCaptureStreamsNum(0), 
      mFirstUpdateFrame(0), mLastMinScope(0), mLastMaxScope(0),
      mLastAdjustRati(0), mCircleSizeScale(0), mUpdataxy(0), mstartUpdate(0),
      mFirstCapture(false), mFirstPreview(false),
      mUpdateCaptureWeightParams(false), mUpdatePreviewWeightParams(false),
      mBuffSize(0), mWeightSize(0), mWeightWidth(0), mWeightHeight(0),
      mLastFaceNum(0), mRotation(0), mLastTouchX(0),
      mLastTouchY(0), mBlurBody(true), mUpdataTouch(false), mVersion(0),
      mIsGalleryBlur(false), mIsBlurAlways(false), mOutWeightBuff(NULL),
      mFirstSprdLightPortrait(false), mFirstSprdDfa(false)

{
    mFaceInfoX = 0;
    mFaceInfoY = 0;
    mGaussEnable = 0;
    lpt_cap_type = 0;
    fb_cap_type = 0;
    mAlgorithmFlag = false;
    prev_sensor_orientation = 0;
    lightPortraitType = 0;
    cameraBV = 0;
    cameraISO = 0;
    cameraCT = 0;
    lpt_return_val = 0;
    bzero(&fd_score, sizeof(int32_t)*FD_SCORE_SIZE);
    bzero(&mPreviewLptAndBeautyFaceinfo, sizeof(mPreviewLptAndBeautyFaceinfo));
    memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
    memset(&mMainStreams, 0, sizeof(camera3_stream_t) * BLUR_MAX_NUM_STREAMS);
    memset(mBlurApi, 0, sizeof(SinglePortraitAPI_t *) * BLUR_LIB_BOKEH_NUM);
    memset(mWinPeakPos, 0, sizeof(short) * BLUR_AF_WINDOW_NUM);
    memset(&mPreviewInitParams, 0,
           sizeof(preview_single_portrait_init_params_t));
    memset(&mPreviewWeightParams, 0,
           sizeof(preview_single_portrait_weight_params_t));
    memset(&mCaptureInitParams, 0,
           sizeof(capture_single_portrait_init_params_t));
    memset(&mCaptureWeightParams, 0,
           sizeof(capture_single_portrait_weight_params_t));
    memset(&mCapture2InitParams, 0,
           sizeof(capture2_single_portrait_init_params_t));
    memset(&mCapture2WeightParams, 0,
           sizeof(capture2_single_portrait_weight_params_t));
    memset(mFaceInfo, 0, sizeof(int32_t) * 4);
    memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
    memset(&mIspInfo, 0, sizeof(single_portrait_isp_info_t));

    memset(&faceDetectionInfo, 0, sizeof(FACE_Tag));
    memset(&lpt_prev, 0, sizeof(class_lpt));
    memset(&lpt_cap, 0, sizeof(class_lpt));
    memset(&dfa_prev, 0, sizeof(class_dfa));
    memset(&dfa_cap, 0, sizeof(class_dfa));
    memset(&lptOptions_prev, 0, sizeof(lpt_options));
    memset(&lptOptions_cap, 0, sizeof(lpt_options));
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
SprdCamera3SinglePortrait::CaptureThread::~CaptureThread() {
    HAL_LOGI(" E");
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
int SprdCamera3SinglePortrait::CaptureThread::loadBlurApi() {
    HAL_LOGI("E");
    const char *error = NULL;
    int i = 0;

    for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
        mBlurApi[i] =
            (SinglePortraitAPI_t *)malloc(sizeof(SinglePortraitAPI_t));
        if (mBlurApi[i] == NULL) {
            HAL_LOGE("mBlurApi malloc failed.");
            goto mem_fail;
        }
        memset(mBlurApi[i], 0, sizeof(SinglePortraitAPI_t));
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
                         float max_slope, float findex2gamma_adjust_ratio,
                         int box_filter_size))dlsym(mBlurApi[i]->handle,
                                                    "iSmoothInit");
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
                (int (*)(void *handle,
                         preview_single_portrait_weight_params_t *params))
                    dlsym(mBlurApi[i]->handle, "iSmoothCreateWeightMap");
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
        }
        mBlurApi[i]->mHandle = NULL;
    }

    HAL_LOGI("load blur Api succuss.");

    return 0;

mem_fail:
    for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++)
        if (mBlurApi[i] != NULL)
            free(mBlurApi[i]);

    return -1;
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
void SprdCamera3SinglePortrait::CaptureThread::unLoadBlurApi() {
    HAL_LOGI("E");
    int i = 0;
    for (i = 0; i < BLUR_LIB_BOKEH_NUM; i++) {
        if (mBlurApi[i] != NULL) {
            if (mBlurApi[i]->mHandle != NULL && (i != 1)) {
                mBlurApi[i]->iSmoothDeinit(mBlurApi[i]->mHandle);
                mBlurApi[i]->mHandle = NULL;
                int rc = deinitLightPortrait();
                if(rc != NO_ERROR) {
                    HAL_LOGE("fail to deinitLightPortrait");
                }
#ifdef CONFIG_FACE_BEAUTY
                rc = deinitFaceBeauty();
                if(rc != NO_ERROR) {
                    HAL_LOGE("fail to deinitFaceBeauty");
                }
#endif
            } else {
                sprd_portrait_capture_deinit(mBlurApi[i]->mHandle);
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
    
    if (mCaptureInitParams.cali_dist_seq != NULL) {
        free(mCaptureInitParams.cali_dist_seq);
        mCaptureInitParams.cali_dist_seq = NULL;
    }
    if (mCaptureInitParams.cali_dac_seq != NULL) {
        free(mCaptureInitParams.cali_dac_seq);
        mCaptureInitParams.cali_dac_seq = NULL;
    }
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :prevblurHandle
 *
 * DESCRIPTION: prevblurHandle
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3SinglePortrait::CaptureThread::prevBlurHandle(
    buffer_handle_t *input1, void *input1_addr, void *input2,
    buffer_handle_t *output, void *output_addr) {
    int ret = 0;
    unsigned char *srcYUV = NULL;
    unsigned char *destYUV = NULL;

    if (input1 != NULL) {
        srcYUV = (unsigned char *)input1_addr;
    }
    if (output != NULL) {
        destYUV = (unsigned char *)output_addr;
    }
    if (mFirstPreview) {
        mFirstPreview = false;
        if (mBlurApi[0]->mHandle != NULL) {
            int64_t deinitStart = systemTime();
            ret = mBlurApi[0]->iSmoothDeinit(mBlurApi[0]->mHandle);
            if (ret != 0) {
                HAL_LOGE("preview iSmoothDeinit Err:%d", ret);
            }
            mBlurApi[0]->mHandle = NULL;
            HAL_LOGD("preview iSmoothDeinit cost %lld ms",
                     ns2ms(systemTime() - deinitStart));
        }
        int64_t initStart = systemTime();

        ret = mBlurApi[0]->iSmoothInit(
            &(mBlurApi[0]->mHandle), mPreviewInitParams.width,
            mPreviewInitParams.height, mPreviewInitParams.min_slope,
            mPreviewInitParams.max_slope,
            mPreviewInitParams.findex2gamma_adjust_ratio,
            mPreviewInitParams.box_filter_size);

        HAL_LOGD("preview iSmoothInit cost %lld ms",
                 ns2ms(systemTime() - initStart));
        if (ret != 0) {
            HAL_LOGE("preview iSmoothInit Err:%d", ret);
        }
    }
    if (mUpdatePreviewWeightParams) {
        int64_t creatStart = systemTime();
        mUpdatePreviewWeightParams = false;

        HAL_LOGD("mPreviewWeightParams:%d %d %d %d %d %d",
                 mPreviewWeightParams.valid_roi, mPreviewWeightParams.roi_type,
                 mPreviewWeightParams.circle_size,
                 mPreviewWeightParams.f_number, mPreviewWeightParams.sel_x,
                 mPreviewWeightParams.sel_y);
        ret = mBlurApi[0]->iSmoothCreateWeightMap(mBlurApi[0]->mHandle,
                                                  &mPreviewWeightParams);
        if (ret != 0) {
            HAL_LOGE("preview CreateWeightMap Err:%d", ret);
        }
        HAL_LOGD("preview iSmoothCreateWeightMap cost %lld ms",
                 ns2ms(systemTime() - creatStart));
    }

    int64_t blurStart = systemTime();
    ret = mBlurApi[0]->iSmoothBlurImage(mBlurApi[0]->mHandle, srcYUV, NULL);
    if (ret != 0)
        LOGE("ismoothblur is error");
    HAL_LOGV("preview iSmoothBlurImage cost %lld ms",
             ns2ms(systemTime() - blurStart));

    return ret;
}
/*===========================================================================
 * FUNCTION   :capblurHandle
 *
 * DESCRIPTION: capblurHandle
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/

int SprdCamera3SinglePortrait::CaptureThread::capBlurHandle(
    buffer_handle_t *input1, void *input1_addr, void *input2,
    buffer_handle_t *output, void *output_addr, void *mask) {
    int ret = 0;
    unsigned char *srcYUV = NULL;
    unsigned char *destYUV = NULL;

    if (input1 != NULL) {
        srcYUV = (unsigned char *)input1_addr;
    }
    if (output != NULL) {
        destYUV = (unsigned char *)output_addr;
    } else {
        HAL_LOGE("Failed to get output buffer");
    }
    if (mFirstCapture) {
        mFirstCapture = false;
        mBuffSize = mWeightSize;
    }
    if (mBlurApi[1]->mHandle) {
        mAlgorithmFlag = true;
    } else {
        mAlgorithmFlag = false;
        ret = NO_ERROR;
    }
    HAL_LOGD("mAlgorithmFlag %d", mAlgorithmFlag);

    InoutYUV yuvData;
    memset(&yuvData, 0, sizeof(InoutYUV));
    yuvData.Src_YUV = srcYUV;
    yuvData.Dst_YUV = destYUV;

    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.noalgo.dump", prop, "0");
    if (!strcmp(prop, "1")) {
        mSinglePortrait->dumpData(
            srcYUV, 1, mSinglePortrait->mCaptureWidth *
                            mSinglePortrait->mCaptureHeight * 3 / 2,
            mSinglePortrait->mCaptureWidth, mSinglePortrait->mCaptureHeight,
            0, "srcYUV");
        mSinglePortrait->dumpData(
            destYUV, 1, mSinglePortrait->mCaptureWidth *
                            mSinglePortrait->mCaptureHeight * 3 / 2,
            mSinglePortrait->mCaptureWidth, mSinglePortrait->mCaptureHeight,
            0, "destYUV1");
    }

    int64_t blurStart = systemTime();
    if (mAlgorithmFlag) {
        ret = sprd_portrait_capture_process(mBlurApi[1]->mHandle, &yuvData,
                                            mask, 1);
    } else {
        memcpy(destYUV, srcYUV, mSinglePortrait->mCaptureWidth *
                                    mSinglePortrait->mCaptureHeight * 3 /
                                    2);
    }
    if (!strcmp(prop, "1")) {
        mSinglePortrait->dumpData(
            destYUV, 1, mSinglePortrait->mCaptureWidth *
                            mSinglePortrait->mCaptureHeight * 3 / 2,
            mSinglePortrait->mCaptureWidth, mSinglePortrait->mCaptureHeight,
            0, "destYUV2");
    }
    if (ret != 0)
        LOGE("ismooth is error");

    HAL_LOGD("mVersion:%d.%d capture sprd_portrait_capture_process cost %lld ms",
                mVersion, mCaptureWeightParams.roi_type,
                ns2ms(systemTime() - blurStart));

    mSinglePortrait->flushIonBuffer(ADP_BUFFD(*output), (void *)destYUV,
                                    ADP_BUFSIZE(*output));
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
bool SprdCamera3SinglePortrait::CaptureThread::threadLoop() {
    single_portrait_queue_msg_t capture_msg;
    HAL_LOGV("run");

    while (!mCaptureMsgList.empty()) {
        List<single_portrait_queue_msg_t>::iterator itor1 =
            mCaptureMsgList.begin();
        capture_msg = *itor1;
        mCaptureMsgList.erase(itor1);
        switch (capture_msg.msg_type) {
        case SINGLE_PORTRAIT_MSG_EXIT: {
            // flush queue
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqstreambuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            HAL_LOGD("SINGLE_PORTRAIT_MSG_EXIT");
            return false;
        } break;
        case SINGLE_PORTRAIT_MSG_DATA_PROC: {
            void *combo_buff_addr = NULL;
            void *output_buff_addr = NULL;
            buffer_handle_t *const output_buffer =
                &mSinglePortrait->mLocalCapBuffer[1].native_handle;

            if (mSinglePortrait->map(capture_msg.combo_buff.buffer,
                                     &combo_buff_addr) != NO_ERROR) {
                HAL_LOGE("map combo buffer(%p) failed",
                         capture_msg.combo_buff.buffer);
                return false;
            }
            if (mSinglePortrait->map(output_buffer, &output_buff_addr) !=
                NO_ERROR) {
                HAL_LOGE("map output buffer(%p) failed", output_buffer);
                return false;
            }

            HAL_LOGD("mFlushing:%d, frame idx:%d", mSinglePortrait->mFlushing,
                     capture_msg.combo_buff.frame_number);

            if (!mSinglePortrait->mFlushing) {
                // blur process
                switch (mVersion) {
                case 1: {
                    blurProcessVer1(capture_msg.combo_buff.buffer,
                                    combo_buff_addr, output_buffer,
                                    output_buff_addr,
                                    capture_msg.combo_buff.frame_number);
                } break;
                default:
                    HAL_LOGD("blur mVersion %d", mVersion);
                    HAL_LOGD("blurProcessVer error ");
                    break;
                }
            }
            if (combo_buff_addr != NULL) {
                mSinglePortrait->unmap(capture_msg.combo_buff.buffer);
                combo_buff_addr = NULL;
            }
            if (output_buff_addr != NULL) {
                mSinglePortrait->unmap(output_buffer);
                output_buff_addr = NULL;
            }

            // yuv reprocess
            if (yuvReprocessCaptureRequest(capture_msg.combo_buff.buffer,
                                           output_buffer) == false) {
                HAL_LOGE("yuv reprocess false");
                return false;
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
 * FUNCTION   :blurProcessVer1
 *
 * DESCRIPTION: blurProcessVer1
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3SinglePortrait::CaptureThread::blurProcessVer1(
    buffer_handle_t *combo_buffer, void *combo_buff_addr,
    buffer_handle_t *output_buffer, void *output_buff_addr,
    uint32_t combe_frm_num) {
    int ret = 0;
    void *nearJpegBufferAddr = NULL;
    unsigned char *outPortraitMask = NULL;
    int portrait_flag = mSinglePortrait->mBlurMode;
    int lightportrait_flag = lpt_cap_type;
    int facebeauty_flag = fb_cap_type;
    HAL_LOGD("portrait_flag:%d,lpt_flag:%d,facebeauty_flag:%d",portrait_flag,
        lightportrait_flag, facebeauty_flag);
    dump_single_portrait_t combo_buff, output_buff;
    dump_single_portrait_t *dump_buffs[DUMP_SINGLE_PORTRAIT_TYPE_MAX];
    memset(&combo_buff, 0, sizeof(dump_single_portrait_t));
    memset(&output_buff, 0, sizeof(dump_single_portrait_t));

    memset(&dump_buffs, 0,
           sizeof(dump_single_portrait_t *) * DUMP_SINGLE_PORTRAIT_TYPE_MAX);
    mSinglePortrait->m_pNearJpegBuffer =
        &mSinglePortrait->mLocalCapBuffer[2].native_handle;

    if (mSinglePortrait->map(mSinglePortrait->m_pNearJpegBuffer,
                             &nearJpegBufferAddr) == NO_ERROR) {
        mSinglePortrait->mNearJpegSize =
            mSinglePortrait->jpeg_encode_exif_simplify(
                combo_buffer, combo_buff_addr,
                mSinglePortrait->m_pNearJpegBuffer, nearJpegBufferAddr, NULL,
                NULL, mSinglePortrait->m_pPhyCamera[CAM_TYPE_MAIN].hwi,
                SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
                    .jpgInfo.orientation);
        mSinglePortrait->unmap(mSinglePortrait->m_pNearJpegBuffer);
        nearJpegBufferAddr = NULL;
    } else {
        HAL_LOGE("map m_pNearJpegBuffer(%p) failed",
                 mSinglePortrait->m_pNearJpegBuffer);
    }

    memcpy(mSinglePortrait->m_pNearYuvBuffer, combo_buff_addr,
           mSinglePortrait->mCaptureWidth * mSinglePortrait->mCaptureHeight *
               3 / 2);

    combo_buff.buffer_addr = combo_buff_addr;
    combo_buff.frame_number = combe_frm_num;
    dump_buffs[DUMP_SINGLE_PORTRAIT_COMBO] = &combo_buff;
    dumpBlurIMG(DUMP_SINGLE_PORTRAIT_COMBO, dump_buffs);

    if (!mSinglePortrait->mFlushing) {
        if (mCaptureWeightParams.roi_type == 2) {
            //                getOutWeightMap(mSavedResultBuff);
        }
        /*get portrait mask*/
        outPortraitMask = (unsigned char *)malloc(Mask_DepthW * Mask_DepthH * 2);
        if(!outPortraitMask) {
            HAL_LOGE("no mem!");
            return ret;
        }
        ret = getPortraitMask(output_buff_addr, combo_buff_addr, 0, outPortraitMask);
        if(!outPortraitMask) {
            HAL_LOGE("no thing!");
        }
        if(portrait_flag){
            ret = capBlurHandle(combo_buffer, combo_buff_addr,
                            mSinglePortrait->m_pNearYuvBuffer, output_buffer,
                            output_buff_addr, (void *)outPortraitMask);
        }
        if(facebeauty_flag){
            if(!portrait_flag){
                HAL_LOGD("feature support:no portrait+fb");
                memcpy(output_buff_addr,combo_buff_addr, ADP_BUFSIZE(*combo_buffer));
            } else {
                HAL_LOGD("feature support:portrait+fb");
            }
            ret = doFaceBeauty(outPortraitMask, output_buff_addr, mCaptureInitParams.width, 
                        mCaptureInitParams.height, 1, &mSinglePortrait->fbLevels_cap, lightportrait_flag);
        }
        if(lightportrait_flag != 0) {
            if(!portrait_flag && (!facebeauty_flag)){
                HAL_LOGD("feature support:lpt");
                memcpy(output_buff_addr,combo_buff_addr, ADP_BUFSIZE(*combo_buffer));
            } else if(portrait_flag) {
                HAL_LOGD("feature support:portrait + lpt");
            } else {
                HAL_LOGD("feature support:facebeauty + lpt");
            }
            ret = capLPT(output_buff_addr,mCaptureInitParams.width,mCaptureInitParams.height,
                    outPortraitMask, lightportrait_flag);
        }
        free(outPortraitMask);
        output_buff.buffer_addr = output_buff_addr;
        output_buff.frame_number = combe_frm_num;
        dump_buffs[DUMP_SINGLE_PORTRAIT_OUTPUT] = &output_buff;
        dumpBlurIMG(DUMP_SINGLE_PORTRAIT_OUTPUT, dump_buffs);
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   :CallSnapBackResult
 *
 * DESCRIPTION: CallSnapBackResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3SinglePortrait::CaptureThread::CallSnapBackResult(
    camera3_buffer_status_t buffer_status) {
    camera3_capture_result_t newResult;
    camera3_stream_buffer_t newOutput_buffers;

    memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
    memset(&newResult, 0, sizeof(camera3_capture_result_t));
    if (mSinglePortrait->mFlushing) {
        buffer_status = CAMERA3_BUFFER_STATUS_ERROR;
    }
    newOutput_buffers.stream =
        mSinglePortrait->mSavedReqStreams[mCaptureStreamsNum - 1];
    newOutput_buffers.buffer = mSavedResultBuff;
    newOutput_buffers.status = buffer_status;
    newOutput_buffers.acquire_fence = -1;
    newOutput_buffers.release_fence = -1;

    newResult.frame_number = mSavedCapRequest.frame_number;
    newResult.output_buffers = &newOutput_buffers;
    newResult.input_buffer = NULL;
    newResult.result = NULL;
    newResult.partial_result = 0;
    newResult.num_output_buffers = 1;
    HAL_LOGD("buffer_status%d frame_number:%d", buffer_status,
             newResult.frame_number);

    mCallbackOps->process_capture_result(mCallbackOps, &newResult);
}

/*===========================================================================
 * FUNCTION   :yuvReprocessCaptureRequest
 *
 * DESCRIPTION: yuvReprocessCaptureRequest
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3SinglePortrait::CaptureThread::yuvReprocessCaptureRequest(
    buffer_handle_t *combe_buffer, buffer_handle_t *output_buffer) {
    int mime_type = (int)SPRD_MIMETPYE_BLUR;
    camera3_capture_request_t request;
    camera3_stream_buffer_t output_stream_buff;
    camera3_stream_buffer_t input_stream_buff;

    memset(&request, 0x00, sizeof(camera3_capture_request_t));
    memset(&output_stream_buff, 0x00, sizeof(camera3_stream_buffer_t));
    memset(&input_stream_buff, 0x00, sizeof(camera3_stream_buffer_t));

    memcpy((void *)&request, &mSavedCapRequest,
           sizeof(camera3_capture_request_t));
    request.settings = mSavedCapReqsettings;

    memcpy((void *)&input_stream_buff, &mSavedCapReqstreambuff,
           sizeof(camera3_stream_buffer_t));
    input_stream_buff.stream = &mMainStreams[mCaptureStreamsNum - 1];
    input_stream_buff.stream->width = mSinglePortrait->mCaptureWidth;
    input_stream_buff.stream->height = mSinglePortrait->mCaptureHeight;
    if (mSinglePortrait->mFlushing) {
        mime_type = 0;
        input_stream_buff.buffer = combe_buffer;
    } else if (mSinglePortrait->mBlurMode == CAM_SINGLE_PORTRAIT_MODE ||
        mSinglePortrait->mBlurMode == CAM_BlUR_MODE) {
        input_stream_buff.buffer = output_buffer;
        mime_type = (int)SPRD_MIMETPYE_NONE;
    } else {
        input_stream_buff.buffer = output_buffer;
        mime_type = (int)SPRD_MIMETPYE_BLUR;
    }
    //mDevMain->hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE, &mime_type, NULL);
    memcpy((void *)&output_stream_buff, &mSavedCapReqstreambuff,
           sizeof(camera3_stream_buffer_t));
    output_stream_buff.stream->width = mSinglePortrait->mCaptureWidth;
    output_stream_buff.stream->height = mSinglePortrait->mCaptureHeight;
    request.output_buffers = &output_stream_buff;
    if (mVersion == 3 && mSinglePortrait->mReqState == WAIT_FIRST_YUV_STATE &&
        !mSinglePortrait->mFlushing) {
        output_stream_buff.stream = &mMainStreams[mCaptureStreamsNum];
        output_stream_buff.buffer =
            &mSinglePortrait->mLocalCapBuffer[0].native_handle;
        request.input_buffer = NULL;
        mSinglePortrait->mReqState = WAIT_SECOND_YUV_STATE;
#ifdef BLUR_V3_TAKE_3_YUV
    } else if (mIsGalleryBlur &&
               mSinglePortrait->mReqState == WAIT_SECOND_YUV_STATE &&
               !mSinglePortrait->mFlushing) {
        output_stream_buff.stream = &mMainStreams[mCaptureStreamsNum];
        output_stream_buff.buffer =
            &mSinglePortrait->mLocalCapBuffer[0].native_handle;
        request.input_buffer = NULL;
        mSinglePortrait->mReqState = WAIT_THIRD_YUV_STATE;
    } else {
        if (mSinglePortrait->mReqState == WAIT_THIRD_YUV_STATE) {
            input_stream_buff.buffer = combe_buffer;
        }
#else
    } else {
#endif
        output_stream_buff.stream = &mMainStreams[mCaptureStreamsNum - 1];
        request.input_buffer = &input_stream_buff;

        mSinglePortrait->mReqState = REPROCESS_STATE;
    }
    request.num_output_buffers = 1;
    if (mSinglePortrait->mFlushing) {
        CallSnapBackResult(CAMERA3_BUFFER_STATUS_ERROR);
    } else {
        if (0 > mDevMain->hwi->process_capture_request(mDevMain->dev, &request))
            HAL_LOGE("failed. process capture request!");
    }
    if (NULL != mSavedCapReqsettings &&
        mSinglePortrait->mReqState == REPROCESS_STATE) {
        free_camera_metadata(mSavedCapReqsettings);
        mSavedCapReqsettings = NULL;
    }
    HAL_LOGD("mReqState:%d", mSinglePortrait->mReqState);

    return true;
}
/*===========================================================================
 * FUNCTION   :dumpBlurIMG
 *
 * DESCRIPTION: dumpBlurIMG
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3SinglePortrait::CaptureThread::dumpBlurIMG(
    dump_single_portrait_type type,
    dump_single_portrait_t *dump_buffs[DUMP_SINGLE_PORTRAIT_TYPE_MAX]) {

    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    unsigned char *buffer_base = NULL;
    dump_single_portrait_t *combo_buff = dump_buffs[DUMP_SINGLE_PORTRAIT_COMBO];
    dump_single_portrait_t *output_buff =
        dump_buffs[DUMP_SINGLE_PORTRAIT_OUTPUT];
    dump_single_portrait_t *result_buff =
        dump_buffs[DUMP_SINGLE_PORTRAIT_RESULT];

    switch (type) {
    case DUMP_SINGLE_PORTRAIT_COMBO:
        property_get("persist.vendor.cam.blur.dump", prop, "0");

        if ((!strcmp(prop, "combo") || !strcmp(prop, "all")) &&
            !mSinglePortrait->mFlushing && combo_buff != NULL) {
            buffer_base = (unsigned char *)combo_buff->buffer_addr;
            mSinglePortrait->dumpData(
                buffer_base, 1, mSinglePortrait->mCaptureWidth *
                                    mSinglePortrait->mCaptureHeight * 3 / 2,
                mSinglePortrait->mCaptureWidth, mSinglePortrait->mCaptureHeight,
                combo_buff->frame_number, "src");
        }
        break;

    case DUMP_SINGLE_PORTRAIT_OUTPUT:
        property_get("persist.vendor.cam.blur.dump", prop, "0");

        if ((!strcmp(prop, "output") || !strcmp(prop, "all")) &&
            combo_buff != NULL && output_buff != NULL) {
            buffer_base = (unsigned char *)output_buff->buffer_addr;
            mSinglePortrait->dumpData(
                buffer_base, 1, mSinglePortrait->mCaptureWidth *
                                    mSinglePortrait->mCaptureHeight * 3 / 2,
                mSinglePortrait->mCaptureWidth, mSinglePortrait->mCaptureHeight,
                output_buff->frame_number, "blur_output");

            buffer_base = (unsigned char *)combo_buff->buffer_addr;
            mSinglePortrait->dumpData(
                buffer_base, 1, mSinglePortrait->mCaptureWidth *
                                    mSinglePortrait->mCaptureHeight * 3 / 2,
                mSinglePortrait->mCaptureWidth, mSinglePortrait->mCaptureHeight,
                combo_buff->frame_number, "blur_yuv");
        }
        break;

    case DUMP_SINGLE_PORTRAIT_RESULT:
        property_get("persist.vendor.cam.blur.dump", prop, "0");
        if ((!strcmp(prop, "result") || !strcmp(prop, "all")) &&
            result_buff != NULL) {

            uint32_t para_num = 0;
            buffer_base = (unsigned char *)result_buff->buffer_addr;
            uint32_t yuv_size =
                mCaptureInitParams.width * mCaptureInitParams.height * 3 / 2;
            uint32_t weight_map_size =
                mCaptureInitParams.width * mCaptureInitParams.height / 4;
            uint32_t output_weight_map = mWeightSize;
            uint32_t para_size = 0;
            uint32_t near_jpeg_size = mSinglePortrait->mNearJpegSize;
            uint32_t far_jpeg_size = mSinglePortrait->mFarJpegSize;
            
           {
                para_num += BLUR_REFOCUS_COMMON_PARAM_NUM +
                            BLUR_REFOCUS_2_PARAM_NUM + BLUR_AF_WINDOW_NUM +
                            BLUR_MAX_ROI * 5 +
                            mCaptureInitParams.cali_seq_len * 2;
                para_size = para_num * 4;
            }
            // dump jpeg
            mSinglePortrait->dumpData(buffer_base, 2, result_buff->jpeg_size,
                                      mSinglePortrait->mCaptureWidth,
                                      mSinglePortrait->mCaptureHeight, 0,
                                      "jpeg");
            // dump para
            buffer_base += (result_buff->use_size - para_size);
            mSinglePortrait->dumpData(buffer_base, 3, para_num, 4, 0, 0,
                                      "parameter");
            // dump near jpeg
            buffer_base -= near_jpeg_size;
            mSinglePortrait->dumpData(
                buffer_base, 2, near_jpeg_size, mSinglePortrait->mCaptureWidth,
                mSinglePortrait->mCaptureHeight, 0, "nearJpeg");
            // dump near yuv
            mSinglePortrait->dumpData(
                (unsigned char *)(mSinglePortrait->m_pNearYuvBuffer), 1,
                yuv_size, mCaptureInitParams.width, mCaptureInitParams.height,
                0, "nearYuv");
            
            if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2)) {
                // dump output weight map
                buffer_base -= output_weight_map;
                mSinglePortrait->dumpData(
                    buffer_base, 1, output_weight_map,
                    mCaptureInitParams.width / mCaptureInitParams.Scalingratio,
                    mCaptureInitParams.height / mCaptureInitParams.Scalingratio,
                    0, "outWeightMap");
            }
        }
        break;

    default:
        break;
    }
}

/*===========================================================================
 * FUNCTION   :initBlur20Params
 *
 * DESCRIPTION: init Blur 2.0 Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3SinglePortrait::CaptureThread::initBlur20Params() {
    memset(mWinPeakPos, 0x00, sizeof(short) * BLUR_AF_WINDOW_NUM);
    unsigned short cali_dist_seq_8M[] = {30, 40, 50,  60,  70,
                                         80, 90, 100, 110, 120};
    unsigned short cali_dac_seq_8M[] = {240, 225, 220, 215, 213,
                                        211, 209, 207, 205, 180};
    unsigned short cali_dist_seq_13M[] = {30, 40, 50,  60,  70,
                                          80, 90, 100, 110, 120};
    unsigned short cali_dac_seq_13M[] = {355, 340, 325, 300, 310,
                                         305, 302, 298, 295, 262};

    mCaptureInitParams.vcm_dac_up_bound = 0;
    mCaptureInitParams.vcm_dac_low_bound = 0;
    mCaptureInitParams.vcm_dac_info = NULL;
    mCaptureInitParams.vcm_dac_gain = 127;
    mCaptureInitParams.valid_depth_clip = 32;
    mCaptureInitParams.method = 0;
    mCaptureInitParams.row_num = 3;
    mCaptureInitParams.column_num = 3;
    mCaptureInitParams.boundary_ratio = 8;
    mCaptureInitParams.sel_size = 1;
    mCaptureInitParams.valid_depth = 8;
    mCaptureInitParams.slope = 0;
    mCaptureInitParams.valid_depth_up_bound = 100;
    mCaptureInitParams.valid_depth_low_bound = 0;
    mCaptureInitParams.cali_seq_len = 10;
    mCaptureInitParams.cali_dist_seq = (unsigned short *)malloc(
        sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    if (mCaptureInitParams.cali_dist_seq == NULL) {
        return -1;
    }
    mCaptureInitParams.cali_dac_seq = (unsigned short *)malloc(
        sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    if (mCaptureInitParams.cali_dac_seq == NULL) {
        goto mem_fail;
    }

    if (SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
            .sensor_InfoInfo.pixer_array_size[0] == 4160) {
        memcpy(mCaptureInitParams.cali_dist_seq, cali_dist_seq_13M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
        memcpy(mCaptureInitParams.cali_dac_seq, cali_dac_seq_13M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    } else {
        memcpy(mCaptureInitParams.cali_dist_seq, cali_dist_seq_8M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
        memcpy(mCaptureInitParams.cali_dac_seq, cali_dac_seq_8M,
               sizeof(unsigned short) * mCaptureInitParams.cali_seq_len);
    }
    return 0;
mem_fail:
    if (mCaptureInitParams.cali_dist_seq != NULL)
        free(mCaptureInitParams.cali_dist_seq);
    return -1;
}

/*===========================================================================
 * FUNCTION   :initBlurInitParams
 *
 * DESCRIPTION: init Blur Init Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3SinglePortrait::CaptureThread::initBlurInitParams() {
    mFirstCapture = true;
    mFirstPreview = true;
    mUpdateCaptureWeightParams = true;
    mFirstUpdateFrame = 1;
    mstartUpdate = 0;
    mBuffSize = 0;
    mWeightWidth = 0;
    mWeightHeight = 0;
    mWeightSize = 0;

    // preview 720P v1.0 v1.1
    mLastMinScope = 10;      // min_slope*10000
    mLastMaxScope = 50;      // max_slope*10000
    mLastAdjustRati = 20000; // findex2gamma_adjust_ratio*10000
    // mPreviewInitParams.SmoothWinSize = 11;
    // mPreviewInitParams.Scalingratio = 2;
    mPreviewInitParams.box_filter_size = 13;
    mPreviewInitParams.min_slope = (float)(mLastMinScope) / 10000;
    mPreviewInitParams.max_slope = (float)(mLastMaxScope) / 10000;
    mPreviewInitParams.findex2gamma_adjust_ratio =
        (float)(mLastAdjustRati) / 10000;

    // capture 5M v1.0 v1.1 v1.2
    mLastMinScope = 4;        // min_slope*10000
    mLastMaxScope = 19;       // max_slope*10000
    mLastAdjustRati = 150000; // findex2gamma_adjust_ratio*10000
    mCaptureInitParams.Scalingratio = 8;
    mCaptureInitParams.SmoothWinSize = 5;
    mCaptureInitParams.box_filter_size = 0;
    mCaptureInitParams.min_slope = (float)(mLastMinScope) / 10000;
    mCaptureInitParams.max_slope = (float)(mLastMaxScope) / 10000;
    mCaptureInitParams.findex2gamma_adjust_ratio =
        (float)(mLastAdjustRati) / 10000;
    mCaptureInitParams.depthW = Mask_DepthW;
    mCaptureInitParams.depthH = Mask_DepthH;
    mCaptureInitParams.platform_id = PLATFORM_ID;
    memset(fd_score, 0, sizeof(int32_t) * 10);
    return initBlur20Params();
}

/*===========================================================================
 * FUNCTION   :initBlurWeightParams
 *
 * DESCRIPTION: init Blur Weight Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3SinglePortrait::CaptureThread::initBlurWeightParams() {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
        mVersion = 3;
        mPreviewWeightParams.roi_type = 0;
        mCaptureWeightParams.version = 2;
        mCaptureWeightParams.roi_type = 0;
        mCaptureWeightParams.rear_cam_en = true;
        property_get("persist.vendor.cam.ba.blur.version", prop, "0");

        if (atoi(prop) == 1) {
            mVersion = 1;
            mCaptureWeightParams.version = 1;
            mPreviewWeightParams.roi_type = 1;
            mCaptureWeightParams.roi_type = 1;
            mCaptureWeightParams.rear_cam_en = true;
            property_get("persist.vendor.cam.fr.blur.version", prop, "1");
            mCaptureWeightParams.camera_angle =
                SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
                    .sensorInfo.orientation;
        }

    } else {
        mVersion = 1;
        mCaptureWeightParams.version = 1;
        mPreviewWeightParams.roi_type = 1;
        mCaptureWeightParams.roi_type = 1;
        mCaptureWeightParams.rear_cam_en = false;
        property_get("persist.vendor.cam.fr.blur.version", prop, "0");
        mCaptureWeightParams.camera_angle =
            SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
                .sensorInfo.orientation;
    }
    if (atoi(prop) == 1 || atoi(prop) == 2 || atoi(prop) == 3) {
        mVersion = atoi(prop);
    }
    if (atoi(prop) == 1 || atoi(prop) == 2) {
        mCaptureWeightParams.version = atoi(prop);
    }
    mIsGalleryBlur = false;
    mIsBlurAlways = false;
    mGaussEnable = 0;
    mUpdataxy = 0;

    if (mVersion == 1) {
        property_get("persist.vendor.cam.fr.blur.type", prop, "2");
        if (atoi(prop) == 0 || atoi(prop) == 1 || atoi(prop) == 2) {
            mPreviewWeightParams.roi_type = atoi(prop);
            mCaptureWeightParams.roi_type = atoi(prop);
            if (atoi(prop) == 2) {
                mPreviewWeightParams.roi_type = 1;
            }
        }
        /*
        if (mSinglePortrait->mCameraId == 0 &&
            mSinglePortrait->mBlurMode != CAM_SINGLE_PORTRAIT_MODE) {
            mPreviewWeightParams.roi_type = 0;
            mCaptureWeightParams.roi_type = 0;
            mIsBlurAlways = false;
            mGaussEnable = 1;
        }
        */
    }

    HAL_LOGD("Camera:%d,lib1v:%d,lib2v:%d", mSinglePortrait->mCameraId,
             mCaptureWeightParams.version, mVersion);

    HAL_LOGD("roi_type:%d, mIsGalleryBlur:%d, mIsBlurAlways:%d",
             mCaptureWeightParams.roi_type, mIsGalleryBlur, mIsBlurAlways);

    mCircleSizeScale = 50;
    mLastFaceNum = 0;
    mRotation = 0;
    mLastTouchX = 0;
    mLastTouchY = 0;
    mBlurBody = true;
    mUpdataTouch = false;
    // preview weight params
    mPreviewWeightParams.f_number = 1;
    mPreviewWeightParams.valid_roi = 0;
    memset(mPreviewWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mPreviewWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
    mUpdatePreviewWeightParams = true;

    // capture weight params
    mCaptureWeightParams.f_number = 1;
    mCaptureWeightParams.valid_roi = 0;
    mCaptureWeightParams.total_roi = 0;
    mCaptureWeightParams.rotate_angle = 0;
    mCaptureWeightParams.win_peak_pos = mWinPeakPos;
    memset(mCaptureWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
    memset(mCaptureWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);

    // capture weight params
    mCapture2WeightParams.f_number = 1;
}

/*===========================================================================
 * FUNCTION   :updateBlurWeightParams
 *
 * DESCRIPTION: update Blur Weight Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3SinglePortrait::CaptureThread::updateBlurWeightParams(
    CameraMetadata metaSettings, int type) {
    uint8_t face_num = 0;
    uint8_t i = 0;
    uint8_t k = 0;
    int32_t x1 = 0;
    int32_t x2 = 0;
    int32_t max = 0;
    int32_t origW = SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[0];
    int32_t origH = SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[1];
    unsigned short savePreviewX = mPreviewWeightParams.sel_x;
    unsigned short savePreviewY = mPreviewWeightParams.sel_y;
    uint32_t orientation =
        SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
            .jpgInfo.orientation;
    /* always get face angle*/
    faceDetectionInfo =
       SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId].faceInfo;
    /* always get BV,ISO,CT*/
    SprdCamera3HWI *hwiMain = mSinglePortrait->m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    int rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_BV, &cameraBV, NULL);
    rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_ISO, &cameraISO, NULL);
    rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_CT, &cameraCT, NULL);
    rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_LPT_TYPE, &lightPortraitType, NULL);
    for(int i = 0;i < faceDetectionInfo.face_num;i++){
        *(fd_score+i) = SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId].fd_score[i];
    }

    mCaptureWeightParams.rotate_angle = orientation;
    if (origW == 0 || origH == 0) {
        return;
    }
    // always get f_num and orientattion in request
    if (type == 0) {
        if (metaSettings.exists(ANDROID_SPRD_LIGHTPORTRAITTYPE)) {
            lightPortraitType = 
                metaSettings.find(ANDROID_SPRD_LIGHTPORTRAITTYPE).data.i32[0];
                HAL_LOGD("lightPortraitType %d",lightPortraitType);
        }
        if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
            int fnum =
                metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
            if (fnum < MIN_F_FUMBER) {
                fnum = MIN_F_FUMBER;
            } else if (fnum > MAX_F_FUMBER) {
                fnum = MAX_F_FUMBER;
            }

            if (mSinglePortrait->mBlurMode == CAM_SINGLE_PORTRAIT_MODE) {
                fnum = (fnum)*MAX_BLUR_F_FUMBER / MAX_F_FUMBER;
                if (mPreviewWeightParams.f_number != fnum) {
                    mPreviewWeightParams.f_number = fnum;
                    mCaptureWeightParams.f_number =
                        (MAX_F_FUMBER + 1 - fnum / 2) * 255 / MAX_F_FUMBER;
                    mCapture2WeightParams.f_number =
                        (MAX_F_FUMBER + 1 - fnum / 2) * 255 / MAX_F_FUMBER;
                    mUpdatePreviewWeightParams = true;
                }
            }
        }
        if (metaSettings.exists(ANDROID_SPRD_DEVICE_ORIENTATION)) {
            mRotation =
                metaSettings.find(ANDROID_SPRD_DEVICE_ORIENTATION).data.i32[0];
            if (mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
                if (mRotation == 0) {
                    mRotation = 180;
                } else if (mRotation == 180) {
                    mRotation = 0;
                }
            }
            mCaptureWeightParams.mobile_angle = mRotation;
            HAL_LOGD("rotation %d", mRotation);
        }
        if (metaSettings.exists(ANDROID_CONTROL_AF_REGIONS)) {
            uint32_t left =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
            uint32_t top =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
            uint32_t right =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[2];
            uint32_t bottom =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[3];
            int32_t x = left, y = top;
            HAL_LOGV("ANDROID_CONTROL_AF_REGIONS (%d,%d,%d,%d)", left, top,
                     right, bottom);
            if (!(left == 0 && top == 0 && right == 0 && bottom == 0)) {
                x = left + (right - left) / 2;
                y = top + (bottom - top) / 2;
                if (mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
                    x = x * mPreviewInitParams.width / origW;
                    y = y * mPreviewInitParams.height / origH;
                    if (x != mPreviewWeightParams.sel_x ||
                        y != mPreviewWeightParams.sel_y) {
                        mPreviewWeightParams.sel_x = x;
                        mPreviewWeightParams.sel_y = y;
                        mUpdatePreviewWeightParams = true;
                    }
                } else {
                    if (mLastFaceNum > 0 &&
                        (x != mLastTouchX || y != mLastTouchY)) {
                        mLastTouchX = x;
                        mLastTouchY = y;
                        mUpdataTouch = true;
                        mUpdatePreviewWeightParams = true;
                        HAL_LOGD("mLastTouch (%d,%d)", mLastTouchX,
                                 mLastTouchY);
                    }
                }
            }
        } else {
            if (mPreviewWeightParams.sel_x != mPreviewInitParams.width / 2 &&
                mPreviewWeightParams.sel_y != mPreviewInitParams.height / 2) {
                if (mPreviewWeightParams.roi_type == 0) {
                    CONTROL_Tag controlInfo;
                    mSinglePortrait->m_pPhyCamera[CAM_TYPE_MAIN]
                        .hwi->mSetting->getCONTROLTag(&controlInfo);
                    if (controlInfo.af_state ==
                            ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN &&
                        controlInfo.af_mode ==
                            ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                        mPreviewWeightParams.sel_x =
                            mPreviewInitParams.width / 2;
                        mPreviewWeightParams.sel_y =
                            mPreviewInitParams.height / 2;
                        mUpdatePreviewWeightParams = true;
                        HAL_LOGD("autofocus and blur center");
                    }
                }
            }
        }
    }
    // back camera get other WeightParams in request
    if (type == 0 && mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
        if (metaSettings.exists(ANDROID_SPRD_BLUR_CIRCLE_SIZE)) {
            int circle =
                metaSettings.find(ANDROID_SPRD_BLUR_CIRCLE_SIZE).data.i32[0];
            if (circle != 0) {
                int max = mPreviewInitParams.height * mCircleSizeScale / 100;
                if (mPreviewInitParams.width < mPreviewInitParams.height) {
                    max = mPreviewInitParams.width * mCircleSizeScale / 100;
                }
                circle = max - (max - BLUR_CIRCLE_VALUE_MIN) * circle / 255;
                if (mPreviewWeightParams.circle_size != circle) {
                    mPreviewWeightParams.circle_size = circle;
                    mUpdatePreviewWeightParams = true;
                }
            }
        }
    }

    // get WeightParams in result
    if (type == 1 && mVersion == 1) {
        face_num = SprdCamera3Setting::s_setting[mSinglePortrait->mCameraId]
                       .faceInfo.face_num;
        HAL_LOGD("mLastFaceNum:%d,face_num:%d", mLastFaceNum, face_num);
        if (face_num <= 0 && mLastFaceNum <= 0) {
            return;
        }
        mLastFaceNum = face_num;

        if (face_num <= 0) {
            if (mIsBlurAlways) {
                HAL_LOGD("blur3.0-->blur1.0");
                return;
            }
            if (mGaussEnable == 1) {
                mPreviewWeightParams.roi_type = 0;
                mCaptureWeightParams.roi_type = 0;
            }
            mPreviewWeightParams.sel_x = mPreviewInitParams.width / 2;
            mPreviewWeightParams.sel_y = mPreviewInitParams.height / 2;
            mPreviewWeightParams.circle_size =
                mPreviewInitParams.height * mCircleSizeScale / 100 / 2;
            mUpdatePreviewWeightParams = true;
            mPreviewWeightParams.valid_roi = 0;
            mUpdataxy = 0;
            memset(mPreviewWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mPreviewWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
            mCaptureWeightParams.valid_roi = 0;
            mCaptureWeightParams.total_roi = 0;
            memset(mCaptureWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
            HAL_LOGD("no face");
            memset(mFaceInfo, 0, sizeof(int32_t) * 4);

        } else if (metaSettings.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
            if (mIsBlurAlways &&
                mSinglePortrait->mReqState == WAIT_FIRST_YUV_STATE) {
                mPreviewWeightParams.roi_type = 1;
                mCaptureWeightParams.roi_type = 2;
                HAL_LOGD("blur3.0-->blur1.2");
            }
            if (mGaussEnable == 1) {
                mPreviewWeightParams.roi_type = 1;
                mCaptureWeightParams.roi_type = 2;
            }
            HAL_LOGD("roi_type:%d,face_num:%d mUpdataTouch:%d",
                     mPreviewWeightParams.roi_type, face_num, mUpdataTouch);

            for (i = 0; i < face_num; i++) {
                x1 = metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[i * 4 + 0];
                x2 = metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[i * 4 + 2];
                if (x2 - x1 > max) {
                    k = i;
                    max = x2 - x1;
                }
            }
            mFaceInfo[0] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 0]) *
                mPreviewInitParams.width / origW;
            mFaceInfo[1] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 1]) *
                mPreviewInitParams.height / origH;
            mFaceInfo[2] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 2]) *
                mPreviewInitParams.width / origW;
            mFaceInfo[3] =
                (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                     .data.i32[k * 4 + 3]) *
                mPreviewInitParams.height / origH;

            if (k >= BLUR_MAX_ROI / 2 && mPreviewWeightParams.roi_type != 0) {
                metaSettings.update(ANDROID_STATISTICS_FACE_RECTANGLES,
                                    mFaceInfo, 4);
            }
            if (mPreviewWeightParams.roi_type == 0) {
                unsigned short sel_x;
                unsigned short sel_y;
                int circle;

                circle = (mFaceInfo[2] - mFaceInfo[0]) * 4 / 5;
                if (mPreviewWeightParams.circle_size != circle) {
                    mPreviewWeightParams.circle_size = circle;
                    mUpdatePreviewWeightParams = true;
                }
                sel_x = (mFaceInfo[0] + mFaceInfo[2]) / 2 + circle / 8;
                if (mPreviewWeightParams.sel_x != sel_x) {
                    mPreviewWeightParams.sel_x = sel_x;
                    mUpdatePreviewWeightParams = true;
                }

                sel_y = (mFaceInfo[1] + mFaceInfo[3]) / 2;
                if (mPreviewWeightParams.sel_y != sel_y) {
                    mPreviewWeightParams.sel_y = sel_y;
                    mUpdatePreviewWeightParams = true;
                }
            } else {
                int32_t bodyInfo[4];
                int32_t faceInfo[4];
                bool touchInBody = false;
                // Don't blur which face acreage less than max acreage width
                // x%
                int32_t max_width = 50;
                // The face width increase by x%
                int32_t width_increase = 30;
                // The face height increase by x% on top
                int32_t height_increase = 70;
                // The width of the body is the width of the face increased
                // by
                // x%
                int32_t body_increase = 130;
                // The upper side of the body is at x% of the face position
                int32_t upper_position = 0;
                // The face height increase by x% on bottom
                int32_t bottom_increase = 20;

                memset(bodyInfo, 0x00, sizeof(int32_t) * 4);
                memset(faceInfo, 0x00, sizeof(int32_t) * 4);
                memset(mPreviewWeightParams.flag, 0x00,
                       sizeof(int) * BLUR_MAX_ROI);

                if (face_num > BLUR_MAX_ROI / 2) {
                    face_num = BLUR_MAX_ROI / 2;
                }
                k = 0;

                if (mCaptureWeightParams.roi_type == 2) {
                    memset(mCaptureWeightParams.x1, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.y1, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.x2, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.y2, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                    memset(mCaptureWeightParams.flag, 0x00,
                           sizeof(int) * BLUR_MAX_ROI);
                }
                for (i = 0; i < face_num; i++) {
                    faceInfo[0] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 0];
                    faceInfo[1] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 1];
                    faceInfo[2] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 2];
                    faceInfo[3] =
                        metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                            .data.i32[i * 4 + 3];

                    if (mCaptureWeightParams.roi_type == 2) {
                        /*for LPT And Beauty */
                        mPreviewLptAndBeautyFaceinfo.x1[i] =
                                faceInfo[0] * mPreviewInitParams.width / origW;
                        mPreviewLptAndBeautyFaceinfo.y1[i] =
                            faceInfo[1] * mPreviewInitParams.height / origH;
                        mPreviewLptAndBeautyFaceinfo.x2[i] =
                            faceInfo[2] * mPreviewInitParams.width / origW;
                        mPreviewLptAndBeautyFaceinfo.y2[i] =
                                faceInfo[3] * mPreviewInitParams.height / origH;

                        if (mRotation == 0) {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[2] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[1] * mCaptureInitParams.depthH / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[0] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[3] * mCaptureInitParams.depthH / origH;
                        } else if (mRotation == 90) {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[2] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[3] * mCaptureInitParams.depthH / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[0] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[1] * mCaptureInitParams.depthH / origH;
                        } else if (mRotation == 180) {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[0] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[3] * mCaptureInitParams.depthH / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[2] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[1] * mCaptureInitParams.depthH / origH;
                        } else {
                            mCaptureWeightParams.x1[i] =
                                faceInfo[0] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y1[i] =
                                faceInfo[1] * mCaptureInitParams.depthH / origH;
                            mCaptureWeightParams.x2[i] =
                                faceInfo[2] * mCaptureInitParams.depthW / origW;
                            mCaptureWeightParams.y2[i] =
                                faceInfo[3] * mCaptureInitParams.depthH / origH;
                        }
                        mCaptureWeightParams.total_roi = face_num;
                    }
                    /* if ((faceInfo[2] - faceInfo[0]) < max * max_width / 100) {
                        if (mPreviewWeightParams.valid_roi == face_num * 2) {
                            mUpdatePreviewWeightParams = true;
                        }
                        k++;
                        continue;
                    }*/ 
                    if (mRotation == 270) {
                        int w = faceInfo[2] - faceInfo[0];
                        int h = faceInfo[3] - faceInfo[1];

                        faceInfo[0] -= w * width_increase / 200;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += w * width_increase / 200;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[1] -= h * height_increase / 100;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += h * bottom_increase / 100;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        bodyInfo[0] =
                            faceInfo[0] -
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[0] < 0) {
                            bodyInfo[0] = 0;
                        }

                        bodyInfo[1] =
                            faceInfo[3] -
                            (faceInfo[3] - faceInfo[1]) * upper_position / 100;

                        bodyInfo[2] =
                            faceInfo[2] +
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[2] > origW) {
                            bodyInfo[2] = origW;
                        }

                        bodyInfo[3] = origH;
                    } else if (mRotation == 90) {
                        int w = faceInfo[2] - faceInfo[0];
                        int h = faceInfo[3] - faceInfo[1];

                        faceInfo[0] -= w * width_increase / 200;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += w * width_increase / 200;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[3] += h * height_increase / 100;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[1] -= h * bottom_increase / 100;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        bodyInfo[0] =
                            faceInfo[0] -
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[0] < 0) {
                            bodyInfo[0] = 0;
                        }

                        bodyInfo[1] = 0;

                        bodyInfo[2] =
                            faceInfo[2] +
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[2] > origW) {
                            bodyInfo[2] = origW;
                        }

                        bodyInfo[3] =
                            faceInfo[1] +
                            (faceInfo[3] - faceInfo[1]) * upper_position / 100;
                    } else if (mRotation == 180) {
                        int w = faceInfo[3] - faceInfo[1];
                        int h = faceInfo[2] - faceInfo[0];

                        faceInfo[1] -= w * width_increase / 200;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += w * width_increase / 200;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[0] -= h * height_increase / 100;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += h * bottom_increase / 100;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        bodyInfo[0] =
                            faceInfo[2] -
                            (faceInfo[2] - faceInfo[0]) * upper_position / 100;

                        bodyInfo[1] =
                            faceInfo[1] -
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[1] < 0) {
                            bodyInfo[1] = 0;
                        }

                        bodyInfo[2] = origW;

                        bodyInfo[3] =
                            faceInfo[3] +
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[3] > origH) {
                            bodyInfo[3] = origH;
                        }
                    } else {
                        int w = faceInfo[3] - faceInfo[1];
                        int h = faceInfo[2] - faceInfo[0];

                        faceInfo[1] -= w * width_increase / 200;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += w * width_increase / 200;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[2] += h * height_increase / 100;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[0] -= h * bottom_increase / 100;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        bodyInfo[0] = 0;

                        bodyInfo[1] =
                            faceInfo[1] -
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[1] < 0) {
                            bodyInfo[1] = 0;
                        }

                        bodyInfo[2] =
                            faceInfo[0] +
                            (faceInfo[2] - faceInfo[0]) * upper_position / 100;

                        bodyInfo[3] =
                            faceInfo[3] +
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[3] > origH) {
                            bodyInfo[3] = origH;
                        }
                    }
                    if (mUpdataTouch == true && touchInBody == false) {
                        if ((mLastTouchX > faceInfo[0] &&
                             mLastTouchX < faceInfo[2] &&
                             mLastTouchY > faceInfo[1] &&
                             mLastTouchY < faceInfo[3]) ||
                            (mLastTouchX > bodyInfo[0] &&
                             mLastTouchX < bodyInfo[2] &&
                             mLastTouchY > bodyInfo[1] &&
                             mLastTouchY < bodyInfo[3])) {
                            mBlurBody = true;
                            touchInBody = true;
                            HAL_LOGD("in body");
                        } else {
                            mBlurBody = false;
                            HAL_LOGD("out body");
                        }
                    }

                    if (mPreviewWeightParams.x1[2 * (i - k)] !=
                        faceInfo[0] * mPreviewInitParams.width / origW) {
                        mPreviewWeightParams.x1[2 * (i - k)] =
                            faceInfo[0] * mPreviewInitParams.width / origW;
                        mUpdatePreviewWeightParams = true;
                    }
                    if (mPreviewWeightParams.y1[2 * (i - k)] !=
                        faceInfo[1] * mPreviewInitParams.height / origH) {
                        mPreviewWeightParams.y1[2 * (i - k)] =
                            faceInfo[1] * mPreviewInitParams.height / origH;
                        mUpdatePreviewWeightParams = true;
                    }
                    if (mPreviewWeightParams.x2[2 * (i - k)] !=
                        faceInfo[2] * mPreviewInitParams.width / origW) {
                        mPreviewWeightParams.x2[2 * (i - k)] =
                            faceInfo[2] * mPreviewInitParams.width / origW;
                        mUpdatePreviewWeightParams = true;
                    }
                    if (mPreviewWeightParams.y2[2 * (i - k)] !=
                        faceInfo[3] * mPreviewInitParams.height / origH) {
                        mPreviewWeightParams.y2[2 * (i - k)] =
                            faceInfo[3] * mPreviewInitParams.height / origH;
                        mUpdatePreviewWeightParams = true;
                    }
                    mPreviewWeightParams.x1[2 * (i - k) + 1] =
                        bodyInfo[0] * mPreviewInitParams.width / origW;
                    mPreviewWeightParams.y1[2 * (i - k) + 1] =
                        bodyInfo[1] * mPreviewInitParams.height / origH;
                    mPreviewWeightParams.x2[2 * (i - k) + 1] =
                        bodyInfo[2] * mPreviewInitParams.width / origW;
                    mPreviewWeightParams.y2[2 * (i - k) + 1] =
                        bodyInfo[3] * mPreviewInitParams.height / origH;
                    mPreviewWeightParams.flag[2 * (i - k)] = 0;
                    mPreviewWeightParams.flag[2 * (i - k) + 1] = 1;
                }
                mPreviewWeightParams.valid_roi = (face_num - k) * 2;
                if (mCaptureWeightParams.roi_type == 2) {
                    mCaptureWeightParams.valid_roi = face_num - k;
                }

                if (mUpdataTouch == true) {
                    mPreviewWeightParams.sel_x =
                        mLastTouchX * mPreviewInitParams.width / origW;
                    mPreviewWeightParams.sel_y =
                        mLastTouchY * mPreviewInitParams.height / origH;
                    mUpdataTouch = false;
                    mUpdataxy = 1;
                    mFirstUpdateFrame = 1;
                }
                if (mUpdataxy != 0) {
                    if (mFirstUpdateFrame == 1) {
                        mstartUpdate = systemTime();
                        mFaceInfoX = mFaceInfo[0];
                        mFaceInfoY = mFaceInfo[1];
                        mFirstUpdateFrame = 0;
                    }
                    if (ns2ms(systemTime() - mstartUpdate) >= MAX_UPDATE_TIME) {
                        int x = FACE_INFO_SCALE(mPreviewInitParams.width,
                                                mPreviewInitParams.height,
                                                mFaceInfo[0], mFaceInfo[1],
                                                mFaceInfo[2], mFaceInfo[3]);
                        if (ABS(mFaceInfoX - mFaceInfo[0]) * x >
                                mPreviewInitParams.width / 2 ||
                            ABS(mFaceInfoY - mFaceInfo[1]) * x >
                                mPreviewInitParams.height / 2)
                            mUpdataxy = 0;
                    }
                }
                if (!mUpdataxy) {
                    mPreviewWeightParams.sel_x =
                        ABS(mFaceInfo[2] + mFaceInfo[0]) / 2;
                    mPreviewWeightParams.sel_y =
                        ABS(mFaceInfo[3] + mFaceInfo[1]) / 2;
                }
            }
        } else {
            if (mUpdataTouch) {
                int i = 0;
                int32_t x = mLastTouchX * mPreviewInitParams.width / origW;
                int32_t y = mLastTouchY * mPreviewInitParams.height / origH;
                for (i = 0; i < mPreviewWeightParams.valid_roi; i++) {
                    if (x > mPreviewWeightParams.x1[i] &&
                        x < mPreviewWeightParams.x2[i] &&
                        y > mPreviewWeightParams.y1[i] &&
                        y < mPreviewWeightParams.y2[i]) {
                        mBlurBody = true;
                        break;
                    } else {
                        mBlurBody = false;
                    }
                }
                mPreviewWeightParams.sel_x =
                    mLastTouchX * mPreviewInitParams.width / origW;
                mPreviewWeightParams.sel_y =
                    mLastTouchY * mPreviewInitParams.height / origH;
                mUpdataTouch = false;
            }
        }
    }

    if (mUpdatePreviewWeightParams) {
        mUpdateCaptureWeightParams = true;
        mCaptureWeightParams.circle_size = mPreviewWeightParams.circle_size *
                                           mCaptureInitParams.height /
                                           mPreviewInitParams.height;
        mCaptureWeightParams.sel_x = mPreviewWeightParams.sel_x *
                                     mCaptureInitParams.width /
                                     mPreviewInitParams.width;
        mCaptureWeightParams.sel_y = mPreviewWeightParams.sel_y *
                                     mCaptureInitParams.height /
                                     mPreviewInitParams.height;
        mCapture2WeightParams.sel_x = mCaptureWeightParams.sel_x;
        mCapture2WeightParams.sel_y = mCaptureWeightParams.sel_y;
        if (mCaptureWeightParams.roi_type == 1 &&
            mPreviewWeightParams.valid_roi > 0 && mSinglePortrait->mSnapshotResultReturn) {
            mCaptureWeightParams.valid_roi = mPreviewWeightParams.valid_roi;
            memset(mCaptureWeightParams.x1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y1, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.x2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.y2, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memset(mCaptureWeightParams.flag, 0x00, sizeof(int) * BLUR_MAX_ROI);
            memcpy(mCaptureWeightParams.flag, mPreviewWeightParams.flag,
                   sizeof(int) * BLUR_MAX_ROI);
            for (i = 0; i < mCaptureWeightParams.valid_roi / 2; i++) {
                mCaptureWeightParams.x1[2 * i] =
                    mPreviewWeightParams.x1[2 * i] * mCaptureInitParams.width /
                    mPreviewInitParams.width;
                mCaptureWeightParams.y1[2 * i] =
                    mPreviewWeightParams.y1[2 * i] * mCaptureInitParams.height /
                    mPreviewInitParams.height;
                mCaptureWeightParams.x2[2 * i] =
                    mPreviewWeightParams.x2[2 * i] * mCaptureInitParams.width /
                    mPreviewInitParams.width;
                mCaptureWeightParams.y2[2 * i] =
                    mPreviewWeightParams.y2[2 * i] * mCaptureInitParams.height /
                    mPreviewInitParams.height;
                mCaptureWeightParams.x1[2 * i + 1] =
                    mPreviewWeightParams.x1[2 * i + 1] *
                    mCaptureInitParams.width / mPreviewInitParams.width;
                mCaptureWeightParams.y1[2 * i + 1] =
                    mPreviewWeightParams.y1[2 * i + 1] *
                    mCaptureInitParams.height / mPreviewInitParams.height;
                mCaptureWeightParams.x2[2 * i + 1] =
                    mPreviewWeightParams.x2[2 * i + 1] *
                    mCaptureInitParams.width / mPreviewInitParams.width;
                mCaptureWeightParams.y2[2 * i + 1] =
                    mPreviewWeightParams.y2[2 * i + 1] *
                    mCaptureInitParams.height / mPreviewInitParams.height;
            }
            mSinglePortrait->mSnapshotResultReturn = false;
        }

        if (mIsBlurAlways && mVersion == 1) {
            mUpdatePreviewWeightParams = false;
            mPreviewWeightParams.roi_type = 0;
            mPreviewWeightParams.sel_x = savePreviewX;
            mPreviewWeightParams.sel_y = savePreviewY;
            mCapture2WeightParams.sel_x = mPreviewWeightParams.sel_x *
                                          mCaptureInitParams.width /
                                          mPreviewInitParams.width;
            mCapture2WeightParams.sel_y = mPreviewWeightParams.sel_y *
                                          mCaptureInitParams.height /
                                          mPreviewInitParams.height;
        }
    }
}

/*===========================================================================
 * FUNCTION   :getOutWeightMap
 *
 * DESCRIPTION: storage output weight map temporary
 *
 * PARAMETERS : near yuv addr
 *
 *
 * RETURN     : none
 *==========================================================================*/
// void SprdCamera3SinglePortrait::CaptureThread::getOutWeightMap(
//    buffer_handle_t *output_buffer) {

//    //only for mSavedResultBuff
//    if(mOutWeightMap == NULL && mSavedResultBuff == output_buffer) {
//        unsigned char *buffer_base = NULL;
//        mSinglePortrait->map(output_buffer, (void **)(&buffer_base));
//        mOutWeightMap = (unsigned short *)(buffer_base +
//        mSinglePortrait->mjpegSize
//        -
//                                       ((mCaptureInitParams.width /
//                                         mCaptureInitParams.Scalingratio)
//                                         *
//                                        (mCaptureInitParams.height /
//                                         mCaptureInitParams.Scalingratio)
//                                         *
//                                        sizeof(unsigned short)));
//    } else {
//        HAL_LOGE("mOutWeightMap %p, not unmap or not mSavedResultBuff",
//        mOutWeightMap);
//    }
//}

/*===========================================================================
 * FUNCTION   :saveCaptureBlurParams
 *
 * DESCRIPTION: save Capture Blur Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3SinglePortrait::CaptureThread::saveCaptureBlurParams(
    buffer_handle_t *result_buff, uint32_t jpeg_size) {
    uint32_t i = 0;
    void *buffer_base = NULL;
    if (mSinglePortrait->map(result_buff, &buffer_base) != NO_ERROR) {
        HAL_LOGE("map result_buff(%p) error", result_buff);
        return;
    }

    unsigned char *buffer = (unsigned char *)buffer_base;
    uint32_t buffer_size = ADP_WIDTH(*result_buff);
    uint32_t yuv_size1 =
        mCaptureInitParams.width * mCaptureInitParams.height * 3 / 2;
    uint32_t yuv_size2 =
        mCaptureInitParams.width * mCaptureInitParams.height * 3 / 2;
    uint32_t output_weight_map = mWeightSize;
    uint32_t para_size = 0;
    uint32_t use_size = 0;
    uint32_t near_jpeg_size = mSinglePortrait->mNearJpegSize;
    uint32_t far_jpeg_size = mSinglePortrait->mFarJpegSize;
    uint32_t weight_map_size = 0;

    HAL_LOGD("usesprdblurcapturelib: %d.%d", mVersion,
             mCaptureWeightParams.roi_type);

   {
    para_size += BLUR_REFOCUS_COMMON_PARAM_NUM * 4 +
                     BLUR_REFOCUS_2_PARAM_NUM * 4 + BLUR_AF_WINDOW_NUM * 4 +
                     BLUR_MAX_ROI * 5 * 4 +
                     mCaptureInitParams.cali_seq_len * 4 * 2;
    }

    if (!mIsGalleryBlur) {
        far_jpeg_size = 0;
        yuv_size2 = 0;
    }
    
    if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2)) {
        use_size = para_size + near_jpeg_size + jpeg_size + output_weight_map;
    } else {
        use_size = para_size + near_jpeg_size + jpeg_size;
    }
    /* memset space after jpeg*/
    memset(buffer + jpeg_size, 0, use_size - jpeg_size);
    
    {
        // blur1.0 and blur2.0 commom
        uint32_t orientation = mCaptureWeightParams.rotate_angle;
        uint32_t MainWidthData = mSinglePortrait->mCaptureWidth;
        uint32_t MainHeightData = mSinglePortrait->mCaptureHeight;
        uint32_t rear_cam_en = mCaptureWeightParams.rear_cam_en;
        uint32_t roi_type = mCaptureWeightParams.roi_type;
        uint32_t FNum = mCaptureWeightParams.f_number;
        uint32_t circle = mCaptureWeightParams.circle_size;
        uint32_t valid_roi = mCaptureWeightParams.valid_roi;
        uint32_t total_roi = mCaptureWeightParams.total_roi;
        uint32_t MinScope = mLastMinScope;
        uint32_t MaxScope = mLastMaxScope;
        uint32_t AdjustRati = mLastAdjustRati;
        uint32_t SelCoordX = mCaptureWeightParams.sel_x;
        uint32_t SelCoordY = mCaptureWeightParams.sel_y;
        uint32_t CameraAngle = mCaptureWeightParams.camera_angle;
        uint32_t MobileAngle = mCaptureWeightParams.mobile_angle;
        uint32_t scaleSmoothWidth = MainWidthData / BLUR_SMOOTH_SIZE_SCALE;
        uint32_t scaleSmoothHeight = MainHeightData / BLUR_SMOOTH_SIZE_SCALE;
        uint32_t box_filter_size = mCaptureInitParams.box_filter_size;
        uint32_t weight_width = mWeightWidth;
        uint32_t weight_height = mWeightHeight;
        uint32_t weight_size = mWeightSize;
        uint32_t platform_id = mCaptureInitParams.platform_id;
        uint32_t version = mCaptureWeightParams.version;
        uint32_t blur_version = 0;
        if (mCaptureWeightParams.roi_type == 2)
            blur_version = 0x180202;
        else
            blur_version = 0x180102;

        char prop1[PROPERTY_VALUE_MAX] = {
            0,
        };

        HAL_LOGD("jpegorientation=%d, blur=%d, FNum=%d", orientation,
                 mCaptureWeightParams.rotate_angle, FNum);
        unsigned char BlurFlag[] = {'B', 'L', 'U', 'R'};
        unsigned char *p1[] = {(unsigned char *)&orientation,
                               (unsigned char *)&MainWidthData,
                               (unsigned char *)&MainHeightData,
                               (unsigned char *)&near_jpeg_size,
                               (unsigned char *)&rear_cam_en,
                               (unsigned char *)&weight_width,
                               (unsigned char *)&weight_height,
                               (unsigned char *)&weight_size,
                               (unsigned char *)&roi_type,
                               (unsigned char *)&FNum,
                               (unsigned char *)&circle,
                               (unsigned char *)&valid_roi,
                               (unsigned char *)&total_roi,
                               (unsigned char *)&MinScope,
                               (unsigned char *)&MaxScope,
                               (unsigned char *)&AdjustRati,
                               (unsigned char *)&SelCoordX,
                               (unsigned char *)&SelCoordY,
                               (unsigned char *)&CameraAngle,
                               (unsigned char *)&MobileAngle,
                               (unsigned char *)&scaleSmoothWidth,
                               (unsigned char *)&scaleSmoothHeight,
                               (unsigned char *)&box_filter_size,
                               (unsigned char *)&platform_id,
                               (unsigned char *)&version,
                               (unsigned char *)&blur_version,
                               (unsigned char *)&BlurFlag};

        // cpoy common param to tail
        buffer += (use_size - BLUR_REFOCUS_COMMON_PARAM_NUM * 4);
        HAL_LOGD("common param base=%p", buffer);
        for (i = 0; i < BLUR_REFOCUS_COMMON_PARAM_NUM; i++) {
            memcpy(buffer + i * 4, p1[i], 4);
        }

        // blur2.0 use only,always save now
        uint32_t scaling_ratio = mCaptureInitParams.Scalingratio;
        uint32_t smooth_winSize = mCaptureInitParams.SmoothWinSize;
        uint32_t vcm_dac_up_bound = mCaptureInitParams.vcm_dac_up_bound;
        uint32_t vcm_dac_low_bound = mCaptureInitParams.vcm_dac_low_bound;
        uint32_t vcm_dac_info = 0; //*mCaptureInitParams.vcm_dac_info;
        uint32_t vcm_dac_gain = mCaptureInitParams.vcm_dac_gain;
        uint32_t valid_depth_clip = mCaptureInitParams.valid_depth_clip;
        uint32_t method = mCaptureInitParams.method;
        uint32_t row_num = mCaptureInitParams.row_num;
        uint32_t column_num = mCaptureInitParams.column_num;
        uint32_t boundary_ratio = mCaptureInitParams.boundary_ratio;
        uint32_t sel_size = mCaptureInitParams.sel_size;
        uint32_t valid_depth = mCaptureInitParams.valid_depth;
        uint32_t slope = mCaptureInitParams.slope;
        uint32_t valid_depth_up_bound = mCaptureInitParams.valid_depth_up_bound;
        uint32_t valid_depth_low_bound =
            mCaptureInitParams.valid_depth_low_bound;
        uint32_t cali_seq_len = mCaptureInitParams.cali_seq_len;
        unsigned char *p2[] = {(unsigned char *)&scaling_ratio,
                               (unsigned char *)&smooth_winSize,
                               (unsigned char *)&vcm_dac_up_bound,
                               (unsigned char *)&vcm_dac_low_bound,
                               (unsigned char *)&vcm_dac_info,
                               (unsigned char *)&vcm_dac_gain,
                               (unsigned char *)&valid_depth_clip,
                               (unsigned char *)&method,
                               (unsigned char *)&row_num,
                               (unsigned char *)&column_num,
                               (unsigned char *)&boundary_ratio,
                               (unsigned char *)&sel_size,
                               (unsigned char *)&valid_depth,
                               (unsigned char *)&slope,
                               (unsigned char *)&valid_depth_up_bound,
                               (unsigned char *)&valid_depth_low_bound,
                               (unsigned char *)&cali_seq_len};

        buffer -= BLUR_REFOCUS_2_PARAM_NUM * 4;
        HAL_LOGD("blur2.0 param base=%p", buffer);
        for (i = 0; i < BLUR_REFOCUS_2_PARAM_NUM; i++) {
            memcpy(buffer + i * 4, p2[i], 4);
        }
        buffer -= BLUR_AF_WINDOW_NUM * 4;
        for (i = 0; i < BLUR_AF_WINDOW_NUM; i++) {
            memcpy(buffer + i * 4, mCaptureWeightParams.win_peak_pos + i, 2);
        }

        buffer -= BLUR_MAX_ROI * 4 * 5;
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy(buffer + i * 4, mCaptureWeightParams.x1 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 4) + i * 4,
                   mCaptureWeightParams.y1 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 8) + i * 4,
                   mCaptureWeightParams.x2 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 12) + i * 4,
                   mCaptureWeightParams.y2 + i, 4);
        }
        for (i = 0; i < BLUR_MAX_ROI; i++) {
            memcpy((buffer + BLUR_MAX_ROI * 16) + i * 4,
                   mCaptureWeightParams.flag + i, 4);
        }

        buffer -= cali_seq_len * 4;
        for (i = 0; i < cali_seq_len; i++) {
            memcpy(buffer + i * 4, mCaptureInitParams.cali_dac_seq + i, 2);
        }
        buffer -= cali_seq_len * 4;
        for (i = 0; i < cali_seq_len; i++) {
            memcpy(buffer + i * 4, mCaptureInitParams.cali_dist_seq + i, 2);
        }
    }

    // cpoy near jpeg1
    unsigned char *orig_jpeg_data1 = NULL;
    if (mSinglePortrait->map(mSinglePortrait->m_pNearJpegBuffer,
                             (void **)(&orig_jpeg_data1)) == NO_ERROR) {
        buffer -= near_jpeg_size;
        memcpy(buffer, orig_jpeg_data1, near_jpeg_size);

        mSinglePortrait->unmap(mSinglePortrait->m_pNearJpegBuffer);
        orig_jpeg_data1 = NULL;
    } else {
        HAL_LOGE("map m_pNearJpegBuffer(%p) failed",
                 mSinglePortrait->m_pNearJpegBuffer);
    }
    
    // copy output weight map for blur1.2
    if ((mVersion == 1) && (mCaptureWeightParams.roi_type == 2)) {
        buffer -= output_weight_map;
        memcpy(buffer, (unsigned char *)mOutWeightBuff, output_weight_map);
    }

    // blob to indicate all image size(use_size)
    buffer = (unsigned char *)buffer_base;
    mSinglePortrait->setJpegSize((char *)buffer, buffer_size, use_size);

    char prop1[PROPERTY_VALUE_MAX] = {
        0,
    };
    dump_single_portrait_t result_blur_buff;
    result_blur_buff.buffer_addr = buffer_base;
    result_blur_buff.use_size = use_size;
    result_blur_buff.jpeg_size = jpeg_size;
    dump_single_portrait_t *dump_buffs[DUMP_SINGLE_PORTRAIT_TYPE_MAX];
    memset(&dump_buffs, 0,
           sizeof(dump_single_portrait_t *) * DUMP_SINGLE_PORTRAIT_TYPE_MAX);
    dump_buffs[DUMP_SINGLE_PORTRAIT_RESULT] = &result_blur_buff;
    dumpBlurIMG(DUMP_SINGLE_PORTRAIT_RESULT, dump_buffs);

    if (buffer_base != NULL) {
        mSinglePortrait->unmap(result_buff);
        buffer_base = NULL;
    }
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
void SprdCamera3SinglePortrait::CaptureThread::requestExit() {
    single_portrait_queue_msg_t blur_msg;
    blur_msg.msg_type = SINGLE_PORTRAIT_MSG_EXIT;
    mCaptureMsgList.push_back(blur_msg);
    mMergequeueSignal.signal();
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
void SprdCamera3SinglePortrait::CaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, BLUR_THREAD_TIMEOUT);
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
void SprdCamera3SinglePortrait::dump(const struct camera3_device *device,
                                     int fd) {
    HAL_LOGV("E");
    CHECK_BLUR();

    mSinglePortrait->_dump(device, fd);

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
int SprdCamera3SinglePortrait::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_BLUR_ERROR();

    rc = mSinglePortrait->_flush(device);

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
int SprdCamera3SinglePortrait::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGI("E");
    CHECK_HWI_ERROR(hwiMain);

    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mCaptureThread->mCaptureStreamsNum = 0;
    mCaptureThread->mSavedResultBuff = NULL;
    mCaptureThread->mSavedCapReqsettings = NULL;
    mjpegSize = 0;
    m_pNearYuvBuffer = NULL;
    weight_map = NULL;
    mNearJpegSize = 0;
    mFarJpegSize = 0;
    m_pNearJpegBuffer = NULL;
    m_pFarJpegBuffer = NULL;
    mFlushing = false;
    mInitThread = false;
    mCaptureThread->mAlgorithmFlag = false;
    mReqState = PREVIEW_REQUEST_STATE;
    SprdCamera3MultiBase::initialize(MODE_BLUR, hwiMain);

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
        int on_off = STREAM_ON;
        rc = hwiAux->camera_ioctrl(CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL,
                                   &on_off, NULL);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error while aux camera streamon !! ");
            return rc;
        }
    }

    memset(mLocalCapBuffer, 0, sizeof(new_mem_t) * BLUR_LOCAL_CAPBUFF_NUM);
    memset(&mCaptureThread->mSavedCapRequest, 0,
           sizeof(camera3_capture_request_t));
    memset(&mCaptureThread->mSavedCapReqstreambuff, 0,
           sizeof(camera3_stream_buffer_t));
    mCaptureThread->mCallbackOps = callback_ops;
    mSinglePortrait->initThread();
    HAL_LOGI("X");

    return rc;
}

int SprdCamera3SinglePortrait::initThread() {
    int rc = NO_ERROR;
    mCaptureThread->mDevMain = &m_pPhyCamera[CAM_TYPE_MAIN];
    mCaptureThread->run(String8::format("Blur").string(), -7);
    rc = mCaptureThread->initBlurInitParams();
    mCaptureThread->initBlurWeightParams();
    mInitThread = true;
    return rc;
}

int SprdCamera3SinglePortrait::resetVariablesToDefault() {
    int rc = NO_ERROR;
    mFlushing = false;
    if (!mInitThread) {
        mSinglePortrait->initThread();
    }
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
int SprdCamera3SinglePortrait::configureStreams(
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

    Mutex::Autolock l(mLock);
    mSinglePortrait->resetVariablesToDefault();

    HAL_LOGD("configurestreams, stream num:%d", stream_list->num_streams);
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        if (requestStreamType == PREVIEW_STREAM) {
            mPreviewStreamsNum = i;
            mCaptureThread->mPreviewInitParams.width =
                stream_list->streams[i]->width;
            mCaptureThread->mPreviewInitParams.height =
                stream_list->streams[i]->height;
            if (mCaptureThread->mFirstPreview) {

                mCaptureThread->mPreviewWeightParams.sel_x =
                    mCaptureThread->mPreviewInitParams.width / 2;
                mCaptureThread->mPreviewWeightParams.sel_y =
                    mCaptureThread->mPreviewInitParams.height / 2;

                mCaptureThread->mPreviewWeightParams.circle_size =
                    mCaptureThread->mPreviewInitParams.height *
                    mCaptureThread->mCircleSizeScale / 100 / 2;
            }
            HAL_LOGD("config preview stream num: %d, size: %dx%d", i,
                     stream_list->streams[i]->width,
                     stream_list->streams[i]->height);
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            w = stream_list->streams[i]->width;
            h = stream_list->streams[i]->height;

// workaround jpeg cant handle 16-noalign issue, when jpeg fix
// this
// issue, we will remove these code
#ifdef CONFIG_CAMERA_MEET_JPG_ALIGNMENT
            if (h == 1944 && w == 2592) {
                h = 1952;
            }
#endif

            if (mCaptureWidth != w && mCaptureHeight != h) {
                freeLocalCapBuffer();

                for (size_t j = 0; j < BLUR_LOCAL_CAPBUFF_NUM; j++) {
                    if (0 > allocateOne(w, h, &(mLocalCapBuffer[j]), YUV420)) {
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                }
                mCaptureThread->mWeightSize =
                    (w / mCaptureThread->mCaptureInitParams.Scalingratio) *
                    (h / mCaptureThread->mCaptureInitParams.Scalingratio) *
                    sizeof(unsigned short);
                mCaptureThread->mOutWeightBuff =
                    (unsigned short *)malloc(mCaptureThread->mWeightSize);
                memset(mCaptureThread->mOutWeightBuff, 0,
                       mCaptureThread->mWeightSize);

                weight_map = (void *)malloc(w * h / 4);
                memset(weight_map, 0, w * h / 4);

                m_pNearYuvBuffer = (void *)malloc(w * h * 3 / 2);
                memset(m_pNearYuvBuffer, 0, w * h * 3 / 2);
            }
            mCaptureWidth = w;
            mCaptureHeight = h;
            mCaptureThread->mCaptureInitParams.width = w;
            mCaptureThread->mCaptureInitParams.height = h;

            if (mCaptureThread->mFirstCapture) {
                mCaptureThread->mCaptureWeightParams.sel_x =
                    mCaptureThread->mCaptureInitParams.width / 2;
                mCaptureThread->mCaptureWeightParams.sel_y =
                    mCaptureThread->mCaptureInitParams.height / 2;
                mCaptureThread->mCaptureWeightParams.circle_size =
                    mCaptureThread->mCaptureInitParams.height *
                    mCaptureThread->mCircleSizeScale / 100 / 2;
                mCaptureThread->mCapture2WeightParams.sel_x =
                    mCaptureThread->mCaptureInitParams.width / 2;
                mCaptureThread->mCapture2WeightParams.sel_y =
                    mCaptureThread->mCaptureInitParams.height / 2;
            }
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
        }
        mCaptureThread->mMainStreams[i] = *stream_list->streams[i];
        pMainStreams[i] = &mCaptureThread->mMainStreams[i];
    }

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = stream_list->num_streams + 1;
    mainconfig.streams = pMainStreams;

    rc = mCaptureThread->initPortraitParams();
    rc = mCaptureThread->initPortraitLightParams();
    rc = mCaptureThread->initFaceBeautyParams();

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
SprdCamera3SinglePortrait::constructDefaultRequestSettings(
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
void SprdCamera3SinglePortrait::saveRequest(
    camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        newStream->reserved[0] = NULL;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            request_saved_single_portrait_t currRequest;
            HAL_LOGV("save request %d", request->frame_number);
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
int SprdCamera3SinglePortrait::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metaSettings;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_stream_buffer_t *out_streams_main = NULL;
    uint32_t tagCnt = 0;
    int snap_stream_num = 2;
    int af_bypass = 0;
    int fb_on = 0;
    char prop[PROPERTY_VALUE_MAX] = {0};

    memset(&req_main, 0x00, sizeof(camera3_capture_request_t));
    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    metaSettings = request->settings;
    saveRequest(req);

    if (metaSettings.exists(ANDROID_SPRD_PORTRAIT_OPTIMIZATION_MODE)) {
        mBlurMode = metaSettings.find(ANDROID_SPRD_PORTRAIT_OPTIMIZATION_MODE)
                        .data.u8[0];
        HAL_LOGD("mBlurMode %d",mBlurMode);
    }

    tagCnt = metaSettings.entryCount();
    if (tagCnt != 0) {
        uint8_t sprdBurstModeEnabled = 0;
        metaSettings.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                            &sprdBurstModeEnabled, 1);
        uint8_t sprdZslEnabled = 1;
        metaSettings.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
    }
    /* save Perfectskinlevel */
    if (metaSettings.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        mSinglePortrait->fbLevels.blemishLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        mSinglePortrait->fbLevels.smoothLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[1];
        mSinglePortrait->fbLevels.skinColor =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[2];
        mSinglePortrait->fbLevels.skinLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[3];
        mSinglePortrait->fbLevels.brightLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[4];
        mSinglePortrait->fbLevels.lipColor =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[5];
        mSinglePortrait->fbLevels.lipLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[6];
        mSinglePortrait->fbLevels.slimLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[7];
        mSinglePortrait->fbLevels.largeLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[8];
    }
    if(mSinglePortrait->fbLevels.blemishLevel != 0 || mSinglePortrait->fbLevels.smoothLevel != 0 || 
        mSinglePortrait->fbLevels.skinColor != 0 || mSinglePortrait->fbLevels.skinLevel != 0 || 
        mSinglePortrait->fbLevels.brightLevel != 0 || mSinglePortrait->fbLevels.lipColor != 0 || 
        mSinglePortrait->fbLevels.lipLevel != 0 || mSinglePortrait->fbLevels.slimLevel != 0 || 
        mSinglePortrait->fbLevels.largeLevel!= 0 ) {
        mSinglePortrait->mFaceBeautyFlag = true;
    } else {
        mSinglePortrait->mFaceBeautyFlag = false;
    }
    property_get("persist.vendor.cam.blur.cov.id", prop, "3");
    if (mCaptureThread->mVersion == 3 || atoi(prop) != 3) {
        if (metaSettings.exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
            int32_t aeTargetFpsRange[2] = {25, 30};
            metaSettings.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                                aeTargetFpsRange, ARRAY_SIZE(aeTargetFpsRange));
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
            mCaptureThread->mSavedResultBuff =
                request->output_buffers[i].buffer;
            mjpegSize = ADP_WIDTH(*request->output_buffers[i].buffer);
            memcpy(&mCaptureThread->mSavedCapRequest, req,
                   sizeof(camera3_capture_request_t));
            memcpy(&mCaptureThread->mSavedCapReqstreambuff,
                   &req->output_buffers[i], sizeof(camera3_stream_buffer_t));
            req->output_buffers[i].stream->reserved[0] = NULL;
            mSavedReqStreams[mCaptureThread->mCaptureStreamsNum - 1] =
                req->output_buffers[i].stream;
            HAL_LOGD("mFlushing:%d,frame_number:%d,mFaceBeautyFlag:%d", 
                mFlushing, request->frame_number,mSinglePortrait->mFaceBeautyFlag);
            if (metaSettings.exists(ANDROID_SPRD_LIGHTPORTRAITTYPE)) {
                mCaptureThread->lpt_cap_type = 
                    metaSettings.find(ANDROID_SPRD_LIGHTPORTRAITTYPE).data.i32[0];
                    HAL_LOGD("lightPortraitType %d", mCaptureThread->lpt_cap_type);
            }
            mCaptureThread->fb_cap_type = mSinglePortrait->mFaceBeautyFlag;
            memcpy(&mSinglePortrait->fbLevels_cap, &mSinglePortrait->fbLevels, sizeof(faceBeautyLevels));
            if (!mFlushing && mCoverValue == 1 && 
                (mBlurMode || mCaptureThread->lightPortraitType != 0 || 
                mSinglePortrait->mFaceBeautyFlag) && 
                ((!(mCaptureThread->mVersion == 3 &&
                    0 != mCaptureThread->mIspInfo.distance_reminder) &&
                  !((!mCaptureThread->mIsBlurAlways) &&
                    mCaptureThread->mCaptureWeightParams.total_roi == 0)) ||
                 mCaptureThread->mGaussEnable)) {
                snap_stream_num = 2;
                out_streams_main[i].buffer = &mLocalCapBuffer[0].native_handle;
                fb_on = 0;
                hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_CAPTURE_FACE_BEAUTIFY,
                                       &fb_on, NULL);
            } else {
                snap_stream_num = 1;
                fb_on = 1;
                out_streams_main[i].buffer = (req->output_buffers[i]).buffer;
                hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_CAPTURE_FACE_BEAUTIFY,
                                       &fb_on, NULL);
            }
            out_streams_main[i].stream =
                &mCaptureThread->mMainStreams[snap_stream_num];

            mReqState = WAIT_FIRST_YUV_STATE;
        } else {
            mSavedReqStreams[mPreviewStreamsNum] =
                req->output_buffers[i].stream;
            out_streams_main[i].stream =
                &mCaptureThread->mMainStreams[mPreviewStreamsNum];
            mCaptureThread->updateBlurWeightParams(metaSettings, 0);
        }
    }
    req_main.output_buffers = out_streams_main;
    req_main.settings = metaSettings.release();
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        if (getStreamType(request->output_buffers[i].stream) ==
            SNAPSHOT_STREAM) {
            if (NULL != mCaptureThread->mSavedCapReqsettings) {
                free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                mCaptureThread->mSavedCapReqsettings = NULL;
            }
            mCaptureThread->mSavedCapReqsettings =
                clone_camera_metadata(req_main.settings);
        }
    }
    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                          &req_main);
    if (rc < 0) {
        HAL_LOGE("failed, mReqState:%d  idx:%d", mReqState,
                 req_main.frame_number);
        goto req_fail;
    }

req_fail:
    if (req_main.settings != NULL) {
        free_camera_metadata(
            const_cast<camera_metadata_t *>(req_main.settings));
        req_main.settings = NULL;
    }
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
void SprdCamera3SinglePortrait::notifyMain(const camera3_notify_msg_t *msg) {
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        (msg->message.shutter.frame_number ==
         mCaptureThread->mSavedCapRequest.frame_number) &&
        mCaptureThread->mSavedCapRequest.frame_number != 0) {
        if (mReqState != WAIT_FIRST_YUV_STATE) {
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
void SprdCamera3SinglePortrait::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metadata;
    metadata = result->result;
    int portrait_flag = mBlurMode;
    int lightportrait_flag = mCaptureThread->lightPortraitType;
    int facebeauty_flag = mSinglePortrait->mFaceBeautyFlag;

    /* Direclty pass preview buffer and meta result for Main camera */
    if (result_buffer == NULL && result->result != NULL) {
        if (result->frame_number ==
                mCaptureThread->mSavedCapRequest.frame_number &&
            0 != result->frame_number) {
            if (mReqState != WAIT_FIRST_YUV_STATE) {
                HAL_LOGD("hold yuv picture call back, framenumber:%d",
                         result->frame_number);
                return;
            } else {
                if (mCaptureThread->mIsBlurAlways &&
                    mCaptureThread->mVersion == 1) {
                    mCaptureThread->updateBlurWeightParams(metadata, 1);
                }
                mCaptureThread->mCallbackOps->process_capture_result(
                    mCaptureThread->mCallbackOps, result);
                return;
            }
        }
        if (mReqState == PREVIEW_REQUEST_STATE) {
            int32_t sprd3BlurCapVersion = SINGLE_PORTRAIT_CAP_NOAI;
            metadata.update(ANDROID_SPRD_BLUR_CAPVERSION, &sprd3BlurCapVersion,
                            4);
        }

        if (mReqState != PREVIEW_REQUEST_STATE) {
            if (mCaptureThread->mIsBlurAlways &&
                mCaptureThread->mVersion == 1 &&
                mReqState == WAIT_FIRST_YUV_STATE) {
                mCaptureThread->updateBlurWeightParams(metadata, 1);
            }
            if (mCaptureThread->mCaptureWeightParams.roi_type == 2 &&
                mCaptureThread->mCaptureWeightParams.valid_roi > 0 && 
                (portrait_flag || lightportrait_flag != 0 || facebeauty_flag)) {
                int32_t sprd3BlurCapVersion = SINGLE_PORTRAIT_CAP_AI;
                metadata.update(ANDROID_SPRD_BLUR_CAPVERSION,
                                &sprd3BlurCapVersion, 4);
            } else {
                int32_t sprd3BlurCapVersion = SINGLE_PORTRAIT_CAP_NOAI;
                metadata.update(ANDROID_SPRD_BLUR_CAPVERSION,
                                &sprd3BlurCapVersion, 4);
            }

        } else {
            mCaptureThread->updateBlurWeightParams(metadata, 1);
            if (2 == m_nPhyCameras && cur_frame_number > 2) {
                SprdCamera3HWI *hwiSub = m_pPhyCamera[CAM_TYPE_AUX].hwi;
                SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
                mCoverValue = getCoveredValue(metadata, hwiSub, hwiMain,
                                              m_pPhyCamera[CAM_TYPE_AUX].id);
            } else { 
                char prop[PROPERTY_VALUE_MAX] = {
                    0,
                };
                property_get("persist.vendor.cam.blur.cov.val", prop, "1");
                if (mCaptureThread->mVersion == 3) {
                    mCoverValue = atoi(prop);
                }
                
            }
        }
        metadata.update(ANDROID_SPRD_LIGHTPORTRAITTIPS, &mCaptureThread->lpt_return_val, 1);
        metadata.update(ANDROID_SPRD_BLUR_COVERED, &mCoverValue, 1);
        camera3_capture_result_t new_result = *result;
        new_result.result = metadata.release();
        mCaptureThread->mCallbackOps->process_capture_result(
            mCaptureThread->mCallbackOps, &new_result);
        free_camera_metadata(
            const_cast<camera_metadata_t *>(new_result.result));

        HAL_LOGV("cur_frame_number:%d mCoverValue:%d mReqState:%d",
                 cur_frame_number, mCoverValue, mReqState);
        return;
    }

    if (result_buffer == NULL) {
        HAL_LOGE("result_buffer = result->output_buffers is NULL");
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    if (mReqState != PREVIEW_REQUEST_STATE &&
        currStreamType == DEFAULT_STREAM) {
        mSnapshotResultReturn = true;
        HAL_LOGD("framenumber:%d, receive yuv:%d", cur_frame_number, mReqState);
        single_portrait_queue_msg_t capture_msg;
        capture_msg.msg_type = SINGLE_PORTRAIT_MSG_DATA_PROC;
        capture_msg.combo_buff.frame_number = result->frame_number;
        capture_msg.combo_buff.buffer = result->output_buffers->buffer;
        capture_msg.combo_buff.input_buffer = result->input_buffer;
        {
            hwiMain->setMultiCallBackYuvMode(false);
            Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
            mCaptureThread->mCaptureMsgList.push_back(capture_msg);
            mCaptureThread->mMergequeueSignal.signal();
        }
        HAL_LOGI("mVersion %d,mReqState %d", mCaptureThread->mVersion,
                 mSinglePortrait->mReqState);
    } else if (mReqState != PREVIEW_REQUEST_STATE &&
               currStreamType == SNAPSHOT_STREAM) {

        HAL_LOGI("jpeg callback framenumber:%d, mReqState:%d",
                 result->frame_number, mReqState);

        uint32_t jpeg_size = 0;
        uint8_t *jpeg_addr = NULL;
        if (mSinglePortrait->map(result->output_buffers->buffer,
                                 (void **)(&jpeg_addr)) == NO_ERROR) {
            jpeg_size = getJpegSize(jpeg_addr,
                                    ADP_WIDTH(*result->output_buffers->buffer));

            mSinglePortrait->unmap(result->output_buffers->buffer);
            jpeg_addr = NULL;
        } else {
            HAL_LOGE("map buffer(%p) failed", result->output_buffers->buffer);
        }
        {
        if (mNearJpegSize > 0)
            mCaptureThread->saveCaptureBlurParams(
                result->output_buffers->buffer, jpeg_size);
        }
        mCaptureThread->CallSnapBackResult(CAMERA3_BUFFER_STATUS_OK);
        if (mCaptureThread->mIsBlurAlways && mCaptureThread->mVersion == 1) {
            mCaptureThread->mVersion = 3;
            mCaptureThread->mCaptureWeightParams.roi_type = 0;
            mCaptureThread->mPreviewWeightParams.roi_type = 0;
        }
        mReqState = PREVIEW_REQUEST_STATE;
    } else {
        camera3_capture_result_t newResult;
        camera3_stream_buffer_t newOutput_buffers;
        memset(&newResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
        {
            Mutex::Autolock l(mRequestLock);
            List<request_saved_single_portrait_t>::iterator i =
                mSavedRequestList.begin();
            while (i != mSavedRequestList.end()) {
                if (i->frame_number == result->frame_number) {
                    if (result->output_buffers->status !=
                            CAMERA3_BUFFER_STATUS_ERROR &&
                        mCoverValue == 1) {
                        if (!mFlushing && (portrait_flag || 
                            lightportrait_flag != 0 || facebeauty_flag )) {
                            void *buffer_addr = NULL;
                            if (map(result->output_buffers->buffer,
                                    &buffer_addr) != NO_ERROR) {
                                HAL_LOGE("output buffer(%p) map error.",
                                         result->output_buffers->buffer);
                            } else {
                                if(portrait_flag && (mCaptureThread->mLastFaceNum > 0)){
                                    mCaptureThread->prevBlurHandle(
                                    result->output_buffers->buffer, buffer_addr,
                                    NULL, NULL, NULL);
                                }
                                if(facebeauty_flag){
                                    int rc = mCaptureThread->doFaceBeauty(NULL, buffer_addr,
                                    mCaptureThread->mPreviewInitParams.width, 
                                    mCaptureThread->mPreviewInitParams.height,
                                    0, &mSinglePortrait->fbLevels, lightportrait_flag);
                                }
                                if(lightportrait_flag != 0){
                                    int rc = mCaptureThread->prevLPT(buffer_addr, 
                                        mCaptureThread->mPreviewInitParams.width, 
                                        mCaptureThread->mPreviewInitParams.height, lightportrait_flag);
                                }
                            }

                            if (buffer_addr != NULL) {
                                unmap(result->output_buffers->buffer);
                                buffer_addr = NULL;
                            }
                        }
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
                    dumpFps();
                    HAL_LOGV("find preview frame %d", result->frame_number);
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
void SprdCamera3SinglePortrait::_dump(const struct camera3_device *device,
                                      int fd) {
    HAL_LOGI(" E");

    HAL_LOGI("X");
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
int SprdCamera3SinglePortrait::_flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI("E flush, mCaptureMsgList.size=%zu, mSavedRequestList.size:%zu",
             mCaptureThread->mCaptureMsgList.size(), mSavedRequestList.size());
    mFlushing = true;
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
        mCaptureThread->join();
        mInitThread = false;
    }
    if (mCaptureThread->mBlurApi[1]->mHandle != NULL) {
        rc = sprd_portrait_capture_deinit(mCaptureThread->mBlurApi[1]->mHandle);
        if (rc != 0) {
            HAL_LOGE("capture iSmoothDeinit Err:%d", rc);
        }
        mCaptureThread->mBlurApi[1]->mHandle = NULL;
    }
    rc = mCaptureThread->deinitLightPortrait();
    rc = mCaptureThread->deinitFaceBeauty();

    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :initPortraitParams
 *
 * DESCRIPTION: init Portrait Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3SinglePortrait::CaptureThread::initPortraitParams() {
    int ret = NO_ERROR;
    /* portrait capture init params */
    PortraitCap_Init_Params capInitParams;
    memset(&capInitParams, 0, sizeof(PortraitCap_Init_Params));

    capInitParams.libLevel = 1;
    capInitParams.productInfo = mCaptureInitParams.platform_id;
    capInitParams.calcDepth = 0;
    capInitParams.width = mCaptureInitParams.width;
    capInitParams.height = mCaptureInitParams.height;
    capInitParams.depthW = mCaptureInitParams.depthW;
    capInitParams.depthH = mCaptureInitParams.depthH;
    int mLastMinScope = 4;        // min_slope*10000
    int mLastMaxScope = 19;       // max_slope*10000
    int mLastAdjustRati = 150000; // findex2gamma_adjust_ratio*10000
    capInitParams.min_slope =
        (float)(mLastMinScope) /
        10000; // 0.001~0.01, default is 0.005 ->0.0004
    capInitParams.max_slope = (float)(mLastMaxScope) /
                                10000; // 0.01~0.1, default is 0.05 ->0.0019
    capInitParams.Findex2Gamma_AdjustRatio =
        (float)(mLastAdjustRati) / 10000; // 2~11, default is 6.0 ->15.0f
    capInitParams.Scalingratio = 8;
    capInitParams.SmoothWinSize = 5;   // odd number ->5
    capInitParams.box_filter_size = 0; // odd number ->0
    int64_t initStart = systemTime();
    ret =
        sprd_portrait_capture_init(&(mBlurApi[1]->mHandle), &capInitParams);
    HAL_LOGD("capture sprd_portrait_capture_init cost %lld ms",
                ns2ms(systemTime() - initStart));

    if (ret != 0) {
        HAL_LOGE("capture sprd_portrait_capture_init Err:%d", ret);
        ret= NO_ERROR;
    }
    return ret;
}

/*===========================================================================
 * FUNCTION   :initPortraitLightParams
 *
 * DESCRIPTION: init Portrait Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3SinglePortrait::CaptureThread::initPortraitLightParams() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    /*dfa init */
    const char *dfaVersion = NULL;
    dfaVersion = DFA_GetVersion();
    char *dfav = new char[strlen(dfaVersion) + 1];
    strcpy(dfav, dfaVersion);
    HAL_LOGD("dfa version :%s", dfav);
    create_dfa_handle(&dfa_prev, 1);
    if (!dfa_prev.hSprdDfa) {
        HAL_LOGE(" create_dfa_handle failed");
    } else {
        mFirstSprdDfa = true;
    }
    /*lpt init*/
    const char *lptVerison = NULL;
    lptVerison = LPT_GetVersion();
    char *lptv = new char[strlen(lptVerison) + 1];
    strcpy(lptv, lptVerison);
    HAL_LOGD("lpt version :%s", lptv);
    init_lpt_options(&lpt_prev);

    create_lpt_handle(&lpt_prev, LPT_WORKMODE_MOVIE, 4);
    if (!lpt_prev.hSprdLPT) {
        HAL_LOGE("create_lpt_handle failed!");
    } else {
        mFirstSprdLightPortrait = true;
    }
    lptOptions_prev.lightCursor =
        5; /* Control fill light and shade ratio. Value range [0, 35]*/
    lptOptions_prev.lightWeight =
        12; /* Control fill light intensity. Value range [0, 20]*/
    lptOptions_prev.lightSplitMode = LPT_LIGHTSPLIT_RIGHT;
    lptOptions_prev.debugMode = 0;
    if (mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
        lptOptions_prev.cameraWork = LPT_CAMERA_REAR;
    } else {
        lptOptions_prev.cameraWork = LPT_CAMERA_FRONT;
    }
    delete [] dfav;
    delete [] lptv;
    HAL_LOGD("X");
    return rc;
}
int SprdCamera3SinglePortrait::CaptureThread::deinitLightPortrait() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    if (mFirstSprdLightPortrait) {
        if (lpt_prev.hSprdLPT) {
            deinit_lpt_handle(&lpt_prev);
        }
        if (lpt_prev.hSprdLPT) {
            HAL_LOGE("deinitLightPortrait failed");
        }
        mFirstSprdLightPortrait = false;
    }

    if (mFirstSprdDfa) {
        if (dfa_prev.hSprdDfa) {
            deinit_dfa_handle(&dfa_prev);
        }
        if (dfa_prev.hSprdDfa) {
            HAL_LOGE("deinit_dfa_handle failed");
        }
        mFirstSprdDfa = false;
    }

    HAL_LOGD("X");
    return rc;
}

typedef enum { PREVIEW = 0, CAPTURE = 1 } portrait_mode;

int SprdCamera3SinglePortrait::CaptureThread::prevLPT(void *input_buff,
         int picWidth, int picHeight, int lightPortraitType) {
    int rc = NO_ERROR;
    HAL_LOGV("prevLPT E");
    /*run dfa */
    HAL_LOGV("runDFA E");
    ATRACE_BEGIN("dfa");
    rc = runDFA(input_buff, picWidth, picHeight, PREVIEW);
    ATRACE_END();
    HAL_LOGV("runDFA X");
    if (rc != NO_ERROR) {
        HAL_LOGE("prevLPT runDFA failed");
    }
    lptOptions_prev.cameraBV = cameraBV;
    lptOptions_prev.cameraISO = cameraISO;
    lptOptions_prev.cameraCT = cameraCT;
    lptOptions_prev.lightPortraitType = lightPortraitType;
    construct_lpt_options(&lpt_prev, lptOptions_prev);
    int index = faceDetectionInfo.face_num;
    for(int j = 0; j < index; j++) {
        int sx = 0, sy = 0, ex = 0, ey = 0;
        sx = mPreviewLptAndBeautyFaceinfo.x1[j];
        sy = mPreviewLptAndBeautyFaceinfo.y1[j];
        ex = mPreviewLptAndBeautyFaceinfo.x2[j];
        ey = mPreviewLptAndBeautyFaceinfo.y2[j];
        int yaw = faceDetectionInfo.pose[j];
        int roll = faceDetectionInfo.angle[j];
        int fScore = fd_score[j];
        unsigned char attriRace = 0;
        unsigned char attriGender = 0;
        unsigned char attriAge = 0;
        construct_lpt_face(&lpt_prev, j, sx, sy, ex, ey, yaw, roll, fScore,
                           attriRace, attriGender, attriAge);
        HAL_LOGD("prev_ yaw %d roll %d fScore %d", yaw, roll, fScore);
    }
    unsigned char *addrY = (unsigned char *)input_buff;
    unsigned char *addrUV = (unsigned char *)input_buff + picWidth * picHeight;
    int format = 1; /*NV21*/
    construct_lpt_image(&lpt_prev, picWidth, picHeight, addrY, addrUV, format);
    int facecount = faceDetectionInfo.face_num;

    /*do lpt */
    HAL_LOGV("do_image_lpt E");
    ATRACE_BEGIN("lpt");
    rc = do_image_lpt(&lpt_prev, facecount);
    ATRACE_END();
    HAL_LOGV("do_image_lpt X");
    lpt_return_val = rc;
    if (rc != NO_ERROR) {
        HAL_LOGD("do_image_lpt error %d", rc);
        rc = NO_ERROR;
    }
    HAL_LOGV("prevLPT X");
    return rc;
}

int SprdCamera3SinglePortrait::CaptureThread::runDFA(void *input_buff, 
                            int picWidth, int picHeight, int mode) {
    int rc = NO_ERROR;
    /*construct dfa options */
    unsigned char *addrY = (unsigned char *)input_buff;
    unsigned char *addrUV = (unsigned char *)input_buff + picWidth * picHeight;
    int format = 1; /*NV21*/
    if(mode == CAPTURE){
        create_dfa_handle(&dfa_cap, 0);
        int mobile_angle = mRotation;
        construct_dfa_yuv420sp(&dfa_cap, picWidth, picHeight, addrY, addrUV,
                               format);
        unsigned char rType = 0;
        int index = faceDetectionInfo.face_num;
        for(int j = 0; j < index; j++) {
            int rX = 0, rY = 0, rWidth = 0, rHeight = 0;
            if(mobile_angle == 0) {
                rX = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rY = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
                rWidth = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW - 
                        mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rHeight = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH - 
                        mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            } else if (mobile_angle == 90) {
                rX = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rY = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
                rWidth = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW - 
                        mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rHeight = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH - 
                        mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            } else if (mobile_angle == 180) {
                rX = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rY = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
                rWidth = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW - 
                        mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rHeight = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH - 
                        mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            } else {
                rX = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rY = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
                rWidth = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW - 
                        mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                rHeight = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH - 
                        mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            }
            int rRollAngle = faceDetectionInfo.angle[j];
            construct_dfa_face(&dfa_cap, j, rX, rY, rWidth, rHeight, rRollAngle,
                               rType);
        }
        /*do dfa */
        DFA_RESULT dfa_result;
        memset(&dfa_result, 0, sizeof(DFA_RESULT));
        do_dfa_image_yuv420sp(&dfa_cap, faceDetectionInfo.face_num, &dfa_result);
        construct_lpt_dfaInfo(
            &lpt_cap, dfa_result.pitch, dfa_result.yaw, dfa_result.roll,
            dfa_result.t3d, 3, dfa_result.scale, dfa_result.R, 3,
            dfa_result.alpha_shp, 40, dfa_result.alpha_exp, 20);
        if (dfa_cap.hSprdDfa) {
            deinit_dfa_handle(&dfa_cap);
        }
    } else {
        construct_dfa_yuv420sp(&dfa_prev, picWidth, picHeight, addrY, addrUV, format);
        unsigned char rType = 0;
        int index = faceDetectionInfo.face_num;
        for(int j = 0; j < index; j++) {
            int rX = 0, rY = 0, rWidth = 0, rHeight = 0;
            rX = mPreviewLptAndBeautyFaceinfo.x1[j];
            rY = mPreviewLptAndBeautyFaceinfo.y1[j];
            rWidth = mPreviewLptAndBeautyFaceinfo.x2[j] - mPreviewLptAndBeautyFaceinfo.x1[j];
            rHeight = mPreviewLptAndBeautyFaceinfo.y2[j] - mPreviewLptAndBeautyFaceinfo.y1[j];
            int rRollAngle = faceDetectionInfo.angle[j];
            construct_dfa_face(&dfa_prev, j, rX, rY, rWidth, rHeight, rRollAngle,
                               rType);
        }
         /*do dfa */
        DFA_RESULT dfa_result;
        memset(&dfa_result, 0, sizeof(DFA_RESULT));
        do_dfa_image_yuv420sp(&dfa_prev, faceDetectionInfo.face_num, &dfa_result);
        construct_lpt_dfaInfo(
            &lpt_prev, dfa_result.pitch, dfa_result.yaw, dfa_result.roll,
            dfa_result.t3d, 3, dfa_result.scale, dfa_result.R, 3,
            dfa_result.alpha_shp, 40, dfa_result.alpha_exp, 20);
    }
    return rc;
}


int SprdCamera3SinglePortrait::CaptureThread:: getPortraitMask(void *output_buff, 
            void *input_buf1_addr, int vcmCurValue, unsigned char *result) {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    /*get portrait_mask*/
    int f_number = 0;
    PortaitCapProcParams wParams;
    InoutYUV yuvData;

    if (!output_buff || !input_buf1_addr) {
        HAL_LOGE(" buff is null");
        rc = BAD_VALUE;
        return rc;
    }

    memset(&wParams, 0, sizeof(PortaitCapProcParams));
    memset(&yuvData, 0, sizeof(InoutYUV));

    wParams.DisparityImage = NULL;
    wParams.version = mCaptureWeightParams.version;
    wParams.roi_type = mCaptureWeightParams.roi_type;
    wParams.F_number = mCaptureWeightParams.f_number;
    wParams.sel_x = mCaptureWeightParams.sel_x;
    wParams.sel_y = mCaptureWeightParams.sel_y;
    wParams.CircleSize = 50;
    wParams.valid_roi = mCaptureWeightParams.valid_roi;
    wParams.total_roi = mCaptureWeightParams.total_roi;
    memcpy(&wParams.x1, &mCaptureWeightParams.x1,
            wParams.total_roi * sizeof(int));
    memcpy(&wParams.x2, &mCaptureWeightParams.x2,
            wParams.total_roi * sizeof(int));
    memcpy(&wParams.y1, &mCaptureWeightParams.y1,
            wParams.total_roi * sizeof(int));
    memcpy(&wParams.y2, &mCaptureWeightParams.y2,
            wParams.total_roi * sizeof(int));
    wParams.rear_cam_en = mCaptureWeightParams.rear_cam_en;   // true
    wParams.rotate_angle = mCaptureWeightParams.rotate_angle; //--
    wParams.camera_angle = mCaptureWeightParams.camera_angle;
    wParams.mobile_angle = mCaptureWeightParams.mobile_angle;

    yuvData.Src_YUV = (unsigned char *)input_buf1_addr;
    yuvData.Dst_YUV = (unsigned char *)output_buff;

    unsigned int maskW = 0, maskH = 0, maskSize = 0;
    rc = sprd_portrait_capture_get_mask_info(mBlurApi[1]->mHandle, &maskW, &maskH,
                                             &maskSize);

    rc = sprd_portrait_capture_get_mask(mBlurApi[1]->mHandle, NULL, &wParams,
                                           &yuvData, result);

    HAL_LOGD("X");
    return rc;
}

int SprdCamera3SinglePortrait::CaptureThread:: capLPT(
            void *output_buff, int picWidth, int picHeight,
            unsigned char *outPortraitMask,int lightPortraitType) {
    int rc = NO_ERROR;
    HAL_LOGD("E lightPortraitType %d",lightPortraitType);
    /*init handle*/
    int mobile_angle_cap = mRotation;
    lptOptions_cap.lightCursor = 5; /* Control fill light and shade ratio. Value range [0, 35]*/
    lptOptions_cap.lightWeight = 12; /* Control fill light intensity. Value range [0, 20]*/
    lptOptions_cap.lightSplitMode = LPT_LIGHTSPLIT_RIGHT;
    lptOptions_cap.debugMode = 0;
    if (mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
        lptOptions_cap.cameraWork = LPT_CAMERA_REAR;
    } else {
        lptOptions_cap.cameraWork = LPT_CAMERA_FRONT;
    }
    lptOptions_cap.cameraBV = cameraBV;
    lptOptions_cap.cameraISO = cameraISO;
    lptOptions_cap.cameraCT = cameraCT;
    lptOptions_cap.lightPortraitType = lightPortraitType;
    init_lpt_options(&lpt_cap);
    create_lpt_handle(&lpt_cap, LPT_WORKMODE_STILL, 4);
    if (!lpt_cap.hSprdLPT) {
        HAL_LOGE("create_lpt_handle failed!");
    }
    /*portrait mask*/
    construct_lpt_mask(&lpt_cap, 512, 384, outPortraitMask);

    /*run dfa */
    rc = runDFA(output_buff, picWidth, picHeight, CAPTURE);
    if (rc != NO_ERROR) {
        HAL_LOGE("capLPT runDFA failed");
    }
    construct_lpt_options(&lpt_cap, lptOptions_cap);
    unsigned char *addrY = (unsigned char *)output_buff;
    unsigned char *addrUV = (unsigned char *)output_buff + picWidth * picHeight;
    int format = 1; /*NV21*/
    int index = faceDetectionInfo.face_num;
    int facecount = faceDetectionInfo.face_num;

    for(int j = 0;j < index;j++) {
        int sx = 0, sy = 0, ex = 0, ey = 0;
        if(mobile_angle_cap == 0) {
            sx = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            sy = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            ex = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            ey = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
        } else if (mobile_angle_cap == 90) {
            sx = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            sy = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            ex = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            ey = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
        } else if (mobile_angle_cap == 180) {
            sx = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            sy = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            ex = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            ey = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
        } else {
            sx = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            sy = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            ex = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
            ey = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
        }
        if(mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
            if(mobile_angle_cap == 0) {
                sx = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                sy = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
                ex = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                ey = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            } else if(mobile_angle_cap == 180) {
                sx = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                sy = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
                ex = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / mCaptureInitParams.depthW;
                ey = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / mCaptureInitParams.depthH;
            }
        }
        int yaw = faceDetectionInfo.pose[j];
        int roll = faceDetectionInfo.angle[j];
        int fScore = fd_score[j];
        unsigned char attriRace = 0;
        unsigned char attriGender = 0;
        unsigned char attriAge = 0;
        construct_lpt_face(&lpt_cap, j, sx, sy, ex, ey, yaw, roll, fScore,
                           attriRace, attriGender, attriAge);
        HAL_LOGD("cap yaw %d roll %d fScore %d", yaw, roll, fScore);
    }
    construct_lpt_image(&lpt_cap, picWidth, picHeight, addrY, addrUV, format);
    rc = do_image_lpt(&lpt_cap, facecount);
    if (rc != NO_ERROR) {
        HAL_LOGD("do_image_lpt error %d", rc);
        rc = NO_ERROR;
    }

    if (lpt_cap.hSprdLPT) {
        deinit_lpt_handle(&lpt_cap);
    }

    HAL_LOGD("X");
    return rc;
}

#ifdef CONFIG_FACE_BEAUTY
int SprdCamera3SinglePortrait::CaptureThread::initFaceBeautyParams() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    face_beauty_set_devicetype(&mSinglePortrait->fb_prev, SPRD_CAMALG_RUN_TYPE_CPU);
    face_beauty_init(&mSinglePortrait->fb_prev, 1, 2, SHARKL5PRO);
    if (!mSinglePortrait->fb_prev.hSprdFB) {
        HAL_LOGE(" create_fb_handle failed");
    } else {
        mSinglePortrait->mFirstSprdFB = true;
    }
    mSinglePortrait->fbLevels.blemishLevel = 0;
    mSinglePortrait->fbLevels.smoothLevel = 6;
    mSinglePortrait->fbLevels.skinColor = FB_SKINCOLOR_WHITE;
    mSinglePortrait->fbLevels.skinLevel = 0;
    mSinglePortrait->fbLevels.brightLevel = 6;
    mSinglePortrait->fbLevels.lipColor = FB_LIPCOLOR_CRIMSON;
    mSinglePortrait->fbLevels.lipLevel = 0;
    mSinglePortrait->fbLevels.slimLevel = 2;
    mSinglePortrait->fbLevels.largeLevel = 2;
    if(mSinglePortrait->mCameraId == CAM_BLUR_MAIN_ID) {
        mSinglePortrait->fbLevels.cameraWork = FB_CAMERA_REAR;
    } else {
        mSinglePortrait->fbLevels.cameraWork = FB_CAMERA_FRONT;
    }
    HAL_LOGD("X");
    return rc;
}
int SprdCamera3SinglePortrait::CaptureThread::deinitFaceBeauty() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    if (mSinglePortrait->mFirstSprdFB) {
        if (mSinglePortrait->fb_prev.hSprdFB) {
            rc = face_beauty_ctrl(&mSinglePortrait->fb_prev, FB_BEAUTY_FAST_STOP_CMD, NULL);
            face_beauty_deinit(&mSinglePortrait->fb_prev);
        }
        if (mSinglePortrait->fb_prev.hSprdFB) {
            HAL_LOGE("deinitFB failed");
        }
        mSinglePortrait->mFirstSprdFB = false;
    }
    HAL_LOGD("X");
    return rc;
}
int SprdCamera3SinglePortrait::CaptureThread::doFaceBeauty(unsigned char *mask, void *input_buff,
                                   int picWidth, int picHeight, int mode,
                                   faceBeautyLevels *facebeautylevel, int lightPortraitType) {
    int rc = NO_ERROR;
    HAL_LOGV("E");
    if (mode == CAPTURE) {
        HAL_LOGD("cap dofacebeauty E");
        int mobile_angle_cap = mRotation;
        face_beauty_set_devicetype(&mSinglePortrait->fb_cap, SPRD_CAMALG_RUN_TYPE_CPU);
        face_beauty_init(&mSinglePortrait->fb_cap, 0, 2, SHARKL5PRO);

        int index = faceDetectionInfo.face_num;
        for (int j = 0; j < index; j++) {
            mSinglePortrait->beauty_face.idx = j;
            if(mobile_angle_cap == 0) {
                mSinglePortrait->beauty_face.startX = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.startY = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
                mSinglePortrait->beauty_face.endX = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.endY = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
            } else if (mobile_angle_cap == 90) {
                mSinglePortrait->beauty_face.startX = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.startY = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
                mSinglePortrait->beauty_face.endX = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.endY = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
            } else if (mobile_angle_cap == 180) {
                mSinglePortrait->beauty_face.startX = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.startY = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
                mSinglePortrait->beauty_face.endX = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.endY = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
            } else {
                mSinglePortrait->beauty_face.startX = mCaptureWeightParams.x1[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.startY = mCaptureWeightParams.y1[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
                mSinglePortrait->beauty_face.endX = mCaptureWeightParams.x2[j] * mCaptureInitParams.width / 
                                                                    mCaptureInitParams.depthW;
                mSinglePortrait->beauty_face.endY = mCaptureWeightParams.y2[j] * mCaptureInitParams.height / 
                                                                    mCaptureInitParams.depthH;
            }

            mSinglePortrait->beauty_face.angle = faceDetectionInfo.angle[j];
            mSinglePortrait->beauty_face.pose = faceDetectionInfo.pose[j];
            rc = face_beauty_ctrl(&mSinglePortrait->fb_cap, FB_BEAUTY_CONSTRUCT_FACE_CMD,
                                  &mSinglePortrait->beauty_face);
        }
        mSinglePortrait->beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
        mSinglePortrait->beauty_image.inputImage.addr[0] = (unsigned char *)input_buff;
        mSinglePortrait->beauty_image.inputImage.addr[1] = 0x0;
        mSinglePortrait->beauty_image.inputImage.addr[2] = 0x0;
        mSinglePortrait->beauty_image.inputImage.ion_fd = 22;//vdsp care
        mSinglePortrait->beauty_image.inputImage.offset[0] = 0;
        mSinglePortrait->beauty_image.inputImage.offset[1] = picWidth * picHeight;
        mSinglePortrait->beauty_image.inputImage.width = picWidth;
        mSinglePortrait->beauty_image.inputImage.height = picHeight;
        mSinglePortrait->beauty_image.inputImage.stride = picWidth;
        mSinglePortrait->beauty_image.inputImage.size = picWidth * picHeight * 3 / 2;
         // update
        mSinglePortrait->fbLevels.blemishLevel = facebeautylevel->blemishLevel;
        mSinglePortrait->fbLevels.smoothLevel = facebeautylevel->smoothLevel;
        mSinglePortrait->fbLevels.skinColor = facebeautylevel->skinColor;
        mSinglePortrait->fbLevels.skinLevel = facebeautylevel->skinLevel;
        mSinglePortrait->fbLevels.brightLevel = facebeautylevel->brightLevel;
        mSinglePortrait->fbLevels.lipColor = facebeautylevel->lipColor;
        mSinglePortrait->fbLevels.lipLevel = facebeautylevel->lipLevel;
        mSinglePortrait->fbLevels.slimLevel = facebeautylevel->slimLevel;
        mSinglePortrait->fbLevels.largeLevel = facebeautylevel->largeLevel;
        mSinglePortrait->fbLevels.cameraBV = lptOptions_cap.cameraBV;
        mSinglePortrait->fbLevels.cameraISO = lptOptions_cap.cameraISO;
        mSinglePortrait->fbLevels.cameraCT = lptOptions_cap.cameraCT;
        HAL_LOGD("facebeautylevel %d %d %d %d %d",mSinglePortrait->fbLevels.blemishLevel,
            mSinglePortrait->fbLevels.smoothLevel,mSinglePortrait->fbLevels.skinColor,
            mSinglePortrait->fbLevels.skinLevel,mSinglePortrait->fbLevels.brightLevel);
        rc = face_beauty_ctrl(&mSinglePortrait->fb_cap, FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                              &mSinglePortrait->beauty_image);
        rc = face_beauty_ctrl(&mSinglePortrait->fb_cap, FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                              &mSinglePortrait->fbLevels);
        fb_beauty_mask_t fbMask;
        fbMask.fb_mask.width = 512;
        fbMask.fb_mask.height = 384;
        fbMask.fb_mask.data = mask;
        rc = face_beauty_ctrl(&mSinglePortrait->fb_cap, FB_BEAUTY_CONSTRUCT_MASK_CMD, &(fbMask));
        fb_beauty_lptparam_t lpt_param;
        lpt_param.faceCount = faceDetectionInfo.face_num;
        lpt_param.lightPortraitType = lightPortraitType;
        rc = face_beauty_ctrl(&mSinglePortrait->fb_cap, FB_BEAUTY_PROCESS_CMD, &(lpt_param));

        HAL_LOGD("capture face beauty done!");
        rc = face_beauty_ctrl(&mSinglePortrait->fb_cap, FB_BEAUTY_FAST_STOP_CMD, NULL);
        face_beauty_deinit(&mSinglePortrait->fb_cap);
        HAL_LOGD("cap dofacebeauty X");
    } else {
        int index = faceDetectionInfo.face_num;
        int faceCount = faceDetectionInfo.face_num;
        for(int j = 0;j < index;j++) {
            mSinglePortrait->beauty_face.idx = j;
            mSinglePortrait->beauty_face.startX  = mPreviewLptAndBeautyFaceinfo.x1[j];
            mSinglePortrait->beauty_face.startY = mPreviewLptAndBeautyFaceinfo.y1[j];
            mSinglePortrait->beauty_face.endX = mPreviewLptAndBeautyFaceinfo.x2[j];
            mSinglePortrait->beauty_face.endY = mPreviewLptAndBeautyFaceinfo.y2[j];
            mSinglePortrait->beauty_face.angle = faceDetectionInfo.angle[j];
            mSinglePortrait->beauty_face.pose = faceDetectionInfo.pose[j];
            rc = face_beauty_ctrl(&mSinglePortrait->fb_prev, FB_BEAUTY_CONSTRUCT_FACE_CMD,
                                  &mSinglePortrait->beauty_face);
        }
        mSinglePortrait->beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
        mSinglePortrait->beauty_image.inputImage.addr[0] = (unsigned char *)input_buff;
        mSinglePortrait->beauty_image.inputImage.addr[1] = 0X0;
        mSinglePortrait->beauty_image.inputImage.addr[2] = 0x0;
        mSinglePortrait->beauty_image.inputImage.ion_fd = 22; //vdsp care
        mSinglePortrait->beauty_image.inputImage.offset[0] = 0;
        mSinglePortrait->beauty_image.inputImage.offset[1] = picWidth * picHeight;
        mSinglePortrait->beauty_image.inputImage.width = picWidth;
        mSinglePortrait->beauty_image.inputImage.height = picHeight;
        mSinglePortrait->beauty_image.inputImage.stride = picWidth;
        mSinglePortrait->beauty_image.inputImage.size = picWidth * picHeight * 3 / 2;

        // update
        mSinglePortrait->fbLevels.blemishLevel = facebeautylevel->blemishLevel;
        mSinglePortrait->fbLevels.smoothLevel = facebeautylevel->smoothLevel;
        mSinglePortrait->fbLevels.skinColor = facebeautylevel->skinColor;
        mSinglePortrait->fbLevels.skinLevel = facebeautylevel->skinLevel;
        mSinglePortrait->fbLevels.brightLevel = facebeautylevel->brightLevel;
        mSinglePortrait->fbLevels.lipColor = facebeautylevel->lipColor;
        mSinglePortrait->fbLevels.lipLevel = facebeautylevel->lipLevel;
        mSinglePortrait->fbLevels.slimLevel = facebeautylevel->slimLevel;
        mSinglePortrait->fbLevels.largeLevel = facebeautylevel->largeLevel;
        mSinglePortrait->fbLevels.cameraBV = lptOptions_prev.cameraBV;
        mSinglePortrait->fbLevels.cameraISO = lptOptions_prev.cameraISO;
        mSinglePortrait->fbLevels.cameraCT = lptOptions_prev.cameraCT;

        rc = face_beauty_ctrl(&mSinglePortrait->fb_prev, FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                              &mSinglePortrait->beauty_image);
        rc = face_beauty_ctrl(&mSinglePortrait->fb_prev, FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                              &mSinglePortrait->fbLevels);
        fb_beauty_lptparam_t lpt_param;
        lpt_param.faceCount = faceCount;
        lpt_param.lightPortraitType = lightPortraitType;
        rc = face_beauty_ctrl(&mSinglePortrait->fb_prev, FB_BEAUTY_PROCESS_CMD, &(lpt_param));
    }

    HAL_LOGV("X");
    return rc;
}
#endif
};
