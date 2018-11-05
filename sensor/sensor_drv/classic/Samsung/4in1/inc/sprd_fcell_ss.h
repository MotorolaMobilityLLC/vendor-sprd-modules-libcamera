#include "remosaic_itf.h" 
#include <fcntl.h>//#include < stdio.h >
//#include <stdlib.h>
#include <malloc.h>
#include<time.h>
//#include<sys / time.h>

typedef unsigned short uint16;
typedef long long int64;
typedef unsigned char uint8;
#define IMG_WIDTH 4640 //4672 //4608 //5120
#define IMG_HEIGHT    3488 //3504 //3456 //3840
#define XTALK_LEN    2048 //600
#define XTALK_DATA    "/data/vendor/cameraserver/eeprom.dat"
//#define OTPDPC_DATA    "/data/misc/cameraserver/dpc_otp.dat"
#define XTALK_BLC    64
#define XTALK_OFFSET_X    0
#define XTALK_OFFSET_Y    0
#define XTALK_IMGWIDTH    4640 //4672 //4608 //5120
#define XTALK_IMGHEIGHT   3488  //3504 //3456 // 3840
#define XTALK_FLIP     0
#define XTALK_MIRROR    0
//#define OTPDPC_OFFSET_X    48
//#define OTPDPC_OFFSET_Y    42
//#define OTPDPC_FLIP    1
//#define OTPDPC_MIRROR    0
#define FCELL_PATTERN    E_OV_FCD_CFA16 |    E_OV_FCD_FLAG_DPC | E_OV_FCD_FLAG_OTPDPC | E_OV_FCD_FLAG_XTALK
#define INPUT_FILE    "/data/vendor/cameraserver/input.raw"
#define OUTPUT_FILE "/data/vendor/cameraserver/output.raw"
#define CAMERA_GAIN   16 

typedef struct tag_ssfcell_init
{
	int width;									/* width of fcell image */
	int height;									/* height of fcell image */
	int xtalk_len;								/*otp xtalk data length which depends on module input*/
	void* xtalk;								/*otp xtalk data buffer*/
	enum e_remosaic_bayer_order bayer_order;
	int32_t pedestal;
//	int otpdpc_len;								/*otp dpc data length which depends on module input*/
//	void* otpdpc;								/*otp dpc buffer*/
//	short ofst_xtalk[2];						/*X/Y direction offset, Range: 0~4095*/
//	short imgwin_xtalk[2];						/*vendor image width/height, Range: 0~4095*/
//	short ofst_otpdpc[2];						/*X/Y direction offset, Range: 0~8191*/
//	int flag;									/*xtalk/otpdpc/dpc control flag*/
}ssfcell_init;


int ss4c_init(ssfcell_init init);//);//unsigned short *xtalk_data, unsigned short *otp_data, int otpdpc_len)
int ss4c_process(unsigned short *src,unsigned short *dst, struct st_remosaic_param* p_param);//
int ss4c_release();   

