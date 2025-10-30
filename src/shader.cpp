#include "shader.h"
#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

// Constructor: builds shader from vertex/fragment source files
Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
  std::string vertCode = readFile(vertexPath);
  std::string fragCode = readFile(fragmentPath);

  unsigned int vert = compileShader(vertCode.c_str(), GL_VERTEX_SHADER);
  unsigned int frag = compileShader(fragCode.c_str(), GL_FRAGMENT_SHADER);

  ID = linkProgram(vert, frag);

  glDeleteShader(vert);
  glDeleteShader(frag);
}

Shader::Shader(Shader &&other) noexcept {
  ID = other.ID;
  uniformLocationCache = std::move(other.uniformLocationCache);
  other.ID = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept {
  if (this != &other) {
    if (ID)
      glDeleteProgram(ID);
    ID = other.ID;
    uniformLocationCache = std::move(other.uniformLocationCache);
    other.ID = 0;
  }
  return *this;
}

Shader::~Shader() {
  if (ID)
    glDeleteProgram(ID);
}

void Shader::use() const { glUseProgram(ID); }

// ---------- File + Compilation Helpers ----------
std::string Shader::readFile(const std::string &path) const {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "ERROR: Cannot open shader file " << path << std::endl;
    return "";
  }
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

unsigned int Shader::compileShader(const char *source,
                                   unsigned int type) const {
  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[1024];
    glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
    std::cerr << "ERROR: Shader compilation failed ("
              << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << ")\n"
              << infoLog << std::endl;
  }

  return shader;
}

unsigned int Shader::linkProgram(unsigned int vert, unsigned int frag) const {
  unsigned int program = glCreateProgram();
  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);

  int success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[1024];
    glGetProgramInfoLog(program, 1024, nullptr, infoLog);
    std::cerr << "ERROR: Shader linking failed\n" << infoLog << std::endl;
  }

  return program;
}

// ---------- Uniform helpers ----------
int Shader::getUniformLocation(const std::string &name) const {
  if (uniformLocationCache.count(name))
    return uniformLocationCache[name];

  int location = glGetUniformLocation(ID, name.c_str());
  if (location == -1)
    std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
  uniformLocationCache[name] = location;
  return location;
}

void Shader::setBool(const std::string &name, bool value) const {
  glUniform1i(getUniformLocation(name), (int)value);
}
void Shader::setInt(const std::string &name, int value) const {
  glUniform1i(getUniformLocation(name), value);
}
void Shader::setFloat(const std::string &name, float value) const {
  glUniform1f(getUniformLocation(name), value);
}
void Shader::setVec2(const std::string &name, const glm::vec2 &v) const {
  glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(v));
}
void Shader::setVec3(const std::string &name, const glm::vec3 &v) const {
  glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(v));
}
void Shader::setVec4(const std::string &name, const glm::vec4 &v) const {
  glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(v));
}
void Shader::setMat3(const std::string &name, const glm::mat3 &m) const {
  glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}
void Shader::setMat4(const std::string &name, const glm::mat4 &m) const {
  glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}

bool Shader::reload(const std::string &vertexPath,
                    const std::string &fragmentPath) {
  std::string vertCode = readFile(vertexPath);
  std::string fragCode = readFile(fragmentPath);
  unsigned int vert = compileShader(vertCode.c_str(), GL_VERTEX_SHADER);
  unsigned int frag = compileShader(fragCode.c_str(), GL_FRAGMENT_SHADER);
  unsigned int newProgram = linkProgram(vert, frag);

  if (newProgram) {
    if (ID)
      glDeleteProgram(ID);
    ID = newProgram;
    glDeleteShader(vert);
    glDeleteShader(frag);
    return true;
  }
  return false;
}
