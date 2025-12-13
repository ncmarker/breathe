#include "mesh.h"
#include <glad/glad.h>
#include <iostream>

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
    : vertices(vertices), indices(indices), initialized(false) {}

Mesh::Mesh(Mesh &&other) noexcept {
  vertices = std::move(other.vertices);
  indices = std::move(other.indices);
  VAO = other.VAO;
  VBO = other.VBO;
  EBO = other.EBO;
  initialized = other.initialized;

  other.VAO = 0;
  other.VBO = 0;
  other.EBO = 0;
  other.initialized = false;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
  if (this != &other) {
    cleanupGL();
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    VAO = other.VAO;
    VBO = other.VBO;
    EBO = other.EBO;
    initialized = other.initialized;

    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.initialized = false;
  }
  return *this;
}

Mesh::~Mesh() {
  cleanupGL();
}

void Mesh::setupMesh() {
  if (initialized) {
    std::cerr << "Warning: Mesh already initialized. Skipping setup.\n";
    return;
  }

  if (vertices.empty()) {
    std::cerr << "Error: Cannot setup mesh with no vertices.\n";
    return;
  }

  // Generate and bind VAO
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Generate and bind VBO
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

  // Generate and bind EBO (if indices exist)
  if (!indices.empty()) {
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
  }

  // Set up vertex attributes
  // Position (location 0)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));

  // Normal (location 1)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));

  // UV (location 2)
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

  // Unbind VAO
  glBindVertexArray(0);

  initialized = true;
}

void Mesh::draw() const {
  if (!initialized) {
    std::cerr << "Error: Cannot draw uninitialized mesh.\n";
    return;
  }

  glBindVertexArray(VAO);

  if (!indices.empty()) {
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
  } else {
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
  }

  glBindVertexArray(0);
}

void Mesh::drawLines() const {
  if (!initialized) {
    std::cerr << "Error: Cannot draw uninitialized mesh.\n";
    return;
  }

  glBindVertexArray(VAO);

  if (!indices.empty()) {
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
  } else {
    glDrawArrays(GL_LINES, 0, vertices.size());
  }

  glBindVertexArray(0);
}

void Mesh::drawLineStrip() const {
  if (!initialized) {
    std::cerr << "Error: Cannot draw uninitialized mesh.\n";
    return;
  }

  glBindVertexArray(VAO);

  if (!indices.empty()) {
    // Enable primitive restart for line strips (breaks strips at 0xFFFFFFFF index)
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFF);
    glDrawElements(GL_LINE_STRIP, indices.size(), GL_UNSIGNED_INT, 0);
    glDisable(GL_PRIMITIVE_RESTART);
  } else {
    glDrawArrays(GL_LINE_STRIP, 0, vertices.size());
  }

  glBindVertexArray(0);
}

void Mesh::drawInstanced(unsigned int instanceCount) const {
  if (!initialized) {
    std::cerr << "Error: Cannot draw uninitialized mesh.\n";
    return;
  }

  glBindVertexArray(VAO);

  if (!indices.empty()) {
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
  } else {
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), instanceCount);
  }

  glBindVertexArray(0);
}

void Mesh::updateVertexData(const std::vector<Vertex> &newVertices) {
  if (!initialized) {
    std::cerr << "Error: Cannot update uninitialized mesh.\n";
    return;
  }

  if (newVertices.size() != vertices.size()) {
    std::cerr << "Error: New vertex count doesn't match existing count.\n";
    return;
  }

  vertices = newVertices;

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::cleanupGL() {
  if (VAO != 0) {
    glDeleteVertexArrays(1, &VAO);
    VAO = 0;
  }
  if (VBO != 0) {
    glDeleteBuffers(1, &VBO);
    VBO = 0;
  }
  if (EBO != 0) {
    glDeleteBuffers(1, &EBO);
    EBO = 0;
  }
  initialized = false;
}
