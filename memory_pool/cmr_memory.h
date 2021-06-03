#ifndef _CMR_MEMORY_H_
#define _CMR_MEMORY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#define MAX_CAMERA_ALLOC_ENTRYS 128

typedef void* (*BufferPool_malloc)(size_t size, char* type);
typedef void (*BufferPool_free)(void* addr);

typedef struct bufferPool_ops {
    BufferPool_malloc bufferPool_malloc;
    BufferPool_free bufferPool_free;
} bufferPool_ops_t;

enum camera_module_type{
    CAMERA_ALG_TYPE_CNR = 0,
    CAMERA_ALG_TYPE_YNR = 1,
    CAMERA_ALG_TYPE_DRE = 2,
    CAMERA_ALG_TYPE_HDR = 3,
    CAMERA_ALG_TYPE_MFNR = 4,
    CAMERA_TYPE_MAX
};

struct camera_heap_mem {
    size_t size;
    enum camera_module_type module_type;
};


int set_bufferpool_ops(void *cb_malloc, void *cb_free);
void* camera_heap_malloc(size_t size, char* type);
void camera_heap_free(void *addr);

#ifdef __cplusplus
}
#endif

#endif

