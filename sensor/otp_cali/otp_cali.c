#include "otp_cali.h"

static cmr_u16 calc_checksum(cmr_u8 *dat, cmr_u16 len);
static cmr_u8 check_checksum(cmr_u8 *buf, cmr_u16 size, cmr_u16 checksum);

static cmr_u16 calc_checksum(cmr_u8 *dat, cmr_u16 len) {
    cmr_u16 num = 0;
    cmr_u32 chkSum = 0;
    while (len > 1) {
        num = (cmr_u16)(*dat);
        dat++;
        num |= (((cmr_u16)(*dat)) << 8);
        dat++;
        chkSum += (cmr_u32)num;
        len -= 2;
    }
    if (len) {
        chkSum += *dat;
    }
    chkSum = (chkSum >> 16) + (chkSum & 0xffff);
    chkSum += (chkSum >> 16);
    return (~chkSum);
}

static cmr_u8 check_checksum(cmr_u8 *buf, cmr_u16 size, cmr_u16 checksum) {
    cmr_u16 crc;

    crc = calc_checksum(buf, size);
    SENSOR_LOGI("crc = 0x%x, checksum=0x%x", crc, checksum);
    if (crc == checksum) {
        return 1;
    }

    return 0;
}

cmr_u16 read_calibration_otp_from_file(cmr_u8 *buf, cmr_u8 dual_flag) {
    int fd = 0;
    int ret = 0;
    cmr_u32 rcount = 0;
    FILE *fileHandle = NULL;
    char *OtpDataPath = NULL;
    char *OtpBkDataPath = NULL;
    cmr_u8 header[CALI_OTP_HEAD_SIZE] = {0};
    otp_header_t *header_ptr = (otp_header_t *)header;
    SENSOR_LOGI("E");

    switch (dual_flag) {
    case CALIBRATION_FLAG_BOKEH:
        OtpDataPath = OTP_CALI_BOKEH_PATH;
        OtpBkDataPath = OTPBK_CALI_BOKEH_PATH;
        break;
    case CALIBRATION_FLAG_T_W:
        OtpDataPath = OTP_CALI_T_W_PATH;
        OtpBkDataPath = OTPBK_CALI_T_W_PATH;
        break;
    case CALIBRATION_FLAG_SPW:
        OtpDataPath = OTP_CALI_SPW_PATH;
        OtpBkDataPath = OTPBK_CALI_SPW_PATH;
        break;
    case CALIBRATION_FLAG_3D_STL:
        OtpDataPath = OTP_CALI_3D_STL_PATH;
        OtpBkDataPath = OTPBK_CALI_3D_STL_PATH;
        break;
    case CALIBRATION_FLAG_OZ1:
        OtpDataPath = OTP_CALI_OZ1_PATH;
        OtpBkDataPath = OTPBK_CALI_OZ1_PATH;
        break;
    case CALIBRATION_FLAG_OZ2:
        OtpDataPath = OTP_CALI_OZ2_PATH;
        OtpBkDataPath = OTPBK_CALI_OZ2_PATH;
        break;
    default:
        SENSOR_LOGE("Invalid cali data type:%d", dual_flag);
        return 0;
    }

    // 1 read origin file
    memset(buf, 0xFF, SPRD_DUAL_OTP_SIZE);
    memset(header, 0x00, CALI_OTP_HEAD_SIZE);
    do {
        if (access(OtpDataPath, F_OK) == 0) {
            fileHandle = fopen(OtpDataPath, "r");
            if (NULL == fileHandle) {
                SENSOR_LOGE("open %s failed!", OtpDataPath);
                break;
            }

            rcount =
                fread(header, sizeof(char), CALI_OTP_HEAD_SIZE, fileHandle);
            if (rcount != CALI_OTP_HEAD_SIZE) {
                SENSOR_LOGE("read %s header count error!", OtpDataPath);
                fclose(fileHandle);
                break;
            }

            rcount = fread(buf, sizeof(char), header_ptr->len, fileHandle);
            if (rcount != header_ptr->len) {
                SENSOR_LOGE("read %s buf count error!", OtpDataPath);
                fclose(fileHandle);
                break;
            }

            fclose(fileHandle);
        } else {
            SENSOR_LOGI(" %s file dosen't exist!", OtpDataPath);
            break;
        }

        if (check_checksum(buf, header_ptr->len, header_ptr->checksum)) {
            SENSOR_LOGI("read %s success!", OtpDataPath);
            if (header_ptr->has_calibration) {
                return header_ptr->len;
            }
        }

        SENSOR_LOGI("read %s error!", OtpDataPath);

    } while (0);

    // 2 read backup file
    memset(buf, 0xFF, SPRD_DUAL_OTP_SIZE);
    memset(header, 0x00, CALI_OTP_HEAD_SIZE);
    do {
        if (access(OtpBkDataPath, F_OK) == 0) {
            fileHandle = fopen(OtpBkDataPath, "r");

            if (NULL == fileHandle) {
                SENSOR_LOGE("open %s failed!", OtpBkDataPath);
                break;
            }

            rcount =
                fread(header, sizeof(char), CALI_OTP_HEAD_SIZE, fileHandle);
            if (rcount != CALI_OTP_HEAD_SIZE) {
                SENSOR_LOGE("read %s header count error!", OtpBkDataPath);
                fclose(fileHandle);
                break;
            }

            rcount = fread(buf, sizeof(char), header_ptr->len, fileHandle);
            if (rcount != header_ptr->len) {
                SENSOR_LOGE("read %s buf count error!", OtpBkDataPath);
                fclose(fileHandle);
                break;
            }

            fclose(fileHandle);
        } else {
            SENSOR_LOGI(" %s file dosen't exist!", OtpBkDataPath);
            break;
        }

        if (check_checksum(buf, header_ptr->len, header_ptr->checksum)) {
            // write otp date to origin path
            if (0 != access("/data/vendor/local/otpdata", F_OK)) {
                ret = mkdir("/data/vendor/local/otpdata",
                            S_IRWXU | S_IRWXG | S_IRWXO);
                if (-1 == ret) {
                    SENSOR_LOGE("mkdir /data/vendor/local/otpdata failed!");
                    if (header_ptr->has_calibration) {
                        return header_ptr->len;
                    } else {
                        return 0;
                    }
                }
            }

            ret = remove(OtpDataPath);
            SENSOR_LOGI("remove file:%s, ret:%d", OtpDataPath, ret);
            fileHandle = fopen(OtpDataPath, "w+");

            if (fileHandle != NULL) {
                if (CALI_OTP_HEAD_SIZE != fwrite(header, sizeof(char),
                                                 CALI_OTP_HEAD_SIZE,
                                                 fileHandle)) {
                    ret = 0;
                    SENSOR_LOGE("%s:origin image header write fail!",
                                OtpDataPath);
                } else {
                    ret = 1;
                    SENSOR_LOGI("%s:write origin header success!", OtpDataPath);
                }

                if (ret &&
                    (header_ptr->len !=
                     fwrite(buf, sizeof(char), header_ptr->len, fileHandle))) {
                    SENSOR_LOGE("%s:origin image write fail!", OtpDataPath);
                    ret = 0;
                }

                if (ret) {
                    fflush(fileHandle);
                    fd = fileno(fileHandle);

                    if (fd > 0) {
                        fsync(fd);
                        ret = 1;
                    } else {
                        SENSOR_LOGI("fileno() =%s", OtpDataPath);
                        ret = 0;
                    }
                }

                fclose(fileHandle);
                SENSOR_LOGI("%s:image write finished, ret:%d", OtpDataPath,
                            ret);
            } else {
                SENSOR_LOGI("open file %s failed!", OtpDataPath);
            }

            SENSOR_LOGI("%s read success!", OtpBkDataPath);

            if (header_ptr->has_calibration) {
                return header_ptr->len;
            }

            return 0;
        }

        SENSOR_LOGI("read %s error!", OtpBkDataPath);
    } while (0);

    return 0;
}

cmr_u8 write_calibration_otp_to_file(cmr_u8 *buf, cmr_u8 dual_flag,
                                     cmr_u16 otp_size) {
    int ret = 1;
    int fd = -1;
    FILE *fileProductinfoHandle = NULL;
    FILE *fileVendorHandle = NULL;
    char *OtpDataPath = NULL;
    char *OtpBkDataPath = NULL;
    cmr_u8 header_buf[CALI_OTP_HEAD_SIZE] = {0};
    otp_header_t *header_ptr = header_buf;
    SENSOR_LOGI("E");

    switch (dual_flag) {
    case CALIBRATION_FLAG_BOKEH:
        OtpDataPath = OTP_CALI_BOKEH_PATH;
        OtpBkDataPath = OTPBK_CALI_BOKEH_PATH;
        break;
    case CALIBRATION_FLAG_T_W:
        OtpDataPath = OTP_CALI_T_W_PATH;
        OtpBkDataPath = OTPBK_CALI_T_W_PATH;
        break;
    case CALIBRATION_FLAG_SPW:
        OtpDataPath = OTP_CALI_SPW_PATH;
        OtpBkDataPath = OTPBK_CALI_SPW_PATH;
        break;
    case CALIBRATION_FLAG_3D_STL:
        OtpDataPath = OTP_CALI_3D_STL_PATH;
        OtpBkDataPath = OTPBK_CALI_3D_STL_PATH;
        break;
    case CALIBRATION_FLAG_OZ1:
        OtpDataPath = OTP_CALI_OZ1_PATH;
        OtpBkDataPath = OTPBK_CALI_OZ1_PATH;
        break;
    case CALIBRATION_FLAG_OZ2:
        OtpDataPath = OTP_CALI_OZ2_PATH;
        OtpBkDataPath = OTPBK_CALI_OZ2_PATH;
        break;
    default:
        SENSOR_LOGE("Invalid cali data type:%d", dual_flag);
        return 0;
    }

    header_ptr->magic = OTP_HEAD_MAGIC;
    header_ptr->len = otp_size;
    header_ptr->checksum = (cmr_u32)calc_checksum(buf, otp_size);
    header_ptr->version = OTP_VERSION;
    header_ptr->data_type = OTPDATA_TYPE_CALIBRATION;
    header_ptr->has_calibration = 1;

    // 1 write backup file
    if (0 != access("/mnt/vendor/productinfo/otpdata", F_OK)) {
        SENSOR_LOGE("access /mnt/vendor/productinfo/otpdata failed!");
        ret = mkdir("/mnt/vendor/productinfo/otpdata",
                    S_IRWXU | S_IRWXG | S_IRWXO);
        if (-1 == ret) {
            SENSOR_LOGE("mkdir /mnt/vendor/productinfo/otpdata failed!");
            ret = 0;
        } else {
            ret = 1;
        }
    }

    if (ret != 0) {
        ret = remove(OtpBkDataPath);
        SENSOR_LOGI("remove file:%s", OtpBkDataPath);
        fileProductinfoHandle = fopen(OtpBkDataPath, "w+");
    }

    if (fileProductinfoHandle != NULL) {
        if (CALI_OTP_HEAD_SIZE != fwrite(header_buf, sizeof(char),
                                         CALI_OTP_HEAD_SIZE,
                                         fileProductinfoHandle)) {
            ret = 0;
            SENSOR_LOGE("%s :backup image header write fail!", OtpBkDataPath);
        } else {
            ret = 1;
            SENSOR_LOGI("%s: backup image header write success!",
                        OtpBkDataPath);
        }

        if (ret && (otp_size == fwrite(buf, sizeof(char), otp_size,
                                       fileProductinfoHandle))) {
            SENSOR_LOGI("%s:backup image write success!", OtpBkDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:backup image write failed!", OtpBkDataPath);
            ret = 0;
        }

        if (ret) {

            fflush(fileProductinfoHandle);
            fd = fileno(fileProductinfoHandle);

            if (fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                SENSOR_LOGI("fileno() =%s", OtpBkDataPath);
                ret = 0;
            }
        }

        fclose(fileProductinfoHandle);
        SENSOR_LOGI("%s:image write finished!", OtpBkDataPath);
    } else {
        ret = 0;
        SENSOR_LOGE("%s:open backup file failed!", OtpBkDataPath);
    }

    // 2 write origin file
    if (0 != access("/data/vendor/local/otpdata", F_OK)) {
        SENSOR_LOGE("access /data/vendor/local/otpdata failed!");
        ret = mkdir("/data/vendor/local/otpdata", 0777);
        if (-1 == ret) {
            SENSOR_LOGE("mkdir /data/vendor/local/otpdata failed!");
            ret = 0;
        } else {
            ret = 1;
        }
    }

    if (ret != 0) {
        ret = remove(OtpDataPath);
        SENSOR_LOGI("remove file:%s", OtpDataPath);
        fileVendorHandle = fopen(OtpDataPath, "w+");
    }

    if (fileVendorHandle != NULL) {
        SENSOR_LOGI("open %s success!", OtpDataPath);
        if (CALI_OTP_HEAD_SIZE != fwrite(header_buf, sizeof(char),
                                         CALI_OTP_HEAD_SIZE,
                                         fileVendorHandle)) {
            ret = 0;
            SENSOR_LOGE("%s:origin image header write fail!", OtpDataPath);
        } else {
            ret = 1;
            SENSOR_LOGI("%s:origin image header write success!", OtpDataPath);
        }

        if (ret && (otp_size ==
                    fwrite(buf, sizeof(char), otp_size, fileVendorHandle))) {
            SENSOR_LOGI("%s:origin image write success!", OtpDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:origin image write fail!", OtpDataPath);
            ret = 0;
        }

        if (ret) {
            SENSOR_LOGI("fflush =%s", OtpDataPath);
            fflush(fileVendorHandle);
            fd = fileno(fileVendorHandle);

            if (fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                SENSOR_LOGI("fileno() =%s", OtpDataPath);
                ret = 0;
            }
        }

        fclose(fileVendorHandle);
        SENSOR_LOGI("%s:image write finished!", OtpDataPath);
    } else {
        SENSOR_LOGE("open:%s failed!", OtpDataPath);
        ret = 0;
    }

    return ret;
}
