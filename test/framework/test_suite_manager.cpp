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
#include <unistd.h>
#include <time.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define LOG_TAG "IT-suiteManager"
#define JSON_1_FILE_PATH "./json1_default.json"
#define RESULT_FILE_PATH "/data/vendor/cameraserver/IT_Result_"
#define CHAR_MAX_LEN 255

using namespace std;

extern map<string, vector<resultData_t> *> gMap_Result;
suiteManager::suiteManager() : m_json1(NULL), m_isParse(false), m_ResultFile(NULL) {
    SuiteFactory *suiteFactory =
        (SuiteFactory *)AbstractFactory::createFactory("suite");
    m_json1 = new stage1json;
    m_curr1CaseID = -1;
}

suiteManager::~suiteManager() {
    if (m_json1)
        delete m_json1;
    if (suiteFactory)
        delete suiteFactory;
    if (m_ResultFile){
        IT_LOGD("result save path=%s",m_ResultPath.data());
        fclose(m_ResultFile);
    }
}

int suiteManager::ParseSecJson(originData_t originData) {
    int ret = IT_OK;
    SuiteBase *currSuite = NULL;
    string moduleName = originData.moduleName;
    vector<caseid *> caseIDList = originData.caseIDList;
    transform(moduleName.begin(), moduleName.end(), moduleName.begin(),
              ::tolower);

    /* if module is unexisted, create it */
    if (mMap_Suite.find(moduleName.data()) == mMap_Suite.end()) {
        gMap_Result[moduleName.data()] = new vector<resultData_t>;
        mMap_Suite[moduleName.data()] = suiteFactory->createProduct(moduleName);
        if (NULL == mMap_Suite[moduleName.data()]) {
            IT_LOGE("Failed to create suite:%s!", moduleName.data());
            return -IT_ERR;
        }
    }

    /* load case data to vector */
    currSuite = mMap_Suite[moduleName.data()];
    vector<caseid *>::iterator it = caseIDList.begin();
    while (it != caseIDList.end()) {
        if (IT_OK != currSuite->ParseSecJson(*it, &mVec_TotalCase))
            return -IT_ERR;
        it++;
    }
    return ret;
}

int suiteManager::prepareResultFile(){
    int ret = IT_OK;
    string time;
    char *char255 = (char*)malloc(CHAR_MAX_LEN);
    Now(time);
    strcpy(char255,RESULT_FILE_PATH);strcat(char255,time.data());
    IT_LOGD("result-file path=%s",char255);
    m_ResultFile = fopen(char255, "wb");
    if (!m_ResultFile){
        ret = -IT_FILE_EMPTY;
    }
    m_ResultPath = char255;
    free(char255);
    return ret;
}

int suiteManager::ParseFirstJson(uint32_t caseID) {
    int ret = IT_OK;
    originData_t originData;
    m_curr1CaseID = caseID;

    if (!m_json1)
        return IT_UNKNOWN_FAULT;

    /* load json1 and create resutl-file*/
    if (!m_isParse) {
        string path = JSON_1_FILE_PATH;
        string context = m_json1->readInputTestFile(path.data());
        m_json1->ParseJson(context);
        IT_LOGI("succeed in initializing json1");
        m_isParse = true;
        if (prepareResultFile()){
            IT_LOGD("Failed to creat result-file");
        }
    }

    /* read json1 case [N]
     * parse SecJson according to caseIDList of [N]
     */
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
    IT_LOGD("[%d]: E",m_curr1CaseID);
    int ret = -IT_ERR;
    char* resultStr=(char*)malloc(CHAR_MAX_LEN);
    memset(resultStr,0,CHAR_MAX_LEN);

    vector<IParseJson *>::iterator itor = mVec_TotalCase.begin();
    while (itor != mVec_TotalCase.end()) {
        string moduleName = (*itor)->m_thisModuleName;
        if (mMap_Suite[moduleName.data()]){
            ret = mMap_Suite[moduleName.data()]->Run(*itor);
        }
        if (ret != IT_OK){
            IT_LOGE("[%d]%s_%d: FAILED",m_curr1CaseID,moduleName.data(),(*itor)->getID());
            sprintf(resultStr,"[%d]%s_%d: FAILED\n",m_curr1CaseID,moduleName.data(),(*itor)->getID());
            if (m_ResultFile){
                fwrite(resultStr,strlen(resultStr),1,m_ResultFile);
                IT_LOGD("[%d]: FAILED",m_curr1CaseID);
                sprintf(resultStr,"[%d]: FAILED\n",m_curr1CaseID);
            }
            goto exit;
        } else{
            IT_LOGD("[%d]_%s_%d: PASSED",m_curr1CaseID,moduleName.data(),(*itor)->getID());
            sprintf(resultStr,"[%d]_%s_%d: PASSED\n",m_curr1CaseID,moduleName.data(),(*itor)->getID());
            if (m_ResultFile){
                fwrite(resultStr,strlen(resultStr),1,m_ResultFile);
            }
        }
        itor++;
    }
    IT_LOGD("[%d]: PASSED",m_curr1CaseID);
    if (m_ResultFile){
        sprintf(resultStr,"[%d]: PASSED\n",m_curr1CaseID);
        fwrite(resultStr,strlen(resultStr),1,m_ResultFile);
    }
exit:
    free(resultStr);
    return ret;
}

bool suiteManager::SortPriority(IParseJson *pVec_TotalCase1,
                                IParseJson *pVec_TotalCase2) {
    return (pVec_TotalCase1->m_thisCasePriority <
            pVec_TotalCase2->m_thisCasePriority);
}

int suiteManager::SortTest() {
    IT_LOGD("before sorting");
    int i = 0;
    auto itor = mVec_TotalCase.begin();
    while (itor != mVec_TotalCase.end()) {
        IT_LOGD("%s_%d: %d",(*itor)->m_thisModuleName.data(),(*itor)->m_caseID, i);
        itor++;
        i++;
    }
    std::sort(mVec_TotalCase.begin(), mVec_TotalCase.end(), SortPriority);
    IT_LOGD("after sorting");
    i = 0;
    itor = mVec_TotalCase.begin();
    while (itor != mVec_TotalCase.end()) {
        IT_LOGD("%s_%d: %d",(*itor)->m_thisModuleName.data(),(*itor)->m_caseID, i);
        itor++;
        i++;
    }
    IT_LOGD("X");
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

void suiteManager::Now(string& time){
    struct timeval t;
    char c_t[20] ={0};

    gettimeofday(&t,NULL);
    strftime(c_t,sizeof(c_t),"%Y-%m-%d_%H_%M_%S",localtime(&t.tv_sec));
    time=c_t;
}