////#include "BufferPool.h"
#include <stdlib.h>
#include<string>
#include <sstream>
#include <fstream>
#include <iostream>
//#include "cmr_memory.h"
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
//#include <BufferPool.h>
#include "PoolManager.h"
#include "cmr_oem.c"

#define LOG_TAG "bufpoolTest"
//#define LOG_NDEBUG 0
#include <log/log.h>
#include <thread>

using namespace std;

#define NUM_THREADS 2

//typedef void (*BMalloc)(void **addr, struct camera_heap_mem *start);
//typedef void (*BFree)(void *addr);

static PoolManager& mPoolManager = PoolManager::getInstance();
//static MallocBufferPool& mMallocBufferPool = MallocBufferPool::getInstance();
//static void bufPool_malloc(void **addr, size_t size, int owner) {
//    struct Command cmd;
//    cmd.operation = MALLOC;
//    cmd.module_type = owner;
//    cmd.pmalloc = addr;
//    cmd.size = size;
//    ALOGE("sabri hwi BufferPool_malloc addr %p", addr);
//    mMallocBufferPool.postCommand(cmd, &mMallocBufferPool);
//}
//
//static void bufPool_free(void *addr){
//    struct Command cmd;
//    cmd.operation = FREE;
//    cmd.pfree = addr;
//    ALOGE("sabri hwi BufferPool_free");
//    mMallocBufferPool.postCommand(cmd, &mMallocBufferPool);
//}

static void * mallocAndFree(void *data) {
    cout << "thread 000" << endl;
//    void *p1 = NULL;
//    void *p2 = NULL;
//    struct camera_heap_mem *s1 = (camera_heap_mem*)malloc(sizeof(camera_heap_mem));
    int size = 10485760;
    char module1[] = "cnr";
    char module2[] = "ynr";
//    s1->module_type = CAMERA_ALG_TYPE_CNR;
    cout << "thread 001" << endl;
//    if (bufPool_malloc == NULL )
//        cout << "thread 999" << endl;
    void *p1 = heap_malloc(size, module1);
    ALOGE("sabri p1 %p", p1);

    cout << "thread 002" << endl;
    void *p2 = heap_malloc(size, module2);
    ALOGE("sabri p2 %p", p2);
    cout << "thread 003" << endl;
    sleep(2);

    cout << "thread p1" << p1 << endl;
    cout << "thread p2" << p2 << endl;
    heap_free(p1);
    cout << "thread 004" << endl;
    heap_free(p2);
    cout << "thread 005" << endl;
//    free(s1);
    cout << "thread 006" << endl;
//    s1 = NULL;
    return NULL;
}



int main() {
    using namespace std;

/**
1M = 10485760       6M = 62914560
2M = 20971520       7M = 73400320
3M = 31457280       8M = 83886080
4M = 41943040       9M = 94371840
5M = 52428800      10M = 104857600
*/
    
    ALOGI("sabri enter main");
    mPoolManager.initialize(104857600);
    sleep(3);
    cout << "main 001" << endl;
    pthread_t tids[NUM_THREADS];
    for(int i = 0; i < NUM_THREADS; ++i) {
        int ret = pthread_create(&tids[i], NULL, mallocAndFree, NULL);
        if (ret != 0) {
           cout << "pthread_create error: error_code=" << ret << endl;
        }
    }
    cout << "main 002" << endl;
    sleep(3);
    pthread_exit(NULL);
//    unlink();
    return 0;
}
