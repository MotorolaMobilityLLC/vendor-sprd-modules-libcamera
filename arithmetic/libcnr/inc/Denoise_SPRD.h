#ifndef _DENOISE_INTERFACE_H_
#define _DENOISE_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CNR_LEVEL 4

	typedef enum
	{
		MODE_YNR,
		MODE_CNR,
		MODE_YNR_CNR
	}denoise_mode;

	typedef struct ThreadSet_t {
		unsigned int threadNum;
		unsigned int coreBundle;
	} ThreadSet;

	typedef struct denoise_buffer_t
	{
		unsigned char *bufferY;
		unsigned char *bufferUV;
	} denoise_buffer;

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

	typedef struct filter_Weights
	{
		unsigned char distWeight[9];   //distance weight for different scale
		unsigned char rangWeight[128]; //range weight for different scale
	} filterParam;

	typedef struct CNR_Parameter_t
	{
		unsigned char filter_en[CNR_LEVEL]; //enable control of filter
		unsigned char rangTh[CNR_LEVEL][2]; //threshold for different scale(rangTh[CNR_LEVEL][0]:U threshold, rangTh[CNR_LEVEL][1]:V threshold)
		filterParam wTable[CNR_LEVEL][2]; //weight table(wTable[CNR_LEVEL][0]:U weight table, wTable[CNR_LEVEL][1]:V weight table) 
	} CNR_Param;

	typedef struct Denoise_Parameter_t
	{
		YNR_Param *ynrParam;
		CNR_Param *cnrParam;
	} Denoise_Param;

	void *sprd_cnr_init(ThreadSet threadInfo);
	int sprd_cnr_process(void *handle, denoise_buffer *imgBuffer, Denoise_Param *paramInfo, denoise_mode mode, int width, int height);
	int sprd_cnr_deinit(void *handle);

#ifdef __cplusplus
}
#endif

#endif
