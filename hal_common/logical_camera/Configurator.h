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

    static std::vector<std::shared_ptr<Configurator>> getConfigurators();
    static std::shared_ptr<Configurator>
    createInstance(int cameraId, const std::string &type,
                   const std::vector<int> &sensorIds,
                   const std::vector<int> &physicalIds);
    /* create instance for single camera */
    static std::shared_ptr<Configurator> createInstance(int cameraId,
                                                        int sensorId);

    int getCameraId() const;
    std::string getType() const;
    std::vector<int> getSensorIds() const;
    std::vector<int> getPhysicalIds() const;
    Json::Value getFeaturesAsJson() const;

    std::string getBrief() const;
    void setCameraId(int cameraId);
    void setPhysicalIds(const std::vector<int> &physicalIds);
    std::shared_ptr<Configurator> clone() const;

  private:
    static std::vector<std::shared_ptr<Configurator>>
    parseFromFile(std::string filename);
    static std::shared_ptr<Configurator>
    createInstance(const Json::Value &value);

    Configurator(const std::string &type, const std::vector<int> &sensorIds);
    Configurator(const std::string &type, const std::vector<int> &sensorIds,
                 const Json::Value &features);

    std::string getSensorIdsAsString() const;

    int mCameraId;
    std::string mType;
    std::vector<int> mSensorIds;
    std::vector<int> mPhysicalIds;
    Json::Value mFeatures;
};
}
