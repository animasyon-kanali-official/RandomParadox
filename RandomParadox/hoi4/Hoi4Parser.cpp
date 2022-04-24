#include "Hoi4Parser.h"

std::vector<std::string> Hoi4Parser::defaultTags;

void Hoi4Parser::dumpAdj(std::string path) {
	UtilLib::logLine("HOI4 Parser: Map: Writing Adjacencies");
	// From;To;Type;Through;start_x;start_y;stop_x;stop_y;adjacency_rule_name;Comment
	// empty file for now
	std::string content;
	content.append("From;To;Type;Through;start_x;start_y;stop_x;stop_y;adjacency_rule_name;Comment");
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpAirports(std::string path, const std::vector<Region>& regions)
{
	UtilLib::logLine("HOI4 Parser: Map: Building Airfields");
	std::string content;
	for (const auto& region : regions) {
		if (region.sea)
			continue;
		content.append(std::to_string(region.ID + 1));
		content.append("={");
		for (const auto& prov : region.provinces) {
			if (!prov->isLake) {
				content.append(std::to_string(prov->ID + 1));
				break;
			}
		}
		content.append(" }\n");
	}
	pU::writeFile(path, content);
}

std::string Hoi4Parser::getBuildingLine(const std::string type, const Region& region, const bool coastal, const Bitmap& heightmap)
{
	auto prov = *UtilLib::select_random(region.provinces);
	auto pix = 0;
	if (coastal) {
		while (!prov->coastal)
			prov = *UtilLib::select_random(region.provinces);
		pix = *UtilLib::select_random(prov->coastalPixels);
	}
	else {
		while (prov->isLake)
			prov = *UtilLib::select_random(region.provinces);
		pix = *UtilLib::select_random(prov->pixels);
	}
	auto widthPos = pix % Data::getInstance().width;
	auto heightPos = pix / Data::getInstance().width;
	std::vector<std::string> arguments{ std::to_string(region.ID + 1), type, std::to_string(widthPos),
										std::to_string((double)heightmap.getColourAtIndex(pix).getRed() / 10.0),
										std::to_string(heightPos), std::to_string((float)-1.57), "0" };
	return pU::csvFormat(arguments, ';', false);
}

// places building positions
void Hoi4Parser::dumpBuildings(std::string path, const std::vector<Region>& regions)
{
	UtilLib::logLine("HOI4 Parser: Map: Constructing Factories");
	const auto& heightmap = Bitmap::findBitmapByKey("heightmap");
	std::vector<std::string> buildingTypes{ "arms_factory", "industrial_complex", "air_base",
		"bunker", "coastal_bunker", "dockyard", "naval_base", "anti_air_building",
		"synthetic_refinery", "nuclear_reactor", "rocket_site", "radar_station", "fuel_silo", "floating_harbor" };
	std::string content;
	auto random = Data::getInstance().random2;
	// stateId; type; pixelX, rotation??, pixelY, rotation??, 0??}
	// 1; arms_factory; 2946.00; 11.63; 1364.00; 0.45; 0
	for (const auto& region : regions) {
		if (region.sea)
			continue;
		bool coastal = false;
		for (const auto& prov : region.provinces) {
			if (prov->coastal)
				coastal = true;
			// add supply node buildings for each province
			auto pix = *UtilLib::select_random(prov->pixels);
			auto widthPos = pix % Data::getInstance().width;
			auto heightPos = pix / Data::getInstance().width;
			content.append(getBuildingLine("supply_node", region, coastal, heightmap));

		}

		for (const auto& type : buildingTypes) {
			if (type == "arms_factory" || type == "industrial_complex")
				for (int i = 0; i < 6; i++)
					content.append(getBuildingLine(type, region, false, heightmap));
			else if (type == "bunker") {
				for (const auto& prov : region.provinces) {
					if (!prov->isLake && !prov->sea) {
						auto pix = *UtilLib::select_random(prov->pixels);
						auto widthPos = pix % Data::getInstance().width;
						auto heightPos = pix / Data::getInstance().width;
						std::vector<std::string> arguments{ std::to_string(region.ID + 1), type, std::to_string(widthPos), std::to_string((double)heightmap.getColourAtIndex(pix).getRed() / 10.0), std::to_string(heightPos), std::to_string(0.5), "0" };
						content.append(pU::csvFormat(arguments, ';', false));
					}
				}
			}
			else if (type == "anti_air_building")
				for (int i = 0; i < 3; i++)
					content.append(getBuildingLine(type, region, false, heightmap));
			else if (type == "coastal_bunker" || type == "naval_base") {
				for (const auto& prov : region.provinces) {
					if (prov->coastal) {
						auto pix = *UtilLib::select_random(prov->coastalPixels);
						int ID = 0;
						if (type == "naval_base")
							// find the ocean province this coastal building is next to
							for (const auto& neighbour : prov->neighbours)
								if (neighbour->sea)
									for (const auto& provPix : neighbour->pixels)
										if (UtilLib::getDistance(provPix, pix, Data::getInstance().width, 0) < 2.0)
											ID = neighbour->ID;
						auto widthPos = pix % Data::getInstance().width;
						auto heightPos = pix / Data::getInstance().width;
						std::vector<std::string> arguments{ std::to_string(region.ID + 1), type, std::to_string(widthPos),  std::to_string((double)heightmap.getColourAtIndex(pix).getRed() / 10.0), std::to_string(heightPos), std::to_string(0.5), std::to_string(ID + 1) };
						content.append(pU::csvFormat(arguments, ';', false));
					}
				}
			}
			else if (type == "dockyard" || type == "floating_harbor")
				content.append(getBuildingLine(type, region, coastal, heightmap));
			else
				content.append(getBuildingLine(type, region, false, heightmap));
		}
	}
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpContinents(std::string path, const std::vector<Continent>& continents)
{
	UtilLib::logLine("HOI4 Parser: Map: Writing Continents");
	std::string content{ "continents = {\n" };

	for (const auto& continent : continents) {
		content.append("\t");
		content.append(std::to_string(continent.ID + 1));
		content.append("\n");
	}
	content.append("}\n");
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpDefinition(std::string path, std::vector<GameProvince>& provinces)
{
	UtilLib::logLine("HOI4 Parser: Map: Defining Provinces");
	// province id; r value; g value; b value; province type (land/sea/lake); coastal (true/false); terrain (plains/hills/urban/etc. Defined for land or sea provinces in common/terrain); continent (int)
	// 0;0;0;0;land;false;unknown;0

	// terraintypes: ocean, lakes, forest, hills, mountain, plains, urban, jungle, marsh, desert, water_fjords, water_shallow_sea, water_deep_ocean
	// TO DO: properly map terrain types from climate
	//Bitmap typeMap(512, 512, 24);
	std::string content{ "0;0;0;0;land;false;unknown;0\n" };
	for (const auto& prov : provinces) {
		auto seaType = prov.baseProvince->sea ? "sea" : "land";
		auto coastal = prov.baseProvince->coastal ? "true" : "false";
		if (prov.baseProvince->sea) {
			for (auto prov2 : prov.baseProvince->neighbours) {
				if (!prov2->sea)
					coastal = "true";
			}
		}
		std::string terraintype;
		if (prov.baseProvince->sea)
			terraintype = "ocean";
		else
			terraintype = prov.terrainType;
		if (prov.baseProvince->isLake) {
			terraintype = "lakes";
			seaType = "lake";
		}
		std::vector<std::string> arguments{ std::to_string(prov.baseProvince->ID + 1),
			std::to_string(prov.baseProvince->colour.getRed()),
			std::to_string(prov.baseProvince->colour.getGreen()),
			std::to_string(prov.baseProvince->colour.getBlue()),
			seaType, coastal, terraintype,
			std::to_string(prov.baseProvince->sea || prov.baseProvince->isLake ? 0 : prov.baseProvince->continentID + 1) // 0 is for sea, no continent
		};
		content.append(pU::csvFormat(arguments, ';', false));
	}
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpRocketSites(std::string path, const std::vector<Region>& regions)
{
	UtilLib::logLine("HOI4 Parser: Map: Launching Rockets");
	std::string content;
	// regionId={provId }
	for (const auto& region : regions) {
		if (region.sea)
			continue;
		content.append(std::to_string(region.ID + 1));
		content.append("={");
		for (const auto& prov : region.provinces) {
			if (!prov->isLake) {
				content.append(std::to_string(prov->ID + 1));
				break;
			}
		}
		content.append(" }\n");
	}
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpUnitStacks(std::string path, const std::vector<Province*> provinces)
{
	UtilLib::logLine("HOI4 Parser: Map: Remilitarizing the Rhineland");
	// 1;0;3359.00;9.50;1166.00;0.00;0.08
	// provID, neighbour?, xPos, zPos yPos, rotation(3=north, 0=south, 1.5=east,4,5=west), ??
	// provID, xPos, ~10, yPos, ~0, 0,5
	// for each neighbour add move state in the direction of the neighbour. 0 might be stand still
	const auto& heightmap = Bitmap::findBitmapByKey("heightmap");
	std::string content{ "" };
	for (const auto& prov : provinces) {
		int position = 0;
		auto pix = *UtilLib::select_random(prov->pixels);
		auto widthPos = pix % Data::getInstance().width;
		auto heightPos = pix / Data::getInstance().width;
		std::vector<std::string> arguments{ std::to_string(prov->ID + 1), std::to_string(position), std::to_string(widthPos), std::to_string((double)heightmap.getColourAtIndex(pix).getRed() / 10.0), std::to_string(heightPos), std::to_string(0.0), "0.0" };
		content.append(pU::csvFormat(arguments, ';', false));
		for (const auto& neighbour : prov->neighbours) {
			position++;
			double angle;
			auto nextPos = prov->getPositionBetweenProvinces(*neighbour, Data::getInstance().width, angle);
			angle += 1.57;
			auto widthPos = nextPos % Data::getInstance().width;
			auto heightPos = nextPos / Data::getInstance().width;
			std::vector<std::string> arguments{ std::to_string(prov->ID + 1), std::to_string(position), std::to_string(widthPos), std::to_string((double)heightmap.getColourAtIndex(pix).getRed() / 10.0), std::to_string(heightPos), std::to_string(angle), "0.0" };
			content.append(pU::csvFormat(arguments, ';', false));
		}
	}
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpWeatherPositions(std::string path, const std::vector<Region>& regions, const std::vector<strategicRegion> strategicRegions)
{
	UtilLib::logLine("HOI4 Parser: Map: Creating Storms");
	// 1; 2781.24; 9.90; 1571.49; small
	std::string content{ "" };
	auto random = Data::getInstance().random2;
	// stateId; pixelX; rotation??; pixelY; rotation??; size
	// 1; arms_factory; 2946.00; 11.63; 1364.00; 0.45; 0

	for (auto i = 0; i < strategicRegions.size(); i++) {
		auto region = *UtilLib::select_random(strategicRegions[i].gameRegionIDs);
		auto prov = *UtilLib::select_random(regions[region].provinces);
		auto pix = *UtilLib::select_random(prov->pixels);
		auto widthPos = pix % Data::getInstance().width;
		auto heightPos = pix / Data::getInstance().width;
		std::vector<std::string> arguments{ std::to_string(i + 1), std::to_string(widthPos), std::to_string(9.90), std::to_string(heightPos), "small" };
		content.append(pU::csvFormat(arguments, ';', false));
	}
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpAdjacencyRules(std::string path)
{
	UtilLib::logLine("HOI4 Parser: Map: Writing Adjacency Rules");
	std::string content{ "" };
	// empty for now
	pU::writeFile(path, content);
}

void Hoi4Parser::dumpStrategicRegions(std::string path, const std::vector<Region>& regions, const std::vector<strategicRegion> strategicRegions)
{
	constexpr std::array<int, 12> daysInMonth{ 30, 27, 30, 29, 30, 29, 30, 30, 29, 30, 29, 30 };
	UtilLib::logLine("HOI4 Parser: Map: Drawing Strategic Regions");
	auto templateContent = pU::readFile("resources\\hoi4\\map\\strategic_region.txt");
	const auto templateWeather = pU::getBracketBlock(templateContent, "period");
	for (auto i = 0; i < strategicRegions.size(); i++) {
		std::string provString{ "" };
		for (const auto& region : strategicRegions[i].gameRegionIDs) {
			for (auto prov : regions[region].provinces) {
				provString.append(std::to_string(prov->ID + 1));
				provString.append(" ");
			}
		}
		auto content{ templateContent };
		pU::replaceOccurences(content, "templateID", std::to_string(i + 1));
		pU::replaceOccurences(content, "template_provinces", provString);

		// weather
		std::string weather{ "" };
		for (auto mo = 0; mo < 12; mo++) {
			auto month{ templateWeather };
			pU::replaceOccurences(month, "templateDateRange", "0." + std::to_string(mo) + " " + std::to_string(daysInMonth[mo]) + "." + std::to_string(mo));
			pU::replaceOccurences(month, "templateTemperatureRange", std::to_string(round((float)strategicRegions[i].weatherMonths[mo][3])).substr(0, 5)
				+ " " + std::to_string(round((float)strategicRegions[i].weatherMonths[mo][4])).substr(0, 5));
			pU::replaceOccurences(month, "templateRainLightChance", std::to_string((float)strategicRegions[i].weatherMonths[mo][5]));
			pU::replaceOccurences(month, "templateRainHeavyChance", std::to_string((float)strategicRegions[i].weatherMonths[mo][6]));
			pU::replaceOccurences(month, "templateMud", std::to_string((float)strategicRegions[i].weatherMonths[mo][7]));
			pU::replaceOccurences(month, "templateBlizzard", std::to_string((float)strategicRegions[i].weatherMonths[mo][8]));
			pU::replaceOccurences(month, "templateSandStorm", std::to_string((float)strategicRegions[i].weatherMonths[mo][9]));
			pU::replaceOccurences(month, "templateSnow", std::to_string((float)strategicRegions[i].weatherMonths[mo][10]));
			pU::replaceOccurences(month, "templateNoPhenomenon", std::to_string((float)strategicRegions[i].weatherMonths[mo][11]));
			//pU::replaceOccurences(month, "templateDateRange", "0." + std::to_string(i) + " 30." + std::to_string(i));
			//pU::replaceOccurences(month, "templateDateRange", "0." + std::to_string(i) + " 30." + std::to_string(i));
			weather.append(month + "\n\t\t");
		}
		pU::replaceOccurences(content, templateWeather, weather);
		pU::replaceOccurences(content, "template_provinces", provString);
		pU::writeFile(UtilLib::varsToString(path, "\\", (i + 1), ".txt"), content);
	}
}

void Hoi4Parser::dumpSupply(std::string path, const std::vector<std::vector<int>> supplyNodeConnections)
{
	std::string supplyNodes = "";
	std::string railways = "";
	std::set<int> nodes;
	for (const auto& connection : supplyNodeConnections) {
		if (connection.size() <= 1)
			continue;
		nodes.insert(connection[0]);
		nodes.insert(connection.back());
		railways += "1 ";
		railways += std::to_string(connection.size());
		railways += " ";
		for (auto prov : connection) {
			railways += std::to_string(prov + 1);
			railways += " ";
		}
		railways += "\n";
	}
	for (const auto& node : nodes) {
		supplyNodes.append("1 " + std::to_string(node + 1) + "\n");
	}
	ParserUtils::writeFile(path + "supply_nodes.txt", supplyNodes);
	ParserUtils::writeFile(path + "railways.txt", railways);
}

void Hoi4Parser::dumpStates(std::string path, std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: History: Drawing State Borders");
	auto templateContent = pU::readFile("resources\\hoi4\\history\\state.txt");
	std::vector<std::string> stateCategories{ "wasteland", "small_island", "pastoral", "rural", "town", "large_town", "city", "large_city", "metropolis", "megalopolis" };
	for (const auto& country : countries) {
		for (const auto& region : country.second.ownedRegions) {
			auto baseRegion{ region.baseRegion };
			if (baseRegion.sea)
				continue;
			sort(baseRegion.provinces.begin(), baseRegion.provinces.end());
			baseRegion.provinces.erase(unique(baseRegion.provinces.begin(), baseRegion.provinces.end()), baseRegion.provinces.end());
			std::string provString{ "" };
			for (const auto& prov : baseRegion.provinces) {
				provString.append(std::to_string(prov->ID + 1));
				provString.append(" ");
			}
			auto content{ templateContent };
			pU::replaceOccurences(content, "templateID", std::to_string(baseRegion.ID + 1));
			pU::replaceOccurences(content, "template_provinces", provString);
			pU::replaceOccurences(content, "templateOwner", country.first);
			pU::replaceOccurences(content, "templateInfrastructure", std::to_string(1 + (int)(region.attributeDoubles.at("development") * 4.0)));
			pU::replaceOccurences(content, "templateAirbase", std::to_string(0));
			pU::replaceOccurences(content, "templateCivilianFactory", std::to_string((int)region.attributeDoubles.at("civilianFactories")));
			pU::replaceOccurences(content, "templateArmsFactory", std::to_string((int)region.attributeDoubles.at("armsFactories")));
			pU::replaceOccurences(content, "templatePopulation", std::to_string((int)region.attributeDoubles.at("population")));
			pU::replaceOccurences(content, "templateStateCategory", stateCategories[(int)region.attributeDoubles.at("stateCategory")]);
			std::string navalBaseContent = "";
			for (const auto& gameProv : region.gameProvinces) {
				if (gameProv.attributeDoubles.at("naval_bases") > 0) {
					navalBaseContent += std::to_string(gameProv.ID + 1) + " = {\n\t\t\t\tnaval_base = " + std::to_string((int)gameProv.attributeDoubles.at("naval_bases")) + "\n\t\t\t}\n\t\t\t";
				}
			}
			pU::replaceOccurences(content, "templateNavalBases", navalBaseContent);
			if (region.attributeDoubles.at("dockyards") > 0)
				pU::replaceOccurences(content, "templateDockyards", std::to_string((int)region.attributeDoubles.at("dockyards")));
			else
				pU::replaceOccurences(content, "dockyard = templateDockyards", "");

			// resources
			for (const auto& resource : std::vector<std::string>{ "aluminium", "chromium", "oil", "rubber", "steel", "tungsten" }) {
				if (region.attributeDoubles.find(resource) != region.attributeDoubles.end())
					pU::replaceOccurences(content, "template" + resource, std::to_string((int)region.attributeDoubles.at(resource)));
			}
			pU::writeFile(path + "\\" + std::to_string(baseRegion.ID + 1) + ".txt", content);
		}
	}
}
void Hoi4Parser::dumpFlags(std::string path, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: Gfx: Printing Flags");
	for (const auto& country : countries) {
		TextureWriter::writeTGA(country.second.flag.width, country.second.flag.height, country.second.flag.getFlag(), path + country.first + ".tga");
		TextureWriter::writeTGA(country.second.flag.width / 2, country.second.flag.height / 2, country.second.flag.resize(country.second.flag.width / 2, country.second.flag.height / 2), path + "\\medium\\" + country.first + ".tga");
		TextureWriter::writeTGA(10, 7, country.second.flag.resize(10, 7), path + "\\small\\" + country.first + ".tga");
	}
}

void Hoi4Parser::writeHistoryCountries(std::string path, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: History: Writing Country History");
	const auto content = pU::readFile("resources\\hoi4\\history\\country_template.txt");
	for (const auto& country : countries) {
		auto tempPath = path + country.first + " - " + country.second.name + ".txt";
		auto countryText{ content };
		auto capitalID = 1;
		if (country.second.ownedRegions.size())
			capitalID = (*UtilLib::select_random(country.second.ownedRegions)).ID + 1;
		pU::replaceOccurences(countryText, "templateCapital", std::to_string(capitalID));
		pU::replaceOccurences(countryText, "templateTag", country.first);
		pU::replaceOccurences(countryText, "templateParty", country.second.attributeStrings.at("rulingParty"));
		std::string electAllowed = country.second.attributeDoubles.at("allowElections") ? "yes" : "no";
		pU::replaceOccurences(countryText, "templateAllowElections", electAllowed);
		pU::replaceOccurences(countryText, "templateDemPop", std::to_string(country.second.attributeDoubles.at("democratic")));
		pU::replaceOccurences(countryText, "templateFasPop", std::to_string(country.second.attributeDoubles.at("fascism")));
		pU::replaceOccurences(countryText, "templateComPop", std::to_string(country.second.attributeDoubles.at("communism")));
		pU::replaceOccurences(countryText, "templateNeuPop", std::to_string(country.second.attributeDoubles.at("neutrality")));
		pU::writeFile(tempPath, countryText);
	}
}

void Hoi4Parser::writeHistoryUnits(std::string path, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: History: Deploying the Troops");
	const auto defaultTemplate = pU::readFile("resources\\hoi4\\history\\default_unit_template.txt");
	const auto unitBlock = pU::readFile("resources\\hoi4\\history\\unit_block.txt");

	const auto unitTemplateFile = ParserUtils::readFile("resources\\hoi4\\history\\divisionTemplates.txt");
	// now tokenize by : character to get single
	const auto unitTemplates = ParserUtils::getTokens(unitTemplateFile, ':');
	//auto IDMapFile = pU::getLines("resources\\hoi4\\history\\divisionIDMapper.txt");
	std::map<int, std::string> IDMap;
	/*for (const auto& line : IDMapFile) {
		if (line.size()) {
			auto lineTokens = pU::getTokens(line, ';');
			IDMap[stoi(lineTokens[0])] = lineTokens[1];
		}
	}*/
	for (const auto& country : countries) {
		// get the template file
		std::string unitFile = defaultTemplate;
		std::string divisionTemplates = "";
		// now insert all the unit templates for this country
		for (const auto ID : country.second.attributeVectors.at("units")) {
			divisionTemplates.append(unitTemplates[ID]);
			// we need to buffer the names of the templates for use in later unit generationm
			auto requirements = ParserUtils::getBracketBlockContent(unitTemplates[ID], "requirements");
			auto value = ParserUtils::getBracketBlockContent(requirements, "templateName");
			IDMap[ID] = value;
			// remove requirements line
			ParserUtils::replaceLine(divisionTemplates, "requirements", "");
		}
		ParserUtils::replaceOccurences(unitFile, "templateTemplateBlock", divisionTemplates);

		// now that we have the templates written down, we deploy units of these templates under the
		// "divisions" key in the unitFile
		std::string totalUnits = "";
		// for every entry in unitCount vector
		for (int i = 0; i < country.second.attributeVectors.at("unitCount").size(); i++) {
			// run unit generation ("unitCount")[i] times
			for (int x = 0; x < country.second.attributeVectors.at("unitCount")[i]; x++) {

				UtilLib::logLine(i, ";", country.second.attributeVectors.at("unitCount")[i]);
				// copy the template unit file
				auto tempUnit{ unitBlock };
				// replace division name with the generic division name
				ParserUtils::replaceOccurences(tempUnit, "templateDivisionName", "\"" + IDMap.at(i) + "\"");
				// now deploy the unit in a random province
				ParserUtils::replaceOccurences(tempUnit, "templateLocation", std::to_string(country.second.ownedRegions[0].gameProvinces[0].ID + 1));
				totalUnits += tempUnit;
			}
		}

		//for (int i = 0; i < country.second.attributeVectors.at("units").size(); i++) {
// 
		//	for (int x = 0; x < country.second.attributeVectors.at("unitCount")[i]; x++) {
		//		UtilLib::logLine(country.second.attributeVectors.at("units")[i]);
		//		auto tempUnit{ unitBlock };
		//		ParserUtils::replaceOccurences(tempUnit, "templateDivisionName", IDMap.at(i));
		//		UtilLib::logLine(IDMap.at(i));
		//		ParserUtils::replaceOccurences(tempUnit, "templateLocation", std::to_string(country.second.ownedRegions[0].gameProvinces[0].ID + 1));
		//		totalUnits += tempUnit;
		//	}
		//}
		ParserUtils::replaceOccurences(unitFile, "templateUnitBlock", totalUnits);
		// units
		auto tempPath = path + country.first + "_1936.txt";
		pU::writeFile(tempPath, unitFile);

		// navies
		tempPath = path + country.first + "_1936_naval.txt";
		pU::writeFile(tempPath, "");
		tempPath = path + country.first + "_1936_naval_mtg.txt";
		pU::writeFile(tempPath, "");
	}
}

void Hoi4Parser::dumpCommonBookmarks(std::string path, const std::map<std::string, Country>& countries, std::map<int, std::vector<std::string>> strengthScores)
{
	auto bookmarkTemplate = pU::readFile("resources\\hoi4\\common\\bookmarks\\the_gathering_storm.txt");
	int count = 0;
	const auto majorTemplate = pU::getBracketBlock(bookmarkTemplate, "templateMajorTAG") + "\n\t\t";
	const auto minorTemplate = pU::getBracketBlock(bookmarkTemplate, "templateMinorTAG") + "\n\t\t";
	pU::removeBracketBlockFromKey(bookmarkTemplate, "templateMajorTAG");
	pU::removeBracketBlockFromBracket(bookmarkTemplate, "templateMinorTAG");
	std::string bookmarkCountries{ "" };
	for (auto iter = strengthScores.rbegin(); iter != strengthScores.rend(); ++iter) {
		if (count == 0) {
			pU::replaceOccurences(bookmarkTemplate, "templateDefaultTAG", iter->second[0]);
		}
		if (count < 7) {
			// major power:
			for (const auto& country : iter->second) {
				auto majorString{ majorTemplate };
				pU::replaceOccurences(majorString, "templateIdeology", countries.at(country).attributeStrings.at("rulingParty"));
				bookmarkCountries.append(pU::replaceOccurences(majorString, "templateMajorTAG", country));
				count++;
			}
		}
		else if (count < 14) {
			// regional power:
			for (const auto& country : iter->second) {
				auto minorString{ minorTemplate };
				pU::replaceOccurences(minorString, "templateIdeology", countries.at(country).attributeStrings.at("rulingParty"));
				bookmarkCountries.append(pU::replaceOccurences(minorString, "templateMinorTAG", country));
				count++;
			}
		}
	}
	pU::replaceOccurences(bookmarkTemplate, "templateMinorTAG=", bookmarkCountries);
	pU::writeFile(path + "the_gathering_storm.txt", bookmarkTemplate);
}

void Hoi4Parser::dumpCommonCountries(std::string path, std::string hoiPath, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: Common: Writing Countries");
	const auto content = pU::readFile("resources\\hoi4\\common\\country_default.txt");
	const auto colorsTxtTemplate = pU::readFile("resources\\hoi4\\common\\colors.txt");
	std::string colorsTxt = pU::readFile(hoiPath);
	for (const auto& country : countries) {
		auto tempPath = path + country.second.name + ".txt";
		auto countryText{ content };
		auto colourString = pU::replaceOccurences(UtilLib::varsToString(country.second.colour), ";", " ");
		pU::replaceOccurences(countryText, "templateCulture", country.second.attributeStrings.at("gfxCulture"));
		pU::replaceOccurences(countryText, "templateColour", colourString);
		pU::writeFile(tempPath, countryText);
		auto templateCopy{ colorsTxtTemplate };
		pU::replaceOccurences(templateCopy, "templateTag", country.first);
		pU::replaceOccurences(templateCopy, "templateColour", colourString);
		colorsTxt.append(templateCopy);
	}
	pU::writeFile(path + "colors.txt", colorsTxt);
}

void Hoi4Parser::dumpCommonCountryTags(std::string path, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: Common: Writing Country Tags");
	std::string content = "";
	for (const auto& country : countries)
		content.append(country.first + " = countries/" + country.second.name + ".txt\n");
	pU::writeFile(path, content);
}

void Hoi4Parser::writeCountryNames(std::string path, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: Localisation: Writing Country Names");
	NameGenerator nG;
	std::string content = "l_english:\n";
	std::vector<std::string> ideologies{ "fascism", "communism", "neutrality", "democratic" };

	for (const auto& c : countries) {
		for (const auto& ideology : ideologies) {
			auto ideologyName = nG.modifyWithIdeology(ideology, c.second.name, c.second.adjective);
			content += " " + c.first + "_" + ideology + ":0 \"" + ideologyName + "\"\n";
			content += " " + c.first + "_" + ideology + "_DEF:0 \"" + ideologyName + "\"\n";;
			content += " " + c.first + "_" + ideology + "_ADJ:0 \"" + c.second.adjective + "\"\n";;
		}
	}
	pU::writeFile(path + "countries_l_english.yml", content, true);
}

void Hoi4Parser::writeStateNames(std::string path, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: Localisation: Writing State Names");
	std::string content = "l_english:\n";

	for (const auto& c : countries) {
		for (const auto& region : c.second.ownedRegions)
			content += " STATE_" + std::to_string(region.ID + 1) + ":0 \"" + region.name + "\"\n";
	}
	pU::writeFile(path + "state_names_l_english.yml", content, true);
}

void Hoi4Parser::writeStrategicRegionNames(std::string path, const std::vector<strategicRegion> strategicRegions)
{
	UtilLib::logLine("HOI4 Parser: Map: Naming the Regions");
	std::string content = "l_english:\n";
	for (auto i = 0; i < strategicRegions.size(); i++) {
		content += UtilLib::varsToString(" STRATEGICREGION_", i, ":0 \"", strategicRegions[i].name, "\"\n");
	}
	pU::writeFile(path + "\\strategic_region_names_l_english.yml", content, true);
}

std::vector<std::string> Hoi4Parser::readTypeMap()
{
	return ParserUtils::getLines("resources\\hoi4\\ai\\national_focus\\baseFiles\\foci.txt");
}

std::map<std::string, std::string> Hoi4Parser::readRewardMap(std::string path)
{
	auto file = ParserUtils::readFile(path);
	auto split = ParserUtils::getTokens(file, ';');
	std::map<std::string, std::string> rewardMap;
	for (const auto& elem : split) {
		auto key = ParserUtils::getBracketBlockContent(elem, "key");
		auto value = ParserUtils::getBracketBlockContent(elem, "value");
		rewardMap[key] = value;
	}
	return { rewardMap };
}

void Hoi4Parser::writeFoci(std::string path, const std::map<std::string, Country>& countries)
{
	UtilLib::logLine("HOI4 Parser: History: Demanding Danzig");
	auto focusTypes = ParserUtils::getLines("resources\\hoi4\\ai\\national_focus\\baseFiles\\foci.txt");
	std::string baseTree = ParserUtils::readFile("resources\\hoi4\\ai\\national_focus\\baseFiles\\focusBase.txt");
	std::vector<std::string> focusTemplates;
	for (auto& focusType : focusTypes)
		focusTemplates.push_back(ParserUtils::readFile("resources\\hoi4\\ai\\national_focus\\focusTypes\\" + focusType + "Focus.txt"));

	for (const auto& c : countries) {
		std::string treeContent = baseTree;
		std::string tempContent = "";
		for (const auto& focusChain : c.second.foci) {
			for (const auto& countryFocus : focusChain) {
				tempContent += focusTemplates[countryFocus.fType];


				// replace completion reward field with rewards in chain
				std::string available = "";
				for (const auto& availKey : countryFocus.available) {
					available += NationalFocus::availableMap.at(availKey);
				}
				std::string bypasses = "";
				for (const auto& bypassKey : countryFocus.bypasses) {
					bypasses += NationalFocus::bypassMap.at(bypassKey);
				}
				std::string completionReward = "";
				for (const auto& rewardKey : countryFocus.completionRewards) {
					completionReward += NationalFocus::rewardMap.at(rewardKey);
				}
				ParserUtils::replaceOccurences(tempContent, "templateAvailable", completionReward);
				ParserUtils::replaceOccurences(tempContent, "templateBypasses", completionReward);
				ParserUtils::replaceOccurences(tempContent, "templateCompletionRewards", completionReward);

				ParserUtils::replaceOccurences(tempContent, "templateStepID", std::to_string(countryFocus.stepID));
				ParserUtils::replaceOccurences(tempContent, "templateChainID", std::to_string(countryFocus.chainID));
				ParserUtils::replaceOccurences(tempContent, "templateSourceTag", c.first);
				ParserUtils::replaceOccurences(tempContent, "templateDestTag", countryFocus.destTag);
				ParserUtils::replaceOccurences(tempContent, "templateXPosition", std::to_string(countryFocus.position[0]));
				ParserUtils::replaceOccurences(tempContent, "templateYPosition", std::to_string(countryFocus.position[1]));
				// now collect all prerequisites
				std::string preString = "";
				//for (const auto& prerequisite : countryFocus.precedingFoci) {
				//	// derive the name of the preceding focus
				//	std::string preName = UtilLib::varsToString(c.first, focusChain[prerequisite].chainID, ".", focusChain[prerequisite].stepID);
				//	preString += "prerequisite = { focus = " + preName + "}\n\t\t";
				//}
				preString += "prerequisite = {";
				for (const auto& prerequisite : countryFocus.precedingFoci) {
					for (const auto& foc : focusChain) {
						if (foc.stepID == prerequisite) {
							// derive the name of the preceding focus
							std::string preName = UtilLib::varsToString(c.first, focusChain[0].chainID, ".", prerequisite);
							preString += " focus = " + preName;
						}
					}
				}
				preString += " }\n\t\t";
				ParserUtils::replaceOccurences(tempContent, "templatePrerequisite", preString);
				// now make exclusive
				preString.clear();
				for (const auto& exclusive : countryFocus.alternativeFoci) {
					for (const auto& foc : focusChain) {
						if (foc.stepID == exclusive) {
							// derive the name of the preceding focus
							std::string preName = UtilLib::varsToString(c.first, focusChain[0].chainID, ".", exclusive);
							preString += " focus = " + preName;
						}
					}
				}
				ParserUtils::replaceOccurences(tempContent, "templateExclusive", preString);
			}
		}
		ParserUtils::replaceOccurences(treeContent, "templateFocusTree", tempContent);
		ParserUtils::replaceOccurences(treeContent, "templateSourceTag", c.first);
		ParserUtils::writeFile(path + c.second.name + ".txt", treeContent);
	}
}

void Hoi4Parser::writeCompatibilityHistory(std::string path, std::string hoiPath, const std::vector<Region>& regions)
{
	const std::filesystem::path hoiDir{ hoiPath + "\\history\\countries\\" };
	const std::filesystem::path modDir{ path };
	auto random = Data::getInstance().random2;
	for (auto const& dir_entry : std::filesystem::directory_iterator{ hoiDir }) {
		std::string pathString = dir_entry.path().string();

		std::string filename = pathString.substr(pathString.find_last_of("\\") + 1, pathString.back() - pathString.find_last_of("\\"));
		auto content = pU::readFile(pathString);
		while (content.find("start_resistance = yes") != std::string::npos) {
			pU::removeSurroundingBracketBlockFromLineBreak(content, "start_resistance = yes");
		}
		pU::replaceLine(content, "capital =", "capital = " + std::to_string(1));
		pU::writeFile(path + filename, content);
	}
}

void Hoi4Parser::copyDescriptorFile(const std::string sourcePath, const std::string destPath, const std::string modsDirectory, const std::string modName)
{
	auto descriptorText = pU::readFile(sourcePath);
	pU::replaceOccurences(descriptorText, "templateName", modName);
	auto modText{ descriptorText };
	pU::replaceOccurences(descriptorText, "templatePath", "");
	pU::writeFile(destPath + "//descriptor.mod", descriptorText);
	pU::replaceOccurences(modText, "templatePath", UtilLib::varsToString("path=\"", destPath, "\""));
	pU::writeFile(modsDirectory + "//" + modName + ".mod", modText);
}
