#include "../external/glad/include/glad/glad.h"
#include "co2_data.h"
#include "geo_borders.h"
#include "mesh.h"
#include "shader.h"
#include "sphere.h"
#include "textRenderer.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_map>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

namespace {
constexpr float SPHERE_RADIUS = 0.8f;
constexpr float BORDER_RADIUS = SPHERE_RADIUS * 1.002f;
constexpr float MARKER_ALTITUDE = 0.02f;
constexpr float MIN_MARKER_RADIUS = 0.01f;
constexpr float MAX_MARKER_RADIUS = 0.15f;
constexpr int MARKER_SEGMENTS = 24;
} // namespace

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
  Mesh sphere = Sphere::generate(SPHERE_RADIUS, 32);
  sphere.setupMesh();

  // ============================================
  // TEXT RENDERER
  // ============================================
  TextRenderer textRenderer(windowWidth, windowHeight);
  textRenderer.Load("assets/Code.ttf", 48);

  // ============================================
  // COUNTRY BORDERS
  // ============================================
  // Load country border data
  auto borders = GeoBorders::loadBordersFromGeoJSON("data/countries.geo.json",
                                                    BORDER_RADIUS);
  auto borderVertices = GeoBorders::bordersToVertices(borders);
  Mesh borderMesh(borderVertices,
                  std::vector<uint32_t>()); // Empty indices, just vertices
  borderMesh.setupMesh();

  // Border shader
  Shader borderShader("shaders/border.vert", "shaders/border.frag");

  // ============================================
  // CO₂ DATA VISUALIZATION
  // ============================================
  auto countryCentroids =
      GeoBorders::computeCountryCentroids(borders, SPHERE_RADIUS);

  CO2DataSet co2Data;
  bool co2Loaded = co2Data.loadFromCsv(
      "data/annual-co-emissions-by-region.csv");
  int currentYear = co2Loaded ? co2Data.latestYear() : 0;

  std::string globalCO2Label = "Global CO2: --";
  std::string yearLabelText = "Year: --";
  double totalEmissionForYear = 0.0;

  Mesh markerMesh;
  bool markersAvailable = false;
  Shader dataShader("shaders/data.vert", "shaders/data.frag");

  if (co2Loaded && currentYear > 0) {
    totalEmissionForYear = 0.0;
    std::vector<std::pair<std::string, double>> emissions;
    emissions.reserve(countryCentroids.size());

    double minEmission = std::numeric_limits<double>::max();
    double maxEmission = std::numeric_limits<double>::lowest();

    // Collect emissions for all countries that have data for the current year
    for (const auto &entry : countryCentroids) {
      auto emission = co2Data.getEmission(entry.first, currentYear);
      if (!emission.has_value())
        continue;

      emissions.emplace_back(entry.first, emission.value());
      emissions.emplace_back(entry.first, emission.value());
      totalEmissionForYear += emission.value();
      minEmission = std::min(minEmission, emission.value());
      maxEmission = std::max(maxEmission, emission.value());
    }

    // Create circular markers for each country with CO2 data
    if (!emissions.empty() && std::isfinite(minEmission) &&
        std::isfinite(maxEmission)) {
      std::vector<Vertex> markerVertices;
      std::vector<uint32_t> markerIndices;
      markerVertices.reserve(emissions.size() * (MARKER_SEGMENTS + 1));
      markerIndices.reserve(emissions.size() * MARKER_SEGMENTS * 3);

      const float liftedRadius = SPHERE_RADIUS + MARKER_ALTITUDE;
      const float angleStep =
          glm::two_pi<float>() / static_cast<float>(MARKER_SEGMENTS);

      // Build circular markers for each country
      for (const auto &item : emissions) {
        const auto centroidIt = countryCentroids.find(item.first);
        if (centroidIt == countryCentroids.end())
          continue;

        // Get the country's centroid position on the sphere
        glm::vec3 basePosition = centroidIt->second;
        if (glm::length(basePosition) <= std::numeric_limits<float>::epsilon())
          continue;

        // Position marker slightly above the sphere surface to avoid z-fighting
        glm::vec3 normal = glm::normalize(basePosition);
        glm::vec3 markerCenter = normal * liftedRadius;

        // Normalize emission value to 0.0-1.0 range for size scaling
        float normalizedValue = 1.0f;
        if (maxEmission > minEmission) {
          normalizedValue = static_cast<float>((item.second - minEmission) /
                                               (maxEmission - minEmission));
        }

        // Calculate marker size based on normalized emission value
        // Larger emissions = larger circles
        float markerRadius =
            MIN_MARKER_RADIUS +
            normalizedValue * (MAX_MARKER_RADIUS - MIN_MARKER_RADIUS);

        // Create tangent and bitangent vectors for building the circle in the
        // plane perpendicular to the sphere surface
        glm::vec3 tangent = glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f));
        if (glm::length(tangent) < 1e-4f) {
          tangent = glm::cross(normal, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        tangent = glm::normalize(tangent);
        glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

        // Create a circular marker by building a fan of triangles
        // Center vertex
        uint32_t startIndex = static_cast<uint32_t>(markerVertices.size());
        Vertex centerVertex;
        centerVertex.position = markerCenter;
        centerVertex.normal = normal;
        centerVertex.uv = glm::vec2(normalizedValue, 0.0f);
        markerVertices.push_back(centerVertex);

        // Create vertices around the circle perimeter
        for (int i = 0; i < MARKER_SEGMENTS; ++i) {
          float angle = angleStep * static_cast<float>(i);
          glm::vec3 offset =
              std::cos(angle) * tangent + std::sin(angle) * bitangent;

          Vertex v;
          v.position = markerCenter + offset * markerRadius;
          v.normal = normal;
          v.uv = glm::vec2(normalizedValue, 0.0f);
          markerVertices.push_back(v);
        }

        // Create triangle indices (fan from center to perimeter)
        for (int i = 1; i <= MARKER_SEGMENTS; ++i) {
          uint32_t current = startIndex + i;
          uint32_t next =
              (i == MARKER_SEGMENTS) ? startIndex + 1 : startIndex + i + 1;
          markerIndices.push_back(startIndex);
          markerIndices.push_back(current);
          markerIndices.push_back(next);
        }
      }

      if (!markerVertices.empty()) {
        markerMesh = Mesh(markerVertices, markerIndices);
        markerMesh.setupMesh();
        markersAvailable = true;
      }
    }
  }

  // ============================================
  // SET UP TEXT LABELS
  // ============================================
  if (co2Loaded && currentYear > 0) {
    yearLabelText = "Year: " + std::to_string(currentYear);

    if (markersAvailable) {
      std::ostringstream oss;
      oss.setf(std::ios::fixed);
      oss.precision(2);

      double totalGt = totalEmissionForYear * 1e-9;
      oss << "Global CO2: " << totalGt << " Gt";
      globalCO2Label = oss.str();
    } else {
      globalCO2Label = "Global CO2: data unavailable";
    }
  }

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
  // Rotation angle for the globe (increases each frame for continuous rotation)
  // To change rotation speed, modify the increment value in the render loop
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
    // Rotate the globe continuously
    // To change rotation speed: increase value for faster, decrease for slower
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

    // ============================================
    // COUNTRY BORDER RENDERING
    // ============================================
    borderShader.use();
    borderShader.setMat4("model", model);
    borderShader.setMat4("view", view);
    borderShader.setMat4("projection", projection);
    borderShader.setVec3("borderColor",
                         glm::vec3(1.0f, 1.0f, 1.0f)); // White borders

    borderMesh.drawLines();

    // ============================================
    // CO₂ MARKER RENDERING
    // ============================================
    if (markersAvailable) {
      dataShader.use();
      dataShader.setMat4("model", model);
      dataShader.setMat4("view", view);
      dataShader.setMat4("projection", projection);
      dataShader.setVec3("baseColor", glm::vec3(1.0f, 0.2f, 0.1f));
      markerMesh.draw();
    }

    // ============================================
    // TEXT RENDERING
    // ============================================
    // Disable depth test for text overlay
    glDisable(GL_DEPTH_TEST);

    // Enable blending for text transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render text
    textRenderer.RenderText("The Breathing Planet", windowWidth / 2.0f - 350.0f,
                            windowHeight - 80.0f, 1.0f,
                            glm::vec3(0.9f, 0.9f, 0.9f));
    textRenderer.RenderText("Breathe", windowWidth / 2.0f - 80.0f, 80.0f, 0.7f,
                            glm::vec3(0.9f, 0.9f, 0.9f));
    textRenderer.RenderText(globalCO2Label, 50.0f, 100.0f, 0.5f,
                            glm::vec3(0.9f, 0.9f, 0.9f));
    textRenderer.RenderText(yearLabelText, 50.0f, 65.0f, 0.5f,
                            glm::vec3(0.9f, 0.9f, 0.9f));

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

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
