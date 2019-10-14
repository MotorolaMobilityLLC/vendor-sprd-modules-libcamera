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

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TestClient"

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
	for(int i = 0;i<6;i++){
	ret = sprd_cavdsp_open_device(SPRD_VDSP_WORK_NORMAL , &handle);
	printf("----------open device-------------\n");
	ALOGD("function:%s , sprd_cavdsp_open_device ret:%d\n" , __func__ ,ret);
	usleep(1000000*3);
	ret = sprd_cavdsp_loadlibrary(&handle , "test_lib" , &in);
	ALOGD("function:%s , sprd_cavdsp_loadlibrary ret:%d\n" , __func__ ,ret);
	ret = sprd_cavdsp_send_cmd(&handle , "testlib" , &in , NULL , NULL , 0 , 1);
	ALOGD("function:%s , sprd_cavdsp_send_cmd ret:%d\n" , __func__ ,ret);
	ret = sprd_cavdsp_unloadlibrary(&handle , "test_lib");
	ALOGD("function:%s , sprd_cavdsp_unloadlibrary ret:%d\n" , __func__ ,ret);
	ret = sprd_cavdsp_close_device(&handle);

	//ALOGD(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_close_device ret:%d\n" , __func__ ,ret);
	//ret = sprd_cavdsp_send_cmd(&handle , "testlib" , &in , &out , NULL , 0 , 1);
	//ALOGD(ANDROID_LOG_DEBUG,TAG_Test,"function:%s , sprd_cavdsp_send_cmd after close ret:%d\n" , __func__ ,ret);
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
	ALOGD("function:%s , sprd_cavdsp_open_device ret:%d\n" , __func__ ,ret);
//        sprd_cavdsp_loadlibrary("testlib1" , &in);
        ret = sprd_cavdsp_send_cmd(&handle , "testlib1" , &in , &out , NULL , 0 , 1);
	ALOGD("function:%s , sprd_cavdsp_send_cmd ret:%d\n" , __func__ ,ret);
//        sprd_cavdsp_unloadlibrary("testlib1");
//        sprd_cavdsp_close_device();
	//sprd_cavdsp_close_device(fd);
	usleep(1000000*2);
	}
	return NULL;
}
typedef struct
{
	int x, y, width, height;
	int yawAngle, pitchAngle;
	int ret;
	unsigned int facepoint_addr;
}FACEID_INFO;
/*
 * Help info from hal.
 * The first part: one int32 is CONTROL_AE_STATE;
 * The second part: one int32 is ae info. Bit 31-16 is the bv value,which is a signed value. 
 *                              Bit 10-1 is the probability of backlight, range[0-1000].
 *                              Bit 0 is whether ae is stable, range[0, 1].
 * The third part: 256 int32 is hist_statistics.
 * The fouth part: one int32 is the phone orientation info.
 */
typedef struct
{
    const int32_t *helpInfo;
    int32_t count;
} FACEID_HELPINFO;

void* thread_faceid(__unused void* test)
{
	struct sprd_vdsp_client_inout in,out,image;
	uint32_t devid = 30;
	uint32_t liveness = 1;
	struct vdsp_handle handle;
	int ret;
	FILE *fp;
	char filename[256];
	void *inputhandle;
	void *outputhandle;
	void *imagehandle;
	FACEID_INFO *face_info;
	int64_t start_time,end_time,duration;
	unsigned long inphyaddr;
	FACEID_IN *faceid_in;
	uint32_t w = 960, h = 720;
	/*camera simulation*/
	image.size = w*h*3/2;
	imagehandle = sprd_alloc_ionmem2(image.size , 0 , &image.fd , &image.viraddr, &inphyaddr);
	image.phy_addr = inphyaddr;

	sprintf(filename , "/vendor/bin/test.yuv" );
    fp = fopen(filename , "rb");
    if(fp) {
            ret = fread(image.viraddr , 1, image.size , fp);
            fprintf(stderr , "%s , fread size:%d\n" , __func__ , ret);
            fclose(fp);
    }
    else
    {
            fprintf(stderr , "fopen test.yuv failed\n");
    }

	/*input ion*/
	in.size = sizeof(FACEID_IN);
	inputhandle = sprd_alloc_ionmem(in.size, 0 , &in.fd , &in.viraddr);
	if(inputhandle != NULL)
	{
		memset(in.viraddr , 0x00 , in.size);
	}

	faceid_in = (FACEID_IN*)in.viraddr;
	faceid_in->height = h;
	faceid_in->width = w;
	faceid_in->liveness = liveness;
	faceid_in->phyaddr = image.phy_addr;
	faceid_in->help_info[0] = 0xFF;
	faceid_in->l_ir_phyaddr = 0x12;
	faceid_in->r_ir_phyaddr = 0x34;
	faceid_in->bgr_phyaddr = 0x56;
	faceid_in->otp_phyaddr = 0x78;

	/*out ion*/
	out.size = sizeof(FACEID_INFO);
	outputhandle = sprd_alloc_ionmem(out.size , 0 , &out.fd , &out.viraddr);
	out.phy_addr = 0;


	if(outputhandle != NULL)
	{
		memset(out.viraddr , 0x00 , out.size);
	}
	/*move up DDR frequency*/
	FILE *dfs_fp = fopen("/sys/class/devfreq/scene-frequency/sprd_governor/scenario_dfs", "wb");
	if (dfs_fp == NULL) {
		printf("fail to open file for DFS: %d ", errno);
	}
	else
	{
		fprintf(dfs_fp, "%s", "faceid");
		fclose(dfs_fp);
		dfs_fp= NULL;
	}

	start_time = systemTime(CLOCK_MONOTONIC);

	ret = sprd_cavdsp_open_device(SPRD_VDSP_WORK_FACEID , &handle);
	end_time = systemTime(CLOCK_MONOTONIC);
	duration = (int)((end_time - start_time)/1000000);
	printf("open take %ld ms\n",duration);

	if(SPRD_VDSP_RESULT_SUCCESS != ret)
	{
		printf("----------open device fail-------------\n");
		sprd_free_ionmem(imagehandle);
		sprd_free_ionmem(inputhandle);
		sprd_free_ionmem(outputhandle);
		return NULL;
	}
	start_time = systemTime(CLOCK_MONOTONIC);

	for(int i = 0;i<devid;i++)
	{
		ret = sprd_cavdsp_send_cmd(&handle , "faceid_fw" , &in , &out , NULL , 0 , 1);
		if (SPRD_XRP_STATUS_SUCCESS != ret)
			fprintf(stderr , "xrp_run_faceid_command failed\n");
		else
		{
			face_info = (FACEID_INFO *)out.viraddr;
			fprintf(stderr ,"vdsp result %d,out addr %X\n",face_info->ret,face_info->facepoint_addr);
			fprintf(stderr ,"x %d y %d w %d h %d yaw %d pitch %d\n",face_info->x,face_info->y,face_info->width,face_info->height,face_info->yawAngle,face_info->pitchAngle);
		}

	}
	end_time = systemTime(CLOCK_MONOTONIC);
	duration = (int)((end_time - start_time)/1000000);
	printf("run %d times take %ld ms\n",devid,duration/devid);
	sprd_cavdsp_close_device(&handle);

	/*move down DDR frequency*/
	dfs_fp = fopen("/sys/class/devfreq/scene-frequency/sprd_governor/exit_scene", "wb");
	if (dfs_fp == NULL) {
		printf("fail to open file for DFS: %d ", errno);
	}
	else
	{
		fprintf(dfs_fp, "%s", "faceid");
		fclose(dfs_fp);
		dfs_fp = NULL;
	}

	sprd_free_ionmem(imagehandle);
	sprd_free_ionmem(inputhandle);
	sprd_free_ionmem(outputhandle);
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
	//pthread_t a ;//,b;
	in.fd = 0;
	in.size = 10;
	out.fd = 0;
	out.size = 10;
#if 1
	//pthread_create(&a , NULL , thread1 , NULL);
//	pthread_create(&b , NULL , thread2 , NULL);
	//thread1(NULL);
	thread_faceid(NULL);
	//thread1(NULL);
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



