#include "ReagentWorks.h"
#include "Power.h"
#include "WaterWorks.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aftertaste;using K=MachineKind;using E=EnergyMedium;
static int checks=0;static void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
static const std::array<Position,34> oldSlots={Position{1,1},{2,1},{1,2},{2,2},{3,2},{4,2},{15,1},{1,4},{2,4},{3,4},{13,3},{0,8},{1,8},{2,8},{3,8},{4,8},{5,8},{6,14},{6,11},{2,11},{10,11},{12,11},{14,11},{0,14},{1,14},{2,14},{3,14},{4,14},{5,14},{7,14},{2,9},{3,9},{2,15},{3,15}};
static std::map<Position,Position> relocations(){std::map<Position,Position> moves;auto plan=Factory::powerDistrictBlueprint();for(size_t n=0;n<oldSlots.size();n++)moves[{powerDistrictOrigin.x+oldSlots[n].x,powerDistrictOrigin.y+oldSlots[n].y}]={powerRackOrigin.x+plan.parts[n].at.x,powerRackOrigin.y+plan.parts[n].at.y};for(const auto& n:Factory::nuclearLabBlueprint().parts)moves[{nuclearLabOrigin.x+n.at.x,nuclearLabOrigin.y+n.at.y}]={powerRackLabOrigin.x+n.at.x,powerRackLabOrigin.y+n.at.y};return moves;}
int main(int argc,char** argv){try{
 {Factory f;auto b=Factory::powerDistrictBlueprint();require(b.width==10&&b.height==7&&b.parts.size()==34,"generation module contracts from 18x18 to 10x7 without adding equipment");int devices=0;
 for(const auto& [p,m]:f.machines)if(p.x<-9&&!isWaterMachine(m.kind)&&p.y<reagentWorksOrigin.y){devices++;require(p.x>=powerRackOrigin.x&&p.x<powerRackOrigin.x+10&&p.y>=powerRackOrigin.y&&p.y<powerRackOrigin.y+7,"every western device, including the nuclear bench, fits one compact campus");bool adjacent=false;for(int d=0;d<4;d++)adjacent|=f.at(nextPosition(p,d))!=nullptr;require(adjacent,"no generation or support equipment is left isolated");}
 require(devices==44,"all 34 generator-yard and ten laboratory machines remain");
 auto cooler=f.at({powerRackOrigin.x+8,powerRackOrigin.y+3});require(cooler&&cooler->kind==K::CoolingPlant&&cooler->dir==3,"cooling plant turns its heat inlet directly toward the adjacent tank");
 f.water=100000;f.farmProduce=10000;f.chemicals=100;bool wind=false,steam=false,gas=false,pump=false,condense=false,cool=false;double delivered=0;
 for(int t=0;t<360;t++){f.step();delivered+=f.grid.delivered;for(const auto& [p,m]:f.machines){if(m.kind==K::Alternator&&p.y==powerRackOrigin.y+2)wind|=m.power.generated>0;if(m.kind==K::SteamTurbine)steam|=m.power.flow>0;if(m.kind==K::Flywheel)gas|=m.power.store[size_t(E::Shaft)]>0;if(m.kind==K::FeedPump)pump|=m.power.flow>0;if(m.kind==K::PowerCondenser)condense|=m.power.flow>0;if(m.kind==K::CoolingPlant)cool|=m.power.flow>0;}}
 require(wind&&steam&&gas,"all three compact process trains produce real work");require(pump&&condense&&cool,"nearby water return, condensation and heat recovery remain functional");require(delivered>1000,"compact campus supplies actual factory and farm demand");}
 if(argc>1){std::ifstream in(argv[1]);json j;in>>j;auto before=j.at("factory").get<FactoryState>();Factory f;f.storageFile=argv[1];require(f.load(),"load the user's version 14 factory");auto moves=relocations();require(f.machines.size()==before.machines.size()+12+Factory::reagentWorksBlueprint().parts.size(),"migration retains every machine");
 for(const auto& [p,m]:before.machines){auto q=moves.count(p)?moves.at(p):p;auto expected=m;if(m.kind==K::CoolingPlant&&moves.count(p))expected.dir=3;if(m.kind==K::PowerCondenser&&moves.count(p))expected.dir=1;require(f.at(q)&&json(*f.at(q))==json(expected),"relocated machines retain complete live state; only the cooler's sockets turn");}
 auto expected=before;expected.machines=f.machines;expected.energyLinks=f.energyLinks;expected.blueprints=f.blueprints;require(json(expected)==json(static_cast<const FactoryState&>(f)),"all clock, stock, resources, records and other factory state remain exact");
 for(auto mutation:{0,1,2,3}){auto custom=before;if(mutation==0)custom.energyLinks.at({-30,-2}).front().rating+=1;if(mutation==1)custom.machines[{powerRackOrigin.x,powerRackOrigin.y+1}]=Machine{};if(mutation==2)custom.machines.at({-26,-1}).dir=1;if(mutation==3){PowerLink l;l.medium=E::AC;custom.energyLinks[{powerRackOrigin.x+5,powerRackOrigin.y}].push_back(l);}auto snap=json(custom);require(!compactDefaultPowerCampusIn(custom)&&json(custom)==snap,"custom machines, directions and routes block migration atomically");}
 auto file=std::filesystem::temp_directory_path()/"aftertaste-compact-campus.json";f.storageFile=file;require(f.save(),"save compact campus");Factory loaded;loaded.storageFile=file;require(loaded.load()&&json(static_cast<FactoryState&>(loaded))==json(static_cast<FactoryState&>(f)),"reload neither moves nor recharges the campus");std::filesystem::remove(file);
 }
 std::cout<<"PASS compact power campus: "<<checks<<" checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL compact power: "<<e.what()<<" (check "<<checks<<")\n";return 1;}}
