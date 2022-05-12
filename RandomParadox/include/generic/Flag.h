#pragma once
#include "FastWorldGenerator.h"
#include "ParserUtils.h"
#include "TextureWriter.h"

class Flag {
  // keep code shorter
  using PU = ParserUtils;
  // static cause we only want to read them from file once
  static std::map<std::string, std::vector<Colour>> colourGroups;
  static std::vector<std::vector<std::vector<int>>> flagTypes;
  static std::vector<std::vector<std::vector<std::string>>> flagTypeColours;
  static std::vector<std::vector<uint8_t>> flagTemplates;
  static std::vector<std::vector<std::string>> flagMetadata;
  static std::vector<std::vector<uint8_t>> symbolTemplates;
  static std::vector<std::vector<std::string>> symbolMetadata;
  // containers
  std::vector<Colour> colours;
  std::vector<unsigned char> image;

public:
  // vars
  int width;
  int height;
  // constructors/destructors
  Flag();
  Flag(const int width, const int height);
  ~Flag();
  // methods - image read/write
  void setPixel(const Colour colour, const int x, const int y);
  void setPixel(const Colour colour, const int index);
  std::vector<unsigned char> getFlag() const;
  // methods - utils
  std::vector<unsigned char> resize(const int width, const int height) const;
  static std::vector<unsigned char>
  resize(const int width, const int height,
         const std::vector<unsigned char> tImage, const int inWidth,
         const int inHeight);
  // methods - read in configs
  static void readColourGroups();
  static void readFlagTypes();
  static void readFlagTemplates();
  static void readSymbolTemplates();
};
