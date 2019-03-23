#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CNR_LEVEL 4
	typedef struct _version {
		uint8_t major;    /*!< API major version */
		uint8_t middle;   /*!< API middle version */
		uint8_t minor;    /*!<optimize verison>*/
	}LibVersion;


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

	typedef struct ThreadSet_t {
		unsigned int threadNum;
		unsigned int coreBundle;
	} ThreadSet;

	void *cnr_init(LibVersion * verInfo, ThreadSet threadInfo);
	int cnr(void *handle, CNR_Parameter *curthr, unsigned char * img,int w,int h);
	int cnr_destroy(void *handle);
	int cnr_get_version(LibVersion * verInfo);

#ifdef __cplusplus
}
#endif

#endif
