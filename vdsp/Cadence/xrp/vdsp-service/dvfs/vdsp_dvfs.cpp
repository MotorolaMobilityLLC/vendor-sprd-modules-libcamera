#include <utils/Mutex.h>
#include <utils/List.h>
#include <android/log.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "vdsp_dvfs.h"
#include "xrp_host_common.h"
#include "xrp_kernel_defs.h"

#define TAG_DVFS  "vdsp_dvfs"
#define DVFS_MONITOR_CYCLE_TIME   (1000*1000)
using namespace android;
enum dvfs_enum_index {
	DVFS_INDEX_0 = 0,
	DVFS_INDEX_1,
	DVFS_INDEX_2,
	DVFS_INDEX_3,
	DVFS_INDEX_4,
	DVFS_INDEX_5,
	DVFS_INDEX_MAX,
};
static Mutex timepiece_lock;
static Mutex powerhint_lock;
static uint32_t g_workingcount;
static pthread_t g_monitor_threadid;
static struct vdsp_work_piece *g_currentpiece;
static uint32_t g_monitor_exit;
static enum sprd_vdsp_power_level g_freqlevel;
static uint32_t g_tempset;
List<struct vdsp_work_piece*> g_workpieces_list;
static void *dvfs_monitor_thread(void* data);

int32_t set_dvfs_maxminfreq(void *device , int32_t maxminflag)
{
	struct xrp_dvfs_ctrl dvfs;
	struct xrp_device *dev = (struct xrp_device *)device;
	dvfs.en_ctl_flag = 0;
	dvfs.enable = 1;
	if(maxminflag)
		dvfs.index = DVFS_INDEX_5;
	else
		dvfs.index = DVFS_INDEX_0;
	ioctl(dev->impl.fd ,XRP_IOCTL_SET_DVFS , &dvfs);
	return 0;
}

int32_t init_dvfs(void* device)
{
	int ret;
	struct xrp_dvfs_ctrl dvfs;
	struct xrp_device *dev = (struct xrp_device *)device;
	g_monitor_exit = 0;
	g_freqlevel = SPRD_VDSP_POWERHINT_NORMAL;
	g_tempset = 0;
	dvfs.en_ctl_flag = 1;
	dvfs.enable = 1;
	dvfs.index = DVFS_INDEX_5;
	ioctl(dev->impl.fd ,XRP_IOCTL_SET_DVFS , &dvfs);
	ret = pthread_create(&g_monitor_threadid , NULL , dvfs_monitor_thread , device);
	if(0 == ret)
		return 1;/*1 is ok*/
	else
		return 0;/*0 is error*/
}
void deinit_dvfs(void *device)
{
	struct xrp_device *dev = (struct xrp_device *)device;
	struct xrp_dvfs_ctrl dvfs;
	List<struct vdsp_work_piece*>::iterator iter;
	/*remove all work pieces list*/
	g_monitor_exit = 1;
	pthread_join(g_monitor_threadid , NULL);
	dvfs.en_ctl_flag = 1;
	dvfs.enable = 0;
	ioctl(dev->impl.fd ,XRP_IOCTL_SET_DVFS , &dvfs);
	for(iter = g_workpieces_list.begin(); iter != g_workpieces_list.end(); iter++) {
		delete *iter;
        }
	g_workpieces_list.clear();
}
static struct vdsp_work_piece* alloc_work_piece()
{
	struct vdsp_work_piece *piece = new struct vdsp_work_piece;//malloc(sizeof(struct vdsp_work_piece));
	if(piece) {
		piece->start_time = 0;
		piece->end_time = 0;
		piece->working_count = 0;
	}
	return piece;
}

void preprocess_work_piece()
{
	struct vdsp_work_piece* piece;
	AutoMutex _l(timepiece_lock);
	if(0 == g_workingcount) {
		piece = alloc_work_piece();
		g_currentpiece = piece;
		g_currentpiece->start_time = systemTime(CLOCK_MONOTONIC);
		g_currentpiece->working_count ++;
		g_workingcount ++;
		/*insert piece to piecelist*/
		g_workpieces_list.push_back(piece);
	}
	else {
		g_workingcount ++;
		g_currentpiece->working_count ++;
	}
}
void postprocess_work_piece()
{
	AutoMutex _l(timepiece_lock);
	g_workingcount --;
	if(0 == g_workingcount) {
		g_currentpiece->end_time = systemTime(CLOCK_MONOTONIC);
	}
}
int32_t set_powerhint_flag(void *device , enum sprd_vdsp_power_level level , uint32_t permanent)
{
	struct xrp_dvfs_ctrl dvfs;
	int ret;
	struct xrp_device *dev = (struct xrp_device *)device;
	powerhint_lock.lock();
	if(permanent) {
		g_freqlevel = level;
	} else {
		g_tempset = 1;
	}
	__android_log_print(ANDROID_LOG_DEBUG,TAG_DVFS ,"%s permant:%d , level:%d\n" , __func__ , permanent , level);
	/*set power hint*/
	dvfs.index = level;
	dvfs.en_ctl_flag = 0;
	ret = ioctl(dev->impl.fd , XRP_IOCTL_SET_DVFS , &dvfs);
	powerhint_lock.unlock();
	return ret;
}
static uint32_t calculate_vdsp_usage(int64_t fromtime , int64_t endtime)
{
	List<struct vdsp_work_piece*>::iterator iter;
	int64_t costtime = 0;
	int64_t small_endtime;
	uint32_t count = 0;
	uint32_t removecount = 0;
	uint32_t notinrange = 0;
#if 0
	timepiece_lock.lock();
	timepiece_lock.unlock();
#endif
	/**/
	for(iter = g_workpieces_list.begin(); iter != g_workpieces_list.end(); iter++) {
		if(((*iter)->start_time <= fromtime) && ((*iter)->end_time) > fromtime) {
			costtime += ((*iter)->end_time - fromtime);
			count++;
		} else if(((*iter)->start_time >= fromtime) && ((*iter)->start_time < endtime)) {
			if((*iter)->end_time != 0) {
				small_endtime = (*iter)->end_time > endtime ? endtime : (*iter)->end_time;
				costtime += (small_endtime - (*iter)->start_time);
			}else {
				small_endtime = endtime;
				costtime += (endtime - (*iter)->start_time);
			}
			count++;
		} else {
			notinrange ++;
			/*time piece not between fromtime and endtime*/
			continue;
		}
	}
	/*lock here?*/
	timepiece_lock.lock();
	/*after this cycle total cost time is completed, remove the unused time pieces*/
	for(iter = g_workpieces_list.begin(); iter != g_workpieces_list.end();) {
		/*if time piece is older than fromtime to endtime erase and continue*/
		if(!(((*iter)->start_time >= endtime) || ((*iter)->end_time > endtime ) || (0 == (*iter)->end_time))) {
			delete *iter;
			iter = g_workpieces_list.erase(iter);
			removecount++;
			continue;
		}
		break;
	}
	timepiece_lock.unlock();
//	__android_log_print(ANDROID_LOG_DEBUG,TAG_DVFS ,"%s count:%d, notinrange:%d , removecount:%d, costtime:%ld, end-from:%ld\n" ,
//				__func__ , count, notinrange , removecount , costtime , (endtime-fromtime));
	return (costtime*100) / (endtime - fromtime);
}
static uint32_t calculate_dvfs_index(uint32_t percent)
{
	static uint32_t last_percent = 0;
	last_percent = percent;
	if((last_percent > 50) && (percent > 50)) {
		return DVFS_INDEX_5;
	} else if(((percent <= 50) && (percent > 20)) && (last_percent <= 50)) {
		return DVFS_INDEX_3;
	} else {
		return DVFS_INDEX_0;
	}
}
static void *dvfs_monitor_thread(__unused void* data)
{
	uint32_t percentage;
	int64_t start_time = systemTime(CLOCK_MONOTONIC);
	struct xrp_dvfs_ctrl dvfs;
	dvfs.en_ctl_flag = 0;
	struct xrp_device *device = (struct xrp_device *)data;
	while(1) {
		if(0 != g_monitor_exit) {
			__android_log_print(ANDROID_LOG_DEBUG,TAG_DVFS ,"%s exit\n" , __func__);
			break;
		}
		powerhint_lock.lock();
		if(1 == g_tempset) {
			g_tempset = 0;
			if(g_freqlevel != SPRD_VDSP_POWERHINT_NORMAL) {
				/*set freq to g_freqlevel*/
				dvfs.index = g_freqlevel;
				dvfs.en_ctl_flag = 0;
				 __android_log_print(ANDROID_LOG_DEBUG,TAG_DVFS ,"%s before set dvfs index:%d\n" , __func__ , g_freqlevel);
				ioctl(device->impl.fd , XRP_IOCTL_SET_DVFS , &dvfs);
			}
		}
		if(SPRD_VDSP_POWERHINT_NORMAL == g_freqlevel) {
			percentage = calculate_vdsp_usage(start_time  , systemTime(CLOCK_MONOTONIC));
			__android_log_print(ANDROID_LOG_DEBUG,TAG_DVFS ,"%s percentage:%d\n" , __func__ , percentage);
			start_time = systemTime(CLOCK_MONOTONIC);
			/*dvfs set freq*/
			dvfs.index = calculate_dvfs_index(percentage);
			ioctl(device->impl.fd , XRP_IOCTL_SET_DVFS , &dvfs);
		}
		powerhint_lock.unlock();
		usleep(DVFS_MONITOR_CYCLE_TIME);
	}
	return NULL;
}
