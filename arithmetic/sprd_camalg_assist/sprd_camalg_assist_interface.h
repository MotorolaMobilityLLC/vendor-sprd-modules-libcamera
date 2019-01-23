#ifndef __SPRD_VDSP_CMD_INTERFACE_H__
#define __SPRD_VDSP_CMD_INTERFACE_H__

typedef enum sprd_vdsp_cmdresult {
	SPRD_VDSP_CMD_SUCCESS = 0,
	SPRD_VDSP_CMD_FAIL
} sprd_vdsp_cmd_result;

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef int int32_t;

#ifdef __cplusplus
extern "C" {
#endif

// vdsp drv
int sprd_vdsp_open_drv();
int sprd_vdsp_close_drv(int32_t fd);

// vdsp cmd
void* sprd_vdsp_alloc_cmd(const char* appid, int32_t inputfd, uint32_t inputsize, 
	int32_t outputfd, uint32_t outputsize, uint32_t buffernum, uint32_t priority);
int sprd_vdsp_free_cmd(void* pcmd);
int sprd_vdsp_cmd_addbuffer(void* pcmd, int32_t buffer_fd, uint32_t size, uint32_t index);
int sprd_vdsp_send_cmd(int32_t fd, void* cmd);

// ion mem
void* sprd_alloc_ionmem(uint32_t size, uint8_t iscache, int32_t* fd, void** viraddr);
int sprd_free_ionmem(void* handle);
int sprd_flush_ionmem(void* handle, void* vaddr, void* paddr, uint32_t size);
int sprd_invalid_ionmem(void* handle);

#ifdef __cplusplus
}
#endif

#endif


