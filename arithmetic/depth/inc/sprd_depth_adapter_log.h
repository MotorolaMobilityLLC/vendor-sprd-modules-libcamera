#ifndef __SPRD_DEPTH_ADAPTER_LOG_H__
#define __SPRD_DEPTH_ADAPTER_LOG_H__

#include <utils/Log.h>

#define LOG_TAG		"sprd_depth_adapter"

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define DEPTH_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define DEPTH_LOGW(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define DEPTH_LOGI(format,...) ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define DEPTH_LOGD(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#endif
