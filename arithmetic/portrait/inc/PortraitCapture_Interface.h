#ifndef __PORTRAIT_CAPTURE_INTERFACE_H__
#define __PORTRAIT_CAPTURE_INTERFACE_H__

#define MAX_ROI 10

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IMG_YUV420_NV12 = 0,
    IMG_YUV422_YUYV,
    IMG_YUV_RAW8_Y
} ImageFormat;

typedef struct _proc_depth_input_data {
    // void *maptable;
    void *subMap;
    void *mainMap;
} ProcDepthInputMap;

typedef struct _inout_yuv {
    unsigned char *Src_YUV;
    unsigned char *Dst_YUV;
} InoutYUV;

typedef struct _portrait_af_golden_vcm_data {
    unsigned short golden_macro;
    unsigned short golden_infinity;
    unsigned short golden_count;
    unsigned short golden_distance[40];
    unsigned short golden_vcm[40];
    unsigned short reserved[10];
} portrait_af_golden_vcm_data;

struct SGMParamStruct {
    unsigned char SensorDirection;
    unsigned char DepthScaleMin;
    unsigned char DepthScaleMax;
    unsigned char CalibInfiniteZeroPt; // The Calibration Zero Point is Infinite
                                       // or Not, it is from Calibration File
    int SearchRange;     // Search Range
    int MinDExtendRatio; // Min Disparity Search Value Adjust Ratio
    int inDistance; // used for DistanceMeasureTips function
    int inRatio; // used for DistanceMeasureTips function
    int outDistance; // used for DistanceMeasureTips function
    int outRatio; // used for DistanceMeasureTips function
    float m_sigma;
    int use_post_processing;
    int maxd_sobel_ratio; // Used to calculate max_dist, MAX 64 ~1.0, for
                          // example 16 ~ 0.25;
    int block_sad_ratio; // ratio used to combine SAD and Census Difference: MAX
                         // 256
    int grad_dist_ratio; // ratio used to combine grad difference and block
                         // difference:MAX 4
    int max_sobel; // gradient is calculated by sobel operator. The gradient is
                   // clipped by max_sobel
    int mind_sobel; // minimum cost calculated by gradient difference
    int maxd_sobel; // maximum cost calculated by gradient difference
    int mind; // minimum cost calculated by pixel intensity difference. It has
              // two components: SAD and census transform
    int maxd; // maximum cost calculated by pixel intensity difference
};

typedef struct {
    int libLevel;
    int productInfo; // Product/Platform ID
    int calcDepth; // 0, not depth mode or used in gallary, 1 depth mode

    // single capture
    int width;  // image width
    int height; // image height
    int depthW; // depth width
    int depthH; // depth height

    float min_slope; // 0.001~0.01, default is 0.005 ->0.0004
    float max_slope; // 0.01~0.1, default is 0.05 ->0.0019
    float Findex2Gamma_AdjustRatio; // 2~11, default is 6.0 ->15.0f
    int Scalingratio; // only support 2,4,6,8 ->8 for input 5M(1952)
    int SmoothWinSize; // odd number ->5
    int box_filter_size; // odd number ->0

    // double capture
    int input_width_main;
    int input_height_main;
    int input_width_sub;
    int input_height_sub;
    ImageFormat imageFormat_main;
    ImageFormat imageFormat_sub;
    void *potpbuf;
    int otpsize;
    char *config_param;
} PortraitCap_Init_Params;

typedef struct _PortaitCapProcParams {
    // depth mode
    unsigned char *DisparityImage;
    int VCM_cur_value;
    portrait_af_golden_vcm_data golden_vcm_data;

    // gaussian mode
    int version; // 1:blur or portrait
    int roi_type; // 0: circle 1:rectangle 2:portrait
    int F_number;
    unsigned short sel_x;
    unsigned short sel_y;
    int CircleSize;
    int valid_roi;
    int total_roi;
    int x1[MAX_ROI];
    int y1[MAX_ROI];
    int x2[MAX_ROI];
    int y2[MAX_ROI];
    int flag[MAX_ROI];
    bool rear_cam_en; // 1:rear camera capture 0:front camera capture
    short rotate_angle; // 0:face up body down 90:face left body right 180:face
                        // down body up 270:face right body left
    short camera_angle;
    short mobile_angle;
} PortaitCapProcParams;

JNIEXPORT int sprd_portrait_capture_init(void **handle, PortraitCap_Init_Params *initParams);
JNIEXPORT int sprd_portrait_capture_deinit(void *handle);
JNIEXPORT int sprd_portrait_capture_get_mask(void *handle , ProcDepthInputMap *depthInputData, PortaitCapProcParams *procParams, InoutYUV *yuvData, void *WeightMask);
JNIEXPORT int sprd_portrait_capture_process(void *handle , InoutYUV *yuvData, void *WeightMask, int isCapture);
JNIEXPORT int sprd_portrait_capture_get_mask_info(void *handle, unsigned int *width, unsigned int *height, unsigned int *bufSize);
JNIEXPORT int sprd_portrait_capture_get_version(char *verInfo, int bufSize);

#ifdef __cplusplus
}
#endif

#endif
