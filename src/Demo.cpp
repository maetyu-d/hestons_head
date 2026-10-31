#include "Factory.h"
#include "Power.h"
#include "WaterWorks.h"
#include "ReagentWorks.h"
#include <algorithm>
#include <stdexcept>

namespace aftertaste {
void Factory::resetDemo(){
 using K=MachineKind;using E=EnergyMedium;
 static_cast<FactoryState&>(*this)=FactoryState{};running=false;history.clear();expandedStarterDemo=true;
 for(const auto& i:pantry())stock[i.key]=40;
 for(const auto& crop:farmCropKeys())seedStock[crop]=36;
 water=80;electricity=600;farmProduce=5;chemicals=.15;
 auto add=[&](Position p,K k,int d=0,const std::string& ingredientKey="brine",double dose=100)->Machine&{
  Machine m;m.kind=k;m.dir=d;m.ingredient=ingredientKey;m.dose=dose;m.duration=std::max(1,machineSpec(k).duration);m.temperature=70;
  if(isEnergyMachine(k))initialisePowerDevice(m);
  if(k==K::Chiller)m.temperature=-24;if(k==K::ColdStore||k==K::GlassCondenser)m.temperature=4;
  if(k==K::SousVide)m.temperature=64;if(k==K::Fermenter)m.temperature=29;
  if(k==K::GlassBeaker||k==K::FractionReceiver)m.automatic=false;
  if(k==K::LatticeReactor){m.temperature=70;m.automatic=false;}
  auto [i,ok]=machines.emplace(p,m);if(!ok)throw std::logic_error("Demo machine overlap at "+positionName(p));return i->second;
 };
 auto link=[&](Position p,E medium)->PowerLink&{auto& layers=energyLinks[p];for(auto& l:layers)if(l.medium==medium)return l;PowerLink l;l.medium=medium;l.rating=medium==E::AC?180:60;l.ports=0;layers.push_back(l);return layers.back();};
 auto route=[&](E medium,std::initializer_list<Position> corners){auto i=corners.begin();Position p=*i++;link(p,medium);for(;i!=corners.end();++i)while(p!=*i){int d=p.x!=i->x?(p.x<i->x?0:2):(p.y<i->y?1:3);auto q=nextPosition(p,d);link(p,medium).ports|=1<<d;link(q,medium).ports|=1<<((d+2)%4);p=q;}};
 auto install=[&](Blueprint b,Position origin){for(const auto& p:b.parts){auto [i,ok]=machines.emplace(Position{origin.x+p.at.x,origin.y+p.at.y},p.machine);if(!ok)throw std::logic_error("Demo plan overlap");}for(const auto& l:b.links){auto& q=link({origin.x+l.at.x,origin.y+l.at.y},l.link.medium);q.ports|=l.link.ports;q.rating=std::max(q.rating,l.link.rating);}blueprints.push_back(std::move(b));};
 // Northwest: one contact bench, with finite experimental fuel and solar/wind backup.
 for(Position p:std::vector<Position>{{-16,-12},{-15,-12},{-16,-11}})add(p,K::SolarPanel);
 add({-15,-11},K::BatteryRack).power.store[size_t(E::DC)]=250;
 add({-14,-11},K::Inverter);add({-13,-11},K::Switchboard);add({-12,-11},K::BatteryRack);
 install(nuclearLabBlueprint(),{-12,-10});
 for(auto& [p,m]:machines)if(isNuclearEquipment(m.kind)){if(m.kind==K::CuriosityPile){m.power.lab.fuel=100;m.power.lab.armed=true;m.power.lab.tuning=.55;}if(m.kind==K::StarChamber){m.power.lab.fuel=50;m.power.lab.armed=false;m.power.lab.tuning=.70;}}
 add({-16,-8},K::WindRotor);add({-15,-8},K::Gearbox).shear=1.8;add({-14,-8},K::Alternator);add({-13,-8},K::Switchboard);
 // Bridge the solar row to the experimental bench with a single contact cabinet.
 add({-12,-10},K::Switchboard);
 route(E::AC,{{-7,-10},{-5,-10},{-5,16}});link({-7,-10},E::AC).ports|=4;
 route(E::AC,{{-5,-10},{-5,-12}});
 // North: six complete crop lanes. Every harvested crop remains a visible batch.
 install(marketGardenBlueprint(),{-2,-13});
 const double growth[]={.99,.88,.95,.72,.83,.67};
 for(int i=0;i<6;i++){Position p{-1+(i%2)*9,-11+(i/2)*3};auto& bed=machines.at(p);if(i==1||i==5){bed.ingredient=i==1?"berry":"greens";machines.at({p.x,p.y-1}).ingredient=bed.ingredient;}seedStock[bed.ingredient]--;bed.farmPlanted=true;bed.farmGrowth=growth[i];bed.farmHealth=1;bed.farmAge=int(growth[i]*55);}
 for(int y:{-11,-8,-5})route(E::AC,{{-5,y},{14,y}});
 // Tomato harvest bypasses inventory and drops through the shared receiving aisle.
 machines.at({5,-11}).forward=true;machines.at({5,-11}).automatic=false;
 for(int y=-11;y<=-3;y++)add({6,y},K::Belt,y==-3?2:1);
 for(int x=-1;x<6;x++)add({x,-3},K::Belt,x==-1?1:2);
 add({-1,-2},K::Belt,1);add({-1,-1},K::Belt,1);add({-1,0},K::Belt,0);
 // West: water and reagents share the same straight electrical spine.
 install(waterWorksBlueprint(),{-16,-5});
 route(E::AC,{{-10,-4},{-5,-4}});link({-10,-4},E::AC).ports|=4;
 add({-8,-3},K::CoolingPlant);
 route(E::AC,{{-9,-4},{-9,-3}});link({-9,-3},E::AC).ports|=1;
 route(E::Cooling,{{-7,-3},{-4,-3},{-4,16}});link({-7,-3},E::Cooling).ports|=4;
 install(reagentWorksBlueprint(),{-16,3});
 route(E::AC,{{-10,3},{-5,3}});link({-10,3},E::AC).ports|=2;
 machines.at({-16,3}).ingredient="berry";machines.at({-15,3}).ingredient="pectin";
 machines.at({-15,6}).ingredient="nutrient";machines.at({-16,6}).reagentProcess.minerals=900;
 for(int x=0;x<7;x++)add({-16+x,12},x==0?K::Feeder:x==1?K::ReagentExtractor:x==2?K::ReagentSeparator:x==3?K::ReagentPolisher:x==4?K::ReagentEvaporator:x==5?K::ReagentTank:K::ReagentIntake);
 machines.at({-16,12}).ingredient="yolk";machines.at({-16,12}).dose=25;machines.at({-15,12}).ingredient="lecithin";
 for(int x=1;x<=4;x++)add({-16+x,13},K::ReagentRecovery);
 add({-11,13},K::ReagentAnalyzer,3);route(E::AC,{{-10,10},{-10,13}});
 machines.at({-16,9}).ingredient="carrot";machines.at({-15,9}).ingredient="xanthan";machines.at({-15,9}).reagentProcess.starter=15;
 // Extraction residue powers a real biogas recovery train, rather than a waste sink.
 machines.erase({-15,4});add({-15,4},K::Belt,2);add({-16,4},K::Belt,2);
 for(int y=4;y<=15;y++)add({-17,y},K::Belt,y==15?0:1);
 add({-16,15},K::Macerator);add({-15,15},K::AnaerobicDigester);add({-14,15},K::GasScrubber);add({-13,15},K::GasHolder);add({-12,15},K::GasEngine);add({-11,15},K::Alternator);add({-10,15},K::Switchboard);
 add({-13,16},K::FeedPump);add({-11,16},K::HeatTank).forward=true;
 add({-15,16},K::CompostBin);auto& nursery=add({-14,16},K::RaisedBed,0,"basil");nursery.temperature=20;
 add({-14,17},K::SeedHopper,3,"basil");route(E::AC,{{-9,15},{-5,15}});link({-9,15},E::AC).ports|=4;
 // Central cooking hall: short parallel lines with real side-stream recombination.
 add({0,0},K::Doser,0,"lecithin",.8);add({1,0},K::Doser,0,"oil",25);add({2,0},K::Homogenizer);add({3,0},K::Centrifuge);
 add({4,0},K::GlassTube);add({5,0},K::SousVide);add({6,0},K::GlassCoil);add({7,0},K::Plater);add({8,0},K::Chiller);
 add({3,1},K::Fermenter);add({4,1},K::Aerator);add({5,1},K::GlassTube);add({6,1},K::GlassTube);add({7,1},K::GlassTube,3);
 add({2,-1},K::DropletViewer,1);add({5,-1},K::TemperatureLogger,1);
 add({-1,4},K::Feeder,0,"carrot",80);add({0,4},K::Mill);add({1,4},K::Doser,0,"pectin",1.2);add({2,4},K::Heater).temperature=90;add({3,4},K::Gel);add({4,4},K::Plater);add({5,4},K::Cloche);add({6,4},K::Chiller);
 add({4,2},K::Feeder,1,"cream",25);add({4,3},K::Chiller,1);add({2,3},K::PHProbe,1);
 // Lattice/glass research is on the floor, beside the working production lanes.
 add({-1,8},K::Feeder,0,"basil",40).automatic=false;add({0,8},K::GlassTube);add({1,6},K::Feeder,1,"oil",15).automatic=false;add({1,7},K::GlassTube,1);add({1,8},K::Mixer);add({2,8},K::Burette,0,"lecithin",1.2);add({3,8},K::GlassFlask);add({4,8},K::GlassCondenser);add({5,8},K::LatticeReactor);add({6,8},K::GlassTube);add({7,8},K::Homogenizer);add({8,8},K::GlassValve);add({3,7},K::TemperatureLogger,1);add({4,7},K::AromaSensor,1);
 // South: a three-cut distillation row; separate fractions leave through the south wall.
 add({1,12},K::Feeder,0,"citrus",80).automatic=false;
 for(int x:{2,6,10}){auto& still=add({x,12},K::FractionatingStill);still.cut=(x-2)/4;still.temperature=78+still.cut*9;auto& column=add({x,13},K::FractionatingColumn,2);column.stages=8+still.cut*4;column.temperature=still.temperature;add({x-1,13},K::GlassCondenser,1);for(int y=14;y<18;y++)add({x-1,y},K::GlassTube,1);add({x-1,18},K::FractionReceiver).serveStyle=ServeStyle::ResearchTasting;}
 for(int x:{3,4,5,7,8,9})add({x,12},K::GlassTube);
 for(int y:{0,4,8,12}){int start=y==0?9:y==4?7:y==8?9:11;for(int x=start;x<=16;x++)add({x,y},y==12?K::GlassTube:K::Belt);auto& dock=add({17,y},K::Dispatch);if(y==8){dock.automatic=false;dock.serveStyle=ServeStyle::ResearchTasting;}else if(y==4)dock.serveStyle=ServeStyle::ChilledPickup;route(E::AC,{{-5,y},{15,y}});}
 // A complete wall, with legitimate conveyor and serving thresholds.
 for(int x=-3;x<=16;x++)for(int y:{-2,18})if(!machines.count({x,y}))add({x,y},K::Wall);
 for(int y=-1;y<18;y++)for(int x:{-3,16})if(!machines.count({x,y}))add({x,y},K::Wall);
 // Short AC drops feed the distillation condensers and the research feeder.
 for(int x:{1,5,9})route(E::AC,{{x,12},{x,17}});
 for(int x:{3,4})route(E::AC,{{x,0},{x,1}});
 for(int x:{2,6,10})route(E::AC,{{x,12},{x,13}});
 route(E::AC,{{1,8},{1,6}});route(E::AC,{{4,4},{4,2}});
 route(E::Heat,{{-12,16},{-6,16},{-6,12},{12,12}});link({-12,16},E::Heat).ports|=8;
 for(int y:{0,4,8,12})route(E::Cooling,{{-4,y},{15,y}});
 for(int x:{1,5,9})route(E::Cooling,{{x,12},{x,17}});
 route(E::AC,{{-5,16},{-5,17},{-15,17},{-15,16}});
 grid.active=true;
 // Named plans keep each district reusable, with its actual services and settings.
 captureBlueprint({-17,-12},{-7,-8},"01 / Contact power laboratory");
 captureBlueprint({-2,-13},{14,-4},"02 / Northern crop and receiving lanes");
 captureBlueprint({-16,-5},{-10,-1},"03 / Water treatment and return loop");
 captureBlueprint({-17,3},{-10,17},"04 / Reagents to biogas and soil recovery");
 captureBlueprint({-1,-1},{17,1},"05 / Tomato emulsion and cultured side fraction");
 captureBlueprint({-1,2},{17,4},"06 / Carrot gel and chilled cream");
 captureBlueprint({-1,6},{17,8},"07 / Glass and lattice research line");
 captureBlueprint({0,12},{17,18},"08 / Three-cut aroma collection");
 // Load only manual research feeds; continuous lanes obtain their inputs normally.
 dispense({-1,8});dispense({1,6});dispense({1,12});
 // Stage the showcase by running its real processes, without granting outputs.
 for(int n=0;n<48;n++)step();
 running=false;history.clear();dirty=false;
 notice="New integrated campus (48 real ticks, paused): northern crops feed the cooking hall; western power, water and reagent labs share one service spine. Extraction leftovers fuel biogas; kitchen side fractions rejoin at the plater. Serve east, collect fractions south, and experiment in the lattice line.";
}
}
