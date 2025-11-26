#include "marker_builder.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <limits>

MarkerBuilder::MarkerBuilder(const MarkerConfig &config) : config(config) {}

std::tuple<Mesh, double, bool> MarkerBuilder::buildMarkers(int year, const CO2DataSet &co2Data,
                                                           const std::unordered_map<std::string, glm::vec3> &centroids,
                                                           float sphereRadius) {
  double totalEmission = 0.0;
  std::vector<std::pair<std::string, double>> emissions;

  // Collect emissions for all countries that have data for the year
  for (const auto &entry : centroids) {
    auto emission = co2Data.getEmission(entry.first, year);
    if (!emission.has_value())
      continue;

    emissions.emplace_back(entry.first, emission.value());
    totalEmission += emission.value();
  }

  if (emissions.empty()) {
    std::vector<Vertex> emptyVerts;
    std::vector<uint32_t> emptyIndices;
    Mesh emptyMesh(emptyVerts, emptyIndices);
    return {std::move(emptyMesh), 0.0, false};
  }

  std::vector<Vertex> markerVertices;
  std::vector<uint32_t> markerIndices;
  markerVertices.reserve(emissions.size() * (config.segments + 1));
  markerIndices.reserve(emissions.size() * config.segments * 3);

  const float liftedRadius = sphereRadius + config.altitude;

  // Build circular markers for each country
  for (const auto &item : emissions) {
    const auto centroidIt = centroids.find(item.first);
    if (centroidIt == centroids.end())
      continue;

    glm::vec3 basePosition = centroidIt->second;
    if (glm::length(basePosition) <= std::numeric_limits<float>::epsilon())
      continue;

    glm::vec3 normal = glm::normalize(basePosition);
    glm::vec3 baseCenter = normal * sphereRadius;
    glm::vec3 markerCenter = normal * liftedRadius;

    float markerRadius = config.minRadius;
    if (item.second > 0.0) {
      double radiusFromEmission = item.second * config.scaleFactor;
      markerRadius = static_cast<float>(config.minRadius + radiusFromEmission);
      markerRadius = std::min(markerRadius, config.maxRadius);
    }

    createMarker(baseCenter, markerCenter, normal, markerRadius, item.second, markerVertices, markerIndices,
                 sphereRadius, liftedRadius);
  }

  if (markerVertices.empty()) {
    std::vector<Vertex> emptyVerts;
    std::vector<uint32_t> emptyIndices;
    Mesh emptyMesh(emptyVerts, emptyIndices);
    return {std::move(emptyMesh), 0.0, false};
  }

  Mesh markerMesh(markerVertices, markerIndices);
  markerMesh.setupMesh();
  return {std::move(markerMesh), totalEmission, true};
}

void MarkerBuilder::createMarker(const glm::vec3 &baseCenter, const glm::vec3 &liftedCenter, const glm::vec3 &normal,
                                 float radius, double emission, std::vector<Vertex> &vertices,
                                 std::vector<uint32_t> &indices, float sphereRadius, float liftedRadius) {
  glm::vec3 tangent = glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f));
  if (glm::length(tangent) < 1e-4f) {
    tangent = glm::cross(normal, glm::vec3(1.0f, 0.0f, 0.0f));
  }
  tangent = glm::normalize(tangent);
  glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

  const float angleStep = glm::two_pi<float>() / static_cast<float>(config.segments);

  float emissionNormalized = static_cast<float>(emission / 1.0e10);
  emissionNormalized = std::min(1.0f, emissionNormalized);

  float angularRadius = std::asin(radius / liftedRadius);

  int numRings = static_cast<int>(4 + (radius / config.maxRadius) * 4);
  numRings = std::max(2, std::min(numRings, 10));

  uint32_t startIndex = static_cast<uint32_t>(vertices.size());

  for (int ring = 0; ring <= numRings; ++ring) {
    float ringT = static_cast<float>(ring) / static_cast<float>(numRings);
    float ringAngularRadius = angularRadius * ringT;
    float distanceFromCenter = ringT;

    for (int i = 0; i < config.segments; ++i) {
      float angle = angleStep * static_cast<float>(i);
      glm::vec3 tangentDirection = std::cos(angle) * tangent + std::sin(angle) * bitangent;

      glm::vec3 rotationAxis = glm::cross(tangentDirection, normal);
      float axisLength = glm::length(rotationAxis);

      glm::vec3 direction;
      if (ring == 0) {
        direction = normal;
      } else if (axisLength < 1e-4f) {
        direction = normal;
      } else {
        rotationAxis = rotationAxis / axisLength;

        float cosAngle = std::cos(ringAngularRadius);
        float sinAngle = std::sin(ringAngularRadius);
        glm::vec3 crossTerm = glm::cross(rotationAxis, normal);
        float dotTerm = glm::dot(rotationAxis, normal);

        direction = normal * cosAngle + crossTerm * sinAngle + rotationAxis * dotTerm * (1.0f - cosAngle);
        direction = glm::normalize(direction);
      }

      glm::vec3 position = direction * liftedRadius;

      Vertex v;
      v.position = position;
      v.normal = direction;
      v.uv = glm::vec2(emissionNormalized, distanceFromCenter);
      vertices.push_back(v);
    }
  }

  for (int ring = 0; ring < numRings; ++ring) {
    for (int i = 0; i < config.segments; ++i) {
      uint32_t currentRingStart = startIndex + ring * config.segments;
      uint32_t nextRingStart = startIndex + (ring + 1) * config.segments;

      uint32_t v0 = currentRingStart + i;
      uint32_t v1 = currentRingStart + ((i + 1) % config.segments);
      uint32_t v2 = nextRingStart + i;
      uint32_t v3 = nextRingStart + ((i + 1) % config.segments);

      indices.push_back(v0);
      indices.push_back(v2);
      indices.push_back(v1);
      indices.push_back(v1);
      indices.push_back(v2);
      indices.push_back(v3);
    }
  }
}
