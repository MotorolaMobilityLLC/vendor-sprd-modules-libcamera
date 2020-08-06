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

#define LOG_TAG "camtuning"

#include <cutils/properties.h>
#include <dlfcn.h>
#include "cmr_log.h"
#include "cmr_types.h"
#include "cmr_common.h"
#include "sprd_ion.h"
#include "MemIon.h"
#include <pthread.h>
#include "sensor_drv_u.h"
#include "isp_video.h"

using namespace android;

#define PREVIEW_BUFF_NUM 8
#define CAPTURE_BUFF_NUM 4
#define ISPB4AWB_BUFF_NUM 16

#define CAMTUNING_PARAM_NUM 4
#define CAMTUNING_MIN_FPS 10
#define CAMTUNING_MAX_FPS 30

#define PTR_LASTONE ((void *)-1)
#define SET_PARM(h, x, y, z)                                                   \
    do {                                                                       \
        if (NULL != h && NULL != h->ops)                                       \
            h->ops->camera_set_param(x, y, z);                                 \
    } while (0)

#define CAPTURE_MODE CAMERA_ISP_TUNING_MODE

/*
 * fd:ion fd
 * phys_addr:  offset from fd, always set 0
 */
typedef struct {
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
static unsigned int camtuning_dump_cnt = 10;
static pthread_mutex_t previewlock;
static int previewvalid = 0;
static int is_iommu_enabled = 0;
static uint32_t mIspFirmwareReserved_cnt = 0;
static sprd_camera_memory_t *mPreviewHeapReserved;
static sprd_camera_memory_t *mIspLscHeapReserved;
static sprd_camera_memory_t *mIspAFLHeapReserved;
static sprd_camera_memory_t *mIspFirmwareReserved;
static sprd_camera_memory_t *mIspAiSceneReserved;
static sprd_camera_memory_t *mIspStatisHeapReserved;
static sprd_camera_memory_t *previewHeapArray[PREVIEW_BUFF_NUM];
static sprd_camera_memory_t *captureHeapArray[CAPTURE_BUFF_NUM];

sprd_camera_memory_t *mIspB4awbHeapReserved[ISPB4AWB_BUFF_NUM];
sprd_camera_memory_t *mIspRawAemHeapReserved[ISPB4AWB_BUFF_NUM];

enum camtuning_camera_id {
    camtuning_CAMERA_BACK = 0,
    camtuning_CAMERA_FRONT,
    camtuning_CAMERA_BACK_EXT,
    camtuning_CAMERA_ID_MAX
};

struct client_t {
    int reserved;
};

struct camtuning_context {
    oem_module_t *oem_dev;
    oem_ops_t *ops;
    cmr_handle oem_handle;
    cmr_handle isp_handle;
    unsigned int camera_id;
    struct img_size pre_size; /* preview */
    struct img_size cap_size; /* capture */
    struct client_t client_data;
    unsigned int jpeg_quality;
    unsigned int flag_prev;
    enum takepicture_mode captureMode;
};

static struct camtuning_context *cxt;

static void stdout_show(char *p) {
    assert(p != NULL);
    fprintf(stdout, "%s", p);
}

static int stdin_getint(char *prompt) {
    int t;

    if (prompt)
        fprintf(stdout, "%s", prompt);
    else
        fprintf(stdout, "please input a number: ");
    if (fscanf(stdin, "%d", &t) <= 0)
        return 0;

    return t;
}

static char stdin_getchar(char *prompt) {
    char t;

    if (prompt)
        fprintf(stdout, "%s", prompt);
    else
        fprintf(stdout, "please input a number: ");
    if (fscanf(stdin, "%c", &t) <= 0)
        return 0;

    return t;
}

static void tolow(char *p) {
    assert(p != NULL);

    while (*p != '\0') {
        if (*p >= 'A' && *p <= 'Z')
            *p += ('a' - 'A');
        p++;
    }
}

static int camtuning_parse_param(int argc, char **argv) {
    int i = 0;
    char str[10];

    if (!cxt || argc == 0) {
        goto exit;
    }

    if (argc < CAMTUNING_PARAM_NUM) {
        goto show_help;
    }

    /* argv like: -parameter value */
    for (i = 1; i < argc - 1; i++) {
        tolow(argv[i]);
        if (strcmp(argv[i], "-cameraid") == 0) {
            cxt->camera_id = atoi(argv[++i]);
            if (cxt->camera_id >= camtuning_CAMERA_ID_MAX) {
                CMR_LOGE("get camera id failed, id %d", cxt->camera_id);
                goto exit;
            }
        } else if (strcmp(argv[i], "-w") == 0) {
            cxt->pre_size.width = atoi(argv[++i]);
            if (cxt->pre_size.width <= 0 || (cxt->pre_size.width % 2)) {
                CMR_LOGE("get preview width failed(align 2), %d",
                         cxt->pre_size.width);
                goto exit;
            }
        } else if (strcmp(argv[i], "-h") == 0) {
            cxt->pre_size.height = atoi(argv[++i]);
            if (cxt->pre_size.height <= 0 || (cxt->pre_size.height % 2)) {
                CMR_LOGE("get preview height failed(align 2), %d",
                         cxt->pre_size.height);
                goto exit;
            }
        } else if (strcmp(argv[i], "-cw") == 0) {
            cxt->cap_size.width = atoi(argv[++i]);
            if (cxt->cap_size.width <= 0 || cxt->cap_size.width % 2) {
                CMR_LOGE("get capture width failed(align 2), %d",
                         cxt->cap_size.width);
                goto exit;
            }
        } else if (strcmp(argv[i], "-ch") == 0) {
            cxt->cap_size.height = atoi(argv[++i]);
            if (cxt->cap_size.height <= 0 || cxt->cap_size.height % 2) {
                CMR_LOGE("get capture width failed(align 2), %d",
                         cxt->cap_size.width);
                goto exit;
            }
        } else if (strcmp(argv[i], "-mode") == 0) {
            cxt->captureMode = (enum takepicture_mode)atoi(argv[++i]);
            if ((int)cxt->captureMode < 0 ||
                (int)cxt->captureMode >= TAKEPICTURE_MODE_MAX) {
                cxt->captureMode = CAMERA_NORMAL_MODE;
            }

        } else {
            goto show_help;
        }
    }

    return 0;

show_help:
    stdout_show("usage:\n");
    stdout_show("cam_tuning -cameraid id -w preview_w -h preview_h");
    stdout_show("cam_tuning -cameraid id -w preview_w -h preview_h -cw "
                "capture_w -ch capture_h");
    stdout_show("for example:\n");
    stdout_show("cam_tuning -cameraid 0 -w 1280 -h 720\n");
    stdout_show("cam_tuning -cameraid 1 -w 1280 -h 720\n");
    stdout_show("cam_tuning -cameraid 0 -w 1280 -h 720, -cw 4608 -ch 3456\n");

exit:
    return -1;
}

static int check_cameraid(void) {
    int num;

    num = sensorGetPhysicalSnsNum();
#ifndef CAMERA_CONFIG_SENSOR_NUM
    CMR_LOGI("num: %d", num);
    if (num && cxt->camera_id && cxt->camera_id >= (unsigned int)num) {
        cxt->camera_id = num - 1;
        CMR_LOGI("camera_id: %d", cxt->camera_id);
    }
#endif
    // check capture size
    if (cxt->cap_size.width <= 0 || cxt->cap_size.height <= 0) {
        cxt->cap_size = cxt->pre_size;
    }

    return 0;
}

static int camtuning_load_lib(void) {

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
        cxt->ops = cxt->oem_dev->ops;

        CMR_LOGD("loaded libcamoem handle=%p", handle);
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

static void camtuning_cb(enum camera_cb_type cb, const void *client_data,
                         enum camera_func_type func, void *parm4) {
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
    struct img_addr addr_vir;

    UNUSED(client_data);

    memset((void *)&addr_vir, 0, sizeof(addr_vir));
    CMR_LOGV("cb_type %d func_type: %d frame %p", cb, func, parm4);

    if (frame == NULL) {
        CMR_LOGI("parm4 error: null");
        return;
    }

    switch (func) {
    case CAMERA_FUNC_TAKE_PICTURE:
        break;
    case CAMERA_FUNC_START_PREVIEW:
        if (CAMERA_EVT_CB_FRAME != cb) {
            CMR_LOGI("camera cb type error: %d", cb);
            break;
        }
        if (!previewHeapArray[frame->buf_id]) {
            CMR_LOGI("preview heap array empty");
            break;
        }
        if (!previewvalid) {
            CMR_LOGI("preview disabled");
            break;
        }
        pthread_mutex_lock(&previewlock);
        addr_vir.addr_y = frame->y_vir_addr;
        addr_vir.addr_u = frame->y_vir_addr + frame->width * frame->height;
        // preview
        send_img_data(ISP_TOOL_YVU420_2FRAME, frame->width, frame->height,
                      (char *)frame->y_vir_addr,
                      frame->width * frame->height * 3 / 2);

        if (camtuning_dump_cnt < dump_total_count) {
            camera_save_yuv_to_file(camtuning_dump_cnt, IMG_DATA_TYPE_YUV420,
                                    frame->width, frame->height, &addr_vir);
            camtuning_dump_cnt++;
        }

        if (cxt->oem_dev == NULL || cxt->ops == NULL) {
            CMR_LOGE("oem_dev is null");
            pthread_mutex_unlock(&previewlock);
            break;
        }
        frame->buf_id = get_preview_buffer_id_for_fd(frame->fd);
        if (frame->buf_id != 0xFFFFFFFF) {
            cxt->ops->camera_set_preview_buffer(
                cxt->oem_handle,
                (cmr_uint)previewHeapArray[frame->buf_id]->phys_addr,
                (cmr_uint)previewHeapArray[frame->buf_id]->data,
                (cmr_s32)previewHeapArray[frame->buf_id]->fd);
        }
        pthread_mutex_unlock(&previewlock);
        break;
    case CAMERA_FUNC_AE_STATE_CALLBACK: // ae
    case CAMERA_FUNC_ENCODE_PICTURE: // encode to jpeg
        break;
    default:
        CMR_LOGI("camera func type error: %d %d", func, cb);
    }

    return;
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

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!previewHeapArray[i])
            continue;

        free_camera_mem(previewHeapArray[i]);
        previewHeapArray[i] = NULL;
    }


    return 0;
}

static int callback_capturefree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    UNUSED(phy_addr);
    UNUSED(vir_addr);
    UNUSED(fd);
    UNUSED(sum);

    for (i = 0; i < CAPTURE_BUFF_NUM; i++) {
        if (!captureHeapArray[i])
            continue;

        free_camera_mem(captureHeapArray[i]);
        captureHeapArray[i] = NULL;
    }

    return 0;
}

static sprd_camera_memory_t *alloc_camera_mem(int buf_size, int num_bufs,
                                              uint32_t is_cache) {

    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    CMR_LOGI("buf_size=%d, num_bufs=%d", buf_size, num_bufs);
    if (buf_size <= 0 || num_bufs <= 0) {
        CMR_LOGE("mem size err");
        return NULL;
    }

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        CMR_LOGE("memory pointer is null");
        return NULL;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;
    mem_size = buf_size * num_bufs;
    mem_size = (mem_size + 4095U) & (~4095U);

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

    if (NULL == pHeapIon->getBase() || PTR_LASTONE == pHeapIon->getBase()) {
        CMR_LOGE("error getBase is null. failed: ion get base err");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    CMR_LOGD("fd=0x%x,phys_addr=0x%lx,virt_addr=%p,size=0x%lx,heap=%p",
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
    sprd_camera_memory_t *memory;
    cmr_uint i = 0;

    CMR_LOGI("size=%d sum=%d", size, sum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < sum; i++) {
        memory = alloc_camera_mem(size, 1, true);
        if (!memory) {
            CMR_LOGE("alloc_camera_mem failed");
            goto mem_fail;
        }

        previewHeapArray[i] = memory;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    callback_previewfree(0, 0, 0, 0);
    return -1;
}

static int callback_capture_malloc(cmr_u32 size, cmr_u32 sum,
                                   cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;

    CMR_LOGI("size=%d sum=%d", size, sum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < sum; i++) {
        memory = alloc_camera_mem(size, 1, true);
        if (!memory) {
            CMR_LOGE("alloc_camera_mem failed");
            goto mem_fail;
        }

        captureHeapArray[i] = memory;
        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    callback_capturefree(0, 0, 0, 0);
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
        for (i = 0; i < ISPB4AWB_BUFF_NUM; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                free_camera_mem(mIspB4awbHeapReserved[i]);
                mIspB4awbHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved) {
            free_camera_mem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_PREVIEW_SCALE_AI_SCENE) {
        if (NULL != mIspAiSceneReserved) {
            free_camera_mem(mIspAiSceneReserved);
            mIspAiSceneReserved = NULL;
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
        for (i = 0; i < ISPB4AWB_BUFF_NUM; i++) {
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

    CMR_LOGD("size=%d sum=%d type=%d", size, sum, type);

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
        }
        *phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
        *fd++ = mPreviewHeapReserved->fd;

    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
        *fd++ = mIspLscHeapReserved->fd;
    } else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;

        if (mIspStatisHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspStatisHeapReserved = memory;
        }
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

        sum = (sum < ISPB4AWB_BUFF_NUM) ? sum : ISPB4AWB_BUFF_NUM;
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

        sum = (sum < ISPB4AWB_BUFF_NUM) ? sum : ISPB4AWB_BUFF_NUM;
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
        }
        *phy_addr++ = (cmr_uint)mIspAFLHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspAFLHeapReserved->data;
        *fd++ = mIspAFLHeapReserved->fd;
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (mIspFirmwareReserved == NULL) {
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
    } else if (type == CAMERA_PREVIEW_SCALE_AI_SCENE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (mIspAiSceneReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspAiSceneReserved = memory;
        } else {
            memory = mIspAiSceneReserved;
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
    } else if (CAMERA_SNAPSHOT == type) {
        CMR_LOGD("fd %p", fd);
        ret = callback_capture_malloc(size, sum, phy_addr, vir_addr, fd);
        CMR_LOGD("phy 0x%x,vir 0x%x, fd %p %d", *phy_addr, *vir_addr, fd, *fd);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_RAWAE || type == CAMERA_ISP_BINGING4AWB ||
               type == CAMERA_ISP_ANTI_FLICKER ||
               type == CAMERA_PREVIEW_SCALE_AI_SCENE) {
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
    } else if (CAMERA_SNAPSHOT == type) {
        ret = callback_capturefree(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER ||
               type == CAMERA_PREVIEW_SCALE_AI_SCENE) {
        ret = callback_other_free(type, phy_addr, vir_addr, fd, sum);
    } else {
        CMR_LOGE("type ignore: %d, do nothing", type);
    }

    previewvalid = 0;

    return ret;
}

static int iommu_is_enabled(struct camtuning_context *cxt) {
    int ret;
    int iommuIsEnabled = 0;

    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        return 0;
    }

    if (cxt->oem_handle == NULL || cxt->oem_dev == NULL || cxt->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        return 0;
    }

    ret = cxt->ops->camera_get_iommu_status(cxt->oem_handle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

static int camtuning_init(void) {
    int ret = 0;
    unsigned int cameraId = 0;
    struct client_t client_data;
    struct phySensorInfo *phyPtr;
    int i;
    int maxw, maxh;

    memset((void *)&client_data, 0, sizeof(client_data));

    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        return -1;
    }
    // set null
    cxt->flag_prev = 0;
    mPreviewHeapReserved = NULL;
    mIspLscHeapReserved = NULL;
    mIspAFLHeapReserved = NULL;
    mIspFirmwareReserved = NULL;
    mIspAiSceneReserved = NULL;
    mIspStatisHeapReserved = NULL;
    for (i = 0; i < PREVIEW_BUFF_NUM; i++)
        previewHeapArray[i] = NULL;
    for (i = 0; i < CAPTURE_BUFF_NUM; i++)
        captureHeapArray[i] = NULL;

    cameraId = cxt->camera_id;
    client_data = cxt->client_data;

    phyPtr = sensorGetPhysicalSnsInfo(cameraId);
    if (phyPtr) {
        maxw = phyPtr->source_width_max;
        maxh = phyPtr->source_height_max;
    } else {
        maxw = 8000;
        maxh = 6000;
    }
    cxt->ops->camera_set_largest_picture_size(cameraId, maxw, maxh);

    ret = cxt->ops->camera_init(cameraId, camtuning_cb, &client_data, 0,
                                &cxt->oem_handle, (void *)callback_malloc,
                                (void *)callback_free);

    is_iommu_enabled = iommu_is_enabled(cxt);
    CMR_LOGI("iommu_enabled=%d, sensor max[%d %d]",
            is_iommu_enabled, maxw, maxh);

    return ret;
}

static int camtuning_deinit(void) {
    cxt->ops->camera_deinit(cxt->oem_handle);

    return 0;
}

static int camtuning_startpreview(uint32_t param1, uint32_t param2) {
    int ret = 0;
    struct cmr_zoom_param zoom_param;
    struct cmr_range_fps_param fps_param;
    enum takepicture_mode captureMode;
    cmr_uint zsl_enable = 0;
    struct img_size cap_size = {0, 0};

    UNUSED(param1);
    UNUSED(param2);

    CMR_LOGI("E");
    captureMode = cxt->captureMode;
    if (captureMode == CAMERA_ZSL_MODE)
        zsl_enable = 1;

    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        goto exit;
    }

    if (cxt->oem_handle == NULL || cxt->oem_dev == NULL || cxt->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        goto exit;
    }

    if (cxt->flag_prev) {
        CMR_LOGI("Preview has been start");
        goto exit;
    }

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio =
        (float)cxt->pre_size.width / cxt->pre_size.height;

    fps_param.is_recording = 0;
    fps_param.min_fps = CAMTUNING_MIN_FPS;
    fps_param.max_fps = CAMTUNING_MAX_FPS;
    fps_param.video_mode = 0;

    cxt->ops->camera_fast_ctrl(cxt->oem_handle, CAMERA_FAST_MODE_FD, 0);

    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_AF_MODE,
             CAMERA_FOCUS_MODE_CAF);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&cxt->pre_size);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&cap_size);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_PREVIEW_FORMAT,
             CAM_IMG_FMT_YUV420_NV21);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_CAPTURE_FORMAT,
             CAM_IMG_FMT_YUV420_NV21);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_ZOOM,
             (cmr_uint)&zoom_param);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             zsl_enable);

    ret = cxt->ops->camera_set_mem_func(
        cxt->oem_handle, (void *)callback_malloc, (void *)callback_free, NULL);
    if (ret) {
        CMR_LOGE("camera_set_mem_func failed");
        goto exit;
    }

    ret = cxt->ops->camera_start_preview(cxt->oem_handle, captureMode);
    if (ret) {
        CMR_LOGE("camera_start_preview failed");
        goto exit;
    }
    cxt->flag_prev = 1;

    return ret;

exit:
    return -1;
}

static int camtuning_stoppreview(uint32_t param1, uint32_t param2) {
    UNUSED(param1);
    UNUSED(param2);

    CMR_LOGI("E");

    if (cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        goto exit;
    }

    if (cxt->oem_handle == NULL || cxt->oem_dev == NULL || cxt->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        goto exit;
    }

    if (!cxt->flag_prev) {
        CMR_LOGI("Preview has been stop");
        goto exit;
    }

    cxt->ops->camera_stop_preview(cxt->oem_handle);
    cxt->flag_prev = 0;

    return 0;

exit:
    return -1;
}

/* input: 0, capture_format */
static int camtuning_takepicture(uint32_t param1, uint32_t param2) {
    int ret;
    int quality;
    enum takepicture_mode captureMode;
    struct img_size jpeg_thumb_size;
    cmr_uint zsl_enable = 0;
    uint32_t capture_format = param2;

    UNUSED(param1);

    CMR_LOGI("E, format 0x%x", capture_format);

    // not clear cxt->flag_prev, stoppreview after takepicture
    cxt->ops->camera_stop_preview(cxt->oem_handle);

    captureMode = cxt->captureMode;
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&cxt->pre_size);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_SHOT_NUM, 1);
    // SET_PARM(cxt->oem_dev, cxt->oem_handle,
    //          CAMERA_PARAM_CAPTURE_FORMAT, capture_format);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_CAPTURE_FORMAT,
             CAM_IMG_FMT_YUV420_NV21);

    quality = cxt->jpeg_quality;
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_JPEG_QUALITY,
             (cmr_uint)quality);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_THUMB_QUALITY,
             (cmr_uint)quality);
    jpeg_thumb_size.width = 320;
    jpeg_thumb_size.height = 240;
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&jpeg_thumb_size);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&cxt->cap_size);
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_EXIF_MIME_TYPE,
             MODE_SINGLE_CAMERA);
    if (captureMode == CAMERA_ZSL_MODE)
        zsl_enable = 1;
    SET_PARM(cxt->oem_dev, cxt->oem_handle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)zsl_enable);

    ret = cxt->ops->camera_take_picture(cxt->oem_handle, captureMode);
    CMR_LOGI("take picture.\n");

    return 0;
}

static int camtuning_setparam(uint32_t param1, uint32_t param2) {
    cxt->cap_size.width = param1;
    cxt->cap_size.height = param2;
    CMR_LOGI("cap size[%d %d]", cxt->cap_size.width, cxt->cap_size.height);

    return 0;
}

static int camtuning_setjpegquality(uint32_t param1, uint32_t param2) {
    UNUSED(param2);

    if (param1)
        cxt->jpeg_quality = param1;
    CMR_LOGI("quality %d", cxt->jpeg_quality);

    return 0;
}

static int isptool_init() {
    cxt->ops->camera_get_isp_handle(cxt->oem_handle, &cxt->isp_handle);
    setispserver(cxt->isp_handle);

    ispvideo_RegCameraFunc(REG_START_PREVIEW, camtuning_startpreview);
    ispvideo_RegCameraFunc(REG_STOP_PREVIEW, camtuning_stoppreview);
    ispvideo_RegCameraFunc(REG_TAKE_PICTURE, camtuning_takepicture);
    ispvideo_RegCameraFunc(REG_SET_PARAM, camtuning_setparam);
    ispvideo_RegCameraFunc(REG_SET_JPEG_QUALITY, camtuning_setjpegquality);

    return 0;
}

int main(int argc, char **argv) {
    int ret = 0;
    struct camtuning_context cxt_main;
    char value[PROPERTY_VALUE_MAX];
    char value2[PROPERTY_VALUE_MAX];

    cxt = &cxt_main;
    memset((void *)cxt, 0, sizeof(*cxt));

    pthread_mutex_init(&previewlock, NULL);

    ret = camtuning_parse_param(argc, argv);
    if (!ret)
        goto RESTART;

INPUT_PARAM:
    /* Input parameter one by one */
    CMR_LOGI("Input parameter one by one");
    cxt->camera_id = stdin_getint("\nPlease input camera id[]: ");
    cxt->pre_size.width = stdin_getint("\nPlease input preview size width: ");
    cxt->pre_size.height = stdin_getint("\nPlease input preview size height: ");
    cxt->cap_size.width = stdin_getint("\nPlease input capture size width: ");
    cxt->cap_size.height = stdin_getint("\nPlease input capture size height: ");

RESTART:
    check_cameraid();
    CMR_LOGD("id %d preview[%d %d], caputre[%d %d]", cxt->camera_id,
             cxt->pre_size.width, cxt->pre_size.height, cxt->cap_size.width,
             cxt->cap_size.height);

    ret = camtuning_load_lib();
    if (ret) {
        CMR_LOGE("camtuning_load_lib failed");
        goto exit;
    }

    ret = camtuning_init();
    if (ret) {
        CMR_LOGE("camtuning_init failed");
        goto exit;
    }
    ret = isptool_init();
    if (ret) {
        CMR_LOGE("init for isp tool fail, ret %d", ret);
        goto exit;
    }
    camtuning_startpreview(NULL, NULL); // start preview

    startispserver(cxt->camera_id);
    property_get("debug.vendor.cam.tuning.exit", value2, "false");
    while (1) {
        sleep(1);
        // ret = (int)stdin_getchar("\nInput x to exit: ");
        // CMR_LOGI("ret = %d", ret);
        // if (ret == (int)'x' || ret == (int)'X')
        // break;
        property_get("debug.vendor.cam.tuning.exit", value, "false");
        if (strcmp(value, value2)) {
            break;
        }
    }
    stopispserver();
    camtuning_stoppreview(NULL, NULL);
    camtuning_deinit();
    stdout_show("exit");
    ret = 0;

    return ret;

exit:
    stdout_show("error, exit");
    return ret;
}
