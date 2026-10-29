#include "FloorTelemetry.h"
#include <iostream>
#include <stdexcept>
using namespace aftertaste;
static int checks=0;
static void require(bool ok,const std::string& why){checks++;if(!ok)throw std::runtime_error(why);}
static bool contains(const FloorChange& c,const std::string& text){if(c.title.find(text)!=std::string::npos)return true;for(auto& l:c.details)if(l.find(text)!=std::string::npos)return true;return false;}
static void step(Factory& f,FloorTelemetry& t,double& time){time+=1;t.observe(f,time);f.step();t.observe(f,time);}
static void line(Factory& f,MachineKind process,const std::string& key){f.machines.clear();f.blueprints.clear();f.build({0,0},MachineKind::Feeder);f.at({0,0})->automatic=false;f.setIngredient({0,0},key);f.build({1,0},process);f.build({2,0},MachineKind::Dispatch);f.at({2,0})->automatic=false;}
int main(){try{
 Factory f;FloorTelemetry t;double time=0;line(f,MachineKind::Belt,"tomato");f.dispense({0,0});t.observe(f,time);auto id=f.at({0,0})->item->id;
 auto saved=json(static_cast<const FactoryState&>(f));for(int n=0;n<5;n++)t.observe(f,time+.01*n);require(saved==json(static_cast<const FactoryState&>(f)),"drawing telemetry cannot change material, clock, IDs, economy or saves");
 require(t.samples.size()==1&&std::abs(t.samples.at(id).material.metrics.mass-100)<1e-8,"visible material matches actual feeder inventory");
 step(f,t,time);require(t.motions.size()==1&&t.motions.front().batch==id&&t.motions.front().from==Position{0,0}&&t.motions.front().to==Position{1,0},"one actual feeder-to-belt move");require(t.moving(id,time)&&!t.moving(id,time+1),"movement has a finite lifetime");
 step(f,t,time);require(t.motions.size()==1&&t.motions.front().to==Position{2,0},"one tile per tick, no imaginary downstream motion");step(f,t,time);require(t.motions.empty()&&f.at({2,0})->item.has_value(),"holding dock food stays still");
 require(f.serveAt({2,0}),"serve real held material");t.observe(f,time);require(t.samples.empty()&&contains(t.changes.at({2,0}),"SERVED"),"last serving is a record, not active floor food");
 f.dispense({0,0});t.observe(f,time);step(f,t,time);step(f,t,time);require(!t.motions.empty(),"new actual transfer is in flight visually");f.serveAt({2,0});t.observe(f,time);require(t.motions.empty()&&t.samples.empty(),"serving removes an unfinished transfer trace immediately, without phantom food");
 line(f,MachineKind::Belt,"cream");t.clear();f.dispense({0,0});f.at({1,0})->enabled=false;t.observe(f,time);step(f,t,time);require(t.motions.empty()&&f.at({0,0})->status=="Backed up","blocked outlet emits no movement");f.at({0,0})->enabled=false;step(f,t,time);require(t.motions.empty(),"disabled equipment does not show active transport");
 // Real two-input experiment, including a doser and relative droplet reduction.
 f.machines.clear();f.stamp(Factory::experimentBlueprint(),{0,0});t.clear();f.dispense({0,2});f.dispense({2,0});t.observe(f,time);bool mix=false,dose=false,drop=false;double enteringDroplet=0;
 for(int n=0;n<18;n++){step(f,t,time);auto hg=f.at({4,2});if(hg->item&&hg->progress==0)enteringDroplet=t.samples.at(hg->item->id).material.droplet;auto a=t.changes.find({2,2});if(a!=t.changes.end()&&(contains(a->second,"MIX 100.0 + 30.0 g")||contains(a->second,"MIX 30.0 + 100.0 g")))mix=true;auto b=t.changes.find({3,2});if(b!=t.changes.end()&&contains(b->second,"DOSE +1.50 g LE"))dose=true;auto c=t.changes.find({4,2});if(c!=t.changes.end()&&contains(c->second,"DROPLETS"))drop=true;for(auto& m:t.motions)require(std::abs(m.from.x-m.to.x)+std::abs(m.from.y-m.to.y)==1,"no transfer skips floor tiles");}
 require(mix,"mixer shows actual measured input masses");require(dose,"doser shows the actual ingredient and mass added");require(drop,"homogeniser reports actual relative droplet reduction");require(enteringDroplet>0&&contains(t.changes.at({4,2}),"DROPLETS "+roundTo(enteringDroplet,0)+" →"),"readout compares machine entry with its completed output, across all dwell ticks");auto result=f.at({5,2})->item;require(result&&std::abs(t.samples.at(result->id).material.metrics.mass-131.5)<1e-8,"floor food remains the actual 131.5 gram result");
 // Thermal history and structure, rather than a canned machine animation.
 line(f,MachineKind::Heater,"eggwhite");t.clear();f.at({1,0})->temperature=95;f.at({1,0})->duration=10;f.dispense({0,0});t.observe(f,time);bool heat=false,protein=false;for(int n=0;n<13;n++){step(f,t,time);auto c=t.changes.find({1,0});if(c!=t.changes.end()){heat|=contains(c->second,"HEAT");protein|=contains(c->second,"PROTEIN");}}
 require(heat&&protein,"thermal effects show real food temperature and protein changes");
 line(f,MachineKind::Chiller,"cream");t.clear();f.at({1,0})->duration=8;f.dispense({0,0});t.observe(f,time);bool ice=false;for(int n=0;n<11;n++){step(f,t,time);auto c=t.changes.find({1,0});if(c!=t.changes.end())ice|=contains(c->second,"ICE");}require(ice,"ice glyphs and measured changes follow actual freezing");
 line(f,MachineKind::Centrifuge,"cream");t.clear();f.at({1,0})->duration=2;f.build({1,1},MachineKind::Dispatch);f.at({1,1})->automatic=false;f.dispense({0,0});t.observe(f,time);bool side=false;for(int n=0;n<5;n++){step(f,t,time);for(auto& m:t.motions)side|=m.from==Position{1,0}&&m.to==Position{1,1};}require(side,"a newly created fraction moves along the actual clockwise side outlet");require(t.samples.size()==2,"both real separated fractions remain visible");
 saved=json(static_cast<const FactoryState&>(f));t.observe(f,time+.2);require(saved==json(static_cast<const FactoryState&>(f)),"telemetry is read-only after separation");
 line(f,MachineKind::Centrifuge,"cream");t.clear();f.at({1,0})->duration=2;f.dispense({0,0});t.observe(f,time);for(int n=0;n<4;n++)step(f,t,time);require(f.at({1,0})->sideOutput&&t.samples.size()==2,"blocked side fraction remains real inventory on the floor");require(contains(t.changes.at({1,0}),"SEPARATE")&&contains(t.changes.at({1,0}),"MAIN "),"separation reports both measured fractions even with a blocked side outlet");require(t.motions.empty(),"a blocked side fraction never animates through an absent outlet");
 f.reset();t.observe(f,time+1);require(t.samples.empty()&&t.motions.empty()&&t.changes.empty(),"clock rewind resets all previous-world visual state");
 // Air is a fraction of total volume, while plated components remain distinct.
 CulinaryBatch b;addIngredient(b,"cream",80,1,1);b.parts.front().food.bubbles=1.5;auto v=materialView(b);require(std::abs(v.air-.6)<1e-10,"air fraction is bounded, not 150 percent");addIngredient(b,"citrus",20,2,2);v=materialView(b);require(v.components.size()==2&&v.ingredients.size()==2&&std::abs(v.metrics.mass-100)<1e-8,"plated structures and ingredient colours come from real separate components");
 std::cout<<"PASS visible floor flow: "<<checks<<" checks; read-only material, measured changes, actual transfers, blocks, side fractions and archives\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL visible floor flow: "<<e.what()<<'\n';return 1;}}
