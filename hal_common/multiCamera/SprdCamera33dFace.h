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
#ifndef SPRDCAMERA3DFACE_H_HEADER
#define SPRDCAMERA3DFACE_H_HEADER
//#include <gralloc_priv.h>

#include "SprdMultiCam3Common.h"

#include "SprdCamera3Multi.h"

namespace sprdcamera {

class SprdCamera33dFace : public SprdCamera3Multi {
  public:
    SprdCamera33dFace();
    virtual ~SprdCamera33dFace();
    static void getCamera3dFace(SprdCamera3Multi **pMuxer);

    config_multi_camera *load_config_file(void);

    void reReqConfig(camera3_capture_request_t *request, CameraMetadata *meta);
    void reConfigInit();
    void processCaptureResultMain(const camera3_capture_result_t *result);

    void processCaptureResultAux1(const camera3_capture_result_t *result);

    void processCaptureResultAux2(const camera3_capture_result_t *result);
    void processIRData(const camera3_capture_result_t *result, int type);
    void clearFrameNeverMatched(int whichCamera);
    void faceThreadExit(void);
    void reConfigGetCameraInfo(CameraMetadata &metadata);
    void reConfigFlush();
    void reConfigClose();

    class MuxerThread : public Thread {
      public:
        MuxerThread();
        virtual ~MuxerThread();
        virtual bool threadLoop();
        virtual void requestExit();

        // This queue stores matched buffer as frame_matched_info_t
        List<muxer_queue_msg_t> mMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        void raw10ToRaw8(void *dest, void *src, int size);

      private:
        void waitMsgAvailable();
        int muxerThreeFrame(frame_matched_info_t *combPreviewResult);
    };
    sp<MuxerThread> mPreviewMuxerThread;

    sem_t sem;

  private:
    int mMaxPendingCount;
    int mPendingRequest;
    bool mCallbackFlag;
    Mutex mCallbackFlagLock;
    Mutex mResultLock;

    Mutex mFrameListMainPreviewLock;
    List<hwi_frame_buffer_info_t> mFrameListMainPreview;

    Mutex mFrameListMainCallbackLock;
    List<hwi_frame_buffer_info_t> mFrameListMainCallback;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux1;
    List<hwi_frame_buffer_info_t> mUnmatchedFrameListAux2;

    Mutex mUnmatchedQueueLock;
};
};

#endif /* SPRDCAMERAMU*/
