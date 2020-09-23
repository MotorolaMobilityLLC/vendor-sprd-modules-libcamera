#pragma once

#include <memory>
#include <vector>
#include <json/json.h>

namespace sprdcamera {

class Configurator {
  public:
    static const char *kFeatureSingleCamera;
    static const char *kFeatureSmoothZoom;
    static const char *kFeatureBokehCamera;
    static const char *kFeatureLogicalMultiCamera;

    using CfgList = std::vector<std::shared_ptr<Configurator>>;

    static CfgList getConfigurators();
    static std::shared_ptr<Configurator>
    createInstance(int cameraId, const std::string &type,
                   const std::vector<int> &sensorIds,
                   const std::vector<int> &physicalIds);
    /* create instance for single camera */
    static std::shared_ptr<Configurator> createInstance(int cameraId,
                                                        int sensorId);

    Configurator(const int cameraId, const std::string &type,
                 const std::vector<int> &sensorIds,
                 const Json::Value &features = Json::Value(),
                 const std::vector<int> &physicalIds = std::vector<int>());

    int getCameraId() const;
    std::string getType() const;
    std::vector<int> getSensorIds() const;
    std::vector<int> getPhysicalIds() const;
    Json::Value getFeaturesAsJson() const;

    std::string getBrief() const;
    void setPhysicalIds(const std::vector<int> &physicalIds);
    std::shared_ptr<Configurator> clone() const;

  private:
    static bool parseFromFile(CfgList &result);
    static bool parseFromSensor(CfgList &result);
    static std::shared_ptr<Configurator>
    createInstance(const Json::Value &value);

    std::string getSensorIdsAsString() const;

    const int mCameraId;
    const std::string mType;
    const std::vector<int> mSensorIds;
    const Json::Value mFeatures;
    std::vector<int> mPhysicalIds;
};
}
