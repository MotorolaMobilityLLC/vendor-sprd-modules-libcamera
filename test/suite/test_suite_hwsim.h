
#ifndef __TEST_SUITE_HWSIM_H__
#define __TEST_SUITE_HWSIM_H__
#include "test_suite_base.h"
#include "module_wrapper_hwsim.h"


class TestSuiteHWSIM : public SuiteBase
{

public:
    TestSuiteHWSIM();
    ~TestSuiteHWSIM();
    int ParseSecJson(caseid *caseid, vector<IParseJson*>* pVec_TotalCase);
    int Run(IParseJson *Json2);
    int ControlEmulator(IT_SWITCH_T status);
    int SetUp();
    int TearDown();
    CamerahwsimIT* m_json2;
};

#endif /* __TEST_SUITE_OEM_H__ */
