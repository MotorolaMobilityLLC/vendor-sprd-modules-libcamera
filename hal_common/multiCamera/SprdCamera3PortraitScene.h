/* Copyright (c) 2020, The Linux Foundataion. All rights reserved.
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
#ifndef SPRDCAMERA3LEPORTRAITSCENE_H_HEADER
#define SPRDCAMERA3LEPORTRAITSCENE_H_HEADER

#include "../SprdCamera3HWI.h"
#include "../SprdCamera3Setting.h"
#include "SprdCamera3FaceBeautyBase.h"
#include "SprdCamera3MultiBase.h"
#include "SprdMultiCam3Common.h"
#include <cutils/properties.h>
#include <dlfcn.h>
#include <hardware/camera.h>
#include <hardware/camera3.h>
#include <sprd_ion.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <system/camera.h>
#include <ui/GraphicBuffer.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Log.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>

#if (defined WIN32 || defined REALVIEW)
#define JNIEXPORT
#else
#include <jni.h>
#endif
#include "sprd_portrait_scene_adapter.h"

namespace sprdcamera {
#define AI_BGIMG_ID_OFFSET (3)
#define AI_BGIMG_BUFF_NUM (5)
#define AI_BGVID_BUFF_NUM (1)
#define AI_BG_PREV_IMG_PATH "vendor/etc/aiimg/common/BG_PREV_"
#define AI_BG_VID_IMG_PATH "vendor/etc/aiimg/common/BG_VID_"
#define AI_BG_FRONT_CAP_IMG_PATH "vendor/etc/aiimg/front/BG_CAP_"
#define AI_BG_REAR_CAP_IMG_PATH "vendor/etc/aiimg/common/BG_CAP_"
#define PBRP_THREAD_TIMEOUT 50e6
#define PBRP_LOCAL_BUFF_NUM (5)
#define PBRP_PREV_TMP_BUFF_NUM (4)
#define PBRP_MAX_NUM_STREAMS (4)
#define PBRP_REFOCUS_COMMON_PARAM_NUM (11)
#define PBRP_MAX_ROI (10)
#define PBRP_CIRCLE_VALUE_MIN (20)
#define PBRP_REFOCUS_PARAM_NUM                                                 \
    (PBRP_REFOCUS_COMMON_PARAM_NUM + PBRP_MAX_ROI * 4)

typedef enum {
    CB_REGULAR = 0,
    CB_NEW_REQUEST = 1,
    CB_SAVED_REQUEST = 2,
    CB_MAX
} callbackStatus;

typedef enum {
    BG_OFF = -2,
    BG_COLOR_RETENTION = -1,
    BG_CITY = 0,
    BG_RELAX = 1,
    BG_NIGHT = 2,
    BG_BEACH = 3,
    BG_PALACE = 4,
    BG_CHANGE = 5,
    BG_MAX_CNT
} portraitSceneBgID;

typedef enum {
    PORTRAIT_SCENE_MSG_INIT = 1,
    PORTRAIT_SCENE_MSG_DATA_PROC,
    PORTRAIT_SCENE_MSG_DATA_CACHE,
    PORTRAIT_SCENE_MSG_EXIT
} portraitSceneMsgType;

typedef enum {
    DUMP_PORTRAIT_SCENE_COMBO,  // process start
    DUMP_PORTRAIT_SCENE_MASK,   // preocess end
    DUMP_PORTRAIT_SCENE_FUSE,   // fuse end
    DUMP_PORTRAIT_SCENE_RESULT, // dump image and params
    DUMP_PORTRAIT_SCENE_TYPE_MAX
} dump_portrait_scene_type;

typedef struct {
    uint32_t frame_number;
    camera_metadata_t *settings;
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t *output_buffer;
    camera3_stream_buffer_t *input_buffer;
    camera3_capture_request_t *savedCapRequest;
    // camera3_stream_t *in_stream;
    // buffer_handle_t *in_buffer;
} request_saved_msg_t;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    camera3_stream_t *stream;
    int status;
    buffer_handle_t *buffer;
} portrait_scene_buffer_combination_t;

typedef struct {
    void *buffer_addr;
    uint32_t frame_number;
    uint32_t use_size;
    uint32_t jpeg_size;
} dump_portrait_scene_t;

typedef struct {
    void *apihandle;
    portraitSceneMsgType msg_type;
    portrait_scene_buffer_combination_t combo_buff;
    uint16_t* mask;
} portrait_scene_queue_msg_t;

typedef struct{
  int64_t time;
  uint32_t idx;
}portrait_time_t;

class SprdCamera3PortraitScene : SprdCamera3MultiBase,
                                 SprdCamera3FaceBeautyBase {
  public:
    static void getCameraPortraitScene(SprdCamera3PortraitScene **pCapture);
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int camera_device_open(__unused const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
    static int initialize(const struct camera3_device *device,
                          const camera3_callback_ops_t *ops);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int configure_streams(const struct camera3_device *device,
                                 camera3_stream_configuration_t *stream_list);
    static int process_capture_request(const struct camera3_device *device,
                                       camera3_capture_request_t *request);
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void notifyMain(const struct camera3_callback_ops *ops,
                           const camera3_notify_msg_t *msg);
    static void
    process_capture_result_aux(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);

    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux;

    void updateWeightParams(CameraMetadata metaSettings, int type);
    int CallBackPrevResultInternal(portrait_scene_queue_msg_t *muxer_msg);
    int CallBackResult(portrait_scene_queue_msg_t *muxer_msg);
    void CallSnapBackResult(camera3_capture_result_t *result,
                            camera3_buffer_status_t buffer_status);
    bool checkIsVideo();
    bool Copy2Video(portrait_scene_queue_msg_t *prev_msg);
    void printUseTime(uint32_t frame_num,char* tag);
    Mutex mWeightLock;
    Mutex mTimeLock;
  private:
    /*common*/
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtCamera;
    camera_metadata_t *mStaticMetadata;
    int m_nPhyCameras;
    Mutex mLock;
    Mutex mRequestLock;
    Mutex mCapBGLock;
    Mutex mCapLock;
    Mutex mPrevBGLock;

    sprd_camera_memory_t *mPrevBgIon[2][AI_BGIMG_BUFF_NUM];
    new_mem_t mCapBgIon;
    portraitSceneBgID mBgID;
    portraitSceneBgID mCacheBgID;
    portraitSceneBgID mCapBgID;
    SprdCamera3Setting *mSetting;
    bool mFlushing;
    bool mInitThread;
    uint8_t mCameraId;
    int64_t mstartUpdate;
    int mUpdataxy;
    int mFirstUpdateFrame;
#ifdef CONFIG_FACE_BEAUTY
    faceBeautyLevels fbLevels;
    bool mFaceBeautyFlag;
    bool mFirstSprdFB;
    fb_beauty_param_t fb_prev;
    fb_beauty_param_t fb_cap;
    fbBeautyFacetT beauty_face;
    fb_beauty_image_t beauty_image;
#endif
    List<portrait_time_t> mUseTime;
    bool isWechatClient;
    int mCircleSizeScale;
    uint8_t mLastFaceNum;
    uint8_t mSkipFaceNum;
    int32_t mLastTouchX;
    int32_t mLastTouchY;
    bool mBlurBody;
    bool mUpdataTouch;
    int mFaceInfoX;
    int mFaceInfoY;
    uint32_t mOrientationAngle;
    bool mIsRunAlgo;
    camera3_stream_t mMainStreams[PBRP_MAX_NUM_STREAMS];
    /*wechat default stream*/
    List<request_saved_msg_t> mDefaultSavedReqList;
    /*preview*/
    sprd_portrait_scene_proc_t mCachePrevWeightParams;
    int mBGWidth;
    int mBGHeight;
    bool mPrevBuffReady;
    int mPreviewStreamsNum;
    List<request_saved_msg_t> mPrevSavedReqList;
    List<uint16_t*> mPrevMaskBuffList;
    sprd_camera_memory_t *mPrevMaskBuffArr[PBRP_PREV_TMP_BUFF_NUM];
    sprd_camera_memory_t *mdebugPrev;
    bool mdebugPrevSwitch;
    /*capture*/
    sprd_portrait_scene_proc_t mCacheCapWeightParams;
    int32_t mFaceInfo[4];
    int mCurrCapFrameNum;

    int mCaptureWidth;
    int mCaptureHeight;
    /*0 used to pass to driver to get first yuv,
    * 1 used to save org jpeg
    * 2 used to save mask yuv data
    * 3 used to save fuse output yuv data
    */
    new_mem_t mLocalCapBuffer[PBRP_LOCAL_BUFF_NUM];
    bool mIsCapBuffAllocated;
    // capture have two stream, mCaptureStreamsNum-1 is org capture,
    // mCaptureStreamsNum is callback
    uint8_t mCaptureStreamsNum;
    camera3_stream_t *mSavedReqStreams[PBRP_MAX_NUM_STREAMS];
    int mjpegSize;
    int mOrgJpegSize;
    int mMaskSaveSize;
    buffer_handle_t *m_pOrgJpegBuffer;
    buffer_handle_t *m_pmaskBuffer;
    List<request_saved_msg_t> mCapSavedReqList;
    /*video*/
    int64_t mSaveTime;
    bool mChangeBGMode;
    int mVideoWidth;
    int mVideoHeight;
    int mVideoStreamsNum;
    bool mIsRecordMode;
    sprd_camera_memory_t *mRecordTmpIon[2];
    List<request_saved_msg_t> mVidSavedReqList;

    int getCameraInfo(int blur_camera_id, struct camera_info *info);
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int closeCameraDevice();
    int initialize(const camera3_callback_ops_t *callback_ops);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void processCaptureResultMain(camera3_capture_result_t *result);
    void notifyMain(const camera3_notify_msg_t *msg);
    void _dump(const struct camera3_device *device, int fd);
    int _flush(const struct camera3_device *device);
    int initThread();
    int resetVariablesToDefault();
    void freeCapBuffer();
    void freePrevBuffer();
    void freePrevBgBuffer(int isHorizon);
    int allocateBuff(int w, int h);
    // int BuffClean(sprd_portrait_scene_channel_t ch);
    int loadBgImageInternal(sprd_camera_memory_t *ion, char *path);
    int loadBgImage(sprd_portrait_scene_channel_t ch, int isHorizon);
    bool search_reqlist(int frame_num, List<request_saved_msg_t> list);
    bool search_restlist(int frame_num, List<portrait_scene_queue_msg_t> list);
    void saveRequest(camera3_capture_request_t *request,
                     const camera_metadata_t *settings);
    void EnQResultMsg(camera3_capture_result_t *result);
    int setupPhysicalCameras();
    bool clearVideoRequst();
  public:
    SprdCamera3PortraitScene();
    virtual ~SprdCamera3PortraitScene();

    class CaptureThread : public Thread {
      public:
        CaptureThread();
        ~CaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        int initCapParams();
        void initCapWeightParams();
        int saveCaptureParams(buffer_handle_t *result_buff,
                               uint32_t jpeg_size);
        void dumpPbrpImg(dump_portrait_scene_type type,
                         dump_portrait_scene_t *dump_buffs);
        int capMattingHandle(buffer_handle_t *combo_buffer,
                             void *combo_buff_addr, buffer_handle_t *maskBuffer,
                             void *mask_data, uint32_t combo_frm_num);
        bool capReadHandle(portrait_scene_queue_msg_t *post_msg);
        int capFuse(void* combo_buff_addr, void* mask_data);
        // This queue stores matched buffer as frame_matched_info_t
        List<portrait_scene_queue_msg_t> mCapMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        void *mCapApihandle;
        bool mFirstCapture;
        unsigned int mWeightWidth;
        unsigned int mWeightHeight;
        bool mUpdateCaptureWeightParams;
        sprd_portrait_scene_init_t mCapInitParams;
        sprd_portrait_scene_proc_t mCapWeightParams;
        sprd_portrait_scene_adapter_mask_t mCapMaskParams;
        // TBD remove up
        buffer_handle_t *mSavedResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        camera3_stream_buffer_t mSavedCapReqStreamBuff;
        camera_metadata_t *mSavedCapReqsettings;

      private:
        void waitMsgAvailable();
    };
    sp<CaptureThread> mCapT;

    class CapturePostThread : public Thread {
      public:
        CapturePostThread();
        ~CapturePostThread();
        int initCapPostInitParams();
        virtual bool threadLoop();
        virtual void requestExit();
        virtual void requestInit();
        bool yuvReprocessCaptureRequest(buffer_handle_t *output_buffer,
                                        uint32_t combo_frm_num);

        const camera3_callback_ops_t *mCallbackOps;
        List<portrait_scene_queue_msg_t> mCapPostMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        sprd_portrait_scene_adapter_fuse_t mCapFuseParams;

      private:
        Mutex mLock;
        void waitMsgAvailable();
    };
    sp<CapturePostThread> mCapPostT;

    class PreviewThread : public Thread {
      public:
        PreviewThread();
        ~PreviewThread();
        int initPrevInitParams();
        void initPrevWeightParams();
        virtual void requestInit();
        virtual void requestExit();
        virtual bool threadLoop();
        bool prevFuseHandle(portrait_scene_queue_msg_t *post_msg);
        int prevMattingHandle(buffer_handle_t *input1, void *input1_addr,
                              buffer_handle_t *output, void *output_addr);
        bool mUpdatePreviewWeightParams;
        sprd_portrait_scene_init_t mPrevInitParams;
        sprd_portrait_scene_proc_t mPrevWeightParams;
        sprd_portrait_scene_adapter_mask_t mPrevMaskParams;
        sprd_portrait_scene_adapter_fuse_t mPrevFuseParams;
        void *mApiSegHandle;
        List<portrait_scene_queue_msg_t> mPrevMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        bool mFirstPreview;

      private:
        Mutex mLock;
        Mutex mExitLock;
        void waitMsgAvailable();
    };
    sp<PreviewThread> mPrevT;

    class PreviewPostThread : public Thread {
      public:
        PreviewPostThread();
        ~PreviewPostThread();
        int prevFuse(portrait_scene_queue_msg_t* muxer_msg,
                      void *buffer_addr);
        virtual bool threadLoop();
        virtual void requestExit();
        virtual void requestInit();

        const camera3_callback_ops_t *mCallbackOps;
        List<portrait_scene_queue_msg_t> mPrevPostMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        int64_t tPostStart;

      private:
        Mutex mLock;
        void waitMsgAvailable();
    };
    sp<PreviewPostThread> mPrevPostT;
};
};

#endif /* SPRDCAMERAMU*/
