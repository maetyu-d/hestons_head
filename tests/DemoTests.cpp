#include "Factory.h"
#include "Power.h"
#include "WaterWorks.h"
#include <iostream>
#include <stdexcept>
using namespace aftertaste;
int main(){try{
 Factory passive;passive.machines.clear();passive.energyLinks.clear();passive.blueprints.clear();
 passive.build({0,0},MachineKind::Feeder);passive.build({1,0},MachineKind::GlassCoil);passive.build({2,0},MachineKind::Dispatch);passive.build({10,10},MachineKind::SolarPanel);passive.at({0,0})->automatic=false;passive.dispense({0,0});for(int n=0;n<20;n++)passive.step();if(passive.shipped!=1||passive.energy!=0)throw std::runtime_error("Passive residence coil stalled or consumed energy on a powered floor");
 Factory f;f.resetDemo();std::cout<<"Machines "<<f.machines.size()<<" routes "<<f.energyLinks.size()<<"\n";
 auto area=f.enclosedFactoryTiles();if(area.empty())throw std::runtime_error("Cooking hall is not enclosed");
 for(const auto& [p,m]:f.machines)if(isTastingDock(m.kind)&&area.count(p))throw std::runtime_error("Serving dock enclosed at "+positionName(p));
 if(!f.at({6,-11})||!f.at({-1,-2})||!f.at({5,-11})->forward)throw std::runtime_error("Direct harvest route missing");
 auto original=json(static_cast<const FactoryState&>(f));f.storageFile="/private/tmp/aftertaste-integrated-demo-test.json";if(!f.save())throw std::runtime_error("Demo save failed");Factory restored;restored.storageFile=f.storageFile;if(!restored.load()||json(static_cast<const FactoryState&>(restored))!=original)throw std::runtime_error("Demo save round trip changed world");
 double generated=0,delivered=0,heat=0;for(int i=0;i<360;i++){f.step();generated+=f.grid.generated;delivered+=f.grid.delivered;heat+=f.grid.recoveredHeat;}
 std::cout<<"After "<<f.tick<<" shipped "<<f.shipped<<" water produced "<<f.waterworks.extracted<<" delivered "<<f.waterworks.delivered<<" farm "<<f.farmProduce<<" reagent "<<f.chemicals<<" gen "<<generated<<" delivered "<<delivered<<" recovered "<<heat<<"\n";
 for(const auto& [p,m]:f.machines)if(isWaterMachine(m.kind)||m.kind==MachineKind::CoolingPlant||m.kind==MachineKind::Dispatch||m.kind==MachineKind::ReagentIntake||m.kind==MachineKind::AnaerobicDigester||m.kind==MachineKind::CuriosityPile||m.kind==MachineKind::LatticeReactor)std::cout<<positionName(p)<<" "<<machineSpec(m.kind).name<<" "<<m.status<<"\n";
 if(generated<=0||delivered<=0||f.waterworks.delivered<=0||f.shipped<=0)throw std::runtime_error("Integrated production did not run");
 if(f.reagentworks.inventory.empty())throw std::runtime_error("Reagent lab did not supply named chemicals");

 auto tomato=f.at({17,0})->lastOutput;if(!tomato||std::find(tomato->route.begin(),tomato->route.end(),Position{-1,-11})==tomato->route.end())throw std::runtime_error("Tomato harvest did not reach a served dish");
 for(int x:{1,5,9})if(!f.at({x,18})->item)throw std::runtime_error("A distillation cut did not reach its collection vessel");
 if(heat<=0)throw std::runtime_error("Shared thermal service did not supply cooking");
 if(!f.at({5,8})->item)throw std::runtime_error("Manual lattice experiment did not receive its feeds");
 std::cout<<"PASS integrated demo / enclosure, thresholds, direct harvest, save round trip and real production\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
