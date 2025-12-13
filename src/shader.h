#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
  unsigned int ID = 0; // OpenGL program handle

  Shader() = default;
  // Construct and compile/link immediately from file paths
  Shader(const std::string &vertexPath, const std::string &fragmentPath);
  // Construct with optional geometry shader
  Shader(const std::string &vertexPath, const std::string &fragmentPath, const std::string &geometryPath);

  // Non-copyable (OpenGL handle management)
  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;

  // Movable
  Shader(Shader &&other) noexcept;
  Shader &operator=(Shader &&other) noexcept;

  ~Shader();

  // Activate program
  void use() const;

  // Utility uniform setters (common types)
  void setBool(const std::string &name, bool value) const;
  void setInt(const std::string &name, int value) const;
  void setFloat(const std::string &name, float value) const;
  void setVec2(const std::string &name, const glm::vec2 &v) const;
  void setVec3(const std::string &name, const glm::vec3 &v) const;
  void setVec4(const std::string &name, const glm::vec4 &v) const;
  void setMat3(const std::string &name, const glm::mat3 &m) const;
  void setMat4(const std::string &name, const glm::mat4 &m) const;

  // Optional: reload shaders from disk (helpful during development)
  bool reload(const std::string &vertexPath, const std::string &fragmentPath);

private:
  // Implementation helpers (private)
  std::string readFile(const std::string &path) const;
  unsigned int compileShader(const char *source, unsigned int type) const;
  unsigned int linkProgram(unsigned int vert, unsigned int frag) const;

  // Cache uniform locations to avoid repeated glGetUniformLocation calls
  mutable std::unordered_map<std::string, int> uniformLocationCache;
  int getUniformLocation(const std::string &name) const;
};
