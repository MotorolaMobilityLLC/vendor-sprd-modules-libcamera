#define LOG_TAG "Cam3Factory"

#include <fstream>
#include <sstream>
#include <log/log_main.h>
#include <SprdCamera3Factory.h>

#include "Configurator.h"

using namespace sprdcamera;
using namespace std;

const char *Configurator::kFeatureSingleCamera = "single-camera";
const char *Configurator::kFeatureSmoothZoom = "smooth-zoom";
const char *Configurator::kFeatureBokehCamera = "bokeh-camera";
const char *Configurator::kFeatureLogicalMultiCamera = "multi-camera";

static string kCameraConfig = "/vendor/etc/camera.config";

vector<shared_ptr<Configurator>> Configurator::getConfigurators() {
    // TODO try get Configurator from sensor first

    return parseFromFile(kCameraConfig);
}

vector<shared_ptr<Configurator>> Configurator::parseFromFile(string filename) {
    vector<shared_ptr<Configurator>> configs;
    ifstream is(filename);
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(is, root, false)) {
        for (auto &camera : root) {
            auto c = createInstance(camera);
            if (c)
                configs.push_back(c);
        }

        ALOGD("finish parsing %zu camera(s)", configs.size());
    } else {
        ALOGE("fail to parse '%s'", filename.c_str());
    }

    is.close();

    return configs;
}

shared_ptr<Configurator>
Configurator::createInstance(const Json::Value &value) {
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

    auto config =
        shared_ptr<Configurator>(new Configurator(type, sensorIds, feature));

    ALOGD("parsed camera \"%s\" using sensor %s", type.c_str(),
          config->getSensorIdsAsString().c_str());

    // TODO feature is k-v pair for now, change code below if impl changed
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
    shared_ptr<Configurator> c =
        shared_ptr<Configurator>(new Configurator(type, sensorIds));
    c->setCameraId(cameraId);
    c->setPhysicalIds(physicalIds);
    return c;
}

shared_ptr<Configurator> Configurator::createInstance(int cameraId,
                                                      int sensorId) {
    shared_ptr<Configurator> c = shared_ptr<Configurator>(
        new Configurator(string(kFeatureSingleCamera), {sensorId}));
    c->setCameraId(cameraId);
    c->setPhysicalIds({cameraId});
    return c;
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

void Configurator::setCameraId(int cameraId) { mCameraId = cameraId; }

void Configurator::setPhysicalIds(const vector<int> &physicalIds) {
    mPhysicalIds.assign(physicalIds.begin(), physicalIds.end());
}

shared_ptr<Configurator> Configurator::clone() const {
    shared_ptr<Configurator> c = shared_ptr<Configurator>(
        new Configurator(mType, mSensorIds, mFeatures));
    c->setCameraId(mCameraId);
    c->setPhysicalIds(mPhysicalIds);

    return c;
}

Configurator::Configurator(const string &type, const vector<int> &sensorIds)
    : mCameraId(-1), mType(type), mSensorIds(sensorIds) {}

Configurator::Configurator(const string &type, const vector<int> &sensorIds,
                           const Json::Value &features)
    : mCameraId(-1), mType(type), mSensorIds(sensorIds), mFeatures(features) {}
