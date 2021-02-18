#ifndef __ISMOOTH_H__
#define __ISMOOTH_H__

#ifdef __linux__
#define JNIEXPORT __attribute__ ((visibility ("default")))
#else
#define JNIEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROI	10
typedef struct {
	int width;
	int height;
	float min_slope;
	float max_slope;
	float Findex2Gamma_AdjustRatio;
	int box_filter_size;
} InitGaussianParams;

typedef struct {

	int roi_type; // 0: circle 1:rectangle
	int F_number; // 1 ~ 20
	unsigned short sel_x; /* The point which be touched */
	unsigned short sel_y; /* The point which be touched */
	int CircleSize;

	// for rectangle
	int valid_roi;
	int x1[MAX_ROI], y1[MAX_ROI]; // left-top point of roi
	int x2[MAX_ROI], y2[MAX_ROI]; // right-bottom point of roi
	int flag[MAX_ROI]; //0:face 1:body
} WeightGaussianParams;


JNIEXPORT int iSmoothInit(void **handle, InitGaussianParams *param);

JNIEXPORT int iSmoothDeinit(void *handle);

JNIEXPORT int iSmoothCreateWeightMap(void *handle, WeightGaussianParams *params);

JNIEXPORT int iSmoothBlurImage(void *handle, void *Src_YUV, void *Output_YUV);

#ifdef __cplusplus
}
#endif

#endif
