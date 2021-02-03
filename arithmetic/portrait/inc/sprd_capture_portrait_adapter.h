#ifndef __SPRD_CAPTURE_PORTRAIT_ADAPTER_HEADER_H__
#define __SPRD_CAPTURE_PORTRAIT_ADAPTER_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

	#include "PortraitCapture_Interface.h"
	
	typedef enum {
		SPRD_CAPTURE_PORTRAIT_INIT_CMD = 0,
		SPRD_CAPTURE_PORTRAIT_DEINIT_CMD,
		SPRD_CAPTURE_PORTRAIT_GET_MASK_INFO_CMD,
		SPRD_CAPTURE_PORTRAIT_GET_MASK_CMD,
		SPRD_CAPTURE_PORTRAIT_PROCESS_CMD,
		SPRD_CAPTURE_PORTRAIT_MAX_CMD
	} sprd_capture_portrait_cmd_t;

	typedef struct {
		void **ctx;
		PortraitCap_Init_Params *param;
	} sprd_capture_portrait_init_param_t;

	typedef struct {
		void **ctx;
	} sprd_capture_portrait_deinit_param_t;

	typedef struct {
		void **ctx;
		void *width;
		void *height;
		void *bufSize;
	} sprd_capture_portrait_get_mask_info_param_t;
	
		typedef struct {
		void **ctx;
		ProcDepthInputMap *depthInputData;
		PortaitCapProcParams *procParams;
		InoutYUV *yuvData;
		void *bokehMask;
		void *lptMask;
	} sprd_capture_portrait_get_mask_param_t;
	
		typedef struct {
		void **ctx;
		InoutYUV *yuvData;
		void *bokehMask;
		int isCapture;
	} sprd_capture_portrait_process_param_t;

	/*
	   capture_portrait adapter cmd process interface
	   return value: 0 is ok, other value is failed
	   @param: depend on cmd type:
	   - SPRD_CAPTURE_PORTRAIT_INIT_CMD：sprd_capture_portrait_init_param_t
	   - SPRD_CAPTURE_PORTRAIT_DEINIT_CMD：sprd_capture_portrait_deinit_param_t
	   - SPRD_CAPTURE_PORTRAIT_GET_MASK_INFO_CMD：sprd_capture_portrait_get_mask_info_param_t
	   - SPRD_CAPTURE_PORTRAIT_GET_MASK_CMD：sprd_capture_portrait_get_mask_param_t
	   - SPRD_CAPTURE_PORTRAIT_PROCESS_CMD: sprd_capture_portrait_process_param_t
	 */
	JNIEXPORT int sprd_capture_portrait_adpt(sprd_capture_portrait_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif
