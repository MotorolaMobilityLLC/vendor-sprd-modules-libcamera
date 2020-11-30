#ifndef _JSON2DRVSENSOR_H_
#define _JSON2DRVSENSOR_H_
#include "IParseJson.h"
#include <map>
#include <vector>
using namespace std;

class SnsCaseComm : public IParseJson {
  public:
    uint32_t m_caseID;
    uint32_t m_physicalSensorID;
    string m_funcName;
    vector<int> m_param;

    SnsCaseComm() {
        m_thisModuleName = "sns";
        m_jsonMethodMap["CaseID"] = &SnsCaseComm::Set_ID;
        m_jsonMethodMap["PhysicalSensorID"] =
            &SnsCaseComm::Set_PhysicalSensorID;
        m_jsonMethodMap["FuncName"] = &SnsCaseComm::Set_Function;
        m_jsonMethodMap["Param"] = &SnsCaseComm::Set_Prarm;
        m_thisCasePriority = 0;
    }

    virtual ~SnsCaseComm() {}
    typedef void (SnsCaseComm::*casecommFunc)(string key, void *value);
    typedef map<string, casecommFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_ID(string strKey, void *value) {
        this->m_caseID = *(static_cast<uint32_t *>(value));
    }
    void Set_PhysicalSensorID(string strKey, void *value) {
        this->m_physicalSensorID = *(static_cast<uint32_t *>(value));
    }
    void Set_Function(string strKey, void *value) {
        this->m_funcName = (char *)value;
    }
    void Set_Prarm(string strKey, void *value) {
        this->m_param.push_back(*(static_cast<int *>(value)));
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

class CameraSnsIT : public IParseJson {
public:
	std::vector<SnsCaseComm *> m_casecommArr;

	virtual IParseJson *CreateJsonItem(string strKey) {
	if (strKey == "ImageSensorCaseLists") {
		SnsCaseComm *pcasecommItem = new SnsCaseComm;
		m_casecommArr.push_back(pcasecommItem);
		return pcasecommItem;
	}
	return this;
}

	virtual IParseJson *getCaseAt(uint32_t caseid){
		std::vector<SnsCaseComm*>::iterator itor=m_casecommArr.begin();
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
