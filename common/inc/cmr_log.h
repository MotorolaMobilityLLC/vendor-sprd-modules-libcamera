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

#ifndef _CMR_LOG_H_
#define _CMR_LOG_H_

#include "cam_log.h"
/*
 * LEVEL_OVER_LOGE - only show ALOGE, err log is always show
 * LEVEL_OVER_LOGW - show ALOGE and ALOGW
 * LEVEL_OVER_LOGI - show ALOGE, ALOGW and ALOGI
 * LEVEL_OVER_LOGD - show ALOGE, ALOGW, ALOGI and ALOGD
 * LEVEL_OVER_LOGV - show ALOGE, ALOGW, ALOGI and ALOGD, ALOGV
 */
enum {
    LEVEL_OVER_LOGE = 1,
    LEVEL_OVER_LOGW,
    LEVEL_OVER_LOGI,
    LEVEL_OVER_LOGD,
    LEVEL_OVER_LOGV,
};

#define DEBUG_STR "%d, %s: "
#define ERROR_STR "%d, %s: hal_err "
#define DEBUG_ARGS __LINE__, __FUNCTION__

extern long g_isp_log_level;
extern long g_oem_log_level;
extern long g_sensor_log_level;

#ifndef WIN32
#define ISP_LOGE(format, args...) LOG(LOG_PRI_E, LOG_ISP_MW, LOG_ALL, format, ##args)
#define ISP_LOGW(format, args...) LOG(LOG_PRI_W, LOG_ISP_MW, LOG_ALL, format, ##args)
#define ISP_LOGI(format, args...) LOG(LOG_PRI_I, LOG_ISP_MW, LOG_ALL, format, ##args)
#define ISP_LOGD(format, args...) LOG(LOG_PRI_D, LOG_ISP_MW, LOG_ALL, format, ##args)
#define ISP_LOGV(format, args...) LOG(LOG_PRI_V, LOG_ISP_MW, LOG_ALL, format, ##args)

#define F_ISP_LOGE(func, format, args...) LOG(LOG_PRI_E, LOG_ISP_MW, func, format, ##args)
#define F_ISP_LOGW(func, format, args...) LOG(LOG_PRI_W, LOG_ISP_MW, func, format, ##args)
#define F_ISP_LOGI(func, format, args...) LOG(LOG_PRI_I, LOG_ISP_MW, func, format, ##args)
#define F_ISP_LOGD(func, format, args...) LOG(LOG_PRI_D, LOG_ISP_MW, func, format, ##args)
#define F_ISP_LOGV(func, format, args...) LOG(LOG_PRI_V, LOG_ISP_MW, func, format, ##args)
#else
#define ISP_LOGE printf
#define ISP_LOGW printf
#define ISP_LOGI printf
#define ISP_LOGD printf
#define ISP_LOGV printf
#endif

#define CMR_LOGE(format, args...) LOG(LOG_PRI_E, LOG_OEM, LOG_ALL, format, ##args)
#define CMR_LOGW(format, args...) LOG(LOG_PRI_W, LOG_OEM, LOG_ALL, format, ##args)
#define CMR_LOGI(format, args...) LOG(LOG_PRI_I, LOG_OEM, LOG_ALL, format, ##args)
#define CMR_LOGD(format, args...) LOG(LOG_PRI_D, LOG_OEM, LOG_ALL, format, ##args)
#define CMR_LOGV(format, args...) LOG(LOG_PRI_V, LOG_OEM, LOG_ALL, format, ##args)

#define F_CMR_LOGE(func, format, args...) LOG(LOG_PRI_E, LOG_OEM, func, format, ##args)
#define F_CMR_LOGW(func, format, args...) LOG(LOG_PRI_W, LOG_OEM, func, format, ##args)
#define F_CMR_LOGI(func, format, args...) LOG(LOG_PRI_I, LOG_OEM, func, format, ##args)
#define F_CMR_LOGD(func, format, args...) LOG(LOG_PRI_D, LOG_OEM, func, format, ##args)
#define F_CMR_LOGV(func, format, args...) LOG(LOG_PRI_V, LOG_OEM, func, format, ##args)


#define SENSOR_LOGE(format, args...) LOG(LOG_PRI_E, LOG_SENSOR, LOG_ALL, format, ##args)
#define SENSOR_LOGW(format, args...) LOG(LOG_PRI_E, LOG_SENSOR, LOG_ALL, format, ##args)
#define SENSOR_LOGI(format, args...) LOG(LOG_PRI_E, LOG_SENSOR, LOG_ALL, format, ##args)
#define SENSOR_LOGD(format, args...) LOG(LOG_PRI_E, LOG_SENSOR, LOG_ALL, format, ##args)
#define SENSOR_LOGV(format, args...) LOG(LOG_PRI_E, LOG_SENSOR, LOG_ALL, format, ##args)

#define F_SENSOR_LOGE(func, format, args...) LOG(LOG_PRI_E, LOG_SENSOR, func, format, ##args)
#define F_SENSOR_LOGW(func, format, args...) LOG(LOG_PRI_E, LOG_SENSOR, func, format, ##args)
#define F_SENSOR_LOGI(func, format, args...) LOG(LOG_PRI_E, LOG_SENSOR, func, format, ##args)
#define F_SENSOR_LOGD(func, format, args...) LOG(LOG_PRI_E, LOG_SENSOR, func, format, ##args)
#define F_SENSOR_LOGV(func, format, args...) LOG(LOG_PRI_E, LOG_SENSOR, func, format, ##args)


#define SENSOR_PRINT_ERR SENSOR_LOGE
#define SENSOR_PRINT_HIGH SENSOR_LOGI
#define SENSOR_PRINT SENSOR_LOGI
#define SENSOR_TRACE SENSOR_LOGI

#define ARITH_LOGE(format, args...) LOG(LOG_PRI_E, LOG_ARITH, LOG_ALL, format, ##args)
#define ARITH_LOGW(format, args...) LOG(LOG_PRI_E, LOG_ARITH, LOG_ALL, format, ##args)
#define ARITH_LOGI(format, args...) LOG(LOG_PRI_E, LOG_ARITH, LOG_ALL, format, ##args)
#define ARITH_LOGD(format, args...) LOG(LOG_PRI_E, LOG_ARITH, LOG_ALL, format, ##args)
#define ARITH_LOGV(format, args...) LOG(LOG_PRI_E, LOG_ARITH, LOG_ALL, format, ##args)

#define F_ARITH_LOGE(func, format, args...) LOG(LOG_PRI_E, LOG_ARITH, func, format, ##args)
#define F_ARITH_LOGW(func, format, args...) LOG(LOG_PRI_E, LOG_ARITH, func, format, ##args)
#define F_ARITH_LOGI(func, format, args...) LOG(LOG_PRI_E, LOG_ARITH, func, format, ##args)
#define F_ARITH_LOGD(func, format, args...) LOG(LOG_PRI_E, LOG_ARITH, func, format, ##args)
#define F_ARITH_LOGV(func, format, args...) LOG(LOG_PRI_E, LOG_ARITH, func, format, ##args)


void isp_init_log_level(void);
void oem_init_log_level(void);
void sensor_init_log_level(void);

#endif
