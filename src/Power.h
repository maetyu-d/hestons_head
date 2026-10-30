#pragma once
#include "Factory.h"
namespace aftertaste {
std::vector<MachineSpec> energyMachineSpecs();
inline constexpr Position powerDistrictOrigin{-27,-2};
bool installPowerDistrictIn(FactoryState& state);
bool tidyDefaultPowerRoutesIn(FactoryState& state);
bool dockDefaultPowerDevicesIn(FactoryState& state);
bool powerContactSupports(MachineKind first,MachineKind second,EnergyMedium medium);
inline constexpr Position nuclearLabOrigin{-35,-2};
// The old origins remain fixed for save migrations. New installations share
// this tightly packed campus beside the factory.
inline constexpr Position powerCampusOrigin{-23,-4};
inline constexpr Position powerCampusLabOrigin{-17,-3};
bool compactDefaultPowerCampusIn(FactoryState& state);
bool compactLegacyNuclearLabIn(FactoryState& state);
bool isContactService(EnergyMedium medium);
bool installNuclearLabIn(FactoryState& state);
bool isNuclearEquipment(MachineKind kind);
double labTuningTarget(MachineKind kind);
void initialisePowerDevice(Machine& m);
void clearPowerInventory(Machine& m);
bool validPowerState(const PowerState& state);
bool validPowerLink(const PowerLink& link);
double solarDaylight(int tick);double windStrength(int tick);
std::string powerContents(const Machine& machine);
}
