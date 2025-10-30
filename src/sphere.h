#pragma once

#include "mesh.h"
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

class Sphere {
public:
  // Generate sphere mesh with specified parameters
  static Mesh generate(float radius = 1.0f, unsigned int segments = 32) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Generate vertices
    for (unsigned int y = 0; y <= segments; ++y) {
      for (unsigned int x = 0; x <= segments; ++x) {
        float xSegment = (float)x / (float)segments;
        float ySegment = (float)y / (float)segments;

        float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
        float yPos = std::cos(ySegment * PI);
        float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

        Vertex vertex;
        vertex.position = glm::vec3(xPos, yPos, zPos) * radius;
        vertex.normal = glm::vec3(xPos, yPos, zPos);
        vertex.uv = glm::vec2(xSegment, ySegment);

        vertices.push_back(vertex);
      }
    }

    // Generate indices
    for (unsigned int y = 0; y < segments; ++y) {
      for (unsigned int x = 0; x < segments; ++x) {
        uint32_t first = (y * (segments + 1)) + x;
        uint32_t second = first + segments + 1;

        // First triangle
        indices.push_back(first);
        indices.push_back(second);
        indices.push_back(first + 1);

        // Second triangle
        indices.push_back(second);
        indices.push_back(second + 1);
        indices.push_back(first + 1);
      }
    }

    return Mesh(vertices, indices);
  }

private:
  static constexpr float PI = 3.14159265358979323846f;
};
