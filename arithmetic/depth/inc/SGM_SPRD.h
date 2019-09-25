#ifndef __SGM_SPRD_H__
#define __SGM_SPRD_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_ROI 10

typedef enum { YUV420_NV12 = 0, YUV422_YUYV } ImageYUVFormat;
typedef enum { MODE_PREVIEW, MODE_CAPTURE } depth_mode;

typedef enum { MODE_DISPARITY, MODE_WEIGHTMAP } outFormat;

typedef enum { DISTANCE_OK = 0, DISTANCE_FAR, DISTANCE_CLOSE } distanceRet;

typedef enum {
    DEPTH_NORMAL = 0,
    DEPTH_STOP,
} depth_stop_flag;

struct depth_init_inputparam {
    int input_width_main;
    int input_height_main;
    int input_width_sub;
    int input_height_sub;
    int output_depthwidth;
    int output_depthheight;
    int online_depthwidth;
    int online_depthheight;
    int depth_threadNum;
    int online_threadNum;
    ImageYUVFormat imageFormat_main;
    ImageYUVFormat imageFormat_sub;
    void *potpbuf;
    int otpsize;
    char *config_param;
};

struct depth_init_outputparam {
    int outputsize;
    int calibration_width;
    int calibration_height;
};

struct af_golden_vcm_data {
    unsigned short golden_macro;
    unsigned short golden_infinity;
    unsigned short golden_count;
    unsigned short golden_distance[40];
    unsigned short golden_vcm[40];
    unsigned short reserved[10];
};

struct portrait_mode_param {
    int face_num;  //
    int mRotation; // counter clock-wise. 0:face up body down 90:face left body
                   // right 180:face down body up 270:face right body left  ->
    int portrait_en; // 1:portrait mode 0:normal mode

    int valid_roi;                //                                                                  ->                                                                  ->1.2
    int x1[NUM_ROI], y1[NUM_ROI]; // left-top point of roi
                                  // ->1.2 only face input max 5
    int x2[NUM_ROI], y2[NUM_ROI]; // right-bottom point of roi
                                  // ->1.2 only face input max 5
    int flag[NUM_ROI]; // 0:face 1:body
    bool rear_cam_en; // 1:rear camera capture 0:front camera capture ->
    short camera_angle;
    short mobile_angle;
};
typedef struct {
    int F_number; // 1 ~ 20
    int sel_x;    /* The point which be touched */
    int sel_y;    /* The point which be touched */
    unsigned char *DisparityImage;
    int VCM_cur_value;
    struct af_golden_vcm_data golden_vcm_data;
    struct portrait_mode_param portrait_param;
} weightmap_param;

typedef struct DistanceTwoPointInfo {

    int x1_pos;
    int y1_pos;
    int x2_pos;
    int y2_pos;
} DistanceTwoPointInfo;

typedef struct {
    unsigned short near;
    unsigned short far;
    unsigned char *confidence_map;
    unsigned char *depthnorm_data;
} gdepth_outparam;

int sprd_depth_VersionInfo_Get(char a_acOutRetbuf[256],
                               unsigned int a_udInSize);

void sprd_depth_Set_Stopflag(void *handle, depth_stop_flag stop_flag);

void *sprd_depth_Init(struct depth_init_inputparam *inparam,
                      struct depth_init_outputparam *outputinfo,
                      depth_mode mode, outFormat format);

int sprd_depth_Run(void *handle, void *a_pOutDisparity, void *a_pInMaptable,
                   void *a_pInSub_YCC420NV21, void *a_pInMain_YCC420NV21,
                   weightmap_param *wParams);

int sprd_depth_Run_distance(void *handle, void *a_pOutDisparity,
                            void *a_pInMaptable, void *a_pInSub_YCC420NV21,
                            void *a_pInMain_YCC420NV21,
                            weightmap_param *wParams, distanceRet *distance);

int sprd_depth_OnlineCalibration(void *handle, void *a_pOutMaptable,
                                 void *a_pInSub_YCC420NV21,
                                 void *a_pInMain_YCC420NV21);

int sprd_depth_rotate(void *a_pOutDisparity, int width, int height, int angle);

int sprd_depth_distancemeasurement(int *distance, void *disparity,
                                   DistanceTwoPointInfo *points_info);

int sprd_depth_Close(void *handle);

int sprd_depth_OnlineCalibration_postprocess(void *handle, void *a_pOutMaptable,
                                             void *a_pOutMaptable_scale);

int sprd_depth_get_gdepthinfo(void *handle, void *a_pOutDisparity,
                              gdepth_outparam *gdepth_output);

int sprd_depth_userset(char *ptr, int size);

#ifdef __cplusplus
} // extern C
#endif

#endif
