#include "Power.h"
#include <algorithm>
namespace aftertaste {
bool isNuclearEquipment(MachineKind k){return k>=MachineKind::CuriosityPile&&k<=MachineKind::ExperimentConsole;}
double labTuningTarget(MachineKind k){return k==MachineKind::StarChamber?.70:.55;}
Blueprint Factory::nuclearLabBlueprint(){
 using K=MachineKind;using E=EnergyMedium;Blueprint b;b.name="Curiosity laboratory / nuclear experiments";b.width=19;b.height=13;
 auto add=[&](Position p,K kind)->Machine&{Machine m;m.kind=kind;initialisePowerDevice(m);m.duration=kind==K::StarChamber?6:1;b.parts.push_back({p,m});return b.parts.back().machine;};
 add({1,1},K::BatteryRack);add({4,1},K::Inverter).power.rate=80;
 add({7,2},K::ExperimentConsole);add({10,4},K::CuriosityPile);add({14,4},K::Calorimeter);
 add({1,6},K::QuenchBath);add({5,9},K::LabVacuumPump);add({10,9},K::StarChamber);add({14,9},K::Calorimeter);add({17,3},K::Switchboard);
 std::map<std::pair<Position,E>,PowerLink> links;
 auto node=[&](Position p,E e)->PowerLink&{auto [i,inserted]=links.emplace(std::make_pair(p,e),PowerLink{});if(inserted){i->second.medium=e;i->second.rating=100;i->second.ports=0;}return i->second;};
 auto run=[&](E e,std::initializer_list<Position> points){auto i=points.begin();Position p=*i++;node(p,e);for(;i!=points.end();++i)while(p!=*i){int d=p.x!=i->x?(p.x<i->x?0:2):(p.y<i->y?1:3);Position q=nextPosition(p,d);node(p,e).ports|=1<<d;node(q,e).ports|=1<<((d+2)%4);p=q;}};
 run(E::DC,{{2,0},{16,0},{16,9},{15,9}});run(E::DC,{{2,1},{2,0}});run(E::DC,{{3,1},{3,0}});run(E::DC,{{15,4},{16,4}});
 run(E::AC,{{5,1},{5,2},{6,2}});run(E::AC,{{5,2},{5,8},{4,8},{4,9}});
 run(E::AC,{{5,7},{0,7},{0,6}});run(E::AC,{{5,8},{9,8},{9,9}});
 run(E::AC,{{5,8},{6,8},{6,12},{17,12},{17,4}});run(E::AC,{{18,3},{18,3}});node({18,3},E::AC).ports|=1;
 run(E::Cooling,{{2,6},{14,6},{14,5}});run(E::Cooling,{{10,5},{10,6}});run(E::Cooling,{{8,6},{8,10},{14,10}});
 run(E::Heat,{{11,4},{13,4}});run(E::Heat,{{11,9},{13,9}});
 run(E::Vacuum,{{6,9},{7,9},{7,7},{10,7},{10,8}});
 run(E::Instrument,{{8,2},{11,2},{11,8},{10,8}});run(E::Instrument,{{10,2},{10,3}});
 for(const auto& part:b.parts)for(auto port:energyPorts(part.machine)){auto i=links.find({nextPosition(part.at,port.direction),port.medium});if(i!=links.end())i->second.ports|=1<<((port.direction+2)%4);}
 for(auto& [key,link]:links)b.links.push_back({key.first,link});return b;
}
bool installNuclearLabIn(FactoryState& state){
 using K=MachineKind;using E=EnergyMedium;
 if(std::any_of(state.machines.begin(),state.machines.end(),[](const auto& p){return isNuclearEquipment(p.second.kind);}))return false;
 auto board=state.machines.find({-14,1});if(board==state.machines.end()||board->second.kind!=K::Switchboard)return false;
 auto plan=Factory::nuclearLabBlueprint();auto origin=nuclearLabOrigin;
 // The entire annex and its export corridor must be vacant. Custom work is kept.
 for(int y=0;y<plan.height;y++)for(int x=0;x<plan.width;x++){Position p{origin.x+x,origin.y+y};if(state.machines.count(p)||state.energyLinks.count(p))return false;}
 for(int x=-28;x<-27;x++)if(state.machines.count({x,1})||state.energyLinks.count({x,1}))return false;
 for(const auto& part:plan.parts)state.machines.emplace(Position{origin.x+part.at.x,origin.y+part.at.y},part.machine);
 for(const auto& node:plan.links)state.energyLinks[{origin.x+node.at.x,origin.y+node.at.y}].push_back(node.link);
 auto node=[&](Position p)->PowerLink&{auto& v=state.energyLinks[p];for(auto& l:v)if(l.medium==E::AC)return l;PowerLink l;l.medium=E::AC;l.rating=100;l.ports=0;v.push_back(l);return v.back();};
 for(int x=-29;x<-16;x++){node({x,1}).ports|=1;node({x+1,1}).ports|=4;}
 auto& pile=state.machines.at({origin.x+10,origin.y+4});pile.power.lab.fuel=60;pile.power.lab.armed=true;
 auto& star=state.machines.at({origin.x+10,origin.y+9});star.power.lab.fuel=30;star.power.lab.armed=true;
 auto& battery=state.machines.at({origin.x+1,origin.y+1});double startup=std::min(40.,state.electricity);state.electricity-=startup;battery.power.store[size_t(E::DC)]=startup;
 if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[&](const auto& p){return p.name==plan.name;}))state.blueprints.push_back(plan);
 return true;
}
}
