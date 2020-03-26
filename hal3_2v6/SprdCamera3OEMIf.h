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
#ifndef _SPRDCAMERA3_OEMIF_H
#define _SPRDCAMERA3_OEMIF_H

#include "MemIon.h"
#include <pthread.h>
#include <semaphore.h>
extern "C" {
//    #include <linux/android_pmem.h>
}
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/threads.h>
#ifndef MINICAMERA
#include <binder/MemoryBase.h>
#endif
#include "cmr_common.h"
#include "cmr_oem.h"
#include "sprd_dma_copy_k.h"
#include <CameraParameters.h>
#include <binder/BinderService.h>
#include <binder/IInterface.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>

#include "SprdCamera3Channel.h"
#include "SprdCamera3Setting.h"
#include "SprdCamera3Stream.h"

#include "hal_common/camera_power_perf/SprdCameraPowerPerformance.h"
#include <hardware/power.h>
#ifdef CONFIG_CAMERA_GYRO
#include <android/sensor.h>
#include <sensor/Sensor.h>
#include <utils/Errors.h>
#include <utils/Looper.h>
#include <utils/RefBase.h>
#endif
#ifdef CONFIG_CAMERA_EIS
#include "sprd_eis.h"
#endif
#ifdef CONFIG_FACE_BEAUTY
#include "sprd_facebeauty_adapter.h"
#endif

#include <cutils/sockets.h>
#include <sys/socket.h>
#include <ui/GraphicBuffer.h>

using namespace android;

namespace sprdcamera {

#ifndef MINICAMERA
typedef void (*preview_callback)(sp<MemoryBase>, void *);
typedef void (*recording_callback)(sp<MemoryBase>, void *);
typedef void (*shutter_callback)(void *);
typedef void (*raw_callback)(sp<MemoryBase>, void *);
typedef void (*jpeg_callback)(sp<MemoryBase>, void *);
#endif
typedef void (*autofocus_callback)(bool, void *);
typedef void (*channel_callback)(int index, void *user_data);
typedef void (*timer_handle_func)(union sigval arg);

typedef enum {
    SNAPSHOT_DEFAULT_MODE = 0,
    SNAPSHOT_NO_ZSL_MODE,
    SNAPSHOT_ZSL_MODE,
    SNAPSHOT_PREVIEW_MODE,
    SNAPSHOT_ONLY_MODE,
    SNAPSHOT_VIDEO_MODE,
} snapshot_mode_type_t;

typedef enum {
    RATE_STOP = 0,
    RATE_NORMAL,
    RATE_FAST,
    RATE_VERY_FAST,
} sensor_rate_level_t;

// fd:         ion fd
// phys_addr:  offset from fd, always set 0
typedef struct sprd_camera_memory {
    MemIon *ion_heap;
    cmr_uint phys_addr;
    cmr_uint phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} sprd_camera_memory_t;

typedef struct {
    sprd_camera_memory_t *mIonHeap;
    camera_mem_cb_type mem_type;
} MemIonQueue;
// List<MemIonQueue> cam_MemIonQueue;

typedef struct sprd_3dnr_memory {
    const native_handle_t *native_handle;
    sp<GraphicBuffer> graphicBuffer;
    void *graphicBuffer_handle;
    cmr_uint buf_size;
} sprd_3dnr_memory_t;

typedef struct sprd_wide_memory {
#ifdef CONFIG_CAMERA_3DNR_CAPTURE_SW
    const native_handle_t *native_handle;
#endif
    sp<GraphicBuffer> bufferhandle;
    void *private_handle;
    cmr_uint buf_size;
} sprd_wide_memory_t;

typedef sprd_wide_memory_t sprd_graphic_memory_t;
struct mlog_infotag {
    int32_t camera_id;
    cmr_s64 prev_timestamp;
    cmr_s64 cap_timestamp;
    cmr_s32 otp_size;
    int32_t fps;
    cmr_s32 cropping_type;
    int32_t vcm_dac;
    int32_t vcm_num;
    int32_t vcm_step;
    int32_t vcm_version;
    int face_type;
    int cap_DT;
    cmr_s16 cur_index;
    cmr_s16 cur_lum;
    cmr_s16 target_lum;
    cmr_s16 cur_bv;
    cmr_s16 cur_bv_nonmatch;
    cmr_u16 cur_exp_line;
    cmr_s32 total_exp_time;
    cmr_s16 cur_again;
    cmr_u16 cur_dummy;
};

typedef struct {
    sprd_graphic_memory_t mGpuHeap;
    camera_mem_cb_type mem_type;
} MemGpuQueue;
// List<MemGupQueue> cam_MemGpuQueue;

struct ZslBufferQueue {
    camera_frame_type frame;
    sprd_camera_memory_t *heap_array;
};

struct rawBufferQueue {
    camera_frame_type frame;
    sprd_camera_memory_t *heap_array;
};

typedef struct {
    List<ZslBufferQueue> *cam1_ZSLQueue;
    List<ZslBufferQueue> *cam3_ZSLQueue;
    ZslBufferQueue match_frame1;
    ZslBufferQueue match_frame3;
} multi_camera_zsl_match_frame;

typedef struct {
    int32_t preWidth;
    int32_t preHeight;
    uint32_t *buffPhy;
    uint32_t *buffVir;
    uint32_t heapSize;
    uint32_t heapNum;
} camera_oem_buff_info_t;

#define MAX_GRAPHIC_BUF_NUM 20
#define MAX_SUB_RAWHEAP_NUM 10
#define MAX_LOOP_COLOR_COUNT 3
#define MAX_Y_UV_COUNT 2
#define ISP_TUNING_WAIT_MAX_TIME 4000

#define USE_ONE_RESERVED_BUF 1

#define SBS_RAW_DATA_WIDTH 3200
#define SBS_RAW_DATA_HEIGHT 1200

enum afTransitionCause {
    AF_MODE_CHANGE = 0,
    AF_INITIATES_NEW_SCAN,
    AF_COMPLETES_CURRENT_SCAN,
    AF_FAILS_CURRENT_SCAN,
    AF_TRIGGER_START,
    AF_SWEEP_DONE_AND_FOCUSED_LOCKED,
    AF_SWEEP_DONE_AND_NOT_FOCUSED_LOCKED,
    AF_TRIGGER_START_AND_FOCUSED_LOCKED,
    AF_TRIGGER_START_AND_NOT_FOCUSED_LOCKED,
    AF_TRIGGER_CANCEL,
    AF_MODE_MAX,
};

enum aeTransitionCause {
    AE_MODE_CHANGE = 0,
    AE_START,
    AE_STABLE,
    AE_STABLE_REQUIRE_FLASH,
    AE_LOCK_ON,
    AE_LOCK_OFF,
    AE_PRECAPTURE_START,
    AE_PRECAPTURE_CANCEL,
    AE_MODE_MAX,
};

enum awbTransitionCause {
    AWB_MODE_CHANGE = 0,
    AWB_START,
    AWB_STABLE,
    AWB_LOCK_ON,
    AWB_LOCK_OFF,
    AWB_MODE_MAX,
};

struct stateMachine {
    uint32_t state;
    uint32_t transitionCause;
    uint32_t newState;
};

class SprdCamera3OEMIf : public virtual RefBase {
  public:
    SprdCamera3OEMIf(int cameraId, SprdCamera3Setting *setting);
    virtual ~SprdCamera3OEMIf();
    inline int getCameraId() const;
    virtual void closeCamera();
    virtual int takePicture();
    virtual int cancelPicture();
    virtual status_t faceDectect(bool enable);
    virtual status_t faceDectect_enable(bool enable);
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual status_t setAePrecaptureSta(uint8_t state);
    virtual int openCamera();
    void initialize();
    void setCaptureRawMode(bool mode);
    // add for 3d capture
    void setCallBackYuvMode(bool mode);
    // add for 3d capture
    void setCaptureReprocessMode(bool mode, uint32_t width, uint32_t height);

    void antiShakeParamSetup();
    bool isFaceBeautyOn(SPRD_DEF_Tag *sprddefInfo);
    bool mManualExposureEnabled;
    char *mFrontFlash;

    enum camera_flush_mem_type_e {
        CAMERA_FLUSH_RAW_HEAP,
        CAMERA_FLUSH_RAW_HEAP_ALL,
        CAMERA_FLUSH_PREVIEW_HEAP,
        CAMERA_FLUSH_MAX
    };

    int flushIonBuffer(int buffer_fd, void *v_addr, void *p_addr, size_t size);
    int invalidateCache(int buffer_fd, void *v_addr, void *p_addr, size_t size);
    sprd_camera_memory_t *allocCameraMem(int buf_size, int num_bufs,
                                         uint32_t is_cache);
    sprd_camera_memory_t *allocReservedMem(int buf_size, int num_bufs,
                                           uint32_t is_cache);
    int start(camera_channel_type_t channel_type, uint32_t frame_number);
    int stop(camera_channel_type_t channel_type, uint32_t frame_number);
    int setCameraConvertCropRegion(void);
    int CameraConvertCropRegion(uint32_t sensorWidth, uint32_t sensorHeight,
                                struct img_rect *cropRegion);
    bool cal_spw_size(int sw_width, int sw_height, cmr_u32* out_width, cmr_u32* out_height);
    inline bool isCameraInit();
    int SetCameraParaTag(cmr_int cameraParaTag);
    int setJpegOrientation(int jpegOrientation);
    int SetJpegGpsInfo(bool is_set_gps_location);
    int setCapturePara(camera_capture_mode_t stream_type,
                       uint32_t frame_number);
    int SetChannelHandle(void *regular_chan, void *picture_chan);
    int setCamStreamInfo(struct img_size size, int format, int stream_type);
    int CameraConvertCoordinateToFramework(int32_t *rect);
    int CameraConvertCoordinateFromFramework(int32_t *rect);
    int CameraConvertRegionFromFramework(int32_t *rect);

    int queueBuffer(buffer_handle_t *buff_handle, int stream_type);
    int qFirstBuffer(int stream_type);
    int PushVideoSnapShotbuff(int32_t frame_number, camera_stream_type_t type);
    camera_status_t GetCameraStatus(camera_status_type_t state);

    int IommuIsEnabled(void);
    void setSensorCloseFlag();
    int checkIfNeedToStopOffLineZsl();
    bool isIspToolMode();
    bool isRawCapture();
    void ispToolModeInit();
    int32_t setStreamOnWithZsl();
    int32_t getStreamOnWithZsl();
    void setJpegWithBigSizePreviewFlag(bool value);
    bool getJpegWithBigSizePreviewFlag();
    void setFlushFlag(int32_t value);
    void setVideoAFBCFlag(cmr_u32 value);
    // add for 3dcapture, get zsl buffer's timestamp in zsl query
    uint64_t getZslBufferTimestamp();

    void GetFocusPoint(cmr_s32 *point_x, cmr_s32 *point_y);
    cmr_s32 ispSwCheckBuf(cmr_uint *param_ptr);
    void getRawFrame(int64_t timestamp, cmr_u8 **y_addr);
    void stopPreview();
    void startPreview();
    int getMultiCameraMode(void);
    void setMultiCallBackYuvMode(bool mode);
    void setSprdCameraLowpower(int flag);
    int setSensorStream(uint32_t on_off);
    int setCameraClearQBuff();
    void getIspDebugInfo(void **addr, int *size);
    int autoFocusToFaceFocus();
    void getDualOtpData(void **addr, int *size, int *read);
    void getOnlineBuffer(void *cali_info);
    bool isNeedAfFullscan();
    bool isVideoCopyFromPreview();
    int camera_ioctrl(int cmd, void *param1, void *param2);
    void setMimeType(int type);

  public:
    uint32_t isPreAllocCapMem();

    static int ZSLMode_monitor_thread_init(void *p_data);
    static int ZSLMode_monitor_thread_deinit(void *p_data);
    static cmr_int ZSLMode_monitor_thread_proc(struct cmr_msg *message,
                                               void *p_data);

    void setIspFlashMode(uint32_t mode);
    void matchZSLQueue(ZslBufferQueue *frame);
    void setMultiCameraMode(multiCameraMode mode);
    void setMasterId(uint8_t masterId);
#ifdef CONFIG_CAMERA_EIS
    virtual void EisPreview_init();
    virtual void EisVideo_init();
    vsOutFrame processPreviewEIS(vsInFrame frame_in);
    vsOutFrame processVideoEIS(vsInFrame frame_in);
    void pushEISPreviewQueue(vsGyro *mGyrodata);
    void popEISPreviewQueue(vsGyro *gyro, int gyro_num);
    void pushEISVideoQueue(vsGyro *mGyrodata);
    void popEISVideoQueue(vsGyro *gyro, int gyro_num);
    void EisPreviewFrameStab(struct camera_frame_type *frame);
    vsOutFrame EisVideoFrameStab(struct camera_frame_type *frame,
                                 uint32_t frame_num);
#endif

#ifdef CONFIG_CAMERA_GYRO
    static int gyro_monitor_thread_init(void *p_data);
    static int gyro_monitor_thread_deinit(void *p_data);
    static void *gyro_monitor_thread_proc(void *p_data);
    static void *gyro_ASensorManager_process(void *p_data);
    static void *gyro_SensorManager_process(void *p_data);
    static int gyro_get_data(void *p_data, ASensorEvent *buffer, int n,
                             struct cmr_af_aux_sensor_info *sensor_info);
#endif

    static void *log_monitor_thread_proc(void *p_data);
    static int log_monitor_thread_init(void *p_data);
    static int log_monitor_thread_deinit(void *p_data);

    void setCamPreformaceScene(sys_performance_camera_scene camera_scene);
    void setUltraWideMode();
    bool mSetCapRatioFlag;
    bool mVideoCopyFromPreviewFlag;
    bool mVideoProcessedWithPreview;
    cmr_uint mVideo3dnrFlag;
    void setTimeoutParams();

  private:
    inline void print_time();

    enum Sprd_Camera_Temp {
        CAMERA_NORMAL_TEMP,
        CAMERA_HIGH_TEMP,
        CAMERA_DANGER_TEMP,
    };

    void freeCameraMem(sprd_camera_memory_t *camera_memory);
    void freeAllCameraMem();
    static void camera_cb(camera_cb_type cb, const void *client_data,
                          camera_func_type func, void *parm4);
    void receiveRawPicture(struct camera_frame_type *frame);
    void receiveJpegPicture(struct camera_frame_type *frame);
    void receivePreviewFrame(struct camera_frame_type *frame);
    void receivePreviewFDFrame(struct camera_frame_type *frame);
    void receivePreviewATFrame(struct camera_frame_type *frame);
    void receiveRawFrame(struct camera_frame_type *frame);
    void receiveCameraExitError(void);
    void receiveTakePictureError(void);
    void receiveJpegPictureError(void);
    void HandleGetBufHandle(enum camera_cb_type cb, void *parm4);
    void HandleReleaseBufHandle(enum camera_cb_type cb, void *parm4);
    void HandleCachedBuf(enum camera_cb_type cb, void *parm4);
    bool returnYuvCallbackFrame(struct camera_frame_type *frame);
    void HandleStopCamera(enum camera_cb_type cb, void *parm4);
    void HandleStartCamera(enum camera_cb_type cb, void *parm4);
    void HandleStartPreview(enum camera_cb_type cb, void *parm4);
    void HandleStopPreview(enum camera_cb_type cb, void *parm4);
    void HandleTakePicture(enum camera_cb_type cb, void *parm4);
    void HandleEncode(enum camera_cb_type cb, void *parm4);
    void HandleFocus(enum camera_cb_type cb, void *parm4);
    void HandleAutoExposure(enum camera_cb_type cb, void *parm4);
    void HandleCancelPicture(enum camera_cb_type cb, void *parm4);
    void calculateTimestampForSlowmotion(int64_t frm_timestamp);
    void doFaceMakeup(struct camera_frame_type *frame);
    int getCameraTemp();
    void adjustFpsByTemp();
    int gatherInfoForMlog();
    int getInfoForMlog(const char *file_name, struct mlog_infotag *mlog_info);
    int getCalibrationInfo(struct mlog_infotag *mlog_info);
    int getAeInfo(struct mlog_infotag *mlog_info);
    int saveMlogInfo();

    enum Sprd_camera_state {
        SPRD_INIT,
        SPRD_IDLE,
        SPRD_ERROR,
        SPRD_PREVIEW_IN_PROGRESS,
        SPRD_FOCUS_IN_PROGRESS,
        SPRD_SET_PARAMS_IN_PROGRESS,
        SPRD_WAITING_RAW,
        SPRD_WAITING_JPEG,
        SPRD_FLASH_IN_PROGRESS,

        // internal states
        SPRD_INTERNAL_PREVIEW_STOPPING,
        SPRD_INTERNAL_CAPTURE_STOPPING,
        SPRD_INTERNAL_PREVIEW_REQUESTED,
        SPRD_INTERNAL_RAW_REQUESTED,
        SPRD_INTERNAL_STOPPING,

    };

    enum state_owner {
        STATE_CAMERA,
        STATE_PREVIEW,
        STATE_CAPTURE,
        STATE_FOCUS,
        STATE_SET_PARAMS,
    };

    typedef struct _camera_state {
        Sprd_camera_state camera_state;
        Sprd_camera_state preview_state;
        Sprd_camera_state capture_state;
        Sprd_camera_state focus_state;
        Sprd_camera_state setParam_state;
    } camera_state;

    typedef struct _slow_motion_para {
        int64_t rec_timestamp;
        int64_t last_frm_timestamp;
    } slow_motion_para;

    const char *getCameraStateStr(Sprd_camera_state s);
    Sprd_camera_state transitionState(Sprd_camera_state from,
                                      Sprd_camera_state to, state_owner owner,
                                      bool lock = true);
    void setCameraState(Sprd_camera_state state,
                        state_owner owner = STATE_CAMERA);
    inline Sprd_camera_state getCameraState();
    inline Sprd_camera_state getPreviewState();
    inline Sprd_camera_state getCaptureState();
    inline Sprd_camera_state getFocusState();
    inline Sprd_camera_state getSetParamsState();
    inline bool isCameraError();
    inline bool isCameraIdle();
    inline bool isPreviewing();
    inline bool isPreviewStart();
    inline bool isPreviewIdle();
    inline bool isCapturing();
    bool WaitForPreviewStart();
    int waitForPipelineStart();
    bool WaitForCaptureDone();
    bool WaitForCameraStop();
    bool WaitForCaptureJpegState();
    bool WaitForFocusCancelDone();
    void getPictureFormat(int *format);
    takepicture_mode getCaptureMode();
    int startPreviewInternal();
    void stopPreviewInternal();
    int cancelPictureInternal();
    bool initPreview();
    void deinitPreview();
    void deinitCapture(bool isPreAllocCapMem);
    int setPreviewParams();
    void setPreviewFps(bool isRecordMode);
    int setSnapshotParams();
    bool returnPreviewFrame(struct camera_frame_type *frame);
    bool isJpegWithPreview();
    bool isJpegWithYuvCallback();
    bool iSZslMode();
    bool checkPreviewStateForCapture();
    bool getLcdSize(uint32_t *width, uint32_t *height);
    bool switchBufferMode(uint32_t src, uint32_t dst);
    void setCameraPrivateData(void);

    int setCamSecurity(multiCameraMode multiCamMode);
    int zslTakePicture();
    int reprocessInputBuffer();
    int reprocessYuvForJpeg(cmr_uint yaddr, cmr_uint yaddr_vir, cmr_uint fd);
    int VideoTakePicture();
    int chooseDefaultThumbnailSize(uint32_t *thumbWidth, uint32_t *thumbHeight);

    int timer_stop();
    int timer_set(void *obj, int32_t delay_ms, timer_handle_func handler);
    static void timer_hand(union sigval arg);
    static void timer_hand_take(union sigval arg);
    void prepareForPostProcess(void);
    void exitFromPostProcess(void);

    int freeCameraMemForGpu(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                            cmr_u32 sum);
    int allocCameraMemForGpu(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_ZslFree(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                         cmr_u32 sum);
    int Callback_ZslMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                           cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_CaptureFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                             cmr_s32 *fd, cmr_u32 sum);
    int Callback_CaptureMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd);
    int Callback_CapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                   cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd);
    int Callback_CapturePathFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                 cmr_s32 *fd, cmr_u32 sum);
    int Callback_GraphicBufferMalloc(enum camera_mem_cb_type type, cmr_u32 size,
                                     cmr_u32 sum,cmr_uint *phy_addr, cmr_uint *vir_addr,
                                     cmr_s32 *fd, void **handle, cmr_uint width, cmr_uint height);
    int Callback_GraphicBufferFree(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                                   cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum);
    int Callback_CommonFree(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                           cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum);
    int Callback_CommonMalloc(enum camera_mem_cb_type type, cmr_u32 size,
                             cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd);
    static int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data);
    static int Callback_IonMalloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                               cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd,
                               void *private_data);
    int Callback_Sw3DNRCapturePathMalloc(cmr_u32 size, cmr_u32 sum,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, void **handle,
                                         cmr_uint width, cmr_uint height);
    int Callback_Sw3DNRCapturePathFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd, cmr_u32 sum);
    static int Callback_GpuMalloc(enum camera_mem_cb_type type,
                                  cmr_u32 *size_ptr, cmr_u32 *sum_ptr,
                                  cmr_uint *phy_addr, cmr_uint *vir_addr,
                                  cmr_s32 *fd, void **handle, cmr_uint *width,
                                  cmr_uint *height, void *private_data);

    // zsl start
    int getZSLQueueFrameNum();
    ZslBufferQueue popZSLQueue(uint64_t need_timestamp);
    ZslBufferQueue popZSLQueue();
    void pushZSLQueue(ZslBufferQueue *frame);
    void releaseZSLQueue();
    void setZslBuffers();
    void snapshotZsl(void *p_data);
    void processZslSnapshot(void *p_data);
    void skipZslFrameForFlashCapture();
    uint32_t getZslBufferIDForFd(cmr_s32 fd);
    int pushZslFrame(struct camera_frame_type *frame);
    struct camera_frame_type popZslFrame();
    void setAeState(enum aeTransitionCause cause);
    void setAwbState(enum awbTransitionCause cause);

    int32_t mLastAeMode;
    int32_t mLastAwbMode;

    List<ZslBufferQueue> mZSLQueue;
    bool mSprdZslEnabled;
    int32_t mSprd3dnrType;
    uint32_t mZslChannelStatus;
    int32_t mZslShotPushFlag;
    // we want to keep mZslMaxFrameNum buffers in mZSLQueue, for snapshot use
    int32_t mZslMaxFrameNum;
    // total zsl buffer num
    int32_t mZslNum;
    Mutex mZslBufLock;
    // zsl end

    void setAfState(enum afTransitionCause cause);
    int32_t mLastAfMode;

    uint32_t getRawQueueFrameNum();
    struct rawBufferQueue popRawQueue();
    void pushRawQueue(rawBufferQueue *node);
    void releaseRawQueue();
    uint32_t getRawBufferIDForFd(cmr_s32 fd);
    int pushRawFrame(struct camera_frame_type *frame);
    struct camera_frame_type popRawFrame();
    int allocateRawBuffers();
    int freeRawBuffers();
    int queueRawBuffers();
    List<rawBufferQueue> mRawQueue;
    uint32_t mRawMaxFrameNum;
    uint32_t mRawNum;

    bool mSprdPipVivEnabled;
    bool mSprdHighIsoEnabled;

    bool mSprdFullscanEnabled;
    bool mSprdRefocusEnabled;
    // add for 3d calibration
    bool mSprd3dCalibrationEnabled;
    // add for 3d capture
    bool mSprdYuvCallBack;
    bool mSprdMultiYuvCallBack;
    bool mSprdReprocessing;
    uint64_t mNeededTimestamp;

    // add for 3dcapture, record unpoped zsl buffer
    bool mIsUnpopped;
    // add for blur2 capture
    bool mIsBlur2Zsl;
    bool mIsSlowmotion;

    // add for portraitscene prev fb
    bool mIsPortraitScene;

    void yuvNv12ConvertToYv12(struct camera_frame_type *frame, char *tmpbuf);
    int nv21Scale(const uint8_t *src_y, const uint8_t *src_vu, int src_width,
                  int src_height, uint8_t *dst_y, uint8_t *dst_vu,
                  int dst_width, int dst_height);

    static const int kPreviewBufferCount = 24;
    static const int kPreviewRotBufferCount = 24;
    static const int kVideoBufferCount = 24;
    static const int kVideoRotBufferCount = 24;
    static const int kZslBufferCount = 24;
    static const int kRawBufferCount = 24;
    static const int kZslRotBufferCount = 24;
    static const int kChannel1BufCnt = 48;
    static const int kChannel2BufCnt = 48;
    static const int kChannel3BufCnt = 48;
    static const int kChannel4BufCnt = 48;
    static const int kRefocusBufferCount = 24;
    static const int kPdafRawBufferCount = 4;
    static const int kJpegBufferCount = 1;
    static const int kRawFrameHeaderSize = 0x0;
    static const int kISPB4awbCount = 16;
    static multiCameraMode mMultiCameraMode;
    static multi_camera_zsl_match_frame *mMultiCameraMatchZsl;
    uint8_t mMasterId;
    Mutex mLock; // API lock -- all public methods
    Mutex mPreviewCbLock;
    Mutex mCaptureCbLock;
    Mutex mStateLock;
    Condition mStateWait;
    Condition mParamWait;
    Mutex mPrevBufLock;
    Mutex mCapBufLock;
    Mutex mZslLock;
    uint32_t mCapBufIsAvail;
    Mutex mRawLock;

    uint32_t m_zslValidDataWidth;
    uint32_t m_zslValidDataHeight;

    sprd_camera_memory_t *mSubRawHeapArray[MAX_SUB_RAWHEAP_NUM];
    sprd_camera_memory_t *mPathRawHeapArray[MAX_SUB_RAWHEAP_NUM];
    sprd_graphic_memory_t mGraphicBufArray[MAX_GRAPHIC_BUF_NUM];
    sprd_graphic_memory_t m3DNRGraphicPathArray[MAX_SUB_RAWHEAP_NUM];

    int mPreviewHeight;
    int mPreviewWidth;
    int mPreviewFormat;
    int mVideoWidth;
    int mVideoHeight;
    int mVideoFormat;
    int mCallbackWidth;
    int mCallbackHeight;
    int mCallbackFormat;
    int mYuv2Width;
    int mYuv2Height;
    int mYuv2Format;
    int mCaptureWidth;
    int mCaptureHeight;
    int mPictureFormat;
    int mRawWidth;
    int mRawHeight;
    int mRawFormat;

    cmr_u16 mLargestPictureWidth;
    cmr_u16 mLargestPictureHeight;

    int mPreviewStartFlag;
    uint32_t mIsDvPreview;
    uint32_t mIsStoppingPreview;

    bool mZslPreviewMode;
    bool mRecordingMode;
    bool mIsSetCaptureMode;
    nsecs_t mRecordingFirstFrameTime;
    void *mUser;
    preview_stream_ops *mPreviewWindow;
    static gralloc_module_t const *mGrallocHal;
    oem_module_t *mHalOem;
    bool mIsStoreMetaData;
    bool mIsFreqChanged;
    int32_t mCameraId;
    volatile camera_state mCameraState;
    int miSPreviewFirstFrame;
    takepicture_mode mCaptureMode;
    bool mCaptureRawMode;
    bool mFlashMask;
    bool mReleaseFLag;
    uint32_t mTimeCoeff;

    // callback thread
    cmr_handle mCameraHandle;
    bool mIsPerformanceTestable;
    bool mIsAndroidZSL;
    channel_callback mChannelCb;
    void *mUserData;
    SprdCamera3Setting *mSetting;
    hal_stream_info_t *mZslStreamInfo;
    List<hal3_trans_info_t> mCbInfoList;
    Condition mBurstCapWait;
    uint8_t BurstCapCnt;
    uint8_t mCapIntent;
    bool jpeg_gps_location;
    uint8_t mGps_processing_method[36];

    void *mRegularChan;
    void *mPictureChan;

    camera_preview_mode_t mParaDCDVMode;
    snapshot_mode_type_t mTakePictureMode;
    int32_t mPicCaptureCnt;

    bool mZSLTakePicture;
    timer_t mPrvTimerID;

    struct cmr_zoom_param mZoomInfo;
    float mReprocessZoomRatio;
    int8_t mFlashMode;

    bool mIsAutoFocus;
    bool mIspToolStart;
    uint32_t mZslHeapNum;
    uint32_t mSubRawHeapNum;
    uint32_t mGraphicBufNum;
    uint32_t mSubRawHeapSize;
    uint32_t mPathRawHeapNum;
    uint32_t mPathRawHeapSize;
    uint32_t m3dnrGraphicPathHeapNum;
    cmr_u32 mTotalIonSize;
    cmr_u32 mTotalGpuSize;
    cmr_u32 mTotalStackSize;

    uint32_t mPreviewDcamAllocBufferCnt;
    sprd_camera_memory_t
        *mZslHeapArray[kZslBufferCount + kZslRotBufferCount + 1];
    sprd_3dnr_memory_t
        mZslGraphicsHandle[kZslBufferCount + kZslRotBufferCount + 1];
    sprd_camera_memory_t *mRawHeapArray[kRawBufferCount + 1];
    cmr_u32 hdr_fd[3];

    List<MemIonQueue> cam_MemIonQueue;
    List<MemGpuQueue> cam_MemGpuQueue;

#ifdef USE_ONE_RESERVED_BUF
    sprd_camera_memory_t *mCommonHeapReserved;
#endif

    static bool mZslCaptureExitLoop;

    uint32_t mPreviewHeapBakUseFlag;
    uint32_t mPreviewHeapArray_size[kPreviewBufferCount +
                                    kPreviewRotBufferCount + 1];
    buffer_handle_t *mPreviewBufferHandle[kPreviewBufferCount];
    buffer_handle_t *mPreviewCancelBufHandle[kPreviewBufferCount];
    bool mCancelBufferEb[kPreviewBufferCount];
    int32_t mGraphBufferCount[kPreviewBufferCount];
    uint32_t mStartFrameNum;
    uint32_t mStopFrameNum;
    slow_motion_para mSlowPara;
    bool mRestartFlag;
    bool mIsRecording;
    int32_t mVideoShotNum;
    int32_t mVideoShotFlag;
    int32_t mVideoShotPushFlag;
    Condition mVideoShotWait;

    // pre-alloc capture memory
    uint32_t mIsPreAllocCapMem;

    // ZSL Monitor Thread
    pthread_t mZSLModeMonitorMsgQueHandle;
    uint32_t mZSLModeMonitorInited;
    // enable or disable powerhint for CNR (only for capture)
    uint32_t mCNRMode;

    bool mFbOn;

    SprdCameraSystemPerformance *mSysPerformace;

    pthread_t mLogHandle;

    // for eis
    bool mGyroInit;
    bool mGyroExit;
    bool mEisPreviewInit;
    bool mEisVideoInit;
    pthread_t mGyroMsgQueHandle;
    int mGyroNum;
    double mGyromaxtimestamp;
    Mutex mReadGyroPreviewLock;
    Mutex mReadGyroVideoLock;
    Condition mReadGyroPreviewCond;
    Condition mReadGyroVideoCond;
    sem_t mGyro_sem;
    Mutex mEisPreviewLock;
    Mutex mEisVideoLock;
#ifdef CONFIG_CAMERA_EIS
    static const int kGyrocount = 100;
    List<vsGyro *> mGyroPreviewInfo;
    List<vsGyro *> mGyroVideoInfo;
    vsGyro mGyrodata[kGyrocount];
    vsParam mPreviewParam;
    vsParam mVideoParam;
    vsInst mPreviewInst;
    vsInst mVideoInst;
    Mutex mEisPreviewProcessLock;
    Mutex mEisVideoProcessLock;
#endif
    bool mSprdEisEnabled;
    // 0 - not use, default value is 0; 1 - use video buffer to jpeg enc;
    int32_t mVideoSnapshotType;

    int mIommuEnabled;
    // 0 - snapshot not need flash; 1 - snapshot need flash
    uint32_t mFlashCaptureFlag;
    uint32_t mVideoSnapshotFrameNum;
    uint32_t mFlashCaptureSkipNum;
    bool mFixedFpsEnabled;
    int32_t mSprdAppmodeId;
    int mTempStates;
    int mIsTempChanged;
    int mSprdCameraLowpower;
    uint32_t mFlagOffLineZslStart;
    int64_t mZslSnapshotTime;
    bool mIsIspToolMode;
    bool mIsUltraWideMode;
    bool mIsMlogMode;
    int64_t mtimestamplast;
    bool mIsFovFusionMode;
    bool mIsRawCapture;
#ifdef CONFIG_FACE_BEAUTY
    struct fb_beauty_param face_beauty;
    bool mflagfb;
#endif
    // for third part app face beauty in camera hal, for example, weixin
    // videocall
    int mChannel2FaceBeautyFlag;
    // top app, like wechat
    int mTopAppId;

    // grab capability
    struct cmr_path_capability grab_capability;

    uint32_t mIsCameraClearQBuf;
    int64_t mLatestFocusDoneTime;
    int32_t mStreamOnWithZsl;
    int32_t mFlush;
    Mutex mPipelineStartLock;
    Condition mPipelineStartSignal;

    uint32_t mFaceDetectStartedFlag;

    uint32_t mIsNeedFlashFired;
    uint32_t mIsPowerhintWait;
    sys_performance_camera_scene mGetLastPowerHint;
    cmr_u32 mVideoAFBCFlag;

    bool mIsJpegWithBigSizePreview;
	static std::atomic_int mLogMonitor; //0:thread exit, 1:init, 2~x:count
	static sem_t mLogMonitorSem; //logmonitor exit without wait 1s
};

}; // namespace sprdcamera

#endif //_SPRDCAMERA3_OEMIF_H
