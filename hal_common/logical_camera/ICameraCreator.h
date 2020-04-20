#pragma once

#include <memory>
#include <ICameraBase.h>
#include <Configurator.h>

namespace sprdcamera {

struct ICameraCreator {
    virtual std::shared_ptr<ICameraBase>
    createInstance(std::shared_ptr<Configurator> cfg) = 0;
    virtual ~ICameraCreator() = 0;
};

inline ICameraCreator::~ICameraCreator() {}
}
