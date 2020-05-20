#ifndef __SPRD_CAMALG_ASSIST_H__
#define __SPRD_CAMALG_ASSIST_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int fd;
	void *v_addr;
	uint32_t size;
	void *pHeapIon;
} ionmem_handle_t;

/************************
 * vdsp
 ***********************/
//初始化vdsp，获取vdsp handle
int sprd_caa_vdsp_open(void **h_vdsp);

//关闭vdsp，释放vdsp handle
int sprd_caa_vdsp_close(void *h_vdsp);

//通过vdsp handle，向vdsp发送命令
int sprd_caa_vdsp_send(void *h_vdsp, const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num);

//一次性向vdsp发送命令
int sprd_caa_vdsp_Send(const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num);

//加载算法firmware(cadence vdsp)
int sprd_caa_cadence_vdsp_load_library(void *h_vdsp, const char *nsid);

int sprd_caa_vdsp_maxfreq_lock(void *h_vdsp);

int sprd_caa_vdsp_maxfreq_unlock(void *h_vdsp);

/************************
 * ionmem
 ***********************/
//分配ionmem
void *sprd_caa_ionmem_alloc(uint32_t size, bool iscache);

//释放ionmem
int sprd_caa_ionmem_free(void *h_ionmem);

//flush ionmem
int sprd_caa_ionmem_flush(void *h_ionmem, uint32_t size);

//invalid ionmem
int sprd_caa_ionmem_invalid(void *h_ionmem);

//sync ionmem
int sprd_caa_ionmem_sync(void *h_ionmem);

//获取ionmem虚拟地址
void *sprd_caa_ionmem_get_vaddr(void *h_ionmem);

//获取ionmem fd
int sprd_caa_ionmem_get_fd(void *h_ionmem);

//获取ionmem size
uint32_t sprd_caa_ionmem_get_size(void *h_ionmem);

/************************
 * binder
 ***********************/
//ProcessState::initWithDriver(const char *driver)
void ProcessState_initWithDriver(const char *driver);

//ProcessState::startThreadPool()
void ProcessState_startThreadPool();

//IPCThreadState::joinThreadPool(bool isMain)
void IPCThreadState_joinThreadPool(bool isMain);

//IPCThreadState::stopProcess(bool immediate)
void IPCThreadState_stopProcess(bool immediate);

#ifdef __cplusplus
}
#endif

#endif