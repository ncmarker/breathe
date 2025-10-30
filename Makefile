# Makefile for Breathe (macOS / Linux-like)
# Assumes:
#  - glad headers in external/glad/include
#  - glad.c at external/glad/glad.c
#  - glfw and glm installed via Homebrew (or otherwise available as -lglfw and headers in standard include paths)

CXX := g++
# Common include paths (will be checked)
INCLUDE_DIRS := -I external/glad/include

# Try to find GLM using common paths
GLM_PATHS := /opt/homebrew/Cellar/glm/1.0.2/include \
             /usr/local/include \
             /usr/include \
             /opt/homebrew/include
GLM_FOUND := $(shell find $(GLM_PATHS) -name "glm.hpp" 2>/dev/null | head -1)
ifneq ($(GLM_FOUND),)
    GLM_INC := $(shell dirname $(dir $(GLM_FOUND)))
    INCLUDE_DIRS += -I $(GLM_INC)
endif

# Try to find GLFW include directory  
GLFW_PATHS := /opt/homebrew/opt/glfw/include \
              /usr/local/include \
              /usr/include \
              /opt/homebrew/include
GLFW_FOUND := $(shell find $(GLFW_PATHS) -name "glfw3.h" 2>/dev/null | head -1)
ifneq ($(GLFW_FOUND),)
    GLFW_INC := $(shell dirname $(dir $(GLFW_FOUND)))
    INCLUDE_DIRS += -I $(GLFW_INC)
endif

# Try to find FreeType
FREETYPE_PATHS := /opt/homebrew/Cellar/freetype/2.14.1_1/include/freetype2 \
                  /opt/homebrew/include/freetype2 \
                  /opt/homebrew/include \
                  /usr/local/include/freetype2 \
                  /usr/include/freetype2
FREETYPE_FOUND := $(shell find $(FREETYPE_PATHS) -name "ft2build.h" 2>/dev/null | head -1)
ifneq ($(FREETYPE_FOUND),)
    FREETYPE_INC := $(shell dirname $(FREETYPE_FOUND))
    INCLUDE_DIRS += -I $(FREETYPE_INC)
endif

# Also search for any freetype version under homebrew
FREETYPE_HOMEBREW := $(wildcard /opt/homebrew/Cellar/freetype/*/include/freetype2)
ifneq ($(FREETYPE_HOMEBREW),)
    INCLUDE_DIRS += -I $(firstword $(FREETYPE_HOMEBREW))
endif

CXXFLAGS := -std=c++17 -O2 -Wall $(INCLUDE_DIRS)

LDFLAGS :=

# Platform-specific linking
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS: link glfw, freetype and OpenGL framework
    LDFLAGS += -L /opt/homebrew/lib -lglfw -lfreetype -framework OpenGL
else
    # Linux: link GL, glfw, and freetype (adjust if needed)
    LDFLAGS += -lglfw -lfreetype -lGL -ldl
endif

# Sources
GLAD_SRC := external/glad/glad.c
SRCS := $(wildcard src/*.cpp)
OBJS := $(GLAD_SRC:.c=.o) $(SRCS:.cpp=.o)
TARGET := breathe

.PHONY: all clean run

all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile GLAD (C file)
external/glad/glad.o: external/glad/glad.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile cpp files
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o external/glad/*.o

run: all
	./$(TARGET)
