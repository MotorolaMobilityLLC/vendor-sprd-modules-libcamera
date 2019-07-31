#include "remosaic_itf.h" 
#include <fcntl.h>//#include < stdio.h >
//#include <stdlib.h>
#include <malloc.h>
#include<time.h>
//#include<sys / time.h>

typedef unsigned short uint16;
typedef long long int64;
typedef unsigned char uint8;

#define XTALK_BLC    64
#define XTALK_LEN    2048 //600


typedef struct tag_ssfcell_init
{
	int width;									/* width of fcell image */
	int height;									/* height of fcell image */
	int xtalk_len;								/*otp xtalk data length which depends on module input*/
	void* xtalk;								/*otp xtalk data buffer*/
	enum e_remosaic_bayer_order bayer_order;
	int32_t pedestal;
}ssfcell_init;


int ss4c_init(ssfcell_init init);
int ss4c_process(unsigned short *src,unsigned short *dst, struct st_remosaic_param* p_param, uint32_t input_width, uint32_t input_height);//
int ss4c_release();   

