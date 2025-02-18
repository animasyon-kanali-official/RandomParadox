#include "generic/GenericModule.h"
namespace Scenario {
bool GenericModule::createPaths() { // mod directory
  return false;
}
// a method to search for the original game files on the hard drive(s)
bool GenericModule::findGame(std::string &path, const std::string &game) {
  using namespace std::filesystem;
  namespace Logging = Fwg::Utils::Logging;
  std::vector<std::string> drives{"C://", "D://", "E://",
                                  "F://", "G://", "H://"};
  // first try to find hoi4 at the configured location
  if (exists(path) && path.find(game) != std::string::npos) {
    Logging::logLine("Located game under ", path);
    return true;
  } else {
    Fwg::Utils::Logging::logLine(
        "Could not find game under configured path ", path,
        " it doesn't exist or is malformed. Auto search will now "
        "try to locate the game, but may not succeed. It is "
        "recommended to correctly configure the path");
  }
  for (const auto &drive : drives) {
    if (exists(drive + "Program Files (x86)//Steam//steamapps//common//" +
               game)) {
      path = drive + "Program Files (x86)//Steam//steamapps//common//" + game;
      Logging::logLine("Located game under ", path);
      return true;
    } else if (exists(drive + "Program Files//Steam//steamapps//common//" +
                      game)) {
      path = drive + "Program Files//Steam//steamapps//common//" + game;
      Logging::logLine("Located game under ", path);
      return true;
    } else if (exists(drive + "Steam//steamapps//common//" + game)) {
      path = drive + "Steams//steamapps//common//" + game;
      Logging::logLine("Located game under ", path);
      return true;
    }
  }
  Logging::logLine("Could not find the game anywhere. Make sure the path to ",
                   game, " is configured correctly in the config files");
  return false;
}

bool GenericModule::validateGameModFolder(const std::string &game) {
  using namespace std::filesystem;
  namespace Logging = Fwg::Utils::Logging;
  // find the usermod directory of the relevant game
  if (exists(pathcfg.gameModsDirectory)) {
    Logging::logLine("Located mods directory folder under ",
                     pathcfg.gameModsDirectory);
    return true;
  } else {
    Logging::logLine(
        "Could not find the games mods directory folder under configured path ",
        pathcfg.gameModsDirectory,
        " it doesn't exist or is malformed. Please correct the path");
    return false;
  }
}
bool GenericModule::autoLocateGameModFolder(const std::string &game) {
  using namespace std::filesystem;
  namespace Logging = Fwg::Utils::Logging;
  Logging::logLine("Trying to auto locate mod directory path for ", game);
  std::string autoPath = getenv("USERPROFILE");
  autoPath += "//Documents//Paradox Interactive//" + game + "//mod//";

  if (exists(autoPath)) {
    pathcfg.gameModsDirectory = autoPath;
    sanitizePath(pathcfg.gameModsDirectory);
    Logging::logLine("Auto located game mod directory is ",
                     pathcfg.gameModsDirectory);
    pathcfg.gameModPath = autoPath + pathcfg.modName;
    sanitizePath(pathcfg.gameModPath);
    Logging::logLine("Auto located mod directory is ", pathcfg.gameModPath);
  }

  return false;
}

// try to find parent directory of where the mod should be
bool GenericModule::validateModFolder(const std::string &game) {
  using namespace std::filesystem;
  namespace Logging = Fwg::Utils::Logging;
  std::string tempPath = pathcfg.gameModPath;
  while (tempPath.back() == '//')
    tempPath.pop_back();
  while (tempPath.back() == '/')
    tempPath.pop_back();
  path modDestinationDir(tempPath);
  if (exists(modDestinationDir.parent_path())) {
    Logging::logLine("Located mod folder under ",
                     modDestinationDir.parent_path());
    return true;
  } else {
    Logging::logLine(
        "Could not find parent directory of mod directory under configured "
        "path ",
        modDestinationDir.parent_path(),
        " it doesn't exist or is malformed. Please correct the path");
    return false;
  }
}

void GenericModule::sanitizePath(std::string &path) {
  Fwg::Parsing::Scenario::replaceOccurences(path, "\\", "//");
  Fwg::Parsing::attachTrailing(path);
}

void GenericModule::generate() {}

// reads generic configs for every module
void GenericModule::configurePaths(
    const std::string &username, const std::string &gameName,
    const boost::property_tree::ptree &gamesConf) {
  // Short alias for this namespace
  // now read the paths
  pathcfg.modName = gamesConf.get<std::string>(gameName + ".modName");
  pathcfg.gamePath = gamesConf.get<std::string>(gameName + ".gamePath");
  sanitizePath(pathcfg.gamePath);
  pathcfg.gameModPath = gamesConf.get<std::string>(gameName + ".modPath");
  // already attach trailing before attaching the modname as the subfolder
  sanitizePath(pathcfg.gameModPath);
  pathcfg.gameModPath += pathcfg.modName;
  sanitizePath(pathcfg.gameModPath);
  Fwg::Parsing::Scenario::replaceOccurences(pathcfg.gameModPath, "<username>",
                                            username);
  Fwg::Parsing::Scenario::replaceOccurences(pathcfg.gamePath, "<username>",
                                            username);
  pathcfg.gameModsDirectory =
      gamesConf.get<std::string>(gameName + ".modsDirectory");
  sanitizePath(pathcfg.gameModsDirectory);
  Fwg::Parsing::Scenario::replaceOccurences(pathcfg.gameModsDirectory,
                                            "<username>", username);
}
} // namespace Scenario