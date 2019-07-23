// Copyright[2015] <SPRD>
#ifndef OTP_BUF_H_
#define OTP_BUF_H_

#include "otp_common.h"

typedef struct {
  char* diskbuf;                          // ramdisk buf
} _RAMOTP_BUF_CTRL;

typedef struct {
  // disk partition id
  uint32 partId;
  uint16 sctNum;  // total number of sector in current disk
                  // disk buffer
  _RAMOTP_BUF_CTRL fromHal;
  _RAMOTP_BUF_CTRL backup;
  _RAMOTP_BUF_CTRL toDisk;
  // fs handle
  RAMDISK_HANDLE fdhandle;
} _RAMOTP_PART_CTRL;

typedef struct {
  uint32 partNum;
  _RAMOTP_PART_CTRL part[RAMOTP_NUM];
} _RAMOTP_CTL;

//----------------------------
//  init buffer module
//----------------------------
void poweron_init_otpdata(void);
//----------------------------
//  read golden data and save to otpdate
//----------------------------
BOOLEAN poweron_init_otp_golden_data(uint16 part_number);
//----------------------------
//  uninit buffer module
//----------------------------
void unpoweron_init_otpdata(void);
//----------------------------
//  partId to index of control array
//----------------------------
uint32 getCtlId(uint32 partId);
//----------------------------
// to "fromHal" buffer
// unit is bytes
//----------------------------
void writeData(uint32 id, uint32 start, uint32 bytesLen, uint8* buf);
//----------------------------
// fromHal -> backup
//----------------------------
BOOLEAN backupData(uint32 id);
//----------------------------
// backup -> fromHal
//----------------------------
void restoreData(uint32 id);
//----------------------------
// backup -> toDisk
//----------------------------
BOOLEAN saveToDisk(BOOLEAN is_golden_data,char *opt_data,uint16 otpdatasize,uint16 part_number);
#endif  // OTP_BUF_H_
