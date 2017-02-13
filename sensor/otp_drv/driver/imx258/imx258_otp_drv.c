#include "imx258_otp_drv.h"

/** c5490_section_data_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
uint32_t imx258_section_checksum(unsigned char *buf,
unsigned int offset, unsigned int data_count, unsigned int check_sum_offset)
{
	uint32_t ret = OTP_CAMERA_SUCCESS;
	unsigned int i = 0, sum = 0;

	OTP_LOGI("in");
	for(i = offset; i < offset + data_count; i++){
		sum += buf[i];
	}
	if( (sum%255 + 1) == buf[check_sum_offset] ){
		ret = OTP_CAMERA_SUCCESS;
	} else {
		ret = CMR_CAMERA_FAIL;
	}
	OTP_LOGI("out: offset:%d, checksum:%d buf: %d" , check_sum_offset,sum,buf[check_sum_offset]);
	return ret;
}

uint32_t imx258_formatdata_buffer_init(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	cmr_int otp_len ;
	OTP_LOGI("in");

	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	/*include random and golden lsc otp data,add reserve*/
	otp_len = sizeof(otp_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER;
	otp_format_data_t *otp_data = malloc(otp_len);
	if (NULL == otp_data){
		OTP_LOGE("malloc otp data buffer failed.\n");
		ret = CMR_CAMERA_FAIL;
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
uint32_t imx258_format_afdata(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	afcalib_data_t *af_cali_dat = &(ctr_cxt->otp_data->af_cali_dat);
	uint8_t *af_src_dat = ctr_cxt->otp_params.buffer + AF_INFO_OFFSET;
	ret = imx258_section_checksum(ctr_cxt->otp_params.buffer, AF_INFO_OFFSET, AF_INFO_SIZE - 1, AF_INFO_CHECKSUM);
	if (OTP_CAMERA_SUCCESS != ret){
		OTP_LOGE("auto focus checksum error,parse failed");
		return ret;
	} else {
		af_cali_dat->infinity_dac = (af_src_dat[1] << 8) | af_src_dat[0];
		af_cali_dat->macro_dac    = (af_src_dat[3] << 8) | af_src_dat[2];
	}
	OTP_LOGI("out");
	return ret;
}

uint32_t imx258_format_awbdata(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;
	awbcalib_data_t *awb_cali_dat = &(ctr_cxt->otp_data->awb_cali_dat);
	uint8_t *awb_src_dat = ctr_cxt->otp_params.buffer + AWB_INFO_OFFSET;
	
	ret = imx258_section_checksum(ctr_cxt->otp_params.buffer, AWB_INFO_OFFSET,
		AWB_INFO_SIZE, AWB_INFO_CHECKSUM);
	if (OTP_CAMERA_SUCCESS != ret){
		OTP_LOGE("awb otp data checksum error,parse failed");
		return ret;
	} else {
		uint32_t i;
		/*random*/
		OTP_LOGI("awb section count:0x%x",AWB_SECTION_NUM);
		for(i = 0; i < AWB_SECTION_NUM; i++,awb_src_dat+=AWB_INFO_SIZE){
			awb_cali_dat->awb_rdm_info[i].R = (awb_src_dat[1] << 8) | awb_src_dat[0];
			awb_cali_dat->awb_rdm_info[i].G = (awb_src_dat[3] << 8) | awb_src_dat[2];
			awb_cali_dat->awb_rdm_info[i].B = (awb_src_dat[5] << 8) | awb_src_dat[4];
			/*golden awb data ,you should ensure awb group number*/
			awb_cali_dat->awb_gld_info[i].R = golden_awb[i].R;
			awb_cali_dat->awb_gld_info[i].G = golden_awb[i].G;
			awb_cali_dat->awb_gld_info[i].B = golden_awb[i].B;
		}
		for(i = 0; i < AWB_MAX_LIGHT; i++)
			OTP_LOGD("rdm:R=0x%x,G=0x%d,B=0x%x.god:R=0x%x,G=0x%d,B=0x%x",
				awb_cali_dat->awb_rdm_info[i].R,
				awb_cali_dat->awb_rdm_info[i].G,
				awb_cali_dat->awb_rdm_info[i].B,
				awb_cali_dat->awb_gld_info[i].R,
				awb_cali_dat->awb_gld_info[i].G,
				awb_cali_dat->awb_gld_info[i].B);
	}
	OTP_LOGI("out");
	return ret;
}

uint32_t imx258_format_lscdata(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_HANDLE(handle);
	OTP_LOGI("in");

	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	lsccalib_data_t *lsc_dst  = &(ctr_cxt->otp_data->lsc_cali_dat);
	optical_center_t *opt_dst = &(ctr_cxt->otp_data->opt_center_dat);
	uint8_t *rdm_dst = (uint8_t*)lsc_dst + lsc_dst->lsc_calib_random.offset;
	uint8_t *gld_dst = (uint8_t*)lsc_dst + lsc_dst->lsc_calib_golden.offset;

	ret = imx258_section_checksum(ctr_cxt->otp_params.buffer, LSC_INFO_OFFSET,
		LSC_INFO_OFFSET + LSC_CHANNEL_SIZE*4, LSC_INFO_CHECKSUM);
	if (OTP_CAMERA_SUCCESS != ret){
		OTP_LOGI("lsc otp data checksum error,parse failed.\n");
	} else {
		/*optical center data*/
		uint8_t *opt_src = ctr_cxt->otp_params.buffer + OPTICAL_INFO_OFFSET;
		opt_dst->R.x  = (opt_src[1] << 8) | opt_src[0];
		opt_dst->R.y  = (opt_src[3] << 8) | opt_src[2];
		opt_dst->GR.x = (opt_src[5] << 8) | opt_src[4];
		opt_dst->GR.y = (opt_src[7] << 8) | opt_src[6];
		opt_dst->GB.x = (opt_src[9] << 8) | opt_src[8];
		opt_dst->GB.y = (opt_src[11] << 8) | opt_src[10];
		opt_dst->B.x  = (opt_src[13] << 8) | opt_src[12];
		opt_dst->B.y  = (opt_src[15] << 8) | opt_src[14];

		/*R channel raw data*/
		memcpy(rdm_dst,ctr_cxt->otp_params.buffer + LSC_INFO_OFFSET,LSC_SRC_CHANNEL_SIZE * 3);
		lsc_dst->lsc_calib_random.length = LSC_SRC_CHANNEL_SIZE * 3;
		/*gold data*/
		memcpy(gld_dst,golden_lsc,GAIN_WIDTH * GAIN_HEIGHT * 3); 
		lsc_dst->lsc_calib_golden.length = GAIN_WIDTH * GAIN_HEIGHT * 3;
	}
	OTP_LOGI("out");
	return ret;
}

uint32_t imx258_format_pdafdata(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	afcalib_data_t *af_cali_dat = ctr_cxt->otp_data->pdaf_cali_dat;
	uint8_t *pdaf_src_dat = ctr_cxt->otp_params.buffer + PDAF_INFO_OFFSET;
	ret = imx258_section_checksum(ctr_cxt->otp_params.buffer, PDAF_INFO_OFFSET, PDAF_INFO_SIZE, PDAF_INFO_CHECKSUM);
	if (OTP_CAMERA_SUCCESS != ret){
		OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
		return ret;
	} else {
		/*I don't know how to define the struct of padf*/
		ctr_cxt->otp_data->pdaf_cali_dat = pdaf_src_dat;
	}
	OTP_LOGI("out");
	return ret;
}

uint32_t imx258_awb_calibration(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	otp_calib_items_t *cal_items = &(ctr_cxt->otp_drv_cxt->otp_cfg.cali_items);
	awbcalib_data_t *awb_cali_dat = &(ctr_cxt->otp_data->awb_cali_dat);
	int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;

	//calculate G gain

	R_gain = awb_cali_dat->awb_gld_info[0].rg_ratio * 1000 /
                     awb_cali_dat->awb_rdm_info[0].rg_ratio;
	B_gain = awb_cali_dat->awb_gld_info[0].bg_ratio * 1000 /
                     awb_cali_dat->awb_rdm_info[0].bg_ratio;
	G_gain = 1000;

	if (R_gain < 1000 || B_gain < 1000) {
		if (R_gain < B_gain)
			Base_gain = R_gain;
		else
			Base_gain = B_gain;
	} else {
		Base_gain = G_gain;
	}
	if(Base_gain != 0) {
		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);
	} else {
		OTP_LOGE("awb parse problem!");
	}
	OTP_LOGI("r_Gain=0x%x\n", R_gain);
	OTP_LOGI("g_Gain=0x%x\n", G_gain);
	OTP_LOGI("b_Gain=0x%x\n", B_gain);

	if(cal_items->is_awbc_self_cal) {
		OTP_LOGD("Do wb calibration local");
	}
	OTP_LOGI("out");
	return ret;
}
uint32_t imx258_lsc_calibration(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;


	OTP_LOGI("out");
	return ret;
}

uint32_t imx258_pdaf_calibration(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	OTP_LOGI("out");
	return ret;
}

uint32_t imx258_format_calibration_data(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	module_data_t *module_dat  = &(ctr_cxt->otp_data->module_dat);
	module_info_t *module_info = NULL;

	if (!ctr_cxt->otp_params.buffer || !ctr_cxt->otp_params.num_bytes) {
		OTP_LOGE("Buff pointer %p buffer size %d", ctr_cxt->otp_params.buffer,
		ctr_cxt->otp_params.num_bytes);
		return OTP_CAMERA_FAIL;
	}
	/* save module info */
	module_info = (module_info_t *)(ctr_cxt->otp_params.buffer + MODULE_INFO_OFFSET);
	module_dat->moule_id = module_info->moule_id;
	module_dat->vcm_id   = module_info->vcm_id;
	module_dat->ir_bg_id = module_info->ir_bg_id;
	module_dat->drvier_ic_id = module_info->drvier_ic_id;

	OTP_LOGI("moule_id:%d\n vcm_id:%d\n ir_bg_id:%d\n drvier_ic_id:%d",
                                                module_info->moule_id,
                                                module_info->vcm_id,
                                                module_info->ir_bg_id,
                                                module_info->drvier_ic_id);
	imx258_format_afdata(handle);
	imx258_format_awbdata(handle);
	imx258_format_lscdata(handle);
	imx258_format_pdafdata(handle);

	OTP_LOGI("out");
	return ret;
}
uint32_t imx258_read_otp_data(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	uint8_t try = 2;

	CHECK_HANDLE(handle);
	OTP_LOGI("in");
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;
	otp_params_t *otp_params = &(ctr_cxt->otp_params);

	uint8_t *buffer = malloc(OTP_LEN);
	if (NULL == buffer){
		OTP_LOGE("malloc otp raw buffer failed\n");
		ret = OTP_CAMERA_FAIL;
	} else {
		memset(buffer,0,OTP_LEN);
		otp_params->buffer = buffer;
		otp_params->num_bytes = OTP_LEN;

		imx258_formatdata_buffer_init(handle);
		/*start read otp data one time*/
#if 0
		while(try--){
			buffer[0] = (OTP_START_ADDR >> 8) & 0xFF;
			buffer[1] = OTP_START_ADDR & 0xFF;
			ret = Sensor_ReadI2C_SEQ(GT24C64A_I2C_ADDR, buffer, SENSOR_I2C_REG_16BIT, OTP_LEN);
			OTP_LOGI("1,ret:0x%x,try:0x%x",ret,try);
			if (ret == OTP_CAMERA_SUCCESS)
				break;
		}
#endif
		//if(!try && (ret != OTP_CAMERA_SUCCESS)) {
		if (1) {
			int i = 0;
			uint8_t cmd_val[3];
			for(i = 0; i < OTP_LEN; i++){
				cmd_val[0] = ((OTP_START_ADDR + i) >> 8) & 0xff;
				cmd_val[1] = (OTP_START_ADDR + i) & 0xff;
				Sensor_ReadI2C(GT24C64A_I2C_ADDR,(uint8_t*)&cmd_val[0],2);
				buffer[i] = cmd_val[0];
			}
		}
		sensor_otp_dump_raw_data(handle);
	}
	OTP_LOGI("out");
	return ret;
}
uint32_t imx258_write_otp_data(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;

	CHECK_HANDLE(handle);
	OTP_LOGI("in");
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	otp_params_t *otp_params = &(ctr_cxt->otp_params);

	if(NULL != otp_params->buffer){
		int i;
		for(i=0; i<otp_params->num_bytes; i++){
			Sensor_WriteI2C(GT24C64A_I2C_ADDR, &otp_params->buffer[i],2);
		}
		OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d",otp_params->dev_name,
                                   otp_params->buffer,otp_params->num_bytes);
	} else {
		OTP_LOGE("ERROR:buffer pointer is null");
		ret = OTP_CAMERA_FAIL;
	}
	OTP_LOGI("out");
	return ret;
}

otp_drv_cxt_t imx258_drv_cxt = 
{
	.otp_cfg =
	{
		.cali_items =
		{
			.is_self_cal = TRUE,    /*1:calibration at sensor side,0:calibration at isp*/
			.is_dul_camc = FALSE,   /* support dual camera calibration */
			.is_awbc     = TRUE,    /* support write balance calibration */
			.is_lsc      = TRUE,    /* support lens shadding calibration */
			.is_pdafc    = FALSE,   /* support pdaf calibration */
		},
		.base_info_cfg =
		{
			.compress_flag = OTP_COMPRESSED_FLAG,/*otp data compressed format ,should confirm with sensor fae*/
			.image_width   = 4208, /*the width of the stream the sensor can output*/
			.image_height  = 3120, /*the height of the stream the sensor can output*/
			.grid_width    = 23,   /*the height of the stream the sensor can output*/
			.grid_height   = 18,
			.gain_width    = GAIN_WIDTH,
			.gain_height   = GAIN_HEIGHT,
		},
	},
	.otp_ops =
	{
		.format_calibration_data = imx258_format_calibration_data,
		.awb_calibration         = imx258_awb_calibration,
		.lsc_calibration         = imx258_lsc_calibration,
		.pdaf_calibration        = NULL,
		.dul_cam_calibration     = NULL,
		.read_otp_data           = imx258_read_otp_data,
		.write_otp_data          = imx258_write_otp_data,
	},
};
