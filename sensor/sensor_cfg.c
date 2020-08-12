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

static struct lensProperty s9863a1h10_3h10_lens_property[SENSOR_ID_MAX] = {
    // bokeh master, imx351 or s5k3p9sx04, sensor_id 0
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9981, -0.0515, -0.0341, 0.0020},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {0, 0, 0},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {617.7280, 615.7952, 408.3161, 301.0174, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {0.0616, -0.1405, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},

    // bokeh slave, ov8856_shine or ov5675_dual, sensor_id 2
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9982, -0.0504, -0.0316, -0.0056},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {-0.0000348, -0.0122521, -0.0017998},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {536.3185, 534.7714, 395.648, 301.1897, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {0.0709, -0.145, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},
};

static struct lensProperty s9863a1c10_lens_property[SENSOR_ID_MAX] = {
    // bokeh master and opticszoom wide, hi1336_m0 or ov13853_m1, sensor_id 0
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9999, -0.0127, 0.0016, 0.0018},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {0, 0, 0},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {619.9433, 617.8813, 395.6978, 301.7107, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {0.066, -0.1447, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},

    // bokeh slave, gc02m1b_js_1 or gc2375_js_2, sensor_id 2
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9999, -0.0158, 0.0026, 0},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {-0.0105043, -0.0006576, -0.0004695},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {559.5789, 557.5552, 392.1703, 307.2183, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {0.0290, -0.0937, 0, 0, 0}},

    // opticszoom superwide, hi846_gj_1 or gc8034_gj_2, sensor_id 3, fake data
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9989, -0.0222, -0.0417, 0.0027},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {0.01, 0, 0},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {504.8576, 504.5636, 403.391, 300.2478, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {-0.0187, 0.0280, 0, 0, 0}},

    // macro lens, gc2385_wj_1 or gc2375_wj_2, sensor_id 4, fake data
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9989, -0.0222, -0.0417, 0.0027},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {-0.022, 0, 0},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {504.8576, 504.5636, 403.391, 300.2478, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {-0.0187, 0.0280, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},
};

static struct lensProperty ums512_1h10_lens_property[SENSOR_ID_MAX] = {
    // bokeh master and opticszoom wide, ov32a1q, sensor_id 0
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9989, -0.0230, -0.0419, 0.0009},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {0, 0, 0},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {597.9681, 597.7089, 401.0117, 300.2753, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {0.0351, -0.1006, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},

    // bokeh slave and opticszoom superwide, ov16885_normal, sensor_id 2
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9989, -0.0222, -0.0417, 0.0027},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {-0.0201775, -0.0004498, -0.000578},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {504.8576, 504.5636, 403.391, 300.2478, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {-0.0187, 0.0280, 0, 0, 0}},

    // opticszoom tele, ov8856_shine, sensor_id 3, fake data
    {// poseRotation (x, y, z, w)
     .poseRotation = {0.9989, -0.0230, -0.0419, 0.0009},
     // poseTranslation (x, y, z), unit: meter
     .poseTranslation = {-0.01, 0, 0},
     // intrinsicCalibration [f_x, f_y, c_x, c_y, s], unit: pixel
     .intrinsicCalibration = {597.9681, 597.7089, 401.0117, 300.2753, 0},
     // distortion [kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5]
     .distortion = {0.0351, -0.1006, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},

    {.poseRotation = {0, 0, 0, 0},
     .poseTranslation = {0, 0, 0},
     .intrinsicCalibration = {0, 0, 0, 0, 0},
     .distortion = {0, 0, 0, 0, 0}},
};

struct lensProperty *sensor_get_lens_property(int phy_id) {
    struct phySensorInfo *phyPtr = NULL;
    struct lensProperty *lenPtr = NULL;

    if (phy_id >= SENSOR_ID_MAX)
        return NULL;

    phyPtr = sensorGetPhysicalSnsInfo(0);

    if (!strcmp(phyPtr->sensor_name, "imx351") ||
        !strcmp(phyPtr->sensor_name, "s5k3p9sx04")) {
        // sharkl3 bokeh module imx351/s5k3p9sx04 + ov8856_shine/ov5675_dual
        lenPtr = &s9863a1h10_3h10_lens_property[phy_id];
    } else if (!strcmp(phyPtr->sensor_name, "hi1336_m0") ||
               !strcmp(phyPtr->sensor_name, "ov13853_m1")) {
        // sharkl3 1c10, hi1336_m0/ov13853_m1, gc02m1b_js_1/gc2375_js_2,
        // hi846_gj_1/gc8034_gj_2, gc2385_wj_1/gc2375_wj_2
        lenPtr = &s9863a1c10_lens_property[phy_id];
    } else if (!strcmp(phyPtr->sensor_name, "ov32a1q")) {
        // sharkl5pro SW + W + T module ov16885_normal + ov32a1q + ov8856_shine
        lenPtr = &ums512_1h10_lens_property[phy_id];
    }

    SENSOR_LOGV("phy_id %d, sensor_name %s", phy_id,
                (phyPtr + phy_id)->sensor_name);
    SENSOR_LOGV("poseRotation(x, y, z, w) = (%f, %f, %f, %f)",
                lenPtr->poseRotation[0], lenPtr->poseRotation[1],
                lenPtr->poseRotation[2], lenPtr->poseRotation[3]);
    SENSOR_LOGV("poseTranslation(x, y, z) = (%f, %f, %f)",
                lenPtr->poseTranslation[0], lenPtr->poseTranslation[1],
                lenPtr->poseTranslation[2]);
    SENSOR_LOGV(
        "intrinsicCalibration[f_x, f_y, c_x, c_y, s]= [%f, %f, %f, %f, %f]",
        lenPtr->intrinsicCalibration[0], lenPtr->intrinsicCalibration[1],
        lenPtr->intrinsicCalibration[2], lenPtr->intrinsicCalibration[3],
        lenPtr->intrinsicCalibration[4]);
    SENSOR_LOGV("distortion[kappa_1, kappa_2, kappa_3] [kappa_4, kappa_5] = "
                "[%f, %f, %f] [%f, %f]",
                lenPtr->distortion[0], lenPtr->distortion[1],
                lenPtr->distortion[2], lenPtr->distortion[3],
                lenPtr->distortion[4]);

    return lenPtr;
}
