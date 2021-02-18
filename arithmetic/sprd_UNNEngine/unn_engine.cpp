#include <memory>
#include <string.h>
#include <vector>
#include "unn_engine.h"
#include "nna_wrapper.h"
#include "tflite_wrapper.h"
#include "mnn_wrapper.h"
#include "vdspnn_wrapper.h"
#include "log.h"
#include "unn_ion.h"

// alignment >1 and  alignment should be pow(2)
static void* aligned_malloc(size_t required_bytes, size_t alignment)
{
    int offset = alignment - 1 + sizeof(void*);
    void* p1 = (void*)malloc(required_bytes + offset);
    if (p1 == NULL)
    {
        LOGE("aligned malloc failed!");
        return NULL;
    }

    void** p2 = (void**)( ( (size_t)p1 + offset ) & ~(alignment - 1) );
    p2[-1] = p1;
    return p2;
}

static void aligned_free(void *p2)
{
    void* p1 = ((void**)p2)[-1];
    free(p1);
}

typedef struct UNNEngineHandle_t {
    void*         handle;
    UNNRunType    rt;
} UNNHDL;

int UNNEngineInit(void **UNN_hd, UNNRunType rt, void* ptr)
{
    int ire = -1;
    UNNHDL *handle = (UNNHDL *)malloc(sizeof(UNNHDL));
    if (NULL == handle) {
        LOGE("unnengine handle malloc failed\n");
        return ire;
    }
    handle->rt = rt;
    switch (handle->rt){
        case kUNN_VDSP:
        {
#ifdef ENABLE_VDSPNN
            ire = VDSPNNEngineInit(&handle->handle);
#else
            LOGE("unsupport VDSP!\n");
            goto _ERR_EXIT;
#endif
            break;
        }
        case kUNN_IMGNNA:
        {
#ifdef ENABLE_IMGNPU
            ire = NNAEngineInit(&handle->handle);
#else
            LOGE("unsupport IMGNPU\n");
            goto _ERR_EXIT;
#endif
            break;
        }
        case kUNN_TFLITE:
        {
#ifdef ENABLE_TFLITE
            ire = TFLITEEngineInit(&handle->handle);
#else
            LOGE("unsupport TFLITE\n");
            goto _ERR_EXIT;
#endif
            break;
        }
        case kUNN_MNN:
        {
#ifdef ENABLE_MNN
            ire = MNNEngineInit(&handle->handle);
#else
            LOGE("unsupport MNN\n");
            goto _ERR_EXIT;
#endif
            break;
        }
        default:
        {
            LOGD("cannot handle UNNRunType %d yet!\n", handle->rt);
            goto _ERR_EXIT;
            break;
        }
    }

    *UNN_hd = handle;
    return ire;

_ERR_EXIT:
    free(handle);
    return ire;
}

int UNNEngineDeInit(void *UNN_hd)
{
    int ire = -1;
    if (NULL == UNN_hd) {
        LOGE("invalid unnengine handle\n");
        return ire;
    }

    UNNHDL *handle = (UNNHDL *)UNN_hd;
    switch (handle->rt)
    {
        case kUNN_VDSP:
        {
#ifdef ENABLE_VDSPNN
            ire = VDSPNNEngineDeInit(handle->handle);
#endif
            break;
        }
        case kUNN_IMGNNA:
        {
#ifdef ENABLE_IMGNPU
            ire = NNAEngineDeInit(handle->handle);
#endif
            break;
        }
        case kUNN_TFLITE:
        {
#ifdef ENABLE_TFLITE
            ire = TFLITEEngineDeInit(handle->handle);
#endif
            break;
        }
        case kUNN_MNN:
        {
#ifdef ENABLE_MNN
            ire = MNNEngineDeInit(handle->handle);
#endif
            break;
        }
        default:
        {
            LOGE("cannot handle UNNRunType %d yet!\n", handle->rt);
            break;
        }
    }
_ERR_EXIT:
    free(UNN_hd);
    return ire;
}


int UNNEngineCreateNetwork(void *UNN_hd, void *info)
{
    int ire = -1;
    if (NULL == UNN_hd) {
        LOGE("invalid unnengine handle\n");
        return ire;
    }
    if (NULL == info) {
        LOGE("invalid unnmodel Info\n");
        return ire;
    }

    UNNHDL *handle = (UNNHDL *)UNN_hd;

    switch (handle->rt)
    {
        case kUNN_VDSP:
        {
#ifdef ENABLE_VDSPNN
            ire = VDSPNNEngineCreateNetwork(handle->handle, (UNNModel *)info);
#endif
            break;
        }
        case kUNN_IMGNNA:
        {
#ifdef ENABLE_IMGNPU
            ire = NNAEngineCreateNetwork(handle->handle, (UNNModel *)info);
#endif
            break;
        }
        case kUNN_TFLITE:
        {
#ifdef ENABLE_TFLITE
            ire = TFLITEEngineCreateNetwork(handle->handle, (UNNModel *)info);
#endif
            break;
        }
        case kUNN_MNN:
        {
#ifdef ENABLE_MNN
            ire = MNNEngineCreateNetwork(handle->handle, (UNNModel *)info);
#endif
            break;
        }
        default:
        {
            LOGE("cannot handle UNNRunType %d yet!\n", handle->rt);
            break;
        }
    }
    return ire;
}

int UNNEngineDestroyNetwork(void *UNN_hd)
{
    int ire = -1;
    if (NULL == UNN_hd) {
        LOGE("invalid unnengine handle\n");
        return ire;
    }

    UNNHDL *handle = (UNNHDL *)UNN_hd;

    switch (handle->rt)
    {
        case kUNN_VDSP:
        {
#ifdef ENABLE_VDSPNN
            ire = VDSPNNEngineDestroyNetwork(handle->handle);
#endif
            break;
        }
        case kUNN_IMGNNA:
        {
#ifdef ENABLE_IMGNPU
            ire = NNAEngineDestroyNetwork(handle->handle);
#endif
            break;
        }
        case kUNN_TFLITE:
        {
#ifdef ENABLE_TFLITE
            ire = TFLITEEngineDestroyNetwork(handle->handle);
#endif
            break;
        }
        case kUNN_MNN:
        {
#ifdef ENABLE_MNN
            ire = MNNEngineDestroyNetwork(handle->handle);
#endif
            break;
        }
        default:
        {
            LOGE("cannot handle UNNRunType %d yet!\n", handle->rt);
            break;
        }
    }
    return ire;
}


int UNNEngineRunSession(void *UNN_hd, void *param)
{
    int ire = -1;
    if (NULL == UNN_hd) {
        LOGE("invalid unnengine handle\n");
        return ire;
    }
    UNNHDL *handle = (UNNHDL *)UNN_hd;
    switch (handle->rt)
    {
        case kUNN_VDSP:
        {
#ifdef ENABLE_VDSPNN
            ire = VDSPNNEngineRunSession(handle->handle, (UNNModelInOutBuf *)param);
#endif
            break;
        }
        case kUNN_IMGNNA:
        {
#ifdef ENABLE_IMGNPU
            ire = NNAEngineRunSession(handle->handle, (UNNModelInOutBuf *)param);
#endif
            break;
        }
        case kUNN_TFLITE:
        {
#ifdef ENABLE_TFLITE
            ire = TFLITEEngineRunSession(handle->handle, (UNNModelInOutBuf *)param);
#endif
            break;
        }
        case kUNN_MNN:
        {
#ifdef ENABLE_MNN
            ire = MNNEngineRunSession(handle->handle, (UNNModelInOutBuf *)param);
#endif
            break;
        }
        default:
        {
            LOGE("cannot handle UNNRunType %d yet!\n", handle->rt);
            break;
        }
    }
    return ire;
}

int UNNEngineAllocTensorBuffer(void *UNN_hd, void *tensor)
{
    int ire = -1;

    if (NULL == tensor) {
        LOGE("invalid tensor\n");
        return ire;
    }

    UNNTensorBuffer *t = (UNNTensorBuffer *)tensor;

    switch (t->memType)
    {
        case kUNN_MEM_TYPE_ION:
        {
            LOGD(" t size %d , memAlign: 0x%x \n", t->size, t->memAlign);
            t->prvHdl =  unn_alloc_ionmem(t->size, t->isCache, &t->fd, &t->data);
            if(!t->prvHdl)
            {
                LOGE(" unn_alloc_ionmem failed \n");
                return ire;
            }
            LOGD(" t->data: 0x%x \n", t->data);
            break;
        }
        case kUNN_MEM_TYPE_CPUPTR:
        {
            LOGD(" t size %d , memAlign: 0x%x \n", t->size, t->memAlign);
            if( t->memAlign > 1){
                t->data = aligned_malloc(t->size, t->memAlign);
            }
            else {
                t->data = (void *) malloc(t->size);
            }
            LOGD(" t->data: 0x%x \n", t->data);
            break;
        }
        default:
            break;
    }
    return 0;
}

int UNNEngineFreeTensorBuffer(void *UNN_hd, void *tensor)
{
    int ire = -1;

    if (NULL == tensor) {
        LOGE("invalid tensor\n");
        return ire;
    }

    UNNTensorBuffer *t = (UNNTensorBuffer *)tensor;

    switch (t->memType)
    {
        case kUNN_MEM_TYPE_ION:
        {
            if(t->prvHdl) unn_free_ionmem(t->prvHdl);
            break;
        }
        case kUNN_MEM_TYPE_CPUPTR:
        {
            if( t->memAlign > 1) {
                if(t->data)
                    aligned_free(t->data);
            }
            else {
                if(t->data)
                    free(t->data);
            }
            break;
        }
        default:
            break;
    }
    return 0;
}


