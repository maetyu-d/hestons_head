#include "Factory.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace aftertaste;
static int checks=0;
static void require(bool ok,const std::string& message){checks++;if(!ok)throw std::runtime_error(message);}
int main(){try{
 Factory f;require(f.sandbox&&!f.running,"sandbox must start unlocked and paused");
 auto cash=f.cash;for(int n=0;n<70;n++)f.step();
 require(f.shipped>20,"starter factory must ship products");
 std::set<std::string> phases;for(auto& [k,p]:f.products)phases.insert(p.phase);
 for(auto phase:{"pearls","foam","crystal","emulsion"})require(phases.count(phase),std::string("native chemistry failed to produce ")+phase);
 require(f.requestsDone>=1,"optional requests should complete without stopping production");require(f.cash>cash&&f.energy>0&&f.waterUsed>0,"production accounting");
 for(auto& [k,n]:f.stock)require(n==40,"creative supplies should be unlimited");
 Factory deterministic;for(int n=0;n<70;n++)deterministic.step();require(json(static_cast<FactoryState&>(f))==json(static_cast<FactoryState&>(deterministic)),"factory must be deterministic");
 require(f.build({10000,-9000},MachineKind::Heater),"world must expand far beyond the starting factory");
 auto count=f.machines.size();require(f.captureBlueprint({-7,0},{-3,0},"Copied pearls"),"blueprint capture");auto bp=f.blueprints.back();
 require(bp.parts.size()==5&&f.stamp(bp,{300,200}),"blueprint stamp");require(f.machines.size()==count+5,"blueprint building count");require(!f.stamp(bp,{300,200}),"overlapping stamp must be atomic");
 auto rotated=Factory::rotatedBlueprint(bp);require(rotated.width==1&&rotated.height==5,"blueprint rotation dimensions");for(auto& p:rotated.parts)require(p.machine.dir==1,"rotated machine outlets");require(f.stamp(rotated,{-300,200}),"rotated blueprint placement");
 f.undo();require(!f.at({-300,200}),"construction undo");
 Factory jam;jam.machines.clear();jam.build({0,0},MachineKind::Feeder);jam.build({1,0},MachineKind::Belt);for(int n=0;n<20;n++)jam.step();require(jam.batches()==2,"blocked flow must preserve batches");require(jam.at({1,0})->status=="No outlet","backpressure status");
 jam.build({2,0},MachineKind::Dispatch);for(int n=0;n<12;n++)jam.step();require(jam.shipped>0,"repaired line must resume");
 Factory mixing;mixing.machines.clear();mixing.build({0,0},MachineKind::Feeder);mixing.build({1,0},MachineKind::Mixer);mixing.build({2,0},MachineKind::Dispatch);for(int n=0;n<15;n++)mixing.step();require(mixing.shipped>0,"mixer must accept two batches over time");
 Factory economy;economy.toggleEconomy();economy.cash=0;auto size=economy.machines.size();require(!economy.build({50,50},MachineKind::Heater)&&economy.machines.size()==size,"economy build budget");economy.cash=1000;require(economy.buyStock("cream"),"economy purchasing");require(economy.stock["cream"]==50&&economy.cash==960,"purchase accounting");economy.powerCapacity=0;for(int n=0;n<20;n++)economy.step();require(economy.shipped==0,"unpowered processing must not finish");economy.upgradePower();require(economy.powerCapacity==4,"power upgrade");
 for(auto ingredient:std::vector<std::string>{"syrup","broth"}){
  Factory roasted;roasted.machines.clear();roasted.build({0,0},MachineKind::Feeder);roasted.at({0,0})->ingredient=ingredient;
  int shift=0;if(ingredient=="broth"){roasted.build({1,0},MachineKind::Doser);roasted.setIngredient({1,0},"glucose");roasted.at({1,0})->dose=8;shift=1;}
  roasted.build({1+shift,0},MachineKind::Heater);roasted.at({1+shift,0})->duration=ingredient=="broth"?16:10;roasted.at({1+shift,0})->temperature=ingredient=="broth"?130:145;
  roasted.build({2+shift,0},MachineKind::Dryer);roasted.at({2+shift,0})->duration=24;roasted.at({2+shift,0})->temperature=105;roasted.build({3+shift,0},MachineKind::Dispatch);roasted.setRequest(ingredient=="broth"?5:3);
  for(int n=0;n<110;n++)roasted.step();bool fulfilled=false;for(auto& shipment:roasted.shipments)fulfilled|=shipment.requested;
  require(fulfilled,"heat/dehydration must make the optional "+ingredient+" request achievable");
 }
 Factory large;large.machines.clear();auto cell=large.blueprints.front();
 for(int i=0;i<200;i++)require(large.stamp(cell,{i*7,30}),"large district stamp");
 auto started=std::chrono::steady_clock::now();for(int n=0;n<24;n++)large.step();
 require(large.machines.size()==1000&&large.shipped>=400,"thousand-machine district must run real production");
 std::cout<<"1000-machine district: "<<large.shipped<<" shipments in "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<" s of simulation\n";
 Factory chain;chain.machines.clear();chain.build({0,0},MachineKind::Feeder);for(int x=1;x<=300;x++)chain.build({x,0},MachineKind::Belt);chain.build({301,0},MachineKind::Dispatch);
 for(int n=0;n<20;n++)chain.step();require(chain.at({20,0})->item&&chain.at({20,0})->item->id==1,"a batch may move at most one conveyor tile per tick");
 auto folder=std::filesystem::temp_directory_path()/"aftertaste-factory-tests";std::filesystem::create_directories(folder);f.storageFile=folder/"factory.json";require(f.save(),"factory save");Factory loaded;loaded.storageFile=f.storageFile;require(loaded.load(),"factory load");require(json(static_cast<FactoryState&>(f))==json(static_cast<FactoryState&>(loaded))&&!loaded.running,"save must preserve batches, products, plans and material states");
 auto saved=json(static_cast<FactoryState&>(loaded));{std::ifstream in(f.storageFile);json j;in>>j;j["factory"]["machines"][0]["machine"]["dir"]=999;std::ofstream out(f.storageFile);out<<j;}
 require(!loaded.load()&&saved==json(static_cast<FactoryState&>(loaded)),"malformed save must not mutate a running factory");
 std::filesystem::remove_all(folder);
 std::cout<<"PASS factory sandbox: "<<checks<<" checks; four real product phases, optional requests, open world, blueprints, congestion, economy, persistence\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
