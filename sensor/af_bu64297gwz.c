/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
ver: 1.0
*/

#include <utils/Log.h>
#include "sensor.h"
#include "af_bu64297gwz.h"

#define bu64297gwz_VCM_SLAVE_ADDR (0x18 >> 1)
#define MOVE_CODE_STEP_MAX 40
#define WAIT_STABLE_TIME  20    //ms


/*==============================================================================
 * Description:
 * init vcm driver
 * you can change this function acording your spec if it's necessary
 *============================================================================*/
uint32_t bu64297gwz_init(void)
{
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = bu64297gwz_VCM_SLAVE_ADDR;
	cmd_val[0] = 0xcc;
	cmd_val[1] = 0x83;  //0x4b;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	cmd_val[0] = 0xd4;
	cmd_val[1] = 0x3c;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
	cmd_val[0] = 0xdc;
	cmd_val[1] = 0x8c;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	cmd_val[0] = 0xe4;
	cmd_val[1] = 0x84;//0x21;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
	return ret_value;
}
/*==============================================================================
 * Description:
 * write code to vcm driver
 * you can change this function acording your spec if it's necessary
 * code: Dac code for vcm driver
 *============================================================================*/
uint32_t bu64297gwz_write_dac_code(int32_t code)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = bu64297gwz_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("%d", code);

	cmd_val[0] = ((code & 0x0300) >> 8)|0xc0;
	cmd_val[1] = code & 0xff ;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	return ret_value;
}
/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 *
 * Param: ISP write dac code
 *============================================================================*/
uint32_t bu64297gwz_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	int32_t target_code=param&0x3FF;

	SENSOR_PRINT("%d", target_code);
	bu64297gwz_write_dac_code(target_code);

	return ret_value;
}

/*==============================================================================
 * Description:
 * deinit vcm driver DRV201  PowerOff DRV201
 * you can change this function acording your Module spec if it's necessary
 * mode:
 * 1: PWM Mode
 * 2: Linear Mode
 *============================================================================*/
uint32_t bu64297gwz_deinit(uint32_t mode)
{
	bu64297gwz_write_af(0);

	return 0;
}

