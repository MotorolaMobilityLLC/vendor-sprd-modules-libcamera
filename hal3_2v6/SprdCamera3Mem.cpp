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

#define LOG_TAG "Cam3Mem"

#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include "MemIon.h"
#include <binder/MemoryHeapBase.h>
#include <sprd_ion.h>
#include "SprdCamera3HALHeader.h"
#include "SprdCamera3Mem.h"
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include "gralloc_public.h"

using namespace android;

namespace sprdcamera {

/*===========================================================================
 * FUNCTION   : SprdCamera3Memory
 *
 * DESCRIPTION: default constructor of SprdCamera3Memory
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Memory::SprdCamera3Memory() {}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3Memory
 *
 * DESCRIPTION: deconstructor of SprdCamera3Memory
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Memory::~SprdCamera3Memory() {}

int SprdCamera3Memory::getUsage(int stream_type, cmr_uint &usage) {
    switch (stream_type) {
    case CAMERA3_STREAM_INPUT:
        usage = GRALLOC_USAGE_SW_READ_OFTEN;
        break;

    case CAMERA3_STREAM_BIDIRECTIONAL:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;

    case CAMERA3_STREAM_OUTPUT:
        usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
    }

    return 0;
}

/*===========================================================================
 * FUNCTION   : cacheOpsInternal
 *
 * DESCRIPTION: ion related memory cache operations
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *   @cmd     : cache ops command
 *   @vaddr   : ptr to the virtual address
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Memory::cacheOpsInternal(int index, unsigned int cmd,
                                        void *vaddr) {
    int ret = OK;

    return ret;
}

/*===========================================================================
 * FUNCTION   : SprdCamera3HeapMemory
 *
 * DESCRIPTION: constructor of SprdCamera3HeapMemory for ion memory used
 *internally in HAL
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
SprdCamera3HeapMemory::SprdCamera3HeapMemory() : SprdCamera3Memory() {}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3HeapMemory
 *
 * DESCRIPTION: deconstructor of SprdCamera3HeapMemory
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
SprdCamera3HeapMemory::~SprdCamera3HeapMemory() {}

/*===========================================================================
 * FUNCTION   : cacheOps
 *
 * DESCRIPTION: ion related memory cache operations
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *   @cmd     : cache ops command
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3HeapMemory::cacheOps(int index, unsigned int cmd) {
    return cacheOpsInternal(index, cmd, NULL);
}

/*===========================================================================
 * FUNCTION   : SprdCamera3GrallocMemory
 *
 * DESCRIPTION: constructor of SprdCamera3GrallocMemory
 *              preview stream buffers are allocated from gralloc native_windoe
 *
 * PARAMETERS :
 *   @getMemory : camera memory request ops table
 *
 * RETURN     : none
 *==========================================================================*/
SprdCamera3GrallocMemory::SprdCamera3GrallocMemory() : SprdCamera3Memory() {}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3GrallocMemory
 *
 * DESCRIPTION: deconstructor of SprdCamera3GrallocMemory
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
SprdCamera3GrallocMemory::~SprdCamera3GrallocMemory() {}

/*===========================================================================
 * FUNCTION   : registerBuffers
 *
 * DESCRIPTION: register frameworks-allocated gralloc buffer_handle_t
 *
 * PARAMETERS :
 *   @num_buffer : number of buffers to be registered
 *   @buffers    : array of buffer_handle_t pointers
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3GrallocMemory::map(buffer_handle_t *buffer_handle,
                                  hal_mem_info_t *mem_info) {
    int ret = NO_ERROR;

    if (NULL == mem_info || NULL == buffer_handle) {
        HAL_LOGE("Param invalid handle=%p, info=%p", buffer_handle, mem_info);
        return -EINVAL;
    }
    HAL_LOGV("E");

    int width = ADP_WIDTH(*buffer_handle);
    int height = ADP_HEIGHT(*buffer_handle);
    int format = ADP_FORMAT(*buffer_handle);
    android_ycbcr ycbcr;
    Rect bounds(width, height);
    void *vaddr = NULL;
    int usage;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    bzero((void *)&ycbcr, sizeof(ycbcr));
    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle, usage,
                               bounds, &ycbcr);
        if (ret != NO_ERROR) {
            HAL_LOGV("lockcbcr.onQueueFilled, mapper.lock failed try "
                     "lockycbcr. %p, ret %d",
                     *buffer_handle, ret);
            ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                              bounds, &vaddr);
            if (ret != NO_ERROR) {
                HAL_LOGE("locky.onQueueFilled, mapper.lock fail %p, ret %d",
                         *buffer_handle, ret);
            } else {
                mem_info->addr_vir = vaddr;
            }
        } else {
            mem_info->addr_vir = ycbcr.y;
        }
    } else {
        ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                          bounds, &vaddr);
        if (ret != NO_ERROR) {
            HAL_LOGV("lockonQueueFilled, mapper.lock failed try lockycbcr. %p, "
                     "ret %d",
                     *buffer_handle, ret);
            ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle,
                                   usage, bounds, &ycbcr);
            if (ret != NO_ERROR) {
                HAL_LOGE("lockycbcr.onQueueFilled, mapper.lock fail %p, ret %d",
                         *buffer_handle, ret);
            } else {
                mem_info->addr_vir = ycbcr.y;
            }
        } else {
            mem_info->addr_vir = vaddr;
        }
    }
    mem_info->fd = ADP_BUFFD(*buffer_handle);
    // mem_info->addr_phy is offset, always set to 0 for yaddr
    mem_info->addr_phy = (void *)0;
    mem_info->size = ADP_BUFSIZE(*buffer_handle);
    mem_info->width = ADP_WIDTH(*buffer_handle);
    mem_info->height = ADP_HEIGHT(*buffer_handle);
    mem_info->format= ADP_FORMAT(*buffer_handle);
    HAL_LOGV("fd = 0x%x, addr_phy offset = %p, addr_vir = %p,"
             " buf size = 0x%x, width = %d, height = %d, fmt = %d",
             mem_info->fd, mem_info->addr_phy, mem_info->addr_vir,
             mem_info->size, mem_info->width, mem_info->height, mem_info->format);

err_out:
    return ret;
}

int SprdCamera3GrallocMemory::map2(buffer_handle_t *buffer_handle,
                                   hal_mem_info_t *mem_info) {
    int ret = NO_ERROR;

    if (NULL == mem_info || NULL == buffer_handle) {
        HAL_LOGE("Param invalid handle=%p, info=%p", buffer_handle, mem_info);
        return -EINVAL;
    }
    HAL_LOGV("E");

    int width = ADP_WIDTH(*buffer_handle);
    int height = ADP_HEIGHT(*buffer_handle);
    int format = ADP_FORMAT(*buffer_handle);
    android_ycbcr ycbcr;
    Rect bounds(width, height);
    void *vaddr = NULL;
    int usage;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    bzero((void *)&ycbcr, sizeof(ycbcr));
    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle, usage,
                               bounds, &ycbcr);
        if (ret != NO_ERROR) {
            HAL_LOGV("lockcbcr.onQueueFilled, mapper.lock failed try "
                     "lockycbcr. %p, ret %d",
                     *buffer_handle, ret);
            ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                              bounds, &vaddr);
            if (ret != NO_ERROR) {
                HAL_LOGE("locky.onQueueFilled, mapper.lock fail %p, ret %d",
                         *buffer_handle, ret);
            } else {
                mem_info->addr_vir = vaddr;
            }
        } else {
            mem_info->addr_vir = ycbcr.y;
        }
    } else {
        ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                          bounds, &vaddr);
        if (ret != NO_ERROR) {
            HAL_LOGV("lockonQueueFilled, mapper.lock failed try lockycbcr. %p, "
                     "ret %d",
                     *buffer_handle, ret);
            ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle,
                                   usage, bounds, &ycbcr);
            if (ret != NO_ERROR) {
                HAL_LOGE("lockycbcr.onQueueFilled, mapper.lock fail %p, ret %d",
                         *buffer_handle, ret);
            } else {
                mem_info->addr_vir = ycbcr.y;
            }
        } else {
            mem_info->addr_vir = vaddr;
        }
    }
    mem_info->fd = ADP_BUFFD(*buffer_handle);
    // mem_info->addr_phy is offset, always set to 0 for yaddr
    mem_info->addr_phy = (void *)0;
    mem_info->size = ADP_BUFSIZE(*buffer_handle);
    mem_info->width = ADP_WIDTH(*buffer_handle);
    mem_info->height = ADP_HEIGHT(*buffer_handle);
    mem_info->format= ADP_FORMAT(*buffer_handle);
    HAL_LOGV("fd = 0x%x, addr_phy offset = %p, addr_vir = %p,"
             " buf size = %zu, width = %d, height = %d, fmt = %d",
             mem_info->fd, mem_info->addr_phy, mem_info->addr_vir,
             mem_info->size, mem_info->width, mem_info->height, mem_info->format);

err_out:
    return ret;
}

/*===========================================================================
 * FUNCTION   : unregisterBuffers
 *
 * DESCRIPTION: unregister buffers
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3GrallocMemory::unmap(buffer_handle_t *buffer_handle,
                                    hal_mem_info_t *mem_info) {
    int ret = NO_ERROR;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    ret = mapper.unlock((const native_handle_t *)*buffer_handle);
    if (ret != NO_ERROR) {
        ALOGE("onQueueFilled, mapper.unlock fail %p", *buffer_handle);
    }
    return ret;
}
/*===========================================================================
 * FUNCTION   : map3
 *
 * DESCRIPTION: for ultrawide map
 *
 * PARAMETERS :
 *   @num_buffer : number of buffers to be registered
 *   @buffers    : array of buffer_handle_t pointers
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3GrallocMemory::map3(buffer_handle_t *buffer_handle,
                                   hal_mem_info_t *mem_info) {
    int ret = NO_ERROR;

    if (NULL == mem_info || NULL == buffer_handle) {
        HAL_LOGE("Param invalid handle=%p, info=%p", buffer_handle, mem_info);
        return -EINVAL;
    }

    int width = ADP_WIDTH(*buffer_handle);
    int height = ADP_HEIGHT(*buffer_handle);
    int format = ADP_FORMAT(*buffer_handle);
    android_ycbcr ycbcr;
    Rect bounds(width, height);
    void *vaddr = NULL;
    int usage;

    native_handle_t *native_handle = (native_handle_t *)(*buffer_handle);
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;

    mem_info->pbuffer = new GraphicBuffer(
        native_handle, GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, width,
        height, HAL_PIXEL_FORMAT_YCrCb_420_SP, 1, yuvTextUsage, width);
    mem_info->bufferPtr = (void *)(mem_info->pbuffer.get());

    bzero((void *)&ycbcr, sizeof(ycbcr));
    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        ret = mem_info->pbuffer->lockYCbCr(usage, bounds, &ycbcr);
        if (ret != NO_ERROR) {
            HAL_LOGV("lockcbcr.onQueueFilled, mapper.lock failed try "
                     "lockycbcr. %p, ret %d",
                     *buffer_handle, ret);
            ret = mem_info->pbuffer->lock(usage, bounds, &vaddr);
            if (ret != NO_ERROR) {
                HAL_LOGE("locky.onQueueFilled, mapper.lock fail %p, ret %d",
                         *buffer_handle, ret);
            } else {
                mem_info->addr_vir = vaddr;
            }
        } else {
            mem_info->addr_vir = ycbcr.y;
        }
    } else {
        ret = mem_info->pbuffer->lock(usage, bounds, &vaddr);
        if (ret != NO_ERROR) {
            HAL_LOGV("lockonQueueFilled, mapper.lock failed try lockycbcr. %p, "
                     "ret %d",
                     *buffer_handle, ret);
            ret = mem_info->pbuffer->lockYCbCr(usage, bounds, &ycbcr);
            if (ret != NO_ERROR) {
                HAL_LOGE("lockycbcr.onQueueFilled, mapper.lock fail %p, ret %d",
                         *buffer_handle, ret);
            } else {
                mem_info->addr_vir = ycbcr.y;
            }
        } else {
            mem_info->addr_vir = vaddr;
        }
    }
    mem_info->fd = ADP_BUFFD(*buffer_handle);
    // mem_info->addr_phy is offset, always set to 0 for yaddr
    mem_info->addr_phy = (void *)0;
    mem_info->size = ADP_BUFSIZE(*buffer_handle);
    mem_info->width = ADP_WIDTH(*buffer_handle);
    mem_info->height = ADP_HEIGHT(*buffer_handle);
    mem_info->format = ADP_FORMAT(*buffer_handle);
    HAL_LOGV("fd = 0x%x, addr_phy offset = %p, addr_vir = %p,"
             " buf size = %zu, width = %d, height = %d, format = %d",
             mem_info->fd, mem_info->addr_phy, mem_info->addr_vir,
             mem_info->size, mem_info->width, mem_info->height, mem_info->format);

err_out:
    return ret;
}

/*===========================================================================
 * FUNCTION   : unmap3
 *
 * DESCRIPTION: unregister buffers for ultrawid
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3GrallocMemory::unmap3(buffer_handle_t *buffer_handle,
                                     hal_mem_info_t *mem_info) {
    if (mem_info->pbuffer != NULL) {
        mem_info->pbuffer->unlock();
        mem_info->pbuffer.clear();
        mem_info->pbuffer = NULL;
        mem_info->bufferPtr = NULL;
    }
    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   : cacheOps
 *
 * DESCRIPTION: ion related memory cache operations
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *   @cmd     : cache ops command
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3GrallocMemory::cacheOps(int index, unsigned int cmd) {
    return cacheOpsInternal(index, cmd, NULL);
}

}; // namespace sprdcamera
