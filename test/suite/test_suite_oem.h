
#ifndef __TEST_SUITE_OEM_H__
#define __TEST_SUITE_OEM_H__
#include "test_suite_base.h"
#include "module_wrapper_oem.h"


class TestSuiteOEM : public SuiteBase
{

public:
    TestSuiteOEM();
    ~TestSuiteOEM();
    int ParseSecJson(caseid *caseid, vector<IParseJson*>* pVec_TotalCase);
    int Run(IParseJson *Json2);
    int ControlEmulator(IT_SWITCH_T status);
    int SetUp();
    int TearDown();
    CameraOemIT* m_json2;
};

#endif /* __TEST_SUITE_OEM_H__ */
