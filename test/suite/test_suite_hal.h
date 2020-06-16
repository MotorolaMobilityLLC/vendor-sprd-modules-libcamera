
#ifndef __TEST_SUITE_HAL_H__
#define __TEST_SUITE_HAL_H__
#include "test_suite_base.h"
#include "module_wrapper_hal.h"


class TestSuiteHAL : public SuiteBase
{

public:
    TestSuiteHAL();
    ~TestSuiteHAL();
    int ParseSecJson(caseid* caseid, vector<IParseJson*>* pVec_TotalCase);
    int Run(IParseJson *Json2);
    int ControlEmulator(IT_SWITCH_T status);
    int SetUp();
    int TearDown();
	CameraHalIT* m_json2;
};

#endif /* __TEST_SUITE_HAL_H__ */
