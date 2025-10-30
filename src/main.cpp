#include "../external/glad/include/glad/glad.h"
#include "mesh.h"
#include "shader.h"
#include "sphere.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // ============================================
  // WINDOW SETUP
  // ============================================
  // Initial window size (these can be changed to resize the window)
  // Using 1600x900 for almost fullscreen on smaller laptops
  const int INITIAL_WIDTH = 1600;
  const int INITIAL_HEIGHT = 900;

  GLFWwindow *window =
      glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "Breathe", NULL, NULL);
  if (window == nullptr) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    return -1;
  }

  int windowWidth, windowHeight;
  glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
  glViewport(0, 0, windowWidth, windowHeight);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Enable depth testing for 3D rendering
  glEnable(GL_DEPTH_TEST);

  // Create shader program
  Shader shader("shaders/sphere.vert", "shaders/sphere.frag");

  // ============================================
  // SPHERE PROPERTIES
  // ============================================
  // Radius: Size of the sphere (1.0f = unit sphere)
  // Segments: Resolution (32 = good quality, higher = smoother but slower)
  Mesh sphere = Sphere::generate(1.0f, 32);
  sphere.setupMesh();

  // ============================================
  // CAMERA SETUP
  // ============================================
  // cameraPos: Where the camera is in 3D space
  // cameraTarget: What the camera is looking at
  // cameraUp: Which direction is "up" for the camera
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  // ============================================
  // LIGHTING SETUP
  // ============================================
  // lightPos: Where the light source is in the scene
  glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 2.0f);

  // ============================================
  // ANIMATION SETUP
  // ============================================
  // angle: Current rotation angle (increases each frame)
  float angle = 0.0f;

  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    // Get current window size
    int currentWidth, currentHeight;
    glfwGetFramebufferSize(window, &currentWidth, &currentHeight);

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ============================================
    // ANIMATION UPDATE
    // ============================================
    // Change 0.01f to make rotation faster/slower
    angle += 0.01f;

    // ============================================
    // TRANSFORM MATRICES
    // ============================================
    // View matrix: Camera position and orientation
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    // Projection matrix: Field of view, aspect ratio, near/far planes
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), // FOV (field of view) in degrees
        (float)currentWidth / (float)currentHeight, // Aspect ratio
        0.1f,                                       // Near clipping plane
        100.0f                                      // Far clipping plane
    );

    // ============================================
    // MODEL TRANSFORMATION
    // ============================================
    // Model matrix: Position, rotation, and scale of the sphere in world space
    glm::mat4 model = glm::mat4(1.0f); // Start with identity matrix
    model = glm::rotate(model, angle,
                        glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y-axis

    // ============================================
    // SHADER SETUP
    // ============================================
    shader.use();

    // Send transformation matrices to GPU
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    // Send lighting information to GPU
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("viewPos", cameraPos);

    // ============================================
    // SPHERE VISUAL PROPERTIES
    // ============================================
    // sphereColor: RGB color of the sphere (0.0 to 1.0 range)
    // This is blue: (red=0.2, green=0.5, blue=1.0)
    shader.setVec3("sphereColor", glm::vec3(0.2f, 0.5f, 1.0f));

    // Draw sphere
    sphere.draw();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow *window) {
  // close windown on escape press
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}
