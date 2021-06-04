#ifndef __MFSR_ADAPTER_H__
#define __MFSR_ADAPTER_H__

#include "sprd_camalg_adapter.h"
#include "mfsr_interface.h"

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

typedef enum {
    MFSR_CMD_OPEN,
    MFSR_CMD_CLOSE,
    MFSR_CMD_PROCESS,
    MFSR_CMD_GET_VERSION,
    MFSR_CMD_GET_EXIF,
    MFSR_CMD_GET_CAPBILITY,
    MFSR_CMD_GET_FRAMENUM,
    MFSR_CMD_FAST_STOP,
    MFSR_CMD_POST_OPEN,
    MFSR_CMD_POST_CLOSE,
    MFSR_CMD_POST_PROCESS,
} MFSR_CMD;

typedef struct {
    void **ctx;
    mfsr_open_param *param;
} MFSR_CMD_OPEN_PARAM_T;

typedef struct {
    void **ctx;
} MFSR_CMD_CLOSE_PARAM_T;

typedef struct {
    void **ctx;
    sprd_camalg_image_t *image_input;
    sprd_camalg_image_t *image_output;
    sprd_camalg_image_t *detail_mask;
    sprd_camalg_image_t *denoise_mask;
    mfsr_process_param *process_param;
} MFSR_CMD_PROCESS_PARAM_T;

typedef struct {
    mfsr_lib_version *lib_version;
} MFSR_CMD_GET_VERSION_PARAM_T;

typedef struct {
    void **ctx;
    mfsr_exif_t *exif_info;
} MFSR_CMD_GET_EXIF_PARAM_T;

typedef struct {
   mfsr_capbility_key key;
   mfsr_capbility_value *value;
} MFSR_CMD_GET_CAPBILITY_PARAM_T;

typedef struct {
    void **ctx;
} MFSR_CMD_FAST_STOP_PARAM_T;

typedef struct {
    mfsr_calc_frame_param_in_t *param_in;
    mfsr_calc_frame_param_out_t *param_out;
    mfsr_calc_frame_status_t *calc_status;
} MFSR_CMD_GET_FRAMENUM_PARAM_T;

typedef struct {
    void **ctx;
    sprd_camalg_image_t *image;
    sprd_camalg_image_t *detail_mask;
    sprd_camalg_image_t *denoise_mask;
    mfsr_process_param *process_param;
} MFSR_CMD_POSTPROCESS_PARAM_T;


#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT int sprd_mfsr_adapter_ctrl(MFSR_CMD cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif