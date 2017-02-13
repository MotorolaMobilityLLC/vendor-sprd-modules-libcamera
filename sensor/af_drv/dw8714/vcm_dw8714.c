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
#include "vcm_dw8714.h"

uint32_t vcm_dw8714_set_position(SENSOR_HW_HANDLE handle, uint32_t pos)
{
	uint32_t ret_value = AF_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	//for direct mode
	slave_addr = DW8714_VCM_SLAVE_ADDR;
	cmd_val[0] = (pos&0xfff0)>>4;
	cmd_val[1] = (pos&0x0f)<<4;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	return ret_value;
}

af_drv_info_t dw8714_drv_info = {
		.af_work_mode = 0,
		.af_ops =
		{
			.set_motor_pos       = vcm_dw8714_set_position,
			.get_motor_pos       = NULL,
			.set_motor_bestmode  = NULL,
			.motor_deinit        = NULL,
			.set_test_motor_mode = NULL,
			.get_test_motor_mode = NULL,
		},
};
