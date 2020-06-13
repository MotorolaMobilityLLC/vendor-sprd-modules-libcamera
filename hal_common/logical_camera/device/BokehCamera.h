#ifndef BOKEHCAMERA_H
#define BOKEHCAMERA_H
#pragma once

#include <CameraDevice_3_5.h>
#include <ICameraBase.h>
#include <ICameraCreator.h>

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

#include "SprdCamera3RealBokeh.h"
#include "SprdCamera3Factory.h"

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

class BokehCamera : public CameraDevice_3_5, SprdCamera3MultiBase, public ICameraBase, public SprdCamera3FaceBeautyBase {
  public:
    virtual ~BokehCamera();

    static std::shared_ptr<ICameraCreator> getCreator();

    int openCamera(hw_device_t **dev) override;
    int getCameraInfo(camera_info_t *info) override;
    int  isStreamCombinationSupported(
            const camera_stream_combination_t *comb);

    int processCaptureRequest(camera3_capture_request_t *request);
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
    static int initialize(const struct camera3_device *device,
                          const camera3_callback_ops_t *ops);
    static int close(struct hw_device_t *device);

    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux;

  private:
    uint8_t mCameraId;
    camera3_device_t *mBokehDev;
    char *mConflictDevices;
    const std::vector<int> mPhysicalIds;
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
    uint8_t mHdrCallbackCnt;

    XMP_StringPtr gCameraURI = "http://ns.google.com/photos/1.0/camera/";
    XMP_StringPtr gCameraPrefix = "GCamera";
    XMP_StringPtr gDepthURI = "http://ns.google.com/photos/1.0/depthmap/";
    XMP_StringPtr gDepthPrefix = "GDepth";
    XMP_StringPtr gImageURI = "http://ns.google.com/photos/1.0/image/";
    XMP_StringPtr gImagePrefix = "GImage";

    pthread_t mJpegCallbackThread;
    camera3_stream_buffer_t *mJpegOutputBuffers;
    Mutex mJpegCallbackLock;
    int maxWidth, maxHeight;
    int maxPrevWidth = 960, maxPrevHeight = 720;

    BokehCamera(shared_ptr<Configurator> cfg, const vector<int> &physicalIds);

    int close();
    int initialize(const camera3_callback_ops_t *callback_ops) override;
    int configure_streams(camera3_stream_configuration_t *stream_list) override;
    const camera_metadata_t *
    construct_default_request_settings(int type) override;
    int process_capture_request(camera3_capture_request_t *request) override;
    void dump(int fd) override;
    int flush() override;

    int setupPhysicalCameras(std::vector<int> SensorIds);
    void setLogicalMultiTag(CameraMetadata &metadata);
    int getMaxSize(const char *resolution);
    int getStreamTypes(camera3_stream_t *new_stream);
    void getDepthImageSize(int inputWidth, int inputHeight, int *outWidth,
                           int *outHeight, int type);
    void freeLocalBuffer();
    void saveRequest(camera3_capture_request_t *request);

    int allocateBuff();
    int thumbYuvProc(buffer_handle_t *src_buffer);
    void preClose();

    int insertGDepthMetadata(unsigned char *result_buffer_addr,
                             uint32_t result_buffer_size, uint32_t jpeg_size);

    void encodeOriginalJPEGandDepth(string *encodeToBase64String,
                                    string *encodeToBase64StringOrigJpeg);

    static int jpeg_callback_thread_init(void *p_data);
    static void *jpeg_callback_thread_proc(void *p_data);

    class BokehCreator : public ICameraCreator {
        std::shared_ptr<ICameraBase>
        createInstance(std::shared_ptr<Configurator> cfg) override;
    };

    friend class BokehCreator;

  public:
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
    bool sn_trim_flag;
    int trim_W;
    int trim_H;
    int configureStreams(
    camera3_stream_configuration_t *stream_list);
    void notifyMain(const camera3_notify_msg_t *msg);
    void notifyAux(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(const camera3_capture_result_t *result);
    void processCaptureResultAux(const camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(int type);
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
    void clearFrameNeverMatched(uint32_t main_frame_number,
                                uint32_t sub_frame_number);
#ifdef YUV_CONVERT_TO_JPEG
    cmr_uint yuvToJpeg(struct private_handle_t *input_handle);
#endif
    void setDepthStatus(DepthStatus status);
    void setDepthTrigger(int vcm);
    void intDepthPrevBufferFlag();
    int getPrevDepthBuffer(BUFFER_FLAG need_flag);
    void setPrevDepthBufferFlag(BUFFER_FLAG cur_flag, int index);
    unsigned char *getaddr(unsigned char *buffer_addr, uint32_t buffer_size);
};
}
#endif
