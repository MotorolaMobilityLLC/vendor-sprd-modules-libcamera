
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "IVdspService.h"
#include "vdsp_interface.h"
#include <cutils/properties.h>
#include "xrp_interface.h"


#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG  TAG_Client
#endif

using namespace android;
#define USE_FD_MAX_NUM  32
#define CHECK_SUM_PROPERTY   "persist.vendor.vdsp.checksum"
struct fd_info
{
	Mutex fd_mutex;
	int32_t working;
};
static int32_t gInitFlag = 0;
static uint32_t gGeneration = 0;
static Mutex gLock;
static sp<IVdspService> gVdspService;
static sp<IBinder> gfakeBinder;
static int32_t gOpenedCount = 0;
static enum sprd_vdsp_worktype gWorkType = SPRD_VDSP_WORK_MAXTYPE;
static int32_t gWorking = 0;
static struct fd_info g_FdInfo[USE_FD_MAX_NUM];
static uint32_t g_fdtable = 0;
class VdspClientDeathRecipient: public IBinder::DeathRecipient
{
public:
        VdspClientDeathRecipient(){}
        // IBinder::DeathRecipient
        virtual void binderDied(const wp<IBinder> &/*who*/)
	{
		AutoMutex _l(gLock);
		gInitFlag = 0;
		gGeneration ++;
		ALOGD("binderDied func:%s , gGeneration:%d\n" , __func__ , gGeneration);
	}
};
static sp<VdspClientDeathRecipient> gDeathRecipient = NULL;
static int32_t check_sum_flag()
{
	char value[128];
	ALOGD("func:%s" , __func__);
	property_get(CHECK_SUM_PROPERTY , value , "");
	if(!strcmp(value ,  "1"))
		return 1;
	else
		return 0;
}
static void print_checksum_result(const char *nsid , const char *info , struct VdspInputOutput* buffer , uint32_t num)
{
	uint32_t i , j;
	uint64_t *result;
	uint8_t *temp;
	uint64_t tempresult = 0;
	if(buffer == NULL)
		return;
	result = (uint64_t*)malloc(num*sizeof(uint64_t));
	for(i = 0; i < num; i++){
		for(j =0; j < buffer[i].size; j++) {
			temp = (uint8_t*)buffer[i].viraddr;
			if(j == 0)
				tempresult = ((uint64_t)temp[j] + (uint64_t)temp[j+1]);
			else
				tempresult = ((uint64_t)tempresult + (uint64_t)temp[j+1]);
		}
		result[i] = tempresult;
	}
	for(i = 0; i < num; i++) {
		ALOGD("func:%s nsid:%s %s index:%d , sum:%llx" , __func__ , nsid , info , i , (unsigned long long)result[i]);
	}
	free(result);
}
static int32_t Alloc_Fd()
{
        uint32_t valid_bits = (g_fdtable ^ 0xffffffff);
        for(int i =0; i < USE_FD_MAX_NUM; i++) {
                if(((valid_bits >> i) & 0x1)) {
			g_fdtable |= (1<<i);
                        return i;
                }
        }
        return -1;
}
static int32_t Free_Fd(int32_t fd)
{
        if(fd < 0)
                return -1;
        /*check whether fd is valid*/
        if((g_fdtable & (1<<fd)) == 0) {
                printf("func:%s , free fd:%d , is not valid, g_fdtable:%x\n" , __func__ , fd , g_fdtable);
                return -1;
        }
        g_fdtable &= ~(1<<fd);
        return 0;
}
static int32_t Check_FdValid(int32_t fd) {
        if(fd < 0)
                return -1;
        if((g_fdtable & (1<<fd)) == 0) {
                printf("func:%s , fd:%d , is not valid, g_fdtable:%x\n" , __func__ , fd , g_fdtable);
                return -1;
        }
        return 0;
}
static int32_t Check_GenrationValid(uint32_t generation)
{
	if(generation == gGeneration)
	{
		return 0;
	}
	return -1;
}
static sp<IVdspService> getVdspService()
{
	AutoMutex _l(gLock);
	int ret;
	ALOGD("func:%s  , gInitFlag:%d\n" , __func__ , gInitFlag);
	if(0 == gInitFlag)
	{
		sp < IServiceManager > sm = defaultServiceManager();
		sp < IBinder > binder = sm->getService(String16("service.vdspservice"));
		if(binder != NULL) {
			g_fdtable = 0;
			gWorkType = SPRD_VDSP_WORK_MAXTYPE;
			for(int i = 0; i< USE_FD_MAX_NUM; i++)
			{
				g_FdInfo[i].working = 0;
			}
			if(gDeathRecipient == NULL) {
				gDeathRecipient = new VdspClientDeathRecipient();
			}
			ret = binder->linkToDeath(gDeathRecipient);
			gVdspService = interface_cast<IVdspService>(binder);
			ALOGD("func:%s  , gDeathRecipient:%p , linkTodeath:%d\n" , __func__ , gDeathRecipient.get() ,ret);
			gInitFlag = 1;
			gOpenedCount = 0;
		}
	}
	ALOGD("func:%s  , gInitFlag:%d , xrpService:%p\n" , __func__ , gInitFlag , gVdspService.get());
	return gVdspService;
}

__attribute__ ((visibility("default"))) enum sprd_vdsp_result sprd_cavdsp_open_device(enum sprd_vdsp_worktype type , struct vdsp_handle *handle)
{
	sp<IVdspService> cs = NULL;
	enum sprd_vdsp_result result;
	int32_t fd = -1;
	cs = getVdspService();
	if((handle == NULL) || (cs == NULL))
	{
		 ALOGE("func:%s get resource failed cs:%p , handle:%p\n" , __func__ ,cs.get() , handle);
		return SPRD_VDSP_RESULT_FAIL;
	}
	printf("func:%s enter cs:%p\n" , __func__ , cs.get());
	ALOGD("func:%s enter cs:%p\n" , __func__ ,cs.get());
	{
		AutoMutex _l(gLock);
		handle->fd = -1;
		handle->generation = 0xffffffff;
		fd = Alloc_Fd();
		if(fd < 0)
		{
			 ALOGE("func:%s Alloc_Fd faild fd:%d\n" , __func__ , fd);
			goto __openerror;;
		}
		handle->fd = fd;
		handle->generation = gGeneration;
		if(0 == gOpenedCount)
		{
			result = (enum sprd_vdsp_result)cs->openXrpDevice(gfakeBinder , type);
			if(result == SPRD_VDSP_RESULT_SUCCESS)
			{
				gOpenedCount ++;
				gWorkType = type;
				ALOGD("func:%s gOpenedCount 0 fd:%d\n" , __func__ , fd);
				goto __openok;
			}
			else {
				ALOGE("func:%s openXrpDevice failed fd:%d\n" , __func__ , fd);
				goto __openerror;
			}
		} else {
			if(type == gWorkType) {
				gOpenedCount++;
			} else {
				ALOGE("func:%s openXrpDevice fail type:%d, gWorkType:%d\n" , __func__ , type , gWorkType);
				Free_Fd(fd);
				goto __openerror;
			}
		}
		ALOGD("func:%s fd:%d\n" , __func__ , fd);
			
		goto __openok;
		//return (enum sprd_vdsp_result) cs->openXrpDevice(gfakeBinder);
	}
__openerror:
	if(fd > 0)
		Free_Fd(fd);
	return SPRD_VDSP_RESULT_FAIL;
__openok:
	return SPRD_VDSP_RESULT_SUCCESS;
}

__attribute__ ((visibility("default"))) enum sprd_vdsp_result sprd_cavdsp_close_device(void *handle)
{
	sp<IVdspService> cs = NULL;
	enum sprd_vdsp_result result;
	uint32_t generation;
	int32_t fd;
	struct vdsp_handle *hnd = (struct vdsp_handle*)handle;
	if(handle == NULL)
	{
		/*fd is abnormal value*/
		 ALOGE("func:%s close invalid input hadle is NULL\n" , __func__);
		return SPRD_VDSP_RESULT_FAIL;
	}
	generation = hnd->generation;
	fd = hnd->fd;
	cs = getVdspService();
	if(cs != NULL)
	{
		AutoMutex _l(gLock);
		if(0 != Check_GenrationValid(generation))
		{
			ALOGE("func:%s Check_GenrationValid failed generation:%d , gGeneration:%d\n" , __func__ ,
				generation , gGeneration);
			gLock.unlock();
			return SPRD_VDSP_RESULT_OLD_GENERATION;
		}
		if(0 != Check_FdValid(fd))
		{
			ALOGE("func:%s Check_FdValid failed fd:%d\n" , __func__ , fd);
			return SPRD_VDSP_RESULT_FAIL;
		}
		gOpenedCount--;
		if(gOpenedCount == 0)
		{
			if(gWorking != 0)
			{
				ALOGE("func:%s gWorking is not zerofd:%d\n" , __func__ , fd);
				return SPRD_VDSP_RESULT_FAIL;
			}
			result = (enum sprd_vdsp_result)cs->closeXrpDevice(gfakeBinder);
			if(result == SPRD_VDSP_RESULT_SUCCESS)
			{
				Free_Fd(fd);
				ALOGD("func:%s closeXrpDevice ok fd:%d\n" , __func__ , fd);
				gWorkType = SPRD_VDSP_WORK_MAXTYPE;
				return SPRD_VDSP_RESULT_SUCCESS;
			}
			else {
				ALOGE("func:%s closeXrpDevice failed fd:%d\n" , __func__ , fd);
				return SPRD_VDSP_RESULT_FAIL;
			}
		}
		if(g_FdInfo[fd].working == 0){
			Free_Fd(fd);
			ALOGD("func:%s freefd ok fd:%d\n" , __func__ , fd);
			return SPRD_VDSP_RESULT_SUCCESS;
		}
		else {
			ALOGE(TAG_Client,"func:%s freefd faild working fd:%d\n" , __func__ , fd);
			return SPRD_VDSP_RESULT_FAIL;
		}
	}
	return SPRD_VDSP_RESULT_FAIL;
}

__attribute__ ((visibility("default"))) enum sprd_vdsp_result sprd_cavdsp_send_cmd(void *handle , const char *nsid , struct sprd_vdsp_client_inout *in, struct sprd_vdsp_client_inout *out ,
                                                struct sprd_vdsp_client_inout *buffer , uint32_t bufnum , uint32_t priority)
{
	sp<IVdspService> cs = NULL;
	enum sprd_vdsp_result result;
	uint32_t generation;
	uint32_t fd;
	struct vdsp_handle *hnd = (struct vdsp_handle*) handle;
	cs = getVdspService();
	if((cs == NULL) || (NULL == handle))
	{
		/*fd is abnormal value*/
                 ALOGE(TAG_Client,"func:%s err param cs:%p , handle:%p\n" , __func__ , cs.get() , handle);
                return SPRD_VDSP_RESULT_FAIL;
	}
	generation = hnd->generation;
	fd = hnd->fd;
	//if(cs != NULL)
	{
		gLock.lock();
		if(0 != Check_GenrationValid(generation))
		{
			ALOGE(TAG_Client,"func:%s Check_GenrationValid failed generation:%d , gGeneration:%d\n" , __func__ ,
				generation , gGeneration);
			gLock.unlock();
			return SPRD_VDSP_RESULT_OLD_GENERATION;
		}
		if(0 != Check_FdValid(fd))
                {
			ALOGE("func:%s Check_FdValid failed fd:%d\n" , __func__ , fd);
			gLock.unlock();
                        return SPRD_VDSP_RESULT_FAIL;
                }
		if(gOpenedCount == 0)
		{
			gLock.unlock();
			ALOGE("func:%s gOpenedCount 0 failed fd:%d\n" , __func__ , fd);
			return SPRD_VDSP_RESULT_FAIL;
		}
		g_FdInfo[fd].working++;
		gWorking++;
		gLock.unlock();
		if(check_sum_flag()) {
			print_checksum_result(nsid , "buffer" , (struct VdspInputOutput*)buffer , bufnum);
			print_checksum_result(nsid , "input" , (struct VdspInputOutput*)in , 1);
			print_checksum_result(nsid , "output" , (struct VdspInputOutput*)out , 1);
		}
		result = (enum sprd_vdsp_result)cs->sendXrpCommand(gfakeBinder , nsid ,(struct VdspInputOutput*)in, (struct VdspInputOutput*)out , (struct VdspInputOutput*)buffer,
									bufnum , priority);
		gLock.lock();
		gWorking --;
		g_FdInfo[fd].working --;
		gLock.unlock();
		if(0 == result) {
			ALOGD("func:%s  ok fd:%d result:%d\n" , __func__ , fd , result);
			return SPRD_VDSP_RESULT_SUCCESS;
		} else {
			ALOGE("func:%s  err fd:%d result:%d\n" , __func__ , fd , result);
			return SPRD_VDSP_RESULT_FAIL;
		}
	}
}

__attribute__ ((visibility("default"))) enum sprd_vdsp_result sprd_cavdsp_loadlibrary(void *handle , const char *libname , struct sprd_vdsp_client_inout *buffer)
{
	sp<IVdspService> cs = NULL;
	enum sprd_vdsp_result result;
	uint32_t generation;
	int32_t fd;
	struct vdsp_handle *hnd = (struct vdsp_handle*) handle;
	cs = getVdspService();
	if((cs == NULL) || (NULL == handle))
	{
		/*fd is abnormal value*/
		ALOGE("func:%s err param cs:%p , handle:%p\n" , __func__ , cs.get() , handle);
		return SPRD_VDSP_RESULT_FAIL;
	}
	fd = hnd->fd;
	generation = hnd->generation;
	//if(cs != NULL)
	{
		gLock.lock();
		if(0 != Check_GenrationValid(generation))
		{
			ALOGE("func:%s Check_GenrationValid failed generation:%d , gGeneration:%d\n" , __func__ ,
				generation , gGeneration);
			gLock.unlock();
			return SPRD_VDSP_RESULT_OLD_GENERATION;
		}
		if(0 != Check_FdValid(fd))
                {
			ALOGE("func:%s Check_FdValid failed fd:%d\n" , __func__ , fd);
			gLock.unlock();
                        return SPRD_VDSP_RESULT_FAIL;
                }
		if(gOpenedCount == 0)
		{
			gLock.unlock();
			ALOGE("func:%s gOpenedCount 0 failed fd:%d\n" , __func__ , fd);
			return SPRD_VDSP_RESULT_FAIL;
		}
		gWorking++;
		g_FdInfo[fd].working++;
		gLock.unlock();
		result = (enum sprd_vdsp_result)cs->loadXrpLibrary(gfakeBinder , libname , (struct VdspInputOutput*)buffer);
		gLock.lock();
		gWorking --;
		g_FdInfo[fd].working --;
		gLock.unlock();
		if( 0 == result) {
			ALOGD("func:%s ok fd:%d result:%d\n" , __func__ , fd , result);
			return SPRD_VDSP_RESULT_SUCCESS;
		} else {
			ALOGE("func:%s err fd:%d result:%d\n" , __func__ , fd , result);
			return SPRD_VDSP_RESULT_FAIL;
		}
	}
	//return SPRD_VDSP_RESULT_FAIL;
}

__attribute__ ((visibility("default"))) enum sprd_vdsp_result sprd_cavdsp_unloadlibrary(void *handle , const char *libname)
{
	sp<IVdspService> cs = NULL;
	enum sprd_vdsp_result result;
	uint32_t generation;
	int32_t fd;
	struct vdsp_handle *hnd = (struct vdsp_handle*) handle;
	cs = getVdspService();
	if((cs == NULL) || (NULL == handle))
	{
		/*fd is abnormal value*/
		ALOGE("func:%s err param cs:%p , handle:%p\n" , __func__ , cs.get() , handle);
		return SPRD_VDSP_RESULT_FAIL;
	}
	generation = hnd->generation;
	fd = hnd->fd;
	//if(cs != NULL)
	{
		gLock.lock();
		if(0 != Check_GenrationValid(generation))
		{
			ALOGE("func:%s Check_GenrationValid failed generation:%d , gGeneration:%d\n" , __func__ ,
				generation , gGeneration);
			gLock.unlock();
			return SPRD_VDSP_RESULT_OLD_GENERATION;
		}
		if(0 != Check_FdValid(fd))
                {
			ALOGE("func:%s Check_FdValid failed fd:%d\n" , __func__ , fd);
			gLock.unlock();
                        return SPRD_VDSP_RESULT_FAIL;
                }
		if(gOpenedCount == 0)
		{
			gLock.unlock();
			ALOGE("func:%s gOpenedCount 0 failed fd:%d\n" , __func__ , fd);
			return SPRD_VDSP_RESULT_FAIL;
		}
		gWorking ++;
		g_FdInfo[fd].working ++;
		gLock.unlock();
		result = (enum sprd_vdsp_result)cs->unloadXrpLibrary(gfakeBinder , libname);
		gLock.lock();
		gWorking --;
		g_FdInfo[fd].working --;
		gLock.unlock();
		if(0 == result) {
			ALOGD("func:%s  fd:%d result:%d , gDeathRecipient:%p\n" , __func__ , fd , result ,gDeathRecipient.get());
			return SPRD_VDSP_RESULT_SUCCESS;
		} else {
			ALOGE("func:%s err fd:%d result:%d , gDeathRecipient:%p\n" , __func__ , fd , result ,gDeathRecipient.get());
			return SPRD_VDSP_RESULT_FAIL;
		}
	}
}
__attribute__ ((visibility("default"))) enum sprd_vdsp_result sprd_cavdsp_power_hint(void *handle , enum sprd_vdsp_power_level level , uint32_t permanent)
{
	sp<IVdspService> cs = NULL;
        enum sprd_vdsp_result result;
        uint32_t generation;
        int32_t fd;
        struct vdsp_handle *hnd = (struct vdsp_handle*) handle;
        cs = getVdspService();
        if((cs == NULL) || (NULL == handle))
        {
                /*fd is abnormal value*/
                ALOGE("func:%s err param cs:%p , handle:%p\n" , __func__ , cs.get() , handle);
                return SPRD_VDSP_RESULT_FAIL;
        }
        generation = hnd->generation;
        fd = hnd->fd;
        //if(cs != NULL)
        {
                gLock.lock();
                if(0 != Check_GenrationValid(generation))
                {
                        ALOGE("func:%s Check_GenrationValid failed generation:%d , gGeneration:%d\n" , __func__ ,
                                generation , gGeneration);
                        gLock.unlock();
                        return SPRD_VDSP_RESULT_OLD_GENERATION;
                }
                if(0 != Check_FdValid(fd))
                {
                        ALOGE("func:%s Check_FdValid failed fd:%d\n" , __func__ , fd);
                        gLock.unlock();
                        return SPRD_VDSP_RESULT_FAIL;
                }
                if(gOpenedCount == 0)
                {
                        gLock.unlock();
                        ALOGE("func:%s gOpenedCount 0 failed fd:%d\n" , __func__ , fd);
                        return SPRD_VDSP_RESULT_FAIL;
                }
                gWorking ++;
		g_FdInfo[fd].working ++;
                gLock.unlock();
                result = (enum sprd_vdsp_result)cs->powerHint(gfakeBinder , level , permanent);
                gLock.lock();
                gWorking --;
                g_FdInfo[fd].working --;
                gLock.unlock();
                if(0 == result) {
			ALOGD("func:%s ok fd:%d result:%d , gDeathRecipient:%p\n" , __func__ , fd , result ,gDeathRecipient.get());
                        return SPRD_VDSP_RESULT_SUCCESS;
                } else {
			ALOGE("func:%s err fd:%d result:%d , gDeathRecipient:%p\n" , __func__ , fd , result ,gDeathRecipient.get());
                        return SPRD_VDSP_RESULT_FAIL;
                }
        }
}
