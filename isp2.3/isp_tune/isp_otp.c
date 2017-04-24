/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>

#define LOG_TAG "isp_otp"
#include <cutils/log.h>
#if 0				/*Solve compile problem */
#include <hardware/camera.h>
#endif
#include "cmr_oem.h"

#include "isp_type.h"
#include "isp_otp.h"
#include "isp_video.h"

#if 0				/*Solve compile problem */

//#define SCI_Trace_Dcam(...)
#define SCI_Trace_Dcam(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define DATA_SIZE                (10*1024)

#define otp_data_start_index  40
#define OTP_DATA_LENG            sizeof(struct otp_setting)

enum otp_cmd_type {
	OTP_CALIBRATION_CMD = 0,
	OTP_CTRL_PARAM_CMD,
	OTP_ACTUATOR_I2C_CMD,
	OTP_SENSOR_I2C_CMD,
	OTP_ROM_DATA_CMD,
	OTP_START_3A_CMD,
	OTP_STOP_3A_CMD,
	OTP_GOLDEN_CMD,
	OTP_WRITE_SN_CMD,
	OTP_RELOAD_ON_CMD,
	OTP_RELOAD_OFF_CMD,
	OTP_CAMERA_POWERON_CMD,
	OTP_CAMERA_POWEROFF_CMD,
	OTP_START_AF_CMD,
	OTP_MAX_CMD
};

struct otp_setting {
	cmr_u32 shutter;	// 4 bytes 1us��10s
	cmr_u32 line;		// 4 bytes 0��8000
	cmr_u32 gain;		// 4 bytes 0��1023
	cmr_u32 frame_rate;	// 4 bytes 0��1023
	cmr_u32 i2c_clock;	// 4 bytes 100��2000 khz
	cmr_u32 main_clock;	// 4 bytes MHz
	cmr_u32 image_pattern;	// 4 bytes RGB_GR:0x00
	cmr_u32 flip;		// 4 bytes 0��no flip;  1:flip
	cmr_u32 set_ev;		// 0--14
	cmr_u32 set_ae;		// 0 lock 1 unlock
	cmr_u32 set_af_mode;	// 0 no af ,1 normal af ,2 caf
	cmr_u32 set_af_position;	// 0 ~ 1023
	cmr_u32 checksum;	// 4 bytes
};

static cmr_u8 s_isp_otp_src_data[DATA_SIZE];
static cmr_u8 s_isp_otp_rec_data[DATA_SIZE];

//#define OTP_DATA_TESTLOG
static cmr_u32 otp_data_offset = 0;	//3792;
#define otp_data_test_length (256/16)	//3120

static cmr_u32 otp_settings_buf[(otp_data_start_index + OTP_DATA_LENG) / 4];

static void *p_camera_device = NULL;
cmr_u32 A3_on = 0;

static const char *cmdstring[] = {
	"OTP_CALIBRATION_CMD",
	"OTP_CTRL_PARAM_CMD",
	"OTP_ACTUATOR_I2C_CMD",
	"OTP_SENSOR_I2C_CMD",
	"OTP_ROM_DATA_CMD",
	"OTP_START_3A_CMD",
	"OTP_STOP_3A_CMD",
	"OTP_GOLDEN_CMD",
	"OTP_WRITE_SN_CMD",
	"OTP_RELOAD_ON_CMD",
	"OTP_RELOAD_OFF_CMD",
	"OTP_CAMERA_POWERON_CMD",
	"OTP_CAMERA_POWEROFF_CMD",
	"OTP_MAX_CMD"
};

cmr_s32 memcpy_ex(cmr_u8 type, void *dest, cmr_u8 * src, cmr_u32 lenth)
{
	cmr_u32 i = 0;
	cmr_u8 i_type = type;
	cmr_u8 *u8_dest = NULL;
	cmr_u16 *u16_dest = NULL;
	cmr_u32 *u32_dest = NULL;

	if (NULL == dest || NULL == src) {
		return -1;
	}

	switch (i_type) {
	case 1:		//byte to byte
		u8_dest = (cmr_u8 *) dest;
		for (i = 0; i < lenth; i++) {
			u8_dest[i] = src[i];
		}
		break;
	case 2:		//byte to word
		u16_dest = (cmr_u16 *) dest;
		for (i = 0; i < lenth; i++) {
			(*u16_dest) |= src[i] << ((i) * 8);
		}
		break;
	case 4:		//byte to dword little edd [b0 b1 b2 b3] -->[b3 b2 b1 b0]
		u32_dest = (cmr_u32 *) dest;
		*u32_dest = 0;
		for (i = 0; i < lenth; i++) {
			(*u32_dest) |= src[i] << ((i) * 8);
		}
		break;
	case 5:		// dword[4 bytes] to dword  [b0 b1 b2 b3] -->[b0 b1 b2 b3]
		u32_dest = (cmr_u32 *) dest;
		for (i = 0; i < lenth; i++) {
			(*u32_dest) |= src[i] << ((lenth - 1 - i) * 8);
		}
		break;
	default:
		break;
	}
	return 0;
}

cmr_s32 compare(cmr_u8 * a_data_buf, cmr_u8 * b_data_buf, cmr_s32 length)
{
	cmr_s32 value = -1;
	cmr_s32 i = 0;
	cmr_u8 *adst = a_data_buf;
	cmr_u8 *bdst = b_data_buf;

	if (!adst || !bdst) {
		value = -1;
		SCI_Trace_Dcam("%s:  parameter is null ", __func__);
		goto RET;
	}

	for (i = 0; i < length; i++) {
		if (adst[i] == bdst[i]) {
			value = 0;
		} else if (adst[i] > bdst[i]) {
			value = 1;
			SCI_Trace_Dcam("%s:addr count =%d  adst[%d] =%d  is bigger than dst[%d]=%d", __func__, i, i, adst[i], i, bdst[i]);
			break;
		} else {
			value = -1;
			SCI_Trace_Dcam("%s:addr count =%d  adst[%d] =%d  is little than dst[%d]=%d", __func__, i, i, adst[i], i, bdst[i]);
			break;
		}
	}

	SCI_Trace_Dcam("%s: is %s   count =%d  result value=%d !", __func__, (value == 0) ? "ok!" : "error ", i, value);
RET:
	return value;
}

cmr_int Sensor_Ioctl(SENSOR_IOCTL_CMD_E sns_cmd, void *arg)
{
	SENSOR_IOCTL_FUNC_PTR func_ptr;
	SENSOR_IOCTL_FUNC_TAB_T *func_tab_ptr;
	cmr_uint temp;
	cmr_u32 ret = CMR_CAMERA_SUCCESS;

	/* ISP can't call the function of OEM */
	//struct sensor_drv_context *sensor_cxt = sensor_get_dev_cxt();
	struct sensor_drv_context *sensor_cxt = NULL;

	if (SENSOR_IOCTL_GET_STATUS != sns_cmd) {
		SCI_Trace_Dcam("cmd = %d, arg = %p.\n", sns_cmd, arg);
	}

	SENSOR_DRV_CHECK_ZERO(sensor_cxt);

	/* ISP can't call the function of OEM */
	if (0 /*!sensor_is_init_common(sensor_cxt) */ ) {
		SCI_Trace_Dcam("warn:sensor has not init.\n");
		return SENSOR_OP_STATUS_ERR;
	}

	if (SENSOR_IOCTL_CUS_FUNC_1 > sns_cmd) {
		SCI_Trace_Dcam("error:can't access internal command !\n");
		return SENSOR_SUCCESS;
	}

	if (PNULL == sensor_cxt->sensor_info_ptr) {
		SCI_Trace_Dcam("error:No sensor info!");
		return -1;
	}

	func_tab_ptr = sensor_cxt->sensor_info_ptr->ioctl_func_tab_ptr;
#if __WORDSIZE == 64
	temp = *(cmr_uint *) ((cmr_uint) func_tab_ptr + sns_cmd * S_BIT_3);
#else
	temp = *(cmr_uint *) ((cmr_uint) func_tab_ptr + sns_cmd * S_BIT_2);
#endif
	func_ptr = (SENSOR_IOCTL_FUNC_PTR) temp;

	if (PNULL != func_ptr) {
		/* ISP can't call the function of OEM */
		//ret = func_ptr((cmr_uint)arg);
	}
	return ret;
}

static cmr_s32 Sensor_isRAW(void)
{
	/* ISP can't call the function of OEM */
	//SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = NULL;
	if (sensor_info_ptr && sensor_info_ptr->image_format == SENSOR_IMAGE_FORMAT_RAW)
		return 1;
	else
		return 0;
}

cmr_s32 send_otp_data_to_isp(cmr_u32 start_addr, cmr_u32 data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_u32 i = 0;
	cmr_u32 j = 0;
	cmr_u32 otp_start_addr = 0;
	cmr_u32 otp_data_len = 0;
	cmr_u8 *dst = &s_isp_otp_src_data[0];
	SENSOR_VAL_T val;
	//SENSOR_OTP_PARAM_T param_ptr;
	struct _sensor_otp_param_tag param_ptr;
	cmr_s32 otp_start_addr_emprty = -1;

	SCI_Trace_Dcam("%s data_size =%d ", __func__, data_size);

	//memset zero
	for (j = 0; j < DATA_SIZE; j++) {
		s_isp_otp_rec_data[j] = 0;
		s_isp_otp_src_data[j] = 0;
	}

	//initial s_isp_otp_src_data array
	for (i = 0; i < data_size; i++) {
		*dst++ = *data_buf++;
	}

	//write the calibration data to sensor
	//otp_start_addr = ( start_addr == 0 ) ? (otp_data_offset):(start_addr);
	//otp_data_len   = ( start_addr == 0 ) ? (otp_data_test_length):(data_size);
	otp_start_addr = start_addr;
	otp_data_len = data_size;
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, otp_start_addr, otp_data_len);

#ifdef OTP_DATA_TESTLOG
	if (start_addr == 0) {
		SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, otp_start_addr, otp_data_len);
		for (j = 0; j < otp_data_len; j++) {
//                      SCI_Trace_Dcam("%s data_buf[%d] =0x%02x ",__func__,j,s_isp_otp_src_data[j]);
		}
		//find if it is exist
		do {
			SCI_Trace_Dcam("test area %s:addr 0x%x cnt %d", __func__, otp_start_addr, otp_data_len);
			param_ptr.start_addr = otp_start_addr;
			param_ptr.len = otp_data_len;
			param_ptr.buff = s_isp_otp_rec_data;
			val.type = SENSOR_VAL_TYPE_READ_OTP;
			val.pval = &param_ptr;
			ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, &val);

			if (memcmp(s_isp_otp_rec_data, s_isp_otp_src_data, otp_data_len) == 0) {	//already exist now
				otp_data_offset = otp_start_addr;
				SCI_Trace_Dcam("find area:addr 0x%x cnt %d", otp_start_addr, otp_data_len);
				return 0;
			}

			for (j = 0; otp_start_addr_emprty == -1 && j < otp_data_len; j++) {
				if (s_isp_otp_rec_data[j] != 0) {
					break;
				}
			}
			if (j >= otp_data_len) {
				otp_start_addr_emprty = otp_start_addr;
				SCI_Trace_Dcam("first empty area:addr 0x%x cnt %d", otp_start_addr, otp_data_len);
				break;
			}

			otp_start_addr += otp_data_len;
		} while (otp_start_addr < 8192);

		//create a new
//              if(otp_start_addr >= 8192){
		if (otp_start_addr_emprty != -1) {
			otp_data_offset = otp_start_addr = otp_start_addr_emprty;
		} else {
			SCI_Trace_Dcam("found none empty area!");
			return -1;
		}
//              }
	}
#endif

	i = 0;
	do {
		SCI_Trace_Dcam("i = %d", i);

		param_ptr.start_addr = otp_start_addr;
		param_ptr.len = otp_data_len;
		param_ptr.buff = s_isp_otp_src_data;
		val.type = SENSOR_VAL_TYPE_WRITE_OTP;
		val.pval = &param_ptr;
		ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, &val);

		param_ptr.start_addr = otp_start_addr;
		param_ptr.len = otp_data_len;
		param_ptr.buff = s_isp_otp_rec_data;
		val.type = SENSOR_VAL_TYPE_READ_OTP;
		val.pval = &param_ptr;
		ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, &val);

		ret = compare(s_isp_otp_src_data, s_isp_otp_rec_data, otp_data_len);
	} while ((ret != 0) && (i++ < 1));

	if (!ret) {
		/* ISP can't call the function of OEM */
		//camera_calibrationconfigure_save (otp_start_addr, otp_data_len);
	}
#ifdef OTP_DATA_TESTLOG
	for (j = 0; j < otp_data_len; j++) {
//              SCI_Trace_Dcam("%s data_buf[%d] =0x%02x ",__func__,j,s_isp_otp_rec_data[j]);
	}

#endif
	return ret;
}

cmr_s32 write_otp_calibration_data(cmr_u32 data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 otp_data_len = 0;
	cmr_s32 write_cnt = 0;
	cmr_s32 addr = 0;
	cmr_s32 addr_cnt = 0;
	cmr_s32 i = 0;
	cmr_u8 *log_ptr = data_buf;

/*	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}*/
	SCI_Trace_Dcam("%s:data_size %d", __func__, data_size);
	memcpy_ex(4, &otp_data_len, &data_buf[0], 4);
	memcpy_ex(4, &addr_cnt, &data_buf[otp_data_start_index], 4);
	write_cnt = otp_data_len - 8;
	SCI_Trace_Dcam("%s:addr_cnt  %d", __func__, addr_cnt);
	for (i = 0; i < addr_cnt; i++) {
		memcpy_ex(4, &addr, &data_buf[otp_data_start_index + 4 + (i * 4)], 4);
		SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, addr, write_cnt);
		ret = send_otp_data_to_isp(addr, write_cnt, &data_buf[otp_data_start_index + 4 + (4 * addr_cnt)]);
		//write the calibration data to sensor
	}
	return ret;
}

cmr_s32 write_sensor_shutter(cmr_u32 shutter_val)
{
	cmr_s32 ret = 0;
	cmr_u32 line_time = 0x00;
	cmr_u32 frame_line = 0x00;
	cmr_uint expsure_line = 0x00;
	cmr_u32 dummy = 0x00;

	/* ISP can't call the function of OEM */
	//struct sensor_drv_context *sensor_cxt = sensor_get_dev_cxt();
	//SENSOR_EXP_INFO_T *psensorinfo = Sensor_GetInfo();
	struct sensor_drv_context *sensor_cxt = NULL;
	SENSOR_EXP_INFO_T *psensorinfo = NULL;
	cmr_uint mode;

	/* ISP can't call the function of OEM */
	//sensor_get_mode_common(sensor_cxt, &mode);

	line_time = psensorinfo->sensor_mode_info[mode].line_time;
	frame_line = psensorinfo->sensor_mode_info[mode].frame_line;
	if (line_time > 0) {
		expsure_line = (shutter_val * 10) / line_time;

		if (expsure_line < frame_line) {
			dummy = frame_line - expsure_line;
		}
		expsure_line = expsure_line & 0x0000ffff;
		expsure_line |= (dummy << 0x10) & 0x0fff0000;
		expsure_line |= (mode << 0x1c) & 0xf0000000;
		ret = Sensor_Ioctl(SENSOR_IOCTL_WRITE_EV, (void *)expsure_line);
	}
	SCI_Trace_Dcam("%s:shutter_val=%d line_time=%d frame_line=%d expsure_line=%ld\n", __func__, shutter_val, line_time, frame_line, expsure_line);

	return ret;
}

cmr_s32 write_sensor_line(cmr_u32 line_val)
{
	cmr_s32 ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, line_val);
	//ret = _hi544_set_shutter(line_val);

	return ret;
}

cmr_s32 write_sensor_gain(cmr_u32 gain_val)
{
	cmr_s32 ret = 0;
	SENSOR_VAL_T val;

	SCI_Trace_Dcam("%s: gain %d\n", __func__, gain_val);

	val.type = SENSOR_VAL_TYPE_WRITE_OTP_GAIN;
	val.pval = &gain_val;

	ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&val);

	return ret;
}

cmr_s32 write_i2c_clock(cmr_u32 clk)
{
	cmr_s32 ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, clk);

//      _Sensor_Device_SetI2CClock(clk);
	return ret;
}

cmr_s32 write_mclk(cmr_u32 mclk)
{
	cmr_s32 ret = 0;

	SCI_Trace_Dcam("%s:0x%x\n", __func__, mclk);
//      Sensor_SetMCLK(mclk);
	return ret;
}

cmr_s32 write_image_pattern(cmr_u32 pattern)
{
	cmr_s32 ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, pattern);

	return ret;
}

cmr_s32 write_sensor_flip(cmr_u32 flip)
{
	cmr_s32 ret = 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, flip);
//      ret = _hi544_set_flip(flip);
	return ret;
}

cmr_s32 write_position(cmr_handle handler, cmr_u32 pos)
{
	cmr_s32 ret = 0;

	SCI_Trace_Dcam("%s:0x%x\n", __func__, pos);

	//ret = _hi544_write_af(pos);
	//ret = Sensor_Ioctl(SENSOR_IOCTL_AF_ENABLE, (void *)pos);
	ret = isp_ioctl(handler, ISP_CTRL_SET_AF_POS, (void *)&pos);

	return ret;
}

cmr_s32 write_start_af(cmr_handle handler)
{
	cmr_s32 ret = 0;
	struct isp_af_win isp_af_param;
	cmr_s32 i = 0;
	cmr_s32 zone_cnt = 1;
	cmr_s32 win_width = 100;
	cmr_s32 win_height = 100;
	cmr_s32 win_x = 0;
	cmr_s32 win_y = 0;
	SENSOR_EXP_INFO_T *exp_info;
	cmr_u32 mode = 0;
	cmr_s32 image_width = 0;
	cmr_s32 image_height = 0;

	if (!Sensor_isRAW())
		return 0;

	SCI_Trace_Dcam("%s:0x%p\n", __func__, handler);

	/* ISP can't call the function of OEM */
	//Sensor_GetMode(&mode);
	//exp_info = Sensor_GetInfo();
	exp_info = NULL;
	image_width = exp_info->sensor_mode_info[mode].trim_width;
	image_height = exp_info->sensor_mode_info[mode].trim_height;

	win_width = (win_width >> 2) << 2;
	win_height = (win_height >> 2) << 2;
	win_x = (image_width - win_width) / 2;
	win_y = (image_height - win_height) / 2;
	win_x = (win_x >> 1) << 1;
	win_y = (win_y >> 1) << 1;

	memset(&isp_af_param, 0, sizeof(struct isp_af_win));
	isp_af_param.mode = ISP_FOCUS_TRIG;
	isp_af_param.valid_win = zone_cnt;

	for (i = 0; i < zone_cnt; i++) {
		isp_af_param.win[i].start_x = win_x;
		isp_af_param.win[i].start_y = win_y;
		isp_af_param.win[i].end_x = win_x + win_width - 1;
		isp_af_param.win[i].end_y = win_y + win_height - 1;

		SCI_Trace_Dcam("af_win num:%d, x:%d y:%d e_x:%d e_y:%d",
			       zone_cnt, isp_af_param.win[i].start_x, isp_af_param.win[i].start_y, isp_af_param.win[i].end_x, isp_af_param.win[i].end_y);
	}

	ret = isp_ioctl(handler, ISP_CTRL_AF, &isp_af_param);

	return ret;
}

cmr_s32 write_ev(cmr_handle handler, cmr_u32 ev)
{
	cmr_s32 ret = 0;

	if (!Sensor_isRAW())
		return 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, ev);
	ret = isp_ioctl(handler, ISP_CTRL_SET_LUM, (void *)&ev);

	return ret;
}

cmr_s32 write_ae(cmr_handle handler, cmr_u32 ae)
{
	cmr_s32 ret = 0;
	cmr_u32 cmd_param = ae;

	if (!Sensor_isRAW())
		return 0;

	SCI_Trace_Dcam("%s:0x%x\n", __func__, ae);
	if (0 == ae) {		//lock
		ret = isp_ioctl(handler, ISP_CTRL_AE_FORCE_CTRL, (void *)&cmd_param);
	} else {		//unlock
		ret = isp_ioctl(handler, ISP_CTRL_AE_FORCE_CTRL, (void *)&cmd_param);
	}

	return ret;
}

cmr_s32 write_frame_rate(cmr_handle handler, cmr_u32 rate)
{
	UNUSED(handler);
	cmr_s32 ret = 0;
	cmr_u32 cmd_param = rate;

	if (!Sensor_isRAW())
		return 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, rate);
	//ret = isp_ioctl(handler, ISP_CTRL_VIDEO_MODE, (void*)&cmd_param);

	return ret;
}

cmr_s32 write_af_mode(cmr_handle handler, cmr_u32 af_mode)
{
	cmr_s32 ret = 0;
	struct isp_af_win isp_af_param;
	cmr_s32 i = 0;
	cmr_s32 zone_cnt = 1;
	cmr_s32 win_width = 100;
	cmr_s32 win_height = 100;
	cmr_s32 win_x = 0;
	cmr_s32 win_y = 0;
	SENSOR_EXP_INFO_T *exp_info;
	cmr_u32 mode = 0;
	cmr_s32 image_width = 0;
	cmr_s32 image_height = 0;

	if (!Sensor_isRAW())
		return 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, af_mode);

	/*Sensor_GetMode(&mode);
	   exp_info = Sensor_GetInfo();
	   image_width = exp_info->sensor_mode_info[mode].trim_width;
	   image_height = exp_info->sensor_mode_info[mode].trim_height;

	   win_width = (win_width>>2)<<2;
	   win_height = (win_height>>2)<<2;
	   win_x = (image_width - win_width) / 2 ;
	   win_y = (image_height - win_height) / 2;
	   win_x = (win_x >>1)<<1;
	   win_y = (win_y>>1)<<1;

	   memset(&isp_af_param, 0, sizeof(struct isp_af_win));
	   if (0 == af_mode) {
	   isp_af_param.mode = ISP_FOCUS_TRIG;
	   } else if (1 == af_mode) {
	   isp_af_param.mode = ISP_FOCUS_MANUAL;
	   } else if (2 == af_mode) {
	   isp_af_param.mode = ISP_FOCUS_CONTINUE;
	   }
	   isp_af_param.valid_win = zone_cnt;

	   for (i = 0; i < zone_cnt; i++) {
	   isp_af_param.win[i].start_x = win_x;
	   isp_af_param.win[i].start_y = win_y;
	   isp_af_param.win[i].end_x = win_x + win_width - 1;
	   isp_af_param.win[i].end_y = win_y + win_height - 1;

	   SCI_Trace_Dcam("af_win num:%d, x:%d y:%d e_x:%d e_y:%d",
	   zone_cnt,
	   isp_af_param.win[i].start_x,
	   isp_af_param.win[i].start_y,
	   isp_af_param.win[i].end_x,
	   isp_af_param.win[i].end_y);
	   }
	   ret = isp_ioctl(handler, ISP_CTRL_AF_MODE, &isp_af_param); */

	if (0 == af_mode) {
		mode = ISP_FOCUS_TRIG;
	} else if (1 == af_mode) {
		mode = ISP_FOCUS_MANUAL;
	} else if (2 == af_mode) {
		mode = ISP_FOCUS_CONTINUE;
	}
	ret = isp_ioctl(handler, ISP_CTRL_AF_MODE, &mode);

	return ret;
}

cmr_s32 write_otp_ctrl_param(cmr_handle handler, cmr_u32 data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 otp_settings_index = 40;
	struct otp_setting write_otp_settings;
	cmr_u32 index = 0;
	cmr_u8 *log_ptr = data_buf;

	while (index < data_size) {
		SCI_Trace_Dcam("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}

	if (data_buf != NULL) {
		memcpy_ex(4, &write_otp_settings.shutter, &data_buf[otp_settings_index], 4);
		memcpy_ex(4, &write_otp_settings.line, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.gain, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.frame_rate, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.i2c_clock, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.main_clock, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.image_pattern, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.flip, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.set_ev, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.set_ae, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.set_af_mode, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.set_af_position, &data_buf[otp_settings_index += 4], 4);
		memcpy_ex(4, &write_otp_settings.checksum, &data_buf[otp_settings_index += 4], 4);

		SCI_Trace_Dcam("shutter = %d , line = %d ,gain = %d,  frame_rate = %d",
			       write_otp_settings.shutter, write_otp_settings.line, write_otp_settings.gain, write_otp_settings.frame_rate);
		SCI_Trace_Dcam("i2c_clock = %d , main_clock = %d, image_pattern = %d , flip = %d",
			       write_otp_settings.i2c_clock, write_otp_settings.main_clock, write_otp_settings.image_pattern, write_otp_settings.flip);
		SCI_Trace_Dcam(" set_ev = %d, set_ae = %d , set_af_mode = %d set_af_position = %d",
			       write_otp_settings.set_ev, write_otp_settings.set_ae, write_otp_settings.set_af_mode, write_otp_settings.set_af_position);
		//SCI_Trace_Dcam(" checksum = 0x%x", write_otp_settings.checksum);

		ret = write_sensor_shutter(write_otp_settings.shutter);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_sensor_line(write_otp_settings.line);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_sensor_gain(write_otp_settings.gain);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_i2c_clock(write_otp_settings.i2c_clock);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_mclk(write_otp_settings.main_clock);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_image_pattern(write_otp_settings.image_pattern);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_sensor_flip(write_otp_settings.flip);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}

		ret = write_position(handler, write_otp_settings.set_af_position);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_ev(handler, write_otp_settings.set_ev);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_ae(handler, write_otp_settings.set_ae);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_af_mode(handler, write_otp_settings.set_af_mode);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
		ret = write_frame_rate(handler, write_otp_settings.frame_rate);
		if (ret) {
			SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
		}
	}
	return ret;
}

cmr_s32 write_otp_actuator_i2c(cmr_u32 data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 otp_data_len = 0;
	cmr_s32 write_cnt = 0;
	cmr_s32 addr = 0;
	cmr_s32 i = 0;
	cmr_u32 index = 0;
	cmr_u8 *log_ptr = data_buf;
	cmr_u8 *read_ptr;
	cmr_u32 reg_val = 0;
	SENSOR_VAL_T io_val;
	cmr_u32 param;

	while (index < data_size) {
		SCI_Trace_Dcam("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}
	memcpy_ex(4, &otp_data_len, &data_buf[0], 4);
	write_cnt = (otp_data_len - 4) / 4;

	memcpy_ex(4, &addr, &data_buf[otp_data_start_index], 4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, addr, write_cnt);

	read_ptr = &data_buf[otp_data_start_index + 4];
	//write to sensor's rom data
	io_val.type = SENSOR_VAL_TYPE_WRITE_VCM;
	io_val.pval = &param;
	for (i = 0; i < write_cnt; i++) {
		memcpy_ex(4, &reg_val, read_ptr, 4);

		param = (addr << 16) | (reg_val & 0xffff);
		ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&io_val);
		if (0 != ret) {
			SCI_Trace_Dcam("%s failed  addr = 0x%x, val=0x%x", __func__, addr, reg_val);
			break;
		}

		addr++;
		read_ptr += 4;
	}

	return ret;
}

cmr_s32 write_otp_sensor_i2c(cmr_u32 data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 otp_data_len = 0;
	cmr_s32 write_cnt = 0;
	cmr_s32 addr = 0;
	cmr_s32 i = 0;
	cmr_u32 index = 0;
	cmr_u8 *log_ptr = data_buf;
	cmr_u8 *read_ptr;
	cmr_u32 reg_val = 0;

	while (index < data_size) {
		SCI_Trace_Dcam("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}
	memcpy_ex(4, &otp_data_len, &data_buf[0], 4);
	write_cnt = (otp_data_len - 4) / 4;

	memcpy_ex(4, &addr, &data_buf[otp_data_start_index], 4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, addr, write_cnt);
	read_ptr = &data_buf[otp_data_start_index + 4];
	//write to sensor's rom data
	for (i = 0; i < write_cnt; i++) {
		memcpy_ex(4, &reg_val, read_ptr, 4);

		/*ret = dcam_i2c_write_reg_16((cmr_u16)addr,(cmr_u16)reg_val); */
		/* ISP can't call the function of OEM */
		//Sensor_WriteReg(addr,reg_val);

		addr++;
		read_ptr += 4;
	}
	return ret;
}

cmr_s32 write_otp_rom_data(cmr_u32 data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 otp_data_len = 0;
	cmr_s32 write_cnt = 0;
	cmr_s32 addr = 0;
	cmr_s32 i = 0;
	cmr_s32 write_val = 0;
	cmr_u32 index = 0;
	cmr_u8 *log_ptr = data_buf;

	while (index < data_size) {
		SCI_Trace_Dcam("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}

	memcpy_ex(4, &otp_data_len, &data_buf[0], 4);
	write_cnt = (otp_data_len - 4) / 4;

	memcpy_ex(4, &addr, &data_buf[otp_data_start_index], 4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, addr, write_cnt);
	//write to sensor's rom data
	return ret;
}

cmr_s32 write_otp_start_3A(cmr_handle handler, cmr_u32 data_size, cmr_u8 * data_buf)
{
	UNUSED(data_size);
	UNUSED(data_buf);
	cmr_s32 ret = 0;
	cmr_u32 cmd_param = 0;
	if (!Sensor_isRAW())
		return 0;

	SCI_Trace_Dcam("%s\n", __func__);

	isp_ioctl(handler, ISP_CTRL_START_3A, (void *)&cmd_param);
	A3_on = 1;

	return ret;
}

cmr_s32 write_otp_stop_3A(cmr_handle handler, cmr_u32 data_size, cmr_u8 * data_buf)
{
	UNUSED(data_size);
	UNUSED(data_buf);
	cmr_s32 ret = 0;
	cmr_u32 cmd_param = 0;
	if (!Sensor_isRAW())
		return 0;

	SCI_Trace_Dcam("%s\n", __func__);

	isp_ioctl(handler, ISP_CTRL_STOP_3A, (void *)&cmd_param);
	write_af_mode(handler, 1);
	A3_on = 0;

	return ret;
}

cmr_s32 write_otp_sn(cmr_u32 data_size, cmr_u8 * data_buf)
{
	UNUSED(data_size);
	cmr_s32 ret = 0;
	cmr_s32 fd = -1;
	cmr_s32 size = 0;
	cmr_s32 data_len = 0;
	//char array[] = {'8','9','8','9','8','9','8','9'};
	cmr_u8 *sn_ptr = &data_buf[otp_data_start_index];

	fd = open("/dev/mmcblk0p16", O_CREAT | O_RDWR, 0);
	if (-1 == fd) {
		return -1;
	}
	memcpy_ex(4, &data_len, &data_buf[0], 4);
	size = write(fd, &data_buf[otp_data_start_index], data_len);
	if (size != data_len) {
		close(fd);
		return -1;
	}
	/*size = write(fd, array,8);
	   if (size != 8) {
	   return -1;
	   } */
	close(fd);
	return ret;
}

cmr_s32 _start_camera(void **pdev)
{
	cmr_s32 ops_status = 0;
#if (MINICAMERA != 1)
	struct hw_module_t *module;
	hw_device_t *tmp = NULL;

	if (*pdev == NULL) {
		ISP_LOGW("pdev->common.methods->open \n");

		/* ISP can't call the function of OEM */
		//ops_status = hw_get_module(CAMERA_HARDWARE_MODULE_ID, (const hw_module_t**)&module);
		if (ops_status) {
			ISP_LOGE("\n hw_get_module %d fail ", ops_status);
			return -1;
		}
		ops_status = module->methods->open(module, 0, (hw_device_t **) & tmp);
		if (ops_status != 0) {
			ISP_LOGE("\n open %d fail ", 0);
			if (tmp)
				tmp->close((hw_device_t *) tmp);	//avoid next time retry fail
			*pdev = NULL;
			return -1;
		}

		*pdev = tmp;
	}
#endif
	return ops_status;

}

cmr_s32 _stop_camera(void *pdev)
{
	cmr_s32 ops_status = 0;
#if (MINICAMERA != 1)
	if (pdev) {
		ISP_LOGW("pdev->close \n");
		ops_status = ((hw_device_t *) pdev)->close((hw_device_t *) pdev);
		/* ISP can't call the function of OEM */
		//mtrace_print_alllog(TRACE_MEMSTAT);
	}
#endif
	return ops_status;

}

#pragma weak start_camera =_start_camera
#pragma weak stop_camera  =_stop_camera

cmr_s32 write_otp_pownon()
{
	cmr_s32 ret = 0;
	if (!p_camera_device) {
		/* ISP can't call the function of OEM */
		//ret = start_camera(&p_camera_device);
	}

	return ret;
}

cmr_s32 write_otp_pownoff()
{
	cmr_s32 ret = 0;
	if (p_camera_device) {
		/* ISP can't call the function of OEM */
		//ret = stop_camera(p_camera_device);
		p_camera_device = NULL;
	}

	return ret;
}

cmr_s32 isp_otp_needstopprev(cmr_u8 * data_buf, cmr_u32 * data_size)
{
	UNUSED(data_size);
	cmr_s32 ret = 0;
	cmr_s32 otp_type = 0;

	if (data_buf == NULL) {
		SCI_Trace_Dcam("%s return error \n", __func__);
		return ret;
	}
	//the real data need to obtain from otp tool settings
	//according to protocol bit[40]~bit[87] is otp 48 bytes data
	//parse the otp data maybe writre a function to do it
	SCI_Trace_Dcam("%s \n", __func__);
	//get otp type
	memcpy_ex(4, &otp_type, &data_buf[4], 4);
	SCI_Trace_Dcam("%s otp_type %d, %s\n", __func__, otp_type, cmdstring[otp_type]);

	switch (otp_type) {
	case OTP_CALIBRATION_CMD:
		ret = 1;
		break;

	default:
		SCI_Trace_Dcam("%s:other \n", __func__);
		break;
	}

	SCI_Trace_Dcam("%s: ret=%d ", __func__, ret);
	return ret;
}

cmr_s32 isp_otp_write(cmr_handle handler, cmr_u8 * data_buf, cmr_u32 * data_size)	//DATA
{
	cmr_s32 ret = 0;
	cmr_s32 otp_type = 0;

	if (data_buf == NULL) {
		SCI_Trace_Dcam("%s return error \n", __func__);
		return ret;
	}
	//the real data need to obtain from otp tool settings
	//according to protocol bit[40]~bit[87] is otp 48 bytes data
	//parse the otp data maybe writre a function to do it
	SCI_Trace_Dcam("%s \n", __func__);
	//get otp type
	memcpy_ex(4, &otp_type, &data_buf[4], 4);
	SCI_Trace_Dcam("%s otp_type %d, %s\n", __func__, otp_type, cmdstring[otp_type]);

	if (((OTP_CAMERA_POWERON_CMD != otp_type) && (OTP_CAMERA_POWEROFF_CMD != otp_type)
	     && (OTP_RELOAD_ON_CMD != otp_type) && (OTP_RELOAD_OFF_CMD != otp_type)
	     && (OTP_WRITE_SN_CMD != otp_type))
	    && (!p_camera_device)) {
		SCI_Trace_Dcam("%s camera off now", __func__);
		*data_size = 4;
		return -1;
	}

	switch (otp_type) {
	case OTP_CALIBRATION_CMD:
		ret = write_otp_calibration_data(*data_size, data_buf);
		//write_otp_pownoff();
		//write_otp_pownon();
		break;
	case OTP_CTRL_PARAM_CMD:
		ret = write_otp_ctrl_param(handler, *data_size, data_buf);
		break;
	case OTP_ACTUATOR_I2C_CMD:
		ret = write_otp_actuator_i2c(*data_size, data_buf);
		break;
	case OTP_SENSOR_I2C_CMD:
		ret = write_otp_sensor_i2c(*data_size, data_buf);
		break;
	case OTP_ROM_DATA_CMD:
		ret = write_otp_rom_data(*data_size, data_buf);
		break;
	case OTP_START_3A_CMD:
		ret = write_otp_start_3A(handler, *data_size, data_buf);
		break;
	case OTP_STOP_3A_CMD:
		ret = write_otp_stop_3A(handler, *data_size, data_buf);
		break;
	case OTP_WRITE_SN_CMD:
		ret = write_otp_sn(*data_size, data_buf);
		break;

	case OTP_RELOAD_ON_CMD:
		/* ISP can't call the function of OEM */
		//camera_set_reload_support(1);
		break;
	case OTP_RELOAD_OFF_CMD:
		/* ISP can't call the function of OEM */
		//camera_set_reload_support(0);
		break;
	case OTP_CAMERA_POWERON_CMD:
		ret = write_otp_pownon();
		break;
	case OTP_CAMERA_POWEROFF_CMD:
		ret = write_otp_pownoff();
		break;
	case OTP_START_AF_CMD:
		ret = write_start_af(handler);
		break;
	default:
		SCI_Trace_Dcam("%s:error \n", __func__);
		break;
	}

	*data_size = 4;
	SCI_Trace_Dcam("data_size= %d \r\n", *data_size);

/*	while(index < data_size)  {
	      SCI_Trace_Dcam ("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}
*/
	SCI_Trace_Dcam("%s: ret=%d ", __func__, ret);
	return ret;
}

cmr_s32 read_otp_calibration_data(cmr_u32 * data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_u32 read_cnt = 0;
	cmr_u32 addr = 0;
	cmr_s32 addr_cnt = 0;
	cmr_u32 otp_start_addr = 0;
	cmr_u32 otp_data_len = 0;
	SENSOR_VAL_T val;
	//SENSOR_OTP_PARAM_T param_ptr;
	struct _sensor_otp_param_tag param_ptr;

	memcpy_ex(4, &addr_cnt, &data_buf[otp_data_start_index], 4);
	memcpy_ex(4, &addr, &data_buf[otp_data_start_index + 4], 4);
	memcpy_ex(4, &read_cnt, &data_buf[otp_data_start_index + 8], 4);
	SCI_Trace_Dcam("origi %s:addr 0x%x cnt %d addr_cnt %d", __func__, addr, read_cnt, addr_cnt);
	//read to calibration data and write to data_buf[write_value_index]

	//otp_start_addr = (addr == 0) ? (otp_data_offset) : (addr);
	//otp_data_len   = (read_cnt == 0) ? (otp_data_test_length) : (read_cnt);
	otp_start_addr = addr;
	//otp_data_len = 8192;

	SCI_Trace_Dcam("final %s:addr 0x%x cnt %d", __func__, otp_start_addr, otp_data_len);

	param_ptr.start_addr = otp_start_addr;
	param_ptr.len = otp_data_len;
	param_ptr.buff = &data_buf[otp_data_start_index];
	val.type = SENSOR_VAL_TYPE_READ_OTP;
	val.pval = &param_ptr;
	ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&val);
	SCI_Trace_Dcam("end %s:otp_data_len %d", __func__, otp_data_len);

	*data_size = otp_data_start_index + param_ptr.len;

	return ret;
}

cmr_s32 read_sensor_shutter(cmr_u32 * shutter_val)
{
	cmr_s32 ret = 0;
	cmr_u32 line_time = 0x00;
	cmr_u32 expsure_line = 0x00;
	//      *shutter_val = _hi544_get_shutter();
	SENSOR_VAL_T val = { SENSOR_VAL_TYPE_SHUTTER, &expsure_line };
	cmr_u32 mode = 0;
	SENSOR_EXP_INFO_T *exp_info;

	ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&val);
	//get mode & line time
	/* ISP can't call the function of OEM */
	//Sensor_GetMode(&mode);
	//exp_info       = Sensor_GetInfo();
	exp_info = NULL;
	line_time = exp_info->sensor_mode_info[mode].line_time;
	*shutter_val = expsure_line * line_time / 10;

	SCI_Trace_Dcam("%s:0x%x, expsure_line=0x%x, ret %d\n", __func__, *shutter_val, expsure_line, ret);
	return ret;
}

cmr_s32 read_sensor_line(cmr_u32 * line_val)
{
	UNUSED(line_val);
	cmr_s32 ret = 0;
	//*line_val = _hi544_get_shutter();
	//SCI_Trace_Dcam("%s:0x%x\n",__func__, *line_val);
	return ret;
}

cmr_s32 read_sensor_gain(cmr_u32 * gain_val)
{
	cmr_s32 ret = 0;
	SENSOR_VAL_T val;
	cmr_u32 gain = 0;

	val.type = SENSOR_VAL_TYPE_READ_OTP_GAIN;
	val.pval = &gain;

	ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&val);
	*gain_val = gain & 0xffff;

	SCI_Trace_Dcam("%s:gain %d ret %d\n", __func__, *gain_val, ret);

	return ret;
}

cmr_s32 read_i2c_clock(cmr_u32 * clk)
{
	UNUSED(clk);
	cmr_s32 ret = 0;
//      SCI_Trace_Dcam("%s:0x%x\n",__func__, *clk);

	return ret;
}

cmr_s32 read_mclk(cmr_u32 * mclk)
{
	UNUSED(mclk);
	cmr_s32 ret = 0;
//      SCI_Trace_Dcam("%s:0x%x\n",__func__, *mclk);

	return ret;
}

cmr_s32 read_image_pattern(cmr_u32 * pattern)
{
	UNUSED(pattern);
	cmr_s32 ret = 0;
//      SCI_Trace_Dcam("%s:0x%x\n",__func__, *pattern);

	return ret;
}

cmr_s32 read_sensor_flip(cmr_u32 * flip)
{
	UNUSED(flip);
	cmr_s32 ret = 0;
//      *flip = _hi544_get_flip();
//      SCI_Trace_Dcam("%s:0x%x\n",__func__, *flip);
	return ret;
}

cmr_s32 read_position(cmr_handle handler, cmr_u32 * pos)
{
	cmr_s32 ret = 0;
	SENSOR_VAL_T val;
	cmr_u32 cmd_param = 0;

	SCI_Trace_Dcam("%s:0x%x\n", __func__, *pos);

	val.type = SENSOR_VAL_TYPE_GET_AFPOSITION;
	val.pval = pos;
	//ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&val);
	ret = isp_ioctl(handler, ISP_CTRL_GET_AF_POS, (void *)&cmd_param);
	*pos = cmd_param;

	return ret;
}

cmr_s32 read_ev(cmr_handle handler, cmr_u32 * ev)
{
	cmr_s32 ret = 0;
	cmr_u32 cmd_param = 0;

	if (!Sensor_isRAW())
		return 0;

	ret = isp_ioctl(handler, ISP_CTRL_GET_LUM, (void *)&cmd_param);
	*ev = cmd_param;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, *ev);

	return ret;
}

cmr_s32 read_ae(cmr_handle handler, cmr_u32 * ae)
{
	cmr_s32 ret = 0;
	cmr_u32 cmd_param = 0;

	if (!Sensor_isRAW())
		return 0;

	ret = isp_ioctl(handler, ISP_CTRL_GET_AE_STATE, (void *)&cmd_param);
	*ae = cmd_param;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, *ae);

	return ret;

}

cmr_s32 read_frame_rate(cmr_handle handler, cmr_u32 * rate)
{
	UNUSED(handler);
	cmr_s32 ret = 0;

	if (!Sensor_isRAW())
		return 0;
	//isp_capability(handler, ISP_CUR_FPS, (void*)rate);
	*rate = 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, *rate);
	return ret;
}

cmr_s32 read_af_mode(cmr_handle handler, cmr_u32 * af_mode)
{
	cmr_s32 ret = 0;
	cmr_u32 cmd_param = 0;

	if (!Sensor_isRAW())
		return 0;

	ret = isp_ioctl(handler, ISP_CTRL_GET_AF_MODE, (void *)&cmd_param);
	if (ISP_FOCUS_TRIG == cmd_param) {
		*af_mode = 0;
	} else if (ISP_FOCUS_MANUAL == cmd_param) {
		*af_mode = 1;
	} else if (ISP_FOCUS_CONTINUE == cmd_param) {
		*af_mode = 2;
	}
	SCI_Trace_Dcam("%s: af_mode %d, cmd_param %d\n", __func__, *af_mode, cmd_param);

	return ret;

}

cmr_s32 read_otp_awb_gain(cmr_handle handler, void *awbc_cfg)
{
	cmr_s32 ret = 0;

	if (!Sensor_isRAW())
		return 0;

	ret = isp_ioctl(handler, ISP_CTRL_GET_AWB_GAIN, (void *)awbc_cfg);

	return ret;

}

cmr_s32 read_otpdata_checksum(cmr_u32 * checksum)
{
	cmr_s32 ret = 0;

	*checksum = 0;
	SCI_Trace_Dcam("%s:0x%x\n", __func__, *checksum);
	return ret;
}

cmr_s32 read_otp_ctrl_param(cmr_handle handler, cmr_u32 * data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 index = 0;
	cmr_u32 length = 0;
	struct otp_setting read_otp_settings;

	memset(&read_otp_settings, 0, sizeof(struct otp_setting));

	read_otp_settings.shutter = -1;
	ret = read_sensor_shutter(&read_otp_settings.shutter);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.line = read_otp_settings.shutter;
	ret = read_sensor_line(&read_otp_settings.line);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.gain = 0;
	ret = read_sensor_gain(&read_otp_settings.gain);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.i2c_clock = -1;
	ret = read_i2c_clock(&read_otp_settings.i2c_clock);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.main_clock = -1;
	ret = read_mclk(&read_otp_settings.main_clock);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.image_pattern = -1;
	ret = read_image_pattern(&read_otp_settings.image_pattern);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.flip = -1;
	ret = read_sensor_flip(&read_otp_settings.flip);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.set_af_position = -1;
	ret = read_position(handler, &read_otp_settings.set_af_position);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}

	read_otp_settings.frame_rate = -1;
	ret = read_frame_rate(handler, &read_otp_settings.frame_rate);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.set_ev = -1;
	ret = read_ev(handler, &read_otp_settings.set_ev);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.set_ae = -1;
	ret = read_ae(handler, &read_otp_settings.set_ae);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	read_otp_settings.set_af_mode = -1;
	ret = read_af_mode(handler, &read_otp_settings.set_af_mode);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}

	read_otp_settings.checksum = -1;
	ret = read_otpdata_checksum(&read_otp_settings.checksum);
	if (ret) {
		SCI_Trace_Dcam("%s:err %d\n", __func__, ret);
	}
	//the real data need to obtain from otp or sensor
	// and need to check data range
//      read_otp_settings.shutter             = 6;
//      read_otp_settings.line                  = -1;
//      read_otp_settings.gain                 = 902;
//      read_otp_settings.frame_rate        = 1000;
	read_otp_settings.i2c_clock = 400;
	read_otp_settings.main_clock = 24;
	read_otp_settings.image_pattern = 1;
	read_otp_settings.flip = 1;
	//      read_otp_settings.set_ev              = 12;
	//      read_otp_settings.set_ae              = 1;
	//      read_otp_settings.set_af_mode     = 2;
	//      read_otp_settings.set_af_position  = 20;
	/*otp data lenth 48 bytes .
	 *it needs to double check when change in the future
	 */
	otp_settings_buf[index] = OTP_DATA_LENG;

	//otp data type
	index++;
	otp_settings_buf[index] = OTP_CTRL_PARAM_CMD;

	//otp reserve 32 bytes
	index++;
	otp_settings_buf[index] = 0x0;

	index++;
	otp_settings_buf[index] = 0x0;

	index++;
	otp_settings_buf[index] = 0x0;

	index++;
	otp_settings_buf[index] = 0x0;

	index++;
	otp_settings_buf[index] = 0x0;

	index++;
	otp_settings_buf[index] = 0x0;

	index++;
	otp_settings_buf[index] = 0x0;

	index++;
	otp_settings_buf[index] = 0x0;

	//otp data
	//shutter
	index++;
	otp_settings_buf[index] = read_otp_settings.shutter;

	//line
	index++;
	otp_settings_buf[index] = read_otp_settings.line;

	//gain
	index++;
	otp_settings_buf[index] = read_otp_settings.gain;

	//frame_rate
	index++;
	otp_settings_buf[index] = read_otp_settings.frame_rate;

	//i2c clock
	index++;
	otp_settings_buf[index] = read_otp_settings.i2c_clock;

	//main_clock
	index++;
	otp_settings_buf[index] = read_otp_settings.main_clock;

	//image_pattern
	index++;
	otp_settings_buf[index] = read_otp_settings.image_pattern;

	//flip
	index++;
	otp_settings_buf[index] = read_otp_settings.flip;

	//set ev
	index++;
	otp_settings_buf[index] = read_otp_settings.set_ev;

	//set ae
	index++;
	otp_settings_buf[index] = read_otp_settings.set_ae;

	//set af mode
	index++;
	otp_settings_buf[index] = read_otp_settings.set_af_mode;

	//set af position
	index++;
	otp_settings_buf[index] = read_otp_settings.set_af_position;

	//checksum
	index++;
	otp_settings_buf[index] = read_otp_settings.checksum;

	//data lenth bytes
	length = (++index) * sizeof(cmr_u32);
	memcpy_ex(1, data_buf, (cmr_u8 *) otp_settings_buf, length);

	*data_size = length;

	return ret;
}

cmr_s32 read_otp_actuator_i2c(cmr_u32 * data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 read_cnt = 0;
	cmr_s32 addr = 0;
	cmr_s32 i = 0;
	cmr_u8 *read_ptr;
	cmr_u16 reg_val = 0;
	SENSOR_VAL_T io_val;
	cmr_u32 param;

	memcpy_ex(4, &addr, &data_buf[otp_data_start_index], 4);
	memcpy_ex(4, &read_cnt, &data_buf[otp_data_start_index + 4], 4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, addr, read_cnt);

	//read to calibration data and write to data_buf[write_value_index]
	read_ptr = &data_buf[otp_data_start_index + 4];
	io_val.type = SENSOR_VAL_TYPE_READ_VCM;
	io_val.pval = &param;

	for (i = 0; i < read_cnt; i++) {
		//      ret = dcam_i2c_read_reg_16((cmr_u16)addr,&reg_val);

		param = addr << 16;
		ret = Sensor_Ioctl(SENSOR_IOCTL_ACCESS_VAL, (void *)&io_val);
		if (0 != ret) {
			SCI_Trace_Dcam("%s failed  addr=0x%x ", __func__, addr);
			break;
		}
		reg_val = param & 0xffff;

		*read_ptr++ = reg_val & 0xff;
		*read_ptr++ = (reg_val >> 8);
		*read_ptr++ = 0;
		*read_ptr++ = 0;
		addr++;
	}
	SCI_Trace_Dcam("%s %d %d %d\n", __func__, read_cnt, (4 + 4 * read_cnt), reg_val);
	*data_size = 4 + 4 * read_cnt + 40;

	return ret;
}

cmr_s32 read_otp_sensor_i2c(cmr_u32 * data_size, cmr_u8 * data_buf)
{
	cmr_s32 ret = 0;
	cmr_s32 read_cnt = 0;
	cmr_s32 addr = 0;
	cmr_s32 i = 0;
	cmr_u8 *read_ptr;
	cmr_u16 reg_val = 0;

	memcpy_ex(4, &addr, &data_buf[otp_data_start_index], 4);
	memcpy_ex(4, &read_cnt, &data_buf[otp_data_start_index + 4], 4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, addr, read_cnt);

	//read to calibration data and write to data_buf[write_value_index]
	read_ptr = &data_buf[otp_data_start_index + 4];
	for (i = 0; i < read_cnt; i++) {
		//      ret = dcam_i2c_read_reg_16((cmr_u16)addr,&reg_val);
		/* ISP can't call the function of OEM */
		//reg_val = Sensor_ReadReg(addr);
		*read_ptr++ = reg_val & 0xff;
		*read_ptr++ = (reg_val >> 8);
		*read_ptr++ = 0;
		*read_ptr++ = 0;
		addr++;
	}
	SCI_Trace_Dcam("%s %d %d %d\n", __func__, read_cnt, (4 + 4 * read_cnt), reg_val);

	*data_size = 4 + 4 * read_cnt + 40;
	return ret;
}

cmr_s32 read_otp_rom_data(cmr_u32 * data_size, cmr_u8 * data_buf)
{
	UNUSED(data_size);
	cmr_s32 ret = 0;
	cmr_s32 otp_data_len = 0;
	cmr_s32 read_cnt = 0;
	cmr_s32 addr = 0;

	memcpy_ex(4, &addr, &data_buf[otp_data_start_index], 4);
	memcpy_ex(4, &read_cnt, &data_buf[otp_data_start_index + 4], 4);
	SCI_Trace_Dcam("%s:addr 0x%x cnt %d", __func__, addr, read_cnt);
	//read to calibration data and write to data_buf[write_value_index]

	return ret;
}

cmr_s32 isp_otp_read(cmr_handle handler, cmr_u8 * data_buf, cmr_u32 * data_size)	//DATA
{
	cmr_s32 ret = 0;
	cmr_u32 index = 0;
	cmr_u8 *log_ptr = data_buf;
	cmr_s32 otp_type = 0;

	if (data_buf == NULL) {
		SCI_Trace_Dcam("%s return error \n", __func__);
		return ret;
	}
	SCI_Trace_Dcam("%s \n", __func__);

	while (index < *data_size) {
		SCI_Trace_Dcam("%s data_buf[%.3d]= %.2x\n", __func__, index++, *log_ptr++);
	}

	//the real data need to obtain from otp tool settings
	//according to protocol bit[40]~bit[87] is otp 48 bytes data
	//parse the otp data maybe writre a function to do it
	//get otp type
	memcpy_ex(4, &otp_type, &data_buf[4], 4);
	SCI_Trace_Dcam("%s otp_type %d, %s\n", __func__, otp_type, cmdstring[otp_type]);

	if (((OTP_CALIBRATION_CMD == otp_type) || (OTP_CTRL_PARAM_CMD == otp_type)
	     || (OTP_ACTUATOR_I2C_CMD == otp_type) || (OTP_SENSOR_I2C_CMD == otp_type)
	     || (OTP_ROM_DATA_CMD == otp_type))
	    && (!p_camera_device)) {
		SCI_Trace_Dcam("%s camera off now", __func__);
		*data_size = 4;
		return -1;
	}

	switch (otp_type) {
	case OTP_CALIBRATION_CMD:
		ret = read_otp_calibration_data(data_size, data_buf);
		//write_otp_pownoff();
		//write_otp_pownon();
		break;
	case OTP_CTRL_PARAM_CMD:
		ret = read_otp_ctrl_param(handler, data_size, data_buf);
		break;
	case OTP_ACTUATOR_I2C_CMD:
		ret = read_otp_actuator_i2c(data_size, data_buf);
		break;
	case OTP_SENSOR_I2C_CMD:
		ret = read_otp_sensor_i2c(data_size, data_buf);
		break;
	case OTP_ROM_DATA_CMD:
		ret = read_otp_rom_data(data_size, data_buf);
		break;
	default:
		SCI_Trace_Dcam("%s:error\n", __func__);
		break;
	}

	SCI_Trace_Dcam("data_size= %d \r\n", *data_size);

	index = 0;
	log_ptr = data_buf;
	while (index < *data_size) {
		SCI_Trace_Dcam("%s data_buf[%d]= %02x\n", __func__, index++, *log_ptr++);
	}

	SCI_Trace_Dcam("%s: ret=%d ", __func__, ret);
	return ret;
}

#endif
