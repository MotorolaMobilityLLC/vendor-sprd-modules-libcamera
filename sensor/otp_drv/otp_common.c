#include "otp_common.h"
#include <string.h>

#if defined(__LP64__)
static char *otp_lib_path = "/system/lib64/libcamotp.so";
#else
static char *otp_lib_path = "/system/lib/libcamotp.so";
#endif

static char *otp_bin_path = "/data/misc/cameraserver/";

/** sensor_otp_rw_data_from_file:
 *  @otp_drv_handle: sensor driver instance.
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
int sensor_otp_rw_data_from_file(uint8_t cmd, char *sensor_name,
                                 otp_format_data_t **otp_data,
                                 int *format_otp_size) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR((void *)otp_data);

    char otp_bin_ext_path[255];
    FILE *fp = NULL;

    snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path), "%s%s_parsed_otp.bin",
             otp_bin_path, sensor_name);

    switch (cmd) {
    case OTP_WRITE_FORMAT_TO_BIN:
        OTP_LOGI("write parsed data:%s", otp_bin_ext_path);
        fp = fopen(otp_bin_ext_path, "wb");
        if (fp != NULL) {
            fwrite(*otp_data, 1, *format_otp_size, fp);
            fclose(fp);
        } else {
            OTP_LOGE("fp is null!!,write otp bin failed");
            ret = OTP_CAMERA_FAIL;
        }
        break;
    case OTP_READ_FORMAT_FROM_BIN: {
        OTP_LOGI("read parsed data:%s", otp_bin_ext_path);
        uint32_t try_time = 3, mRead = 0;
        if (-1 != access(otp_bin_ext_path, 0)) {
            fp = fopen(otp_bin_ext_path, "rb");
            if (fp != NULL) {
                fseek(fp, 0L, SEEK_END);
                *format_otp_size = ftell(fp); // get bin size
                *otp_data = malloc(*format_otp_size);
                if (*otp_data) {
                    while (try_time--) {
                        mRead = fread(*otp_data, 1, *format_otp_size, fp);
                        if ((mRead == 0) && (feof(fp) != 0))
                            break;
                        else
                            OTP_LOGE("error:otp lenght doesn't "
                                     "match,Read:0x%x,otp_data_len:0x%x",
                                     mRead, *format_otp_size);
                    }
                    fclose(fp);
                    if ((!try_time) && (mRead != *format_otp_size)) {
                        free(*otp_data);
                        *otp_data = NULL;
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
    } break;
    default:
        OTP_LOGE("your cmd is wrong,please check you cmd");
        ret = OTP_CAMERA_FAIL;
        break;
    }

    OTP_LOGI("Exit");
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
int sensor_otp_lsc_decompress(otp_base_info_cfg_t *otp_base_info,
                              lsccalib_data_t *lsc_cal_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_base_info);
    CHECK_PTR(lsc_cal_data);

    uint32_t compress_bits_size;
    uint32_t channal_cmp_bytes_size = 0;
    uint32_t one_channal_decmp_size = 0;
    uint32_t gain_compressed_bits = 0;
    uint32_t gain_mak_bits = 0;
    uint32_t random_buf_size = 0;
    uint16_t *random_buf = NULL;
    uint16_t *ptr_random_buf = NULL;
    uint32_t cmp_uncompensate_size = 0;
    uint32_t i;

    unsigned char *lsc_rdm_src_data, *lsc_rdm_dst_data;

    /* get random lsc data */
    lsc_rdm_src_data =
        (unsigned char *)lsc_cal_data + lsc_cal_data->lsc_calib_random.offset;

    compress_bits_size = otp_base_info->gain_width *
                         otp_base_info->gain_height * GAIN_COMPRESSED_14BITS;
    channal_cmp_bytes_size = (compress_bits_size + GAIN_ORIGIN_BITS - 1) /
                             GAIN_ORIGIN_BITS * (GAIN_ORIGIN_BITS / 8);

    if (0 == channal_cmp_bytes_size) {
        return -1;
    }
    cmp_uncompensate_size = otp_base_info->gain_width *
                            otp_base_info->gain_height * gain_compressed_bits %
                            GAIN_ORIGIN_BITS;

    /*malloc random temp buffer*/
    random_buf_size = otp_base_info->gain_width * otp_base_info->gain_height *
                      sizeof(uint16_t) * CHANNAL_NUM;
    lsc_rdm_dst_data = (uint16_t *)malloc(random_buf_size);
    if (NULL == lsc_rdm_dst_data) {
        ret = -1;
        OTP_LOGE("malloc decompress buf failed!");
        goto EXIT;
    }

    gain_compressed_bits = GAIN_COMPRESSED_14BITS;
    gain_mak_bits = GAIN_MASK_14BITS;

    for (i = 0; i < CHANNAL_NUM; i++) {
        one_channal_decmp_size = sensor_otp_decompress_gain(
            lsc_rdm_src_data, channal_cmp_bytes_size, cmp_uncompensate_size,
            lsc_rdm_dst_data, gain_compressed_bits, gain_mak_bits);
        if (0 == one_channal_decmp_size) {
            ret = -1;
            goto EXIT;
        }
        lsc_rdm_src_data += channal_cmp_bytes_size / 2;
        lsc_rdm_dst_data += one_channal_decmp_size;
    }
    memcpy(lsc_rdm_src_data, lsc_rdm_dst_data, random_buf_size);
EXIT:

    if (NULL != lsc_rdm_dst_data) {
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
int sensor_otp_decompress_gain(uint16_t *src, uint32_t src_bytes,
                               uint32_t src_uncompensate_bytes, uint16_t *dst,
                               uint32_t GAIN_COMPRESSED_BITS,
                               uint32_t GAIN_MASK) {
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t bit_left = 0;
    uint32_t bit_buf = 0;
    uint32_t offset = 0;
    uint32_t dst_gain_num = 0;

    if (NULL == src || NULL == dst) {
        return 0;
    }

    if (0 == src_bytes || 0 != (src_bytes & 1)) {
        return 0;
    }

    if (12 == GAIN_COMPRESSED_BITS) {
        for (i = 0; i < src_bytes / 2; i++) {
            bit_buf |= src[i] << bit_left;
            bit_left += GAIN_ORIGIN_BITS;

            if (bit_left > GAIN_COMPRESSED_BITS) {
                offset = 0;
                while (bit_left >= GAIN_COMPRESSED_BITS) {
                    dst[j] = (bit_buf & GAIN_MASK) << 2;
                    j++;
                    bit_left -= GAIN_COMPRESSED_BITS;
                    bit_buf = (bit_buf >> GAIN_COMPRESSED_BITS);
                }
            }
        }
    } else if (14 == GAIN_COMPRESSED_BITS) {
        for (i = 0; i < src_bytes / 2; i++) {
            bit_buf |= src[i] << bit_left;
            bit_left += GAIN_ORIGIN_BITS;

            if (bit_left > GAIN_COMPRESSED_BITS) {
                offset = 0;
                while (bit_left >= GAIN_COMPRESSED_BITS) {
                    dst[j] = (bit_buf & GAIN_MASK);
                    j++;
                    bit_left -= GAIN_COMPRESSED_BITS;
                    bit_buf = (bit_buf >> GAIN_COMPRESSED_BITS);
                }
            }
        }
    }

    if (GAIN_COMPRESSED_BITS == src_uncompensate_bytes) {
        dst_gain_num = j - 1;
    } else {
        dst_gain_num = j;
    }

    return dst_gain_num;
}

void sensor_otp_change_pattern(uint32_t pattern, uint16_t *interlaced_gain,
                               uint16_t *chn_gain[4], uint16_t gain_num) {
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

    for (i = 0; i < gain_num; i++) {
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
int sensor_otp_dump_raw_data(uint8_t *buffer, int size, char *dev_name) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    char value[255];
    char otp_bin_ext_path[255];

    CHECK_PTR(buffer);
    property_get("debug.camera.save.otp.raw.data", value, "0");
    if (atoi(value) == 1) {
        snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path),
                 "%s%s_otp_dump.bin", otp_bin_path, dev_name);
        OTP_LOGD("otp_data_dump_path:%s", otp_bin_ext_path);
        FILE *fp = fopen(otp_bin_ext_path, "wb");
        if (fp != NULL) {
            fwrite(buffer, 1, size, fp);
            fclose(fp);
        } else {
            OTP_LOGE("fp is null!! dump otp raw data failed");
            ret = -1;
        }
    }
    return ret;
}

/** sensor_otp_drv_create:
 *  @handle: create sensor driver instance.
 *
 * NOTE:
 * The only interface provided to the upper layer.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read or write success.
 * CMR_CAMERA_FAILURE : read or write failed
 **/
int sensor_otp_drv_create(otp_drv_init_para_t *input_para, cmr_handle* sns_otp_drv_handle) {
	cmr_int ret = OTP_CAMERA_SUCCESS;
	CHECK_PTR(input_para);
	CHECK_PTR((void*)sns_otp_drv_handle);
	OTP_LOGI("In");

	otp_drv_cxt_t *otp_cxt = malloc(sizeof(otp_drv_cxt_t));
	if(!otp_cxt) {
		OTP_LOGD("otp handle create failed!");
		return OTP_CAMERA_FAIL;
	}
	memset(otp_cxt,0,sizeof(otp_drv_cxt_t));
	if(input_para->sensor_name)
		memcpy(otp_cxt->dev_name,input_para->sensor_name,32);
	otp_cxt->otp_raw_data.buffer = NULL;
	otp_cxt->otp_data = NULL;
	otp_cxt->hw_handle= input_para->hw_handle;
	OTP_LOGI("out");
	*sns_otp_drv_handle = otp_cxt;

	return ret;
}

/** sensor_otp_drv_delete:
 *  @handle: delete otp driver instance.
 *
 * NOTE:
 * The only interface provided to the upper layer.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read or write success.
 * CMR_CAMERA_FAILURE : read or write failed
 **/

int sensor_otp_drv_delete(void *otp_drv_handle) {
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("In");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    if (otp_cxt->otp_raw_data.buffer) {
        free(otp_cxt->otp_raw_data.buffer);
        otp_cxt->otp_raw_data.buffer = NULL;
    }
    if (otp_cxt->otp_data) {
        free(otp_cxt->otp_data);
        otp_cxt->otp_data = NULL;
    }
    if (otp_cxt->compat_convert_data) {
        free(otp_cxt->compat_convert_data);
        otp_cxt->compat_convert_data = NULL;
    }
    if (otp_cxt) {
        free(otp_cxt);
    }
    OTP_LOGI("out");
    return 0;
}
