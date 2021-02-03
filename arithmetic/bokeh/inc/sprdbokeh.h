#ifndef __SPRDBOKEH_LIBRARY_HEADER_
#define __SPRDBOKEH_LIBRARY_HEADER_

typedef enum
{
	MODE_DUAL_CAMERA = 0,
	MODE_SING_CAMERA,
	MODE_EDIT,
}bokeh_mode; 

typedef struct{
	int productID;
	bokeh_mode mode;
}init_param;

typedef struct{
	unsigned short *near;
	unsigned short *far;
	unsigned char *confidence_map;
	unsigned char *depthnorm_data;
}gdepth_param;

#ifdef __cplusplus
extern "C" {
#endif
	int sprd_bokeh_userset(char * ptr,int size);

	int sprd_bokeh_Init(void **handle, int a_dInImgW, int a_dInImgH,  init_param *param);

	int sprd_bokeh_Close(void *handle);

	int sprd_bokeh_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

	int sprd_bokeh_ReFocusPreProcess(void *handle, void *a_pInBokehBufYCC420NV21, void *a_pInDisparityBuf16, int depthW, int depthH, void *param);

	int sprd_bokeh_ReFocusGen(void *handle, void *a_pOutBlurYCC420NV21, int a_dInBlurStrength, int a_dInPositionX, int a_dInPositionY);

	int sprd_bokeh_ReFocusGen_Portrait(void *handle, void *a_pOutBlurYCC420NV21, int a_dInBlurStrength, int a_dInPositionX, int a_dInPositionY);

	int sprd_bokeh_GdepthToDepth16(void *handle, gdepth_param *pInDisparityBuf, void *pOutDisparityBuf,int depthW ,int depthH);

#ifdef __cplusplus
}
#endif

#endif
