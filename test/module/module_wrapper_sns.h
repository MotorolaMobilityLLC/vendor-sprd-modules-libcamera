#ifndef __MODULE_WRAPPER_SNS_H__
#define __MODULE_WRAPPER_SNS_H__
#include "module_wrapper_base.h"

class ModuleWrapperSNS : public ModuleWrapperBase
{
public:
    ModuleWrapperSNS();
    ~ModuleWrapperSNS();
    virtual int InitOps();
    virtual int SetUp();
    virtual int TearDown();
    virtual int Run(IParseJson *Json2);
};

#endif /* __MODULE_WRAPPER_SNS_H__ */
