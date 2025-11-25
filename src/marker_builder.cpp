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

  // Build marker geometry
  std::vector<Vertex> markerVertices;
  std::vector<uint32_t> markerIndices;
  markerVertices.reserve(emissions.size() * (config.segments + 1));
  markerIndices.reserve(emissions.size() * config.segments * 3);

  const float liftedRadius = sphereRadius + config.altitude;
  const float angleStep = glm::two_pi<float>() / static_cast<float>(config.segments);

  // Build circular markers for each country
  for (const auto &item : emissions) {
    const auto centroidIt = centroids.find(item.first);
    if (centroidIt == centroids.end())
      continue;

    glm::vec3 basePosition = centroidIt->second;
    if (glm::length(basePosition) <= std::numeric_limits<float>::epsilon())
      continue;

    // Position marker slightly above the sphere surface
    glm::vec3 normal = glm::normalize(basePosition);
    glm::vec3 markerCenter = normal * liftedRadius;

    // Calculate marker size using scalar approach
    float markerRadius = config.minRadius;
    if (item.second > 0.0) {
      double radiusFromEmission = item.second * config.scaleFactor;
      markerRadius = static_cast<float>(config.minRadius + radiusFromEmission);
      markerRadius = std::min(markerRadius, config.maxRadius);
    }

    // Create the marker
    createMarker(markerCenter, normal, markerRadius, item.second, markerVertices, markerIndices);
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

void MarkerBuilder::createMarker(const glm::vec3 &center, const glm::vec3 &normal, float radius, double emission,
                                 std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
  // Create tangent and bitangent vectors for building the circle
  glm::vec3 tangent = glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f));
  if (glm::length(tangent) < 1e-4f) {
    tangent = glm::cross(normal, glm::vec3(1.0f, 0.0f, 0.0f));
  }
  tangent = glm::normalize(tangent);
  glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

  const float angleStep = glm::two_pi<float>() / static_cast<float>(config.segments);

  // Center vertex
  uint32_t startIndex = static_cast<uint32_t>(vertices.size());
  Vertex centerVertex;
  centerVertex.position = center;
  centerVertex.normal = normal;
  float emissionNormalized = static_cast<float>(emission / 1.0e10); // Normalize for shader
  centerVertex.uv = glm::vec2(emissionNormalized, 0.0f);
  vertices.push_back(centerVertex);

  // Create vertices around the circle perimeter
  for (int i = 0; i < config.segments; ++i) {
    float angle = angleStep * static_cast<float>(i);
    glm::vec3 offset = std::cos(angle) * tangent + std::sin(angle) * bitangent;

    Vertex v;
    v.position = center + offset * radius;
    v.normal = normal;
    v.uv = glm::vec2(emissionNormalized, 0.0f);
    vertices.push_back(v);
  }

  // Create triangle indices (fan from center to perimeter)
  for (int i = 1; i <= config.segments; ++i) {
    uint32_t current = startIndex + i;
    uint32_t next = (i == config.segments) ? startIndex + 1 : startIndex + i + 1;
    indices.push_back(startIndex);
    indices.push_back(current);
    indices.push_back(next);
  }
}
