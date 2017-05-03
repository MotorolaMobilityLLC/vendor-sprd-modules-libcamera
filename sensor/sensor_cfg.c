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
extern SENSOR_INFO_T g_autotest_ov13850_mipi_raw_info;
extern SENSOR_INFO_T g_at_ov5648_mipi_raw_info;
extern SENSOR_INFO_T g_at_ov5670_mipi_raw_info;
extern SENSOR_INFO_T g_imx258_mipi_raw_info;
extern SENSOR_INFO_T g_ov13855_mipi_raw_info;
#ifdef CONFIG_COVERED_SENSOR
extern SENSOR_INFO_T g_GC0310_MIPI_yuv_info;
extern SENSOR_INFO_T g_c2580_mipi_raw_info;
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
extern SENSOR_INFO_T g_ov5675_mipi_raw_info;
extern SENSOR_INFO_T g_ov5675_dual_mipi_raw_info;
extern SENSOR_INFO_T g_ov13850_mipi_raw_info;
extern SENSOR_INFO_T g_gc8024_mipi_raw_info;
extern SENSOR_INFO_T g_gc5005_mipi_raw_info;
extern SENSOR_INFO_T g_gc2375_mipi_raw_info;
extern SENSOR_INFO_T g_c2390_mipi_raw_info;
extern SENSOR_INFO_T g_sp8407_mipi_raw_info;
extern SENSOR_INFO_T g_s5k5e8yx_mipi_raw_info;
#endif
#define AUTO_TEST_CAMERA 1
extern otp_drv_entry_t imx258_drv_entry;
extern otp_drv_entry_t ov13855_drv_entry;
extern otp_drv_entry_t ov13855_sunny_drv_entry;
extern otp_drv_entry_t ov5675_sunny_drv_entry;
extern otp_drv_entry_t imx258_truly_drv_entry;

extern struct sns_af_drv_entry dw9800_drv_entry;
extern struct sns_af_drv_entry dw9714_drv_entry;
extern struct sns_af_drv_entry dw9718s_drv_entry;
extern struct sns_af_drv_entry bu64297gwz_drv_entry;
extern struct sns_af_drv_entry vcm_ak7371_drv_entry;
extern struct sns_af_drv_entry lc898214_drv_entry;
extern struct sns_af_drv_entry dw9763_drv_entry;
extern struct sns_af_drv_entry vcm_zc524_drv_entry;

/*---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/
const SENSOR_MATCH_T main_sensor_infor_tab[] = {
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    {"gc8024_mipi_raw", &g_gc8024_mipi_raw_info, {&dw9714_drv_entry, 0}, NULL},
    {"imx258_mipi_raw",
     &g_imx258_mipi_raw_info,
     {&dw9800_drv_entry, 0},
     &imx258_drv_entry},
#if defined(CONFIG_DUAL_MODULE)
    {"ov13855_mipi_raw",
     &g_ov13855_mipi_raw_info,
     {&vcm_zc524_drv_entry, 0},
     &ov13855_sunny_drv_entry}, /* for bringup */
#else
    {"ov13855_mipi_raw",
     &g_ov13855_mipi_raw_info,
     {&dw9718s_drv_entry, 0},
     &ov13855_drv_entry},
#endif
     {"g_c2390_mipi_raw_info", &g_c2390_mipi_raw_info,{NULL,0},NULL},
       {"gc5005_mipi_raw", &g_gc5005_mipi_raw_info, {&dw9714_drv_entry, 0}, NULL},
   //   {"g_sp8407_mipi_raw_info", &g_sp8407_mipi_raw_info,{&dw9714_drv_entry,0},NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_3)
#ifdef CAMERA_SENSOR_BACK_I2C_SWITCH
    {"imx258_mipi_raw", &g_imx258_mipi_raw_info, {&dw9763_drv_entry, 0}, NULL},
#else
    {"imx258_mipi_raw",
     &g_imx258_mipi_raw_info,
     {&lc898214_drv_entry, 0},
     &imx258_truly_drv_entry},
#endif
    //{"ov13850r2a_mipi_raw", &g_ov13850r2a_mipi_raw_info, {NULL,0}, NULL},
    {"s5k3l8xxm3_mipi_raw",
     &g_s5k3l8xxm3_mipi_raw_info,
     {&vcm_ak7371_drv_entry, 0},
     NULL},
    {"imx230_mipi_raw", &g_imx230_mipi_raw_info, {&dw9800_drv_entry, 0}, NULL},
    {"s5k3p8sm_mipi_raw",
     &g_s5k3p8sm_mipi_raw_info,
     {&bu64297gwz_drv_entry, 0},
     NULL},
#endif
    {0}};

const SENSOR_MATCH_T sub_sensor_infor_tab[] = {
#ifdef CONFIG_COVERED_SENSOR
    {"GC0310_MIPI", &g_GC0310_MIPI_yuv_info, {NULL, 0}, NULL},
//{"g_c2580_mipi_raw", &g_c2580_mipi_raw_info, {NULL,0}, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
//    {"gc5005_mipi_raw", &g_gc5005_mipi_raw_info, {NULL, 0}, NULL},
    {"ov5675_mipi_raw", &g_ov5675_mipi_raw_info, {NULL, 0}, NULL},
    {"gc2375_mipi_raw", &g_gc2375_mipi_raw_info, {NULL, 0}, NULL},
    {"g_sp8407_mipi_raw_info", &g_sp8407_mipi_raw_info,{&dw9763_drv_entry,0},NULL},
    {"s5k5e8yx_mipi_raw", &g_s5k5e8yx_mipi_raw_info, {NULL, 0}, 0},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {"s5k4h8yx_mipi_raw", &g_s5k4h8yx_mipi_raw_info, {NULL, 0}, NULL},
    {"ov8856_mipi_raw", &g_ov8856_mipi_raw_info, {NULL, 0}, NULL},
    {"g_s5k5e2ya_mipi_raw_info", &g_s5k5e2ya_mipi_raw_info, {NULL, 0}, NULL},
#endif
    {0}};

const SENSOR_MATCH_T sensor2_infor_tab[] = {
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {"ov2680_mipi_raw", &g_ov2680_mipi_raw_info, {NULL, 0}, NULL},
    {"sp2509_mipi_raw", &g_sp2509_mipi_raw_info, {NULL, 0}, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
#ifdef CONFIG_COVERED_SENSOR
   // {"g_c2580_mipi_raw", &g_c2580_mipi_raw_info, {NULL, 0}, NULL},
#endif
#if defined(CONFIG_DUAL_MODULE)
    {"ov5675_dual_mipi_raw",
     &g_ov5675_dual_mipi_raw_info,
     {NULL, 0},
     &ov5675_sunny_drv_entry},
#endif
#endif
    {0}};

const SENSOR_MATCH_T sensor3_infor_tab[] = {
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {"ov8856s_mipi_raw", &g_ov8856s_mipi_raw_info, {NULL, 0}, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
#if defined(CONFIG_COVERED_SENSOR)
    {"g_c2580_mipi_raw", &g_c2580_mipi_raw_info, {NULL, 0}, NULL},
#endif
#endif
#ifdef SC_FPGA
//	{"ov5640_mipi_raw", &g_ov5640_mipi_raw_info},
#endif
    {0}};

const SENSOR_MATCH_T atv_infor_tab[] = {
    //&g_nmi600_yuv_info, //&g_tlg1120_yuv_info,
    {0}};

/*
*add for auto test for main and sub camera (raw yuv)
* 2014-02-07 freed wang  begin
*/
#if 0
const SENSOR_MATCH_T at_main_sensor_infor_tab[] = {
#ifndef SC_FPGA
    {"autotest_ov13850_mipi_raw",
     &g_autotest_ov13850_mipi_raw_info,
     {NULL, 0},
     NULL},
    {"autotest_ov5670_mipi_raw", &g_at_ov5670_mipi_raw_info, {NULL, 0}, NULL},
#if 0 //!(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
#ifdef CONFIG_BACK_CAMERA_MIPI
    {"autotest_ov8825_mipi_raw", &g_autotest_ov8825_mipi_raw_info, {NULL,0}, NULL},
    {"autotest_ov5640_mipi_yuv", &g_autotest_ov5640_mipi_yuv_info, {NULL,0}, NULL},
    {"ov5648_mipi_raw", &g_ov5648_mipi_raw_info, {NULL,0}, NULL},
#endif
#ifdef CONFIG_BACK_CAMERA_CCIR
    {"at_ov5640_ccir_yuv", &g_at_ov5640_ccir_yuv_info, {NULL,0}, NULL},
    {"at_hi253_yuv", &g_hi253_yuv_info, {NULL,0}, NULL},
    //{"at_GT2005_yuv", &g_GT2005_yuv_info},
    //{"at_s5k4ec_yuv", &g_s5k4ec_yuv_info},
    {"autotest_yuv", &g_autotest_yuv_info, {NULL,0}, NULL},
#endif
#else
#ifdef CONFIG_BACK_CAMERA_MIPI
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {"at_s5k3m2xxm3_mipi_raw", &g_s5k3m2xxm3_mipi_raw_info, {NULL, 0}, NULL},
    {"at_imx230_mipi_raw", &g_imx230_mipi_raw_info, {NULL, 0}, NULL},
    {"at_imx258_mipi_raw", &g_imx258_mipi_raw_info, {NULL, 0}, NULL},
    {"at_s5k3l8xx3_mipi_raw", &{NULL, 0}, NULL, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    {"at_imx258_mipi_raw", &g_imx258_mipi_raw_info, {NULL, 0}, NULL},
#endif
#endif
#endif
#endif

    {0}

};
const SENSOR_MATCH_T at_sub_sensor_infor_tab[] = {
#ifndef SC_FPGA
    {"autotest_ov5648_mipi_raw", &g_at_ov5648_mipi_raw_info, {NULL, 0}, NULL},
//	{"autotest_GC0310_MIPI_yuv", &g_GC0310_MIPI_yuv_info,NULL,NULL},
#if 0 //!(defined(CONFIG_CAMERA_ISP_VERSION_V3) ||                                 \
      defined(CONFIG_CAMERA_ISP_VERSION_V4))
#ifdef CONFIG_FRONT_CAMERA_CCIR
    {"GC0308_yuv", &g_GC0308_yuv_info, {NULL,0}, NULL},
    {"GC2035_yuv", &g_GC2035_yuv_info, {NULL,0}, NULL},
    //	{"HI702_yuv", &g_HI702_yuv_info},
    {"OV7675_yuv", &g_OV7675_yuv_info, {NULL,0}, NULL},
//	{"autotest_yuv", &g_autotest_yuv_info},
#endif
#ifdef CONFIG_FRONT_CAMERA_MIPI
    {"GC2155_MIPI_yuv", &g_GC2155_MIPI_yuv_info, {NULL,0}, NULL},
#endif
#else
#ifdef CONFIG_FRONT_CAMERA_MIPI
//  {"ov5648_mipi_raw", &g_ov5648_mipi_raw_info, NULL, NULL},
//    {"ov5648_darling_mipi_raw", &g_ov5648_darling_mipi_raw_info, NULL,
//    NULL},
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {"ov8856_mipi_raw", &g_ov8856_mipi_raw_info, {NULL, 0}, NULL},
    {"at_s5k4h8yx_mipi_raw", &g_s5k4h8yx_mipi_raw_info, {NULL, 0}, NULL},
#endif
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    {"ov5675_mipi_raw", &g_ov5675_mipi_raw_info, {NULL, 0}, NULL},
#endif
#endif
#endif
#endif

    {0}

};
const SENSOR_MATCH_T at_dev2_sensor_infor_tab[] = {

#if defined(CONFIG_CAMERA_ISP_DIR_3)
    {"at_ov2680_mipi_raw", &g_ov2680_mipi_raw_info, {NULL, 0}, NULL},
    {"at_sp2509_mipi_raw", &g_sp2509_mipi_raw_info, {NULL, 0}, NULL},
#endif
    {0}};
#endif
const SENSOR_MATCH_T at_atv_infor_tab[] = {
    //{"nmi600_yuv", &g_nmi600_yuv_info},
    //{"tlg1120_yuv", &g_tlg1120_yuv_info},  bonnie
    {0}};

/*
* add for auto test for main and sub camera (raw yuv)
* 2014-02-07 freed wang end
*/

SENSOR_MATCH_T *Sensor_GetInforTab(struct sensor_drv_context *sensor_cxt,
                                   SENSOR_ID_E sensor_id) {
    SENSOR_MATCH_T *sensor_infor_tab_ptr = NULL;
    cmr_u32 index = 0;
    cmr_int at_flag = sensor_cxt->is_autotest;
    SENSOR_LOGD("at %d", at_flag);

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

uint32_t Sensor_GetInforTabLenght(struct sensor_drv_context *sensor_cxt,
                                  SENSOR_ID_E sensor_id) {
    uint32_t tab_lenght = 0;
    cmr_int at_flag = sensor_cxt->is_autotest;
    if (AUTO_TEST_CAMERA == at_flag) {
        switch (sensor_id) {
        case SENSOR_MAIN:
            tab_lenght =
                (sizeof(main_sensor_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;
        case SENSOR_DEVICE2:
            tab_lenght = (sizeof(sensor2_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;

        case SENSOR_SUB:
            tab_lenght =
                (sizeof(sub_sensor_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;
        case SENSOR_DEVICE3:
            tab_lenght = (sizeof(sensor3_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;

        case SENSOR_ATV:
            tab_lenght = (sizeof(at_atv_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;

        default:
            break;
        }
    } else {
        switch (sensor_id) {
        case SENSOR_MAIN:
            tab_lenght =
                (sizeof(main_sensor_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;
        case SENSOR_SUB:
            tab_lenght =
                (sizeof(sub_sensor_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;
        case SENSOR_DEVICE2:
            tab_lenght =
                (sizeof(sensor2_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;
        case SENSOR_DEVICE3:
            tab_lenght = (sizeof(sensor3_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;

        case SENSOR_ATV:
            tab_lenght = (sizeof(atv_infor_tab) / sizeof(SENSOR_MATCH_T));
            break;

        default:
            break;
        }
    }
    return tab_lenght;
}

cmr_u32 Sensor_IndexGet(struct sensor_drv_context *sensor_cxt, cmr_u32 index) {
    cmr_u32 i = 0;
    cmr_u32 retValue = 0xFF;
    cmr_u32 mSnNum = 0;
    cmr_u32 sSnNum = 0;
    cmr_int at_flag = sensor_cxt->is_autotest;
    if (AUTO_TEST_CAMERA == at_flag) {
        if (index == SENSOR_MAIN) {
            mSnNum = sizeof(main_sensor_infor_tab) / sizeof(SENSOR_MATCH_T) - 1;
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
        if (index == SENSOR_SUB) {
            sSnNum = sizeof(sub_sensor_infor_tab) / sizeof(SENSOR_MATCH_T) - 1;
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
	if (index == SENSOR_DEVICE2) {
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
        if (index == SENSOR_DEVICE3) {
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
        if (index == SENSOR_MAIN) {
            mSnNum = sizeof(main_sensor_infor_tab) / sizeof(SENSOR_MATCH_T) - 1;
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
        if (index == SENSOR_SUB) {
            sSnNum = sizeof(sub_sensor_infor_tab) / sizeof(SENSOR_MATCH_T) - 1;
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
        if (index == SENSOR_DEVICE2) {
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
        if (index == SENSOR_DEVICE3) {
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
    }
    return retValue;
}
