#include "ReagentWorks.h"
#include "Power.h"
#include "WaterWorks.h"
#include <algorithm>
#include <cmath>
#include <numeric>
namespace aftertaste {
namespace {
using K=MachineKind;
constexpr double eps=1e-8;
bool finite(double v,double maximum=1e12){return std::isfinite(v)&&v>=0&&v<=maximum;}
double mass(const Batch& b){return analyse(b.food).mass;}
double compound(const Batch& b,Compound c){double n=0;for(const auto& p:b.food.parts)n+=p.food.c[c];return n;}
double carrier(const Batch& b){return std::max(0.,mass(b)-b.reagent.active-b.reagent.impurity);}
void visit(Batch& b,Position p){auto i=std::lower_bound(b.route.begin(),b.route.end(),p);if(i==b.route.end()||*i!=p)b.route.insert(i,p);}
Batch lot(Factory& f,const Batch& parent,const std::string& key,double active,double impurity,double water,double temperature){
 Batch b;b.id=f.nextId++;b.route=parent.route;b.age=parent.age;b.reagent={key,active,impurity,false,false};
 addIngredient(b.food,key,active,f.nextComponent++,f.nextOperation++);
 if(b.food.parts.empty())b.food.parts.push_back({f.nextComponent++,{}});
 auto& food=b.food.parts.front().food;food.c[Other]+=impurity;food.c[Water]+=water;food.temperature=temperature;
 food.fraction="Reagent / "+ingredient(key).name;food.events.push_back("Reagent production / mass-conserving game process");
 b.matter=projectCell(b.food);return b;
}
Batch residue(Factory& f,const Batch& source,const Batch& product,K kind){
 Batch b=source;b.id=f.nextId++;b.reagent={};b.cropBatch=false;b.residue="reagent residue";
 double remaining=mass(product),original=mass(source);
 auto take=[&](Compound c,double wanted){double available=compound(b,c);double amount=std::min({wanted,available,remaining});if(available>eps)for(auto& p:b.food.parts)p.food.c[c]*=1-amount/available;remaining-=amount;};
 if(kind==K::ReagentExtractor){
  // Extracted precursor leaves the crop. Repeated extraction cannot recreate it.
  auto& profile=ingredient(product.reagent.key);
  for(size_t n=0;n<CompoundCount;n++)if(n!=Water&&profile.fractions[n]>0){Compound from=Compound(n);if(from==Gelatin)from=Protein;take(from,product.reagent.active*profile.fractions[n]);}
 }else if(kind==K::CultureVat){take(Reducing,product.reagent.active);take(Sucrose,remaining);}
 else for(auto c:{Salt,Calcium,Other})take(c,product.reagent.active);
 take(Water,carrier(product)+product.reagent.active*ingredient(product.reagent.key).fractions[Water]);
 for(size_t n=0;n<CompoundCount;n++)if(n!=Water&&remaining>eps)take(Compound(n),remaining);
 if(remaining>eps)take(Water,remaining);
 double scale=original>eps?mass(b)/original:0;
 for(auto& part:b.food.parts){part.id=f.nextComponent++;auto& x=part.food;for(auto& v:x.aroma)v*=scale;for(auto& [key,v]:x.ingredients)v*=scale;x.lostMass*=scale;x.boundCalcium=std::min(x.boundCalcium,x.c[Calcium]);}
 b.food.parts.erase(std::remove_if(b.food.parts.begin(),b.food.parts.end(),[](const auto& p){return foodMass(p.food)<eps;}),b.food.parts.end());
 b.matter=projectCell(b.food);return b;
}
void finish(Machine& m,Batch main,Batch side){
 m.item=std::move(main);if(mass(side)>eps)m.sideOutput=std::move(side);m.ready=true;m.progress=0;
 m.status="Ready / route product and side fraction";m.power.flow=m.item->reagent.active;
}
void assay(Machine& instrument,Batch& b,int tick){
 b.reagent.assayed=true;auto& r=instrument.reagentProcess;r.sampleKey=b.reagent.key;r.sampleMass=mass(b);r.samplePurity=reagentPurity(b);r.sampleConcentration=reagentConcentration(b);
 instrument.sensorBatch=b.id;instrument.sensorTick=tick;instrument.sensorValue=r.samplePurity;instrument.sensorHistory.push_back(r.samplePurity);if(instrument.sensorHistory.size()>80)instrument.sensorHistory.erase(instrument.sensorHistory.begin());
 instrument.status="Assayed / "+roundTo(r.samplePurity*100,1)+"% purity / "+roundTo(r.sampleConcentration*100,1)+"% concentration";
}
}
bool isReagentMachine(K k){return k>=K::MineralHopper&&k<=K::ReagentRecovery;}
bool acceptsReagentMaterial(K k){return isReagentMachine(k)&&k!=K::MineralHopper&&k!=K::ReagentAnalyzer;}
bool reagentCompatible(K k,const Batch& b){
 if(k==K::MineralHopper||k==K::ReagentAnalyzer)return false;
 if(isReagentMachine(k))return true;
 // An unfinished reagent lot cannot become food through a mixer or serving dock.
 if(b.reagent.mineral||!b.reagent.key.empty())return isTransport(k)||k==K::GlassCoil||k==K::GlassValve||k==K::Bin;
 return true;
}
std::vector<std::string> reagentRecipes(K k){
 if(k==K::ReagentExtractor)return {"pectin","starch","lecithin","gelatin"};
 if(k==K::CultureVat)return {"culture","yeast","xanthan","amylase","binder","lactic_acid"};
 if(k==K::SaltReactor)return {"nutrient","salt","calcium","citrate","bicarbonate"};
 return {};
}
std::vector<MachineSpec> reagentMachineSpecs(){return {
 {K::MineralHopper,"Mineral feed hopper","MIN","A finite imported mineral blend. Load basic feedstock here; refine it on the floor rather than importing finished reagents.","#b8a286",50,1,0,0},
 {K::ReagentExtractor,"Botanical extraction vessel","EXT","Extracts pectin, starch, lecithin or gelatin from the corresponding real compounds in farm and food batches. Heat and dwell control recovery; spent material leaves the side port.","#b4a14e",120,6,0,0},
 {K::CultureVat,"Culture propagation vat","BIO","Uses real sugar-rich material and finite starter to grow cultures, enzymes or xanthan, or make acid. Temperature and dwell change yield. A stylised biological process.","#8baa74",140,10,0,0},
 {K::SaltReactor,"Mineral reaction bench","SAL","Refines finite mineral feed, ash or digestate into a chosen salt or farm nutrient. Calcium and citrate additionally consume supplied acid. Reactions use abstract game yields.","#b196ba",125,6,0,0},
 {K::ReagentSeparator,"Reagent fraction separator","SEP","Splits active reagent, impurities and carrier into distinct real fractions. Higher selectivity sacrifices yield to improve purity. Route both outlets.","#729fb1",120,4,0,0},
 {K::ReagentEvaporator,"Reagent concentrator","CONC","Removes carrier water into a side condensate batch while keeping active reagent. Heat controls concentration; excessive temperature damages active material.","#cb956c",125,5,0,0},
 {K::ReagentPolisher,"Reagent polishing column","POL","Removes most remaining impurity with a small active loss. Retained material is a side fraction, never silently deleted.","#8b92bd",105,4,0,0},
 {K::ReagentTank,"Reagent collection flask","LOT","Holds a real lot for assay, or forwards it after measurement. Connect an assay instrument beside the flask.","#69a7a3",55,1,0,0},
 {K::ReagentAnalyzer,"Reagent assay instrument","ASSAY","Points at an adjacent batch. Records its identity, dry purity and concentration, and certifies that actual lot for the factory inlet.","#7faeaa",70,1,0,0},
 {K::ReagentIntake,"Factory reagent inlet","CHEM+","Accepts assayed lots above 85% dry purity and 35% concentration. Only the named active reagent enters factory stock; water returns for treatment and impurity remains to be hauled.","#c07e9b",70,1,0,0},
 {K::ReagentRecovery,"Reagent return receiver","RET","Collects side fractions, returns carrier water to the waterworks, and holds finite solids for hauling. Organic extraction residues may instead feed compost, a boiler or another extraction.","#999b7a",45,1,0,0}
 };}
void initialiseReagentDevice(Machine& m){
 m.reagentProcess={};m.duration=std::max(1,machineSpec(m.kind).duration);m.temperature=65;m.shear=.8;m.dose=50;m.power.capacity=2000;
 auto recipes=reagentRecipes(m.kind);if(!recipes.empty())m.ingredient=recipes.front();
 if(m.kind==K::CultureVat)m.temperature=30;
 if(m.kind==K::ReagentEvaporator)m.temperature=70;
 if(m.kind==K::ReagentTank){m.temperature=20;m.forward=true;}
}
void clearReagentInventory(Machine& m){m.reagentProcess={};}
double reagentPurity(const Batch& b){double n=b.reagent.active+b.reagent.impurity;return n>eps?b.reagent.active/n:0;}
double reagentConcentration(const Batch& b){double n=mass(b);return n>eps?b.reagent.active/n:0;}
double genericChemicals(const FactoryState& f){double named=0;for(const auto& [key,kg]:f.reagentworks.inventory)named+=kg;return std::max(0.,f.chemicals-named);}
bool hasReagentSupply(const FactoryState& f,const std::string& key,double grams){auto i=f.reagentworks.inventory.find(key);double named=i==f.reagentworks.inventory.end()?0:i->second;return finite(grams)&&named+genericChemicals(f)+eps>=grams/1000.;}
bool canSupplyReagents(const FactoryState& f,const std::map<std::string,double>& grams){double generic=genericChemicals(f);for(const auto& [key,g]:grams)if(ingredient(key).category=="Agent"){auto i=f.reagentworks.inventory.find(key);double named=i==f.reagentworks.inventory.end()?0:i->second;generic-=std::max(0.,g/1000.-named);}return generic>=-eps;}
bool consumeReagentSupply(FactoryState& f,const std::string& key,double grams){
 if(!hasReagentSupply(f,key,grams))return false;
 auto i=f.reagentworks.inventory.find(key);if(i!=f.reagentworks.inventory.end())i->second=std::max(0.,i->second-grams/1000.);
 f.chemicals=std::max(0.,f.chemicals-grams/1000.);return true;
}
bool validReagentLot(const Batch& b){const auto& r=b.reagent;
 if(!finite(r.active)||!finite(r.impurity)||r.active+r.impurity>mass(b)+1e-5)return false;
 if(r.key.empty())return r.active==0&&r.impurity==0&&!r.assayed;
 return !r.mineral&&std::any_of(pantry().begin(),pantry().end(),[&](const auto& x){return x.key==r.key&&x.category=="Agent";});
}
bool validReagentState(const Machine& m){auto& r=m.reagentProcess;for(auto v:{r.minerals,r.starter,r.waste,r.produced,r.delivered,r.samplePurity,r.sampleConcentration,r.sampleMass})if(!finite(v))return false;
 if(r.minerals>5000||r.starter>100||r.waste>1e7||r.samplePurity>1||r.sampleConcentration>1||r.sampleKey.size()>32)return false;
 auto recipes=reagentRecipes(m.kind);return recipes.empty()||std::find(recipes.begin(),recipes.end(),m.ingredient)!=recipes.end();
}
bool validReagentWorks(const FactoryState& f){double sum=0;for(const auto& [key,n]:f.reagentworks.inventory){const std::string target=key;if(!finite(n)||!std::any_of(pantry().begin(),pantry().end(),[&](const auto& x){return x.key==target&&x.category=="Agent";}))return false;sum+=n;}
 for(auto v:{f.reagentworks.mineralImported,f.reagentworks.starterImported,f.reagentworks.produced,f.reagentworks.delivered,f.reagentworks.returnedWater,f.reagentworks.disposed})if(!finite(v))return false;
 return sum<=f.chemicals+1e-7;
}
bool configureReagentDevice(Factory& f,Machine& m,const std::string& setting){
 if(!isReagentMachine(m.kind))return false;auto& r=m.reagentProcess;
 if(setting=="reagent-recipe"){
  if(m.item||m.progress){f.notice="Finish or reclaim the current batch before changing its reaction.";return false;}
  auto options=reagentRecipes(m.kind);if(options.empty())return false;auto i=std::find(options.begin(),options.end(),m.ingredient);m.ingredient=options[(size_t(i-options.begin())+1)%options.size()];
 }else if(setting=="reagent-minerals"&&m.kind==K::MineralHopper){
  if(r.minerals>4500||(!f.sandbox&&f.cash<20)){f.notice="Mineral hopper full, or 20 credits needed for feedstock.";return false;}
  if(!f.sandbox)f.cash-=20;r.minerals+=500;f.reagentworks.mineralImported+=500;f.notice="Delivered 500 g raw mineral feed; finished reagents still require refining.";
 }else if(setting=="reagent-starter"&&m.kind==K::CultureVat){
  if(r.starter>90||(!f.sandbox&&f.cash<8)){f.notice="Starter bay full, or 8 credits needed for a starter culture.";return false;}
  if(!f.sandbox)f.cash-=8;r.starter+=10;f.reagentworks.starterImported+=10;f.notice="Loaded 10 g finite starter culture.";
 }else if(setting=="reagent-inoculate"&&m.kind==K::CultureVat){
  auto i=f.reagentworks.inventory.find("culture");if(r.starter>99||i==f.reagentworks.inventory.end()||i->second<.001){f.notice="Needs 1 g of your own purified culture in factory stock.";return false;}consumeReagentSupply(f,"culture",1);r.starter+=1;f.notice="Charged the vat with 1 g of your own propagated culture.";
 }else if(setting=="reagent-haul"&&(m.kind==K::ReagentRecovery||m.kind==K::ReagentIntake)){
  double price=r.waste*.01;if(!f.sandbox&&f.cash<price){f.notice="Not enough credits to haul the held solids.";return false;}if(!f.sandbox)f.cash-=price;f.reagentworks.disposed+=r.waste;r.waste=0;
 }else if(setting=="reagent-forward"&&m.kind==K::ReagentTank)m.forward=!m.forward;
 else if(setting=="reagent-reject"&&m.kind==K::ReagentIntake){if(!m.item||m.sideOutput)return false;m.sideOutput=std::move(m.item);m.item.reset();m.ready=false;}
 else return false;
 f.dirty=true;return true;
}
void disposeReagentInventory(FactoryState& f,Machine& m){f.reagentworks.disposed+=m.reagentProcess.waste+m.reagentProcess.minerals+m.reagentProcess.starter;clearReagentInventory(m);}
void runReagentWorks(Factory& f,const ReagentPower& supplied){
 auto power=[&](Position p,Machine& m,double draw){draw*=m.power.throttle;if(draw<=eps)return false;f.grid.requested+=draw;if(!supplied(p,draw)){m.status="Needs connected AC electricity";return false;}f.grid.delivered+=draw;f.powerUsed+=draw;f.energy+=draw;m.power.delivered+=draw;return true;};
 for(auto& [p,m]:f.machines){if(!isReagentMachine(m.kind)||!m.enabled||m.power.tripped)continue;auto& r=m.reagentProcess;
  if(m.kind==K::ReagentAnalyzer)continue;
  if(m.kind==K::MineralHopper){
   if(m.item)continue;double grams=std::min(m.dose,r.minerals);if(grams<=eps){m.status="Load raw mineral feedstock";continue;}if(!power(p,m,.1))continue;
   Batch b;b.id=f.nextId++;addIngredient(b.food,"salt",grams,f.nextComponent++,f.nextOperation++);b.reagent.mineral=true;b.residue="mineral feed";b.matter=projectCell(b.food);visit(b,p);m.item=b;m.ready=true;r.minerals-=grams;m.status="Measured mineral feed / "+roundTo(grams,1)+" g";continue;
  }
  if(!m.item){m.status="Awaiting material";continue;}
  if(m.kind==K::ReagentTank){m.ready=m.forward&&m.item->reagent.assayed;m.status=!m.forward?"Holding lot":m.ready?"Assayed / forwarding":"Awaiting adjacent assay instrument";continue;}
  if(m.kind==K::ReagentIntake||m.kind==K::ReagentRecovery){
   auto& b=*m.item;bool intake=m.kind==K::ReagentIntake;
   if(intake&&(b.reagent.key.empty()||!b.reagent.assayed||reagentPurity(b)<.85||reagentConcentration(b)<.35)){m.status="QUALITY HOLD / assay, purify and concentrate";continue;}
   double water=b.reagent.key.empty()?compound(b,Water):carrier(b);double dry=mass(b)-water;double retained=intake?b.reagent.impurity:dry;
   if(r.waste+retained>m.power.capacity+eps){m.status="Solids receiver full / haul or reroute side streams";continue;}
   double litres=water/1000.;if(f.waterworks.greywater.volume+litres>500+eps){m.status="Water return full / operate the waterworks";continue;}
   if(!power(p,m,.15))continue;
   addWaterParcel(f.waterworks.greywater,{litres,0,0,litres*.1});f.reagentworks.returnedWater+=litres;r.waste+=retained;
   if(intake){double kg=b.reagent.active/1000.;f.reagentworks.inventory[b.reagent.key]+=kg;f.chemicals+=kg;f.stock[b.reagent.key]+=kg*10;r.delivered+=b.reagent.active;f.reagentworks.delivered+=b.reagent.active;m.status="Stored "+ingredient(b.reagent.key).name+" / +"+roundTo(b.reagent.active,2)+" g";}
   else m.status="Returned carrier water / held solids "+roundTo(r.waste,1)+" g";
   m.lastOutput=b;m.item.reset();m.ready=false;m.progress=0;continue;
  }
  if(m.ready)continue;
  if(m.sideOutput){m.status="Side outlet blocked / route the by-product";continue;}
  bool reaction=m.kind==K::ReagentExtractor||m.kind==K::CultureVat||m.kind==K::SaltReactor;
  auto& b=*m.item;
  if(reaction&&(!b.reagent.key.empty()||b.reagent.mineral!=(m.kind==K::SaltReactor))){
   if(!(m.kind==K::SaltReactor&&(b.residue=="ash"||b.residue=="digestate"))){m.status="Wrong feed / use the matching raw material";continue;}
  }
  if(!reaction&&b.reagent.key.empty()){m.status="Needs a reagent extract or fermentation lot";continue;}
  double available=0;
  if(m.kind==K::ReagentExtractor){Compound c=m.ingredient=="pectin"?Pectin:m.ingredient=="starch"?Starch:m.ingredient=="lecithin"?Lecithin:Protein;available=compound(b,c);if(available<.001){m.status="Feed lacks "+ingredient(m.ingredient).name+" precursor";continue;}}
  if(m.kind==K::CultureVat){available=compound(b,Sucrose)+compound(b,Reducing);if(available<.05){m.status="Needs actual fermentable crop sugars";continue;}}
  if(m.kind==K::SaltReactor)available=mass(b)-compound(b,Water);
  double addedWater=reaction&&m.progress==0?.005:0;
  double starter=m.kind==K::CultureVat&&m.progress==0?.05:0;
  double acid=m.kind==K::SaltReactor&&m.progress==0&&(m.ingredient=="calcium"||m.ingredient=="citrate")?mass(b)*.15:0;
  if(f.water+eps<addedWater){m.status="Needs treated process water";continue;}
  if(r.starter+eps<starter){m.status="Load a finite starter culture";continue;}
  if(acid>eps&&!hasReagentSupply(f,"lactic_acid",acid)){m.status="Needs acid reagent / route a fermentation line";continue;}
  if(!power(p,m,m.kind==K::ReagentEvaporator?2:m.kind==K::CultureVat?.6:1.1))continue;
  if(addedWater>0){f.water-=addedWater;f.waterUsed+=addedWater;b.food.parts.front().food.c[Water]+=addedWater*1000;}
  if(starter>0){r.starter-=starter;addIngredient(b.food,"culture",starter,f.nextComponent++,f.nextOperation++);}
  if(acid>0){consumeReagentSupply(f,"lactic_acid",acid);addIngredient(b.food,"lactic_acid",acid,f.nextComponent++,f.nextOperation++);}
  m.power.workCredit+=m.power.throttle;if(m.power.workCredit<1-eps)continue;m.power.workCredit=std::max(0.,m.power.workCredit-1);m.progress++;m.status="Processing / "+std::to_string(m.progress)+" / "+std::to_string(m.duration);
  if(m.progress<m.duration)continue;
  if(reaction){
   double optimal=m.kind==K::CultureVat?30:m.kind==K::ReagentExtractor?65:70;
   double window=m.kind==K::CultureVat?15:45;
   double efficiency=(1-std::exp(-m.duration/(m.kind==K::CultureVat?6.:4.)))*std::exp(-std::pow((m.temperature-optimal)/window,2));
   if(b.cropBatch)efficiency*=std::clamp(b.cropQuality*b.cropFreshness,.05,1.);
   double active=available*efficiency*(m.kind==K::CultureVat?.38:m.kind==K::ReagentExtractor?.85:.75);
   if(active<.00001){m.progress=0;m.status="Reaction outside its working window / adjust temperature";continue;}
   double total=mass(b),impurity=active*((m.kind==K::ReagentExtractor?.8:.45)+(b.cropBatch?b.cropDirt*2:0)),water=std::min({std::max(0.,total-active-impurity),compound(b,Water),std::max(1.,active*2)});
   double product=active+impurity+water;
   if(product>total){double scale=total/product;active*=scale;impurity*=scale;water*=scale;product=total;}
   auto main=lot(f,b,m.ingredient,active,impurity,water,m.temperature);auto side=residue(f,b,main,m.kind);visit(main,p);visit(side,p);
   r.produced+=active;f.reagentworks.produced+=active;finish(m,std::move(main),std::move(side));
  }else{
   auto a=b.reagent.active,i=b.reagent.impurity,w=carrier(b);double aMain=a,iMain=i,wMain=w;
   if(m.kind==K::ReagentSeparator){double selectivity=std::clamp(m.shear/2.,0.,1.);aMain=a*(.98-selectivity*.12);iMain=i*(.45-selectivity*.35);wMain=w*.6;}
   if(m.kind==K::ReagentPolisher){aMain=a*.96;iMain=i*.06;wMain=w*.8;}
   if(m.kind==K::ReagentEvaporator){double remaining=std::clamp(1.-(m.temperature-25)/50,.025,1.);wMain=w*remaining;if(m.temperature>100){double damage=std::clamp((m.temperature-100)/160,0.,.65);double damaged=aMain*damage;aMain-=damaged;iMain+=damaged;}}
   auto main=lot(f,b,b.reagent.key,aMain,iMain,wMain,m.temperature);
   double sideActive=m.kind==K::ReagentEvaporator?0:std::max(0.,a-aMain),sideImpurity=m.kind==K::ReagentEvaporator?0:std::max(0.,i-iMain);
   auto side=lot(f,b,b.reagent.key,sideActive,sideImpurity,std::max(0.,w-wMain),m.temperature);
   finish(m,std::move(main),std::move(side));
  }
 }
 for(auto& [p,m]:f.machines)if(m.kind==K::ReagentAnalyzer&&m.enabled&&!m.power.tripped){auto other=f.at(nextPosition(p,m.dir));if(!other||!other->item||other->item->reagent.key.empty()){m.status="Point at a real reagent lot";continue;}if(!power(p,m,.05))continue;assay(m,*other->item,f.tick);}
}
Blueprint Factory::reagentWorksBlueprint(){
 Blueprint b;b.name="Reagent laboratory / extraction, culture and salts";b.width=7;b.height=8;
 auto add=[&](Position p,K k,int d=0)->Machine&{Machine m;m.kind=k;m.dir=d;if(isEnergyMachine(k))initialisePowerDevice(m);else{m.duration=1;m.ingredient="berry";m.dose=100;}b.parts.push_back({p,m});return b.parts.back().machine;};
 for(int row=0;row<3;row++){
  int y=row*3;
  if(row==1)add({0,y},K::MineralHopper);else{auto& feed=add({0,y},K::Feeder);feed.ingredient=row==0?"berry":"beet";feed.dose=100;feed.automatic=true;}
  add({1,y},row==0?K::ReagentExtractor:row==1?K::SaltReactor:K::CultureVat);
  add({2,y},K::ReagentSeparator);add({3,y},K::ReagentPolisher);add({4,y},K::ReagentEvaporator);add({5,y},K::ReagentTank);add({6,y},K::ReagentIntake);
  for(int x=1;x<=4;x++)add({x,y+1},K::ReagentRecovery);
  add({5,y+1},K::ReagentAnalyzer,3);
 }
 // One square AC header under the receivers; touching apparatus distributes it.
 for(int y=1;y<=7;y++){PowerLink link;link.medium=EnergyMedium::AC;link.rating=100;link.ports=10;b.links.push_back({{6,y},link});}
 return b;
}
bool installReagentWorksIn(FactoryState& f){
 if(std::any_of(f.machines.begin(),f.machines.end(),[](const auto& x){return isReagentMachine(x.second.kind);}))return false;
 auto b=Factory::reagentWorksBlueprint();auto o=reagentWorksOrigin;
 // Preserve player construction, including wiring and every empty gap in the plan.
 for(int y=0;y<b.height;y++)for(int x=0;x<b.width;x++)if(f.machines.count({o.x+x,o.y+y})||f.energyLinks.count({o.x+x,o.y+y}))return false;
 for(const auto& p:b.parts)f.machines.emplace(Position{o.x+p.at.x,o.y+p.at.y},p.machine);
 for(const auto& l:b.links)f.energyLinks[{o.x+l.at.x,o.y+l.at.y}].push_back(l.link);
 // A single square perimeter branch joins the existing waterworks AC export.
 std::vector<Position> feed;for(int y=7;y<=12;y++)feed.push_back({-10,y});for(int x=-15;x<=-11;x++)feed.push_back({x,12});feed.push_back({-15,13});
 for(auto p:feed){auto& links=f.energyLinks[p];auto i=std::find_if(links.begin(),links.end(),[](auto& l){return l.medium==EnergyMedium::AC;});if(i==links.end()){PowerLink l;l.medium=EnergyMedium::AC;l.rating=100;l.ports=15;links.push_back(l);}else i->ports|=15;}
 if(std::none_of(f.blueprints.begin(),f.blueprints.end(),[&](const auto& p){return p.name==b.name;}))f.blueprints.push_back(b);
 return true;
}
}
