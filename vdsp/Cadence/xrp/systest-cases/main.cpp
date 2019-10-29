#include <stdio.h>
#include "SGM_SPRD.h"
#include "SGM_parameter.h"
#include "ImageFormat_Conversion.h"
#include "bmp_io.h"
#include "common.h"
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include "assist.h"
#ifdef BOKEH_TEST
#include "iBokeh.h"
#endif
#ifdef WIN32
#define CRTDBG_MAP_ALLOC
#include<crtdbg.h>
#endif
#ifdef __linux__
#include <dlfcn.h>
#include <pthread.h>
#else
#endif

#include "unistd.h"
using namespace android;
#ifdef __linux__

typedef struct {

	struct depth_init_inputparam *inparam;
	struct depth_init_outputparam *outputinfo;
	weightmap_param *wParams;
	unsigned short *WeightMap;
	int * a_pOutMaptable;
	int * a_pOutMaptable_scale;
	unsigned short *Disparity;
	unsigned char *pInleft;
	unsigned char *pInright;
	gdepth_outparam *gdepth_output;
	int outWidth;
	int outHeight;
	int loopNum;
	int sleepNum;
} vdsp_thread_t;
camalg_assist_lib_api_t CAA_API;

#define CAMALG_ASSIST_LIB	"libsprdcamalgassist.so"

static int bokeh_end = 0;
static int depth_end = 0;
extern int test_hdr(int argc, char **argv);

int sprd_depth_load_api(camalg_assist_lib_api_t *libapi)
{
#ifdef __linux__
	const char *error = 0;
	libapi->dl_handle = dlopen(CAMALG_ASSIST_LIB, RTLD_LAZY);
	if (libapi->dl_handle == 0) {
		error = dlerror();
		printf("open CAMALG ASSIST API failed.error = %s", error);
		return 1;
	}

	libapi->sprd_caa_vdsp_Send = (int (*)(const char *nsid, int priority, void **h_ionmem_list, uint32_t h_ionmem_num))dlsym(libapi->dl_handle, "sprd_caa_vdsp_Send");
	if (!libapi->sprd_caa_vdsp_Send) {
		printf("fail to dlsym sprd_caa_vdsp_Send");
		goto error_dlsym;
	}

	libapi->sprd_caa_ionmem_alloc = (void *(*)(uint32_t size, bool iscache))dlsym(libapi->dl_handle, "sprd_caa_ionmem_alloc");
	if (!libapi->sprd_caa_ionmem_alloc) {
		printf("fail to dlsym sprd_caa_ionmem_alloc");
		goto error_dlsym;
	}

	libapi->sprd_caa_ionmem_free = (int (*)(void *h_ionmem))dlsym(libapi->dl_handle, "sprd_caa_ionmem_free");
	if (!libapi->sprd_caa_ionmem_free) {
		printf("fail to dlsym sprd_caa_ionmem_free");
		goto error_dlsym;
	}

	libapi->sprd_caa_ionmem_get_vaddr = (void *(*)(void *h_ionmem))dlsym(libapi->dl_handle, "sprd_caa_ionmem_get_vaddr");
	if (!libapi->sprd_caa_ionmem_get_vaddr) {
		printf("fail to dlsym sprd_caa_ionmem_get_vaddr");
		goto error_dlsym;
	}

	libapi->sprd_caa_ionmem_get_fd = (int (*)(void *h_ionmem))dlsym(libapi->dl_handle, "sprd_caa_ionmem_get_fd");
	if (!libapi->sprd_caa_ionmem_get_fd) {
		printf("fail to dlsym sprd_caa_ionmem_get_fd");
		goto error_dlsym;
	}

	libapi->ProcessState_initWithDriver = (void (*)(const char *driver))dlsym(libapi->dl_handle, "ProcessState_initWithDriver");
	if (!libapi->ProcessState_initWithDriver) {
		printf("fail to dlsym ProcessState_initWithDriver");
		goto error_dlsym;
	}

	libapi->ProcessState_startThreadPool = (void (*)())dlsym(libapi->dl_handle, "ProcessState_startThreadPool");
	if (!libapi->ProcessState_startThreadPool) {
		printf("fail to dlsym ProcessState_startThreadPool");
		goto error_dlsym;
	}

	libapi->IPCThreadState_joinThreadPool = (void (*)(bool isMain))dlsym(libapi->dl_handle, "IPCThreadState_joinThreadPool");
	if (!libapi->IPCThreadState_joinThreadPool) {
		printf("fail to dlsym IPCThreadState_joinThreadPool");
		goto error_dlsym;
	}
	libapi->IPCThreadState_stopProcess = (void (*)(bool immediate))dlsym(libapi->dl_handle, "IPCThreadState_stopProcess");
	if (!libapi->IPCThreadState_stopProcess) {
		printf("fail to dlsym IPCThreadState_stopProcess");
		goto error_dlsym;
	}

	return 0;
#endif

#ifdef __linux__
error_dlsym:
	error = dlerror();
	printf("error = %s", error);
	dlclose(libapi->dl_handle);
#endif
	return 1;
}
void *vdsp_main_depth(void *arg)
{
	printf("--------------vdsp_main depth start--------------\n");
	vdsp_thread_t *param = (vdsp_thread_t *)arg;
	int ret=0;

	//////preview///////
	param->inparam->online_depthwidth=param->outWidth;//capture output size;   
	param->inparam->online_depthheight=param->outHeight;
	param->inparam->depth_threadNum=1;
	param->inparam->online_threadNum=2;
	void *handle_preview = sprd_depth_vdsp_Init(param->inparam,param->outputinfo,MODE_CAPTURE,MODE_WEIGHTMAP);//sync
	printf("sprd_depth_vdsp_Init end:%p\n",handle_preview);
	if(handle_preview)
	{
		char str[256];
		printf("depth_loop_num:%d\n",param->loopNum);
		for(int i=0;i<param->loopNum;i++)
		{
			ret=sprd_depth_OnlineCalibration(handle_preview,(void *) param->a_pOutMaptable, (void *)param->pInright,(void *)param->pInleft);//get online mapTable for capture
			sprd_depth_OnlineCalibration_postprocess(handle_preview , (void *) param->a_pOutMaptable, (void *) param->a_pOutMaptable_scale);
			ret = sprd_depth_vdsp_Run(handle_preview, (void *)param->WeightMap, param->a_pOutMaptable_scale, (void *)param->pInright,(void *)param->pInleft,param->wParams);//online mapTable pointer set NULL,don't 
			printf("handle_preview0 :%p\n",handle_preview);
			//SaveImage_gray2_u16("/data/output_preview_vdsp.bmp", param->WeightMap, param->inparam->input_width_main, param->inparam->input_height_main, param->inparam->input_width_main);
			sprintf(str,"/data/depthOut_weightMap_%d.yuv",i);
			FILE *fp=fopen(str,"wb");	
			fwrite(param->WeightMap,2,param->inparam->input_width_main*param->inparam->input_height_main,fp);
			fclose(fp);
			printf("handle_preview1 :%p\n",handle_preview);
			usleep(param->sleepNum);
		}
	}
	else
	{
		printf("sprd_depth_vdsp_Init error\n");
	}
	sprd_depth_vdsp_Close(handle_preview);
	depth_end = 1;
	printf("--------------vdsp_depth main end----------------\n");
	return NULL;
}

#ifdef BOKEH_TEST
typedef struct {
	int loopNum;
	int sleepNum;
	InitParams *initParam;
	WeightParams *weightParam;
	WeightParams_vdsp *depthIn_vdsp;
	GraphicBuffer_vdsp *imgIn_vdsp;
	GraphicBuffer_vdsp *imgOut_vdsp;
} vdsp_bokeh_thread_t;

void *vdsp_main_bokeh(void *arg)
{
	printf("----------vdsp_main_bokeh start---------------\n");
	vdsp_bokeh_thread_t *param = (vdsp_bokeh_thread_t *)arg;
	void *handle;
	WeightParams_vdsp *depthIn_vdsp=param->depthIn_vdsp;
	GraphicBuffer_vdsp *imgIn_vdsp=param->imgIn_vdsp;
	GraphicBuffer_vdsp *imgOut_vdsp=param->imgOut_vdsp;

	InitParams *initParam=param->initParam;
	WeightParams *weightParam=param->weightParam;
	int width = initParam->width;
	int height = initParam->height;
	int depthW = initParam->depth_width;
	int depthH = initParam->depth_height;

	iBokehInit_vdsp(&handle, initParam);
	char str[256];
	printf("bokeh_loop_num:%d\n",param->loopNum);
	for(int i=0;i<param->loopNum;i++)
	{
	/*
		sprintf(str,"/data/weightMap_%d.yuv",i);
		FILE *fp=fopen(str,"wb");
		fwrite(weightParam->DisparityImage,2,depthW*depthH,fp);
		fclose(fp);
	*/
		iBokehCreateWeightMap_vdsp(handle, depthIn_vdsp);
		iBokehBlurImage_vdsp(handle, imgIn_vdsp, imgOut_vdsp);
		sprintf(str,"/data/imgOut_vdsp_%d.yuv",i);
		FILE *fp=fopen(str,"wb");
		fwrite(imgOut_vdsp->data,1,width*height*3/2,fp);
		fclose(fp);
		usleep(param->sleepNum);
	}

	iBokehDeinit_vdsp(handle);
	bokeh_end = 1;
	printf("-----------vdsp_main_bokeh end--------------\n");
	return NULL;
}
#endif
#endif

#define READ_OTP_TXT

int test_bokeh_depth(int argc, char **argv)
{
#ifdef CEVA_OPT
	test_vdsp_api();
#else
	FILE* fid;
	int inWidthL,inHeightL,inWidthR,inHeightR,outWidth,outHeight;
	int OTP_Datanum;
	char OTP_Data[5200];

	char *leftfile=argv[5];
	char *rightfile=argv[6];
	char *OTPFilename=argv[7];
	char *OutFile=argv[8];
	char *paramFile=argv[9];
	int sleepNum0,sleepNum1,sleepNum2;
	int loopNum_depth;
	int loopNum_bokeh;
	ImageYUVFormat left_YUVFormat,right_YUVFormat;
	FILE *fp;
#ifdef WIN32
	_CrtDumpMemoryLeaks();
#endif

	int ret=0;
	sscanf(argv[2],"%dx%d",&inWidthL,&inHeightL);
	printf("width=%d,height=%d\n",inWidthL,inHeightL);    
	sscanf(argv[3],"%dx%d",&inWidthR,&inHeightR);
	printf("width=%d,height=%d\n",inWidthR,inHeightR);     
	sscanf(argv[4],"%dx%d",&outWidth,&outHeight);
	printf("width=%d,height=%d\n",outWidth,outHeight);  
	sscanf(argv[10],"%d",&sleepNum0); 
	sscanf(argv[11],"%d",&sleepNum1); 
	sscanf(argv[12],"%d",&sleepNum2);  
	sscanf(argv[13],"%d",&loopNum_depth);
	sscanf(argv[14],"%d",&loopNum_bokeh);   
	printf("sleepNum_thread=%d,sleepNum_depth=%d,sleepNum_bokeh=%d,loopNum_depth=%d,loopNum_bokeh=%d\n",sleepNum0,sleepNum1,sleepNum2,loopNum_depth,loopNum_bokeh);

#ifdef READ_OTP_TXT
	int *OTP_Data_t=(int *)OTP_Data;
	OTP_Data_t[0]=0x5;
	char *OTP_Data_tt=(char *)OTP_Data+4;
	OTP_Data_tt[0]=1;//0;
	OTP_Data_tt[1]=7;//1;//3;//
	OTP_Data_t=(int *)OTP_Data+4;
	fid = fopen(OTPFilename, "r");//OTPFilename
	OTP_Datanum = 0;
	while(!feof(fid))
	{
		fscanf(fid,"%d\n", &OTP_Data_t[OTP_Datanum]);
		OTP_Datanum ++;
	}
	fclose(fid);
#elif defined READ_OTP_BIN
	fid = fopen(OTPFilename, "rb");
	fread(OTP_Data,1808,1,fid);
	fclose(fid);
#endif

	left_YUVFormat=YUV420_NV12;//????nv21
	right_YUVFormat=YUV420_NV12;

#ifdef __linux__
		sprd_depth_load_api(&CAA_API);
#endif

	unsigned char *pInleft=(unsigned char *)malloc(inWidthL * inHeightL *sizeof(unsigned char)*2);
	if(left_YUVFormat == YUV420_NV12) {
		ReadNV21File(leftfile, inWidthL,inHeightL,pInleft);
	}
	else{
		ReadYUV422File(leftfile, inWidthL,inHeightL,pInleft);
	}

	unsigned char *pInright=(unsigned char *)malloc(inWidthR * inHeightR *sizeof(unsigned char)*2);
	if(right_YUVFormat == YUV420_NV12) {
		ReadNV21File(rightfile, inWidthR,inHeightR,pInright);
	}
	else{
		ReadYUV422File(rightfile, inWidthR,inHeightR,pInright);
	}

	char acVersion[256];
	struct depth_init_inputparam inparam;
	struct depth_init_outputparam outputinfo;
	weightmap_param wParams;

	//depth_preview
	inparam.input_width_main=inWidthL;
	inparam.input_height_main=inHeightL;
	inparam.input_width_sub=inWidthR;
	inparam.input_height_sub=inHeightR;
	inparam.output_depthwidth=800;//outWidth;
	inparam.output_depthheight=600;//outHeight;
	inparam.imageFormat_main=left_YUVFormat;
	inparam.imageFormat_sub=right_YUVFormat;
	inparam.online_depthwidth=outWidth;//capture output size;   
	inparam.online_depthheight=outHeight;
	inparam.depth_threadNum=1;
	inparam.online_threadNum=2;
	inparam.potpbuf=OTP_Data;

#ifdef OTP_VERSION_10_2_TEST
	inparam.otpsize=256;
	*((uint8_t *)inparam.potpbuf+255)=1;//isVertical param
#else
	inparam.otpsize=OTP_Datanum*4+16;//
#endif

#ifdef PARAM_INPUT
	inparam.config_param=(char *)malloc(sizeof(struct SGMParamStruct));
	fid=fopen(paramFile,"rb");
	fread(inparam.config_param,sizeof(struct SGMParamStruct),1,fid);
	fclose(fid);
#else
	struct SGMParamStruct input_param;
	input_param.SensorDirection=0;
	input_param.DepthScaleMin=10;
	input_param.DepthScaleMax=200; 

	input_param.CalibInfiniteZeroPt=1;  //The Calibration Zero Point is Infinite or Not, it is from Calibration File
	input_param.SearchRange=32; // Search Range
	input_param.MinDExtendRatio=50; // Min Disparity Search Value Adjust Ratio 
	input_param.inDistance=1500;
	input_param.inRatio=3;
	input_param.outDistance=700;
	input_param.outRatio=60;

	inparam.config_param=(char *)&input_param;
#endif

	wParams.sel_x = inWidthL/2;
	wParams.sel_y = inHeightL/2;
	wParams.F_number = 1;
	wParams.VCM_cur_value=500;//300;//2048;//326;
	wParams.golden_vcm_data.golden_macro=410;
	wParams.golden_vcm_data.golden_infinity=0;
	wParams.golden_vcm_data.golden_distance[7]=10,wParams.golden_vcm_data.golden_distance[8]=15,wParams.golden_vcm_data.golden_distance[9]=30;
	wParams.golden_vcm_data.golden_vcm[7]=608,wParams.golden_vcm_data.golden_vcm[8]=485,wParams.golden_vcm_data.golden_vcm[9]=393;
	int value_table[5]={0,500,250,0,0};

	printf("sprd_depthmap[%s]",acVersion);

	outputinfo.outputsize=800*600*2;
	unsigned short *WeightMap=(unsigned short *)malloc(outputinfo.outputsize);

	int * a_pOutMaptable=(int *)malloc(inparam.online_depthwidth*inparam.online_depthheight*sizeof(int));
	int * a_pOutMaptable_scale=(int *)malloc(inparam.output_depthwidth*inparam.output_depthheight*sizeof(int));//(int *)malloc(inparam.online_depthwidth/2*inparam.online_depthheight/2*sizeof(int));

	{
		wParams.VCM_cur_value=value_table[0];
#ifdef DEBUG
		FILE *fp;
		fp=fopen("a_pOutMaptable.bin","wb");
		fwrite(a_pOutMaptable,1,inparam.online_depthwidth*inparam.online_depthheight*sizeof(int),fp);
		fclose(fp);
#endif
		distanceRet distance;
	}

#if 1
	//depth_capture
	inparam.online_depthwidth=0;//if online_depthwidth set 0, don't calculate online mapTable 
	inparam.online_depthheight=0;
	inparam.online_threadNum=0;
	inparam.potpbuf=OTP_Data;
	inparam.output_depthwidth=outWidth;
	inparam.output_depthheight=outHeight;
	wParams.VCM_cur_value=0;

	unsigned short *Disparity = (unsigned short *)malloc(outputinfo.outputsize);

	gdepth_outparam gdepth_output;
	gdepth_output.confidence_map=(unsigned char *)malloc(outWidth*outHeight*sizeof(unsigned char));
	gdepth_output.depthnorm_data=(unsigned char *)malloc(outWidth*outHeight*sizeof(unsigned char));
	int size=sizeof("49874");

	wParams.VCM_cur_value=value_table[0];
#ifdef SCALE_DEPTH
	scale_result=Disparity+sizeof(struct DisparityImageStruct);

	min_d = 1000;
	max_d = 0;
	for (j = 0; j < (outWidth*outHeight); j ++)
	{
		min_d = MIN2(scale_result[j],min_d);
		max_d = MAX2(scale_result[j],max_d);
	}

	for (j = 0; j < (outWidth*outHeight); j ++)
		scale_result[j] = (200-10) * (scale_result[j] -min_d)/(max_d-min_d)+ 10;
#endif
#ifdef DEBUG
	fp=fopen("capture_depth_final.yuv","wb");
	fwrite(Disparity,1,outputinfo.outputsize,fp);
	fclose(fp);
#endif

#endif
#ifdef ROTATE
	SaveImage_gray(out, Disparity+sizeof(struct DisparityImageStruct),outWidth,outHeight);
#endif
#endif

#ifdef BOKEH_TEST
	WeightParams_vdsp depthIn_vdsp;
	GraphicBuffer_vdsp imgIn_vdsp, imgOut_vdsp;

	InitParams initParam;
	initParam.ClipRatio = 50;
	initParam.DisparitySmoothWinSize = 11;
	initParam.Scalingratio = 4;
	initParam.SmoothWinSize = 11;
	initParam.width = 960;
	initParam.height = 720;
	initParam.depth_width = 800;//324;//800;
	initParam.depth_height = 600;//243;//600;

	WeightParams weightParam;
	weightParam.F_number=1;
	weightParam.sel_x=initParam.width/2;
	weightParam.sel_y=initParam.height/2;
	void *WeightMap_handle = CAA_API.sprd_caa_ionmem_alloc(initParam.depth_width*initParam.depth_height*2, 0);
	void *pInleft_handle = CAA_API.sprd_caa_ionmem_alloc(initParam.width*initParam.height*3/2, 0);
	weightParam.DisparityImage = (WeightParams *)CAA_API.sprd_caa_ionmem_get_vaddr(WeightMap_handle);

	pthread_t t_bokeh;
	vdsp_bokeh_thread_t arg_bokeh;
	imgIn_vdsp.data = (GraphicBuffer *)CAA_API.sprd_caa_ionmem_get_vaddr(pInleft_handle);
	imgIn_vdsp.fd = CAA_API.sprd_caa_ionmem_get_fd(pInleft_handle);
	void *imgOut_handle = CAA_API.sprd_caa_ionmem_alloc(initParam.width*initParam.height*3/2, 0);
	imgOut_vdsp.data = (GraphicBuffer *)CAA_API.sprd_caa_ionmem_get_vaddr(imgOut_handle);
	imgOut_vdsp.fd = CAA_API.sprd_caa_ionmem_get_fd(imgOut_handle);
	depthIn_vdsp.weightParams=&weightParam;
	depthIn_vdsp.fd = CAA_API.sprd_caa_ionmem_get_fd(WeightMap_handle);

	fp=fopen("/data/Main_disp2depth.bin","rb");
	fread(weightParam.DisparityImage,2,initParam.depth_width*initParam.depth_height,fp);
	fclose(fp);
	fp=fopen("/data/Main_960x720.yuv","rb");
	fread(imgIn_vdsp.data,1,initParam.width*initParam.height*3/2,fp);
	fclose(fp);

	arg_bokeh.initParam = &initParam;
	arg_bokeh.weightParam = &weightParam;
	arg_bokeh.depthIn_vdsp = &depthIn_vdsp;
	arg_bokeh.imgIn_vdsp = &imgIn_vdsp;
	arg_bokeh.imgOut_vdsp = &imgOut_vdsp;
	arg_bokeh.loopNum=loopNum_bokeh;
	arg_bokeh.sleepNum=sleepNum2*1000;
#endif

	int32_t fd_rem;
	void **rem;
#ifdef __linux__
	CAA_API.ProcessState_initWithDriver("/dev/vndbinder");
	pthread_t t_depth;
	vdsp_thread_t arg;
	arg.inparam=&inparam;
	arg.outputinfo=&outputinfo;
	arg.wParams=&wParams;
	arg.WeightMap=WeightMap;
	arg.a_pOutMaptable=a_pOutMaptable;
	arg.a_pOutMaptable_scale=a_pOutMaptable_scale;
	arg.Disparity=Disparity;
	arg.pInleft=pInleft;
	arg.pInright=pInright;
	arg.gdepth_output=&gdepth_output;
	arg.outWidth=outWidth;
	arg.outHeight=outHeight;
	arg.loopNum=loopNum_depth;
	arg.sleepNum=sleepNum1*1000;
	bokeh_end = 1;
	depth_end = 1;
	printf("pthread_create start.\n");
	if(loopNum_depth != 0) {
		depth_end = 0;
		pthread_create(&t_depth, NULL, vdsp_main_depth, &arg);
	}
	usleep(sleepNum0*1000);
#ifdef BOKEH_TEST
	if(loopNum_bokeh != 0) {
		bokeh_end = 0;
		pthread_create(&t_bokeh, NULL, vdsp_main_bokeh, &arg_bokeh);
	}
#endif
	printf("test_bokeh_depth end.\n");

#endif

	return 0;

}
#if 1
int main(int argc, char **argv)
{
        int caseid = atoi(argv[1]);
	int exitflag = 0;
	set_testend_flag(0);
        switch(caseid) {
        case 0:
                test_bokeh_depth(argc , argv);
                break;
	case 1:
		test_hdr(argc , argv);
		break;
        default:
                break;
        }
	ProcessState::self()->startThreadPool();

	switch(caseid) {
	case 0:
		while(1) {
			if(depth_end && bokeh_end) {
				set_testend_flag(1);
				break;
			}
			usleep(100000);
		}
		break;
	default:
		break;
	}
	while(1) {
		if(get_testend_flag())
			break;
		usleep(100000);
	}
	printf("----------------------test_vdsp process done---------------------\n");
//	IPCThreadState::self()->joinThreadPool();
	//CAA_API.ProcessState_startThreadPool();
	//CAA_API.IPCThreadState_joinThreadPool(true);
}
#endif
