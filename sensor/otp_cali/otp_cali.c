#define LOG_TAG "otp_cali"
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
    SENSOR_LOGD("crc = 0x%x, checksum=0x%x", crc, checksum);
    if (crc == checksum) {
        return 1;
    }

    return 0;
}

cmr_u16 read_calibration_cmei(cmr_u8 dual_flag, cmr_u8 *cmei_buf) {
    int fd = 0;
    int ret = 0;
    cmr_u32 rcount = 0;
    FILE *fileHandle = NULL;
    char *OtpDataPath = NULL;
    char *OtpBkDataPath = NULL;
    cmr_u8 header[CALI_OTP_HEAD_SIZE] = {0};
    cmr_u8 otp_buf[SPRD_DUAL_OTP_SIZE] = {0};
    otp_header_t *header_ptr = (otp_header_t *)header;
    SENSOR_LOGD("E");

    switch (dual_flag) {
    case CALIBRATION_FLAG_BOKEH:
        OtpDataPath = OTP_CALI_BOKEH_PATH;
        OtpBkDataPath = OTPBK_CALI_BOKEH_PATH;
        break;
    case CALIBRATION_FLAG_OZ1:
        OtpDataPath = OTP_CALI_OZ1_PATH;
        OtpBkDataPath = OTPBK_CALI_OZ1_PATH;
        break;
    case CALIBRATION_FLAG_OZ2:
        OtpDataPath = OTP_CALI_OZ2_PATH;
        OtpBkDataPath = OTPBK_CALI_OZ2_PATH;
        break;
    case CALIBRATION_FLAG_BOKEH_MANUAL_CMEI:
        OtpDataPath = OTP_CALI_BOKEH_MANUAL_CMEI_PATH;
        OtpBkDataPath = OTPBK_CALI_BOKEH_MANUAL_CMEI_PATH;
        break;
    default:
        SENSOR_LOGE("Invalid cali data type:%d", dual_flag);
        return 0;
    }

    // 1 read origin file
    memset(header, 0x00, CALI_OTP_HEAD_SIZE);
    memset(cmei_buf, 0x00, MAX_CMEI_SIZE);
    do {
        if (NULL == OtpDataPath) {
            SENSOR_LOGE("OtpDataPath is NULL !");
            break;
        }
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

            if(header_ptr->version != OTP_VERSION2) {
                SENSOR_LOGE("OTP version wrong, hasnot cmei data!");
                fclose(fileHandle);
                break;
            }

            if(fseek(fileHandle, (CALI_OTP_HEAD_SIZE + header_ptr->otp_len), 0))
            {
                SENSOR_LOGE("fseek wrong");
                fclose(fileHandle);
                break;
            }

            if (header_ptr->cmei_len > 0) {
                if (header_ptr->cmei_len > MAX_CMEI_SIZE) {
                    SENSOR_LOGE("read %d byte, overflow cmei_buf max_size %d",
                            header_ptr->cmei_len, MAX_CMEI_SIZE);
                    fclose(fileHandle);
                    break;
                }
            rcount = fread(cmei_buf, sizeof(char), header_ptr->cmei_len, fileHandle);
            if (rcount != header_ptr->cmei_len) {
                SENSOR_LOGE("read %s cmei count error!", OtpDataPath);
                fclose(fileHandle);
                break;
                }
            }

            fclose(fileHandle);
            return header_ptr->cmei_len;
        } else {
            SENSOR_LOGI("%s file dosen't exist!", OtpDataPath);
            break;
        }
    } while (0);

    // 2 read backup file
    memset(header, 0x00, CALI_OTP_HEAD_SIZE);
    memset(otp_buf, 0x00, SPRD_DUAL_OTP_SIZE);
    memset(cmei_buf, 0x00, MAX_CMEI_SIZE);
    do {
        if (NULL == OtpBkDataPath) {
            SENSOR_LOGE("OtpBkDataPath is NULL !");
            break;
        }
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

            if (header_ptr->otp_len > 0) {
                if (header_ptr->otp_len > SPRD_DUAL_OTP_SIZE) {
                    SENSOR_LOGE("read %d byte, overflow otp_buf max_size %d",
                            header_ptr->otp_len, SPRD_DUAL_OTP_SIZE);
                    fclose(fileHandle);
                    break;
                }
            rcount = fread(otp_buf, sizeof(char), header_ptr->otp_len, fileHandle);
            if (rcount != header_ptr->otp_len) {
                SENSOR_LOGE("read %s buf count error!", OtpBkDataPath);
                fclose(fileHandle);
                break;
                }
            }

            if(header_ptr->version != OTP_VERSION2) {
                SENSOR_LOGE("OTP version wrong, hasnot cmei data!");
                fclose(fileHandle);
                break;
            }

            if (header_ptr->cmei_len > 0) {
                if (header_ptr->cmei_len > MAX_CMEI_SIZE) {
                    SENSOR_LOGE("read %d byte, overflow cmei_buf max_size %d",
                            header_ptr->cmei_len, MAX_CMEI_SIZE);
                    fclose(fileHandle);
                    break;
                }
            rcount = fread(cmei_buf, sizeof(char), header_ptr->cmei_len, fileHandle);
            if (rcount != header_ptr->cmei_len) {
                SENSOR_LOGE("read %s cmei buf count error!", OtpBkDataPath);
                fclose(fileHandle);
                break;
                }
            }

            fclose(fileHandle);
        } else {
            SENSOR_LOGI("%s file dosen't exist!", OtpBkDataPath);
            break;
        }

        if (check_checksum(otp_buf, header_ptr->otp_len, header_ptr->otp_checksum)) {
            // write otp date to origin path
            if (0 != access("/data/vendor/local/otpdata", F_OK)) {
                ret = mkdir("/data/vendor/local/otpdata",
                            S_IRWXU | S_IRWXG | S_IRWXO);
                if (-1 == ret) {
                    SENSOR_LOGE("mkdir /data/vendor/local/otpdata failed!");
                        return header_ptr->cmei_len;
                }
            }

            ret = remove(OtpDataPath);
            SENSOR_LOGD("remove file:%s, ret:%d", OtpDataPath, ret);
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
                    SENSOR_LOGI("%s:write origin header success", OtpDataPath);
                }

                if (header_ptr->otp_len > 0) {
                if (ret &&
                    (header_ptr->otp_len !=
                     fwrite(otp_buf, sizeof(char), header_ptr->otp_len, fileHandle))) {
                    SENSOR_LOGE("%s:origin image write fail!", OtpDataPath);
                    ret = 0;
                    }
                }

                if (header_ptr->cmei_len > 0) {
                if (ret &&
                    (header_ptr->cmei_len !=
                     fwrite(cmei_buf, sizeof(char), header_ptr->cmei_len, fileHandle))) {
                    SENSOR_LOGE("%s:origin cmei write fail!", OtpDataPath);
                    ret = 0;
                    }
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
                SENSOR_LOGE("open file %s failed!", OtpDataPath);
            }

            SENSOR_LOGI("%s read success", OtpBkDataPath);

            return header_ptr->cmei_len;
        }

        SENSOR_LOGI("read %s error!", OtpBkDataPath);
        break;
    } while (0);

    return 0;
}

cmr_u16 read_calibration_otp(cmr_u8 dual_flag, cmr_u8 *otp_buf,
                                                   cmr_u8 *multi_module_name) {
    int fd = 0;
    int ret = 0;
    cmr_u32 rcount = 0;
    FILE *fileHandle = NULL;
    char *OtpDataPath = NULL;
    char otp_data_path[CALI_OTP_FILE_PATH_LENGTH] = {0};
    char *OtpBkDataPath = NULL;
    cmr_u8 header[CALI_OTP_HEAD_SIZE] = {0};
    cmr_u8 cmei_buf[MAX_CMEI_SIZE] = {0};
    otp_header_t *header_ptr = (otp_header_t *)header;
    SENSOR_LOGD("E");

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
    case CALIBRATION_FLAG_BOKEH_GLD:
        OtpDataPath = OTP_CALI_BOKEH_GLD_PATH;
        OtpBkDataPath = NULL;
        break;
    case CALIBRATION_FLAG_BOKEH_GLD2:
        if (NULL == multi_module_name) {
        OtpDataPath = OTP_CALI_BOKEH_GLD2_PATH;
        } else {
            OtpDataPath = &otp_data_path;
            strcat(OtpDataPath, "/vendor/etc/otpdata/otp_cali_bokeh_gld2_");
            strcat(OtpDataPath, multi_module_name);
            strcat(OtpDataPath, ".bin");
        }
        OtpBkDataPath = NULL;
        SENSOR_LOGI("bokeh gld online file path:%s", OtpDataPath);
        break;
    case CALIBRATION_FLAG_OZ1_GLD:
        OtpDataPath = OTP_CALI_OZ1_GLD_PATH;
        OtpBkDataPath = NULL;
        break;
    case CALIBRATION_FLAG_OZ2_GLD:
        OtpDataPath = OTP_CALI_OZ2_GLD_PATH;
        OtpBkDataPath = NULL;
        break;
    default:
        SENSOR_LOGE("Invalid cali data type:%d", dual_flag);
        return 0;
    }

    // 1 read origin file
    memset(header, 0x00, CALI_OTP_HEAD_SIZE);
    memset(otp_buf, 0x00, SPRD_DUAL_OTP_SIZE);
    do {
        if (NULL == OtpDataPath) {
            SENSOR_LOGE("OtpDataPath is NULL !");
            break;
        }
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

            if (header_ptr->otp_len > 0) {
                if (header_ptr->otp_len > SPRD_DUAL_OTP_SIZE) {
                    SENSOR_LOGE("read %d byte, overflow otp_buf max_size %d",
                            header_ptr->otp_len, SPRD_DUAL_OTP_SIZE);
                    fclose(fileHandle);
                    break;
                }
            rcount = fread(otp_buf, sizeof(char), header_ptr->otp_len, fileHandle);
            if (rcount != header_ptr->otp_len) {
                SENSOR_LOGE("read %s buf count error!", OtpDataPath);
                fclose(fileHandle);
                break;
                }
            }

            fclose(fileHandle);
        } else {
            SENSOR_LOGI("%s file dosen't exist!", OtpDataPath);
            break;
        }

        if (check_checksum(otp_buf, header_ptr->otp_len, header_ptr->otp_checksum)) {
            SENSOR_LOGI("read %s success", OtpDataPath);
            if (header_ptr->has_calibration) {
                return header_ptr->otp_len;
            }
        }

        SENSOR_LOGI("read %s error!", OtpDataPath);
        break;
    } while (0);

    // 2 read backup file
    memset(header, 0x00, CALI_OTP_HEAD_SIZE);
    memset(otp_buf, 0x00, SPRD_DUAL_OTP_SIZE);
    memset(cmei_buf, 0x00, MAX_CMEI_SIZE);
    do {
        if (NULL == OtpBkDataPath) {
            SENSOR_LOGE("OtpBkDataPath is NULL !");
            break;
        }
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

            if (header_ptr->otp_len > 0) {
                if (header_ptr->otp_len > SPRD_DUAL_OTP_SIZE) {
                    SENSOR_LOGE("read %d byte, overflow otp_buf max_size %d",
                            header_ptr->otp_len, SPRD_DUAL_OTP_SIZE);
                    fclose(fileHandle);
                    break;
                }
            rcount = fread(otp_buf, sizeof(char), header_ptr->otp_len, fileHandle);
            if (rcount != header_ptr->otp_len) {
                SENSOR_LOGE("read %s buf count error!", OtpBkDataPath);
                fclose(fileHandle);
                break;
                }
            }

            if ((OTP_VERSION2  == header_ptr->version) && (header_ptr->cmei_len > 0)) {
                if (header_ptr->cmei_len > MAX_CMEI_SIZE) {
                    SENSOR_LOGE("read %d byte, overflow cmei_buf max_size %d",
                            header_ptr->cmei_len, MAX_CMEI_SIZE);
                    fclose(fileHandle);
                    break;
                }
                rcount = fread(cmei_buf, sizeof(char), header_ptr->cmei_len, fileHandle);
                if (rcount != header_ptr->cmei_len) {
                    SENSOR_LOGE("read %s cmei buf count error!", OtpBkDataPath);
                    fclose(fileHandle);
                    break;
                }
            }

            fclose(fileHandle);
        } else {
            SENSOR_LOGI("%s file dosen't exist!", OtpBkDataPath);
            break;
        }

        if (check_checksum(otp_buf, header_ptr->otp_len, header_ptr->otp_checksum)) {
            // write otp date to origin path
            if (0 != access("/data/vendor/local/otpdata", F_OK)) {
                ret = mkdir("/data/vendor/local/otpdata",
                            S_IRWXU | S_IRWXG | S_IRWXO);
                if (-1 == ret) {
                    SENSOR_LOGE("mkdir /data/vendor/local/otpdata failed!");
                    if (header_ptr->has_calibration) {
                        return header_ptr->otp_len;
                    } else {
                        return 0;
                    }
                }
            }

            ret = remove(OtpDataPath);
            SENSOR_LOGD("remove file:%s, ret:%d", OtpDataPath, ret);
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
                    SENSOR_LOGI("%s:write origin header success", OtpDataPath);
                }

                if (header_ptr->otp_len > 0) {
                if (ret &&
                    (header_ptr->otp_len !=
                     fwrite(otp_buf, sizeof(char), header_ptr->otp_len, fileHandle))) {
                    SENSOR_LOGE("%s:origin image write fail!", OtpDataPath);
                    ret = 0;
                    }
                }

                if ((OTP_VERSION2  == header_ptr->version) && (header_ptr->cmei_len > 0)) {
                    if (ret &&
                        (header_ptr->cmei_len!=
                        fwrite(cmei_buf, sizeof(char), header_ptr->cmei_len, fileHandle))) {
                        SENSOR_LOGE("%s:origin cmei write fail!", OtpDataPath);
                        ret = 0;
                    }
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
                SENSOR_LOGE("open file %s failed!", OtpDataPath);
            }

            SENSOR_LOGI("%s read success", OtpBkDataPath);

            if (header_ptr->has_calibration) {
                return header_ptr->otp_len;
            }

            return 0;
        }

        SENSOR_LOGI("read %s error!", OtpBkDataPath);
    } while (0);

    return 0;
}

cmr_u8 write_calibration_otp_with_cmei(cmr_u8 dual_flag, cmr_u8 *otp_buf,
                                     cmr_u16 otp_size, cmr_u8 *cmei_buf, cmr_u16 cmei_size) {
    int ret = 1;
    int fd = -1;
    FILE *fileProductinfoHandle = NULL;
    FILE *fileVendorHandle = NULL;
    char *OtpDataPath = NULL;
    char *OtpBkDataPath = NULL;
    cmr_u8 header_buf[CALI_OTP_HEAD_SIZE] = {0};
    otp_header_t *header_ptr = header_buf;
    SENSOR_LOGD("E");

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
    case CALIBRATION_FLAG_BOKEH_MANUAL_CMEI:
        OtpDataPath = OTP_CALI_BOKEH_MANUAL_CMEI_PATH;
        OtpBkDataPath = OTPBK_CALI_BOKEH_MANUAL_CMEI_PATH;
        break;
    default:
        SENSOR_LOGE("Invalid cali data type:%d", dual_flag);
        return 0;
    }

    header_ptr->magic = OTP_HEAD_MAGIC;
    header_ptr->otp_len = otp_size;
    header_ptr->otp_checksum = (cmr_u32)calc_checksum(otp_buf, otp_size);
    header_ptr->version = OTP_VERSION2;
    header_ptr->data_type = OTPDATA_TYPE_CALIBRATION;
    header_ptr->has_calibration = 1;
    header_ptr->cmei_len = cmei_size;

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
        SENSOR_LOGD("remove file:%s, ret:%d", OtpBkDataPath, ret);
        fileProductinfoHandle = fopen(OtpBkDataPath, "w+");
    }

    if (fileProductinfoHandle != NULL) {
        if (CALI_OTP_HEAD_SIZE != fwrite(header_buf, sizeof(char),
                                         CALI_OTP_HEAD_SIZE,
                                         fileProductinfoHandle)) {
            ret = 0;
            SENSOR_LOGE("%s:backup image header write fail!", OtpBkDataPath);
        } else {
            ret = 1;
            SENSOR_LOGI("%s:backup image header write success",
                        OtpBkDataPath);
        }

        if ((NULL != otp_buf) && (otp_size > 0) ) {
        if (ret && (otp_size == fwrite(otp_buf, sizeof(char), otp_size,
                                       fileProductinfoHandle))) {
            SENSOR_LOGI("%s:backup image write success", OtpBkDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:backup image write failed!", OtpBkDataPath);
            ret = 0;
        }
        }

        if ((NULL != cmei_buf) && (cmei_size > 0) ) {
        if (ret && (cmei_size == fwrite(cmei_buf, sizeof(char), cmei_size,
                                       fileProductinfoHandle))) {
            SENSOR_LOGI("%s:backup cmei write success", OtpBkDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:backup cmei write failed!", OtpBkDataPath);
            ret = 0;
            }
        }

        if (ret) {

            fflush(fileProductinfoHandle);
            fd = fileno(fileProductinfoHandle);

            if (fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                SENSOR_LOGI("fileno() = %s", OtpBkDataPath);
                ret = 0;
            }
        }

        fclose(fileProductinfoHandle);
        SENSOR_LOGI("%s:image write finished", OtpBkDataPath);
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
        SENSOR_LOGD("remove file:%s, ret:%d", OtpDataPath, ret);
        fileVendorHandle = fopen(OtpDataPath, "w+");
    }

    if (fileVendorHandle != NULL) {
        SENSOR_LOGI("open %s success", OtpDataPath);
        if (CALI_OTP_HEAD_SIZE != fwrite(header_buf, sizeof(char),
                                         CALI_OTP_HEAD_SIZE,
                                         fileVendorHandle)) {
            ret = 0;
            SENSOR_LOGE("%s:origin image header write fail!", OtpDataPath);
        } else {
            ret = 1;
            SENSOR_LOGI("%s:origin image header write success", OtpDataPath);
        }

        if ((NULL != otp_buf) && (otp_size > 0) ) {
        if (ret && (otp_size ==
                    fwrite(otp_buf, sizeof(char), otp_size, fileVendorHandle))) {
            SENSOR_LOGI("%s:origin image write success", OtpDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:origin image write fail!", OtpDataPath);
            ret = 0;
        }
        }

        if ((NULL != cmei_buf) && (cmei_size > 0) ) {
        if (ret && (cmei_size ==
                    fwrite(cmei_buf, sizeof(char), cmei_size, fileVendorHandle))) {
            SENSOR_LOGI("%s:origin cmei write success", OtpDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:origin cmei write fail!", OtpDataPath);
            ret = 0;
            }
        }

        if (ret) {
            SENSOR_LOGI("fflush = %s", OtpDataPath);
            fflush(fileVendorHandle);
            fd = fileno(fileVendorHandle);

            if (fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                SENSOR_LOGI("fileno() = %s", OtpDataPath);
                ret = 0;
            }
        }

        fclose(fileVendorHandle);
        SENSOR_LOGI("%s:image write finished", OtpDataPath);
    } else {
        SENSOR_LOGE("open:%s failed!", OtpDataPath);
        ret = 0;
    }

    return ret;
}

cmr_u8 write_calibration_otp_no_cmei(cmr_u8 dual_flag, cmr_u8 *otp_buf,
                                     cmr_u16 otp_size) {
    int ret = 1;
    int fd = -1;
    FILE *fileProductinfoHandle = NULL;
    FILE *fileVendorHandle = NULL;
    char *OtpDataPath = NULL;
    char *OtpBkDataPath = NULL;
    cmr_u8 header_buf[CALI_OTP_HEAD_SIZE] = {0};
    otp_header_t *header_ptr = header_buf;
    SENSOR_LOGD("E");

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
    header_ptr->otp_len = otp_size;
    header_ptr->otp_checksum = (cmr_u32)calc_checksum(otp_buf, otp_size);
    header_ptr->version = OTP_VERSION1;
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
        SENSOR_LOGD("remove file:%s, ret:%d", OtpBkDataPath, ret);
        fileProductinfoHandle = fopen(OtpBkDataPath, "w+");
    }

    if (fileProductinfoHandle != NULL) {
        if (CALI_OTP_HEAD_SIZE != fwrite(header_buf, sizeof(char),
                                         CALI_OTP_HEAD_SIZE,
                                         fileProductinfoHandle)) {
            ret = 0;
            SENSOR_LOGE("%s:backup image header write fail!", OtpBkDataPath);
        } else {
            ret = 1;
            SENSOR_LOGI("%s:backup image header write success",
                        OtpBkDataPath);
        }

        if ((NULL != otp_buf) && (otp_size > 0) ) {
        if (ret && (otp_size == fwrite(otp_buf, sizeof(char), otp_size,
                                       fileProductinfoHandle))) {
            SENSOR_LOGI("%s:backup image write success", OtpBkDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:backup image write failed!", OtpBkDataPath);
            ret = 0;
            }
        }

        if (ret) {

            fflush(fileProductinfoHandle);
            fd = fileno(fileProductinfoHandle);

            if (fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                SENSOR_LOGI("fileno() = %s", OtpBkDataPath);
                ret = 0;
            }
        }

        fclose(fileProductinfoHandle);
        SENSOR_LOGI("%s:image write finished", OtpBkDataPath);
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
        SENSOR_LOGD("remove file:%s, ret:%d", OtpDataPath, ret);
        fileVendorHandle = fopen(OtpDataPath, "w+");
    }

    if (fileVendorHandle != NULL) {
        SENSOR_LOGI("open %s success", OtpDataPath);
        if (CALI_OTP_HEAD_SIZE != fwrite(header_buf, sizeof(char),
                                         CALI_OTP_HEAD_SIZE,
                                         fileVendorHandle)) {
            ret = 0;
            SENSOR_LOGE("%s:origin image header write fail!", OtpDataPath);
        } else {
            ret = 1;
            SENSOR_LOGI("%s:origin image header write success", OtpDataPath);
        }

        if ((NULL != otp_buf) && (otp_size > 0) ) {
        if (ret && (otp_size ==
                    fwrite(otp_buf, sizeof(char), otp_size, fileVendorHandle))) {
            SENSOR_LOGI("%s:origin image write success", OtpDataPath);
            ret = 1;
        } else {
            SENSOR_LOGE("%s:origin image write fail!", OtpDataPath);
            ret = 0;
            }
        }

        if (ret) {
            SENSOR_LOGI("fflush = %s", OtpDataPath);
            fflush(fileVendorHandle);
            fd = fileno(fileVendorHandle);

            if (fd > 0) {
                fsync(fd);
                ret = 1;
            } else {
                SENSOR_LOGI("fileno() = %s", OtpDataPath);
                ret = 0;
            }
        }

        fclose(fileVendorHandle);
        SENSOR_LOGI("%s:image write finished", OtpDataPath);
    } else {
        SENSOR_LOGE("open:%s failed!", OtpDataPath);
        ret = 0;
    }

    return ret;
}
