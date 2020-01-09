/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "minicamera"

#include <dlfcn.h>
#include "cmr_log.h"
#include "cmr_types.h"
#include "cmr_common.h"
#include "sprd_ion.h"
#include "MemIon.h"
#include <pthread.h>
#include "sensor_drv_u.h"

using namespace android;

#define MINICAMERA_WIDTH_MAX 4640
#define MINICAMERA_HEIGHT_MAX 3488
#define PREVIEW_BUFF_NUM 8
#define MINICAMERA_PARAM_NUM 4
#define MINICAMERA_MIN_FPS 5
#define MINICAMERA_MAX_FPS 30

#define SET_PARM(h, x, y, z)                                                   \
    do {                                                                       \
        if (NULL != h && NULL != h->ops)                                       \
            h->ops->camera_set_param(x, y, z);                                 \
    } while (0)

/*
* *  fd:         ion fd
* *  phys_addr:  offset from fd, always set 0
* */
typedef struct sprd_camera_memory {
    MemIon *ion_heap;
    cmr_uint phys_addr;
    cmr_uint phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} sprd_camera_memory_t;

static unsigned int dump_total_count = 10;
static unsigned int minicamera_dump_cnt = 0;
static pthread_mutex_t previewlock;
static int previewvalid = 0;
static int is_iommu_enabled = 0;
static unsigned int mPreviewHeapNum = 0;
static uint32_t mIspFirmwareReserved_cnt = 0;
static const int kISPB4awbCount = 16;
static sprd_camera_memory_t *mPreviewHeapReserved = NULL;
static sprd_camera_memory_t *mIspLscHeapReserved = NULL;
static sprd_camera_memory_t *mIspAFLHeapReserved = NULL;
static sprd_camera_memory_t *mIspFirmwareReserved = NULL;
static sprd_camera_memory_t *mIspStatisHeapReserved = NULL;
sprd_camera_memory_t *mIspB4awbHeapReserved[kISPB4awbCount];
sprd_camera_memory_t *mIspRawAemHeapReserved[kISPB4awbCount];
static sprd_camera_memory *previewHeapArray[PREVIEW_BUFF_NUM];

enum minicamera_camera_id {
    MINICAMERA_CAMERA_BACK = 0,
    MINICAMERA_CAMERA_FRONT,
    MINICAMERA_CAMERA_BACK_EXT,
    MINICAMERA_CAMERA_ID_MAX
};

struct client_t {
    int reserved;
};

struct minicamera_context {
    oem_module_t *oem_dev;
    cmr_handle oem_handle;
    unsigned int camera_id;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    unsigned int loop;
    struct client_t client_data;
};

static struct minicamera_context *minicamera_dev;

static void usage(void) {
    fprintf(
        stderr,
        "usage:\n"
        "minicamera -cameraid camera_id -w preview_width -h "
        "preview_height [-fps framerate] [-dump_cnt n] [--loop]\n"
        "for example:\n"
        "minicamera -cameraid 0 -w 1280 -h 720\n"
        "minicamera -cameraid 0 -w 1280 -h 720 -fps 10\n"
        "minicamera -cameraid 0 -w 1280 -h 720 -fps 10 -dump_cnt 15 --loop\n");
}

static int minicamera_parse_param(struct minicamera_context *cxt, int argc,
                                  char **argv) {
    int i = 0;
    int num = 0;

    cxt->loop = 0;
    if (!cxt) {
        CMR_LOGE("failed: input cxt is null");
        goto exit;
    }

    if (argc < MINICAMERA_PARAM_NUM) {
        usage();
        goto exit;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-cameraid") == 0 && (i < argc - 1)) {
            cxt->camera_id = atoi(argv[++i]);
            if (cxt->camera_id >= MINICAMERA_CAMERA_ID_MAX) {
                CMR_LOGE("get right camera id failed");
                goto exit;
            }
        } else if (strcmp(argv[i], "-w") == 0 && (i < argc - 1)) {
            cxt->width = atoi(argv[++i]);
            if (cxt->width > MINICAMERA_WIDTH_MAX || (cxt->width % 2)) {
                CMR_LOGE("get right width failed");
                goto exit;
            }
        } else if (strcmp(argv[i], "-h") == 0 && (i < argc - 1)) {
            cxt->height = atoi(argv[++i]);
            if (cxt->height > MINICAMERA_HEIGHT_MAX || (cxt->height % 2)) {
                CMR_LOGE("get right height failed");
                goto exit;
            }
        } else if (strcmp(argv[i], "-fps") == 0 && (i < argc - 1)) {
            cxt->fps = atoi(argv[++i]);
            if (cxt->fps > MINICAMERA_MAX_FPS || cxt->fps < MINICAMERA_MIN_FPS) {
                cxt->fps = 10;
            }
        } else if (strcmp(argv[i], "-dump_cnt") == 0 && (i < argc - 1)) {
            dump_total_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--loop") == 0) {
            cxt->loop = 1;
        } else {
            usage();
            goto exit;
        }
    }

    num = sensorGetPhysicalSnsNum();
#ifndef CAMERA_CONFIG_SENSOR_NUM
    CMR_LOGI("num: %d",num);
    if (num && cxt->camera_id && cxt->camera_id >= (unsigned int)num) {
        cxt->camera_id = num - 1;
        CMR_LOGI("camera_id: %d",cxt->camera_id);
    }
#endif
    return 0;

exit:
    return -1;
}

static int minicamera_load_lib(struct minicamera_context *cxt) {

    void *handle = NULL;
    oem_module_t *omi = NULL;
    const char *sym = OEM_MODULE_INFO_SYM_AS_STR;

    if (!cxt) {
        CMR_LOGE("failed: input cxt is null");
        return -1;
    }

    if (!cxt->oem_dev) {
        cxt->oem_dev = (oem_module_t *)malloc(sizeof(oem_module_t));
        handle = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
        cxt->oem_dev->dso = handle;

        if (handle == NULL) {
            CMR_LOGE("open libcamoem failed");
            goto loaderror;
        }

        /* Get the address of the struct hal_module_info. */
        omi = (oem_module_t *)dlsym(handle, sym);
        if (omi == NULL) {
            CMR_LOGE("symbol failed");
            goto loaderror;
        }
        cxt->oem_dev->ops = omi->ops;

        CMR_LOGV("loaded libcamoem handle=%p", handle);
    }

    return 0;

loaderror:
    if (cxt->oem_dev->dso != NULL) {
        dlclose(cxt->oem_dev->dso);
    }
    free((void *)cxt->oem_dev);
    cxt->oem_dev = NULL;

    return -1;
}

static int get_preview_buffer_id_for_fd(cmr_s32 fd) {
    unsigned int i = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!previewHeapArray[i])
            continue;

        if (!(cmr_uint)previewHeapArray[i]->fd)
            continue;

        if (previewHeapArray[i]->fd == fd)
            return i;
    }

    return -1;
}

static void minicamera_cb(enum camera_cb_type cb, const void *client_data,
                          enum camera_func_type func, void *parm4) {
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
    oem_module_t *oem_dev = minicamera_dev->oem_dev;
    cmr_handle oem_handle = minicamera_dev->oem_handle;
    struct img_addr addr_vir;

    UNUSED(client_data);

    memset((void *)&addr_vir, 0, sizeof(addr_vir));

    if (frame == NULL) {
        CMR_LOGI("parm4 error: null");
        return;
    }

    if (CAMERA_FUNC_START_PREVIEW != func) {
        CMR_LOGI("camera func type error: %d", func);
        return;
    }

    if (CAMERA_EVT_CB_FRAME != cb) {
        CMR_LOGI("camera cb type error: %d", cb);
        return;
    }

    pthread_mutex_lock(&previewlock);
    if (!previewHeapArray[frame->buf_id]) {
        CMR_LOGI("preview heap array empty");
        pthread_mutex_unlock(&previewlock);
        return;
    }

    if (!previewvalid) {
        CMR_LOGI("preview disabled");
        pthread_mutex_unlock(&previewlock);
        return;
    }

    addr_vir.addr_y = frame->y_vir_addr;
    addr_vir.addr_u = frame->y_vir_addr + frame->width * frame->height;
    if (minicamera_dump_cnt < dump_total_count) {
        camera_save_yuv_to_file(minicamera_dump_cnt, IMG_DATA_TYPE_YUV420,
                                frame->width, frame->height, &addr_vir);
        minicamera_dump_cnt++;
    }

    if (oem_dev == NULL || oem_dev->ops == NULL) {
        CMR_LOGE("oem_dev is null");
        return;
    }

    frame->buf_id = get_preview_buffer_id_for_fd(frame->fd);
    if (frame->buf_id != 0xFFFFFFFF) {
        oem_dev->ops->camera_set_preview_buffer(
            oem_handle, (cmr_uint)previewHeapArray[frame->buf_id]->phys_addr,
            (cmr_uint)previewHeapArray[frame->buf_id]->data,
            (cmr_s32)previewHeapArray[frame->buf_id]->fd);
    }
    pthread_mutex_unlock(&previewlock);
}

static void free_camera_mem(sprd_camera_memory_t *memory) {

    if (memory == NULL)
        return;

    if (memory->ion_heap) {
        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
}

static int callback_previewfree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    UNUSED(phy_addr);
    UNUSED(vir_addr);
    UNUSED(fd);
    UNUSED(sum);

    pthread_mutex_lock(&previewlock);
    for (i = 0; i < mPreviewHeapNum; i++) {
        if (!previewHeapArray[i])
            continue;

        free_camera_mem(previewHeapArray[i]);
        previewHeapArray[i] = NULL;
    }

    mPreviewHeapNum = 0;
    pthread_mutex_unlock(&previewlock);
    return 0;
}

static sprd_camera_memory_t *alloc_camera_mem(int buf_size, int num_bufs,
                                              uint32_t is_cache) {

    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    CMR_LOGI("buf_size=%d, num_bufs=%d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        CMR_LOGE("failed: fatal error! memory pointer is null");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        CMR_LOGE("failed: mem size err");
        goto getpmem_fail;
    }

    if (0 == is_iommu_enabled) {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM);
        }
    } else {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        CMR_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || ((void *)-1) == pHeapIon->getBase()) {
        CMR_LOGE("error getBase is null. failed: ion "
                 "get base err");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    CMR_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx"
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

static int callback_preview_malloc(cmr_u32 size, cmr_u32 sum,
                                   cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;

    CMR_LOGI("size=%d sum=%d mPreviewHeapNum=%d", size, sum, mPreviewHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = alloc_camera_mem(size, 1, true);
        if (!memory) {
            CMR_LOGE("alloc_camera_mem failed");
            goto mem_fail;
        }

        previewHeapArray[i] = memory;
        mPreviewHeapNum++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    callback_previewfree(0, 0, 0, 0);
    return -1;
}

static int callback_other_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
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

static int callback_other_malloc(enum camera_mem_cb_type type, cmr_u32 size,
                                 cmr_u32 sum, cmr_uint *phy_addr,
                                 cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;

    CMR_LOGD("size=%d sum=%d mPreviewHeapNum=%d, type=%d", size, sum,
             mPreviewHeapNum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == mPreviewHeapReserved) {
            memory = alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mPreviewHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            CMR_LOGI("type=%d,request num=%d, request size=0x%x", type, sum,
                     size);
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
            *fd++ = mPreviewHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
            *fd++ = mIspLscHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (mIspStatisHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
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
        *phy_addr++ = kaddr;
        *phy_addr = kaddr >> 32;
        *vir_addr++ = (cmr_uint)mIspStatisHeapReserved->data;
        *fd++ = mIspStatisHeapReserved->fd;
        *fd++ = mIspStatisHeapReserved->dev_fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
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
    } else if (type == CAMERA_ISP_RAWAE) {
        cmr_u64 kaddr = 0;
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        size_t ksize = 0;
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspRawAemHeapReserved[i] = memory;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr_64++ = kaddr;
            *vir_addr_64++ = (cmr_u64)(memory->data);
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAFLHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspAFLHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            *phy_addr++ = (cmr_uint)mIspAFLHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspAFLHeapReserved->data;
            *fd++ = mIspAFLHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        } else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else {
        CMR_LOGE("type ignore: %d, do nothing", type);
    }

    return 0;

mem_fail:
    callback_other_free(type, 0, 0, 0, 0);
    return -1;
}

static cmr_int callback_malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                               cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd,
                               void *private_data) {
    cmr_int ret = 0;
    cmr_u32 size;
    cmr_u32 sum;

    UNUSED(private_data);

    CMR_LOGI("type=%d", type);

    pthread_mutex_lock(&previewlock);
    if (phy_addr == NULL || vir_addr == NULL || size_ptr == NULL ||
        sum_ptr == NULL || (0 == *size_ptr) || (0 == *sum_ptr)) {
        CMR_LOGE("alloc error 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr,
                 (cmr_uint)vir_addr, (cmr_uint)size_ptr);
        pthread_mutex_unlock(&previewlock);
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (CAMERA_PREVIEW == type) {
        ret = callback_preview_malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_RAWAE || type == CAMERA_ISP_BINGING4AWB ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = callback_other_malloc(type, size, sum, phy_addr, vir_addr, fd);
    } else {
        CMR_LOGE("type ignore: %d, do nothing", type);
    }

    previewvalid = 1;
    pthread_mutex_unlock(&previewlock);
    return ret;
}

static cmr_int callback_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data) {
    cmr_int ret = 0;

    if (private_data == NULL || vir_addr == NULL || fd == NULL) {
        CMR_LOGE("error param 0x%lx 0x%lx %p 0x%lx", (cmr_uint)phy_addr,
                 (cmr_uint)vir_addr, fd, (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        CMR_LOGE("mem type error %d", type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = callback_previewfree(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = callback_other_free(type, phy_addr, vir_addr, fd, sum);
    } else {
        CMR_LOGE("type ignore: %d, do nothing", type);
    }

    previewvalid = 0;

    return ret;
}

static int iommu_is_enabled(struct minicamera_context *cxt) {
    int ret;
    int iommuIsEnabled = 0;

    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        return 0;
    }

    if (cxt->oem_handle == NULL || cxt->oem_dev == NULL ||
        cxt->oem_dev->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        return 0;
    }

    ret = cxt->oem_dev->ops->camera_get_iommu_status(cxt->oem_handle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

static int minicamera_init(struct minicamera_context *cxt) {
    int ret = 0;
    unsigned int cameraId = 0;
    struct client_t client_data;

    memset((void *)&client_data, 0, sizeof(client_data));

    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        return -1;
    }

    cameraId = cxt->camera_id;
    client_data = cxt->client_data;
    minicamera_dev = cxt;

    ret = cxt->oem_dev->ops->camera_init(
        cameraId, minicamera_cb, &client_data, 0, &cxt->oem_handle,
        (void *)callback_malloc, (void *)callback_free);

    is_iommu_enabled = iommu_is_enabled(cxt);
    CMR_LOGI("is_iommu_enabled=%d", is_iommu_enabled);

    return ret;
}

static int minicamera_startpreview(struct minicamera_context *cxt) {
    int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;
    struct cmr_range_fps_param fps_param;
    memset(&zoom_param, 0, sizeof(struct cmr_zoom_param));
    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        goto exit;
    }

    if (cxt->oem_handle == NULL || cxt->oem_dev == NULL ||
        cxt->oem_dev->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        goto exit;
    }

    preview_size.width = cxt->width;
    preview_size.height = cxt->height;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio = (float)cxt->width / cxt->height;

    fps_param.is_recording = 0;
    if (cxt->fps > 0) {
        fps_param.min_fps = cxt->fps;
        fps_param.max_fps = cxt->fps;
    } else {
        fps_param.min_fps = MINICAMERA_MIN_FPS;
        fps_param.max_fps = MINICAMERA_MAX_FPS;
    }
    fps_param.video_mode = 0;

    cxt->oem_dev->ops->camera_fast_ctrl(cxt->oem_handle, CAMERA_FAST_MODE_FD,
                                        0);

    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);

    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_PREVIEW_FORMAT,
             IMG_DATA_TYPE_YUV420);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_ZOOM,
             (cmr_uint)&zoom_param);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    ret = cxt->oem_dev->ops->camera_set_mem_func(
        cxt->oem_handle, (void *)callback_malloc, (void *)callback_free, NULL);
    if (ret) {
        CMR_LOGE("camera_set_mem_func failed");
        goto exit;
    }

    ret = cxt->oem_dev->ops->camera_start_preview(cxt->oem_handle,
                                                  CAMERA_NORMAL_MODE);
    if (ret) {
        CMR_LOGE("camera_start_preview failed");
        goto exit;
    }

    return ret;

exit:
    return -1;
}

static int minicamera_stoppreview(struct minicamera_context *cxt) {

    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        goto exit;
    }

    if (cxt->oem_handle == NULL || cxt->oem_dev == NULL ||
        cxt->oem_dev->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        goto exit;
    }

    cxt->oem_dev->ops->camera_stop_preview(cxt->oem_handle);
    cxt->oem_dev->ops->camera_deinit(cxt->oem_handle);

    return 0;

exit:
    return -1;
}

int main(int argc, char **argv) {
    int ret = 0;
    struct minicamera_context cxt;

    memset((void *)&cxt, 0, sizeof(cxt));
    pthread_mutex_init(&previewlock, NULL);

    ret = minicamera_parse_param(&cxt, argc, argv);
    if (ret) {
        CMR_LOGE("minicamera_parse_param failed");
        goto exit;
    }

    ret = minicamera_load_lib(&cxt);
    if (ret) {
        CMR_LOGE("minicamera_load_lib failed");
        goto exit;
    }

    ret = minicamera_init(&cxt);
    if (ret) {
        CMR_LOGE("minicamera_init failed");
        goto exit;
    }

    ret = minicamera_startpreview(&cxt);
    if (ret) {
        CMR_LOGE("minicamera_startpreview failed");
        goto exit;
    }

    CMR_LOGI("into finish loop!");

    while (1) {
        sleep(1);
        if (cxt.loop == 1)
            continue;

        if (minicamera_dump_cnt >= dump_total_count) {
            ret = minicamera_stoppreview(&cxt);
            if (ret) {
                CMR_LOGE("minicamera_stoppreview failed");
                goto exit;
            }
            break;
        }
    }

    CMR_LOGI("minicamera_test finish and exit");
    return ret;

exit:
    return -1;
}
