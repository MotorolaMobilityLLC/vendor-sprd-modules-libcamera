
#include "cmr_common.h"

#ifndef _WATERMARK_H_
#define _WATERMARK_H_

#define WATERMARK_LOGO 0x1
#define WATERMARK_TIME 0x2

typedef struct {
    int imgW;
    int imgH;
    int logoW;
    int logoH;
    int posX;
    int posY;
    int isMirror; /* !0: mirror */
    int angle;    /* angle,use:[0,90,180,270] */
    char *filename;
} sizeParam_t;

int watermark_add_yuv(cmr_handle oem_handle, cmr_u8 *pyuv,
                      sizeParam_t *sizeparam);

#endif //_WATERMARK_H_
