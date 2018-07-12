#ifndef _OTP_PARSER_H_
#define _OTP_PARSER_H_

#include <sys/types.h>
#include "cmr_types.h"
#include "cmr_log.h"
#include <utils/Log.h>

enum otp_eeprom_num {
    OTP_EEPROM_NONE = 0,        /*CAMERA NOT SOPPROT OTP*/
    OTP_EEPROM_SINGLE,          /*ONE CAMERA SENSOR ONE OTP*/
    OTP_EEPROM_SINGLE_CAM_DUAL, /*TWO CAMERA SENSOR AND ONE OTP,JUST FOR DUAL CAMERA*/
    OTP_EEPROM_DUAL_CAM_DUAL,   /*TWO CAMERA SENSOR AND TWO OTP,JUST FOR DUAL CAMERA*/
    OTP_EEPROM_MAX
};

struct otp_parser_section_info {
    cmr_u32 data_size;
    cmr_u32 data_offset;
};

struct otp_parser_camera_info {
    cmr_uint has_parsered;
    struct otp_parser_section_info  af_info;
    struct otp_parser_section_info  awb_info;
    struct otp_parser_section_info  lsc_info;
    struct otp_parser_section_info  pdaf_sprd_info;
    struct otp_parser_section_info  pdaf_sensor_vendor_info;
    struct otp_parser_section_info  dualcam_info;
    struct otp_parser_section_info  ae_info;
};

enum otp_parser_return_value {
    OTP_PARSER_SUCCESS = 0x00,
    OTP_PARSER_PARAM_ERR,
    OTP_PARSER_CHECKSUM_ERR,
    OTP_PARSER_VERSION_ERR,
    OTP_PARSER_CMD_ERR,
    OTP_PARSER_RTN_MAX
};

struct otp_parser_map_version{
    cmr_u16 version;
};

struct otp_parser_af_data{
    cmr_u8 version;
    cmr_u16 inf_position;
    cmr_u16 macro_position;
    cmr_u8 posture;
    cmr_u16 temperature1;
    cmr_u16 temperature2;
};

struct otp_parser_awb_data{
    cmr_u8 version;
    cmr_u16 awb1_r_gain;
    cmr_u16 awb1_g_gain;
    cmr_u16 awb1_b_gain;
    cmr_u16 awb2_r_gain;
    cmr_u16 awb2_g_gain;
    cmr_u16 awb2_b_gain;
};

struct otp_parser_lsc_data{
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

struct otp_parser_pdaf_for_sprd_3rd{
    cmr_u8 version;
    cmr_u8 *pdaf_data_addr;
    cmr_u32 pdaf_data_size;
};

struct otp_parser_pdaf_spc_from_sensor_vendor{
    cmr_u8 version;
    cmr_u8 *pdaf_spc_vendor_data_addr;
    cmr_u32 pdaf_spc_vendor_data_size;
};

struct otp_parser_ae_data{
    cmr_u8 version;
    cmr_u16 target_lum;
    cmr_u32 exp_1x_gain;
    cmr_u32 exp_2x_gain;
    cmr_u32 exp_4x_gain;
    cmr_u32 exp_8x_gain;
};

struct otp_parser_dualcam_for_sprd_3rd{
    cmr_u8 version;
    cmr_u8 dual_flag;
    cmr_u8 *data_3d_data;
    cmr_u32 data_3d_data_size;
};

struct otp_parser_cross_talk_data{
    cmr_u8 version;
    cmr_u8 *cross_talk_data;
    cmr_u32 cross_talk_size;
};

struct otp_parser_dpc_data{
    cmr_u8 version;
    cmr_u8 *level_Cali_Table_1;
    cmr_u32 level_Cali_Table_1_size;
};

enum otp_parser_cmd{
    OTP_PARSER_MAP_VERSION,
    OTP_PARSER_AE,
    OTP_PARSER_AF,
    OTP_PARSER_AWB,
    OTP_PARSER_LSC,
    OTP_PARSER_PDAF_SPRD,
    OTP_PARSER_PDAF_SPC_SENSOR_VENDOR,
    OTP_PARSER_DUAL_CAMERA,
    OTP_PARSER_CROSS_TALK,
    OTP_PARSER_DPC,
    OTP_PARSER_MAX,
};

cmr_int otp_parser(void * raw_data, enum otp_parser_cmd cmd, cmr_uint eeprom_num, cmr_int camera_id, cmr_int raw_height, cmr_int raw_width, void *result);
#endif
