#pragma once
#include "Factory.h"
namespace aftertaste {
std::vector<MachineSpec> energyMachineSpecs();
bool installPowerDistrictIn(FactoryState& state);
void initialisePowerDevice(Machine& m);
void clearPowerInventory(Machine& m);
bool validPowerState(const PowerState& state);
bool validPowerLink(const PowerLink& link);
double solarDaylight(int tick);double windStrength(int tick);
std::string powerContents(const Machine& machine);
}
