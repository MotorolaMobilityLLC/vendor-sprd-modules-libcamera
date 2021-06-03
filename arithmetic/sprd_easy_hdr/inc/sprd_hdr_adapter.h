#ifndef __SPRD_HDR_ADAPTER_HEADER_H__
#define __SPRD_HDR_ADAPTER_HEADER_H__

#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))
#define HDR_IMG_NUM_MAX    (3+4)
#define HDR_FACE_NUM_MAX        10


typedef enum {
    SPRD_HDR_GET_VERSION_CMD = 0,
    SPRD_HDR_PROCESS_CMD,
    SPRD_HDR_FAST_STOP_CMD,
    SPRD_HDR_MAX_CMD,
    SPRD_HDR_GET_EXIF_SIZE_CMD,
    SPRD_HDR_GET_EXIF_INFO_CMD
} sprd_hdr_cmd_t;

typedef struct {
    uint8_t    major;              /*!< API major version */
    uint8_t    minor;              /*!< API minor version */
    uint8_t    micro;              /*!< API micro version */
    uint8_t    nano;               /*!< API nano version */
    char    built_date[0x20];    /*!< API built date */
    char    built_time[0x20];    /*!< API built time */
    char    built_rev[0x100];    /*!< API built version, linked with vcs resivion> */
} sprd_hdr_version_t;

typedef struct
{
    int p_smooth_flag;
    int p_frameID;
}sprd_hdr_status_t;

typedef struct
{
    uint32_t* hist;
    uint8_t*  img;
    int w;
    int h;
    int s;
    int bv;
    uint8_t face_num;
    uint16_t abl_weight;
    uint32_t iso;
    void* tuning_param;
    int tuning_size;
}sprd_hdr_detect_in_t;

typedef struct
{
    float       ev[HDR_IMG_NUM_MAX];
    uint32_t    shutter[HDR_IMG_NUM_MAX];
    uint32_t    gain[HDR_IMG_NUM_MAX];
    int         frame_num;
    int         tuning_param_index;
    uint64_t    clock;
    int         bv;
    uint32_t    face_num;
    uint32_t    prop_dark;
    uint32_t    prop_bright;
    uint32_t    sceneChosen;
    int         scene_flag;
}sprd_hdr_detect_out_t;

typedef struct
{
    int startX;
    int starty;
    int width;
    int height;      /* Face rectangle*/
    int yawAngle;    /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;  */
    int rollAngle;   /* In-plane rotation angle (Roll); In (-180, +180] degrees;   */
    int score;       /* Confidence score; In [0, 1000]*/
    int humanId;     /* Human ID Number*/
    int reserv[8];   //For extend
}sprd_hdr_face_info_t;

typedef struct {
    struct sprd_camalg_image input[HDR_IMG_NUM_MAX];//hdr2: use input[0]&[1]
    int input_frame_num;
    struct sprd_camalg_image output;
    sprd_hdr_face_info_t  face_info[HDR_FACE_NUM_MAX];
    int face_num;
    sprd_hdr_detect_out_t *detect_out;
    void *bayerHist;
    float  ev[HDR_IMG_NUM_MAX];//hdr2: use ev[0]&[1]
    void*  callback;
    void*  sensor_ae;
} sprd_hdr_param_t;

typedef struct {
    int      tuning_param_size;
    void*    tuning_param;
    void*    (*malloc)(size_t size, char* type);
    void     (*free)(void* addr);
} sprd_hdr_init_param_t;

typedef struct {
    int      exif_size;
} sprd_hdr_get_exif_size_param_t;

typedef struct {
    void*    exif_info;
} sprd_hdr_get_exif_info_param_t;
/*
    hdr adapter scene detection instance
    return value: 0 is ok, other value is failed;
    @detect_in_param: input detect parameter
    @detect_out_param: output detect parameter
*/
JNIEXPORT int sprd_hdr_adpt_detect(sprd_hdr_detect_in_t *detect_in_param, sprd_hdr_detect_out_t *detect_out_param, sprd_hdr_status_t *status);

/*
    init hdr adapter instance
    return value: handle;
    @max_width/height: max supported width/height
    @param: reserved, pass 0 is ok
*/
JNIEXPORT void *sprd_hdr_adpt_init(int max_width, int max_height, void *param);

/*
    deinit hdr adapter instance
    return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_hdr_adpt_deinit(void *handle);

/*
    hdr adapter cmd process interface
    return value: 0 is ok, other value is failed
    @param: depend on cmd type:
    - SPRD_HDR_GET_VERSION_CMD: sprd_hdr_version_t
    - SPRD_HDR_PROCESS_CMD: sprd_hdr_param_t
    - SPRD_HDR_FAST_STOP_CMD: 0
*/
JNIEXPORT int sprd_hdr_adpt_ctrl(void *handle, sprd_hdr_cmd_t cmd, void *param);

/*
    hdr adapter get running type, the output is type, such as cpu/gpu/vdsp
    return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_hdr_get_devicetype(enum camalg_run_type *type);

/*
    hdr adapter set running type
    return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_hdr_set_devicetype(enum camalg_run_type type);

#ifdef __cplusplus
}
#endif

#endif
