#pragma once

#include "co2_data.h"
#include "geo_borders.h"
#include "mesh.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// Configuration for marker appearance
struct MarkerConfig {
  float minRadius = 0.01f;
  float maxRadius = 0.15f;
  float altitude = 0.02f;
  int segments = 24;
  double scaleFactor = 1.0e-10;
};

// Builds circular markers for CO2 emission visualization
class MarkerBuilder {
public:
  MarkerBuilder(const MarkerConfig &config = MarkerConfig());

  std::tuple<Mesh, double, bool> buildMarkers(int year, const CO2DataSet &co2Data,
                                              const std::unordered_map<std::string, glm::vec3> &centroids,
                                              float sphereRadius);

  const MarkerConfig &getConfig() const {
    return config;
  }
  void setConfig(const MarkerConfig &newConfig) {
    config = newConfig;
  }

private:
  MarkerConfig config;

  void createMarker(const glm::vec3 &baseCenter, const glm::vec3 &liftedCenter, const glm::vec3 &normal, float radius,
                    double emission, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, float sphereRadius,
                    float liftedRadius);
};
