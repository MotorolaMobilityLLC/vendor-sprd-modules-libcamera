#include <pthread.h>
#include <utils/Log.h>
#include <ion/ion.h>
#include <sprd_ion.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "IVdspService.h"
#include "vdsp_interface.h"
#include "Test.h"

//using namespace android;


void* thread1(__unused void* test)
{
	struct sprd_vdsp_client_inout in,out;
	uint32_t size = 8192;
	struct vdsp_handle handle;
	int ret;
	FILE *fp;
	char filename[256];
	int buffersize = 8192;
	void *inputhandle;
	void *outputhandle;
	void *inviraddr;
	void *outviraddr;
	inputhandle = sprd_alloc_ionmem(size , 0 , &in.fd , &inviraddr);
	outputhandle = sprd_alloc_ionmem(size , 0 , &out.fd , &outviraddr);
	in.size = size;
	out.size = size;
	in.viraddr = inviraddr;
	if(inputhandle != NULL)
	{
		memset(inviraddr , 0xbb , size);
	}
	if(outputhandle != NULL)
	{
		memset(outviraddr , 0xcc , size);
	}
	sprintf(filename , "/vendor/firmware/%s.bin" , "test_lib");
        fp = fopen(filename , "rb");
        if(fp) {
                ret = fread(in.viraddr , 1, buffersize , fp);
                fprintf(stderr , "yzl add %s , fwrite test_lib.bin buffersize:%d , size:%d\n" , __func__ ,buffersize , ret);
                fclose(fp);
        }
        else
        {
                fprintf(stderr , "yzl add %s , fopen test_lib.bin failed\n" , __func__);
        }
	while(1) {
	ret = sprd_cavdsp_open_device(SPRD_VDSP_WORK_NORMAL , &handle);
	printf("----------open device-------------\n");
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_open_device ret:%d\n" , __func__ ,ret);
	usleep(1000000*3);
	ret = sprd_cavdsp_loadlibrary(&handle , "testlib" , &in);
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_loadlibrary ret:%d\n" , __func__ ,ret);
	ret = sprd_cavdsp_send_cmd(&handle , "testlib" , &in , NULL , NULL , 0 , 1);
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_send_cmd ret:%d\n" , __func__ ,ret);
	ret = sprd_cavdsp_unloadlibrary(&handle , "testlib");
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_unloadlibrary ret:%d\n" , __func__ ,ret);
	ret = sprd_cavdsp_close_device(&handle);

	//__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_close_device ret:%d\n" , __func__ ,ret);
	//ret = sprd_cavdsp_send_cmd(&handle , "testlib" , &in , &out , NULL , 0 , 1);
	//__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_send_cmd after close ret:%d\n" , __func__ ,ret);
	usleep(1000000*2);
	}
	return NULL;
}
void* thread2(__unused void* test)
{
	struct sprd_vdsp_client_inout in,out;
	struct vdsp_handle handle;
	int ret;
	uint32_t size = 0x2000;
	void *inputhandle;
        void *outputhandle;
        void *inviraddr;
        void *outviraddr;
        inputhandle = sprd_alloc_ionmem(size , 0 , &in.fd , &inviraddr);
        outputhandle = sprd_alloc_ionmem(size , 0 , &out.fd , &outviraddr);
	in.size = size;
        out.size = size;
        if(inputhandle != NULL)
        {
                memset(inviraddr , 0xdd , size);
        }
        if(outputhandle != NULL)
        {
                memset(outviraddr , 0xee , size);
        }
	while(1) {
        ret = sprd_cavdsp_open_device(SPRD_VDSP_WORK_NORMAL , &handle);
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_open_device ret:%d\n" , __func__ ,ret);
//        sprd_cavdsp_loadlibrary("testlib1" , &in);
        ret = sprd_cavdsp_send_cmd(&handle , "testlib1" , &in , &out , NULL , 0 , 1);
	__android_log_print(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_send_cmd ret:%d\n" , __func__ ,ret);
//        sprd_cavdsp_unloadlibrary("testlib1");
//        sprd_cavdsp_close_device();
	//sprd_cavdsp_close_device(fd);
	usleep(1000000*2);
	}
	return NULL;
}
int main() {
#if 0
		sp<IVdspService> cs = NULL;
	android::ProcessState::initWithDriver("/dev/vndbinder");
		sp < IServiceManager > sm = defaultServiceManager();
		sp < IBinder > binder = sm->getService(String16("service.algorithmservice"));
		cs = interface_cast<IVdspService>(binder);
	cs->openXrpDevice();
	cs->closeXrpDevice();
#else
	android::ProcessState::initWithDriver("/dev/vndbinder");
	struct sprd_vdsp_client_inout in,out;
	pthread_t a ;//,b;
	in.fd = 0;
	in.size = 10;
	out.fd = 0;
	out.size = 10;
#if 1
	pthread_create(&a , NULL , thread1 , NULL);
//	pthread_create(&b , NULL , thread2 , NULL);
#else
	sprd_cavdsp_open_device();
	sprd_cavdsp_open_device();
	sprd_cavdsp_loadlibrary("testlib" , &in);
	sprd_cavdsp_send_cmd("testlib" , &in , &out , NULL , 0 , 1);
	sprd_cavdsp_close_device();
	sprd_cavdsp_unloadlibrary("testlib");
#endif
#endif
	ProcessState::self()->startThreadPool();
                        IPCThreadState::self()->joinThreadPool();
	return 0;
}



