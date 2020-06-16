
#ifndef __MODULE_WRAPPER_OEM_H__
#define __MODULE_WRAPPER_OEM_H__
#include "module_wrapper_base.h"


class ModuleWrapperOEM: public ModuleWrapperBase
{
public:
    ModuleWrapperOEM();
    ~ModuleWrapperOEM();
    virtual int InitOps();
    virtual int SetUp();
    virtual int TearDown();
    virtual int Run(IParseJson *Json2);
};

#endif /* __MODULE_WRAPPER_OEM_H__ */
