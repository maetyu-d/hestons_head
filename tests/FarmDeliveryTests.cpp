#include "FloorTelemetry.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aftertaste;
static int checks=0;
static void require(bool ok,const char* why){checks++;if(!ok)throw std::runtime_error(why);}
static void disconnect(Factory& f){for(const auto& part:Factory::farmDeliveryBlueprint().parts)f.machines.erase({1+part.at.x,-10+part.at.y});for(auto& [p,m]:f.machines)if(m.kind==MachineKind::ProduceIntake)m.forward=false;}
int main(){try{
 Factory f;f.electricity=100000;f.water=10000;f.chemicals=100;
 for(auto& [p,m]:f.machines){if(m.kind==MachineKind::Feeder)m.enabled=false;if(isFarmBed(m.kind)){m.farmPlanted=true;m.farmGrowth=1;}if(m.kind==MachineKind::SeedHopper)m.automatic=false;}
 double before=f.farmProduce;FloorTelemetry t;t.observe(f,0);std::set<uint64_t> crops,delivered;bool actualMove=false,molecular=false;
 for(int tick=1;tick<=85;tick++){f.step();t.observe(f,tick);for(const auto& [id,sample]:t.samples)if(sample.material.cropBatch){crops.insert(id);if(sample.position==Position{1,0})delivered.insert(id);if(!sample.material.cropIntact)molecular=true;}for(const auto& move:t.motions)if(move.from==Position{1,-1}&&move.to==Position{1,0})actualMove=true;}
 require(crops.size()==6&&delivered.size()==6,"all six harvested batches physically reach the factory entrance");require(actualMove&&molecular,"the same crops animate into the factory and acquire molecular processing visuals");require(f.farmInventory.empty()&&f.farmProduce==before,"direct deliveries never duplicate food into shared stock");
 Factory held;held.machines.clear();require(held.build({0,0},MachineKind::Feeder)&&held.build({1,0},MachineKind::ProduceIntake)&&held.build({2,0},MachineKind::Belt),"handoff fixture");held.at({0,0})->automatic=false;require(held.setIngredient({0,0},"carrot")&&held.dispense({0,0}),"real measured crop input");held.at({0,0})->item->cropBatch=true;auto id=held.at({0,0})->item->id;double mass=analyse(held.at({0,0})->item->food).mass;require(held.configure({1,0},"mode_forward"),"select physical output");held.at({2,0})->enabled=false;held.step();held.step();require(held.at({1,0})->item&&held.at({1,0})->item->id==id&&held.farmInventory.empty(),"blocked handoff holds the original food");held.at({2,0})->enabled=true;held.step();require(held.at({2,0})->item&&held.at({2,0})->item->id==id&&analyse(held.at({2,0})->item->food).mass==mass,"unblocked output keeps batch identity and mass");
 auto folder=std::filesystem::temp_directory_path()/"aftertaste-farm-delivery-tests";std::filesystem::create_directories(folder);
 Factory old;disconnect(old);Machine wall;wall.kind=MachineKind::Wall;
 for(int x=-8;x<=7;x++){old.machines[{x,-2}]=wall;old.machines[{x,14}]=wall;}for(int y=-1;y<14;y++){old.machines[{-8,y}]=wall;old.machines[{7,y}]=wall;}
 auto zone=old.enclosedFactoryTiles();require(!zone.empty(),"closed zoning fixture");old.storageFile=folder/"v7.json";{std::ofstream out(old.storageFile);out<<json{{"version",7},{"factory",static_cast<FactoryState&>(old)}};}
 Factory upgrade;upgrade.storageFile=old.storageFile;require(upgrade.load(),"load disconnected saved farm");require(upgrade.at({1,-2})->kind==MachineKind::Belt&&upgrade.at({7,-1})->kind==MachineKind::Belt,"straight wall crossings receive physical conveyors");require(upgrade.enclosedFactoryTiles()==zone,"wall conveyor openings preserve factory zoning exactly");require(upgrade.at({0,-10})->forward&&upgrade.at({9,-4})->forward,"saved farm intakes switch to physical delivery");require(upgrade.save(),"save connected factory");Factory again;again.storageFile=old.storageFile;require(again.load()&&json(static_cast<FactoryState&>(again))==json(static_cast<FactoryState&>(upgrade)),"delivery layout and settings survive reload without duplication");
 Factory custom;disconnect(custom);require(custom.build({1,-6},MachineKind::Heater),"custom construction in delivery aisle");auto machines=json(custom.machines);custom.storageFile=folder/"custom-v7.json";{std::ofstream out(custom.storageFile);out<<json{{"version",7},{"factory",static_cast<FactoryState&>(custom)}};}
 Factory preserved;preserved.storageFile=custom.storageFile;require(preserved.load()&&json(preserved.machines)==machines,"automatic routing leaves custom obstructions and the entire layout intact");
 std::filesystem::remove_all(folder);std::cout<<"PASS farm delivery: "<<checks<<" checks; six physical crop streams, original food, blocked outputs, molecular processing, save migration and unchanged zoning\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL farm delivery: "<<e.what()<<'\n';return 1;}}
