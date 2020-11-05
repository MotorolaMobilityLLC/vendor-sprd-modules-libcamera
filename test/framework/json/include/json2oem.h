#ifndef _JSON2OEM_H_
#define _JSON2OEM_H_
#include "IParseJson.h"
#include "gralloc_public.h"
//#include <crtdefs.h>
#include <map>
#include <vector>
using namespace std;

union oem_Val {
    int32_t i32;
    double d;
};

class oem_Metadata : public IParseJson {
  public:
    string m_tagName;
    vector<oem_Val> mVec_tagVal;
    oem_Metadata() {
        m_jsonMethodMap["tagName"] = &oem_Metadata::Set_Tag;
        m_jsonMethodMap["tagVal"] = &oem_Metadata::Set_TagVal;
    }
    virtual ~oem_Metadata() {}
    typedef void (oem_Metadata::*MetadataFunc)(string key, void *value);
    typedef map<string, MetadataFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    void Set_Tag(string strKey, void *value) {
        this->m_tagName = (char *)value;
    }
    void Set_TagVal(string strKey, void *value) {
        oem_Val V;
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
        oem_Val V;
        V.d = value;
        this->mVec_tagVal.push_back(V);
    }
    virtual IParseJson *CreateJsonItem(string strKey) { return this; }
};

class oem_stream : public IParseJson {
  public:
    uint32_t s_width;
    uint32_t s_height;
    uint32_t s_usage;
    uint32_t s_type;
    uint32_t s_format;
    uint32_t s_fps;
    // we can add each other property like loglevel emulator on/off
    // at this class
    oem_stream() {
        m_jsonMethodMap["type"] = &oem_stream::Set_Type;
        m_jsonMethodMap["width"] = &oem_stream::Set_Width;
        m_jsonMethodMap["height"] = &oem_stream::Set_Height;
        m_jsonMethodMap["format"] = &oem_stream::Set_Fromat;
        m_jsonMethodMap["usage"] = &oem_stream::Set_Usage;
        m_jsonMethodMap["fps"] = &oem_stream::Set_Fps;
        m_jsonTypeMap.insert({"default_stream",0});
        m_jsonTypeMap.insert({"preview_stream",1});
        m_jsonTypeMap.insert({"video_stream",2});
        m_jsonTypeMap.insert({"callback_stream",3});
        m_jsonTypeMap.insert({"yuv2_stream",4});
        m_jsonTypeMap.insert({"zsl_prev_stream",5});
        m_jsonTypeMap.insert({"picture_stream",6});
    }
    virtual ~oem_stream() {}
    typedef void (oem_stream::*streamFunc)(string key, void *value);
    typedef map<string, streamFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    map<string, uint32_t> m_jsonTypeMap;
    void Set_Type(string strKey, void *value) {
        string *tmp = (static_cast<string *>(value));
         if(m_jsonTypeMap.find(*tmp) != m_jsonTypeMap.end()) {
            this->s_type = m_jsonTypeMap[*tmp];
        } else {
            IT_LOGE("ERR stream type info");
        }
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
    void Set_Fps(string strKey, void *value) {
        this->s_fps = *(static_cast<uint32_t *>(value));
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
    virtual IParseJson *CreateJsonItem(string strKey) { return this; }
};

class OemCaseComm : public IParseJson {
  public:
    uint32_t m_cameraID;
    std::string m_funcName;
    uint32_t m_dumpNum;
    uint32_t m_time;
    int m_Priority;
    std::vector<oem_stream *> m_StreamArr;
    std::vector<oem_Metadata *> m_MetadataArr;

    OemCaseComm() {
        m_thisModuleName = "oem";
        m_jsonMethodMap["CaseID"] = &OemCaseComm::Set_ID;
        m_jsonMethodMap["Priority"] = &OemCaseComm::Set_Priority;
        m_jsonMethodMap["FuncName"] = &OemCaseComm::Set_Function;
        m_jsonMethodMap["CameraID"] = &OemCaseComm::Set_CameraID;
        m_jsonMethodMap["DumpNum"] = &OemCaseComm::Set_DumpNum;
        m_jsonMethodMap["Time"] = &OemCaseComm::Set_Time;
    }

    virtual ~OemCaseComm() {}
    typedef void (OemCaseComm::*casecommFunc)(string key, void *value);
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
    void Set_DumpNum(string strKey, void *value) {
        this->m_dumpNum = *(static_cast<uint32_t *>(value));
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
            oem_stream *pstreamItem = new oem_stream;
            m_StreamArr.push_back(pstreamItem);
            return pstreamItem;
        }

        if (strKey == "Metadata") {
            oem_Metadata *pMetadataItem = new oem_Metadata;
            m_MetadataArr.push_back(pMetadataItem);
            return pMetadataItem;
        }
        return this;
    }
};

class CameraOemIT : public IParseJson {
  public:
    std::vector<OemCaseComm *> m_casecommArr;

    virtual IParseJson *CreateJsonItem(string strKey) {
        if (strKey == "oemCaseLists") {
            OemCaseComm *pcasecommItem = new OemCaseComm;
            m_casecommArr.push_back(pcasecommItem);
            return pcasecommItem;
        }
        return this;
    }

    virtual IParseJson *getCaseAt(uint32_t caseid) {
        std::vector<OemCaseComm *>::iterator itor = m_casecommArr.begin();
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
