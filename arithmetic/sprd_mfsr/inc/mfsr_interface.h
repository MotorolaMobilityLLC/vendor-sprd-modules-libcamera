#ifndef __MFSR_INTERFACE_H__
#define __MFSR_INTERFACE_H__

#include "sprd_camalg_adapter.h"

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

typedef struct {
    uint8_t     major;              /*!< lib major version */
    uint8_t     minor;              /*!< lib minor version */
    uint8_t     micro;              /*!< lib micro version */
    uint8_t     nano;               /*!< lib nano version */
    uint32_t    bug_id;             /*!< lib bug id */
    char        built_date[0x20];   /*!< lib built date */
    char        built_time[0x20];   /*!< lib built time */
    char        built_rev[0x100];   /*!< lib built version, linked with vcs resivion> */
} mfsr_lib_version;

typedef struct {
    int input_data_mode;//0:raw10, 1:raw14 2:nv21
} mfsr_open_align_param;

typedef struct {
    int max_width;
    int max_height;
    int run_type;//0:cpu, 2:vdsp acc
    mfsr_open_align_param align_param;
    void *tuning_param;
    int tuning_param_size;
    int sr_mode; //0:auto, 1:HD
    int max_width_out;//for hd mode
    int max_height_out;
} mfsr_open_param;

typedef struct {
    int version;
    uint8_t merge_num;
    uint8_t sharpen_field;
    uint8_t sg_contrast_pos;
    uint8_t clip_contrast_min;
    uint8_t clip_contrast_max;
    uint8_t denoise_field[2];
    uint8_t reserved;
    int im_input_w;     //input image width
    int im_input_h;     //input image height
    int im_output_w;    //output image width
    int im_output_h;    //output image height
    int scale;        //zoom ratio
    int gain;
    int exposure_time;
    int sharpen_level;
    int sharpen_threshold[2];
    int detail_threshold;
    int deghost_scale;
    int deghost_threshold;
    int deghost_maxscale;
    int deghost_shift;
    int sg_contrast_bw[2];
    int clip_contrast_bw[2];
    int saturation_scale[3];
    int denoise_level[2];
} mfsr_exif_t;

typedef struct {
    uint8_t max_total_frame_num;
} mfsr_capbility_value;

typedef enum {
    MFSR_EXPO_MODE_0_MAX_FRAME,
} mfsr_capbility_key;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} mfsr_crop_param;

typedef struct {
    float gain;
    float exposure_time;
    mfsr_crop_param crop_area;
} mfsr_process_param;

typedef struct {
    void *tuning_param;
    float gain;
    int input_w;
    int crop_area_w;
} mfsr_calc_frame_param_in_t;

typedef struct {
    int cur_total_frame_num;
} mfsr_calc_frame_param_out_t;

typedef struct {
    int frameID;
    int last_total_frame_num;
} mfsr_calc_frame_status_t;


#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT int sprd_mfsr_get_capbility(mfsr_capbility_key key, mfsr_capbility_value *value);

JNIEXPORT int sprd_mfsr_get_version(mfsr_lib_version* version);

JNIEXPORT int sprd_mfsr_open(void **ctx, mfsr_open_param *param);
/* sprd_mfsr_get_frame_num
Usage: Call it every frame to calculate mfsr capture frame num.
Param:
    @tuning_param[in]
    @gain[in]
    @cur_total_frame_num[out]
    @calc_status[inout]: initial value must be 0.
*/
JNIEXPORT int sprd_mfsr_get_frame_num(mfsr_calc_frame_param_in_t *param_in, mfsr_calc_frame_param_out_t *param_out, mfsr_calc_frame_status_t *calc_status);

JNIEXPORT int sprd_mfsr_process(void *ctx, sprd_camalg_image_t *image_input,sprd_camalg_image_t *image_output,sprd_camalg_image_t *detail_mask,sprd_camalg_image_t *denoise_mask,mfsr_process_param *process_param);
/* sprd_mfsr_get_exif
Usage: Call it after all process and before close to get exif info.
Param:
    @ctx[in]
    @mfsr_exif[out]
*/
JNIEXPORT int sprd_mfsr_get_exif(void *ctx, mfsr_exif_t *mfsr_exif);

JNIEXPORT int sprd_mfsr_fast_stop(void *ctx);
JNIEXPORT int sprd_mfsr_close(void **ctx);

JNIEXPORT int sprd_mfsr_post_open(void **ctx, mfsr_open_param *param);
JNIEXPORT int sprd_mfsr_post_process(void *ctx, sprd_camalg_image_t *image,sprd_camalg_image_t *detail_mask,sprd_camalg_image_t *denoise_mask,mfsr_process_param *process_param);
JNIEXPORT int sprd_mfsr_post_close(void **ctx);

#ifdef __cplusplus
}
#endif

#endif