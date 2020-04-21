#include "sprd_dre_adapter_pro.h"
#include "sprd_dre_adapter_log_pro.h"
#include "properties.h"
#include <string.h>
#include <utils/Log.h>

#define LOG_TAG "sprd_dre_adapter_pro"

#define DRE_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define DRE_LOGI(format,...) ALOGI(format, ##__VA_ARGS__)
#define DRE_LOGW(format,...) ALOGW(format, ##__VA_ARGS__)

#if defined DEFAULT_RUNTYPE_GPU
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_GPU;
#elif defined DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

int sprd_dre_pro_adpt_init(void **handle, int max_width, int max_height, void *init_param)
{
    int ret = 0;
    char strRunType[256];

    property_get("persist.vendor.cam.dre.run_type", strRunType , "cpu");
    if (!(strcmp("cpu", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if (!(strcmp("vdsp", strRunType)))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    else
        g_run_type = SPRD_CAMALG_RUN_TYPE_GPU;

    DRE_LOGI("current run type is: %d\n", g_run_type);

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) {
        ret = sprd_dre_pro_init(handle, max_width, max_height, init_param);
        if(ret) {
            DRE_LOGI("DRE init fail ,error type = %d \n",ret);
        }
    }
    else {
        DRE_LOGI("current run type is not CPU\n");
        ret = EPARAM;
    }
    return ret;
}

int sprd_dre_pro_adpt_deinit(void *handle)
{
    int ret = 0;
    if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
        ret = sprd_dre_pro_deinit(&handle);
    return ret;
}

int sprd_dre_pro_adpt_ctrl(void *handle, sprd_dre_pro_cmd_t cmd, sprd_camalg_image_t* input, void *param)
{
    int ret = 0;
    switch (cmd) {
    case SPRD_DRE_PROCESS_CMD:
            if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) {
                ret = sprd_dre_pro_run(handle, input, param);
            }
            break;
    case SPRD_DRE_FAST_STOP_CMD:
            sprd_dre_pro_fast_stop(handle);
            break;
    default:
            DRE_LOGW("unknown cmd: %d\n", cmd);
            break;
    }

    return ret;
}

int sprd_dre_pro_get_devicetype(enum camalg_run_type *type)
{
    if (!type)
        return 1;
        *type = g_run_type;

    return 0;
}

int sprd_dre_pro_set_devicetype(enum camalg_run_type type)
{
    if (type < SPRD_CAMALG_RUN_TYPE_CPU || type >= SPRD_CAMALG_RUN_TYPE_MAX)
        return 1;
    g_run_type = type;

    return 0;
}