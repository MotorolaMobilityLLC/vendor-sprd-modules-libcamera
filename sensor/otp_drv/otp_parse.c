#include "otp_parse.h"
#include <string.h>

#if defined(__LP64__)
static char *otp_lib_path = "/system/lib64/libcamotp.so";
#else
static char *otp_lib_path = "/system/lib/libcamotp.so";
#endif

static char *otp_bin_path = "/data/misc/cameraserver/";

/** sensor_otp_calibration_data:
 *  @handle: sensor driver instance
 *
 * do some calibration on sensor side or isp
 * if is_self_cal is true,do calibration at sensor side,
 * otherwise at isp module.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : calibration success
 * CMR_CAMERA_FAILURE : calibration failed
 **/
static uint32_t sensor_otp_calibration_data(SENSOR_HW_HANDLE handle){
	cmr_int ret = OTP_CAMERA_SUCCESS;

	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	otp_calib_items_t *cali_items = &(ctr_cxt->otp_drv_cxt->otp_cfg.cali_items);
	otp_ops_t *otp_ops = &(ctr_cxt->otp_drv_cxt->otp_ops);

	if(otp_ops && cali_items) {
		if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal &&
                            (otp_ops->pdaf_calibration != NULL))
			otp_ops->pdaf_calibration(handle);
		/*calibration at sensor or isp */
		if (cali_items->is_awbc && cali_items->is_awbc_self_cal &&
                           (otp_ops->awb_calibration != NULL))
			otp_ops->awb_calibration(handle);
		if (cali_items->is_lsc && cali_items->is_awbc_self_cal &&
                            (otp_ops->lsc_calibration != NULL))
			otp_ops->lsc_calibration(handle);
	}
    /*If there are other items that need calibration, please add to here*/
	OTP_LOGI("Exit");
	return ret;
}
/** sensor_otp_read_format_data:
 *  handle: sensor driver instance.
 *
 *  NOTE:
 *  Firstly driver will read otp format data from binary file that saved
 *  in /data/misc/cameraserver. If the bin file don't exist, the driver
 *  will read otp raw data and decompress raw data if need. After that 
 *  otp driver will check whether do self calibration or not.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read success
 * CMR_CAMERA_FAILURE : read failed
 **/
static uint32_t sensor_otp_read_format_data(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	otp_ops_t *otp_ops    = &(ctr_cxt->otp_drv_cxt->otp_ops);
	otp_base_info_cfg_t *base_info = &(ctr_cxt->otp_drv_cxt->otp_cfg.base_info_cfg);

	ret = sensor_otp_rw_data_from_file(handle,READ_FORMAT_OTP_FROM_BIN); /*load format data from bin file*/
	if (ret != OTP_CAMERA_SUCCESS){
		OTP_LOGI("ret:%d,otp_ops:0x%x",ret,otp_ops);
		ret = otp_ops->read_otp_data(handle); /*read otp raw data from sensor*/
		if(ret != OTP_CAMERA_SUCCESS){
			return OTP_CAMERA_FAIL;
		}
		ret = otp_ops->format_calibration_data(handle); /*parse otp raw data*/
		if(ret != OTP_CAMERA_SUCCESS){
			return OTP_CAMERA_FAIL;
		}
		if(base_info->compress_flag != GAIN_ORIGIN_BITS){ /*decompress lsc data if needed*/
			ret = sensor_otp_lsc_decompress(handle);
			if(ret != OTP_CAMERA_SUCCESS){
				return OTP_CAMERA_FAIL;
			}
		}

		ret = sensor_otp_rw_data_from_file(handle,WRITE_FORMAT_OTP_TO_BIN);
	}
	OTP_LOGI("Exit");
	return ret;
}
/** sensor_otp_rw_data_from_file:
 *  @handle: sensor driver instance.
 *  @cmd: the command of read or write otp
 *          data from binary file.
 * NOTE:
 * This function executes before sensor ready to read otp raw data
 * from sensor eeprom. If the formated binary file exists,
 * otp driver will load it to a buffer.But if doesn't exist,the
 * function will return.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read or write otp from file success
 * CMR_CAMERA_FAILURE : read or write otp from file failed
 **/
static uint32_t sensor_otp_rw_data_from_file(SENSOR_HW_HANDLE handle,uint8_t cmd)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;

	char otp_bin_ext_path[255];
	FILE* fp = NULL;

	snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path), "%s%s_otp.bin",
                        otp_bin_path,sensor_cxt->sensor_info_ptr->name);
	OTP_LOGD("otp_data_path:%s",otp_bin_ext_path);

	switch(cmd)
	{
		case WRITE_FORMAT_OTP_TO_BIN:
			fp = fopen(otp_bin_ext_path, "wb");
			if (fp != NULL){
				fwrite(ctr_cxt->otp_data, 1, ctr_cxt->otp_data_len, fp);
				fclose(fp);
			} else {
				OTP_LOGE("fp is null!!,write otp bin failed");
				ret = OTP_CAMERA_FAIL;
			}
			break;
		case READ_FORMAT_OTP_FROM_BIN:
			{
				uint32_t try_time = 3,mRead = 0;
				if (-1 != access(otp_bin_ext_path,0)){
					fp = fopen(otp_bin_ext_path, "rb");
					if (fp != NULL){
						fseek(fp, 0L, SEEK_END );
						ctr_cxt->otp_data_len = ftell(fp); //get bin size
						ctr_cxt->otp_data = malloc(ctr_cxt->otp_data_len);
						if (ctr_cxt->otp_data){
							while(try_time--){
								mRead = fread(ctr_cxt->otp_data, 1, ctr_cxt->otp_data_len, fp);
								if ((mRead == 0) && (feof(fp) != 0))
									break;
								else
									OTP_LOGE("error:otp lenght doesn't match,Read:0x%x,otp_data_len:0x%x",
                                                               mRead,ctr_cxt->otp_data_len);
							}
							fclose(fp);
							if((!try_time) && (mRead != ctr_cxt->otp_data_len)){
								free(ctr_cxt->otp_data);
								ctr_cxt->otp_data = NULL;
								ret = OTP_CAMERA_FAIL;
							}
						} else {
							OTP_LOGE("malloc otp data buffer failed");
							fclose(fp);
							ret = OTP_CAMERA_FAIL;
						}
					} else {
						OTP_LOGE("fp is null!!,read otp bin failed");
						ret = OTP_CAMERA_FAIL;
					}
				} else {
					OTP_LOGE("file don't exist");
					ret = OTP_CAMERA_FAIL;
				}
			}
			break;
		default:
			OTP_LOGE("your cmd is wrong,please check you cmd");
			ret = OTP_CAMERA_FAIL;
			break;
	}

	OTP_LOGI("Exit");
	return ret;
}
/** sensor_otp_write_format_data:
 *  @handle: sensor driver instance.
 *
 * NOTE:
 * If you need to write otp data to sensor eeprom.This interface is your
 * wanted.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : write otp success
 * CMR_CAMERA_FAILURE : write otp failed
 **/
static uint32_t sensor_otp_write_format_data(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	OTP_LOGI("in");
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;
	otp_ops_t *otp_ops      = &(ctr_cxt->otp_drv_cxt->otp_ops);

	if(otp_ops){
		ret = otp_ops->write_otp_data(ctr_cxt);
	} else {
		ret = OTP_CAMERA_FAIL;
		OTP_LOGE("write_otp_data failed");
	}
	OTP_LOGI("Exit");
	return ret;
}
/** sensor_otp_process:
 *  @handle: sensor driver instance.
 *
 * NOTE:
 * The only interface provided to the upper layer.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read or write success.
 * CMR_CAMERA_FAILURE : read or write failed
 **/
uint32_t sensor_otp_process(SENSOR_HW_HANDLE handle, int cmd, void* params)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	OTP_LOGD("sensor_cxt:0x%x",sensor_cxt);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;
	//otp_ctrl_cmd_t *otp_ctrl_data = (otp_ctrl_cmd_t *)params;
	switch(cmd)
	{
	case READ_FORMAT_OTP_DATA:
		ret = sensor_otp_read_format_data(handle);
		if(ret != OTP_CAMERA_SUCCESS){
			OTP_LOGE("sensor read format data failed,ret:%d",ret);
			goto failed;
		}
		ret = sensor_otp_calibration_data(handle);
		if(ret != OTP_CAMERA_SUCCESS){
			OTP_LOGE("sensor read format data failed,ret:%d",ret);
			goto failed;
		}
		break;
	case WRITE_FORMAT_OTP_DATA:
		/*here you can parse you data*/
		memcpy(&(ctr_cxt->otp_params),params,sizeof(otp_params_t));
		ret = sensor_otp_write_format_data(handle);
		if(ret != OTP_CAMERA_SUCCESS){
			OTP_LOGE("sensor write otp data failed,ret:%d",ret);
			goto failed;
		}
		break;
	case RELEASE_OTP_DATA:
failed:
		if(ctr_cxt->otp_params.buffer){
			free(ctr_cxt->otp_params.buffer);
			ctr_cxt->otp_params.buffer = NULL;
		}
		if(ctr_cxt->otp_data){
			free(ctr_cxt->otp_data);
			ctr_cxt->otp_data = NULL;
		}
		break;
	default:
		break;
	}
	OTP_LOGD("out");
	return ret;
}

/** sensor_otp_lsc_decompress:
 *  @handle: sensor driver instance.
 *
 * NOTE:
 * decompress otp data read from otp eeprom.
 *
 * This function executes in module sensor context
 *
 * Return:
 * SENSOR_SUCCESS : decompress success
 * SENSOR_FAILURE : decompress failed
 **/
static uint32_t sensor_otp_lsc_decompress(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;


	uint32_t compress_bits_size;
	uint32_t channal_cmp_bytes_size=0;
	uint32_t one_channal_decmp_size=0;
	uint32_t gain_compressed_bits = 0;
	uint32_t gain_mak_bits = 0;
	uint32_t random_buf_size = 0;
	uint16_t * random_buf = NULL;
	uint16_t * ptr_random_buf = NULL;
	uint32_t cmp_uncompensate_size=0;
	uint32_t i;
	otp_base_info_cfg_t otp_base_info;
	lsccalib_data_t *lsc_cal_data;
	unsigned char *lsc_rdm_src_data,*lsc_rdm_dst_data;

	otp_base_info = ctr_cxt->otp_drv_cxt->otp_cfg.base_info_cfg;
	/* get lsc data pointer*/
	lsc_cal_data = &(ctr_cxt->otp_data->lsc_cali_dat);
	/* get random lsc data */
	lsc_rdm_src_data = (unsigned char*)lsc_cal_data + lsc_cal_data->lsc_calib_random.offset;

	compress_bits_size = otp_base_info.gain_width * otp_base_info.gain_height * GAIN_COMPRESSED_14BITS;
	channal_cmp_bytes_size = (compress_bits_size + GAIN_ORIGIN_BITS - 1) / GAIN_ORIGIN_BITS * (GAIN_ORIGIN_BITS / 8);

	if(0 == channal_cmp_bytes_size){
		return -1;
	}
	cmp_uncompensate_size = otp_base_info.gain_width * otp_base_info.gain_height * \
					gain_compressed_bits % GAIN_ORIGIN_BITS;

	/*malloc random temp buffer*/
	random_buf_size = otp_base_info.gain_width * otp_base_info.gain_height *  \
						sizeof(uint16_t) * CHANNAL_NUM;
	lsc_rdm_dst_data = (uint16_t *)malloc(random_buf_size);
	if (NULL == lsc_rdm_dst_data){
		ret = -1;
		OTP_LOGE("malloc decompress buf failed!");
		goto EXIT;
	}

	gain_compressed_bits = GAIN_COMPRESSED_14BITS;
	gain_mak_bits = GAIN_MASK_14BITS;

	for(i=0;i<CHANNAL_NUM;i++){
		one_channal_decmp_size = sensor_otp_decompress_gain(lsc_rdm_src_data, channal_cmp_bytes_size,
				cmp_uncompensate_size,  lsc_rdm_dst_data, gain_compressed_bits, gain_mak_bits);
		if(0 == one_channal_decmp_size){
			ret = -1;
			goto EXIT;
		}
		lsc_rdm_src_data += channal_cmp_bytes_size / 2;
		lsc_rdm_dst_data += one_channal_decmp_size;
	}
	memcpy(lsc_rdm_src_data,lsc_rdm_dst_data,random_buf_size);
EXIT:

	if(NULL != lsc_rdm_dst_data){
		free(lsc_rdm_dst_data);
		lsc_rdm_dst_data = NULL;
	}
	return ret;
}
/** sensor_otp_decompress_gain:
 *  @src: random lsc otp data.
 *  @src_bytes: the orign data size one channel.
 *  @src_uncompensate_bytes: the post data size of one channel.
 *  @dst:the destination buffer.
 *
 * NOTE:
 * The core decompression method.
 *
 * This function executes in module sensor context
 *
 * Return:
 * SENSOR_SUCCESS : decompress success
 * SENSOR_FAILURE : decompress failed
 **/
static uint32_t sensor_otp_decompress_gain(uint16_t *src, uint32_t src_bytes,
        uint32_t src_uncompensate_bytes, uint16_t *dst,uint32_t GAIN_COMPRESSED_BITS,uint32_t GAIN_MASK)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t bit_left = 0;
	uint32_t bit_buf = 0;
	uint32_t offset = 0;
	uint32_t dst_gain_num = 0;

	if(NULL == src || NULL == dst){
		return 0;
	}

	if(0 == src_bytes || 0 != (src_bytes & 1)){
		return 0;
	}

	if(12==GAIN_COMPRESSED_BITS)
	{
		for(i=0; i<src_bytes/2; i++){
			bit_buf |= src[i] << bit_left;
			bit_left += GAIN_ORIGIN_BITS;

			if(bit_left > GAIN_COMPRESSED_BITS){
				offset = 0;
				while(bit_left >= GAIN_COMPRESSED_BITS){
					dst[j] = (bit_buf & GAIN_MASK)<<2;
					j++;
					bit_left -= GAIN_COMPRESSED_BITS;
					bit_buf = (bit_buf >> GAIN_COMPRESSED_BITS);
				}
			}
		}
	}else if(14==GAIN_COMPRESSED_BITS)
	{
		for(i=0; i<src_bytes/2; i++){
			bit_buf |= src[i] << bit_left;
			bit_left += GAIN_ORIGIN_BITS;

			if(bit_left > GAIN_COMPRESSED_BITS){
				offset = 0;
				while(bit_left >= GAIN_COMPRESSED_BITS){
					dst[j] = (bit_buf & GAIN_MASK);
					j++;
					bit_left -= GAIN_COMPRESSED_BITS;
					bit_buf = (bit_buf >> GAIN_COMPRESSED_BITS);
				}
			}
		}
	}

	if(GAIN_COMPRESSED_BITS==src_uncompensate_bytes)
	{
		dst_gain_num = j-1;
	}else{
		dst_gain_num = j;
	}

	return dst_gain_num;
}

void sensor_otp_change_pattern(uint32_t pattern, uint16_t *interlaced_gain, uint16_t *chn_gain[4], uint16_t gain_num)
{
	uint32_t i = 0;
	uint32_t chn_idx[4] = {0};
	uint16_t *chn[4] = {NULL};

	chn[0] = chn_gain[0];
	chn[1] = chn_gain[1];
	chn[2] = chn_gain[2];
	chn[3] = chn_gain[3];

	switch (pattern) {
	case SENSOR_IMAGE_PATTERN_RGGB:
	default:
		chn_idx[0] = 0;
		chn_idx[1] = 1;
		chn_idx[2] = 2;
		chn_idx[3] = 3;
		break;

	case SENSOR_IMAGE_PATTERN_GRBG:
		chn_idx[0] = 1;
		chn_idx[1] = 0;
		chn_idx[2] = 3;
		chn_idx[3] = 2;
		break;

	case SENSOR_IMAGE_PATTERN_GBRG:
		chn_idx[0] = 2;
		chn_idx[1] = 3;
		chn_idx[2] = 0;
		chn_idx[3] = 1;
		break;

	case SENSOR_IMAGE_PATTERN_BGGR:
		chn_idx[0] = 3;
		chn_idx[1] = 2;
		chn_idx[2] = 1;
		chn_idx[3] = 0;
		break;
	}

	for (i=0; i<gain_num; i++) {
		*interlaced_gain++ = *chn[chn_idx[0]]++;
		*interlaced_gain++ = *chn[chn_idx[1]]++;
		*interlaced_gain++ = *chn[chn_idx[2]]++;
		*interlaced_gain++ = *chn[chn_idx[3]]++;
	}
}
/** sensor_otp_dump_raw_data:
 *  @handle: sensor driver instance.
 *
 * NOTE:
 * Dump otp raw data,when debug.
 *
 * Return:
 * SENSOR_SUCCESS : decompress success
 * SENSOR_FAILURE : decompress failed
 **/
uint32_t sensor_otp_dump_raw_data(SENSOR_HW_HANDLE handle)
{
	cmr_int ret = OTP_CAMERA_SUCCESS;
	char value[255];
	char otp_bin_ext_path[255];

	CHECK_HANDLE(handle);
	struct sensor_drv_context *sensor_cxt = (struct sensor_drv_context *)(handle->privatedata);
	otp_ctrl_cxt_t *ctr_cxt = (otp_ctrl_cxt_t*)sensor_cxt->sensor_otp_cxt;
	otp_params_t *otp_buffer = &ctr_cxt->otp_params;

	property_get("debug.camera.save.otp.raw.data", value, "0");
	if ( (atoi(value) == 1) && otp_buffer->buffer ) {
		snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path), "%s%s_otp_dump.bin",
                                 otp_bin_path,sensor_cxt->sensor_info_ptr->name);
		OTP_LOGD("otp_data_dump_path:%s",otp_bin_ext_path);
		FILE* fp = fopen(otp_bin_ext_path, "wb");
		if( fp != NULL){
			fwrite(otp_buffer->buffer, 1, otp_buffer->num_bytes, fp);
			fclose(fp);
		} else {
			OTP_LOGE("fp is null!! dump otp raw data failed");
			ret = -1;
		}
	}
	return ret;
}
