#ifndef _UTILITY_H_
#define _UTILITY_H_

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef signed char         int8;
typedef signed short        int16;
typedef signed int          int32;

void yvu420s_yuv420p(uint8* yvu420s, int width, int height, uint8 *yuv420p);
void yuv420p_yvu420s(uint8 *yuv420p, int width, int height, uint8 *yvu420s);
void yuv2rgb(uint8 *input, uint8 *output, uint16 iwidth, uint16 iheight);
void yuv420tobmp(char* filename, uint8 *imgdata, int32 w, int32 h, uint8 yvu420s_en);
//void image_resize(uint8 *input, int32 w_in, int32 h_in, uint8 *output, int32 w_out, int32 h_out, pixel_format_t fmt);


#endif