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

#include "s5k4h7_tsp_otp_drv.h"

/*jiangguijuan add,20180419*/
static cmr_u8	group_index = 0;
/*jiangguijuan add,20180419*/

/*==============================================================================
 * Description:
 * read register value from sensor
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_i2c_read(void *otp_drv_handle, uint16_t slave_addr, uint16_t memory_addr, uint8_t *memory_data)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	
	uint8_t cmd_val[5] = { 0x00 };
	uint16_t cmd_len = 0;	
	
	cmd_val[0] = memory_addr>>8;
	cmd_val[1] = memory_addr&0xff;
	cmd_len = 2;
	
	ret = hw_Sensor_ReadI2C(otp_cxt->hw_handle,slave_addr,(uint8_t*)&cmd_val[0], cmd_len<<16);
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
static cmr_int s5k4h7_tsp_i2c_write(void *otp_drv_handle, uint16_t slave_addr, uint16_t memory_addr, uint16_t memory_data)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	
	uint8_t cmd_val[5] = { 0x00 };
	uint16_t cmd_len = 0;
	
	cmd_val[0] = memory_addr>>8;
	cmd_val[1] = memory_addr&0xff;
	cmd_val[2] = memory_data;
	cmd_len = 3;
	ret = hw_Sensor_WriteI2C(otp_cxt->hw_handle,slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
	
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
static cmr_int s5k4h7_tsp_section_checksum(unsigned char *buf, unsigned int offset, unsigned int data_count, unsigned int check_sum_offset) 
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
		OTP_LOGE("offset:0x%x, sum:%d, checksum:0x%x", OTP_START_ADDR+check_sum_offset, sum,
				buf[check_sum_offset]);
		ret = OTP_CAMERA_FAIL;
	}
	
	OTP_LOGI("out");
	return ret;
}

/*==============================================================================
 * Description:
 * inital otp data buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_buffer_init(cmr_handle otp_drv_handle) {
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
static cmr_int s5k4h7_tsp_parse_module_data(cmr_handle otp_drv_handle) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/*module info data*/
	otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
	cmr_u8 *module_src_dat = otp_cxt->otp_raw_data.buffer + MODULE_INFO_GROUP_FLAG_OFFSET;
	cmr_u8 module_flag = *(otp_cxt->otp_raw_data.buffer + MODULE_INFO_GROUP_FLAG_OFFSET);
	
	if((module_flag&0xC0) == 0x40){
		OTP_LOGI("group 1 valid");
		ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, MODULE_INFO_START_OFFSET,
						MODULE_INFO_SIZE, MODULE_INFO_CHECKSUM_OFFSET);
		if(!ret){
			module_src_dat=otp_cxt->otp_raw_data.buffer;
			OTP_LOGI("module id = 0x%x", *(module_src_dat + 1));
			OTP_LOGI("lens id = 0x%x", *(module_src_dat + 2));
			OTP_LOGI("year = %d", *(module_src_dat + 4));
			OTP_LOGI("month = %d", *(module_src_dat + 5));
			OTP_LOGI("day = %d", *(module_src_dat + 6));
		}else
			OTP_LOGE("module data checksum error,parse failed");
	}else if((module_flag&0x30) == 0x10){
		OTP_LOGI("group 2 valid");
		ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, MODULE_INFO_START_OFFSET+32,
						MODULE_INFO_SIZE, MODULE_INFO_CHECKSUM_OFFSET+32);
		if(!ret){
			module_src_dat=otp_cxt->otp_raw_data.buffer + 32;
			OTP_LOGI("module id = 0x%x", *(module_src_dat + 1));
			OTP_LOGI("lens id = 0x%x", *(module_src_dat + 2));
			OTP_LOGI("year = %d", *(module_src_dat + 4));
			OTP_LOGI("month = %d", *(module_src_dat + 5));
			OTP_LOGI("day = %d", *(module_src_dat + 6));
		}else
			OTP_LOGE("module data checksum error,parse failed");
	}else{
		OTP_LOGE("otp all value is Invalid");
		ret = OTP_CAMERA_FAIL;
	}
	
	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("module data parse error,parse failed");
		module_dat->rdm_info.buffer = NULL;
		module_dat->rdm_info.size = 0;
		return ret;
	} else {
		module_dat->rdm_info.buffer = module_src_dat;
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
static cmr_int s5k4h7_tsp_parse_af_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/*af data*/
	otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
	cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_GROUP_FLAG_OFFSET;
	cmr_u8 af_data[4] = {0x00, 0x00, 0x00, 0x00};
	
	if((*(af_src_dat)) == 0x40){
		OTP_LOGI("group 1 valid");
		ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, AF_INFO_START_OFFSET,
					AF_INFO_SIZE, AF_INFO_CHECKSUM_OFFSET);
		if(!ret){
			af_data[0] = *(af_src_dat + 3);
			af_data[1] = *(af_src_dat + 2);
			af_data[2] = *(af_src_dat + 5);
			af_data[3] = *(af_src_dat + 4);
		}else
			OTP_LOGE("af data checksum error,parse failed");
	}else if((*(af_src_dat)) == 0xd0){
		OTP_LOGI("group 2 valid");
		ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, AF_INFO_START_OFFSET + 25,
					AF_INFO_SIZE, AF_INFO_CHECKSUM_OFFSET + 25);
		if(!ret){
			af_data[0] = *(af_src_dat + 3);
			af_data[1] = *(af_src_dat + 2);
			af_data[2] = *(af_src_dat + 5);
			af_data[3] = *(af_src_dat + 4);
		}else
			OTP_LOGE("af data checksum error,parse failed");
	}
	
	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("auto focus parse error,parse failed");
		return ret;
	} else {
		memcpy(af_src_dat, af_data, 4);
		af_cali_dat->rdm_info.buffer = af_src_dat;
		af_cali_dat->rdm_info.size = 4;
		af_cali_dat->gld_info.buffer = NULL;
		af_cali_dat->gld_info.size = 0;
	}
	
	OTP_LOGI("out");
	return ret;
}



/*==============================================================================
 * Description:
 * parse otp data of awb
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_parse_awb_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	cmr_uint data_count_rdm = 0;
	cmr_uint data_count_gld = 0;
	
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/*awb data*/
	otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
	cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer+ AWB_INFO_GROUP_FLAG_OFFSET;
	cmr_u8 *awb_golden_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_GROUP_FLAG_OFFSET;
	cmr_u8 awb_flag = *(otp_cxt->otp_raw_data.buffer + AWB_INFO_GROUP_FLAG_OFFSET);
	
	if((awb_flag&0xC0) == 0x40){
		OTP_LOGI("group 1 valid");
		group_index =  0x1;
		ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, AWB_INFO_START_OFFSET,
						AWB_INFO_SIZE, AWB_INFO_CHECKSUM_OFFSET);
		if(!ret){
			awb_src_dat = otp_cxt->otp_raw_data.buffer + 7;
			//read golden awb data from otp
			/*awb_golden_dat = otp_cxt->otp_raw_data.buffer + 17;
			tsp_awb[0].rg_ratio = (*(awb_golden_dat	+ 1)) | (*(awb_golden_dat) << 8);
			tsp_awb[0].bg_ratio = (*(awb_golden_dat + 3)) | (*(awb_golden_dat + 2) << 8);*/
		}else
			OTP_LOGE("awb data checksum error,parse failed");
	}else if((awb_flag&0x30) == 0x10){
		OTP_LOGI("group 2 valid");
		group_index =  0x2;
		ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, AWB_INFO_START_OFFSET+32,
						AWB_INFO_SIZE, AWB_INFO_CHECKSUM_OFFSET+32);
		if(!ret){
			awb_src_dat = otp_cxt->otp_raw_data.buffer + 39;
			//read golden awb data from otp
			/*awb_golden_dat = otp_cxt->otp_raw_data.buffer + 42;
			tsp_awb[0].rg_ratio = (*(awb_golden_dat	+ 1)) | (*(awb_golden_dat) << 8);
			tsp_awb[0].bg_ratio = (*(awb_golden_dat + 3)) | (*(awb_golden_dat + 2) << 8);*/
		}else
			OTP_LOGE("awb data checksum error,parse failed");
	}else{
		OTP_LOGE("otp all value is Invalid");
		ret = OTP_CAMERA_FAIL;
	}
	
		data_count_rdm = AWB_SECTION_NUM * AWB_DATA_SIZE;
		data_count_gld = AWB_SECTION_NUM * (sizeof(awb_target_packet_t));
		awb_cali_dat->gld_info.buffer = tsp_awb;
		awb_cali_dat->gld_info.size = data_count_gld;
	
	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGE("awb otp data parse error,parse failed");
		awb_cali_dat->rdm_info.buffer = NULL;
		awb_cali_dat->rdm_info.size = 0;
		return ret;
	} else {
		awb_cali_dat->rdm_info.buffer = awb_src_dat;
		awb_cali_dat->rdm_info.size = data_count_rdm;
	}
	
	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * parse otp data of lsc
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_parse_lsc_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/*lsc and opt data*/
	otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
	otp_section_info_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
	
	ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
								LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
	if (OTP_CAMERA_SUCCESS != ret) {
		OTP_LOGI("lsc otp data checksum error,parse failed.\n");
	} else {
		/*optical center data*/
		cmr_u8 *opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
		opt_dst->rdm_info.buffer = opt_src;
		opt_dst->rdm_info.size = LSC_INFO_OFFSET - OPTICAL_INFO_OFFSET;
		opt_dst->gld_info.buffer = NULL;
		opt_dst->gld_info.size = 0;
	
		/*lsc data*/
		cmr_u8 *rdm_dst = otp_cxt->otp_raw_data.buffer + LSC_INFO_OFFSET;
		lsc_dst->rdm_info.buffer = rdm_dst;
		lsc_dst->rdm_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
		lsc_dst->gld_info.buffer = tsp_lsc;
		lsc_dst->gld_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
	}
	
	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * parse otp data of pdaf
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_parse_pdaf_data(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	/*pdaf data*/
	otp_section_info_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);
	cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;
	
	ret = s5k4h7_tsp_section_checksum(otp_cxt->otp_raw_data.buffer, PDAF_INFO_OFFSET,
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
static cmr_int s5k4h7_tsp_awb_calibration(cmr_handle otp_drv_handle){
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_PTR(otp_drv_handle); 
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
	
	if((!awb_cali_dat->rdm_info.size)||(!awb_cali_dat->gld_info.size)){
		OTP_LOGE("awb data null");
		return OTP_CAMERA_FAIL;
	}
	
	/*TODO*/
	cmr_u8 *awb_info_rdm = (cmr_u8 *)(awb_cali_dat->rdm_info.buffer);
	cmr_u16 *awb_info_gld = (cmr_u16 *)(awb_cali_dat->gld_info.buffer);
	
	cmr_u16 rg_ratio_typical = (*(awb_info_gld+3));
	cmr_u16 bg_ratio_typical = (*(awb_info_gld+4));
	
	OTP_LOGI("rg_typical= 0x%x, bg_typical= 0x%x\n",rg_ratio_typical,bg_ratio_typical);
	
	cmr_u16 rg_ratio_current = ((*(awb_info_rdm) )<<8) | (*(awb_info_rdm + 1));
	cmr_u16 bg_ratio_current = ((*(awb_info_rdm+2))<<8) | (*(awb_info_rdm + 3));
	
	OTP_LOGI("rg_current= 0x%x, bg_current= 0x%x\n",rg_ratio_current,bg_ratio_current);
	
	cmr_u32 g_gain = 0,r_gain = 0, b_gain = 0,g_gain_b = 0, g_gain_r =0;
	
	if((!rg_ratio_current)||(!bg_ratio_current)){
		g_gain=r_gain=b_gain=0x100;
	}
	else{
	
	//calculate R,G,B gain
	r_gain =  rg_ratio_typical*0x100/rg_ratio_current;
	b_gain =  bg_ratio_current*0x100/bg_ratio_current;
	g_gain = 0x0100;
	
		if (r_gain < b_gain) {
			if (r_gain < 0x0100) {
				b_gain = (0x0100 * b_gain) / r_gain;
				g_gain = (0x0100 * g_gain) / r_gain;
				r_gain = 0x0100;
			}
		} else {
			if (b_gain < 0x0100) {
				r_gain = (0x0100 * r_gain) / b_gain;
				g_gain = (0x0100 * g_gain) / b_gain;
				b_gain = 0x0100;
			}
		}
	}
	
	//write to register
	OTP_LOGI("r_Gain=0x%x\n", r_gain);	
	OTP_LOGI("g_Gain=0x%x\n", g_gain);	
	OTP_LOGI("b_Gain=0x%x\n", b_gain);	
	
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x3C0F, 0x00);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x020E, g_gain >> 8);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x020F, g_gain & 0xff);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x0210, r_gain >> 8);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x0211, r_gain & 0xff);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x0212, b_gain >> 8);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x0213, b_gain & 0xff);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x0214, g_gain >> 8);
	hw_sensor_write_reg(otp_cxt->hw_handle, 0x0215, g_gain & 0xff);
	/*END*/
	
	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * lsc calibration
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_lsc_calibration(cmr_handle otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_PTR(otp_drv_handle);
	
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
	otp_section_info_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
	
	/*TODO*/
	
	/*END*/
	
	OTP_LOGI("out");
	return ret;
}


/*==============================================================================
 * Description:
 * pdaf calibration
 * please modify this function acording your spec
 *============================================================================*/
static int s5k4h7_tsp_pdaf_calibration(cmr_handle otp_drv_handle) {
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
static cmr_int s5k4h7_tsp_compatible_convert(cmr_handle otp_drv_handle,void *p_data) 
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
	
	/*otp vendor type*/
	convert_data->otp_vendor = OTP_VENDOR_SINGLE;
	
	/*otp raw data*/
	convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
	convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;
	
	/*module data*/
	single_otp->module_info = (struct sensor_otp_section_info *)&format_data->module_dat;
	
	/*awb convert*/
	if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_awbc){
		if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_awbc_self_cal){
			
		}else{
			single_otp->iso_awb_info = (struct sensor_otp_section_info *)&format_data->awb_cali_dat;
		}
	}
	
	/*optical center convert*/
	if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_self_cal){
		
	}else{
		single_otp->optical_center_info = (struct sensor_otp_section_info *)&format_data->opt_center_dat;
	}
	
	/*lsc convert*/
	if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_lsc){		
		if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_lsc_self_cal){
	
		}else{
			single_otp->lsc_info = (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
		}
	}
	
	/*af convert*/
	if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_afc){
		if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_af_self_cal){
			
		}else{
			single_otp->af_info = (struct sensor_otp_section_info *)&format_data->af_cali_dat;
		}
	}
	
	/*pdaf convert*/
	if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_pdafc){
		if(s5k4h7_tsp_drv_entry.otp_cfg.cali_items.is_pdaf_self_cal){
			
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

/*jiangguijuan add,20180419*/
/*static cmr_int s5k4h7_tsp_get_module_otp_info(cmr_handle otp_drv_handle,void *p_data) {
	OTP_LOGI("in");
	cmr_int ret = OTP_CAMERA_SUCCESS;
	SPROCOMM_INFO *data = (SPROCOMM_INFO *)p_data;
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	
	if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
		data->module_id_info = 0x05;	//	5:tianshi
		data->otp_group_info = group_index;
		OTP_LOGI("==sprocomm add ,s5k4h7_tsp_get_module_otp_info==%d,%d\n",data->module_id_info,data->otp_group_info);
		return ret;
	
	}
	else
	return CMR_CAMERA_FAIL;
}*/
/*jiangguijuan add,20180419*/

/*==================================================
*                  External interface
====================================================*/


/*==============================================================================
 * Description:
 * create otp handle
 * please don't modify this function
 *============================================================================*/
static cmr_int s5k4h7_tsp_otp_create(otp_drv_init_para_t *input_para, cmr_handle *sns_af_drv_handle){
	OTP_LOGI("in");
	return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}


/*==============================================================================
 * Description:
 * create otp handle
 * please don't modify this function
 *============================================================================*/
static cmr_int s5k4h7_tsp_otp_drv_delete(cmr_handle otp_drv_handle) {
	
	OTP_LOGI("in");
	return sensor_otp_drv_delete(otp_drv_handle);
}


/*==============================================================================
 * Description:
 * read otp data from sensor to buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_otp_drv_read(cmr_handle otp_drv_handle, void *p_params)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	cmr_int i = 0;
	cmr_int p = 0;
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
		s5k4h7_tsp_buffer_init(otp_drv_handle);
	}
	
	if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
		OTP_LOGI("otp raw data has read before,return directly");
		if (p_data) {
			p_data->buffer = otp_raw_data->buffer;
			p_data->num_bytes = otp_raw_data->num_bytes;
		}
		goto exit;
	} else {
	
		/*start read otp data one time*/
		/*TODO*/
		/*stream on*/
		hw_sensor_write_reg(otp_cxt->hw_handle, 0x0100, 0x01);
		usleep(50000U);
		/*page set*/
		hw_sensor_write_reg(otp_cxt->hw_handle, 0x0A02, 0x15);
		/*otp enable read*/
		hw_sensor_write_reg(otp_cxt->hw_handle, 0x0A00, 0x01);
		usleep(55000U);
		
		for(i = 0; i < OTP_LEN; i++)
		{
			*(otp_raw_data->buffer + i) = hw_sensor_read_reg(otp_cxt->hw_handle, OTP_START_ADDR + i);
			OTP_LOGI("address ,value = [0x%lx, 0x%x]\n",OTP_START_ADDR + i,otp_raw_data->buffer[i]);
		}
	
		hw_sensor_write_reg(otp_cxt->hw_handle, 0x0A00, 0x00);
		usleep(5000U);		
	/*
		if (OTP_CAMERA_SUCCESS == ret) {
			property_get("debug.camera.save.otp.raw.data", value, "0");
			if (atoi(value) == 1) {
				if (sensor_otp_dump_raw_data(otp_raw_data->buffer, OTP_LEN,
											otp_cxt->dev_name))
					OTP_LOGE("dump failed");
			}
		}
	*/
		/*END*/
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
static cmr_int s5k4h7_tsp_otp_drv_write(cmr_handle otp_drv_handle,void *p_params) 
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
static cmr_int s5k4h7_tsp_otp_drv_parse(cmr_handle otp_drv_handle,void *p_params) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	otp_base_info_cfg_t *base_info = &(s5k4h7_tsp_drv_entry.otp_cfg.base_info_cfg);
	otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
	
	if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) 
	{
		OTP_LOGI("otp has parse before,return directly");
		return ret;
	} else if(otp_raw_data->buffer){
		/*begain read raw data, save module info */
		OTP_LOGI("drver has read otp raw data,start parsed.");
		s5k4h7_tsp_parse_awb_data(otp_drv_handle);
		s5k4h7_tsp_parse_module_data(otp_drv_handle);
		//s5k4h7_tsp_parse_lsc_data(otp_drv_handle);
		//s5k4h7_tsp_parse_af_data(otp_drv_handle);
		
		/*decompress lsc data if needed*/
		if ((base_info->compress_flag != GAIN_ORIGIN_BITS) && base_info->is_lsc_drv_decompression == TRUE) 
		{
			ret = sensor_otp_lsc_decompress(
				&s5k4h7_tsp_drv_entry.otp_cfg.base_info_cfg,
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
static cmr_int s5k4h7_tsp_otp_drv_calibration(cmr_handle otp_drv_handle) 
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
	
	otp_calib_items_t *cali_items = &(s5k4h7_tsp_drv_entry.otp_cfg.cali_items);
	
	if(cali_items){
		if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal){
			//s5k4h7_tsp_pdaf_calibration(otp_drv_handle);
		}
		/*calibration at sensor or isp */
		if (cali_items->is_awbc && cali_items->is_awbc_self_cal){
			s5k4h7_tsp_awb_calibration(otp_drv_handle);
		}
		if (cali_items->is_lsc && cali_items->is_lsc_self_cal){
			//s5k4h7_tsp_lsc_calibration(otp_drv_handle);
		}
	}
	/*If there are other items that need calibration, please add to here*/
	
	OTP_LOGI("out");
	return ret;
}

/*just for expend*/
/*==============================================================================
 * Description:
 * reserve interface: just for expending
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int s5k4h7_tsp_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd, void *params) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(otp_drv_handle);
	OTP_LOGI("in");
	
	/*you can add you command*/
	switch (cmd) {
	case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
		s5k4h7_tsp_compatible_convert(otp_drv_handle, params);
		break;
	/*jiangguijuan add,20180419*/
	/*case CMD_SNS_GET_MODULE_OTP_INFO:
		ret = s5k4h7_tsp_get_module_otp_info(otp_drv_handle, params);
		break;*/
	/*jiangguijuan add,20180419*/
	default:
		break;
	}
	OTP_LOGI("out");
	return ret;
}

void *otp_driver_open_lib(void)
{
     return &s5k4h7_tsp_drv_entry;
}