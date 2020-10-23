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
        m_jsonTypeMap.insert({"default_stream",0});
        m_jsonTypeMap.insert({"preview_stream",1});
        m_jsonTypeMap.insert({"video_stream",2});
        m_jsonTypeMap.insert({"callback_stream",3});
        m_jsonTypeMap.insert({"yuv2_stream",4});
        m_jsonTypeMap.insert({"zsl_prev_stream",5});
        m_jsonTypeMap.insert({"picture_stream",6});
        m_jsonFormatMap.insert({"RGBA_8888",1});
        m_jsonFormatMap.insert({"RGBX_8888",2});
        m_jsonFormatMap.insert({"RGB_888",3});
        m_jsonFormatMap.insert({"RGB_565",4});
        m_jsonFormatMap.insert({"BGRA_8888",5});
        m_jsonFormatMap.insert({"YCBCR_422_SP",16});
        m_jsonFormatMap.insert({"YCRCB_420_SP",17});
        m_jsonFormatMap.insert({"YCBCR_422_I",20});
        m_jsonFormatMap.insert({"RGBA_FP16",22});
        m_jsonFormatMap.insert({"RAW16",32});
        m_jsonFormatMap.insert({"BLOB",33});
        m_jsonFormatMap.insert({"IMPLEMENTATION_DEFINED",34});
        m_jsonFormatMap.insert({"YCBCR_420_888",35});
        m_jsonFormatMap.insert({"RAW_OPAQUE",36});
        m_jsonFormatMap.insert({"RAW10",37});
        m_jsonFormatMap.insert({"RAW12",38});
        m_jsonFormatMap.insert({"RGBA_1010102",43});
        m_jsonFormatMap.insert({"Y8",538982489});
        m_jsonFormatMap.insert({"Y16",540422489});
        m_jsonFormatMap.insert({"YV12",842094169});
    }
    virtual ~stream() {}
    typedef void (stream::*streamFunc)(string key, void *value);
    typedef map<string, streamFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    map<string, uint32_t> m_jsonTypeMap;
    map<string, uint32_t> m_jsonFormatMap;
    void Set_Type(string strKey, void *value) {
        string *tmp = (static_cast<string *>(value));
         if(m_jsonTypeMap.find(*tmp) != m_jsonTypeMap.end()) {
            s_type = m_jsonTypeMap[*tmp];
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
        string *tmp = (static_cast<string *>(value));
         if(m_jsonFormatMap.find(*tmp) != m_jsonFormatMap.end()) {
            s_format = m_jsonFormatMap[*tmp];
        } else {
            IT_LOGE("ERR stream format info");
        }
    }
    void Set_Usage(string strKey, void *value) {
        this->s_usage = *(static_cast<uint32_t *>(value));
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

class HalCaseComm : public IParseJson {
  public:
    uint32_t m_cameraID;
    std::string m_funcName;
    uint32_t m_loopNum;
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
        m_jsonMethodMap["LoopNum"] = &HalCaseComm::Set_Loopnum;
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
    void Set_Loopnum(string strKey, void *value) {
        this->m_loopNum = *(static_cast<uint32_t *>(value));
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
