#define LOG_TAG "BokehCamera"

#include <log/log.h>
#include "BokehCamera.h"
#include <android/hardware/graphics/common/1.0/types.h>

using android::hardware::graphics::common::V1_0::BufferUsage;
using namespace sprdcamera;
using namespace std;

#ifdef CONFIG_SUPPORT_GDEPTH
#undef CONFIG_SUPPORT_GDEPTH
#endif

#define BOKEH_THREAD_TIMEOUT (50e6)
#define CLEAR_PRE_FRAME_UNMATCH_TIMEOUT (2000) // 2s
#define LIB_BOKEH_PATH "libsprdbokeh.so"
#define LIB_DEPTH_PATH "libsprddepth.so"
#define LIB_BOKEH_PREVIEW_PATH "libbokeh_depth.so"

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
#define DEFAULT_PREVIEW_WIDTH (960)
#define DEFAULT_PREVIEW_HEIGHT (720)
#define DEFAULT_CAPTURE_WIDTH (3264)
#define DEFAULT_CAPTURE_HEIGHT (2448)

#define BOKEH_PREVIEW_PARAM_LIST (10)

/* refocus api error code */
#define ALRNB_ERR_SUCCESS 0x00

#define BOKEH_TIME_DIFF (100e6)
#define PENDINGTIME (1000000)
#define PENDINGTIMEOUT (5000000000)
#define MASTER_ID 0
#define EXTEND_MODE_NUM 2
#define STREAM_NUM 5
BokehCamera *mBokehCamera = NULL;

const uint8_t kavailableExtendedSceneModes[] = {
    ANDROID_CONTROL_EXTENDED_SCENE_MODE_DISABLED,
    ANDROID_CONTROL_EXTENDED_SCENE_MODE_BOKEH_STILL_CAPTURE,
    ANDROID_CONTROL_EXTENDED_SCENE_MODE_BOKEH_CONTINUOUS};

// Error Check Macros
#define CHECK_CAPTURE()                                                        \
    if (!mBokehCamera) {                                                       \
        HAL_LOGE("Error getting capture ");                                    \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_CAPTURE_ERROR()                                                  \
    if (!mBokehCamera) {                                                       \
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

camera3_callback_ops BokehCamera::callback_ops_main = {
    .process_capture_result = BokehCamera::process_capture_result_main,
    .notify = BokehCamera::notifyMain};

camera3_callback_ops BokehCamera::callback_ops_aux = {
    .process_capture_result = BokehCamera::process_capture_result_aux,
    .notify = BokehCamera::notifyAux};

BokehCamera::BokehCamera(shared_ptr<Configurator> cfg,
                         const vector<int> &physicalIds)
    : CameraDevice_3_5(cfg->getCameraId()), mPhysicalIds(physicalIds) {
    HAL_LOGI(" E");
    // m_nPhyCameras = 2; // m_nPhyCameras should always be 2 with dual camera
    // mode
    std::vector<int> mSensorIds = cfg->getSensorIds();
    m_nPhyCameras = mSensorIds.size(); // BokehCamera's physical camera num.
    setupPhysicalCameras(mSensorIds);
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    m_VirtualCamera.id = mCameraId;
    mStaticMetadata = NULL;
    mCaptureStreamsNum = 0;
    mCallbackStreamsNum = 0;
    mPreviewStreamsNum = 0;
    mVideoStreamsNum = -1;
    mCapFrameNumber = -1;
    mPendingRequest = 0;
    mSavedCapStreams = NULL;
    mCapFrameNumber = -1;
    mPrevBlurFrameNumber = 0;
    mPrevFrameNumber = 0;
    mIsCapturing = false;
    mSnapshotResultReturn = false;
    mjpegSize = 0;
    capture_result_timestamp = 0;
    mFlushing = false;
    mVcmSteps = 0;
    mVcmStepsFixed = 0;
    mApiVersion = BOKEH_API_MODE;
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
    mDepthStatus = BOKEH_DEPTH_INVALID;
    mDepthTrigger = BOKEH_TRIGGER_FNUM;
    mReqTimestamp = 0;
    mLastOnlieVcm = 0;
    mBokehAlgo = NULL;
    mIsHdrMode = false;
    mIsCapDepthFinish = false;
    mHdrSkipBlur = false;
    mHdrCallbackCnt = 0;
    mAfstate = 0;
    mSavedRequestList.clear();
    mSavedVideoReqList.clear();
    mCaptureThread = new BokehCaptureThread();
    mPreviewMuxerThread = new PreviewMuxerThread();
    mDepthMuxerThread = new DepthMuxerThread();
    memset(&mScaleInfo, 0, sizeof(struct img_frm));
    memset(&mBokehSize, 0, sizeof(BokehSize));
    memset(&mBokehStream, 0, sizeof(mBokehStream));
    memset(mLocalBuffer, 0, sizeof(new_mem_t) * LOCAL_BUFFER_NUM);
    memset(&mLocalScaledBuffer, 0, sizeof(new_mem_t));
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(mMainStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(mFaceInfo, 0, sizeof(int32_t) * 4);
#ifdef CONFIG_SPRD_FB_VDSP_SUPPORT
    memset(&mPerfectskinlevel, 0, sizeof(faceBeautyLevels));
#else
    memset(&mPerfectskinlevel, 0, sizeof(face_beauty_levels));
#endif

    memset(&mThumbReq, 0, sizeof(multi_request_saved_t));
    memset(&mBokehDepthBuffer, 0, sizeof(BokehDepthBuffer));
    memset(&mOtpData, 0, sizeof(OtpData));
    memset(&mbokehParm, 0, sizeof(bokeh_params));
    memset(&mReqCapType, 0, sizeof(req_type));

    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    mPrevFrameNotifyList.clear();
    mMetadataReqList.clear();
    mExtendMode = 0;
    physicIds = new uint8_t[mPhysicalIds.size() * 2];
    mConflictDevices = NULL;
    mCameraNum = 0;
    mBokehDev = NULL;
    mSnapshotMainReturn = false;
    mSnapshotAuxReturn = false;
    mBokehMode = 0;
    mDoPortrait = 0;
    mlimited_infi = 0;
    mlimited_macro = 0;
    maxCapWidth = DEFAULT_CAPTURE_WIDTH;
    maxCapHeight = DEFAULT_CAPTURE_HEIGHT;
    maxPrevWidth = DEFAULT_PREVIEW_WIDTH;
    maxPrevHeight = DEFAULT_PREVIEW_WIDTH;
    mVideoPrevShare = false;
    far = 0;
    near = 0;
    sn_trim_flag = true;
    trim_W = 0;
    trim_H = 0;
    HAL_LOGI("X");
}

BokehCamera::~BokehCamera() {
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
    if (mConflictDevices) {
        for (int i = 0; i < mCameraNum - 1; i++)
            delete[] mConflictDevices[i];
        delete[] mConflictDevices;
        mConflictDevices = NULL;
    }

    if (physicIds) {
        delete[] physicIds;
        physicIds = NULL;
    }

    HAL_LOGI("X");
}

shared_ptr<ICameraCreator> BokehCamera::getCreator() {
    return shared_ptr<ICameraCreator>(new BokehCreator());
}

int BokehCamera::openCameraDevice() {
    int rc = NO_ERROR;
    uint8_t phyId = 0;

    HAL_LOGI(" E");
    hw_device_t *hw_dev[m_nPhyCameras];
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
        hw->setMasterId(MASTER_ID);

        rc = hw->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hw;
            closeCameraDevice();
            return rc;
        }
#ifdef CONFIG_BOKEH_CROP
        if (i == 0) {
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

    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.bokeh.api.version", prop, "0");
    mApiVersion = atoi(prop);
    HAL_LOGD("api version %d", mApiVersion);
    if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {
#ifdef CONFIG_SPRD_BOKEH_SUPPORT
        mBokehAlgo = new SprdBokehAlgo();
        mLocalBufferNumber = LOCAL_BUFFER_NUM;
#endif
        mLocalBufferNumber = LOCAL_BUFFER_NUM;
    }
    HAL_LOGI("X,mLocalBufferNumber=%d", mLocalBufferNumber);
    mBokehDev = reinterpret_cast<camera3_device_t *>(&common);
    return rc;
}

int BokehCamera::getCameraInfo(camera_info_t *info) {
    if (!info)
        return -EINVAL;
    int rc = NO_ERROR;
    int camera_id;
    int32_t img_size = 0;

    if (mStaticMetadata)
        free_camera_metadata(mStaticMetadata);

    HAL_LOGI("BokehCamera mCameraId = %d", mCameraId);
    camera_id = 0;
    SprdCamera3Setting::getSensorStaticInfo(camera_id);
    SprdCamera3Setting::initDefaultParameters(camera_id);
    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }
    CameraMetadata metadata = clone_camera_metadata(mStaticMetadata);
    setBokehCameraTags(metadata);
    mStaticMetadata = metadata.release();

    SprdCamera3Setting::getCameraInfo(camera_id, info);
    mCameraNum = SprdCamera3Factory::get_number_of_cameras();
    HAL_LOGD("mCameraNum %d", mCameraNum);
    if (mCameraNum - 1) {
        mConflictDevices = new char *[mCameraNum - 1];
        int id = 0, i = 0;
        while (id < mCameraNum) {
            if (id != mCameraId) {
                mConflictDevices[i] = new char[5];
                size_t s = snprintf(mConflictDevices[i], 4, "%d", id);
                mConflictDevices[i][s] = '\0';
                HAL_LOGD("mConflictDevices[%d] %s", i, mConflictDevices[i]);
                i++;
            }
            id++;
        }
    }
    info->device_version = CAMERA_DEVICE_API_VERSION_3_5;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices = mConflictDevices;
    info->conflicting_devices_length = mCameraNum - 1;
    info->resource_cost = 100;
    HAL_LOGI("X rc = %d", rc);

    return rc;
}

int BokehCamera::initialize(const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    HAL_LOGI("E");
    mPendingRequest = 0;
    mBokehSize.capture_w = 0;
    mBokehSize.capture_h = 0;
    mPreviewStreamsNum = 0;
    mCaptureStreamsNum = 1;
    mCallbackStreamsNum = 2;
    mVideoStreamsNum = -1;
    mCaptureThread->mReprocessing = false;
    mIsCapturing = false;
    mSnapshotResultReturn = false;
    mCapFrameNumber = -1;
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
    mVcmSteps = 0;
    mOtpData.otp_size = 0;
    mOtpData.otp_type = 0;
    mJpegOrientation = 0;
    mIsHdrMode = false;
    mIsCapDepthFinish = false;
    mHdrSkipBlur = false;
    mHdrCallbackCnt = 0;
    mMetadataReqList.clear();
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

    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_BOKEH_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiMain);
    SprdCamera3MultiBase::initialize(MODE_BOKEH, hwiMain);

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_BOKEH_DEPTH];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error aux camera while initialize !! ");
        return rc;
    }
    memset(mAuxStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(mMainStreams, 0,
           sizeof(camera3_stream_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(&mCaptureThread->mSavedCapRequest, 0,
           sizeof(camera3_capture_request_t));
    memset(&mCaptureThread->mSavedCapReqStreamBuff, 0,
           sizeof(camera3_stream_buffer_t));
    mCaptureThread->mSavedCapReqsettings = NULL;
    mCaptureThread->mSavedResultBuff = NULL;
    mCaptureThread->mSavedOneResultBuff = NULL;

    mCaptureThread->mCallbackOps = callback_ops;
    mCaptureThread->mDevmain = &m_pPhyCamera[CAM_BOKEH_MAIN];

    uint8_t otpType = SprdCamera3Setting::s_setting[0].otpInfo.otp_type;
    int otpSize = SprdCamera3Setting::s_setting[0].otpInfo.otp_size;
    HAL_LOGD("otpType %d, otp_size %d", otpType, otpSize);
    if (mOtpData.otp_exist == false && otpSize > 0) {
        mOtpData.otp_exist = true;
        mOtpData.otp_type = otpType;
        mOtpData.otp_size = otpSize;
        memcpy(mOtpData.otp_data,
               SprdCamera3Setting::s_setting[0].otpInfo.otp_data,
               mOtpData.otp_size);
    }
    HAL_LOGI("X");
    return rc;
}

int BokehCamera::closeCameraDevice() {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    HAL_LOGI("E");
    Mutex::Autolock jcl(mJpegCallbackLock);
    mFlushing = true;
    preClose();
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
    HAL_LOGD("close camera device end");

    mReqTimestamp = 0;
    mPrevFrameNumber = 0;
    mCapFrameNumber = -1;
    freeLocalBuffer();
    mSavedRequestList.clear();
    mSavedVideoReqList.clear();
    mLocalBufferList.clear();
    mMetadataList.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mNotifyListMain.clear();
    mNotifyListAux.clear();
    mPrevFrameNotifyList.clear();
    mMetadataReqList.clear();
    HAL_LOGD("deinit mBokehAlgo");
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
    }

exit:
    if (mBokehAlgo) {
        delete mBokehAlgo;
        mBokehAlgo = NULL;
    }

    HAL_LOGI("X, rc: %d", rc);
    return rc;
}

int BokehCamera::configure_streams(
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();
    if (stream_list == NULL || stream_list->streams[0] == NULL) {
        HAL_LOGE("NULL stream_list");
        return -EINVAL;
    }

    rc = mBokehCamera->configureStreams(stream_list);

    HAL_LOGI(" X");

    return rc;
}

const camera_metadata_t *
BokehCamera::construct_default_request_settings(int type) {
    const camera_metadata_t *rc;

    HAL_LOGD("E");
    if (!mBokehCamera) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mBokehCamera->constructDefaultRequestSettings(type);

    HAL_LOGD("X, rc: %p", rc);

    return rc;
}

int BokehCamera::process_capture_request(camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGD("idx:%d", request->frame_number);
    CHECK_CAPTURE_ERROR();
    rc = mBokehCamera->processCaptureRequest(request);

    return rc;
}

void BokehCamera::dump(__unused int fd) {
    HAL_LOGI("E");
    CHECK_CAPTURE();
    HAL_LOGI("X");
}

int BokehCamera::flush() {
    int rc = 0;

    HAL_LOGI(" E");
    CHECK_CAPTURE_ERROR();
    Mutex::Autolock l(mFlushLock);
    Mutex::Autolock jcl(mJpegCallbackLock);
    mFlushing = true;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_BOKEH_MAIN].dev);
    if (rc != 0)
        return rc;

    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_BOKEH_DEPTH].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_BOKEH_DEPTH].dev);

    mFlushing = false;
    HAL_LOGI("X");
    return rc;
}

int BokehCamera::isStreamCombinationSupported(
    const camera_stream_combination_t *comb) {
    int ret = 0;
    HAL_LOGI("E");
    if (!comb) {
        HAL_LOGE("NULL stream combination");
        return -EINVAL;
    }

    if (comb->num_streams < 1 ||
        comb->num_streams > REAL_BOKEH_MAX_NUM_STREAMS) {
        HAL_LOGE("Bad number of streams num: %d", comb->num_streams);
        return -EINVAL;
    }
    if (comb->operation_mode != 0) { // CAMERA3_STREAM_CONFIGURATION_NORMAL_MODE
        HAL_LOGE("error operation_mode %d", comb->operation_mode);
        return -EINVAL;
    }
    camera3_stream_configuration_t streamList = {
        comb->num_streams, /*streams*/ nullptr, comb->operation_mode,
        /*session_parameters*/ nullptr};
    streamList.streams = new camera3_stream_t *[comb->num_streams];
    camera3_stream_t *streamBuffer = new camera3_stream_t[comb->num_streams];
    if (streamList.streams == NULL) {
        HAL_LOGD("NULL stream list");
        ret = -EINVAL;
        goto exit;
    }
    for (uint8_t i = 0; i < comb->num_streams; i++) {
        streamBuffer[i] = {comb->streams[i].stream_type,
                           comb->streams[i].width,
                           comb->streams[i].height,
                           comb->streams[i].format,
                           comb->streams[i].usage,
                           /*max_buffers*/ 0,
                           /*priv*/ nullptr,
                           comb->streams[i].data_space,
                           comb->streams[i].rotation,
                           comb->streams[i].physical_camera_id,
                           /*reserved*/ {nullptr}};
        streamList.streams[i] = &streamBuffer[i];
        ret = validateStream(streamList.streams[i]);
        if (ret != 0) {
            goto exit;
        }
    }
exit:
    HAL_LOGI("X, rc: %d", ret);
    if (streamBuffer)
        delete[] streamBuffer;
    if (streamList.streams)
        delete[] streamList.streams;
    return ret;
}

void BokehCamera::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mBokehCamera->processCaptureResultMain(result);
}

void BokehCamera::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGD("idx:%d", result->frame_number);
    CHECK_CAPTURE();
    mBokehCamera->processCaptureResultAux(result);
}

int BokehCamera::configureStreams(camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");
    int rc = 0;
    camera3_stream_t *pmainStreams[REAL_BOKEH_MAX_NUM_STREAMS];
    camera3_stream_t *pauxStreams[REAL_BOKEH_MAX_NUM_STREAMS];
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *snapStream = NULL;
    mBokehSize.depth_prev_out_w = DEPTH_OUTPUT_WIDTH;
    mBokehSize.depth_prev_out_h = DEPTH_OUTPUT_HEIGHT;
    mBokehSize.depth_snap_out_w = DEPTH_SNAP_OUTPUT_WIDTH;
    mBokehSize.depth_snap_out_h = DEPTH_SNAP_OUTPUT_HEIGHT;
    mVideoPrevShare = false;
    far = 0;
    near = 0;
    mBokehSize.depth_jepg_size = 0;
    mFlushing = false;
    mhasCallbackStream = false;
    mReqTimestamp = 0;
    mLastOnlieVcm = 0;
    mIsCapDepthFinish = false;
    mHdrSkipBlur = false;
    sn_trim_flag = true;
    mReqCapType.frame_number = -1;
    mReqCapType.reqType = 0;
    struct af_relbokeh_oem_data af_relbokeh_info;
    bool prevStreamMalloc = false, snapStreamMalloc = false;

    Mutex::Autolock l(mLock);
    preClose();
    mPreviewMuxerThread->mPreviewMuxerMsgList.clear();
    mCaptureThread->mCaptureMsgList.clear();
    mDepthMuxerThread->mDepthMuxerMsgList.clear();

    mAfstate = 0;
    mbokehParm.f_number = 2;
    memset(pmainStreams, 0,
           sizeof(camera3_stream_t *) * REAL_BOKEH_MAX_NUM_STREAMS);
    memset(pauxStreams, 0,
           sizeof(camera3_stream_t *) * REAL_BOKEH_MAX_NUM_STREAMS);
    bzero(&af_relbokeh_info, sizeof(struct af_relbokeh_oem_data));
    mBokehStream.video_w = 0;
    mBokehStream.video_h = 0;
    mVideoStreamsNum = -1;

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        HAL_LOGD("framework configurestreams %d, streamtype:%d, format:%d, "
                 "width:%d, "
                 "height:%d priv %p usage 0x%x max_buffers %d, data_space %d, "
                 "rotation %d, %s",
                 i, stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->format,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height, stream_list->streams[i]->priv,
                 stream_list->streams[i]->usage,
                 stream_list->streams[i]->max_buffers,
                 stream_list->streams[i]->data_space,
                 stream_list->streams[i]->rotation,
                 stream_list->streams[i]->physical_camera_id);
        rc = validateStream(stream_list->streams[i]);
        if (rc != 0) {
            HAL_LOGE("dont support this stream_list");
            return rc;
        }
    }
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamTypes(stream_list->streams[i]);
        HAL_LOGD("requestStreamType %d", requestStreamType);
        if (requestStreamType == PREVIEW_STREAM && (previewStream == NULL)) {
            HAL_LOGD("PreviewStream index %d", mPreviewStreamsNum);
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
            pmainStreams[mPreviewStreamsNum] =
                &mMainStreams[mPreviewStreamsNum];
            pauxStreams[mPreviewStreamsNum] = &mAuxStreams[mPreviewStreamsNum];
        } else if (requestStreamType == SNAPSHOT_STREAM && snapStream == NULL) {
            HAL_LOGD("CaptureStream index %d", mCaptureStreamsNum);
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

            mMainStreams[mCallbackStreamsNum].max_buffers = 1;
            mMainStreams[mCallbackStreamsNum].width = mBokehSize.capture_w;
            mMainStreams[mCallbackStreamsNum].height = mBokehSize.capture_h;
            mMainStreams[mCallbackStreamsNum].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mMainStreams[mCallbackStreamsNum].usage =
                (uint64_t)BufferUsage::CPU_READ_OFTEN;
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
                (uint64_t)BufferUsage::CPU_READ_OFTEN;
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
        } else if (requestStreamType == VIDEO_STREAM) {
            mVideoStreamsNum = i;
            HAL_LOGD("mVideoStreamsNum %d", mVideoStreamsNum);
            (stream_list->streams[i])->max_buffers = 7;
            mBokehStream.video_w = (stream_list->streams[i])->width;
            mBokehStream.video_h = (stream_list->streams[i])->height;
        }
    }
    HAL_LOGD("mVideoStreamsNum %d", mVideoStreamsNum);
    if (previewStream == NULL) {
        HAL_LOGD("PreviewStream index %d", mPreviewStreamsNum);
        prevStreamMalloc = true;
        previewStream = new camera3_stream_t;
        memcpy(previewStream, (camera3_stream_t *)(stream_list->streams[0]),
               sizeof(camera3_stream_t));
        if (mVideoStreamsNum >= 0) {
            mVideoPrevShare = true;
            mBokehSize.preview_w = mBokehStream.video_w;
            mBokehSize.preview_h = mBokehStream.video_h;
        } else {
            mBokehSize.preview_w = maxPrevWidth;
            mBokehSize.preview_h = maxPrevHeight;
        }
        HAL_LOGD("mBokehSize %d, %d", mBokehSize.preview_w,
                 mBokehSize.preview_h);
        getDepthImageSize(mBokehSize.preview_w, mBokehSize.preview_h,
                          &mBokehSize.depth_prev_sub_w,
                          &mBokehSize.depth_prev_sub_h, PREVIEW_STREAM);

        mMainStreams[mPreviewStreamsNum] = *previewStream;
        mAuxStreams[mPreviewStreamsNum] = *previewStream;
        (mAuxStreams[mPreviewStreamsNum]).width = mBokehSize.depth_prev_sub_w;
        (mAuxStreams[mPreviewStreamsNum]).height = mBokehSize.depth_prev_sub_h;
        (mMainStreams[mPreviewStreamsNum]).data_space = HAL_DATASPACE_UNKNOWN;
        (mAuxStreams[mPreviewStreamsNum]).data_space = HAL_DATASPACE_UNKNOWN;
        (mMainStreams[mPreviewStreamsNum]).format =
            HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
        (mAuxStreams[mPreviewStreamsNum]).format =
            HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
        (mMainStreams[mPreviewStreamsNum]).usage = 0x900;
        (mAuxStreams[mPreviewStreamsNum]).usage = 0x900;
        pmainStreams[mPreviewStreamsNum] = &mMainStreams[mPreviewStreamsNum];
        pauxStreams[mPreviewStreamsNum] = &mAuxStreams[mPreviewStreamsNum];
    }
    if (snapStream == NULL) {
        HAL_LOGD("CaptureStream index %d", mCaptureStreamsNum);
        snapStreamMalloc = true;
        snapStream = new camera3_stream_t;
        memcpy(snapStream, (camera3_stream_t *)(stream_list->streams[0]),
               sizeof(camera3_stream_t));
        snapStream->width = maxCapWidth;
        snapStream->height = maxCapHeight;
        mBokehSize.capture_w = snapStream->width;
        mBokehSize.capture_h = snapStream->height;
        mBokehSize.callback_w = mBokehSize.capture_w;
        mBokehSize.callback_h = mBokehSize.capture_h;
        getDepthImageSize(mBokehSize.capture_w, mBokehSize.capture_h,
                          &mBokehSize.depth_snap_sub_w,
                          &mBokehSize.depth_snap_sub_h, SNAPSHOT_STREAM);
        mMainStreams[mCaptureStreamsNum] = *snapStream;
        (mMainStreams[mCaptureStreamsNum]).format = HAL_PIXEL_FORMAT_BLOB;
        snapStream->format = HAL_PIXEL_FORMAT_BLOB;
        (mMainStreams[mCaptureStreamsNum]).data_space = HAL_DATASPACE_JFIF;
        mAuxStreams[mCaptureStreamsNum] = *snapStream;
        (mAuxStreams[mCaptureStreamsNum]).width = mBokehSize.depth_snap_sub_w;
        (mAuxStreams[mCaptureStreamsNum]).height = mBokehSize.depth_snap_sub_h;
        (mAuxStreams[mCaptureStreamsNum]).data_space = HAL_DATASPACE_JFIF;
        (mAuxStreams[mCaptureStreamsNum]).usage = 0x3;
        pmainStreams[mCaptureStreamsNum] = &mMainStreams[mCaptureStreamsNum];
        HAL_LOGD("CallbackStream index %d", mCallbackStreamsNum);
        mMainStreams[mCallbackStreamsNum].max_buffers = 1;
        mMainStreams[mCallbackStreamsNum].width = mBokehSize.capture_w;
        mMainStreams[mCallbackStreamsNum].height = mBokehSize.capture_h;
        mMainStreams[mCallbackStreamsNum].format =
            HAL_PIXEL_FORMAT_YCbCr_420_888;
        mMainStreams[mCallbackStreamsNum].usage = 0x3;
        mMainStreams[mCallbackStreamsNum].stream_type = CAMERA3_STREAM_OUTPUT;
        mMainStreams[mCallbackStreamsNum].data_space = HAL_DATASPACE_JFIF;
        mMainStreams[mCallbackStreamsNum].rotation = snapStream->rotation;
        pmainStreams[mCallbackStreamsNum] = &mMainStreams[mCallbackStreamsNum];
        mAuxStreams[mCallbackStreamsNum].max_buffers = 1;
        mAuxStreams[mCallbackStreamsNum].width = mBokehSize.depth_snap_sub_w;
        mAuxStreams[mCallbackStreamsNum].height = mBokehSize.depth_snap_sub_h;
        mAuxStreams[mCallbackStreamsNum].format =
            HAL_PIXEL_FORMAT_YCbCr_420_888;
        mAuxStreams[mCallbackStreamsNum].stream_type = CAMERA3_STREAM_OUTPUT;
        mAuxStreams[mCallbackStreamsNum].data_space = HAL_DATASPACE_JFIF;
        mAuxStreams[mCallbackStreamsNum].rotation = snapStream->rotation;
        mAuxStreams[mCallbackStreamsNum].usage = 0x3;
        pauxStreams[mCaptureStreamsNum] = &mAuxStreams[mCallbackStreamsNum];
        HAL_LOGD("snapStream w %d h %d format %d", snapStream->width,
                 snapStream->height, snapStream->format);
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
    initPrevDepthBufferFlag();

    camera3_stream_configuration mainconfig;
    mainconfig = *stream_list;
    mainconfig.num_streams = REAL_BOKEH_MAX_NUM_STREAMS;
    mainconfig.streams = pmainStreams;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;

    camera3_stream_configuration auxconfig;
    auxconfig = *stream_list;
    auxconfig.num_streams = REAL_BOKEH_MAX_NUM_STREAMS - 1;
    auxconfig.streams = pauxStreams;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_BOKEH_DEPTH].hwi;
    HAL_LOGD("AuxConfig Num %d", auxconfig.num_streams);
    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_BOKEH_DEPTH].dev,
                                   &auxconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure aux streams!!");
        goto exit;
    }
    HAL_LOGD("MainConfig Num %d", mainconfig.num_streams);
    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_BOKEH_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
        goto exit;
    }
    for (size_t i = 0; i < mainconfig.num_streams; i++) {
        HAL_LOGD(
            "mainconfig configurestreams %d, streamtype:%d, format:%d, "
            "width:%d, "
            "height:%d priv %p usage 0x%x max_buffers %d, data_space %d, "
            "rotation %d",
            i, mainconfig.streams[i]->stream_type,
            mainconfig.streams[i]->format, mainconfig.streams[i]->width,
            mainconfig.streams[i]->height, mainconfig.streams[i]->priv,
            mainconfig.streams[i]->usage, mainconfig.streams[i]->max_buffers,
            mainconfig.streams[i]->data_space, mainconfig.streams[i]->rotation);
    }

    memcpy(previewStream, &mMainStreams[mPreviewStreamsNum],
           sizeof(camera3_stream_t));
    HAL_LOGD("previewStream  max buffers %d", previewStream->max_buffers);
    previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;

    memcpy(snapStream, &mMainStreams[mCaptureStreamsNum],
           sizeof(camera3_stream_t));
    snapStream->width = mBokehSize.capture_w;
    snapStream->height = mBokehSize.capture_h;

    mCaptureThread->run(String8::format("Bokeh-Cap").string());
    mPreviewMuxerThread->run(String8::format("Bokeh-Prev").string());
    mDepthMuxerThread->run(String8::format("depth-prev").string());

    mbokehParm.sel_x = mBokehSize.preview_w / 2;
    mbokehParm.sel_y = mBokehSize.preview_h / 2;
    mbokehParm.capture_x = mBokehSize.preview_w / 2;
    mbokehParm.capture_y = mBokehSize.preview_h / 2;
    hwiMain->camera_ioctrl(CAMERA_IOCTRL_GET_REBOKE_DATA, &af_relbokeh_info,
                           NULL);
    memcpy(&mbokehParm.relbokeh_oem_data, &af_relbokeh_info,
           sizeof(af_relbokeh_oem_data));
    HAL_LOGD("ALG mBokehAlgo->initParam start");
    rc = mBokehAlgo->initParam(&mBokehSize, &mOtpData,
                               mCaptureThread->mAbokehGallery);
    HAL_LOGD("ALG mBokehAlgo->initParam end");
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to initParam");
    }

    setPrevBokehSupport();
    if (mPreviewMuxerThread->isRunning()) {
        mPreviewMuxerThread->requestInit();
    }

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int type = getStreamTypes(stream_list->streams[i]);
        if (type == PREVIEW_STREAM || type == VIDEO_STREAM) {
            stream_list->streams[i]->max_buffers = previewStream->max_buffers;
        } else if (type == SNAPSHOT_STREAM) {
            stream_list->streams[i]->max_buffers = snapStream->max_buffers;
        }
        HAL_LOGD("configurestreams %d, streamtype:%d, format:%d, width:%d, "
                 "height:%d priv %p usage 0x%x max_buffers %d, data_space %d, "
                 "rotation %d",
                 i, stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->format,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height, stream_list->streams[i]->priv,
                 stream_list->streams[i]->usage,
                 stream_list->streams[i]->max_buffers,
                 stream_list->streams[i]->data_space,
                 stream_list->streams[i]->rotation);
    }
    HAL_LOGI("mum_streams:%d,%d,depth_cap (%d,%d),cap (%d,%d),"
             "depth_prev (%d,%d), prev (%d,%d), video (%d, %d), "
             "mIsSupportPBokeh:%d,",
             mainconfig.num_streams, auxconfig.num_streams,
             mBokehSize.depth_snap_sub_w, mBokehSize.depth_snap_sub_h,
             mBokehSize.capture_w, mBokehSize.capture_h,
             mBokehSize.depth_prev_sub_w, mBokehSize.depth_prev_sub_h,
             mBokehSize.preview_w, mBokehSize.preview_h, mBokehStream.video_w,
             mBokehStream.video_h, mIsSupportPBokeh);
exit:
    HAL_LOGI("X rc: %d.", rc);
    if (prevStreamMalloc) {
        delete previewStream;
    }
    if (snapStreamMalloc) {
        delete snapStream;
    }
    return rc;
}

const camera_metadata_t *
BokehCamera::constructDefaultRequestSettings(int type) {
    const camera_metadata_t *fwk_metadata = NULL;
    SprdCamera3HWI *hw = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hw_depth = m_pPhyCamera[CAM_BOKEH_DEPTH].hwi;
    Mutex::Autolock l(mLock);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[CAM_BOKEH_MAIN].dev, type);
    HAL_LOGD("fwk_metadata %p", fwk_metadata);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }

    /*
     * [CTS] LogicalCameraDeviceTest#testDefaultFov
     *
     * logical camera must provide android.scaler.cropRegion
     */
    {
        CameraMetadata data(const_cast<camera_metadata_t *>(fwk_metadata));

        int32_t cropRegion[4] = {0, 0, maxCapWidth, maxCapHeight};
        data.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);
        HAL_LOGD("update crop region to (0, 0, %d, %d) for type %d",
                 maxCapWidth, maxCapHeight, type);

        data.release();
    }

    HAL_LOGD("X");

    return fwk_metadata;
}

int BokehCamera::processCaptureRequest(camera3_capture_request_t *request) {
    int rc = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_BOKEH_DEPTH].hwi;
    CameraMetadata metaSettingsMain, metaSettingsAux;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    buffer_handle_t *new_buffer = NULL;
    camera3_stream_buffer_t out_streams_main[REAL_BOKEH_MAX_NUM_STREAMS];
    camera3_stream_buffer_t out_streams_aux[REAL_BOKEH_MAX_NUM_STREAMS];
    bool is_captureing = false;
    uint32_t tagCnt = 0;
    camera3_stream_t *preview_stream = NULL;
    cmr_u16 snsW, snsH;
    req_type curReqType = {.frame_number = (int)request->frame_number,
                           .reqType = 0};
    size_t prevReqNum = 0;
    bzero(out_streams_main,
          sizeof(camera3_stream_buffer_t) * REAL_BOKEH_MAX_NUM_STREAMS);
    bzero(out_streams_aux,
          sizeof(camera3_stream_buffer_t) * REAL_BOKEH_MAX_NUM_STREAMS);

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }
    HAL_LOGD("frame_number:%d,num_output_buffers=%d", request->frame_number,
             request->num_output_buffers);
    metaSettingsMain = request->settings;
    metaSettingsAux = request->settings;

    saveRequest(request);

#ifdef CONFIG_BOKEH_HDR_SUPPORT
    // only sprd bokeh api support hdr
    if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {
        if (metaSettingsMain.exists(ANDROID_CONTROL_SCENE_MODE)) {
            uint8_t scene_mode =
                metaSettingsMain.find(ANDROID_CONTROL_SCENE_MODE).data.u8[0];
            if (scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR)
                mIsHdrMode = true;
            else
                mIsHdrMode = false;
        }
    }

    if (mIsHdrMode) {
        uint8_t value = ANDROID_CONTROL_SCENE_MODE_HDR;
        // main sensor need normal + hdr frame, so enable hdr mode
        metaSettingsMain.update(ANDROID_CONTROL_SCENE_MODE, &value, 1);
        // aux sensor just need normal + hdr frame, so enable hdr mode too
        metaSettingsAux.update(ANDROID_CONTROL_SCENE_MODE, &value, 1);
    }
#endif

    for (size_t i = 0; i < req->num_output_buffers; i++) {
        camera3_stream_t *stream = request->output_buffers[i].stream;
        HAL_LOGD("request %d, streamType:%d, width:%d, height:%d, format:%d,"
                 "usage:0x%x, max_buffers:%d, data_space:%d, rotation:%d, "
                 "priv:%p, phyId:%s, buffer: %p",
                 i, stream->stream_type, stream->width, stream->height,
                 stream->format, stream->usage, stream->max_buffers,
                 stream->data_space, stream->rotation, stream->priv,
                 stream->physical_camera_id, request->output_buffers[i].buffer);
        int requestStreamType =
            getStreamTypes(request->output_buffers[i].stream);
        HAL_LOGD("streamtype:%d", requestStreamType);
        if (requestStreamType == SNAPSHOT_STREAM) {
            mIsCapturing = true;
            is_captureing = mIsCapturing;
            mSnapshotMainReturn = false;
            mSnapshotAuxReturn = false;
            curReqType.reqType |= 2;
            HAL_LOGD("curReqType %d %d", curReqType.frame_number,
                     curReqType.reqType);
            if (metaSettingsMain.exists(ANDROID_JPEG_ORIENTATION)) {
                mJpegOrientation =
                    metaSettingsMain.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
            }
            mBokehCamera->mIsCapDepthFinish = false;
            HAL_LOGD("mJpegOrientation=%d", mJpegOrientation);
        } else if (requestStreamType == CALLBACK_STREAM ||
                   requestStreamType == PREVIEW_STREAM ||
                   (mVideoPrevShare && requestStreamType == VIDEO_STREAM)) {
            curReqType.reqType |= 1;
            prevReqNum += 1;
            HAL_LOGD("curReqType %d %d", curReqType.frame_number,
                     curReqType.reqType);
            preview_stream = (request->output_buffers[i]).stream;
            updateApiParams(metaSettingsMain, 0, request->frame_number);
        }
    }
    if (curReqType.reqType >= 2) {
        mReqCapType.frame_number = curReqType.frame_number;
        mReqCapType.reqType = curReqType.reqType;
        HAL_LOGD("mReqCapType %d %d", mReqCapType.frame_number,
                 mReqCapType.reqType);
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
    if (metaSettingsMain.exists(ANDROID_SCALER_CROP_REGION)) {
        int32_t crop_region[4];
        crop_region[0] =
            metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[0];
        crop_region[1] =
            metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[1];
        crop_region[2] =
            metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[2];
        crop_region[3] =
            metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[3];
        HAL_LOGD("crop_region %d %d %d %d", crop_region[0], crop_region[1],
                 crop_region[2], crop_region[3]);
        metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[0] = 0;
        metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[1] = 0;
        metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[2] = 0;
        metaSettingsMain.find(ANDROID_SCALER_CROP_REGION).data.i32[3] = 0;
    }

    req_main = *req;
    req_aux = *req;
    req_main.settings = metaSettingsMain.release();
    req_aux.settings = metaSettingsAux.release();

    int main_buffer_index = 0;
    int aux_buffer_index = 0;
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamTypes(request->output_buffers[i].stream);
        new_stream = (req->output_buffers[i]).stream;
        new_buffer = (req->output_buffers[i]).buffer;
        HAL_LOGD("%d, output_buffers %d, streamtype:%d", i,
                 req->num_output_buffers, requestStreamType);
        HAL_LOGD("%d, w: %d, h: %d, format: %d, usage: %d", i,
                 new_stream->width, new_stream->height, new_stream->format,
                 new_stream->usage);
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
        } else if ((requestStreamType == CALLBACK_STREAM ||
                    requestStreamType == PREVIEW_STREAM ||
                    (requestStreamType == VIDEO_STREAM && mVideoPrevShare)) &&
                   prevReqNum > 0) {
            prevReqNum = 0;
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
        HAL_LOGD("currentmainTimestamp=%llu,currentauxTimestamp=%llu",
                 currentmainTimestamp, currentauxTimestamp);
        hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_BOKEH_SCALE_INFO, &mScaleInfo,
                               NULL);
        if (currentmainTimestamp < currentauxTimestamp) {
            HAL_LOGD("start main, idx:%d", req_main.frame_number);
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_BOKEH_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
            hwiAux->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                  &currentmainTimestamp, NULL);
            HAL_LOGD("start sub, idx:%d", req_aux.frame_number);
            if (!mFlushing) {
                rc = hwiAux->process_capture_request(
                    m_pPhyCamera[CAM_BOKEH_DEPTH].dev, &req_aux);
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
                    m_pPhyCamera[CAM_BOKEH_DEPTH].dev, &req_aux);
                if (rc < 0) {
                    HAL_LOGE("failed, idx:%d", req_aux.frame_number);
                    goto req_fail;
                }
            }
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            HAL_LOGD("start main, idx:%d", req_main.frame_number);
            rc = hwiMain->process_capture_request(
                m_pPhyCamera[CAM_BOKEH_MAIN].dev, &req_main);
            if (rc < 0) {
                HAL_LOGE("failed, idx:%d", req_main.frame_number);
                goto req_fail;
            }
        }
    } else {
        /*HAL_LOGD("process_capture_request main,
         * id=%d,frame_num:%d",m_pPhyCamera[CAM_BOKEH_MAIN].id,
         * req_main.frame_number);
        */
        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_BOKEH_MAIN].dev,
                                              &req_main);
        if (rc < 0) {
            HAL_LOGE("failed, idx:%d", req_main.frame_number);
            goto req_fail;
        }
        /*HAL_LOGD("process_capture_request aux, id=%d,frame_num:%d",
         * m_pPhyCamera[CAM_BOKEH_DEPTH].id, req_aux.frame_number);
        */
        rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_BOKEH_DEPTH].dev,
                                             &req_aux);
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
            HAL_LOGD("mPendingRequest=%d, mMaxPendingCount=%d", mPendingRequest,
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
    HAL_LOGD("rc. %d req_frame_num %d", rc, request->frame_number);

    return rc;
}

void BokehCamera::processCaptureResultMain(
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
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_BOKEH_DEPTH].hwi;
    if (result_buffer == NULL) {
        // meta process
        HAL_LOGD("result_buffer == NULL");
        metadata = result->result;
        updateApiParams(metadata, 1, cur_frame_number);
        if (metadata.exists(ANDROID_SPRD_VCM_STEP) && cur_frame_number) {
            vcmSteps = metadata.find(ANDROID_SPRD_VCM_STEP).data.i32[0];
            HAL_LOGD("vcm %d", vcmSteps);
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
            HAL_LOGD("VCM_INFO:vcmSteps_fixed=%d afState %d", vcmSteps_fixed,
                     afState);
        }
        if (cur_frame_number == mCapFrameNumber) {
            if (mCaptureThread->mReprocessing) {
                HAL_LOGD("hold jpeg picture call bac1k, framenumber:%d",
                         result->frame_number);
            } else {
                {
                    Mutex::Autolock l(mBokehCamera->mMetatLock);
                    metadata_t.frame_number = cur_frame_number;
                    metadata_t.metadata = clone_camera_metadata(result->result);
                    mMetadataList.push_back(metadata_t);
                }
            }
            return;
        } else {
            HAL_LOGD("send  meta, framenumber:%d", cur_frame_number);
            metadata_t.frame_number = cur_frame_number;
            metadata_t.metadata = clone_camera_metadata(result->result);
            Mutex::Autolock l(mBokehCamera->mMetatLock);
            mMetadataList.push_back(metadata_t);
            return;
        }
    }
    HAL_LOGD("num_output_buffers %d", result->num_output_buffers);
    int currStreamType = getStreamTypes(result_buffer->stream);
    HAL_LOGD("streamType: %d", currStreamType);
    /* Process error buffer for Main camera*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGD("Return local buffer:%d caused by error Buffer status",
                 result->frame_number);
        if (currStreamType == CALLBACK_STREAM ||
            currStreamType == PREVIEW_STREAM) {
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
        }
        if (mhasCallbackStream &&
            mThumbReq.frame_number == result->frame_number &&
            mThumbReq.frame_number) {
            CallBackSnapResult(CAMERA3_BUFFER_STATUS_ERROR);
        }
        CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                       PREV_TYPE);

        return;
    }
    HAL_LOGD("mIsCapturing %d, cap_f %d, cur_f %d, rtn %d", mIsCapturing,
             mCapFrameNumber, cur_frame_number, mSnapshotMainReturn);
    if (mIsCapturing && (mCapFrameNumber == cur_frame_number) &&
        (currStreamType != SNAPSHOT_STREAM) &&
        (mSavedCapStreams->width == result_buffer->stream->width &&
         mSavedCapStreams->height == result_buffer->stream->height) &&
        !mSnapshotMainReturn) {
        mSnapshotResultReturn = true;
        mSnapshotMainReturn = true;
        if (mFlushing ||
            result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            if (mhasCallbackStream &&
                mThumbReq.frame_number == result->frame_number &&
                mThumbReq.frame_number) {
                CallBackSnapResult(CAMERA3_BUFFER_STATUS_ERROR);
            }
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                           CAP_TYPE);
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
            HAL_LOGD("save main");
            mCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {
            capture_queue_msg_t capture_msg;
            HAL_LOGD("MSG_DATA_PROC");
            capture_msg.msg_type = MSG_DATA_PROC;
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
                mBokehCamera->mHdrSkipBlur = false;
                mCaptureThread->mCaptureMsgList.push_front(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (mIsCapturing && currStreamType == SNAPSHOT_STREAM) {
        bzero(mJpegOutputBuffers, sizeof(camera3_stream_buffer_t));
        memcpy(mJpegOutputBuffers, result->output_buffers,
               sizeof(camera3_stream_buffer_t));

        jpeg_callback_thread_init((void *)this);
    } else if (currStreamType == CALLBACK_STREAM ||
               currStreamType == PREVIEW_STREAM) {
        if (!mIsSupportPBokeh) {
            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                           PREV_TYPE);
            return;
        }
        // process preview buffer
        {
            Mutex::Autolock l(mNotifyLockMain);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListMain.begin();
                 i != mNotifyListMain.end();) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    HAL_LOGD("i->type: %d", i->type);
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
                                       CAMERA3_BUFFER_STATUS_ERROR, PREV_TYPE);
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
            HAL_LOGD("matchTwoFrame");
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListAux,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                HAL_LOGD("MUXER_MSG_DATA_PROC");
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = cur_frame.buffer;
                muxer_msg.combo_frame.buffer2 = matched_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                             muxer_msg.combo_frame.frame_number);
                    // if (cur_frame.frame_number > 5)
                    clearFrameNeverMatched(cur_frame.frame_number,
                                           matched_frame.frame_number);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGD("Enqueue newest unmatched frame:%d for Main camera",
                         cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
                if (discard_frame != NULL) {
                    HAL_LOGD("discard frame_number %u",
                             discard_frame->frame_number);
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    // if (cur_frame.frame_number > 5)
                    CallBackResult(discard_frame->frame_number,
                                   CAMERA3_BUFFER_STATUS_ERROR, PREV_TYPE);
                    delete discard_frame;
                }
            }
        }
    }
    return;
}

void BokehCamera::processCaptureResultAux(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    CameraMetadata metadata;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    metadata = result->result;
    uint64_t result_timestamp = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    camera3_stream_t *newStream = NULL;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_BOKEH_DEPTH].hwi;
    HAL_LOGD("aux frame_number %d", cur_frame_number);
    if (result->output_buffers == NULL) {
        return;
    }
    if (mFlushing) {
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                       mLocalBufferNumber, mLocalBufferList);
        return;
    }

    int currStreamType = getStreamTypes(result_buffer->stream);
    if (mIsCapturing && mCapFrameNumber == cur_frame_number &&
        !mSnapshotAuxReturn) {
        mSnapshotAuxReturn = true;
        Mutex::Autolock l(mDefaultStreamLock);
        if (NULL == mCaptureThread->mSavedOneResultBuff) {
            HAL_LOGD("save aux");
            mCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {
            capture_queue_msg_t capture_msg;
            HAL_LOGD("send msg MSG_DATA_PROC");
            capture_msg.msg_type = MSG_DATA_PROC;
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
                mBokehCamera->mHdrSkipBlur = false;
                mCaptureThread->mCaptureMsgList.push_front(capture_msg);
                mCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (currStreamType == CALLBACK_STREAM ||
               currStreamType == PREVIEW_STREAM) {
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
            HAL_LOGD("matchTwoFrame");
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListMain,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                HAL_LOGD("MUXER_MSG_DATA_PROC");
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = matched_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame.buffer;
                muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                             muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five
                    // frame.
                    // for first frame meta and ok status buffer update at
                    // the
                    // same time.
                    // app need the point that the first meta updated to
                    // hide
                    // image cover
                    // if (cur_frame.frame_number > 5)
                    clearFrameNeverMatched(matched_frame.frame_number,
                                           cur_frame.frame_number);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGD("Enqueue newest unmatched frame:%d for Aux camera",
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

void BokehCamera::notifyMain(const struct camera3_callback_ops *ops,
                             const camera3_notify_msg_t *msg) {
    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    mBokehCamera->notifyMain(msg);
}

void BokehCamera::notifyMain(const camera3_notify_msg_t *msg) {
    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    if (msg->type == CAMERA3_MSG_SHUTTER &&
        cur_frame_number == mCaptureThread->mSavedCapRequest.frame_number) {
        if (msg->message.shutter.timestamp != 0) {
            capture_result_timestamp = msg->message.shutter.timestamp;
        }
    }

    if (msg->type == CAMERA3_MSG_SHUTTER &&
        cur_frame_number == mCaptureThread->mSavedCapRequest.frame_number &&
        mCaptureThread->mReprocessing) {
        HAL_LOGD("hold cap notify");
        return;
    }
    HAL_LOGD("%d %d", mReqCapType.frame_number, mReqCapType.reqType);
    if ((cur_frame_number != mCapFrameNumber ||
         (cur_frame_number == mReqCapType.frame_number &&
          (mReqCapType.reqType & 1) == 1)) &&
        mIsSupportPBokeh) {
        Mutex::Autolock l(mNotifyLockMain);
        HAL_LOGD("push_back mNotifyListMain");
        mNotifyListMain.push_back(*msg);
        Mutex::Autolock m(mPrevFrameNotifyLock);
        mPrevFrameNotifyList.push_back(*msg);
        if (mPrevFrameNotifyList.size() > BOKEH_PREVIEW_PARAM_LIST) {
            mPrevFrameNotifyList.erase(mPrevFrameNotifyList.begin());
        }
    }
    HAL_LOGD("frame_number:%d, type: %d", msg->message.shutter.frame_number,
             msg->type);
    mCallbackOps->notify(mCallbackOps, msg);
}

void BokehCamera::notifyAux(const struct camera3_callback_ops *ops,
                            const camera3_notify_msg_t *msg) {
    CHECK_CAPTURE();
    mBokehCamera->notifyAux(msg);
}

void BokehCamera::notifyAux(const camera3_notify_msg_t *msg) {
    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    if ((cur_frame_number != mCapFrameNumber ||
         (cur_frame_number == mReqCapType.frame_number &&
          mReqCapType.reqType == 3)) &&
        mIsSupportPBokeh) {
        Mutex::Autolock l(mNotifyLockAux);
        HAL_LOGD("notifyAux push success frame_number %d", cur_frame_number);
        mNotifyListAux.push_back(*msg);
    }
    return;
}

void BokehCamera::initPrevDepthBufferFlag() {
    int index = 0;
    for (; index < 2; index++) {
        mBokehDepthBuffer.prev_depth_buffer[index].w_flag = true;
        mBokehDepthBuffer.prev_depth_buffer[index].r_flag = false;
    }
}

void BokehCamera::setPrevDepthBufferFlag(BufferFlag cur_flag, int index) {
    Mutex::Autolock l(mBokehDepthBufferLock);
    int tmp = 0;
    HAL_LOGD("cur_flag %d, index %d", cur_flag, index);

    if (index < 0)
        return;

    switch (cur_flag) {
    case BOKEH_BUFFER_PING:
        mBokehDepthBuffer.prev_depth_buffer[index].w_flag = true;
        mBokehDepthBuffer.prev_depth_buffer[index].r_flag = true;
        tmp = index == 0 ? 1 : 0;
        mBokehDepthBuffer.prev_depth_buffer[tmp].w_flag = true;
        mBokehDepthBuffer.prev_depth_buffer[tmp].r_flag = false;
        break;
    case BOKEH_BUFFER_PANG:
        mBokehDepthBuffer.prev_depth_buffer[index].w_flag = true;
        break;
    default:
        HAL_LOGI("buffer flag err");
        break;
    }
}

int BokehCamera::getPrevDepthBufferFlag(BufferFlag need_flag) {
    Mutex::Autolock l(mBokehDepthBufferLock);
    int index = 0;
    int ret = -1;

    switch (need_flag) {
    case BOKEH_BUFFER_PING:
        for (index = 0; index < 2; index++) {
            HAL_LOGD("prev_depth_buffer[%d].w_flag:%d, r_flag %d", index,
                     mBokehDepthBuffer.prev_depth_buffer[index].w_flag,
                     mBokehDepthBuffer.prev_depth_buffer[index].r_flag);
            if (mBokehDepthBuffer.prev_depth_buffer[index].w_flag &&
                !mBokehDepthBuffer.prev_depth_buffer[index].r_flag) {
                mBokehDepthBuffer.prev_depth_buffer[index].w_flag = false;
                mBokehDepthBuffer.prev_depth_buffer[index].r_flag = false;
                ret = index;
                break;
            }
        }
        break;
    case BOKEH_BUFFER_PANG:
        for (index = 0; index < 2; index++) {
            HAL_LOGD("prev_depth_buffer[%d].r_flag:%d", index,
                     mBokehDepthBuffer.prev_depth_buffer[index].r_flag);
            if (mBokehDepthBuffer.prev_depth_buffer[index].r_flag) {
                mBokehDepthBuffer.prev_depth_buffer[index].w_flag = false;
                ret = index;
                break;
            }
        }
        break;
    default:
        HAL_LOGI("buffer flag err");
        break;
    }
    HAL_LOGD("get buffer index=%d,flag=%d", ret, need_flag);

    return ret;
}

int BokehCamera::allocateBuff() {
    int rc = 0;
    size_t count = 0;
    size_t j = 0;
    size_t capture_num = 0;
    size_t preview_num = LOCAL_PREVIEW_NUM / 2;
    int w = 0, h = 0;
    HAL_LOGI(":E");
    mLocalBufferList.clear();
    mSavedRequestList.clear();
    mSavedVideoReqList.clear();
    mMetadataList.clear();
    mMetadataReqList.clear();
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

#ifdef CONFIG_BOKEH_HDR_SUPPORT
    capture_num = LOCAL_CAPBUFF_NUM;
#else
    capture_num = LOCAL_CAPBUFF_NUM - 1;
#endif

    HAL_LOGD("capture_num = %d", capture_num);

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

    if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {
        if (0 > allocateOne(mBokehSize.depth_snap_main_w,
                            mBokehSize.depth_snap_main_h, &mLocalScaledBuffer,
                            YUV420)) {
            HAL_LOGE("request one buf failed.");
            goto mem_fail;
        } else {
            rc = mBokehCamera->map(&mLocalScaledBuffer.native_handle,
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

#ifdef CONFIG_SUPPORT_GDEPTH
        for (j = 0; j < SNAP_DEPTH_NUM; j++) {
            if (0 > allocateOne(mBokehSize.depth_snap_out_w,
                                mBokehSize.depth_snap_out_h,
                                &(mLocalBuffer[count + j]), YUV420)) {
                HAL_LOGE("request one buf failed.");
                goto mem_fail;
            }
            mLocalBuffer[count + j].type = SNAPSHOT_GDEPTH_BUFFER;
            mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
        }
        count += SNAP_DEPTH_NUM;
        for (j = 0; j < SNAP_DEPTH_NUM; j++) {
            if (0 > allocateOne(mBokehSize.depth_snap_out_w,
                                mBokehSize.depth_snap_out_h,
                                &(mLocalBuffer[count + j]), YUV420)) {
                HAL_LOGE("request one buf failed.");
                goto mem_fail;
            }
            mLocalBuffer[count + j].type = SNAP_GDEPTHJPEG_BUFFER;
            mLocalBufferList.push_back(&(mLocalBuffer[count + j]));
        }
        count += SNAP_DEPTH_NUM;
#endif

        mBokehCamera->mLocalBufferNumber = count;
        for (j = 0; j < 2; j++) {
            mBokehDepthBuffer.prev_depth_buffer[j].buffer =
                (void *)malloc(mBokehSize.depth_prev_size);
            if (mBokehDepthBuffer.prev_depth_buffer[j].buffer == NULL) {
                HAL_LOGE("mBokehDepthBuffer.prev_depth_buffer malloc failed");
                goto mem_fail;
            } else {
                memset(mBokehDepthBuffer.prev_depth_buffer[j].buffer, 0,
                       mBokehSize.depth_prev_size);
            }
        }
        mBokehDepthBuffer.snap_depth_buffer =
            (void *)malloc(mBokehSize.depth_snap_size);
        if (mBokehDepthBuffer.snap_depth_buffer == NULL) {
            HAL_LOGE("mBokehDepthBuffer.snap_depth_buffer malloc failed");
            goto mem_fail;
        } else {
            memset(mBokehDepthBuffer.snap_depth_buffer, 0,
                   mBokehSize.depth_snap_size);
        }

#if defined(CONFIG_SUPPORT_GDEPTH) ||                                          \
    defined(CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV)
        mBokehDepthBuffer.snap_depthConfidence_buffer =
            (uint8_t *)malloc(mBokehSize.depth_confidence_map_size);
        if (mBokehDepthBuffer.snap_depthConfidence_buffer == NULL) {
            HAL_LOGE("mBokehDepthBuffer.snap_depth_buffer malloc failed");
            goto mem_fail;
        } else {
            memset(mBokehDepthBuffer.snap_depthConfidence_buffer, 0,
                   mBokehSize.depth_confidence_map_size);
        }
        mBokehDepthBuffer.depth_normalize_data_buffer =
            (uint8_t *)malloc(mBokehSize.depth_yuv_normalize_size);
        if (mBokehDepthBuffer.depth_normalize_data_buffer == NULL) {
            HAL_LOGE(
                "mBokehDepthBuffer.depth_normalize_data_buffer malloc failed");
            goto mem_fail;
        } else {
            memset(mBokehDepthBuffer.depth_normalize_data_buffer, 0,
                   mBokehSize.depth_yuv_normalize_size);
        }
#endif

        mBokehDepthBuffer.depth_out_map_table =
            (void *)malloc(mBokehSize.depth_weight_map_size);
        if (mBokehDepthBuffer.depth_out_map_table == NULL) {
            HAL_LOGE("mBokehDepthBuffer.depth_out_map_table  malloc failed.");
            goto mem_fail;
        } else {
            memset(mBokehDepthBuffer.depth_out_map_table, 0,
                   mBokehSize.depth_weight_map_size);
        }
        mBokehDepthBuffer.prev_depth_scale_buffer =
            (void *)malloc(mBokehSize.depth_prev_scale_size);
        if (mBokehDepthBuffer.prev_depth_scale_buffer == NULL) {
            HAL_LOGE(
                "mBokehDepthBuffer.prev_depth_scale_buffer  malloc failed.");
            goto mem_fail;
        } else {
            memset(mBokehDepthBuffer.prev_depth_scale_buffer, 0,
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
void BokehCamera::freeLocalBuffer() {
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
    if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {

        for (int i = 0; i < 2; i++) {
            if (mBokehDepthBuffer.prev_depth_buffer[i].buffer != NULL) {
                free(mBokehDepthBuffer.prev_depth_buffer[i].buffer);
                mBokehDepthBuffer.prev_depth_buffer[i].buffer = NULL;
            }
        }
        if (mBokehDepthBuffer.snap_depth_buffer != NULL) {
            free(mBokehDepthBuffer.snap_depth_buffer);
            mBokehDepthBuffer.snap_depth_buffer = NULL;
        }

#if defined(CONFIG_SUPPORT_GDEPTH) ||                                          \
    defined(CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV)
        if (mBokehDepthBuffer.snap_depthConfidence_buffer != NULL) {
            free(mBokehDepthBuffer.snap_depthConfidence_buffer);
            mBokehDepthBuffer.snap_depthConfidence_buffer = NULL;
        }
        if (mBokehDepthBuffer.depth_normalize_data_buffer != NULL) {
            free(mBokehDepthBuffer.depth_normalize_data_buffer);
            mBokehDepthBuffer.depth_normalize_data_buffer = NULL;
        }
#endif

        if (mBokehDepthBuffer.depth_out_map_table != NULL) {
            free(mBokehDepthBuffer.depth_out_map_table);
            mBokehDepthBuffer.depth_out_map_table = NULL;
        }
        if (mBokehDepthBuffer.prev_depth_scale_buffer != NULL) {
            free(mBokehDepthBuffer.prev_depth_scale_buffer);
            mBokehDepthBuffer.prev_depth_scale_buffer = NULL;
        }
    }
}

void BokehCamera::getDepthImageSize(int inputWidth, int inputHeight,
                                    int *outWidth, int *outHeight, int type) {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    int value = 0;
    if (mApiVersion == BOKEH_API_MODE) {
        int aspect_ratio = (inputWidth * SFACTOR) / inputHeight;
        if (ABS(aspect_ratio - AR4_3) < 1) {
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
        } else if (type == VIDEO_STREAM) {
            *outWidth = CAM_AUX_PREV_WIDTH;
            *outHeight = CAM_AUX_PREV_HEIGHT;
        }
    }

    HAL_LOGD("iw:%d,ih:%d,ow:%d,oh:%d", inputWidth, inputHeight, *outWidth,
             *outHeight);
}

int BokehCamera::setupPhysicalCameras(std::vector<int> SensorIds) {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    for (int i = 0; i < m_nPhyCameras; i++) {
        m_pPhyCamera[i].id = SensorIds[i];
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   :updateApiParams
 * DESCRIPTION: update bokeh and depth api Params
 * PARAMETERS :
 * RETURN     : none
 *==========================================================================*/
void BokehCamera::updateApiParams(CameraMetadata metaSettings, int type,
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

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;
    FACE_Tag *faceDetectionInfo =
        (FACE_Tag *)&(hwiMain->mSetting->s_setting[phyId].faceInfo);
    uint8_t numFaces = faceDetectionInfo->face_num;
    int32_t faceRectangles[CAMERA3MAXFACE * 4];
    MRECT face_rect[CAMERA3MAXFACE];
    int j = 0;
    int max = 0;
    int max_index = 0;

    faceaf_frame_buffer_info_t mtempParm;
    memset(&mtempParm, 0, sizeof(faceaf_frame_buffer_info_t));

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
        SprdCamera3Setting::s_setting[m_pPhyCamera[0].id]
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
                    faceInfo[2] * mBokehSize.capture_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[1] * mBokehSize.capture_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[0] * mBokehSize.capture_h / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[3] * mBokehSize.capture_h / origH;
            } else if (mbokehParm.portrait_param.mobile_angle == 90) {
                mbokehParm.portrait_param.x1[i] =
                    faceInfo[2] * mBokehSize.capture_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[3] * mBokehSize.capture_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[0] * mBokehSize.capture_w / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[1] * mBokehSize.capture_h / origH;
            } else if (mbokehParm.portrait_param.mobile_angle == 180) {
                mbokehParm.portrait_param.x1[i] =
                    faceInfo[0] * mBokehSize.capture_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[3] * mBokehSize.capture_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[2] * mBokehSize.capture_w / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[1] * mBokehSize.capture_h / origH;
            } else {
                mbokehParm.portrait_param.x1[i] =
                    faceInfo[0] * mBokehSize.capture_w / origW;
                mbokehParm.portrait_param.y1[i] =
                    faceInfo[1] * mBokehSize.capture_h / origH;
                mbokehParm.portrait_param.x2[i] =
                    faceInfo[2] * mBokehSize.capture_w / origW;
                mbokehParm.portrait_param.y2[i] =
                    faceInfo[3] * mBokehSize.capture_h / origH;
            }
            mbokehParm.portrait_param.flag[i] = 0;
            mbokehParm.portrait_param.flag[i + 1] = 1;
        }
        mbokehParm.portrait_param.valid_roi = numFaces;
    }

    if (mbokehParm.portrait_param.valid_roi && mBokehMode == PORTRAIT_MODE) {
        mDoPortrait = 1;
    } else {
        mDoPortrait = 0;
    }
    HAL_LOGD("mDoPortrait%d,numFaces=%d,mBokehMode=%d", mDoPortrait, numFaces,
             mBokehMode);
    if (m_pPhyCamera[0].id == 0)
        mbokehParm.portrait_param.rear_cam_en = true;
    else
        mbokehParm.portrait_param.rear_cam_en = 0;
    mbokehParm.portrait_param.face_num = numFaces;
    mbokehParm.portrait_param.portrait_en = mBokehMode;
    isUpdate = true;
    if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
        int fnum = metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
        if (fnum < MIN_F_FUMBER) {
            fnum = MIN_F_FUMBER;
        } else if (fnum > MAX_F_FUMBER) {
            fnum = MAX_F_FUMBER;
        }
        if (fnum != mbokehParm.f_number) {
            mbokehParm.f_number = fnum;
            HAL_LOGD("fnum %d", fnum);
            HAL_LOGD("ALG mBokehAlgo->setBokenParam start");
            mBokehAlgo->setBokenParam((void *)&mbokehParm);
            HAL_LOGD("ALG mBokehAlgo->setBokenParam end");
            mDepthTrigger = BOKEH_TRIGGER_FNUM;
            HAL_LOGD("mDepthTrigger:%d", mDepthTrigger);
        }
    }
    if (metaSettings.exists(ANDROID_SPRD_AF_ROI)) {
        int left = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[0];
        int top = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[1];
        int right = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[2];
        int bottom = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[3];
        mAfstate = metaSettings.find(ANDROID_SPRD_AF_ROI).data.i32[4];
        int x = left, y = top;
        if (sn_trim_flag) {
            struct sprd_img_path_rect sn_trim;
            bzero(&sn_trim, sizeof(struct sprd_img_path_rect));
            hwiMain->camera_ioctrl(CAMERA_TOCTRL_GET_BOKEH_SN_TRIM, &sn_trim,
                                   NULL);

            trim_W = sn_trim.trim_valid_rect.w;
            trim_H = sn_trim.trim_valid_rect.h;
            sn_trim_flag = false;
        }
        if (left != 0 && top != 0 && right != 0 && bottom != 0) {
            x = left + (right - left) / 2;
            y = top + (bottom - top) / 2;
            x = x * mBokehSize.preview_w / trim_W;
            y = y * mBokehSize.preview_h / trim_H;
            if (x != mbokehParm.sel_x || y != mbokehParm.sel_y) {
                mbokehParm.sel_x = x;
                mbokehParm.sel_y = y;
                isUpdate = true;
                HAL_LOGD("sel_x %d ,sel_y %d", x, y);
            }
            mtempParm.x = x;
            mtempParm.y = y;
            mtempParm.frame_number = cur_frame_number;
            mFaceafList.push_back(mtempParm);
        }
        if (mSnapshotResultReturn) {
            if (!mFaceafList.empty()) {
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
                for (List<faceaf_frame_buffer_info_t>::iterator j =
                         mFaceafList.begin();
                     j != mFaceafList.end(); j++) {
                    if (j->frame_number == prev_frame_number) {
                        if (j->x != mbokehParm.capture_x ||
                            j->y != mbokehParm.capture_y) {
                            mbokehParm.capture_x = j->x;
                            mbokehParm.capture_y = j->y;
                            isUpdate = true;
                        }
                        break;
                    }
                }
            }
            mSnapshotResultReturn = false;
        }
        if (mFaceafList.size() > BOKEH_PREVIEW_PARAM_LIST) {
            mFaceafList.erase(mFaceafList.begin());
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
    if (mDoPortrait == 1) {
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
        HAL_LOGD("ALG mBokehAlgo->setBokenParam start");
        HAL_LOGD("fnum %d", mbokehParm.f_number);
        mBokehAlgo->setBokenParam((void *)&mbokehParm);
        HAL_LOGD("ALG mBokehAlgo->setBokenParam end");
    }
}

void BokehCamera::setBokehCameraTags(CameraMetadata &metadata) {
    uint8_t available = 1, unavailable = 0;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.res.bokeh", prop, "RES_5M");
    HAL_LOGI("bokeh support cap resolution %s", prop);
    getMaxSize(prop);
    /*ANDROID_JPEG_MAX_SIZE*/
    int32_t jpeg_stream_size =
        (maxCapHeight * maxCapWidth * 3 / 2 + sizeof(camera3_jpeg_blob_t));
    int32_t img_size =
        jpeg_stream_size * 2 +
        (DEPTH_SNAP_OUTPUT_WIDTH * DEPTH_SNAP_OUTPUT_HEIGHT * 2) +
        (BOKEH_REFOCUS_COMMON_PARAM_NUM * 4) + BOKEH_REFOCUS_COMMON_XMP_SIZE +
        sizeof(camera3_jpeg_blob_t) + 1024;
    metadata.update(ANDROID_JPEG_MAX_SIZE, &img_size, 1);
    /*ANDROID_REQUEST_AVAILABLE_CAPABILITIES*/
    const uint8_t kavl_capabilities[] = {
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA};
    metadata.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES, kavl_capabilities,
                    sizeof(kavl_capabilities) / sizeof(uint8_t));
    /*ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS*/
    const int32_t kavl_high_speed_video_configration[] = {
        maxPrevWidth, maxPrevHeight, 120, 120, 1};
    metadata.update(ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS,
                    (int32_t *)kavl_high_speed_video_configration,
                    sizeof(kavl_high_speed_video_configration) /
                        sizeof(int32_t));
    /*ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS*/
    camera_metadata_entry_t entry =
        metadata.find(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS);
    size_t count = entry.count;
    int32_t *key = new int32_t[count + 5];
    memcpy(key, entry.data.i32, entry.count * sizeof(int32_t));
    key[count++] = ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS;
    key[count++] = ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE;
    key[count++] = ANDROID_CONTROL_AVAILABLE_MODES;
    key[count++] = ANDROID_CONTROL_AE_LOCK_AVAILABLE;
    key[count++] = ANDROID_CONTROL_AWB_LOCK_AVAILABLE;
    metadata.update(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, key, count);
    delete[] key;

    /*ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS*/
    for (size_t i = 0; i < mPhysicalIds.size(); i++) {
        physicIds[i * 2] = '0' + mPhysicalIds[i];
        physicIds[i * 2 + 1] = '\0';
        HAL_LOGD("physicalIds[%d], id %d", i * 2, mPhysicalIds[i]);
    }
    metadata.update(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS, physicIds,
                    mPhysicalIds.size() * 2);

    /*ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE*/
    uint8_t sync_type =
        ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE_CALIBRATED;
    metadata.update(ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE, &sync_type,
                    1);

    /*ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS*/
    camera_metadata_entry_t entrys =
        metadata.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS);
    count = entrys.count;
    int32_t *keys = new int32_t[count + 4];
    memcpy(keys, entrys.data.i32, entrys.count * sizeof(int32_t));
    keys[count++] = ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_MAX_SIZES;
    keys[count++] =
        ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_ZOOM_RATIO_RANGES;
    keys[count++] = ANDROID_CONTROL_ZOOM_RATIO;
    keys[count++] = ANDROID_CONTROL_EXTENDED_SCENE_MODE;
    metadata.update(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, keys, count);
    delete[] keys;
    {
        /*ANDROID_REQUEST_AVAILABLE_RESULT_KEYS*/
        entrys = metadata.find(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS);
        count = entrys.count;
        keys = new int32_t[count + 2];
        memcpy(keys, entrys.data.i32, entrys.count * sizeof(int32_t));
        keys[count++] = ANDROID_CONTROL_EXTENDED_SCENE_MODE;
        keys[count++] = ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID;
        metadata.update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, keys, count);
        delete[] keys;
    }
    /*ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_MAX_SIZES*/
    int32_t kavailableExtendedSceneModeMaxSizes[EXTEND_MODE_NUM][3] = {
        {ANDROID_CONTROL_EXTENDED_SCENE_MODE_DISABLED, 0, 0},
        {ANDROID_CONTROL_EXTENDED_SCENE_MODE_BOKEH_STILL_CAPTURE, maxPrevWidth,
         maxPrevHeight}, /*YUV_420_888*/
    };
    metadata.update(ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_MAX_SIZES,
                    (int32_t *)kavailableExtendedSceneModeMaxSizes,
                    sizeof(kavailableExtendedSceneModeMaxSizes) /
                        sizeof(int32_t));
    /*ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_ZOOM_RATIO_RANGES*/
    const float kavailableZoomRatioRanges[2] = {1.0, 1.0};
    metadata.update(
        ANDROID_CONTROL_AVAILABLE_EXTENDED_SCENE_MODE_ZOOM_RATIO_RANGES,
        (float *)kavailableZoomRatioRanges,
        sizeof(kavailableZoomRatioRanges) / sizeof(float));

    int32_t avlThmbSizes[4] = {0, 0, 320, 240};
    metadata.update(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES, avlThmbSizes, 4);
    metadata.update(ANDROID_FLASH_INFO_AVAILABLE, &unavailable, 1);
    /*AE AWB LOCK*/
    metadata.update(ANDROID_CONTROL_AE_LOCK_AVAILABLE, &unavailable, 1);
    uint8_t awbLockAvailable = 0; /*AWB_LOCK_AVAILABLE*/
    metadata.update(ANDROID_CONTROL_AWB_LOCK_AVAILABLE, &unavailable, 1);
    float zoomRatioRange[2] = {1.0, 1.0};
    metadata.update(ANDROID_CONTROL_ZOOM_RATIO_RANGE, zoomRatioRange, 2);
    /*ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS*/
    int32_t kAvlStreamConfig[][4] = {
        /*format, width, height, output/input stream*/
        //{HAL_PIXEL_FORMAT_BLOB, 320, 240, 0},
        //{HAL_PIXEL_FORMAT_BLOB, 640, 480, 0},
        //{HAL_PIXEL_FORMAT_BLOB, 1280, 720, 0},
        {HAL_PIXEL_FORMAT_BLOB, maxCapWidth, maxCapHeight, 0},
        {HAL_PIXEL_FORMAT_BLOB, 1920, 1080, 0},
        {HAL_PIXEL_FORMAT_YCBCR_420_888, maxPrevWidth, maxPrevHeight, 0},
        {HAL_PIXEL_FORMAT_YCBCR_420_888, 320, 240, 0},
        {HAL_PIXEL_FORMAT_YCBCR_420_888, 640, 480, 0},
        {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, maxPrevWidth, maxPrevHeight,
         0},
        {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, 320, 240, 0},
        {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, 640, 480, 0},
    };
    metadata.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                    (int32_t *)kAvlStreamConfig,
                    sizeof(kAvlStreamConfig) / sizeof(int32_t));
    /*ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS*/
    int32_t stream_num = sizeof(kAvlStreamConfig) / sizeof(int32_t) / 4;
    HAL_LOGD("stream_num %d", stream_num);
    Vector<int64_t> min_duration, stall_duration;
    int64_t duration = 33331760;
    for (int i = 0; i < stream_num; i++) {
        for (int j = 0; j < 3; j++) {
            min_duration.add(kAvlStreamConfig[i][j]);
            stall_duration.add(kAvlStreamConfig[i][j]);
        }
        min_duration.add(duration);
        if (kAvlStreamConfig[i][0] == HAL_PIXEL_FORMAT_BLOB)
            stall_duration.add(duration);
        else
            stall_duration.add(0);
    }
    metadata.update(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
                    min_duration.array(), min_duration.size());
    metadata.update(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,
                    stall_duration.array(), stall_duration.size());

    int32_t activeArraySize[4] = {0, 0, maxCapWidth, maxCapHeight};
    metadata.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeArraySize, 4);
    int32_t pixelArraySize[2] = {maxCapWidth, maxCapHeight};
    metadata.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixelArraySize, 2);

    float availableMaxDigitalZoom = 1.0f;
    metadata.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
                    &availableMaxDigitalZoom, 1);
    const int32_t kavailable_fps_ranges_back[] = {10, 10, 5,  15, 15, 15, 5,
                                                  20, 20, 20, 5,  24, 24, 24,
                                                  5,  30, 20, 30, 30, 30};
    metadata.update(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                    kavailable_fps_ranges_back,
                    sizeof(kavailable_fps_ranges_back) / sizeof(int32_t));
    uint8_t availableControlModes[] = {
        ANDROID_CONTROL_MODE_OFF, ANDROID_CONTROL_MODE_AUTO,
        ANDROID_CONTROL_MODE_USE_EXTENDED_SCENE_MODE};
    metadata.update(ANDROID_CONTROL_AVAILABLE_MODES, availableControlModes,
                    sizeof(availableControlModes) / sizeof(uint8_t));
    uint8_t aeAvlModes = {ANDROID_CONTROL_AE_MODE_ON}; // AE_MODES
    metadata.update(ANDROID_CONTROL_AE_AVAILABLE_MODES, &aeAvlModes, 1);
    uint8_t awbAvlModes = {ANDROID_CONTROL_AWB_MODE_AUTO}; // AWB_MODES
    metadata.update(ANDROID_CONTROL_AWB_AVAILABLE_MODES, &awbAvlModes, 1);
    HAL_LOGI("fill BokeCamera Tags");
}

/*===========================================================================
 * FUNCTION   :cap_3d_doFaceMakeup
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
#ifdef CONFIG_FACE_BEAUTY
void BokehCamera::bokehFaceMakeup(buffer_handle_t *buffer_handle,
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
        SprdCamera3Setting::s_setting[mBokehCamera->m_pPhyCamera[0].id]
            .faceInfo.face_num;
    newFace.face[0].rect[0] = faceInfo[0];
    newFace.face[0].rect[1] = faceInfo[1];
    newFace.face[0].rect[2] = faceInfo[2];
    newFace.face[0].rect[3] = faceInfo[3];
    mBokehCamera->doFaceMakeup2(frame, mBokehCamera->mPerfectskinlevel,
                                &newFace,
                                0); // work mode 1 for preview, 0 for picture
}
#endif

/*===========================================================================
 * FUNCTION   :saveRequest
 *
 * DESCRIPTION: save buffer in request
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void BokehCamera::saveRequest(camera3_capture_request_t *request) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    multi_request_saved_t currRequest;
    multi_request_saved_t curVideoRequest;
    meta_req_t curMeta;
    memset(&currRequest, 0, sizeof(multi_request_saved_t));
    memset(&curVideoRequest, 0, sizeof(multi_request_saved_t));
    memset(&curMeta, 0, sizeof(meta_req_t));
    HAL_LOGD(" E %d", request->frame_number);
    CameraMetadata settings = clone_camera_metadata(request->settings);
    if (settings.exists(ANDROID_CONTROL_EXTENDED_SCENE_MODE)) {
        mExtendMode =
            settings.find(ANDROID_CONTROL_EXTENDED_SCENE_MODE).data.u8[0];
        HAL_LOGD("request %d, extend_mode %d", request->frame_number,
                 mExtendMode);
    }
    {
        Mutex::Autolock l(mMetaReqLock);
        curMeta.frame_number = request->frame_number;
        curMeta.metadata = request->settings;
        if (!curMeta.metadata.exists(ANDROID_CONTROL_EXTENDED_SCENE_MODE)) {
            camera_metadata_entry_t entrys =
                curMeta.metadata.find(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS);
            int count = entrys.count;
            int32_t *keys = new int32_t[count + 1];
            memcpy(keys, entrys.data.i32, entrys.count * sizeof(int32_t));
            keys[count++] = ANDROID_CONTROL_EXTENDED_SCENE_MODE;
            curMeta.metadata.update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, keys,
                                    count);
            delete[] keys;
        }
        curMeta.metadata.update(ANDROID_CONTROL_EXTENDED_SCENE_MODE,
                                &mExtendMode, 1);
        mMetadataReqList.push_back(curMeta);
    }
    Mutex::Autolock l(mRequestLock);
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        newStream->reserved[0] = NULL;
        int type = getStreamTypes(newStream);
        if (type == CALLBACK_STREAM || type == PREVIEW_STREAM) {
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.preview_stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            currRequest.frame_number = request->frame_number;
            HAL_LOGD("%d, %dx%d", i, currRequest.preview_stream->width,
                     currRequest.preview_stream->height);
            HAL_LOGD("save request:id %d", request->frame_number);
            mSavedRequestList.push_back(currRequest);
        } else if (type == DEFAULT_STREAM) {
            mThumbReq.buffer = request->output_buffers[i].buffer;
            mThumbReq.preview_stream = request->output_buffers[i].stream;
            mThumbReq.input_buffer = request->input_buffer;
            mThumbReq.frame_number = request->frame_number;
            HAL_LOGD("save pic request:id %d, w= %d,h=%d",
                     request->frame_number, (mThumbReq.preview_stream)->width,
                     (mThumbReq.preview_stream)->height);
        } else if (type == VIDEO_STREAM) {
            curVideoRequest.buffer = (request->output_buffers[i]).buffer;
            curVideoRequest.input_buffer = request->input_buffer;
            curVideoRequest.video_stream = request->output_buffers[i].stream;
            curVideoRequest.frame_number = request->frame_number;
            HAL_LOGD("save video request:id %d", request->frame_number);
            mSavedVideoReqList.push_back(curVideoRequest);
            if (mVideoPrevShare) {
                currRequest.buffer = request->output_buffers[i].buffer;
                currRequest.preview_stream = request->output_buffers[i].stream;
                currRequest.input_buffer = request->input_buffer;
                currRequest.frame_number = request->frame_number;
                HAL_LOGD("save prev request:id %d", request->frame_number);
                mSavedRequestList.push_back(currRequest);
            }
        }
    }
    HAL_LOGD(" X");
}

int BokehCamera::thumbYuvProc(buffer_handle_t *src_buffer) {
    int ret = NO_ERROR;
    int angle = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_BOKEH_MAIN].hwi;
    struct img_frm src_img;
    struct img_frm dst_img;
    struct snp_thumb_yuv_param thumb_param;
    void *src_buffer_addr = NULL;
    void *thumb_req_addr = NULL;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    HAL_LOGI(" E");

    ret = mBokehCamera->map(src_buffer, &src_buffer_addr);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map src buffer");
        goto fail_map_src;
    }
    ret = mBokehCamera->map(mThumbReq.buffer, &thumb_req_addr);
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

    mBokehCamera->unmap(mThumbReq.buffer);
fail_map_thumb:
    mBokehCamera->unmap(src_buffer);
fail_map_src:

    return ret;
}

void BokehCamera::dumpCaptureBokeh(unsigned char *result_buffer_addr,
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

        if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {
            para_size = BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
            depth_size =
                mBokehSize.depth_snap_out_w * mBokehSize.depth_snap_out_h * 2;
            common_num = BOKEH_REFOCUS_COMMON_PARAM_NUM;
            depth_width = mBokehSize.depth_snap_out_w;
            depth_height = mBokehSize.depth_snap_out_h;
        }
        uint32_t orig_yuv_size = mBokehCamera->mBokehSize.capture_w *
                                 mBokehCamera->mBokehSize.capture_h * 3 / 2;
        unsigned char *buffer_base = result_buffer_addr;
        dumpData(buffer_base, 2, mXmpSize + jpeg_size, mBokehSize.capture_w,
                 mBokehSize.capture_h, mBokehCamera->mCapFrameNumber,
                 "bokehJpeg");
        HAL_LOGD("jpeg1 size=:%d", mXmpSize + jpeg_size);
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        depth_yuv_normalize_size =
            mBokehCamera->mBokehSize.depth_yuv_normalize_size;
#endif
#ifdef YUV_CONVERT_TO_JPEG
        uint32_t use_size = para_size + depth_yuv_normalize_size + depth_size +
                            mBokehCamera->mOrigJpegSize + jpeg_size + xmp_size;
#else
        uint32_t use_size = para_size + depth_size + orig_yuv_size + jpeg_size;
#endif
        buffer_base += (use_size - para_size + mXmpSize);
        dumpData(buffer_base, 3, common_num, 4, 0, 0, "parameter");
        buffer_base -= (int)(depth_size);

        dumpData(buffer_base, 1, depth_size, depth_width, depth_height,
                 mBokehCamera->mCapFrameNumber, "depth");
#ifdef YUV_CONVERT_TO_JPEG
        buffer_base -=
            (int)(mBokehCamera->mOrigJpegSize + depth_yuv_normalize_size);
        dumpData(buffer_base, 2, mBokehCamera->mOrigJpegSize,
                 mBokehSize.capture_w, mBokehSize.capture_h,
                 mBokehCamera->mCapFrameNumber, "origJpeg");

#else
        buffer_base -= (int)(orig_yuv_size);
        dumpData(buffer_base, 1, orig_yuv_size, mBokehSize.capture_w,
                 mBokehSize.capture_h, mBokehCamera->mCapFrameNumber,
                 "origYuv");
#endif
    }
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
void BokehCamera::CallBackMetadata(uint32_t frame_number) {
    camera3_capture_result_t result;
    bzero(&result, sizeof(camera3_capture_result_t));
    List<meta_save_t>::iterator itor;
    {
        Mutex::Autolock l(mMetatLock);
        itor = mMetadataList.begin();
        while (itor != mMetadataList.end() &&
               itor->frame_number <= frame_number) {
            result.frame_number = itor->frame_number;
            result.result = const_cast<camera_metadata_t *>(itor->metadata);
            result.num_output_buffers = 0;
            result.output_buffers = NULL;
            result.input_buffer = NULL;
            result.partial_result = 1;
            CameraMetadata metadata = clone_camera_metadata(result.result);
            setResultMetadata(metadata, result.frame_number);
            result.result = metadata.release();
            HAL_LOGD("CallBackMetadata, %d", result.frame_number);
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(result.result));
            mMetadataList.erase(itor);
            itor++;
        }
    }
}

void BokehCamera::CallBackSnapResult(int status) {

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
    CallBackMetadata(result.frame_number);
    mCallbackOps->process_capture_result(mCallbackOps, &result);
    memset(&mThumbReq, 0, sizeof(multi_request_saved_t));
    HAL_LOGD("snap id: %d ", result.frame_number);
}

void BokehCamera::CallBackResult(uint32_t frame_number,
                                 camera3_buffer_status_t buffer_status,
                                 intent_type type) {

    camera3_capture_result_t result;
    List<multi_request_saved_t>::iterator itor, itor2;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));

    if (type == PREV_TYPE) {
        Mutex::Autolock l(mBokehCamera->mRequestLock);
        itor = mBokehCamera->mSavedRequestList.begin();
        while (itor != mBokehCamera->mSavedRequestList.end()) {
            if (itor->frame_number == frame_number) {
                CallBackMetadata(frame_number);
                result_buffers.stream = itor->preview_stream;
                result_buffers.buffer = itor->buffer;
                if (mVideoStreamsNum >= 0) {
                    callbackVideoResult(frame_number, buffer_status,
                                        result_buffers.buffer);
                }
                HAL_LOGD("erase frame_number %u", frame_number);
                mBokehCamera->mSavedRequestList.erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mBokehCamera->mSavedRequestList.end()) {
            HAL_LOGE("can't find frame in mSavedRequestList %u:", frame_number);
            return;
        }
        itor2 = mBokehCamera->mSavedRequestList.begin();
        while (itor2 != mBokehCamera->mSavedRequestList.end()) {
            if (itor2->frame_number == frame_number) {
                camera3_capture_result_t result2;
                camera3_stream_buffer_t result_buffers2;
                bzero(&result2, sizeof(camera3_capture_result_t));
                bzero(&result_buffers2, sizeof(camera3_stream_buffer_t));
                void *addr = NULL, *addr2 = NULL;
                uint32_t size =
                    itor2->stream->width * itor2->stream->height * 3 / 2;
                if (mBokehCamera->map(itor2->buffer, &addr2) != NO_ERROR) {
                    HAL_LOGE("fail to map buffer2");
                    return;
                }
                if (mBokehCamera->map(result_buffers.buffer, &addr) !=
                    NO_ERROR) {
                    HAL_LOGE("fail to map buffer1");
                    mBokehCamera->unmap(itor2->buffer);
                    return;
                }
                memcpy(addr2, addr, size);
                mBokehCamera->unmap(itor2->buffer);
                mBokehCamera->unmap(result_buffers.buffer);
                result_buffers2.stream = itor2->preview_stream;
                result_buffers2.buffer = itor2->buffer;
                result_buffers2.status = buffer_status;
                result_buffers2.acquire_fence = -1;
                result_buffers2.release_fence = -1;
                result2.result = NULL;
                result2.frame_number = frame_number;
                result2.num_output_buffers = 1;
                result2.output_buffers = &result_buffers2;
                result2.input_buffer = NULL;
                result2.partial_result = 0;
                HAL_LOGD("id:%d buffer_status %u, %p", result2.frame_number,
                         buffer_status, (result2.output_buffers)->buffer);
                mCallbackOps->process_capture_result(mCallbackOps, &result2);
                HAL_LOGD("erase frame_number %u", frame_number);
                mBokehCamera->mSavedRequestList.erase(itor2);
                break;
            }
            itor2++;
        }
    } else if (type == CAP_TYPE) {
        CallBackMetadata(frame_number);
        result_buffers.stream = mBokehCamera->mSavedCapStreams;
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

    if (!mVideoPrevShare) {
        HAL_LOGD("priv:%p,%s", result_buffers.stream->priv,
                 result_buffers.stream->physical_camera_id);
        HAL_LOGD("result_buffer:%d,%d", result_buffers.stream->width,
                 result_buffers.stream->height);
        HAL_LOGD("id:%d buffer_status %u, %p", result.frame_number,
                 buffer_status, (result.output_buffers)->buffer);
        mCallbackOps->process_capture_result(mCallbackOps, &result);
    }
    if (!buffer_status) {
        mBokehCamera->dumpFps();
    }

    if (type == PREV_TYPE) {
        Mutex::Autolock l(mBokehCamera->mPendingLock);
        mBokehCamera->mPendingRequest--;
        if (mBokehCamera->mPendingRequest < mBokehCamera->mMaxPendingCount) {
            HAL_LOGD("signal request m_PendingRequest = %d",
                     mBokehCamera->mPendingRequest);
            mBokehCamera->mRequestSignal.signal();
        }
        if (frame_number == mReqCapType.frame_number &&
            (mReqCapType.reqType & 1) == 1)
            mReqCapType.reqType &= 2;
    } else {
        mBokehCamera->mCapFrameNumber = -1;
        mReqCapType.reqType &= 1;
    }
}

void BokehCamera::callbackVideoResult(uint32_t frame_number,
                                      camera3_buffer_status_t buffer_status,
                                      buffer_handle_t *buffer) {
    void *prev_addr = NULL;
    void *video_addr = NULL;
    camera3_capture_result_t result;
    List<multi_request_saved_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));
    itor = mBokehCamera->mSavedVideoReqList.begin();
    while (itor != mBokehCamera->mSavedVideoReqList.end()) {
        if (itor->frame_number == frame_number) {
            int rc = mBokehCamera->map(buffer, &prev_addr);
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to map prev buffer %d", frame_number);
                return;
            }
            rc = mBokehCamera->map(itor->buffer, &video_addr);
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to map video buffer %d", frame_number);
                return;
            }
            if (mBokehStream.video_w != mBokehSize.preview_w ||
                mBokehStream.video_h != mBokehSize.preview_h) {
                rc = mBokehCamera->hwScale(
                    (uint8_t *)video_addr, mBokehStream.video_w,
                    mBokehStream.video_h, ADP_BUFFD(*(itor->buffer)),
                    (uint8_t *)prev_addr, mBokehSize.preview_w,
                    mBokehSize.preview_h, ADP_BUFFD(*buffer));
                if (rc != NO_ERROR) {
                    HAL_LOGE("fail to get video buffer %d", frame_number);
                    return;
                }
            } else {
                uint32_t buf_size =
                    mBokehStream.video_w * mBokehStream.video_h * 3 / 2;
                memcpy(video_addr, prev_addr, buf_size);
            }
            mBokehCamera->unmap(buffer);
            mBokehCamera->unmap(itor->buffer);
            result_buffers.stream = itor->video_stream;
            result_buffers.buffer = itor->buffer;
            HAL_LOGD("erase video frame_number %u", frame_number);
            mBokehCamera->mSavedVideoReqList.erase(itor);
            break;
        }
        itor++;
    }
    if (itor == mBokehCamera->mSavedVideoReqList.end()) {
        HAL_LOGE("can't find frame in mSavedVideoReqList %u:", frame_number);
        return;
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
    HAL_LOGD("result_buffer:%d,%d", result_buffers.stream->width,
             result_buffers.stream->height);
    mCallbackOps->process_capture_result(mCallbackOps, &result);
    HAL_LOGD("id:%d buffer_status %u", result.frame_number, buffer_status);
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
void BokehCamera::clearFrameNeverMatched(uint32_t main_frame_number,
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
            CallBackResult(frame_num, CAMERA3_BUFFER_STATUS_ERROR, PREV_TYPE);
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

void BokehCamera::setDepthStatus(BokehDepthStatus state) {
    HAL_LOGD("state %d", state);
    Mutex::Autolock l(mDepthStatusLock);
    BokehDepthStatus pre_status = mDepthStatus;
    switch (pre_status) {
    case BOKEH_DEPTH_DONING:
        mDepthStatus = state;
        if (state == BOKEH_DEPTH_DONE) {
            mDepthTrigger = BOKEH_TRIGGER_FALSE;
        }
        break;
    case BOKEH_DEPTH_DONE:
        mDepthStatus = state;
        break;
    case BOKEH_DEPTH_INVALID:
        if (state == BOKEH_DEPTH_DONE) {
            mDepthStatus = BOKEH_DEPTH_INVALID;
        } else {
            mDepthStatus = state;
        }
        break;
    default:
        mDepthStatus = state;
        break;
    }

    if (mFlushing) {
        mDepthStatus = BOKEH_DEPTH_INVALID;
    }
    HAL_LOGD("set depth status %d to %d,mDepthTrigger=%d", pre_status,
             mDepthStatus, mDepthTrigger);
}

/*===========================================================================
 * FUNCTION   :setDepthTrigger
 *
 * DESCRIPTION:
 *
 *
 * RETURN     : None
 *==========================================================================*/
void BokehCamera::setDepthTrigger(int vcm) {

    if (mAfstate) {
        mDepthTrigger = BOKEH_TRIGGER_AF;
    } else if (/*mVcmSteps != vcm ||*/ mFlushing) {
        setDepthStatus(BOKEH_DEPTH_INVALID);
        mDepthTrigger = BOKEH_TRIGGER_FALSE;
    }
    HAL_LOGD("mDepthStatus %d,mDepthTrigger=%d,mVcmSteps=%d,vcm=%d,mAfstate=%d",
             mDepthStatus, mDepthTrigger, mVcmSteps, vcm, mAfstate);

    mVcmSteps = vcm;
}

unsigned char *BokehCamera::getaddr(unsigned char *result_buffer_addr,
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
int BokehCamera::insertGDepthMetadata(unsigned char *result_buffer_addr,
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
    depth_size = mBokehCamera->mBokehSize.depth_snap_size;
    xmp_size = BOKEH_REFOCUS_COMMON_XMP_SIZE;
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
    depth_yuv_normalize_size =
        mBokehCamera->mBokehSize.depth_yuv_normalize_size;
#endif
    uint32_t use_size = para_size + depth_yuv_normalize_size + depth_size +
                        mBokehCamera->mOrigJpegSize + jpeg_size + xmp_size;

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
                meta.SetProperty_Float(gDepthURI, "Near", mBokehCamera->near,
                                       0);
            }
            HAL_LOGI("Near1 %u ", mBokehCamera->near);

            double farValueD = 0.0f;
            exists = meta.GetProperty_Float(gDepthURI, "Far", &farValueD, NULL);
            if (exists) {
                HAL_LOGI("Far = %f", farValueD);
            } else {
                meta.SetProperty_Float(gDepthURI, "Far", mBokehCamera->far, 0);
            }
            HAL_LOGI("Far1 %u ", mBokehCamera->far);

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

    mBokehCamera->setJpegSize((char *)result_buffer_addr, result_buffer_size,
                              size);

    return rc;
}

void BokehCamera::encodeOriginalJPEGandDepth(
    string *encodeToBase64String, string *encodeToBase64StringOrigJpeg) {

    // string encodeToBase64String;
    SXMPUtils::EncodeToBase64(
        (char *)mBokehCamera->mBokehDepthBuffer.snap_gdepthjpeg_buffer_addr,
        mBokehCamera->mBokehSize.depth_jepg_size, encodeToBase64String);
    mBokehCamera->unmap(mBokehCamera->mBokehDepthBuffer.snap_gdepthJpg_buffer);
    HAL_LOGI("encodeToBase64String = %s", encodeToBase64String->c_str());
    void *gdepth_ori_jpeg_addr = NULL;
    if (mBokehCamera->map(mBokehCamera->m_pDstGDepthOriJpegBuffer,
                          &gdepth_ori_jpeg_addr) != NO_ERROR) {
        HAL_LOGE("fail to map m_pDstGDepthOriJpegBuffer");
        return;
    }
    SXMPUtils::EncodeToBase64((char *)gdepth_ori_jpeg_addr,
                              mBokehCamera->mGDepthOriJpegSize,
                              encodeToBase64StringOrigJpeg);
    mBokehCamera->unmap(mBokehCamera->m_pDstGDepthOriJpegBuffer);
    HAL_LOGI("encodeToBase64StringOrigJpeg = %s",
             encodeToBase64StringOrigJpeg->c_str());
}

int BokehCamera::jpeg_callback_thread_init(void *p_data) {
    int ret = NO_ERROR;
    pthread_attr_t attr;

    BokehCamera *obj = (BokehCamera *)p_data;

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

void *BokehCamera::jpeg_callback_thread_proc(void *p_data) {
    int ret = 0;
    BokehCamera *obj = (BokehCamera *)p_data;
    HAL_LOGD(" E");
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
    if (rc != NO_ERROR || result_buffer_addr == NULL) {
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
    if (save_param && mBokehCamera->mBokehMode == DUAL_BOKEH_MODE) {
        obj->mCaptureThread->saveCaptureBokehParams(result_buffer_addr,
                                                    obj->mjpegSize, jpeg_size);
#ifdef CONFIG_SUPPORT_GDEPTH
        obj->insertGDepthMetadata(result_buffer_addr, obj->mjpegSize,
                                  jpeg_size);
#endif
        obj->dumpCaptureBokeh(result_buffer_addr, jpeg_size);
    } else {
        obj->setJpegSize((char *)result_buffer_addr, obj->mjpegSize, jpeg_size);
    }
    obj->unmap(output_buffers->buffer);
#ifdef YUV_CONVERT_TO_JPEG
    obj->pushBufferList(obj->mLocalBuffer, obj->m_pDstJpegBuffer,
                        obj->mLocalBufferNumber, obj->mLocalBufferList);
#ifdef CONFIG_SUPPORT_GDEPTH
    obj->pushBufferList(obj->mLocalBuffer, obj->m_pDstGDepthOriJpegBuffer,
                        obj->mLocalBufferNumber, obj->mLocalBufferList);
#endif
#endif
    obj->mCaptureThread->mReprocessing = false;
    obj->mIsCapturing = false;
    obj->CallBackResult(obj->mCapFrameNumber, CAMERA3_BUFFER_STATUS_OK,
                        CAP_TYPE);
    HAL_LOGD(" X");
    return NULL;
}
/*
    {RES_8M, {{3264, 2448}, {960, 720}, {320, 240}}},
    {RES_12M, {{4000, 3000}, {960, 720}, {320, 240}}},
*/
void BokehCamera::getMaxSize(const char *resolution) {
    char value[PROPERTY_VALUE_MAX];
    int32_t jpeg_stream_size = 0;

    if (!strncmp(resolution, "RES_8M", 12)) {
        maxCapWidth = 3264;
        maxCapHeight = 2448;
        maxPrevWidth = 960;
        maxPrevHeight = 720;
    } else if (!strncmp(resolution, "RES_12M", 12)) {
        maxCapWidth = 4000;
        maxCapHeight = 3000;
        maxPrevWidth = 960;
        maxPrevHeight = 720;
    } else {
        HAL_LOGE("Error,not support resolution %s", resolution);
        maxCapWidth = 6528;
        maxCapHeight = 4896;
    }
    // enlarge buffer size for isp debug info for userdebug version
    property_get("ro.debuggable", value, "0");
    if (!strcmp(value, "1")) {
        jpeg_stream_size += 1024 * 1024;
    }
    CMR_LOGI("W %d, H %d, PW %d, PH %d, jpeg_stream_size %d", maxCapWidth,
             maxCapHeight, maxPrevWidth, maxPrevHeight, jpeg_stream_size);
}

int BokehCamera::getStreamTypes(camera3_stream_t *new_stream) {
    int stream_type = 0;
    int format = new_stream->format;
    CMR_LOGD("new_stream w %d, h %d", new_stream->width, new_stream->height);
    if (new_stream->stream_type == CAMERA3_STREAM_OUTPUT) {
        if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) // 34
            format = HAL_PIXEL_FORMAT_YCrCb_420_SP;            // 17
        HAL_LOGD("format %d, usage %d", format, new_stream->usage);
        switch (format) {
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            if (new_stream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                HAL_LOGD("VIDEO_STREAM");
                stream_type = VIDEO_STREAM;
            } else if (new_stream->usage &
                       (uint64_t)BufferUsage::CPU_READ_OFTEN) {
                HAL_LOGD("CALLBACK_STREAM");
                stream_type = CALLBACK_STREAM;
            } else {
                HAL_LOGD("PREVIEW_STREAM");
                stream_type = PREVIEW_STREAM;
            }
            break;
        case HAL_PIXEL_FORMAT_RAW16:
            if (new_stream->usage & (uint64_t)BufferUsage::CPU_READ_OFTEN) {
                stream_type = DEFAULT_STREAM;
            }
            break;
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCbCr_420_888: // 35
            if (new_stream->usage & (uint64_t)BufferUsage::CPU_READ_OFTEN) {
                HAL_LOGD("PREVIEW_STREAM");
                stream_type = PREVIEW_STREAM;
            }
            break;
        case HAL_PIXEL_FORMAT_BLOB:
            HAL_LOGD("SNAPSHOT_STREAM");
            stream_type = SNAPSHOT_STREAM;
            break;
        default:
            HAL_LOGD("DEFAULT_STREAM");
            stream_type = DEFAULT_STREAM;
            break;
        }
    } else if (new_stream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
        stream_type = CALLBACK_STREAM;
    }
    return stream_type;
}

void BokehCamera::preClose(void) {

    HAL_LOGI("E");
    if ((mBokehCamera->mApiVersion == BOKEH_API_MODE) && mBokehAlgo) {
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

void BokehCamera::setResultMetadata(CameraMetadata &metadata,
                                    uint32_t frame_number) {
    uint8_t unavl = 0;
    List<meta_req_t>::iterator itor;
    Mutex::Autolock l(mMetaReqLock);
    itor = mMetadataReqList.begin();
    while (itor != mMetadataReqList.end()) {
        if (frame_number == itor->frame_number) {
            CameraMetadata meta = itor->metadata;
            uint8_t exSceneMode = 0;
            if (meta.exists(ANDROID_CONTROL_EXTENDED_SCENE_MODE))
                exSceneMode =
                    meta.find(ANDROID_CONTROL_EXTENDED_SCENE_MODE).data.u8[0];
            HAL_LOGD("result %d, extend_mode %d", frame_number, exSceneMode);
            metadata.update(ANDROID_CONTROL_EXTENDED_SCENE_MODE, &exSceneMode,
                            1);

            /*ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID*/
            camera_metadata_entry_t entrys =
                metadata.find(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS);
            int count = entrys.count;
            int32_t *keys = new int32_t[count + 1];
            memcpy(keys, entrys.data.i32, entrys.count * sizeof(int32_t));
            keys[count++] = ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID;
            metadata.update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, keys, count);
            delete[] keys;
            metadata.update(ANDROID_LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID,
                            physicIds, mPhysicalIds.size() * 2);

            if (meta.exists(ANDROID_SCALER_CROP_REGION)) {
                for (int i = 0; i < 4; i++) {
                    metadata.find(ANDROID_SCALER_CROP_REGION).data.i32[i] =
                        meta.find(ANDROID_SCALER_CROP_REGION).data.i32[i];
                }
            }
            if (metadata.exists(ANDROID_FLASH_STATE)) {
                metadata.update(ANDROID_FLASH_STATE, &unavl, 1);
            }
            mMetadataReqList.erase(itor);
            break;
        }
        itor++;
    }
}

bool BokehCamera::matchTwoFrame(hwi_frame_buffer_info_t result1,
                                List<hwi_frame_buffer_info_t> &list,
                                hwi_frame_buffer_info_t *result2) {
    List<hwi_frame_buffer_info_t>::iterator itor2;
    int64_t mMatchTimeThreshold = 400;
    if (list.empty()) {
        HAL_LOGE("match failed for idx:%d, unmatched queue is empty",
                 result1.frame_number);
        return MATCH_FAILED;
    } else {
        itor2 = list.begin();
        while (itor2 != list.end()) {
            int64_t diff =
                (int64_t)result1.timestamp - (int64_t)itor2->timestamp;
            HAL_LOGD("T1:%llu,T2:%llu", result1.timestamp, itor2->timestamp);
            if (ns2ms(abs((cmr_s32)diff)) < mMatchTimeThreshold) {
                *result2 = *itor2;
                list.erase(itor2);
                HAL_LOGD("[%d:match:%d],diff=%llu T1:%llu,T2:%llu",
                         result1.frame_number, itor2->frame_number, diff,
                         result1.timestamp, itor2->timestamp);
                return MATCH_SUCCESS;
            }
            itor2++;
        }
        HAL_LOGD("no match for idx:%d, could not find matched frame",
                 result1.frame_number);
        return MATCH_FAILED;
    }
}

int BokehCamera::validateStream(camera3_stream_t *stream) {
    if (stream == NULL) {
        HAL_LOGE("NULL stream");
        return -EINVAL;
    }
    if (stream->stream_type != CAMERA3_STREAM_OUTPUT) {
        HAL_LOGE("dont support stream_type %d", stream->stream_type);
        return -EINVAL;
    }
    int type = getStreamTypes(stream);
    uint32_t width = stream->width, height = stream->height;
    /*
     * [CTS] LogicalCameraDeviceTest#testBasicLogicalPhysicalStreamCombination
     * [CTS] LogicalCameraDeviceTest#testBasicPhysicalStreaming
     * [CTS] NativeCameraDeviceTest#testCameraDeviceLogicalPhysicalStreaming
     *
     * don't support any physical stream for now
     */
    if (stream->physical_camera_id && strlen(stream->physical_camera_id)) {
        HAL_LOGW("assign physical camera id %d to stream is currently not "
                 "supported",
                 atoi(stream->physical_camera_id));
        return -EINVAL;
    }
    if (type == PREVIEW_STREAM) {
        if (width > maxPrevWidth || width < 320 || height > maxPrevHeight ||
            height < 240) {
            HAL_LOGE("dont support prev size (%d, %d)", width, height);
            return -EINVAL;
        }
    } else if (type == SNAPSHOT_STREAM) {
        if (width > maxCapWidth || width < 320 || height > maxCapHeight ||
            height < 240) {
            HAL_LOGE("dont support cap size (%d, %d)", width, height);
            return -EINVAL;
        }
    }
    return 0;
}

void BokehCamera::setPrevBokehSupport() {
    if (mOtpData.otp_exist) {
        char prop[PROPERTY_VALUE_MAX] = {
            0,
        };
        property_get("persist.vendor.cam.pbokeh.enable", prop, "1");
        mIsSupportPBokeh = atoi(prop);
        HAL_LOGD("mIsSupportPBokeh prop %d", mIsSupportPBokeh);
    } else {
        mIsSupportPBokeh = false;
        HAL_LOGD("mIsSupportPBokeh %d", mIsSupportPBokeh);
    }
}

shared_ptr<ICameraBase>
BokehCamera::BokehCreator::createInstance(shared_ptr<Configurator> cfg) {
    if (!cfg)
        return nullptr;
    auto physicalIds = cfg->getPhysicalIds();
    mBokehCamera = new BokehCamera(cfg, physicalIds);
    return shared_ptr<ICameraBase>(mBokehCamera);
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
BokehCamera::PreviewMuxerThread::PreviewMuxerThread() {
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
BokehCamera::PreviewMuxerThread::~PreviewMuxerThread() {
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

bool BokehCamera::PreviewMuxerThread::threadLoop() {
    muxer_queue_msg_t muxer_msg;
    buffer_handle_t *output_buffer = NULL;
    uint32_t frame_number = 0;
    int rc = 0;
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
            HAL_LOGD("ALG mBokehAlgo->initAlgo start");
            rc = mBokehCamera->mBokehAlgo->initAlgo();
            HAL_LOGD("ALG mBokehAlgo->initAlgo end");
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to initAlgo");
                // return rc;
            }
            HAL_LOGD("ALG mBokehAlgo->initPrevDepth start");
            rc = mBokehCamera->mBokehAlgo->initPrevDepth();
            HAL_LOGD("ALG mBokehAlgo->initPrevDepth end");
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to initPrevDepth");
                // return rc;
            }
#ifdef CONFIG_DECRYPT_DEPTH
            char userset[] = "12345";
            rc = mBokehCamera->mBokehAlgo->setUserset(userset, sizeof(userset));
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to setUserset");
                return rc;
            }
#endif
        } break;
        case MUXER_MSG_EXIT: {
            HAL_LOGD("MUXER_MSG_EXIT.");
            List<multi_request_saved_t>::iterator itor =
                mBokehCamera->mSavedRequestList.begin();
            HAL_LOGD("exit frame_number %u", itor->frame_number);
            while (itor != mBokehCamera->mSavedRequestList.end()) {
                frame_number = itor->frame_number;
                itor++;
                mBokehCamera->CallBackResult(
                    frame_number, CAMERA3_BUFFER_STATUS_ERROR, PREV_TYPE);
            }
        }
            return false;
        case MUXER_MSG_DATA_PROC: {
            HAL_LOGD("MUXER_MSG_DATA_PROC");
            {
                Mutex::Autolock l(mBokehCamera->mRequestLock);

                List<multi_request_saved_t>::iterator itor =
                    mBokehCamera->mSavedRequestList.begin();
                HAL_LOGD("muxer_msg.combo_frame.frame_number %d",
                         muxer_msg.combo_frame.frame_number);
                while (itor != mBokehCamera->mSavedRequestList.end()) {
                    if (itor->frame_number ==
                        muxer_msg.combo_frame.frame_number) {
                        output_buffer = itor->buffer;
                        mBokehCamera->mPrevBlurFrameNumber =
                            muxer_msg.combo_frame.frame_number;
                        frame_number = muxer_msg.combo_frame.frame_number;
                        break;
                    }
                    itor++;
                }
            }

            if (output_buffer != NULL) {
                bool isDoDepth = false;
                void *output_buf_addr = NULL;
                void *input_buf1_addr = NULL;
                if ((mBokehCamera->mApiVersion == BOKEH_API_MODE &&
                     mBokehCamera->mBokehMode != PORTRAIT_MODE) ||
                    (mBokehCamera->mApiVersion == BOKEH_API_MODE &&
                     mBokehCamera->mDoPortrait &&
                     mBokehCamera->mBokehMode == PORTRAIT_MODE)) {
                    rc = sprdBokehPreviewHandle(output_buffer,
                                                muxer_msg.combo_frame.buffer1);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("sprdBokehPreviewHandle failed");
                        return false;
                    }
                    HAL_LOGD("mDepthTrigger %d", mBokehCamera->mDepthTrigger);
                    if (mBokehCamera->mDepthTrigger != BOKEH_TRIGGER_FALSE &&
                        mBokehCamera->mOtpData.otp_exist) {
                        isDoDepth = sprdDepthHandle(&muxer_msg);
                    }
                } else {

                    rc = mBokehCamera->map(output_buffer, &output_buf_addr);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("fail to map output buffer");
                        goto fail_map_output;
                    }
                    rc = mBokehCamera->map(muxer_msg.combo_frame.buffer1,
                                           &input_buf1_addr);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("fail to map input buffer");
                        goto fail_map_input;
                    }

                    memcpy(output_buf_addr, input_buf1_addr,
                           ADP_BUFSIZE(*muxer_msg.combo_frame.buffer1));
                    mBokehCamera->flushIonBuffer(ADP_BUFFD(*output_buffer),
                                                 output_buf_addr,
                                                 ADP_BUFSIZE(*output_buffer));

                    mBokehCamera->unmap(muxer_msg.combo_frame.buffer1);
                fail_map_input:
                    mBokehCamera->unmap(output_buffer);
                fail_map_output:
                    rc = NO_ERROR;
                }
                if (!isDoDepth) {
                    mBokehCamera->pushBufferList(
                        mBokehCamera->mLocalBuffer,
                        muxer_msg.combo_frame.buffer1,
                        mBokehCamera->mLocalBufferNumber,
                        mBokehCamera->mLocalBufferList);
                    mBokehCamera->pushBufferList(
                        mBokehCamera->mLocalBuffer,
                        muxer_msg.combo_frame.buffer2,
                        mBokehCamera->mLocalBufferNumber,
                        mBokehCamera->mLocalBufferList);
                }
                mBokehCamera->CallBackResult(
                    frame_number, CAMERA3_BUFFER_STATUS_OK, PREV_TYPE);
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
bool BokehCamera::PreviewMuxerThread::sprdDepthHandle(
    muxer_queue_msg_t *muxer_msg) {
    bool ret = false;
    muxer_queue_msg_t send_muxer_msg = *muxer_msg;

    Mutex::Autolock l(mBokehCamera->mDepthMuxerThread->mMergequeueMutex);
    HAL_LOGD("Enqueue combo frame:%d for frame depth!,size=%d",
             send_muxer_msg.combo_frame.frame_number,
             mBokehCamera->mDepthMuxerThread->mDepthMuxerMsgList.size());
    if (mBokehCamera->mDepthMuxerThread->mDepthMuxerMsgList.empty()) {
        HAL_LOGD("mDepthMuxerMsgList.push_back");
        mBokehCamera->mDepthMuxerThread->mDepthMuxerMsgList.push_back(
            send_muxer_msg);
        mBokehCamera->mDepthMuxerThread->mMergequeueSignal.signal();
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
int BokehCamera::PreviewMuxerThread::sprdBokehPreviewHandle(
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
    buffer_index = mBokehCamera->getPrevDepthBufferFlag(BOKEH_BUFFER_PANG);
    HAL_LOGD(
        "buffer_index:%d,mIsSupportPBokeh:%d,mDepthStatus:%d,mDepthTrigger:%d",
        buffer_index, mBokehCamera->mIsSupportPBokeh,
        mBokehCamera->mDepthStatus, mBokehCamera->mDepthTrigger);
    if (mBokehCamera->mIsSupportPBokeh &&
        (mBokehCamera->mDepthStatus == BOKEH_DEPTH_DONE ||
         (mBokehCamera->mDepthStatus == BOKEH_DEPTH_DONING &&
          mBokehCamera->mDepthTrigger == BOKEH_TRIGGER_FNUM)) &&
        buffer_index != -1) {
        unsigned char *depth_buffer =
            (unsigned char *)(mBokehCamera->mBokehDepthBuffer
                                  .prev_depth_buffer[buffer_index]
                                  .buffer);
        /*Bokeh GPU interface start*/
        uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                                GraphicBuffer::USAGE_SW_READ_OFTEN |
                                GraphicBuffer::USAGE_SW_WRITE_OFTEN;
        int32_t yuvTextFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        uint32_t inWidth = 0, inHeight = 0, inStride = 0;
        if (!mBokehCamera->mIommuEnabled) {
            yuvTextUsage |= GRALLOC_USAGE_VIDEO_BUFFER;
        }

#if defined(CONFIG_SPRD_ANDROID_8)
        uint32_t inLayCount = 1;
#endif
        inWidth = mBokehCamera->mBokehSize.preview_w;
        inHeight = mBokehCamera->mBokehSize.preview_h;
        inStride = mBokehCamera->mBokehSize.preview_w;
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
        HAL_LOGD("ALG mBokehAlgo->prevBluImage start");
        mBokehCamera->mBokehAlgo->prevBluImage(srcBuffer, dstBuffer,
                                               (void *)depth_buffer);
        HAL_LOGD("ALG mBokehAlgo->prevBluImage end");
        {
            char prop[PROPERTY_VALUE_MAX] = {
                0,
            };

            property_get("persist.vendor.cam.bokeh.dump", prop, "0");
            if (!strcmp(prop, "sprdpreyuv") || !strcmp(prop, "all")) {
                rc = mBokehCamera->map(output_buf, &output_buf_addr);
                if (rc != NO_ERROR) {
                    HAL_LOGE("fail to map output buffer");
                }
                mBokehCamera->dumpData((unsigned char *)output_buf_addr, 1,
                                       ADP_BUFSIZE(*output_buf),
                                       mBokehCamera->mBokehSize.preview_w,
                                       mBokehCamera->mBokehSize.preview_h,
                                       mBokehCamera->mPrevBlurFrameNumber,
                                       "bokeh");
                mBokehCamera->unmap(output_buf);
            }
        }
        mBokehCamera->setPrevDepthBufferFlag(BOKEH_BUFFER_PANG, buffer_index);

    } else {

        rc = mBokehCamera->map(output_buf, &output_buf_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map output buffer");
            goto fail_map_output;
        }
        rc = mBokehCamera->map(input_buf1, &input_buf1_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map input buffer");
            goto fail_map_input;
        }

        memcpy(output_buf_addr, input_buf1_addr, ADP_BUFSIZE(*input_buf1));
        mBokehCamera->flushIonBuffer(ADP_BUFFD(*output_buf), output_buf_addr,
                                     ADP_BUFSIZE(*output_buf));

        mBokehCamera->unmap(input_buf1);
    fail_map_input:
        mBokehCamera->unmap(output_buf);
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
void BokehCamera::PreviewMuxerThread::requestInit() {
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
void BokehCamera::PreviewMuxerThread::requestExit() {
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
void BokehCamera::PreviewMuxerThread::waitMsgAvailable() {
    while (mPreviewMuxerMsgList.empty()) {
        {
            Mutex::Autolock l(mMergequeueMutex);
            mMergequeueSignal.waitRelative(mMergequeueMutex,
                                           BOKEH_THREAD_TIMEOUT);
        }
        struct timespec t1;
        clock_gettime(CLOCK_BOOTTIME, &t1);
        uint64_t now_time_stamp = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
        if (mBokehCamera->mReqTimestamp &&
            mBokehCamera->mPrevFrameNumber != 0 &&
            (ns2ms(now_time_stamp - mBokehCamera->mReqTimestamp) >
             CLEAR_PRE_FRAME_UNMATCH_TIMEOUT)) {
            HAL_LOGD("clear unmatch frame for force kill app");
            Mutex::Autolock l(mBokehCamera->mUnmatchedQueueLock);
            mBokehCamera->clearFrameNeverMatched(
                mBokehCamera->mPrevFrameNumber + 1,
                mBokehCamera->mPrevFrameNumber + 1);
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
BokehCamera::DepthMuxerThread::DepthMuxerThread() {
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
BokehCamera::DepthMuxerThread::~DepthMuxerThread() {
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

bool BokehCamera::DepthMuxerThread::threadLoop() {
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
        case MUXER_MSG_EXIT:
            HAL_LOGD("MUXER_MSG_EXIT.");
            {
                Mutex::Autolock l(mMergequeueMutex);
                mDepthMuxerMsgList.erase(it);
            }
            return false;
        case MUXER_MSG_DATA_PROC: {
            // Mutex::Autolock l(mLock);
            HAL_LOGD("MUXER_MSG_DATA_PROC.");
            mBokehCamera->setDepthStatus(BOKEH_DEPTH_DONING);
            rc = sprdDepthDo(muxer_msg.combo_frame.buffer1,
                             muxer_msg.combo_frame.buffer2);
            if (rc != NO_ERROR) {
                HAL_LOGE("sprdDepthDo failed");
                mBokehCamera->setDepthStatus(BOKEH_DEPTH_INVALID);
            } else {
                HAL_LOGD("sprdDepthDo success.");
            }

            mBokehCamera->pushBufferList(mBokehCamera->mLocalBuffer,
                                         muxer_msg.combo_frame.buffer1,
                                         mBokehCamera->mLocalBufferNumber,
                                         mBokehCamera->mLocalBufferList);
            mBokehCamera->pushBufferList(mBokehCamera->mLocalBuffer,
                                         muxer_msg.combo_frame.buffer2,
                                         mBokehCamera->mLocalBufferNumber,
                                         mBokehCamera->mLocalBufferList);
            {
                Mutex::Autolock l(mMergequeueMutex);
                mDepthMuxerMsgList.erase(it);
            }

        } break;
        default:
            break;
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
int BokehCamera::DepthMuxerThread::sprdDepthDo(buffer_handle_t *input_buf1,
                                               buffer_handle_t *input_buf2) {
    int rc = NO_ERROR;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    int buffer_index = 0;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (input_buf1 == NULL || input_buf2 == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }

    HAL_LOGD("map1 start");
    rc = mBokehCamera->map(input_buf1, &input_buf1_addr);
    HAL_LOGD("map1 end");
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer1");
        goto fail_map_input1;
    }
    HAL_LOGD("map2 start");
    rc = mBokehCamera->map(input_buf2, &input_buf2_addr);
    HAL_LOGD("map2 end");
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer2");
        goto fail_map_input2;
    }
    HAL_LOGD("buf1:%p,buf2:%p", input_buf1_addr, input_buf2_addr);

    property_get("persist.vendor.cam.bokehcamera.dump", prop, "all");
    if (!strcmp(prop, "predepth") || !strcmp(prop, "all")) {
        mBokehCamera->dumpData((unsigned char *)input_buf1_addr, 1,
                               ADP_BUFSIZE(*input_buf1),
                               mBokehCamera->mBokehSize.preview_w,
                               mBokehCamera->mBokehSize.preview_h,
                               mBokehCamera->mPrevBlurFrameNumber, "buf1_0");
        mBokehCamera->dumpData((unsigned char *)input_buf2_addr, 1,
                               ADP_BUFSIZE(*input_buf2),
                               mBokehCamera->mBokehSize.depth_prev_sub_w,
                               mBokehCamera->mBokehSize.depth_prev_sub_h,
                               mBokehCamera->mPrevBlurFrameNumber, "buf2_0");
    }
    buffer_index = mBokehCamera->getPrevDepthBufferFlag(BOKEH_BUFFER_PING);
    HAL_LOGD("PING buffer_index %d", buffer_index);
    if (buffer_index == -1) {
        goto fail_map_input3;
    }

    mBokehCamera->mLastOnlieVcm = 0;
    rc = mBokehCamera->mBokehAlgo->onLine(
        mBokehCamera->mBokehDepthBuffer.depth_out_map_table, input_buf1_addr,
        input_buf2_addr,
        mBokehCamera->mBokehDepthBuffer.prev_depth_scale_buffer);
    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("Sprd algo onLine failed! %d", rc);
        mBokehCamera->mLastOnlieVcm = 0;
        goto fail_map_input3;
    } else {
        mBokehCamera->mLastOnlieVcm = mBokehCamera->mVcmSteps;
        HAL_LOGD("vcm %d", mBokehCamera->mLastOnlieVcm);
    }

    HAL_LOGD("buf1:%p,buf2:%p", input_buf1_addr, input_buf2_addr);
    property_get("persist.vendor.cam.bokehcamera.dump", prop, "all");
    if (!strcmp(prop, "predepth") || !strcmp(prop, "all")) {
        mBokehCamera->dumpData((unsigned char *)input_buf1_addr, 1,
                               ADP_BUFSIZE(*input_buf1),
                               mBokehCamera->mBokehSize.preview_w,
                               mBokehCamera->mBokehSize.preview_h,
                               mBokehCamera->mPrevBlurFrameNumber, "buf1_1");
        mBokehCamera->dumpData((unsigned char *)input_buf2_addr, 1,
                               ADP_BUFSIZE(*input_buf2),
                               mBokehCamera->mBokehSize.depth_prev_sub_w,
                               mBokehCamera->mBokehSize.depth_prev_sub_h,
                               mBokehCamera->mPrevBlurFrameNumber, "buf2_1");
    }
    HAL_LOGD(
        "buffer_index %d,w_flag %d, r_flag %d", buffer_index,
        mBokehCamera->mBokehDepthBuffer.prev_depth_buffer[buffer_index].w_flag,
        mBokehCamera->mBokehDepthBuffer.prev_depth_buffer[buffer_index].r_flag);
    HAL_LOGD(
        "ALG mBokehAlgo->prevDepthRun start %p, %p, %p, %p  ",
        mBokehCamera->mBokehDepthBuffer.prev_depth_buffer[buffer_index].buffer,
        input_buf1_addr, input_buf2_addr,
        mBokehCamera->mBokehDepthBuffer.prev_depth_scale_buffer);
    rc = mBokehCamera->mBokehAlgo->prevDepthRun(
        mBokehCamera->mBokehDepthBuffer.prev_depth_buffer[buffer_index].buffer,
        input_buf1_addr, input_buf2_addr,
        mBokehCamera->mBokehDepthBuffer.prev_depth_scale_buffer);
    HAL_LOGD("ALG mBokehAlgo->prevDepthRun end");
    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("sprd_depth_Run_distance failed! %d", rc);
        goto fail_map_input3;
    }
    mBokehCamera->setPrevDepthBufferFlag(BOKEH_BUFFER_PING, buffer_index);
    mBokehCamera->setDepthStatus(BOKEH_DEPTH_DONE);
    HAL_LOGD("buf1:%p,buf2:%p", input_buf1_addr, input_buf2_addr);
    {
        property_get("persist.vendor.cam.bokeh.dump", prop, "0");
        if (!strcmp(prop, "predepth") || !strcmp(prop, "all")) {
            mBokehCamera->dumpData(
                (unsigned char *)mBokehCamera->mBokehDepthBuffer
                    .prev_depth_buffer[buffer_index]
                    .buffer,
                6, mBokehCamera->mBokehSize.depth_prev_size,
                mBokehCamera->mBokehSize.preview_w,
                mBokehCamera->mBokehSize.preview_h,
                mBokehCamera->mPrevBlurFrameNumber, "depth");
            mBokehCamera->dumpData(
                (unsigned char *)input_buf2_addr, 1, ADP_BUFSIZE(*input_buf2),
                mBokehCamera->mBokehSize.depth_prev_sub_w,
                mBokehCamera->mBokehSize.depth_prev_sub_h,
                mBokehCamera->mPrevBlurFrameNumber, "prevSub");
            mBokehCamera->dumpData(
                (unsigned char *)input_buf1_addr, 1, ADP_BUFSIZE(*input_buf1),
                mBokehCamera->mBokehSize.preview_w,
                mBokehCamera->mBokehSize.preview_h,
                mBokehCamera->mPrevBlurFrameNumber, "prevMain");
        }
    }
fail_map_input3:
    mBokehCamera->unmap(input_buf2);
fail_map_input2:
    mBokehCamera->unmap(input_buf1);
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
void BokehCamera::DepthMuxerThread::requestExit() {
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
 * DESCRIPTION: wait util list fif data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void BokehCamera::DepthMuxerThread::waitMsgAvailable() {
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
BokehCamera::BokehCaptureThread::BokehCaptureThread() {
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
    memset(&mGDepthOutputParam, 0, sizeof(gdepth_outparam));

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
BokehCamera::BokehCaptureThread::~BokehCaptureThread() {
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
int BokehCamera::BokehCaptureThread::saveCaptureBokehParams(
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
    mBokehCamera->mBokehAlgo->getBokenParam((void *)&param);
    if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {
        if (result_buffer_addr == NULL) {
            HAL_LOGE("result_buff is NULL!");
            return BAD_VALUE;
        }
        xmp_size = BOKEH_REFOCUS_COMMON_XMP_SIZE;
        para_size = BOKEH_REFOCUS_COMMON_PARAM_NUM * 4;
        depth_size = mBokehCamera->mBokehSize.depth_snap_size;
#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        depth_yuv_normalize_size =
            mBokehCamera->mBokehSize.depth_yuv_normalize_size;
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

    uint32_t orig_yuv_size = mBokehCamera->mBokehSize.capture_w *
                             mBokehCamera->mBokehSize.capture_h * 3 / 2;
#ifdef YUV_CONVERT_TO_JPEG
    uint32_t use_size = para_size + depth_yuv_normalize_size + depth_size +
                        mBokehCamera->mOrigJpegSize + jpeg_size + xmp_size;
    uint32_t orig_jpeg_size = mBokehCamera->mOrigJpegSize;
    uint32_t process_jpeg = jpeg_size;
#else
    uint32_t use_size = para_size + depth_size + orig_yuv_size + jpeg_size;
#endif
    if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {

        depth_yuv =
            (unsigned char *)mBokehCamera->mBokehDepthBuffer.snap_depth_buffer;

#ifdef CONFIG_SUPPORT_GDEPTH
        depthJpg =
            (unsigned char *)
                mBokehCamera->mBokehDepthBuffer.snap_gdepthjpeg_buffer_addr;
#endif

#ifdef CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV
        depth_yuv_normalize =
            (unsigned char *)
                mBokehCamera->mBokehDepthBuffer.depth_normalize_data_buffer;
#endif

        // refocus commom param
        uint32_t far = mBokehCamera->far;
        uint32_t near = mBokehCamera->near;
        uint32_t format = 1;
        uint32_t mime = 0;
        uint32_t param_num = BOKEH_REFOCUS_COMMON_PARAM_NUM;
        uint32_t main_width = mBokehCamera->mBokehSize.capture_w;
        uint32_t main_height = mBokehCamera->mBokehSize.capture_h;
        uint32_t depth_width = mBokehCamera->mBokehSize.depth_snap_out_w;
        uint32_t depth_height = mBokehCamera->mBokehSize.depth_snap_out_h;
        uint32_t bokeh_level = param.sprd.cap.bokeh_level;
        uint32_t position_x = param.sprd.cap.sel_x;
        uint32_t position_y = param.sprd.cap.sel_y;
        uint32_t param_state = param.sprd.cap.param_state;
        uint32_t rotation =
            SprdCamera3Setting::s_setting[mBokehCamera->m_pPhyCamera[0].id]
                .jpgInfo.orientation;
        uint32_t bokeh_version = 0;
#ifndef CONFIG_SUPPORT_GDEPTH
        bokeh_version = 0x1C0101;
#else
        bokeh_version = 0x11c0101;
#endif

#ifdef CONFIG_DECRYPT_DEPTH
        uint32_t decrypt_mode = 1;
#else
        uint32_t decrypt_mode = 0;
#endif

        unsigned char bokeh_flag[] = {'B', 'O', 'K', 'E'};
        depth_size = mBokehCamera->mBokehSize.depth_snap_size;
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
    ret = mBokehCamera->map(mBokehCamera->m_pDstJpegBuffer,
                            (void **)&orig_jpeg_data);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map dst jpeg buffer");
        goto fail_map_dst;
    }
    buffer_base -= orig_jpeg_size;
    memcpy(buffer_base, orig_jpeg_data, orig_jpeg_size);
#else
    // cpoy original yuv
    ret = mBokehCamera->map(mBokehCamera->m_pMainSnapBuffer,
                            (void **)&orig_yuv_data);
    if (ret != NO_ERROR) {
        HAL_LOGE("fail to map snap buffer");
        goto fail_map_dst;
    }
    buffer_base -= orig_yuv_size;
    memcpy(buffer_base, orig_yuv_data, orig_yuv_size);
#endif
#ifndef CONFIG_SUPPORT_GDEPTH
    // blob to indicate all image size(use_size)
    mBokehCamera->setJpegSize((char *)result_buffer_addr, result_buffer_size,
                              use_size);
#endif

#ifdef YUV_CONVERT_TO_JPEG
    mBokehCamera->unmap(mBokehCamera->m_pDstJpegBuffer);
#else
    mBokehCamera->unmap(mBokehCamera->m_pMainSnapBuffer);
#endif

fail_map_dst:
    return ret;
}

/*===========================================================================
 * FUNCTION   :reprocessReq
 *
 * DESCRIPTION: reprocessReq
 *
 * PARAMETERS : buffer_handle_t *output_buffer, capture_queue_msg_t
 * capture_msg
 *               buffer_handle_t *depth_output_buffer, buffer_handle_t
 * scaled_buffer
 *
 * RETURN     : None
 *==========================================================================*/
void BokehCamera::BokehCaptureThread::reprocessReq(
    buffer_handle_t *output_buffer, capture_queue_msg_t capture_msg) {
    camera3_capture_request_t request = mSavedCapRequest;
    camera3_stream_buffer_t output_buffers;
    camera3_stream_buffer_t input_buffer;

    Mutex::Autolock l(mBokehCamera->mFlushLock);
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
        &(mBokehCamera->mMainStreams[mBokehCamera->mCaptureStreamsNum]);

    memcpy((void *)&output_buffers, &mSavedCapReqStreamBuff,
           sizeof(camera3_stream_buffer_t));
    output_buffers.stream =
        &(mBokehCamera->mMainStreams[mBokehCamera->mCaptureStreamsNum]);
#ifdef BOKEH_YUV_DATA_TRANSFORM
    buffer_handle_t *transform_buffer = NULL;
    if (mBokehCamera->mBokehSize.transform_w != 0) {
        transform_buffer = (mBokehCamera->popBufferList(
            mBokehCamera->mLocalBufferList, SNAPSHOT_TRANSFORM_BUFFER));
        buffer_handle_t *output_handle = NULL;
        void *output_handle_addr = NULL;
        void *transform_buffer_addr = NULL;

        if ((!mAbokehGallery || (mBokehCamera->mDoPortrait &&
                                 mBokehCamera->mBokehMode == PORTRAIT_MODE)) &&
            mBokehResult) {
            output_handle = output_buffer;
        } else {
            output_handle = capture_msg.combo_buff.buffer1;
        }
        int rc = NO_ERROR;
        rc = mBokehCamera->map(output_handle, &output_handle_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map output buffer");
        }
        rc = mBokehCamera->map(transform_buffer, &transform_buffer_addr);
        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map transform buffer");
        }

        int64_t alignTransform = systemTime();
        mBokehCamera->alignTransform(
            output_handle_addr, mBokehCamera->mBokehSize.capture_w,
            mBokehCamera->mBokehSize.capture_h,
            mBokehCamera->mBokehSize.transform_w,
            mBokehCamera->mBokehSize.transform_h, transform_buffer_addr);
        HAL_LOGD("alignTransform run cost %lld ms",
                 ns2ms(systemTime() - alignTransform));

        input_buffer.stream->width = mBokehCamera->mBokehSize.transform_w;
        input_buffer.stream->height = mBokehCamera->mBokehSize.transform_h;
        input_buffer.buffer = transform_buffer;
        output_buffers.stream->width = mBokehCamera->mBokehSize.transform_w;
        output_buffers.stream->height = mBokehCamera->mBokehSize.transform_h;
        mBokehCamera->unmap(output_handle);
        mBokehCamera->unmap(transform_buffer);
    } else {
        input_buffer.stream->width = mBokehCamera->mBokehSize.capture_w;
        input_buffer.stream->height = mBokehCamera->mBokehSize.capture_h;
        output_buffers.stream->width = mBokehCamera->mBokehSize.capture_w;
        output_buffers.stream->height = mBokehCamera->mBokehSize.capture_h;
        if ((!mAbokehGallery || (mBokehCamera->mDoPortrait &&
                                 mBokehCamera->mBokehMode == PORTRAIT_MODE)) &&
            mBokehResult) {
            input_buffer.buffer = output_buffer;
        } else {
            input_buffer.buffer = capture_msg.combo_buff.buffer1;
        }
    }
#else
    input_buffer.stream->width = mBokehCamera->mBokehSize.capture_w;
    input_buffer.stream->height = mBokehCamera->mBokehSize.capture_h;
    output_buffers.stream->width = mBokehCamera->mBokehSize.capture_w;
    output_buffers.stream->height = mBokehCamera->mBokehSize.capture_h;
    if ((!mAbokehGallery || (mBokehCamera->mDoPortrait &&
                             mBokehCamera->mBokehMode == PORTRAIT_MODE)) &&
        mBokehResult) {
        input_buffer.buffer = output_buffer;
    } else {
        input_buffer.buffer = capture_msg.combo_buff.buffer1;
    }

#endif

    request.num_output_buffers = 1;
    request.frame_number = mBokehCamera->mCapFrameNumber;
    request.output_buffers = &output_buffers;
    request.input_buffer = &input_buffer;
    mReprocessing = true;
    if (!mAbokehGallery || (mBokehCamera->mDoPortrait &&
                            mBokehCamera->mBokehMode == PORTRAIT_MODE)) {
        mBokehCamera->pushBufferList(mBokehCamera->mLocalBuffer, output_buffer,
                                     mBokehCamera->mLocalBufferNumber,
                                     mBokehCamera->mLocalBufferList);
    }
#ifdef BOKEH_YUV_DATA_TRANSFORM
    if (mBokehCamera->mBokehSize.transform_w != 0) {
        mBokehCamera->pushBufferList(
            mBokehCamera->mLocalBuffer, transform_buffer,
            mBokehCamera->mLocalBufferNumber, mBokehCamera->mLocalBufferList);
    }
#endif
    mBokehCamera->pushBufferList(
        mBokehCamera->mLocalBuffer, capture_msg.combo_buff.buffer1,
        mBokehCamera->mLocalBufferNumber, mBokehCamera->mLocalBufferList);

    if (!mBokehCamera->mIsHdrMode) {
        mBokehCamera->pushBufferList(
            mBokehCamera->mLocalBuffer, capture_msg.combo_buff.buffer2,
            mBokehCamera->mLocalBufferNumber, mBokehCamera->mLocalBufferList);
    }

    if (mBokehCamera->mFlushing) {
        mBokehCamera->CallBackResult(mBokehCamera->mCapFrameNumber,
                                     CAMERA3_BUFFER_STATUS_ERROR, CAP_TYPE);
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
bool BokehCamera::BokehCaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    buffer_handle_t *main_buffer = NULL;
    void *src_vir_addr = NULL;
    void *pic_vir_addr = NULL;
    void *pic_jpg_vir_addr = NULL;
    capture_queue_msg_t capture_msg;
    int rc = 0;
    int mime_type = 0;

    while (!mCaptureMsgList.empty()) {
        {
            Mutex::Autolock l(mMergequeueMutex);
            List<capture_queue_msg_t>::iterator itor1 = mCaptureMsgList.begin();
            capture_msg = *itor1;
            // The specail case is continuous 2 frames data of main camera,
            // but data of aux camera has't come in.
            if (mBokehCamera->mIsCapDepthFinish == false &&
                capture_msg.combo_buff.buffer1 != NULL &&
                capture_msg.combo_buff.buffer2 == NULL &&
                mBokehCamera->mIsHdrMode && mBokehCamera->mOtpData.otp_exist) {
                mBokehCamera->mHdrSkipBlur = true;
                HAL_LOGI("frame is hdr, and depth hasn't do");
                break;
            }
            mCaptureMsgList.erase(itor1);
        }
        switch (capture_msg.msg_type) {
        case MSG_EXIT: {
            // flush queue
            HAL_LOGI("MSG_EXIT,mCapFrameNumber=%d",
                     mBokehCamera->mCapFrameNumber);
            if (mBokehCamera->mCapFrameNumber != -1) {
                HAL_LOGE("error.HAL don't send capture frame");
                mBokehCamera->CallBackResult(mBokehCamera->mCapFrameNumber,
                                             CAMERA3_BUFFER_STATUS_ERROR,
                                             CAP_TYPE);
            }
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqStreamBuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            return false;
        }
        case MSG_DATA_PROC: {
            HAL_LOGD("MSG_DATA_PROC,mCapFrameNumber=%d",
                     mBokehCamera->mCapFrameNumber);
            void *input_buf1_addr = NULL;
            rc = mBokehCamera->map(capture_msg.combo_buff.buffer1,
                                   &input_buf1_addr);
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to map input buffer1");
                return false;
            }
#ifdef CONFIG_FACE_BEAUTY
            if (mBokehCamera->mPerfectskinlevel.smoothLevel > 0 &&
                mBokehCamera->mFaceInfo[2] - mBokehCamera->mFaceInfo[0] > 0 &&
                mBokehCamera->mFaceInfo[3] - mBokehCamera->mFaceInfo[1] > 0 &&
                capture_msg.combo_buff.buffer1) {
                mBokehCamera->bokehFaceMakeup(capture_msg.combo_buff.buffer1,
                                              input_buf1_addr);
            }
#endif
            mBokehResult = true;
            if (!mAbokehGallery ||
                (mBokehCamera->mDoPortrait &&
                 mBokehCamera->mBokehMode == PORTRAIT_MODE)) {
                output_buffer = (mBokehCamera->popBufferList(
                    mBokehCamera->mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
            }
            if (mBokehCamera->mIsHdrMode &&
                capture_msg.combo_buff.buffer1 != NULL &&
                capture_msg.combo_buff.buffer2 == NULL) {
                HAL_LOGD("start process hdr frame to get "
                         "bokeh data!");
#ifdef YUV_CONVERT_TO_JPEG
                mBokehCamera->m_pDstJpegBuffer = (mBokehCamera->popBufferList(
                    mBokehCamera->mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
                rc = mBokehCamera->map(mBokehCamera->m_pDstJpegBuffer,
                                       &pic_vir_addr);
                if (rc != NO_ERROR) {
                    HAL_LOGE("fail to map jpeg buffer");
                }

                mBokehCamera->mOrigJpegSize =
                    mBokehCamera->jpeg_encode_exif_simplify(
                        capture_msg.combo_buff.buffer1, input_buf1_addr,
                        mBokehCamera->m_pDstJpegBuffer, pic_vir_addr, NULL,
                        NULL, mBokehCamera->m_pPhyCamera[CAM_BOKEH_MAIN].hwi,
                        SprdCamera3Setting::s_setting
                            [mBokehCamera->m_pPhyCamera[0].id]
                                .jpgInfo.orientation);
                mBokehCamera->unmap(mBokehCamera->m_pDstJpegBuffer);
#else
                mBokehCamera->m_pMainSnapBuffer =
                    capture_msg.combo_buff.buffer1;
#endif
                if (!mAbokehGallery) {
                    rc = sprdBokehCaptureHandle(output_buffer,
                                                capture_msg.combo_buff.buffer1,
                                                input_buf1_addr);
                    if (rc != NO_ERROR) {
                        mBokehCamera->mOrigJpegSize = 0;
                        mBokehResult = false;
                        HAL_LOGE("sprdBokehCaptureHandle failed");
                    }
                }
                mBokehCamera->unmap(capture_msg.combo_buff.buffer1);
                reprocessReq(output_buffer, capture_msg);
                break;
            }
            HAL_LOGD("start process normal frame to get "
                     "depth data!");
            if (mBokehCamera->mApiVersion == BOKEH_API_MODE) {
                HAL_LOGD("mBokehCamera->mOtpData.otp_exist %d",
                         mBokehCamera->mOtpData.otp_exist);
                if ((mBokehCamera->mOtpData.otp_exist &&
                     mBokehCamera->mBokehMode == DUAL_BOKEH_MODE) ||
                    (mBokehCamera->mDoPortrait &&
                     mBokehCamera->mBokehMode == PORTRAIT_MODE)) {
                    rc = sprdDepthCaptureHandle(capture_msg.combo_buff.buffer1,
                                                input_buf1_addr,
                                                capture_msg.combo_buff.buffer2);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("depthCaptureHandle failed");
                        mBokehResult = false;
                    }
                } else {
                    mBokehResult = false;
                }
                HAL_LOGI("mBokehCamera->mIsHdrMode=%d,mAbokehGallery=%d,"
                         "mBokehResult=%d",
                         mBokehCamera->mIsHdrMode, mAbokehGallery,
                         mBokehResult);
                if ((!mBokehCamera->mIsHdrMode && !mAbokehGallery &&
                     (mBokehResult == true) &&
                     mBokehCamera->mBokehMode == DUAL_BOKEH_MODE) ||
                    (mBokehCamera->mDoPortrait &&
                     mBokehCamera->mBokehMode == PORTRAIT_MODE)) {
                    rc = sprdBokehCaptureHandle(output_buffer,
                                                capture_msg.combo_buff.buffer1,
                                                input_buf1_addr);
                    if (rc != NO_ERROR) {
                        mBokehCamera->mOrigJpegSize = 0;
                        mBokehResult = false;
                        HAL_LOGE("sprdBokehCaptureHandle failed");
                    }
                }
            }
            if (mBokehResult == false) {
                mime_type = 0;
            } else if (mAbokehGallery) {
                if (mBokehCamera->mIsHdrMode) {
                    mime_type = (1 << 8) | (int)SPRD_MIMETPYE_BOKEH_HDR;
                } else {
                    mime_type = (1 << 8) | (int)SPRD_MIMETPYE_BOKEH;
                }
            } else {
                if (mBokehCamera->mIsHdrMode) {
                    mime_type = (int)SPRD_MIMETPYE_BOKEH_HDR;
                } else {
                    mime_type = (int)SPRD_MIMETPYE_BOKEH;
                }
            }
            HAL_LOGD("mime_type %d", mime_type);
            if (mBokehCamera->mBokehMode == PORTRAIT_MODE) {
                mime_type = SPRD_MIMETPYE_NONE;
            }
            if (!mBokehCamera->mIsHdrMode) {
#ifdef YUV_CONVERT_TO_JPEG
                mBokehCamera->m_pDstJpegBuffer = (mBokehCamera->popBufferList(
                    mBokehCamera->mLocalBufferList, SNAPSHOT_MAIN_BUFFER));
                if (mBokehResult) {
                    rc = mBokehCamera->map(mBokehCamera->m_pDstJpegBuffer,
                                           &pic_vir_addr);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("fail to map jpeg buffer");
                    }

                    mBokehCamera->mOrigJpegSize =
                        mBokehCamera->jpeg_encode_exif_simplify(
                            capture_msg.combo_buff.buffer1, input_buf1_addr,
                            mBokehCamera->m_pDstJpegBuffer, pic_vir_addr, NULL,
                            NULL,
                            mBokehCamera->m_pPhyCamera[CAM_BOKEH_MAIN].hwi,
                            SprdCamera3Setting::s_setting
                                [mBokehCamera->m_pPhyCamera[0].id]
                                    .jpgInfo.orientation);
                    mBokehCamera->unmap(mBokehCamera->m_pDstJpegBuffer);
#ifdef CONFIG_SUPPORT_GDEPTH
                    mBokehCamera->m_pDstGDepthOriJpegBuffer =
                        (mBokehCamera->popBufferList(
                            mBokehCamera->mLocalBufferList,
                            SNAPSHOT_MAIN_BUFFER));
                    rc = mBokehCamera->map(
                        mBokehCamera->m_pDstGDepthOriJpegBuffer, &pic_vir_addr);
                    if (rc != NO_ERROR) {
                        HAL_LOGE("fail to map GDepth jpeg buffer");
                    }
                    mBokehCamera->mGDepthOriJpegSize =
                        mBokehCamera->jpeg_encode_exif_simplify_format(
                            capture_msg.combo_buff.buffer1, input_buf1_addr,
                            mBokehCamera->m_pDstGDepthOriJpegBuffer,
                            pic_vir_addr, NULL, NULL,
                            mBokehCamera->m_pPhyCamera[CAM_BOKEH_MAIN].hwi,
                            IMG_DATA_TYPE_YUV420,
                            SprdCamera3Setting::s_setting
                                [mBokehCamera->m_pPhyCamera[0].id]
                                    .jpgInfo.orientation,
                            0);
                    mBokehCamera->unmap(
                        mBokehCamera->m_pDstGDepthOriJpegBuffer);
#endif
                }
#else
                mBokehCamera->m_pMainSnapBuffer =
                    capture_msg.combo_buff.buffer1;
#endif
            }

            mBokehCamera->unmap(capture_msg.combo_buff.buffer1);
            if (!mBokehCamera->mFlushing)
                mDevmain->hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE,
                                             &mime_type, NULL);
            if (!mBokehCamera->mIsHdrMode) {
                reprocessReq(output_buffer, capture_msg);
            } else {
                if (!mAbokehGallery) {
                    mBokehCamera->pushBufferList(
                        mBokehCamera->mLocalBuffer, output_buffer,
                        mBokehCamera->mLocalBufferNumber,
                        mBokehCamera->mLocalBufferList);
                }

                mBokehCamera->pushBufferList(mBokehCamera->mLocalBuffer,
                                             capture_msg.combo_buff.buffer1,
                                             mBokehCamera->mLocalBufferNumber,
                                             mBokehCamera->mLocalBufferList);
                mBokehCamera->pushBufferList(mBokehCamera->mLocalBuffer,
                                             capture_msg.combo_buff.buffer2,
                                             mBokehCamera->mLocalBufferNumber,
                                             mBokehCamera->mLocalBufferList);
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
void BokehCamera::BokehCaptureThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    capture_queue_msg_t capture_msg;
    bzero(&capture_msg, sizeof(capture_queue_msg_t));
    capture_msg.msg_type = MSG_EXIT;
    mCaptureMsgList.push_front(capture_msg);
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
void BokehCamera::BokehCaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCaptureMsgList.empty() || mBokehCamera->mHdrSkipBlur) {
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
int BokehCamera::BokehCaptureThread::sprdDepthCaptureHandle(
    buffer_handle_t *input_buf1, void *input_buf1_addr,
    buffer_handle_t *input_buf2) {
    void *input_buf2_addr = NULL;
    void *snapshot_gdepth_buffer_addr = NULL;
    int64_t depthRun = 0;
    cmr_uint depth_jepg = 0;
    int rc = NO_ERROR;
    HAL_LOGD("E");
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    if (input_buf1 == NULL || input_buf2 == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }

#ifdef CONFIG_SUPPORT_GDEPTH
    buffer_handle_t *snapshot_gdepth_buffer = (mBokehCamera->popBufferList(
        mBokehCamera->mLocalBufferList, SNAPSHOT_GDEPTH_BUFFER));
    mBokehCamera->mBokehDepthBuffer.snap_gdepthJpg_buffer =
        (mBokehCamera->popBufferList(mBokehCamera->mLocalBufferList,
                                     SNAP_GDEPTHJPEG_BUFFER));
#endif

    rc = mBokehCamera->map(input_buf2, &input_buf2_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer2");
        goto fail_map_input2;
    }

#ifdef CONFIG_SUPPORT_GDEPTH
    rc =
        mBokehCamera->map(snapshot_gdepth_buffer, &snapshot_gdepth_buffer_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map snapshot_gdepth_buffer");
        goto fail_map_gdepth;
    }

    rc = mBokehCamera->map(
        mBokehCamera->mBokehDepthBuffer.snap_gdepthJpg_buffer,
        &(mBokehCamera->mBokehDepthBuffer.snap_gdepthjpeg_buffer_addr));
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map  "
                 "mBokehCamera->mBokehDepthBuffer.snap_gdepthJpg_buffer");
        goto fail_map_gdepthJpg;
    }
#endif

    property_get("persist.vendor.cam.bokeh.scale", prop, "yuv_sw");
    if (!strcmp(prop, "sw")) {
        mBokehCamera->swScale(
            (uint8_t *)mBokehCamera->mScaleInfo.addr_vir.addr_y,
            mBokehCamera->mBokehSize.depth_snap_main_w,
            mBokehCamera->mBokehSize.depth_snap_main_h,
            mBokehCamera->mScaleInfo.fd, (uint8_t *)input_buf1_addr,
            mBokehCamera->mBokehSize.capture_w,
            mBokehCamera->mBokehSize.capture_h, ADP_BUFFD(*input_buf1));

    } else if (!strcmp(prop, "yuv_sw")) {
        mBokehCamera->Yuv420Scale(
            (uint8_t *)mBokehCamera->mScaleInfo.addr_vir.addr_y,
            mBokehCamera->mBokehSize.depth_snap_main_w,
            mBokehCamera->mBokehSize.depth_snap_main_h,
            (uint8_t *)input_buf1_addr, mBokehCamera->mBokehSize.capture_w,
            mBokehCamera->mBokehSize.capture_h);
    } else if (!strcmp(prop, "hw-k")) {
        HAL_LOGD("scale from kernel");
    } else {
        mBokehCamera->hwScale(
            (uint8_t *)mBokehCamera->mScaleInfo.addr_vir.addr_y,
            mBokehCamera->mBokehSize.depth_snap_main_w,
            mBokehCamera->mBokehSize.depth_snap_main_h,
            mBokehCamera->mScaleInfo.fd, (uint8_t *)input_buf1_addr,
            mBokehCamera->mBokehSize.capture_w,
            mBokehCamera->mBokehSize.capture_h, ADP_BUFFD(*input_buf1));
    }
    HAL_LOGD("scale src:%d,%d,dst:%d,%d", mBokehCamera->mBokehSize.capture_w,
             mBokehCamera->mBokehSize.capture_h,
             mBokehCamera->mBokehSize.depth_snap_main_w,
             mBokehCamera->mBokehSize.depth_snap_main_h);

    depthRun = systemTime();
    if (mBokehCamera->mLastOnlieVcm &&
        mBokehCamera->mLastOnlieVcm == mBokehCamera->mVcmSteps) {
        rc = mBokehCamera->mBokehAlgo->capDepthRun(
            mBokehCamera->mBokehDepthBuffer.snap_depth_buffer,
            mBokehCamera->mBokehDepthBuffer.depth_out_map_table,
            input_buf2_addr, (void *)mBokehCamera->mScaleInfo.addr_vir.addr_y,
            mBokehCamera->mVcmStepsFixed, mBokehCamera->mlimited_infi,
            mBokehCamera->mlimited_macro);
    } else {
        rc = mBokehCamera->mBokehAlgo->capDepthRun(
            mBokehCamera->mBokehDepthBuffer.snap_depth_buffer, NULL,
            input_buf2_addr, (void *)mBokehCamera->mScaleInfo.addr_vir.addr_y,
            mBokehCamera->mVcmStepsFixed, mBokehCamera->mlimited_infi,
            mBokehCamera->mlimited_macro);
        HAL_LOGD("close online depth");
    }
    if (rc != ALRNB_ERR_SUCCESS) {
        HAL_LOGE("sprd_depth_Run failed! %d", rc);
        goto exit;
    }
    HAL_LOGD("depth run cost %lld ms", ns2ms(systemTime() - depthRun));

#if defined(CONFIG_SUPPORT_GDEPTH) ||                                          \
    defined(CONFIG_BOKEH_JPEG_APPEND_NORMALIZED_DEPTH_YUV)
    memset(&mGDepthOutputParam, 0, sizeof(gdepth_outparam));
    mGDepthOutputParam.confidence_map =
        mBokehCamera->mBokehDepthBuffer.snap_depthConfidence_buffer;
    mGDepthOutputParam.depthnorm_data =
        mBokehCamera->mBokehDepthBuffer.depth_normalize_data_buffer;
    mBokehCamera->mBokehAlgo->getGDepthInfo(
        mBokehCamera->mBokehDepthBuffer.snap_depth_buffer, &mGDepthOutputParam);
    mBokehCamera->far = mGDepthOutputParam.far;
    mBokehCamera->near = mGDepthOutputParam.near;
#endif
#ifdef CONFIG_SUPPORT_GDEPTH
    memcpy(snapshot_gdepth_buffer_addr, mGDepthOutputParam.depthnorm_data,
           mBokehCamera->mBokehSize.depth_snap_size / 2);
    mBokehCamera->mBokehSize
        .depth_jepg_size = mBokehCamera->jpeg_encode_exif_simplify_format(
        snapshot_gdepth_buffer, snapshot_gdepth_buffer_addr,
        mBokehCamera->mBokehDepthBuffer.snap_gdepthJpg_buffer,
        mBokehCamera->mBokehDepthBuffer.snap_gdepthjpeg_buffer_addr, NULL, NULL,
        mBokehCamera->m_pPhyCamera[CAM_BOKEH_MAIN].hwi, IMG_DATA_TYPE_YUV400,
        SprdCamera3Setting::s_setting[m_pPhyCamera[0].id].jpgInfo.orientation,
        0);
#endif
    mBokehCamera->mIsCapDepthFinish = true;

exit : { // dump yuv data

    property_get("persist.vendor.cam.bokeh.dump", prop, "0");
    if (!strcmp(prop, "capdepth") || !strcmp(prop, "all")) {
        // input_buf1 or left image
        mBokehCamera->dumpData((unsigned char *)input_buf1_addr, 1,
                               ADP_BUFSIZE(*input_buf1),
                               mBokehCamera->mBokehSize.capture_w,
                               mBokehCamera->mBokehSize.capture_h,
                               mBokehCamera->mCapFrameNumber, "Main");
        // input_buf2 or right image
        mBokehCamera->dumpData((unsigned char *)input_buf2_addr, 1,
                               ADP_BUFSIZE(*input_buf2),
                               mBokehCamera->mBokehSize.depth_snap_sub_w,
                               mBokehCamera->mBokehSize.depth_snap_sub_h,
                               mBokehCamera->mCapFrameNumber, "Sub");
        mBokehCamera->dumpData(
            (uint8_t *)mBokehCamera->mScaleInfo.addr_vir.addr_y, 1,
            mBokehCamera->mScaleInfo.buf_size,
            mBokehCamera->mBokehSize.depth_snap_main_w,
            mBokehCamera->mBokehSize.depth_snap_main_h,
            mBokehCamera->mCapFrameNumber, "MainScale");
        mBokehCamera->dumpData((unsigned char *)(mBokehCamera->mBokehDepthBuffer
                                                     .snap_depth_buffer),
                               5, mBokehCamera->mBokehSize.depth_snap_size,
                               mBokehCamera->mBokehSize.depth_snap_out_w,
                               mBokehCamera->mBokehSize.depth_snap_out_h,
                               mBokehCamera->mCapFrameNumber, "depth");
#ifdef CONFIG_SUPPORT_GDEPTH
        mBokehCamera->dumpData(
            (unsigned char *)snapshot_gdepth_buffer_addr, 5,
            mBokehCamera->mBokehSize.depth_yuv_normalize_size,
            mBokehCamera->mBokehSize.depth_snap_out_w,
            mBokehCamera->mBokehSize.depth_snap_out_h,
            mBokehCamera->mCapFrameNumber, "depthLinear");
#endif
    }
}
    HAL_LOGI(":X");

#ifdef CONFIG_SUPPORT_GDEPTH
fail_map_gdepthJpg:
    mBokehCamera->unmap(snapshot_gdepth_buffer);
#endif

fail_map_gdepth:
    mBokehCamera->unmap(input_buf2);
fail_map_input2:

#ifdef CONFIG_SUPPORT_GDEPTH
    mBokehCamera->pushBufferList(
        mBokehCamera->mLocalBuffer, snapshot_gdepth_buffer,
        mBokehCamera->mLocalBufferNumber, mBokehCamera->mLocalBufferList);
    mBokehCamera->pushBufferList(
        mBokehCamera->mLocalBuffer,
        mBokehCamera->mBokehDepthBuffer.snap_gdepthJpg_buffer,
        mBokehCamera->mLocalBufferNumber, mBokehCamera->mLocalBufferList);
#endif

    return rc;
}

/*===========================================================================
 * FUNCTION   :sprdBokehCaptureHandle
 *
 * DESCRIPTION: sprdBokehCaptureHandle
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int BokehCamera::BokehCaptureThread::sprdBokehCaptureHandle(
    buffer_handle_t *output_buf, buffer_handle_t *input_buf1,
    void *input_buf1_addr) {
    void *output_buf_addr = NULL;
    int rc = NO_ERROR;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    HAL_LOGI("E");
    if (output_buf == NULL || input_buf1 == NULL) {
        HAL_LOGE("buffer is NULL!");
        return BAD_VALUE;
    }
    rc = mBokehCamera->map(output_buf, &output_buf_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map output buffer");
        goto fail_map_output;
    }
    HAL_LOGD("%p,%p,%p, mDoPortrait %d", input_buf1_addr,
             mBokehCamera->mBokehDepthBuffer.snap_depth_buffer, output_buf_addr,
             mBokehCamera->mDoPortrait);
    HAL_LOGD("capBlurImage start");
    mBokehCamera->mBokehAlgo->capBlurImage(
        input_buf1_addr, mBokehCamera->mBokehDepthBuffer.snap_depth_buffer,
        output_buf_addr, DEPTH_SNAP_OUTPUT_WIDTH, DEPTH_SNAP_OUTPUT_HEIGHT,
        mBokehCamera->mDoPortrait);
    HAL_LOGD("capBlurImage end");
    {
        char prop[PROPERTY_VALUE_MAX] = {
            0,
        };
        property_get("persist.vendor.cam.bokeh.dump", prop, "0");
        if (!strcmp(prop, "sprdcapyuv") || !strcmp(prop, "all")) {
            mBokehCamera->dumpData((unsigned char *)output_buf_addr, 1,
                                   ADP_BUFSIZE(*output_buf),
                                   mBokehCamera->mBokehSize.capture_w,
                                   mBokehCamera->mBokehSize.capture_h,
                                   mBokehCamera->mCapFrameNumber, "bokeh");
        }
    }

    HAL_LOGI(":X");

    mBokehCamera->unmap(output_buf);
fail_map_output:

    return rc;
}
