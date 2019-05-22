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
#define LOG_TAG "sns_cfg"

#include "sensor_drv_u.h"
#include "sensor_cfg.h"
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// gc area
#ifdef GC0310
extern SENSOR_INFO_T g_GC0310_MIPI_yuv_info;
#endif
#ifdef GC2165
extern SENSOR_INFO_T g_gc2165_mipi_yuv_info;
#endif
#ifdef GC2375
extern SENSOR_INFO_T g_gc2375_mipi_raw_info;
#endif
#ifdef GC2375A
extern SENSOR_INFO_T g_gc2375a_mipi_raw_info;
#endif
#ifdef GC5005
extern SENSOR_INFO_T g_gc5005_mipi_raw_info;
#endif
#ifdef GC5024
extern SENSOR_INFO_T g_gc5024_mipi_raw_info;
#endif
#ifdef GC8024
extern SENSOR_INFO_T g_gc8024_mipi_raw_info;
#endif
#ifdef GC030A
extern SENSOR_INFO_T g_gc030a_mipi_raw_info;
#endif
#ifdef GC030A_F
extern SENSOR_INFO_T g_gc030af_mipi_raw_info;
#endif
#ifdef GC030A_T
extern SENSOR_INFO_T g_gc030at_mipi_raw_info;
#endif
#ifdef GC2385
extern SENSOR_INFO_T g_gc2385_mipi_raw_info;
#endif
#ifdef GC2145
extern SENSOR_INFO_T g_gc2145_mipi_raw_info;
#endif

//hynix area
#ifdef HI556
extern SENSOR_INFO_T g_hi556_mipi_raw_info;
#endif
#ifdef HI846
extern SENSOR_INFO_T g_hi846_mipi_raw_info;
#endif

// ov area
#ifdef OV2680
extern SENSOR_INFO_T g_ov2680_mipi_raw_info;
#endif
#ifdef OV5675
extern SENSOR_INFO_T g_ov5675_mipi_raw_info;
#endif
#ifdef OV5675_DUAL
extern SENSOR_INFO_T g_ov5675_dual_mipi_raw_info;
#endif
#ifdef OV8856_SHINE
extern SENSOR_INFO_T g_ov8856_shine_mipi_raw_info;
#endif
#ifdef OV8856
extern SENSOR_INFO_T g_ov8856_mipi_raw_info;
#endif
#ifdef OV8858
extern SENSOR_INFO_T g_ov8858_mipi_raw_info;
#endif
#ifdef OV13855
extern SENSOR_INFO_T g_ov13855_mipi_raw_info;
#endif
#ifdef OV13855_SUNNY
extern SENSOR_INFO_T g_ov13855_sunny_mipi_raw_info;
#endif
#ifdef OV13855A
extern SENSOR_INFO_T g_ov13855a_mipi_raw_info;
#endif
#ifdef OV13850R2A
extern SENSOR_INFO_T g_ov13850r2a_mipi_raw_info;
#endif
#ifdef OV16885
extern SENSOR_INFO_T g_ov16885_mipi_raw_info;
#endif
#ifdef OV7251
extern SENSOR_INFO_T g_ov7251_mipi_raw_info;
#endif
#ifdef OV7251_DUAL
extern SENSOR_INFO_T g_ov7251_dual_mipi_raw_info;
#endif
#ifdef OV12A10
extern SENSOR_INFO_T g_ov12a10_mipi_raw_info;
#endif

// imx 258
#ifdef IMX135
extern SENSOR_INFO_T g_imx135_mipi_raw_info;
#endif
#ifdef IMX230
extern SENSOR_INFO_T g_imx230_mipi_raw_info;
#endif
#ifdef IMX258
extern SENSOR_INFO_T g_imx258_mipi_raw_info;
#endif
#ifdef IMX351
extern SENSOR_INFO_T g_imx351_mipi_raw_info;
#endif
#ifdef IMX362
extern SENSOR_INFO_T g_imx362_mipi_raw_info;
#endif
#ifdef IMX363
extern SENSOR_INFO_T g_imx363_mipi_raw_info;
#endif
#ifdef IMX386
extern SENSOR_INFO_T g_imx386_mipi_raw_info;
#endif

// cista area
#ifdef C2390
extern SENSOR_INFO_T g_c2390_mipi_raw_info;
#endif
#ifdef C2580
extern SENSOR_INFO_T g_c2580_mipi_raw_info;
#endif

// sp area
#ifdef SP0A09
extern SENSOR_INFO_T g_sp0a09_mipi_raw_info;
#endif
#ifdef SP2509
extern SENSOR_INFO_T g_sp2509_mipi_raw_info;
#endif
#ifdef SP0A09Z
extern SENSOR_INFO_T g_sp0a09z_mipi_raw_info;
#endif
#ifdef SP2509Z
extern SENSOR_INFO_T g_sp2509z_mipi_raw_info;
#endif
#ifdef SP250A
extern SENSOR_INFO_T g_sp250a_mipi_raw_info;
#endif
#ifdef SP2509R
extern SENSOR_INFO_T g_sp2509r_mipi_raw_info;
#endif
#ifdef SP2509V
extern SENSOR_INFO_T g_sp2509v_mipi_raw_info;
#endif
#ifdef SP8407
extern SENSOR_INFO_T g_sp8407_mipi_raw_info;
#endif
#ifdef SP2509ZZ
extern SENSOR_INFO_T g_sp2509zz_mipi_raw_info;
#endif

// samsung area
#ifdef S5K3L8XXM3
extern SENSOR_INFO_T g_s5k3l8xxm3_mipi_raw_info;
#endif
#ifdef S5K3L8XXM3Q
extern SENSOR_INFO_T g_s5k3l8xxm3q_mipi_raw_info;
#endif
#ifdef S5K3L8XXM3R
extern SENSOR_INFO_T g_s5k3l8xxm3r_mipi_raw_info;
#endif
#ifdef S5K3P8SM
extern SENSOR_INFO_T g_s5k3p8sm_mipi_raw_info;
#endif
#ifdef S5K4H8YX
extern SENSOR_INFO_T g_s5k4h8yx_mipi_raw_info;
#endif
#ifdef S5K5E2YA
extern SENSOR_INFO_T g_s5k5e2ya_mipi_raw_info;
#endif
#ifdef S5K5E8YX
extern SENSOR_INFO_T g_s5k5e8yx_mipi_raw_info;
#endif
#ifdef S5K4H9YX
extern SENSOR_INFO_T g_s5k4h9yx_mipi_raw_info;
#endif
#ifdef S5K4H7
extern SENSOR_INFO_T g_s5k4h7_mipi_raw_info;
#endif
#ifdef S5K3L6
extern SENSOR_INFO_T g_s5k3l6_mipi_raw_info;
#endif
#ifdef S5K3P9SX04
extern SENSOR_INFO_T g_s5k3p9sx04_mipi_raw_info;
#endif
#ifdef S5K5E9YU05
extern SENSOR_INFO_T g_s5k5e9yu05_mipi_raw_info;
#endif
#ifdef S5K3M3SM24
extern SENSOR_INFO_T g_s5k3m3sm24_mipi_raw_info;
#endif

extern otp_drv_entry_t imx258_drv_entry;
extern otp_drv_entry_t ov13855_drv_entry;
extern otp_drv_entry_t ov13855_sunny_drv_entry;
extern otp_drv_entry_t ov5675_sunny_drv_entry;
extern otp_drv_entry_t imx258_truly_drv_entry;
extern otp_drv_entry_t ov13855_altek_drv_entry;
extern otp_drv_entry_t s5k3l8xxm3_qtech_drv_entry;
extern otp_drv_entry_t s5k3p8sm_truly_drv_entry;
extern otp_drv_entry_t gc5024_common_drv_entry;
extern otp_drv_entry_t s5k3l8xxm3_reachtech_drv_entry;
extern otp_drv_entry_t gc2375_altek_drv_entry;
extern otp_drv_entry_t ov8856_cmk_drv_entry;
extern otp_drv_entry_t ov8858_cmk_drv_entry;
extern otp_drv_entry_t ov2680_cmk_drv_entry;
extern otp_drv_entry_t sp8407_otp_entry;
extern otp_drv_entry_t sp8407_cmk_otp_entry;
extern otp_drv_entry_t ov8856_shine_otp_entry;
extern otp_drv_entry_t s5k5e8yx_jd_otp_entry;
extern otp_drv_entry_t dual_master_2e_otp_entry;
extern otp_drv_entry_t dual_slave_2e_otp_entry;
extern otp_drv_entry_t single_1e_otp_entry;
extern otp_drv_entry_t general_otp_entry;
extern otp_drv_entry_t hi846_drv_entry;

extern struct sns_af_drv_entry dw9800_drv_entry;
extern struct sns_af_drv_entry dw9714_drv_entry;
extern struct sns_af_drv_entry dw9714a_drv_entry;
extern struct sns_af_drv_entry dw9714p_drv_entry;
extern struct sns_af_drv_entry dw9714v_drv_entry;
extern struct sns_af_drv_entry dw9718s_drv_entry;
extern struct sns_af_drv_entry bu64297gwz_drv_entry;
extern struct sns_af_drv_entry vcm_ak7371_drv_entry;
extern struct sns_af_drv_entry lc898214_drv_entry;
extern struct sns_af_drv_entry lc898213_drv_entry;
extern struct sns_af_drv_entry dw9763_drv_entry;
extern struct sns_af_drv_entry dw9763a_drv_entry;
extern struct sns_af_drv_entry vcm_zc524_drv_entry;
extern struct sns_af_drv_entry ad5823_drv_entry;
extern struct sns_af_drv_entry vm242_drv_entry;
extern struct sns_af_drv_entry dw9763r_drv_entry;
extern struct sns_af_drv_entry ces6301_drv_entry;
extern struct sns_af_drv_entry vcm_zc533_drv_entry;

/**
 * NOTE: the interface can only be used by sensor ic.
 *       not for otp and vcm device.
 **/
const cmr_u8 camera_module_name_str[MODULE_MAX][20] = {
        [0] = "default",
        [MODULE_SUNNY] = "Sunny",
        [MODULE_TRULY] = "Truly",
        [MODULE_RTECH] = "ReachTech",
        [MODULE_QTECH] = "Qtech",
        [MODULE_ALTEK] = "Altek", /*5*/
        [MODULE_CMK] = "CameraKing",
        [MODULE_SHINE] = "Shine",
        [MODULE_DARLING] = "Darling",
        [MODULE_BROAD] = "Broad",
        [MODULE_DMEGC] = "DMEGC", /*10*/
        [MODULE_SEASONS] = "Seasons",
        [MODULE_SUNWIN] = " Sunwin",
        [MODULE_OFLIM] = "Oflim",
        [MODULE_HONGSHI] = "Hongshi",
        [MODULE_SUNNINESS] = "Sunniness", /*15*/
        [MODULE_RIYONG] = "Riyong",
        [MODULE_TONGJU] = "Tongju",
        [MODULE_A_KERR] = "A-kerr",
        [MODULE_LITEARRAY] = "LiteArray",
        [MODULE_HUAQUAN] = "Huaquan",
        [MODULE_KINGCOM] = "Kingcom",
        [MODULE_BOOYI] = "Booyi",
        [MODULE_LAIMU] = "Laimu",
        [MODULE_WDSEN] = "Wdsen",
        [MODULE_SUNRISE] = "Sunrise",

    /*add you camera module name following*/
};

/*---------------------------------------------------------------------------*
 **                         Constant Variables                                *
 **---------------------------------------------------------------------------*/

//{.sn_name = "imx258_mipi_raw", .sensor_info =  &g_imx258_mipi_raw_info,
// .af_dev_info = {.af_drv_entry = &dw9800_drv_entry, .af_work_mode = 0},
// &imx258_drv_entry},

const SENSOR_MATCH_T back_sensor_infor_tab[] = {
// gc area
#ifdef GC5005
    {MODULE_SUNNY, "gc5005", &g_gc5005_mipi_raw_info, {&dw9714_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef GC8024
    {MODULE_SUNNY, "gc8024", &g_gc8024_mipi_raw_info, {&dw9714_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef GC030A
    {MODULE_SUNNY, "gc030a", &g_gc030a_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef GC2385
    {MODULE_SUNNY, "gc2385", &g_gc2385_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

// ov area
#ifdef OV8856_SHINE
    {MODULE_SUNNY, "ov8856_shine", &g_ov8856_shine_mipi_raw_info, {&dw9714p_drv_entry, 0}, {&general_otp_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef OV8856
    {MODULE_SUNNY, "ov8856", &g_ov8856_mipi_raw_info, {&dw9763_drv_entry, 0}, {&general_otp_entry, 0xB0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef OV8858
    {MODULE_SUNNY, "ov8858", &g_ov8858_mipi_raw_info, {&dw9763a_drv_entry, 0}, {&general_otp_entry, 0xB0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef OV13855
#if defined(CONFIG_DUAL_MODULE)
    {MODULE_SUNNY, "ov13855", &g_ov13855_mipi_raw_info, {&vcm_zc524_drv_entry, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_TWO_EEPROM, 8192}},
#else
    {MODULE_SUNNY, "ov13855", &g_ov13855_mipi_raw_info, {&dw9718s_drv_entry, 0}, {&ov13855_drv_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#endif
#ifdef OV13855_SUNNY
    {MODULE_SUNNY, "ov13855_sunny", &g_ov13855_sunny_mipi_raw_info, {&vcm_zc524_drv_entry, 0}, {&ov13855_sunny_drv_entry, 0xA0, DUAL_CAM_TWO_EEPROM, 8192}},
#endif
#ifdef OV13855A
    {MODULE_SUNNY, "ov13855a", &g_ov13855a_mipi_raw_info, {&bu64297gwz_drv_entry, 0}, {&ov13855_altek_drv_entry, 0xA0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef OV16885
    {MODULE_SUNNY, "ov16885", &g_ov16885_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef OV12A10
    {MODULE_SUNNY, "ov12a10", &g_ov12a10_mipi_raw_info, {&dw9714v_drv_entry, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif

// imx area
#ifdef IMX258
     {MODULE_SUNNY, "imx258", &g_imx258_mipi_raw_info, {&dw9714_drv_entry, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef IMX135
    {MODULE_SUNNY, "imx135", &g_imx135_mipi_raw_info, {&ad5823_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef IMX230
    {MODULE_SUNNY, "imx230", &g_imx230_mipi_raw_info, {&dw9800_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef IMX351
    {MODULE_SUNNY, "imx351", &g_imx351_mipi_raw_info, {&dw9714p_drv_entry, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_TWO_EEPROM, 8192}},
#endif
#ifdef IMX362
    {MODULE_SUNNY, "imx362", &g_imx362_mipi_raw_info, {&lc898213_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef IMX363
#ifdef SHARKL5_OIS_IMX363
    {MODULE_SUNNY, "imx363", &g_imx363_mipi_raw_info, {&lc898213_drv_entry, 0}, {NULL, 0, 0, 0}},
#else
    {MODULE_SUNNY, "imx363", &g_imx363_mipi_raw_info, {&dw9800_drv_entry, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
#endif
#ifdef IMX386
    {MODULE_SUNNY, "imx386", &g_imx386_mipi_raw_info, {&vcm_ak7371_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif

// cista area
#ifdef C2390
    {MODULE_SUNNY, "c2390", &g_c2390_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

// sp area
#ifdef SP2509
    {MODULE_SUNNY, "sp2509", &g_sp2509_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef SP2509Z
    {MODULE_SUNNY, "sp2509z", &g_sp2509z_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef SP250A
    {MODULE_SUNNY, "sp250a", &g_sp250a_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef SP8407
#ifdef SBS_SENSOR_FRONT
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {&dw9763_drv_entry, 0}, {&sp8407_cmk_otp_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#else
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {&dw9763a_drv_entry, 0}, {&sp8407_otp_entry, 0xB0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#endif

// samsung area
#ifdef S5K3L8XXM3R
#if defined(CONFIG_DUAL_MODULE)
    {MODULE_SUNNY, "s5k3l8xxm3r", &g_s5k3l8xxm3r_mipi_raw_info, {&vm242_drv_entry, 0}, {NULL, 0, 0, 0}},
#else
    {MODULE_SUNNY, "s5k3l8xxm3r", &g_s5k3l8xxm3r_mipi_raw_info, {&dw9763r_drv_entry, 0}, {&s5k3l8xxm3_reachtech_drv_entry, 0xB0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#endif
#ifdef S5K3L8XXM3
    {MODULE_SUNNY, "s5k3l8xxm3", &g_s5k3l8xxm3_mipi_raw_info, {&dw9714_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef S5K3P8SM
    {MODULE_SUNNY, "s5k3p8sm", &g_s5k3p8sm_mipi_raw_info, {&bu64297gwz_drv_entry, 0}, {&s5k3p8sm_truly_drv_entry, 0, 0, 0}},
#endif
#ifdef S5K5E8YX
    {MODULE_SUNNY, "s5k5e8yx", &g_s5k5e8yx_mipi_raw_info, {&dw9714_drv_entry, 4}, {&s5k5e8yx_jd_otp_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef S5K4H9YX
    {MODULE_SUNNY, "s5k4h9yx", &g_s5k4h9yx_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef S5K3L6
    {MODULE_SUNNY, "s5k3l6", &g_s5k3l6_mipi_raw_info, {&dw9714_drv_entry, 4}, {&general_otp_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef S5K3P9SX04
    {MODULE_SUNNY, "s5k3p9sx04", &g_s5k3p9sx04_mipi_raw_info, {&vcm_zc533_drv_entry, 0}, {&general_otp_entry, 0xB0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
// hynix area
#ifdef HI846
    {MODULE_SUNNY, "hi846", &g_hi846_mipi_raw_info, {&dw9714_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif

    {0, "0", NULL, {NULL, 0}, {NULL, 0, 0, 0}}};

const SENSOR_MATCH_T front_sensor_infor_tab[] = {
// gc area
#ifdef GC030A_F
    {MODULE_SUNNY, "gc030a_f", &g_gc030af_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef GC030A_T
    {MODULE_SUNNY, "gc030a_t", &g_gc030at_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef GC2375
    {MODULE_SUNNY, "gc2375", &g_gc2375_mipi_raw_info, {NULL, 0}, {NULL, 0, 0 ,0}},
#endif
#ifdef GC5005
    {MODULE_SUNNY, "gc5005", &g_gc5005_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef GC5024
    {MODULE_SUNNY, "gc5024", &g_gc5024_mipi_raw_info, {NULL, 0}, {&gc5024_common_drv_entry, 0, 0, 0}},
#endif
#ifdef GC2145
    {MODULE_SUNNY, "gc2145", &g_gc2145_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

// ov area
#ifdef OV5675
    {MODULE_DARLING, "ov5675", &g_ov5675_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef OV8856
    {MODULE_SUNNY, "ov8856", &g_ov8856_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef OV8856_SHINE
#ifdef SENSOR_OV8856_TELE
    {MODULE_SUNNY, "ov8856_shine", &g_ov8856_shine_mipi_raw_info, {&dw9714_drv_entry, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_TWO_EEPROM, 8192}},
#elif defined(OV8856_NO_VCM_SENSOR_OTP)
    {MODULE_SUNNY, "ov8856_shine", &g_ov8856_shine_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#else
    {MODULE_SUNNY, "ov8856_shine", &g_ov8856_shine_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xA2, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif
#endif
#ifdef OV8858
    {MODULE_SUNNY, "ov8858", &g_ov8858_mipi_raw_info, {&dw9763a_drv_entry, 0}, {&ov8858_cmk_drv_entry, 0xB0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef OV13850R2A
    {MODULE_SUNNY,  "ov13850r2a", &g_ov13850r2a_mipi_raw_info, {&dw9714a_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef OV7251
    {MODULE_SUNNY, "ov7251", &g_ov7251_mipi_raw_info, {NULL, 0}, NULL},
#endif
#ifdef OV16885
    {MODULE_SUNNY, "ov16885", &g_ov16885_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#endif

// sp area
#ifdef SP0A09
    {MODULE_SUNNY, "sp0a09", &g_sp0a09_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef SP0A09Z
    {MODULE_SUNNY, "sp0a09z", &g_sp0a09z_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

#ifdef SP8407
#ifdef CONFIG_FRONT_CAMERA_AUTOFOCUS
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {&dw9763_drv_entry, 0}, {&sp8407_cmk_otp_entry, 0xA0, SINGLE_CAM_ONE_EEPROM, 8192}},
#else
    {MODULE_SUNNY, "sp8407", &g_sp8407_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#endif

// samsung area
#ifdef S5K3L8XXM3Q
    {MODULE_QTECH, "s5k3l8xxm3q", &g_s5k3l8xxm3q_mipi_raw_info, {NULL, 0}, {&s5k3l8xxm3_qtech_drv_entry, 0xA8, DUAL_CAM_TWO_EEPROM, 8192}},
#endif
#ifdef S5K4H8YX
    {MODULE_SUNNY, "s5k4h8yx", &g_s5k4h8yx_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef S5K5E2YA
    {MODULE_SUNNY, "s5k5e2ya", &g_s5k5e2ya_mipi_raw_info, {&dw9714_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef S5K4H7
    {MODULE_SUNNY, "s5k4h7", &g_s5k4h7_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
// hynix area
#ifdef HI556
    {MODULE_SUNNY, "hi556", &g_hi556_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef HI846
    {MODULE_SUNNY, "hi846", &g_hi846_mipi_raw_info, {NULL, 0}, {&hi846_drv_entry, 0, 0, 0}},
#endif

    {0, "0", NULL, {NULL, 0}, {NULL, 0, 0, 0}}};

const SENSOR_MATCH_T back_sensor2_infor_tab[] = {
// ov area
#ifdef OV2680
    {MODULE_SUNNY, "ov2680", &g_ov2680_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xB0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif
#ifdef OV5675_DUAL
#ifdef OV5675_VCM_OTP_DUAL_CAM_ONE_EEPROM
    {MODULE_SUNNY, "ov5675_dual", &g_ov5675_dual_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_ONE_EEPROM, 8192}},
#else
    {MODULE_SUNNY, "ov5675_dual", &g_ov5675_dual_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_TWO_EEPROM, 8192}},
#endif
#endif
#ifdef OV8856_SHINE
    {MODULE_SUNNY, "ov8856_shine", &g_ov8856_shine_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_TWO_EEPROM, 8192}},
#endif
#ifdef OV7251_DUAL
    {MODULE_SUNNY, "ov7251_dual", &g_ov7251_dual_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

// gc area
#ifdef GC2165
    {MODULE_SUNNY, "gc2165", &g_gc2165_mipi_yuv_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef GC2375A
    {MODULE_SUNNY, "gc2375a", &g_gc2375a_mipi_raw_info, {NULL, 0}, {&gc2375_altek_drv_entry, 0xA0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif

// cista area
#ifdef C2580
    {MODULE_SUNNY, "c2580", &g_c2580_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

// sp area
#ifdef SP2509V
    {MODULE_SUNNY, "sp2509v", &g_sp2509v_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef SP2509R
    {MODULE_SUNNY, "sp2509r", &g_sp2509r_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef SP2509ZZ
    {MODULE_SUNNY, "sp2509zz", &g_sp2509zz_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xA0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif

// samsung area
#ifdef S5K3M3SM24
    {MODULE_SUNNY, "s5k3m3sm24", &g_s5k3m3sm24_mipi_raw_info, {&dw9714a_drv_entry, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef S5K5E9YU05
    {MODULE_SUNNY, "s5k5e9yu05", &g_s5k5e9yu05_mipi_raw_info, {NULL, 0}, {&general_otp_entry, 0xB0, DUAL_CAM_ONE_EEPROM, 8192}},
#endif

    {0, "0", NULL, {NULL, 0}, {NULL, 0, 0, 0}}};

const SENSOR_MATCH_T front_sensor2_infor_tab[] = {
// ov area
#ifdef OV7251
    {MODULE_SUNNY, "ov7251", &g_ov7251_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

// cista area
#ifdef C2580
    {MODULE_SUNNY, "c2580", &g_c2580_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

    {0, "0", NULL, {NULL, 0}, {NULL, 0, 0, 0}}};
const SENSOR_MATCH_T back_sensor3_infor_tab[] = {
#ifdef OV7251
    {MODULE_SUNNY, "ov7251", &g_ov7251_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef OV7251_DUAL
    {MODULE_SUNNY, "ov7251_dual", &g_ov7251_dual_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

    {0, "0", NULL, {NULL, 0}, {NULL, 0, 0, 0}}};

const SENSOR_MATCH_T front_sensor3_infor_tab[] = {
#ifdef OV7251
    {MODULE_SUNNY, "ov7251", &g_ov7251_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif
#ifdef OV7251_DUAL
    {MODULE_SUNNY, "ov7251_dual", &g_ov7251_dual_mipi_raw_info, {NULL, 0}, {NULL, 0, 0, 0}},
#endif

    {0, "0", NULL, {NULL, 0}, {NULL, 0, 0, 0}}};

const SNS_MULTI_CAMERA_INFO_T multi_camera_sensor_group[] = {
#ifdef CONFIG_BOKEH_SUPPORT
    {SPRD_BLUR_ID, MODE_BOKEH, 2, {"imx351", "0", "ov8856_shine", "0", "0", "0"}, SNS_FACE_BACK, 90},
    {SPRD_BLUR_ID, MODE_BOKEH, 2, {"imx351", "0", "ov5675_dual", "0", "0", "0"}, SNS_FACE_BACK, 90},
    {SPRD_BLUR_ID, MODE_BOKEH, 2, {"ov13855", "0", "ov5675_dual", "0", "0", "0"}, SNS_FACE_BACK, 90},
    {SPRD_BLUR_ID, MODE_BOKEH, 2, {"ov12a10", "0", "ov5675_dual", "0", "0", "0"}, SNS_FACE_BACK, 90},
    {SPRD_BLUR_ID, MODE_BOKEH, 2, {"ov8856", "0", "ov2680", "0", "0", "0"}, SNS_FACE_BACK, 90},
    {SPRD_BLUR_ID, MODE_BOKEH, 2, {"s5k3p9sx04", "0", "s5k5e9yu05", "0", "0", "0"}, SNS_FACE_BACK, 90},
    {SPRD_BLUR_ID, MODE_BOKEH, 2, {"imx363", "0", "ov5675_dual", "0", "0", "0"}, SNS_FACE_BACK, 90},
#endif
#ifdef CONFIG_BLUR_SUPPORT
    {SPRD_BLUR_FRONT_ID, MODE_BLUR, 0, {"any", "any", "any", "any", "any", "any"}, SNS_FACE_FRONT, 270},
#endif
#ifdef CONFIG_SINGLE_FACEID_SUPPORT
    {SPRD_SINGLE_FACEID_REGISTER_ID, MODE_SINGLE_FACEID_REGISTER, 0, {"any", "any", "any", "any", "any", "any"}, SNS_FACE_FRONT, 270},
    {SPRD_SINGLE_FACEID_UNLOCK_ID, MODE_SINGLE_FACEID_UNLOCK, 0, {"any", "any", "any", "any", "any", "any"}, SNS_FACE_FRONT, 270},
#endif
#ifdef CONFIG_OPTICSZOOM_SUPPORT
    {SPRD_SOFY_OPTICAL_ZOOM_ID, MODE_SOFY_OPTICAL_ZOOM, 2, {"s5k3p9sx04", "ov8856_shine", "0", "0", "0", "0"}, SNS_FACE_BACK, 90},
#endif
#ifdef CONFIG_3DFACE_SUPPORT
    {SPRD_3D_FACE_ID, MODE_3D_FACE, 3, {"s5k4h9yx", "0", "ov7251_dual", "ov7251", "0", "0"}, SNS_FACE_FRONT, 270},
#endif
};

void sensor_customize_cam_attribute(PHYSICAL_SENSOR_INFO_T *phyPtr, cmr_u32 slot_id) {

    switch (slot_id) {
    case SENSOR_MAIN:
        phyPtr->sensor_role = 0;
        phyPtr->face_type = SNS_FACE_BACK;
        phyPtr->angle = 90;
        phyPtr->resource_cost = 100;
        break;
    case SENSOR_SUB:
        phyPtr->sensor_role = 0;
        phyPtr->face_type = SNS_FACE_FRONT;
        phyPtr->angle = 270;
        phyPtr->resource_cost = 100;
        break;
    case SENSOR_MAIN2:
        phyPtr->sensor_role = 0;
        phyPtr->face_type = SNS_FACE_BACK;
        phyPtr->angle = 90;
        phyPtr->resource_cost = 0;
        break;
    case SENSOR_SUB2:
        phyPtr->sensor_role = 0;
        phyPtr->face_type = SNS_FACE_FRONT;
        phyPtr->angle = 270;
        phyPtr->resource_cost = 0;
        break;
    case SENSOR_MAIN3:
        phyPtr->sensor_role = 0;
        phyPtr->face_type = SNS_FACE_BACK;
        phyPtr->angle = 90;
        phyPtr->resource_cost = 0;
        break;
    case SENSOR_SUB3:
        phyPtr->sensor_role = 0;
        phyPtr->face_type = SNS_FACE_FRONT;
        phyPtr->angle = 270;
        phyPtr->resource_cost = 0;
        break;
    }

}

SNS_MULTI_CAMERA_INFO_T *sensor_get_multi_cam_cfg_group(cmr_int *group_num) {
      SNS_MULTI_CAMERA_INFO_T *multiCameraGroupPtr = multi_camera_sensor_group;

      *group_num = ARRAY_SIZE(multi_camera_sensor_group);

      return multiCameraGroupPtr;
}

SENSOR_MATCH_T *sensor_get_regist_table(cmr_u32 sensor_id) {
    SENSOR_MATCH_T *sensor_reg_tab_ptr = NULL;

    switch (sensor_id) {
    case SENSOR_MAIN:
        sensor_reg_tab_ptr = (SENSOR_MATCH_T *)back_sensor_infor_tab;
        break;
    case SENSOR_SUB:
        sensor_reg_tab_ptr = (SENSOR_MATCH_T *)front_sensor_infor_tab;
        break;
    case SENSOR_MAIN2:
        sensor_reg_tab_ptr = (SENSOR_MATCH_T *)back_sensor2_infor_tab;
        break;
    case SENSOR_SUB2:
        sensor_reg_tab_ptr = (SENSOR_MATCH_T *)front_sensor2_infor_tab;
        break;
    case SENSOR_MAIN3:
        sensor_reg_tab_ptr = (SENSOR_MATCH_T *)back_sensor3_infor_tab;
        break;
    case SENSOR_SUB3:
        sensor_reg_tab_ptr = (SENSOR_MATCH_T *)front_sensor3_infor_tab;
        break;
    }

    return (SENSOR_MATCH_T *)sensor_reg_tab_ptr;
}

char *sensor_get_name_list(cmr_u32 sensor_id) {
    char *sensor_name_list_ptr = NULL;

    switch (sensor_id) {
    case SENSOR_MAIN:
        sensor_name_list_ptr = (char *)CAMERA_SENSOR_TYPE_BACK;
        break;
    case SENSOR_SUB:
        sensor_name_list_ptr = (char *)CAMERA_SENSOR_TYPE_FRONT;
        break;
    case SENSOR_MAIN2:
        sensor_name_list_ptr = (char *)CAMERA_SENSOR_TYPE_BACK2;
        break;
    case SENSOR_SUB2:
        sensor_name_list_ptr = (char *)CAMERA_SENSOR_TYPE_FRONT2;
        break;
    case SENSOR_MAIN3:
        sensor_name_list_ptr = (char *)CAMERA_SENSOR_TYPE_BACK3;
        break;
    case SENSOR_SUB3:
        sensor_name_list_ptr = (char *)CAMERA_SENSOR_TYPE_FRONT3;
        break;

    }

    return (char *)sensor_name_list_ptr;
}

SENSOR_MATCH_T *sensor_get_entry_by_idx(cmr_u32 sensor_id, cmr_u16 idx) {
    SENSOR_MATCH_T *sns_reg_tab_ptr = sensor_get_regist_table(sensor_id);

    if (sns_reg_tab_ptr == NULL)
        return NULL;

    sns_reg_tab_ptr += idx;

    return sns_reg_tab_ptr;
}

cmr_int sensor_check_name(cmr_u32 sensor_id, SENSOR_MATCH_T *reg_tab_ptr) {
    cmr_int ret = SENSOR_SUCCESS;
    const char *delimiters = ",";
    char *sns_name_list_ptr = sensor_get_name_list(sensor_id);
    char sns_name_str[MAX_SENSOR_NAME_LEN] = {0};
    char *token;

    memcpy(sns_name_str, sns_name_list_ptr,
           MIN(strlen(sns_name_list_ptr), MAX_SENSOR_NAME_LEN));
    for (token = strtok(sns_name_str, delimiters); token != NULL;
         token = strtok(NULL, delimiters)) {
        if (strcasecmp(reg_tab_ptr->sn_name, token) == 0) {
            SENSOR_LOGI("%s name match succesful\n", token);
            return ret;
        }
    }
    return SENSOR_FAIL;
}

