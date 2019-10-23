#include <pthread.h>
#include <utils/Log.h>
#include <ion/ion.h>
#include <sprd_ion.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "IVdspService.h"
#include "vdsp_interface_internal.h"
//#include <sys/mman.h>
//#include <android/log.h>
//#include "MemIon.h"
//#include <cutils/ashmem.h>
using namespace android;

int32_t ionTestAlloc(uint32_t size){
	int32_t shared_fd = -1;
	int32_t ionfd = -1;
	unsigned char *p = NULL;
	int map_fd = -1;
	ion_user_handle_t handle;
	ionfd = open("/dev/ion", O_RDWR);
	printf("\nclient run ionTestAlloc ionfd = %d\n",ionfd);

	int rc = ion_alloc(ionfd, size ,0x1000,ION_HEAP_ID_MASK_SYSTEM ,0,&handle);
	printf("client run ionTestAlloc result = %d\n",rc);

	rc = ion_share(ionfd,handle,&shared_fd);
	printf("client run ionTestAlloc shared_fd = %d, rc:%d\n",shared_fd , rc);
#if 0
	rc = ion_map(shared_fd,handle,size ,PROT_READ|PROT_WRITE,MAP_SHARED,0,&p,&map_fd);
	if (rc){
		printf("call Client  function ionTestInput ion_map FAILED : %s \n",
			strerror(errno));
	  return  -1;
	}
	memset(p, 0xaa , size);
//	printf("client run ionTestInput set content is %s ",p);
	munmap(p, 0x1000);
#else
	p = (unsigned char *)mmap(NULL, size , PROT_READ|PROT_WRITE,
                        MAP_SHARED, shared_fd , 0);
	if (p == MAP_FAILED) {
                __android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"function:%s mmap failed %s\n",__func__ , strerror(errno));
                return -1;
        	
	}
	else {
		memset(p, 0xaa , size);
		__android_log_print(ANDROID_LOG_DEBUG,TAG_Server,"function:%s set buffer p:%p ,size:%x, to 0xaa, %x,%x,%x,%x\n" , __func__ , p , size,
			p[0],p[1],p[size-2],p[size-1]);
//		munmap(p , size);
	}
#endif
//	ion_close(ionfd);
	return shared_fd;
}

void* thread1(void* test)
{
	struct sprd_vdsp_client_inout in,out;
	uint32_t size = 0x1000;
	in.fd = ionTestAlloc(size);
        in.size = size;
        out.fd = ionTestAlloc(size);
        out.size = size;
	int32_t fd;
	while(1) {
	fd = sprd_cavdsp_open_device(SPRD_VDSP_WORK_NORMAL);
	sprd_cavdsp_loadlibrary(fd , "testlib" , &in);
	sprd_cavdsp_send_cmd(fd , "testlib" , &in , &out , NULL , 0 , 1);
	sprd_cavdsp_unloadlibrary(fd , "testlib");
//	sprd_cavdsp_close_device();
	usleep(1000000*2);
	}
	return NULL;
}
void* thread2(void* test)
{
	struct sprd_vdsp_client_inout in,out;
	uint32_t size = 0x2000;
	in.fd = ionTestAlloc(size);
        in.size = size;
        out.fd = ionTestAlloc(size);;
        out.size = size;
	int32_t fd;
	while(1) {
        fd = sprd_cavdsp_open_device(SPRD_VDSP_WORK_NORMAL);
        sprd_cavdsp_loadlibrary(fd , "testlib1" , &in);
        sprd_cavdsp_send_cmd(fd , "testlib1" , &in , &out , NULL , 0 , 1);
//        sprd_cavdsp_unloadlibrary("testlib1");
//        sprd_cavdsp_close_device();
//	sprd_cavdsp_close_device();
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
	struct sprd_vdsp_client_inout in,out;
	pthread_t a ,b;
	in.fd = 0;
	in.size = 10;
	out.fd = 0;
	out.size = 10;
#if 1
//	pthread_create(&a , NULL , thread1 , NULL);
	pthread_create(&b , NULL , thread2 , NULL);
#else
	sprd_cavdsp_open_device();
	sprd_cavdsp_open_device();
	sprd_cavdsp_loadlibrary("testlib" , &in);
	sprd_cavdsp_send_cmd("testlib" , &in , &out , NULL , 0 , 1);
	sprd_cavdsp_close_device();
	sprd_cavdsp_unloadlibrary("testlib");
#endif
#endif

	while(1);
	return 0;
}



