
#ifndef __MODULE_WRAPPER_HWSIM_H__
#define __MODULE_WRAPPER_HWSIM_H__
#include "module_wrapper_base.h"


class ModuleWrapperHWSIM: public ModuleWrapperBase
{
public:
    ModuleWrapperHWSIM();
    ~ModuleWrapperHWSIM();
    virtual int InitOps();
    virtual int SetUp();
    virtual int TearDown();
    virtual int Run(IParseJson *Json2);
};

#endif /* __MODULE_WRAPPER_HWSIM_H__ */
