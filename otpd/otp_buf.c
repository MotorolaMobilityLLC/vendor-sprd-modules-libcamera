// Copyright[2015] <SPRD>
#include "otp_common.h"
#include "otp_os.h"
#include "otp_fs.h"
#include "otp_buf.h"

_RAMOTP_CTL ramOtpCtl;

//----------------------------
//  init buffer module
//----------------------------
BOOLEAN poweron_init_otp_golden_data(uint16 part_number)
{
    uint32 id = 0;
    int ret = 0;
    uint16 golden_data_length = 0;
    getMutex();
    id = part_number-1;
    golden_data_length= ramDisk_Read_Golden(ramOtpCtl.part[id].fdhandle,ramOtpCtl.part[id].toDisk.diskbuf,RAMOTP_DIRTYTABLE_DEFAULTSIZE);

    if (golden_data_length > 0) {
        ret = ramDisk_Write(ramOtpCtl.part[id].fdhandle,ramOtpCtl.part[id].toDisk.diskbuf,golden_data_length,0);
        OTP_LOGD("%s: saveToDisk save success", __FUNCTION__);
    }
    putMutex();
    return ret;
}

void poweron_init_otpdata(void) {

	uint32 i, k;
	const RAM_OTP_CONFIG* config;

	ramOtpCtl.partNum = 0;
	i = 0;

	config = ramDisk_Init();

	while (config->partId) {


        ramOtpCtl.part[i].fdhandle = ramDisk_Open(config->partId);

		if (0 == ramOtpCtl.part[i].fdhandle) {
			config++;
			continue;
		}

		ramOtpCtl.part[i].partId = config->partId;
		ramOtpCtl.part[i].sctNum = 1;//config->image_size/RAMOTP_SECT_SIZE;
		ramOtpCtl.part[i].fromHal.diskbuf = malloc(RAMOTP_DIRTYTABLE_MAXSIZE);
		ramOtpCtl.part[i].backup.diskbuf = malloc(RAMOTP_DIRTYTABLE_MAXSIZE);
		ramOtpCtl.part[i].toDisk.diskbuf = malloc(RAMOTP_DIRTYTABLE_MAXSIZE);
		//--- for test ---
		memset(ramOtpCtl.part[i].fromHal.diskbuf, 0, RAMOTP_DIRTYTABLE_MAXSIZE);
		memset(ramOtpCtl.part[i].backup.diskbuf, 0, RAMOTP_DIRTYTABLE_MAXSIZE);
		memset(ramOtpCtl.part[i].toDisk.diskbuf, 0, RAMOTP_DIRTYTABLE_MAXSIZE);
		//------------
		//------------------------------------------------------------
		if (ramDisk_Read(ramOtpCtl.part[i].fdhandle,ramOtpCtl.part[i].fromHal.diskbuf, config->image_size)>0) {
			//free(ramOtpCtl.part[i].fromHal.diskbuf);
			//free(ramOtpCtl.part[i].backup.diskbuf);
			//free(ramOtpCtl.part[i].toDisk.diskbuf);
			getMutex();
			memcpy(ramOtpCtl.part[i].backup.diskbuf,
			ramOtpCtl.part[i].fromHal.diskbuf, config->image_size);
			memcpy(ramOtpCtl.part[i].toDisk.diskbuf,
			ramOtpCtl.part[i].fromHal.diskbuf, config->image_size);
			putMutex();
		}

		ramOtpCtl.partNum++;
		i++;
		config++;
	}
}

//----------------------------
//  uninit buffer module
//----------------------------
void unpoweron_init_otpdata(void) {

	uint32 i;

	for (i = 0; i < ramOtpCtl.partNum; i++) {
		ramOtpCtl.part[i].sctNum = 0;
		if (ramOtpCtl.part[i].fromHal.diskbuf) {
			free(ramOtpCtl.part[i].fromHal.diskbuf);
		}
		
		if (ramOtpCtl.part[i].backup.diskbuf) {
			free(ramOtpCtl.part[i].backup.diskbuf);
		}
		
		if (ramOtpCtl.part[i].toDisk.diskbuf) {
			free(ramOtpCtl.part[i].toDisk.diskbuf);
		}
	}
	ramOtpCtl.partNum = 0;
}

//----------------------------
//  partId to index of control array
//----------------------------
uint32 getCtlId(uint32 partId) {
	uint32 i;

	for (i = 0; i < ramOtpCtl.partNum; i++) {
		if (partId == ramOtpCtl.part[i].partId) {
			return i;
		}
	}
	return (uint32)-1;
}
//----------------------------
// to "fromHal" buffer
// unit is sector
//----------------------------
/*PASS*/
#define _U32MASK ((uint32)(-1))  // 0xFFFFFFFF

//----------------------------
// to "fromHal" buffer
// unit is bytes
//----------------------------
void writeData(uint32 id, uint32 start, uint32 bytesLen, uint8* buf) {
	if (bytesLen) {
		getMutex();
		memcpy(&ramOtpCtl.part[id].fromHal.diskbuf[start], buf, bytesLen);
		putMutex();
	}
}

//----------------------------
// fromHal -> backup
//----------------------------
BOOLEAN backupData(uint32 id) {
	BOOLEAN ret = 0;

	getMutex();
	memcpy(ramOtpCtl.part[id].backup.diskbuf,
			ramOtpCtl.part[id].fromHal.diskbuf,
			RAMOTP_SECT_SIZE * ramOtpCtl.part[id].sctNum);
	putMutex();
	ret = saveToDisk(0,ramOtpCtl.part[id].backup.diskbuf,RAMOTP_SECT_SIZE * ramOtpCtl.part[id].sctNum,ramOtpCtl.part[id].sctNum);
	return ret;
}

//----------------------------
// backup -> fromHal
//----------------------------
void restoreData(uint32 id) {
	getMutex();

	memcpy(ramOtpCtl.part[id].fromHal.diskbuf,
			ramOtpCtl.part[id].backup.diskbuf,
			RAMOTP_SECT_SIZE * ramOtpCtl.part[id].sctNum);

	putMutex();
	return;
}

//----------------------------
// backup -> toDisk
//----------------------------
BOOLEAN saveToDisk(BOOLEAN is_golden_data,char *opt_data,uint16 otpdatasize,uint16 part_number) {
    uint16 id= 0;
    BOOLEAN ret = 0;
    getMutex();
    id = part_number-1;
    OTP_LOGD("%s: ramOtpCtl.partNum:%d is_golden_data:%d  part_number:%d", __FUNCTION__,ramOtpCtl.partNum,is_golden_data,part_number);
    if(is_golden_data){
        if (ramDisk_Write_Golden(ramOtpCtl.part[id].fdhandle,opt_data,otpdatasize)) {
            OTP_LOGD("%s: saveToDisk save golden success", __FUNCTION__);
            ret = 1;
        }
    }else{
        if (ramDisk_Write(ramOtpCtl.part[id].fdhandle,opt_data,otpdatasize,1)) {
            OTP_LOGD("%s: saveToDisk save success", __FUNCTION__);
            ret = 1;
        }
    }
    putMutex();
    return ret;
}

