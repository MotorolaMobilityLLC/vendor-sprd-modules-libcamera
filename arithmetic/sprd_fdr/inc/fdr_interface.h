#ifndef __FDR_INTERFACE_H__
#define __FDR_INTERFACE_H__

#include "sprd_camalg_adapter.h"

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

#define AE_FD_NUM   20

typedef struct {
    uint8_t     major;              /*!< lib major version */
    uint8_t     minor;              /*!< lib minor version */
    uint8_t     micro;              /*!< lib micro version */
    uint8_t     nano;               /*!< lib nano version */
    uint32_t    bug_id;             /*!< lib bug id */
    char        built_date[0x20];   /*!< lib built date */
    char        built_time[0x20];   /*!< lib built time */
    char        built_rev[0x100];   /*!< lib built version, linked with vcs resivion> */
} fdr_lib_version;

typedef struct {
    int start_x;
    int start_y;
    int end_x;
    int end_y;
} fdr_ae_rect;

typedef struct {
    fdr_ae_rect rect;
    uint32_t face_lum;
    int pose;
    int angle;
} fdr_ae_face;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t face_num;
    fdr_ae_face face_area[AE_FD_NUM];
} fdr_ae_fd_param;

typedef struct {
    uint32_t* hist256;      /*!< histogram buffer pointer */
    uint8_t* img;           /*!< gray image buffer pointer */
    int w;                  /*!< image width */
    int h;                  /*!< image height */
    int s;                  /*!< image buffer stride */
    fdr_ae_fd_param fd_param;
    uint16_t base_target_lum;
    uint16_t target_lum;
    uint32_t face_stable;
    uint32_t camera_id;
} fdr_stat_t;

typedef struct {
    int camera_id;
    int face_stable;
    int fdr_scene_num;
    int sceneChosen;
    int prop_dark;
    int prop_bright;
    int ev_pre;
    int ev_final;
    int cur_face_up;
    int face_up_enable;
    int up_ratio;
    int cur_face_down;
    int face_down_enable;
    int down_ratio;
    int ev_face;
} fdr_AE_exif_t;

typedef struct {
    void *tuning_param;
    fdr_stat_t stat;
} fdr_det_param_in_t;

typedef struct {
    float ev;
    int sensor_gain_underexp;
    int cur_bv_underexp;
    uint32_t exp_line_underexp;
    uint32_t total_gain_underexp;
    uint32_t exp_time_underexp;
    fdr_AE_exif_t fdr_AE_exif;
} fdr_det_param_out_t;

typedef struct {
    int smooth_flag;
    int frameID;
} fdr_det_status_t;

typedef struct {
    void *tuning_param;
    int BV;
} fdr_calc_frame_param_in_t;

typedef struct {
    int cur_total_frame_num;
    int ref_frame_num;
} fdr_calc_frame_param_out_t;

typedef struct {
    int last_total_frame_num;
} fdr_calc_frame_status_t;

typedef struct {
    fdr_det_param_out_t det_param;
    int sensor_gain_AE;
    int cur_bv_AE;
    uint32_t exp_line_AE;
    uint32_t total_gain_AE;
    uint32_t face_stable;
    uint16_t face_num;
    uint32_t exp_time_AE;
} fdr_scene_param_t;

typedef struct {
    int input_data_mode;//0:raw10, 1:raw14
} fdr_open_align_param;

typedef struct {
    int output_mode;//0:raw14x1, 1:raw10x2, 2:raw14x2
    int merge_mode;//0:fft, 1:gf
} fdr_open_merge_param;

typedef struct {
    int inputMode;//0:rgb14x1, 1:yuvx2, 2:yuvx1, 3:raw14
} fdr_open_fusion_param;

typedef struct {
    int max_width;
    int max_height;
    int run_type;//0:cpu, 2:vdsp acc
    fdr_open_align_param align_param;
    fdr_open_merge_param merge_param;
    fdr_open_fusion_param fusion_param;
    void *tuning_param;
    int tuning_param_size;
    int alignmerge_bypass; // 0:fdr, 1: for exposure fusion only
    int fusion_bypass; // reserved
} fdr_open_param;

typedef struct {
    int total_frame_num;
    int ref_frame_num;
} fdr_align_init_param;

typedef struct {
    int blc_r;
    int blc_gr;
    int blc_gb;
    int blc_b;
    int bayer_mode;
    fdr_scene_param_t scene_param;
} fdr_merge_param;

typedef struct {
    int gain;
    int bin0;
    int nlm_out_ratio0;
    int nlm_out_ratio1;
    int nlm_out_ratio2;
    int nlm_out_ratio3;
    int nlm_out_ratio4;
} fdr_merge_out_param;

typedef struct {
    int bin0;
    int fusion_gain;
    fdr_scene_param_t scene_param;
    int blc_r;
    int blc_gr;
    int blc_gb;
    int blc_b;
    int bayer_mode;
} expfusion_proc_param_t;

typedef struct {
    int maxGain;
    int minGain;
    int gain_ae_limit;
    int gain_min;
    int gain_high;
    int gain;
    int binMin;
    int bin50;
    int bin0;
    int target_high;
    int target_min;
    int sensor_gain_AE;
    int total_gain_AE;
    int cur_bv_AE;
    uint32_t exp_time_AE;
    int sensor_gain_underexp;
    int total_gain_underexp;
    int cur_bv_underexp;
    uint32_t exp_time_underexp;
    int total_frame_num;
    int ref_frame_num;
} fdr_merge_exif_t;

typedef struct {
    int version;
    fdr_AE_exif_t fdr_AE_exif;
    fdr_merge_exif_t fdr_merge_exif;
} fdr_exif_t;

#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT int sprd_fdr_get_version(fdr_lib_version* version);

/* sprd_fdr_scndet
Usage: Call it every frame to detect whether it is FDR scene.
Param:
    @tuning_param[in]
    @stat[in]
    @ev[out]
    @det_status[inout]: initial value must be 0.
*/
JNIEXPORT int sprd_fdr_scndet(fdr_det_param_in_t *param_in, fdr_det_param_out_t *param_out, fdr_det_status_t *det_status);

/* sprd_fdr_get_frame_num
Usage: Call it every frame to calculate fdr capture frame num and reference frame num.
Param:
    @tuning_param[in]
    @BV[in]
    @cur_total_frame_num[out]
    @ref_frame_num[out]
    @calc_status[inout]: initial value must be 0.
*/
JNIEXPORT int sprd_fdr_get_frame_num(fdr_calc_frame_param_in_t *param_in, fdr_calc_frame_param_out_t *param_out, fdr_calc_frame_status_t *calc_status);
JNIEXPORT int sprd_fdr_get_max_frame_num(int *max_total_frame_num, int *max_ref_frame_num);

JNIEXPORT int sprd_fdr_open(void **ctx, fdr_open_param *param);
JNIEXPORT int sprd_fdr_align_init(void *ctx, sprd_camalg_image_t *image_array, fdr_align_init_param *init_param);
JNIEXPORT int sprd_fdr_align(void *ctx, sprd_camalg_image_t *image);
JNIEXPORT int sprd_fdr_merge(void *ctx, sprd_camalg_image_t *image_array, fdr_merge_param *merge_param, fdr_merge_out_param *merge_out_param);
JNIEXPORT int sprd_fdr_fusion(void *ctx, sprd_camalg_image_t *image_in_array, sprd_camalg_image_t *image_out, expfusion_proc_param_t *param_proc);

/* sprd_fdr_get_exif
Usage: Call it after all process and before close to get exif info.
Param:
    @ctx[in]
    @fdr_exif[out]
*/
JNIEXPORT int sprd_fdr_get_exif(void *ctx, fdr_exif_t *fdr_exif);

JNIEXPORT int sprd_fdr_fast_stop(void *ctx);
JNIEXPORT int sprd_fdr_close(void **ctx);

#ifdef __cplusplus
}
#endif

#endif