#include "sprd_yuv_denoise_adapter.h"
#include "sprd_yuv_denoise_adapter_log.h"
#include "properties.h"
#include <string.h>

#ifdef DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

#ifdef CAMERA_CNR3_ENABLE
void *sprd_yuv_denoise_adpt_init(void *param)
{
    void *handle = 0;
    if(param==NULL)
    {
        DENOISE_LOGE("params is NULL\n");
        return NULL;
    }
	sprd_yuv_denoise_init_t *init_param=(sprd_yuv_denoise_init_t *)param;

	char strRunType[256];
	property_get("persist.vendor.cam.yuv_denoise.run_type", strRunType , "");
	if (!(strcmp("cpu", strRunType)))
		g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
	else if (!(strcmp("vdsp", strRunType)))
		g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
	DENOISE_LOGI("current run type: %d\n", g_run_type);

	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
	{
		handle=sprd_cnr_init(init_param->width,init_param->height,init_param->runversion);
	}
	else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
	{
		handle=sprd_cnr_init_vdsp(init_param->width,init_param->height,init_param->runversion);
	}
	else
	{
		DENOISE_LOGI("unknown type: %d\n",g_run_type);
	}

	return handle;
}

int sprd_yuv_denoise_adpt_deinit(void *handle)
{
	int ret = 0;
	if(handle==NULL)
	{
		DENOISE_LOGE("params is NULL\n");
		return -1;
	}

	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
	{
		ret = sprd_cnr_deinit(handle);
	}
	else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
	{
		ret = sprd_cnr_deinit_vdsp(handle);
	}
	else
	{
		DENOISE_LOGI("unknown type: %d\n",g_run_type);
	}

	return ret;
}

int sprd_yuv_denoise_adpt_ctrl(void *handle, sprd_yuv_denoise_cmd_t cmd, void *param)
{
	int ret = 0;
	if(handle==NULL||param==NULL)
	{
		DENOISE_LOGE("params is NULL\n");
		return -1;
	}

	sprd_yuv_denoise_param_t *denoise_param=(sprd_yuv_denoise_param_t *)param;
	Denoise_Param paramInfo;
	paramInfo.ynrParam=denoise_param->ynrParam;
	paramInfo.cnr2Param=denoise_param->cnr2Param;
	paramInfo.cnr3Param=denoise_param->cnr3Param;
	int width=denoise_param->width;
	int height=denoise_param->height;
	denoise_mode cmd_in = (denoise_mode)cmd;

	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) 
	{
		denoise_buffer imgBuffer;
		imgBuffer.bufferY=(unsigned char *)denoise_param->bufferY.addr[0];
		imgBuffer.bufferUV=(unsigned char *)denoise_param->bufferUV.addr[0];
		ret = sprd_cnr_process(handle, &imgBuffer, &paramInfo, cmd_in, width, height);
	}
	else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP) 
	{
		denoise_buffer_vdsp imgBuffer_vdsp;
		imgBuffer_vdsp.bufferY=(unsigned char *)denoise_param->bufferY.addr[0];
		imgBuffer_vdsp.bufferUV=(unsigned char *)denoise_param->bufferUV.addr[0];
		imgBuffer_vdsp.fd_Y=denoise_param->bufferY.ion_fd;
		imgBuffer_vdsp.fd_UV=denoise_param->bufferUV.ion_fd;
		ret = sprd_cnr_process_vdsp(handle, &imgBuffer_vdsp, &paramInfo, cmd_in, width, height);
	}
	else
	{
		DENOISE_LOGI("unknown run_type: %d\n", g_run_type);
	}

	return ret;
}

int sprd_yuv_denoise_get_devicetype(enum camalg_run_type *type)
{
	if (!type)
	return 1;

	*type = g_run_type;

	return 0;
}

int sprd_yuv_denoise_set_devicetype(enum camalg_run_type type)
{
	if (type < SPRD_CAMALG_RUN_TYPE_CPU || type >= SPRD_CAMALG_RUN_TYPE_MAX)
	return 1;
	g_run_type = type;

	return 0;
}

#else
void *sprd_yuv_denoise_adpt_init(void *param)
{
    void *handle = 0;
    if(param==NULL)
    {
        DENOISE_LOGE("params is NULL\n");
        return NULL;
    }
    sprd_yuv_denoise_init_t *init_param=(sprd_yuv_denoise_init_t *)param;

    char strRunType[256];
    property_get("persist.vendor.cam.yuv_denoise.run_type", strRunType , "");
    if (!(strcmp("cpu", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if (!(strcmp("vdsp", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    DENOISE_LOGI("current run type: %d\n", g_run_type);

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
    {
        handle=sprd_cnr_init(init_param->width,init_param->height,init_param->runversion);
    }
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
    {
        handle=sprd_cnr_init_vdsp(init_param->width,init_param->height,init_param->runversion);
    }
    else
    {
        DENOISE_LOGI("unknown type: %d\n",g_run_type);
    }
    return handle;
}

int sprd_yuv_denoise_adpt_deinit(void *handle)
{
    int ret = 0;
    if(handle==NULL)
    {
        DENOISE_LOGE("params is NULL\n");
        return -1;
    }

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
    {
        ret = sprd_cnr_deinit(handle);
    }
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
    {
        ret = sprd_cnr_deinit_vdsp(handle);
    }
    else
    {
        DENOISE_LOGI("unknown type: %d\n",g_run_type);
    }

    return ret;
}

int sprd_yuv_denoise_adpt_ctrl(void *handle, sprd_yuv_denoise_cmd_t cmd, void *param)
{
	int ret = 0;
	if(handle==NULL||param==NULL)
	{
		DENOISE_LOGE("params is NULL\n");
		return -1;
	}

	sprd_yuv_denoise_param_t *denoise_param=(sprd_yuv_denoise_param_t *)param;
	Denoise_Param paramInfo;
	paramInfo.ynrParam=denoise_param->ynrParam;
	paramInfo.cnr2Param=denoise_param->cnr2Param;
	paramInfo.cnr3Param=denoise_param->cnr3Param;
	int width=denoise_param->width;
	int height=denoise_param->height;

	DENOISE_LOGI("run_type = %d, cmd = %d", g_run_type, cmd);

	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) 
	{
        denoise_buffer imgBuffer;
        imgBuffer.bufferY=(unsigned char *)denoise_param->bufferY.addr[0];
        imgBuffer.bufferUV=(unsigned char *)denoise_param->bufferUV.addr[0];

        switch (cmd){
        case SPRD_YNR_PROCESS_CMD:
                ret = sprd_cnr_process(handle, &imgBuffer, &paramInfo, MODE_YNR, width, height);
                break;
         case SPRD_CNR_PROCESS_CMD:
                ret = sprd_cnr_process(handle, &imgBuffer, &paramInfo, MODE_CNR2, width, height);
                break;
         case SPRD_YNR_CNR_PROCESS_CMD:
                ret = sprd_cnr_process(handle, &imgBuffer, &paramInfo, MODE_YNR_CNR2, width, height);
                break;
         default:
                DENOISE_LOGI("unknown cmd: %d\n", cmd);
                break;
	}
    }
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP) 
    {
         denoise_buffer_vdsp imgBuffer_vdsp;
         imgBuffer_vdsp.bufferY=(unsigned char *)denoise_param->bufferY.addr[0];
         imgBuffer_vdsp.bufferUV=(unsigned char *)denoise_param->bufferUV.addr[0];
         imgBuffer_vdsp.fd_Y=denoise_param->bufferY.ion_fd;
         imgBuffer_vdsp.fd_UV=denoise_param->bufferUV.ion_fd;

         switch (cmd){
         case SPRD_YNR_PROCESS_CMD:
               ret = sprd_cnr_process_vdsp(handle, &imgBuffer_vdsp, &paramInfo, MODE_YNR, width, height);
               break;
         case SPRD_CNR_PROCESS_CMD:
               ret = sprd_cnr_process_vdsp(handle, &imgBuffer_vdsp, &paramInfo, MODE_CNR2, width, height);
               break;
         case SPRD_YNR_CNR_PROCESS_CMD:
               ret = sprd_cnr_process_vdsp(handle, &imgBuffer_vdsp, &paramInfo, MODE_YNR_CNR2, width, height);
               break;
         default:
               DENOISE_LOGI("unknown cmd: %d\n", cmd);
               break;
         }
    }
    else
    {
         DENOISE_LOGI("unknown run_type: %d\n", g_run_type);
    }

    return ret;
}

int sprd_yuv_denoise_get_devicetype(enum camalg_run_type *type)
{
	if (!type)
	return 1;

	*type = g_run_type;

	return 0;
}

int sprd_yuv_denoise_set_devicetype(enum camalg_run_type type)
{
	if (type < SPRD_CAMALG_RUN_TYPE_CPU || type >= SPRD_CAMALG_RUN_TYPE_MAX)
	return 1;
	g_run_type = type;

	return 0;
}

#endif


