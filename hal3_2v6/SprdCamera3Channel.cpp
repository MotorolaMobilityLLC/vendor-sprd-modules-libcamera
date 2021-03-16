/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#define LOG_TAG "Cam3Channel"
//#define LOG_NDEBUG 0
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)

#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <hardware/camera3.h>
#include <system/camera_metadata.h>
#include <utils/Trace.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <cutils/properties.h>
#include "SprdCamera3Channel.h"
#include "SprdCamera3HWI.h"

using namespace android;

#define MIN_STREAMING_BUFFER_NUM 7 + 11
#define AE_CONTROL (1<<0)
#define AF_CONTROL (1<<1)
#define AESYNCNUM 4
#define CALLBACK_ERROR 1


namespace sprdcamera {

/**************************SprdCamera3Channel************************************************/
SprdCamera3Channel::SprdCamera3Channel(SprdCamera3OEMIf *oem_if,
                                       channel_cb_routine cb_routine,
                                       SprdCamera3Setting *setting,
                                       void *userData) {
    mOEMIf = oem_if;
    mUserData = userData;
    mChannelCB = cb_routine;
    mSetting = setting;
}

SprdCamera3Channel::~SprdCamera3Channel() {}

bool SprdCamera3Channel::isFaceBeautyOn(SPRD_DEF_Tag *sprddefInfo) {
    for (int i = 0; i < SPRD_FACE_BEAUTY_PARAM_NUM; i++) {
        if (sprddefInfo->perfect_skin_level[i] != 0)
            return true;
    }
    return false;
}
/**********************SprdCamera3RegularChannel start*************************/
SprdCamera3RegularChannel::SprdCamera3RegularChannel(
    SprdCamera3OEMIf *oem_if, channel_cb_routine cb_routine,
    SprdCamera3Setting *setting, SprdCamera3Channel *metadata_channel,
    camera_channel_type_t channel_type, void *userData)
    : SprdCamera3Channel(oem_if, cb_routine, setting, userData) {

    HAL_LOGV("E");
    mChannelType = channel_type;
    for (size_t i = 0; i < CHANNEL_MAX_STREAM_NUM; i++)
        mCamera3Stream[i] = NULL;

    mInputBuff = NULL;
    mMemory = new SprdCamera3GrallocMemory();
    if (NULL == mMemory) {
        HAL_LOGE("no mem!");
    }
    memset(&mInputBufInfo, 0, sizeof(hal_mem_info_t));
    mMetadataChannel = metadata_channel;

    HAL_LOGV("X");
}

SprdCamera3RegularChannel::~SprdCamera3RegularChannel() {
    for (size_t i = 0; i < CHANNEL_MAX_STREAM_NUM; i++) {
        if (mCamera3Stream[i]) {
            delete mCamera3Stream[i];
            mCamera3Stream[i] = NULL;
        }
    }
    if (mMemory) {
        delete mMemory;
        mMemory = NULL;
    }
}

int SprdCamera3RegularChannel::start(uint32_t frame_number) {
    int ret = NO_ERROR;

    ret = mOEMIf->start(mChannelType, frame_number);

    return ret;
}

int SprdCamera3RegularChannel::stop(uint32_t frame_number) {
    int ret;

    ret = mOEMIf->stop(mChannelType, frame_number);

    return NO_ERROR;
}

int SprdCamera3RegularChannel::channelCbRoutine(
    uint32_t frame_number, int64_t timestamp,
    camera_stream_type_t stream_type) {
    int ret = NO_ERROR;
    cam_result_data_info_t result_info;
    int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;
    camera3_stream_t *stream;
    buffer_handle_t *buffer;
    uint32_t handledFrameNum = 0;

#ifdef CAMERA_POWER_DEBUG_ENABLE
    bool cam_not_disp;
    char value[PROPERTY_VALUE_MAX];

    property_get("vendor.camera.nodisplay", value, "false");
    cam_not_disp = !strcmp(value, "true");
#endif

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGE("channel has no valied stream");
        return INVALID_OPERATION;
    }

    // same stream, first in, should first out
    handledFrameNum = mCamera3Stream[index]->getHandledFrameNum();
    if (frame_number - handledFrameNum > 1 || frame_number == 1)
        channelClearInvalidQBuff(frame_number - 1, timestamp, stream_type);

    mCamera3Stream[index]->getStreamInfo(&stream);
    ret = mCamera3Stream[index]->buffDoneDQ(frame_number, &buffer);
    if (ret != NO_ERROR) {
        HAL_LOGE("dq error,stream_type=%d,index=%d,fram_number=%d", stream_type,
                 index, frame_number);
        return BAD_VALUE;
    }

#ifdef CONFIG_CAMERA_EIS
    // stream reserved[0] used for save eis crop rect.
    EIS_CROP_Tag eiscrop_Info;
    SPRD_DEF_Tag *sprddefInfo;
    CONTROL_Tag controlInfo;
    sprddefInfo = mSetting->getSPRDDEFTagPTR();
    mSetting->getCONTROLTag(&controlInfo);
    memset(&eiscrop_Info, 0x00, sizeof(EIS_CROP_Tag));
    // before used,set reserved[0] to null
    stream->reserved[0] = NULL;
    if (ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD ==
        controlInfo.capture_intent && sprddefInfo->sprd_eis_enabled &&
        stream->data_space == HAL_DATASPACE_UNKNOWN) {
        mSetting->getEISCROPTag(&eiscrop_Info);
        stream->reserved[0] = (void *)&eiscrop_Info;
        HAL_LOGI("eis crop:[%d, %d, %d, %d]", eiscrop_Info.crop[0],
                 eiscrop_Info.crop[1], eiscrop_Info.crop[2],
                 eiscrop_Info.crop[3]);
    }
#endif

    result_info.is_urgent = false;
    result_info.buffer = buffer;
    result_info.frame_number = frame_number;
    result_info.stream = stream;
    result_info.timestamp = timestamp;
    result_info.buff_status = CAMERA3_BUFFER_STATUS_OK;
#ifdef CAMERA_POWER_DEBUG_ENABLE
    if (cam_not_disp) {
        result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
    }
#endif
    result_info.msg_type = CAMERA3_MSG_SHUTTER;

    mChannelCB(&result_info, mUserData);

    mCamera3Stream[index]->setHandledFrameNum(frame_number);

#ifdef CONFIG_CAMERA_EIS
    // after used, set stream reserved[0] to null
    stream->reserved[0] = NULL;
#endif
    return NO_ERROR;
}

int SprdCamera3RegularChannel::channelClearInvalidQBuff(
    uint32_t frame_num, int64_t timestamp, camera_stream_type_t stream_type) {
    camera3_stream_t *stream;
    buffer_handle_t *buffer = NULL;
    int32_t buff_num;
    uint32_t buff_frame_number;
    int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;
    cam_result_data_info_t result_info;
    int ret = NO_ERROR;
    int i = 0;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGW("channel has no valied stream, stream_type %d", stream_type);
        return INVALID_OPERATION;
    }

    mCamera3Stream[index]->getQBufListNum(&buff_num);
    while (i < buff_num) {
        ret = mCamera3Stream[index]->getQBuffFirstNum(&buff_frame_number);
        if (ret != NO_ERROR || buff_frame_number > frame_num) {
            return ret;
        }

        mCamera3Stream[index]->getStreamInfo(&stream);
        mCamera3Stream[index]->buffDoneDQ(buff_frame_number, &buffer);
        result_info.is_urgent = false;
        result_info.buffer = buffer;
        result_info.frame_number = buff_frame_number;
        result_info.stream = stream;
        result_info.timestamp = timestamp;
        result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
        result_info.msg_type = CAMERA3_MSG_ERROR;

        mChannelCB(&result_info, mUserData);

        i++;
        HAL_LOGD("drop buff frame_number %d, stream_type %d", buff_frame_number,
                 stream_type);
    }

    return NO_ERROR;
}

int SprdCamera3RegularChannel::channelClearAllQBuff(
    int64_t timestamp, camera_stream_type_t stream_type) {
    int ret = NO_ERROR;
    cam_result_data_info_t result_info;
    int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGW("channel has no valied stream");
        return INVALID_OPERATION;
    }

    uint32_t frame_number;
    camera3_stream_t *stream;
    buffer_handle_t *buffer;
    int buff_num;
    mCamera3Stream[index]->getQBufListNum(&buff_num);
    mCamera3Stream[index]->getStreamInfo(&stream);
    for (int i = 0; i < buff_num; i++) {
        ret = mCamera3Stream[index]->buffFirstDoneDQ(&frame_number, &buffer);
        if (ret != NO_ERROR) {
            HAL_LOGD("dq, bufId = %d", i);
            continue;
        }
        result_info.is_urgent = false;
        result_info.buffer = buffer;
        result_info.frame_number = frame_number;
        result_info.stream = stream;
        result_info.timestamp = timestamp;
        result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
        result_info.msg_type = CAMERA3_MSG_ERROR;

        mChannelCB(&result_info, mUserData);
    }
    return NO_ERROR;
}

int SprdCamera3RegularChannel::channelUnmapCurrentQBuff(
    uint32_t frame_num, int64_t timestamp, camera_stream_type_t stream_type) {
    int ret = NO_ERROR;
    cam_result_data_info_t result_info;
    buffer_handle_t *buffer;
    int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGE("channel has no valied stream");
        return INVALID_OPERATION;
    }

    ret = mCamera3Stream[index]->buffDoneDQ(frame_num, &buffer);

    return NO_ERROR;
}

int SprdCamera3RegularChannel::registerBuffers(
    const camera3_stream_buffer_set_t *buffer_set) {
    return NO_ERROR;
}

int SprdCamera3RegularChannel::request(camera3_stream_t *stream,
                                       buffer_handle_t *buffer,
                                       uint32_t frameNumber) {
    int ret = NO_ERROR;
    int i;
    char multicameramode[PROPERTY_VALUE_MAX];

    for (i = 0; i < CHANNEL_MAX_STREAM_NUM; i++) {
        if (mCamera3Stream[i]) {
            camera3_stream_t *new_stream;

            mCamera3Stream[i]->getStreamInfo(&new_stream);
            if (new_stream == stream) {
                ret = mCamera3Stream[i]->buffDoneQ(frameNumber, buffer);
                if (ret != NO_ERROR) {
                    HAL_LOGE("err do Q, stream id = %d", i);
                    return ret;
                }

                if (i == 0) {
#ifdef CONFIG_ISP_2_3
                    if (!mOEMIf->getJpegWithBigSizePreviewFlag())
#endif
                        mOEMIf->queueBuffer(buffer, CAMERA_STREAM_TYPE_PREVIEW);
                } else if (i == (CAMERA_STREAM_TYPE_VIDEO -
                                 REGULAR_STREAM_TYPE_BASE)) {
                   if (mOEMIf->isVideoCopyFromPreview()) {
                       HAL_LOGD("video stream copy preview stream");
                   } else {
                      mOEMIf->queueBuffer(buffer, CAMERA_STREAM_TYPE_VIDEO);
                   }
                } else if (i == (CAMERA_STREAM_TYPE_CALLBACK -
                                 REGULAR_STREAM_TYPE_BASE))
                    mOEMIf->queueBuffer(buffer, CAMERA_STREAM_TYPE_CALLBACK);
                else if (i ==
                         (CAMERA_STREAM_TYPE_YUV2 - REGULAR_STREAM_TYPE_BASE))
                    mOEMIf->queueBuffer(buffer, CAMERA_STREAM_TYPE_YUV2);
                break;
            }
        }
    }

    if (i == CHANNEL_MAX_STREAM_NUM) {
        HAL_LOGE("buff request failed, has no stream %p", stream);
        return INVALID_OPERATION;
    }

    return ret;
}

int SprdCamera3RegularChannel::addStream(camera_stream_type_t stream_type,
                                         camera3_stream_t *stream) {
    camera_stream_type_t oldstream_type;
    camera3_stream_t *oldstream;
    int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;
    camera_dimension_t stream_size;

    HAL_LOGD("E: index = %d", index);
    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index]) {
        mCamera3Stream[index]->getStreamType(&oldstream_type);
        mCamera3Stream[index]->getStreamInfo(&oldstream);
        if (stream_type == oldstream_type) {
            if (oldstream == stream && oldstream->width == stream->width &&
                oldstream->height == stream->height) {
                return NO_ERROR;
            } else {
                delete mCamera3Stream[index];
                mCamera3Stream[index] = NULL;
            }
        }
    }

    stream_size.width = stream->width;
    stream_size.height = stream->height;
    mCamera3Stream[index] =
        new SprdCamera3Stream(stream, stream_size, stream_type, this);
    if (mCamera3Stream[index] == NULL) {
        HAL_LOGE("stream create failed");
        return INVALID_OPERATION;
    }

    HAL_LOGV("X");

    return NO_ERROR;
}

int SprdCamera3RegularChannel::deleteStream() {
    int8_t index = 0;
    while (mCamera3Stream[index]) {
        delete mCamera3Stream[index];
        HAL_LOGD("index=%d", index);
        mCamera3Stream[index++] = NULL;
    }
    return NO_ERROR;
}

int SprdCamera3RegularChannel::clearAllStreams() {
    size_t i;

    for (i = 0; i < CHANNEL_MAX_STREAM_NUM; i++) {
        if (mCamera3Stream[i]) {
            delete mCamera3Stream[i];
            mCamera3Stream[i] = NULL;
        }
    }

    return NO_ERROR;
}

int SprdCamera3RegularChannel::getStream(camera_stream_type_t stream_type,
                                         SprdCamera3Stream **stream) {
    int8_t index = stream_type - REGULAR_STREAM_TYPE_BASE;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGV("channel has no valied stream(type is %d)", stream_type);
        return INVALID_OPERATION;
    }

    *stream = mCamera3Stream[index];

    return NO_ERROR;
}

int SprdCamera3RegularChannel::setCapturePara(camera_capture_mode_t cap_mode) {
    mOEMIf->setCapturePara(cap_mode, 0);
    return NO_ERROR;
}

int SprdCamera3RegularChannel::setInputBuff(buffer_handle_t *buff) {
    mInputBuff = buff;

    return NO_ERROR;
}

int SprdCamera3RegularChannel::getInputBuff(cmr_uint *addr_vir,
                                            cmr_uint *addr_phy,
                                            cmr_uint *priv_data) {
    int ret = NO_ERROR;

    ret = mMemory->map(mInputBuff, &mInputBufInfo);
    if (ret != NO_ERROR)
        return ret;
    else {
        *addr_vir = (cmr_uint)mInputBufInfo.addr_vir;
        *addr_phy = (cmr_uint)mInputBufInfo.addr_phy;
        *priv_data = (cmr_uint)mInputBufInfo.fd;
    }

    return NO_ERROR;
}

int SprdCamera3RegularChannel::releaseInputBuff() {
    mMemory->unmap(mInputBuff, &mInputBufInfo);
    mInputBuff = NULL;
    return NO_ERROR;
}

int SprdCamera3RegularChannel::kMaxBuffers = 4;
/***********************SprdCamera3PicChannel start****************************/
SprdCamera3PicChannel::SprdCamera3PicChannel(
    SprdCamera3OEMIf *oem_if, channel_cb_routine cb_routine,
    SprdCamera3Setting *setting, SprdCamera3Channel *metadata_channel,
    camera_channel_type_t channel_type, void *userData)
    : SprdCamera3Channel(oem_if, cb_routine, setting, userData) {
    HAL_LOGV("E");
    mChannelType = channel_type;

    for (size_t i = 0; i < CHANNEL_MAX_STREAM_NUM; i++) {
        mCamera3Stream[i] = NULL;
    }
    buff_index = 0;

    mMetadataChannel = metadata_channel;
    HAL_LOGV("X");
}

SprdCamera3PicChannel::~SprdCamera3PicChannel() {
    for (size_t i = 0; i < CHANNEL_MAX_STREAM_NUM; i++) {
        if (mCamera3Stream[i]) {
            delete mCamera3Stream[i];
            mCamera3Stream[i] = NULL;
        }
    }
}

int SprdCamera3PicChannel::start(uint32_t frame_number) {
    int ret = NO_ERROR;

    ret = mOEMIf->start(mChannelType, frame_number);
    return ret;
}

int SprdCamera3PicChannel::stop(uint32_t frame_number) {
    int ret;

    ret = mOEMIf->stop(mChannelType, frame_number);

    return NO_ERROR;
}

int SprdCamera3PicChannel::channelCbRoutine(uint32_t frame_number,
                                            int64_t timestamp,
                                            camera_stream_type_t stream_type) {
    int ret = NO_ERROR;
    cam_result_data_info_t result_info;
    int8_t index = stream_type - PIC_STREAM_TYPE_BASE;
    uint32_t handledFrameNum = 0;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGE("channel has no valied stream");
        return INVALID_OPERATION;
    }

    camera3_stream_t *stream;
    buffer_handle_t *buffer;

    // same stream, first in, should first out
    handledFrameNum = mCamera3Stream[index]->getHandledFrameNum();
    if (frame_number - handledFrameNum > 1 || frame_number == 1) {
        channelClearInvalidQBuff(frame_number - 1, timestamp, stream_type);
    }

    mCamera3Stream[index]->getStreamInfo(&stream);
    ret = mCamera3Stream[index]->buffDoneDQ(frame_number, &buffer);
    if (ret != NO_ERROR) {
        HAL_LOGE("dq error, stream_type = %d", stream_type);
        return BAD_VALUE;
    }

    result_info.is_urgent = false;
    result_info.buffer = buffer;
    result_info.frame_number = frame_number;
    result_info.stream = stream;
    result_info.timestamp = timestamp;
    result_info.buff_status = CAMERA3_BUFFER_STATUS_OK;
    result_info.msg_type = CAMERA3_MSG_SHUTTER;

    mChannelCB(&result_info, mUserData);

    mCamera3Stream[index]->setHandledFrameNum(frame_number);

    return NO_ERROR;
}

int SprdCamera3PicChannel::channelClearInvalidQBuff(
    uint32_t frame_num, int64_t timestamp, camera_stream_type_t stream_type) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    cam_result_data_info_t result_info;
    int8_t index = stream_type - PIC_STREAM_TYPE_BASE;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGW("channel has no valied stream");
        return INVALID_OPERATION;
    }

    uint32_t buff_frame_number;
    camera3_stream_t *stream;
    buffer_handle_t *buffer = NULL;
    int buff_num;
    mCamera3Stream[index]->getQBufListNum(&buff_num);

    for (int i = 0; i < buff_num; i++) {
        ret = mCamera3Stream[index]->getQBuffFirstNum(&buff_frame_number);
        if (ret != NO_ERROR || buff_frame_number > frame_num) {
            return ret;
        }

        mCamera3Stream[index]->getStreamInfo(&stream);
        mCamera3Stream[index]->buffFirstDoneDQ(&buff_frame_number, &buffer);
        result_info.is_urgent = false;
        result_info.buffer = buffer;
        result_info.frame_number = buff_frame_number;
        result_info.stream = stream;
        result_info.timestamp = timestamp;
        result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
        result_info.msg_type = CAMERA3_MSG_ERROR;

        mChannelCB(&result_info, mUserData);
    }
    return NO_ERROR;
}

int SprdCamera3PicChannel::channelClearAllQBuff(
    int64_t timestamp, camera_stream_type_t stream_type) {
    ATRACE_CALL();

    int ret = NO_ERROR;
    cam_result_data_info_t result_info;
    int8_t index = stream_type - PIC_STREAM_TYPE_BASE;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGW("channel has no valied stream");
        return INVALID_OPERATION;
    }

    uint32_t frame_number;
    camera3_stream_t *stream;
    buffer_handle_t *buffer;
    int buff_num;
    mCamera3Stream[index]->getQBufListNum(&buff_num);

    for (int i = 0; i < buff_num; i++) {
        mCamera3Stream[index]->getStreamInfo(&stream);
        ret = mCamera3Stream[index]->buffFirstDoneDQ(&frame_number, &buffer);
        if (ret != NO_ERROR) {
            HAL_LOGD("dq, bufId = %d", i);
            continue;
        }

        result_info.is_urgent = false;
        result_info.buffer = buffer;
        result_info.frame_number = frame_number;
        result_info.stream = stream;
        result_info.timestamp = timestamp;
        result_info.buff_status = CAMERA3_BUFFER_STATUS_ERROR;
        result_info.msg_type = CAMERA3_MSG_ERROR;

        mChannelCB(&result_info, mUserData);
    }
    return NO_ERROR;
}

int SprdCamera3PicChannel::registerBuffers(
    const camera3_stream_buffer_set_t *buffer_set) {
    return NO_ERROR;
}

int32_t SprdCamera3PicChannel::request(camera3_stream_t *stream,
                                       buffer_handle_t *buffer,
                                       uint32_t frameNumber) {
    int ret;
    int i;
    camera_stream_type_t stream_type;

    for (i = 0; i < CHANNEL_MAX_STREAM_NUM; i++) {
        if (mCamera3Stream[i]) {
            camera3_stream_t *new_stream;
            mCamera3Stream[i]->getStreamInfo(&new_stream);

            if (new_stream == stream) {
                if (i ==
                    CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT -
                        PIC_STREAM_TYPE_BASE) {
                    ret = mCamera3Stream[i]->buffDoneQ2(frameNumber, buffer);
                    return ret;
                }
            }
        }
    }

    HAL_LOGE("buff request failed, has no stream %p", stream);

    return INVALID_OPERATION;
}

int SprdCamera3PicChannel::addStream(camera_stream_type_t stream_type,
                                     camera3_stream_t *stream) {
    camera_stream_type_t oldstream_type;
    camera3_stream_t *oldstream;
    int8_t index = stream_type - PIC_STREAM_TYPE_BASE;
    camera_dimension_t stream_size;

    HAL_LOGD("E: index = %d", index);

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index]) {
        mCamera3Stream[index]->getStreamType(&oldstream_type);
        mCamera3Stream[index]->getStreamInfo(&oldstream);
        if (stream_type == oldstream_type) {
            if (oldstream == stream && oldstream->width == stream->width &&
                oldstream->height == stream->height) {
                return NO_ERROR;
            } else {
                delete mCamera3Stream[index];
                mCamera3Stream[index] = NULL;
            }
        }
    }

    stream_size.width = stream->width;
    stream_size.height = stream->height;
    mCamera3Stream[index] =
        new SprdCamera3Stream(stream, stream_size, stream_type, this);
    if (mCamera3Stream[index] == NULL) {
        HAL_LOGE("stream create failed");
        return INVALID_OPERATION;
    }

    HAL_LOGV("X");
    return NO_ERROR;
}

int SprdCamera3PicChannel::deleteStream() {
    int8_t index = 0;
    while (mCamera3Stream[index]) {
        delete mCamera3Stream[index];
        HAL_LOGD("index=%d", index);
        mCamera3Stream[index++] = NULL;
    }
    return NO_ERROR;
}

int SprdCamera3PicChannel::clearAllStreams() {
    size_t i;

    for (i = 0; i < CHANNEL_MAX_STREAM_NUM; i++) {
        if (mCamera3Stream[i]) {
            delete mCamera3Stream[i];
            mCamera3Stream[i] = NULL;
        }
    }

    return NO_ERROR;
}

int SprdCamera3PicChannel::getStream(camera_stream_type_t stream_type,
                                     SprdCamera3Stream **stream) {
    int8_t index = stream_type - PIC_STREAM_TYPE_BASE;

    if (index < 0) {
        HAL_LOGE("stream_type %d is not valied type", stream_type);
        return BAD_VALUE;
    }

    if (mCamera3Stream[index] == NULL) {
        HAL_LOGW("channel has no valied stream(type is %d)", stream_type);
        return INVALID_OPERATION;
    }

    *stream = mCamera3Stream[index];

    return NO_ERROR;
}

int SprdCamera3PicChannel::setCapturePara(camera_capture_mode_t cap_mode) {
    mOEMIf->setCapturePara(cap_mode, 0);
    return NO_ERROR;
}

int SprdCamera3PicChannel::kMaxBuffers = 1;
/*******************SprdCamera3MetadataChannel start***************************/
SprdCamera3MetadataChannel::SprdCamera3MetadataChannel(
    SprdCamera3OEMIf *oem_if, channel_cb_routine cb_routine,
    SprdCamera3Setting *setting, void *userData)
    : SprdCamera3Channel(oem_if, cb_routine, setting, userData) {
        ReSetFirstMeta = true;
        memset(&syncAeParams, 0, sizeof(struct ae_params));
        memset(&syncAfParams, 0, sizeof(struct af_params));

//        mIspParamsList.clear();
    }

SprdCamera3MetadataChannel::~SprdCamera3MetadataChannel() {
    Mutex::Autolock lr(mLock);
    FrameVec.clear();
    mAeCallBackQue.clear();
    mAfCallBackQue.clear();
    mSyncResult.clear();
    mRequestInfoList.clear();
}
int SprdCamera3MetadataChannel::request(const CameraMetadata &metadata) {
    mSetting->updateWorkParameters(metadata);
    return 0;
}

int SprdCamera3MetadataChannel::request(
    const CameraMetadata &metadata, uint32_t frame_number) {
    int ret = 0;
    int tag = 0;
    int tmp_control = 0;
    bool aeaf_triger = false;
    bool isInvalidpa = false;
    struct isp_sync_params isp_params;
    memset(&isp_params, 0, sizeof(struct isp_sync_params));
    HAL_LOGV("ReSetFirstMeta = %d frame_number = %d", ReSetFirstMeta, frame_number);
    if(ReSetFirstMeta) {
        HAL_LOGI("Isp is not work");
        ReSetFirstMeta = false;
        return 0;
    } else
        mSetting->updateIspParameters(metadata);

    Mutex::Autolock lr(mLock);
    ret = mSetting->getAeParams(&(isp_params.ae_cts_params));
    ret = mSetting->getAfParams(&(isp_params.af_cts_params));

    if (metadata.exists(ANDROID_FLASH_MODE)) {
        HAL_LOGV("flashmode is %d [%d]", metadata.find(ANDROID_FLASH_MODE).data.u8[0], frame_number);
        if (metadata.find(ANDROID_FLASH_MODE).data.u8[0] == 0) {
            mFlashMap[frame_number] = ANDROID_FLASH_STATE_READY;
            HAL_LOGV("flash is off [%d]", frame_number);
        } else {
            mFlashMap[frame_number] = ANDROID_FLASH_STATE_FIRED;
            HAL_LOGV("flash is on [%d]", frame_number);
        }
    }

    if (isp_params.ae_cts_params.is_cts == true &&
        isp_params.ae_cts_params.exp_time == 99000 &&
        isp_params.ae_cts_params.sensitivity == 50 &&
        frame_number != 0) {
        HAL_LOGD("avoid to repeat setting ITS default params");
        isp_params.ae_cts_params.is_cts == false;
        }

    if (isp_params.ae_cts_params.is_cts == false) {
        if (metadata.exists(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER) &&
            frame_number != 0) {
            HAL_LOGV("AE frame_number:%d, precaptrue triger[%d]",
                frame_number, metadata.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER).data.u8[0]);
            if(metadata.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER).data.u8[0] == 1) {
                isp_params.ae_cts_params.frame_number = frame_number;
                isp_params.ae_cts_params.ae_precap_triger = 1;
                aeaf_triger = true;
                tmp_control |= AE_CONTROL;
            }
        } else
            isp_params.ae_cts_params.frame_number = -1;
    } else if (isp_params.ae_cts_params.is_cts == true) {
        isp_params.ae_cts_params.frame_number = frame_number;
        HAL_LOGV("AE frame_number:%d,", frame_number);
        tmp_control |= AE_CONTROL;
    }

    if (isp_params.af_cts_params.is_cts == false) {
        if (metadata.exists(ANDROID_CONTROL_AF_TRIGGER)) {
            if(metadata.find(ANDROID_CONTROL_AF_TRIGGER).data.u8[0] == 1) {
                HAL_LOGV("AF triger frame_number:%d,", frame_number);
                isp_params.af_cts_params.frame_number = frame_number;
                isp_params.af_cts_params.af_triger = 1;
                aeaf_triger = true;
                tmp_control |= AF_CONTROL;
            }
        } else
            isp_params.af_cts_params.frame_number = -1;
    } else if (isp_params.af_cts_params.is_cts == true) {
        isp_params.af_cts_params.frame_number = frame_number;
        HAL_LOGV("AF frame_number:%d,", frame_number);
        tmp_control |= AF_CONTROL;
    }

    ret = mSetting->setAeParams(isp_params.ae_cts_params);
    ret = mSetting->setAfParams(isp_params.af_cts_params);

    HAL_LOGV("isp_params:%p->%d,", &isp_params, sizeof(struct isp_sync_params));
    isp_params.syncRequst = tmp_control;
    isp_params.frame_number = (cmr_s32) frame_number;
    HAL_LOGD("isp_params.frame_number %d", isp_params.frame_number);

    if ((isp_params.frame_number < AESYNCNUM) &&
        isp_params.ae_cts_params.sensitivity == 50 &&
        isp_params.ae_cts_params.exp_time == 99000)
        isInvalidpa = true;

    if (!isInvalidpa && (isp_params.ae_cts_params.is_cts == true ||
        isp_params.af_cts_params.is_cts == true ||
        aeaf_triger == true)) {
        mRequestInfoList.push_back(isp_params);
        if (!mRequestInfoList.empty()) {
            int size = mRequestInfoList.size();;
            struct isp_sync_params tmp_control_ =
            mRequestInfoList.at(size - 1);
            HAL_LOGD("syncRequst:%d, frame_number: %d",
                tmp_control_.syncRequst,
                tmp_control_.frame_number);
        }
    }
    FrameVec.push_back(frame_number);
    HAL_LOGD("frameNum %d FrameVec size %d",frame_number, FrameVec.size());

    return 0;
}

bool SprdCamera3MetadataChannel::pushResult () {
    bool ret = false;
    bool is_first_fame = false;
    HAL_LOGD("mAeCallBackQue size %d, mAfCallBackQue size %d, FrameVec %d",
    mAeCallBackQue.size(), mAfCallBackQue.size(), FrameVec.size());
    Mutex::Autolock lr(mLock);
    if (!mAeCallBackQue.empty() && !mAfCallBackQue.empty()) {
        camera_metadata_t *new_result;
        sync_result_t tmp_result;
        struct isp_sync_params tmp_isp_params;


        const ae_params_t &ae_params_tmp = mAeCallBackQue.front();
        const af_params_t &af_params_tmp = mAfCallBackQue.front();
        cmr_u32 FrameNum = FrameVec.front();
        memset(&tmp_result, 0, sizeof(sync_result_t));
        memset(&tmp_isp_params, 0, sizeof(isp_sync_params));
        tmp_result.frame_number = FrameNum;
        tmp_result.result = mSetting->translateLocalToFwMetadata();
        tmp_isp_params.frame_number = FrameNum;
        //exp_time = ae_params_tmp.exp_time;
        //CMR_LOGD("exp_time %lld", exp_time);

        memcpy(&(tmp_isp_params.ae_cts_params), &ae_params_tmp, sizeof(ae_params_t));
        memcpy(&(tmp_isp_params.af_cts_params), &af_params_tmp, sizeof(af_params_t));

        tmp_result.result =
            mSetting->reportMetadataToFramework(&tmp_isp_params, tmp_result.result);

        mAeCallBackQue.pop_front();
        mAfCallBackQue.pop_front();

        /*delete mismatch ae, keep report the latest ae to framework*/
        if (tmp_isp_params.ae_cts_params.frame_number == -1 &&
            tmp_isp_params.af_cts_params.frame_number == -1 &&
            !mSyncResult.empty() && tmp_isp_params.frame_number >= AESYNCNUM) {
            HAL_LOGD("skip useless ae frame");
        } else {
            mSyncResult.push_back(tmp_result);
        }
        HAL_LOGD("frameNum %d", FrameNum);
        ret = true;
    }
    HAL_LOGV("mAeCallBackQue size %d, mAfCallBackQue size %d, FrameVec %d",
    mAeCallBackQue.size(), mAfCallBackQue.size(), FrameVec.size());
    return ret;
}

int SprdCamera3MetadataChannel::channelCbRoutine(
    uint32_t frame_number, int64_t timestamp,
    camera_stream_type_t stream_type) {

    HAL_LOGD("E");
    cam_result_data_info_t result_info;
    memset(&result_info, 0, sizeof(result_info));

    result_info.is_urgent = true;
    result_info.timestamp = timestamp;
    result_info.frame_number = 0;
    mChannelCB(&result_info, mUserData);
    HAL_LOGV("X");

    return NO_ERROR;
}

int SprdCamera3MetadataChannel::channelCbRoutine(
    enum camera_cb_type cb, void *params) {
    struct isp_sync_params isp_params;
    HAL_LOGD("E");
    SprdCamera3HWI *hwi = (SprdCamera3HWI *) mUserData;
    if(hwi->isMultiCameraMode(hwi->getMultiCameraMode())||
        mOEMIf->mSprdAppmodeId == CAMERA_MODE_SLOWMOTION) {
        //HAL_LOGE("multi camera can not use manule sensor");
        return 0;
    }
#ifndef CAMERA_MANULE_SNEOSR
    {
        return 0;
    }
#endif
    switch (cb) {
    case CAMERA_EVT_CB_AE_PARAMS: {
        // bool result_notfied = false;
        ae_params_t tmp_ae_params;
        struct ae_callback_params *ae_cts_callback_params = (struct ae_callback_params *)params;
        tmp_ae_params.frame_number = ae_cts_callback_params->frame_number;
        tmp_ae_params.exp_time = ae_cts_callback_params->cur_effect_exp_time;
        tmp_ae_params.sensitivity = ae_cts_callback_params->cur_effect_sensitivity;
        tmp_ae_params.fps = ae_cts_callback_params->cur_effect_fps;
        HAL_LOGV("report frame_number:%d, exp_time: %lld, sensitivity %d",
            tmp_ae_params.frame_number,
            tmp_ae_params.exp_time,
            tmp_ae_params.sensitivity);
        HAL_LOGV("AE control ae list %d", mAeCallBackQue.size());

        if (!FrameVec.empty()) {
            if (!mRequestInfoList.empty() &&
                (mRequestInfoList[0].syncRequst & AE_CONTROL)) {
                HAL_LOGD("FrameVec.front():%d, mRequestInfoList[0].frame_number:%d, mRequestInfoList[0].syncRequst:%d",
                FrameVec.front(),
                mRequestInfoList[0].frame_number,
                mRequestInfoList[0].syncRequst);

                struct isp_sync_params AE_syncframe;
                bool syncfamr_found = false;
                if (FrameVec.front() < mRequestInfoList[0].frame_number) {

                    vector<struct isp_sync_params>::iterator lsItr =
                    find_if(mRequestInfoList.begin(), mRequestInfoList.end(),
                        [&](const struct isp_sync_params &result_tmp) {
                            return (ae_cts_callback_params->frame_number ==
                                result_tmp.ae_cts_params.frame_number);
                        });
                    if(lsItr != mRequestInfoList.end()) {
                        AE_syncframe = *lsItr;
                        syncfamr_found = true;
                    }

                    if (syncfamr_found && (ae_cts_callback_params->frame_number ==
                        AE_syncframe.frame_number ||
                        AE_syncframe.ae_cts_params.ae_precap_triger == 1)) {
                        tmp_ae_params.ae_precap_triger == 1;
                        AE_syncframe.ae_notified = CALLBACK_ERROR;
                    } else {
                        mAeCallBackQue.push_back(tmp_ae_params);
                    }
                } else if(FrameVec.front() == mRequestInfoList[0].frame_number) {
                    if (mRequestInfoList[0].ae_notified == CALLBACK_ERROR) {
                            if (tmp_ae_params.exp_time !=
                                mRequestInfoList[0].ae_cts_params.exp_time) {
                                tmp_ae_params.exp_time = mRequestInfoList[0]
                                    .ae_cts_params.exp_time;
                                tmp_ae_params.sensitivity = mRequestInfoList[0]
                                    .ae_cts_params.sensitivity;
                                }
                                mAeCallBackQue.push_back(tmp_ae_params);
                    } else {
                        if (ae_cts_callback_params->frame_number ==
                            mRequestInfoList[0].frame_number &&
                            mRequestInfoList[0].unSyncCount < 4) {
                            mAeCallBackQue.push_back(tmp_ae_params);
                        } else {
                            mAeCallBackQue.push_back(tmp_ae_params);
                        }
                        mRequestInfoList[0].unSyncCount++;
                    }
                    HAL_LOGD("report mRequestInfoList frame_number:%d, exp_time: %lld, sensitivity %d",
                        mRequestInfoList[0].ae_cts_params.frame_number,
                        mRequestInfoList[0].ae_cts_params.exp_time,
                        mRequestInfoList[0].ae_cts_params.sensitivity);
                }
            }else
                mAeCallBackQue.push_back(tmp_ae_params);
        }else if (mAeCallBackQue.empty() && !mSyncResult.empty())
            mAeCallBackQue.push_back(tmp_ae_params);
        HAL_LOGV("AE control ae list %d", mAeCallBackQue.size());
    } break;

    case CAMERA_EVT_CB_AF_PARAMS: {
        af_params_t tmp_af_params;
        struct af_callback_params *af_cts_callback_params = (struct af_callback_params *)params;
        HAL_LOGV("af_callback_params->frame_number:%d", af_cts_callback_params->frame_number);
            tmp_af_params.frame_number = af_cts_callback_params->frame_number;
            tmp_af_params.focus_distance = af_cts_callback_params->focus_distance;
            tmp_af_params.lens_state = af_cts_callback_params->lens_state;
        HAL_LOGV("report frame_number:%d, focus_distance: %f, lens_state %d",
            tmp_af_params.frame_number,
            tmp_af_params.focus_distance,
            tmp_af_params.lens_state);

        if(!mRequestInfoList.empty())
            HAL_LOGV("AF control syncRequst:%d, frame_number: %d",
                mRequestInfoList[0].syncRequst,
                mRequestInfoList[0].frame_number);

        if (!FrameVec.empty()) {
            if (!mRequestInfoList.empty() &&
                (mRequestInfoList[0].syncRequst & AF_CONTROL)) {
                HAL_LOGV("AF control FrameVec.front[%d], request frame_number [%d], lens_state [%d]",
                FrameVec.front(),
                mRequestInfoList[0].frame_number,
                tmp_af_params.lens_state);
                struct isp_sync_params AF_syncframe;
                bool syncfamr_found = false;

                if (FrameVec.front() < mRequestInfoList[0].frame_number) {
                    vector<struct isp_sync_params>::iterator lsItr =
                    find_if(mRequestInfoList.begin(), mRequestInfoList.end(),
                        [&](const struct isp_sync_params &result_tmp) {
                            return (af_cts_callback_params->frame_number ==
                                result_tmp.af_cts_params.frame_number);
                        });
                    if(lsItr != mRequestInfoList.end()) {
                        AF_syncframe = *lsItr;
                        syncfamr_found = true;
                    }
                    if (syncfamr_found && (af_cts_callback_params->frame_number ==
                        AF_syncframe.frame_number ||
                        AF_syncframe.af_cts_params.af_triger == 1)) {
                        AF_syncframe.af_notified = CALLBACK_ERROR;
                        tmp_af_params.af_triger == 1;
                        memcpy(&syncAfParams, &tmp_af_params, sizeof(af_params_t));
                        tmp_af_params.focus_distance = 0;
                        tmp_af_params.lens_state = 0;
                        HAL_LOGV("AF control push %d",FrameVec.front());
                    } else {
                    HAL_LOGV("AF control push %d",FrameVec.front());
                    mAfCallBackQue.push_back(tmp_af_params);
                    }
                } else if(FrameVec.front() == mRequestInfoList[0].frame_number ) {
                    mSetting->s_setting[mOEMIf->getOemCameraId()].afMovingCount = 1;
                    if (mRequestInfoList[0].af_notified == CALLBACK_ERROR ||
                        mRequestInfoList[0].frame_number == 0) {
                        if (tmp_af_params.focus_distance !=
                            mRequestInfoList[0].af_cts_params.focus_distance) {
                                tmp_af_params.focus_distance = syncAfParams.focus_distance;
                                tmp_af_params.lens_state = syncAfParams.lens_state;
                            }
                        HAL_LOGV("AF control push %d",FrameVec.front());
                        mAfCallBackQue.push_back(tmp_af_params);
                    } else {
                        if (af_cts_callback_params->frame_number ==
                            mRequestInfoList[0].frame_number &&
                            mRequestInfoList[0].unSyncCount < 4) {
                            mAfCallBackQue.push_back(tmp_af_params);
                            HAL_LOGV("AF control push %d",FrameVec.front());
                        } else {
                            mAfCallBackQue.push_back(tmp_af_params);
                            HAL_LOGV("AF control push %d",FrameVec.front());
                        }
                        mRequestInfoList[0].unSyncCount++;
                        HAL_LOGV("AF control unSyncCount %d",mRequestInfoList[0].unSyncCount);
                    }
                    HAL_LOGD("report mRequestInfoList frame_number:%d, focus_distance: %f, sensitivity %d",
                        mRequestInfoList[0].af_cts_params.frame_number,
                        mRequestInfoList[0].af_cts_params.focus_distance,
                        mRequestInfoList[0].af_cts_params.focus_distance);
                } 
            }else {
                HAL_LOGV("AF control push %d",FrameVec.front());
                mAfCallBackQue.push_back(tmp_af_params);
            }
        }
        HAL_LOGV("AF_CALLBACK");
    } break;

    default:
        break;
    }

    {
        std::unique_lock <std::mutex> lck(mResultLock);
        if(pushResult()) {
            HAL_LOGD("mResultSignal push");
            //mResultSignal.signal();
            mResultSignal.notify_one();
        }
    }

    return NO_ERROR;
}

camera_metadata_t *SprdCamera3MetadataChannel::getMetadata(cmr_s32 frame_num) {
    {
        std::unique_lock <std::mutex> lck(mResultLock);
        //Mutex::Autolock lr(mResultLock);
        HAL_LOGD("E");
        if (mResultSignal.wait_for(lck, std::chrono:: milliseconds (120)) ==
          std::cv_status::timeout) {
            HAL_LOGE("timeout wait for mResultSignal");
        };
        HAL_LOGV("report frame_num %d mSyncResult size %d", frame_num, mSyncResult.size());
    }

    if (!FrameVec.empty()) {
        list<cmr_u32>::iterator lsItr =
        find_if(FrameVec.begin(), FrameVec.end(),
            [&](const cmr_u32 &frame_tmp) {
                return (frame_tmp == frame_num);
            });
        if (lsItr != FrameVec.end()) {
            FrameVec.erase(lsItr);
        }
    }
    Mutex::Autolock lr(mLock);
    if (!mSyncResult.empty()) {
        sync_result_t result = mSyncResult.front();
        mSyncResult.pop_front();
        /*update sync meta*/
        if (!mRequestInfoList.empty()) {
            struct isp_sync_params tmp_isp_sync_params;
            vector<struct isp_sync_params>::iterator lsItr =
                find_if(mRequestInfoList.begin(), mRequestInfoList.end(),
                    [&](const struct isp_sync_params &result_tmp) {
                        return (frame_num == result_tmp.ae_cts_params.frame_number)||
                               (frame_num == result_tmp.af_cts_params.frame_number);
                    });
            if(lsItr != mRequestInfoList.end()) {
                tmp_isp_sync_params = *lsItr;
                tmp_isp_sync_params.ae_cts_params.fps = 
                    (cmr_u32)tmp_isp_sync_params.ae_cts_params.exp_time/NSEC_PER_SEC;
                tmp_isp_sync_params.ae_cts_params.frame_duration = (cmr_s64) tmp_isp_sync_params.ae_cts_params.exp_time;
                HAL_LOGV("report AF framenum %d, focus_distance %f, lens_state %d", \
                        tmp_isp_sync_params.frame_number, \
                        tmp_isp_sync_params.af_cts_params.focus_distance, \
                        tmp_isp_sync_params.af_cts_params.lens_state);
                HAL_LOGV("report AE framenum %d, exp_time %lld, sensitivity %d",
                        tmp_isp_sync_params.frame_number,tmp_isp_sync_params.ae_cts_params.exp_time,
                        tmp_isp_sync_params.ae_cts_params.sensitivity);

                if(frame_num == 0)
                    memcpy(&syncAeParams, &tmp_isp_sync_params, sizeof(struct isp_sync_params));
                result.result = mSetting->reportMetadataToFramework \
                            (&tmp_isp_sync_params, result.result);
                mRequestInfoList.erase(lsItr);
            }
        }

        /*skip 0-2 frame*/
        if (mSetting->s_setting[mOEMIf->getOemCameraId()].controlInfo.ae_mode == 0 &&
            syncAeParams.frame_number == 0 &&
            syncAeParams.syncRequst != 0 &&
            frame_num < AESYNCNUM) {
            HAL_LOGD("skip 0-2 frame, report syncAeParams framenum %d, exp_time %lld, sensitivity %d",
                    syncAeParams.frame_number, syncAeParams.ae_cts_params.exp_time, syncAeParams.ae_cts_params.sensitivity);
            result.result = mSetting->reportMetadataToFramework \
                    (&syncAeParams, result.result);
        }

        /*update timstamp*/
        {
            camera_metadata_t *new_result = clone_camera_metadata(result.result);
            if (result.result) {
                free_camera_metadata(result.result);
                result.result = nullptr;
            }
            CameraMetadata *camMetadata = new CameraMetadata(new_result);

            if (camMetadata->exists(ANDROID_SENSOR_TIMESTAMP)) {
                int64_t tmp_timestamp = 
                    camMetadata->find(ANDROID_SENSOR_TIMESTAMP)
                    .data.i64[0];
                    std::map<uint32_t, cmr_s64>::iterator iter;
                    iter = mOEMIf->mFmtimeMap.find(frame_num);
                    if (iter != mOEMIf->mFmtimeMap.end()) {
                        HAL_LOGD("tmp_timestamp:%llx, FmtimeMap[%d]:%llx", tmp_timestamp,
                            frame_num, iter->second);
                        if (tmp_timestamp != iter->second)
                            tmp_timestamp = iter->second;
                        camMetadata->update(ANDROID_SENSOR_TIMESTAMP,
                                   &tmp_timestamp, 1);
                        mOEMIf->mFmtimeMap.erase(iter);
                    }
            }

            if (camMetadata->exists(ANDROID_LENS_FOCUS_DISTANCE)) {
                HAL_LOGV("frame [%d] report focus_distance %f",frame_num, camMetadata->find(ANDROID_LENS_FOCUS_DISTANCE)
                    .data.f[0]);
                HAL_LOGV("frame [%d] report ANDROID_LENS_STATE %d",frame_num, camMetadata->find(ANDROID_LENS_STATE)
                    .data.u8[0]);
            }

            /*update exif iso*/
            if (frame_num == mOEMIf->mPictureFrameNum && mOEMIf->mSprdAppmodeId == -1) {
                if (mOEMIf->mIsoMap.empty()) {
                    HAL_LOGD("wait for writing exif info");
                    usleep(40 * 1000);
                }
                if (camMetadata->exists(ANDROID_SENSOR_SENSITIVITY)) {
                    int iso_value = camMetadata->find(ANDROID_SENSOR_SENSITIVITY).data.i32[0];
                    std::map<uint32_t, cmr_u32>::iterator iter;
                    iter = mOEMIf->mIsoMap.find(frame_num);
                    if (iter != mOEMIf->mIsoMap.end()) {
                        HAL_LOGD("iso_value:%d, mOEMIf->mIsoMap[%d]:%d", iso_value,
                            frame_num, iter->second);
                        if(iso_value != iter->second && (iter->second != 0))
                            iso_value = iter->second;
                        camMetadata->update(ANDROID_SENSOR_SENSITIVITY,
                           &iso_value, 1);
                        mOEMIf->mIsoMap.erase(iter);
                    }
                }
                if (camMetadata->exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
                    cmr_s64 exp_value = camMetadata->find(ANDROID_SENSOR_EXPOSURE_TIME).data.i64[0];
                    std::map<uint32_t, cmr_s64>::iterator iter;
                    iter = mOEMIf->mExptimeMap.find(frame_num);
                    if (iter != mOEMIf->mExptimeMap.end()) {
                        HAL_LOGD("exp_value:%lld, mOEMIf->mIsoMap[%d]:%d", exp_value,
                            frame_num, iter->second);
                        if (exp_value != iter->second &&
                            iter->second!= 0)
                            exp_value = iter->second;
                        camMetadata->update(ANDROID_SENSOR_EXPOSURE_TIME,
                           &exp_value, 1);
                        mOEMIf->mExptimeMap.erase(iter);
                    }
                }
            }

            if (camMetadata->exists(ANDROID_FLASH_STATE) &&
                mOEMIf->getOemCameraId() != 1) {
                uint8_t valueU8 = camMetadata->find(ANDROID_FLASH_STATE).data.u8[0];
                std::map<uint32_t, uint8_t>::iterator iter;
                iter = mFlashMap.find(frame_num);
                if (iter != mFlashMap.end()) {
                    if (valueU8 != mFlashMap[frame_num]) {
                        HAL_LOGV("update flash state");
                        valueU8 = mFlashMap[frame_num];
                        camMetadata->update(ANDROID_FLASH_STATE, &valueU8, 1);
                        mFlashMap.erase(iter);
                    }
                 }
           }

            HAL_LOGV("frame [%d] AE_STATE ANDROID_CONTROL_AE_STATE=%d", frame_num,
                camMetadata->find(ANDROID_CONTROL_AE_STATE).data.u8[0]);
            HAL_LOGV("frame [%d] ANDROID_CONTROL_AF_STATE= %d", frame_num,
                camMetadata->find(ANDROID_CONTROL_AF_STATE).data.u8[0]);
            HAL_LOGV("exp_time ANDROID_SENSOR_EXPOSURE_TIME=%lld",
                camMetadata->find(ANDROID_SENSOR_EXPOSURE_TIME).data.i64[0]);
            HAL_LOGV("iso_value ANDROID_SENSOR_SENSITIVITY=%d",
                camMetadata->find(ANDROID_SENSOR_SENSITIVITY).data.i32[0]);

            result.result = camMetadata->release();
            delete camMetadata;
        }
        return result.result;
    }else {
        HAL_LOGE("AE or AF callback error, give default metadata %p",
            mSetting->translateLocalToFwMetadata());
        camera_metadata_t *tmp_result = mSetting->translateLocalToFwMetadata();;
        if (!mRequestInfoList.empty()) {
            struct isp_sync_params tmp_isp_sync_params;
            vector<struct isp_sync_params>::iterator lsItr =
                find_if(mRequestInfoList.begin(), mRequestInfoList.end(),
                    [&](const struct isp_sync_params &result_tmp) {
                        return (frame_num == result_tmp.ae_cts_params.frame_number)||
                               (frame_num == result_tmp.af_cts_params.frame_number);
                    });
            if (lsItr != mRequestInfoList.end()) {
                tmp_isp_sync_params = *lsItr;
                tmp_isp_sync_params.ae_cts_params.fps = 
                    (cmr_u32)tmp_isp_sync_params.ae_cts_params.exp_time/NSEC_PER_SEC;
                tmp_isp_sync_params.ae_cts_params.frame_duration = 
                                    (cmr_s64)tmp_isp_sync_params.ae_cts_params.exp_time;
                if (frame_num == 0)
                    memcpy(&syncAeParams, &tmp_isp_sync_params, sizeof(struct isp_sync_params));
                tmp_result = mSetting->reportMetadataToFramework \
                            (&tmp_isp_sync_params, tmp_result);
                mRequestInfoList.erase(lsItr);
            }
        }
        if (frame_num < AESYNCNUM &&
            syncAeParams.frame_number == 0 &&
            syncAeParams.syncRequst != 0) {
           tmp_result = mSetting->reportMetadataToFramework \
                    (&syncAeParams, tmp_result);
        }

        return tmp_result;
    }
    return nullptr;
}

int SprdCamera3MetadataChannel::HandleCbRoutine(void *params) {
    struct isp_sync_params *isp_params = (struct isp_sync_params *)params;
    cam_result_data_info_t result_info;
    memset(&result_info, 0, sizeof(result_info));
    result_info.is_urgent = true;
    return NO_ERROR;
}

int SprdCamera3MetadataChannel::start(uint32_t frame_number) {
    CONTROL_Tag controlInfo;
    SPRD_DEF_Tag *sprddefInfo;
    JPEG_Tag jpegInfo;
    STATISTICS_Tag statisticsInfo;
    int tag = 0;

    while ((tag = mSetting->popAndroidParaTag()) != -1) {
        switch (tag) {
        case ANDROID_CONTROL_AF_TRIGGER:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AF_TRIGGER);
            break;

        case ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER:
            HAL_LOGV("ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER");
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
            break;
        case ANDROID_SCALER_CROP_REGION:
            HAL_LOGV("SCALER_CROP_REGION");
            mOEMIf->setCameraConvertCropRegion();
            break;
        case ANDROID_CONTROL_CAPTURE_INTENT:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_CAPTURE_INTENT);
            break;
        case ANDROID_LENS_FOCAL_LENGTH:
            HAL_LOGV("ANDROID_LENS_FOCAL_LENGTH");
            mOEMIf->SetCameraParaTag(ANDROID_LENS_FOCAL_LENGTH);
            break;
        case ANDROID_JPEG_QUALITY:
            HAL_LOGV("ANDROID_JPEG_QUALITY");
            mOEMIf->SetCameraParaTag(ANDROID_JPEG_QUALITY);
            break;
        case ANDROID_JPEG_GPS_COORDINATES:
            HAL_LOGV("ANDROID_JPEG_GPS_COORDINATES");
            mOEMIf->SetJpegGpsInfo(true);
            break;
        case ANDROID_JPEG_GPS_PROCESSING_METHOD:
            HAL_LOGV("ANDROID_JPEG_GPS_PROCESSING_METHOD");
            break;
        case ANDROID_JPEG_GPS_TIMESTAMP:
            HAL_LOGD("ANDROID_JPEG_GPS_TIMESTAMP");
            break;
        case ANDROID_CONTROL_SCENE_MODE:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_SCENE_MODE);
            break;
        case ANDROID_CONTROL_EFFECT_MODE:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_EFFECT_MODE);
            break;
        case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
            break;
        case ANDROID_CONTROL_AWB_MODE:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AWB_MODE);
            break;
        case ANDROID_CONTROL_AWB_LOCK:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AWB_LOCK);
            break;
        case ANDROID_CONTROL_AE_MODE:
            HAL_LOGV("ANDROID_CONTROL_AE_MODE");
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_MODE);
            {
                struct isp_sync_params isp_params;
                memset(&isp_params, 0, sizeof(struct isp_sync_params));
                int ret = mSetting->getAeParams(&(isp_params.ae_cts_params));
                if (isp_params.ae_cts_params.is_push == true) {
                    mOEMIf->setCameraIspPara(SPRD_AE_PARAMS);
                }
            }
            break;
        case ANDROID_SENSOR_EXPOSURE_TIME:
            mOEMIf->SetCameraParaTag(ANDROID_SENSOR_EXPOSURE_TIME);
            break;
	    case ANDROID_SENSOR_SENSITIVITY:
	            HAL_LOGV("ANDROID_SENSOR_SENSITIVITY");
            mOEMIf->SetCameraParaTag(ANDROID_SENSOR_SENSITIVITY);
            break;
        case ANDROID_CONTROL_AE_ANTIBANDING_MODE:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
            break;
        case ANDROID_FLASH_MODE:
            HAL_LOGV("ANDROID_FLASH_MODE");
            mOEMIf->SetCameraParaTag(ANDROID_FLASH_MODE);
            break;
        case ANDROID_CONTROL_AF_MODE:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AF_MODE);
            HAL_LOGV("ANDROID_CONTROL_AF_MODE");
            break;
        case ANDROID_STATISTICS_FACE_DETECT_MODE:
            HAL_LOGV("FACE DECTION");
            sprddefInfo = mSetting->getSPRDDEFTagPTR();
            mSetting->getSTATISTICSTag(&statisticsInfo);
            if (statisticsInfo.face_detect_mode ==
                    ANDROID_STATISTICS_FACE_DETECT_MODE_OFF &&
                !isFaceBeautyOn(sprddefInfo)) {
                mOEMIf->faceDectect(0);
            } else
                mOEMIf->faceDectect(1);
            break;
        case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
            break;
        case ANDROID_CONTROL_AE_LOCK:
            HAL_LOGV("ANDROID_CONTROL_AE_LOCK");
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_LOCK);
            break;
        case ANDROID_CONTROL_AE_REGIONS:
            HAL_LOGV("ANDROID_CONTROL_AE_REGIONS");
            mOEMIf->SetCameraParaTag(ANDROID_CONTROL_AE_REGIONS);
            break;
        case ANDROID_LENS_FOCUS_DISTANCE:
            if (mSetting->sprd_app_id != 1 && !mSetting->mMultiCameraMode) {
                mOEMIf->SetCameraParaTag(ANDROID_LENS_FOCUS_DISTANCE);
                mOEMIf->setCameraIspPara(SPRD_AF_PARAMS);
            } else if(mSetting->sprd_app_id == 1 || (mSetting->mMultiCameraMode && (mSetting->sprd_app_id == -1))) {
                mOEMIf->SetCameraParaTag(ANDROID_LENS_FOCUS_DISTANCE);
            }
            break;

        default:
            HAL_LOGV("other tag");
            break;
        }
    }
    while ((tag = mSetting->popSprdParaTag()) != -1) {
        switch (tag) {
        case ANDROID_SPRD_BRIGHTNESS:
            HAL_LOGV("ANDROID_SPRD_BRIGHTNESS");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_BRIGHTNESS);
            break;
        case ANDROID_SPRD_AI_SCENE_ENABLED:
            HAL_LOGV("ANDROID_SPRD_AI_SCENE_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_AI_SCENE_ENABLED);
            break;
        case ANDROID_SPRD_CONTRAST:
            HAL_LOGV("contrast");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_CONTRAST);
            break;
        case ANDROID_SPRD_SATURATION:
            HAL_LOGV("ANDROID_SPRD_SATURATION");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SATURATION);
            break;
        case ANDROID_SPRD_CAPTURE_MODE:
            HAL_LOGV("ANDROID_SPRD_CAPTURE_MODE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_CAPTURE_MODE);
            break;
        case ANDROID_SPRD_SENSOR_ORIENTATION:
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SENSOR_ORIENTATION);
            break;
        case ANDROID_SPRD_SENSOR_ROTATION:
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SENSOR_ROTATION);
            break;
        case ANDROID_SPRD_UCAM_SKIN_LEVEL:
            HAL_LOGV("ANDROID_SPRD_UCAM_SKIN_LEVEL");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_UCAM_SKIN_LEVEL);
            break;
        case ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR:
            HAL_LOGV("ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_CONTROL_FRONT_CAMERA_MIRROR);
            break;
        case ANDROID_SPRD_METERING_MODE:
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_METERING_MODE);
            break;
        /*case ANDROID_SPRD_ISO:
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_ISO);
            break;*/
        case ANDROID_SPRD_ZSL_ENABLED:
            HAL_LOGV("ANDROID_SPRD_ZSL_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_ZSL_ENABLED);
            break;
        case ANDROID_SPRD_SLOW_MOTION:
            HAL_LOGV("ANDROID_SPRD_SLOW_MOTION");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SLOW_MOTION);
            break;
        case ANDROID_SPRD_EIS_ENABLED:
            HAL_LOGV("ANDROID_SPRD_EIS_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_EIS_ENABLED);
            break;
        case ANDROID_SPRD_CONTROL_REFOCUS_ENABLE:
            HAL_LOGV("ANDROID_SPRD_CONTROL_REFOCUS_ENABLE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_CONTROL_REFOCUS_ENABLE);
            break;
        case ANDROID_SPRD_SET_TOUCH_INFO:
            HAL_LOGV("ANDROID_SPRD_SET_TOUCH_INFO");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SET_TOUCH_INFO);
            break;
        case ANDROID_SPRD_PIPVIV_ENABLED:
            HAL_LOGV("ANDROID_SPRD_PIPVIV_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_PIPVIV_ENABLED);
            break;
        case ANDROID_SPRD_3DCALIBRATION_ENABLED: /**add for 3d calibration
                                                    update params begin*/
            HAL_LOGV("ANDROID_SPRD_3DCALIBRATION_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_3DCALIBRATION_ENABLED);
            break; /**add for 3d calibration update params end*/
        case ANDROID_SPRD_3DNR_ENABLED:
            HAL_LOGV("ANDROID_SPRD_3DNR_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_3DNR_ENABLED);
            break;
        case ANDROID_SPRD_FIXED_FPS_ENABLED:
            HAL_LOGV("ANDROID_SPRD_FIXED_FPS_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_FIXED_FPS_ENABLED);
            break;
        case ANDROID_SPRD_APP_MODE_ID:
            HAL_LOGV("ANDROID_SPRD_APP_MODE_ID");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_APP_MODE_ID);
            break;
        case ANDROID_SPRD_FILTER_TYPE:
            HAL_LOGV("ANDROID_SPRD_FILTER_TYPE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_FILTER_TYPE);
            break;
        case ANDROID_SPRD_AUTO_HDR_ENABLED:
            HAL_LOGV("ANDROID_SPRD_AUTO_HDR_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_AUTO_HDR_ENABLED);
            break;
        case ANDROID_SPRD_DEVICE_ORIENTATION:
            HAL_LOGV("ANDROID_SPRD_DEVICE_ORIENTATION");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_DEVICE_ORIENTATION);
            break;
        case ANDROID_SPRD_CALIBRATION_DIST:
            HAL_LOGV("ANDROID_SPRD_CALIBRATION_DIST");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_CALIBRATION_DIST);
            break;
        case ANDROID_SPRD_CALIBRATION_OTP_DATA:
            HAL_LOGV("ANDROID_SPRD_CALIBRATION_OTP_DATA");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_CALIBRATION_OTP_DATA);
            break;
        case ANDROID_SPRD_SET_VERIFICATION_FLAG:
            HAL_LOGV("ANDROID_SPRD_SET_VERIFICATION_FLAG");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SET_VERIFICATION_FLAG);
            break;
        case ANDROID_SPRD_AUTOCHASING_REGION:
            HAL_LOGV("ANDROID_SPRD_AUTOCHASING_REGION");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_AUTOCHASING_REGION);
            break;
        case ANDROID_SPRD_BLUR_F_NUMBER:
            HAL_LOGV("ANDROID_SPRD_BLUR_F_NUMBER");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_BLUR_F_NUMBER);
            break;
        case ANDROID_SPRD_FLASH_LCD_MODE:
            HAL_LOGV("ANDROID_SPRD_FLASH_LCD_MODE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_FLASH_LCD_MODE);
            break;
        case ANDROID_SPRD_AUTO_3DNR_ENABLED:
            HAL_LOGV("ANDROID_SPRD_AUTO_3DNR_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_AUTO_3DNR_ENABLED);
            break;
        case ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE:
            HAL_LOGV("ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_FACE_ATTRIBUTES_ENABLE);
            break;
        case ANDROID_SPRD_TOUCH_INFO:
            HAL_LOGV("ANDROID_SPRD_TOUCH_INFO");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_TOUCH_INFO);
            break;
        case ANDROID_SPRD_AUTOCHASING_REGION_ENABLE:
            HAL_LOGV("ANDROID_SPRD_AUTOCHASING_REGION_ENABLE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_AUTOCHASING_REGION_ENABLE);
            break;
        case ANDROID_SPRD_LOGOWATERMARK_ENABLED:
            HAL_LOGV("ANDROID_SPRD_LOGOWATERMARK_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_LOGOWATERMARK_ENABLED);
            break;
        case ANDROID_SPRD_TIMEWATERMARK_ENABLED:
            HAL_LOGV("ANDROID_SPRD_TIMEWATERMARK_ENABLED");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_TIMEWATERMARK_ENABLED);
            break;
	case ANDROID_SPRD_SMILE_CAPTURE:
            HAL_LOGV("ANDROID_SPRD_SMILE_CAPTURE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SMILE_CAPTURE);
            break;
        case ANDROID_SPRD_SUPER_MACROPHOTO_ENABLE:
            HAL_LOGV("ANDROID_SPRD_SUPER_MACROPHOTO_ENABLE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SUPER_MACROPHOTO_ENABLE);
            break;
        case ANDROID_SPRD_SMILE_CAPTURE_ENABLE:
            HAL_LOGV("ANDROID_SPRD_SMILE_CAPTURE_ENABLE");
            mOEMIf->SetCameraParaTag(ANDROID_SPRD_SMILE_CAPTURE_ENABLE);
            break;
        default:
            HAL_LOGV("other tag");
            break;
        }
    }
    HAL_LOGV("set parameter out");
    return 0;
}

int SprdCamera3MetadataChannel::stop(uint32_t frame_number) {
    CONTROL_Tag controlInfo;
    int i = 0;
    HAL_LOGD("E");
    mSetting->getCONTROLTag(&controlInfo);
    if ((controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_START) ||
        (controlInfo.af_trigger == ANDROID_CONTROL_AF_TRIGGER_IDLE))
        mOEMIf->cancelAutoFocus();
    HAL_LOGV("X");
    ReSetFirstMeta = true;

    i = FrameVec.size();

    while(!FrameVec.empty()) {
        std::unique_lock <std::mutex> lck(mResultLock);
        HAL_LOGD("flush mResultSignal push");
        mResultSignal.notify_one();
        if(i-- <= 0)
            break;
    }

    return 0;
}

void SprdCamera3MetadataChannel::clear(){
    Mutex::Autolock lr(mLock);
    FrameVec.clear();
    mAeCallBackQue.clear();
    mAfCallBackQue.clear();
    mSyncResult.clear();
    mRequestInfoList.clear();
}
}; // namespace sprdcamera
