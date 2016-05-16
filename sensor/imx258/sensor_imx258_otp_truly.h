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
#ifndef _SENSOR_IMX230_OTP_TRULY_H_
#define _SENSOR_IMX230_OTP_TRULY_H_

/*use dual otp info*/
#define IMX230_DUAL_OTP        1

#define OTP_LSC_INFO_LEN 1658
static cmr_u8 imx258_opt_lsc_data[OTP_LSC_INFO_LEN];
static struct sensor_otp_cust_info imx258_otp_info;
#if IMX230_DUAL_OTP
#define OTP_DUAL_SIZE 8192
static cmr_u8 imx258_dual_otp_data[OTP_DUAL_SIZE];
#endif

static cmr_u8 imx258_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr);
static int imx258_otp_init(SENSOR_HW_HANDLE handle);
static int imx258_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long imx258_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx258_parse_single_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx258_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
#if IMX230_DUAL_OTP
static unsigned long imx258_otp_dual_to_single(SENSOR_HW_HANDLE handle);
static int imx258_dual_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long imx258_dual_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx258_parse_dual_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
#endif

#define  imx258_i2c_read_otp(addr)    imx258_i2c_read_otp_set(handle, addr)

static cmr_u8 imx258_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr)
{
	return sensor_grc_read_i2c(0xA0 >> 1, addr, BITS_ADDR16_REG8);
}


static int imx258_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static int imx258_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;
	static cmr_u8 first_flag = 1;

	SENSOR_PRINT("E");
	if (first_flag)
	{
		imx258_otp_info.program_flag = imx258_i2c_read_otp(0x0000);
		SENSOR_PRINT("program_flag = %d", imx258_otp_info.program_flag);
		if (1 != imx258_otp_info.program_flag) {
			SENSOR_PRINT("failed to read otp or the otp is wrong data");
			return -1;
		}
		checksum += imx258_otp_info.program_flag;
		imx258_otp_info.module_info.year = imx258_i2c_read_otp(0x0001);
		checksum += imx258_otp_info.module_info.year;
		imx258_otp_info.module_info.month = imx258_i2c_read_otp(0x0002);
		checksum += imx258_otp_info.module_info.month;
		imx258_otp_info.module_info.day = imx258_i2c_read_otp(0x0003);
		checksum += imx258_otp_info.module_info.day;
		imx258_otp_info.module_info.mid = imx258_i2c_read_otp(0x0004);
		checksum += imx258_otp_info.module_info.mid;
		imx258_otp_info.module_info.lens_id = imx258_i2c_read_otp(0x0005);
		checksum += imx258_otp_info.module_info.lens_id;
		imx258_otp_info.module_info.vcm_id = imx258_i2c_read_otp(0x0006);
		checksum += imx258_otp_info.module_info.vcm_id;
		imx258_otp_info.module_info.driver_ic_id = imx258_i2c_read_otp(0x0007);
		checksum += imx258_otp_info.module_info.driver_ic_id;

		high_val = imx258_i2c_read_otp(0x0010);
		checksum += high_val;
		low_val = imx258_i2c_read_otp(0x0011);
		checksum += low_val;
		imx258_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
		high_val = imx258_i2c_read_otp(0x0012);
		checksum += high_val;
		low_val = imx258_i2c_read_otp(0x0013);
		checksum += low_val;
		imx258_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = imx258_i2c_read_otp(0x0014);
		checksum += high_val;
		low_val = imx258_i2c_read_otp(0x0015);
		checksum += low_val;
		imx258_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = imx258_i2c_read_otp(0x0016);
		checksum += high_val;
		low_val = imx258_i2c_read_otp(0x0017);
		checksum += low_val;
		imx258_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

		for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
			imx258_opt_lsc_data[i] = imx258_i2c_read_otp(0x0020 + i);
			checksum += imx258_opt_lsc_data[i];
		}
		imx258_otp_info.lsc_info.lsc_data_addr = imx258_opt_lsc_data;
		imx258_otp_info.lsc_info.lsc_data_size = sizeof(imx258_opt_lsc_data);

		imx258_otp_info.af_info.flag = imx258_i2c_read_otp(0x06A0);
		if (0 == imx258_otp_info.af_info.flag)
			SENSOR_PRINT("af otp is wrong");

		checksum += imx258_otp_info.af_info.flag;

		low_val = imx258_i2c_read_otp(0x06A1);
		checksum += low_val;
		high_val = imx258_i2c_read_otp(0x06A2);
		checksum += high_val;
		imx258_otp_info.af_info.infinite_cali = (high_val << 8 | low_val);
		low_val = imx258_i2c_read_otp(0x06A3);
		checksum += low_val;
		high_val = imx258_i2c_read_otp(0x06A4);
		checksum += high_val;
		imx258_otp_info.af_info.macro_cali = (high_val << 8 | low_val);

		imx258_otp_info.checksum = imx258_i2c_read_otp(0x06A5);

		SENSOR_PRINT("checksum = 0x%x imx258_otp_info.checksum = 0x%x", checksum, imx258_otp_info.checksum);

		if ((checksum % 0xff) != imx258_otp_info.checksum) {
			SENSOR_PRINT_ERR("checksum error!");
			imx258_otp_info.program_flag = 0;
			return -1;
		}
		first_flag = 0;
	}
	SENSOR_PRINT("X");
	return 0;
}

static unsigned long imx258_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;

	SENSOR_PRINT("E");
#if IMX230_DUAL_OTP
	imx258_dual_otp_read_data(handle);
	imx258_otp_dual_to_single(handle);
#else
	imx258_otp_read_data(handle);
#endif
	otp_info = &imx258_otp_info;

	if (1 != otp_info->program_flag) {
		SENSOR_PRINT_ERR("otp error");
		param->pval = NULL;
		return -1;
	} else {
		param->pval = (void *)otp_info;
	}
	SENSOR_PRINT("param->pval = %p", param->pval);
	return 0;
}

static unsigned long imx258_parse_single_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;
	cmr_u8 *buff = NULL;
	cmr_u16 i = 0;
	cmr_u16 j = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	SENSOR_PRINT("E");
	if (NULL == param->pval) {
		SENSOR_PRINT("imx258_parse_single_otp is NULL data");
		return -1;
	}
	buff = param->pval;

	if (1 != buff[0]) {
		SENSOR_PRINT("imx258_parse_single_otp is wrong data");
		param->pval = NULL;
		return -1;
	} else {
		imx258_otp_info.program_flag = buff[i++];
		SENSOR_PRINT("program_flag = %d", imx258_otp_info.program_flag);

		checksum += imx258_otp_info.program_flag;
		imx258_otp_info.module_info.year = buff[i++];
		checksum += imx258_otp_info.module_info.year;
		imx258_otp_info.module_info.month = buff[i++];
		checksum += imx258_otp_info.module_info.month;
		imx258_otp_info.module_info.day = buff[i++];
		checksum += imx258_otp_info.module_info.day;
		imx258_otp_info.module_info.mid = buff[i++];
		checksum += imx258_otp_info.module_info.mid;
		imx258_otp_info.module_info.lens_id = buff[i++];
		checksum += imx258_otp_info.module_info.lens_id;
		imx258_otp_info.module_info.vcm_id = buff[i++];
		checksum += imx258_otp_info.module_info.vcm_id;
		imx258_otp_info.module_info.driver_ic_id = buff[i++];
		checksum += imx258_otp_info.module_info.driver_ic_id;

		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx258_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx258_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx258_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx258_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

		for (j = 0; j < OTP_LSC_INFO_LEN; j++) {
			imx258_opt_lsc_data[j] = buff[i++];
			checksum += imx258_opt_lsc_data[j];
		}
		imx258_otp_info.lsc_info.lsc_data_addr = imx258_opt_lsc_data;
		imx258_otp_info.lsc_info.lsc_data_size = sizeof(imx258_opt_lsc_data);

		imx258_otp_info.af_info.flag = buff[i++];
		if (0 == imx258_otp_info.af_info.flag)
			SENSOR_PRINT("af otp is wrong");

		checksum += imx258_otp_info.af_info.flag;
		/* cause checksum, skip af flag */
		low_val = buff[i++];
		checksum += low_val;
		high_val = buff[i++];
		checksum += high_val;
		imx258_otp_info.af_info.infinite_cali = (high_val << 8 | low_val);
		low_val = buff[i++];
		checksum += low_val;
		high_val = buff[i++];
		checksum += high_val;
		imx258_otp_info.af_info.macro_cali = (high_val << 8 | low_val);

		imx258_otp_info.checksum = buff[i++];
		SENSOR_PRINT("checksum = 0x%x imx258_otp_info.checksum = 0x%x", checksum, imx258_otp_info.checksum);

		if ((checksum % 0xff) != imx258_otp_info.checksum) {
			SENSOR_PRINT_ERR("checksum error!");
			imx258_otp_info.program_flag = 0;
			param->pval = NULL;
			return -1;
		}
		otp_info = &imx258_otp_info;
		param->pval = (void *)otp_info;
	}
	SENSOR_PRINT("param->pval = %p", param->pval);

	return 0;
}

static unsigned long imx258_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;
	cmr_u16 i = 0;
	cmr_u8 *buff = NULL;

	SENSOR_PRINT("E");
#if IMX230_DUAL_OTP
	imx258_parse_dual_otp(handle, param);
	if (NULL == param->pval) {
		SENSOR_PRINT("imx258_parse_otp is NULL data");
		return -1;
	}
	imx258_otp_dual_to_single(handle);
	otp_info = &imx258_otp_info;

	if (1 != otp_info->program_flag) {
		SENSOR_PRINT_ERR("otp error");
		param->pval = NULL;
		return -1;
	} else {
		param->pval = (void *)otp_info;
	}
	SENSOR_PRINT("param->pval = %p", param->pval);
#else
	imx258_parse_single_otp(handle, param);
#endif

	return 0;
}

#if IMX230_DUAL_OTP
static unsigned long imx258_otp_dual_to_single(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	SENSOR_PRINT("E");
	imx258_otp_info.program_flag = imx258_dual_otp_data[0];
	SENSOR_PRINT("program_flag = %d", imx258_otp_info.program_flag);
	if (1 != imx258_otp_info.program_flag) {
		SENSOR_PRINT("failed to read otp or the otp is wrong data");
		return -1;
	}
	checksum += imx258_otp_info.program_flag;
	imx258_otp_info.module_info.year = imx258_dual_otp_data[1];
	checksum += imx258_otp_info.module_info.year;
	imx258_otp_info.module_info.month = imx258_dual_otp_data[2];
	checksum += imx258_otp_info.module_info.month;
	imx258_otp_info.module_info.day = imx258_dual_otp_data[3];
	checksum += imx258_otp_info.module_info.day;
	imx258_otp_info.module_info.mid = imx258_dual_otp_data[4];
	checksum += imx258_otp_info.module_info.mid;
	imx258_otp_info.module_info.lens_id = imx258_dual_otp_data[5];
	checksum += imx258_otp_info.module_info.lens_id;
	imx258_otp_info.module_info.vcm_id = imx258_dual_otp_data[6];
	checksum += imx258_otp_info.module_info.vcm_id;
	imx258_otp_info.module_info.driver_ic_id = imx258_dual_otp_data[7];
	checksum += imx258_otp_info.module_info.driver_ic_id;

	high_val = imx258_dual_otp_data[16];
	checksum += high_val;
	low_val = imx258_dual_otp_data[17];
	checksum += low_val;
	imx258_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
	high_val = imx258_dual_otp_data[18];
	checksum += high_val;
	low_val = imx258_dual_otp_data[19];
	checksum += low_val;
	imx258_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
	high_val = imx258_dual_otp_data[20];
	checksum += high_val;
	low_val = imx258_dual_otp_data[21];
	checksum += low_val;
	imx258_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
	high_val = imx258_dual_otp_data[22];
	checksum += high_val;
	low_val = imx258_dual_otp_data[23];
	checksum += low_val;
	imx258_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

	for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
		imx258_opt_lsc_data[i] = imx258_dual_otp_data[32+i];
		checksum += imx258_opt_lsc_data[i];
	}

	imx258_otp_info.lsc_info.lsc_data_addr = imx258_opt_lsc_data;
	imx258_otp_info.lsc_info.lsc_data_size = sizeof(imx258_opt_lsc_data);

	imx258_otp_info.af_info.flag = imx258_dual_otp_data[1696];
	if (0 == imx258_otp_info.af_info.flag)
		SENSOR_PRINT("af otp is wrong");

	checksum += imx258_otp_info.af_info.flag;

	low_val = imx258_dual_otp_data[1697];
	checksum += low_val;
	high_val = imx258_dual_otp_data[1698];
	checksum += high_val;
	imx258_otp_info.af_info.infinite_cali = (high_val << 8 | low_val);
	low_val = imx258_dual_otp_data[1699];
	checksum += low_val;
	high_val = imx258_dual_otp_data[1700];
	checksum += high_val;
	imx258_otp_info.af_info.macro_cali = (high_val << 8 | low_val);

	imx258_otp_info.checksum = imx258_dual_otp_data[1701];

	SENSOR_PRINT("checksum = 0x%x imx258_otp_info.checksum = 0x%x", checksum, imx258_otp_info.checksum);

	if ((checksum % 0xff) != imx258_otp_info.checksum) {
		SENSOR_PRINT_ERR("checksum error!");
		imx258_otp_info.program_flag = 0;
		return -1;
	}
	SENSOR_PRINT("X");
	return 0;
}

static int imx258_dual_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	static cmr_u8 first_flag = 1;

	SENSOR_PRINT("E");
	if (first_flag)
	{
		imx258_dual_otp_data[0] = imx258_i2c_read_otp(0x0000);
		if (1 != imx258_dual_otp_data[0]) {
			SENSOR_PRINT("failed to read otp or the otp is wrong data");
			return -1;
		}
		for (i = 1; i < OTP_DUAL_SIZE; i++) {
			imx258_dual_otp_data[i] = imx258_i2c_read_otp(0x0000 + i);
		}
		first_flag = 0;
	}
	SENSOR_PRINT("X");
	return 0;
}

static unsigned long imx258_dual_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;

	SENSOR_PRINT("E");
	imx258_dual_otp_read_data(handle);
	if (1 != imx258_dual_otp_data[0]) {
		SENSOR_PRINT_ERR("otp error");
		param->pval = NULL;
		return -1;
	}
	param->pval = (void *)imx258_dual_otp_data;
	SENSOR_PRINT("param->pval = %p", param->pval);
	return 0;
}

static unsigned long imx258_parse_dual_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u8 *buff = NULL;

	SENSOR_PRINT("E");
	if (NULL == param->pval) {
		SENSOR_PRINT("imx258_parse_dual_otp is NULL data");
		return -1;
	}
	buff = param->pval;
	imx258_dual_otp_data[0] = buff[0];
	if (1 != imx258_dual_otp_data[0]) {
		SENSOR_PRINT_ERR("otp error");
		param->pval = NULL;
		return -1;
	}
	for (i = 1; i < OTP_DUAL_SIZE; i++) {
		imx258_dual_otp_data[i] = buff[i];
	}
	param->pval = (void *)imx258_dual_otp_data;
	SENSOR_PRINT("param->pval = %p", param->pval);
	return 0;
}
#endif
#endif
