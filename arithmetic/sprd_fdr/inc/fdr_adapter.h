#ifndef __FDR_ADAPTER_H__
#define __FDR_ADAPTER_H__

#include "sprd_camalg_adapter.h"
#include "fdr_interface.h"

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

typedef enum {
    FDR_CMD_OPEN,
    FDR_CMD_CLOSE,
    FDR_CMD_ALIGN_INIT,
    FDR_CMD_ALIGN,
    FDR_CMD_MERGE,
    FDR_CMD_FUSION,
} FDR_CMD;

typedef struct {
    void **ctx;
    fdr_open_param *param;
} FDR_CMD_OPEN_PARAM_T;

typedef struct {
    void **ctx;
} FDR_CMD_CLOSE_PARAM_T;

typedef struct {
    void **ctx;
    sprd_camalg_image_t *image_array;
    fdr_align_init_param *init_param;
} FDR_CMD_ALIGN_INIT_PARAM_T;

typedef struct {
    void **ctx;
    sprd_camalg_image_t *image;
} FDR_CMD_ALIGN_PARAM_T;

typedef struct {
    void **ctx;
    sprd_camalg_image_t *image_array;
    fdr_merge_param *merge_param;
    fdr_merge_out_param *merge_out_param;
} FDR_CMD_MERGE_PARAM_T;

typedef struct {
    void **ctx;
    sprd_camalg_image_t *image_in_array;
    sprd_camalg_image_t *image_out;
    expfusion_proc_param_t *param_proc;
} FDR_CMD_FUSION_PARAM_T;

#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT int sprd_fdr_adapter_ctrl(FDR_CMD cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif