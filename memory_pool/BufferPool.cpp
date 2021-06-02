#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cutils/properties.h>
#define LOG_TAG "bufPool"
//#define LOG_NDEBUG 0
#include <log/log.h>
//#include <string.h>
#include "BufferPool.h"

#define TIMEOUT_MS 500

using namespace std;
using ::android::wp;
using ::android::sp;

BufferPool::BufferPool(){
}

BufferPool::~BufferPool(){
}

MallocBufferPool::MallocBufferPool(){
    is_inited = false;
}

MallocBufferPool::~MallocBufferPool() {
}

void MallocBufferPool::initialize() {
    sem_init(&sem, 0, 0);
    sem_init(&sem_pool, 0, 1);
    sem_init(&thread_sem, 0, 0);
    dumpCount = 0;

    //init first block
    ALOGI("initialize: E");
    memory_set = (memory_block*)malloc(sizeof(memory_block));
    memset(memory_set, 0, sizeof(memory_block));

    if (!mProcessThread) {
        mProcessThread = new ProcessThread(this);
    }
    mProcessThread->run("BufPoolThread", android::PRIORITY_DISPLAY);
    ALOGI("initialize done X");
}

void MallocBufferPool::initializePool(size_t poolSize) {
    ALOGI("initializePool E");
    if (is_inited == true) {
        ALOGI("bufferpool do not need init");
        return;
    }
    is_inited = true;
    usedSize = 0;
    peakSize = 0;

    if (NULL != memory_set->memory) {
        ALOGW("bufferpool already init ,memory %p", memory_set->memory);
        releasePool();
        return;
    }

    char value[128];
    property_get("persist.vendor.cam.mempool.size", value, "0");
    size_t size = atoi(value);
    if (strcmp(value, "0")) {
        poolSize = size;
    }
    //init pool size
    memory_set->memory = (void*)malloc(poolSize);
    memory_set->size = poolSize;
    memory_set->is_allocated = 0;
    memory_set->next = NULL;
    memory_set->owner = {"null"};
    mPoolSize = poolSize;
    ALOGD("initializePool X, poolSize %d", poolSize);
}

void MallocBufferPool::releasePool() {
    ALOGI("releasePool E");
    memory_block *block = memory_set;
    if (memory_set->memory == NULL) {
        return;
    }
    int time = 0;
    while (NULL != memory_set->next){
        //add time limit?
        time++;
        usleep(1000);
        if (time >= 30) {
            ALOGW("exist used memory! check it's deinit");
            return;
        }
    }
    while (NULL != block->next) {
        memory_block *needFree = block->next;
        block->next = needFree->next;
        needFree->next = NULL;
        free(needFree);
        needFree = NULL;
    }

    sem_wait(&sem_pool);
    free(memory_set->memory);
    memory_set->memory = NULL;
    sem_post(&sem_pool);
    mPoolSize = 0;
    is_inited = false;
    //for debug
    char value[128];
    property_get("persist.vendor.cam.mempool.debug1", value, "0");
    if (strcmp(value, "0")) {
        dumpHistoryUsed();
    }

    if (usedSize != 0) {
        ALOGE("bufferpool exists mem leak! leak size %d", usedSize);
    }
    ALOGI(" releasePool X, peakSize is %d", peakSize);
}

void MallocBufferPool::deinit() {
    ALOGI("deinit E");
    if (mProcessThread != NULL) {
        mProcessThread->requestExit();
        // wait threads quit to relese object
        mProcessThread->join();
    }
    sem_destroy(&sem);
    sem_destroy(&sem_pool);
    sem_destroy(&thread_sem);
    releasePool();
    free(memory_set);
    memory_set = NULL;
    is_inited = false;

    for (auto it = historyUsed.begin(); it != historyUsed.end(); ) {
        it = historyUsed.erase(it);
    }
    ALOGI("deinit X");
}

void MallocBufferPool::allocateMemory(void *pmem, size_t size, char* owner) {
    ALOGD("allocateMemory E size is %d", size);
    memory_block *block = memory_set;

    //for address alignment
    size_t allocSize = size % 4 ? (size/4 + 1) * 4 : size;

    while (NULL != block) {
        if (block->is_allocated) {
            block = block->next;
            continue;
        }
        if(block->size >= allocSize)
            break;
        block = block->next;
    }

    if (NULL == block) {
        ALOGD("pool size is %d not enough, using system malloc %d", mPoolSize, allocSize);
        *((long long *)pmem) = (long long)malloc(allocSize);
        sem_post(&sem);
//        dumpMemorySet();
        return;
    }

    if (block->size == allocSize) {
        block->is_allocated = 1;
    } else {
        memory_block *remain_block;
        remain_block = (memory_block*)malloc(sizeof(memory_block));
        //init remain block init
        remain_block->memory = (void*)((char*)block->memory + allocSize);
        remain_block->size = block->size - allocSize;
        remain_block->is_allocated = 0;
        remain_block->next = block->next;
        remain_block->owner = {"null"};

        //insert remain block between block and block->next
        block->size = allocSize;
        block->is_allocated = 1;
        block->next = remain_block;
    }
    block->owner = owner;
    *((long long *)pmem) = (long long)block->memory;
    sem_post(&sem);

    usedSize += block->size;
    if (peakSize < usedSize) {
        peakSize = usedSize;
    }

    //for debug: dump buffer pool state
    char value0[128];
    property_get("persist.vendor.cam.mempool.debug0", value0, "0");
    if (strcmp(value0, "0")) {
        dumpMemorySet();
    }
    char value1[128];
    property_get("persist.vendor.cam.mempool.debug1", value1, "0");
    if (strcmp(value1, "0")) {
        historyUsed.push_back(std::make_pair(block->owner, usedSize));
    }
    ALOGD("allocateMemory X pmem is %p", pmem);
}

void MallocBufferPool::freeMemory(void* pmem) {
    ALOGD("freeMemory E, pmem is %p", pmem);
    //for debug: historyused push_back
    char value1[128];
    property_get("persist.vendor.cam.mempool.debug1", value1, "0");

    if (NULL == memory_set) {
        return;
    }
    void *p = pmem;
    //free first block
    memory_block *block = memory_set;
    if (block->memory == p) {
        usedSize -= block->size;
        if (block->is_allocated == 0)
            return;
        if (NULL != block->next) {
            memory_block *tmp = block->next;
            if (tmp->is_allocated == 0){
                block->size += tmp->size;
                block->next = tmp->next;
                tmp->next = NULL;
                free(tmp);
                tmp = NULL;
            }
        }
        block->is_allocated = 0;
        if (strcmp(value1, "0")) {
            historyUsed.push_back(std::make_pair(block->owner, usedSize));
        }
        block->owner = {"null"};
        return;
    }

    //free nonfirst block
    memory_block *cur = block->next;
    while (NULL != cur) {
        if (cur->memory == p)
            break;
        cur = cur->next;
        block = block->next;
    }
    if (NULL == cur) {
//        dumpMemorySet();
        ALOGD("free system block pmem %p", pmem);
        free(pmem);
        return;
    }
    usedSize -= cur->size;
    if (strcmp(value1, "0")) {
        historyUsed.push_back(std::make_pair(cur->owner, usedSize));
    }

    //merge next block if unallocated
    if (NULL != cur->next) {
        memory_block *next_block = cur->next;
        if (next_block->is_allocated == 0) {
            cur->size += next_block->size;
            cur->next = next_block->next;
            next_block->next = NULL;
            free(next_block);
            next_block = NULL;
        }
    }
    cur->owner = {"null"};
    cur->is_allocated = 0;
    //merge pre block if unallocated
    if (block->is_allocated == 0) {
        block->size += cur->size;
        block->next = cur->next;
        cur->next = NULL;
        free(cur);
        cur = NULL;
    }

    //for debug: dump buffer pool state
    char value0[128];
    property_get("persist.vendor.cam.mempool.debug0", value0, "0");
    if (strcmp(value0, "0")) {
        dumpMemorySet();
    }
    ALOGD("freeMemory X");
    return;
}

void MallocBufferPool::resizePool(size_t poolSize) {
    if (poolSize == mPoolSize) {
        return;
    }
    while (NULL != memory_set->next){
        ALOGE("cannot resize");
        return;
    }
    sem_wait(&sem_pool);
    void* ptr = (void*)realloc(memory_set->memory, poolSize);
    if (NULL == ptr) {
        ALOGE("resize error, buffer pool is null");
        free(memory_set->memory);
        memory_set->memory = (void*)malloc(poolSize);
    } else {
        memory_set->memory = ptr;
    }
    sem_post(&sem_pool);
    memory_set->size = poolSize;
    mPoolSize = poolSize;
    ALOGI("bufferpool resize, new size is %d", memory_set->size);
    usedSize = 0;
    ALOGV("bufferpool peakSize before resize is %d", peakSize);
    peakSize = 0;
}

void MallocBufferPool::dumpMemorySet() {
    ALOGD("dumpMemorySet E");
    memory_block *block = memory_set;
    int i = 1;
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);

    std::ostringstream oss;
    oss << " Block-number    Addresses        Size        Is_allocated    Owner \n";
//    std::string owner;
    while (NULL != block) {
//        if (NULL != block->owner)
//            owner = block->owner;
        oss << "         " << i++ << "        " 
            << "      " << block->memory << "   "
            << "  " << block->size << ""
            << "        " << block->is_allocated << "        "
            << "    " << block->owner << "\n";
        block = block->next;
    }
    char name_buf[128];
//    sprintf(name_buf, "/data/vendor/cameraserver/CameraMemorySet_%02d%02d%02d%02d.txt", p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    sprintf(name_buf, "/data/vendor/cameraserver/CameraMemorySet_%d.txt", dumpCount++);
    std::ofstream file_out(name_buf);
    if (! file_out.is_open()) {
        ALOGE("- %s: file_out not open ", __FUNCTION__);
        return ;
    }
    file_out << "========================= + CameraMemorySet ============================================\n" ;
    std::string str = oss.str();
    file_out << str;
    file_out << "========================= - CameraMemorySet ============================================\n" ;
    file_out.close();
    ALOGD("dumpMemorySet X");
}

void MallocBufferPool::dumpHistoryUsed() {
    ALOGD("dumpHistoryUsed E");

    int i = 1;
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);

    std::ostringstream oss;
    oss << " Operation-record   Operator        Real-time size \n";
    std::vector<std::pair<char*, int>>::iterator it;
     for(it = historyUsed.begin(); it != historyUsed.end(); it++) {
        std::string owner;
        if (NULL != it->first) {
            owner = it->first;
        } else {
            owner = "unknown";
        }
        oss << "             " << i++ << "           " << /*it->first*/ owner << "            " << it->second << "\n";
     }
    char name_buf[128];
    sprintf(name_buf, "/data/vendor/cameraserver/PoolHistoryUsed_%02d%02d%02d%02d.txt", p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    std::ofstream file_out(name_buf);
    if (! file_out.is_open()) {
        ALOGE("- %s: file_out not open ", __FUNCTION__);
        return ;
    }
    file_out << "========================= + PoolHistoryUsed ============================================\n" ;
    std::string str = oss.str();
    file_out << str;
    file_out << "========================= - PoolHistoryUsed ============================================\n" ;
    file_out.close();
    ALOGD("dumpHistoryUsed X");
}

void MallocBufferPool::postCommand(Command cmd, sp<MallocBufferPool> data) {
    ALOGD("postCommand E");
    sp<MallocBufferPool> ptr = data;
    {
        std::unique_lock<std::mutex> lck(ptr->mThreadLock);
        ptr->mCommandQueue.push(cmd);
        ALOGD("mCommandQueue size : %d", mCommandQueue.size());
    }
    sem_post(&thread_sem);
    if (cmd.sync == true) {
        sem_wait(&sem);
    }
}

MallocBufferPool::ProcessThread::ProcessThread(wp<MallocBufferPool> parent) : mParent(parent){
}

MallocBufferPool::ProcessThread::~ProcessThread() {
}

bool MallocBufferPool::ProcessThread::threadLoop() {
    waitForStatus();
    auto parent = mParent.promote();
        std::unique_lock<std::mutex> lk(parent->mThreadLock);
    if (!parent->mCommandQueue.empty()) {
        Command& cmd = parent->mCommandQueue.front();
        switch (cmd.operation) {
        case INIT:
            ALOGV("thread cmd is init, size %d", cmd.size);
            parent->initializePool(cmd.size);
            break;
        case MALLOC:
            ALOGV("thread cmd is malloc, addr %p", (void*)cmd.pmalloc);
            parent->allocateMemory(cmd.pmalloc, cmd.size, cmd.module_type);
            ALOGD("thread cmd after malloc, addr %p", (void *)cmd.pmalloc);
            break;
        case FREE:
            ALOGV("thread cmd is free, addr %p", cmd.pfree);
            parent->freeMemory(cmd.pfree);
            break;
        case REALLOC:
            ALOGV("thread cmd is resize, new size %d", cmd.size);
            parent->resizePool(cmd.size);
            break;
        case DEINIT:
            return false;
        default:
            break;
        }
        parent->mCommandQueue.pop();
        ALOGD("threadloop after pop, now size is %d", parent->mCommandQueue.size());
    }
    return true;
}

bool MallocBufferPool::ProcessThread::waitForStatus(){
    long msecs = TIMEOUT_MS;
    clock_gettime(CLOCK_REALTIME, &t);
    long secs = msecs/1000;
    msecs = msecs%1000;
    long add = 0;
    msecs = msecs*1000*1000 + t.tv_nsec;
    add = msecs / (1000*1000*1000);
    t.tv_sec += (add + secs);
    t.tv_nsec = msecs%(1000*1000*1000);

    auto parent = mParent.promote();
    sem_timedwait(&(parent->thread_sem), &t);
    return true;
}


