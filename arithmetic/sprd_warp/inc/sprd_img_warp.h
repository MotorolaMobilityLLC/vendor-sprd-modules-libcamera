#ifndef _SPRD_IMG_WARP_H_
#define _SPRD_IMG_WARP_H_

#define MAX_CHANNAL_NUM 3

typedef enum { WARP_FMT_YUV420P, WARP_FMT_YUV420SP } warp_fmt_e;

typedef enum {
    WARP_RECTIFY,
    WARP_UNDISTORT,
    WARP_PROJECTIVE,
} img_warp_mode_t;

typedef struct {
    int fullsize_width;
    int fullsize_height;
    int input_width;
    int input_height;
    int crop_x;
    int crop_y;
    int crop_width;
    int crop_height;
} img_warp_input_info_t;

typedef struct {
    int fullsize_width;
    int fullsize_height;
    int calib_width;
    int calib_height;
    int crop_x;
    int crop_y;
    int crop_width;
    int crop_height;
    float fov_scale;
    float camera_k[3][3];  // for WARP_UNDISTORT
    float dist_coefs[5];   // k1, k2, p1, p2, k3	// for WARP_UNDISTORT
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
#ifdef __cplusplus
}
#endif

#endif
