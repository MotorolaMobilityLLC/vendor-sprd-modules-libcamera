#include <sys/types.h>

void *ispvideo_GetIspHandle(void)
{
	return NULL;
}

void send_img_data(uint32_t format, uint32_t width, uint32_t height,
		   char *imgptr, int imagelen)
{
}

void send_capture_complete_msg()
{
	int x = 0;
	x = x;
}

void send_capture_data(uint32_t format, uint32_t width, uint32_t height,
		       char *ch0_ptr, int ch0_len, char *ch1_ptr, int ch1_len,
		       char *ch2_ptr, int ch2_len)
{
}

int ispvideo_RegCameraFunc(uint32_t cmd, int (*func) (uint32_t, uint32_t))
{
	return 0;
}

void stopispserver()
{
}

void startispserver()
{
}

void setispserver(void *handle)
{
}
