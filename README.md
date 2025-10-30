# Breathe - OpenGL Sphere Renderer

A simple OpenGL 3D sphere renderer built with C++ using GLFW, GLAD, and GLM. Features Tron-like glowing edge effects with smooth rotation.

## Features

- **3D Sphere Rendering**: Parametric sphere generation with customizable segments
- **Tron-style Shading**: Fresnel-based edge glow for a futuristic look
- **Smooth Animation**: Continuous rotation with depth testing
- **Modern OpenGL**: Uses VAO, VBO, and EBO for efficient rendering
- **Easy to Extend**: Clean, beginner-friendly codebase

## Requirements

- macOS or Linux
- C++17 compiler (g++ or clang++)
- OpenGL 3.3+
- GLFW (window management)
- GLM (math library)
- GLAD (OpenGL loader)

## Installation

### macOS

```bash
# Install dependencies
brew install glfw glm

# Build the project
make

# Run
./breathe
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libglfw3-dev libglm-dev

# Build the project
make

# Run
./breathe
```

## Project Structure

```
Breathe/
├── src/
│   ├── main.cpp      # Main application loop
│   ├── shader.h      # Shader program wrapper
│   ├── shader.cpp    # Shader implementation
│   ├── mesh.h        # Mesh data structure
│   ├── mesh.cpp      # Mesh rendering logic
│   └── sphere.h      # Sphere generator utility
├── shaders/
│   ├── sphere.vert   # Vertex shader
│   └── sphere.frag   # Fragment shader (Tron style)
├── external/
│   └── glad/         # OpenGL loader
└── Makefile          # Build configuration
```

## Controls

- **ESC**: Exit the application

## Customization

### Change Sphere Color

Edit `src/main.cpp` line 93:
```cpp
shader.setVec3("sphereColor", glm::vec3(0.2f, 0.5f, 1.0f)); // Blue
```

### Adjust Sphere Quality

Edit `src/main.cpp` line 54:
```cpp
Mesh sphere = Sphere::generate(1.0f, 32); // radius, segments
```

### Modify Glow Effect

Edit `shaders/sphere.frag` line 28:
```glsl
finalColor += vec3(0.2, 0.6, 1.0) * fresnel * 0.5; // glow color and intensity
```

## Code Overview

### Key Classes

- **Shader**: Loads and compiles GLSL shaders, sets uniforms
- **Mesh**: Manages VAO/VBO/EBO for vertex data
- **Sphere**: Generates sphere geometry parametrically

### Rendering Pipeline

1. Generate sphere vertices and indices
2. Upload to GPU via VAO/VBO/EBO
3. Compile and link shaders
4. Each frame:
   - Clear color and depth buffers
   - Calculate model/view/projection matrices
   - Set shader uniforms
   - Draw sphere mesh

## Future Enhancements

- Camera controls (mouse/keyboard)
- Texture mapping for Earth
- Atmospheric scattering effects
- Multiple spheres
- Post-processing effects

## License

MIT License - feel free to use for learning and projects!

## Credits

Built with:
- [GLFW](https://www.glfw.org/) - Window management
- [GLM](https://github.com/g-truc/glm) - Mathematics
- [GLAD](https://github.com/Dav1dde/glad) - OpenGL loading

