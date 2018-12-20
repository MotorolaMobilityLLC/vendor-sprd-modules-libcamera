/*-------------------------------------------------------------------*/
/*  Copyright(C) 2015 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/*
    Face Detection Library API
*/

#ifndef __SPRD_FDAPI_H__
#define __SPRD_FDAPI_H__
#include <sys/types.h>

#define OUT_BUFFER_SIZE 1024*64
#define DIM_BUFFER_SIZE 1024*1024*2

typedef unsigned long fd_uint;
typedef long fd_int;
typedef uint64_t fd_u64;
typedef int64_t fd_s64;
typedef unsigned int fd_u32;
typedef int fd_s32;
typedef unsigned short fd_u16;
typedef short fd_s16;
typedef unsigned char fd_u8;
typedef signed char fd_s8;


/* Face Detector handle */
typedef void * HWFD_DETECTOR_HANDLE;

enum FD_IP_STATUS
{
	FD_IP_IDLE = 1000,
	FD_IP_BUSY ,
};

typedef struct{
	void* fd_cmd_addr;
	fd_u32 fd_cmd_size;
	fd_u32 fd_cmd_num;
	fd_u32 fd_image_linestep;
#ifdef FD_DEBUG	
	void* fd_image_addr_debug;
	fd_u32 fd_image_size_debug;
#else
	fd_s32 fd_image_addr;
#endif
}HWFD_REQUEST;


typedef struct{
	int error_code;
	unsigned int face_count;
	void* data;
}HWFD_RESPONSE;

enum {
	HWFD_ERROR = -1,
	HWFD_OK,
};

#ifdef  __cplusplus
extern "C" {
#endif


int  hwfd_open(HWFD_DETECTOR_HANDLE *hDT);
int  hwfd_start_fd(HWFD_DETECTOR_HANDLE hDT,void* i_request,void* o_response);
int  hwfd_is_busy(HWFD_DETECTOR_HANDLE hDT);
void  hwfd_close(HWFD_DETECTOR_HANDLE *hDT);



#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FDAPI_H__ */
