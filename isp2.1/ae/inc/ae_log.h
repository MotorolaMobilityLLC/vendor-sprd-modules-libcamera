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
#ifndef CONFIG_FOR_TIZEN
#include <utils/Log.h>
#else
#include "osal_log.h"
#endif
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

/*android*/
extern cmr_u32 g_ae_log_level;
#define AE_LOG(fmt, args...) ALOGE("%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGE(fmt, args...)  ALOGE("%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGW(fmt, args...) ALOGW_IF(g_ae_log_level >= AE_LOG_LEVEL_OVER_LOGW, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGI(fmt, args...) ALOGI_IF(g_ae_log_level >= AE_LOG_LEVEL_OVER_LOGI, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGD(fmt, args...) ALOGD_IF(g_ae_log_level >= AE_LOG_LEVEL_OVER_LOGD, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#define AE_LOGV(fmt, args...) ALOGV_IF(g_ae_log_level >= AE_LOG_LEVEL_OVER_LOGV, "%d, %s: " fmt, __LINE__, __FUNCTION__, ##args)
#else
#define AE_LOG printf
#define AE_LOGI printf
#define AE_LOGE printf
#define AE_LOGV printf
#define AE_LOGD printf
#define ALOGE printf
#define AE_LOGX printf
#endif

#define AE_TRAC(_x_) AE_LOGE _x_
#define AE_RETURN_IF_FAIL(exp,warning) do{if(exp) {AE_TRAC(warning); return exp;}}while(0)
#define AE_TRACE_IF_FAIL(exp,warning) do{if(exp) {AE_TRAC(warning);}}while(0)

void ae_init_log_level(void);
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
