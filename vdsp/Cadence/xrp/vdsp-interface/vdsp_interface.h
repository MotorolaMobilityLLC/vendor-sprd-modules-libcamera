#ifndef _SPRD_VDSP_INTERFACE__H
#define _SPRD_VDSP_INTERFACE__H


enum sprd_vdsp_status
{
        SPRD_XRP_STATUS_SUCCESS = 0,
        SPRD_XRP_STATUS_FAILURE,
        SPRD_XRP_STATUS_PENDING,
};
enum sprd_vdsp_result
{
	SPRD_VDSP_RESULT_SUCCESS = 0,
	SPRD_VDSP_RESULT_FAIL,
	SPRD_VDSP_RESULT_OLD_GENERATION,
	SPRD_VDSP_RESULT_MAX,
};
enum sprd_vdsp_power_level
{
	SPRD_VDSP_POWERHINT_NORMAL,
	SPRD_VDSP_POWERHINT_LEVEL_0,
	SPRD_VDSP_POWERHINT_LEVEL_1,
	SPRD_VDSP_POWERHINT_LEVEL_2,
	SPRD_VDSP_POWERHINT_LEVEL_MAX,
};
enum sprd_vdsp_bufflag
{
	SPRD_VDSP_XRP_READ,
	SPRD_VDSP_XRP_WRITE,
	SPRD_VDSP_XRP_READ_WRITE,
	SPRD_VDSP_XRP_ACCESS_MAX,
};
struct sprd_vdsp_client_inout
{
	int32_t fd;
	void *viraddr;
	uint32_t phy_addr;
	uint32_t size;
	enum sprd_vdsp_bufflag flag;
};
enum sprd_vdsp_worktype
{
	SPRD_VDSP_WORK_NORMAL,
	SPRD_VDSP_WORK_FACEID,
	SPRD_VDSP_WORK_MAXTYPE,
};
enum sprd_xrp_queue_priority
{
        SPRD_XRP_PRIORITY_0 = 0,
        SPRD_XRP_PRIORITY_1,
        SPRD_XRP_PRIORITY_2,
        SPRD_XRP_PRIORITY_MAX
};

struct vdsp_handle
{
        int32_t fd;
        uint32_t generation;
};
typedef struct
{
	uint32_t width, height;
	uint32_t phyaddr;
	uint32_t liveness;
}FACEID_IN;
#ifdef __cplusplus
extern "C" {
#endif

/************************************************************
open type is SPRD_VDSP_WORK_NORMAL for arithmetic firmwwareand SPRD_VDSP_WORK_FACEID for faceid firmware
       handle -------- handle.fd is fd; generation is xrp server generation
return value: 0 is ok, other value is failed
***********************************************************/
enum sprd_vdsp_result sprd_cavdsp_open_device(enum sprd_vdsp_worktype type , struct vdsp_handle *handle);

/****************************************************************
sprd_cavdsp_close_device
param:
vdsphandle is handl which get from sprd_cavdsp_open_device
return value:
SPRD_VDSP_RESULT_SUCCESS   ------ ok
SPRD_VDSP_RESULT_FAIL    --------failed
SPRD_VDSP_RESULT_OLD_GENERATION   -----return failed
					because of xrp server may be restart,
					the generation is not matched
****************************************************************/
enum sprd_vdsp_result sprd_cavdsp_close_device(void *vdsphandle);

/****************************************************************
sprd_cavdsp_send_cmd
param:
handle  ---- handle get from sprd_cavdsp_open_device
nsid    ---- library name or system cmd name
in      ---- input param
out     ---- output param
buffer  ---- buffer descriptor pointer which pointer the
	     bufnum of struct sprd_vdsp_client_inout
bufnum  ---- buffer number
priority ---- cmd priority from 0~?
return value:
SPRD_VDSP_RESULT_SUCCESS   ------ ok
SPRD_VDSP_RESULT_FAIL    --------failed
SPRD_VDSP_RESULT_OLD_GENERATION   -----return failed
                                        because of xrp server may be restart,
                                        the generation is not matched
****************************************************************/
enum sprd_vdsp_result sprd_cavdsp_send_cmd(void *handle , const char *nsid , struct sprd_vdsp_client_inout *in, struct sprd_vdsp_client_inout *out , 
						struct sprd_vdsp_client_inout *buffer , uint32_t bufnum , uint32_t priority);

/************************************************************
sprd_cavdsp_loadlibrary
param:
handle   ------ handle get from sprd_cavdsp_open_device
libname  ------ library name which need be loaded
buffer   ------ library code and data buffer ,which store the library code and data
return value:
SPRD_VDSP_RESULT_SUCCESS   ------ ok
SPRD_VDSP_RESULT_FAIL    --------failed
SPRD_VDSP_RESULT_OLD_GENERATION   -----return failed
                                        because of xrp server may be restart,
                                        the generation is not matched
************************************************************/
enum sprd_vdsp_result sprd_cavdsp_loadlibrary(void *handle , const char *libname , struct sprd_vdsp_client_inout *buffer);

/************************************************************
sprd_cavdsp_unloadlibrary
param:
handle ---- handle get from sprd_cavdsp_open_device
libname ----library name which need be unloaded
return value:
SPRD_VDSP_RESULT_SUCCESS   ------ ok
SPRD_VDSP_RESULT_FAIL    --------failed
SPRD_VDSP_RESULT_OLD_GENERATION   -----return failed
                                        because of xrp server may be restart,
                                        the generation is not matched
************************************************************/
enum sprd_vdsp_result sprd_cavdsp_unloadlibrary(void *handle , const char *libname);

/***************
sprd_cavdsp_power_hint
param:
handle ---- handle get from sprd_cavdsp_open_device
setunset --- 1 is set power hint, 0 is unset restore
level ---- power level
permanent ---- 1 is permanent setting , 0 is only set once and then recover dvfs mode.
*************/
enum sprd_vdsp_result sprd_cavdsp_power_hint(void *handle , enum sprd_vdsp_power_level level , uint32_t permanent);

// ion mem
void* sprd_alloc_ionmem(uint32_t size, uint8_t iscache, int32_t* fd, void** viraddr);
void* sprd_alloc_ionmem2(uint32_t size, uint8_t iscache, int32_t* fd, void** viraddr, unsigned long* phyaddr);
enum sprd_vdsp_status sprd_free_ionmem(void* handle);
enum sprd_vdsp_status sprd_flush_ionmem(void* handle, void* vaddr, void* paddr, uint32_t size);
enum sprd_vdsp_status sprd_invalid_ionmem(void* handle);


#ifdef __cplusplus
}
#endif

#endif

