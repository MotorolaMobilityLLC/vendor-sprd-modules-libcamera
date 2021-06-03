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
    uint8_t movement_inf;
} vsOutFrame;

typedef struct vs_param {
    uint16_t src_w;
    uint16_t src_h;
    uint16_t dst_w;
    uint16_t dst_h;
    int method;
    int camera_id;
    double f;
    double td;
    double ts;
    double wdx;
    double wdy;
    double wdz;
    float fov_loss;
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

typedef struct sprd_eis_multi_init_info {
	char board_name[36];
	char sensor_name[36];
	double f;
	double td;
	double ts;
	double fov_loss;
} sprd_eis_multi_init_info_t;

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
SPRD_ISP_API int video_stab_close(vsInst *inst);

const sprd_eis_init_info_t eis_init_info_tab[] = {
    {"sp9853i-1", 0.773f, 0.0177f, 0.012f},
    {"sp9860g-1", 1230.0f, 0.004f, 0.021f},
    {"sp9860g-3", 1160.0f, -0.01f, 0.024f},
    {"sp9861e-1", 1230.0f, 0.004f, 0.021f},
    {"sp9861e-2", 1230.0f, 0.004f, 0.021f},
    {"sp9863a-1", 0.7747f, 0.038f, 0.024f},
    {"ums312-1", 0.768f, 0.01f, 0.0144f},
    {"ums512-1", 0.7385f, 0.014f, 0.021f},
};

const sprd_eis_multi_init_info eis_multi_init_info_tab[] = {
    {"sp9863a-1", "imx351", 0.7766f, 0.0397f, 0.0224f, 0.25f},
    {"sp9863a-1", "ov5675_dual", 0.4701f, 0.0397f, 0.0219f, 0.25f},
    {"ums512-1", "ov32a1q", 0.7432f, 0.0289f, 0.0208f, 0.25f},
    {"ums512-1", "ov16885_normal", 0.4468f, 0.0297f, 0.0204f, 0.25f},
    {"ums512-1", "ov8856_shine", 1.5442f, 0.0308f, 0.0222f, 0.25f},
    {"ums9230-2", "imx586", 0.7415f, 0.0232f, 0.0242f, 0.25f},
    {"ums9230-2", "ov8856_back", 0.4735f, 0.0193f, 0.0231f, 0.25f},
    {"ums9230-2", "ov08a10_back", 2.2842f, 0.0167f, 0.0207f, 0.25f},
    {"ums9230-2-go", "imx586", 0.7354f, 0.0237f, 0.0247f, 0.25f},
    {"ums9230-3", "imx586", 0.7617f, 0.0236f, 0.0240f, 0.25f},
    {"ums9230-3", "ov8856_back", 0.4911f, 0.0246f, 0.0249f, 0.25f},
    {"ums9230-3", "ov08a10_back", 2.3375f, 0.0200f, 0.0208f, 0.25f},
    {"ums9230-3-go", "imx586", 0.7354f, 0.0237f, 0.0247f, 0.25f},
    {"ums9230-4", "imx586", 0.7617f, 0.0236f, 0.0240f, 0.25f},
    {"ums9230-4", "ov8856_back", 0.4911f, 0.0246f, 0.0249f, 0.25f},
    {"ums9230-4", "ov08a10_back", 2.3375f, 0.0200f, 0.0208f, 0.25f},
    {"ums9230-4-go", "imx586", 0.7354f, 0.0237f, 0.0247f, 0.25f},
};

#ifdef __cplusplus
}
#endif

#endif
