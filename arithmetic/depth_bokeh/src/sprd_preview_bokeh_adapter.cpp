#include "sprd_preview_bokeh_adapter.h"
#include "iBokeh.h"
#include <utils/Log.h>

#define LOG_TAG "sprd_preview_bokeh_adapter"
#define ADAPTER_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define ADAPTER_LOGD(format,...) ALOGD(format, ##__VA_ARGS__)

int iBokehInit_adpt(void **handle, InitParams *params)
{
	return iBokehInit(handle, params);
}

int iBokehCreateWeightMap_adpt(void **handle, WeightParams *params)
{
	return iBokehCreateWeightMap(*handle, params);
}

int iBokehBlurImage_adpt(void **handle, void *Src_YUV, void *Output_YUV)
{
	return iBokehBlurImage(*handle, Src_YUV, Output_YUV);
}

int iBokehDeinit_adpt(void **handle)
{
	return iBokehDeinit(*handle);
}

int iBokehCtrl_adpt(sprd_preview_bokeh_cmd_t cmd, void *param)
{
	int ret = 0;
	if(param==NULL)
	{
		ADAPTER_LOGE("params is NULL\n");
		return -1;
	}

	switch (cmd) 
	{
		case SPRD_PREVIEW_BOKEH_INIT_CMD: 
			{
				sprd_preview_bokeh_init_param_t *bokeh_param=(sprd_preview_bokeh_init_param_t *)param;
				InitParams init_param;
				init_param.width=bokeh_param->width;
				init_param.height=bokeh_param->height;
				init_param.depth_width=bokeh_param->depth_width;
				init_param.depth_height=bokeh_param->depth_height;
				init_param.SmoothWinSize=bokeh_param->SmoothWinSize;
				init_param.ClipRatio=bokeh_param->ClipRatio;
				init_param.Scalingratio=bokeh_param->Scalingratio;
				init_param.DisparitySmoothWinSize=bokeh_param->DisparitySmoothWinSize;
				ret = iBokehInit_adpt(bokeh_param->ctx, &init_param);

				break;
			}
		case SPRD_PREVIEW_BOKEH_DEINIT_CMD: 
			{
				sprd_preview_bokeh_deinit_param_t *bokeh_param=(sprd_preview_bokeh_deinit_param_t *)param;
				ret = iBokehDeinit_adpt(bokeh_param->ctx);

				break;
			}
		case SPRD_PREVIEW_BOKEH_WEIGHTMAP_PROCESS_CMD: 
			{
				sprd_preview_bokeh_weight_param_t *bokeh_param=(sprd_preview_bokeh_weight_param_t *)param;
				WeightParams weight_param;
				weight_param.F_number=bokeh_param->F_number;
				weight_param.sel_x=bokeh_param->sel_x;
				weight_param.sel_y=bokeh_param->sel_y;
				weight_param.DisparityImage=(unsigned char *)bokeh_param->DisparityImage;
				ret = iBokehCreateWeightMap_adpt(bokeh_param->ctx, &weight_param);

				break;
			}
		case SPRD_PREVIEW_BOKEH_BLUR_PROCESS_CMD: 
			{
				sprd_preview_bokeh_blur_param_t *bokeh_param=(sprd_preview_bokeh_blur_param_t *)param;		
				ret = iBokehBlurImage_adpt(bokeh_param->ctx, bokeh_param->Src_YUV, bokeh_param->Output_YUV);

				break;
			}									
		default:
			ADAPTER_LOGE("unknown cmd: %d\n", cmd);
	}

	return ret;
}
