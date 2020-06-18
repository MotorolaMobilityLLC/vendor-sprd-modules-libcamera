/*************************************************************************
	> File Name: cpat_camera.h
	> Author:
	> Mail:
	> Created Time: Thu 18 Jun 2020 04:23:37 PM CST
 ************************************************************************/

#ifndef _CPAT_CAMERA_H
#define _CPAT_CAMERA_H

int cpat_camera_init(int camera_id);
int cpat_camera_deinit(void);
int cpat_camera_startpreview(void) ;
int cpat_camera_stoppreview(void);
int cpat_read_cam_buf(void **pp_image_addr, int size, int *p_out_size);


#endif
