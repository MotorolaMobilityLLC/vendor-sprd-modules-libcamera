
#ifndef __MODULE_WRAPPER_BASE_H__
#define __MODULE_WRAPPER_BASE_H__
#include "test_common_header.h"
#include "test_mem_alloc.h"
#include "json1case.h"
using namespace std;

typedef struct originData {
    string moduleName;
    vector<caseid *> caseIDList;
    string path;
} originData_t;

typedef struct resultData {
    uint32_t funcID;
    string funcName;
    int32_t ret;
    bool available;
    map<string, void *> data;
} resultData_t;

typedef struct {
    uint64_t time;
    string tag;
} g_time_t;
extern map<string, g_time_t> g_time;
extern map<string, vector<resultData_t> *> gMap_Result;
class ModuleWrapperBase {
  public:
    ModuleWrapperBase();
    virtual ~ModuleWrapperBase();
    virtual int InitOps() = 0;
    virtual int SetUp();
    virtual int TearDown();
    virtual int Run(IParseJson *Json2) = 0;
    virtual int WaitData();
    virtual int GetData();
    vector<resultData_t> *pVec_Result;

    static void timeFunc(string tagName, int id) {
        if (id == 0) {
            g_time[tagName.data()].time = systemTime();
            g_time[tagName.data()].tag = tagName;
        } else if (id == 1) {
            IT_LOGD("time= %lld ms,tag=%s",
                     ns2ms(systemTime() - g_time[tagName.data()].time), tagName.data());
        }
    }

  private:
    void *ModuleFuncOps;
};
#define PERFORMANCE_MONITOR(x, y)                                              \
    do {                                                                       \
        ModuleWrapperBase::timeFunc(x, 0);                                     \
        y;                                                                     \
        ModuleWrapperBase::timeFunc(x, 1);                                     \
    } while (0)
#endif /* __MODULE_WRAPPER_BASE_H__ */
