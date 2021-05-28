#ifndef _POOLMANAGER_
#define _POOLMANAGER_

#include <stdlib.h>
#include "BufferPool.h"
#include <atomic>

class PoolManager {
public:
    ~PoolManager();
    PoolManager(const PoolManager&)=delete;
    PoolManager& operator=(const PoolManager&)=delete;
    static PoolManager& getInstance() {
        static PoolManager instance;
        return instance;
    }
    void initialize();
    void initializePool(size_t size);
    void releasePool();
    void deinit();
    void setPoolOpreation(void *pMalloc,void *pFree);
    void resizePool(size_t size);
    static void* bufPool_malloc(size_t size, char* type);
    static void bufPool_free(void *addr);

private:
    PoolManager();
    bool is_inited;
    bool is_deinited;
    bool is_pool_inited;
    bool is_pool_released;
    std::atomic_int count;
};

#endif
