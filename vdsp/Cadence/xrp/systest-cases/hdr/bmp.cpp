#ifndef XTENSA_DEBUGLIB
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "bmp.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define     SAFE_FREE(x) {if(0 != x) {free(x); x = 0;}}

int32_t filesize(FILE *stream)
{
    int32_t curpos, length;
    curpos = ftell(stream);
    fseek(stream, 0L, SEEK_END);
    length = ftell(stream);
    fseek(stream, curpos, SEEK_SET);
    return length;
}
uint32_t convertRGB2BGR(
         uint8_t *pRGBin,
         uint8_t *pBGRout,
         uint32_t width,
         uint32_t height
            )
{
    //uint8_t  r=0;
    //uint8_t  *rp=NULL,*bp=NULL;
    uint32_t i = 0 ,j = 0, offset=0;
    uint8_t temp;
    for (i=0; i<height; i++)
    {
        for (j=0; j<width; j++)
        {
            offset = (i*width+j)*3;
            temp = *(pBGRout+offset +2);
            *(pBGRout+offset +2)  = *(pRGBin+offset);
            *(pBGRout+offset +1) = *(pRGBin+offset +1);
            *(pBGRout+offset   ) = temp;
        }
    }

    return 0;
}

uint32_t convertBGR2RGB(
    uint8_t *pBGRin,
    uint8_t *pRGBout,
    uint32_t width,
    uint32_t height
    )
{
    convertRGB2BGR(pBGRin, pRGBout, width, height);

    return 0;
}

uint32_t ChangeRows(
         uint8_t *pRGBin,
         uint8_t *pRGBout,
         uint32_t width,
         uint32_t height
            )
{
    uint32_t  i = 0 ,j = 0,
            offsetin=0,
            offsetout=0;
    for (i=0; i<height; i++)
    {
        for (j=0; j<width; j++)
        {
            offsetin  = (i*width+j)*3;
            offsetout = ((height-i-1)*width+j)*3;
            *(pRGBout+offsetout +2) = *(pRGBin+offsetin);
            *(pRGBout+offsetout +1) = *(pRGBin+offsetin +1);
            *(pRGBout+offsetout +0) = *(pRGBin+offsetin +2);
        }
    }

    return 0;
}

uint32_t save_bmp(
        const char *filename,
        uint8_t *pRGBData,
        uint32_t width,
        uint32_t height
             )
{
    uint32_t  bRVal = FALSE;
    uint8_t   *pTempData1 = NULL, *pTempData2 = NULL;
    BITMAPINFO1* pBbmpInfo  = NULL;
    BITMAPFILEHEADER1 bfh   = {0};
    int nTable              = 0;
    uint32_t i,j              = 0;
    uint32_t dwSize           = 0 ;
    uint32_t dwImageSize      = 0;
    uint32_t dwWriteSize      = 0 ;
    FILE *bmpFile   = NULL;
    uint32_t widthByteNum;

    pBbmpInfo = (BITMAPINFO1*)malloc(sizeof(BITMAPINFO1) + 255*sizeof(RGBQUAD1));
    pBbmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER1);
    pBbmpInfo->bmiHeader.biPlanes = 1;
    pBbmpInfo->bmiHeader.biBitCount = 24;
    pBbmpInfo->bmiHeader.biCompression = 0L;
    pBbmpInfo->bmiHeader.biSizeImage = 0;
    pBbmpInfo->bmiHeader.biXPelsPerMeter = 72;
    pBbmpInfo->bmiHeader.biYPelsPerMeter = 72;
    pBbmpInfo->bmiHeader.biClrUsed = 0;
    pBbmpInfo->bmiHeader.biClrImportant = 0;
    pBbmpInfo->bmiHeader.biWidth = width;   //图象宽度
    pBbmpInfo->bmiHeader.biHeight = height;  //图象高度
    //bmpInfo.bmiHeader.biBitCount = 24;

    for (i = 0 ; i < 256 ; i++)
    {
        pBbmpInfo->bmiColors[i].rgbBlue = (uint8_t)i;
        pBbmpInfo->bmiColors[i].rgbGreen = (uint8_t)i;
        pBbmpInfo->bmiColors[i].rgbRed = (uint8_t)i;
        pBbmpInfo->bmiColors[i].rgbReserved = 0;
    }

    if (pBbmpInfo->bmiHeader.biBitCount > 8)
    {
        nTable = 0;
    }
    else
    {
        nTable = 255;   //除非biBitCount == 24否则总是存256个调色板颜色.
    }

    dwImageSize =  (pBbmpInfo->bmiHeader.biWidth * pBbmpInfo->bmiHeader.biHeight)
        * ((pBbmpInfo->bmiHeader.biBitCount + 7) / 8);
    //计算4Bytes行对齐的的imageSize,256色和24bit的BMP图片都是4B对齐的.modified by Hongbo
    dwImageSize =  WIDTH_BYTES(pBbmpInfo->bmiHeader.biWidth * pBbmpInfo->bmiHeader.biBitCount)
        * pBbmpInfo->bmiHeader.biHeight;

    pBbmpInfo->bmiHeader.biSizeImage = dwImageSize;

    if (dwImageSize <= 0)
    {
        bRVal = FALSE;
    }
    else{
        bfh.bfType      = (uint16_t)'M' << 8 | 'B';
        bfh.bfOffBits   = sizeof(BITMAPFILEHEADER1) + sizeof(BITMAPINFOHEADER1) + nTable * sizeof(RGBQUAD1);
        bfh.bfSize      = bfh.bfOffBits + dwImageSize;

        bmpFile = fopen(filename,"wb");
        if (bmpFile == NULL) {
            bRVal = FALSE;
        }
        else{
            dwSize = sizeof(BITMAPFILEHEADER1);
            assert(dwSize == 14);
            dwWriteSize = fwrite(&bfh, dwSize, 1, bmpFile );

            dwSize = sizeof(BITMAPINFOHEADER1) + nTable * sizeof(RGBQUAD1);
            dwWriteSize = fwrite(pBbmpInfo, dwSize, 1, bmpFile );

            dwSize = dwImageSize;
            pTempData1 = (uint8_t*)calloc(dwImageSize, 1);
            pTempData2 = (uint8_t*)calloc(width * height * 3, 1);
            ChangeRows(pRGBData, pTempData2, width, height);
            widthByteNum = WIDTH_BYTES(pBbmpInfo->bmiHeader.biWidth * pBbmpInfo->bmiHeader.biBitCount);
            for(i = 0; i < height; i++)
            {
                for(j = 0; j < width * 3; j++)
                {
                    pTempData1[i * widthByteNum + j] = pTempData2[width * 3 * i + j];
                }
            }

            dwWriteSize = fwrite(pTempData1, dwSize, 1, bmpFile );


            fclose(bmpFile);
            fprintf(stdout, "saved to : %s\n",filename);
            bRVal = bfh.bfSize;

            free(pTempData1);
            free(pTempData2);
            pTempData1 = NULL;
            pTempData2 = NULL;
        }
    }
	free(pBbmpInfo);

    return bRVal;
}


//打开图像文件函数
int OpenBMPFile(const char* pFileName, BITMAPINFO1 *pBmpInfo, uint8_t **ppImageBuffer)
{
    int     bRVal = TRUE;
    uint32_t  dwBytesRead = 0;
    uint32_t  dwDataSize = 0;
    int32_t       width=0,height=0;
    BITMAPFILEHEADER1 bfh;
    uint8_t * pImageBuffer = NULL, * pUpsideDownImageBuffer = NULL, * pTempData1 = NULL;
    FILE *hFile = fopen(pFileName,"rb");
    uint16_t widthByteNum;
    int32_t i, j;
    if (hFile == NULL) {
        char errMsg[1024]="";
        sprintf(errMsg,"ERROR:[%d] %s\nFile:%s\n",errno,strerror(errno),pFileName);
        perror(errMsg);
        bRVal = FALSE;
    }
    else{
        dwDataSize = sizeof(BITMAPFILEHEADER1);
        dwBytesRead = fread(&bfh,1, dwDataSize, hFile);
        if (bfh.bfType != ((uint16_t)'M' << 8 | 'B')) {
            bRVal = FALSE;
        }
        else{
            dwDataSize = sizeof(BITMAPINFOHEADER1);
            dwBytesRead = fread(pBmpInfo,1, dwDataSize,  hFile);
            width = pBmpInfo->bmiHeader.biWidth;
            height= pBmpInfo->bmiHeader.biHeight;
            if(bfh.bfOffBits - sizeof(BITMAPFILEHEADER1) - sizeof(BITMAPINFOHEADER1) > 0)//if (pBmpInfo->bmiHeader.biBitCount <= 8)//[2006-11-02]
            {
                uint8_t* pTempBuff = NULL;
                dwDataSize = bfh.bfOffBits - sizeof(BITMAPFILEHEADER1) - sizeof(BITMAPINFOHEADER1);
                pTempBuff = (uint8_t*)malloc(dwDataSize);
                //本项目为RGB,调色板信息没有用,丢掉.
                dwBytesRead = fread(pTempBuff,1,dwDataSize,hFile);
                free(pTempBuff);
            }

            dwDataSize = WIDTH_BYTES(pBmpInfo->bmiHeader.biWidth * pBmpInfo->bmiHeader.biBitCount) *
                pBmpInfo->bmiHeader.biHeight;
            widthByteNum = WIDTH_BYTES(pBmpInfo->bmiHeader.biWidth * pBmpInfo->bmiHeader.biBitCount);
            pImageBuffer = (uint8_t *)malloc(width * height * 3);
            pTempData1 = (uint8_t *)malloc(width * height * 3);
            pUpsideDownImageBuffer = (uint8_t *)malloc(dwDataSize);
            dwBytesRead = fread( pUpsideDownImageBuffer, 1, dwDataSize, hFile);
            for(i = 0; i < height; i++)
            {
                for(j = 0; j < width * 3; j++)
                {
                     pTempData1[width * 3 * i + j] = pUpsideDownImageBuffer[i * widthByteNum + j];
                }
            }

            if (ferror(hFile))
            {
                perror(pFileName);
                //system("pause");
            }
            if (ppImageBuffer)
            {
                ChangeRows(pTempData1, pImageBuffer, width, height);
                *ppImageBuffer = pImageBuffer;
                SAFE_FREE(pUpsideDownImageBuffer);
            }
            SAFE_FREE(pTempData1);
        }
        fclose(hFile);
    }

    return bRVal;
}
#endif
