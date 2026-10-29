#include "Factory.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace aftertaste;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 auto folder=std::filesystem::temp_directory_path()/"aftertaste-farm-migration";
 std::filesystem::create_directories(folder);
 for(int version:{5,6}){
  Factory old;
  for(auto it=old.machines.begin();it!=old.machines.end();)if(it->first.y<0)it=old.machines.erase(it);else ++it;
  const std::vector<std::pair<Position,MachineKind>> legacy={
   {{10,4},MachineKind::SeedHopper},{{11,4},MachineKind::RaisedBed},{{12,4},MachineKind::Harvester},{{13,4},MachineKind::Belt},{{14,4},MachineKind::Washer},{{15,4},MachineKind::Grader},{{16,4},MachineKind::ColdStore},{{17,4},MachineKind::ProduceIntake},
   {{11,5},MachineKind::HydroponicBed},{{11,3},MachineKind::SoilProbe},{{12,5},MachineKind::GrowLight},{{10,5},MachineKind::PollinatorHive},{{11,6},MachineKind::Fertigation}};
  for(const auto& [pos,kind]:legacy)require(old.build(pos,kind),"legacy farm placement");
  require(old.build({9,3},MachineKind::Wall),"custom factory wall placement");
  auto* bed=old.at({11,4});bed->farmPlanted=true;bed->farmGrowth=.63;bed->farmHealth=.82;
  old.at({14,4})->duration=17;
  old.at({12,4})->item.swap(old.at({-7,6})->item);
  old.at({12,4})->item->route={{11,4},{12,4}};
  old.at({-4,0})->item.swap(old.at({-5,4})->item);
  old.at({-4,0})->item->route={{11,4},{-4,0}};
  old.farmInventory["tomato"]=2.3;
  old.storageFile=folder/("v"+std::to_string(version)+".json");
  {std::ofstream out(old.storageFile);out<<json{{"version",version},{"factory",static_cast<FactoryState&>(old)}};}
  Factory migrated;migrated.storageFile=old.storageFile;require(migrated.load(),"load legacy farm");
  for(const auto& [pos,kind]:legacy)require(!migrated.at(pos),"east farm removed");
  require(migrated.at({9,3})->kind==MachineKind::Wall,"custom factory walls retained");
  require(migrated.at({-6,-10})->farmGrowth==.63&&migrated.at({-6,-10})->farmHealth==.82,"growing crops retained in correct northern bed");
  require(migrated.at({-3,-10})->duration==17,"process settings retained");
  require(migrated.at({-5,-10})->item->route==std::vector<Position>{{-6,-10},{-5,-10}},"farm batch routes relocated once");
  require(migrated.at({-4,0})->item->route==std::vector<Position>{{-6,-10},{-4,0}},"downstream batch provenance relocated");
  require(migrated.farmInventory.at("tomato")==2.3&&migrated.batches()==old.batches(),"inventory and food retained");
  int beds=0;for(const auto& [pos,m]:migrated.machines)if(isFarmBed(m.kind)){require(pos.y<0,"all farm beds north of factory");beds++;}
  require(beds==6,"six northern crop lanes installed");
  require(migrated.save(),"save relocated farm");Factory again;again.storageFile=old.storageFile;require(again.load(),"reload relocated farm");
  require(json(static_cast<FactoryState&>(again))==json(static_cast<FactoryState&>(migrated)),"migration remains stable across reloads");
 }
 std::filesystem::remove_all(folder);std::cout<<"PASS farm migration: version 5 and 6, customized factory, crop and batch preservation, north coordinates, stable reload\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
