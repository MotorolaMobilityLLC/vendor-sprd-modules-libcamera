#ifndef __WT_INTERFACE_H__
#define __WT_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	int yuv_mode; //0: nv12 1: nv21
	int is_preview; //0:capture 1:preview
	int image_width_wide;
	int image_height_wide;
	int image_width_tele;
	int image_height_tele;
	void *otpbuf;
	int otpsize;
	int VCMup;//VCM上限
    int VCMdown; //VCM下限
} WT_inparam;

int WTprocess_Init(void **handle,WT_inparam *inparam);

int WTprocess_deinit(void *handle);

int WTprocess_function(void *handle,void *input_image_wide,void *input_image_tele,void *output_image,int VCM_cur_value,float ScaleRatio);


#ifdef __cplusplus
}
#endif

#endif
