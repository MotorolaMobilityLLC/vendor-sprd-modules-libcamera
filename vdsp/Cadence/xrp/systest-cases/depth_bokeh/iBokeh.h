#ifndef __IBOKEH_H__
#define __IBOKEH_H__

#if (defined WIN32 || defined DMA_OPT)
#define JNIEXPORT 
#else
#define JNIEXPORT __attribute__ ((visibility("default")))
#endif

#define NO_GRAPHIC_INTERFACE

#ifdef NO_GRAPHIC_INTERFACE
#define GraphicBuffer	unsigned char
#else
#include "GraphicBuffer.h"
using namespace android;
#endif

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		int width;  // image width
		int height; // image height
		int depth_width;
		int depth_height;
		int SmoothWinSize;//odd number
		int ClipRatio; // RANGE 1:64
		int Scalingratio;//2,4,6,8
		int DisparitySmoothWinSize;//odd number
		//int HistMinNumberRatio;
		//int UpdatedMaxMinDiff;
	} InitParams;

	typedef struct {
		int F_number; // 1 ~ 20
		int sel_x; /* The point which be touched */
		int sel_y; /* The point which be touched */
		void *DisparityImage;
	} WeightParams;

	typedef struct {
		WeightParams *weightParams;
		int fd;
	} WeightParams_vdsp;

	typedef struct {
		GraphicBuffer *data; // 1 ~ 20
		int fd;
	} GraphicBuffer_vdsp;

	JNIEXPORT int iBokehUserset(char *ptr,int size);
	JNIEXPORT int iBokehInit(void **handle, InitParams *params);
	JNIEXPORT int iBokehDeinit(void *handle);
	JNIEXPORT int iBokehCreateWeightMap(void *handle, WeightParams *params);
	JNIEXPORT int iBokehBlurImage(void *handle, GraphicBuffer *Src_YUV, GraphicBuffer *Output_YUV);

#if defined (__linux__) || defined (WIN32)
	JNIEXPORT int iBokehInit_vdsp(void **handle, InitParams *params);
	JNIEXPORT int iBokehDeinit_vdsp(void *handle);
	JNIEXPORT int iBokehCreateWeightMap_vdsp(void *handle, WeightParams_vdsp *params);
	JNIEXPORT int iBokehBlurImage_vdsp(void *handle, GraphicBuffer_vdsp *Src_YUV, GraphicBuffer_vdsp *Output_YUV);
#endif

#ifdef __cplusplus
}
#endif

#endif
