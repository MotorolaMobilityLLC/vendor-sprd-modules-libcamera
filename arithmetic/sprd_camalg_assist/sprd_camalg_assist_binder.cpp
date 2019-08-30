#include "sprd_camalg_assist.h"
#include "sprd_camalg_assist_common.h"
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>

using namespace android;

JNIEXPORT void ProcessState_initWithDriver(const char *driver)
{
	ProcessState::initWithDriver(driver);
}

JNIEXPORT void ProcessState_startThreadPool()
{
	ProcessState::self()->startThreadPool();
}

JNIEXPORT void IPCThreadState_joinThreadPool(bool isMain)
{
	IPCThreadState::self()->joinThreadPool(isMain);
}

JNIEXPORT void IPCThreadState_stopProcess(bool immediate)
{
	IPCThreadState::self()->stopProcess(immediate);
}