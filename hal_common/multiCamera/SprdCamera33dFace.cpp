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
#define LOG_TAG "Cam33dFace"
//#define LOG_NDEBUG 0
#include "SprdCamera33dFace.h"

using namespace android;
namespace sprdcamera {

SprdCamera33dFace *m_3dface = NULL;
#define PROC_3DFACE_TIMEOUT 3 /*sec*/

// Error Check Macros
#define CHECK_MUXER()                                                          \
    if (!m_3dface) {                                                           \
        HAL_LOGE("Error getting muxer ");                                      \
        return;                                                                \
    }
/*===========================================================================
 * FUNCTION         : SprdCamera33dFace
 *
 * DESCRIPTION     : SprdCamera33dFace Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera33dFace::SprdCamera33dFace() {
    HAL_LOGI(" E");

    mCallbackFlag = false;
    mPreviewMuxerThread = new MuxerThread();
    mFrameListMainPreview.clear();
    mFrameListMainCallback.clear();
    mUnmatchedFrameListAux1.clear();
    mUnmatchedFrameListAux2.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera33dFace
 *
 * DESCRIPTION     : SprdCamera33dFace Desctructor
 *
 *==========================================================================*/
SprdCamera33dFace::~SprdCamera33dFace() {
    HAL_LOGI("E");
    mFrameListMainPreview.clear();
    mFrameListMainCallback.clear();
    mUnmatchedFrameListAux1.clear();
    mUnmatchedFrameListAux2.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : getCamera3dFace
 *
 * DESCRIPTION     : Creates Camera 3dFace if not created
 *
 * PARAMETERS:
 *   @pMuxer               : Pointer to retrieve Camera 3dFace
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera33dFace::getCamera3dFace(SprdCamera3Multi **pMuxer) {
    *pMuxer = NULL;
    if (!m_3dface) {
        m_3dface = new SprdCamera33dFace();
    }
    CHECK_MUXER();
    *pMuxer = m_3dface;
    HAL_LOGD("m_3dface: %p ", m_3dface);

    return;
}

static config_multi_camera face_3d_config = {
#include "face_3d_config.h"
};
/*===========================================================================
 * FUNCTION         : load_config_file
 *
 * DESCRIPTION     : load 3dface config file
 * PARAMETERS:
 *
 *
 * RETURN             :  config_multi_camera *
 *==========================================================================*/

config_multi_camera *SprdCamera33dFace::load_config_file(void) {

    HAL_LOGD("load_config_file ");

    return &face_3d_config;
}

void SprdCamera33dFace::reReqConfig(camera3_capture_request_t *request,
                                    CameraMetadata *meta) {
    // meta config
    if (!meta) {
        return;
    }

    int tagCnt = 0;
    for (int i = 0; i < 3; i++) {
        tagCnt = meta[i].entryCount();
        if (tagCnt != 0) {
            uint8_t sprdZslEnabled = 0;
            meta[i].update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
        }
    }

    Mutex::Autolock l(mCallbackFlagLock);
    if (mCallbackFlag) {
        mReqConfigNum = 1;
    } else {
        mReqConfigNum = 0;
    }
}

void SprdCamera33dFace::reConfigInit() {

    mCallbackFlag = false;
    mFrameListMainPreview.clear();
    mFrameListMainCallback.clear();
    mUnmatchedFrameListAux1.clear();
    mUnmatchedFrameListAux2.clear();
    sem_init(&sem, 0, 0);
    HAL_LOGI("mMsgList %p", &mPreviewMuxerThread->mMsgList);

    if (mPreviewMuxerThread->mMsgList.empty()) {
        HAL_LOGI("mMsgList is empty");
    } else {
        mPreviewMuxerThread->mMsgList.clear();
    }
    m_3dface->mPreviewMuxerThread->run(
        String8::format("3DFACE_Muxer").string());
}

void SprdCamera33dFace::reConfigGetCameraInfo(CameraMetadata &metadata) {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    property_get("persist.vendor.3dface.res", prop, "RES_1080P");
    HAL_LOGI("3dface support cap resolution %s", prop);
    addAvailableStreamSize(metadata, prop);
}

void SprdCamera33dFace::reConfigFlush() {
    HAL_LOGI("E");

    sem_destroy(&sem);
    faceThreadExit();

    HAL_LOGI("X");
}

void SprdCamera33dFace::reConfigClose() {
    HAL_LOGI("E");

    if (!mFlushing) {
        mFlushing = true;
        sem_destroy(&sem);
        faceThreadExit();
    }

    HAL_LOGI("X");
}

#define TIME_DIFF_3D (60000000)

void SprdCamera33dFace::processCaptureResultMain(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    meta_save_t metadata_t;
    int index = 0;

    HAL_LOGD("E,id = %u", cur_frame_number);

    if (result_buffer == NULL) {
        // meta process
        metadata = result->result;
        HAL_LOGD("send  meta, framenumber:%d", cur_frame_number);
        metadata_t.frame_number = cur_frame_number;
        metadata_t.metadata = clone_camera_metadata(result->result);
        Mutex::Autolock l(mMetatLock);
        index = findMNIndex(cur_frame_number);
        if (index == CAM_TYPE_MAIN)
            mMetadataList.push_back(metadata_t);
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    hwi_frame_buffer_info_t cur_frame;
    cur_frame.frame_number = cur_frame_number;
    cur_frame.buffer = result->output_buffers->buffer;
    if (currStreamType == DEFAULT_STREAM) { // CallBack stream  process
        Mutex::Autolock l(m_3dface->mFrameListMainCallbackLock);
        HAL_LOGD("DEFAULT_STREAM  frame:%d for main camera",
                 cur_frame.frame_number);
        mFrameListMainCallback.push_back(cur_frame);
    } else if (currStreamType == CALLBACK_STREAM) { // Preview stream  process
        HAL_LOGD("CALLBACK_STREAM  frame:%d for main camera",
                 cur_frame.frame_number);
        Mutex::Autolock l(m_3dface->mFrameListMainPreviewLock);
        mFrameListMainPreview.push_back(cur_frame);
        sem_post(&sem);
    }

    HAL_LOGV("X");
}

void SprdCamera33dFace::processCaptureResultAux1(
    const camera3_capture_result_t *result) {
    processIRData(result, CAM_TYPE_AUX1);
}

void SprdCamera33dFace::processCaptureResultAux2(
    const camera3_capture_result_t *result) {
    processIRData(result, CAM_TYPE_AUX2);
}

void SprdCamera33dFace::processIRData(const camera3_capture_result_t *result,
                                      int type) {

    Mutex::Autolock l(mResultLock);
    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    HAL_LOGD("E,id=%d, type %d", cur_frame_number, type);
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    List<camera3_notify_msg_t> *notifyList = NULL;

    List<hwi_frame_buffer_info_t> *unmatchedFrameList = NULL;
    List<hwi_frame_buffer_info_t> *tomatchedFrameList = NULL;
    if (!result->output_buffers) {
        return;
    }
    int currStreamType = getStreamType(result_buffer->stream);

    if (type == CAM_TYPE_AUX1) {
        notifyList = &mNotifyListAux1;
        unmatchedFrameList = &mUnmatchedFrameListAux1;
        tomatchedFrameList = &mUnmatchedFrameListAux2;
    } else {
        notifyList = &mNotifyListAux2;
        unmatchedFrameList = &mUnmatchedFrameListAux2;
        tomatchedFrameList = &mUnmatchedFrameListAux1;
    }

    for (List<camera3_notify_msg_t>::iterator i = (*notifyList).begin();
         i != (*notifyList).end(); i++) {
        if (i->message.shutter.frame_number == cur_frame_number) {
            if (i->type == CAMERA3_MSG_SHUTTER) {
                searchnotifyresult = NOTIFY_SUCCESS;
                result_timestamp = i->message.shutter.timestamp;
            } else if (i->type == CAMERA3_MSG_ERROR) {
                HAL_LOGE("Return local buffer:%d caused by error Notify status",
                         result->frame_number);
                searchnotifyresult = NOTIFY_ERROR;

                pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                               mLocalBufferNumber, mLocalBufferList);
                return;
            }
        }
    }

    hwi_frame_buffer_info_t matched_frame;
    hwi_frame_buffer_info_t cur_frame;
    memset(&matched_frame, 0, sizeof(matched_frame));
    memset(&cur_frame, 0, sizeof(cur_frame));

    cur_frame.frame_number = cur_frame_number;
    cur_frame.timestamp = result_timestamp;
    cur_frame.buffer = (result->output_buffers)->buffer;
    hwi_frame_buffer_info_t *discard_frame = NULL;

    // Match frame
    if (MATCH_SUCCESS ==
        matchTwoFrame(cur_frame, *tomatchedFrameList, &matched_frame))

    {
        Mutex::Autolock l(mUnmatchedQueueLock);

        muxer_queue_msg_t muxer_msg;
        memset(&muxer_msg, 0, sizeof(muxer_msg));
        muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
        muxer_msg.combo_frame.frame_number = matched_frame.frame_number;
        if (type == CAM_TYPE_AUX1) {
            // camera hardware limit
            muxer_msg.combo_frame.buffer2 = matched_frame.buffer;
            muxer_msg.combo_frame.buffer3 = cur_frame.buffer;
        } else {
            muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
            muxer_msg.combo_frame.buffer3 = matched_frame.buffer;
        }
        {
            Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
            HAL_LOGD("Enqueue combo frame:%d for frame merge!ir buffer %p",
                     muxer_msg.combo_frame.frame_number,
                     muxer_msg.combo_frame.buffer2);
            clearFrameNeverMatched(type);
            mPreviewMuxerThread->mMsgList.push_back(muxer_msg);
            mPreviewMuxerThread->mMergequeueSignal.signal();
        }
    } else {
        HAL_LOGD("Enqueue newest unmatched frame:%d for Aux camera",
                 cur_frame.frame_number);

        discard_frame = pushToUnmatchedQueue(cur_frame, *unmatchedFrameList);
        if (discard_frame != NULL) {
            pushBufferList(mLocalBuffer, discard_frame->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            delete discard_frame;
        }
    }
}
/*===========================================================================
 * FUNCTION   :clearFrameNeverMatched
 *
 * DESCRIPTION: clear earlier frame which will never be matched any more
 *
 * PARAMETERS : which camera queue to be clear
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera33dFace::clearFrameNeverMatched(int whichCamera) {

    List<hwi_frame_buffer_info_t>::iterator itor;
    if (whichCamera == CAM_TYPE_AUX1) {
        itor = mUnmatchedFrameListAux1.begin();
        while (itor != mUnmatchedFrameListAux1.end()) {

            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux idx:%d", itor->frame_number);
            itor = mUnmatchedFrameListAux1.erase(itor);
        }
    } else if (whichCamera == CAM_TYPE_AUX2) {
        itor = mUnmatchedFrameListAux2.begin();
        while (itor != mUnmatchedFrameListAux2.end()) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux2 idx:%d", itor->frame_number);
            itor = mUnmatchedFrameListAux2.erase(itor);
        }
    }
}

/*===========================================================================
 * FUNCTION   :faceThreadExit
 *
 * DESCRIPTION: 3dface thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera33dFace::faceThreadExit(void) {

    HAL_LOGI("E");
    if (mPreviewMuxerThread != NULL) {
        if (mPreviewMuxerThread->isRunning()) {
            mPreviewMuxerThread->requestExit();
        }
        // wait threads quit to relese object
        mPreviewMuxerThread->join();
    }
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :MuxerThread
 *
 * DESCRIPTION: constructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera33dFace::MuxerThread::MuxerThread() {
    HAL_LOGI("E");

    // mMsgList.clear();
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~~MuxerThread
 *
 * DESCRIPTION: deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera33dFace::MuxerThread::~MuxerThread() {
    HAL_LOGI(" E");
    mMsgList.clear();

    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION: threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera33dFace::MuxerThread::threadLoop() {

    buffer_handle_t *output_buffer = NULL;
    muxer_queue_msg_t muxer_msg;
    uint32_t frame_number = 0;

    while (!mMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mMsgList.begin();
            muxer_msg = *it;
            mMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT: {
            // flush queue
            HAL_LOGW("MuxerThread Stopping bef, mMsgList.size=%d, "
                     "m_3dface->mSavedCallbackRequestList.size:%d",
                     mMsgList.size(),
                     m_3dface->mSavedCallbackRequestList.size());

            List<multi_request_saved_t>::iterator itor =
                m_3dface->mSavedPrevRequestList.begin();
            HAL_LOGD("exit frame_number %u", itor->frame_number);
            while (itor != m_3dface->mSavedPrevRequestList.end()) {
                frame_number = itor->frame_number;
                itor++;
                m_3dface->CallBackResult(frame_number,
                                         CAMERA3_BUFFER_STATUS_ERROR,
                                         CALLBACK_STREAM, CAM_TYPE_MAIN);
                m_3dface->CallBackResult(frame_number,
                                         CAMERA3_BUFFER_STATUS_ERROR,
                                         DEFAULT_STREAM, CAM_TYPE_MAIN);
            }
            return false;
        } break;
        case MUXER_MSG_DATA_PROC: {
            HAL_LOGI("mFrameListMainPreview  size %d mFrameListMainCallback  "
                     "size %d, frame num %d",
                     m_3dface->mFrameListMainPreview.size(),
                     m_3dface->mFrameListMainCallback.size(),
                     muxer_msg.combo_frame.frame_number);
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += PROC_3DFACE_TIMEOUT;

            if (sem_timedwait(&m_3dface->sem, &ts) == -1) {
                HAL_LOGE("sem_timedwait() timed out\n");
            } else {
                if (!m_3dface->mFrameListMainPreview.empty()) {
                    Mutex::Autolock l(m_3dface->mFrameListMainPreviewLock);

                    List<hwi_frame_buffer_info_t>::iterator itor;
                    itor = m_3dface->mFrameListMainPreview.begin();
                    muxer_msg.combo_frame.buffer1 = itor->buffer;
                    muxer_msg.combo_frame.frame_number = itor->frame_number;
                    m_3dface->mFrameListMainPreview.erase(itor);

                    if (!m_3dface->mFrameListMainCallback.empty()) {
                        Mutex::Autolock l(m_3dface->mFrameListMainCallbackLock);
                        itor = m_3dface->mFrameListMainCallback.begin();
                        muxer_msg.combo_frame.buffer4 = itor->buffer;
                        m_3dface->mFrameListMainCallback.erase(itor);
                    } else {
                        muxer_msg.combo_frame.buffer4 = NULL;
                    }
                    HAL_LOGI("mFrameListMainPreview  size %d "
                             "mFrameListMainCallback  size %d ",
                             m_3dface->mFrameListMainPreview.size(),
                             m_3dface->mFrameListMainCallback.size());

                    if (NO_ERROR == muxerThreeFrame(&muxer_msg.combo_frame)) {
                        m_3dface->CallBackResult(
                            muxer_msg.combo_frame.frame_number,
                            CAMERA3_BUFFER_STATUS_OK, DEFAULT_STREAM,
                            CAM_TYPE_MAIN);
                        m_3dface->CallBackResult(
                            muxer_msg.combo_frame.frame_number,
                            CAMERA3_BUFFER_STATUS_OK, CALLBACK_STREAM,
                            CAM_TYPE_MAIN);
                    } else {
                        m_3dface->CallBackResult(
                            muxer_msg.combo_frame.frame_number,
                            CAMERA3_BUFFER_STATUS_ERROR, CALLBACK_STREAM,
                            CAM_TYPE_MAIN);
                        m_3dface->CallBackResult(
                            muxer_msg.combo_frame.frame_number,
                            CAMERA3_BUFFER_STATUS_ERROR, DEFAULT_STREAM,
                            CAM_TYPE_MAIN);
                    }
                }
            }

        } break;
        default:
            break;
        }
    };

    waitMsgAvailable();

    return true;
}
/*===========================================================================
 * FUNCTION   :requestExit
 *
 * DESCRIPTION: request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera33dFace::MuxerThread::requestExit() {

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}
/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera33dFace::MuxerThread::waitMsgAvailable() {
    while (mMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}

void SprdCamera33dFace::MuxerThread::raw10ToRaw8(void *dest, void *src,
                                                 int size) {
    if (dest == NULL || src == NULL || size == 0) {
        HAL_LOGE("para is null. dst %p, src %p, size %d", dest, src, size);
        return;
    }
    int i;
    char *p1 = (char *)src;
    char *p2 = (char *)dest;
    for (i = 0; i < size / 5; i++) {
        memcpy(p2, p1, 4 * sizeof(char));

        p1 += 5;
        p2 += 4;
    }

    HAL_LOGI(" success %d", i);
}

/*===========================================================================
 * FUNCTION   :muxerThreeFrame
 *
 * DESCRIPTION: muxerThreeFrame
 * combPreviewResult buffer1  is main preview
 * combPreviewResult buffer2  is aux
 * combPreviewResult buffer3  is aux2
 * combPreviewResult buffer3  is main call back
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera33dFace::MuxerThread::muxerThreeFrame(
    frame_matched_info_t *combPreviewResult) {
    HAL_LOGD("E");

    int rc = NO_ERROR;
    List<multi_request_saved_t>::iterator itor;
    buffer_handle_t *output_buf = NULL;
    void *output_buf_addr = NULL;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    void *input_buf3_addr = NULL;
    void *input_buf4_addr = NULL;
    int8_t *dst_addr = NULL;
    int8_t *src_addr = NULL;
    int size_left = 0;
    int src_size = 0;
    int buf_size = 0;
    int width = 0;
    int height = 0;
    int convert_size = 0;
    int angle = IMG_ANGLE_0;
    int dst_fd = 0;
    int src_fd = 0;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.3dface.rot", prop, "1");
    angle = atoi(prop);
    HAL_LOGI("3dface rotate %d", angle);

    if (combPreviewResult->buffer1 == NULL ||
        combPreviewResult->buffer2 == NULL ||
        combPreviewResult->buffer3 == NULL) {
        HAL_LOGE("error,input_buf1:%p input_buf2:%p,input_buf3 %p",
                 combPreviewResult->buffer1, combPreviewResult->buffer2,
                 combPreviewResult->buffer3);
        return BAD_VALUE;
    }

    {
        Mutex::Autolock l(m_3dface->mRequestLock);
        itor = m_3dface->mSavedCallbackRequestList.begin();
        while (itor != m_3dface->mSavedCallbackRequestList.end()) {
            HAL_LOGI("itor->frame_number=%d, combine num %d",
                     itor->frame_number, combPreviewResult->frame_number);
            if (itor->frame_number == combPreviewResult->frame_number) {
                break;
            }
            itor++;
        }
        // if no matching request found, return buffers
        if (itor == m_3dface->mSavedCallbackRequestList.end()) {
            m_3dface->pushBufferList(
                m_3dface->mLocalBuffer, combPreviewResult->buffer2,
                m_3dface->mLocalBufferNumber, m_3dface->mLocalBufferList);
            m_3dface->pushBufferList(
                m_3dface->mLocalBuffer, combPreviewResult->buffer3,
                m_3dface->mLocalBufferNumber, m_3dface->mLocalBufferList);
            if (combPreviewResult->buffer4) {
                m_3dface->pushBufferList(
                    m_3dface->mLocalBuffer, combPreviewResult->buffer4,
                    m_3dface->mLocalBufferNumber, m_3dface->mLocalBufferList);
            }
            HAL_LOGE("can't find frame %d", combPreviewResult->frame_number);
            return UNKNOWN_ERROR;
        } else {
            output_buf = itor->buffer;
            if (output_buf == NULL) {
                HAL_LOGE("output buffer is null");
                rc = BAD_VALUE;
                goto fail_map_output;
            }
            rc = m_3dface->map(output_buf, &output_buf_addr);
            if (rc != NO_ERROR) {
                HAL_LOGE("fail to map output buffer");
                goto fail_map_output;
            }
        }
    }

    rc = m_3dface->map(combPreviewResult->buffer1, &input_buf1_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer1");
        goto fail_map_input1;
    }

    rc = m_3dface->map(combPreviewResult->buffer2, &input_buf2_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer2");
        goto fail_map_input2;
    }

    rc = m_3dface->map(combPreviewResult->buffer3, &input_buf3_addr);
    if (rc != NO_ERROR) {
        HAL_LOGE("fail to map input buffer3");
        goto fail_map_input3;
    }

    if (combPreviewResult->buffer4) {
        rc = m_3dface->map(combPreviewResult->buffer4, &input_buf4_addr);

        if (rc != NO_ERROR) {
            HAL_LOGE("fail to map input buffer3");
            goto fail_map_input4;
        }
    }

    size_left = ADP_BUFSIZE(*output_buf);
    dst_addr = (int8_t *)output_buf_addr;
    dst_fd = ADP_BUFFD(*output_buf);
    HAL_LOGD("dst_addr:%p, size %d", dst_addr, size_left);

    // buf1--main preview buf process
    if (output_buf_addr) {
        width = ADP_WIDTH(*combPreviewResult->buffer1);
        height = ADP_HEIGHT(*combPreviewResult->buffer1);
        buf_size = ADP_BUFSIZE(*combPreviewResult->buffer1);
        src_size = width * height * 3 / 2; // yuv size
        src_addr = (int8_t *)input_buf1_addr;
        src_fd = ADP_BUFFD(*combPreviewResult->buffer1);
        HAL_LOGD("input_buf1:src size %d, size_left %d", src_size, size_left);
        HAL_LOGD("intput width %d, height %d", ADP_WIDTH(*output_buf),
                 ADP_HEIGHT(*output_buf));
        HAL_LOGD("buf1 %d, %d, buf size %d, fd %d, %d", width, height, buf_size,
                 src_fd, dst_fd);
        property_get("persist.vendor.dump.3dface", prop, "null");
        if (!strcmp(prop, "preview") || !strcmp(prop, "all")) {
            m_3dface->dumpData((unsigned char *)input_buf1_addr, 1, src_size,
                               width, height, itor->frame_number, "prev");
        }
        if (src_size <= size_left) {
            m_3dface->NV21Rotate(dst_addr, dst_fd, src_addr, src_fd, width,
                                 height, angle);
            if (!strcmp(prop, "preview") || !strcmp(prop, "all")) {
                if (angle == IMG_ANGLE_90 || angle == IMG_ANGLE_270) {
                    m_3dface->dumpData((unsigned char *)dst_addr, 1, src_size,
                                       height, width, itor->frame_number,
                                       "prev");
                } else {
                    m_3dface->dumpData((unsigned char *)dst_addr, 1, src_size,
                                       width, height, itor->frame_number,
                                       "prev");
                }
            }
            size_left -= src_size;
            dst_addr += src_size;
        }
    }

    // buf2--aux1 buf process
    if (input_buf2_addr) {
        width = ADP_WIDTH(*combPreviewResult->buffer2);
        height = ADP_HEIGHT(*combPreviewResult->buffer2);
        buf_size = ADP_BUFSIZE(*combPreviewResult->buffer2);
        src_size = width * height * 10 / 8; // raw10 size
        convert_size = width * height;      // raw8 size
        src_addr = (int8_t *)input_buf2_addr;
        src_fd = ADP_BUFFD(*combPreviewResult->buffer2);
        property_get("persist.vendor.dump.3dface", prop, "null");
        if (!strcmp(prop, "aux1_raw10") || !strcmp(prop, "all_raw10")) {
            m_3dface->dumpData((unsigned char *)input_buf2_addr, 4, src_size,
                               width, height, itor->frame_number, "aux1_raw10");
        }
        HAL_LOGD("input_buf2:%p, src size %d, size_left %d", src_addr, src_size,
                 size_left);
        HAL_LOGD("buf2 width %d, height %d,buf size %d, fd %d", width, height,
                 buf_size, src_fd);
        if (src_size <= size_left) {
            int temp_size = width * height;
            uint8_t *temp_addr = (uint8_t *)malloc(width * height);
            memset(temp_addr, 0, sizeof(sizeof(uint8_t) * temp_size));
            raw10ToRaw8((void *)temp_addr, (void *)src_addr, src_size);
            m_3dface->Raw8Rotate((uint8_t *)dst_addr, (uint8_t *)temp_addr,
                                 width, height, angle);
            property_get("persist.vendor.dump.3dface", prop, "null");
            if (!strcmp(prop, "aux1_raw8") || !strcmp(prop, "all")) {
                if (angle == IMG_ANGLE_90 || angle == IMG_ANGLE_270) {
                    m_3dface->dumpData((unsigned char *)dst_addr, 4,
                                       convert_size, height, width,
                                       itor->frame_number, "aux1_raw8");
                } else {
                    m_3dface->dumpData((unsigned char *)dst_addr, 4,
                                       convert_size, width, height,
                                       itor->frame_number, "aux1_raw8");
                }
            }
            size_left -= convert_size;
            dst_addr += convert_size;
            free(temp_addr);
        }
    }

    // buf3--aux2 buf process
    if (input_buf3_addr) {
        width = ADP_WIDTH(*combPreviewResult->buffer3);
        height = ADP_HEIGHT(*combPreviewResult->buffer3);
        buf_size = ADP_BUFSIZE(*combPreviewResult->buffer3);
        src_size = width * height * 10 / 8; // raw10 size;
        convert_size = width * height;      // raw8 size
        src_addr = (int8_t *)input_buf3_addr;
        src_fd = ADP_BUFFD(*combPreviewResult->buffer3);
        property_get("persist.vendor.dump.3dface", prop, "null");
        if (!strcmp(prop, "aux2_raw10") || !strcmp(prop, "all_raw10")) {
            m_3dface->dumpData((unsigned char *)input_buf3_addr, 4, src_size,
                               width, height, itor->frame_number, "aux2_raw10");
        }
        HAL_LOGD("input_buf3:%p, src size %d, size_left %d", src_addr, src_size,
                 size_left);
        HAL_LOGD("buf3 width %d, height %d, buf size %d, fd %d", width, height,
                 buf_size, src_fd);
        if (src_size <= size_left) {
            int temp_size = width * height;
            uint8_t *temp_addr = (uint8_t *)malloc(width * height);
            memset(temp_addr, 0, sizeof(sizeof(uint8_t) * temp_size));
            raw10ToRaw8((void *)temp_addr, (void *)src_addr, src_size);
            m_3dface->Raw8Rotate((uint8_t *)dst_addr, (uint8_t *)temp_addr,
                                 width, height, angle);
            property_get("persist.vendor.dump.3dface", prop, "null");
            if (!strcmp(prop, "aux2_raw8") || !strcmp(prop, "all")) {
                if (angle == IMG_ANGLE_90 || angle == IMG_ANGLE_270) {
                    m_3dface->dumpData((unsigned char *)dst_addr, 4,
                                       convert_size, height, width,
                                       itor->frame_number, "aux2_raw8");
                } else {
                    m_3dface->dumpData((unsigned char *)dst_addr, 4,
                                       convert_size, width, height,
                                       itor->frame_number, "aux2_raw8");
                }
            }
            size_left -= convert_size;
            dst_addr += convert_size;
            free(temp_addr);
        }
    }

    // add callback flag
    {
        Mutex::Autolock l(m_3dface->mCallbackFlagLock);
        m_3dface->mCallbackFlag = (input_buf4_addr == NULL) ? false : true;
        if (src_size <= size_left) {
            memcpy(dst_addr, &m_3dface->mCallbackFlag, 1);
            HAL_LOGD("mCallbackFlag %d, size_left %d, dst_addr %d",
                     m_3dface->mCallbackFlag, size_left, *dst_addr);
            size_left -= 1;
            dst_addr += 1;
        }
    }

    // buf4--main callback buf process
    if (input_buf4_addr) {
        width = ADP_WIDTH(*combPreviewResult->buffer4);
        height = ADP_HEIGHT(*combPreviewResult->buffer4);
        buf_size = ADP_BUFSIZE(*combPreviewResult->buffer4);
        src_size = width * height * 10 / 8; // raw10 size;
        convert_size = width * height;      // raw8 size
        src_addr = (int8_t *)input_buf4_addr;
        src_fd = ADP_BUFFD(*combPreviewResult->buffer4);
        property_get("persist.vendor.dump.3dface", prop, "null");
        if (!strcmp(prop, "main_raw10") || !strcmp(prop, "all_raw10")) {
            m_3dface->dumpData((unsigned char *)input_buf4_addr, 4, src_size,
                               width, height, itor->frame_number, "main_raw10");
        }
        HAL_LOGD("input_buf4:%p, src size %d, size_left %d", src_addr, src_size,
                 size_left);
        HAL_LOGD("buf4 width %d, height %d, buf size %d, fd %d", width, height,
                 buf_size, src_fd);

        if (src_size <= size_left) {
            int temp_size = width * height;
            uint8_t *temp_addr = (uint8_t *)malloc(width * height);
            memset(temp_addr, 0, sizeof(sizeof(uint8_t) * temp_size));
            raw10ToRaw8((void *)temp_addr, (void *)src_addr, src_size);
            m_3dface->Raw8Rotate((uint8_t *)dst_addr, (uint8_t *)temp_addr,
                                 width, height, angle);
            property_get("persist.vendor.dump.3dface", prop, "null");
            if (!strcmp(prop, "main_raw8") || !strcmp(prop, "all")) {
                if (angle == IMG_ANGLE_90 || angle == IMG_ANGLE_270) {
                    m_3dface->dumpData((unsigned char *)dst_addr, 4,
                                       convert_size, height, width,
                                       itor->frame_number, "main_raw8");
                } else {
                    m_3dface->dumpData((unsigned char *)dst_addr, 4,
                                       convert_size, width, height,
                                       itor->frame_number, "main_raw8");
                }
            }
            size_left -= convert_size;
            dst_addr += convert_size;
            free(temp_addr);
        }
    }

    m_3dface->flushIonBuffer(ADP_BUFFD(*output_buf), output_buf_addr,
                             ADP_BUFSIZE(*output_buf));

    if (combPreviewResult->buffer2) {
        m_3dface->pushBufferList(
            m_3dface->mLocalBuffer, combPreviewResult->buffer2,
            m_3dface->mLocalBufferNumber, m_3dface->mLocalBufferList);
    }
    if (combPreviewResult->buffer3) {
        m_3dface->pushBufferList(
            m_3dface->mLocalBuffer, combPreviewResult->buffer3,
            m_3dface->mLocalBufferNumber, m_3dface->mLocalBufferList);
    }
    if (combPreviewResult->buffer4) {
        m_3dface->pushBufferList(
            m_3dface->mLocalBuffer, combPreviewResult->buffer4,
            m_3dface->mLocalBufferNumber, m_3dface->mLocalBufferList);
    }

    if (combPreviewResult->buffer4) {
        m_3dface->unmap(combPreviewResult->buffer4);
    }
fail_map_input4:
    m_3dface->unmap(combPreviewResult->buffer3);
fail_map_input3:
    m_3dface->unmap(combPreviewResult->buffer2);
fail_map_input2:
    m_3dface->unmap(combPreviewResult->buffer1);
fail_map_input1:
    m_3dface->unmap(output_buf);
fail_map_output:

    HAL_LOGD("X");

    return rc;
}
};
