#pragma once
#include <ft2build.h>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include FT_FREETYPE_H
#include "shader.h"

struct Character {
  unsigned int TextureID; // ID handle of the glyph texture
  glm::ivec2 Size;        // Size of glyph
  glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
  unsigned int Advance;   // Offset to advance to next glyph
};

class TextRenderer {
public:
  std::map<char, Character> Characters;
  unsigned int VAO, VBO;
  Shader shader;

  TextRenderer(int width, int height);
  void Load(std::string font, unsigned int fontSize);
  void RenderText(std::string text, float x, float y, float scale,
                  glm::vec3 color);
};
