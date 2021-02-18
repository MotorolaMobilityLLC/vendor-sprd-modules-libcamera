#ifndef __SPRD_PREVIEW_BOKEH_ADAPTER_HEADER_H__
#define __SPRD_PREVIEW_BOKEH_ADAPTER_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

	typedef enum {
		SPRD_PREVIEW_BOKEH_INIT_CMD = 0,
		SPRD_PREVIEW_BOKEH_DEINIT_CMD,
		SPRD_PREVIEW_BOKEH_WEIGHTMAP_PROCESS_CMD,
		SPRD_PREVIEW_BOKEH_BLUR_PROCESS_CMD,
		SPRD_PREVIEW_BOKEH_MAX_CMD
	} sprd_preview_bokeh_cmd_t;

	typedef struct {
		void **ctx;
		int width;				/* image width */
		int height;				/* image height */
		int depth_width;
		int depth_height;
		int SmoothWinSize;			/* odd number */
		int ClipRatio;				/* RANGE 1:64 */
		int Scalingratio;			/* 2,4,6,8 */
		int DisparitySmoothWinSize; 		/* odd number */
	} sprd_preview_bokeh_init_param_t;

	typedef struct {
		void **ctx;
	} sprd_preview_bokeh_deinit_param_t;

	typedef struct {
		void **ctx;
		int F_number;				/* 1 ~ 20 */
		int sel_x;				/* The point which be touched */
		int sel_y;				/* The point which be touched */
		void *DisparityImage;
	} sprd_preview_bokeh_weight_param_t;

	typedef struct {
		void **ctx;
		void *Src_YUV;             		/* graphic buffer */
		void *Output_YUV;			/* graphic buffer */
	} sprd_preview_bokeh_blur_param_t;

	/*
	   preview_bokeh adapter cmd process interface
	   return value: 0 is ok, other value is failed
	   @param: depend on cmd type:
	   - SPRD_PREVIEW_BOKEH_INIT_CMD：sprd_bokeh_init_param_t
	   - SPRD_PREVIEW_BOKEH_DEINIT_CMD：sprd_bokeh_deinit_param_t
	   - SPRD_PREVIEW_BOKEH_WEIGHTMAP_PROCESS_CMD: sprd_bokeh_weight_param_t
	   - SPRD_PREVIEW_BOKEH_BLUR_PROCESS_CMD: sprd_bokeh_blur_param_t
	 */
	JNIEXPORT int iBokehCtrl_adpt(sprd_preview_bokeh_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif
