#pragma once
#include "MachineArt.h"
namespace aftertaste {
Color energyColor(EnergyMedium medium);
void drawEnergyHousing(Rectangle r,const MachineAppearance& look);
void drawEnergyMechanism(Rectangle r,const MachineAppearance& look);
}
