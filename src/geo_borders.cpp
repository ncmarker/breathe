#include "geo_borders.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>

glm::vec3 GeoBorders::latLonToSphere(float lat, float lon, float radius) {
  // Convert degrees to radians
  float latRad = glm::radians(lat);
  float lonRad = glm::radians(lon);

  // Convert to 3D cartesian coordinates on a sphere
  // Match the sphere generator's coordinate system
  // lat = -90 to 90 (south to north), lon = -180 to 180 (west to east)
  float x =
      radius * std::cos(latRad) * std::sin(lonRad); // sin for lon (left/right)
  float y = radius * std::sin(latRad);              // sin for lat (up/down)
  float z = radius * std::cos(latRad) *
            std::cos(lonRad); // cos for lon (forward/back)

  return glm::vec3(x, y, z);
}

// convert borders to vertices for rendering
std::vector<Vertex>
GeoBorders::bordersToVertices(const std::vector<CountryBorder> &borders) {
  std::vector<Vertex> vertices;

  for (const auto &country : borders) {
    for (const auto &ring : country.rings) {
      if (ring.size() < 2)
        continue;

      // Create line segments from the ring
      for (size_t i = 0; i < ring.size(); ++i) {
        Vertex v1, v2;
        v1.position = ring[i].position;
        v1.normal = glm::normalize(ring[i].position);
        v1.uv = glm::vec2(0.0f, 0.0f); // Not used for borders

        size_t nextIdx = (i + 1) % ring.size();
        v2.position = ring[nextIdx].position;
        v2.normal = glm::normalize(ring[nextIdx].position);
        v2.uv = glm::vec2(0.0f, 0.0f); // Not used for borders

        vertices.push_back(v1);
        vertices.push_back(v2);
      }
    }
  }

  return vertices;
}

// load country borders from GeoJSON file
std::vector<CountryBorder>
GeoBorders::loadBordersFromGeoJSON(const std::string &filename, float radius) {
  std::vector<CountryBorder> borders;
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cerr << "ERROR: Could not open GeoJSON file: " << filename
              << std::endl;
    return borders;
  }

  nlohmann::json document;
  try {
    file >> document;
  } catch (const nlohmann::json::parse_error &err) {
    std::cerr << "ERROR: Failed to parse GeoJSON '" << filename
              << "': " << err.what() << std::endl;
    return borders;
  }

  if (!document.is_object()) {
    std::cerr << "ERROR: GeoJSON root is not an object in file: " << filename
              << std::endl;
    return borders;
  }

  const std::string type = document.value("type", "");
  if (type != "FeatureCollection") {
    std::cerr << "ERROR: Expected GeoJSON FeatureCollection, got '" << type
              << "' in file: " << filename << std::endl;
    return borders;
  }

  const auto featuresIter = document.find("features");
  if (featuresIter == document.end() || !featuresIter->is_array()) {
    std::cerr << "ERROR: GeoJSON FeatureCollection lacks 'features' array: "
              << filename << std::endl;
    return borders;
  }

  const auto &features = *featuresIter;

  for (const auto &feature : features) {
    if (!feature.is_object())
      continue;

    CountryBorder border;

    if (feature.contains("id") && feature["id"].is_string()) {
      border.countryCode = feature["id"].get<std::string>();
    } else if (feature.contains("properties") &&
               feature["properties"].is_object()) {
      const auto &properties = feature["properties"];
      if (properties.contains("ISO_A3") && properties["ISO_A3"].is_string()) {
        border.countryCode = properties["ISO_A3"].get<std::string>();
      } else if (properties.contains("ADMIN") &&
                 properties["ADMIN"].is_string()) {
        border.countryCode = properties["ADMIN"].get<std::string>();
      }
    }

    auto geometryIter = feature.find("geometry");
    if (geometryIter == feature.end() || geometryIter->is_null())
      continue;

    const auto &geometry = *geometryIter;
    if (!geometry.is_object())
      continue;

    const std::string geomType = geometry.value("type", "");
    const auto coordsIter = geometry.find("coordinates");
    if (coordsIter == geometry.end())
      continue;

    const auto &coordinates = *coordsIter;

    auto addRing = [&](const nlohmann::json &ringArray) {
      if (!ringArray.is_array())
        return;

      std::vector<BorderPoint> ring;
      ring.reserve(ringArray.size());

      for (const auto &point : ringArray) {
        if (!point.is_array() || point.size() < 2)
          continue;

        const double lonDeg = point[0].get<double>();
        const double latDeg = point[1].get<double>();

        BorderPoint borderPoint;
        borderPoint.longitude = static_cast<float>(lonDeg);
        borderPoint.latitude = static_cast<float>(latDeg);
        borderPoint.position =
            latLonToSphere(borderPoint.latitude, borderPoint.longitude, radius);

        ring.push_back(borderPoint);
      }

      if (ring.size() >= 2) {
        border.rings.push_back(std::move(ring));
      }
    };

    if (geomType == "Polygon") {
      if (coordinates.is_array()) {
        for (const auto &ringArray : coordinates) {
          addRing(ringArray);
        }
      }
    } else if (geomType == "MultiPolygon") {
      if (coordinates.is_array()) {
        for (const auto &polygon : coordinates) {
          if (!polygon.is_array())
            continue;
          for (const auto &ringArray : polygon) {
            addRing(ringArray);
          }
        }
      }
    } else {
      std::cerr << "WARNING: Unsupported geometry type '" << geomType
                << "' encountered in GeoJSON file: " << filename << std::endl;
    }

    if (!border.rings.empty()) {
      borders.push_back(std::move(border));
    }
  }

  if (borders.empty()) {
    std::cerr << "WARNING: No borders were parsed from GeoJSON file"
              << std::endl;
  }

  return borders;
}

std::unordered_map<std::string, glm::vec3>
GeoBorders::computeCountryCentroids(const std::vector<CountryBorder> &borders,
                                    float radius) {
  std::unordered_map<std::string, glm::vec3> centroids;
  centroids.reserve(borders.size());

  for (const auto &border : borders) {
    if (border.countryCode.empty())
      continue;

    // For countries with multiple disconnected regions (like USA with
    // Alaska/Hawaii), use the largest ring to compute the centroid to avoid
    // skewing
    const std::vector<BorderPoint> *largestRing = nullptr;
    size_t largestRingSize = 0;

    for (const auto &ring : border.rings) {
      if (ring.size() > largestRingSize) {
        largestRingSize = ring.size();
        largestRing = &ring;
      }
    }

    if (largestRing == nullptr || largestRingSize == 0)
      continue;

    // Compute centroid from the largest ring only
    glm::vec3 sum(0.0f);
    size_t count = 0;

    for (const auto &point : *largestRing) {
      float length = glm::length(point.position);
      if (length <= std::numeric_limits<float>::epsilon())
        continue;

      sum += point.position / length;
      ++count;
    }

    if (count == 0)
      continue;

    glm::vec3 direction = glm::normalize(sum / static_cast<float>(count));

    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
        !std::isfinite(direction.z))
      continue;

    centroids[border.countryCode] = direction * radius;
  }

  return centroids;
}
