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
    FDR_CMD_GET_VERSION,
    FDR_CMD_GET_EXIF,
    FDR_CMD_GET_FRAMENUM,
    FDR_CMD_GET_MAX_FRAMENUM,
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
    void *ae_param;
    fdr_merge_out_param *merge_out_param;
} FDR_CMD_MERGE_PARAM_T;

typedef struct {
    void **ctx;
    sprd_camalg_image_t *image_in_array;
    sprd_camalg_image_t *image_out;
    expfusion_proc_param_t *param_proc;
    void *ae_param;
} FDR_CMD_FUSION_PARAM_T;

typedef struct {
    fdr_lib_version *lib_version;
} FDR_CMD_GET_VERSION_PARAM_T;

typedef struct {
    void **ctx;
    fdr_exif_t *exif_info;
} FDR_CMD_GET_EXIF_PARAM_T;

typedef struct {
    fdr_calc_frame_param_in_t *param_in;
    fdr_calc_frame_param_out_t *param_out;
    fdr_calc_frame_status_t *calc_status;
} FDR_CMD_GET_FRAMENUM_PARAM_T;

typedef struct {
    int *max_total_frame_num;
    int *max_ref_frame_num;
} FDR_CMD_GET_MAX_FRAMENUM_PARAM_T;


#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT int sprd_fdr_adapter_ctrl(FDR_CMD cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif