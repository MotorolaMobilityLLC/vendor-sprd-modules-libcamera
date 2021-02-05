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
#include "test_suite_hwsim.h"
#include <string.h>
#include<stdio.h>
#include<stdlib.h>
#define LOG_TAG "IT-suitehwsim"
#define THIS_MODULE_NAME "hwsim"
#define JSON_PATH "./hwsim.json"

TestSuiteHWSIM::TestSuiteHWSIM()
{
    m_isParsed = false;
    m_Module=GetModuleWrapper(string(THIS_MODULE_NAME));
    m_json2 = new CamerahwsimIT;
}

TestSuiteHWSIM::~TestSuiteHWSIM()
{
    IT_LOGD("");
    if (m_json2)
        delete m_json2;
}

int TestSuiteHWSIM::SetUp(void)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}

int TestSuiteHWSIM::TearDown(void)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}

int TestSuiteHWSIM::ControlEmulator(IT_SWITCH_T status)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}

int TestSuiteHWSIM::Run(IParseJson *Json2)
{
    int ret = -IT_ERR;
    if (m_Module)
        ret = m_Module->Run(Json2);
    return ret;
}

int TestSuiteHWSIM::ParseSecJson(caseid *caseid,
                                     vector<IParseJson*>* pVec_TotalCase) {
    int ret = IT_OK;
    if (!m_isParsed) {
        string path = JSON_PATH;
        string input = m_json2->readInputTestFile(path.data());
        if (input.empty()) {
            IT_LOGD("Failed to read input or empty input: %s", path.data());
            return IT_FILE_EMPTY;
        }
        int exitCode = m_json2->ParseJson(input);
        std::vector<hwsimCaseComm *>::iterator i;

        /*debug: display json2 info */
        IT_LOGD("print hwsim case info");
        for (i = m_json2->m_casecommArr.begin();
             i != m_json2->m_casecommArr.end(); i++) {
            IT_LOGD("*********************************");
            IT_LOGD("case2ID:%d", (*i)->m_caseID);
            IT_LOGD("m_Priority:%d", (*i)->m_Priority);
            IT_LOGD("StreamSize:%d", (*i)->m_StreamArr.size());
            IT_LOGD("MetadataSize:%d", (*i)->m_MetadataArr.size());
            std::vector<hwsim_Metadata *>::iterator itor2 =
                (*i)->m_MetadataArr.begin();
            while (itor2 != (*i)->m_MetadataArr.end()) {
                IT_LOGD("Metadata tag:%s", (*itor2)->m_tagName.data());
                std::vector<hwsim_Val>::iterator itor3 =
                    (*itor2)->mVec_tagVal.begin();
                /* i32 example */
                /*IT_LOGD("tagVal:%d",itor3->i32);itor3++;
                if (itor3!=(*itor2)->mVec_tagVal.end()){
                    IT_LOGD("tagVal:%d",itor3->i32);
                }*/
                itor2++;
            }
        }
        IT_LOGD("*********************************");
        m_isParsed = true;
    }
    IParseJson *caseData = m_json2->getCaseAt(caseid->getID());
    std::vector<hwsimCaseComm *>::iterator itor = m_json2->m_casecommArr.begin();
    while (itor != m_json2->m_casecommArr.end()) {
        if ((*itor)->m_caseID == caseid->getID()) {
            if (caseData) {
                caseData->m_thisCasePriority = (*itor)->m_Priority;
            }
            break;
        }
        itor++;
    }

    if (caseData) {
        caseData->m_thisModuleName = THIS_MODULE_NAME;
        pVec_TotalCase->push_back(caseData);
    } else {
        IT_LOGE("Failed to find this case ID:%d", caseid->getID());
        ret = -IT_ERR;
    }
    return ret;
}