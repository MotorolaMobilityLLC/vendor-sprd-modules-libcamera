#include "factory.h"
#include <cstring>
#include "../suite/test_suite_hal.h"
#include "../suite/test_suite_drv.h"
#include "../module/module_wrapper_drv.h"
#include "../module/module_wrapper_hal.h"
#define LOG_TAG "IT-factory"
using namespace std;

SuiteBase *SuiteFactory::createProduct(string productName) {
    transform(productName.begin(), productName.end(), productName.begin(),
              ::tolower);
    if (!strcmp(productName.data(), "hal"))
        return new TestSuiteHAL();
    else if (!strcmp(productName.data(), "drv"))
        return new TestSuiteDRV();
    else
        IT_LOGE("error suite Name=%s", productName.data());
    return NULL;
}

ModuleWrapperBase *ModuleFactory::createProduct(string productName) {
    transform(productName.begin(), productName.end(), productName.begin(),
              ::tolower);
    if (!strcmp(productName.data(), "hal"))
        return new ModuleWrapperHAL();
    else if (!strcmp(productName.data(), "drv"))
        return new ModuleWrapperDRV();
    else
        IT_LOGE("error moduleWrapper Name=%s", productName.data());
    return NULL;
}

void *AbstractFactory::createFactory(string factoryName) {
    if (!strcmp(factoryName.data(), "suite"))
        return new SuiteFactory();
    else if (!strcmp(factoryName.data(), "module"))
        return new ModuleFactory();
    else
        // IT_LOGE("error factoryName=%s",factoryName.data());
        return NULL;
}