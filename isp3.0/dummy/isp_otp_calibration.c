#include <sys/types.h>

int32_t isp_cur_bv;
uint32_t isp_cur_ct;

int32_t isp_calibration_get_info(void *golden_info, void *cali_info)
{
	return 0;
}

uint32_t isp_raw_para_update_from_file(void *sensor_info_ptr, int sensor_id)
{
	return 0;
}

int read_position(void *handler, uint32_t * pos)
{
	return 0;
}

int read_otp_awb_gain(void *handler, void *awbc_cfg)
{
	return 0;
}

int read_sensor_shutter(uint32_t * shutter_val)
{
	return 0;
}

int read_sensor_gain(uint32_t * gain_val)
{
	return 0;
}

int32_t isp_calibration(void *param, void *result)
{
	return 0;
}
