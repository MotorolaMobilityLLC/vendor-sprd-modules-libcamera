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
#ifndef _CAM_LOG_H_
#define _CAM_LOG_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif




typedef enum{
    LOG_HAL = 0,
    LOG_OEM,
    LOG_ISP_MW,
    LOG_SENSOR,
    LOG_ISPALG,
    LOG_ARITH,
    LOG_MODULE_MAX,
}LOG_MODULE_E;

typedef enum{
    LOG_ALL = 0,
    LOG_AE,
    LOG_AF,
    LOG_AWBC,
    LOG_FUNC_MAX
}LOG_FUNC_E;

extern const int LOG_STATUS_EN;
extern const int LOG_STATUS_DIS;
extern const int LOG_PRI_E;
extern const int LOG_PRI_W;
extern const int LOG_PRI_I;
extern const int LOG_PRI_D;
extern const int LOG_PRI_V;
extern char g_log_level[LOG_MODULE_MAX];
extern char g_log_module_setting[LOG_MODULE_MAX];
extern char g_log_func_setting[LOG_FUNC_MAX];



int __print_log(int pri, int module, int func,
                     const char* tag, const char* format, ...);

#ifndef LOG
#define LOG(prio, module, func,format,...) \
               ((void)PRI_LOG(prio, module, func,LOG_TAG, \
                 "%d, %s: " format,__LINE__, __FUNCTION__,##__VA_ARGS__))
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(prio,module,func,format,args...)  \
              LOG_T(prio, module, func, "%d, %s: " format,__LINE__, __FUNCTION__, ##args)
#define LOG_T(prio, module, func,...) \
	          ((void)PRI_LOG(prio, module, func,LOG_TAG, ##__VA_ARGS__))
#endif

#ifndef PRI_LOG
#define PRI_LOG(prio, module, func,tag, format...) \
	    __print_log(prio,module,func,tag,format)
#endif
#ifdef __cplusplus
 }
#endif
#endif

