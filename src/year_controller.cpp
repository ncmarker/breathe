#include "year_controller.h"
#include <algorithm>

YearController::YearController(const CO2DataSet &co2Data)
    : co2Data(co2Data), currentYear(co2Data.latestYear()), earliestYear(co2Data.earliestYear()),
      latestYear(co2Data.latestYear()) {
  clampYear();
}

void YearController::incrementYear(int delta) {
  currentYear += delta;
  clampYear();
}

void YearController::decrementYear(int delta) {
  currentYear -= delta;
  clampYear();
}

void YearController::setYear(int year) {
  currentYear = year;
  clampYear();
}

bool YearController::isValidYear(int year) const {
  return year >= earliestYear && year <= latestYear;
}

void YearController::clampYear() {
  currentYear = std::max(earliestYear, std::min(currentYear, latestYear));
}
