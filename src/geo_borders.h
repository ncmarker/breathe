#pragma once

#include "mesh.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct BorderPoint {
  float latitude;
  float longitude;
  glm::vec3 position; // 3D position on sphere
};

struct CountryBorder {
  std::string countryCode;
  std::vector<std::vector<BorderPoint>> rings; // Each ring is a closed loop
};

class GeoBorders {
public:
  // Convert lat/lon to 3D position on unit sphere
  static glm::vec3 latLonToSphere(float lat, float lon, float radius = 1.0f);

  // Load borders from a simple text file (we'll create this format)
  static std::vector<CountryBorder>
  loadBordersFromFile(const std::string &filename);

  // Load borders from GeoJSON file
  static std::vector<CountryBorder>
  loadBordersFromGeoJSON(const std::string &filename);

  // Generate border line vertices for rendering as a Mesh
  static std::vector<Vertex>
  bordersToVertices(const std::vector<CountryBorder> &borders);
};
