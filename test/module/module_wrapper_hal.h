
#ifndef __MODULE_WRAPPER_HAL_H__
#define __MODULE_WRAPPER_HAL_H__
#include "module_wrapper_base.h"


class ModuleWrapperHAL : public ModuleWrapperBase
{
public:
    ModuleWrapperHAL();
    ~ModuleWrapperHAL();
    virtual int InitOps();
    virtual int SetUp();
    virtual int TearDown();
    virtual int Run(IParseJson *Json2);
};

#endif /* __MODULE_WRAPPER_HAL_H__ */
