/*************************************************************************
	> File Name: cpat_camera.cpp
	> Author:
	> Mail:
	> Created Time: Thu 18 Jun 2020 04:23:27 PM CST
 ************************************************************************/

#define LOG_TAG "cpat_camera"

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
#include <utils/Mutex.h>
#include "MemIon.h"
#include "sprd_fts_type.h"

#include "cpat_camera.h"

using namespace android;

#define PREVIEW_WIDTH 960
#define PREVIEW_HIGHT 720
#define PREVIEW_BUFF_NUM 8
#define SPRD_MAX_PREVIEW_BUF PREVIEW_BUFF_NUM

#define SET_PARM(h, x, y, z)                                                   \
    do {                                                                       \
        if (h != NULL && h->ops != NULL)                                       \
            h->ops->camera_set_param(x, y, z);                                 \
    } while (0)


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

static Mutex preview_lock;
static int preview_valid = 0;
static int is_iommu_enabled = 0;
static unsigned char camera_id = 0;
static uint32_t frame_has_callback = 0;
static unsigned int m_preview_heap_num = 0;
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
static cmr_handle oem_handle = 0;
static uint32_t lcd_w = 0, lcd_h = 0;

struct client_t {
    int reserved;
};
static struct client_t client_data;

static int cpat_get_iommu_is_enabled(void);
static unsigned int cpat_get_preview_buf_id_for_fd(cmr_s32 fd);
void cpat_camera_cb(enum camera_cb_type cb, const void *client_data,
                        enum camera_func_type func, void *parm4);
static void cpat_free_camera_mem(sprd_camera_memory_t *memory);
static int cpat_callback_other_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum);
static int cpat_callback_preview_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                 cmr_s32 *fd, cmr_u32 sum);
static cmr_int cpat_callback_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data);
static sprd_camera_memory_t *cpat_alloc_camera_mem(int buf_size, int num_bufs,
                                              uint32_t is_cache);
static int cpat_callback_preview_malloc(cmr_u32 size, cmr_u32 sum,
                                   cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd);
static int cpat_callback_other_malloc(enum camera_mem_cb_type type, cmr_u32 size,
                                 cmr_u32 sum, cmr_uint *phy_addr,
                                 cmr_uint *vir_addr, cmr_s32 *fd);
static cmr_int cpat_callback_malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                               cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd,
                               void *private_data);
static int  cpat_load_hal_lib(void);

static int cpat_get_iommu_is_enabled(void) {
    int ret;
    int iommu_is_enabled = 0;

    if (NULL == oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops) {
        CMR_LOGE("oem is null or oem ops is null");
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

static unsigned int cpat_get_preview_buf_id_for_fd(cmr_s32 fd) {
    unsigned int i = 0;

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

void cpat_camera_cb(enum camera_cb_type cb, const void *client_data,
                        enum camera_func_type func, void *parm4) {
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;

    if (!frame) {
        CMR_LOGE("callback with no frame");
        return;
    }

    if (CAMERA_FUNC_START_PREVIEW != func) {
        CMR_LOGE("camera func type error: %d, do nothing", func);
        return;
    }

    if (CAMERA_EVT_CB_FRAME != cb) {
        CMR_LOGE("camera cb type error: %d, do nothing", cb);
        return;
    }

    target_buffer_id = cpat_get_preview_buf_id_for_fd(frame->fd);
    CMR_LOGI("target_buffer_id: %d", target_buffer_id);

    if (!frame_has_callback) {
        frame_has_callback = 1;
    }

    if (!preview_valid) {
        CMR_LOGE("preview disabled, do nothing");
        return;
    }

    m_hal_oem->ops->camera_set_preview_buffer(
        oem_handle, (cmr_uint)preview_heap_array[target_buffer_id]->phys_addr,
        (cmr_uint)preview_heap_array[target_buffer_id]->data,
        (cmr_s32)preview_heap_array[target_buffer_id]->fd);

}

static void cpat_free_camera_mem(sprd_camera_memory_t *memory) {
    CMR_LOGI("E");

    if (!memory)
        return;

    if (memory->ion_heap) {
        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
    CMR_LOGI("X");
}

static int cpat_callback_other_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                        cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    int i;
    CMR_LOGI("E");

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != m_preview_heap_reserved) {
            cpat_free_camera_mem(m_preview_heap_reserved);
            m_preview_heap_reserved = NULL;
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != m_isp_lsc_heap_reserved) {
            cpat_free_camera_mem(m_isp_lsc_heap_reserved);
            m_isp_lsc_heap_reserved = NULL;
        }
    }
    if (type == CAMERA_ISP_STATIS) {
        if (NULL != m_isp_statis_heap_reserved) {
            m_isp_statis_heap_reserved->ion_heap->free_kaddr();
            cpat_free_camera_mem(m_isp_statis_heap_reserved);
            m_isp_statis_heap_reserved = NULL;
        }
    }
    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < k_isp_b4_awb_count; i++) {
            if (NULL != m_isp_b4_awb_heap_reserved[i]) {
                cpat_free_camera_mem(m_isp_b4_awb_heap_reserved[i]);
                m_isp_b4_awb_heap_reserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != m_isp_firmware_reserved) {
            cpat_free_camera_mem(m_isp_firmware_reserved);
            m_isp_firmware_reserved = NULL;
        }
    }

    if (type == CAMERA_ISP_RAWAE) {
        for (i = 0; i < k_isp_b4_awb_count; i++) {
            if (NULL != m_isp_raw_aem_heap_reserved[i]) {
                m_isp_raw_aem_heap_reserved[i]->ion_heap->free_kaddr();
                cpat_free_camera_mem(m_isp_raw_aem_heap_reserved[i]);
            }
            m_isp_raw_aem_heap_reserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != m_isp_afl_heap_reserved) {
            cpat_free_camera_mem(m_isp_afl_heap_reserved);
            m_isp_afl_heap_reserved = NULL;
        }
    }

    return 0;
}

static int cpat_callback_preview_free(cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_s32 *fd,
                          cmr_u32 sum) {
    cmr_u32 i;

    CMR_LOGI("E");

    preview_lock.lock();

    for (i = 0; i < m_preview_heap_num; i++) {
        if (!preview_heap_array[i])
            continue;

        cpat_free_camera_mem(preview_heap_array[i]);
        preview_heap_array[i] = NULL;
    }

    m_preview_heap_num = 0;

    preview_lock.unlock();

    return 0;
}

static cmr_int cpat_callback_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                      cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                      void *private_data) {
    cmr_int ret = 0;

    CMR_LOGI("E");

    if (!private_data || !vir_addr || !fd) {
        CMR_LOGE("error param 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr,
                 (cmr_uint)vir_addr, (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        CMR_LOGE("mem type error %d", type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = cpat_callback_preview_free(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = cpat_callback_other_free(type, phy_addr, vir_addr, fd, sum);
    } else {
        CMR_LOGE("type ignore: %d, do nothing.", type);
    }

    preview_valid = 0;

    return ret;
}

static sprd_camera_memory_t *cpat_alloc_camera_mem(int buf_size, int num_bufs,
                                       uint32_t is_cache) {

    size_t mem_size = 0;
    MemIon *p_heap_ion = NULL;

    CMR_LOGI("E buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        CMR_LOGE("failed: fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        CMR_LOGE("failed: mem size err.");
        goto getpmem_fail;
    }

    CMR_LOGI("is_iommu_enabled =%d", is_iommu_enabled);
    if (is_iommu_enabled == 0) {
        if (is_cache) {
            p_heap_ion = new MemIon("/dev/ion", mem_size, 0,
                                    (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            p_heap_ion = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                    ION_HEAP_ID_MASK_MM);
        }
    } else {
        if (is_cache) {
            p_heap_ion = new MemIon("/dev/ion", mem_size, 0,
                                    (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            p_heap_ion = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                    ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (p_heap_ion == NULL || p_heap_ion->getHeapID() < 0) {
        CMR_LOGE("failed: p_heap_ion is null or getHeapID failed.");
        goto getpmem_fail;
    }

    if (NULL == p_heap_ion->getBase() || MAP_FAILED == p_heap_ion->getBase()) {
        CMR_LOGE("error getBase is null or ion get base error");
        goto getpmem_fail;
    }

    memory->ion_heap = p_heap_ion;
    memory->fd = p_heap_ion->getHeapID();
    memory->dev_fd = p_heap_ion->getIonDeviceFd();
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = p_heap_ion->getBase();

    CMR_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, "
             "heap=%p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             p_heap_ion);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

static int cpat_callback_preview_malloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                            cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;
    CMR_LOGI("E size %d sum %d m_preview_heap_num %d", size, sum,
             m_preview_heap_num);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = cpat_alloc_camera_mem(size, 1, true);
        if (!memory) {
            CMR_LOGE("failed: alloc camera mem err.");
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
    cpat_callback_preview_free(0, 0, 0, 0);
    return -1;
}

static int cpat_callback_other_malloc(enum camera_mem_cb_type type, cmr_u32 size,
                          cmr_u32 sum, cmr_uint *phy_addr, cmr_uint *vir_addr,
                          cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;

    CMR_LOGI("E size %d sum %d m_preview_heap_num %d, type %d", size, sum,
             m_preview_heap_num, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == m_preview_heap_reserved) {
            memory = cpat_alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                CMR_LOGE("failed: alloc camera mem err.");
                goto mem_fail;
            }
            m_preview_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            CMR_LOGI(
                "malloc Common memory for preview, video, and zsl, malloced "
                "type %d,request num %d, request size 0x%x",
                type, sum, size);
            *phy_addr++ = (cmr_uint)m_preview_heap_reserved->phys_addr;
            *vir_addr++ = (cmr_uint)m_preview_heap_reserved->data;
            *fd++ = m_preview_heap_reserved->fd;
        }
    } else if (type == CAMERA_ISP_LSC) {
        if (m_isp_lsc_heap_reserved == NULL) {
            memory = cpat_alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("failed: alloc camera mem err. type:%d", type);
                goto mem_fail;
            }
            m_isp_lsc_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            CMR_LOGI("malloc isp lsc memory, malloced type %d,request num %d, "
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
            memory = cpat_alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("memory is null");
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
            memory = cpat_alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("failed: alloc camera mem err.");
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
            memory = cpat_alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("error memory is null,malloced type %d", type);
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
            memory = cpat_alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("failed: alloc camera mem err.");
                goto mem_fail;
            }
            m_isp_afl_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            CMR_LOGI("malloc isp afl memory, malloced type %d,request num %d, "
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
            memory = cpat_alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("failed: alloc camera mem err.");
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
        CMR_LOGE("type ignore: %d, do nothing.", type);
    }

    return 0;

mem_fail:
    cpat_callback_other_free(type, 0, 0, 0, 0);
    return -1;
}

static cmr_int cpat_callback_malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                        cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                        cmr_uint *vir_addr, cmr_s32 *fd, void *private_data) {
    cmr_int ret = 0;
    cmr_u32 size;
    cmr_u32 sum;

    CMR_LOGI("%d E", type);

    preview_lock.lock();

    if (!phy_addr || !vir_addr || !size_ptr || !sum_ptr || (0 == *size_ptr) ||
        (0 == *sum_ptr)) {
        CMR_LOGE("alloc param error 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr,
                 (cmr_uint)vir_addr, (cmr_uint)size_ptr);
        preview_lock.unlock();
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    CMR_LOGI("callback type: %d", type);
    if (CAMERA_PREVIEW == type) {
        ret = cpat_callback_preview_malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = cpat_callback_other_malloc(type, size, sum, phy_addr, vir_addr, fd);
    } else {
        CMR_LOGE("type ignore: %d, do nothing.", type);
    }

    preview_valid = 1;

    preview_lock.unlock();

    return ret;
}

static int  cpat_load_hal_lib(void) {
    int ret = 0;

    if (!m_hal_oem) {
        oem_module_t *omi;
        m_hal_oem = (oem_module_t *)malloc(sizeof(oem_module_t));
        m_hal_oem->dso = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
        if (NULL == m_hal_oem->dso) {
            char const *err_str = dlerror();
            CMR_LOGE("dlopen error%s", err_str ? err_str : "unknown");
            ret = -1;
            goto loaderror;
        }

        /* Get the address of the struct hal_module_info */
        const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
        omi = (oem_module_t *)dlsym(m_hal_oem->dso, sym);
        if (omi == NULL) {
            CMR_LOGE("load: couldn't find symbol %s", sym);
            ret = -1;
            goto loaderror;
        }

        m_hal_oem->ops = omi->ops;

        CMR_LOGI("loaded HAL libcamoem m_hal_oem->dso = %p", m_hal_oem->dso);
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

int cpat_camera_init(int camera_id) {
    int ret = 0;

    CMR_LOGI("E, camera_id %d", camera_id);

    ret = cpat_load_hal_lib();
    if (ret) {
        CMR_LOGE("cpat_load_hal_lib error");
        return -1;
    }

    frame_has_callback = 0;

    ret = m_hal_oem->ops->camera_init(
        camera_id, cpat_camera_cb, &client_data, 0, &oem_handle,
        (void *)cpat_callback_malloc, (void *)cpat_callback_free);
    if (ret) {
        CMR_LOGE("Native MMI Test: camera_init failed, ret=%d", ret);
        goto exit;
    }

    property_set("vendor.cam.recovery.on", "0");

    is_iommu_enabled = cpat_get_iommu_is_enabled();
    CMR_LOGI("is_iommu_enabled=%d", is_iommu_enabled);

    CMR_LOGI("X");
exit:
    return ret;
}

int cpat_camera_deinit(void) {
    cmr_int ret = 0;

    CMR_LOGI("E");

    frame_has_callback = 0;

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

    CMR_LOGI("X");
    return ret;
}


int cpat_camera_startpreview(void) {
    cmr_int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;
    struct cmr_range_fps_param fps_param;
    memset(&zoom_param, 0, sizeof(struct cmr_zoom_param));

    CMR_LOGI("E");

    if (!oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops)
        return -1;

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
             IMG_DATA_TYPE_YUV420);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    ret = m_hal_oem->ops->camera_set_mem_func(
        oem_handle, (void *)cpat_callback_malloc, (void *)cpat_callback_free, NULL);
    if (ret) {
        CMR_LOGE("camera_set_mem_func failed");
        return -1;
    }

    ret = m_hal_oem->ops->camera_start_preview(oem_handle, CAMERA_NORMAL_MODE);
    if (ret) {
        CMR_LOGE("camera_start_preview failed");
        return -1;
    }

    CMR_LOGI("X");
    return ret;
}

int cpat_camera_stoppreview(void) {
    int ret = 0;

    CMR_LOGI("E");

    if (!oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops) {
        CMR_LOGI("oem is null or oem ops is null, do nothing");
        return -1;
    }

    ret = m_hal_oem->ops->camera_stop_preview(oem_handle);
    if (ret) {
        CMR_LOGE("camera_stop_preview failed");
        return -1;
    }

    CMR_LOGI("X");
    return ret;
}

int cpat_read_cam_buf(void **pp_image_addr, int size, int *p_out_size) {
    cmr_uint vir_addr, tmp;
    uint32_t i = 0;

    CMR_LOGI("E, target_buffer_id: %d", target_buffer_id);

    if (!preview_heap_array[target_buffer_id]) {
        CMR_LOGE("preview_heap_array error");
        return -1;
    }

    for (i = 0; i < 10; i++) {
        if(frame_has_callback)
            break;
        usleep(200 * 1000);
        if(9 == i) {
            CMR_LOGE("wait for 2s, there is no sensor image!");
            return -1;
        }
    }

    vir_addr =
        (cmr_uint)preview_heap_array[target_buffer_id]->ion_heap->getBase();

#if 0
    tmp = vir_addr;
    for (i = 0; i < 20; i++) {
        ALOGI("read y: %d %d %d %d %d %d %d %d %d %d", *(uint8_t *)tmp,
              *(uint8_t *)(tmp + 1), *(uint8_t *)(tmp + 2),
              *(uint8_t *)(tmp + 3), *(uint8_t *)(tmp + 4),
              *(uint8_t *)(tmp + 5), *(uint8_t *)(tmp + 6),
              *(uint8_t *)(tmp + 7), *(uint8_t *)(tmp + 8),
              *(uint8_t *)(tmp + 9));
        tmp = tmp + PREVIEW_WIDTH;
    }

    tmp = vir_addr + PREVIEW_WIDTH * PREVIEW_HIGHT;
    for (i = 0; i < 20; i++) {
        ALOGI("read uv: %d %d %d %d %d %d %d %d %d %d", *(uint8_t *)tmp,
              *(uint8_t *)(tmp + 1), *(uint8_t *)(tmp + 2),
              *(uint8_t *)(tmp + 3), *(uint8_t *)(tmp + 4),
              *(uint8_t *)(tmp + 5), *(uint8_t *)(tmp + 6),
              *(uint8_t *)(tmp + 7), *(uint8_t *)(tmp + 8),
              *(uint8_t *)(tmp + 9));
        tmp = tmp + PREVIEW_WIDTH;
    }
#endif

    *p_out_size = preview_heap_array[target_buffer_id]->phys_size;
    memcpy((void *)*pp_image_addr, (void *)vir_addr, size);

    CMR_LOGI("X");
    return 1;
}
