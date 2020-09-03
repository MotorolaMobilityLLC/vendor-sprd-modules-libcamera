#include <stdio.h>
#include <string.h>
#include "dirent.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include "jpeg_exif_header.h"
#include "sensor_raw.h"
#include "sensor_drv_u.h"
#include "parameters/sensor_default_param_raw_param_main.c"
#include "cmr_sensor_info.h"

#define LOG_TAG "default_param_process.c"

#define DEFAULT_LOG_ERR "%d, %s: default_tuning_error: "
#define DEFAULT_LOG_DEBUG "%d, %s: default_tuning_debug: "

#define DEFAULT_ARGS __LINE__, __FUNCTION__
#define DEFAULT_LOGE(format, ...)                                               \
    ALOGE(DEFAULT_LOG_ERR format, DEFAULT_ARGS,##__VA_ARGS__)
#define DEFAULT_LOGD(format, ...)                                               \
	ALOGD(DEFAULT_LOG_DEBUG format, DEFAULT_ARGS,##__VA_ARGS__)


void *default_tuning_param_get_ptr(param_input_t *default_param_input) {

	struct sensor_raw_info *default_param_mipi_raw_info_ptr = &s_default_param_mipi_raw_info ;

//***************************Modify sensor_name***************************
	cmr_u32 *get_sensor_name = default_param_input->module_cfg.sensor_basic_info.sensor_name;
	cmr_s8 *sensor_name;

	sensor_name = default_param_mipi_raw_info_ptr->version_info->sensor_ver_name.sensor_name;
	DEFAULT_LOGD("default_tuning_name %s\n", sensor_name);

	strcpy(default_param_mipi_raw_info_ptr->version_info->sensor_ver_name.sensor_name,get_sensor_name);
	sensor_name = default_param_mipi_raw_info_ptr->version_info->sensor_ver_name.sensor_name;
	DEFAULT_LOGD("return_tuning_name %s\n", sensor_name);
//***************************Modify size************************************
	cmr_u8 *temp_point;
	DEFAULT_LOGD(" w*h=0x%x,=0x%x\n",default_param_input->module_cfg.sensor_cfg.sensor_settings_info.size_info[0].size_w,\
	default_param_input->module_cfg.sensor_cfg.sensor_settings_info.size_info[0].size_h);
			/***********common**************/
			temp_point = default_param_mipi_raw_info_ptr->mode_ptr[0].addr;

			DEFAULT_LOGD("before_modify_w*h =0x%x,=0x%x,=0x%x,=0x%x\n",*(temp_point+24),*(temp_point+25),*(temp_point+28),*(temp_point+29));
			temp_point = default_param_mipi_raw_info_ptr->mode_ptr[0].addr;

			*(temp_point+24) = default_param_input->module_cfg.sensor_cfg.sensor_settings_info.size_info[0].size_w&0xff;	 //weight_low
			*(temp_point+25) = default_param_input->module_cfg.sensor_cfg.sensor_settings_info.size_info[0].size_w>>8; 		 //weight_high
			*(temp_point+28) = default_param_input->module_cfg.sensor_cfg.sensor_settings_info.size_info[0].size_h&0xff;		 //height_low
			*(temp_point+29) = default_param_input->module_cfg.sensor_cfg.sensor_settings_info.size_info[0].size_h>>8; 		 //height_high
			temp_point = default_param_mipi_raw_info_ptr->mode_ptr[0].addr;
			DEFAULT_LOGD("after_modify_w*h =0x%x,=0x%x,=0x%x,=0x%x\n",*(temp_point+24),*(temp_point+25),*(temp_point+28),*(temp_point+29));

	DEFAULT_LOGD("exit");
	return default_param_mipi_raw_info_ptr;
}
