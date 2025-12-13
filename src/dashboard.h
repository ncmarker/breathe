#pragma once

#include "mesh.h"
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

class Dashboard {
public:
  // Generate a single ring (for per-ring thickness control)
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateSingleRing(float ringRadius, int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int i = 0; i <= segments; ++i) {
      float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
      float x = ringRadius * std::cos(angle);
      float z = ringRadius * std::sin(angle);
      float y = 0.0f;

      Vertex v;
      v.position = glm::vec3(x, y, z);
      v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
      v.uv = glm::vec2(0.0f, 0.0f);
      vertices.push_back(v);
      indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
    }

    return {vertices, indices};
  }

  // Generate filled disc with radial gradient (for plate center fill)
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateFilledDisc(float innerRadius, float outerRadius,
                                                                                  int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Center vertex
    Vertex center;
    center.position = glm::vec3(0.0f, 0.0f, 0.0f);
    center.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    center.uv = glm::vec2(0.0f, 0.0f); // UV.x = 0.0 = center for gradient
    vertices.push_back(center);
    uint32_t centerIndex = 0;

    // Generate rings from inner to outer radius
    int numRings = 32; // More rings for smoother gradient
    for (int ring = 0; ring <= numRings; ++ring) {
      float t = static_cast<float>(ring) / static_cast<float>(numRings);
      float ringRadius = innerRadius + (outerRadius - innerRadius) * t;
      float distanceFromCenter = t; // For gradient calculation (0.0 = center, 1.0 = edge)

      for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = ringRadius * std::cos(angle);
        float z = ringRadius * std::sin(angle);
        float y = 0.0f;

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v.uv = glm::vec2(distanceFromCenter, 0.0f); // UV.x = distance from center for gradient
        vertices.push_back(v);
      }
    }

    // Generate triangles (fan from center for first ring, then quads between rings)
    // First ring: triangles from center
    for (int i = 0; i < segments; ++i) {
      indices.push_back(centerIndex);
      indices.push_back(1 + i);
      indices.push_back(1 + ((i + 1) % (segments + 1)));
    }

    // Subsequent rings: quads between adjacent rings
    for (int ring = 0; ring < numRings; ++ring) {
      uint32_t ringStart = 1 + ring * (segments + 1);
      uint32_t nextRingStart = 1 + (ring + 1) * (segments + 1);

      for (int i = 0; i < segments; ++i) {
        uint32_t v0 = ringStart + i;
        uint32_t v1 = ringStart + ((i + 1) % (segments + 1));
        uint32_t v2 = nextRingStart + i;
        uint32_t v3 = nextRingStart + ((i + 1) % (segments + 1));

        // First triangle
        indices.push_back(v0);
        indices.push_back(v2);
        indices.push_back(v1);

        // Second triangle
        indices.push_back(v1);
        indices.push_back(v2);
        indices.push_back(v3);
      }
    }

    return {vertices, indices};
  }

  // Generate 3D ring geometry (torus-like) for smooth thick rings without gaps
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generate3DRing(float ringRadius, float thickness,
                                                                              float depth, int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Create rectangular cross-section: inner/outer edges, top/bottom
    float halfThickness = thickness * 0.0005 * 0.5f; // Convert to world space
    float halfDepth = depth * 0.5f;

    for (int seg = 0; seg <= segments; ++seg) {
      float angle = 2.0f * PI * static_cast<float>(seg) / static_cast<float>(segments);

      // Calculate radial direction (from center outward)
      glm::vec3 radial = glm::normalize(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
      glm::vec3 center = radial * ringRadius;

      // Create 4 vertices for rectangular cross-section: inner-top, outer-top, inner-bottom, outer-bottom
      // Inner-top
      Vertex vInnerTop;
      vInnerTop.position = center + glm::vec3(0.0f, halfDepth, 0.0f) - radial * halfThickness;
      vInnerTop.normal = glm::vec3(0.0f, 1.0f, 0.0f);
      vInnerTop.uv = glm::vec2(static_cast<float>(seg) / static_cast<float>(segments), 0.0f);
      vertices.push_back(vInnerTop);

      // Outer-top
      Vertex vOuterTop;
      vOuterTop.position = center + glm::vec3(0.0f, halfDepth, 0.0f) + radial * halfThickness;
      vOuterTop.normal = glm::vec3(0.0f, 1.0f, 0.0f);
      vOuterTop.uv = glm::vec2(static_cast<float>(seg) / static_cast<float>(segments), 0.33f);
      vertices.push_back(vOuterTop);

      // Outer-bottom
      Vertex vOuterBottom;
      vOuterBottom.position = center - glm::vec3(0.0f, halfDepth, 0.0f) + radial * halfThickness;
      vOuterBottom.normal = glm::vec3(0.0f, -1.0f, 0.0f);
      vOuterBottom.uv = glm::vec2(static_cast<float>(seg) / static_cast<float>(segments), 0.66f);
      vertices.push_back(vOuterBottom);

      // Inner-bottom
      Vertex vInnerBottom;
      vInnerBottom.position = center - glm::vec3(0.0f, halfDepth, 0.0f) - radial * halfThickness;
      vInnerBottom.normal = glm::vec3(0.0f, -1.0f, 0.0f);
      vInnerBottom.uv = glm::vec2(static_cast<float>(seg) / static_cast<float>(segments), 1.0f);
      vertices.push_back(vInnerBottom);
    }

    // Generate triangles connecting adjacent cross-sections
    for (int seg = 0; seg < segments; ++seg) {
      uint32_t segStart = seg * 4;
      uint32_t nextSegStart = (seg + 1) * 4;

      // Top face quad
      indices.push_back(segStart + 0);     // Inner-top current
      indices.push_back(nextSegStart + 0); // Inner-top next
      indices.push_back(segStart + 1);     // Outer-top current
      indices.push_back(segStart + 1);
      indices.push_back(nextSegStart + 0);
      indices.push_back(nextSegStart + 1); // Outer-top next

      // Outer side quad
      indices.push_back(segStart + 1);     // Outer-top current
      indices.push_back(nextSegStart + 1); // Outer-top next
      indices.push_back(segStart + 2);     // Outer-bottom current
      indices.push_back(segStart + 2);
      indices.push_back(nextSegStart + 1);
      indices.push_back(nextSegStart + 2); // Outer-bottom next

      // Bottom face quad
      indices.push_back(segStart + 2);     // Outer-bottom current
      indices.push_back(nextSegStart + 2); // Outer-bottom next
      indices.push_back(segStart + 3);     // Inner-bottom current
      indices.push_back(segStart + 3);
      indices.push_back(nextSegStart + 2);
      indices.push_back(nextSegStart + 3); // Inner-bottom next

      // Inner side quad
      indices.push_back(segStart + 3);     // Inner-bottom current
      indices.push_back(nextSegStart + 3); // Inner-bottom next
      indices.push_back(segStart + 0);     // Inner-top current (close loop)
      indices.push_back(segStart + 0);
      indices.push_back(nextSegStart + 3);
      indices.push_back(nextSegStart + 0); // Inner-top next
    }

    return {vertices, indices};
  }

  // Generate segmented 3D ring geometry (with gaps and end caps)
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateSegmented3DRing(float ringRadius,
                                                                                       float thickness, float depth,
                                                                                       int segmentsPerRing,
                                                                                       int numSegments, float gapAngle,
                                                                                       float rotationAngle = 0.0f) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float halfThickness = thickness * 0.0005 * 0.5f; // Convert to world space
    float halfDepth = depth * 0.5f;

    // Each ring is divided into segments with gaps
    float segmentAngle = (2.0f * PI - gapAngle * numSegments) / numSegments;
    int pointsPerSegment = segmentsPerRing / numSegments;

    for (int seg = 0; seg < numSegments; ++seg) {
      float segmentStartAngle = seg * (segmentAngle + gapAngle) + rotationAngle;
      uint32_t segmentStartIndex = static_cast<uint32_t>(vertices.size());

      // Generate points for this segment
      for (int i = 0; i <= pointsPerSegment; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(pointsPerSegment);
        float angle = segmentStartAngle + t * segmentAngle;

        glm::vec3 radial = glm::normalize(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
        glm::vec3 center = radial * ringRadius;

        // Create 4 vertices for rectangular cross-section
        // Inner-top
        Vertex vInnerTop;
        vInnerTop.position = center + glm::vec3(0.0f, halfDepth, 0.0f) - radial * halfThickness;
        vInnerTop.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vInnerTop.uv = glm::vec2(t, 0.0f);
        vertices.push_back(vInnerTop);

        // Outer-top
        Vertex vOuterTop;
        vOuterTop.position = center + glm::vec3(0.0f, halfDepth, 0.0f) + radial * halfThickness;
        vOuterTop.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vOuterTop.uv = glm::vec2(t, 0.33f);
        vertices.push_back(vOuterTop);

        // Outer-bottom
        Vertex vOuterBottom;
        vOuterBottom.position = center - glm::vec3(0.0f, halfDepth, 0.0f) + radial * halfThickness;
        vOuterBottom.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        vOuterBottom.uv = glm::vec2(t, 0.66f);
        vertices.push_back(vOuterBottom);

        // Inner-bottom
        Vertex vInnerBottom;
        vInnerBottom.position = center - glm::vec3(0.0f, halfDepth, 0.0f) - radial * halfThickness;
        vInnerBottom.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        vInnerBottom.uv = glm::vec2(t, 1.0f);
        vertices.push_back(vInnerBottom);
      }

      // Generate triangles for this segment
      for (int i = 0; i < pointsPerSegment; ++i) {
        uint32_t segStart = segmentStartIndex + i * 4;
        uint32_t nextSegStart = segmentStartIndex + (i + 1) * 4;

        // Top face quad
        indices.push_back(segStart + 0);
        indices.push_back(nextSegStart + 0);
        indices.push_back(segStart + 1);
        indices.push_back(segStart + 1);
        indices.push_back(nextSegStart + 0);
        indices.push_back(nextSegStart + 1);

        // Outer side quad
        indices.push_back(segStart + 1);
        indices.push_back(nextSegStart + 1);
        indices.push_back(segStart + 2);
        indices.push_back(segStart + 2);
        indices.push_back(nextSegStart + 1);
        indices.push_back(nextSegStart + 2);

        // Bottom face quad
        indices.push_back(segStart + 2);
        indices.push_back(nextSegStart + 2);
        indices.push_back(segStart + 3);
        indices.push_back(segStart + 3);
        indices.push_back(nextSegStart + 2);
        indices.push_back(nextSegStart + 3);

        // Inner side quad
        indices.push_back(segStart + 3);
        indices.push_back(nextSegStart + 3);
        indices.push_back(segStart + 0);
        indices.push_back(segStart + 0);
        indices.push_back(nextSegStart + 3);
        indices.push_back(nextSegStart + 0);
      }

      // Add end caps for this segment (close the ends)
      uint32_t segmentFirst = segmentStartIndex;
      uint32_t segmentLast = segmentStartIndex + pointsPerSegment * 4;

      // Start cap (4 vertices forming a quad)
      indices.push_back(segmentFirst + 0); // Inner-top
      indices.push_back(segmentFirst + 1); // Outer-top
      indices.push_back(segmentFirst + 3); // Inner-bottom
      indices.push_back(segmentFirst + 1); // Outer-top
      indices.push_back(segmentFirst + 2); // Outer-bottom
      indices.push_back(segmentFirst + 3); // Inner-bottom

      // End cap (4 vertices forming a quad)
      indices.push_back(segmentLast + 0); // Inner-top
      indices.push_back(segmentLast + 3); // Inner-bottom
      indices.push_back(segmentLast + 1); // Outer-top
      indices.push_back(segmentLast + 1); // Outer-top
      indices.push_back(segmentLast + 3); // Inner-bottom
      indices.push_back(segmentLast + 2); // Outer-bottom
    }

    return {vertices, indices};
  }

  // Generate bottom plate: circular grid with concentric rings (smooth circles with many segments)
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateBottomPlate(float radius, int numRings,
                                                                                   int segmentsPerRing) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    const uint32_t PRIMITIVE_RESTART = 0xFFFFFFFF;

    // Generate concentric rings (more segments for smoother circles)
    for (int ring = 0; ring <= numRings; ++ring) {
      float ringRadius = radius * (static_cast<float>(ring) / static_cast<float>(numRings));

      for (int i = 0; i <= segmentsPerRing; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segmentsPerRing);
        float x = ringRadius * std::cos(angle);
        float z = ringRadius * std::sin(angle);
        float y = 0.0f; // Plate is horizontal at y=0

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Normal points up
        v.uv = glm::vec2(0.0f, 0.0f);
        vertices.push_back(v);
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
      }

      indices.push_back(PRIMITIVE_RESTART);
    }

    // Generate radial lines (from center to outer edge)
    int numRadialLines = 16;
    for (int line = 0; line < numRadialLines; ++line) {
      float angle = 2.0f * PI * static_cast<float>(line) / static_cast<float>(numRadialLines);

      // Center point
      Vertex center;
      center.position = glm::vec3(0.0f, 0.0f, 0.0f);
      center.normal = glm::vec3(0.0f, 1.0f, 0.0f);
      center.uv = glm::vec2(0.0f, 0.0f);
      vertices.push_back(center);
      indices.push_back(static_cast<uint32_t>(vertices.size() - 1));

      // Outer edge point
      Vertex outer;
      outer.position = glm::vec3(radius * std::cos(angle), 0.0f, radius * std::sin(angle));
      outer.normal = glm::vec3(0.0f, 1.0f, 0.0f);
      outer.uv = glm::vec2(0.0f, 0.0f);
      vertices.push_back(outer);
      indices.push_back(static_cast<uint32_t>(vertices.size() - 1));

      indices.push_back(PRIMITIVE_RESTART);
    }

    return {vertices, indices};
  }

  // Generate a single segmented ring at a specific radius and height with optional rotation
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateSegmentedRing(float ringRadius, float height,
                                                                                     int segmentsPerRing,
                                                                                     int numSegments, float gapAngle,
                                                                                     float rotationAngle = 0.0f) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    const uint32_t PRIMITIVE_RESTART = 0xFFFFFFFF;

    // Each ring is divided into segments with gaps
    float segmentAngle = (2.0f * PI - gapAngle * numSegments) / numSegments;

    for (int seg = 0; seg < numSegments; ++seg) {
      float segmentStartAngle = seg * (segmentAngle + gapAngle) + rotationAngle;

      // Generate points for this segment
      for (int i = 0; i <= segmentsPerRing / numSegments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segmentsPerRing / numSegments);
        float angle = segmentStartAngle + t * segmentAngle;

        float x = ringRadius * std::cos(angle);
        float z = ringRadius * std::sin(angle);
        float y = height;

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v.uv = glm::vec2(0.0f, 0.0f);
        vertices.push_back(v);
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
      }

      indices.push_back(PRIMITIVE_RESTART);
    }

    return {vertices, indices};
  }

  // Generate elevated segmented rings: outer 1-2 rings, slightly smaller, segmented into quarters/thirds
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateElevatedRings(float baseRadius, int numRings,
                                                                                     float height, int segmentsPerRing,
                                                                                     int numSegments, float gapAngle,
                                                                                     float baseRotation = 0.0f) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    const uint32_t PRIMITIVE_RESTART = 0xFFFFFFFF;

    // Generate the outer 1-2 rings (skip the outermost, use the next ones)
    for (int ringOffset = 1; ringOffset <= numRings; ++ringOffset) {
      // Calculate ring radius (slightly smaller than base)
      float ringRadius = baseRadius * (1.0f - 0.15f * ringOffset); // Each ring 15% smaller

      // Apply different rotation to each ring so gaps don't align
      float rotationAngle = baseRotation + 0.6f * static_cast<float>(ringOffset - 1); // Rotate each ring differently

      // Generate segmented ring with rotation
      auto [ringVertices, ringIndices] =
        generateSegmentedRing(ringRadius, height, segmentsPerRing, numSegments, gapAngle, rotationAngle);

      // Append to main vertices/indices
      uint32_t indexOffset = static_cast<uint32_t>(vertices.size());
      vertices.insert(vertices.end(), ringVertices.begin(), ringVertices.end());
      for (uint32_t idx : ringIndices) {
        if (idx == PRIMITIVE_RESTART) {
          indices.push_back(PRIMITIVE_RESTART);
        } else {
          indices.push_back(idx + indexOffset);
        }
      }
    }

    return {vertices, indices};
  }

  // Generate sphere grid wireframe (for background)
  static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateSphereGrid(float radius, int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    const uint32_t PRIMITIVE_RESTART = 0xFFFFFFFF;

    // Generate latitude lines (horizontal circles)
    for (int lat = 0; lat <= segments; ++lat) {
      float latAngle = PI * static_cast<float>(lat) / static_cast<float>(segments);
      float y = radius * std::cos(latAngle);
      float latRadius = radius * std::sin(latAngle);

      for (int lon = 0; lon <= segments; ++lon) {
        float lonAngle = 2.0f * PI * static_cast<float>(lon) / static_cast<float>(segments);
        float x = latRadius * std::cos(lonAngle);
        float z = latRadius * std::sin(lonAngle);

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = glm::normalize(glm::vec3(x, y, z));
        v.uv = glm::vec2(0.0f, 0.0f);
        vertices.push_back(v);
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
      }

      indices.push_back(PRIMITIVE_RESTART);
    }

    // Generate longitude lines (vertical lines from pole to pole)
    for (int lon = 0; lon <= segments; ++lon) {
      float lonAngle = 2.0f * PI * static_cast<float>(lon) / static_cast<float>(segments);

      for (int lat = 0; lat <= segments; ++lat) {
        float latAngle = PI * static_cast<float>(lat) / static_cast<float>(segments);
        float y = radius * std::cos(latAngle);
        float latRadius = radius * std::sin(latAngle);
        float x = latRadius * std::cos(lonAngle);
        float z = latRadius * std::sin(lonAngle);

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = glm::normalize(glm::vec3(x, y, z));
        v.uv = glm::vec2(0.0f, 0.0f);
        vertices.push_back(v);
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
      }

      indices.push_back(PRIMITIVE_RESTART);
    }

    return {vertices, indices};
  }

private:
  static constexpr float PI = 3.14159265358979323846f;
};
