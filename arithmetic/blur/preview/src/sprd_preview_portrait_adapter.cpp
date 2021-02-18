#include "sprd_preview_portrait_adapter.h"
#include <utils/Log.h>

#define LOG_TAG "sprd_preview_portrait_adapter"
#define ADAPTER_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define ADAPTER_LOGD(format,...) ALOGD(format, ##__VA_ARGS__)

int sprd_portrait_preview_init_adpt(void **handle, InitGaussianParams *params)
{
	return iSmoothInit(handle, params);
}

int sprd_portrait_preview_weightmap_adpt(void **handle, WeightGaussianParams *params)
{
	return iSmoothCreateWeightMap(*handle, params);
}

int sprd_portrait_preview_blur_adpt(void **handle, void *srcImg, void *dstImg)
{
	return iSmoothBlurImage(*handle, srcImg, dstImg);
}

int sprd_portrait_preview_deinit_adpt(void **handle)
{
	return iSmoothDeinit(*handle);
}

int sprd_preview_portrait_adpt(sprd_preview_portrait_cmd_t cmd, void *param)
{
	int ret = 0;
	if(param==NULL)
	{
		ADAPTER_LOGE("params is NULL\n");
		return -1;
	}

	switch (cmd) 
	{
		case SPRD_PREVIEW_PORTRAIT_INIT_CMD: 
			{
				sprd_preview_portrait_init_param_t *portrait_param=(sprd_preview_portrait_init_param_t *)param;			
				ret = sprd_portrait_preview_init_adpt(portrait_param->ctx, portrait_param->params);

				break;
			}
		case SPRD_PREVIEW_PORTRAIT_DEINIT_CMD: 
			{
				sprd_preview_portrait_deinit_param_t *portrait_param=(sprd_preview_portrait_deinit_param_t *)param;
				ret = sprd_portrait_preview_deinit_adpt(portrait_param->ctx);

				break;
			}
		case SPRD_PREVIEW_PORTRAIT_GET_MASK_CMD: 
			{
				sprd_preview_portrait_weightmap_param_t *portrait_param=(sprd_preview_portrait_weightmap_param_t *)param;		
				ret = sprd_portrait_preview_weightmap_adpt(portrait_param->ctx, portrait_param->params);

				break;
			}	
		case SPRD_PREVIEW_PORTRAIT_PROCESS_CMD: 
			{
				sprd_preview_portrait_blur_param_t *portrait_param=(sprd_preview_portrait_blur_param_t *)param;		
				ret = sprd_portrait_preview_blur_adpt(portrait_param->ctx, portrait_param->srcImg, portrait_param->dstImg);

				break;
			}				
		default:
			ADAPTER_LOGE("unknown cmd: %d\n", cmd);
	}

	return ret;
}
