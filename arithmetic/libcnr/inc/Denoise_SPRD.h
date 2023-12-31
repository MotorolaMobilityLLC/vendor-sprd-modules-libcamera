#ifndef _DENOISE_SPRD_H_
#define _DENOISE_SPRD_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LAYER_NUM (5)
#define CNR_LEVEL 4

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

/*****************************ynr***************************************/
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

	typedef struct {
		void *data;
		int fd;
	} sprd_ynr_buffer_vdsp;

/*****************************cnr2***************************************/
	typedef struct filter_Weights
	{
		unsigned char distWeight[9];   //distance weight for different scale
		unsigned char rangWeight[128]; //range weight for different scale
	}filterParam;

	typedef struct CNR_Parameter_t
	{
		unsigned char filter_en[CNR_LEVEL]; //enable control of filter
		unsigned char rangTh[CNR_LEVEL][2]; //threshold for different scale(rangTh[CNR_LEVEL][0]:U threshold, rangTh[CNR_LEVEL][1]:V threshold)
		filterParam wTable[CNR_LEVEL][2]; //weight table(wTable[CNR_LEVEL][0]:U weight table, wTable[CNR_LEVEL][1]:V weight table) 
	} CNR_Parameter;

/*****************************cnr3***************************************/
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
	} cnr_param_t;

/************************common*******************************/
	typedef struct Denoise_Parameter_t
	{
		YNR_Param *ynrParam; 
		CNR_Parameter *cnr2Param; //CNR2.0
		cnr_param_t *cnr3Param;   //CNR3.0
	} Denoise_Param;

	/*! version information */
	typedef struct _tag_nr_version_t
	{
		unsigned char		major;              /*!< API major version */
		unsigned char		minor;              /*!< API minor version */
		unsigned char		micro;              /*!< API micro version */
		unsigned char		nano;               /*!< API nano version */
		int                 bugid;              /*!< API bugid */
		char		built_date[0x20];   /*!< API built date */
		char		built_time[0x20];   /*!< API built time */
		char		built_rev[0x100];	/*!< API built version, linked with vcs resivion> */
	} nr_version_t;
	
	void *sprd_cnr_init(int width, int height, int cnr_runversion);
	int sprd_cnr_process(void *handle, denoise_buffer *imgBuffer, Denoise_Param *paramInfo, denoise_mode mode, int width, int height);
	int sprd_cnr_deinit(void *handle);
	int nr_get_version(nr_version_t * version);

	void *sprd_cnr_init_vdsp(int width, int height, int cnr_runversion);
	int sprd_cnr_process_vdsp(void *handle, denoise_buffer_vdsp *imgBuffer, Denoise_Param *paramInfo, denoise_mode mode, int width, int height);
	int sprd_cnr_deinit_vdsp(void *handle);

#ifdef __cplusplus
}
#endif

#endif