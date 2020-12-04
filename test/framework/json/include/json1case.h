#ifndef _JSON1CASE_H_
#define _JSON1CASE_H_
#include "IParseJson.h"
//#include <crtdefs.h>
#include <map>
#include <vector>
using namespace std;

class caseid : public IParseJson {
  public:
    uint32_t getID() { return m_caseID; }
    uint32_t getOrder() { return m_order; }
    void setID(uint32_t id) { m_caseID = id; }
    uint32_t m_caseID;
    uint32_t m_order;

    caseid() { m_jsonMethodMap["mcaseid"] = &caseid::Set_Id; }
    virtual ~caseid() {}
    typedef void (caseid::*caseidFunc)(string key, void *value);
    typedef map<string, caseidFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_Id(string strKey, void *value) {
        this->m_caseID = *(static_cast<uint32_t *>(value));
    }
    virtual void DealJsonNode(string strNode, int value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }
    virtual IParseJson *CreateJsonItem(string strKey) { return this; }
};

class moduleinfo : public IParseJson {
  public:
    std::string m_name;
    std::vector<caseid *> m_caseid;
    void setOrder(uint32_t order) {
        std::vector<caseid *>::iterator itor = m_caseid.begin();
        while (itor != m_caseid.end()) {
            (*itor)->m_order = order;
            itor++;
        }
    }
    string getModuleName() { return m_name; }
    std::vector<caseid *> getCaseIDList() { return m_caseid; }
    moduleinfo() { m_jsonMethodMap["name"] = &moduleinfo::Set_Name; }
    virtual ~moduleinfo() {}
    typedef void (moduleinfo::*moduleinfoFunc)(string key, void *value);
    typedef map<string, moduleinfoFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_Name(string strKey, void *value) {
        this->m_name = (char *)value;
        transform(this->m_name.begin(), this->m_name.end(),
                  this->m_name.begin(), ::tolower);
    }
    virtual void DealJsonNode(string strNode, string value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)value.c_str());
        }
    }
    virtual IParseJson *CreateJsonItem(string strKey) {
        if (strKey == "mcaseid") {
            caseid *pcaseidItem = new caseid;
            m_caseid.push_back(pcaseidItem);
            return pcaseidItem;
        }
        return this;
    }
    int caseid_count() { return (int)(m_caseid.size()); }
};

class order : public IParseJson {
  public:
    uint32_t m_index;
    uint32_t m_time;
    typedef void (order::*orderFunc)(string key, void *value);
    typedef map<string, orderFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    std::vector<moduleinfo *> moduleList;
    order() {
        m_jsonMethodMap["index"] = &order::Set_Index;
        m_jsonMethodMap["time"] = &order::Set_time;
        // m_jsonMethodMap["logL"] = &order::Set_LogLevel;
        // m_jsonMethodMap["simulator"] = &order::Set_Simulator;
    }
    virtual ~order() {}
    uint32_t getOrderIndex() { return m_index; }
    uint32_t getTime() { return m_time; }

    void Set_Index(string strKey, void *value) {
        this->m_index = *(static_cast<uint32_t *>(value));
    }
    void Set_time(string strKey, void *value) {
        this->m_time = *(static_cast<uint32_t *>(value));
    }

    virtual void DealJsonNode(string strNode, int value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }
    virtual IParseJson *CreateJsonItem(string strKey) {
        if (strKey == "module") {
            moduleinfo *pmoduleItem = new moduleinfo;
            moduleList.push_back(pmoduleItem);
            return pmoduleItem;
        }
        return this;
    }

    int moduleSize() { return (int)(moduleList.size()); }
    moduleinfo *getModuleAt(int index) {
        std::vector<moduleinfo *>::iterator itor = moduleList.begin();
        for (int i = 0; itor != moduleList.end(); itor++) {
            if (i == index)
                return *itor;
            i++;
        }
        return NULL;
    }
};

class injectInfo : public IParseJson {
  public:
    /*uint32_t m_frameOrder;*/
    string m_path;
    uint32_t m_imgWidth;
    uint32_t m_imgHeight;
    int m_injectType;
    int m_imgFormat;
    map<string, int> m_InjectTagMap;
    map<string, int> m_InjectFormatMap;
    /*string m_otherInfo;*/

    injectInfo() {
        /*m_jsonMethodMap["frameOrder"] = &injectInfo::Set_Frame_Order;*/
        m_jsonMethodMap["path"] = &injectInfo::Set_Path;
        m_jsonMethodMap["imgW"] = &injectInfo::Set_img_Width;
        m_jsonMethodMap["imgH"] = &injectInfo::Set_img_Height;
        m_jsonMethodMap["injectType"] = &injectInfo::Set_inject_Type;
        m_jsonMethodMap["imgFormat"] = &injectInfo::Set_img_Format;
        /*m_jsonMethodMap["otherInfo"] = &injectInfo::Set_Other_Info;*/
        m_InjectTagMap.insert({"DCAM_PREV",INJECT_DCAM_PREV});
        m_InjectTagMap.insert({"DCAM_CAP",INJECT_DCAM_CAP});
        m_InjectTagMap.insert({"ISP_PREV",INJECT_ISP_PREV});
        m_InjectTagMap.insert({"ISP_CAP",INJECT_ISP_CAP});
        m_InjectTagMap.insert({"OEM_PREV",INJECT_OEM_PREV});
        m_InjectTagMap.insert({"OEM_CAP",INJECT_OEM_CAP});
        m_InjectTagMap.insert({"HAL_PREV",INJECT_HAL_PREV});
        m_InjectTagMap.insert({"HAL_CAP",INJECT_HAL_CAP});
        m_InjectTagMap.insert({"GOLDEN_DCAM_PREV",INJECT_GOLDEN_DCAM_PREV});
        m_InjectTagMap.insert({"GOLDEN_DCAM_CAP",INJECT_GOLDEN_DCAM_CAP});
        m_InjectTagMap.insert({"GOLDEN_ISP_PREV",INJECT_GOLDEN_ISP_PREV});
        m_InjectTagMap.insert({"GOLDEN_ISP_CAP",INJECT_GOLDEN_ISP_CAP});
        m_InjectTagMap.insert({"GOLDEN_OEM_PREV",INJECT_GOLDEN_OEM_PREV});
        m_InjectTagMap.insert({"GOLDEN_OEM_CAP",INJECT_GOLDEN_OEM_CAP});
        m_InjectTagMap.insert({"GOLDEN_HAL_PREV",INJECT_GOLDEN_HAL_PREV});
        m_InjectTagMap.insert({"GOLDEN_HAL_CAP",INJECT_GOLDEN_HAL_CAP});
        m_InjectFormatMap.insert({"raw", IT_IMG_FORMAT_RAW});
        m_InjectFormatMap.insert({"yuv", IT_IMG_FORMAT_YUV});
        m_InjectFormatMap.insert({"jpg", IT_IMG_FORMAT_JPEG});
    }
    virtual ~injectInfo() {}
    typedef void (injectInfo::*injectInfoFunc)(string key, void *value);
    typedef map<string, injectInfoFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;

    /*void Set_Frame_Order(string strKey, void *value) {
        this->m_frameOrder = *(static_cast<uint32_t *>(value));
    }*/
    void Set_Path(string strKey, void *value) {
        this->m_path = (static_cast<string *>(value)->data());
    }
    void Set_img_Width(string strKey, void *value) {
        this->m_imgWidth = *(static_cast<uint32_t *>(value));
    }
    void Set_img_Height(string strKey, void *value) {
        this->m_imgHeight = *(static_cast<uint32_t *>(value));
    }
    void Set_inject_Type(string strKey, void *value) {
        this->m_injectType = m_InjectTagMap[(static_cast<string *>(value)->data())];
    }
    void Set_img_Format(string strKey, void *value) {
        this->m_imgFormat =  m_InjectFormatMap[(static_cast<string *>(value)->data())];
    }
    /*void Set_Other_Info(string strKey, void *value) {
        this->m_otherInfo = (static_cast<string *>(value)->data());
    }*/

    virtual void DealJsonNode(string strNode, int value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }

    virtual void DealJsonNode(string strNode, string value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }
};

class casecomm : public IParseJson {
  public:
    uint32_t m_caseID;
    std::vector<string> m_simulatorArr;
    std::vector<injectInfo *> m_injectArr;
    std::vector<order *> m_OrderArr;
    casecomm() {
        m_jsonMethodMap["ID"] = &casecomm::Set_ID;
        m_jsonMethodMap["openSimulator"] = &casecomm::Set_Simulator;
    }

    virtual ~casecomm() {}
    typedef void (casecomm::*casecommFunc)(string key, void *value);
    typedef map<string, casecommFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;

    void Set_ID(string strKey, void *value) {
        this->m_caseID = *(static_cast<uint32_t *>(value));
    }

    void Set_Simulator(string strKey, void *value) {
        this->m_simulatorArr.push_back(*(static_cast<string *>(value)));
    }

    virtual void DealJsonNode(string strNode, int value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }

    virtual void DealJsonNode(string strNode, string value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }

    virtual IParseJson *CreateJsonItem(string strKey) {
        if (strKey == "order") {
            order *porderItem = new order;
            m_OrderArr.push_back(porderItem);
            return porderItem;
        } else if (strKey == "injectDataInfo") {
            injectInfo *pInjectItem = new injectInfo;
            m_injectArr.push_back(pInjectItem);
            return pInjectItem;
        }
        return this;
    }

    int orderSize() { return (int)(m_OrderArr.size()); }

    order *getOrderAt(uint32_t orderIndex) {
        std::vector<order *>::iterator itor = m_OrderArr.begin();
        while (itor != m_OrderArr.end()) {
            if ((*itor)->m_index == orderIndex)
                return *itor;
            itor++;
        }
        return NULL;
    }
};

class stage1json : public IParseJson {
  public:
    std::vector<casecomm *> m_casecommArr;
    casecomm *getCaseAt(uint32_t json1ID) {
        std::vector<casecomm *>::iterator itor = m_casecommArr.begin();
        while (itor != m_casecommArr.end()) {
            if ((*itor)->m_caseID == json1ID)
                return *itor;
            itor++;
        }
        return NULL;
    }

    virtual IParseJson *CreateJsonItem(string strKey) {
        if (strKey == "caselists") {
            casecomm *pcasecommItem = new casecomm;
            m_casecommArr.push_back(pcasecommItem);
            return pcasecommItem;
        }
        return this;
    }

    int total_count() { return (int)(m_casecommArr.size()); }
};

#endif
