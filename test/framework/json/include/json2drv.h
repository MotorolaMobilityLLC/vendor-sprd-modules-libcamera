#ifndef _JSON3DRV_H_
#define _JSON3DRV_H_
#include "IParseJson.h"
#include <map>
#include <vector>
using namespace std;

class DrvCaseComm : public IParseJson {
  public:
    uint32_t m_caseID;
    uint32_t m_chipID;
    uint32_t m_pathID;
    uint32_t m_testMode;
    std::string m_parmPath;
    std::string m_imageName;
    struct host_info_t *g_host_info;

    DrvCaseComm() {
        m_thisModuleName = "drv";
        m_jsonMethodMap["CaseID"] = &DrvCaseComm::Set_ID;
        m_jsonMethodMap["ChipID"] = &DrvCaseComm::Set_ChipID;
        m_jsonMethodMap["PathID"] = &DrvCaseComm::Set_PathID;
        m_jsonMethodMap["TestMode"] = &DrvCaseComm::Set_TestMode;
        m_jsonMethodMap["ParmPath"] = &DrvCaseComm::Set_ParmPath;
        m_jsonMethodMap["ImagePath"] = &DrvCaseComm::Set_ImagePath;
    }

    virtual ~DrvCaseComm() {}
    typedef void (DrvCaseComm::*casecommFunc)(string key, void *value);
    typedef map<string, casecommFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_ID(string strKey, void *value) {
        this->m_caseID = *(static_cast<uint32_t *>(value));
    }
    void Set_ChipID(string strKey, void *value) {
        this->m_chipID = *(static_cast<uint32_t *>(value));
    }
    void Set_PathID(string strKey, void *value) {
        this->m_pathID = *(static_cast<uint32_t *>(value));
    }
    void Set_TestMode(string strKey, void *value) {
        this->m_testMode = *(static_cast<uint32_t *>(value));
    }
    void Set_ParmPath(string strKey, void *value) {
        this->m_parmPath = (char *)value;
    }
    void Set_ImagePath(string strKey, void *value) {
        this->m_imageName = (char *)value;
    }
    virtual void DealJsonNode(string strNode, int value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }
    virtual void DealJsonNode(string strNode, string value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)value.c_str());
        }
    }
    virtual uint32_t getID(){
        return m_caseID;
    }
};

class CameraDrvIT : public IParseJson {
public:
	std::vector<DrvCaseComm *> m_casecommArr;

	virtual IParseJson *CreateJsonItem(string strKey) {
	if (strKey == "drvCaseLists") {
		DrvCaseComm *pcasecommItem = new DrvCaseComm;
		m_casecommArr.push_back(pcasecommItem);
		return pcasecommItem;
	}
	return this;
}

	virtual IParseJson *getCaseAt(uint32_t caseid){
		std::vector<DrvCaseComm*>::iterator itor=m_casecommArr.begin();
		while(itor != m_casecommArr.end()){
			if ((*itor)->m_caseID == caseid){
				return *itor;
			}
			itor++;
		}
		return NULL;
	}

};

#endif
