#pragma once
#include "UI/GUI.h"
#include "generic/ParserUtils.h"
#include "utils/Logging.h"
#include "generic/ScenarioUtils.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <string>
namespace Scenario {
class GenericModule {
protected:
  int numCountries;
  bool cut;
  Utils::Pathcfg pathcfg;

  void configurePaths(const std::string &username, const std::string &gameName,
                      const boost::property_tree::ptree &gamesConf);
  void createPaths(const std::string &basePath);
  // try to locate hoi4 at configured path, if not found, try other
  // standard locations
  bool findGame(std::string &path, const std::string &game);
  // check if configured mod directories are correct
  bool findModFolders();
};
} // namespace Scenario