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
#include <android/log.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "cam_log.h"

#ifdef __cplusplus
extern "C" {
#endif
const int LOG_STATUS_EN = '0';
const int LOG_STATUS_DIS = '1';
const int LOG_PRI_E = '1';
const int LOG_PRI_W = '2';
const int LOG_PRI_I = '3';
const int LOG_PRI_D = '4';
const int LOG_PRI_V = '5';

char g_log_level[LOG_MODULE_MAX] = {'4', '4', '4', '4', '4', '4'};
char g_log_module_setting[LOG_MODULE_MAX] = {'0', '0', '0', '0', '0', '0'};
char g_log_func_setting[LOG_FUNC_MAX] = {'0', '0', '0', '0'};


int __print_log(int pri, int module, int func,
                      const char* tag, const char* format, ...){
    int prio=0;
    if((g_log_level[module] > LOG_PRI_V) ||
        (g_log_level[module] < LOG_PRI_E)) {
             g_log_level[module] = LOG_PRI_D;
    }
    if((int)g_log_module_setting[module] == LOG_STATUS_EN) {
       if((int)g_log_func_setting[func] == LOG_STATUS_EN) {
          //update g_log_level according to module priority
         if((int)g_log_level[module] >= pri){
             va_list ap;
             char buf[1024];
             va_start(ap, format);
             vsnprintf(buf, 1024, format, ap);
             va_end(ap);
             switch(pri) {
             case LOG_PRI_E: prio=ANDROID_LOG_ERROR; break;
             case LOG_PRI_W: prio=ANDROID_LOG_WARN; break;
             case LOG_PRI_I: prio=ANDROID_LOG_INFO; break;
             case LOG_PRI_D: prio=ANDROID_LOG_DEBUG; break;
             case LOG_PRI_V: prio=ANDROID_LOG_VERBOSE; break;
             default:prio=ANDROID_LOG_DEBUG;
          }
          return __android_log_write(prio, tag, buf);
       }
    }
  }
  return 0;
}
#ifdef __cplusplus
 }
#endif

