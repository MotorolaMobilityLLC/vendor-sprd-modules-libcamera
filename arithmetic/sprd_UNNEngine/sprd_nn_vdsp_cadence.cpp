#ifdef ENABLE_VDSPNN

#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "vdsp_interface.h"
#include "sprd_nn_vdsp_cadence.h"
#include "unn_engine.h"
#include "log.h"


typedef struct {
    struct vdsp_handle hd;
    int libloaded;
} cadence_vdsp_handle_t;

double ts = 0.0;
inline double getTime()
{
    double cur_t;
    struct timespec t;
    long long cur_ns;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    cur_ns = (long long)t.tv_sec * 1e9 + (long long)t.tv_nsec;
    cur_t = (double)cur_ns / 1e6;
    return cur_t;
}

int cadence_vdsp_open(void **h_vdsp)
{

    cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)calloc(1, sizeof(cadence_vdsp_handle_t));
    if (NULL == handle) {
        LOGE("vdsp handle malloc failed\n");
        return 1;
    }
    ts = getTime();
    if (sprd_cavdsp_open_device((sprd_vdsp_worktype)0, &handle->hd)) {
        LOGE("vdsp open failed\n");
        free(handle);
        return 1;
    }

    *h_vdsp = handle;

    LOGD("sprd_cavdsp_open_device success(%.3f ms)\n", getTime()-ts);

    return 0;
}

int cadence_vdsp_close(void *h_vdsp, const char* nsid)
{
    ts = getTime();
    if (NULL == h_vdsp) {
        LOGE("invalid handle\n");
        return 1;
    }

    cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)h_vdsp;
    if(!nsid)
    {
        LOGE("deinit nsid is null!\n");
        return 1;
    }
    LOGD("deinit nsid is %s!\n", nsid);
    if (handle->libloaded && sprd_cavdsp_unloadlibrary((void *)&handle->hd, nsid)) {
        LOGE("unload library %s failed\n", nsid);
        return 1;
    }
    double t1 = getTime();
    LOGD("libservice sprd_cavdsp_unloadlibrary success(%.3f ms)\n", t1-ts);

    if (sprd_cavdsp_close_device(&handle->hd)) {
        LOGE("vdsp close failed\n");
        return 1;
    }

    free(h_vdsp);
    LOGD("libservice sprd_cavdsp_close_device success(%.3f ms)\n", getTime()-t1);

    return 0;
}

int cadence_vdsp_send(void *h_vdsp, const char *nsid, int priority,
    void **h_ionmem_list, uint32_t h_ionmem_num)
{
    int ret = 0;
    cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)h_vdsp;
    struct sprd_vdsp_client_inout *bufferGroup = NULL;
    struct sprd_vdsp_client_inout *in = NULL, *out = NULL, *buffer = NULL;
    ionmem_handle_t **ionGroup = (ionmem_handle_t **)h_ionmem_list;
    int bufnum = 0;

    if (NULL == h_vdsp) {
        LOGE("invalid handle\n");
        ret = 1;
        goto _ERR_EXIT;
    }

    if (0 == h_ionmem_num) {
        LOGE("ionmem_num is 0\n");
        ret = 1;
        goto _ERR_EXIT;
    }

    bufferGroup = (struct sprd_vdsp_client_inout *)calloc(h_ionmem_num, sizeof(struct sprd_vdsp_client_inout));
    if (NULL == bufferGroup) {
        LOGE("bufferGroup malloc failed\n");
        ret = 1;
        goto _ERR_EXIT;
    }

    if (h_ionmem_num > 0) {
        in = &bufferGroup[0];
        in->fd = ionGroup[0]->fd;
        in->viraddr = ionGroup[0]->v_addr;
        in->size = ionGroup[0]->size;
        in->flag = SPRD_VDSP_XRP_READ_WRITE;
        LOGD("buffer 0: fd(%d), size(%u)\n", ionGroup[0]->fd, ionGroup[0]->size);
    }

    if (h_ionmem_num > 1) {
        out = &bufferGroup[1];
        out->fd = ionGroup[1]->fd;
        out->viraddr = ionGroup[1]->v_addr;
        out->size = ionGroup[1]->size;
        out->flag = SPRD_VDSP_XRP_READ_WRITE;
        LOGD("buffer 1: fd(%d), size(%u)\n", ionGroup[1]->fd, ionGroup[1]->size);
    }

    if (h_ionmem_num > 2) {
        bufnum = h_ionmem_num - 2;
        buffer = &bufferGroup[2];
        for (uint32_t i = 2; i < h_ionmem_num; i++) {
            buffer[i-2].fd = ionGroup[i]->fd;
            buffer[i-2].viraddr = ionGroup[i]->v_addr;
            buffer[i-2].size = ionGroup[i]->size;
            buffer[i-2].flag = SPRD_VDSP_XRP_READ_WRITE;
            LOGD("buffer %d: fd(%d), size(%u)\n", i, ionGroup[i]->fd, ionGroup[i]->size);
        }
    }
    ts = getTime();
    ret = sprd_cavdsp_send_cmd(&handle->hd, nsid, in, out, buffer, bufnum, priority);

    LOGD("libservice sprd_cavdsp_send_cmd Run time: %f ms\n", getTime()-ts);

_ERR_EXIT:
    if (bufferGroup)
        free(bufferGroup);
    return ret;
}


void *sprd_nn_load_coffe_library(const char *libname)
{

    // strcat(libname, "_network.bin");
    char libcoeffname[UNN_MODEL_MAX_MODEL_PATH_NAME_LENGTH+1];
    sprintf(libcoeffname, "%s_network.bin", libname);
    FILE *fp = fopen(libcoeffname, "rb");
    if (NULL == fp) {
        LOGE("%s open failed\n", libcoeffname);
        return NULL;
    }
    LOGD("%s open success\n", libcoeffname);
    fseek(fp, 0, SEEK_END);
    uint32_t size = ftell(fp);

    ionmem_handle_t *h_libion = (ionmem_handle_t *)calloc(1, sizeof(ionmem_handle_t));
    ts = getTime();
    h_libion->pHeapIon = sprd_alloc_ionmem(size, 0 ,&(h_libion->fd ), &(h_libion->v_addr));
    h_libion->size = size;
    if (h_libion) {
        void *buf = h_libion->v_addr;
        fseek(fp, 0, SEEK_SET);
        fread(buf, 1, size, fp);
    }
    fclose(fp);
    LOGD("sprd_nn_load_coffe_library time: %f ms\n", getTime()-ts);

    return h_libion;
}

/************************************************************
alloc ionmem handle
param:
size            ---- ionmem size
iscache         ---- iscache
return value: ionmem handle
************************************************************/
void *sprd_nn_ionmem_alloc(uint32_t size, bool iscache)
{
    ionmem_handle_t *handle = (ionmem_handle_t *)malloc(sizeof(ionmem_handle_t));
    if (NULL == handle) {
        LOGE("ionmem handle malloc failed\n");
        return NULL;
    }
    handle->pHeapIon = sprd_alloc_ionmem(size, iscache, &(handle->fd), &(handle->v_addr));
    if(handle->pHeapIon == NULL){
        LOGE("sprd_alloc_ionmem fail");
        return NULL;
    }

    handle->size = size;

    LOGD("ionmem alloc fd(%d), v_addr(%p), size(%u)\n", handle->fd, handle->v_addr, handle->size);

    return handle;
}

/************************************************************
free ionmem handle
param:
h_ionmem        ---- ionmem handle
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_ionmem_free(void *h_ionmem)
{
    if (h_ionmem == NULL) {
        LOGE("invalid h_ionmem(%p)\n", h_ionmem);
        return 1;
    }
    ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
    ts = getTime();
    if(sprd_free_ionmem(handle->pHeapIon)){
        LOGE("invalid h_ionmem pHeapIon(%p)\n", handle->pHeapIon);
        return 1;
    }
    LOGD("libservice sprd_free_ionmem success(%.3f ms)\n", getTime()-ts);
    // int ret = plibapi->sprd_free_ionmem(handle->pHeapIon);
    // printf("sprd_free_ionmem ret: %d\n", ret);
    free(h_ionmem);
    return 0;
}

/************************************************************
import ionmem
param:
size            ---- ionmem size
fd              ---- ionmem fd
v_addr          ---- ionmem v_addr
ionHDL          ---- ionmem handle
return value: ionmem handle
************************************************************/
void *sprd_nn_ionmem_import(uint32_t size, int32_t fd, void *v_addr, void *ionHDL)
{
    ionmem_handle_t *handle = (ionmem_handle_t *)malloc(sizeof(ionmem_handle_t));
    if (NULL == handle) {
        LOGE("ionmem handle malloc failed\n");
        return NULL;
    }

    handle->pHeapIon = ionHDL;
    handle->fd       = fd;
    handle->v_addr   = v_addr;
    handle->size = size;

    LOGD("ionmem import fd(%d), v_addr(%p), size(%u)\n", handle->fd, handle->v_addr, handle->size);

    return handle;
}

/************************************************************
unimport ionmem handle
param:
h_ionmem        ---- ionmem handle
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_ionmem_unimport(void *h_ionmem)
{
    if (h_ionmem == NULL) {
        LOGE("invalid h_ionmem(%p)\n", h_ionmem);
        return 1;
    }
    ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
    free(h_ionmem);
    return 0;
}

/************************************************************
get ionmem vaddr
param:
h_ionmem        ---- ionmem handle
return value: vaddr
************************************************************/
void *sprd_nn_ionmem_get_vaddr(void *h_ionmem)
{
    if (h_ionmem == NULL) {
        LOGE("invalid h_ionmem(%p)\n", h_ionmem);
        return NULL;
    }
    ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
    return handle->v_addr;
}



/************************************************************
open vdsp, get vdsp handle
return value: 0 is ok, other value is failed
***********************************************************/
int sprd_nn_vdsp_open(void **h_vdsp)
{
    if (cadence_vdsp_open(h_vdsp))
        return 1;
    return 0;
}


/************************************************************
close vdsp, free vdsp handle
param:
h_vdsp          ---- handle get from sprd_caa_cadence_vdsp_load_library
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_vdsp_close(void *h_vdsp, const char* nsid)
{
    if (cadence_vdsp_close(h_vdsp, nsid))
        return 1;
    return 0;
}


/************************************************************
send command to vdsp
param:
h_vdsp          ---- handle get from sprd_caa_cadence_vdsp_load_library
nsid            ---- model name
priority        ---- cmd priority from 0~?
h_ionmem_list   ---- ionmem handle list
h_ionmem_num    ---- ionmem handle num
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_vdsp_send(void *h_vdsp, const char *nsid, int priority,
    void **h_ionmem_list, uint32_t h_ionmem_num)
{
    if (cadence_vdsp_send(h_vdsp, nsid, priority, h_ionmem_list, h_ionmem_num))
        return 1;
    return 0;
}


/************************************************************
load vdsp cadence library
param:
h_vdsp   ---- handle get from sprd_caa_vdsp_open
libname     ---- model name
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_load_vdsp_code_library(void *h_vdsp, const char *libname, const char* nsid)
{
    cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)h_vdsp;
    struct sprd_vdsp_client_inout client_buffer;
    // ionmem_handle_t *h_libion = (ionmem_handle_t *)calloc(1, sizeof(ionmem_handle_t));;
    int ret = 0;
    char libcodename[UNN_MODEL_MAX_MODEL_PATH_NAME_LENGTH+1];
    if (0 == handle->libloaded) {
        sprintf(libcodename, "%s_cadence.bin", libname);

        FILE *fp = fopen(libcodename, "rb");
        if (NULL == fp) {
            LOGE("%s open failed\n", libcodename);
            return 1;
        }
        LOGD("%s open success\n", libcodename);
        fseek(fp, 0, SEEK_END);
        client_buffer.size = ftell(fp);

        void* pHeapIon = sprd_alloc_ionmem(client_buffer.size, 0, &(client_buffer.fd), &(client_buffer.viraddr));
        if (pHeapIon) {
            void *buf = client_buffer.viraddr;
            fseek(fp, 0, SEEK_SET);
            fread(buf, 1, client_buffer.size, fp);
        }
        fclose(fp);

        if (NULL == pHeapIon) {
            ret = 1;
            LOGE("load_libarary failed\n");
            return ret;
        }

        client_buffer.flag = SPRD_VDSP_XRP_READ_WRITE;
        ts = getTime();
        if (sprd_cavdsp_loadlibrary((void *)&(handle->hd), nsid, &client_buffer)) {
            LOGE("load library %s failed\n", nsid);
            ret = 1;
            return ret;
        }

        handle->libloaded = 1;
        sprd_free_ionmem(pHeapIon);
        LOGD("sprd_cavdsp_loadlibrary success(%.3f ms)\n", getTime()-ts);
    }
    return ret;
}

#endif