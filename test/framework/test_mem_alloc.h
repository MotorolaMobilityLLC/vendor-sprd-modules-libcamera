#ifndef _TEST_CAMERA_MEM_ALLOC
#define _TEST_CAMERA_MEM_ALLOC

#include "sprd_ion.h"
#include "MemIon.h"
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#define MAX_GRAPHIC_BUF_NUM 20
#define MAX_SUB_RAWHEAP_NUM 10

struct test_camera_mem_type_stat {
    unsigned long mem_type;
    unsigned long is_cache;
};

typedef struct test_camera_memory {
    android::MemIon *ion_heap;
    unsigned long phys_addr;
    unsigned long phys_size;
    int fd;
    int dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} test_camera_memory_t;

/*use for map &unmap*/
typedef struct test_hal_mem_info {
    int fd;
    size_t size;
    // offset from fd, always set to 0
    void *addr_phy;
    void *addr_vir;
    int width;
    int height;
    int format;
    android::sp<android::GraphicBuffer> pbuffer;
    void *bufferPtr;
} test_hal_mem_info_t;

struct camera_mem_private_data {
    int mZslNum;
    unsigned int mZslHeapNum;
    test_camera_memory_t *mZslHeapArray[49];
};

enum test_camera_mem_type {
    TEST_CAMERA_PREVIEW = 0,
    TEST_CAMERA_CAPTURE,
    TEST_CAMERA_MEM_TYPE_MAX
};

typedef struct test_wide_memory {
    android::sp<android::GraphicBuffer> bufferhandle;
    void *private_handle;
    unsigned long buf_size;
} test_wide_memory_t;

typedef test_wide_memory_t test_graphic_memory_t;

typedef struct Test_MemGpuQueue {
    test_graphic_memory_t mGpuHeap;
    test_camera_mem_type_stat mem_type;
} Test_MemGpuQueue_t;

typedef struct Test_MemIonQueue {
    test_camera_memory_t *mIonHeap;
    test_camera_mem_type_stat mem_type;
} Test_MemIonQueue_t;

class Test_Mem_alloc {
  public:
    Test_Mem_alloc();
    ~Test_Mem_alloc();
    int Test_Callback_GpuMalloc(struct test_camera_mem_type_stat type,
                                unsigned int *size_ptr, unsigned int *sum_ptr,
                                unsigned long *phy_addr,
                                unsigned long *vir_addr, int *fd, void **handle,
                                unsigned long *width, unsigned long *height,
                                void *private_data);

    int Test_Callback_IonMalloc(struct test_camera_mem_type_stat type,
                                unsigned int *size_ptr, unsigned int *sum_ptr,
                                unsigned long *phy_addr,
                                unsigned long *vir_addr, int *fd,
                                void *private_data);

    int Test_Callback_Free(struct test_camera_mem_type_stat type,
                           unsigned long *phy_addr, unsigned long *vir_addr,
                           int *fd, unsigned int sum, void *private_data);
};
#endif
