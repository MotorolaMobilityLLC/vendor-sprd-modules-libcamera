/*---------------------------------------------------------------*
 * SPRD general otp driver
 *  only for SPRD otp, not support 3rd party or sensor self otp
 *  compat otp v0.1, v0.2, v0.3, v0.4, v0.5, v1.0, v1.1
 *  compat single camera, dual camera
 *  compat master camera, slave camera
 *  compat rear camera, front camera
 *  compat bokeh, w+t, spw
 *---------------------------------------------------------------*
 *  compat otp maps below
 *  path: \\shplm06\01-Unisoc Camera OTP 标准\OTP map
 *
 *  otp map 0.4/0.5:
 *        5M_Single Module_memory map_V0.4_2017801.xlsx
 *        8M_Single Module_memory map_V0.4_2017803.xlsx
 *        13M_Single Module_memory map_V0.4_20180103.xlsx
 *
 *    ov8858+ov2680 sbs
 *        8M+2M_dualcam_ONE memory map_V0.4_20170801.xlsx
 *        8M+2M_dualcam_ONE memory map_V0.5_20171228.xlsx
 *
 *    ov13855+ov5675, not include arcsoft dualcam data after addr 0x00001000
 *        13M+5M_dualcam_separate memory map_V0.4_20170901.xlsx
 *
 *  otp map 1.0:
 *        0x0c00_12M_Single Module_memory map_V1.0_20171019.xlsx
 *        0x0d0001_13M_Single Module_memory map_V1.0_20180427.xlsx
 *        0x100001_16M(Include PDAF)_ONE memory map_V1.0_20180309.xlsx
 *
 *    ov16885, lsc 4M, not include 2 new sections - Cross Talk and DPC
 *        0xb00001_16M(Include 4in1)_ONE memory map_V1.0_20180518.xlsx
 *
 *    dualcam_version 1, size 255
 *        0x0802_8M+2M_dualcam_ONE memory map_V1.0_20171214.xlsx
 *
 *    dualcam_version 2, size 256
 *        0x080501_8M+5M_dualcam_ONE memory map_V1.0_20180307.xlsx
 *        0x0c0501_12M+5M_dualcam_ONE memory map_V1.0_20180820.xlsx
 *        0x0d020001_13M+2M_dualcam_ONE memory map_V1.0_20180907.xlsx
 *
 *      ov13855+ov5675
 *        0x0d0501_13M+5M_dualcam_ONE memory map_V1.0_20180103.xlsx
 *
 *      imx351+ov5675
 *        0x100501_16M+5M_dualcam_ONE memory map_V1.0_20180403.xlsx
 *        0x100502_16M+5M_dualcam_TWO memory map_V1.0_20180517.xlsx
 *
 *      imx351+ov8856
 *        0x100801_16M+8M_dualcam_TWO memory map_V1.0_20180105.xlsx
 *
 *  otp map 1.1
 *        0x20100800_3EEPROM_16KB_6528x4896(OV32A1Q)+8KB_4672x3504(OV16885)
 *         +8KB_3264x2448(OV8856)_V1.1_20190507.xlsx
 *---------------------------------------------------------------*/

#define LOG_TAG "general_otp_drv"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "otp_common.h"
#include "cmr_sensor_info.h"

static cmr_int _general_otp_section_checksum(cmr_u8 *buffer, cmr_uint offset,
                                             cmr_uint size,
                                             cmr_uint checksum_offset,
                                             enum otp_version_t otp_version);
static cmr_int _general_otp_get_lsc_channel_size(cmr_u16 width, cmr_u16 height,
                                                 cmr_u8 grid);
static cmr_int _general_otp_parse_map_version(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_module_data_0v4(cmr_handle otp_drv_handle,
                                                  cmr_u16 module_info_offset);
static cmr_int _general_otp_get_single_addr_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_get_master_addr_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_get_slave_addr_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_module_data_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_module_data_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_af_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_af_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_af_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_af_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_af_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_awb_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_awb_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_awb_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_awb_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_awb_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_lsc_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_lsc_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_lsc_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_lsc_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_lsc_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_pdaf(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_pdaf_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_xtalk_4in1_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_dpc_4in1_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_spw_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_ae_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_ae_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_ae_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_slave_ae_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_dualcam_0v4(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_master_dualcam_1v0(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_bokeh_1v1(cmr_handle otp_drv_handle);
static cmr_int _general_otp_parse_wt_1v1(cmr_handle otp_drv_handle);

static cmr_int _general_otp_compatible_convert_single(cmr_handle otp_drv_handle,
                                                      void *p_data);
static cmr_int _general_otp_compatible_convert_master(cmr_handle otp_drv_handle,
                                                      void *p_data);
static cmr_int _general_otp_compatible_convert_slave(cmr_handle otp_drv_handle,
                                                     void *p_data);

static cmr_int general_otp_drv_create(otp_drv_init_para_t *input_para,
                                      cmr_handle *otp_drv_handle);
static cmr_int general_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int general_otp_drv_read(cmr_handle otp_drv_handle, void *param);
static cmr_int general_otp_drv_write(cmr_handle otp_drv_handle, void *param);
static cmr_int general_otp_drv_parse(cmr_handle otp_drv_handle, void *param);
static cmr_int general_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int general_otp_compatible_convert(cmr_handle otp_drv_handle,
                                              void *p_data);
static cmr_int general_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                     void *param);

otp_drv_entry_t general_otp_entry = {
    .otp_ops =
        {
            .sensor_otp_create = general_otp_drv_create,
            .sensor_otp_delete = general_otp_drv_delete,
            .sensor_otp_read = general_otp_drv_read,
            .sensor_otp_write = general_otp_drv_write,
            .sensor_otp_parse = general_otp_drv_parse,
            .sensor_otp_calibration = general_otp_drv_calibration,
            .sensor_otp_ioctl = general_otp_drv_ioctl, /*expand*/
        },
};
