#ifndef __SPRD_CAMALG_ASSIST_LOG_H__
#define __SPRD_CAMALG_ASSIST_LOG_H__

#include <log/log.h>
#include <time.h>
#include <sys/time.h>

enum {
	LEVEL_OVER_LOGE = 1,
	LEVEL_OVER_LOGW,
	LEVEL_OVER_LOGI,
	LEVEL_OVER_LOGD,
	LEVEL_OVER_LOGV
};

#define LOG_TAG "sprd_caa"

#define LOG_LEVEL	5

#define CAA_LOGE(format, ...) ALOGE(format, ##__VA_ARGS__)
#define CAA_LOGI(format, ...) ALOGI_IF(LOG_LEVEL >= LEVEL_OVER_LOGI, format, ##__VA_ARGS__)
#define CAA_LOGD(format, ...) ALOGD_IF(LOG_LEVEL >= LEVEL_OVER_LOGD, format, ##__VA_ARGS__)

#define VDSP_CMD_LOGE CAA_LOGE
#define VDSP_CMD_LOGI CAA_LOGI
#define VDSP_CMD_LOGD CAA_LOGD

inline double getTime()
{
	double cur_t;
	struct timespec t;
	long long cur_ns;
	t.tv_sec = t.tv_nsec = 0;
	clock_gettime(CLOCK_MONOTONIC, &t);
	cur_ns = (long long)t.tv_sec * 1e9 + (long long)t.tv_nsec;
	cur_t = (double)cur_ns / 1e6;
	return cur_t;
}

#endif
