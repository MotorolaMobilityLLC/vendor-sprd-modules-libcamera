#ifndef _JSON2HAL_H_
#define _JSON2HAL_H_
#include "IParseJson.h"
#include "SprdCamera3HALHeader.h"
#include "gralloc_public.h"
//#include <crtdefs.h>
#include <map>
#include <vector>
using namespace std;
using namespace sprdcamera;

union Val {
    int32_t i32;
    double d;
    float f;
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
    virtual void DealJsonNode(string strNode, float value) {
        Val V;
        V.f = value;
        ALOGI("V f is %f", V.f);
        this->mVec_tagVal.push_back(V);
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
        m_jsonTypeMap.insert({"default_stream",CAMERA_STREAM_TYPE_DEFAULT});
        m_jsonTypeMap.insert({"preview_stream",CAMERA_STREAM_TYPE_PREVIEW});
        m_jsonTypeMap.insert({"video_stream",CAMERA_STREAM_TYPE_VIDEO});
        m_jsonTypeMap.insert({"callback_stream",CAMERA_STREAM_TYPE_CALLBACK});
        m_jsonTypeMap.insert({"yuv2_stream",CAMERA_STREAM_TYPE_YUV2});
        m_jsonTypeMap.insert({"zsl_prev_stream",CAMERA_STREAM_TYPE_ZSL_PREVIEW});
        m_jsonTypeMap.insert({"picture_stream",CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT});
        m_jsonFormatMap.insert({"RGBA_8888", HAL_PIXEL_FORMAT_RGBA_8888});
        m_jsonFormatMap.insert({"RGBX_8888", HAL_PIXEL_FORMAT_RGBX_8888});
        m_jsonFormatMap.insert({"RGB_888", HAL_PIXEL_FORMAT_RGB_888});
        m_jsonFormatMap.insert({"RGB_565", HAL_PIXEL_FORMAT_RGB_565});
        m_jsonFormatMap.insert({"BGRA_8888", HAL_PIXEL_FORMAT_BGRA_8888});
        m_jsonFormatMap.insert({"YCBCR_422_SP", HAL_PIXEL_FORMAT_YCBCR_422_SP});
        m_jsonFormatMap.insert({"YCRCB_420_SP", HAL_PIXEL_FORMAT_YCRCB_420_SP});
        m_jsonFormatMap.insert({"YCBCR_422_I", HAL_PIXEL_FORMAT_YCBCR_422_I});
        m_jsonFormatMap.insert({"RGBA_FP16", HAL_PIXEL_FORMAT_RGBA_FP16});
        m_jsonFormatMap.insert({"RAW16", HAL_PIXEL_FORMAT_RAW16});
        m_jsonFormatMap.insert({"BLOB", HAL_PIXEL_FORMAT_BLOB});
        m_jsonFormatMap.insert({"IMPLEMENTATION_DEFINED", HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED});
        m_jsonFormatMap.insert({"YCBCR_420_888", HAL_PIXEL_FORMAT_YCBCR_420_888});
        m_jsonFormatMap.insert({"RAW_OPAQUE", HAL_PIXEL_FORMAT_RAW_OPAQUE});
        m_jsonFormatMap.insert({"RAW10", HAL_PIXEL_FORMAT_RAW10});
        m_jsonFormatMap.insert({"RAW12", HAL_PIXEL_FORMAT_RAW12});
        m_jsonFormatMap.insert({"RGBA_1010102", HAL_PIXEL_FORMAT_RGBA_1010102});
        m_jsonFormatMap.insert({"Y8", HAL_PIXEL_FORMAT_Y8});
        m_jsonFormatMap.insert({"Y16", HAL_PIXEL_FORMAT_Y16});
        m_jsonFormatMap.insert({"YV12", HAL_PIXEL_FORMAT_YV12});
    }
    virtual ~stream() {}
    typedef void (stream::*streamFunc)(string key, void *value);
    typedef map<string, streamFunc> JsonMethodMap;
    JsonMethodMap m_jsonMethodMap;
    map<string, camera_stream_type_t> m_jsonTypeMap;
    map<string, android_pixel_format_t> m_jsonFormatMap;
    void Set_Type(string strKey, void *value) {
        string *tmp = (static_cast<string *>(value));
         if(m_jsonTypeMap.find(*tmp) != m_jsonTypeMap.end()) {
            s_type = (int32_t)m_jsonTypeMap[*tmp];
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
            s_format = (int32_t)m_jsonFormatMap[*tmp];
        } else {
            IT_LOGE("ERR stream format info");
        }
    }
    void Set_Usage(string strKey, void *value) {
        string *tmp = (static_cast<string *>(value));
        int64_t usage = 0;
        sscanf(tmp->c_str(),"%x",&usage);
        s_usage = (uint32_t)usage;
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
