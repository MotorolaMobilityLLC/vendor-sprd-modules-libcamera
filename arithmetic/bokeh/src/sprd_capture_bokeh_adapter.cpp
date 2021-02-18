#include "sprd_capture_bokeh_adapter.h"
#include "sprdfacebeautyfdr.h"
#include "sprdfdapi.h"
#include <utils/Log.h>
#include <string.h>

#define LOG_TAG "sprd_capture_bokeh_adapter"
#define ADAPTER_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define ADAPTER_LOGD(format,...) ALOGD(format, ##__VA_ARGS__)

static int imgWidth=0;
static int imgHeight=0;
int sprd_bokeh_fb(void *image, int width, int height)
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

int sprd_bokeh_init_adpt(void **handle, int imgW, int imgH, init_param *param)
{
	imgWidth=imgW;
	imgHeight=imgH;
	param->memOps=NULL;       
	return sprd_bokeh_Init(handle, imgW, imgH, param);
}

int sprd_bokeh_userset_adpt(void *ptr,int size)
{
	return sprd_bokeh_userset((char *)ptr, size);
}

int sprd_bokeh_GdepthToDepth16_adpt(void **handle, gdepth_param *depthIn, void *depthOut,int depthW ,int depthH)
{
	return sprd_bokeh_GdepthToDepth16(*handle,depthIn,depthOut,depthW,depthH);
}

int sprd_bokeh_ReFocusPreProcess_adpt(void **handle, void *srcImg, void *depthIn, int depthW, int depthH, void *param)
{
	int ret = 0;
	ADAPTER_LOGD("please do faceBeauty\n");
	ret = sprd_bokeh_fb(srcImg,imgWidth,imgHeight);
	ret |= sprd_bokeh_ReFocusPreProcess(*handle,srcImg,depthIn,depthW,depthH,param);
	return ret;
}

int sprd_bokeh_ReFocusGen_adpt(void **handle, void *dstImg, int F_number, int sel_x, int sel_y)
{
	return sprd_bokeh_ReFocusGen(*handle,dstImg,F_number,sel_x,sel_y);
}

int sprd_bokeh_ReFocusGen_Portrait_adpt(void **handle, void *dstImg, int F_number, int sel_x, int sel_y)
{
	return sprd_bokeh_ReFocusGen_Portrait(*handle,dstImg,F_number,sel_x,sel_y);
}

int sprd_bokeh_deinit_adpt(void **handle)
{
	return sprd_bokeh_Close(*handle);
}

int sprd_bokeh_ctrl_adpt(sprd_capture_bokeh_cmd_t cmd, void *param)
{
	int ret = 0;
	if(param==NULL)
	{
		ADAPTER_LOGE("params is NULL\n");
		return -1;
	}

	switch (cmd) 
	{
		case SPRD_CAPTURE_BOKEH_INIT_CMD: 
			{
				sprd_capture_bokeh_init_param_t *bokeh_param=(sprd_capture_bokeh_init_param_t *)param;
				char acVersion[256];
				ret=sprd_bokeh_VersionInfo_Get((void *)acVersion,256);
				ADAPTER_LOGD("Bokeh Api Version [%s]", acVersion);
				ret=sprd_bokeh_init_adpt(bokeh_param->ctx,bokeh_param->imgW,bokeh_param->imgH,bokeh_param->param);

				break;
			}
		case SPRD_CAPTURE_BOKEH_DEINIT_CMD: 
			{
				sprd_capture_bokeh_deinit_param_t *bokeh_param=(sprd_capture_bokeh_deinit_param_t *)param;
				ret=sprd_bokeh_deinit_adpt(bokeh_param->ctx);

				break;
			}
		case SPRD_CAPTURE_BOKEH_USERSET_CMD: 
			{
				sprd_capture_bokeh_info_param_t *bokeh_param=(sprd_capture_bokeh_info_param_t *)param;
				ret=sprd_bokeh_userset_adpt(bokeh_param->ptr,bokeh_param->size);

				break;
			}
		case SPRD_CAPTURE_BOKEH_GDEPTH_CMD: 
			{
				sprd_capture_bokeh_gdepth_param_t *bokeh_param=(sprd_capture_bokeh_gdepth_param_t *)param;
				ret=sprd_bokeh_GdepthToDepth16_adpt(bokeh_param->ctx,bokeh_param->param,(void *)bokeh_param->depthOut,bokeh_param->depthW,bokeh_param->depthH);

				break;
			}
		case SPRD_CAPTURE_BOKEH_REFOCUSPRE_CMD: 
			{
				sprd_capture_bokeh_pre_param_t *bokeh_param=(sprd_capture_bokeh_pre_param_t *)param;
				ret = sprd_bokeh_ReFocusPreProcess_adpt(bokeh_param->ctx,bokeh_param->srcImg,bokeh_param->depthIn,bokeh_param->depthW,bokeh_param->depthH,bokeh_param->params);

				break;
			}
		case SPRD_CAPTURE_BOKEH_REFOCUSGEN_CMD: 
			{
				sprd_capture_bokeh_gen_param_t *bokeh_param=(sprd_capture_bokeh_gen_param_t *)param;
				ret = sprd_bokeh_ReFocusGen_adpt(bokeh_param->ctx,bokeh_param->dstImg,bokeh_param->F_number,bokeh_param->sel_x,bokeh_param->sel_y);

				break;
			}
		case SPRD_CAPTURE_BOKEH_REFOCUSGEN_PORTRAIT_CMD: 
			{
				sprd_capture_bokeh_gen_param_t *bokeh_param=(sprd_capture_bokeh_gen_param_t *)param;
				ret = sprd_bokeh_ReFocusGen_Portrait_adpt(bokeh_param->ctx,bokeh_param->dstImg,bokeh_param->F_number,bokeh_param->sel_x,bokeh_param->sel_y);

				break;
			}									
		default:
			ADAPTER_LOGE("unknown cmd: %d\n", cmd);
			break;
	}

	return ret;
}
