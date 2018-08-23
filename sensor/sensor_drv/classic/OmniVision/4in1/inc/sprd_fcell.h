#include "fcell.h" 
#include <fcntl.h>//#include < stdio.h >
//#include <stdlib.h>
#include <malloc.h>
#include<time.h>
//#include<sys / time.h>

typedef unsigned short uint16;
typedef long long int64;
typedef unsigned char uint8;
#define IMG_WIDTH 4672 //4608 //5120
#define IMG_HEIGHT    3504 //3456 //3840
#define XTALK_LEN    600
#define XTALK_DATA    "/data/vendor/cameraserver/xtalk_otp.dat"
#define OTPDPC_DATA    "/data/vendor/cameraserver/dpc_otp.dat"
#define XTALK_BLC    64
#define XTALK_OFFSET_X    0
#define XTALK_OFFSET_Y    0
#define XTALK_IMGWIDTH    4672 //4608 //5120
#define XTALK_IMGHEIGHT   3504 //3456 // 3840
#define XTALK_FLIP     0
#define XTALK_MIRROR    0
#define OTPDPC_OFFSET_X    48
#define OTPDPC_OFFSET_Y    42
#define OTPDPC_FLIP    1
#define OTPDPC_MIRROR    0
#define FCELL_PATTERN    E_OV_FCD_CFA16 |    E_OV_FCD_FLAG_DPC | E_OV_FCD_FLAG_OTPDPC | E_OV_FCD_FLAG_XTALK
#define INPUT_FILE    "/data/vendor/cameraserver/input.raw"
#define OUTPUT_FILE "/data/vendor/cameraserver/output.raw"
#define CAMERA_GAIN   16
void *allocator(size_t sizeInBytes) {
    return (void *)malloc(sizeInBytes);
}
void deallocator(void *ptr) {
    if (ptr != NULL)
        free(ptr);
}
static ovfcell_control control;
int ov4c_init(ovfcell_init init);//);//unsigned short *xtalk_data, unsigned short *otp_data, int otpdpc_len)
int ov4c_process(unsigned short *src,unsigned short *dst);//
int ov4c_release();   

