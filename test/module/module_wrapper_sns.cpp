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
#include "module_wrapper_sns.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "json2sns.h"
#include <dlfcn.h>

#include <vector>
#include "cmr_log.h"
#include "cmr_types.h"
#include "cmr_common.h"
#include "sprd_ion.h"
#include "MemIon.h"
#include <pthread.h>
#include "sensor_drv_u.h"

#define LOG_TAG "IT-moduleSensor"
#define THIS_MODULE_NAME "sns"
using namespace std;
using namespace android;

#define PREVIEW_BUFF_NUM 8
#define MINICAMERA_PARAM_NUM 4
static const int kISPB4awbCount = 16;
#define DEFAULT_DUMP_IMAGES_COUNT 1

#define MN_SET_PRARM(h, x, y, z)                                               \
    do {                                                                       \
        if (NULL != h && NULL != h->ops)                                       \
            h->ops->camera_set_param(x, y, z);                                 \
    } while (0)

/*
 * *  fd:         ion fd
 * *  phys_addr:  offset from fd, always set 0
 * */
typedef struct sprd_camera_memory {
    MemIon* ion_heap;
    cmr_uint phys_addr;
    cmr_uint phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void* handle;
    void* data;
    bool busy_flag;
} sprd_camera_memory_t;

struct client_t {
    int reserved;
};

struct it_sensor_states {
    cmr_int flag_open;
    cmr_int flag_preview;
    oem_module_t* oem_dev;
    cmr_handle oem_handle;
};

static pthread_mutex_t IT_sns_lock;
static it_sensor_states sensor_state[SENSOR_ID_MAX] = { 0 };

static unsigned int m_dump_total_count = DEFAULT_DUMP_IMAGES_COUNT;
static unsigned int m_preview_dump_cnt = 0;
static int mIs_preview_valid = 0;
static int mIs_iommu_enabled = 0;

static unsigned int mPreviewHeapNum = 0;
static sprd_camera_memory* mPreviewHeapArray[PREVIEW_BUFF_NUM];
static uint32_t mIspFirmwareReserved_cnt = 0;
static sprd_camera_memory_t* mPreviewHeapReserved = NULL;
static sprd_camera_memory_t* mIspLscHeapReserved = NULL;
static sprd_camera_memory_t* mIspAFLHeapReserved = NULL;
static sprd_camera_memory_t* mIspFirmwareReserved = NULL;
static sprd_camera_memory_t* mIspStatisHeapReserved = NULL;
static sprd_camera_memory_t* mIspB4awbHeapReserved[kISPB4awbCount];
static sprd_camera_memory_t* mIspRawAemHeapReserved[kISPB4awbCount];
static oem_module_t* m_oem_dev = NULL;
static cmr_handle m_oem_handle = NULL;

extern map<string, vector<resultData_t>*> gMap_Result;

static void mn_reset_variables() {
    m_dump_total_count = DEFAULT_DUMP_IMAGES_COUNT;
    m_preview_dump_cnt = 0;
    mIs_preview_valid = 0;
    mIs_iommu_enabled = 0;
    mPreviewHeapNum = 0;
    memset(mPreviewHeapArray, 0, sizeof(mPreviewHeapArray));
    mIspFirmwareReserved_cnt = 0;
    mPreviewHeapReserved = 0;
    mIspLscHeapReserved = NULL;
    mIspAFLHeapReserved = NULL;
    mIspFirmwareReserved = NULL;
    mIspStatisHeapReserved = NULL;
    memset(mIspB4awbHeapReserved, 0, sizeof(mIspB4awbHeapReserved));
    memset(mIspRawAemHeapReserved, 0, sizeof(mIspRawAemHeapReserved));
    m_oem_dev = NULL;
    m_oem_handle = NULL;
}

static void free_camera_mem(sprd_camera_memory_t* memory) {

    if (memory == NULL)
        return;

    if (memory->ion_heap) {
        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
}

static int get_preview_buffer_id_for_fd(cmr_s32 fd,
    sprd_camera_memory** previewheapArray,
    cmr_int max_buff_num) {
    unsigned int i = 0;

    for (i = 0; i < max_buff_num; i++) {
        if (!previewheapArray[i])
            continue;

        if (!(cmr_uint)previewheapArray[i]->fd)
            continue;

        if (previewheapArray[i]->fd == fd)
            return i;
    }

    return -1;
}

/*callback_malloc free*/
static int callback_previewfree(cmr_uint* phy_addr, cmr_uint* vir_addr,
    cmr_s32* fd, cmr_u32 sum) {
    cmr_u32 i;

    UNUSED(phy_addr);
    UNUSED(vir_addr);
    UNUSED(fd);
    UNUSED(sum);

    pthread_mutex_lock(&IT_sns_lock);
    for (i = 0; i < mPreviewHeapNum; i++) {
        if (!mPreviewHeapArray[i])
            continue;

        free_camera_mem(mPreviewHeapArray[i]);
        mPreviewHeapArray[i] = NULL;
    }

    mPreviewHeapNum = 0;
    pthread_mutex_unlock(&IT_sns_lock);
    return 0;
}

/*alloc*/
static sprd_camera_memory_t* alloc_camera_mem(int buf_size, int num_bufs,
    uint32_t is_cache) {

    size_t mem_size = 0;
    MemIon* pHeapIon = NULL;

    IT_LOGI("buf_size=%d, num_bufs=%d", buf_size, num_bufs);

    sprd_camera_memory_t* memory =
        (sprd_camera_memory_t*)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        IT_LOGE("failed: fatal error! memory pointer is null");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        IT_LOGE("failed: mem size err");
        goto getpmem_fail;
    }

    if (0 == mIs_iommu_enabled) {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                (1 << 31) | ION_HEAP_ID_MASK_MM);
        }
        else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                ION_HEAP_ID_MASK_MM);
        }
    }
    else {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        }
        else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        IT_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || ((void*)-1) == pHeapIon->getBase()) {
        IT_LOGE("error getBase is null. failed: ion "
            "get base err");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    IT_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx"
        "heap = %p",
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

/*in callback_malloc*/
static int callback_preview_malloc(cmr_u32 size, cmr_u32 sum,
    cmr_uint* phy_addr, cmr_uint* vir_addr,
    cmr_s32* fd) {
    sprd_camera_memory_t* memory = NULL;
    cmr_uint i = 0;

    IT_LOGI("size=%d sum=%d ", size, sum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = alloc_camera_mem(size, 1, true);
        if (!memory) {
            IT_LOGE("alloc_camera_mem failed");
            goto mem_fail;
        }

        mPreviewHeapArray[i] = memory;
        mPreviewHeapNum++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

    mem_fail:
    return -1;
}

/*in callback_other_malloc*/
static int callback_other_free(enum camera_mem_cb_type type, cmr_uint* phy_addr,
    cmr_uint* vir_addr, cmr_s32* fd, cmr_u32 sum) {
    unsigned int i;

    UNUSED(phy_addr);
    UNUSED(vir_addr);
    UNUSED(fd);
    UNUSED(sum);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != mPreviewHeapReserved) {
            free_camera_mem(mPreviewHeapReserved);
            mPreviewHeapReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            free_camera_mem(mIspLscHeapReserved);
            mIspLscHeapReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                free_camera_mem(mIspB4awbHeapReserved[i]);
                mIspB4awbHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved && !(--mIspFirmwareReserved_cnt)) {
            free_camera_mem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_STATIS) {
        if (NULL != mIspStatisHeapReserved) {
            mIspStatisHeapReserved->ion_heap->free_kaddr();
            free_camera_mem(mIspStatisHeapReserved);
            mIspStatisHeapReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_RAWAE) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspRawAemHeapReserved[i]) {
                mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
                free_camera_mem(mIspRawAemHeapReserved[i]);
            }
            mIspRawAemHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAFLHeapReserved) {
            free_camera_mem(mIspAFLHeapReserved);
            mIspAFLHeapReserved = NULL;
        }
    }

    return 0;
}

/*in callback_malloc*/
static int callback_other_malloc(enum camera_mem_cb_type type, cmr_u32 size,
    cmr_u32 sum, cmr_uint* phy_addr,
    cmr_uint* vir_addr, cmr_s32* fd) {
    sprd_camera_memory_t* memory = NULL;
    cmr_u32 i;

    IT_LOGD("size=%d sum=%d mPreviewHeapNum=%d, type=%d", size, sum,
        mPreviewHeapNum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == mPreviewHeapReserved) {
            memory = alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                IT_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mPreviewHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
        else {
            IT_LOGI("type=%d,request num=%d, request size=0x%x", type, sum,
                size);
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
            *fd++ = mPreviewHeapReserved->fd;
        }
    }
    else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                IT_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
        else {
            *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
            *fd++ = mIspLscHeapReserved->fd;
        }
    }
    else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (mIspStatisHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                IT_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspStatisHeapReserved = memory;
        }
#if defined(CONFIG_ISP_2_6)
        // sharkl5 dont have get_kaddr interface
        // m_isp_statis_heap_reserved->ion_heap->get_kaddr(&kaddr, &ksize);
#else
        mIspStatisHeapReserved->ion_heap->get_kaddr(&kaddr, &ksize);
#endif
        * phy_addr++ = kaddr;
        *phy_addr = kaddr >> 32;
        *vir_addr++ = (cmr_uint)mIspStatisHeapReserved->data;
        *fd++ = mIspStatisHeapReserved->fd;
        *fd++ = mIspStatisHeapReserved->dev_fd;
    }
    else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
        cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                IT_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspB4awbHeapReserved[i] = memory;
            *phy_addr_64++ = (cmr_u64)memory->phys_addr;
            *vir_addr_64++ = (cmr_u64)memory->data;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr++ = kaddr;
            *phy_addr = kaddr >> 32;
            *fd++ = memory->fd;
        }
    }
    else if (type == CAMERA_ISP_RAWAE) {
        cmr_u64 kaddr = 0;
        cmr_u64* phy_addr_64 = (cmr_u64*)phy_addr;
        cmr_u64* vir_addr_64 = (cmr_u64*)vir_addr;
        size_t ksize = 0;
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                IT_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspRawAemHeapReserved[i] = memory;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr_64++ = kaddr;
            *vir_addr_64++ = (cmr_u64)(memory->data);
            *fd++ = memory->fd;
        }
    }
    else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAFLHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                IT_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspAFLHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
        else {
            *phy_addr++ = (cmr_uint)mIspAFLHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspAFLHeapReserved->data;
            *fd++ = mIspAFLHeapReserved->fd;
        }
    }
    else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                IT_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        }
        else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    }
    else {
        IT_LOGE("type ignore: %d, do nothing", type);
    }

    return 0;

    mem_fail:
    callback_other_free(type, 0, 0, 0, 0);
    return -1;
}

static void callback_callback(enum camera_cb_type cb, const void* client_data,
    enum camera_func_type func, void* parm4) {
    struct camera_frame_type* frame = (struct camera_frame_type*)parm4;
    oem_module_t* oem_dev = m_oem_dev;

    struct img_addr addr_vir;

    UNUSED(client_data);

    memset((void*)&addr_vir, 0, sizeof(addr_vir));

    if (frame == NULL) {
        IT_LOGI("parm4 error: null");
        return;
    }

    if (CAMERA_FUNC_START_PREVIEW != func) {
        IT_LOGI("camera func type error: %d", func);
        return;
    }

    if (CAMERA_EVT_CB_FRAME != cb) {
        IT_LOGI("camera cb type error: %d", cb);
        return;
    }

    pthread_mutex_lock(&IT_sns_lock);
    if (!mPreviewHeapArray[frame->buf_id]) {
        IT_LOGI("preview heap array empty");
        pthread_mutex_unlock(&IT_sns_lock);
        return;
    }

    if (!mIs_preview_valid) {
        IT_LOGI("preview disabled");
        pthread_mutex_unlock(&IT_sns_lock);
        return;
    }

    addr_vir.addr_y = frame->y_vir_addr;
    addr_vir.addr_u = frame->y_vir_addr + frame->width * frame->height * 5 / 4;
    if (m_preview_dump_cnt < m_dump_total_count) {
        camera_save_yuv_to_file(m_preview_dump_cnt, CAM_IMG_FMT_BAYER_MIPI_RAW,
            frame->width, frame->height, &addr_vir);
        // dump_image("camera_raw_proc", CAM_IMG_FMT_BAYER_MIPI_RAW,
        // frame->width,
        //            frame->height, m_preview_dump_cnt, &addr_vir,
        //            frame->width * frame->height);
        m_preview_dump_cnt++;
    }

    if (oem_dev == NULL || oem_dev->ops == NULL) {
        IT_LOGE("oem_dev is null");
        pthread_mutex_unlock(&IT_sns_lock);
        return;
    }

    frame->buf_id = get_preview_buffer_id_for_fd(frame->fd, mPreviewHeapArray,
        PREVIEW_BUFF_NUM);
    if (frame->buf_id != 0xFFFFFFFF) {
        oem_dev->ops->camera_set_preview_buffer(
            m_oem_handle, (cmr_uint)mPreviewHeapArray[frame->buf_id]->phys_addr,
            (cmr_uint)mPreviewHeapArray[frame->buf_id]->data,
            (cmr_s32)mPreviewHeapArray[frame->buf_id]->fd);
    }
    pthread_mutex_unlock(&IT_sns_lock);
}

static cmr_int callback_malloc(enum camera_mem_cb_type type, cmr_u32* size_ptr,
    cmr_u32* sum_ptr, cmr_uint* phy_addr,
    cmr_uint* vir_addr, cmr_s32* fd,
    void* private_data) {
    cmr_int ret = 0;
    cmr_u32 size;
    cmr_u32 sum;


    UNUSED(private_data);

    IT_LOGI("type=%d", type);

    pthread_mutex_lock(&IT_sns_lock);
    if (phy_addr == NULL || vir_addr == NULL || size_ptr == NULL ||
        sum_ptr == NULL || (0 == *size_ptr) || (0 == *sum_ptr)) {
        IT_LOGE("alloc error 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr,
            (cmr_uint)vir_addr, (cmr_uint)size_ptr);
        pthread_mutex_unlock(&IT_sns_lock);
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (CAMERA_PREVIEW == type) {
        ret = callback_preview_malloc(size, sum, phy_addr, vir_addr, fd);
        if (ret) {
            pthread_mutex_unlock(&IT_sns_lock);
            IT_LOGE("alloc_camera_mem failed");
            callback_previewfree(0, 0, 0, 0);
            return ret;
        }
    }
    else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
        type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
        type == CAMERA_ISP_RAWAE || type == CAMERA_ISP_BINGING4AWB ||
        type == CAMERA_ISP_ANTI_FLICKER) {
        ret = callback_other_malloc(type, size, sum, phy_addr, vir_addr, fd);
        if(ret)
        {
            IT_LOGE("callback_other_malloc failed");
            goto exit;
        }
    }
    else {
        IT_LOGI("type ignore: %d, do nothing", type);
    }

    mIs_preview_valid = 1;
    pthread_mutex_unlock(&IT_sns_lock);
exit:
    return ret;
}

static cmr_int callback_free(enum camera_mem_cb_type type, cmr_uint* phy_addr,
    cmr_uint* vir_addr, cmr_s32* fd, cmr_u32 sum,
    void* private_data) {
    cmr_int ret = 0;

    if (private_data == NULL || vir_addr == NULL || fd == NULL) {
        IT_LOGE("error param 0x%lx 0x%lx %p 0x%lx", (cmr_uint)phy_addr,
            (cmr_uint)vir_addr, fd, (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        IT_LOGE("mem type error %d", type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = callback_previewfree(phy_addr, vir_addr, fd, sum);
    }
    else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
        type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
        type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
        type == CAMERA_ISP_ANTI_FLICKER) {
        ret = callback_other_free(type, phy_addr, vir_addr, fd, sum);
    }
    else {
        IT_LOGI("type ignore: %d, do nothing", type);
    }

    mIs_preview_valid = 0;

    return ret;
}

static int iommu_is_enabled(oem_module_t* oem_dev, cmr_handle oem_handle) {
    // oem_module_t *oem_dev
    int ret;
    int iommuIsEnabled = 0;

    if (oem_dev == NULL || oem_dev->ops == NULL || oem_handle == NULL) {
        IT_LOGE("failed: input param is null");
        return 0;
    }

    ret = oem_dev->ops->camera_get_iommu_status(oem_handle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

static int mn_startpreview(oem_module_t* oem_dev, cmr_handle oem_handle,
    unsigned int width, unsigned int height,
    unsigned int fps) {
    int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;
    struct cmr_range_fps_param fps_param;
    memset(&zoom_param, 0, sizeof(struct cmr_zoom_param));

    if (oem_handle == NULL || oem_dev == NULL || oem_dev->ops == NULL) {
        IT_LOGE("failed: input param is null "
            "oem_handle:%p,oem_dev:%p,oem_dev->ops:%p",
            oem_handle, oem_dev, oem_dev->ops);
        goto exit;
    }

    preview_size.width = width;
    preview_size.height = height;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio = (float)width / height;

    fps_param.is_recording = 0;

    // TODO  CHECK FPS UNVAILD
    fps_param.min_fps = fps;
    fps_param.max_fps = fps;

    fps_param.video_mode = 0;

    oem_dev->ops->camera_fast_ctrl(oem_handle, CAMERA_FAST_MODE_FD, 0);

    MN_SET_PRARM(oem_dev, oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
        (cmr_uint)&preview_size);
    MN_SET_PRARM(oem_dev, oem_handle, CAMERA_PARAM_PREVIEW_FORMAT,
        IMG_DATA_TYPE_YUV420);
    MN_SET_PRARM(oem_dev, oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    MN_SET_PRARM(oem_dev, oem_handle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
    MN_SET_PRARM(oem_dev, oem_handle, CAMERA_PARAM_RANGE_FPS,
        (cmr_uint)&fps_param);

    ret = oem_dev->ops->camera_set_mem_func(oem_handle, (void*)callback_malloc,
        (void*)callback_free, NULL);
    if (ret) {
        IT_LOGE("camera_set_mem_func failed");
        goto exit;
    }

    ret = oem_dev->ops->camera_start_preview(oem_handle, CAMERA_NORMAL_MODE);
    if (ret) {
        IT_LOGE("camera_start_preview failed");
        goto exit;
    }

    return ret;

    exit:
    return -1;
}

static int mn_stoppreview(oem_module_t* oem_dev, cmr_handle oem_handle) {

    if (oem_handle == NULL || oem_dev == NULL || oem_dev->ops == NULL) {
        IT_LOGE("failed: input param is null");
        goto exit;
    }

    oem_dev->ops->camera_stop_preview(oem_handle);
    oem_dev->ops->camera_deinit(oem_handle);
    return 0;
    exit:
    return -1;
}

static int load_lib(oem_module_t*& oem_dev) {

    void* handle = NULL;
    oem_module_t* omi = NULL;
    const char* sym = OEM_MODULE_INFO_SYM_AS_STR;

    if (!oem_dev) {
        oem_dev = (oem_module_t*)malloc(sizeof(oem_module_t));
        handle = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
        oem_dev->dso = handle;

        if (handle == NULL) {
            IT_LOGE("open libcamoem failed");
            goto loaderror;
        }

        /* Get the address of the struct hal_module_info. */
        omi = (oem_module_t*)dlsym(handle, sym);
        if (omi == NULL) {
            IT_LOGE("symbol failed");
            goto loaderror;
        }
        oem_dev->ops = omi->ops;

        IT_LOGI("loaded libcamoem handle=%p", handle);
    }
    return 0;

    loaderror:
    if (oem_dev->dso != NULL) {
        dlclose(oem_dev->dso);
    }
    free((void*)oem_dev);
    oem_dev = NULL;

    return -1;
}

static void unload_lib(oem_module_t*& oem_dev) {
    if (oem_dev->dso != NULL) {
        dlclose(oem_dev->dso);
    }
    free((void*)oem_dev);
    oem_dev = NULL;
}

extern "C" void *sensor_get_dev_cxt(void);

namespace ITsensorIf {
    /*LOCAL*/
    sensor_raw_info* ICGetSensorRawInfo(cmr_u32 id) {
        SENSOR_EXP_INFO_T* exp_info_ptr = NULL;
        struct sensor_drv_context* current_sensor_cxt = (struct sensor_drv_context*)sensor_get_dev_cxt();

        if (NULL == current_sensor_cxt) {
            IT_LOGE("zero pointer ");
            return NULL;
        }
        if (!sensor_is_init_common(&current_sensor_cxt[id])) {
            IT_LOGE("sensor has not init");
            return NULL;
        }
        exp_info_ptr = &current_sensor_cxt[id].sensor_exp_info;
        if (NULL == exp_info_ptr) {
            IT_LOGE("sensor exp_info_ptr == NULL");
            return NULL;
        }
        return exp_info_ptr->raw_info_ptr;
    }

    /*Get Sensor Information PTR from id*/
    SENSOR_EXP_INFO_T* GetInfoFromID(cmr_u32 id) {
        struct sensor_drv_context* current_sensor_cxt = (struct sensor_drv_context*)sensor_get_dev_cxt();
        if (NULL == current_sensor_cxt) {
            IT_LOGE("zero pointer ");
            return NULL;
        }
        if (!sensor_is_init_common(&current_sensor_cxt[id])) {
            IT_LOGE("sensor has not init");
            return NULL;
        }
        return &current_sensor_cxt[id].sensor_exp_info;
    }

    cmr_int ICWriteGain(cmr_u32 id, cmr_u32 param) {
        struct sensor_drv_context* current_sensor_cxt = (struct sensor_drv_context*)sensor_get_dev_cxt();
        sensor_raw_info* raw_info = ICGetSensorRawInfo(id);
        if (NULL == raw_info || NULL == raw_info->ioctrl_ptr ||
            NULL == raw_info->ioctrl_ptr->set_gain) {
            IT_LOGE("NULL==raw_info||NULL==raw_info->ioctrl_ptr||raw_info->ioctrl_"
                "ptr->set_gain");
            return CMR_CAMERA_FAIL;

        }
        return raw_info->ioctrl_ptr->set_gain(&current_sensor_cxt[id], param);
    }

    cmr_int ICWriteExposure(cmr_u32 id,
        cmr_u32 exposure, cmr_u32 dummy, cmr_u32 size_index, cmr_u32 exp_time) {
        struct sensor_drv_context* current_sensor_cxt = (struct sensor_drv_context*)sensor_get_dev_cxt();
        struct sensor_ex_exposure exp = { 0 };
        sensor_raw_info* raw_info = ICGetSensorRawInfo(id);
        if (NULL == raw_info || NULL == raw_info->ioctrl_ptr ||
            NULL == raw_info->ioctrl_ptr->ex_set_exposure) {
            IT_LOGE("NULL==raw_info||NULL==raw_info->ioctrl_ptr||raw_info->ioctrl_"
                "ptr->ex_set_exposure");
            return CMR_CAMERA_FAIL;
        }
        exp.exposure = 10000;
        exp.dummy = 14;
        exp.size_index = 1;
        exp.exp_time = 0;
        return raw_info->ioctrl_ptr->ex_set_exposure(&current_sensor_cxt[id], (cmr_uint)&exp);
    }

    cmr_int ITSnsOpenCommon(oem_module_t*& oem_dev, cmr_handle& oem_handle,
        unsigned int camera_id) {
        cmr_int ret = CMR_CAMERA_SUCCESS;
        int num = 0;
        struct client_t client_data;

        pthread_mutex_init(&IT_sns_lock, NULL);
        mn_reset_variables();

        ret = load_lib(oem_dev);
        if (ret) {
            IT_LOGE("load_lib failed");
            goto exit;
        }

        if (oem_dev == NULL) {
            ret = CMR_CAMERA_FAIL;
            IT_LOGE("ITSnsOpenCommon failed");
            goto exit;
        }

        memset((void*)&client_data, 0, sizeof(client_data));

        num = sensorGetPhysicalSnsNum();
        IT_LOGD("Physical Sns Num : %d", num);

        ret = oem_dev->ops->camera_init(camera_id, callback_callback, &client_data,
            0, &oem_handle, (void*)callback_malloc,
            (void*)callback_free);

        m_oem_dev = oem_dev;
        m_oem_handle = oem_handle;
        mIs_iommu_enabled = iommu_is_enabled(oem_dev, oem_handle);
        IT_LOGI("mIs_iommu_enabled=%d", mIs_iommu_enabled);

        exit:
        return ret;
    }

    cmr_int ITSnsCloseCommon(oem_module_t*& oem_dev, cmr_handle& oem_handle,
        unsigned int camera_id) {
        cmr_int ret = CMR_CAMERA_SUCCESS;
        if (oem_dev == NULL || oem_handle == NULL ||
            oem_dev->ops->camera_deinit == NULL) {
            IT_LOGE("oem_dev==NULL||oem_handle==NULL||oem_dev->ops->camera_deinit=="
                "NULL,Check  your cases");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        // ret = oem_dev->ops->camera_deinit(&oem_handle);
        oem_handle=NULL;
        pthread_mutex_destroy(&IT_sns_lock);
        unload_lib(oem_dev);
        exit:
        return ret;
    }

    cmr_int ITSnsPreviewStart(oem_module_t* oem_dev, cmr_handle oem_handle,
        unsigned int camera_id, unsigned int width,
        unsigned int height, unsigned int fps) {
        cmr_int ret = CMR_CAMERA_SUCCESS;

        ret = mn_startpreview(oem_dev, oem_handle, width, height, fps);
        if (ret) {
            IT_LOGE("mn_startpreview failed");
            goto exit;
        }
        exit:
        return ret;
    }

    cmr_int ITSnsPreviewStop(oem_module_t* oem_dev, cmr_handle oem_handle) {
        cmr_int ret = CMR_CAMERA_SUCCESS;

        while (1) {
            sleep(2);
            if (m_preview_dump_cnt >= m_dump_total_count) {
                ret = mn_stoppreview(oem_dev, oem_handle);
                if (ret) {
                    IT_LOGE("mn_stoppreview failed");
                    goto exit;
                }
                break;
            }
        }
        exit:
        return ret;
    }

} // namespace ITsensorIf

static cmr_int check_test_function_states(string fn_name, int sensor_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;

    if (sensor_id > SENSOR_ID_MAX) {
        IT_LOGE("fail in sensor id");
        goto exit;
    }
    if (fn_name == "OpenCommon") {
        // if (sensor_state[sensor_id].flag_open)
        //     {IT_LOGE("sensor_state[sensor_id].flag_open");goto failexit;}
        // if (sensor_state[sensor_id].flag_preview)
        //     {IT_LOGE("sensor_state[sensor_id].flag_preview");goto failexit;}
        // if (sensor_state[sensor_id].oem_dev)
        //     {IT_LOGE("sensor_state[sensor_id].oem_dev");goto failexit;}
        // if (sensor_state[sensor_id].oem_handle)
        //     {IT_LOGE("sensor_state[sensor_id].oem_handle");goto failexit;}
    }
    else if (fn_name == "CloseCommon") {
        if (!sensor_state[sensor_id].flag_open)
            goto failexit;
        if (sensor_state[sensor_id].flag_preview)
            goto failexit;
        if (!sensor_state[sensor_id].oem_dev)
            goto failexit;
        if (!sensor_state[sensor_id].oem_handle)
            goto failexit;
    }
    else if (fn_name == "PreviewOpenCommon") {
        if (!sensor_state[sensor_id].flag_open)
            goto failexit;
        if (sensor_state[sensor_id].flag_preview)
            goto failexit;
        if (!sensor_state[sensor_id].oem_dev)
            goto failexit;
        if (!sensor_state[sensor_id].oem_handle)
            goto failexit;
    }
    else if (fn_name == "PreviewStopCommon") {
        if (!sensor_state[sensor_id].flag_open)
            goto failexit;
        if (!sensor_state[sensor_id].flag_preview)
            goto failexit;
        if (!sensor_state[sensor_id].oem_dev)
            goto failexit;
        if (!sensor_state[sensor_id].oem_handle)
            goto failexit;
    }
    else {
        if (!sensor_state[sensor_id].flag_open)
        {IT_LOGE("sensor_state[sensor_id].flag_open");goto failexit;}
        if (!sensor_state[sensor_id].flag_preview)
           {IT_LOGE("sensor_state[sensor_id].flag_preview");goto failexit;}
        if (!sensor_state[sensor_id].oem_dev)
          {IT_LOGE("sensor_state[sensor_id].oem_dev");goto failexit;}
        if (!sensor_state[sensor_id].oem_handle)
            {IT_LOGE("sensor_state[sensor_id].oem_handle");goto failexit;}
    }
    exit:
    return ret;
    failexit:
    ret = CMR_CAMERA_FAIL;
    return ret;
}

static cmr_int run_test_function(string fn_name, vector<int>& prarm,
    int sensor_id) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    if (fn_name == "OpenCommon") {
        ret = ITsensorIf::ITSnsOpenCommon(sensor_state[sensor_id].oem_dev,
            sensor_state[sensor_id].oem_handle,
            sensor_id);
        if (ret) {
            IT_LOGE("OpenCommon ERROR");
            goto exit;
        }
        sensor_state[sensor_id].flag_open = 1;
        goto exit;
    }

    if (fn_name == "CloseCommon") {
        ret = ITsensorIf::ITSnsCloseCommon(sensor_state[sensor_id].oem_dev,
            sensor_state[sensor_id].oem_handle,
            sensor_id);
        if (ret) {
            IT_LOGE("CloseCommon ERROR");
            goto exit;
        }
        sensor_state[sensor_id].flag_open = 0;
        goto exit;
    }

    if (fn_name == "PreviewOpenCommon") {
        if (prarm.size() != 3) {
            ret = CMR_CAMERA_FAIL;
            IT_LOGE("CloseCommon ERROR");
            goto exit;
        }
        ret = ITsensorIf::ITSnsPreviewStart(
            sensor_state[sensor_id].oem_dev, sensor_state[sensor_id].oem_handle,
            sensor_id, prarm[0], prarm[1], prarm[2]);
        if (ret) {
            IT_LOGE("PreviewOpenCommon ERROR");
            goto exit;
        }
        sensor_state[sensor_id].flag_preview = 1;
        goto exit;
    }

    if (fn_name == "PreviewStopCommon") {
        ret = ITsensorIf::ITSnsPreviewStop(sensor_state[sensor_id].oem_dev,
            sensor_state[sensor_id].oem_handle);
        if (ret) {
            IT_LOGE("PreviewStopCommon ERROR");
            goto exit;
        }
        sensor_state[sensor_id].flag_preview = 0;
        goto exit;
    }

    if (fn_name == "ICWriteExposure") {
        if (prarm.size() != 4) {
            ret = CMR_CAMERA_FAIL;
            IT_LOGE("ICWriteExposure ERROR");
            goto exit;
        }
        ret = ITsensorIf::ICWriteExposure(sensor_id, prarm[0], prarm[1], prarm[2], prarm[3]);
        if (ret) {
            IT_LOGE("ICWriteExposure ERROR");
            goto exit;
        }
        goto exit;
    }

    if (fn_name == "ICWriteGain") {
        if (prarm.size() != 1) {
            ret = CMR_CAMERA_FAIL;
            IT_LOGE("ICWriteGain ERROR");
            goto exit;
        }
        ret = ITsensorIf::ICWriteGain(sensor_id, prarm[0]);
        if (ret) {
            IT_LOGE("ICWriteGain ERROR");
            goto exit;
        }
        goto exit;
    }
    IT_LOGE("Unkown Function");
    ret = CMR_CAMERA_FAIL;
exit:
    return ret;
}

ModuleWrapperSNS::ModuleWrapperSNS() {
    IT_LOGD("in ModuleWrapperSNS");
    pVec_Result = gMap_Result[THIS_MODULE_NAME];
}

ModuleWrapperSNS::~ModuleWrapperSNS() { IT_LOGD(""); }

int ModuleWrapperSNS::SetUp() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int ModuleWrapperSNS::TearDown() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int ModuleWrapperSNS::Run(IParseJson* Json2) {
    int ret = CMR_CAMERA_SUCCESS;
    SnsCaseComm* _json2 = (SnsCaseComm*)Json2;

    string function_name = _json2->m_funcName;
    vector function_param = _json2->m_param;
    unsigned int sensor_id = _json2->m_physicalSensorID;

    /*TO DO
     * Check Prarms
     */
    ret = check_test_function_states(function_name, sensor_id);
    if (ret) {
        IT_LOGE("check_test_function_states failed,%s,%d",
            function_name.c_str(), sensor_id);
        goto exit;
    }
    IT_LOGI("Function:%s",function_name.c_str());
    ret = run_test_function(function_name, function_param, sensor_id);
    if (ret) {
        IT_LOGE("run_test_function failed %s",function_name.c_str());
        goto exit;
    }

    exit:
    return ret;
}

int ModuleWrapperSNS::InitOps() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}
