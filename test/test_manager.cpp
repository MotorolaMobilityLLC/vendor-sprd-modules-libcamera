/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "test_suite_manager.h"
#include <unistd.h>
#define LOG_TAG "IT-testManager"
using namespace std;

typedef enum {
    IT_PARSE_FIRST = 1,
    IT_PARSE_SECOND = 2,
    IT_PARSE_HELP,
    IT_PARSE_MAX
} parse_type_t;

class testManager {
  public:
    // get single instance
    static testManager *GetTestManager();
    void RegisterCase2totalList(SuiteBase *funcObj);
    // run unit test
    bool CheckNum(char *data);
    int Read();
    int Run();
    int Clear();
    int Sort();
    int CmdParse(int argc, char *argv[], originData_t *originData);
    int ParseFirstJson(int caseID);
    int ParseSecJson(originData_t originData);

    suiteManager *impl() { return suitManager; }
    testManager();
    virtual ~testManager();
    suiteManager *suitManager;
    uint32_t m_PassedNum=0,m_FailedNum=0;

    // DISALLOW_COPY_AND_ASSIGN_(testManager);
};

testManager::testManager() { suitManager = new suiteManager(); }

testManager *testManager::GetTestManager() {
    static testManager *const instanse = new testManager();
    return instanse;
}

testManager::~testManager() { delete suitManager; }

int testManager::ParseFirstJson(int caseID) {
    return impl()->ParseFirstJson(caseID);
}

int testManager::ParseSecJson(originData_t originData) {
    return impl()->ParseSecJson(originData);
}

int testManager::Read() { return impl()->ReadInjectData(); }

int testManager::Run() {
    return impl()->RunTest();
}

int testManager::Clear() { return impl()->Clear(); }

int testManager::Sort() {
    return impl()->SortTest();
}

bool testManager::CheckNum(char *data) {
    if (!strcmp(data, "0"))
        return true;

    if (atoi(data) == 0)
        return false;
    else
        return true;
}

int testManager::CmdParse(int argc, char *argv[], originData_t *originData) {
    int ret = IT_OK;

    if (argc == 1)
        return -IT_ERR;

    if (argc == 2 && !strcmp(argv[1], "--help"))
        return IT_PARSE_HELP;

    if (argc == 2)
        return -IT_ERR;

    if (!strcmp(argv[2], "all")) {
        caseid *examID = (caseid *)malloc(sizeof(caseid));
        examID->setID(RUN_ALL_CASE);
        originData->caseIDList.push_back(examID);
        return IT_PARSE_FIRST;
    }

    for (int i = 2; i < argc; i++) {
        if (CheckNum(argv[i]) == false) {
            IT_LOGE("Failed to get ID num, param[%d]=%s", i - 1, argv[i]);
            return -IT_ERR;
        }
    }

    if (!strcmp(argv[1], "json1_default")) {
        ret = IT_PARSE_FIRST;
    } else if (!strcmp(argv[1], "hal") || !strcmp(argv[1], "oem") ||
               !strcmp(argv[1], "drv")) {
        originData->moduleName = argv[1];
        ret = IT_PARSE_SECOND;
    } else {
        IT_LOGE("Fail to get module name:%s", argv[1]);
        return -IT_ERR;
    }

    for (int i = 2; i < argc; i++) {
        caseid *examID = (caseid *)malloc(sizeof(caseid));
        examID->setID((uint32_t)atoi(argv[i]));
        originData->caseIDList.push_back(examID);
    }
    return ret;
}

int body(testManager *p_TestManager, originData_t originData, uint32_t id,
         int cmd) {
    int runResult = -IT_ERR;
    int ret = -IT_ERR;
    originData_t _originData;
    caseid caseid;

    switch (cmd) {
    case IT_PARSE_FIRST:
        ret = p_TestManager->ParseFirstJson(id);
        if (ret != IT_OK) {
            IT_LOGE("Fail to ParseFirstJson, ret = %d", ret);
            goto exit;
        }
        p_TestManager->Read();
        p_TestManager->Sort();
        break;
    case IT_PARSE_SECOND:
        _originData.moduleName = originData.moduleName;
        caseid.setID(id);
        _originData.caseIDList.push_back(&caseid);
        ret = p_TestManager->ParseSecJson(_originData);
        if (ret != IT_OK) {
            IT_LOGE("Fail to ParseSecJson, ret = %d", ret);
            goto exit;
        }
        break;
    default:
        goto exit;
    }
    runResult = p_TestManager->Run();
    p_TestManager->Clear();

    switch(runResult){
    case -IT_NO_THIS_CASE:{
        goto exit;
    }break;
    case IT_OK:{
        p_TestManager->m_PassedNum++;
    }break;
    default:{
        p_TestManager->m_FailedNum++;
    }break;
    }
    return runResult;
exit:
    IT_LOGI("Fail to start this case,stage:%d ,id:%d", cmd, id);
    return ret;
}

int main(int argc, char *argv[]) {
    IT_LOGI("=====camera IT begin!=====");
    std::vector<caseid *>::iterator itor;
    testManager *p_TestManager = testManager::GetTestManager();
    int ret = IT_OK;
    uint32_t id = 0;
    originData_t originData;

    int cmd = p_TestManager->CmdParse(argc, argv, &originData);

    if (cmd < IT_PARSE_FIRST) {
        IT_LOGE("Failed to parse CMD format, pls set \"--help\" to get more "
                "format");
        goto exit;
    }
    if (cmd >= IT_PARSE_HELP) {
        IT_LOGD("CMD example: ./CameraIT json1_default all");
        IT_LOGD("CMD example: ./CameraIT json1_default 0");
        IT_LOGD("CMD example: ./CameraIT hal 0");
        goto exit;
    }

    itor = originData.caseIDList.begin();

    if (itor != originData.caseIDList.end() &&
        (*itor)->getID() == RUN_ALL_CASE) {
        for (uint32_t i = 0; i < RUN_ALL_CASE; i++) {
            if (-IT_NO_THIS_CASE == body(p_TestManager, originData, i, cmd)){
                goto exit;
            }
        }
    } else {
        while (itor != originData.caseIDList.end()) {
            id = (*itor)->getID();
            ret = body(p_TestManager, originData, id, cmd);
            itor++;
        }
    }
exit:
    IT_LOGI("Results summary for test-tag 'cameraIT': %d Tests [%d Passed %d Failed]",
            p_TestManager->m_PassedNum+p_TestManager->m_FailedNum,
            p_TestManager->m_PassedNum,p_TestManager->m_FailedNum);

    IT_LOGI("=====Camera IT completed!=====");
    delete p_TestManager;
    return 0;
}
