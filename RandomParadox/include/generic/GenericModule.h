#pragma once
#include "generic/ScenarioGenerator.h"
#include "generic/ParserUtils.h"
#include "generic/ScenarioUtils.h"
#include "utils/Logging.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <string>
namespace Scenario {
class GenericModule {
protected:
  bool cut;

  void configurePaths(const std::string &username, const std::string &gameName,
                      const boost::property_tree::ptree &gamesConf);


public:
  virtual bool createPaths() =0;
  std::shared_ptr<Scenario::Generator> generator;
  int numCountries;
  // try to locate hoi4 at configured path, if not found, try other
  // standard locations
  bool findGame(std::string &path, const std::string &game);
  // check if configured mod directory of the game is correct
  bool validateGameModFolder(const std::string &game);
  bool autoLocateGameModFolder(const std::string &game);
  // check if configured mod location directory is correct
  bool validateModFolder(const std::string &game);
  Utils::Pathcfg pathcfg;
};
} // namespace Scenario