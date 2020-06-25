/*************************************************************************
	> File Name: cpat_camera.h
	> Author:
	> Mail:
	> Created Time: Thu 18 Jun 2020 04:23:37 PM CST
 ************************************************************************/

#ifndef _CPAT_CAMERA_H
#define _CPAT_CAMERA_H

#define SENSOR_LIBRARY_PATH "libcamsensor.so"
#define SENSOR_CAPT_FAIL 1
#define SENSOR_DEFAULT_INFO "0xff"
#define SENSOR_CHECK_PARAM(a)                       \
        do {                                        \
            if(!a){                                 \
                SENSOR_LOGD("invalid param");       \
                goto exit;                          \
            }                                       \
        } while(0)
typedef int (*GET_CAM_INFO)(void);

int cpat_camera_init(int camera_id);
int cpat_camera_deinit(void);
int cpat_camera_startpreview(void) ;
int cpat_camera_stoppreview(void);
int cpat_read_cam_buf(void **pp_image_addr, int size, int *p_out_size);
int cpat_camera_get_camera_info(char *req, char *rsp);

#endif
