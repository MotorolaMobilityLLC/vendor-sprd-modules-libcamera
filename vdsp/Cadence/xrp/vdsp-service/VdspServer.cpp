#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "IVdspService.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "VdspServer"

using namespace android;

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
	ALOGD("addService VdspSrevice:%d\n"  ,service_status);
	
	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();
	
	printf("service run out \n");
	return 0;
}

