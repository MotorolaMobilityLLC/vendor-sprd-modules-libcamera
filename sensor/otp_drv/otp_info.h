#ifndef _OTP_INFO_H_
#define _OTP_INFO_H_

#include "cmr_common.h"
#include "cmr_types.h"
#include <cutils/properties.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
#define OTP_RESERVE_BUFFER 64
#define CHANNAL_NUM 4

#define OTP_LOGI CMR_LOGI
#define OTP_LOGD CMR_LOGD
#define OTP_LOGE CMR_LOGE
#define OTP_LOGV CMR_LOGV
#define OTP_CAMERA_SUCCESS CMR_CAMERA_SUCCESS
#define OTP_CAMERA_FAIL CMR_CAMERA_FAIL

#ifndef CHECK_PTR
#define CHECK_PTR(expr)                                                        \
    if ((expr) == NULL) {                                                      \
        ALOGE("ERROR: NULL pointer detected " #expr);                          \
        return FALSE;                                                          \
    }
#endif

#define SENSOR_I2C_VAL_8BIT 0x00
#define SENSOR_I2C_VAL_16BIT 0x01
#define SENSOR_I2C_REG_8BIT (0x00 << 1)
#define SENSOR_I2C_REG_16BIT (0x01 << 1)
/*decompress*/
#define GAIN_ORIGIN_BITS 16
#define GAIN_COMPRESSED_12BITS 12
#define GAIN_COMPRESSED_14BITS 14
#define GAIN_MASK_12BITS (0xfff)
#define GAIN_MASK_14BITS (0x3fff)

enum otp_version_t {
    VER_ERROR = 0,
    OTP_0_1 = 1,  /*otp 0.1*/
    OTP_0_2 = 2,  /*otp 0.2*/
    OTP_0_3 = 3,  /*otp 0.3*/
    OTP_0_4 = 4,  /*otp 0.4*/
    OTP_0_5 = 5,  /*otp 0.5*/
    OTP_1_0 = 10, /*otp 1.0*/
    OTP_1_1 = 11, /*otp 1.1*/
};

enum eeprom_num_t {
    SINGLE_CAM_ONE_EEPROM = 0,
    DUAL_CAM_ONE_EEPROM = 1,
    DUAL_CAM_TWO_EEPROM = 2,
    MULTICAM_INDEPENDENT_EEPROM = 3,
};

enum otp_main_cmd {
    OTP_READ_RAW_DATA,
    OTP_READ_PARSE_DATA,
    OTP_WRITE_DATA,
    OTP_IOCTL,
};
enum otp_sub_cmd {
    OTP_RESERVE, /*just reserve*/
    OTP_READ_FORMAT_FROM_BIN,
    OTP_WRITE_FORMAT_TO_BIN,
    OTP_DATA_COMPATIBLE_CONVERT,
    /*expand,you can add your */
};
enum otp_compress_type {
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

enum awb_light_type {
    AWB_OUTDOOR_SUNLIGHT = 0, /* D65 */
    AWB_OUTDOOR_CLOUDY,       /* D75 */
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

typedef enum otp_vendor { OTP_SUNNY = 0, OTP_TRULY, OTP_MAX } otp_vendor_t;

typedef enum otp_third_cali {
    OTP_CALI_SPRD = 0,
    OTP_CALI_ARCSOFT,
    OTP_CALI_ALTEK,
    OTP_CALI_MAX
} otp_third_cali_t;

typedef struct {
    cmr_u16 reg_addr;  /* otp start address.if read ,we don't need care it*/
    cmr_u8 *data;      /* format otp data saved or otp data write to sensor*/
    cmr_u32 num_bytes; /* if read ,we don't need care it*/
} otp_buffer_t;

/* ctrl data*/
typedef struct {
    cmr_u8 cmd;
    cmr_u8 sub_cmd;
    void *data;
} otp_ctrl_cmd_t;

struct wb_source_packet {
    cmr_u16 R;
    cmr_u16 GR;
    cmr_u16 GB;
    cmr_u16 B;
};

typedef struct {
    cmr_u8 year;
    cmr_u8 month;
    cmr_u8 day;
    cmr_u32 moule_id;
    cmr_u8 vcm_id;
    cmr_u16 vendor_id;
    cmr_u8 drvier_ic_id;
    cmr_u8 ir_bg_id;
    cmr_u8 lens_id;
    /*for ov13885_sunny*/
    cmr_u8 env_record;
    cmr_u16 work_stat_id;
    cmr_u16 calib_version;
} module_data_t;

typedef struct otp_data_info {
    cmr_u32 size;
    void *buffer;
} otp_data_info_t;
/*
* AWB data will transmitted to ISP
*/
typedef struct {
    cmr_u16 R;
    cmr_u16 G;
    cmr_u16 B;
    /*sometime wb data is ratio */
    cmr_u16 rg_ratio;
    cmr_u16 bg_ratio;
    cmr_u16 GrGb_ratio;
} awb_target_packet_t;

typedef struct {
    cmr_u16 R;
    cmr_u16 GR;
    cmr_u16 GB;
    cmr_u16 B;
} awb_src_packet_t;

typedef struct {
    uint32_t wb_flag;
    awb_target_packet_t awb_gld_info[AWB_MAX_LIGHT];
    awb_target_packet_t awb_rdm_info[AWB_MAX_LIGHT];
} awbcalib_data_t;

typedef struct {
    cmr_u16 target_lum;
    cmr_u64 gain_1x_exp;
    cmr_u64 gain_2x_exp;
    cmr_u64 gain_4x_exp;
    cmr_u64 gain_8x_exp;
    cmr_u64 reserve;
} aecalib_data_t;

typedef struct {
    cmr_u16 infinity_dac;
    cmr_u16 macro_dac;
    cmr_u16 afc_direction;
    cmr_s32 vcm_step;
    cmr_u16 vcm_step_min;
    cmr_u16 vcm_step_max;
    cmr_u16 reserve;
} afcalib_data_t;

struct section_info {
    cmr_u32 offset;
    cmr_u32 length;
};

typedef struct {
    struct section_info lsc_calib_golden;
    struct section_info lsc_calib_random;
} lsccalib_data_t;

typedef struct {
    cmr_u16 x;
    cmr_u16 y;
} point_t;

typedef struct {
    point_t R;
    point_t GR;
    point_t GB;
    point_t B;
} optical_center_t;
typedef struct {
    cmr_u8 program_flag;
    cmr_u8 af_flag;
    cmr_u8 pdaf_flag;
    cmr_u16 checksum;
    /*you can add some items here*/
} extended_data_t;

typedef struct {
    otp_data_info_t rdm_info;
    otp_data_info_t gld_info;
} otp_section_info_t;

/**
 * here include formate data
 * you can add some items if you need.
 * |------------------------------------|
 * |    module_data_t module_dat;       |
 * |                - - -               |
 * |                - - -               |
 * |                - - -               |
 * | optical_center_t opt_center_dat;   |
 * | otp_data_info_t pdaf_cali_dat;     |
 * |                - - -               |
 * |                - - -               |
 * |                - - -               |
 * |   lsccalib_data_t lsc_cali_dat;    |
 * |------------------------------------|
 * | ^         cmr_u8 data[4];          |
 * | |                                  |
 * | | the buffer maybe include otp lsc |
 * | | ,3D_cal data buffer etc,if need. |
 * | | You can get buffer address       |
 * | | by data[4] pointer.              |
 * | |                                  |
 * | | NOTE: the buffer size depends on |
 * | V your otp driver.                 |
 * |------------------------------------|
 **/
typedef struct {
    otp_section_info_t module_dat;
    cmr_u16 iso_dat;
    enum otp_vendor_type otp_vendor;
    otp_section_info_t af_cali_dat;
    otp_section_info_t ae_cali_dat;
    otp_section_info_t awb_cali_dat;
    otp_section_info_t opt_center_dat;
    otp_section_info_t pdaf_cali_dat;
    /*spc:sensor pixel calibration,used by pdaf*/
    otp_section_info_t spc_cali_dat;

    otp_section_info_t xtalk_4in1_cali_dat;
    otp_section_info_t dpc_4in1_cali_dat;
    otp_section_info_t spw_cali_dat;      /*superwide*/
    otp_section_info_t dual_cam_cali_dat; /*bokeh*/
    otp_section_info_t wt_cali_dat;       /*w+t:optics_zoom*/
    extended_data_t extend_dat;
    otp_section_info_t lsc_cali_dat;
    otp_section_info_t third_cali_dat;
    cmr_u8 data[4]; /*must be last*/
} otp_format_data_t;

typedef struct {
    cmr_u16 reg_addr;
    cmr_u8 *buffer;
    cmr_u32 num_bytes;
} otp_params_t;

/*otp driver file*/
typedef struct {
    /* 1:calibration at sensor side,
     * 0:calibration at isp
     */
    cmr_uint is_self_cal : 1;

    /* support dual camera self calibration */
    cmr_uint is_dul_cam_self_cal : 1;
    cmr_uint is_dul_camc : 1;

    /* support awb self calibration */
    cmr_uint is_awbc_self_cal : 1;
    /* support awb calibration */
    cmr_uint is_awbc : 1;

    /* support lens shadding self calibration */
    cmr_uint is_lsc_self_cal : 1;
    /* support lens shadding calibration */
    cmr_uint is_lsc : 1;

    /* support pdaf self calibration */
    cmr_uint is_pdaf_self_cal : 1;
    /* support pdaf calibration */
    cmr_uint is_pdafc : 1;
    /* support af self calibration */
    cmr_uint is_af_self_cal : 1;
    /* support af calibration */
    cmr_uint is_afc : 1;
} otp_calib_items_t;
/*
 * here include base info include
 */
typedef struct {
    /*decompression on otp driver or isp*/
    cmr_uint is_lsc_drv_decompression;

    /*otp data compressed format,should confirm with sensor
     *fae
     */
    cmr_uint compress_flag;

    /*sensor bayer pattern*/
    cmr_uint bayer_pattern;

    /*the width of the stream the sensor can output*/
    cmr_uint image_width;

    /*the height of the stream the sensor can output*/
    cmr_uint image_height;

    /*lsc otp grid: 16M-128, 13M/8M-96, 5M-64, 2M/0.5M-32*/
    cmr_u32 lsc_otp_grid;
    cmr_u32 full_img_width;
    cmr_u32 full_img_height;
    cmr_uint grid_width;
    cmr_uint grid_height;
    cmr_uint gain_width;
    cmr_uint gain_height;
} otp_base_info_cfg_t;

typedef struct {
    otp_calib_items_t cali_items;
    otp_base_info_cfg_t base_info_cfg;
} otp_config_t;

typedef struct {
    cmr_uint is_depend_relation;
    cmr_uint depend_sensor_id;
} otp_depend_sensor_t;

typedef struct otp_drv_init_para {
    cmr_handle hw_handle;
    char *sensor_name;
    cmr_u32 sensor_id;
    cmr_u8 sensor_ic_addr;
    /*you can add your param here*/
    cmr_u8 eeprom_i2c_addr;
    enum eeprom_num_t eeprom_num;
    cmr_u32 eeprom_size; /* unit: Byte */
    cmr_u16 sensor_max_width;
    cmr_u16 sensor_max_height;
} otp_drv_init_para_t;

/*
 * if not supported some feature items,please set NULL
 */
typedef struct {
    cmr_int (*sensor_otp_create)(otp_drv_init_para_t *input_para,
                                 cmr_handle *sns_af_drv_handle);
    cmr_int (*sensor_otp_delete)(cmr_handle otp_drv_handle);
    cmr_int (*sensor_otp_read)(cmr_handle otp_drv_handle, void *param);
    cmr_int (*sensor_otp_write)(cmr_handle otp_drv_handle, void *param);
    cmr_int (*sensor_otp_parse)(cmr_handle otp_drv_handle, void *param);
    cmr_int (*sensor_otp_calibration)(cmr_handle otp_drv_handle);
    cmr_int (*sensor_otp_ioctl)(cmr_handle otp_drv_handle, cmr_uint cmd,
                                void *p_params); /*expend*/
} sensor_otp_ops_t;

typedef struct {
    otp_config_t otp_cfg;
    sensor_otp_ops_t otp_ops;
    otp_depend_sensor_t otp_dep;
} otp_drv_entry_t;

typedef struct {
    otp_drv_entry_t *otp_drv_entry;
    cmr_u8 eeprom_i2c_addr;
    enum eeprom_num_t eeprom_num;
    cmr_u32 eeprom_size; /* unit: Byte */
} otp_drv_info_t;

/*==================================================
              for otp v1.0 and v1.1 start
====================================================*/
struct module_id_info_t {
    cmr_u8 master_vendor_id;
    cmr_u8 master_lens_id;
    cmr_u8 master_vcm_id;
    cmr_u8 slave_vendor_id;
    cmr_u8 slave_lens_id;
    cmr_u8 slave_vcm_id;
    cmr_u8 year;
    cmr_u8 month;
    cmr_u8 day;
    cmr_u16 work_station_id;
    cmr_u16 env_record;
    /*for otp v1.1*/
    cmr_u8 sensor_num;
};

struct sensor_setting_t {
    cmr_u8 eeprom_info;
    cmr_u8 master_setting;
    cmr_u8 master_ob;
    cmr_u8 slave_setting;
    cmr_u8 slave_ob;
};

struct section_info_t {
    cmr_u16 offset;
    cmr_u16 size;
};

struct module_info_t {
    cmr_u32 otp_tag;
    cmr_u16 calib_version;
    enum otp_version_t otp_version;
    cmr_u32 otp_map_index;
    struct module_id_info_t module_id_info;
    struct sensor_setting_t sensor_setting;
    struct section_info_t master_af_info;
    struct section_info_t master_awb_info;
    struct section_info_t master_lsc_info;
    struct section_info_t master_pdaf1_info;
    struct section_info_t master_pdaf2_info;
    struct section_info_t master_ae_info;
    struct section_info_t master_xtalk_4in1_info;
    struct section_info_t master_dpc_4in1_info;
    struct section_info_t master_spw_info;
    struct section_info_t master_bokeh_info;
    struct section_info_t master_wt_info;
    struct section_info_t slave_af_info;
    struct section_info_t slave_awb_info;
    struct section_info_t slave_lsc_info;
    struct section_info_t slave_pdaf1_info;
    struct section_info_t slave_pdaf2_info;
    struct section_info_t slave_ae_info;
    struct section_info_t slave_xtalk_4in1_info;
    struct section_info_t slave_dpc_4in1_info;
    struct section_info_t slave_spw_info;
    struct section_info_t slave_bokeh_info;
    struct section_info_t slave_wt_info;
    cmr_u16 sensor_max_width;
    cmr_u16 sensor_max_height;
    cmr_u8 lsc_grid;
    cmr_u32 resolution;
};

struct af_data_t {
    cmr_u8 af_version;
    cmr_u16 af_infinity_position;
    cmr_u16 af_macro_position;
    cmr_u8 af_posture;
    cmr_u16 af_temperature_start;
    cmr_u16 af_temperature_end;
};

struct awb_data_t {
    cmr_u8 awb_version;
    cmr_u16 awb_random_r;
    cmr_u16 awb_random_g;
    cmr_u16 awb_random_b;
    cmr_u16 awb_golden_r;
    cmr_u16 awb_golden_g;
    cmr_u16 awb_golden_b;
};

struct lsc_data_t {
    cmr_u8 lsc_version;
    cmr_u16 lsc_oc_r_x;
    cmr_u16 lsc_oc_r_y;
    cmr_u16 lsc_oc_gr_x;
    cmr_u16 lsc_oc_gr_y;
    cmr_u16 lsc_oc_gb_x;
    cmr_u16 lsc_oc_gb_y;
    cmr_u16 lsc_oc_b_x;
    cmr_u16 lsc_oc_b_y;
    cmr_u16 lsc_img_width;
    cmr_u16 lsc_img_height;
    cmr_u8 lsc_grid;
    cmr_u32 resolution;
    cmr_u32 lsc_channel_size;
    cmr_u32 lsc_width;
    cmr_u32 lsc_height;
};

struct ae_data_t {
    cmr_u8 ae_version;
    cmr_u16 ae_target_lum;
    cmr_u32 ae_gain_1x_exp;
    cmr_u32 ae_gain_2x_exp;
    cmr_u32 ae_gain_4x_exp;
    cmr_u32 ae_gain_8x_exp;
};

struct dualcam_data_t {
    cmr_u8 dualcam_version;
    cmr_u16 dualcam_vcm_position;
    cmr_u8 dualcam_location;
};
/*==================================================
              for otp v1.0 and v1.1 end
====================================================*/

typedef struct {
    cmr_u32 sensor_id;
    char dev_name[32];
    /*sensor ic i2c address*/
    cmr_u8 sensor_ic_addr;

    cmr_u8 eeprom_i2c_addr;
    enum eeprom_num_t eeprom_num;
    cmr_u32 eeprom_size; /* unit: Byte */

    cmr_handle hw_handle;

    /*raw otp data buffer*/
    otp_params_t otp_raw_data;

    /*format otp data*/
    otp_format_data_t *otp_data;

    /*format otp data length*/
    uint32_t otp_data_len;
    void *compat_convert_data;

    cmr_uint otp_data_module_index;

    struct module_info_t otp_module_info;
} otp_drv_cxt_t;

struct otp_drv_lib {
    void *otp_lib_handle;
    otp_drv_entry_t *otp_info_ptr;
};

#endif
