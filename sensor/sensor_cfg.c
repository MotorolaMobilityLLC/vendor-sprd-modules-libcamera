/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "sensor_cfg"

#include "sensor_drv_u.h"
#include "sensor_cfg.h"

#if defined(CONFIG_CAMERA_ISP_DIR_3)
extern SENSOR_INFO_T g_ov2680_mipi_raw_info;
extern SENSOR_INFO_T g_s5k3l8xxm3_mipi_raw_info;
extern SENSOR_INFO_T g_sp2509_mipi_raw_info;
extern SENSOR_INFO_T g_ov8856_mipi_raw_info;
extern SENSOR_INFO_T g_ov8856s_mipi_raw_info;
extern SENSOR_INFO_T g_s5k5e2ya_mipi_raw_info;
extern SENSOR_INFO_T g_s5k3p8sm_mipi_raw_info;
extern SENSOR_INFO_T g_s5k4h8yx_mipi_raw_info;
extern SENSOR_INFO_T g_imx230_mipi_raw_info;
#endif
extern SENSOR_INFO_T g_imx258_mipi_raw_info;
extern SENSOR_INFO_T g_ov13855_mipi_raw_info;
extern SENSOR_INFO_T g_c2580_mipi_raw_info;
extern SENSOR_INFO_T g_ov13855a_mipi_raw_info;
extern SENSOR_INFO_T g_s5k3l8xxm3q_mipi_raw_info;
extern SENSOR_INFO_T g_gc2375a_mipi_raw_info;
extern SENSOR_INFO_T g_gc5024_mipi_raw_info;
extern SENSOR_INFO_T g_ov13850r2a_mipi_raw_info;
extern SENSOR_INFO_T g_imx135_mipi_raw_info;
extern SENSOR_INFO_T g_sp2509r_mipi_raw_info;
extern SENSOR_INFO_T g_s5k3l8xxm3r_mipi_raw_info;

#ifdef CONFIG_COVERED_SENSOR
extern SENSOR_INFO_T g_GC0310_MIPI_yuv_info;
//extern SENSOR_INFO_T g_c2580_mipi_raw_info;
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
extern SENSOR_INFO_T g_ov5675_mipi_raw_info;
extern SENSOR_INFO_T g_ov5675_dual_mipi_raw_info;
extern SENSOR_INFO_T g_gc8024_mipi_raw_info;
extern SENSOR_INFO_T g_gc5005_mipi_raw_info;
extern SENSOR_INFO_T g_gc2375_mipi_raw_info;
extern SENSOR_INFO_T g_c2390_mipi_raw_info;
//extern SENSOR_INFO_T g_c2580_mipi_raw_info;
extern SENSOR_INFO_T g_sp8407_mipi_raw_info;
extern SENSOR_INFO_T g_s5k5e8yx_mipi_raw_info;
#endif

#define AUTO_TEST_CAMERA 1
extern otp_drv_entry_t imx258_drv_entry;
extern otp_drv_entry_t ov13855_drv_entry;
extern otp_drv_entry_t ov13855_sunny_drv_entry;
extern otp_drv_entry_t ov5675_sunny_drv_entry;
extern otp_drv_entry_t imx258_truly_drv_entry;
extern otp_drv_entry_t ov13855_altek_drv_entry;
extern otp_drv_entry_t s5k3l8xxm3_qtech_drv_entry;
extern otp_drv_entry_t s5k3p8sm_truly_drv_entry;

extern struct sns_af_drv_entry dw9800_drv_entry;
extern struct sns_af_drv_entry dw9714_drv_entry;
extern struct sns_af_drv_entry dw9714a_drv_entry;
extern struct sns_af_drv_entry dw9718s_drv_entry;
extern struct sns_af_drv_entry bu64297gwz_drv_entry;
extern struct sns_af_drv_entry vcm_ak7371_drv_entry;
extern struct sns_af_drv_entry lc898214_drv_entry;
extern struct sns_af_drv_entry dw9763_drv_entry;
extern struct sns_af_drv_entry vcm_zc524_drv_entry;
extern struct sns_af_drv_entry ad5823_drv_entry;
extern struct sns_af_drv_entry vm242_drv_entry;



/**
 * NOTE: the interface can only be used by sensor ic.
 *       not for otp and vcm device.
 **/
const cmr_u8 camera_module_name_str[MODULE_MAX][20] = {
    [0] = "default",
    [MODULE_SUNNY] = "Sunny",
    [MODULE_TRULY] = "Truly",
    [MODULE_A_KERR] = "A-kerr",
    [MODULE_LITEARRAY] = "LiteArray",
    [MODULE_DARLING] = "Darling", /*5*/
    [MODULE_QTECH] = "Qtech",
    [MODULE_OFLIM] = "Oflim",
    [MODULE_HUAQUAN] = "Huaquan",
    [MODULE_KINGCOM] = "Kingcom",
    [MODULE_BOOYI] = "Booyi", /*10*/
    [MODULE_LAIMU] = "Laimu",
    [MODULE_WDSEN] = "Wdsen",
    [MODULE_SUNRISE] = "Sunrise",
    [MODULE_CAMERAKING] = "CameraKing",
    [MODULE_SUNNINESS] = "Sunniness", /*15*/
    [MODULE_RIYONG] = "Riyong",
    [MODULE_TONGJU] = "Tongju",
    /*add you camera module name following*/
};

/*---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/

//{.sn_name = "imx258_mipi_raw", .sensor_info =  &g_imx258_mipi_raw_info,
// .af_dev_info = {.af_drv_entry = &dw9800_drv_entry, .af_work_mode = 0}, &imx258_drv_entry},

const SENSOR_MATCH_T main_sensor_infor_tab[] = {
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    {MODULE_SUNNY, "gc8024_mipi_raw", &g_gc8024_mipi_raw_info, {&dw9714_drv_entry, 0}, NULL},
    {MODULE_TRULY, "imx258_mipi_raw", &g_imx258_mipi_raw_info, {&dw9800_drv_entry, 0}, &imx258_drv_entry},
    {MODULE_SUNNY, "gc5005_mipi_raw", &g_gc5005_mipi_raw_info, {NULL, 0}, NULL},
#if defined(CONFIG_DUAL_MODULE)
    {MODULE_SUNNY, "ov13855_mipi_raw", &g_ov13855_mipi_raw_info, {&vcm_zc524_drv_entry, 0}, &ov13855_sunny_drv_entry},
    {MODULE_SUNNY, "ov13855a_mipi_raw", &g_ov13855a_mipi_raw_info, {&bu64297gwz_drv_entry, 0}, &ov13855_altek_drv_entry},
    {MODULE_SUNNY, "imx135_mipi_raw", &g_imx135_mipi_raw_info, {&ad5823_drv_entry, 0}, NULL},
    {MODULE_SUNNY, "s5k3l8xxm3r_mipi_raw", &g_s5k3l8xxm3r_mipi_raw_info, {&vm242_drv_entry, 0}, NULL},
#else
    {MODULE_SUNNY, "ov13855_mipi_raw", &g_ov13855_mipi_raw_info, {&dw9718s_drv_entry, 0}, &ov13855_drv_entry},
#endif
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_3)
#ifdef CAMERA_SENSOR_BACK_I2C_SWITCH
    {MODULE_DARLING, "imx258_mipi_raw", &g_imx258_mipi_raw_info, {&dw9763_drv_entry, 0}, NULL},
#else
    {MODULE_SUNNY ,"imx258_mipi_raw", &g_imx258_mipi_raw_info, {&lc898214_drv_entry, 0}, &imx258_truly_drv_entry},
#endif
    {MODULE_SUNNY ,"s5k3l8xxm3_mipi_raw", &g_s5k3l8xxm3_mipi_raw_info, {&vcm_ak7371_drv_entry, 0}, NULL},
    {MODULE_SUNNY ,"imx230_mipi_raw", &g_imx230_mipi_raw_info, {&dw9800_drv_entry, 0}, NULL},
    {MODULE_SUNNY ,"s5k3p8sm_mipi_raw", &g_s5k3p8sm_mipi_raw_info, {&bu64297gwz_drv_entry, 0}, &s5k3p8sm_truly_drv_entry},
#endif
    {0}};

const SENSOR_MATCH_T sub_sensor_infor_tab[] = {
#ifdef CONFIG_COVERED_SENSOR
    //{MODULE_SUNNY ,"GC0310_MIPI", &g_GC0310_MIPI_yuv_info, {NULL, 0}, NULL},
//{"g_c2580_mipi_raw", &g_c2580_mipi_raw_info, {NULL,0}, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    {MODULE_SUNNY, "gc5005_mipi_raw", &g_gc5005_mipi_raw_info, {NULL, 0}, NULL},
    {MODULE_DARLING, "ov5675_mipi_raw", &g_ov5675_mipi_raw_info, {NULL, 0}, NULL},
    {MODULE_SUNNY, "sp8407_mipi_raw", &g_sp8407_mipi_raw_info, {NULL, 0}, NULL},
    //{MODULE_SUNNY, "gc2375_mipi_raw", &g_gc2375_mipi_raw_info, {NULL, 0}, NULL},
    {MODULE_QTECH, "s5k3l8xxm3q_mipi_raw", &g_s5k3l8xxm3q_mipi_raw_info, {NULL, 0}, &s5k3l8xxm3_qtech_drv_entry},
    {MODULE_SUNNY, "gc5024_mipi_raw", &g_gc5024_mipi_raw_info, {NULL, 0}, NULL},
    {MODULE_SUNNY, "ov13850r2a_mipi_raw", &g_ov13850r2a_mipi_raw_info, {&dw9714a_drv_entry, 0}, NULL},

#endif
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {MODULE_SUNNY, "s5k4h8yx_mipi_raw", &g_s5k4h8yx_mipi_raw_info, {NULL, 0}, NULL},
    {MODULE_SUNNY, "ov8856_mipi_raw", &g_ov8856_mipi_raw_info, {NULL, 0}, NULL},
    {MODULE_SUNNY, "g_s5k5e2ya_mipi_raw_info", &g_s5k5e2ya_mipi_raw_info, {&dw9714_drv_entry, 0}, NULL},
#endif
    {0}};

const SENSOR_MATCH_T sensor2_infor_tab[] = {
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {MODULE_SUNNY, "ov2680_mipi_raw", &g_ov2680_mipi_raw_info, {NULL, 0}, NULL},
    //{MODULE_SUNNY, "sp2509_mipi_raw", &g_sp2509_mipi_raw_info, {NULL, 0}, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    {MODULE_SUNNY, "g_c2580_mipi_raw", &g_c2580_mipi_raw_info, {NULL, 0}, NULL},
#ifdef CONFIG_COVERED_SENSOR
    {MODULE_SUNNY, "g_c2580_mipi_raw", &g_c2580_mipi_raw_info, {NULL, 0}, NULL},
#endif

#if defined(CONFIG_DUAL_MODULE)
    {MODULE_SUNNY, "ov5675_dual_mipi_raw", &g_ov5675_dual_mipi_raw_info, {NULL, 0}, &ov5675_sunny_drv_entry},
    {MODULE_SUNNY, "gc2375a_mipi_raw", &g_gc2375a_mipi_raw_info, {NULL, 0}, NULL},
	{MODULE_SUNNY, "sp2509r_mipi_raw", &g_sp2509r_mipi_raw_info, {NULL, 0}, NULL},
#endif

#endif
    {0}};

const SENSOR_MATCH_T sensor3_infor_tab[] = {
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    //{MODULE_SUNNY, "ov8856s_mipi_raw", &g_ov8856s_mipi_raw_info, {NULL, 0}, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
#if defined(CONFIG_COVERED_SENSOR)
    {MODULE_SUNNY,"g_c2580_mipi_raw", &g_c2580_mipi_raw_info, {NULL, 0}, NULL},
#endif
#endif
#ifdef SC_FPGA
    //"ov5640_mipi_raw", &g_ov5640_mipi_raw_info},
#endif
    {0}};

const SENSOR_MATCH_T atv_infor_tab[] = {
    //&g_nmi600_yuv_info, //&g_tlg1120_yuv_info,
    {0}};


const SENSOR_MATCH_T at_atv_infor_tab[] = {
    //{"nmi600_yuv", &g_nmi600_yuv_info},
    //{"tlg1120_yuv", &g_tlg1120_yuv_info},  bonnie
    {0}};

/*
* add for auto test for main and sub camera (raw yuv)
* 2014-02-07 freed wang end
*/
//SENSOR_MATCH_T *Sensor_GetInforTab(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id) {
SENSOR_MATCH_T *sensor_get_module_tab(cmr_int at_flag, cmr_u32 sensor_id) {
    SENSOR_MATCH_T *sensor_infor_tab_ptr = NULL;
    cmr_u32 index = 0;
    SENSOR_LOGD("at_flag: %d", at_flag);

    if (AUTO_TEST_CAMERA == at_flag) {
        switch (sensor_id) {
        case SENSOR_MAIN:
            sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&main_sensor_infor_tab;
            break;
        case SENSOR_DEVICE2:
            sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&sensor2_infor_tab;
            break;
        case SENSOR_SUB:
            sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&sub_sensor_infor_tab;
            break;
        case SENSOR_DEVICE3:
            sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&sensor3_infor_tab;
            break;
        case SENSOR_ATV:
            sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&at_atv_infor_tab;
            break;
        default:
            break;
        }
        return (SENSOR_MATCH_T *)sensor_infor_tab_ptr;
    }
    switch (sensor_id) {
    case SENSOR_MAIN:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&main_sensor_infor_tab;
        break;
    case SENSOR_SUB:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&sub_sensor_infor_tab;
        break;
    case SENSOR_DEVICE2:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&sensor2_infor_tab;
        break;
    case SENSOR_DEVICE3:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&sensor3_infor_tab;
        break;

    case SENSOR_ATV:
        sensor_infor_tab_ptr = (SENSOR_MATCH_T *)&atv_infor_tab;
        break;

    default:
        break;
    }

    return (SENSOR_MATCH_T *)sensor_infor_tab_ptr;
}

//Sensor_GetInforTabLenght
cmr_u32 sensor_get_tab_length(cmr_int at_flag, cmr_u32 sensor_id) {
    cmr_u32 tab_lenght = 0;
    if (AUTO_TEST_CAMERA == at_flag) {
        switch (sensor_id) {
        case SENSOR_MAIN:
            tab_lenght = ARRAY_SIZE(main_sensor_infor_tab);
            break;
        case SENSOR_DEVICE2:
            tab_lenght = ARRAY_SIZE(sensor2_infor_tab);
            break;
        case SENSOR_SUB:
            tab_lenght = ARRAY_SIZE(sub_sensor_infor_tab);
            break;
        case SENSOR_DEVICE3:
            tab_lenght = ARRAY_SIZE(sensor3_infor_tab);
            break;
        case SENSOR_ATV:
            tab_lenght = ARRAY_SIZE(at_atv_infor_tab);
            break;
        default:
            break;
        }
    } else {
        switch (sensor_id) {
        case SENSOR_MAIN:
            tab_lenght = ARRAY_SIZE(main_sensor_infor_tab);
            break;
        case SENSOR_SUB:
            tab_lenght = ARRAY_SIZE(sub_sensor_infor_tab);
            break;
        case SENSOR_DEVICE2:
            tab_lenght = ARRAY_SIZE(sensor2_infor_tab);
            break;
        case SENSOR_DEVICE3:
            tab_lenght = ARRAY_SIZE(sensor3_infor_tab);
            break;
        case SENSOR_ATV:
            tab_lenght = ARRAY_SIZE(atv_infor_tab);
            break;
        default:
            break;
        }
    }
    SENSOR_LOGI("module table length:%d", tab_lenght);
    return tab_lenght;
}

cmr_u32 sensor_get_match_index(cmr_int at_flag, cmr_u32 sensor_id) {
    cmr_u32 i = 0;
    cmr_u32 retValue = 0xFF;
    cmr_u32 mSnNum = 0;
    cmr_u32 sSnNum = 0;

    if (AUTO_TEST_CAMERA == at_flag) {
        if (sensor_id == SENSOR_MAIN ) {
            mSnNum = ARRAY_SIZE(main_sensor_infor_tab) - 1;
            SENSOR_LOGI("sensor autotest sensorTypeMatch main is %d", mSnNum);
            for (i = 0; i < mSnNum; i++) {
                if (strcmp(main_sensor_infor_tab[i].sn_name,
                           AT_CAMERA_SENSOR_TYPE_BACK) == 0) {
                    SENSOR_LOGI("sensor autotest sensor matched  %dth  is %s",
                                i, AT_CAMERA_SENSOR_TYPE_BACK);
                    retValue = i;
                    break;
                }
            }
        }
        if (sensor_id == SENSOR_SUB ) {
            sSnNum = ARRAY_SIZE(sub_sensor_infor_tab) - 1;
            SENSOR_LOGI("sensor autotest sensorTypeMatch sub is %d", sSnNum);
            for (i = 0; i < sSnNum; i++) {
                if (strcmp(sub_sensor_infor_tab[i].sn_name,
                           AT_CAMERA_SENSOR_TYPE_FRONT) == 0) {
                    SENSOR_LOGI("sensor autotest matched the %dth  is %s", i,
                                AT_CAMERA_SENSOR_TYPE_FRONT);
                    retValue = i;
                    break;
                }
            }
        }
        if (sensor_id == SENSOR_DEVICE2) {
            mSnNum = sizeof(sensor2_infor_tab) / sizeof(SENSOR_MATCH_T) - 1;
            SENSOR_LOGI("sensor sensorTypeMatch main2 is %d", mSnNum);
            for (i = 0; i < mSnNum; i++) {
                if (strcmp(sensor2_infor_tab[i].sn_name,
                           CAMERA_SENSOR_TYPE_BACK_EXT) == 0) {
                    SENSOR_LOGI("sensor sensor matched  %dth  is %s", i,
                                CAMERA_SENSOR_TYPE_BACK_EXT);
                    retValue = i;
                    break;
                }
            }
        }
        if (sensor_id == SENSOR_DEVICE3) {
            sSnNum = sizeof(sensor3_infor_tab) / sizeof(SENSOR_MATCH_T) - 1;
            SENSOR_LOGI("sensor sensorTypeMatch sub2 is %d", sSnNum);
            for (i = 0; i < sSnNum; i++) {
                if (strcmp(sensor3_infor_tab[i].sn_name,
                           CAMERA_SENSOR_TYPE_FRONT_EXT) == 0) {
                    SENSOR_LOGI("sensor sensor matched the %dth  is %s", i,
                                CAMERA_SENSOR_TYPE_FRONT_EXT);
                    retValue = i;
                    break;
                }
            }
        }
    } else {
        if (sensor_id == SENSOR_MAIN) {
            mSnNum = ARRAY_SIZE(main_sensor_infor_tab) - 1;
            SENSOR_LOGI("sensor sensorTypeMatch main is %d", mSnNum);
            for (i = 0; i < mSnNum; i++) {
                if (strcmp(main_sensor_infor_tab[i].sn_name,
                           CAMERA_SENSOR_TYPE_BACK) == 0) {
                    SENSOR_LOGI("sensor sensor matched  %dth  is %s", i,
                                CAMERA_SENSOR_TYPE_BACK);
                    retValue = i;
                    break;
                }
            }
        }
        if (sensor_id == SENSOR_SUB) {
            sSnNum = ARRAY_SIZE(sub_sensor_infor_tab) - 1;
            SENSOR_LOGI("sensor sensorTypeMatch sub is %d", sSnNum);
            for (i = 0; i < sSnNum; i++) {
                if (strcmp(sub_sensor_infor_tab[i].sn_name,
                           CAMERA_SENSOR_TYPE_FRONT) == 0) {
                    SENSOR_LOGI("sensor sensor matched the %dth  is %s", i,
                                CAMERA_SENSOR_TYPE_FRONT);
                    retValue = i;
                    break;
                }
            }
        }
        if (sensor_id == SENSOR_DEVICE2) {
            mSnNum = ARRAY_SIZE(sensor2_infor_tab) - 1;
            SENSOR_LOGI("sensor sensorTypeMatch main2 is %d", mSnNum);
            for (i = 0; i < mSnNum; i++) {
                if (strcmp(sensor2_infor_tab[i].sn_name,
                           CAMERA_SENSOR_TYPE_BACK_EXT) == 0) {
                    SENSOR_LOGI("sensor sensor matched  %dth  is %s", i,
                                CAMERA_SENSOR_TYPE_BACK_EXT);
                    retValue = i;
                    break;
                }
            }
        }
        if (sensor_id == SENSOR_DEVICE3) {
            sSnNum = ARRAY_SIZE(sensor3_infor_tab) - 1;
            SENSOR_LOGI("sensor sensorTypeMatch sub2 is %d", sSnNum);
            for (i = 0; i < sSnNum; i++) {
                if (strcmp(sensor3_infor_tab[i].sn_name,
                           CAMERA_SENSOR_TYPE_FRONT_EXT) == 0) {
                    SENSOR_LOGI("sensor sensor matched the %dth  is %s", i,
                                CAMERA_SENSOR_TYPE_FRONT_EXT);
                    retValue = i;
                    break;
                }
            }
        }
    }
    return retValue;
}
