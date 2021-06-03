#ifndef _BUFFERPOOL_H  
#define _BUFFERPOOL_H  

#include <stdlib.h>
#include <queue>  //std::queue
#include "utils/Mutex.h"
#include "utils/Thread.h"
#include <thread>
#include <semaphore.h>
#include <iostream>
#include <vector>
#include <sys/time.h>

using ::android::wp;
using ::android::sp;

typedef struct memory_block {
    void *memory;
    size_t size;
    bool is_allocated;
    char* owner;
    memory_block *next;
} memory_block;

typedef enum {
    INIT,
    MALLOC,
    FREE,
    REALLOC,
    DEINIT,
    UNKNOWN,
} Op;

typedef struct Command {
    Op operation;
    bool sync;
    char* module_type;
    void *pmalloc;
    void *pfree;
    size_t size;
} Command;

class BufferPool {
public:
    virtual void initialize() = 0;
    virtual void initializePool(size_t poolSize) = 0;
    virtual void releasePool() = 0;
    virtual void deinit() = 0;
    virtual void allocateMemory(void *pmem, size_t size, char* owner) = 0;
    virtual void freeMemory(void* pmem) = 0;
    virtual void resizePool(size_t poolSize) = 0;
    virtual void dumpMemorySet() = 0;
    virtual void dumpHistoryUsed() = 0;
    virtual ~BufferPool();
    BufferPool();
};

class MallocBufferPool :  public virtual android::RefBase, public BufferPool{
public:
    virtual void initialize();
    virtual void initializePool(size_t poolSize);
    virtual void releasePool();
    virtual void deinit();
    virtual void allocateMemory(void *pmem, size_t size, char* owner);
    virtual void freeMemory(void* pmem);
    virtual void resizePool(size_t poolSize);
    virtual void dumpMemorySet();
    virtual void dumpHistoryUsed();
    void postCommand(Command cmd, sp<MallocBufferPool> data);
    MallocBufferPool();
    virtual ~MallocBufferPool();
    sem_t sem;
    sem_t sem_pool;
    sem_t thread_sem;

    std::mutex mThreadLock;
    int dumpCount;
    size_t usedSize;
    size_t peakSize;
    size_t mPoolSize;
    std::vector<std::pair<char*, int>> historyUsed;

    class ProcessThread :  public android::Thread {
    public:
        ProcessThread(wp<MallocBufferPool> parent);
        virtual ~ProcessThread();
        virtual bool threadLoop() override;
        bool waitForStatus();
        wp<MallocBufferPool> mParent;
        struct timespec t;
    };

private:
    memory_block *memory_set;
    std::queue<Command> mCommandQueue;
    sp<ProcessThread> mProcessThread;
    bool is_inited;
};

#endif

