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

    string getModuleName(){return m_thisModuleName;};

    int getID(){return m_caseID;};

    int getPriority(){return m_thisCasePriority;};

    std::string m_thisModuleName;

    int m_thisCasePriority;

    int m_caseID;
  protected:
    virtual void DealJsonNode(string strNode, string value) {}

    virtual void DealJsonNode(string strNode, int value) {}

    virtual void DealJsonNode(string strNode, unsigned int value) {}

    virtual void DealJsonNode(string strNode, double value) {}

    virtual void DealJsonNode(string strNode, float value) {}

    virtual IParseJson *CreateJsonItem(string strKey);

  private:
    virtual void PrintValueTree(Json::Value &value, IParseJson *pParent,
                                IParseJson *pCurObj, const std::string strkey);


};

#endif
