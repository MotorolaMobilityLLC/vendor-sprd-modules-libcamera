#ifndef __SPRD_EE_ADAPTER_H__
#define __SPRD_EE_ADAPTER_H__

#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))

	typedef enum {
		SPRD_EE_OPEN_CMD = 0,
		SPRD_EE_CLOSE_CMD,
		SPRD_EE_PROCESS_CMD,
		SPRD_EE_MAX_CMD
	} sprd_ee_cmd_t;


	typedef struct {
		void *ctx;
		struct sprd_camalg_image imgIn;		/*input image buffer*/
		struct sprd_camalg_image imgOut;	/*output image buffer*/
		void *tuningParam;			/*tuning params*/
		int tuningSize;				/*the size of tuningParam*/
		int mode_idx;				/*mode index*/
		int scene_idx;				/*scene index*/
		int level_idx[2];			/*level index array*/
		int level_weight[2];			/*level weight array*/
		int level_num;				/*level number*/
		void *scene_map_buffer;			/*scene map buffer*/
		int width;				/*image width*/
		int height;				/*image height*/
		int crop_width;				/*image crop width*/
		int crop_height;			/*image crop height*/
		void *ae_param; /*param structure pointer from ae*/
	} sprd_ee_param_t;

	/*
	ee adapter cmd process interface
	return value: 0 is ok, other value is failed
	*/
	JNIEXPORT int sprd_ee_adpt_ctrl(sprd_ee_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif
