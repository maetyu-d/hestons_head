#include "FloorTelemetry.h"
#include <algorithm>
#include <cmath>

namespace aftertaste {
MaterialView materialView(const CulinaryBatch& batch){
 MaterialView v;v.metrics=analyse(batch);v.vapor=v.metrics.vapor;v.fraction=v.metrics.fraction;v.droplet=v.particle=0;double mass=std::max(.000001,v.metrics.mass);
 for(const auto& p:batch.parts){const auto& f=p.food;double w=foodMass(f)/mass;
  v.droplet+=w*f.droplet;v.particle+=w*f.particle;v.air+=w*f.bubbles/(1+f.bubbles);v.shell+=w*f.shell;v.denatured+=w*f.denatured;v.links+=w*f.boundCalcium;v.starch+=w*f.starchCooked;
  for(int i=0;i<CompoundCount;i++)v.compounds[i]+=f.c[i];for(auto& [k,g]:f.ingredients)v.ingredients[k]+=g;
  for(auto& e:f.events)if(std::find(v.events.begin(),v.events.end(),e)==v.events.end())v.events.push_back(e);
  v.components.push_back({foodMass(f),f.phase});
 }return v;
}
MaterialView materialView(const Batch& batch){auto v=materialView(batch.food);v.cropBatch=v.cropIntact=batch.cropBatch;v.cropDirt=batch.cropDirt;v.cropFreshness=batch.cropFreshness;v.cropQuality=batch.cropQuality;return v;}
namespace {
std::vector<std::string> differences(const MaterialView& a,const MaterialView& b){
 std::vector<std::string> lines;
 auto delta=[&](double x,double y,double threshold,const std::string& label,const std::string& suffix,int precision=0){if(std::abs(y-x)>=threshold)lines.push_back(label+" "+roundTo(x,precision)+" → "+roundTo(y,precision)+suffix);};
 if(b.cropBatch){delta(a.cropDirt*100,b.cropDirt*100,.5,"SOIL","%");delta(a.cropFreshness*100,b.cropFreshness*100,.5,"FRESHNESS","%");delta(a.cropQuality*100,b.cropQuality*100,.1,"CROP QUALITY","%");}
 delta(a.metrics.temperature,b.metrics.temperature,.3,"HEAT"," C");
 if(a.vapor!=b.vapor)lines.push_back(b.vapor?"VAPOUR FORMED / "+upper(b.fraction):"CONDENSED / "+upper(b.fraction));
 else if(a.fraction!=b.fraction&&!b.fraction.empty())lines.push_back("FRACTION / "+upper(b.fraction));
 delta(a.droplet,b.droplet,.8,"DROPLETS"," size");
 delta(a.air*100,b.air*100,1,"AIR","%");
 delta(a.metrics.gel*100,b.metrics.gel*100,2,"GEL","%");
 delta(a.shell*100,b.shell*100,2,"SHELL","%");
 delta(a.metrics.ice*100,b.metrics.ice*100,1,"ICE","%");
 delta(a.metrics.pH,b.metrics.pH,.025,"pH","",2);
 delta(a.denatured*100,b.denatured*100,2,"PROTEIN SET","%");
 delta(a.starch*100,b.starch*100,2,"STARCH COOKED","%");
 delta(a.metrics.brown*100,b.metrics.brown*100,1,"BROWNING","%");
 delta(a.particle,b.particle,2,"PARTICLES"," size");
 if(a.compounds[Water]-b.compounds[Water]>.02)lines.push_back("WATER −"+roundTo(a.compounds[Water]-b.compounds[Water],2)+" g");
 if(b.compounds[Amino]-a.compounds[Amino]>.01)lines.push_back("AMINO ACIDS +"+roundTo(b.compounds[Amino]-a.compounds[Amino],2)+" g");
 if(b.compounds[Ethanol]-a.compounds[Ethanol]>.01)lines.push_back("ALCOHOL +"+roundTo(b.compounds[Ethanol]-a.compounds[Ethanol],2)+" g");
 if(b.compounds[Reducing]-a.compounds[Reducing]>.02)lines.push_back("SIMPLE SUGARS +"+roundTo(b.compounds[Reducing]-a.compounds[Reducing],2)+" g");
 for(const auto& e:b.events)if(std::find(a.events.begin(),a.events.end(),e)==a.events.end())lines.insert(lines.begin(),e);
 return lines;
}
uint64_t revision(const Factory& f){uint64_t h=1469598103934665603ull;auto mix=[&](uint64_t n){h=(h^n)*1099511628211ull;};mix(f.tick);mix(f.nextId);mix(f.shipped);mix(f.wasted);for(auto& [p,m]:f.machines){mix(p.x);mix(p.y);mix(int(m.kind));mix(m.progress);mix(m.enabled);mix(m.farmPlanted);mix(m.farmAge);mix(uint64_t(m.farmGrowth*1000000));mix(uint64_t(m.farmMoisture*1000000));for(char c:m.ingredient)mix(c);for(auto b:{&m.item,&m.extra,&m.sideOutput}){mix(*b?(*b)->id:0);mix(*b?(*b)->age:0);}}return h;}
}
void FloorTelemetry::clear(){samples.clear();changes.clear();motions.clear();farmReadings.clear();farmFlows.clear();inputs.clear();entries.clear();tick=-1;fingerprint=0;}
bool FloorTelemetry::moving(uint64_t id,double now)const{for(const auto& m:motions)if(m.batch==id&&now<m.start+m.duration)return true;return false;}
void FloorTelemetry::observe(const Factory& f,double now,double duration){
 motions.erase(std::remove_if(motions.begin(),motions.end(),[&](const FloorMotion& m){return now>=m.start+m.duration;}),motions.end());
 farmFlows.erase(std::remove_if(farmFlows.begin(),farmFlows.end(),[&](const FarmFlow& flow){return now>=flow.start+flow.duration;}),farmFlows.end());
 auto hash=revision(f);if(hash==fingerprint&&tick==f.tick)return;
 if(tick>f.tick||f.tick>tick+1)clear();bool advanced=tick>=0&&f.tick==tick+1;
 // Catch-up and manual stepping may advance several ticks inside one animation.
 // Keep only movements from the newest physical tick, so processed food stays
 // visible in its current equipment instead of an outdated transfer trace.
 if(advanced){motions.clear();farmFlows.clear();}
 std::map<Position,FarmReading> nextFarm;
 auto farmFlow=[&](Position from,Position to,FarmFlowKind kind,const std::string& crop,double amount=0){farmFlows.push_back({from,to,kind,crop,amount,now,duration});};
 std::map<uint64_t,FloorSample> next;std::map<Position,std::pair<uint64_t,uint64_t>> nextInputs;
 for(auto& [p,m]:f.machines){
  if(isFarmEquipment(m.kind))nextFarm.emplace(p,FarmReading{m.kind,m.ingredient,m.status,m.farmPlanted,m.enabled,m.farmGrowth,m.farmMoisture,m.farmHealth,m.farmN+m.farmP+m.farmK,m.cooldown});
  nextInputs[p]={m.item?m.item->id:0,m.extra?m.extra->id:0};int slot=0;
  for(auto b:{&m.item,&m.extra,&m.sideOutput}){if(*b){
   FloorSample sample{p,slot,materialView(**b),(*b)->route,0,0,{}};
   if(sample.material.cropIntact)for(auto stop:(*b)->route){auto device=f.at(stop);if(device&&isProcessingMachine(device->kind)&&!isFarmEquipment(device->kind)){sample.material.cropIntact=false;break;}}
   for(auto& part:(*b)->food.parts)sample.parts.push_back(part.id);
   for(auto op=(*b)->food.history.rbegin();op!=(*b)->food.history.rend();++op)if(op->action==RecipeAction::Split){sample.split=op->id;sample.sidePart=op->side;break;}
   next.emplace((*b)->id,std::move(sample));
  }slot++;}
 }
 // Farm support effects are observed only after the simulation performed them.
 // The links remain visible while idle, but activity never comes from a UI timer.
 if(advanced)for(const auto& [p,reading]:nextFarm){auto prior=farmReadings.find(p);if(prior==farmReadings.end())continue;auto m=f.at(p);if(!m||!reading.enabled)continue;
  if(isFarmBed(reading.kind)&&reading.planted){
   if(!prior->second.planted)for(int d=0;d<4;d++){Position q=nextPosition(p,d);auto source=f.at(q);if(source&&source->kind==MachineKind::SeedHopper&&source->enabled&&source->automatic&&nextPosition(q,source->dir)==p&&source->status.rfind("Planted ",0)==0)farmFlow(q,p,FarmFlowKind::Seed,reading.crop,1);}
  }
  for(int d=0;d<4;d++){Position q=nextPosition(p,d);auto bed=nextFarm.find(q);if(bed==nextFarm.end()||!isFarmBed(bed->second.kind)||!bed->second.planted||!bed->second.enabled)continue;
   if(reading.kind==MachineKind::Fertigation&&reading.status.rfind("Fed ",0)==0)farmFlow(p,q,FarmFlowKind::Nutrients,bed->second.crop);
   if(reading.kind==MachineKind::GrowLight&&reading.status.rfind("Lit /",0)==0)farmFlow(p,q,FarmFlowKind::Light,bed->second.crop);
   if(reading.kind==MachineKind::PollinatorHive&&(bed->second.crop=="tomato"||bed->second.crop=="pepper"||bed->second.crop=="berry"||bed->second.crop=="citrus"))farmFlow(p,q,FarmFlowKind::Pollination,bed->second.crop);
   if(reading.kind==MachineKind::CompostBin&&reading.status.rfind("Compost returned",0)==0)farmFlow(p,q,FarmFlowKind::Compost,bed->second.crop);
  }
 }
 for(auto& [id,b]:next){auto old=samples.find(id);if(old!=samples.end()){
   const auto& a=old->second;auto entry=entries.find(a.position);const auto& baseline=entry!=entries.end()&&entry->second.first==id?entry->second.second:a.material;auto lines=differences(baseline,b.material);auto source=f.at(a.position);
   auto pair=inputs.find(a.position);if(advanced&&source&&pair!=inputs.end()&&a.slot==0&&pair->second.second&&!next.count(pair->second.second)){
    auto extra=samples.find(pair->second.second);if(extra!=samples.end()&&(isMixingMachine(source->kind)||source->kind==MachineKind::Plater))lines.insert(lines.begin(),std::string(isMixingMachine(source->kind)?"MIX ":"ASSEMBLE ")+roundTo(a.material.metrics.mass)+" + "+roundTo(extra->second.material.metrics.mass)+" g");
   }
   for(auto& [k,g]:b.material.ingredients){auto i=baseline.ingredients.find(k);double added=g-(i==baseline.ingredients.end()?0:i->second);if(added>.005&&source&&isDosingMachine(source->kind))lines.insert(lines.begin(),"DOSE +"+roundTo(added,2)+" g "+ingredient(k).code);}
   auto prior=changes.find(a.position);if(prior!=changes.end()&&prior->second.batch==id&&(prior->second.title.rfind("MIX ",0)==0||prior->second.title.rfind("ASSEMBLE ",0)==0)&&std::find(lines.begin(),lines.end(),prior->second.title)==lines.end())lines.insert(lines.begin(),prior->second.title);
   if(a.slot==0&&!lines.empty()){std::string title=lines.front();lines.erase(lines.begin());changes[a.position]={id,f.tick,title,lines,b.material};}
   if(advanced&&a.position!=b.position&&std::abs(a.position.x-b.position.x)+std::abs(a.position.y-b.position.y)==1){uint64_t batchId=id;motions.erase(std::remove_if(motions.begin(),motions.end(),[batchId](const FloorMotion& m){return m.batch==batchId;}),motions.end());motions.push_back({id,a.position,b.position,b.slot,b.material,now,duration});}
  }else if(advanced){
   // Harvest creates a real batch at the harvester; it may enter the next belt
   // in that same tick. Its recorded bed and route establish both handoffs.
   if(b.material.cropBatch)for(const auto& [p,reading]:nextFarm)if(reading.kind==MachineKind::Harvester){auto source=f.at(p);Position bed=nextPosition(p,(source->dir+2)%4);auto oldBed=farmReadings.find(bed);auto newBed=nextFarm.find(bed);if(oldBed==farmReadings.end()||newBed==nextFarm.end()||!oldBed->second.planted||newBed->second.planted||!std::binary_search(b.route.begin(),b.route.end(),bed)||!std::binary_search(b.route.begin(),b.route.end(),p))continue;
    farmFlow(bed,p,FarmFlowKind::Harvest,oldBed->second.crop,b.material.metrics.mass);changes[p]={id,f.tick,"HARVEST "+roundTo(b.material.metrics.mass,0)+" g",{ingredient(oldBed->second.crop).name+" / grade "+(b.material.cropQuality>.8?"A":b.material.cropQuality>.55?"B":"C")},b.material};
    if(b.position==nextPosition(p,source->dir))motions.push_back({id,p,b.position,b.slot,b.material,now,duration});break;}
   // A source can create and move a batch during the same tick. Infer its edge
   // only from a connected real feeder or the matching physical split operation.
   for(int direction=0;direction<4;direction++){Position p=nextPosition(b.position,direction);auto device=f.at(p);if(!device)continue;const auto& m=*device;bool feed=!b.split&&m.enabled&&m.automatic&&std::binary_search(b.route.begin(),b.route.end(),p)&&m.kind==MachineKind::Feeder&&nextPosition(p,m.dir)==b.position&&b.material.ingredients.size()==1&&b.material.ingredients.count(m.ingredient)&&!inputs[p].first;
    if(feed){motions.push_back({id,p,b.position,b.slot,b.material,now,duration});break;}
   }
  }
 }
 // Both fractions are reported even when a side outlet is blocked. The split
 // operation's real component ID identifies its new side batch unambiguously.
 for(auto& [id,a]:samples)if(a.slot==0){auto main=next.find(id);if(main==next.end()||!main->second.split||main->second.split==a.split)continue;
  auto machine=f.at(a.position);if(!machine||!hasSideOutlet(machine->kind))continue;
  for(auto& [sideId,b]:next)if(sideId!=id&&std::find(b.parts.begin(),b.parts.end(),main->second.sidePart)!=b.parts.end()){
   std::string title=machine->kind==MachineKind::FractionatingStill?upper(b.material.fraction)+" VAPOUR CUT":machine->kind==MachineKind::FractionatingColumn?"REFLUX / ENRICHED VAPOUR":"SEPARATE / TWO FRACTIONS";
   changes[a.position]={id,f.tick,title,{"MAIN "+roundTo(main->second.material.metrics.mass,1)+" g / SIDE "+roundTo(b.material.metrics.mass,1)+" g",machine->kind==MachineKind::FractionatingColumn?"LIQUID REFLUX / VAPOUR PRODUCT":"BOTH OUTPUTS RETAINED"},main->second.material};
   if(advanced&&!samples.count(sideId)&&b.position==nextPosition(a.position,(machine->dir+1)%4))motions.push_back({sideId,a.position,b.position,b.slot,b.material,now,duration});
   break;
  }
 }
 for(auto& [id,a]:samples)if(!next.count(id)){auto m=f.at(a.position);if(m&&m->lastOutput&&m->lastOutput->id==id){bool stored=m->kind==MachineKind::ProduceIntake;changes[a.position]={id,f.tick,(stored?"STORED ":"SERVED ")+roundTo(a.material.metrics.mass)+" g",{stored?"NAMED FARM INVENTORY / "+ingredient(m->ingredient).name:"LAST RESULT / FOOD CONSUMED"},a.material};if(advanced&&stored)farmFlow(a.position,a.position,FarmFlowKind::Storage,m->ingredient,a.material.metrics.mass);}}
 motions.erase(std::remove_if(motions.begin(),motions.end(),[&](const FloorMotion& m){return !next.count(m.batch);}),motions.end());
 for(auto& [id,sample]:next)if(sample.slot==0){auto e=entries.find(sample.position);if(e==entries.end()||e->second.first!=id)entries[sample.position]={id,sample.material};}
 for(auto e=entries.begin();e!=entries.end();)if(!f.at(e->first))e=entries.erase(e);else ++e;
 for(auto i=changes.begin();i!=changes.end();)if(!f.at(i->first))i=changes.erase(i);else ++i;
 if(motions.size()>512)motions.erase(motions.begin(),motions.end()-512);
 if(farmFlows.size()>512)farmFlows.erase(farmFlows.begin(),farmFlows.end()-512);
 samples=std::move(next);farmReadings=std::move(nextFarm);inputs=std::move(nextInputs);tick=f.tick;fingerprint=hash;
}
} // namespace aftertaste
