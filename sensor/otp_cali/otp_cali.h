#ifndef _OTP_CALI_H_
#define _OTP_CALI_H_

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cmr_types.h"
#include "cmr_type.h"
#include "cmr_log.h"

#define CALI_OTP_HEAD_SIZE 32

#define OTP_HEAD_MAGIC 0x00004e56
#define OTP_VERSION 001
#define OTPDATA_TYPE_CALIBRATION (1)
#define OTPDATA_TYPE_GOLDEN (2)

typedef struct {
    cmr_u32 magic;
    cmr_u16 len;
    cmr_u32 checksum;
    cmr_u32 version;
    cmr_u8 data_type;
    cmr_u8 has_calibration;
} otp_header_t;

enum calibration_flag {
    CALIBRATION_FLAG_BOKEH = 1,
    CALIBRATION_FLAG_T_W,
    CALIBRATION_FLAG_SPW,
    CALIBRATION_FLAG_3D_STL,
    CALIBRATION_FLAG_OZ1,
    CALIBRATION_FLAG_OZ2,
    CALIBRATION_FLAG_BOKEH_GLD,
    CALIBRATION_FLAG_BOKEH_GLD_ONLINE,
    CALIBRATION_FLAG_OZ1_GLD,
    CALIBRATION_FLAG_OZ2_GLD,
    CALIBRATION_FLAG_MAX
};

#define OTP_CALI_BOKEH_PATH "/data/vendor/local/otpdata/otp_cali_bokeh.bin"
#define OTPBK_CALI_BOKEH_PATH                                                  \
    "/mnt/vendor/productinfo/otpdata/otpbk_cali_bokeh.bin"

#define BOKEH_CMEI_PATH "/data/vendor/local/otpdata/bokeh_cmei.bin"
#define BOKEHBK_CMEI_PATH                                                  \
    "/mnt/vendor/productinfo/otpdata/bokehbk_cmei.bin"

#define OZ1_CMEI_PATH "/data/vendor/local/otpdata/oz1_cmei.bin"
#define OZ1BK_CMEI_PATH                                                  \
    "/mnt/vendor/productinfo/otpdata/oz1bk_cmei.bin"

#define OZ2_CMEI_PATH "/data/vendor/local/otpdata/oz2_cmei.bin"
#define OZ2BK_CMEI_PATH                                                  \
    "/mnt/vendor/productinfo/otpdata/oz2bk_cmei.bin"

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

#define OTP_CALI_BOKEH_GLD_ONLINE_PATH "/vendor/etc/otpdata/otp_cali_bokeh_gld_online.bin"

#define OTP_CALI_OZ1_GLD_PATH "/vendor/etc/otpdata/otp_cali_oz1_gld.bin"

#define OTP_CALI_OZ2_GLD_PATH "/vendor/etc/otpdata/otp_cali_oz2_gld.bin"

cmr_u16 read_cmei_from_file(cmr_u8 *buf,cmr_u16 buf_size, cmr_u8 dual_flag);
cmr_u8 write_cmei_to_file (cmr_u8 *buf,cmr_u16 buf_size, cmr_u8 dual_flag);

cmr_u16 read_calibration_otp_from_file(cmr_u8 *buf, cmr_u8 dual_flag);
cmr_u8 write_calibration_otp_to_file(cmr_u8 *buf, cmr_u8 dual_flag,
                                     cmr_u16 otp_size);

#endif
