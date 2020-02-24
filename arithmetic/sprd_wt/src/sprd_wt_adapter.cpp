#include "sprd_wt_adapter.h"
#include "sprd_wt_adapter_log.h"
#include <cutils/properties.h>
#include <string.h>
#include "WT_interface.h"


#ifdef DEFAULT_RUNTYPE_GPU
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_GPU;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

void *sprd_wt_adpt_init(void *param)
{
    char strRunType[256];
    property_get("persist.vendor.cam.wt.run_type", strRunType , "");
    if (!(strcmp("cpu", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if (!(strcmp("vdsp", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    else if (!(strcmp("gpu", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_GPU;

    WT_LOGI("wt current run type: %d\n", g_run_type);

    void *handle = NULL;
    sprd_wt_init_param_t *init_param=(sprd_wt_init_param_t *)param;
    WT_inparam input_param;

    input_param.yuv_mode = 1;
    input_param.is_preview = 0;
    input_param.image_width_wide = init_param->image_width_wide;
    input_param.image_height_wide = init_param->image_height_wide;
    input_param.image_width_tele = init_param->image_width_tele;
    input_param.image_height_tele = init_param->image_height_tele;
    input_param.otpbuf = init_param->otpbuf;
    input_param.otpsize = init_param->otpsize;
    input_param.VCMup = init_param->VCMup;
    input_param.VCMdown = init_param->VCMdown;

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
    {
       WTprocess_Init(&handle,&input_param);
    }
	
    return handle;
}

int sprd_wt_adpt_deinit(void *handle)
{
    int ret = 0;

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
    {
	ret = WTprocess_deinit(handle);
    }
    return ret;
}

int sprd_wt_adpt_ctrl(void *handle, sprd_wt_cmd_t cmd, void *param)
{
    int ret = 0;
    switch (cmd) {
    case SPRD_WT_RUN_CMD: {
	    sprd_wt_run_param_t *wt_param = (sprd_wt_run_param_t *)param;

        int VCM_cur_value=wt_param->params.VCM_cur_value;
        float ScaleRatio=wt_param->params.ScaleRatio;
        WT_LOGD("VCM_cur_value=%d ScaleRatio=%f\n",
            VCM_cur_value,ScaleRatio);
		
        if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) 
	{
	    void * input_image_wide = wt_param->input[0].addr[0];
	    void * input_image_tele = wt_param->input[1].addr[0];
	    void * output_image = wt_param->output.addr[0];

	    ret = WTprocess_function(handle, input_image_wide, input_image_tele,output_image,VCM_cur_value,ScaleRatio);
	
        }
        break; 
    }
    default:
	WT_LOGW("unknown wt cmd: %d\n", cmd);
	break;
    }
	
    return ret;
}

int sprd_wt_get_devicetype(enum camalg_run_type *type)
{
    if (!type)
        return 1;

    *type = g_run_type;

    return 0;
}

int sprd_wt_set_devicetype(enum camalg_run_type type)
{
    if (type < SPRD_CAMALG_RUN_TYPE_CPU || type >= SPRD_CAMALG_RUN_TYPE_MAX)
        return 1;
    g_run_type = type;
	
    return 0;
}
