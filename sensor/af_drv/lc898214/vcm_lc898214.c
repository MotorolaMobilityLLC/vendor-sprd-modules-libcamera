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
#include "vcm_lc898214.h"
#include "sensor_drv_u.h"

#define LC898214_VCM_SLAVE_ADDR (0xE4 >> 1)
#define SENSOR_SUCCESS      0
#define POSE_UP_HORIZONTAL 32
#define POSE_DOWN_HORIZONTAL 37
static uint32_t m_cur_pos=0;

uint32_t vcm_LC898214_init(SENSOR_HW_HANDLE handle)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t SlaveID = 0xE4;
	uint16_t EepromSlaveID = 0xE6;

	uint16_t SlvCh = 0xff;
	uint16_t EepCh = 0xff;

	slave_addr = LC898214_VCM_SLAVE_ADDR;
	SENSOR_LOGI("E");
	for(int i=0;i<30;i++)
	{
		//Sensor_ReadI2C(slave_addr, 0xF0, &SlvCh, 0);//
		SlvCh = sensor_grc_read_i2c(slave_addr,  0xF0, BITS_ADDR8_REG8);
		usleep(50);
		if (SlvCh == 0x42)
		{
			break;
		}
	}
	usleep(100);
	cmd_len = 2;

	cmd_val[0] = 0x87;
	cmd_val[1] = 0x00;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	//I2C_Write(SlaveID, 0x87, 0x00, 0);
		usleep(20);
		uint16_t Max_H = 0xff, Max_L = 0xff, Min_H = 0xff, Min_L = 0xff;
		Max_H = sensor_grc_read_i2c(EepromSlaveID,  0x38, BITS_ADDR8_REG8);
		//I2C_Read(EepromSlaveID, 0x38, &Max_H, 0); //读取Eeprom内部HALL MAX、MIN数据数据
		usleep(50);
		Max_L = sensor_grc_read_i2c(EepromSlaveID,  0x39, BITS_ADDR8_REG8);
		//I2C_Read(EepromSlaveID, 0x39, &Max_L, 0); //读取Eeprom内部HALL MAX、MIN数据数据
		usleep(50);		
		Min_H = sensor_grc_read_i2c(EepromSlaveID,  0x3A, BITS_ADDR8_REG8);
		//I2C_Read(EepromSlaveID, 0x3A, &Min_H, 0); //读取Eeprom内部HALL MAX、MIN数据数据
		usleep(50);		
		Min_L = sensor_grc_read_i2c(EepromSlaveID,  0x3B, BITS_ADDR8_REG8);
//		I2C_Read(EepromSlaveID, 0x3B, &Min_L, 0); //读取Eeprom内部HALL MAX、MIN数据数据
		usleep(50);
		uint16_t Max = (uint16_t)((Max_H << 8) | Max_L);
		uint16_t Min = (uint16_t)((Min_H << 8) | Min_L);
		Hall_Max = (int16_t)Max;
		Hall_Min = (int16_t)Min; 
		//SENSOR_LOGI("DriverIC Init Max_H  %d %d %d %d! %d %d",Max_H ,Max_L ,Min_H ,Min_L,Max, Min);
		
		//I2C_Write(SlaveID, 0xE0, 0x01, 0);
		cmd_val[0] = 0xE0;
		cmd_val[1] = 0x01;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		for(int i=0;i<30;i++)
		{
			EepCh = sensor_grc_read_i2c(slave_addr,  0xE0, BITS_ADDR8_REG8);
//			I2C_Read(SlaveID, 0xE0, &EepCh, 0);
			usleep(50);
			if (EepCh == 0x00)
			{
				break;
			}
		}
		if ((SlvCh != 0x42) || (EepCh != 0x00))
		{
			SENSOR_LOGI("DriverIC Init Err!");
			return -1;
		}
		cmd_val[0] = 0xA0;
		cmd_val[1] = Hall_Min;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		//I2C_Write(SlaveID, 0xA0, Hall_Min, 2);
		HallValue = Hall_Min;
		rCode = 0;
#if 0
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
			SENSOR_LOGI("SENSOR_vcm: _LC898214_SRCInit 1 fail!");
		}

		usleep(200);
	
	}
		break;
	default:
		break;
	}
#endif	
	return ret_value;
}

uint32_t vcm_LC898214_set_position(SENSOR_HW_HANDLE handle, uint32_t pos)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[3] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t time_out = 0;

	if ((int32_t)pos < 0)
		pos = 0;
	else if ((int32_t)pos > 0x3FF)
		pos = 0x3FF;
	m_cur_pos=pos&0x3FF;
	SENSOR_LOGI("set position %d", pos);
	rCode = pos;
	uint16_t	Min_Pos = 0, Max_Pos = 1023;
	uint16_t ivalue2;
	uint16_t temp;
	double k = 1.0*(Hall_Max - Hall_Min) / (Max_Pos - Min_Pos);
	temp = Hall_Min;
	int iK = (int)k*100;
	double k2 = iK / 100.0;

	uint16_t a = (uint16_t)((pos * k2) + temp);
	ivalue2 = a;
	HallValue = ivalue2 & 0xfff0;

	if ((short)ivalue2 < (short)Hall_Min)
	{
		ivalue2 = Hall_Min;
	}
	if ((short)ivalue2 > (short)Hall_Max)
	{
		ivalue2 = Hall_Max;
	}
	//BOOL bRet = I2C_Write(SlaveID, 0xA0 ,ivalue2, 2);
	cmd_len = 3;
	cmd_val[0] = 0xA0;
	cmd_val[1] = pos;//ivalue2;//TODO
	ret_value = Sensor_WriteI2C(LC898214_VCM_SLAVE_ADDR,(uint8_t*)&cmd_val[0], cmd_len);
	//SENSOR_LOGI("set position %d SlvCh %d %d", pos,ivalue2,ret_value);

	uint16_t SlvCh;
	for(int i=0;i<30;i++)
	{
		SlvCh=sensor_grc_read_i2c(LC898214_VCM_SLAVE_ADDR,  0x8F, BITS_ADDR8_REG8);
		//I2C_Read(SlaveID, 0x8F, &SlvCh, 0);
		ret_value = sensor_grc_read_i2c(LC898214_VCM_SLAVE_ADDR,0xA0, BITS_ADDR8_REG16);
		usleep(50);
	//	SENSOR_LOGI("set position %d SlvCh1 %d %d", pos,SlvCh,ret_value);
		if (SlvCh == 0x00)
		{
			break;
		}
	}
#if 0
	SENSOR_LOGI("set position %d", pos);
	slave_addr = LC898214_VCM_SLAVE_ADDR;
	cmd_len = 2;
	cmd_val[0] = 0x00;
	cmd_val[1] = (pos&0x3ff)>>2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	cmd_val[0] = 0x01;
	cmd_val[1] = (pos&0x03)<<6;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	//cmd_len=sensor_grc_read_i2c(LC898214_VCM_SLAVE_ADDR,  0x03, BITS_ADDR8_REG8);
	//time_out=sensor_grc_read_i2c(LC898214_VCM_SLAVE_ADDR,  0x04, BITS_ADDR8_REG8);
	//SENSOR_LOGI("set position %d %d", pos,((cmd_len&0x03)<<8)|time_out);
#endif
	return ret_value;
}

uint32_t vcm_LC898214_get_pose_dis(SENSOR_HW_HANDLE handle, uint32_t *up2h, uint32_t *h2down)
{
	*up2h = POSE_UP_HORIZONTAL;
	*h2down = POSE_DOWN_HORIZONTAL;

	return 0;
}

uint32_t vcm_LC898214_deinit(SENSOR_HW_HANDLE handle)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t pos = m_cur_pos;
	uint32_t vcm_last_pos = pos & 0xffff;
	uint32_t vcm_last_delay = pos & 0xffff0000;


	while(vcm_last_pos > 0) {
		vcm_last_pos = vcm_last_pos >> 2;
		SENSOR_LOGI("vcm_last_pos=%d  ,%d",vcm_last_pos,(vcm_last_pos|vcm_last_delay));
		vcm_LC898214_set_position(handle,vcm_last_pos|vcm_last_delay);
		usleep(1*1000);
	}

	SENSOR_LOGI("vcm_LC898214_deinit");
	return ret_value;
}


