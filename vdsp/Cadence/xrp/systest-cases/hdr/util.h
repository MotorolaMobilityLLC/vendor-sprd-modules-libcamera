#ifndef _PARSE_H_
#define _PARSE_H_

//#include "hdr_context.h"
#define MAX_FRAME_NUM 2
typedef struct
{
    int     frame_num;
    char*   norm_expo_image;
    int     norm_image_width;
    int     norm_image_height;
    char*   multi_expo_images[MAX_FRAME_NUM];
    char*   output_prefix;
} cli_opt_t;

#define MAX_FILENAME_LENGTH     (0x200)

void parse(int argc, char **argv, cli_opt_t *opt, hdr_config_t *cfg);
void yuv420tobmp(char* fileName,uint8_t *imgData,int32_t w,int32_t h);

void save_as_bmp(char* fileName,uint8_t *imgData,int32_t w,int32_t h);
void save_as_nv21(char* fileName,uint8_t *imgData,int32_t w,int32_t h);

#endif
