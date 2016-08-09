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

#if defined(CONFIG_CAMERA_RE_FOCUS)
	/*use dual otp info*/
	#define IMX230_DUAL_OTP
#endif

#define OTP_LSC_INFO_LEN 1658
static cmr_u8 imx230_opt_lsc_data[OTP_LSC_INFO_LEN];
static struct sensor_otp_cust_info imx230_otp_info;
#ifdef IMX230_DUAL_OTP
#define OTP_DUAL_SIZE 8192
static cmr_u8 imx230_dual_otp_data[OTP_DUAL_SIZE];
#endif

static cmr_u8 imx230_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr);
static int imx230_otp_init(SENSOR_HW_HANDLE handle);
static int imx230_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long imx230_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx230_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
#ifdef IMX230_DUAL_OTP
static unsigned long imx230_otp_dual_to_single(SENSOR_HW_HANDLE handle);
static int imx230_dual_otp_read_data(SENSOR_HW_HANDLE handle);
static unsigned long imx230_dual_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
static unsigned long imx230_parse_dual_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param);
#endif

#define  imx230_i2c_read_otp(addr)    imx230_i2c_read_otp_set(handle, addr)

static cmr_u8 imx230_i2c_read_otp_set(SENSOR_HW_HANDLE handle, cmr_u16 addr)
{
	return sensor_grc_read_i2c(0xA0 >> 1, addr, BITS_ADDR16_REG8);
}


static int imx230_otp_init(SENSOR_HW_HANDLE handle)
{
	return 0;
}

static int imx230_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;
	static cmr_u8 first_flag = 1;

	SENSOR_LOGI("E");
	//if (first_flag)
	{
		imx230_otp_info.program_flag = imx230_i2c_read_otp(0x0000);
		SENSOR_LOGI("program_flag = %d", imx230_otp_info.program_flag);
		if (1 != imx230_otp_info.program_flag) {
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			return -1;
		}
		checksum += imx230_otp_info.program_flag;
		imx230_otp_info.module_info.year = imx230_i2c_read_otp(0x0001);
		checksum += imx230_otp_info.module_info.year;
		imx230_otp_info.module_info.month = imx230_i2c_read_otp(0x0002);
		checksum += imx230_otp_info.module_info.month;
		imx230_otp_info.module_info.day = imx230_i2c_read_otp(0x0003);
		checksum += imx230_otp_info.module_info.day;
		imx230_otp_info.module_info.mid = imx230_i2c_read_otp(0x0004);
		checksum += imx230_otp_info.module_info.mid;
		imx230_otp_info.module_info.lens_id = imx230_i2c_read_otp(0x0005);
		checksum += imx230_otp_info.module_info.lens_id;
		imx230_otp_info.module_info.vcm_id = imx230_i2c_read_otp(0x0006);
		checksum += imx230_otp_info.module_info.vcm_id;
		imx230_otp_info.module_info.driver_ic_id = imx230_i2c_read_otp(0x0007);
		checksum += imx230_otp_info.module_info.driver_ic_id;

		high_val = imx230_i2c_read_otp(0x0010);
		checksum += high_val;
		low_val = imx230_i2c_read_otp(0x0011);
		checksum += low_val;
		imx230_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
		high_val = imx230_i2c_read_otp(0x0012);
		checksum += high_val;
		low_val = imx230_i2c_read_otp(0x0013);
		checksum += low_val;
		imx230_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = imx230_i2c_read_otp(0x0014);
		checksum += high_val;
		low_val = imx230_i2c_read_otp(0x0015);
		checksum += low_val;
		imx230_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = imx230_i2c_read_otp(0x0016);
		checksum += high_val;
		low_val = imx230_i2c_read_otp(0x0017);
		checksum += low_val;
		imx230_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

		for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
			imx230_opt_lsc_data[i] = imx230_i2c_read_otp(0x0020 + i);
			checksum += imx230_opt_lsc_data[i];
		}
		imx230_otp_info.lsc_info.lsc_data_addr = imx230_opt_lsc_data;
		imx230_otp_info.lsc_info.lsc_data_size = sizeof(imx230_opt_lsc_data);

		imx230_otp_info.af_info.flag = imx230_i2c_read_otp(0x06A0);
		if (0 == imx230_otp_info.af_info.flag)
			SENSOR_LOGI("af otp is wrong");

		checksum += imx230_otp_info.af_info.flag;

		low_val = imx230_i2c_read_otp(0x06A1);
		checksum += low_val;
		high_val = imx230_i2c_read_otp(0x06A2);
		checksum += high_val;
		imx230_otp_info.af_info.infinite_cali = (high_val << 8 | low_val);
		low_val = imx230_i2c_read_otp(0x06A3);
		checksum += low_val;
		high_val = imx230_i2c_read_otp(0x06A4);
		checksum += high_val;
		imx230_otp_info.af_info.macro_cali = (high_val << 8 | low_val);

		imx230_otp_info.checksum = imx230_i2c_read_otp(0x06A5);

		SENSOR_LOGI("checksum = 0x%x imx230_otp_info.checksum = 0x%x", checksum, imx230_otp_info.checksum);

		if ((checksum % 0xff) != imx230_otp_info.checksum) {
			SENSOR_LOGI("checksum error!");
			imx230_otp_info.program_flag = 0;
			return -1;
		}
		first_flag = 0;
	}
	SENSOR_LOGI("X");
	return 0;
}

static unsigned long imx230_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;

	SENSOR_LOGI("E");
	imx230_otp_read_data(handle);
	otp_info = &imx230_otp_info;

	if (1 != otp_info->program_flag) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	}
	param->pval = (void *)otp_info;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long imx230_parse_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	struct sensor_otp_cust_info *otp_info = NULL;
	cmr_u8 *buff = NULL;
	cmr_u16 i = 0;
	cmr_u16 j = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	SENSOR_LOGI("E");
	if (NULL == param->pval) {
		SENSOR_LOGI("imx230_parse_otp is NULL data");
		return -1;
	}
	buff = param->pval;

	if (1 != buff[0]) {
		SENSOR_LOGI("imx230_parse_otp is wrong data");
		param->pval = NULL;
		return -1;
	} else {
		imx230_otp_info.program_flag = buff[i++];
		SENSOR_LOGI("program_flag = %d", imx230_otp_info.program_flag);

		checksum += imx230_otp_info.program_flag;
		imx230_otp_info.module_info.year = buff[i++];
		checksum += imx230_otp_info.module_info.year;
		imx230_otp_info.module_info.month = buff[i++];
		checksum += imx230_otp_info.module_info.month;
		imx230_otp_info.module_info.day = buff[i++];
		checksum += imx230_otp_info.module_info.day;
		imx230_otp_info.module_info.mid = buff[i++];
		checksum += imx230_otp_info.module_info.mid;
		imx230_otp_info.module_info.lens_id = buff[i++];
		checksum += imx230_otp_info.module_info.lens_id;
		imx230_otp_info.module_info.vcm_id = buff[i++];
		checksum += imx230_otp_info.module_info.vcm_id;
		imx230_otp_info.module_info.driver_ic_id = buff[i++];
		checksum += imx230_otp_info.module_info.driver_ic_id;

		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx230_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx230_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx230_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
		high_val = buff[i++];
		checksum += high_val;
		low_val = buff[i++];
		checksum += low_val;
		imx230_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

		for (j = 0; j < OTP_LSC_INFO_LEN; j++) {
			imx230_opt_lsc_data[j] = buff[i++];
			checksum += imx230_opt_lsc_data[j];
		}
		imx230_otp_info.lsc_info.lsc_data_addr = imx230_opt_lsc_data;
		imx230_otp_info.lsc_info.lsc_data_size = sizeof(imx230_opt_lsc_data);

		imx230_otp_info.af_info.flag = buff[i++];
		if (0 == imx230_otp_info.af_info.flag)
			SENSOR_LOGI("af otp is wrong");

		checksum += imx230_otp_info.af_info.flag;
		/* cause checksum, skip af flag */
		low_val = buff[i++];
		checksum += low_val;
		high_val = buff[i++];
		checksum += high_val;
		imx230_otp_info.af_info.infinite_cali = (high_val << 8 | low_val);
		low_val = buff[i++];
		checksum += low_val;
		high_val = buff[i++];
		checksum += high_val;
		imx230_otp_info.af_info.macro_cali = (high_val << 8 | low_val);

		imx230_otp_info.checksum = buff[i++];
		SENSOR_LOGI("checksum = 0x%x imx230_otp_info.checksum = 0x%x", checksum, imx230_otp_info.checksum);

		if ((checksum % 0xff) != imx230_otp_info.checksum) {
			SENSOR_LOGI("checksum error!");
			imx230_otp_info.program_flag = 0;
			param->pval = NULL;
			return -1;
		}
		otp_info = &imx230_otp_info;
		param->pval = (void *)otp_info;
	}
	SENSOR_LOGI("param->pval = %p", param->pval);

	return 0;
}

#ifdef IMX230_DUAL_OTP
static unsigned long imx230_otp_dual_to_single(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	cmr_u8 high_val = 0;
	cmr_u8 low_val = 0;
	cmr_u32 checksum = 0;

	SENSOR_LOGI("E");
	imx230_otp_info.program_flag = imx230_dual_otp_data[0];
	SENSOR_LOGI("program_flag = %d", imx230_otp_info.program_flag);
	if (1 != imx230_otp_info.program_flag) {
		SENSOR_LOGI("failed to read otp or the otp is wrong data");
		return -1;
	}
	checksum += imx230_otp_info.program_flag;
	imx230_otp_info.module_info.year = imx230_dual_otp_data[1];
	checksum += imx230_otp_info.module_info.year;
	imx230_otp_info.module_info.month = imx230_dual_otp_data[2];
	checksum += imx230_otp_info.module_info.month;
	imx230_otp_info.module_info.day = imx230_dual_otp_data[3];
	checksum += imx230_otp_info.module_info.day;
	imx230_otp_info.module_info.mid = imx230_dual_otp_data[4];
	checksum += imx230_otp_info.module_info.mid;
	imx230_otp_info.module_info.lens_id = imx230_dual_otp_data[5];
	checksum += imx230_otp_info.module_info.lens_id;
	imx230_otp_info.module_info.vcm_id = imx230_dual_otp_data[6];
	checksum += imx230_otp_info.module_info.vcm_id;
	imx230_otp_info.module_info.driver_ic_id = imx230_dual_otp_data[7];
	checksum += imx230_otp_info.module_info.driver_ic_id;

	high_val = imx230_dual_otp_data[16];
	checksum += high_val;
	low_val = imx230_dual_otp_data[17];
	checksum += low_val;
	imx230_otp_info.isp_awb_info.iso = (high_val << 8 | low_val);
	high_val = imx230_dual_otp_data[18];
	checksum += high_val;
	low_val = imx230_dual_otp_data[19];
	checksum += low_val;
	imx230_otp_info.isp_awb_info.gain_r = (high_val << 8 | low_val);
	high_val = imx230_dual_otp_data[20];
	checksum += high_val;
	low_val = imx230_dual_otp_data[21];
	checksum += low_val;
	imx230_otp_info.isp_awb_info.gain_g = (high_val << 8 | low_val);
	high_val = imx230_dual_otp_data[22];
	checksum += high_val;
	low_val = imx230_dual_otp_data[23];
	checksum += low_val;
	imx230_otp_info.isp_awb_info.gain_b = (high_val << 8 | low_val);

	for (i = 0; i < OTP_LSC_INFO_LEN; i++) {
		imx230_opt_lsc_data[i] = imx230_dual_otp_data[32+i];
		checksum += imx230_opt_lsc_data[i];
	}

	imx230_otp_info.lsc_info.lsc_data_addr = imx230_opt_lsc_data;
	imx230_otp_info.lsc_info.lsc_data_size = sizeof(imx230_opt_lsc_data);

	imx230_otp_info.af_info.flag = imx230_dual_otp_data[1696];
	if (0 == imx230_otp_info.af_info.flag)
		SENSOR_LOGI("af otp is wrong");

	checksum += imx230_otp_info.af_info.flag;

	low_val = imx230_dual_otp_data[1697];
	checksum += low_val;
	high_val = imx230_dual_otp_data[1698];
	checksum += high_val;
	imx230_otp_info.af_info.infinite_cali = (high_val << 8 | low_val);
	low_val = imx230_dual_otp_data[1699];
	checksum += low_val;
	high_val = imx230_dual_otp_data[1700];
	checksum += high_val;
	imx230_otp_info.af_info.macro_cali = (high_val << 8 | low_val);

	imx230_otp_info.checksum = imx230_dual_otp_data[1701];

	SENSOR_LOGI("checksum = 0x%x imx230_otp_info.checksum = 0x%x", checksum, imx230_otp_info.checksum);

	if ((checksum % 0xff) != imx230_otp_info.checksum) {
		SENSOR_LOGI("checksum error!");
		imx230_otp_info.program_flag = 0;
		return -1;
	}
	SENSOR_LOGI("X");
	return 0;
}

static int imx230_dual_otp_read_data(SENSOR_HW_HANDLE handle)
{
	cmr_u16 i = 0;
	static cmr_u8 first_flag = 1;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;

	SENSOR_LOGI("E");
	//if (first_flag)
	{
		imx230_dual_otp_data[0] = imx230_i2c_read_otp(0x0000);
		if (1 != imx230_dual_otp_data[0]) {
			SENSOR_LOGI("failed to read otp or the otp is wrong data");
			return -1;
		}
		checksum_total += imx230_dual_otp_data[0];
		for (i = 1; i < OTP_DUAL_SIZE; i++) {
			imx230_dual_otp_data[i] = imx230_i2c_read_otp(0x0000 + i);
			if (i < OTP_DUAL_SIZE - 2)
				checksum_total += imx230_dual_otp_data[i] ;
		}
		checksum = imx230_dual_otp_data[OTP_DUAL_SIZE-2] << 8 | imx230_dual_otp_data[OTP_DUAL_SIZE-1];
		SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);
		if ((checksum_total & 0xffff) != checksum ) {
			SENSOR_LOGI("checksum error!");
			return -1;
		}
		first_flag = 0;
	}
	SENSOR_LOGI("X");
	return 0;
}

static unsigned long imx230_dual_otp_read(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	SENSOR_LOGI("E");
	ret_value = imx230_dual_otp_read_data(handle);
	if (ret_value) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return ret_value;
	}
	param->pval = (void *)imx230_dual_otp_data;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}

static unsigned long imx230_parse_dual_otp(SENSOR_HW_HANDLE handle, SENSOR_VAL_T* param)
{
	cmr_u16 i = 0;
	cmr_u8 *buff = NULL;
	cmr_u16 checksum = 0;
	cmr_u32 checksum_total = 0;

	SENSOR_LOGI("E");
	if (NULL == param->pval) {
		SENSOR_LOGI("imx230_parse_dual_otp is NULL data");
		return -1;
	}
	buff = param->pval;
	imx230_dual_otp_data[0] = buff[0];
	if (1 != imx230_dual_otp_data[0]) {
		SENSOR_LOGI("otp error");
		param->pval = NULL;
		return -1;
	}
	checksum_total += imx230_dual_otp_data[0];
	for (i = 1; i < OTP_DUAL_SIZE; i++) {
		imx230_dual_otp_data[i] = buff[i];
		if (i < OTP_DUAL_SIZE - 2)
			checksum_total += imx230_dual_otp_data[i] ;
	}
	checksum = imx230_dual_otp_data[OTP_DUAL_SIZE-2] << 8 | imx230_dual_otp_data[OTP_DUAL_SIZE-1];
	SENSOR_LOGI("checksum_total=0x%x, checksum=0x%x", checksum_total, checksum);
	if ((checksum_total & 0xffff) != checksum ) {
		SENSOR_LOGI("checksum error!");
		param->pval = NULL;
		return -1;
	}
	param->pval = (void *)imx230_dual_otp_data;
	SENSOR_LOGI("param->pval = %p", param->pval);
	return 0;
}
#endif
#endif
