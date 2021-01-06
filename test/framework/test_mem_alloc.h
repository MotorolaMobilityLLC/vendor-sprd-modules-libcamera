#ifndef _TEST_CAMERA_MEM_ALLOC
#define _TEST_CAMERA_MEM_ALLOC

#include "sprd_ion.h"
#include "MemIon.h"
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include "SprdMultiCam3Common.h"
#include <string.h>
#include <vector>
#include <cmr_types.h>

typedef struct {
    android::MemIon *ion_heap;
    cmr_uint phys_addr;
    cmr_uint phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void *handle;
    void *virs_addr;
    bool busy_flag;
    int ion_tag;
} new_ion_mem_t;
typedef struct {
    int buf_size;
    int num_bufs;
    uint32_t is_cache;
    int tag;
} ion_info_t;

typedef struct {
    const native_handle_t *native_handle;
    sp<GraphicBuffer> graphicBuffer;
    uint32_t width;
    uint32_t height;
    void *vir_addr;
    void *phy_addr;
    int gpu_tag;
    void *private_handle;
} new_gpu_mem_t;
typedef struct {
    uint32_t w;
    uint32_t h;
    int num_buf;
    bool cache_flag;
    int tag;
} gpu_info_t;

union bufferData{
    buffer_handle_t *handle;
    new_ion_mem_t *ion_buffer;
};

class TestMemPool {
  public:
    ~TestMemPool();
    TestMemPool(vector<ion_info_t> &ion_config);
    TestMemPool(vector<gpu_info_t> &gpu_config);
    void pushBufferList (buffer_handle_t *backgpu);
    bufferData popBufferList(int tag);
    bufferData popBufferList(int width,int height);
    void pushBufferList(new_ion_mem_t *backion);
    int CheckFlag();
    int Map(buffer_handle_t *buffer_handle, void **addr_vir);
    int UnMap(buffer_handle_t *buffer);
    int Map(new_gpu_mem_t *gpu_buf);
    int UnMap(new_gpu_mem_t *gpu_buf);
    bufferData popBufferAndWait(int type);
  private:
    TestMemPool()=delete;
    vector<new_ion_mem_t> mIonBufferArr;
    vector<new_gpu_mem_t> mGpuBufferArr;
    bool mFlag;
    Mutex mBufferListLock;
    List<new_ion_mem_t *> mIonBufferList;
    List<new_gpu_mem_t *> mGpuBufferList;
    int mIonTotalNums;
    int mGpuTotalNums;
    int mIommuEnabled;
};

typedef enum InjectType {
    INJECT_DCAM_PREV=0,
    INJECT_DCAM_CAP,
    INJECT_ISP_PREV,
    INJECT_ISP_CAP,
    INJECT_OEM_PREV,
    INJECT_OEM_CAP,
    INJECT_HAL_PREV,
    INJECT_HAL_CAP,
    INJECT_GOLDEN_DCAM_PREV=16,
    INJECT_GOLDEN_DCAM_CAP,
    INJECT_GOLDEN_ISP_PREV,
    INJECT_GOLDEN_ISP_CAP,
    INJECT_GOLDEN_OEM_PREV,
    INJECT_GOLDEN_OEM_CAP,
    INJECT_GOLDEN_HAL_PREV,
    INJECT_GOLDEN_HAL_CAP,
}InjectType_t;
#endif
