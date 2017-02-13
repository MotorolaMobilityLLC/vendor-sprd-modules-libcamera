#include "c5490_otp.h"

/** c5490_section_data_checksum:
 *    @buff: address of otp buffer
 *    @first: the start address of the section
 *    @last: the end address of the section
 *    @position: the section checksum offset
 *Return: unsigned char.
 **/
uint32_t c5490_section_checksum(unsigned char *buf,
unsigned int first, unsigned int last, unsigned int position)
{
	uint32_t ret = OTP_CAMERA_SUCCESS;
	unsigned int i = 0, sum = 0;

	OTP_LOGI("Enter");
	for(i = first; i <= last; i++){
		sum += buf[i];
	}
	if(sum%255 == buf[position]){
		OTP_LOGD("Checksum good, pos:%d, sum:%d buf: %d" , position,sum,buf[position]);
		ret = 1;
	} else {
		OTP_LOGD("Checksum error, pos:%d, sum:%d buf: %d" , position,sum,buf[position]);
		ret = -1;
	}
	OTP_LOGI("Exit");
	return ret;
}

uint32_t c5490_formatdata_buffer_init(void *ptr)
{
	cmr_int	ret = OTP_CAMERA_SUCCESS;
	cmr_int otp_len ;

	CHECK_PTR(ptr);
	OTP_LOGI("in");

	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;
	/*include random and golden lsc otp data,add reserve*/
	otp_len = sizeof(otp_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER;
	otp_format_data_t *otp_data = malloc(otp_len);
	if (NULL == otp_data){
		OTP_LOGE("%s,device malloc formatdata buffer failed\n",ctr_cxt->otp_params.dev_name);
		ret = -1;
	} else {
		ctr_cxt->otp_data_len = otp_len;
		memset(otp_data, 0, otp_len);
		lsccalib_data_t *lsc_data = &(otp_data->lsc_cali_dat);
		lsc_data->lsc_calib_golden.length = LSC_FORMAT_SIZE / 2;
		lsc_data->lsc_calib_golden.offset = sizeof(lsccalib_data_t);

		lsc_data->lsc_calib_random.length = LSC_FORMAT_SIZE / 2;
		lsc_data->lsc_calib_random.offset = sizeof(lsccalib_data_t) + LSC_FORMAT_SIZE / 2;
	}
	ctr_cxt->otp_data = otp_data;
	OTP_LOGI("out");
	return ret;
}

uint32_t c5490_format_wbdata(void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_PTR(ptr);
	OTP_LOGI("in");
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;
	awbcalib_data_t *wb_dat = &(ctr_cxt->otp_data->awb_cali_dat);
	awb_src_packet_t *wb = (awb_src_packet_t *)(ctr_cxt->otp_params.buffer + WB_INFO_OFFSET);
	ret = c5490_section_checksum(ctr_cxt->otp_params.buffer, WB_INFO_OFFSET, 
		WB_INFO_OFFSET + WB_DATA_SIZE, WB_INFO_CHECK_SUM);
	if (CMR_CAMERA_SUCCESS != ret){
		OTP_LOGE("%s,device malloc formatdata buffer failed\n",ctr_cxt->otp_params.dev_name);
	} else {
		uint32_t i;
		/*random*/
		for(i = 0; i < AWB_MAX_LIGHT; i++,wb++){
			wb_dat->awb_rdm_info[i].R = wb->R;
			wb_dat->awb_rdm_info[i].B = wb->B;
			wb_dat->awb_rdm_info[i].G = (wb->GR + wb->GR)/2;
			/*golden*/
			wb_dat->awb_gld_info[i].R = golden_awb[i].R;
			wb_dat->awb_gld_info[i].G = golden_awb[i].G;
			wb_dat->awb_gld_info[i].B = golden_awb[i].B;
		}
		for(i = 0; i < AWB_MAX_LIGHT; i++)
		OTP_LOGD("rdm:R=0x%x,G=0x%d,B=0x%x.god:R=0x%x,G=0x%d,B=0x%x",
				wb_dat->awb_rdm_info[i].R,
				wb_dat->awb_rdm_info[i].G,
				wb_dat->awb_rdm_info[i].B,
				wb_dat->awb_gld_info[i].R,
				wb_dat->awb_gld_info[i].G,
				wb_dat->awb_gld_info[i].B);
	}
	OTP_LOGI("out");
	return ret;
}

uint32_t c5490_format_lensdata(void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_PTR(ptr);
	OTP_LOGI("in");
	uint8_t *rdm_dst,*gld_dst;
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;
	lsccalib_data_t *lsc = &(ctr_cxt->otp_data->lsc_cali_dat);
	rdm_dst = (uint8_t*)lsc + lsc->lsc_calib_random.offset;
	gld_dst = (uint8_t*)lsc + lsc->lsc_calib_random.offset;

	ret = c5490_section_checksum(ctr_cxt->otp_params.buffer, LSC_INFO_OFFSET,
		LSC_INFO_OFFSET + LSC_CHANNEL_SIZE*4, LSC_INFO_CHECK_SUM);
	if (OTP_CAMERA_SUCCESS != ret){
		OTP_LOGI("%s,device malloc formatdata buffer failed\n",ctr_cxt->otp_params.dev_name);
	} else {
		/*R channel raw data*/
		memcpy(rdm_dst,ctr_cxt->otp_params.buffer + LSC_INFO_OFFSET,LSC_SRC_CHANNEL_SIZE * 4);
		lsc->lsc_calib_random.length = LSC_SRC_CHANNEL_SIZE * 4;
		/*gold data*/
		memcpy(gld_dst,golden_lsc,GAIN_WIDTH * GAIN_HEIGHT * 4); 
		lsc->lsc_calib_golden.length = GAIN_WIDTH * GAIN_HEIGHT * 4;
	}
	OTP_LOGI("out");
	return ret;

}

uint32_t c5490_wbc_calibration (void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_PTR(ptr);
	OTP_LOGI("in");
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;
	otp_calib_items_t *cal_items = &(ctr_cxt->otp_drv_cxt->otp_cfg.cali_items);
	awbcalib_data_t *wb_dat = &(ctr_cxt->otp_data->awb_cali_dat);
	int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;

	//calculate G gain

	R_gain = wb_dat->awb_gld_info[0].rg_ratio * 1000 /
				     wb_dat->awb_rdm_info[0].rg_ratio;
	B_gain = wb_dat->awb_gld_info[0].bg_ratio * 1000 /
				     wb_dat->awb_rdm_info[0].bg_ratio;
	G_gain = 1000;

	if (R_gain < 1000 || B_gain < 1000) {
		if (R_gain < B_gain)
			Base_gain = R_gain;
		else
			Base_gain = B_gain;
	} else {
		Base_gain = G_gain;
	}
	R_gain = 0x400 * R_gain / (Base_gain);
	B_gain = 0x400 * B_gain / (Base_gain);
	G_gain = 0x400 * G_gain / (Base_gain);

	OTP_LOGI("r_Gain=0x%x\n", R_gain);
	OTP_LOGI("g_Gain=0x%x\n", G_gain);
	OTP_LOGI("b_Gain=0x%x\n", B_gain);

	if(cal_items->is_awbc_self_cal) {
		OTP_LOGD("Do wb calibration local");
	}
	OTP_LOGI("out");
	return ret;
}
uint32_t c5490_lsc_calibration(void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_PTR(ptr);
	OTP_LOGI("in");
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;

	OTP_LOGI("out");
	return ret;
}

uint32_t c5490_pdaf_calibration(void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_PTR(ptr);
	OTP_LOGI("in");
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;

	OTP_LOGI("out");
	return ret;
}

uint32_t c5490_format_calibration_data(void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_PTR(ptr);
	OTP_LOGI("in");
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;
	module_data_t *module_dat = &(ctr_cxt->otp_data->module_dat);
	module_info_t *module_info;

	if (!ctr_cxt->otp_params.buffer || !ctr_cxt->otp_params.num_bytes) {
		OTP_LOGE("Buff pointer %p buffer size %d", ctr_cxt->otp_params.buffer,
		ctr_cxt->otp_params.num_bytes);
		return -1;
	}
	/* save module info */
	module_info = (module_info_t *)(ctr_cxt->otp_params.buffer + MODULE_INFO_OFFSET);
	module_dat->moule_id = module_info->moule_id;
	module_dat->vcm_id = module_info->vcm_id;
	module_dat->ir_bg_id = module_info->ir_bg_id;
	module_dat->drvier_ic_id = module_info->drvier_ic_id;

	OTP_LOGI("moule_id:%d\n vcm_id:%d\n ir_bg_id:%d\n drvier_ic_id:%d",
									           module_info->moule_id,
									           module_info->vcm_id,
									           module_info->ir_bg_id,
									           module_info->drvier_ic_id);

	c5490_format_wbdata(ptr);
	c5490_format_lensdata(ptr);

	OTP_LOGI("out");
	return ret;
}
uint32_t c5490_read_otp_data(void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	uint8_t try = 2;
	CHECK_PTR(ptr);

	OTP_LOGI("in");
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;
	otp_params_t *otp_params = &(ctr_cxt->otp_params);

	uint8_t *buffer = malloc(OTP_LEN);
	if (NULL == buffer){
		OTP_LOGE("%s,device malloc formatdata buffer failed\n",ctr_cxt->otp_params.dev_name);
		ret = -1;
	} else {
		memset(buffer,0,OTP_LEN);
		otp_params->buffer = buffer;
		otp_params->num_bytes = OTP_LEN;
		
		c5490_formatdata_buffer_init(ptr);
		/*start read otp data one time*/
		while(try--){
			//ret = sns_dev_i2c_read_seq(C5490_I2C_ADDR, buffer, SENSOR_I2C_REG_8BIT, OTP_LEN);
			if (ret == OTP_CAMERA_SUCCESS)
				break;
		}
		if(!try && (ret != OTP_CAMERA_SUCCESS)) {
			int i = 0;
			uint8_t cmd_val[3];
			for(i = 0; i < OTP_LEN; i++){
				cmd_val[0] = ((OTP_START_ADDR + i) >> 8) & 0xff;
				cmd_val[1] = (OTP_START_ADDR + i) & 0xff;
				//Sensor_ReadI2C(C5490_I2C_ADDR,(uint8_t*)&cmd_val[0],2);
				buffer[i] = cmd_val[0];
			}
		}
	}
	OTP_LOGI("out");
	return ret;
}
uint32_t c5490_write_otp_data(void *ptr)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_PTR(ptr);
	OTP_LOGI("in");
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)ptr;
	otp_params_t *otp_params = &(ctr_cxt->otp_params);

	if(NULL != otp_params->buffer){
		/*start read otp data one time*/
		//sns_dev_i2c_read_seq(C5490_I2C_ADDR, otp_params->buffer,
		//					SENSOR_I2C_REG_8BIT, otp_params->num_bytes);
		OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d",otp_params->dev_name,
									otp_params->buffer,otp_params->num_bytes);
	} else {
		OTP_LOGE("ERROR:buffer pointer is null");
		ret = -1;
	}
	OTP_LOGI("out");
	return ret;
}
