#pragma once
#include "Hoi4Generator.h"
#include "Hoi4Parsing.h"
#include "generic/ParserUtils.h"
#include "utils/Bitmap.h"
#include "utils/Logging.h"
#include <iostream>
namespace Scenario::Hoi4::MapPainting {

struct ChangeHolder {
  std::set<int> deletedProvs;
  std::set<int> changedProvs;
  std::map<int, int> provIdMapping;

  std::set<int> deletedStates;
  std::set<int> changedStates;
  std::map<int, int> stateIdMapping;

  std::set<std::string> deletedCountries;
  std::set<std::string> changedCountries;
  std::map<std::string, std::string> countryTagMapping;
};

namespace Detail {

void stateBitmap(const std::string &inPath, Fwg::Gfx::Bitmap countries,
                 const std::vector<Fwg::Province> &provinces,
                 const std::vector<Region> &states);
} // namespace Detail
namespace Countries {
void edit(const std::string &inPath, const std::string &outputPath,
          const std::string &mapName, Generator &hoi4Gen,
          ChangeHolder &changes);

}
namespace Provinces {
void edit(const std::string &inPath, const std::string &outputPath,
          const std::string &mapName, Generator &hoi4Gen,
          ChangeHolder &changes);
}

}; // namespace Scenario::Hoi4::MapPainting
