#ifndef _OTP_PARSE_COMMON_H_
#define _OTP_PARSE_COMMON_H_

#include "cmr_common.h"
#include "sensor_drv_u.h"
#include <cutils/properties.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
#define OTP_RESERVE_BUFFER 64
#define CHANNAL_NUM 4

#define OTP_LOGI       CMR_LOGI
#define OTP_LOGD       CMR_LOGD
#define OTP_LOGE       CMR_LOGE
#define OTP_CAMERA_SUCCESS  CMR_CAMERA_SUCCESS
#define OTP_CAMERA_FAIL     CMR_CAMERA_FAIL


#ifndef CHECK_PTR
#define CHECK_PTR(expr)\
    if ((expr) == NULL) {\
        ALOGE("ERROR: NULL pointer detected " #expr);\
        return FALSE;\
     }
#endif

#define CHECK_HANDLE(handle)\
	if (NULL == handle || NULL == handle->privatedata){ \
		ALOGE("Handle is invalid " #handle); \
		return SENSOR_CTX_ERROR; \
	}

#define SENSOR_I2C_VAL_8BIT        0x00
#define SENSOR_I2C_VAL_16BIT       0x01
#define SENSOR_I2C_REG_8BIT        (0x00 << 1)
#define SENSOR_I2C_REG_16BIT       (0x01 << 1)
/*decompress*/
#define GAIN_ORIGIN_BITS           16
#define GAIN_COMPRESSED_12BITS     12
#define GAIN_COMPRESSED_14BITS     14
#define GAIN_MASK_12BITS           (0xfff)
#define GAIN_MASK_14BITS           (0x3fff)

enum otp_ctrl{
	READ_FORMAT_OTP_DATA,
	WRITE_FORMAT_OTP_DATA,
	READ_FORMAT_OTP_FROM_BIN,
	WRITE_FORMAT_OTP_TO_BIN,
	/*add*/
	RELEASE_OTP_DATA,
};

enum otp_compress_type{
	OTP_COMPRESSED_12BITS = 0,
	OTP_COMPRESSED_14BITS = 1,
	OTP_COMPRESSED_16BITS = 2,
};
enum otp_calibration_lsc_pattern {
	SENSOR_IMAGE_PATTERN_RGGB = 0,
	SENSOR_IMAGE_PATTERN_GRBG = 1,
	SENSOR_IMAGE_PATTERN_GBRG = 2,
	SENSOR_IMAGE_PATTERN_BGGR = 3,
};

enum awb_light_type
{
	AWB_OUTDOOR_SUNLIGHT =  0,  /* D65 */
	AWB_OUTDOOR_CLOUDY,         /* D75 */
#if 0
	AWB_INDOOR_INCANDESCENT,    /* A */
	AWB_INDOOR_WARM_FLO,        /* TL84 */
	AWB_INDOOR_COLD_FLO,        /* CW */
	AWB_HORIZON,                /* H */
	AWB_OUTDOOR_SUNLIGHT1,      /* D50 */
	AWB_INDOOR_CUSTOM_FLO,      /* CustFlo */
	AWB_OUTDOOR_NOON,           /* Noon */
	AWB_HYBRID,                 /* Daylight */
#endif
	AWB_MAX_LIGHT,
};

typedef struct{
	uint16_t  reg_addr;  /* otp start address.if read ,we don't need care it*/
	uint8_t   *data;     /* format otp data saved or otp data write to sensor*/
	uint32_t  num_bytes; /* if read ,we don't need care it*/
}otp_buffer_t;

typedef struct{
	uint8_t       cmd;     /* read or write*/
	void          *data;
}otp_ctrl_cmd_t;

struct wb_source_packet {
	uint16_t R;
	uint16_t GR;
	uint16_t GB;
	uint16_t B;
};

typedef struct{
	unsigned char moule_id;
	unsigned char vcm_id;
	unsigned char drvier_ic_id;
	unsigned char ir_bg_id;
}module_data_t;

/*
* AWB data will transmitted to ISP
*/
typedef struct{
	uint16_t R;
	uint16_t G;
	uint16_t B;
	/*sometime wb data is ratio */
	uint16_t rg_ratio;
	uint16_t bg_ratio;
}awb_target_packet_t;

typedef struct{
	uint16_t R;
	uint16_t GR;
	uint16_t GB;
	uint16_t B;
}awb_src_packet_t;

typedef struct {
	uint32_t wb_flag;
	awb_target_packet_t awb_gld_info[AWB_MAX_LIGHT];
	awb_target_packet_t awb_rdm_info[AWB_MAX_LIGHT];
}awbcalib_data_t;

typedef struct {
	uint16_t infinity_dac;
	uint16_t macro_dac;
	uint16_t afc_direction;
	uint16_t reserve;
}afcalib_data_t;

struct section_info {
	uint32_t offset;
	uint32_t length;
};

typedef struct {
	struct section_info lsc_calib_golden;
	struct section_info lsc_calib_random;
}lsccalib_data_t;

typedef struct{
	uint16_t x;
	uint16_t y;
}point_t;

typedef struct {
	point_t R;
	point_t GR;
	point_t GB;
	point_t B;
}optical_center_t;

/**
 * here include formate data
 * you can add some items if you need.
 **/
typedef struct {
	module_data_t    module_dat;
	afcalib_data_t   af_cali_dat;
	awbcalib_data_t  awb_cali_dat;
	optical_center_t opt_center_dat;
	void             *pdaf_cali_dat;     /*reserver*/
	void             *dual_cam_cali_dat; /*reserver*/
	lsccalib_data_t  lsc_cali_dat;
}otp_format_data_t;

typedef struct {
	char     dev_name[32];
	uint16_t reg_addr;
	uint8_t  *buffer;
	uint32_t num_bytes;
}otp_params_t;

/*otp driver file*/
typedef struct {
	int is_self_cal:1;

	int is_dul_cam_self_cal:1;
	int is_dul_camc:1;

	int is_awbc_self_cal :1;
	int is_awbc:1;

	int is_lsc_self_cal:1;
	int is_lsc:1;

	int is_pdaf_self_cal:1;
	int is_pdafc:1;
}otp_calib_items_t;
/*
 * here include base info include
 */
typedef struct{
	int  compress_flag;
	int  bayer_pattern;
	int  image_width;
	int  image_height;
	int  grid_width;
	int  grid_height;
	int  gain_width;
	int  gain_height;
}otp_base_info_cfg_t;

typedef struct{
	otp_calib_items_t cali_items;
	otp_base_info_cfg_t base_info_cfg;
}otp_config_t;
/*
 * if not supported some feature items,please set NULL
 */
typedef struct {
  uint32_t (*format_calibration_data)(SENSOR_HW_HANDLE handle);
  uint32_t (*awb_calibration)(SENSOR_HW_HANDLE handle);
  uint32_t (*lsc_calibration)(SENSOR_HW_HANDLE handle);
  uint32_t (*pdaf_calibration)(SENSOR_HW_HANDLE handle);
  uint32_t (*dul_cam_calibration)(SENSOR_HW_HANDLE handle);
  uint32_t (*read_otp_data)(SENSOR_HW_HANDLE handle);
  uint32_t (*write_otp_data)(SENSOR_HW_HANDLE handle);
}otp_ops_t;

typedef struct {
	otp_config_t otp_cfg;
	otp_ops_t    otp_ops;
}otp_drv_cxt_t;
typedef struct otp_data{
	otp_config_t       otp_cfg;
	otp_format_data_t  otp_format_data;
}otp_data_t;
typedef struct {
	otp_params_t        otp_params;   /*raw otp data buffer*/
	otp_drv_cxt_t       *otp_drv_cxt;
	otp_format_data_t   *otp_data;    /*format otp data*/
	uint32_t            otp_data_len; /*format otp data length*/
}otp_ctrl_cxt_t;

static uint32_t sensor_otp_calibration_data(SENSOR_HW_HANDLE handle);
static uint32_t sensor_otp_read_format_data(SENSOR_HW_HANDLE handle);
static uint32_t sensor_otp_rw_data_from_file(SENSOR_HW_HANDLE handle, uint8_t cmd);
static uint32_t sensor_otp_write_format_data(SENSOR_HW_HANDLE handle);
uint32_t sensor_otp_process(SENSOR_HW_HANDLE handle, int cmd, void *params);
static uint32_t sensor_otp_lsc_decompress(SENSOR_HW_HANDLE handle);
static uint32_t sensor_otp_decompress_gain(uint16_t *src, uint32_t src_bytes,
        uint32_t src_uncompensate_bytes, uint16_t *dst,uint32_t GAIN_COMPRESSED_BITS,uint32_t GAIN_MASK);
static void sensor_otp_change_pattern(uint32_t pattern, uint16_t *interlaced_gain, uint16_t *chn_gain[4], uint16_t gain_num);
uint32_t sensor_otp_dump_raw_data(SENSOR_HW_HANDLE handle);
#endif

