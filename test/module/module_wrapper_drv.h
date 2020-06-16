
#ifndef __MODULE_WRAPPER_DRV_H__
#define __MODULE_WRAPPER_DRV_H__
#include "module_wrapper_base.h"

class ModuleWrapperDRV : public ModuleWrapperBase
{
public:
    ModuleWrapperDRV();
    ~ModuleWrapperDRV();
    virtual int InitOps();
    virtual int SetUp();
    virtual int TearDown();
    virtual int Run(IParseJson *Json2);
};

#endif /* __MODULE_WRAPPER_DRV_H__ */
