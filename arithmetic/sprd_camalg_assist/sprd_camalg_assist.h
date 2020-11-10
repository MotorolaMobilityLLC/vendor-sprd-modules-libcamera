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
//check vdsp is supported
int sprd_caa_vdsp_check_supported();

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

/************************
 * GraphicBuffer
 ***********************/
#define FORMAT_RGB_888        0
#define FORMAT_YCRCB_420_SP   1//NV21
#define FORMAT_RGBA_8888      2

//分配GraphicBuffer，返回h_graphic_buffer
void *GraphicBuffer_new(uint32_t width, uint32_t height, int format);

//释放GraphicBuffer
void GraphicBuffer_delete(void *h_graphic_buffer);

//lock GraphicBuffer，返回CPU地址
void *GraphicBuffer_lock(void *h_graphic_buffer);

//unlock GraphicBuffer
void GraphicBuffer_unlock(void *h_graphic_buffer);

//getNativeBuffer
void *GraphicBuffer_getNativeBuffer(void *h_graphic_buffer);

#ifdef __cplusplus
}
#endif

#endif