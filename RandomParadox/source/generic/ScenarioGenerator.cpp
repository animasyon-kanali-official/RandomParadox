#include "generic/ScenarioGenerator.h"
namespace Logging = Fwg::Utils::Logging;
namespace Scenario {
using namespace Fwg::Gfx;
Generator::Generator() {}
Generator::Generator(Fwg::FastWorldGenerator &fwg) : fwg(fwg) {
  Gfx::Flag::readColourGroups();
  Gfx::Flag::readFlagTypes();
  Gfx::Flag::readFlagTemplates();
  Gfx::Flag::readSymbolTemplates();
}

Generator::~Generator() {}

void Generator::loadRequiredResources(const std::string &gamePath) {
  bitmaps["provinces"] = ResourceLoading::loadProvinceMap(gamePath);
  bitmaps["heightmap"] = ResourceLoading::loadHeightMap(gamePath);
}

void Generator::generateWorld() {
  typeMap = mapTerrain();
  generatePopulations();
  generateDevelopment();
}

void Generator::mapContinents() {
  Logging::logLine("Mapping Continents");
  for (const auto &continent : fwg.areas.continents) {
    // we copy the fwg continents by choice, to leave them untouched
    pdoxContinents.push_back(PdoxContinent(continent));
  }
}

void Generator::mapRegions() {
  Logging::logLine("Mapping Regions");
  for (auto &region : fwg.areas.regions) {
    std::sort(region.provinces.begin(), region.provinces.end(),
              [](const Fwg::Province *a, const Fwg::Province *b) {
                return (*a < *b);
              });
    auto gR = std::make_shared<Region>(region);
    for (auto &baseRegion : gR->neighbours)
      gR->neighbours.push_back(baseRegion);
    // generate random name for region
    gR->name = NameGeneration::generateName(nData);
    for (auto &province : gR->provinces) {
      gR->gameProvinces.push_back(gameProvinces[province->ID]);
    }
    // save game region
    gameRegions.push_back(gR);
  }
  // check if we have the same amount of gameProvinces as FastWorldGen provinces
  if (gameProvinces.size() != fwg.areas.provinces.size())
    throw(std::exception("Fatal: Lost provinces, terminating"));
  if (gameRegions.size() != fwg.areas.regions.size())
    throw(std::exception("Fatal: Lost regions, terminating"));
  for (const auto &gameRegion : gameRegions) {
    if (gameRegion->ID > gameRegions.size()) {
      throw(std::exception("Fatal: Invalid region IDs, terminating"));
    }
  }
  // sort by gameprovince ID
  std::sort(gameRegions.begin(), gameRegions.end(),
            [](auto l, auto r) { return *l < *r; });
}

void Generator::mapProvinces() {
  gameProvinces.clear();
  for (auto &prov : fwg.areas.provinces) {
    // edit coastal status: lakes are not coasts!
    if (prov->coastal && prov->isLake)
      prov->coastal = false;
    // if it is a land province, check that a neighbour is an ocean, otherwise
    // this isn't coastal in this scenario definition
    else if (prov->coastal) {
      bool foundTrueCoast = false;
      for (auto &neighbour : prov->neighbours) {
        if (neighbour->sea) {
          foundTrueCoast = true;
        }
      }
      prov->coastal = foundTrueCoast;
    }

    // now create gameprovinces from FastWorldGen provinces
    auto gP = std::make_shared<GameProvince>(prov);
    // also copy neighbours
    for (auto &baseProvinceNeighbour : gP->baseProvince->neighbours)
      gP->neighbours.push_back(baseProvinceNeighbour);
    // give name to province
    gP->name = NameGeneration::generateName(nData);
    gameProvinces.push_back(gP);
  }
  // sort by gameprovince ID
  std::sort(gameProvinces.begin(), gameProvinces.end(),
            [](auto l, auto r) { return *l < *r; });
}

void Generator::generatePopulations() {
  Logging::logLine("Generating Population");
  auto &config = Fwg::Cfg::Values();
  const auto &popMap = fwg.populationMap;
  const auto &cityMap = fwg.cityMap;
  for (auto &c : countries)
    for (auto &gR : c.second.ownedRegions)
      for (auto &gProv : gameRegions[gR]->gameProvinces) {
        // calculate the population factor
        gProv->popFactor =
            0.1 + popMap[gProv->baseProvince->position.weightedCenter] /
                      config.colours["population"];
        int cityPixels = 0;
        // calculate share of province that is a city
        for (auto pix : gProv->baseProvince->pixels)
          if (cityMap[pix].isShadeOf(config.colours["cities"]))
            cityPixels++;
        gProv->cityShare =
            (double)cityPixels / gProv->baseProvince->pixels.size();
      }
}

void Generator::generateDevelopment() {
  // high pop-> high development
  // high city share->high dev
  // terrain type?
  // .....
  Logging::logLine("Generating State Development");
  auto &config = Fwg::Cfg::Values();
  const auto &cityMap = fwg.cityMap;
  for (auto &c : countries)
    for (auto &gR : c.second.ownedRegions)
      for (auto &gameProv : gameRegions[gR]->gameProvinces) {
        auto cityDensity = 0.0;
        // calculate development with density of city and population factor
        if (gameProv->baseProvince->cityPixels.size())
          cityDensity = cityMap[gameProv->baseProvince->cityPixels[0]] /
                        config.colours["cities"];
        gameProv->devFactor =
            std::clamp(0.2 + 0.5 * gameProv->popFactor +
                           1.0 * gameProv->cityShare * cityDensity,
                       0.0, 1.0);
      }
}

Fwg::Gfx::Bitmap Generator::mapTerrain() {
  const auto &climateMap = fwg.climateMap;
  Bitmap typeMap(climateMap.bInfoHeader.biWidth,
                 climateMap.bInfoHeader.biHeight, 24);
  Logging::logLine("Mapping Terrain");
  auto &colours = Fwg::Cfg::Values().colours;
  for (auto &c : countries)
    for (auto &gameRegion : c.second.ownedRegions)
      for (auto &gameProv : gameRegions[gameRegion]->gameProvinces) {
        gameProv->terrainType =
            terrainTypeToString.at(gameProv->baseProvince->terrainType);
        auto tType = gameProv->terrainType;
        for (auto pix : gameProv->baseProvince->pixels) {
          if (tType == "jungle")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{255, 255, 0});
          else if (tType == "forest")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{0, 255, 0});
          else if (tType == "hills")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{128, 128, 128});
          else if (tType == "grassland")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{0, 255, 128});
          else if (tType == "savannah")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{0, 255, 128});
          else if (tType == "desert")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{0, 255, 255});
          else if (tType == "mountain")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{255, 255, 255});
          else if (tType == "peaks")
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{255, 255, 255});
          else if (tType == "lakes")
            typeMap.setColourAtIndex(pix, colours.at("lake"));
          else
            typeMap.setColourAtIndex(pix, Fwg::Gfx::Colour{255, 0, 0});
        }
      }
  Bmp::save(typeMap, "Maps/typeMap.bmp");
  return typeMap;
}

std::shared_ptr<Region> &Generator::findStartRegion() {
  std::vector<std::shared_ptr<Region>> freeRegions;
  for (const auto &gameRegion : gameRegions)
    if (!gameRegion->assigned && !gameRegion->sea)
      freeRegions.push_back(gameRegion);

  if (freeRegions.size() == 0)
    return gameRegions[0];

  const auto &startRegion = Fwg::Utils::selectRandom(freeRegions);
  return gameRegions[startRegion->ID];
}

// generate countries according to given ruleset for each game
// TODO: rulesets, e.g. naming schemes? tags? country size?
void Generator::generateCountries(int numCountries,
                                  const std::string &gamePath) {
  auto &config = Fwg::Cfg::Values();
  this->numCountries = numCountries;
  Logging::logLine("Generating Countries");
  // load tags from hoi4 that are used by the base game
  // do not use those to avoid conflicts

  for (int i = 0; i < numCountries; i++) {
    auto name{NameGeneration::generateName(nData)};
    PdoxCountry pdoxC(NameGeneration::generateTag(name, nData), i, name,
                      NameGeneration::generateAdjective(name, nData),
                      Gfx::Flag(82, 52));
    // randomly set development of countries
    pdoxC.developmentFactor = RandNum::getRandom(0.1, 1.0);
    countries.emplace(pdoxC.tag, pdoxC);
  }
  for (auto &pdoxCountry : countries) {
    auto startRegion(findStartRegion());
    if (startRegion->assigned || startRegion->sea)
      continue;
    pdoxCountry.second.assignRegions(6, gameRegions, startRegion,
                                     gameProvinces);
  }
  for (auto &gameRegion : gameRegions) {
    if (!gameRegion->sea && !gameRegion->assigned) {
      auto gR = Fwg::Utils::getNearestAssignedLand(gameRegions, gameRegion,
                                                   config.width, config.height);
      std::cout << gR->owner << std::endl;
      countries.at(gR->owner).addRegion(gameRegion, gameRegions, gameProvinces);
    }
  }
}

void Generator::evaluateNeighbours() {
  Logging::logLine("Evaluating Country Neighbours");
  for (auto &c : countries)
    for (const auto &gR : c.second.ownedRegions)
      for (const auto &neighbourRegion : gameRegions[gR]->neighbours)
        if (gameRegions[neighbourRegion]->owner != c.first)
          c.second.neighbours.insert(gameRegions[neighbourRegion]->owner);
}

Bitmap Generator::dumpDebugCountrymap(const std::string &path) {
  Logging::logLine("Mapping Continents");
  auto &config = Fwg::Cfg::Values();
  Bitmap countryBMP(config.width, config.height, 24);
  for (const auto &country : countries)
    for (const auto &region : country.second.ownedRegions)
      for (const auto &prov : gameRegions[region]->provinces)
        for (const auto &pix : prov->pixels)
          countryBMP.setColourAtIndex(pix, country.second.colour);

  Bmp::save(countryBMP, (path).c_str());
  return countryBMP;
}
} // namespace Scenario