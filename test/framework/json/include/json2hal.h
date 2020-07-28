#ifndef _JSON2HAL_H_
#define _JSON2HAL_H_
#include "IParseJson.h"
//#include <crtdefs.h>
#include <map>
#include <vector>
using namespace std;

union Val {
    int32_t i32;
    double d;
};

class Metadata : public IParseJson {
  public:
    string m_tagName;
    vector<Val> mVec_tagVal;
    Metadata() {
        m_jsonMethodMap["tagName"] = &Metadata::Set_Tag;
        m_jsonMethodMap["tagVal"] = &Metadata::Set_TagVal;
    }
    virtual ~Metadata() {}
    typedef void (Metadata::*MetadataFunc)(string key, void *value);
    typedef map<string, MetadataFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_Tag(string strKey, void *value) {
        this->m_tagName = (char *)value;
    }
    void Set_TagVal(string strKey, void *value) {
        Val V;
        V.i32 = *(static_cast<int32_t *>(value));
        this->mVec_tagVal.push_back(V);
    }
    virtual void DealJsonNode(string strNode, string value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)value.c_str());
        }
    }
    virtual void DealJsonNode(string strNode, int value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }
    virtual void DealJsonNode(string strNode, double value) {
        Val V;
        V.d = value;
        this->mVec_tagVal.push_back(V);
    }
    virtual IParseJson *CreateJsonItem(string strKey) { return this; }
};

class stream : public IParseJson {
  public:
    uint32_t s_width;
    uint32_t s_height;
    uint32_t s_usage;
    uint32_t s_type;
    uint32_t s_format;
    // we can add each other property like loglevel emulator on/off
    // at this class
    stream() {
        m_jsonMethodMap["type"] = &stream::Set_Type;
        m_jsonMethodMap["width"] = &stream::Set_Width;
        m_jsonMethodMap["height"] = &stream::Set_Height;
        m_jsonMethodMap["format"] = &stream::Set_Fromat;
        m_jsonMethodMap["usage"] = &stream::Set_Usage;
    }
    virtual ~stream() {}
    typedef void (stream::*streamFunc)(string key, void *value);
    typedef map<string, streamFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_Type(string strKey, void *value) {
        this->s_type = *(static_cast<uint32_t *>(value));
    }
    void Set_Width(string strKey, void *value) {
        this->s_width = *(static_cast<uint32_t *>(value));
    }
    void Set_Height(string strKey, void *value) {
        this->s_height = *(static_cast<uint32_t *>(value));
    }
    void Set_Fromat(string strKey, void *value) {
        this->s_format = *(static_cast<uint32_t *>(value));
    }
    void Set_Usage(string strKey, void *value) {
        this->s_usage = *(static_cast<uint32_t *>(value));
    }
    virtual void DealJsonNode(string strNode, int value) {
        if (m_jsonMethodMap.find(strNode) != m_jsonMethodMap.end()) {
            (this->*m_jsonMethodMap[strNode])(strNode, (void *)&value);
        }
    }
    virtual IParseJson *CreateJsonItem(string strKey) { return this; }
};

class HalCaseComm : public IParseJson {
  public:
    uint32_t m_cameraID;
    std::string m_funcName;
    uint32_t m_frameNum;
    uint32_t m_time;
    int m_Priority;
    std::vector<stream *> m_StreamArr;
    std::vector<Metadata *> m_MetadataArr;

    HalCaseComm() {
        m_thisModuleName = "hal";
        m_jsonMethodMap["CaseID"] = &HalCaseComm::Set_ID;
        m_jsonMethodMap["Priority"] = &HalCaseComm::Set_Priority;
        m_jsonMethodMap["FuncName"] = &HalCaseComm::Set_Function;
        m_jsonMethodMap["CameraID"] = &HalCaseComm::Set_CameraID;
        m_jsonMethodMap["FrameNum"] = &HalCaseComm::Set_Framenum;
        m_jsonMethodMap["Time"] = &HalCaseComm::Set_Time;
    }

    virtual ~HalCaseComm() {}
    typedef void (HalCaseComm::*casecommFunc)(string key, void *value);
    typedef map<string, casecommFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_ID(string strKey, void *value) {
        this->m_caseID = *(static_cast<uint32_t *>(value));
    }
    void Set_Function(string strKey, void *value) {
        this->m_funcName = (char *)value;
    }
    void Set_CameraID(string strKey, void *value) {
        this->m_cameraID = *(static_cast<uint32_t *>(value));
    }
    void Set_Priority(string strKey, void *value) {
        this->m_Priority = *(static_cast<uint32_t *>(value));
    }
    void Set_Framenum(string strKey, void *value) {
        this->m_frameNum = *(static_cast<uint32_t *>(value));
    }
    void Set_Time(string strKey, void *value) {
        this->m_time = *(static_cast<uint32_t *>(value));
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
    virtual IParseJson *CreateJsonItem(string strKey) {
        if (strKey == "StreamList") {
            stream *pstreamItem = new stream;
            m_StreamArr.push_back(pstreamItem);
            return pstreamItem;
        }

        if (strKey == "Metadata") {
            Metadata *pMetadataItem = new Metadata;
            m_MetadataArr.push_back(pMetadataItem);
            return pMetadataItem;
        }
        return this;
    }
};

class CameraHalIT : public IParseJson {
  public:
    std::vector<HalCaseComm *> m_casecommArr;

    virtual IParseJson *CreateJsonItem(string strKey) {
        if (strKey == "halCaseLists") {
            HalCaseComm *pcasecommItem = new HalCaseComm;
            m_casecommArr.push_back(pcasecommItem);
            return pcasecommItem;
        }
        return this;
    }

    virtual IParseJson *getCaseAt(uint32_t caseid) {
        std::vector<HalCaseComm *>::iterator itor = m_casecommArr.begin();
        while (itor != m_casecommArr.end()) {
            if ((*itor)->m_caseID == caseid) {
                return *itor;
            }
            itor++;
        }
        return NULL;
    }
};

#endif
