#include "sprd_portrait_scene_adapter.h"
#include "sprd_portrait_scene_adapter_log.h"
#include <cutils/properties.h>
#include <string.h>

static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;

int sprd_portrait_scene_adpt_deinit(void *handle, sprd_portrait_scene_channel_t ch){
    int rc = -1;
    if (ch == SPRD_PORTRAIT_SCENE_PREVIEW) {
		rc = unisoc_portrait_scene_preview_deinit(handle);
        PScene_LOGI("preview deinit rc%d", rc);
        }
    else if (ch == SPRD_PORTRAIT_SCENE_CAPTURE) {
        rc = unisoc_portrait_scene_capture_deinit(handle);
        PScene_LOGI("capture deinit rc%d", rc);
        }

    return rc;
}

void *sprd_portrait_scene_adpt_init(sprd_portrait_scene_init_t *_param){
    int rc=0;
    char prop_value[256];
    if(!_param){
        PScene_LOGE("Failed to get init param");
        return NULL;
    }
/*
    _param->min_slope=0.005;
    _param->max_slope=0.05;
    _param->Findex2Gamma_AdjustRatio=6.0;


    _param->box_filter_size=3;
    _param->slope=128;
    _param->gau_min_slope=0.0001;
    _param->gau_max_slope=0.005;
    _param->gau_Findex2Gamma_AdjustRatio=2.0;
    _param->gau_box_filter_size=11;
*/
    _param->Scalingratio=4;
    _param->SmoothWinSize=5;

    property_get("persist.vendor.cam.PScene.run_type", prop_value, "");
    if(!(strcmp("cpu",prop_value)))
       g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if(!(strcmp("vdsp", prop_value)))
       g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
 
    PScene_LOGI("current run type:%d", g_run_type);

    if(_param->handle){
        rc=sprd_portrait_scene_adpt_deinit(_param->handle, _param->ch);
        _param->handle=NULL;
        if(rc){
            PScene_LOGE("Failed to deinit handle");
            return NULL;
        }
    }
    if (_param->ch == SPRD_PORTRAIT_SCENE_PREVIEW) {
		rc=unisoc_portrait_scene_preview_init(&_param->handle, _param);
         PScene_LOGI("preview init rc%d", rc);
        }
    else if (_param->ch == SPRD_PORTRAIT_SCENE_CAPTURE) {
        rc=unisoc_portrait_scene_capture_init(&_param->handle, _param);
         PScene_LOGI("capture init rc%d", rc);
        }
    else {
		PScene_LOGE("unknown ch %d", _param->ch);
		rc = -1;
    }
    if(rc)
        return NULL;
    else
        return _param->handle;
}

int sprd_portrait_scene_adpt_ctrl(void *handle, sprd_portrait_scene_cmd_t cmd, void *param){
    int rc = -1;

    if(!handle || !param){
        PScene_LOGE("Failed to get param,handle =%p,param=%p",handle,param);
        return rc;
    }
    PScene_LOGV("cmd=%d",cmd);
    switch (cmd){
	    case SPRD_PORTRAIT_SCENE_WEIGHT_CMD:{
	        sprd_portrait_scene_proc_t *_param=(sprd_portrait_scene_proc_t *)param;
	        if(_param->ch == SPRD_PORTRAIT_SCENE_PREVIEW){
	            rc= unisoc_portrait_scene_preview_create_weight_map(handle,_param);
					PScene_LOGI("preview create weightmap rc%d", rc);
					return rc;
	        }else if(_param->ch==SPRD_PORTRAIT_SCENE_CAPTURE){
				rc = unisoc_portrait_scene_capture_create_weight_map(handle,_param);
				PScene_LOGI("capture create weightmap rc%d", rc);
				return rc;
	        }else{
                PScene_LOGE("Failed to get right channel,ch=%d",_param->ch);
            }
	    } break;
	    case SPRD_PORTRAIT_SCENE_PROCESS_CMD:{
	        sprd_portrait_scene_adapter_mask_t *_param=(sprd_portrait_scene_adapter_mask_t *)param;
	        if(_param->ch == SPRD_PORTRAIT_SCENE_PREVIEW){
				rc = unisoc_portrait_scene_preview_process(handle, _param->src_YUV,  _param->dst_YUV, _param->mask);
				PScene_LOGI("preview process rc%d", rc);
				return rc;
	        }else if(_param->ch == SPRD_PORTRAIT_SCENE_CAPTURE){
                PScene_LOGI("src_yuv=%p,dst_yuv=%p",_param->src_YUV,_param->dst_YUV);
				rc= unisoc_portrait_scene_capture_process(handle, _param->src_YUV,  _param->dst_YUV,_param->mask);
				PScene_LOGI("capture process rc%d", rc);
				return rc;
            }else{
                PScene_LOGE("Failed to get right channel,ch=%d",_param->ch);
            }
        } break;
	    case SPRD_PORTRAIT_SCENE_FUSE_CMD:{
	        sprd_portrait_scene_adapter_fuse_t *_param=(sprd_portrait_scene_adapter_fuse_t *)param;
	        if(_param->fuse.ch == SPRD_PORTRAIT_SCENE_PREVIEW){
				rc= unisoc_portrait_scene_preview_fuse_img(handle,
					_param->src_YUV, _param->dst_YUV, &_param->fuse);
				PScene_LOGI("preview fuse rc%d", rc);
				return rc;
	        }else if(_param->fuse.ch ==SPRD_PORTRAIT_SCENE_CAPTURE){
				rc = unisoc_portrait_scene_capture_fuse_img(handle,
					_param->src_YUV, _param->dst_YUV, &_param->fuse);
				PScene_LOGI("capture fuse rc%d", rc);
				return rc;
	    	}else{
                PScene_LOGE("Failed to get right channel,ch=%d",_param->fuse.ch);
            }
        } break;
	    default:
	        PScene_LOGE("Failed to get right CMD");
	        return 1;
    	}
    return 0;
}

int sprd_portrait_scene_get_devicetype(enum camalg_run_type *type){

    *type = g_run_type;

    return 0;
}

