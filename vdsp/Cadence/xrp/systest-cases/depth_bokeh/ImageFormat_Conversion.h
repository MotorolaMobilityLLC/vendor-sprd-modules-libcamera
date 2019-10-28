#ifndef __IMAGEFORMAT_CONVERSION__
#define __IMAGEFORMAT_CONVERSION__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "typedef.h"


void YCbCr2RGB(int *Y, int *Cb, int *Cr,  int nWidth, int nHeight);
void ReadoutYUV420(char inputfilename[256],int nWidth, int nHeight, unsigned char *RGBBuffer, int flip_flag, int mirror_flag);

//input file is NV21 format, and yuv_image has separate plane for Y, U and V components.
BOOL ReadYUV420File(char inputfilename[256],int nWidth, int nHeight, uint8 *yuv_image);

BOOL ReadYUV422File(char inputfilename[256],int nWidth, int nHeight, uint8 *yuv_image);

void ImageYUV420ToRGB3Planar(uint8* yuv420_image, int32 image_width, int32 image_height, uint8* rgb_image);

void ImageYUV420_flip(uint8* yuv420_image, int32 image_width, int32 image_height, uint8* yuv420_image_flipped, BOOL flip_flag, BOOL mirror_flag);

BOOL ReadNV21File(char inputfilename[256], int nWidth, int nHeight, uint8 *yuv_image);
#endif
