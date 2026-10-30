#include "Power.h"
#include <algorithm>
namespace aftertaste {
static Blueprint legacyPowerDistrictBlueprint(){
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
// Keep the generator layout stable; route each service deliberately instead of
// joining every terminal to a hub. One extra top row accommodates the import cable.
Blueprint Factory::powerDistrictBlueprint(){
 using E=EnergyMedium;auto b=legacyPowerDistrictBlueprint();b.links.clear();b.height=18;
 std::map<std::pair<Position,E>,PowerLink> links;
 auto node=[&](Position p,E medium)->PowerLink&{auto [i,inserted]=links.emplace(std::make_pair(p,medium),PowerLink{});if(inserted){i->second.medium=medium;i->second.rating=medium==E::AC?100:60;i->second.ports=0;}return i->second;};
 auto run=[&](E medium,std::initializer_list<Position> points){auto i=points.begin();Position p=*i++;node(p,medium);for(;i!=points.end();++i){Position end=*i;while(p!=end){int d=p.x!=end.x?(p.x<end.x?0:2):(p.y<end.y?1:3);Position q=nextPosition(p,d);node(p,medium).ports|=1<<d;node(q,medium).ports|=1<<((d+2)%4);p=q;}}};
 // Solar collection is a parallel bus below the panels, with a battery branch.
 run(E::DC,{{2,1},{8,1}});for(int x:{2,4,6,8})run(E::DC,{{x,0},{x,1}});
 // Generator collector and auxiliaries. Only the switchboard feeds the factory.
 run(E::AC,{{11,-1},{11,15}});run(E::AC,{{16,0},{16,-1},{11,-1}});
 run(E::AC,{{10,0},{11,0}});run(E::AC,{{12,2},{11,2}});run(E::AC,{{14,2},{17,2}});
 run(E::AC,{{6,3},{11,3}});run(E::AC,{{9,7},{11,7}});
 run(E::AC,{{2,6},{2,5},{11,5}});run(E::AC,{{2,9},{13,9},{13,10}});
 run(E::AC,{{8,12},{11,12}});run(E::AC,{{2,15},{11,15}});
 // Short mechanical trains and direct steam/gas stages.
 run(E::Shaft,{{2,3},{2,3}});run(E::Shaft,{{4,3},{4,3}});
 run(E::Shaft,{{7,5},{7,7}});run(E::Shaft,{{9,13},{9,13}});
 run(E::Steam,{{3,7},{3,7}});run(E::Steam,{{5,7},{5,7}});
 run(E::Exhaust,{{6,8},{6,9},{5,9},{5,10}});
 run(E::RawGas,{{3,13},{3,13}});run(E::Gas,{{5,13},{5,13}});run(E::Gas,{{7,13},{7,13}});
 // A small return-water loop serves the boiler and scrubber.
 run(E::Water,{{3,9},{7,9},{7,10}});run(E::Water,{{3,9},{3,10}});
 run(E::Water,{{2,6},{3,6},{3,9}});run(E::Water,{{4,9},{4,12}});
 // Recovered heat shares one header; the cooling export has its own route.
 run(E::Heat,{{1,11},{17,11}});run(E::Heat,{{2,8},{1,8},{1,11}});
 run(E::Heat,{{6,11},{6,11}});run(E::Heat,{{9,10},{9,11}});
 run(E::Heat,{{11,10},{11,11}});run(E::Heat,{{13,11},{13,9},{14,9}});
 run(E::Heat,{{8,14},{9,14},{9,11}});run(E::Heat,{{2,12},{2,11}});
 run(E::Cooling,{{15,10},{17,10}});
 for(const auto& part:b.parts)for(auto port:energyPorts(part.machine)){auto i=links.find({nextPosition(part.at,port.direction),port.medium});if(i!=links.end())i->second.ports|=1<<((port.direction+2)%4);}
 node({2,9},E::AC).ports|=8;node({2,15},E::AC).ports|=8;
 node({17,2},E::AC).ports|=1;node({17,10},E::Cooling).ports|=1;node({17,11},E::Heat).ports|=1;
 for(auto& part:b.parts)part.at.y++;
 for(auto& [key,link]:links)b.links.push_back({{key.first.x,key.first.y+1},link});
 return b;
}
namespace {
using Routes=std::map<Position,std::vector<PowerLink>>;
Routes demoRoutes(const FactoryState& state,bool legacy){
 using K=MachineKind;using E=EnergyMedium;Routes routes;auto b=legacy?legacyPowerDistrictBlueprint():Factory::powerDistrictBlueprint();Position origin=legacy?Position{-27,-1}:powerDistrictOrigin;
 auto add=[&](Position p,E e,double rating=100,int ports=0)->PowerLink&{auto& v=routes[p];auto i=std::find_if(v.begin(),v.end(),[&](const auto& l){return l.medium==e;});if(i==v.end()){PowerLink l;l.medium=e;l.rating=rating;l.ports=ports;v.push_back(l);return v.back();}i->ports|=ports;return *i;};
 auto run=[&](Position p,Position end,E e){add(p,e,100,legacy?15:0);for(int n=0;n<3000&&p!=end;n++){int d=p.x!=end.x?(p.x<end.x?0:2):(p.y<end.y?1:3);Position q=nextPosition(p,d);add(p,e,100,legacy?15:0).ports|=1<<d;add(q,e,100,legacy?15:0).ports|=1<<((d+2)%4);p=q;}};
 for(const auto& node:b.links)add({origin.x+node.at.x,origin.y+node.at.y},node.link.medium,node.link.rating,node.link.ports);
 if(legacy){for(const auto& [p,m]:state.machines){if(isEnergyMachine(m.kind)||machineSpec(m.kind).power<=0||p.x<-9)continue;run({-13,1},p,E::AC);if(m.kind==K::Heater||m.kind==K::SousVide||m.kind==K::FractionatingStill||m.kind==K::GlassFlask)run({-16,10},p,E::Heat);if(m.kind==K::Chiller||m.kind==K::ColdStore||m.kind==K::GlassCondenser)run({-12,10},p,E::Cooling);}return routes;}
 // A service corridor west of the food floor replaces individual vertical runs.
 std::array<std::map<int,int>,3> rows;
 for(const auto& [p,m]:state.machines){if(isEnergyMachine(m.kind)||machineSpec(m.kind).power<=0||p.x<-9)continue;Position sink=p;auto row=[&](int service){auto [it,inserted]=rows[size_t(service)].emplace(sink.y,sink.x);if(!inserted)it->second=std::max(it->second,sink.x);};row(0);if(m.kind==K::Heater||m.kind==K::SousVide||m.kind==K::FractionatingStill||m.kind==K::GlassFlask)row(1);if(m.kind==K::Chiller||m.kind==K::ColdStore||m.kind==K::GlassCondenser)row(2);}
 const std::array<E,3> media={E::AC,E::Heat,E::Cooling};const std::array<int,3> trunks={-9,-7,-8};const std::array<Position,3> sources={Position{-10,1},Position{-10,10},Position{-10,9}};
 for(size_t n=0;n<3;n++){if(rows[n].empty())continue;int x=trunks[n],low=std::min(sources[n].y,rows[n].begin()->first),high=std::max(sources[n].y,rows[n].rbegin()->first);run(sources[n],{x,sources[n].y},media[n]);run({x,low},{x,high},media[n]);for(auto [y,end]:rows[n])run({x,y},{end,y},media[n]);}
 return routes;
}
bool sameRoutes(const Routes& a,const Routes& b){if(a.size()!=b.size())return false;for(const auto& [p,links]:a){auto i=b.find(p);if(i==b.end()||i->second.size()!=links.size())return false;for(const auto& link:links){auto j=std::find_if(i->second.begin(),i->second.end(),[&](const auto& l){return l.medium==link.medium;});if(j==i->second.end()||j->rating!=link.rating||j->enabled!=link.enabled||j->ports!=link.ports||j->tripped)return false;}}return true;}
}
bool tidyDefaultPowerRoutesIn(FactoryState& state){
 auto old=legacyPowerDistrictBlueprint();for(const auto& part:old.parts){auto i=state.machines.find({part.at.x-27,part.at.y-1});if(i==state.machines.end()||i->second.kind!=part.machine.kind||i->second.dir!=part.machine.dir)return false;}
 // A changed cable, rating, isolation or machine direction makes this a custom layout.
 if(!sameRoutes(demoRoutes(state,true),state.energyLinks))return false;
 state.energyLinks=demoRoutes(state,false);
 for(auto& plan:state.blueprints)if(plan.name==old.name&&plan.width==old.width&&plan.height==old.height)plan=Factory::powerDistrictBlueprint();
 return true;
}
bool installPowerDistrictIn(FactoryState& state){
 using E=EnergyMedium;
 if(std::any_of(state.machines.begin(),state.machines.end(),[](const auto& e){return isEnergyMachine(e.second.kind);}))return false;
 auto plan=Factory::powerDistrictBlueprint();Position origin=powerDistrictOrigin;
 for(const auto& part:plan.parts)if(state.machines.count({origin.x+part.at.x,origin.y+part.at.y}))return false;
 for(const auto& part:plan.parts)state.machines.emplace(Position{origin.x+part.at.x,origin.y+part.at.y},part.machine);
 auto routes=demoRoutes(state,false);for(auto& [p,links]:routes)for(const auto& l:links){auto& existing=state.energyLinks[p];if(std::none_of(existing.begin(),existing.end(),[&](const auto& q){return q.medium==l.medium;}))existing.push_back(l);}
 auto& battery=state.machines.at({-20,-1});double reserve=std::min(250.,state.electricity);state.electricity-=reserve;battery.power.store[size_t(E::DC)]=reserve;state.grid.active=true;state.grid.stored=reserve;
 if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[&](const Blueprint& p){return p.name==plan.name;}))state.blueprints.push_back(std::move(plan));return true;
}
bool Factory::installPowerDistrict(){snapshot();bool changed=installPowerDistrictIn(*this);if(!changed){history.pop_back();notice="Power district already present or its western site is occupied. Use Plans to build another installation.";return false;}dirty=true;notice="Western power district installed. Actual generation now supplies connected factory and farm circuits.";return true;}
}
