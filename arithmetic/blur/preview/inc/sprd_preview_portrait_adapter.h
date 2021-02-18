#ifndef __SPRD_PREVIEW_PORTRAIT_ADAPTER_HEADER_H__
#define __SPRD_PREVIEW_PORTRAIT_ADAPTER_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

	#include "iSmooth.h"
	
	typedef enum {
		SPRD_PREVIEW_PORTRAIT_INIT_CMD = 0,
		SPRD_PREVIEW_PORTRAIT_DEINIT_CMD,
		SPRD_PREVIEW_PORTRAIT_GET_MASK_CMD,
		SPRD_PREVIEW_PORTRAIT_PROCESS_CMD,
		SPRD_PREVIEW_PORTRAIT_MAX_CMD
	} sprd_preview_portrait_cmd_t;

	typedef struct {
		void **ctx;
		InitGaussianParams *params;
	} sprd_preview_portrait_init_param_t;

	typedef struct {
		void **ctx;
	} sprd_preview_portrait_deinit_param_t;

	typedef struct {
		void **ctx;
		WeightGaussianParams *params;
	} sprd_preview_portrait_weightmap_param_t;
	
	typedef struct {
		void **ctx;
		void *srcImg;
		void *dstImg;
	} sprd_preview_portrait_blur_param_t;

	/*
	   capture_portrait adapter cmd process interface
	   return value: 0 is ok, other value is failed
	   @param: depend on cmd type:
	   - SPRD_PREVIEW_PORTRAIT_INIT_CMD：sprd_preview_portrait_init_param_t
	   - SPRD_PREVIEW_PORTRAIT_DEINIT_CMD：sprd_preview_portrait_deinit_param_t
	   - SPRD_CAPTURE_PORTRAIT_GET_MASK_CMD：sprd_preview_portrait_weightmap_param_t
	   - SPRD_CAPTURE_PORTRAIT_PROCESS_CMD: sprd_preview_portrait_blur_param_t
	 */
	JNIEXPORT int sprd_preview_portrait_adpt(sprd_preview_portrait_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif
