/*************************************************************************
	> File Name: bbat_camera.h
	> Author: 
	> Mail: 
	> Created Time: Tue 23 Jun 2020 08:52:04 PM CST
 ************************************************************************/

#ifndef _BBAT_CAMERA_H
#define _BBAT_CAMERA_H

int bbat_back_flash(char *buf, int buf_len, char *rsp, int rsp_size);
int bbat_front_flash(char *buf, int buf_len, char *rsp, int rsp_size);
int bbat_mipicam(char *buf, int buf_len, char *rsp, int rsp_size);

#endif
