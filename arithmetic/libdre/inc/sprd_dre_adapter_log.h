#ifndef __SPRD_DRE_ADAPTER_LOG_H__
#define __SPRD_DRE_ADAPTER_LOG_H__

#include <utils/Log.h>

#define LOG_TAG     "sprd_dre_adapter"

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define DRE_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define DRE_LOGI(format,...) ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define DRE_LOGW(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#endif