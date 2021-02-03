#include "../inc/sprd_capture_portrait_adapter.h"
#include "sprdfacebeautyfdr.h"
#include "sprdfdapi.h"
#include <utils/Log.h>
#include <string.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "sprd_capture_portrait_adapter"
#define ADAPTER_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define ADAPTER_LOGD(format,...) ALOGD(format, ##__VA_ARGS__)

static int imgWidth=0;
static int imgHeight=0;
int sprd_portrait_fb(void *image, int width, int height)
{
	FD_IMAGE fdImage;
	memset(&fdImage, 0, sizeof(FD_IMAGE));
	fdImage.data = (unsigned char *)image;
	fdImage.width = width;
	fdImage.height = height;
	fdImage.step = width;
	fdImage.context.orientation = 270;

	void *fd_handle = 0;
	FD_OPTION option = {0};
	option.fdEnv = FD_ENV_SW;

	FdInitOption(&option);

	option.platform = PLATFORM_ID_SHARKL5P;
	option.workMode = FD_WORKMODE_MOVIE;
	option.maxFaceNum = 10;
	option.minFaceSize = ((fdImage.height < fdImage.width) ? fdImage.height:fdImage.width) / 12;
	option.directions = FD_DIRECTION_ALL;
	option.angleFrontal = FD_ANGLE_RANGE_90;
	option.angleHalfProfile = FD_ANGLE_RANGE_30;
	option.angleFullProfile = FD_ANGLE_RANGE_30;
	option.detectDensity = 5;
	option.scoreThreshold = 0;
	option.detectInterval = 6;
	option.trackDensity = 5;
	option.lostRetryCount = 1;
	option.lostHoldCount = 1;
	option.holdPositionRate = 5;
	option.holdSizeRate = 4;
	option.swapFaceRate = 200;
	option.guessFaceDirection = 1;

	FdCreateDetector(&fd_handle, &option);

	FdDetectFace(fd_handle, &fdImage);

	FD_FACEINFO face;
	FBFDR_FACEINFO faceInfo[10];
	int faceNum = FdGetFaceCount(fd_handle);
	faceNum = faceNum > 10 ? 10:faceNum;
	for (int i = 0; i < faceNum; i++) {
		FdGetFaceInfo(fd_handle, i, &face);

		faceInfo[i].x = face.x;
		faceInfo[i].y = face.y;
		faceInfo[i].width = face.width;
		faceInfo[i].height = face.height;
		faceInfo[i].yawAngle = face.yawAngle;
		faceInfo[i].rollAngle = face.rollAngle;
	}

	FBFDR_IMAGE_YUV420SP fbImage;
	fbImage.format = FBFDR_YUV420_FORMAT_CRCB;
	fbImage.width = width;
	fbImage.height = height;
	fbImage.yData = (unsigned char *)image;
	fbImage.uvData = fbImage.yData + fbImage.width * fbImage.height;

	FBFDR_FaceBeauty_YUV420SP(&fbImage, faceInfo, faceNum, 4);

	FdDeleteDetector(&fd_handle);

	return 0;
}

int sprd_portrait_capture_init_adpt(void **handle, PortraitCap_Init_Params *params)
{
	imgWidth=params->width;
	imgHeight=params->height;
	return sprd_portrait_capture_init(handle, params);
}

int sprd_portrait_capture_get_mask_info_adpt(void **handle, void *width, void *height, void *bufSize)
{
	return sprd_portrait_capture_get_mask_info(*handle, (unsigned int *)width, (unsigned int *)height, (unsigned int *)bufSize);
}

int sprd_portrait_capture_get_mask_adpt(void **handle, ProcDepthInputMap *depthInputData, PortaitCapProcParams *procParams, InoutYUV *yuvData, void *bokehMask, void *lptMask)
{
	int ret = 0;
	ADAPTER_LOGD("please do faceBeauty\n");
	ret = sprd_portrait_fb(yuvData->Src_YUV,imgWidth,imgHeight);
	ret |= sprd_portrait_capture_get_mask(*handle, depthInputData, procParams, yuvData, bokehMask, lptMask);
	return ret;
}

int sprd_portrait_capture_process_adpt(void **handle, InoutYUV *yuvData, void *WeightMask, int isCapture)
{
	return sprd_portrait_capture_process(*handle, yuvData, WeightMask, isCapture);
}

int sprd_portrait_capture_deinit_adpt(void **handle)
{
	return sprd_portrait_capture_deinit(*handle);
}

int sprd_capture_portrait_adpt(sprd_capture_portrait_cmd_t cmd, void *param)
{
	int ret = 0;
	if(param==NULL)
	{
		ADAPTER_LOGE("params is NULL\n");
		return -1;
	}

	switch (cmd) 
	{
		case SPRD_CAPTURE_PORTRAIT_INIT_CMD: 
			{
				sprd_capture_portrait_init_param_t *portrait_param=(sprd_capture_portrait_init_param_t *)param;
				char acVersion[256];
				ret=sprd_portrait_capture_get_version(acVersion,256);
				ADAPTER_LOGD("Bokeh Api Version [%s]", acVersion);				
				ret = sprd_portrait_capture_init_adpt(portrait_param->ctx, portrait_param->param);

				break;
			}
		case SPRD_CAPTURE_PORTRAIT_DEINIT_CMD: 
			{
				sprd_capture_portrait_deinit_param_t *portrait_param=(sprd_capture_portrait_deinit_param_t *)param;
				ret = sprd_portrait_capture_deinit_adpt(portrait_param->ctx);

				break;
			}
		case SPRD_CAPTURE_PORTRAIT_GET_MASK_INFO_CMD: 
			{
				sprd_capture_portrait_get_mask_info_param_t *portrait_param=(sprd_capture_portrait_get_mask_info_param_t *)param;
				ret = sprd_portrait_capture_get_mask_info_adpt(portrait_param->ctx, portrait_param->width, portrait_param->height, portrait_param->bufSize);

				break;
			}
		case SPRD_CAPTURE_PORTRAIT_GET_MASK_CMD: 
			{
				sprd_capture_portrait_get_mask_param_t *portrait_param=(sprd_capture_portrait_get_mask_param_t *)param;		
				ret = sprd_portrait_capture_get_mask_adpt(portrait_param->ctx, portrait_param->depthInputData, portrait_param->procParams, portrait_param->yuvData, portrait_param->bokehMask, portrait_param->lptMask);

				break;
			}	
		case SPRD_CAPTURE_PORTRAIT_PROCESS_CMD: 
			{
				sprd_capture_portrait_process_param_t *portrait_param=(sprd_capture_portrait_process_param_t *)param;		
				ret = sprd_portrait_capture_process_adpt(portrait_param->ctx, portrait_param->yuvData, portrait_param->bokehMask, portrait_param->isCapture);

				break;
			}				
		default:
			ADAPTER_LOGE("unknown cmd: %d\n", cmd);
	}

	return ret;
}
