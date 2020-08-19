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

typedef enum { CAM_BOKEH_MAIN = 0, CAM_BOKEH_DEPTH } BokehCameraTypes;
typedef enum { BOKEH_PREVIEW_MODE = 0, BOKEH_CAPTURE_MODE } BokehCameraMode;
typedef enum { BOKEH_API_MODE = 0 } BokehApiMode;
typedef enum {
    BOKEH_DEPTH_DONING = 0,
    BOKEH_DEPTH_DONE,
    BOKEH_DEPTH_INVALID
} BokehDepthStatus;
typedef enum {
    BOKEH_TRIGGER_FALSE = 0,
    BOKEH_TRIGGER_FNUM,
    BOKEH_TRIGGER_AF
} BokehDepthTrigger;
typedef enum { BOKEH_BUFFER_PING = 0, BOKEH_BUFFER_PANG } BufferFlag;

typedef enum { DUAL_BOKEH_MODE = 0, PORTRAIT_MODE = 1 } BokehMode;

typedef enum { MSG_DATA_PROC = 1, MSG_COMBAIN_PROC, MSG_EXIT } captureMsgType;
typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *buffer1;
    buffer_handle_t *buffer2;
} buffer_combination_t;
typedef struct {
    captureMsgType msg_type;
    buffer_combination_t combo_buff;
} capture_queue_msg_t;
typedef struct {
    void *buffer;
    bool w_flag;
    bool r_flag;
} BokehPingPangBuffer;
typedef struct {
    BokehPingPangBuffer prev_depth_buffer[2];
    void *snap_depth_buffer;
    buffer_handle_t *snap_gdepthJpg_buffer;
    void *snap_gdepthjpeg_buffer_addr;
    uint8_t *snap_depthConfidence_buffer;
    uint8_t *depth_normalize_data_buffer;
    void *depth_out_map_table;
    void *prev_depth_scale_buffer;
} BokehDepthBuffer;

typedef struct {
    CameraMetadata metadata;
    uint32_t frame_number;
} meta_req_t;

typedef struct {
    int frame_number;
    int reqType;
} req_type;

enum intent_type { PREV_TYPE, CAP_TYPE };
class BokehCamera : public CameraDevice_3_5,
                    SprdCamera3MultiBase,
                    public SprdCamera3FaceBeautyBase {
  public:
    virtual ~BokehCamera();

    static std::shared_ptr<ICameraCreator> getCreator();

    int getCameraInfo(camera_info_t *info) override;
    int openCameraDevice() override;
    int closeCameraDevice() override;
    int isStreamCombinationSupported(
        const camera_stream_combination_t *comb) override;

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

    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux;

  private:
    int mCameraNum;
    camera3_device_t *mBokehDev;
    char **mConflictDevices;
    const std::vector<int> mPhysicalIds;
    uint8_t *physicIds;
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
    bool mSnapshotMainReturn;
    bool mSnapshotAuxReturn;
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
    int maxCapWidth, maxCapHeight;
    int maxPrevWidth, maxPrevHeight;

    BokehCamera(shared_ptr<Configurator> cfg, const vector<int> &physicalIds);

    int initialize(const camera3_callback_ops_t *callback_ops) override;
    int configure_streams(camera3_stream_configuration_t *stream_list) override;
    const camera_metadata_t *
    construct_default_request_settings(int type) override;
    int process_capture_request(camera3_capture_request_t *request) override;
    void dump(int fd) override;
    int flush() override;

    int setupPhysicalCameras(std::vector<int> SensorIds);
    void setBokehCameraTags(CameraMetadata &metadata);
    bool matchTwoFrame(hwi_frame_buffer_info_t result1,
                       List<hwi_frame_buffer_info_t> &list,
                       hwi_frame_buffer_info_t *result2);
    void getMaxSize(const char *resolution);
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
        List<capture_queue_msg_t> mCaptureMsgList;
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
        bool mAbokehGallery;
        bool mBokehResult;
        gdepth_outparam mGDepthOutputParam;
        void reprocessReq(buffer_handle_t *output_buffer,
                          capture_queue_msg_t capture_msg);

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
    bool mVideoPrevShare;
    req_type mReqCapType;
    int far;
    int near;
    uint8_t mCaptureStreamsNum;
    uint8_t mCallbackStreamsNum;
    uint8_t mPreviewStreamsNum;
    int mVideoStreamsNum;
    stream_size_t mBokehStream;
    List<multi_request_saved_t> mSavedRequestList;
    List<multi_request_saved_t> mSavedVideoReqList;
    Mutex mMetatLock;
    Mutex mMetaReqLock;
    Mutex mBokehDepthBufferLock;
    Mutex mUnmatchedQueueLock;
    List<meta_save_t> mMetadataList;
    List<meta_req_t> mMetadataReqList;
    camera3_stream_t *mSavedCapStreams;
    int mCapFrameNumber;
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
    BokehDepthStatus mDepthStatus;
    Mutex mDepthStatusLock;
    Mutex mClearBufferLock;
    BokehDepthTrigger mDepthTrigger;
    BokehDepthBuffer mBokehDepthBuffer;
    OtpData mOtpData;
    uint64_t mReqTimestamp;
    int mLastOnlieVcm;
    int mVcmSteps;
    int mVcmStepsFixed;
    IBokehAlgo *mBokehAlgo;
    bool mIsHdrMode;
    bool sn_trim_flag;
    int trim_W;
    int trim_H;
    uint8_t mExtendMode;
    int configureStreams(camera3_stream_configuration_t *stream_list);
    void notifyMain(const camera3_notify_msg_t *msg);
    void notifyAux(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(const camera3_capture_result_t *result);
    void processCaptureResultAux(const camera3_capture_result_t *result);
    const camera_metadata_t *constructDefaultRequestSettings(int type);
    void CallBackResult(uint32_t frame_number,
                        camera3_buffer_status_t buffer_status,
                        intent_type type);
    void CallBackMetadata(uint32_t frame_number);
    void CallBackSnapResult(int status);
    void callbackVideoResult(uint32_t frame_number,
                             camera3_buffer_status_t buffer_status,
                             buffer_handle_t *buffer);
    void initDepthApiParams();
    void initBokehPrevApiParams();
    void dumpCaptureBokeh(unsigned char *result_buffer_addr,
                          uint32_t jpeg_size);
    void bokehFaceMakeup(buffer_handle_t *buffer_handle, void *input_buf1_addr);
    void updateApiParams(CameraMetadata metaSettings, int type,
                         uint32_t cur_frame_number);
    void clearFrameNeverMatched(uint32_t main_frame_number,
                                uint32_t sub_frame_number);
#ifdef YUV_CONVERT_TO_JPEG
    cmr_uint yuvToJpeg(struct private_handle_t *input_handle);
#endif
    void setDepthStatus(BokehDepthStatus status);
    void setDepthTrigger(int vcm);
    void initPrevDepthBufferFlag();
    int getPrevDepthBufferFlag(BufferFlag need_flag);
    void setPrevDepthBufferFlag(BufferFlag cur_flag, int index);
    unsigned char *getaddr(unsigned char *buffer_addr, uint32_t buffer_size);
    void setResultMetadata(CameraMetadata &metadata, uint32_t frame_number);
    int validateStream(camera3_stream_t *stream);
    void setPrevBokehSupport();
};
}
#endif
