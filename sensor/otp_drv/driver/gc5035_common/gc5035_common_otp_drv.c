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
 * V1.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#include "gc5035_common_otp_drv.h"

static cmr_int	group_index = 0;

/*==============================================================================
 * Description:
 * read register value from sensor
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_i2c_read(void *otp_drv_handle, uint16_t slave_addr, uint16_t memory_addr, uint8_t *memory_data)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

	uint8_t cmd_val[5] = { 0x00 };
	uint16_t cmd_len = 0;	

	cmd_val[0] = memory_addr & 0xff;
	cmd_len = 1;

	ret = hw_Sensor_ReadI2C(otp_cxt->hw_handle, slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
	if (OTP_CAMERA_SUCCESS == ret) {
		*memory_data  = cmd_val[0];
	}

	return ret;

}

/*==============================================================================
 * Description:
 * read register value from sensor
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_i2c_write(void *otp_drv_handle, uint16_t slave_addr, uint16_t memory_addr, uint16_t memory_data)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

	uint8_t cmd_val[5] = { 0x00 };
	uint16_t cmd_len = 0;

	cmd_val[0] = memory_addr & 0xff;
	cmd_val[1] = memory_data;
	cmd_len = 2;
	ret = hw_Sensor_WriteI2C(otp_cxt->hw_handle, slave_addr, (uint8_t *) &cmd_val[0], cmd_len);

	return ret;
}

static uint8_t gc5035_common_otp_enable(cmr_handle otp_drv_handle, uint8_t state)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	cmr_u16 VALUE;
	OTP_LOGI("in");

	if (state) {
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfc, 0x01);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf4, 0x40);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf5, 0xe9);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf6, 0x14);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf8, 0x49);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf9, 0x82);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfa, 0x00);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfc, 0x81);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x36, 0x01);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xd3, 0x87);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x36, 0x00);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x33, 0x00);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf7, 0x01);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfc, 0x8e);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xee, 0x30);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfa, 0x10);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf5, 0xe9);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x02); 
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x67, 0xc0); 
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x59, 0x3f);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x55, 0x80);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x65, 0x80);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x66, 0x03);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);
		OTP_LOGI("Enable OTP!");
	} else {
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x02);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x67, 0x00);
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00); 
		gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfa, 0x00); 
		OTP_LOGI("Disable OTP!");
	}

	return ret;
}

static uint8_t gc5035_common_read_otp_group(cmr_handle otp_drv_handle, uint16_t addr, uint8_t *buff, uint16_t size)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	cmr_u16 i;

	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x02);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x69, (addr >> 8) & 0x1f);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x6a, addr & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf3, 0x20);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf3, 0x12);

	for (i = 0; i < size; i++){
		gc5035_common_i2c_read(otp_drv_handle, SENSOR_I2C_ADDR, 0x6c, &buff[i]);
	}

	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xf3, 0x00);

	return ret;
}


 /*==============================================================================
 * Description:
 * otp data checksum
 *	-buff:address of otp buffer
 *	-offset: the start address of the section
 *	-data_count: data count of the section
 *	-check_sum_offset: the section checksum offset
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_section_checksum(unsigned char *buf, unsigned int offset, unsigned int data_count, unsigned int check_sum_offset) 
{
	uint32_t ret = OTP_CAMERA_SUCCESS;
	uint32_t i = 0, sum = 0;

	OTP_LOGI("in");
	for (i = offset; i < offset + data_count; i++) {
		sum += buf[i];
	}
	if ((sum % 255 + 1) == buf[check_sum_offset]) {
		ret = OTP_CAMERA_SUCCESS;
	} else {
		ret = OTP_CAMERA_FAIL;
	}
	OTP_LOGI("out: offset:0x%x, sum:0x%x, checksum:0x%x", check_sum_offset, sum,
			buf[check_sum_offset]);
	return ret;
}

/*==============================================================================
 * Description:
 * inital otp data buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_buffer_init(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	cmr_int otp_len;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/*include random and golden lsc otp data,add reserve*/
	otp_len = sizeof(otp_format_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER;
	otp_format_data_t *otp_data =(otp_format_data_t *)sensor_otp_get_formatted_buffer(otp_len, otp_cxt->sensor_id);

	if (NULL == otp_data) {
		OTP_LOGE("malloc otp data buffer failed.\n");
		ret = OTP_CAMERA_FAIL;
	} 

	otp_cxt->otp_data = otp_data;
	OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * parse otp data of module info
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_parse_module_data(cmr_handle otp_drv_handle) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/* module info data */
	otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
	cmr_u8 *module_info = malloc(MODULE_INFO_SIZE * sizeof(cmr_u8));
	cmr_u8 *module_src_dat = otp_cxt->otp_raw_data.buffer + MODULE_INFO_OFFSET;
	cmr_u8 module_flag = *(otp_cxt->otp_raw_data.buffer + MODULE_INFO_GROUP_FLAG_OFFSET);

	if (!module_info) {
		OTP_LOGI("malloc convert data buffer failed");
		return OTP_CAMERA_FAIL;
	}

	memset(module_info, 0, MODULE_INFO_SIZE* sizeof(cmr_u8));

	OTP_LOGI("module_flag = 0x%x", module_flag);

	if ((module_flag & 0x0c) == 0x04) {
		OTP_LOGI("group 1 is valid!");
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, MODULE_INFO_OFFSET,
						MODULE_INFO_SIZE - 1, MODULE_INFO_CHECKSUM);
		if (!ret) {
			OTP_LOGI("module id = 0x%x", *(module_src_dat));
			OTP_LOGI("lens id = 0x%x", *(module_src_dat + 1));
			OTP_LOGI("year = %d", *(module_src_dat + 2));
			OTP_LOGI("month = %d", *(module_src_dat + 3));
			OTP_LOGI("day = %d",*(module_src_dat + 4));
		} else {
			OTP_LOGE("module data checksum error, parse failed!");
			ret = OTP_CAMERA_FAIL;
			}
	} else if ((module_flag & 0x03) == 0x01) {
		OTP_LOGI("group 2 is Valid!");
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, MODULE_INFO_OFFSET + MODULE_INFO_SIZE,
						MODULE_INFO_SIZE - 1, MODULE_INFO_CHECKSUM + MODULE_INFO_SIZE);
		if (!ret) {
			OTP_LOGI("module id = 0x%x", *(module_src_dat + 6));
			OTP_LOGI("lens id = 0x%x", *(module_src_dat + 7));
			OTP_LOGI("year = %d", *(module_src_dat + 8));
			OTP_LOGI("month = %d", *(module_src_dat + 9));
			OTP_LOGI("day = %d",*(module_src_dat + 10));
		} else {
			OTP_LOGE("module data checksum error, parse failed!");
			ret = OTP_CAMERA_FAIL;
			}
	} else if ((module_flag & 0x0f) == 0x00){
		OTP_LOGE("module info is empty!");
		ret = OTP_CAMERA_FAIL;
	} else {
		OTP_LOGE("module info is invalid!");
		ret = OTP_CAMERA_FAIL;
	}

	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("module data parse error, parse failed");
		free(module_info);
		return ret;
	} else {
		*(module_info) = 0x00;
		*(module_info + 1) = 0x00;
		*(module_info + 2) = 0x00;
		*(module_info + 3) = 0x00;
		*(module_info + 4) = 0x00;
		*(module_info + 5) = 0x01;
		module_dat->rdm_info.buffer = module_info;
		module_dat->rdm_info.size = 6;
		module_dat->gld_info.buffer = NULL;
		module_dat->gld_info.size = 0;
	}

	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * parse otp data of af
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_parse_af_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/* af data */
	otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
/*
	cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;
	cmr_u8 af_flag = *(otp_cxt->otp_raw_data.buffer + AF_INFO_GROUP_FLAG_OFFSET);
	cmr_u8 af_data[4] = {0x00, 0x00, 0x00, 0x00};

	if ((af_flag & 0x0c) == 0x04) {
		OTP_LOGE("af group 1 is valid!");
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
					AF_INFO_SIZE - 1, AF_INFO_CHECKSUM);
		if (!ret) {
			af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;
			af_data[0] = *(af_src_dat + 1);
			af_data[1] = (*(af_src_dat) & 0xf0) >> 4;
			af_data[2] = *(af_src_dat + 2);
			af_data[3] = *(af_src_dat) & 0x0f;
		} else
			OTP_LOGE("af data checksum error, parse failed!");
	} else if ((af_flag & 0x03) == 0x01) {
		OTP_LOGE("af group 2 is valid!");
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET + AF_INFO_SIZE,
					AF_INFO_SIZE - 1, AF_INFO_CHECKSUM + AF_INFO_SIZE);
		if(!ret){
			af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET + AF_INFO_SIZE;
			af_data[0] = *(af_src_dat + 1);
			af_data[1] = (*(af_src_dat) & 0xf0) >> 4;
			af_data[2] = *(af_src_dat + 2);
			af_data[3] = *(af_src_dat) & 0x0f;
		} else
			OTP_LOGE("af data checksum error, parse failed!");
	}

	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("auto focus parse error, parse failed!");
		return ret;
	} else {
		memcpy(af_src_dat, af_data, AF_INFO_SIZE);
		af_cali_dat->rdm_info.buffer = af_src_dat;
		af_cali_dat->rdm_info.size = AF_INFO_CHECKSUM - AF_INFO_OFFSET;
		af_cali_dat->gld_info.buffer = NULL;
		af_cali_dat->gld_info.size = 0;
	}
*/
	OTP_LOGI("out");
	return ret;
}

/*==============================================================================
 * Description:
 * parse otp data of awb
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_parse_awb_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	cmr_uint data_count_rdm = 0;
	cmr_uint data_count_gld = 0;
	cmr_uint rg_typicl,bg_typicl = 0;

	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/* awb data */
	otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
	cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;
	cmr_u8 *awb_golden_dat = otp_cxt->otp_raw_data.buffer + AWB_GOLDEN_INFO_OFFSET;
	cmr_u8 awb_flag = *(otp_cxt->otp_raw_data.buffer + AWB_INFO_GROUP_FLAG_OFFSET);

	/* wb data */
	if ((awb_flag & 0x0c) == 0x04) {
		OTP_LOGI("wb group 1 is valid!");
		group_index = 1;
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
						AWB_INFO_SIZE - 1, AWB_INFO_CHECKSUM);
		if (!ret) {
			awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;
			awb_cali_dat->rdm_info.buffer = awb_src_dat;
			awb_cali_dat->rdm_info.size = AWB_INFO_SIZE;
		} else {
			OTP_LOGE("WB data checksum error, parse failed!");
			ret = OTP_CAMERA_SUCCESS;
			}
	} else if ((awb_flag & 0x03) == 0x01) {
		OTP_LOGI("wb group 2 is valid!");
		group_index = 2;
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET + AWB_INFO_SIZE,
						AWB_INFO_SIZE - 1, AWB_INFO_CHECKSUM + AWB_INFO_SIZE);
		if (!ret) {
			awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET + AWB_INFO_SIZE;
			awb_cali_dat->rdm_info.buffer = awb_src_dat;
			awb_cali_dat->rdm_info.size = AWB_INFO_SIZE;
		} else {
			OTP_LOGE("WB data checksum error, parse failed!");
			ret = OTP_CAMERA_FAIL;
			}
	} else if (awb_flag & 0x0f == 0x00) {
		OTP_LOGE("wb info is empty!");
		awb_cali_dat->rdm_info.buffer = NULL;
		awb_cali_dat->rdm_info.size = 0;
		ret = OTP_CAMERA_FAIL;
	} else {
		OTP_LOGE("wb info is invalid!");
		awb_cali_dat->rdm_info.buffer = NULL;
		awb_cali_dat->rdm_info.size = 0;
		ret = OTP_CAMERA_FAIL;
	}

	/* golden data */
	if ((awb_flag & 0xc0) == 0x40) {
		OTP_LOGI("golden group 1 is valid!");
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, AWB_GOLDEN_INFO_OFFSET,
						AWB_GOLDEN_INFO_SIZE - 1, AWB_GOLDEN_INFO_CHECKSUM);
		if (!ret) {
			awb_golden_dat = otp_cxt->otp_raw_data.buffer + AWB_GOLDEN_INFO_OFFSET;
			rg_typicl = *(awb_golden_dat) + ((*(awb_golden_dat + 1) & 0xf0) << 4);
			gc5035_common_awb[0].rg_ratio = rg_typicl > 0 ? rg_typicl : gc5035_common_awb[0].rg_ratio;
			bg_typicl = *(awb_golden_dat + 2) + ((*(awb_golden_dat + 1) & 0x0f) << 8);
			gc5035_common_awb[0].bg_ratio = bg_typicl > 0 ? bg_typicl : gc5035_common_awb[0].bg_ratio;
		} else
			OTP_LOGE("golden data checksum error, parse failed!");
	} else if ((awb_flag & 0x30) == 0x10) {
		OTP_LOGI("golden group 2 is valid!");
		ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, AWB_GOLDEN_INFO_OFFSET + AWB_GOLDEN_INFO_SIZE,
						AWB_GOLDEN_INFO_SIZE - 1, AWB_GOLDEN_INFO_CHECKSUM + AWB_GOLDEN_INFO_SIZE);
		if (!ret) {
			awb_golden_dat = otp_cxt->otp_raw_data.buffer + AWB_GOLDEN_INFO_OFFSET + AWB_GOLDEN_INFO_SIZE;
			rg_typicl = *(awb_golden_dat) + ((*(awb_golden_dat + 1) & 0xf0) << 4);
			gc5035_common_awb[0].rg_ratio = rg_typicl > 0 ? rg_typicl : gc5035_common_awb[0].rg_ratio;
			bg_typicl = *(awb_golden_dat + 2) + ((*(awb_golden_dat + 1) & 0x0f) << 8);
			gc5035_common_awb[0].bg_ratio = bg_typicl > 0 ? bg_typicl : gc5035_common_awb[0].bg_ratio;
		} else
			OTP_LOGE("golden data checksum error, parse failed!");
	} else if (awb_flag & 0xf0 == 0x00) {
		OTP_LOGE("golden info is empty!");
	} else {
		OTP_LOGE("golden info is invalid!");
	}

	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("awb otp data parse error, parse failed");
		return ret;
	} else {
		data_count_rdm = AWB_SECTION_NUM * AWB_INFO_SIZE;
		data_count_gld = AWB_SECTION_NUM * (sizeof(awb_target_packet_t));
		awb_cali_dat->rdm_info.buffer = awb_src_dat;
		awb_cali_dat->rdm_info.size = data_count_rdm;
		awb_cali_dat->gld_info.buffer = gc5035_common_awb;
		awb_cali_dat->gld_info.size = data_count_gld;
	}

	OTP_LOGI("out");
	return ret;
}

/*==============================================================================
 * Description:
 * parse otp data of lsc
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_parse_lsc_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/* lsc and opt data */

	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * parse otp data of pdaf
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_parse_pdaf_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/*pdaf data*/
	otp_section_info_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);
	cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;

	ret = gc5035_common_section_checksum(otp_cxt->otp_raw_data.buffer, PDAF_INFO_OFFSET,
								PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET, PDAF_INFO_CHECKSUM);

	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
		return ret;
	} else {
		pdaf_cali_dat->rdm_info.buffer = pdaf_src_dat;
		pdaf_cali_dat->rdm_info.size = PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET;
		pdaf_cali_dat->gld_info.buffer = NULL;
		pdaf_cali_dat->gld_info.size = 0;
	}

	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * awb calibration
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_awb_calibration(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_PTR(otp_drv_handle);

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);

	/* TODO */
	cmr_u8 *awb_info_rdm = (cmr_u8 *)(awb_cali_dat->rdm_info.buffer);
	cmr_u16 rg_ratio_typical = gc5035_common_awb[0].rg_ratio;
	cmr_u16 bg_ratio_typical = gc5035_common_awb[0].bg_ratio;
	if(awb_cali_dat->rdm_info.buffer==NULL)
	{
		OTP_LOGI("rdm_info NULL, awb calibration fail!!");
		return OTP_CAMERA_FAIL;
	}
	cmr_u16 rg_ratio_current = (*(awb_info_rdm)) | ((*(awb_info_rdm + 1) & 0xf0) << 4);
	cmr_u16 bg_ratio_current = ((*(awb_info_rdm + 1) & 0x0f) << 8) | (*(awb_info_rdm + 2));
	cmr_u16 r_gain_current = 0, g_gain_current = 0, b_gain_current = 0, base_gain = 0;
	cmr_u16 r_gain = 1024, g_gain = 1024, b_gain = 1024;

	rg_ratio_typical = rg_ratio_typical > 0 ? rg_ratio_typical : 0x400;
	bg_ratio_typical = bg_ratio_typical > 0 ? bg_ratio_typical : 0x400;

	rg_ratio_current = rg_ratio_current > 0 ? rg_ratio_current : 0x400;
	bg_ratio_current = bg_ratio_current > 0 ? bg_ratio_current : 0x400;
	OTP_LOGI("rg_ratio_typical = 0x%x, bg_ratio_typical = 0x%x\n", rg_ratio_typical, bg_ratio_typical);
	OTP_LOGI("rg_ratio_current = 0x%x, bg_ratio_current = 0x%x\n", rg_ratio_current, bg_ratio_current);

	r_gain_current = 0x400 * rg_ratio_typical / rg_ratio_current;
	b_gain_current = 0x400 * bg_ratio_typical / bg_ratio_current;
	g_gain_current = 0x400;

	base_gain = r_gain_current < b_gain_current ? r_gain_current : b_gain_current;
	base_gain = base_gain < g_gain_current ? base_gain : g_gain_current;
	OTP_LOGI("r_gain_current = 0x%x, b_gain_current = 0x%x, base_gain = 0x%x\n",
		r_gain_current, b_gain_current, base_gain);

	r_gain = 0x400 * r_gain_current / base_gain;
	g_gain = 0x400 * g_gain_current / base_gain;
	b_gain = 0x400 * b_gain_current / base_gain;
	OTP_LOGI("r_gain = 0x%x, g_gain = 0x%x, b_gain = 0x%x\n", r_gain, g_gain, b_gain);

	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x04);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x40, g_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x41, r_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x42, b_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x43, g_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x44, g_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x45, r_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x46, b_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x47, g_gain & 0xff);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x48, (g_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x49, (r_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x4a, (b_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x4b, (g_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x4c, (g_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x4d, (r_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x4e, (b_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0x4f, (g_gain >> 8) & 0x07);
	gc5035_common_i2c_write(otp_drv_handle, SENSOR_I2C_ADDR, 0xfe, 0x00);

	OTP_LOGI("out");
	return ret;
}

/*==============================================================================
 * Description:
 * lsc calibration
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_lsc_calibration(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_PTR(otp_drv_handle);
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
	otp_section_info_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);

	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * pdaf calibration
 * please modify this function acording your spec
 *============================================================================*/
static int gc5035_common_pdaf_calibration(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_PTR(otp_drv_handle);

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_section_info_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);

	/*TODO*/

	/*END*/

	OTP_LOGI("out");
	return ret;
}

/*==============================================================================
 * Description:
 * compatible convert, use for otp calibration through ISP
 * convert to format isp can recognize
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_compatible_convert(cmr_handle otp_drv_handle,void *p_data) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_format_data_t *format_data = otp_cxt->otp_data;
	SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
	struct sensor_single_otp_info *single_otp = NULL;

	struct sensor_otp_cust_info *convert_data = (struct sensor_otp_cust_info *)malloc(
								sizeof(struct sensor_otp_cust_info));
	if (!convert_data) {
		OTP_LOGI("malloc convert data buffer failed");
		return OTP_CAMERA_FAIL;
	}

	cmr_bzero(convert_data, sizeof(*convert_data));
	single_otp = &convert_data->single_otp;

	/* otp vendor type */
	convert_data->otp_vendor = OTP_VENDOR_SINGLE;

	/* otp raw data */
	convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
	convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;

	/* module data */
	single_otp->module_info = (struct sensor_otp_section_info *)&format_data->module_dat;

	/* awb convert */
	if(gc5035_common_drv_entry.otp_cfg.cali_items.is_awbc){
		if(gc5035_common_drv_entry.otp_cfg.cali_items.is_awbc_self_cal){
	
		}else{
			single_otp->iso_awb_info = (struct sensor_otp_section_info *)&format_data->awb_cali_dat;
		}
	}

	/* optical center convert */
	if(gc5035_common_drv_entry.otp_cfg.cali_items.is_self_cal){
		
	}else{
		single_otp->optical_center_info = (struct sensor_otp_section_info *)&format_data->opt_center_dat;
	}

	/* lsc convert */
	if(gc5035_common_drv_entry.otp_cfg.cali_items.is_lsc){		
		if(gc5035_common_drv_entry.otp_cfg.cali_items.is_lsc_self_cal){
	
		}else{
			single_otp->lsc_info = (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
		}
	}

	/* af convert */
	if(gc5035_common_drv_entry.otp_cfg.cali_items.is_afc){
		if(gc5035_common_drv_entry.otp_cfg.cali_items.is_af_self_cal){
	
		}else{
			single_otp->af_info = (struct sensor_otp_section_info *)&format_data->af_cali_dat;
		}
	}

	/* pdaf convert */
	if(gc5035_common_drv_entry.otp_cfg.cali_items.is_pdafc){
		if(gc5035_common_drv_entry.otp_cfg.cali_items.is_pdaf_self_cal){
	
		}else{
			single_otp->pdaf_info = (struct sensor_otp_section_info *)&format_data->pdaf_cali_dat;
		}
	}

	otp_cxt->compat_convert_data = convert_data;
	p_val->pval = convert_data;
	p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;

	OTP_LOGI("out");
	return ret;
}

/*==================================================
*                  External interface
====================================================*/


/*==============================================================================
 * Description:
 * create otp handle
 * please don't modify this function
 *============================================================================*/
static cmr_int gc5035_common_otp_create(otp_drv_init_para_t *input_para, cmr_handle *sns_af_drv_handle){
	OTP_LOGI("in");
	return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}


/*==============================================================================
 * Description:
 * create otp handle
 * please don't modify this function
 *============================================================================*/
static cmr_int gc5035_common_otp_drv_delete(cmr_handle otp_drv_handle) {
	OTP_LOGI("in");
	return sensor_otp_drv_delete(otp_drv_handle);
}


/*==============================================================================
 * Description:
 * read otp data from sensor to buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_otp_drv_read(cmr_handle otp_drv_handle, void *p_params)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	char value[255];
	OTP_LOGI("in");

	cmr_u8 *buffer = NULL;
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
	otp_params_t *p_data = (otp_params_t *)p_params;

	if (!otp_raw_data->buffer) {
		/*when mobile power on , it will init*/
		otp_raw_data->buffer = sensor_otp_get_raw_buffer(OTP_LEN, otp_cxt->sensor_id);
		if (NULL == otp_raw_data->buffer) {
			OTP_LOGE("malloc otp raw buffer failed\n");
			ret = OTP_CAMERA_FAIL;
			goto exit;
		} 
		otp_raw_data->num_bytes = OTP_LEN;
		gc5035_common_buffer_init(otp_drv_handle);
	}

	if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
		OTP_LOGI("otp raw data has read before,return directly");
		if (p_data) {
			p_data->buffer = otp_raw_data->buffer;
			p_data->num_bytes = otp_raw_data->num_bytes;
		}
		goto exit;
	} else {
		/* start read otp data one time */
		/* TODO */
		gc5035_common_otp_enable(otp_drv_handle, otp_open);
		gc5035_common_read_otp_group(otp_drv_handle, OTP_START_ADDR, otp_raw_data->buffer, OTP_LEN);
		gc5035_common_otp_enable(otp_drv_handle, otp_close);
		/* END */
		for(int i = 0; i < OTP_LEN; i++)
		{
			OTP_LOGI("otp address, value: p1:[0x%x,0x%x]",OTP_START_ADDR+i*8,otp_raw_data->buffer[i]);
		}
		/*
		if (OTP_CAMERA_SUCCESS == ret) {
			property_get("debug.camera.save.otp.raw.data", value, "0");
			if (atoi(value) == 1) {
				if (sensor_otp_dump_raw_data(otp_raw_data->buffer, OTP_LEN,
											otp_cxt->dev_name))
					OTP_LOGE("dump failed");
			}
		}*/
	}
exit:
    OTP_LOGI("out");
    return ret;
}


/*==============================================================================
 * Description:
 * write otp data
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_otp_drv_write(cmr_handle otp_drv_handle,void *p_params) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	CHECK_PTR(p_params);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_params_t *otp_write_data = p_params;

	if (NULL != otp_write_data->buffer) {
		OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d", otp_cxt->dev_name,
				(unsigned int)otp_write_data->buffer, otp_write_data->num_bytes);

		/*TODO*/

		/*END*/
	} else {
		OTP_LOGE("ERROR:buffer pointer is null");
		ret = OTP_CAMERA_FAIL;
	}
	OTP_LOGI("out");
	return ret;
}

/*==============================================================================
 * Description:
 * read otp to buffer  and parse otp data in buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_otp_drv_parse(cmr_handle otp_drv_handle,void *p_params) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_base_info_cfg_t *base_info = &(gc5035_common_drv_entry.otp_cfg.base_info_cfg);
	otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

	if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) 
	{
		OTP_LOGI("otp has parse before,return directly");
		return ret;
	} else if (otp_raw_data->buffer) {
		/* begain read raw data, save module info */
		OTP_LOGI("drver has read otp raw data, start parsed.");
		ret = gc5035_common_parse_module_data(otp_drv_handle);
		if(ret != OTP_CAMERA_SUCCESS)
			return ret;
		//gc5035_common_parse_lsc_data(otp_drv_handle);
		ret = gc5035_common_parse_awb_data(otp_drv_handle);
		if(ret != OTP_CAMERA_SUCCESS)
			return ret;
		//gc5035_common_parse_af_data(otp_drv_handle);
	
		/* decompress lsc data if needed */
		if ((base_info->compress_flag != GAIN_ORIGIN_BITS) && base_info->is_lsc_drv_decompression == TRUE) 
		{
			ret = sensor_otp_lsc_decompress(
				&gc5035_common_drv_entry.otp_cfg.base_info_cfg,
				&otp_cxt->otp_data->lsc_cali_dat);
			if (ret != OTP_CAMERA_SUCCESS) {
				return OTP_CAMERA_FAIL;
			}
		}
		sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1); /*read to memory*/
	}else{
		OTP_LOGE("should read otp before parse");
		return OTP_CAMERA_FAIL;
	}

	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * otp drv calibration
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_otp_drv_calibration(cmr_handle otp_drv_handle) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

	otp_calib_items_t *cali_items = &(gc5035_common_drv_entry.otp_cfg.cali_items);

	if (cali_items) {
		if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal) {
			gc5035_common_pdaf_calibration(otp_drv_handle);
		}
		/* calibration at sensor or isp */
		if (cali_items->is_awbc && cali_items->is_awbc_self_cal) {
			gc5035_common_awb_calibration(otp_drv_handle);
		}
		if (cali_items->is_lsc && cali_items->is_lsc_self_cal) {
			gc5035_common_lsc_calibration(otp_drv_handle);
		}
	}
	/* If there are other items that need calibration, please add to here */

	OTP_LOGI("out");
	return ret;
}
/*
static cmr_int gc5035_common_get_module_otp_info(cmr_handle otp_drv_handle,void *p_data) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	SPROCOMM_INFO *data = (SPROCOMM_INFO *)p_data;
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

	if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
		data->module_id_info = 0x02;    //unimage
		data->otp_group_info = group_index;
		OTP_LOGI("==sprocomm add ,gc5035_common_get_module_otp_info:%d,%d\n",data->module_id_info,data->otp_group_info);
		return ret;
	}
	else
		return CMR_CAMERA_FAIL;
}*/

/*just for expend*/
/*==============================================================================
 * Description:
 * reserve interface: just for expending
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_common_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd, void *params) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");

	/* you can add you command */
	switch (cmd) {
	case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
		gc5035_common_compatible_convert(otp_drv_handle, params);
		break;
	/*case CMD_SNS_GET_MODULE_OTP_INFO:
		ret =gc5035_common_get_module_otp_info(otp_drv_handle, params);
		break;*/
	default:
		break;
	}
	OTP_LOGI("out");
	return ret;
}

void *otp_driver_open_lib(void)
{
     return &gc5035_common_drv_entry;
}
