#pragma once
#include "Factory.h"
namespace aftertaste {
std::vector<MachineSpec> energyMachineSpecs();
inline constexpr Position powerDistrictOrigin{-27,-2};
bool installPowerDistrictIn(FactoryState& state);
bool tidyDefaultPowerRoutesIn(FactoryState& state);
void initialisePowerDevice(Machine& m);
void clearPowerInventory(Machine& m);
bool validPowerState(const PowerState& state);
bool validPowerLink(const PowerLink& link);
double solarDaylight(int tick);double windStrength(int tick);
std::string powerContents(const Machine& machine);
}
