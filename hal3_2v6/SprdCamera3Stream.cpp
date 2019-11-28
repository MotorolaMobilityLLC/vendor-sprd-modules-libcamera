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

#define LOG_TAG "SprdCamera3Stream"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <utils/Errors.h>
#include "SprdCamera3HWI.h"
#include "SprdCamera3Stream.h"
#include "SprdCamera3Channel.h"
#include "gralloc_public.h"

using namespace android;

namespace sprdcamera {

SprdCamera3Stream::SprdCamera3Stream(camera3_stream_t *new_stream,
                                     camera_dimension_t dim,
                                     camera_stream_type_t type,
                                     void *userdata) {
    mCameraStream = new_stream;
    mWidth = dim.width;
    mHeight = dim.height;
    mStreamType = type;
    mUserChan = userdata;
    mBuffNum = 0;
    mIsUltraWideMode = 0;
    mHandledFrameNum = 0;

    mMemory = new SprdCamera3GrallocMemory();
    if (NULL == mMemory) {
        HAL_LOGE("object NULL");
    }
}

SprdCamera3Stream::~SprdCamera3Stream() {
    if (mMemory) {
        delete mMemory;
        mMemory = NULL;
    }
}

int SprdCamera3Stream::registerBuffers(uint32_t num_buffers,
                                       buffer_handle_t **buffers) {
    return NO_ERROR;
}

int SprdCamera3Stream::getStreamType(camera_stream_type_t *stream_type) {
    *stream_type = mStreamType;

    return NO_ERROR;
}

int SprdCamera3Stream::getStreamInfo(camera3_stream_t **stream) {
    *stream = mCameraStream;

    return NO_ERROR;
}

int SprdCamera3Stream::getStreamSize(int32_t *width, int32_t *height) {
    *width = mWidth;
    *height = mHeight;

    return NO_ERROR;
}

int SprdCamera3Stream::buffDoneQ2(uint32_t frameNumber,
                                  buffer_handle_t *buffer) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    hal_buff_list_t *buff_hal = new hal_buff_list_t;

    hal_mem_info_t *buf_mem_info = &(buff_hal->mem_info);
    if (buff_hal == NULL) {
        HAL_LOGE("ERROR: buff_hal is %p", buff_hal);
        return BAD_VALUE;
    }

    if (mMemory == NULL) {
        HAL_LOGE("mMemory is NULL");
        delete buff_hal;
        return BAD_VALUE;
    }

    buff_hal->buffer_handle = buffer;
    ret = mMemory->map2(buffer, buf_mem_info);
    if (ret != NO_ERROR) {
        HAL_LOGE("buffer queue Done Q buffer(%p) error", buffer);
        mBuffNum++;
        buff_hal->buffer_handle = buffer;
        buff_hal->frame_number = frameNumber;
        buf_mem_info->fd = 0;
        buf_mem_info->addr_vir = NULL;
        buf_mem_info->addr_phy = NULL;
        mBufferList.add(buff_hal);
        ret = NO_ERROR;
    } else {
        HAL_LOGV("addr_phy = %p, addr_vir = %p, fd=0x%x, size = %d, mStreamType = %d",
                 buf_mem_info->addr_phy, buf_mem_info->addr_vir, buf_mem_info->fd,
                 buf_mem_info->size, mStreamType);
        mBuffNum++;
        buff_hal->buffer_handle = buffer;
        buff_hal->frame_number = frameNumber;
        HAL_LOGV("frame_number %d, handle %p, mStreamType %d",
                 buff_hal->frame_number, buffer, mStreamType);
        mBufferList.add(buff_hal);
    }

    return ret;
}

int SprdCamera3Stream::buffDoneQ(uint32_t frameNumber,
                                 buffer_handle_t *buffer) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    hal_buff_list_t *buff_hal = new hal_buff_list_t;

    hal_mem_info_t *buf_mem_info = &(buff_hal->mem_info);
    if (buff_hal == NULL) {
        HAL_LOGE("ERROR: buff_hal is %p", buff_hal);
        return BAD_VALUE;
    }

    if (mMemory == NULL) {
        HAL_LOGE("mMemory is NULL");
        delete buff_hal;
        return BAD_VALUE;
    }

    buff_hal->buffer_handle = buffer;
    if (mIsUltraWideMode == 1) {
        ret = mMemory->map3(buffer, buf_mem_info);
    } else {
        ret = mMemory->map(buffer, buf_mem_info);
    }
    if (ret != NO_ERROR) {
        HAL_LOGE("buffer queue Done Q buffer(%p) error", buffer);
        mBuffNum++;
        buff_hal->buffer_handle = buffer;
        buff_hal->frame_number = frameNumber;
        buf_mem_info->fd = 0;
        buf_mem_info->addr_vir = NULL;
        buf_mem_info->addr_phy = NULL;
        mBufferList.add(buff_hal);
        ret = NO_ERROR;
    } else {
        HAL_LOGV("addr_phy = %p, addr_vir = %p, fd=0x%x, size = 0x%x"
                 ", mStreamType = %d, fmt=%d,width=%d, height=%d",
                 buf_mem_info->addr_phy, buf_mem_info->addr_vir,buf_mem_info->fd,
                 buf_mem_info->size, mStreamType, buf_mem_info->format,
                 buf_mem_info->width,buf_mem_info->height);
        mBuffNum++;
        buff_hal->buffer_handle = buffer;
        buff_hal->frame_number = frameNumber;
        HAL_LOGV("frame_number %d, handle %p, mStreamType %d",
                 buff_hal->frame_number, buffer, mStreamType);
        mBufferList.add(buff_hal);
    }

    return ret;
}

int SprdCamera3Stream::buffDoneDQ(uint32_t frameNumber,
                                  buffer_handle_t **buffer) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    Vector<hal_buff_list_t *>::iterator iter;

    if (mMemory == NULL) {
        HAL_LOGE("mMemory is NULL");
        return BAD_VALUE;
    }

    for (iter = mBufferList.begin(); iter != mBufferList.end(); iter++) {
        if ((*iter) && (*iter)->frame_number == frameNumber) {
            *buffer = (*iter)->buffer_handle;
            if (mIsUltraWideMode == 1 && mStreamType == 1) {
                mMemory->unmap3((*iter)->buffer_handle, &((*iter)->mem_info));
            } else {
                mMemory->unmap((*iter)->buffer_handle, &((*iter)->mem_info));
            }

            HAL_LOGV("frame_number %d, mStreamType %d", (*iter)->frame_number,
                     mStreamType);
            delete *iter;
            mBufferList.erase(iter);

            return ret;
        }
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::buffFirstDoneDQ(uint32_t *frameNumber,
                                       buffer_handle_t **buffer) {
    Mutex::Autolock l(mLock);
    Vector<hal_buff_list_t *>::iterator iter;

    if (mMemory == NULL) {
        HAL_LOGE("mMemory is NULL");
        return BAD_VALUE;
    }

    if (mBufferList.size()) {
        iter = mBufferList.begin();

        *buffer = (*iter)->buffer_handle;
        *frameNumber = (uint32_t)((*iter)->frame_number);
        mMemory->unmap((*iter)->buffer_handle, &((*iter)->mem_info));

        HAL_LOGV(
            "buffer queue First Done DQ frame_number = %d, mStreamType = %d",
            (*iter)->frame_number, mStreamType);

        delete *iter;
        mBufferList.erase(iter);

        return NO_ERROR;
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::getHeapSize(uint32_t *mm_heap_size) {
    Vector<hal_buff_list_t *>::iterator iter = mBufferList.begin();

    if ((*iter) == NULL) {
        HAL_LOGE("stream has no buffer");
        return BAD_VALUE;
    }

    *mm_heap_size = ADP_WIDTH(*((*iter)->buffer_handle));

    return NO_ERROR;
}

int SprdCamera3Stream::getHeapNum(uint32_t *mm_heap_num) {
    //*mm_heap_num = mBufferTable.size();

    return NO_ERROR;
}

int SprdCamera3Stream::getRegisterBuffPhyAddr(cmr_uint *buff_phy) {
    /*cmr_uint* buff_phy_p = buff_phy;
    for (Vector<hal_buffer_idex_table_t*>::iterator
    iter=mBufferTable.begin();iter!=mBufferTable.end();iter++)
    {
            if(*iter){
                    *buff_phy_p = (cmr_uint)((*iter)->mem_info.addr_phy);
                    buff_phy_p++;
            }
    }*/

    return NO_ERROR;
}

int SprdCamera3Stream::getRegisterBuffVirAddr(cmr_uint *buff_vir) {
    /*cmr_uint* buff_vir_p = buff_vir;
    for (Vector<hal_buffer_idex_table_t*>::iterator
    iter=mBufferTable.begin();iter!=mBufferTable.end();iter++)
    {
            if(*iter){
                    *buff_vir_p = (cmr_uint)((*iter)->mem_info.addr_vir);
                    buff_vir_p++;
            }
    }*/

    return NO_ERROR;
}

int SprdCamera3Stream::getQBufListNum(int32_t *buff_num) {
    Mutex::Autolock l(mLock);
    *buff_num = mBufferList.size();
    return NO_ERROR;
}

int SprdCamera3Stream::getRegisterBufListNum(int32_t *buff_num) {
    *buff_num = mBuffNum;
    return NO_ERROR;
}

int SprdCamera3Stream::getQBuffFirstVir(cmr_uint *addr_vir) {
    Mutex::Autolock l(mLock);
    Vector<hal_buff_list_t *>::iterator iter;

    if (mBufferList.size()) {
        iter = mBufferList.begin();

        *addr_vir = (cmr_uint)((*iter)->mem_info.addr_vir);
        return NO_ERROR;
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::getQBuffFirstPhy(cmr_uint *addr_phy) {
    Mutex::Autolock l(mLock);
    Vector<hal_buff_list_t *>::iterator iter;
    if (mBufferList.size()) {
        iter = mBufferList.begin();
        *addr_phy = (cmr_uint)((*iter)->mem_info.addr_phy);
        return NO_ERROR;
    }
    return BAD_VALUE;
}

int SprdCamera3Stream::getQBuffFirstFd(cmr_s32 *fd) {
    Mutex::Autolock l(mLock);
    Vector<hal_buff_list_t *>::iterator iter;
    if (mBufferList.size()) {
        iter = mBufferList.begin();
        *fd = (cmr_s32)((*iter)->mem_info.fd);
        return NO_ERROR;
    }
    return BAD_VALUE;
}

int SprdCamera3Stream::getQBuffFirstNum(uint32_t *frameNumber) {
    Mutex::Autolock l(mLock);
    Vector<hal_buff_list_t *>::iterator iter;

    if (mBufferList.size()) {
        iter = mBufferList.begin();

        *frameNumber = (uint32_t)((*iter)->frame_number);
        return NO_ERROR;
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::getQBufAddrForNum(uint32_t frameNumber,
                                         cmr_uint *addr_vir, cmr_uint *addr_phy,
                                         cmr_s32 *fd) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    Vector<hal_buff_list_t *>::iterator iter;

    for (iter = mBufferList.begin(); iter != mBufferList.end(); iter++) {
        if ((*iter) && (*iter)->frame_number == frameNumber) {
            *addr_vir = (cmr_uint)((*iter)->mem_info.addr_vir);
            *addr_phy = (cmr_uint)((*iter)->mem_info.addr_phy);
            *fd = (cmr_s32)((*iter)->mem_info.fd);
            return ret;
        }
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::getQBufHandleForNum(uint32_t frameNumber,
                                           buffer_handle_t **buff) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    Vector<hal_buff_list_t *>::iterator iter;

    for (iter = mBufferList.begin(); iter != mBufferList.end(); iter++) {
        if ((*iter) && (*iter)->frame_number == frameNumber) {
            *buff = (*iter)->buffer_handle;
            return ret;
        }
    }
    return BAD_VALUE;
}
int SprdCamera3Stream::getQBufHandle(cmr_uint addr_vir, cmr_uint addr_phy,
                                     cmr_s32 fd, buffer_handle_t **buff,
                                     void **prevGraphBuffer) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    Vector<hal_buff_list_t *>::iterator iter;

    for (iter = mBufferList.begin(); iter != mBufferList.end(); iter++) {
        if (*iter) {
            hal_mem_info_t *mem_info = &((*iter)->mem_info);
            if ((cmr_uint)mem_info->addr_vir == addr_vir) {
                *buff = (*iter)->buffer_handle;
                *prevGraphBuffer = mem_info->bufferPtr;
                return ret;
            }
        }
    }
    return BAD_VALUE;
}

int SprdCamera3Stream::getQBufNumForVir(cmr_uint addr_vir,
                                        uint32_t *frameNumber) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    Vector<hal_buff_list_t *>::iterator iter;
    for (iter = mBufferList.begin(); iter != mBufferList.end(); iter++) {
        if ((*iter) && (*iter)->mem_info.addr_vir == (void *)addr_vir) {
            *frameNumber = (*iter)->frame_number;
            return ret;
        }
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::getQBufForHandle(buffer_handle_t *buff,
                                        cmr_uint *addr_vir, cmr_uint *addr_phy,
                                        cmr_s32 *fd) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    Vector<hal_buff_list_t *>::iterator iter;

    for (iter = mBufferList.begin(); iter != mBufferList.end(); iter++) {
        if ((*iter) && (*iter)->buffer_handle == buff) {
            *addr_vir = (cmr_uint)((*iter)->mem_info.addr_vir);
            *addr_phy = (cmr_uint)((*iter)->mem_info.addr_phy);
            *fd = (*iter)->mem_info.fd;
            return ret;
        }
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::getQBufInfoForHandle(buffer_handle_t *buff,
                                            cam_buffer_info_t *bufInfo) {
    Mutex::Autolock l(mLock);
    int ret = NO_ERROR;
    Vector<hal_buff_list_t *>::iterator iter;

    for (iter = mBufferList.begin(); iter != mBufferList.end(); iter++) {
        if ((*iter) && (*iter)->buffer_handle == buff) {
            bufInfo->fd = (*iter)->mem_info.fd;
            bufInfo->size = (*iter)->mem_info.size;
            bufInfo->addr_phy = (*iter)->mem_info.addr_phy;
            bufInfo->addr_vir = (*iter)->mem_info.addr_vir;
            bufInfo->width = (*iter)->mem_info.width;
            bufInfo->height = (*iter)->mem_info.height;
            bufInfo->format = (*iter)->mem_info.format;
            bufInfo->frame_number = (*iter)->frame_number;
            return ret;
        }
    }

    return BAD_VALUE;
}

int SprdCamera3Stream::getQBufFirstBuf(cam_buffer_info_t *bufInfo) {
    Mutex::Autolock l(mLock);
    Vector<hal_buff_list_t *>::iterator iter;

    if (mBufferList.size()) {
        iter = mBufferList.begin();

        bufInfo->fd = (*iter)->mem_info.fd;
        bufInfo->size = (*iter)->mem_info.size;
        bufInfo->addr_phy = (*iter)->mem_info.addr_phy;
        bufInfo->addr_vir = (*iter)->mem_info.addr_vir;
        bufInfo->width = (*iter)->mem_info.width;
        bufInfo->height = (*iter)->mem_info.height;
        bufInfo->format = (*iter)->mem_info.format;
        return NO_ERROR;
    }

    return BAD_VALUE;
}

uint32_t SprdCamera3Stream::getHandledFrameNum() { return mHandledFrameNum; }

void SprdCamera3Stream::setHandledFrameNum(uint32_t frameNum) {
    mHandledFrameNum = frameNum;
}

void SprdCamera3Stream::setUltraWideMode(bool ultra_wide_mode) {
    mIsUltraWideMode = ultra_wide_mode;
    HAL_LOGD("mIsUltraWideMode:%d", mIsUltraWideMode);
}

}; // namespace sprdcamera
