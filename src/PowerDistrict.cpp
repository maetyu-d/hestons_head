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
static Blueprint routedPowerDistrictBlueprint(bool clearance){
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
 int auxiliaryRow=clearance?4:5;run(E::AC,{{2,6},{2,auxiliaryRow},{11,auxiliaryRow}});run(E::AC,{{2,9},{13,9},{13,10}});
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
static Blueprint dockedPowerDistrictBlueprint(){
 using E=EnergyMedium;auto b=routedPowerDistrictBlueprint(true);b.links.clear();
 const std::map<size_t,Position> docks={{0,{1,1}},{1,{2,1}},{2,{1,2}},{3,{2,2}},{4,{3,2}},{5,{4,2}},{8,{2,4}},{9,{3,4}},{14,{3,8}},{15,{4,8}},{16,{5,8}},{17,{6,14}},{26,{3,14}},{27,{4,14}},{28,{5,14}},{29,{7,14}}};
 for(auto [n,p]:docks)b.parts.at(n).at=p;
 std::map<std::pair<Position,E>,PowerLink> links;
 auto node=[&](Position p,E e)->PowerLink&{auto [i,inserted]=links.emplace(std::make_pair(p,e),PowerLink{});if(inserted){i->second.medium=e;i->second.rating=e==E::AC?100:60;i->second.ports=0;}return i->second;};
 auto run=[&](E e,std::initializer_list<Position> points){auto i=points.begin();Position p=*i++;node(p,e);for(;i!=points.end();++i)while(p!=*i){int d=p.x!=i->x?(p.x<i->x?0:2):(p.y<i->y?1:3);auto q=nextPosition(p,d);node(p,e).ports|=1<<d;node(q,e).ports|=1<<((d+2)%4);p=q;}};
 // The solar bank has no DC cabling. Mechanical/steam/gas trains dock at
 // their actual process ports. Only outputs and separated fluid loops use lines.
 run(E::AC,{{11,0},{11,16}});run(E::AC,{{16,1},{16,0},{11,0}});
 run(E::AC,{{5,2},{11,2}});run(E::AC,{{12,3},{11,3}});run(E::AC,{{14,3},{17,3}});
 run(E::AC,{{4,4},{11,4}});run(E::AC,{{6,8},{11,8}});
 run(E::AC,{{2,7},{2,5},{11,5}});run(E::AC,{{2,10},{13,10},{13,11}});
 run(E::AC,{{5,13},{11,13}});run(E::AC,{{2,16},{11,16}});
 run(E::Exhaust,{{4,9},{4,10},{5,10},{5,11}});
 run(E::Water,{{3,10},{7,10},{7,11}});run(E::Water,{{3,10},{3,11}});
 run(E::Water,{{2,7},{3,7},{3,10}});run(E::Water,{{3,10},{3,13}});
 run(E::Heat,{{1,12},{17,12}});run(E::Heat,{{2,9},{1,9},{1,12}});
 run(E::Heat,{{6,12},{6,12}});run(E::Heat,{{9,11},{9,12}});
 run(E::Heat,{{11,11},{11,12}});run(E::Heat,{{13,12},{13,10},{14,10}});
 run(E::Heat,{{5,15},{9,15},{9,12}});run(E::Heat,{{2,13},{2,12}});
 run(E::Cooling,{{15,11},{17,11}});
 for(const auto& part:b.parts)for(auto port:energyPorts(part.machine)){auto i=links.find({nextPosition(part.at,port.direction),port.medium});if(i!=links.end())i->second.ports|=1<<((port.direction+2)%4);}
 node({2,10},E::AC).ports|=8;node({2,16},E::AC).ports|=8;
 node({17,3},E::AC).ports|=1;node({17,11},E::Cooling).ports|=1;node({17,12},E::Heat).ports|=1;
 for(const auto& [key,link]:links)b.links.push_back({key.first,link});return b;
}
static Blueprint compactPowerDistrictBlueprint(){
 using E=EnergyMedium;auto b=dockedPowerDistrictBlueprint();b.width=12;b.height=10;b.links.clear();
 // Two benches at the top, three short process rows below. Auxiliaries sit at
 // the train they support, rather than as isolated machines across open floor.
 const std::array<Position,34> slots={Position{0,1},{1,1},{0,2},{1,2},{2,2},{3,2},{3,1},
  {0,3},{1,3},{2,3},{4,2},{0,5},{1,5},{2,5},{3,5},{4,5},{5,5},{6,8},
  {5,6},{1,4},{7,4},{8,4},{9,4},{0,8},{1,8},{2,8},{3,8},{4,8},{5,8},{7,8},
  {2,6},{3,6},{2,9},{3,9}};
 for(size_t n=0;n<slots.size();n++)b.parts[n].at=slots[n];
 b.parts[22].machine.dir=3; // Heat tank -> cooling plant's heat inlet.
 std::map<std::pair<Position,E>,PowerLink> links;
 auto node=[&](Position p,E e)->PowerLink&{auto [i,inserted]=links.emplace(std::make_pair(p,e),PowerLink{});if(inserted){i->second.medium=e;i->second.rating=e==E::AC?100:60;i->second.ports=0;}return i->second;};
 auto run=[&](E e,std::initializer_list<Position> points){auto i=points.begin();Position p=*i++;node(p,e);for(;i!=points.end();++i)while(p!=*i){int d=p.x!=i->x?(p.x<i->x?0:2):(p.y<i->y?1:3);auto q=nextPosition(p,d);node(p,e).ports|=1<<d;node(q,e).ports|=1<<((d+2)%4);p=q;}};
 // Collector inside the campus. The switchboard separates this from the
 // export cable, which follows the clear top/right perimeter to the factory.
 run(E::AC,{{2,4},{5,4},{5,3}});run(E::AC,{{4,3},{4,4}});
 run(E::AC,{{6,5},{6,4},{5,4}});run(E::AC,{{5,7},{6,7},{6,5}});
 run(E::AC,{{8,8},{8,7},{6,7}});run(E::AC,{{9,5},{9,6},{6,6}});
 run(E::AC,{{4,1},{4,1}});node({4,1},E::AC).ports=6;
 run(E::AC,{{5,2},{5,0},{11,0},{11,4}});
 node({5,3},E::AC).ports|=1; // Vacuum bench's west AC socket.
 node({11,1},E::AC).ports|=4; // Neighbouring nuclear bench's switchboard.
 node({11,4},E::AC).ports|=1;
 // Closed working-fluid loop immediately beside the steam train.
 run(E::Exhaust,{{4,6},{4,6}});node({4,6},E::Exhaust).ports=9;
 run(E::Water,{{2,4},{6,4},{6,7},{3,7}});
 // Recovery header below the thermal row, above the gas row; the exchanger,
 // heat tank and cooling plant form one adjoining service bench.
 run(E::Heat,{{2,6},{2,7},{6,7},{6,4}});run(E::Heat,{{5,7},{6,7}});
 run(E::Heat,{{5,9},{6,9},{6,7}});run(E::Heat,{{9,4},{11,4}});
 run(E::Cooling,{{9,3},{11,3}});node({11,3},E::Cooling).ports|=1;
 for(const auto& part:b.parts)for(auto port:energyPorts(part.machine)){auto i=links.find({nextPosition(part.at,port.direction),port.medium});if(i!=links.end())i->second.ports|=1<<((port.direction+2)%4);}
 for(const auto& [key,link]:links)b.links.push_back({key.first,link});return b;
}
Blueprint Factory::powerDistrictBlueprint(){return compactPowerDistrictBlueprint();}
namespace {
using Routes=std::map<Position,std::vector<PowerLink>>;
Routes demoRoutes(const FactoryState& state,bool legacy,bool clearance=true,bool docked=true){
 using K=MachineKind;using E=EnergyMedium;Routes routes;auto b=legacy?legacyPowerDistrictBlueprint():docked?dockedPowerDistrictBlueprint():routedPowerDistrictBlueprint(clearance);Position origin=legacy?Position{-27,-1}:powerDistrictOrigin;
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
// Distribution outside the generation campus retains the factory's established
// row taps. Only three bundled services travel between the campus and the works.
Routes compactCampusRoutes(const FactoryState& state){
 using E=EnergyMedium;Routes routes;
 auto node=[&](Position p,E e)->PowerLink&{auto& v=routes[p];auto i=std::find_if(v.begin(),v.end(),[&](const auto& l){return l.medium==e;});if(i!=v.end())return *i;PowerLink l;l.medium=e;l.rating=100;l.ports=0;v.push_back(l);return v.back();};
 auto run=[&](Position p,Position end,E e){node(p,e);while(p!=end){int d=p.x!=end.x?(p.x<end.x?0:2):(p.y<end.y?1:3);auto q=nextPosition(p,d);node(p,e).ports|=1<<d;node(q,e).ports|=1<<((d+2)%4);p=q;}};
 for(const auto& n:compactPowerDistrictBlueprint().links){Position p{powerCampusOrigin.x+n.at.x,powerCampusOrigin.y+n.at.y};node(p,n.link.medium)=n.link;}
 const std::array<E,3> media={E::AC,E::Heat,E::Cooling};
 const std::array<Position,3> outlets={Position{-12,0},Position{-12,0},Position{-12,-1}};
 const std::array<Position,3> sources={Position{-10,1},Position{-10,10},Position{-10,9}};
 const std::array<int,3> trunks={-9,-7,-8};
 std::array<std::map<int,int>,3> rows;
 for(const auto& [p,m]:state.machines){if(isEnergyMachine(m.kind)||machineSpec(m.kind).power<=0||p.x<-9)continue;Position sink=p;auto row=[&](int n){auto [i,inserted]=rows[size_t(n)].emplace(sink.y,sink.x);if(!inserted)i->second=std::max(i->second,sink.x);};row(0);if(m.kind==MachineKind::Heater||m.kind==MachineKind::SousVide||m.kind==MachineKind::FractionatingStill||m.kind==MachineKind::GlassFlask)row(1);if(m.kind==MachineKind::Chiller||m.kind==MachineKind::ColdStore||m.kind==MachineKind::GlassCondenser)row(2);}
 for(size_t n=0;n<3;n++){run(outlets[n],{-11,outlets[n].y},media[n]);run({-11,outlets[n].y},{-11,sources[n].y},media[n]);run({-11,sources[n].y},sources[n],media[n]);if(rows[n].empty())continue;int x=trunks[n],low=std::min(sources[n].y,rows[n].begin()->first),high=std::max(sources[n].y,rows[n].rbegin()->first);run(sources[n],{x,sources[n].y},media[n]);run({x,low},{x,high},media[n]);for(auto [y,end]:rows[n])run({x,y},{end,y},media[n]);}
 return routes;
}
bool sameRoutes(const Routes& a,const Routes& b){if(a.size()!=b.size())return false;for(const auto& [p,links]:a){auto i=b.find(p);if(i==b.end()||i->second.size()!=links.size())return false;for(const auto& link:links){auto j=std::find_if(i->second.begin(),i->second.end(),[&](const auto& l){return l.medium==link.medium;});if(j==i->second.end()||j->rating!=link.rating||j->enabled!=link.enabled||j->ports!=link.ports||j->tripped)return false;}}return true;}
}
bool tidyDefaultPowerRoutesIn(FactoryState& state){
 auto old=legacyPowerDistrictBlueprint();for(const auto& part:old.parts){auto i=state.machines.find({part.at.x-27,part.at.y-1});if(i==state.machines.end()||i->second.kind!=part.machine.kind||i->second.dir!=part.machine.dir)return false;}
 // A changed cable, rating, isolation or machine direction makes this a custom layout.
 if(!sameRoutes(demoRoutes(state,true),state.energyLinks)&&!sameRoutes(demoRoutes(state,false,false,false),state.energyLinks))return false;
 state.energyLinks=demoRoutes(state,false,true,false);
 for(auto& plan:state.blueprints)if(plan.name==old.name&&plan.width==old.width&&(plan.height==old.height||plan.height==18))plan=routedPowerDistrictBlueprint(true);
 return true;
}
bool dockDefaultPowerDevicesIn(FactoryState& state){
 using E=EnergyMedium;auto old=routedPowerDistrictBlueprint(true),next=dockedPowerDistrictBlueprint();std::set<Position> owned;
 for(const auto& part:old.parts){Position p{powerDistrictOrigin.x+part.at.x,powerDistrictOrigin.y+part.at.y};auto m=state.machines.find(p);if(m==state.machines.end()||m->second.kind!=part.machine.kind||m->second.dir!=part.machine.dir)return false;owned.insert(p);}
 auto worldRoutes=[&](const Blueprint& plan){Routes r;for(const auto& n:plan.links)r[{powerDistrictOrigin.x+n.at.x,powerDistrictOrigin.y+n.at.y}].push_back(n.link);
  // The separate nuclear bench joins this collector through its existing cable.
  auto lab=state.machines.find({nuclearLabOrigin.x+1,nuclearLabOrigin.y+1});if(lab!=state.machines.end()&&lab->second.kind==MachineKind::CuriosityPile)for(int x=-27;x<=-16;x++){
   auto& v=r[{x,1}];auto i=std::find_if(v.begin(),v.end(),[](const auto& l){return l.medium==E::AC;});if(i==v.end()){PowerLink l;l.medium=E::AC;l.rating=100;l.ports=5;v.push_back(l);}else i->ports|=4;
  }
  return r;};
 auto expected=worldRoutes(old);Routes actual;
 for(int y=0;y<old.height;y++)for(int x=0;x<old.width;x++){Position p{powerDistrictOrigin.x+x,powerDistrictOrigin.y+y};if(state.machines.count(p)&&!owned.count(p))return false;auto i=state.energyLinks.find(p);if(i!=state.energyLinks.end())actual.emplace(p,i->second);}
 if(!sameRoutes(actual,expected))return false;
 for(const auto& part:next.parts){Position p{powerDistrictOrigin.x+part.at.x,powerDistrictOrigin.y+part.at.y};if(state.machines.count(p)&&!owned.count(p))return false;}
 std::vector<Machine> live;for(const auto& part:old.parts){Position p{powerDistrictOrigin.x+part.at.x,powerDistrictOrigin.y+part.at.y};live.push_back(state.machines.at(p));state.machines.erase(p);}
 for(const auto& [p,links]:actual)state.energyLinks.erase(p);
 for(size_t n=0;n<next.parts.size();n++)state.machines.emplace(Position{powerDistrictOrigin.x+next.parts[n].at.x,powerDistrictOrigin.y+next.parts[n].at.y},std::move(live[n]));
 for(const auto& [p,links]:worldRoutes(next))state.energyLinks[p]=links;
 for(auto& plan:state.blueprints)if(json(plan)==json(old))plan=next;
 return true;
}
bool compactDefaultPowerCampusIn(FactoryState& state){
 using E=EnergyMedium;
 const auto old=dockedPowerDistrictBlueprint(),next=compactPowerDistrictBlueprint(),lab=Factory::nuclearLabBlueprint();
 std::vector<std::pair<Position,Position>> moves;std::set<Position> owned;
 for(size_t n=0;n<old.parts.size();n++)moves.push_back({{powerDistrictOrigin.x+old.parts[n].at.x,powerDistrictOrigin.y+old.parts[n].at.y},{powerCampusOrigin.x+next.parts[n].at.x,powerCampusOrigin.y+next.parts[n].at.y}});
 bool hasLab=state.machines.count({nuclearLabOrigin.x+1,nuclearLabOrigin.y+1});
 if(hasLab)for(const auto& part:lab.parts)moves.push_back({{nuclearLabOrigin.x+part.at.x,nuclearLabOrigin.y+part.at.y},{powerCampusLabOrigin.x+part.at.x,powerCampusLabOrigin.y+part.at.y}});
 for(size_t n=0;n<moves.size();n++){auto i=state.machines.find(moves[n].first);const auto& expected=n<old.parts.size()?old.parts[n].machine:lab.parts[n-old.parts.size()].machine;if(i==state.machines.end()||i->second.kind!=expected.kind||i->second.dir!=expected.dir)return false;owned.insert(moves[n].first);}
 auto expected=demoRoutes(state,false);
 auto add=[&](Position p,int mask)->PowerLink&{auto& v=expected[p];auto i=std::find_if(v.begin(),v.end(),[](const auto& l){return l.medium==E::AC;});if(i==v.end()){PowerLink l;l.medium=E::AC;l.rating=100;l.ports=0;v.push_back(l);i=std::prev(v.end());}i->ports|=mask;return *i;};
 if(hasLab){for(const auto& n:lab.links)expected[{nuclearLabOrigin.x+n.at.x,nuclearLabOrigin.y+n.at.y}].push_back(n.link);Position p{nuclearLabOrigin.x+5,nuclearLabOrigin.y};for(Position end:{Position{-29,p.y},Position{-29,1},Position{-16,1}})while(p!=end){int d=p.x!=end.x?(p.x<end.x?0:2):(p.y<end.y?1:3);auto q=nextPosition(p,d);add(p,1<<d);add(q,1<<((d+2)%4));p=q;}}
 auto inOld=[](Position p){return p.x>=-35&&p.x<=-10&&p.y>=-2&&p.y<=15;};
 Routes wanted,actual;for(const auto& [p,v]:expected)if(inOld(p))wanted[p]=v;for(const auto& [p,v]:state.energyLinks)if(inOld(p))actual[p]=v;
 if(!sameRoutes(actual,wanted))return false;
 for(const auto& [p,m]:state.machines){bool inNew=p.x>=powerCampusOrigin.x&&p.x<powerCampusOrigin.x+next.width&&p.y>=powerCampusOrigin.y&&p.y<powerCampusOrigin.y+next.height;if((inOld(p)||inNew)&&!owned.count(p))return false;}
 auto replacement=compactCampusRoutes(state);
 for(const auto& [from,to]:moves)if(state.machines.count(to)&&!owned.count(to))return false;
 for(const auto& [p,v]:replacement)if(p.x<=-10&&!inOld(p)&&state.energyLinks.count(p))return false;
 // All guards precede mutation. Move the live objects, including food, charge,
 // fuel, wear and recorder history; only the cooler's sockets turn to dock.
 std::vector<Machine> live;for(const auto& [from,to]:moves){live.push_back(state.machines.at(from));state.machines.erase(from);}live[22].dir=next.parts[22].machine.dir;
 for(size_t n=0;n<moves.size();n++)state.machines.emplace(moves[n].second,std::move(live[n]));
 for(const auto& [p,v]:actual)state.energyLinks.erase(p);
 for(const auto& [p,v]:replacement)if(p.x<=-10)state.energyLinks[p]=v;
 for(auto& plan:state.blueprints)if(json(plan)==json(old))plan=next;
 return true;
}
bool installPowerDistrictIn(FactoryState& state){
 using E=EnergyMedium;
 if(std::any_of(state.machines.begin(),state.machines.end(),[](const auto& e){return isEnergyMachine(e.second.kind);}))return false;
 auto plan=Factory::powerDistrictBlueprint();Position origin=powerCampusOrigin;
 for(const auto& part:plan.parts)if(state.machines.count({origin.x+part.at.x,origin.y+part.at.y}))return false;
 for(const auto& part:plan.parts)state.machines.emplace(Position{origin.x+part.at.x,origin.y+part.at.y},part.machine);
 auto routes=compactCampusRoutes(state);for(auto& [p,links]:routes)for(const auto& l:links){auto& existing=state.energyLinks[p];if(std::none_of(existing.begin(),existing.end(),[&](const auto& q){return q.medium==l.medium;}))existing.push_back(l);}
 auto& battery=state.machines.at({powerCampusOrigin.x+1,powerCampusOrigin.y+2});double reserve=std::min(250.,state.electricity);state.electricity-=reserve;battery.power.store[size_t(E::DC)]=reserve;state.grid.active=true;state.grid.stored=reserve;
 if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[&](const Blueprint& p){return p.name==plan.name;}))state.blueprints.push_back(std::move(plan));return true;
}
bool Factory::installPowerDistrict(){snapshot();bool changed=installPowerDistrictIn(*this);if(!changed){history.pop_back();notice="Power district already present or its western site is occupied. Use Plans to build another installation.";return false;}dirty=true;notice="Western power district installed. Actual generation now supplies connected factory and farm circuits.";return true;}
}
