#include "generic/GameProvince.h"
namespace Scenario {
GameProvince::GameProvince(FastWorldGen::Province *province)
    : baseProvince(province), ID(province->ID), popFactor(0.0), devFactor(0.0),
      cityShare(0.0) {}

GameProvince::GameProvince()
    : baseProvince(nullptr), ID(-1), popFactor(0.0), devFactor(0.0),
      cityShare(0.0) {}

GameProvince::~GameProvince() {}
} // namespace Scenario