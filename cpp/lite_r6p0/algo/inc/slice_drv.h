#ifndef _CPP_SLICE_DRV_H
#define _CPP_SLICE_DRV_H

#include "sprd_cpp.h"

#ifdef __cplusplus
extern "C"{
#endif
#ifdef CPP_LITE_R6P0
void slice_drv_param_calc(struct sprd_cpp_scale_slice_parm *param_ptr);
#else
#ifdef CPP_LITE_R5P0
void slice_drv_param_calc(slice_drv_param_t *param_ptr);
#endif
#endif
#ifdef __cplusplus
    };
#endif

#endif
