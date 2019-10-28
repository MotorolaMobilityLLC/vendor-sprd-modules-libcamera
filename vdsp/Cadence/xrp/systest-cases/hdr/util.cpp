#ifndef XTENSA_DEBUGLIB
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "sprd_hdr_api.h"
#include "bmp.h"

#include "util.h"

#ifdef _MSC_VER
#   define strdup         _strdup
#endif

static int parse_input_frames(const char* optarg, char* inputs[MAX_FRAME_NUM])
{
    int i, j, idx;
    char* arg = strdup(optarg);
    char* input = NULL;
    int len = strlen(arg);

    j = 0;
    idx = 0;
    for (i = 0; i < len && j < MAX_FRAME_NUM; i++)
    {
        if (arg[i] != '|' && i != len-1)
            continue;

        if (arg[i] == '|')
            arg[i] = '\0';

        inputs[j++] = strdup(&arg[idx]);
        idx = i+1;
    }

    if (arg != NULL)
        free(arg);

    return j;
}

void parse(int argc, char** argv, cli_opt_t* opt, hdr_config_t* cfg)
{
	sscanf(argv[2], "%dx%d", &opt->norm_image_width, &opt->norm_image_height);
	opt->norm_expo_image = strdup(argv[4]);
	opt->frame_num = parse_input_frames(argv[8], opt->multi_expo_images);
	sscanf(argv[6], "%dx%d", &cfg->img_width, &cfg->img_height);
	opt->output_prefix = strdup(argv[10]);
	printf("norm_img width:%d,height:%d , expoimage:%s , framnum:%d, img_width:%d,height:%d,output prefix:%s\n" , 
			opt->norm_image_width, opt->norm_image_height , opt->norm_expo_image,
			opt->frame_num , cfg->img_width , cfg->img_height , opt->output_prefix);
}

#define ISP_CLIP(val, top, bot)     {if (val > (top)) val = (top); if (val < (bot)) val = (bot);}

void yuv2rgb(uint8_t *input, uint8_t *output, uint16_t iwidth, uint16_t iheight)
{
    uint16_t  iy, ix;
    uint8_t   *cur_Y = 0, *cur_U = 0, *cur_V = 0;
    uint8_t   *temp_rgb = 0;
    uint16_t  uv_width, uv_height;
    uint32_t  uv_pos;
    int16_t   tmp_var, red_val, green_val, blue_val;
    uint8_t   y_val, u_val, v_val;

    uv_width = iwidth;
    uv_height = iheight;

    cur_Y = input;
    cur_U = input + iwidth * iheight;
    cur_V = cur_U + iwidth * iheight;

    temp_rgb = output;


    for (iy = 0; iy < iheight; iy++)
    {
        for (ix = 0; ix < iwidth; ix++)
        {
            y_val = *(cur_Y++);
            // method 1: U&V adopting copy
            uv_pos = (iy)* uv_width + ix;

            u_val = *(cur_U + uv_pos);
            v_val = *(cur_V + uv_pos);

            //----------------------------------------
            tmp_var = (int16_t)(y_val + (((v_val)* 359 + 128) >> 8) - 180);
            ISP_CLIP(tmp_var, 255, 0);
            red_val = tmp_var;

            *(temp_rgb++) = (uint8_t)red_val;//R
            //----------------------------------------------
            tmp_var = (int16_t)(y_val - (((u_val)* 88 + 128) >> 8) - (((v_val)* 183 + 128) >> 8) + 136);
            ISP_CLIP(tmp_var, 255, 0);

            green_val = tmp_var;
            *(temp_rgb++) = (uint8_t)green_val;//G
            //--------------------------------------------
            tmp_var = (int16_t)(y_val + (((u_val)* 454 + 128) >> 8) - 227);
            ISP_CLIP(tmp_var, 255, 0);

            blue_val = tmp_var;
            *(temp_rgb++) = (uint8_t)blue_val; //B

        }//end ix
    }//end iy
}

void yvu420s_yuv420p(uint8_t* yvu420s, int width, int height, uint8_t *yuv420p)
{
    int r, c;
    int s = 0;
    int uv_w = width;
    int uv_h = height / 2;
    int y_pix_num = width*height;

    memcpy(yuv420p, yvu420s, sizeof(uint8_t)*y_pix_num);  //y

    for (r = 0; r < uv_h; r++)
    {
        for (c = 0; c < uv_w; c += 2)
        {
            yuv420p[y_pix_num + s] = yvu420s[y_pix_num + r*uv_w + c + 1];     //u
            yuv420p[y_pix_num * 5 / 4 + s] = yvu420s[y_pix_num + r*uv_w + c];   //v
            s++;
        }
    }

}

void yuv420p_yvu420s(uint8_t *yuv420p, int width, int height, uint8_t *yvu420s)
{
    int r,c, s;
    int uv_width = width/2;
    int uv_height = height/2;
    int pix_num = width*height;
    uint8_t *uv_ptr = NULL;
    uint8_t *u_ptr = NULL, *v_ptr = NULL;

    memcpy(yvu420s, yuv420p, pix_num*sizeof(uint8_t));  //copy y channel

    uv_ptr = yvu420s + pix_num;
    u_ptr = yuv420p + pix_num;
    v_ptr = u_ptr + pix_num/4;
    s = uv_width;

    for(r = 0; r < uv_height; r++)
    {
        for(c = 0; c < uv_width; c++)
        {
            uv_ptr[0] = v_ptr[r*s + c];     //copy v channel
            uv_ptr[1] = u_ptr[r*s + c];     //copy u channel
            uv_ptr += 2;                    //!
        }
    }
}

void yuv420tobmp(char* fileName,uint8_t *imgData,int32_t w,int32_t h)
{
    uint8_t *pYUV, *pRGB;
    int32_t i,j;

    pYUV = (uint8_t*)malloc(w * h * 3);
    pRGB = (uint8_t*)malloc(w * h * 3);

    for(i =0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            pYUV[i * w + j] = imgData[i * w + j];
            pYUV[i * w + j + w * h] = imgData[(i / 2) * w / 2 + (j / 2) + w * h];
            pYUV[i * w + j + w * h * 2] = imgData[(i / 2) * w / 2 + (j / 2) + w * h * 5 / 4];
        }
    }
    yuv2rgb(pYUV, pRGB, w, h); //yuv444 to rgb packed

    if (0)  //save yvu420s
    {
        FILE *fp = NULL;
        char yvu420s_file[0x100];
        uint8_t *yvu420s = (uint8_t*)malloc(w*h * 3 / 2);

        yuv420p_yvu420s(imgData, w, h, yvu420s);

        sprintf(yvu420s_file, "%s_out.NV21", fileName);
        fp = fopen(yvu420s_file, "wb");
        fwrite(yvu420s, 1, w*h * 3 / 2, fp);
        fclose(fp);

        free(yvu420s);
    }

    save_bmp(fileName,pRGB,w,h);
}

void save_as_bmp(char* fileName,uint8_t *imgData, int32_t w,int32_t h)
{
    uint8_t *pYUV, *pRGB, *pTmp;
    int32_t i,j;
    char outfile[MAX_FILENAME_LENGTH];
    sprintf(outfile, "%s.bmp", fileName);

    pYUV = (uint8_t*)malloc(w * h * 3);
    pRGB = (uint8_t*)malloc(w * h * 3);
    pTmp = (uint8_t*)malloc(w * h * 3 / 2);

    yvu420s_yuv420p(imgData,w,h,pTmp);

    for(i =0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            pYUV[i * w + j] = pTmp[i * w + j];
            pYUV[i * w + j + w * h] = pTmp[(i / 2) * w / 2 + (j / 2) + w * h];
            pYUV[i * w + j + w * h * 2] = pTmp[(i / 2) * w / 2 + (j / 2) + w * h * 5 / 4];            
        }
    }
    yuv2rgb(pYUV, pRGB, w, h); //yuv444 to rgb packed

    save_bmp(outfile,pRGB,w,h);

	free(pYUV);
	free(pRGB);
	free(pTmp);
}

void save_as_nv21(char* fileName,uint8_t *imgData,int32_t w,int32_t h)
{
    FILE *fp = NULL;
    char outfile[MAX_FILENAME_LENGTH];
    sprintf(outfile, "%s.nv21", fileName);

    fp = fopen(outfile, "wb");
    fwrite(imgData, 1, w*h*3/2, fp);   // yvu420s -> imgData

    fclose(fp);

    //free(yvu420s);
}
#endif
