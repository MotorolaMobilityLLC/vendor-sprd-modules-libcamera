#ifndef __SPRD_HDR_ADAPTER_HEADER_H__
#define __SPRD_HDR_ADAPTER_HEADER_H__

#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))
#define HDR_IMG_NUM_MAX		3

typedef enum {
	SPRD_HDR_GET_VERSION_CMD = 0,
	SPRD_HDR_PROCESS_CMD,
	SPRD_HDR_FAST_STOP_CMD,
	SPRD_HDR_MAX_CMD
} sprd_hdr_cmd_t;

typedef struct {
	uint8_t		major;              /*!< API major version */
	uint8_t		minor;              /*!< API minor version */
	uint8_t		micro;              /*!< API micro version */
	uint8_t		nano;               /*!< API nano version */
	char		built_date[0x20];   /*!< API built date */
	char		built_time[0x20];   /*!< API built time */
	char		built_rev[0x100];	/*!< API built version, linked with vcs resivion> */
} sprd_hdr_version_t;

typedef struct {
	struct sprd_camalg_image input[HDR_IMG_NUM_MAX];//hdr2: use input[0]&[1]
	struct sprd_camalg_image output;
	float ev[HDR_IMG_NUM_MAX];//hdr2: use ev[0]&[1]
} sprd_hdr_param_t;

/*
	init hdr adapter instance
	return value: handle; 
	@max_width/height: max supported width/height
	@param: reserved, pass 0 is ok
*/
JNIEXPORT void *sprd_hdr_adpt_init(int max_width, int max_height, void *param);

/*
	deinit hdr adapter instance 
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_hdr_adpt_deinit(void *handle);

/*
	hdr adapter cmd process interface
	return value: 0 is ok, other value is failed
	@param: depend on cmd type:
		- SPRD_HDR_GET_VERSION_CMD: sprd_hdr_version_t
		- SPRD_HDR_PROCESS_CMD: sprd_hdr_param_t
		- SPRD_HDR_FAST_STOP_CMD: 0
*/
JNIEXPORT int sprd_hdr_adpt_ctrl(void *handle, sprd_hdr_cmd_t cmd, void *param);

/*
	hdr adapter get running type, the output is type, such as cpu/gpu/vdsp 
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_hdr_get_devicetype(enum camalg_run_type *type);

/*
	hdr adapter set running type 
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_hdr_set_devicetype(enum camalg_run_type type);

#ifdef __cplusplus
}
#endif

#endif
