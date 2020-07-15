#include "mfnr_adapt_interface.h"
#include "mfnr_adapt_log.h"
#include "properties.h"
#include <string.h>
#include <cutils/trace.h>

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)


#ifdef DEFAULT_RUNTYPE_VDSP
sprd_camalg_device_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
sprd_camalg_device_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif


int sprd_mfnr_adpt_init(void **handle, mfnr_param_info_t *param, void *tune_params)
{
    int retval = 0;
    LOGI("MFNR ADAPT INIT run  handle=%x \n", *handle);
    if (!handle || !param)
    {
        LOGI("MFNR ADAPT INIT handle or param NULL! \n");
        //*handle = NULL;
        return 1;
    }

    char str_property[256];
    property_get("persist.vendor.cam.mfnr.run_type", str_property, "");
    if (!(strcmp("cpu", str_property)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if  (!(strcmp("vdsp", str_property)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    else if(!(strcmp("gpu", str_property)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_GPU;
    LOGI("MFNR ADAPT INIT run type from property is %d \n", g_run_type);

    if (g_run_type != SPRD_CAMALG_RUN_TYPE_VDSP && g_run_type != SPRD_CAMALG_RUN_TYPE_CPU && g_run_type != SPRD_CAMALG_RUN_TYPE_GPU)
    {
        LOGI("MFNR ADAPT INIT please set run type before init \n");
        *handle = NULL;
        return 2;//fail
    }

    //int  platform_id = PLATFORM_ID;
    int  platform_id = param->productInfo;
    if (platform_id == LOCAL_PLATFORM_ID_SHARKL3  || platform_id == LOCAL_PLATFORM_ID_SHARKL5)
		param->productInfo = 1;
    else
		param->productInfo = 0;
    LOGI("MFNR ADAPT INIT PLATFORM_ID is %d  so set productInfo as %d  \n", platform_id,  param->productInfo);

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
    {
		LOGI("MFNR ADAPT INIT call  mfnr_init  \n");
        retval = mfnr_init(handle, param);

    }
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
    {
        //vdsp init
		LOGI("MFNR ADAPT INIT call  mfnr_init_vdsp  \n");
        retval = mfnr_init_vdsp(handle, param);

    }
	LOGI("MFNR ADAPT INIT finish  retval=%d handle=%x \n",retval, *handle);
    return retval;
}

int sprd_mfnr_adpt_deinit(void** handle)
{
	LOGI("MFNR ADAPT DEINIT run handle=%x \n", *handle);
    int retval = 0;
    if (!handle)
    {
        LOGI("MFNR ADAPT DEINIT handle is NULL! \n");
        return 1;
    }
	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
    {
		LOGI("MFNR ADAPT DEINIT call  mfnr_deinit  \n");
        retval = mfnr_deinit(handle);

    }
    else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
    {
        //vdsp deinit
		LOGI("MFNR ADAPT DEINIT call  mfnr_deinit_vdsp  \n");
        retval = mfnr_deinit_vdsp(handle);

    } 
    
	LOGI("MFNR ADAPT DEINIT finish  retval=%d \n",retval);
    return retval;

}

int sprd_mfnr_adpt_ctrl(void *handle, sprd_mfnr_cmd_e cmd, void* param)
{
    int retval = 0;
    mfnr_cmd_proc_t *cmdptr;
    mfnr_capture_cmd_param_t *cap_param_ptr;
    mfnr_capnew_cmd_param_t  *cap_new_param_ptr;
    mfnr_capture_preview_cmd_param_t *pre_param_ptr;
    mfnr_setparam_cmd_param_t *setpara_param_ptr;
    mfnr_pre_inparam_t pre_inparam;

    LOGI("MFNR ADAPT CTRL run cmd=%d handle=%x \n",cmd, handle);
    if (!handle || cmd >= SPRD_MFNR_PROC_MAX_CMD)
    {
        LOGI("MFNR ADAPT CTRL handle or cmd is wrong! cmd = %d\n",cmd);
        return 1;
    }
    if (!param && cmd != SPRD_MFNR_PROC_STOP_CMD)
    {
        LOGI("MFNR ADAPT CTRL param is NULL! \n");
        return 2;
    }
    if (g_run_type != SPRD_CAMALG_RUN_TYPE_VDSP && g_run_type != SPRD_CAMALG_RUN_TYPE_CPU && g_run_type != SPRD_CAMALG_RUN_TYPE_GPU)
    {
        LOGI("MFNR ADAPT CTRL please set run type before init \n");
        return 3;//fail
    }

    cmdptr = (mfnr_cmd_proc_t *)param;

    if (cmd == SPRD_MFNR_PROC_CAPTURE_CMD)
    {
        if (cmdptr->callWay != 0 && cmdptr->callWay != 1)
        {
            LOGI("MFNR ADAPT CTRL wrong callWay \n");
            return 4;//fail
        }
    }
    switch(cmd)
    {
        case SPRD_MFNR_PROC_CAPTURE_CMD:
		    LOGI("MFNR ADAPT CTRL cmdptr->callWay = %d \n", cmdptr->callWay);
		    if (!cmdptr->callWay)
            {
				cap_param_ptr = (mfnr_capture_cmd_param_t *)&cmdptr->proc_param.cap_param;
				if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
				{
                    retval = mfnr_function(handle, cap_param_ptr->small_image, cap_param_ptr->orig_image);
				}
				else if(g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
				{
					LOGI("MFNR ADAPT CTRL CAPTURE small_fd = %x  small_bufferY = %x \n", cap_param_ptr->small_image->cpu_buffer.fd, cap_param_ptr->small_image->cpu_buffer.bufferY);
					LOGI("MFNR ADAPT CTRL CAPTURE orig_fd = %x  orig_bufferY = %x \n", cap_param_ptr->orig_image->cpu_buffer.fd, cap_param_ptr->orig_image->cpu_buffer.bufferY);
					retval = mfnr_function_vdsp(handle, cap_param_ptr->small_image, cap_param_ptr->orig_image);
				}
            }
            else
            {
                cap_new_param_ptr = (mfnr_capnew_cmd_param_t  *)&cmdptr->proc_param.cap_new_param;
				if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
				{
				    retval = mfnr_function_new(handle, cap_new_param_ptr->small_image, cap_new_param_ptr->orig_image);
				}
				else if(g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
				{
					LOGI("MFNR ADAPT CTRL error VDSP capture  callWay must == 0 \n");
					retval = 6;
				}
            } 
            break;
        case SPRD_MFNR_PROC_PREVIEW_CMD:
        case SPRD_MFNR_PROC_VIDEO_CMD:
            pre_param_ptr = (mfnr_capture_preview_cmd_param_t *)&cmdptr->proc_param.pre_param;
            pre_inparam.gain = pre_param_ptr->gain;
			LOGI("MFNR ADAPT CTRL pre_param_ptr->gain = %d \n", pre_param_ptr->gain);
			if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
			{
                ATRACE_BEGIN("mfnr_function_pre");
                retval = mfnr_function_pre(handle, pre_param_ptr->small_image, pre_param_ptr->orig_image, pre_param_ptr->out_image, &pre_inparam);
                ATRACE_END();
			}
			else if(g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
			{
				LOGI("MFNR ADAPT CTRL PREVIEW small_fd = %x  small_bufferY = %x \n", pre_param_ptr->small_image->cpu_buffer.fd, pre_param_ptr->small_image->cpu_buffer.bufferY);
				LOGI("MFNR ADAPT CTRL PREVIEW orig_fd = %x  orig_bufferY = %x \n", pre_param_ptr->orig_image->cpu_buffer.fd, pre_param_ptr->orig_image->cpu_buffer.bufferY);
				retval = mfnr_function_pre_vdsp(handle, pre_param_ptr->small_image, pre_param_ptr->orig_image, pre_param_ptr->out_image, pre_inparam.gain);
			}
            break;
        case SPRD_MFNR_PROC_SET_PARAMS_CMD:
            setpara_param_ptr = (mfnr_setparam_cmd_param_t *)&cmdptr->proc_param.setpara_param;
			if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
			{
                retval = mfnr_setparams(handle, setpara_param_ptr->thr, setpara_param_ptr->slp);
			}
			else if(g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
			{
				retval = mfnr_setparams_vdsp(handle, setpara_param_ptr->thr, setpara_param_ptr->slp);
			}
            break;
        case SPRD_MFNR_PROC_STOP_CMD:
		    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU ||g_run_type == SPRD_CAMALG_RUN_TYPE_GPU)
			{
                retval = mfnr_setstop_flag(handle);
			}
			else if(g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
			{
				retval = mfnr_setstop_flag_vdsp(handle);
			}
            break;
        default:
            retval = 5;
            break;
    }
	LOGI("MFNR ADAPT CTRL finish retval=%d \n",retval);
    return retval;
}

int sprd_mfnr_get_devicetype(sprd_camalg_device_type *type)
{
    *type = g_run_type;
    return 0;
}

int sprd_mfnr_set_devicetype(sprd_camalg_device_type type)
{
   if (type != SPRD_CAMALG_RUN_TYPE_VDSP && type != SPRD_CAMALG_RUN_TYPE_CPU && type != SPRD_CAMALG_RUN_TYPE_GPU)
   {
        LOGI("MFNR ADAPT set wrong type %d \n", type);
        return 1;//fail
   }

   g_run_type = type;

   LOGI("MFNR ADAPT SET DEVICE TYPE as %d by Adapter layer \n", g_run_type);
   return 0;//success;
}
