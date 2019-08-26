#ifndef _LIBINPUT_IXRP_SERVICE_H
#define _LIBINPUT_IXRP_SERVICE_H

#include <stdint.h>
#include <sys/types.h>

#include <binder/IInterface.h>
#include <utils/Mutex.h>
#include <utils/List.h>
#include "vdsp_interface.h"

//#define DVFS_OPEN

namespace android {

class ClientInfo;


struct PTest {
	uint32_t r;
	uint32_t g;
	uint32_t b;
};

struct VdspInputOutput {
	int32_t fd;
	void *viraddr;
	uint32_t phy_addr;
	uint32_t size;
	enum sprd_vdsp_bufflag flag;
};
static const char* TAG_Server = "VdspService_Server";
static const char* TAG_Client = "VdspService_Client";
static const char *TAG_Test = "VdspService_Test";
/*
 * This class defines the Binder IPC interface for accessing various
 * IVdspService features.
 */
class __attribute__ ((visibility("default"))) IVdspService : public IInterface {
public:
	DECLARE_META_INTERFACE(VdspService)	
	virtual int32_t openXrpDevice(sp<IBinder> &client , enum sprd_vdsp_worktype type) = 0;
	virtual int32_t closeXrpDevice(sp<IBinder> &client) = 0;
	virtual int32_t sendXrpCommand(sp<IBinder> &client , const char *nsid , struct VdspInputOutput *in , struct VdspInputOutput *out , 
						struct VdspInputOutput *buffer , uint32_t bufnum , uint32_t priority) = 0;
	virtual int32_t loadXrpLibrary(sp<IBinder> &client , const char* name , struct VdspInputOutput *buffer) = 0;
	virtual int32_t unloadXrpLibrary(sp<IBinder> &client , const char *name) = 0;
	virtual int32_t powerHint(sp<IBinder> &client , enum sprd_vdsp_power_level level , uint32_t permanent) = 0;
};


/**
 * Binder implementation.
 */
class __attribute__ ((visibility("default"))) BnVdspService : public BnInterface<IVdspService> {
public:
	enum {
		ACTION_OPEN = IBinder::FIRST_CALL_TRANSACTION,
		ACTION_CLOSE= IBinder::FIRST_CALL_TRANSACTION +1,
		ACTION_SEND_CMD = IBinder::FIRST_CALL_TRANSACTION +2, 
		ACTION_LOAD_LIBRARY = IBinder::FIRST_CALL_TRANSACTION +3,
		ACTION_UNLOAD_LIBRARY = IBinder::FIRST_CALL_TRANSACTION + 4,
		ACTION_POWER_HINT = IBinder::FIRST_CALL_TRANSACTION + 5,
	};
	virtual status_t onTransact(uint32_t code, const Parcel& data,
					Parcel* reply, uint32_t flags = 0);
	virtual int32_t openXrpDevice(sp<IBinder> &client , enum sprd_vdsp_worktype type);
	virtual int32_t closeXrpDevice(sp<IBinder> &client);
#if 1
	virtual int32_t sendXrpCommand(sp<IBinder> &client , const char *nsid , struct VdspInputOutput *input , struct VdspInputOutput *output ,
					struct VdspInputOutput *buffer , uint32_t bufnum , uint32_t priority);
	virtual int32_t loadXrpLibrary(sp<IBinder> &client , const char* name , struct VdspInputOutput *buffer);
	virtual int32_t unloadXrpLibrary(sp<IBinder> &client , const char *name);
	virtual int32_t powerHint(sp<IBinder> &client , enum sprd_vdsp_power_level level , uint32_t permanent);
	virtual void lockMlock();
	virtual void unlockMlock();
	virtual void lockLoadLock();
	virtual void unlockLoadLock();
#endif
#if 1
	class VdspServerDeathRecipient: public IBinder::DeathRecipient
	{
	public:
		VdspServerDeathRecipient(BnVdspService* service){mVdspService = service;}
		~VdspServerDeathRecipient() {}

		// IBinder::DeathRecipient
		virtual void binderDied(const wp<IBinder> &who);
	public:
		BnVdspService *mVdspService;
	};

private:
	int32_t AddClientOpenNumber(sp<IBinder> &client , int32_t *count);
	int32_t DecClientOpenNumber(sp<IBinder> &client);
	int32_t GetClientOpenNum(sp<IBinder> &client);
	int32_t AddClientLoadNumByName(const char *libname , sp<IBinder> &client);
	int32_t DecClientLoadNumByName(const char *libname , sp<IBinder> &client);
	int32_t GetClientLoadNumByName(const char *libname , sp<IBinder> &client);
	int32_t GetLoadNumTotalByName(const char *libname);
	int32_t ClearClientInfo(sp<IBinder> &client);
	void    AddDecOpenCount(int32_t count);
	int32_t GetClientLoadTotalNum(sp<IBinder> &client);
	int32_t closeXrpDevice_NoLock(sp<IBinder> &client);
	void* MapIonFd(int32_t infd , uint32_t size);
	int32_t unMapIon(void *ptr , size_t size);
	int32_t unloadLibraryLoadByClient(sp<IBinder> &client);
	int32_t GetTotalOpenNumAndLibCount(int32_t *opencount , int32_t *libcount);

#endif
private:

	Mutex mLock;
	Mutex mLoadLock;
	uint32_t mopen_count;
	uint32_t mworking;
	void *mDevice;
	enum sprd_vdsp_worktype	mType;
#ifdef DVFS_OPEN
	int32_t mDvfs;
#endif
#if 1
	Vector<sp<ClientInfo>> mClientInfo;
	int32_t mIonDevFd;
//	List<sp<IBinder>> m_callbacks;
//	sp<IBinder> m_currentcb;
//	sp<VdspServerDeathRecipient> mDeathRecipient;
//	List<sp<VdspServerDeathRecipient>> mDeathRecipientList;
#endif
};

class LoadLibInfo : public virtual RefBase
{
public:
        char libname[32];
        int32_t loadcount;;
};

class ClientInfo : public virtual RefBase
{
public:
        sp<IBinder> mclientbinder;
        int32_t mopencount;
	int32_t mworking;
        sp<BnVdspService::VdspServerDeathRecipient>mDepthRecipient;
        Vector<sp<LoadLibInfo>> mloadinfo;
};


} // namespace android

#endif // _LIBINPUT_IALGORITHM_SERVICE_H

