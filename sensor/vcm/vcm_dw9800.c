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
#include "vcm_dw9800.h"
#include "sensor_drv_u.h"

#define DW9800_VCM_SLAVE_ADDR (0x18 >> 1)
#define SENSOR_SUCCESS      0

uint32_t vcm_dw9800_init(void)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9800_VCM_SLAVE_ADDR;
	SENSOR_PRINT("E");
	cmd_len = 2;
	cmd_val[0] = 0x02;
	cmd_val[1] = 0x02;
	Sensor_WriteI2C_Ex(slave_addr,(uint8_t*)&cmd_val[0], cmd_len, SENSOR_MAIN);
	usleep(1000);
	cmd_val[0] = 0x06;
	cmd_val[1] = 0x80;
	Sensor_WriteI2C_Ex(slave_addr,(uint8_t*)&cmd_val[0], cmd_len, SENSOR_MAIN);
	usleep(1000);
	cmd_val[0] = 0x07;
	cmd_val[1] = 0x75;
	Sensor_WriteI2C_Ex(slave_addr,(uint8_t*)&cmd_val[0], cmd_len, SENSOR_MAIN);

	return ret_value;
}

uint32_t vcm_dw9800_set_position(uint32_t pos)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t time_out = 0;

	SENSOR_PRINT("set position %d", pos);
	slave_addr = DW9800_VCM_SLAVE_ADDR;
	cmd_len = 2;
	cmd_val[0] = 0x03;
	cmd_val[1] = (pos&0x300)>>8;
	ret_value = Sensor_WriteI2C_Ex(slave_addr,(uint8_t*)&cmd_val[0], cmd_len, SENSOR_MAIN);
	cmd_val[0] = 0x04;
	cmd_val[1] = (pos&0xff);
	ret_value = Sensor_WriteI2C_Ex(slave_addr,(uint8_t*)&cmd_val[0], cmd_len, SENSOR_MAIN);

	return ret_value;
}
