#ifndef _OTP_CALI_H_
#define _OTP_CALI_H_

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cmr_types.h"
#include "cmr_type.h"
#include "cmr_log.h"
#include "cmr_sensor_info.h"

#define CALI_OTP_HEAD_SIZE 32

#define CALI_OTP_FILE_PATH_LENGTH 512

#define OTP_HEAD_MAGIC 0x00004e56
#define OTP_VERSION1 1 // calibration data structure, header+data
#define OTP_VERSION2 2 // calibration data structure, header+data+cmei
#define OTPDATA_TYPE_CALIBRATION (1)
#define OTPDATA_TYPE_GOLDEN (2)

typedef struct {
    cmr_u32 magic;
    cmr_u16 otp_len;
    cmr_u32 otp_checksum;
    cmr_u32 version;
    cmr_u8 data_type;
    cmr_u8 has_calibration;
    cmr_u16 cmei_len;
} otp_header_t;

enum calibration_flag {
    CALIBRATION_FLAG_BOKEH = 1,
    CALIBRATION_FLAG_T_W,
    CALIBRATION_FLAG_SPW,
    CALIBRATION_FLAG_3D_STL,
    CALIBRATION_FLAG_OZ1,
    CALIBRATION_FLAG_OZ2,
    CALIBRATION_FLAG_BOKEH_GLD, //For bokeh after-sale plan B, 7 times calibration data
    CALIBRATION_FLAG_BOKEH_GLD2, //For bokeh after-sale golden plan, 1 time calibration data
    CALIBRATION_FLAG_OZ1_GLD,
    CALIBRATION_FLAG_OZ2_GLD,
    CALIBRATION_FLAG_BOKEH_MANUAL_CMEI,
    CALIBRATION_FLAG_MAX
};

#define OTP_CALI_BOKEH_PATH "/data/vendor/local/otpdata/otp_cali_bokeh.bin"
#define OTPBK_CALI_BOKEH_PATH                                                  \
    "/mnt/vendor/productinfo/otpdata/otpbk_cali_bokeh.bin"

#define OTP_CALI_T_W_PATH "/data/vendor/local/otpdata/otp_cali_t_w.bin"
#define OTPBK_CALI_T_W_PATH "/mnt/vendor/productinfo/otpdata/otpbk_cali_t_w.bin"

#define OTP_CALI_SPW_PATH "/data/vendor/local/otpdata/otp_cali_spw.bin"
#define OTPBK_CALI_SPW_PATH "/mnt/vendor/productinfo/otpdata/otpbk_cali_spw.bin"

#define OTP_CALI_3D_STL_PATH "/data/vendor/local/otpdata/otp_cali_3d_stl.bin"
#define OTPBK_CALI_3D_STL_PATH                                                 \
    "/mnt/vendor/productinfo/otpdata/otpbk_cali_3d_stl.bin"

#define OTP_CALI_OZ1_PATH "/data/vendor/local/otpdata/otp_cali_oz1.bin"
#define OTPBK_CALI_OZ1_PATH "/mnt/vendor/productinfo/otpdata/otpbk_cali_oz1.bin"

#define OTP_CALI_OZ2_PATH "/data/vendor/local/otpdata/otp_cali_oz2.bin"
#define OTPBK_CALI_OZ2_PATH "/mnt/vendor/productinfo/otpdata/otpbk_cali_oz2.bin"

#define OTP_CALI_BOKEH_GLD_PATH "/vendor/etc/otpdata/otp_cali_bokeh_gld.bin"
#define OTP_CALI_BOKEH_GLD2_PATH "/vendor/etc/otpdata/otp_cali_bokeh_gld2.bin"
#define OTP_CALI_OZ1_GLD_PATH "/vendor/etc/otpdata/otp_cali_oz1_gld.bin"
#define OTP_CALI_OZ2_GLD_PATH "/vendor/etc/otpdata/otp_cali_oz2_gld.bin"

#define OTP_CALI_BOKEH_MANUAL_CMEI_PATH \
    "/data/vendor/local/otpdata/otp_cali_bokeh_manual_cmei.bin"
#define OTPBK_CALI_BOKEH_MANUAL_CMEI_PATH \
    "/mnt/vendor/productinfo/otpdata/otpbk_cali_bokeh_manual_cmei.bin"
cmr_u16 read_calibration_cmei(cmr_u8 dual_flag,
                      cmr_u8 *cmei_buf);
cmr_u16 read_calibration_otp(cmr_u8 dual_flag, cmr_u8 *otp_buf,
                                                  cmr_u8 *multi_module_name);
cmr_u8 write_calibration_otp_with_cmei(cmr_u8 dual_flag, cmr_u8 *otp_buf,
                                     cmr_u16 otp_size, cmr_u8 *cmei_buf, cmr_u16 cmei_size);
cmr_u8 write_calibration_otp_no_cmei(cmr_u8 dual_flag, cmr_u8 *otp_buf,
                                     cmr_u16 otp_size);

#endif
