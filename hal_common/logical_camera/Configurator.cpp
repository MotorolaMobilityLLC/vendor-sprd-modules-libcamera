#define LOG_TAG "Cam3Factory"

#include <fstream>
#include <sstream>
#include <cutils/properties.h>
#include <log/log_main.h>
#include <sensor_drv_u.h>
#include <SprdCamera3Factory.h>

#include "Configurator.h"

using namespace sprdcamera;
using namespace std;

const char *Configurator::kFeatureSingleCamera = "single-camera";
const char *Configurator::kFeatureSmoothZoom = "smooth-zoom";
const char *Configurator::kFeatureBokehCamera = "bokeh-camera";
const char *Configurator::kFeatureLogicalMultiCamera = "multi-camera";

static string kCameraConfig = "/vendor/etc/camera.config";

Configurator::CfgList Configurator::getConfigurators() {
    CfgList result;

    /* try get camera config from sensor module */
    if (property_get_bool("persist.vendor.cam.dynamic_id.enable", 0)) {
        if (parseFromSensor(result)) {
            ALOGD("parsed %zu camera(s) from sensor", result.size());
            return result;
        }
    }

    /* try get camera config from config file */
    if (parseFromFile(result)) {
        ALOGD("parsed %zu camera(s) from file", result.size());
        return result;
    }

    return CfgList();
}

bool Configurator::parseFromFile(CfgList &result) {
    ifstream is(kCameraConfig);
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(is, root, false)) {
        for (auto &camera : root) {
            auto c = createInstance(camera);
            if (c)
                result.push_back(c);
        }
    } else {
        ALOGW("fail to parse '%s'", kCameraConfig.c_str());
    }

    is.close();

    return result.size();
}

bool Configurator::parseFromSensor(CfgList &result) {
    static const map<int, string> sTypeMapper = {
        pair<int, string>(MODE_BOKEH /* 12 */, kFeatureBokehCamera),
        pair<int, string>(MODE_MULTI_CAMERA /* 21 */, kFeatureSmoothZoom)};

    logicalCameraInfo *info = nullptr;
    int count = 0;

    count = sensorGetLogicalCameraList(&info);
    if (count <= 0)
        return false;

    for (int i = 0; i < count; i++) {
        if (info[i].logicalCameraId < 0) {
            ALOGE("invalid id %d from sensor module", info[i].logicalCameraId);
            result.clear();
            return false;
        }

        if (info[i].sensorNum < 1 ||
            (info[i].sensorNum > 1 &&
             sTypeMapper.find(info[i].multiCameraMode) == sTypeMapper.cend())) {
            ALOGE("invalid sensor num %d with multi-camera mode %d for camera "
                  "id %d",
                  info[i].sensorNum, info[i].multiCameraMode,
                  info[i].logicalCameraId);
            result.clear();
            return false;
        }

        result.push_back(make_shared<Configurator>(
            info[i].logicalCameraId,
            info[i].sensorNum == 1 ? kFeatureSingleCamera
                                   : sTypeMapper.at(info[i].multiCameraMode),
            vector<int>(info[i].sensorIdGroup,
                        info[i].sensorIdGroup + info[i].sensorNum)));
    }

    return result.size();
}

shared_ptr<Configurator>
Configurator::createInstance(const Json::Value &value) {
    /* camera id */
    if (value["id"].isNull()) {
        ALOGE("need \"id\" for this camera");
        return nullptr;
    }
    int id = value["id"].asInt();

    /* camera type */
    if (value["type"].isNull()) {
        ALOGE("need \"type\" for this camera");
        return nullptr;
    }
    string type = value["type"].asString();

    /* sensor ids */
    if (value["sensor-list"].isNull()) {
        ALOGE("need \"sensor-list\" for this camera");
        return nullptr;
    }
    vector<int> sensorIds;
    for (auto id : value["sensor-list"])
        sensorIds.push_back(id.asInt());

    /* feature */
    Json::Value feature = value["features"];

    auto config = make_shared<Configurator>(id, type, sensorIds, feature);
    ALOGD("parsed camera \"%s\" with id \"%d\" using sensor %s",
          config->getType().c_str(), config->getCameraId(),
          config->getSensorIdsAsString().c_str());

    if (!feature.isNull()) {
        auto member = feature.getMemberNames();
        for (auto &k : member) {
            auto str = feature[k].isConvertibleTo(Json::stringValue)
                           ? feature[k].asString()
                           : string("<object>");
            ALOGD(" > %s: %s", k.c_str(), str.c_str());
        }
    }

    return config;
}

shared_ptr<Configurator>
Configurator::createInstance(int cameraId, const string &type,
                             const vector<int> &sensorIds,
                             const vector<int> &physicalIds) {
    return make_shared<Configurator>(cameraId, type, sensorIds, Json::Value(),
                                     physicalIds);
}

shared_ptr<Configurator> Configurator::createInstance(int cameraId,
                                                      int sensorId) {
    return make_shared<Configurator>(cameraId, string(kFeatureSingleCamera),
                                     vector<int>({sensorId}), Json::Value(),
                                     vector<int>({sensorId}));
}

int Configurator::getCameraId() const { return mCameraId; }

string Configurator::getType() const { return mType; }

vector<int> Configurator::getSensorIds() const { return mSensorIds; }

vector<int> Configurator::getPhysicalIds() const { return mPhysicalIds; }

string Configurator::getSensorIdsAsString() const {
    stringstream s;

    s << "[ ";
    for (int id : mSensorIds)
        s << id << " ";
    s << "]";

    return s.str();
}

Json::Value Configurator::getFeaturesAsJson() const { return mFeatures; }

string Configurator::getBrief() const {
    stringstream s;

    s << "id '" << mCameraId << "', type '" << mType << "', sensor(s) [ ";
    for (int id : mSensorIds)
        s << id << " ";
    s << "]";

    return s.str();
}

void Configurator::setPhysicalIds(const vector<int> &physicalIds) {
    mPhysicalIds.assign(physicalIds.begin(), physicalIds.end());
}

shared_ptr<Configurator> Configurator::clone() const {
    return make_shared<Configurator>(mCameraId, mType, mSensorIds, mFeatures,
                                     mPhysicalIds);
}

Configurator::Configurator(const int cameraId, const string &type,
                           const vector<int> &sensorIds,
                           const Json::Value &features,
                           const vector<int> &physicalIds)
    : mCameraId(cameraId), mType(type), mSensorIds(sensorIds),
      mFeatures(features), mPhysicalIds(physicalIds) {}
