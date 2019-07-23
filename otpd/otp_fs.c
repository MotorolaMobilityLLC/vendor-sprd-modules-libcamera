// Copyright[2015] <SPRD>
#include "otp_common.h"
#include "otp_fs.h"
#include "cutils/properties.h"

typedef struct _OTP_HEADER {
    uint32 magic;
    uint16 len;
    uint32 checksum;
    uint32 version;
    uint8 data_type;
    BOOLEAN has_calibration;
} otp_header_t;

#define OTP_HEAD_MAGIC 0x00004e56
#define OTP_VERSION 003
#define OTPDATA_BASE  (0)
#define OTPDATA_OFFSET  (519 * 1024)
#define OTPDATA_START (OTPDATA_BASE + OTPDATA_OFFSET)

#define OTPDATA_TYPE_CALIBRATION  (1)
#define OTPDATA_TYPE_GOLDEN  (2)

#define RO_PRODUCT_OTPDATA_PARTITION_PATH "/data/vendor/local/otpdata"  
#define RO_PRODUCT_OTPBKDATA_PARTITION_PATH "/mnt/vendor/productinfo/otpdata"
#define RO_DEF_NAME "/"

#define RO_BOKEH_OTPDATA_NAME "otpdata.txt"
#define RO_BOKEH_OTPBKDATA_NAME "otpbkdata.txt"
#define RW_BOKEH_PRODUCT_GOLDEN_PARTITION_PATH "/system/etc/otpdata/otpgoldendata.txt"

#define RO_W_T_OTPDATA_NAME "w_t_otpdata.bin"
#define RO_W_T_OTPBKDATA_NAME "w_t_otpbkdata.bin"
#define RW_W_T_PRODUCT_GOLDEN_PARTITION_PATH "/system/etc/otpdata/w_t_otpgoldendata.bin"

#define RO_SPW_OTPDATA_NAME "spw_otpdata.bin"
#define RO_SPW_OTPBKDATA_NAME "spw_otpbkdata.bin"
#define RW_SPW_PRODUCT_GOLDEN_PARTITION_PATH "/system/etc/otpdata/spw_otpgoldendata.bin"

#define RO_CAMERA_CHAR "ro.vendor.camera"
#define RO_CAMERA_OTPDATASIZE ".otpdata_size"

static BOOLEAN checksum_64 = 0;

static RAM_OTP_CONFIG _ramdiskCfg[RAMOTP_NUM + 1] = {
    {1, "/data/vendor/local/otpdata/otpdata.txt", "/mnt/vendor/productinfo/otpdata/otpbkdata.txt", RW_BOKEH_PRODUCT_GOLDEN_PARTITION_PATH,RAMOTP_DIRTYTABLE_DEFAULTSIZE},
    {2, "/data/vendor/local/otpdata/w_t_otpdata.bin", "/mnt/vendor/productinfo/otpdata/w_t_otpbkdata.bin", RW_W_T_PRODUCT_GOLDEN_PARTITION_PATH,RAMOTP_DIRTYTABLE_DEFAULTSIZE},
    {3, "/data/vendor/local/otpdata/spw_otpdata.bin", "/mnt/vendor/productinfo/otpdata/spw_otpbkdata.bin", RW_SPW_PRODUCT_GOLDEN_PARTITION_PATH,RAMOTP_DIRTYTABLE_DEFAULTSIZE},
    {0, "", "", 0}};

char bokeh_otpdata_ori_path[100];
char bokeh_otpdata_bak_path[100];
char bokeh_otpdata_golden_path[100];

char w_t_otpdata_ori_path[100];
char w_t_otpdata_bak_path[100];
char w_t_otpdata_golden_path[100];

char spw_otpdata_ori_path[100];
char spw_otpdata_bak_path[100];
char spw_otpdata_golden_path[100];

uint32 otpdata_size;

void otp_dump_data(uint8* buf, uint32 size) 
{
    return;
    OTP_LOGD("dump all mem buf:\n");
    for(int i = 0;i<size;i++)
    {
        OTP_LOGD("buf[%d]:0x%x\n",i,buf[i]);
    }
}

static unsigned short calc_checksum(unsigned char* dat, uint16 len) {  // NOLINT
	unsigned short num = 0;  // NOLINT
	uint32_t chkSum = 0;  // NOLINT
	while (len > 1) {
		num = (unsigned short)(*dat);  // NOLINT
		dat++;
		num |= (((unsigned short)(*dat)) << 8);  // NOLINT
		dat++;
		chkSum += (uint32_t)num;  // NOLINT
		len -= 2;
	}
	if (len) {
		chkSum += *dat;
	}
	chkSum = (chkSum >> 16) + (chkSum & 0xffff);
	chkSum += (chkSum >> 16);
	return (~chkSum);
}

static unsigned short calc_checksum64(unsigned char* dat, uint16 len) {  // NOLINT
	unsigned short num = 0;  // NOLINT
	uint64_t chkSum = 0;  // NOLINT
	while (len > 1) {
		num = (unsigned short)(*dat);  // NOLINT
		dat++;
		num |= (((unsigned short)(*dat)) << 8);  // NOLINT
		dat++;
		chkSum += (uint64_t)num;  // NOLINT
		len -= 2;
	}
	if (len) {
		chkSum += *dat;
	}
	chkSum = (chkSum >> 16) + (chkSum & 0xffff);
	chkSum += (chkSum >> 16);
	return (~chkSum);
}

/*
        TRUE(1): pass
        FALSE(0): fail
*/
static BOOLEAN _chkOTPEcc(uint8* buf, uint16 size, uint16 checksum) {
	uint16 crc;

	crc = calc_checksum64(buf, size);
	OTP_LOGD("%s:crc = 0x%x,checksum=0x%x\n",__FUNCTION__,crc,checksum);
	if (crc == checksum) {
		checksum_64 = 1;
		OTP_LOGD("%s:checksum_64=%d\n",__FUNCTION__,checksum_64);
		return 1;
	}

	crc = calc_checksum(buf, size);
	OTP_LOGD("%s:crc = 0x%x,checksum=0x%x\n",__FUNCTION__,crc,checksum);
	if (crc == checksum) {
		checksum_64 = 0;
		OTP_LOGD("%s:checksum_64=%d\n",__FUNCTION__,checksum_64);
		return 1;
	}

	return 0;
}

int init_otp_parament(void) {

	char otpdata[95];
	char otpdata_property[50];
	char ro_prop[20];
	char partition_path[100];

	otpdata[0] = '\0';

	// get system prop eg:ro.modem.t;ro.modem.w ....
	if (strlen(RO_CAMERA_CHAR) < 20) {
		strcpy(ro_prop, RO_CAMERA_CHAR);  // NOLINT
	}
	OTP_LOGD("%s:ro_prop %s\n",__FUNCTION__,ro_prop);

	// get otp size
	if (strlen(ro_prop) + strlen(RO_CAMERA_OTPDATASIZE) < 50) {
		strcpy(otpdata_property, ro_prop);  // NOLINT
		strcat(otpdata_property, RO_CAMERA_OTPDATASIZE);  // NOLINT
		property_get(otpdata_property, otpdata, "");
		if (0 == strlen(otpdata)) {
			OTP_LOGD("%s:invalid ro.modem.w.otpdata_size\n",__FUNCTION__);
			return 0;
		}
		OTP_LOGD("%s:otpdata_property is %s otpdata %s\n",__FUNCTION__,otpdata_property,otpdata);
		otpdata_size = strtol(otpdata, 0, 16);
		OTP_LOGD("otpdata_size %x\n", otpdata_size);
	}

	// get otp path: eg: partition_path+td+otpdata

	if ((100 > strlen(RO_PRODUCT_OTPBKDATA_PARTITION_PATH)+strlen(RO_DEF_NAME)+strlen(RO_BOKEH_OTPBKDATA_NAME))
	&& (100 > strlen(RO_PRODUCT_OTPDATA_PARTITION_PATH)+strlen(RO_DEF_NAME)+strlen(RO_BOKEH_OTPDATA_NAME))
	&& (100 > strlen(RW_BOKEH_PRODUCT_GOLDEN_PARTITION_PATH))){

		strcpy(bokeh_otpdata_ori_path, RO_PRODUCT_OTPDATA_PARTITION_PATH);  // NOLINT
		strcat(bokeh_otpdata_ori_path, RO_DEF_NAME);  // NOLINT
		strcat(bokeh_otpdata_ori_path, RO_BOKEH_OTPDATA_NAME);  // NOLINT
		strcpy(bokeh_otpdata_bak_path, RO_PRODUCT_OTPBKDATA_PARTITION_PATH);  // NOLINT
		strcat(bokeh_otpdata_bak_path, RO_DEF_NAME);  // NOLINT
		strcat(bokeh_otpdata_bak_path, RO_BOKEH_OTPBKDATA_NAME);  // NOLINT
		strcpy(bokeh_otpdata_golden_path, RW_BOKEH_PRODUCT_GOLDEN_PARTITION_PATH);  // NOLINT

		strcpy(w_t_otpdata_ori_path, RO_PRODUCT_OTPDATA_PARTITION_PATH);  // NOLINT
		strcat(w_t_otpdata_ori_path, RO_DEF_NAME);  // NOLINT
		strcat(w_t_otpdata_ori_path, RO_W_T_OTPDATA_NAME);  // NOLINT
		strcpy(w_t_otpdata_bak_path, RO_PRODUCT_OTPBKDATA_PARTITION_PATH);  // NOLINT
		strcat(w_t_otpdata_bak_path, RO_DEF_NAME);  // NOLINT
		strcat(w_t_otpdata_bak_path, RO_W_T_OTPBKDATA_NAME);  // NOLINT
		strcpy(w_t_otpdata_golden_path, RW_W_T_PRODUCT_GOLDEN_PARTITION_PATH);  // NOLINT

		strcpy(spw_otpdata_ori_path, RO_PRODUCT_OTPDATA_PARTITION_PATH);  // NOLINT
		strcat(spw_otpdata_ori_path, RO_DEF_NAME);  // NOLINT
		strcat(spw_otpdata_ori_path, RO_SPW_OTPDATA_NAME);  // NOLINT
		strcpy(spw_otpdata_bak_path, RO_PRODUCT_OTPBKDATA_PARTITION_PATH);  // NOLINT
		strcat(spw_otpdata_bak_path, RO_DEF_NAME);  // NOLINT
		strcat(spw_otpdata_bak_path, RO_SPW_OTPBKDATA_NAME);  // NOLINT
		strcpy(spw_otpdata_golden_path, RW_SPW_PRODUCT_GOLDEN_PARTITION_PATH);  // NOLINT

		OTP_LOGD("%s:bokeh_otpdata_ori_path %s bokeh_otpdata_golden_path %s  bokeh_otpdata_bak_path %s\n",
					__FUNCTION__,
					bokeh_otpdata_ori_path,
					bokeh_otpdata_golden_path,
					bokeh_otpdata_bak_path
					);

	} else {

		OTP_LOGD("%s:otpdata path too long! check it \n",__FUNCTION__);
		return 0;
	}

	return 1;
}

const RAM_OTP_CONFIG* ramDisk_Init(void) {
    if (strlen(bokeh_otpdata_ori_path) > 100
        || strlen(bokeh_otpdata_bak_path) > 100
        ||  strlen(bokeh_otpdata_golden_path) > 100) {
            OTP_LOGD("%s:init config fail: invalid param!\n",__FUNCTION__);
            return _ramdiskCfg;
    }

    memset(_ramdiskCfg, 0, sizeof(_ramdiskCfg));
    _ramdiskCfg[0].partId = 1;
    strcpy(_ramdiskCfg[0].image_path, bokeh_otpdata_ori_path);  // NOLINT
    strcpy(_ramdiskCfg[0].imageBak_path, bokeh_otpdata_bak_path);  // NOLINT
    strcpy(_ramdiskCfg[0].imageGolden_path, bokeh_otpdata_golden_path);  // NOLINT
    _ramdiskCfg[0].image_size = RAMOTP_DIRTYTABLE_DEFAULTSIZE;//otpdata_size
    OTP_LOGD("%s:i = \t0x%x\t%32s\t%32s\t%32s\t0x%8x\n",__FUNCTION__,_ramdiskCfg[0].partId,
	    _ramdiskCfg[0].image_path,
	    _ramdiskCfg[0].imageBak_path,
	    _ramdiskCfg[0].imageGolden_path,
	    _ramdiskCfg[0].image_size);

    _ramdiskCfg[1].partId = 2;
    strcpy(_ramdiskCfg[1].image_path, w_t_otpdata_ori_path);  // NOLINT
    strcpy(_ramdiskCfg[1].imageBak_path, w_t_otpdata_bak_path);  // NOLINT
    strcpy(_ramdiskCfg[1].imageGolden_path, w_t_otpdata_golden_path);  // NOLINT
    _ramdiskCfg[1].image_size = RAMOTP_DIRTYTABLE_DEFAULTSIZE;//otpdata_size
    OTP_LOGD("%s:i = \t0x%x\t%32s\t%32s\t%32s\t0x%8x\n",__FUNCTION__,_ramdiskCfg[1].partId,
	    _ramdiskCfg[1].image_path,
	    _ramdiskCfg[1].imageBak_path,
	    _ramdiskCfg[1].imageGolden_path,
	    _ramdiskCfg[1].image_size);

    _ramdiskCfg[2].partId = 3;
    strcpy(_ramdiskCfg[2].image_path, spw_otpdata_ori_path);  // NOLINT
    strcpy(_ramdiskCfg[2].imageBak_path, spw_otpdata_bak_path);  // NOLINT
    strcpy(_ramdiskCfg[2].imageGolden_path, spw_otpdata_golden_path);  // NOLINT
    _ramdiskCfg[2].image_size = RAMOTP_DIRTYTABLE_DEFAULTSIZE;//otpdata_size
    OTP_LOGD("%s:i = \t0x%x\t%32s\t%32s\t%32s\t0x%8x\n",__FUNCTION__,_ramdiskCfg[2].partId,
	    _ramdiskCfg[2].image_path,
	    _ramdiskCfg[2].imageBak_path,
	    _ramdiskCfg[2].imageGolden_path,
	    _ramdiskCfg[2].image_size);

    return _ramdiskCfg;
}

RAMDISK_HANDLE ramDisk_Open(uint32 partId) { return (RAMDISK_HANDLE)partId; }

int _getIdx(RAMDISK_HANDLE handle) {
	uint32 i;
	uint32 partId = (uint32)handle;

    OTP_LOGD("%s:partId =%d",__FUNCTION__,partId);
	for (i = 0; i < sizeof(_ramdiskCfg) / sizeof(RAM_OTP_CONFIG); i++) {
        OTP_LOGD("%s:_ramdiskCfg.partId =%d",__FUNCTION__,_ramdiskCfg[i].partId);
		if (0 == _ramdiskCfg[i].partId) {
			return -1;
		} else if (partId == _ramdiskCfg[i].partId) {
			return i;
		}
	}
	return -1;
}

/*
        1 read imagPath first, if succes return , then
        2 read imageBakPath, if fail , return, then
        3 fix imagePath

        note:
                first imagePath then imageBakPath
*/
uint16 ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint16 size) {
    int ret = 0;
    int idx =0;
    uint32 rcount = 0,ret1 = 0, ret2 = 0;
    char header[RAMOTP_HEAD_SIZE];
    otp_header_t* header_ptr = NULL;
    char *OtpDataPath, *OtpBkDataPath;
    FILE * fileProductinfoHandle = NULL;

    OTP_LOGD("%s:ramDisk_Read enter\n",__FUNCTION__);
    header_ptr = (otp_header_t*)header;
    idx = _getIdx(handle);
    if (-1 == idx  || idx > RAMOTP_NUM) {
	OTP_LOGD("%s:null handle=%d, idx =%d",__FUNCTION__,handle, idx);
	return 0;
    }

    // 0 get read order
    OtpDataPath = _ramdiskCfg[idx].image_path;
    OtpBkDataPath = _ramdiskCfg[idx].imageBak_path;

	if(NULL == OtpDataPath ||NULL == OtpBkDataPath )
	{
		OTP_LOGD("%s: OtpDataPath if Null",__FUNCTION__);
		return 0;
	}

    // 1 read first image is productinfo otpdata
    memset(buf, 0xFF, RAMOTP_DIRTYTABLE_MAXSIZE);
    memset(header, 0x00, RAMOTP_HEAD_SIZE);
    do {
        if(access(OtpDataPath, F_OK) == 0){
            //system("chmod 666 /data/vendor/local/otpdata/otpdata.txt");
            fileProductinfoHandle = fopen(OtpDataPath, "r");

            if(NULL == fileProductinfoHandle){
                OTP_LOGD("%s: fopen fail %s",__FUNCTION__,OtpDataPath);
                break;
            }

            rcount = fread(header, sizeof(char), RAMOTP_HEAD_SIZE, fileProductinfoHandle);
            if(rcount != RAMOTP_HEAD_SIZE){
                fclose(fileProductinfoHandle);
                break;
            }

            size = header_ptr->len;

            rcount = fread(buf, sizeof(char), size, fileProductinfoHandle);
            if(rcount != size){
                fclose(fileProductinfoHandle);
                break;
            }

            fclose(fileProductinfoHandle);
        }else{
            OTP_LOGD("%s:partId%x:%s open file failed!\n",__FUNCTION__,_ramdiskCfg[idx].partId,OtpDataPath);
            break;
        }

        if(_chkOTPEcc(buf, size, header_ptr->checksum)){
            OTP_LOGD("%s:partId%x:%s read success!\n",__FUNCTION__,_ramdiskCfg[idx].partId,OtpDataPath);
            if(header_ptr->has_calibration){
                return header_ptr->len;
            }
        }

        OTP_LOGD("%s:partId%x:%s read error!\n",__FUNCTION__,_ramdiskCfg[idx].partId,OtpDataPath);
    } while (0);
    
    // 2 read otp backup data
    memset(buf, 0xFF, RAMOTP_DIRTYTABLE_MAXSIZE);
    memset(header, 0x00, RAMOTP_HEAD_SIZE);
    do {
        if(access(OtpBkDataPath, F_OK) == 0){
            //system("chmod 666 /mnt/vendor/productinfo/otpdata/otpbkdata.txt");
            fileProductinfoHandle = fopen(OtpBkDataPath, "r");

            if(NULL == fileProductinfoHandle){
                OTP_LOGD("%s: fopen fail %s",__FUNCTION__,OtpBkDataPath);
                break;
            }

            rcount = fread(header, sizeof(char), RAMOTP_HEAD_SIZE, fileProductinfoHandle);
            if(rcount != RAMOTP_HEAD_SIZE){
                fclose(fileProductinfoHandle);
                break;
            }
            
            size = header_ptr->len;

            rcount = fread(buf, sizeof(char), size, fileProductinfoHandle);
            if(rcount != size){
                fclose(fileProductinfoHandle);
                break;
            }

            fclose(fileProductinfoHandle);
        }else{
            OTP_LOGD("%s:partId%x:%s open file failed!\n",__FUNCTION__,_ramdiskCfg[idx].partId,OtpBkDataPath);
            break;
        }

        if(_chkOTPEcc(buf, size, header_ptr->checksum)){
            //write otp date to otpdate path
            if (0 != access(RO_PRODUCT_OTPDATA_PARTITION_PATH, F_OK)) {
                ret = mkdir(RO_PRODUCT_OTPDATA_PARTITION_PATH, S_IRWXU | S_IRWXG | S_IRWXO);
                if (-1 == ret) {
                    OTP_LOGE("%s:mkdir %s failed.",__FUNCTION__,RO_PRODUCT_OTPDATA_PARTITION_PATH);
                    if(header_ptr->has_calibration){
                        return header_ptr->len;
                    }else{
                        return 0;
                    }
                }
            }
            
            //system("chmod 777 /data/vendor/local/otpdata/otpdata.txt");
            ret = remove(_ramdiskCfg[idx].image_path);
            OTP_LOGD("%s:remove file %s ret:%d \n",__FUNCTION__,_ramdiskCfg[idx].image_path,ret);
            fileProductinfoHandle = fopen(_ramdiskCfg[idx].image_path, "w+");

            if (fileProductinfoHandle != NULL) {
                int fd = 0;
                if (RAMOTP_HEAD_SIZE != fwrite(header, sizeof(char), RAMOTP_HEAD_SIZE, fileProductinfoHandle)) {
                    ret = 0;
                    OTP_LOGD("%s:partId%x:origin image header write fail!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
                } else {
                    ret = 1;
                    OTP_LOGD("write origin header success");
                }

                if (ret && (size != fwrite(buf, sizeof(char), size, fileProductinfoHandle))) {
                    OTP_LOGD("%s:partId%x:origin image write fail!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
                    ret = 0;
                }

                if(ret){
                    fflush(fileProductinfoHandle);
                    fd = fileno(fileProductinfoHandle);
                    
                    if(fd > 0) {
                        fsync(fd);
                        ret = 1;
                    } else {
                        OTP_LOGD("%s: fileno() =%s", __FUNCTION__, _ramdiskCfg[idx].image_path);
                        ret = 0;
                    }
                }

                fclose(fileProductinfoHandle);
                OTP_LOGD("%s:partId%x:image write finished %d!\n",__FUNCTION__,_ramdiskCfg[idx].partId,ret);
            }else{
                OTP_LOGD("%s:open file %s failed\n",__FUNCTION__,_ramdiskCfg[idx].image_path);
            }

            OTP_LOGD("%s:partId%x:%s read success!\n",__FUNCTION__,_ramdiskCfg[idx].partId,OtpBkDataPath);
            
            if(header_ptr->has_calibration){
                return header_ptr->len;
            }

            return 0;
        }

        OTP_LOGD("%s:partId%x:%s read error!\n",__FUNCTION__,_ramdiskCfg[idx].partId,OtpBkDataPath);
    } while (0);

#if 0
    {
        uint16 golden_data_length = 0;
        // 3 read golden data and write to otpdate & otpbkdata
        memset(buf, 0xFF, size);
        memset(header, 0x00, RAMOTP_HEAD_SIZE);
        golden_data_length = ramDisk_Read_Golden(handle,buf,size);
        if(golden_data_length > 0){
            ret = ramDisk_Write(handle,buf,golden_data_length,0);
        }
    }

   OTP_LOGD("%s:partId%x:%s read golden data write otp and otp_bk is ok!\n",__FUNCTION__,_ramdiskCfg[idx].partId,_ramdiskCfg[idx].imageGolden_path);
#endif

    return 0;
}

/*
        1 read golden data, if succes then write otp.txt and otp_bk.txt 
        2 read golden data, if failed then return 0

        note:
*/
uint16 ramDisk_Read_Golden(RAMDISK_HANDLE handle,uint8* buf, uint16 size) {
    int idx =0;
    uint16 rcount = 0;
    char header[RAMOTP_HEAD_SIZE];
    otp_header_t* header_ptr = NULL;
    char *goldenName;
    FILE * fileGoldenHandle = NULL;

    OTP_LOGD("%s:ramDisk_Read_Golden enter\n",__FUNCTION__);
    header_ptr = (otp_header_t*)header;
    idx = _getIdx(handle);
    if (-1 == idx) {
        return 0;
    }
    // 0 get read order
    goldenName = _ramdiskCfg[idx].imageGolden_path;

    // 1 read golden otpdate is productinfo otpdata
    memset(buf, 0xFF, size);
    memset(header, 0x00, RAMOTP_HEAD_SIZE);

    do {
        if(access(goldenName, F_OK) == 0){
            fileGoldenHandle = fopen(goldenName, "r");

            if(NULL == fileGoldenHandle){
                OTP_LOGD("%s: fopen fail %s",__FUNCTION__,fileGoldenHandle);
                return 0;
            }

            rcount = fread(header, sizeof(char), RAMOTP_HEAD_SIZE, fileGoldenHandle);
            if(rcount != RAMOTP_HEAD_SIZE){
                fclose(fileGoldenHandle);
                return 0;
            }

            rcount = fread(buf, sizeof(char), header_ptr->len, fileGoldenHandle);
            if(rcount != header_ptr->len){
                fclose(fileGoldenHandle);
                return 0;
            }

            fclose(fileGoldenHandle);
            
            if(!_chkOTPEcc(buf, header_ptr->len, header_ptr->checksum)){
                OTP_LOGD("%s:partId%x:%s read ok!len:%d \n",__FUNCTION__,_ramdiskCfg[idx].partId,goldenName,header_ptr->len);
                return header_ptr->len;
            }

        }else{
            OTP_LOGD("%s:partId%x:%s open golden file failed!\n",__FUNCTION__,_ramdiskCfg[idx].partId,goldenName);
            return 0;
        }
    } while (0);
    //golden has read is ok then set otp.txt and otp_bk.txt
    //test code
    otp_dump_data(buf,rcount);
    //test code

    return rcount;
}


/*
1 get Ecc first
2 write  imageBakPath
3 write imagePath

note:
        first imageBakPath then imagePath
*/
BOOLEAN ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint16 size,BOOLEAN is_calibration_write) {
    int ret;
    FILE * fileProductinfoHandle = NULL;
    FILE * fileVendorHandle = NULL;
    int idx;
    int fd = -1;

    char header_buf[RAMOTP_HEAD_SIZE];
    otp_header_t* header_ptr = NULL;

    idx = _getIdx(handle);
    OTP_LOGD("%s:handle=%d, idx =%d",__FUNCTION__,handle,idx);

    if (-1 == idx) {
        return 0;
    }
    memset(header_buf, 0x00, RAMOTP_HEAD_SIZE);
    header_ptr = header_buf;
    header_ptr->magic = OTP_HEAD_MAGIC;
    header_ptr->len = size;
    header_ptr->version = OTP_VERSION;

    OTP_LOGD("%s:is_calibration_write =%d",__FUNCTION__,is_calibration_write);

    if(is_calibration_write)
    {
        //calibration data write
        header_ptr->has_calibration = 1;
        header_ptr->data_type = OTPDATA_TYPE_CALIBRATION;
    }else{
        //is golden data write then read otpdata.txt or otpbkdata.txt head check has_calibration
        char header_buf_read[RAMOTP_HEAD_SIZE];
        otp_header_t* read_header_ptr = NULL;
        FILE * fileReadHandle = NULL;
        uint32 readcount = 0;

        memset(header_buf_read, 0x00, RAMOTP_HEAD_SIZE);
        read_header_ptr = header_buf_read;
        
        if(access(_ramdiskCfg[idx].image_path, F_OK) == 0){
            OTP_LOGD("%s:access:%s ok",__FUNCTION__,_ramdiskCfg[idx].image_path);
            fileReadHandle = fopen(_ramdiskCfg[idx].image_path, "r");

            if(NULL != fileReadHandle){
                OTP_LOGD("%s:open:%s ok",__FUNCTION__,_ramdiskCfg[idx].image_path);
                readcount = fread(read_header_ptr, sizeof(char), RAMOTP_HEAD_SIZE, fileReadHandle);
                fclose(fileReadHandle);
            }

            if(readcount == RAMOTP_HEAD_SIZE)
            {
                OTP_LOGD("%s:read:%s ok",__FUNCTION__,_ramdiskCfg[idx].image_path);
                header_ptr->has_calibration = read_header_ptr->has_calibration;
                header_ptr->data_type = OTPDATA_TYPE_GOLDEN;
                OTP_LOGD("%s:read_header_ptr has_calibration:%d ",__FUNCTION__,read_header_ptr->has_calibration);
            }else{
                OTP_LOGE("%s:read:%s failed",__FUNCTION__,_ramdiskCfg[idx].image_path);
                readcount = 0;
            }
        }

        if((readcount == 0) && access(_ramdiskCfg[idx].imageBak_path, F_OK) == 0){
            memset(header_buf_read, 0x00, RAMOTP_HEAD_SIZE);
            read_header_ptr = header_buf_read;
            
            OTP_LOGD("%s:access:%s ok",__FUNCTION__,_ramdiskCfg[idx].imageBak_path);
            fileReadHandle = fopen(_ramdiskCfg[idx].imageBak_path, "r");

            if(NULL != fileReadHandle){
                OTP_LOGD("%s:open:%s ok",__FUNCTION__,_ramdiskCfg[idx].imageBak_path);
                readcount = fread(read_header_ptr, sizeof(char), RAMOTP_HEAD_SIZE, fileReadHandle);
                fclose(fileReadHandle);
            }

            if(readcount == RAMOTP_HEAD_SIZE)
            {
                OTP_LOGD("%s:read:%s ok",__FUNCTION__,_ramdiskCfg[idx].imageBak_path);
                header_ptr->has_calibration = read_header_ptr->has_calibration;
                header_ptr->data_type = OTPDATA_TYPE_GOLDEN;
                OTP_LOGD("%s:readbk_header_ptr has_calibration:%d ",__FUNCTION__,read_header_ptr->has_calibration);
            }else{
                OTP_LOGE("%s:read:%s failed",__FUNCTION__,_ramdiskCfg[idx].imageBak_path);
                readcount = 0;
            }
        }

        if(readcount == 0){
            header_ptr->has_calibration = 0;
        }
        
        header_ptr->data_type = OTPDATA_TYPE_GOLDEN;
    }
    
    OTP_LOGD("%s:header_ptr->has_calibration:%d:header_ptr->data_type=%d",__FUNCTION__,header_ptr->has_calibration,header_ptr->data_type);
    OTP_LOGD("%s:partId%x:checksum_64=%d",__FUNCTION__,_ramdiskCfg[idx].partId,checksum_64);
    // 1 get Ecc
    if(checksum_64){
        header_ptr->checksum = (uint32)calc_checksum64(buf, size);
    }else{
        header_ptr->checksum = (uint32)calc_checksum(buf, size);
    }
    
    // 2 write bakup image is miscdata otpdata
    ret = 1;
    if (0 != access(RO_PRODUCT_OTPBKDATA_PARTITION_PATH, F_OK)) {
        OTP_LOGE("%s:access %s failed",__FUNCTION__,RO_PRODUCT_OTPBKDATA_PARTITION_PATH);
        ret = mkdir(RO_PRODUCT_OTPBKDATA_PARTITION_PATH, S_IRWXU | S_IRWXG | S_IRWXO);
        if (-1 == ret) {
            OTP_LOGE("%s:mkdir %s failed.",__FUNCTION__,RO_PRODUCT_OTPBKDATA_PARTITION_PATH);
            ret = 0;
        }
        else{
            ret = 1;
        }
    }

    if(ret != 0){
        //ret = chmod(_ramdiskCfg[idx].imageBak_path, 0777);
        ret = remove(_ramdiskCfg[idx].imageBak_path);
        OTP_LOGE("%s:remove:%s idx :%d",__FUNCTION__,_ramdiskCfg[idx].imageBak_path,idx);
        fileProductinfoHandle = fopen(_ramdiskCfg[idx].imageBak_path, "w+");
    }

    if (fileProductinfoHandle != NULL) {
        if (RAMOTP_HEAD_SIZE != fwrite(header_buf, sizeof(char), RAMOTP_HEAD_SIZE, fileProductinfoHandle)) {
            ret = 0;
            OTP_LOGD("%s:partId%x:backup image header write fail!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
        } else {
            OTP_LOGD("%s:write backup header",__FUNCTION__);
            ret = 1;
        }
        
        if (ret && (size == fwrite(buf, sizeof(char), size, fileProductinfoHandle))) {
            OTP_LOGD("%s:partId%x:backup image write success!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
            ret = 1;
        }
        else{
            OTP_LOGD("%s:partId%x:backup image write failed!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
            ret = 0;
        }

        if(ret){
            
            fflush(fileProductinfoHandle);
            fd = fileno(fileProductinfoHandle);
            
            if(fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                OTP_LOGD("%s: fileno() =%s", __FUNCTION__,_ramdiskCfg[idx].imageBak_path);
                ret = 0;
            }
        }

        fclose(fileProductinfoHandle);
        OTP_LOGD("%s:partId%x:image write finished %s!\n",__FUNCTION__,_ramdiskCfg[idx].partId,_ramdiskCfg[idx].imageBak_path);
    }else{
        ret = 0;
        OTP_LOGD("%s:partId%x:open bkup %s failed!\n",__FUNCTION__,_ramdiskCfg[idx].partId,_ramdiskCfg[idx].imageBak_path);
    }
    
    // 3 write origin image
    if (0 != access(RO_PRODUCT_OTPDATA_PARTITION_PATH, F_OK)) {
        OTP_LOGE("%s:access %s failed.",__FUNCTION__,RO_PRODUCT_OTPDATA_PARTITION_PATH);
        ret = mkdir(RO_PRODUCT_OTPDATA_PARTITION_PATH, 0777);
        if (-1 == ret) {
            OTP_LOGE("%s:mkdir %s failed.",__FUNCTION__,RO_PRODUCT_OTPDATA_PARTITION_PATH);
            ret = 0;
        }
        else{
            ret = 1;
        }
    }

    if(ret != 0){
        //system("chmod 777 /data/vendor/local/otpdata/otpdata.txt");
        ret = remove(_ramdiskCfg[idx].image_path);
        OTP_LOGD("%s:remove file %s idx:%d \n",__FUNCTION__,_ramdiskCfg[idx].image_path,idx);
        fileVendorHandle = fopen(_ramdiskCfg[idx].image_path, "w+");
    }

    if (fileVendorHandle != NULL) {
        OTP_LOGE("%s:open %s success.",__FUNCTION__,_ramdiskCfg[idx].image_path);
        if (RAMOTP_HEAD_SIZE != fwrite(header_buf, sizeof(char), RAMOTP_HEAD_SIZE, fileVendorHandle)) {
            ret = 0;
            OTP_LOGD("%s:partId%x:origin image header write fail!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
        } else {
            ret = 1;
            OTP_LOGD("%s:write origin header",__FUNCTION__);
        }

        if (ret && (size == fwrite(buf, sizeof(char), size, fileVendorHandle))) {
            OTP_LOGD("%s:partId%x:origin image write success!:%d \n",__FUNCTION__,_ramdiskCfg[idx].partId,size);
            ret = 1;
        }else{
            OTP_LOGD("%s:partId%x:origin image write fail!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
            ret = 0;
        }

        if(ret){
            OTP_LOGD("%s: fflush =%s", __FUNCTION__,_ramdiskCfg[idx].image_path);
            fflush(fileVendorHandle);
            fd = fileno(fileVendorHandle);
            
            if(fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                OTP_LOGD("%s: fileno() =%s", __FUNCTION__,_ramdiskCfg[idx].image_path);
                ret = 0;
            }
        }

        fclose(fileVendorHandle);
        OTP_LOGD("%s:partId%x:image write finished %d!\n",__FUNCTION__,_ramdiskCfg[idx].partId,ret);
    }else{
        OTP_LOGE("%s:open:%s failed",__FUNCTION__,_ramdiskCfg[idx].image_path);
        ret = 0;
    }
    //test code
    otp_dump_data(buf,size);
    return ret;
}

/*
1 get Ecc first
2 write  imageBakPath
3 write imagePath

note:
        write golden data
*/
BOOLEAN ramDisk_Write_Golden(RAMDISK_HANDLE handle, uint8* buf, uint16 size) {
    int ret;
    FILE * fileProductinfoHandle = NULL;
    int idx;
    int fd = -1;

    char header_buf[RAMOTP_HEAD_SIZE];
    otp_header_t* header_ptr = NULL;

    idx = _getIdx(handle);
    if (-1 == idx) {
        return 0;
    }

    return 1;
    
    memset(header_buf, 0x00, RAMOTP_HEAD_SIZE);
    header_ptr = header_buf;
    header_ptr->magic = OTP_HEAD_MAGIC;
    header_ptr->len = size;
    header_ptr->version = OTP_VERSION;
    OTP_LOGD("%s:partId%x:checksum_64=%d",__FUNCTION__,_ramdiskCfg[idx].partId,checksum_64);
    if(checksum_64)
        header_ptr->checksum = (uint32)calc_checksum64(buf, size);
    else
        header_ptr->checksum = (uint32)calc_checksum(buf, size);

    // 1 get Ecc

    if (0 != access("/system/etc/otpdata", F_OK)) {
        OTP_LOGE("%s:/system/etc/otpdata.",__FUNCTION__);
        ret = mkdir("/system/etc/otpdata", S_IRWXU | S_IRWXG | S_IRWXO);
        if (-1 == ret) {
            OTP_LOGE("%s:mkdir /system/etc/otpdata failed.",__FUNCTION__);
            return 0;
        }
    }

    fileProductinfoHandle = fopen(_ramdiskCfg[idx].imageGolden_path, "w+");

    if (fileProductinfoHandle != NULL) {
        if (RAMOTP_HEAD_SIZE != fwrite(header_buf, sizeof(char), RAMOTP_HEAD_SIZE, fileProductinfoHandle)) {
            ret = 0;
            OTP_LOGD("%s:partId%x:origin image header write fail!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
        } else {
            OTP_LOGD("%s:write origin header",__FUNCTION__);
        }

        if (size != fwrite(buf, sizeof(char), size, fileProductinfoHandle)) {
            OTP_LOGD("%s:partId%x:origin image write fail!\n",__FUNCTION__,_ramdiskCfg[idx].partId);
            ret = 0;
        }

        fflush(fileProductinfoHandle);
        fd = fileno(fileProductinfoHandle);

        if(fd > 0) {
            fsync(fd);
            ret = 1;
        } else {
            OTP_LOGD("%s: fileno() =%s", __FUNCTION__,_ramdiskCfg[idx].image_path);
            ret = 0;
        }

        fclose(fileProductinfoHandle);
        OTP_LOGD("%s:partId%x:image write finished %d!\n",__FUNCTION__,_ramdiskCfg[idx].partId,ret);
    }
    
    //test code
    otp_dump_data(buf,size);

    return ret;
}


