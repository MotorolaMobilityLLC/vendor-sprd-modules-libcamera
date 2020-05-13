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
#include "cmr_common.h"
#include "cam_log.h"
#include <cutils/properties.h>
#include <fcntl.h>
#include <math.h>
#include <media/hardware/MetadataBufferType.h>
#include <sprd_ion.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include <utils/Trace.h>
#ifdef SPRD_PERFORMANCE
#include <androidfw/SprdIlog.h>
#endif
#include <ui/GraphicBuffer.h>
#ifdef CAMERA_3DNR_CAPTURE_GPU
#include "gralloc_buffer_priv.h"
#ifdef CONFIG_GPU_PLATFORM_ROGUE
#include <gralloc_public.h>
#else
#include <gralloc_priv.h>
#endif
#endif
#include "SprdCamera3Channel.h"
#include "SprdCamera3Flash.h"
#include "SprdCamera3HALHeader.h"
#include "gralloc_public.h"
#include <dlfcn.h>

#include <cutils/ashmem.h>
#include <linux/ion.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include <sys/resource.h>
#include <cutils/sched_policy.h>
#include <system/thread_defs.h>

#include "libyuv/convert.h"
#include "libyuv/convert_from.h"
#include "libyuv/scale.h"

extern "C" {
#include "isp_video.h"
}
#ifdef CONFIG_FACE_BEAUTY
#include "sprd_facebeauty_adapter.h"
#endif
struct CAMERA_MEM_CB_TYPE_STAT mem_cb_stat[CAMERA_MEM_CB_TYPE_MAX] = {
    {CAMERA_PREVIEW, CACHE_TRUE},
    {CAMERA_SNAPSHOT, CACHE_TRUE},
    {CAMERA_SNAPSHOT_ZSL, CACHE_TRUE},
    {CAMERA_VIDEO, CACHE_TRUE},
    {CAMERA_PREVIEW_RESERVED, CACHE_TRUE},
    {CAMERA_SNAPSHOT_ZSL_RESERVED_MEM, CACHE_TRUE},
    {CAMERA_SNAPSHOT_ZSL_RESERVED, CACHE_TRUE},
    {CAMERA_VIDEO_RESERVED, CACHE_TRUE},
    {CAMERA_ISP_LSC, CACHE_FASLE},
    {CAMERA_ISP_BINGING4AWB, CACHE_FASLE},
    {CAMERA_SNAPSHOT_PATH, CACHE_TRUE},
    {CAMERA_ISP_FIRMWARE, CACHE_FASLE},
    {CAMERA_SNAPSHOT_HIGHISO, CACHE_TRUE},
    {CAMERA_ISP_RAW_DATA, CACHE_TRUE},
    {CAMERA_ISP_ANTI_FLICKER, CACHE_FASLE},
    {CAMERA_ISP_RAWAE, CACHE_FASLE},
    {CAMERA_ISP_PREVIEW_Y, CACHE_FASLE},
    {CAMERA_ISP_PREVIEW_YUV, CACHE_FASLE},
    {CAMERA_DEPTH_MAP, CACHE_TRUE},
    {CAMERA_DEPTH_MAP_RESERVED, CACHE_TRUE},
    {CAMERA_PDAF_RAW, CACHE_TRUE},
    {CAMERA_PDAF_RAW_RESERVED, CACHE_TRUE},
    {CAMERA_ISP_STATIS, CACHE_FASLE},
    {CAMERA_SNAPSHOT_3DNR, CACHE_TRUE},
    {CAMERA_SNAPSHOT_3DNR_DST, CACHE_TRUE},
    {CAMERA_PREVIEW_3DNR, CACHE_TRUE},
    {CAMERA_PREVIEW_SCALE_3DNR, CACHE_TRUE},
    {CAMERA_PREVIEW_SCALE_AI_SCENE, CACHE_TRUE},
    {CAMERA_PREVIEW_SCALE_AUTO_TRACKING, CACHE_TRUE},
    {CAMERA_PREVIEW_DEPTH, CACHE_FASLE},
    {CAMERA_PREVIEW_SW_OUT, CACHE_FASLE},
    {CAMERA_PREVIEW_ULTRA_WIDE, CACHE_TRUE},
    {CAMERA_VIDEO_ULTRA_WIDE, CACHE_TRUE},
    {CAMERA_VIDEO_EIS_ULTRA_WIDE, CACHE_TRUE},
    {CAMERA_SNAPSHOT_ULTRA_WIDE, CACHE_TRUE},
    {CAMERA_SNAPSHOT_SW3DNR, CACHE_TRUE},
    {CAMERA_SNAPSHOT_SW3DNR_PATH, CACHE_TRUE},
    {CAMERA_4IN1_PROC, CACHE_TRUE},
    {CAMERA_SNAPSHOT_SLAVE_RESERVED, CACHE_TRUE},
    {CAMERA_ISPSTATS_AEM, CACHE_TRUE},
    {CAMERA_ISPSTATS_AFM, CACHE_TRUE},
    {CAMERA_ISPSTATS_AFL, CACHE_TRUE},
    {CAMERA_ISPSTATS_PDAF, CACHE_TRUE},
    {CAMERA_ISPSTATS_BAYERHIST, CACHE_TRUE},
    {CAMERA_ISPSTATS_YUVHIST, CACHE_TRUE},
    {CAMERA_ISPSTATS_LSCM, CACHE_TRUE},
    {CAMERA_ISPSTATS_3DNR, CACHE_TRUE},
    {CAMERA_ISPSTATS_EBD, CACHE_TRUE},
    {CAMERA_CHANNEL_0_RESERVED, CACHE_TRUE},
    {CAMERA_CHANNEL_1, CACHE_TRUE},
    {CAMERA_CHANNEL_1_RESERVED, CACHE_TRUE},
    {CAMERA_CHANNEL_2, CACHE_TRUE},
    {CAMERA_CHANNEL_2_RESERVED, CACHE_TRUE},
    {CAMERA_CHANNEL_3, CACHE_TRUE},
    {CAMERA_CHANNEL_3_RESERVED, CACHE_TRUE},
    {CAMERA_CHANNEL_4, CACHE_TRUE},
    {CAMERA_CHANNEL_4_RESERVED, CACHE_TRUE},
    {CAMERA_FD_SMALL, CACHE_TRUE},
    {CAMERA_SNAPSHOT_SW3DNR_SMALL_PATH, CACHE_TRUE},
};
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

#define ZSL_FRAME_TIMEOUT 1000000000      /*1000ms*/
#define SET_PARAM_TIMEOUT 2000000000      /*2000ms*/
#define CAP_TIMEOUT 5000000000            /*5000ms*/
#define PREV_TIMEOUT 5000000000           /*5000ms*/
#define CAP_START_TIMEOUT 5000000000      /* 5000ms*/
#define PREV_STOP_TIMEOUT 3000000000      /* 3000ms*/
#define CANCEL_AF_TIMEOUT 500000000       /*1000ms*/
#define DO_AF_TIMEOUT 2800000000          /*2800ms*/
#define PIPELINE_START_TIMEOUT 5000000000 /*5s*/

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
// modification for mZslSnapshotTime, 30 means 30 ms
#define MODIFICATION_TIME 30

// add for default zsl buffer
#define DEFAULT_ZSL_BUFFER_NUM 3

// for mlog
#define MLOG_DUMP_PATH "/data/mlog/"
#define MLOG_AE_PATH "/data/mlog/ae.txt"
#define MLOG_FOR_READ_PATH "/data/mlog/mlog.txt"
#define MLOG_CALIBRIATION_PATH "/system/etc/otpdata/input_parameters_values.txt"

// 3dnr Video mode
enum VIDEO_3DNR {
    VIDEO_OFF = 0,
    VIDEO_ON,
};

enum cmr_flash_lcd_mode {
    FLASH_LCD_MODE_OFF = 0,
    FLASH_LCD_MODE_AUTO = 1,
    FLASH_LCD_MODE_ON = 2,
    FLASH_LCD_MODE_MAX
};

#define CONFIG_PRE_ALLOC_CAPTURE_MEM 1
#define HAS_CAMERA_POWER_HINTS 1

// for set slowmotion fps
#define SLOWMOTION_120FPS 120

/*ZSL Thread*/
#define ZSLMode_MONITOR_QUEUE_SIZE 50
#define CMR_EVT_ZSL_MON_BASE 0x800
#define CMR_EVT_ZSL_MON_INIT 0x801
#define CMR_EVT_ZSL_MON_EXIT 0x802
#define CMR_EVT_ZSL_MON_SNP 0x804
#define CMR_EVT_ZSL_MON_STOP_OFFLINE_PATH 0x805

#define UPDATE_RANGE_FPS_COUNT 0x04
#define CAM_POWERHINT_WAIT_COUNT 35

#define MULTI_THREE_SECTION 3
/**********************Static Members**********************/
static nsecs_t s_start_timestamp = 0;
static nsecs_t s_end_timestamp = 0;
static int s_use_time = 0;
static nsecs_t cam_init_begin_time = 0;
bool gIsApctCamInitTimeShow = false;
bool gIsApctRead = false;
struct mlog_infotag mlog_info[CAMERA_ID_COUNT]; /* total as physic camera id */

bool SprdCamera3OEMIf::mZslCaptureExitLoop = false;
multi_camera_zsl_match_frame *SprdCamera3OEMIf::mMultiCameraMatchZsl = NULL;
multiCameraMode SprdCamera3OEMIf::mMultiCameraMode = MODE_SINGLE_CAMERA;
std::atomic_int SprdCamera3OEMIf::mLogMonitor = 1;
sem_t SprdCamera3OEMIf::mLogMonitorSem;


struct stateMachine aeStateMachine[] = {
    {ANDROID_CONTROL_AE_STATE_INACTIVE, AE_START,
     ANDROID_CONTROL_AE_STATE_SEARCHING},
    {ANDROID_CONTROL_AE_STATE_INACTIVE, AE_LOCK_ON,
     ANDROID_CONTROL_AE_STATE_LOCKED},
    {ANDROID_CONTROL_AE_STATE_SEARCHING, AE_STABLE,
     ANDROID_CONTROL_AE_STATE_CONVERGED},
    {ANDROID_CONTROL_AE_STATE_SEARCHING, AE_STABLE_REQUIRE_FLASH,
     ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED},
    {ANDROID_CONTROL_AE_STATE_SEARCHING, AE_LOCK_ON,
     ANDROID_CONTROL_AE_STATE_LOCKED},
    {ANDROID_CONTROL_AE_STATE_CONVERGED, AE_START,
     ANDROID_CONTROL_AE_STATE_SEARCHING},
    {ANDROID_CONTROL_AE_STATE_CONVERGED, AE_LOCK_ON,
     ANDROID_CONTROL_AE_STATE_LOCKED},
    {ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED, AE_START,
     ANDROID_CONTROL_AE_STATE_SEARCHING},
    {ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED, AE_LOCK_ON,
     ANDROID_CONTROL_AE_STATE_LOCKED},
    {ANDROID_CONTROL_AE_STATE_LOCKED, AE_LOCK_OFF,
     ANDROID_CONTROL_AE_STATE_SEARCHING},
    {ANDROID_CONTROL_AE_STATE_PRECAPTURE, AE_STABLE,
     ANDROID_CONTROL_AE_STATE_CONVERGED},
    {ANDROID_CONTROL_AE_STATE_PRECAPTURE, AE_LOCK_ON,
     ANDROID_CONTROL_AE_STATE_LOCKED},
    // All ae states exlcuding LOCKED change to PRECAPTURE when ae preCapture
    // start
    {ANDROID_CONTROL_AE_STATE_INACTIVE, AE_PRECAPTURE_START,
     ANDROID_CONTROL_AE_STATE_PRECAPTURE},
    {ANDROID_CONTROL_AE_STATE_SEARCHING, AE_PRECAPTURE_START,
     ANDROID_CONTROL_AE_STATE_PRECAPTURE},
    {ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED, AE_PRECAPTURE_START,
     ANDROID_CONTROL_AE_STATE_PRECAPTURE},
    {ANDROID_CONTROL_AE_STATE_CONVERGED, AE_PRECAPTURE_START,
     ANDROID_CONTROL_AE_STATE_PRECAPTURE},
    // All ae states excluding LOCKED change to INACTIVE when ae preCapture
    // cancel
    {ANDROID_CONTROL_AE_STATE_PRECAPTURE, AE_PRECAPTURE_CANCEL,
     ANDROID_CONTROL_AE_STATE_INACTIVE},
    {ANDROID_CONTROL_AE_STATE_SEARCHING, AE_PRECAPTURE_CANCEL,
     ANDROID_CONTROL_AE_STATE_INACTIVE},
    {ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED, AE_PRECAPTURE_CANCEL,
     ANDROID_CONTROL_AE_STATE_INACTIVE},
    {ANDROID_CONTROL_AE_STATE_CONVERGED, AE_PRECAPTURE_CANCEL,
     ANDROID_CONTROL_AE_STATE_INACTIVE},
};

struct stateMachine awbStateMachine[] = {
    {ANDROID_CONTROL_AWB_STATE_INACTIVE, AWB_START,
     ANDROID_CONTROL_AWB_STATE_SEARCHING},
    {ANDROID_CONTROL_AWB_STATE_INACTIVE, AWB_LOCK_ON,
     ANDROID_CONTROL_AWB_STATE_LOCKED},
    {ANDROID_CONTROL_AWB_STATE_SEARCHING, AWB_STABLE,
     ANDROID_CONTROL_AWB_STATE_CONVERGED},
    {ANDROID_CONTROL_AWB_STATE_SEARCHING, AWB_LOCK_ON,
     ANDROID_CONTROL_AWB_STATE_LOCKED},
    {ANDROID_CONTROL_AWB_STATE_CONVERGED, AWB_START,
     ANDROID_CONTROL_AWB_STATE_SEARCHING},
    {ANDROID_CONTROL_AWB_STATE_CONVERGED, AWB_LOCK_ON,
     ANDROID_CONTROL_AWB_STATE_LOCKED},
    {ANDROID_CONTROL_AWB_STATE_LOCKED, AWB_LOCK_OFF,
     ANDROID_CONTROL_AWB_STATE_SEARCHING},
};

struct stateMachine afModeAutoOrMacroStateMachine[] = {
    {ANDROID_CONTROL_AF_STATE_INACTIVE, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN, AF_SWEEP_DONE_AND_FOCUSED_LOCKED,
     ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN, AF_SWEEP_DONE_AND_NOT_FOCUSED_LOCKED,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN, AF_TRIGGER_CANCEL,
     ANDROID_CONTROL_AF_STATE_INACTIVE},
    {ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED, AF_TRIGGER_CANCEL,
     ANDROID_CONTROL_AF_STATE_INACTIVE},
    {ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED, AF_TRIGGER_CANCEL,
     ANDROID_CONTROL_AF_STATE_INACTIVE},
    {ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN},
};

struct stateMachine afModeContinuousPictureStateMachine[] = {
    {ANDROID_CONTROL_AF_STATE_INACTIVE, AF_INITIATES_NEW_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_INACTIVE, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN, AF_COMPLETES_CURRENT_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN, AF_FAILS_CURRENT_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN, AF_TRIGGER_START_AND_FOCUSED_LOCKED,
     ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN,
     AF_TRIGGER_START_AND_NOT_FOCUSED_LOCKED,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED, AF_INITIATES_NEW_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED, AF_INITIATES_NEW_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    // when cancel af trigger in caf mode, set af state to PASSIVE_SCAN
    {ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED, AF_TRIGGER_CANCEL,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED, AF_TRIGGER_CANCEL,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
};

struct stateMachine afModeContinuousVideoStateMachine[] = {
    {ANDROID_CONTROL_AF_STATE_INACTIVE, AF_INITIATES_NEW_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_INACTIVE, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN, AF_COMPLETES_CURRENT_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN, AF_FAILS_CURRENT_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED, AF_INITIATES_NEW_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED, AF_INITIATES_NEW_SCAN,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED},
    {ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED, AF_TRIGGER_START,
     ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED},
    // when cancel af trigger in caf mode, set af state to PASSIVE_SCAN
    {ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED, AF_TRIGGER_CANCEL,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
    {ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED, AF_TRIGGER_CANCEL,
     ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN},
};

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
        sprintf(buf, "\n");
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
      mSprdPipVivEnabled(0), mSprdHighIsoEnabled(0), mSprdFullscanEnabled(0),
      mSprdRefocusEnabled(0), mSprd3dCalibrationEnabled(0), mSprdYuvCallBack(0),
      mSprdMultiYuvCallBack(0), mSprdReprocessing(0), mNeededTimestamp(0),
      mIsUnpopped(false), mIsBlur2Zsl(false), mIsSlowmotion(false),
      mIsPortraitScene(false),
      mPreviewFormat(CAM_IMG_FMT_YUV420_NV21),
      mVideoFormat(CAM_IMG_FMT_YUV420_NV21),
      mCallbackFormat(CAM_IMG_FMT_YUV420_NV21),
      mPictureFormat(CAM_IMG_FMT_YUV420_NV21),
      mRawFormat(CAM_IMG_FMT_BAYER_MIPI_RAW), mPreviewStartFlag(0),
      mIsDvPreview(0), mIsStoppingPreview(0), mRecordingMode(0),
      mIsSetCaptureMode(false), mRecordingFirstFrameTime(0), mUser(0),
      mPreviewWindow(NULL), mHalOem(NULL), mIsStoreMetaData(false),
      mIsFreqChanged(false), mCameraId(cameraId), miSPreviewFirstFrame(1),
      mCaptureMode(CAMERA_NORMAL_MODE), mCaptureRawMode(0), mFlashMask(false),
      mReleaseFLag(false), mTimeCoeff(1), mIsPerformanceTestable(false),
      mIsAndroidZSL(false), mSetting(setting), BurstCapCnt(0), mCapIntent(0),
      mPrvTimerID(NULL), mFlashMode(-1), mIsAutoFocus(false),
      mIspToolStart(false), mSubRawHeapNum(0), mGraphicBufNum(0),
      mSubRawHeapSize(0), mPathRawHeapNum(0), mPathRawHeapSize(0),
      mPreviewDcamAllocBufferCnt(0), mIsRecording(false),
      mZSLModeMonitorMsgQueHandle(0), mZSLModeMonitorInited(0), mCNRMode(0), mEEMode(0),
      mGyroInit(0), mGyroExit(0), mEisPreviewInit(false), mEisVideoInit(false),
      mGyroNum(0), mSprdEisEnabled(false), mVideoSnapshotType(0),
      mIommuEnabled(false), mFlashCaptureFlag(0),
      mFlashCaptureSkipNum(FLASH_CAPTURE_SKIP_FRAME_NUM), mFixedFpsEnabled(0),
      mSprdAppmodeId(-1), mTempStates(CAMERA_NORMAL_TEMP), mIsTempChanged(0),
      mFlagOffLineZslStart(0), mZslSnapshotTime(0), mIsIspToolMode(0),
      mIsUltraWideMode(false), mIsFovFusionMode(false), mIsRawCapture(0), mIsCameraClearQBuf(0),
      mLatestFocusDoneTime(0), mFaceDetectStartedFlag(0),
      mIsJpegWithBigSizePreview(0)

{
    ATRACE_CALL();
    HAL_LOGI(":hal3: Constructor E camId=%d", cameraId);
    mDualVideoShotFlag =0;
    mDualVideoMode = 0;
#ifdef CONFIG_FACE_BEAUTY
    memset(&face_beauty, 0, sizeof(face_beauty));
    mflagfb = false;
#endif

    mFrontFlash = (char *)malloc(10 * sizeof(char));
    memset(mFrontFlash, 0, 10 * sizeof(char));
#ifdef FRONT_CAMERA_FLASH_TYPE
    strcpy(mFrontFlash, FRONT_CAMERA_FLASH_TYPE);
#endif

    memset(&grab_capability, 0, sizeof(grab_capability));

    SprdCameraSystemPerformance::getSysPerformance(&mSysPerformace);
    if (mSysPerformace) {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
    }

#if defined(LOWPOWER_DISPLAY_30FPS)
    property_set("vendor.cam.lowpower.display.30fps", "true");
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
    memset(mZslRawHeapArray, 0, sizeof(mZslRawHeapArray));
    memset(&mSlowPara, 0, sizeof(slow_motion_para));
    memset(mPathRawHeapArray, 0, sizeof(mPathRawHeapArray));
    memset(mGraphicBufArray, 0, sizeof(mGraphicBufArray));
    memset(mRawHeapArray, 0, sizeof(mRawHeapArray));
    memset(mZslGraphicsHandle, 0, sizeof(mZslGraphicsHandle));
    memset(mIspStatsDebugHeap, 0, sizeof(mIspStatsDebugHeap));

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
    mYuv2Width = 0;
    mYuv2Height = 0;
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

    mPicCaptureCnt = 0;

    mRegularChan = NULL;
    mPictureChan = NULL;

    mZslRawHeapNum = 0;
    mZslHeapNum = 0;
    mSubRawHeapSize = 0;
    m3dnrGraphicPathHeapNum = 0;
    mTotalIonSize = 0;
    mTotalGpuSize = 0;
    mTotalStackSize = 0;

#ifdef USE_ONE_RESERVED_BUF
    mCommonHeapReserved = NULL;
#endif

    mVideoShotFlag = 0;
    mVideoShotNum = 0;
    mVideoShotPushFlag = 0;

    mRestartFlag = false;

    mZslPreviewMode = false;

    mSprdZslEnabled = false;
    mZslMaxFrameNum = 1;
    mZslNum = DEFAULT_ZSL_BUFFER_NUM;
    mZslShotPushFlag = 0;
    mZslChannelStatus = 1;
    mZSLQueue.clear();

    mRawMaxFrameNum = 0;
    mRawNum = 1;
    mRawQueue.clear();

    mZslCaptureExitLoop = false;
    mSprdCameraLowpower = 0;
    mGetLastPowerHint = CAM_PERFORMNCE_LEVEL_MAX;

    mVideo3dnrFlag = 0;
    mCapBufIsAvail = 0;
    m_zslValidDataWidth = 0;
    m_zslValidDataHeight = 0;
    mChannelCb = NULL;
    mUserData = NULL;
    mZslStreamInfo = NULL;
    mStartFrameNum = 0;
    mStopFrameNum = 0;
    mGyroMsgQueHandle = 0;
    mGyromaxtimestamp = 0;
    mGyro_sem.count = 0;
    mVideoSnapshotFrameNum = 0;
    mMasterId = 0;
    mReprocessZoomRatio = 1.0f;
    mStreamOnWithZsl = 0;
    mIsNeedFlashFired = 0;
    mIsPowerhintWait = 0;
    mtimestamplast = 0ll;

#ifdef CONFIG_CAMERA_EIS
    memset(mGyrodata, 0, sizeof(mGyrodata));
    memset(&mPreviewParam, 0, sizeof(mPreviewParam));
    memset(&mVideoParam, 0, sizeof(mVideoParam));
    mPreviewInst = NULL;
    mVideoInst = NULL;
#endif

    mTopAppId = TOP_APP_NONE;
    mChannel2FaceBeautyFlag = 0;

    mFlush = 0;
    mVideoAFBCFlag = 0;

    HAL_LOGI(":hal3: Constructor X");
}

SprdCamera3OEMIf::~SprdCamera3OEMIf() {
    ATRACE_CALL();

    HAL_LOGI(":hal3: Destructor E camId=%d", mCameraId);
    int ret = NO_ERROR;
#ifdef CONFIG_FACE_BEAUTY
    if (mflagfb) {
        mflagfb = false;
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_FAST_STOP_CMD,NULL);
        face_beauty_deinit(&face_beauty);
    }
#endif

    if (!mReleaseFLag) {
        closeCamera();
    }

#if defined(LOWPOWER_DISPLAY_30FPS)
    char value[PROPERTY_VALUE_MAX];
    property_get("vendor.cam.lowpower.display.30fps", value, "false");
    if (!strcmp(value, "true")) {
        property_set("vendor.cam.lowpower.display.30fps", "false");
        HAL_LOGI("camera low power mode exit");
    }
#endif

    // clean memory in case memory leak
    freeAllCameraMem();

    if (mHalOem) {
        if (NULL != mHalOem->dso)
            dlclose(mHalOem->dso);
        free((void *)mHalOem);
        mHalOem = NULL;
    }

    if (mFrontFlash) {
        free((void *)mFrontFlash);
        mFrontFlash = NULL;
    }

    HAL_LOGI(":hal3: X");
    timer_stop();
    SprdCameraSystemPerformance::freeSysPerformance(&mSysPerformace);
}

void SprdCamera3OEMIf::closeCamera() {
    ATRACE_CALL();
    FLASH_INFO_Tag flashInfo;

    HAL_LOGI(":hal3: E camId=%d", mCameraId);

    Mutex::Autolock l(&mLock);

    mZslCaptureExitLoop = true;
    // Either preview was ongoing, or we are in the middle or taking a
    // picture.  It's the caller's responsibility to make sure the camera
    // is in the idle or init state before destroying this object.
    HAL_LOGD("camera state = %s, preview state = %s, capture state = %s",
             getCameraStateStr(getCameraState()),
             getCameraStateStr(getPreviewState()),
             getCameraStateStr(getCaptureState()));

    if (mSprdReprocessing) {
        SprdCamera3RegularChannel *channel =
            reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        HAL_LOGD("reprocess jpeg encode failed,unmap InputBuff");
        setCaptureReprocessMode(false, mCallbackWidth, mCallbackHeight);
        channel->releaseInputBuff();
    }

    if (isCapturing()) {
        cancelPictureInternal();
    }

    if (isPreviewing()) {
        stopPreviewInternal();
    }

    mSetting->getFLASHINFOTag(&flashInfo);
    if (flashInfo.available) {
        SprdCamera3Flash::releaseFlash(mCameraId);
    }

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

    ZSLMode_monitor_thread_deinit((void *)this);
    log_monitor_thread_deinit((void *)this);

    mReleaseFLag = true;
    HAL_LOGI(":hal3: X CamId=%d TotalIonSize=%d TotalGpuSize=%d",
              mCameraId, mTotalIonSize, mTotalGpuSize);
}

int SprdCamera3OEMIf::getCameraId() const { return mCameraId; }

void SprdCamera3OEMIf::initialize() {
    memset(&mSlowPara, 0, sizeof(slow_motion_para));
    mIsRecording = false;
    mVideoShotFlag = 0;
    mZslNum = DEFAULT_ZSL_BUFFER_NUM;

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
    mIsNeedFlashFired = 0;
    mSprdZslEnabled = 0;
    mSprd3dnrType = 0;
    mVideoSnapshotType = 0;
    mTopAppId = TOP_APP_NONE;
    mChannel2FaceBeautyFlag = 0;
    mZslCaptureExitLoop = false;
    mIsSlowmotion = false;
    mFlush = 0;
    mVideoAFBCFlag = 0;
#ifdef CONFIG_FACE_BEAUTY
    mflagfb = false;
#endif
}

int SprdCamera3OEMIf::start(camera_channel_type_t channel_type,
                            uint32_t frame_number) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    Mutex::Autolock l(&mLock);

    HAL_LOGD("channel_type = %d, frame_number = %d", channel_type,
             frame_number);
    mStartFrameNum = frame_number;
    setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);

    switch (channel_type) {
    case CAMERA_CHANNEL_TYPE_REGULAR: {
        mIsPowerhintWait = 1;
        if (mParaDCDVMode == CAMERA_PREVIEW_FORMAT_DV)
            mRecordingFirstFrameTime = 0;

#ifdef CONFIG_CAMERA_EIS
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        if (sprddefInfo->sprd_eis_enabled == 1) {
            mSprdEisEnabled = true;
        } else {
            mSprdEisEnabled = false;
        }
        if (!mEisPreviewInit && sprddefInfo->sprd_eis_enabled == 1 &&
            mPreviewWidth != 0 && mPreviewHeight != 0) {
            EisPreview_init();
            mEisPreviewInit = true;
        }
        if (!mEisVideoInit && sprddefInfo->sprd_eis_enabled == 1 &&
            mVideoWidth != 0 && mVideoHeight != 0) {
            EisVideo_init();
            mEisVideoInit = true;
        }
#endif
        char value[PROPERTY_VALUE_MAX];
        property_get("debug.camera.dcam.raw.mode", value, "null");
        if (!strcmp(value, "dcamraw")) {
            usleep(500 * 1000);
            HAL_LOGD("dcam_raw_mode_delay= %d ms", 500);
        }
        ret = startPreviewInternal();
        break;
    }
    case CAMERA_CHANNEL_TYPE_PICTURE: {
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
        mHalOem->ops->camera_set_mm_dvfs_policy(mCameraHandle, DVFS_ISP,
                                                IS_CAP_BEGIN);
#endif

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

    HAL_LOGD("X");
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
        stopPreviewInternal();
#ifdef CONFIG_CAMERA_EIS
        if (mEisPreviewInit) {
            Mutex::Autolock l(&mEisPreviewProcessLock);
            video_stab_close(&mPreviewInst);
            mEisPreviewInit = false;
            HAL_LOGI("preview stab close");
        }
        if (mEisVideoInit) {
            Mutex::Autolock l(&mEisVideoProcessLock);
            video_stab_close(&mVideoInst);
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

    HAL_LOGD("X");
    return ret;
}

int SprdCamera3OEMIf::takePicture() {
    ATRACE_CALL();

    int ret = 0;
    HAL_LOGD("E");

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
                (strcmp(mFrontFlash, "lcd") == 0) ? true : false;
            // whether FRONT_CAMERA_FLASH_TYPE is flash
            bool isFrontFlash =
                (strcmp(mFrontFlash, "flash") == 0) ? true : false;
            if (mMultiCameraMode == MODE_MULTI_CAMERA || mCameraId == 0 ||
                isFrontLcd || isFrontFlash || mCameraId == 4) {
                mHalOem->ops->camera_start_preflash(mCameraHandle);
            }
            stopPreviewInternal();
        }
        HAL_LOGV("stopPreviewInternal done, preview state = %d",
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
    HAL_LOGD("X");
    /*must return NO_ERROR, otherwise can't flush camera normal*/
    return NO_ERROR;
}

// Send async zsl snapshot msg
int SprdCamera3OEMIf::zslTakePicture() {
    ATRACE_CALL();

    uint32_t ret = 0;

    if (mSprdZslEnabled == true) {
        CMR_MSG_INIT(message);
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

    HAL_LOGV("X");
    return ret;
}

int SprdCamera3OEMIf::reprocessYuvForJpeg(cmr_uint yaddr, cmr_uint yaddr_vir,
                                          cmr_uint fd) {
    ATRACE_CALL();

    uint32_t ret = 0;
    struct img_size capturesize;
    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();

    HAL_LOGD("E");
    GET_START_TIME;
    print_time();

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    mSprdYuvCallBack = false;
    if (getMultiCameraMode() == MODE_3D_CAPTURE) {
        mSprd3dCalibrationEnabled = sprddefInfo->sprd_3dcalibration_enabled;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                 sprddefInfo->sprd_3dcalibration_enabled);
    }
    if (getMultiCameraMode() == MODE_BLUR ||
        getMultiCameraMode() == MODE_BOKEH ||
        getMultiCameraMode() == MODE_MULTI_CAMERA) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_YUV_CALLBACK_ENABLE,
                 mSprdYuvCallBack);
        HAL_LOGD("reprocess mode, force enable reprocess");
    }
    setCameraState(SPRD_FLASH_IN_PROGRESS, STATE_CAPTURE);

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
    HAL_LOGD("JPEG thumbnail size = %d x %d", jpeg_thumb_size.width,
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
    HAL_LOGD("X");
    return ret;
}

int SprdCamera3OEMIf::checkIfNeedToStopOffLineZsl() {
    uint32_t ret = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();

    // capture_mode: 1 single capture; >1: n capture
    if (mFlagOffLineZslStart && mSprdZslEnabled == 1 &&
        sprddefInfo->capture_mode == 1 && mZslShotPushFlag == 0) {
        HAL_LOGD("mFlagOffLineZslStart=%d, sprddefInfo->capture_mode=%d",
                 mFlagOffLineZslStart, sprddefInfo->capture_mode);
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

    HAL_LOGD("E mCameraId = %d", mCameraId);
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
    HAL_LOGD("X");
    return NO_ERROR;
}

int SprdCamera3OEMIf::cancelPicture() {
    Mutex::Autolock l(&mLock);

    return cancelPictureInternal();
}

status_t SprdCamera3OEMIf::faceDectect(bool enable) {
    status_t ret = NO_ERROR;
    SPRD_DEF_Tag *sprddefInfo;
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    if (sprddefInfo->slowmotion > 1)
        return ret;

    if (enable) {
        mHalOem->ops->camera_fd_start(mCameraHandle, 1);
    } else {
        mHalOem->ops->camera_fd_start(mCameraHandle, 0);
    }
    return ret;
}

status_t SprdCamera3OEMIf::faceDectect_enable(bool enable) {
    status_t ret = NO_ERROR;
    SPRD_DEF_Tag *sprddefInfo;
    int sensorOrientation;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if ((mMultiCameraMode == MODE_BOKEH && mCameraId == 2) ||
        mMultiCameraMode == MODE_3D_CALIBRATION ||
        (mMultiCameraMode == MODE_TUNING && mCameraId == 2)) {
        return ret;
    }

    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    if (sprddefInfo->slowmotion > 1)
        return ret;

    sensorOrientation = SprdCamera3Setting::s_setting[mCameraId].sensorInfo.orientation;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
             sensorOrientation);
    HAL_LOGV("sensorOrientation = %d", sensorOrientation);

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
    char prop[PROPERTY_VALUE_MAX];
    property_get("ro.vendor.camera.dualcamera_cali_time", prop, "0");

    if (mSysPerformace) {
        mGetLastPowerHint = mSysPerformace->mCurrentPowerHintScene;
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
    }

    mSetting->getCONTROLTag(&controlInfo);

    if (!isPreviewing()) {
        HAL_LOGW("preveiw is not start yet");
        controlInfo.af_state = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
        mSetting->setAfCONTROLTag(&controlInfo);
        goto exit;
    }

    if (SPRD_IDLE != getFocusState()) {
        HAL_LOGE("existing, direct return!");
        return NO_ERROR;
    }

    setCameraState(SPRD_FOCUS_IN_PROGRESS, STATE_FOCUS);
    mIsAutoFocus = true;
    if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
        controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO) {
        int verification_enable = mSetting->getVERIFITag();
        if (getMultiCameraMode() == MODE_BLUR && isNeedAfFullscan() &&
            controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START &&
            controlInfo.af_regions[0] == 0 && controlInfo.af_regions[1] == 0 &&
            controlInfo.af_regions[2] == 0 && controlInfo.af_regions[3] == 0) {
            HAL_LOGD("set full scan mode");
            if (mCameraId == 1) {
                autoFocusToFaceFocus();
            }
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                     CAMERA_FOCUS_MODE_FULLSCAN);
        } else if (mSprdRefocusEnabled && 3 == atoi(prop) &&
                   (1 != verification_enable) &&
                   (getMultiCameraMode() == MODE_MULTI_CAMERA ||
                    mCameraId == 0 || mCameraId == 4)) {
            HAL_LOGD("mm-test set full scan mode");
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE,
                     CAMERA_FOCUS_MODE_FULLSCAN);
        }
    }

    if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO ||
        controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_MACRO ||
        controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
        setAfState(AF_TRIGGER_START);
    } else if (controlInfo.af_mode ==
               ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
        setAfState(AF_INITIATES_NEW_SCAN);
    }

    if (0 != mHalOem->ops->camera_start_autofocus(mCameraHandle)) {
        HAL_LOGE("auto foucs fail");
        setCameraState(SPRD_IDLE, STATE_FOCUS);
        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO ||
            controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_MACRO) {
            setAfState(AF_SWEEP_DONE_AND_NOT_FOCUSED_LOCKED);
        } else if (controlInfo.af_mode ==
                       ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
                   controlInfo.af_mode ==
                       ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
            setAfState(AF_TRIGGER_START_AND_NOT_FOCUSED_LOCKED);
        }
    }

exit:
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

    ret = mHalOem->ops->camera_cancel_autofocus(mCameraHandle);

    WaitForFocusCancelDone();
    {
        CONTROL_Tag controlInfo;
        mSetting->getCONTROLTag(&controlInfo);
        if (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
            setAfState(AF_TRIGGER_CANCEL);
        }
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
            getMultiCameraMode() == MODE_BOKEH ||
            getMultiCameraMode() == MODE_MULTI_CAMERA) {
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
    HAL_LOGV("E");

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

    HAL_LOGV("X");
}

cmr_s32 SprdCamera3OEMIf::ispSwCheckBuf(cmr_uint *param_ptr) {
    HAL_LOGV("E");
    /*
        return mHalOem->ops->camera_isp_sw_check_buf(mCameraHandle, param_ptr);
    */
    HAL_LOGV("X");
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

    // HAL_LOGD("E");
    ret = mHalOem->ops->camera_ioctrl(
        mCameraHandle, CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL, &on_off);

    return ret;
}

int SprdCamera3OEMIf::setCameraClearQBuff() {
    int ret = 0;

    HAL_LOGD("E");
    mIsCameraClearQBuf = 1;
    // HAL_LOGD("mIsCameraClearQBuf %d", mIsCameraClearQBuf);
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
    mHalOem->ops->camera_get_sensor_otp_info(mCameraHandle, 1, &otp_info);

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
    // HAL_LOGD("online buffer addr %p", cali_info);
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
    case CAMERA_IOCTRL_ULTRA_WIDE_MODE: {
        if (*(unsigned int *)param1 == 1) {
            mIsUltraWideMode = true;
        } else {
            mIsUltraWideMode = false;
        }
        break;
    }
    case CAMERA_IOCTRL_FOV_FUSION_MODE: {
        if (*(unsigned int *)param1 == 1) {
            mIsFovFusionMode = true;
        } else {
            mIsFovFusionMode = false;
        }
        break;
    }
    case CAMERA_TOCTRL_SET_HIGH_RES_MODE: {
        SPRD_DEF_Tag *sprddefInfo;

        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        sprddefInfo->high_resolution_mode = *(unsigned int *)param1;
        break;
    }
    case CAMERA_IOCTRL_SET_VISIBLE_REGION:{
        struct visible_region_info *info = (struct visible_region_info *)param1;

        if (info) {
            uint16_t w, h;

            mSetting->getLargestSensorSize(mCameraId, &w, &h);
            info->max_size.width = w;
            info->max_size.height = h;
        }
        break;
    }
    case CAMERA_IOCTRL_SET_FB_SWITCH:{
            mIsPortraitScene = *((bool *)param1);
        }
        break;
    } /* switch */
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

    HAL_LOGD("X,mime_type=0x%x", mime_type);
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
    struct img_size yuv2Size = {0, 0};
    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();

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
        mPreviewFormat = CAM_IMG_FMT_YUV420_NV21;
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&previewSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PREVIEW_FORMAT,
             (cmr_uint)mPreviewFormat);

    // video
    if (mVideoWidth > 0 && mVideoHeight > 0) {
        // TBD: will remove this
        if (sprddefInfo->slowmotion <= 1)
            mCaptureMode = CAMERA_ZSL_MODE;
    }

    if (mVideoCopyFromPreviewFlag) {
        // HAL_LOGD("video stream copy from preview stream");
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
        getMultiCameraMode() != MODE_3D_CALIBRATION &&
        getMultiCameraMode() != MODE_MULTI_CAMERA) {
        callbackSize.width = mCallbackWidth;
        callbackSize.height = mCallbackHeight;
    }
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_YUV_CALLBACK_SIZE,
             (cmr_uint)&callbackSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_YUV_CALLBACK_FORMAT,
             (cmr_uint)mCallbackFormat);

    yuv2Size.width = mYuv2Width;
    yuv2Size.height = mYuv2Height;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_YUV2_SIZE,
             (cmr_uint)&yuv2Size);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_YUV2_FORMAT,
             (cmr_uint)mYuv2Format);

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
        mPictureFormat = mCallbackFormat;
    }

    if (mSprdAppmodeId == CAMERA_MODE_SLOWMOTION) {
        captureSize.width = 0;
        captureSize.height = 0;
    }

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&captureSize);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CAPTURE_FORMAT,
             (cmr_uint)mPictureFormat);

    setPreviewFps(mRecordingMode);

    HAL_LOGD("preview: w=%d, h=%d, format=%d  video: w=%d, h=%d, format=%d  "
             "calback: w=%d, h=%d, format=%d  capture: w=%d, h=%d, format=%d,  "
             "raw: w=%d, h=%d, format=%d  yuv2: w=%d, h=%d, format=%d",
             previewSize.width, previewSize.height, mPreviewFormat,
             videoSize.width, videoSize.height, mVideoFormat,
             callbackSize.width, callbackSize.height, mCallbackFormat,
             captureSize.width, captureSize.height, mPictureFormat,
             rawSize.width, rawSize.height, mRawFormat, yuv2Size.width,
             yuv2Size.height, mYuv2Format);

    return true;
}

int SprdCamera3OEMIf::setSnapshotParams() {
    struct img_size previewSize = {0, 0}, videoSize = {0, 0};
    struct img_size callbackSize = {0, 0}, captureSize = {0, 0};
    int ret = 0;
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

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

void SprdCamera3OEMIf::setAfState(enum afTransitionCause cause) {
    CONTROL_Tag controlInfo;
    uint32_t state;
    uint32_t newState;
    uint32_t size;
    uint32_t i;

    mSetting->getCONTROLTag(&controlInfo);
    state = controlInfo.af_state;
    newState = state;

    // start with INACTIVE when changing af mode
    if (AF_MODE_CHANGE == cause) {
        // switching between AF_MODE_CONTINOUS_* will be ignored
        if (!(controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO &&
              mLastAfMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) &&
            !(controlInfo.af_mode ==
                  ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE &&
              mLastAfMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO)) {
            newState = ANDROID_CONTROL_AF_STATE_INACTIVE;
            goto exit;
        }
    }

    switch (controlInfo.af_mode) {
    case ANDROID_CONTROL_AF_MODE_OFF:
    case ANDROID_CONTROL_AF_MODE_EDOF:
        // af state is always INACTIVE when af mode is OFF or EDOF
        newState = ANDROID_CONTROL_AF_STATE_INACTIVE;
        break;
    case ANDROID_CONTROL_AF_MODE_AUTO:
    case ANDROID_CONTROL_AF_MODE_MACRO:
        size =
            sizeof(afModeAutoOrMacroStateMachine) / sizeof(struct stateMachine);
        for (i = 0; i < size; i++) {
            if ((afModeAutoOrMacroStateMachine[i].transitionCause == cause) &&
                (afModeAutoOrMacroStateMachine[i].state == state)) {
                newState = afModeAutoOrMacroStateMachine[i].newState;
                break;
            }
        }
        break;
    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
        size = sizeof(afModeContinuousPictureStateMachine) /
               sizeof(struct stateMachine);
        for (i = 0; i < size; i++) {
            if ((afModeContinuousPictureStateMachine[i].transitionCause ==
                 cause) &&
                (afModeContinuousPictureStateMachine[i].state == state)) {
                newState = afModeContinuousPictureStateMachine[i].newState;
                break;
            }
        }
        break;
    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
        size = sizeof(afModeContinuousVideoStateMachine) /
               sizeof(struct stateMachine);
        for (i = 0; i < size; i++) {
            if ((afModeContinuousVideoStateMachine[i].transitionCause ==
                 cause) &&
                (afModeContinuousVideoStateMachine[i].state == state)) {
                newState = afModeContinuousVideoStateMachine[i].newState;
                break;
            }
        }
        break;
    default:
        newState = ANDROID_CONTROL_AF_STATE_INACTIVE;
        break;
    }

exit:
    HAL_LOGD("mCameraId=%d, Af mode=%d, transition cause=%d, cur state=%d, new "
             "state=%d",
             mCameraId, controlInfo.af_mode, cause, state, newState);

    controlInfo.af_state = newState;
    mSetting->setAfCONTROLTag(&controlInfo);
}

void SprdCamera3OEMIf::setPreviewFps(bool isRecordMode) {
    struct cmr_range_fps_param fps_param;
    int ret = NO_ERROR;
    struct ae_fps_range_info range;
    char value[PROPERTY_VALUE_MAX];
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    cmr_bzero(&range, sizeof(ae_fps_range_info));
    fps_param.is_recording = isRecordMode;
    if (isRecordMode) {
        fps_param.min_fps = controlInfo.ae_target_fps_range[0];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];

        // get DV fps from tunning
        ret = mHalOem->ops->camera_ioctrl(
            mCameraHandle, CAMERA_IOCTRL_GET_AE_FPS_RANGE_INFO, &range);
        if (ret) {
            HAL_LOGE("get range fps failed, ret=%d", ret);
        } else {
            if (fps_param.min_fps < range.dv_fps_min &&
                range.dv_fps_min < fps_param.max_fps) {
                fps_param.min_fps = range.dv_fps_min;
            }
        }

        fps_param.video_mode = 1;

        HAL_LOGV(
            "get DV mode tuning fps range[%d, %d], ae_target_fps_range[%d, %d]",
            range.dv_fps_min, range.dv_fps_max,
            controlInfo.ae_target_fps_range[0],
            controlInfo.ae_target_fps_range[1]);

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
        if(fps_param.min_fps == SLOWMOTION_120FPS && fps_param.max_fps == SLOWMOTION_120FPS) {
            mIsSlowmotion = true;
        } else {
            mIsSlowmotion = false;
        }
    } else {
        fps_param.min_fps = controlInfo.ae_target_fps_range[0];
        fps_param.max_fps = controlInfo.ae_target_fps_range[1];
        fps_param.video_mode = 0;

        char fps_prop[PROPERTY_VALUE_MAX];
        property_get("ro.vendor.camera.dualcamera_fps", fps_prop, "19");

        // TBD: check why 20fps, not 30fps
        if (mMultiCameraMode == MODE_BOKEH ||
            mMultiCameraMode == MODE_3D_CALIBRATION ||
            mMultiCameraMode == MODE_REFOCUS ||
            mMultiCameraMode == MODE_RANGE_FINDER ||
            mMultiCameraMode == MODE_TUNING ||
            mMultiCameraMode == MODE_DUAL_FACEID_REGISTER ||
            mMultiCameraMode == MODE_DUAL_FACEID_UNLOCK) {
            fps_param.min_fps = atoi(fps_prop);
            fps_param.max_fps = atoi(fps_prop);
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

    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SLOW_MOTION_FLAG, (cmr_uint)mIsSlowmotion);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    HAL_LOGD("camera id= %d, min_fps=%ld, max_fps=%ld, video_mode=%ld, mIsSlowmotion:%d",
             mCameraId, fps_param.min_fps, fps_param.max_fps,
             fps_param.video_mode, mIsSlowmotion);
}

void SprdCamera3OEMIf::setAeState(enum aeTransitionCause cause) {
    CONTROL_Tag controlInfo;
    uint32_t state;
    uint32_t newState;
    uint32_t size;
    uint32_t i;

    mSetting->getCONTROLTag(&controlInfo);
    state = controlInfo.ae_state;
    newState = state;

    // ae state is always INACTIVE when ae mode is OFF
    if (ANDROID_CONTROL_AE_MODE_OFF == controlInfo.ae_mode) {
        HAL_LOGD("set INACTIVE when AE mode is OFF");
        newState = ANDROID_CONTROL_AE_STATE_INACTIVE;
        goto exit;
    }

    // start with INACTIVE when changing ae mode
    if (AE_MODE_CHANGE == cause && ANDROID_CONTROL_AE_STATE_LOCKED != state) {
        newState = ANDROID_CONTROL_AE_STATE_INACTIVE;
        HAL_LOGD("set INACTIVE when changing ae mode");
    }

    // FLASH_REQUIRED cannot be a result of precapture sequence
    if (AE_STABLE_REQUIRE_FLASH == cause &&
        ANDROID_CONTROL_AE_STATE_PRECAPTURE == state) {
        cause = AE_STABLE;
    }

    /* remove later
     * for CTS: testBasicTriggerSequence
     * SEARCHING cannot be a resut of precapture sequence.
     * Precapture ctrl flow should be designed later.
     */
    if ((mSprdAppmodeId == -1) && (AE_START == cause) &&
        (ANDROID_CONTROL_AE_STATE_CONVERGED == state)) {
        goto exit;
    }

    size = sizeof(aeStateMachine) / sizeof(struct stateMachine);
    for (i = 0; i < size; i++) {
        if ((aeStateMachine[i].transitionCause == cause) &&
            (aeStateMachine[i].state == state)) {
            newState = aeStateMachine[i].newState;
            HAL_LOGD("mCameraId=%d, Ae transition cause=%d, cur state=%d, new "
                     "state=%d",
                     mCameraId, cause, state, newState);
            break;
        }
    }

exit:
    controlInfo.ae_state = newState;
    mSetting->setAeCONTROLTag(&controlInfo);
}

void SprdCamera3OEMIf::setAwbState(enum awbTransitionCause cause) {
    CONTROL_Tag controlInfo;
    uint32_t state;
    uint32_t newState;
    uint32_t size;
    uint32_t i;

    mSetting->getCONTROLTag(&controlInfo);
    state = controlInfo.awb_state;
    newState = controlInfo.awb_state;

    // awb state is always INACTIVE when awb mode is not AUTO
    if (ANDROID_CONTROL_AWB_MODE_AUTO != controlInfo.awb_mode) {
        HAL_LOGD("set INACTIVE when AWB mode is not AUTO");
        newState = ANDROID_CONTROL_AWB_STATE_INACTIVE;
        goto exit;
    }

    // start with INACTIVE when changing awb mode
    if (AWB_MODE_CHANGE == cause && ANDROID_CONTROL_AWB_STATE_LOCKED != state) {
        newState = ANDROID_CONTROL_AWB_STATE_INACTIVE;
        HAL_LOGD("set INACTIVE when changing awb mode");
    }

    size = sizeof(awbStateMachine) / sizeof(struct stateMachine);
    for (i = 0; i < size; i++) {
        if ((awbStateMachine[i].transitionCause == cause) &&
            (awbStateMachine[i].state == state)) {
            newState = awbStateMachine[i].newState;
            HAL_LOGD("awb transition cause=%d, cur state=%d, next state=%d",
                     cause, state, newState);
            break;
        }
    }

exit:
    controlInfo.awb_state = newState;
    mSetting->setAwbCONTROLTag(&controlInfo);
}

void SprdCamera3OEMIf::setTimeoutParams() {
    setCameraState(SPRD_ERROR, STATE_PREVIEW);
}

void SprdCamera3OEMIf::setCameraState(Sprd_camera_state state,
                                      state_owner owner) {
    Sprd_camera_state org_state = SPRD_IDLE;
    volatile Sprd_camera_state *state_owner = NULL;
    Mutex::Autolock stateLock(&mStateLock);
    HAL_LOGV("E:state: %s, owner: %d camera id %d", getCameraStateStr(state),
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
    HAL_LOGV("%s", getCameraStateStr(mCameraState.focus_state));
    return mCameraState.focus_state;
}

SprdCamera3OEMIf::Sprd_camera_state SprdCamera3OEMIf::getSetParamsState() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.setParam_state));
    return mCameraState.setParam_state;
}

bool SprdCamera3OEMIf::isCameraInit() {
    HAL_LOGV("%s", getCameraStateStr(mCameraState.camera_state));
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

bool SprdCamera3OEMIf::isPreviewIdle() {
    HAL_LOGI("%s", getCameraStateStr(mCameraState.preview_state));
    return (SPRD_IDLE == mCameraState.preview_state);
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

    HAL_LOGV("E");
    Mutex::Autolock stateLock(&mStateLock);

    if (SPRD_INTERNAL_STOPPING == mCameraState.camera_state) {
        while (SPRD_INIT != mCameraState.camera_state &&
               SPRD_ERROR != mCameraState.camera_state) {
            HAL_LOGD("waiting for SPRD_IDLE");
            mStateWait.wait(mStateLock);
            HAL_LOGD("woke up");
        }
    }

    HAL_LOGV("X");

    return SPRD_INIT == mCameraState.camera_state;
}

int SprdCamera3OEMIf::waitForPipelineStart() {
    ATRACE_CALL();
    int ret = 0;

    HAL_LOGV("E");
    Mutex::Autolock l(mPipelineStartLock);

    while (mCameraState.preview_state != SPRD_PREVIEW_IN_PROGRESS &&
           mCameraState.preview_state != SPRD_INTERNAL_PREVIEW_REQUESTED &&
           mCameraState.preview_state != SPRD_ERROR) {
        // HAL_LOGD("waiting for pipeline start");
        if (mPipelineStartSignal.waitRelative(mPipelineStartLock,
                                              PIPELINE_START_TIMEOUT)) {
            HAL_LOGE("timeout");
            ret = -1;
            break;
        }
    }

    HAL_LOGV("X");
    return ret;
}

bool SprdCamera3OEMIf::WaitForPreviewStart() {
    ATRACE_CALL();

    HAL_LOGV("E");
    Mutex::Autolock stateLock(&mStateLock);

    while (SPRD_PREVIEW_IN_PROGRESS != mCameraState.preview_state &&
           SPRD_ERROR != mCameraState.preview_state) {
        // HAL_LOGD("waiting for SPRD_PREVIEW_IN_PROGRESS");
        if (mStateWait.waitRelative(mStateLock, PREV_TIMEOUT)) {
            HAL_LOGE("timeout");
            break;
        }
        HAL_LOGD("woke up");
    }

    HAL_LOGV("X");

    return SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state;
}

bool SprdCamera3OEMIf::WaitForCaptureDone() {
    ATRACE_CALL();

    Mutex::Autolock stateLock(&mStateLock);
    while (SPRD_IDLE != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.capture_state) {
        // HAL_LOGD("waiting for SPRD_IDLE");
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
        // HAL_LOGD("waiting for SPRD_WAITING_JPEG");
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
    mTotalIonSize = mTotalIonSize + mem_size;
    HAL_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p, TotalIonSize=%d",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon, mTotalIonSize);

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
            mTotalIonSize = mTotalIonSize - (memory->phys_size);
            HAL_LOGD(
                "fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p, TotalIonSize=%d",
                memory->fd, memory->phys_addr, memory->data, memory->phys_size,
                memory->ion_heap, mTotalIonSize);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            HAL_LOGD("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                     "virt_addr=%p, size=0x%lx, TotalIonSize=%d",
                     memory->fd, memory->phys_addr, memory->data,
                     memory->phys_size,mTotalIonSize);
        }
        free(memory);
        memory = NULL;
    }
}

void SprdCamera3OEMIf::freeAllCameraMem() {
    int i, sum;
    uint32_t j;
    SprdCamera3GrallocMemory *memory = new SprdCamera3GrallocMemory();
    HAL_LOGI(":hal3: E");

    // performance optimization:move Callback_CaptureFree to closeCamera
    Callback_CaptureFree(0, 0, 0, 0);

    for (j = 0; j < mZslHeapNum; j++) {
        if (NULL != mZslHeapArray[j]) {
            freeCameraMem(mZslHeapArray[j]);
            mZslHeapArray[j] = NULL;
        }
    }
    mZslHeapNum = 0;

    for (j = 0; j < mZslRawHeapNum; j++) {
        if (NULL != mZslRawHeapArray[j]) {
            freeCameraMem(mZslRawHeapArray[j]);
            mZslRawHeapArray[j] = NULL;
        }
    }
    mZslRawHeapNum = 0;

    for (j = 0; j < mPathRawHeapNum; j++) {
        if (NULL != mPathRawHeapArray[j]) {
            freeCameraMem(mPathRawHeapArray[j]);
        }
        mPathRawHeapArray[j] = NULL;
    }
    mPathRawHeapNum = 0;
    mPathRawHeapSize = 0;

    Callback_Sw3DNRCapturePathFree(0, 0, 0, 0);

#ifdef USE_ONE_RESERVED_BUF
    if (NULL != mCommonHeapReserved) {
        freeCameraMem(mCommonHeapReserved);
        mCommonHeapReserved = NULL;
    }
#endif
    // free all ION buffer
    for (List<MemIonQueue>::iterator i = cam_MemIonQueue.begin(); i != cam_MemIonQueue.end(); i++) {
        if (i->mIonHeap != NULL) {
            HAL_LOGV("mem_type=%d mIonHeap=%p", i->mem_type, i->mIonHeap);
            freeCameraMem(i->mIonHeap);
            i->mIonHeap = NULL;
            cam_MemIonQueue.erase(i);
        }
    }
    // free all GPU buffer
    for (List<MemGpuQueue>::iterator i = cam_MemGpuQueue.begin(); i != cam_MemGpuQueue.end(); i++) {
        if (i->mGpuHeap.bufferhandle != NULL) {
            if (mIsUltraWideMode) {
                i->mGpuHeap.bufferhandle->unlock();
            } else {
                memory->unmap(&(i->mGpuHeap.bufferhandle->handle),
                              NULL);
            }
            mTotalGpuSize = mTotalGpuSize - (i->mGpuHeap.buf_size);
            i->mGpuHeap.bufferhandle.clear();
            i->mGpuHeap.bufferhandle = NULL;
            cam_MemGpuQueue.erase(i);
        }
    }

    sum = ISP_STATSDBG_MAX;
    for (i = 0; i < sum; i++) {
        if (NULL != mIspStatsDebugHeap[i]) {
            freeCameraMem(mIspStatsDebugHeap[i]);
        }
        mIspStatsDebugHeap[i] = NULL;
    }
    freeRawBuffers();
    delete memory;

    if ((mTotalIonSize != 0) || (mTotalGpuSize != 0))
        HAL_LOGE("ION/GPU buffer memory leak!!!");

    HAL_LOGI(":hal3: X CamId=%d TotalIonSize=%d TotalGpuSize=%d",
              mCameraId, mTotalIonSize, mTotalGpuSize);
}

bool SprdCamera3OEMIf::initPreview() {
    HAL_LOGV("E");

    setPreviewParams();

    HAL_LOGV("X");
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

    // Clean the autotrackingInfo
    AUTO_TRACKING_Tag autotrackingInfo;
    mSetting->getAUTOTRACKINGTag(&autotrackingInfo);
    memset(&autotrackingInfo, 0, sizeof(AUTO_TRACKING_Tag));
    mSetting->setAUTOTRACKINGTag(&autotrackingInfo);

    HAL_LOGV("X");
}

void SprdCamera3OEMIf::deinitCapture(bool isPreAllocCapMem) {
    HAL_LOGV("E %d", isPreAllocCapMem);

    if (0 == isPreAllocCapMem) {
        Callback_CaptureFree(0, 0, 0, 0);
    } else {
        HAL_LOGD("pre_allocate mode.");
    }

    Callback_CapturePathFree(0, 0, 0, 0);
    Callback_Sw3DNRCapturePathFree(0, 0, 0, 0);
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
    cmr_u16 af_support = 0;
    bool is_volte = false;
    char value[PROPERTY_VALUE_MAX];
    char multicameramode[PROPERTY_VALUE_MAX];
    struct img_size jpeg_thumb_size;
    FLASH_INFO_Tag flashInfo;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    HAL_LOGI("E camera id %d", mCameraId);

    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();

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
        HAL_LOGV("Preview already in progress, mRestartFlag=%d", mRestartFlag);
        if (mRestartFlag == false) {
            return NO_ERROR;
        } else {
            stopPreviewInternal();
        }
    }

    mZslCaptureExitLoop = false;
    mRestartFlag = false;
    mVideoCopyFromPreviewFlag = false;
    mVideoProcessedWithPreview = false;
    mVideo3dnrFlag = VIDEO_OFF;
    camera_ioctrl(CAMERA_IOCTRL_3DNR_VIDEOMODE, &mVideo3dnrFlag, NULL);
    camera_ioctrl(CAMERA_TOCTRL_GET_AF_SUPPORT, &af_support, NULL);

    sprddefInfo->af_support = af_support;

    if (mRecordingMode == false && sprddefInfo->sprd_zsl_enabled == 1) {
        mSprdZslEnabled = true;
    } else if (mRecordingMode == false && mIsIspToolMode == 1) {
        mSprdZslEnabled = false;
    } else if (mSprdAppmodeId == CAMERA_MODE_SLOWMOTION) {
        mSprdZslEnabled = false;
    } else if (mRecordingMode == false && mStreamOnWithZsl == 1) {
        mSprdZslEnabled = true;
    } else if ((mRecordingMode == true && sprddefInfo->slowmotion > 1) ||
               (mRecordingMode == true && mVideoSnapshotType == 1)) {
        mSprdZslEnabled = false;
        if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_VIDEO_HW) {
            // 1 for 3dnr hw process.
            if (grab_capability.support_3dnr_mode == 1) {
                // TBD: check the code later
            } else {
                mVideo3dnrFlag = VIDEO_ON;
                camera_ioctrl(CAMERA_IOCTRL_3DNR_VIDEOMODE, &mVideo3dnrFlag,
                              NULL);
            }
        }
    } else if (mRecordingMode == true && mVideoWidth != 0 &&
               mVideoHeight != 0 && mCaptureWidth != 0 && mCaptureHeight != 0) {
        mSprdZslEnabled = true;
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

    mTopAppId = sprddefInfo->top_app_id;
    if (mTopAppId == TOP_APP_WECHAT) {
        faceDectect(1);
        if (mPreviewWidth == 640 && mPreviewHeight == 480 &&
            mCallbackWidth == 640 && mCallbackHeight == 480) {
            property_get("persist.vendor.wechat.videocall.facebeauty", value,
                         "off");
            if (!strcmp(value, "on")) {
                mChannel2FaceBeautyFlag = 1;
            }
        }
    }

    if (!initPreview()) {
        HAL_LOGE("initPreview failed");
        deinitPreview();
        return UNKNOWN_ERROR;
    }

    mSetting->getFLASHINFOTag(&flashInfo);
    if (flashInfo.available) {
        SprdCamera3Flash::reserveFlash(mCameraId);
    }
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

    HAL_LOGD("mVideoAFBCFlag=%d", mVideoAFBCFlag);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_AFBC_ENABLED,
             (cmr_uint)mVideoAFBCFlag);

    if (sprddefInfo->sprd_3dcapture_enabled) {
        mZslNum = DUALCAM_ZSL_NUM;
        mZslMaxFrameNum = DUALCAM_MAX_ZSL_NUM;
    }

    if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
        mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {
        mZslNum = 5;
        mZslMaxFrameNum = 5;
    }

/* for sharkle auto3dnr */
#ifdef CONFIG_ISP_2_3
    if (mSprdAppmodeId == CAMERA_MODE_AUTO_PHOTO) {
        mZslNum = 5;
        mZslMaxFrameNum = 5;
    }
#endif

    if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW) {
        mVideoProcessedWithPreview = true;
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
    if ((getMultiCameraMode() == MODE_BOKEH ||
         getMultiCameraMode() == MODE_3D_CALIBRATION) &&
        mCameraId == sensorGetRole(MODULE_SPW_NONE_BACK)) {
        setCameraConvertCropRegion();
        property_get("persist.vendor.cam.focus.distance", prop, "0");
        if (atoi(prop))
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_LENS_FOCUS_DISTANCE,
                     (cmr_uint)(atoi(prop)));
    }

    setCameraState(SPRD_INTERNAL_PREVIEW_REQUESTED, STATE_PREVIEW);
    mPipelineStartSignal.signal();

    setAeState(AE_START);
    setAwbState(AWB_START);

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

    HAL_LOGI("X CamId=%d TotalIonSize=%d TotalGpuSize=%d",
              mCameraId, mTotalIonSize, mTotalGpuSize);

    return NO_ERROR;
}

void SprdCamera3OEMIf::stopPreviewInternal() {
    ATRACE_CALL();

    nsecs_t start_timestamp = systemTime();
    nsecs_t end_timestamp = systemTime();
    int ret = NO_ERROR;
    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    HAL_LOGI("E mCameraId=%d", mCameraId);
    mIsStoppingPreview = 1;
    mZslCaptureExitLoop = true;

    if (isCapturing()) {
        cancelPictureInternal();
    }

    if (isPreviewStart()) {
        HAL_LOGV("Preview not start! wait preview start");
        WaitForPreviewStart();
    }

    if (isPreviewIdle()) {
        HAL_LOGD("Preview is idle! stopPreviewInternal X");
        goto exit;
    }

    setCameraState(SPRD_INTERNAL_PREVIEW_STOPPING, STATE_PREVIEW);

    if (SPRD_FOCUS_IN_PROGRESS == mCameraState.focus_state) {
        Mutex::Autolock stateLock(&mStateLock);
        while (SPRD_IDLE != mCameraState.focus_state) {
            HAL_LOGD("waiting for SPRD_IDLE");
            if (mStateWait.waitRelative(mStateLock, DO_AF_TIMEOUT)) {
                HAL_LOGE("timeout");
                break;
            }
            HAL_LOGD("focus state: SPRD_IDLE");
        }
    }

    if (CMR_CAMERA_SUCCESS !=
        mHalOem->ops->camera_stop_preview(mCameraHandle)) {
        setCameraState(SPRD_ERROR, STATE_PREVIEW);
        HAL_LOGE("camera_stop_preview failed");
    }
    if (sprddefInfo->high_resolution_mode && sprddefInfo->fin1_highlight_mode) {
        mSprdZslEnabled = 0;
        HAL_LOGD("mSprdZslEnabled=%d", mSprdZslEnabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
                 (cmr_uint)mSprdZslEnabled);
    }

    deinitPreview();
    end_timestamp = systemTime();

    setCameraState(SPRD_IDLE, STATE_PREVIEW);

#ifdef CONFIG_FACE_BEAUTY
    if (mflagfb) {
        mflagfb = false;
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_FAST_STOP_CMD,NULL);
        face_beauty_deinit(&face_beauty);
    }
#endif
    // used for single camera need raw stream
    // freeRawBuffers();

exit:
    mIsStoppingPreview = 0;
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

    HAL_LOGV("X");
    return result ? NO_ERROR : UNKNOWN_ERROR;
}

void SprdCamera3OEMIf::setCameraPrivateData() {}

void SprdCamera3OEMIf::getPictureFormat(int *format) {
    *format = mPictureFormat;
}

int SprdCamera3OEMIf::CameraConvertCoordinateToFramework(int32_t *cropRegion) {

    float left = 0, top = 0, width = 0, height = 0, zoomWidth = 0,
          zoomHeight = 0;
    uint32_t i = 0;
    int ret = 0;
    int flag_square = 0;
    SCALER_Tag scaleInfo;
    struct img_rect scalerCrop;
    uint16_t picW = 0, picH = 0, fdWid = 0, fdHeight = 0;

    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return 1;
    }
    if (fdWid == fdHeight)
        flag_square = 1;
    mSetting->getSCALERTag(&scaleInfo);
    HAL_LOGD("mCameraId=%d,face rect %d %d %d %d, scaleInfo crop %d %d %d %d",
             mCameraId, cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3], scaleInfo.crop_region[0], scaleInfo.crop_region[1],
             scaleInfo.crop_region[2], scaleInfo.crop_region[3]);
    scalerCrop.start_x = scaleInfo.crop_region[0];
    scalerCrop.start_y = scaleInfo.crop_region[1];
    scalerCrop.width = scaleInfo.crop_region[2];
    scalerCrop.height = scaleInfo.crop_region[3];
    // changed code hare to handle crop reagion 0 ,0 ,0 ,0
    if (scalerCrop.width == 0 || scalerCrop.height == 0) {
        mSetting->getLargestPictureSize(mCameraId, &picW, &picH);
        scalerCrop.width = picW;
        scalerCrop.height = picH;
    }
    float previewAspect = (float)mPreviewWidth / mPreviewHeight;
    float cropAspect = (float)scalerCrop.width / scalerCrop.height;
    if (previewAspect > cropAspect) {
        width = scalerCrop.width;
        height = cropAspect * scalerCrop.height / previewAspect;
        left = scalerCrop.start_x;
        top = scalerCrop.start_y + (scalerCrop.height - height) / 2;
    } else {
        width = previewAspect * scalerCrop.width / cropAspect;
        height = scalerCrop.height;
        left = scalerCrop.start_x + (scalerCrop.width - width) / 2;
        top = scalerCrop.start_y;
    }

    zoomWidth = width / (float)mPreviewWidth;
    zoomHeight = height / (float)mPreviewHeight;
    HAL_LOGD("mCameraId=%d, previewAspect=%f, cropAspect=%f,"
            "width=%f, height=%f, left=%f, top=%f, zoomWidth=%f, zoomHeight=%f",
            mCameraId, previewAspect, cropAspect, width, height, left, top,
            zoomWidth, zoomHeight);
    cropRegion[0] = (cmr_u32)((float)cropRegion[0] * zoomWidth + left);
    cropRegion[1] = (cmr_u32)((float)cropRegion[1] * zoomHeight + top);
    cropRegion[2] = (cmr_u32)((float)cropRegion[2] * zoomWidth + left);
    cropRegion[3] = (cmr_u32)((float)cropRegion[3] * zoomHeight + top);
    //for facebeauty, face crop must be square
    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (flag_square == 1){
        HAL_LOGD("mCameraId=%d, Crop calculated before correct "
             "(xs=%d,ys=%d,xe=%d,ye=%d, )",
             mCameraId, cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3]);
        if (fdWid > fdHeight){
           cropRegion[2] = cropRegion[0] +fdHeight;
        }else if (fdHeight > fdWid){
           cropRegion[3] = cropRegion[1]+ fdWid;
        }
        flag_square = 0;
}
    HAL_LOGD("mCameraId=%d, crop_face_rect calculated "
             "(xs=%d,ys=%d,xe=%d,ye=%d, )",
             mCameraId, cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3]);
    return ret;
}

int SprdCamera3OEMIf::CameraConvertRegionFromFramework(int32_t *cropRegion) {
    float left = 0, top = 0, width = 0, height = 0, zoomWidth = 0,
          zoomHeight = 0;
    uint32_t i = 0;
    int ret = 0;
    SCALER_Tag scaleInfo;
    struct img_rect scalerCrop;
    uint16_t sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("mPreviewWidth = %d, mPreviewHeight = %d, crop %d %d %d %d",
             mPreviewWidth, mPreviewHeight, cropRegion[0], cropRegion[1],
             cropRegion[2], cropRegion[3]);
    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return 1;
    }
    mSetting->getSCALERTag(&scaleInfo);
    scalerCrop.start_x = scaleInfo.crop_region[0];
    scalerCrop.start_y = scaleInfo.crop_region[1];
    scalerCrop.width = scaleInfo.crop_region[2];
    scalerCrop.height = scaleInfo.crop_region[3];
    float previewAspect = (float)mPreviewWidth / mPreviewHeight;
    float cropAspect = (float)scalerCrop.width / scalerCrop.height;
    if (previewAspect > cropAspect) {
        width = scalerCrop.width;
        height = cropAspect * scalerCrop.height / previewAspect;
        left = scalerCrop.start_x;
        top = scalerCrop.start_y + (scalerCrop.height - height) / 2;
    } else {
        width = previewAspect * scalerCrop.width / cropAspect;
        height = scalerCrop.height;
        left = scalerCrop.start_x + (scalerCrop.width - width) / 2;
        top = scalerCrop.start_y;
    }
    zoomWidth = (float)mPreviewWidth / width;
    zoomHeight = (float)mPreviewHeight / height;
    cropRegion[0] = (cmr_u32)(((float)cropRegion[0] - left) * zoomWidth);
    cropRegion[1] = (cmr_u32)(((float)cropRegion[1] - top) * zoomHeight);
    cropRegion[2] = (cmr_u32)(((float)cropRegion[2] - left) * zoomWidth);
    cropRegion[3] = (cmr_u32)(((float)cropRegion[3] - top) * zoomHeight);
    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d, )", cropRegion[0],
             cropRegion[1], cropRegion[2], cropRegion[3]);
    return ret;
}

int SprdCamera3OEMIf::CameraConvertCoordinateFromFramework(
    int32_t *cropRegion) {
    float left = 0, top = 0, width = 0, height = 0, zoomWidth = 0,
          zoomHeight = 0;
    uint32_t i = 0;
    int ret = 0;
    int flag_square = 0;
    SCALER_Tag scaleInfo;
    struct img_rect scalerCrop;
    uint16_t sensorOrgW = 0, sensorOrgH = 0, fdWid = 0, fdHeight = 0;
    HAL_LOGD("mPreviewWidth = %d, mPreviewHeight = %d, crop %d %d %d %d",
             mPreviewWidth, mPreviewHeight, cropRegion[0], cropRegion[1],
             cropRegion[2], cropRegion[3]);
    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (fdWid == 0 || fdHeight == 0) {
        HAL_LOGE("parameters error.");
        return 1;
    }
    if (fdWid == fdHeight)
        flag_square = 1;
    mSetting->getSCALERTag(&scaleInfo);
    scalerCrop.start_x = scaleInfo.crop_region[0];
    scalerCrop.start_y = scaleInfo.crop_region[1];
    scalerCrop.width = scaleInfo.crop_region[2];
    scalerCrop.height = scaleInfo.crop_region[3];
    float previewAspect = (float)mPreviewWidth / mPreviewHeight;
    float cropAspect = (float)scalerCrop.width / scalerCrop.height;
    if (previewAspect > cropAspect) {
        width = scalerCrop.width;
        height = cropAspect * scalerCrop.height / previewAspect;
        left = scalerCrop.start_x;
        top = scalerCrop.start_y + (scalerCrop.height - height) / 2;
    } else {
        width = previewAspect * scalerCrop.width / cropAspect;
        height = scalerCrop.height;
        left = scalerCrop.start_x + (scalerCrop.width - width) / 2;
        top = scalerCrop.start_y;
    }
    zoomWidth = (float)mPreviewWidth / width;
    zoomHeight = (float)mPreviewHeight / height;
    cropRegion[0] = (cmr_u32)(((float)cropRegion[0] - left) * zoomWidth);
    cropRegion[1] = (cmr_u32)(((float)cropRegion[1] - top) * zoomHeight);
    cropRegion[2] = (cmr_u32)(((float)cropRegion[2] - left) * zoomWidth);
    cropRegion[3] = (cmr_u32)(((float)cropRegion[3] - top) * zoomHeight);
    //for facebeauty, face crop must be square
    fdWid = cropRegion[2] - cropRegion[0];
    fdHeight = cropRegion[3] - cropRegion[1];
    if (flag_square == 1){
         HAL_LOGD("mCameraId=%d, crop_face_rect calculated before correct"
             "(xs=%d,ys=%d,xe=%d,ye=%d, )",
             mCameraId, cropRegion[0], cropRegion[1], cropRegion[2],
             cropRegion[3]);
        if (fdWid > fdHeight){
           cropRegion[2] = cropRegion[0] +fdHeight;
        }else if (fdHeight > fdWid){
           cropRegion[3] = cropRegion[1]+ fdWid;
        }
        flag_square = 0;
}

    HAL_LOGD("Crop calculated (xs=%d,ys=%d,xe=%d,ye=%d, )", cropRegion[0],
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
    FACE_Tag orifaceInfo;
#ifdef CONFIG_CAMERA_EIS
    EIS_CROP_Tag eiscrop_Info;
    SPRD_DEF_Tag *sprddefInfo;
#endif

    ssize_t offset = frame->buf_id;
    // camera_frame_metadata_t metadata;
    // camera_face_t face_info[FACE_DETECT_NUM];
    int32_t k = 0;
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t ex = 0;
    int32_t ey = 0;
    int32_t fd_score[10];
    struct img_rect rect = {0, 0, 0, 0};
    mSetting->getFACETag(&faceInfo);
#ifdef CONFIG_CAMERA_EIS
    int32_t delta_w = 0;
    int32_t delta_h = 0;
    mSetting->getEISCROPTag(&eiscrop_Info);
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
#endif
    memset(&faceInfo, 0, sizeof(FACE_Tag));
    memset(&orifaceInfo, 0, sizeof(FACE_Tag));
    HAL_LOGV("receive face_num %d.mid=%d", frame->face_num, mCameraId);
    int32_t number_of_faces =
        frame->face_num <= FACE_DETECT_NUM ? frame->face_num : FACE_DETECT_NUM;
    faceInfo.face_num = number_of_faces;
    orifaceInfo.face_num = number_of_faces;

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
#ifdef CONFIG_CAMERA_EIS
            if(sprddefInfo->sprd_eis_enabled) {
               delta_w = ((ex - sx)*frame->width)/(eiscrop_Info.crop[2] - eiscrop_Info.crop[0]);
               delta_h = ((ey - sy)*frame->height)/(eiscrop_Info.crop[3] - eiscrop_Info.crop[1]);
               faceInfo.face[k].rect[0] = ((sx - eiscrop_Info.crop[0])*frame->width)/(eiscrop_Info.crop[2] - eiscrop_Info.crop[0]);
               faceInfo.face[k].rect[1] = ((sy - eiscrop_Info.crop[1])*frame->height)/(eiscrop_Info.crop[3] - eiscrop_Info.crop[1]);
               faceInfo.face[k].rect[2] = faceInfo.face[k].rect[0] + delta_w;
               faceInfo.face[k].rect[3] = faceInfo.face[k].rect[1] + delta_h;
            }else {
               faceInfo.face[k].rect[0] = sx;
               faceInfo.face[k].rect[1] = sy;
               faceInfo.face[k].rect[2] = ex;
               faceInfo.face[k].rect[3] = ey;
            }
#else
            faceInfo.face[k].rect[0] = sx;
            faceInfo.face[k].rect[1] = sy;
            faceInfo.face[k].rect[2] = ex;
            faceInfo.face[k].rect[3] = ey;
#endif
            faceInfo.angle[k] = frame->face_info[k].angle;
            faceInfo.pose[k] = frame->face_info[k].pose;
            memcpy(&orifaceInfo.face[k], &faceInfo.face[k],
                   sizeof(camera_face_t));
            orifaceInfo.angle[k] = frame->face_info[k].angle;
            orifaceInfo.pose[k] = frame->face_info[k].pose;
            fd_score[k] = frame->face_info[k].score;

            HAL_LOGD("smile level %d. face:%d  %d  %d  %d ,angle %d,fd_score %d\n",
                     frame->face_info[k].smile_level, faceInfo.face[k].rect[0],
                     faceInfo.face[k].rect[1], faceInfo.face[k].rect[2],
                     faceInfo.face[k].rect[3], faceInfo.angle[k], fd_score[k]);
            CameraConvertCoordinateToFramework(faceInfo.face[k].rect);
            /*When the half of face at the edge of the screen,the smile level
            returned by face detection library  can often more than 30.
            In order to repaier this defetion ,so when the face on the screen is
            too close to the edge of screen, the smile level will be set to 0.
            */
            faceInfo.face[k].score = frame->face_info[k].smile_level;
            if (faceInfo.face[k].score < 0)
                faceInfo.face[k].score = 0;
            faceInfo.gender_age_race[k] = frame->face_info[k].gender_age_race;
        }
    }
    mSetting->setORIFACETag(&orifaceInfo);
    mSetting->setFACETag(&faceInfo);
    mSetting->setFdScore(fd_score, number_of_faces);
}

void SprdCamera3OEMIf::receivePreviewATFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();

    if (NULL == frame) {
        HAL_LOGE("invalid frame pointer");
        return;
    }
    Mutex::Autolock l(&mPreviewCbLock);
    float zoomWidth = 0.0f, zoomHeight = 0.0f;
    uint16_t sensorOrgW = 0, sensorOrgH = 0;
    AUTO_TRACKING_Tag autotrackingInfo;

    mSetting->getAUTOTRACKINGTag(&autotrackingInfo);

    HAL_LOGD("frame coordinate x=%d y=%d, status=%d", frame->at_cb_info.objectX,
             frame->at_cb_info.objectY, frame->at_cb_info.status);

    // Do coordinate transition
    zoomWidth = autotrackingInfo.w_ratio;
    zoomHeight = autotrackingInfo.h_ratio;
    if (0 != zoomWidth && 0 != zoomHeight) {
        autotrackingInfo.at_cb_info[1] = frame->at_cb_info.objectX / zoomWidth;
        autotrackingInfo.at_cb_info[2] = frame->at_cb_info.objectY / zoomHeight;
    } else {
        autotrackingInfo.at_cb_info[1] = 0;
        autotrackingInfo.at_cb_info[2] = 0;
        HAL_LOGE("invalid zoom ratio");
    }
    autotrackingInfo.at_cb_info[0] = frame->at_cb_info.status;
    if (autotrackingInfo.at_cb_info[0] == 2 &&
        autotrackingInfo.at_cb_info[1] == 0 &&
        autotrackingInfo.at_cb_info[2] == 0)
        autotrackingInfo.at_cb_info[0] = 0;

    HAL_LOGD("cb coordinate status=%d x=%d y=%d,",
             autotrackingInfo.at_cb_info[0], autotrackingInfo.at_cb_info[1],
             autotrackingInfo.at_cb_info[2]);

    mSetting->setAUTOTRACKINGTag(&autotrackingInfo);
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

    diff_timestamp = frm_timestamp - mSlowPara.last_frm_timestamp;
    // Google handle slowmotion timestamp at framework, therefore, we don't
    // multiply slowmotion ratio

    mSlowPara.rec_timestamp += diff_timestamp * tmp_slow_mot;
    mSlowPara.last_frm_timestamp = frm_timestamp;
}

bool SprdCamera3OEMIf::isFaceBeautyOn(SPRD_DEF_Tag *sprddefInfo) {
    for (int i = 0; i < SPRD_FACE_BEAUTY_PARAM_NUM; i++) {
        if (sprddefInfo->perfect_skin_level[i] != 0)
            return true;
    }
    return false;
}

void SprdCamera3OEMIf::receivePreviewFrame(struct camera_frame_type *frame) {
    ATRACE_CALL();

    Mutex::Autolock cbLock(&mPreviewCbLock);
    int ret = NO_ERROR;
    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    MLOG_Tag *mlogInfo;
    mSetting->getMLOGTag(&mlogInfo);

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
        HAL_LOGI("Launch Camera Time:%d(ms).", s_use_time);

        float cam_init_time;
        if (getApctCamInitSupport()) {
            cam_init_time =
                ((float)(systemTime() - cam_init_begin_time)) / 1000000000;
            writeCamInitTimeToProc(cam_init_time);
        }
        miSPreviewFirstFrame = 0;
    }

    if (mIsIspToolMode && frame->type == PREVIEW_FRAME) {
        send_img_data(ISP_TOOL_YVU420_2FRAME, mPreviewWidth, mPreviewHeight,
                      (char *)frame->y_vir_addr,
                      frame->width * frame->height * 3 / 2);
    }

    int64_t buffer_timestamp = frame->monoboottime;

    if (0 == buffer_timestamp)
        HAL_LOGE("buffer_timestamp shouldn't be 0,please check your code");

    VCM_Tag sprdvcmInfo;
    if (((mSprdRefocusEnabled == true || getMultiCameraMode() == MODE_BOKEH ||
          getMultiCameraMode() == MODE_MULTI_CAMERA) &&
         (mCameraId == 0 || mCameraId == 4)) ||
        getMultiCameraMode() == MODE_OPTICSZOOM_CALIBRATION) {
        mSetting->getVCMTag(&sprdvcmInfo);
        uint32_t vcm_step = 0;
        mHalOem->ops->camera_get_sensor_vcm_step(mCameraHandle, mCameraId,
                                                 &vcm_step);
        HAL_LOGD("mCameraId %d, mMultiCameraMode %d, vcm_step %d 0x%x",
                 mCameraId, mMultiCameraMode, vcm_step, vcm_step);
        sprdvcmInfo.vcm_step = vcm_step;
        mSetting->setVCMTag(sprdvcmInfo);
        sprdvcmInfo.vcm_step = vcm_step;
        mlogInfo->vcm_step = vcm_step;
   }

    if (mSprdRefocusEnabled == true && mSprdFullscanEnabled &&
        (getMultiCameraMode() == MODE_MULTI_CAMERA || mCameraId == 0 ||
         mCameraId == 4)) {
        struct vcm_range_info range;

        ret = mHalOem->ops->camera_ioctrl(mCameraHandle,
                CAMERA_IOCTRL_GET_CALIBRATION_VCMINFO, &range);
        mSetting->setVCMDACTag(range.vcm_dac, range.total_seg);
        mSprdFullscanEnabled = 0;
        if (mIsMlogMode && (getMultiCameraMode() == MODE_BOKEH ||
           getMultiCameraMode() == MODE_3D_CALIBRATION) && range.total_seg) {
            mlogInfo->vcm_dac = range.vcm_dac[range.total_seg - 1];
            mlogInfo->vcm_num = range.total_seg;
            HAL_LOGV("vcm dac %d num %d",mlogInfo->vcm_dac, mlogInfo->vcm_num);
        }
    }

    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    cmr_uint pre_addr_vir = 0, rec_addr_vir = 0, callback_addr_vir = 0;
    SprdCamera3Stream *pre_stream = NULL, *rec_stream = NULL,
                      *callback_stream = NULL, *yuv2_stream = NULL;
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
    int64_t tmp64;

    if (channel == NULL) {
        HAL_LOGE("channel=%p", channel);
        goto exit;
    }

    channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &pre_stream);
    channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &rec_stream);
    channel->getStream(CAMERA_STREAM_TYPE_CALLBACK, &callback_stream);
    channel->getStream(CAMERA_STREAM_TYPE_YUV2, &yuv2_stream);
    HAL_LOGV("pre_stream %p, rec_stream %p, callback_stream %p", pre_stream,
             rec_stream, callback_stream);

#ifdef CONFIG_FACE_BEAUTY
    struct faceBeautyLevels beautyLevels;
    struct facebeauty_param_info fb_param_map_prev;
    if (isFaceBeautyOn(sprddefInfo) && frame->type == PREVIEW_FRAME &&
        isPreviewing() && (sprddefInfo->sprd_appmode_id != CAMERA_MODE_AUTO_VIDEO)
        && (getMultiCameraMode() != MODE_BOKEH)
        && (getMultiCameraMode() != MODE_BLUR|| mIsPortraitScene==true)) {
        FACE_Tag faceInfo;
        fbBeautyFacetT beauty_face;
        fb_beauty_image_t beauty_image;
        cmr_u32 bv;
        cmr_u32 ct;
        cmr_u32 iso;
        CMR_LOGD("thomas face fb mode=%d",face_beauty.fb_mode);
        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_BV, &bv);
        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_CT, &ct);
        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_ISO, &iso);
        mSetting->getFACETag(&faceInfo);
        if (faceInfo.face_num > 0) {
            for (int i = 0; i < faceInfo.face_num; i++) {
                CameraConvertCoordinateFromFramework(faceInfo.face[i].rect);
                beauty_face.idx = i;
                beauty_face.startX = faceInfo.face[i].rect[0];
                beauty_face.startY = faceInfo.face[i].rect[1];
                beauty_face.endX = faceInfo.face[i].rect[2];
                beauty_face.endY = faceInfo.face[i].rect[3];
                beauty_face.angle = faceInfo.angle[i];
                beauty_face.pose = faceInfo.pose[i];
                ret = face_beauty_ctrl(
                    &face_beauty, FB_BEAUTY_CONSTRUCT_FACE_CMD, &beauty_face);
            }
        }
        beautyLevels.blemishLevel =
            (unsigned char)sprddefInfo->perfect_skin_level[0];
        beautyLevels.smoothLevel =
            (unsigned char)sprddefInfo->perfect_skin_level[1];
        beautyLevels.skinColor =
            (unsigned char)sprddefInfo->perfect_skin_level[2];
        beautyLevels.skinLevel =
            (unsigned char)sprddefInfo->perfect_skin_level[3];
        beautyLevels.brightLevel =
            (unsigned char)sprddefInfo->perfect_skin_level[4];
        beautyLevels.lipColor =
            (unsigned char)sprddefInfo->perfect_skin_level[5];
        beautyLevels.lipLevel =
            (unsigned char)sprddefInfo->perfect_skin_level[6];
        beautyLevels.slimLevel =
            (unsigned char)sprddefInfo->perfect_skin_level[7];
        beautyLevels.largeLevel =
            (unsigned char)sprddefInfo->perfect_skin_level[8];
        beautyLevels.cameraBV = (int)bv;
        beautyLevels.cameraWork = (int)mCameraId;
        beautyLevels.cameraCT = (int)ct;
        beautyLevels.cameraISO = (int)iso;
        HAL_LOGV("cameraBV %d, cameraWork %d, cameraCT %d, cameraISO %d",
            bv, mCameraId, ct, iso);

        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_FB_PARAM,
                       &fb_param_map_prev);
        if (ret == ISP_SUCCESS) {
            for(int i = 0; i < ISP_FB_SKINTONE_NUM; i++){
                HAL_LOGV("i %d blemishSizeThrCoeff %d removeBlemishFlag %d "
                    "lipColorType %d skinColorType %d", i,
                    fb_param_map_prev.cur.fb_param[i].blemishSizeThrCoeff,
                    fb_param_map_prev.cur.fb_param[i].removeBlemishFlag,
                    fb_param_map_prev.cur.fb_param[i].lipColorType,
                    fb_param_map_prev.cur.fb_param[i].skinColorType);
                HAL_LOGV("largeEyeDefaultLevel %d skinSmoothDefaultLevel %d "
                    "skinSmoothRadiusCoeffDefaultLevel %d",
                    fb_param_map_prev.cur.fb_param[i].fb_layer.largeEyeDefaultLevel,
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinSmoothDefaultLevel,
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinSmoothRadiusDefaultLevel);
                for(int j = 0; j < 11; j++){
                HAL_LOGV("i %d, j %d largeEyeLevel %d skinBrightLevel %d "
                    "skinSmoothRadiusCoeff %d", i, j,
                    fb_param_map_prev.cur.fb_param[i].fb_layer.largeEyeLevel[j],
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinBrightLevel[j],
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinSmoothRadiusCoeff[j]);
                }
            }
            ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_FACEMAP_CMD,
                                   &fb_param_map_prev);
        }

        if (!mflagfb) {
            face_beauty_set_devicetype(&face_beauty, SPRD_CAMALG_RUN_TYPE_CPU);

            fb_chipinfo chipinfo;
#if defined(CONFIG_ISP_2_3)
            chipinfo = SHARKLE;
#elif defined(CONFIG_ISP_2_4)
            chipinfo = PIKE2;
#elif defined(CONFIG_ISP_2_5)
            chipinfo = SHARKL3;
#elif defined(CONFIG_ISP_2_6)
            chipinfo = SHARKL5;
#elif defined(CONFIG_ISP_2_7)
            chipinfo = SHARKL5PRO;
#endif
            face_beauty_init(&face_beauty, 1, 2, chipinfo);
            if (face_beauty.hSprdFB != NULL) {
                mflagfb = true;
            }
        }
        invalidateCache(frame->fd, (void *)frame->y_vir_addr, 0,
                        frame->width * frame->height * 3 / 2);
        beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
        beauty_image.inputImage.addr[0] = (void *)frame->y_vir_addr;
        beauty_image.inputImage.addr[1] = (void *)frame->uv_vir_addr;
        beauty_image.inputImage.addr[2] = (void *)frame->uv_vir_addr;

        beauty_image.inputImage.ion_fd = frame->fd;
        beauty_image.inputImage.offset[0] = 0;
        beauty_image.inputImage.offset[1] = frame->width * frame->height;
        beauty_image.inputImage.width = frame->width;
        beauty_image.inputImage.height = frame->height;
        beauty_image.inputImage.stride = frame->width;
        beauty_image.inputImage.size = frame->width * frame->height * 3 / 2;

        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                               &beauty_image);
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                               &beautyLevels);
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_PROCESS_CMD,
                               &(faceInfo.face_num));
        flushIonBuffer(frame->fd, (void *)frame->y_vir_addr, 0,
                       frame->width * frame->height * 3 / 2);
    }  else if (mChannel2FaceBeautyFlag == 1 && frame->type == CHANNEL2_FRAME){
        FACE_Tag faceInfo;
        fbBeautyFacetT beauty_face;
        fb_beauty_image_t beauty_image;
        cmr_u32 bv;
        cmr_u32 ct;
        cmr_u32 iso;
        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_BV, &bv);
        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_CT, &ct);
        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_ISO, &iso);
        mSetting->getFACETag(&faceInfo);
        if (faceInfo.face_num > 0) {
            for (int i = 0; i < faceInfo.face_num; i++) {
                CameraConvertCoordinateFromFramework(faceInfo.face[i].rect);
                beauty_face.idx = i;
                beauty_face.startX = faceInfo.face[i].rect[0];
                beauty_face.startY = faceInfo.face[i].rect[1];
                beauty_face.endX = faceInfo.face[i].rect[2];
                beauty_face.endY = faceInfo.face[i].rect[3];
                beauty_face.angle = faceInfo.angle[i];
                beauty_face.pose = faceInfo.pose[i];
                ret = face_beauty_ctrl(
                    &face_beauty, FB_BEAUTY_CONSTRUCT_FACE_CMD, &beauty_face);
            }
        }

        // defalt beautyLevels for third app like wechat
        beautyLevels.blemishLevel = 0;
        beautyLevels.smoothLevel = 6;
        beautyLevels.skinColor = 0;
        beautyLevels.skinLevel = 0;
        beautyLevels.brightLevel = 6;
        beautyLevels.lipColor = 0;
        beautyLevels.lipLevel = 0;
        beautyLevels.slimLevel = 2;
        beautyLevels.largeLevel = 2;
        beautyLevels.cameraBV = (int)bv;
        beautyLevels.cameraWork = (int)mCameraId;
        beautyLevels.cameraCT = (int)ct;
        beautyLevels.cameraISO = (int)iso;
        HAL_LOGV("cameraBV %d, cameraWork %d, cameraCT %d, cameraISO %d",bv, mCameraId, ct, iso);

        ret = mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_FB_PARAM,
                       &fb_param_map_prev);
        if (ret == ISP_SUCCESS) {
            for(int i = 0; i < ISP_FB_SKINTONE_NUM; i++){
                HAL_LOGV("i %d blemishSizeThrCoeff %d removeBlemishFlag %d "
                    "lipColorType %d skinColorType %d", i,
                    fb_param_map_prev.cur.fb_param[i].blemishSizeThrCoeff,
                    fb_param_map_prev.cur.fb_param[i].removeBlemishFlag,
                    fb_param_map_prev.cur.fb_param[i].lipColorType,
                    fb_param_map_prev.cur.fb_param[i].skinColorType);
                HAL_LOGV("largeEyeDefaultLevel %d skinSmoothDefaultLevel %d "
                    "skinSmoothRadiusCoeffDefaultLevel %d",
                    fb_param_map_prev.cur.fb_param[i].fb_layer.largeEyeDefaultLevel,
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinSmoothDefaultLevel,
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinSmoothRadiusDefaultLevel);
                for(int j = 0; j < 11; j++){
                HAL_LOGV("i %d, j %d largeEyeLevel %d skinBrightLevel %d "
                    "skinSmoothRadiusCoeff %d", i, j,
                    fb_param_map_prev.cur.fb_param[i].fb_layer.largeEyeLevel[j],
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinBrightLevel[j],
                    fb_param_map_prev.cur.fb_param[i].fb_layer.skinSmoothRadiusCoeff[j]);
                }
            }
            ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_FACEMAP_CMD,
                                   &fb_param_map_prev);
        }

        if (!mflagfb) {
#ifdef CONFIG_SPRD_FB_VDSP_SUPPORT
            face_beauty_set_devicetype(&face_beauty, SPRD_CAMALG_RUN_TYPE_VDSP);
#else
            face_beauty_set_devicetype(&face_beauty, SPRD_CAMALG_RUN_TYPE_CPU);
#endif

            fb_chipinfo chipinfo;
#if defined(CONFIG_ISP_2_3)
            chipinfo = SHARKLE;
#elif defined(CONFIG_ISP_2_4)
            chipinfo = PIKE2;
#elif defined(CONFIG_ISP_2_5)
            chipinfo = SHARKL3;
#elif defined(CONFIG_ISP_2_6)
            chipinfo = SHARKL5;
#elif defined(CONFIG_ISP_2_7)
            chipinfo = SHARKL5PRO;
#endif
            face_beauty_init(&face_beauty, 1, 2, chipinfo);
            if (face_beauty.hSprdFB != NULL) {
                mflagfb = true;
            }
        }
        invalidateCache(frame->fd, (void *)frame->y_vir_addr, 0,
                        frame->width * frame->height * 3 / 2);
        beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
        beauty_image.inputImage.addr[0] = (void *)frame->y_vir_addr;
        beauty_image.inputImage.addr[1] = (void *)frame->uv_vir_addr;
        beauty_image.inputImage.addr[2] = (void *)frame->uv_vir_addr;
        beauty_image.inputImage.ion_fd = frame->fd;
        beauty_image.inputImage.offset[0] = 0;
        beauty_image.inputImage.offset[1] = frame->width * frame->height;
        beauty_image.inputImage.width = frame->width;
        beauty_image.inputImage.height = frame->height;
        beauty_image.inputImage.stride = frame->width;
        beauty_image.inputImage.size = frame->width * frame->height * 3 / 2;
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                               &beauty_image);
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                               &beautyLevels);
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_PROCESS_CMD,
                               &(faceInfo.face_num));
        flushIonBuffer(frame->fd, (void *)frame->y_vir_addr, 0,
                       frame->width * frame->height * 3 / 2);

    } else {
        if (frame->type != PREVIEW_ZSL_FRAME &&
            frame->type != PREVIEW_CANCELED_FRAME &&
            frame->type != CHANNEL2_FRAME &&
            frame->type != PREVIEW_VIDEO_FRAME &&
            frame->type != PREVIEW_FRAME && mflagfb) {
            mflagfb = false;
            ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_FAST_STOP_CMD,NULL);
            face_beauty_deinit(&face_beauty);
        }
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

        ATRACE_BEGIN("video_frame");
        HAL_LOGD("record:mCameraId = %d, fd = 0x%x, vir = 0x%lx, num = %d"
                 ", time = %" PRId64 ", rec = %" PRId64,
                 mCameraId, frame->fd, buff_vir, frame_num, buffer_timestamp,
                 mSlowPara.rec_timestamp);
        if (frame->type == PREVIEW_VIDEO_FRAME) {
            if (mVideoWidth <= mCaptureWidth &&
                mVideoHeight <= mCaptureHeight) {
                HAL_LOGD(
                    "mDualVideoMode %d, mVideoShotFlag = %d mDualVideoShotFlag %d",
                    mDualVideoMode, mVideoShotFlag, mDualVideoShotFlag);
		HAL_LOGD(
                    "frame_num %d mVideoSnapshotFrameNum %d",
                    frame_num, mVideoSnapshotFrameNum);
                if (mDualVideoMode) {
                    if (mVideoShotFlag && mDualVideoShotFlag &&
                        (frame_num >= mVideoSnapshotFrameNum))
                        PushVideoSnapShotbuff(frame_num,
                                              CAMERA_STREAM_TYPE_VIDEO);
                } else {
                    if (mVideoShotFlag && (frame_num >= mVideoSnapshotFrameNum))
                        PushVideoSnapShotbuff(frame_num,
                                              CAMERA_STREAM_TYPE_VIDEO);
                }
            }

            if (mSlowPara.last_frm_timestamp == 0) { /*record first frame*/
                mSlowPara.last_frm_timestamp = buffer_timestamp;
                mSlowPara.rec_timestamp = buffer_timestamp;
                mIsRecording = true;
            }
            calculateTimestampForSlowmotion(buffer_timestamp);

#ifdef CONFIG_CAMERA_EIS
            vsOutFrame frame_out;
            frame_out.frame_data = NULL;
            HAL_LOGV("eis_enable = %d", sprddefInfo->sprd_eis_enabled);
            if (sprddefInfo->sprd_eis_enabled) {
                // camera exit/switch dont need to do eis
                if (mFlush == 0) {
                    Mutex::Autolock l(&mEisVideoProcessLock);
                    frame_out = EisVideoFrameStab(frame, frame_num);
                }
                if (frame_out.frame_data) {
                    channel->channelCbRoutine(frame_out.frame_num, frame_out.timestamp*1000000000,
                                                    CAMERA_STREAM_TYPE_VIDEO);
                }
                HAL_LOGV("video callback frame vir address=0x%lx,frame_num=%d",
                          frame_out.frame_data, frame_out.frame_num);
                goto bypass_rec;
            }
#endif
            channel->channelCbRoutine(frame_num, mSlowPara.rec_timestamp,
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
        HAL_LOGD("mCameraId=%d, prev:fd=%d, vir=0x%lx, num=%d, width=%d, "
                 "height=%d, time=%" PRId64,
                 mCameraId, frame->fd, buff_vir, frame_num, frame->width,
                 frame->height, buffer_timestamp);

        if (mIsMlogMode) {
            if (mtimestamplast == 0) {
                mtimestamplast = frame->monoboottime / 1000000;
            } else {
                mlogInfo->prev_timestamp = frame->monoboottime / 1000000;
                tmp64 = mlogInfo->prev_timestamp - mtimestamplast;
                mtimestamplast = mlogInfo->prev_timestamp;
                if (tmp64)
                    mlogInfo->fps = 1000 / tmp64;
            }
        }
        if (!isCapturing() && mIsPowerhintWait && !mIsAutoFocus) {
            if ((frame_num > mStartFrameNum) &&
                (frame_num - mStartFrameNum > CAM_POWERHINT_WAIT_COUNT)) {
                if (getMultiCameraMode() == MODE_BLUR ||
                    getMultiCameraMode() == MODE_BOKEH ||
                    mSprdAppmodeId == CAMERA_MODE_PANORAMA ||
                    mSprdAppmodeId == CAMERA_MODE_3DNR_PHOTO ||
                    mSprdAppmodeId == CAMERA_MODE_FILTER ||
                    mSprdAppmodeId == -1 ||
                    (mRecordingMode && !mVideoWidth && !mVideoHeight)) {
                    setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_4);
                } else if (mSprdAppmodeId == CAMERA_MODE_CONTINUE ||
                           sprddefInfo->slowmotion > 1) {
                    setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
                } else if (mRecordingMode == true) {
                    setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_3);
                } else if (getMultiCameraMode() != MODE_SINGLE_FACEID_UNLOCK) {
                    setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_1);
                }
                mIsPowerhintWait = 0;
            }
        }

        if (frame->type == PREVIEW_FRAME) {
#ifdef CONFIG_CAMERA_EIS
            HAL_LOGV("eis_enable = %d", sprddefInfo->sprd_eis_enabled);
            if (sprddefInfo->sprd_eis_enabled) {
                // camera exit/switch dont need to do eis
                if (mFlush == 0) {
                    Mutex::Autolock l(&mEisPreviewProcessLock);
                    EisPreviewFrameStab(frame);
                }
            }
#endif

            if (mVideoCopyFromPreviewFlag || mVideoProcessedWithPreview) {
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

            if (mIsRecording && rec_stream) {
                calculateTimestampForSlowmotion(buffer_timestamp);
                if (mVideoCopyFromPreviewFlag || mVideoProcessedWithPreview) {
                    ret = rec_stream->getQBufAddrForNum(
                        frame_num, &videobuf_vir, &videobuf_phy, &fd0);
                    if (ret || videobuf_vir == 0) {
                        HAL_LOGE("getQBufAddrForNum failed");
                        goto exit;
                    }
                    pre_stream->getQBufAddrForNum(frame_num, &prebuf_vir,
                                                  &prebuf_phy, &fd1);
                    HAL_LOGV("frame_num=%d, videobuf_phy=0x%lx, "
                             "videobuf_vir=0x%lx,fd=0x%x",
                             frame_num, videobuf_phy, videobuf_vir, fd0);
                    HAL_LOGV("frame_num=%d, prebuf_phy=0x%lx, prebuf_vir=0x%lx",
                             frame_num, prebuf_phy, prebuf_vir);
			if (mVideoCopyFromPreviewFlag) {
                            memcpy((void *)videobuf_vir, (void *)prebuf_vir,
                               mPreviewWidth * mPreviewHeight * 3 / 2);
                    }
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

        HAL_LOGD("callback fd=0x%x, vir=0x%lx, frame_num %d, time %" PRId64
                 ", frame type = %ld",
                 frame->fd, buff_vir, frame_num, buffer_timestamp, frame->type);

        channel->channelCbRoutine(frame_num, buffer_timestamp,
                                  CAMERA_STREAM_TYPE_CALLBACK);

        if ((mTakePictureMode == SNAPSHOT_PREVIEW_MODE) &&
            (isJpegRequest == true)) {
            timer_set(this, 1, timer_hand_take);
        }
        ATRACE_END();

    bypass_callback:
        HAL_LOGV("callback_stream X");
    }

    if (yuv2_stream) {
        ret = yuv2_stream->getQBufNumForVir(buff_vir, &frame_num);
        if (ret) {
            goto bypass_yuv2;
        }

        HAL_LOGD("yuv2 fd=0x%x, vir=0x%lx, frame_num %d, time %" PRId64
                 ", frame type = %ld",
                 frame->fd, buff_vir, frame_num, buffer_timestamp, frame->type);
        channel->channelCbRoutine(frame_num, buffer_timestamp,
                                  CAMERA_STREAM_TYPE_YUV2);
    bypass_yuv2:
        HAL_LOGV("yuv2_stream X");
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
    /* mlog */
    if (mIsMlogMode) {
        ret = gatherInfoForMlog();
        if (!ret)
            saveMlogInfo();
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
    int64_t timestamp = frame->monoboottime;

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
            struct img_frm src, dst;
            struct img_frm *pScale[2];

            src.addr_phy.addr_y = (cmr_uint)frame->y_phy_addr;
            src.addr_phy.addr_u =
                src.addr_phy.addr_y + frame->width * frame->height;
            src.addr_vir.addr_y = (cmr_uint)frame->y_vir_addr;
            src.addr_vir.addr_u =
                src.addr_vir.addr_y + frame->width * frame->height;
            src.buf_size = frame->width * frame->height * 3 >> 1;
            src.fd = frame->fd;
            src.fmt = CAM_IMG_FMT_YUV420_NV21;
            src.rect.start_x = 0;
            src.rect.start_y = 0;
            src.rect.width = frame->width;
            src.rect.height = frame->height;
            src.size.width = frame->width;
            src.size.height = frame->height;

            dst.addr_phy.addr_y = (cmr_uint)addr_phy;
            dst.addr_phy.addr_u =
                dst.addr_phy.addr_y + mPreviewWidth * mPreviewHeight;
            dst.addr_vir.addr_y = (cmr_uint)addr_vir;
            dst.addr_vir.addr_u =
                dst.addr_vir.addr_y + mPreviewWidth * mPreviewHeight;
            dst.buf_size = mPreviewWidth * mPreviewHeight * 3 >> 1;
            dst.fd = ion_fd;
            dst.fmt = CAM_IMG_FMT_YUV420_NV21;
            dst.rect.start_x = 0;
            dst.rect.start_y = 0;
            dst.rect.width = mPreviewWidth;
            dst.rect.height = mPreviewHeight;
            dst.size.width = mPreviewWidth;
            dst.size.height = mPreviewHeight;
            pScale[0] = &dst;
            pScale[1] = &src;
            mHalOem->ops->camera_ioctrl(mCameraHandle,
                                        CAMERA_IOCTRL_START_SCALE, pScale);
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
    HAL_LOGV("X");
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
    int64_t timestamp = frame->monoboottime;

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
    HAL_LOGV("X");
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
                            libyuv::kFilterBilinear);
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

    if (CAM_IMG_FMT_YUV420_NV21 == img_fmt || CAM_IMG_FMT_YUV422P == img_fmt) {
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

        if (CAM_IMG_FMT_YUV420_NV21 == img_fmt) {
            fwrite((void *)addr->addr_u, 1, width * height / 2, fp);
        } else {
            fwrite((void *)addr->addr_u, 1, width * height, fp);
        }
        fclose(fp);
    } else if (CAM_IMG_FMT_JPEG == img_fmt) {
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
    } else if (CAM_IMG_FMT_BAYER_MIPI_RAW == img_fmt) {
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
    cmr_u32 value = 0;

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

    //fovfusion mode tele need close hdr when hdr arithmetic done
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);

    if (controlInfo.scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR
        && mCameraId == sensorGetRole(MODULE_OPTICSZOOM_TELE_BACK)
        && mIsFovFusionMode == true) {
        mHalOem->ops->camera_ioctrl(mCameraHandle,
                                    CAMERA_IOCTRL_SET_HDR_DISABLE,
                                    &value);
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
    HAL_LOGV("X");
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
    int64_t exposureTime = 0;

    HAL_LOGD("E mCameraId = %d, encInfo->size = %d, enc->buffer = %p,"
             " encInfo->need_free = %d, time=%" PRId64,
             mCameraId, encInfo->size, encInfo->outPtr, encInfo->need_free,
             frame->timestamp);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (0 != frame->sensor_info.exposure_time_denominator) {
        exposureTime = 1000000000ll *
                       frame->sensor_info.exposure_time_numerator /
                       frame->sensor_info.exposure_time_denominator;
        mSetting->setExposureTimeTag(exposureTime);
    }
    timestamp = frame->monoboottime;

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
            int64_t timestamp1 = systemTime(SYSTEM_TIME_BOOTTIME);
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
            (!strcmp(debug_value, "debug")) || mIsRawCapture == 1) {
            struct img_addr vir_addr;
            vir_addr.addr_y = pic_addr_vir;
            mHalOem->ops->dump_image_with_isp_info(
                mCameraHandle, CAM_IMG_FMT_JPEG, mCaptureWidth, mCaptureHeight,
                encInfo->size + ispInfoSize, &vir_addr);
        }
    }

    jpegBlob = (camera3_jpeg_blob *)((char *)pic_addr_vir +
                                     (maxJpegSize - sizeof(camera3_jpeg_blob)));
    jpegBlob->jpeg_size = encInfo->size + ispInfoSize;
    jpegBlob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;

#ifdef SUPER_MACRO
    /* copy yuv for super macro capture */
    if (encInfo->super.addr != 0) {
        HAL_LOGV("enc_param: addr 0x%x, (%d, %d), size %d, dst addr 0x%x", encInfo->super.addr, encInfo->super.width,
            encInfo->super.height, encInfo->super.size, ((char *)pic_addr_vir + jpegBlob->jpeg_size));

        int value[3] = {0};
        if (jpegBlob->jpeg_size + encInfo->super.size < maxJpegSize - sizeof(camera3_jpeg_blob)) {
            memcpy(((char *)pic_addr_vir + jpegBlob->jpeg_size), (char *)encInfo->super.addr, encInfo->super.size);
            jpegBlob->jpeg_size += encInfo->super.size;
            jpegBlob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
        } else {
            HAL_LOGE("not enought buf: %d > %d", jpegBlob->jpeg_size + encInfo->super.size,
				maxJpegSize - sizeof(camera3_jpeg_blob));
        }
        memset(value, 0, 12);
        memcpy(&value[0], (char *)(pic_addr_vir + jpegBlob->jpeg_size - 12), sizeof(int));
        memcpy(&value[1], (char *)(pic_addr_vir + jpegBlob->jpeg_size - 8), sizeof(int));
        memcpy(&value[2], (char *)(pic_addr_vir + jpegBlob->jpeg_size - 4), sizeof(int));
        HAL_LOGV("width = %d, height = %d, size = %d", value[0], value[1], value[2]);
    }
#endif

    picChannel->channelCbRoutine(frame_num, timestamp,
                                 CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);

    if (mSprdReprocessing) {
        SprdCamera3RegularChannel *channel =
            reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        HAL_LOGD("jpeg encode done, reprocessing end");
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
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_3);
    } else if (mSprdAppmodeId == CAMERA_MODE_CONTINUE) {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_6);
    } else {
        setCamPreformaceScene(CAM_PERFORMANCE_LEVEL_1);
    }
#ifdef CONFIG_CAMERA_MM_DVFS_SUPPORT
    if (mSprdAppmodeId != CAMERA_MODE_CONTINUE) {
        mHalOem->ops->camera_set_mm_dvfs_policy(mCameraHandle, DVFS_ISP,
                                                IS_CAP_END);
    }
#endif
exit:
    HAL_LOGV("X");
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
        break;

    case CAMERA_EVT_CB_INVALIDATE_CACHE:
        ionBuf = (cam_ion_buffer_t *)parm4;
        if (ionBuf)
            invalidateCache(ionBuf->fd, ionBuf->addr_vir, ionBuf->addr_phy,
                            ionBuf->size);
        break;

    case CAMERA_RSP_CB_SUCCESS:
        if (mIsStoppingPreview == 1)
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

    case CAMERA_EVT_CB_AUTO_TRACKING:
        HAL_LOGV("CAMERA_EVT_CB_AUTO_TRACKING");
        if (isPreviewing()) {
            receivePreviewATFrame((struct camera_frame_type *)parm4);
        }
        break;

    case CAMERA_EXIT_CB_FAILED:
        HAL_LOGE("SprdCamera3OEMIf::camera_cb: @CAMERA_EXIT_CB_FAILURE(%p) in "
                 "state %s.",
                 parm4, getCameraStateStr(getPreviewState()));
        transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
        receiveCameraExitError();
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

exit:
    HAL_LOGV("out, state = %s", getCameraStateStr(getPreviewState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleStopPreview(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    Sprd_camera_state tmpPrevState = SPRD_IDLE;

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    HAL_LOGD("state = %s", getCameraStateStr(getPreviewState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleTakePicture(enum camera_cb_type cb, void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGD("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCaptureState()));
    bool encode_location = true;
    camera_position_type pt = {0, 0, 0, 0, NULL};
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
        if (checkPreviewStateForCapture()) {
            if (mTakePictureMode == SNAPSHOT_NO_ZSL_MODE ||
                mTakePictureMode == SNAPSHOT_DEFAULT_MODE ||
                (getMultiCameraMode() != MODE_SINGLE_CAMERA &&
                 mTakePictureMode == SNAPSHOT_ZSL_MODE)) {
                receiveRawPicture((struct camera_frame_type *)parm4);
            }
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
	 CONTROL_Tag controlInfo;
	 int8_t drvSceneMode = 0;
        mSetting->getCONTROLTag(&controlInfo);
        mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode, &drvSceneMode);
        if (isPreviewing() && iSZslMode() &&
            (mSprd3dCalibrationEnabled || mSprdYuvCallBack ||
             mMultiCameraMode == MODE_BLUR || mMultiCameraMode == MODE_BOKEH ||
             mMultiCameraMode == MODE_MULTI_CAMERA)) {
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
            HAL_LOGD("zsl_frame->fd=0x%x, drvSceneMode:%d", zsl_frame->fd, drvSceneMode);
            buf_id = getZslBufferIDForFd(zsl_frame->fd);
            if (buf_id != 0xFFFFFFFF && drvSceneMode != CAMERA_SCENE_MODE_FDR) {
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, mZslHeapArray[buf_id]->phys_addr,
                    (cmr_uint)mZslHeapArray[buf_id]->data,
                    mZslHeapArray[buf_id]->fd);
            }
        }
        break;
    }
    case CAMERA_EVT_CB_RETURN_SW_ALGORITHM_ZSL_BUF: {
        if (mFlush) {
            HAL_LOGD("mFlush=%d", mFlush);
            goto exit;
        }

        CONTROL_Tag controlInfo;
        mSetting->getCONTROLTag(&controlInfo);

        if (controlInfo.scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {
            for (i = 0; i < (cmr_int)mZslNum; i++) {
                for (int j = 0; j < 3; j++) {
                    if (mZslHeapArray[i]->fd == hdr_fd[j]) {
                        mHalOem->ops->camera_set_zsl_buffer(
                            mCameraHandle, mZslHeapArray[i]->phys_addr,
                            (cmr_uint)mZslHeapArray[i]->data, mZslHeapArray[i]->fd);
                    }
                }
            }
        } else if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
                        mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {
            for (i = 0; i < (cmr_int)mZslNum; i++) {
                mHalOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, mZslHeapArray[i]->phys_addr,
                    (cmr_uint)mZslHeapArray[i]->data, mZslHeapArray[i]->fd);
            }
        }
        break;
    }
    default: {
        HAL_LOGE("unkown cb = %d", cb);
        transitionState(getCaptureState(), SPRD_ERROR, STATE_CAPTURE);
        receiveTakePictureError();
        break;
    }
    }

exit:
    HAL_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleCancelPicture(enum camera_cb_type cb,
                                           void *parm4) {
    ATRACE_BEGIN(__FUNCTION__);

    HAL_LOGV("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
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
    CONTROL_Tag controlInfo;
    int8_t drvSceneMode = 0;
    mSetting->getCONTROLTag(&controlInfo);
    mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode, &drvSceneMode);

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
                if(mSprdAppmodeId == CAMERA_MODE_AUDIO_PICTURE &&
                        drvSceneMode == CAMERA_SCENE_MODE_FDR) {
                    HAL_LOGD("fdr mode, skip set zsl buffer");
                } else {
                    mHalOem->ops->camera_set_zsl_buffer(
                        mCameraHandle, mZslHeapArray[buf_id]->phys_addr,
                        (cmr_uint)mZslHeapArray[buf_id]->data,
                         mZslHeapArray[buf_id]->fd);
               }
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
    struct isp_af_notice *af_ctrl;
    cmr_int af_type = 0;

    SPRD_DEF_Tag *sprddefInfo;
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
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
            if (mIsAutoFocus) {
                setCamPreformaceScene(mGetLastPowerHint);
            }
            if (parm4 != NULL) {
                af_ctrl = (isp_af_notice *)parm4;
                if (af_ctrl->af_roi.sx != 0 || af_ctrl->af_roi.sy != 0 ||
                    af_ctrl->af_roi.ex != 0 || af_ctrl->af_roi.ey != 0) {
                    controlInfo.af_roi[0] = af_ctrl->af_roi.sx;
                    controlInfo.af_roi[1] = af_ctrl->af_roi.sy;
                    controlInfo.af_roi[2] = af_ctrl->af_roi.ex;
                    controlInfo.af_roi[3] = af_ctrl->af_roi.ey;
                    controlInfo.af_roi[4] = 1;
                    mSetting->setAfRoiCONTROLTag(&controlInfo);
                    break;
                }
            }

            if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO ||
                controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_MACRO) {
                setAfState(AF_SWEEP_DONE_AND_FOCUSED_LOCKED);
            } else if (controlInfo.af_mode ==
                           ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
                       controlInfo.af_mode ==
                           ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
                setAfState(AF_TRIGGER_START_AND_FOCUSED_LOCKED);
            }
            mLatestFocusDoneTime = systemTime(SYSTEM_TIME_BOOTTIME);

            if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO) {
                mIsAutoFocus = false;
            }
        }
        break;

    case CAMERA_EXIT_CB_ABORT:
    case CAMERA_EXIT_CB_FAILED: {
        if (mIsAutoFocus) {
            setCamPreformaceScene(mGetLastPowerHint);
        }
        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO ||
            controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_MACRO) {
            setAfState(AF_SWEEP_DONE_AND_NOT_FOCUSED_LOCKED);
        } else if (controlInfo.af_mode ==
                       ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
                   controlInfo.af_mode ==
                       ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
            setAfState(AF_TRIGGER_START_AND_NOT_FOCUSED_LOCKED);
        }

        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO) {
            mIsAutoFocus = false;
        }

    } break;

    case CAMERA_EVT_CB_FOCUS_MOVE:
        focus_status = (cmr_focus_status *)parm4;
        HAL_LOGV("parm4=%p autofocus=%d", parm4, mIsAutoFocus);
        if (!mIsAutoFocus && focus_status->af_focus_type == CAM_AF_FOCUS_CAF) {
            if (focus_status->is_in_focus) {
                setAfState(AF_INITIATES_NEW_SCAN);
                af_type = 0;
            } else {
                setAfState(AF_COMPLETES_CURRENT_SCAN);
                mLatestFocusDoneTime = systemTime(SYSTEM_TIME_BOOTTIME);
            }
        } else if (!mIsAutoFocus &&
                   focus_status->af_focus_type == CAM_AF_FOCUS_FAF) {
            if (!(focus_status->is_in_focus))
                af_type = focus_status->af_focus_type;
        }
        sprddefInfo->af_type = af_type;
        break;

    case CAMERA_EVT_CB_FOCUS_END: {
        focus_status = (cmr_focus_status *)parm4;
        cmr_u32 af_status = 1;
        VCM_Tag sprdvcmInfo;
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        if (getMultiCameraMode() == MODE_BOKEH && mCameraId == 0) {
            mSetting->getVCMTag(&sprdvcmInfo);
            HAL_LOGD("VCM_INFO:vcm step is %d", focus_status->af_motor_pos);
            sprdvcmInfo.vcm_step_for_bokeh = focus_status->af_motor_pos;
            mSetting->setVCMTag(sprdvcmInfo);
        }
        HAL_LOGD("CAMERA_EVT_CB_FOCUS_END focus_status->af_mode %d "
                 "mSprdRefocusEnabled %d mCameraId %d mSprdFullscanEnabled %d",
                 focus_status->af_mode, mSprdRefocusEnabled, mCameraId,
                 mSprdFullscanEnabled);
        if (mSprdRefocusEnabled == true &&
            (getMultiCameraMode() == MODE_MULTI_CAMERA || mCameraId == 0 ||
             mCameraId == 4) &&
            CAMERA_FOCUS_MODE_FULLSCAN == focus_status->af_mode) {
            mSprdFullscanEnabled = 1;
        }
        if (sprddefInfo->sprd_ot_switch == 1) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_STATUS_NOTIFY_TRACKING, af_status);
        }
    }
        break;

    default:
        HAL_LOGE("camera cb: unknown cb %d for CAMERA_FUNC_START_FOCUS!", cb);
        {
            if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO ||
                controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_MACRO) {
                setAfState(AF_SWEEP_DONE_AND_NOT_FOCUSED_LOCKED);
            } else if (controlInfo.af_mode ==
                           ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
                       controlInfo.af_mode ==
                           ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) {
                setAfState(AF_TRIGGER_START_AND_NOT_FOCUSED_LOCKED);
            }

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
    cmr_u32 ae_stab = 0;
    cmr_u32 *ae_info = NULL;

    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);
    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();

    HAL_LOGV("E: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getPreviewState()));

    switch (cb) {
    case CAMERA_EVT_CB_AE_STAB_NOTIFY:
        if (NULL != parm4) {
            ae_info = (cmr_u32 *)parm4;
            ae_stab = ae_info[AE_CB_STABLE_INDEX];
            HAL_LOGV("ae_info = %d, ae_stab = %d", ae_info, ae_stab);
        }

        if (ae_stab) {
            if (controlInfo.ae_comp_effect_frames_cnt != 0 && controlInfo.ae_lock) {
                setAeState(AE_LOCK_ON);
                goto exit;
            }
            if (!mIsNeedFlashFired) {
                setAeState(AE_STABLE);
            } else {
                setAeState(AE_STABLE_REQUIRE_FLASH);
            }
            setAwbState(AWB_STABLE);
        } else {
            setAeState(AE_START);
            setAwbState(AWB_START);
        }

        if (controlInfo.ae_state != ANDROID_CONTROL_AE_STATE_LOCKED) {
            // callback ae info
            for (int i = 0; i < AE_CB_MAX_INDEX; i++) {
                sprddefInfo->ae_info[i] = ae_info[i];
            }
        }
    exit:
        HAL_LOGV("CAMERA_EVT_CB_AE_STAB_NOTIFY");
        break;
    case CAMERA_EVT_CB_AE_LOCK_NOTIFY:
        // controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_LOCKED;
        // mSetting->setAeCONTROLTag(&controlInfo);
        HAL_LOGD("CAMERA_EVT_CB_AE_LOCK_NOTIFY");
        break;
    case CAMERA_EVT_CB_AE_UNLOCK_NOTIFY:
        // controlInfo.ae_state = ANDROID_CONTROL_AE_STATE_CONVERGED;
        // mSetting->setAeCONTROLTag(&controlInfo);
        HAL_LOGD("CAMERA_EVT_CB_AE_UNLOCK_NOTIFY");
        break;
    case CAMERA_EVT_CB_AE_FLASH_FIRED:
        if (parm4 != NULL) {
            mIsNeedFlashFired = *(uint8_t *)parm4;
        }
        if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
            mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW ||
            mMultiCameraMode == MODE_BOKEH) {
            mIsNeedFlashFired = 0;
        }
        // mIsNeedFlashFired is used by APP to check if af_trigger being sent
        // before capture
        HAL_LOGD("mIsNeedFlashFired = %d", mIsNeedFlashFired);
        break;
    case CAMERA_EVT_CB_HDR_SCENE:
        SPRD_DEF_Tag *sprdInfo;
        sprdInfo = mSetting->getSPRDDEFTagPTR();
        sprdInfo->sprd_is_hdr_scene = *(uint8_t *)parm4;
        HAL_LOGV("sprd_is_hdr_scene = %d", sprdInfo->sprd_is_hdr_scene);
        break;
    case CAMERA_EVT_CB_FDR_SCENE:
        SPRD_DEF_Tag *sprdFdrInfo;
        sprdFdrInfo = mSetting->getSPRDDEFTagPTR();
        sprdFdrInfo->sprd_is_fdr_scene = *(uint8_t *)parm4;
        sprdFdrInfo->sprd_is_hdr_scene = *(uint8_t *)parm4;
        HAL_LOGD("need to set fdr scene, now use hdr instead,  sprd_is_hdr_scene = %d",
                  sprdFdrInfo->sprd_is_hdr_scene);
        break;
    case CAMERA_EVT_CB_3DNR_SCENE:
        SPRD_DEF_Tag *sprd3dnrInfo;
        sprd3dnrInfo = mSetting->getSPRDDEFTagPTR();
        sprd3dnrInfo->sprd_is_3dnr_scene = *(uint8_t *)parm4;
        HAL_LOGV("sprd_is_3dnr_scene = %d", sprd3dnrInfo->sprd_is_3dnr_scene);
        break;
    case CAMERA_EVT_CB_EV_ADJUST_SCENE:
        SPRD_DEF_Tag *sprddreInfo;
        sprddreInfo = mSetting->getSPRDDEFTagPTR();
        sprddreInfo->sprd_is_lowev_scene = *(uint8_t *)parm4;
        HAL_LOGD(" sprd_is_lowev_scene = %d", sprddreInfo->sprd_is_lowev_scene);
        break;
    case CAMERA_EVT_CB_AI_SCENE:
        SPRD_DEF_Tag *sprdAIInfo;
        sprdAIInfo = mSetting->getSPRDDEFTagPTR();
        sprdAIInfo->sprd_ai_scene_type_current = *(cmr_u8 *)parm4;
        HAL_LOGD("sprdInfo->sprd_ai_scene_type_current :%u",
                 sprdAIInfo->sprd_ai_scene_type_current);
        break;

    case CAMERA_EVT_CB_VCM_RESULT: {
        int32_t vcm_result;
        mSetting->getVCMRETag(&vcm_result);
        vcm_result = *(int32_t *)parm4;
        mSetting->setVCMRETag(vcm_result);
        HAL_LOGD("CAMERA_EVT_CB_VCM_RESULT vcm_result %d", vcm_result);
    } break;

    case CAMERA_EVT_CB_HIST_REPORT: {
        int32_t hist_report[CAMERA_ISP_HIST_ITEMS] = {0};
        memcpy(hist_report, (int32_t *)parm4,
               sizeof(cmr_u32) * CAMERA_ISP_HIST_ITEMS);
        mSetting->setHISTOGRAMTag(hist_report);

        // control log print
        char prop[PROPERTY_VALUE_MAX];
        property_get("persist.vendor.cam.histogram.log.enable", prop, "0");
        if (atoi(prop)) {
            for (int i = 0; i < CAMERA_ISP_HIST_ITEMS; i++) {
                HAL_LOGI("CAMERA_EVT_CB_HIST_REPORT histogram %d",
                         hist_report[i]);
            }
        }
    } break;

    default:
        break;
    }

    HAL_LOGV("X");
    ATRACE_END();
}

void SprdCamera3OEMIf::HandleStartCamera(enum camera_cb_type cb, void *parm4) {
    HAL_LOGV("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));

    transitionState(SPRD_INIT, SPRD_IDLE, STATE_CAMERA);

    HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::HandleStopCamera(enum camera_cb_type cb, void *parm4) {
    HAL_LOGV("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));

    transitionState(SPRD_INTERNAL_STOPPING, SPRD_INIT, STATE_CAMERA);

    HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::HandleGetBufHandle(enum camera_cb_type cb, void *parm4) {
    int ret = 0;

    HAL_LOGV("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));
    SprdCamera3Stream *stream = NULL;
    // private_handle_t *buffer = NULL;
    native_handle_t *native_handle = NULL;
    cam_graphic_buffer_info_t *buf_info = (cam_graphic_buffer_info_t *)parm4;

    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;

    switch (cb) {
    case CAMERA_EVT_PREVIEW_BUF_HANDLE: {
        buffer_handle_t *buff_handle = NULL;
        SprdCamera3RegularChannel *channel =
            reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel != NULL)
            channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);
        ret = stream->getQBufHandle(buf_info->addr_vir, buf_info->addr_phy,
                                    buf_info->fd, &buff_handle,
                                    &(buf_info->graphic_buffer));

        if (buff_handle != NULL)
            native_handle = (native_handle_t *)(*buff_handle);

        break;
    }
    case CAMERA_EVT_VIDEO_BUF_HANDLE: {
        buffer_handle_t *buff_handle = NULL;
        SprdCamera3RegularChannel *channel =
            reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel != NULL)
            channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);
        ret = stream->getQBufHandle(buf_info->addr_vir, buf_info->addr_phy,
                                    buf_info->fd, &buff_handle,
                                    &(buf_info->graphic_buffer));
        if (buff_handle != NULL)
            native_handle = (native_handle_t *)(*buff_handle);

        break;
    }
    case CAMERA_EVT_CAPTURE_BUF_HANDLE: {
        buffer_handle_t *buff_handle = NULL;
        SprdCamera3PicChannel *channel =
            reinterpret_cast<SprdCamera3PicChannel *>(mPictureChan);
        if (channel != NULL)
            channel->getStream(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, &stream);
        ret = stream->getQBufHandle(buf_info->addr_vir, buf_info->addr_phy,
                                    buf_info->fd, &buff_handle,
                                    &(buf_info->graphic_buffer));
        if (buff_handle != NULL)
            native_handle = (native_handle_t *)(*buff_handle);

        break;
    }
    default:
        HAL_LOGD("[PFC] case not handled");
        break;
    }

    if (ret == NO_ERROR && native_handle != NULL) {
        buf_info->private_data = NULL;
    }
    HAL_LOGD("out, state = %s", getCameraStateStr(getCameraState()));
}

void SprdCamera3OEMIf::HandleReleaseBufHandle(enum camera_cb_type cb,
                                              void *parm4) {
    int ret = 0;
    cam_graphic_buffer_info_t *buf_info = (cam_graphic_buffer_info_t *)parm4;
    if (buf_info->private_data) {
        buf_info->private_data = NULL;
        buf_info->graphic_buffer = NULL;
    }
}

void SprdCamera3OEMIf::HandleCachedBuf(enum camera_cb_type cb, void *parm4) {
    int ret = 0;
    cam_ion_buffer_t *ionBuf = NULL;
    HAL_LOGV("in: cb = %d, parm4 = %p, state = %s", cb, parm4,
             getCameraStateStr(getCameraState()));
    if (!parm4) {
        HAL_LOGE("error: null input. cb %d\n", cb);
        return;
    }
    ionBuf = (cam_ion_buffer_t *)parm4;

    switch (cb) {
    case CAMERA_EVT_CB_INVALIDATE_BUF: {
        invalidateCache(ionBuf->fd,
                ionBuf->addr_vir, ionBuf->addr_phy, ionBuf->size);
        break;
    }
    case CAMERA_EVT_CB_FLUSH_BUF: {
        flushIonBuffer(ionBuf->fd,
                ionBuf->addr_vir, ionBuf->addr_phy, ionBuf->size);
        break;
    }
    default:
        HAL_LOGD("case not handled");
        break;
    }

    HAL_LOGV("out, state = %s", getCameraStateStr(getCameraState()));
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
    case CAMERA_FUNC_GET_BUF_HANDLE:
        obj->HandleGetBufHandle(cb, parm4);
        break;
    case CAMERA_FUNC_RELEASE_BUF_HANDLE:
        obj->HandleReleaseBufHandle(cb, parm4);
        break;
    case CAMERA_FUNC_BUFCACHE:
        obj->HandleCachedBuf(cb, parm4);
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

    HAL_LOGV("E");

    int ret = 0;
    ret = MemIon::Sync_ion_buffer(buffer_fd);
    if (ret) {
        HAL_LOGE("Sync_ion_buffer failed, ret=%d", ret);
        goto exit;
    }

    HAL_LOGV("X");

exit:
    return ret;
}

int SprdCamera3OEMIf::invalidateCache(int buffer_fd, void *v_addr, void *p_addr,
                                      size_t size) {
    ATRACE_CALL();

    HAL_LOGV("E");

    int ret = 0;
    ret = MemIon::Invalid_ion_buffer(buffer_fd);
    if (ret) {
        HAL_LOGE("Invalid_ion_buffer failed, ret=%d", ret);
        goto exit;
    }

    HAL_LOGV("X");

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
    struct exif_info exif_info = {0, 0, {0, 0}};
    LENS_Tag lensInfo;
    SPRD_DEF_Tag *sprddefInfo;
    struct sensor_mode_info mode_info[SENSOR_MODE_MAX];

    HAL_LOGI(":hal3: E camId=%d", mCameraId);

    if (NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    if (mCameraId >= CAMERA_ID_COUNT) {
        // for coverity, normally can't run to here
        HAL_LOGW("camera id error,set to 0, mCameraId=%d", mCameraId);
        mCameraId = 0;
    }
    mIsMlogMode = 0; //default disable
    property_get("persist.vendor.cam.mlog.mode.enable", value, "false");
    if (!strcmp(value, "true")) {
        mIsMlogMode = 1;
        mSetting->clearMLOGTag();
        /* clear global variable for mlog */
        memset(&mlog_info[mCameraId], 0x00, sizeof(struct mlog_infotag));
        getCalibrationInfo(&mlog_info[mCameraId]);

        HAL_LOGI("mlog enable");
    }

    GET_START_TIME;
    memset(mode_info, 0, sizeof(struct sensor_mode_info) * SENSOR_MODE_MAX);

    log_monitor_thread_init((void *)this);
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
                                    &mCameraHandle, (void *)Callback_IonMalloc,
                                    (void *)Callback_Free);
    if (ret) {
        setCameraState(SPRD_INIT);
        HAL_LOGE("camera_init failed");
        goto exit;
    }
    if (!isCameraInit()) {
#if defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_4) ||                      \
    defined(CONFIG_CAMERA_3DNR_CAPTURE_SW) ||                                  \
    defined(CONFIG_CAMERA_SUPPORT_ULTRA_WIDE)
        mHalOem->ops->camera_set_gpu_mem_ops(mCameraHandle,
                                             (void *)Callback_GpuMalloc, NULL);
#endif
    }
    setCameraState(SPRD_IDLE);

    if (mIsMlogMode) {
        struct phySensorInfo *phyPtr = NULL;
        MLOG_Tag *mlogInfo;

        mSetting->getMLOGTag(&mlogInfo);
        phyPtr = sensorGetPhysicalSnsInfo(mCameraId);
        if (mCameraId == 0 || mCameraId == 1 || mCameraId == 2)
            mlogInfo->face_type = phyPtr->face_type;
    }

    mHalOem->ops->camera_ioctrl(
        mCameraHandle, CAMERA_IOCTRL_GET_GRAB_CAPABILITY, &grab_capability);

    // TBD: these should be move to sprdcamera3settings.cpp
    // LensInfo
    mHalOem->ops->camera_get_sensor_exif_info(mCameraHandle, &exif_info);
    mSetting->getLENSTag(&lensInfo);
    lensInfo.aperture = exif_info.aperture;
    mSetting->setLENSTag(lensInfo);
    HAL_LOGV("lensInfo.aperture %f", lensInfo.aperture);

    if (MODE_3D_CALIBRATION == mMultiCameraMode) {
        mSprdRefocusEnabled = true;
        HAL_LOGD("mSprdRefocusEnabled %d", mSprdRefocusEnabled);
    }

    // dual otp
    HAL_LOGD("camera_id %d, mMultiCameraMode %d", mCameraId, mMultiCameraMode);
    if (((mMultiCameraMode == MODE_BOKEH ||
          mMultiCameraMode == MODE_3D_CALIBRATION ||
          mMultiCameraMode == MODE_DUAL_FACEID_UNLOCK) &&
         mCameraId < 2) ||
        (mCameraId == sensorGetRole(MODULE_SPW_NONE_BACK)) ||
        ((mMultiCameraMode == MODE_MULTI_CAMERA ||
          mMultiCameraMode == MODE_OPTICSZOOM_CALIBRATION) &&
        (mCameraId == sensorGetRole(MODULE_OPTICSZOOM_WIDE_BACK) ||
          mCameraId == sensorGetRole(MODULE_OPTICSZOOM_TELE_BACK))) ||
        (mMultiCameraMode == MODE_3D_FACEID_REGISTER ||
         mMultiCameraMode == MODE_3D_FACEID_UNLOCK) ||
         mMultiCameraMode == MODE_3D_FACE) {
        cmr_u8 dual_flag = 0;
        if ((mMultiCameraMode == MODE_BOKEH ||
             mMultiCameraMode == MODE_3D_CALIBRATION ||
             mMultiCameraMode == MODE_DUAL_FACEID_UNLOCK) &&
            mCameraId < 2)
            dual_flag = 1;
        else if (mCameraId == sensorGetRole(MODULE_SPW_NONE_BACK))
            dual_flag = 3;
        else if ((mMultiCameraMode == MODE_MULTI_CAMERA ||
                  mMultiCameraMode == MODE_OPTICSZOOM_CALIBRATION) &&
                 mCameraId == sensorGetRole(MODULE_OPTICSZOOM_WIDE_BACK))
            dual_flag = 5;
        else if ((mMultiCameraMode == MODE_MULTI_CAMERA ||
                  mMultiCameraMode == MODE_OPTICSZOOM_CALIBRATION) &&
                 mCameraId == sensorGetRole(MODULE_OPTICSZOOM_TELE_BACK))
            dual_flag = 6;
        else if (mMultiCameraMode == MODE_3D_FACEID_REGISTER ||
                 mMultiCameraMode == MODE_3D_FACEID_UNLOCK ||
                 mMultiCameraMode == MODE_3D_FACE)
            dual_flag = 4;
        else
            dual_flag = 0;
        OTP_Tag otpInfo;
        memset(&otpInfo, 0, sizeof(OTP_Tag));
        mSetting->getOTPTag(&otpInfo);

        struct sensor_otp_cust_info otp_info;
        memset(&otp_info, 0, sizeof(struct sensor_otp_cust_info));
        mHalOem->ops->camera_get_sensor_otp_info(mCameraHandle, dual_flag,
                                                 &otp_info);
        if (otp_info.total_otp.data_ptr != NULL &&
            otp_info.dual_otp.dual_flag) {
            memcpy(otpInfo.otp_data, (char *)otp_info.dual_otp.data_3d.data_ptr,
                   otp_info.dual_otp.data_3d.size);
            otpInfo.otp_type = otp_info.dual_otp.data_3d.dualcam_cali_lib_type;
            otpInfo.dual_otp_flag = otp_info.dual_otp.dual_flag;

            HAL_LOGD("camera_id %d, total_otp raw buffer %p, total_otp size "
                     "%d, dual_flag %d, dual_otp size %d",
                     mCameraId, otp_info.total_otp.data_ptr,
                     otp_info.total_otp.size, otpInfo.dual_otp_flag,
                     otp_info.dual_otp.data_3d.size);
        } else {
            otpInfo.dual_otp_flag = 0;
            HAL_LOGD("camera_id %d, get no dual_otp data from socket or eeprom",
                     mCameraId);
        }

        {
            bzero(file_name, sizeof(file_name));
            strcpy(file_name, CAMERA_DUMP_PATH);
            if (dual_flag == 1)
                strcat(file_name, "otp_manual_bokeh.txt");
            else if (dual_flag == 3)
                strcat(file_name, "otp_manual_spw.txt");
            else if (dual_flag == 5)
                strcat(file_name, "otp_manual_oz1.txt");
            else if (dual_flag == 6)
                strcat(file_name, "otp_manual_oz2.txt");
            else if (dual_flag == 4)
                strcat(file_name, "otp_manual_slt3d.txt");

            FILE *fid = fopen(file_name, "rb");
            if (NULL == fid) {
                HAL_LOGD("dual_flag %d, manual calibration otp txt not exist",
                         dual_flag);
            } else {
                int read_byte = 0;
                cmr_u8 *otp_data = (cmr_u8 *)otpInfo.otp_data;
                while (!feof(fid)) {
                    /* coverity:check_return: Calling "fscanf(fid, "%d\n",
                     * otp_data)"
                     * without checking return value. This library function
                     * may fail and return an error code.
                     */
                    if (fscanf(fid, "%d\n", otp_data) != EOF) {
                        otp_data += 4;
                        read_byte += 4;
                    }
                }
                fclose(fid);
                HAL_LOGD(
                    "dual_flag %d, manual calibration otp txt read_bytes = %d",
                    dual_flag, read_byte);
                if (read_byte) {
                    otp_info.dual_otp.data_3d.size = read_byte;
                    otpInfo.otp_type = 0; // OTP_CALI_SPRD;
                    otpInfo.dual_otp_flag = dual_flag;
                }
            }
        }

        HAL_LOGI("dual_flag %d, dual_otp size %d", otpInfo.dual_otp_flag,
                 otp_info.dual_otp.data_3d.size);
        if (otp_info.dual_otp.data_3d.size > 0) {
            if (dual_flag == 1)
                save_file("otp_dump_bokeh.bin", otpInfo.otp_data,
                          otp_info.dual_otp.data_3d.size);
            else if (dual_flag == 3)
                save_file("otp_dump_spw.bin", otpInfo.otp_data,
                          otp_info.dual_otp.data_3d.size);
            else if (dual_flag == 5)
                save_file("otp_dump_oz1.bin", otpInfo.otp_data,
                          otp_info.dual_otp.data_3d.size);
            else if (dual_flag == 6)
                save_file("otp_dump_oz2.bin", otpInfo.otp_data,
                          otp_info.dual_otp.data_3d.size);
            else if (dual_flag == 4)
                save_file("otp_dump_slt3d.bin", otpInfo.otp_data,
                          otp_info.dual_otp.data_3d.size);

            mSetting->setOTPTag(&otpInfo, otp_info.dual_otp.data_3d.size,
                                otpInfo.otp_type);
        }
        if (mIsMlogMode) {
            MLOG_Tag *mlogInfo;

            mSetting->getMLOGTag(&mlogInfo);
            mlogInfo->otp_size = otp_info.dual_otp.data_3d.size;
        }
    }

    // Add for 3d calibration get max sensor size begin
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    mHalOem->ops->camera_get_sensor_info_for_raw(mCameraHandle, mode_info);
    for (i = SENSOR_MODE_PREVIEW_ONE; i < SENSOR_MODE_MAX; i++) {
        HAL_LOGV("trim w=%d, h=%d", mode_info[i].trim_width,
                 mode_info[i].trim_height);
        if (mode_info[i].trim_width * mode_info[i].trim_height >=
            sprddefInfo->sprd_3dcalibration_cap_size[0] *
                sprddefInfo->sprd_3dcalibration_cap_size[1]) {
            sprddefInfo->sprd_3dcalibration_cap_size[0] =
                mode_info[i].trim_width;
            sprddefInfo->sprd_3dcalibration_cap_size[1] =
                mode_info[i].trim_height;
        }
    }
    HAL_LOGI("sprd_3dcalibration_cap_size w=%d, h=%d",
             sprddefInfo->sprd_3dcalibration_cap_size[0],
             sprddefInfo->sprd_3dcalibration_cap_size[1]);

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

    setCamSecurity(mMultiCameraMode);
    ZSLMode_monitor_thread_init((void *)this);

#ifdef CONFIG_CAMERA_GYRO
    gyro_monitor_thread_init((void *)this);
#endif

    property_get("persist.vendor.cam.raw.mode", value, "jpeg");
    if (!strcmp(value, "raw")) {
        mIsRawCapture = 1;
        HAL_LOGI("mIsRawCapture=%d", mIsRawCapture);
    }

    property_get("persist.vendor.cam.isptool.mode.enable", value, "false");
    if (!strcmp(value, "true")) {
        mIsIspToolMode = 1;
        HAL_LOGI("mIsIspToolMode=%d", mIsIspToolMode);
    }

#if defined(CONFIG_CAMERA_FACE_DETECT)
    faceDectect_enable(1);
#endif

exit:
    HAL_LOGI(":hal3: X");
    return ret;
}

/* collect info for camera_id 0/1/2  */
int SprdCamera3OEMIf::gatherInfoForMlog()
{
    cmr_s32 rtn = 0;
    struct mlog_infotag mlog_ae_info;
    MLOG_Tag *mlogInfo;

    memset(&mlog_ae_info, 0, sizeof(struct mlog_infotag));

    mSetting->getMLOGTag(&mlogInfo);
    if (mCameraId >= 0 && mCameraId < 3) {
        rtn = getAeInfo(&mlog_ae_info);
        if (!rtn) {
            mlog_info[mCameraId].cur_index = mlog_ae_info.cur_index;
            mlog_info[mCameraId].cur_lum = mlog_ae_info.cur_lum;
            mlog_info[mCameraId].target_lum = mlog_ae_info.target_lum;
            mlog_info[mCameraId].cur_bv = mlog_ae_info.cur_bv;
            mlog_info[mCameraId].cur_bv_nonmatch = mlog_ae_info.cur_bv_nonmatch;
            mlog_info[mCameraId].cur_exp_line = mlog_ae_info.cur_exp_line;
            mlog_info[mCameraId].total_exp_time = mlog_ae_info.total_exp_time;
            mlog_info[mCameraId].cur_again = mlog_ae_info.cur_again;
            mlog_info[mCameraId].cur_dummy = mlog_ae_info.cur_dummy;

        }
        mlog_info[mCameraId].otp_size = mlogInfo->otp_size;
        mlog_info[mCameraId].prev_timestamp = mlogInfo->prev_timestamp;
        mlog_info[mCameraId].cap_timestamp = mlogInfo->cap_timestamp;
        mlog_info[mCameraId].fps = mlogInfo->fps;
        mlog_info[mCameraId].cropping_type = mlogInfo->cropping_type;
        mlog_info[mCameraId].vcm_dac = mlogInfo->vcm_dac;
        mlog_info[mCameraId].vcm_num = mlogInfo->vcm_num;
        mlog_info[mCameraId].vcm_step = mlogInfo->vcm_step;
        mlog_info[mCameraId].face_type = mlogInfo->face_type;
        rtn = 0;
    }

exit:
    return rtn;
}

int SprdCamera3OEMIf::saveMlogInfo()
{
    int rtn = 0;
    int num = 0;
    char tmp_buf[1024];
    int prev_DT = 0;
    int cap_DT = 0;

    bzero(tmp_buf, sizeof(tmp_buf));
    if ((getMultiCameraMode() == MODE_BOKEH ||
         getMultiCameraMode() == MODE_3D_CALIBRATION)) {
        prev_DT = abs(mlog_info[0].prev_timestamp - mlog_info[2].prev_timestamp);
        cap_DT = abs(mlog_info[0].cap_timestamp - mlog_info[2].cap_timestamp);
    }

    rtn = sprintf(tmp_buf, "\nCam0:d_otp: %d cap_t: %lld fps: %d crop: %d "
                 "facing: %d idx: %d cur-l: %d tar-l: %d bv(lv): %d "
                 "cali_bv: %d expl: %d expt: %d gain: %d dmy: "
                 "%d\n\nCam1:d_otp: %d cap_t: %lld fps: %d crop: %d "
                 "facing: %d idx: %d cur-l: %d tar-l: %d bv(lv): %d "
                 "cali_bv: %d expl: %d expt: %d gain: %d dmy: "
                 "%d\n\nCam2:d_otp: %d cap_t: %lld fps: %d "
                 "crop: %d facing: %d idx: %d cur-l: %d tar-l: %d "
                 "bv(lv): %d cali_bv: %d expl: %d expt: %d gain: %d "
                 "dmy: %d\nprev_DT: %d cap_DT: %d "
                 "vcm_dac: %d vcm_num: %d vcm_step: %d v1/v2: %d\n\n",
        mlog_info[0].otp_size, mlog_info[0].cap_timestamp, mlog_info[0].fps,
        mlog_info[0].cropping_type, mlog_info[0].face_type, mlog_info[0].cur_index,
        mlog_info[0].cur_lum, mlog_info[0].target_lum, mlog_info[0].cur_bv,
        mlog_info[0].cur_bv_nonmatch, mlog_info[0].cur_exp_line,
        mlog_info[0].total_exp_time, mlog_info[0].cur_again,
        mlog_info[0].cur_dummy, mlog_info[1].otp_size, mlog_info[1].cap_timestamp,
        mlog_info[1].fps, mlog_info[1].cropping_type, mlog_info[1].face_type,
        mlog_info[1].cur_index, mlog_info[1].cur_lum, mlog_info[1].target_lum,
        mlog_info[1].cur_bv, mlog_info[1].cur_bv_nonmatch,
        mlog_info[1].cur_exp_line, mlog_info[1].total_exp_time,
        mlog_info[1].cur_again, mlog_info[1].cur_dummy, mlog_info[2].otp_size,
        mlog_info[2].cap_timestamp, mlog_info[2].fps, mlog_info[2].cropping_type,
        mlog_info[2].face_type, mlog_info[2].cur_index, mlog_info[2].cur_lum,
        mlog_info[2].target_lum, mlog_info[2].cur_bv, mlog_info[2].cur_bv_nonmatch,
        mlog_info[2].cur_exp_line, mlog_info[2].total_exp_time,
        mlog_info[2].cur_again, mlog_info[2].cur_dummy, prev_DT, cap_DT,
        mlog_info[0].vcm_dac, mlog_info[0].vcm_num, mlog_info[0].vcm_step,
        mlog_info[0].vcm_version);

    HAL_LOGD("cameraId %d", mCameraId);
    FILE *fid = fopen(MLOG_FOR_READ_PATH, "wb");
    if (fid == NULL) {
        CMR_LOGE("open file failed");
        return -1;
    }

    rtn = fwrite(tmp_buf, 1, strlen(tmp_buf), fid);
    fclose(fid);

    return 0;
}
/* function: get name and value from buf, set \0 to buf
 * name:start[a-zA-Z], end[space,\n,:]
 * value:[0-9.]
 */
static int GetNameAndValue(char *p, int &i, char **pn, char **pv)
{
    // get name start [a-zA-Z]
    while (p[i] != '\0') {
        if ((p[i] >= 'A' && p[i] <= 'Z') ||
            (p[i] >= 'a' && p[i] <= 'z')) {
            *pn = &p[i];
            break;
        }
        i++;
    }

    // to name end
    while (p[i] != '\0') {
        if (p[i] == ' ' || p[i] == ':' || p[i] == '\n') {
            p[i++] = '\0';
            break;
        }
        i++;
    }

    // to value start
    while (p[i] != '\0') {
        if (p[i] >= '0' && p[i] <= '9') {
            *pv = &p[i];
            break;
        }
        i++;
    }
    while (p[i] != '\0') {
        if (!((p[i] >= '0' && p[i] <= '9') || p[i] == '.')) {
           p[i++] = '\0';
           // got
           return 0;
        }
        i++;
    }

    return -1;
}

int SprdCamera3OEMIf::getAeInfo(struct mlog_infotag *pinfo)
{
    cmr_s32 rtn = 0;
    char tmp_buf[400];
    int i = 0;
    FILE *fp;
    int len;
    char *pname;
    char *pvalue;

    fp = fopen(MLOG_AE_PATH, "rb");
    if (fp == NULL) {
        CMR_LOGW("open mlog file failed");
        return -1;
    }

    rtn = fgetc(fp);
    if (rtn == -1) {
        CMR_LOGD("file is NULL");
        goto exit;
    }
    memset(tmp_buf, 0x00, sizeof(tmp_buf));
    len = fread(tmp_buf, 1, sizeof(tmp_buf) - 1, fp);
    if (len > 0)
        tmp_buf[len] = '\0';
    i = 0;
    while (len && (0 == GetNameAndValue(tmp_buf, i, &pname, &pvalue))) {
        if (strcmp(pname, "am-id") == 0)
            pinfo->camera_id = atoi(pvalue);
        else if (strcmp(pname, "frm-id") == 0)
            pinfo->cur_index = atoi(pvalue);
        else if (strcmp(pname, "cur-l") == 0)
            pinfo->cur_lum = atoi(pvalue);
        else if (strcmp(pname, "tar-l") == 0)
            pinfo->target_lum = atoi(pvalue);
        else if (strcmp(pname, "bv(lv)") == 0)
            pinfo->cur_bv = atoi(pvalue);
        else if (strcmp(pname, "cali_bv") == 0)
            pinfo->cur_bv_nonmatch = atoi(pvalue);
        else if (strncmp(pname, "expl", 4) == 0)
            pinfo->cur_exp_line = atoi(pvalue);
        else if (strcmp(pname, "expt") == 0)
            pinfo->total_exp_time = atoi(pvalue);
        else if (strcmp(pname, "gain") == 0)
            pinfo->cur_again = atoi(pvalue);
        else if (strcmp(pname, "dmy") == 0)
            pinfo->cur_dummy = atoi(pvalue);
    }
    rtn = 0;

exit:
    fclose(fp);

    return rtn;
}

int SprdCamera3OEMIf::getCalibrationInfo(struct mlog_infotag *mlog_info)
{
    cmr_s32 rtn = 0;
    char tmp_buf[32];
    int len;
    int i;
    char *pn, *pv;

    FILE *fp = fopen(MLOG_CALIBRIATION_PATH, "rb");
    if (fp == NULL) {
        CMR_LOGW("open mlog file failed");
        return -1;
    }
    len = fread(tmp_buf, 1, sizeof(tmp_buf) - 1, fp);
    if (len > 0)
        tmp_buf[len] = '\0';

    i = 0;
    while (len && GetNameAndValue(tmp_buf, i, &pn, &pv) == 0) {
        // fscanf(fp, "%s %d", &tmp_buf[0], &mlog_info->vcm_version);
        mlog_info->vcm_version = atoi(pv);
        break;
    }

exit:
    fclose(fp);

    return rtn;
}

int SprdCamera3OEMIf::setCamSecurity(multiCameraMode multiCamMode) {
    struct sprd_cam_sec_cfg securityCfg;
    int ret = NO_ERROR;

    HAL_LOGI("multi camera mode = %d", multiCamMode);

    memset(&securityCfg, 0, sizeof(sprd_cam_sec_cfg));

#ifdef CONFIG_CAMERA_SECURITY_TEE_FULL
    if (multiCamMode == MODE_SINGLE_FACEID_REGISTER ||
        multiCamMode == MODE_SINGLE_FACEID_UNLOCK) {
        securityCfg.camsec_mode = SEC_TIME_PRIORITY;
        securityCfg.work_mode = FACEID_SINGLE;
    } else if (multiCamMode == MODE_DUAL_FACEID_REGISTER ||
               multiCamMode == MODE_DUAL_FACEID_UNLOCK) {
        securityCfg.camsec_mode = SEC_TIME_PRIORITY;
        securityCfg.work_mode = FACEID_DUAL;
    } else { // close face lock menu
        securityCfg.camsec_mode = SEC_UNABLE;
        securityCfg.work_mode = FACEID_INVALID;
    }
#else
    securityCfg.camsec_mode = SEC_UNABLE;
    securityCfg.work_mode = FACEID_INVALID;
#endif

    ret = mHalOem->ops->camera_ioctrl(
        mCameraHandle, CAMERA_IOCTRL_SET_CAM_SECURITY, &securityCfg);

    return ret;
}

void SprdCamera3OEMIf::setCamPreformaceScene(
    sys_performance_camera_scene camera_scene) {

    if (mSysPerformace) {
        mSysPerformace->setCamPreformaceScene(camera_scene);
    }
}
void SprdCamera3OEMIf::setUltraWideMode() {
    SprdCamera3RegularChannel *channel =
        reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
    HAL_LOGD("mIsUltraWideMode:%d, channel:%p", mIsUltraWideMode, channel);
    if (channel != NULL) {
        SprdCamera3Stream *stream = NULL;
        channel->getStream(CAMERA_STREAM_TYPE_PREVIEW, &stream);
        if (stream != NULL) {
            stream->setUltraWideMode(mIsUltraWideMode);
        }
        SprdCamera3Stream *video_stream = NULL;
        channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &video_stream);
        if (video_stream != NULL) {
            video_stream->setUltraWideMode(mIsUltraWideMode);
        }
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
    uint8_t PhyCam = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mSetting->getSCALERTag(&scaleInfo);
    cropRegion.start_x = scaleInfo.crop_region[0];
    cropRegion.start_y = scaleInfo.crop_region[1];
    cropRegion.width = scaleInfo.crop_region[2];
    cropRegion.height = scaleInfo.crop_region[3];
    HAL_LOGD("camera %u, crop start_x=%d start_y=%d width=%d height=%d",
            mCameraId, cropRegion.start_x, cropRegion.start_y,
            cropRegion.width, cropRegion.height);
    if ((getMultiCameraMode() == MODE_BOKEH ||
         getMultiCameraMode() == MODE_3D_CALIBRATION) &&
        mCameraId == sensorGetRole(MODULE_SPW_NONE_BACK)) {
        mSetting->getLargestSensorSize(mCameraId, &sensorOrgW, &sensorOrgH);
        cal_spw_size(sensorOrgW, sensorOrgH, &(cropRegion.width),
                     &(cropRegion.height));
        cropRegion.start_x = (sensorOrgW - cropRegion.width) >> 1;
        cropRegion.start_y = (sensorOrgH - cropRegion.height) >> 1;
    }
    if (getMultiCameraMode() == MODE_MULTI_CAMERA) {
        mSetting->getLargestSensorSize(mCameraId, &sensorOrgW, &sensorOrgH);
    } else {
        mSetting->getLargestPictureSize(mCameraId, &sensorOrgW, &sensorOrgH);
    }

    if (mIsMlogMode) {
       MLOG_Tag *mlogInfo;
       mSetting->getMLOGTag(&mlogInfo);
        if (mCameraId == 0 || mCameraId == 1 || mCameraId == 2)
            mlogInfo->cropping_type = scaleInfo.cropping_type;
    }

    if (cropRegion.width > 0 && cropRegion.height > 0) {
        zoomRatio = static_cast<float>(sensorOrgW) / cropRegion.width;
        HAL_LOGV("mCameraId=%d, zoomRatio=%f", mCameraId, zoomRatio);
    }

    if (zoomRatio < MIN_DIGITAL_ZOOM_RATIO)
        zoomRatio = MIN_DIGITAL_ZOOM_RATIO;

    if (getMultiCameraMode() == MODE_MULTI_CAMERA) {
        if (zoomRatio > MULTI_MAX_DIGITAL_ZOOM_RATIO) {
            zoomRatio = MULTI_MAX_DIGITAL_ZOOM_RATIO;
        }
    } else if (zoomRatio > MAX_DIGITAL_ZOOM_RATIO) {
        zoomRatio = MAX_DIGITAL_ZOOM_RATIO;
    }

    if (mCameraId == sensorGetRole(MODULE_SPW_NONE_BACK) &&
        getMultiCameraMode() == MODE_MULTI_CAMERA &&
        zoomRatio > MAX_DIGITAL_ZOOM_RATIO) {
        zoomRatio = MAX_DIGITAL_ZOOM_RATIO;
    }

    struct sensor_zoom_param_input ZoomInputParam;
    ret = sensorGetZoomParam(&ZoomInputParam);
    PhyCam = ZoomInputParam.PhyCameras;

    if (mCameraId == sensorGetRole(MODULE_OPTICSZOOM_WIDE_BACK) &&
        getMultiCameraMode() == MODE_MULTI_CAMERA &&
        zoomRatio > MAX_DIGITAL_ZOOM_RATIO &&
        PhyCam == MULTI_THREE_SECTION) {
        zoomRatio = MAX_DIGITAL_ZOOM_RATIO;
    }

    mZoomInfo.mode = ZOOM_INFO;
    HAL_LOGD("mCameraId=%d, zoomRatio=%f, mIsUltraWideMode=%d", mCameraId,
             zoomRatio, mIsUltraWideMode);
    mZoomInfo.zoom_info.zoom_ratio = zoomRatio;
    mZoomInfo.zoom_info.prev_aspect_ratio = zoomRatio;
    if (mIsFovFusionMode == true && sensorGetRole(MODULE_OPTICSZOOM_WIDE_BACK)) {
       mZoomInfo.zoom_info.capture_aspect_ratio = 1.0;
    } else {
       mZoomInfo.zoom_info.capture_aspect_ratio = zoomRatio;
    }
    HAL_LOGD("mIsFovFusionMode %d, capture_aspect_ratio=%f", mIsFovFusionMode, mZoomInfo.zoom_info.capture_aspect_ratio);
    mZoomInfo.zoom_info.video_aspect_ratio = zoomRatio;
    mZoomInfo.zoom_info.pixel_size.width = sensorOrgW;
    mZoomInfo.zoom_info.pixel_size.height = sensorOrgH;
    mZoomInfo.zoom_info.crop_region = cropRegion;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ZOOM, (cmr_uint)&mZoomInfo);

    return ret;
}

bool SprdCamera3OEMIf::cal_spw_size(int sw_width, int sw_height,
                                    cmr_u32 *out_width, cmr_u32 *out_height) {
    float sw_fov = 0, w_fov = 0;
    mSetting->getSensorFov(&w_fov, &sw_fov);
    if ((sw_fov - w_fov) < 6.0) {
        HAL_LOGD(
            "the input fov is wrong, please check! wide_fov= %f, sw_fov= %f",
            w_fov, sw_fov);
        return false;
    }
    float fov_scale = (w_fov + 6.0) / sw_fov;

    // the out_height/out_width is 3/4, and are a multiple of 16
    int scale = sw_height * fov_scale / 48 + 1;
    *out_height = 48 * scale;
    *out_width = 64 * scale;

    return true;
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

    HAL_LOGD("mCameraId %d crop %d %d %d %d sens w/h %d %d.", mCameraId,
             cropRegion->start_x, cropRegion->start_y, cropRegion->width,
             cropRegion->height, sensorWidth, sensorHeight);

    if (getMultiCameraMode() == MODE_MULTI_CAMERA) {
        mSetting->getLargestSensorSize(mCameraId, &sensorOrgW, &sensorOrgH);
    } else {
        mSetting->getLargestPictureSize(mCameraId, &sensorOrgW, &sensorOrgH);
    }


    SensorRotate = mHalOem->ops->camera_get_preview_rot_angle(mCameraHandle);

    if (sensorWidth == sensorOrgW && sensorHeight == sensorOrgH &&
        SensorRotate == IMG_ANGLE_0) {
        HAL_LOGD("mCameraId %d dont' need to convert.", mCameraId);
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
    if (IsRotate) {
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
    HAL_LOGD("mCameraId = %d, Crop calculated (x=%d,y=%d,w=%d,h=%d rot=0x%lx)",
             mCameraId, cropRegion->start_x, cropRegion->start_y,
             cropRegion->width, cropRegion->height, SensorRotate);
    return ret;
}

int SprdCamera3OEMIf::setJpegOrientation(int jpegOrientation) {
    int sensorOrientation;

    SprdCamera3Setting::s_setting[mCameraId].jpgInfo.orientation =
        jpegOrientation;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ENCODE_ROTATION,
             jpegOrientation);
    HAL_LOGD("jpegOrientation = %d", jpegOrientation);

    sensorOrientation = SprdCamera3Setting::s_setting[mCameraId].sensorInfo.orientation;
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
             sensorOrientation);
    HAL_LOGD("sensorOrientation = %d", sensorOrientation);
    return NO_ERROR;
}

int SprdCamera3OEMIf::SetCameraParaTag(cmr_int cameraParaTag) {
    int ret = 0;
    CONTROL_Tag controlInfo;
    char value[PROPERTY_VALUE_MAX];

    HAL_LOGV("set camera para, tag is %ld", cameraParaTag);

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    mSetting->getCONTROLTag(&controlInfo);
    switch (cameraParaTag) {
    case ANDROID_CONTROL_SCENE_MODE: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        if (1 == sprddefInfo->sprd_3dnr_enabled &&
            controlInfo.scene_mode != ANDROID_CONTROL_SCENE_MODE_HDR) {
            controlInfo.scene_mode = ANDROID_CONTROL_SCENE_MODE_NIGHT;
        }
        if (sprddefInfo->sprd_auto_3dnr_enable == CAMERA_3DNR_AUTO) {
            controlInfo.scene_mode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
        }

        int8_t drvSceneMode = 0;
        mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode,
                                            &drvSceneMode);
        HAL_LOGD("drvSceneMode: %d, mMultiCameraMode: %d", drvSceneMode, mMultiCameraMode);
        if (sprddefInfo->sprd_appmode_id == CAMERA_MODE_PANORAMA) {
            drvSceneMode = CAMERA_SCENE_MODE_PANORAMA;
        }
	  if (sprddefInfo->sprd_appmode_id == CAMERA_MODE_SLOWMOTION) {
            drvSceneMode = CAMERA_SCENE_MODE_SLOWMOTION;
        }
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
        if (controlInfo.awb_mode != mLastAwbMode) {
            setAwbState(AWB_MODE_CHANGE);
            mLastAwbMode = controlInfo.awb_mode;
        }
    } break;

    case ANDROID_CONTROL_AWB_LOCK: {
        uint8_t awb_lock;
        awb_lock = controlInfo.awb_lock;
        HAL_LOGV("ANDROID_CONTROL_AWB_LOCK awb_lock=%d", awb_lock);
        if (awb_lock && controlInfo.awb_mode == ANDROID_CONTROL_AWB_MODE_AUTO) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK,
                     awb_lock);
            setAwbState(AWB_LOCK_ON);
        } else if (!awb_lock &&
                   controlInfo.awb_state == ANDROID_CONTROL_AWB_STATE_LOCKED) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AWB_LOCK_UNLOCK,
                     awb_lock);
            setAwbState(AWB_LOCK_OFF);
        }
    } break;

    case ANDROID_SCALER_CROP_REGION:
        // SET_PARM(mHalOem, CAMERA_PARM_ZOOM_RECT, cameraParaValue);
        break;

    case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
        SPRD_DEF_Tag *sprddefInfo;
        struct cmr_ae_compensation_param ae_compensation_param;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();

        if (sprddefInfo->sprd_appmode_id == CAMERA_MODE_MANUAL) {
            // just for legacy sprd isp ae compensation manual mode
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
            mManualExposureEnabled = true;
        }

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXPOSURE_COMPENSATION,
                 (cmr_uint)&ae_compensation_param);
        break;
    case ANDROID_CONTROL_AF_TRIGGER: {
        HAL_LOGD("mCameraId=%d, AF_TRIGGER %d", mCameraId,
                 controlInfo.af_trigger);
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        struct auto_tracking_info info;
        cmr_u32 af_status = 0;
        if (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START) {
            struct img_rect zoom1 = {0, 0, 0, 0};
            struct img_rect zoom = {0, 0, 0, 0};
            cmr_u16 picW, picH, snsW, snsH;
            float w_ratio = 0.000f, h_ratio = 0.000f;
            struct cmr_focus_param focus_para;
            if (mCameraState.preview_state == SPRD_PREVIEW_IN_PROGRESS) {
                zoom.start_x = controlInfo.af_regions[0];
                zoom.start_y = controlInfo.af_regions[1];
                zoom.width = controlInfo.af_regions[2];
                zoom.height = controlInfo.af_regions[3];
#ifndef CONFIG_ISP_2_3
                mHalOem->ops->camera_get_sensor_trim(mCameraHandle, &zoom1);
#else
                // for sharkle only
                mHalOem->ops->camera_get_sensor_trim2(mCameraHandle, &zoom1);
#endif
                mSetting->getLargestSensorSize(mCameraId, &snsW, &snsH);
                mSetting->getLargestPictureSize(mCameraId, &picW, &picH);
                w_ratio = (float)snsW / (float)picW;
                h_ratio = (float)snsH / (float)picH;
                HAL_LOGV("w_ratio = %f, h_ratio = %f", w_ratio, h_ratio);
                if (sprddefInfo->sprd_ot_switch == 1) {
                   SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_STATUS_NOTIFY_TRACKING, af_status);
                   info.objectX = w_ratio * (controlInfo.af_regions[0] + controlInfo.af_regions[2]/2);
                   info.objectY = h_ratio * (controlInfo.af_regions[1] + controlInfo.af_regions[3]/2);
                   info.frame_id = controlInfo.ot_frame_id;
                   if (info.objectX != 0 && info.objectY != 0)
                       info.status = 1;
                   else
                       info.status = 0;
                   HAL_LOGV("AF_TRIGGER =%d, %d, %d", info.objectX, info.objectY, info.status);
                   HAL_LOGV("ot_frame_id=%d",info.frame_id);
                   SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AUTO_TRACKING_INFO,
                           (cmr_uint)&info);
                }
                if ((0 == zoom.start_x && 0 == zoom.start_y &&
                     0 == zoom.width && 0 == zoom.height) ||
                    !CameraConvertCropRegion(zoom1.width, zoom1.height,
                                             &zoom)) {
                    focus_para.zone[0].start_x = zoom.start_x;
                    focus_para.zone[0].start_y = zoom.start_y;
                    focus_para.zone[0].width = zoom.width;
                    focus_para.zone[0].height = zoom.height;

                    if (0 == zoom.start_x && 0 == zoom.start_y &&
                        0 == zoom.width && 0 == zoom.height) {
                        focus_para.zone_cnt = 0;
                    } else {
                        focus_para.zone_cnt = 1;
                    }

                    HAL_LOGD("after crop AF region mCameraId = %d"
                             ", x, y, w, h, %d %d %d %d, zone_cnt %d",
                             mCameraId,
                             focus_para.zone[0].start_x,
                             focus_para.zone[0].start_y,
                             focus_para.zone[0].width,
                             focus_para.zone[0].height,
                             focus_para.zone_cnt);
                    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FOCUS_RECT,
                             (cmr_uint)&focus_para);
                }
            }
            autoFocus();
        } else if (controlInfo.af_trigger ==
                   ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
            cancelAutoFocus();
        }
    }
        break;
    case ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD(" sprddefInfo->availabe_gender_race_age_enable: %d",
                 sprddefInfo->gender_race_age_enable);

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FACE_ATTRIBUTES_ENABLE,
                 (uint32_t)sprddefInfo->gender_race_age_enable);
    } break;

    case ANDROID_SPRD_TOUCH_INFO: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        struct img_rect touch_areas = {0, 0, 0, 0};
        struct fd_touch_info info;
        int32_t touchRegion[4];

        touch_areas.start_x = sprddefInfo->am_regions[0];
        touch_areas.start_y = sprddefInfo->am_regions[1];
        touch_areas.width =
            sprddefInfo->am_regions[2] - sprddefInfo->am_regions[0];
        touch_areas.height =
            sprddefInfo->am_regions[3] - sprddefInfo->am_regions[1];

        touchRegion[0] = touch_areas.start_x;
        touchRegion[1] = touch_areas.start_y;
        touchRegion[2] = touch_areas.width;
        touchRegion[3] = touch_areas.height;
        CameraConvertRegionFromFramework(touchRegion);

        info.fd_touchX = touchRegion[0] + touchRegion[2] / 2;
        info.fd_touchY = touchRegion[1] + touchRegion[3] / 2;
        HAL_LOGD("fd_touchX %d fd_touchY %d", info.fd_touchX, info.fd_touchY);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_TOUCH_INFO_TO_FD,
                 (cmr_uint)&info);
    } break;
    case ANDROID_SPRD_SENSOR_ORIENTATION: {
        SPRD_DEF_Tag *sprddefInfo;

        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("orient=%d", sprddefInfo->sensor_orientation);
        if (1 == sprddefInfo->sensor_orientation) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
                     1);
        } else {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ORIENTATION,
                     0);
        }
    } break;
    case ANDROID_SPRD_SENSOR_ROTATION: {
        SPRD_DEF_Tag *sprddefInfo;

        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("rot=%d", sprddefInfo->sensor_rotation);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION,
                 sprddefInfo->sensor_rotation);
    } break;
    case ANDROID_SPRD_UCAM_SKIN_LEVEL: {
        SPRD_DEF_Tag *sprddefInfo;
        struct beauty_info fb_param;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        fb_param.blemishLevel = sprddefInfo->perfect_skin_level[0];
        fb_param.smoothLevel = sprddefInfo->perfect_skin_level[1];
        fb_param.skinColor = sprddefInfo->perfect_skin_level[2];
        fb_param.skinLevel = sprddefInfo->perfect_skin_level[3];
        fb_param.brightLevel = sprddefInfo->perfect_skin_level[4];
        fb_param.lipColor = sprddefInfo->perfect_skin_level[5];
        fb_param.lipLevel = sprddefInfo->perfect_skin_level[6];
        fb_param.slimLevel = sprddefInfo->perfect_skin_level[7];
        fb_param.largeLevel = sprddefInfo->perfect_skin_level[8];
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_PERFECT_SKIN_LEVEL,
                 (cmr_uint) & (fb_param));
        mFbOn = fb_param.blemishLevel || fb_param.smoothLevel ||
                fb_param.skinColor || fb_param.skinLevel ||
                fb_param.brightLevel || fb_param.lipColor ||
                fb_param.lipLevel || fb_param.slimLevel || fb_param.largeLevel;
    } break;
    case ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR: {
        SPRD_DEF_Tag *sprddefInfo;

        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("flip_on_level = %d", sprddefInfo->flip_on);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLIP_ON,
                 sprddefInfo->flip_on);
    } break;

    case ANDROID_SPRD_EIS_ENABLED: {
        SPRD_DEF_Tag *sprddefInfo;

        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("sprd_eis_enabled = %d", sprddefInfo->sprd_eis_enabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_EIS_ENABLED,
                 sprddefInfo->sprd_eis_enabled);
    } break;

    case ANDROID_CONTROL_AF_MODE: {
        int8_t AfMode = 0;
        cmr_u32 af_status = 0;
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        mSetting->androidAfModeToDrvAfMode(controlInfo.af_mode, &AfMode);
        HAL_LOGD("ANDROID_CONTROL_AF_MODE");

        if (mMultiCameraMode == MODE_3D_CALIBRATION &&
            AfMode == CAMERA_FOCUS_MODE_MANUAL) {
            AfMode = CAMERA_FOCUS_MODE_INFINITY;
        }
        if ((mMultiCameraMode == MODE_BOKEH ||
             mMultiCameraMode == MODE_3D_CALIBRATION) &&
            mCameraId == 2) {
            AfMode = CAMERA_FOCUS_MODE_INFINITY;
        }
        if (!mIsAutoFocus) {
            if (sprddefInfo->sprd_ot_switch == 1 && AfMode == CAMERA_FOCUS_MODE_AUTO) {
                SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_STATUS_NOTIFY_TRACKING, af_status);
                HAL_LOGV("clean_af_status = %d", af_status);
            }
            if (mRecordingMode &&
                CAMERA_FOCUS_MODE_CAF ==
                    AfMode) { /*dv mode but recording not start*/
                AfMode = CAMERA_FOCUS_MODE_CAF_VIDEO;
            }
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AF_MODE, AfMode);
        }
        if (controlInfo.af_mode != mLastAfMode) {
            setAfState(AF_MODE_CHANGE);
            mLastAfMode = controlInfo.af_mode;
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
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_BRIGHTNESS,
                 (uint32_t)sprddefInfo->brightness);
    } break;

    case ANDROID_SPRD_AI_SCENE_ENABLED: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD(" sprddefInfo->ai_scene_enabled: %d",
                 sprddefInfo->ai_scene_enabled);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AI_SCENE_ENABLED,
                 (uint32_t)sprddefInfo->ai_scene_enabled);
    } break;

    case ANDROID_SPRD_CONTRAST: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_CONTRAST,
                 (uint32_t)sprddefInfo->contrast);
    } break;

    case ANDROID_SPRD_SATURATION: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SATURATION,
                 (uint32_t)sprddefInfo->saturation);
    } break;

    case ANDROID_CONTROL_AE_MODE:
        if (getMultiCameraMode() == MODE_MULTI_CAMERA || mCameraId == 0 ||
            mCameraId == 1 || mCameraId == 4 || mCameraId == 3 ||
            (mCameraId == sensorGetRole(MODULE_SPW_NONE_BACK) &&
             getMultiCameraMode() != MODE_BOKEH &&
             getMultiCameraMode() != MODE_3D_CALIBRATION &&
             getMultiCameraMode() != MODE_PORTRAIT)) {
            int8_t drvAeMode;
            mSetting->androidAeModeToDrvAeMode(controlInfo.ae_mode, &drvAeMode);

            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AE_MODE,
                     controlInfo.ae_mode);

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
                    }
                }

                if (controlInfo.ae_mode != mLastAeMode) {
                    setAeState(AE_MODE_CHANGE);
                    mLastAeMode = controlInfo.ae_mode;
                }
            }
        }
        break;

    case ANDROID_SENSOR_EXPOSURE_TIME:
        if (controlInfo.ae_mode == ANDROID_CONTROL_AE_MODE_OFF) {
            SENSOR_Tag sensorInfo;
            mSetting->getSENSORTag(&sensorInfo);
            HAL_LOGD("exposure_time %lld", sensorInfo.exposure_time);
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXPOSURE_TIME,
                     (cmr_uint)(sensorInfo.exposure_time));
        }
        break;

    case ANDROID_SENSOR_SENSITIVITY:
        int8_t drv_iso_level;
        SENSOR_Tag sensorInfo;
        mSetting->getSENSORTag(&sensorInfo);
        mSetting->androidIsoToDrvMode(sensorInfo.sensitivity,
                                            &drv_iso_level);
        HAL_LOGD("drv_iso_level %d", drv_iso_level);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISO,
                 drv_iso_level);
        break;

    case ANDROID_CONTROL_AE_LOCK: {
        uint8_t ae_lock;
        ae_lock = controlInfo.ae_lock;
        HAL_LOGD("ANDROID_CONTROL_AE_LOCK, ae_lock = %d", ae_lock);
        if (ae_lock && controlInfo.ae_mode != ANDROID_CONTROL_AE_MODE_OFF) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AE_LOCK_UNLOCK,
                     ae_lock);
            setAeState(AE_LOCK_ON);
        } else if (!ae_lock &&
                   controlInfo.ae_state == ANDROID_CONTROL_AE_STATE_LOCKED) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISP_AE_LOCK_UNLOCK,
                     ae_lock);
            setAeState(AE_LOCK_OFF);
        }
    } break;

    case ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER: {
        HAL_LOGV("mCameraId=%d, ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER: %d",
                 mCameraId, controlInfo.ae_precap_trigger);
        if (controlInfo.ae_precap_trigger ==
            ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
            setAeState(AE_PRECAPTURE_START);
        } else if (controlInfo.ae_precap_trigger ==
                   ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL) {
            setAeState(AE_PRECAPTURE_CANCEL);
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

        HAL_LOGI("after crop ae region mCameraId %d"
                 ", x, y, w, h, %d %d %d %d",
                 mCameraId,
                 ae_param.win_area.rect[0].start_x,
                 ae_param.win_area.rect[0].start_y,
                 ae_param.win_area.rect[0].width,
                 ae_param.win_area.rect[0].height);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AE_REGION,
                 (cmr_uint)&ae_param);

    } break;

    case ANDROID_FLASH_MODE:
        if (getMultiCameraMode() == MODE_MULTI_CAMERA || mCameraId == 0 ||
            mCameraId == 1 || mCameraId == 4 || mCameraId == 3) {
            int8_t flashMode;
            FLASH_Tag flashInfo;
            mSetting->getFLASHTag(&flashInfo);
            mSetting->androidFlashModeToDrvFlashMode(flashInfo.mode,
                                                     &flashMode);
            if (controlInfo.ae_mode != ANDROID_CONTROL_AE_MODE_OFF) {
                if (CAMERA_FLASH_MODE_TORCH == flashMode ||
                    CAMERA_FLASH_MODE_TORCH == mFlashMode) {
                    HAL_LOGD("set flashMode when ae_mode is not off and TORCH");
                    mFlashMode = flashMode;
                    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH,
                             mFlashMode);
                }
            } else {
                HAL_LOGD("set flashMode when ANDROID_CONTROL_AE_MODE_OFF");
                mFlashMode = flashMode;
                SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH,
                         mFlashMode);
            }
        }
        break;

    case ANDROID_SPRD_CAPTURE_MODE: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM,
                 sprddefInfo->capture_mode);
        if (sprddefInfo->capture_mode > 1)
            BurstCapCnt = sprddefInfo->capture_mode;
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
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        struct cmr_ae_param ae_param;
        memset(&ae_param, 0, sizeof(ae_param));
        ae_param.mode = sprddefInfo->am_mode;
        HAL_LOGV("sprddefInfo->am_mode=%d", sprddefInfo->am_mode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AUTO_EXPOSURE_MODE,
                 (cmr_uint)&ae_param);
    } break;

    case ANDROID_SPRD_ISO: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_ISO,
                 (cmr_uint)sprddefInfo->iso);
    } break;

    case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
        setPreviewFps(mRecordingMode);
        break;

    case ANDROID_SPRD_ZSL_ENABLED:
        break;

    case ANDROID_SPRD_CALIBRATION_DIST: {
        VCM_DIST_TAG disc_info;
        vcm_disc_info vcm_disc;
        mSetting->getVCMDISTTag(&disc_info);
        vcm_disc.total_seg = disc_info.vcm_dist_count;
        for (int i = 0; i < disc_info.vcm_dist_count; i++) {
            vcm_disc.distance[i] = disc_info.vcm_dist[i];
        }
        ret = mHalOem->ops->camera_ioctrl(
            mCameraHandle, CAMERA_IOCTRL_SET_VCM_DISC, &vcm_disc);
    } break;

    case ANDROID_SPRD_CALIBRATION_OTP_DATA: {
        CAL_OTP_Tag cal_info;
        struct cal_otp_info otp_info;
        mSetting->getCALOTPTag(&cal_info);
        otp_info.dual_otp_flag = cal_info.dual_otp_flag;
        otp_info.cal_otp_result = cal_info.cal_otp_result;
        otp_info.otp_size = cal_info.otp_size;
        memcpy(otp_info.otp_data, cal_info.otp_data, cal_info.otp_size);
        SET_PARM(mHalOem, mCameraHandle,
                 CAMERA_PARAM_WRITE_CALIBRATION_OTP_DATA,
                 (cmr_uint) & (otp_info));
        HAL_LOGI("write calibration otp result: %d (1:success, 2:fail)",
                 otp_info.cal_otp_result);
        mSetting->setCALOTPRETag(otp_info.cal_otp_result);
    } break;

    case ANDROID_SPRD_SLOW_MOTION: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("slow_motion=%d", sprddefInfo->slowmotion);
        if (sprddefInfo->slowmotion > 1) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SLOW_MOTION_FLAG,
                     sprddefInfo->slowmotion);
        } else {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SLOW_MOTION_FLAG, 0);
        }
    } break;
#ifdef CONFIG_CAMERA_PIPVIV_SUPPORT
    case ANDROID_SPRD_PIPVIV_ENABLED: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("sprd_pipviv_enabled=%d camera id %d",
                 sprddefInfo->sprd_pipviv_enabled, mCameraId);
        if (sprddefInfo->sprd_pipviv_enabled == 1) {
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
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_REFOCUS_ENABLE,
                 sprddefInfo->refocus_enable);

    } break;
    case ANDROID_SPRD_SET_TOUCH_INFO: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        struct touch_coordinate touch_param;
        touch_param.touchX = sprddefInfo->touchxy[0];
        touch_param.touchY = sprddefInfo->touchxy[1];
        HAL_LOGD("ANDROID_SPRD_SET_TOUCH_INFO, touchX = %d, touchY %d",
                 touch_param.touchX, touch_param.touchY);

        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_TOUCH_XY,
                 (cmr_uint)&touch_param);

    } break;
    case ANDROID_SPRD_3DCALIBRATION_ENABLED: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        mSprd3dCalibrationEnabled = sprddefInfo->sprd_3dcalibration_enabled;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DCAL_ENABLE,
                 sprddefInfo->sprd_3dcalibration_enabled);
        HAL_LOGD("sprddefInfo->sprd_3dcalibration_enabled=%d "
                 "mSprd3dCalibrationEnabled %d",
                 sprddefInfo->sprd_3dcalibration_enabled,
                 mSprd3dCalibrationEnabled);
    } break;
    case ANDROID_SPRD_3DNR_ENABLED: {
        SPRD_DEF_Tag *sprddefInfo;
        CONTROL_Tag controlInfo;
        uint8_t sprd_3dnr_enabled;

        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        mSetting->getCONTROLTag(&controlInfo);

        sprd_3dnr_enabled = sprddefInfo->sprd_3dnr_enabled;

        // on flash auto and torch mode, set 3dnr enable to false
        if ((controlInfo.ae_mode == ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH) ||
            (controlInfo.ae_mode == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH &&
             mIsNeedFlashFired == 1) ||
            (sprddefInfo->sprd_flash_lcd_mode == FLASH_LCD_MODE_AUTO &&
             mIsNeedFlashFired == 1) ||
            (sprddefInfo->sprd_flash_lcd_mode == FLASH_LCD_MODE_ON)) {
            sprd_3dnr_enabled = 0;
        }

        if ((sprddefInfo->sprd_is_hdr_scene &&
             sprddefInfo->sprd_auto_hdr_enable) ||
            controlInfo.scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {
            sprd_3dnr_enabled = 0;
        }

        HAL_LOGV("sprd_3dnr_enabled: %d, sprd_auto_3dnr_enable:%d, "
                 "sprd_appmode_id:%d, mIsNeedFlashFired:%d",
                 sprd_3dnr_enabled, sprddefInfo->sprd_auto_3dnr_enable,
                 sprddefInfo->sprd_appmode_id, mIsNeedFlashFired);

        if (sprd_3dnr_enabled == 1) {
            if (sprddefInfo->sprd_auto_3dnr_enable == CAMERA_3DNR_AUTO) {
                // auto 3dnr mode, detected 3dnr scene
#ifdef CONFIG_NIGHT_3DNR_PREV_SW_CAP_SW
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_SW_CAP_SW;
#else
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_NULL_CAP_HW;
#endif
            } else {
#ifdef CONFIG_NIGHT_3DNR_PREV_HW_CAP_HW
                // night shot
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_HW_CAP_HW;
#elif CONFIG_NIGHT_3DNR_PREV_HW_CAP_SW
                // night shot
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_HW_CAP_SW;
#elif CONFIG_NIGHT_3DNR_PREV_NULL_CAP_HW
                // night shot
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_NULL_CAP_SW;

#elif CONFIG_NIGHT_3DNR_PREV_SW_CAP_SW
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_SW_CAP_SW;

#else // default,night shot
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_HW_CAP_SW;
#endif
            }

            if (sprddefInfo->sprd_appmode_id == CAMERA_MODE_NIGHT_PHOTO) {
#if defined(CONFIG_ISP_2_3) // for sharkle
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_SW_CAP_SW;
#else
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_HW_CAP_SW;
#endif
            }
            if (sprddefInfo->sprd_appmode_id == CAMERA_MODE_3DNR_VIDEO ||
                   sprddefInfo->sprd_appmode_id == CAMERA_MODE_NIGHT_VIDEO) {
// 3dnr video mode
#if defined(CONFIG_ISP_2_3) // for sharkle
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_SW_VIDEO_SW;
#else
                mSprd3dnrType = CAMERA_3DNR_TYPE_PREV_HW_VIDEO_HW;
#endif
            }
        } else {
            // 3dnr off
            mSprd3dnrType = CAMERA_3DNR_TYPE_NULL;
        }

        HAL_LOGD("mSprd3dnrType: %d ", mSprd3dnrType);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_3DNR_TYPE,
                 mSprd3dnrType);
    } break;
    case ANDROID_SPRD_FIXED_FPS_ENABLED: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        mFixedFpsEnabled = sprddefInfo->sprd_fixedfps_enabled;
    } break;
    case ANDROID_SPRD_APP_MODE_ID: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        mSprdAppmodeId = sprddefInfo->sprd_appmode_id;
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_SET_APPMODE,
                 sprddefInfo->sprd_appmode_id);
    } break;
    case ANDROID_SPRD_FILTER_TYPE: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("sprddefInfo->sprd_filter_type: %d ",
                 sprddefInfo->sprd_filter_type);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FILTER_TYPE,
                 sprddefInfo->sprd_filter_type);
    } break;
    case ANDROID_LENS_FOCUS_DISTANCE: {
        if (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_OFF) {
            LENS_Tag lensInfo;
            mSetting->getLENSTag(&lensInfo);

            HAL_LOGD("focus_distance:%f", lensInfo.focus_distance);
            if (lensInfo.focus_distance) {
                SET_PARM(mHalOem, mCameraHandle,
                         CAMERA_PARAM_LENS_FOCUS_DISTANCE,
                         (cmr_uint)(lensInfo.focus_distance));
            }
        }
    } break;
    case ANDROID_SPRD_AUTO_HDR_ENABLED: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("sprddefInfo.sprd_auto_hdr_enables=%d, mSprdAppmodeId:%d ",
                 sprddefInfo->sprd_auto_hdr_enable, mSprdAppmodeId);
        if(CAMERA_MODE_AUDIO_PICTURE == mSprdAppmodeId || 
           mSprdAppmodeId == CAMERA_MODE_AUTO_PHOTO ||
           mSprdAppmodeId == CAMERA_MODE_BACK_ULTRA_WIDE) {
#ifdef CONFIG_SUPPROT_AUTO_FDR
            HAL_LOGD("change from auto hdr to auto fdr enable:%d", sprddefInfo->sprd_auto_hdr_enable);
            sprddefInfo->sprd_auto_fdr_enable = sprdInfo.sprd_auto_hdr_enable;
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_AUTO_FDR_ENABLED,
                 sprddefInfo->sprd_auto_fdr_enable);
#else
            HAL_LOGD("set auto hdr because not config CONFIG_SUPPROT_AUTO_FDR");
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_AUTO_HDR_ENABLED,
                 sprddefInfo->sprd_auto_hdr_enable);
#endif
       } else {
            HAL_LOGD("set auto hdr not audio picture mode");
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_AUTO_HDR_ENABLED,
                 sprddefInfo->sprd_auto_hdr_enable);
       }
    } break;
    case ANDROID_SPRD_DEVICE_ORIENTATION: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("sprddefInfo->device_orietation=%d ",
                 sprddefInfo->device_orietation);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SET_DEVICE_ORIENTATION,
                 sprddefInfo->device_orietation);
    } break;
    case ANDROID_SPRD_SET_VERIFICATION_FLAG: {
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        HAL_LOGD("verification_enable:%d", sprddefInfo->verification_enable);
        mSetting->setVERIFITag(sprddefInfo->verification_enable);
    } break;
    case ANDROID_SPRD_AUTOCHASING_REGION: {
        AUTO_TRACKING_Tag autotrackingInfo;
        struct auto_tracking_info info;
        mSetting->getAUTOTRACKINGTag(&autotrackingInfo);
        info.objectX = autotrackingInfo.at_start_info[0];
        info.objectY = autotrackingInfo.at_start_info[1];
        info.status = autotrackingInfo.at_start_info[2];
        info.frame_id = autotrackingInfo.frame_id;
        HAL_LOGD("ANDROID_SPRD_AUTOCHASING_REGION=%d, %d, %d, %d", info.objectX, info.objectY, info.status,
                 info.frame_id);
        /*if(info.objectX == 0 && info.objectY ==0 )
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_AUTO_TRACKING_INFO,
                 (cmr_uint)&info);*/
    } break;
    case ANDROID_SPRD_BLUR_F_NUMBER: {
        LENS_Tag lensInfo;
        mSetting->getLENSTag(&lensInfo);
        if (lensInfo.f_number) {
            SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_F_NUMBER,
                     (cmr_uint)(lensInfo.f_number * 100));
        }
    } break;
    case ANDROID_SPRD_FLASH_LCD_MODE: {
        int8_t flashMode;
        SPRD_DEF_Tag *sprddefInfo;
        sprddefInfo = mSetting->getSPRDDEFTagPTR();
        mSetting->flashLcdModeToDrvFlashMode(sprddefInfo->sprd_flash_lcd_mode,
                                             &flashMode);
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_FLASH, flashMode);
    } break;
    case ANDROID_SPRD_AUTO_3DNR_ENABLED: {
        SPRD_DEF_Tag *sprdInfo;
        sprdInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_AUTO_3DNR_ENABLED,
                 sprdInfo->sprd_auto_3dnr_enable);
    } break;
    case ANDROID_SPRD_AUTOCHASING_REGION_ENABLE: {
        SPRD_DEF_Tag *sprdInfo;
        sprdInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle,
                 CAMERA_PARAM_SPRD_AUTOCHASING_REGION_ENABLE,
                 sprdInfo->sprd_ot_switch);
    } break;
    case ANDROID_SPRD_SMILE_CAPTURE: {
        SPRD_DEF_Tag *sprdInfo;
        sprdInfo = mSetting->getSPRDDEFTagPTR();
    } break;
    case ANDROID_SPRD_LOGOWATERMARK_ENABLED: {
        SPRD_DEF_Tag *sprdInfo;
        sprdInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle,
                 CAMERA_PARAM_SPRD_LOGO_WATERMARK_ENABLED,
                 sprdInfo->sprd_is_logo_watermark);
    } break;
    case ANDROID_SPRD_TIMEWATERMARK_ENABLED: {
        SPRD_DEF_Tag *sprdInfo;
        sprdInfo = mSetting->getSPRDDEFTagPTR();
        SET_PARM(mHalOem, mCameraHandle,
                 CAMERA_PARAM_SPRD_TIME_WATERMARK_ENABLED,
                 sprdInfo->sprd_is_time_watermark);
    } break;
    case ANDROID_SPRD_SUPER_MACROPHOTO_ENABLE: {
        SPRD_DEF_Tag sprdInfo;
        mSetting->getSPRDDEFTag(&sprdInfo);
        SET_PARM(mHalOem, mCameraHandle,
                 CAMERA_PARAM_SPRD_SUPER_MACROPHOTO_ENABLE,
                 sprdInfo.sprd_super_macro);
        break;
    }
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
    SPRD_DEF_Tag *sprddefInfo;

    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    mTopAppId = sprddefInfo->top_app_id;
    HAL_LOGD("cap_mode = %d,sprd_zsl_enabled=%d,mStreamOnWithZsl=%d", cap_mode,
             sprddefInfo->sprd_zsl_enabled, mStreamOnWithZsl);
    switch (cap_mode) {
    case CAMERA_CAPTURE_MODE_PREVIEW:
        /* non-zsl when weichat video for power save */
        if (mTopAppId == TOP_APP_WECHAT) {
            if (mPreviewWidth == 640 && mPreviewHeight == 480 &&
                mCallbackWidth == 640 && mCallbackHeight == 480) {
                mStreamOnWithZsl = 0;
            }
        }

        if (sprddefInfo->sprd_zsl_enabled == 1 || mStreamOnWithZsl == 1) {
            mTakePictureMode = SNAPSHOT_ZSL_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mVideoSnapshotType = 0;
        } else {
            mCaptureMode = CAMERA_NORMAL_MODE;
            mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
        }
        mParaDCDVMode = CAMERA_PREVIEW_FORMAT_DC;
        mRecordingMode = false;
        mZslPreviewMode = false;
        break;

    case CAMERA_CAPTURE_MODE_STILL_CAPTURE:
        if (sprddefInfo->high_resolution_mode &&
            sprddefInfo->fin1_highlight_mode) {
            sprddefInfo->sprd_zsl_enabled = 0;
            mStreamOnWithZsl = 0;
        }
        if (sprddefInfo->sprd_zsl_enabled == 1 || mStreamOnWithZsl == 1) {
            mTakePictureMode = SNAPSHOT_ZSL_MODE;
            mCaptureMode = CAMERA_ZSL_MODE;
            mPicCaptureCnt = 1;
        } else {
            mTakePictureMode = SNAPSHOT_NO_ZSL_MODE;
            mCaptureMode = CAMERA_NORMAL_MODE;
            mPicCaptureCnt = 1;
            if (mIsRawCapture == 1) {
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
        mPreviewFormat = CAM_IMG_FMT_YUV420_NV21;
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

int SprdCamera3OEMIf::freeCameraMemForGpu(cmr_uint *phy_addr,
                                          cmr_uint *vir_addr, cmr_s32 *fd,
                                          cmr_u32 sum) {
    cmr_u32 i;
    int ret = 0;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    HAL_LOGD("mZslHeapNum %d sum %d", mZslHeapNum, sum);
    for (i = 0; i < mZslHeapNum; i++) {
        ret = mapper.unlock(
            (const native_handle_t *)mZslGraphicsHandle[i].native_handle);
        if (ret != NO_ERROR) {
            HAL_LOGE("mapper.unlock fail %p",
                     mZslGraphicsHandle[i].native_handle);
            return ret;
        }
        if (mZslGraphicsHandle[i].graphicBuffer != NULL) {
            mZslGraphicsHandle[i].graphicBuffer.clear();
            mZslGraphicsHandle[i].graphicBuffer = NULL;
            mTotalGpuSize = mTotalGpuSize - mZslGraphicsHandle[i].buf_size;
        }
        HAL_LOGD("graphicBuffer_handle 0x%lx",
                 mZslGraphicsHandle[i].graphicBuffer_handle);
        mZslGraphicsHandle[i].graphicBuffer_handle = NULL;
        mZslGraphicsHandle[i].native_handle = NULL;
        if (NULL != mZslHeapArray[i]) {
            free(mZslHeapArray[i]);
            mZslHeapArray[i] = NULL;
        }
    }
    mZslHeapNum = 0;
    releaseZSLQueue();
    HAL_LOGD("TotalGpuSize=%d", mTotalGpuSize);
    return 0;
}

int SprdCamera3OEMIf::allocCameraMemForGpu(cmr_u32 size, cmr_u32 sum,
                                           cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd) {
    sp<GraphicBuffer> graphicBuffer = NULL;
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;
    int ret = 0;
    void *vaddr = NULL;
    native_handle_t *nativeHandle = NULL;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;
    if (!mIommuEnabled) {
        yuvTextUsage |= GRALLOC_USAGE_VIDEO_BUFFER;
    }
    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    Rect bounds(mCaptureWidth, mCaptureHeight);
    HAL_LOGD("size %d sum %d mZslHeapNum %d", size, sum, mZslHeapNum);

    for (i = 0; i < (cmr_int)mZslNum; i++) {
        if (mZslHeapArray[i] == NULL) {
            sprd_camera_memory_t *memory =
                (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
            if (NULL == memory) {
                HAL_LOGE("fatal error! memory pointer is null.");
                goto mem_fail;
            }
            memset(memory, 0, sizeof(sprd_camera_memory_t));
            mZslHeapArray[i] = memory;
            mZslHeapNum++;
            mTotalGpuSize = mTotalGpuSize + mCaptureWidth * mCaptureHeight;
            graphicBuffer = new GraphicBuffer(mCaptureWidth, mCaptureHeight,
                                              HAL_PIXEL_FORMAT_YCrCb_420_SP, 1,
                                              yuvTextUsage, "sw_3dnr");

            nativeHandle = (native_handle_t *)graphicBuffer->handle;

            mZslHeapArray[i]->fd = ADP_BUFFD(nativeHandle);
            mZslHeapArray[i]->phys_addr = 0;
            mZslHeapArray[i]->phys_size = size;
            ret = mapper.lock((const native_handle_t *)nativeHandle, usage,
                              bounds, &vaddr);
            if (ret) {
                HAL_LOGE("mapper.lock failed, ret=%d", ret);
                goto mem_fail;
            }
            mZslHeapArray[i]->data = (void *)vaddr;
            mZslGraphicsHandle[i].graphicBuffer = graphicBuffer;
            mZslGraphicsHandle[i].graphicBuffer_handle = graphicBuffer.get();
            mZslGraphicsHandle[i].native_handle = nativeHandle;
            mZslGraphicsHandle[i].buf_size = mCaptureWidth * mCaptureHeight;
            HAL_LOGD("graphicBuffer_handle 0x%lx",
                     mZslGraphicsHandle[i].graphicBuffer_handle);
        }
        *phy_addr++ = (cmr_uint)mZslHeapArray[i]->phys_addr;
        *vir_addr++ = (cmr_uint)mZslHeapArray[i]->data;
        *fd++ = mZslHeapArray[i]->fd;
    }
    HAL_LOGD("TotalGpuSize=%d", mTotalGpuSize);
    return 0;

mem_fail:
    freeCameraMemForGpu(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);
    Mutex::Autolock zsllock(&mZslBufLock);

    if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW) {
        freeCameraMemForGpu(phy_addr, vir_addr, fd, sum);
        return 0;
    }
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
    int ret;
    SPRD_DEF_Tag *sprddefInfo;
    int BufferCount = kZslBufferCount;

    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    if (sprddefInfo->slowmotion <= 1)
        BufferCount = 8;

    HAL_LOGD("size %d sum %d mZslHeapNum %d, BufferCount %d", size, sum,
             mZslHeapNum, BufferCount);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW) {
        ret = allocCameraMemForGpu(size, sum, phy_addr, vir_addr, fd);
        return ret;
    }

    if (mZslHeapNum >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("error mZslHeapNum %d", mZslHeapNum);
        return BAD_VALUE;
    }

    if ((mZslHeapNum + sum) >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mZslHeapNum, sum);
        return BAD_VALUE;
    }

    if (mSprdZslEnabled == true) {
        releaseZSLQueue();
        if (mMultiCameraMode == MODE_BOKEH) {
#ifndef CONFIG_BOKEH_HDR_SUPPORT
            mZslNum = 1;
#endif
        }
        HAL_LOGD("mZslNum %d", mZslNum);
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
    }

    return 0;

mem_fail:
    Callback_ZslFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_Zsl_raw_Malloc(cmr_u32 size, cmr_u32 sum,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;
    int ret;

    HAL_LOGD("size %d sum %d mZslHeapNum %d", size, sum, mZslHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (mZslRawHeapNum >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("error mPreviewHeapNum %d", mZslRawHeapNum);
        return BAD_VALUE;
    }

    if ((mZslRawHeapNum + sum) >= (kZslBufferCount + kZslRotBufferCount + 1)) {
        HAL_LOGE("malloc is too more %d %d", mZslRawHeapNum, sum);
        return BAD_VALUE;
    }

        HAL_LOGD("zsl_raw num %d", sum);
        for (i = 0; i < (cmr_int)sum; i++) {
            if(mZslRawHeapArray[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("error memory is null.");
                    goto mem_fail;
                }
                mZslRawHeapArray[i] = memory;
                mZslRawHeapNum++;
            }
            *phy_addr++ = (cmr_uint)mZslRawHeapArray[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mZslRawHeapArray[i]->data;
            *fd++ = mZslRawHeapArray[i]->fd;
        }

        return 0;

mem_fail:
    Callback_ZslRawFree(0, 0, 0, 0);
    return BAD_VALUE;
}

int SprdCamera3OEMIf::Callback_ZslRawFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);
    Mutex::Autolock zsllock(&mZslBufLock);
    SPRD_DEF_Tag sprddefInfo;

    HAL_LOGD("mZslRawHeapNum %d sum %d", mZslRawHeapNum, sum);
    for (i = 0; i < mZslRawHeapNum; i++) {
        if (NULL != mZslRawHeapArray[i]) {
            freeCameraMem(mZslRawHeapArray[i]);
        }
        mZslRawHeapArray[i] = NULL;
    }

    mZslRawHeapNum = 0;

    return 0;
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

int SprdCamera3OEMIf::Callback_Sw3DNRCapturePathMalloc(
    cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr,
    cmr_s32 *fd, void **handle, cmr_uint width, cmr_uint height) {
#ifdef CAMERA_3DNR_CAPTURE_GPU
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;
    private_handle_t *buffer = NULL;
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;

    HAL_LOGI(
        "Callback_Sw3DNRCapturePathMalloc: size %d sum %d mPathRawHeapNum %d"
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
        for (i = 0; i < (cmr_int)mZslNum; i++) {
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

            buffer = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION,
                                          0x130, size, (void *)memory->data, 0);
            if (NULL == buffer) {
                HAL_LOGE("mem alloc 3dnr graphic buffer failed");
                goto mem_fail;
            }
            if (buffer->share_attr_fd < 0) {
                buffer->share_attr_fd = ashmem_create_region(
                    "camera_gralloc_shared_attr", PAGE_SIZE);
                if (buffer->share_attr_fd < 0) {
                    ALOGE(
                        "Failed to allocate page for shared attribute region");
                    goto mem_fail;
                }
            }

            buffer->attr_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, buffer->share_attr_fd, 0);
            if (buffer->attr_base != MAP_FAILED) {
                attr_region *region = (attr_region *)buffer->attr_base;
                memset(buffer->attr_base, 0xff, PAGE_SIZE);
                munmap(buffer->attr_base, PAGE_SIZE);
                buffer->attr_base = MAP_FAILED;
            } else {
                ALOGE("Failed to mmap shared attribute region");
                m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].bufferhandle =
                    NULL;
                m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].private_handle =
                    buffer;
                m3dnrGraphicPathHeapNum++;
                goto mem_fail;
            }

            buffer->share_fd = memory->fd;
            buffer->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            buffer->byte_stride = width;
            buffer->internal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            buffer->width = width;
            buffer->height = height;
            buffer->stride = width;
            buffer->internalWidth = width;
            buffer->internalHeight = height;

#if defined(CONFIG_SPRD_ANDROID_8)
            sp<GraphicBuffer> pbuffer = new GraphicBuffer(
                buffer, GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, width,
                height, HAL_PIXEL_FORMAT_YCrCb_420_SP, 1, yuvTextUsage, width);
            *handle = pbuffer.get();
            HAL_LOGD("add alloc graphic buffer in CapturePathMalloc "
                     "index:%ld , buffer:%p",
                     i, *handle);
#else
            sp<GraphicBuffer> pbuffer =
                new GraphicBuffer(width, height, HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                  yuvTextUsage, width, buffer, 0);
            *handle = pbuffer.get();
#endif
            mTotalGpuSize = mTotalGpuSize + width * height;
            handle++;
            m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].bufferhandle =
                pbuffer;
            m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].private_handle =
                buffer;
            m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].buf_size =
                width * height;
            m3dnrGraphicPathHeapNum++;
        }
    } else if (0 == m3dnrGraphicPathHeapNum) {
        if ((mPathRawHeapNum >= sum) && (mPathRawHeapSize >= size)) {
            HAL_LOGD("3DNR Graphic path is null, use pre-alloc path array");
            mPathRawHeapSize = size;
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mPathRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mPathRawHeapArray[i]->data;
                *fd++ = mPathRawHeapArray[i]->fd;

                buffer = new private_handle_t(
                    private_handle_t::PRIV_FLAGS_USES_ION, 0x130, size,
                    (void *)mPathRawHeapArray[i]->data, 0);
                if (NULL == buffer) {
                    HAL_LOGE("mem alloc 3dnr graphic buffer failed");
                    goto mem_fail;
                }
                if (buffer->share_attr_fd < 0) {
                    buffer->share_attr_fd = ashmem_create_region(
                        "camera_gralloc_shared_attr", PAGE_SIZE);
                    if (buffer->share_attr_fd < 0) {
                        ALOGE("Failed to allocate page for shared attribute "
                              "region");
                        goto mem_fail;
                    }
                }

                buffer->attr_base =
                    mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                         buffer->share_attr_fd, 0);
                if (buffer->attr_base != MAP_FAILED) {
                    attr_region *region = (attr_region *)buffer->attr_base;
                    memset(buffer->attr_base, 0xff, PAGE_SIZE);
                    munmap(buffer->attr_base, PAGE_SIZE);
                    buffer->attr_base = MAP_FAILED;
                } else {
                    ALOGE("Failed to mmap shared attribute region");
                    m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum]
                        .bufferhandle = NULL;
                    m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum]
                        .private_handle = buffer;
                    m3dnrGraphicPathHeapNum++;
                    goto mem_fail;
                }

                buffer->share_fd = mPathRawHeapArray[i]->fd;
                buffer->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                buffer->byte_stride = width;
                buffer->internal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                buffer->width = width;
                buffer->height = height;
                buffer->stride = width;
                buffer->internalWidth = width;
                buffer->internalHeight = height;

#if defined(CONFIG_SPRD_ANDROID_8)
                sp<GraphicBuffer> pbuffer = new GraphicBuffer(
                    buffer, GraphicBuffer::HandleWrapMethod::CLONE_HANDLE,
                    width, height, HAL_PIXEL_FORMAT_YCrCb_420_SP, 1,
                    yuvTextUsage, width);
                *handle = pbuffer.get();
                HAL_LOGD("2 add alloc graphic buffer in CapturePathMalloc "
                         "index:%ld , buffer:%p",
                         i, *handle);
#else
                sp<GraphicBuffer> pbuffer = new GraphicBuffer(
                    width, height, HAL_PIXEL_FORMAT_YCrCb_420_SP, yuvTextUsage,
                    width, buffer, 0);
                *handle = pbuffer.get();
#endif
                mTotalGpuSize = mTotalGpuSize + width * height;
                handle++;
                m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].bufferhandle =
                    pbuffer;
                m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].private_handle =
                    buffer;
                m3DNRGraphicPathArray[m3dnrGraphicPathHeapNum].buf_size =
                    width * height;
                m3dnrGraphicPathHeapNum++;
            }
        } else {
            LOGE("failed to alloc graphic buffer, malloced num %d,request num "
                 "%d, "
                 "size 0x%x, request size 0x%x",
                 mPathRawHeapNum, sum, mPathRawHeapSize, size);
            goto mem_fail;
        }
    } else {
        if ((mPathRawHeapNum >= sum) && (mPathRawHeapSize >= size)) {
            LOGI("Callback_CapturePathMalloc :test");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mPathRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mPathRawHeapArray[i]->data;
                *fd++ = mPathRawHeapArray[i]->fd;
                *handle = m3DNRGraphicPathArray[i].bufferhandle.get();
            }
        } else {
            LOGE("failed to malloc memory, malloced num %d,request num %d, "
                 "size 0x%x, request size 0x%x",
                 mPathRawHeapNum, sum, mPathRawHeapSize, size);
            goto mem_fail;
        }
    }
    HAL_LOGD("TotalGpuSize=%d", mTotalGpuSize);
    return 0;

mem_fail:
    Callback_Sw3DNRCapturePathFree(0, 0, 0, 0);
    return -1;
#else
    return 0;
#endif

}

int SprdCamera3OEMIf::Callback_Sw3DNRCapturePathFree(cmr_uint *phy_addr,
                                                     cmr_uint *vir_addr,
                                                     cmr_s32 *fd, cmr_u32 sum) {
#ifdef CAMERA_3DNR_CAPTURE_GPU
    uint32_t i = 0;
    HAL_LOGI("Free Gpu buffer start");
    Callback_CapturePathFree(0, 0, 0, 0);
    Callback_ZslFree(0, 0, 0, 0);

    for (i = 0; i < m3dnrGraphicPathHeapNum; i++) {
        if (m3DNRGraphicPathArray[i].bufferhandle != NULL) {
            m3DNRGraphicPathArray[i].bufferhandle.clear();
            m3DNRGraphicPathArray[i].bufferhandle = NULL;
            mTotalGpuSize = mTotalGpuSize - m3DNRGraphicPathArray[i].buf_size;
        }
        if (m3DNRGraphicPathArray[i].private_handle != NULL) {
            struct private_handle_t *pHandle =
                (private_handle_t *)m3DNRGraphicPathArray[i].private_handle;
            if (pHandle->attr_base != MAP_FAILED) {
                HAL_LOGI("Warning shared attribute region mapped at free. "
                         "Unmapping");
                munmap(pHandle->attr_base, PAGE_SIZE);
                pHandle->attr_base = MAP_FAILED;
            }
            close(pHandle->share_attr_fd);
            pHandle->share_attr_fd = -1;
            delete pHandle;
            m3DNRGraphicPathArray[i].private_handle = NULL;
        }
    }
    m3dnrGraphicPathHeapNum = 0;
#else
#endif
    HAL_LOGD("TotalGpuSize=%d", mTotalGpuSize);
    return 0;
}

int SprdCamera3OEMIf::Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                                 cmr_uint *phy_addr,
                                                 cmr_uint *vir_addr,
                                                 cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    HAL_LOGI("Callback_CapturePathMalloc: size %d sum %d mPathRawHeapNum %d"
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
            HAL_LOGI("Callback_CapturePathMalloc :test");
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

int SprdCamera3OEMIf::Callback_GraphicBufferMalloc(
    enum camera_mem_cb_type type,cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
    cmr_uint *vir_addr, cmr_s32 *fd, void **handle, cmr_uint width, cmr_uint height) {
    int ret = NO_ERROR;

    hal_mem_info_t buf_mem_info;
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;

    SprdCamera3GrallocMemory *memory = new SprdCamera3GrallocMemory();
    MemGpuQueue memGpu_queue;
    HAL_LOGV("ultra wide malloc %d, shape: %d x %d, size: %d!", sum, width,
             height, size);
    for (cmr_u32 i = 0; i < sum; i++) {
        if (mGraphicBufNum >= MAX_GRAPHIC_BUF_NUM)
            goto malloc_failed;

        mTotalGpuSize = mTotalGpuSize + width * height;
        sp<GraphicBuffer> pbuffer = new GraphicBuffer(
            width, height, HAL_PIXEL_FORMAT_YCrCb_420_SP, yuvTextUsage,
            std::string("Camera3OEMIf GraphicBuffer"));
        ret = pbuffer->initCheck();
        if (ret)
            goto malloc_failed;

        if (!pbuffer->handle)
            goto malloc_failed;
        if (mIsUltraWideMode) {
            int usage =
                GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
            Rect bounds(width, height);
            void *vaddr = NULL;
            android_ycbcr ycbcr;
            bzero((void *)&ycbcr, sizeof(ycbcr));

            ret = pbuffer->lockYCbCr(usage, bounds, &ycbcr);
            if (ret != NO_ERROR) {
                ret = pbuffer->lock(usage, bounds, &vaddr);
                if (ret != NO_ERROR) {
                    HAL_LOGE("GraphicBuffer lock  fail, ret %d", ret);
                } else {
                    buf_mem_info.addr_vir = vaddr;
                }
            } else {
                buf_mem_info.addr_vir = ycbcr.y;
            }
            buf_mem_info.fd = ADP_BUFFD(pbuffer->handle);
            buf_mem_info.addr_phy = (void *)0;
        } else {
            ret = memory->map(&(pbuffer->handle), &buf_mem_info);
        }
        if (ret == NO_ERROR) {
            phy_addr[i] = (cmr_uint)buf_mem_info.addr_phy;
            vir_addr[i] = (cmr_uint)buf_mem_info.addr_vir;
            fd[i] = buf_mem_info.fd;
        } else {
            phy_addr[i] = 0;
            vir_addr[i] = 0;
            fd[i] = 0;
            goto malloc_failed;
        }

        *handle = pbuffer.get();
        handle++;
        memGpu_queue.mGpuHeap.bufferhandle = pbuffer;
        if (type == CAMERA_VIDEO_EIS_ULTRA_WIDE) {
            memGpu_queue.mGpuHeap.private_handle = (void *)fd;
            HAL_LOGD("fd=0x%x", fd);
        } else
            memGpu_queue.mGpuHeap.private_handle = NULL;
        memGpu_queue.mGpuHeap.buf_size = width * height;
        memGpu_queue.mem_type = type;
        cam_MemGpuQueue.push_back(memGpu_queue);
        mGraphicBufNum++;
    }
    HAL_LOGD("mem_type=%d sum=%d TotalGpuSize=%d", type, sum, mTotalGpuSize);
    delete memory;
    return NO_ERROR;

malloc_failed:
    LOGE("Failed to alloc graphic buffer, malloced num %d,request num "
         "%d, request size 0x%x!",
         mGraphicBufNum, sum, size);
    delete memory;
    Callback_GraphicBufferFree(type, 0, 0, 0, 0);
    return ret;
}

int SprdCamera3OEMIf::Callback_GraphicBufferFree(
                    enum camera_mem_cb_type type, cmr_uint *phy_addr,
                    cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i = 0;
    SprdCamera3GrallocMemory *memory = new SprdCamera3GrallocMemory();

    Callback_CaptureFree(0, 0, 0, 0);
    Callback_ZslFree(0, 0, 0, 0);

    for (List<MemGpuQueue>::iterator i = cam_MemGpuQueue.begin(); i != cam_MemGpuQueue.end(); i++) {
        if ((type == i->mem_type) && i->mGpuHeap.bufferhandle != NULL) {
            if (mIsUltraWideMode) {
                i->mGpuHeap.bufferhandle->unlock();
            } else {
                memory->unmap(&(i->mGpuHeap.bufferhandle->handle),
                              NULL);
            }
            mTotalGpuSize = mTotalGpuSize - (i->mGpuHeap.buf_size);
            i->mGpuHeap.bufferhandle.clear();
            i->mGpuHeap.bufferhandle = NULL;
            cam_MemGpuQueue.erase(i);
        }
    }
    mGraphicBufNum = 0;
    HAL_LOGD("mem_type=%d sum=%d TotalGpuSize=%d", type, sum, mTotalGpuSize);
    delete memory;

    return 0;
}

int SprdCamera3OEMIf::Callback_CapturePathFree(cmr_uint *phy_addr,
                                               cmr_uint *vir_addr, cmr_s32 *fd,
                                               cmr_u32 sum) {
    cmr_u32 i;

    HAL_LOGI("Callback_CapturePathFree: mPathRawHeapNum %d sum %d",
             mPathRawHeapNum, sum);

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

int SprdCamera3OEMIf::Callback_CommonFree(enum camera_mem_cb_type type,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);

    HAL_LOGD("mem_type=%d sum=%d", type, sum);

    if (type == CAMERA_ISPSTATS_DEBUG) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspStatsDebugHeap[i]) {
                freeCameraMem(mIspStatsDebugHeap[i]);
            }
            mIspStatsDebugHeap[i] = NULL;
        }
    }

    for (List<MemIonQueue>::iterator i = cam_MemIonQueue.begin(); i != cam_MemIonQueue.end(); i++) {
        if ((type == i->mem_type) && (NULL != i->mIonHeap)) {
            HAL_LOGV("mem_type=%d mIonHeap=%p", type, i->mIonHeap);
            freeCameraMem(i->mIonHeap);
            i->mIonHeap = NULL;
            cam_MemIonQueue.erase(i);
        }
    }

    return 0;
}

int SprdCamera3OEMIf::Callback_CommonMalloc(enum camera_mem_cb_type type,
                                           cmr_u32 size, cmr_u32 *sum_ptr,
                                           cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    MemIonQueue memIon_queue;
    cmr_u32 reuse = 0, totalNum = 0;
    cmr_u32 i;
    cmr_u32 mem_size;
    cmr_u32 sum = *sum_ptr;

    HAL_LOGD("mem_type=%d size=%d sum=%d", type, size, sum);
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
#if defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6)
            mem_size = mLargestPictureWidth * mLargestPictureHeight * 3 / 2;
#else
            /* from sharkl5pro, raw16 should be supported */
            char prop[PROPERTY_VALUE_MAX] = {0};
            property_get("persist.vendor.cam.res.multi.camera.fullsize", prop, "0");
            if (atoi(prop) == 1 && mCameraId == sensorGetRole(MODULE_SPW_NONE_BACK)) {
                cmr_u16 picW = 0;
                cmr_u16 picH = 0;
                mSetting->getLargestPictureSize(sensorGetRole(MODULE_OPTICSZOOM_WIDE_BACK), &picW, &picH);
                mLargestPictureWidth = picW;
                mLargestPictureHeight = picH;
            }
            mem_size = mLargestPictureWidth * mLargestPictureHeight * 2;
            HAL_LOGV("mLargestPictureWidth=%d, mLargestPictureHeight=%d", mLargestPictureWidth, mLargestPictureHeight);
#endif
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
        return 0;
    }
#endif
    //ion memory reuse
    for (List<MemIonQueue>::iterator i = cam_MemIonQueue.begin(); i != cam_MemIonQueue.end(); i++) {
        if ((type == i->mem_type) && (NULL != i->mIonHeap)) {
            HAL_LOGD("use pre-alloc this type mem");
            if (type == CAMERA_ISP_STATIS) {
                cmr_u64 kaddr = 0;
                *phy_addr++ = kaddr;
                *phy_addr = kaddr >> 32;
                *vir_addr++ = (cmr_uint)(i->mIonHeap)->data;
                *fd++ = (i->mIonHeap)->fd;
                *fd++ = (i->mIonHeap)->dev_fd;
            } else {
                *phy_addr++ = (cmr_uint)(i->mIonHeap)->phys_addr;
                *vir_addr++ = (cmr_uint)(i->mIonHeap)->data;
                *fd++ = (i->mIonHeap)->fd;
            }
            reuse++;
        }
    }
    totalNum = sum - reuse;
    if (totalNum > 0) {
        for (i = 0; i < totalNum; i++) {
            memIon_queue.mIonHeap = allocCameraMem(size, 1, mem_cb_stat[type].is_cache);
            if (NULL == memIon_queue.mIonHeap) {
               HAL_LOGE("error memory is null,malloced type %d", type);
               goto mem_fail;
            }
            memIon_queue.mem_type = type;
            HAL_LOGV("mem_type=%d mIonHeap=%p is_cache=%d",memIon_queue.mem_type,
                      memIon_queue.mIonHeap, mem_cb_stat[type].is_cache);
            cam_MemIonQueue.push_back(memIon_queue);
            if ((type == CAMERA_CHANNEL_1 || type == CAMERA_CHANNEL_2 ||
                 type == CAMERA_CHANNEL_3 || type == CAMERA_CHANNEL_4) &&
                (memIon_queue.mIonHeap->data ==NULL || memIon_queue.mIonHeap->fd < 0)) {
                HAL_LOGE("malloced type %d", type);
                goto mem_fail;
            }
            if (type == CAMERA_ISP_STATIS) {
                cmr_u64 kaddr = 0;
                //size_t ksize = 0;
                //cmr_s32 rtn = 0;
                // shark5 dont need kernel software r/w the buffer
               //rtn = mIspStatisHeapReserved->ion_heap->get_kaddr(&kaddr, &ksize);
              // if (rtn) {
              //    HAL_LOGE("get kaddr error");
              //    goto mem_fail;
              //}
                *phy_addr++ = kaddr;
                *phy_addr = kaddr >> 32;
                *vir_addr++ = (cmr_uint)memIon_queue.mIonHeap->data;
                *fd++ = memIon_queue.mIonHeap->fd;
                *fd++ = memIon_queue.mIonHeap->dev_fd;
            } else {
                *phy_addr++ = (cmr_uint)memIon_queue.mIonHeap->phys_addr;
                *vir_addr++ = (cmr_uint)memIon_queue.mIonHeap->data;
                *fd++ = memIon_queue.mIonHeap->fd;
            }
        }
    }

    return 0;

mem_fail:
    Callback_CommonFree(type, 0, 0, 0, 0);
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
    // CAMERA_PREVIEW & CAMERA_VIDEO type ION buffer from framework
    if (CAMERA_PREVIEW == type || CAMERA_VIDEO == type || CAMERA_SNAPSHOT == type) {
        // Performance optimization:move Callback_CaptureFree to closeCamera
        // function
        // ret = camera->Callback_CaptureFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT_ZSL_RAW == type) {
        ret = camera->Callback_ZslRawFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathFree(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_PREVIEW_ULTRA_WIDE == type ||
               CAMERA_VIDEO_ULTRA_WIDE == type ||
               CAMERA_SNAPSHOT_ULTRA_WIDE == type ||
               CAMERA_VIDEO_EIS_ULTRA_WIDE == type) {
        ret = camera->Callback_GraphicBufferFree(type, phy_addr, vir_addr, fd, sum);
    }else if (CAMERA_SNAPSHOT_SW3DNR == type) {
        ret = camera->Callback_Sw3DNRCapturePathFree(phy_addr, vir_addr, fd, sum);
    } else {
        ret = camera->Callback_CommonFree(type, phy_addr, vir_addr, fd, sum);
    }

    HAL_LOGV("X");
    return ret;
}

int SprdCamera3OEMIf::Callback_IonMalloc(enum camera_mem_cb_type type,
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

    if (CAMERA_PREVIEW == type || CAMERA_VIDEO == type) {
        //preview & video buffer from framework
        HAL_LOGD("Do not need malloc,mem_type %d buffer from framework", type);
    } else if (CAMERA_SNAPSHOT == type) {
        ret = camera->Callback_CaptureMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->Callback_ZslMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_ZSL_RAW == type) {
        ret = camera->Callback_Zsl_raw_Malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_PATH == type) {
        ret = camera->Callback_CapturePathMalloc(size, sum, phy_addr, vir_addr,
                                                 fd);
    } else {
        ret = camera->Callback_CommonMalloc(type, size, sum_ptr, phy_addr,
                                           vir_addr, fd);
    }

    HAL_LOGV("X");
    return ret;
}

int SprdCamera3OEMIf::Callback_GpuMalloc(enum camera_mem_cb_type type,
                                         cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, void **handle,
                                         cmr_uint *width, cmr_uint *height,
                                         void *private_data) {
    SprdCamera3OEMIf *camera = (SprdCamera3OEMIf *)private_data;
    int ret = 0, i = 0;
    uint32_t size, sum;
    SprdCamera3RegularChannel *channel = NULL;
    HAL_LOGV("E");

    if (!private_data || !vir_addr || !fd || !size_ptr || !sum_ptr ||
        (0 == *size_ptr) || (0 == *sum_ptr)) {
        HAL_LOGE("param error 0x%lx 0x%lx 0x%lx 0x%lx", (cmr_uint)handle,
                 (cmr_uint)private_data, (cmr_uint)size_ptr, (cmr_uint)sum_ptr);
        return BAD_VALUE;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (type >= CAMERA_MEM_CB_TYPE_MAX) {
        HAL_LOGE("mem type error %ld", (cmr_uint)type);
        return BAD_VALUE;
    }

    switch (type) {
    case CAMERA_PREVIEW_ULTRA_WIDE:
    case CAMERA_VIDEO_ULTRA_WIDE:
    case CAMERA_VIDEO_EIS_ULTRA_WIDE:
        ret = camera->Callback_GraphicBufferMalloc(type,
            size, sum, phy_addr, vir_addr, fd, handle, *width, *height);
        break;
    case CAMERA_SNAPSHOT_ULTRA_WIDE:
        // malloc with callback_graphicbuffermalloc
        sum = camera->mZslNum;
        ret = camera->Callback_GraphicBufferMalloc(type,
            size, sum, phy_addr, vir_addr, fd, handle, *width, *height);
         // record in mzslHeapArray
        for (i = 0; i < (cmr_int)camera->mZslNum; i++) {
            sprd_camera_memory_t *memory =
                (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
            memset(memory, 0, sizeof(sprd_camera_memory_t));
            memory->busy_flag = false;
            memory->ion_heap = NULL;
            memory->fd = fd[i];
            memory->phys_addr = 0;
            memory->phys_size = size;
            memory->data = (void *)vir_addr[i];
            camera->mZslHeapArray[camera->mZslHeapNum] = memory;
            camera->mZslHeapNum += 1;
        }
        break;
    case CAMERA_SNAPSHOT_SW3DNR:
        ret = camera->Callback_Sw3DNRCapturePathMalloc(
            size, sum, phy_addr, vir_addr, fd, handle, *width, *height);

        for (i = 0; i < (cmr_int)camera->mZslNum; i++) {
            sprd_camera_memory_t *memory =
                (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
            memset(memory, 0, sizeof(sprd_camera_memory_t));
            memory->busy_flag = false;
            memory->ion_heap = NULL;
            memory->fd = fd[i];
            memory->phys_addr = 0;
            memory->phys_size = size;
            memory->data = (void *)vir_addr[i];
            camera->mZslHeapArray[camera->mZslHeapNum] = memory;
            camera->mZslHeapNum += 1;
        }
        break;
    default:
        break;
    }

    HAL_LOGV("X");
    return ret;
}
int SprdCamera3OEMIf::SetChannelHandle(void *regular_chan, void *picture_chan) {
    mRegularChan = regular_chan;
    mPictureChan = picture_chan;
    return NO_ERROR;
}

int SprdCamera3OEMIf::setCamStreamInfo(struct img_size size, int format,
                                       int stream_tpye) {
    uint32_t imageFormat = CAM_IMG_FMT_BAYER_MIPI_RAW;
    int isYuvSensor = 0, i;
    SPRD_DEF_Tag *sprddefInfo;
    char value[PROPERTY_VALUE_MAX];
    struct img_size req_size;
    struct sensor_mode_info mode_info[SENSOR_MODE_MAX];

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return UNKNOWN_ERROR;
    }

    sprddefInfo = mSetting->getSPRDDEFTagPTR();

    mHalOem->ops->camera_ioctrl(mCameraHandle, CAMERA_IOCTRL_GET_SENSOR_FORMAT,
                                &imageFormat);
    if (imageFormat == CAM_IMG_FMT_YUV422P) {
        isYuvSensor = 1;
    }

    switch (stream_tpye) {
    case CAMERA_STREAM_TYPE_PREVIEW:
        mPreviewWidth = size.width;
        mPreviewHeight = size.height;
        if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP ||
            format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
            format == HAL_PIXEL_FORMAT_YCBCR_420_888) {
            mPreviewFormat = CAM_IMG_FMT_YUV420_NV21;
        } else if (format == HAL_PIXEL_FORMAT_RAW16) {
            mPreviewFormat = CAM_IMG_FMT_BAYER_MIPI_RAW;
        }
        if (isYuvSensor) {
            mPreviewFormat = CAM_IMG_FMT_YUV422P;
        }
        break;
    case CAMERA_STREAM_TYPE_VIDEO:
        mVideoWidth = size.width;
        mVideoHeight = size.height;
        if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP ||
            format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            mVideoFormat = CAM_IMG_FMT_YUV420_NV21;
        }
        if (isYuvSensor) {
            mVideoFormat = CAM_IMG_FMT_YUV422P;
        }

        if (mVideoWidth > 0 && mVideoWidth == mCaptureWidth &&
            mVideoHeight == mCaptureHeight && sprddefInfo->slowmotion <= 1) {
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
            format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
            format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
            mCallbackFormat = CAM_IMG_FMT_YUV420_NV21;
        }
        if (isYuvSensor) {
            mCallbackFormat = CAM_IMG_FMT_YUV422P;
        }
        break;
    case CAMERA_STREAM_TYPE_YUV2:
        mYuv2Width = size.width;
        mYuv2Height = size.height;
        if (format == HAL_PIXEL_FORMAT_YCbCr_420_888 ||
            format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
            format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
            mYuv2Format = CAM_IMG_FMT_YUV420_NV21;
        }
        if (isYuvSensor) {
            mYuv2Format = CAM_IMG_FMT_YUV422P;
        }
        break;
    case CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT:
        mCaptureWidth = size.width;
        mCaptureHeight = size.height;
        if (format == HAL_PIXEL_FORMAT_BLOB) {
            mPictureFormat = CAM_IMG_FMT_YUV420_NV21;
        }
        if (isYuvSensor) {
            mPictureFormat = CAM_IMG_FMT_YUV422P;
        }

        if (getMultiCameraMode() != MODE_TUNING) {
            if (mIsRawCapture == 1) {
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
                HAL_LOGD(
                    "raw capture mode: mCaptureWidth=%d, mCaptureHeight=%d",
                    mCaptureWidth, mCaptureHeight);
            }
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
        if (mIsIspToolMode == 1 &&
            getPreviewState() != SPRD_INTERNAL_PREVIEW_REQUESTED &&
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
        if (mIsIspToolMode == 1 &&
            getPreviewState() != SPRD_INTERNAL_PREVIEW_REQUESTED &&
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
            getMultiCameraMode() != MODE_BOKEH &&
            getMultiCameraMode() != MODE_3D_CALIBRATION &&
            getMultiCameraMode() != MODE_MULTI_CAMERA) {
            mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                       SPRD_CAM_STREAM_CALLBACK);
        }
        break;
    case CAMERA_STREAM_TYPE_YUV2:
        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel == NULL) {
            ret = -1;
            HAL_LOGE("mRegularChan is null");
            goto exit;
        }

        ret = channel->getStream(CAMERA_STREAM_TYPE_YUV2, &stream);
        if (ret || stream == NULL) {
            HAL_LOGE("getStream failed");
            goto exit;
        }

        ret = stream->getQBufInfoForHandle(buff_handle, &buffer);
        if (ret || buffer.addr_vir == NULL) {
            HAL_LOGE("getQBufForHandle failed");
            goto exit;
        }

        mHalOem->ops->queue_buffer(mCameraHandle, buffer, SPRD_CAM_STREAM_YUV2);
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
                ret = stream->getQBufFirstBuf(&buffer);
                if (ret) {
                    CMR_LOGE("getQBufFirstBuf failed");
                    goto exit;
                }
                mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                           SPRD_CAM_STREAM_PREVIEW);
            }
        }
        break;

    case CAMERA_STREAM_TYPE_VIDEO:
        channel = reinterpret_cast<SprdCamera3RegularChannel *>(mRegularChan);
        if (channel) {
            ret = channel->getStream(CAMERA_STREAM_TYPE_VIDEO, &stream);
            if (stream) {
                ret = stream->getQBufFirstBuf(&buffer);
                if (ret) {
                    CMR_LOGE("getQBufFirstBuf failed");
                    goto exit;
                }
                mHalOem->ops->queue_buffer(mCameraHandle, buffer,
                                           SPRD_CAM_STREAM_VIDEO);
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

    HAL_LOGI("E");

    if (mVideoShotPushFlag == 0 && frame_number == mVideoShotNum)
        mVideoShotWait.wait(mPreviewCbLock);

    if (mVideoShotPushFlag) {
        channel->getStream(type, &stream);

        if (stream) {
            ret = stream->getQBufAddrForNum(frame_number, &addr_vir, &addr_phy,
                                            &fd);
            HAL_LOGD("mDualVideoMode %d", mDualVideoMode);
            if (mDualVideoMode)
                ret = stream->getBufAddrForDualVideo(mSnapBuffInfo, &addr_vir,
                                                     &addr_phy, &fd);
            HAL_LOGD("mCameraId = %d, addr_phy = 0x%lx, addr_vir = 0x%lx, "
                     "fd = 0x%x, frame_number = %d",
                     mCameraId, addr_phy, addr_vir, fd, frame_number);
            if (mCameraHandle != NULL && mHalOem != NULL &&
                mHalOem->ops != NULL && ret == NO_ERROR &&
                addr_vir != (cmr_uint)NULL)
                mHalOem->ops->camera_set_video_snapshot_buffer(
                    mCameraHandle, addr_phy, addr_vir, fd);

            mVideoShotPushFlag = 0;
            mVideoShotFlag = 0;
            mDualVideoShotFlag = 0;
        }
    }

    HAL_LOGD("X");

    return NO_ERROR;
}

void SprdCamera3OEMIf::pushDualVideoBuffer(hal_mem_info_t *mem_info) {
    mSnapBuffInfo = mem_info;
    mDualVideoShotFlag = 1;
    HAL_LOGD("pushDualVideoBuffer %p", mSnapBuffInfo);
}

void SprdCamera3OEMIf::setRealMultiMode(bool mode) {
    mDualVideoShotFlag = 0;
    mDualVideoMode = mode;
    HAL_LOGD("mDualVideoMode %d", mDualVideoMode);
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
        if (mZslHeapArray[i] == NULL) {
            HAL_LOGE("mZslHeapArray[%d] is null", i);
            goto exit;
        }

        if (0 != mZslHeapArray[i]->fd && mZslHeapArray[i]->fd == fd) {
            id = i;
            break;
        }
    }

exit:
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
    if (frame->buf_id != 0xFFFFFFFF) {
        memset(&zsl_buffer_q, 0, sizeof(zsl_buffer_q));
        zsl_buffer_q.frame = *frame;
        zsl_buffer_q.heap_array = mZslHeapArray[frame->buf_id];
        pushZSLQueue(&zsl_buffer_q);
    }
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

    frame->buf_id = getRawBufferIDForFd(frame->fd);
    if (frame->buf_id != 0xFFFFFFFF) {
        memset(&node, 0, sizeof(struct rawBufferQueue));
        node.frame = *frame;
        node.heap_array = mZslHeapArray[frame->buf_id];
        pushRawQueue(&node);
    } else {
        HAL_LOGE("mZslHeapArray id not found.");
    }

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

// this is for real zsl flash capture, like sharkls/sharklt8, not
// sharkl2-like
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

    int ret;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    struct camera_frame_type zsl_frame;
    struct image_sw_algorithm_buf src_sw_algorithm_buf;
    struct image_sw_algorithm_buf dst_sw_algorithm_buf;
    cmr_u32 value = 0;
    uint32_t cnt = 0;
    int64_t diff_ms = 0;
    uint32_t sw_algorithm_buf_cnt = 0;
    cmr_u32 buf_id = 0;
    int sleep_rtn = 0;

    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops ||
        obj->mZslShotPushFlag == 0) {
        HAL_LOGE("mCameraHandle=%p, mHalOem=%p,", mCameraHandle, mHalOem);
        HAL_LOGE("obj->mZslShotPushFlag=%d", obj->mZslShotPushFlag);
        goto exit;
    }

    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);

    bzero(&zsl_frame, sizeof(struct camera_frame_type));
    bzero(&src_sw_algorithm_buf, sizeof(struct image_sw_algorithm_buf));
    bzero(&dst_sw_algorithm_buf, sizeof(struct image_sw_algorithm_buf));

    // this is for real zsl flash capture, like sharkls/sharklt8, not
    // sharkl2-like
    // obj->skipZslFrameForFlashCapture();

    while (1) {
        // for exception exit
        if (obj->mZslCaptureExitLoop == true) {
            HAL_LOGD("zsl loop exit done.");
            break;
        }

        zsl_frame = obj->popZslFrame();
        if (zsl_frame.y_vir_addr == 0) {
            HAL_LOGD("wait for correct zsl frame");
            sleep_rtn = usleep(20 * 1000);
            if (sleep_rtn) {
                HAL_LOGE("ERROR: no pause for 1000ms");
            }
            continue;
        }

        if (mFlush) {
            HAL_LOGD("mFlush=%d", mFlush);
            goto exit;
        }

/*for sharkle auto3dnr skip frame*/
        if (mZslSnapshotTime > zsl_frame.monoboottime &&
            mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {
            diff_ms = (mZslSnapshotTime - zsl_frame.monoboottime) / 1000000;
            HAL_LOGI("diff_ms=%lld", diff_ms);
            // make single capture frame time > mZslSnapshotTime
            if (diff_ms > ZSL_SNAPSHOT_THRESHOLD_TIME) {
                HAL_LOGI("3dnr not the right frame, skip it");
               mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
                continue;
            }
        }

        // for 3dnr sw 2.0
        if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
            mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {

            buf_id = getZslBufferIDForFd(zsl_frame.fd);
            if (buf_id == 0xFFFFFFFF) {
                goto exit;
            }
            if (mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) {
                src_sw_algorithm_buf.reserved =
                    (void *)m3DNRGraphicPathArray[buf_id].bufferhandle.get();
            } else {
                src_sw_algorithm_buf.reserved =
                    (void *)mZslGraphicsHandle[buf_id].graphicBuffer_handle;
            }
            src_sw_algorithm_buf.height = zsl_frame.height;
            src_sw_algorithm_buf.width = zsl_frame.width;
            src_sw_algorithm_buf.fd = zsl_frame.fd;
            src_sw_algorithm_buf.format = zsl_frame.format;
            src_sw_algorithm_buf.y_vir_addr = zsl_frame.y_vir_addr;
            src_sw_algorithm_buf.y_phy_addr = zsl_frame.y_phy_addr;

            // dst_sw_algorithm_buf use first intput frame for now
            if (sw_algorithm_buf_cnt == 0) {
                dst_sw_algorithm_buf.height = zsl_frame.height;
                dst_sw_algorithm_buf.width = zsl_frame.width;
                dst_sw_algorithm_buf.fd = zsl_frame.fd;
                dst_sw_algorithm_buf.format = zsl_frame.format;
                dst_sw_algorithm_buf.y_vir_addr = zsl_frame.y_vir_addr;
                dst_sw_algorithm_buf.y_phy_addr = zsl_frame.y_phy_addr;
            }
            HAL_LOGD("3dnr fd=0x%x", zsl_frame.fd);
	    if(sprddefInfo->sprd_appmode_id == CAMERA_MODE_NIGHT_PHOTO) {
	        mHalOem->ops->image_sw_algorithm_processing(
                    obj->mCameraHandle, &src_sw_algorithm_buf,
                    &dst_sw_algorithm_buf, SPRD_CAM_IMAGE_SW_ALGORITHM_NIGHT,
                    CAM_IMG_FMT_YUV420_NV21);
	    } else {
                mHalOem->ops->image_sw_algorithm_processing(
                    obj->mCameraHandle, &src_sw_algorithm_buf,
                    &dst_sw_algorithm_buf, SPRD_CAM_IMAGE_SW_ALGORITHM_3DNR,
                    CAM_IMG_FMT_YUV420_NV21);
	    }

            sw_algorithm_buf_cnt++;
            if (sw_algorithm_buf_cnt >= 5) {
                ret =
                    obj->mHalOem->ops->camera_stop_capture(obj->mCameraHandle);
                if (ret) {
                    HAL_LOGE("camera_stop_capture failed");
                }
                obj->mFlagOffLineZslStart = 0;
                break;
            }
            continue;
        }

        int8_t drvSceneMode = 0;
        mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode, &drvSceneMode);
        // for zsl hdr
        if (drvSceneMode == CAMERA_SCENE_MODE_HDR) {
            if (zsl_frame.monoboottime < mZslSnapshotTime) {
                HAL_LOGD("not the right hdr frame, skip it");
                mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
                continue;
            }
            src_sw_algorithm_buf.height = zsl_frame.height;
            src_sw_algorithm_buf.width = zsl_frame.width;
            src_sw_algorithm_buf.fd = zsl_frame.fd;
            src_sw_algorithm_buf.format = zsl_frame.format;
            src_sw_algorithm_buf.y_vir_addr = zsl_frame.y_vir_addr;
            src_sw_algorithm_buf.y_phy_addr = zsl_frame.y_phy_addr;

            if (sw_algorithm_buf_cnt == 0) {
                dst_sw_algorithm_buf.height = zsl_frame.height;
                dst_sw_algorithm_buf.width = zsl_frame.width;
                dst_sw_algorithm_buf.fd = zsl_frame.fd;
                dst_sw_algorithm_buf.format = zsl_frame.format;
                dst_sw_algorithm_buf.y_vir_addr = zsl_frame.y_vir_addr;
                dst_sw_algorithm_buf.y_phy_addr = zsl_frame.y_phy_addr;
            }
            hdr_fd[sw_algorithm_buf_cnt] = zsl_frame.fd;
            sw_algorithm_buf_cnt++;
            if (mMultiCameraMode == MODE_BOKEH) {
                char prop[PROPERTY_VALUE_MAX] = {
                    0,
                };
                property_get("persist.vendor.cam.bokeh.hdr.ev", prop, "3");
                if (sw_algorithm_buf_cnt == (uint32_t)atoi(prop)) {
                    if (mIsStoppingPreview == 1) {
                        HAL_LOGD("preview is stoped");
                        goto exit;
                    }

                    receiveRawPicture(&zsl_frame);
                }
                if (sw_algorithm_buf_cnt == 3 && mCameraId == 2) {
                    mHalOem->ops->camera_ioctrl(obj->mCameraHandle,
                                                CAMERA_IOCTRL_SET_HDR_DISABLE,
                                                &value);
                    mHalOem->ops->camera_set_zsl_snapshot_buffer(
                        obj->mCameraHandle, zsl_frame.y_phy_addr,
                        zsl_frame.y_vir_addr, zsl_frame.fd);
                    break;
                }
                if (mCameraId == 2) {
                    continue;
                }
            }

            HAL_LOGD("hdr fd=0x%x", zsl_frame.fd);
            mHalOem->ops->image_sw_algorithm_processing(
                obj->mCameraHandle, &src_sw_algorithm_buf,
                &dst_sw_algorithm_buf, SPRD_CAM_IMAGE_SW_ALGORITHM_HDR,
                CAM_IMG_FMT_YUV420_NV21);

            if (sw_algorithm_buf_cnt >= 3) {
                ret =
                    obj->mHalOem->ops->camera_stop_capture(obj->mCameraHandle);
                if (ret) {
                    HAL_LOGE("camera_stop_capture failed");
                }
                obj->mFlagOffLineZslStart = 0;
                break;
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
            if (mZslSnapshotTime > zsl_frame.monoboottime ||
                ((mZslSnapshotTime < zsl_frame.monoboottime) &&
                 (((zsl_frame.monoboottime - mZslSnapshotTime) / 1000000) <
                  delay_time))) {
                mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
                continue;
            }

            HAL_LOGD("blur fd=0x%x", zsl_frame.fd);
            mHalOem->ops->camera_set_zsl_snapshot_buffer(
                obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            break;
        }

        if (mMultiCameraMode == MODE_BOKEH ||
            mMultiCameraMode == MODE_3D_CALIBRATION) {
            if (mIsMlogMode) {
                MLOG_Tag *mlogInfo;

                mSetting->getMLOGTag(&mlogInfo);
                if (mCameraId == 0) {
                    mlogInfo->cap_timestamp = zsl_frame.monoboottime / 1000000;
                } else if (mCameraId == 2) {
                    mlogInfo->cap_timestamp = zsl_frame.monoboottime / 1000000;
                }
            }
            HAL_LOGD("bokeh/calibration fd=0x%x", zsl_frame.fd);
            // for calibration/verification debug
            if (SprdCamera3Setting::mSensorFocusEnable[mCameraId])
                HAL_LOGV("diff_ms=%" PRId64,
                         (zsl_frame.monoboottime - obj->mLatestFocusDoneTime) /
                             1000000);
            mHalOem->ops->camera_set_zsl_snapshot_buffer(
                obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            break;
        }

        if (mZslSnapshotTime > zsl_frame.monoboottime) {
            diff_ms = (mZslSnapshotTime - zsl_frame.monoboottime) / 1000000;
            HAL_LOGV("diff_ms=%" PRId64, diff_ms);
            // make single capture frame time > mZslSnapshotTime
            if (sprddefInfo->capture_mode == 1 ||
                diff_ms > ZSL_SNAPSHOT_THRESHOLD_TIME) {
                HAL_LOGD("not the right frame, skip it");
                mHalOem->ops->camera_set_zsl_buffer(
                    obj->mCameraHandle, zsl_frame.y_phy_addr,
                    zsl_frame.y_vir_addr, zsl_frame.fd);
                continue;
            }
        }

        // single capture wait the caf focused frame
        if (sprddefInfo->capture_mode == 1 && obj->mLatestFocusDoneTime > 0 &&
            zsl_frame.monoboottime < obj->mLatestFocusDoneTime &&
            mFlashCaptureFlag == 0 && !sprddefInfo->sprd_is_lowev_scene) {
            HAL_LOGD("not the focused frame, skip it");
            mHalOem->ops->camera_set_zsl_buffer(
                obj->mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);
            continue;
        }

        // for debug
        // diff_ms = (zsl_frame.monoboottime - mZslSnapshotTime) / 1000000;
        // HAL_LOGD("diff_ms=%" PRId64, diff_ms);
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
    SPRD_DEF_Tag *sprddefInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    CONTROL_Tag controlInfo;
    mSetting->getCONTROLTag(&controlInfo);

    // whether FRONT_CAMERA_FLASH_TYPE is lcd
    bool isFrontLcd =
        (strcmp(mFrontFlash, "lcd") == 0) ? true : false;
    // whether FRONT_CAMERA_FLASH_TYPE is flash
    bool isFrontFlash =
        (strcmp(mFrontFlash, "flash") == 0) ? true : false;

    HAL_LOGD("E mCameraId = %d", mCameraId);
    if (NULL == obj->mCameraHandle || NULL == obj->mHalOem ||
        NULL == obj->mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        goto exit;
    }

    if (SPRD_ERROR == mCameraState.capture_state) {
        HAL_LOGE("in error status, deinit capture at first ");
        goto exit;
    }

    /*bug1180023:telegram start capture do not wait focus done*/
    if (sprddefInfo->sprd_appmode_id == -1) {
        HAL_LOGD("af mode:%d", controlInfo.af_mode);
        if ((mIsAutoFocus == true) &&
            (controlInfo.af_mode == ANDROID_CONTROL_AF_MODE_AUTO)) {
            int count = 0;
            while (1) {
                usleep(2000);
                if (count > 1000) {
                    HAL_LOGI("wait for af done timeout 2s");
                    count = 0;
                    break;
                }
                if (mIsAutoFocus == false) {
                    HAL_LOGD("wait focus done");
                    break;
                } else {
                    count++;
                    continue;
                }
            }
        }
    }

    mZslSnapshotTime = systemTime(SYSTEM_TIME_BOOTTIME);

    if (isCapturing()) {
        WaitForCaptureDone();
    }

    if (getMultiCameraMode() == MODE_MULTI_CAMERA || mCameraId == 0 ||
        isFrontLcd || isFrontFlash || mCameraId == 4) {
        obj->mHalOem->ops->camera_start_preflash(obj->mCameraHandle);
    }
    obj->mHalOem->ops->camera_snapshot_is_need_flash(
        obj->mCameraHandle, mCameraId, &mFlashCaptureFlag);

    SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_SHOT_NUM,
             mPicCaptureCnt);

    LENS_Tag lensInfo;
    mSetting->getLENSTag(&lensInfo);
    if (lensInfo.focal_length) {
        SET_PARM(obj->mHalOem, obj->mCameraHandle, CAMERA_PARAM_FOCAL_LENGTH,
                 (int32_t)(lensInfo.focal_length * 1000));
    }

    char value[PROPERTY_VALUE_MAX];
    if ((CAMERA_MODE_CONTINUE != mSprdAppmodeId) &&
        (CAMERA_MODE_FILTER != mSprdAppmodeId) && (0 == mFbOn) &&
        (0 == mMultiCameraMode || MODE_MULTI_CAMERA == mMultiCameraMode) &&
        (ANDROID_CONTROL_SCENE_MODE_HDR != controlInfo.scene_mode) &&
        (false == mRecordingMode)) {
        property_get("persist.vendor.cam.cnr.mode", value, "0");
        if (atoi(value)) {
            mCNRMode = 1;
        }
    } else {
        mCNRMode = 0;
    }
    if (getMultiCameraMode() == MODE_BLUR) {
        property_get("persist.vendor.cam.cnr.mode", value, "0");
        if (atoi(value)) {
            mCNRMode = 1;
        }
    }
    int8_t drvSceneMode;
    mSetting->androidSceneModeToDrvMode(controlInfo.scene_mode, &drvSceneMode);
    if(drvSceneMode == CAMERA_SCENE_MODE_FDR) {
        property_get("persist.vendor.cam.cnr.mode", value, "0");
        if (atoi(value)) {
            mCNRMode = 1;
            HAL_LOGD("fdr mode, set cnr mode to 1");
        }
    }
    HAL_LOGD("mCNRMode = %d", mCNRMode);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ENABLE_CNR, mCNRMode);

    if(drvSceneMode == CAMERA_SCENE_MODE_FDR) {
       property_get("persist.vendor.cam.fdr.enable", value, "0");
       if (atoi(value)) {
            mEEMode = 1;
            HAL_LOGD("ee mode, set ee mode to 1");
       }
    }
    HAL_LOGD("mEEMode = %d", mEEMode);
    SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_SPRD_ENABLE_POSTEE, mEEMode);

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
    if (sprddefInfo->sprd_ai_scene_type_current) {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXIF_MIME_TYPE,
                 SPRD_MIMETPYE_AI);
    } else {
        SET_PARM(mHalOem, mCameraHandle, CAMERA_PARAM_EXIF_MIME_TYPE,
                 MODE_SINGLE_CAMERA);
    }

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

    if (controlInfo.scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {
        mZslMaxFrameNum = 3;
    } else if ((mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_HW_CAP_SW ||
                mSprd3dnrType == CAMERA_3DNR_TYPE_PREV_SW_CAP_SW) &&
               mRecordingMode == false) {
        mZslMaxFrameNum = 5;
    } else {
        mZslMaxFrameNum = 1;
    }
    if (sprddefInfo->is_smile_capture == 1 && sprddefInfo->af_support == 1) {
        int count = 0;
        while (1) {
            // sprddefInfo = mSetting->getSPRDDEFTagPTR();
            usleep(1000);
            if (count > 2500) {
                HAL_LOGD("wait for af locked timeout 2.5s");
                count = 0;
                break;
            }
            if (sprddefInfo->af_type == CAM_AF_FOCUS_FAF) {
                HAL_LOGV("af_state=%d,af_type =%d,wait_smile_capture =%d ms",
                         controlInfo.af_state, sprddefInfo->af_type, count);
                break;
            } else {
                count++;
                continue;
            }
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
    HAL_LOGD("X");
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
        // change this thread priority
        setpriority(PRIO_PROCESS, 0, -10);
        // set_sched_policy(0, SP_FOREGROUND);
        HAL_LOGD("zsl thread msg init");
        break;
    case CMR_EVT_ZSL_MON_SNP:
        obj->mZslShotPushFlag = 1;
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

void SprdCamera3OEMIf::setSensorCloseFlag() {
    if (NULL == mCameraHandle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return;
    }

    // for performance: dont delay for dc/dv switch or front/back switch
    mHalOem->ops->camera_set_sensor_close_flag(mCameraHandle);
}

bool SprdCamera3OEMIf::isIspToolMode() { return mIsIspToolMode; }

bool SprdCamera3OEMIf::isRawCapture() { return mIsRawCapture; }

void SprdCamera3OEMIf::ispToolModeInit() {
    cmr_handle isp_handle = 0;
    mHalOem->ops->camera_get_isp_handle(mCameraHandle, &isp_handle);
    setispserver(isp_handle);
}

int32_t SprdCamera3OEMIf::setStreamOnWithZsl() {
    mStreamOnWithZsl = 1;
    return 0;
}

void SprdCamera3OEMIf::setJpegWithBigSizePreviewFlag(bool value) {
    mIsJpegWithBigSizePreview = value;
}

bool SprdCamera3OEMIf::getJpegWithBigSizePreviewFlag() {
    return mIsJpegWithBigSizePreview;
}

int32_t SprdCamera3OEMIf::getStreamOnWithZsl() { return mStreamOnWithZsl; }

void SprdCamera3OEMIf::setFlushFlag(int32_t value) { mFlush = value; }

void SprdCamera3OEMIf::setVideoAFBCFlag(cmr_u32 value) {
    mVideoAFBCFlag = value;
}

 void *SprdCamera3OEMIf::log_monitor_thread_proc(void *p_data){
      char prop[PROPERTY_VALUE_MAX];
      int val=0;
      struct timespec ts;

      HAL_LOGI("E, %d",mLogMonitor.load());
	  while(mLogMonitor.load() > 1) {
	  /*
      *bit[0]:hal,bit[1]:oem,bit[2]:isp_middleware,bit[3]:sensor
      *bit[4]:ispalg,bit[5]:arithmetic
	  */
      property_get("persist.vendor.cam.log.modulelog", prop, 0);
      val=atoi(prop);
      if(0 < val) {
         g_log_module_setting = val;
      }
      /*
      *bit[0]:AE,bit[1]:AF,bit[3]:all
      */
      property_get("persist.vendor.cam.log.functionlog", prop, 0);
      val=atoi(prop);
      if(0 < val) {
         g_log_func_setting = val;
      }
	  /*
	  *bit[3:0]:hal,bit[7:4]:oem,bit[11:8]:isp_middleware
	  *bit[15:12]:sensor,bit[19:16]:ispalg,bit[23:20]:arithmetic
	  *default value:LOG_PRI_D=4
	  */
      property_get("persist.vendor.cam.log.loglevel", prop, 0);
      val=atoi(prop);
      if(0 < val) {
         g_log_level = val;
      }
      property_get("persist.vendor.cam.log.info", prop, 0);
      val=atoi(prop);
      if(0 < val) {
         ALOGE("Module setting: %d,func setting: %d,log level val:%d",
               g_log_module_setting,g_log_func_setting,g_log_level);
      }
      property_get("persist.vendor.cam.log.testcase", prop, 0);
      val=atoi(prop);
      if(0 < val) {
      switch(val) {
      case 1:ALOGE("testcase1:module test");
             HAL_LOGE("hal module log_ret=%d", 101);
		     CMR_LOGW("oem module log_ret=%d", 102);
             ISP_LOGI("isp module log_ret=%d", 103);
             ARITH_LOGD("arithmetic module log_ret=%d", 104);
             SENSOR_LOGD("sensor module log_ret=%d", 105);

             F_HAL_LOGE(LOG_AE,"hal module f_log_ret=%d", 1001);
             F_CMR_LOGW(LOG_AF,"oem module f_log_ret=%d", 1002);
             F_ISP_LOGI(LOG_ALL,"isp module f_log_ret=%d", 1003);
             F_ARITH_LOGD(LOG_AE,"arithmetic module f_log_ret=%d", 1004);
             F_SENSOR_LOGD(LOG_AF,"sensor module f_log_ret=%d", 1005);
             break;
      case 2:ALOGE("testcase2:loglevel test");
             HAL_LOGE("hal loglevel_ret=%d", 201);
             HAL_LOGW("hal loglevel_ret=%d", 202);
             HAL_LOGI("hal loglevel_ret=%d", 203);
             HAL_LOGD("hal loglevel_ret=%d", 204);
             HAL_LOGV("hal loglevel_ret=%d", 205);

             F_HAL_LOGE(LOG_AE,"hal loglevel_ret=%d", 2001);
             F_HAL_LOGW(LOG_AE,"hal loglevel_ret=%d", 2002);
             F_HAL_LOGI(LOG_AE,"hal loglevel_ret=%d", 2003);
             F_HAL_LOGD(LOG_AE,"hal loglevel_ret=%d", 2004);
             F_HAL_LOGV(LOG_AE,"hal loglevel_ret=%d", 2005);
             break;
      case 3:ALOGE("testcase1:function test start");
             F_HAL_LOGE(LOG_AE,"hal function log_ae_ret=%d",3001);
             F_HAL_LOGE(LOG_AF,"hal function log_af_ret=%d",3002);
             F_HAL_LOGE(LOG_AWBC,"hal function log_awbc_ret=%d",3003);
             F_HAL_LOGE(LOG_ALL,"hal function log_all_ret=%d",3004);
             break;
       }
      }
// sleep(1);
   if (clock_gettime(CLOCK_REALTIME, &ts))
       continue;

   ts.tv_nsec += ms2ns(1000);
   if (ts.tv_nsec > 1000000000) {
       ts.tv_sec += 1;
       ts.tv_nsec -= 1000000000;
   }
   sem_timedwait(&mLogMonitorSem, &ts);
  }
  mLogMonitor = 0;
  HAL_LOGV("X");

  return NULL;
}

int SprdCamera3OEMIf::log_monitor_thread_init(void * p_data) {
    int ret = NO_ERROR;
    pthread_attr_t attr;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
	if(!obj) {
       return -1;
    }
    mLogMonitor++;
    if (mLogMonitor.load() > 2)
        return ret;
    mLogMonitorSem.count = 0;
	sem_init(&mLogMonitorSem, 0, 0);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&obj->mLogHandle, &attr,
                          log_monitor_thread_proc, (void *)obj);
    pthread_setname_np(obj->mLogHandle, "logstatus");
    pthread_attr_destroy(&attr);
    return ret;
}

int SprdCamera3OEMIf::log_monitor_thread_deinit(void * p_data) {
    int ret = NO_ERROR;
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;
    int i = 10;

	if (!obj) {
       return UNKNOWN_ERROR;
	}
	mLogMonitor--;
	if (mLogMonitor.load() == 1 && obj->mLogHandle) {
        sem_post(&mLogMonitorSem);
		while (i--) {
           if (mLogMonitor.load() == 0)
               break;
		   usleep(1000);
        }
        mLogMonitor = 1;
        sem_destroy(&mLogMonitorSem);
        obj->mLogHandle = 0;
    }
    return ret;
}

#ifdef CONFIG_CAMERA_EIS
void SprdCamera3OEMIf::EisPreview_init() {
    int i = 0;
    int num = 0;
    if (mMultiCameraMode == MODE_MULTI_CAMERA) {
        num = sizeof(eis_multi_init_info_tab) / sizeof(sprd_eis_multi_init_info_t);
    }else {
        num = sizeof(eis_init_info_tab) / sizeof(sprd_eis_init_info_t);
    }
    video_stab_param_default(&mPreviewParam);
    mPreviewParam.src_w = (uint16_t)mPreviewWidth;
    mPreviewParam.src_h = (uint16_t)mPreviewHeight;
    mPreviewParam.dst_w = (uint16_t)mPreviewWidth * 5 / 6;
    mPreviewParam.dst_h = (uint16_t)mPreviewHeight * 5 / 6;
    mPreviewParam.method = 0;
    mPreviewParam.camera_id = mCameraId;
    mPreviewParam.wdx = 0;
    mPreviewParam.wdy = 0;
    mPreviewParam.wdz = 0;
    // f,td,ts is different in each project.
    mPreviewParam.f = 1230.0f;
    mPreviewParam.td = 0.0001f;
    mPreviewParam.ts = 0.0001f;
    // EIS parameter depend on board version
    for (i = 0; i < num; i++) {
         if(mMultiCameraMode == MODE_MULTI_CAMERA) {
           if ((strcmp(eis_multi_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
                 0) && (mCameraId == eis_multi_init_info_tab[i].camera_id)) {
                mPreviewParam.f = eis_multi_init_info_tab[i].f;   // 1230;
                mPreviewParam.td = eis_multi_init_info_tab[i].td; // 0.004;
                mPreviewParam.ts = eis_multi_init_info_tab[i].ts; // 0.021;
           }
         }else {
           if (strcmp(eis_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
                 0) {
                mPreviewParam.f = eis_init_info_tab[i].f;   // 1230;
                mPreviewParam.td = eis_init_info_tab[i].td; // 0.004;
                mPreviewParam.ts = eis_init_info_tab[i].ts; // 0.021;
           }
		}
    }
    HAL_LOGI("mCameraId: %d, mParam f: %lf, td:%lf, ts:%lf", mCameraId, mPreviewParam.f, mPreviewParam.td,
             mPreviewParam.ts);
    video_stab_open(&mPreviewInst, &mPreviewParam);
    HAL_LOGI("mParam src_w: %d, src_h:%d, dst_w:%d, dst_h:%d",
             mPreviewParam.src_w, mPreviewParam.src_h, mPreviewParam.dst_w,
             mPreviewParam.dst_h);
}

void SprdCamera3OEMIf::EisVideo_init() {
    // mParam = {0};
    int i = 0;
    int num = 0;
    if (mMultiCameraMode == MODE_MULTI_CAMERA) {
        num = sizeof(eis_multi_init_info_tab) / sizeof(sprd_eis_multi_init_info_t);
    } else {
        num = sizeof(eis_init_info_tab) / sizeof(sprd_eis_init_info_t);
    }
    video_stab_param_default(&mVideoParam);
    mVideoParam.src_w = (uint16_t)mVideoWidth;
    mVideoParam.src_h = (uint16_t)mVideoHeight;
    mVideoParam.dst_w = (uint16_t)mVideoWidth;
    mVideoParam.dst_h = (uint16_t)mVideoHeight;
    mVideoParam.method = 1;
    mVideoParam.camera_id = mCameraId;
    mVideoParam.wdx = 0;
    mVideoParam.wdy = 0;
    mVideoParam.wdz = 0;
    // f,td,ts is different in each project.
    mVideoParam.f = 1230.0f;
    mVideoParam.td = 0.0001f;
    mVideoParam.ts = 0.0001f;
    // EIS parameter depend on board version
    for (i = 0; i < num; i++) {
         if(mMultiCameraMode == MODE_MULTI_CAMERA) {
           if ((strcmp(eis_multi_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
                 0) && (mCameraId == eis_multi_init_info_tab[i].camera_id)) {
               mVideoParam.f = eis_multi_init_info_tab[i].f;   // 1230;
               mVideoParam.td = eis_multi_init_info_tab[i].td; // 0.004;
               mVideoParam.ts = eis_multi_init_info_tab[i].ts; // 0.021;
           }
         }else {
           if (strcmp(eis_init_info_tab[i].board_name, CAMERA_EIS_BOARD_PARAM) ==
               0) {
               mVideoParam.f = eis_init_info_tab[i].f;   // 1230;
               mVideoParam.td = eis_init_info_tab[i].td; // 0.004;
               mVideoParam.ts = eis_init_info_tab[i].ts; // 0.021;
           }
         }
    }
    HAL_LOGI("mCameraId: %d, mParam f: %lf, td:%lf, ts:%lf", mCameraId, mVideoParam.f, mVideoParam.td,
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
    int ret;
    uint count = 0;
    vsGyro *gyro = NULL;
    vsOutFrame frame_out_preview;
    memset(&frame_out_preview, 0x00, sizeof(vsOutFrame));

    if (mGyromaxtimestamp) {
        HAL_LOGI("in frame timestamp: %lf, mGyromaxtimestamp %lf",
                 frame_in.timestamp, mGyromaxtimestamp);
        ret = video_stab_write_frame(mPreviewInst, &frame_in);
        if (ret) {
            HAL_LOGE("video_stab_write_frame failed");
            goto exit;
        }

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
                ret = video_stab_write_gyro(mPreviewInst, gyro, gyro_num);
                if (ret) {
                    HAL_LOGE("video_stab_write_gyro failed");
                    if (gyro) {
                        free(gyro);
                        gyro = NULL;
                    }
                    goto exit;
                }

                if (gyro) {
                    free(gyro);
                    gyro = NULL;
                }
            }

            ret_eis = video_stab_check_gyro(mPreviewInst);
            if (ret_eis == -1) {
                HAL_LOGE("video_stab_check_gyro failed");
                goto exit;
            } else if (ret_eis == 1) {
                HAL_LOGV("gyro is NOT ready,check  gyro again");
                if (mIsStoppingPreview == 1) {
                    HAL_LOGD("preview is stoped");
                    goto exit;
                }
            } else if (ret_eis == 0) {
                HAL_LOGV("gyro is ready");
                break;
            }

            if (++count >= 4 || (NO_ERROR !=
                                 mReadGyroPreviewCond.waitRelative(
                                     mReadGyroPreviewLock, 30000000))) {
                HAL_LOGW("gyro data is too slow for eis process");
                break;
            }
        } while (ret_eis == 1);

        ret_eis = video_stab_read(mPreviewInst, &frame_out_preview);
        if (ret_eis == 0) {
            HAL_LOGV("out frame_num =%d,frame timestamp %lf, frame_out %p",
                     frame_out_preview.frame_num, frame_out_preview.timestamp,
                     frame_out_preview.frame_data);
        } else if (ret_eis == -1) {
            HAL_LOGE("video_stab_read failed");
            goto exit;
        } else if (ret_eis == 1) {
            HAL_LOGV("no frame out");
        }

    } else {
        HAL_LOGD("no gyro data to process EIS");
    }

exit:
    return frame_out_preview;
}

vsOutFrame SprdCamera3OEMIf::processVideoEIS(vsInFrame frame_in) {
    ATRACE_CALL();

    int gyro_num = 0;
    int ret_eis = 1;
    int ret;
    uint count = 0;
    vsGyro *gyro = NULL;
    vsOutFrame frame_out_video;
    memset(&frame_out_video, 0x00, sizeof(vsOutFrame));

    if (mGyromaxtimestamp) {
        HAL_LOGI("in frame timestamp: %lf, mGyromaxtimestamp %lf",
                 frame_in.timestamp, mGyromaxtimestamp);
        ret = video_stab_write_frame(mVideoInst, &frame_in);
        if (ret) {
            HAL_LOGE("video_stab_write_frame failed");
            goto exit;
        }

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
                ret = video_stab_write_gyro(mVideoInst, gyro, gyro_num);
                if (ret) {
                    HAL_LOGE("video_stab_write_gyro failed");
                    if (gyro) {
                        free(gyro);
                        gyro = NULL;
                    }
                    goto exit;
                }

                if (gyro) {
                    free(gyro);
                    gyro = NULL;
                }
            }

            ret_eis = video_stab_check_gyro(mVideoInst);
            if (ret_eis == -1) {
                HAL_LOGE("video_stab_check_gyro failed");
                goto exit;
            } else if (ret_eis == 1) {
                HAL_LOGD("gyro is NOT ready,check  gyro again");
                if (mIsStoppingPreview == 1) {
                    HAL_LOGD("preview is stoped");
                    goto exit;
                }
            } else if (ret_eis == 0) {
                HAL_LOGD("gyro is ready");
                break;
            }

            if (++count >= 4 || (NO_ERROR !=
                                 mReadGyroVideoCond.waitRelative(
                                     mReadGyroVideoLock, 30000000))) {
                HAL_LOGW("gyro data is too slow for eis process");
                break;
            }
        } while (ret_eis == 1);

        ret_eis = video_stab_read(mVideoInst, &frame_out_video);
        if (ret_eis == 0) {
            HAL_LOGV("out frame_num =%d,frame timestamp %lf, frame_out %p",
                     frame_out_video.frame_num, frame_out_video.timestamp,
                     frame_out_video.frame_data);
        } else if (ret_eis == -1) {
            HAL_LOGE("video_stab_read failed");
            goto exit;
        } else if (ret_eis == 1) {
            HAL_LOGD("no frame out");
        }
    } else {
        HAL_LOGD("no gyro data to process EIS");
    }

exit:
    return frame_out_video;
}

void SprdCamera3OEMIf::pushEISPreviewQueue(vsGyro *mGyrodata) {
    Mutex::Autolock l(&mEisPreviewLock);

    mGyroPreviewInfo.push_back(mGyrodata);
    /* Clear any pending gyro information from previous session. Also gyro data
     * size is only kGyrocount so size of list shouldn't go beyond this.
     */
    if (SprdCamera3OEMIf::kGyrocount <= mGyroPreviewInfo.size() ||
        !mEisPreviewInit)
        mGyroPreviewInfo.clear();
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

    /* Clear any pending gyro information from previous session. Also gyro data
     * size is only kGyrocount so size of list shouldn't go beyond this.
     */
    if (SprdCamera3OEMIf::kGyrocount <= mGyroVideoInfo.size() || !mEisVideoInit)
        mGyroVideoInfo.clear();
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
        frame_in.timestamp = (double)boot_time / 1000000000;
        frame_in.ae_time = (double)ae_time / 1000000000;
        frame_in.zoom = (double)zoom_ratio;
        frame_out = processPreviewEIS(frame_in);
        HAL_LOGD("transfer_matrix wrap %lf, %lf, %lf, %lf, %lf, %lf, %lf, "
                 "%lf, %lf",
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
        frame_in.timestamp = (double)boot_time / 1000000000;
        frame_in.ae_time = (double)ae_time / 1000000000;
        frame_in.zoom = (double)zoom_ratio;
        frame_in.frame_num = frame_num;
        HAL_LOGV("video frame in vir address=0x%lx,frame_num=%d",frame_in.frame_data,
                 frame_in.frame_num);
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
            SPRD_DEF_Tag *sprddefInfo;
            sprddefInfo = obj->mSetting->getSPRDDEFTagPTR();
            if (obj->mRecordingMode && sprddefInfo->sprd_eis_enabled) {
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

    HAL_LOGV("E");
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
            HAL_LOGE("Unable to register sensorUnable to register sensor "
                     "%d with rate %d and report latency %d" PRId64 "",
                     ASENSOR_TYPE_ACCELEROMETER, GsensorRate, 0);
            goto exit;
        } else {
            Gsensor_flag = 1;
        }
    }
    if (gyroSensor != NULL) {
        if (ASensorEventQueue_registerSensor(sensorEventQueue, gyroSensor,
                                             GyroRate, 0) < 0) {
            HAL_LOGE("Unable to register sensorUnable to register sensor "
                     "%d with rate %d and report latency %d" PRId64 "",
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
    HAL_LOGV("X");
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

    HAL_LOGV("E");
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
    HAL_LOGV("X");
    return NULL;
}
#endif
void *SprdCamera3OEMIf::gyro_monitor_thread_proc(void *p_data) {
    SprdCamera3OEMIf *obj = (SprdCamera3OEMIf *)p_data;

    HAL_LOGV("E");
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
    HAL_LOGV("X");
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
            /*when gyro thread proc time is long when camera close, we
             * should wait for thread end at last 1000ms*/
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
        {
            Mutex::Autolock l(&obj->mEisPreviewLock);
            while (!obj->mGyroPreviewInfo.empty())
                obj->mGyroPreviewInfo.erase(obj->mGyroPreviewInfo.begin());
            while (!obj->mGyroVideoInfo.empty())
                obj->mGyroVideoInfo.erase(obj->mGyroVideoInfo.begin());
        }
#endif
    }
    HAL_LOGD("X inited=%d, Deinit = %d", obj->mGyroInit, obj->mGyroExit);

    return ret;
}

void SprdCamera3OEMIf::setOriginalPictureSize( int32_t width ,int32_t  height) {
   if (width == 0 || height == 0) {
         CMR_LOGI("original picture width and height must not be 0");
    }
    mHalOem->ops->camera_set_original_picture_size(mCameraHandle ,width,height);
}

#endif
} // namespace sprdcamera
