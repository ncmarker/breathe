#pragma once

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

class BreatheAnimation {
public:
  BreatheAnimation() : isActive(false), startTime(0.0), duration(3.0f), startYear(0), targetYear(0) {}

  void start(int currentYear, int yearIncrement) {
    isActive = true;
    startTime = glfwGetTime();
    startYear = currentYear;
    targetYear = std::min(currentYear + yearIncrement, maxYear);
  }

  void stop() {
    isActive = false;
  }

  void update(double currentTime, int &currentYear, bool &shouldUpdateMarkers) {
    if (!isActive)
      return;

    float elapsed = static_cast<float>(currentTime - startTime);
    float progress = std::min(elapsed / duration, 1.0f);

    if (progress >= 1.0f) {
      currentYear = targetYear;
      shouldUpdateMarkers = true;
      isActive = false;
      return;
    }

    float t = progress;
    int newYear = static_cast<int>(startYear + (targetYear - startYear) * t);
    if (newYear != currentYear) {
      currentYear = newYear;
      shouldUpdateMarkers = true;
    }
  }

  float getScale() const {
    if (!isActive)
      return 1.0f;

    float elapsed = static_cast<float>(glfwGetTime() - startTime);
    float progress = std::min(elapsed / duration, 1.0f);

    float breathingPhase = progress * glm::pi<float>();
    float scale = 1.0f + 0.1f * std::sin(breathingPhase);
    return scale;
  }

  bool getIsActive() const {
    return isActive;
  }

  void setMaxYear(int max) {
    maxYear = max;
  }
  void setDuration(float dur) {
    duration = dur;
  }

private:
  bool isActive;
  double startTime;
  float duration;
  int startYear;
  int targetYear;
  int maxYear = 3000;
};
