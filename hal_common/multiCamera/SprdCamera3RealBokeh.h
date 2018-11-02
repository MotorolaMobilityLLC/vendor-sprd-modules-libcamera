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
#ifndef SPRDCAMERAREEALBOKEH_H_HEADER
#define SPRDCAMERAREEALBOKEH_H_HEADER

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
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"
#include <ui/GraphicBufferAllocator.h>
#include "SprdCamera3MultiBase.h"
#include "SprdCamera3FaceBeautyBase.h"

#include "IBokehAlgo.h"
#include "SprdBokehAlgo.h"
#include "ArcSoftBokehAlgo.h"

namespace sprdcamera {
#define BOKEH_YUV_DATA_TRANSFORM
#define YUV_CONVERT_TO_JPEG

#define LOCAL_PREVIEW_NUM (20)
#define SNAP_DEPTH_NUM 1
#define SNAP_SCALE_NUM 1
#define LOCAL_CAPBUFF_NUM 3

#ifdef BOKEH_YUV_DATA_TRANSFORM
#define SNAP_TRANSF_NUM 1
#else
#define SNAP_TRANSF_NUM 0
#endif

#define LOCAL_BUFFER_NUM                                                       \
    LOCAL_PREVIEW_NUM + LOCAL_CAPBUFF_NUM + SNAP_SCALE_NUM + SNAP_TRANSF_NUM + \
        SNAP_DEPTH_NUM

#define REAL_BOKEH_MAX_NUM_STREAMS 3

typedef enum {
    BOKEH_MSG_DATA_PROC = 1,
    BOKEH_MSG_COMBAIN_PROC,
    BOKEH_MSG_EXIT
} captureMsgType_bokeh;

typedef enum { CAM_TYPE_BOKEH_MAIN = 0, CAM_TYPE_DEPTH } BokehCameraDeviceType;
typedef enum { PREVIEW_MODE = 0, CAPTURE_MODE } CameraMode;
typedef enum { SPRD_API_MODE = 0, ARCSOFT_API_MODE } ApiMode;
typedef enum { DEPTH_DONING = 0, DEPTH_DONE, DEPTH_INVALID } DepthStatus;
typedef enum { TRIGGER_FLASE = 0, TRIGGER_FNUM, TRIGGER_AF } DepthTrigger;
typedef enum { BUFFER_PING = 0, BUFFER_PANG } BUFFER_FLAG;
typedef enum {
    /* Main camera device id*/
    CAM_BOKEH_MAIN_ID = 0,
    /* Aux camera device id*/
    CAM_DEPTH_ID = 2
} CameraBokehID;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *buffer1;
    buffer_handle_t *buffer2;
} buffer_combination_t_bokeh;

typedef struct {
    captureMsgType_bokeh msg_type;
    buffer_combination_t_bokeh combo_buff;
} capture_queue_msg_t_bokeh;

typedef struct {
    void *buffer;
    bool w_flag;
    bool r_flag;
} PingPangBuffer;

typedef struct {
    PingPangBuffer prev_depth_buffer[2];
    void *snap_depth_buffer;
    void *depth_out_map_table;
    void *prev_depth_scale_buffer;
} DepthBuffer;

class SprdCamera3RealBokeh : SprdCamera3MultiBase, SprdCamera3FaceBeautyBase {
  public:
    static void getCameraBokeh(SprdCamera3RealBokeh **pCapture);
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

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    Mutex mDefaultStreamLock;
    camera_metadata_t *mStaticMetadata;

    new_mem_t mLocalBuffer[LOCAL_BUFFER_NUM];
    Mutex mRequestLock;
    List<new_mem_t *> mLocalBufferList;
    List<camera3_notify_msg_t> mNotifyListMain;
    Mutex mNotifyLockMain;
    List<camera3_notify_msg_t> mNotifyListAux;
    Mutex mNotifyLockAux;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListMain;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux;
    bool mIsCapturing;
    int mjpegSize;
    uint8_t mCameraId;
    face_beauty_levels mPerfectskinlevel;
    bool mFlushing;
    bool mIsSupportPBokeh;
    int mApiVersion;
    int mJpegOrientation;
#ifdef YUV_CONVERT_TO_JPEG
    buffer_handle_t *m_pDstJpegBuffer;
    cmr_uint mOrigJpegSize;
#else
    buffer_handle_t *m_pMainSnapBuffer;
#endif
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(int id, struct camera_info *info);
    void getDepthImageSize(int inputWidth, int inputHeight, int *outWidth,
                           int *outHeight, int type);
    void freeLocalBuffer();
    void saveRequest(camera3_capture_request_t *request);

    int allocateBuff();
    int thumbYuvProc(buffer_handle_t *src_buffer);

  public:
    SprdCamera3RealBokeh();
    virtual ~SprdCamera3RealBokeh();

    class BokehCaptureThread : public Thread {
      public:
        BokehCaptureThread();
        ~BokehCaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        void videoErrorCallback(uint32_t frame_number);
        int saveCaptureBokehParams(unsigned char *result_buffer_addr,
                                   uint32_t result_buffer_size,
                                   size_t jpeg_size);
        int sprdBokehCaptureHandle(buffer_handle_t *output_buf,
                                   buffer_handle_t *input_buf1,
                                   void *input_buf1_addr);
        int sprdDepthCaptureHandle(buffer_handle_t *input_buf1,
                                   void *input_buf1_addr,
                                   buffer_handle_t *input_buf2);
        int arcSoftBokehCaptureHandle(buffer_handle_t *output_bufer,
                                      buffer_handle_t *input_buf1,
                                      void *input_buf1_addr,
                                      buffer_handle_t *input_buf2);
        // This queue stores matched buffer as frame_matched_info_t
        List<capture_queue_msg_t_bokeh> mCaptureMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        const camera3_callback_ops_t *mCallbackOps;
        sprdcamera_physical_descriptor_t *mDevmain;
        buffer_handle_t *mSavedOneResultBuff;
        buffer_handle_t *mSavedResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        camera3_stream_buffer_t mSavedCapReqStreamBuff;
        camera_metadata_t *mSavedCapReqsettings;
        bool mReprocessing;
        bokeh_cap_params_t mCapbokehParam;
        bool mAbokehGallery;
        bool mBokehResult;
        void reprocessReq(buffer_handle_t *output_buffer,
                          capture_queue_msg_t_bokeh capture_msg);

      private:
        void waitMsgAvailable();
    };

    sp<BokehCaptureThread> mCaptureThread;
    class PreviewMuxerThread : public Thread {
      public:
        PreviewMuxerThread();
        ~PreviewMuxerThread();
        virtual bool threadLoop();
        virtual void requestExit();
        int sprdBokehPreviewHandle(buffer_handle_t *output_buf,
                                   buffer_handle_t *input_buf1);
        int arcsoftBokehPreviewHandle(buffer_handle_t *output_bufer,
                                      buffer_handle_t *input_buf1,
                                      buffer_handle_t *input_buf2);
        bool sprdDepthHandle(muxer_queue_msg_t *muxer_msg);

        List<muxer_queue_msg_t> mPreviewMuxerMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;

      private:
        Mutex mLock;
        void waitMsgAvailable();
    };

    sp<PreviewMuxerThread> mPreviewMuxerThread;

    class DepthMuxerThread : public Thread {
      public:
        DepthMuxerThread();
        ~DepthMuxerThread();
        virtual bool threadLoop();
        virtual void requestExit();
        int sprdDepthDo(buffer_handle_t *input_buf1,
                        buffer_handle_t *input_buf2);

        List<muxer_queue_msg_t> mDepthMuxerMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;

      private:
        Mutex mLock;
        void waitMsgAvailable();
    };

    sp<DepthMuxerThread> mDepthMuxerThread;
    bokeh_params mbokehParm;
    camera3_stream_t mMainStreams[REAL_BOKEH_MAX_NUM_STREAMS];
    camera3_stream_t mAuxStreams[REAL_BOKEH_MAX_NUM_STREAMS];
    int32_t mFaceInfo[4];
    BokehSize mBokehSize;
    camera_buffer_type_t mDepthPrevbufType;
    uint8_t mCaptureStreamsNum;
    uint8_t mCallbackStreamsNum;
    uint8_t mPreviewStreamsNum;
    List<multi_request_saved_t> mSavedRequestList;
    Mutex mMetatLock;
    Mutex mDepthBufferLock;
    Mutex mUnmatchedQueueLock;
    List<meta_save_t> mMetadataList;
    camera3_stream_t *mSavedCapStreams;
    uint32_t mCapFrameNumber;
    uint32_t mPrevFrameNumber;
    uint32_t mPrevBlurFrameNumber;
    int mLocalBufferNumber;
    const camera3_callback_ops_t *mCallbackOps;
    int mMaxPendingCount;
    int mPendingRequest;
    Mutex mPendingLock;
    Condition mRequestSignal;
    bool mhasCallbackStream;
    multi_request_saved_t mThumbReq;
    DepthStatus mDepthStatus;
    Mutex mDepthStatusLock;
    Mutex mClearBufferLock;
    DepthTrigger mDepthTrigger;
    uint8_t mCurAFStatus;
    uint8_t mCurAFMode;
    DepthBuffer mDepthBuffer;
    OtpData mOtpData;
    uint64_t mReqTimestamp;
    int mLastOnlieVcm;
    int mVcmSteps;
    uint64_t mCapTimestamp;
    IBokehAlgo *mBokehAlgo;
    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(const camera3_capture_result_t *result);
    void notifyAux(const camera3_notify_msg_t *msg);
    void processCaptureResultAux(const camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void CallBackResult(uint32_t frame_number,
                        camera3_buffer_status_t buffer_status);
    void CallBackMetadata();
    void CallBackSnapResult();
    void initDepthApiParams();
    void initBokehPrevApiParams();
    void dumpCaptureBokeh(unsigned char *result_buffer_addr,
                          uint32_t jpeg_size);
    void bokehFaceMakeup(buffer_handle_t *buffer_handle, void *input_buf1_addr);
    void updateApiParams(CameraMetadata metaSettings, int type);
    int bokehHandle(buffer_handle_t *output_buf, buffer_handle_t *inputbuff1,
                    buffer_handle_t *inputbuff2, CameraMode camera_mode);
    void _dump(const struct camera3_device *device, int fd);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
    void clearFrameNeverMatched(uint32_t main_frame_number,
                                uint32_t sub_frame_number);
    void preClose();
#ifdef YUV_CONVERT_TO_JPEG
    cmr_uint yuvToJpeg(struct private_handle_t *input_handle);
#endif
#ifdef CONFIG_ALTEK_ZTE_CALI
    int createArcSoftCalibrationData(unsigned char *pBuffer, int nBufSize);
#endif
    void setDepthStatus(DepthStatus status);
    void setDepthTrigger(int vcm);
    void intDepthPrevBufferFlag();
    int getPrevDepthBuffer(BUFFER_FLAG need_flag);
    void setPrevDepthBufferFlag(BUFFER_FLAG cur_flag, int index);
};
};

#endif /* SPRDCAMERAMU*/
