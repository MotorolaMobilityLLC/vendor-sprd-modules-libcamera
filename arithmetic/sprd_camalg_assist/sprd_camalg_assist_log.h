#ifndef __SPRD_VDSP_CMD_LOG_H__
#define __SPRD_VDSP_CMD_LOG_H__

enum {
	LEVEL_OVER_LOGE = 1,
	LEVEL_OVER_LOGW,
	LEVEL_OVER_LOGI,
	LEVEL_OVER_LOGD,
	LEVEL_OVER_LOGV
};

#define DEBUG_STR "%d, %s: "
#define ERROR_STR "%d, %s: sprd_camalg_vdsp_cmd error "
#define DEBUG_ARGS __LINE__, __FUNCTION__

long g_vdsp_cmd_log_level = 5;

#define VDSP_CMD_LOGE(format, ...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define VDSP_CMD_LOGW(format, ...)                                                  \
    ALOGW_IF(g_vdsp_cmd_log_level >= LEVEL_OVER_LOGW, DEBUG_STR format, DEBUG_ARGS,##__VA_ARGS__)

#define VDSP_CMD_LOGI(format, ...)                                                  \
    ALOGI_IF(g_vdsp_cmd_log_level >= LEVEL_OVER_LOGI, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)

#define VDSP_CMD_LOGD(format, ...)                                                  \
    ALOGD_IF(g_vdsp_cmd_log_level >= LEVEL_OVER_LOGD, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)

/* ISP_LOGV uses ALOGD_IF */
#define VDSP_CMD_LOGV(format, ...)                                                  \
    ALOGD_IF(g_vdsp_cmd_log_level >= LEVEL_OVER_LOGV, DEBUG_STR format, DEBUG_ARGS, \
             ##__VA_ARGS__)
#endif
