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
#include "test_suite_sns.h"
#define LOG_TAG "IT-suiteSns"
#define THIS_MODULE_NAME "sns"
#define JSON_PATH "./sns.json"

TestSuiteSNS::TestSuiteSNS()
{
	IT_LOGD("");
	m_isParsed = false;
	m_Module=GetModuleWrapper(string(THIS_MODULE_NAME));
	m_json2 = new CameraSnsIT;
}

TestSuiteSNS::~TestSuiteSNS()
{
    IT_LOGD("");
	if(m_json2) {
	delete m_json2;
	m_json2 = NULL;
	}
}

int TestSuiteSNS::SetUp(void)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}

int TestSuiteSNS::TearDown(void)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}
int TestSuiteSNS::ControlEmulator(IT_SWITCH_T status)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}

int TestSuiteSNS::Run(IParseJson *Json2) {
    int ret = IT_OK;
    IT_LOGD(" TestSuiteSNS::Run start ");
    if (m_Module)
        ret = m_Module->Run(Json2);
    return ret;
}

int TestSuiteSNS::ParseSecJson(caseid* caseid, vector<IParseJson*>* pVec_TotalCase)
{
	int ret=IT_OK;
	if (!m_isParsed){
		string path= JSON_PATH;
		string input = m_json2->readInputTestFile(path.data());
		if (input.empty()) {
			IT_LOGD("Failed to read input or empty input: %s", path.data());
			return IT_FILE_EMPTY;
		}
		int exitCode = m_json2->ParseJson(input);
		std::vector<SnsCaseComm *>::iterator i;
		/* json2 debug */
		for (i = m_json2->m_casecommArr.begin();
			i != m_json2->m_casecommArr.end(); i++) {
			IT_LOGD("================");
			IT_LOGD("caseID:%d",(*i)->m_caseID);
			IT_LOGD("physicalSensorID: %d", (*i)->m_physicalSensorID);
    		IT_LOGD("funcName: %s", (*i)->m_funcName.c_str());
			for(int j = 0; j < (*i)->m_param.size(); j++){
				IT_LOGD("param[%d]:%d",j,(*i)->m_param[j]);
			}
		}
		m_isParsed=true;
		IT_LOGD("*********************************");
	}
	IParseJson* caseData= m_json2->getCaseAt(caseid->getID());
	if (caseData) {
		caseData->m_thisModuleName=THIS_MODULE_NAME;
		pVec_TotalCase->push_back(caseData);
	} else {
	IT_LOGE("Failed to find this case ID:%d",caseid->getID());
	ret = IT_ERR;
	}
	return ret;
}