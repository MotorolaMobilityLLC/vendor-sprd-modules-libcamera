#ifndef __SPRD_HDR3_API_H__
#define __SPRD_HDR3_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MAX_CHANNEL_NUM 3
#define HDR3_FACE_NUM_MAX        10

typedef enum {
    RGB_8 = 0x8,
    RGB_10 = 0xa,
    RGB_14 = 0xe,
    Y_UV_420 = 0x10
} Image_Type_t;

/*! hdr statistic parameters */
typedef struct
{
    int p_smooth_flag;
    int p_frameID;
} hdr3_stat_t;

/*! hdr_detect parameters */
typedef struct
{
    uint32_t*   hist;            /*!< histogram buffer pointer */
    int w;
    int h;
    int         bv;
    uint8_t     face_num;
    uint16_t    abl_weight;
    uint32_t    iso;
    void*       tuning_param;
    int         tuning_param_size;
} hdr3_detect_t;

typedef struct
{
    float ev[7];
    int tuning_param_index;
    uint64_t clock;
} hdr3_detect_out_t;

typedef struct
{
    int  startX;
    int  startY;
    int  width;
    int  height;
    int  yawAngle;
    int  rollAngle;//
    int  score;
    int humanID;
    int reserve[8];
} sprd_hdr3_face_info_t;

typedef struct {
void *data[MAX_CHANNEL_NUM];
    int width;
    int height;
    int ion_fd;
    int stride;
    int type;
    float ev;
} hdr3_Image_t;

typedef struct {
    void *tuning_param;
    int tuning_size;
    void*    (*malloc)(size_t size, char* type);
    void     (*free)(void* addr);
} hdr3_config_t;

typedef struct {
    sprd_hdr3_face_info_t  face_info[HDR3_FACE_NUM_MAX];
    int face_num;
    int tuning_param_index;
    int input_frame_num;
    uint64_t clock;
} hdr3_param_info_t;

int sprd_hdr3_scndet(hdr3_detect_t* scndet, hdr3_detect_out_t* scence_out, hdr3_stat_t* stat);

int sprd_hdr3_init(void **handle, int max_width, int max_height, void *init_param);

int sprd_hdr3_run(void *handle, hdr3_Image_t *input, hdr3_Image_t *output, void *param);

void sprd_hdr3_fast_stop(void *handle);

int sprd_hdr3_deinit(void **handle);

#ifdef __cplusplus
}
#endif

#endif