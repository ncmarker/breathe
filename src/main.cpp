#include "../external/glad/include/glad/glad.h"
#include "breathe_animation.h"
#include "co2_data.h"
#include "dashboard.h"
#include "geo_borders.h"
#include "marker_builder.h"
#include "mesh.h"
#include "shader.h"
#include "sphere.h"
#include "textRenderer.h"
#include "year_controller.h"
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

struct MouseState {
  bool *isDragging;
  double *lastMouseX;
  double *lastMouseY;
  float *cameraAzimuth;
  float *cameraElevation;
  float *angle;
  float *initialAzimuth;
  float *initialElevation;
};

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window, YearController &yearController, BreatheAnimation &breatheAnimation);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);

namespace {
constexpr float SPHERE_RADIUS = 0.65f;
constexpr float BORDER_RADIUS = SPHERE_RADIUS * 1.002f;
constexpr int MANUAL_YEAR = -1;
constexpr int YEAR_INCREMENT = 5;
constexpr float DASHBOARD_PLATE_RADIUS = 0.775f;
constexpr float DASHBOARD_PLATE_Y = -0.85f;
constexpr float SPHERE_GRID_RADIUS = 3.0f;
constexpr float SPHERE_GRID_UNIT_SIZE = 1.0f;
constexpr int SPHERE_GRID_SEGMENTS = 32;
constexpr float THICK_LINE_DEPTH = 0.01f;
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
  const int INITIAL_WIDTH = 1600;
  const int INITIAL_HEIGHT = 900;

  GLFWwindow *window = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "Breathe", NULL, NULL);
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
  auto borders = GeoBorders::loadBordersFromGeoJSON("data/countries.geo.json", BORDER_RADIUS);
  auto [borderVertices, borderIndices] = GeoBorders::bordersToVerticesWithIndices(borders);
  Mesh borderMesh(borderVertices, borderIndices);
  borderMesh.setupMesh();

  // Border shader (with geometry shader for thick lines - blue glow)
  Shader borderShader("shaders/border.vert", "shaders/border.frag", "shaders/border.geom");

  // Simple border shader (no geometry shader - for white center line)
  Shader borderSimpleShader("shaders/border_simple.vert", "shaders/border_simple.frag");

  // ============================================
  // DASHBOARD GEOMETRY
  // ============================================
  std::vector<Mesh> dashboardRings;
  std::vector<float> ringThicknesses;
  std::vector<bool> ringIs3D;
  int numRings = 5;
  int segmentsPerRing = 64;
  for (int ring = numRings; ring >= 0; --ring) {
    float ringRadius = DASHBOARD_PLATE_RADIUS * (static_cast<float>(ring) / static_cast<float>(numRings));

    float thickness;
    bool isThick = false;
    if (ring == 3) {
      thickness = 108.0f;
      isThick = true;
    } else {
      if (ring > 3) {
        continue;
      }
      thickness = 3.0f + (5.8f - 3.0f) * (static_cast<float>(ring) / 2.0f);
      isThick = false;
    }
    ringThicknesses.push_back(thickness);
    ringIs3D.push_back(isThick);

    Mesh ringMesh;
    if (isThick) {
      auto [ringVertices, ringIndices] =
        Dashboard::generateSegmented3DRing(ringRadius, thickness, THICK_LINE_DEPTH, 128, 2, 0.15f, 0.0f);
      ringMesh = Mesh(ringVertices, ringIndices);
    } else {
      auto [ringVertices, ringIndices] = Dashboard::generateSingleRing(ringRadius, segmentsPerRing);
      ringMesh = Mesh(ringVertices, ringIndices);
    }
    ringMesh.setupMesh();
    dashboardRings.push_back(std::move(ringMesh));
  }

  float discInnerRadius = 0.0f;
  float discOuterRadius = DASHBOARD_PLATE_RADIUS * 0.85f;
  auto [discVertices, discIndices] = Dashboard::generateFilledDisc(discInnerRadius, discOuterRadius, 64);
  Mesh dashboardDiscMesh(discVertices, discIndices);
  dashboardDiscMesh.setupMesh();

  Shader discShader("shaders/disc.vert", "shaders/disc.frag");
  Shader ring3DShader("shaders/ring3d.vert", "shaders/ring3d.frag");

  float elevatedHeight = -0.7f;
  float elevatedRotation = 0.3f;
  auto [elevatedVertices, elevatedIndices] =
    Dashboard::generateElevatedRings(DASHBOARD_PLATE_RADIUS, 2, elevatedHeight, 64, 4, 0.2f, elevatedRotation);
  Mesh dashboardElevatedMesh(elevatedVertices, elevatedIndices);
  dashboardElevatedMesh.setupMesh();

  std::vector<Mesh> smallSegmentedRings;
  std::vector<float> smallRingHeights;
  std::vector<float> smallRingThicknesses;
  int numSmallRings = 3;
  for (int smallRing = 1; smallRing <= numSmallRings; ++smallRing) {
    float smallRingRadius = DASHBOARD_PLATE_RADIUS * (static_cast<float>(smallRing - 1) / static_cast<float>(numRings));
    float smallRingHeight =
      DASHBOARD_PLATE_Y +
      (elevatedHeight - DASHBOARD_PLATE_Y) * (static_cast<float>(smallRing) / static_cast<float>(numSmallRings + 1));

    float rotation = 0.3f + 0.5f * static_cast<float>(smallRing - 1);

    auto [smallVertices, smallIndices] =
      Dashboard::generateSegmentedRing(smallRingRadius, smallRingHeight, 64, 4, 0.2f, rotation);
    Mesh smallRingMesh(smallVertices, smallIndices);
    smallRingMesh.setupMesh();
    smallSegmentedRings.push_back(std::move(smallRingMesh));
    smallRingHeights.push_back(smallRingHeight);

    float thickness =
      20.0f - (20.0f - 10.0f) * (static_cast<float>(smallRing - 1) / static_cast<float>(numSmallRings - 1));
    smallRingThicknesses.push_back(thickness);
  }

  float thickLineRadius = DASHBOARD_PLATE_RADIUS * (1.0f / static_cast<float>(numRings));
  float thickLineHeight = DASHBOARD_PLATE_Y + 0.15f;
  auto [thickLineVertices, thickLineIndices] = Dashboard::generateSingleRing(thickLineRadius, 64);
  Mesh thickLineMesh(thickLineVertices, thickLineIndices);
  thickLineMesh.setupMesh();
  float thickLineThickness = 25.0f;

  int gridSegments = static_cast<int>(SPHERE_GRID_SEGMENTS * SPHERE_GRID_UNIT_SIZE);
  auto [sphereGridVertices, sphereGridIndices] = Dashboard::generateSphereGrid(SPHERE_GRID_RADIUS, gridSegments);
  Mesh sphereGridMesh(sphereGridVertices, sphereGridIndices);
  sphereGridMesh.setupMesh();

  // ============================================
  // CO₂ DATA VISUALIZATION
  // ============================================
  auto countryCentroids = GeoBorders::computeCountryCentroids(borders, SPHERE_RADIUS);

  CO2DataSet co2Data;
  bool co2Loaded = co2Data.loadFromCsv("data/annual-co-emissions-by-region.csv");

  // ============================================
  // YEAR CONTROLLER
  // ============================================
  YearController yearController(co2Data);
  if (co2Loaded && MANUAL_YEAR != -1) {
    yearController.setYear(MANUAL_YEAR);
  }

  // ============================================
  // BREATHE ANIMATION
  // ============================================
  BreatheAnimation breatheAnimation;
  breatheAnimation.setMaxYear(yearController.getLatestYear());
  breatheAnimation.setDuration(3.0f);

  // ============================================
  // MARKER BUILDER
  // ============================================
  MarkerConfig markerConfig;
  markerConfig.minRadius = 0.02f;
  markerConfig.maxRadius = 0.5f;
  markerConfig.altitude = 0.02f;
  markerConfig.segments = 24;
  markerConfig.scaleFactor = 3.0e-10;
  MarkerBuilder markerBuilder(markerConfig);

  // ============================================
  // INITIAL MARKER CREATION
  // ============================================
  Mesh markerMesh;
  bool markersAvailable = false;
  double totalEmissionForYear = 0.0;
  Shader dataShader("shaders/data.vert", "shaders/data.frag");

  if (co2Loaded && yearController.getCurrentYear() > 0) {
    auto [mesh, totalEmission, success] =
      markerBuilder.buildMarkers(yearController.getCurrentYear(), co2Data, countryCentroids, SPHERE_RADIUS);
    if (success) {
      markerMesh = std::move(mesh);
      markersAvailable = true;
      totalEmissionForYear = totalEmission;
    }
  }

  // ============================================
  // TEXT LABELS
  // ============================================
  std::string globalCO2Label = "Global CO2: --";
  std::string yearLabelText = "Year: --";

  // ============================================
  // CAMERA SETUP
  // ============================================
  const float cameraDistance = 3.0f;
  float initialAzimuth = 0.0f;
  float initialElevation = 0.0f;
  float cameraAzimuth = initialAzimuth;
  float cameraElevation = initialElevation;

  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  auto updateCameraPosition = [&]() {
    float x = cameraDistance * std::cos(cameraElevation) * std::sin(cameraAzimuth);
    float y = cameraDistance * std::sin(cameraElevation);
    float z = cameraDistance * std::cos(cameraElevation) * std::cos(cameraAzimuth);
    return glm::vec3(x, y, z);
  };

  glm::vec3 cameraPos = updateCameraPosition();

  // ============================================
  // LIGHTING SETUP
  // ============================================
  glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 2.0f);

  // ============================================
  // MOUSE INPUT SETUP
  // ============================================
  bool isDragging = false;
  double lastMouseX = 0.0;
  double lastMouseY = 0.0;

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  MouseState mouseState;
  mouseState.isDragging = &isDragging;
  mouseState.lastMouseX = &lastMouseX;
  mouseState.lastMouseY = &lastMouseY;
  mouseState.cameraAzimuth = &cameraAzimuth;
  mouseState.cameraElevation = &cameraElevation;
  mouseState.initialAzimuth = &initialAzimuth;
  mouseState.initialElevation = &initialElevation;

  glfwSetWindowUserPointer(window, &mouseState);

  // ============================================
  // ANIMATION SETUP
  // ============================================
  float angle = 0.0f;
  mouseState.angle = &angle;

  while (!glfwWindowShouldClose(window)) {
    processInput(window, yearController, breatheAnimation);

    // Update breathe animation (may modify year during animation)
    bool shouldUpdateMarkers = false;
    int currentYear = yearController.getCurrentYear();
    breatheAnimation.update(glfwGetTime(), currentYear, shouldUpdateMarkers);
    if (shouldUpdateMarkers) {
      yearController.setYear(currentYear);
    }

    // Check if year changed and rebuild markers if needed
    static int lastYear = -1;
    currentYear = yearController.getCurrentYear();
    if (currentYear != lastYear && co2Loaded) {
      auto [mesh, totalEmission, success] =
        markerBuilder.buildMarkers(currentYear, co2Data, countryCentroids, SPHERE_RADIUS);
      if (success) {
        markerMesh = std::move(mesh);
        markersAvailable = true;
        totalEmissionForYear = totalEmission;
      } else {
        markersAvailable = false;
      }
      lastYear = currentYear;
    }

    // Update text labels
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

    // Get current window size
    int currentWidth, currentHeight;
    glfwGetFramebufferSize(window, &currentWidth, &currentHeight);

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ============================================
    // ANIMATION UPDATE
    // ============================================
    if (!isDragging) {
      angle += 0.01f;
    }

    cameraPos = updateCameraPosition();

    // ============================================
    // TRANSFORM MATRICES
    // ============================================
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glm::mat4 projection =
      glm::perspective(glm::radians(45.0f), (float)currentWidth / (float)currentHeight, 0.1f, 100.0f);

    // ============================================
    // MODEL TRANSFORMATION
    // ============================================
    float breatheScale = breatheAnimation.getScale();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(breatheScale));
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    // ============================================
    // SHADER SETUP
    // ============================================
    shader.use();

    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("viewPos", cameraPos);

    // ============================================
    // SPHERE VISUAL PROPERTIES
    // ============================================
    shader.setVec3("sphereColor", glm::vec3(0.2f, 0.5f, 1.0f));

    // Draw sphere
    sphere.draw();

    // ============================================
    // SPHERE GRID BACKGROUND
    // ============================================
    borderShader.use();
    borderShader.setVec3("borderColor", glm::vec3(0.4f, 0.7f, 1.0f));
    borderShader.setFloat("uTime", static_cast<float>(glfwGetTime()));
    borderShader.setFloat("uGlowIntensity", 1.0f);
    borderShader.setFloat("uAltitudeOffset", 0.0f);
    borderShader.setFloat("uLineDepth", THICK_LINE_DEPTH);
    borderShader.setMat4("view", view);
    borderShader.setMat4("projection", projection);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glm::mat4 sphereGridModel = glm::mat4(1.0f);
    borderShader.setMat4("model", sphereGridModel);
    borderShader.setFloat("uLineWidth", 2.0f);
    borderShader.setFloat("uAlphaMultiplier", 0.6f);
    sphereGridMesh.drawLineStrip();
    borderShader.setFloat("uAlphaMultiplier", 1.0f);

    // ============================================
    // DASHBOARD RENDERING
    // ============================================
    borderShader.setVec3("borderColor", glm::vec3(0.4f, 0.7f, 1.0f));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glm::mat4 plateModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, DASHBOARD_PLATE_Y, 0.0f));
    borderShader.setMat4("model", plateModel);

    // Filled disc with radial gradient
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    discShader.use();
    discShader.setMat4("model", plateModel);
    discShader.setMat4("view", view);
    discShader.setMat4("projection", projection);
    discShader.setVec3("discColor", glm::vec3(0.4f, 0.7f, 1.0f));
    discShader.setFloat("uAlphaMultiplier", 1.0f);
    dashboardDiscMesh.draw();

    // Bottom plate rings (3D ring + thin line rings)
    for (int i = dashboardRings.size() - 1; i >= 0; --i) {
      if (ringIs3D[i]) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ring3DShader.use();
        ring3DShader.setMat4("model", plateModel);
        ring3DShader.setMat4("view", view);
        ring3DShader.setMat4("projection", projection);
        ring3DShader.setVec3("ringColor", glm::vec3(0.4f, 0.7f, 1.0f));
        ring3DShader.setFloat("uAlphaMultiplier", 0.10f);
        dashboardRings[i].draw();
      } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        borderShader.use();
        borderShader.setMat4("model", plateModel);
        borderShader.setMat4("view", view);
        borderShader.setMat4("projection", projection);
        borderShader.setFloat("uLineWidth", ringThicknesses[i]);
        dashboardRings[i].drawLineStrip();
      }
    }

    // Thick line above plate
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glm::mat4 thickLineModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, thickLineHeight, 0.0f));
    borderShader.setMat4("model", thickLineModel);
    borderShader.setFloat("uLineWidth", thickLineThickness);
    thickLineMesh.drawLineStrip();

    // Small segmented rings between sphere and plate
    borderShader.use();
    borderShader.setMat4("view", view);
    borderShader.setMat4("projection", projection);
    for (size_t i = 0; i < smallSegmentedRings.size(); ++i) {
      glm::mat4 smallModel = glm::mat4(1.0f);
      borderShader.setMat4("model", smallModel);
      if (smallRingThicknesses[i] > 20.0f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      }
      borderShader.setFloat("uLineWidth", smallRingThicknesses[i]);
      smallSegmentedRings[i].drawLineStrip();
    }

    // Elevated segmented rings near bottom of sphere
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glm::mat4 elevatedModel = glm::mat4(1.0f);
    borderShader.setMat4("model", elevatedModel);
    borderShader.setFloat("uLineWidth", 25.0f);
    dashboardElevatedMesh.drawLineStrip();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // ============================================
    // COUNTRY BORDER RENDERING
    // ============================================
    borderShader.use();
    borderShader.setMat4("model", model);
    borderShader.setMat4("view", view);
    borderShader.setMat4("projection", projection);
    borderShader.setVec3("borderColor", glm::vec3(0.4f, 0.7f, 1.0f));
    borderShader.setFloat("uTime", static_cast<float>(glfwGetTime()));

    // Blue glow layer
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    borderShader.setFloat("uGlowIntensity", 1.0f);
    borderShader.setFloat("uLineWidth", 7.0f);
    borderShader.setFloat("uAltitudeOffset", 0.0f);
    borderMesh.drawLineStrip();

    // White center line layer
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    borderSimpleShader.use();
    borderSimpleShader.setMat4("model", model);
    borderSimpleShader.setMat4("view", view);
    borderSimpleShader.setMat4("projection", projection);
    borderSimpleShader.setFloat("uAltitudeOffset", 0.001f);
    glLineWidth(1.0f);
    borderMesh.drawLineStrip();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // ============================================
    // CO₂ MARKER RENDERING
    // ============================================
    if (markersAvailable) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDepthMask(GL_FALSE);

      dataShader.use();
      dataShader.setMat4("model", model);
      dataShader.setMat4("view", view);
      dataShader.setMat4("projection", projection);
      dataShader.setFloat("uTime", angle);
      markerMesh.draw();

      glDepthMask(GL_TRUE);
    }

    // ============================================
    // TEXT RENDERING
    // ============================================
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render text
    textRenderer.RenderText("The Breathing Planet", windowWidth / 2.0f - 350.0f, windowHeight - 80.0f, 1.0f,
                            glm::vec3(0.9f, 0.9f, 0.9f));
    textRenderer.RenderText("Breathe", windowWidth / 2.0f - 80.0f, 80.0f, 0.7f, glm::vec3(0.9f, 0.9f, 0.9f));
    textRenderer.RenderText(globalCO2Label, 50.0f, 100.0f, 0.5f, glm::vec3(0.9f, 0.9f, 0.9f));
    textRenderer.RenderText(yearLabelText, 50.0f, 65.0f, 0.5f, glm::vec3(0.9f, 0.9f, 0.9f));

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow *window, YearController &yearController, BreatheAnimation &breatheAnimation) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  static bool bPressed = false;
  if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
    if (!bPressed && !breatheAnimation.getIsActive()) {
      int currentYear = yearController.getCurrentYear();
      int latestYear = yearController.getLatestYear();
      if (currentYear < latestYear) {
        breatheAnimation.start(currentYear, YEAR_INCREMENT);
        bPressed = true;
      }
    }
  } else {
    bPressed = false;
  }

  static bool rPressed = false;
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    if (!rPressed) {
      MouseState *state = static_cast<MouseState *>(glfwGetWindowUserPointer(window));
      if (state) {
        *state->cameraAzimuth = *state->initialAzimuth;
        *state->cameraElevation = *state->initialElevation;
      }
      rPressed = true;
    }
  } else {
    rPressed = false;
  }

  if (!breatheAnimation.getIsActive()) {
    static bool leftPressed = false;
    static bool rightPressed = false;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
      if (!leftPressed) {
        yearController.decrementYear(YEAR_INCREMENT);
        leftPressed = true;
      }
    } else {
      leftPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      if (!rightPressed) {
        yearController.incrementYear(YEAR_INCREMENT);
        rightPressed = true;
      }
    } else {
      rightPressed = false;
    }
  }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    MouseState *state = static_cast<MouseState *>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
      *state->isDragging = true;
      glfwGetCursorPos(window, state->lastMouseX, state->lastMouseY);
    } else if (action == GLFW_RELEASE) {
      *state->isDragging = false;
    }
  }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  MouseState *state = static_cast<MouseState *>(glfwGetWindowUserPointer(window));

  if (*state->isDragging) {
    double deltaX = xpos - *state->lastMouseX;
    double deltaY = ypos - *state->lastMouseY;

    const float sensitivity = 0.005f;
    *state->cameraAzimuth -= static_cast<float>(deltaX) * sensitivity;
    *state->cameraElevation += static_cast<float>(deltaY) * sensitivity;

    const float maxElevation = glm::half_pi<float>() - 0.1f;
    *state->cameraElevation = std::clamp(*state->cameraElevation, -maxElevation, maxElevation);

    *state->lastMouseX = xpos;
    *state->lastMouseY = ypos;
  }
}
