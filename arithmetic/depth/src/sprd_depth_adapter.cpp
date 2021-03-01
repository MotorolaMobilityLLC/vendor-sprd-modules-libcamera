#include "sprd_depth_adapter.h"
#include "sprd_depth_adapter_log.h"
#include "properties.h"
#include <string.h>


#ifdef DEFAULT_RUNTYPE_NPU
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_NPU;
#elif defined DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

void *sprd_depth_adpt_init(void *param)
{
    char strRunType[256];
    property_get("persist.vendor.cam.depth.run_type", strRunType , "");
    if (!(strcmp("cpu", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if (!(strcmp("vdsp", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    else if (!(strcmp("npu", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_NPU;

    DEPTH_LOGI("depth current run type: %d\n", g_run_type);

    void *handle = NULL;
    sprd_depth_init_param_t *init_param=(sprd_depth_init_param_t *)param;
    depth_init_inputparam input_param;
    depth_init_outputparam output_info;
    depth_mode mode;
    outFormat  outformat;
    
    mode = MODE_CAPTURE;
    outformat = (outFormat)init_param->format;
    input_param.input_width_main = init_param->inparam.input_width_main;
    input_param.input_height_main = init_param->inparam.input_height_main;
    input_param.input_width_sub = init_param->inparam.input_width_sub;
    input_param.input_height_sub = init_param->inparam.input_height_sub;
    if(outformat==MODE_DISPARITY)
    {
        input_param.output_depthwidth = 800;
        input_param.output_depthheight = 600;
    }else
    {
        input_param.output_depthwidth = 400;
        input_param.output_depthheight = 300;        
    }
    input_param.online_depthwidth = input_param.output_depthwidth;
    input_param.online_depthheight = input_param.output_depthheight;
    input_param.depth_threadNum = 1;
    input_param.online_threadNum = 2;
    input_param.imageFormat_main = YUV420_NV21;
    input_param.imageFormat_sub = YUV420_NV21;
    input_param.potpbuf = init_param->inparam.otpbuf;
    input_param.otpsize = init_param->inparam.otpsize;
    input_param.config_param = NULL;
    
    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
        input_param.run_type=0;
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_NPU)
        input_param.run_type=1;
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
        input_param.run_type=2;

    handle=sprd_depth_Init(&input_param , &output_info,mode,outformat);
    
    init_param->outputinfo.outputsize=output_info.outputsize;
    init_param->outputinfo.output_width=input_param.output_depthwidth;
    init_param->outputinfo.output_height=input_param.output_depthheight;
    return handle;
}

int sprd_depth_adpt_deinit(void *handle)
{
    int ret = 0;
    ret = sprd_depth_Close(handle);
    return ret;
}

int sprd_depth_adpt_ctrl(void *handle, sprd_depth_cmd_t cmd, void *param)
{
    int ret = 0;
    DEPTH_LOGI("CMD value=%d\n",cmd);
    switch (cmd) {
    case SPRD_DEPTH_GET_VERSION_CMD: {
        char acVersion[256] = {0};
        ret = sprd_depth_VersionInfo_Get(acVersion, 256);
        DEPTH_LOGI("depth api version [%s]", acVersion);
    break;
    }
    case SPRD_DEPTH_RUN_CMD: {
        sprd_depth_run_param_t *depth_param = (sprd_depth_run_param_t *)param;
        void * pInMain;
        void * pInSub;
        void * output;
        updateotp_param otp_params;
        depth_param->ret_otp=0;

        weightmap_param weightParams;
        weightParams.F_number = depth_param->params.F_number;
        weightParams.sel_x = depth_param->params.sel_x;
        weightParams.sel_y = depth_param->params.sel_y;
        weightParams.DisparityImage = NULL;
        weightParams.VCM_cur_value = depth_param->params.VCM_cur_value;
        weightParams.golden_vcm_data=depth_param->params.tuning_golden_vcm;
        weightParams.portrait_param=depth_param->params.portrait_param;       
        DEPTH_LOGD("F_number %d sel_coordinate (%d,%d) VCM_CUR:%d\n",
            weightParams.F_number, weightParams.sel_x, weightParams.sel_y,
            weightParams.VCM_cur_value);
    
        if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP) {           
            pInMain=depth_param->input[0].addr[0];
            pInSub=depth_param->input[1].addr[0];
            output=depth_param->output.addr[0];
        }
        else {
            pInMain = depth_param->input[0].addr[0];
            pInSub = depth_param->input[1].addr[0];
            output = depth_param->output.addr[0];
        }
        if(depth_param->mChangeSensor)
        {
            updateotp_param otp_params;
            otp_params.mChangeSensor=depth_param->mChangeSensor;
            otp_params.input_otpsize=depth_param->input_otpsize;
            otp_params.output_otpsize=depth_param->output_otpsize=272; 
            DEPTH_LOGD("mChangeSensor %d input_otpsize %d output_otpsize %d ret_otp %d\n",otp_params.mChangeSensor, otp_params.input_otpsize, otp_params.output_otpsize,depth_param->ret_otp);
            depth_param->ret_otp = sprd_depth_updateotp(handle,pInSub,pInMain,depth_param->input_otpbuf,depth_param->output_otpbuf,&otp_params);
            DEPTH_LOGD("ret_otp:%d\n",depth_param->ret_otp);
        }        
        ret = sprd_depth_Run(handle , output,NULL,pInSub,pInMain,&weightParams);

        break;
    }
    case SPRD_DEPTH_FAST_STOP_CMD: {

        sprd_depth_Set_Stopflag(handle,DEPTH_STOP);
        
        break;
    }
    case SPRD_DEPTH_GDEPTH_CMD: {

        gdepth_outparam gdepth_output;
        sprd_depth_gdepth_param_t *depth_param = (sprd_depth_gdepth_param_t *)param;
        void * indepth=depth_param->input.addr[0];
        gdepth_output.confidence_map=(unsigned char *)depth_param->gdepth_output.output[0].addr[0];
        gdepth_output.depthnorm_data=(unsigned char *)depth_param->gdepth_output.output[1].addr[0];
        
        ret = sprd_depth_get_gdepthinfo(handle,indepth, &gdepth_output);
        
        depth_param->gdepth_output.near=gdepth_output.near;
        depth_param->gdepth_output.far=gdepth_output.far;
        
        break;
    }
    case SPRD_DEPTH_USERSET_CMD: {

        sprd_depth_userset_param_t *depth_param = (sprd_depth_userset_param_t *)param;
        char *ptr=depth_param->ptr;
        int size=depth_param->size;
        ret = sprd_depth_userset(ptr,size);
        
    break;
    }
        case SPRD_DEPTH_ONLINECALI_CMD: {

        sprd_depth_onlineCali_param_t *depth_param = (sprd_depth_onlineCali_param_t *)param;

        void * pInMain = depth_param->input[0].addr[0];
        void * pInSub = depth_param->input[1].addr[0];
        void * a_pOutMaptable = depth_param->output.addr[0];
        
        ret = sprd_depth_OnlineCalibration(handle , a_pOutMaptable, pInSub, pInMain);		
        
    break;
    }
        case SPRD_DEPTH_ONLINECALI_POST_CMD: {

        sprd_depth_onlinepost_param_t *depth_param = (sprd_depth_onlinepost_param_t *)param;

        void * inMaptable = depth_param->input.addr[0];
        void * a_pOutMaptable_scale = depth_param->output.addr[0];
        
        ret = sprd_depth_OnlineCalibration_postprocess(handle , inMaptable, a_pOutMaptable_scale);		
        
        break;
    }
    default:
        DEPTH_LOGW("unknown depth cmd: %d\n", cmd);
        break;
    }
    
    return ret;
}

int sprd_depth_get_devicetype(enum camalg_run_type *type)
{
    if (!type)
        return 1;

    *type = g_run_type;

    return 0;
}

int sprd_depth_set_devicetype(enum camalg_run_type type)
{
    if (type < SPRD_CAMALG_RUN_TYPE_CPU || type >= SPRD_CAMALG_RUN_TYPE_MAX)
        return 1;
    g_run_type = type;
    
    return 0;
}
