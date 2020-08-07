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
#include "test_suite_drv.h"
#define LOG_TAG "IT-suiteDrv"
#define THIS_MODULE_NAME "drv"
#define JSON_PATH "./drv.json"
#include "test_drv_isp_parameter.h"

struct host_info_t host_info[ISP_PATH_NUM] = {0};

TestSuiteDRV::TestSuiteDRV()
{
	IT_LOGD("");
	m_Module=GetModuleWrapper(string(THIS_MODULE_NAME));
	m_json2 = new CameraDrvIT;
}

TestSuiteDRV::~TestSuiteDRV()
{
    IT_LOGD("");
}

int TestSuiteDRV::SetUp(void)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}

int TestSuiteDRV::TearDown(void)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}
int TestSuiteDRV::ControlEmulator(IT_SWITCH_T status)
{
    int ret=IT_OK;
    IT_LOGD("");
    return ret;
}

int TestSuiteDRV::Run(IParseJson *Json2)
{
	int ret=IT_OK;
	IT_LOGD(" TestSuiteDRV::Run start \n");
	DrvCaseComm *json2=(DrvCaseComm *)Json2;
	json2->g_host_info = host_info;
	if (0 != isp_path_init(json2) ){
		IT_LOGE("fail to parse cam config parameters\n");
	}
	 if (m_Module)
		ret = m_Module->Run(json2);
	 return ret;
}

int TestSuiteDRV::ParseSecJson(caseid* caseid, vector<IParseJson*>* pVec_TotalCase)
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
		std::vector<DrvCaseComm *>::iterator i;
		/* json2 debug */
		for (i = m_json2->m_casecommArr.begin();
			i != m_json2->m_casecommArr.end(); i++) {
			IT_LOGD("================");
			IT_LOGD("caseID:%d",(*i)->m_caseID);
			IT_LOGD("chipID:%d",(*i)->m_chipID);
			for(int j = 0; j < (*i)->m_pathID.size(); j++)
				IT_LOGD("pathID[%d]:%d\n",j,(*i)->m_pathID[j]);
			IT_LOGD("testMode:%d",(*i)->m_testMode);
			IT_LOGD("parmPath:%s",(*i)->m_parmPath.data());
			IT_LOGD("imagePath:%s",(*i)->m_imageName.data());
		}
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