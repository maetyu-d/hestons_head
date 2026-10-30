#include "Power.h"
#include <algorithm>
namespace aftertaste {
Blueprint Factory::powerDistrictBlueprint(){
 using K=MachineKind;using E=EnergyMedium;Blueprint b;b.name="Western power district / hybrid generation";b.width=18;b.height=17;
 auto add=[&](Position p,K kind,int direction=0)->Machine&{Machine m;m.kind=kind;m.dir=direction;m.duration=std::max(1,machineSpec(kind).duration);if(isEnergyMachine(kind))initialisePowerDevice(m);b.parts.push_back({p,m});return b.parts.back().machine;};
 add({1,0},K::SolarPanel);add({3,0},K::SolarPanel);add({5,0},K::SolarPanel);
 add({7,0},K::BatteryRack);add({9,0},K::Inverter);add({13,1},K::PowerRelay);auto& external=add({15,0},K::GridInlet);external.enabled=false;
 add({1,3},K::WindRotor);add({3,3},K::Gearbox).shear=1.8;add({5,3},K::Alternator);add({13,2},K::Switchboard);
 auto& solid=add({0,7},K::Feeder);solid.ingredient="cocoa";solid.dose=90;solid.automatic=true;add({1,7},K::Macerator);add({2,7},K::BiomassBoiler);
 add({4,7},K::SteamAccumulator);add({6,7},K::SteamTurbine);add({8,7},K::Alternator);add({8,5},K::Flywheel);
 add({6,10},K::PowerCondenser);add({2,10},K::FeedPump);add({10,10},K::HeatExchanger);auto& tank=add({12,10},K::HeatTank);tank.forward=true;add({14,10},K::CoolingPlant);
 auto& scraps=add({0,13},K::Feeder);scraps.ingredient="greens";scraps.dose=100;scraps.automatic=true;add({1,13},K::Macerator);add({2,13},K::AnaerobicDigester);add({4,13},K::GasScrubber);add({6,13},K::GasHolder);add({8,13},K::GasEngine);add({10,13},K::Alternator);
 add({2,8},K::CompostBin);auto& soil=add({3,8},K::RaisedBed);soil.ingredient="basil";soil.temperature=20;
 add({2,14},K::CompostBin);auto& bed=add({3,14},K::RaisedBed);bed.ingredient="greens";bed.temperature=20;
 const std::array<Position,EnergyCount> hubs={Position{10,2},{8,1},{7,4},{5,8},{7,11},{3,12},{7,12},{3,11},{11,11},{15,11}};
 std::map<std::pair<Position,E>,PowerLink> links;
 auto route=[&](Position from,Position to,E medium){Position p=from;for(int n=0;n<150;n++){PowerLink l;l.medium=medium;l.rating=medium==E::AC?100:60;links[{p,medium}]=l;if(p==to)break;if(p.x!=to.x)p.x+=p.x<to.x?1:-1;else p.y+=p.y<to.y?1:-1;}};
 for(const auto& part:b.parts){if(part.machine.kind==K::Switchboard){route(nextPosition(part.at,2),hubs[size_t(E::AC)],E::AC);route(nextPosition(part.at,0),{14,2},E::AC);continue;}for(auto port:energyPorts(part.machine))if(port.medium!=E::Shaft)route(nextPosition(part.at,port.direction),hubs[size_t(port.medium)],port.medium);if(machineSpec(part.machine.kind).power>0)route(part.at,hubs[size_t(E::AC)],E::AC);}
 route({2,3},{2,3},E::Shaft);route({4,3},{4,3},E::Shaft);
 route({7,7},{7,5},E::Shaft);route({7,5},{7,5},E::Shaft);route({9,5},{9,6},E::Shaft);route({9,13},{9,13},E::Shaft);
 for(auto& [key,link]:links)b.links.push_back({key.first,link});return b;
}
bool installPowerDistrictIn(FactoryState& state){
 using K=MachineKind;using E=EnergyMedium;
 if(std::any_of(state.machines.begin(),state.machines.end(),[](const auto& e){return isEnergyMachine(e.second.kind);}))return false;
 auto plan=Factory::powerDistrictBlueprint();Position origin{-27,-1};
 for(const auto& part:plan.parts)if(state.machines.count({origin.x+part.at.x,origin.y+part.at.y}))return false;
 for(const auto& part:plan.parts)state.machines.emplace(Position{origin.x+part.at.x,origin.y+part.at.y},part.machine);
 auto link=[&](Position p,E medium,double rating=100){auto& v=state.energyLinks[p];if(std::none_of(v.begin(),v.end(),[&](const PowerLink& l){return l.medium==medium;})){PowerLink l;l.medium=medium;l.rating=rating;v.push_back(l);}};
 for(const auto& part:plan.links)link({origin.x+part.at.x,origin.y+part.at.y},part.link.medium,part.link.rating);
 Position root{origin.x+14,origin.y+2};
 auto route=[&](Position from,Position to,E medium){Position p=from;for(int n=0;n<3000;n++){link(p,medium);if(p==to)break;if(p.x!=to.x)p.x+=p.x<to.x?1:-1;else p.y+=p.y<to.y?1:-1;}};
 for(const auto& [p,m]:state.machines){if(isEnergyMachine(m.kind)||machineSpec(m.kind).power<=0||p.x<origin.x+18)continue;route(root,p,E::AC);if(m.kind==K::Heater||m.kind==K::SousVide||m.kind==K::FractionatingStill||m.kind==K::GlassFlask)route({origin.x+11,origin.y+11},p,E::Heat);if(m.kind==K::Chiller||m.kind==K::ColdStore||m.kind==K::GlassCondenser)route({origin.x+15,origin.y+11},p,E::Cooling);}
 auto& battery=state.machines.at({origin.x+7,origin.y});double reserve=std::min(250.,state.electricity);state.electricity-=reserve;battery.power.store[size_t(E::DC)]=reserve;state.grid.active=true;state.grid.stored=reserve;
 if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[&](const Blueprint& p){return p.name==plan.name;}))state.blueprints.push_back(std::move(plan));return true;
}
bool Factory::installPowerDistrict(){snapshot();bool changed=installPowerDistrictIn(*this);if(!changed){history.pop_back();notice="Power district already present or its western site is occupied. Use Plans to build another installation.";return false;}dirty=true;notice="Western power district installed. Actual generation now supplies connected factory and farm circuits.";return true;}
}
