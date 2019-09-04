#ifndef _XRP_INTERFACE_INTERNAL_H
#define _XRP_INTERFACE_INTERNAL_H
#include <stddef.h>
#include "vdsp_interface.h"
#include "xrp_api.h"

#define USER_LIBRARY_CMD_LOAD_UNLOAD_INPUTSIZE 44
struct sprd_vdsp_inout
{
	int fd;
	char *vir_addr;
	uint32_t phy_addr;
	uint32_t size;
	enum sprd_vdsp_bufflag flag;
};
#if 0
enum sprd_vdsp_status
{
	SPRD_XRP_STATUS_SUCCESS = 0,
	SPRD_XRP_STATUS_FAILURE,
	SPRD_XRP_STATUS_PENDING,
};
enum sprd_xrp_queue_priority
{
	SPRD_XRP_PRIORITY_0 = 0,
	SPRD_XRP_PRIORITY_1,
	SPRD_XRP_PRIORITY_2,
	SPRD_XRP_PRIORITY_MAX
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

	void *sprd_vdsp_open_device(int idx , enum sprd_vdsp_worktype type);
	void sprd_vdsp_release_device(void *device);

	int sprd_vdsp_send_command(void *device , const char *nsid , 
					struct sprd_vdsp_inout *input, struct sprd_vdsp_inout *output, 
					struct sprd_vdsp_inout *buffer ,  uint32_t buf_num,
					enum sprd_xrp_queue_priority priority);
	int sprd_vdsp_send_command_directly(void *device , const char *nsid , struct sprd_vdsp_inout *input, struct sprd_vdsp_inout *output,
					struct sprd_vdsp_inout *buffer ,  uint32_t buf_num, enum sprd_xrp_queue_priority priority);
	int sprd_vdsp_run_faceid_command_directly(void *device,unsigned long in_data, unsigned int in_height,
								unsigned int in_width,unsigned int *out_result,int out_fd);
	int sprd_vdsp_load_library(void *device , struct sprd_vdsp_inout *buffer , const char *libname , enum sprd_xrp_queue_priority priority);
	int sprd_vdsp_unload_library(void *device , const char *libname , enum sprd_xrp_queue_priority priority);
#ifdef __cplusplus
}
#endif

#endif
