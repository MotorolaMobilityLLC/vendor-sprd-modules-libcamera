#ifndef _SPRD_VDSP_DVFS_H_
#define _SPRD_VDSP_DVFS_H_
#include "vdsp_interface.h"
struct vdsp_work_piece
{
	int64_t start_time;
	int64_t end_time;
	uint32_t working_count;
};

#ifdef __cplusplus
extern "C" {
#endif
int32_t set_dvfs_maxminfreq(void *device , int32_t maxminflag);
int32_t init_dvfs(void *device);
void deinit_dvfs(void *device);
void preprocess_work_piece();
void postprocess_work_piece();
int32_t set_powerhint_flag(void* device , enum sprd_vdsp_power_level level , uint32_t permanent);
#ifdef __cplusplus
}
#endif

#endif

