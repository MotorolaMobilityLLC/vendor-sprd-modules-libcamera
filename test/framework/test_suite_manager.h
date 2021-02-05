#ifndef __TEST_SUITE_MANAGER_H__
#define __TEST_SUITE_MANAGER_H__

#define RUN_ALL_CASE 0xFFFF

#include "test_common_header.h"
#include "test_suite_oem.h"
#include "test_suite_drv.h"
#include "test_suite_hal.h"
#include "test_suite_hwsim.h"
#include "compare_func.h"
#include "factory.h"
#include "test_mem_alloc.h"
using namespace std;

typedef struct inject_data {
    uint32_t sensorID;
    uint32_t frameOrder;
    new_ion_mem_t *data;
    uint32_t w;
    uint32_t h;
} inject_data_t;

/*this class is implementation of the testManager class*/
class suiteManager {
  public:
    suiteManager();
    virtual ~suiteManager();
    int ParseFirstJson(uint32_t caseid);
    int ParseSecJson(originData_t originData);
    int TotalTestSuiteCount() const;
    int SuccessfulTestCount() const;
    int FailedTestCount() const;
    int TotalTestCount() const;
    int ReadInjectData();
    int prepareResultFile();
    void Now(string& time);
    SuiteBase *CreateTestSuite(const char *test_suite_name);
    void SetCurrentTestSuite(SuiteBase *a_current_test_suite) {
        m_CurrentSuite = a_current_test_suite;
    }
    // void RegisterFunction(SuiteBase *funcObj);
    //compareInfo_t JpegToYuv(compareInfo_t &img_info);
    vector<new_ion_mem_t> TestGetInjectImg(InjectType_t type);
    int dump_image(compareInfo_t &f_imgInfo);
    int RunTest();
    int RunTests();
    int SortTest();
    static bool SortPriority(IParseJson *pVec_TotalCase1,
                             IParseJson *pVec_TotalCase2);
    int Compare();
    int Clear();

  private:
    SuiteFactory *suiteFactory;
    map<string, SuiteBase *> mMap_Suite;
    stage1json *m_json1;
    bool m_isParse;
    vector<IParseJson *> mVec_TotalCase;
    SuiteBase *m_CurrentSuite;
    int32_t m_curr1CaseID;
    map<uint32_t, new_ion_mem_t *> m_injectImg;
    string m_ResultPath;
    FILE* m_ResultFile;
    TestMemPool* m_InjectBufAlloc;
};

#endif /* __TEST_SUITE_MANAGER_H__ */