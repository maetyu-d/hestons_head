#pragma once
#include "MachineArt.h"

namespace aftertaste {
std::vector<Vector2> glassPath(Rectangle bounds,MachineKind kind,int inlet,int outlet);
Vector2 glassPoint(const std::vector<Vector2>& path,float fraction);
void drawGlassWall(const std::vector<Vector2>& path,float diameter,bool ghost=false,bool monochrome=false,bool chemical=false);
} // namespace aftertaste
