// Copyright[2015] <SPRD>
#ifndef OTP_COMMON_H_
#define OTP_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <cutils/log.h>

#define OTPD_DEBUG
#ifdef OTPD_DEBUG
#define OTP_LOGD(x...) ALOGD(x)
#define OTP_LOGE(x...) ALOGE(x)
#else
#define OTP_LOGD(x...) do {} while (0)
#define OTP_LOGE(x...) do {} while (0)
#endif

typedef unsigned char BOOLEAN;
typedef unsigned char uint8;
typedef unsigned short uint16;  //NOLINT
typedef unsigned int uint32;

typedef signed char int8;
typedef signed short int16;  //NOLINT
typedef signed int int32;

//-------------------------------------------------
//              Config: can be changed if nessarry
//-------------------------------------------------
#define RAMOTP_DIRTYTABLE_MAXSIZE                 20480
#define RAMOTP_DIRTYTABLE_MINSIZE                  256
#define RAMOTP_DIRTYTABLE_DEFAULTSIZE      256

//-------------------------------------------------
//              Const config: can not be changed
//-------------------------------------------------
#define RAMOTP_NUM                        15         // max number of ramdisk, can not >= 15.
#define RAMOTP_SECT_SIZE           RAMOTP_DIRTYTABLE_MAXSIZE//256  // sect size of ramdisk
#define RAMOTP_HEAD_SIZE         32  // sect size of ramdisk

#endif  // OTP_COMMON_H_
