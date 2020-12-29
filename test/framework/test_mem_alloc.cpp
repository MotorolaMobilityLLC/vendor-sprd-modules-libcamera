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
#include <memory.h>
#include <vector>
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
#include <map>
#include <string.h>
#define LOG_TAG "IT-bufferPool"

TestMemPool::~TestMemPool() {
    auto freeIon = [](new_ion_mem_t &memory){
        if (memory.ion_heap) {
            IT_LOGD ("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
                    memory.fd, memory.phys_addr, memory.virs_addr, memory.phys_size,
                    memory.ion_heap);
            delete memory.ion_heap;
            memory.ion_heap = NULL;
        } else {
            IT_LOGD ("memory.ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                    "virt_addr=%p, size=0x%lx",
                    memory.fd, memory.phys_addr, memory.virs_addr,
                    memory.phys_size);
        }
    };
    auto freeGpu = [](new_gpu_mem_t &buffer){
        if (buffer.graphicBuffer != NULL) {
            buffer.graphicBuffer.clear();
            buffer.graphicBuffer = NULL;
            buffer.phy_addr = NULL;
            buffer.vir_addr = NULL;
        }
        buffer.native_handle = NULL;
    };

    for(auto &unit:mIonBufferArr){
        freeIon(unit);
    }
    mIonBufferArr.clear();

    for(auto &unit:mGpuBufferArr){
        freeGpu(unit);
    }
    mGpuBufferArr.clear();
}

//allocate ion
TestMemPool::TestMemPool(vector<ion_info_t> &ion_config){
    mIommuEnabled = 1;
    mGpuBufferList.clear();
    mIonBufferList.clear();
    mGpuTotalNums = 0;
    mIonTotalNums = 0;
    mIonBufferArr.clear();
    mGpuBufferArr.clear();
    size_t mem_size = 0;
    android::MemIon *pHeapIon = NULL;
    new_ion_mem_t ionArr;
    vector<ion_info_t>::iterator itor = ion_config.begin();
    while(itor!=ion_config.end()){
      if (itor->num_bufs == 0){
        IT_LOGE ("buf_nums is 0.");
        goto getpmem_fail;
      }
        mIonTotalNums += itor->num_bufs;
        IT_LOGD ("ion nums：%d" , mIonTotalNums);
        itor++;
    }

    itor=ion_config.begin();
    while(itor!=ion_config.end()){
        mem_size = itor->buf_size;
        mem_size = (mem_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        for(int x=0;x<itor->num_bufs;x++){
            if (mem_size == 0 ) {
                IT_LOGE ("buffer size is 0");
                goto getpmem_fail;
            }
            ionArr.busy_flag = false;
            IT_LOGD ("mIommuEnabled %d,mem size=%d", mIommuEnabled,mem_size);

            if (!mIommuEnabled) {
                if (itor->is_cache) {
                    pHeapIon = new (android::MemIon) ("/dev/ion", mem_size, 0,
                                                     (1<<31)|ION_HEAP_ID_MASK_MM|
                                                     ION_FLAG_NO_CLEAR);
                } else{
                    pHeapIon = new (android::MemIon) ("/dev/ion", mem_size,
                                                      android::MemIon::NO_CACHING,
                                                      ION_HEAP_ID_MASK_MM | ION_FLAG_NO_CLEAR);
                }
            } else {
                if (itor->is_cache) {
                    pHeapIon = new (android::MemIon) ("/dev/ion", mem_size, 0,
                                                     (1<<31) | ION_HEAP_ID_MASK_SYSTEM |
                                                     ION_FLAG_NO_CLEAR);
                } else {
                    pHeapIon = new (android::MemIon) ("/dev/ion", mem_size,
                                                      android::MemIon::NO_CACHING,
                                                      ION_HEAP_ID_MASK_SYSTEM | ION_FLAG_NO_CLEAR);
                }
            }
            if (pHeapIon == NULL || pHeapIon->getHeapID() < 0){
                IT_LOGE ("pHeapIon is null or getHeapID failed");
                goto getpmem_fail;
            }
            if (NULL == pHeapIon->getBase()||MAP_FAILED == pHeapIon->getBase()){
                IT_LOGE ("getBase is null");
                goto getpmem_fail;
            }

            ionArr.ion_heap = pHeapIon;
            ionArr.fd = pHeapIon->getHeapID();
            ionArr.dev_fd = pHeapIon->getIonDeviceFd();
            ionArr.phys_addr = 0;
            ionArr.phys_size = mem_size;
            ionArr.virs_addr = pHeapIon->getBase();
            ionArr.ion_tag = itor->tag;
            IT_LOGD ("fd=0x%x, phys_addr=0x%lx, virs_addr=%p, size=0x%lx, heap=%p",
                    ionArr.fd, ionArr.phys_addr, ionArr.virs_addr, ionArr.phys_size,pHeapIon);
            mIonBufferArr.push_back(ionArr);
        }
        itor++;
    }
     for(auto &unit:mIonBufferArr){
        mIonBufferList.push_back(&unit);
        IT_LOGD ("mIonBufferList=%p" , &unit);
    }
    mFlag=true;
    return ;
    getpmem_fail:
    mFlag=false;
    return ;
}

//allocate gpu
TestMemPool::TestMemPool(vector<gpu_info_t> &gpu_config){
    mIommuEnabled = 1;
    mGpuBufferList.clear();
    mIonBufferList.clear();
    mGpuTotalNums = 0;
    mIonTotalNums = 0;
    mIonBufferArr.clear();
    mGpuBufferArr.clear();
    sp<GraphicBuffer> graphicBuffer = NULL;
    native_handle_t *native_handle = NULL;
    unsigned long phy_addr = 0;
    uint32_t yuvTextUsage;
    new_gpu_mem_t gpuArr;
    vector<gpu_info_t>::iterator itor=gpu_config.begin();
    while(itor!=gpu_config.end()){
      if (itor->num_buf == 0){
        IT_LOGE ("buf_num is 0.");
        goto getpmem_fail;
      }
        mGpuTotalNums += itor->num_buf;
        itor++;
     IT_LOGD ("gpu nums：%d" , mGpuTotalNums);
    }

    itor=gpu_config.begin();
    while(itor!=gpu_config.end()){
        for(int x=0;x<itor->num_buf;x++){
            if (itor->cache_flag == true){
                yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                               GraphicBuffer::USAGE_SW_READ_OFTEN |
                               GraphicBuffer::USAGE_SW_WRITE_OFTEN;
            } else {
                yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE;
            }
            int wid = itor->w;
            int hei = itor->h;
            graphicBuffer = new GraphicBuffer(wid, hei, HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                            1, yuvTextUsage);
            native_handle = (native_handle_t *)graphicBuffer->handle;
            gpuArr.phy_addr = (void *)phy_addr;
            gpuArr.native_handle = native_handle;
            gpuArr.graphicBuffer = graphicBuffer;
            gpuArr.width = wid;
            gpuArr.height = hei;
            gpuArr.gpu_tag = itor->tag;
            IT_LOGD ("w=%d, h=%d, mIommuEnabled=%d, phy_addr=%p",
                    gpuArr.width, gpuArr.height, mIommuEnabled, gpuArr.phy_addr);

            mGpuBufferArr.push_back(gpuArr);
        }
        itor++;
    }
    for(auto &unit:mGpuBufferArr){
        mGpuBufferList.push_back(&unit);
        IT_LOGD ("mGpuBufferList=%p" , &unit);
    }
    mFlag=true;
    return;
    getpmem_fail:
    mFlag=false;
    return;
}

int TestMemPool::CheckFlag(){
    if (mFlag == true){
        return NO_ERROR;
    } else {
        return BAD_VALUE;
    }
}

//pushBUfferList(gpu)
void TestMemPool::pushBufferList (buffer_handle_t *backgpu){
    if (backgpu == NULL){
        IT_LOGE ("backbuf is NULL");
        return;
    }
    int i = 0;
    Mutex::Autolock l(mBufferListLock);
    for (auto &unit:mGpuBufferArr){
        if ((&unit.native_handle) == backgpu){
            IT_LOGD ("mGpuBufferArr %p", &unit);
            mGpuBufferList.push_back(&unit);
            break;
        }
    }
    if (i>= mGpuTotalNums){
        IT_LOGE ("find backbuf failed");
    }
}

//popBUfferList(ion gpu)
bufferData TestMemPool::popBufferList(int tag){
    bufferData ret = {0};

    Mutex::Autolock l (mBufferListLock);
    if(mGpuBufferArr.size()>0){
        List<new_gpu_mem_t *>::iterator j = mGpuBufferList.begin();
        for (;j!=mGpuBufferList.end(); j++){
            if ((*j)->gpu_tag == tag){
                ret.handle = &((*j)->native_handle);
                IT_LOGD ("popbufferlist gpu :%p", ret.handle);
                break;
            }
        }
        if (ret.handle == NULL){
            IT_LOGE ("popBufferList failed!");
            return ret;
        }else{
            mGpuBufferList.erase(j);
            return ret;
        }
    }else{
        List<new_ion_mem_t *>::iterator k = mIonBufferList.begin();
        for (;k!=mIonBufferList.end(); k++){
            if ((*k)->ion_tag == tag){
                ret.ion_buffer = *k;
                IT_LOGD ("popbufferlist ion :%p", ret.ion_buffer);
                break;
            }
        }
        if (ret.ion_buffer == NULL || k ==mIonBufferList.end()){
            IT_LOGE ("popBufferList failed!");
            return ret;
        }else{
            mIonBufferList.erase(k);
            return ret;
        }
    }
}

bufferData TestMemPool::popBufferList(int width,int height){
    bufferData ret = {0};
    if (mGpuBufferList.empty()){
        IT_LOGE ("list is NULL");
        ret.handle = NULL;
        return ret;
    }
    Mutex::Autolock l (mBufferListLock);
    List<new_gpu_mem_t *>::iterator j = mGpuBufferList.begin();
    for (;j!=mGpuBufferList.end(); j++){
        if (((*j)->width == width) && ((*j)->height == height)){
            ret.handle = &((*j)->native_handle);
            IT_LOGD ("popbufferlist gpu :%p", ret.handle);
            break;
        }
    }
    if (ret.handle == NULL || j ==mGpuBufferList.end()){
        IT_LOGE ("popBufferList failed!");
        return ret;
    } else{
        mGpuBufferList.erase(j);
        return ret;
    }
}

//pushbufferlist(ion)
void TestMemPool::pushBufferList(new_ion_mem_t *backion){
    int i = 0;
    Mutex::Autolock l (mBufferListLock);
    for (auto &unit:mIonBufferArr){
        if (&unit == backion){
            IT_LOGD ("mIonBufferArr %p", &unit);
            mIonBufferList.push_back (&unit);
            break;
        }
    }
    if (i>= mIonTotalNums){
        IT_LOGE ("find backbuf failed");
    }
}

//map
int TestMemPool::Map(buffer_handle_t *buffer_handle, void **addr_vir){
    int ret = NO_ERROR;
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

    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888){
        ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle,
                                usage, bounds, &ycbcr);
        if (ret != NO_ERROR){
            IT_LOGE ("lockcbcr.onQueueFilled, mapper.lock failed try"
                   "lockycbcr.%p, ret %d", *buffer_handle, ret);
            ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                              bounds, &vaddr);
            if (ret != NO_ERROR){
            IT_LOGE ("locky.onQueueFilled, mapper.lock fail %p, ret %d",
                    *buffer_handle, ret);
            } else {
                *addr_vir = vaddr;
            }
        } else {
            *addr_vir = ycbcr.y;
        }
    } else {
        ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                              bounds, &vaddr);
        if (ret !=NO_ERROR){
            IT_LOGE ("lockonQueueFilled, mapper.lock failed try lockycbcr. %p,"
                   "ret %d", *buffer_handle, ret);
            ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle,
                                    usage, bounds, &ycbcr);
            if (ret != NO_ERROR){
                IT_LOGE ("lockycbcr.onQueueFilled, mapper.lock fail %p, ret %d",
                    *buffer_handle, ret);
            } else {
                *addr_vir = ycbcr.y;
            }
        } else {
           *addr_vir = vaddr;
        }
    }
    return ret;
}

//unmap
int TestMemPool::UnMap(buffer_handle_t *buffer){
    int ret = NO_ERROR;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    ret = mapper.unlock ((const native_handle_t *)*buffer);
    if (ret != NO_ERROR){
        IT_LOGE ("onQueueFilled, mapper.unlock fail %p", *buffer);
    }
    return ret;
}

//another map
int TestMemPool::Map(new_gpu_mem_t *gpu_buf){
    int ret = 0;
    buffer_handle_t *buffer_handle = &(gpu_buf->native_handle);
    void **addr_vir = (void **)(&(gpu_buf->vir_addr));
    ret = Map(buffer_handle, addr_vir);
    return ret;
}

//another unmap
int TestMemPool::UnMap(new_gpu_mem_t *gpu_buf){
    int ret = 0;
    buffer_handle_t *buffer = &(gpu_buf->native_handle);
    ret = UnMap(buffer);
    return ret;
}

bufferData TestMemPool::popBufferAndWait(int type) {
    bufferData buf_wait = {0};
    while (1)
    {
        buf_wait = popBufferList(type);
        if (buf_wait.handle || buf_wait.ion_buffer) {
            break;
        }
    }
    return buf_wait;
}
