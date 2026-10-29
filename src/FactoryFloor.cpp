#include "Factory.h"
#include <algorithm>
#include <stdexcept>

namespace aftertaste {
Blueprint Factory::experimentBlueprint(){
 Blueprint b;b.name="Experiment cell";b.width=6;b.height=3;
 auto add=[&](Position p,MachineKind k,int dir=0,const std::string& key="brine",double grams=100){Machine m;m.kind=k;m.dir=dir;m.ingredient=key;m.dose=grams;m.duration=std::max(1,machineSpec(k).duration);m.temperature=k==MachineKind::LatticeReactor?120:70;if(k==MachineKind::Feeder||isTastingDock(k))m.automatic=false;if(k==MachineKind::GlassBeaker)m.serveStyle=ServeStyle::ResearchTasting;if(k==MachineKind::FractionReceiver)m.serveStyle=ServeStyle::ChilledPickup;b.parts.push_back({p,m});};
 add({0,2},MachineKind::Feeder,0,"tomato",100);add({1,2},MachineKind::Belt);add({2,0},MachineKind::Feeder,1,"oil",30);add({2,1},MachineKind::Belt,1);add({2,2},MachineKind::Mixer);add({3,2},MachineKind::Doser,0,"lecithin",1.5);add({4,2},MachineKind::LatticeReactor);add({5,2},MachineKind::Dispatch);return b;
}
Blueprint Factory::glassLabBlueprint(){
 Blueprint b;b.name="Glass lab apparatus";b.width=9;b.height=5;
 auto add=[&](Position p,MachineKind k,int dir=0,const std::string& key="brine",double dose=100){Machine m;m.kind=k;m.dir=dir;m.ingredient=key;m.dose=dose;m.duration=std::max(1,machineSpec(k).duration);m.temperature=k==MachineKind::GlassCondenser?4:70;if(k==MachineKind::Feeder||isTastingDock(k))m.automatic=false;if(k==MachineKind::GlassBeaker)m.serveStyle=ServeStyle::ResearchTasting;if(k==MachineKind::FractionReceiver)m.serveStyle=ServeStyle::ChilledPickup;b.parts.push_back({p,m});};
 // Equal-length feed branches preserve the measured ingredient ratio during
 // automatic production, while providing real bends and glass joints to inspect.
 add({0,0},MachineKind::Feeder,1,"tomato",100);
 for(auto step:std::vector<std::pair<Position,int>>{{{0,1},0},{{1,1},1},{{1,2},2},{{0,2},1},{{0,3},1},{{0,4},0},{{1,4},0}})add(step.first,MachineKind::GlassTube,step.second);
 add({4,0},MachineKind::Feeder,1,"oil",30);
 for(auto step:std::vector<std::pair<Position,int>>{{{4,1},2},{{3,1},2},{{2,1},1},{{2,2},0},{{3,2},1},{{3,3},2},{{2,3},1}})add(step.first,MachineKind::GlassTube,step.second);
 add({2,4},MachineKind::GlassJunction);add({3,4},MachineKind::Burette,0,"lecithin",1.5);
 add({4,4},MachineKind::GlassFlask);add({5,4},MachineKind::GlassCondenser);add({6,4},MachineKind::GlassCoil);add({7,4},MachineKind::GlassValve);add({8,4},MachineKind::GlassBeaker);
 return b;
}
Blueprint Factory::fractionalDistillationBlueprint(){
 Blueprint b;b.name="Fractional distillation train";b.width=12;b.height=4;
 auto add=[&](Position p,MachineKind k,int dir=0,const std::string& key="brine",double dose=100){Machine m;m.kind=k;m.dir=dir;m.ingredient=key;m.dose=dose;m.duration=std::max(1,machineSpec(k).duration);m.temperature=k==MachineKind::GlassCondenser?4:70;if(k==MachineKind::FractionatingStill)m.temperature=86;if(k==MachineKind::FractionatingColumn)m.temperature=86;if(k==MachineKind::Feeder||isTastingDock(k))m.automatic=false;if(k==MachineKind::GlassBeaker)m.serveStyle=ServeStyle::ResearchTasting;if(k==MachineKind::FractionReceiver)m.serveStyle=ServeStyle::ChilledPickup;b.parts.push_back({p,m});return &b.parts.back().machine;};
 // A single measured feed passes through three distinct cuts. Each still's
 // clockwise vapour branch enters a real reflux column; column reflux returns
 // to the boiler while enriched vapour runs through a condenser to its own cup.
 add({1,0},MachineKind::Feeder,0,"citrus",100);
 auto* heads=add({2,0},MachineKind::FractionatingStill,0);heads->cut=0;heads->temperature=78;
 add({3,0},MachineKind::GlassTube,0);add({4,0},MachineKind::GlassTube,0);add({5,0},MachineKind::GlassTube,0);
 auto* hearts=add({6,0},MachineKind::FractionatingStill,0);hearts->cut=1;hearts->temperature=86;
 add({7,0},MachineKind::GlassTube,0);add({8,0},MachineKind::GlassTube,0);add({9,0},MachineKind::GlassTube,0);
 auto* tails=add({10,0},MachineKind::FractionatingStill,0);tails->cut=2;tails->temperature=96;
 add({11,0},MachineKind::FractionReceiver,0);
 auto* columnA=add({2,1},MachineKind::FractionatingColumn,2);columnA->stages=8;columnA->temperature=78;
 add({1,1},MachineKind::GlassCondenser,2);add({0,1},MachineKind::FractionReceiver,0);
 auto* columnB=add({6,1},MachineKind::FractionatingColumn,2);columnB->stages=12;columnB->temperature=86;
 add({5,1},MachineKind::GlassTube,2);add({4,1},MachineKind::GlassTube,2);add({3,1},MachineKind::GlassTube,1);add({3,2},MachineKind::GlassCondenser,1);add({3,3},MachineKind::FractionReceiver,0);
 auto* columnC=add({10,1},MachineKind::FractionatingColumn,2);columnC->stages=16;columnC->temperature=96;
 add({9,1},MachineKind::GlassTube,2);add({8,1},MachineKind::GlassTube,2);add({7,1},MachineKind::GlassTube,1);add({7,2},MachineKind::GlassCondenser,1);add({7,3},MachineKind::FractionReceiver,0);
 return b;
}
Blueprint Factory::marketGardenBlueprint(){
 Blueprint b;b.name="Northern market farm / crop rotation";b.width=17;b.height=10;
 auto add=[&](Position p,MachineKind kind,int dir=0,const std::string& crop="tomato"){Machine m;m.kind=kind;m.dir=dir;m.ingredient=crop;m.duration=std::max(1,machineSpec(kind).duration);m.temperature=(kind==MachineKind::FieldBed||kind==MachineKind::RaisedBed||kind==MachineKind::HydroponicBed||kind==MachineKind::SoilProbe)?20:kind==MachineKind::ColdStore?4:70;m.automatic=kind==MachineKind::SeedHopper;if(kind==MachineKind::HydroponicBed){m.farmMoisture=.76;m.farmOrganic=.12;m.farmN=m.farmP=m.farmK=.62;}b.parts.push_back({p,m});};
 // Six north-facing crop blocks give the starter district a real field depth:
 // fruit, leafy greens, roots, peppers, culinary herbs and shade crops each
 // have their own measured seed-to-intake lane and seasonal growing conditions.
 struct CropBlock {const char* crop;MachineKind bed;MachineKind westSupport;MachineKind southSupport;};
 const std::array<CropBlock,6> blocks={{{"tomato",MachineKind::RaisedBed,MachineKind::PollinatorHive,MachineKind::Fertigation},
  {"greens",MachineKind::HydroponicBed,MachineKind::SoilProbe,MachineKind::GrowLight},
  {"carrot",MachineKind::FieldBed,MachineKind::SoilProbe,MachineKind::Fertigation},
  {"pepper",MachineKind::RaisedBed,MachineKind::PollinatorHive,MachineKind::GrowLight},
  {"basil",MachineKind::HydroponicBed,MachineKind::SoilProbe,MachineKind::Fertigation},
  {"mushroom",MachineKind::FieldBed,MachineKind::SoilProbe,MachineKind::Fertigation}}};
 for(size_t i=0;i<blocks.size();i++){
  int x=int(i%2)*9,y=2+int(i/2)*3;const auto& crop=blocks[i];
  add({x+1,y-1},MachineKind::SeedHopper,1,crop.crop);add({x+1,y},crop.bed,0,crop.crop);
  add({x+2,y},MachineKind::Harvester,0);add({x+3,y},MachineKind::Belt,0);add({x+4,y},MachineKind::Washer,0);
  add({x+5,y},MachineKind::Grader,0);add({x+6,y},MachineKind::ColdStore,0);add({x+7,y},MachineKind::ProduceIntake,0);
  add({x,y},crop.westSupport,0,crop.crop);add({x+1,y+1},crop.southSupport,0,crop.crop);
 }
 return b;
}
const Batch* Factory::batchAt(Position p,int slot)const{
 auto m=at(p);if(!m)return nullptr;
 if(slot==1)return m->extra?&*m->extra:nullptr;if(slot==2)return m->sideOutput?&*m->sideOutput:nullptr;if(slot==3)return m->lastOutput?&*m->lastOutput:nullptr;
 if(m->item)return &*m->item;return (isTastingDock(m->kind)||m->kind==MachineKind::ProduceIntake)&&m->lastOutput?&*m->lastOutput:nullptr;
}
bool Factory::dispense(Position p){
 auto m=at(p);if(!m||m->kind!=MachineKind::Feeder||m->item){notice="Choose an empty ingredient feeder to dispense one measured batch.";return false;}
 if(!sandbox&&stock[m->ingredient]+1e-10<m->dose/100){notice="The feeder needs more "+ingredient(m->ingredient).name+".";return false;}
 if(auto missing=missingInputSupply(m->ingredient,m->dose);!missing.empty()){notice="Factory input empty: "+missing+". Replenish it in the inspector before dispensing.";return false;}
 snapshot();m->item=makeBatch(m->ingredient,m->dose);m->item->route={p};if(!sandbox)stock[m->ingredient]=std::max(0.,stock[m->ingredient]-m->dose/100);consumeInputSupply(m->ingredient,m->dose);m->ready=true;m->cooldown=4;m->status="One measured batch loaded";dirty=true;notice="Loaded "+roundTo(m->dose)+" g "+ingredient(m->ingredient).name+" into the feeder. Run or Step to move it through the real machines.";return true;
}
bool Factory::serveAt(Position p){auto m=at(p);if(!m||!isTastingDock(m->kind)||!m->item)return false;if(enclosedFactoryTiles().count(p)){m->status="Serve point must be outside factory walls";notice="Serve points must be outside factory areas. Move this dock beyond the walls or into a wall opening.";return false;}snapshot();ship(*m,p);dirty=true;return !m->item;}
bool Factory::sendOnward(Position p){auto m=at(p);if(!m||!isTastingDock(m->kind)||!m->item||!at(nextPosition(p,m->dir))){notice="Connect another machine at this vessel's outlet before sending the batch onward.";return false;}if(enclosedFactoryTiles().count(p)){m->status="Serve point must be outside factory walls";notice="This serve point is inside a factory area. Move it beyond the walls or into a wall opening before sending or serving.";return false;}snapshot();m->automatic=false;m->forward=false;m->ready=true;dirty=true;notice="Batch queued for the next simulation tick.";return true;}
bool Factory::discardBatch(Position p,int slot){auto m=at(p);if(!m||slot==3)return false;std::optional<Batch>* b=slot==1?&m->extra:slot==2?&m->sideOutput:&m->item;if(!*b)return false;snapshot();b->reset();wasted++;if(slot==0){m->progress=0;m->ready=false;m->program.clear();m->programOutput.clear();m->cursor=m->subtick=0;if(m->kind==MachineKind::LatticeReactor){m->lattice.clear();m->latticeBatch=0;m->latticePending=-1;m->latticeMoves=0;m->latticeRelease=false;}}dirty=true;notice="Selected batch reclaimed as waste. Ingredient stocks were not refunded.";return true;}
uint64_t Factory::recordDish(Position p,const std::string& name,int slot,uint64_t expectedBatch){
 auto m=at(p);auto b=batchAt(p,slot);if(!m||!isTastingDock(m->kind)||!b||b->food.parts.empty()||dishbook.size()>=500){notice="Bring a finished batch to a tasting dock or receiving beaker before recording a dish.";return 0;}
 if(enclosedFactoryTiles().count(p)){notice="This tasting dock is inside a factory area. Move it beyond the walls or into a wall opening before recording a dish.";return 0;}
 if(expectedBatch&&b->id!=expectedBatch){notice="The selected serving changed. Pause or hold the vessel, inspect the batch, then record it.";return 0;}
 if(b->food.history.empty()||!validCulinary(b->food))return 0;
 CulinaryBatch replay;for(auto& op:b->food.history)if(!executeStep(replay,op)||replay.parts.size()>16){notice="This batch has an incomplete process record.";return 0;}
 snapshot();DishDesign d;d.id=nextDish++;d.name=name.empty()?"Dish "+std::to_string(d.id):name.substr(0,64);d.sample=b->food;for(auto& part:b->food.parts)d.output.push_back(part.id);d.quality=analyse(b->food).quality;
 std::set<Position> nodes(b->route.begin(),b->route.end());nodes.insert(p);std::vector<Position> sidePaths;bool complete=true,hasFeeder=false;
 for(auto pos:nodes){auto source=at(pos);if(!source){complete=false;continue;}if(source->kind==MachineKind::RecipeKitchen)complete=false;if(source->kind==MachineKind::Feeder)hasFeeder=true;if(hasSideOutlet(source->kind))sidePaths.push_back(nextPosition(pos,(source->dir+1)%4));}
 // Keep actual by-product routing with the contributing line; construction never copies food.
 while(!sidePaths.empty()&&nodes.size()<100000){auto pos=sidePaths.back();sidePaths.pop_back();auto source=at(pos);if(!source||!nodes.insert(pos).second)continue;if(source->kind==MachineKind::RecipeKitchen){complete=false;continue;}if(source->kind==MachineKind::Bin||(isTastingDock(source->kind)&&!source->forward))continue;sidePaths.push_back(nextPosition(pos,source->dir));if(hasSideOutlet(source->kind))sidePaths.push_back(nextPosition(pos,(source->dir+1)%4));}
 if(complete&&hasFeeder&&!nodes.empty()){
  int minx=1000001,miny=1000001,maxx=-1000001,maxy=-1000001;for(auto pos:nodes){minx=std::min(minx,pos.x);miny=std::min(miny,pos.y);maxx=std::max(maxx,pos.x);maxy=std::max(maxy,pos.y);}
  if(maxx-minx<=1000&&maxy-miny<=1000){Blueprint layout;layout.name=d.name;layout.width=maxx-minx+1;layout.height=maxy-miny+1;
   for(auto pos:nodes){auto source=at(pos);if(!source)continue;auto copy=*source;copy.item.reset();copy.extra.reset();copy.sideOutput.reset();copy.lastOutput.reset();copy.program.clear();copy.programOutput.clear();copy.cursor=copy.subtick=copy.progress=copy.cooldown=0;copy.ready=false;copy.wear=0;copy.status="Idle";if(isTastingDock(copy.kind)){copy.recipe=pos==p?d.id:0;copy.forward=pos!=p;copy.automatic=false;}layout.parts.push_back({{pos.x-minx,pos.y-miny},copy});}
   dishLayouts[d.id]=std::move(layout);
  }
 }
 m->recipe=d.id;if(m->item&&m->item->id==b->id)m->item->dish=d.id;if(m->lastOutput&&m->lastOutput->id==b->id)m->lastOutput->dish=d.id;dishbook.push_back(d);dirty=true;
 notice=dishLayouts.count(d.id)?"Recorded "+d.name+" from a real batch. Build line copies its actual machines; no finished food is copied.":"Recorded "+d.name+" as a reference. Its complete physical line is unavailable; recreate the stages on the floor.";return d.id;
}
std::optional<Blueprint> Factory::productionLine(uint64_t id)const{
 auto it=dishLayouts.find(id);if(it==dishLayouts.end())return {};auto copy=it->second;
 for(auto& part:copy.parts){auto& m=part.machine;if(m.kind==MachineKind::RecipeKitchen)return {};if(m.kind==MachineKind::Feeder)m.automatic=true;if(isTastingDock(m.kind)&&m.recipe==id){m.automatic=true;m.forward=false;}}
 return copy;
}
} // namespace aftertaste
