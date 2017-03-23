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
#ifndef SPRDCAMERA3BLUR_H_HEADER
#define SPRDCAMERA3BLUR_H_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <hardware/camera.h>
#include <system/camera.h>
#include <sys/mman.h>
#include <sprd_ion.h>
#include <gralloc_priv.h>
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"

namespace sprdcamera {

#define BLUR_LOCAL_CAPBUFF_NUM 2
#define BLUR_MAX_NUM_STREAMS 3
#define BLUR_THREAD_TIMEOUT 50e6
#define BLUR_LIB_BOKEH_PREVIEW "libbokeh_gaussian.so"
#define BLUR_LIB_BOKEH_CAPTURE "libbokeh_gaussian_cap.so"
#define BLUR_LIB_BOKEH_NUM (2)
#define BLUR_USE_DUAL_CAMERA (1)
#define BLUR_REFOCUS_PARAM_NUM (13)
#define BLUR_CIRCLE_SIZE_SCALE (3)
#define BLUR_SMOOTH_SIZE_SCALE (8)
#define BLUR_CIRCLE_VALUE_MIN (20)
#define BLUR_ALL_CONVERED_VALURE (70)

/*
YUV_IMG_SIZE = (3264 * 2448 * 3/2)
offset+reserve = 28*105
BLUR_IMG_SIZE = BLUR_JPEG_SIZE + YUV_IMG_SIZE + offset+reserve
*/
#define BLUR_JPEG_BUFFER_SIZE                                                  \
    (4160 * 3120 * 3 / 2 + sizeof(camera3_jpeg_blob_t))
#define BLUR_IMG_BUFFER_SIZE                                                   \
    (BLUR_JPEG_BUFFER_SIZE + (4160 * 3120 * 3 / 2) + (28 * 105))
#define BLUR_MIN_JPEG_BUFFER_SIZE (256 * 1024 + sizeof(camera3_jpeg_blob));

typedef struct {
    uint32_t frame_number;
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t *input_buffer;
} request_saved_blur_t;

typedef enum {
    BLUR_MSG_DATA_PROC = 1,
    BLUR_MSG_COMBAIN_PROC,
    BLUR_MSG_EXIT
} blurMsgType;

typedef enum {
    /* Main camera device id*/
    CAM_BLUR_MAIN_ID = 0,
    /* Aux camera device id*/
    CAM_BLUR_AUX_ID = 2
} CameraBlurID;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *buffer;
} buffer_combination_blur_t;

typedef struct {
    blurMsgType msg_type;
    buffer_combination_blur_t combo_buff;
} blur_queue_msg_t;

typedef struct {
    void *handle;
    int (*iSmoothInit)(void **handle, int width, int height, float min_slope,
                       float max_slope, float Findex2Gamma_AdjustRatio);
    int (*iSmoothCapInit)(void **handle, int width, int height, float min_slope,
                          float max_slope, float Findex2Gamma_AdjustRatio,
                          int scaleSmoothWidth, int scalerSmoothHeight);
    int (*iSmoothDeinit)(void *handle);
    int (*iSmoothCreateWeightMap)(void *handle, int sel_x, int sel_y,
                                  int F_number, int CircleSize);
    int (*iSmoothBlurImage)(void *handle, unsigned char *Src_YUV,
                            unsigned char *Output_YUV);
    void *mHandle;
} BlurAPI_t;

typedef struct {
    const native_handle_t *native_handle;
    MemIon *pHeapIon;
} new_ion_mem_blur_t;

class SprdCamera3Blur {
  public:
    static void getCameraBlur(SprdCamera3Blur **pBlur);
    static int camera_device_open(__unused const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int initialize(const struct camera3_device *device,
                          const camera3_callback_ops_t *ops);
    static int configure_streams(const struct camera3_device *device,
                                 camera3_stream_configuration_t *stream_list);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *device,
                                       camera3_capture_request_t *request);
    static void notifyMain(const struct camera3_callback_ops *ops,
                           const camera3_notify_msg_t *msg);
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);
    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static void
    process_capture_result_aux(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static camera3_callback_ops callback_ops_aux;

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    camera_metadata_t *mStaticMetadata;
    int mCaptureWidth;
    int mCaptureHeight;
    bool mIommuEnabled;
    new_ion_mem_blur_t mLocalCapBuffer[BLUR_LOCAL_CAPBUFF_NUM];
    bool mFlushing;
    List<request_saved_blur_t> mSavedRequestList;
    camera3_stream_t *mSavedReqStreams[BLUR_MAX_NUM_STREAMS];
    int mPreviewStreamsNum;
    Mutex mRequestLock;
    bool mIsCapturing;
    uint32_t mFNum;
    uint32_t mCircleSize;
    uint32_t mSelCoordX;
    uint32_t mSelCoordY;
    int mjpegSize;
    bool mIsWaitSnapYuv;
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(struct camera_info *info);
    int getStreamType(camera3_stream_t *new_stream);
    void freeLocalCapBuffer();
    int allocateOne(int w, int h, uint32_t is_cache,
                    new_ion_mem_blur_t *new_mem);
    int validateCaptureRequest(camera3_capture_request_t *request);
    void saveRequest(camera3_capture_request_t *request);
    uint8_t getCoveredValue();

  public:
    SprdCamera3Blur();
    virtual ~SprdCamera3Blur();

    class CaptureThread : public Thread {
      public:
        CaptureThread();
        ~CaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        int loadBlurApi();
        void unLoadBlurApi();
        int blurHandle(struct private_handle_t *input,
                       struct private_handle_t *output);
        // This queue stores matched buffer as frame_matched_info_t
        List<blur_queue_msg_t> mCaptureMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        const camera3_callback_ops_t *mCallbackOps;
        sprdcamera_physical_descriptor_t *mDevMain;
        buffer_handle_t *mSavedResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        camera3_stream_buffer_t mSavedCapReqstreambuff;
        camera_metadata_t *mSavedCapReqsettings;
        camera3_stream_t mMainStreams[BLUR_MAX_NUM_STREAMS];
        uint8_t mCaptureStreamsNum;
        bool mReprocessing;
        BlurAPI_t *mBlurApi[BLUR_LIB_BOKEH_NUM];
        int mLastSelCoordX;
        int mLastSelCoordY;
        int mLastCircleSize;
        int mLastFNum;
        int mLastMinScope;
        int mLastMaxScope;
        int mLastAdjustRati;

      private:
        int mVFrameCount;
        int mVLastFrameCount;
        nsecs_t mVLastFpsTime;
        double mVFps;
        void dumpFps();
        void waitMsgAvailable();
    };
    sp<CaptureThread> mCaptureThread;
    Mutex mMergequeueFinishMutex;
    Condition mMergequeueFinishSignal;

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void _dump(const struct camera3_device *device, int fd);
    void dumpData(unsigned char *addr, int type, int size, int param1,
                  int param2);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
};
};

#endif /* SPRDCAMERAMU*/
