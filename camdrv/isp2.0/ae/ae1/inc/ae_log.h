/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_LOG_H_
#define _AE_LOG_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/**---------------------------------------------------------------------------*
**				 Macro Define				*
**----------------------------------------------------------------------------*/
#ifndef WIN32

#include <sys/types.h>
#include <android/log.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

enum {
	AE_LOG_LEVEL_OVER_LOGE = 1,
	AE_LOG_LEVEL_OVER_LOGW = 2,
	AE_LOG_LEVEL_OVER_LOGI = 3,
	AE_LOG_LEVEL_OVER_LOGD = 4,
	AE_LOG_LEVEL_OVER_LOGV = 5
};

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#ifndef CONDITION
#define CONDITION(cond) (__builtin_expect((cond)!=0, 0))
#endif

// ---------------------------------------------------------------------
#ifndef android_printLog
#define android_printLog(prio, tag, fmt...) \
__android_log_print(prio, tag, fmt)
#endif

/*
* Log macro that allows you to specify a number for the priority.
*/
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
android_printLog(priority, tag, __VA_ARGS__)
#endif

// ---------------------------------------------------------------------
#ifndef ALOG
#define ALOG(priority, tag, ...) \
LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

/*
* Simplified macro to send a verbose log message using the current LOG_TAG.
*/	
#ifndef ALOGV_IF
#define ALOGV_IF(cond, ...) \
( (CONDITION(cond)) \
? ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
: (void)0 )
#endif

/*
* Simplified macro to send a debug log message using the current LOG_TAG.
*/
#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) \
( (CONDITION(cond)) \
? ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
: (void)0 )
#endif

/*
* Simplified macro to send an info log message using the current LOG_TAG.
*/	
#ifndef ALOGI_IF
#define ALOGI_IF(cond, ...) \
( (CONDITION(cond)) \
? ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
: (void)0 )
#endif

/*
* Simplified macro to send a warning log message using the current LOG_TAG.
*/	
#ifndef ALOGW_IF
#define ALOGW_IF(cond, ...) \
( (CONDITION(cond)) \
? ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
: (void)0 )
#endif

/*
* Simplified macro to send an error log message using the current LOG_TAG.
*/	
#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
( (CONDITION(cond)) \
? ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
: (void)0 )
#endif	

#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
( (CONDITION(cond)) \
? ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
: (void)0 )
#endif

/*android*/
extern volatile uint32_t gAELogLevel;
#define AE_LOG(fmt, args...) ALOGE("%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGE(fmt, args...)  ALOGE("%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGW(fmt, args...) ALOGW_IF(gAELogLevel >= AE_LOG_LEVEL_OVER_LOGW, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGI(fmt, args...) ALOGI_IF(gAELogLevel >= AE_LOG_LEVEL_OVER_LOGI, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGD(fmt, args...) ALOGD_IF(gAELogLevel >= AE_LOG_LEVEL_OVER_LOGD, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGV(fmt, args...) ALOGV_IF(gAELogLevel >= AE_LOG_LEVEL_OVER_LOGV, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#else
#define AE_LOG printf
#define AE_LOGI printf
#define AE_LOGE printf
#define AE_LOGV printf
#define AE_LOGD printf
#define ALOGE printf
#define AE_LOGX printf
#endif

#ifndef AE_TRAC
#define AE_TRAC(_x_) AE_LOGE _x_
#endif

#ifndef AE_RETURN_IF_FAIL
#define AE_RETURN_IF_FAIL(exp,warning) do{if(exp) {AE_TRAC(warning); return exp;}}while(0)
#endif

#ifndef AE_TRACE_IF_FAIL
#define AE_TRACE_IF_FAIL(exp,warning) do{if(exp) {AE_TRAC(warning);}}while(0)
#endif
/**---------------------------------------------------------------------------*
**				Data Prototype				*
**----------------------------------------------------------------------------*/

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
