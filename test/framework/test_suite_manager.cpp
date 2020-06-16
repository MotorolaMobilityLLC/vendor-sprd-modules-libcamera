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
#include "json1case.h"
#define LOG_TAG "IT-suiteManager"
#define JSON_1_FILE_PATH "./json1_default.json"
extern map<string, vector<resultData_t> *> gMap_Result;
suiteManager::suiteManager() : m_json1(NULL), m_isParse(false) {
    SuiteFactory *suiteFactory =
        (SuiteFactory *)AbstractFactory::createFactory("suite");
    m_json1 = new stage1json;
}

suiteManager::~suiteManager() {
    if (m_json1)
        delete m_json1;
    if (suiteFactory)
        delete suiteFactory;
}

int suiteManager::ParseSecJson(originData_t originData) {
    int ret = IT_OK;
    SuiteBase *currSuite = NULL;
    string moduleName = originData.moduleName;
    vector<caseid *> caseIDList = originData.caseIDList;
    transform(moduleName.begin(), moduleName.end(), moduleName.begin(),
              ::tolower);

    if (mMap_Suite.find(moduleName.data()) == mMap_Suite.end()) {
        gMap_Result[moduleName.data()] = new vector<resultData_t>;
        mMap_Suite[moduleName.data()] = suiteFactory->createProduct(moduleName);
        if (NULL == mMap_Suite[moduleName.data()]) {
            IT_LOGE("Failed to create suite:%s", moduleName.data());
            return -IT_ERR;
        }
    }
    currSuite = mMap_Suite[moduleName.data()];
    vector<caseid *>::iterator it = caseIDList.begin();
    while (it != caseIDList.end()) {
        if (IT_OK != currSuite->ParseSecJson(*it, &mVec_TotalCase))
            return -IT_ERR;
        it++;
    }
    return ret;
}

int suiteManager::ParseFirstJson(uint32_t caseID) {
    int ret = IT_OK;
    originData_t originData;
    m_curr1CaseID = caseID;

    if (!m_json1)
        return IT_UNKNOWN_FAULT;

    if (!m_isParse) {
        string path = JSON_1_FILE_PATH;
        string context = m_json1->readInputTestFile(path.data());
        m_json1->ParseJson(context);
        IT_LOGI("succeed in initializing json1");
        m_isParse = true;
    }

    IT_LOGD("now, get json1 caseid =%d", caseID);
    order *currOrderElem = NULL;
    casecomm *json1case = m_json1->getCaseAt(caseID);
    if (!json1case)
        return -IT_NO_THIS_CASE;

    for (uint32_t order = 0; order < json1case->orderSize(); order++) {
        currOrderElem = json1case->getOrderAt(order);
        for (uint32_t mod = 0; mod < currOrderElem->moduleSize(); mod++) {
            moduleinfo *currModuleElem = currOrderElem->getModuleAt(mod);
            currModuleElem->setOrder(order);
            originData.moduleName = currModuleElem->getModuleName();
            originData.caseIDList = currModuleElem->getCaseIDList();
            ParseSecJson(originData);
        }
    }
    return ret;
}

int suiteManager::ReadInjectData() {
    int ret = IT_OK;

    casecomm *json1case = m_json1->getCaseAt(m_curr1CaseID);
    vector<injectInfo *>::iterator itor = json1case->m_injectArr.begin();
    while (itor != json1case->m_injectArr.end()) {
        IT_LOGD("frameOrder = %u, path = %s, otherInfo = %s",
                (*itor)->m_frameOrder, (*itor)->m_path.data(),
                (*itor)->m_otherInfo.data());
        itor++;
    }

    return ret;
}

int suiteManager::RunTest() {
    IT_LOGD("start test");
    int ret = -IT_ERR;

    vector<IParseJson *>::iterator itor = mVec_TotalCase.begin();
    while (itor != mVec_TotalCase.end()) {
        string moduleName = (*itor)->m_thisModuleName;
        IT_LOGD("now run %s's case", moduleName.data());
        if (mMap_Suite[moduleName.data()])
            ret = mMap_Suite[moduleName.data()]->Run(*itor);
        if (ret != IT_OK){
            IT_LOGE("Failed case, ret=%d",ret);
            goto exit;
        } else{
            IT_LOGD("Succeeded case");
        }
        itor++;
    }
    IT_LOGD("finish test");
exit:
    return ret;
}

bool suiteManager::SortPriority(IParseJson *pVec_TotalCase1,
                                IParseJson *pVec_TotalCase2) {
    return (pVec_TotalCase1->m_thisCasePriority <
            pVec_TotalCase2->m_thisCasePriority);
}

int suiteManager::SortTest() {
    IT_LOGD("start sort");

    std::sort(mVec_TotalCase.begin(), mVec_TotalCase.end(), SortPriority);
    vector<IParseJson *>::iterator itor = mVec_TotalCase.begin();
    while (itor != mVec_TotalCase.end()) {
        IT_LOGD("sort test case priority=%d", (*itor)->m_thisCasePriority);
        itor++;
    }
    IT_LOGD("finish sort");
    return 0;
}

int suiteManager::RunTests() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int suiteManager::Clear() {
    int ret = IT_OK;
    IT_LOGD("");

    vector<IParseJson *>::iterator itor = mVec_TotalCase.begin();
    while (itor != mVec_TotalCase.end()) {
        string moduleName = (*itor)->m_thisModuleName;
        if (mMap_Suite[moduleName.data()])
            mMap_Suite[moduleName.data()]->Clear(); // release result data
        itor++;
    }
    mVec_TotalCase.clear();

    return ret;
}