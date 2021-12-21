#pragma once
#include "../FastWorldGen/FastWorldGen/FastWorldGenerator.h"
#include "../FormatConverter.h"
#include "Hoi4Parser.h"
#include <filesystem>
#include <experimental/filesystem>
#include "Hoi4ScenarioGenerator.h"
class Hoi4Module
{
	
public:
	Hoi4Module();
	~Hoi4Module();
	void createPaths(std::string hoi4ModPath);
	void genHoi(std::string hoi4ModPath, std::string hoi4Path, bool useDefaultMap, bool useDefaultStates, bool useDefaultProvinces, ScenarioGenerator& scenGen);
};

