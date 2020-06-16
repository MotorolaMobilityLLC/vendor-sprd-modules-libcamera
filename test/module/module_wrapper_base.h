
#ifndef __MODULE_WRAPPER_BASE_H__
#define __MODULE_WRAPPER_BASE_H__
#include "test_common_header.h"
#include "test_mem_alloc.h"
#include "json1case.h"
using namespace std;

typedef struct originData{
    string moduleName;
    vector<caseid*> caseIDList;
    string path;
}originData_t;

typedef struct resultData{
	uint32_t funcID;
	string funcName;
	int32_t ret;
	bool available;
	map<string,void*> data;
}resultData_t;

class ModuleWrapperBase {
	public:
		ModuleWrapperBase();
		virtual ~ModuleWrapperBase();
		virtual int InitOps() = 0;
		virtual int SetUp();
		virtual int TearDown();
		virtual int Run(IParseJson *Json2) =0;
		virtual int WaitData();
		virtual int GetData();
		vector<resultData_t>* pVec_Result;

	private:
		void* ModuleFuncOps;
};

#endif /* __MODULE_WRAPPER_BASE_H__ */
