#include <stdio.h>
#include <stdlib.h>
//#include <malloc.h>
#include "cmr_memory.h"
#define LOG_TAG "memoryIF"
//#define LOG_NDEBUG 0
#include <log/log.h>

static struct bufferPool_ops buf_ops = {NULL,NULL};

int set_bufferpool_ops(void *cb_malloc, void *cb_free) {
    ALOGD("set_bufferpool_ops E");
    buf_ops.bufferPool_malloc = cb_malloc;
    buf_ops.bufferPool_free = cb_free;
    return 0;
}

void* camera_heap_malloc(size_t size, char* type) {
    void* addr;

    if (size == 0) {
        return NULL;
    }

    if (NULL != buf_ops.bufferPool_malloc) {
        ALOGD("heap_malloc size is %d, type is %s", size, type);
        addr = buf_ops.bufferPool_malloc(size, type);
    } else {
        ALOGD("heap_malloc system malloc");
        addr = (void*)malloc(size);
    }

    return addr;
}

void camera_heap_free(void *pmem) {
    if(NULL != buf_ops.bufferPool_malloc) {
        ALOGD("bufferpool free %p ", pmem);
        buf_ops.bufferPool_free(pmem);
    } else {
        ALOGD("system free %p \n", pmem);
        free(pmem);
    }
}

