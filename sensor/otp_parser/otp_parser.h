#ifndef _OTP_PARSER_H_
#define _OTP_PARSER_H_

#include <sys/types.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include "cmr_types.h"
#include "cmr_log.h"

enum otp_eeprom_num {
    OTP_EEPROM_SINGLE = 0,      /*single camera with one eeprom*/
    OTP_EEPROM_SINGLE_CAM_DUAL, /*dual camera share one eeprom, v0.4 and v1.0*/
    OTP_EEPROM_DUAL_CAM_DUAL, /*dual camera each has one eeprom, v0.4 and v1.0*/
    OTP_EEPROM_INDEPENDENT,   /*multi camera with eeprom individually, v1.1*/
    OTP_EEPROM_MAX
};

struct otp_parser_section_info {
    cmr_u32 data_size;
    cmr_u32 data_offset;
};

struct otp_parser_camera_info {
    cmr_uint has_parsered;
    struct otp_parser_section_info af_info;
    struct otp_parser_section_info awb_info;
    struct otp_parser_section_info lsc_info;
    struct otp_parser_section_info pdaf1_info;
    struct otp_parser_section_info pdaf2_info;
    struct otp_parser_section_info xtalk_4in1_info;
    struct otp_parser_section_info dpc_4in1_info;
    struct otp_parser_section_info spw_info;
    struct otp_parser_section_info bokeh_info;
    struct otp_parser_section_info wt_info;
    struct otp_parser_section_info ae_info;
};

enum otp_parser_cmd {
    OTP_PARSER_MAP_VERSION,
    OTP_PARSER_AE,
    OTP_PARSER_AF,
    OTP_PARSER_AWB,
    OTP_PARSER_LSC,
    OTP_PARSER_PDAF1,
    OTP_PARSER_SECTION_PDAF1,
    OTP_PARSER_SECTION_PDAF2,
    OTP_PARSER_SECTION_XTALK_4IN1,
    OTP_PARSER_SECTION_DPC_4IN1,
    OTP_PARSER_SECTION_SPW,
    OTP_PARSER_SECTION_BOKEH,
    OTP_PARSER_SECTION_WT,
    OTP_PARSER_SECTION_AF,
    OTP_PARSER_SECTION_AWB,
    OTP_PARSER_SECTION_LSC,
    OTP_PARSER_MAX,
};

enum otp_parser_return_value {
    OTP_PARSER_SUCCESS = 0x00,
    OTP_PARSER_PARAM_ERR,
    OTP_PARSER_CHECKSUM_ERR,
    OTP_PARSER_VERSION_ERR,
    OTP_PARSER_CMD_ERR,
    OTP_PARSER_RTN_MAX
};

struct otp_parser_map_version {
    cmr_u16 version;
};

struct otp_parser_section_with_version {
    cmr_u8 version;
    cmr_u8 *data_addr;
    cmr_u32 data_size;
};

struct otp_parser_section {
    cmr_u8 *data_addr;
    cmr_u32 data_size;
};

struct otp_parser_af_data {
    cmr_u8 version;
    cmr_u16 inf_position;
    cmr_u16 macro_position;
    cmr_u8 posture;
    cmr_u16 temperature1;
    cmr_u16 temperature2;
};

struct otp_parser_awb_data {
    cmr_u8 version;
    cmr_u16 awb1_r_gain;
    cmr_u16 awb1_g_gain;
    cmr_u16 awb1_b_gain;
    cmr_u16 awb2_r_gain;
    cmr_u16 awb2_g_gain;
    cmr_u16 awb2_b_gain;
};

struct otp_parser_lsc_data {
    cmr_u8 version;
    cmr_u16 oc_r_x;
    cmr_u16 oc_r_y;
    cmr_u16 oc_gr_x;
    cmr_u16 oc_gr_y;
    cmr_u16 oc_gb_x;
    cmr_u16 oc_gb_y;
    cmr_u16 oc_b_x;
    cmr_u16 oc_b_y;
    cmr_u16 sensor_resolution_h;
    cmr_u16 sensor_resolution_v;
    cmr_u8 lsc_grid;
    cmr_u8 *lsc_table_addr;
    cmr_u32 lsc_table_size;
};

struct otp_parser_pdaf1_data {
    cmr_u8 version;
    cmr_u8 *pdaf1_data_addr;
    cmr_u32 pdaf1_data_size;
};

struct otp_parser_pdaf2_data {
    cmr_u8 *pdaf2_data_addr;
    cmr_u32 pdaf2_data_size;
};

struct otp_parser_ae_data {
    cmr_u8 version;
    cmr_u16 target_lum;
    cmr_u32 exp_1x_gain;
    cmr_u32 exp_2x_gain;
    cmr_u32 exp_4x_gain;
    cmr_u32 exp_8x_gain;
};

struct otp_parser_dualcam_for_sprd_3rd {
    cmr_u8 version;
    cmr_u8 dual_flag;
    cmr_u8 *data_3d_data;
    cmr_u32 data_3d_data_size;
};

cmr_int otp_parser(void *raw_data, enum otp_parser_cmd cmd, cmr_uint eeprom_num,
                   cmr_int camera_id, cmr_int raw_height, cmr_int raw_width,
                   void *result);
cmr_int otp_parser_v1(void *raw_data, enum otp_parser_cmd cmd,
                      cmr_int camera_id, void *result);
#endif
