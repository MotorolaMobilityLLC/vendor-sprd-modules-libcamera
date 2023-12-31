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
#include <string>
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

#define TXMP_STRING_TYPE std::string
#define XMP_INCLUDE_XMPFILES 1
#include <XMP.incl_cpp>
#include <XMP.hpp>

using namespace std;
namespace sprdcamera {
#define YUV_CONVERT_TO_JPEG
#ifdef CONFIG_CAMERA_MEET_JPG_ALIGNMENT
#define BOKEH_YUV_DATA_TRANSFORM
#endif

#define LOCAL_PREVIEW_NUM (20)
#define SNAP_DEPTH_NUM 1
#define SNAP_SCALE_NUM 1
#define LOCAL_CAPBUFF_NUM 4

#ifdef BOKEH_YUV_DATA_TRANSFORM
#define SNAP_TRANSF_NUM 1
#else
#define SNAP_TRANSF_NUM 0
#endif

#define LOCAL_BUFFER_NUM                                                       \
    LOCAL_PREVIEW_NUM + LOCAL_CAPBUFF_NUM + SNAP_SCALE_NUM + SNAP_TRANSF_NUM + \
        SNAP_DEPTH_NUM * 3

#define REAL_BOKEH_MAX_NUM_STREAMS 3

typedef enum {
    BOKEH_MSG_DATA_PROC = 1,
    BOKEH_MSG_COMBAIN_PROC,
    BOKEH_MSG_EXIT
} captureMsgType_bokeh;

typedef enum { CAM_TYPE_BOKEH_MAIN = 0, CAM_TYPE_DEPTH } BokehCameraDeviceType;
typedef enum { PREVIEW_MODE = 0, CAPTURE_MODE } CameraMode;
typedef enum { SPRD_API_MODE = 0 } ApiMode;
typedef enum { DEPTH_DONING = 0, DEPTH_DONE, DEPTH_INVALID } DepthStatus;
typedef enum { TRIGGER_FALSE = 0, TRIGGER_FNUM, TRIGGER_AF } DepthTrigger;
typedef enum { BUFFER_PING = 0, BUFFER_PANG } BUFFER_FLAG;
typedef enum {
    /* Main camera device id*/
    CAM_BOKEH_MAIN_ID = 0,
    /* Aux camera device id*/
    CAM_DEPTH_ID = 2
} CameraBokehID;

typedef enum { CAM_DUAL_BOKEH_MODE = 0, CAM_PORTRAIT_MODE = 1 } CameraBokehMode;

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
    buffer_handle_t *snap_gdepthJpg_buffer;
    void *snap_gdepthjpeg_buffer_addr;
    uint8_t *snap_depthConfidence_buffer;
    uint8_t *depth_normalize_data_buffer;
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

    typedef enum {
        DUMP_MULTI_NR_AFTER,
        DUMP_SINGLE_NR_BEFORE,
        DUMP_SINGLE_NR_AFTER,
        DUMP_PORTRAIT_SCENE_TYPE_MAX
    } dump_bokeh_type;
    typedef struct {
       void *buffer_addr;
       uint32_t frame_number;
       int format;
       int width;
       int height;
     } dump_image_info_t;

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    int mAfstate;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    Mutex mDefaultStreamLock;
    Mutex mFlushLock;
    camera_metadata_t *mStaticMetadata;
    new_mem_t mLocalBuffer[LOCAL_BUFFER_NUM];
    new_mem_t mLocalScaledBuffer;
    struct img_frm mScaleInfo;
    Mutex mRequestLock;
    List<new_mem_t *> mLocalBufferList;
    List<camera3_notify_msg_t> mNotifyListMain;
    List<camera3_notify_msg_t> mPrevFrameNotifyList;
    Mutex mNotifyLockMain;
    Mutex mPrevFrameNotifyLock;
    uint64_t capture_result_timestamp;
    List<camera3_notify_msg_t> mNotifyListAux;
    List<faceaf_frame_buffer_info_t> mFaceafList;
    Mutex mNotifyLockAux;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListMain;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux;
    bool mIsCapturing;
    bool mSnapshotResultReturn;
    bool mIsCapDepthFinish;
    bool mHdrSkipBlur;
    int mjpegSize;
    uint8_t mCameraId;
    int mFrontCamera;
#ifdef CONFIG_SPRD_FB_VDSP_SUPPORT
    faceBeautyLevels mPerfectskinlevel;
#else
    face_beauty_levels mPerfectskinlevel;
#endif
    bool mFlushing;
    bool mIsSupportPBokeh;
    long mXmpSize;
    int mApiVersion;
    int mJpegOrientation;
    uint8_t mBokehMode;
    int mDoPortrait;
    int mlimited_infi;
    int mlimited_macro;
#ifdef YUV_CONVERT_TO_JPEG
    buffer_handle_t *m_pDstJpegBuffer;
    buffer_handle_t *m_pDstGDepthOriJpegBuffer;
    cmr_uint mOrigJpegSize;
    cmr_uint mGDepthOriJpegSize;
#else
    buffer_handle_t *m_pMainSnapBuffer;
#endif
    uint8_t mNrCallbackCnt;

    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(int id, struct camera_info *info);
    void getDepthImageSize(int inputWidth, int inputHeight, int *outWidth,
                           int *outHeight, int type);
    void freeLocalBuffer();
    void saveRequest(camera3_capture_request_t *request);

    int allocateBuff();
    int thumbYuvProc(buffer_handle_t *src_buffer);

    XMP_StringPtr gCameraURI = "http://ns.google.com/photos/1.0/camera/";
    XMP_StringPtr gCameraPrefix = "GCamera";
    XMP_StringPtr gDepthURI = "http://ns.google.com/photos/1.0/depthmap/";
    XMP_StringPtr gDepthPrefix = "GDepth";
    XMP_StringPtr gImageURI = "http://ns.google.com/photos/1.0/image/";
    XMP_StringPtr gImagePrefix = "GImage";

    pthread_t mJpegCallbackThread;
    camera3_stream_buffer_t *mJpegOutputBuffers;
    Mutex mJpegCallbackLock;

    int insertGDepthMetadata(unsigned char *result_buffer_addr,
                             uint32_t result_buffer_size, uint32_t jpeg_size);

    void encodeOriginalJPEGandDepth(string *encodeToBase64String,
                                    string *encodeToBase64StringOrigJpeg);

    static int jpeg_callback_thread_init(void *p_data);
    static void *jpeg_callback_thread_proc(void *p_data);

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
        gdepth_outparam mGDepthOutputParam;
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
        virtual void requestInit();
        int sprdBokehPreviewHandle(buffer_handle_t *output_buf,
                                   buffer_handle_t *input_buf1);
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
    int far;
    int near;
    int mGdepthSize;
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
    int mVcmStepsFixed;
    uint64_t mCapTimestamp;
    IBokehAlgo *mBokehAlgo;
    bool mIsHdrMode;
    bool mIsNrMode;
    uint8_t sprd_3dnr_enabled;
    bool sn_trim_flag;
    int trim_W;
    int trim_H;
    bool mChangeSensor;
    int mCameraIdMaster;
    int mCameraIdSlave;
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
    void CallBackSnapResult(int status);
    void initDepthApiParams();
    void initBokehPrevApiParams();
    void dumpCaptureBokeh(unsigned char *result_buffer_addr,
                          uint32_t jpeg_size);
    void bokehFaceMakeup(buffer_handle_t *buffer_handle, void *input_buf1_addr);
    void updateApiParams(CameraMetadata metaSettings, int type, uint32_t cur_frame_number);
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
    void setDepthStatus(DepthStatus status);
    void setDepthTrigger(int vcm);
    void intDepthPrevBufferFlag();
    int getPrevDepthBuffer(BUFFER_FLAG need_flag);
    void setPrevDepthBufferFlag(BUFFER_FLAG cur_flag, int index);
    unsigned char *getaddr(unsigned char *buffer_addr, uint32_t buffer_size);
    /*int ProcessAlgo(buffer_handle_t *buffer,sprd_cam_image_sw_algorithm_type_t sw_algorithm_type);
    int mapMemInfo(buffer_handle_t *buffer_handle,
                                    struct camera_frame_type *zsl_frame);*/
    void dumpImg(
    dump_bokeh_type type, dump_image_info_t *img_info);
};
};

#endif /* SPRDCAMERAMU*/
