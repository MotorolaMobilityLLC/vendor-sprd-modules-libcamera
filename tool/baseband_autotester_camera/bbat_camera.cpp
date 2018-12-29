#define LOG_TAG "bbat_camera"
#include <utils/Log.h>
#if defined(CONFIG_ISP_2_1) || defined(CONFIG_ISP_2_2) ||                      \
    defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_5) ||                      \
    defined(CONFIG_ISP_2_6)
#if defined(CONFIG_ISP_2_1_A)
#include "hal3_2v1a/SprdCamera3OEMIf.h"
#include "hal3_2v1a/SprdCamera3Setting.h"
#else
#include "hal3_2v1/SprdCamera3OEMIf.h"
#include "hal3_2v1/SprdCamera3Setting.h"
#endif
#endif
#if defined(CONFIG_ISP_2_4)
#include "hal3_2v4/SprdCamera3OEMIf.h"
#include "hal3_2v4/SprdCamera3Setting.h"
#endif
#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"
#include "sprd_ion.h"
#include <linux/fb.h>
#include "sprd_cpp.h"
#include <dlfcn.h>
#include "eng_modules.h"
#include "eng_diag.h"

using namespace sprdcamera;

#define PREVIEW_WIDTH 960
#define PREVIEW_HIGHT 720
#define PREVIEW_BUFF_NUM 8
#define SPRD_MAX_PREVIEW_BUF PREVIEW_BUFF_NUM

static Mutex preview_lock;
static int preview_valid = 0;
static int s_mem_method = 0;
static unsigned char camera_id = 0;
static uint32_t frame_num = 0;              /*record frame number*/
static unsigned int m_preview_heap_num = 0; /*allocated preview buffer number*/
static sprd_camera_memory_t *m_preview_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_lsc_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_statis_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_afl_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_firmware_reserved = NULL;
static const int k_isp_b4_awb_count = 16;
static sprd_camera_memory_t *m_isp_b4_awb_heap_reserved[k_isp_b4_awb_count];
static sprd_camera_memory_t *m_isp_raw_aem_heap_reserved[k_isp_b4_awb_count];
static sprd_camera_memory_t *preview_heap_array[PREVIEW_BUFF_NUM];
static int target_buffer_id = 0;

static oem_module_t *m_hal_oem;

struct client_t {
    int reserved;
};
static struct client_t client_data;
static cmr_handle oem_handle = 0;

static uint32_t lcd_w = 0, lcd_h = 0;

int autotest_iommu_is_enabled(void) {
    int ret;
    int iommu_is_enabled = 0;

    if (NULL == oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops) {
        ALOGE("%s,%d, oem is null or oem ops is null.", __func__, __LINE__);
        return 0;
    }

    ret = m_hal_oem->ops->camera_get_iommu_status(oem_handle);
    if (ret) {
        iommu_is_enabled = 0;
        return iommu_is_enabled;
    }

    iommu_is_enabled = 1;
    return iommu_is_enabled;
}

static unsigned int get_preview_buffer_id_for_fd(cmr_s32 fd) {
    unsigned int i = 0;

    ALOGV("%s,%d E", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!preview_heap_array[i])
            continue;

        if (!(cmr_uint)preview_heap_array[i]->fd)
            continue;

        if (preview_heap_array[i]->fd == fd)
            return i;
    }

    return 0xFFFFFFFF;
}

void autotest_camera_close(void) {
    ALOGI("%s,%d E", __func__, __LINE__);
    return;
}

int autotest_flashlight_ctrl(uint32_t flash_status) {
    int ret = 0;

    ALOGI("%s,%d E", __func__, __LINE__);

    return ret;
}

void autotest_camera_cb(enum camera_cb_type cb, const void *client_data,
                        enum camera_func_type func, void *parm4) {
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;

    ALOGI("%s,%d E", __func__, __LINE__);

    if (!frame) {
        ALOGV("%s,%d, callback with no frame", __func__, __LINE__);
        return;
    }

    if (CAMERA_FUNC_START_PREVIEW != func) {
        ALOGV("%s,%d, camera func type error: %d, do nothing", __func__,
              __LINE__, func);
        return;
    }

    if (CAMERA_EVT_CB_FRAME != cb) {
        ALOGV("%s,%d, camera cb type error: %d, do nothing", __func__, __LINE__,
              cb);
        return;
    }

    /*get frame buffer id*/
    target_buffer_id = get_preview_buffer_id_for_fd(frame->fd);
    ALOGD("%s, target_buffer_id: %d", __func__, target_buffer_id);
    /* if (0xFFFFFFFF == target_buffer_id) {
         ALOGW("buffer id is invalid, do nothing");
         return;
     }*/

    /*preview enable or disable?*/
    if (!preview_valid) {
        ALOGI("%s,%d, preview disabled, do nothing", __func__, __LINE__);
        return;
    }

    m_hal_oem->ops->camera_set_preview_buffer(
        oem_handle, (cmr_uint)preview_heap_array[target_buffer_id]->phys_addr,
        (cmr_uint)preview_heap_array[target_buffer_id]->data,
        (cmr_s32)preview_heap_array[target_buffer_id]->fd);

    frame_num++;
}

static void free_camera_mem(sprd_camera_memory_t *memory) {
    ALOGI("%s,%d E", __func__, __LINE__);

    if (!memory)
        return;

    if (memory->ion_heap) {
        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
}

static int callback_other_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    int i;
    ALOGI("%s,%d E", __func__, __LINE__);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != m_preview_heap_reserved) {
            free_camera_mem(m_preview_heap_reserved);
            m_preview_heap_reserved = NULL;
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != m_isp_lsc_heap_reserved) {
            free_camera_mem(m_isp_lsc_heap_reserved);
            m_isp_lsc_heap_reserved = NULL;
        }
    }
    if (type == CAMERA_ISP_STATIS) {
        if (NULL != m_isp_statis_heap_reserved) {
            m_isp_statis_heap_reserved->ion_heap->free_kaddr();
            free_camera_mem(m_isp_statis_heap_reserved);
            m_isp_statis_heap_reserved = NULL;
        }
    }
    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < k_isp_b4_awb_count; i++) {
            if (NULL != m_isp_b4_awb_heap_reserved[i]) {
                free_camera_mem(m_isp_b4_awb_heap_reserved[i]);
                m_isp_b4_awb_heap_reserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != m_isp_firmware_reserved) {
            free_camera_mem(m_isp_firmware_reserved);
            m_isp_firmware_reserved = NULL;
        }
    }

    if (type == CAMERA_ISP_RAWAE) {
        for (i = 0; i < k_isp_b4_awb_count; i++) {
            if (NULL != m_isp_raw_aem_heap_reserved[i]) {
                m_isp_raw_aem_heap_reserved[i]->ion_heap->free_kaddr();
                free_camera_mem(m_isp_raw_aem_heap_reserved[i]);
            }
            m_isp_raw_aem_heap_reserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != m_isp_afl_heap_reserved) {
            free_camera_mem(m_isp_afl_heap_reserved);
            m_isp_afl_heap_reserved = NULL;
        }
    }

    return 0;
}

static int callback_preview_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                 cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    ALOGI("%s,%d E", __func__, __LINE__);

    /*lock*/
    preview_lock.lock();

    for (i = 0; i < m_preview_heap_num; i++) {
        if (!preview_heap_array[i])
            continue;

        free_camera_mem(preview_heap_array[i]);
        preview_heap_array[i] = NULL;
    }

    m_preview_heap_num = 0;

    /*unlock*/
    preview_lock.unlock();

    return 0;
}

static cmr_int callback_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data) {
    cmr_int ret = 0;

    ALOGI("%s,%d E", __func__, __LINE__);

    if (!private_data || !vir_addr || !fd) {
        ALOGE("%s,%d, error param 0x%lx 0x%lx 0x%lx", __func__, __LINE__,
              (cmr_uint)phy_addr, (cmr_uint)vir_addr, (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        ALOGE("%s,%d, mem type error %d", __func__, __LINE__, type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = callback_preview_free(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = callback_other_free(type, phy_addr, vir_addr, fd, sum);
    } else {
        ALOGE("%s,%s,%d, type ignore: %d, do nothing.", __FILE__, __func__,
              __LINE__, type);
    }

    /* disable preview flag */
    preview_valid = 0;

    return ret;
}

static sprd_camera_memory_t *alloc_camera_mem(int buf_size, int num_bufs,
                                              uint32_t is_cache) {

    size_t mem_size = 0;
    MemIon *p_heap_ion = NULL;

    ALOGI("%s,%s,%d E", __FILE__, __func__, __LINE__);
    ALOGI("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        ALOGE("%s,%d, failed: fatal error! memory pointer is null.", __func__,
              __LINE__);
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        ALOGE("%s,%d, failed: mem size err.", __func__, __LINE__);
        goto getpmem_fail;
    }

    if (0 == s_mem_method) {
        ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
        if (is_cache) {
            p_heap_ion = new MemIon("/dev/ion", mem_size, 0,
                                    (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            p_heap_ion = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                    ION_HEAP_ID_MASK_MM);
        }
    } else {
        ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
        if (is_cache) {
            p_heap_ion = new MemIon("/dev/ion", mem_size, 0,
                                    (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            p_heap_ion = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                    ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (p_heap_ion == NULL || p_heap_ion->getHeapID() < 0) {
        ALOGE("%s,%s,%d, failed: p_heap_ion is null or getHeapID "
              "failed.",
              __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }

    if (NULL == p_heap_ion->getBase() || MAP_FAILED == p_heap_ion->getBase()) {
        ALOGE("error getBase is null. %s,%s,%d, failed: ion get base "
              "err.",
              __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }
    ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);

    memory->ion_heap = p_heap_ion;
    memory->fd = p_heap_ion->getHeapID();
    memory->dev_fd = p_heap_ion->getIonDeviceFd();
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = p_heap_ion->getBase();

    if (0 == s_mem_method) {
        ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              p_heap_ion);
    } else {
        ALOGD("iommu: fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, "
              "heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              p_heap_ion);
    }

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

    ALOGI("%s,%d E", __func__, __LINE__);
    ALOGI("size %d sum %d m_preview_heap_num %d", size, sum,
          m_preview_heap_num);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = alloc_camera_mem(size, 1, true);
        if (!memory) {
            ALOGE("%s,%d, failed: alloc camera mem err.", __func__, __LINE__);
            goto mem_fail;
        }
        preview_heap_array[i] = memory;
        m_preview_heap_num++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    callback_preview_free(0, 0, 0, 0);
    return -1;
}

static int callback_other_malloc(enum camera_mem_cb_type type, cmr_u32 size,
                                 cmr_u32 sum, cmr_uint *phy_addr,
                                 cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;

    ALOGI("%s,%s,%d E", __FILE__, __func__, __LINE__);
    ALOGD("size %d sum %d m_preview_heap_num %d, type %d", size, sum,
          m_preview_heap_num, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == m_preview_heap_reserved) {
            ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                ALOGE("%s,%d, failed: alloc camera mem err.", __func__,
                      __LINE__);
                LOGE("error memory is null.");
                goto mem_fail;
            }
            m_preview_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
            ALOGI("malloc Common memory for preview, video, and zsl, malloced "
                  "type %d,request num %d, request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)m_preview_heap_reserved->phys_addr;
            *vir_addr++ = (cmr_uint)m_preview_heap_reserved->data;
            *fd++ = m_preview_heap_reserved->fd;
        }
    } else if (type == CAMERA_ISP_LSC) {
        if (m_isp_lsc_heap_reserved == NULL) {
            ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("%s,%d, failed: alloc camera mem err.", __func__,
                      __LINE__);
                ALOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            m_isp_lsc_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp lsc memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)m_isp_lsc_heap_reserved->phys_addr;
            *vir_addr++ = (cmr_uint)m_isp_lsc_heap_reserved->data;
            *fd++ = m_isp_lsc_heap_reserved->fd;
        }
    } else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (m_isp_statis_heap_reserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("memory is null");
                goto mem_fail;
            }
            m_isp_statis_heap_reserved = memory;
        }

#if defined(CONFIG_ISP_2_6)
// sharkl5 dont have get_kaddr interface
// m_isp_statis_heap_reserved->ion_heap->get_kaddr(&kaddr, &ksize);
#else
        m_isp_statis_heap_reserved->ion_heap->get_kaddr(&kaddr, &ksize);
#endif
        *phy_addr++ = kaddr;
        *phy_addr = kaddr >> 32;
        *vir_addr++ = (cmr_uint)m_isp_statis_heap_reserved->data;
        *fd++ = m_isp_statis_heap_reserved->fd;
        *fd++ = m_isp_statis_heap_reserved->dev_fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("%s,%d, failed: alloc camera mem err.", __func__,
                      __LINE__);
                goto mem_fail;
            }
            m_isp_b4_awb_heap_reserved[i] = memory;
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
            ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            m_isp_raw_aem_heap_reserved[i] = memory;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr_64++ = kaddr;
            *vir_addr_64++ = (cmr_u64)(memory->data);
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (m_isp_afl_heap_reserved == NULL) {
            ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("%s,%d, failed: alloc camera mem err.", __func__,
                      __LINE__);
                goto mem_fail;
            }
            m_isp_afl_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp afl memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)m_isp_afl_heap_reserved->phys_addr;
            *vir_addr++ = (cmr_uint)m_isp_afl_heap_reserved->data;
            *fd++ = m_isp_afl_heap_reserved->fd;
        }
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (m_isp_firmware_reserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("%s,%d, failed: alloc camera mem err.", __func__,
                      __LINE__);
                goto mem_fail;
            }
            m_isp_firmware_reserved = memory;
        } else {
            memory = m_isp_firmware_reserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else {
        ALOGE("%s,%s,%d, type ignore: %d, do nothing.", __FILE__, __func__,
              __LINE__, type);
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

    ALOGI("%s,%s,%d type %d E", __FILE__, __func__, __LINE__, type);

    /*lock*/
    preview_lock.lock();

    if (!phy_addr || !vir_addr || !size_ptr || !sum_ptr || (0 == *size_ptr) ||
        (0 == *sum_ptr)) {
        ALOGE("%s,%d, alloc param error 0x%lx 0x%lx 0x%lx", __func__, __LINE__,
              (cmr_uint)phy_addr, (cmr_uint)vir_addr, (cmr_uint)size_ptr);
        /*unlock*/
        preview_lock.unlock();
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (CAMERA_PREVIEW == type) {
        /* preview buffer */
        ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
        ret = callback_preview_malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ALOGI("%s,%s,%d", __FILE__, __func__, __LINE__);
        ret = callback_other_malloc(type, size, sum, phy_addr, vir_addr, fd);
    } else {
        ALOGE("%s,%s,%d, type ignore: %d, do nothing.", __FILE__, __func__,
              __LINE__, type);
    }

    /* enable preview flag */
    preview_valid = 1;

    /*unlock*/
    preview_lock.unlock();

    return ret;
}

static void autotest_camera_startpreview(void) {
    cmr_int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;

    struct cmr_range_fps_param fps_param;

    if (!oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops)
        return;

    ALOGI("%s,%d E", __func__, __LINE__);

    preview_size.width = PREVIEW_WIDTH;
    preview_size.height = PREVIEW_HIGHT;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio =
        (float)PREVIEW_WIDTH / PREVIEW_HIGHT;

    fps_param.is_recording = 0;
    fps_param.min_fps = 5;
    fps_param.max_fps = 30;
    fps_param.video_mode = 0;

    m_hal_oem->ops->camera_fast_ctrl(oem_handle, CAMERA_FAST_MODE_FD, 0);

    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_PREVIEW_FORMAT,
             CAMERA_DATA_FORMAT_YUV420);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    /* set malloc && free callback*/
    ret = m_hal_oem->ops->camera_set_mem_func(
        oem_handle, (void *)callback_malloc, (void *)callback_free, NULL);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("%s,%d, failed: camera set mem func error.", __func__, __LINE__);
        return;
    }

    /*start preview*/
    ret = m_hal_oem->ops->camera_start_preview(oem_handle, CAMERA_NORMAL_MODE);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("%s,%d, failed: camera start preview error.", __func__, __LINE__);
        return;
    }
}

static int autotest_camera_stoppreview(void) {
    int ret;

    ALOGI("%s,%d E", __func__, __LINE__);

    if (!oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops) {
        ALOGI("oem is null or oem ops is null, do nothing");
        return -1;
    }

    ret = m_hal_oem->ops->camera_stop_preview(oem_handle);

    return ret;
}

extern "C" {
int autotest_camera_deinit() {
    cmr_int ret;

    ALOGI("%s,%d E", __func__, __LINE__);

    ret = autotest_camera_stoppreview();

    if (oem_handle != NULL && m_hal_oem != NULL && m_hal_oem->ops != NULL) {
        ret = m_hal_oem->ops->camera_deinit(oem_handle);
    }

    if (NULL != m_hal_oem && NULL != m_hal_oem->dso) {
        dlclose(m_hal_oem->dso);
    }
    if (NULL != m_hal_oem) {
        free((void *)m_hal_oem);
        m_hal_oem = NULL;
    }

    return ret;
}

static cmr_int autotest_load_hal_lib(void) {
    int ret = 0;
    if (!m_hal_oem) {
        oem_module_t *omi;
        m_hal_oem = (oem_module_t *)malloc(sizeof(oem_module_t));
        m_hal_oem->dso = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
        if (NULL == m_hal_oem->dso) {
            char const *err_str = dlerror();
            ALOGE("dlopen error%s", err_str ? err_str : "unknown");
            ret = -1;
            goto loaderror;
        }

        /* Get the address of the struct hal_module_info. */
        const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
        omi = (oem_module_t *)dlsym(m_hal_oem->dso, sym);
        if (omi == NULL) {
            ALOGE("load: couldn't find symbol %s", sym);
            ret = -1;
            goto loaderror;
        }

        m_hal_oem->ops = omi->ops;

        ALOGV("loaded HAL libcamoem m_hal_oem->dso = %p", m_hal_oem->dso);
    }

    return ret;

loaderror:

    if (NULL != m_hal_oem->dso) {
        dlclose(m_hal_oem->dso);
    }
    free((void *)m_hal_oem);
    m_hal_oem = NULL;
    return ret;
}

int autotest_camera_init(int camera_id) {
    int ret = 0;

    ALOGI("%s,%d E, camera_id %d", __func__, __LINE__, camera_id);

    ret = autotest_load_hal_lib();
    if (ret) {
        ALOGE("autotest_load_hal_lib error");
        return -1;
    }

    ret = m_hal_oem->ops->camera_init(
        camera_id, autotest_camera_cb, &client_data, 0, &oem_handle,
        (void *)callback_malloc, (void *)callback_free);
    if (ret) {
        ALOGE("Native MMI Test: camera_init failed, ret=%d", ret);
        goto exit;
    }
    s_mem_method = autotest_iommu_is_enabled();
    ALOGI("%s,%s,%d， s_mem_method %d", __FILE__, __func__, __LINE__,
          s_mem_method);

    autotest_camera_startpreview();

exit:
    return ret;
}

int autotest_read_cam_buf(void **pp_image_addr, int size, int *p_out_size) {
    ALOGI("%s,%d E, target_buffer_id: %d", __func__, __LINE__,
          target_buffer_id);
    cmr_uint vir_addr;

    if (!preview_heap_array[target_buffer_id]) {
        ALOGE("preview_heap_array error");
        return -1;
    }
    vir_addr = (cmr_uint)preview_heap_array[target_buffer_id]
                   ->ion_heap->getBase(); // virtual address
    *p_out_size = preview_heap_array[target_buffer_id]->phys_size; // image size
    memcpy((void *)*pp_image_addr, (void *)vir_addr, size);

    ALOGI("%s,%d X", __func__, __LINE__);
    return 1;
}
}

int flashlightSetValue(int value) {

    int ret = 0;
    char cmd[200] = " ";

    sprintf(cmd, "echo 0x%02x > /sys/class/misc/sprd_flash/test", value);
    ret = system(cmd) ? -1 : 0;
    ALOGD("cmd = %s,ret = %d", cmd, ret);

    return ret;
}

/*
    7E 49 00 00 00 0A 00 38 0C 08 01 7E    // White light on
    7E 49 00 00 00 0A 00 38 0C 04 01 7E    // Back cold light on
    7E 00 00 00 00 0A 00 38 0C 04 02 7E    // Color temperature light on
      buf[10] express turn off the light

    you can use that cmd to control light after adb shell enter
    echo 0x72 > /sys/class/misc/sprd_flash/test  // White light on
    echo 0x20 > /sys/class/misc/sprd_flash/test  // Color temperature light on
    echo 0x00 > /sys/class/misc/sprd_flash/test  // Turn off the light
*/
int autotest_flash(char *buf, int buf_len, char *rsp, int rsp_size) {

    ALOGD("autotest_flash 0x%02x", buf[10]);
    int ret = 0;
    if (buf[10] == 0x01) {
        ALOGD("open back flash");
        ret = flashlightSetValue(0x10); // Back cold light on
    } else if (buf[10] == 0x02) {
        ALOGD("open temple flash");
        ret = flashlightSetValue(0x20); // Color temperature light on
    } else if (buf[10] == 0x00) {
        ALOGD("close flash");
        ret = flashlightSetValue(0x31); // Turn off the light
    } else {
        ALOGE("undefined cmd");
    }

    /*----------------------后续代码为通用代码，所有模块可以直接复制-----------------*/
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);

    p_msg_head->len = 8;

    ALOGD("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    }
    ALOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
          rsp[sizeof(MSG_HEAD_T)]);
    rsp[p_msg_head->len + 2 - 1] = 0x7E; //加上数据尾标志
    ALOGD("dylib test :return len:%d", p_msg_head->len + 2);
    ALOGD("engpc->pc flash:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1],
          rsp[2], rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8], rsp[9]);

    return p_msg_head->len + 2;
    /*----------------------如上虚线之间代码为通用代码，直接赋值即可-----------------*/
}

int autotest_front_flash(char *buf, int buf_len, char *rsp, int rsp_size) {

    ALOGD("front_flash 0x%02x", buf[10]);
    int ret = 0;

    if (buf[10] == 0x01) {
        ALOGD("open front flash");
        ret = flashlightSetValue(0x72); // Front cold light on
    } else if (buf[10] == 0x00) {
        ALOGD("close front flash");
        ret = flashlightSetValue(0x00); // Turn off the light
    } else {
        ALOGE("undefined cmd");
    }

    /*----------------------后续代码为通用代码，所有模块可以直接复制-----------------*/
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);

    p_msg_head->len = 8;

    ALOGD("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    }
    ALOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
          rsp[sizeof(MSG_HEAD_T)]);
    rsp[p_msg_head->len + 2 - 1] = 0x7E; //加上数据尾标志
    ALOGD("dylib test :return len:%d", p_msg_head->len + 2);
    ALOGD("engpc->pc flash:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1],
          rsp[2], rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8], rsp[9]);

    return p_msg_head->len + 2;
    /*----------------------如上虚线之间代码为通用代码，直接赋值即可-----------------*/
}

int autotest_mipicam(char *buf, int buf_len, char *rsp, int rsp_size) {

    int ret = 0;
    int fun_ret = 0;
#if 1
    int rec_image_size = 0;
#define CAM_IDX 1
#define SBUFFER_SIZE (600 * 1024)
    cmr_u32 *p_buf = NULL;
    p_buf = new cmr_u32[600 * 1024];
    static int sensor_id = 0; // 0:back  ,1:front

    switch (buf[9]) {
    case 1:
        autotest_load_hal_lib();
        fun_ret = autotest_camera_init(sensor_id);
        if (fun_ret < 0) {
            ret = -1;
        }
        break;
    case 2:
        ALOGI("%s,%d, rsp_size: %d", __func__, __LINE__, rsp_size);
        fun_ret = autotest_read_cam_buf((void **)&p_buf, SBUFFER_SIZE,
                                        &rec_image_size);
        if (fun_ret < 0) {
            ALOGI("%s,%d, rec_image_size: %d", __func__, __LINE__,
                  rec_image_size);
            ret = -1;
        } else {
            ALOGI("%s,%d, rec_image_size: %d", __func__, __LINE__,
                  rec_image_size);
            if (rec_image_size > rsp_size - 1) {
                memcpy(rsp, p_buf, 768);
                ret = 768; // rsp_size-1;
            } else {
                memcpy(rsp, p_buf, 768);
                ret = 768; // rec_image_size;
            }
        }
        break;
    case 3:
        fun_ret = autotest_camera_deinit();
        if (fun_ret < 0) {
            ret = -1;
        }
        break;
    case 4:
        sensor_id = buf[10];
        break;
    default:
        break;
    }

    /*------------------------------后续代码为通用代码，所有模块可以直接复制，------------------------------------------*/
    //填充协议头7E xx xx xx xx 08 00 38
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1); //复制7E xx xx xx xx 08 00
    // 38至rsp,，减1是因为返回值中不包含MSG_HEAD_T的subtype（38后面的数据即为subtype）
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);

    //修改返回数据的长度，如上复制的数据长度等于PC发送给engpc的数据长度，现在需要改成engpc返回给pc的数据长度。
    p_msg_head->len = 8; //返回的最基本数据格式：7E xx xx xx xx 08 00 38 xx
    // 7E，去掉头和尾的7E，共8个数据。如果需要返回数据,则直接在38
    // XX后面补充

    //填充成功或失败标志位rsp[1 +
    // sizeof(MSG_HEAD_T)]，如果ret>0,则继续填充返回的数据。
    ALOGD("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1; // 38 01 表示测试失败。
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0; // 38 00 表示测试ok
    } else if (ret > 0) {
        // 38 00 表示测试ok,ret > 0,表示需要返回 ret个字节数据。
        rsp[sizeof(MSG_HEAD_T)] = 0;
        //将获取到的ret个数据，复制到38 00 后面
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), p_buf, ret);
        //返回长度：基本数据长度8+获取到的ret个字节数据长度。
        p_msg_head->len += ret;
    }
    ALOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
          rsp[sizeof(MSG_HEAD_T)]);
    //填充协议尾部的7E
    rsp[p_msg_head->len + 2 - 1] = 0x7E; //加上数据尾标志
    ALOGD("dylib test :return len:%d", p_msg_head->len + 2);
    ALOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1], rsp[2],
          rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8],
          rsp[9]); // 78 xx xx xx xx _ _38 _ _ 打印返回的10个数据

    return p_msg_head->len + 2;
/*------------------------------如上虚线之间代码为通用代码，直接赋值即可---------*/

#endif
    delete[] p_buf;
    return ret;
}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num) {
    int moudles_num = 0; //用于表示注册的节点的数目。

    reg->type = 0x38;
    reg->subtype = 0x06;
    reg->eng_diag_func = autotest_mipicam;
    moudles_num++;

    (reg + 1)->type = 0x38;
    (reg + 1)->subtype = 0x0C;
    (reg + 1)->diag_ap_cmd = 0x04;
    (reg + 1)->eng_diag_func = autotest_flash;

    (reg + 2)->type = 0x38;
    (reg + 2)->subtype = 0x0C;
    (reg + 2)->diag_ap_cmd = 0x08;
    (reg + 2)->eng_diag_func = autotest_front_flash;
    moudles_num++;

    *num = moudles_num;
    ALOGD("register_this_module_ext: %d - %d", *num, moudles_num);
}
