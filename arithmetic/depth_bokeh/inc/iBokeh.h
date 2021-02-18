#ifndef __IBOKEH_H__
#define __IBOKEH_H__
#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;  		// image width
    int height; 		// image height
    int depth_width;
    int depth_height;
    int SmoothWinSize;          // odd number
    int ClipRatio;              // RANGE 1:64
    int Scalingratio;           // 2,4,6,8
    int DisparitySmoothWinSize; // odd number
} InitParams;

typedef struct {
    int F_number; // 1 ~ 20
    int sel_x;    /* The point which be touched */
    int sel_y;    /* The point which be touched */
    unsigned char *DisparityImage;
} WeightParams;

JNIEXPORT int iBokehInit(void **handle, InitParams *params);
JNIEXPORT int iBokehDeinit(void *handle);
JNIEXPORT int iBokehCreateWeightMap(void *handle, WeightParams *params);
JNIEXPORT int iBokehBlurImage(void *handle, void *Src_YUV, void *Output_YUV);

#ifdef __cplusplus
}
#endif

#endif
