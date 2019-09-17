#include <stdint.h>
#include "sprd_img_warp.h"
#include "sprd_camalg_adapter.h"
#include "properties.h"
#include <string.h>
#include <utils/Log.h>

#define LOG_TAG "sprd_warp_adapter"
#define WARP_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define WARP_LOGD(format,...) ALOGD(format, ##__VA_ARGS__)

#if defined DEFAULT_RUNTYPE_GPU
static enum camalg_run_type g_run_type[2] = {SPRD_CAMALG_RUN_TYPE_GPU, SPRD_CAMALG_RUN_TYPE_GPU};
#elif defined DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type[2] = {SPRD_CAMALG_RUN_TYPE_VDSP, SPRD_CAMALG_RUN_TYPE_VDSP};
#else
static enum camalg_run_type g_run_type[2] = {SPRD_CAMALG_RUN_TYPE_CPU, SPRD_CAMALG_RUN_TYPE_CPU};
#endif

int sprd_warp_adapter_open(img_warp_inst_t *inst, bool *isISPZoom, void *param, INST_TAG tag)
{
    int ret = 0;

    char strRunType[256];
    if (tag == WARP_CAPTURE)
        property_get("persist.vendor.cam.warp.capture.run_type", strRunType , "");
    else if (tag == WARP_PREVIEW)
        property_get("persist.vendor.cam.warp.preview.run_type", strRunType , "");

    if (!(strcmp("gpu", strRunType))) {
        g_run_type[tag] = SPRD_CAMALG_RUN_TYPE_GPU;
    } else if (!(strcmp("vdsp", strRunType))) {
        g_run_type[tag] = SPRD_CAMALG_RUN_TYPE_VDSP;
    } else if (!(strcmp("cpu", strRunType))) {
        g_run_type[tag] = SPRD_CAMALG_RUN_TYPE_CPU;
    }

    WARP_LOGD("current run type: %d\n", g_run_type[tag]);

    if (g_run_type[tag] == SPRD_CAMALG_RUN_TYPE_GPU) {
        ret = img_warp_grid_open(inst, (img_warp_param_t *)param);
        *isISPZoom = false;
    } else if (g_run_type[tag] == SPRD_CAMALG_RUN_TYPE_VDSP) {
        ret = img_warp_grid_vdsp_open(inst, (img_warp_param_t *)param, tag);
        *isISPZoom = true;
    } else {
        ret = img_warp_grid_cpu_open(inst, (img_warp_param_t *)param, tag);
        *isISPZoom = false;
    }

    return ret;
}

void sprd_warp_adapter_run(img_warp_inst_t inst, img_warp_buffer_t *input, img_warp_buffer_t *output, void *param, INST_TAG tag)
{
    if (g_run_type[tag] == SPRD_CAMALG_RUN_TYPE_GPU)
        img_warp_grid_run(inst, input, output, param);
    else if (g_run_type[tag] == SPRD_CAMALG_RUN_TYPE_VDSP)
        img_warp_grid_vdsp_run(inst, input, output, param);
    else
        img_warp_grid_cpu_run(inst, input, output, param);
}

void sprd_warp_adapter_close(img_warp_inst_t *inst, INST_TAG tag)
{
    if (g_run_type[tag] == SPRD_CAMALG_RUN_TYPE_GPU)
        img_warp_grid_close(inst);
    else if (g_run_type[tag] == SPRD_CAMALG_RUN_TYPE_VDSP)
        img_warp_grid_vdsp_close(inst);
    else
        img_warp_grid_cpu_close(inst);
}


