#include "threednr_adapt_interface.h"
#include "threednr_adapt_log.h"
#include "properties.h"
#include <string.h>


#ifdef DEFAULT_RUNTYPE_VDSP
sprd_camalg_device_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
sprd_camalg_device_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif


int sprd_3dnr_adpt_init(void **handle, c3dnr_param_info_t *param, void *tune_params)
{
    int retval = 0;

    if (!handle || !param)
    {
        LOGI("3DNR ADAPT handle or param NULL! \n");
        //*handle = NULL;
        return 1;
    }

    char str_property[256];
    property_get("persist.vendor.cam.3dnr.run_type", str_property, "");
    if (!(strcmp("cpu", str_property)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if  (!(strcmp("vdsp", str_property)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    else if(!(strcmp("gpu", str_property)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_GPU;
    LOGI("3DNR ADAPT run type from property is %d \n", g_run_type);

    if (g_run_type != SPRD_CAMALG_RUN_TYPE_VDSP && g_run_type != SPRD_CAMALG_RUN_TYPE_CPU && g_run_type != SPRD_CAMALG_RUN_TYPE_GPU)
    {
        LOGI("3DNR ADAPT please set run type before init \n");
        *handle = NULL;
        return 2;//fail
    }

    //int  platform_id = PLATFORM_ID;
    int  platform_id = param->productInfo;
    if (platform_id == LOCAL_PLATFORM_ID_SHARKL3  || platform_id == LOCAL_PLATFORM_ID_SHARKL5)
		param->productInfo = 1;
    else
		param->productInfo = 0;
    LOGI("3DNR ADAPT PLATFORM_ID is %d  so set productInfo as %d  \n", platform_id,  param->productInfo);

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
    {
        retval = threednr_init(handle, param);

    }
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
    {
        //at present it's the same as SPRD_CAMALG_RUN_TYPE_CPU
        retval = threednr_init(handle, param);

    }
    return retval;
}

int sprd_3dnr_adpt_deinit(void** handle)
{
    int retval = 0;
    if (!handle)
    {
        LOGI("3DNR ADAPT handle is NULL! \n");
        return 1;
    }
    retval = threednr_deinit(handle);
    return retval;

}

int sprd_3dnr_adpt_ctrl(void *handle, sprd_3dnr_cmd_e cmd, void* param)
{
    int retval = 0;
    c3dnr_cmd_proc_t *cmdptr;
    c3dnr_capture_cmd_param_t *cap_param_ptr;
    c3dnr_capnew_cmd_param_t  *cap_new_param_ptr;
    c3dnr_capture_preview_cmd_param_t *pre_param_ptr;
    c3dnr_setparam_cmd_param_t *setpara_param_ptr;
    c3dnr_pre_inparam_t pre_inparam;

    LOGI("3DNR ADAPT cmd=%d",cmd);
    if (!handle || cmd >= SPRD_3DNR_PROC_MAX_CMD)
    {
        LOGI("3DNR ADAPT handle or cmd is wrong! cmd = %d\n",cmd);
        return 1;
    }
    if (!param && cmd != SPRD_3DNR_PROC_STOP_CMD)
    {
        LOGI("3DNR ADAPT param is NULL! \n");
        return 2;
    }
    if (g_run_type != SPRD_CAMALG_RUN_TYPE_VDSP && g_run_type != SPRD_CAMALG_RUN_TYPE_CPU && g_run_type != SPRD_CAMALG_RUN_TYPE_GPU)
    {
        LOGI("3DNR ADAPT please set run type before init \n");
        return 3;//fail
    }

    cmdptr = (c3dnr_cmd_proc_t *)param;

    if (cmd == SPRD_3DNR_PROC_CAPTURE_CMD)
    {
        if (cmdptr->callWay != 0 && cmdptr->callWay != 1)
        {
            LOGI("3DNR ADAPT wrong callWay \n");
            return 4;//fail
        }
    }
    switch(cmd)
    {
        case SPRD_3DNR_PROC_CAPTURE_CMD:
            if (!cmdptr->callWay)
            {
                cap_param_ptr = (c3dnr_capture_cmd_param_t *)&cmdptr->proc_param.cap_param;
                retval = threednr_function(handle, cap_param_ptr->small_image, cap_param_ptr->orig_image);
            }
            else
            {
                cap_new_param_ptr = (c3dnr_capnew_cmd_param_t  *)&cmdptr->proc_param.cap_new_param;
                retval = threednr_function_new(handle, cap_new_param_ptr->small_image, cap_new_param_ptr->orig_image);
            }
            break;
        case SPRD_3DNR_PROC_PREVIEW_CMD:
        case SPRD_3DNR_PROC_VIDEO_CMD:
            pre_param_ptr = (c3dnr_capture_preview_cmd_param_t *)&cmdptr->proc_param.pre_param;
            pre_inparam.gain = pre_param_ptr->gain;
            retval = threednr_function_pre(handle, pre_param_ptr->small_image, pre_param_ptr->orig_image, pre_param_ptr->out_image, &pre_inparam);
            break;
        case SPRD_3DNR_PROC_SET_PARAMS_CMD:
            setpara_param_ptr = (c3dnr_setparam_cmd_param_t *)&cmdptr->proc_param.setpara_param;
            retval = threednr_setparams(handle, setpara_param_ptr->thr, setpara_param_ptr->slp);
            break;
        case SPRD_3DNR_PROC_STOP_CMD:
            retval = threednr_setstop_flag(handle);
            break;
        default:
            retval = 5;
            break;
    }
    return retval;
}

int sprd_3dnr_get_devicetype(sprd_camalg_device_type *type)
{
    *type = g_run_type;
    return 0;
}

int sprd_3dnr_set_devicetype(sprd_camalg_device_type type)
{
   if (type != SPRD_CAMALG_RUN_TYPE_VDSP && type != SPRD_CAMALG_RUN_TYPE_CPU && type != SPRD_CAMALG_RUN_TYPE_GPU)
   {
        LOGI("3DNR ADAPT set wrong type %d \n", type);
        return 1;//fail
   }

   g_run_type = type;

   LOGI("3DNR ADAPT set device type as %d by Adapter layer \n", g_run_type);
   return 0;//success;
}
