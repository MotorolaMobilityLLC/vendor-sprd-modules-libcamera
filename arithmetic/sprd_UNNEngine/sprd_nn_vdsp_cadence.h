#ifndef __SPRD_VDSP_CADENCE_H__
#define __SPRD_VDSP_CADENCE_H__

typedef struct {
    int fd;
    void *v_addr;
    uint32_t size;
    void *pHeapIon;
} ionmem_handle_t;


/************************************************************
open vdsp, get vdsp handle
return value: 0 is ok, other value is failed
***********************************************************/
int sprd_nn_vdsp_open(void **h_vdsp);

/************************************************************
close vdsp, free vdsp handle
param:
h_vdsp          ---- handle get from sprd_caa_cadence_vdsp_load_library
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_vdsp_close(void *h_vdsp, const char* nsid);

/************************************************************
load vdsp network code library
param:
h_vdsp    ---- vdsp handle
libname      ---- model name
return ionmem handle
************************************************************/
int sprd_nn_load_vdsp_code_library(void *h_vdsp, const char *libname, const char* nsid);

/************************************************************
load vdsp network coffe library
param:
libname   ---- model name
return ionmem handle
************************************************************/
void *sprd_nn_load_coffe_library(const char *libname);


/************************************************************
get ionmem's virtual address
param:
h_ionmem   ---- ionmem handle
return h_ionmem->v_addr
************************************************************/
void *sprd_nn_ionmem_get_vaddr(void *h_ionmem);


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
                       void **h_ionmem_list, uint32_t h_ionmem_num);


/************************************************************
alloc ionmem
param:
size          ---- ionmem size
iscache       ---- iscache
return ionmem handle
************************************************************/
void *sprd_nn_ionmem_alloc(uint32_t size, bool iscache);

/************************************************************
free ionmem handle
param:
h_ionmem        ---- ionmem handle
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_ionmem_free(void *h_ionmem);

/************************************************************
import ionmem
param:
size            ---- ionmem size
fd              ---- ionmem fd
v_addr          ---- ionmem v_addr
ionHDL          ---- ionmem handle
return value: ionmem handle
************************************************************/
void *sprd_nn_ionmem_import(uint32_t size, int32_t fd, void *v_addr, void *ionHDL);

/************************************************************
unimport ionmem handle
param:
h_ionmem        ---- ionmem handle
return value: 0 is ok, other value is failed
************************************************************/
int sprd_nn_ionmem_unimport(void *h_ionmem);

#endif
