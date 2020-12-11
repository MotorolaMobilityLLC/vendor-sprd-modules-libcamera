#ifndef CAMERA_IT_COMMON_H_
#define CAMERA_IT_COMMON_H_

#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <stdio.h>
#include <string>
#include <string>
#include <time.h>
#include <vector>

using namespace std;

#include <cutils/log.h>

#define IT_LOGE(fmt, args...)                                                  \
    do {                                                                       \
        ALOGE("%s:%d: " fmt, __func__, __LINE__, ##args);                      \
        printf(" E %s: %d, %s: ", LOG_TAG, __LINE__, __func__);                \
        printf(fmt "\n", ##args);                                              \
    } while (0)

#define IT_LOGV(fmt, args...)                                                  \
    do {                                                                       \
        ALOGV("%s:%d: " fmt, __func__, __LINE__, ##args);                      \
        printf(" V %s: %d, %s: ", LOG_TAG, __LINE__, __func__);                \
        printf(fmt "\n", ##args);                                              \
    } while (0)

#define IT_LOGD(fmt, args...)                                                  \
    do {                                                                       \
        ALOGD("%s:%d: " fmt, __func__, __LINE__, ##args);                      \
        printf(" D %s: %d, %s: ", LOG_TAG, __LINE__, __func__);                \
        printf(fmt "\n", ##args);                                              \
    } while (0)

#define IT_LOGI(fmt, args...)                                                  \
    do {                                                                       \
        ALOGI("%s:%d: " fmt, __func__, __LINE__, ##args);                      \
        printf(" I %s: %d, %s: ", LOG_TAG, __LINE__, __func__);                \
        printf(fmt "\n", ##args);                                              \
    } while (0)

#define IT_LOGW(fmt, args...)                                                  \
    do {                                                                       \
        ALOGW("%s:%d: " fmt, __func__, __LINE__, ##args);                      \
        printf(" W %s: %d, %s: ", LOG_TAG, __LINE__, __func__);                \
        printf(fmt "\n", ##args);                                              \
    } while (0)

typedef enum {
    IT_OK = 0,
    IT_ERR,
    IT_NO_THIS_CASE,
    IT_FILE_EMPTY,
    IT_UNKNOWN_FAULT,
    IT_NO_MEM,
    IT_TRY_AGAIN,
    IT_BAD_ADDR,
    IT_BUSY,
    IT_FILE_EXIST,
    IT_INVAL,
    IT_WRITE_FAIL,
    IT_STATUS_MAX,
} IT_STATUS_T;

typedef enum {
    IT_SIMULATOR_OFF = 0,
    IT_SIMULATOR_ON = 1
} IT_WORK_MODE_T;

typedef enum {
    IT_OFF = 0,
    IT_ON = 1
} IT_SWITCH_T;

typedef enum {
    IT_IMG_FORMAT_YUV,
    IT_IMG_FORMAT_JPEG,
    IT_IMG_FORMAT_RAW
} IT_IMG_FORMAT_T;

typedef struct compareInfo {
    unsigned char *test_img;
    unsigned char *golden_img;
    int w;
    int h;
    IT_IMG_FORMAT_T format;
} compareInfo_t;

#endif
