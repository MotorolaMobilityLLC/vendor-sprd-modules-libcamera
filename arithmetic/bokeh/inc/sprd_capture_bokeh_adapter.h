#ifndef __SPRD_CAPTURE_BOKEH_ADAPTER_HEADER_H__
#define __SPRD_CAPTURE_BOKEH_ADAPTER_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

#include "sprdbokeh.h"

	typedef enum {
		SPRD_CAPTURE_BOKEH_INIT_CMD = 0,
		SPRD_CAPTURE_BOKEH_DEINIT_CMD,
		SPRD_CAPTURE_BOKEH_USERSET_CMD,
		SPRD_CAPTURE_BOKEH_GDEPTH_CMD,
		SPRD_CAPTURE_BOKEH_REFOCUSPRE_CMD,
		SPRD_CAPTURE_BOKEH_REFOCUSGEN_CMD,
		SPRD_CAPTURE_BOKEH_REFOCUSGEN_PORTRAIT_CMD,
		SPRD_CAPTURE_BOKEH_MAX_CMD
	} sprd_capture_bokeh_cmd_t;

	typedef struct {
		void **ctx;
		int imgW;
		int imgH;
		init_param *param;
	} sprd_capture_bokeh_init_param_t;

	typedef struct {
		void **ctx;
	} sprd_capture_bokeh_deinit_param_t;

	typedef struct {
		void *ptr;
		int size;
	} sprd_capture_bokeh_info_param_t;

	typedef struct {
		void **ctx;
		int depthW;
		int depthH; 
		void *depthOut;
		gdepth_param *param;
	} sprd_capture_bokeh_gdepth_param_t;

	typedef struct {
		void **ctx;
		void *srcImg;
		void *depthIn;
		void *params;
		int depthW;
		int depthH; 
	} sprd_capture_bokeh_pre_param_t;

	typedef struct {
		void **ctx;
		void *dstImg;
		int F_number;		/* 1 ~ 20 */
		int sel_x;		/* The point which be touched */
		int sel_y;		/* The point which be touched */
	} sprd_capture_bokeh_gen_param_t;

	/*
	   capture_bokeh adapter cmd process interface
	   return value: 0 is ok, other value is failed
	   @param: depend on cmd type:
	   - SPRD_CAPTURE_BOKEH_INIT_CMD: sprd_bokeh_init_param_t
	   - SPRD_CAPTURE_BOKEH_DEINIT_CMD: sprd_bokeh_deinit_param_t
	   - SPRD_CAPTURE_BOKEH_USERSET_CMD: sprd_bokeh_info_param_t
	   - SPRD_CAPTURE_BOKEH_VERSION_GET_CMD: param reserved
	   - SPRD_CAPTURE_BOKEH_GDEPTH_CMD: sprd_bokeh_gdepth_param_t
	   - SPRD_CAPTURE_BOKEH_REFOCUSPRE_CMD: sprd_bokeh_pre_param_t
	   - SPRD_CAPTURE_BOKEH_REFOCUSGEN_CMD: sprd_bokeh_gen_param_t
	   - SPRD_CAPTURE_BOKEH_REFOCUSGEN_PORTRAIT_CMD: sprd_bokeh_gen_param_t
	 */
	JNIEXPORT int sprd_bokeh_ctrl_adpt(sprd_capture_bokeh_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif
