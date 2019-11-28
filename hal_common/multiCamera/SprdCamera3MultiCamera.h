/* Copyright (c) 2016, The Linux Foundataion. All rights reserved.
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

#include "SprdMultiCam3Common.h"
#include "SprdCamera3MultiBase.h"
#include "WT_interface.h"

namespace sprdcamera {

class SprdCamera3MultiCamera : public SprdCamera3MultiBase {
  public:
    static int get_camera_info(int camera_id, struct camera_info *info);

  private:
    static camera_metadata_t *mStaticCameraCharacteristics;

  public:
    SprdCamera3MultiCamera();
    virtual ~SprdCamera3MultiCamera();
    // static void getCameraMultiCamera(SprdCamera3MultiCamera **pMuxer);
    static int camera_device_open(__unused const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
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
    static void notify_Aux1(const struct camera3_callback_ops *ops,
                            const camera3_notify_msg_t *msg);
    static void notify_Aux2(const struct camera3_callback_ops *ops,
                            const camera3_notify_msg_t *msg);
    static void notify_Aux3(const struct camera3_callback_ops *ops,
                            const camera3_notify_msg_t *msg);

    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);
    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_multi[MAX_MULTI_NUM_CAMERA];
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void
    process_capture_result_aux1(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void
    process_capture_result_aux2(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void
    process_capture_result_aux3(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);

    virtual int initialize(const camera3_callback_ops_t *callback_ops);
    virtual int configureStreams(const struct camera3_device *device,
                                 camera3_stream_configuration_t *stream_list);
    virtual int processCaptureRequest(const struct camera3_device *device,
                                      camera3_capture_request_t *request);
    virtual void notifyMain(const camera3_notify_msg_t *msg);
    virtual void notifyAux1(const camera3_notify_msg_t *msg);
    virtual void notifyAux2(const camera3_notify_msg_t *msg);
    virtual void notifyAux3(const camera3_notify_msg_t *msg);
    virtual const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    virtual void _dump(const struct camera3_device *device, int fd);

    virtual int _flush(const struct camera3_device *device);
    // virtual int getCameraInfo(int id, struct camera_info *info);
    virtual void
    saveRequest(camera3_capture_request_t *request,
                camera3_stream_buffer_t fw_buffer[MAX_MULTI_NUM_STREAMS + 1],
                int index);

    virtual void freeLocalBuffer();
    virtual int allocateBuff();

    virtual void parse_configure_info(config_multi_camera *config_info);
    virtual void video_parse_configure_info(config_multi_camera *config_info);
    // provide interface for inherited class

    void reConfigOpen();

    void reReqConfig(camera3_capture_request_t *request, CameraMetadata *meta);

    config_multi_camera *load_config_file(void);
    void setAlgoTrigger(int vcm);
    void setZoomCropRegion(CameraMetadata *WideSettings,
                           CameraMetadata *TeleSettings,
                           CameraMetadata *SwSettings, float multi_zoom_ratio);
    void reConfigInit();
    void reConfigStream();
    camera_metadata_t *reConfigResultMeta(camera_metadata_t *meta);
    void processCaptureResultAux3(const camera3_capture_result_t *result);
    void processCaptureResultAux2(const camera3_capture_result_t *result);
    void processCaptureResultAux1(const camera3_capture_result_t *result);
    void processCaptureResultMain(const camera3_capture_result_t *result);
    void clearFrameNeverMatched(uint32_t main_frame_number,
                                uint32_t sub_frame_number,
                                uint32_t sub2_frame_number);
    void reConfigFlush();
    void reConfigClose();
    void TWThreadExit();
    void initAlgo();
    int runAlgo(void *handle, buffer_handle_t *input_image_wide,
                buffer_handle_t *input_image_tele,
                buffer_handle_t *output_image);
    void deinitAlgo();

  public:
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int closeCameraDevice();
    int createHalStream(camera3_stream_t *preview_stream,
                        camera3_stream_t *callback_stream,
                        camera3_stream_t *snap_stream,
                        camera3_stream_t *default_stream,
                        camera3_stream_t *output_list,
                        sprdcamera_physical_descriptor_t *camera_phy_info);

    int createOutputBufferStream(
        camera3_stream_buffer_t *outputBuffer, int req_stream,
        const sprdcamera_physical_descriptor_t *phy_cam_info,
        camera3_stream_buffer_t fw_buffer[MAX_MULTI_NUM_STREAMS + 1], int *num);

    camera3_stream_t *
    findStream(int type, const sprdcamera_physical_descriptor_t *phy_cam_info);
    hal_req_stream_config *
    findHalReq(int type, const hal_req_stream_config_total *total_config);

    void CallBackMetadata();
    void CallBackResult(uint32_t frame_number, int buffer_status,
                        int stream_type, int camera_index);

    int findMNIndex(uint32_t frame_number);
    int reprocessReq(buffer_handle_t *input_buffers);
    void sendWaitFrameSingle();
    bool isFWBuffer(buffer_handle_t *MatchBuffer);

  public:
    multiCameraMode mMultiMode;
    int mLocalBufferNumber;
    int64_t mCurFrameNum;
    int64_t mCapFrameNum;
    stream_size_t mMainSize;

    bool mFlushing;
    bool mIsSyncFirstFrame;
    int mFirstFrameCount;
    Mutex mRequestLock;
    Mutex mMultiLock;
    Mutex mMetatLock;
    Mutex mWaitFrameLock;
    Mutex mDefaultStreamLock;
    Mutex mNotifyLockMain;
    Mutex mNotifyLockAux1;
    Mutex mNotifyLockAux2;
    Mutex mNotifyLockVideoMain;
    Mutex mNotifyLockVideoAux1;
    Mutex mNotifyLockVideoAux2;
    int64_t mWaitFrameNum;
    int64_t mSendFrameNum;
    Condition mWaitFrameSignal;
    List<new_mem_t *> mLocalBufferList;
    List<multi_request_saved_t> mSavedPrevRequestList;
    List<multi_request_saved_t> mSavedCallbackRequestList;
    List<multi_request_saved_t> mSavedVideoRequestList;
    List<camera3_notify_msg_t> mNotifyListMain;
    List<camera3_notify_msg_t> mNotifyListAux1;
    List<camera3_notify_msg_t> mNotifyListAux2;
    List<camera3_notify_msg_t> mNotifyListAux3;
    List<camera3_notify_msg_t> mNotifyListVideoMain;
    List<camera3_notify_msg_t> mNotifyListVideoAux1;
    List<camera3_notify_msg_t> mNotifyListVideoAux2;
    List<camera3_notify_msg_t> mNotifyListVideoAux3;
    List<meta_save_t> mMetadataList;
    multi_request_saved_t mSavedSnapRequest;
    const camera3_callback_ops_t *mCallbackOps;
    camera_metadata_t *mSavedCapReqsettings;

    uint8_t m_nPhyCameras;
    sprd_virtual_camera_t m_VirtualCamera;
    sprdcamera_physical_descriptor_t m_pPhyCamera[MAX_MULTI_NUM_CAMERA];
    hal_buffer_info mBufferInfo[MAX_MULTI_NUM_BUFFER];
    new_mem_t mLocalBuffer[MAX_MULTI_NUM_BUFFER * (MAX_MULTI_NUM_BUFFER + 11)];
    hal_req_stream_config_total mHalReqConfigStreamInfo[MAX_MULTI_NUM_STREAMS];
    request_state mRequstState;
    // select request config number; inherited class configure
    int mReqConfigNum;
    // select metadata and notify owner
    int mMetaNotifyIndex;
    bool mIsCapturing;
    bool mIsVideoMode;

  private:
    buffer_handle_t *mCapInputbuffer;
    float mZoomValue;
    float mSwitch_W_Sw_Threshold;
    float mSwitch_W_T_Threshold;
    int32_t main_FaceRect[10 * 4];
    int32_t aux1_FaceRect[10 * 4];
    int32_t aux2_FaceRect[10 * 4];
    int32_t aux1_FaceGenderRaceAge[10];
    int32_t aux2_FaceGenderRaceAge[10];
    int aux1_face_number;
    int aux2_face_number;
    uint8_t aux1_ai_scene_type;
    uint8_t aux2_ai_scene_type;
    uint8_t aux1_af_state;
    uint8_t aux2_af_state;
    int mVcmSteps;
    int mAlgoStatus;
    Mutex mUnmatchedQueueLock;
    uint32_t mRefIdex;
    uint32_t mRefCameraId;
    uint32_t mLastRefCameraId;

    Mutex mClearBufferLock;
    cmr_u16 mTeleMaxWidth;
    cmr_u16 mTeleMaxHeight;
    cmr_u16 mWideMaxWidth;
    cmr_u16 mWideMaxHeight;
    cmr_u16 mSwMaxWidth;
    cmr_u16 mSwMaxHeight;
    void *mPrevAlgoHandle;
    void *mCapAlgoHandle;
    OtpData mOtpData;
    List<hwi_frame_buffer_info_t> mFrameListMainPreview;
    List<hwi_frame_buffer_info_t> mFrameListMainCallback;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListMain;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux1;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux2;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListVideoMain;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListVideoAux1;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListVideoAux2;

    class TWPreviewMuxerThread : public Thread {
      public:
        TWPreviewMuxerThread();
        virtual ~TWPreviewMuxerThread();
        virtual bool threadLoop();
        virtual void requestExit();

        // This queue stores matched buffer as frame_matched_info_t
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        List<muxer_queue_msg_t> mPreviewMuxerMsgList;

      private:
        void waitMsgAvailable();
    };

    sp<TWPreviewMuxerThread> mPreviewMuxerThread;

    class TWCaptureThread : public Thread {
      public:
        TWCaptureThread();
        virtual ~TWCaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        // This queue stores matched buffer as frame_matched_info_t
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        buffer_handle_t *mSavedOneResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        List<muxer_queue_msg_t> mCaptureMsgList;

      private:
        void waitMsgAvailable();
    };
    sp<TWCaptureThread> mTWCaptureThread;

    class VideoThread : public Thread {
      public:
        VideoThread();
        virtual ~VideoThread();
        virtual bool threadLoop();
        virtual void requestExit();
        // This queue stores matched buffer as frame_matched_info_t
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        List<muxer_queue_msg_t> mVideoMuxerMsgList;

      private:
        void waitMsgAvailable();
    };
    sp<VideoThread> mVideoThread;

};
};

/* SPRDCAMERA3MULTICAMERA_H_HEADER*/
