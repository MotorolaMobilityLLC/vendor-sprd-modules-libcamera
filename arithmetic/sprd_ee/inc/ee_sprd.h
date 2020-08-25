#ifndef __EE_SPRD_H__
#define __EE_SPRD_H__

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		void *data;			/*image buffer*/
		int fd;				/*image fd*/
	} sprd_ee_buffer_vdsp;

	typedef struct {
		void *tuning_param;		/*tuning buffer*/
		int tuning_size;		/*tuning size*/
		int mode_idx;			/*mode index*/
		int scene_idx;			/*scene index*/
		int level_idx[2];		/*level index array*/
		int level_weight[2];		/*level weight array*/
		int level_num;			/*level number*/
		int crop_width;			/*image crop width*/
		int crop_height;		/*image crop height*/
		void *scene_map_buffer;		/*scene map buffer*/
		unsigned int face_stable; //new, whether contain face
		unsigned short face_num;//new, count of face
	} sprd_ee_tuning_param;

	/****** cpu interface ******/
	/*
	usage: init ee instance
	return value: handle 
	@param: 
	-width: image width
	-height: image height
	*/
	void *sprd_ee_init(int width, int height);
	/*
	usage: ee process interface
	return value: 0 is ok, other value is failed
	@param: 
	-handle: ee instance
	-imgIn: input image buffer
	-imgOut: output image buffer
	-tuningParam: tuning param
	-width: image width
	-height: image height
	*/
	int sprd_ee_process(void *handle, void *imgIn, void *imgOut, sprd_ee_tuning_param *tuningParam, int width, int height);
	/*
	Usage: deinit ee instance 
	return value: 0 is ok, other value is failed
	@param: 
	-handle: ee instance
	*/
	int sprd_ee_deinit(void *handle);

	/****** vdsp interface ******/
#if defined __linux__
	/*
	usage: init ee instance of vdsp
	return value: handle 
	@param: 
	-width: image width
	-height: image height
	*/
	void *sprd_ee_init_vdsp(int width, int height);
	/*
	usage: ee process interface of vdsp
	return value: 0 is ok, other value is failed
	@param: 
	-handle: ee instance
	-imgIn: input image buffer
	-imgOut: output image buffer
	-tuningParam: tuning param
	-width: image width
	-height: image height
	*/
	int sprd_ee_process_vdsp(void *handle, sprd_ee_buffer_vdsp *imgIn, sprd_ee_buffer_vdsp *imgOut, sprd_ee_tuning_param *tuningParam, int width, int height);
	/*
	Usage: deinit ee instance of vdsp
	return value: 0 is ok, other value is failed
	@param: 
	-handle: ee instance
	*/
	int sprd_ee_deinit_vdsp(void *handle);
#endif

#ifdef __cplusplus
}
#endif

#endif
