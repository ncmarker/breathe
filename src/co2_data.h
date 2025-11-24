#pragma once

#include <limits>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

struct CountryEmissionRecord {
  std::string isoCode;
  std::map<int, double>
      emissionsByYear; // year -> annual CO2 emissions (tonnes)
};

class CO2DataSet {
public:
  bool loadFromCsv(const std::string &csvPath);

  std::optional<double> getEmission(const std::string &isoCode, int year) const;

  int earliestYear() const { return minYear; }
  int latestYear() const { return maxYear; }

  const std::unordered_map<std::string, CountryEmissionRecord> &
  records() const {
    return data;
  }

private:
  std::unordered_map<std::string, CountryEmissionRecord> data;
  int minYear = std::numeric_limits<int>::max();
  int maxYear = std::numeric_limits<int>::min();
};
