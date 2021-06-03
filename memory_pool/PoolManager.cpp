#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <iostream>
#define LOG_TAG "bufPool"
//#define LOG_NDEBUG 0
#include <log/log.h>
#include "PoolManager.h"
#include "cmr_memory.h"
using android::sp;

#define MINSIZE 1024000

//static sp<MallocBufferPool> mMallocBufferPool = new MallocBufferPool();
static sp<MallocBufferPool> mMallocBufferPool;
static sem_t sem;

PoolManager::PoolManager() {
    is_inited = false;
    is_deinited = true;
    is_pool_inited = false;
    is_pool_released = true;
    count = 0;
}

PoolManager::~PoolManager() {
}

void PoolManager::initialize() {
    if (count > 0) {
        count++;
        goto exit;
    }
    ALOGI("bufferpool manager initialize E");
    if (is_inited) {
        ALOGV("do not need init");
        return;
    }
    is_inited = true;
    is_deinited = false;
    sem_init(&sem, 0, 1);
    if (!mMallocBufferPool) {
        mMallocBufferPool = new MallocBufferPool();
    }
    mMallocBufferPool->initialize();
    count++;
exit:
    return;
}

void PoolManager::initializePool(size_t size) {
    ALOGI("bufferpool manager initialize Pool E, size is %d", size);
    if (is_pool_inited) {
        ALOGV("do not need initPool");
        return;
    }
    is_pool_inited = true;
    is_pool_released = false;

    setPoolOpreation((void*)bufPool_malloc, (void*)bufPool_free);

    struct Command cmd;
    cmd.operation = INIT;
    cmd.size = size;
    cmd.sync = false;
    mMallocBufferPool->postCommand(cmd, mMallocBufferPool);
}

void PoolManager::releasePool() {
    if (is_pool_released == true) {
        ALOGV("do not need releasePool");
        return;
    }
    is_pool_released = true;
    is_pool_inited = false;
    mMallocBufferPool->releasePool();
    setPoolOpreation(NULL, NULL);
}

void PoolManager::deinit() {
    if (count != 1) {
        count--;
        goto exit;
    }
    if (is_deinited == true) {
        ALOGV("do not need deinit");
        return;
    }
    is_deinited = true;
    is_inited = false;

    struct Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.operation = DEINIT;
    mMallocBufferPool->postCommand(cmd, mMallocBufferPool);
    sem_destroy(&sem);
    if (mMallocBufferPool) {
        mMallocBufferPool->deinit();
        mMallocBufferPool.clear();
    }
    count--;
exit:
    return;
}

void PoolManager::setPoolOpreation(void *pMalloc,void *pFree) {
    set_bufferpool_ops(pMalloc, pFree);
}

void PoolManager::resizePool(size_t size) {
    ALOGI("bufferpool manager resizePool E");
    if (mMallocBufferPool->mPoolSize == size) {
        ALOGV("do not need resizePool");
        return;
    }
    struct Command cmd;
    cmd.operation = REALLOC;
    cmd.size = size;
    cmd.sync = false;
    mMallocBufferPool->postCommand(cmd, mMallocBufferPool);
}

void* PoolManager::bufPool_malloc(size_t size, char* type) {
//    void* addr;
    sem_wait(&sem);
    long long addr;
    if(size < MINSIZE) {
        ALOGD("malloc size is small, system malloc");
        sem_post(&sem);
        return (void*)malloc(size);
    }

    struct Command cmd;
    cmd.operation = MALLOC;
    cmd.module_type = type;
    cmd.pmalloc = &addr;
    cmd.size = size;
    cmd.sync = true;
    mMallocBufferPool->postCommand(cmd, mMallocBufferPool);
    ALOGD("addr is %p", (void *)addr);
    sem_post(&sem);
    return (void*)(addr & 0x0ffffffff);
}

void PoolManager::bufPool_free(void *addr) {
    struct Command cmd;
    cmd.operation = FREE;
    cmd.pfree = addr;
    cmd.sync = false;
    ALOGV("BufferPool_free addr %p", addr);
    mMallocBufferPool->postCommand(cmd, mMallocBufferPool);
}

