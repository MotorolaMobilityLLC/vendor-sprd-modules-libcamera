#ifndef __FB_LOG_H__
#define __FB_LOG_H__
//#include <stdio.h>
//#include <stdlib.h>
//#include <cutils/log.h>
//#define LOG_TAG "SW_ISP"
#define DEBUG_STR     "%d, %s: "
//#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define SWISP_LOGE(format,...) \
	ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define SWISP_LOGW(format,...) \
        ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define SWISP_LOGI(format,...) \
	ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define SWISP_LOGD(format,...) \
	ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#endif
