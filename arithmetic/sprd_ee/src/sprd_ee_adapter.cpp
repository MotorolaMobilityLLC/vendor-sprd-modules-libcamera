#include "sprd_ee_adapter.h"
#include "sprd_ee_adapter_log.h"
#include "properties.h"
#include <string.h>
#include "ee_sprd.h"

#include "cmr_types.h"
#include "ae_ctrl_common.h"

#ifdef DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

void *sprd_ee_adpt_init(int width, int height, void *param)
{
	void *handle = 0;
	char strRunType[256];
	property_get("persist.vendor.cam.ee.run_type", strRunType , "");
	if (!(strcmp("cpu", strRunType)))
		g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
	else if (!(strcmp("vdsp", strRunType)))
		g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
	EE_LOGI("current run type: %d\n", g_run_type);

	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
	{
		handle=sprd_ee_init(width,height);
	}
	else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
	{
		handle=sprd_ee_init_vdsp(width,height);
	}
	else
	{
		EE_LOGI("unknown type: %d\n",g_run_type);
	}

	return handle;
}

int sprd_ee_adpt_deinit(void *handle)
{
	int ret = 0;
	if(handle==NULL)
	{
		EE_LOGE("params is NULL\n");
		return -1;
	}

	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
	{
		ret = sprd_ee_deinit(handle);
	}
	else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
	{
		ret = sprd_ee_deinit_vdsp(handle);
	}
	else
	{
		EE_LOGI("unknown type: %d\n",g_run_type);
	}

	return ret;
}

int sprd_ee_adpt_ctrl(sprd_ee_cmd_t cmd, void *param)
{
	int ret = 0;
	if(param==NULL)
	{
		EE_LOGE("params is NULL\n");
		return -1;
	}

	switch (cmd)
	{
	case SPRD_EE_OPEN_CMD:
		{
			sprd_ee_param_t *ee_param=(sprd_ee_param_t *)param;
			ee_param->ctx=sprd_ee_adpt_init(ee_param->width,ee_param->height,NULL);
			if(NULL==ee_param->ctx)
			{
				EE_LOGE("sprd_ee_adpt_init fail\n");
				ret=-1;
			}
			break;
		}
	case SPRD_EE_CLOSE_CMD:
		{
			sprd_ee_param_t *ee_param=(sprd_ee_param_t *)param;
			ret=sprd_ee_adpt_deinit(ee_param->ctx);
			break;
		}
	case SPRD_EE_PROCESS_CMD:
		{
			sprd_ee_param_t *ee_param=(sprd_ee_param_t *)param;
			sprd_ee_tuning_param tuningParam;
			tuningParam.tuning_param=ee_param->tuningParam;
			tuningParam.tuning_size=ee_param->tuningSize;
			tuningParam.mode_idx=ee_param->mode_idx;
			tuningParam.scene_idx=ee_param->scene_idx;
			tuningParam.level_idx[0]=ee_param->level_idx[0];
			tuningParam.level_idx[1]=ee_param->level_idx[1];
			tuningParam.level_weight[0]=ee_param->level_weight[0];
			tuningParam.level_weight[1]=ee_param->level_weight[1];
			tuningParam.level_num=ee_param->level_num;
			tuningParam.crop_width=ee_param->crop_width;
			tuningParam.crop_height=ee_param->crop_height;
			tuningParam.scene_map_buffer=ee_param->scene_map_buffer;
			if(0 != ee_param->ae_param)
			{
				struct ae_callback_param *p = (struct ae_callback_param*)(ee_param->ae_param);
				tuningParam.face_stable = p->face_stable;
				tuningParam.face_num = (unsigned short)(p->face_num);
			} else {
				tuningParam.face_stable = 0;
				tuningParam.face_num = 0;
				EE_LOGW("ee_param == NULL, face info set to 0\n");
			}

			if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
			{
				void *srcImg=ee_param->imgIn.addr[0];
				void *dstImg=ee_param->imgOut.addr[0];

				ret = sprd_ee_process(ee_param->ctx, srcImg, dstImg, &tuningParam, ee_param->width, ee_param->height);
			}
			else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
			{
				sprd_ee_buffer_vdsp srcImg,dstImg;
				srcImg.data=ee_param->imgIn.addr[0];
				srcImg.fd=ee_param->imgIn.ion_fd;
				dstImg.data=ee_param->imgOut.addr[0];
				dstImg.fd=ee_param->imgOut.ion_fd;

				ret = sprd_ee_process_vdsp(ee_param->ctx, &srcImg, &dstImg, &tuningParam, ee_param->width, ee_param->height);
			}
			break;
		}
	default:
		EE_LOGI("unknown cmd: %d\n", cmd);
		break;
	}

	return ret;
}
