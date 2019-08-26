#include "Test.h"
#include "IVdspService.h"
//#include "IcuUtils.h"
int main() {
	printf("service run \n");
	android::status_t service_status;
	android::ProcessState::initWithDriver("/dev/vndbinder");
	printf("service init with driver \n");
	signal(SIGPIPE, SIG_IGN);
	sp < ProcessState > proc(ProcessState::self());
	sp < IServiceManager > sm = defaultServiceManager();
//	InitializeIcuOrDie();
	service_status = sm->addService(String16("service.vdspservice"), new BnVdspService());
	printf("algorithm service add service  \n");
	 __android_log_print(ANDROID_LOG_DEBUG,TAG_Client,"addService VdspSrevice:%d\n"  ,service_status);
	
	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();
	
	printf("service run out \n");
	return 0;
}

