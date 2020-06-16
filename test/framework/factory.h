#ifndef __FACTORY_H__
#define __FACTORY_H__
#include <iostream>
#include <string>

#include "../module/module_wrapper_base.h"
#include "../suite/test_suite_base.h"
using namespace std;

class SuiteFactory {
  public:
    SuiteFactory(){};
    ~SuiteFactory(){};
    SuiteBase *createProduct(string productName);
};

class ModuleFactory {
  public:
    ModuleFactory(){};
    ~ModuleFactory(){};
    ModuleWrapperBase *createProduct(string productName);
};

class AbstractFactory {
  public:
    AbstractFactory(){};
    ~AbstractFactory(){};
    static void *createFactory(string factoryName);
};

#endif /* __MODULE_WRAPPER_MANAGER_H__ */
