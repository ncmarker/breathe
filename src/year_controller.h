#pragma once

#include "co2_data.h"

// Handles year selection and navigation for CO2 data visualization
class YearController {
public:
  YearController(const CO2DataSet &co2Data);

  // Get current year
  int getCurrentYear() const {
    return currentYear;
  }

  // Navigate years
  void incrementYear(int delta = 5);
  void decrementYear(int delta = 5);
  void setYear(int year);

  // Check if year is valid
  bool isValidYear(int year) const;

  // Get year range
  int getEarliestYear() const {
    return earliestYear;
  }
  int getLatestYear() const {
    return latestYear;
  }

private:
  const CO2DataSet &co2Data;
  int currentYear;
  int earliestYear;
  int latestYear;

  void clampYear();
};
