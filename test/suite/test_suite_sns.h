#ifndef __TEST_SUITE_SNS_H__
#define __TEST_SUITE_SNS_H__
#include "test_suite_base.h"
#include "module_wrapper_sns.h"

class TestSuiteSNS : public SuiteBase
{
public:
	TestSuiteSNS();
	~TestSuiteSNS();
	int ParseSecJson(caseid* caseid, vector<IParseJson*>* pVec_TotalCase);
	int Run(IParseJson *Json2);
	int ControlEmulator(IT_SWITCH_T status);
	int SetUp();
	int TearDown();
	CameraSnsIT* m_json2;
};

#endif /* __TEST_SUITE_SNS_H__ */