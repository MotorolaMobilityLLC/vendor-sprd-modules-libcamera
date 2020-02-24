#ifndef __SPRD_WT_ADAPTER_HEADER_H__
#define __SPRD_WT_ADAPTER_HEADER_H__

#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))

typedef enum {
	SPRD_WT_RUN_CMD,
	SPRD_WT_MAX_CMD
} sprd_wt_cmd_t;

typedef struct {
	int image_width_wide;
	int image_height_wide;
	int image_width_tele;
	int image_height_tele;
	void *otpbuf;
	int otpsize;
	int VCMup;//VCM Upper Limit
        int VCMdown; //VCM Lower Limit
} sprd_wt_init_param_t;
	
typedef struct {
        int VCM_cur_value;
	float ScaleRatio;
} wtrun_inparam;

typedef struct {
 	struct sprd_camalg_image input[2];//[0]input_image_wide   [1]input_image_tele 
	struct sprd_camalg_image output;//output_image 
	wtrun_inparam params;
}sprd_wt_run_param_t;
  

/*
	init wt adapter instance
	return value: handle; 
	@param: sprd_wt_init_param_t
*/
JNIEXPORT void *sprd_wt_adpt_init(void *param);

/*
	deinit wt adapter instance 
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_wt_adpt_deinit(void *handle);

/*
	wt adapter cmd process interface
	return value: 0 is ok, other value is failed
	@param: depend on cmd type:
		- SPRD_DEPTH_RUN_CMD: sprd_wt_run_param_t
*/
JNIEXPORT int sprd_wt_adpt_ctrl(void *handle, sprd_wt_cmd_t cmd, void *param);

/*
	wt adapter get running type, the output is type, such as cpu/gpu/vdsp 
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_wt_get_devicetype(enum camalg_run_type *type);

/*
	wt adapter set running type 
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_wt_set_devicetype(enum camalg_run_type type);


#ifdef __cplusplus
}
#endif

#endif
