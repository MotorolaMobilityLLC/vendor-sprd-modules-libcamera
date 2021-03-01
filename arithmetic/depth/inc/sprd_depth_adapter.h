#ifndef __SPRD_DEPTH_ADAPTER_HEADER_H__
#define __SPRD_DEPTH_ADAPTER_HEADER_H__

#include "sprd_camalg_adapter.h"
#include "SGM_SPRD.h"
#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))

typedef enum {
    SPRD_DEPTH_GET_VERSION_CMD = 0,
    SPRD_DEPTH_RUN_CMD,
    SPRD_DEPTH_FAST_STOP_CMD,
    SPRD_DEPTH_GDEPTH_CMD,
    SPRD_DEPTH_USERSET_CMD,
    SPRD_DEPTH_ONLINECALI_CMD,     //reserved
    SPRD_DEPTH_ONLINECALI_POST_CMD,//reserved
    SPRD_DEPTH_MAX_CMD
} sprd_depth_cmd_t;

typedef enum {
    CAPTURE_MODE,
    PREVIEW_MODE
}depthoutFormat;

struct depth_inputparam {
    int input_width_main;
    int input_height_main;
    int input_width_sub;
    int input_height_sub;
    void *otpbuf;
    int otpsize;
    char *config_param;
};

struct depth_outputparam {
    int  outputsize;
    int  output_width;
    int  output_height;
};

typedef struct {
    int F_number; // 1 ~ 20
    int sel_x; /* The point which be touched */
    int sel_y; /* The point which be touched */
    int VCM_cur_value;
    void *tuning_golden_vcm;
    struct portrait_mode_param *portrait_param;
}depthrun_inparam;

typedef struct {
    unsigned short near;
    unsigned short far;
    struct sprd_camalg_image output[2];//[0]confidence_map [1]depthnorm_data
} gdepth_out;

typedef struct {
    struct depth_inputparam inparam ;
    struct depth_outputparam outputinfo;
    depthoutFormat format;
}sprd_depth_init_param_t;

typedef struct {
    struct sprd_camalg_image input[2];//[0]left_yuv   [1]right_yuv 
    struct sprd_camalg_image output;//depth16(16bit) 
    depthrun_inparam params;
    bool mChangeSensor;
    void *input_otpbuf;
    int input_otpsize;
    void *output_otpbuf;
    int output_otpsize;
    int ret_otp;//0 success,else fail
}sprd_depth_run_param_t;
  
typedef struct {
    struct sprd_camalg_image input;//depth16(16bit)
    gdepth_out gdepth_output; 
}sprd_depth_gdepth_param_t;
  
typedef struct {
    char *ptr;
    int size;
}sprd_depth_userset_param_t;

typedef struct {
    struct sprd_camalg_image input[2];//[0] left_yuv   [1]right_yuv 
    struct sprd_camalg_image output;//Maptable(32bit) 
}sprd_depth_onlineCali_param_t;//reserved

typedef struct {
    struct sprd_camalg_image input;//Maptable(32bit) 
    struct sprd_camalg_image output;//Maptable(32bit) 
}sprd_depth_onlinepost_param_t;//reserved
/*
    init depth adapter instance
    return value: handle; 
    @param: sprd_depth_init_param_t
*/
JNIEXPORT void *sprd_depth_adpt_init(void *param);

/*
    deinit depth adapter instance 
    return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_depth_adpt_deinit(void *handle);

/*
    depth adapter cmd process interface
    return value: 0 is ok, other value is failed
    @param: depend on cmd type:
        - SPRD_DEPTH_GET_VERSION_CMD: 0
        - SPRD_DEPTH_RUN_CMD: sprd_depth_run_param_t
        - SPRD_DEPTH_FAST_STOP_CMD: 0 
        - SPRD_DEPTH_GDEPTH_CMD: sprd_depth_gdepth_param_t
        - SPRD_DEPTH_USERSET_CMD: sprd_depth_userset_param_t
        - SPRD_DEPTH_ONLINECALI_CMD: sprd_depth_onlineCali_param_t  //reserved
        - SPRD_DEPTH_ONLINECALI_POST_CMD: sprd_depth_onlinepost_param_t  //reserved
*/
JNIEXPORT int sprd_depth_adpt_ctrl(void *handle, sprd_depth_cmd_t cmd, void *param);

/*
    depth adapter get running type, the output is type, such as cpu/gpu/vdsp 
    return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_depth_get_devicetype(enum camalg_run_type *type);

/*
    depth adapter set running type 
    return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_depth_set_devicetype(enum camalg_run_type type);


#ifdef __cplusplus
}
#endif

#endif
