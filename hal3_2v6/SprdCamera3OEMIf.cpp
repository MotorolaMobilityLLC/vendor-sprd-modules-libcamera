/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "Cam3OEMIf"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include "SprdCamera3OEMIf.h"
#include <utils/Log.h>
#include <utils/String16.h>
#include <utils/Trace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <sprd_ion.h>
#include <media/hardware/MetadataBufferType.h>
#include <math.h>
#include "cmr_common.h"
#ifdef SPRD_PERFORMANCE
#include <androidfw/SprdIlog.h>
#endif
#include "gralloc_public.h"
#include "SprdCamera3HALHeader.h"
#include "SprdCamera3Channel.h"
#include "SprdCamera3Flash.h"
#include <dlfcn.h>

#include <linux/ion.h>
#include <ui/GraphicBuffer.h>
#include <cutils/ashmem.h>

#include "libyuv/convert.h"
#include "libyuv/convert_from.h"
#include "libyuv/scale.h"

extern "C" {
#include "isp_video.h"
}
#ifdef CONFIG_FACE_BEAUTY
#include "camera_face_beauty.h"
#endif
using namespace android;

namespace sprdcamera {

/**********************Macro Define**********************/
#define SWITCH_MONITOR_QUEUE_SIZE 50
#define GET_START_TIME                                                         \
    do {                                                                       \
        s_start_timestamp = systemTime();                                      \
    } while (0)
#define GET_END_TIME                                                           \
    do {                                                                       \
        s_end_timestamp = systemTime();                                        \
    } while (0)
#define GET_USE_TIME                                                           \
    do {                                                                       \
        s_use_time = (s_end_timestamp - s_start_timestamp) / 1000000;          \
    } while (0)
#define ZSL_FRAME_TIMEOUT 1000000000     /*1000ms*/
#define SET_PARAM_TIMEOUT 2000000000     /*2000ms*/
#define CAP_TIMEOUT 5000000000           /*5000ms*/
#define PREV_TIMEOUT 5000000000          /*5000ms*/
#define CAP_START_TIMEOUT 5000000000     /* 5000ms*/
#define PREV_STOP_TIMEOUT 3000000000     /* 3000ms*/
#define CANCEL_AF_TIMEOUT 500000000      /*1000ms*/
#define PIPELINE_START_TIMEOUT 500000000 /*5s*/

#define SET_PARAMS_TIMEOUT 250 /*250 means 250*10ms*/
#define ON_OFF_ACT_TIMEOUT 50  /*50 means 50*10ms*/
#define IS_ZOOM_SYNC 0
#define NO_FREQ_REQ 0
#define NO_FREQ_STR "0"
#if defined(CONFIG_CAMERA_SMALL_PREVSIZE)
#define BASE_FREQ_REQ 192
#define BASE_FREQ_STR "0" /*base mode can treated with AUTO*/
#define MEDIUM_FREQ_REQ 200
#define MEDIUM_FREQ_STR "200000"
#define HIGH_FREQ_REQ 300
#define HIGH_FREQ_STR "300000"
#else
#define BASE_FREQ_REQ 200
#define BASE_FREQ_STR "0" /*base mode can treated with AUTO*/
#define MEDIUM_FREQ_REQ 300
#define MEDIUM_FREQ_STR "300000"
#define HIGH_FREQ_REQ 500
#define HIGH_FREQ_STR "500000"
#endif
#define DUALCAM_TIME_DIFF (15e6) /**add for 3d capture*/
#define DUALCAM_ZSL_NUM (7)      /**add for 3d capture*/
#define DUALCAM_MAX_ZSL_NUM (4)

// legacy sprd isp ae compensation manual mode, just for manual mode, dont
// change this easily
#define LEGACY_SPRD_AE_COMPENSATION_RANGE_MIN -3
#define LEGACY_SPRD_AE_COMPENSATION_RANGE_MAX 3
#define LEGACY_SPRD_AE_COMPENSATION_STEP_NUMERATOR 1
#define LEGACY_SPRD_AE_COMPENSATION_STEP_DEMINATOR 1

#define SHINWHITED_NOT_DETECTFD_MAXNUM 10

// 300 means 300ms
#define ZSL_SNAPSHOT_THRESHOLD_TIME 300

// 3dnr Video mode
enum VIDEO_3DNR {
    VIDEO_OFF = 0,
    VIDEO_ON,
};

#define CONFIG_PRE_ALLOC_CAPTURE_MEM 1
#define HAS_CAMERA_POWER_HINTS 1
//#define CONFIG_NEED_UNMAP

/*ZSL Thread*/
#define ZSLMode_MONITOR_QUEUE_SIZE 50
#define CMR_EVT_ZSL_MON_BASE 0x800
#define CMR_EVT_ZSL_MON_INIT 0x801
#define CMR_EVT_ZSL_MON_EXIT 0x802
#define CMR_EVT_ZSL_MON_SNP 0x804
#define CMR_EVT_ZSL_MON_STOP_OFFLINE_PATH 0x805

#define UPDATE_RANGE_FPS_COUNT 0x04

#ifdef CONFIG_SPRD_LCD_FLASH
#define LCD_BACKLIGHT_MAX_DEV "/sys/class/backlight/sprd_backlight/brightness"
#define MAX_BRIGHTNESS 0xff
#endif

/**********************Static Members**********************/
static nsecs_t s_start_timestamp = 0;
static nsecs_t s_end_timestamp = 0;
static int s_use_time = 0;
static nsecs_t cam_init_begin_time = 0;
bool gIsApctCamInitTimeShow = false;
bool gIsApctRead = false;

sprd_camera_memory_t *SprdCamera3OEMIf::mIspFirmwareReserved = NULL;
uint32_t SprdCamera3OEMIf::mIspFirmwareReserved_cnt = 0;
bool SprdCamera3OEMIf::mZslCaptureExitLoop = false;
multi_camera_zsl_match_frame *SprdCamera3OEMIf::mMultiCameraMatchZsl = NULL;
multiCameraMode SprdCamera3OEMIf::mMultiCameraMode = MODE_SINGLE_CAMERA;

static void writeCamInitTimeToApct(char *buf) {
    int apct_dir_fd = open("/data/apct", O_CREAT, 0777);

    if (apct_dir_fd >= 0) {
        fchmod(apct_dir_fd, 0777);
        close(apct_dir_fd);
    }

    int apct_fd =
        open("/data/apct/apct_data.log", O_CREAT | O_RDWR | O_TRUNC, 0666);

    if (apct_fd >= 0) {
        char buf[100] = {0};
        sprintf(buf, "\n%s", buf);
        write(apct_fd, buf, strlen(buf));
        fchmod(apct_fd, 0666);
        close(apct_fd);
    }
}

static void writeCamInitTimeToProc(float init_time) {
    char cam_time_buf[256] = {0};
    const char *cam_time_proc = "/proc/benchMark/cam_time";

    sprintf(cam_time_buf, "Camera init time:%.2fs", init_time);

    FILE *f = fopen(cam_time_proc, "r+w");
    if (NULL != f) {
        fseek(f, 0, 0);
        fwrite(cam_time_buf, strlen(cam_time_buf), 1, f);
        fclose(f);
    }
    writeCamInitTimeToApct(cam_time_buf);
}

bool getApctCamInitSupport() {
    if (gIsApctRead) {
        return gIsApctCamInitTimeShow;
    }
    gIsApctRead = true;

    int ret = 0;
    char str[10] = {'\0'};
    const char *FILE_NAME = "/data/data/com.sprd.APCT/apct/apct_support";

    FILE *f = fopen(FILE_NAME, "r");

    if (NULL != f) {
        fseek(f, 0, 0);
        ret = fread(str, 5, 1, f);
        fclose(f);
        if (ret) {
            long apct_config = atol(str);
            gIsApctCamInitTimeShow =
                (apct_config & 0x8010) == 0x8010 ? true : false;
        }
    }
    return gIsApctCamInitTimeShow;
}

SprdCamera3OEMIf::SprdCamera3OEMIf(int cameraId, SprdCamera3Setting *setting)
    : mSetCapRatioFlag(false), mVideoCopyFromPreviewFlag(false),
      mSprdPipVivEnabled(0), mSprdHighIsoEnabled(0), mSprdRefocusEnabled(0),
      mSprd3dCalibrationEnabled(0), mSprdYuvCallBack(0),
      mSprdMultiYuvCallBack(0), mSprdReprocessing(0), mNeededTimestamp(0),
      mIsUnpopped(false), mIsBlur2Zsl(false),
      mPreviewFormat(CAMERA_DATA_FORMAT_YUV422), mPictureFormat(1),
      mPreviewStartFlag(0), mIsDvPreview(0), mIsStoppingPreview(0),
      mRecordingMode(0), mIsSetCaptureMode(false), mRecordingFirstFrameTime(0),
      mUser(0), mPreviewWindow(NULL), mHalOem(NULL), mIsStoreMetaData(false),
      mIsFreqChanged(false), mCameraId(cameraId), miSPreviewFirstFrame(1),
      mCaptureMode(CAMERA_NORMAL_MODE), mCaptureRawMode(0), mFlashMask(false),
      mReleaseFLag(false), mTimeCoeff(1), mIsPerformanceTestable(false),
      mIsAndroidZSL(false), mSetting(setting), BurstCapCnt(0), mCapIntent(0),
      mPrvTimerID(NULL), mFlashMode(-1), mIsAutoFocus(false),
      mIspToolStart(false), mSubRawHeapNum(0), mSubRawHeapSize(0),
      mPathRawHeapNum(0), mPathRawHeapSize(0), mPreviewDcamAllocBufferCnt(0),
      mPreviewFrameNum(0), mRecordFrameNum(0), mIsRecording(false),
      mPreAllocCapMemInited(0), mIsPreAllocCapMemDone(0),
      mZSLModeMonitorMsgQueHandle(0), mZSLModeMonitorInited(0), mCNRMode(0),
      mGyroInit(0), mGyroExit(0), mEisPreviewInit(false), mEisVideoInit(false),
      mGyroNum(0), mSprdEisEnabled(false), mIsUpdateRangeFps(false),
      mPrvBufferTimestamp(0), mUpdateRangeFpsCount(0), mPrvMinFps(0),
      mPrvMaxFps(0), mVideoSnapshotType(0), mIommuEnabled(false),
      mFlashCaptureFlag(0), mFlashCaptureSkipNum(FLASH_CAPTURE_SKIP_FRAME_NUM),
      mFixedFpsEnabled(0), mSprdAppmodeId(-1), mTempStates(CAMERA_NORMAL_TEMP),
      mIsTempChanged(0), mFlagOffLineZslStart(0), mZslSnapshotTime(0),
      mIsIspToolMode(0), mIsCameraClearQBuf(0), mLastCafDoneTime(0),
      mFaceDetectStartedFlag(0)

{
    ATRACE_CALL();
    HAL_LOGI(":hal3: Constructor E camId=%d", cameraId);

#ifdef CONFIG_FACE_BEAUTY
    memset(&face_beauty, 0, sizeof(face_beauty));
#endif

    memset(&grab_capability, 0, sizeof(grab_capability));

#ifdef CONFIG_SPRD_LCD_FLASH
    mResetBrightness = -1;
#endif

    SprdCameraSystemPerformance::getSysPerformance(&mSysPerformace);
    if (mSysPerformace) {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
    }

#if defined(LOWPOWER_DISPLAY_30FPS)
    property_set("lowpower.display.30fps", "true");
#endif

#if defined(CONFIG_PRE_ALLOC_CAPTURE_MEM)
    mIsPreAllocCapMem = 1;
#else
    mIsPreAllocCapMem = 0;
#endif

    if (mMultiCameraMatchZsl == NULL) {
        mMultiCameraMatchZsl = (multi_camera_zsl_match_frame *)malloc(
            sizeof(multi_camera_zsl_match_frame));
        memset(mMultiCameraMatchZsl, 0, sizeof(multi_camera_zsl_match_frame));
    }

    memset(mSubRawHeapArray, 0, sizeof(mSubRawHeapArray));
    memset(mZslHeapArray, 0, sizeof(mZslHeapArray));
    memset(mPreviewHeapArray, 0, sizeof(mPreviewHeapArray));
    memset(mVideoHeapArray, 0, sizeof(mVideoHeapArray));
    memset(&mSlowPara, 0, sizeof(slow_motion_para));
    memset(mPdafRawHeapArray, 0, sizeof(mPdafRawHeapArray));
    memset(mRefocusHeapArray, 0, sizeof(mRefocusHeapArray));
    memset(mPathRawHeapArray, 0, sizeof(mPathRawHeapArray));
    memset(mChannel2Heap, 0, sizeof(mChannel2Heap));
    memset(mChannel3Heap, 0, sizeof(mChannel3Heap));
    memset(mRawHeapArray, 0, sizeof(mRawHeapArray));

    setCameraState(SPRD_INIT, STATE_CAMERA);

    if (!mHalOem) {
        oem_module_t *omi;
        mHalOem = (oem_module_t *)malloc(sizeof(oem_module_t));
        if (NULL == mHalOem) {
            HAL_LOGE("mHalOem is NULL");
        } else {
            memset(mHalOem, 0, sizeof(*mHalOem));

            mHalOem->dso = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
            if (NULL == mHalOem->dso) {
                char const *err_str = dlerror();
                HAL_LOGE("dlopen error%s ", err_str ? err_str : "unknown");
            }

            const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
            omi = (oem_module_t *)dlsym(mHalOem->dso, sym);
            if (omi) {
                mHalOem->ops = omi->ops;
            }
            HAL_LOGI("loaded libcamoem.so mHalOem->dso = %p", mHalOem->dso);
        }
    }

    mCameraId = cameraId;
    if (mCameraId == 1) {
        mMultiCameraMatchZsl->cam1_ZSLQueue = &mZSLQueue;
    } else if (mCameraId == 3) {
        mMultiCameraMatchZsl->cam3_ZSLQueue = &mZSLQueue;
    }
    mCbInfoList.clear();

    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mVideoWidth = 0;
    mVideoHeight = 0;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mRawWidth = 0;
    mRawHeight = 0;
    mCallbackWidth = 0;
    mCallbackHeight = 0;
    mLargestPictureWidth = 0;
    mLargestPictureHeight = 0;

    jpeg_gps_location = false;
    mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
    mPreviewHeapBakUseFlag = 0;

    for (int i = 0; i < kPreviewBufferCount; i++) {
        mPreviewBufferHandle[i] = NULL;
        mPreviewCancelBufHandle[i] = NULL;
    }
    mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
    mZSLTakePicture = false;

    mZoomInfo.mode = ZOOM_INFO;
    mZoomInfo.zoom_info.prev_aspect_ratio = 0.0f;
    mZoomInfo.zoom_info.video_aspect_ratio = 0.0f;
    mZoomInfo.zoom_info.capture_aspect_ratio = 0.0f;

    mCameraHandle = NULL;

    memset(mGps_processing_method, 0, sizeof(mGps_processing_method));
    memset(mIspB4awbHeapReserved, 0, sizeof(mIspB4awbHeapReserved));
    memset(mIspRawAemHeapReserved, 0, sizeof(mIspRawAemHeapReserved));
    memset(mIspPreviewYReserved, 0, sizeof(mIspPreviewYReserved));
    memset(m3DNRPrevHeapReserverd, 0, sizeof(m3DNRPrevHeapReserverd));
    memset(m3DNRScaleHeapReserverd, 0, sizeof(m3DNRScaleHeapReserverd));
    memset(m4in1HeapArray, 0, sizeof(m4in1HeapArray));

    mJpegRotaSet = false;
    mPicCaptureCnt = 0;

    mRegularChan = NULL;
    mPictureChan = NULL;

    mPreviewHeapNum = 0;
    mVideoHeapNum = 0;
    mZslHeapNum = 0;
    mRefocusHeapNum = 0;
    mPdafRawHeapNum = 0;
    mSubRawHeapSize = 0;
    mChannel2HeapNum = 0;
    mChannel3HeapNum = 0;

#ifdef USE_ONE_RESERVED_BUF
    mCommonHeapReserved = NULL;
#else
    mPreviewHeapReserved = NULL;
    mVideoHeapReserved = NULL;
    mZslHeapReserved = NULL;
#endif

    mDepthHeapReserved = NULL;
    mIspLscHeapReserved = NULL;
    mIspStatisHeapReserved = NULL;
    mIspYUVReserved = NULL;
    mPdafRawHeapReserved = NULL;
    mIspAntiFlickerHeapReserved = NULL;
    m3DNRHeapReserverd = NULL;
    mAISceneScaleHeapReserverd = NULL;
    mPrevDepthHeapReserved = NULL;
    mPrevSwOutHeapReserved = NULL;

    mVideoShotFlag = 0;
    mVideoShotNum = 0;
    mVideoShotPushFlag = 0;

    mRestartFlag = false;
    mReDisplayHeap = NULL;

    mZslPreviewMode = false;

    mSprdZslEnabled = false;
    mZslMaxFrameNum = 1;
    mZslNum = 2;
    mZslShotPushFlag = 0;
    mZslChannelStatus = 1;
    mZSLQueue.clear();

    mRawMaxFrameNum = 0;
    mRawNum = 1;
    mRawQueue.clear();

    mZslCaptureExitLoop = false;
    mSprdCameraLowpower = 0;

    mVideo3dnrFlag = 0;
    mCapBufIsAvail = 0;
    m_zslValidDataWidth = 0;
    m_zslValidDataHeight = 0;
    mChannelCb = NULL;
    mUserData = NULL;
    mZslStreamInfo = NULL;
    mStartFrameNum = 0;
    mStopFrameNum = 0;
    mDropPreviewFrameNum = 0;
    mDropVideoFrameNum = 0;
    mDropZslFrameNum = 0;
    mPreAllocCapMemThread = 0;
    mPreAllocCapMemSemDone.count = 0;
    mGyroMsgQueHandle = 0;
    mGyromaxtimestamp = 0;
    mGyro_sem.count = 0;
    mVideoSnapshotFrameNum = 0;
    mMasterId = 0;

    mStreamOnWithZsl = 0;
    mFrameSyncFlag = 0;
    mFrameSyncNum = 0;

#ifdef CONFIG_CAMERA_EIS
    memset(mGyrodata, 0, sizeof(mGyrodata));
    memset(&mPreviewParam, 0, sizeof(mPreviewParam));
    memset(&mVideoParam, 0, sizeof(mVideoParam));
    mPreviewInst = NULL;
    mVideoInst = NULL;
#endif

    HAL_LOGI(":hal3: Constructor X");
}

SprdCamera3OEMIf::~SprdCamera3OEMIf() {
    ATRACE_CALL();

    HAL_LOGI(":hal3: Destructor E camId=%d", mCameraId);

    if (!mReleaseFLag) {
        closeCamera();
    }

#if defined(LOWPOWER_DISPLAY_30FPS)
    char value[PROPERTY_VALUE_MAX];
    property_get("lowpower.display.30fps", value, "false");
    if (!strcmp(value, "true")) {
        property_set("lowpower.display.30fps", "false");
        HAL_LOGI("camera low power mode exit");
    }
#endif

    // clean memory in case memory leak
    freeAllCameraMemIon();

    if (mHalOem) {
        if (NULL != mHalOem->dso)
            dlclose(mHalOem->dso);
        free((void *)mHalOem);
        mHalOem = NULL;
    }

    HAL_LOGI(":hal3: X");
    timer_stop();
    SprdCameraSystemPerformance::freeSysPerformance(&mSysPerformace);
}

void SprdCamera3OEMIf::closeCamera() {
    ATRACE_CALL();

    HAL_LOGI(":hal3: E camId=%d", mCameraId);

    Mutex::Autolock l(&mLock);

    mZslCaptureExitLoop = true;
    sem_post(&((SprdCamera3OEMIf *)this)->mZslQueueSemDone);
    // Either preview was ongoing, or we are in the middle or taking a
    // picture.  It's the caller's responsibility to make sure the camera
    // is in the idle or init state before destroying this object.
    HAL_LOGD("camera state = %s, preview state = %s, capture state = %s",
             getCameraStateStr(getCameraState()),
             getCameraStateStr(getPreviewState()),
             getCameraStateStr(getCaptureState()));

    if (isCapturing()) {
        cancelPictureInternal();
    }

    if (isPreviewing()) {
        stopPreviewInternal();
    }

    SprdCamera3Flash::releaseFlash(mCameraId);

#ifdef CONFIG_CAMERA_GYRO
    gyro_monitor_thread_deinit((void *)this);
#endif

    if (isCameraInit()) {
        setCameraState(SPRD_INTERNAL_STOPPING, STATE_CAMERA);

        HAL_LOGI(":hal3: deinit camera");
        if (CMR_CAMERA_SUCCESS != mHalOem->ops->camera_deinit(mCameraHandle)) {
            setCameraState(SPRD_ERROR, STATE_CAMERA);
            mReleaseFLag = true;
            HAL_LOGE("camera_deinit failed");
            return;
        }

        WaitForCameraStop();
    }

    pre_alloc_cap_mem_thread_deinit((void *)this);
    ZSLMode_monitor_thread_deinit((void *)this);

    mReleaseFLag = true;
    HAL_LOGI(":hal3: X");
}

int SprdCamera3OEMIf::getCameraId() const { return mCameraId; }

void SprdCamera3OEMIf::initialize() {
    memset(&mSlowPara, 0, sizeof(slow_motion_para));
    mIsRecording = false;
    mVideoShotFlag = 0;

    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mVideoWidth = 0;
    mVideoHeight = 0;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mRawWidth = 0;
    mRawHeight = 0;
    mCallbackWidth = 0;
    mCallbackHeight = 0;
    mStreamOnWithZsl = 0;
    mSprdZslEnabled = 0;
    mFrameSyncFlag = 0;
    mFrameSyncNum = 0;
    mVideoSnapshotType = 0;
}

int SprdCamera3OEMIf::start(camera_channel_type_t channel_type,
                            uint32_t frame_number) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    Mutex::Autolock l(&mLock);

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    HAL_LOGI("channel_type = %d, frame_number = %d", channel_type,
             frame_number);
    mStartFrameNum = frame_number;

    switch (channel_type) {
    case CAMERA_CHANNEL_TYPE_REGULAR: {
        if (mParaDCDVMode == CAMERA_PREVIEW_FORMAT_DV)
            mRecordingFirstFrameTime = 0;

#ifdef CONFIG_CAMERA_EIS
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        if (sprddefInfo.sprd_eis_enabled == 1) {
            mSprdEisEnabled = true;
        } else {
            mSprdEisEnabled = false;
        }
        if (!mEisPreviewInit && sprddefInfo.sprd_eis_enabled == 1 &&
            mPreviewWidth != 0 && mPreviewHeight != 0) {
            EisPreview_init();
            mEisPreviewInit = true;
        }
        if (!mEisVideoInit && sprddefInfo.sprd_eis_enabled == 1 &&
            mVideoWidth != 0 && mVideoHeight != 0) {
            EisVideo_init();
            mEisVideoInit = true;
        }
#endif

        ret = startPreviewInternal();
        break;
    }
    case CAMERA_CHANNEL_TYPE_PICTURE: {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);

        if (mSprdReprocessing) {
            ret = reprocessInputBuffer();
            mNeededTimestamp = 0;
        } else if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
                   mTakePictureMode == SNAPSHOT_ONLY_MODE) {
            ret = takePicture();
        } else if (mTakePictureMode == SNAPSHOT_ZSL_MODE) {
            mVideoSnapshotFrameNum = frame_number;
            ret = zslTakePicture();
        } else if (mTakePictureMode == SNAPSHOT_VIDEO_MODE) {
            mVideoSnapshotFrameNum = frame_number;
            ret = VideoTakePicture();
        }
        break;
    }
    default:
        break;
    }

    HAL_LOGI("X");
    return ret;
}

int SprdCamera3OEMIf::stop(camera_channel_type_t channel_type,
                           uint32_t frame_number) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    int capture_intent = 0;

    HAL_LOGI("channel_type = %d, frame_number = %d", channel_type,
             frame_number);
    mStopFrameNum = frame_number;

    switch (channel_type) {
    case CAMERA_CHANNEL_TYPE_REGULAR:
        SprdCamera3Flash::releaseFlash(mCameraId);
        stopPreviewInternal();
#ifdef CONFIG_CAMERA_EIS
        if (mEisPreviewInit) {
            video_stab_close(mPreviewInst);
            mEisPreviewInit = false;
            HAL_LOGI("preview stab close");
        }
        if (mEisVideoInit) {
            video_stab_close(mVideoInst);
            mEisVideoInit = false;
            HAL_LOGI("video stab close");
        }
#endif
        break;
    case CAMERA_CHANNEL_TYPE_PICTURE:
        cancelPictureInternal();
        break;
    default:
        break;
    }

    HAL_LOGI("X");
    return ret;
}

int SprdCamera3OEMIf::takePicture() {
    ATRACE_CALL();

    int ret = 0;
    HAL_LOGI("E");

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("take picture in error status, deinit capture at first");
        deinitCapture(mIsPreAllocCapMem);
    } else if (SPRD_IDLE != mCameraState.capture_state) {
        usleep(50 * 1000);
        if (SPRD_IDLE != mCameraState.capture_state) {
            HAL_LOGE("take picture: action alread exist, direct return");
            goto exit;
        }
    }

    setCameraState(SPRD_FLASH_IN_PROGRESS, STATE_CAPTURE);

    if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
        mTakePictureMode == SNAPSHOT_PREVIEW_MODE ||
        mTakePictureMode == SNAPSHOT_ONLY_MODE) {
        if (((mCaptureMode == CAMERA_ISP_TUNING_MODE) ||
             (mCaptureMode == CAMERA_ISP_SIMULATION_MODE)) &&
            !mIspToolStart) {
            mIspToolStart = true;
            timer_set(this, ISP_TUNING_WAIT_MAX_TIME, timer_hand);
        }

        if (isPreviewStart()) {
            HAL_LOGV("Preview not start! wait preview start");
            WaitForPreviewStart();
            usleep(20 * 1000);
        }

        if (isPreviewing()) {
            HAL_LOGD("call stopPreviewInternal in takePicture().");
            // whether FRONT_CAMERA_FLASH_TYPE is lcd
            bool isFrontLcd =
                (strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") == 0) ? true : false;
            // whether FRONT_CAMERA_FLASH_TYPE is flash
            bool isFrontFlash =
                (strcmp(FRONT_CAMERA_FLASH_TYPE, "flash") == 0) ? true : false;
            if (mCameraId == 0 || isFrontLcd || isFrontFlash) {
                mHalOem->ops->camera_start_preflash(mCameraHandle);
            }
            stopPreviewInternal();
        }
        HAL_LOGI("stopPreviewInternal done, preview state = %d",
                 getPreviewState());

        if (isPreviewing()) {
            HAL_LOGE("takePicture: stop preview error!, preview state = %d",
                     getPreviewState());
            goto exit;
        }
    }

    if (isCapturing()) {
        WaitForCaptureDone();
    }

    setSnapshotParams();

    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        HAL_LOGE("fail to camera_take_picture");
        goto exit;
    }

exit:
    HAL_LOGI("X");
    /*must return NO_ERROR, otherwise can't flush camera normal*/
    return NO_ERROR;
}

// Send async zsl snapshot msg
int SprdCamera3OEMIf::zslTakePicture() {
    ATRACE_CALL();

    uint32_t ret = 0;

    if (mSprdZslEnabled == true) {
        CMR_MSG_INIT(message);
        mZslShotPushFlag = 1;
        message.msg_type = CMR_EVT_ZSL_MON_SNP;
        message.sync_flag = CMR_MSG_SYNC_NONE;
        message.data = NULL;
        ret = cmr_thread_msg_send((cmr_handle)mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("Fail to send one msg!");
            goto exit;
        }
    }

exit:
    return NO_ERROR;
}

int SprdCamera3OEMIf::reprocessInputBuffer() {
    ATRACE_CALL();

    uint32_t ret = 0;
    cmr_uint yaddr, yaddr_vir, fd;
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);

    ret = channel->getInputBuff(&yaddr_vir, &yaddr, &fd);
    if (ret != NO_ERROR)
        return ret;

    ret = reprocessYuvForJpeg(yaddr, yaddr_vir, fd);

    if (ret != NO_ERROR) {
        HAL_LOGE("Release input reprocess buffer!");
        channel->releaseInputBuff();
    }

    HAL_LOGI("X");
    return ret;
}

int SprdCamera3OEMIf::reprocessYuvForJpeg(cmr_uint yaddr, cmr_uint yaddr_vir,
                                          cmr_uint fd) {
    ATRACE_CALL();

    uint32_t ret = 0;
    struct img_size capturesize;
    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    HAL_LOGI("E");
    GET_START_TIME;
    print_time();

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    mSprdYuvCallBack = false;
    if (getMultiCameraMode() == MODE_3D_CAPTURE) {
        mSprd3dCalibrationEnabled = sprddefInfo.sprd_3dcalibration_enabled;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                 sprddefInfo.sprd_3dcalibration_enabled);
    }
    if (getMultiCameraMode() == MODE_BLUR ||
        getMultiCameraMode() == MODE_BOKEH) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE,
                 mSprdYuvCallBack);
        HAL_LOGD("reprocess mode, force enable reprocess");
    }
    setCameraState(SPRD_FLASH_IN_PROGRESS, STATE_CAPTURE);

    mHalOem->ops->camera_fast_ctrl(mCameraHandle, CAMERA_FAST_MODE_FD, 0);

    if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        if (mVideoWidth != 0 && mVideoHeight != 0 && mRecordingMode == true &&
            ((mCaptureMode != CAMERA_ISP_TUNING_MODE) &&
             (mCaptureMode != CAMERA_ISP_SIMULATION_MODE))) {
            capturesize.width = (cmr_u32)mPreviewWidth;   // mVideoWidth;
            capturesize.height = (cmr_u32)mPreviewHeight; // mVideoHeight;
        } else {
            capturesize.width = (cmr_u32)mCaptureWidth;
            capturesize.height = (cmr_u32)mCaptureHeight;
        }
    } else {
        capturesize.width = (cmr_u32)mRawWidth;
        capturesize.height = (cmr_u32)mRawHeight;
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&capturesize);

    if (isCapturing()) {
        WaitForCaptureDone();
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);

    JPEG_Tag jpgInfo;
    struct img_size jpeg_thumb_size;
    struct img_size capture_size;
    mSetting->getJPEGTag(&jpgInfo);
    HAL_LOGV("JPEG quality = %d", jpgInfo.quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
             jpgInfo.quality);
    HAL_LOGV("JPEG thumbnail quality = %d", jpgInfo.thumbnail_quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY,
             jpgInfo.thumbnail_quality);
    jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
    jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
    if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        capture_size.width = (cmr_u32)mCaptureWidth;
        capture_size.height = (cmr_u32)mCaptureHeight;
    } else {
        capture_size.width = (cmr_u32)mRawWidth;
        capture_size.height = (cmr_u32)mRawHeight;
    }
    if (jpeg_thumb_size.width > capture_size.width &&
        jpeg_thumb_size.height > capture_size.height) {
        jpeg_thumb_size.width = 0;
        jpeg_thumb_size.height = 0;
    }
    HAL_LOGI("JPEG thumbnail size = %d x %d", jpeg_thumb_size.width,
             jpeg_thumb_size.height);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);

    HAL_LOGD("mSprdZslEnabled=%d", mSprdZslEnabled);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)mSprdZslEnabled);

    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);

    ret = mHalOem->ops->camera_reprocess_yuv_for_jpeg(
        mCameraHandle, mCaptureMode, yaddr, yaddr_vir, fd);

    if (ret) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        HAL_LOGE("fail to camera_take_picture");
        goto exit;
    }

    print_time();

exit:
    HAL_LOGI("X");
    return ret;
}

int SprdCamera3OEMIf::checkIfNeedToStopOffLineZsl() {
    uint32_t ret = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    // capture_mode: 1 single capture; >1: n capture
    if (mFlagOffLineZslStart && mSprdZslEnabled == 1 &&
        sprddefInfo.capture_mode == 1 && mZslShotPushFlag == 0) {
        HAL_LOGI("mFlagOffLineZslStart=%d, sprddefInfo.capture_mode=%d",
                 mFlagOffLineZslStart, sprddefInfo.capture_mode);
        CMR_MSG_INIT(message);
        message.msg_type = CMR_EVT_ZSL_MON_STOP_OFFLINE_PATH;
        message.sync_flag = CMR_MSG_SYNC_NONE;
        message.data = NULL;
        ret = cmr_thread_msg_send((cmr_handle)mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("Fail to send one msg!");
        }
    }

exit:
    return NO_ERROR;
}

int SprdCamera3OEMIf::VideoTakePicture() {
    ATRACE_CALL();

    JPEG_Tag jpgInfo;

    HAL_LOGI("E");
    GET_START_TIME;
    print_time();

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("in error status, deinit capture at first ");
        deinitCapture(mIsPreAllocCapMem);
    }

    if (isCapturing()) {
        WaitForCaptureDone();
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);

    LENS_Tag lensInfo;
    mSetting->getLENSTag(&lensInfo);
    if (lensInfo.focal_length) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                 (int32_t)(lensInfo.focal_length * 1000));
        HAL_LOGD("lensInfo.focal_length = %f", lensInfo.focal_length);
    }

    struct img_size jpeg_thumb_size;
    mSetting->getJPEGTag(&jpgInfo);
    HAL_LOGD("JPEG quality = %d", jpgInfo.quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
             jpgInfo.quality);
    HAL_LOGD("JPEG thumbnail quality = %d", jpgInfo.thumbnail_quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY,
             jpgInfo.thumbnail_quality);
    jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
    jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
    HAL_LOGD("JPEG thumbnail size = %d x %d", jpeg_thumb_size.width,
             jpeg_thumb_size.height);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);

    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        HAL_LOGE("fail to camera_take_picture.");
        goto exit;
    }

    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
    mVideoShotPushFlag = 1;
    mVideoShotWait.signal();
    print_time();

exit:
    HAL_LOGI("X");
    return NO_ERROR;
}

int SprdCamera3OEMIf::cancelPicture() {
    Mutex::Autolock l(&mLock);

    return cancelPictureInternal();
}

status_t SprdCamera3OEMIf::faceDectect(bool enable) {
    status_t ret = NO_ERROR;
    SPRD_DEF_Tag sprddefInfo;
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }
    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion > 1)
        return ret;

    if (enable) {
        if (mFaceDetectStartedFlag == 0) {
            mHalOem->ops->camera_fd_start(mCameraHandle, 1);
            mFaceDetectStartedFlag = 1;
        }
    } else {
        if (mFaceDetectStartedFlag == 1) {
            mHalOem->ops->camera_fd_start(mCameraHandle, 0);
            mFaceDetectStartedFlag = 0;
        }
    }
    return ret;
}

status_t SprdCamera3OEMIf::faceDectect_enable(bool enable) {
    status_t ret = NO_ERROR;
    SPRD_DEF_Tag sprddefInfo;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (mMultiCameraMode == MODE_BOKEH && mCameraId == 2) {
        return ret;
    }

    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion > 1)
        return ret;

    if (enable) {
        mHalOem->ops->camera_fd_enable(mCameraHandle, 1);
    } else {
        mHalOem->ops->camera_fd_enable(mCameraHandle, 0);
    }
    return ret;
}

int SprdCamera3OEMIf::autoFocusToFaceFocus() {
    FACE_Tag faceInfo;
    mSetting->getFACETag(&faceInfo);
    if (faceInfo.face_num > 0) {
        int i = 0;
        int k = 0;
        int x1 = 0;
        int x2 = 0;
        int max = 0;
        uint16_t max_width = 0;
        uint16_t max_height = 0;
        struct cmr_focus_param focus_para;
        CONTROL_Tag controlInfo;
        mSetting->getCONTROLTag(&controlInfo);

        mSetting->getLargestPictureSize(mCameraId, &max_width, &max_height);

        for (i = 0; i < faceInfo.face_num; i++) {
            x1 = faceInfo.face[i].rect[0];
            x2 = faceInfo.face[i].rect[2];
            if (x2 - x1 > max) {
                k = i;
                max = x2 - x1;
            }
        }

        HAL_LOGD("max_width:%d, max_height:%d, focus src x:%d  y:%d", max_width,
                 max_height, controlInfo.af_regions[0],
                 controlInfo.af_regions[1]);
        controlInfo.af_regions[0] =
            ((faceInfo.face[k].rect[0] + faceInfo.face[k].rect[2]) / 2 +
             (faceInfo.face[k].rect[2] - faceInfo.face[k].rect[0]) / 10);
        controlInfo.af_regions[1] =
            (faceInfo.face[k].rect[1] + faceInfo.face[k].rect[3]) / 2;
        controlInfo.af_regions[2] = 624;
        controlInfo.af_regions[3] = 624;
        focus_para.zone[0].start_x = controlInfo.af_regions[0];
        focus_para.zone[0].start_y = controlInfo.af_regions[1];
        focus_para.zone[0].width = controlInfo.af_regions[2];
        focus_para.zone[0].height = controlInfo.af_regions[3];
        focus_para.zone_cnt = 1;
        HAL_LOGD("focus face x:%d  y:%d", controlInfo.af_regions[0],
                 controlInfo.af_regions[1]);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                 CAMERA_FOCUS_MODE_AUTO);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCUS_RECT,
                 (cmr_uint)&focus_para);
    }

    return NO_ERROR;
}

status_t SprdCamera3OEMIf::autoFocus() {
    ATRACE_CALL();

    if (!SprdCamera3Setting::mSensorFocusEnable[mCameraId]) {
        return NO_ERROR;
    }

    HAL_LOGD("E");
    Mutex::Autolock l(&mLock);
    CONTROL_Tag controlInfo;

    if (mCameraId == 3) {
        return NO_ERROR;
    }

    mSetting->getCONTROLTag(&controlInfo);
    if (isPreviewStart()) {
        HAL_LOGV("Preview not start! wait preview start");
        WaitForPreviewStart();
    }

    if (!isPreviewing()) {
        HAL_LOGE("not previewing");
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
        mSetting->setAfCONTROLTag(&controlInfo);
        return INVALID_OPERATION;
    }

    if (SPRD_IDLE != getFocusState()) {
        HAL_LOGE("existing, direct return!");
        return NO_ERROR;
    }
    setCameraState(SPRD_FOCUS_IN_PROGRESS, STATE_FOCUS);
    mIsAutoFocus = true;
    /*caf transit to auto focus*/
    if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
        controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO) {
        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE)
            mHalOem->ops->camera_transfer_caf_to_af(mCameraHandle);
        else if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO)
            mHalOem->ops->camera_transfer_af_to_caf(mCameraHandle);

        if (getMultiCameraMode() == MODE_BLUR && isNeedAfFullscan() &&
            controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START &&
            controlInfo.af_regions[0] == 0 && controlInfo.af_regions[1] == 0 &&
            controlInfo.af_regions[2] == 0 && controlInfo.af_regions[3] == 0) {
            HAL_LOGD("set full scan mode");
            if (mCameraId == 1) {
                autoFocusToFaceFocus();
            }
            mHalOem->ops->camera_transfer_af_to_caf(mCameraHandle);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                     CAMERA_FOCUS_MODE_FULLSCAN);
        } else {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                     CAMERA_FOCUS_MODE_AUTO);
        }
    }
    if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN;
    } else if (controlInfo.af_mode ==
               ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
    } else {
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN;
    }
    mSetting->setAfCONTROLTag(&controlInfo);
    if (0 != mHalOem->ops->camera_start_autofocus(mCameraHandle)) {
        HAL_LOGE("auto foucs fail.");
        setCameraState(SPRD_IDLE, STATE_FOCUS);
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
        mSetting->setAfCONTROLTag(&controlInfo);
    }

    HAL_LOGD("X");
    return NO_ERROR;
}

status_t SprdCamera3OEMIf::cancelAutoFocus() {
    ATRACE_CALL();

    if (!SprdCamera3Setting::mSensorFocusEnable[mCameraId]) {
        return NO_ERROR;
    }

    bool ret = 0;
    HAL_LOGD("E");

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    Mutex::Autolock l(&mLock);

    if (mCameraId == 3)
        return NO_ERROR;

    ret = mHalOem->ops->camera_cancel_autofocus(mCameraHandle);

    WaitForFocusCancelDone();
    {
        int64_t timeStamp = 0;
        timeStamp = systemTime();

        CONTROL_Tag controlInfo;
        mSetting->getCONTROLTag(&controlInfo);
        if (!(controlInfo.af_state ==
                  ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED &&
              (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START ||
               controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_IDLE))) {
            if (controlInfo.af_mode ==
                    ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
                controlInfo.af_mode ==
                    ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
                controlInfo.af_state =
                    ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED;
            } else {
                controlInfo.af_state = ANDROID_CONTROL_AF_STATE_INACTIVE;
            }
        }
        /*auto focus resume to caf*/
        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE &&
            mCameraId == 0 && mIsAutoFocus) {
            if (controlInfo.af_mode ==
                ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE)
                mHalOem->ops->camera_transfer_af_to_caf(mCameraHandle);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                     CAMERA_FOCUS_MODE_CAF);
        }
        mSetting->setAfCONTROLTag(&controlInfo);
    }
    mIsAutoFocus = false;
    HAL_LOGD("X");
    return ret;
}

int SprdCamera3OEMIf::getMultiCameraMode() {
    int mode = (int)mMultiCameraMode;

    return mode;
}

void SprdCamera3OEMIf::setCallBackYuvMode(bool mode) {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    HAL_LOGD("setCallBackYuvMode: %d, %d", mode, mSprdYuvCallBack);
    mSprdYuvCallBack = mode;
    if (mSprdYuvCallBack && (!mSprdReprocessing)) {
        if (getMultiCameraMode() == MODE_3D_CAPTURE) {
            mSprd3dCalibrationEnabled = true;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                     mSprd3dCalibrationEnabled);
            HAL_LOGD("yuv call back mode, force enable 3d cal");
        }

        if (getMultiCameraMode() == MODE_BLUR ||
            getMultiCameraMode() == MODE_BOKEH) {
            SET_PARM(mHalOem, mCameraHandle,
                     CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE, mSprdYuvCallBack);
            HAL_LOGD("yuv call back mode");
        }
    }
}

void SprdCamera3OEMIf::setMultiCallBackYuvMode(bool mode) {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    mSprdMultiYuvCallBack = mode;
    HAL_LOGD("setMultiCallBackYuvMode: %d, %d", mode, mSprdMultiYuvCallBack);
}

void SprdCamera3OEMIf::GetFocusPoint(cmr_s32 *point_x, cmr_s32 *point_y) {
    HAL_LOGD("E");

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    /*
        if (0 !=
            mHalOem->ops->camera_get_focus_point(mCameraHandle, point_x,
       point_y)) {
            HAL_LOGE("Fail to get focus point.");
        }
    */

    HAL_LOGD("X");
}

cmr_s32 SprdCamera3OEMIf::ispSwCheckBuf(cmr_uint *param_ptr) {
    HAL_LOGD("E");
    /*
        return mHalOem->ops->camera_isp_sw_check_buf(mCameraHandle, param_ptr);
    */
    HAL_LOGD("X");
    return 0;
}

void SprdCamera3OEMIf::stopPreview() {
    HAL_LOGD("switch stop preview");
    stopPreviewInternal();
}

void SprdCamera3OEMIf::startPreview() {
    HAL_LOGD("switch start preview");
    startPreviewInternal();
}

void SprdCamera3OEMIf::setCaptureReprocessMode(bool mode, uint32_t width,
                                               uint32_t height) {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }
    HAL_LOGD("setCaptureReprocessMode: %d, %d, reprocess size: %d, %d", mode,
             mSprdReprocessing, width, height);
    mSprdReprocessing = mode;
    mHalOem->ops->camera_set_reprocess_picture_size(
        mCameraHandle, mSprdReprocessing, mCameraId, width, height);
}

int SprdCamera3OEMIf::setSensorStream(uint32_t on_off) {
    int ret;

    HAL_LOGD("E");
    ret = mHalOem->ops->camera_ioctrl(
        mCameraHandle, CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL, &on_off);

    return ret;
}

int SprdCamera3OEMIf::setCameraClearQBuff() {
    int ret = 0;

    HAL_LOGD("E");
    mIsCameraClearQBuf = 1;
    HAL_LOGD("mIsCameraClearQBuf %d", mIsCameraClearQBuf);
    return ret;
}

void SprdCamera3OEMIf::getRawFrame(int64_t timestamp, cmr_u8 **y_addr) {
    HAL_LOGD("E");

    List<ZslBufferQueue>::iterator itor;

    if (mZSLQueue.empty()) {
        HAL_LOGD("zsl queue is null");
        return;
    } else {
        itor = mZSLQueue.begin();
        while (itor != mZSLQueue.end()) {
            HAL_LOGD("y_addr %lu", itor->frame.y_vir_addr);
            int diff = (int64_t)timestamp - (int64_t)itor->frame.timestamp;
            HAL_LOGD("preview timestamp %" PRId64 ", zsl timestamp %" PRId64,
                     timestamp, itor->frame.timestamp);
            if (abs(diff) < DUALCAM_TIME_DIFF) {
                HAL_LOGD("get zsl frame");
                *y_addr = (cmr_u8 *)(itor->frame.y_vir_addr);
                HAL_LOGD("y_addr %p", *y_addr);
                return;
            }
            itor++;
        }
    }
    *y_addr = NULL;

    return;
}

void SprdCamera3OEMIf::getDualOtpData(void **addr, int *size, int *read) {
    struct sensor_otp_cust_info otp_info;

    memset(&otp_info, 0, sizeof(struct sensor_otp_cust_info));
    mHalOem->ops->camera_get_sensor_otp_info(mCameraHandle, &otp_info);

    if (otp_info.total_otp.data_ptr != NULL && otp_info.dual_otp.dual_flag) {
        *addr = otp_info.dual_otp.data_3d.data_ptr;
        *size = otp_info.dual_otp.data_3d.size;
        *read = otp_info.dual_otp.dual_flag;
    }

    HAL_LOGD("OTP INFO:addr 0x%p, size = %d", *addr, *size);
    return;
}

void SprdCamera3OEMIf::getOnlineBuffer(void *cali_info) {

    /*
        mHalOem->ops->camera_get_online_buffer(mCameraHandle, cali_info);
    */
    HAL_LOGD("online buffer addr %p", cali_info);
    return;
}

void SprdCamera3OEMIf::getIspDebugInfo(void **addr, int *size) {

    void *ispInioAddr = NULL;
    int ispInfoSize = 0;

    mHalOem->ops->camera_get_isp_info(mCameraHandle, &ispInioAddr,
                                      &ispInfoSize);

    *addr = ispInioAddr;
    *size = ispInfoSize;
    HAL_LOGD("ISP INFO:addr 0x%p, size = %d", *addr, *size);

    return;
}

int SprdCamera3OEMIf::camera_ioctrl(int cmd, void *param1, void *param2) {
    int ret = 0;

    switch (cmd) {
    case CAMERA_IOCTRL_SET_MULTI_CAMERAMODE: {
        mMultiCameraMode = *(multiCameraMode *)param1;
        break;
    }
    case CAMERA_IOCTRL_GET_FULLSCAN_INFO: {
        int version = *(int *)param2;
        struct isp_af_fullscan_info *af_fullscan_info =
            (struct isp_af_fullscan_info *)param1;
        if (version == 3 && af_fullscan_info->distance_reminder != 1) {
            mIsBlur2Zsl = true;
        } else {
            mIsBlur2Zsl = false;
        }
        break;
    }
    case CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP: {
        mNeededTimestamp = *(uint64_t *)param1;
        break;
    }
    case CAMERA_IOCTRL_SET_MIME_TYPE: {
        int type = *(int *)param1;
        setMimeType(type);
        break;
    }
    case CAMERA_IOCTRL_GET_IOMMU_AVAILABLE: {
        *(int *)param1 = IommuIsEnabled();
        break;
    }
    case CAMERA_IOCTRL_SET_MASTER_ID: {
        mMasterId = *(int8_t *)param1;
        break;
    }
    }
    ret = mHalOem->ops->camera_ioctrl(mCameraHandle, cmd, param1);

    return ret;
}

bool SprdCamera3OEMIf::isNeedAfFullscan() {
    bool ret = false;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (getMultiCameraMode() != MODE_BLUR) {
        return ret;
    }
    if (mCameraId == 1) {
        property_get("persist.vendor.cam.fr.blur.version", prop, "0");
    } else {
        property_get("persist.vendor.cam.ba.blur.version", prop, "0");
    }
    if (2 <= atoi(prop)) {
        ret = true;
    }
    return ret;
}

void SprdCamera3OEMIf::setMimeType(int type) {
    int mime_type = type;

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXIF_MIME_TYPE, mime_type);

    HAL_LOGI("X,mime_type=0x%x", mime_type);
}

status_t SprdCamera3OEMIf::setAePrecaptureSta(uint8_t state) {
    status_t ret = 0;
    Mutex::Autolock l(&mLock);
    CONTROL_Tag controlInfo;

    mSetting->getCONTROLTag(&controlInfo);
    controlInfo.ae_state = state;
    mSetting->setAeCONTROLTag(&controlInfo);
    HAL_LOGD("ae sta %d", state);
    return ret;
}

void SprdCamera3OEMIf::setCaptureRawMode(bool mode) {
    struct img_size req_size;
    struct cmr_zoom_param zoom_param;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    mCaptureRawMode = mode;
    HAL_LOGD("ISP_TOOL: setCaptureRawMode: %d, %d", mode, mCaptureRawMode);
    if (1 == mode) {
        req_size.width = (cmr_u32)mCaptureWidth;
        req_size.height = (cmr_u32)mCaptureHeight;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
                 (cmr_uint)&req_size);

        zoom_param.mode = ZOOM_LEVEL;
        zoom_param.zoom_level = 0;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM,
                 (cmr_uint)&zoom_param);

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    }
}

void SprdCamera3OEMIf::setIspFlashMode(uint32_t mode) {
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_FLASH, mode);
}

void SprdCamera3OEMIf::antiShakeParamSetup() {
#ifdef CONFIG_CAMERA_ANTI_SHAKE
    mPreviewWidth =
        mPreviewWidth_backup + ((((mPreviewWidth_backup / 10) + 15) >> 4) << 4);
    mPreviewHeight = mPreviewHeight_backup +
                     ((((mPreviewHeight_backup / 10) + 15) >> 4) << 4);
#endif
}

const char *
SprdCamera3OEMIf::getCameraStateStr(SprdCamera3OEMIf::Sprd_camera_state s) {
    static const char *states[] = {
#define STATE_STR(x) #x
        STATE_STR(SPRD_INIT),
        STATE_STR(SPRD_IDLE),
        STATE_STR(SPRD_ERROR),
        STATE_STR(SPRD_PREVIEW_IN_PROGRESS),
        STATE_STR(SPRD_FOCUS_IN_PROGRESS),
        STATE_STR(SPRD_SET_PARAMS_IN_PROGRESS),
        STATE_STR(SPRD_WAITING_RAW),
        STATE_STR(SPRD_WAITING_JPEG),
        STATE_STR(SPRD_FLASH_IN_PROGRESS),
        STATE_STR(SPRD_INTERNAL_PREVIEW_STOPPING),
        STATE_STR(SPRD_INTERNAL_CAPTURE_STOPPING),
        STATE_STR(SPRD_INTERNAL_PREVIEW_REQUESTED),
        STATE_STR(SPRD_INTERNAL_RAW_REQUESTED),
        STATE_STR(SPRD_INTERNAL_STOPPING),
#undef STATE_STR
    };
    return states[s];
}

void SprdCamera3OEMIf::print_time() {
#if PRINT_TIME
    struct timeval time;
    gettimeofday(&time, NULL);
    HAL_LOGD("time: %" PRId64 " us", time.tv_sec * 1000000LL + time.tv_usec);
#endif
}

int SprdCamera3OEMIf::getCameraTemp() {
    const char *const temp = "/sys/devices/virtual/thermal/thermal_zone0/temp";
    int fd = open(temp, O_RDONLY);
    if (fd < 0) {
        ALOGE("error opening %s.", temp);
        return 0;
    }
    char buf[5] = {0};
    int n = read(fd, buf, 5);
    if (n == -1) {
        ALOGE("error reading %s", temp);
        close(fd);
        return 0;
    }
    close(fd);
    return atoi(buf) / 1000;
}

void SprdCamera3OEMIf::adjustFpsByTemp() {
    struct cmr_range_fps_param fps_param;
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);

    int temp = 0, tempStates = -1;
    temp = getCameraTemp();
    if (temp < 65) {
        if (mTempStates != CAMERA_NORMAL_TEMP) {
            if (temp < 60) {
                tempStates = CAMERA_NORMAL_TEMP;
            }
        } else {
            tempStates = CAMERA_NORMAL_TEMP;
        }
    } else if (temp >= 65 && temp < 75) {
        if (mTempStates == CAMERA_DANGER_TEMP) {
            if (temp < 70) {
                tempStates = CAMERA_HIGH_TEMP;
            }
        } else {
            tempStates = CAMERA_HIGH_TEMP;
        }
    } else {
        tempStates = CAMERA_DANGER_TEMP;
    }

    if (tempStates != mTempStates) {
        mTempStates = tempStates;
        mIsTempChanged = 1;
    }

    if (mIsTempChanged) {
        switch (tempStates) {
        case CAMERA_NORMAL_TEMP:
            setPreviewFps(mRecordingMode);
            break;
        case CAMERA_HIGH_TEMP:
            fps_param.min_fps = fps_param.max_fps =
                (controlInfo.ae_target_fps_range[1] + 10) / 2;
            HAL_LOGD("high temp, set camera min_fps: %lu, max_fps: %lu",
                     fps_param.min_fps, fps_param.max_fps);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
                     (cmr_uint)&fps_param);
            break;
        case CAMERA_DANGER_TEMP:
            fps_param.min_fps = fps_param.max_fps = 10;
            HAL_LOGD("danger temp, set camera min_fps: %lu, max_fps: %lu",
                     fps_param.min_fps, fps_param.max_fps);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
                     (cmr_uint)&fps_param);
            break;
        default:
            HAL_LOGE("error states");
            break;
        }
        mIsTempChanged = 0;
    }
}

void SprdCamera3OEMIf::setSprdCameraLowpower(int flag) {
    mSprdCameraLowpower = flag;
}

int SprdCamera3OEMIf::setPreviewParams() {
    struct img_size previewSize = {0, 0}, videoSize = {0, 0}, rawSize = {0, 0};
    struct img_size captureSize = {0, 0}, callbackSize = {0, 0};

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    // preview
    if (mPreviewWidth != 0 && mPreviewHeight != 0) {
        previewSize.width = (cmr_u32)mPreviewWidth;
        previewSize.height = (cmr_u32)mPreviewHeight;
    } else {
        // TBD: may be this is no use, will remove it
        previewSize.width = 720;
        previewSize.height = 576;
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&previewSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_FORMAT,
             (cmr_uint)mPreviewFormat);

    // video
    if (mVideoWidth > 0 && mVideoHeight > 0) {
        // TBD: will remove this
        if (sprddefInfo.slowmotion <= 1)
            mCaptureMode = CAMERA_ZSL_MODE;
    }

    if (mVideoCopyFromPreviewFlag) {
        HAL_LOGD("video stream copy from preview stream");
        videoSize.width = 0;
        videoSize.height = 0;
    } else {
        videoSize.width = (cmr_u32)mVideoWidth;
        videoSize.height = (cmr_u32)mVideoHeight;
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_VIDEO_SIZE,
             (cmr_uint)&videoSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_VIDEO_FORMAT,
             (cmr_uint)mVideoFormat);

    // calback
    // for now blur/bokeh use zsl buffer for yuvcallback, maybe change it to
    // standard yuvcallback later
    if (getMultiCameraMode() != MODE_BLUR &&
        getMultiCameraMode() != MODE_BOKEH &&
        getMultiCameraMode() != MODE_3D_CALIBRATION) {
        callbackSize.width = mCallbackWidth;
        callbackSize.height = mCallbackHeight;
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_YUV_CALLBACK_SIZE,
             (cmr_uint)&callbackSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_YUV_CALLBACK_FORMAT,
             (cmr_uint)mCallbackFormat);

    rawSize.width = mRawWidth;
    rawSize.height = mRawHeight;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RAW_SIZE, (cmr_uint)&rawSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RAW_FORMAT,
             (cmr_uint)mRawFormat);
    // used for single camera need raw stream
    // allocateRawBuffers();

    if (mSprdZslEnabled || mVideoSnapshotType) {
        captureSize.width = mCaptureWidth;
        captureSize.height = mCaptureHeight;
    }
    if (getMultiCameraMode() == MODE_3D_CALIBRATION) {
        captureSize.width = mCallbackWidth;
        captureSize.height = mCallbackHeight;
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&captureSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_FORMAT,
             (cmr_uint)mPictureFormat);

    setPreviewFps(mRecordingMode);

    HAL_LOGD("preview: w=%d, h=%d, format=%d, video: w=%d, h=%d, format=%d "
             "calback: w=%d, h=%d, format=%d capture: w=%d, h=%d format=%d, "
             "raw: w=%d, h=%d, format=%d",
             previewSize.width, previewSize.height, mPreviewFormat,
             videoSize.width, videoSize.height, mVideoFormat,
             callbackSize.width, callbackSize.height, mCallbackFormat,
             captureSize.width, captureSize.height, mPictureFormat,
             rawSize.width, rawSize.height, mRawFormat);

    return true;
}

int SprdCamera3OEMIf::setSnapshotParams() {
    struct img_size previewSize = {0, 0}, videoSize = {0, 0};
    struct img_size callbackSize = {0, 0}, captureSize = {0, 0};

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mHalOem->ops->camera_fast_ctrl(mCameraHandle, CAMERA_FAST_MODE_FD, 0);

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&previewSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_VIDEO_SIZE,
             (cmr_uint)&videoSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_YUV_CALLBACK_SIZE,
             (cmr_uint)&callbackSize);

    captureSize.width = mCaptureWidth;
    captureSize.height = mCaptureHeight;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&captureSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_FORMAT,
             (cmr_uint)mPictureFormat);

    HAL_LOGD("mPicCaptureCnt=%d", mPicCaptureCnt);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);

    LENS_Tag lensInfo;
    mSetting->getLENSTag(&lensInfo);
    if (lensInfo.focal_length) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                 (int32_t)(lensInfo.focal_length * 1000));
        HAL_LOGD("lensInfo.focal_length = %f", lensInfo.focal_length);
    }

    JPEG_Tag jpgInfo;
    struct img_size jpeg_thumb_size;
    mSetting->getJPEGTag(&jpgInfo);
    jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
    jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
             jpgInfo.quality);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY,
             jpgInfo.thumbnail_quality);
    HAL_LOGD("JPEG thumb_size=%dx%d, quality=%d, thumb_quality=%d",
             jpeg_thumb_size.width, jpeg_thumb_size.height, jpgInfo.quality,
             jpgInfo.thumbnail_quality);

    HAL_LOGD("mSprdZslEnabled=%d, capture_w=%d, capture_h=%d", mSprdZslEnabled,
             mCaptureWidth, mCaptureHeight);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)mSprdZslEnabled);

    return 0;
}

void SprdCamera3OEMIf::setPreviewFps(bool isRecordMode) {
    struct cmr_range_fps_param fps_param;
    char value[PROPERTY_VALUE_MAX];
    CONTROL_Tag controlInfo;
    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);
    mSetting->getCONTROLTag(&controlInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    fps_param.is_recording = isRecordMode;
    if (isRecordMode) {
        fps_param.min_fps = controlInfo.ae_target_fps_range[0];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];
        fps_param.video_mode = 1;

// reduce video recording power for spreadtrum app, if sprd app change this, we
// will remove these code
#ifdef CONFIG_CAMRECORDER_DYNAMIC_FPS
        if (mSprdAppmodeId == CAMERA_MODE_AUTO_VIDEO) {
            fps_param.min_fps = 15;
        }
#endif

        // to set recording fps by setprop
        char prop[PROPERTY_VALUE_MAX];
        int val_max = 0;
        int val_min = 0;
        property_get("persist.vendor.cam.record.fps", prop, "0");
        if (atoi(prop) != 0) {
            val_min = atoi(prop) % 100;
            val_max = atoi(prop) / 100;
            fps_param.min_fps = val_min > 5 ? val_min : 5;
            fps_param.max_fps = val_max;
        }
    } else {
        fps_param.min_fps = controlInfo.ae_target_fps_range[0];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];
        fps_param.video_mode = 0;

        // TBD: check why 20fps, not 30fps
        if (MODE_BOKEH == mMultiCameraMode) {
            if (fps_param.min_fps < 20) {
                fps_param.min_fps = 20;
                fps_param.max_fps = 20;
            }
        }

        // to set preview fps by setprop
        char prop[PROPERTY_VALUE_MAX];
        int val_max = 0;
        int val_min = 0;
        property_get("persist.vendor.cam.preview.fps", prop, "0");
        if (atoi(prop) != 0) {
            val_min = atoi(prop) % 100;
            val_max = atoi(prop) / 100;
            fps_param.min_fps = val_min > 5 ? val_min : 5;
            fps_param.max_fps = val_max;
        }
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    HAL_LOGD("min_fps=%ld, max_fps=%ld, video_mode=%ld", fps_param.min_fps,
             fps_param.max_fps, fps_param.video_mode);

// TBD: check these code
#if 1 // for cts
    mIsUpdateRangeFps = true;
    mUpdateRangeFpsCount++;
    if (mUpdateRangeFpsCount == UPDATE_RANGE_FPS_COUNT) {
        if ((mPrvMinFps == fps_param.min_fps) &&
            (mPrvMaxFps == fps_param.max_fps))
            mUpdateRangeFpsCount = 0;
    }
    mPrvMinFps = fps_param.min_fps;
    mPrvMaxFps = fps_param.max_fps;
#endif
}

void SprdCamera3OEMIf::setCameraState(Sprd_camera_state state,
                                      state_owner owner) {
    Sprd_camera_state org_state = SPRD_IDLE;
    volatile Sprd_camera_state *state_owner = NULL;
    Mutex::Autolock stateLock(&mStateLock);
    HAL_LOGD("E:state: %s, owner: %d camera id %d", getCameraStateStr(state),
             owner, mCameraId);

    if (owner == STATE_CAPTURE) {
        if (mCameraState.capture_state == SPRD_INTERNAL_CAPTURE_STOPPING &&
            state != SPRD_INIT && state != SPRD_ERROR && state != SPRD_IDLE)
            return;
    }

    switch (owner) {
    case STATE_CAMERA:
        org_state = mCameraState.camera_state;
        state_owner = &(mCameraState.camera_state);
        break;

    case STATE_PREVIEW:
        org_state = mCameraState.preview_state;
        state_owner = &(mCameraState.preview_state);
        break;

    case STATE_CAPTURE:
        org_state = mCameraState.capture_state;
        state_owner = &(mCameraState.capture_state);
        break;

    case STATE_FOCUS:
        org_state = mCameraState.focus_state;
        state_owner = &(mCameraState.focus_state);
        break;

    case STATE_SET_PARAMS:
        org_state = mCameraState.setParam_state;
        state_owner = &(mCameraState.setParam_state);
        break;
    default:
        HAL_LOGE("owner error!");
        break;
    }

    switch (state) {
    /*camera state*/
    case SPRD_INIT:
        mCameraState.camera_state = SPRD_INIT;
        mCameraState.preview_state = SPRD_IDLE;
        mCameraState.capture_state = SPRD_IDLE;
        mCameraState.focus_state = SPRD_IDLE;
        mCameraState.setParam_state = SPRD_IDLE;
        break;

    case SPRD_IDLE:
        *state_owner = SPRD_IDLE;
        break;

    case SPRD_INTERNAL_STOPPING:
        mCameraState.camera_state = state;
        break;

    case SPRD_ERROR:
        *state_owner = SPRD_ERROR;
        break;

    /*preview state*/
    case SPRD_PREVIEW_IN_PROGRESS:
    case SPRD_INTERNAL_PREVIEW_STOPPING:
    case SPRD_INTERNAL_PREVIEW_REQUESTED:
        mCameraState.preview_state = state;
        break;

    /*capture state*/
    case SPRD_FLASH_IN_PROGRESS:
    case SPRD_INTERNAL_RAW_REQUESTED:
    case SPRD_WAITING_RAW:
    case SPRD_WAITING_JPEG:
    case SPRD_INTERNAL_CAPTURE_STOPPING:
        mCameraState.capture_state = state;
        break;

    /*focus state*/
    case SPRD_FOCUS_IN_PROGRESS:
        mCameraState.focus_state = state;
        break;

    /*set_param state*/
    case SPRD_SET_PARAMS_IN_PROGRESS:
        mCameraState.setParam_state = state;
        break;

    default:
        HAL_LOGE("unknown owner");
        break;
    }

    if (org_state != state)
        mStateWait.broadcast();

    HAL_LOGD("X: camera state = %s, preview state = %s, capture state = %s "
             "focus state = %s set param state = %s",
             getCameraStateStr(mCameraState.camera_state),
             getCameraStateStr(mCameraState.preview_state),
             getCameraStateStr(mCameraState.capture_state),
             getCameraStateStr(mCameraState.focus_state),
             getCameraStateStr(mCameraState.setParam_state));
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getCameraState() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.camera_state));
    return mCameraState.camera_state;
}

camera_status_t SprdCamera3OEMIf::GetCameraStatus(camera_status_type_t state) {
    camera_status_t ret = CAMERA_PREVIEW_IN_PROC;
    switch (state) {
    case CAMERA_STATUS_PREVIEW:
        switch (mCameraState.preview_state) {
        case SPRD_IDLE:
            ret = CAMERA_PREVIEW_IDLE;
            break;
        default:
            break;
        }
        break;
    case CAMERA_STATUS_SNAPSHOT:
        ret = CAMERA_SNAPSHOT_IDLE;
        switch (mCameraState.capture_state) {
        case SPRD_INTERNAL_RAW_REQUESTED:
        case SPRD_WAITING_JPEG:
        case SPRD_WAITING_RAW:
            ret = CAMERA_SNAPSHOT_IN_PROC;
            break;
        default:
            break;
        }
        break;
    }
    return ret;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getPreviewState() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.preview_state));
    return mCameraState.preview_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getCaptureState() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.capture_state));
    return mCameraState.capture_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getFocusState() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.focus_state));
    return mCameraState.focus_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getSetParamsState() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.setParam_state));
    return mCameraState.setParam_state;
}

bool SprdCamera3OEMIf::isCameraInit() {
    HAL_LOGD("%s", getCameraStateStr(mCameraState.camera_state));
    return (SPRD_IDLE == mCameraState.camera_state);
}

bool SprdCamera3OEMIf::isCameraIdle() {
    return (SPRD_IDLE == mCameraState.preview_state &&
            SPRD_IDLE == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::isPreviewing() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.preview_state));
    return (SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state);
}

bool SprdCamera3OEMIf::isPreviewStart() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.preview_state));
    return (SPRD_INTERNAL_PREVIEW_REQUESTED == mCameraState.preview_state);
}

bool SprdCamera3OEMIf::isCapturing() {
    bool ret = false;
    HAL_LOGV("%s", getCameraStateStr(mCameraState.capture_state));
    if (SPRD_FLASH_IN_PROGRESS == mCameraState.capture_state) {
        return false;
    }

    if ((SPRD_INTERNAL_RAW_REQUESTED == mCameraState.capture_state) ||
        (SPRD_WAITING_RAW == mCameraState.capture_state) ||
        (SPRD_WAITING_JPEG == mCameraState.capture_state)) {
        ret = true;
    } else if ((SPRD_INTERNAL_CAPTURE_STOPPING == mCameraState.capture_state) ||
               (SPRD_ERROR == mCameraState.capture_state)) {
        setCameraState(SPRD_IDLE, STATE_CAPTURE);
        HAL_LOGD("%s", getCameraStateStr(mCameraState.capture_state));
    } else if (SPRD_IDLE != mCameraState.capture_state) {
        HAL_LOGE("error: unknown capture status");
        ret = true;
    }
    return ret;
}

bool SprdCamera3OEMIf::checkPreviewStateForCapture() {
    bool ret = true;
    Sprd_camera_state tmpState = SPRD_IDLE;

    tmpState = getCaptureState();
    if ((SPRD_INTERNAL_CAPTURE_STOPPING == tmpState) ||
        (SPRD_ERROR == tmpState)) {
        HAL_LOGE("incorrect capture status %s", getCameraStateStr(tmpState));
        ret = false;
    } else {
        tmpState = getPreviewState();
        if (iSZslMode() || mSprdZslEnabled || mVideoSnapshotType == 1) {
            if (SPRD_PREVIEW_IN_PROGRESS != tmpState) {
                HAL_LOGE("incorrect preview status %d of ZSL capture mode",
                         (uint32_t)tmpState);
                ret = false;
            }
        } else {
            if (SPRD_IDLE != tmpState) {
                HAL_LOGE("incorrect preview status %d of normal capture mode",
                         (uint32_t)tmpState);
                ret = false;
            }
        }
    }
    return ret;
}

bool SprdCamera3OEMIf::WaitForCameraStop() {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock stateLock(&mStateLock);

    if (SPRD_INTERNAL_STOPPING == mCameraState.camera_state) {
        while (SPRD_INIT != mCameraState.camera_state &&
               SPRD_ERROR != mCameraState.camera_state) {
            HAL_LOGD("waiting for SPRD_IDLE");
            mStateWait.wait(mStateLock);
            HAL_LOGD("woke up");
        }
    }

    HAL_LOGD("X");

    return SPRD_INIT == mCameraState.camera_state;
}

int SprdCamera3OEMIf::waitForPipelineStart() {
    ATRACE_CALL();
    int ret = 0;

    HAL_LOGD("E");
    Mutex::Autolock l(mPipelineStartLock);

    while (mCameraState.preview_state != SPRD_PREVIEW_IN_PROGRESS &&
           mCameraState.preview_state != SPRD_INTERNAL_PREVIEW_REQUESTED &&
           mCameraState.preview_state != SPRD_ERROR) {
        HAL_LOGD("waiting for pipeline start");
        if (mPipelineStartSignal.waitRelative(mPipelineStartLock,
                                              PIPELINE_START_TIMEOUT)) {
            HAL_LOGE("timeout");
            ret = -1;
            break;
        }
    }

    HAL_LOGD("X");
    return ret;
}

bool SprdCamera3OEMIf::WaitForPreviewStart() {
    ATRACE_CALL();

    HAL_LOGD("E");
    Mutex::Autolock stateLock(&mStateLock);

    while (SPRD_PREVIEW_IN_PROGRESS != mCameraState.preview_state &&
           SPRD_ERROR != mCameraState.preview_state) {
        HAL_LOGD("waiting for SPRD_PREVIEW_IN_PROGRESS");
        if (mStateWait.waitRelative(mStateLock, PREV_TIMEOUT)) {
            HAL_LOGE("timeout");
            break;
        }
        HAL_LOGD("woke up");
    }

    HAL_LOGD("X");

    return SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state;
}

bool SprdCamera3OEMIf::WaitForCaptureDone() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);
    while (SPRD_IDLE != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.capture_state) {
        HAL_LOGD("waiting for SPRD_IDLE");
        if (mStateWait.waitRelative(mStateLock, CAP_TIMEOUT)) {
            HAL_LOGE("timeout");
            break;
        }
        HAL_LOGD("woke up");
    }

    return SPRD_IDLE == mCameraState.capture_state;
}

bool SprdCamera3OEMIf::WaitForCaptureJpegState() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);

    while (SPRD_WAITING_JPEG != mCameraState.capture_state &&
           SPRD_IDLE != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.capture_state &&
           SPRD_INTERNAL_CAPTURE_STOPPING != mCameraState.capture_state) {
        HAL_LOGD("waiting for SPRD_WAITING_JPEG");
        mStateWait.wait(mStateLock);
        HAL_LOGD("woke up, state is %s",
                 getCameraStateStr(mCameraState.capture_state));
    }
    return (SPRD_WAITING_JPEG == mCameraState.capture_state);
}

bool SprdCamera3OEMIf::WaitForFocusCancelDone() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);
    while (SPRD_IDLE != mCameraState.focus_state &&
           SPRD_ERROR != mCameraState.focus_state) {
        HAL_LOGD("waiting for SPRD_IDLE from %s",
                 getCameraStateStr(getFocusState()));
        if (mStateWait.waitRelative(mStateLock, CANCEL_AF_TIMEOUT)) {
            HAL_LOGV("timeout");
        }
        HAL_LOGD("woke up");
    }

    return SPRD_IDLE == mCameraState.focus_state;
}

sprd_camera_memory_t *SprdCamera3OEMIf::allocReservedMem(int buf_size,
                                                         int num_bufs,
                                                         uint32_t is_cache) {
    ATRACE_CALL();

    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    HAL_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        HAL_LOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (is_cache) {
        pHeapIon =
            new MemIon("/dev/ion", mem_size, 0,
                       (1 << 31) | ION_HEAP_ID_MASK_CAM | ION_FLAG_NO_CLEAR);
    } else {
        pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                              ION_HEAP_ID_MASK_CAM | ION_FLAG_NO_CLEAR);
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        HAL_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        HAL_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    HAL_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

sprd_camera_memory_t *SprdCamera3OEMIf::allocCameraMem(int buf_size,
                                                       int num_bufs,
                                                       uint32_t is_cache) {
    ATRACE_CALL();

    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    HAL_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);
    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        HAL_LOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (!mIommuEnabled) {
        if (is_cache) {
            pHeapIon =
                new MemIon("/dev/ion", mem_size, 0,
                           (1 << 31) | ION_HEAP_ID_MASK_MM | ION_FLAG_NO_CLEAR);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM | ION_FLAG_NO_CLEAR);
        }
    } else {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM |
                                      ION_FLAG_NO_CLEAR);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM | ION_FLAG_NO_CLEAR);
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

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    HAL_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

void SprdCamera3OEMIf::freeCameraMem(sprd_camera_memory_t *memory) {
    if (memory) {
        if (memory->ion_heap) {
            HAL_LOGD(
                "fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
                memory->fd, memory->phys_addr, memory->data, memory->phys_size,
                memory->ion_heap);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            HAL_LOGD("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                     "virt_addr=%p, size=0x%lx",
                     memory->fd, memory->phys_addr, memory->data,
                     memory->phys_size);
        }
        free(memory);
        memory = NULL;
    }
}

void SprdCamera3OEMIf::freeAllCameraMemIon() {
    int i;
    uint32_t j;

    HAL_LOGI(":hal3: E");

    // performance optimization:move Callback_CaptureFree to closeCamera
    Callback_CaptureFree(0, 0, 0, 0);

    if (NULL != mReDisplayHeap) {
        freeCameraMem(mReDisplayHeap);
        mReDisplayHeap = NULL;
    }

    for (j = 0; j < mZslHeapNum; j++) {
        if (NULL != mZslHeapArray[j]) {
            freeCameraMem(mZslHeapArray[j]);
            mZslHeapArray[j] = NULL;
        }
    }
    mZslHeapNum = 0;

    for (j = 0; j < mPathRawHeapNum; j++) {
        if (NULL != mPathRawHeapArray[j]) {
            freeCameraMem(mPathRawHeapArray[j]);
        }
        mPathRawHeapArray[j] = NULL;
    }
    mPathRawHeapNum = 0;
    mPathRawHeapSize = 0;

    if (NULL != mIspLscHeapReserved) {
        freeCameraMem(mIspLscHeapReserved);
        mIspLscHeapReserved = NULL;
    }

    if (NULL != mIspStatisHeapReserved) {
        mIspStatisHeapReserved->ion_heap->free_kaddr();
        freeCameraMem(mIspStatisHeapReserved);
        mIspStatisHeapReserved = NULL;
    }

    if (NULL != mIspAntiFlickerHeapReserved) {
        freeCameraMem(mIspAntiFlickerHeapReserved);
        mIspAntiFlickerHeapReserved = NULL;
    }

    for (i = 0; i < kISPB4awbCount; i++) {
        if (NULL != mIspB4awbHeapReserved[i]) {
            freeCameraMem(mIspB4awbHeapReserved[i]);
            mIspB4awbHeapReserved[i] = NULL;
        }
    }

    for (i = 0; i < 2; i++) {
        if (NULL != mIspPreviewYReserved[i]) {
            freeCameraMem(mIspPreviewYReserved[i]);
            mIspPreviewYReserved[i] = NULL;
        }
    }

    if (NULL != mIspYUVReserved) {
        freeCameraMem(mIspYUVReserved);
        mIspYUVReserved = NULL;
    }

    for (i = 0; i < kISPB4awbCount; i++) {
        if (NULL != mIspRawAemHeapReserved[i]) {
            mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
            freeCameraMem(mIspRawAemHeapReserved[i]);
            mIspRawAemHeapReserved[i] = NULL;
        }
    }

#ifdef USE_ONE_RESERVED_BUF
    if (NULL != mCommonHeapReserved) {
        freeCameraMem(mCommonHeapReserved);
        mCommonHeapReserved = NULL;
    }
#else
    if (NULL != mPreviewHeapReserved) {
        freeCameraMem(mPreviewHeapReserved);
        mPreviewHeapReserved = NULL;
    }

    if (NULL != mVideoHeapReserved) {
        freeCameraMem(mVideoHeapReserved);
        mVideoHeapReserved = NULL;
    }

    if (NULL != mZslHeapReserved) {
        freeCameraMem(mZslHeapReserved);
        mZslHeapReserved = NULL;
    }
#endif

    if (NULL != mDepthHeapReserved) {
        freeCameraMem(mDepthHeapReserved);
        mDepthHeapReserved = NULL;
    }

    if (NULL != mPdafRawHeapReserved) {
        freeCameraMem(mPdafRawHeapReserved);
        mPdafRawHeapReserved = NULL;
    }

    if (NULL != m3DNRHeapReserverd) {
        freeCameraMem(m3DNRHeapReserverd);
        m3DNRHeapReserverd = NULL;
    }
    for (i = 0; i < CAP_3DNR_NUM; i++) {
        if (NULL != m3DNRScaleHeapReserverd[i]) {
            m3DNRScaleHeapReserverd[i]->ion_heap->free_kaddr();
            freeCameraMem(m3DNRScaleHeapReserverd[i]);
            m3DNRScaleHeapReserverd[i] = NULL;
        }
    }
    for (i = 0; i < PRE_3DNR_NUM; i++) {
        if (NULL != m3DNRPrevHeapReserverd[i]) {
            m3DNRPrevHeapReserverd[i]->ion_heap->free_kaddr();
            freeCameraMem(m3DNRPrevHeapReserverd[i]);
            m3DNRPrevHeapReserverd[i] = NULL;
        }
    }

    if (NULL != mPrevDepthHeapReserved) {
        mPrevDepthHeapReserved->ion_heap->free_kaddr();
        freeCameraMem(mPrevDepthHeapReserved);
    }
    mPrevDepthHeapReserved = NULL;

    if (NULL != mPrevSwOutHeapReserved) {
        mPrevSwOutHeapReserved->ion_heap->free_kaddr();
        freeCameraMem(mPrevSwOutHeapReserved);
    }
    mPrevSwOutHeapReserved = NULL;

    for (i = 0; i < (int)mChannel2HeapNum; i++) {
        if (NULL != mChannel2Heap[i]) {
            freeCameraMem(mChannel2Heap[i]);
        }
        mChannel2Heap[i] = NULL;
    }

    for (i = 0; i < (int)mChannel3HeapNum; i++) {
        if (NULL != mChannel3Heap[i]) {
            freeCameraMem(mChannel3Heap[i]);
        }
        mChannel3Heap[i] = NULL;
    }

    if (NULL != mAISceneScaleHeapReserverd) {
        freeCameraMem(mAISceneScaleHeapReserverd);
        mAISceneScaleHeapReserverd = NULL;
    }

    freeRawBuffers();

    HAL_LOGI(":hal3: X");
}

bool SprdCamera3OEMIf::initPreview() {
    HAL_LOGD("E");

    setPreviewParams();

    HAL_LOGD("X");
    return true;
}

void SprdCamera3OEMIf::deinitPreview() {
    HAL_LOGV("E");
    FACE_Tag faceInfo;
    mSetting->getFACETag(&faceInfo);
    if (faceInfo.face_num > 0) {
        memset(&faceInfo, 0, sizeof(FACE_Tag));
        mSetting->setFACETag(&faceInfo);
    }

    HAL_LOGV("X");
}

void SprdCamera3OEMIf::deinitCapture(bool isPreAllocCapMem) {
    HAL_LOGD("E %d", isPreAllocCapMem);

    if (0 == isPreAllocCapMem) {
        Callback_CaptureFree(0, 0, 0, 0);
    } else {
        HAL_LOGD("pre_allocate mode.");
    }

    Callback_CapturePathFree(0, 0, 0, 0);
}

int SprdCamera3OEMIf::chooseDefaultThumbnailSize(uint32_t *thumbWidth,
                                                 uint32_t *thumbHeight) {
    int i;
    JPEG_Tag jpgInfo;
    float capRatio = 0.0f, thumbRatio = 0.0f, diff = 1.0f, minDiff = 1.0f;
    unsigned char thumbCnt = 0;
    unsigned char matchCnt = 0;

    HAL_LOGV("mCaptureWidth=%d, mCaptureHeight=%d", mCaptureWidth,
             mCaptureHeight);
    mSetting->getJPEGTag(&jpgInfo);
    thumbCnt = sizeof(jpgInfo.available_thumbnail_sizes) /
               sizeof(jpgInfo.available_thumbnail_sizes[0]);
    if (mCaptureWidth > 0 && mCaptureHeight > 0) {
        capRatio = static_cast<float>(mCaptureWidth) / mCaptureHeight;
        for (i = 0; i < thumbCnt / 2; i++) {
            if (jpgInfo.available_thumbnail_sizes[2 * i] <= 0 ||
                jpgInfo.available_thumbnail_sizes[2 * i + 1] <= 0)
                continue;

            thumbRatio =
                static_cast<float>(jpgInfo.available_thumbnail_sizes[2 * i]) /
                jpgInfo.available_thumbnail_sizes[2 * i + 1];
            if (thumbRatio < 1.0f) {
                HAL_LOGE("available_thumbnail_sizes change sequences");
            }

            if (capRatio > thumbRatio)
                diff = capRatio - thumbRatio;
            else
                diff = thumbRatio - capRatio;

            if (diff < minDiff) {
                minDiff = diff;
                matchCnt = i;
            }
        }
    }

    if (minDiff < 1.0f) {
        *thumbWidth = jpgInfo.available_thumbnail_sizes[2 * matchCnt];
        *thumbHeight = jpgInfo.available_thumbnail_sizes[2 * matchCnt + 1];
    } else {
        *thumbWidth = 320;
        *thumbHeight = 240;
    }

    HAL_LOGV("JPEG thumbnail size = %d x %d", *thumbWidth, *thumbHeight);
    return 0;
}

int SprdCamera3OEMIf::startPreviewInternal() {
    ATRACE_CALL();

    cmr_int ret = 0;
    bool is_volte = false;
    char value[PROPERTY_VALUE_MAX];
    char multicameramode[PROPERTY_VALUE_MAX];
    struct img_size jpeg_thumb_size;

    HAL_LOGI("E camera id %d", mCameraId);

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (isCapturing()) {
        WaitForCaptureDone();
        cancelPictureInternal();
    }

    if (isPreviewStart()) {
        HAL_LOGV("Preview not start! wait preview start");
        WaitForPreviewStart();
    }

    if (isPreviewing()) {
        HAL_LOGD("Preview already in progress, mRestartFlag=%d", mRestartFlag);
        if (mRestartFlag == false) {
            return NO_ERROR;
        } else {
            stopPreviewInternal();
        }
    }

    mZslNum = 2;
    mZslMaxFrameNum = 1;
    mRestartFlag = false;
    mVideoCopyFromPreviewFlag = false;
    mVideo3dnrFlag = VIDEO_OFF;
    camera_ioctrl(CAMERA_IOCTRL_3DNR_VIDEOMODE, &mVideo3dnrFlag, NULL);

    if (mRecordingMode == false && sprddefInfo.sprd_zsl_enabled == 1) {
        mSprdZslEnabled = true;
    } else if (mRecordingMode == false && mStreamOnWithZsl == 1) {
        mSprdZslEnabled = true;
    } else if ((mRecordingMode == true && sprddefInfo.slowmotion > 1) ||
               (mRecordingMode == true && mVideoSnapshotType == 1)) {
        mSprdZslEnabled = false;
        if (sprddefInfo.sprd_3dnr_enabled == 1) {
            // 1 for 3dnr hw process.
            if (grab_capability.support_3dnr_mode == 1) {
                mVideoCopyFromPreviewFlag = false;
            } else {
                mVideoCopyFromPreviewFlag = true;
                mVideo3dnrFlag = VIDEO_ON;
                camera_ioctrl(CAMERA_IOCTRL_3DNR_VIDEOMODE, &mVideo3dnrFlag,
                              NULL);
            }
        }
    } else if (mRecordingMode == true && mVideoWidth != 0 &&
               mVideoHeight != 0 && mCaptureWidth != 0 && mCaptureHeight != 0) {
        mSprdZslEnabled = true;
        mZslNum = 3;
    } else if (mSprdRefocusEnabled == true && mCallbackHeight != 0 &&
               mCallbackWidth != 0) {
        mSprdZslEnabled = true;
    } else if (mSprd3dCalibrationEnabled == true && mRawHeight != 0 &&
               mRawWidth != 0) {
        mSprdZslEnabled = true;
    } else if (getMultiCameraMode() == MODE_BLUR ||
               getMultiCameraMode() == MODE_BOKEH ||
               getMultiCameraMode() == MODE_3D_CALIBRATION) {
        mSprdZslEnabled = true;
    } else {
        mSprdZslEnabled = false;
    }

    property_get("volte.incall.camera.enable", value, "false");
    if (!strcmp(value, "true")) {
        mSprdZslEnabled = false;
        CMR_LOGI("volte incall, don't need to configure zsl ");
    }

    if (!initPreview()) {
        HAL_LOGE("initPreview failed");
        deinitPreview();
        return UNKNOWN_ERROR;
    }
    SprdCamera3Flash::reserveFlash(mCameraId);

    // api1 dont set thumbnail size when preview, so we calculate the value same
    // as camera app
    chooseDefaultThumbnailSize(&jpeg_thumb_size.width, &jpeg_thumb_size.height);
    HAL_LOGD("JPEG thumbnail size = %d x %d", jpeg_thumb_size.width,
             jpeg_thumb_size.height);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);

    HAL_LOGD("mSprdZslEnabled=%d", mSprdZslEnabled);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)mSprdZslEnabled);

    HAL_LOGD("mVideoSnapshotType=%d", mVideoSnapshotType);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_VIDEO_SNAPSHOT_TYPE,
             (cmr_uint)mVideoSnapshotType);

    if (sprddefInfo.sprd_3dcapture_enabled) {
        mZslNum = DUALCAM_ZSL_NUM;
        mZslMaxFrameNum = DUALCAM_MAX_ZSL_NUM;
    }

    if (sprddefInfo.sprd_3dnr_enabled == 1 && mRecordingMode == false) {
        mZslNum = 5;
        mZslMaxFrameNum = 5;
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_MODE,
             (uint32_t)mCaptureMode);

    HAL_LOGD("mCaptureMode=%d", mCaptureMode);
    ret = mHalOem->ops->camera_start_preview(mCameraHandle, mCaptureMode);
    if (ret != CMR_CAMERA_SUCCESS) {
        HAL_LOGE("camera_start_preview failed");
        setCameraState(SPRD_ERROR, STATE_PREVIEW);
        deinitPreview();
        return UNKNOWN_ERROR;
    }
    if (mIspToolStart) {
        mIspToolStart = false;
    }

    setCameraState(SPRD_INTERNAL_PREVIEW_REQUESTED, STATE_PREVIEW);
    mPipelineStartSignal.signal();

    /*
    qFirstBuffer(CAMERA_STREAM_TYPE_PREVIEW);
    if (mVideoCopyFromPreviewFlag) {
        HAL_LOGI("copy preview buffer to video");
    } else {
        qFirstBuffer(CAMERA_STREAM_TYPE_VIDEO);
    }
    qFirstBuffer(CAMERA_STREAM_TYPE_CALLBACK);
    */

    // used for single camera need raw stream
    // queueRawBuffers();

    HAL_LOGI("X camera id %d", mCameraId);

    return NO_ERROR;
}

void SprdCamera3OEMIf::stopPreviewInternal() {
    ATRACE_CALL();

    nsecs_t start_timestamp = systemTime();
    nsecs_t end_timestamp;
    mUpdateRangeFpsCount = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    HAL_LOGI("E camera id %d", mCameraId);
    if (isCapturing()) {
        setCameraState(SPRD_INTERNAL_CAPTURE_STOPPING, STATE_CAPTURE);
        if (0 != mHalOem->ops->camera_cancel_takepicture(mCameraHandle)) {
            HAL_LOGE("camera_stop_capture failed!");
            return;
        }
    }

    if (isPreviewStart()) {
        HAL_LOGV("Preview not start! wait preview start");
        WaitForPreviewStart();
    }

    if (!isPreviewing()) {
        HAL_LOGD("Preview not in progress! stopPreviewInternal X");
        return;
    }

    mIsStoppingPreview = 1;
    setCameraState(SPRD_INTERNAL_PREVIEW_STOPPING, STATE_PREVIEW);
    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_stop_preview(mCameraHandle)) {
        setCameraState(SPRD_ERROR, STATE_PREVIEW);
        HAL_LOGE("camera_stop_preview failed");
    }
    mIsStoppingPreview = 0;

    deinitPreview();
    end_timestamp = systemTime();

#ifdef CONFIG_FACE_BEAUTY
    deinit_fb_handle(&face_beauty);
#endif

    // used for single camera need raw stream
    // freeRawBuffers();

    setCameraState(SPRD_IDLE, STATE_PREVIEW);

    HAL_LOGI("X Time:%" PRId64 " ms camera id %d",
             (end_timestamp - start_timestamp) / 1000000, mCameraId);
}

takepicture_mode SprdCamera3OEMIf::getCaptureMode() {
    HAL_LOGD("cap mode %d", mCaptureMode);

    return mCaptureMode;
}

bool SprdCamera3OEMIf::isJpegWithPreview() {
    if (CAMERA_ZSL_MODE == mCaptureMode)
        return false;

    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3Stream *stream = NULL;
    int32_t ret = BAD_VALUE;
    cmr_uint addr_vir = 0;

    if (channel) {
        channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);
        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
        }
    }

    return ret == NO_ERROR;
}

bool SprdCamera3OEMIf::iSZslMode() {
    bool ret = true;

    if (CAMERA_ZSL_MODE != mCaptureMode)
        ret = false;

    if (mSprdZslEnabled == 0)
        ret = false;

    return ret;
}

int SprdCamera3OEMIf::cancelPictureInternal() {
    ATRACE_CALL();

    bool result = true;
    HAL_LOGD("E, state %s", getCameraStateStr(getCaptureState()));

    switch (getCaptureState()) {
    case SPRD_INTERNAL_RAW_REQUESTED:
    case SPRD_WAITING_RAW:
    case SPRD_WAITING_JPEG:
    case SPRD_FLASH_IN_PROGRESS:
        HAL_LOGD("camera state is %s, stopping picture.",
                 getCameraStateStr(getCaptureState()));

        setCameraState(SPRD_INTERNAL_CAPTURE_STOPPING, STATE_CAPTURE);
        if (0 != mHalOem->ops->camera_cancel_takepicture(mCameraHandle)) {
            HAL_LOGE("camera_stop_capture failed!");
            return UNKNOWN_ERROR;
        }

        result = WaitForCaptureDone();
        if (!iSZslMode()) {
            deinitCapture(mIsPreAllocCapMem);
            // camera_set_capture_trace(0);
        }
        break;

    default:
        HAL_LOGW("not taking a picture (state %s)",
                 getCameraStateStr(getCaptureState()));
        break;
    }

    HAL_LOGD("X");
    return result ? NO_ERROR : UNKNOWN_ERROR;
}

void SprdCamera3OEMIf::setCameraPrivateData() {}

void SprdCamera3OEMIf::getPictureFormat(int *format) {
    *format = mPictureFormat;
}

int SprdCamera3OEMIf::CameraConvertCoordinateToFramework(int32_t *cropRegion) {
    float zoomWidth, zoomHeight;
    uint32_t i = 0;
    int ret = 0;
    uint16_t sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("crop %d %d %d %d", cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3]);

    mSetting->getLargestPictureSize(mCameraId, &sensorOrgW, &sensorOrgH);

    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return 1;
    }
    zoomWidth = (float)sensorOrgW / (float)mPreviewWidth;
    zoomHeight = (float)sensorOrgH / (float)mPreviewHeight;
    cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth);
    cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight);
    cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth);
    cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight);
    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)", cropRegion[0],
             cropRegion[1], cropRegion[2], cropRegion[3]);
    return ret;
}

int SprdCamera3OEMIf::CameraConvertCoordinateFromFramework(
    int32_t *cropRegion) {
    float zoomWidth, zoomHeight;
    uint32_t i = 0;
    int ret = 0;
    uint16_t sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("crop %d %d %d %d", cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3]);

    mSetting->getLargestPictureSize(mCameraId, &sensorOrgW, &sensorOrgH);

    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return 1;
    }
    zoomWidth = (float)mPreviewWidth / (float)sensorOrgW;
    zoomHeight = (float)mPreviewHeight / (float)sensorOrgH;
    cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth);
    cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight);
    cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth);
    cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight);
    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d)", cropRegion[0],
             cropRegion[1], cropRegion[2], cropRegion[3]);
    return ret;
}

void SprdCamera3OEMIf::receivePreviewFDFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();

    if (NULL == frame) {
        HAL_LOGE("invalid frame pointer");
        return;
    }
    Mutex::Autolock l(&mPreviewCbLock);
    FACE_Tag faceInfo;

    ssize_t offset = frame->buf_id;
    // camera_frame_metadata_t metadata;
    // camera_face_t face_info[FACE_DETECT_NUM];
    int32_t k = 0;
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t ex = 0;
    int32_t ey = 0;
    struct img_rect rect = {0, 0, 0, 0};
    mSetting->getFACETag(&faceInfo);
    memset(&faceInfo, 0, sizeof(FACE_Tag));
    HAL_LOGV("receive face_num %d.mid=%d", frame->face_num, mCameraId);
    int32_t number_of_faces =
        frame->face_num <= FACE_DETECT_NUM ? frame->face_num : FACE_DETECT_NUM;
    faceInfo.face_num = number_of_faces;

    if (0 != number_of_faces) {
        for (k = 0; k < number_of_faces; k++) {
            faceInfo.face[k].id = k;
            sx = MIN(MIN(frame->face_info[k].sx, frame->face_info[k].srx),
                     MIN(frame->face_info[k].ex, frame->face_info[k].elx));
            sy = MIN(MIN(frame->face_info[k].sy, frame->face_info[k].sry),
                     MIN(frame->face_info[k].ey, frame->face_info[k].ely));
            ex = MAX(MAX(frame->face_info[k].sx, frame->face_info[k].srx),
                     MAX(frame->face_info[k].ex, frame->face_info[k].elx));
            ey = MAX(MAX(frame->face_info[k].sy, frame->face_info[k].sry),
                     MAX(frame->face_info[k].ey, frame->face_info[k].ely));
            HAL_LOGD("face rect:s(%d,%d) sr(%d,%d) e(%d,%d) el(%d,%d)",
                     frame->face_info[k].sx, frame->face_info[k].sy,
                     frame->face_info[k].srx, frame->face_info[k].sry,
                     frame->face_info[k].ex, frame->face_info[k].ey,
                     frame->face_info[k].elx, frame->face_info[k].ely);
            faceInfo.face[k].rect[0] = sx;
            faceInfo.face[k].rect[1] = sy;
            faceInfo.face[k].rect[2] = ex;
            faceInfo.face[k].rect[3] = ey;
            faceInfo.angle[k] = frame->face_info[k].angle;
            faceInfo.pose[k] = frame->face_info[k].pose;
            HAL_LOGD("smile level %d. face:%d  %d  %d  %d ,angle %d\n",
                     frame->face_info[k].smile_level, faceInfo.face[k].rect[0],
                     faceInfo.face[k].rect[1], faceInfo.face[k].rect[2],
                     faceInfo.face[k].rect[3], faceInfo.angle[k]);
            CameraConvertCoordinateToFramework(faceInfo.face[k].rect);
            /*When the half of face at the edge of the screen,the smile level
            returned by face detection library  can often more than 30.
            In order to repaier this defetion ,so when the face on the screen is
            too close to the edge of screen, the smile level will be set to 0.
            */
            faceInfo.face[k].score = frame->face_info[k].smile_level;
            if (faceInfo.face[k].score < 0)
                faceInfo.face[k].score = 0;
        }
    }

    mSetting->setFACETag(&faceInfo);
}

/*
* check if iommu is enabled
* return val:
* 1: iommu is enabled
* 0: iommu is disabled
*/
int SprdCamera3OEMIf::IommuIsEnabled(void) {
    int ret;
    int iommuIsEnabled = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    ret = mHalOem->ops->camera_get_iommu_status(mCameraHandle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

void SprdCamera3OEMIf::prepareForPostProcess(void) { return; }

void SprdCamera3OEMIf::exitFromPostProcess(void) { return; }

void SprdCamera3OEMIf::calculateTimestampForSlowmotion(int64_t frm_timestamp) {
    int64_t diff_timestamp = 0;
    uint8_t tmp_slow_mot = 1;
    SPRD_DEF_Tag sprddefInfo;

    diff_timestamp = frm_timestamp - mSlowPara.last_frm_timestamp;
    // Google handle slowmotion timestamp at framework, therefore, we don't
    // multiply slowmotion ratio

    mSlowPara.rec_timestamp += diff_timestamp * tmp_slow_mot;
    mSlowPara.last_frm_timestamp = frm_timestamp;
}

#ifdef CONFIG_SPRD_LCD_FLASH
bool SprdCamera3OEMIf::set_lcd_brightness(int brightness) {
    char set_brightness[8];
    int fd;
    int ret = 0;
    fd = open(LCD_BACKLIGHT_MAX_DEV, O_RDWR);
    if (-1 == fd) {
        HAL_LOGE("unable to open max brightness interface");
        return false;
    }
    if (-1 == mResetBrightness) {
        memset(set_brightness, 0, sizeof(set_brightness));
        if (-1 == (ret = read(fd, set_brightness, sizeof(set_brightness)))) {
            HAL_LOGE("Error: Could not read from file %s\n", strerror(errno));
            return false;
        }
        set_brightness[ret - 1] = '\0';
        mResetBrightness = atoi(set_brightness);
        HAL_LOGI("mResetBrightness : %d", mResetBrightness);
    }
    memset(set_brightness, 0, sizeof(set_brightness));
    sprintf(set_brightness, "%d", brightness);
    if (-1 == (ret = write(fd, set_brightness, strlen(set_brightness)))) {
        HAL_LOGE("Error Could not write to file %s\n", strerror(errno));
        return false;
    }
    close(fd);
    HAL_LOGI("X:%s", __func__);
    return true;
}
#endif

bool SprdCamera3OEMIf::isFaceBeautyOn(SPRD_DEF_Tag sprddefInfo) {
    for (int i = 0; i < SPRD_FACE_BEAUTY_PARAM_NUM; i++) {
        if (sprddefInfo.perfect_skin_level[i] != 0)
            return true;
    }
    return false;
}

void SprdCamera3OEMIf::receivePreviewFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();

    Mutex::Autolock cbLock(&mPreviewCbLock);
    int ret = NO_ERROR;

    HAL_LOGV("E");
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops ||
        NULL == frame) {
        HAL_LOGE("mCameraHandle=%p, mHalOem=%p,", mCameraHandle, mHalOem);
        HAL_LOGE("frame=%p", frame);
        return;
    }

    if (miSPreviewFirstFrame) {
        GET_END_TIME;
        GET_USE_TIME;
        HAL_LOGD("Launch Camera Time:%d(ms).", s_use_time);

        float cam_init_time;
        if (getApctCamInitSupport()) {
            cam_init_time =
                ((float)(systemTime() - cam_init_begin_time)) / 1000000000;
            writeCamInitTimeToProc(cam_init_time);
        }
        miSPreviewFirstFrame = 0;

        if (getMultiCameraMode() == MODE_BLUR ||
            getMultiCameraMode() == MODE_BOKEH ||
            mSprdAppmodeId == CAMERA_MODE_PANORAMA ||
            mSprdAppmodeId == CAMERA_MODE_3DNR_PHOTO) {
            setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_4);
        } else if (mRecordingMode == true) {
            setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_2);
        } else if (mSprdAppmodeId == CAMERA_MODE_CONTINUE) {
            setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
        } else {
            setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_1);
        }
    }

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    if (mIsIspToolMode) {
        if (PREVIEW_FRAME == frame->type) {
            send_img_data(ISP_TOOL_YVU420_2FRAME, mPreviewWidth, mPreviewHeight,
                          (char *)frame->y_vir_addr,
                          frame->width * frame->height * 3 / 2);
        }
    }

#if 1 // for cts
    int64_t buffer_timestamp_fps = 0;

    if ((!mRecordingMode) && (mUpdateRangeFpsCount >= UPDATE_RANGE_FPS_COUNT)) {
        struct cmr_range_fps_param fps_param;
        CONTROL_Tag controlInfo;
        mSetting->getCONTROLTag(&controlInfo);

        fps_param.min_fps = controlInfo.ae_target_fps_range[0];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];

        int64_t fps_range_up = 0;
        int64_t fps_range_low = 0;
        int64_t fps_range_offset = 0;
        int64_t timestamp = 0;

        fps_range_offset = 1000000000 / fps_param.max_fps;
        fps_range_low =
            (int64_t)((1000000000.0 / fps_param.max_fps) * (1 - 0.015));
        fps_range_up =
            (int64_t)((1000000000.0 / fps_param.min_fps) * (1 + 0.015));

        if (mPrvBufferTimestamp == 0) {
            buffer_timestamp_fps = frame->timestamp;
        } else {
            if (mIsUpdateRangeFps) {
                buffer_timestamp_fps = frame->timestamp;
                mIsUpdateRangeFps = false;
            } else {
                timestamp = frame->timestamp - mPrvBufferTimestamp;
                HAL_LOGD("timestamp is %" PRId64, timestamp);
                if ((timestamp > fps_range_low) && (timestamp < fps_range_up)) {
                    buffer_timestamp_fps = frame->timestamp;
                } else {
                    buffer_timestamp_fps =
                        mPrvBufferTimestamp + fps_range_offset;
                    HAL_LOGV("fix buffer_timestamp_fps is %" PRId64,
                             buffer_timestamp_fps);
                }
            }
        }
        mPrvBufferTimestamp = buffer_timestamp_fps;
    } else {
        buffer_timestamp_fps = frame->timestamp;
    }
#endif

#ifdef CONFIG_CAMERA_EIS
    vsOutFrame frame_out;
    frame_out.frame_data = NULL;
    int64_t buffer_timestamp;
    if (sprddefInfo.sprd_eis_enabled) {
        buffer_timestamp = buffer_timestamp_fps - frame->ae_time / 2;
    } else {
        buffer_timestamp = buffer_timestamp_fps;
    }
#else
    int64_t buffer_timestamp = buffer_timestamp_fps;
#endif

    if (0 == buffer_timestamp)
        HAL_LOGE("buffer_timestamp shouldn't be 0,please check your code");

    VCM_Tag sprdvcmInfo;
    if ((mSprdRefocusEnabled == true || getMultiCameraMode() == MODE_BOKEH) &&
        mCameraId == 0) {
        mSetting->getVCMTag(&sprdvcmInfo);
        uint32_t vcm_step = 0;
        mHalOem->ops->camera_get_sensor_vcm_step(mCameraHandle, mCameraId,
                                                 &vcm_step);
        HAL_LOGD("vcm step is 0x%x", vcm_step);
        sprdvcmInfo.vcm_step = vcm_step;
        mSetting->setVCMTag(sprdvcmInfo);
    }

    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint pre_addr_vir = 0, rec_addr_vir = 0, callback_addr_vir = 0;
    SprdCamera3Stream *pre_stream = NULL, *rec_stream = NULL,
                      *callback_stream = NULL;
    int32_t pre_dq_num;
    uint32_t frame_num = 0;
    cmr_uint buff_vir = (cmr_uint)(frame->y_vir_addr);
    uint32_t buff_id = frame->buf_id;
    uint16_t img_width = frame->width, img_height = frame->height;
    buffer_handle_t *buff_handle = NULL;
    int32_t buf_deq_num = 0;
    int32_t buf_num = 0;

    cmr_uint videobuf_phy = 0;
    cmr_uint videobuf_vir = 0;
    cmr_uint prebuf_phy = 0;
    cmr_uint prebuf_vir = 0;
    cmr_s32 fd0 = 0;
    cmr_s32 fd1 = 0;

#ifdef CONFIG_FACE_BEAUTY
    int sx, sy, ex, ey, angle, pose;
    struct face_beauty_levels beautyLevels;
    beautyLevels.blemishLevel =
        (unsigned char)sprddefInfo.perfect_skin_level[0];
    beautyLevels.smoothLevel = (unsigned char)sprddefInfo.perfect_skin_level[1];
    beautyLevels.skinColor = (unsigned char)sprddefInfo.perfect_skin_level[2];
    beautyLevels.skinLevel = (unsigned char)sprddefInfo.perfect_skin_level[3];
    beautyLevels.brightLevel = (unsigned char)sprddefInfo.perfect_skin_level[4];
    beautyLevels.lipColor = (unsigned char)sprddefInfo.perfect_skin_level[5];
    beautyLevels.lipLevel = (unsigned char)sprddefInfo.perfect_skin_level[6];
    beautyLevels.slimLevel = (unsigned char)sprddefInfo.perfect_skin_level[7];
    beautyLevels.largeLevel = (unsigned char)sprddefInfo.perfect_skin_level[8];
#endif
    SENSOR_Tag sensorInfo;

    mSetting->getSENSORTag(&sensorInfo);
    sensorInfo.timestamp = buffer_timestamp;
    mSetting->setSENSORTag(sensorInfo);

    if (channel == NULL) {
        HAL_LOGE("channel=%p", channel);
        goto exit;
    }

    channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
    channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &rec_stream);
    channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &callback_stream);
    HAL_LOGV("pre_stream %p, rec_stream %p, callback_stream %p", pre_stream,
             rec_stream, callback_stream);

#ifdef CONFIG_FACE_BEAUTY
    if (PREVIEW_ZSL_FRAME != frame->type && isFaceBeautyOn(sprddefInfo)) {
        faceDectect(1);
        if (isPreviewing() && frame->type == PREVIEW_FRAME) {
            if (MODE_3D_VIDEO != mMultiCameraMode &&
                MODE_3D_PREVIEW != mMultiCameraMode) {
                FACE_Tag faceInfo;
                mSetting->getFACETag(&faceInfo);
                if (faceInfo.face_num > 0) {
                    for (int i = 0; i < faceInfo.face_num; i++) {
                        CameraConvertCoordinateFromFramework(
                            faceInfo.face[i].rect);
                        sx = faceInfo.face[i].rect[0];
                        sy = faceInfo.face[i].rect[1];
                        ex = faceInfo.face[i].rect[2];
                        ey = faceInfo.face[i].rect[3];
                        angle = faceInfo.angle[i];
                        pose = faceInfo.pose[i];
                        construct_fb_face(&face_beauty, i, sx, sy, ex, ey,
                                          angle, pose);
                    }
                }
                init_fb_handle(&face_beauty, 1, 2);
                construct_fb_image(
                    &face_beauty, frame->width, frame->height,
                    (unsigned char *)(frame->y_vir_addr),
                    (unsigned char *)(frame->y_vir_addr +
                                      frame->width * frame->height),
                    0);
                construct_fb_level(&face_beauty, beautyLevels);
                do_face_beauty(&face_beauty, faceInfo.face_num);
                flushIonBuffer(frame->fd, (void *)frame->y_vir_addr, 0,
                               frame->width * frame->height * 3 / 2);
            }
        }
    } else if (PREVIEW_ZSL_FRAME != frame->type) {
        deinit_fb_handle(&face_beauty);
    }
#endif

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.debug", value, "0");
    if (atoi(value) != 0) {
        img_debug img_debug;
        img_debug.input.addr_y = frame->y_vir_addr;
        img_debug.input.addr_u = frame->uv_vir_addr;
        img_debug.input.addr_v = img_debug.input.addr_u;
        // process img data on input addr, so set output addr to 0.
        img_debug.output.addr_y = 0;
        img_debug.output.addr_u = 0;
        img_debug.output.addr_v = img_debug.output.addr_u;
        img_debug.size.width = frame->width;
        img_debug.size.height = frame->height;
        img_debug.format = frame->format;
        FACE_Tag ftag;
        mSetting->getFACETag(&ftag);
        img_debug.params = &ftag;
        ret = mHalOem->ops->camera_ioctrl(mCameraHandle,
                                          CAMERA_IOCTRL_DEBUG_IMG, &img_debug);
    }

    if (rec_stream) {
        ret = rec_stream->getQBufNumForVir(buff_vir, &frame_num);
        if (ret) {
            goto bypass_rec;
        }

        if (mFrameSyncFlag == 1 && frame_num >= mFrameSyncNum) {
            mFrameSyncSignal.signal();
        }

        ATRACE_BEGIN("video_frame");
        HAL_LOGD("record:fd=%d, vir=0x%lx, num=%d, time=%" PRId64
                 ", rec=%" PRId64,
                 frame->fd, buff_vir, frame_num, buffer_timestamp,
                 mSlowPara.rec_timestamp);
        if (frame->type == PREVIEW_VIDEO_FRAME) {
            if (mVideoWidth <= mCaptureWidth &&
                mVideoHeight <= mCaptureHeight) {
                if (mVideoShotFlag && (frame_num >= mVideoSnapshotFrameNum))
                    PushVideoSnapShotbuff(frame_num, CAMERA_STREAM_TYPE_VIDEO);
            }

            if (mSlowPara.last_frm_timestamp == 0) { /*record first frame*/
                mSlowPara.last_frm_timestamp = buffer_timestamp;
                mSlowPara.rec_timestamp = buffer_timestamp;
                mIsRecording = true;
            }
            calculateTimestampForSlowmotion(buffer_timestamp);

#ifdef CONFIG_CAMERA_EIS
            HAL_LOGV("eis_enable = %d", sprddefInfo.sprd_eis_enabled);
            if (sprddefInfo.sprd_eis_enabled) {
                frame_out = EisVideoFrameStab(frame, frame_num);

                if (frame_out.frame_data) {
                    channel->channelCbRoutine(frame_out.frame_num,
                                              frame_out.timestamp * 1000000000,
                                              CAMERA_STREAM_TYPE_VIDEO);
                    frame_out.frame_data = NULL;
                }
                goto bypass_rec;
            }
#endif
            if (mMultiCameraMode == MODE_3D_VIDEO) {
                mSlowPara.rec_timestamp = buffer_timestamp;
            }
            channel->channelCbRoutine(frame_num, mSlowPara.rec_timestamp,
                                      CAMERA_STREAM_TYPE_VIDEO);
            if (frame_num == (mDropVideoFrameNum + 1)) // for IOMMU error
                channel->channelClearInvalidQBuff(mDropVideoFrameNum,
                                                  buffer_timestamp,
                                                  CAMERA_STREAM_TYPE_VIDEO);
        } else {
            channel->channelClearInvalidQBuff(frame_num, buffer_timestamp,
                                              CAMERA_STREAM_TYPE_VIDEO);
        }

        ATRACE_END();

    bypass_rec:
        HAL_LOGV("rec_stream X");
    }

    if (pre_stream) {
        ret = pre_stream->getQBufNumForVir(buff_vir, &frame_num);
        if (ret) {
            pre_stream = NULL;
            goto bypass_pre;
        }
        ATRACE_BEGIN("preview_frame");

        HAL_LOGD("prev:fd=%d, vir=0x%lx, num=%d, time=%" PRId64, frame->fd,
                 buff_vir, frame_num, buffer_timestamp);
#ifdef CONFIG_SPRD_LCD_FLASH
        if (mCameraId == 1 && mFlashCaptureFlag == 1) {
            HAL_LOGI("LCDFLASH : Whitening the frame ");
            if (-1 == mResetBrightness) {
                if (!set_lcd_brightness(MAX_BRIGHTNESS))
                    HAL_LOGE("Error : Could not set max brightness!!!!!");
            }
            int y_size, uv_size;
            y_size = (frame->width * frame->height);
            uv_size = (y_size / 2);
            memset((void *)buff_vir, 255, y_size);
            memset((void *)(buff_vir + y_size), 128, uv_size);
        } else if (-1 != mResetBrightness) {
            HAL_LOGI("Resetting brightness to its actual brightness %d",
                     mResetBrightness);
            if (!set_lcd_brightness(mResetBrightness))
                HAL_LOGE("Error : Could not set previous brightness!!!!!");
            mResetBrightness = -1;
        }
#endif

        if (frame->type == PREVIEW_FRAME) {
#ifdef CONFIG_CAMERA_EIS
            HAL_LOGV("eis_enable = %d", sprddefInfo.sprd_eis_enabled);
            if (mRecordingMode && sprddefInfo.sprd_eis_enabled) {
                EisPreviewFrameStab(frame);
            }
#endif

            if (mVideoCopyFromPreviewFlag) {
                if (rec_stream) {
                    ret = rec_stream->getQBufAddrForNum(
                        frame_num, &videobuf_vir, &videobuf_phy, &fd0);
                    if (ret == NO_ERROR && videobuf_vir != 0) {
                        mIsRecording = true;
                        if (mSlowPara.last_frm_timestamp == 0) {
                            mSlowPara.last_frm_timestamp = buffer_timestamp;
                            mSlowPara.rec_timestamp = buffer_timestamp;
                        }
                    }
                }
            }

            if (mFrameSyncFlag == 1 && frame_num >= mFrameSyncNum) {
                mFrameSyncSignal.signal();
            }

            if (mIsRecording && rec_stream) {
                calculateTimestampForSlowmotion(buffer_timestamp);

                if (mVideoCopyFromPreviewFlag) {
                    ret = rec_stream->getQBufAddrForNum(
                        frame_num, &videobuf_vir, &videobuf_phy, &fd0);
                    if (ret || videobuf_vir == 0) {
                        HAL_LOGE("getQBufAddrForNum failed");
                        goto exit;
                    }

                    pre_stream->getQBufAddrForNum(frame_num, &prebuf_vir,
                                                  &prebuf_phy, &fd1);
                    HAL_LOGV(
                        "frame_num=%d, videobuf_phy=0x%lx, videobuf_vir=0x%lx",
                        frame_num, videobuf_phy, videobuf_vir);
                    HAL_LOGV("frame_num=%d, prebuf_phy=0x%lx, prebuf_vir=0x%lx",
                             frame_num, prebuf_phy, prebuf_vir);
                    memcpy((void *)videobuf_vir, (void *)prebuf_vir,
                           mPreviewWidth * mPreviewHeight * 3 / 2);
                    flushIonBuffer(fd0, (void *)videobuf_vir, 0,
                                   mPreviewWidth * mPreviewHeight * 3 / 2);
                    channel->channelCbRoutine(frame_num,
                                              mSlowPara.rec_timestamp,
                                              CAMERA_STREAM_TYPE_VIDEO);
                }

                channel->channelCbRoutine(frame_num, mSlowPara.rec_timestamp,
                                          CAMERA_STREAM_TYPE_PREVIEW);
            } else {
                channel->channelCbRoutine(frame_num, buffer_timestamp,
                                          CAMERA_STREAM_TYPE_PREVIEW);
            }
        } else {
            channel->channelClearInvalidQBuff(frame_num, buffer_timestamp,
                                              CAMERA_STREAM_TYPE_PREVIEW);
        }

        if (mTakePictureMode == SNAPSHOT_PREVIEW_MODE) {
            timer_set(this, 1, timer_hand_take);
        }

        if (mSprdCameraLowpower && (0 == frame_num % 100)) {
            adjustFpsByTemp();
        }

        ATRACE_END();
    bypass_pre:
        HAL_LOGV("pre_stream X");
    }

    if (callback_stream) {
        uint32_t pic_frame_num;
        bool isJpegRequest = false;
        SprdCamera3Stream *local_pic_stream = NULL;
        SprdCamera3PicChannel *local_pic_channel =
            reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);

        ret = callback_stream->getQBufNumForVir(buff_vir, &frame_num);
        if (ret) {
            goto bypass_callback;
        }

        ATRACE_BEGIN("callback_frame");

        HAL_LOGD("callback fd=%d, vir=0x%lx, frame_num %d, time %" PRId64
                 ", frame type = %ld",
                 frame->fd, buff_vir, frame_num, buffer_timestamp, frame->type);

        channel->channelCbRoutine(frame_num, buffer_timestamp,
                                  CAMERA_STREAM_TYPE_CALLBACK);

        if (mFrameSyncFlag == 1 && frame_num >= mFrameSyncNum) {
            mFrameSyncSignal.signal();
        }

        if ((mTakePictureMode == SNAPSHOT_PREVIEW_MODE) &&
            (isJpegRequest == true)) {
            timer_set(this, 1, timer_hand_take);
        }
        ATRACE_END();

    bypass_callback:
        HAL_LOGV("callback_stream X");
    }

    if (mSprdZslEnabled) {
        cmr_int need_pause;
        CMR_MSG_INIT(message);
        mHalOem->ops->camera_zsl_snapshot_need_pause(mCameraHandle,
                                                     &need_pause);
        if (PREVIEW_ZSL_FRAME == frame->type) {
            ATRACE_BEGIN("zsl_frame");

            HAL_LOGD("zsl buff fd=0x%x, frame type=%ld", frame->fd,
                     frame->type);
            pushZslFrame(frame);
            sem_post(&((SprdCamera3OEMIf *)this)->mZslQueueSemDone);

            if (getZSLQueueFrameNum() > mZslMaxFrameNum) {
                struct camera_frame_type zsl_frame;
                zsl_frame = popZslFrame();
                if (zsl_frame.y_vir_addr != 0) {
                    mHalOem->ops->camera_set_zsl_buffer(
                        mCameraHandle, zsl_frame.y_phy_addr,
                        zsl_frame.y_vir_addr, zsl_frame.fd);
                }
            }

            ATRACE_END();
        } else if (PREVIEW_ZSL_CANCELED_FRAME == frame->type) {
            if (!isCapturing() || !need_pause) {
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, frame->y_phy_addr, frame->y_vir_addr,
                    frame->fd);
            }
        }
    }

exit:
    HAL_LOGV("X");
}

void SprdCamera3OEMIf::receiveRawFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();

    Mutex::Autolock cbLock(&mPreviewCbLock);
    int ret = NO_ERROR;
    cam_buffer_info_t buffer;

    cmr_bzero(&buffer, sizeof(cam_buffer_info_t));

    HAL_LOGV("E");
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops ||
        NULL == frame) {
        HAL_LOGE("mCameraHandle=%p, mHalOem=%p,", mCameraHandle, mHalOem);
        HAL_LOGE("frame=%p", frame);
        return;
    }

    CMR_MSG_INIT(message);

    HAL_LOGD("fd=0x%x, frame type=%ld", frame->fd, frame->type);
    pushRawFrame(frame);

    if (getRawQueueFrameNum() > mRawMaxFrameNum) {
        struct camera_frame_type raw_frame;
        raw_frame = popRawFrame();
        if (raw_frame.y_vir_addr != 0) {
            buffer.fd = raw_frame.fd;
            buffer.addr_phy = (void *)raw_frame.y_phy_addr;
            buffer.addr_vir = (void *)raw_frame.y_vir_addr;
            mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                       SPRD_CAM_STREAM_RAW);
        }
    }

exit:
    HAL_LOGV("X");
}

bool SprdCamera3OEMIf::returnPreviewFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();
    Mutex::Autolock cbLock(&mPreviewCbLock);

    SprdCamera3Stream *pre_stream = NULL, *pic_stream = NULL;
    int32_t ret = 0;
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 ion_fd = 0;
    uint32_t frame_num = 0;
    uint8_t *src_y, *src_vu, *dst_y, *dst_vu;
    int src_width, src_height, dst_width, dst_height;
    int64_t timestamp = systemTime();

    SprdCamera3RegularChannel *regular_channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3PicChannel *pic_channel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);

    if (regular_channel == NULL || pic_channel == NULL) {
        HAL_LOGE("regular_channel or pic_channel is null");
        return 0;
    }

    regular_channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
    pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &pic_stream);
    if (pre_stream == NULL) {
        HAL_LOGE("pre_stream is null");
        return 0;
    }

    pre_stream->getQBuffFirstNum(&frame_num);
    pre_stream->getQBuffFirstVir(&addr_vir);
    pre_stream->getQBuffFirstPhy(&addr_phy);
    pre_stream->getQBuffFirstFd(&ion_fd);

    if (addr_vir == 0 || ion_fd == 0) {
        HAL_LOGW("addr_vir=%ld, ion_fd=%d", addr_vir, ion_fd);
        goto exit;
    }

    if ((int)frame->width == mPreviewWidth &&
        (int)frame->height == mPreviewHeight) {
        memcpy((void *)addr_vir, (void *)frame->y_vir_addr,
               (frame->width * frame->height * 3) / 2);
        flushIonBuffer(ion_fd, (void *)addr_vir, (void *)NULL,
                       mPreviewWidth * mPreviewHeight * 3 / 2);
    } else {
        // hardware scale
        if (0) {
            ret = mHalOem->ops->camera_get_redisplay_data(
                mCameraHandle, ion_fd, addr_phy, addr_vir, mPreviewWidth,
                mPreviewHeight, frame->fd, frame->y_phy_addr,
                frame->uv_phy_addr, frame->y_vir_addr, frame->width,
                frame->height);
            if (ret) {
                HAL_LOGE("camera_get_data_redisplay failed");
                goto exit;
            }
        } else {
            // software scale
            src_y = (uint8_t *)frame->y_vir_addr;
            src_vu =
                (uint8_t *)(frame->y_vir_addr + frame->width * frame->height);
            src_width = frame->width;
            src_height = frame->height;
            dst_y = (uint8_t *)addr_vir;
            dst_vu = (uint8_t *)(addr_vir + mPreviewWidth * mPreviewHeight);
            dst_width = mPreviewWidth;
            dst_height = mPreviewHeight;
            ret = nv21Scale(src_y, src_vu, src_width, src_height, dst_y, dst_vu,
                            dst_width, dst_height);
            if (ret) {
                CMR_LOGE("nv21Scale failed");
            }

            if (0) {
                mHalOem->ops->dump_jpeg_file((void *)src_y,
                                             src_width * src_height * 3 / 2,
                                             src_width, src_height);
                mHalOem->ops->dump_jpeg_file((void *)dst_y,
                                             dst_width * dst_height * 3 / 2,
                                             dst_width, dst_height);
            }

            flushIonBuffer(ion_fd, (void *)addr_vir, (void *)NULL,
                           mPreviewWidth * mPreviewHeight * 3 / 2);
        }
    }

    regular_channel->channelCbRoutine(frame_num, timestamp,
                                      CAMERA_STREAM_TYPE_PREVIEW);

exit:
    HAL_LOGD("X");
    return true;
}

bool SprdCamera3OEMIf::isJpegWithYuvCallback() {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3Stream *stream = NULL;
    int32_t ret = BAD_VALUE;
    cmr_uint addr_vir = 0;

    if (channel) {
        channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &stream);
        if (stream) {
            ret = stream->getQBuffFirstVir(&addr_vir);
        }
    }

    return ret == NO_ERROR;
}

bool SprdCamera3OEMIf::returnYuvCallbackFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();
    Mutex::Autolock cbLock(&mPreviewCbLock);

    SprdCamera3Stream *callback_stream = NULL, *pic_stream = NULL;
    int32_t ret = 0;
    cmr_uint addr_vir = 0, addr_phy = 0;
    cmr_s32 ion_fd = 0;
    uint32_t frame_num = 0;
    uint8_t *src_y, *src_vu, *dst_y, *dst_vu;
    int src_width, src_height, dst_width, dst_height;
    int64_t timestamp = systemTime();

    SprdCamera3RegularChannel *regular_channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    SprdCamera3PicChannel *pic_channel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);

    if (regular_channel == NULL || pic_channel == NULL) {
        HAL_LOGE("regular_channel or pic_channel is null");
        return 0;
    }

    regular_channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &callback_stream);
    if (callback_stream == NULL) {
        HAL_LOGE("pre_stream is null");
        return 0;
    }
    pic_channel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &pic_stream);

    callback_stream->getQBuffFirstNum(&frame_num);
    callback_stream->getQBuffFirstVir(&addr_vir);
    callback_stream->getQBuffFirstPhy(&addr_phy);
    callback_stream->getQBuffFirstFd(&ion_fd);

    if (addr_vir == 0 || ion_fd == 0) {
        HAL_LOGW("addr_vir=%ld, ion_fd=%d", addr_vir, ion_fd);
        goto exit;
    }

    if ((int)frame->width == mCallbackWidth &&
        (int)frame->height == mCallbackHeight) {
        memcpy((void *)addr_vir, (void *)frame->y_vir_addr,
               (mCallbackWidth * mCallbackHeight * 3) / 2);
        flushIonBuffer(ion_fd, (void *)addr_vir, (void *)NULL,
                       mCallbackWidth * mCallbackHeight * 3 / 2);
    } else {
        // hardware scale
        if (0) {
            ret = mHalOem->ops->camera_get_redisplay_data(
                mCameraHandle, ion_fd, addr_phy, addr_vir, mCallbackWidth,
                mCallbackHeight, frame->fd, frame->y_phy_addr,
                frame->uv_phy_addr, frame->y_vir_addr, frame->width,
                frame->height);
            if (ret) {
                HAL_LOGE("failed");
            }
        } else {
            // software scale
            src_y = (uint8_t *)frame->y_vir_addr;
            src_vu =
                (uint8_t *)(frame->y_vir_addr + frame->width * frame->height);
            src_width = frame->width;
            src_height = frame->height;
            dst_y = (uint8_t *)addr_vir;
            dst_vu = (uint8_t *)(addr_vir + mCallbackWidth * mCallbackHeight);
            dst_width = mCallbackWidth;
            dst_height = mCallbackHeight;
            ret = nv21Scale(src_y, src_vu, src_width, src_height, dst_y, dst_vu,
                            dst_width, dst_height);
            if (ret) {
                CMR_LOGE("nv21Scale failed");
            }

            if (0) {
                mHalOem->ops->dump_jpeg_file((void *)src_y,
                                             src_width * src_height * 3 / 2,
                                             src_width, src_height);
                mHalOem->ops->dump_jpeg_file((void *)dst_y,
                                             dst_width * dst_height * 3 / 2,
                                             dst_width, dst_height);
            }

            flushIonBuffer(ion_fd, (void *)addr_vir, (void *)NULL,
                           mCallbackWidth * mCallbackHeight * 3 / 2);
        }
    }

    regular_channel->channelCbRoutine(frame_num, timestamp,
                                      CAMERA_STREAM_TYPE_CALLBACK);

exit:
    HAL_LOGD("X");
    return true;
}

int SprdCamera3OEMIf::nv21Scale(const uint8_t *src_y, const uint8_t *src_vu,
                                int src_width, int src_height, uint8_t *dst_y,
                                uint8_t *dst_vu, int dst_width,
                                int dst_height) {
    int ret = 0;
    uint8_t *i420_src = (uint8_t *)malloc(src_width * src_height * 3 / 2);
    uint8_t *i420_src_u = (uint8_t *)i420_src + src_width * src_height;
    uint8_t *i420_src_v = (uint8_t *)i420_src + src_width * src_height * 5 / 4;
    uint8_t *i420_dst = (uint8_t *)malloc(dst_width * dst_height * 3 / 2);
    uint8_t *i420_dst_u = (uint8_t *)i420_dst + dst_width * dst_height;
    uint8_t *i420_dst_v = (uint8_t *)i420_dst + dst_width * dst_height * 5 / 4;

    ret = libyuv::NV21ToI420(src_y, src_width, src_vu, src_width, i420_src,
                             src_width, i420_src_u, src_width / 2, i420_src_v,
                             src_width / 2, src_width, src_height);
    if (ret) {
        HAL_LOGE("libyuv::NV21ToI420 failed");
        goto exit;
    }

    ret = libyuv::I420Scale(i420_src, src_width, i420_src_u, src_width / 2,
                            i420_src_v, src_width / 2, src_width, src_height,
                            i420_dst, dst_width, i420_dst_u, dst_width / 2,
                            i420_dst_v, dst_width / 2, dst_width, dst_height,
                            libyuv::kFilterBox);
    if (ret) {
        HAL_LOGE("libyuv::I420Scale failed");
        goto exit;
    }

    ret = libyuv::I420ToNV21(i420_dst, dst_width, i420_dst_u, dst_width / 2,
                             i420_dst_v, dst_width / 2, dst_y, dst_width,
                             dst_vu, dst_width, dst_width, dst_height);
    if (ret) {
        HAL_LOGE("libyuv::I420ToNV21 failed");
        goto exit;
    }

exit:
    free(i420_src);
    free(i420_dst);

    return ret;
}

void SprdCamera3OEMIf::yuvNv12ConvertToYv12(struct camera_frame_type *frame,
                                            char *tmpbuf) {
    int width, height;

    width = frame->width;
    height = frame->height;
    if (tmpbuf) {
        char *addr0 = (char *)frame->y_vir_addr + width * height;
        char *addr1 = addr0 + SIZE_ALIGN(width / 2) * height / 2;
        char *addr2 = (char *)tmpbuf;

        memcpy((void *)tmpbuf, (void *)addr0, width * height / 2);
        if (width % 32) {
            int gap = SIZE_ALIGN(width / 2) - width / 2;
            for (int i = 0; i < width * height / 4; i++) {
                *addr0++ = *addr2++;
                *addr1++ = *addr2++;
                if (!((i + 1) % (width / 2))) {
                    addr0 = addr0 + gap;
                    addr1 = addr1 + gap;
                }
            }
        } else {
            for (int i = 0; i < width * height / 4; i++) {
                *addr0++ = *addr2++;
                *addr1++ = *addr2++;
            }
        }
    }
}

cmr_int save_yuv_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width,
                         cmr_u32 height, struct img_addr *addr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char file_name[40] = {
        0,
    };
    char tmp_str[10] = {
        0,
    };
    FILE *fp = NULL;

    HAL_LOGD("index %d format %d width %d heght %d", index, img_fmt, width,
             height);

    strcpy(file_name, CAMERA_DUMP_PATH);
    sprintf(tmp_str, "%d", width);
    strcat(file_name, tmp_str);
    strcat(file_name, "X");
    sprintf(tmp_str, "%d", height);
    strcat(file_name, tmp_str);

    if (IMG_DATA_TYPE_YUV420 == img_fmt || IMG_DATA_TYPE_YUV422 == img_fmt) {
        strcat(file_name, "_y_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".raw");
        HAL_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");

        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, width * height, fp);
        fclose(fp);

        strcpy(file_name, CAMERA_DUMP_PATH);
        sprintf(tmp_str, "%d", width);
        strcat(file_name, tmp_str);
        strcat(file_name, "X");
        sprintf(tmp_str, "%d", height);
        strcat(file_name, tmp_str);
        strcat(file_name, "_uv_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".raw");
        HAL_LOGD("file name %s", file_name);
        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        if (IMG_DATA_TYPE_YUV420 == img_fmt) {
            fwrite((void *)addr->addr_u, 1, width * height / 2, fp);
        } else {
            fwrite((void *)addr->addr_u, 1, width * height, fp);
        }
        fclose(fp);
    } else if (IMG_DATA_TYPE_JPEG == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".jpg");
        HAL_LOGD("file name %s", file_name);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, width * height * 2, fp);
        fclose(fp);
    } else if (IMG_DATA_TYPE_RAW == img_fmt) {
        strcat(file_name, "_");
        sprintf(tmp_str, "%d", index);
        strcat(file_name, tmp_str);
        strcat(file_name, ".mipi_raw");
        HAL_LOGD("file name %s", file_name);

        fp = fopen(file_name, "wb");
        if (NULL == fp) {
            HAL_LOGE("can not open file: %s \n", file_name);
            return 0;
        }

        fwrite((void *)addr->addr_y, 1, (uint32_t)(width * height * 5 / 4), fp);
        fclose(fp);
    }
    return 0;
}

void SprdCamera3OEMIf::receiveRawPicture(struct camera_frame_type *frame) {
    ATRACE_CALL();

    Mutex::Autolock cbLock(&mCaptureCbLock);

    bool hasPreviewBuf, hasYuvCallbackBuf;
    int ret;
    int dst_fd = 0;
    cmr_uint dst_paddr = 0;
    uint32_t dst_width = 0;
    uint32_t dst_height = 0;
    cmr_uint dst_vaddr = 0;

    HAL_LOGD("E");
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (SPRD_INTERNAL_CAPTURE_STOPPING == getCaptureState()) {
        HAL_LOGW("warning: capture state = SPRD_INTERNAL_CAPTURE_STOPPING");
        goto exit;
    }

    if (NULL == frame) {
        HAL_LOGE("invalid frame pointer");
        goto exit;
    }

    if (mIsIspToolMode == 1) {
        HAL_LOGI("isp tool dont go this way");
        goto exit;
    }

    hasPreviewBuf = isJpegWithPreview();
    hasYuvCallbackBuf = isJpegWithYuvCallback();

    if (hasPreviewBuf) {
        returnPreviewFrame(frame);
    }

    if (hasYuvCallbackBuf) {
        returnYuvCallbackFrame(frame);
    }

exit:
    HAL_LOGD("X");
}

void SprdCamera3OEMIf::receiveJpegPicture(struct camera_frame_type *frame) {
    ATRACE_CALL();

    print_time();
    Mutex::Autolock cbLock(&mCaptureCbLock);
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    struct camera_jpeg_param *encInfo = &frame->jpeg_param;
    int64_t temp = 0, temp1 = 0;
    buffer_handle_t *jpeg_buff_handle = NULL;
    ssize_t maxJpegSize = -1;
    camera3_jpeg_blob *jpegBlob = NULL;
    int64_t timestamp;
    cmr_uint pic_addr_vir = 0x0;
    SprdCamera3Stream *pic_stream = NULL;
    int ret;
    uint32_t heap_size;
    SprdCamera3PicChannel *picChannel =
        reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
    SprdCamera3RegularChannel *regularChannel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    uint32_t frame_num = 0;
    char value[PROPERTY_VALUE_MAX];
    char debug_value[PROPERTY_VALUE_MAX];
    unsigned char is_raw_capture = 0;
    void *ispInfoAddr = NULL;
    int ispInfoSize = 0;

    HAL_LOGD("E encInfo->size = %d, enc->buffer = %p, encInfo->need_free = %d "
             "time=%" PRId64,
             encInfo->size, encInfo->outPtr, encInfo->need_free,
             frame->timestamp);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    SENSOR_Tag sensorInfo;
    mSetting->getSENSORTag(&sensorInfo);
    if (0 != frame->sensor_info.exposure_time_denominator) {
        sensorInfo.exposure_time = 1000000000ll *
                                   frame->sensor_info.exposure_time_numerator /
                                   frame->sensor_info.exposure_time_denominator;
    }
    sensorInfo.timestamp = frame->timestamp;
    mSetting->setSENSORTag(sensorInfo);
    timestamp = sensorInfo.timestamp;

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }
    property_get("persist.vendor.cam.debug.mode", debug_value, "non-debug");

    if (picChannel == NULL || encInfo->outPtr == NULL) {
        HAL_LOGE("picChannel=%p, encInfo->outPtr=%p", picChannel,
                 encInfo->outPtr);
        goto exit;
    }

    picChannel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &pic_stream);
    if (pic_stream == NULL) {
        HAL_LOGE("pic_stream=%p", pic_stream);
        goto exit;
    }

    ret = pic_stream->getQBuffFirstVir(&pic_addr_vir);
    if (ret || pic_addr_vir == 0x0) {
        HAL_LOGW("getQBuffFirstVir failed, ret=%d, pic_addr_vir=%ld", ret,
                 pic_addr_vir);
        if (mIsIspToolMode == 1 && regularChannel) {
            int64_t timestamp1 = systemTime();
            regularChannel->channelClearAllQBuff(timestamp1,
                                                 CAMERA_STREAM_TYPE_PREVIEW);
            regularChannel->channelClearAllQBuff(timestamp1,
                                                 CAMERA_STREAM_TYPE_VIDEO);
            regularChannel->channelClearAllQBuff(timestamp1,
                                                 CAMERA_STREAM_TYPE_CALLBACK);
        }
        goto exit;
    }

    HAL_LOGV("pic_addr_vir = 0x%lx", pic_addr_vir);
    memcpy((char *)pic_addr_vir, (char *)(encInfo->outPtr), encInfo->size);

    pic_stream->getQBuffFirstNum(&frame_num);
    pic_stream->getHeapSize(&heap_size);
    pic_stream->getQBufHandleForNum(frame_num, &jpeg_buff_handle);
    if (jpeg_buff_handle == NULL) {
        HAL_LOGE("failed to get jpeg buffer handle");
        goto exit;
    }

    maxJpegSize = ADP_WIDTH(*jpeg_buff_handle);
    if ((uint32_t)maxJpegSize > heap_size) {
        maxJpegSize = heap_size;
    }

    property_get("ro.debuggable", value, "0");
    if (!strcmp(value, "1")) {
        // add isp debug info for userdebug version
        ret = mHalOem->ops->camera_get_isp_info(mCameraHandle, &ispInfoAddr,
                                                &ispInfoSize);
        if (ret == 0 && ispInfoSize > 0) {
            HAL_LOGV("ispInfoSize=%d, encInfo->size=%d, maxJpegSize=%d",
                     ispInfoSize, encInfo->size, maxJpegSize);
            if (encInfo->size + ispInfoSize + sizeof(camera3_jpeg_blob) <
                (uint32_t)maxJpegSize) {
                memcpy(((char *)pic_addr_vir + encInfo->size),
                       (char *)ispInfoAddr, ispInfoSize);
            } else {
                HAL_LOGW("jpeg size is not big enough for ispdebug info");
                ispInfoSize = 0;
            }
        }

        // dump jpeg file
        if ((mCaptureMode == CAMERA_ISP_TUNING_MODE) ||
            (!strcmp(debug_value, "debug")) || is_raw_capture) {
            mHalOem->ops->dump_jpeg_file((void *)pic_addr_vir,
                                         encInfo->size + ispInfoSize,
                                         mCaptureWidth, mCaptureHeight);
        }
    }

    jpegBlob = (camera3_jpeg_blob *)((char *)pic_addr_vir +
                                     (maxJpegSize - sizeof(camera3_jpeg_blob)));
    jpegBlob->jpeg_size = encInfo->size + ispInfoSize;
    jpegBlob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;

    picChannel->channelCbRoutine(frame_num, timestamp,
                                 CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);

    if (mSprdReprocessing) {
        SprdCamera3RegularChannel *channel =
            reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        HAL_LOGI("jpeg encode done, reprocessing end");
        setCaptureReprocessMode(false, mCallbackWidth, mCallbackHeight);
        channel->releaseInputBuff();
    }

    if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
        mTakePictureMode == SNAPSHOT_DEFAULT_MODE) {
        SprdCamera3RegularChannel *regularChannel =
            reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (regularChannel) {
            regularChannel->channelClearInvalidQBuff(
                frame_num, timestamp, CAMERA_STREAM_TYPE_PREVIEW);
            regularChannel->channelClearInvalidQBuff(frame_num, timestamp,
                                                     CAMERA_STREAM_TYPE_VIDEO);
            regularChannel->channelClearInvalidQBuff(
                frame_num, timestamp, CAMERA_STREAM_TYPE_CALLBACK);
        }
    }

    if (encInfo->need_free) {
        if (!iSZslMode()) {
            deinitCapture(mIsPreAllocCapMem);
        }
    }

    if (getMultiCameraMode() == MODE_BLUR ||
        getMultiCameraMode() == MODE_BOKEH ||
        mSprdAppmodeId == CAMERA_MODE_3DNR_PHOTO) {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_4);
    } else if (mRecordingMode == true) {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_2);
    } else if (mSprdAppmodeId == CAMERA_MODE_CONTINUE) {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
    } else {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_1);
    }

exit:
    HAL_LOGD("X");
}

void SprdCamera3OEMIf::receiveJpegPictureError(void) {
    print_time();
    Mutex::Autolock cbLock(&mCaptureCbLock);
    if (!checkPreviewStateForCapture()) {
        HAL_LOGE("drop current jpegPictureError msg");
        return;
    }

    int index = 0;

    HAL_LOGD("JPEG callback was cancelled--not delivering image.");

    print_time();
}

void SprdCamera3OEMIf::receiveCameraExitError(void) {
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    Mutex::Autolock cbCaptureLock(&mCaptureCbLock);

    if (!checkPreviewStateForCapture()) {
        HAL_LOGE("drop current cameraExit msg");
        return;
    }

    HAL_LOGE("HandleErrorState:don't enable error msg!");
}

void SprdCamera3OEMIf::receiveTakePictureError(void) {
    Mutex::Autolock cbLock(&mCaptureCbLock);

    if (!checkPreviewStateForCapture()) {
        HAL_LOGE("drop current takePictureError msg");
        return;
    }

    HAL_LOGE("camera cb: invalid state %s for taking a picture!",
             getCameraStateStr(getCaptureState()));
}

/*transite from 'from' state to 'to' state and signal the waitting thread. if
 * the current*/
/*state is not 'from', transite to SPRD_ERROR state should be called from the
 * callback*/
SprdCamera3OEMIf::Sprd_camera_state
SprdCamera3OEMIf::transitionState(SprdCamera3OEMIf::Sprd_camera_state from,
                                  SprdCamera3OEMIf::Sprd_camera_state to,
                                  SprdCamera3OEMIf::state_owner owner,
                                  bool lock) {
    volatile SprdCamera3OEMIf::Sprd_camera_state *which_ptr = NULL;

    if (lock)
        mStateLock.lock();

    HAL_LOGD("owner = %d, lock = %d", owner, lock);
    switch (owner) {
    case STATE_CAMERA:
        which_ptr = &mCameraState.camera_state;
        break;

    case STATE_PREVIEW:
        which_ptr = &mCameraState.preview_state;
        break;

    case STATE_CAPTURE:
        which_ptr = &mCameraState.capture_state;
        break;

    case STATE_FOCUS:
        which_ptr = &mCameraState.focus_state;
        break;

    default:
        HAL_LOGD("changeState: error owner");
        break;
    }

    if (NULL != which_ptr) {
        if (from != *which_ptr) {
            to = SPRD_ERROR;
        }

        HAL_LOGD("changeState: %s --> %s", getCameraStateStr(from),
                 getCameraStateStr(to));

        if (*which_ptr != to) {
            *which_ptr = to;
            // mStateWait.signal();
            mStateWait.broadcast();
        }
    }

    if (lock)
        mStateLock.unlock();

    return to;
}

void SprdCamera3OEMIf::HandleStartPreview(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGV("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getPreviewState()));
    cam_ion_buffer_t *ionBuf = NULL;

    switch (cb) {
    case CAMERA_EXIT_CB_PREPARE:
        if (isPreAllocCapMem() && mSprdZslEnabled == 1) {
            pre_alloc_cap_mem_thread_init((void *)this);
        }
        break;

    case CAMERA_EVT_CB_INVALIDATE_CACHE:
        ionBuf = (cam_ion_buffer_t *)parm4;
        if (ionBuf)
            invalidateCache(ionBuf->fd, ionBuf->addr_vir, ionBuf->addr_phy,
                            ionBuf->size);
        break;

    case CAMERA_RSP_CB_SUCCESS:
        if (mIsStoppingPreview)
            HAL_LOGW("when is stopping preview, will change previw status");
        setCameraState(SPRD_PREVIEW_IN_PROGRESS, STATE_PREVIEW);
        break;

    case CAMERA_EVT_CB_FRAME:
        HAL_LOGV("CAMERA_EVT_CB_FRAME");
        switch (getPreviewState()) {
        case SPRD_PREVIEW_IN_PROGRESS:
            receivePreviewFrame((struct camera_frame_type *)parm4);
            break;

        case SPRD_INTERNAL_PREVIEW_STOPPING:
            HAL_LOGD("discarding preview frame while stopping preview");
            break;

        default:
            HAL_LOGW("invalid state");
            break;
        }
        break;

    case CAMERA_EVT_CB_RAW_FRAME:
        receiveRawFrame((struct camera_frame_type *)parm4);
        break;

    case CAMERA_EVT_CB_FD:
        HAL_LOGV("CAMERA_EVT_CB_FD");
        if (isPreviewing()) {
            receivePreviewFDFrame((struct camera_frame_type *)parm4);
        }
        break;

    case CAMERA_EXIT_CB_FAILED:
        HAL_LOGE("SprdCamera3OEMIf::camera_cb: @CAMERA_EXIT_CB_FAILURE(%p) in "
                 "state %s.",
                 parm4, getCameraStateStr(getPreviewState()));
        transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
        receiveCameraExitError();
        break;

    case CAMERA_EVT_CB_FLUSH:
        ionBuf = (cam_ion_buffer_t *)parm4;
        if (ionBuf) {
            flushIonBuffer(ionBuf->fd, ionBuf->addr_vir, ionBuf->addr_phy,
                           ionBuf->size);
        }
        break;

    case CAMERA_EVT_CB_RESUME:
        if (isPreviewing() && iSZslMode()) {
            setZslBuffers();
            mZslChannelStatus = 1;
        }
        break;

    default:
        transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
        HAL_LOGE("unexpected cb %d for CAMERA_FUNC_START_PREVIEW.", cb);
        break;
    }

    HAL_LOGV("out, state = %s", getCameraStateStr(getPreviewState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleStopPreview(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    Sprd_camera_state tmpPrevState = SPRD_IDLE;

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_INACTIVE;
    controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_INACTIVE;
    mSetting->setAeCONTROLTag(&controlInfo);
    mSetting->setAwbCONTROLTag(&controlInfo);
    HAL_LOGD("state = %s", getCameraStateStr(getPreviewState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleTakePicture(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCaptureState()));
    bool encode_location = true;
    camera_position_type pt = {0, 0, 0, 0, NULL};
    SPRD_DEF_Tag sprddefInfo; /**add for 3d calibration*/
    memset(&sprddefInfo, 0, sizeof(SPRD_DEF_Tag));
    cam_ion_buffer_t *ionBuf = NULL;
    cmr_int i = 0;

    switch (cb) {
    case CAMERA_EXIT_CB_PREPARE:
        prepareForPostProcess();
        break;
    case CAMERA_EVT_CB_FLUSH:
        ionBuf = (cam_ion_buffer_t *)parm4;
        if (ionBuf)
            flushIonBuffer(ionBuf->fd, ionBuf->addr_vir, ionBuf->addr_phy,
                           ionBuf->size);
        break;
    case CAMERA_EVT_CB_INVALIDATE_CACHE: {
        ionBuf = (cam_ion_buffer_t *)parm4;
        if (ionBuf)
            invalidateCache(ionBuf->fd, ionBuf->addr_vir, ionBuf->addr_phy,
                            ionBuf->size);
        break;
    }
    case CAMERA_RSP_CB_SUCCESS: {
        HAL_LOGV("CAMERA_RSP_CB_SUCCESS");

        Sprd_camera_state tmpCapState = SPRD_INIT;
        tmpCapState = getCaptureState();
        if (SPRD_WAITING_RAW == tmpCapState) {
            HAL_LOGD("CAMERA_RSP_CB_SUCCESS has been called before, skip it");
        } else if (tmpCapState != SPRD_INTERNAL_CAPTURE_STOPPING) {
            transitionState(SPRD_INTERNAL_RAW_REQUESTED, SPRD_WAITING_RAW,
                            STATE_CAPTURE);
        }
        break;
    }
    case CAMERA_EVT_CB_CAPTURE_FRAME_DONE: {
        HAL_LOGV("CAMERA_EVT_CB_CAPTURE_FRAME_DONE");
        JPEG_Tag jpegInfo;
        mSetting->getJPEGTag(&jpegInfo);
        if (jpeg_gps_location) {
            camera_position_type pt = {0, 0, 0, 0, NULL};
            pt.altitude = jpegInfo.gps_coordinates[2];
            pt.latitude = jpegInfo.gps_coordinates[0];
            pt.longitude = jpegInfo.gps_coordinates[1];
            memcpy(mGps_processing_method, jpegInfo.gps_processing_method,
                   sizeof(mGps_processing_method));
            pt.process_method =
                reinterpret_cast<const char *>(&mGps_processing_method[0]);
            pt.timestamp = jpegInfo.gps_timestamp;
            HAL_LOGV("gps pt.latitude = %f, pt.altitude = %f, pt.longitude = "
                     "%f, pt.process_method = %s, jpegInfo.gps_timestamp = "
                     "%" PRId64,
                     pt.latitude, pt.altitude, pt.longitude, pt.process_method,
                     jpegInfo.gps_timestamp);

            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_POSITION,
                     (cmr_uint)&pt);

            jpeg_gps_location = false;
        }
        break;
    }
    case CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE: {
        exitFromPostProcess();
        break;
    }
    case CAMERA_EVT_CB_SNAPSHOT_DONE: {
        float aperture = 0;
        struct exif_spec_pic_taking_cond_tag exif_pic_info;
        memset(&exif_pic_info, 0, sizeof(struct exif_spec_pic_taking_cond_tag));
        LENS_Tag lensInfo;
        mHalOem->ops->camera_get_sensor_result_exif_info(mCameraHandle,
                                                         &exif_pic_info);
        if (exif_pic_info.ApertureValue.denominator)
            aperture = (float)exif_pic_info.ApertureValue.numerator /
                       (float)exif_pic_info.ApertureValue.denominator;
        mSetting->getLENSTag(&lensInfo);
        lensInfo.aperture = aperture;
        mSetting->setLENSTag(lensInfo);
        if (checkPreviewStateForCapture() &&
            (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
             mTakePictureMode == SNAPSHOT_DEFAULT_MODE ||
             mTakePictureMode == SNAPSHOT_ZSL_MODE)) {
            receiveRawPicture((struct camera_frame_type *)parm4);
        } else {
            HAL_LOGW("drop current rawPicture");
        }
        break;
    }

    case CAMERA_EXIT_CB_DONE: {
        if (SPRD_WAITING_RAW == getCaptureState()) {
            /**modified for 3d calibration&3d capture return yuv buffer finished
             * begin */
            HAL_LOGD("mSprdYuvCallBack:%d, mSprd3dCalibrationEnabled:%d, "
                     "mTakePictureMode:%d",
                     mSprdYuvCallBack, mSprd3dCalibrationEnabled,
                     mTakePictureMode);
            if ((mSprdYuvCallBack || mSprd3dCalibrationEnabled) &&
                mTakePictureMode == SNAPSHOT_ZSL_MODE) {
                transitionState(SPRD_WAITING_RAW, SPRD_IDLE, STATE_CAPTURE);
                mSprdYuvCallBack = false;
            } else {
                transitionState(SPRD_WAITING_RAW, SPRD_WAITING_JPEG,
                                STATE_CAPTURE);
            }
            /**modified for 3d calibration return yuv buffer finished end */
        }
        break;
    }

    case CAMERA_EXIT_CB_FAILED: {
        HAL_LOGE("SprdCamera3OEMIf::camera_cb: @CAMERA_EXIT_CB_FAILURE(%p) in "
                 "state %s.",
                 parm4, getCameraStateStr(getCaptureState()));
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveCameraExitError();
        // camera_set_capture_trace(0);
        break;
    }

    case CAMERA_EVT_CB_ZSL_FRM: {
        HAL_LOGV("CAMERA_EVT_CB_HAL2_ZSL_NEW_FRM");
        break;
    }
    case CAMERA_EVT_CB_RETURN_ZSL_BUF: {
        if (isPreviewing() && iSZslMode() &&
            (mSprd3dCalibrationEnabled || mSprdYuvCallBack ||
             mMultiCameraMode == MODE_BLUR || mMultiCameraMode == MODE_BOKEH)) {
            cmr_u32 buf_id = 0;
            struct camera_frame_type *zsl_frame = NULL;
            zsl_frame = (struct camera_frame_type *)parm4;
            if (mIsUnpopped) {
                mIsUnpopped = false;
            }
            if (zsl_frame->fd <= 0) {
                HAL_LOGW("zsl lost a buffer, this should not happen");
                break;
            }
            HAL_LOGD("zsl_frame->fd=0x%x", zsl_frame->fd);
            buf_id = getZslBufferIDForFd(zsl_frame->fd);
            if (buf_id != 0xFFFFFFFF) {
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, mZslHeapArray[buf_id]->phys_addr,
                    (cmr_uint)mZslHeapArray[buf_id]->data,
                    mZslHeapArray[buf_id]->fd);
            }
        }
        break;
    }
    case CAMERA_EVT_CB_RETURN_SW_ALGORITHM_ZSL_BUF: {
        for (i = 0; i < (cmr_int)mZslNum; i++) {
            mHalOem->ops->camera_set_zsl_buffer(
                mCameraHandle, mZslHeapArray[i]->phys_addr,
                (cmr_uint)mZslHeapArray[i]->data, mZslHeapArray[i]->fd);
        }
        break;
    }
    default: {
        HAL_LOGE("unkown cb = %d", cb);
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveTakePictureError();
        // camera_set_capture_trace(0);
        break;
    }
    }

    HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleCancelPicture(enum camera_cb_type cb,
                                           void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCaptureState()));

    if (SPRD_INTERNAL_CAPTURE_STOPPING != getCaptureState()) {
        HAL_LOGD("HandleCancelPicture don't handle");
        return;
    }
    setCameraState(SPRD_IDLE, STATE_CAPTURE);

    HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleEncode(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCaptureState()));

    switch (cb) {
    case CAMERA_RSP_CB_SUCCESS:
        break;

    case CAMERA_EXIT_CB_DONE:
        HAL_LOGD("CAMERA_EXIT_CB_DONE");
        receiveJpegPicture((struct camera_frame_type *)parm4);
        setCameraState(SPRD_IDLE, STATE_CAPTURE);
        break;

    case CAMERA_EXIT_CB_FAILED:
        HAL_LOGD("CAMERA_EXIT_CB_FAILED");
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveCameraExitError();
        break;

    case CAMERA_EVT_CB_RETURN_ZSL_BUF:
        if (isPreviewing() && iSZslMode()) {
            cmr_u32 buf_id;
            struct camera_frame_type *zsl_frame;
            zsl_frame = (struct camera_frame_type *)parm4;
            if (zsl_frame->fd <= 0) {
                HAL_LOGW("zsl lost a buffer, this should not happen");
                goto handle_encode_exit;
            }
            HAL_LOGD("zsl_frame->fd=0x%x", zsl_frame->fd);
            buf_id = getZslBufferIDForFd(zsl_frame->fd);
            if (buf_id != 0xFFFFFFFF) {
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, mZslHeapArray[buf_id]->phys_addr,
                    (cmr_uint)mZslHeapArray[buf_id]->data,
                    mZslHeapArray[buf_id]->fd);
            }
        }
        break;

    default:
        HAL_LOGD("unkown error = %d", cb);
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveJpegPictureError();
        break;
    }

handle_encode_exit:
    HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleFocus(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    struct cmr_focus_status *focus_status;
    int64_t timeStamp = 0;
    timeStamp = systemTime();

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getPreviewState()));

    setCameraState(SPRD_IDLE, STATE_FOCUS);

    switch (cb) {
    case CAMERA_RSP_CB_SUCCESS:
        HAL_LOGV("camera cb: autofocus has started.");
        break;

    case CAMERA_EXIT_CB_DONE:
        HAL_LOGV("camera cb: autofocus succeeded.");
        {
            controlInfo.af_state = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            mSetting->setAfCONTROLTag(&controlInfo);
            // channel->channelCbRoutine(0, timeStamp,
            // CAMERA_STREAM_TYPE_DEFAULT);
            if (controlInfo.af_mode ==
                ANDROID_CONTROL_AF_MODE_AUTO) // reset autofocus only in TouchAF
                                              // process
            {
                mIsAutoFocus = false;
            }
        }
        break;

    case CAMERA_EXIT_CB_ABORT:
    case CAMERA_EXIT_CB_FAILED: {
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
        mSetting->setAfCONTROLTag(&controlInfo);
        // channel->channelCbRoutine(0, timeStamp, CAMERA_STREAM_TYPE_DEFAULT);
        if (controlInfo.af_mode ==
            ANDROID_CONTROL_AF_MODE_AUTO) // reset autofocus only in TouchAF
                                          // process
        {
            mIsAutoFocus = false;
        }

    } break;

    case CAMERA_EVT_CB_FOCUS_MOVE:
        focus_status = (cmr_focus_status *)parm4;
        HAL_LOGV("parm4=%p autofocus=%d", parm4, mIsAutoFocus);
        if (!mIsAutoFocus) {
            if (focus_status->is_in_focus) {
                controlInfo.af_state = ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN;
            } else {
                controlInfo.af_state = ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED;
                mLastCafDoneTime = systemTime();
            }

            if (focus_status->af_focus_type == CAM_AF_FOCUS_CAF)
                mSetting->setAfCONTROLTag(&controlInfo);
        }
        break;

    default:
        HAL_LOGE("camera cb: unknown cb %d for CAMERA_FUNC_START_FOCUS!", cb);
        {
            controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
            mSetting->setAfCONTROLTag(&controlInfo);

            // channel->channelCbRoutine(0, timeStamp,
            // CAMERA_STREAM_TYPE_DEFAULT);
        }
        break;
    }

    HAL_LOGD("out, state = %s", getCameraStateStr(getFocusState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleAutoExposure(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    HAL_LOGV("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getPreviewState()));

    switch (cb) {
    case CAMERA_EVT_CB_AE_STAB_NOTIFY:
        if (controlInfo.ae_state != ANDROID_CONTROL_AE_STATE_LOCKED) {
            controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
            mSetting->setAeCONTROLTag(&controlInfo);
        }
        if (controlInfo.awb_state != ANDROID_CONTROL_AWB_STATE_LOCKED) {
            controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_CONVERGED;
            mSetting->setAwbCONTROLTag(&controlInfo);
        }
        HAL_LOGV("CAMERA_EVT_CB_AE_STAB_NOTIFY, ae_state = %d",
                 controlInfo.ae_state);
        break;
    case CAMERA_EVT_CB_AE_LOCK_NOTIFY:
        // controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_LOCKED;
        // mSetting->setAeCONTROLTag(&controlInfo);
        HAL_LOGI("CAMERA_EVT_CB_AE_LOCK_NOTIFY, ae_state = %d",
                 controlInfo.ae_state);
        break;
    case CAMERA_EVT_CB_AE_UNLOCK_NOTIFY:
        // controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
        // mSetting->setAeCONTROLTag(&controlInfo);
        HAL_LOGI("CAMERA_EVT_CB_AE_UNLOCK_NOTIFY, ae_state = %d",
                 controlInfo.ae_state);
        break;
    case CAMERA_EVT_CB_AE_FLASH_FIRED:
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        sprddefInfo.is_takepicture_with_flash = *(uint8_t *)parm4;
        if (sprddefInfo.sprd_3dnr_enabled) {
            sprddefInfo.is_takepicture_with_flash = 0;
        }
        mSetting->setSPRDDEFTag(sprddefInfo);
        HAL_LOGI("is_takepicture_with_flash = %d",
                 sprddefInfo.is_takepicture_with_flash);
        break;
    case CAMERA_EVT_CB_HDR_SCENE:
        SPRD_DEF_Tag sprdInfo;
        mSetting->getSPRDDEFTag(&sprdInfo);
        sprdInfo.sprd_is_hdr_scene = *(uint8_t *)parm4;
        mSetting->setSPRDDEFTag(sprdInfo);
        HAL_LOGI("sprd_is_hdr_scene = %d", sprdInfo.sprd_is_hdr_scene);
        break;
    case CAMERA_EVT_CB_AI_SCENE:
        SPRD_DEF_Tag sprdAIInfo;
        mSetting->getSPRDDEFTag(&sprdAIInfo);
        sprdAIInfo.sprd_ai_scene_type_current = *(cmr_u8 *)parm4;
        mSetting->setSPRDDEFTag(sprdAIInfo);
        HAL_LOGD("sprdInfo.sprd_ai_scene_type_current :%u",
                 sprdAIInfo.sprd_ai_scene_type_current);
        break;
    default:
        break;
    }

    HAL_LOGV("X");
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleStartCamera(enum camera_cb_type cb, void *parm4) {
    HAL_LOGD("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));

    transitionState(SPRD_INIT, SPRD_IDLE, STATE_CAMERA);

    HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::HandleStopCamera(enum camera_cb_type cb, void *parm4) {
    HAL_LOGD("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));

    transitionState(SPRD_INTERNAL_STOPPING, SPRD_INIT, STATE_CAMERA);

    HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::camera_cb(enum camera_cb_type cb,
                                 const void *client_data,
                                 enum camera_func_type func, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)client_data;
    HAL_LOGV("E");
    HAL_LOGV("cb = %d func = %d parm4 = %p", cb, func, parm4);
    switch (func) {
    case CAMERA_FUNC_START_PREVIEW:
        obj->HandleStartPreview(cb, parm4);
        break;

    case CAMERA_FUNC_STOP_PREVIEW:
        obj->HandleStopPreview(cb, parm4);
        break;

    case CAMERA_FUNC_RELEASE_PICTURE:
        obj->HandleCancelPicture(cb, parm4);
        break;

    case CAMERA_FUNC_TAKE_PICTURE:
        obj->HandleTakePicture(cb, parm4);
        break;

    case CAMERA_FUNC_ENCODE_PICTURE:
        obj->HandleEncode(cb, parm4);
        break;

    case CAMERA_FUNC_START_FOCUS:
        obj->HandleFocus(cb, parm4);
        break;

    case CAMERA_FUNC_AE_STATE_CALLBACK:
        obj->HandleAutoExposure(cb, parm4);
        break;

    case CAMERA_FUNC_START:
        obj->HandleStartCamera(cb, parm4);
        break;

    case CAMERA_FUNC_STOP:
        obj->HandleStopCamera(cb, parm4);
        break;

    default:
        HAL_LOGE("Unknown camera-callback status %d", cb);
        break;
    }

    ATRACE_END();
    HAL_LOGV("X");
}

int SprdCamera3OEMIf::flushIonBuffer(int buffer_fd, void *v_addr, void *p_addr,
                                     size_t size) {
    ATRACE_CALL();

    HAL_LOGD("E");

    int ret = 0;
    ret = MemIon::Sync_ion_buffer(buffer_fd);
    if (ret) {
        HAL_LOGE("Sync_ion_buffer failed, ret=%d", ret);
        goto exit;
    }

    HAL_LOGD("X");

exit:
    return ret;
}

int SprdCamera3OEMIf::invalidateCache(int buffer_fd, void *v_addr, void *p_addr,
                                      size_t size) {
    ATRACE_CALL();

    HAL_LOGD("E");

    int ret = 0;
    ret = MemIon::Invalid_ion_buffer(buffer_fd);
    if (ret) {
        HAL_LOGE("Invalid_ion_buffer failed, ret=%d", ret);
        goto exit;
    }

    HAL_LOGD("X");

exit:
    return ret;
}

int SprdCamera3OEMIf::openCamera() {
    ATRACE_CALL();

    char value[PROPERTY_VALUE_MAX];
    int ret = NO_ERROR;
    int is_raw_capture = 0;
    cmr_u16 picW, picH, snsW, snsH;
    int i = 0;
    char file_name[128];
    struct exif_info exif_info = {0, 0};
    LENS_Tag lensInfo;
    SPRD_DEF_Tag sprddefInfo;
    struct sensor_mode_info mode_info[SENSOR_MODE_MAX];

    HAL_LOGI(":hal3: E camId=%d", mCameraId);

    if (NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    GET_START_TIME;
    memset(&sprddefInfo, 0, sizeof(SPRD_DEF_Tag));
    memset(mode_info, 0, sizeof(struct sensor_mode_info) * SENSOR_MODE_MAX);

    mSetting->getLargestPictureSize(mCameraId, &picW, &picH);
    mSetting->getLargestSensorSize(mCameraId, &snsW, &snsH);
    if (picW * picH > snsW * snsH) {
        mLargestPictureWidth = picW;
        mLargestPictureHeight = picH;
    } else {
        mLargestPictureWidth = snsW;
        mLargestPictureHeight = snsH;
    }
    mHalOem->ops->camera_set_largest_picture_size(
        mCameraId, mLargestPictureWidth, mLargestPictureHeight);

    if (isCameraInit()) {
        HAL_LOGE("camera hardware has been started already");
        goto exit;
    }

    HAL_LOGI(":hal3: camera_init");
    ret = mHalOem->ops->camera_init(mCameraId, camera_cb, this, 0,
                                    &mCameraHandle, (void *)Callback_Malloc,
                                    (void *)Callback_Free);
    if (ret) {
        setCameraState(SPRD_INIT);
        HAL_LOGE("camera_init failed");
        goto exit;
    }
    setCameraState(SPRD_IDLE);

    mHalOem->ops->camera_ioctrl(
        mCameraHandle, CAMERA_IOCTRL_GET_GRAB_CAPABILITY, &grab_capability);

    // TBD: these should be move to sprdcamera3settings.cpp
    // LensInfo
    mHalOem->ops->camera_get_sensor_exif_info(mCameraHandle, &exif_info);
    mSetting->getLENSTag(&lensInfo);
    lensInfo.aperture = exif_info.aperture;
    mSetting->setLENSTag(lensInfo);
    HAL_LOGV("lensInfo.aperture %f", lensInfo.aperture);

    // dual otp
    if ((mMultiCameraMode == MODE_BOKEH ||
         mMultiCameraMode == MODE_3D_CALIBRATION ||
         mMultiCameraMode == MODE_DUAL_FACEID_UNLOCK) &&
        mCameraId < 2) {
        OTP_Tag otpInfo;
        memset(&otpInfo, 0, sizeof(OTP_Tag));
        mSetting->getOTPTag(&otpInfo);

        struct sensor_otp_cust_info otp_info;
        memset(&otp_info, 0, sizeof(struct sensor_otp_cust_info));
        mHalOem->ops->camera_get_sensor_otp_info(mCameraHandle, &otp_info);
        if (otp_info.total_otp.data_ptr != NULL &&
            otp_info.dual_otp.dual_flag) {
            memcpy(otpInfo.otp_data, (char *)otp_info.dual_otp.data_3d.data_ptr,
                   otp_info.dual_otp.data_3d.size);
            otpInfo.otp_type = otp_info.dual_otp.data_3d.dualcam_cali_lib_type;
            otpInfo.dual_otp_flag = 1;
        } else {
            otpInfo.dual_otp_flag = 0;
            HAL_LOGD("dual_otp_flag %d", otpInfo.dual_otp_flag);
        }
        {
            bzero(file_name, sizeof(file_name));
            strcpy(file_name, CAMERA_DUMP_PATH);
            strcat(file_name, "calibration.txt");
            FILE *fid = fopen(file_name, "rb");
            if (NULL == fid) {
                HAL_LOGV("calibration read failed!");
            } else {
                int read_byte = 0;
                cmr_u8 *otp_data = (cmr_u8 *)otpInfo.otp_data;
                while (!feof(fid)) {
                    fscanf(fid, "%d\n", otp_data);
                    otp_data += 4;
                    read_byte += 4;
                }
                fclose(fid);
                HAL_LOGD("calibration read_bytes=%d ", read_byte);
                if (read_byte) {
                    otp_info.dual_otp.data_3d.size = read_byte;
                    otpInfo.otp_type = 0; // OTP_CALI_SPRD;
                    otpInfo.dual_otp_flag = 1;
                }
            }
        }
        if (otp_info.dual_otp.data_3d.size > 0) {
            save_file("dualcamera.bin", otpInfo.otp_data,
                      otp_info.dual_otp.data_3d.size);
            mSetting->setOTPTag(&otpInfo, otp_info.dual_otp.data_3d.size,
                                otpInfo.otp_type);
        }
    }

    // Add for 3d calibration get max sensor size begin
    mSetting->getSPRDDEFTag(&sprddefInfo);
    mHalOem->ops->camera_get_sensor_info_for_raw(mCameraHandle, mode_info);
    for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
        HAL_LOGV("trim w=%d, h=%d", mode_info[i].trim_width,
                 mode_info[i].trim_height);
        if (mode_info[i].trim_width * mode_info[i].trim_height >=
            sprddefInfo.sprd_3dcalibration_cap_size[0] *
                sprddefInfo.sprd_3dcalibration_cap_size[1]) {
            sprddefInfo.sprd_3dcalibration_cap_size[0] =
                mode_info[i].trim_width;
            sprddefInfo.sprd_3dcalibration_cap_size[1] =
                mode_info[i].trim_height;
        }
    }
    HAL_LOGI("sprd_3dcalibration_cap_size w=%d, h=%d",
             sprddefInfo.sprd_3dcalibration_cap_size[0],
             sprddefInfo.sprd_3dcalibration_cap_size[1]);
    mSetting->setSPRDDEFTag(sprddefInfo);

    // TBD: camera id 2 open alone, need iommu
    if ((getMultiCameraMode() == MODE_BLUR ||
         getMultiCameraMode() == MODE_SELF_SHOT ||
         getMultiCameraMode() == MODE_PAGE_TURN) &&
        mCameraId >= 2) {
        HAL_LOGD("dont need ion memory for blur");
    } else {
        mIommuEnabled = IommuIsEnabled();
    }
    HAL_LOGI("mIommuEnabled=%d", mIommuEnabled);

    ZSLMode_monitor_thread_init((void *)this);

#ifdef CONFIG_CAMERA_GYRO
    gyro_monitor_thread_init((void *)this);
#endif

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        is_raw_capture = 1;
    }

    property_get("persist.vendor.cam.isptool.mode.enable", value, "false");
    if (!strcmp(value, "true")) {
        mIsIspToolMode = 1;
        HAL_LOGI("mIsIspToolMode=%d", mIsIspToolMode);
    }

exit:
    HAL_LOGI(":hal3: X");
    return ret;
}

int SprdCamera3OEMIf::handleCbData(hal3_trans_info_t &result_info,
                                   void *userdata) {
    cam_stream_type_t type = CAM_STREAM_TYPE_DEFAULT;
    SprdCamera3Channel *channel = (SprdCamera3Channel *)userdata;

    HAL_LOGD("S data=%p", channel);
    for (List<hal3_trans_info_t>::iterator i = mCbInfoList.begin();
         i != mCbInfoList.end(); i++) {
        if (channel == (SprdCamera3Channel *)i->user_data) {
            type = i->stream_type;
            HAL_LOGD("type=%d", type);
            break;
        }
    }
    // channel->channelCbRoutine(result_info.frame.index, result_info.timestamp,
    // NULL, type);
    HAL_LOGD("X");
    return 0;
}

void SprdCamera3OEMIf::setCamPreformaceScene(
    sys_performance_camera_scene camera_scene) {

    if (mSysPerformace) {
        mSysPerformace->setCamPreformaceScene(camera_scene);
    }
}

int SprdCamera3OEMIf::setCameraConvertCropRegion(void) {
    float zoomWidth, zoomHeight, zoomRatio = 1.0f;
    float prevAspectRatio, capAspectRatio, videoAspectRatio;
    float sensorAspectRatio, outputAspectRatio;
    uint16_t sensorOrgW = 0, sensorOrgH = 0;
    SCALER_Tag scaleInfo;
    struct img_rect cropRegion;
    int ret = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mSetting->getSCALERTag(&scaleInfo);
    cropRegion.start_x = scaleInfo.crop_region[0];
    cropRegion.start_y = scaleInfo.crop_region[1];
    cropRegion.width = scaleInfo.crop_region[2];
    cropRegion.height = scaleInfo.crop_region[3];
    HAL_LOGD("crop start_x=%d start_y=%d width=%d height=%d",
             cropRegion.start_x, cropRegion.start_y, cropRegion.width,
             cropRegion.height);

    mSetting->getLargestPictureSize(mCameraId, &sensorOrgW, &sensorOrgH);

    sensorAspectRatio = static_cast<float>(sensorOrgW) / sensorOrgH;

    if (mPreviewWidth != 0 && mPreviewHeight != 0) {
        prevAspectRatio = static_cast<float>(mPreviewWidth) / mPreviewHeight;
    } else {
        prevAspectRatio = sensorAspectRatio;
    }

    if (mVideoWidth != 0 && mVideoHeight != 0) {
        videoAspectRatio = static_cast<float>(mVideoWidth) / mVideoHeight;
    } else {
        videoAspectRatio = prevAspectRatio;
    }

    if (mSetCapRatioFlag == true && mCaptureWidth != 0 && mCaptureHeight != 0) {
        capAspectRatio = static_cast<float>(mCaptureWidth) / mCaptureHeight;
    } else if (mCallbackWidth != 0 && mCallbackHeight != 0) {
        capAspectRatio = static_cast<float>(mCallbackWidth) / mCallbackHeight;
    } else if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        capAspectRatio = static_cast<float>(mCaptureWidth) / mCaptureHeight;
    } else {
        capAspectRatio = prevAspectRatio;
    }

    if (cropRegion.width > 0 && cropRegion.height > 0) {
        zoomWidth = static_cast<float>(cropRegion.width);
        zoomHeight = static_cast<float>(cropRegion.height);
    } else {
        zoomWidth = static_cast<float>(sensorOrgW);
        zoomHeight = static_cast<float>(sensorOrgH);
    }

    if (mPreviewWidth != 0 && mPreviewHeight != 0) {
        if (prevAspectRatio >= sensorAspectRatio) {
            zoomRatio = static_cast<float>(sensorOrgW) / zoomWidth;
        } else {
            zoomRatio = static_cast<float>(sensorOrgH) / zoomHeight;
        }
    } else if (mCaptureWidth != 0 && mCaptureHeight != 0) {
        if (capAspectRatio >= sensorAspectRatio) {
            zoomRatio = static_cast<float>(sensorOrgW) / zoomWidth;
        } else {
            zoomRatio = static_cast<float>(sensorOrgH) / zoomHeight;
        }
    } else {
        zoomRatio = static_cast<float>(sensorOrgW) / zoomWidth;
    }

    if (zoomRatio < MIN_DIGITAL_ZOOM_RATIO)
        zoomRatio = MIN_DIGITAL_ZOOM_RATIO;
    if (zoomRatio > MAX_DIGITAL_ZOOM_RATIO)
        zoomRatio = MAX_DIGITAL_ZOOM_RATIO;

    mZoomInfo.mode = ZOOM_INFO;
    mZoomInfo.zoom_info.zoom_ratio = zoomRatio;
    mZoomInfo.zoom_info.prev_aspect_ratio = prevAspectRatio;
    mZoomInfo.zoom_info.video_aspect_ratio = videoAspectRatio;
    mZoomInfo.zoom_info.capture_aspect_ratio = capAspectRatio;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM, (cmr_uint)&mZoomInfo);

    HAL_LOGD(
        "camId=%d, zoomRatio=%f, outputAspectRatio: prev=%f, video=%f, cap=%f",
        mCameraId, zoomRatio, prevAspectRatio, videoAspectRatio,
        capAspectRatio);

    return ret;
}

int SprdCamera3OEMIf::CameraConvertCropRegion(uint32_t sensorWidth,
                                              uint32_t sensorHeight,
                                              struct img_rect *cropRegion) {
    float minOutputRatio;
    float zoomWidth, zoomHeight, zoomRatio;
    uint32_t i = 0;
    int ret = 0;
    uint32_t endX = 0, endY = 0, startXbak = 0, startYbak = 0;
    uint16_t sensorOrgW = 0, sensorOrgH = 0;
    cmr_uint SensorRotate = IMG_ANGLE_0;
#define ALIGN_ZOOM_CROP_BITS (~0x03)

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    HAL_LOGD("crop %d %d %d %d sens w/h %d %d.", cropRegion->start_x,
             cropRegion->start_y, cropRegion->width, cropRegion->height,
             sensorWidth, sensorHeight);

    mSetting->getLargestPictureSize(mCameraId, &sensorOrgW, &sensorOrgH);

    SensorRotate = mHalOem->ops->camera_get_preview_rot_angle(mCameraHandle);
    if (sensorWidth == sensorOrgW && sensorHeight == sensorOrgH &&
        SensorRotate == IMG_ANGLE_0) {
        HAL_LOGD("dont' need to convert.");
        return 0;
    }
    /*
    zoomWidth = (float)cropRegion->width;
    zoomHeight = (float)cropRegion->height;
    //get dstRatio and zoomRatio frm framework
    minOutputRatio = zoomWidth / zoomHeight;
    if (minOutputRatio > ((float)sensorOrgW / (float)sensorOrgH)) {
        zoomRatio = (float)sensorOrgW / zoomWidth;
    } else {
        zoomRatio = (float)sensorOrgH / zoomHeight;
    }
    if(IsRotate) {
        minOutputRatio = 1 / minOutputRatio;
    }
    if (minOutputRatio > ((float)sensorWidth / (float)sensorHeight)) {
        zoomWidth = (float)sensorWidth / zoomRatio;
        zoomHeight = zoomWidth / minOutputRatio;
    } else {
        zoomHeight = (float)sensorHeight / zoomRatio;
        zoomWidth = zoomHeight * minOutputRatio;
    }
    cropRegion->start_x = ((uint32_t)(sensorWidth - zoomWidth) >> 1) &
    ALIGN_ZOOM_CROP_BITS;
    cropRegion->start_y = ((uint32_t)(sensorHeight - zoomHeight) >> 1) &
    ALIGN_ZOOM_CROP_BITS;
    cropRegion->width = ((uint32_t)zoomWidth) & ALIGN_ZOOM_CROP_BITS;
    cropRegion->height = ((uint32_t)zoomHeight) & ALIGN_ZOOM_CROP_BITS;*/

    zoomWidth = (float)sensorWidth / (float)sensorOrgW;
    zoomHeight = (float)sensorHeight / (float)sensorOrgH;
    endX = cropRegion->start_x + cropRegion->width;
    endY = cropRegion->start_y + cropRegion->height;
    cropRegion->start_x = (cmr_u32)((float)cropRegion->start_x * zoomWidth);
    cropRegion->start_y = (cmr_u32)((float)cropRegion->start_y * zoomHeight);
    endX = (cmr_u32)((float)endX * zoomWidth);
    endY = (cmr_u32)((float)endY * zoomHeight);
    cropRegion->width = endX - cropRegion->start_x;
    cropRegion->height = endY - cropRegion->start_y;

    switch (SensorRotate) {
    case IMG_ANGLE_90:
        startYbak = cropRegion->start_y;
        cropRegion->start_y = cropRegion->start_x;
        cropRegion->start_x = sensorHeight - endY;
        cropRegion->width = (sensorHeight - startYbak) - cropRegion->start_x;
        cropRegion->height = endX - cropRegion->start_y;
        break;
    case IMG_ANGLE_180:
        startYbak = cropRegion->start_y;
        startXbak = cropRegion->start_x;
        cropRegion->start_x = sensorHeight - endX;
        cropRegion->start_y = sensorWidth - endY;
        cropRegion->width = (sensorHeight - startXbak) - cropRegion->start_x;
        cropRegion->height = (sensorWidth - startYbak) - cropRegion->start_y;
        break;
    case IMG_ANGLE_270:
        startYbak = cropRegion->start_y;
        startXbak = cropRegion->start_x;
        cropRegion->start_x = cropRegion->start_y;
        cropRegion->start_y = sensorHeight - endX;
        cropRegion->width = endY - cropRegion->start_x;
        cropRegion->height = (sensorHeight - startXbak) - cropRegion->start_y;
        break;
    }
    HAL_LOGD("Crop calculated (x=%d,y=%d,w=%d,h=%d rot=0x%lx)",
             cropRegion->start_x, cropRegion->start_y, cropRegion->width,
             cropRegion->height, SensorRotate);
    return ret;
}

int SprdCamera3OEMIf::SetCameraParaTag(cmr_int cameraParaTag) {
    int ret = 0;
    CONTROL_Tag controlInfo;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.raw.mode", value, "jpeg");

    HAL_LOGV("set camera para, tag is %ld", cameraParaTag);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mSetting->getCONTROLTag(&controlInfo);
    switch (cameraParaTag) {
    case ANDROID_CONTROL_SCENE_MODE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        if (1 == sprddefInfo.sprd_3dnr_enabled) {
            controlInfo.scene_mode = ANDROID_CONTROL_SCENE_MODE_NIGHT;
        }

        int8_t drvSceneMode = 0;
        mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode,
                                            &drvSceneMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SCENE_MODE, drvSceneMode);
    } break;

    case ANDROID_CONTROL_EFFECT_MODE: {
        int8_t effectMode = 0;
        mSetting->androidEffectModeToDrvMode(controlInfo.effect_mode,
                                             &effectMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EFFECT, effectMode);
    } break;

    case ANDROID_CONTROL_CAPTURE_INTENT:
        mCapIntent = controlInfo.capture_intent;
        break;

    case ANDROID_CONTROL_AWB_MODE: {
        int8_t drvAwbMode = 0;
        mSetting->androidAwbModeToDrvAwbMode(controlInfo.awb_mode, &drvAwbMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_WB, drvAwbMode);
        if (controlInfo.awb_mode != ANDROID_CONTROL_AWB_MODE_AUTO)
            controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_INACTIVE;
        else if (!controlInfo.awb_lock)
            controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_SEARCHING;
    } break;

    case ANDROID_CONTROL_AWB_LOCK: {
        uint8_t awb_lock;
        awb_lock = controlInfo.awb_lock;
        HAL_LOGV("ANDROID_CONTROL_AWB_LOCK awb_lock=%d", awb_lock);
        if (awb_lock && controlInfo.awb_mode == ANDROID_CONTROL_AWB_MODE_AUTO) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK,
                     awb_lock);
            controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_LOCKED;
            mSetting->setAwbCONTROLTag(&controlInfo);
        } else if (!awb_lock &&
                   controlInfo.awb_state == ANDROID_CONTROL_AWB_STATE_LOCKED) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK,
                     awb_lock);
            controlInfo.awb_state = ANDROID_CONTROL_AWB_STATE_SEARCHING;
            mSetting->setAwbCONTROLTag(&controlInfo);
        }
    } break;

    case ANDROID_SCALER_CROP_REGION:
        // SET_PARM(mHalOem, CAMERA_PARM_ZOOM_RECT, cameraParaValue);
        break;

    case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
        SPRD_DEF_Tag sprddefInfo;
        struct cmr_ae_compensation_param ae_compensation_param;
        mSetting->getSPRDDEFTag(&sprddefInfo);

        if (sprddefInfo.sprd_appmode_id == CAMERA_MODE_MANUAL) {
            // legacy sprd isp ae compensation manual mode, just for manual mode
            ae_compensation_param.ae_compensation_range[0] =
                LEGACY_SPRD_AE_COMPENSATION_RANGE_MIN;
            ae_compensation_param.ae_compensation_range[1] =
                LEGACY_SPRD_AE_COMPENSATION_RANGE_MAX;
            ae_compensation_param.ae_compensation_step_numerator =
                LEGACY_SPRD_AE_COMPENSATION_STEP_NUMERATOR;
            ae_compensation_param.ae_compensation_step_denominator =
                LEGACY_SPRD_AE_COMPENSATION_STEP_DEMINATOR;
            ae_compensation_param.ae_exposure_compensation =
                controlInfo.ae_exposure_compensation;
        } else {
            // standard implementation following android api
            ae_compensation_param.ae_compensation_range[0] =
                controlInfo.ae_compensation_range[0];
            ae_compensation_param.ae_compensation_range[1] =
                controlInfo.ae_compensation_range[1];
            ae_compensation_param.ae_compensation_step_numerator =
                controlInfo.ae_compensation_step.numerator;
            ae_compensation_param.ae_compensation_step_denominator =
                controlInfo.ae_compensation_step.denominator;
            ae_compensation_param.ae_exposure_compensation =
                controlInfo.ae_exposure_compensation;
        }

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXPOSURE_COMPENSATION,
                 (cmr_uint)&ae_compensation_param);
        break;
    case ANDROID_CONTROL_AF_TRIGGER:
        HAL_LOGV("AF_TRIGGER %d", controlInfo.af_trigger);
        if (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START) {
            struct img_rect zoom1 = {0, 0, 0, 0};
            struct img_rect zoom = {0, 0, 0, 0};
            struct cmr_focus_param focus_para;
            if (mCameraState.preview_state == SPRD_PREVIEW_IN_PROGRESS) {
                zoom.start_x = controlInfo.af_regions[0];
                zoom.start_y = controlInfo.af_regions[1];
                zoom.width = controlInfo.af_regions[2];
                zoom.height = controlInfo.af_regions[3];
                mHalOem->ops->camera_get_sensor_trim(mCameraHandle, &zoom1);
                if ((0 == zoom.start_x && 0 == zoom.start_y &&
                     0 == zoom.width && 0 == zoom.height) ||
                    !CameraConvertCropRegion(zoom1.width, zoom1.height,
                                             &zoom)) {
                    focus_para.zone[0].start_x = zoom.start_x;
                    focus_para.zone[0].start_y = zoom.start_y;
                    focus_para.zone[0].width = zoom.width;
                    focus_para.zone[0].height = zoom.height;
                    focus_para.zone_cnt = 1;
                    HAL_LOGI(
                        "after crop AF region is %d %d %d %d",
                        focus_para.zone[0].start_x, focus_para.zone[0].start_y,
                        focus_para.zone[0].width, focus_para.zone[0].height);
                    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCUS_RECT,
                             (cmr_uint)&focus_para);
                }
            }

            autoFocus();
        } else if (controlInfo.af_trigger ==
                   ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
            cancelAutoFocus();
        }
        break;
    case ANDROID_SPRD_SENSOR_ORIENTATION: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("orient=%d", sprddefInfo.sensor_orientation);
        if (1 == sprddefInfo.sensor_orientation) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
                     1);
        } else {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
                     0);
        }
    } break;
    case ANDROID_SPRD_SENSOR_ROTATION: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("rot=%d", sprddefInfo.sensor_rotation);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION,
                 sprddefInfo.sensor_rotation);
    } break;
    case ANDROID_SPRD_UCAM_SKIN_LEVEL: {
        SPRD_DEF_Tag sprddefInfo;
        struct beauty_info fb_param;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        fb_param.blemishLevel = sprddefInfo.perfect_skin_level[0];
        fb_param.smoothLevel = sprddefInfo.perfect_skin_level[1];
        fb_param.skinColor = sprddefInfo.perfect_skin_level[2];
        fb_param.skinLevel = sprddefInfo.perfect_skin_level[3];
        fb_param.brightLevel = sprddefInfo.perfect_skin_level[4];
        fb_param.lipColor = sprddefInfo.perfect_skin_level[5];
        fb_param.lipLevel = sprddefInfo.perfect_skin_level[6];
        fb_param.slimLevel = sprddefInfo.perfect_skin_level[7];
        fb_param.largeLevel = sprddefInfo.perfect_skin_level[8];
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PERFECT_SKIN_LEVEL,
                 (cmr_uint) & (fb_param));
        mFbOn = fb_param.blemishLevel || fb_param.smoothLevel ||
                fb_param.skinColor || fb_param.skinLevel ||
                fb_param.brightLevel || fb_param.lipColor ||
                fb_param.lipLevel || fb_param.slimLevel || fb_param.largeLevel;
    } break;
    case ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("flip_on_level = %d", sprddefInfo.flip_on);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLIP_ON,
                 sprddefInfo.flip_on);
    } break;

    case ANDROID_SPRD_EIS_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;

        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprd_eis_enabled = %d", sprddefInfo.sprd_eis_enabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_EIS_ENABLED,
                 sprddefInfo.sprd_eis_enabled);
    } break;

    case ANDROID_CONTROL_AF_MODE: {
        int8_t AfMode = 0;
        mSetting->androidAfModeToDrvAfMode(controlInfo.af_mode, &AfMode);
        if (!mIsAutoFocus) {
            if (mRecordingMode &&
                CAMERA_FOCUS_MODE_CAF ==
                    AfMode) { /*dv mode but recording not start*/
                AfMode = CAMERA_FOCUS_MODE_CAF_VIDEO;
            }
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE, AfMode);
        }
    } break;
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE: {
        int8_t antibanding_mode = 0;
        mSetting->androidAntibandingModeToDrvAntibandingMode(
            controlInfo.antibanding_mode, &antibanding_mode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ANTIBANDING,
                 antibanding_mode);
    } break;

    case ANDROID_SPRD_BRIGHTNESS: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_BRIGHTNESS,
                 (uint32_t)sprddefInfo.brightness);
    } break;

    case ANDROID_SPRD_AI_SCENE_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD(" sprddefInfo.ai_scene_enabled: %d",
                 sprddefInfo.ai_scene_enabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AI_SCENE_ENABLED,
                 (uint32_t)sprddefInfo.ai_scene_enabled);
    } break;

    case ANDROID_SPRD_CONTRAST: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CONTRAST,
                 (uint32_t)sprddefInfo.contrast);
    } break;

    case ANDROID_SPRD_SATURATION: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SATURATION,
                 (uint32_t)sprddefInfo.saturation);
    } break;

    case ANDROID_JPEG_ORIENTATION: {
        JPEG_Tag jpegInfo;
        mSetting->getJPEGTag(&jpegInfo);
        mJpegRotaSet = true;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ENCODE_ROTATION,
                 jpegInfo.orientation);
        HAL_LOGD("JPEG orientation = %d", jpegInfo.orientation);
    } break;

    case ANDROID_CONTROL_AE_MODE:
        if (mCameraId == 0 || mCameraId == 1) {
            int8_t drvAeMode;
            mSetting->androidAeModeToDrvAeMode(controlInfo.ae_mode, &drvAeMode);

            if (controlInfo.ae_mode != ANDROID_CONTROL_AE_MODE_OFF) {
                if (drvAeMode != CAMERA_FLASH_MODE_TORCH &&
                    mFlashMode != CAMERA_FLASH_MODE_TORCH) {
                    if (mFlashMode != drvAeMode) {
                        mFlashMode = drvAeMode;
                        HAL_LOGD(
                            "set flash mode capture_state:%d mFlashMode:%d",
                            mCameraState.capture_state, mFlashMode);
                        if (mCameraState.capture_state ==
                            SPRD_FLASH_IN_PROGRESS) {
                            break;
                        } else {
                            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH,
                                     mFlashMode);
                        }
                        if (controlInfo.ae_state !=
                            ANDROID_CONTROL_AE_STATE_LOCKED) {
                            controlInfo.ae_state =
                                ANDROID_CONTROL_AE_STATE_SEARCHING;
                            mSetting->setAeCONTROLTag(&controlInfo);
                        }
                    }
                }
            }
        }
        break;

    case ANDROID_CONTROL_AE_LOCK: {
        uint8_t ae_lock;
        ae_lock = controlInfo.ae_lock;
        HAL_LOGD("ANDROID_CONTROL_AE_LOCK, ae_lock = %d", ae_lock);
        if (ae_lock && controlInfo.ae_mode != ANDROID_CONTROL_AE_MODE_OFF) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AE_LOCK_UNLOCK,
                     ae_lock);
            controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_LOCKED;
            mSetting->setAeCONTROLTag(&controlInfo);
        } else if (!ae_lock &&
                   controlInfo.ae_state == ANDROID_CONTROL_AE_STATE_LOCKED) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AE_LOCK_UNLOCK,
                     ae_lock);
            controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_SEARCHING;
            mSetting->setAeCONTROLTag(&controlInfo);
        }
    } break;

    case ANDROID_CONTROL_AE_REGIONS: {
        struct cmr_ae_param ae_param;
        struct img_rect sensor_trim = {0, 0, 0, 0};
        struct img_rect ae_aera = {0, 0, 0, 0};

        ae_aera.start_x = controlInfo.ae_regions[0];
        ae_aera.start_y = controlInfo.ae_regions[1];
        ae_aera.width = controlInfo.ae_regions[2] - controlInfo.ae_regions[0];
        ae_aera.height = controlInfo.ae_regions[3] - controlInfo.ae_regions[1];

        mHalOem->ops->camera_get_sensor_trim(mCameraHandle, &sensor_trim);
        ret = CameraConvertCropRegion(sensor_trim.width, sensor_trim.height,
                                      &ae_aera);
        if (ret) {
            HAL_LOGE("CameraConvertCropRegion failed");
        }

        ae_param.win_area.count = 1;
        ae_param.win_area.rect[0].start_x = ae_aera.start_x;
        ae_param.win_area.rect[0].start_y = ae_aera.start_y;
        ae_param.win_area.rect[0].width = ae_aera.width;
        ae_param.win_area.rect[0].height = ae_aera.height;

        HAL_LOGI("after crop ae region is %d %d %d %d",
                 ae_param.win_area.rect[0].start_x,
                 ae_param.win_area.rect[0].start_y,
                 ae_param.win_area.rect[0].width,
                 ae_param.win_area.rect[0].height);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AE_REGION,
                 (cmr_uint)&ae_param);

    } break;

    case ANDROID_FLASH_MODE:
        if (mCameraId == 0 || mCameraId == 1) {
            int8_t flashMode;
            FLASH_Tag flashInfo;
            mSetting->getFLASHTag(&flashInfo);
            mSetting->androidFlashModeToDrvFlashMode(flashInfo.mode,
                                                     &flashMode);
            if (CAMERA_FLASH_MODE_TORCH == flashMode ||
                CAMERA_FLASH_MODE_TORCH == mFlashMode) {
                mFlashMode = flashMode;
                SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH,
                         mFlashMode);
            }
        }
        break;

    case ANDROID_SPRD_CAPTURE_MODE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM,
                 sprddefInfo.capture_mode);
        if (sprddefInfo.capture_mode > 1)
            BurstCapCnt = sprddefInfo.capture_mode;
    } break;

    case ANDROID_LENS_FOCAL_LENGTH:
        LENS_Tag lensInfo;
        mSetting->getLENSTag(&lensInfo);
        if (lensInfo.focal_length) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                     (cmr_uint)(lensInfo.focal_length * 1000));
        }
        break;

    case ANDROID_JPEG_QUALITY:
        JPEG_Tag jpgInfo;
        mSetting->getJPEGTag(&jpgInfo);
        if (jpgInfo.quality) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
                     jpgInfo.quality);
        }
        break;

    case ANDROID_JPEG_THUMBNAIL_QUALITY:
        break;

    case ANDROID_JPEG_THUMBNAIL_SIZE:
        break;

    case ANDROID_SPRD_METERING_MODE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        struct cmr_ae_param ae_param;
        memset(&ae_param, 0, sizeof(ae_param));
        ae_param.mode = sprddefInfo.am_mode;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AUTO_EXPOSURE_MODE,
                 (cmr_uint)&ae_param);
    } break;

    case ANDROID_SPRD_ISO: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISO,
                 (cmr_uint)sprddefInfo.iso);
    } break;

    case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
        setPreviewFps(mRecordingMode);
        break;

    case ANDROID_SPRD_ZSL_ENABLED:
        break;

    case ANDROID_SPRD_SLOW_MOTION: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("slow_motion=%d", sprddefInfo.slowmotion);
        if (sprddefInfo.slowmotion > 1) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SLOW_MOTION_FLAG,
                     sprddefInfo.slowmotion);
        } else {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SLOW_MOTION_FLAG, 0);
        }
    } break;
#ifdef CONFIG_CAMERA_PIPVIV_SUPPORT
    case ANDROID_SPRD_PIPVIV_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprd_pipviv_enabled=%d camera id %d",
                 sprddefInfo.sprd_pipviv_enabled, mCameraId);
        if (sprddefInfo.sprd_pipviv_enabled == 1) {
            mSprdPipVivEnabled = true;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_PIPVIV_ENABLED,
                     1);
        } else {
            mSprdPipVivEnabled = false;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_PIPVIV_ENABLED,
                     0);
        }
    } break;
#endif
    case ANDROID_SPRD_CONTROL_REFOCUS_ENABLE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_REFOCUS_ENABLE,
                 sprddefInfo.refocus_enable);

    } break;
    case ANDROID_SPRD_SET_TOUCH_INFO: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        struct touch_coordinate touch_param;
        touch_param.touchX = sprddefInfo.touchxy[0];
        touch_param.touchY = sprddefInfo.touchxy[1];
        HAL_LOGD("ANDROID_SPRD_SET_TOUCH_INFO, touchX = %d, touchY %d",
                 touch_param.touchX, touch_param.touchY);

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_TOUCH_XY,
                 (cmr_uint)&touch_param);

    } break;
    case ANDROID_SPRD_3DCALIBRATION_ENABLED: /**add for 3d calibration get max
                                                sensor size begin*/
    {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        mSprd3dCalibrationEnabled = sprddefInfo.sprd_3dcalibration_enabled;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                 sprddefInfo.sprd_3dcalibration_enabled);
        HAL_LOGD("sprddefInfo.sprd_3dcalibration_enabled=%d "
                 "mSprd3dCalibrationEnabled %d",
                 sprddefInfo.sprd_3dcalibration_enabled,
                 mSprd3dCalibrationEnabled);
    } break; /**add for 3d calibration get max sensor size end*/
    case ANDROID_SPRD_3DNR_ENABLED: /*add for 3dnr*/
    {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprddefInfo.sprd_3dnr_enabled=%d ",
                 sprddefInfo.sprd_3dnr_enabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DNR_ENABLED,
                 sprddefInfo.sprd_3dnr_enabled);
    } break;
    case ANDROID_SPRD_FIXED_FPS_ENABLED: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        mFixedFpsEnabled = sprddefInfo.sprd_fixedfps_enabled;
    } break;
    case ANDROID_SPRD_APP_MODE_ID: {
        SPRD_DEF_Tag sprddefInfo;
        char value[PROPERTY_VALUE_MAX];
        mSetting->getSPRDDEFTag(&sprddefInfo);
        mSprdAppmodeId = sprddefInfo.sprd_appmode_id;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_SET_APPMODE,
                 sprddefInfo.sprd_appmode_id);

        if ((CAMERA_MODE_CONTINUE != mSprdAppmodeId) &&
            (CAMERA_MODE_FILTER != mSprdAppmodeId) &&
            (CAMERA_MODE_3DNR_PHOTO != mSprdAppmodeId) && (0 == mFbOn) &&
            (0 == mMultiCameraMode) &&
            (ANDROID_CONTROL_SCENE_MODE_HDR != controlInfo.scene_mode) &&
            (false == mRecordingMode)) {
            property_get("persist.vendor.cam.cnr.mode", value, "0");
            if (atoi(value)) {
                mCNRMode = 1;
            }
        } else {
            mCNRMode = 0;
        }
        HAL_LOGD("mCNRMode = %d", mCNRMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ENABLE_CNR,
                 mCNRMode);
    } break;
    case ANDROID_SPRD_FILTER_TYPE: {
        SPRD_DEF_Tag sprddefInfo;
        mSetting->getSPRDDEFTag(&sprddefInfo);
        HAL_LOGD("sprddefInfo.sprd_filter_type: %d ",
                 sprddefInfo.sprd_filter_type);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FILTER_TYPE,
                 sprddefInfo.sprd_filter_type);
    } break;
    case ANDROID_SPRD_AUTO_HDR_ENABLED: {
        SPRD_DEF_Tag sprdInfo;
        mSetting->getSPRDDEFTag(&sprdInfo);
        HAL_LOGD("sprdInfo.sprd_auto_hdr_enables=%d ",
                 sprdInfo.sprd_auto_hdr_enable);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_AUTO_HDR_ENABLED,
                 sprdInfo.sprd_auto_hdr_enable);
    } break;
    default:
        ret = BAD_VALUE;
        break;
    }
    return ret;
}

int SprdCamera3OEMIf::SetJpegGpsInfo(bool is_set_gps_location) {
    if (is_set_gps_location)
        jpeg_gps_location = true;

    return 0;
}

int SprdCamera3OEMIf::setCapturePara(camera_capture_mode_t cap_mode,
                                     uint32_t frame_number) {
    char value[PROPERTY_VALUE_MAX];
    char value2[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    HAL_LOGD("cap_mode = %d", cap_mode);
    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    switch (cap_mode) {
    case CAMERA_CAPTURE_MODE_PREVIEW:
        if (sprddefInfo.sprd_zsl_enabled == 1 || mStreamOnWithZsl == 1) {
            mTakePictureMode = SNAPSHOT_ZSL_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
        } else {
            mCaptureMode = CAMERA_NORMAL_MODE;
            mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
        }
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mRecordingMode = false;
        mZslPreviewMode = false;
        break;

    case CAMERA_CAPTURE_MODE_STILL_CAPTURE:
        if (sprddefInfo.sprd_zsl_enabled == 1 || mStreamOnWithZsl == 1) {
            mTakePictureMode = SNAPSHOT_ZSL_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mPicCaptureCnt = 1;
        } else {
            mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
            mCaptureMode = CAMERA_NORMAL_MODE;
            mPicCaptureCnt = 1;
            if (!strcmp(value, "raw")) {
                HAL_LOGD("enter isp tuning mode");
                mCaptureMode = CAMERA_ISP_TUNING_MODE;
            } else if (!strcmp(value, "sim")) {
                HAL_LOGD("enter isp simulation mode");
                mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
            }
        }

        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mRecordingMode = false;
        mZslPreviewMode = false;
        break;

    case CAMERA_CAPTURE_MODE_VIDEO:
        // change it to CAMERA_ZSL_MODE when start video record
        mCaptureMode = CAMERA_NORMAL_MODE;
        mTakePictureMode = SNAPSHOT_DEFAULT_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
        mRecordingMode = true;
        mZslPreviewMode = false;
        mVideoShotFlag = 0;
        break;

    case CAMERA_CAPTURE_MODE_VIDEO_SNAPSHOT:
        if (mVideoSnapshotType != 1) {
            mTakePictureMode = SNAPSHOT_ZSL_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
            mRecordingMode = true;
            mPicCaptureCnt = 1;
            mZslPreviewMode = false;
        } else {
            mTakePictureMode = SNAPSHOT_VIDEO_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DV;
            mRecordingMode = true;
            mPicCaptureCnt = 1;
            mZslPreviewMode = false;
            mVideoShotNum = frame_number;
            mVideoShotFlag = 1;
        }
        break;

    // zsl shutter lag not support now, will support it when reprocess ready
    case CAMERA_CAPTURE_MODE_ZERO_SHUTTER_LAG:
        mCaptureMode = CAMERA_ZSL_MODE;
        mTakePictureMode = SNAPSHOT_ZSL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mRecordingMode = false;
        mZslPreviewMode = true;
        break;

    case CAMERA_CAPTURE_MODE_CTS_YUVCALLBACK_AND_SNAPSHOT:
        mTakePictureMode = SNAPSHOT_PREVIEW_MODE;
        mCaptureMode = CAMERA_ZSL_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mPreviewFormat = CAMERA_DATA_FORMAT_YUV420;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;

    case CAMERA_CAPTURE_MODE_ISP_TUNING_TOOL:
        mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
        mCaptureMode = CAMERA_ISP_TUNING_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mRecordingMode = false;
        mPicCaptureCnt = 1;
        mZslPreviewMode = false;
        break;
    case CAMERA_CAPTURE_MODE_ISP_SIMULATION_TOOL:
        mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
        mCaptureMode = CAMERA_ISP_SIMULATION_MODE;
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mRecordingMode = false;
        mPicCaptureCnt = 100;
        mZslPreviewMode = false;
        break;

    default:
        break;
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::timer_stop() {
    if (mPrvTimerID) {
        timer_delete(mPrvTimerID);
        mPrvTimerID = NULL;
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::timer_set(void *obj, int32_t delay_ms,
                                timer_handle_func handler) {
    int status;
    struct itimerspec ts;
    struct sigevent se;
    SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(obj);

    if (!mPrvTimerID) {
        memset(&se, 0, sizeof(struct sigevent));
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = dev;
        se.sigev_notify_function = handler;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &mPrvTimerID);
        if (status != 0) {
            HAL_LOGD("time create fail");
            return status;
        }
        HAL_LOGD("timer id=%p ms=%d", mPrvTimerID, delay_ms);
    }

    ts.it_value.tv_sec = delay_ms / 1000;
    ts.it_value.tv_nsec = (delay_ms - (ts.it_value.tv_sec * 1000)) * 1000000;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    status = timer_settime(mPrvTimerID, 0, &ts, NULL);

    return status;
}

void SprdCamera3OEMIf::timer_hand(union sigval arg) {
    SprdCamera3Stream *pre_stream = NULL;
    uint32_t frm_num = 0;
    int ret = NO_ERROR;
    bool ispStart = false;
    SENSOR_Tag sensorInfo;
    SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(arg.sival_ptr);
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(dev->mRegularChan);
    HAL_LOGD("E proc stat=%d timer id=%p get lock bef", dev->mIspToolStart,
             dev->mPrvTimerID);

    timer_delete(dev->mPrvTimerID);
    dev->mPrvTimerID = NULL;
    {
        Mutex::Autolock l(&dev->mLock);
        ispStart = dev->mIspToolStart;
    }
    if (ispStart) {
        channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
        if (pre_stream) { // preview stream
            ret = pre_stream->getQBuffFirstNum(&frm_num);
            if (ret == NO_ERROR) {
                dev->mSetting->getSENSORTag(&sensorInfo);
                channel->channelClearInvalidQBuff(frm_num, sensorInfo.timestamp,
                                                  CAMERA_STREAM_TYPE_PREVIEW);
                dev->timer_set(dev, ISP_TUNING_WAIT_MAX_TIME, timer_hand);
            }
        }
    }

    HAL_LOGD("X frm=%d", frm_num);
}

void SprdCamera3OEMIf::timer_hand_take(union sigval arg) {
    int ret = NO_ERROR;
    SprdCamera3OEMIf *dev = reinterpret_cast<SprdCamera3OEMIf *>(arg.sival_ptr);
    HAL_LOGD("E timer id=%p", dev->mPrvTimerID);

    timer_delete(dev->mPrvTimerID);
    dev->mPrvTimerID = NULL;
    dev->mCaptureMode = CAMERA_NORMAL_MODE;
    dev->takePicture();
}

int SprdCamera3OEMIf::Callback_VideoFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mVideoHeapNum %d sum %d", mVideoHeapNum, sum);
    for (i = 0; i < mVideoHeapNum; i++) {
        if (NULL != mVideoHeapArray[i]) {
            freeCameraMem(mVideoHeapArray[i]);
        }
        mVideoHeapArray[i] = NULL;
    }

    mVideoHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_VideoMalloc(cmr_u32 size, cmr_u32 sum,
                                           cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;
    SPRD_DEF_Tag sprddefInfo;
    int BufferCount = kVideoBufferCount;

    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion <= 1)
        BufferCount = 8;

    HAL_LOGD("size %d sum %d mVideoHeapNum %d", size, sum, mVideoHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mVideoHeapNum >= (kVideoBufferCount + kVideoRotBufferCount + 1)) {
        HAL_LOGE("error mPreviewHeapNum %d", mVideoHeapNum);
        return BAD_VALUE;
    }

    if ((mVideoHeapNum + sum) >=
        (kVideoBufferCount + kVideoRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mVideoHeapNum, sum);
        return BAD_VALUE;
    }

    if (sum > (cmr_uint)BufferCount) {
        mVideoHeapNum = BufferCount;
        phy_addr += BufferCount;
        vir_addr += BufferCount;
        fd += BufferCount;
        for (i = BufferCount; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mVideoHeapArray[mVideoHeapNum] = memory;
            mVideoHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mVideoHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_VideoFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);
    Mutex::Autolock zsllock(&mZslBufLock);

    HAL_LOGD("mZslHeapNum %d sum %d", mZslHeapNum, sum);
    for (i = 0; i < mZslHeapNum; i++) {
        if (NULL != mZslHeapArray[i]) {
            freeCameraMem(mZslHeapArray[i]);
        }
        mZslHeapArray[i] = NULL;
    }

    mZslHeapNum = 0;
    releaseZSLQueue();

    return 0;
}

int SprdCamera3OEMIf::Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    SPRD_DEF_Tag sprddefInfo;
    int BufferCount = kZslBufferCount;

    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion <= 1)
        BufferCount = 8;

    HAL_LOGD("size %d sum %d mZslHeapNum %d, BufferCount %d", size, sum,
             mZslHeapNum, BufferCount);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mZslHeapNum >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("error mPreviewHeapNum %d", mZslHeapNum);
        return BAD_VALUE;
    }

    if ((mZslHeapNum + sum) >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mZslHeapNum, sum);
        return BAD_VALUE;
    }

    if (mSprdZslEnabled == true) {
        releaseZSLQueue();

        for (i = 0; i < (cmr_int)mZslNum; i++) {
            if (mZslHeapArray[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null.");
                    goto mem_fail;
                }
                mZslHeapArray[i] = memory;
                mZslHeapNum++;
            }
            *phy_addr++ = (cmr_uint)mZslHeapArray[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mZslHeapArray[i]->data;
            *fd++ = mZslHeapArray[i]->fd;
        }
        return 0;
    }

    if (sum > (cmr_uint)BufferCount) {
        mZslHeapNum = BufferCount;
        phy_addr += BufferCount;
        vir_addr += BufferCount;
        fd += BufferCount;
        for (i = BufferCount; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mZslHeapArray[mZslHeapNum] = memory;
            mZslHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mZslHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_ZslFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_RefocusFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mRefocusHeapNum %d sum %d", mRefocusHeapNum, sum);

    for (i = 0; i < mRefocusHeapNum; i++) {
        if (NULL != mRefocusHeapArray[i]) {
            freeCameraMem(mRefocusHeapArray[i]);
        }
        mRefocusHeapArray[i] = NULL;
    }
    mRefocusHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_RefocusMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mRefocusHeapNum %d", size, sum, mRefocusHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;

    if (mRefocusHeapNum >= kRefocusBufferCount) {
        HAL_LOGE("error mRefocusHeapNum %d", mRefocusHeapNum);
        return BAD_VALUE;
    }
    if (mRefocusHeapNum + sum >= kRefocusBufferCount) {
        HAL_LOGE("malloc is too more %d %d", mRefocusHeapNum, sum);
        return BAD_VALUE;
    }

    for (i = 0; i < (cmr_int)sum; i++) {
        memory = allocCameraMem(size, 1, true);

        if (NULL == memory) {
            HAL_LOGE("error memory is null.");
            goto mem_fail;
        }

        mRefocusHeapArray[mRefocusHeapNum] = memory;
        mRefocusHeapNum++;
        *phy_addr++ = 0; //(cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        if (NULL != fd)
            *fd++ = (cmr_s32)memory->fd;
    }

    return 0;

mem_fail:
    Callback_RefocusFree(0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_PdafRawFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mPdafRawHeapNum %d sum %d", mPdafRawHeapNum, sum);

    for (i = 0; i < mPdafRawHeapNum; i++) {
        if (NULL != mPdafRawHeapArray[i]) {
            freeCameraMem(mPdafRawHeapArray[i]);
        }
        mPdafRawHeapArray[i] = NULL;
    }
    mPdafRawHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_PdafRawMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mPdafRawHeapNum %d", size, sum, mPdafRawHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;

    if (mPdafRawHeapNum >= kPdafRawBufferCount) {
        HAL_LOGE("error mPdafRawHeapNum %d", mPdafRawHeapNum);
        return BAD_VALUE;
    }
    if (mPdafRawHeapNum + sum >= kPdafRawBufferCount) {
        HAL_LOGE("malloc is too more %d %d", mPdafRawHeapNum, sum);
        return BAD_VALUE;
    }

    for (i = 0; i < (cmr_int)sum; i++) {
        memory = allocCameraMem(size, 1, true);
        if (NULL == memory) {
            HAL_LOGE("error memory is null.");
            goto mem_fail;
        }

        mPdafRawHeapArray[mPdafRawHeapNum] = memory;
        mPdafRawHeapNum++;
        *phy_addr++ = 0; //(cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        if (NULL != fd)
            *fd++ = (cmr_s32)memory->fd;
    }

    return 0;

mem_fail:
    Callback_PdafRawFree(0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_PreviewFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd,
                                           cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGI("mPreviewHeapNum %d sum %d", mPreviewHeapNum, sum);

    for (i = 0; i < mPreviewHeapNum; i++) {
        if (NULL != mPreviewHeapArray[i]) {
            freeCameraMem(mPreviewHeapArray[i]);
        }
        mPreviewHeapArray[i] = NULL;
    }
    mPreviewHeapNum = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;
    SPRD_DEF_Tag sprddefInfo;
    int BufferCount = kPreviewBufferCount;

    mSetting->getSPRDDEFTag(&sprddefInfo);
    if (sprddefInfo.slowmotion <= 1)
        BufferCount = 8;

    HAL_LOGD("size %d sum %d mPreviewHeapNum %d, BufferCount %d", size, sum,
             mPreviewHeapNum, BufferCount);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mPreviewHeapNum >= (kPreviewBufferCount + kPreviewRotBufferCount + 1)) {
        HAL_LOGE("error mPreviewHeapNum %d", mPreviewHeapNum);
        return BAD_VALUE;
    }

    if ((mPreviewHeapNum + sum) >=
        (kPreviewBufferCount + kPreviewRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mPreviewHeapNum, sum);
        return BAD_VALUE;
    }

    if (sum > (cmr_uint)BufferCount) {
        int start;
        mPreviewHeapNum = BufferCount;
        phy_addr += BufferCount;
        vir_addr += BufferCount;
        fd += BufferCount;

        for (i = BufferCount; i < (cmr_uint)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mPreviewHeapArray[mPreviewHeapNum] = memory;
            mPreviewHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        HAL_LOGD("Do not need malloc, malloced num %d,request num %d, request "
                 "size 0x%x",
                 mPreviewHeapNum, sum, size);
        goto mem_fail;
    }

    return 0;

mem_fail:
    Callback_PreviewFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_CaptureFree(cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd,
                                           cmr_u32 sum) {
    cmr_u32 i;
    HAL_LOGD("mSubRawHeapNum %d sum %d", mSubRawHeapNum, sum);
    Mutex::Autolock l(&mCapBufLock);

    for (i = 0; i < mSubRawHeapNum; i++) {
        if (NULL != mSubRawHeapArray[i]) {
            freeCameraMem(mSubRawHeapArray[i]);
        }
        mSubRawHeapArray[i] = NULL;
    }
    mSubRawHeapNum = 0;
    mSubRawHeapSize = 0;
    mCapBufIsAvail = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum,
                                             cmr_uint *phy_addr,
                                             cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGD("size %d sum %d mSubRawHeapNum %d", size, sum, mSubRawHeapNum);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

cap_malloc:
    mCapBufLock.lock();

    if (mSubRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
        HAL_LOGE("error mSubRawHeapNum %d", mSubRawHeapNum);
        return BAD_VALUE;
    }

    if ((mSubRawHeapNum + sum) >= MAX_SUB_RAWHEAP_NUM) {
        HAL_LOGE("malloc is too more %d %d", mSubRawHeapNum, sum);
        return BAD_VALUE;
    }

    if (0 == mSubRawHeapNum) {
        for (i = 0; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }

            mSubRawHeapArray[mSubRawHeapNum] = memory;
            mSubRawHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
        mSubRawHeapSize = size;
    } else {
        if ((mSubRawHeapNum >= sum) && (mSubRawHeapSize >= size)) {
            HAL_LOGD("use pre-alloc cap mem");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mSubRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mSubRawHeapArray[i]->data;
                *fd++ = mSubRawHeapArray[i]->fd;
            }
        } else {
            mCapBufLock.unlock();
            Callback_CaptureFree(0, 0, 0, 0);
            goto cap_malloc;
        }
    }

    mCapBufIsAvail = 1;
    mCapBufLock.unlock();
    return 0;

mem_fail:
    mCapBufLock.unlock();
    Callback_CaptureFree(0, 0, 0, 0);
    return -1;
}

int SprdCamera3OEMIf::Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                                 cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    LOGI("Callback_CapturePathMalloc: size %d sum %d mPathRawHeapNum %d"
         "mPathRawHeapSize %d",
         size, sum, mPathRawHeapNum, mPathRawHeapSize);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mPathRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
        LOGE("Callback_CapturePathMalloc: error mPathRawHeapNum %d",
             mPathRawHeapNum);
        return -1;
    }
    if ((mPathRawHeapNum + sum) >= MAX_SUB_RAWHEAP_NUM) {
        LOGE("Callback_CapturePathMalloc: malloc is too more %d %d",
             mPathRawHeapNum, sum);
        return -1;
    }
    if (0 == mPathRawHeapNum) {
        mPathRawHeapSize = size;
        for (i = 0; i < (cmr_int)sum; i++) {
            memory = allocCameraMem(size, 1, true);

            if (NULL == memory) {
                LOGE("Callback_CapturePathMalloc: error memory is null.");
                goto mem_fail;
            }

            mPathRawHeapArray[mPathRawHeapNum] = memory;
            mPathRawHeapNum++;

            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else {
        if ((mPathRawHeapNum >= sum) && (mPathRawHeapSize >= size)) {
            LOGI("Callback_CapturePathMalloc :test");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mPathRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mPathRawHeapArray[i]->data;
                *fd++ = mPathRawHeapArray[i]->fd;
            }
        } else {
            LOGE("failed to malloc memory, malloced num %d,request num %d, "
                 "size 0x%x, request size 0x%x",
                 mPathRawHeapNum, sum, mPathRawHeapSize, size);
            goto mem_fail;
        }
    }
    return 0;

mem_fail:
    Callback_CapturePathFree(0, 0, 0, 0);
    return -1;
}

int SprdCamera3OEMIf::Callback_CapturePathFree(cmr_uint *phy_addr,
                                               cmr_uint *vir_addr, cmr_s32 *fd,
                                               cmr_u32 sum) {
    cmr_u32 i;

    LOGI("Callback_CapturePathFree: mPathRawHeapNum %d sum %d", mPathRawHeapNum,
         sum);

    for (i = 0; i < mPathRawHeapNum; i++) {
        if (NULL != mPathRawHeapArray[i]) {
            freeCameraMem(mPathRawHeapArray[i]);
        }
        mPathRawHeapArray[i] = NULL;
    }
    mPathRawHeapNum = 0;
    mPathRawHeapSize = 0;

    return 0;
}

int SprdCamera3OEMIf::Callback_OtherFree(enum camera_mem_cb_type type,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("sum %d", sum);

#ifndef USE_ONE_RESERVED_BUF
    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != mPreviewHeapReserved) {
            freeCameraMem(mPreviewHeapReserved);
            mPreviewHeapReserved = NULL;
        }
    }

    if (type == CAMERA_VIDEO_RESERVED) {
        if (NULL != mVideoHeapReserved) {
            freeCameraMem(mVideoHeapReserved);
            mVideoHeapReserved = NULL;
        }
    }

// channel0, channel2 use mCommonHeapReserved, free it when exit camera
//    if (type == CAMERA_CHANNEL_0_RESERVED) {
//
//    }
//    if (type == CAMERA_CHANNEL_1_RESERVED) {
//
//    }
//    if (type == CAMERA_CHANNEL_2_RESERVED) {
//
//    }
//    if (type == CAMERA_CHANNEL_3_RESERVED) {
//
//    }
//    if (type == CAMERA_CHANNEL_4_RESERVED) {
//
//    }

// Performance optimization:move Callback_CaptureFree to closeCamera function
//     if(type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
//             if (NULL != mZslHeapReserved) {
//                     freeCameraMem(mZslHeapReserved);
//                     mZslHeapReserved = NULL;
//             }
//     }
#endif

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            freeCameraMem(mIspLscHeapReserved);
        }
        mIspLscHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_STATIS) {
        if (NULL != mIspStatisHeapReserved) {
            mIspStatisHeapReserved->ion_heap->free_kaddr();
            freeCameraMem(mIspStatisHeapReserved);
        }
        mIspStatisHeapReserved = NULL;
    }
    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAntiFlickerHeapReserved) {
            freeCameraMem(mIspAntiFlickerHeapReserved);
        }
        mIspAntiFlickerHeapReserved = NULL;
    }

    if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (NULL != mPdafRawHeapReserved) {
            freeCameraMem(mPdafRawHeapReserved);
        }
        mPdafRawHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                freeCameraMem(mIspB4awbHeapReserved[i]);
            }
            mIspB4awbHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved && !(--mIspFirmwareReserved_cnt)) {
            freeCameraMem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspPreviewYReserved[i]) {
                freeCameraMem(mIspPreviewYReserved[i]);
            }
            mIspPreviewYReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (NULL != mIspYUVReserved) {
            freeCameraMem(mIspYUVReserved);
        }
        mIspYUVReserved = NULL;
    }

    if (type == CAMERA_SNAPSHOT_3DNR_DST) {
        if (NULL != m3DNRHeapReserverd) {
            freeCameraMem(m3DNRHeapReserverd);
        }
        m3DNRHeapReserverd = NULL;
    }

    if (type == CAMERA_SNAPSHOT_3DNR) {
        for (i = 0; i < sum; i++) {
            if (NULL != m3DNRScaleHeapReserverd[i]) {
                freeCameraMem(m3DNRScaleHeapReserverd[i]);
            }
            m3DNRScaleHeapReserverd[i] = NULL;
        }
    }

    if (type == CAMERA_PREVIEW_3DNR) {
        for (i = 0; i < sum; i++) {
            if (NULL != m3DNRPrevHeapReserverd[i]) {
                freeCameraMem(m3DNRPrevHeapReserverd[i]);
            }
            m3DNRPrevHeapReserverd[i] = NULL;
        }
    }

    if (type == CAMERA_PREVIEW_DEPTH) {
        if (NULL != mPrevDepthHeapReserved) {
            freeCameraMem(mPrevDepthHeapReserved);
        }
        mPrevDepthHeapReserved = NULL;
    }

    if (type == CAMERA_PREVIEW_SW_OUT) {
        if (NULL != mPrevSwOutHeapReserved) {
            freeCameraMem(mPrevSwOutHeapReserved);
        }
        mPrevSwOutHeapReserved = NULL;
    }

    if (type == CAMERA_CHANNEL_1) {
        for (i = 0; i < sum; i++) {
            if (NULL != mChannel1Heap[i]) {
                freeCameraMem(mChannel1Heap[i]);
            }
            mChannel1Heap[i] = NULL;
        }
    }

    if (type == CAMERA_CHANNEL_2) {
        for (i = 0; i < sum; i++) {
            if (NULL != mChannel2Heap[i]) {
                freeCameraMem(mChannel2Heap[i]);
            }
            mChannel2Heap[i] = NULL;
        }
    }

    if (type == CAMERA_CHANNEL_3) {
        for (i = 0; i < sum; i++) {
            if (NULL != mChannel3Heap[i]) {
                freeCameraMem(mChannel3Heap[i]);
            }
            mChannel3Heap[i] = NULL;
        }
    }

    if (type == CAMERA_CHANNEL_4) {
        for (i = 0; i < sum; i++) {
            if (NULL != mChannel4Heap[i]) {
                freeCameraMem(mChannel4Heap[i]);
            }
            mChannel4Heap[i] = NULL;
        }
    }

    if (type == CAMERA_4IN1_PROC) {
        for (i = 0; i < sum; i++) {
            if (NULL != m4in1HeapArray[i]) {
                freeCameraMem(m4in1HeapArray[i]);
            }
            m4in1HeapArray[i] = NULL;
        }
    }

    if (type == CAMERA_PREVIEW_SCALE_AI_SCENE) {
        if (NULL != mAISceneScaleHeapReserverd) {
            freeCameraMem(mAISceneScaleHeapReserverd);
        }
        mAISceneScaleHeapReserverd = NULL;
    }

    return 0;
}

int SprdCamera3OEMIf::Callback_OtherMalloc(enum camera_mem_cb_type type,
                                           cmr_u32 size, cmr_u32 *sum_ptr,
                                           cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;
    cmr_u32 mem_size;
    cmr_u32 mem_sum;
    int buffer_id;
    int ret;

    cmr_u32 sum = *sum_ptr;

    HAL_LOGD("size=%d, sum=%d, mem_type=%d", size, sum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

#ifdef USE_ONE_RESERVED_BUF
    if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_VIDEO_RESERVED ||
        type == CAMERA_SNAPSHOT_ZSL_RESERVED ||
        type == CAMERA_CHANNEL_0_RESERVED ||
        type == CAMERA_CHANNEL_1_RESERVED ||
        type == CAMERA_CHANNEL_2_RESERVED ||
        type == CAMERA_CHANNEL_3_RESERVED ||
        type == CAMERA_CHANNEL_4_RESERVED) {
        if (mCommonHeapReserved == NULL) {
            mem_size = mLargestPictureWidth * mLargestPictureHeight * 3 / 2;
            memory = allocCameraMem(mem_size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null");
                goto mem_fail;
            }
            mCommonHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mCommonHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mCommonHeapReserved->data;
        *fd++ = mCommonHeapReserved->fd;
    }
#else
    if (type == CAMERA_PREVIEW_RESERVED) {
        if (mPreviewHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mPreviewHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
        *fd++ = mPreviewHeapReserved->fd;
    } else if (type == CAMERA_VIDEO_RESERVED) {
        if (mVideoHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mVideoHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mVideoHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mVideoHeapReserved->data;
        *fd++ = mVideoHeapReserved->fd;
    } else if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        if (mZslHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mZslHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mZslHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mZslHeapReserved->data;
        *fd++ = mZslHeapReserved->fd;
    }
#endif
    else if (type == CAMERA_DEPTH_MAP_RESERVED) {
        if (mDepthHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mDepthHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mDepthHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mDepthHeapReserved->data;
        *fd++ = mDepthHeapReserved->fd;
    } else if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (mPdafRawHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mPdafRawHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mPdafRawHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPdafRawHeapReserved->data;
        *fd++ = mPdafRawHeapReserved->fd;
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
        *fd++ = mIspLscHeapReserved->fd;
    } else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        cmr_s32 rtn = 0;
        if (mIspStatisHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null");
                goto mem_fail;
            }
            mIspStatisHeapReserved = memory;
        }
        // shark5 dont need kernel software r/w the buffer
        //rtn = mIspStatisHeapReserved->ion_heap->get_kaddr(&kaddr, &ksize);
        //if (rtn) {
        //    HAL_LOGE("get kaddr error");
        //    goto mem_fail;
        //}
        *phy_addr++ = kaddr;
        *phy_addr = kaddr >> 32;
        *vir_addr++ = (cmr_uint)mIspStatisHeapReserved->data;
        *fd++ = mIspStatisHeapReserved->fd;
        *fd++ = mIspStatisHeapReserved->dev_fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        cmr_s32 rtn = 0;

        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspB4awbHeapReserved[i] = memory;
            *phy_addr_64++ = (cmr_u64)memory->phys_addr;
            *vir_addr_64++ = (cmr_u64)memory->data;
            rtn = memory->ion_heap->get_kaddr(&kaddr, &ksize);
            if (rtn) {
                HAL_LOGE("get kaddr error");
                goto mem_fail;
            }
            *phy_addr++ = kaddr;
            *phy_addr = kaddr >> 32;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAntiFlickerHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspAntiFlickerHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->data;
        *fd++ = mIspAntiFlickerHeapReserved->fd;
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        } else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspPreviewYReserved[i] = memory;
            *phy_addr++ = 0;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (mIspYUVReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mIspYUVReserved = memory;
        }
        *phy_addr++ = 0;
        *vir_addr++ = (cmr_uint)mIspYUVReserved->data;
        *fd++ = mIspYUVReserved->fd;
    } else if (type == CAMERA_SNAPSHOT_3DNR) {
        for (i = 0; i < sum; i++) {
            if (m3DNRScaleHeapReserverd[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null,malloced type %d", type);
                    goto mem_fail;
                }
                m3DNRScaleHeapReserverd[i] = memory;
            }
            *phy_addr++ = (cmr_uint)m3DNRScaleHeapReserverd[i]->phys_addr;
            *vir_addr++ = (cmr_uint)m3DNRScaleHeapReserverd[i]->data;
            *fd++ = m3DNRScaleHeapReserverd[i]->fd;
        }
    } else if (type == CAMERA_SNAPSHOT_3DNR_DST) {
        if (m3DNRHeapReserverd == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            m3DNRHeapReserverd = memory;
        }
        *phy_addr++ = (cmr_uint)m3DNRHeapReserverd->phys_addr;
        *vir_addr++ = (cmr_uint)m3DNRHeapReserverd->data;
        *fd++ = m3DNRHeapReserverd->fd;
    } else if (type == CAMERA_PREVIEW_3DNR) {
        for (i = 0; i < sum; i++) {
            if (m3DNRPrevHeapReserverd[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null,malloced type %d", type);
                    goto mem_fail;
                }
                m3DNRPrevHeapReserverd[i] = memory;
            }
            *phy_addr++ = (cmr_uint)m3DNRPrevHeapReserverd[i]->phys_addr;
            *vir_addr++ = (cmr_uint)m3DNRPrevHeapReserverd[i]->data;
            *fd++ = m3DNRPrevHeapReserverd[i]->fd;
        }
    } else if (type == CAMERA_PREVIEW_DEPTH) {
        HAL_LOGD("REAL TIME DEPTH");
        if (mPrevDepthHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mPrevDepthHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mPrevDepthHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPrevDepthHeapReserved->data;
        *fd++ = mPrevDepthHeapReserved->fd;
    } else if (type == CAMERA_PREVIEW_SW_OUT) {
        HAL_LOGD("REAL TIME SW OUT");
        if (mPrevSwOutHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mPrevSwOutHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mPrevSwOutHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPrevSwOutHeapReserved->data;
        *fd++ = mPrevSwOutHeapReserved->fd;
    } else if (type == CAMERA_CHANNEL_1) {
        for (i = 0; i < sum; i++) {
            if (mChannel1Heap[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null,malloced type %d", type);
                    goto mem_fail;
                }
                mChannel1Heap[i] = memory;
                mChannel1HeapNum++;
            }
            if (mChannel1Heap[i]->data == NULL || mChannel1Heap[i]->fd < 0) {
                HAL_LOGE("malloced type %d", type);
                goto mem_fail;
            }
            *phy_addr++ = (cmr_uint)mChannel1Heap[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mChannel1Heap[i]->data;
            *fd++ = mChannel1Heap[i]->fd;
        }
    } else if (type == CAMERA_CHANNEL_2) {
        for (i = 0; i < sum; i++) {
            if (mChannel2Heap[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null,malloced type %d", type);
                    goto mem_fail;
                }
                mChannel2Heap[i] = memory;
                mChannel2HeapNum++;
            }
            if (mChannel2Heap[i]->data == NULL || mChannel2Heap[i]->fd < 0) {
                HAL_LOGE("malloced type %d", type);
                goto mem_fail;
            }
            *phy_addr++ = (cmr_uint)mChannel2Heap[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mChannel2Heap[i]->data;
            *fd++ = mChannel2Heap[i]->fd;
        }
    } else if (type == CAMERA_CHANNEL_3) {
        for (i = 0; i < sum; i++) {
            if (mChannel3Heap[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null,malloced type %d", type);
                    goto mem_fail;
                }
                mChannel3Heap[i] = memory;
                mChannel3HeapNum++;
            }
            if (mChannel3Heap[i]->data == NULL || mChannel3Heap[i]->fd < 0) {
                HAL_LOGE("malloced type %d", type);
                goto mem_fail;
            }
            *phy_addr++ = (cmr_uint)mChannel3Heap[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mChannel3Heap[i]->data;
            *fd++ = mChannel3Heap[i]->fd;
        }
    } else if (type == CAMERA_CHANNEL_4) {
        for (i = 0; i < sum; i++) {
            if (mChannel4Heap[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null,malloced type %d", type);
                    goto mem_fail;
                }
                mChannel4Heap[i] = memory;
                mChannel4HeapNum++;
            }
            if (mChannel4Heap[i]->data == NULL || mChannel4Heap[i]->fd < 0) {
                HAL_LOGE("malloced type %d", type);
                goto mem_fail;
            }
            *phy_addr++ = (cmr_uint)mChannel4Heap[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mChannel4Heap[i]->data;
            *fd++ = mChannel4Heap[i]->fd;
        }
    } else if (type == CAMERA_4IN1_PROC) {
        for (i = 0; i < sum; i++) {
            if (m4in1HeapArray[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null,malloced type %d", type);
                    goto mem_fail;
                }
                m4in1HeapArray[i] = memory;
            }
            *phy_addr++ = (cmr_uint)m4in1HeapArray[i]->phys_addr;
            *vir_addr++ = (cmr_uint)m4in1HeapArray[i]->data;
            *fd++ = m4in1HeapArray[i]->fd;
        }
    } else if (type == CAMERA_PREVIEW_SCALE_AI_SCENE) {
        if (mAISceneScaleHeapReserverd == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mAISceneScaleHeapReserverd = memory;
        }
        *phy_addr++ = (cmr_uint)mAISceneScaleHeapReserverd->phys_addr;
        *vir_addr++ = (cmr_uint)mAISceneScaleHeapReserverd->data;
        *fd++ = mAISceneScaleHeapReserverd->fd;
    }

    return 0;

mem_fail:
    Callback_OtherFree(type, 0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_Free(enum camera_mem_cb_type type,
                                    cmr_uint *phy_addr, cmr_uint *vir_addr,
                                    cmr_s32 *fd, cmr_u32 sum,
                                    void *private_data) {
    SprdCamera3OEMIf *camera = (SprdCamera3OEMIf *)private_data;
    int ret = 0;
    HAL_LOGV("E");

    if (!private_data || !vir_addr || !fd) {
        HAL_LOGE("error param %p 0x%lx 0x%lx", fd, (cmr_uint)vir_addr,
                 (cmr_uint)private_data);
        return BAD_VALUE;
    }

    if (type >= CAMERA_MEM_CB_TYPE_MAX) {
        HAL_LOGE("mem type error %ld", (cmr_uint)type);
        return BAD_VALUE;
    }

    if (CAMERA_PREVIEW == type) {
        ret = camera->Callback_PreviewFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT == type) {
        // Performance optimization:move Callback_CaptureFree to closeCamera
        // function
        // ret = camera->Callback_CaptureFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_VIDEO == type) {
        ret = camera->Callback_VideoFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_DEPTH_MAP == type) {
        ret = camera->Callback_RefocusFree(phy_addr, vir_addr, sum);
    } else if (CAMERA_PDAF_RAW == type) {
        ret = camera->Callback_PdafRawFree(phy_addr, vir_addr, sum);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_PREVIEW_RESERVED == type ||
               CAMERA_VIDEO_RESERVED == type || CAMERA_ISP_FIRMWARE == type ||
               CAMERA_SNAPSHOT_ZSL_RESERVED == type ||
               CAMERA_DEPTH_MAP_RESERVED == type ||
               CAMERA_PDAF_RAW_RESERVED == type || CAMERA_ISP_LSC == type ||
               CAMERA_ISP_STATIS == type || CAMERA_ISP_BINGING4AWB == type ||
               CAMERA_ISP_RAW_DATA == type || CAMERA_ISP_PREVIEW_Y == type ||
               CAMERA_ISP_PREVIEW_YUV == type || CAMERA_SNAPSHOT_3DNR == type ||
               CAMERA_SNAPSHOT_3DNR_DST == type ||
               CAMERA_PREVIEW_3DNR == type || CAMERA_PREVIEW_DEPTH == type ||
               CAMERA_PREVIEW_SW_OUT == type || CAMERA_4IN1_PROC == type ||
               CAMERA_CHANNEL_0_RESERVED == type ||
               CAMERA_CHANNEL_1_RESERVED == type ||
               CAMERA_CHANNEL_2_RESERVED == type ||
               CAMERA_CHANNEL_3_RESERVED == type ||
               CAMERA_CHANNEL_4_RESERVED == type ||
               CAMERA_PREVIEW_SCALE_AI_SCENE == type) {
        ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_CHANNEL_1 == type) {
        ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_CHANNEL_2 == type) {
        ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_CHANNEL_3 == type) {
        ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_CHANNEL_4 == type) {
        ret = camera->Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    }

    HAL_LOGV("X");
    return ret;
}

int SprdCamera3OEMIf::Callback_Malloc(enum camera_mem_cb_type type,
                                      cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
                                      cmr_uint *phy_addr, cmr_uint *vir_addr,
                                      cmr_s32 *fd, void *private_data) {
    SprdCamera3OEMIf *camera = (SprdCamera3OEMIf *)private_data;
    int ret = 0, i = 0;
    uint32_t size, sum;
    SprdCamera3RegularChannel *channel = NULL;
    HAL_LOGV("E");

    if (!private_data || !vir_addr || !fd || !size_ptr || !sum_ptr ||
        (0 == *size_ptr) || (0 == *sum_ptr)) {
        HAL_LOGE("param error %p 0x%lx 0x%lx 0x%lx 0x%lx", fd,
                 (cmr_uint)vir_addr, (cmr_uint)private_data, (cmr_uint)size_ptr,
                 (cmr_uint)sum_ptr);
        return BAD_VALUE;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (type >= CAMERA_MEM_CB_TYPE_MAX) {
        HAL_LOGE("mem type error %ld", (cmr_uint)type);
        return BAD_VALUE;
    }

    if (CAMERA_PREVIEW == type) {
        ret = camera->Callback_PreviewMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT == type) {
        ret = camera->Callback_CaptureMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_VIDEO == type) {
        ret = camera->Callback_VideoMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_DEPTH_MAP == type) {
        ret = camera->Callback_RefocusMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_PDAF_RAW == type) {
        ret = camera->Callback_PdafRawMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathMalloc(size, sum, phy_addr, vir_addr,
                                                 fd);
    } else if (CAMERA_PREVIEW_RESERVED == type ||
               CAMERA_VIDEO_RESERVED == type || CAMERA_ISP_FIRMWARE == type ||
               CAMERA_SNAPSHOT_ZSL_RESERVED == type ||
               CAMERA_PDAF_RAW_RESERVED == type || CAMERA_ISP_LSC == type ||
               CAMERA_ISP_STATIS == type || CAMERA_ISP_BINGING4AWB == type ||
               CAMERA_ISP_RAW_DATA == type || CAMERA_ISP_PREVIEW_Y == type ||
               CAMERA_ISP_PREVIEW_YUV == type || CAMERA_SNAPSHOT_3DNR == type ||
               CAMERA_SNAPSHOT_3DNR_DST == type ||
               CAMERA_PREVIEW_3DNR == type || CAMERA_PREVIEW_DEPTH == type ||
               CAMERA_PREVIEW_SW_OUT == type || CAMERA_4IN1_PROC == type ||
               CAMERA_CHANNEL_0_RESERVED == type ||
               CAMERA_CHANNEL_1_RESERVED == type ||
               CAMERA_CHANNEL_2_RESERVED == type ||
               CAMERA_CHANNEL_3_RESERVED == type ||
               CAMERA_CHANNEL_4_RESERVED == type ||
               CAMERA_PREVIEW_SCALE_AI_SCENE == type) {
        ret = camera->Callback_OtherMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    } else if (CAMERA_CHANNEL_1 == type) {
        ret = camera->Callback_OtherMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    } else if (CAMERA_CHANNEL_2 == type) {
        ret = camera->Callback_OtherMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    } else if (CAMERA_CHANNEL_3 == type) {
        ret = camera->Callback_OtherMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    } else if (CAMERA_CHANNEL_4 == type) {
        ret = camera->Callback_OtherMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    }

    HAL_LOGV("X");
    return ret;
}

int SprdCamera3OEMIf::SetChannelHandle(void *regular_chan, void *picture_chan) {
    mRegularChan = regular_chan;
    mPictureChan = picture_chan;
    return NO_ERROR;
}

int SprdCamera3OEMIf::setCamStreamInfo(cam_dimension_t size, int format,
                                       int stream_tpye) {
    uint32_t imageFormat = IMG_DATA_TYPE_RAW;
    int isYuvSensor = 0, i;
    SPRD_DEF_Tag sprddefInfo;
    char value[PROPERTY_VALUE_MAX];
    struct img_size req_size;
    struct sensor_mode_info mode_info[SENSOR_MODE_MAX];

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mSetting->getSPRDDEFTag(&sprddefInfo);

    mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_SENSOR_FORMAT,
                                &imageFormat);
    if (imageFormat == IMG_DATA_TYPE_YUV422) {
        isYuvSensor = 1;
    }

    switch (stream_tpye) {
    case CAMERA_STREAM_TYPE_PREVIEW:
        mPreviewWidth = size.width;
        mPreviewHeight = size.height;
        if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP ||
            format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            mPreviewFormat = IMG_DATA_TYPE_YUV420;
        }
        if (isYuvSensor) {
            mPreviewFormat = IMG_DATA_TYPE_YUV422;
        }
        break;
    case CAMERA_STREAM_TYPE_VIDEO:
        mVideoWidth = size.width;
        mVideoHeight = size.height;
        if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP ||
            format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            mVideoFormat = IMG_DATA_TYPE_YUV420;
        }
        if (isYuvSensor) {
            mVideoFormat = IMG_DATA_TYPE_YUV422;
        }

        if (mVideoWidth > 0 && mVideoWidth >= mCaptureWidth &&
            sprddefInfo.slowmotion <= 1) {
            mVideoSnapshotType = 1;
        } else {
            mVideoSnapshotType = 0;
        }
        // just for bandwidth pressure test, zsl video snapshot
        // sudo adb shell setprop persist.vendor.cam.video.zsl true
        property_get("persist.vendor.cam.video.zsl", value, "false");
        if (!strcmp(value, "true")) {
            mVideoSnapshotType = 0;
        }
        break;
    case CAMERA_STREAM_TYPE_CALLBACK:
        mCallbackWidth = size.width;
        mCallbackHeight = size.height;
        if (format == HAL_PIXEL_FORMAT_YCbCr_420_888 ||
            format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            mCallbackFormat = IMG_DATA_TYPE_YUV420;
        }
        if (isYuvSensor) {
            mCallbackFormat = IMG_DATA_TYPE_YUV422;
        }
        break;
    case CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT:
        mCaptureWidth = size.width;
        mCaptureHeight = size.height;
        if (format == HAL_PIXEL_FORMAT_BLOB) {
            mPictureFormat = IMG_DATA_TYPE_YUV420;
        }
        if (isYuvSensor) {
            mPictureFormat = IMG_DATA_TYPE_YUV422;
        }

        property_get("persist.vendor.cam.raw.mode", value, "jpeg");
        if (!strcmp(value, "raw")) {
            mHalOem->ops->camera_get_sensor_info_for_raw(mCameraHandle,
                                                         mode_info);
            for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
                HAL_LOGD("trim w=%d, h=%d", mode_info[i].trim_width,
                         mode_info[i].trim_height);
                if (mode_info[i].trim_width >= mCaptureWidth) {
                    mCaptureWidth = mode_info[i].trim_width;
                    mCaptureHeight = mode_info[i].trim_height;
                    break;
                }
            }
            req_size.width = mCaptureWidth;
            req_size.height = mCaptureHeight;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RAW_CAPTURE_SIZE,
                     (cmr_uint)&req_size);
            HAL_LOGD("raw capture mode: mCaptureWidth=%d, mCaptureHeight=%d",
                     mCaptureWidth, mCaptureHeight);
        }
        break;

    default:
        break;
    }

    return NO_ERROR;
}

int SprdCamera3OEMIf::queueBuffer(buffer_handle_t *buff_handle,
                                  int stream_type) {
    SprdCamera3RegularChannel *channel = NULL;
    cmr_uint addr_vir = (cmr_uint)NULL, addr_phy = (cmr_uint)NULL;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;
    cam_buffer_info_t buffer;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    cmr_bzero(&buffer, sizeof(cam_buffer_info_t));

    switch (stream_type) {
    case CAMERA_STREAM_TYPE_PREVIEW:
        if (getPreviewState() != SPRD_INTERNAL_PREVIEW_REQUESTED &&
            getPreviewState() != SPRD_PREVIEW_IN_PROGRESS) {
            // set buffers to driver after params
            ret = waitForPipelineStart();
            if (ret) {
                HAL_LOGE("waitForPipelineStart failed");
                goto exit;
            }
        }

        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel == NULL) {
            ret = -1;
            HAL_LOGE("mRegularChan is null");
            goto exit;
        }

        ret = channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);
        if (ret || stream == NULL) {
            HAL_LOGE("getStream failed");
            goto exit;
        }

        ret = stream->getQBufInfoForHandle(buff_handle, &buffer);
        if (ret || buffer.addr_vir == NULL) {
            HAL_LOGE("getQBufForHandle failed");
            goto exit;
        }
        mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                   SPRD_CAM_STREAM_PREVIEW);
        break;

    case CAMERA_STREAM_TYPE_VIDEO:
        if (getPreviewState() != SPRD_INTERNAL_PREVIEW_REQUESTED &&
            getPreviewState() != SPRD_PREVIEW_IN_PROGRESS) {
            // set buffers to driver after params
            ret = waitForPipelineStart();
            if (ret) {
                HAL_LOGE("waitForPipelineStart failed");
                goto exit;
            }
        }

        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel == NULL) {
            ret = -1;
            HAL_LOGE("mRegularChan is null");
            goto exit;
        }

        ret = channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);
        if (ret || stream == NULL) {
            HAL_LOGE("getStream failed");
            goto exit;
        }

        ret = stream->getQBufInfoForHandle(buff_handle, &buffer);
        if (ret || buffer.addr_vir == NULL) {
            HAL_LOGE("getQBufForHandle failed");
            goto exit;
        }
        mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                   SPRD_CAM_STREAM_VIDEO);
        break;

    case CAMERA_STREAM_TYPE_CALLBACK:
        if (getPreviewState() != SPRD_INTERNAL_PREVIEW_REQUESTED &&
            getPreviewState() != SPRD_PREVIEW_IN_PROGRESS) {
            // set buffers to driver after params
            ret = waitForPipelineStart();
            if (ret) {
                HAL_LOGE("waitForPipelineStart failed");
                goto exit;
            }
        }

        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel == NULL) {
            ret = -1;
            HAL_LOGE("mRegularChan is null");
            goto exit;
        }

        ret = channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &stream);
        if (ret || stream == NULL) {
            HAL_LOGE("getStream failed");
            goto exit;
        }

        ret = stream->getQBufInfoForHandle(buff_handle, &buffer);
        if (ret || buffer.addr_vir == NULL) {
            HAL_LOGE("getQBufForHandle failed");
            goto exit;
        }

        // bokeh use zsl capture for callback stream
        // TBD: bokeh use standard callback strem, dont use zsl capture
        if (getMultiCameraMode() != MODE_BLUR &&
            getMultiCameraMode() != MODE_BOKEH) {
            mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                       SPRD_CAM_STREAM_CALLBACK);
        }
        break;

    case CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT:
        break;
    }

exit:
    return NO_ERROR;
}

// this func dont use for now, will be delete it later
int SprdCamera3OEMIf::qFirstBuffer(int stream_type) {
    SprdCamera3RegularChannel *channel;
    cmr_uint addr_vir = (cmr_uint)NULL, addr_phy = (cmr_uint)NULL;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;
    cam_buffer_info_t buffer;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    cmr_bzero(&buffer, sizeof(cam_buffer_info_t));

    switch (stream_type) {
    case CAMERA_STREAM_TYPE_PREVIEW:
        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel) {
            ret = channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);
            if (stream) {
                ret = stream->getQBuffFirstVir(&addr_vir);
                stream->getQBuffFirstPhy(&addr_phy);
                stream->getQBuffFirstFd(&fd);
                HAL_LOGD(
                    "addr_phy = 0x%lx, addr_vir = 0x%lx, fd = %d, ret = %d",
                    addr_phy, addr_vir, fd, ret);
                if (ret || addr_vir == 0) {
                    HAL_LOGE("getQBuffFirstVir failed");
                    stream->getQBuffFirstNum(&mDropPreviewFrameNum);
                    goto exit;
                }
                mHalOem->ops->camera_set_preview_buffer(mCameraHandle, addr_phy,
                                                        addr_vir, fd);
            }
        }
        break;

    case CAMERA_STREAM_TYPE_VIDEO:
        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel) {
            ret = channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);
            if (stream) {
                ret = stream->getQBuffFirstVir(&addr_vir);
                stream->getQBuffFirstPhy(&addr_phy);
                stream->getQBuffFirstFd(&fd);
                HAL_LOGD(
                    "addr_phy = 0x%lx, addr_vir = 0x%lx, fd = %d, ret = %d",
                    addr_phy, addr_vir, fd, ret);
                if (ret || addr_vir == 0) {
                    HAL_LOGE("getQBuffFirstVir failed");
                    stream->getQBuffFirstNum(&mDropVideoFrameNum);
                    goto exit;
                }
                mHalOem->ops->camera_set_video_buffer(mCameraHandle, addr_phy,
                                                      addr_vir, fd);
            }
        }
        break;

    case CAMERA_STREAM_TYPE_CALLBACK:
        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel) {
            ret = channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &stream);
            if (stream) {
                ret = stream->getQBufFirstBuf(&buffer);
                if (ret) {
                    CMR_LOGE("getQBufFirstBuf failed");
                    goto exit;
                }
                mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                           SPRD_CAM_STREAM_CALLBACK);
            }
        }
        break;

    default:
        break;
    }

exit:
    return ret;
}

int SprdCamera3OEMIf::PushVideoSnapShotbuff(int32_t frame_number,
                                            camera_stream_type_t type) {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint addr_vir = (cmr_uint)NULL, addr_phy = (cmr_uint)NULL;
    cmr_s32 fd = 0;
    SprdCamera3Stream *stream = NULL;
    int ret = NO_ERROR;

    HAL_LOGD("E");

    if (mVideoShotPushFlag == 0 && frame_number == mVideoShotNum)
        mVideoShotWait.wait(mPreviewCbLock);

    if (mVideoShotPushFlag) {
        channel->getStream(type, &stream);

        if (stream) {
            ret = stream->getQBufAddrForNum(frame_number, &addr_vir, &addr_phy,
                                            &fd);
            HAL_LOGD("addr_phy = 0x%lx, addr_vir = 0x%lx, fd = 0x%x, "
                     "frame_number = %d",
                     addr_phy, addr_vir, fd, frame_number);
            if (mCameraHandle != NULL && mHalOem != NULL &&
                mHalOem->ops != NULL && ret == NO_ERROR &&
                addr_vir != (cmr_uint)NULL)
                mHalOem->ops->camera_set_video_snapshot_buffer(
                    mCameraHandle, addr_phy, addr_vir, fd);

            mVideoShotPushFlag = 0;
            mVideoShotFlag = 0;
        }
    }

    HAL_LOGD("X");

    return NO_ERROR;
}

ZslBufferQueue SprdCamera3OEMIf::popZSLQueue() {
    Mutex::Autolock l(&mZslLock);

    List<ZslBufferQueue>::iterator frame;
    ZslBufferQueue ret;

    bzero(&ret, sizeof(ZslBufferQueue));
    if (mZSLQueue.size() == 0) {
        return ret;
    }
    frame = mZSLQueue.begin()++;
    ret = static_cast<ZslBufferQueue>(*frame);
    mZSLQueue.erase(frame);

    return ret;
}

struct rawBufferQueue SprdCamera3OEMIf::popRawQueue() {
    Mutex::Autolock l(&mRawLock);

    List<rawBufferQueue>::iterator frame;
    struct rawBufferQueue ret;

    bzero(&ret, sizeof(rawBufferQueue));
    if (mRawQueue.size() == 0) {
        return ret;
    }
    frame = mRawQueue.begin()++;
    ret = static_cast<rawBufferQueue>(*frame);
    mRawQueue.erase(frame);

    return ret;
}

ZslBufferQueue SprdCamera3OEMIf::popZSLQueue(uint64_t need_timestamp) {
    Mutex::Autolock l(&mZslLock);

    List<ZslBufferQueue>::iterator frame;
    ZslBufferQueue zsl_frame;

    bzero(&zsl_frame, sizeof(ZslBufferQueue));
    if (mZSLQueue.size() == 0) {
        HAL_LOGV("cameraid:%d,find invalid zsl Frame.mCameraId", mCameraId);
        return zsl_frame;
    }
    if (need_timestamp == 0) {
        for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
            if (frame->frame.isMatchFlag != 1) {
                zsl_frame = static_cast<ZslBufferQueue>(*frame);
                mZSLQueue.erase(frame);
                HAL_LOGV("pop zsl frame vddr=0x%lx", frame->frame.y_vir_addr);
                return zsl_frame;
            }
        }
        frame = mZSLQueue.begin();
        zsl_frame = static_cast<ZslBufferQueue>(*frame);
        mZSLQueue.erase(frame);
        HAL_LOGV("pop zsl end frame vddr=0x%lx", frame->frame.y_vir_addr);
        return zsl_frame;
    }

    // process match frame need
    if (mMultiCameraMatchZsl->match_frame1.heap_array == NULL &&
        mMultiCameraMatchZsl->match_frame3.heap_array == NULL) {
        frame = mZSLQueue.begin();
        zsl_frame = static_cast<ZslBufferQueue>(*frame);
        mZSLQueue.erase(frame);
        mIsUnpopped = true;
        HAL_LOGD("match_Frame NULL");
        return zsl_frame;
    }

    if (mCameraId == 1) {
        if (ns2ms(abs((int)((int64_t)mMultiCameraMatchZsl->match_frame1.frame
                                .timestamp -
                            (int64_t)need_timestamp))) > 1500) {
            HAL_LOGD("match timestmp is too long,no use it");
            for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
                if (frame->frame.isMatchFlag != 1) {
                    zsl_frame = static_cast<ZslBufferQueue>(*frame);
                    mZSLQueue.erase(frame);
                    bzero(&mMultiCameraMatchZsl->match_frame1,
                          sizeof(ZslBufferQueue));
                    HAL_LOGV("pop zsl frame vddr=0x%lx",
                             frame->frame.y_vir_addr);
                    mIsUnpopped = true;
                    return zsl_frame;
                }
            }
            frame = mZSLQueue.begin();
            zsl_frame = static_cast<ZslBufferQueue>(*frame);
            mZSLQueue.erase(frame);
            bzero(&mMultiCameraMatchZsl->match_frame1, sizeof(ZslBufferQueue));
            mIsUnpopped = true;
            HAL_LOGV("pop zsl end frame vddr=0x%lx", frame->frame.y_vir_addr);
            return zsl_frame;
        }
        mIsUnpopped = true;
        zsl_frame = mMultiCameraMatchZsl->match_frame1;
        bzero(&mMultiCameraMatchZsl->match_frame1, sizeof(ZslBufferQueue));
    }
    if (mCameraId == 3) {
        if (ns2ms(abs((int)((int64_t)mMultiCameraMatchZsl->match_frame3.frame
                                .timestamp -
                            (int64_t)need_timestamp))) > 1500) {
            HAL_LOGD("match timestmp is too long,no use it");
            for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
                if (frame->frame.isMatchFlag != 1) {
                    zsl_frame = static_cast<ZslBufferQueue>(*frame);
                    mZSLQueue.erase(frame);
                    mIsUnpopped = true;
                    HAL_LOGV("pop zsl frame vddr=0x%lx",
                             frame->frame.y_vir_addr);
                    bzero(&mMultiCameraMatchZsl->match_frame3,
                          sizeof(ZslBufferQueue));
                    return zsl_frame;
                }
            }
            frame = mZSLQueue.begin();
            zsl_frame = static_cast<ZslBufferQueue>(*frame);
            mZSLQueue.erase(frame);
            bzero(&mMultiCameraMatchZsl->match_frame3, sizeof(ZslBufferQueue));
            mIsUnpopped = true;
            HAL_LOGV("pop zsl end frame vddr=0x%lx", frame->frame.y_vir_addr);
            return zsl_frame;
        }
        mIsUnpopped = true;
        zsl_frame = mMultiCameraMatchZsl->match_frame3;
        bzero(&mMultiCameraMatchZsl->match_frame3, sizeof(ZslBufferQueue));
    }
    frame = mZSLQueue.begin();
    while (frame != mZSLQueue.end()) {
        if (frame->frame.timestamp == zsl_frame.frame.timestamp) {
            mZSLQueue.erase(frame);
            HAL_LOGD("erase match frame,mid=%d", mCameraId);
            break;
        }
        frame++;
    }
    if (frame == mZSLQueue.end()) {
        frame = mZSLQueue.begin();
        zsl_frame = static_cast<ZslBufferQueue>(*frame);
        mZSLQueue.erase(frame);
    }
    mIsUnpopped = true;
    return zsl_frame;
}

int SprdCamera3OEMIf::getZSLQueueFrameNum() {
    Mutex::Autolock l(&mZslLock);

    int ret = 0;
    ret = mZSLQueue.size();
    HAL_LOGV("%d", ret);
    return ret;
}

uint32_t SprdCamera3OEMIf::getRawQueueFrameNum() {
    Mutex::Autolock l(&mRawLock);

    int ret = 0;
    ret = mRawQueue.size();
    return ret;
}

void SprdCamera3OEMIf::matchZSLQueue(ZslBufferQueue *frame) {
    List<ZslBufferQueue>::iterator itor1, itor2;
    List<ZslBufferQueue> *match_ZSLQueue = NULL;
    ZslBufferQueue *frame1 = NULL;
    ZslBufferQueue frame_queue;
    HAL_LOGV("E");
    if (mCameraId == 1) {
        match_ZSLQueue = mMultiCameraMatchZsl->cam3_ZSLQueue;
    }
    if (mCameraId == 3) {
        match_ZSLQueue = mMultiCameraMatchZsl->cam1_ZSLQueue;
    }
    if (NULL == match_ZSLQueue || match_ZSLQueue->empty()) {
        HAL_LOGD("camera %d,match_queue.cam3_ZSLQueue empty", mCameraId);
        return;
    } else {
        itor1 = match_ZSLQueue->begin();
        while (itor1 != match_ZSLQueue->end()) {
            int diff = (int64_t)frame->frame.timestamp -
                       (int64_t)itor1->frame.timestamp;
            if (abs(diff) < DUALCAM_TIME_DIFF) {
                itor2 = mMultiCameraMatchZsl->cam1_ZSLQueue->begin();
                while (itor2 != mMultiCameraMatchZsl->cam1_ZSLQueue->end()) {
                    if (itor2->frame.timestamp ==
                        mMultiCameraMatchZsl->match_frame1.frame.timestamp) {
                        itor2->frame.isMatchFlag = 0;
                        break;
                    }
                    itor2++;
                }
                itor2 = mMultiCameraMatchZsl->cam3_ZSLQueue->begin();
                while (itor2 != mMultiCameraMatchZsl->cam3_ZSLQueue->end()) {
                    if (itor2->frame.timestamp ==
                        mMultiCameraMatchZsl->match_frame3.frame.timestamp) {
                        itor2->frame.isMatchFlag = 0;
                        break;
                    }
                    itor2++;
                }
                frame->frame.isMatchFlag = 1;
                itor1->frame.isMatchFlag = 1;
                if (mCameraId == 1) {
                    mMultiCameraMatchZsl->match_frame1 = *frame;
                    mMultiCameraMatchZsl->match_frame3 =
                        static_cast<ZslBufferQueue>(*itor1);
                }
                if (mCameraId == 3) {
                    mMultiCameraMatchZsl->match_frame1 =
                        static_cast<ZslBufferQueue>(*itor1);
                    mMultiCameraMatchZsl->match_frame3 = *frame;
                }
                break;
            }
            itor1++;
        }
    }
}

void SprdCamera3OEMIf::pushZSLQueue(ZslBufferQueue *frame) {
    Mutex::Autolock l(&mZslLock);
    if (getMultiCameraMode() == MODE_3D_CAPTURE) {
        frame->frame.isMatchFlag = 0;
        if (!mSprdMultiYuvCallBack) {
            matchZSLQueue(frame);
        }
    }
    mZSLQueue.push_back(*frame);
}

void SprdCamera3OEMIf::pushRawQueue(rawBufferQueue *node) {
    Mutex::Autolock l(&mRawLock);
    mRawQueue.push_back(*node);
}

void SprdCamera3OEMIf::releaseZSLQueue() {
    Mutex::Autolock l(&mZslLock);

    List<ZslBufferQueue>::iterator round;
    HAL_LOGD("para changed.size : %d", mZSLQueue.size());
    while (mZSLQueue.size() > 0) {
        round = mZSLQueue.begin()++;
        mZSLQueue.erase(round);
    }
}

void SprdCamera3OEMIf::releaseRawQueue() {
    Mutex::Autolock l(&mRawLock);

    List<rawBufferQueue>::iterator round;
    while (mRawQueue.size() > 0) {
        round = mRawQueue.begin()++;
        mRawQueue.erase(round);
    }
}

void SprdCamera3OEMIf::setZslBuffers() {
    cmr_uint i = 0;

    HAL_LOGD("mZslHeapNum %d", mZslHeapNum);

    releaseZSLQueue();
    for (i = 0; i < mZslHeapNum; i++) {
        mHalOem->ops->camera_set_zsl_buffer(
            mCameraHandle, mZslHeapArray[i]->phys_addr,
            (cmr_uint)mZslHeapArray[i]->data, mZslHeapArray[i]->fd);
    }
}

uint32_t SprdCamera3OEMIf::getZslBufferIDForFd(cmr_s32 fd) {
    uint32_t id = 0xFFFFFFFF;
    uint32_t i;
    for (i = 0; i < mZslHeapNum; i++) {
        if (0 != mZslHeapArray[i]->fd && mZslHeapArray[i]->fd == fd) {
            id = i;
            break;
        }
    }

    return id;
}

uint32_t SprdCamera3OEMIf::getRawBufferIDForFd(cmr_s32 fd) {
    uint32_t id = 0xFFFFFFFF;
    uint32_t i;
    for (i = 0; i < mRawNum; i++) {
        if (0 != mRawHeapArray[i]->fd && mRawHeapArray[i]->fd == fd) {
            id = i;
            break;
        }
    }

    return id;
}

int SprdCamera3OEMIf::pushZslFrame(struct camera_frame_type *frame) {
    int ret = 0;
    ZslBufferQueue zsl_buffer_q;

    frame->buf_id = getZslBufferIDForFd(frame->fd);
    memset(&zsl_buffer_q, 0, sizeof(zsl_buffer_q));
    zsl_buffer_q.frame = *frame;
    zsl_buffer_q.heap_array = mZslHeapArray[frame->buf_id];
    pushZSLQueue(&zsl_buffer_q);
    return ret;
}

struct camera_frame_type SprdCamera3OEMIf::popZslFrame() {
    ZslBufferQueue zslFrame;
    struct camera_frame_type zsl_frame;

    bzero(&zslFrame, sizeof(zslFrame));
    bzero(&zsl_frame, sizeof(struct camera_frame_type));

    if (getMultiCameraMode() == MODE_3D_CAPTURE) {
        zslFrame = popZSLQueue(mNeededTimestamp);
    } else {
        zslFrame = popZSLQueue();
    }
    zsl_frame = zslFrame.frame;

    return zsl_frame;
}

/**add for 3dcapture, record received zsl buffer begin*/
uint64_t SprdCamera3OEMIf::getZslBufferTimestamp() {
    Mutex::Autolock l(&mZslLock);
    uint64_t timestamp = 0;
    List<ZslBufferQueue>::iterator frame;

    if (mZSLQueue.size() == 0) {
        return 0;
    }
    frame = mZSLQueue.begin();
    for (frame = mZSLQueue.begin(); frame != mZSLQueue.end(); frame++) {
        timestamp = frame->frame.timestamp;
    }

    return frame->frame.timestamp;
}

int SprdCamera3OEMIf::pushRawFrame(struct camera_frame_type *frame) {
    int ret = 0;
    struct rawBufferQueue node;

    memset(&node, 0, sizeof(struct rawBufferQueue));

    frame->buf_id = getRawBufferIDForFd(frame->fd);

    node.frame = *frame;
    node.heap_array = mZslHeapArray[frame->buf_id];

    pushRawQueue(&node);

    return ret;
}

struct camera_frame_type SprdCamera3OEMIf::popRawFrame() {
    struct rawBufferQueue node;

    bzero(&node, sizeof(struct rawBufferQueue));
    node = popRawQueue();

    return node.frame;
}

int SprdCamera3OEMIf::allocateRawBuffers() {
    uint32_t ret = 0, i = 0;
    int raw_bufsize = 0;
    sprd_camera_memory_t *memory = NULL;

    releaseRawQueue();

    raw_bufsize = mRawWidth * mRawHeight * 2;

    for (i = 0; i < mRawNum; i++) {
        if (mRawHeapArray[i] == NULL) {
            memory = allocCameraMem(raw_bufsize, 1, true);
            if (NULL == memory) {
                ret = -1;
                HAL_LOGE("allocCameraMem failed");
                goto exit;
            }
            mRawHeapArray[i] = memory;
        }
    }

exit:
    return ret;
}

int SprdCamera3OEMIf::freeRawBuffers() {
    uint32_t ret = 0, i = 0;

    for (i = 0; i < mRawNum; i++) {
        if (mRawHeapArray[i] != NULL) {
            freeCameraMem(mRawHeapArray[i]);
            mRawHeapArray[i] = NULL;
        }
    }
    releaseRawQueue();

exit:
    return ret;
}

int SprdCamera3OEMIf::queueRawBuffers() {
    uint32_t ret = 0, i = 0;
    cam_buffer_info_t buffer;

    cmr_bzero(&buffer, sizeof(cam_buffer_info_t));

    for (i = 0; i < mRawNum; i++) {
        if (mRawHeapArray[i] != NULL) {
            buffer.fd = mRawHeapArray[i]->fd;
            buffer.addr_phy = (void *)mRawHeapArray[i]->phys_addr;
            buffer.addr_vir = mRawHeapArray[i]->data;
            mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                       SPRD_CAM_STREAM_RAW);
        }
    }

exit:
    return ret;
}

// this is for real zsl flash capture, like sharkls/sharklt8, not sharkl2-like
void SprdCamera3OEMIf::skipZslFrameForFlashCapture() {
    ATRACE_CALL();

    struct camera_frame_type zsl_frame;
    uint32_t cnt = 0;

    bzero(&zsl_frame, sizeof(struct camera_frame_type));

    if (mFlashCaptureFlag) {
        while (1) {
            // for exception exit
            if (mZslCaptureExitLoop == true) {
                HAL_LOGD("zsl loop exit done.");
                break;
            }

            if (cnt > mFlashCaptureSkipNum) {
                HAL_LOGD("flash capture skip %d frame", cnt);
                break;
            }
            zsl_frame = popZslFrame();
            if (zsl_frame.y_vir_addr == 0) {
                HAL_LOGD("flash capture wait for zsl frame");
                usleep(20 * 1000);
                continue;
            }

            HAL_LOGV("flash capture skip one frame");
            mHalOem->ops->camera_set_zsl_buffer(
                mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            zsl_frame.y_vir_addr = 0;
            cnt++;
        }
    }
exit:
    HAL_LOGV("X");
}

void SprdCamera3OEMIf::snapshotZsl(void *p_data) {
    ATRACE_CALL();

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    struct camera_frame_type zsl_frame;
    struct image_sw_algorithm_buf sw_algorithm_buf;
    uint32_t cnt = 0;
    int64_t diff_ms = 0;
    uint32_t sw_algorithm_buf_cnt = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops ||
        obj->mZslShotPushFlag == 0) {
        HAL_LOGE("mCameraHandle=%p, mHalOem=%p,", mCameraHandle, mHalOem);
        HAL_LOGE("obj->mZslShotPushFlag=%d", obj->mZslShotPushFlag);
        goto exit;
    }

    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    bzero(&zsl_frame, sizeof(struct camera_frame_type));
    bzero(&sw_algorithm_buf, sizeof(struct image_sw_algorithm_buf));

    if (mFrameSyncFlag == 1) {
        HAL_LOGD("wait for frame sync");
        // TBD: later will add select same timestamp of zsl with preview frame
        Mutex::Autolock l(mFrameSyncLock);
        mFrameSyncSignal.waitRelative(mFrameSyncLock, ZSL_FRAME_TIMEOUT);
        mFrameSyncFlag = 0;
    }

    // this is for real zsl flash capture, like sharkls/sharklt8, not
    // sharkl2-like
    // obj->skipZslFrameForFlashCapture();

    while (1) {
        sem_wait(&obj->mZslQueueSemDone);

        // for exception exit
        if (obj->mZslCaptureExitLoop == true) {
            HAL_LOGD("zsl loop exit done.");
            break;
        }

        zsl_frame = obj->popZslFrame();

        // for 3dnr sw 2.0
        if (sprddefInfo.sprd_3dnr_enabled) {
            sw_algorithm_buf.height = zsl_frame.height;
            sw_algorithm_buf.width = zsl_frame.width;
            sw_algorithm_buf.fd = zsl_frame.fd;
            sw_algorithm_buf.format = zsl_frame.format;
            sw_algorithm_buf.y_vir_addr = zsl_frame.y_vir_addr;
            mHalOem->ops->image_sw_algorithm_processing(
                obj->mCameraHandle, &sw_algorithm_buf,
                SPRD_CAM_IMAGE_SW_ALGORITHM_3DNR, IMG_DATA_TYPE_YVU420);

            sw_algorithm_buf_cnt++;
            if (sw_algorithm_buf_cnt >= 5) {
                goto exit;
            }
            continue;
        }

        if (mMultiCameraMode == MODE_BLUR && mIsBlur2Zsl == true) {
            char prop[PROPERTY_VALUE_MAX];
            uint32_t ae_fps = 30;
            uint32_t delay_time = 200;
            property_get("persist.vendor.cam.blur3.zsl.time", prop, "200");
            if (atoi(prop) != 0) {
                delay_time = atoi(prop);
            }
            HAL_LOGV("delay_time=%d,ae_fps=%d", delay_time, ae_fps);
            if (mZslSnapshotTime > zsl_frame.timestamp ||
                ((mZslSnapshotTime < zsl_frame.timestamp) &&
                 (((zsl_frame.timestamp - mZslSnapshotTime) / 1000000) <
                  delay_time))) {
                mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
                continue;
            }

            HAL_LOGD("mode blur fd=0x%x", zsl_frame.fd);
            mHalOem->ops->camera_set_zsl_snapshot_buffer(
                obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            break;
        }

        if (mMultiCameraMode == MODE_BOKEH) {
            HAL_LOGD("mode_bokeh fd=0x%x", zsl_frame.fd);
            mHalOem->ops->camera_set_zsl_snapshot_buffer(
                obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            break;
        }

        if (mZslSnapshotTime > zsl_frame.timestamp) {
            diff_ms = (mZslSnapshotTime - zsl_frame.timestamp) / 1000000;
            HAL_LOGV("diff_ms=%" PRId64, diff_ms);
            // make single capture frame time > mZslSnapshotTime
            if (sprddefInfo.capture_mode == 1 ||
                diff_ms > ZSL_SNAPSHOT_THRESHOLD_TIME) {
                HAL_LOGD("not the right frame, skip it");
                mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
                continue;
            }
        }

        // single capture wait the caf focused frame
        if (sprddefInfo.capture_mode == 1 && obj->mLastCafDoneTime > 0 &&
            zsl_frame.timestamp < obj->mLastCafDoneTime) {
            HAL_LOGD("not the focused frame, skip it");
            mHalOem->ops->camera_set_zsl_buffer(
                obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            continue;
        }

        HAL_LOGD("fd=0x%x", zsl_frame.fd);
        mHalOem->ops->camera_set_zsl_snapshot_buffer(
            obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
            zsl_frame.fd);
        break;
    }

exit:
    obj->mZslShotPushFlag = 0;
    obj->mFlashCaptureFlag = 0;
}

void SprdCamera3OEMIf::processZslSnapshot(void *p_data) {

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    uint32_t ret = 0;
    int64_t tmp1, tmp2;
    SPRD_DEF_Tag sprddefInfo;
    mSetting->getSPRDDEFTag(&sprddefInfo);

    HAL_LOGI("E");

    if (NULL == obj->mCameraHandle || NULL == obj->mHalOem ||
        NULL == obj->mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("in error status, deinit capture at first ");
        deinitCapture(mIsPreAllocCapMem);
    }

    mZslSnapshotTime = systemTime();

    if (isCapturing()) {
        WaitForCaptureDone();
    }

    if (isPreviewing()) {
        // whether FRONT_CAMERA_FLASH_TYPE is lcd
        bool isFrontLcd =
            (strcmp(FRONT_CAMERA_FLASH_TYPE, "lcd") == 0) ? true : false;
        // whether FRONT_CAMERA_FLASH_TYPE is flash
        bool isFrontFlash =
            (strcmp(FRONT_CAMERA_FLASH_TYPE, "flash") == 0) ? true : false;
        if (mCameraId == 0 || isFrontLcd || isFrontFlash) {
            obj->mHalOem->ops->camera_start_preflash(obj->mCameraHandle);
        }
        obj->mHalOem->ops->camera_snapshot_is_need_flash(
            obj->mCameraHandle, mCameraId, &mFlashCaptureFlag);
    }

    SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_SHOT_NUM,
             mPicCaptureCnt);

    LENS_Tag lensInfo;
    mSetting->getLENSTag(&lensInfo);
    if (lensInfo.focal_length) {
        SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                 (int32_t)(lensInfo.focal_length * 1000));
    }

    JPEG_Tag jpgInfo;
    struct img_size jpeg_thumb_size;
    mSetting->getJPEGTag(&jpgInfo);
    SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_JPEG_QUALITY,
             jpgInfo.quality);
    SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_THUMB_QUALITY,
             jpgInfo.thumbnail_quality);
    jpeg_thumb_size.width = jpgInfo.thumbnail_size[0];
    jpeg_thumb_size.height = jpgInfo.thumbnail_size[1];
    SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);

    // the following code will cause preview stuck, should be removed later
    if (SprdCamera3Setting::mSensorFocusEnable[mCameraId]) {
        if (getMultiCameraMode() == MODE_BLUR && isNeedAfFullscan()) {
            tmp1 = systemTime();
            SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_AF_MODE,
                     CAMERA_FOCUS_MODE_FULLSCAN);
            tmp2 = systemTime();
            HAL_LOGD("wait caf cost %" PRId64 " ms", (tmp2 - tmp1) / 1000000);
            /*after caf picture, set af mode again to isp*/
            SetCameraParaTag(ANDROID_CONTROL_AF_MODE);
        }
    }

    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
    ret = obj->mHalOem->ops->camera_take_picture(obj->mCameraHandle,
                                                 obj->mCaptureMode);
    if (ret) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        HAL_LOGE("fail to camera_take_picture");
        goto exit;
    }

    // for offline zsl
    if (mSprdZslEnabled == 1 && mVideoSnapshotType == 0) {
        mFlagOffLineZslStart = 1;
    }

    HAL_LOGD("mFlashCaptureFlag=%d, thumb=%dx%d, mZslShotPushFlag=%d",
             obj->mFlashCaptureFlag, jpgInfo.thumbnail_size[0],
             jpgInfo.thumbnail_size[1], obj->mZslShotPushFlag);
    HAL_LOGV("jpgInfo.quality=%d, thumb_quality=%d, focal_length=%f",
             jpgInfo.quality, jpgInfo.thumbnail_quality, lensInfo.focal_length);

    snapshotZsl(p_data);

exit:
    HAL_LOGI("X");
}

int SprdCamera3OEMIf::ZSLMode_monitor_thread_init(void *p_data) {
    CMR_MSG_INIT(message);
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj->mZSLModeMonitorInited) {
        ret = cmr_thread_create((cmr_handle *)&obj->mZSLModeMonitorMsgQueHandle,
                                ZSLMode_MONITOR_QUEUE_SIZE,
                                ZSLMode_monitor_thread_proc, (void *)obj);
        if (ret) {
            HAL_LOGE("create send msg failed!");
            return ret;
        }
        obj->mZSLModeMonitorInited = 1;
        sem_init(&obj->mZslQueueSemDone, 0, 0);

        message.msg_type = CMR_EVT_ZSL_MON_INIT;
        message.sync_flag = CMR_MSG_SYNC_RECEIVED;
        ret = cmr_thread_msg_send((cmr_handle)obj->mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("send msg failed!");
            return ret;
        }
    }
    return ret;
}

int SprdCamera3OEMIf::ZSLMode_monitor_thread_deinit(void *p_data) {
    CMR_MSG_INIT(message);
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (obj->mZSLModeMonitorInited) {
        message.msg_type = CMR_EVT_ZSL_MON_EXIT;
        message.sync_flag = CMR_MSG_SYNC_PROCESSED;
        ret = cmr_thread_msg_send((cmr_handle)obj->mZSLModeMonitorMsgQueHandle,
                                  &message);
        if (ret) {
            HAL_LOGE("send msg failed!");
        }

        ret = cmr_thread_destroy((cmr_handle)obj->mZSLModeMonitorMsgQueHandle);
        obj->mZSLModeMonitorMsgQueHandle = 0;
        obj->mZSLModeMonitorInited = 0;
        sem_destroy(&obj->mZslQueueSemDone);
    }
    return ret;
}

cmr_int SprdCamera3OEMIf::ZSLMode_monitor_thread_proc(struct cmr_msg *message,
                                                      void *p_data) {
    int exit_flag = 0;
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    cmr_int need_pause;

    HAL_LOGD("zsl thread message.msg_type 0x%x, sub-type 0x%x, ret %d",
             message->msg_type, message->sub_msg_type, ret);

    switch (message->msg_type) {
    case CMR_EVT_ZSL_MON_INIT:
        HAL_LOGD("zsl thread msg init");
        break;
    case CMR_EVT_ZSL_MON_SNP:
        obj->processZslSnapshot(p_data);
        break;
    case CMR_EVT_ZSL_MON_STOP_OFFLINE_PATH:
        ret = obj->mHalOem->ops->camera_stop_capture(obj->mCameraHandle);
        if (ret) {
            HAL_LOGE("camera_stop_capture failedd");
        }
        obj->mFlagOffLineZslStart = 0;
        break;
    case CMR_EVT_ZSL_MON_EXIT:
        HAL_LOGD("zsl thread msg exit");
        exit_flag = 1;
        break;
    default:
        HAL_LOGE("unsupported zsl message");
        break;
    }

    if (exit_flag) {
        HAL_LOGD("zsl monitor thread exit ");
    }
    return ret;
}

bool SprdCamera3OEMIf::isVideoCopyFromPreview() {
    return mVideoCopyFromPreviewFlag;
}

uint32_t SprdCamera3OEMIf::isPreAllocCapMem() {
    if (0 == mIsPreAllocCapMem) {
        return 0;
    } else {
        return 1;
    }
}

int SprdCamera3OEMIf::pre_alloc_cap_mem_thread_init(void *p_data) {
    int ret = NO_ERROR;
    pthread_attr_t attr;

    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj) {
        HAL_LOGE("obj null  error");
        return -1;
    }

    HAL_LOGD("inited=%d", obj->mPreAllocCapMemInited);

    if (!obj->mPreAllocCapMemInited) {
        obj->mPreAllocCapMemInited = 1;
        obj->mIsPreAllocCapMemDone = 0;
        sem_init(&obj->mPreAllocCapMemSemDone, 0, 0);
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&obj->mPreAllocCapMemThread, &attr,
                             pre_alloc_cap_mem_thread_proc, (void *)obj);
        pthread_attr_destroy(&attr);
        if (ret) {
            obj->mPreAllocCapMemInited = 0;
            sem_destroy(&obj->mPreAllocCapMemSemDone);
            HAL_LOGE("fail to send init msg");
        }
    }

    return ret;
}

int SprdCamera3OEMIf::pre_alloc_cap_mem_thread_deinit(void *p_data) {
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj) {
        HAL_LOGE("obj null  error");
        return -1;
    }

    HAL_LOGD("inited=%d", obj->mPreAllocCapMemInited);

    if (obj->mPreAllocCapMemInited) {
        sem_wait(&obj->mPreAllocCapMemSemDone);
        sem_destroy(&obj->mPreAllocCapMemSemDone);
        obj->mPreAllocCapMemInited = 0;
        obj->mIsPreAllocCapMemDone = 0;
    }
    return ret;
}

void *SprdCamera3OEMIf::pre_alloc_cap_mem_thread_proc(void *p_data) {
    cmr_u32 mem_size = 0;
    int32_t buffer_id = 0;
    cmr_u32 sum = 0;
    int ret = 0;
    cmr_uint phy_addr, virt_addr;
    cmr_s32 fd;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    HAL_LOGD("E");

    if (!obj) {
        HAL_LOGE("obj=%p", obj);
        return NULL;
    }

    if (NULL == obj->mHalOem || NULL == obj->mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return NULL;
    }

    ret = obj->mHalOem->ops->camera_get_postprocess_capture_size(obj->mCameraId,
                                                                 &mem_size);
    if (ret) {
        HAL_LOGE("camera_get_postprocess_capture_size failed");
        obj->mIsPreAllocCapMem = 0;
        goto exit;
    }

    obj->mSubRawHeapSize = mem_size;
    sum = 1;
    ret =
        obj->Callback_CaptureMalloc(mem_size, sum, &phy_addr, &virt_addr, &fd);
    if (ret) {
        obj->mIsPreAllocCapMem = 0;
        HAL_LOGE("Callback_CaptureMalloc failed");
        goto exit;
    }

    obj->mIsPreAllocCapMemDone = 1;

exit:
    sem_post(&obj->mPreAllocCapMemSemDone);

    HAL_LOGD("X");

    return NULL;
}

void SprdCamera3OEMIf::setSensorCloseFlag() {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    mHalOem->ops->camera_set_sensor_close_flag(mCameraHandle);
}

bool SprdCamera3OEMIf::isIspToolMode() { return mIsIspToolMode; }

void SprdCamera3OEMIf::ispToolModeInit() {
    cmr_handle isp_handle = 0;
    mHalOem->ops->camera_get_isp_handle(mCameraHandle, &isp_handle);
    setispserver(isp_handle);
}

int32_t SprdCamera3OEMIf::setStreamOnWithZsl() {
    mStreamOnWithZsl = 1;
    return 0;
}

int32_t SprdCamera3OEMIf::getStreamOnWithZsl() { return mStreamOnWithZsl; }

int32_t SprdCamera3OEMIf::setFrameSyncFlag(uint32_t frameNum) {
    mFrameSyncFlag = 1;
    mFrameSyncNum = frameNum;
    HAL_LOGD("mFrameSyncNum=%d", mFrameSyncNum);
    return 0;
}

#ifdef CONFIG_CAMERA_EIS
void SprdCamera3OEMIf::EisPreview_init() {
    int i = 0;
    int num = sizeof(eis_init_info_tab) / sizeof(sprd_eis_init_info_t);
    video_stab_param_default(&mPreviewParam);
    mPreviewParam.src_w = (uint16_t)mPreviewWidth;
    mPreviewParam.src_h = (uint16_t)mPreviewHeight;
    mPreviewParam.dst_w = (uint16_t)mPreviewWidth * 5 / 6;
    mPreviewParam.dst_h = (uint16_t)mPreviewHeight * 5 / 6;
    mPreviewParam.method = 0;
    mPreviewParam.wdx = 0;
    mPreviewParam.wdy = 0;
    mPreviewParam.wdz = 0;
    // f,td,ts is different in each project.
    mPreviewParam.f = 1230.0f;
    mPreviewParam.td = 0.0001f;
    mPreviewParam.ts = 0.0001f;
    // EIS parameter depend on board version
    for (i = 0; i < num; i++) {
        if (strcmp(eis_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
            0) {
            mPreviewParam.f = eis_init_info_tab[i].f;   // 1230;
            mPreviewParam.td = eis_init_info_tab[i].td; // 0.004;
            mPreviewParam.ts = eis_init_info_tab[i].ts; // 0.021;
        }
    }
    HAL_LOGI("mParam f: %lf, td:%lf, ts:%lf", mPreviewParam.f, mPreviewParam.td,
             mPreviewParam.ts);
    video_stab_open(&mPreviewInst, &mPreviewParam);
    HAL_LOGI("mParam src_w: %d, src_h:%d, dst_w:%d, dst_h:%d",
             mPreviewParam.src_w, mPreviewParam.src_h, mPreviewParam.dst_w,
             mPreviewParam.dst_h);
}

void SprdCamera3OEMIf::EisVideo_init() {
    // mParam = {0};
    int i = 0;
    int num = sizeof(eis_init_info_tab) / sizeof(sprd_eis_init_info_t);
    video_stab_param_default(&mVideoParam);
    mVideoParam.src_w = (uint16_t)mVideoWidth;
    mVideoParam.src_h = (uint16_t)mVideoHeight;
    mVideoParam.dst_w = (uint16_t)mVideoWidth * 5 / 6;
    mVideoParam.dst_h = (uint16_t)mVideoHeight * 5 / 6;
    mVideoParam.method = 1;
    mVideoParam.wdx = 0;
    mVideoParam.wdy = 0;
    mVideoParam.wdz = 0;
    // f,td,ts is different in each project.
    mVideoParam.f = 1230.0f;
    mVideoParam.td = 0.0001f;
    mVideoParam.ts = 0.0001f;
    // EIS parameter depend on board version
    for (i = 0; i < num; i++) {
        if (strcmp(eis_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
            0) {
            mVideoParam.f = eis_init_info_tab[i].f;   // 1230;
            mVideoParam.td = eis_init_info_tab[i].td; // 0.004;
            mVideoParam.ts = eis_init_info_tab[i].ts; // 0.021;
        }
    }
    HAL_LOGI("mParam f: %lf, td:%lf, ts:%lf", mVideoParam.f, mVideoParam.td,
             mVideoParam.ts);
    video_stab_open(&mVideoInst, &mVideoParam);
    HAL_LOGI("mParam src_w: %d, src_h:%d, dst_w:%d, dst_h:%d",
             mVideoParam.src_w, mVideoParam.src_h, mVideoParam.dst_w,
             mVideoParam.dst_h);
}

vsOutFrame SprdCamera3OEMIf::processPreviewEIS(vsInFrame frame_in) {
    ATRACE_CALL();

    int gyro_num = 0;
    int ret_eis = 1;
    uint count = 0;
    vsGyro *gyro = NULL;
    vsOutFrame frame_out_preview;
    memset(&frame_out_preview, 0x00, sizeof(vsOutFrame));

    if (mGyromaxtimestamp) {
        HAL_LOGI("in frame timestamp: %lf, mGyromaxtimestamp %lf",
                 frame_in.timestamp, mGyromaxtimestamp);
        video_stab_write_frame(mPreviewInst, &frame_in);
        do {
            gyro_num = mGyroPreviewInfo.size();
            if (gyro_num) {
                gyro = (vsGyro *)malloc(gyro_num * sizeof(vsGyro));
                if (NULL == gyro) {
                    HAL_LOGE(" malloc gyro buffer is fail");
                    break;
                }
                memset(gyro, 0, gyro_num * sizeof(vsGyro));
                popEISPreviewQueue(gyro, gyro_num);
                video_stab_write_gyro(mPreviewInst, gyro, gyro_num);
                if (gyro) {
                    free(gyro);
                    gyro = NULL;
                }
            }
            ret_eis = video_stab_check_gyro(mPreviewInst);
            if (ret_eis == 0) {
                HAL_LOGD("gyro is ready ");
                break;
            } else {
                ret_eis = 1;
                HAL_LOGD("gyro is NOT ready,check  gyro again");
            }
            if (++count >= 4 || (NO_ERROR !=
                                 mReadGyroPreviewCond.waitRelative(
                                     mReadGyroPreviewLock, 30000000))) {
                HAL_LOGW("gyro data is too slow for eis process");
                break;
            }
        } while (ret_eis == 1);

        ret_eis = video_stab_read(mPreviewInst, &frame_out_preview);
        if (ret_eis == 0)
            HAL_LOGD("out frame_num =%d,frame timestamp %lf, frame_out %p",
                     frame_out_preview.frame_num, frame_out_preview.timestamp,
                     frame_out_preview.frame_data);
        else
            HAL_LOGD("no frame out");
    } else
        HAL_LOGD("no gyro data to process EIS");
    return frame_out_preview;
}

vsOutFrame SprdCamera3OEMIf::processVideoEIS(vsInFrame frame_in) {
    ATRACE_CALL();

    int gyro_num = 0;
    int ret_eis = 1;
    uint count = 0;
    vsGyro *gyro = NULL;
    vsOutFrame frame_out_video;
    memset(&frame_out_video, 0x00, sizeof(vsOutFrame));

    if (mGyromaxtimestamp) {
        HAL_LOGI("in frame timestamp: %lf, mGyromaxtimestamp %lf",
                 frame_in.timestamp, mGyromaxtimestamp);
        video_stab_write_frame(mVideoInst, &frame_in);
        do {
            gyro_num = mGyroVideoInfo.size();
            HAL_LOGV("gyro_num = %d", gyro_num);
            if (gyro_num) {
                gyro = (vsGyro *)malloc(gyro_num * sizeof(vsGyro));
                if (NULL == gyro) {
                    HAL_LOGE(" malloc gyro buffer is fail");
                    break;
                }
                memset(gyro, 0, gyro_num * sizeof(vsGyro));
                popEISVideoQueue(gyro, gyro_num);
                video_stab_write_gyro(mVideoInst, gyro, gyro_num);
                if (gyro) {
                    free(gyro);
                    gyro = NULL;
                }
            }
            ret_eis = video_stab_check_gyro(mVideoInst);
            if (ret_eis == 0) {
                HAL_LOGD("gyro is ready ");
                break;
            } else {
                ret_eis = 1;
                HAL_LOGE("gyro is NOT ready,check  gyro again");
            }
            if (++count >= 4 || (NO_ERROR !=
                                 mReadGyroVideoCond.waitRelative(
                                     mReadGyroVideoLock, 30000000))) {
                HAL_LOGW("gyro data is too slow for eis process");
                break;
            }
        } while (ret_eis == 1);
        ret_eis = video_stab_read(mVideoInst, &frame_out_video);
        if (ret_eis == 0)
            HAL_LOGD("out frame_num =%d,frame timestamp %lf, frame_out %p",
                     frame_out_video.frame_num, frame_out_video.timestamp,
                     frame_out_video.frame_data);
        else
            HAL_LOGD("no frame out");
    } else
        HAL_LOGD("no gyro data to process EIS");
    return frame_out_video;
}

void SprdCamera3OEMIf::pushEISPreviewQueue(vsGyro *mGyrodata) {
    Mutex::Autolock l(&mEisPreviewLock);

    mGyroPreviewInfo.push_back(mGyrodata);
}

void SprdCamera3OEMIf::popEISPreviewQueue(vsGyro *gyro, int gyro_num) {
    Mutex::Autolock l(&mEisPreviewLock);

    int i;
    vsGyro *GyroInfo = NULL;
    for (i = 0; i < gyro_num; i++) {
        if (*mGyroPreviewInfo.begin() != NULL) {
            GyroInfo = *mGyroPreviewInfo.begin();
            gyro[i].t = GyroInfo->t / 1000000000;
            gyro[i].w[0] = GyroInfo->w[0];
            gyro[i].w[1] = GyroInfo->w[1];
            gyro[i].w[2] = GyroInfo->w[2];
            HAL_LOGV("gyro i %d,timestamp %lf, x: %lf, y: %lf, z: %lf", i,
                     gyro[i].t, gyro[i].w[0], gyro[i].w[1], gyro[i].w[2]);
        } else
            HAL_LOGW("gyro data is null in %d", i);
        mGyroPreviewInfo.erase(mGyroPreviewInfo.begin());
    }
}

void SprdCamera3OEMIf::pushEISVideoQueue(vsGyro *mGyrodata) {
    Mutex::Autolock l(&mEisVideoLock);

    mGyroVideoInfo.push_back(mGyrodata);
}

void SprdCamera3OEMIf::popEISVideoQueue(vsGyro *gyro, int gyro_num) {
    Mutex::Autolock l(&mEisVideoLock);

    int i;
    vsGyro *GyroInfo = NULL;
    for (i = 0; i < gyro_num; i++) {
        if (*mGyroVideoInfo.begin() != NULL) {
            GyroInfo = *mGyroVideoInfo.begin();
            gyro[i].t = GyroInfo->t / 1000000000;
            gyro[i].w[0] = GyroInfo->w[0];
            gyro[i].w[1] = GyroInfo->w[1];
            gyro[i].w[2] = GyroInfo->w[2];
            HAL_LOGV("gyro i %d,timestamp %lf, x: %lf, y: %lf, z: %lf", i,
                     gyro[i].t, gyro[i].w[0], gyro[i].w[1], gyro[i].w[2]);
        } else
            HAL_LOGW("gyro data is null in %d", i);
        mGyroVideoInfo.erase(mGyroVideoInfo.begin());
    }
}

void SprdCamera3OEMIf::EisPreviewFrameStab(struct camera_frame_type *frame) {
    vsInFrame frame_in;
    vsOutFrame frame_out;
    frame_in.frame_data = NULL;
    frame_out.frame_data = NULL;
    EIS_CROP_Tag eiscrop_Info;
    int64_t boot_time = frame->monoboottime;
    int64_t ae_time = frame->ae_time;
    uintptr_t buff_vir = (uintptr_t)(frame->y_vir_addr);
    // int64_t sleep_time = boot_time - buffer_timestamp;
    memset(&eiscrop_Info, 0x00, sizeof(EIS_CROP_Tag));

    if (mGyroInit && !mGyroExit) {
        if (frame->zoom_ratio == 0)
            frame->zoom_ratio = 1.0f;
        float zoom_ratio = frame->zoom_ratio;
        HAL_LOGV("boot_time = %" PRId64, boot_time);
        HAL_LOGV("ae_time = %" PRId64 ", zoom_ratio = %f", ae_time, zoom_ratio);
        frame_in.frame_data = (uint8_t *)buff_vir;
        frame_in.timestamp = (double)(boot_time - ae_time / 2);
        frame_in.timestamp = frame_in.timestamp / 1000000000;
        frame_in.zoom = (double)zoom_ratio;
        frame_out = processPreviewEIS(frame_in);
        HAL_LOGD(
            "transfer_matrix wrap %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
            frame_out.warp.dat[0][0], frame_out.warp.dat[0][1],
            frame_out.warp.dat[0][2], frame_out.warp.dat[1][0],
            frame_out.warp.dat[1][1], frame_out.warp.dat[1][2],
            frame_out.warp.dat[2][0], frame_out.warp.dat[2][1],
            frame_out.warp.dat[2][2]);

        double crop_start_w =
            frame_out.warp.dat[0][2] + mPreviewParam.src_w / 12;
        double crop_start_h =
            frame_out.warp.dat[1][2] + mPreviewParam.src_h / 12;
        eiscrop_Info.crop[0] = (int)(crop_start_w + 0.5);
        eiscrop_Info.crop[1] = (int)(crop_start_h + 0.5);
        eiscrop_Info.crop[2] = (int)(crop_start_w + 0.5) + mPreviewParam.dst_w;
        eiscrop_Info.crop[3] = (int)(crop_start_h + 0.5) + mPreviewParam.dst_h;
        mSetting->setEISCROPTag(eiscrop_Info);
    } else {
        HAL_LOGW("gyro is not enable, eis process is not work");
        eiscrop_Info.crop[0] = 0;
        eiscrop_Info.crop[1] = 0;
        eiscrop_Info.crop[2] = mPreviewWidth;
        eiscrop_Info.crop[3] = mPreviewHeight;
        mSetting->setEISCROPTag(eiscrop_Info);
    }
}

vsOutFrame SprdCamera3OEMIf::EisVideoFrameStab(struct camera_frame_type *frame,
                                               uint32_t frame_num) {
    vsInFrame frame_in;
    vsOutFrame frame_out;
    frame_in.frame_data = NULL;
    EIS_CROP_Tag eiscrop_Info;
    int64_t boot_time = frame->monoboottime;
    int64_t ae_time = frame->ae_time;
    uintptr_t buff_vir = (uintptr_t)(frame->y_vir_addr);

    memset(&frame_out, 0x00, sizeof(vsOutFrame));
    memset(&eiscrop_Info, 0x00, sizeof(EIS_CROP_Tag));

    // gyro data is should be used for video frame stab
    if (mGyroInit && !mGyroExit) {
        if (frame->zoom_ratio == 0)
            frame->zoom_ratio = 1.0f;
        float zoom_ratio = frame->zoom_ratio;
        HAL_LOGV("boot_time = %lld,ae_time =%lld,zoom_ratio = %f", boot_time,
                 ae_time, zoom_ratio);
        frame_in.frame_data = (uint8_t *)buff_vir;
        frame_in.timestamp = (double)(boot_time - ae_time / 2);
        frame_in.timestamp = frame_in.timestamp / 1000000000;
        frame_in.zoom = (double)zoom_ratio;
        frame_in.frame_num = frame_num;
        frame_out = processVideoEIS(frame_in);
        if (frame_out.frame_data)
            HAL_LOGD("transfer_matrix wrap %lf, %lf, %lf, %lf, %lf, %lf, %lf, "
                     "%lf, %lf",
                     frame_out.warp.dat[0][0], frame_out.warp.dat[0][1],
                     frame_out.warp.dat[0][2], frame_out.warp.dat[1][0],
                     frame_out.warp.dat[1][1], frame_out.warp.dat[1][2],
                     frame_out.warp.dat[2][0], frame_out.warp.dat[2][1],
                     frame_out.warp.dat[2][2]);
    } else {
        HAL_LOGD("gyro is not enable, eis process is not work");
    }
    if (frame_out.frame_data) {
        char *eis_buff = (char *)(frame_out.frame_data) +
                         frame->width * frame->height * 3 / 2;
        double *warp_buff = (double *)eis_buff;
        HAL_LOGV("video vir address 0x%lx,warp address %p",
                 frame_out.frame_data, warp_buff);
        if (warp_buff) {
            *warp_buff++ = 16171225;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (warp_buff)
                        *warp_buff++ = frame_out.warp.dat[i][j];
                }
            }
        }
    }

    return frame_out;
}
#endif

#ifdef CONFIG_CAMERA_GYRO
int SprdCamera3OEMIf::gyro_monitor_thread_init(void *p_data) {
    int ret = NO_ERROR;
    pthread_attr_t attr;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj) {
        HAL_LOGE("obj null  error");
        return -1;
    }

    HAL_LOGD("E inited=%d", obj->mGyroInit);

    if (!obj->mGyroInit) {
        obj->mGyroInit = 1;
        sem_init(&obj->mGyro_sem, 0, 0);
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&obj->mGyroMsgQueHandle, &attr,
                             gyro_monitor_thread_proc, (void *)obj);
        pthread_setname_np(obj->mGyroMsgQueHandle, "gyro");
        pthread_attr_destroy(&attr);
        if (ret) {
            obj->mGyroInit = 0;
            sem_destroy(&obj->mGyro_sem);
            HAL_LOGE("fail to init gyro thread");
        }
    }

    return ret;
}

int SprdCamera3OEMIf::gyro_get_data(
    void *p_data, ASensorEvent *buffer, int n,
    struct cmr_af_aux_sensor_info *sensor_info) {
    int ret = 0;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    if (!obj) {
        HAL_LOGE("obj null  error");
        ret = -1;
        return ret;
    }
    for (int i = 0; i < n; i++) {
        memset((void *)sensor_info, 0, sizeof(*sensor_info));
        switch (buffer[i].type) {
        case Sensor::TYPE_GYROSCOPE: {
#ifdef CONFIG_CAMERA_EIS
            // push gyro data for eis preview & video queue
            SPRD_DEF_Tag sprddefInfo;
            obj->mSetting->getSPRDDEFTag(&sprddefInfo);
            if (obj->mRecordingMode && sprddefInfo.sprd_eis_enabled) {
                struct timespec t1;
                clock_gettime(CLOCK_BOOTTIME, &t1);
                nsecs_t time1 = (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
                HAL_LOGV("mGyroCamera CLOCK_BOOTTIME =%" PRId64, time1);
                obj->mGyrodata[obj->mGyroNum].t = buffer[i].timestamp;
                obj->mGyrodata[obj->mGyroNum].w[0] = buffer[i].data[0];
                obj->mGyrodata[obj->mGyroNum].w[1] = buffer[i].data[1];
                obj->mGyrodata[obj->mGyroNum].w[2] = buffer[i].data[2];
                obj->mGyromaxtimestamp =
                    obj->mGyrodata[obj->mGyroNum].t / 1000000000;
                obj->pushEISPreviewQueue(&obj->mGyrodata[obj->mGyroNum]);
                obj->mReadGyroPreviewCond.signal();
                if (obj->mIsRecording) {
                    obj->pushEISVideoQueue(&obj->mGyrodata[obj->mGyroNum]);
                    obj->mReadGyroVideoCond.signal();
                }
                if (++obj->mGyroNum >= obj->kGyrocount)
                    obj->mGyroNum = 0;
                HAL_LOGV("gyro timestamp %" PRId64 ", x: %f, y: %f, z: %f",
                         buffer[i].timestamp, buffer[i].data[0],
                         buffer[i].data[1], buffer[i].data[2]);
            }
#endif

            sensor_info->type = CAMERA_AF_GYROSCOPE;
            sensor_info->gyro_info.timestamp = buffer[i].timestamp;
            sensor_info->gyro_info.x = buffer[i].data[0];
            sensor_info->gyro_info.y = buffer[i].data[1];
            sensor_info->gyro_info.z = buffer[i].data[2];
            if (NULL != obj->mCameraHandle &&
                SPRD_IDLE == obj->mCameraState.capture_state &&
                NULL != obj->mHalOem) {
                obj->mHalOem->ops->camera_set_sensor_info_to_af(
                    obj->mCameraHandle, sensor_info);
            }
            break;
        }

        case Sensor::TYPE_ACCELEROMETER: {
            sensor_info->type = CAMERA_AF_ACCELEROMETER;
            sensor_info->gsensor_info.timestamp = buffer[i].timestamp;
            sensor_info->gsensor_info.vertical_up = buffer[i].data[0];
            sensor_info->gsensor_info.vertical_down = buffer[i].data[1];
            sensor_info->gsensor_info.horizontal = buffer[i].data[2];
            HAL_LOGV("gsensor timestamp %" PRId64 ", x: %f, y: %f, z: %f",
                     buffer[i].timestamp, buffer[i].data[0], buffer[i].data[1],
                     buffer[i].data[2]);
            if (NULL != obj->mCameraHandle && NULL != obj->mHalOem) {
                obj->mHalOem->ops->camera_set_sensor_info_to_af(
                    obj->mCameraHandle, sensor_info);
            }
            break;
        }

        default:
            break;
        }
    }
    return ret;
}

#ifdef CONFIG_SPRD_ANDROID_8

void *SprdCamera3OEMIf::gyro_ASensorManager_process(void *p_data) {
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    ASensorManager *mSensorManager;
    int mNumSensors;
    ASensorList mSensorList;
    uint32_t GyroRate = 10 * 1000;    // us
    uint32_t GsensorRate = 50 * 1000; // us
    uint32_t delayTime = 10 * 1000;   // us
    uint32_t Gyro_flag = 0;
    uint32_t Gsensor_flag = 0;
    ASensorEvent buffer[8];
    ssize_t n;

    HAL_LOGD("E");
    if (!obj) {
        HAL_LOGE("obj null  error");
        return NULL;
    }
    mSensorManager = ASensorManager_getInstanceForPackage("");
    if (mSensorManager == NULL) {
        HAL_LOGE("can not get ISensorManager service");
        sem_post(&obj->mGyro_sem);
        obj->mGyroExit = 1;
        return NULL;
    }
    mNumSensors = ASensorManager_getSensorList(mSensorManager, &mSensorList);
    const ASensor *accelerometerSensor = ASensorManager_getDefaultSensor(
        mSensorManager, ASENSOR_TYPE_ACCELEROMETER);
    const ASensor *gyroSensor =
        ASensorManager_getDefaultSensor(mSensorManager, ASENSOR_TYPE_GYROSCOPE);
    ALooper *mLooper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ASensorEventQueue *sensorEventQueue =
        ASensorManager_createEventQueue(mSensorManager, mLooper, 0, NULL, NULL);

    if (accelerometerSensor != NULL) {
        if (ASensorEventQueue_registerSensor(
                sensorEventQueue, accelerometerSensor, GsensorRate, 0) < 0) {
            HAL_LOGE(
                "Unable to register sensorUnable to register sensor %d with "
                "rate %d and report latency %d" PRId64 "",
                ASENSOR_TYPE_ACCELEROMETER, GsensorRate, 0);
            goto exit;
        } else {
            Gsensor_flag = 1;
        }
    }
    if (gyroSensor != NULL) {
        if (ASensorEventQueue_registerSensor(sensorEventQueue, gyroSensor,
                                             GyroRate, 0) < 0) {
            HAL_LOGE(
                "Unable to register sensorUnable to register sensor %d with "
                "rate %d and report latency %d" PRId64 "",
                ASENSOR_TYPE_GYROSCOPE, GyroRate, 0);
            goto exit;
        } else {
            Gyro_flag = 1;
        }
    }
    struct cmr_af_aux_sensor_info sensor_info;
    while (true == obj->mGyroInit) {
        if ((n = ASensorEventQueue_getEvents(sensorEventQueue, buffer, 8)) >
            0) {
            gyro_get_data(p_data, buffer, n, &sensor_info);
        }
        usleep(delayTime);
    }

    if (Gsensor_flag) {
        ASensorEventQueue_disableSensor(sensorEventQueue, accelerometerSensor);
        Gsensor_flag = 0;
    }
    if (Gyro_flag) {
        ASensorEventQueue_disableSensor(sensorEventQueue, gyroSensor);
        Gyro_flag = 0;
    }

exit:
    ASensorManager_destroyEventQueue(mSensorManager, sensorEventQueue);
    sem_post(&obj->mGyro_sem);
    obj->mGyroExit = 1;
    HAL_LOGD("X");
    return NULL;
}

#else
void *SprdCamera3OEMIf::gyro_SensorManager_process(void *p_data) {
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    uint32_t ret = 0;
    int32_t result = 0;
    int events;
    uint32_t GyroRate = 10 * 1000;    //  us
    uint32_t GsensorRate = 50 * 1000; // us
    uint32_t Gyro_flag = 0;
    uint32_t Gsensor_flag = 0;
    uint32_t Timeout = 100; // ms
    ASensorEvent buffer[8];
    ssize_t n;

    HAL_LOGE("E");
    if (!obj) {
        HAL_LOGE("obj null  error");
        return NULL;
    }
    SensorManager &mgr(
        SensorManager::getInstanceForPackage(String16("EIS intergrate")));

    Sensor const *const *list;
    ssize_t count = mgr.getSensorList(&list);
    sp<SensorEventQueue> q = mgr.createEventQueue();
    Sensor const *gyroscope = mgr.getDefaultSensor(Sensor::TYPE_GYROSCOPE);
    Sensor const *gsensor = mgr.getDefaultSensor(Sensor::TYPE_ACCELEROMETER);
    if (q == NULL) {
        HAL_LOGE("createEventQueue error");
        sem_post(&obj->mGyro_sem);
        obj->mGyroExit = 1;
        return NULL;
    }
    const int fd = q->getFd();
    HAL_LOGI("EventQueue fd %d", fd);
    sp<Looper> loop = NULL;

    if (gyroscope != NULL) {
        ret = q->enableSensor(gyroscope, GyroRate);
        if (ret)
            HAL_LOGE("enable gyroscope fail");
        else
            Gyro_flag = 1;
    } else
        HAL_LOGW("this device not support gyro");
    if (gsensor != NULL) {
        ret = q->enableSensor(gsensor, GsensorRate);
        if (ret) {
            HAL_LOGE("enable gsensor fail");
            if (Gyro_flag == 0)
                goto exit;
        } else
            Gsensor_flag = 1;
    } else {
        HAL_LOGW("this device not support Gsensor");
        if (Gyro_flag == 0)
            goto exit;
    }

    loop = new Looper(true);
    if (loop == NULL) {
        HAL_LOGE("creat loop fail");
        if (Gsensor_flag) {
            q->disableSensor(gsensor);
            Gsensor_flag = 0;
        }
        if (Gyro_flag) {
            q->disableSensor(gyroscope);
            Gyro_flag = 0;
        }
        goto exit;
    }
    loop->addFd(fd, fd, ALOOPER_EVENT_INPUT, NULL, NULL);
    struct cmr_af_aux_sensor_info sensor_info;
    while (true == obj->mGyroInit) {
        result = loop->pollOnce(Timeout, NULL, &events, NULL);
        if ((result == ALOOPER_POLL_ERROR) || (events & ALOOPER_EVENT_HANGUP)) {
            HAL_LOGE("SensorEventQueue::waitForEvent error");
            if (Gsensor_flag) {
                q->disableSensor(gsensor);
                Gsensor_flag = 0;
            }
            if (Gyro_flag) {
                q->disableSensor(gyroscope);
                Gyro_flag = 0;
            }
            goto exit;
        } else if (result == ALOOPER_POLL_TIMEOUT) {
            HAL_LOGW("SensorEventQueue::waitForEvent timeout");
        }
        if (!obj) {
            HAL_LOGE("obj is null,exit thread loop");
            if (Gsensor_flag) {
                q->disableSensor(gsensor);
                Gsensor_flag = 0;
            }
            if (Gyro_flag) {
                q->disableSensor(gyroscope);
                Gyro_flag = 0;
            }
            goto exit;
        }
        if ((result == fd) && (n = q->read(buffer, 8)) > 0) {
            gyro_get_data(p_data, buffer, n, &sensor_info);
        }
    }

exit:
    sem_post(&obj->mGyro_sem);
    obj->mGyroExit = 1;
    HAL_LOGE("X");
    return NULL;
}
#endif
void *SprdCamera3OEMIf::gyro_monitor_thread_proc(void *p_data) {
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    HAL_LOGD("E");
    if (!obj) {
        HAL_LOGE("obj null  error");
        return NULL;
    }

    if (NULL == obj->mCameraHandle || NULL == obj->mHalOem ||
        NULL == obj->mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        sem_post(&obj->mGyro_sem);
        obj->mGyroExit = 1;
        return NULL;
    }
    obj->mGyroNum = 0;
    obj->mGyromaxtimestamp = 0;
#ifdef CONFIG_CAMERA_EIS
    memset(obj->mGyrodata, 0, sizeof(obj->mGyrodata));
#endif

#ifdef CONFIG_SPRD_ANDROID_8
    gyro_ASensorManager_process(p_data);
#else
    gyro_SensorManager_process(p_data);
#endif
    HAL_LOGD("X");
    return NULL;
}

int SprdCamera3OEMIf::gyro_monitor_thread_deinit(void *p_data) {
    int ret = NO_ERROR;
    void *dummy;
    struct timespec ts;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    if (!obj) {
        HAL_LOGE("obj null error");
        return UNKNOWN_ERROR;
    }

    HAL_LOGD("E inited=%d, Deinit = %d", obj->mGyroInit, obj->mGyroExit);

    if (obj->mGyroInit) {
        obj->mGyroInit = 0;

        if (!obj->mGyroExit) {
            if (clock_gettime(CLOCK_REALTIME, &ts)) {
                HAL_LOGE("get time failed");
                return UNKNOWN_ERROR;
            }
            /*when gyro thread proc time is long when camera close, we should
             * wait for thread end at last 1000ms*/
            ts.tv_nsec += ms2ns(1000);
            if (ts.tv_nsec > 1000000000) {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000000000;
            }
            ret = sem_timedwait(&obj->mGyro_sem, &ts);
            if (ret)
                HAL_LOGW("wait for gyro timeout");
        } else
            HAL_LOGW("gyro thread already end");

        sem_destroy(&obj->mGyro_sem);
        // ret = pthread_join(obj->mGyroMsgQueHandle, &dummy);
        obj->mGyroMsgQueHandle = 0;

#ifdef CONFIG_CAMERA_EIS
        while (!obj->mGyroPreviewInfo.empty())
            obj->mGyroPreviewInfo.erase(obj->mGyroPreviewInfo.begin());
        while (!obj->mGyroVideoInfo.empty())
            obj->mGyroVideoInfo.erase(obj->mGyroVideoInfo.begin());
#endif
    }
    HAL_LOGD("X inited=%d, Deinit = %d", obj->mGyroInit, obj->mGyroExit);

    return ret;
}

#endif
}
