/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "otp_parser_common"
#include "otp_parser.h"

struct otp_parser_camera_info camera_otp_info[4];

cmr_int otp_parser_map_v04(void *raw_data, enum otp_parser_cmd cmd,
                           cmr_uint eeprom_num, cmr_int camera_id,
                           cmr_int raw_height, cmr_int raw_width, void *result);
cmr_int otp_parser_map_v05(void *raw_data, enum otp_parser_cmd cmd,
                           cmr_uint eeprom_num, cmr_int camera_id,
                           cmr_int raw_height, cmr_int raw_width, void *result);
cmr_int otp_parser_map_v10(void *raw_data, enum otp_parser_cmd cmd,
                           cmr_int camera_id, void *result);
cmr_int otp_parser_map_v11(void *raw_data, enum otp_parser_cmd cmd,
                           cmr_int camera_id, void *result);

cmr_int _otp_get_map_version(void *raw_data, cmr_u16 *map_version) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    cmr_u16 byte4_byte5 = 0;
    cmr_u16 version = 0;
    cmr_u8 *raw_otp = (cmr_u8 *)raw_data;

    if (NULL == raw_data) {
        rtn = OTP_PARSER_PARAM_ERR;
        CMR_LOGE("raw_data is null");
        return rtn;
    }

    byte4_byte5 = (raw_otp[4] << 8) | raw_otp[5];

    switch (byte4_byte5) {
    case 0x0001:
        version = 0x0001;
        break;
    case 0x0002:
        version = 0x0002;
        break;
    case 0x0003:
        version = 0x0003;
        break;
    case 0x0004:
        version = 0x0004;
        break;
    case 0x0005:
        version = 0x0005;
        break;
    case 0x0100:
        if (raw_otp[0] == 0x53 && raw_otp[1] == 0x50 && raw_otp[2] == 0x52 &&
            raw_otp[3] == 0x44) {
            version = 0x0100;
        } else {
            version = 0x0001;
        }
        break;
    case 0x0101:
        version = 0x0101;
        break;
    case 0x0200:
        version = 0x0002;
        break;
    case 0x0300:
        version = 0x0003;
        break;
    case 0x0400:
        version = 0x0004;
        break;
    case 0x0500:
        version = 0x0005;
        break;
    default:
        version = 0;
        rtn = OTP_PARSER_VERSION_ERR;
        break;
    }

    *map_version = version;
    CMR_LOGD("otp map version = 0x%04x", version);

    return rtn;
}

/***********************************************************
** Only otp map version after 0.3 will be parsered in "otp_parser()"  API
** Otp map 0.3 and otp map 0.4 is same
** Otp map 0.4 and otp map 0.5 differs in the size of dual-camera data, with 230
*(0.4) and 256 (0.5)
** Otp map 1.0 and otp map 1.1 can get each section start address and size in
*module info
************************************************************/
cmr_int otp_parser(void *raw_data, enum otp_parser_cmd cmd, cmr_uint eeprom_num,
                   cmr_int camera_id, cmr_int raw_height, cmr_int raw_width,
                   void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    cmr_u16 map_version = 0x0000;

    CMR_LOGD("E");

    if (NULL == raw_data || NULL == result) {
        rtn = OTP_PARSER_PARAM_ERR;
        CMR_LOGE("input param is null");
        goto exit;
    }

    rtn = _otp_get_map_version(raw_data, &map_version);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("otp map version is wrong");
        goto exit;
    }

    if (OTP_PARSER_MAP_VERSION == cmd) {
        struct otp_parser_map_version *map_ver =
            (struct otp_parser_map_version *)result;
        map_ver->version = map_version;
        CMR_LOGV("OTP_PARSER_MAP_VERSION is 0x%04x", map_ver->version);
        goto exit;
    }

    switch (map_version) {
    case 0x0003:
    case 0x0004:
        rtn = otp_parser_map_v04(raw_data, cmd, eeprom_num, camera_id,
                                 raw_height, raw_width, result);
        break;
    case 0x0005:
        rtn = otp_parser_map_v05(raw_data, cmd, eeprom_num, camera_id,
                                 raw_height, raw_width, result);
        break;
    case 0x0100:
        rtn = otp_parser_map_v10(raw_data, cmd, camera_id, result);
        break;
    case 0x0101:
        rtn = otp_parser_map_v11(raw_data, cmd, camera_id, result);
        break;
    default:
        CMR_LOGD("map version is 0x%04x, no parser", map_version);
        break;
    }

exit:
    CMR_LOGD("X rtn = %ld", rtn);
    return rtn;
}

cmr_int otp_parser_v1(void *raw_data, enum otp_parser_cmd cmd,
                      cmr_int camera_id, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    cmr_u16 map_version = 0x0000;

    CMR_LOGV("E");

    if (NULL == raw_data || NULL == result) {
        rtn = OTP_PARSER_PARAM_ERR;
        CMR_LOGE("input param is null");
        goto exit;
    }

    rtn = _otp_get_map_version(raw_data, &map_version);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("otp map version is wrong");
        goto exit;
    }

    if (OTP_PARSER_MAP_VERSION == cmd) {
        struct otp_parser_map_version *map_ver =
            (struct otp_parser_map_version *)result;
        map_ver->version = map_version;
        CMR_LOGV("OTP_PARSER_MAP_VERSION is 0x%04x", map_ver->version);
        goto exit;
    }

    switch (map_version) {
    case 0x0100:
        rtn = otp_parser_map_v10(raw_data, cmd, camera_id, result);
        break;
    case 0x0101:
        rtn = otp_parser_map_v11(raw_data, cmd, camera_id, result);
        break;
    default:
        CMR_LOGD("map version is 0x%04x, no parser", map_version);
        break;
    }

exit:
    CMR_LOGV("X rtn = %ld", rtn);
    return rtn;
}
