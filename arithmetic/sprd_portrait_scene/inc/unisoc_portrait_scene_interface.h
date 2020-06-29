#ifndef UNISOC_PORTRAIT_SCENE_INTF_H__
#define UNISOC_PORTRAIT_SCENE_INTF_H__

/*********************************************
 * change log
 * initial version		    yucai.huang 20200220
 * correct slip of pen      yucai.huang 20200224
 * merge the preview and capture library common interface yucai.huang 20200302
 * add chaneg filed ch in init struct yucai.huang20100303
 * update for capture library yucai.huang20100304
 * add color retention flag yucai.huang20100311
 *
 *********************************************/

#if (defined WIN32 || defined REALVIEW)
#define JNIEXPORT
#else
#ifndef JNIEXPORT 
#define JNIEXPORT  __attribute__ ((visibility ("default")))
#endif
#endif

//FIXME should add macro to indirectly including file
#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PORTRAIT_SCENE_MAX_CNT 10
#define MAX_ROI 10

typedef enum sprd_portrait_scene_channel{
    SPRD_PORTRAIT_SCENE_PREVIEW = 1,
    SPRD_PORTRAIT_SCENE_CAPTURE,
    SPRD_PORTRAIT_SCENE_VIDEO
}sprd_portrait_scene_channel_t;


typedef enum sprd_image_file_format {
    IMG_FILE_FORMAT_YUV = 1,
    IMG_FILE_FORMAT_BMP,
    IMG_FILE_FORMAT_PNG,
    IMG_FILE_FORMAT_JPG,
    IMG_FILE_FORMAT_MAX
} sprd_image_file_format_t;

typedef struct sprd_portrait_scene_init {
    sprd_portrait_scene_channel_t ch;
	int32_t libLevel;
	int16_t productID;     //system or product information, 0, sharkle pike2, 1, sharkl3 sharkl5
	float min_slope; //0.001~0.01, default is 0.005	->0.0004
	float max_slope; //0.01~0.1, default is 0.05	->0.0019
	uint16_t slope; /* Range is [0,256]. Default is 0 */
	float Findex2Gamma_AdjustRatio; //2~11, default is 6.0	->15.0f
	int32_t Scalingratio;//only support 2,4,6,8	->8 for input 5M(1952)
	int32_t SmoothWinSize;//odd number	->5
	int32_t width;  // image width	->
	int32_t height; // image height	->
	int32_t box_filter_size;//odd number

	float gau_min_slope;
	float gau_max_slope;
	float gau_Findex2Gamma_AdjustRatio;
	int32_t gau_box_filter_size;

	int16_t isFrontCamera; //1 is front camera, 0 back camera
    enum camalg_run_type run_type;  //operation platform, CPU or VDSP
    void *handle;
	int mask_w;
	int mask_h;
	int mask_size;
} sprd_portrait_scene_init_t;



typedef struct sprd_portrait_scene_proc {
    sprd_portrait_scene_channel_t ch;
	int32_t version;  //1~2, 1: 1.0 bokeh; 2: 2.0 bokeh with AF	->1
	int32_t roi_type; // 0: circle 1:rectangle 2:seg	->2
	int32_t f_number; // 1 ~ 20	->
	int32_t sel_x;    // The point32_t which be touched	->
	int32_t sel_y;    // The point32_t which be touched	->

	// for version 1.0 
	int32_t circle_size;
	
	//detect info:
	int32_t valid_roi;//->
	int32_t total_roi;//	->1.2
	int32_t x1[MAX_ROI], y1[MAX_ROI]; // left-top point32_t of roi		->1.2 only face input max 5
	int32_t x2[MAX_ROI], y2[MAX_ROI]; // right-bottom point32_t of roi	->1.2 only face input max 5
	int32_t flag[MAX_ROI]; //0:face 1:body
	int8_t  rear_cam_en; //1:rear camera capture 0:front camera capture 
	int16_t rotate_angle; // counter clock-wise. 0:face up body down 90:face left body right 180:face down body up 270:face right body left
	int16_t camera_angle;
	int16_t mobile_angle;
	uint8_t isCapture; //1
	uint8_t isFrontCamera; //1 is front camera, 0 back camera
	void * ptr1;         //reserve
	void * ptr2;         //reserve
} sprd_portrait_scene_proc_t;



typedef struct sprd_portrait_scene_fuse {
    sprd_portrait_scene_channel_t ch;
    bool isColorRetention;
    int32_t index;//build-in-scene id
    int32_t rotate_angle;//rotate angle base horizontal 0
	uint8_t isFrontCamera; //1 is front camera, 0 back camera
    int32_t total;//num of current loaded scene, <= PORTRAIT_SCENE_MAX_CNT
    int8_t *data;//scene buff，whose format is RGBRGB，without header of photo
    int32_t width[PORTRAIT_SCENE_MAX_CNT];//width of current scene img
    int32_t height[PORTRAIT_SCENE_MAX_CNT];//height of current scene img
    sprd_image_file_format_t fileFormat[PORTRAIT_SCENE_MAX_CNT];//format of scene file in SD card
    void *yuv420[PORTRAIT_SCENE_MAX_CNT];//scene file will be converted to yuv format, and save here
	uint16_t *mask;
} sprd_portrait_scene_fuse_t;

typedef struct BgrpVersion_t{
    uint8_t       major;              /*!< API major version */
    uint8_t       minor;              /*!< API minor version */
    uint8_t       micro;              /*!< API micro version */
    uint8_t       nano;               /*!< API nano version */
    uint32_t      bugid;              /*!< API bugid */
    char          built_date[0x20];   /*!< API built date */
    char          built_time[0x20];   /*!< API built time */
    char          built_rev[0x100];   /*!< API built version, linked with vcs resivion> */
} bgrp_version_t;

JNIEXPORT int32_t unisoc_portrait_scene_preview_get_version(bgrp_version_t *version);
JNIEXPORT int32_t unisoc_portrait_scene_capture_get_version(bgrp_version_t *version);
/********************************************
	Func name: unisoc_portrait_scene_preview_init

	Purpose & Use: 
		1. 初始化参数和中间内存
		2. 不改变尺寸只初始化一次即可.

	Parameters:
		handle: 传入二级指针，函数内分配内存
		param:
	Returned value:
		Zero: success
		Non-Zero: fail
********************************************/
JNIEXPORT int32_t unisoc_portrait_scene_preview_init(void **handle, sprd_portrait_scene_init_t *param);
JNIEXPORT int32_t unisoc_portrait_scene_capture_init(void **handle, sprd_portrait_scene_init_t *param);



/********************************************
	Func name: unisoc_portrait_scene_preview_deinit

	Purpose & Use: 
		1. 释放中间内存
		2. 处理结束或重新初始化前需要调用一次

	Parameters:
		handle: 函数内释放内存
        param:

	Returned value:
		Zero: success
		Non-Zero: fail
********************************************/
JNIEXPORT int32_t unisoc_portrait_scene_preview_deinit(void *handle);
JNIEXPORT int32_t unisoc_portrait_scene_capture_deinit(void *handle);



/********************************************
	Func name: unisoc_portrait_scene__preview_create_weight_map

	Purpose & Use: 
		1. 根据sel point和F，创建weight map
		2. 如果sel point和F不变，不需要每帧都调用

	Parameters:
		handle:
                param:

	Returned value:
		Zero: success
		Non-Zero: fail
********************************************/
JNIEXPORT int32_t unisoc_portrait_scene_preview_create_weight_map(void *handle, sprd_portrait_scene_proc_t *param);

JNIEXPORT int32_t unisoc_portrait_scene_capture_create_weight_map(void *handle, sprd_portrait_scene_proc_t *param);

/********************************************
	Func name: unisoc_portrait_scene_preview_process

	Purpose & Use: 
		1. 抠出前景mask
		2. 每帧调用一次

	Parameters:
		handle:
		src_yuv: 由用户分配内存, 大小为width * height * 3 / 2 * sizeof(char)，只支持y uv 420
		dst_yuv: 由用户分配内存, 大小为width * height * 3 / 2 * sizeof(char)，只支持y uv 420
					如果dst_yuv为空，将使用src_yuv作为输出buffer

	Returned value:
		Zero: success
		Non-Zero: fail
********************************************/
JNIEXPORT int32_t unisoc_portrait_scene_preview_process(void *handle, uint8_t *src_yuv, uint8_t *dst_yuv,uint16_t *mask);

JNIEXPORT int32_t unisoc_portrait_scene_capture_process(void *handle, uint8_t *src_yuv, uint8_t *dst_yuv,uint16_t *mask);

/********************************************
	Func name: unisoc_portrait_scene_preview_fuse_img

	Purpose & Use: 
		1. 前景与背景融合
		2. 每帧调用一次

	Parameters:
		handle:
		src_yuv: 由unisoc_portrait_scene_preview_process调用产生NN Mask
		dst_yuv: 由src_yuv同背景YUV融合产生目标YUV;如果dst_yuv为空，将使用src_yuv作为输出buffer
		bginfo：背景图片信息
	Returned value:
		Zero: success
		Non-Zero: fail
********************************************/
JNIEXPORT int32_t unisoc_portrait_scene_preview_fuse_img(void *handle, uint8_t *src_yuv, uint8_t *dst_yuv, sprd_portrait_scene_fuse_t *param);

JNIEXPORT int32_t unisoc_portrait_scene_capture_fuse_img(void *handle, uint8_t *src_yuv, uint8_t *dst_yuv, sprd_portrait_scene_fuse_t *param);

#ifdef __cplusplus
}
#endif

#endif	//UNISOC_PORTRAIT_SCENE_INTF_H__
