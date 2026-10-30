#pragma once
#include "Factory.h"
#include "raylib.h"

namespace aftertaste {
struct MachineAppearance {
 MachineKind kind=MachineKind::Belt;
 int direction=0;
 bool active=false,ghost=false,monochrome=false;
 double time=0;
 bool closed=false;
 UtilityKind utility=UtilityKind::Power;
 double progress=0;double labFlux=0,labVacuum=0,labStability=0,labCooling=0;bool labArmed=false;
 bool cropPlanted=false;
 bool forwarding=false;std::string crop="tomato";double cropHealth=1,cropMoisture=.62;
};
void drawCropProduce(Rectangle bounds,const std::string& crop,double dirt=0,double freshness=1);
// Scalable, text-free equipment drawings. Food is drawn between these two
// layers, through a window that rotates with the actual outlet direction.
void drawMachineHousing(Rectangle bounds,const MachineAppearance& appearance);
void drawMachineMechanism(Rectangle bounds,const MachineAppearance& appearance);
Rectangle machineFoodWindow(Rectangle bounds,MachineKind kind,int direction,int slot=0,bool paired=false);
int verifyMachineArtwork(); // Native visual QA; requires an initialized drawing context.
} // namespace aftertaste
