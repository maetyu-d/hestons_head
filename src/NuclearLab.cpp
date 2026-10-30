#include "Power.h"
#include <algorithm>
namespace aftertaste {
bool isNuclearEquipment(MachineKind k){return k>=MachineKind::CuriosityPile&&k<=MachineKind::ExperimentConsole;}
double labTuningTarget(MachineKind k){return k==MachineKind::StarChamber?.70:.55;}
static Blueprint pipedNuclearLabBlueprint(){
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
Blueprint Factory::nuclearLabBlueprint(){
 auto b=pipedNuclearLabBlueprint();b.width=6;b.height=3;b.links.clear();
 const std::array<Position,10> dock={Position{2,0},{3,0},{1,0},{1,1},{2,1},{3,1},{0,2},{1,2},{2,2},{4,0}};
 for(size_t n=0;n<b.parts.size();n++)b.parts[n].at=dock[n];
 PowerLink cable;cable.medium=EnergyMedium::AC;cable.rating=100;cable.ports=5;b.links.push_back({{5,0},cable});return b;
}
namespace {
void connectLabExport(FactoryState& state,bool campus=false){
 using E=EnergyMedium;
 auto node=[&](Position p)->PowerLink&{auto& v=state.energyLinks[p];for(auto& l:v)if(l.medium==E::AC)return l;PowerLink l;l.medium=E::AC;l.rating=100;l.ports=0;v.push_back(l);return v.back();};
 if(campus)return; // Its output docks into the campus perimeter cable.
 Position p{nuclearLabOrigin.x+5,nuclearLabOrigin.y};
 for(Position end:{Position{-29,p.y},Position{-29,1},Position{-16,1}})while(p!=end){int d=p.x!=end.x?(p.x<end.x?0:2):(p.y<end.y?1:3);Position q=nextPosition(p,d);node(p).ports|=1<<d;node(q).ports|=1<<((d+2)%4);p=q;}
}
}
bool compactLegacyNuclearLabIn(FactoryState& state){
 const Position oldOrigin{-47,-2};auto old=pipedNuclearLabBlueprint(),compact=Factory::nuclearLabBlueprint();
 std::set<Position> oldParts;
 for(const auto& part:old.parts){Position p{oldOrigin.x+part.at.x,oldOrigin.y+part.at.y};auto i=state.machines.find(p);if(i==state.machines.end()||i->second.kind!=part.machine.kind||i->second.dir!=part.machine.dir)return false;oldParts.insert(p);}
 std::map<Position,std::vector<PowerLink>> expected;
 for(const auto& n:old.links)expected[{oldOrigin.x+n.at.x,oldOrigin.y+n.at.y}].push_back(n.link);
 // Only the stock apparatus footprint can move. Custom routing, isolation,
 // direction changes or construction in the new dock footprint block migration.
 for(int y=0;y<old.height;y++)for(int x=0;x<old.width;x++){
  Position p{oldOrigin.x+x,oldOrigin.y+y};if(state.machines.count(p)&&!oldParts.count(p))return false;
  auto a=state.energyLinks.find(p),b=expected.find(p);if((a==state.energyLinks.end())!=(b==expected.end()))return false;if(a==state.energyLinks.end())continue;
  if(a->second.size()!=b->second.size())return false;for(const auto& e:b->second){auto i=std::find_if(a->second.begin(),a->second.end(),[&](const auto& l){return l.medium==e.medium;});if(i==a->second.end()||i->ports!=e.ports||i->rating!=e.rating||i->enabled!=e.enabled||i->tripped)return false;}
 }
 for(const auto& part:compact.parts){Position p{nuclearLabOrigin.x+part.at.x,nuclearLabOrigin.y+part.at.y};if(state.machines.count(p)&&!oldParts.count(p))return false;}
 std::vector<Machine> live;for(const auto& part:old.parts){Position p{oldOrigin.x+part.at.x,oldOrigin.y+part.at.y};live.push_back(state.machines.at(p));state.machines.erase(p);}
 for(const auto& [p,links]:expected)state.energyLinks.erase(p);
 for(size_t n=0;n<compact.parts.size();n++)state.machines.emplace(Position{nuclearLabOrigin.x+compact.parts[n].at.x,nuclearLabOrigin.y+compact.parts[n].at.y},std::move(live[n]));
 for(const auto& n:compact.links)state.energyLinks[{nuclearLabOrigin.x+n.at.x,nuclearLabOrigin.y+n.at.y}].push_back(n.link);
 connectLabExport(state);for(auto& b:state.blueprints)if(b.name==old.name&&b.width==old.width&&b.height==old.height)b=compact;return true;
}
bool installNuclearLabIn(FactoryState& state){
 using K=MachineKind;using E=EnergyMedium;
 if(std::any_of(state.machines.begin(),state.machines.end(),[](const auto& p){return isNuclearEquipment(p.second.kind);}))return false;
 auto board=state.machines.find({powerCampusOrigin.x+4,powerCampusOrigin.y+2});bool campus=board!=state.machines.end()&&board->second.kind==K::Switchboard;
 if(!campus){board=state.machines.find({-14,1});if(board==state.machines.end()||board->second.kind!=K::Switchboard)return false;}
 auto plan=Factory::nuclearLabBlueprint();auto origin=campus?powerCampusLabOrigin:nuclearLabOrigin;
 // Only the campus plan's own perimeter/service lines may already occupy the
 // bench. A player cable, changed rating or apparatus still blocks installation.
 std::map<Position,std::vector<PowerLink>> planned;
 if(campus)for(const auto& n:Factory::powerDistrictBlueprint().links)planned[{powerCampusOrigin.x+n.at.x,powerCampusOrigin.y+n.at.y}].push_back(n.link);
 for(int y=0;y<plan.height;y++)for(int x=0;x<plan.width;x++){Position p{origin.x+x,origin.y+y};if(state.machines.count(p))return false;auto i=state.energyLinks.find(p);if(i!=state.energyLinks.end()){
  auto expected=planned.find(p);if(expected==planned.end()||i->second.size()!=expected->second.size())return false;
  for(const auto& l:i->second){auto q=std::find_if(expected->second.begin(),expected->second.end(),[&](const auto& e){return e.medium==l.medium;});if(q==expected->second.end()||q->ports!=l.ports||q->rating!=l.rating||q->enabled!=l.enabled||l.tripped)return false;}
 }}
 for(int x=-28;!campus&&x<-27;x++)if(state.machines.count({x,1})||state.energyLinks.count({x,1}))return false;
 for(const auto& part:plan.parts)state.machines.emplace(Position{origin.x+part.at.x,origin.y+part.at.y},part.machine);
 for(const auto& node:plan.links){auto& v=state.energyLinks[{origin.x+node.at.x,origin.y+node.at.y}];auto i=std::find_if(v.begin(),v.end(),[&](const auto& l){return l.medium==node.link.medium;});if(i==v.end())v.push_back(node.link);else if(!campus)i->ports|=node.link.ports;}
 connectLabExport(state,campus);
 auto& pile=state.machines.at({origin.x+1,origin.y+1});pile.power.lab.fuel=60;pile.power.lab.armed=true;
 auto& star=state.machines.at({origin.x+1,origin.y+2});star.power.lab.fuel=30;star.power.lab.armed=true;
 auto& battery=state.machines.at({origin.x+2,origin.y});double startup=std::min(40.,state.electricity);state.electricity-=startup;battery.power.store[size_t(E::DC)]=startup;
 if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[&](const auto& p){return p.name==plan.name;}))state.blueprints.push_back(plan);
 return true;
}
}
