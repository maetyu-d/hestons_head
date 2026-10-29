#include "FloorTelemetry.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace aftertaste;
static int checks=0;
static void require(bool ok,const char* message){checks++;if(!ok)throw std::runtime_error(message);}
static bool flow(const FloorTelemetry& t,FarmFlowKind kind,Position from,Position to){for(const auto& event:t.farmFlows)if(event.kind==kind&&event.from==from&&event.to==to)return true;return false;}
static bool change(const FloorTelemetry& t,Position p,const std::string& key){auto i=t.changes.find(p);if(i==t.changes.end())return false;if(i->second.title.find(key)!=std::string::npos)return true;for(const auto& line:i->second.details)if(line.find(key)!=std::string::npos)return true;return false;}
int main(){try{
 Factory f;f.machines.clear();f.blueprints.clear();
 auto add=[&](Position p,MachineKind k,int dir=0){require(f.build(p,k,dir),"farm fixture placement");};
 add({0,-1},MachineKind::SeedHopper,1);add({0,0},MachineKind::RaisedBed);add({1,0},MachineKind::Harvester);add({2,0},MachineKind::Belt);add({3,0},MachineKind::Washer);add({4,0},MachineKind::Grader);add({5,0},MachineKind::ColdStore);add({6,0},MachineKind::ProduceIntake);add({0,1},MachineKind::Fertigation);add({-1,0},MachineKind::PollinatorHive);
 add({10,0},MachineKind::HydroponicBed);f.at({10,0})->ingredient="greens";f.at({10,0})->farmPlanted=true;add({10,1},MachineKind::GrowLight);
 FloorTelemetry t;double time=0;auto step=[&]{t.observe(f,time);f.step();time++;t.observe(f,time);};t.observe(f,time);
 auto state=json(static_cast<const FactoryState&>(f));t.observe(f,time+.01);require(state==json(static_cast<const FactoryState&>(f)),"farm observation never mutates the simulation");
 step();require(flow(t,FarmFlowKind::Seed,{0,-1},{0,0}),"actual planting shows seed-to-bed flow");require(flow(t,FarmFlowKind::Nutrients,{0,1},{0,0}),"actual fertigation shows nutrient-to-bed flow");require(flow(t,FarmFlowKind::Pollination,{-1,0},{0,0}),"fruit pollination is visible");require(flow(t,FarmFlowKind::Light,{10,1},{10,0}),"powered growing light is visible");
 auto seeds=f.seedStock.at("tomato");t.observe(f,time+.1);require(f.seedStock.at("tomato")==seeds,"rendering seed flow never spends another seed");
 f.at({0,0})->farmGrowth=.999999;t.observe(f,time);step();
 require(flow(t,FarmFlowKind::Harvest,{0,0},{1,0}),"a crop ripening this tick visibly enters its harvester");require(f.at({2,0})->item.has_value(),"harvest creates a real batch on the adjacent belt");auto id=f.at({2,0})->item->id;double mass=analyse(f.at({2,0})->item->food).mass;
 require(t.motions.size()==1&&t.motions.front().from==Position{1,0}&&t.motions.front().to==Position{2,0},"new harvest batch animates its real output edge");
 const auto& sample=t.samples.at(id);require(sample.material.cropIntact&&sample.material.cropDirt>0&&sample.material.cropFreshness<=1,"raw crop appearance carries measured dirt and freshness");require(std::binary_search(sample.route.begin(),sample.route.end(),Position{0,0}),"harvest retains its real grow-bed provenance");
 bool washed=false,chilled=false,stored=false,soilChange=false;for(int n=0;n<22;n++){step();auto s=t.samples.find(id);if(s!=t.samples.end()){washed|=s->second.material.cropDirt==0;chilled|=s->second.material.metrics.temperature<=8;require(s->second.material.cropIntact,"farm handling preserves raw crop visuals");}soilChange|=change(t,{3,0},"SOIL");stored|=flow(t,FarmFlowKind::Storage,{6,0},{6,0});}
 require(washed&&soilChange,"washing reports measured soil removal");require(chilled,"cold-store appearance follows actual temperature");require(stored&&change(t,{6,0},"STORED"),"intake shows an actual inventory handoff");require(std::abs(f.farmInventory.at("tomato")*1000-mass)<.01,"stored crop mass matches the harvested batch");require(!t.samples.count(id)&&f.at({6,0})->lastOutput&&f.at({6,0})->lastOutput->id==id,"stored food is a record, not phantom floor inventory");
 f.storageFile=std::filesystem::temp_directory_path()/"aftertaste-farm-flow-save"/"factory.json";require(f.save(),"save farm inventory and intake receipt");Factory restored;restored.storageFile=f.storageFile;require(restored.load()&&restored.at({6,0})->lastOutput->id==id&&restored.batches()==f.batches(),"intake receipt survives save and reload without recreating food");std::filesystem::remove(f.storageFile);
 f.at({10,1})->enabled=false;f.at({-1,0})->enabled=false;f.at({0,1})->enabled=false;step();require(!flow(t,FarmFlowKind::Light,{10,1},{10,0})&&!flow(t,FarmFlowKind::Pollination,{-1,0},{0,0})&&!flow(t,FarmFlowKind::Nutrients,{0,1},{0,0}),"disabled farm equipment produces no active effects");
 t.observe(f,time+1);require(t.farmFlows.empty(),"farm pulses expire while paused");
 f.at({0,0})->farmPlanted=true;f.at({0,0})->farmGrowth=1;f.at({2,0})->enabled=false;step();require(f.at({1,0})->item.has_value()&&t.motions.empty(),"blocked harvest remains in its machine without an invented conveyor move");
 f.at({2,0})->enabled=true;f.at({3,0})->kind=MachineKind::Heater;bool molecular=false;for(int n=0;n<5;n++){step();for(const auto& [id,sample]:t.samples)if(sample.material.cropBatch&&!sample.material.cropIntact)molecular=true;}require(molecular,"crop material switches to molecular visuals when it enters cooking equipment");
 f.reset();t.observe(f,time+2);require(t.farmFlows.empty(),"world reset removes old farm activity");
 std::cout<<"PASS farm flow: "<<checks<<" checks; real seed, nutrient, harvest, wash, chill and storage activity, blocks, pause and mass preservation\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL farm flow: "<<e.what()<<'\n';return 1;}}
