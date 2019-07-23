// copyright[2015] <SPRD>
#ifndef OTP_FS_H_
#define OTP_FS_H_
#include "otp_common.h"

typedef struct {
  uint32 partId;
  char image_path[100];
  char imageBak_path[100];
  char imageGolden_path[100];
  uint16 image_size;
} RAM_OTP_CONFIG;

typedef uint32 RAMDISK_HANDLE;

const RAM_OTP_CONFIG* ramDisk_Init(void);
RAMDISK_HANDLE ramDisk_Open(uint32 partId);
uint16 ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint16 size);
uint16 ramDisk_Read_Golden(RAMDISK_HANDLE handle,uint8* buf, uint16 size);
BOOLEAN ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint16 size,BOOLEAN is_calibration_write);
BOOLEAN ramDisk_Write_Golden(RAMDISK_HANDLE handle, uint8* buf, uint16 size);
int init_otp_parament(void);
void otp_dump_data(uint8* buf, uint32 size);
#endif  // OTP_FS_H_
