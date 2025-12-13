#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

// Vertex layout used consistently across shaders
struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

// Usage: Mesh mesh(vertices, indices); mesh.setupMesh(); mesh.draw();
class Mesh {
public:
  Mesh() = default;
  // Create from CPU-side arrays
  Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

  // Non-copyable (GL handles)
  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  // Movable
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  ~Mesh();

  // Uploads vertices/indices to GPU and configures VAO/VBO/EBO
  void setupMesh();

  // Draw call (binds VAO and issues glDrawElements)
  void draw() const;

  // Draw with custom mode (GL_TRIANGLES, GL_LINES, etc.)
  void drawLines() const;
  void drawLineStrip() const;

  // Draw with instance count
  void drawInstanced(unsigned int instanceCount) const;

  // If you want to update vertices dynamically (for vertex displacement / breathing)
  // call this to re-upload vertex positions/normals (assumes same vertex count).
  void updateVertexData(const std::vector<Vertex> &vertices);

  // Accessors
  size_t vertexCount() const {
    return vertices.size();
  }
  size_t indexCount() const {
    return indices.size();
  }

private:
  // CPU-side copies (optional: free them after upload if memory is a concern)
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  // OpenGL handles
  unsigned int VAO = 0;
  unsigned int VBO = 0;
  unsigned int EBO = 0;

  bool initialized = false;

  // Internal helper to free GL objects
  void cleanupGL();
};
