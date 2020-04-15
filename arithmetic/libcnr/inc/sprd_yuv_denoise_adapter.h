#ifdef CAMERA_CNR3_ENABLE
#ifndef __SPRD_YUV_DENOISE_ADAPTER_HEADER_H__
#define __SPRD_YUV_DENOISE_ADAPTER_HEADER_H__

#include "Denoise_SPRD.h"
#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))

	typedef enum {
		SPRD_YNR_PROCESS_CMD = 1,
		SPRD_CNR2_PROCESS_CMD,
		SPRD_YNR_CNR2_PROCESS_CMD,
		SPRD_CNR3_PROCESS_CMD,
		SPRD_YNR_CNR3_PROCESS_CMD,
		SPRD_CNR2_CNR3_PROCESS_CMD,
		SPRD_YNR_CNR2_CNR3_PROCESS_CMD,
		SPRD_YUV_DENOISE_MAX_CMD
	} sprd_yuv_denoise_cmd_t;

	typedef struct {
		int width;
		int height;
		int runversion;
	} sprd_yuv_denoise_init_t;

	typedef struct {
		struct sprd_camalg_image bufferY;
		struct sprd_camalg_image bufferUV;
		YNR_Param *ynrParam; 
		CNR_Parameter *cnr2Param; //CNR2.0
		cnr_param_t *cnr3Param;   //CNR3.0
		int width;
		int height;
		float zoom_ratio;
		int ynr_ration_base;
		int cnr_ration_base;
	} sprd_yuv_denoise_param_t;

	/*
	init yuv_denoise adapter instance
	return value: handle; 
	@param: reserved, pass 0 is ok
	*/
	JNIEXPORT void *sprd_yuv_denoise_adpt_init(void *param);

	/*
	deinit yuv_denoise adapter instance 
	return value: 0 is ok, other value is failed
	*/
	JNIEXPORT int sprd_yuv_denoise_adpt_deinit(void *handle);

	/*
	yuv_denoise adapter cmd process interface
	return value: 0 is ok, other value is failed
	*/
	JNIEXPORT int sprd_yuv_denoise_adpt_ctrl(void *handle, sprd_yuv_denoise_cmd_t cmd, void *param);

	/*
	yuv_denoise adapter get running type, the output is type, such as cpu/gpu/vdsp 
	return value: 0 is ok, other value is failed
	*/
	JNIEXPORT int sprd_yuv_denoise_get_devicetype(enum camalg_run_type *type);

	/*
	yuv_denoise adapter set running type 
	return value: 0 is ok, other value is failed
	*/
	JNIEXPORT int sprd_yuv_denoise_set_devicetype(enum camalg_run_type type);

#ifdef __cplusplus
}
#endif

#endif

#else
#ifndef __SPRD_YUV_DENOISE_ADAPTER_HEADER_H__
#define __SPRD_YUV_DENOISE_ADAPTER_HEADER_H__

#include "Denoise_SPRD.h"
#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))

  typedef enum {
    SPRD_YNR_PROCESS_CMD = 0,
    SPRD_CNR_PROCESS_CMD,
    SPRD_YNR_CNR_PROCESS_CMD,
    SPRD_YUV_DENOISE_MAX_CMD
  } sprd_yuv_denoise_cmd_t;

  typedef struct {
    int width;
    int height;
    int runversion;
  } sprd_yuv_denoise_init_t;

  typedef struct {
    struct sprd_camalg_image bufferY;
    struct sprd_camalg_image bufferUV;
    YNR_Param *ynrParam; 
    CNR_Parameter *cnr2Param; //CNR2.0
    cnr_param_t *cnr3Param;   //CNR3.0
    int width;
    int height;
    float zoom_ratio;
  } sprd_yuv_denoise_param_t;

  /*
     init yuv_denoise adapter instance
     return value: handle; 
     @param: reserved, pass 0 is ok
     */
  JNIEXPORT void *sprd_yuv_denoise_adpt_init(void *param);

  /*
     deinit yuv_denoise adapter instance 
     return value: 0 is ok, other value is failed
     */
  JNIEXPORT int sprd_yuv_denoise_adpt_deinit(void *handle);

  /*
     yuv_denoise adapter cmd process interface
     return value: 0 is ok, other value is failed
     */
  JNIEXPORT int sprd_yuv_denoise_adpt_ctrl(void *handle, sprd_yuv_denoise_cmd_t cmd, void *param);

  /*
     yuv_denoise adapter get running type, the output is type, such as cpu/gpu/vdsp 
     return value: 0 is ok, other value is failed
     */
  JNIEXPORT int sprd_yuv_denoise_get_devicetype(enum camalg_run_type *type);

  /*
     yuv_denoise adapter set running type
     return value: 0 is ok, other value is failed
     */
  JNIEXPORT int sprd_yuv_denoise_set_devicetype(enum camalg_run_type type);

#ifdef __cplusplus
}
#endif

#endif

#endif
