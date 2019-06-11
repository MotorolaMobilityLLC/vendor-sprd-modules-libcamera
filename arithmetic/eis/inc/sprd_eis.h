#ifndef _SPRD_EIS_H_
#define _SPRD_EIS_H_

#ifndef _DATA_TYPE_H_
typedef struct { double dat[3][3]; } mat33;
#endif

typedef struct frame_in {
    uint8_t *frame_data;
    double timestamp;
    double ae_time;
    double zoom;
    uint32_t frame_num;
} vsInFrame;

typedef struct frame_out {
    uint8_t *frame_data;
    mat33 warp;
    uint32_t frame_num;
    double timestamp;
} vsOutFrame;

typedef struct vs_param {
    uint16_t src_w;
    uint16_t src_h;
    uint16_t dst_w;
    uint16_t dst_h;
    int method;

    double f;
    double td;
    double ts;
    double wdx;
    double wdy;
    double wdz;
} vsParam;

typedef struct gyro_vs {
    double t;
    double w[3];
} vsGyro;

typedef void *vsInst;

typedef struct sprd_eis_init_info {
    char board_name[36];
    double f;
    double td;
    double ts;
} sprd_eis_init_info_t;

typedef struct eis_info {
    float zoom_ratio;
    int64_t timestamp;
} eis_info_t;

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
#define SPRD_ISP_API
#endif

#ifdef __cplusplus
extern "C" {
#endif
// return value 0: success  -1:faild
SPRD_ISP_API int video_stab_param_default(vsParam *param);
// return value 0: success  -1:faild
SPRD_ISP_API int video_stab_open(vsInst *inst, vsParam *param);
// return value 0: success  -1:faild
SPRD_ISP_API int video_stab_write_frame(vsInst inst, vsInFrame *frame);
// return value 0: success  -1:faild
SPRD_ISP_API int video_stab_write_gyro(vsInst inst, vsGyro *gyro, int gyro_num);
// return value 0: gyro is ready  -1:faild 1:gyro is not ready
SPRD_ISP_API int video_stab_check_gyro(vsInst inst);
// return value 0: success  -1: faild 1: no frame out
SPRD_ISP_API int video_stab_read(vsInst inst, vsOutFrame *frame);
// return value 0: success  -1:faild
SPRD_ISP_API int video_stab_close(vsInst inst);

const sprd_eis_init_info_t eis_init_info_tab[] = {
    {"sp9853i-1", 0.773f, 0.0177f, 0.012f},
    {"sp9860g-1", 1230.0f, 0.004f, 0.021f},
    {"sp9860g-3", 1160.0f, -0.01f, 0.024f},
    {"sp9861e-1", 1230.0f, 0.004f, 0.021f},
    {"sp9861e-2", 1230.0f, 0.004f, 0.021f},
    {"sp9863a-1", 0.7747f, 0.038f, 0.024f},
};

#ifdef __cplusplus
}
#endif

#endif
