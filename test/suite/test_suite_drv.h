
#ifndef __TEST_SUITE_DRV_H__
#define __TEST_SUITE_DRV_H__
#include "test_suite_base.h"
#include "module_wrapper_drv.h"



class TestSuiteDRV : public SuiteBase
{

public:
    TestSuiteDRV();
    ~TestSuiteDRV();
    int ParseSecJson(caseid* caseid, vector<IParseJson*>* pVec_TotalCase);
    int Run(IParseJson *Json2);
    int ControlEmulator(IT_SWITCH_T status);
    int SetUp();
    int TearDown();

};

#endif /* __TEST_SUITE_DRV_H__ */
