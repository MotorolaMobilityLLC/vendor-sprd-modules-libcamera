#ifndef _SPRD_IMG_WARP_H_
#define _SPRD_IMG_WARP_H_

#define MAX_CHANNAL_NUM 3

typedef enum { WARP_FMT_YUV420SP, WARP_FMT_YUV420P } warp_fmt_e;

typedef enum {
    WARP_UNDISTORT,
    WARP_RECTIFY,
    WARP_PROJECTIVE,
} img_warp_mode_t;

typedef struct {
    int fullsize_width;  /*sensor fullsize width*/
    int fullsize_height; /*sensor fullsize height*/
    int input_width;     /*input image width*/
    int input_height;    /*input image height*/
    /* crop relation between input image and sensor output image*/
    int crop_x;           /*crop_x is the start x coordinate*/
    int crop_y;           /*crop_y is the start y coordinate*/
    int crop_width;       /*crop_width is the output width of croping*/
    int crop_height;      /*crop height is the output height of croping*/
    int binning_mode;     /*binning mode 0: no binning 1:binning*/
} img_warp_input_info_t;

typedef struct {
    int fullsize_width;  /*sensor fullsize width*/
    int fullsize_height; /*sensor fullsize height*/
    int calib_width;     /*calib image width*/
    int calib_height;    /*calib image height*/
    /* crop relation between input image and sensor fullsize image*/
    int crop_x;          /*crop_x is the start x coordinate*/
    int crop_y;          /*crop_y is the start y coordinate*/
    int crop_width;      /*crop_width is the output width of croping*/
    int crop_height;     /*crop height is the output height of croping*/
    float fov_scale;     /*field of view scale*/
    float camera_k[3][3];  // for WARP_UNDISTORT
    float dist_coefs[14];  // 0~7:k1, k2, p1, p2, k3, k4, k5, k6, s1, s2, s3, s4, tauX, tauY for WARP_UNDISTORT
    float rectify_r[3][3]; // for WARP_RECTIFY
    float rectify_p[3][3]; // for WARP_RECTIFY
} img_warp_calib_info_t;

typedef struct {
    img_warp_input_info_t input_info;
    img_warp_calib_info_t calib_info;
    int dst_width;
    int dst_height;
    int mblk_order;
    int grid_order;
    img_warp_mode_t warp_mode;
    warp_fmt_e img_fmt;
    void *otp_buf;
    int otp_size;
} img_warp_param_t;

typedef void *img_warp_inst_t;

typedef struct {
    int width;
    int height;
    int stride;
    int ion_fd;                  // reserved for ion_buffer
    void *addr[MAX_CHANNAL_NUM]; // reserved for virtual address
    int offset[MAX_CHANNAL_NUM]; // reserved for channal offset
    void *graphic_handle;        // graphic buffer pointer
} img_warp_buffer_t;

typedef struct { float warp_projective[3][3]; } img_warp_projective_param_t;

typedef struct {
    float zoomRatio; //  [1.0f ~ 1.6f]
} img_warp_undistort_param_t;

#ifdef _MSC_VER
#ifdef _USRDLL
#ifdef SPRD_ISP_EXPORTS
#define SPRD_ISP_API __declspec(dllexport)
#else
#define SPRD_ISP_API __declspec(dllimport)
#endif
#else
#define SPRD_ISP_API
#endif
#else
#define SPRD_ISP_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

SPRD_ISP_API void img_warp_grid_config_default(img_warp_param_t *param);
/* img_warp_grid_config_default
 * usage: call once at initialization
 * @param->warp_mode: use WARP_UNDISTORT by default
 */
SPRD_ISP_API int img_warp_grid_open(img_warp_inst_t *inst,
                                    img_warp_param_t *param);
/* img_warp_grid_open
 * usage: call once at initialization
 */
SPRD_ISP_API void img_warp_grid_run(img_warp_inst_t inst,
                                    img_warp_buffer_t *input,
                                    img_warp_buffer_t *output, void *param);
/* img_warp_grid_run
 * usage: call each frame
 * @input: 1 input buffer pointer, use GraphicBuffer by default
 * @output: 1 output buffer, different from input buffer, use GraphicBuffer by
 * default
 * @param: img_warp_undistort_param_t for WARP_UNDISTORT mode
 */
SPRD_ISP_API void img_warp_grid_close(img_warp_inst_t *inst);
/* img_warp_grid_close
 * usage: call once at deinitialization
 */

/*
 * API for vdsp
 */
SPRD_ISP_API int img_warp_grid_vdsp_open(img_warp_inst_t *inst,
                                         img_warp_param_t *param);
SPRD_ISP_API void img_warp_grid_vdsp_run(img_warp_inst_t inst,
                                         img_warp_buffer_t *input,
                                         img_warp_buffer_t *output,
                                         void *param);
SPRD_ISP_API void img_warp_grid_vdsp_close(img_warp_inst_t *inst);

#ifdef __cplusplus
}
#endif

#endif
