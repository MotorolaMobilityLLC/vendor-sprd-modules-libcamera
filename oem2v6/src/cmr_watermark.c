

/* These interfaces for capture's watermark
 * logo watermark, time watermark
 * 1: need prepare the source iamge files
 * 2: Update some setting about function:
 *    camera_select_logo, camera_select_time_file
 */
#define LOG_TAG "cmr_watermark"
#include "cmr_oem.h"
#include "cmr_watermark.h"
#include "math.h"

/* max number of char of timestamp */
#define TIMESTAMP_CHAR_MAX (20)
/* watermark source file path */
#define CAMERA_LOGO_PATH "/vendor/logo/"

#define Clamp(x, a, b) (((x) < (a)) ? (a) : ((x) > (b)) ? (b) : (x))

/* struct for select time source file */
struct time_src_tag {
    int img_width, img_height;       /* capture image size */
    int angle;                       /* 0,90,180,270 */
    int subnum_width, subnum_height; /* single char's width, height */
    char *pfile;
};

cmr_int camera_select_time_file(struct time_src_tag *pt);

int ImageRotate(unsigned char *imgIn, unsigned char *imgOut, int *width,
                int *height, int angle) {

    int i, j;
    int proc_x = 0;
    int proc_y = 0;
    int widthSrc, heightSrc;
    int widthDst, heightDst;
    widthSrc = *width;
    heightSrc = *height;

    if (90 == angle || 270 == angle) {
        widthDst = heightSrc;
        heightDst = widthSrc;
    } else if (180 == angle) {
        widthDst = widthSrc;
        heightDst = heightSrc;
    } else {
        return -1;
    }
    for (i = 0; i < heightDst; i++) {
        for (j = 0; j < widthDst; j++) {
            if (90 == angle) {
                proc_y = j;
                proc_x = widthSrc - 1 - i;
            } else if (180 == angle) {
                proc_y = heightSrc - 1 - i;
                proc_x = widthSrc - 1 - j;
            } else if (270 == angle) {
                proc_y = heightSrc - 1 - j;
                proc_x = i;
            }

            imgOut[(i * widthDst + j) * 4] =
                imgIn[(proc_y * widthSrc + proc_x) * 4];
            imgOut[(i * widthDst + j) * 4 + 1] =
                imgIn[(proc_y * widthSrc + proc_x) * 4 + 1];
            imgOut[(i * widthDst + j) * 4 + 2] =
                imgIn[(proc_y * widthSrc + proc_x) * 4 + 2];
            imgOut[(i * widthDst + j) * 4 + 3] =
                imgIn[(proc_y * widthSrc + proc_x) * 4 + 3];
        }
    }

    *width = widthDst;
    *height = heightDst;

    return 0;
}

int ImageRotateYuv420(cmr_u8 *imgIn, cmr_u8 *imgOut, int *width, int *height,
                      int angle) {

    int i, j;
    int proc_x = 0;
    int proc_y = 0;
    int widthSrc, heightSrc;
    int widthDst, heightDst;
    cmr_u8 *puvIn, *puvOut;

    widthSrc = *width;
    heightSrc = *height;

    if (90 == angle || 270 == angle) {
        widthDst = heightSrc;
        heightDst = widthSrc;
    } else if (180 == angle) {
        widthDst = widthSrc;
        heightDst = heightSrc;
    } else if (0 == angle) {
        widthDst = widthSrc;
        heightDst = heightSrc;
        *width = widthDst;
        *height = heightDst;
        memcpy(imgOut, imgIn, widthSrc * heightSrc * 3 / 2);

        return 0;
    } else {
        return -1;
    }
    memset(imgOut, 0x00, widthSrc * heightSrc * 3 / 2);
    puvIn = imgIn + widthSrc * heightSrc * 3 / 2;
    puvOut = imgOut + widthSrc * heightSrc * 3 / 2;
    /* y */
    for (i = 0; i < heightDst; i++) {
        for (j = 0; j < widthDst; j++) {
            if (90 == angle) {
                proc_y = j;
                proc_x = widthSrc - 1 - i;
            } else if (180 == angle) {
                proc_y = heightSrc - 1 - i;
                proc_x = widthSrc - 1 - j;
            } else if (270 == angle) {
                proc_y = heightSrc - 1 - j;
                proc_x = i;
            }
            *imgOut = imgIn[(proc_y * widthSrc + proc_x)];
            imgOut++;
        }
    }
    /* uv */
    imgIn += (widthSrc * heightSrc);
    for (i = 0; i < heightDst / 2; i++) {
        for (j = 0; j < widthDst / 2; j++) {
            if (90 == angle) {
                proc_y = j;
                proc_x = widthSrc / 2 - 1 - i;
            } else if (180 == angle) {
                proc_y = heightSrc / 2 - 1 - i;
                proc_x = widthSrc / 2 - 1 - j;
            } else if (270 == angle) {
                proc_y = heightSrc / 2 - 1 - j;
                proc_x = i;
            }
            *imgOut = imgIn[(proc_y * widthSrc + proc_x * 2)];
            imgOut++;
            *imgOut = imgIn[(proc_y * widthSrc + proc_x * 2) + 1];
            imgOut++;
        }
    }
    *width = widthDst;
    *height = heightDst;

    return 0;
}

void RGBA2YUV420_semiplanar(unsigned char *R, unsigned char *Yout, int nWidth,
                            int nHeight) {
    int i, j;
    unsigned char *Y, *Cb, *Cr;
    unsigned char *UVout = Yout + nWidth * nHeight;
    Y = (unsigned char *)calloc(nWidth * nHeight, sizeof(unsigned char));
    Cb = (unsigned char *)calloc(nWidth * nHeight, sizeof(unsigned char));
    Cr = (unsigned char *)calloc(nWidth * nHeight, sizeof(unsigned char));
    for (i = 0; i < nHeight; i++) {
        for (j = 0; j < nWidth; j++) {
            int loc = i * nWidth + j;
            Y[loc] = Clamp((floor(R[4 * loc] * 0.299 + 0.587 * R[4 * loc + 1] +
                                  0.114 * R[4 * loc + 2])),
                           0, 255);
            Cb[loc] = Clamp(
                (floor(R[4 * loc] * (-0.1687) + (-0.3312) * R[4 * loc + 1] +
                       0.5 * R[4 * loc + 2] + 128)),
                0, 255);
            Cr[loc] =
                Clamp((floor(R[4 * loc] * (0.5) + (-0.4186) * R[4 * loc + 1] +
                             (-0.0813) * R[4 * loc + 2] + 128)),
                      0, 255);
        }
    }

    for (i = 0; i < (nWidth * nHeight); i++)
        Yout[i] = Y[i];

    for (i = 0; i < (nHeight / 2); i++) {
        for (j = 0; j < (nWidth / 2); j++) {
            int loc_y = i * 2;
            int loc_x = j * 2;
            int loc1 = loc_y * nWidth + loc_x;
            int loc2 = loc_y * nWidth + loc_x + 1;
            int loc3 = (loc_y + 1) * nWidth + loc_x;
            int loc4 = (loc_y + 1) * nWidth + loc_x + 1;

            UVout[i * nWidth + 2 * j] =
                (unsigned char)((Cr[loc1] + Cr[loc2] + Cr[loc3] + Cr[loc4] +
                                 2) / 4); // v
            UVout[i * nWidth + 2 * j + 1] =
                (unsigned char)((Cb[loc1] + Cb[loc2] + Cb[loc3] + Cb[loc4] +
                                 2) / 4); // u
        }
    }

    free(Y);
    free(Cb);
    free(Cr);
}

int sprd_fusion_yuv420_semiplanar(unsigned char *img, unsigned char *logo,
                                  sizeParam_t *param) {

    if (img == NULL || logo == NULL || param == NULL) {
        CMR_LOGE("%p %p %p", img, logo, param);
        return -1;
    }

    int imgW = param->imgW;
    int imgH = param->imgH;
    int logoW = param->logoW;
    int logoH = param->logoH;
    int posX = param->posX;
    int posY = param->posY;
    int isMirror = param->isMirror;
    int angle = param->angle;
    int imgSize = imgW * imgH;
    int logoSize = logoW * logoH;
    unsigned char *imgY, *imgUV;
    unsigned char *logoY, *logoUV;
    int proc_x, proc_y;
    unsigned char alpha;
    unsigned char *logoRotate =
        (unsigned char *)malloc(logoSize * 4 * sizeof(unsigned char));
    if (logoRotate == NULL) {
        CMR_LOGE("Alloc buf fail");
        return -1;
    }
    if (angle == 0) {
        memcpy(logoRotate, logo, logoSize * 4);
        posY = imgH - logoH - posY;
        if (isMirror != 0) {
            posX = imgW - logoW - posX;
        }
    } else if (angle == 90) {
        ImageRotate(logo, logoRotate, &logoW, &logoH, 90);
        posX ^= posY;
        posY ^= posX;
        posX ^= posY;
        posX = imgW - logoW - posX;
        if (isMirror == 0) {
            posY = imgH - logoH - posY;
        }
    } else if (angle == 180) {
        ImageRotate(logo, logoRotate, &logoW, &logoH, 180);
        if (isMirror == 0) {
            posX = imgW - logoW - posX;
        }
    } else if (angle == 270) {
        ImageRotate(logo, logoRotate, &logoW, &logoH, 270);
        posX ^= posY;
        posY ^= posX;
        posX ^= posY;
        if (isMirror != 0) {
            posY = imgH - logoH - posY;
        }
    }
    posX = (posX < 0) ? 0 : posX;
    posY = (posY < 0) ? 0 : posY;
    /* logo large than image */
    if (posX + logoW > imgW)
        posX = imgW - logoW;
    if (posY + logoH > imgH)
        posY = imgH - logoH;

    CMR_LOGI("src[%d %d], logo[%d %d %d %d] angle[%d %d]", imgW, imgH, posX,
             posY, logoW, logoH, angle, isMirror);
    if (posX < 0 || posY < 0) {
        CMR_LOGW("src img too small to add logo watermark");
        free(logoRotate);
        return -1;
    }
    imgY = img + posY * imgW + posX;
    /* If posX is Odd, need align to 2 */
    imgUV = img + imgSize + posY / 2 * imgW + (posX & (~0x1));
    logoY = logo;
    logoUV = logoY + logoSize;

    RGBA2YUV420_semiplanar(logoRotate, logoY, logoW, logoH);

    if (isMirror == 0) {
        for (int i = 0; i < logoH; i++) {
            for (int j = 0; j < logoW; j++) {
                alpha = logoRotate[4 * (i * logoW + j) + 3];
                imgY[i * imgW + j] = ((255 - alpha) * imgY[i * imgW + j] +
                                      alpha * logoY[i * logoW + j]) /
                                     255;

                if (i % 2 == 0 && j % 2 == 0) {
                    imgUV[i / 2 * imgW + j] =
                        ((255 - alpha) * imgUV[i / 2 * imgW + j] +
                         alpha * logoUV[i / 2 * logoW + j]) /
                        255;
                    imgUV[i / 2 * imgW + j + 1] =
                        ((255 - alpha) * imgUV[i / 2 * imgW + j + 1] +
                         alpha * logoUV[i / 2 * logoW + j + 1]) /
                        255;
                }
            }
        }
    } else {
        for (int i = 0; i < logoH; i++) {
            for (int j = 0; j < logoW; j++) {
                if (angle == 0 || angle == 180) {
                    proc_x = logoW - 1 - j;
                    proc_y = i;
                } else {
                    proc_x = j;
                    proc_y = logoH - 1 - i;
                }
                alpha = logoRotate[4 * (proc_y * logoW + proc_x) + 3];
                imgY[i * imgW + j] = ((255 - alpha) * imgY[i * imgW + j] +
                                      alpha * logoY[proc_y * logoW + proc_x]) /
                                     255;

                if (i % 2 == 0 && j % 2 == 0) {
                    imgUV[i / 2 * imgW + j] =
                        ((255 - alpha) * imgUV[i / 2 * imgW + j] +
                         alpha * logoUV[proc_y / 2 * logoW + proc_x]) /
                        255;
                    imgUV[i / 2 * imgW + j + 1] =
                        ((255 - alpha) * imgUV[i / 2 * imgW + j + 1] +
                         alpha * logoUV[proc_y / 2 * logoW + proc_x + 1]) /
                        255;
                }
            }
        }
    }
    free(logoRotate);

    return 0;
}

static int fusion_yuv420_yuv420(cmr_u8 *img, cmr_u8 *logo, sizeParam_t *param) {

    if (img == NULL || logo == NULL || param == NULL)
        return -1;

    int imgW = param->imgW;
    int imgH = param->imgH;
    int logoW = param->logoW;
    int logoH = param->logoH;
    int posX = param->posX;
    int posY = param->posY;
    int isMirror = param->isMirror;
    int angle = param->angle;
    int imgSize = imgW * imgH;
    int logoSize = logoW * logoH;
    cmr_u8 *imgY, *imgUV;
    cmr_u8 *logoY, *logoUV;
    int proc_x, proc_y;
    cmr_u8 tmp_u8;
    cmr_u8 *logoRotate = (cmr_u8 *)malloc(logoSize * 3 / 2);

    if (!logoRotate) {
        CMR_LOGE("alloc buf fail");
        return -ENOMEM;
    }
    if (angle == 0) {
        ImageRotateYuv420(logo, logoRotate, &logoW, &logoH, 0);
        posY = imgH - logoH - posY;
        if (isMirror != 0) {
            posX = imgW - logoW - posX;
        }
    } else if (angle == 90) {
        ImageRotateYuv420(logo, logoRotate, &logoW, &logoH, 90);
        posX ^= posY;
        posY ^= posX;
        posX ^= posY;
        posX = imgW - logoW - posX;
        if (isMirror == 0) {
            posY = imgH - logoH - posY;
        }
    } else if (angle == 180) {
        ImageRotateYuv420(logo, logoRotate, &logoW, &logoH, 180);
        if (isMirror == 0) {
            posX = imgW - logoW - posX;
        }
    } else if (angle == 270) {
        ImageRotateYuv420(logo, logoRotate, &logoW, &logoH, 270);
        posX ^= posY;
        posY ^= posX;
        posX ^= posY;
        if (isMirror != 0) {
            posY = imgH - logoH - posY;
        }
    }
    /* check */
    posX = (posX < 0) ? 0 : posX;
    posY = (posY < 0) ? 0 : posY;
    /* logo too large, start 0 */
    if (posX + logoW > imgW)
        posX = imgW - logoW;
    if (posY + logoH > imgH)
        posY = imgH - logoH;

    CMR_LOGI("src[%d %d], text[%d %d %d %d], angle %d %d", imgW, imgH, posX,
             posY, logoW, logoH, angle, isMirror);
    if (posX < 0 || posY < 0) {
        CMR_LOGW("Src img too small to add time watermark");
        free(logoRotate);
        return -1;
    }

    imgY = img + posY * imgW + posX;
    imgUV = img + imgSize + posY / 2 * imgW + posX;
    logoY = logoRotate;
    logoUV = logoY + logoSize;
    if (isMirror == 0) {
        for (int i = 0; i < logoH; i++) {
            for (int j = 0; j < logoW; j++) {
                /* if (logoY[i * logoW + j] != 0) ~ way1, fixed alpha value
                 * way1: if time watermark y == 0, pixel should use the capture
                 *       iamge's y(named Y), if y near 0, the fusion pixel
                 *       should too dark(because get little from Y).
                 * way2: use time watermark y as alpha, then y near 0, likely
                 *       use Y, if y near 255, Y affect less
                 *       Of course can try as logo watermark use rgba data
                 */
                tmp_u8 = logoY[i * logoW + j];
                imgY[i * imgW + j] =
                    (255 - tmp_u8) * imgY[i * imgW + j] / 255 + tmp_u8;
                if (i % 2 == 0 && j % 2 == 0) {
                    imgUV[i / 2 * imgW + j] =
                        ((255 - tmp_u8) * imgUV[i / 2 * imgW + j] +
                         tmp_u8 * logoUV[i / 2 * logoW + j]) /
                        255;
                    imgUV[i / 2 * imgW + j + 1] =
                        ((255 - tmp_u8) * imgUV[i / 2 * imgW + j + 1] +
                         tmp_u8 * logoUV[i / 2 * logoW + j + 1]) /
                        255;
                }
            }
        }
    } else {
        for (int i = 0; i < logoH; i++) {
            for (int j = 0; j < logoW; j++) {
                if (angle == 0 || angle == 180) {
                    proc_x = logoW - 1 - j;
                    proc_y = i;
                } else {
                    proc_x = j;
                    proc_y = logoH - 1 - i;
                }
                /* if (logoY[proc_y * logoW + proc_x] != 0) {
                 * As above
                 */
                tmp_u8 = logoY[proc_y * logoW + proc_x];
                imgY[i * imgW + j] =
                    (255 - tmp_u8) * imgY[i * imgW + j] / 255 + tmp_u8;
                if (i % 2 == 0 && j % 2 == 0) {
                    imgUV[i / 2 * imgW + j] =
                        ((255 - tmp_u8) * imgUV[i / 2 * imgW + j] +
                         tmp_u8 * logoUV[proc_y / 2 * logoW + proc_x]) /
                        255;
                    imgUV[i / 2 * imgW + j + 1] =
                        ((255 - tmp_u8) * imgUV[i / 2 * imgW + j + 1] +
                         tmp_u8 * logoUV[proc_y / 2 * logoW + proc_x + 1]) /
                        255;
                }
            }
        }
    }

    free(logoRotate);

    return 0;
}

/* open and read logo watermark file
 * Named as: logo_<width>x<height>.rgba
 */
cmr_int camera_get_logo_data(cmr_u8 *logo, int Width, int Height) {
    char tmp_name[128];
    char file_name[40];
    cmr_u32 len = 0;

    strcpy(tmp_name, CAMERA_LOGO_PATH);
    sprintf(file_name, "logo_%d", Width);
    strcat(tmp_name, file_name);
    strcat(tmp_name, "x");
    sprintf(file_name, "%d", Height);
    strcat(tmp_name, file_name);
    strcat(tmp_name, ".rgba");
    FILE *fp = fopen(tmp_name, "rb");
    if (fp) {
        len = fread(logo, 1, Width * Height * 4, fp);
        fclose(fp);
        if (len == Width * Height * 4)
            return 0;
    }
    CMR_LOGW("open logo watermark src file failed");

    return -ENOENT;
}
/* combine image as:2019-10-10 10:37:45 with src
 * input: time source(yuv)
 * output: timestamp image as 2019-10-10 10:37:45, vertical
 * return: how many char
 */
cmr_int combine_time_watermark(cmr_u8 *pnum, cmr_u8 *ptext, int subnum_len,
                            const int subnum_total)
{
    cmr_s32 rtn = 0;
    time_t timep;
    struct tm *p;
    char time_text[TIMESTAMP_CHAR_MAX]; /* total = 19,string as:"2019-11-04 13:30:50" */
    cmr_u8 *pdst, *psrc, *puvs;
    cmr_u32 i, j;

    /* get time */
    time(&timep);
    p = localtime(&timep);
    if (!p) {
        CMR_LOGE("get time fail");
        return 0;
    }
    /* sizeof time_text should large than the string lenght */
    sprintf(time_text, "%04d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year),
            (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    CMR_LOGV("Time watermark: %s", time_text);
    /* copy y */
    pdst = ptext;
    for (i = 0; i < sizeof(time_text); i++) {
        if ((time_text[i] == '\0') || (i >= TIMESTAMP_CHAR_MAX))
            break;
        if (time_text[i] == '-')
            j = 10;
        else if (time_text[i] == ':')
            j = 11;
        else if (time_text[i] >= '0' && time_text[i] <= '9')
            j = time_text[i] - '0';
        else
            j = 12;
        psrc = pnum + subnum_len * j;
        memcpy(pdst, psrc, subnum_len);
        pdst += subnum_len;
    }
    /* copy uv:420 */
    puvs = pnum + subnum_len * subnum_total;
    for (i = 0; i < sizeof(time_text); i++) {
        if ((time_text[i] == '\0') || (i >= TIMESTAMP_CHAR_MAX))
            break;
        if (time_text[i] == '-')
            j = 10;
        else if (time_text[i] == ':')
            j = 11;
        else if (time_text[i] >= '0' && time_text[i] <= '9')
            j = time_text[i] - '0';
        else
            j = 12;
        psrc = puvs + subnum_len / 2 * j;
        memcpy(pdst, psrc, subnum_len / 2);
        pdst += (subnum_len / 2);
    }
    rtn = i;

    return rtn;
}

/* for time watermark
 * source: yuv file,"0123456789-: ",last one is space
 * Input: width, height ---- size of capture image
 * Output: yuv data as text:2019-10-10 10:37:45
 *        width, height --- size of yuv data(timestamp)
 * return: 0:seccuss,other:fail;
 */
cmr_int camera_get_time_yuv420(cmr_u8 **data, int *width, int *height) {
    cmr_s32 rtn = -1;
    char tmp_name[256];
    /* info of source file for number */
    const char *file_name;       /* source file for number:0123456789-:  */
    const int subnum_total = 13; /* 0--9,-,:, (space), total = 13 */
    int subnum_width;            /* sub number width:align 2 */
    int subnum_height;           /* sub number height:align 2 */
    int subnum_len;              /* sub number pixels,y data size */
    int src_filelen;             /* size for alloc buf for time.yuv */
    cmr_u8 *pnum = NULL, *ptext = NULL, *ptextout = NULL;
    int dst_width, dst_height;
    struct time_src_tag time_info;
    cmr_u32 j;

    time_info.img_width = *width;
    time_info.img_height = *height;
    rtn = camera_select_time_file(&time_info);
    if (rtn) {
        CMR_LOGW("Get time yuv file fail");
        goto exit;
    }
    subnum_width = time_info.subnum_width;
    subnum_height = time_info.subnum_height;
    file_name = time_info.pfile;
    subnum_len = subnum_width * subnum_height;
    src_filelen = subnum_width * subnum_height * subnum_total * 3 / 2;

    CMR_LOGV("sub number w,h[%d %d],total %d", subnum_width, subnum_height,
             subnum_total);
    /* read source of number */
    pnum = (cmr_u8 *)malloc(src_filelen);
    if (pnum == NULL) {
        rtn = -ENOMEM;
        goto exit;
    }
    strcpy(tmp_name, CAMERA_LOGO_PATH);
    if (strlen(tmp_name) + strlen(file_name) >= sizeof(tmp_name)) {
        rtn = -1;
        CMR_LOGE("File name too long");
        goto exit;
    }
    strcat(tmp_name, file_name);
    FILE *fp = fopen(tmp_name, "rb");
    if (fp == NULL) {
        CMR_LOGW("open time watermark src file failed");
        rtn = -ENOENT;
        goto exit;
    }
    j = fread(pnum, 1, src_filelen, fp);
    fclose(fp);
    if (j < src_filelen) {
        CMR_LOGW("read time watermark file fail, len %d", j);
        rtn = -ENOENT;
        goto exit;
    }
    /* for text of time */
    ptext = (cmr_u8 *)malloc(subnum_len * TIMESTAMP_CHAR_MAX * 3 / 2);
    if (!ptext) {
        rtn = -ENOMEM;
        goto exit;
    }
    j = combine_time_watermark(pnum, ptext, subnum_len, subnum_total);
    if (j == 0) {
        CMR_LOGE("combine fail");
        goto exit;
    }
    free(pnum);
    pnum = NULL;
    dst_width = subnum_width;
    dst_height = subnum_height * j;
    /* rotation 90 anticlockwise */
    ptextout = (cmr_u8 *)malloc(subnum_len * TIMESTAMP_CHAR_MAX * 3 / 2);
    if (!ptextout) {
        rtn = -ENOMEM;
        goto exit;
    }
    ImageRotateYuv420(ptext, ptextout, &dst_width, &dst_height, 90);
    free(ptext);
    ptext = NULL;
    *data = ptextout; /* free after return */
    *width = dst_width;
    *height = dst_height;
    rtn = 0;
    CMR_LOGD("done, %p", ptextout);

    return rtn;

exit:
    if (pnum)
        free(pnum);
    if (ptext)
        free(ptext);
    /*    if (ptextout) //(logic DEADCODE)
     *       free(ptextout);
     */
    CMR_LOGE("fail,rtn = %d", rtn);

    return rtn;
}

/* select one logo for capture
   * which picture use which one logo, do as you like
   * return: 0 success,other fail
   */
cmr_int camera_select_logo(sizeParam_t *size) {
    cmr_int ret = -1;
    cmr_int i;
#if 1
    cmr_int side_len;
    /* range mode: check side length for logo */
    const sizeParam_t cap_logo[] = {
        {4000, 0, 1000, 200, 0, 0, 0, 0},
        {3000, 0, 800, 160, 0, 0, 0, 0},
        {2000, 0, 600, 120, 0, 0, 0, 0},
        {1000, 0, 300, 60, 0, 0, 0, 0},
        {500, 0, 100, 20, 0, 0, 0, 0}
        /* little than 500, capture too small to add watermark */
    };

    /* logo watermark at width side of capture image */
    side_len = size->imgW;
    if (size->angle == 90 || size->angle == 270)
        side_len = size->imgH; /* at height side of capture iamge */
    for (i = 0; i < ARRAY_SIZE(cap_logo); i++) {
        if (side_len >= cap_logo[i].imgW) {
            size->posX = cap_logo[i].posX;
            size->posY = cap_logo[i].posY;
            size->logoW = cap_logo[i].logoW;
            size->logoH = cap_logo[i].logoH;
            CMR_LOGD("%d, [%d %d]", side_len, size->logoW, size->logoH);
            ret = 0; /* got */
            break;
        }
    }

#else /* one by one mode: set one logo for one picture */
    const sizeParam_t cap_logo[] = {
        /* cap: width,height;logo:width,height;cap:posx,posy */
        /* L3 */
        {4000, 3000, 1200, 240, 0, 0, 0, 0},
        {4000, 2250, 1200, 240, 0, 0, 0, 0},
        {2592, 1944, 1000, 200, 0, 0, 0, 0},
        {2592, 1458, 1000, 200, 0, 0, 0, 0},
        {2048, 1536, 800, 160, 0, 0, 0, 0},
        {2048, 1152, 800, 160, 0, 0, 0, 0},
        {4608, 3456, 1200, 240, 0, 0, 0, 0},
        {4608, 2592, 1000, 200, 0, 0, 0, 0},
        {3264, 2448, 1000, 200, 0, 0, 0, 0},
        {3264, 1836, 1000, 200, 0, 0, 0, 0},
        {2320, 1740, 800, 160, 0, 0, 0, 0},
    };
    for (i = 0; i < ARRAY_SIZE(cap_logo); i++) {
        if (size->imgW == cap_logo[i].imgW && size->imgH == cap_logo[i].imgH) {
            size->posX = cap_logo[i].posX;
            size->posY = cap_logo[i].posY;
            size->logoW = cap_logo[i].logoW;
            size->logoH = cap_logo[i].logoH;
            ret = 0; /* got */
            break;
        }
    }
#endif
    return ret;
}
/* select one yuv(time) timestamp of capture
   * which picture use which one yuv(time) file, do as you like
   * return: 0 success,other fail
   */
cmr_int camera_select_time_file(struct time_src_tag *pt) {
    cmr_int ret = -1;
    int i;

#if 1 /* range mode: check side width */
    int side_len;
    const struct time_src_tag tb_time[] = {
        /* width, height, angle, subnum_width, subnum_height, file name */
        {4000, 0, 0, 100, 50, "time_vert_100x50x13.yuv"},
        {3000, 0, 0, 72, 36, "time_vert_72x36x13.yuv"},
        {1000, 0, 0, 48, 24, "time_vert_48x24x13.yuv"},
        {720, 0, 0, 24, 12, "time_vert_24x12x13.yuv"}
        /* less than 720, capture too small to add watermark */
    };
    side_len = pt->img_width;
    for (i = 0; i < ARRAY_SIZE(tb_time); i++) {
        if (side_len >= tb_time[i].img_width) {
            pt->subnum_width = tb_time[i].subnum_width;
            pt->subnum_height = tb_time[i].subnum_height;
            pt->pfile = tb_time[i].pfile;
            CMR_LOGD("%d [%d %d] %s", side_len, pt->subnum_width,
                     pt->subnum_height, pt->pfile);
            ret = 0; /* got */
            break;
        }
    }

#else /* one by one mode: sample(If use,please setting and debug) */
    const struct time_src_tag tb_time[] = {
        /* width, height, angle, subnum_width, subnum_height, file name */
        {4000, 3000, 0, 80, 40, "time_large.yuv"},
        {4608, 3456, 0, 80, 40, "time_large.yuv"},
        {3264, 2448, 0, 60, 30, "time_middle.yuv"},
        {2048, 1536, 0, 30, 16, "time_little.yuv"},
    };
    for (i = 0; i < ARRAY_SIZE(tb_time); i++) {
        if (pt->img_width == tb_time[i].img_width &&
            pt->img_height == tb_time[i].img_height) {

            pt->subnum_width = tb_time[i].subnum_width;
            pt->subnum_height = tb_time[i].subnum_height;
            pt->pfile = tb_time[i].pfile;
            ret = 0; /* got */
            break;
        }
    }

#endif
    return ret;
}

/* return: 0: not add logo or time, for flush memory
   *         WATERMARK_LOGO: add logo
   *         WATERMARK_TIME: add time
   */
int watermark_add_yuv(cmr_handle oem_handle, cmr_u8 *pyuv,
                      sizeParam_t *sizeparam) {
    cmr_int ret;
    cmr_int flag;
    int img_width, img_height, img_angle;

    img_width = sizeparam->imgW;
    img_height = sizeparam->imgH;
    img_angle = sizeparam->angle;
    flag = camera_get_watermark_flag(oem_handle);
    CMR_LOGI("watermark flag %x,[%d %d]", flag, sizeparam->imgW,
             sizeparam->imgH);
    flag = flag & (WATERMARK_LOGO | WATERMARK_TIME);
    if (flag == 0x00)
        return flag;
    if (flag & WATERMARK_LOGO) {
        ret = camera_select_logo(sizeparam);
        if (ret == 0) {
            cmr_u8 *logo_buf =
                (cmr_u8 *)malloc((sizeparam->logoW) * (sizeparam->logoH) * 4);
            if (NULL == logo_buf) {
                CMR_LOGE("No mem!");
                return CMR_CAMERA_NO_MEM;
            }

            /* in camera_select_logo, remove this
             * suggest: align 2
             */
            sizeparam->posX = 0;
            sizeparam->posY = 0;
            ret = camera_get_logo_data(logo_buf, sizeparam->logoW,
                                       sizeparam->logoH);
            if (!ret)
                sprd_fusion_yuv420_semiplanar(pyuv, logo_buf, sizeparam);
            free(logo_buf);
            logo_buf = NULL;
        }
    }
    /* time */
    if (flag & WATERMARK_TIME) { /* time watermark */
        int time_width, time_height;
        cmr_u8 *ptime;

        time_width = img_width;
        time_height = img_height;
        if (img_angle == 90 || img_angle == 270) {
            time_width = img_height;
            time_height = img_width;
        }
        ret = camera_get_time_yuv420(&ptime, &time_width, &time_height);
        if (!ret) {
            sizeparam->logoW = time_width;
            sizeparam->logoH = time_height;
            sizeparam->posY = 0; /* lower right corner */
            /* time watermark position,align 2 */
            sizeparam->posX = (sizeparam->imgW - sizeparam->logoW) & (~0x1);
            if (sizeparam->posX >= 0)
                fusion_yuv420_yuv420(pyuv, ptime, sizeparam);
            else
                CMR_LOGW("Capture too small to add time watermark");
            free(ptime);
        }
    }

    return flag;
}
