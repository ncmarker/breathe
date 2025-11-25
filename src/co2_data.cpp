#include "co2_data.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

void trim(std::string &s) {
  auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };

  s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
  s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
}

std::string toUpperCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return value;
}

} // namespace

bool CO2DataSet::loadFromCsv(const std::string &csvPath) {
  std::ifstream file(csvPath);
  if (!file.is_open()) {
    std::cerr << "ERROR: Could not open CO2 dataset: " << csvPath << std::endl;
    return false;
  }

  data.clear();
  minYear = std::numeric_limits<int>::max();
  maxYear = std::numeric_limits<int>::min();

  std::string line;
  // Skip header
  if (!std::getline(file, line)) {
    std::cerr << "ERROR: CO2 dataset is empty: " << csvPath << std::endl;
    return false;
  }

  size_t lineNumber = 1;
  while (std::getline(file, line)) {
    ++lineNumber;
    if (line.empty())
      continue;

    std::stringstream ss(line);
    std::string entity, code, yearStr, valueStr;

    if (!std::getline(ss, entity, ','))
      continue;
    if (!std::getline(ss, code, ','))
      continue;
    if (!std::getline(ss, yearStr, ','))
      continue;
    if (!std::getline(ss, valueStr))
      continue;

    trim(code);
    trim(yearStr);
    trim(valueStr);

    if (code.empty())
      continue; // skip aggregated entries without ISO code

    int year = 0;
    try {
      year = std::stoi(yearStr);
    } catch (const std::exception &) {
      std::cerr << "WARNING: Invalid year at line " << lineNumber << " (" << yearStr << ")\n";
      continue;
    }

    double value = 0.0;
    if (valueStr.empty())
      continue;

    try {
      value = std::stod(valueStr);
    } catch (const std::exception &) {
      std::cerr << "WARNING: Invalid emission value at line " << lineNumber << " (" << valueStr << ")\n";
      continue;
    }

    std::string iso = toUpperCopy(code);
    auto &record = data[iso];
    if (record.isoCode.empty()) {
      record.isoCode = iso;
    }

    record.emissionsByYear[year] = value;
    minYear = std::min(minYear, year);
    maxYear = std::max(maxYear, year);
  }

  if (data.empty()) {
    std::cerr << "WARNING: No CO2 records loaded from " << csvPath << std::endl;
    return false;
  }

  if (minYear == std::numeric_limits<int>::max())
    minYear = 0;
  if (maxYear == std::numeric_limits<int>::min())
    maxYear = 0;

  return true;
}

double CO2DataSet::getGlobalMaxEmission() const {
  double maxEmission = 0.0;
  for (const auto &record : data) {
    for (const auto &yearEmission : record.second.emissionsByYear) {
      maxEmission = std::max(maxEmission, yearEmission.second);
    }
  }
  return maxEmission;
}

std::optional<double> CO2DataSet::getEmission(const std::string &isoCode, int year) const {
  if (isoCode.empty())
    return std::nullopt;

  std::string iso = toUpperCopy(isoCode);
  auto recordIt = data.find(iso);
  if (recordIt == data.end())
    return std::nullopt;

  const auto &yearMap = recordIt->second.emissionsByYear;
  auto valueIt = yearMap.find(year);
  if (valueIt == yearMap.end())
    return std::nullopt;

  return valueIt->second;
}
