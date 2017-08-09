#ifndef _3DNR_MODULE_H_
#define _3DNR_MODULE_H_
#include "interface.h"

#define T3DNR_PREVIEW_MODE  0	//only preview view, for each frame, 3ndr filtered image will be output
#define T3DNR_CAPTURE_MODE  1   //only output final 3DNR filtered image in fullsize
#define T3DNR_FULLSIZE_MODE 2   //process 5 fullsize image stored in DDR

#define CAPTURE_MODE_PREVIEW_STAGE 3
#define CAPTURE_MODE_CAPTURE_STAGE 4

#define FULLSIZE_MODE_PREVIEW_STAGE 4
#define FULLSIZE_MODE_FUSE_STAGE    5

#define T3DNR_CAPTURE_MODE_PREVIEW_STAGE (T3DNR_CAPTURE_MODE + CAPTURE_MODE_PREVIEW_STAGE)
#define T3DNR_CAPTURE_MODE_CAPTURE_STAGE (T3DNR_CAPTURE_MODE + CAPTURE_MODE_CAPTURE_STAGE)

#define T3DNR_FULLSIZE_MODE_PREVIEW_STAGE (T3DNR_FULLSIZE_MODE + FULLSIZE_MODE_PREVIEW_STAGE)
#define T3DNR_FULLSIZE_MODE_FUSE_STAGE (T3DNR_FULLSIZE_MODE + FULLSIZE_MODE_FUSE_STAGE)

typedef enum c3dnr_status
{
	C3DNR_STATUS_IDLE = 0,
	C3DNR_STATUS_INITED,
	C3DNR_STATUS_DEINITED,
	C3DNR_STATUS_BUSY,
}c3dnr_status_t;

typedef struct c3dnr_info
{
		uint8_t operation_mode;                //0: preview mode, 1: capture mode, 2: fullsize capture mode
		uint8_t fusion_mode;                   //1bit unsigned {0, 1} imge fusion mode:0: adaptive weight 1: fixed weight
		uint8_t MV_mode;                       //1bit unsigned {0, 1} MV_mode:0 MV calculated by integral projection 1: MV set by users
		int32_t *MV_x;                   //6bit signed [-32, 31] output MV_x calculated by integral projection
		int32_t *MV_y;                   //6bit signed [-32, 31] output MV_y calcualted by integral projection
		//int32_t MV_x_output;                   //6bit signed [-32, 31] output MV_x calculated by integral projection
		//int32_t MV_y_output;
		uint8_t filter_switch;                 //1bit unsigned{0, 1} whether use 3x3 filter to smooth current pixel to calculate pixel difference for Y component 0: not filtering 1: filtering
		uint8_t *y_src_weight;					 //8bit unsigned [0, 255] weight for fusing current frame and reference frame set by user for Y component
		uint8_t *u_src_weight;					 //8bit unsigned [0, 255] weight for fusing current frame and reference frame set by user for U component
		uint8_t *v_src_weight;					 //8bit unsigned [0, 255] weight for fusing current frame and reference frame set by user for V component
		uint32_t frame_num;                    //count;
		uint32_t curr_frameno;
		int32_t orig_width;
		int32_t orig_height;
		int32_t small_width;
		int32_t small_height;
		uint32_t imageNum;
		int32_t** xProj1D;
		int32_t** yProj1D;
		c3dnr_buffer_t pdst_img;
		c3dnr_buffer_t intermedbuf;
		c3dnr_buffer_t *pout_blendimg;
		c3dnr_buffer_t *pfirst_blendimg;
		c3dnr_buffer_t *psecond_blendimg;
		c3dnr_buffer_t *porigimg;
		uint8_t* ref_imageY;
		uint8_t* ref_imageU;
		uint8_t* ref_imageV;
		int32_t* extra;
		pthread_t monitor_thread;
		cmr_handle isp_handle; //wxz: the isp handle for isp_ioctl
		isp_ioctl_fun isp_ioctrl;
		c3dnr_buffer_t out_img[2]; //wxz: for preview
		uint8_t is_first_frame;
		uint8_t blend_num;
		c3dnr_status_t status;
		uint32_t is_cancel;
}c3dnr_info_t;


typedef enum c3dnr_frame_type
{
	FIRST_FRAME = 0,
	NORMAL_FRAME,
	LAST_FRAME
}c3dnr_frame_type_t;

int initModule(int32_t small_width,int32_t small_height , int32_t orig_width , int32_t orig_height , uint32_t total_imageNum);
void Config3DNR(c3dnr_param_info_t *param);
void threeDNR_1D_process(c3dnr_buffer_t curr_image[] ,int32_t width ,int32_t height , uint32_t imageNum);
c3dnr_info_t *get_3dnr_moduleinfo();
#endif
