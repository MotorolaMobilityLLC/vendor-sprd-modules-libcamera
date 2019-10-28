#ifndef __bmp_h__
#define __bmp_h__

#include "stdint.h"

#pragma pack(push, 1)

typedef struct tagRGBQUAD11 {
    uint8_t    rgbBlue;
    uint8_t    rgbGreen;
    uint8_t    rgbRed;
    uint8_t    rgbReserved;
} RGBQUAD1;

typedef struct tagBITMAPINFOHEADER1{
    uint32_t       biSize;
    int32_t               biWidth;
    int32_t               biHeight;
    uint16_t      biPlanes;
    uint16_t      biBitCount;
    uint32_t       biCompression;
    uint32_t       biSizeImage;
    int32_t               biXPelsPerMeter;
    int32_t               biYPelsPerMeter;
    uint32_t       biClrUsed;
    uint32_t       biClrImportant;
} BITMAPINFOHEADER1,  *PBITMAPINFOHEADER1;

typedef struct tagBITMAPINFO1 {
    BITMAPINFOHEADER1    bmiHeader;
    RGBQUAD1             bmiColors[1];
} BITMAPINFO1,  *PBITMAPINFO1;

typedef struct tagBITMAPFILEHEADER1 {
    uint16_t   bfType;
    uint32_t    bfSize;
    uint16_t   bfReserved1;
    uint16_t   bfReserved2;
    uint32_t    bfOffBits;
} BITMAPFILEHEADER1, *PBITMAPFILEHEADER1;

#pragma pack(pop)

#define WIDTH_BYTES(bits) (((bits) + 31) / 32 * 4)

uint32_t convertRGB2BGR(
    uint8_t *pRGBin,
    uint8_t *pBGRout,
    uint32_t width,
    uint32_t height
    );

uint32_t convertBGR2RGB(
    uint8_t *pBGRin,
    uint8_t *pRGBout,
    uint32_t width,
    uint32_t height
    );

uint32_t save_bmp(
            const char *filename,
            uint8_t *pRGBData,
            uint32_t width,
            uint32_t height
            );

int OpenBMPFile(const char* pFileName, BITMAPINFO1 *pBmpInfo, uint8_t **ppImageBuffer);

#endif
