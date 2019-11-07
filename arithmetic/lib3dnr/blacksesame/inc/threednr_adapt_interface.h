#ifndef _3DNR_INTERFACE_H
#define _3DNR_INTERFACE_H

#include "sprd_camalg_adapter.h"
#include <stdint.h>

//#include "esUtil.h"

#define LOCAL_PLATFORM_ID_GENERIC     0x0000
#define LOCAL_PLATFORM_ID_PIKE2          0x0100
#define LOCAL_PLATFORM_ID_SHARKLE     0x0200
#define LOCAL_PLATFORM_ID_SHARKLER   0x0201
#define LOCAL_PLATFORM_ID_SHARKL3     0x0300
#define LOCAL_PLATFORM_ID_SHARKL5     0x0400
#define LOCAL_PLATFORM_ID_SHARKL5P   0x0401
#define LOCAL_PLATFORM_ID_SHARKL6     0x0500
#define LOCAL_PLATFORM_ID_SHARKL6P   0x0501
#define LOCAL_PLATFORM_ID_ROC1          0x0600

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))


typedef  struct private_handle {
    uint8_t *bufferY;
}private_handle_t;

typedef struct c3dnr_cpu_buffer {
    uint8_t *bufferY;
    uint8_t *bufferU;
    uint8_t *bufferV;
    int32_t fd; //wxz: for phy addr.
}c3dnr_cpu_buffer_t;

typedef struct c3dnr_gpu_buffer{
    void *handle;
}c3dnr_gpu_buffer_t;

typedef union c3dnr_buffer {
    c3dnr_cpu_buffer_t cpu_buffer;
    c3dnr_gpu_buffer_t gpu_buffer;
}c3dnr_buffer_t;

typedef struct c3dnr_cap_gpu_buffer {
    void * gpuHandle;
    uint8_t *bufferY;
    uint8_t *bufferU;
    uint8_t *bufferV;
}c3dnr_cap_gpu_buffer_t;

typedef struct c3dnr_param_info
{
    int productInfo;      //pike2 sharkLe 0, sharkl3, sharkl5 1;
    uint16_t orig_width;
    uint16_t orig_height;
    uint16_t small_width;
    uint16_t small_height;
    uint16_t total_frame_num;
    uint16_t gain;
    uint16_t low_thr;
    uint16_t ratio;
    int *sigma_tmp;
    int *slope_tmp;
    c3dnr_buffer_t *poutimg;
#ifdef USE_ISP_HW
    cmr_handle isp_handle; //wxz: call the isp_ioctl need the handle.
    isp_ioctl_fun isp_ioctrl;
#endif
    int yuv_mode;    //0: nv12 1: nv21
    int control_en;  //1//1, gpu time check, 0, not check
    int thread_num;
    int thread_num_acc;
    int preview_cpyBuf;

	//---
    uint16_t SearchWindow_x;
    uint16_t SearchWindow_y;
    int (*threthold)[6];
    int (*slope)[6];
    int recur_str; // recursion stride for preview
    int match_ratio_sad;
    int match_ratio_pro;
    int feat_thr;
    int luma_ratio_high;
    int luma_ratio_low;
    int zone_size;
    int gain_thr[6];
    int reserverd[16];
}c3dnr_param_info_t;

typedef struct c3dnr_pre_inparam
{
    uint16_t gain;
}c3dnr_pre_inparam_t;

typedef enum {
    SPRD_3DNR_PROC_CAPTURE_CMD = 0,
    SPRD_3DNR_PROC_PREVIEW_CMD,
    SPRD_3DNR_PROC_VIDEO_CMD,
    SPRD_3DNR_PROC_SET_PARAMS_CMD,
    SPRD_3DNR_PROC_STOP_CMD,
    SPRD_3DNR_PROC_MAX_CMD
} sprd_3dnr_cmd_e;

typedef struct __c3dnr_capture_cmd_param{
    c3dnr_buffer_t* small_image;
    c3dnr_buffer_t* orig_image;
}c3dnr_capture_cmd_param_t;

typedef struct __c3dnr_preview_cmd_param{
    c3dnr_buffer_t* small_image;
    c3dnr_buffer_t* orig_image;
    c3dnr_buffer_t* out_image;
    uint16_t gain;
}c3dnr_capture_preview_cmd_param_t;

typedef struct __c3dnr_capnew_cmd_param{
    c3dnr_buffer_t* small_image;
    c3dnr_cap_gpu_buffer_t* orig_image;
}c3dnr_capnew_cmd_param_t;

typedef struct __c3dnr_setparam_cmd_param{
    int thr[4];
    int slp[4];
}c3dnr_setparam_cmd_param_t;

typedef union __c3dnr_cmd_param{
    c3dnr_capture_cmd_param_t cap_param;
    c3dnr_capnew_cmd_param_t  cap_new_param;
    c3dnr_capture_preview_cmd_param_t pre_param;
    c3dnr_setparam_cmd_param_t setpara_param;
}c3dnr_cmd_param_u;


typedef struct __c3dnr_cmd_proc {
    c3dnr_cmd_param_u proc_param;
    int callWay;//control gpu buffer lock way, 0, lock in libs, 1 lock out libs
}c3dnr_cmd_proc_t;

int threednr_deinit(void** handle);
int threednr_init(void** handle, c3dnr_param_info_t* param);
int threednr_callback(void* handle);
int threednr_setstop_flag(void* handle);
int threednr_setparams(void* handle, int thr[4], int slp[4]);
int threednr_function(void* handle, c3dnr_buffer_t* small_image, c3dnr_buffer_t* orig_image);
int threednr_function_new(void* handle, c3dnr_buffer_t* small_image, c3dnr_cap_gpu_buffer_t* orig_image);
int threednr_function_pre(void* handle, c3dnr_buffer_t *small_image, c3dnr_buffer_t *orig_image, c3dnr_buffer_t *out_image, c3dnr_pre_inparam_t* inputparam);
JNIEXPORT int sprd_3dnr_adpt_init(void **handle, c3dnr_param_info_t *param, void *tune_params);
JNIEXPORT int sprd_3dnr_adpt_deinit(void** handle);
JNIEXPORT int sprd_3dnr_adpt_ctrl(void *handle, sprd_3dnr_cmd_e cmd,void* param);
JNIEXPORT int sprd_3dnr_get_devicetype(sprd_camalg_device_type *type);
JNIEXPORT int sprd_3dnr_set_devicetype(sprd_camalg_device_type type);



#ifdef __cplusplus
}
#endif



#endif
