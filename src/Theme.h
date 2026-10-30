#pragma once
#include "Factory.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
namespace aftertaste::theme {
inline constexpr Color Ink{22,41,75,255},Muted{77,99,131,255},Paper{255,255,255,255},Line{196,205,217,255};
inline constexpr Color Red{240,43,43,255},Blue{26,99,235,255},Yellow{255,211,22,255},Green{13,183,76,255},Violet{146,53,226,255},Cyan{0,174,238,255},Pink{239,52,143,255};
inline constexpr Color Floor{255,255,255,255},Grid{232,235,239,255},FactoryFloor{255,255,255,255};
inline Color blend(Color a,Color b,float t){return {static_cast<unsigned char>(a.r+(b.r-a.r)*t),static_cast<unsigned char>(a.g+(b.g-a.g)*t),static_cast<unsigned char>(a.b+(b.b-a.b)*t),255};}
// A small, high-chroma colour vocabulary is shared by chips, material and art.
inline Color primary(Color c){
 auto hsv=ColorToHSV(c);if(hsv.y<.14f)return blend(c,WHITE,.22f);
 float h=hsv.x;Color paint=h<25||h>=345?Red:h<75?Yellow:h<160?Green:h<205?Cyan:h<255?Blue:h<300?Violet:Pink;
 return hsv.z<.50f?blend(paint,Ink,.35f):paint;
}
inline Color on(Color c){return (.2126f*c.r+.7152f*c.g+.0722f*c.b)/255.f<.52f?WHITE:Ink;}
inline Color pigment(const std::string& value){if(value.size()!=7)return Ink;unsigned n=unsigned(std::stoul(value.substr(1),nullptr,16));return primary({static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>8),static_cast<unsigned char>(n),255});}
inline Color machinePaint(MachineKind k){using K=MachineKind;
 switch(k){
  case K::Heater:case K::SousVide:case K::Smoker:case K::ReagentEvaporator:case K::SaltReactor:case K::BiomassBoiler:case K::StarChamber:case K::HeatTank:case K::HeatExchanger:case K::ConcentrateTank:return Red;
  case K::Dryer:case K::Extruder:case K::Burette:case K::MineralHopper:case K::ReagentExtractor:case K::SandFilter:case K::CuriosityPile:case K::Calorimeter:case K::Alternator:case K::Inverter:case K::Switchboard:case K::BatteryRack:case K::PowerRelay:case K::GridInlet:case K::Grader:case K::SeedHopper:case K::GrowLight:case K::PollinatorHive:return Yellow;
  case K::Feeder:case K::Doser:case K::Spherifier:case K::Gel:case K::Fermenter:case K::EnzymeReactor:case K::CultureVat:case K::ReagentIntake:case K::FieldBed:case K::RaisedBed:case K::HydroponicBed:case K::Harvester:case K::ProduceIntake:case K::CompostBin:case K::ReedReclaimer:case K::AnaerobicDigester:case K::GasScrubber:return Green;
  case K::Belt:case K::Mixer:case K::Homogenizer:case K::Centrifuge:case K::Chiller:case K::ColdStore:case K::FreezeDryer:case K::Dispatch:case K::LatticeReactor:case K::SolarPanel:case K::WindRotor:case K::Gearbox:case K::SteamTurbine:case K::ExperimentConsole:case K::QuenchBath:case K::CoolingPlant:case K::BoreholePump:case K::RainCollector:case K::RawWaterTank:case K::MembraneFilter:case K::CleanWaterTank:case K::WaterOutlet:case K::WaterAnalyzer:case K::ReagentSeparator:case K::ReagentAnalyzer:case K::ReagentTank:return Blue;
  case K::ReagentPolisher:case K::ReagentRecovery:case K::UVSterilizer:case K::VacuumChamber:return Violet;
  default:return pigment(machineSpec(k).color);
 }
}
}
