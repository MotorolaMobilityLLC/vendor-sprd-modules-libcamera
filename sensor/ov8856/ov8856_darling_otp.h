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
 * V2.0
 */
#ifndef _SENSOR_OV8856_OTP_TRULY_H_
#define _SENSOR_OV8856_OTP_TRULY_H_

#if defined(CONFIG_CAMERA_RE_FOCUS)
	/*use dual otp info*/
	#define OV8856_DUAL_OTP
#endif

#define OTP_LSC_INFO_LEN 400  //1658
static cmr_u8 ov8856_opt_lsc_data[OTP_LSC_INFO_LEN];
//#define OTP_PDAF_INFO_LEN 500  //1658
//static cmr_u8 ov8856_opt_pdaf_data[OTP_PDAF_INFO_LEN];
static struct sensor_otp_cust_info ov8856_otp_info;
#ifdef OV8856_DUAL_OTP
#define OTP_DUAL_SIZE 8192
#define OV8856_MASTER_LSC_INFO_LEN 1400
#define OV8856_SLAVE_LSC_INFO_LEN 400
#define OV8856_DUAL_OTP_DATA3D_SIZE 1705
#define OV8856_DUAL_OTP_MASTER_SLAVE_OFFSET 4430
#define OV8856_DUAL_OTP_DATA3D_OFFSET 6271

static cmr_u8 ov8856_dual_otp_data[OTP_DUAL_SIZE];
static struct sensor_dual_otp_info ov8856_dual_otp_info;

#endif

static cmr_u8 ov8856_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr);
static int ov8856_otp_init(SENSOR_HW_HANDLE handle);
static int ov8856_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long ov8856_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long ov8856_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
#ifdef OV8856_DUAL_OTP
static unsigned long ov8856_dual_otp_parse(SENSOR_HW_HANDLE handle);
static int ov8856_dual_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long ov8856_dual_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long ov8856_parse_dual_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
#endif

#define  ov8856_i2c_read_otp(addr)    ov8856_i2c_read_otp_set(handle, addr)

static cmr_u8 ov8856_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr)
{
	return sensor_grc_read_i2c(0xA0 >> 1, addr, BITS_ADDR16_REG8);
}


static int ov8856_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static uint32_t otp_length=0;

static int ov8856_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u8 otp_version = 0;
	cmr_u16 start_addr = 0x0000;
	cmr_u32 checksum = 0;
	static cmr_u8 first_flag = 1;

	SENSOR_LOGI("E");
	//if (first_flag)
	{
		//otp_version = ov8856_i2c_read_otp(0x000E);
			
		ov8856_otp_info.program_flag = ov8856_i2c_read_otp(0x0000);
		SENSOR_LOGI("program_flag = %d", ov8856_otp_info.program_flag);
		if (0x0009 < ov8856_otp_info.program_flag) {
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			return -1;
		}
		if((ov8856_otp_info.program_flag & 0x0001) == 0x0001){
			otp_version = 1;
			start_addr = 0x001B;
		}
		if((ov8856_otp_info.program_flag & 0x0004) == 0x0004){
			otp_version = 2;
			start_addr = 0x0001;
		}
		//checksum += ov8856_otp_info.program_flag;
		ov8856_otp_info.module_info.year = ov8856_i2c_read_otp(start_addr + 0x0004);
		//checksum += ov8856_otp_info.module_info.year;
		ov8856_otp_info.module_info.month = ov8856_i2c_read_otp(start_addr + 0x0005);
		//checksum += ov8856_otp_info.module_info.month;
		ov8856_otp_info.module_info.day = ov8856_i2c_read_otp(start_addr + 0x0006);
		//checksum += ov8856_otp_info.module_info.day;
		ov8856_otp_info.module_info.mid = ov8856_i2c_read_otp(start_addr + 0x0002);
		//checksum += ov8856_otp_info.module_info.mid;
		ov8856_otp_info.module_info.lens_id = ov8856_i2c_read_otp(start_addr + 0x0008);
		//checksum += ov8856_otp_info.module_info.lens_id;
		ov8856_otp_info.module_info.vcm_id = ov8856_i2c_read_otp(start_addr + 0x0009);
		//checksum += ov8856_otp_info.module_info.vcm_id;
		ov8856_otp_info.module_info.driver_ic_id = ov8856_i2c_read_otp(start_addr + 0x000A);
		//checksum += ov8856_otp_info.module_info.driver_ic_id;

		high_val = ov8856_i2c_read_otp(start_addr + 0x0010);
		//checksum += high_val;
		low_val = ov8856_i2c_read_otp(start_addr + 0x0011);
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
		high_val = ov8856_i2c_read_otp(start_addr + 0x0012);
		//checksum += high_val;
		low_val = ov8856_i2c_read_otp(start_addr + 0x0013);
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = ov8856_i2c_read_otp(start_addr + 0x0014);
		//checksum += high_val;
		low_val = ov8856_i2c_read_otp(start_addr + 0x0015);
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = ov8856_i2c_read_otp(start_addr + 0x0016);
		//checksum += high_val;
		low_val = ov8856_i2c_read_otp(start_addr + 0x0017);
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);
		SENSOR_LOGI("iso = 0x%x ov8856_otp_info.gain_r = 0x%x %x %x", ov8856_otp_info.isp_awb_info.iso, ov8856_otp_info.isp_awb_info.gain_r,ov8856_otp_info.isp_awb_info.gain_g,ov8856_otp_info.isp_awb_info.gain_b);

		for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
			ov8856_opt_lsc_data[i] = ov8856_i2c_read_otp(start_addr + 0x001B + i);
			//checksum += ov8856_opt_lsc_data[i];
		}
		ov8856_otp_info.lsc_info.lsc_data_addr = ov8856_opt_lsc_data;
		ov8856_otp_info.lsc_info.lsc_data_size = sizeof(ov8856_opt_lsc_data);
		
		otp_length=459;
	
		high_val = ov8856_i2c_read_otp(otp_length-2);
		//checksum += low_val;
		low_val = ov8856_i2c_read_otp(otp_length-1);
		//checksum += high_val;
		ov8856_otp_info.checksum= (high_val << 8 | low_val);

		for (i = 0; i < otp_length-4; i++) {
			checksum += ov8856_i2c_read_otp(0x0000 + i);
			
		}

		SENSOR_LOGI("checksum = 0x%x ov8856_otp_info.checksum = 0x%x %x %x %x %x", checksum, ov8856_otp_info.checksum,high_val,low_val,otp_version,otp_length);
#if 0
		FILE *fd=fopen("/data/misc/media/dual-otp.dump.bin","wb+");
		for (i = 0; i < otp_length; i++) {
			low_val = ov8856_i2c_read_otp(0x0000 + i);
			fwrite((char *)&low_val,1,1,fd);
		}
		fclose(fd);
#endif

		if ((checksum&0xffff)  != ov8856_otp_info.checksum) {
			SENSOR_LOGI("checksum error!");
			ov8856_otp_info.program_flag = 0;
			return -1;
		}
		first_flag = 0;
	}
	SENSOR_LOGI("X");
	return 0;
}

static unsigned long ov8856_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;

	SENSOR_LOGI("E");
	ov8856_otp_read_data(handle);
	otp_info = &ov8856_otp_info;

	if (1 != otp_info->program_flag) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	}
	param->pval = (void *)otp_info;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long ov8856_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;
	cmr_u8 *buff = NULL;
	cmr_u16 i = 0;
	cmr_u16 j = 0;
	cmr_u16 start_addr = 0;
	cmr_u16 otp_version = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	SENSOR_LOGI("E");
	if (NULL == param->pval) {
		SENSOR_LOGI("ov8856_parse_otp is NULL data");
		return -1;
	}
	buff = param->pval;
	otp_length = 459;//ov8856_i2c_read_otp(0x000E)==1?1733:2033;

	if (0x0009 <  buff[0]) {
		SENSOR_LOGI("ov8856_parse_otp is wrong data");
		param->pval = NULL;
		return -1;
	} else {
		ov8856_otp_info.program_flag = buff[0];
		SENSOR_LOGI("program_flag = %d %x", ov8856_otp_info.program_flag,otp_length);
		if (0x0009 < ov8856_otp_info.program_flag) {
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			return -1;
		}
		if((ov8856_otp_info.program_flag & 0x0001) == 0x0001){
			otp_version = 1;
			start_addr = 0x001B;
		}
		if((ov8856_otp_info.program_flag & 0x0004) == 0x0004){
			otp_version = 2;
			start_addr = 0x0001;
		}

		//checksum += ov8856_otp_info.program_flag;
		ov8856_otp_info.module_info.year = buff[start_addr + 0x0004];
		//checksum += ov8856_otp_info.module_info.year;
		ov8856_otp_info.module_info.month = buff[start_addr + 0x0005];
		//checksum += ov8856_otp_info.module_info.month;
		ov8856_otp_info.module_info.day = buff[start_addr + 0x0006];
		//checksum += ov8856_otp_info.module_info.day;
		ov8856_otp_info.module_info.mid = buff[start_addr + 0x0002];
		//checksum += ov8856_otp_info.module_info.mid;
		ov8856_otp_info.module_info.lens_id = buff[start_addr + 0x0008];
		//checksum += ov8856_otp_info.module_info.lens_id;
		ov8856_otp_info.module_info.vcm_id = buff[start_addr + 0x0009];
		//checksum += ov8856_otp_info.module_info.vcm_id;
		ov8856_otp_info.module_info.driver_ic_id = buff[start_addr + 0x000A];
		//checksum += ov8856_otp_info.module_info.driver_ic_id;

		high_val = buff[start_addr + 0x0010];
		//checksum += high_val;
		low_val = buff[start_addr + 0x0011];
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
		high_val = buff[start_addr + 0x0012];
		//checksum += high_val;
		low_val = buff[start_addr + 0x0013];
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = buff[start_addr + 0x0014];
		//checksum += high_val;
		low_val = buff[start_addr + 0x0015];
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = buff[start_addr + 0x0016];
		//checksum += high_val;
		low_val = buff[start_addr + 0x0017];
		//checksum += low_val;
		ov8856_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

		for (j = 0; j < OTP_LSC_INFO_LEN; j++) {
			ov8856_opt_lsc_data[j] = buff[start_addr + 0x001B + j];
			//checksum += ov8856_opt_lsc_data[j];
		}
		ov8856_otp_info.lsc_info.lsc_data_addr = ov8856_opt_lsc_data;
		ov8856_otp_info.lsc_info.lsc_data_size = sizeof(ov8856_opt_lsc_data);


		//ov8856_otp_info.checksum = buff[2031]<<8|buff[2032];
		ov8856_otp_info.checksum = buff[otp_length-2]<<8|buff[otp_length-1];
		for (i = 0; i < otp_length-4; i++) {
			checksum += buff[i];
		}
		SENSOR_LOGI("checksum = 0x%x ov8856_otp_info.checksum = 0x%x", checksum, ov8856_otp_info.checksum);

		if ((checksum &0xffff) != ov8856_otp_info.checksum) {
			SENSOR_LOGI("checksum error!");
			ov8856_otp_info.program_flag = 0;
			param->pval = NULL;
			return -1;
		}
		otp_info = &ov8856_otp_info;
		param->pval = (void *)otp_info;
	}
	SENSOR_LOGI("param->pval = %p", param->pval);

	return 0;
}

#ifdef OV8856_DUAL_OTP
static unsigned long ov8856_dual_otp_parse(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;
	cmr_u8 *buffer = NULL;

	SENSOR_LOGI("E");
	ov8856_otp_info.program_flag = ov8856_dual_otp_data[0];
	SENSOR_LOGI("program_flag = %d", ov8856_otp_info.program_flag);
	if (1 != ov8856_otp_info.program_flag) {
		SENSOR_LOGI("failed to read otp or the otp is wrong data");
		return -1;
	}
	//checksum += ov8856_otp_info.program_flag;
	ov8856_otp_info.module_info.year = ov8856_dual_otp_data[3];
	//checksum += ov8856_otp_info.module_info.year;
	ov8856_otp_info.module_info.month = ov8856_dual_otp_data[4];
	//checksum += ov8856_otp_info.module_info.month;
	ov8856_otp_info.module_info.day = ov8856_dual_otp_data[5];
	//checksum += ov8856_otp_info.module_info.day;
	ov8856_otp_info.module_info.mid = ov8856_dual_otp_data[1];
	//checksum += ov8856_otp_info.module_info.mid;
	ov8856_otp_info.module_info.lens_id = ov8856_dual_otp_data[7];
	//checksum += ov8856_otp_info.module_info.lens_id;
	ov8856_otp_info.module_info.vcm_id = ov8856_dual_otp_data[8];
	//checksum += ov8856_otp_info.module_info.vcm_id;
	ov8856_otp_info.module_info.driver_ic_id = ov8856_dual_otp_data[9];
	//checksum += ov8856_otp_info.module_info.driver_ic_id;

	high_val = ov8856_dual_otp_data[51];
	//checksum += high_val;
	low_val = ov8856_dual_otp_data[52];
	//checksum += low_val;
	ov8856_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
	high_val = ov8856_dual_otp_data[53];
	//checksum += high_val;
	low_val = ov8856_dual_otp_data[54];
	//checksum += low_val;
	ov8856_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
	high_val = ov8856_dual_otp_data[55];
	//checksum += high_val;
	low_val = ov8856_dual_otp_data[56];
	//checksum += low_val;
	ov8856_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
	high_val = ov8856_dual_otp_data[57];
	//checksum += high_val;
	low_val = ov8856_dual_otp_data[58];
	//checksum += low_val;
	ov8856_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

	for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
		ov8856_opt_lsc_data[i] = ov8856_dual_otp_data[61+i];
		//checksum += ov8856_opt_lsc_data[i];
	}

	ov8856_otp_info.lsc_info.lsc_data_addr = ov8856_opt_lsc_data;
	ov8856_otp_info.lsc_info.lsc_data_size = sizeof(ov8856_opt_lsc_data);

	for (i = 0; i < OTP_PDAF_INFO_LEN; i++) {
		ov8856_opt_pdaf_data[i] = ov8856_dual_otp_data[1529+i];
		//checksum += ov8856_opt_lsc_data[j];
	}
	ov8856_otp_info.pdaf_info.pdaf_data_addr = ov8856_opt_pdaf_data;
	ov8856_otp_info.pdaf_info.pdaf_data_size = sizeof(ov8856_opt_pdaf_data);

	ov8856_otp_info.af_info.flag = ov8856_dual_otp_data[23];
	if (0 == ov8856_otp_info.af_info.flag)
		SENSOR_LOGI("af otp is wrong");

	//checksum += ov8856_otp_info.af_info.flag;

	high_val = ov8856_dual_otp_data[27];
	//checksum += low_val;
	low_val = ov8856_dual_otp_data[28];
	//checksum += high_val;
	ov8856_otp_info.af_info.infinite_cali = (high_val << 8 | low_val) >> 6;
	high_val = ov8856_dual_otp_data[29];
	//checksum += low_val;
	 low_val= ov8856_dual_otp_data[30];
	//checksum += high_val;
	ov8856_otp_info.af_info.macro_cali = (high_val << 8 | low_val) >> 6;

	ov8856_otp_info.checksum = ov8856_dual_otp_data[8190]<<8|ov8856_dual_otp_data[8191];
	for (i = 0; i < OTP_DUAL_SIZE; i++) {
		checksum += ov8856_opt_lsc_data[i];
	}
	SENSOR_LOGI("checksum = 0x%x ov8856_otp_info.checksum = 0x%x", checksum, ov8856_otp_info.checksum);

	if ((checksum % 0xff) != ov8856_otp_info.checksum) {
		SENSOR_LOGI("checksum error!");
		ov8856_otp_info.program_flag = 0;
		return -1;
	}
	buffer = (cmr_u8 *)ov8856_dual_otp_data;
	ov8856_dual_otp_info.dual_otp.data_ptr = (void *)ov8856_dual_otp_data;
	ov8856_dual_otp_info.dual_otp.size = OTP_DUAL_SIZE;
	ov8856_dual_otp_info.single_otp_ptr = &ov8856_otp_info;
	ov8856_dual_otp_info.data_3d.data_ptr = (void *)(buffer + OV8856_DUAL_OTP_DATA3D_OFFSET);
	ov8856_dual_otp_info.data_3d.size = OV8856_DUAL_OTP_DATA3D_SIZE;
	buffer += OV8856_DUAL_OTP_MASTER_SLAVE_OFFSET;

	ov8856_dual_otp_info.master_lsc_info.lsc_data_addr = buffer;
	ov8856_dual_otp_info.master_lsc_info.lsc_data_size = OV8856_MASTER_LSC_INFO_LEN;
	buffer += OV8856_MASTER_LSC_INFO_LEN;

	ov8856_dual_otp_info.slave_lsc_info.lsc_data_addr = buffer;
	ov8856_dual_otp_info.slave_lsc_info.lsc_data_size = OV8856_SLAVE_LSC_INFO_LEN;
	buffer += OV8856_SLAVE_LSC_INFO_LEN;
	buffer += 25;
	ov8856_dual_otp_info.master_isp_awb_info.iso = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	ov8856_dual_otp_info.master_isp_awb_info.gain_r = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	ov8856_dual_otp_info.master_isp_awb_info.gain_g = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	ov8856_dual_otp_info.master_isp_awb_info.gain_b = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	ov8856_dual_otp_info.slave_isp_awb_info.iso = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	ov8856_dual_otp_info.slave_isp_awb_info.gain_r = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	ov8856_dual_otp_info.slave_isp_awb_info.gain_g = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	ov8856_dual_otp_info.slave_isp_awb_info.gain_b = (buffer[0] << 8 | buffer[1]);
	buffer += 2;
	SENSOR_LOGI("master iso r g b %d %d,%d,%d,lsc size=%d", ov8856_dual_otp_info.master_isp_awb_info.iso, ov8856_dual_otp_info.master_isp_awb_info.gain_r,
					ov8856_dual_otp_info.master_isp_awb_info.gain_g, ov8856_dual_otp_info.master_isp_awb_info.gain_b, ov8856_dual_otp_info.master_lsc_info.lsc_data_size);
	SENSOR_LOGI("slave iso r g b %d %d,%d,%d,lsc size=%d", ov8856_dual_otp_info.slave_isp_awb_info.iso, ov8856_dual_otp_info.slave_isp_awb_info.gain_r,
					ov8856_dual_otp_info.slave_isp_awb_info.gain_g, ov8856_dual_otp_info.slave_isp_awb_info.gain_b, ov8856_dual_otp_info.slave_lsc_info.lsc_data_size);

	SENSOR_LOGI("X");
	return 0;
}

static int ov8856_dual_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	static cmr_u8 first_flag = 1;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	//if (first_flag)
	{
		ov8856_dual_otp_data[0] = ov8856_i2c_read_otp(0x0000);
		if (1 != ov8856_dual_otp_data[0]) {
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			return -1;
		}
		checksum_total += ov8856_dual_otp_data[0];
		for (i = 1; i < OTP_DUAL_SIZE; i++) {
			ov8856_dual_otp_data[i] = ov8856_i2c_read_otp(0x0000 + i);
			if (i < OTP_DUAL_SIZE - 2)
				checksum_total += ov8856_dual_otp_data[i] ;
		}
		checksum = ov8856_dual_otp_data[OTP_DUAL_SIZE-2] << 8 | ov8856_dual_otp_data[OTP_DUAL_SIZE-1];
		SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);
		if ((checksum_total & 0xffff) != checksum ) {
			SENSOR_LOGI("checksum error!");
			return -1;
		}
		ret_value =ov8856_dual_otp_parse(handle);
		first_flag = 0;
	}
	SENSOR_LOGI("X");
	return ret_value;
}

static unsigned long ov8856_dual_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	ret_value = ov8856_dual_otp_read_data(handle);
	if (ret_value) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return ret_value;
	}
	param->pval = (void *)ov8856_dual_otp_data;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long ov8856_parse_dual_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u8 *buff = NULL;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;
	cmr_u32 ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	if (NULL == param->pval) {
		SENSOR_LOGI("ov8856_parse_dual_otp is NULL data");
		return -1;
	}
	buff = param->pval;
	ov8856_dual_otp_data[0] = buff[0];
	if (1 != ov8856_dual_otp_data[0]) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	}
	checksum_total += ov8856_dual_otp_data[0];
	for (i = 1; i < OTP_DUAL_SIZE; i++) {
		ov8856_dual_otp_data[i] = buff[i];
		if (i < OTP_DUAL_SIZE - 2)
			checksum_total += ov8856_dual_otp_data[i] ;
	}
	checksum = ov8856_dual_otp_data[OTP_DUAL_SIZE-2] << 8 | ov8856_dual_otp_data[OTP_DUAL_SIZE-1];
	SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);
	if ((checksum_total % 0xff) != checksum ) {
		SENSOR_LOGI("checksum error!");
		param->pval = NULL;
		return -1;
	}
	ret_value = ov8856_dual_otp_parse(handle);
	if (ret_value) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return ret_value;
	}
	//param->pval = (void *)&imx230_dual_otp_info;
	param->pval = (void *)&ov8856_dual_otp_info;//data;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}
#endif
#endif


