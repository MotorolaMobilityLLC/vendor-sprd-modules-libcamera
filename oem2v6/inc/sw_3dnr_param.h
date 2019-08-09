#ifndef _SW_3DNR_PARAM_H
#include "../../sensor/sensor_drv/sensor_ic_drv.h"
#define _SW_3DNR_PARAM_H

#define MAX_SENSOR_NAME_LEN 128

struct threednr_tuning_param {
cmr_s32 threshold[4][6];
cmr_s32 slope[4][6];
cmr_s32 gain_thr[6];
cmr_u16 searchWindow_x;
cmr_u16 searchWindow_y;
cmr_s32 recur_str;
cmr_s32 match_ratio_sad;
cmr_s32 match_ratio_pro;
cmr_s32 feat_thr;
cmr_s32 zone_size;
cmr_s32 luma_ratio_high;
cmr_s32 luma_ratio_low;
cmr_s32 reserverd[16];
};

struct threednr_sns_match_tab {
    char sn_name[36];
    struct threednr_tuning_param *prev_param;
    struct threednr_tuning_param *cap_param;
};

static struct threednr_tuning_param prev_default_3dnr_param = {
    {{0, 2, 4, 9, 9, 9}, {0, 1, 5, 9, 9, 9}, {0, 1, 5, 9, 9, 9}, {0, 1, 6, 9, 9, 9}},
    {{255, 5, 6, 9, 9, 9}, {255, 5, 6, 9, 9, 9}, {255, 5, 6, 9, 9, 9}, {255, 4, 5, 9, 9, 9}},
    {6, 12, 18, 24, 30, 36}, 11, 11, 3, 0, 12, 100, 5, 665, 410, {0}
};

static struct threednr_tuning_param cap_default_3dnr_param = {
    {{3, 4, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {2, 6, 7, 9, 9, 9}},
    {{5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 6, 9, 9, 9}},
    {6, 12, 18, 24, 30, 36}, 11, 11, -1, 0, 12, 100, 6, 665, 410, {0}
};

#ifdef OV13855
static struct threednr_tuning_param ov13855_prev_3dnr_param = {
    {{0, 2, 2, 4, 9, 9}, {0, 1, 3, 5, 9, 9}, {0, 1, 3, 5, 9, 9}, {0, 1, 4, 6, 9, 9}},
    {{255, 5, 5, 6, 9, 9}, {255, 5, 5, 6, 9, 9}, {255, 5, 5, 6, 9, 9}, {255, 4, 5, 5, 9, 9}},
    {12, 24, 36, 64, 80, 96}, 11, 11, 1, 0, 12, 100, 5, 588, 435, {1,3,247,3342362,6684749,10027136,13369523,16711910}
};
static struct threednr_tuning_param ov13855_cap_3dnr_param = {
    {{3, 4, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {2, 6, 7, 9, 9, 9}},
    {{5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 6, 9, 9, 9}},
    {12, 24, 36, 64, 80, 96}, 11, 11, -1, 0, 12, 100, 6, 588, 435, {0}
};
#endif

#ifdef OV8856_SHINE
static struct threednr_tuning_param ov8856_shine_prev_3dnr_param = {
    {{0, 2, 2, 4, 9, 9}, {0, 1, 3, 5, 9, 9}, {0, 1, 3, 5, 9, 9}, {0, 1, 4, 6, 9, 9}},
    {{255, 5, 5, 6, 9, 9}, {255, 5, 5, 6, 9, 9}, {255, 5, 5, 6, 9, 9}, {255, 4, 5, 5, 9, 9}},
    {12, 24, 36, 64, 80, 96}, 11, 11, 1, 0, 12, 100, 5, 588, 435, {1,3,247,3342362,6684749,10027136,13369523,16711910}
};
static struct threednr_tuning_param ov8856_shine_cap_3dnr_param = {
    {{3, 4, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {2, 6, 7, 9, 9, 9}},
    {{5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 6, 9, 9, 9}},
    {12, 24, 36, 64, 80, 96}, 11, 11, -1, 0, 12, 100, 6, 588, 435, {0}
};
#endif

#ifdef OV5675
static struct threednr_tuning_param ov5675_prev_3dnr_param = {
    {{0, 2, 2, 4, 9, 9}, {0, 1, 3, 5, 9, 9}, {0, 1, 3, 5, 9, 9}, {0, 1, 4, 6, 9, 9}},
    {{255, 5, 5, 6, 9, 9}, {255, 5, 5, 6, 9, 9}, {255, 5, 5, 6, 9, 9}, {255, 4, 5, 5, 9, 9}},
    {12, 24, 36, 64, 80, 96}, 11, 11, 1, 0, 12, 100, 5, 588, 435, {1,3,247,3342362,6684749,10027136,13369523,16711910}
};
static struct threednr_tuning_param ov5675_cap_3dnr_param = {
    {{3, 4, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {3, 5, 6, 9, 9, 9}, {2, 6, 7, 9, 9, 9}},
    {{5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 7, 9, 9, 9}, {5, 6, 6, 9, 9, 9}},
    {12, 24, 36, 64, 80, 96}, 11, 11, -1, 0, 12, 100, 6, 588, 435, {0}
};
#endif

const struct threednr_sns_match_tab sns_3dnr_param_tab[] = {
    {"default", &prev_default_3dnr_param, &cap_default_3dnr_param},
#ifdef OV13855
    {"ov13855", &ov13855_prev_3dnr_param, &ov13855_cap_3dnr_param},
#endif
#ifdef OV8856_SHINE
    {"ov8856_shine", &ov8856_shine_prev_3dnr_param, &ov8856_shine_cap_3dnr_param},
#endif
#ifdef OV5675
    {"ov5675", &ov5675_prev_3dnr_param, &ov5675_cap_3dnr_param},
#endif
    {"null", NULL, NULL}
};
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
    }

    return (char *)sensor_name_list_ptr;
}

static cmr_u32 threednr_get_sns_match_index(cmr_uint sensor_id) {
    cmr_u32 i = 0;
    cmr_u32 retValue = 0;
    cmr_u32 mSnNum = 0;

    const char *delimiters = ",";
    char *sns_name_list_ptr = sensor_get_name_list(sensor_id);
    char sns_name_str[MAX_SENSOR_NAME_LEN] = {0};
    char *token;

    memcpy(sns_name_str, sns_name_list_ptr,
           MIN(strlen(sns_name_list_ptr), MAX_SENSOR_NAME_LEN));

    mSnNum = ARRAY_SIZE(sns_3dnr_param_tab) - 1;
    CMR_LOGD("sensor 3dnr param sum is %d", mSnNum);

    for (token = strtok(sns_name_str, delimiters); token != NULL;
         token = strtok(NULL, delimiters)) {
        for (i = 0; i < mSnNum; i++) {
            if (strcasecmp(sns_3dnr_param_tab[i].sn_name, token) == 0) {
                CMR_LOGI("sensor 3dnr param matched %dth  is %s", i, token);
                retValue = i;
                break;
            }
        }
    }
    return retValue;
}

#endif
