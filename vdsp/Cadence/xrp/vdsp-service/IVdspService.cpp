
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "IVdspService.h"
#include <android/log.h>
#include "vdsp_interface_internal.h"
#include <ion/ion.h>
#include <sprd_ion.h>
#include <sys/mman.h>
#ifdef DVFS_OPEN
#include "vdsp_dvfs.h"
#endif



namespace android {
class ClientCallback : public BBinder{
public:
        ClientCallback() { m_count = 0; mworking = 0;}
        virtual ~ClientCallback(){}
        void addCount() {m_count++;}
        void decCount() {m_count--;}
	void setWorking(int32_t flag){mworking = flag;}
        int32_t getCount() {return m_count;}
private:
        int32_t m_count;
	int32_t mworking;
};

class BpVdspService : public BpInterface<IVdspService> {
public:
    explicit BpVdspService(const sp<IBinder>& impl) :
            BpInterface<IVdspService>(impl) {

		callback = new ClientCallback;
	}

	virtual int32_t openXrpDevice(__unused sp<IBinder> &client , enum sprd_vdsp_worktype type) {
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy  function openXrpDevice\n");
		Parcel data, reply;
		status_t status;
		int32_t result;
		data.writeInterfaceToken(IVdspService::getInterfaceDescriptor());
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function openXrpDevice callback:%p\n" , callback.get());
		data.writeStrongBinder(sp<IBinder> (callback));
		data.writeInt32((int32_t) type);
		status = remote()->transact(BnVdspService::ACTION_OPEN, data, &reply);
		result = reply.readInt32();
		if(status != OK) {
			__android_log_print(ANDROID_LOG_ERROR ,TAG_Client,"call proxy function openXrpDevice err result = %d \n",result);
			return status;
		}
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function openXrpDevice result = %d \n",result);
		return result;
	}


    virtual int32_t closeXrpDevice(__unused sp<IBinder> &client) {
		Parcel data, reply;
		status_t status;
		int32_t result;
		 __android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function %s\n",__func__);
		data.writeInterfaceToken(IVdspService::getInterfaceDescriptor());
		data.writeStrongBinder(sp<IBinder> (callback));
		status = remote()->transact(BnVdspService::ACTION_CLOSE, data, &reply);
		result = reply.readInt32();
		if(status != OK) {
			return status;
		}
		return result;

	}
	virtual int32_t sendXrpCommand(__unused sp<IBinder> &client , const char *nsid , struct VdspInputOutput *input , struct VdspInputOutput *output ,
                                         struct VdspInputOutput *buffer ,  uint32_t bufnum ,  uint32_t priority) {
		uint32_t i;
		Parcel data, reply;
		status_t status;
		int32_t result;
		data.writeInterfaceToken(IVdspService::getInterfaceDescriptor());
		/*write client binder*/
		data.writeStrongBinder(sp<IBinder> (callback));
		/*write nsid*/
		data.writeCString(nsid);
		/*write whether input exist*/
		data.writeInt32((input != NULL));
		/*write input*/
		if(input != NULL) {
			data.writeFileDescriptor(input->fd);
			data.writeUint32(input->size);
		}
		/*write output exist flag*/
		data.writeInt32((output != NULL));
		/*write output*/
		if(NULL != output) {
			data.writeFileDescriptor(output->fd);
			data.writeUint32(output->size);
		}
		/*write buffers exist?*/
		data.writeInt32((NULL != buffer));
		if(NULL != buffer) {
			/*write buffer num*/
			data.writeUint32(bufnum);
			/*write buffers*/
			for(i = 0; i < bufnum; i++)
			{
				data.writeFileDescriptor(buffer[i].fd);
				data.writeUint32(buffer[i].size);
				data.writeUint32(buffer[i].phy_addr);
				data.writeInt32(buffer[i].flag);
				__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy %s , buffer fd:%d , size:%d\n" , __func__ , buffer[i].fd , buffer[i].size);
			}
		}
		/*write priority*/
		data.writeUint32(priority);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function %s before transact\n",__func__);
		status = remote()->transact(BnVdspService::ACTION_SEND_CMD, data, &reply);
		result = reply.readInt32();
		if(status != OK) {
			__android_log_print(ANDROID_LOG_ERROR ,TAG_Client,"call proxy function %s err status:%d , result = %d after transact\n",__func__ , status , result);
			return status;
		}
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function %s status:%d , result = %d after transact\n",__func__ , status , result);
		return result;
	}

	virtual int32_t loadXrpLibrary(__unused sp<IBinder> &client , const char* name ,  struct VdspInputOutput *input){

		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy	function %s nsid = %s\n",__func__ , name);
		Parcel data, reply;
		status_t status;
		int32_t result;
		data.writeInterfaceToken(IVdspService::getInterfaceDescriptor());
		/*write client binder*/
		data.writeStrongBinder(sp<IBinder> (callback));
		/*write nsid*/
		data.writeCString(name);
		/*write input*/
		data.writeFileDescriptor(input->fd);
		data.writeInt32(input->size);
		status = remote()->transact(BnVdspService::ACTION_LOAD_LIBRARY, data, &reply);
		result = reply.readInt32();
		if(status != OK) {
			__android_log_print(ANDROID_LOG_ERROR ,TAG_Client,"call proxy function %s err status:%d result = %d \n",__func__ , status , result);
			return status;
		}
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function %s status:%d result = %d \n",__func__ , status , result);
		return result;
	}
	virtual int32_t unloadXrpLibrary(__unused sp<IBinder> &client , const char* name){
		Parcel data , reply;
		status_t status;
		int32_t result;
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy    function %s , name:%s\n" , __func__ , name);
		data.writeInterfaceToken(IVdspService::getInterfaceDescriptor());
		/*write client binder*/
		data.writeStrongBinder(sp<IBinder> (callback));
		/*write nsid*/
		data.writeCString(name);
		status = remote()->transact(BnVdspService::ACTION_UNLOAD_LIBRARY, data, &reply);
		result = reply.readInt32();
		if(status != OK) {
			__android_log_print(ANDROID_LOG_ERROR ,TAG_Client,"call proxy function %s err status:%d , result = %d\n",__func__ ,status , result);
			return status;
		}
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function %s status:%d , result = %d\n",__func__ ,status , result);
		return result;
	}
	virtual int32_t powerHint(__unused sp<IBinder> &client , enum sprd_vdsp_power_level level , uint32_t permanent) {
		Parcel data , reply;
		status_t status;
		int32_t result;
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy    function %s , level:%d\n" , __func__ , level);
		data.writeInterfaceToken(IVdspService::getInterfaceDescriptor());
		data.writeStrongBinder(sp<IBinder> (callback));
		data.writeInt32(level);
		data.writeInt32(permanent);
		status = remote()->transact(BnVdspService::ACTION_POWER_HINT , data, &reply);
		result = reply.readInt32();
		if(status != OK) {
			__android_log_print(ANDROID_LOG_ERROR , TAG_Client,"call proxy function %s err status:%d , result = %d\n",__func__ ,status , result);
			return status;
		}
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"call proxy function %s status:%d , result = %d\n",__func__ ,status , result);
		return result;
	}
private:
	sp<ClientCallback> callback;
};


IMPLEMENT_META_INTERFACE(VdspService, "android.camera.IVdspService");

status_t BnVdspService::onTransact(
        uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
	switch(code) {
	case ACTION_OPEN: {
		sp<IBinder> client;
		enum sprd_vdsp_worktype type;
		CHECK_INTERFACE(IVdspService, data, reply);
		client = data.readStrongBinder();
		type = (enum sprd_vdsp_worktype) data.readInt32();
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"server ACTION_OPEN enter client:%p\n", client.get());
		uint32_t r = openXrpDevice(client , type);
		reply->writeInt32(r);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"server ACTION_OPEN reply write r:%d\n" , r);
	}
	break;
	case ACTION_CLOSE: {
		sp<IBinder> client;
		CHECK_INTERFACE(IVdspService, data, reply);
		client = data.readStrongBinder();
		uint32_t r = closeXrpDevice(client);
		reply->writeInt32(r);
	}
	break;
	case ACTION_SEND_CMD: {
		sp<IBinder> client;
		const char *nsid;
		struct VdspInputOutput input,output;
		struct VdspInputOutput *pinput = NULL;
		struct VdspInputOutput *poutput = NULL;
		struct VdspInputOutput *buffer = NULL;
		input.fd=0;
		input.size=0;
		output.fd=0;
		output.size=0;
		uint32_t bufnum = 0;
		uint32_t priority = 0;
		uint32_t i;
		int32_t bufvalid = 0;
		CHECK_INTERFACE(IVdspService, data, reply);
		client = data.readStrongBinder();
		nsid = data.readCString();
		bufvalid = data.readInt32();
		if(bufvalid) {
			input.fd = data.readFileDescriptor();
			input.size = data.readUint32();
			pinput = &input;
		}
		bufvalid = data.readInt32();
		if(bufvalid) {
		output.fd = data.readFileDescriptor();
		output.size = data.readUint32();
		poutput = &output;
		}
		bufvalid = data.readInt32();
		if(bufvalid) {
			bufnum = data.readUint32();
			if(bufnum != 0) {
				buffer = (struct VdspInputOutput *) malloc(bufnum * sizeof(struct VdspInputOutput));
				if(buffer == NULL)
				{
					reply->writeInt32(-ENOMEM);
					return NO_ERROR;
				}
				for(i = 0; i < bufnum; i++)
				{

					buffer[i].fd = data.readFileDescriptor();
					buffer[i].size = data.readUint32();
					buffer[i].phy_addr = data.readUint32();
					buffer[i].flag = (enum sprd_vdsp_bufflag)data.readInt32();
					__android_log_print(ANDROID_LOG_DEBUG , TAG_Server , "action send cmd buff i:%d , fd:%d, size:%d\n" , i , buffer[i].fd, buffer[i].size);
				}
			}
			else {
				buffer = NULL;
			}
		}
		priority = data.readUint32();
		uint32_t r = sendXrpCommand(client , nsid, pinput , poutput , buffer , bufnum , priority);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"action send cmd nsid:%s , input fd:%d, size:%d, output fd:%d, size:%d , bufnum:%d , priority:%d\n",
                                        nsid , input.fd, input.size , output.fd , output.size , bufnum , priority);
		if(buffer != NULL)
			free(buffer);
		reply->writeInt32(r);
	}
	break;
	case ACTION_LOAD_LIBRARY: {
		sp<IBinder> client;
		const char *name;
		struct VdspInputOutput input;
		CHECK_INTERFACE(IVdspService, data, reply);
		client = data.readStrongBinder();
		name = data.readCString();
		input.fd = data.readFileDescriptor();
		input.size = data.readInt32();
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"action load library nsid:%s , input fd:%d ,size:%d\n" , name , input.fd , input.size);
		uint32_t r = loadXrpLibrary(client , name , &input);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"action load library nsid:%s ,result:%d\n" , name , r);
		reply->writeInt32(r);
	}
	break;
	case ACTION_UNLOAD_LIBRARY: {
		sp<IBinder> client;
		const char *name;
		CHECK_INTERFACE(IVdspService, data, reply);
		client = data.readStrongBinder();
		name = data.readCString();
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"action unload library nsid:%s\n" , name);
		uint32_t r = unloadXrpLibrary(client , name);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"action unload library nsid:%s , result:%d\n" , name , r);
		reply->writeInt32(r);
	}
	break;
	case ACTION_POWER_HINT: {
		sp<IBinder> client;
		uint32_t permanent;
		enum sprd_vdsp_power_level level;
		CHECK_INTERFACE(IVdspService, data, reply);
		client = data.readStrongBinder();
		level = (enum sprd_vdsp_power_level) data.readInt32();
		permanent = data.readInt32();
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"action ACTION_POWER_HINT library level:%d\n" , level);
		uint32_t r = powerHint(client , level , permanent);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"action ACTION_POWER_HINT library level:%d ,result:%d\n" , level , r);
		reply->writeInt32(r);
	}
	break;

    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
    return NO_ERROR;
}

int32_t BnVdspService::openXrpDevice(sp<IBinder> &client , enum sprd_vdsp_worktype type) {
	int32_t newclient;
	AutoMutex _l(mLock);
	if(mopen_count == 0) {
		mDevice = sprd_vdsp_open_device(0 , type);
		mType = type;
		mIonDevFd = open("/dev/ion" , O_RDWR);
		#ifdef DVFS_OPEN
		mDvfs = init_dvfs(mDevice);
		#endif
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s , really open device type:%d device:%p , IondevFd:%d\n" , __func__ , type , mDevice , mIonDevFd);
		if((mDevice != NULL) && (mIonDevFd > 0))
			mopen_count++;
		else {
			#ifdef DVFS_OPEN
			if(0 != mDvfs) {
				deinit_dvfs(mDevice);
				mDvfs = 0;
			}
			#endif
			if(mDevice != NULL)
				sprd_vdsp_release_device(mDevice);
			if(mIonDevFd > 0)
				close(mIonDevFd);
			mDevice = NULL;
			mIonDevFd = -1;
			mType = SPRD_VDSP_WORK_MAXTYPE;
			__android_log_print(ANDROID_LOG_ERROR , TAG_Server , "func:%s , error mDevice:%p ,mIonDevFd:%d" , __func__ , mDevice , mIonDevFd);
			return -1;
		}
	}
	else {
		if(mType == type) {
			mopen_count++;
		}
		else {
			__android_log_print(ANDROID_LOG_ERROR,TAG_Server,"func:%s , open failed client:%p , mType:%d, type:%d\n" , __func__ , client.get() ,
						mType , type);
			return -2;
		}
	}
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s , before AddClientOpenNumber client:%p, mopen_count:%d\n" , __func__ , client.get() , mopen_count);
	AddClientOpenNumber(client , &newclient);
	return 0;
}
int32_t BnVdspService::closeXrpDevice(sp<IBinder> &client) {
	int32_t ret = 0;
	int32_t count;
	AutoMutex _l(mLock);
	count = GetClientOpenNum(client);
	if(count <= 0) {
		__android_log_print(ANDROID_LOG_ERROR,TAG_Server, "func:%s , client:%p open count is 0, return invalid client\n" , __func__ , client.get());
		return -3;
	}
	mopen_count --;
	if(mopen_count == 0) {
		if(mworking != 0) {
			/*busying*/
			mopen_count ++;
			__android_log_print(ANDROID_LOG_ERROR,TAG_Server, "func:%s , mworking:%d\n" , __func__ , mworking);
			return -2;
		}
		#ifdef DVFS_OPEN
		if(0 != mDvfs) {
			deinit_dvfs(mDevice);
			mDvfs = 0;
		}
		#endif
		sprd_vdsp_release_device(mDevice);
		close(mIonDevFd);
		mIonDevFd = -1;
		mDevice = NULL;
		mType = SPRD_VDSP_WORK_MAXTYPE;
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , really release device:%p\n" , __func__ , mDevice);
	}
	DecClientOpenNumber(client);
	if(0 == mopen_count) {
		int32_t opencount ,libcount;
		GetTotalOpenNumAndLibCount(&opencount ,&libcount);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "Check result func:%s , really release device , opencount:%d, libcount:%d\n" , __func__ ,
				opencount , libcount);
	}
	return ret;
}
int32_t BnVdspService::closeXrpDevice_NoLock(sp<IBinder> &client) {
        int32_t ret = 0;
        int32_t count;
        count = GetClientOpenNum(client);
        if(count <= 0) {
                __android_log_print(ANDROID_LOG_ERROR,TAG_Server, "func:%s , client:%p open count is 0, return invalid client\n" , __func__ , client.get());
                return -3;
        }
        mopen_count --;
        if(mopen_count == 0) {
		if(mworking != 0) {
			/*busying*/
			while(mworking != 0) {
			mLoadLock.unlock();
			mLock.unlock();
			usleep(1000);
			mLoadLock.lock();
			mLock.lock();
			}
			if(mopen_count != 0)
				goto __exitprocess;
		}
                sprd_vdsp_release_device(mDevice);
                close(mIonDevFd);
                mIonDevFd = -1;
		mDevice = NULL;
                __android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , really release device:%p\n" , __func__ , mDevice);
        }
__exitprocess:
        ret = DecClientOpenNumber(client);
        return ret;
}
int32_t BnVdspService::sendXrpCommand(sp<IBinder> &client , const char *nsid , struct VdspInputOutput *input , struct VdspInputOutput *output ,
                                         struct VdspInputOutput *buffer , uint32_t bufnum , uint32_t priority) {
	int32_t client_opencount = 0;
	void * pinput , *poutput;
	int32_t ret;
	mLock.lock();
	if((mopen_count == 0) || (0 == (client_opencount = GetClientOpenNum(client)))) {
		mLock.unlock();
		/*error not opened*/
		__android_log_print(ANDROID_LOG_ERROR,TAG_Server,"sendXrpCommand mopen_count:%d, client:%p opencount:%d\n" , mopen_count , client.get() , client_opencount);
		return -1;
	}
	mworking ++;
	mLock.unlock();
	/*do send */
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , sprd_vdsp_send_command mDevice:%p , nsid:%s , input:%p,output:%p, buffer:%p\n" , __func__ , mDevice , nsid , input , output , buffer);
#if 1
	if((input!= NULL) && (-1 != input->fd)) {
		pinput = MapIonFd(input->fd , input->size);
		if(pinput == NULL) {
			__android_log_print(ANDROID_LOG_ERROR,TAG_Server, "func:%s , map input error\n" , __func__);
			return -1;
		}
		input->viraddr = pinput;
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , map input fd:%d inputvir:%p\n" , __func__ ,input->fd , input->viraddr);
	}
	if((output != NULL) && (-1 != output->fd)) {
		poutput = MapIonFd(output->fd  , output->size);
		if(NULL == poutput) {
			__android_log_print(ANDROID_LOG_ERROR,TAG_Server, "func:%s , map output error\n" , __func__);
			return -1;
		}
		output->viraddr = poutput;
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , map output fd:%d outputvir:%p\n" , __func__ ,output->fd , output->viraddr);
	}
#else
	if(NULL != buffer) {
		int32_t oldfd;
		for(uint32_t i = 0; i < bufnum; i++) {
			oldfd = buffer[i].fd;
			buffer[i].fd = MapIonFd(buffer[i].fd , buffer[i].size);
			__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , map buffer i:%d, fd:%d to new fd:%d\n" , __func__ ,i , oldfd  , buffer[i].fd);
		}
	}
#endif
	#ifdef DVFS_OPEN
	preprocess_work_piece();
	#endif
	ret = sprd_vdsp_send_command_directly(mDevice , nsid ,(struct sprd_vdsp_inout*)input, (struct sprd_vdsp_inout*)output,(struct sprd_vdsp_inout*)buffer ,bufnum,(enum sprd_xrp_queue_priority)priority);
	#ifdef DVFS_OPEN
	postprocess_work_piece();
	#endif
#if 1
	if((input != NULL) && (-1 != input->fd)) {
		unMapIon(pinput , input->size);
	}
	if((output != NULL) && (-1 != output->fd)) {
		unMapIon(poutput , output->size);
	}
#endif
//	sprd_vdsp_send_command(mDevice , nsid ,(struct sprd_vdsp_inout*)input, (struct sprd_vdsp_inout*)output,(struct sprd_vdsp_inout*)buffer ,bufnum,(enum sprd_xrp_queue_priority)priority);
	mLock.lock();
	mworking --;
	mLock.unlock();
	return ret;
}
int32_t BnVdspService::loadXrpLibrary(sp<IBinder> &client , const char *name , struct VdspInputOutput *input) {
	int32_t ret = 0;
	int32_t client_opencount;
	AutoMutex _l(mLoadLock);
	mLock.lock();
	client_opencount = GetClientOpenNum(client);
	if((mopen_count == 0) || (0 == client_opencount)) {
		/*error not opened*/
		mLock.unlock();
		__android_log_print(ANDROID_LOG_ERROR ,TAG_Server,"func:%s , mopen_count is:%d , client:%p , opencount:%d , return error\n" , __func__ , mopen_count , client.get(),client_opencount);
		return -1;
	}
	mworking ++;
	mLock.unlock();
	//buffer = input->fd;
	if(0 == GetLoadNumTotalByName(name)) {
		#ifdef DVFS_OPEN
		preprocess_work_piece();
		#endif
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s , before really load lib:%s , device:%p\n" , __func__ , name , mDevice);
		ret = sprd_vdsp_load_library(mDevice , (struct sprd_vdsp_inout*)input , name , SPRD_XRP_PRIORITY_2);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s , after really load lib:%s , device:%p result:%d\n" , __func__ , name , mDevice ,ret);
		#ifdef DVFS_OPEN
		postprocess_work_piece();
		#endif
	}
	if(0 == ret) {
		AddClientLoadNumByName(name , client);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s , current libname:%s , total count:%d\n" , __func__ , name , GetLoadNumTotalByName(name));
	}
	mLock.lock();
	mworking --;
	mLock.unlock();
	return ret;
}
int32_t BnVdspService::unloadXrpLibrary(sp<IBinder> &client , const char *name) {
	int32_t ret = 0;
	int32_t client_opencount;
	AutoMutex _l(mLoadLock);
	mLock.lock();
	client_opencount = GetClientOpenNum(client);
	if((mopen_count == 0) || (0 ==client_opencount)) {
		mLock.unlock();
		__android_log_print(ANDROID_LOG_ERROR,TAG_Server,"func:%s , mopen_count is:%d , client:%p , opencount:%d , return error\n" , __func__ , mopen_count , client.get(),client_opencount);
		/*error not opened*/
		return -1;
	}
	mworking ++;
	mLock.unlock();
	ret = DecClientLoadNumByName(name , client);
	if(ret != 0) {
		__android_log_print(ANDROID_LOG_ERROR ,TAG_Server,"func:%s , DecClientLoadNumByName name:%s , client:%p , return error\n" , __func__ ,name , client.get());
		mLock.lock();
		mworking --;
		mLock.unlock();
		return -1;
	}
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , current libname:%s , total count:%d\n" , __func__ , name , GetLoadNumTotalByName(name));
	if(0 == GetLoadNumTotalByName(name)) {
		#ifdef DVFS_OPEN
                preprocess_work_piece();
                #endif
		ret = sprd_vdsp_unload_library(mDevice , name , SPRD_XRP_PRIORITY_2);
		#ifdef DVFS_OPEN
                postprocess_work_piece();
                #endif
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s , really unload lib:%s , device:%p\n" , __func__ , name , mDevice);
	}
#if 0
	if(0 == ret) {
		DecClientLoadNumByName(name , client);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , current libname:%s , total count:%d\n" , __func__ , name , GetLoadNumTotalByName(name));
	}
#endif
	mLock.lock();
	mworking --;
	mLock.unlock();
	return ret;
}
int32_t BnVdspService::powerHint(sp<IBinder> &client , enum sprd_vdsp_power_level level , uint32_t permanent) {
	int32_t ret = 0;
	int32_t client_opencount;
	mLock.lock();
	client_opencount = GetClientOpenNum(client);
	if((mopen_count == 0) || (0 ==client_opencount)) {
		mLock.unlock();
		__android_log_print(ANDROID_LOG_ERROR ,TAG_Server,"func:%s , mopen_count is:%d , client:%p , opencount:%d , return error\n" , __func__ , mopen_count , client.get(),client_opencount);
                /*error not opened*/
		return -1;
	}
	mworking ++;
	mLock.unlock();
#ifdef DVFS_OPEN
	ret = set_powerhint_flag(mDevice , level ,permanent);//sprd_cavdsp_power_hint(mDevice , level , permanent);
#else
	__android_log_print(ANDROID_LOG_DEBUG , TAG_Server, "func:%s , level:%d, permant:%d\n" , __func__ , level, permanent);
#endif
	mLock.lock();
	mworking --;
	mLock.unlock();
	return ret;
}
void BnVdspService::VdspServerDeathRecipient::binderDied(const wp<IBinder> &who){
	//pthread_t tid;
	int clientopencount = 0;
	__android_log_print(ANDROID_LOG_WARN ,TAG_Server,"call binderDied who:%p\n" , who.promote().get());
	mVdspService->lockLoadLock();
	mVdspService->lockMlock();
	//DecClientLoadNumByName(name , client);
	sp<IBinder> client = who.promote();
	/*unload library all loaded by this client*/
	mVdspService->unloadLibraryLoadByClient(client);
	/*close all open count for this client*/
	clientopencount = mVdspService->GetClientOpenNum(client);
	for(int i = 0;  i < clientopencount; i++) {
		__android_log_print(ANDROID_LOG_WARN,TAG_Server,"call binderDied who:%p , open num is:%d , close:%d\n" , who.promote().get() , clientopencount, i);
		mVdspService->closeXrpDevice_NoLock(client);
	}
	mVdspService->ClearClientInfo(client);
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "Check result binderDied who:%p , client opennum:%d , client load libnum:%d\n" ,  client.get() ,
			mVdspService->GetClientOpenNum(client) , mVdspService->GetClientLoadTotalNum(client));

	mVdspService->unlockMlock();
	mVdspService->unlockLoadLock();
}

int32_t BnVdspService::unloadLibraryLoadByClient(sp<IBinder> &client) {
	int32_t ret = 0;
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	int clientfind =0;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		if((*iter)->mclientbinder == client) {
			clientfind = 1;
			for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); iter1++) {
				(*iter1)->loadcount = 0;
				if(GetLoadNumTotalByName((*iter1)->libname) == 0) {
					ret = sprd_vdsp_unload_library(mDevice ,(*iter1)->libname , SPRD_XRP_PRIORITY_2);
					__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , who:%p , libname:%s is really unloaded\n" , 
								__func__ , client.get() , (*iter1)->libname);
				} else {
					 __android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func: %s , who:%p , libname:%s num is set 0\n" ,
                                                                __func__ , client.get() , (*iter1)->libname);
				}
			}
		}
	}
	return ret;
}

int32_t BnVdspService::AddClientLoadNumByName(const char *libname , sp<IBinder> &client) {
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	int clientfind =0;
	int namefind = 0;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		if((*iter)->mclientbinder == client) {
			clientfind = 1;
			for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); iter1++) {
				if(0 == strcmp(libname , (*iter1)->libname)) {
					namefind = 1;
					(*iter1)->loadcount ++;
					__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"AddClientLoadNumByName client is:%p , load count is:%d\n" , client.get() , (*iter1)->loadcount);
					return 0;
				}
			}
			/*no libname find add it*/
			sp<LoadLibInfo> newloadlibinfo = new LoadLibInfo;
			strcpy(newloadlibinfo->libname , libname);
			newloadlibinfo->loadcount = 1;
			(*iter)->mloadinfo.push_back(newloadlibinfo);
			__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "AddClientLoadNumByName new libname:%s added\n" , libname);
			return 0;
		}
	}
	__android_log_print(ANDROID_LOG_ERROR ,TAG_Server, "AddClientLoadNumByName error ------------- not find client find:%d, name find:%d\n" , clientfind , namefind);
	return -1;
}
int32_t BnVdspService::DecClientLoadNumByName(const char *libname , sp<IBinder> &client) {
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	int clientfind =0;
	int namefind = 0;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		if((*iter)->mclientbinder == client) {
			clientfind = 1;
			for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); /*iter1++*/) {
				if(0 == strcmp(libname , (*iter1)->libname)) {
					namefind = 1;
					(*iter1)->loadcount --;
					if(0 == (*iter1)->loadcount) {
						iter1 = (*iter)->mloadinfo.erase(iter1);
						__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , libname:%s is zero count ,client:%p relase it\n" , __func__ , libname , client.get());
					}
					else {
						__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "AddClientLoadNumByName client is:%p , load count is:%d\n" , client.get() , (*iter1)->loadcount);
					}
					return 0;
				}
				else {
					iter1 ++;
				}
			}
		}
	}
	__android_log_print(ANDROID_LOG_ERROR ,TAG_Server, "DecClientLoadNumByName error ------------- not find client find:%d, name find:%d\n" , clientfind , namefind);
	return -1;
}
int32_t BnVdspService::GetClientLoadNumByName(const char *libname , sp<IBinder> &client) {
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	int32_t loadcount = 0;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		if(client == (*iter)->mclientbinder) {
			for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); iter1++) {
				if(0 == strcmp(libname , (*iter1)->libname)) {
					loadcount += (*iter1)->loadcount;
					__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "AddClientLoadNumByName client is:%p , load count is:%d\n" , client.get() , (*iter1)->loadcount);
					break;
				}
			}
			break;
		}
	}
	return loadcount;
}
int32_t BnVdspService::GetLoadNumTotalByName(const char *libname) {
	int32_t load_count = 0;
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); iter1++) {
			if(0 == strcmp((*iter1)->libname , libname)) {
				__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "GetLoadNumTotalByName one client count:%d\n" , (*iter1)->loadcount);
				load_count += (*iter1)->loadcount;
				break;
			}
		}
	}
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "GetLoadNumTotalByName total count:%d\n" ,load_count);
	return load_count;
}
int32_t BnVdspService::AddClientOpenNumber(sp<IBinder> &client , int32_t *newclient) {
	int32_t find = 0;
	*newclient = 0;
	Vector<sp<ClientInfo>>::iterator iter;
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s enter , client:%p , iter:%p ,end:%p\n" , __func__ , client.get(), mClientInfo.begin() , mClientInfo.end());
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		if(client == (*iter)->mclientbinder) {
			(*iter)->mopencount ++;
			find = 1;
			__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "AddClientOpenNumber opencount is:%d\n" , (*iter)->mopencount);
			break;
		}
	}
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:AddClientOpenNumber find:%d\n" , find);
	if(0 == find) {
		sp<ClientInfo> newclientinfo = new ClientInfo;
		newclientinfo->mclientbinder = client;
		newclientinfo->mopencount = 1;
		newclientinfo->mDepthRecipient = new VdspServerDeathRecipient(this);
                client->linkToDeath(newclientinfo->mDepthRecipient);
		mClientInfo.push_back(newclientinfo);
		*newclient = 1;
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "AddClientOpenNumber new client opencount is 1\n");
	}
	return 0;
}
int32_t BnVdspService::DecClientOpenNumber(sp<IBinder> &client) {
	int32_t find = 0;
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); /*iter++*/) {
		if(client == (*iter)->mclientbinder) {
			(*iter)->mopencount--;
			find = 1;
			if(0 == (*iter)->mopencount) {
				for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); /*iter1++*/) {
					iter1 = (*iter)->mloadinfo.erase(iter1);
				}
				mClientInfo.erase(iter);
				__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s , erase client:%p\n" , __func__ , client.get());
			}
			else {
				__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "DecClientOpenNumber opencount is:%d\n" , (*iter)->mopencount);
			}
			break;
		} else {
			iter ++;
		}
	}
	if(0 == find) {
		__android_log_print(ANDROID_LOG_ERROR ,TAG_Server,"DecClientOpenNumber not find client, error------------------\n");
		return -1;
	}
	return 0;
}
int32_t BnVdspService::ClearClientInfo(sp<IBinder> &client) {
	int32_t load_count = 0;
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); /*iter++*/) {
		__android_log_print(ANDROID_LOG_DEBUG ,TAG_Server, "func:%s client cycle iter:%p , client:%p , iter clientbinder:%p\n" , __func__ , iter , client.get(),
                                (*iter)->mclientbinder.get());
		if((*iter)->mclientbinder == client) {
			(*iter)->mopencount = 0;
			__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s client matched:%p------------\n" , __func__ , client.get());
			for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); /*iter1++*/) {
				__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"func:%s mloadinfo iter1:%p , libname:%s , loadcount:%d\n" , __func__ , iter1,
					(*iter1)->libname , (*iter1)->loadcount);
				(*iter1)->loadcount = 0;
				iter1 = (*iter)->mloadinfo.erase(iter1);
				__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "func:%s ,iter1:%p  , mloadinfo end:%p , client:%p\n"  , __func__ , iter1 , (*iter)->mloadinfo.end() , client.get());
			}
			iter = mClientInfo.erase(iter);
			__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "ClearClientInfo client:%p\n" , client.get());
			return 0;
		} else {
			iter++;
		}
	}
	__android_log_print(ANDROID_LOG_ERROR ,TAG_Server, "GetLoadNumTotalByName total count:%d\n" ,load_count);
	return -1;
}
int32_t BnVdspService::GetClientOpenNum(sp<IBinder> &client)
{
	Vector<sp<ClientInfo>>::iterator iter;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		if((*iter)->mclientbinder == client) {
			__android_log_print(ANDROID_LOG_DEBUG,TAG_Server, "GetClientOpenNum client:%p , open count:%d\n" , client.get() , (*iter)->mopencount);
			return (*iter)->mopencount;
		}
	}
	return 0;
}
int32_t BnVdspService::GetTotalOpenNumAndLibCount(int32_t *opencount , int32_t *libcount)
{
	*opencount = 0;
	*libcount = 0;
	Vector<sp<ClientInfo>>::iterator iter;
        Vector<sp<LoadLibInfo>>::iterator iter1;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		*opencount += (*iter)->mopencount;
		for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); iter1++) {
			*libcount += (*iter1)->loadcount;
		}
	}
	return 0;
}
int32_t  BnVdspService::GetClientLoadTotalNum(sp<IBinder> &client) {
	int32_t loadcount = 0;
	Vector<sp<ClientInfo>>::iterator iter;
	Vector<sp<LoadLibInfo>>::iterator iter1;
	for(iter = mClientInfo.begin(); iter != mClientInfo.end(); iter++) {
		if(client == (*iter)->mclientbinder) {
			for(iter1 = (*iter)->mloadinfo.begin(); iter1 != (*iter)->mloadinfo.end(); iter1++) {
				loadcount += (*iter1)->loadcount;
			}
			break;
		}
	}
	return loadcount;
}


void BnVdspService::lockMlock()
{
	mLock.lock();
}
void BnVdspService::unlockMlock() {
	mLock.unlock();
}
void BnVdspService::lockLoadLock() {
	mLoadLock.lock();

}
void BnVdspService::unlockLoadLock() {
	mLoadLock.unlock();
}
void BnVdspService::AddDecOpenCount(int32_t count) {
	mopen_count += count;
}

void* BnVdspService::MapIonFd(int32_t infd , uint32_t size) {
	unsigned char *map_buf;
	map_buf = (unsigned char *)mmap(NULL, size , PROT_READ|PROT_WRITE,
			MAP_SHARED, infd , 0);
	if (map_buf == MAP_FAILED) {
		__android_log_print(ANDROID_LOG_ERROR,TAG_Server,"call Service  function MapIonFd Failed - mmap: %s\n",strerror(errno));
		return NULL;
	}
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"call Service  function size:%x ,MapIonFd data:%x,%x,%x,%x\n" , size , map_buf[0], map_buf[1] , map_buf[size-2] , map_buf[size-1]);
	return map_buf;
}
int32_t BnVdspService::unMapIon(void *ptr , size_t size) {
	munmap(ptr, size);
	return 0;
}
};
