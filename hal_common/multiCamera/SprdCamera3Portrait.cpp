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
#define LOG_TAG "CamPortrait"
//#define LOG_NDEBUG 0
#include "SprdCamera3Portrait.h"
#include "../../sensor/otp_drv/otp_info.h"

// XMP define and include
#define TXMP_STRING_TYPE std::string
#include <XMP.hpp>
#include <XMP.incl_cpp>

using namespace android;
namespace sprdcamera {

#define BOKEH_THREAD_TIMEOUT (50e6)
#define CLEAR_PRE_FRAME_UNMATCH_TIMEOUT (2000) // 2s

#ifdef YUV_CONVERT_TO_JPEG
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
#define BOKEH_REFOCUS_COMMON_PARAM_NUM (19)
#else
#define BOKEH_REFOCUS_COMMON_PARAM_NUM (15)
#endif
#else
#define BOKEH_REFOCUS_COMMON_PARAM_NUM (13)
#endif

#define BOKEH_REFOCUS_COMMON_XMP_SIZE 0 //(64 * 1024)

#define DEPTH_OUTPUT_WIDTH (400)       //(160)
#define DEPTH_OUTPUT_HEIGHT (300)      //(120)
#define DEPTH_SNAP_OUTPUT_WIDTH (800)  //(324)
#define DEPTH_SNAP_OUTPUT_HEIGHT (600) //(243)

#define BOKEH_PREVIEW_PARAM_LIST (10)
/* refocus api error code */
#define ALRNB_ERR_SUCCESS 0x00

#define BOKEH_TIME_DIFF (100e6)
#define PENDINGTIME (1000000)
#define PENDINGTIMEOUT (5000000000)
#define MASTER_ID 0
SprdCamera3Portrait *mPortrait = NULL;
const uint8_t kavailable_physical_ids[] = {'0', '\0', '2', '\0'};
// Error Check Macros
#define CHECK_CAPTURE()                                                        \
    if (!mPortrait) {                                                          \
        HAL_LOGE("Error getting capture ");                                    \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_CAPTURE_ERROR()                                                  \
    if (!mPortrait) {                                                          \
        HAL_LOGE("Error getting capture ");                                    \
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

camera3_device_ops_t SprdCamera3Portrait::mCameraCaptureOps = {
    .initialize = SprdCamera3Portrait::initialize,
    .configure_streams = SprdCamera3Portrait::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3Portrait::construct_default_request_settings,
    .process_capture_request = SprdCamera3Portrait::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3Portrait::dump,
    .flush = SprdCamera3Portrait::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3Portrait::callback_ops_main = {
    .process_capture_result = SprdCamera3Portrait::process_capture_result_main,
    .notify = SprdCamera3Portrait::notifyMain};

camera3_callback_ops SprdCamera3Portrait::callback_ops_aux = {
    .process_capture_result = SprdCamera3Portrait::process_capture_result_aux,
    .notify = SprdCamera3Portrait::notifyAux};

/*===========================================================================
 * FUNCTION         : SprdCamera3Portrait
 *
 * DESCRIPTION     : SprdCamera3Portrait Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3Portrait::SprdCamera3Portrait() {
    HAL_LOGI(" E");
    m_nPhyCameras = 2; // m_nPhyCameras should always be 2 with dual camera mode
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = (uint8_t)CAM_PORTRAIT_MAIN_ID;
    mStaticMetadata = NULL;
    m_pPhyCamera = NULL;
    mCaptureStreamsNum = 0;
    mCallbackStreamsNum = 0;
    mPreviewStreamsNum = 0;
    mPendingRequest = 0;
    mSavedCapStreams = NULL;
    mCapFrameNumber = 0;
    mPrevBlurFrameNumber = 0;
    mPrevFrameNumber = 0;
    mIsCapturing = false;
    mSnapshotResultReturn = false;
    mjpegSize = 0;
    capture_result_timestamp = 0;
    mCameraId = 0;
    mFlushing = false;
    mVcmSteps = 0;
    mVcmStepsFixed = 0;
    mApiVersion = SPRD_API_PORTRAIT_MODE;
    mLocalBufferNumber = 0;
    mIsSupportPBokeh = false;
    mhasCallbackStream = false;
    mJpegOrientation = 0;
    mMaxPendingCount = 0;
    mXmpSize = 0;
    mCallbackOps = NULL;
    mJpegCallbackThread = 0;
    mJpegOutputBuffers = new camera3_stream_buffer_t[1];
#ifdef YUV_CONVERT_TO_JPEG
    mOrigJpegSize = 0;
    mGDepthOriJpegSize = 0;
    m_pDstJpegBuffer = NULL;
    m_pDstGDepthOriJpegBuffer = NULL;
#else
    m_pMainSnapBuffer = NULL;
#endif
    mDepthStatus = DEPTH_INVALID_PORTRAIT;
    mDepthTrigger = TRIGGER_FNUM_PORTRAIT;
    mReqTimestamp = 0;
    mLastOnlieVcm = 0;
    mBokehAlgo = NULL;
    mIsHdrMode = false;
    mIsCapDepthFinish = false;
    mHdrSkipBlur = false;
    mHdrCallbackCnt = 0;
    mAfstate = 0;
    mCameraIdMaster = CAM_PORTRAIT_MAIN_ID;
    mCameraIdSlave = CAM_DEPTH_PORTRAIT_ID;
    mSavedRequestList.clear();
    setupPhysicalCameras();
    mCaptureThread = new BokehCaptureThread();
    mPreviewMuxerThread = new PreviewMuxerThread();
    mDepthMuxerThread = new DepthMuxerThread();
    memset(&mScaleInfo, 0, sizeof(struct img_frm));
    memset(&mBokehSize, 0, sizeof(BokehSize));
    memset(mLocalBuffer, 0, sizeof(new_mem_t) * LOCAL_BUFFER_NUM);
    memset(&mLocalScaledBuffer, 0, sizeof(new_mem_t));
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * PORTRAIT__MAX_NUM_STREAMS);
    memset(mMainStreams, 0,
           sizeof(camera3_stream_t) * PORTRAIT__MAX_NUM_STREAMS);
    memset(mFaceInfo, 0, sizeof(int32_t) * 4);

    memset(&facebeautylevel, 0, sizeof(faceBeautyLevels));
    memset(&mThumbReq, 0, sizeof(multi_request_saved_t));
    memset(&mDepthBuffer, 0, sizeof(DepthBufferPortrait));
    memset(&mOtpData, 0, sizeof(OtpData));
    memset(&mbokehParm, 0, sizeof(bokeh_params));
#ifdef CONFIG_FACE_BEAUTY
    mFaceBeautyFlag = false;
#endif
    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    mPrevFrameNotifyList.clear();
    mCapFaceInfoList.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3Portrait
 *
 * DESCRIPTION     : SprdCamera3Portrait Desctructor
 *
 *==========================================================================*/
SprdCamera3Portrait::~SprdCamera3Portrait() {
    HAL_LOGI("E");
    mCaptureThread = NULL;
    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }

    if (mStaticMetadata)
        free_camera_metadata(mStaticMetadata);

    mJpegCallbackThread = 0;
    delete[] mJpegOutputBuffers;

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : getCameraCapture
 *
 * DESCRIPTION     : Creates Camera Capture if not created
 *
 * PARAMETERS:
 *   @pCapture               : Pointer to retrieve Camera Capture
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3Portrait::getCameraPortrait(SprdCamera3Portrait **pCapture) {
    *pCapture = NULL;
    if (!mPortrait) {
        mPortrait = new SprdCamera3Portrait();
    }
    CHECK_CAPTURE();
    *pCapture = mPortrait;
    HAL_LOGV("mPortrait: %p ", mPortrait);

    return;
}

/*===========================================================================
 * FUNCTION         : get_camera_info
 *
 * DESCRIPTION     : get logical camera info
 *
 * PARAMETERS:
 *   @camera_id     : Logical Camera ID
 *   @info              : Logical main Camera Info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              ENODEV : Camera not found
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Portrait::get_camera_info(__unused int camera_id,
                                         struct camera_info *info) {

    int rc = NO_ERROR;

    HAL_LOGI("E");
    if (info) {
        rc = mPortrait->getCameraInfo(camera_id, info);
    }
    HAL_LOGI("X, rc: %d", rc);

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
int SprdCamera3Portrait::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;

    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    rc = mPortrait->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGD("id= %d, rc: %d", atoi(id), rc);

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
int SprdCamera3Portrait::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }
    return mPortrait->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION   : close  amera device
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
int SprdCamera3Portrait::closeCameraDevice() {

    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    HAL_LOGI("E");
    Mutex::Autolock jcl(mJpegCallbackLock);
    if (!mFlushing) {
        mFlushing = true;
        preClose();
    }
    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = m_nPhyCameras; i > 0; i--) {
        sprdCam = &m_pPhyCamera[i - 1];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL)
            continue;

        HAL_LOGW("camera id:%d", sprdCam->id);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", sprdCam->id);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }

    mReqTimestamp = 0;
    mPrevFrameNumber = 0;
    mCapFrameNumber = 0;
    freeLocalBuffer();
    mSavedRequestList.clear();
    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    mPrevFrameNotifyList.clear();
    mCapFaceInfoList.clear();

    if (mBokehAlgo) {
        rc = mBokehAlgo->deinitAlgo();
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to deinitAlgo");
            goto exit;
        }
        rc = mBokehAlgo->deinitPrevDepth();
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to deinitPrevDepth");
            goto exit;
        }
        rc = mBokehAlgo->deinitCapDepth();
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to deinitCapDepth");
            goto exit;
        }
        rc = mBokehAlgo->deinitPortrait();
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to deinitPortrait");
            goto exit;
        }
        rc = mBokehAlgo->deinitLightPortrait();
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to deinitLightPortrait");
            goto exit;
        }
#ifdef CONFIG_FACE_BEAUTY
        rc = mBokehAlgo->deinitFaceBeauty();
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to deinitFaceBeauty");
            goto exit;
        }
#endif
    }

exit:
    if (mBokehAlgo) {
        delete mBokehAlgo;
        mBokehAlgo = NULL;
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
int SprdCamera3Portrait::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGI("E");
    CHECK_CAPTURE_ERROR();

    rc = mPortrait->initialize(callback_ops);

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
SprdCamera3Portrait::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGD("E");
    if (!mPortrait) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mPortrait->constructDefaultRequestSettings(device, type);

    HAL_LOGD("X");

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
int SprdCamera3Portrait::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();

    rc = mPortrait->configureStreams(device, stream_list);

    HAL_LOGI(" X");

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
int SprdCamera3Portrait::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_CAPTURE_ERROR();
    rc = mPortrait->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3Portrait
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mPortrait->processCaptureResultMain(result);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3Portrait
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mPortrait->processCaptureResultAux(result);
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
void SprdCamera3Portrait::notifyMain(const struct camera3_callback_ops *ops,
                                     const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    mPortrait->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: aux sensor  notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::notifyAux(const struct camera3_callback_ops *ops,
                                    const camera3_notify_msg_t *msg) {
    CHECK_CAPTURE();
    mPortrait->notifyAux(msg);
}

/*===========================================================================
 * FUNCTION   :intDepthPrevBufferFlag
 *
 * DESCRIPTION: for pingpang buffer init
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Portrait::intDepthPrevBufferFlag() {
    int index = 0;
    for (; index < 2; index++) {
        mDepthBuffer.prev_depth_buffer[index].w_flag = true;
        mDepthBuffer.prev_depth_buffer[index].r_flag = false;
    }
}

/*===========================================================================
 * FUNCTION   :getPrevDepthBuffer
 *
 * DESCRIPTION: get pingpang buffer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3Portrait::getPrevDepthBuffer(BUFFER_FLAG_PORTRAIT need_flag) {
    Mutex::Autolock l(mDepthBufferLock);
    int index = 0;
    int ret = -1;

    switch (need_flag) {
    case BUFFER_PING_PORTRAIT:
        for (index = 0; index < 2; index++) {
            if (mDepthBuffer.prev_depth_buffer[index].w_flag &&
                !mDepthBuffer.prev_depth_buffer[index].r_flag) {
                mDepthBuffer.prev_depth_buffer[index].w_flag = false;
                mDepthBuffer.prev_depth_buffer[index].r_flag = false;
                ret = index;
                break;
            }
        }
        break;
    case BUFFER_PANG_PORTRAIT:
        for (index = 0; index < 2; index++) {
            if (mDepthBuffer.prev_depth_buffer[index].r_flag) {
                mDepthBuffer.prev_depth_buffer[index].w_flag = false;
                ret = index;
                break;
            }
        }
        break;
    default:
        HAL_LOGI("buffer flag err");
        break;
    }
    HAL_LOGV("get buffer index=%d,flag=%d", ret, need_flag);

    return ret;
}

/*===========================================================================
 * FUNCTION   :setPrevDepthBufferFlag
 *
 * DESCRIPTION: set pingpang buffer flag
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::setPrevDepthBufferFlag(BUFFER_FLAG_PORTRAIT cur_flag,
                                                 int index) {
    Mutex::Autolock l(mDepthBufferLock);
    int tmp = 0;

    if (index < 0)
        return;

    switch (cur_flag) {
    case BUFFER_PING_PORTRAIT:
        mDepthBuffer.prev_depth_buffer[index].w_flag = true;
        mDepthBuffer.prev_depth_buffer[index].r_flag = true;
        tmp = index == 0 ? 1 : 0;
        mDepthBuffer.prev_depth_buffer[tmp].w_flag = true;
        mDepthBuffer.prev_depth_buffer[tmp].r_flag = false;
        break;
    case BUFFER_PANG_PORTRAIT:
        mDepthBuffer.prev_depth_buffer[index].w_flag = true;
        break;
    default:
        HAL_LOGI("buffer flag err");
        break;
    }
}

/*===========================================================================
 * FUNCTION   :allocatebuf
 *
 * DESCRIPTION: deconstructor of SprdCamera3Portrait
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3Portrait::allocateBuff() {
    int rc = 0;
    size_t count = 0;
    size_t j = 0;
    size_t capture_num = 0;
    size_t preview_num = LOCAL_PREVIEW_NUM / 2;
    int w = 0, h = 0;
    HAL_LOGI(":E");
    mLocalBufferList.clear();
    mSavedRequestList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    mPrevFrameNotifyList.clear();
    freeLocalBuffer();

    for (size_t j = 0; j < preview_num; j++) {
        if (0 > allocateOne(mBokehSize.preview_w, mBokehSize.preview_h,
                            &(mLocalBuffer[j]), YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[j].type = PREVIEW_MAIN_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[j]));

        if (0 > allocateOne(mBokehSize.depth_prev_sub_w,
                            mBokehSize.depth_prev_sub_h,
                            &(mLocalBuffer[preview_num + j]), YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[preview_num + j].type = PREVIEW_DEPTH_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[preview_num + j]));
    }
    count += LOCAL_PREVIEW_NUM;
    /*
    if (mCaptureThread->mAbokehGallery) {
        capture_num = LOCAL_CAPBUFF_NUM - 1;
    } else {
        capture_num = LOCAL_CAPBUFF_NUM;
    }
    */
    capture_num = LOCAL_CAPBUFF_NUM - 1;

    for (j = 0; j < capture_num; j++) {
        if (0 > allocateOne(mBokehSize.callback_w, mBokehSize.callback_h,
                            &(mLocalBuffer[count + j]), YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[count + j].type = SNAPSHOT_MAIN_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
    }
    count += capture_num;

    for (j = 0; j < SNAP_DEPTH_NUM; j++) {
        if (0 > allocateOne(mBokehSize.depth_snap_sub_w,
                            mBokehSize.depth_snap_sub_h,
                            &(mLocalBuffer[count + j]), YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        }
        mLocalBuffer[count + j].type = SNAPSHOT_DEPTH_BUFFER;
        mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
    }
    count += SNAP_DEPTH_NUM;

#ifdef BOKEH_YUV_DATA_TRANSFORM
    if (mBokehSize.transform_w != 0) {
        for (j = 0; j < SNAP_TRANSF_NUM; j++) {
            if (0 > allocateOne(mBokehSize.transform_w, mBokehSize.transform_h,
                                &(mLocalBuffer[count + j]), YUV420)) {
                HAL_LOGE("request one buf failed.");
                goto mem_fail;
            }
            mLocalBuffer[count + j].type = SNAPSHOT_TRANSFORM_BUFFER;
            mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
        }
    }
    count += SNAP_TRANSF_NUM;
#endif

    if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {
        if (0 > allocateOne(mBokehSize.depth_snap_main_w,
                            mBokehSize.depth_snap_main_h, &mLocalScaledBuffer,
                            YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        } else {
            rc = mPortrait->map(&mLocalScaledBuffer.native_handle,
                                (void **)&mScaleInfo.addr_vir.addr_y);
            if (rc != NO_ERROR) {
                HAL_LOGE("map buf failed.");
                goto mem_fail;
            }
            mScaleInfo.fd = ADP_BUFFD(mLocalScaledBuffer.native_handle);
            mScaleInfo.size.width = mLocalScaledBuffer.width;
            mScaleInfo.size.height = mLocalScaledBuffer.height;
        }
        count += SNAP_SCALE_NUM;

        mPortrait->mLocalBufferNumber = count;
        for (j = 0; j < 2; j++) {
            mDepthBuffer.prev_depth_buffer[j].buffer =
                (void *)malloc(mBokehSize.depth_prev_size);
            if (mDepthBuffer.prev_depth_buffer[j].buffer == NULL) {
                HAL_LOGE("mDepthBuffer.prev_depth_buffer malloc failed");
                goto mem_fail;
            } else {
                memset(mDepthBuffer.prev_depth_buffer[j].buffer, 0,
                       mBokehSize.depth_prev_size);
            }
        }
        mDepthBuffer.snap_depth_buffer =
            (void *)malloc(mBokehSize.depth_snap_size);
        if (mDepthBuffer.snap_depth_buffer == NULL) {
            HAL_LOGE("mDepthBuffer.snap_depth_buffer malloc failed");
            goto mem_fail;
        } else {
            memset(mDepthBuffer.snap_depth_buffer, 0,
                   mBokehSize.depth_snap_size);
        }

#if defined(CONFIG_SUPPORT_GDEPTH) ||                                          \
    defined(CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV)
        mDepthBuffer.snap_depthConfidence_buffer =
            (uint8_t *)malloc(mBokehSize.depth_confidence_map_size);
        if (mDepthBuffer.snap_depthConfidence_buffer == NULL) {
            HAL_LOGE("mDepthBuffer.snap_depth_buffer malloc failed");
            goto mem_fail;
        } else {
            memset(mDepthBuffer.snap_depthConfidence_buffer, 0,
                   mBokehSize.depth_confidence_map_size);
        }
        mDepthBuffer.depth_normalize_data_buffer =
            (uint8_t *)malloc(mBokehSize.depth_yuv_normalize_size);
        if (mDepthBuffer.depth_normalize_data_buffer == NULL) {
            HAL_LOGE("mDepthBuffer.depth_normalize_data_buffer malloc failed");
            goto mem_fail;
        } else {
            memset(mDepthBuffer.depth_normalize_data_buffer, 0,
                   mBokehSize.depth_yuv_normalize_size);
        }
#endif

        mDepthBuffer.depth_out_map_table =
            (void *)malloc(mBokehSize.depth_weight_map_size);
        if (mDepthBuffer.depth_out_map_table == NULL) {
            HAL_LOGE("mDepthBuffer.depth_out_map_table  malloc failed.");
            goto mem_fail;
        } else {
            memset(mDepthBuffer.depth_out_map_table, 0,
                   mBokehSize.depth_weight_map_size);
        }
        mDepthBuffer.prev_depth_scale_buffer =
            (void *)malloc(mBokehSize.depth_prev_scale_size);
        if (mDepthBuffer.prev_depth_scale_buffer == NULL) {
            HAL_LOGE("mDepthBuffer.prev_depth_scale_buffer  malloc failed.");
            goto mem_fail;
        } else {
            memset(mDepthBuffer.prev_depth_scale_buffer, 0,
                   mBokehSize.depth_prev_scale_size);
        }
    }

    HAL_LOGI("X,count=%d", count);

    return rc;

mem_fail:
    freeLocalBuffer();
    mLocalBufferList.clear();
    return -1;
}

/*===========================================================================
 * FUNCTION         : freeLocalBuffer
 *
 * DESCRIPTION     : free native_handle_t buffer
 *
 * PARAMETERS:
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3Portrait::freeLocalBuffer() {
    for (int i = 0; i < mLocalBufferNumber; i++) {
        freeOneBuffer(&mLocalBuffer[i]);
    }
    if (mLocalScaledBuffer.native_handle) {
        if (mScaleInfo.addr_vir.addr_y) {
            unmap(&mLocalScaledBuffer.native_handle);
            memset(&mScaleInfo, 0, sizeof(struct img_frm));
        }
        freeOneBuffer(&mLocalScaledBuffer);
    }
    if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {

        for (int i = 0; i < 2; i++) {
            if (mDepthBuffer.prev_depth_buffer[i].buffer != NULL) {
                free(mDepthBuffer.prev_depth_buffer[i].buffer);
                mDepthBuffer.prev_depth_buffer[i].buffer = NULL;
            }
        }
        if (mDepthBuffer.snap_depth_buffer != NULL) {
            free(mDepthBuffer.snap_depth_buffer);
            mDepthBuffer.snap_depth_buffer = NULL;
        }

#if defined(CONFIG_SUPPORT_GDEPTH) ||                                          \
    defined(CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV)
        if (mDepthBuffer.snap_depthConfidence_buffer != NULL) {
            free(mDepthBuffer.snap_depthConfidence_buffer);
            mDepthBuffer.snap_depthConfidence_buffer = NULL;
        }
        if (mDepthBuffer.depth_normalize_data_buffer != NULL) {
            free(mDepthBuffer.depth_normalize_data_buffer);
            mDepthBuffer.depth_normalize_data_buffer = NULL;
        }
#endif

        if (mDepthBuffer.depth_out_map_table != NULL) {
            free(mDepthBuffer.depth_out_map_table);
            mDepthBuffer.depth_out_map_table = NULL;
        }
        if (mDepthBuffer.prev_depth_scale_buffer != NULL) {
            free(mDepthBuffer.prev_depth_scale_buffer);
            mDepthBuffer.prev_depth_scale_buffer = NULL;
        }
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
int SprdCamera3Portrait::cameraDeviceOpen(__unused int camera_id,
                                          struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint8_t phyId = 0;

    HAL_LOGI(" E");
    hw_device_t *hw_dev[m_nPhyCameras];
    mCameraId = mCameraIdMaster;
    m_VirtualCamera.id = (uint8_t)mCameraIdMaster;
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((int)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode(MODE_BOKEH);
        hw->setMasterId(mCameraIdMaster);

        rc = hw->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hw;
            closeCameraDevice();
            return rc;
        }
#ifdef CONFIG_BOKEH_CROP
        if (phyId == CAM_PORTRAIT_MAIN_ID) {
            char prop[PROPERTY_VALUE_MAX] = {
                0,
            };
            property_get("persist.vendor.cam.bokeh.crop.enable", prop, "0");
            if (atoi(prop)) {
                property_get("persist.vendor.cam.bokeh.crop.start_x", prop,
                             "0");
                bokeh_trim_info.start_x = atoi(prop);
                property_get("persist.vendor.cam.bokeh.crop.start_y", prop,
                             "0");
                bokeh_trim_info.start_y = atoi(prop);
                property_get("persist.vendor.cam.bokeh.crop.width", prop, "0");
                bokeh_trim_info.width = atoi(prop);
                property_get("persist.vendor.cam.bokeh.crop.height", prop, "0");
                bokeh_trim_info.height = atoi(prop);
            }
            hw->camera_ioctrl(CAMERA_IOCTRL_SET_TRIM_INFO, &bokeh_trim_info,
                              NULL);
        }
#endif
        HAL_LOGD("open id=%d success", phyId);
        m_pPhyCamera[i].dev = reinterpret_cast<camera3_device_t *>(hw_dev[i]);
        m_pPhyCamera[i].hwi = hw;
    }

    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraCaptureOps;
    m_VirtualCamera.dev.priv = (void *)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.bokeh.api.version", prop, "0");
    mApiVersion = atoi(prop);
    HAL_LOGD("api version %d", mApiVersion);
    if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {
#ifdef CONFIG_PORTRAIT_SUPPORT
        mBokehAlgo = new SprdPortraitAlgo();
        mLocalBufferNumber = LOCAL_BUFFER_NUM;
#endif
        mLocalBufferNumber = LOCAL_BUFFER_NUM;
    }
    HAL_LOGI("X,mLocalBufferNumber=%d", mLocalBufferNumber);
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
int SprdCamera3Portrait::getCameraInfo(int id, struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;
    int32_t img_size = 0;
    int version = SPRD_API_PORTRAIT_MODE;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    int32_t jpeg_stream_size = 0;
    struct logicalSensorInfo *logicalPtr = NULL;

    HAL_LOGI("E, camera_id = %d", camera_id);
    if (mStaticMetadata)
        free_camera_metadata(mStaticMetadata);
    logicalPtr = sensorGetLogicaInfo4multiCameraId(id);
    if (logicalPtr) {
        if (mPortrait->m_nPhyCameras == logicalPtr->physicalNum) {
            for (int i = 0; i < logicalPtr->physicalNum; i++) {
                mPortrait->m_pPhyCamera[i].id = (uint8_t)logicalPtr->phyIdGroup[i].phyId;
                if (SENSOR_ROLE_DUALCAM_MASTER == logicalPtr->phyIdGroup[i].sensor_role)
                    mPortrait->mCameraIdMaster = mPortrait->m_pPhyCamera[i].id;
                if (SENSOR_ROLE_DUALCAM_SLAVE == logicalPtr->phyIdGroup[i].sensor_role)
                    mPortrait->mCameraIdSlave = mPortrait->m_pPhyCamera[i].id;
                HAL_LOGD("i = %d, phyId = %d", i, logicalPtr->phyIdGroup[i].phyId);
            }
        }
    }

    m_VirtualCamera.id = mPortrait->mCameraIdMaster;
    camera_id = (int)m_VirtualCamera.id;
    SprdCamera3Setting::initDefaultParameters(camera_id);
    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    CameraMetadata metadata = clone_camera_metadata(mStaticMetadata);
    property_get("persist.vendor.cam.res.bokeh", prop, "RES_5M");
    HAL_LOGI("bokeh support cap resolution %s", prop);
    addAvailableStreamSize(metadata, prop);
    jpeg_stream_size = getJpegStreamSize(prop);
    property_get("persist.vendor.cam.api.version", prop, "0");
    version = atoi(prop);
    if (version == SPRD_API_PORTRAIT_MODE) {
        img_size = jpeg_stream_size +
                   (DEPTH_SNAP_OUTPUT_WIDTH * DEPTH_SNAP_OUTPUT_HEIGHT * 2) +
                   (BOKEH_REFOCUS_COMMON_PARAM_NUM * 4) +
                   BOKEH_REFOCUS_COMMON_XMP_SIZE + sizeof(camera3_jpeg_blob_t) +
                   1024;
    }
    metadata.update(ANDROID_JPEG_MAX_SIZE, &img_size, 1);

    if (SPRD_MULTI_CAMERA_BASE_ID > id) {
        HAL_LOGI(" logical id %d", id);
        setLogicIdTag(metadata, (uint8_t *)kavailable_physical_ids, 4);
    }

    mStaticMetadata = metadata.release();

    SprdCamera3Setting::getCameraInfo(camera_id, info);
    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;
    HAL_LOGI("X rc=%d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION         : getDepthImageSize
 *
 * DESCRIPTION     : getDepthImageSize
 *
 *==========================================================================*/
void SprdCamera3Portrait::getDepthImageSize(int inputWidth, int inputHeight,
                                            int *outWidth, int *outHeight,
                                            int type) {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    int value = 0;
    if (mApiVersion == SPRD_API_PORTRAIT_MODE) {
        int aspect_ratio = (inputWidth * SFACTOR) / inputHeight;
        if (abs(aspect_ratio - AR4_3) < 1) {
            *outWidth = 240;
            *outHeight = 180;
        } else if (abs(aspect_ratio - AR16_9) < 1) {
            *outWidth = 320;
            *outHeight = 180;
        } else {
            *outWidth = 240;
            *outHeight = 180;
        }
        // TODO:Remove this after depth engine support 16:9
        if (type == PREVIEW_STREAM) {
            *outWidth = CAM_AUX_PREV_WIDTH;
            *outHeight = CAM_AUX_PREV_HEIGHT;
        } else if (type == SNAPSHOT_STREAM) {
            *outWidth = CAM_AUX_SNAP_WIDTH;
            *outHeight = CAM_AUX_SNAP_HEIGHT;

            property_get("persist.vendor.cam.bokeh.main.w", prop, "800");
            value = atoi(prop);
            mBokehSize.depth_snap_main_w = value;

            property_get("persist.vendor.cam.bokeh.main.h", prop, "600");
            value = atoi(prop);
            mBokehSize.depth_snap_main_h = value;
        }
    }

    HAL_LOGD("iw:%d,ih:%d,ow:%d,oh:%d", inputWidth, inputHeight, *outWidth,
             *outHeight);
}

/*===========================================================================
 * FUNCTION         : setupPhysicalCameras
 *
 * DESCRIPTION     : Creates Camera Capture if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Portrait::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].id = (uint8_t)CAM_PORTRAIT_MAIN_ID;
    m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].id = (uint8_t)CAM_DEPTH_PORTRAIT_ID;

    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   :PreviewMuxerThread
 *
 * DESCRIPTION: constructor of PreviewMuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Portrait::PreviewMuxerThread::PreviewMuxerThread() {
    HAL_LOGI("E");
    mPreviewMuxerMsgList.clear();
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~PreviewMuxerThread
 *
 * DESCRIPTION: deconstructor of PreviewMuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Portrait::PreviewMuxerThread::~PreviewMuxerThread() {
    HAL_LOGI(" E");

    mPreviewMuxerMsgList.clear();

    HAL_LOGI("X");
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

bool SprdCamera3Portrait::PreviewMuxerThread::threadLoop() {
    muxer_queue_msg_t muxer_msg;
    buffer_handle_t *output_buffer = NULL;
    uint32_t frame_number = 0;
    int rc = 0;
    int mBokehMode = 0, lightPortraitType = 0, mFaceBeautyFlag = 0;

    while (!mPreviewMuxerMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mPreviewMuxerMsgList.begin();
            muxer_msg = *it;
            mPreviewMuxerMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_INIT: {
            rc = mPortrait->mBokehAlgo->initAlgo();
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to initAlgo");
                // return rc;
            }
            rc = mPortrait->mBokehAlgo->initPrevDepth();
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to initPrevDepth");
                // return rc;
            }
#ifdef CONFIG_DECRYPT_DEPTH
            char userset[] = "12345";
            rc = mPortrait->mBokehAlgo->setUserset(userset, sizeof(userset));
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to setUserset");
                return rc;
            }
#endif
        } break;
        case MUXER_MSG_EXIT: {

            List<multi_request_saved_t>::iterator itor =
                mPortrait->mSavedRequestList.begin();
            HAL_LOGD("exit frame_number %u", itor->frame_number);
            while (itor != mPortrait->mSavedRequestList.end()) {
                frame_number = itor->frame_number;
                itor++;
                mPortrait->CallBackResult(frame_number,
                                          CAMERA3_BUFFER_STATUS_ERROR);
            }
        }
            return false;
        case MUXER_MSG_DATA_PROC: {
            {
                Mutex::Autolock l(mPortrait->mRequestLock);
                List<multi_request_saved_t>::iterator itor =
                    mPortrait->mSavedRequestList.begin();
                HAL_LOGV("muxer_msg.combo_frame.frame_number %d",
                         muxer_msg.combo_frame.frame_number);
                while (itor != mPortrait->mSavedRequestList.end()) {
                    if (itor->frame_number ==
                        muxer_msg.combo_frame.frame_number) {
                        output_buffer = itor->buffer;
                        mPortrait->mPrevBlurFrameNumber =
                            muxer_msg.combo_frame.frame_number;
                        frame_number = muxer_msg.combo_frame.frame_number;
                        break;
                    }
                    itor++;
                }
            }
            mBokehMode = mPortrait->mBokehMode;
            lightPortraitType = mPortrait->lightPortraitType;
            mFaceBeautyFlag = mPortrait->mFaceBeautyFlag;

            if (output_buffer != NULL) {
                bool isDoDepth = false;
                void *output_buf_addr = NULL;
                void *input_buf1_addr = NULL;
                if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE &&
                    ((mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE) ||
                     (mFaceBeautyFlag) || (lightPortraitType != 0))) {
                    if (mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE &&
                        mPortrait->mPrevPortrait) {
                        rc = sprdBokehPreviewHandle(
                            output_buffer, muxer_msg.combo_frame.buffer1);
                        if (rc != NO_ERROR) {
                            HAL_LOGE("sprdBokehPreviewHandle failed");
                            return false;
                        }
                        if (mPortrait->mDepthTrigger !=
                                TRIGGER_FALSE_PORTRAIT &&
                            mPortrait->mPortraitFlag) {
                            isDoDepth = sprdDepthHandle(&muxer_msg);
                        }
                    } else if (mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE &&
                               !mPortrait->mPrevPortrait) {
                        rc = mPortrait->map(output_buffer, &output_buf_addr);
                        rc = mPortrait->map(muxer_msg.combo_frame.buffer1,
                                            &input_buf1_addr);
                        memcpy(output_buf_addr, input_buf1_addr,
                               ADP_BUFSIZE(*muxer_msg.combo_frame.buffer1));
                        mPortrait->flushIonBuffer(ADP_BUFFD(*output_buffer),
                                                  output_buf_addr,
                                                  ADP_BUFSIZE(*output_buffer));
                        mPortrait->unmap(muxer_msg.combo_frame.buffer1);
                        mPortrait->unmap(output_buffer);
                        rc = NO_ERROR;
                    }
                    rc = mPortrait->map(output_buffer, &output_buf_addr);
                    if (mFaceBeautyFlag) {
                        if (mBokehMode == CAM_COMMON_MODE) {
                            rc = mPortrait->map(muxer_msg.combo_frame.buffer1,
                                                &input_buf1_addr);
                            memcpy(output_buf_addr, input_buf1_addr,
                                   ADP_BUFSIZE(*muxer_msg.combo_frame.buffer1));
                        }
                        rc = mPortrait->mBokehAlgo->doFaceBeauty(
                            NULL, output_buf_addr,
                            mPortrait->mBokehSize.preview_w,
                            mPortrait->mBokehSize.preview_h, 0,
                            &mPortrait->facebeautylevel);
                        if (mBokehMode == CAM_COMMON_MODE) {
                            mPortrait->unmap(muxer_msg.combo_frame.buffer1);
                        }
                    }
                    if (lightPortraitType != 0) {
                        if (rc != NO_ERROR) {
                            HAL_LOGE("fail to map output buffer");
                        }
                        if (mBokehMode == CAM_COMMON_MODE && !mFaceBeautyFlag) {
                            rc = mPortrait->map(muxer_msg.combo_frame.buffer1,
                                                &input_buf1_addr);
                            if (rc != NO_ERROR) {
                                HAL_LOGE("fail to map input buffer");
                            }
                            memcpy(output_buf_addr, input_buf1_addr,
                                   ADP_BUFSIZE(*muxer_msg.combo_frame.buffer1));
                        }
                        rc = mPortrait->mBokehAlgo->prevLPT(
                            output_buf_addr, mPortrait->mBokehSize.preview_w,
                            mPortrait->mBokehSize.preview_h);
                        if (rc != NO_ERROR) {
                            HAL_LOGE("fail to prevDfaAndLPT");
                        }
                        if (mBokehMode == CAM_COMMON_MODE && !mFaceBeautyFlag) {
                            mPortrait->unmap(muxer_msg.combo_frame.buffer1);
                        }
                    }
                    mPortrait->unmap(output_buffer);
                } else {
                    rc = mPortrait->map(output_buffer, &output_buf_addr);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("fail to map output buffer");
                        goto fail_map_output;
                    }
                    rc = mPortrait->map(muxer_msg.combo_frame.buffer1,
                                        &input_buf1_addr);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("fail to map input buffer");
                        goto fail_map_input;
                    }
                    memcpy(output_buf_addr, input_buf1_addr,
                           ADP_BUFSIZE(*muxer_msg.combo_frame.buffer1));

                    mPortrait->flushIonBuffer(ADP_BUFFD(*output_buffer),
                                              output_buf_addr,
                                              ADP_BUFSIZE(*output_buffer));

                    mPortrait->unmap(muxer_msg.combo_frame.buffer1);
                fail_map_input:
                    mPortrait->unmap(output_buffer);
                fail_map_output:
                    rc = NO_ERROR;
                }
                if (!isDoDepth) {
                    mPortrait->pushBufferList(mPortrait->mLocalBuffer,
                                              muxer_msg.combo_frame.buffer1,
                                              mPortrait->mLocalBufferNumber,
                                              mPortrait->mLocalBufferList);
                    mPortrait->pushBufferList(mPortrait->mLocalBuffer,
                                              muxer_msg.combo_frame.buffer2,
                                              mPortrait->mLocalBufferNumber,
                                              mPortrait->mLocalBufferList);
                }
                mPortrait->CallBackResult(frame_number,
                                          CAMERA3_BUFFER_STATUS_OK);
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
 * FUNCTION   :sprdDepthHandle
 *
 * DESCRIPTION: sprdDepthHandle
 *
 * PARAMETERS :
 *
 * RETURN	  : None
 *==========================================================================*/

bool SprdCamera3Portrait::PreviewMuxerThread::sprdDepthHandle(
    muxer_queue_msg_t *muxer_msg) {
    bool ret = false;
    muxer_queue_msg_t send_muxer_msg = *muxer_msg;

    Mutex::Autolock l(mPortrait->mDepthMuxerThread->mMergequeueMutex);
    HAL_LOGV("Enqueue combo frame:%d for frame depth!,size=%d",
             send_muxer_msg.combo_frame.frame_number,
             mPortrait->mDepthMuxerThread->mDepthMuxerMsgList.size());
    if (mPortrait->mDepthMuxerThread->mDepthMuxerMsgList.empty()) {
        mPortrait->mDepthMuxerThread->mDepthMuxerMsgList.push_back(
            send_muxer_msg);
        mPortrait->mDepthMuxerThread->mMergequeueSignal.signal();
        ret = true;
    }
    return ret;
}

/*===========================================================================
 * FUNCTION   :sprdBokehPreviewHandle
 *
 * DESCRIPTION: sprdBokehPreviewHandle
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Portrait::PreviewMuxerThread::sprdBokehPreviewHandle(
    buffer_handle_t *output_buf, buffer_handle_t *input_buf1) {
    int rc = NO_ERROR;
    void *output_buf_addr = NULL;
    void *input_buf1_addr = NULL;
    int buffer_index = 0;
    Mutex::Autolock l(mLock);
    HAL_LOGD("E");

    if (output_buf == NULL || input_buf1 == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }
    buffer_index = mPortrait->getPrevDepthBuffer(BUFFER_PANG_PORTRAIT);

    if (mPortrait->mIsSupportPBokeh &&
        (mPortrait->mDepthStatus == DEPTH_DONE_PORTRAIT ||
         (mPortrait->mDepthStatus == DEPTH_DONING_PORTRAIT &&
          mPortrait->mDepthTrigger == TRIGGER_FNUM_PORTRAIT)) &&
        buffer_index != -1) {
        unsigned char *depth_buffer =
            (unsigned char *)(mPortrait->mDepthBuffer
                                  .prev_depth_buffer[buffer_index]
                                  .buffer);
        /*Bokeh GPU interface start*/
        uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                                GraphicBuffer::USAGE_SW_READ_OFTEN |
                                GraphicBuffer::USAGE_SW_WRITE_OFTEN;
        int32_t yuvTextFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        uint32_t inWidth = 0, inHeight = 0, inStride = 0;
        if (!mPortrait->mIommuEnabled) {
            yuvTextUsage |= GRALLOC_USAGE_VIDEO_BUFFER;
        }

#if defined(CONFIG_SPRD_ANDROID_8)
        uint32_t inLayCount = 1;
#endif
        inWidth = mPortrait->mBokehSize.preview_w;
        inHeight = mPortrait->mBokehSize.preview_h;
        inStride = mPortrait->mBokehSize.preview_w;

#if defined(CONFIG_SPRD_ANDROID_8)

        sp<GraphicBuffer> srcBuffer = new GraphicBuffer(
            (native_handle_t *)(*input_buf1),
            GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
            yuvTextFormat, inLayCount, yuvTextUsage, inStride);
        sp<GraphicBuffer> dstBuffer = new GraphicBuffer(
            (native_handle_t *)(*output_buf),
            GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
            yuvTextFormat, inLayCount, yuvTextUsage, inStride);
#else

        sp<GraphicBuffer> srcBuffer =
            new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                              inStride, (native_handle_t *)(*input_buf1), 0);
        sp<GraphicBuffer> dstBuffer =
            new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                              inStride, (native_handle_t *)(*output_buf), 0);
#endif
        mPortrait->mBokehAlgo->prevBluImage(srcBuffer, dstBuffer,
                                            (void *)depth_buffer);
        {
            char prop[PROPERTY_VALUE_MAX] = {
                0,
            };

            property_get("persist.vendor.cam.bokeh.dump", prop, "0");
            if (!strcmp(prop, "sprdpreyuv") || !strcmp(prop, "all")) {
                rc = mPortrait->map(output_buf, &output_buf_addr);
                if (rc != NO_ERROR) {
                    HAL_LOGE("fail to map output buffer");
                }
                mPortrait->dumpData(
                    (unsigned char *)output_buf_addr, 1,
                    ADP_BUFSIZE(*output_buf), mPortrait->mBokehSize.preview_w,
                    mPortrait->mBokehSize.preview_h,
                    mPortrait->mPrevBlurFrameNumber, "bokeh_blur");
                mPortrait->unmap(output_buf);
            }
        }

        mPortrait->setPrevDepthBufferFlag(BUFFER_PANG_PORTRAIT, buffer_index);
    } else {

        rc = mPortrait->map(output_buf, &output_buf_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map output buffer");
            goto fail_map_output;
        }
        rc = mPortrait->map(input_buf1, &input_buf1_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map input buffer");
            goto fail_map_input;
        }

        memcpy(output_buf_addr, input_buf1_addr, ADP_BUFSIZE(*input_buf1));
        mPortrait->flushIonBuffer(ADP_BUFFD(*output_buf), output_buf_addr,
                                  ADP_BUFSIZE(*output_buf));

        mPortrait->unmap(input_buf1);
    fail_map_input:
        mPortrait->unmap(output_buf);
    fail_map_output:
        rc = NO_ERROR;
    }
    HAL_LOGD("X");

    return rc;
}
/*===========================================================================
 * FUNCTION   :requestInit
 *
 * DESCRIPTION: request thread init
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::PreviewMuxerThread::requestInit() {
    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_INIT;
    mPreviewMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
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
void SprdCamera3Portrait::PreviewMuxerThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mPreviewMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::PreviewMuxerThread::waitMsgAvailable() {
    while (mPreviewMuxerMsgList.empty()) {
        {
            Mutex::Autolock l(mMergequeueMutex);
            mMergequeueSignal.waitRelative(mMergequeueMutex,
                                           BOKEH_THREAD_TIMEOUT);
        }
        struct timespec t1;
        clock_gettime(CLOCK_BOOTTIME, &t1);
        uint64_t now_time_stamp = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
        if (mPortrait->mReqTimestamp && mPortrait->mPrevFrameNumber != 0 &&
            (ns2ms(now_time_stamp - mPortrait->mReqTimestamp) >
             CLEAR_PRE_FRAME_UNMATCH_TIMEOUT)) {
            HAL_LOGV("clear unmatch frame for force kill app");
            Mutex::Autolock l(mPortrait->mUnmatchedQueueLock);
            mPortrait->clearFrameNeverMatched(mPortrait->mPrevFrameNumber + 1,
                                              mPortrait->mPrevFrameNumber + 1);
        }
    }
}

/*===========================================================================
 * FUNCTION   :DepthMuxerThread
 *
 * DESCRIPTION: constructor of DepthMuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Portrait::DepthMuxerThread::DepthMuxerThread() {
    HAL_LOGI("E");
    mDepthMuxerMsgList.clear();
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~DepthMuxerThread
 *
 * DESCRIPTION: deconstructor of DepthMuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Portrait::DepthMuxerThread::~DepthMuxerThread() {
    HAL_LOGI(" E");

    mDepthMuxerMsgList.clear();

    HAL_LOGI("X");
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

bool SprdCamera3Portrait::DepthMuxerThread::threadLoop() {
    muxer_queue_msg_t muxer_msg;
    buffer_handle_t *depth_output_buffer = NULL;
    int rc = 0;

    while (!mDepthMuxerMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mDepthMuxerMsgList.begin();
            muxer_msg = *it;
        }
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT: {
            {
                Mutex::Autolock l(mMergequeueMutex);
                mDepthMuxerMsgList.erase(it);
            }
            return false;
        case MUXER_MSG_DATA_PROC: {

            mPortrait->setDepthStatus(DEPTH_DONING_PORTRAIT);
            rc = sprdDepthDo(muxer_msg.combo_frame.buffer1,
                             muxer_msg.combo_frame.buffer2);
            if (rc != NO_ERROR) {
                HAL_LOGE("sprdDepthDo failed");
                mPortrait->setDepthStatus(DEPTH_INVALID_PORTRAIT);
            }

            mPortrait->pushBufferList(
                mPortrait->mLocalBuffer, muxer_msg.combo_frame.buffer1,
                mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);
            mPortrait->pushBufferList(
                mPortrait->mLocalBuffer, muxer_msg.combo_frame.buffer2,
                mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);
            {
                Mutex::Autolock l(mMergequeueMutex);
                mDepthMuxerMsgList.erase(it);
            }

        } break;
        default:
            break;
        }
        };
    }
    waitMsgAvailable();

    return true;
}

/*===========================================================================
 * FUNCTION   :sprdDepthDo
 *
 * DESCRIPTION:sprdDepthDo
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Portrait::DepthMuxerThread::sprdDepthDo(
    buffer_handle_t *input_buf1, buffer_handle_t *input_buf2) {
    int rc = NO_ERROR;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    int buffer_index = 0;
    if (input_buf1 == NULL || input_buf2 == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }

    rc = mPortrait->map(input_buf1, &input_buf1_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer1");
        goto fail_map_input1;
    }

    rc = mPortrait->map(input_buf2, &input_buf2_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer2");
        goto fail_map_input2;
    }
    buffer_index = mPortrait->getPrevDepthBuffer(BUFFER_PING_PORTRAIT);
    if (buffer_index == -1) {
        goto fail_map_input3;
    }

    mPortrait->mLastOnlieVcm = 0;
    rc = mPortrait->mBokehAlgo->onLine(
        mPortrait->mDepthBuffer.depth_out_map_table, input_buf1_addr,
        input_buf2_addr, mPortrait->mDepthBuffer.prev_depth_scale_buffer);
    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("Sprd algo onLine failed! %d", rc);
        mPortrait->mLastOnlieVcm = 0;
        goto fail_map_input3;
    } else {
        mPortrait->mLastOnlieVcm = mPortrait->mVcmSteps;
    }

    rc = mPortrait->mBokehAlgo->prevDepthRun(
        mPortrait->mDepthBuffer.prev_depth_buffer[buffer_index].buffer,
        input_buf1_addr, input_buf2_addr,
        mPortrait->mDepthBuffer.prev_depth_scale_buffer);

    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("sprd_depth_Run_distance failed! %d", rc);
        goto fail_map_input3;
    }
    mPortrait->setPrevDepthBufferFlag(BUFFER_PING_PORTRAIT, buffer_index);
    mPortrait->setDepthStatus(DEPTH_DONE_PORTRAIT);

    {
        char prop[PROPERTY_VALUE_MAX] = {
            0,
        };

        property_get("persist.vendor.cam.bokeh.dump", prop, "0");
        if (!strcmp(prop, "predepth") || !strcmp(prop, "all")) {
            mPortrait->dumpData((unsigned char *)mPortrait->mDepthBuffer
                                    .prev_depth_buffer[buffer_index]
                                    .buffer,
                                6, mPortrait->mBokehSize.depth_prev_size,
                                mPortrait->mBokehSize.preview_w,
                                mPortrait->mBokehSize.preview_h,
                                mPortrait->mPrevBlurFrameNumber, "depth");
            mPortrait->dumpData((unsigned char *)input_buf2_addr, 1,
                                ADP_BUFSIZE(*input_buf2),
                                mPortrait->mBokehSize.depth_prev_sub_w,
                                mPortrait->mBokehSize.depth_prev_sub_h,
                                mPortrait->mPrevBlurFrameNumber, "prevSub");
            mPortrait->dumpData((unsigned char *)input_buf1_addr, 1,
                                ADP_BUFSIZE(*input_buf1),
                                mPortrait->mBokehSize.preview_w,
                                mPortrait->mBokehSize.preview_h,
                                mPortrait->mPrevBlurFrameNumber, "prevMain");
        }
    }
fail_map_input3:
    mPortrait->unmap(input_buf2);
fail_map_input2:
    mPortrait->unmap(input_buf1);
fail_map_input1:

    return rc;
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
void SprdCamera3Portrait::DepthMuxerThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mDepthMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::DepthMuxerThread::waitMsgAvailable() {
    while (mDepthMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex,
                                       BOKEH_THREAD_TIMEOUT * 2);
    }
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
SprdCamera3Portrait::BokehCaptureThread::BokehCaptureThread() {
    HAL_LOGI(" E");
    mSavedResultBuff = NULL;
    mSavedCapReqsettings = NULL;
    mReprocessing = false;
    mSavedOneResultBuff = NULL;
    mDevmain = NULL;
    mCallbackOps = NULL;
    char prop1[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.gallery.abokeh", prop1, "0");
    if (1 == atoi(prop1)) {
        mAbokehGallery = true;
    } else {
        mAbokehGallery = false;
    }
    mBokehResult = true;
    memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
    memset(&mSavedCapReqStreamBuff, 0, sizeof(camera3_stream_buffer_t));

    mCaptureMsgList.clear();
    HAL_LOGI(" X");
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
SprdCamera3Portrait::BokehCaptureThread::~BokehCaptureThread() {
    HAL_LOGI(" E");
    mCaptureMsgList.clear();
    HAL_LOGI(" X");
}

/*===========================================================================
 * FUNCTION   :saveCaptureBokehParams
 *
 * DESCRIPTION: save capture bokeh params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3Portrait::BokehCaptureThread::saveCaptureBokehParams(
    unsigned char *result_buffer_addr, uint32_t result_buffer_size,
    size_t jpeg_size) {
    int i = 0;
    int ret = NO_ERROR;
    uint32_t depth_size = 0;
    uint32_t depth_yuv_normalize_size = 0;
    int xmp_size = 0;
    int para_size = 0;
    BOKEH_PARAM param;
    memset(&param, 0, sizeof(BOKEH_PARAM));
    mPortrait->mBokehAlgo->getBokenParam((void *)&param);
    if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {
        if (result_buffer_addr == NULL) {
            HAL_LOGE("result_buff is NULL!");
            return BAD_VALUE;
        }
        xmp_size = BOKEH_REFOCUS_COMMON_XMP_SIZE;
        para_size = BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
        depth_size = mPortrait->mBokehSize.depth_snap_size;
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        depth_yuv_normalize_size =
            mPortrait->mBokehSize.depth_yuv_normalize_size;
#endif
    }

    unsigned char *buffer_base = result_buffer_addr;
    unsigned char *depth_yuv = NULL;
    unsigned char *depthJpg = NULL;
    unsigned char *depth_yuv_normalize = NULL;
#ifdef YUV_CONVERT_TO_JPEG
    unsigned char *orig_jpeg_data = NULL;
#else
    unsigned char *orig_yuv_data = NULL;
#endif

    uint32_t orig_yuv_size = mPortrait->mBokehSize.capture_w *
                             mPortrait->mBokehSize.capture_h * 3 / 2;
#ifdef YUV_CONVERT_TO_JPEG
    uint32_t use_size = para_size + depth_yuv_normalize_size + depth_size +
                        mPortrait->mOrigJpegSize + jpeg_size + xmp_size;
    uint32_t orig_jpeg_size = mPortrait->mOrigJpegSize;
    uint32_t process_jpeg = jpeg_size;
#else
    uint32_t use_size = para_size + depth_size + orig_yuv_size + jpeg_size;
#endif
    if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {

        depth_yuv = (unsigned char *)mPortrait->mDepthBuffer.snap_depth_buffer;

#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        depth_yuv_normalize =
            (unsigned char *)
                mPortrait->mDepthBuffer.depth_normalize_data_buffer;
#endif

        // refocus commom param
        uint32_t far = mPortrait->far;
        uint32_t near = mPortrait->near;
        uint32_t format = 1;
        uint32_t mime = 0;
        uint32_t param_num = BOKEH_REFOCUS_COMMON_PARAM_NUM;
        uint32_t main_width = mPortrait->mBokehSize.capture_w;
        uint32_t main_height = mPortrait->mBokehSize.capture_h;
        uint32_t depth_width = mPortrait->mBokehSize.depth_snap_out_w;
        uint32_t depth_height = mPortrait->mBokehSize.depth_snap_out_h;
        uint32_t bokeh_level = param.sprd.cap.bokeh_level;
        uint32_t position_x = param.sprd.cap.sel_x;
        uint32_t position_y = param.sprd.cap.sel_y;
        uint32_t param_state = param.sprd.cap.param_state;
        uint32_t rotation = SprdCamera3Setting::s_setting[mPortrait->mCameraId]
                                .jpgInfo.orientation;
        uint32_t bokeh_version = 0;
        bokeh_version = 0x1C0101;

#ifdef CONFIG_DECRYPT_DEPTH
        uint32_t decrypt_mode = 1;
#else
        uint32_t decrypt_mode = 0;
#endif

        unsigned char bokeh_flag[] = {'B', 'O', 'K', 'E'};
        depth_size = mPortrait->mBokehSize.depth_snap_size;
        unsigned char *p[] = {
            (unsigned char *)&decrypt_mode,
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
            (unsigned char *)&near,          (unsigned char *)&far,
            (unsigned char *)&format,        (unsigned char *)&mime,
#endif
            (unsigned char *)&main_width,    (unsigned char *)&main_height,
            (unsigned char *)&depth_width,   (unsigned char *)&depth_height,
            (unsigned char *)&depth_size,    (unsigned char *)&bokeh_level,
            (unsigned char *)&position_x,    (unsigned char *)&position_y,
            (unsigned char *)&param_state,   (unsigned char *)&rotation,
#ifdef YUV_CONVERT_TO_JPEG
            (unsigned char *)&param_num,     (unsigned char *)&orig_jpeg_size,
#endif
            (unsigned char *)&bokeh_version, (unsigned char *)&bokeh_flag};
        HAL_LOGD("sprd param: %d ,%d , %d, %d, %d, %d, %d, %d, %d, %d, %x, %s",
                 main_width, main_height, depth_width, depth_height, depth_size,
                 bokeh_level, position_x, position_y, param_state, rotation,
                 bokeh_version, bokeh_flag);
#ifdef YUV_CONVERT_TO_JPEG
        HAL_LOGD("process_jpeg %d,orig_jpeg_size %d,pram_num=%d", process_jpeg,
                 orig_jpeg_size, param_num);
#endif
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        HAL_LOGD("near %d,far %d, format %d, mime %d", near, far, format, mime);
#endif
        int appendCount = BOKEH_REFOCUS_COMMON_PARAM_NUM;
        HAL_LOGD("append parameter count: %d", appendCount);
        // cpoy common param to tail
        buffer_base += (use_size - BOKEH_REFOCUS_COMMON_PARAM_NUM * 4);
        for (i = 0; i < BOKEH_REFOCUS_COMMON_PARAM_NUM; i++) {
            memcpy(buffer_base + i * 4, p[i], 4);
        }
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        buffer_base -= depth_yuv_normalize_size;
        memcpy(buffer_base, depth_yuv_normalize, depth_yuv_normalize_size);
#endif
        // cpoy depth yuv
        buffer_base -= depth_size;
        memcpy(buffer_base, depth_yuv, depth_size);
    }

#ifdef YUV_CONVERT_TO_JPEG
    // cpoy original jpeg
    ret = mPortrait->map(mPortrait->m_pDstJpegBuffer, (void **)&orig_jpeg_data);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map dst jpeg buffer");
        goto fail_map_dst;
    }
    buffer_base -= orig_jpeg_size;
    memcpy(buffer_base, orig_jpeg_data, orig_jpeg_size);
#else
    // cpoy original yuv
    ret = mPortrait->map(mPortrait->m_pMainSnapBuffer, (void **)&orig_yuv_data);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map snap buffer");
        goto fail_map_dst;
    }
    buffer_base -= orig_yuv_size;
    memcpy(buffer_base, orig_yuv_data, orig_yuv_size);
#endif
    // blob to indicate all image size(use_size)
    mPortrait->setJpegSize((char *)result_buffer_addr, result_buffer_size,
                           use_size);

#ifdef YUV_CONVERT_TO_JPEG
    mPortrait->unmap(mPortrait->m_pDstJpegBuffer);
#else
    mPortrait->unmap(mPortrait->m_pMainSnapBuffer);
#endif

fail_map_dst:
    return ret;
}

/*===========================================================================
 * FUNCTION   :reprocessReq
 *
 * DESCRIPTION: reprocessReq
 *
 * PARAMETERS : buffer_handle_t *output_buffer, capture_queue_msg_t_portrait
 * capture_msg
 *               buffer_handle_t *depth_output_buffer, buffer_handle_t
 * scaled_buffer
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::BokehCaptureThread::reprocessReq(
    buffer_handle_t *output_buffer, capture_queue_msg_t_portrait capture_msg) {
    camera3_capture_request_t request = mSavedCapRequest;
    camera3_stream_buffer_t output_buffers;
    camera3_stream_buffer_t input_buffer;

    memset(&output_buffers, 0x00, sizeof(camera3_stream_buffer_t));
    memset(&input_buffer, 0x00, sizeof(camera3_stream_buffer_t));
    memcpy(&input_buffer, &mSavedCapReqStreamBuff,
           sizeof(camera3_stream_buffer_t));
    if (mSavedCapReqsettings) {
        request.settings = mSavedCapReqsettings;
    } else {
        HAL_LOGE("metadata is null,failed.");
    }
    input_buffer.stream =
        &(mPortrait->mMainStreams[mPortrait->mCaptureStreamsNum]);

    memcpy((void *)&output_buffers, &mSavedCapReqStreamBuff,
           sizeof(camera3_stream_buffer_t));
    output_buffers.stream =
        &(mPortrait->mMainStreams[mPortrait->mCaptureStreamsNum]);
#ifdef BOKEH_YUV_DATA_TRANSFORM
    buffer_handle_t *transform_buffer = NULL;
    if (mPortrait->mBokehSize.transform_w != 0) {
        transform_buffer = (mPortrait->popBufferList(
            mPortrait->mLocalBufferList, SNAPSHOT_TRANSFORM_BUFFER));
        buffer_handle_t *output_handle = NULL;
        void *output_handle_addr = NULL;
        void *transform_buffer_addr = NULL;

        if ((!mAbokehGallery ||
             (mPortrait->mDoPortrait &&
              mPortrait->mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE)) &&
            mBokehResult) {
            output_handle = output_buffer;
        } else {
            output_handle = capture_msg.combo_buff.buffer1;
        }
        int rc = NO_ERROR;
        rc = mPortrait->map(output_handle, &output_handle_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map output buffer");
        }
        rc = mPortrait->map(transform_buffer, &transform_buffer_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map transform buffer");
        }

        int64_t alignTransform = systemTime();
        mPortrait->alignTransform(
            output_handle_addr, mPortrait->mBokehSize.capture_w,
            mPortrait->mBokehSize.capture_h, mPortrait->mBokehSize.transform_w,
            mPortrait->mBokehSize.transform_h, transform_buffer_addr);
        HAL_LOGD("alignTransform run cost %lld ms",
                 ns2ms(systemTime() - alignTransform));

        input_buffer.stream->width = mPortrait->mBokehSize.transform_w;
        input_buffer.stream->height = mPortrait->mBokehSize.transform_h;
        input_buffer.buffer = transform_buffer;
        output_buffers.stream->width = mPortrait->mBokehSize.transform_w;
        output_buffers.stream->height = mPortrait->mBokehSize.transform_h;
        mPortrait->unmap(output_handle);
        mPortrait->unmap(transform_buffer);
    } else {
        input_buffer.stream->width = mPortrait->mBokehSize.capture_w;
        input_buffer.stream->height = mPortrait->mBokehSize.capture_h;
        output_buffers.stream->width = mPortrait->mBokehSize.capture_w;
        output_buffers.stream->height = mPortrait->mBokehSize.capture_h;
        if ((!mAbokehGallery ||
             (mPortrait->mDoPortrait &&
              mPortrait->mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE)) &&
            mBokehResult) {
            input_buffer.buffer = output_buffer;
        } else {
            input_buffer.buffer = capture_msg.combo_buff.buffer1;
        }
    }
#else
    input_buffer.stream->width = mPortrait->mBokehSize.capture_w;
    input_buffer.stream->height = mPortrait->mBokehSize.capture_h;
    output_buffers.stream->width = mPortrait->mBokehSize.capture_w;
    output_buffers.stream->height = mPortrait->mBokehSize.capture_h;
    if ((!mAbokehGallery ||
         (mPortrait->mDoPortrait &&
          mPortrait->mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE)) &&
        mBokehResult) {
        input_buffer.buffer = output_buffer;

    } else {
        input_buffer.buffer = capture_msg.combo_buff.buffer1;
    }

#endif
    // input_buffer.buffer = output_buffer;

    request.num_output_buffers = 1;
    request.frame_number = mPortrait->mCapFrameNumber;
    request.output_buffers = &output_buffers;
    request.input_buffer = &input_buffer;
    mReprocessing = true;
    if (!mAbokehGallery ||
        (mPortrait->mDoPortrait &&
         mPortrait->mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE)) {
        mPortrait->pushBufferList(mPortrait->mLocalBuffer, output_buffer,
                                  mPortrait->mLocalBufferNumber,
                                  mPortrait->mLocalBufferList);
    }
#ifdef BOKEH_YUV_DATA_TRANSFORM
    if (mPortrait->mBokehSize.transform_w != 0) {
        mPortrait->pushBufferList(mPortrait->mLocalBuffer, transform_buffer,
                                  mPortrait->mLocalBufferNumber,
                                  mPortrait->mLocalBufferList);
    }
#endif
    mPortrait->pushBufferList(
        mPortrait->mLocalBuffer, capture_msg.combo_buff.buffer1,
        mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);

    if (!mPortrait->mIsHdrMode) {
        mPortrait->pushBufferList(
            mPortrait->mLocalBuffer, capture_msg.combo_buff.buffer2,
            mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);
    }

    if (mPortrait->mFlushing) {
        mPortrait->CallBackResult(mPortrait->mCapFrameNumber,
                                  CAMERA3_BUFFER_STATUS_ERROR);
        goto exit;
    }
    if (0 > mDevmain->hwi->process_capture_request(mDevmain->dev, &request)) {
        HAL_LOGE("failed. process capture request!");
    }
exit:
    if (NULL != mSavedCapReqsettings) {
        free_camera_metadata(mSavedCapReqsettings);
        mSavedCapReqsettings = NULL;
    }
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
bool SprdCamera3Portrait::BokehCaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    buffer_handle_t *main_buffer = NULL;
    void *src_vir_addr = NULL;
    void *pic_vir_addr = NULL;
    void *pic_jpg_vir_addr = NULL;
    capture_queue_msg_t_portrait capture_msg;
    int rc = 0;
    int mime_type = 0;

    while (!mCaptureMsgList.empty()) {
        {
            Mutex::Autolock l(mMergequeueMutex);
            List<capture_queue_msg_t_portrait>::iterator itor1 =
                mCaptureMsgList.begin();
            capture_msg = *itor1;
            // The specail case is continuous 2 frames data of main camera,
            // but data of aux camera has't come in.
            if (mPortrait->mIsCapDepthFinish == false &&
                capture_msg.combo_buff.buffer1 != NULL &&
                capture_msg.combo_buff.buffer2 == NULL &&
                mPortrait->mIsHdrMode) {
                mPortrait->mHdrSkipBlur = true;
                HAL_LOGI("frame is hdr, and depth hasn't do");
                break;
            }
            mCaptureMsgList.erase(itor1);
        }
        switch (capture_msg.msg_type) {
        case PORTRAIT_MSG_EXIT: {
            // flush queue
            HAL_LOGI("PORTRAIT_MSG_EXIT,mCapFrameNumber=%d",
                     mPortrait->mCapFrameNumber);
            if (mPortrait->mCapFrameNumber) {
                HAL_LOGE("error.HAL don't send capture frame");
                mPortrait->CallBackResult(mPortrait->mCapFrameNumber,
                                          CAMERA3_BUFFER_STATUS_ERROR);
            }
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqStreamBuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            return false;
        }
        case PORTRAIT_MSG_DATA_PROC: {
            void *input_buf1_addr = NULL;
            rc = mPortrait->map(capture_msg.combo_buff.buffer1,
                                &input_buf1_addr);
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to map input buffer1");
                return false;
            }
            mBokehResult = true;
            if (!mAbokehGallery ||
                (mPortrait->mDoPortrait &&
                 mPortrait->mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE)) {
                output_buffer = (mPortrait->popBufferList(
                    mPortrait->mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
            }
            HAL_LOGD("start process normal frame to get "
                     "depth data!");
            if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {
                HAL_LOGD(
                    "mPortrait->mPortraitFlag %d mPortrait->mDoPortrait %d",
                    mPortrait->mPortraitFlag, mPortrait->mDoPortrait);
                if (mPortrait->mDoPortrait &&
                    ((mPortrait->lightPortraitType != 0) ||
                     (mPortrait->mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE) ||
                     (mPortrait->mFaceBeautyFlag))) {
                    rc = sprdDepthCaptureHandle(
                        capture_msg.combo_buff.buffer1, input_buf1_addr,
                        capture_msg.combo_buff.buffer2, output_buffer);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("depthCaptureHandle failed");
                        mBokehResult = false;
                    }
                } else {
                    mBokehResult = false;
                }
                HAL_LOGI("mPortrait->mIsHdrMode=%d,mAbokehGallery=%d,"
                         "mBokehResult=%d",
                         mPortrait->mIsHdrMode, mAbokehGallery, mBokehResult);
            }
            if (mBokehResult == false) {
                mime_type = 0;
            } else if (mAbokehGallery) {
                if (mPortrait->mIsHdrMode) {
                    mime_type = (1 << 8) | (int)SPRD_MIMETPYE_BOKEH_HDR;
                } else {
                    mime_type = (1 << 8) | (int)SPRD_MIMETPYE_BOKEH;
                }
            } else {
                if (mPortrait->mIsHdrMode) {
                    mime_type = (int)SPRD_MIMETPYE_BOKEH_HDR;
                } else {
                    mime_type = (int)SPRD_MIMETPYE_BOKEH;
                }
            }

            if (mPortrait->mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE) {
                mime_type = SPRD_MIMETPYE_NONE;
            } else {
                mime_type = SPRD_MIMETPYE_NONE;
            }
            if (!mPortrait->mIsHdrMode) {
#ifdef YUV_CONVERT_TO_JPEG
                mPortrait->m_pDstJpegBuffer = (mPortrait->popBufferList(
                    mPortrait->mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
                // mPortrait->m_pDstGDepthOriJpegBuffer =
                //  (mPortrait->popBufferList(mPortrait->mLocalBufferList,
                //                          SNAPSHOT_MAIN_BUFFER));
                if (mBokehResult) {
                    rc = mPortrait->map(mPortrait->m_pDstJpegBuffer,
                                        &pic_vir_addr);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("fail to map jpeg buffer");
                    }
                    mPortrait->mOrigJpegSize =
                        mPortrait->jpeg_encode_exif_simplify(
                            capture_msg.combo_buff.buffer1, input_buf1_addr,
                            mPortrait->m_pDstJpegBuffer, pic_vir_addr, NULL,
                            NULL,
                            mPortrait->m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi,
                            SprdCamera3Setting::s_setting[mPortrait->mCameraId]
                                .jpgInfo.orientation);
                    mPortrait->unmap(mPortrait->m_pDstJpegBuffer);
                }
#else
                mPortrait->m_pMainSnapBuffer = capture_msg.combo_buff.buffer1;
#endif
            }

            mPortrait->unmap(capture_msg.combo_buff.buffer1);
            if (!mPortrait->mFlushing)
                mDevmain->hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE,
                                             &mime_type, NULL);
            if (!mPortrait->mIsHdrMode) {
                reprocessReq(output_buffer, capture_msg);
            } else {
                if (!mAbokehGallery) {
                    mPortrait->pushBufferList(mPortrait->mLocalBuffer,
                                              output_buffer,
                                              mPortrait->mLocalBufferNumber,
                                              mPortrait->mLocalBufferList);
                }

                mPortrait->pushBufferList(
                    mPortrait->mLocalBuffer, capture_msg.combo_buff.buffer1,
                    mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);
                mPortrait->pushBufferList(
                    mPortrait->mLocalBuffer, capture_msg.combo_buff.buffer2,
                    mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);
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
void SprdCamera3Portrait::BokehCaptureThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    capture_queue_msg_t_portrait capture_msg;
    bzero(&capture_msg, sizeof(capture_queue_msg_t_portrait));
    capture_msg.msg_type = PORTRAIT_MSG_EXIT;
    mCaptureMsgList.push_front(capture_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::BokehCaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty() || mPortrait->mHdrSkipBlur) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, BOKEH_THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION   :sprdDepthCaptureHandle
 *
 * DESCRIPTION: sprdDepthCaptureHandle
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Portrait::BokehCaptureThread::sprdDepthCaptureHandle(
    buffer_handle_t *input_buf1, void *input_buf1_addr,
    buffer_handle_t *input_buf2, buffer_handle_t *output_buf) {
    void *input_buf2_addr = NULL;
    void *snapshot_gdepth_buffer_addr = NULL;
    void *output_buf_addr = NULL;
    unsigned char *outPortraitMask = NULL;
    int64_t depthRun = 0;
    cmr_uint depth_jepg = 0;
    int rc = NO_ERROR;
    int mBokehMode = 0, lightPortraitType = 0, mFaceBeautyFlag = 0;
    mBokehMode = mPortrait->mBokehMode;
    lightPortraitType = mPortrait->lightPortraitType;
    mFaceBeautyFlag = mPortrait->mFaceBeautyFlag;
    HAL_LOGD("E");
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    if (output_buf == NULL || input_buf1 == NULL || input_buf2 == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }

    rc = mPortrait->map(input_buf2, &input_buf2_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer2");
        goto fail_map_input2;
    }

    rc = mPortrait->map(output_buf, &output_buf_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map output buffer");
        goto fail_map_output;
    }

    property_get("persist.vendor.cam.bokeh.scale", prop, "yuv_sw");
    if (!strcmp(prop, "sw")) {
        mPortrait->swScale(
            (uint8_t *)mPortrait->mScaleInfo.addr_vir.addr_y,
            mPortrait->mBokehSize.depth_snap_main_w,
            mPortrait->mBokehSize.depth_snap_main_h, mPortrait->mScaleInfo.fd,
            (uint8_t *)input_buf1_addr, mPortrait->mBokehSize.capture_w,
            mPortrait->mBokehSize.capture_h, ADP_BUFFD(*input_buf1));

    } else if (!strcmp(prop, "yuv_sw")) {
        mPortrait->Yuv420Scale(
            (uint8_t *)mPortrait->mScaleInfo.addr_vir.addr_y,
            mPortrait->mBokehSize.depth_snap_main_w,
            mPortrait->mBokehSize.depth_snap_main_h, (uint8_t *)input_buf1_addr,
            mPortrait->mBokehSize.capture_w, mPortrait->mBokehSize.capture_h);
    } else if (!strcmp(prop, "hw-k")) {
        HAL_LOGD("scale from kernel");
    } else {
        mPortrait->hwScale(
            (uint8_t *)mPortrait->mScaleInfo.addr_vir.addr_y,
            mPortrait->mBokehSize.depth_snap_main_w,
            mPortrait->mBokehSize.depth_snap_main_h, mPortrait->mScaleInfo.fd,
            (uint8_t *)input_buf1_addr, mPortrait->mBokehSize.capture_w,
            mPortrait->mBokehSize.capture_h, ADP_BUFFD(*input_buf1));
    }
    HAL_LOGD("scale src:%d,%d,dst:%d,%d", mPortrait->mBokehSize.capture_w,
             mPortrait->mBokehSize.capture_h,
             mPortrait->mBokehSize.depth_snap_main_w,
             mPortrait->mBokehSize.depth_snap_main_h);

    /*get portrait mask*/

    outPortraitMask = (unsigned char *)malloc(DEPTH_SNAP_OUTPUT_WIDTH *
                                              DEPTH_SNAP_OUTPUT_HEIGHT * 2);
    if (!outPortraitMask) {
        HAL_LOGE("no mem!");
        return rc;
    }
    if (mPortrait->mFlushing) {
        rc = BAD_VALUE;
        goto exit;
    }
    sem_wait(&mPortrait->mFaceinfoSignSem);
    mPortrait->mBokehAlgo->getPortraitMask(output_buf_addr, input_buf1_addr,
                                           mPortrait->mVcmStepsFixed,
                                           outPortraitMask);
    if (mPortrait->mFlushing) {
        rc = BAD_VALUE;
        goto exit;
    }

    if (!outPortraitMask) {
        HAL_LOGE("no thing!");
    }
    /*do portrait*/
    if (mBokehMode == CAM_PORTRAIT_PORTRAIT_MODE) {
        if (mPortrait->mPortraitFlag) {
            depthRun = systemTime();
            HAL_LOGI("mLastOnlieVcm = %d, mVcmSteps = %d",
                     mPortrait->mLastOnlieVcm, mPortrait->mVcmSteps);
            if (mPortrait->mLastOnlieVcm &&
                mPortrait->mLastOnlieVcm == mPortrait->mVcmSteps) {
                rc = mPortrait->mBokehAlgo->capPortraitDepthRun(
                    mPortrait->mDepthBuffer.snap_depth_buffer,
                    mPortrait->mDepthBuffer.depth_out_map_table,
                    input_buf2_addr,
                    (void *)mPortrait->mScaleInfo.addr_vir.addr_y,
                    input_buf1_addr, output_buf_addr, mPortrait->mVcmStepsFixed,
                    mPortrait->mlimited_infi, mPortrait->mlimited_macro);
            } else {
                rc = mPortrait->mBokehAlgo->capPortraitDepthRun(
                    mPortrait->mDepthBuffer.snap_depth_buffer, NULL,
                    input_buf2_addr,
                    (void *)mPortrait->mScaleInfo.addr_vir.addr_y,
                    input_buf1_addr, output_buf_addr, mPortrait->mVcmStepsFixed,
                    mPortrait->mlimited_infi, mPortrait->mlimited_macro);
                HAL_LOGD("close online depth");
            }
            // memcpy(output_buf_addr,input_buf1_addr, ADP_BUFSIZE(*input_buf1));
            if (rc != ALRNB_ERR_SUCCESS) {
                HAL_LOGE("sprd_depth_Run failed! %d", rc);
                goto exit;
            }
            HAL_LOGD("depth run cost %lld ms", ns2ms(systemTime() - depthRun));
        } else {
            HAL_LOGE("otp.data null");
        }
    }
    if (mPortrait->mFlushing) {
        rc = BAD_VALUE;
        goto exit;
    }
#ifdef CONFIG_FACE_BEAUTY
    /*do face beauty*/
    if (mFaceBeautyFlag) {
        if (mBokehMode == CAM_COMMON_MODE) {
            HAL_LOGD("feature support:no portrait+fb");
            memcpy(output_buf_addr, input_buf1_addr, ADP_BUFSIZE(*input_buf1));
        } else {
            HAL_LOGD("feature support: portrait+fb");
        }
        rc = mPortrait->mBokehAlgo->doFaceBeauty(
            outPortraitMask, output_buf_addr, mPortrait->mBokehSize.capture_w,
            mPortrait->mBokehSize.capture_h, 1, &mPortrait->facebeautylevel);
    }
#endif

    /*do lpt*/
    if (lightPortraitType != 0) {
        if (mBokehMode == CAM_COMMON_MODE && (!mFaceBeautyFlag)) {
            HAL_LOGD("feature support:lpt");
            memcpy(output_buf_addr, input_buf1_addr, ADP_BUFSIZE(*input_buf1));
        } else {
            HAL_LOGD("feature support:portrait + lpt");
        }

        rc = mPortrait->mBokehAlgo->capLPT(output_buf_addr,
                                           mPortrait->mBokehSize.capture_w,
                                           mPortrait->mBokehSize.capture_h,
                                           outPortraitMask, lightPortraitType);
        if (rc != ALRNB_ERR_SUCCESS) {
            HAL_LOGE("capLPT failed! %d", rc);
            goto exit;
        }
    }


#if defined(CONFIG_SUPPORT_GDEPTH) ||                                          \
    defined(CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV)
    memset(&mGDepthOutputParam, 0, sizeof(gdepth_outparam));
    mGDepthOutputParam.confidence_map =
        mPortrait->mDepthBuffer.snap_depthConfidence_buffer;
    mGDepthOutputParam.depthnorm_data =
        mPortrait->mDepthBuffer.depth_normalize_data_buffer;
    mPortrait->mBokehAlgo->getGDepthInfo(
        mPortrait->mDepthBuffer.snap_depth_buffer, &mGDepthOutputParam);
    mPortrait->far = mGDepthOutputParam.far;
    mPortrait->near = mGDepthOutputParam.near;
#endif

    /*mPortrait->DepthRangLinear(
        (uint8_t *)snapshot_gdepth_buffer_addr,
        (uint16_t *)(mPortrait->mDepthBuffer.snap_depth_buffer),
        mPortrait->mBokehSize.depth_snap_out_w,
        mPortrait->mBokehSize.depth_snap_out_h, &mPortrait->far,
        &mPortrait->near);*/
    mPortrait->mIsCapDepthFinish = true;

exit : { // dump yuv data

    property_get("persist.vendor.cam.bokeh.dump", prop, "0");
    if (!strcmp(prop, "capdepth") || !strcmp(prop, "all")) {
        // input_buf1 or left image
        mPortrait->dumpData(
            (unsigned char *)input_buf1_addr, 1, ADP_BUFSIZE(*input_buf1),
            mPortrait->mBokehSize.capture_w, mPortrait->mBokehSize.capture_h,
            mPortrait->mCapFrameNumber, "Main");
        // input_buf2 or right image
        mPortrait->dumpData((unsigned char *)input_buf2_addr, 1,
                            ADP_BUFSIZE(*input_buf2),
                            mPortrait->mBokehSize.depth_snap_sub_w,
                            mPortrait->mBokehSize.depth_snap_sub_h,
                            mPortrait->mCapFrameNumber, "Sub");
        mPortrait->dumpData((uint8_t *)mPortrait->mScaleInfo.addr_vir.addr_y, 1,
                            mPortrait->mScaleInfo.buf_size,
                            mPortrait->mBokehSize.depth_snap_main_w,
                            mPortrait->mBokehSize.depth_snap_main_h,
                            mPortrait->mCapFrameNumber, "MainScale");
        mPortrait->dumpData(
            (unsigned char *)(mPortrait->mDepthBuffer.snap_depth_buffer), 5,
            mPortrait->mBokehSize.depth_snap_size,
            mPortrait->mBokehSize.depth_snap_out_w,
            mPortrait->mBokehSize.depth_snap_out_h, mPortrait->mCapFrameNumber,
            "depth");
        mPortrait->dumpData(
            (unsigned char *)output_buf_addr, 1, ADP_BUFSIZE(*output_buf),
            mPortrait->mBokehSize.capture_w, mPortrait->mBokehSize.capture_h,
            mPortrait->mCapFrameNumber, "output_buf_addr");
    }
}
    HAL_LOGI(":X");
    free(outPortraitMask);
    mPortrait->unmap(output_buf);
fail_map_output:
    mPortrait->unmap(input_buf2);
fail_map_input2:

    return rc;
}

/*===========================================================================
 * FUNCTION   :updateApiParams
 *
 * DESCRIPTION: update bokeh and depth api Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3Portrait::updateApiParams(CameraMetadata metaSettings, int type,
                                          uint32_t cur_frame_number) {
    // always get f_num in request
    int32_t origW =
        SprdCamera3Setting::s_setting[0].sensor_InfoInfo.pixer_array_size[0];
    int32_t origH =
        SprdCamera3Setting::s_setting[0].sensor_InfoInfo.pixer_array_size[1];
    // get face info
    uint8_t phyId = 0;
    bool isUpdate = false;
    phyId = m_pPhyCamera[CAM_TYPE_MAIN].id;
    int32_t faceInfo[4];
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;
    FACE_Tag *faceDetectionInfo =
        (FACE_Tag *)&(hwiMain->mSetting->s_setting[phyId].faceInfo);
    uint8_t numFaces = faceDetectionInfo->face_num;
    int32_t faceRectangles[CAMERA3MAXFACE * 4];
    MRECT face_rect[CAMERA3MAXFACE];
    int32_t fd_score[10] = {0};
    for (int i = 0; i < numFaces; i++) {
        *(fd_score + i) = hwiMain->mSetting->s_setting[phyId].fd_score[i];
    }

    int j = 0;
    int max = 0;
    int max_index = 0;
    mBokehAlgo->setFaceInfo(faceDetectionInfo->angle, faceDetectionInfo->pose,
                            fd_score);

    for (int i = 0; i < numFaces; i++) {
        convertToRegions(faceDetectionInfo->face[i].rect, faceRectangles + j,
                         -1);
        face_rect[i].left = faceRectangles[0 + j];
        face_rect[i].top = faceRectangles[1 + j];
        face_rect[i].right = faceRectangles[2 + j];
        face_rect[i].bottom = faceRectangles[3 + j];
        j += 4;
        if (face_rect[i].right - face_rect[i].left > max) {
            max_index = i;
            max = face_rect[i].right - face_rect[i].left;
        }
    }

    if (metaSettings.exists(ANDROID_SPRD_VCM_STEP_FOR_BOKEH)) {
        if (mbokehParm.vcm !=
            metaSettings.find(ANDROID_SPRD_VCM_STEP_FOR_BOKEH).data.i32[0]) {
            mbokehParm.vcm =
                metaSettings.find(ANDROID_SPRD_VCM_STEP_FOR_BOKEH).data.i32[0];
            isUpdate = true;
        }
    }
    // face

    if (metaSettings.exists(ANDROID_SPRD_DEVICE_ORIENTATION)) {
        mbokehParm.portrait_param.mobile_angle =
            metaSettings.find(ANDROID_SPRD_DEVICE_ORIENTATION).data.i32[0];
        if (mbokehParm.portrait_param.mobile_angle == 0) {
            mbokehParm.portrait_param.mobile_angle = 180;
        } else if (mbokehParm.portrait_param.mobile_angle == 180) {
            mbokehParm.portrait_param.mobile_angle = 0;
        }
    }
    mbokehParm.portrait_param.mRotation =
        mJpegOrientation; // mbokehParm.portrait_param.mobile_angle;

    mbokehParm.portrait_param.camera_angle =
        SprdCamera3Setting::s_setting[mPortrait->mCameraId]
            .sensorInfo.orientation;

    if (!mIsCapturing) {
        j = 0;
        for (int i = 0; i < numFaces; i++) {
            convertToRegions(faceDetectionInfo->face[i].rect,
                             faceRectangles + j, -1);
            faceInfo[0] = faceRectangles[0 + j];
            faceInfo[1] = faceRectangles[1 + j];
            faceInfo[2] = faceRectangles[2 + j];
            faceInfo[3] = faceRectangles[3 + j];
            j += 4;
            if (mbokehParm.portrait_param.mobile_angle == 0) {
                mbokehParm.portrait_param.x1[i] =
                    faceInfo[2] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[1] * mBokehSize.depth_snap_out_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[0] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[3] * mBokehSize.depth_snap_out_h / origH;
            } else if (mbokehParm.portrait_param.mobile_angle == 90) {
                mbokehParm.portrait_param.x1[i] =
                    faceInfo[2] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[3] * mBokehSize.depth_snap_out_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[0] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[1] * mBokehSize.depth_snap_out_h / origH;
            } else if (mbokehParm.portrait_param.mobile_angle == 180) {
                mbokehParm.portrait_param.x1[i] =
                    faceInfo[0] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[3] * mBokehSize.depth_snap_out_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[2] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[1] * mBokehSize.depth_snap_out_h / origH;
            } else {
                mbokehParm.portrait_param.x1[i] =
                    faceInfo[0] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[1] * mBokehSize.depth_snap_out_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[2] * mBokehSize.depth_snap_out_w / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[3] * mBokehSize.depth_snap_out_h / origH;
            }
            mbokehParm.portrait_param.flag[i] = 0;
            mbokehParm.portrait_param.flag[i + 1] = 1;
        }
        mbokehParm.portrait_param.valid_roi = numFaces;
    }
    HAL_LOGV("face_info %d %d %d %d", mbokehParm.portrait_param.x1[0],
             mbokehParm.portrait_param.y1[0], mbokehParm.portrait_param.x2[0],
             mbokehParm.portrait_param.y2[0]);

    mPrevPortrait = numFaces;

    if (mbokehParm.portrait_param.valid_roi) {
        mDoPortrait = 1;
    } else {
        mDoPortrait = 0;
    }
    HAL_LOGD(".mDoPortrait%d,numFaces=%d,mBokehMode=%d", mDoPortrait, numFaces,
             mBokehMode);
    if (mPortrait->mCameraId == 0)
        mbokehParm.portrait_param.rear_cam_en = true;
    else
        mbokehParm.portrait_param.rear_cam_en = 0;

    mbokehParm.portrait_param.face_num = numFaces;
    mbokehParm.portrait_param.portrait_en = mBokehMode;
    isUpdate = true;
    //
    if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
        int fnum = metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
        if (fnum < MIN_F_FUMBER) {
            fnum = MIN_F_FUMBER;
        } else if (fnum > MAX_F_FUMBER) {
            fnum = MAX_F_FUMBER;
        }
        if (fnum != mbokehParm.f_number) {
            mbokehParm.f_number = fnum;
            mBokehAlgo->setBokenParam((void *)&mbokehParm);
            mDepthTrigger = TRIGGER_FNUM_PORTRAIT;
        }
    }
    if (metaSettings.exists(ANDROID_SPRD_AF_ROI)) {
        int left = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[0];
        int top = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[1];
        int right = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[2];
        int bottom = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[3];
        mAfstate = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[4];
        int x = left, y = top;
        if (left != 0 && top != 0 && right != 0 && bottom != 0) {
            x = left + (right - left) / 2;
            y = top + (bottom - top) / 2;
            x = x * mBokehSize.preview_w / origW;
            y = y * mBokehSize.preview_h / origH;
            if (x != mbokehParm.sel_x || y != mbokehParm.sel_y) {
                mbokehParm.sel_x = x;
                mbokehParm.sel_y = y;
                isUpdate = true;
                HAL_LOGD("sel_x %d ,sel_y %d", x, y);
            }
        }
        mbokehParm.cur_frame_number = cur_frame_number;
        mCapFaceInfoList.push_back(mbokehParm);
        if (mSnapshotResultReturn && type == 1) {
            bokeh_params capfaceinfo;
            memset(&capfaceinfo, 0, sizeof(bokeh_params));
            if (!mCapFaceInfoList.empty()) {
                Mutex::Autolock m(mPrevFrameNotifyLock);
                int64_t timestampMIN = capture_result_timestamp;
                uint32_t prev_frame_number = 0;
                for (List<camera3_notify_msg_t>::iterator i =
                         mPrevFrameNotifyList.begin();
                     i != mPrevFrameNotifyList.end(); i++) {
                    int64_t timestampTEMP =
                        ABS((int64_t)((uint64_t)i->message.shutter.timestamp -
                                      (uint64_t)capture_result_timestamp));
                    if (timestampTEMP < timestampMIN) {
                        timestampMIN = timestampTEMP;
                        prev_frame_number = i->message.shutter.frame_number;
                    }
                    if (timestampMIN == 0) {
                        break;
                    }
                }

                for (List<bokeh_params>::iterator i = mCapFaceInfoList.begin();
                     i != mCapFaceInfoList.end(); i++) {
                    if (i->cur_frame_number == prev_frame_number) {
                        capfaceinfo.sel_x = i->sel_x;
                        capfaceinfo.sel_y = i->sel_y;
                        capfaceinfo.f_number = i->f_number;
                        capfaceinfo.relbokeh_oem_data = i->relbokeh_oem_data;
                        capfaceinfo.portrait_param = i->portrait_param;
                        break;
                    }
                }
            }

            capfaceinfo.portrait_param.mRotation = mJpegOrientation;
            mBokehAlgo->setCapFaceParam((void *)&capfaceinfo);
            sem_post(&mFaceinfoSignSem);
            mSnapshotResultReturn = false;
        }

        if (mCapFaceInfoList.size() > BOKEH_PREVIEW_PARAM_LIST) {
            mCapFaceInfoList.erase(mCapFaceInfoList.begin());
        }
    }
#ifdef CONFIG_FACE_BEAUTY
    if (metaSettings.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
        mFaceInfo[0] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[0];
        mFaceInfo[1] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[1];
        mFaceInfo[2] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[2];
        mFaceInfo[3] =
            metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[3];
    }
#endif
    if (mPrevPortrait) {
        int x = face_rect[max_index].left +
                (face_rect[max_index].right - face_rect[max_index].left) / 2;
        int y = face_rect[max_index].top +
                (face_rect[max_index].bottom - face_rect[max_index].top) / 2;

        mbokehParm.sel_x = x * mBokehSize.preview_w / origW;
        mbokehParm.sel_y = y * mBokehSize.preview_h / origH;
        HAL_LOGD("update sel_x %d ,sel_y %d", mbokehParm.sel_x,
                 mbokehParm.sel_y);
    }

    if (isUpdate) {
        mBokehAlgo->setBokenParam((void *)&mbokehParm);
    }

    int cameraBV = 0, cameraISO = 0, cameraCT = 0;
    if (metaSettings.exists(ANDROID_SPRD_LIGHTPORTRAITTYPE)) {
        mPortrait->lightPortraitType =
            metaSettings.find(ANDROID_SPRD_LIGHTPORTRAITTYPE).data.i32[0];
    }

    int rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_BV, &cameraBV, NULL);
    rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_ISO, &cameraISO, NULL);
    rc = hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_CT, &cameraCT, NULL);
    mBokehAlgo->setLightPortraitParam(cameraBV, cameraISO, cameraCT,
                                      mPortrait->lightPortraitType);
    HAL_LOGV("cameraBV %d;cameraISO %d;cameraCT %d;lightPortraitType %d",
             cameraBV, cameraISO, cameraCT, mPortrait->lightPortraitType);
}

#ifdef CONFIG_FACE_BEAUTY

/*===========================================================================
 * FUNCTION   :cap_3d_doFaceMakeup
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::bokehFaceMakeup(buffer_handle_t *buffer_handle,
                                          void *input_buf1_addr) {
    struct camera_frame_type cap_3d_frame;
    struct camera_frame_type *frame = NULL;
    int faceInfo[4];
    FACE_Tag newFace;
    bzero(&cap_3d_frame, sizeof(struct camera_frame_type));
    frame = &cap_3d_frame;
    frame->y_vir_addr = (cmr_uint)input_buf1_addr;
    frame->width = ADP_WIDTH(*buffer_handle);
    frame->height = ADP_HEIGHT(*buffer_handle);

    faceInfo[0] = mFaceInfo[0] * mBokehSize.capture_w / mBokehSize.preview_w;
    faceInfo[1] = mFaceInfo[1] * mBokehSize.capture_w / mBokehSize.preview_w;
    faceInfo[2] = mFaceInfo[2] * mBokehSize.capture_w / mBokehSize.preview_w;
    faceInfo[3] = mFaceInfo[3] * mBokehSize.capture_w / mBokehSize.preview_w;

    newFace.face_num =
        SprdCamera3Setting::s_setting[mPortrait->mCameraId].faceInfo.face_num;
    newFace.face[0].rect[0] = faceInfo[0];
    newFace.face[0].rect[1] = faceInfo[1];
    newFace.face[0].rect[2] = faceInfo[2];
    newFace.face[0].rect[3] = faceInfo[3];
    // mPortrait->doFaceMakeup2(frame, mPortrait->mPerfectskinlevel, &newFace,
    //                         0); // work mode 1 for preview, 0 for picture
}
#endif

/*======================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump fd
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Portrait::dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");

    CHECK_CAPTURE();

    mPortrait->_dump(device, fd);

    HAL_LOGI("X");
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
int SprdCamera3Portrait::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();

    rc = mPortrait->_flush(device);

    HAL_LOGI(" X");

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
int SprdCamera3Portrait::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    HAL_LOGI("E");
    mPendingRequest = 0;
    mBokehSize.capture_w = 0;
    mBokehSize.capture_h = 0;
    mPreviewStreamsNum = 0;
    mCaptureStreamsNum = 1;
    mCaptureThread->mReprocessing = false;
    mIsCapturing = false;
    mSnapshotResultReturn = false;
    mCapFrameNumber = 0;
    mPrevBlurFrameNumber = 0;
    mjpegSize = 0;
    mFlushing = false;
    mCallbackOps = callback_ops;
    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    mPrevFrameNotifyList.clear();
    mOtpData.otp_exist = false;
    mPortraitFlag = false;
    mVcmSteps = 0;
    mOtpData.otp_size = 0;
    mOtpData.otp_type = 0;
    mJpegOrientation = 0;
    mIsHdrMode = false;
    mIsCapDepthFinish = false;
    mHdrSkipBlur = false;
    mHdrCallbackCnt = 0;
#ifdef YUV_CONVERT_TO_JPEG
    mOrigJpegSize = 0;
    m_pDstJpegBuffer = NULL;
    m_pDstGDepthOriJpegBuffer = NULL;
#else
    m_pMainSnapBuffer = NULL;
#endif
    mlimited_infi = 0;
    mlimited_macro = 0;
    memset(&mbokehParm, 0, sizeof(bokeh_params));

    sprdcamera_physical_descriptor_t sprdCam =
        m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiMain);
    SprdCamera3MultiBase::initialize(MODE_BOKEH, hwiMain);

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error aux camera while initialize !! ");
        return rc;
    }
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * PORTRAIT__MAX_NUM_STREAMS);
    memset(mMainStreams, 0,
           sizeof(camera3_stream_t) * PORTRAIT__MAX_NUM_STREAMS);
    memset(&mCaptureThread->mSavedCapRequest, 0,
           sizeof(camera3_capture_request_t));
    memset(&mCaptureThread->mSavedCapReqStreamBuff, 0,
           sizeof(camera3_stream_buffer_t));
    mCaptureThread->mSavedCapReqsettings = NULL;
    mCaptureThread->mSavedResultBuff = NULL;
    mCaptureThread->mSavedOneResultBuff = NULL;

    mCaptureThread->mCallbackOps = callback_ops;
    mCaptureThread->mDevmain = &m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN];

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
int SprdCamera3Portrait::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");
    int rc = 0;
    size_t i = 0;
    size_t j = 0;
    camera3_stream_t *pmainStreams[PORTRAIT__MAX_NUM_STREAMS];
    camera3_stream_t *pauxStreams[PORTRAIT__MAX_NUM_STREAMS];
    camera3_stream_t *newStream = NULL;
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *snapStream = NULL;
    camera3_stream_t *thumbStream = NULL;
    mBokehSize.depth_prev_out_w = DEPTH_OUTPUT_WIDTH;
    mBokehSize.depth_prev_out_h = DEPTH_OUTPUT_HEIGHT;
    mBokehSize.depth_snap_out_w = DEPTH_SNAP_OUTPUT_WIDTH;
    mBokehSize.depth_snap_out_h = DEPTH_SNAP_OUTPUT_HEIGHT;
    far = 0;
    near = 0;
    mBokehSize.depth_jepg_size = 0;
    mFlushing = false;
    mhasCallbackStream = false;
    mReqTimestamp = 0;
    mLastOnlieVcm = 0;
    mIsCapDepthFinish = false;
    mHdrSkipBlur = false;
    struct af_relbokeh_oem_data af_relbokeh_info;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    mPreviewMuxerThread->mPreviewMuxerMsgList.clear();
    mCaptureThread->mCaptureMsgList.clear();
    mDepthMuxerThread->mDepthMuxerMsgList.clear();
    mAfstate = 0;
    mDoPortrait = 0;
    mPrevPortrait = false;
    mbokehParm.f_number = 0;
    sem_init(&mFaceinfoSignSem, 0, 0);
    memset(pmainStreams, 0,
           sizeof(camera3_stream_t *) * PORTRAIT__MAX_NUM_STREAMS);
    memset(pauxStreams, 0,
           sizeof(camera3_stream_t *) * PORTRAIT__MAX_NUM_STREAMS);
    bzero(&af_relbokeh_info, sizeof(struct af_relbokeh_oem_data));

    Mutex::Autolock l(mLock);

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGD("configurestreams, org streamtype:%d",
                 stream_list->streams[i]->stream_type);
        if (requestStreamType == PREVIEW_STREAM) {
            previewStream = stream_list->streams[i];

            mBokehSize.preview_w = stream_list->streams[i]->width;
            mBokehSize.preview_h = stream_list->streams[i]->height;
            getDepthImageSize(mBokehSize.preview_w, mBokehSize.preview_h,
                              &mBokehSize.depth_prev_sub_w,
                              &mBokehSize.depth_prev_sub_h, requestStreamType);

            mMainStreams[mPreviewStreamsNum] = *stream_list->streams[i];
            mAuxStreams[mPreviewStreamsNum] = *stream_list->streams[i];
            (mAuxStreams[mPreviewStreamsNum]).width =
                mBokehSize.depth_prev_sub_w;
            (mAuxStreams[mPreviewStreamsNum]).height =
                mBokehSize.depth_prev_sub_h;
            pmainStreams[i] = &mMainStreams[mPreviewStreamsNum];
            pauxStreams[i] = &mAuxStreams[mPreviewStreamsNum];

        } else if (requestStreamType == SNAPSHOT_STREAM) {
            snapStream = stream_list->streams[i];
            mBokehSize.capture_w = stream_list->streams[i]->width;
            mBokehSize.capture_h = stream_list->streams[i]->height;

#ifdef BOKEH_YUV_DATA_TRANSFORM
            // workaround jpeg cant handle 16-noalign issue, when jpeg fix
            // this
            // issue, we will remove these code
            if (mBokehSize.capture_h == 1944 && mBokehSize.capture_w == 2592) {
                mBokehSize.transform_w = 2592;
                mBokehSize.transform_h = 1952;
            } else if (mBokehSize.capture_h == 1836 &&
                       mBokehSize.capture_w == 3264) {
                mBokehSize.transform_w = 3264;
                mBokehSize.transform_h = 1840;
            } else if (mBokehSize.capture_h == 360 &&
                       mBokehSize.capture_w == 640) {
                mBokehSize.transform_w = 640;
                mBokehSize.transform_h = 368;
            } else if (mBokehSize.capture_h == 1080 &&
                       mBokehSize.capture_w == 1920) {
                mBokehSize.transform_w = 1920;
                mBokehSize.transform_h = 1088;
            } else {
                mBokehSize.transform_w = 0;
                mBokehSize.transform_h = 0;
            }
#endif
            mBokehSize.callback_w = mBokehSize.capture_w;
            mBokehSize.callback_h = mBokehSize.capture_h;
            getDepthImageSize(mBokehSize.capture_w, mBokehSize.capture_h,
                              &mBokehSize.depth_snap_sub_w,
                              &mBokehSize.depth_snap_sub_h, requestStreamType);
            mMainStreams[mCaptureStreamsNum] = *stream_list->streams[i];
            mAuxStreams[mCaptureStreamsNum] = *stream_list->streams[i];
            (mAuxStreams[mCaptureStreamsNum]).width =
                mBokehSize.depth_snap_sub_w;
            (mAuxStreams[mCaptureStreamsNum]).height =
                mBokehSize.depth_snap_sub_h;

            pmainStreams[mCaptureStreamsNum] =
                &mMainStreams[mCaptureStreamsNum];
            mCallbackStreamsNum = PORTRAIT__MAX_NUM_STREAMS - 1;
            mMainStreams[mCallbackStreamsNum].max_buffers = 1;
            mMainStreams[mCallbackStreamsNum].width = mBokehSize.capture_w;
            mMainStreams[mCallbackStreamsNum].height = mBokehSize.capture_h;
            mMainStreams[mCallbackStreamsNum].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mMainStreams[mCallbackStreamsNum].usage =
                GRALLOC_USAGE_SW_READ_OFTEN;
            mMainStreams[mCallbackStreamsNum].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mMainStreams[mCallbackStreamsNum].data_space =
                stream_list->streams[i]->data_space;
            mMainStreams[mCallbackStreamsNum].rotation =
                stream_list->streams[i]->rotation;
            pmainStreams[mCallbackStreamsNum] =
                &mMainStreams[mCallbackStreamsNum];

            mAuxStreams[mCallbackStreamsNum].max_buffers = 1;
            mAuxStreams[mCallbackStreamsNum].width =
                mBokehSize.depth_snap_sub_w;
            mAuxStreams[mCallbackStreamsNum].height =
                mBokehSize.depth_snap_sub_h;
            mAuxStreams[mCallbackStreamsNum].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mAuxStreams[mCallbackStreamsNum].usage =
                GRALLOC_USAGE_SW_READ_OFTEN;
            mAuxStreams[mCallbackStreamsNum].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mAuxStreams[mCallbackStreamsNum].data_space =
                stream_list->streams[i]->data_space;
            mAuxStreams[mCallbackStreamsNum].rotation =
                stream_list->streams[i]->rotation;
            pauxStreams[mCaptureStreamsNum] = &mAuxStreams[mCallbackStreamsNum];
        } else if (requestStreamType == DEFAULT_STREAM) {
            mhasCallbackStream = true;
            (stream_list->streams[i])->max_buffers = 1;
        }
    }
    mBokehSize.depth_prev_scale_size =
        mBokehSize.depth_prev_out_w * mBokehSize.depth_prev_out_h * sizeof(int);
    mBokehSize.depth_prev_size =
        mBokehSize.preview_w * mBokehSize.preview_h * 2;
    mBokehSize.depth_snap_size =
        mBokehSize.depth_snap_out_w * mBokehSize.depth_snap_out_h * 2;
    mBokehSize.depth_weight_map_size =
        mBokehSize.depth_snap_out_w * mBokehSize.depth_snap_out_h * sizeof(int);
    mBokehSize.depth_yuv_normalize_size = mBokehSize.depth_snap_out_w *
                                          mBokehSize.depth_snap_out_h *
                                          sizeof(uint8_t);
    mBokehSize.depth_confidence_map_size = mBokehSize.depth_snap_out_w *
                                           mBokehSize.depth_snap_out_h *
                                           sizeof(uint8_t);
    rc = allocateBuff();
    if (rc < 0) {
        HAL_LOGE("failed to allocateBuff.");
    }
    intDepthPrevBufferFlag();

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = PORTRAIT__MAX_NUM_STREAMS;
    mainconfig.streams = pmainStreams;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;

    camera3_stream_configuration auxconfig;
    auxconfig = *stream_list;
    auxconfig.num_streams = PORTRAIT__MAX_NUM_STREAMS - 1;
    auxconfig.streams = pauxStreams;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].hwi;

    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].dev,
                                   &auxconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }

    if (previewStream != NULL) {
        memcpy(previewStream, &mMainStreams[mPreviewStreamsNum],
               sizeof(camera3_stream_t));
        HAL_LOGV("previewStream  max buffers %d", previewStream->max_buffers);
        previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
    }
    if (snapStream != NULL) {
        memcpy(snapStream, &mMainStreams[mCaptureStreamsNum],
               sizeof(camera3_stream_t));
        snapStream->width = mBokehSize.capture_w;
        snapStream->height = mBokehSize.capture_h;
    }
    mCaptureThread->run(String8::format("Bokeh-Cap").string());
    mPreviewMuxerThread->run(String8::format("Bokeh-Prev").string());
    if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {
        mDepthMuxerThread->run(String8::format("depth-prev").string());
    }

    mbokehParm.sel_x = mBokehSize.preview_w / 2;
    mbokehParm.sel_y = mBokehSize.preview_h / 2;
    hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_REBOKE_DATA, &af_relbokeh_info,
                           NULL);
    memcpy(&mbokehParm.relbokeh_oem_data, &af_relbokeh_info,
           sizeof(af_relbokeh_oem_data));

    rc = mBokehAlgo->initParam(&mBokehSize, &mOtpData,
                               mCaptureThread->mAbokehGallery);

    if (mPortraitFlag) {
        rc = mBokehAlgo->initPortraitParams(&mBokehSize, &mOtpData,
                                            mCaptureThread->mAbokehGallery);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to initPortraitParams");
            mPortraitFlag = false;
            rc = NO_ERROR;
        }
    }

    rc = mBokehAlgo->initPortraitLightParams();
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to initPortraitLightParams");
        rc = NO_ERROR;
    }
#ifdef CONFIG_FACE_BEAUTY
    rc = mBokehAlgo->initFaceBeautyParams();
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to initFaceBeautyParams");
        rc = NO_ERROR;
    }
#endif
    if (mPortraitFlag) {
        property_get("persist.vendor.cam.pbokeh.enable", prop, "1");
        mIsSupportPBokeh = atoi(prop);
        HAL_LOGD("mIsSupportPBokeh prop %d", mIsSupportPBokeh);
    } else {
        mIsSupportPBokeh = false;
        HAL_LOGD("mIsSupportPBokeh %d", mIsSupportPBokeh);
    }
    if (mPreviewMuxerThread->isRunning()) {
        mPreviewMuxerThread->requestInit();
    }
    HAL_LOGI("x rc%d.", rc);
    for (i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD(
            "main configurestreams, streamtype:%d, format:%d, width:%d, "
            "height:%d %p",
            stream_list->streams[i]->stream_type,
            stream_list->streams[i]->format, stream_list->streams[i]->width,
            stream_list->streams[i]->height, stream_list->streams[i]->priv);
    }
    HAL_LOGI("mum_streams:%d,%d,w,h:(%d,%d)(%d,%d)(%d,%d)(%d,%d)),"
             "mIsSupportPBokeh:%d,",
             mainconfig.num_streams, auxconfig.num_streams,
             mBokehSize.depth_snap_sub_w, mBokehSize.depth_snap_sub_h,
             mBokehSize.capture_w, mBokehSize.capture_h,
             mBokehSize.depth_prev_sub_w, mBokehSize.depth_prev_sub_h,
             mBokehSize.preview_w, mBokehSize.preview_h, mIsSupportPBokeh);

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
const camera_metadata_t *SprdCamera3Portrait::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;
    SprdCamera3HWI *hw_depth = m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].hwi;
    Mutex::Autolock l(mLock);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw_depth->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].dev, type);
    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].dev, type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }
    CameraMetadata metadata;
    metadata = fwk_metadata;
    if (mOtpData.otp_exist == false && metadata.exists(ANDROID_SPRD_OTP_DATA)) {
        uint8_t otpType;
        int otpSize;
        otpType = SprdCamera3Setting::s_setting[mPortrait->mCameraId]
                      .otpInfo.otp_type;
        otpSize = SprdCamera3Setting::s_setting[mPortrait->mCameraId]
                      .otpInfo.otp_size;
        HAL_LOGI("otpType %d, otpSize %d", otpType, otpSize);
        mOtpData.otp_exist = true;
        mPortraitFlag = true;
        mOtpData.otp_type = otpType;
        mOtpData.otp_size = otpSize;
        memcpy(mOtpData.otp_data, metadata.find(ANDROID_SPRD_OTP_DATA).data.u8,
               otpSize);
    }
    HAL_LOGD("mOtpData.otp_exist %d, mPortraitFlag %d", mOtpData.otp_exist,
             mPortraitFlag);
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
void SprdCamera3Portrait::saveRequest(camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    multi_request_saved_t currRequest;
    Mutex::Autolock l(mRequestLock);
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        newStream->reserved[0] = NULL;
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.preview_stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            currRequest.frame_number = request->frame_number;
            HAL_LOGD("save request:id %d, ", request->frame_number);
            mSavedRequestList.push_back(currRequest);
        } else if (getStreamType(newStream) == DEFAULT_STREAM) {
            mThumbReq.buffer = request->output_buffers[i].buffer;
            mThumbReq.preview_stream = request->output_buffers[i].stream;
            mThumbReq.input_buffer = request->input_buffer;
            mThumbReq.frame_number = request->frame_number;
            HAL_LOGD("save thumb request:id %d, w= %d,h=%d",
                     request->frame_number, (mThumbReq.preview_stream)->width,
                     (mThumbReq.preview_stream)->height);
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
int SprdCamera3Portrait::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].hwi;
    CameraMetadata metaSettingsMain, metaSettingsAux;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    buffer_handle_t *new_buffer = NULL;
    camera3_stream_buffer_t out_streams_main[PORTRAIT__MAX_NUM_STREAMS];
    camera3_stream_buffer_t out_streams_aux[PORTRAIT__MAX_NUM_STREAMS];
    bool is_captureing = false;
    uint32_t tagCnt = 0;
    camera3_stream_t *preview_stream = NULL;
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };
    bzero(out_streams_main,
          sizeof(camera3_stream_buffer_t) * PORTRAIT__MAX_NUM_STREAMS);
    bzero(out_streams_aux,
          sizeof(camera3_stream_buffer_t) * PORTRAIT__MAX_NUM_STREAMS);

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    HAL_LOGD("frame_number:%d,num_output_buffers=%d", request->frame_number,
             request->num_output_buffers);
    metaSettingsMain = request->settings;
    metaSettingsAux = request->settings;

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        if (requestStreamType == SNAPSHOT_STREAM) {
            mIsCapturing = true;
            is_captureing = mIsCapturing;
            if (metaSettingsMain.exists(ANDROID_JPEG_ORIENTATION)) {
                mJpegOrientation =
                    metaSettingsMain.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
            }
            mPortrait->mIsCapDepthFinish = false;
            HAL_LOGD("mJpegOrientation=%d", mJpegOrientation);
        } else if (requestStreamType == CALLBACK_STREAM) {
            preview_stream = (request->output_buffers[i]).stream;
            updateApiParams(metaSettingsMain, 0, request->frame_number);
        }
    }

    if (metaSettingsMain.exists(ANDROID_SPRD_PORTRAIT_OPTIMIZATION_MODE)) {
        mBokehMode =
            metaSettingsMain.find(ANDROID_SPRD_PORTRAIT_OPTIMIZATION_MODE)
                .data.u8[0];
        HAL_LOGD("mBokehMode=%d", mBokehMode);
    }

    tagCnt = metaSettingsMain.entryCount();
    if (tagCnt != 0) {
        if (metaSettingsMain.exists(ANDROID_SPRD_BURSTMODE_ENABLED)) {
            uint8_t sprdBurstModeEnabled = 0;
            metaSettingsMain.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                    &sprdBurstModeEnabled, 1);
            metaSettingsAux.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                   &sprdBurstModeEnabled, 1);
        }

        uint8_t sprdZslEnabled = 1;
        metaSettingsMain.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
        metaSettingsAux.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
    }
    saveRequest(request);
    /* save Perfectskinlevel */
    if (metaSettingsMain.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        facebeautylevel.blemishLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        facebeautylevel.smoothLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[1];
        facebeautylevel.skinColor =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[2];
        facebeautylevel.skinLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[3];
        facebeautylevel.brightLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[4];
        facebeautylevel.lipColor =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[5];
        facebeautylevel.lipLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[6];
        facebeautylevel.slimLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[7];
        facebeautylevel.largeLevel =
            metaSettingsMain.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[8];
        CMR_LOGD(
            "fb blemishLevel %d, smoothLevel %d, skinColor %d, lipLevel %d",
            facebeautylevel.blemishLevel, facebeautylevel.smoothLevel,
            facebeautylevel.skinColor, facebeautylevel.lipLevel);
    }

    if ((facebeautylevel.blemishLevel != 0) ||
        (facebeautylevel.smoothLevel != 0) ||
        (facebeautylevel.skinColor != 0) || (facebeautylevel.skinLevel != 0) ||
        (facebeautylevel.brightLevel != 0) || (facebeautylevel.lipColor != 0) ||
        (facebeautylevel.lipLevel != 0) || (facebeautylevel.slimLevel != 0) ||
        (facebeautylevel.largeLevel != 0)) {
        mPortrait->mFaceBeautyFlag = true;
    } else {
        mPortrait->mFaceBeautyFlag = false;
    }
    HAL_LOGV("mPortrait->mFaceBeautyFlag %d", mPortrait->mFaceBeautyFlag);
    req_main = *req;
    req_aux = *req;
    req_main.settings = metaSettingsMain.release();
    req_aux.settings = metaSettingsAux.release();

    int main_buffer_index = 0;
    int aux_buffer_index = 0;
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        new_stream = (req->output_buffers[i]).stream;
        new_buffer = (req->output_buffers[i]).buffer;
        HAL_LOGD("num_output_buffers:%d, streamtype:%d",
                 req->num_output_buffers, requestStreamType);

        if (requestStreamType == SNAPSHOT_STREAM) {
            // first step: save capture request stream info
            if (NULL != mCaptureThread->mSavedCapReqsettings) {
                free_camera_metadata(mCaptureThread->mSavedCapReqsettings);
                mCaptureThread->mSavedCapReqsettings = NULL;
            }
            mCaptureThread->mSavedCapReqsettings =
                clone_camera_metadata(req_main.settings);
            mCaptureThread->mSavedOneResultBuff = NULL;
            mCaptureThread->mSavedCapRequest = *req;
            mCaptureThread->mSavedCapReqStreamBuff = req->output_buffers[i];
            mSavedCapStreams = req->output_buffers[i].stream;
            mSavedCapStreams->reserved[0] = NULL;
            mCapFrameNumber = request->frame_number;
            mCaptureThread->mSavedResultBuff =
                request->output_buffers[i].buffer;
            mjpegSize = ADP_WIDTH(*new_buffer);
            // sencond step:construct callback Request
            out_streams_main[main_buffer_index] = req->output_buffers[i];
            if (!mFlushing) {
                out_streams_main[main_buffer_index].buffer =
                    (popBufferList(mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
                out_streams_main[main_buffer_index].stream =
                    &mMainStreams[mCallbackStreamsNum];
            } else {
                out_streams_main[main_buffer_index].buffer =
                    (req->output_buffers[i]).buffer;
                out_streams_main[main_buffer_index].stream =
                    &mMainStreams[mCaptureStreamsNum];
            }
            HAL_LOGD("jpeg frame newtype:%d, rotation:%d ,frame_number %u",
                     out_streams_main[main_buffer_index].stream->format,
                     out_streams_main[main_buffer_index].stream->rotation,
                     req->frame_number);

            if (NULL == out_streams_main[main_buffer_index].buffer) {
                HAL_LOGE("failed, LocalBufferList is empty!");
                goto req_fail;
            }
            main_buffer_index++;
            if (mIsHdrMode && !mFlushing) {
                camera3_stream_buffer_t *sbuf =
                    &out_streams_main[main_buffer_index];
                *sbuf = req->output_buffers[i];
                sbuf->stream = &mMainStreams[mCallbackStreamsNum];
                sbuf->buffer =
                    popBufferList(mLocalBufferList, SNAPSHOT_MAIN_BUFFER);
                main_buffer_index++;
            }

            out_streams_aux[aux_buffer_index] = req->output_buffers[i];
            if (!mFlushing) {
                out_streams_aux[aux_buffer_index].buffer =
                    (popBufferList(mLocalBufferList, SNAPSHOT_DEPTH_BUFFER));
                out_streams_aux[aux_buffer_index].stream =
                    &mAuxStreams[mCallbackStreamsNum];
            } else {
                out_streams_aux[aux_buffer_index].buffer =
                    (req->output_buffers[i]).buffer;
                out_streams_aux[aux_buffer_index].stream =
                    &mAuxStreams[mCaptureStreamsNum];
            }
            out_streams_aux[aux_buffer_index].acquire_fence = -1;
            if (NULL == out_streams_aux[aux_buffer_index].buffer) {
                HAL_LOGE("failed, LocalBufferList is empty!");
                goto req_fail;
            }
            aux_buffer_index++;
        } else if (requestStreamType == CALLBACK_STREAM) {
            out_streams_main[main_buffer_index] = req->output_buffers[i];
            out_streams_main[main_buffer_index].stream =
                &mMainStreams[mPreviewStreamsNum];
            if (mIsSupportPBokeh) {
                out_streams_main[main_buffer_index].buffer =
                    (popBufferList(mLocalBufferList, PREVIEW_MAIN_BUFFER));
                if (NULL == out_streams_main[main_buffer_index].buffer) {
                    HAL_LOGE("failed, mPrevLocalBufferList is empty!");
                    return NO_MEMORY;
                }
            }
            main_buffer_index++;

            out_streams_aux[aux_buffer_index] = req->output_buffers[i];
            out_streams_aux[aux_buffer_index].stream =
                &mAuxStreams[mPreviewStreamsNum];

            out_streams_aux[aux_buffer_index].buffer =
                (popBufferList(mLocalBufferList, PREVIEW_DEPTH_BUFFER));
            if (NULL == out_streams_aux[aux_buffer_index].buffer) {
                HAL_LOGE("failed, mDepthLocalBufferList is empty!");
                return NO_MEMORY;
            }

            out_streams_aux[aux_buffer_index].acquire_fence = -1;
            aux_buffer_index++;
        } else {
            if (mhasCallbackStream) {
                HAL_LOGD("has callback stream,num_output_buffers=%d",
                         request->num_output_buffers);
            }
        }
    }
    req_main.num_output_buffers = main_buffer_index;
    req_aux.num_output_buffers = aux_buffer_index;
    req_main.output_buffers = out_streams_main;
    req_aux.output_buffers = out_streams_aux;

    if (is_captureing) {
        struct timespec t1;
        clock_gettime(CLOCK_BOOTTIME, &t1);
        uint64_t currentmainTimestamp = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
        uint64_t currentauxTimestamp = currentmainTimestamp;
        HAL_LOGV("currentmainTimestamp=%llu,currentauxTimestamp=%llu",
                 currentmainTimestamp, currentauxTimestamp);
        //    hwiMain->setMultiCallBackYuvMode(true);
        //   hwiAux->setMultiCallBackYuvMode(true);
        hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_BOKEH_SCALE_INFO, &mScaleInfo,
                               NULL);
        if (currentmainTimestamp < currentauxTimestamp) {
            HAL_LOGD("start main, idx:%d", req_main.frame_number);
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
            hwiAux->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                  &currentmainTimestamp, NULL);
            HAL_LOGD("start sub, idx:%d", req_aux.frame_number);
            if (!mFlushing) {
                rc = hwiAux->process_capture_request(
                    m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].dev, &req_aux);
                if (rc < 0) {
                    HAL_LOGE("failed, idx:%d", req_aux.frame_number);
                    goto req_fail;
                }
            }
        } else {
            HAL_LOGD("start sub, idx:%d,currentauxTimestamp=%llu",
                     req_aux.frame_number, currentauxTimestamp);
            hwiAux->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                  &currentmainTimestamp, NULL);
            if (!mFlushing) {
                rc = hwiAux->process_capture_request(
                    m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].dev, &req_aux);
                if (rc < 0) {
                    HAL_LOGE("failed, idx:%d", req_aux.frame_number);
                    goto req_fail;
                }
            }
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            HAL_LOGD("start main, idx:%d", req_main.frame_number);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
        }
    } else {
        rc = hwiMain->process_capture_request(
            m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].dev, &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }
        rc = hwiAux->process_capture_request(
            m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].dev, &req_aux);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_aux.frame_number);
            goto req_fail;
        }
        struct timespec t1;
        clock_gettime(CLOCK_BOOTTIME, &t1);
        mReqTimestamp = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
        mPrevFrameNumber = request->frame_number;
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
    }
req_fail:
    if (req_main.settings)
        free_camera_metadata((camera_metadata_t *)req_main.settings);

    if (req_aux.settings)
        free_camera_metadata((camera_metadata_t *)req_aux.settings);
    HAL_LOGV("rc. %d idx%d", rc, request->frame_number);

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
void SprdCamera3Portrait::notifyMain(const camera3_notify_msg_t *msg) {
    uint32_t cur_frame_number = msg->message.shutter.frame_number;
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        cur_frame_number == mCaptureThread->mSavedCapRequest.frame_number &&
        cur_frame_number != 0) {
        if (msg->message.shutter.timestamp != 0) {
            capture_result_timestamp = msg->message.shutter.timestamp;
            HAL_LOGD(" capture_result_timestamp %lld",
                     capture_result_timestamp);
        }
    }
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        cur_frame_number == mCaptureThread->mSavedCapRequest.frame_number &&
        mCaptureThread->mReprocessing) {
        HAL_LOGD(" hold cap notify");
        return;
    }

    if ((!(cur_frame_number == mCapFrameNumber && cur_frame_number != 0)) &&
        mIsSupportPBokeh) {
        Mutex::Autolock l(mNotifyLockMain);
        mNotifyListMain.push_back(*msg);
        Mutex::Autolock m(mPrevFrameNotifyLock);
        mPrevFrameNotifyList.push_back(*msg);
        if (mPrevFrameNotifyList.size() > BOKEH_PREVIEW_PARAM_LIST) {
            mPrevFrameNotifyList.erase(mPrevFrameNotifyList.begin());
        }
    }

    mCallbackOps->notify(mCallbackOps, msg);
}

/*===========================================================================
 * FUNCTION   :thumbYuvProc
 *
 * DESCRIPTION: process thumb yuv
 *
 * PARAMETERS : process thumb yuv
 *
 * RETURN     : None
 *==========================================================================*/

int SprdCamera3Portrait::thumbYuvProc(buffer_handle_t *src_buffer) {
    int ret = NO_ERROR;
    int angle = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;
    struct img_frm src_img;
    struct img_frm dst_img;
    struct snp_thumb_yuv_param thumb_param;
    void *src_buffer_addr = NULL;
    void *thumb_req_addr = NULL;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    HAL_LOGI(" E");

    ret = mPortrait->map(src_buffer, &src_buffer_addr);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map src buffer");
        goto fail_map_src;
    }
    ret = mPortrait->map(mThumbReq.buffer, &thumb_req_addr);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map thumb buffer");
        goto fail_map_thumb;
    }

    memset(&src_img, 0, sizeof(struct img_frm));
    memset(&dst_img, 0, sizeof(struct img_frm));
    src_img.addr_phy.addr_y = 0;
    src_img.addr_phy.addr_u = src_img.addr_phy.addr_y +
                              ADP_WIDTH(*src_buffer) * ADP_HEIGHT(*src_buffer);
    src_img.addr_phy.addr_v = src_img.addr_phy.addr_u;
    src_img.addr_vir.addr_y = (cmr_uint)src_buffer_addr;
    src_img.addr_vir.addr_u = (cmr_uint)src_buffer_addr +
                              ADP_WIDTH(*src_buffer) * ADP_HEIGHT(*src_buffer);
    src_img.buf_size = ADP_BUFSIZE(*src_buffer);
    src_img.fd = ADP_BUFFD(*src_buffer);
    src_img.fmt = IMG_DATA_TYPE_YUV420;
    src_img.rect.start_x = 0;
    src_img.rect.start_y = 0;
    src_img.rect.width = ADP_WIDTH(*src_buffer);
    src_img.rect.height = ADP_HEIGHT(*src_buffer);
    src_img.size.width = ADP_WIDTH(*src_buffer);
    src_img.size.height = ADP_HEIGHT(*src_buffer);

    dst_img.addr_phy.addr_y = 0;
    dst_img.addr_phy.addr_u =
        dst_img.addr_phy.addr_y +
        ADP_WIDTH(*mThumbReq.buffer) * ADP_HEIGHT(*mThumbReq.buffer);
    dst_img.addr_phy.addr_v = dst_img.addr_phy.addr_u;

    dst_img.addr_vir.addr_y = (cmr_uint)thumb_req_addr;
    dst_img.addr_vir.addr_u =
        (cmr_uint)thumb_req_addr +
        ADP_WIDTH(*mThumbReq.buffer) * ADP_HEIGHT(*mThumbReq.buffer);
    dst_img.buf_size = ADP_BUFSIZE(*mThumbReq.buffer);
    dst_img.fd = ADP_BUFFD(*mThumbReq.buffer);
    dst_img.fmt = IMG_DATA_TYPE_YUV420;
    dst_img.rect.start_x = 0;
    dst_img.rect.start_y = 0;
    dst_img.rect.width = ADP_WIDTH(*mThumbReq.buffer);
    dst_img.rect.height = ADP_HEIGHT(*mThumbReq.buffer);
    dst_img.size.width = ADP_WIDTH(*mThumbReq.buffer);
    dst_img.size.height = ADP_HEIGHT(*mThumbReq.buffer);

    memcpy(&thumb_param.src_img, &src_img, sizeof(struct img_frm));
    memcpy(&thumb_param.dst_img, &dst_img, sizeof(struct img_frm));
    switch (mJpegOrientation) {
    case 0:
        angle = IMG_ANGLE_0;
        break;

    case 180:
        angle = IMG_ANGLE_180;
        break;

    case 90:
        angle = IMG_ANGLE_90;
        break;

    case 270:
        angle = IMG_ANGLE_270;
        break;

    default:
        angle = IMG_ANGLE_0;
        break;
    }
    thumb_param.angle = angle;

    hwiMain->camera_ioctrl(CAMERA_IOCTRL_THUMB_YUV_PROC, &thumb_param, NULL);

    property_get("persist.vendor.cam.bokeh.dump", prop, "0");
    if (!strcmp(prop, "thumb") || !strcmp(prop, "all")) {
        dumpData((unsigned char *)src_img.addr_vir.addr_y, 1, src_img.buf_size,
                 src_img.rect.width, src_img.size.height, 5, "input");
        dumpData((unsigned char *)dst_img.addr_vir.addr_y, 1, dst_img.buf_size,
                 dst_img.rect.width, dst_img.size.height, 5, "output");
    }

    HAL_LOGI(" x,angle=%d JpegOrientation=%d", thumb_param.angle,
             mJpegOrientation);

    mPortrait->unmap(mThumbReq.buffer);
fail_map_thumb:
    mPortrait->unmap(src_buffer);
fail_map_src:

    return ret;
}

void SprdCamera3Portrait::dumpCaptureBokeh(unsigned char *result_buffer_addr,
                                           uint32_t jpeg_size) {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.bokeh.dump", prop, "0");
    if (!strcmp(prop, "capture") || !strcmp(prop, "all")) {
        uint32_t para_size = 0;
        uint32_t depth_size = 0;
        int common_num = 0;
        int depth_width = 0, depth_height = 0;
        uint32_t depth_yuv_normalize_size = 0;
        int xmp_size = 0;
        xmp_size = BOKEH_REFOCUS_COMMON_XMP_SIZE;

        if (mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) {
            para_size = BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
            depth_size =
                mBokehSize.depth_snap_out_w * mBokehSize.depth_snap_out_h * 2;
            common_num = BOKEH_REFOCUS_COMMON_PARAM_NUM;
            depth_width = mBokehSize.depth_snap_out_w;
            depth_height = mBokehSize.depth_snap_out_h;
        }
        uint32_t orig_yuv_size = mPortrait->mBokehSize.capture_w *
                                 mPortrait->mBokehSize.capture_h * 3 / 2;
        unsigned char *buffer_base = result_buffer_addr;
        dumpData(buffer_base, 2, mXmpSize + jpeg_size, mBokehSize.capture_w,
                 mBokehSize.capture_h, mPortrait->mCapFrameNumber, "bokehJpeg");
        HAL_LOGD("jpeg1 size=:%d", mXmpSize + jpeg_size);
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        depth_yuv_normalize_size =
            mPortrait->mBokehSize.depth_yuv_normalize_size;
#endif
#ifdef YUV_CONVERT_TO_JPEG
        uint32_t use_size = para_size + depth_yuv_normalize_size + depth_size +
                            mPortrait->mOrigJpegSize + jpeg_size + xmp_size;
#else
        uint32_t use_size = para_size + depth_size + orig_yuv_size + jpeg_size;
#endif
        buffer_base += (use_size - para_size + mXmpSize);
        dumpData(buffer_base, 3, common_num, 4, 0, 0, "parameter");
        buffer_base -= (int)(depth_size);

        dumpData(buffer_base, 1, depth_size, depth_width, depth_height,
                 mPortrait->mCapFrameNumber, "depth");
#ifdef YUV_CONVERT_TO_JPEG
        buffer_base -=
            (int)(mPortrait->mOrigJpegSize + depth_yuv_normalize_size);
        dumpData(buffer_base, 2, mPortrait->mOrigJpegSize, mBokehSize.capture_w,
                 mBokehSize.capture_h, mPortrait->mCapFrameNumber, "origJpeg");

#else
        buffer_base -= (int)(orig_yuv_size);
        dumpData(buffer_base, 1, orig_yuv_size, mBokehSize.capture_w,
                 mBokehSize.capture_h, mPortrait->mCapFrameNumber, "origYuv");
#endif
    }
}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the main hwi
 *
 * PARAMETERS : capture result structure from hwi1
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::processCaptureResultMain(
    const camera3_capture_result_t *result) {
    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    meta_save_t metadata_t;
    int vcmSteps = 0;
    int vcmSteps_fixed = 0;
    uint8_t afState = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].hwi;
    if (result_buffer == NULL) {
        // meta process
        metadata = result->result;
        updateApiParams(metadata, 1, cur_frame_number);
        if (metadata.exists(ANDROID_SPRD_VCM_STEP) && cur_frame_number) {
            vcmSteps = metadata.find(ANDROID_SPRD_VCM_STEP).data.i32[0];
            setDepthTrigger(vcmSteps);
        }
        if (metadata.exists(ANDROID_SPRD_VCM_STEP_FOR_BOKEH) &
            cur_frame_number) {
            vcmSteps_fixed =
                metadata.find(ANDROID_SPRD_VCM_STEP_FOR_BOKEH).data.i32[0];
            if (metadata.exists(ANDROID_CONTROL_AF_STATE)) {
                afState = metadata.find(ANDROID_CONTROL_AF_STATE).data.i32[0];
            }
            if (afState == ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
                vcmSteps_fixed = FOCUS_FAIL;
            }
            mVcmStepsFixed = vcmSteps_fixed;
            HAL_LOGD("VCM_INFO:vcmSteps=%d afState %d", vcmSteps_fixed,
                     afState);
        }
        if (cur_frame_number == mCapFrameNumber && cur_frame_number != 0) {
            if (mCaptureThread->mReprocessing) {
                HAL_LOGD("hold jpeg picture call bac1k, framenumber:%d",
                         result->frame_number);
            } else {
                {
                    Mutex::Autolock l(mPortrait->mMetatLock);
                    metadata_t.frame_number = cur_frame_number;
                    metadata_t.metadata = clone_camera_metadata(result->result);
                    mMetadataList.push_back(metadata_t);
                }
                CallBackMetadata();
            }
            return;
        } else {
            if (metadata.exists(ANDROID_CONTROL_AF_STATE)) {
                mCurAFStatus =
                    metadata.find(ANDROID_CONTROL_AF_STATE).data.u8[0];
            }
            if (metadata.exists(ANDROID_CONTROL_AF_MODE)) {
                mCurAFMode = metadata.find(ANDROID_CONTROL_AF_MODE).data.u8[0];
            }

            HAL_LOGV("send  meta, framenumber:%d", cur_frame_number);
            metadata_t.frame_number = cur_frame_number;
            metadata_t.metadata = clone_camera_metadata(result->result);
            Mutex::Autolock l(mPortrait->mMetatLock);
            mMetadataList.push_back(metadata_t);
            return;
        }
    }

    int currStreamType = getStreamType(result_buffer->stream);
    /* Process error buffer for Main camera*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGD("Return local buffer:%d caused by error Buffer status",
                 result->frame_number);
        if (currStreamType == CALLBACK_STREAM) {
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
        }
        if (mhasCallbackStream &&
            mThumbReq.frame_number == result->frame_number &&
            mThumbReq.frame_number) {
            CallBackSnapResult(CAMERA3_BUFFER_STATUS_ERROR);
        }
        CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR);

        return;
    }

    if (mIsCapturing && (currStreamType == DEFAULT_STREAM)) {
        mSnapshotResultReturn = true;
        if (mFlushing ||
            result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            if (mhasCallbackStream &&
                mThumbReq.frame_number == result->frame_number &&
                mThumbReq.frame_number) {
                CallBackSnapResult(CAMERA3_BUFFER_STATUS_ERROR);
            }
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR);
            return;
        }
        if (mhasCallbackStream && mThumbReq.frame_number) {
            thumbYuvProc(result->output_buffers->buffer);
            CallBackSnapResult(CAMERA3_BUFFER_STATUS_OK);
        }
        Mutex::Autolock l(mDefaultStreamLock);
        if (mIsHdrMode) {
            mHdrCallbackCnt++;
        }

        if (NULL == mCaptureThread->mSavedOneResultBuff) {
            mCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {
            capture_queue_msg_t_portrait capture_msg;
            capture_msg.msg_type = PORTRAIT_MSG_DATA_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.buffer1 = result->output_buffers->buffer;
            // hdr mode: the first DEFAULT_STREAM is normal frame,
            // and the second is hdr frame.
            if (mHdrCallbackCnt == 2) {
                mHdrCallbackCnt = 0;
                capture_msg.combo_buff.buffer2 = NULL;
            } else {
                capture_msg.combo_buff.buffer2 =
                    mCaptureThread->mSavedOneResultBuff;
            }
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            HAL_LOGD("main capture combined begin: framenumber %d",
                     capture_msg.combo_buff.frame_number);
            {
                //    hwiMain->setMultiCallBackYuvMode(false);
                //   hwiAux->setMultiCallBackYuvMode(false);
                Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         capture_msg.combo_buff.frame_number);
                mPortrait->mHdrSkipBlur = false;
                mCaptureThread->mCaptureMsgList.push_front(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (mIsCapturing && currStreamType == SNAPSHOT_STREAM) {
        bzero(mJpegOutputBuffers, sizeof(camera3_stream_buffer_t));
        memcpy(mJpegOutputBuffers, result->output_buffers,
               sizeof(camera3_stream_buffer_t));
        jpeg_callback_thread_init((void *)this);
        /*
        bool save_param = false;
        HAL_LOGD("jpeg callback: framenumber %d", cur_frame_number);
        int rc = NO_ERROR;
        unsigned char *result_buffer_addr = NULL;
        uint32_t result_buffer_size =
            ADP_WIDTH(*result->output_buffers->buffer);
        rc = mPortrait->map(result->output_buffers->buffer,
                             (void **)&result_buffer_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map result buffer");
            return;
        }
        uint32_t jpeg_size =
            getJpegSize(result_buffer_addr, result_buffer_size);
#ifdef YUV_CONVERT_TO_JPEG
        if ((mCaptureThread->mBokehResult == true) &&
            (mPortrait->mOrigJpegSize > 0)) {
            save_param = true;
        }
#else
        if (mCaptureThread->mBokehResult == true) {
            save_param = true;
        }
#endif
        if (save_param) {
            mCaptureThread->saveCaptureBokehParams(result_buffer_addr,
                                                   mjpegSize, jpeg_size);
            dumpCaptureBokeh(result_buffer_addr, jpeg_size);
        } else {
            mPortrait->setJpegSize((char *)result_buffer_addr, mjpegSize,
                                    jpeg_size);
        }
#ifdef YUV_CONVERT_TO_JPEG
        mPortrait->pushBufferList(
            mPortrait->mLocalBuffer, mPortrait->m_pDstJpegBuffer,
            mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);
        mPortrait->pushBufferList(
            mPortrait->mLocalBuffer, mPortrait->m_pDstGDepthOriJpegBuffer,
            mPortrait->mLocalBufferNumber, mPortrait->mLocalBufferList);

#endif
        CallBackResult(mCapFrameNumber, CAMERA3_BUFFER_STATUS_OK);
        mCaptureThread->mReprocessing = false;
        mIsCapturing = false;
        mPortrait->unmap(result->output_buffers->buffer);
        */
    } else if (currStreamType == CALLBACK_STREAM) {
        if (!mIsSupportPBokeh) {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK);
            return;
        }
        // process preview buffer
        {
            Mutex::Autolock l(mNotifyLockMain);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListMain.begin();
                 i != mNotifyListMain.end();) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        mNotifyListMain.erase(i++);
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("Return local buffer:%d caused by error "
                                 "Notify status",
                                 result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        CallBackResult(cur_frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR);
                        mNotifyListMain.erase(i++);
                        return;
                    }
                } else
                    i++;
            }
        }
        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("found no corresponding notify");
            return;
        }

        hwi_frame_buffer_info_t matched_frame;
        hwi_frame_buffer_info_t cur_frame;

        memset(&matched_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListAux,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = cur_frame.buffer;
                muxer_msg.combo_frame.buffer2 = matched_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("Enqueue combo frame:%d for frame merge!",
                             muxer_msg.combo_frame.frame_number);
                    if (cur_frame.frame_number > 5)
                        clearFrameNeverMatched(cur_frame.frame_number,
                                               matched_frame.frame_number);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("Enqueue newest unmatched frame:%d for Main camera",
                         cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
                if (discard_frame != NULL) {
                    HAL_LOGD("discard frame_number %u", cur_frame_number);
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    if (cur_frame.frame_number > 5)
                        CallBackResult(discard_frame->frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR);
                    delete discard_frame;
                }
            }
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: notifyAux
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::notifyAux(const camera3_notify_msg_t *msg) {
    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    if ((!(cur_frame_number == mCapFrameNumber && cur_frame_number != 0)) &&
        mIsSupportPBokeh) {
        Mutex::Autolock l(mNotifyLockAux);
        HAL_LOGV("notifyAux push success frame_number %d", cur_frame_number);
        mNotifyListAux.push_back(*msg);
    }
    return;
}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi2
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::processCaptureResultAux(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    CameraMetadata metadata;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    metadata = result->result;
    uint64_t result_timestamp = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    camera3_stream_t *newStream = NULL;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].hwi;
    HAL_LOGV("aux frame_number %d", cur_frame_number);
    if (result->output_buffers == NULL) {
        return;
    }
    if (mFlushing) {
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                       mLocalBufferNumber, mLocalBufferList);
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    if (mIsCapturing && currStreamType == DEFAULT_STREAM) {
        Mutex::Autolock l(mDefaultStreamLock);
        result_buffer = result->output_buffers;
        if (NULL == mCaptureThread->mSavedOneResultBuff) {
            mCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {
            capture_queue_msg_t_portrait capture_msg;

            capture_msg.msg_type = PORTRAIT_MSG_DATA_PROC;
            capture_msg.combo_buff.frame_number = result->frame_number;
            capture_msg.combo_buff.buffer1 =
                mCaptureThread->mSavedOneResultBuff;
            capture_msg.combo_buff.buffer2 = result->output_buffers->buffer;
            capture_msg.combo_buff.input_buffer = result->input_buffer;
            HAL_LOGD("aux capture combined begin: framenumber %d",
                     capture_msg.combo_buff.frame_number);
            {
                //    hwiMain->setMultiCallBackYuvMode(false);
                //   hwiAux->setMultiCallBackYuvMode(false);
                Mutex::Autolock l(mCaptureThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         capture_msg.combo_buff.frame_number);
                mPortrait->mHdrSkipBlur = false;
                mCaptureThread->mCaptureMsgList.push_front(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (mIsCapturing && currStreamType == SNAPSHOT_STREAM) {
        HAL_LOGD("should not entry here, shutter frame:%d",
                 result->frame_number);
    } else if (currStreamType == CALLBACK_STREAM) {
        if (!mIsSupportPBokeh) {
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }
        // process preview buffer
        {
            Mutex::Autolock l(mNotifyLockAux);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListAux.begin();
                 i != mNotifyListAux.end();) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        mNotifyListAux.erase(i++);
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("Return local buffer:%d caused by error "
                                 "Notify status",
                                 result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        mNotifyListAux.erase(i++);
                        return;
                    }
                } else
                    i++;
            }
        }
        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("found no corresponding notify");
            return;
        }
        /* Process error buffer for Aux camera: just return local buffer*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGD("Return local buffer:%d caused by error Buffer status",
                     result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }
        hwi_frame_buffer_info_t matched_frame;
        hwi_frame_buffer_info_t cur_frame;

        memset(&matched_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListMain,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = matched_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame.buffer;
                muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("Enqueue combo frame:%d for frame merge!",
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
                        clearFrameNeverMatched(matched_frame.frame_number,
                                               cur_frame.frame_number);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("Enqueue newest unmatched frame:%d for Aux camera",
                         cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux);
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
 * FUNCTION   :CallBackMetadata
 *
 * DESCRIPTION: CallBackMetadata
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::CallBackMetadata() {

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
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(result.result));
            mMetadataList.erase(itor++);
        }
    }
}

/*===========================================================================
 * FUNCTION   :CallBackSnapResult
 *
 * DESCRIPTION: CallBackSnapResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::CallBackSnapResult(int status) {

    camera3_capture_result_t result;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    result_buffers.stream = mThumbReq.preview_stream;
    result_buffers.buffer = mThumbReq.buffer;

    result_buffers.status = status;
    result_buffers.acquire_fence = -1;
    result_buffers.release_fence = -1;
    result.result = NULL;
    result.frame_number = mThumbReq.frame_number;
    result.num_output_buffers = 1;
    result.output_buffers = &result_buffers;
    result.input_buffer = NULL;
    result.partial_result = 0;

    mCallbackOps->process_capture_result(mCallbackOps, &result);
    memset(&mThumbReq, 0, sizeof(multi_request_saved_t));
    HAL_LOGD("snap id: %d ", result.frame_number);
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
void SprdCamera3Portrait::CallBackResult(
    uint32_t frame_number, camera3_buffer_status_t buffer_status) {

    camera3_capture_result_t result;
    List<multi_request_saved_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    CallBackMetadata();
    if ((frame_number != mPortrait->mCapFrameNumber) || (frame_number == 0)) {
        Mutex::Autolock l(mPortrait->mRequestLock);
        itor = mPortrait->mSavedRequestList.begin();
        while (itor != mPortrait->mSavedRequestList.end()) {
            if (itor->frame_number == frame_number) {
                HAL_LOGV("erase frame_number %u", frame_number);
                result_buffers.stream = itor->preview_stream;
                result_buffers.buffer = itor->buffer;
                mPortrait->mSavedRequestList.erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mPortrait->mSavedRequestList.end()) {
            HAL_LOGE("can't find frame in mSavedRequestList %u:", frame_number);
            return;
        }
    } else {
        result_buffers.stream = mPortrait->mSavedCapStreams;
        result_buffers.buffer = mCaptureThread->mSavedCapReqStreamBuff.buffer;
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
    HAL_LOGI("id:%d buffer_status %u", result.frame_number, buffer_status);
    if (!buffer_status) {
        mPortrait->dumpFps();
    }

    if ((frame_number != mPortrait->mCapFrameNumber) || (frame_number == 0)) {
        Mutex::Autolock l(mPortrait->mPendingLock);
        mPortrait->mPendingRequest--;
        if (mPortrait->mPendingRequest < mPortrait->mMaxPendingCount) {
            HAL_LOGV("signal request m_PendingRequest = %d",
                     mPortrait->mPendingRequest);
            mPortrait->mRequestSignal.signal();
        }
    } else {
        mPortrait->mCapFrameNumber = 0;
    }
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3Portrait
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::_dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :preClose
 *
 * DESCRIPTION: preview and capture thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::preClose(void) {

    HAL_LOGI("E");
    if ((mPortrait->mApiVersion == SPRD_API_PORTRAIT_MODE) && mBokehAlgo) {
        mBokehAlgo->setFlag();
    }
    if (mCaptureThread != NULL) {
        if (mCaptureThread->isRunning()) {
            mCaptureThread->requestExit();
        }
        // wait threads quit to relese object
        mCaptureThread->join();
    }
    if (mPreviewMuxerThread != NULL) {
        if (mPreviewMuxerThread->isRunning()) {
            mPreviewMuxerThread->requestExit();
        }
        // wait threads quit to relese object
        mPreviewMuxerThread->join();
    }
    if (mDepthMuxerThread != NULL) {
        if (mDepthMuxerThread->isRunning()) {
            mDepthMuxerThread->requestExit();
        }
        // wait threads quit to relese object
        mDepthMuxerThread->join();
    }

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
int SprdCamera3Portrait::_flush(const struct camera3_device *device) {
    int rc = 0;
    HAL_LOGI("E");
    Mutex::Autolock l(mFlushLock);
    Mutex::Autolock jcl(mJpegCallbackLock);
    mFlushing = true;
    sem_destroy(&mFaceinfoSignSem);
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_PORTRAIT_MAIN].dev);

    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_PORTRAIT_DEPTH].dev);

    preClose();
    if (mBokehAlgo) {
        rc = mBokehAlgo->deinitPortrait();
        if (rc != NO_ERROR)
            HAL_LOGE("fail to deinitPortrait");
        rc = mBokehAlgo->deinitLightPortrait();
        if (rc != NO_ERROR)
            HAL_LOGE("fail to deinitLightPortrait");
#ifdef CONFIG_FACE_BEAUTY
        rc = mBokehAlgo->deinitFaceBeauty();
        if (rc != NO_ERROR)
            HAL_LOGE("fail to deinitFaceBeauty");
#endif
    }

    HAL_LOGI("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   :clearFrameNeverMatched
 *
 * DESCRIPTION: clear earlier frame which will never be matched any more
 *
 * PARAMETERS : which camera queue to be clear
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::clearFrameNeverMatched(uint32_t main_frame_number,
                                                 uint32_t sub_frame_number) {
    List<hwi_frame_buffer_info_t>::iterator itor;
    uint32_t frame_num = 0;
    Mutex::Autolock l(mClearBufferLock);

    itor = mUnmatchedFrameListMain.begin();
    while (itor != mUnmatchedFrameListMain.end()) {
        if (itor->frame_number < main_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame main idx:%d", itor->frame_number);
            frame_num = itor->frame_number;
            mUnmatchedFrameListMain.erase(itor++);
            CallBackResult(frame_num, CAMERA3_BUFFER_STATUS_ERROR);
            continue;
        }
        itor++;
    }

    itor = mUnmatchedFrameListAux.begin();
    while (itor != mUnmatchedFrameListAux.end()) {
        if (itor->frame_number < sub_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux idx:%d", itor->frame_number);
            mUnmatchedFrameListAux.erase(itor++);
            continue;
        }
        itor++;
    }
}
/*===========================================================================
 * FUNCTION   :setDepthStatus
 *
 * DESCRIPTION:
 *
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::setDepthStatus(DepthStatusPortrait state) {

    Mutex::Autolock l(mDepthStatusLock);
    DepthStatusPortrait pre_status = mDepthStatus;
    switch (pre_status) {
    case DEPTH_DONING_PORTRAIT:
        mDepthStatus = state;
        if (state == DEPTH_DONE_PORTRAIT) {
            mDepthTrigger = TRIGGER_FALSE_PORTRAIT;
        }
        break;
    case DEPTH_DONE_PORTRAIT:
        mDepthStatus = state;
        break;
    case DEPTH_INVALID_PORTRAIT:
        if (state == DEPTH_DONE_PORTRAIT) {
            mDepthStatus = DEPTH_INVALID_PORTRAIT;
        } else {
            mDepthStatus = state;
        }
        break;
    default:
        mDepthStatus = state;
        break;
    }

    if (mFlushing) {
        mDepthStatus = DEPTH_INVALID_PORTRAIT;
    }
    HAL_LOGV("set depth status %d to %d,mDepthTrigger=%d", pre_status, state,
             mDepthTrigger);
}

/*===========================================================================
 * FUNCTION   :setDepthTrigger
 *
 * DESCRIPTION:
 *
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Portrait::setDepthTrigger(int vcm) {

    if (mAfstate) {
        mDepthTrigger = TRIGGER_AF_PORTRAIT;
    } else if (mVcmSteps != vcm || mFlushing) {
        setDepthStatus(DEPTH_INVALID_PORTRAIT);
        mDepthTrigger = TRIGGER_FALSE_PORTRAIT;
    }
    HAL_LOGV("mDepthStatus %d,trigger=%d,vcm=%d,%d,mAfstate=%d", mDepthStatus,
             mDepthTrigger, mVcmSteps, vcm, mAfstate);

    mVcmSteps = vcm;
}

unsigned char *SprdCamera3Portrait::getaddr(unsigned char *result_buffer_addr,
                                            uint32_t buffer_size) {
    unsigned char *buffer_base = result_buffer_addr;
    HAL_LOGI("buffer_base %p", buffer_base);
    buffer_base += buffer_size;
    HAL_LOGI("buffer_base2 %p", buffer_base);
    return buffer_base;
}

/*===========================================================================
 * FUNCTION   : insertGDepthMetadata
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3Portrait::insertGDepthMetadata(unsigned char *result_buffer_addr,
                                              uint32_t result_buffer_size,
                                              uint32_t jpeg_size) {
    int rc = 0;
    FILE *fp = NULL;
    char file2_name[256] = {0};

    strcpy(file2_name, CAMERA_DUMP_PATH);
    strcat(file2_name, "xmp_temp2.jpg");

    // remove previous temp file
    remove(file2_name);

    uint32_t para_size = 0;
    uint32_t depth_size = 0;
    uint32_t depth_yuv_normalize_size = 0;
    int xmp_size = 0;
    para_size = BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
    depth_size = mPortrait->mBokehSize.depth_snap_size;
    xmp_size = BOKEH_REFOCUS_COMMON_XMP_SIZE;
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
    depth_yuv_normalize_size = mPortrait->mBokehSize.depth_yuv_normalize_size;
#endif
    uint32_t use_size = para_size + depth_yuv_normalize_size + depth_size +
                        mPortrait->mOrigJpegSize + jpeg_size + xmp_size;

    // xmp_temp2.jpg is for XMP to process
    fp = fopen(file2_name, "wb");
    if (fp == NULL) {
        HAL_LOGE("Unable to open file: %s \n", file2_name);
        return -1;
    }
    fwrite((void *)result_buffer_addr, 1, use_size, fp);
    fclose(fp);

    string encodeToBase64String;
    string encodeToBase64StringOrigJpeg;
    encodeOriginalJPEGandDepth(&encodeToBase64String,
                               &encodeToBase64StringOrigJpeg);

    if (!SXMPMeta::Initialize()) {
        HAL_LOGE("Could not initialize SXMPMeta");
        return -1;
    }

    XMP_OptionBits options = 0;
    options |= kXMPFiles_ServerMode;

    // Must initialize SXMPFiles before we use it
    if (SXMPFiles::Initialize(options)) {
        // Options to open the file with - open for editing and use a smart
        // handler
        XMP_OptionBits opts =
            kXMPFiles_OpenForUpdate | kXMPFiles_OpenUseSmartHandler;

        bool ok;
        SXMPFiles xmpFile;

        // First we try and open the file
        ok = xmpFile.OpenFile(file2_name, kXMP_UnknownFile, opts);
        if (ok) {
            SXMPMeta::RegisterNamespace(gCameraURI, gCameraPrefix, NULL);
            SXMPMeta::RegisterNamespace(gDepthURI, gDepthPrefix, NULL);
            SXMPMeta::RegisterNamespace(gImageURI, gImagePrefix, NULL);

            SXMPMeta meta;
            xmpFile.GetXMP(&meta);

            bool exists;
            // Set gCamera property ++++++
            string simpleValue;
            exists = meta.GetProperty(gCameraURI, "SpecialTypeID", &simpleValue,
                                      NULL);
            if (exists) {
                HAL_LOGI("SpecialTypeID = %s", simpleValue.c_str());
            } else {
                meta.SetProperty(gCameraURI, "SpecialTypeID",
                                 "BOKEH_PHOTO_TYPE", 0);
            }
            HAL_LOGI("SpecialTypeID");
            // Set gCamera property ------

            // Set gDepth property ++++++
            string formatValue;
            exists = meta.GetProperty(gDepthURI, "Format", &formatValue, NULL);
            if (exists) {
                HAL_LOGI("Format = %s", formatValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "Format", "RangeLinear", 0);
            }
            HAL_LOGI("Format");

            double nearValueD = 0.0f;
            exists =
                meta.GetProperty_Float(gDepthURI, "Near", &nearValueD, NULL);
            if (exists) {
                HAL_LOGI("Near = %f", nearValueD);
            } else {
                meta.SetProperty_Float(gDepthURI, "Near", mPortrait->near, 0);
            }
            HAL_LOGI("Near1 %u ", mPortrait->near);

            double farValueD = 0.0f;
            exists = meta.GetProperty_Float(gDepthURI, "Far", &farValueD, NULL);
            if (exists) {
                HAL_LOGI("Far = %f", farValueD);
            } else {
                meta.SetProperty_Float(gDepthURI, "Far", mPortrait->far, 0);
            }
            HAL_LOGI("Far1 %u ", mPortrait->far);

            string depthMimeValue;
            exists = meta.GetProperty(gDepthURI, "Mime", &depthMimeValue, NULL);
            if (exists) {
                HAL_LOGI("depth:Mime = %s", depthMimeValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "Mime", "image/jpeg", 0);
            }
            HAL_LOGI("depth:Mime");

            string depthDataValue;
            exists = meta.GetProperty(gDepthURI, "Data", &depthDataValue, NULL);
            if (exists) {
                HAL_LOGI("depth:Data = %s", depthDataValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "Data",
                                 encodeToBase64String.c_str(), 0);
            }
            HAL_LOGI("depth:Data %s", encodeToBase64String.c_str());

            string unitsValue;
            exists = meta.GetProperty(gDepthURI, "Units", &unitsValue, NULL);
            if (exists) {
                HAL_LOGI("Units = %s", unitsValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "Units", "m", 0);
            }
            HAL_LOGI("Units");

            string measureTypeValue;
            exists = meta.GetProperty(gDepthURI, "MeasureType",
                                      &measureTypeValue, NULL);
            if (exists) {
                HAL_LOGI("MeasureType = %s", measureTypeValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "MeasureType", "OpticalAxis", 0);
            }
            HAL_LOGI("MeasureType");

            string confidenceMimeValue;
            exists = meta.GetProperty(gDepthURI, "ConfidenceMime",
                                      &confidenceMimeValue, NULL);
            if (exists) {
                HAL_LOGI("ConfidenceMime = %s", confidenceMimeValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "ConfidenceMime", "image/jpeg", 0);
            }
            HAL_LOGI("ConfidenceMime");

            string manufacturerValue;
            exists = meta.GetProperty(gDepthURI, "Manufacturer",
                                      &manufacturerValue, NULL);
            if (exists) {
                HAL_LOGI("Manufacturer = %s", manufacturerValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "Manufacturer", "UNISOC", 0);
            }
            HAL_LOGI("Manufacturer");

            string modelValue;
            exists = meta.GetProperty(gDepthURI, "Model", &modelValue, NULL);
            if (exists) {
                HAL_LOGI("Model = %s", modelValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "Model", "SharkLE", 0);
            }
            HAL_LOGI("Model");

            string softwareValue;
            exists =
                meta.GetProperty(gDepthURI, "Software", &softwareValue, NULL);
            if (exists) {
                HAL_LOGI("Software = %s", softwareValue.c_str());
            } else {
                meta.SetProperty(gDepthURI, "Software", "ALgo", 0);
            }
            HAL_LOGI("Software");

            XMP_Int32 imageWidthValue = 0;
            exists = meta.GetProperty_Int(gDepthURI, "ImageWidth",
                                          &imageWidthValue, NULL);
            if (exists) {
                HAL_LOGI("ImageWidth = %d", imageWidthValue);
            } else {
                meta.SetProperty_Int(gDepthURI, "ImageWidth",
                                     mBokehSize.callback_w, 0);
            }
            HAL_LOGI("ImageWidth %d ", mBokehSize.callback_w);

            XMP_Int32 imageHeightValue = 0;
            exists = meta.GetProperty_Int(gDepthURI, "ImageHeight",
                                          &imageHeightValue, NULL);
            if (exists) {
                HAL_LOGI("ImageHeight = %d", imageHeightValue);
            } else {
                meta.SetProperty_Int(gDepthURI, "ImageHeight",
                                     mBokehSize.callback_h, 0);
            }
            HAL_LOGI("ImageHeight %d ", mBokehSize.callback_h);
            // Set gDepth property ------

            // Set gImage property ++++++
            string imageMimeValue;
            exists = meta.GetProperty(gImageURI, "Mime", &imageMimeValue, NULL);
            if (exists) {
                HAL_LOGI("image:Mime = %s", imageMimeValue.c_str());
            } else {
                meta.SetProperty(gImageURI, "Mime", "image/jpeg", 0);
            }
            HAL_LOGI("image:Mime");

            string imageDataValue;
            exists = meta.GetProperty(gImageURI, "Data", &imageDataValue, NULL);
            if (exists) {
                HAL_LOGI("image:Data = %s", imageDataValue.c_str());
            } else {
                meta.SetProperty(gImageURI, "Data",
                                 encodeToBase64StringOrigJpeg.c_str(), 0);
            }
            HAL_LOGI("image:Data %s", encodeToBase64StringOrigJpeg.c_str());

            // Set gImage property ------

            string standardXMP;
            string extendedXMP;
            string extendedDigest;
            SXMPUtils::PackageForJPEG(meta, &standardXMP, &extendedXMP,
                                      &extendedDigest);
            HAL_LOGI("standardXMP = %s", standardXMP.c_str());
            HAL_LOGI("extendedXMP = %s", extendedXMP.c_str());
            HAL_LOGI("extendedDigest = %s", extendedDigest.c_str());

            if (xmpFile.CanPutXMP(standardXMP.c_str())) {
                xmpFile.PutXMP(standardXMP.c_str());
                if (xmpFile.CanPutXMP(extendedXMP.c_str())) {
                    xmpFile.PutXMP(extendedXMP.c_str());
                } else {
                    HAL_LOGE("Unable to put extendedXMP");
                }
            } else {
                HAL_LOGE("Unable to put standardXMP");
            }
            xmpFile.CloseFile();
        } else {
            HAL_LOGE("Unable to open file %s", file2_name);
        }
    } else {
        HAL_LOGE("Could not initialize SXMPFiles");
        SXMPMeta::Terminate();
        return -1;
    }

    SXMPFiles::Terminate();
    SXMPMeta::Terminate();

    // copy XMP-modified jpg to result buffer
    fp = fopen(file2_name, "rb");
    fseek(fp, 0, SEEK_END);
    long size;
    size = ftell(fp);
    mXmpSize = size - use_size;
    rewind(fp);
    fread((void *)result_buffer_addr, 1, size, fp);
    fclose(fp);

    mPortrait->setJpegSize((char *)result_buffer_addr, result_buffer_size,
                           size);

    return rc;
}

void SprdCamera3Portrait::encodeOriginalJPEGandDepth(
    string *encodeToBase64String, string *encodeToBase64StringOrigJpeg) {

    // string encodeToBase64String;
    SXMPUtils::EncodeToBase64(
        (char *)mPortrait->mDepthBuffer.snap_gdepthjpeg_buffer_addr,
        mPortrait->mBokehSize.depth_jepg_size, encodeToBase64String);
    mPortrait->unmap(mPortrait->mDepthBuffer.snap_gdepthJpg_buffer);
    HAL_LOGI("encodeToBase64String = %s", encodeToBase64String->c_str());

    // char file1_name[256] = {0};
    // strcpy(file1_name, CAMERA_DUMP_PATH);
    // strcat(file1_name, "string1.txt");
    // remove(file1_name);
    // FILE *fPtr1;
    // fPtr1 = fopen(file1_name,"w");
    // fprintf(fPtr1, encodeToBase64String->c_str());
    // fclose(fPtr1);
    // dumpData(buffer_base, 1, depth_yuv_normalize_size,
    // mBokehSize.depth_snap_out_w, mBokehSize.depth_snap_out_h,
    // mPortrait->mCapFrameNumber, "gdepth");

    // string encodeToBase64StringOrigJpeg;
    void *gdepth_ori_jpeg_addr = NULL;
    mPortrait->map(mPortrait->m_pDstGDepthOriJpegBuffer, &gdepth_ori_jpeg_addr);
    SXMPUtils::EncodeToBase64((char *)gdepth_ori_jpeg_addr,
                              mPortrait->mGDepthOriJpegSize,
                              encodeToBase64StringOrigJpeg);
    mPortrait->unmap(mPortrait->m_pDstGDepthOriJpegBuffer);
    HAL_LOGI("encodeToBase64StringOrigJpeg = %s",
             encodeToBase64StringOrigJpeg->c_str());

    // char file2_name[256] = {0};
    // strcpy(file2_name, CAMERA_DUMP_PATH);
    // strcat(file2_name, "string2.txt");
    // remove(file2_name);
    // FILE *fPtr2;
    // fPtr2 = fopen(file2_name,"w");
    // fprintf(fPtr2, encodeToBase64StringOrigJpeg->c_str());
    // fclose(fPtr2);
    // dumpData(buffer_base, 2, mPortrait->mOrigJpegSize, mBokehSize.capture_w,
    // mBokehSize.capture_h, mPortrait->mCapFrameNumber, "origJpeg");
}

int SprdCamera3Portrait::jpeg_callback_thread_init(void *p_data) {
    int ret = NO_ERROR;
    pthread_attr_t attr;

    SprdCamera3Portrait *obj = (SprdCamera3Portrait *)p_data;

    if (!obj) {
        HAL_LOGE("obj null error");
        return -1;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&obj->mJpegCallbackThread, &attr,
                         jpeg_callback_thread_proc, (void *)obj);
    pthread_attr_destroy(&attr);
    if (ret) {
        HAL_LOGE("fail to send init msg");
    }

    HAL_LOGD("init state=%d", ret);
    return ret;
}

void *SprdCamera3Portrait::jpeg_callback_thread_proc(void *p_data) {
    int ret = 0;
    SprdCamera3Portrait *obj = (SprdCamera3Portrait *)p_data;

    if (!obj) {
        HAL_LOGE("obj=%p", obj);
        return NULL;
    }

    Mutex::Autolock l(obj->mJpegCallbackLock);
    if (obj->mFlushing) {
        return NULL;
    }
    bool save_param = false;
    int rc = NO_ERROR;
    unsigned char *result_buffer_addr = NULL;
    const camera3_stream_buffer_t *output_buffers = obj->mJpegOutputBuffers;
    uint32_t result_buffer_size = ADP_WIDTH(*output_buffers->buffer);
    rc = obj->map(output_buffers->buffer, (void **)&result_buffer_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map output_buffers buffer");
        return NULL;
    }
    uint32_t jpeg_size =
        obj->getJpegSize(result_buffer_addr, result_buffer_size);
#ifdef YUV_CONVERT_TO_JPEG
    if ((obj->mCaptureThread->mBokehResult == true) &&
        (obj->mOrigJpegSize > 0)) {
        save_param = true;
    }
#else
    if (obj->mCaptureThread->mBokehResult == true) {
        save_param = true;
    }
#endif
    /*
    if (save_param && mPortrait->mBokehMode == CAM_DUAL_PORTRAIT_MODE) {
        obj->mCaptureThread->saveCaptureBokehParams(result_buffer_addr,
                                                    obj->mjpegSize, jpeg_size);
        obj->dumpCaptureBokeh(result_buffer_addr, jpeg_size);
    } else
    */
    { obj->setJpegSize((char *)result_buffer_addr, obj->mjpegSize, jpeg_size); }
    obj->unmap(output_buffers->buffer);
#ifdef YUV_CONVERT_TO_JPEG
    obj->pushBufferList(obj->mLocalBuffer, obj->m_pDstJpegBuffer,
                        obj->mLocalBufferNumber, obj->mLocalBufferList);
// obj->pushBufferList(obj->mLocalBuffer, obj->m_pDstGDepthOriJpegBuffer,
//                  obj->mLocalBufferNumber, obj->mLocalBufferList);

#endif
    obj->CallBackResult(obj->mCapFrameNumber, CAMERA3_BUFFER_STATUS_OK);
    obj->mCaptureThread->mReprocessing = false;
    obj->mIsCapturing = false;
    return NULL;
}
camera_metadata_t *
SprdCamera3Portrait::reConfigResultMeta(camera_metadata_t *meta) {
    CameraMetadata *camMetadata = new CameraMetadata(meta);
    int lpt_return_val = 0;
    mBokehAlgo->getLightPortraitParam(&lpt_return_val);
    HAL_LOGV("lpt_return_val %d", lpt_return_val);
    camMetadata->update(ANDROID_SPRD_LIGHTPORTRAITTIPS, &lpt_return_val, 1);
    meta = camMetadata->release();
    delete camMetadata;
    return meta;
}
};
