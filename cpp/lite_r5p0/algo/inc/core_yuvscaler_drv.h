#ifndef __CORE_YUVSCALER_DRV_H__
#define __CORE_YUVSCALER_DRV_H__

#include "algo_int.h"
#include "algo_macro.h"

#define COUNT   256
#define PI  3.14159265357
#define SIGN2(input,p)  {if (p>=0) input = 1; if (p < 0) input = -1;}

#define YUV422  2
#define YUV420  0

#define SEMI_PLANAR 0
#define PLANAR  1

#define LUMA    1
#define CB      2
#define CR      3

#define clip3(a, b, c)  ( ((c)<(a)) ? (a) : (((c) > (b)) ? (b) : (c)) )
#define clip1(x)    clip3(0, 255, (x))

#define SCALER_COEF_TAB_LEN_HOR 48
#define SCALER_COEF_TAB_LEN_VER 68

#define YUVSCALER_OVERLAP_UP    32
#define YUVSCALER_OVERLAP_DOWN  52
#define YUVSCALER_OVERLAP_LEFT  16
#define YUVSCALER_OVERLAP_RIGHT 68

typedef struct
{
    uint8   trim_en;

    uint16  trim_start_x;
    uint16  trim_start_y;
    uint16  trim_size_x;
    uint16  trim_size_y;
} trim_info_t;

typedef struct
{
    uint8   deci_x_en;          ///< 0: disable; 1:enable
    uint8   deci_y_en;          ///< 0: disable; 1:enable

    uint8   deci_x;             ///< deci factor:1,2,4,8,16
    uint8   deci_y;             ///< deci factor:1,2,4,8,16
    uint8   deci_cut_first_y;
    uint8   deci_option;        ///< 0:direct deci; 1:average deci; 2:only average for luma
} deci_info_t;

typedef struct
{
    int16     y_hor_coef[8][8];     /* Luma horizontal coefficients table */
    int16     c_hor_coef[8][8];     /* Chroma horizontal coefficients table */
    int16     y_ver_coef[9][16];    /* Luma vertical down coefficients table */
    int16     c_ver_coef[9][16];    /* Chroma veritical down coefficients table */
} scaler_coef_info_t;

typedef struct
{
    int32   scaler_init_phase[2]     ;
    int16   scaler_init_phase_int[2][2] ;    //[hor/ver][luma/chroma]
    uint16  scaler_init_phase_rmd[2][2] ;
} scaler_phase_info_t;

typedef struct
{
    uint8   scaler_en;          ///0: disable; 1:enable

    uint8   input_pixfmt;          //input yuv format: 0=yuv422 or 1=yuv420;
    uint8   output_pixfmt;

    uint16  scaler_in_width;
    uint16  scaler_in_height;
    uint16  scaler_out_width;
    uint16  scaler_out_height;

    uint16  scaler_factor_in_hor;
    uint16  scaler_factor_out_hor;
    uint16  scaler_factor_in_ver;
    uint16  scaler_factor_out_ver;

    uint8   scaler_y_hor_tap;
    uint8   scaler_y_ver_tap;   ///Y Vertical tap of scaling
    uint8   scaler_uv_hor_tap;
    uint8   scaler_uv_ver_tap;

    scaler_phase_info_t init_phase_info;
    scaler_coef_info_t  scaler_coef_info;
} scaler_info_t;

typedef struct
{
    uint16  overlap_up;
    uint16  overlap_down;
    uint16  overlap_left;
    uint16  overlap_right;
} scaler_overlap_t;

typedef struct
{
    int slice_id;
    int slice_width;
    int slice_height;

    int sliceRows;
    int sliceCols;
    int sliceRowNo;
    int sliceColNo;

    int start_col;
    int start_row;
    int end_col;
    int end_row;

    int overlap_left;
    int overlap_right;
    int overlap_up;
    int overlap_down;
    
    int init_phase_hor;
    int init_phase_ver;

} scaler_slice_t;

typedef struct
{
    uint8           bypass;

    uint8           input_pixfmt;        ///< 00:YUV422; 1:YUV420;
    uint8           output_pixfmt;       ///< 00:YUV422; 1:YUV420;
    uint8           output_align_hor;
    uint8           output_align_ver;

    uint16          src_size_x;
    uint16          src_size_y;
    uint16          dst_start_x;
    uint16          dst_start_y;
    uint16          dst_size_x;
    uint16          dst_size_y;

    int32 init_phase_hor;

    trim_info_t     trim0_info;
    deci_info_t     deci_info;
    scaler_info_t   scaler_info;
    trim_info_t     trim1_info;
} yuvscaler_param_t;

#ifdef __cplusplus
extern "C" {
#endif

    void yuv_scaler_init_frame_info(yuvscaler_param_t *frame_scaler);
    
    void yuv_scaler_init_slice_info(yuvscaler_param_t *frame_scaler, yuvscaler_param_t *slice_scaler, scaler_slice_t *slice_info, scaler_overlap_t *scaler_overlap, uint16 ref_ox, uint16 ref_oy);
    
    void yuv_scaler_init_slice_info_v2(yuvscaler_param_t *frame_scaler, yuvscaler_param_t *slice_scaler, 
        scaler_slice_t *slice_info,const scaler_slice_t *scaler_slice_info,
        scaler_overlap_t *scaler_overlap, uint16 ref_ox, uint16 ref_oy);
   
    void yuv_scaler_init_slice_info_v3(
        yuvscaler_param_t *frame_scaler, 
        yuvscaler_param_t *slice_scaler, 
        scaler_slice_t *slice_info,
        const scaler_slice_t *input_slice_info,
        const scaler_slice_t *output_slice_info);

    void yuv_scaler_gen_scaler_coef(int16 i_w, int16 i_h, int16 o_w, int16 o_h, uint8 input_yuvfmt, uint8 output_yuvfmt, uint8 *luma_tap_hor, uint8 *chroma_tap_hor, uint8 *luma_tap_ver, uint8 *chroma_tap_ver, scaler_coef_info_t *scaler_coef_info);

    void yuv_scaler_get_input_slice(yuvscaler_param_t *slice_scaler, const yuvscaler_param_t *frame_scaler, const scaler_slice_t *slice_info);

    void calc_scaler_output_slice_info(int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
        int input_slice_start, int input_slice_size, int output_pixel_align,
        int *output_slice_start, int *output_slice_size);
    
    void calc_scaler_input_slice_info(int trim_en,int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
        int output_slice_start, int output_slice_size, int input_pixel_align,
        int *input_slice_start, int *input_slice_size, int *input_slice_phase);
    
    void calc_scaler_output_slice_info_v2(int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
        int input_slice_start, int input_slice_size, int output_pixel_align,
        int *output_slice_start, int *output_slice_size);
void est_scaler_output_slice_info(int trim_en,int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
    int input_slice_start, int input_slice_size, int output_pixel_align, int *output_slice_end);
 void est_scaler_output_slice_info_v2(int trim_en,int trim_start, int trim_size, int deci, int scl_factor_in, int scl_factor_out, int scl_tap, int init_phase,
    int input_slice_start, int input_slice_size, int output_pixel_align, int *output_slice_end);
#ifdef __cplusplus
}
#endif


#endif
