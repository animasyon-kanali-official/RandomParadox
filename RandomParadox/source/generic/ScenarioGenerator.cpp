#include "generic/ScenarioGenerator.h"
namespace Logging = Fwg::Utils::Logging;
namespace Scenario {
using namespace Fwg::Gfx;
Generator::Generator() {}

Generator::Generator(const std::string &configSubFolder)
    : FastWorldGenerator(configSubFolder) {
  Gfx::Flag::readColourGroups();
  Gfx::Flag::readFlagTypes();
  Gfx::Flag::readFlagTemplates();
  Gfx::Flag::readSymbolTemplates();
}

Generator::~Generator() {}

void Generator::loadRequiredResources(const std::string &gamePath) {}

void Generator::generateWorldCivilizations() {
  typeMap = mapTerrain();
  generatePopulations();
  generateDevelopment();
  generateReligions();
  generateCultures();
  for (auto &region : gameRegions) {
    region->sumPopulations();
  }
}

void Generator::mapContinents() {
  Logging::logLine("Mapping Continents");
  for (const auto &continent : this->areas.continents) {
    // we copy the fwg continents by choice, to leave them untouched
    pdoxContinents.push_back(ScenarioContinent(continent));
  }
}

void Generator::mapRegions() {
  Logging::logLine("Mapping Regions");
  for (auto &region : this->areas.regions) {
    std::sort(region.provinces.begin(), region.provinces.end(),
              [](const Fwg::Province *a, const Fwg::Province *b) {
                return (*a < *b);
              });
    auto gameRegion = std::make_shared<Region>(region);
    for (auto &baseRegion : gameRegion->neighbours)
      gameRegion->neighbours.push_back(baseRegion);
    // generate random name for region
    gameRegion->name = NameGeneration::generateName(nData);
    for (auto &province : gameRegion->provinces) {
      gameRegion->gameProvinces.push_back(gameProvinces[province->ID]);
    }
    // save game region
    gameRegions.push_back(gameRegion);
  }
  // check if we have the same amount of gameProvinces as FastWorldGen provinces
  if (gameProvinces.size() != this->areas.provinces.size())
    throw(std::exception("Fatal: Lost provinces, terminating"));
  if (gameRegions.size() != this->areas.regions.size())
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
  for (auto &prov : this->areas.provinces) {
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
  const auto &popMap = this->populationMap;
  const auto &cityMap = this->cityMap;
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

void Generator::generateReligions() {
  auto &config = Fwg::Cfg::Values();
  Bitmap religionMap(config.width, config.height, 24);
  for (int i = 0; i < 8; i++) {
    Religion r;
    r.name = NameGeneration::generateName(this->nData);
    do {
      r.centerOfReligion = Fwg::Utils::selectRandom(gameProvinces)->ID;
    } while (!gameProvinces[r.centerOfReligion]->baseProvince->isLand());
    r.colour.randomize();
    religions.push_back(std::make_shared<Religion>(r));
  }

  for (auto &gameProvince : gameProvinces) {
    if (!gameProvince->baseProvince->isLand())
      continue;
    auto closestReligion = 0;
    auto distance = 100000000.0;
    for (auto x = 0; x < religions.size(); x++) {
      auto &religion = religions[x];
      auto religionCenter = gameProvinces[religion->centerOfReligion];
      auto nDistance = Fwg::getPositionDistance(
          religionCenter->baseProvince->position,
          gameProvince->baseProvince->position, config.width);
      if (Fwg::Utils::switchIfComparator(nDistance, distance, std::less())) {
        closestReligion = x;
      }
    }
    // add only the main religion at this time
    gameProvince->religions[religions[closestReligion]] = 1.0;
    for (auto pix : gameProvince->baseProvince->pixels) {
      religionMap.setColourAtIndex(pix, religions[closestReligion]->colour);
    }
  }
  Png::save(religionMap, "Maps/world/religions.png");
}

void Generator::generateCultures() {
  auto &config = Fwg::Cfg::Values();
  Bitmap cultureMap(config.width, config.height, 24);
  for (int i = 0; i < 200; i++) {
    Culture r;
    r.name = NameGeneration::generateName(this->nData);
    do {
      r.centerOfCulture = Fwg::Utils::selectRandom(gameProvinces)->ID;
    } while (!gameProvinces[r.centerOfCulture]->baseProvince->isLand());
    auto presentReligions = gameProvinces[r.centerOfCulture]->religions;

    using pair_type = decltype(presentReligions)::value_type;

    auto pr = std::max_element(std::begin(presentReligions),
                               std::end(presentReligions),
                               [](const pair_type &p1, const pair_type &p2) {
                                 return p1.second < p2.second;
                               });

    r.primaryReligion = pr->first;

    r.colour.randomize();
    cultures.push_back(std::make_shared<Culture>(r));
  }

  for (auto &gameProvince : gameProvinces) {
    if (gameProvince->baseProvince->sea || gameProvince->baseProvince->isLake)
      continue;
    auto closestCulture = 0;
    auto distance = 100000000.0;
    for (auto x = 0; x < cultures.size(); x++) {
      auto &culture = cultures[x];
      auto cultureCenter = gameProvinces[culture->centerOfCulture];
      auto nDistance = Fwg::getPositionDistance(
          cultureCenter->baseProvince->position,
          gameProvince->baseProvince->position, config.width);
      if (Fwg::Utils::switchIfComparator(nDistance, distance, std::less())) {
        closestCulture = x;
      }
    }
    // add only the main culture at this time
    gameProvince->cultures[cultures[closestCulture]] = 1.0;
    for (auto pix : gameProvince->baseProvince->pixels) {
      cultureMap.setColourAtIndex(pix, cultures[closestCulture]->colour);
    }
  }
  Png::save(cultureMap, "Maps/world/cultures.png");
}

void Generator::generateDevelopment() {
  // high pop-> high development
  // high city share->high dev
  // terrain type?
  // .....
  Logging::logLine("Generating State Development");
  auto &config = Fwg::Cfg::Values();
  Bitmap development(config.width, config.height, 24);
  const auto &cityMap = this->cityMap;
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

        for (auto pix : gameProv->baseProvince->pixels) {
          development.setColourAtIndex(pix, gameProv->devFactor * 255.0);
        }
      }
  Png::save(development, "Maps/world/development.png");
}

Fwg::Gfx::Bitmap Generator::mapTerrain() {
  const auto &climateMap = this->climateMap;
  Bitmap typeMap(climateMap.bInfoHeader.biWidth,
                 climateMap.bInfoHeader.biHeight, 24);
  auto &colours = Fwg::Cfg::Values().colours;
  typeMap.fill(colours.at("sea"));
  Logging::logLine("Mapping Terrain");
  for (auto &c : countries) {
    for (auto &gameRegion : c.second.ownedRegions) {
      for (auto &gameProv : gameRegions[gameRegion]->gameProvinces) {
        gameProv->terrainType =
            terrainTypeToString.at(gameProv->baseProvince->terrainType);

        if (climateMap.imageData.size()) {
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
      }
    }
  }
  Png::save(typeMap, "Maps/typeMap.png");
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
    Country pdoxC(NameGeneration::generateTag(name, nData), i, name,
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
      countries.at(gR->owner).addRegion(gameRegion, gameRegions, gameProvinces);
    }
  }
}

void Generator::generateStrategicRegions() {
  Fwg::Utils::Logging::logLine(
      "Scenario: Dividing world into strategic regions");
  std::set<int> assignedIdeas;
  for (auto &region : gameRegions) {
    if (assignedIdeas.find(region->ID) == assignedIdeas.end()) {
      strategicRegion sR;
      // std::set<int>stratRegion;
      sR.gameRegionIDs.insert(region->ID);
      assignedIdeas.insert(region->ID);
      for (auto &neighbour : region->neighbours) {
        // should be equal in sea/land
        if (neighbour > gameRegions.size())
          continue;
        if (gameRegions[neighbour]->sea == region->sea &&
            assignedIdeas.find(neighbour) == assignedIdeas.end()) {
          sR.gameRegionIDs.insert(neighbour);
          assignedIdeas.insert(neighbour);
        }
      }
      sR.name = NameGeneration::generateName(nData);
      strategicRegions.push_back(sR);
    }
  }
  Bitmap stratRegionBMP(Fwg::Cfg::Values().width, Fwg::Cfg::Values().height,
                        24);
  for (auto &strat : strategicRegions) {
    Colour c{static_cast<unsigned char>(RandNum::getRandom(255)),
             static_cast<unsigned char>(RandNum::getRandom(255)),
             static_cast<unsigned char>(RandNum::getRandom(255))};
    for (auto &reg : strat.gameRegionIDs) {
      c.setBlue(gameRegions[reg]->sea ? 255 : 0);
      for (auto &prov : gameRegions[reg]->gameProvinces) {
        for (auto &pix : prov->baseProvince->pixels) {
          stratRegionBMP.setColourAtIndex(pix, c);
        }
      }
    }
  }
  Bmp::bufferBitmap("strat", stratRegionBMP);
  Bmp::save(stratRegionBMP, "Maps\\stratRegions.bmp");
}

void Generator::evaluateNeighbours() {
  Logging::logLine("Evaluating Country Neighbours");
  for (auto &c : countries)
    for (const auto &gR : c.second.ownedRegions)
      for (const auto &neighbourRegion : gameRegions[gR]->neighbours)
        // TO DO: Investigate rare crash issue with index being out of range
        if (neighbourRegion < gameRegions.size() &&
            gameRegions[neighbourRegion]->owner != c.first)
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