#include <stdint.h>
#include "mfsr_interface.h"
#include "mfsr_adapter.h"
#include "sprd_camalg_adapter.h"
#include "properties.h"
#include <string.h>
#include <utils/Log.h>
#include "cmr_types.h"
#include "ae_ctrl_common.h"
#include "sprd_camalg_assist.h"

#define LOG_TAG "sprd_mfsr_adapter"
#define ADAPTER_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define ADAPTER_LOGD(format,...) ALOGD(format, ##__VA_ARGS__)

#ifdef DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

int sprd_mfsr_adapter_open(void **ctx, mfsr_open_param *param)
{
    char strProp[256];

    param->align_param.input_data_mode = ALIGN_INPUT_MODE;

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP && !sprd_caa_vdsp_check_supported())
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    property_get("persist.vendor.cam.mfsr.run_type", strProp , "");
    if (!strcmp("cpu", strProp))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if (!strcmp("vdsp", strProp))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    param->run_type = g_run_type;

    ADAPTER_LOGD("run_type: %d\n", param->run_type);

    return sprd_mfsr_open(ctx, param);
}

int sprd_mfsr_adapter_process(void **ctx, sprd_camalg_image_t *image_input,sprd_camalg_image_t *image_output,sprd_camalg_image_t *detail_mask,sprd_camalg_image_t *denoise_mask,mfsr_process_param *process_param)
{
    return sprd_mfsr_process(*ctx, image_input,image_output,detail_mask,denoise_mask,process_param);
}
 int sprd_mfsr_adapter_post_process(void **ctx, sprd_camalg_image_t *image,sprd_camalg_image_t *detail_mask,sprd_camalg_image_t *denoise_mask,mfsr_process_param *process_param)
{
    return sprd_mfsr_post_process(*ctx, image,detail_mask,denoise_mask,process_param);
}
int sprd_mfsr_adapter_close(void **ctx)
{
    return sprd_mfsr_close(ctx);
}

int sprd_mfsr_adapter_ctrl(MFSR_CMD cmd, void *vparam)
{
    int ret = -1;
    if (!vparam) {
        ADAPTER_LOGE("null vparam pointer\n");
        return ret;
    }
    switch (cmd) {
        case MFSR_CMD_OPEN:
        {
            MFSR_CMD_OPEN_PARAM_T *param = (MFSR_CMD_OPEN_PARAM_T *)vparam;
            ret = sprd_mfsr_adapter_open(param->ctx, param->param);
            break;
        }
        case MFSR_CMD_CLOSE:
        {
            MFSR_CMD_CLOSE_PARAM_T *param = (MFSR_CMD_CLOSE_PARAM_T *)vparam;
            ret = sprd_mfsr_adapter_close(param->ctx);
            break;
        }
        case MFSR_CMD_PROCESS:
        {
            MFSR_CMD_PROCESS_PARAM_T *param = (MFSR_CMD_PROCESS_PARAM_T *)vparam;
            ret = sprd_mfsr_adapter_process(param->ctx, param->image_input, param->image_output, param->detail_mask, param->denoise_mask,param->process_param);
            break;
        }
        case MFSR_CMD_GET_VERSION:
        {
            MFSR_CMD_GET_VERSION_PARAM_T *param = (MFSR_CMD_GET_VERSION_PARAM_T *)vparam;
            ret = sprd_mfsr_get_version(param->lib_version);
            break;
        }
        case MFSR_CMD_GET_EXIF:
        {
            MFSR_CMD_GET_EXIF_PARAM_T *param = (MFSR_CMD_GET_EXIF_PARAM_T *)vparam;
            ret = sprd_mfsr_get_exif(*param->ctx, param->exif_info);
            break;
        }
        case MFSR_CMD_GET_CAPBILITY:
        {
            MFSR_CMD_GET_CAPBILITY_PARAM_T *param = (MFSR_CMD_GET_CAPBILITY_PARAM_T *)vparam;
            ret = sprd_mfsr_get_capbility(param->key, param->value);
            break;
        }
        case MFSR_CMD_GET_FRAMENUM:
        {
            MFSR_CMD_GET_FRAMENUM_PARAM_T *param = (MFSR_CMD_GET_FRAMENUM_PARAM_T *)vparam;
            ret = sprd_mfsr_get_frame_num(param->param_in, param->param_out, param->calc_status);
            break;
        }
        case MFSR_CMD_FAST_STOP:
        {
            MFSR_CMD_FAST_STOP_PARAM_T *param = (MFSR_CMD_FAST_STOP_PARAM_T *)vparam;
            ret = sprd_mfsr_fast_stop(*param->ctx);
            break;
        }
        case MFSR_CMD_POST_OPEN:
        {
            MFSR_CMD_OPEN_PARAM_T *param = (MFSR_CMD_OPEN_PARAM_T *)vparam;
            ret = sprd_mfsr_post_open(param->ctx, param->param);
            break;
        }
        case MFSR_CMD_POST_CLOSE:
        {
            MFSR_CMD_CLOSE_PARAM_T *param = (MFSR_CMD_CLOSE_PARAM_T *)vparam;
            ret = sprd_mfsr_post_close(param->ctx);
            break;
        }
        case MFSR_CMD_POST_PROCESS:
        {
            MFSR_CMD_POSTPROCESS_PARAM_T *param = (MFSR_CMD_POSTPROCESS_PARAM_T *)vparam;
            ret = sprd_mfsr_adapter_post_process(param->ctx, param->image, param->detail_mask, param->denoise_mask,param->process_param);
            break;
        }
        default:
            ADAPTER_LOGE("unknown cmd: %d\n", cmd);
    }
    return ret;
}
