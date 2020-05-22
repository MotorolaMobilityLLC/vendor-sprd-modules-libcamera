#ifndef __SPRD_WT_ADAPTER_LOG_H__
#define __SPRD_WT_ADAPTER_LOG_H__

#define LOG_TAG		"sprd_wt_adapter"

#include <utils/Log.h>

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define WT_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define WT_LOGW(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define WT_LOGI(format,...) ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define WT_LOGD(format,...) ALOGD(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#endif
