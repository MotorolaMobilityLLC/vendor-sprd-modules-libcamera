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

#include "SprdCamera3Multi.h"
#include "SprdMultiCam3Common.h"
#include "WT_interface.h"
namespace sprdcamera {
typedef enum { NO_ALGO = 0, DO_ALGO } TWAlgoStatus;
class SprdCamera3OpticsZoomV1 : public SprdCamera3Multi {
  public:
    SprdCamera3OpticsZoomV1();
    virtual ~SprdCamera3OpticsZoomV1();
    static void getCamera3dZoomV1(SprdCamera3Multi **pMuxer);

    void reReqConfig(camera3_capture_request_t *request, CameraMetadata *meta);

    config_multi_camera *load_config_file(void);
    void setAlgoTrigger(int vcm);
    float setZoomInfo(CameraMetadata *WideSettings,
                      CameraMetadata *TeleSettings);
    void reConfigGetCameraInfo(CameraMetadata &metadata);
    void reConfigInit();
    void reConfigStream();
    camera_metadata_t *reConfigResultMeta(camera_metadata_t *meta);
    void coordinateTra(int inputWidth, int inputHeight, int outputWidth,
                       int outputHeight, float inputRatio, float outputRatio,
                       int *area);
    void processCaptureResultAux1(const camera3_capture_result_t *result);
    void processCaptureResultMain(const camera3_capture_result_t *result);
    void clearFrameNeverMatched(uint32_t main_frame_number,
                                uint32_t sub_frame_number);
    void reConfigFlush();
    void reConfigClose();
    void TWThreadExit();
    void initAlgo();
    int runAlgo(void *handle, buffer_handle_t *input_image_wide,
                buffer_handle_t *input_image_tele,
                buffer_handle_t *output_image);
    void deinitAlgo();

  private:
    buffer_handle_t *mCapInputbuffer;
    float mZoomValue;
    float mZoomValueTh;
    int mVcmSteps;
    int mAlgoStatus;
    Mutex mUnmatchedQueueLock;
    Mutex mClearBufferLock;
    cmr_u16 mTeleMaxWidth;
    cmr_u16 mTeleMaxHeight;
    cmr_u16 mWideMaxWidth;
    cmr_u16 mWideMaxHeight;
    void *mPrevAlgoHandle;
    void *mCapAlgoHandle;
    OtpData mOtpData;
    List<hwi_frame_buffer_info_t> mFrameListMainPreview;
    List<hwi_frame_buffer_info_t> mFrameListMainCallback;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListMain;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux1;
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
};
};

/* SPRDCAMERA3ZOOM_H_HEADER*/
