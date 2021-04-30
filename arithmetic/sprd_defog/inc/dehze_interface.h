#ifndef _DEHAZE_INTERFACE_H_
#define _DEHAZE_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __linux__
#define EXPORT_INTERFACE
#else
#define EXPORT_INTERFACE __attribute__((visibility("default")))
#endif

	typedef struct _imgBuffer {
		unsigned char * bufR;
		unsigned char * bufG;
		unsigned char * bufB;
	}imgBuffer;

	EXPORT_INTERFACE void * dehaze_init(uint32 img_width, unsigned int img_height);
	EXPORT_INTERFACE int dehaze_yuv4202rgb(void * dehazeHandle, unsigned char * srcData);
	EXPORT_INTERFACE int dehaze_rgb2yuv420(void * dehazeHandle, imgBuffer * buf1, unsigned char * yuvData);
	EXPORT_INTERFACE int dehaze_copyRGBData(void *dehazeHandle, unsigned char * srcData);
	EXPORT_INTERFACE int dehaze_process(void * dehazeHandle, unsigned char * OutData);
	EXPORT_INTERFACE int dehaze_deinit(void *dehazeHandle);

#ifdef __cplusplus
};
#endif

#endif