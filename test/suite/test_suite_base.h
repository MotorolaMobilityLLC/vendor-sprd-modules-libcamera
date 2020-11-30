
#ifndef __TEST_SUITE_BASE_H__
#define __TEST_SUITE_BASE_H__

#include "test_common_header.h"
#include "module_wrapper_base.h"
#include "json1case.h"
#include "json2hal.h"
#include "json2drv.h"
#include "json2oem.h"
#include "json2sns.h"

class SuiteBase// : public TestSuiteManager
{
private:
	char *testSuiteName;
	char *testCaseName;
	char *testFullName;
	int result;
	int order;
public:
	SuiteBase();
	virtual ~SuiteBase();
	virtual int ControlEmulator(IT_SWITCH_T status) = 0;
	virtual int Run(IParseJson *Json2) = 0;
	virtual int ParseSecJson(caseid* caseid, vector<IParseJson*>* pVec_TotalCase) = 0;
	virtual int SetUp();
	virtual int TearDown();
	virtual int Clear();

	ModuleWrapperBase* GetModuleWrapper(string moduleName);

	virtual char* GetTestSuiteName() { return 0; };
	virtual char* GetTestCaseName() { return 0; };
	virtual char* GetTestFullName() { return 0; };
	virtual int   GetOrder() { return 0; };
	bool m_isParsed;
	ModuleWrapperBase* m_Module;
};

#endif /* __TEST_SUITE_BASE_H__ */
