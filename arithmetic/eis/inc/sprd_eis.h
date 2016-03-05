#ifndef _SPRD_EIS_H_
#define _SPRD_EIS_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct
{
    double		dat[3][3];
} mat33;

typedef struct frame_in
{
	uint8_t*	frame_data;
	double		timestamp;
	double          zoom;
} vsInFrame;

typedef struct frame_out
{
	uint8_t*	frame_data;
	mat33		warp;
} vsOutFrame;

typedef struct vs_param
{
	uint16_t	src_w;
	uint16_t	src_h;
	uint16_t	dst_w;
	uint16_t	dst_h;
	int         	method;

	double		f;
	double		td;
	double		ts;
	double          wdx;
	double          wdy;
	double          wdz;
} vsParam;

typedef struct gyro_vs
{
	 double		t;
	 double		w[3];
}vsGyro;

typedef struct eis_info
{
	float 		zoom_ratio;
	long long	sleep_time;
} vsEisInfo;

typedef void * vsInst;
void video_stab_param_default(vsParam* param);
void video_stab_open(vsInst* inst, vsParam* param);
int video_stab_read(vsInst inst, vsOutFrame* frame);
void video_stab_write(vsInst inst, vsInFrame* frame, vsGyro* gyro, int gyro_num);
void video_stab_close(vsInst inst);

#ifdef __cplusplus
}
#endif

#endif
