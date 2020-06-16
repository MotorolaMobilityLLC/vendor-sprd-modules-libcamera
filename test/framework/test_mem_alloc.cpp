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

#include <string.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <utils/Trace.h>
#include "cmr_types.h"
#include <gralloc_public.h>
#include "test_common_header.h"
#include "test_mem_alloc.h"
#include "gralloc_public.h"
#include "sprd_ion.h"
#include "MemIon.h"
mutex mTestCapBufLock;

static test_camera_memory_t *mSubRawHeapArray[MAX_SUB_RAWHEAP_NUM];

static int mIommuEnabled;
static unsigned int mCapBufIsAvail;
static unsigned int mSubRawHeapSize = 0;
static unsigned int mTotalIonSize = 0;
static unsigned int mTotalGpuSize = 0;
static unsigned int mTotalStackSize = 0;
static unsigned int mGraphicBufNum = 0;
static unsigned int mSubRawHeapNum = 0;
static bool mIsUltraWideMode;

list<Test_MemIonQueue_t> cam_MemIonQueue;
list<Test_MemGpuQueue_t> cam_MemGpuQueue;

static int Test_map(buffer_handle_t *buffer_handle,
                    test_hal_mem_info_t *mem_info);

static int Test_unmap(buffer_handle_t *buffer_handle,
                      test_hal_mem_info_t *mem_info);

static int Callback_GraphicBufferMalloc(
    struct test_camera_mem_type_stat type, unsigned int size, unsigned int sum,
    unsigned long *phy_addr, unsigned long *vir_addr, int *fd, void **handle,
    unsigned long width, unsigned long height, void *private_data);

static int Callback_GraphicBufferFree(unsigned long mem_type,
                                      unsigned long *phy_addr,
                                      unsigned long *vir_addr, int *fd,
                                      unsigned int sum);

static int Callback_CommonFree(unsigned long mem_type, unsigned long *phy_addr,
                               unsigned long *vir_addr, int *fd,
                               unsigned int sum);

static int Callback_CaptureFree(unsigned long *phy_addr,
                                unsigned long *vir_addr, int *fd,
                                unsigned int sum);
static int Callback_CaptureMalloc(unsigned int size, unsigned int sum,
                                  unsigned long *phy_addr,
                                  unsigned long *vir_addr, int *fd);

static test_camera_memory_t *allocCameraMem(int buf_size, int num_bufs,
                                            unsigned int is_cache);

static void freeCameraMem(test_camera_memory_t *memory);

static int Callback_CommonMalloc(struct test_camera_mem_type_stat type,
                                 unsigned int size, unsigned int *sum_ptr,
                                 unsigned long *phy_addr,
                                 unsigned long *vir_addr, int *fd);

Test_Mem_alloc::Test_Mem_alloc() {
    mIommuEnabled = 1;
    mCapBufIsAvail = 0;
    mIsUltraWideMode = false;
    IT_LOGD("");
}

Test_Mem_alloc::~Test_Mem_alloc() { IT_LOGD(""); }

int Test_Mem_alloc::Test_Callback_GpuMalloc(
    struct test_camera_mem_type_stat type, unsigned int *size_ptr,
    unsigned int *sum_ptr, unsigned long *phy_addr, unsigned long *vir_addr,
    int *fd, void **handle, unsigned long *width, unsigned long *height,
    void *private_data) {
    unsigned int ret = 0, i = 0;
    unsigned int size, sum;
    camera_mem_private_data *camera = (camera_mem_private_data *)private_data;
    //     android::GraphicBuffer* data = (android::GraphicBuffer*)
    //     private_data;
    android::sp<android::GraphicBuffer> *buf;
    buf = (android::sp<android::GraphicBuffer> *)private_data;

    if (!size_ptr || !sum_ptr || (0 == *size_ptr) || (0 == *sum_ptr)) {
        IT_LOGE("param error 0x%lx 0x%lx 0x%lx 0x%lx", (unsigned long)handle,
                (unsigned long)private_data, (unsigned long)size_ptr,
                (unsigned long)sum_ptr);
        return -1;
    }

    if (type.mem_type >= TEST_CAMERA_MEM_TYPE_MAX) {
        IT_LOGE("mem type error %ld", (unsigned long)type.mem_type);
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    switch (type.mem_type) {
    case TEST_CAMERA_PREVIEW:
        ret = Callback_GraphicBufferMalloc(type, size, sum, phy_addr, vir_addr,
                                           fd, handle, *width, *height,
                                           private_data);
        break;

    default:
        break;
    }

    return ret;
}

int Test_Mem_alloc::Test_Callback_IonMalloc(
    struct test_camera_mem_type_stat type, unsigned int *size_ptr,
    unsigned int *sum_ptr, unsigned long *phy_addr, unsigned long *vir_addr,
    int *fd, void *private_data) {

    int ret = 0, i = 0;
    unsigned int size, sum;
    IT_LOGV("E");

    if (!size_ptr || !sum_ptr || (0 == *size_ptr) || (0 == *sum_ptr)) {
        IT_LOGE("param error %p 0x%lx 0x%lx 0x%lx 0x%lx", fd,
                (unsigned long)vir_addr, (unsigned long)private_data,
                (unsigned long)size_ptr, (unsigned long)sum_ptr);
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (type.mem_type >= TEST_CAMERA_MEM_TYPE_MAX) {
        IT_LOGE("mem type error %ld", (unsigned long)type.mem_type);
        return -1;
    }

    if (TEST_CAMERA_CAPTURE == type.mem_type) {
        ret =
            Callback_CommonMalloc(type, size, sum_ptr, phy_addr, vir_addr, fd);
    }

    IT_LOGV("X");
    return 0;
}

int Test_Mem_alloc::Test_Callback_Free(struct test_camera_mem_type_stat type,
                                       unsigned long *phy_addr,
                                       unsigned long *vir_addr, int *fd,
                                       unsigned int sum, void *private_data) {
    camera_mem_private_data *camera = (camera_mem_private_data *)private_data;
    int ret = 0;
    IT_LOGD("E");

    if (type.mem_type >= TEST_CAMERA_MEM_TYPE_MAX) {
        IT_LOGE("mem type error %ld", (unsigned long)type.mem_type);
        return -1;
    }

    if (TEST_CAMERA_PREVIEW == type.mem_type) {
        ret = Callback_GraphicBufferFree(type.mem_type, phy_addr, vir_addr, fd,
                                         sum);
    }
    if (TEST_CAMERA_CAPTURE == type.mem_type) {
        ret = Callback_CommonFree(type.mem_type, phy_addr, vir_addr, fd, sum);
    }
    IT_LOGD("X");
    return ret;
}

int Callback_GraphicBufferMalloc(struct test_camera_mem_type_stat type,
                                 unsigned int size, unsigned int sum,
                                 unsigned long *phy_addr,
                                 unsigned long *vir_addr, int *fd,
                                 void **handle, unsigned long width,
                                 unsigned long height, void *private_data) {
    int NO_ERROR = 0, ret = 0;
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;
    unsigned long mem_type = type.mem_type;
    test_hal_mem_info_t buf_mem_info;
    unsigned int yuvTextUsage = android::GraphicBuffer::USAGE_HW_TEXTURE |
                                android::GraphicBuffer::USAGE_SW_READ_OFTEN |
                                android::GraphicBuffer::USAGE_SW_WRITE_OFTEN;
    android::sp<android::GraphicBuffer> *buf;
    buf = (android::sp<android::GraphicBuffer> *)private_data;
    Test_MemGpuQueue_t memGpu_queue;
    IT_LOGD("malloc %d, shape: %d x %d, size: %d!", sum, width, height, size);
    for (unsigned int i = 0; i < sum; i++) {
        if (mGraphicBufNum >= MAX_GRAPHIC_BUF_NUM)
            goto malloc_failed;

        mTotalGpuSize = mTotalGpuSize + width * height;
        *buf = new (android::GraphicBuffer)(
            width, height, HAL_PIXEL_FORMAT_YCrCb_420_SP, yuvTextUsage,
            std::string("Camera3OEMIf GraphicBuffer"));
        android::sp<android::GraphicBuffer> pbuffer = *buf;
        ret = pbuffer->initCheck();
        if (ret)
            goto malloc_failed;

        if (!pbuffer->handle)
            goto malloc_failed;

        IT_LOGD("buf address =%p", buf);
        memGpu_queue.mGpuHeap.bufferhandle = pbuffer;
        memGpu_queue.mGpuHeap.private_handle = NULL;
        memGpu_queue.mGpuHeap.buf_size = width * height;
        memGpu_queue.mem_type = type;

        ret = Test_map(&(pbuffer->handle), &buf_mem_info);
        if (ret == NO_ERROR) {
            IT_LOGD("ret = NO ERROR");
            phy_addr[i] = (unsigned long)buf_mem_info.addr_phy;
            vir_addr[i] = (unsigned long)buf_mem_info.addr_vir;
            fd[i] = buf_mem_info.fd;
        } else {
            phy_addr[i] = 0;
            vir_addr[i] = 0;
            fd[i] = 0;
            goto malloc_failed;
        }
        *handle = pbuffer.get();
        IT_LOGD("*handle = %p", *handle);
        handle++;
        cam_MemGpuQueue.push_back(memGpu_queue);
        mGraphicBufNum++;
    }
    return NO_ERROR;

malloc_failed:
    Callback_GraphicBufferFree(mem_type, 0, 0, 0, 0);
    return ret;
}

int Callback_GraphicBufferFree(unsigned long mem_type, unsigned long *phy_addr,
                               unsigned long *vir_addr, int *fd,
                               unsigned int sum) {

    unsigned int i = 0;

    Callback_CaptureFree(0, 0, 0, 0);

    for (list<Test_MemGpuQueue_t>::iterator i = cam_MemGpuQueue.begin();
         i != cam_MemGpuQueue.end(); i++) {
        if ((mem_type == i->mem_type.mem_type) &&
            i->mGpuHeap.bufferhandle != NULL) {
            Test_unmap(&(i->mGpuHeap.bufferhandle->handle), NULL);
            mTotalGpuSize = mTotalGpuSize - (i->mGpuHeap.buf_size);
            i->mGpuHeap.bufferhandle.clear();
            i->mGpuHeap.bufferhandle = NULL;
            cam_MemGpuQueue.erase(i);
        }
    }
    mGraphicBufNum = 0;
    IT_LOGD("mem_type=%d sum=%d TotalGpuSize=%d", mem_type, sum, mTotalGpuSize);
    return 0;
}

int Test_map(buffer_handle_t *buffer_handle, test_hal_mem_info_t *mem_info) {
    int NO_ERROR = 0, ret = 0;

    if (NULL == mem_info || NULL == buffer_handle) {
        IT_LOGE("Param invalid handle=%p, info=%p", buffer_handle, mem_info);
        return -EINVAL;
    }
    IT_LOGD("E");

    int width = ADP_WIDTH(*buffer_handle);
    int height = ADP_HEIGHT(*buffer_handle);
    int format = ADP_FORMAT(*buffer_handle);
    android_ycbcr ycbcr;
    android::Rect bounds(width, height);
    void *vaddr = NULL;
    int usage;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();

    bzero((void *)&ycbcr, sizeof(ycbcr));
    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle, usage,
                               bounds, &ycbcr);
        if (ret != NO_ERROR) {
            IT_LOGV("lockcbcr.onQueueFilled, mapper.lock failed try "
                    "lockycbcr. %p, ret %d",
                    *buffer_handle, ret);
            ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                              bounds, &vaddr);
            if (ret != NO_ERROR) {
                IT_LOGE("locky.onQueueFilled, mapper.lock fail %p, ret %d",
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
            IT_LOGV("lockonQueueFilled, mapper.lock failed try lockycbcr. %p, "
                    "ret %d",
                    *buffer_handle, ret);
            ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle,
                                   usage, bounds, &ycbcr);
            if (ret != NO_ERROR) {
                IT_LOGE("lockycbcr.onQueueFilled, mapper.lock fail %p, ret %d",
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
    IT_LOGD("fd = 0x%x, addr_phy offset = %p, addr_vir = %p,"
            " buf size = 0x%x, width = %d, height = %d, fmt = %d",
            mem_info->fd, mem_info->addr_phy, mem_info->addr_vir,
            mem_info->size, mem_info->width, mem_info->height,
            mem_info->format);
    IT_LOGD("ret = %d", ret);
err_out:
    return ret;
}

int Test_unmap(buffer_handle_t *buffer_handle, test_hal_mem_info_t *mem_info) {
    int ret = 0;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    ret = mapper.unlock((const native_handle_t *)*buffer_handle);
    if (ret != 0) {
        IT_LOGE("onQueueFilled, mapper.unlock fail %p", *buffer_handle);
    }
    return ret;
}

int Callback_CaptureFree(unsigned long *phy_addr, unsigned long *vir_addr,
                         int *fd, unsigned int sum) {

    unsigned int i;
    IT_LOGD("mSubRawHeapNum %d sum %d", mSubRawHeapNum, sum);

    for (i = 0; i < mSubRawHeapNum; i++) {
        if (NULL != mSubRawHeapArray[i]) {
            freeCameraMem(mSubRawHeapArray[i]);
        }
        mSubRawHeapArray[i] = NULL;
    }
    mSubRawHeapNum = 0;
    mSubRawHeapSize = 0;
    mCapBufIsAvail = 0;

    return 0;
}

int Callback_CommonFree(unsigned long mem_type, unsigned long *phy_addr,
                        unsigned long *vir_addr, int *fd, unsigned int sum) {
    unsigned int i;
    IT_LOGD("mem_type=%d sum=%d", mem_type, sum);

    if (mem_type == TEST_CAMERA_CAPTURE) {
        for (list<Test_MemIonQueue_t>::iterator i = cam_MemIonQueue.begin();
             i != cam_MemIonQueue.end(); i++) {
            if ((mem_type == i->mem_type.mem_type) && (NULL != i->mIonHeap)) {
                IT_LOGV("mem_type=%d mIonHeap=%p", mem_type, i->mIonHeap);
                freeCameraMem(i->mIonHeap);
                i->mIonHeap = NULL;
                cam_MemIonQueue.erase(i);
            }
        }
    }
    return 0;
}

int Callback_CaptureMalloc(unsigned int size, unsigned int sum,
                           unsigned long *phy_addr, unsigned long *vir_addr,
                           int *fd) {
    test_camera_memory_t *memory = NULL;
    long i = 0;

    IT_LOGD("size %d sum %d mSubRawHeapNum %d", size, sum, mSubRawHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

cap_malloc:
    mTestCapBufLock.lock();
    if (mSubRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
        IT_LOGE("error mSubRawHeapNum %d", mSubRawHeapNum);
        return -1;
    }

    if ((mSubRawHeapNum + sum) >= MAX_SUB_RAWHEAP_NUM) {
        IT_LOGE("malloc is too more %d %d", mSubRawHeapNum, sum);
        return -1;
    }

    if (0 == mSubRawHeapNum) {
        for (i = 0; i < (long)sum; i++) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                IT_LOGE("error memory is null.");
                goto mem_fail;
            }

            mSubRawHeapArray[mSubRawHeapNum] = memory;
            mSubRawHeapNum++;
            *phy_addr++ = (unsigned long)memory->phys_addr;
            *vir_addr++ = (unsigned long)memory->data;
            *fd++ = memory->fd;
        }
        mSubRawHeapSize = size;
    } else {
        if ((mSubRawHeapNum >= sum) && (mSubRawHeapSize >= size)) {
            IT_LOGD("use pre-alloc cap mem");
            for (i = 0; i < (long)sum; i++) {
                *phy_addr++ = (unsigned long)mSubRawHeapArray[i]->phys_addr;
                *vir_addr++ = (unsigned long)mSubRawHeapArray[i]->data;
                *fd++ = mSubRawHeapArray[i]->fd;
            }
        } else {
            mTestCapBufLock.unlock();
            Callback_CaptureFree(0, 0, 0, 0);
            goto cap_malloc;
        }
    }
    mCapBufIsAvail = 1;
    mTestCapBufLock.unlock();
    return 0;

mem_fail:
    mTestCapBufLock.unlock();
    Callback_CaptureFree(0, 0, 0, 0);
    return -1;
}

void freeCameraMem(test_camera_memory_t *memory) {
    if (memory) {
        if (memory->ion_heap) {
            IT_LOGD(
                "fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
                memory->fd, memory->phys_addr, memory->data, memory->phys_size,
                memory->ion_heap);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            IT_LOGD("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                    "virt_addr=%p, size=0x%lx",
                    memory->fd, memory->phys_addr, memory->data,
                    memory->phys_size);
        }
        free(memory);
        memory = NULL;
    }
}

test_camera_memory_t *allocCameraMem(int buf_size, int num_bufs,
                                     unsigned int is_cache) {
    ATRACE_CALL();

    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    android::MemIon *pHeapIon = NULL;

    IT_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);
    test_camera_memory_t *memory =
        (test_camera_memory_t *)malloc(sizeof(test_camera_memory_t));
    if (NULL == memory) {
        IT_LOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(test_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (!mIommuEnabled) {
        if (is_cache) {
            IT_LOGD("part 0");
            pHeapIon = new (android::MemIon)("/dev/ion", mem_size, 0,
                                             (1 << 31) | ION_HEAP_ID_MASK_MM |
                                                 ION_FLAG_NO_CLEAR);
        } else {
            IT_LOGD("part 1");
            pHeapIon = new (android::MemIon)(
                "/dev/ion", mem_size, android::MemIon::NO_CACHING,
                ION_HEAP_ID_MASK_MM | ION_FLAG_NO_CLEAR);
        }
    } else {
        if (is_cache) {
            pHeapIon = new (android::MemIon)(
                "/dev/ion", mem_size, 0,
                (1 << 31) | ION_HEAP_ID_MASK_SYSTEM | ION_FLAG_NO_CLEAR);
        } else {
            pHeapIon = new (android::MemIon)(
                "/dev/ion", mem_size, android::MemIon::NO_CACHING,
                ION_HEAP_ID_MASK_SYSTEM | ION_FLAG_NO_CLEAR);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        IT_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        IT_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    IT_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
            memory->fd, memory->phys_addr, memory->data, memory->phys_size,
            pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

int Callback_CommonMalloc(struct test_camera_mem_type_stat type,
                          unsigned int size, unsigned int *sum_ptr,
                          unsigned long *phy_addr, unsigned long *vir_addr,
                          int *fd) {
    test_camera_memory_t *memory = NULL;
    Test_MemIonQueue_t memIon_queue;
    unsigned int reuse = 0, totalNum = 0;
    unsigned int i;
    unsigned int mem_size;
    unsigned int sum = *sum_ptr;

    IT_LOGD("mem_type=%d size=%d sum=%d", type.mem_type, size, sum);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    totalNum = sum;
    if (totalNum > 0) {
        for (i = 0; i < totalNum; i++) {
            memIon_queue.mIonHeap = allocCameraMem(size, 1, type.is_cache);
            if (NULL == memIon_queue.mIonHeap) {
                IT_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            memIon_queue.mem_type = type;
            IT_LOGV("mem_type=%d mIonHeap=%p is_cache=%d",
                    memIon_queue.mem_type.mem_type, memIon_queue.mIonHeap,
                    memIon_queue.mem_type.is_cache);
            cam_MemIonQueue.push_back(memIon_queue);
            *phy_addr++ = (cmr_uint)memIon_queue.mIonHeap->phys_addr;
            *vir_addr++ = (cmr_uint)memIon_queue.mIonHeap->data;
            *fd++ = memIon_queue.mIonHeap->fd;
        }
    }

    return 0;

mem_fail:
    Callback_CommonFree(type.mem_type, 0, 0, 0, 0);
    return -1;
}
