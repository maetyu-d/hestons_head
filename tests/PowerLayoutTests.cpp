#include "Power.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aftertaste;
using K=MachineKind;using E=EnergyMedium;
static int checks=0;
static void require(bool ok,const char* reason){++checks;if(!ok)throw std::runtime_error(reason);}
static int verticalRuns(const FactoryState& f){int n=0;for(const auto& [p,links]:f.energyLinks)if(p.x>=-9&&p.x!=-9)for(const auto& l:links)if(l.medium==E::AC&&(l.ports&10))++n;return n;}
int main(int argc,char** argv){try{
 Factory f;require(verticalRuns(f)==0,"factory distribution has one vertical AC trunk, rather than a mesh of individual risers");
 auto plan=Factory::powerDistrictBlueprint();for(const auto& link:plan.links)require(link.at.x>=0&&link.at.y>=0&&link.at.x<plan.width&&link.at.y<plan.height&&validPowerLink(link.link),"all planned routes fit the reusable module");
 Blueprint b;b.width=3;b.height=2;PowerLink l;l.ports=5;b.links.push_back({{1,0},l});auto turned=Factory::rotatedBlueprint(b);require(turned.links.front().link.ports==10,"rotating a plan rotates cable connections");
 Position light;bool found=false;for(auto& [p,m]:f.machines){if(isEnergyMachine(m.kind))m.enabled=m.kind==K::GridInlet||m.kind==K::Switchboard||m.kind==K::PowerRelay;if(m.kind==K::GridInlet)m.power.rate=1000;if(m.kind==K::GrowLight&&!found){light=p;found=true;}}
 require(found,"demo has a real farm load");f.electricity=100000;f.water=10000;f.beginPowerStep();require(f.powerAvailable(light,machineSpec(K::GrowLight).power),"shared trunks supply the northern farm");f.usePower(light,machineSpec(K::GrowLight).power);f.finishPowerStep();
 for(auto& [p,m]:f.machines)if(m.kind==K::Switchboard)m.enabled=false;f.beginPowerStep();require(!f.powerAvailable(light,machineSpec(K::GrowLight).power),"the tidy layout cannot bypass an open switchboard");
 if(argc>1){std::ifstream input(argv[1]);json j;input>>j;auto original=j.at("factory").get<FactoryState>();Factory upgraded;upgraded.storageFile=argv[1];require(upgraded.load(),"load the saved demo with old automatic wiring");require(upgraded.machines.size()==original.machines.size(),"tidying only changes routes");for(const auto& [p,m]:original.machines)require(upgraded.at(p)&&json(*upgraded.at(p))==json(m),"machine settings, live food, crops and stored energy remain exact");require(upgraded.water==original.water&&upgraded.electricity==original.electricity&&upgraded.chemicals==original.chemicals&&upgraded.tick==original.tick,"tidying neither consumes resources nor advances the clock");require(verticalRuns(upgraded)==0&&verticalRuns(original)>0,"old demo wiring is replaced by the single-trunk layout");
  std::cout<<"AC riser cells outside the main trunk: "<<verticalRuns(original)<<" -> "<<verticalRuns(upgraded)<<"\n";
  auto file=std::filesystem::temp_directory_path()/"aftertaste-custom-power-layout.json";for(auto& [p,links]:original.energyLinks)for(auto& link:links)link.rating+=1;{std::ofstream out(file);out<<json{{"version",9},{"factory",original}};}Factory custom;custom.storageFile=file;require(custom.load()&&json(custom.energyLinks)==json(original.energyLinks),"custom wiring and ratings are never silently replaced");std::filesystem::remove(file);
  upgraded.storageFile=std::filesystem::temp_directory_path()/"aftertaste-tidy-power-layout.json";require(upgraded.save(),"save tidy routes");Factory again;again.storageFile=upgraded.storageFile;require(again.load()&&json(static_cast<FactoryState&>(again))==json(static_cast<FactoryState&>(upgraded)),"tidy layouts remain stable across reloads");std::filesystem::remove(upgraded.storageFile);
 }
 std::cout<<"PASS power layout: "<<checks<<" checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL power layout: "<<e.what()<<"\n";return 1;}}
