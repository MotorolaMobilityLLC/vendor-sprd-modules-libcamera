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

uint32_t g_log_level = (LOG_PRI_D<<20)|(LOG_PRI_D<<16)|(LOG_PRI_D<<12)
  						|(LOG_PRI_D<<8)|(LOG_PRI_D<<4)|(LOG_PRI_D<<0);
uint32_t g_log_module_setting = 0;
uint32_t g_log_func_setting = 0;

int __print_log(int pri, int module, int func,
  						const char* tag, const char* format, ...){
  	int prio=0;
  	int prio_threshold=0;
  	if(!((g_log_module_setting>>module)&0x1))
  	{
  		if(!((g_log_func_setting>>func)&0x1))
  		{
  			prio_threshold=(int)((g_log_level>>(4*module))&0xf);
  			if(prio_threshold >= LOG_PRI_MAX)
  			{
  				prio_threshold = LOG_PRI_D;
  			}
  			//update g_log_level according to module priority
  			if(prio_threshold >= pri){
  				va_list ap;
  				char buf[1024];
  				va_start(ap, format);
      			vsnprintf(buf, 1024, format, ap);
      			va_end(ap);
  				switch(pri)
  				{
  				case LOG_PRI_E:
  					prio=ANDROID_LOG_ERROR;
  					break;
  				case LOG_PRI_W:
  					prio=ANDROID_LOG_WARN;
  					break;
  				case LOG_PRI_I:
  					prio=ANDROID_LOG_INFO;
  					break;
				case LOG_PRI_D:
  					prio=ANDROID_LOG_DEBUG;
  					break;
  				case LOG_PRI_V:
  					prio=ANDROID_LOG_VERBOSE;
  					break;
  				default:
  					prio=ANDROID_LOG_DEBUG;
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

