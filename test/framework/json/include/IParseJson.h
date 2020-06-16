#ifndef _IPARSEJSON_H_
#define _IPARSEJSON_H_
#include "../../test_common_header.h"
#include "json/json.h"
//#include "test_suite_base.h"
#include <string>
using namespace std;
#ifndef BOOL
#define BOOL int
#endif
class IParseJson {

  public:
    IParseJson() {}

    virtual ~IParseJson() {}
/*
    int run(){
      map<string, SuiteBase *> mMap_Suite
    };
*/
    virtual BOOL ParseJson(const std::string &json);

    std::string readInputTestFile(const char *path);

    std::string m_thisModuleName;

    BOOL m_thisCasePriority;

  protected:
    virtual void DealJsonNode(string strNode, string value) {}

    virtual void DealJsonNode(string strNode, int value) {}

    virtual void DealJsonNode(string strNode, unsigned int value) {}

    virtual void DealJsonNode(string strNode, double value) {}

    virtual IParseJson *CreateJsonItem(string strKey);

  private:
    virtual void PrintValueTree(Json::Value &value, IParseJson *pParent,
                                IParseJson *pCurObj, const std::string strkey);
};

#endif
