#ifndef _SPRD_EIS_H_
#define _SPRD_EIS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { double dat[3][3]; } mat33;

typedef struct frame_in {
    uint8_t *frame_data;
    double timestamp;
    double zoom;
} vsInFrame;

typedef struct frame_out {
    uint8_t *frame_data;
    mat33 warp;
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

void video_stab_param_default(vsParam *param);
void video_stab_open(vsInst *inst, vsParam *param);
int video_stab_read(vsInst inst, vsOutFrame *frame);
void video_stab_write_frame(vsInst inst, vsInFrame *frame);
void video_stab_write_gyro(vsInst inst, vsGyro *gyro, int gyro_num);
void video_stab_close(vsInst inst);

const sprd_eis_init_info_t eis_init_info_tab[] = {
    {"sp9860g-1", 1230.0f, 0.004f, 0.021f},
    {"sp9860g-3", 1160.0f, -0.01f, 0.024f}};

#ifdef __cplusplus
}
#endif

#endif
