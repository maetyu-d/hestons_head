#pragma once
#include "PowerArt.h"
namespace aftertaste {
enum class LineFamily {Electricity,Water,Service};
LineFamily lineFamily(EnergyMedium medium);
float infrastructureWidth(EnergyMedium medium,float tile);
void drawInfrastructureRoute(const std::vector<Vector2>& path,EnergyMedium medium,float tile,bool enabled=true,bool tripped=false,bool focused=false);
void drawInfrastructureFitting(Vector2 at,Vector2 tangent,EnergyMedium medium,float tile,bool enabled=true);
void drawInfrastructureJunction(Vector2 at,EnergyMedium medium,float tile);
void drawInfrastructureFlow(const std::vector<Vector2>& path,EnergyMedium medium,float tile,float phase,bool reverse=false);
}
