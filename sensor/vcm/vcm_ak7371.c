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
#include <utils/Log.h>
#include "vcm_ak7371.h"
#include "sensor_drv_u.h"

#define AK7371_VCM_SLAVE_ADDR (0x18 >> 1)
#define SENSOR_SUCCESS      0
#define POSE_UP_HORIZONTAL 32
#define POSE_DOWN_HORIZONTAL 37
static uint32_t m_cur_pos=0;

uint32_t vcm_ak7371_init(SENSOR_HW_HANDLE handle,int mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = AK7371_VCM_SLAVE_ADDR;
	SENSOR_LOGI("E");
	usleep(100);
	switch (mode) {
	case 1:
		break;

	case 2:
	{
		cmd_len = 2;

		cmd_val[0] = 0x02;
		cmd_val[1] = 0x00;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_LOGI("SENSOR_vcm: _ak7371_SRCInit 1 fail!");
		}

		usleep(200);
	
	}
		break;
	default:
		break;
	}
	return ret_value;
}

uint32_t vcm_ak7371_set_position(SENSOR_HW_HANDLE handle, uint32_t pos)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t time_out = 0;

	if ((int32_t)pos < 0)
		pos = 0;
	else if ((int32_t)pos > 0x3FF)
		pos = 0x3FF;
	m_cur_pos=pos&0x3FF;

	//SENSOR_LOGI("set position %d", pos);
	slave_addr = AK7371_VCM_SLAVE_ADDR;
	cmd_len = 2;
	cmd_val[0] = 0x00;
	cmd_val[1] = (pos&0x3ff)>>2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	cmd_val[0] = 0x01;
	cmd_val[1] = (pos&0x03)<<6;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	//cmd_len=sensor_grc_read_i2c(ak7371_VCM_SLAVE_ADDR,  0x03, BITS_ADDR8_REG8);
	//time_out=sensor_grc_read_i2c(ak7371_VCM_SLAVE_ADDR,  0x04, BITS_ADDR8_REG8);
	//SENSOR_LOGI("set position %d %d", pos,((cmd_len&0x03)<<8)|time_out);

	return ret_value;
}

uint32_t vcm_ak7371_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h, uint32_t *h2down)
{
	*up2h = POSE_UP_HORIZONTAL;
	*h2down = POSE_DOWN_HORIZONTAL;

	return 0;
}

uint32_t vcm_ak7371_deinit(SENSOR_HW_HANDLE handle)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t pos = m_cur_pos;
	uint32_t vcm_last_pos = pos & 0xffff;
	uint32_t vcm_last_delay = pos & 0xffff0000;


	while(vcm_last_pos > 0) {
		vcm_last_pos = vcm_last_pos >> 2;
		SENSOR_LOGI("vcm_last_pos=%d  ,%d",vcm_last_pos,(vcm_last_pos|vcm_last_delay));
		vcm_ak7371_set_position(handle,vcm_last_pos|vcm_last_delay);
		usleep(1*1000);
	}

	SENSOR_LOGI("vcm_ak7371_deinit");
	return ret_value;
}


