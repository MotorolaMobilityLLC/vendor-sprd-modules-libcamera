#ifndef _DENOISE_INTERFACE_H_
#define _DENOISE_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CNR_LEVEL 4
#define LAYER_NUM 5
	typedef enum
	{
		MODE_YNR = 1,
		MODE_CNR2,
		MODE_YNR_CNR2,
		MODE_CNR3,
		MODE_YNR_CNR3,
		MODE_CNR2_CNR3,
		MODE_YNR_CNR2_CNR3
	}denoise_mode; 

	typedef struct _tag_ldr_image_t
	{
		unsigned char *data;
		int         imgWidth;
		int         imgHeight;
	} ldr_image_t;

	typedef struct
	{
		ldr_image_t image;
		int			fd;
	} ldr_image_vdsp_t;

	typedef struct ThreadSet_t {
		unsigned int threadNum;
		unsigned int coreBundle;
	} ThreadSet;

	typedef struct denoise_buffer_t
	{
		unsigned char *bufferY;
		unsigned char *bufferUV;
	} denoise_buffer;

	typedef struct denoise_buffer_vdsp_t{
		unsigned char *bufferY;
		unsigned char *bufferUV;
		int fd_Y;
		int fd_UV;
	} denoise_buffer_vdsp;

	//YNR param
	typedef struct YNR_Parameter_t
	{
		unsigned char ynr_lumi_thresh[2];
		unsigned char ynr_gf_rnr_ratio[5];
		unsigned char ynr_gf_addback_enable[5];
		unsigned char ynr_gf_addback_ratio[5];
		unsigned char ynr_gf_addback_clip[5];
		unsigned short ynr_Radius;
		unsigned short ynr_imgCenterX;
		unsigned short ynr_imgCenterY;
		unsigned short ynr_gf_epsilon[5][3];
		unsigned short ynr_gf_enable[5];
		unsigned short ynr_gf_radius[5];
		unsigned short ynr_gf_rnr_offset[5];
		unsigned short ynr_bypass;
		unsigned char reserved[2];		
	} YNR_Param;

	//CNR2.0param
	typedef struct filter_Weights
	{
		unsigned char distWeight[9];   
		unsigned char rangWeight[128]; 
	} filterParam; 

	typedef struct CNR_Parameter_t
	{
		unsigned char filter_en[CNR_LEVEL];		
		unsigned char rangTh[CNR_LEVEL][2];		
		filterParam wTable[CNR_LEVEL][2];
	} CNR_Parameter;//CNR2_Param; 

	//CNR3.0 param
	typedef struct _tag_multilayer_param_t
	{
		unsigned char lowpass_filter_en;
		unsigned char denoise_radial_en;
		unsigned char order[3];
		unsigned short imgCenterX;
		unsigned short imgCenterY;
		unsigned short slope;
		unsigned short baseRadius;
		unsigned short minRatio;
		unsigned short luma_th[2];
		float sigma[3];
	}multiParam;

	typedef struct _tag_cnr_param_t
	{
		unsigned char bypass;
		multiParam paramLayer[LAYER_NUM];
	} cnr_param_t; //CNR3_Param;

	typedef struct Denoise_Parameter_t
	{
		YNR_Param *ynrParam; 
		CNR_Parameter *cnr2Param; //CNR2.0
		cnr_param_t *cnr3Param;   //CNR3.0
	} Denoise_Param;

	void *sprd_cnr_init(int width, int height, int cnr_runversion);
	int sprd_cnr_process(void *handle, denoise_buffer *imgBuffer, Denoise_Param *paramInfo, denoise_mode mode, int width, int height);
	int sprd_cnr_deinit(void *handle);

	void *sprd_cnr_init_vdsp(int width, int height, int cnr_runversion);
	int sprd_cnr_process_vdsp(void *handle, denoise_buffer_vdsp *imgBuffer, Denoise_Param *paramInfo, denoise_mode mode, int width, int height);
	int sprd_cnr_deinit_vdsp(void *handle);

#ifdef __cplusplus
}
#endif

#endif
