#include "Factory.h"
#include "Power.h"
#include <algorithm>
#include <fstream>
#include <numeric>
#include <queue>
#include <stdexcept>

namespace aftertaste {
static void visit(Batch& b,Position p){auto it=std::lower_bound(b.route.begin(),b.route.end(),p);if(it==b.route.end()||*it!=p)b.route.insert(it,p);}
namespace {
struct CropProfile {double days,water,ph,temp,light,nutrient,yield;bool fruit;};
CropProfile cropProfile(const std::string& key){
 if(key=="mushroom")return {70,.82,6.6,19,.12,.38,360,false};
 if(key=="greens")return {46,.68,6.3,18,.62,.52,280,false};
 if(key=="basil")return {66,.62,6.4,22,.78,.48,180,false};
 if(key=="carrot")return {92,.60,6.5,18,.70,.58,390,false};
 if(key=="beet")return {88,.64,6.6,19,.66,.55,420,false};
 if(key=="berry")return {96,.68,6.0,20,.78,.55,460,true};
 if(key=="citrus")return {135,.63,6.2,24,.84,.62,520,true};
 if(key=="pepper")return {108,.62,6.4,25,.86,.64,440,true};
 return {100,.65,6.4,23,.82,.60,480,true};
}
std::string cropFamily(const std::string& key){
 if(key=="tomato"||key=="citrus"||key=="berry"||key=="pepper")return "fruiting";
 if(key=="basil"||key=="greens")return "leaf";
 if(key=="carrot"||key=="beet")return "root";
 return key=="mushroom"?"fungal":"other";
}
void plantFarmCrop(Machine& bed,const std::string& crop){
 if(!bed.farmLastCrop.empty()){
  if(cropFamily(bed.farmLastCrop)==cropFamily(crop))bed.farmRotationStress=clamp(bed.farmRotationStress+.32,0,1);
  else bed.farmRotationStress=std::max(0.,bed.farmRotationStress-.30);
 }
 bed.ingredient=crop;bed.farmPlanted=true;bed.farmGrowth=0;bed.farmAge=0;bed.farmHealth=1;
}
double farmInventorySum(const FactoryState& s){double total=0;for(const auto& [key,kg]:s.farmInventory)total+=kg;return total;}
std::string batchCropKey(const Batch& batch){std::string key;double largest=0;for(const auto& part:batch.food.parts)for(const auto& [candidate,amount]:part.food.ingredients)if(amount>largest&&ingredient(candidate).category=="Produce"){largest=amount;key=candidate;}return key;}
}
Position nextPosition(Position p,int dir){auto d=directions[(dir%4+4)%4];return {p.x+d[0],p.y+d[1]};}
std::string positionName(Position p){return std::to_string(p.x)+", "+std::to_string(p.y);}
std::string farmSeasonName(int tick){static const std::array<const char*,4> names={"Spring","Summer","Autumn","Winter"};return names[size_t((std::max(0,tick)/120)%4)];}
double farmSeasonTemperatureOffset(int tick){static const std::array<double,4> offsets={0,5,-2,-7};return offsets[size_t((std::max(0,tick)/120)%4)];}
double farmSeasonDaylight(int tick){static const std::array<double,4> daylight={.76,.98,.58,.34};return daylight[size_t((std::max(0,tick)/120)%4)];}
const std::array<MachineSpec,size_t(MachineKind::Count)>& machineSpecs(){
 static const std::array<MachineSpec,size_t(MachineKind::Count)> specs=[](){std::array<MachineSpec,size_t(MachineKind::Count)> all{};std::vector<MachineSpec> base={{MachineKind::Belt,"Conveyor","CV","Carries one batch per tile. Drag to lay a line; R turns the outlet.","#659ba7",2,0,0,0},
 {MachineKind::Feeder,"Ingredient feeder","IN","Draws a measured dose from the pantry and the matching farm-produce or chemical input buffer. Choose from 40 ingredients and agents.","#549b80",40,0,0,0},
 {MachineKind::Mixer,"Mixer","MX","Combines two input batches into one mixture. Ingredient ratios and inherited process history are preserved.","#9673bf",95,3,2,.03},
 {MachineKind::Gel,"Setting cabinet","GL","Cools hydrated hydrocolloids. Agar must be heated first; gelatin, pectin and alginate have different setting rules.","#00a976",70,6,1,.08},
 {MachineKind::Aerator,"Aerator","AR","Adds air under shear. Protein, lecithin and viscosity stabilise bubbles; un-emulsified fat disrupts them.","#00acbf",85,6,2,.02},
 {MachineKind::Chiller,"Blast chiller","CH","Freezes available water. Sugar, salt and alcohol depress freezing; cooling rate controls crystal size.","#148ed1",95,7,3,.03},
 {MachineKind::Heater,"Thermal reactor","HT","Hydrates agar, gelatinises starch, denatures proteins and develops Maillard or caramel flavours.","#f2663c",90,8,3,0},
 {MachineKind::Dryer,"Dehydrator","DR","Evaporates water and concentrates solutes. Water activity determines crispness and reaction rates.","#e6ad32",80,12,2,0},
 {MachineKind::Cloche,"Aroma vault","AV","Traps volatile aroma and fermentation gas. Excess pressure can fracture a fragile structure.","#cd6599",90,7,1,0},
 {MachineKind::Cycler,"Thermal cycler","TC","Alternates warm and cold stages. Thermal history changes gel networks and ice crystals.","#7d8ace",105,8,2,0},
 {MachineKind::Dispatch,"Tasting dock","OUT","Holds a real finished batch for tasting and naming, serves automatically, or forwards it for further processing. Place serve points outside factory areas or in a wall opening.","#335d70",45,0,0,0},
 {MachineKind::Bin,"Reclaimer","BIN","Clears unwanted material. Consider separating useful by-products before reclaiming.","#a4927c",8,0,0,0},
 {MachineKind::Doser,"Precision doser","DS","Adds measured grams to an incoming batch. Dose hydrocolloids, emulsifiers, enzymes or any pantry ingredient.","#4b9f86",65,1,1,0},
 {MachineKind::Homogenizer,"Homogeniser","HG","Reduces fat droplet size. Stable emulsions also require water and an interfacial stabiliser.","#538daf",120,5,3,.02},
 {MachineKind::Spherifier,"Spherification bath","SP","Adds a measured calcium dose and forms a shell around alginate-rich liquid. Strong acidity inhibits crosslinking.","#34a684",125,6,2,.08},
 {MachineKind::SousVide,"Sous vide bath","SV","Controlled gentle heating with little evaporation. Retains aroma and avoids harsh temperature overshoot.","#cf8c67",115,12,2,.04},
 {MachineKind::Fermenter,"Fermentation tank","FT","Incubates cultures. Sugar becomes acid, ethanol and gas; salt, alcohol and temperature alter activity.","#849b5f",135,16,1,.02},
 {MachineKind::EnzymeReactor,"Enzyme incubator","EN","Maintains the enzyme activity window. Papain breaks protein; amylase converts starch into reducing sugars.","#91aa64",130,10,2,0},
 {MachineKind::Centrifuge,"Centrifuge","CF","Separates aqueous and concentrated fat/solid fractions. Side outlet turns clockwise from the main outlet.","#878db2",150,5,4,.01},
 {MachineKind::Filter,"Clarifying filter","FL","Retains particles in a side fraction while passing clarified liquid. Both fractions keep mass and provenance.","#7b9fa1",100,4,1,.03},
 {MachineKind::VacuumChamber,"Vacuum chamber","VC","Vents gas and strips volatile aroma. Useful for degassing mixtures before setting or freezing.","#9b84ab",125,4,3,0},
 {MachineKind::FreezeDryer,"Freeze dryer","FD","Freezes then removes water under low pressure, preserving a porous concentrated solid.","#7cabc2",175,18,4,0},
 {MachineKind::Extruder,"Texture extruder","EX","Shapes sufficiently viscous mixtures. Cold thickening and polymer networks change its result.","#a39a6a",120,4,2,0},
 {MachineKind::Plater,"Component plater","PL","Joins two input batches as distinct components. Preserves texture and temperature contrast instead of blending.","#bb7790",95,1,1,0},
 {MachineKind::Smoker,"Aroma smoker","SM","Adds smoky aroma while warming. Time and temperature still alter the underlying material.","#a28070",110,6,2,0},
 {MachineKind::Mill,"Colloid mill","ML","Reduces suspended particle size and changes mouthfeel. Helps smooth cocoa, fruit and vegetable purees.","#aa9765",115,5,3,0},
 {MachineKind::Binder,"Protein binder","PB","Incubates transglutaminase and protein. Builds a crosslinked structure in a mild temperature window.","#a9809f",130,12,2,0},
 {MachineKind::RecipeKitchen,"Legacy recipe kitchen","PK","Compatibility device for older saves. New dishes are made with physical production lines.","#426f80",240,1,3,.03},
 {MachineKind::GlassTube,"Glass tubing","GT","Transparent batch transport. Drag to lay a run; bends follow connected inlets and the chosen outlet. One tile per tick.","#78b3bf",4,1,0,0},
 {MachineKind::GlassCoil,"Glass residence coil","GC","Coiled transparent tubing holds a batch for the configured dwell. Ambient chemistry continues before the batch moves onward.","#73a9b4",18,4,0,0},
 {MachineKind::GlassValve,"Glass stopcock","GV","A transparent isolation valve. Open or close it to admit or stop actual batches; closed valves retain any held food.","#5299aa",12,1,0,0},
 {MachineKind::GlassBeaker,"Receiving beaker","BK","Graduated receiving glassware. Hold food for inspection and naming, serve it, or forward it through a connected tube. Place serve points outside factory areas or in a wall opening.","#86b7bf",30,1,0,0},
 {MachineKind::GlassFlask,"Heated reaction flask","RF","Round-bottom glass flask on a heating mantle. Changes real food temperature and thermal chemistry at the chosen target and dwell.","#c58d71",85,8,2,0},
 {MachineKind::GlassCondenser,"Cooling condenser","CD","Jacketed heat exchanger cools a stream. When fed distillation vapour it condenses that real fraction into a liquid stream.","#7aaccc",90,6,2,.04},
 {MachineKind::Burette,"Glass burette","BU","Graduated glass dosing column with a stopcock. Adds the configured measured ingredient once to each incoming batch.","#80b8ac",55,1,1,0},
 {MachineKind::GlassJunction,"Glass mixing junction","YJ","A glass Y-mixer joins two actual input batches, preserving their ingredient masses and mixing them under shear.","#9d9cc3",55,2,1,0},
 {MachineKind::FractionatingStill,"Fractionating still","ST","Heated boiler that splits a real feed into a selected heads, hearts or tails vapour cut and concentrated stillage. Route the main residue onward and the clockwise side stream to a reflux column.","#d88361",150,7,3,0},
 {MachineKind::FractionatingColumn,"Reflux column","FC","Equilibrium trays enrich the vapour by volatility. Adjustable theoretical stages return a water-rich liquid reflux through the clockwise side port.","#6faeb5",110,5,1,0},
 {MachineKind::FractionReceiver,"Glass fraction receiver","FR","A dedicated glass vessel that holds, serves or forwards a condensed, labelled fraction as a real batch. Place serve points outside factory areas or in a wall opening.","#a3a6c5",38,1,0,0},
 {MachineKind::LatticeReactor,"Molecular lattice reactor","LR","Hold a real batch, arrange representative compound-family tokens into compatible matches, and apply one chemistry step using the batch's actual ingredients and reactor conditions.","#bd77a0",145,1,3,.01},
 {MachineKind::Wall,"Factory wall","WL","Place wall tiles in a continuous loop to enclose and mark a factory area. Untinted open floor remains outside the factory.","#82999a",8,0,0,0},
 {MachineKind::UtilityLine,"Utility line","UL","A typed, branching service conduit. Connect it to a matching utility station and tap nearby machines. Select power, heat, cooling, water or vacuum.","#587d8c",5,0,0,0},
 {MachineKind::UtilityStation,"Utility station","US","Supplies one shared factory service to connected lines. Stations of the same type add capacity to their network.","#9b79a5",85,0,0,0},
 {MachineKind::PHProbe,"pH probe","PH","Samples an adjacent live batch each tick and records its pH with batch identity and a short history.","#37a68f",28,0,0,0},
 {MachineKind::TemperatureLogger,"Temperature logger","TL","Logs the temperature of an adjacent live batch over time. Rotate its pointer toward the stream or vessel.","#e3784d",28,0,0,0},
 {MachineKind::DropletViewer,"Droplet microscope","DM","Measures the batch's actual average fat-droplet diameter and records the sampled batch.","#8973bd",36,0,0,0},
 {MachineKind::AromaSensor,"Aroma sensor","AS","Tracks volatile aroma intensity from an adjacent batch as it is heated, fermented, vented or distilled.","#bb914c",36,0,0,0},
 {MachineKind::FieldBed,"Field bed","FB","Open soil plot exposed to seasonal daylight and temperature. Repeating the same crop family builds rotation fatigue; use seeds, moisture, pH and NPK to shape the harvest.","#819b59",18,0,0,0},
 {MachineKind::RaisedBed,"Raised bed","RB","Deep raised soil holds more organic matter and buffers moisture and temperature swings. Rotate crop families to keep the bed healthy.","#9c8050",26,0,0,0},
 {MachineKind::HydroponicBed,"Hydroponic trough","HY","Water-fed controlled trough buffers seasonal temperature, grows quickly and responds precisely to nutrients. Connect fertigation or light nearby.","#42aabd",38,0,0,0},
 {MachineKind::SeedHopper,"Seed hopper","SH","Select a produce variety and face an empty bed. Finite seed stock auto-plants each cycle; switching the hopper to a new crop family eases soil rotation pressure.","#c69b4b",24,0,0,0},
 {MachineKind::Fertigation,"Fertigation mixer","FM","Feeds water and measured nutrient solution to adjacent beds. Uses factory water and chemical buffers; nutrient balance and salinity affect crop quality.","#568bb1",48,0,0,0},
 {MachineKind::GrowLight,"Grow light","LT","Supplies supplemental light to adjacent beds while electricity lasts, extending short-day seasonal growth. Mushrooms prefer shade.","#dbb84f",34,0,2,0},
 {MachineKind::PollinatorHive,"Pollinator hive","PHV","Improves fruit set for flowering crops in adjacent beds. Needs no batch route; place beside berry, tomato, pepper or citrus.","#d18b3e",32,0,.2,0},
 {MachineKind::SoilProbe,"Soil probe","SPB","Reads the nearest adjacent planted bed: moisture, pH, NPK and health. A diagnostic instrument for growing conditions.","#50a995",28,0,0,0},
 {MachineKind::Harvester,"Crop harvester","HV","Harvests a ripe bed directly behind its intake. Crop condition changes yield and flavour; the resulting real batch enters the conveyor network.","#719665",72,2,1,.01},
 {MachineKind::Washer,"Produce washer","PW","Washes soil from harvested crops. Consumes process water; washing removes dirt and a little soluble aroma.","#57a9c5",45,3,1,.08},
 {MachineKind::Grader,"Produce grader","PG","Sorts a crop batch by condition and evens out quality. Damaged or stressed produce yields a lower grade.","#bd9a55",42,2,.5,0},
 {MachineKind::ColdStore,"Cold store","CS","Rapidly chills produce and slows freshness loss before intake. Freshness decays while batches wait on the floor.","#508fbd",55,3,2,.02},
 {MachineKind::ProduceIntake,"Produce intake","PI","Accepts a named crop batch into the matching farm inventory. Store named crops, or send the actual batch onward through conveyors into factory processes.","#4e9d79",40,1,0,0},
 {MachineKind::CompostBin,"Composter","CB","Breaks down an unwanted food batch into organic matter and nutrients for an adjacent bed. Compost also eases crop-rotation fatigue.","#8b7955",30,4,.5,.02}};auto energy=energyMachineSpecs();base.insert(base.end(),energy.begin(),energy.end());if(base.size()!=all.size())throw std::logic_error("Power equipment catalogue mismatch");std::copy(base.begin(),base.end(),all.begin());return all;}();return specs;
}
const MachineSpec& machineSpec(MachineKind k){return machineSpecs().at(size_t(k));}
std::string utilityName(UtilityKind k){static const std::array<std::string,5> names={"Power","Heat","Cooling","Water","Vacuum"};return names.at(size_t(k));}
std::string utilityCode(UtilityKind k){static const std::array<std::string,5> names={"PWR","HEAT","COLD","H2O","VAC"};return names.at(size_t(k));}
std::string serveStyleName(ServeStyle s){static const std::array<std::string,3> names={"Restaurant pass","Research tasting","Chilled pickup"};return names.at(size_t(s));}
static double defaultUtilityCapacity(UtilityKind k){static const std::array<double,5> capacities={16,12,12,10,6};return capacities.at(size_t(k));}
bool isUtilityEquipment(MachineKind k){return k==MachineKind::UtilityLine||k==MachineKind::UtilityStation;}
bool isSensor(MachineKind k){return k==MachineKind::PHProbe||k==MachineKind::TemperatureLogger||k==MachineKind::DropletViewer||k==MachineKind::AromaSensor||k==MachineKind::SoilProbe;}
bool isFarmBed(MachineKind k){return k==MachineKind::FieldBed||k==MachineKind::RaisedBed||k==MachineKind::HydroponicBed;}
bool isFarmEquipment(MachineKind k){return k>=MachineKind::FieldBed&&k<=MachineKind::CompostBin;}
bool isFarmInfrastructure(MachineKind k){return isFarmBed(k)||k==MachineKind::SeedHopper||k==MachineKind::Fertigation||k==MachineKind::GrowLight||k==MachineKind::PollinatorHive||k==MachineKind::SoilProbe;}
std::vector<std::string> farmCropKeys(){std::vector<std::string> keys;for(const auto& crop:pantry())if(crop.category=="Produce")keys.push_back(crop.key);return keys;}
bool isTransport(MachineKind k){return k==MachineKind::Belt||k==MachineKind::GlassTube||k==MachineKind::GlassValve;}
bool hasSideOutlet(MachineKind k){return k==MachineKind::BiomassBoiler||k==MachineKind::AnaerobicDigester||k==MachineKind::Centrifuge||k==MachineKind::Filter||k==MachineKind::FractionatingStill||k==MachineKind::FractionatingColumn;}
bool isGlassConduit(MachineKind k){return k==MachineKind::GlassTube||k==MachineKind::GlassCoil||k==MachineKind::GlassValve;}
bool isLabGlassware(MachineKind k){return k>=MachineKind::GlassTube&&k<=MachineKind::FractionReceiver;}
bool isMixingMachine(MachineKind k){return k==MachineKind::Mixer||k==MachineKind::GlassJunction;}
bool isDosingMachine(MachineKind k){return k==MachineKind::Doser||k==MachineKind::Spherifier||k==MachineKind::Burette;}
bool isTastingDock(MachineKind k){return k==MachineKind::Dispatch||k==MachineKind::GlassBeaker||k==MachineKind::FractionReceiver;}
bool isProcessingMachine(MachineKind k){return !isEnergyMachine(k)&&!isEnergyConduit(k)&&!isTransport(k)&&k!=MachineKind::Feeder&&!isTastingDock(k)&&k!=MachineKind::Bin&&k!=MachineKind::Wall&&!isUtilityEquipment(k)&&!isSensor(k)&&!isFarmInfrastructure(k)&&k!=MachineKind::Harvester&&k!=MachineKind::ProduceIntake&&k!=MachineKind::CompostBin;}
static bool vaporBatch(const Batch& b){return !b.food.parts.empty()&&std::all_of(b.food.parts.begin(),b.food.parts.end(),[](const FoodPart& p){return p.food.vapor;});}

const std::vector<Request>& factoryRequests(){
 static const std::vector<Request> requests={
  {"Tidal pearls","pearls","Produce salt-rich bursting pearls. Dose alginate into brine, then use a calcium spherification bath.",{{"salt",1.1}},-1.6,1.5,6,100},
  {"Citrus clouds","foam","Ship a bright acidic foam while its bubbles still hold. Short output routes preserve fragile texture.",{{"acid",1.0}},-1.5,1.5,6,110},
  {"Frozen velvet","crystal","Freeze a sweet creamy mixture and deliver it cold. A backed-up line can change the serving state.",{{"sugar",.8},{"fat",.35}},-3.6,-.4,6,130},
  {"Caramel glass","crisp","Brown sugar with heat, then dry it into a brittle glass. Excess dwell can scorch the product.",{{"sugar",1.0},{"brown",.18}},-.5,3.2,6,150},
  {"Silken suspension","emulsion","Combine fat, protein and water, then aerate. Two feeders into a mixer make a flexible production cell.",{{"fat",.32},{"protein",.35}},-1.4,1.8,6,170},
  {"Roast savoury shards","crisp","Brown protein-rich food, then dehydrate it. The goal checks material and texture, not ingredient names.",{{"protein",.6},{"brown",.18}},-.5,3.2,6,160}};
 return requests;
}
void to_json(json& j,const Machine& m){
 j={{"power",m.power},{"kind",int(m.kind)},{"dir",m.dir},{"duration",m.duration},{"progress",m.progress},{"cooldown",m.cooldown},{"ingredient",m.ingredient},{"status",m.status},{"temperature",m.temperature},{"wear",m.wear},{"dose",m.dose},{"shear",m.shear},{"cut",m.cut},{"stages",m.stages},{"utility",int(m.utility)},{"utilityCapacity",m.utilityCapacity},{"sensorValue",m.sensorValue},{"sensorBatch",m.sensorBatch},{"sensorTick",m.sensorTick},{"sensorHistory",m.sensorHistory},{"serveStyle",int(m.serveStyle)},{"serviceFeedback",m.serviceFeedback},{"recipe",m.recipe},{"cursor",m.cursor},{"subtick",m.subtick},{"farmPlanted",m.farmPlanted},{"farmAge",m.farmAge},{"farmGrowth",m.farmGrowth},{"farmMoisture",m.farmMoisture},{"farmPH",m.farmPH},{"farmN",m.farmN},{"farmP",m.farmP},{"farmK",m.farmK},{"farmOrganic",m.farmOrganic},{"farmHealth",m.farmHealth},{"farmRotationStress",m.farmRotationStress},{"farmLastCrop",m.farmLastCrop},{"program",m.program},{"programOutput",m.programOutput},{"enabled",m.enabled},{"ready",m.ready},{"automatic",m.automatic},{"forward",m.forward},{"lattice",m.lattice},{"latticeBatch",m.latticeBatch},{"latticePending",m.latticePending},{"latticeMoves",m.latticeMoves},{"latticeRelease",m.latticeRelease},{"lastOutput",m.lastOutput?json(*m.lastOutput):json(nullptr)},{"item",m.item?json(*m.item):json(nullptr)},{"extra",m.extra?json(*m.extra):json(nullptr)},{"sideOutput",m.sideOutput?json(*m.sideOutput):json(nullptr)}};
}
void from_json(const json& j,Machine& m){
 m=Machine{};int k=j.at("kind");if(k<0||k>=int(machineSpecs().size()))throw std::invalid_argument("Unknown machine");m.kind=MachineKind(k);if(j.contains("utility")){int u=j.at("utility");if(u<0||u>=int(UtilityKind::Count))throw std::invalid_argument("Unknown utility");m.utility=UtilityKind(u);}if(j.contains("serveStyle")){int s=j.at("serveStyle");if(s<0||s>=3)throw std::invalid_argument("Unknown serving style");m.serveStyle=ServeStyle(s);}else if(m.kind==MachineKind::GlassBeaker)m.serveStyle=ServeStyle::ResearchTasting;else if(m.kind==MachineKind::FractionReceiver)m.serveStyle=ServeStyle::ChilledPickup;
#define READ(v) if(j.contains(#v))j.at(#v).get_to(m.v);
 READ(power) READ(dir) READ(duration) READ(progress) READ(cooldown) READ(ingredient) READ(status) READ(temperature) READ(wear) READ(dose) READ(shear) READ(cut) READ(stages) READ(utilityCapacity) READ(sensorValue) READ(sensorBatch) READ(sensorTick) READ(sensorHistory) READ(serviceFeedback) READ(recipe) READ(cursor) READ(subtick) READ(farmPlanted) READ(farmAge) READ(farmGrowth) READ(farmMoisture) READ(farmPH) READ(farmN) READ(farmP) READ(farmK) READ(farmOrganic) READ(farmHealth) READ(farmRotationStress) READ(farmLastCrop) READ(program) READ(programOutput) READ(enabled) READ(ready) READ(automatic) READ(forward) READ(lattice) READ(latticeBatch) READ(latticePending) READ(latticeMoves) READ(latticeRelease)
#undef READ
 if(j.contains("lastOutput")&&!j["lastOutput"].is_null())m.lastOutput=j["lastOutput"].get<Batch>();
 if(j.contains("sideOutput")&&!j["sideOutput"].is_null())m.sideOutput=j["sideOutput"].get<Batch>();
 if(j.contains("item")&&!j["item"].is_null())m.item=j["item"].get<Batch>();if(j.contains("extra")&&!j["extra"].is_null())m.extra=j["extra"].get<Batch>();
}
void to_json(json& j,const FactoryState& s){
 j=json::object();j["energyLinks"]=json::array();for(const auto& [p,links]:s.energyLinks)j["energyLinks"].push_back({{"at",p},{"links",links}});j["grid"]=s.grid;j["machines"]=json::array();for(auto& [pos,m]:s.machines)j["machines"].push_back({{"at",pos},{"machine",m}});
#define SAVE(v) j[#v]=s.v;
 SAVE(stock) SAVE(farmInventory) SAVE(farmQuality) SAVE(seedStock) SAVE(products) SAVE(shipments) SAVE(blueprints) SAVE(outputHistory) SAVE(discoveries) SAVE(cash) SAVE(water) SAVE(powerCapacity) SAVE(electricity) SAVE(farmProduce) SAVE(chemicals) SAVE(energy) SAVE(waterUsed) SAVE(powerUsed) SAVE(revenue) SAVE(tick) SAVE(shipped) SAVE(wasted) SAVE(rawUsed) SAVE(requestsDone) SAVE(request) SAVE(requestProgress) SAVE(requestDeadline) SAVE(nextId) SAVE(nextComponent) SAVE(nextOperation) SAVE(nextDish) SAVE(bench) SAVE(dishbook) SAVE(dishLayouts) SAVE(sandbox)
#undef SAVE
}
void from_json(const json& j,FactoryState& s){
 s=FactoryState{};if(j.contains("energyLinks"))for(const auto& p:j.at("energyLinks"))s.energyLinks.emplace(p.at("at").get<Position>(),p.at("links").get<std::vector<PowerLink>>());if(j.contains("grid"))j.at("grid").get_to(s.grid);if(!j.at("machines").is_array())throw std::invalid_argument("Invalid factory floor");
 for(auto& p:j.at("machines")){Position pos=p.at("at").get<Position>();if(!s.machines.emplace(pos,p.at("machine").get<Machine>()).second)throw std::invalid_argument("Duplicate machine location");}
#define READ(v) if(j.contains(#v))j.at(#v).get_to(s.v);
 READ(stock) READ(farmInventory) READ(farmQuality) READ(seedStock) READ(products) READ(shipments) READ(blueprints) READ(outputHistory) READ(discoveries) READ(cash) READ(water) READ(powerCapacity) READ(electricity) READ(farmProduce) READ(chemicals) READ(energy) READ(waterUsed) READ(powerUsed) READ(revenue) READ(tick) READ(shipped) READ(wasted) READ(rawUsed) READ(requestsDone) READ(request) READ(requestProgress) READ(requestDeadline) READ(nextId) READ(nextComponent) READ(nextOperation) READ(nextDish) READ(bench) READ(dishbook) READ(dishLayouts) READ(sandbox)
#undef READ
 if(!j.contains("requestDeadline")&&s.request>=0&&s.request<int(factoryRequests().size()))s.requestDeadline=s.tick+factoryRequests()[size_t(s.request)].window;
}
Factory::Factory(){reset();}
Machine* Factory::at(Position p){auto i=machines.find(p);return i==machines.end()?nullptr:&i->second;}
const Machine* Factory::at(Position p)const{auto i=machines.find(p);return i==machines.end()?nullptr:&i->second;}
static Machine device(MachineKind k,int dir=0,const std::string& key="brine"){
 Machine m;m.kind=k;m.dir=dir;m.duration=std::max(1,machineSpec(k).duration);m.ingredient=key;m.dose=ingredient(key).dose;
 switch(k){case MachineKind::Chiller:m.temperature=-28;break;case MachineKind::FreezeDryer:m.temperature=-35;break;case MachineKind::ColdStore:m.temperature=4;break;case MachineKind::SousVide:m.temperature=64;break;case MachineKind::Fermenter:m.temperature=29;break;case MachineKind::EnzymeReactor:m.temperature=52;break;case MachineKind::Binder:m.temperature=42;break;case MachineKind::GlassCondenser:m.temperature=4;break;case MachineKind::GlassFlask:m.temperature=75;break;case MachineKind::FractionatingStill:m.temperature=86;break;case MachineKind::FractionatingColumn:m.temperature=86;break;case MachineKind::Heater:m.temperature=110;break;case MachineKind::LatticeReactor:m.temperature=120;break;case MachineKind::FieldBed:case MachineKind::RaisedBed:case MachineKind::HydroponicBed:case MachineKind::SoilProbe:m.temperature=20;break;default:m.temperature=70;break;}
 if(k==MachineKind::GlassBeaker||k==MachineKind::FractionReceiver)m.automatic=false;if(k==MachineKind::FractionatingStill)m.cut=1;if(k==MachineKind::FractionatingColumn)m.stages=8;if(k==MachineKind::Burette){m.ingredient="lecithin";m.dose=1.5;}
 if(k==MachineKind::GlassBeaker)m.serveStyle=ServeStyle::ResearchTasting;if(k==MachineKind::FractionReceiver)m.serveStyle=ServeStyle::ChilledPickup;
 if(isEnergyMachine(k))initialisePowerDevice(m);if(k==MachineKind::SeedHopper){m.ingredient="tomato";m.automatic=true;}if(k==MachineKind::FieldBed||k==MachineKind::RaisedBed||k==MachineKind::HydroponicBed)m.ingredient="tomato";if(k==MachineKind::HydroponicBed){m.farmMoisture=.76;m.farmOrganic=.12;m.farmN=m.farmP=m.farmK=.62;}
 if(k==MachineKind::UtilityStation)m.utilityCapacity=defaultUtilityCapacity(m.utility);
 if(k==MachineKind::Doser){m.ingredient="alginate";m.dose=.8;}if(k==MachineKind::Spherifier){m.ingredient="calcium";m.dose=1;}return m;
}
static bool matchesStarterDemo(const std::map<Position,Machine>& machines,bool withDistillation,bool interconnected=false,bool latticeCell=false){
 auto core=std::vector<std::pair<Position,MachineKind>>{
  {{-7,0},MachineKind::Feeder},{{-6,0},MachineKind::Doser},{{-5,0},MachineKind::Spherifier},{{-4,0},MachineKind::Belt},{{-3,0},MachineKind::Dispatch},
  {{-7,2},MachineKind::Feeder},{{-6,2},MachineKind::Doser},{{-5,2},MachineKind::Aerator},{{-4,2},MachineKind::Belt},{{-3,2},MachineKind::Dispatch},
  {{-1,0},MachineKind::Feeder},{{0,0},MachineKind::Chiller},{{1,0},MachineKind::Belt},{{2,0},MachineKind::Dispatch},
  {{-1,4},MachineKind::Feeder},{{0,4},MachineKind::Belt},{{1,2},MachineKind::Feeder},{{1,3},MachineKind::Belt},{{1,4},MachineKind::Mixer},{{2,4},MachineKind::Homogenizer},{{3,4},MachineKind::Belt},{{4,4},MachineKind::Dispatch},
  {{-7,6},MachineKind::Feeder},{{-6,6},MachineKind::Belt},{{-5,4},MachineKind::Feeder},{{-5,5},MachineKind::Belt},{{-5,6},MachineKind::Mixer},{{-4,6},MachineKind::Doser},{{-3,6},MachineKind::Homogenizer},{{-2,6},MachineKind::Dispatch}};
 if(latticeCell)core[core.size()-2].second=MachineKind::LatticeReactor;
 auto distillation=Factory::fractionalDistillationBlueprint();auto expected=core;
 if(withDistillation)for(const auto& part:distillation.parts)expected.push_back({{-7+part.at.x,9+part.at.y},part.machine.kind});
 if(interconnected){for(int x=-3;x<=4;x++)expected.push_back({{x,1},MachineKind::Belt});expected.push_back({{5,1},MachineKind::Belt});for(int y=2;y<=4;y++)expected.push_back({{5,y},MachineKind::Belt});expected.push_back({{3,0},MachineKind::Belt});expected.push_back({{4,0},MachineKind::Belt});expected.push_back({{5,0},MachineKind::Belt});for(int x=-1;x<=4;x++)expected.push_back({{x,6},MachineKind::Belt});expected.push_back({{5,6},MachineKind::Belt});for(int x=1;x<=4;x++)expected.push_back({{x,12},MachineKind::Belt});expected.push_back({{5,12},MachineKind::Belt});for(int y=7;y<=11;y++)expected.push_back({{5,y},MachineKind::Belt});expected.push_back({{5,5},MachineKind::Mixer});expected.push_back({{6,5},MachineKind::Homogenizer});expected.push_back({{7,5},MachineKind::Belt});expected.push_back({{8,5},MachineKind::Dispatch});}
 if(machines.size()!=expected.size())return false;for(const auto& [p,k]:expected){auto i=machines.find(p);if(i==machines.end()||i->second.kind!=k)return false;}return true;
}
static constexpr Position northernFarmOrigin{-7,-12};
static bool northernFarmAt(const std::map<Position,Machine>& machines){
 for(int i=0;i<6;i++){int x=(i%2)*9,y=2+(i/2)*3;auto bed=machines.find({northernFarmOrigin.x+x+1,northernFarmOrigin.y+y}),intake=machines.find({northernFarmOrigin.x+x+7,northernFarmOrigin.y+y});if(bed==machines.end()||!isFarmBed(bed->second.kind)||intake==machines.end()||intake->second.kind!=MachineKind::ProduceIntake)return false;}return true;
}
static bool connectNorthernFarmDelivery(std::map<Position,Machine>& machines){
 if(!northernFarmAt(machines))return false;auto entry=machines.find({1,0});if(entry==machines.end()||entry->second.kind!=MachineKind::Belt||entry->second.dir!=0)return false;
 const auto plan=Factory::farmDeliveryBlueprint();
 // Only occupy the demo's clear aisles. A conveyor can replace a straight
 // wall segment because it seals that opening under the existing zoning rule.
 for(const auto& part:plan.parts){Position p{1+part.at.x,-10+part.at.y};auto existing=machines.find(p);if(existing==machines.end())continue;
  const auto& m=existing->second;if(m.kind==MachineKind::Belt&&m.dir==part.machine.dir)continue;
  if(m.kind!=MachineKind::Wall||m.item||m.extra||m.sideOutput)return false;
  bool straight=false;for(int d=0;d<2;d++){auto a=machines.find(nextPosition(p,d)),b=machines.find(nextPosition(p,d+2));if(a!=machines.end()&&b!=machines.end()&&a->second.kind==MachineKind::Wall&&b->second.kind==MachineKind::Wall)straight=true;}if(!straight)return false;
 }
 bool changed=false;for(const auto& part:plan.parts){Position p{1+part.at.x,-10+part.at.y};auto existing=machines.find(p);if(existing==machines.end()){machines.emplace(p,part.machine);changed=true;}else if(existing->second.kind==MachineKind::Wall){existing->second=part.machine;changed=true;}}
 for(int i=0;i<6;i++){Position p{northernFarmOrigin.x+(i%2)*9+7,northernFarmOrigin.y+2+(i/2)*3};auto& intake=machines.at(p);if(!intake.forward||intake.dir!=0){intake.forward=true;intake.automatic=false;intake.dir=0;intake.ready=bool(intake.item);changed=true;}}
 return changed;
}
static bool migrateLegacyStarterGarden(std::map<Position,Machine>& machines){
 const std::vector<std::pair<Position,MachineKind>> legacy={
  {{0,1},MachineKind::SeedHopper},{{1,1},MachineKind::RaisedBed},{{2,1},MachineKind::Harvester},{{3,1},MachineKind::Belt},{{4,1},MachineKind::Washer},{{5,1},MachineKind::Grader},{{6,1},MachineKind::ColdStore},{{7,1},MachineKind::ProduceIntake},
  {{1,2},MachineKind::HydroponicBed},{{1,0},MachineKind::SoilProbe},{{2,2},MachineKind::GrowLight},{{0,2},MachineKind::PollinatorHive},{{1,3},MachineKind::Fertigation}};
 auto core=machines;std::vector<std::pair<Position,Machine>> saved;for(const auto& expected:legacy){Position p{10+expected.first.x,3+expected.first.y};auto i=core.find(p);if(i==core.end()||i->second.kind!=expected.second)return false;saved.push_back(*i);core.erase(i);}
 auto blueprint=Factory::marketGardenBlueprint();for(const auto& part:blueprint.parts)if(core.count({northernFarmOrigin.x+part.at.x,northernFarmOrigin.y+part.at.y}))return false;
 for(const auto& part:blueprint.parts)core.emplace(Position{northernFarmOrigin.x+part.at.x,northernFarmOrigin.y+part.at.y},part.machine);
 const std::vector<std::pair<Position,Position>> moves={
  {{10,4},{-6,-11}},{{11,4},{-6,-10}},{{12,4},{-5,-10}},{{13,4},{-4,-10}},{{14,4},{-3,-10}},{{15,4},{-2,-10}},{{16,4},{-1,-10}},{{17,4},{0,-10}},
  {{11,5},{3,-10}},{{11,3},{2,-10}},{{12,5},{3,-9}},{{10,5},{-7,-10}},{{11,6},{-6,-9}}};
 std::map<Position,Position> remap;for(const auto& move:moves)remap.emplace(move.first,move.second);
 auto updateBatch=[&](std::optional<Batch>& batch){if(!batch)return;for(auto& p:batch->route)if(auto i=remap.find(p);i!=remap.end())p=i->second;std::sort(batch->route.begin(),batch->route.end());batch->route.erase(std::unique(batch->route.begin(),batch->route.end()),batch->route.end());};
 for(size_t i=0;i<saved.size();i++){auto machine=saved[i].second;Position target=remap.at(saved[i].first);if(machine.kind==MachineKind::SeedHopper)machine.dir=1;if(machine.kind==MachineKind::SoilProbe)machine.dir=0;updateBatch(machine.item);updateBatch(machine.extra);updateBatch(machine.sideOutput);updateBatch(machine.lastOutput);core[target]=std::move(machine);}
 for(auto& [pos,machine]:core){updateBatch(machine.item);updateBatch(machine.extra);updateBatch(machine.sideOutput);updateBatch(machine.lastOutput);}
 machines=std::move(core);return true;
}
static bool upgradeStarterLattice(std::map<Position,Machine>& machines){
 auto i=machines.find({-3,6});if(i==machines.end()||i->second.kind!=MachineKind::Homogenizer)return false;
 i->second.kind=MachineKind::LatticeReactor;i->second.duration=1;i->second.progress=0;i->second.ready=false;i->second.lattice.clear();i->second.latticeBatch=0;i->second.latticePending=-1;i->second.latticeMoves=0;i->second.latticeRelease=false;i->second.status="Idle";return true;
}
static void refreshExperimentBlueprint(std::vector<Blueprint>& blueprints){
 auto i=std::find_if(blueprints.begin(),blueprints.end(),[](const Blueprint& b){return b.name=="Experiment cell";});if(i!=blueprints.end())*i=Factory::experimentBlueprint();else blueprints.push_back(Factory::experimentBlueprint());
}
static bool interconnectStarterDemo(std::map<Position,Machine>& machines){
 auto add=[&](Position p,MachineKind k,int dir=0){return machines.emplace(p,device(k,dir)).second;};
 auto forward=[&](Position p,int dir){auto i=machines.find(p);if(i==machines.end()||!isTastingDock(i->second.kind))return false;i->second.dir=dir;i->second.automatic=false;i->second.forward=true;return true;};
 if(!forward({-3,0},1)||!forward({-3,2},3)||!forward({2,0},0)||!forward({4,4},0)||!forward({-2,6},0)||!forward({0,12},0))return false;
 for(int x=-3;x<=4;x++)if(!add({x,1},MachineKind::Belt,0))return false;if(!add({5,1},MachineKind::Belt,1))return false;for(int y=2;y<=4;y++)if(!add({5,y},MachineKind::Belt,1))return false;
 if(!add({3,0},MachineKind::Belt,0)||!add({4,0},MachineKind::Belt,0)||!add({5,0},MachineKind::Belt,1))return false;
 for(int x=-1;x<=4;x++)if(!add({x,6},MachineKind::Belt,0))return false;if(!add({5,6},MachineKind::Belt,3))return false;
 for(int x=1;x<=4;x++)if(!add({x,12},MachineKind::Belt,0))return false;if(!add({5,12},MachineKind::Belt,3))return false;for(int y=7;y<=11;y++)if(!add({5,y},MachineKind::Belt,3))return false;
 bool connected=add({5,5},MachineKind::Mixer,0)&&add({6,5},MachineKind::Homogenizer,0)&&add({7,5},MachineKind::Belt,0)&&add({8,5},MachineKind::Dispatch,0);
 if(connected&&machines.at({-3,6}).kind==MachineKind::Homogenizer)upgradeStarterLattice(machines);return connected;
}
static Blueprint starterDemoBlueprint(const std::map<Position,Machine>& machines){
 Blueprint bp;bp.name="Interconnected demo factory";bp.width=16;bp.height=13;for(const auto& [pos,machine]:machines)if(pos.x>=-7&&pos.x<=8&&pos.y>=0&&pos.y<=12){auto copy=machine;copy.item.reset();copy.extra.reset();copy.sideOutput.reset();copy.lastOutput.reset();copy.program.clear();copy.programOutput.clear();copy.cursor=copy.subtick=0;copy.progress=copy.cooldown=0;copy.ready=false;copy.wear=0;clearPowerInventory(copy);copy.lattice.clear();copy.latticeBatch=0;copy.latticePending=-1;copy.latticeMoves=0;copy.latticeRelease=false;copy.status="Idle";bp.parts.push_back({{pos.x+7,pos.y},copy});}return bp;
}
void Factory::reset(){
 static_cast<FactoryState&>(*this)=FactoryState{};running=false;history.clear();expandedStarterDemo=false;
 for(auto& i:pantry())stock[i.key]=40;
 for(const auto& crop:farmCropKeys())seedStock[crop]=12;
 auto add=[&](Position p,MachineKind k,int dir=0,const std::string& key="brine"){machines.emplace(p,device(k,dir,key));};
 // Compact starter district; loaded worlds retain their saved coordinates.
 add({-7,0},MachineKind::Feeder);add({-6,0},MachineKind::Doser);add({-5,0},MachineKind::Spherifier);add({-4,0},MachineKind::Belt);add({-3,0},MachineKind::Dispatch);
 add({-7,2},MachineKind::Feeder,0,"citrus");add({-6,2},MachineKind::Doser);machines.at({-6,2}).ingredient="lecithin";machines.at({-6,2}).dose=.8;add({-5,2},MachineKind::Aerator);add({-4,2},MachineKind::Belt);add({-3,2},MachineKind::Dispatch);
 add({-1,0},MachineKind::Feeder,0,"cream");add({0,0},MachineKind::Chiller);add({1,0},MachineKind::Belt);add({2,0},MachineKind::Dispatch);
 add({-1,4},MachineKind::Feeder,0,"cream");add({0,4},MachineKind::Belt);add({1,2},MachineKind::Feeder,1,"broth");add({1,3},MachineKind::Belt,1);add({1,4},MachineKind::Mixer);add({2,4},MachineKind::Homogenizer);add({3,4},MachineKind::Belt);add({4,4},MachineKind::Dispatch);
 captureBlueprint({-7,0},{-3,0},"Tidal pearls");captureBlueprint({-7,2},{-3,2},"Citrus air");captureBlueprint({-1,0},{2,0},"Frozen cream");captureBlueprint({-1,2},{4,4},"Emulsion cell");
 auto experiment=experimentBlueprint();stamp(experiment,{-7,4});blueprints.push_back(experiment);
 auto distillation=fractionalDistillationBlueprint();for(const auto& part:distillation.parts)machines.emplace(Position{-7+part.at.x,9+part.at.y},part.machine);blueprints.push_back(distillation);
 auto garden=marketGardenBlueprint();for(const auto& part:garden.parts)machines.emplace(Position{northernFarmOrigin.x+part.at.x,northernFarmOrigin.y+part.at.y},part.machine);blueprints.push_back(garden);
 expandedStarterDemo=interconnectStarterDemo(machines);if(expandedStarterDemo){blueprints.push_back(starterDemoBlueprint(machines));dispense({-7,6});dispense({-5,4});}
 connectNorthernFarmDelivery(machines);blueprints.push_back(farmDeliveryBlueprint());
 // Stagger real planted crops in a fresh demo so its delivery lines begin
 // producing promptly while other beds show earlier growth stages.
 const std::array<double,6> demoGrowth={.99,.94,.82,.73,.98,.61};for(int i=0;i<6;i++){Position p{northernFarmOrigin.x+(i%2)*9+1,northernFarmOrigin.y+2+(i/2)*3};auto& bed=machines.at(p);if(seedStock[bed.ingredient]>0){seedStock[bed.ingredient]--;plantFarmCrop(bed,bed.ingredient);bed.farmGrowth=demoGrowth[size_t(i)];}}

 installPowerDistrictIn(*this);
 history.clear();dirty=false;notice="A six-crop northern farm feeds its own harvest, washing, grading and cold-storage lines. Rotate crop families to protect soil health; seasonal light and temperature change outdoor growth.";
}
void Factory::snapshot(){history.push_back(static_cast<const FactoryState&>(*this));size_t limit=machines.size()>2000?8:40;while(history.size()>limit)history.pop_front();}
void Factory::undo(){if(history.empty()){notice="No construction to undo. Simulation ticks clear construction history.";return;}static_cast<FactoryState&>(*this)=history.back();history.pop_back();running=false;dirty=true;notice="Construction restored. Factory paused.";}
static bool validPosition(Position p){return std::abs(int64_t(p.x))<=1000000&&std::abs(int64_t(p.y))<=1000000;}
bool Factory::build(Position p,MachineKind k,int dir){
 if(isEnergyConduit(k))return buildEnergyLink(p,conduitMedium(k));
 if(!validPosition(p)||at(p)){notice="That tile is occupied. Inspect or reclaim the machine first.";return false;}
 if(isTastingDock(k)&&enclosedFactoryTiles().count(p)){notice="Serve points must stay outside factory areas. Place this dock beyond the walls or in a wall opening.";return false;}
 const auto& spec=machineSpec(k);if(!sandbox&&cash<spec.cost){notice="Not enough credits. Free build can be enabled at any time.";return false;}
 Machine m=device(k,(dir%4+4)%4);if(k==MachineKind::RecipeKitchen&&!dishbook.empty())m.recipe=dishbook.back().id;machines.emplace(p,m);
 bool invalidServePoint=false;if(k==MachineKind::Wall||isTransport(k)||isTastingDock(k)){auto area=enclosedFactoryTiles();for(const auto& [where,device]:machines)if(isTastingDock(device.kind)&&area.count(where)){invalidServePoint=true;break;}}
 machines.erase(p);if(invalidServePoint){notice="Walls cannot enclose serve points. Place each dock beyond the perimeter or in a wall opening.";return false;}
 snapshot();machines.emplace(p,std::move(m));if(!sandbox)cash-=spec.cost;dirty=true;notice=k==MachineKind::Wall?"Factory wall placed. Close a perimeter; serve points and conveyors can fill a wall opening without breaking the zone.":isTastingDock(k)?spec.name+" built outside the factory area.":spec.name+" built at "+positionName(p)+".";return true;
}
bool Factory::demolish(Position p){auto m=at(p);if(!m)return false;bool wall=m->kind==MachineKind::Wall;snapshot();if(m->item)wasted++;if(m->extra)wasted++;if(m->sideOutput)wasted++;if(!sandbox)cash+=machineSpec(m->kind).cost*.65;machines.erase(p);dirty=true;notice=wall?"Factory wall removed. Any open room is no longer marked as a factory area.":"Machine reclaimed. Occupied batches count as waste. Undo is available until the next tick.";return true;}
bool Factory::rotate(Position p){auto m=at(p);if(!m)return false;snapshot();m->dir=(m->dir+1)%4;dirty=true;notice="Outlet rotated at "+positionName(p)+".";return true;}
bool Factory::configure(Position p,const std::string& setting){
 auto m=at(p);if(!m)return false;snapshot();
 if(setting=="power-rate+")m->power.rate=std::min(1000.,m->power.rate+5);
 else if(setting=="power-rate-")m->power.rate=std::max(1.,m->power.rate-5);
 else if(setting=="power-capacity+")m->power.capacity=std::min(1e7,m->power.capacity+100);
 else if(setting=="power-capacity-")m->power.capacity=std::max(1.,m->power.capacity-100);
 else if(setting=="power-throttle+")m->power.throttle=std::min(1.,m->power.throttle+.1);
 else if(setting=="power-throttle-")m->power.throttle=std::max(0.,m->power.throttle-.1);
 else if(setting=="power-priority")m->power.priority=(m->power.priority+1)%3;
 else if(setting=="relay-low+")m->power.low=std::min(m->power.high-.05,m->power.low+.05);
 else if(setting=="relay-low-")m->power.low=std::max(0.,m->power.low-.05);
 else if(setting=="relay-high+")m->power.high=std::min(1.,m->power.high+.05);
 else if(setting=="relay-high-")m->power.high=std::max(m->power.low+.05,m->power.high-.05);
 else if(setting=="power-reset"){if(m->power.heat>.6){history.pop_back();notice="Breaker is still hot. Allow it to cool before resetting.";return false;}m->power.tripped=false;}
 else if(setting=="power-crank"&&m->kind==MachineKind::Flywheel){m->power.store[size_t(EnergyMedium::Shaft)]=std::min(m->power.capacity,m->power.store[size_t(EnergyMedium::Shaft)]+5);notice="Manual crank supplied 5 units of external mechanical work.";}
 else if(setting=="ingredient"){size_t next=0;const auto& choices=m->kind==MachineKind::SeedHopper?farmCropKeys():std::vector<std::string>{};if(m->kind==MachineKind::SeedHopper){auto i=std::find(choices.begin(),choices.end(),m->ingredient);if(i!=choices.end())next=(size_t(i-choices.begin())+1)%choices.size();m->ingredient=choices[next];}else{for(size_t i=0;i<pantry().size();i++)if(pantry()[i].key==m->ingredient)next=(i+1)%pantry().size();m->ingredient=pantry()[next].key;m->dose=pantry()[next].dose;}}
 else if(setting=="crop-next"&&isFarmBed(m->kind)){const auto choices=farmCropKeys();auto i=std::find(choices.begin(),choices.end(),m->ingredient);size_t next=i==choices.end()?0:(size_t(i-choices.begin())+1)%choices.size();m->ingredient=choices[next];}
 else if(setting=="plant"&&isFarmBed(m->kind)){if(m->farmPlanted){history.pop_back();notice="Harvest the current crop before planting this bed again.";return false;}if(seedStock[m->ingredient]<=0){history.pop_back();notice="No "+ingredient(m->ingredient).name+" seeds. Refill seed stock in the seed hopper.";return false;}seedStock[m->ingredient]--;plantFarmCrop(*m,m->ingredient);m->status="Seed planted";}
 else if(setting=="seed+"&&m->kind==MachineKind::SeedHopper){if(!sandbox&&cash<8){history.pop_back();notice="A seed packet costs 8 credits.";return false;}if(!sandbox)cash-=8;seedStock[m->ingredient]+=10;}
 else if(setting=="soil-ph+"&&isFarmBed(m->kind))m->farmPH=std::min(8.5,m->farmPH+.2);
 else if(setting=="soil-ph-"&&isFarmBed(m->kind))m->farmPH=std::max(4.0,m->farmPH-.2);
 else if(setting=="irrigate"&&isFarmBed(m->kind)){if(water<1){history.pop_back();notice="Water input buffer is empty.";return false;}water-=1;waterUsed+=1;m->farmMoisture=std::min(1.,m->farmMoisture+.3);}
 else if(setting=="cut-heads"&&m->kind==MachineKind::FractionatingStill){m->cut=0;m->temperature=78;}
 else if(setting=="cut-hearts"&&m->kind==MachineKind::FractionatingStill){m->cut=1;m->temperature=86;}
 else if(setting=="cut-tails"&&m->kind==MachineKind::FractionatingStill){m->cut=2;m->temperature=96;}
 else if(setting=="stages+"&&m->kind==MachineKind::FractionatingColumn)m->stages=std::min(20,m->stages+1);
 else if(setting=="stages-"&&m->kind==MachineKind::FractionatingColumn)m->stages=std::max(1,m->stages-1);
 else if(setting=="dwell+")m->duration=std::min(120,m->duration+1);
 else if(setting=="dwell-")m->duration=std::max(1,m->duration-1);
 else if(setting=="temp+")m->temperature=clamp(m->temperature+5,-100,220);
 else if(setting=="temp-")m->temperature=clamp(m->temperature-5,-100,220);
 else if(setting=="dose+")m->dose=std::min(1000.,m->dose+(m->dose<5?.1:5));
 else if(setting=="dose-")m->dose=std::max(.1,m->dose-(m->dose<=5?.1:5));
 else if(setting=="shear+")m->shear=std::min(2.,m->shear+.1);
 else if(setting=="shear-")m->shear=std::max(0.,m->shear-.1);
 else if(setting=="automatic")m->automatic=!m->automatic;
 else if(setting=="mode_hold"){m->automatic=false;m->forward=false;m->ready=false;}
 else if(setting=="mode_auto"){m->automatic=true;m->forward=false;m->ready=false;}
 else if(setting=="mode_forward"){m->automatic=false;m->forward=true;}
 else if(setting=="serve-style-next"&&isTastingDock(m->kind)){m->serveStyle=ServeStyle((int(m->serveStyle)+1)%3);}
 else if(setting=="utility-next"&&isUtilityEquipment(m->kind)){m->utility=UtilityKind((int(m->utility)+1)%int(UtilityKind::Count));if(m->kind==MachineKind::UtilityStation)m->utilityCapacity=defaultUtilityCapacity(m->utility);}
 else if(setting.rfind("utility-",0)==0&&isUtilityEquipment(m->kind)){std::string name=setting.substr(8);static const std::array<std::string,5> keys={"power","heat","cooling","water","vacuum"};for(int n=0;n<int(UtilityKind::Count);n++)if(name==keys[size_t(n)]){m->utility=UtilityKind(n);if(m->kind==MachineKind::UtilityStation)m->utilityCapacity=defaultUtilityCapacity(m->utility);}}
 else if(setting=="capacity+")m->utilityCapacity=std::min(200.,m->utilityCapacity+2);
 else if(setting=="capacity-")m->utilityCapacity=std::max(1.,m->utilityCapacity-2);
 else if(setting=="enabled")m->enabled=!m->enabled;
 else if(setting=="clean"){if(!sandbox&&cash<6){history.pop_back();notice="Cleaning costs 6 credits.";return false;}if(!sandbox)cash-=6;m->wear=0;}
 else{history.pop_back();return false;}
 dirty=true;notice=machineSpec(m->kind).name+" configured.";return true;
}
bool Factory::buyStock(const std::string& ingredient,int count){
 if(!stock.count(ingredient)||count<=0||count>1000)return false;double price=aftertaste::ingredient(ingredient).cost*count;
 if(!sandbox&&cash<price){notice="Not enough credits for this pantry purchase.";return false;}snapshot();if(!sandbox)cash-=price;stock[ingredient]+=count;const auto& category=aftertaste::ingredient(ingredient).category;if(category=="Produce"){double add=count*.1,old=farmInventory[ingredient],quality=farmQuality.count(ingredient)?farmQuality[ingredient]:.78;farmInventory[ingredient]+=add;farmQuality[ingredient]=(old*quality+add*.78)/farmInventory[ingredient];farmProduce+=add;}else if(category=="Dairy")farmProduce+=count*.1;dirty=true;notice="Pantry replenished: "+data()["TNAME"][ingredient].get<std::string>()+" +"+std::to_string(count)+".";return true;
}
void Factory::receiveElectricity(){constexpr double amount=2500,cost=60;if(!sandbox&&cash<cost){notice="Not enough credits for the electricity delivery.";return;}snapshot();if(!sandbox)cash-=cost;electricity+=amount;dirty=true;notice="Electricity delivered to the factory input buffer: +2500 units.";}
void Factory::receiveFarmProduce(){constexpr double amount=25,cost=80;if(!sandbox&&cash<cost){notice="Not enough credits for the farm-produce delivery.";return;}snapshot();if(!sandbox)cash-=cost;farmProduce+=amount;dirty=true;notice="Farm produce delivered to the factory input buffer: +25 kg.";}
void Factory::receiveChemicals(){constexpr double amount=2.5,cost=120;if(!sandbox&&cash<cost){notice="Not enough credits for the chemical delivery.";return;}snapshot();if(!sandbox)cash-=cost;chemicals+=amount;dirty=true;notice="Chemicals delivered to the factory input buffer: +2.5 kg.";}
void Factory::refillWater(){if(!sandbox&&cash<12){notice="Water refill costs 12 credits.";return;}snapshot();water+=100;if(!sandbox)cash-=12;dirty=true;notice="Water delivered to the factory input buffer: +100 units.";}
void Factory::upgradePower(){if(!sandbox&&cash<120){notice="Grid expansion costs 120 credits.";return;}snapshot();powerCapacity+=4;if(!sandbox)cash-=120;dirty=true;notice="Power capacity increased by 4 kW.";}
void Factory::flush(){snapshot();for(auto& [p,m]:machines){if(m.item)wasted++;if(m.extra)wasted++;if(m.sideOutput)wasted++;m.item.reset();m.extra.reset();m.sideOutput.reset();m.program.clear();m.programOutput.clear();m.cursor=m.subtick=0;m.progress=0;m.ready=false;m.lattice.clear();m.latticeBatch=0;m.latticePending=-1;m.latticeMoves=0;m.latticeRelease=false;}dirty=true;notice="In-flight batches cleared; your buildings are preserved.";}
void Factory::toggleEconomy(){snapshot();sandbox=!sandbox;running=false;dirty=true;notice=sandbox?"Free build enabled. Construction and factory-input deliveries are free; electricity, water, farm produce and chemicals still deplete.":"Economy enabled. Pantry, factory inputs, utilities, credits and cleaning are finite. Factory paused.";}
void Factory::setRequest(int index){if(index<0||index>=int(factoryRequests().size()))return;request=index;requestProgress=0;requestDeadline=tick+factoryRequests()[size_t(request)].window;dirty=true;notice="Restaurant ticket opened: "+factoryRequests()[request].name+" / "+std::to_string(factoryRequests()[size_t(request)].window)+" ticks.";}
void Factory::updateTicketClock(){if(request<0||request>=int(factoryRequests().size()))return;int window=std::max(1,factoryRequests()[size_t(request)].window);if(requestDeadline<=0)requestDeadline=tick+window;if(tick>=requestDeadline){requestProgress=0;requestDeadline=tick+window;dirty=true;notice="Ticket window ended. "+factoryRequests()[size_t(request)].name+" reopened; no batches were lost.";}}
std::string Factory::missingInputSupply(const std::string& key,double grams)const{const auto& kind=ingredient(key).category;if(kind=="Produce"||kind=="Dairy"){double named=0;auto i=farmInventory.find(key);if(i!=farmInventory.end())named=i->second;double generic=std::max(0.,farmProduce-farmInventorySum(*this));if(named+generic+1e-9<grams/1000.)return "Farm produce / "+ingredient(key).name;}if(kind=="Agent"&&chemicals+1e-9<grams/1000.)return "Chemical";return {};}
void Factory::consumeInputSupply(const std::string& key,double grams){const auto& kind=ingredient(key).category;if(kind=="Produce"||kind=="Dairy"){double generic=std::max(0.,farmProduce-farmInventorySum(*this)),needed=grams/1000.,usedNamed=0;auto i=farmInventory.find(key);if(i!=farmInventory.end()){usedNamed=std::min(needed,i->second);i->second-=usedNamed;needed-=usedNamed;}double usedGeneric=std::min(needed,generic);farmProduce=std::max(0.,farmProduce-usedNamed-usedGeneric);}else if(kind=="Agent")chemicals=std::max(0.,chemicals-grams/1000.);}
Batch Factory::makeBatch(const std::string& key,double grams){Batch b;b.id=nextId++;addIngredient(b.food,key,grams,nextComponent++,nextOperation++);b.matter=projectCell(b.food);rawUsed++;return b;}

namespace {
std::array<double,LatticeFamilyCount> latticeAmounts(const Batch& batch){
 Compounds c{};for(const auto& part:batch.food.parts)for(int i=0;i<CompoundCount;i++)c[size_t(i)]+=part.food.c[size_t(i)];
 return {c[Sucrose]+c[Reducing],c[Protein]+c[Amino],c[Fat],c[Alginate]+c[Agar]+c[Gelatin]+c[Pectin]+c[Xanthan],c[Starch],std::min(c[Acid],c[Base]),c[Calcium],c[Lecithin]};
}
bool latticeHasTriplet(const std::vector<int>& board){
 if(board.size()!=25)return false;for(int y=0;y<5;y++)for(int x=0;x<5;x++){int at=y*5+x,v=board[size_t(at)];if(v<0)continue;if(x<3&&board[size_t(at+1)]==v&&board[size_t(at+2)]==v)return true;if(y<3&&board[size_t(at+5)]==v&&board[size_t(at+10)]==v)return true;}return false;
}
std::vector<int> latticeMatches(const std::vector<int>& board){
 std::vector<int> marked(25,0);if(board.size()!=25)return marked;
 for(int y=0;y<5;y++)for(int x=0;x<5;){int at=y*5+x,v=board[size_t(at)],end=x+1;if(v>=0)while(end<5&&board[size_t(y*5+end)]==v)end++;if(v>=0&&end-x>=3)for(int q=x;q<end;q++)marked[size_t(y*5+q)]=v+1;x=end;}
 for(int x=0;x<5;x++)for(int y=0;y<5;){int at=y*5+x,v=board[size_t(at)],end=y+1;if(v>=0)while(end<5&&board[size_t((end)*5+x)]==v)end++;if(v>=0&&end-y>=3)for(int q=y;q<end;q++)marked[size_t(q*5+x)]=v+1;y=end;}
 return marked;
}
}
void Factory::rebuildLattice(Machine& machine,const Batch& batch){
 machine.latticeBatch=batch.id;machine.latticePending=-1;machine.latticeRelease=false;machine.lattice.assign(25,-1);
 std::vector<int> families;auto amounts=latticeAmounts(batch);for(int i=0;i<LatticeFamilyCount;i++)if(amounts[size_t(i)]>.015)families.push_back(i);
 if(families.size()<2)return;
 size_t shift=size_t((batch.id+uint64_t(machine.latticeMoves)*3)%families.size());std::rotate(families.begin(),families.begin()+shift,families.end());
 const int a=families[0],b=families[1];
 for(int y=0;y<5;y++)for(int x=0;x<5;x++)machine.lattice[size_t(y*5+x)]=(y%2==0?((x==2)?b:a):((x==2)?a:b));
 // The two central cells in the top row are a guaranteed first match after a
 // single swap. Extra families are placed only where they do not create a
 // match before the player acts.
 size_t cursor=size_t((batch.id*7+uint64_t(machine.latticeMoves)*11)%20);
 for(size_t family=2;family<families.size();family++)for(int copies=0;copies<3;copies++){
  bool placed=false;for(size_t attempt=0;attempt<20;attempt++){
   size_t step=(cursor+attempt*7)%20;int index=int(step)+5;if(index==12||index==13)continue;
   int old=machine.lattice[size_t(index)];machine.lattice[size_t(index)]=families[family];if(!latticeHasTriplet(machine.lattice)){placed=true;cursor=(step+1)%20;break;}machine.lattice[size_t(index)]=old;
  }
  if(!placed)break;
 }
}
bool Factory::latticeReaction(const Machine& machine,const Batch& batch,int family,Technique& technique,std::string& reason){
 Compounds c{};for(const auto& part:batch.food.parts)for(int i=0;i<CompoundCount;i++)c[size_t(i)]+=part.food.c[size_t(i)];
 const double sugar=c[Sucrose]+c[Reducing],protein=c[Protein]+c[Amino],hydros=c[Alginate]+c[Agar]+c[Gelatin]+c[Pectin]+c[Xanthan];auto anyPart=[&](auto predicate){return std::any_of(batch.food.parts.begin(),batch.food.parts.end(),[&](const FoodPart& part){return predicate(part.food);});};
 switch(family){
 case LatticeSugar:if(sugar<=.015){reason="No sugar in this batch";return false;}if(machine.temperature<45){reason="Raise reactor temperature above 45 C";return false;}if(!anyPart([&](const Food& f){return f.c[Sucrose]+f.c[Reducing]>.015&&f.temperature+(machine.temperature-f.temperature)*.38>=45;})){reason="Preheat the batch with another hot match first";return false;}technique=Technique::Heat;return true;
 case LatticeProtein:if(protein<=.015){reason="No protein in this batch";return false;}if(!anyPart([](const Food& f){return f.c[Protein]+f.c[Amino]>.015&&f.c[Water]>.02;})){reason="Protein aeration needs water in the same component";return false;}if(machine.shear<.2){reason="Increase shear for protein aeration";return false;}technique=Technique::Aerate;return true;
 case LatticeFat:if(c[Fat]<=.015){reason="No fat in this batch";return false;}if(!anyPart([](const Food& f){return f.c[Fat]>.015&&f.c[Water]>.02;})){reason="Emulsification needs fat and water in the same component";return false;}if(machine.shear<.1){reason="Increase shear for emulsification";return false;}technique=Technique::Homogenize;return true;
 case LatticeHydrocolloid:
  if(hydros<=.015){reason="No hydrocolloid in this batch";return false;}
  if(anyPart([](const Food& f){return f.c[Alginate]>.015&&f.c[Calcium]-f.boundCalcium>.001&&foodPH(f)>4&&f.c[Salt]/std::max(.001,foodMass(f))<.1;})){technique=Technique::Spherify;return true;}
  if(anyPart([](const Food& f){return f.c[Agar]>.015&&f.agarHydrated>.1&&f.c[Water]>.02;})){technique=Technique::Gel;return true;}
  if(anyPart([](const Food& f){return f.c[Gelatin]>.015&&f.gelatinHydrated>.1&&f.c[Water]>.02;})){technique=Technique::Gel;return true;}
  if(anyPart([](const Food& f){return f.c[Pectin]>.015&&f.c[Water]>.02&&(f.c[Calcium]>.01||((f.c[Sucrose]+f.c[Reducing])>.1&&foodPH(f)<4.2));})){technique=Technique::Gel;return true;}
  if(anyPart([](const Food& f){return f.c[Xanthan]>.015&&f.c[Water]>.02;})){if(machine.shear<.1){reason="Increase shear to disperse xanthan";return false;}technique=Technique::Mix;return true;}
  if(c[Alginate]>.015){reason="Alginate needs free calcium and pH above 4";return false;}if(c[Agar]>.015){reason="Hydrate agar with a hot sugar match first";return false;}if(c[Gelatin]>.015){reason="Hydrate gelatin with a hot match first";return false;}if(c[Pectin]>.015){reason="Pectin needs acid and sugar, or calcium";return false;}reason="Hydrocolloid setting needs water";return false;
 case LatticeStarch:if(c[Starch]<=.015){reason="No starch in this batch";return false;}if(machine.temperature<65){reason="Raise reactor temperature above 65 C";return false;}if(!anyPart([&](const Food& f){return f.c[Starch]>.015&&f.c[Water]>f.c[Starch]*2&&f.temperature+(machine.temperature-f.temperature)*.38>=65;})){reason="Starch needs more water or a hotter batch";return false;}technique=Technique::Heat;return true;
 case LatticeAcidBase:if(c[Acid]<=.015||c[Base]<=.015){reason="Neutralisation needs both acid and base";return false;}if(!anyPart([](const Food& f){return f.c[Acid]>.015&&f.c[Base]>.015;})){reason="Keep the acid and base in the same component";return false;}technique=Technique::Mix;return true;
 case LatticeCalcium:if(c[Calcium]<=.015){reason="No calcium in this batch";return false;}if(!anyPart([](const Food& f){return f.c[Calcium]-f.boundCalcium>.001&&f.c[Alginate]>.015&&foodPH(f)>4;})){reason="Pearls need free calcium, alginate and pH above 4 together";return false;}technique=Technique::Spherify;return true;
 case LatticeLecithin:if(c[Lecithin]<=.015){reason="No lecithin in this batch";return false;}if(!anyPart([](const Food& f){return f.c[Lecithin]>.015&&f.c[Fat]>.015&&f.c[Water]>.02;})){reason="Lecithin needs fat and water in the same component";return false;}if(machine.shear<.1){reason="Increase shear for emulsification";return false;}technique=Technique::Homogenize;return true;
 default:reason="Unknown molecular family";return false;
 }
}
bool Factory::prepareLattice(Position p){
 auto machine=at(p);if(!machine||machine->kind!=MachineKind::LatticeReactor||!machine->item)return false;
 if(machine->latticeBatch!=machine->item->id||machine->lattice.size()!=25){rebuildLattice(*machine,*machine->item);dirty=true;}
 if(machine->lattice.empty()||std::count_if(machine->lattice.begin(),machine->lattice.end(),[](int f){return f>=0;})<6){machine->status="Needs a second compound family";return false;}
 return true;
}
bool Factory::swapLattice(Position p,int first,int second){
 auto machine=at(p);if(!machine||machine->kind!=MachineKind::LatticeReactor||!machine->item||first<0||first>=25||second<0||second>=25||std::abs(first/5-second/5)+std::abs(first%5-second%5)!=1||machine->latticePending>=0)return false;
 if(!prepareLattice(p)||machine->lattice[size_t(first)]<0||machine->lattice[size_t(second)]<0){notice="Choose two adjacent occupied lattice cells while a real batch is in the reactor.";return false;}
 auto candidate=machine->lattice;std::swap(candidate[size_t(first)],candidate[size_t(second)]);auto marked=latticeMatches(candidate);std::set<int> families;for(int value:marked)if(value)families.insert(value-1);
 if(families.empty()){notice="No reaction formed. Swap adjacent compounds to align three or more.";return false;}
 if(families.size()>1){notice="Resolve one molecular family per move so each reaction stays legible.";return false;}
 Technique technique;std::string reason;int family=*families.begin();if(!latticeReaction(*machine,*machine->item,family,technique,reason)){notice=reason+". Adjust the reactor or batch, then try again.";machine->status=reason;return false;}
 snapshot();machine=at(p);machine->lattice=std::move(candidate);machine->latticePending=family;for(size_t i=0;i<marked.size();i++)if(marked[i])machine->lattice[i]=-1;machine->status="Reaction queued";dirty=true;
 static const std::array<std::string,LatticeFamilyCount> names={"sugar","protein","fat","hydrocolloid","starch","acid / base","calcium","lecithin"};notice=""+names[size_t(family)]+" match queued. Press Step or Run to change the actual batch.";return true;
}
bool Factory::releaseLattice(Position p){
 auto machine=at(p);if(!machine||machine->kind!=MachineKind::LatticeReactor||!machine->item){notice="Load a real batch into the lattice reactor before releasing it.";return false;}snapshot();machine=at(p);machine->latticeRelease=true;machine->status=machine->latticePending>=0?"Release after reaction":"Release queued";dirty=true;notice=machine->latticePending>=0?"The queued reaction will apply before this batch leaves the reactor.":"Batch release queued. Press Step or Run to move it to the connected machine.";return true;
}

static Environment environment(const Machine& m){
 Technique t=Technique::Rest;switch(m.kind){
 case MachineKind::GlassJunction:case MachineKind::Mixer:t=Technique::Mix;break;case MachineKind::Gel:t=Technique::Gel;break;case MachineKind::Aerator:t=Technique::Aerate;break;case MachineKind::Chiller:t=Technique::Freeze;break;case MachineKind::GlassFlask:case MachineKind::Heater:t=Technique::Heat;break;case MachineKind::Dryer:t=Technique::Dry;break;case MachineKind::Cloche:t=Technique::Seal;break;case MachineKind::Cycler:t=Technique::Cycle;break;
 case MachineKind::Homogenizer:t=Technique::Homogenize;break;case MachineKind::Spherifier:t=Technique::Spherify;break;case MachineKind::SousVide:t=Technique::SousVide;break;case MachineKind::Fermenter:t=Technique::Ferment;break;case MachineKind::EnzymeReactor:t=Technique::Enzyme;break;case MachineKind::Centrifuge:t=Technique::Centrifuge;break;case MachineKind::Filter:t=Technique::Filter;break;case MachineKind::VacuumChamber:t=Technique::Vacuum;break;case MachineKind::FreezeDryer:t=Technique::FreezeDry;break;case MachineKind::Extruder:t=Technique::Extrude;break;case MachineKind::Smoker:t=Technique::Smoke;break;case MachineKind::Mill:t=Technique::Mill;break;case MachineKind::Binder:t=Technique::Bind;break;case MachineKind::FractionatingStill:t=Technique::Distill;break;case MachineKind::FractionatingColumn:t=Technique::Reflux;break;case MachineKind::GlassCondenser:t=Technique::Condense;break;default:break;}
 Environment e{t,m.temperature,m.shear,t==Technique::Vacuum||t==Technique::FreezeDry?.12:1};e.cut=m.cut;e.stages=m.stages;return e;
}
void Factory::chemistryStep(Batch& b,const Machine& m,bool processing){
 std::vector<uint64_t> targets;for(auto& p:b.food.parts)targets.push_back(p.id);
 processBatch(b.food,processing?environment(m):Environment{},targets,nextOperation++);b.age++;b.matter=projectCell(b.food);
 for(auto& p:b.food.parts)for(auto& e:p.food.events)discoveries.insert(e);
}
void Factory::merge(Machine& m){
 if(!m.item||!m.extra)return;auto& a=*m.item;auto& b=*m.extra;if(a.residue.empty())a.residue=b.residue;double massA=std::max(.001,analyse(a.food).mass),massB=std::max(.001,analyse(b.food).mass),mass=massA+massB;a.cropBatch=a.cropBatch||b.cropBatch;a.cropQuality=(a.cropQuality*massA+b.cropQuality*massB)/mass;a.cropDirt=(a.cropDirt*massA+b.cropDirt*massB)/mass;a.cropFreshness=std::min(a.cropFreshness,b.cropFreshness);mergeHistories(a.food,b.food);std::vector<uint64_t> ids;for(auto& p:a.food.parts)ids.push_back(p.id);
 if(isMixingMachine(m.kind))blendParts(a.food,ids,nextComponent++,nextOperation++);a.matter=projectCell(a.food);a.age=std::max(a.age,b.age);a.dish=0;std::vector<Position> route;std::set_union(a.route.begin(),a.route.end(),b.route.begin(),b.route.end(),std::back_inserter(route));a.route=std::move(route);m.extra.reset();
}
static double meanCompound(const Batch& b,const std::string& key){double total=0,weight=0;for(size_t i=0;i<b.matter.stack.size();i++){auto& p=b.matter.stack[i];if(p.kind!="food")continue;double w=std::pow(1.35,double(i));total+=(key=="brown"?p.brown:p.at(key))*w;weight+=w;}return weight?total/weight:0;}
Quality Factory::assess(const Batch& b,int requestIndex)const{
 auto p=profile(b.matter);Quality q;if(!b.residue.empty()){q.reason="Industrial residue: "+b.residue;return q;}if(!p.has){q.reason="No edible material";return q;}
 q.score=analyse(b.food).quality;
 int which=requestIndex<0?request:requestIndex;if(which<0||which>=int(factoryRequests().size()))return q;const auto& spec=factoryRequests()[which];
 if(p.phase!=spec.phase){q.reason="Needs "+upper(spec.phase)+"; this is "+upper(p.phase);return q;}
 for(auto& [key,minimum]:spec.minimum)if(meanCompound(b,key)<minimum){q.reason="Needs more "+upper(key);return q;}
 if(p.temp<spec.lowTemp||p.temp>spec.highTemp){q.reason="Serving temperature outside request range";return q;}
 if(q.score<65){q.reason="Request needs quality 65+";return q;}q.matches=true;q.reason="Matches the optional request";return q;
}
double Factory::marketValue(const Batch& b)const{
 if(!b.residue.empty())return 0;
 auto p=profile(b.matter);auto metrics=analyse(b.food);p.phase=metrics.phase;static const std::map<std::string,double> values={{"liquid",8},{"gel",16},{"pearls",27},{"foam",24},{"crystal",25},{"crisp",22},{"emulsion",33},{"plated",35},{"puree",13},{"curd",19},{"char",3},{"split",4}};
 auto found=values.find(p.phase);double base=found==values.end()?8:found->second;
 double aroma=clamp(std::accumulate(p.a.begin(),p.a.end(),0.)/4,0,1.5);
 return (base+std::max(0,p.layers-1)*3+aroma*3+p.reveal*2+clamp(p.roast,0,1)*2+metrics.contrast*10)*(.4+.6*assess(b).score/100.);
}
static std::string joined(const std::vector<std::string>& words,const std::string& separator){std::string value;for(const auto& word:words){if(!value.empty())value+=separator;value+=word;}return value;}
static std::string feedbackForServing(const Batch& batch,ServeStyle style,int requestIndex,bool ticketMet){
 auto metrics=analyse(batch.food);auto p=profile(batch.matter);p.phase=metrics.phase;std::string snapshot="Q "+roundTo(metrics.quality,0)+" / pH "+roundTo(metrics.pH,2)+" / "+roundTo(metrics.temperature,1)+" C / aroma "+roundTo(metrics.aroma*100,0)+" / AIR "+roundTo(metrics.foam/(1+metrics.foam)*100,0)+"% / GEL "+roundTo(metrics.gel*100,0)+"%";
 if(style==ServeStyle::ResearchTasting){std::string report="Research tasting / "+snapshot;if(!metrics.notes.empty())report+=" / investigate: "+joined(std::vector<std::string>(metrics.notes.begin(),metrics.notes.begin()+std::min<size_t>(2,metrics.notes.size())),"; ");else report+=" / no strong process warning";return report;}
 if(style==ServeStyle::ChilledPickup)return ticketMet?"Cold-chain passed at "+roundTo(metrics.temperature,1)+" C / "+snapshot:"Cold-chain held at "+roundTo(metrics.temperature,1)+" C; target <= 8 C / route through a chiller / "+snapshot;
 const Request& req=factoryRequests()[size_t(std::clamp(requestIndex,0,int(factoryRequests().size())-1))];std::vector<std::string> met,missed,advice;double tempMin=20+req.lowTemp*40,tempMax=20+req.highTemp*40;
 if(p.phase==req.phase)met.push_back("phase "+upper(req.phase));else{missed.push_back("phase needs "+upper(req.phase)+", got "+upper(p.phase));advice.push_back("change the final setting route");}
 for(const auto& [key,minimum]:req.minimum){double amount=meanCompound(batch,key);if(amount+1e-9>=minimum)met.push_back(upper(key)+" "+roundTo(amount,2)+" g");else{missed.push_back("needs "+upper(key)+" "+roundTo(minimum,2)+" g, got "+roundTo(amount,2)+" g");advice.push_back("dose or recover more "+upper(key));}}
 if(p.temp>=req.lowTemp&&p.temp<=req.highTemp)met.push_back("temperature window");else{missed.push_back("temperature target "+roundTo(tempMin,0)+"–"+roundTo(tempMax,0)+" C; got "+roundTo(metrics.temperature,1)+" C");advice.push_back("adjust the final thermal stage");}
 if(metrics.quality>=65)met.push_back("quality "+roundTo(metrics.quality,0));else{missed.push_back("quality needs 65+, got "+roundTo(metrics.quality,0));advice.push_back("inspect the process notes below");}
 std::string report="Order / "+req.name+" / "+snapshot+" / met: "+(met.empty()?"none":joined(met,", "))+" / missed: "+(missed.empty()?"none":joined(missed,", "));if(!advice.empty()){std::sort(advice.begin(),advice.end());advice.erase(std::unique(advice.begin(),advice.end()),advice.end());report+=" / try: "+joined(advice,"; ");}return report;
}
void Factory::ship(Machine& m,Position at){
 if(!m.item)return;if(!m.item->residue.empty()){m.status="Process residue / route to compost or fuel equipment";return;}updateTicketClock();auto preview=analyse(m.item->food);if(m.serveStyle==ServeStyle::ChilledPickup&&preview.temperature>8){m.serviceFeedback=feedbackForServing(*m.item,m.serveStyle,request,false);m.status="Too warm for chilled pickup";notice=machineSpec(m.kind).name+" is holding the batch above 8 C. Chill it upstream, then try again.";return;}
 auto batch=std::move(*m.item);m.item.reset();m.ready=false;if(dish(m.recipe))batch.dish=m.recipe;auto p=profile(batch.matter);auto metrics=analyse(batch.food);p.phase=metrics.phase;Quality q=m.serveStyle==ServeStyle::OrderPass?assess(batch):Quality{metrics.quality,m.serveStyle==ServeStyle::ChilledPickup,""};
 if(!p.has||p.integrity<.12){wasted++;m.serviceFeedback="Not served / batch integrity is too low / Q "+roundTo(metrics.quality,0)+" / pH "+roundTo(metrics.pH,2)+" / "+roundTo(metrics.temperature,1)+" C";m.status="Spoiled batch reclaimed";notice="The batch failed its integrity check and was reclaimed. Review its process history before the next run.";return;}
 std::string report=feedbackForServing(batch,m.serveStyle,request,q.matches);m.serviceFeedback=report;
 std::set<std::string> ingredients;for(auto& z:batch.matter.stack)if(z.kind=="food")ingredients.insert(z.k);
 std::string key,name;for(auto& z:ingredients){if(!name.empty()){name+=" + ";key+="/";}name+=data()["TNAME"][z].get<std::string>();key+=z;}key+="/"+p.phase;name+=" / "+upper(p.phase);if(auto d=dish(batch.dish)){key="dish/"+std::to_string(d->id);name=d->name;for(auto& design:dishbook)if(design.id==d->id)design.made++;}
 double value=m.serveStyle==ServeStyle::ResearchTasting?0:marketValue(batch);if(m.serveStyle==ServeStyle::ChilledPickup)value*=1.1;if(m.serveStyle==ServeStyle::OrderPass&&q.matches){value+=8;requestProgress++;}
 m.lastOutput=batch;cash+=value;revenue+=value;shipped++;
 auto& product=products[key];product.name=name;product.phase=p.phase;product.count++;product.best=std::max(product.best,q.score);product.totalQuality+=q.score;product.totalValue+=value;
 Shipment shipment{name,p.phase,q.score,value,m.serveStyle==ServeStyle::OrderPass&&q.matches,tick,serveStyleName(m.serveStyle),positionName(at),report};shipments.insert(shipments.begin(),std::move(shipment));if(shipments.size()>40)shipments.pop_back();m.status=m.serveStyle==ServeStyle::ResearchTasting?"Sample analysed":m.serveStyle==ServeStyle::ChilledPickup?"Cold pickup served":q.matches?"Served / ticket matched":"Served / ticket missed";
 if(m.serveStyle==ServeStyle::OrderPass&&q.matches&&requestProgress>=factoryRequests()[size_t(request)].need){int bonus=factoryRequests()[size_t(request)].bonus;cash+=bonus;revenue+=bonus;requestsDone++;requestProgress=0;request=(request+1)%int(factoryRequests().size());requestDeadline=tick+factoryRequests()[size_t(request)].window;notice="Ticket completed +"+std::to_string(bonus)+" credits. Next restaurant pass: "+factoryRequests()[size_t(request)].name+".";}else notice=serveStyleName(m.serveStyle)+" / "+(m.serveStyle==ServeStyle::ResearchTasting?"sample logged without a sales score.":m.serveStyle==ServeStyle::ChilledPickup?"batch accepted for cold pickup.":q.matches?"ticket target met.":"served; review the sensory misses and process notes.");
}
bool Factory::accept(Machine& m,Batch b){
 if(m.kind==MachineKind::Wall||(isEnergyMachine(m.kind)&&!acceptsEnergyFuel(m.kind)))return false;
 if(m.kind==MachineKind::Feeder||m.kind==MachineKind::RecipeKitchen||isUtilityEquipment(m.kind)||isSensor(m.kind)||isFarmInfrastructure(m.kind)||m.kind==MachineKind::Harvester||m.sideOutput)return false;
 if(m.kind==MachineKind::ProduceIntake&&!b.cropBatch)return false;
 if(m.kind==MachineKind::FractionatingColumn&&!vaporBatch(b))return false;
 if(m.kind==MachineKind::FractionatingStill&&vaporBatch(b))return false;
 if(isTastingDock(m.kind)&&vaporBatch(b))return false;
 if(!m.item){m.item=std::move(b);m.progress=0;m.ready=isTransport(m.kind);return true;}
 if((isMixingMachine(m.kind)||m.kind==MachineKind::Plater)&&m.progress==0&&!m.ready&&!m.extra&&m.item->food.parts.size()+b.food.parts.size()<=16){m.extra=std::move(b);return true;}
 return false;
}
namespace {
struct UtilityGrid {bool networked=false;std::map<Position,int> component;std::vector<double> capacity,used;};
using UtilityGrids=std::array<UtilityGrid,size_t(UtilityKind::Count)>;
UtilityGrids utilityGrids(const std::map<Position,Machine>& machines){
 UtilityGrids grids;for(int service=0;service<int(UtilityKind::Count);service++){auto& grid=grids[size_t(service)];std::set<Position> unseen;for(const auto& [p,m]:machines)if(m.enabled&&isUtilityEquipment(m.kind)&&m.utility==UtilityKind(service)){unseen.insert(p);if(m.kind==MachineKind::UtilityStation)grid.networked=true;}
  while(!unseen.empty()){Position first=*unseen.begin();unseen.erase(first);int id=int(grid.capacity.size());grid.capacity.push_back(0);grid.used.push_back(0);std::vector<Position> queue{first};grid.component[first]=id;
   for(size_t i=0;i<queue.size();i++){Position p=queue[i];auto it=machines.find(p);if(it!=machines.end()&&it->second.kind==MachineKind::UtilityStation)grid.capacity[size_t(id)]+=it->second.utilityCapacity;for(int d=0;d<4;d++){Position q=nextPosition(p,d);auto n=unseen.find(q);if(n!=unseen.end()){unseen.erase(n);grid.component[q]=id;queue.push_back(q);}}}
  }
 }
 return grids;
}
bool hotUtilityMachine(MachineKind k){return k==MachineKind::Heater||k==MachineKind::SousVide||k==MachineKind::Fermenter||k==MachineKind::EnzymeReactor||k==MachineKind::Binder||k==MachineKind::GlassFlask||k==MachineKind::FractionatingStill||k==MachineKind::FractionatingColumn||k==MachineKind::LatticeReactor||k==MachineKind::Smoker||k==MachineKind::Cycler;}
bool coldUtilityMachine(MachineKind k){return k==MachineKind::Chiller||k==MachineKind::GlassCondenser||k==MachineKind::FreezeDryer||k==MachineKind::Cycler;}
}
void Factory::step(){
 tick++;history.clear();updateTicketClock();powerUsed=0;int before=shipped;const auto factoryArea=enclosedFactoryTiles();auto grids=utilityGrids(machines);beginPowerStep();
 auto consumeUtilities=[&](Position p,const Machine& m)->std::string{
  const auto& spec=machineSpec(m.kind);std::vector<std::pair<UtilityKind,double>> needs;if(spec.power>0&&!grid.active)needs.push_back({UtilityKind::Power,spec.power});if(spec.water>0)needs.push_back({UtilityKind::Water,std::max(.2,spec.water*20)});
  if(hotUtilityMachine(m.kind)&&m.temperature>20)needs.push_back({UtilityKind::Heat,1+std::max(0.,m.temperature-20)/40});if(coldUtilityMachine(m.kind)&&m.temperature<20)needs.push_back({UtilityKind::Cooling,1+std::max(0.,20-m.temperature)/40});if(m.kind==MachineKind::VacuumChamber||m.kind==MachineKind::FreezeDryer)needs.push_back({UtilityKind::Vacuum,1});
  struct Reservation{size_t service;int component;double amount;};std::vector<Reservation> reservations;for(auto [service,amount]:needs){size_t index=size_t(service);auto& grid=grids[index];if(!grid.networked)continue;bool supplied=false;int chosen=-1;for(int d=0;d<4;d++){auto n=grid.component.find(nextPosition(p,d));if(n==grid.component.end())continue;int id=n->second;if(grid.capacity[size_t(id)]>0)supplied=true;if(grid.capacity[size_t(id)]-grid.used[size_t(id)]+1e-9>=amount){chosen=id;break;}}if(chosen<0)return supplied?utilityName(service)+" utility exhausted":"Connect a "+utilityName(service)+" source";reservations.push_back({index,chosen,amount});}
  for(const auto& r:reservations)grids[r.service].used[size_t(r.component)]+=r.amount;return {};
 };
 std::set<Position> litBeds,pollinatedBeds;
 // Farm support equipment runs before beds so its influence is deterministic on every floor.
 for(auto& [pos,m]:machines){
  if(!m.enabled)continue;
  if(m.kind==MachineKind::SeedHopper){if(!m.automatic){m.status="Automatic planting paused";continue;}Position target=nextPosition(pos,m.dir);auto bed=at(target);if(!bed||!isFarmBed(bed->kind)||bed->farmPlanted){m.status=bed&&isFarmBed(bed->kind)?"Waiting for empty bed":"Point at a grow bed";continue;}if(seedStock[m.ingredient]<=0){m.status="Seed stock empty / "+ingredient(m.ingredient).name;continue;}seedStock[m.ingredient]--;plantFarmCrop(*bed,m.ingredient);m.status="Planted "+ingredient(m.ingredient).name;bed->status="Seed planted";dirty=true;continue;}
  if(m.kind==MachineKind::GrowLight){bool powered=powerAvailable(pos,machineSpec(m.kind).power)&& (grid.active||sandbox||powerUsed+machineSpec(m.kind).power<=powerCapacity);std::string missing=powered?consumeUtilities(pos,m):"Electricity supply empty";if(!missing.empty())powered=false;if(powered){powerUsed+=machineSpec(m.kind).power;energy+=machineSpec(m.kind).power;usePower(pos,machineSpec(m.kind).power);for(int d=0;d<4;d++){Position q=nextPosition(pos,d);if(auto bed=at(q);bed&&isFarmBed(bed->kind))litBeds.insert(q);}m.status="Lit / " + roundTo(machineSpec(m.kind).power,0)+" power";}else m.status=missing.empty()?"Power limited":missing;continue;}
  if(m.kind==MachineKind::PollinatorHive){for(int d=0;d<4;d++){Position q=nextPosition(pos,d);auto bed=at(q);if(bed&&isFarmBed(bed->kind)&&cropProfile(bed->ingredient).fruit)pollinatedBeds.insert(q);}m.status="Pollinating nearby fruit crops";continue;}
  if(m.kind==MachineKind::Fertigation){if(m.cooldown>0){m.cooldown--;m.status="Nutrient solution settling";continue;}std::vector<Machine*> beds;for(int d=0;d<4;d++){auto bed=at(nextPosition(pos,d));if(bed&&isFarmBed(bed->kind)&&bed->farmPlanted)beds.push_back(bed);}if(beds.empty()){m.status="Needs an adjacent planted bed";continue;}if(chemicals<.02||water<1){m.status=chemicals<.02?"Chemical input empty":"Water input empty";continue;}auto missing=consumeUtilities(pos,m);if(!missing.empty()){m.status=missing;continue;}chemicals-=.02;water-=1;waterUsed+=1;for(auto bed:beds){bed->farmMoisture=std::min(1.,bed->farmMoisture+.12);bed->farmN=std::min(1.,bed->farmN+.12);bed->farmP=std::min(1.,bed->farmP+.08);bed->farmK=std::min(1.,bed->farmK+.10);bed->farmPH=clamp(bed->farmPH-.015,4,8.5);}m.cooldown=6;m.status="Fed "+std::to_string(beds.size())+" bed(s) / NPK +";continue;}
 }
 for(auto& [pos,m]:machines){
  if(isEnergyMachine(m.kind))continue;
  if(m.kind==MachineKind::Wall){m.status="Boundary";continue;}
  if(isUtilityEquipment(m.kind)){m.status=!m.enabled?"Service disabled":m.kind==MachineKind::UtilityStation?utilityName(m.utility)+" supply / "+roundTo(m.utilityCapacity,0):utilityName(m.utility)+" line";continue;}
  if(isSensor(m.kind)){m.status=m.enabled?"Waiting for sample":"Sensor paused";continue;}
  if(m.kind==MachineKind::SeedHopper||m.kind==MachineKind::Fertigation||m.kind==MachineKind::GrowLight||m.kind==MachineKind::PollinatorHive){if(!m.enabled)m.status="Disabled";continue;}
  const auto& spec=machineSpec(m.kind);m.status="Idle";if(m.cooldown>0)m.cooldown--;
  if(m.sideOutput){chemistryStep(*m.sideOutput,m,false);if(m.sideOutput->cropBatch)m.sideOutput->cropFreshness=std::max(0.,m.sideOutput->cropFreshness-.001);}
  if(m.item&&m.item->cropBatch&&m.kind!=MachineKind::ColdStore)m.item->cropFreshness=std::max(0.,m.item->cropFreshness-.001);
  if(!m.enabled){m.status="Disabled";if(m.item&& !m.item->food.parts.empty())chemistryStep(*m.item,m,false);if(m.extra)chemistryStep(*m.extra,m,false);continue;}
  if(isFarmBed(m.kind)){
   if(!m.farmPlanted){m.status="Empty grow bed / crop "+ingredient(m.ingredient).name;continue;}
   const auto crop=cropProfile(m.ingredient);m.farmAge++;double waterLoss=m.kind==MachineKind::HydroponicBed?.0025:.004; m.farmMoisture=std::max(0.,m.farmMoisture-waterLoss);m.farmRotationStress=std::max(0.,m.farmRotationStress-.00015*m.farmOrganic);
   if(m.farmMoisture<crop.water-.16&&water>=.8){water-=.8;waterUsed+=.8;m.farmMoisture=std::min(1.,m.farmMoisture+.26);}
   double light=m.ingredient=="mushroom"?.16:m.kind==MachineKind::HydroponicBed?.80:farmSeasonDaylight(tick);if(litBeds.count(pos))light=.98;double lightFactor=clamp(1.-std::abs(light-(m.ingredient=="mushroom"?.18:.84))*1.5,.25,1.);
   if(pollinatedBeds.count(pos))m.status="Flower set / pollinated";
   double moistureFactor=clamp(1.-std::abs(m.farmMoisture-crop.water)*2.6,.12,1.);
   double phFactor=clamp(1.-std::max(0.,std::abs(m.farmPH-crop.ph)-.25)*.55,.18,1.);
   double nutrient=(m.farmN+m.farmP+m.farmK)/3.;double nutrientFactor=clamp(.30+nutrient*1.05,.3,1.05);
   double seasonBuffer=m.kind==MachineKind::HydroponicBed?.18:m.kind==MachineKind::RaisedBed?.72:1.;double effectiveTemp=m.temperature+farmSeasonTemperatureOffset(tick)*seasonBuffer;double tempFactor=clamp(1.-std::abs(effectiveTemp-crop.temp)/18.,.2,1.);
   double healthFactor=clamp(m.farmHealth,.15,1.);double rotationFactor=clamp(1.-m.farmRotationStress*.42+m.farmOrganic*.08,.48,1.02);
   double grow=moistureFactor*phFactor*lightFactor*nutrientFactor*tempFactor*healthFactor*rotationFactor;
   if(m.farmMoisture<crop.water*.55){m.farmHealth=std::max(.05,m.farmHealth-.006);m.status="Drought stress / irrigate";}
   else if(phFactor<.5){m.farmHealth=std::max(.05,m.farmHealth-.003);m.status="pH stress / adjust soil";}
   else if(nutrient<.22){m.farmHealth=std::max(.05,m.farmHealth-.003);m.status="Nutrient stress / fertigate";}
   else if(m.farmRotationStress>.68){m.farmHealth=std::max(.05,m.farmHealth-.002);m.status="Rotation fatigue / change crop family";}
   else m.farmHealth=std::min(1.,m.farmHealth+.0015);
   if(m.farmGrowth<1){m.farmGrowth=std::min(1.,m.farmGrowth+grow/crop.days*(m.kind==MachineKind::HydroponicBed?1.18:m.kind==MachineKind::RaisedBed?1.04:1.));m.farmN=std::max(0.,m.farmN-.00055);m.farmP=std::max(0.,m.farmP-.00032);m.farmK=std::max(0.,m.farmK-.00045);m.farmOrganic=std::max(0.,m.farmOrganic-.0001);}
   if(m.farmGrowth>=1){if(m.farmGrowth==1)m.status="Ripe / ready for harvest";else m.status="Overripe / harvest now";m.farmGrowth=std::min(1.2,m.farmGrowth+.001);}else if(m.status=="Idle"||m.status=="Flower set / pollinated")m.status=ingredient(m.ingredient).name+" / "+std::to_string(int(m.farmGrowth*100))+"% / health "+std::to_string(int(m.farmHealth*100))+"%";
   continue;
  }
  if(isFarmInfrastructure(m.kind))continue;
  if(m.kind==MachineKind::Harvester){
   if(m.item){m.status=m.ready?"Harvest ready / route to washer":"Holding crop batch";continue;}
   Position bedPos=nextPosition(pos,(m.dir+2)%4);auto bed=at(bedPos);if(!bed||!isFarmBed(bed->kind)){m.status="Needs a grow bed behind intake";continue;}if(!bed->farmPlanted||bed->farmGrowth<1){m.status=bed->farmPlanted?"Crop growing / "+std::to_string(int(bed->farmGrowth*100))+"%":"Bed is empty";continue;}
   if(!powerAvailable(pos,machineSpec(m.kind).power)||(!grid.active&&!sandbox&&powerUsed+machineSpec(m.kind).power>powerCapacity)||water<machineSpec(m.kind).water){m.status=!powerAvailable(pos,machineSpec(m.kind).power)?"Electricity input empty":"Check farm utilities";continue;}auto missing=consumeUtilities(pos,m);if(!missing.empty()){m.status=missing;continue;}
   double quality=clamp(bed->farmHealth*(.72+.28*std::min(1.,(bed->farmN+bed->farmP+bed->farmK)/1.5))*(1.-bed->farmRotationStress*.30),.08,1.);bool pollinated=pollinatedBeds.count(bedPos);auto profile=cropProfile(bed->ingredient);double grams=profile.yield*quality*(profile.fruit&&!pollinated?.78:1.)*(bed->kind==MachineKind::HydroponicBed?1.12:1.);m.item=makeBatch(bed->ingredient,grams);m.item->cropBatch=true;m.item->cropQuality=quality*(profile.fruit&&!pollinated?.86:1.);m.item->cropDirt=bed->kind==MachineKind::FieldBed?.28:bed->kind==MachineKind::RaisedBed?.14:.035;m.item->cropFreshness=1;
   for(auto& part:m.item->food.parts){part.food.c[Reducing]*=1.+quality*.12;part.food.c[Acid]*=1.+(1.-quality)*.3;for(size_t a=0;a<part.food.aroma.size();a++)part.food.aroma[a]*=.75+quality*.4;}
   visit(*m.item,bedPos);visit(*m.item,pos);m.item->matter=projectCell(m.item->food);m.ready=true;m.progress=0;bed->farmLastCrop=bed->ingredient;bed->farmPlanted=false;bed->farmGrowth=0;bed->farmAge=0;bed->farmN=std::max(bed->farmN,.28);m.status="Harvested "+ingredient(bed->ingredient).name+" / "+std::to_string(int(grams))+" g / grade "+(quality>.8?"A":quality>.55?"B":"C");powerUsed+=machineSpec(m.kind).power;energy+=machineSpec(m.kind).power;usePower(pos,machineSpec(m.kind).power);waterUsed+=machineSpec(m.kind).water;water=std::max(0.,water-machineSpec(m.kind).water);continue;
  }
  if(m.kind==MachineKind::ProduceIntake){if(!m.item){m.status="Awaiting washed crop batch";continue;}auto key=batchCropKey(*m.item);if(key.empty()){m.status="Needs a named produce crop";continue;}m.ingredient=key;if(m.forward){m.ready=true;m.status="Crop queued / send to factory";continue;}double mass=analyse(m.item->food).mass,kg=std::max(0.,mass/1000.),old=farmInventory[key],oldQ=farmQuality.count(key)?farmQuality[key]:.7;double quality=clamp(m.item->cropQuality*m.item->cropFreshness*(1.-m.item->cropDirt*.18),0,1);farmInventory[key]+=kg;farmProduce+=kg;farmQuality[key]=farmInventory[key]>0?(old*oldQ+kg*quality)/farmInventory[key]:quality;stock[key]+=kg*10;auto name=ingredient(key).name;m.ingredient=key;m.lastOutput=m.item;m.item.reset();m.progress=0;m.ready=false;m.status="Stored "+name+" / +"+roundTo(kg,2)+" kg / grade "+(quality>.8?"A":quality>.55?"B":"C");continue;}
  if(m.kind==MachineKind::CompostBin){if(!m.item){m.status="Ready for food leftovers";continue;}Machine* bed=nullptr;for(int d=0;d<4&&!bed;d++){auto nearby=at(nextPosition(pos,d));if(nearby&&isFarmBed(nearby->kind))bed=nearby;}if(!bed){m.status="Connect compost to a grow bed";continue;}if(m.progress==0&&water<machineSpec(m.kind).water){m.status="Water input empty";continue;}if(!grid.active&&!sandbox&&powerUsed+machineSpec(m.kind).power>powerCapacity){m.status="Power limited";continue;}if(!powerAvailable(pos,machineSpec(m.kind).power)){m.status="Electricity input empty";continue;}auto missing=consumeUtilities(pos,m);if(!missing.empty()){m.status=missing;continue;}chemistryStep(*m.item,m,true);m.progress++;powerUsed+=machineSpec(m.kind).power;energy+=machineSpec(m.kind).power;usePower(pos,machineSpec(m.kind).power);if(m.progress>=m.duration){bed->farmOrganic=std::min(1.,bed->farmOrganic+.18);bed->farmN=std::min(1.,bed->farmN+.10);bed->farmP=std::min(1.,bed->farmP+.06);bed->farmK=std::min(1.,bed->farmK+.10);bed->farmRotationStress=std::max(0.,bed->farmRotationStress-.18);m.item.reset();m.progress=0;m.ready=false;m.status="Compost returned nutrients and eased rotation pressure";}else m.status="Composting / "+std::to_string(m.progress)+" / "+std::to_string(m.duration);continue;}
  if(m.kind==MachineKind::RecipeKitchen){if(m.item||dish(m.recipe)){auto missing=consumeUtilities(pos,m);if(!missing.empty()){m.status=missing;continue;}}kitchenStep(m);if(m.item)visit(*m.item,pos);continue;}
  if(m.kind==MachineKind::Feeder&&m.automatic&&!m.item&&m.cooldown==0){auto missing=missingInputSupply(m.ingredient,m.dose);if(!sandbox&&stock[m.ingredient]<m.dose/100)m.status="Pantry empty";else if(!missing.empty())m.status=missing+" supply empty";else{m.item=makeBatch(m.ingredient,m.dose);visit(*m.item,pos);if(!sandbox)stock[m.ingredient]-=m.dose/100;consumeInputSupply(m.ingredient,m.dose);m.ready=true;m.cooldown=4;}}
  if(isTastingDock(m.kind)){if(factoryArea.count(pos)){m.status="Serve point must be outside factory walls";continue;}if(m.item&&vaporBatch(*m.item)){m.status="Needs condenser";continue;}if(m.forward){if(m.item){chemistryStep(*m.item,m,false);m.ready=true;m.status="Forwarding batch";}}else if(m.automatic)ship(m,pos);else if(m.item){chemistryStep(*m.item,m,false);m.status=m.ready?"Forward queued":m.serveStyle==ServeStyle::ChilledPickup&&analyse(m.item->food).temperature>8?"Waiting for <= 8 C cold pickup":"Holding for tasting";}continue;}
  if(m.kind==MachineKind::Bin){if(m.item){wasted++;m.item.reset();m.status="Reclaimed batch";}continue;}
  if(!m.item)continue;
  if(m.kind==MachineKind::LatticeReactor){
   if(m.ready){m.status="Ready";continue;}
   if(m.latticeBatch!=m.item->id||m.lattice.size()!=25)rebuildLattice(m,*m.item);
   if(m.latticePending<0){if(m.latticeRelease){m.ready=true;m.latticeRelease=false;m.status="Ready";}else m.status="Awaiting match";continue;}
   Technique technique;std::string reason;if(!latticeReaction(m,*m.item,m.latticePending,technique,reason)){m.status=reason;continue;}
   if(!sandbox&&m.wear>=1){m.status="Needs cleaning";continue;}if(!grid.active&&!sandbox&&powerUsed+spec.power>powerCapacity){m.status="Power limited";continue;}if(water+1e-9<spec.water){m.status="Water supply empty";continue;}if(!sandbox&&cash<spec.power*.035){m.status="Credits exhausted";continue;}if(!powerAvailable(pos,spec.power)){m.status="Electricity supply empty";continue;}if(auto missing=consumeUtilities(pos,m);!missing.empty()){m.status=missing;continue;}
   Environment reaction{technique,m.temperature,m.shear,1};std::vector<uint64_t> targets;for(const auto& part:m.item->food.parts)targets.push_back(part.id);processBatch(m.item->food,reaction,targets,nextOperation++);m.item->age++;m.item->matter=projectCell(m.item->food);for(const auto& part:m.item->food.parts)for(const auto& event:part.food.events)discoveries.insert(event);
   powerUsed+=spec.power;energy+=spec.power;usePower(pos,spec.power);waterUsed+=spec.water;water=std::max(0.,water-spec.water);if(!sandbox)cash-=spec.power*.035;m.wear=clamp(m.wear+.0015*(1+meanCompound(*m.item,"fat")+meanCompound(*m.item,"protein")),0,1.2);
   static const std::array<std::string,LatticeFamilyCount> names={"sugar","protein","fat","hydrocolloid","starch","acid / base","calcium","lecithin"};int family=m.latticePending;m.latticeMoves++;m.latticePending=-1;rebuildLattice(m,*m.item);m.status="Reacted: "+names[size_t(family)];if(m.latticeRelease){m.ready=true;m.latticeRelease=false;m.status="Reaction complete / ready to release";}continue;
  }
  bool active=isProcessingMachine(m.kind)&&!m.ready;
  if(m.kind==MachineKind::FractionatingColumn&&!vaporBatch(*m.item)){active=false;m.status="Needs vapour";}
  if(m.kind==MachineKind::FractionatingStill&&vaporBatch(*m.item)){active=false;m.status="Needs liquid feed";}
  if((isMixingMachine(m.kind)||m.kind==MachineKind::Plater)&&m.progress==0){if(m.extra)merge(m);else{active=false;m.status="Needs second input";}}
  if(active&&!sandbox&&m.wear>=1){active=false;m.status="Needs cleaning";}
  if(active&&!grid.active&&!sandbox&&powerUsed+spec.power>powerCapacity){active=false;m.status="Power limited";}
  if(active&&water+1e-9<spec.water){active=false;m.status="Water supply empty";}
  if(active&&!sandbox&&cash<spec.power*.035){active=false;m.status="Credits exhausted";}
  if(active&&!powerAvailable(pos,spec.power)){active=false;m.status="Electricity supply empty";}
  if(active){auto missing=consumeUtilities(pos,m);if(!missing.empty()){active=false;m.status=missing;}}
  bool dose=isDosingMachine(m.kind)&&m.progress==0;
  if(active&&dose&&!sandbox&&stock[m.ingredient]<m.dose/100){active=false;m.status="Dosing ingredient empty";}
  if(active&&dose){auto missing=missingInputSupply(m.ingredient,m.dose);if(!missing.empty()){active=false;m.status=missing+" supply empty";}}
  if(active){
   if(dose){if(!sandbox)stock[m.ingredient]-=m.dose/100;consumeInputSupply(m.ingredient,m.dose);addIngredient(m.item->food,m.ingredient,m.dose,m.item->food.parts.front().id,nextOperation++);}
   powerUsed+=spec.power;energy+=spec.power;usePower(pos,spec.power);waterUsed+=spec.water;water=std::max(0.,water-spec.water);if(!sandbox)cash-=spec.power*.035;
   double fraction=grid.active?clamp(m.power.voltage,0,1):1.;m.power.workCredit+=fraction;bool workTick=m.power.workCredit+1e-9>=1;if(workTick){m.power.workCredit=std::max(0.,m.power.workCredit-1);chemistryStep(*m.item,m,true);}else chemistryStep(*m.item,m,false);if(workTick&&m.item->cropBatch){if(m.kind==MachineKind::Washer)m.item->cropDirt=std::max(0.,m.item->cropDirt-.24/std::max(1,m.duration));if(m.kind==MachineKind::ColdStore){m.item->cropFreshness=std::min(1.,m.item->cropFreshness+.02);for(auto& part:m.item->food.parts)part.food.temperature=std::max(m.temperature,part.food.temperature-18);}if(m.kind==MachineKind::Grader)m.item->cropQuality=clamp(m.item->cropQuality-.001*(m.item->cropDirt*2+(1.-m.item->cropFreshness)),0,1);}if(workTick)m.progress++;m.wear=clamp(m.wear+.0015*(1+meanCompound(*m.item,"fat")+meanCompound(*m.item,"protein")),0,1.2);m.status="Processing";
   if(m.progress>=m.duration){
    bool separate=m.kind==MachineKind::Centrifuge||m.kind==MachineKind::Filter||m.kind==MachineKind::FractionatingStill||m.kind==MachineKind::FractionatingColumn;
    if(separate){auto& b=*m.item;double volatileMass=0;for(auto& part:b.food.parts)volatileMass+=part.food.c[Water]+part.food.c[Ethanol]+part.food.c[Acid]+part.food.c[CO2];
     bool canSeparate=(m.kind!=MachineKind::FractionatingStill||volatileMass>.05)&&(m.kind!=MachineKind::FractionatingColumn||vaporBatch(b));
     if(canSeparate){std::vector<uint64_t> sourceParts;for(auto& part:b.food.parts)sourceParts.push_back(part.id);std::set<uint64_t> sideParts;
      for(auto id:sourceParts){auto it=std::find_if(b.food.parts.begin(),b.food.parts.end(),[&](const FoodPart& part){return part.id==id;});if(it==b.food.parts.end())continue;double partVolatile=it->food.c[Water]+it->food.c[Ethanol]+it->food.c[Acid]+it->food.c[CO2];if(m.kind==MachineKind::FractionatingStill&&partVolatile<=.005)continue;uint64_t main=nextComponent++,side=nextComponent++;splitPart(b.food,id,main,side,environment(m),nextOperation++);sideParts.insert(side);}
      if(!sideParts.empty()){Batch z;z.id=nextId++;z.age=b.age;z.route=b.route;z.residue=b.residue;z.cropBatch=b.cropBatch;z.cropQuality=b.cropQuality;z.cropDirt=b.cropDirt;z.cropFreshness=b.cropFreshness;z.food.history=b.food.history;for(auto& p:b.food.parts)if(sideParts.count(p.id))z.food.parts.push_back(p);b.food.parts.erase(std::remove_if(b.food.parts.begin(),b.food.parts.end(),[&](const FoodPart& p){return sideParts.count(p.id);}),b.food.parts.end());z.matter=projectCell(z.food);b.matter=projectCell(b.food);m.sideOutput=z;}
      if(m.kind==MachineKind::FractionatingStill)m.status=upper(distillationCutName(m.cut))+" cut / stillage";else if(m.kind==MachineKind::FractionatingColumn)m.status="Enriched vapour / reflux";else m.status="Separated / two fractions";
     }else if(m.kind==MachineKind::FractionatingStill)m.status="Spent / stillage";else if(m.kind==MachineKind::FractionatingColumn)m.status="Vapour depleted";
    }
    m.ready=true;if(m.kind==MachineKind::Washer&&m.item->cropBatch){m.item->cropDirt=0;m.status="Crop washed / soil removed";}else if(m.kind==MachineKind::Grader&&m.item->cropBatch){m.status="Grade "+std::string(m.item->cropQuality>.8?"A":m.item->cropQuality>.55?"B":"C");}else if(m.kind==MachineKind::ColdStore&&m.item->cropBatch)m.status="Fresh crop chilled / "+roundTo(m.item->cropFreshness*100,0)+"% freshness";else if(m.status=="Processing")m.status="Ready";
   }
  }else chemistryStep(*m.item,m,false);
  if(m.extra)chemistryStep(*m.extra,m,false);
  if(isTransport(m.kind)||m.kind==MachineKind::Feeder)m.ready=true;
 }
 // Tail-first chain resolution moves each original batch at most one tile per tick.
 std::map<Position,uint64_t> pending;for(auto& [p,m]:machines)if(m.enabled&&m.item&&m.ready)pending[p]=m.item->id;
 while(!pending.empty()){
  Position cursor=pending.begin()->first;std::vector<Position> chain;std::set<Position> seen;bool open=false;
  while(true){auto original=pending.find(cursor);auto src=at(cursor);if(original==pending.end()||!src||!src->item||src->item->id!=original->second)break;if(!seen.insert(cursor).second)break;chain.push_back(cursor);
   Position target=nextPosition(cursor,src->dir);auto dst=at(target);if(!dst||!dst->enabled||dst->kind==MachineKind::Wall||(isEnergyMachine(dst->kind)&&!acceptsEnergyFuel(dst->kind))||dst->kind==MachineKind::Feeder||dst->kind==MachineKind::RecipeKitchen||isUtilityEquipment(dst->kind)||isSensor(dst->kind)||isFarmInfrastructure(dst->kind)||dst->kind==MachineKind::Harvester||dst->sideOutput)break;
   bool compatible=!(dst->kind==MachineKind::FractionatingColumn&&!vaporBatch(*src->item))&&!(dst->kind==MachineKind::FractionatingStill&&vaporBatch(*src->item))&&!(isTastingDock(dst->kind)&&vaporBatch(*src->item))&&!(dst->kind==MachineKind::ProduceIntake&&!src->item->cropBatch);
   bool can=compatible&&(!dst->item||((isMixingMachine(dst->kind)||dst->kind==MachineKind::Plater)&&dst->progress==0&&!dst->ready&&!dst->extra&&dst->item->food.parts.size()+src->item->food.parts.size()<=16));
   if(can){open=true;break;}if(!pending.count(target))break;cursor=target;
  }
  if(chain.empty()){pending.erase(pending.begin());continue;}
  for(auto it=chain.rbegin();it!=chain.rend();++it){auto src=at(*it);auto dst=at(nextPosition(*it,src->dir));if(open){auto item=std::move(*src->item);visit(item,nextPosition(*it,src->dir));src->item.reset();src->ready=false;src->progress=0;if(src->kind==MachineKind::LatticeReactor){src->lattice.clear();src->latticeBatch=0;src->latticePending=-1;src->latticeRelease=false;}accept(*dst,std::move(item));src->status="Flowing";}else if(dst&&dst->kind==MachineKind::FractionatingColumn&&src->item&&!vaporBatch(*src->item))src->status="Needs vapour";else if(dst&&isTastingDock(dst->kind)&&src->item&&vaporBatch(*src->item))src->status="Needs condenser";else if(dst&&dst->kind==MachineKind::FractionatingStill&&src->item&&vaporBatch(*src->item))src->status="Needs liquid feed";else src->status=dst?"Backed up":"No outlet";pending.erase(*it);}
 }
 // Side fractions were created or held before transport; they never join the main move set.
 for(auto& [p,m]:machines)if(m.enabled&&m.sideOutput){auto dst=at(nextPosition(p,(m.dir+1)%4));Batch side=*m.sideOutput;visit(side,nextPosition(p,(m.dir+1)%4));if(dst&&dst->enabled&&accept(*dst,std::move(side)))m.sideOutput.reset();else m.status="Side outlet blocked";}
 for(auto& [p,utility]:machines)if(utility.enabled&&isUtilityEquipment(utility.kind)){const auto& grid=grids[size_t(utility.utility)];auto component=grid.component.find(p);if(utility.kind==MachineKind::UtilityStation){double used=component==grid.component.end()?0:grid.used[size_t(component->second)],capacity=component==grid.component.end()?utility.utilityCapacity:grid.capacity[size_t(component->second)];utility.status=utilityName(utility.utility)+" / "+roundTo(used,1)+" used of "+roundTo(capacity,0);}else utility.status=component!=grid.component.end()&&grid.capacity[size_t(component->second)]>0?utilityName(utility.utility)+" network":"Unfed "+utilityName(utility.utility)+" line";}
 for(auto& [p,sensor]:machines)if(isSensor(sensor.kind)&&sensor.enabled){Position target=nextPosition(p,sensor.dir);auto source=at(target);if(sensor.kind==MachineKind::SoilProbe){if(!source||!isFarmBed(source->kind)){sensor.sensorBatch=0;sensor.status="Point probe at a grow bed";continue;}sensor.sensorValue=source->farmMoisture;sensor.sensorTick=tick;sensor.sensorHistory.push_back(sensor.sensorValue);if(sensor.sensorHistory.size()>80)sensor.sensorHistory.erase(sensor.sensorHistory.begin());sensor.status="Moisture "+roundTo(source->farmMoisture*100,0)+"% / pH "+roundTo(source->farmPH,1)+" / NPK "+roundTo(source->farmN*100,0)+"-"+roundTo(source->farmP*100,0)+"-"+roundTo(source->farmK*100,0)+" / health "+roundTo(source->farmHealth*100,0)+"%";continue;}const Batch* batch=source?(source->item?&*source->item:source->extra?&*source->extra:source->sideOutput?&*source->sideOutput:nullptr):nullptr;if(!batch||batch->food.parts.empty()){sensor.sensorBatch=0;sensor.status="No batch at "+positionName(target);continue;}auto metrics=analyse(batch->food);double value=0;if(sensor.kind==MachineKind::PHProbe)value=metrics.pH;else if(sensor.kind==MachineKind::TemperatureLogger)value=metrics.temperature;else if(sensor.kind==MachineKind::AromaSensor)value=metrics.aroma;else{double total=0,mass=0;for(const auto& part:batch->food.parts){double grams=foodMass(part.food);total+=part.food.droplet*grams;mass+=grams;}value=mass>0?total/mass:0;}sensor.sensorValue=value;sensor.sensorBatch=batch->id;sensor.sensorTick=tick;sensor.sensorHistory.push_back(value);if(sensor.sensorHistory.size()>80)sensor.sensorHistory.erase(sensor.sensorHistory.begin());sensor.status=machineSpec(sensor.kind).name+" / "+roundTo(value,sensor.kind==MachineKind::PHProbe?2:1)+(sensor.kind==MachineKind::PHProbe?" pH":sensor.kind==MachineKind::TemperatureLogger?" C":sensor.kind==MachineKind::DropletViewer?" um":" aroma")+" / batch #"+std::to_string(batch->id);}
 outputHistory.push_back(shipped-before);if(outputHistory.size()>80)outputHistory.erase(outputHistory.begin());dirty=true;
 finishPowerStep();
}
int Factory::batches()const{int n=0;for(auto& [p,m]:machines){if(m.item)n++;if(m.extra)n++;if(m.sideOutput)n++;}return n;}
int Factory::processingMachines()const{int n=0;for(auto& [p,m]:machines)if(m.status=="Processing")n++;return n;}
static std::set<Position> enclosedFactoryTilesFor(const std::map<Position,Machine>& machines){
 std::set<Position> walls;for(const auto& [p,m]:machines)if(m.kind==MachineKind::Wall)walls.insert(p);if(walls.size()<3)return {};
 std::set<Position> boundary=walls;for(const auto& [p,m]:machines){if(m.kind==MachineKind::Wall||(!isTransport(m.kind)&&!isTastingDock(m.kind)))continue;bool horizontal=walls.count(nextPosition(p,0))&&walls.count(nextPosition(p,2));bool vertical=walls.count(nextPosition(p,1))&&walls.count(nextPosition(p,3));if(horizontal||vertical)boundary.insert(p);}
 if(boundary.size()<4)return {};
 std::map<Position,int> degree;std::queue<Position> prune;
 for(auto p:boundary){int n=0;for(int d=0;d<4;d++)if(boundary.count(nextPosition(p,d)))n++;degree[p]=n;if(n<2)prune.push(p);}
 std::set<Position> core=boundary;
 while(!prune.empty()){auto p=prune.front();prune.pop();if(!core.erase(p))continue;for(int d=0;d<4;d++){auto q=nextPosition(p,d);if(core.count(q)&&--degree[q]<2)prune.push(q);}}
 std::set<Position> enclosed,unseen=core;
 while(!unseen.empty()){
  Position first=*unseen.begin();unseen.erase(first);std::vector<Position> component{first};int minX=first.x,maxX=first.x,minY=first.y,maxY=first.y;
  for(size_t i=0;i<component.size();i++){auto p=component[i];minX=std::min(minX,p.x);maxX=std::max(maxX,p.x);minY=std::min(minY,p.y);maxY=std::max(maxY,p.y);for(int d=0;d<4;d++){auto q=nextPosition(p,d);auto it=unseen.find(q);if(it!=unseen.end()){component.push_back(q);unseen.erase(it);}}}
  int64_t width=int64_t(maxX)-minX+1,height=int64_t(maxY)-minY+1;if(width*height>100000)continue;
  std::set<Position> visited;
  for(int y=minY;y<=maxY;y++)for(int x=minX;x<=maxX;x++){
   Position start{x,y};if(boundary.count(start)||visited.count(start))continue;std::queue<Position> open;std::vector<Position> region{start};visited.insert(start);open.push(start);bool touchesEdge=false;
   while(!open.empty()){auto p=open.front();open.pop();if(p.x==minX||p.x==maxX||p.y==minY||p.y==maxY)touchesEdge=true;for(int d=0;d<4;d++){auto q=nextPosition(p,d);if(q.x<minX||q.x>maxX||q.y<minY||q.y>maxY||boundary.count(q)||!visited.insert(q).second)continue;region.push_back(q);open.push(q);}}
   if(!touchesEdge)enclosed.insert(region.begin(),region.end());
  }
 }
 return enclosed;
}
std::set<Position> Factory::enclosedFactoryTiles()const{return enclosedFactoryTilesFor(machines);}
bool Factory::captureBlueprint(Position a,Position b,const std::string& name){
 int left=std::min(a.x,b.x),top=std::min(a.y,b.y),right=std::max(a.x,b.x),bottom=std::max(a.y,b.y);
 if(int64_t(right)-left>1000||int64_t(bottom)-top>1000){notice="Select a smaller blueprint region.";return false;}
 Blueprint bp;bp.name=name.empty()?"Layout "+std::to_string(blueprints.size()+1):name;bp.width=right-left+1;bp.height=bottom-top+1;
 for(auto& [pos,m]:machines)if(pos.x>=left&&pos.x<=right&&pos.y>=top&&pos.y<=bottom){auto copy=m;copy.item.reset();copy.extra.reset();copy.sideOutput.reset();copy.lastOutput.reset();copy.program.clear();copy.programOutput.clear();copy.cursor=copy.subtick=0;copy.progress=copy.cooldown=0;copy.ready=false;copy.wear=0;clearPowerInventory(copy);copy.lattice.clear();copy.latticeBatch=0;copy.latticePending=-1;copy.latticeMoves=0;copy.latticeRelease=false;copy.sensorValue=0;copy.sensorBatch=0;copy.sensorTick=-1;copy.sensorHistory.clear();copy.status="Idle";bp.parts.push_back({{pos.x-left,pos.y-top},copy});}
 for(const auto& [pos,links]:energyLinks)if(pos.x>=left&&pos.x<=right&&pos.y>=top&&pos.y<=bottom)for(auto link:links){link.flow=link.heat=link.overload=0;link.tripped=false;bp.links.push_back({{pos.x-left,pos.y-top},link});}
 if(bp.parts.empty()&&bp.links.empty()){notice="No machines or energy routes in this selection.";return false;}if(blueprints.size()>=1000){notice="The process plan library is full. Reclaim an old plan first.";return false;}snapshot();blueprints.push_back(std::move(bp));dirty=true;notice="Process plan saved with machine settings, flow directions, side branches and utility routes. Choose it from Plans to stamp a copy.";return true;
}
void Factory::setPlanName(size_t index,const std::string& name){if(index>=blueprints.size()||name.empty())return;snapshot();blueprints[index].name=name.substr(0,64);dirty=true;notice="Process plan renamed: "+blueprints[index].name+".";}
Blueprint Factory::rotatedBlueprint(Blueprint b){for(auto& p:b.parts){p.at={b.height-1-p.at.y,p.at.x};p.machine.dir=(p.machine.dir+1)%4;}for(auto& p:b.links){p.at={b.height-1-p.at.y,p.at.x};p.link.dir=(p.link.dir+1)%4;p.link.ports=((p.link.ports<<1)|(p.link.ports>>3))&15;}std::swap(b.width,b.height);return b;}
bool Factory::canStamp(const Blueprint& b,Position origin)const{
 double cost=0;bool zoningChange=false,zonesPossible=false;for(const auto& [where,m]:machines)zonesPossible|=m.kind==MachineKind::Wall;for(const auto& p:b.parts){Position at={origin.x+p.at.x,origin.y+p.at.y};if(!validPosition(at)||this->at(at))return false;cost+=machineSpec(p.machine.kind).cost;zoningChange|=p.machine.kind==MachineKind::Wall||isTransport(p.machine.kind)||isTastingDock(p.machine.kind);zonesPossible|=p.machine.kind==MachineKind::Wall;}
 for(const auto& part:b.links){Position p{origin.x+part.at.x,origin.y+part.at.y};if(!validPosition(p)||energyLink(p,part.link.medium)||!validPowerLink(part.link))return false;cost+=3;}
 if(!sandbox&&cash<cost)return false;
 if(zoningChange&&zonesPossible){std::map<Position,Machine> candidate;for(const auto& [where,m]:machines){Machine cell;cell.kind=m.kind;candidate.emplace(where,std::move(cell));}for(const auto& p:b.parts){Machine cell;cell.kind=p.machine.kind;candidate.emplace(Position{origin.x+p.at.x,origin.y+p.at.y},std::move(cell));}auto area=enclosedFactoryTilesFor(candidate);for(const auto& [where,m]:candidate)if(isTastingDock(m.kind)&&area.count(where))return false;}
 return true;
}
bool Factory::stamp(const Blueprint& b,Position origin){
 if(!canStamp(b,origin)){notice="Blueprint overlaps, exceeds your economy budget, or would place or enclose a serve point inside factory walls.";return false;}snapshot();for(auto& p:b.parts){auto copy=p.machine;copy.item.reset();copy.extra.reset();copy.sideOutput.reset();copy.lastOutput.reset();copy.program.clear();copy.programOutput.clear();copy.cursor=copy.subtick=0;copy.progress=copy.cooldown=0;copy.ready=false;clearPowerInventory(copy);copy.lattice.clear();copy.latticeBatch=0;copy.latticePending=-1;copy.latticeMoves=0;copy.latticeRelease=false;copy.sensorValue=0;copy.sensorBatch=0;copy.sensorTick=-1;copy.sensorHistory.clear();machines.emplace(Position{origin.x+p.at.x,origin.y+p.at.y},copy);if(!sandbox)cash-=machineSpec(copy.kind).cost;}for(const auto& part:b.links){auto link=part.link;link.flow=link.heat=link.overload=0;link.tripped=false;energyLinks[{origin.x+part.at.x,origin.y+part.at.y}].push_back(link);if(!sandbox)cash-=3;}dirty=true;notice="Stamped "+b.name+" with its original settings, routes and utility topology.";return true;
}
bool Factory::setIngredient(Position p,const std::string& key){auto m=at(p);if(!m)return false;try{const auto& i=ingredient(key);if((m->kind==MachineKind::SeedHopper||isFarmBed(m->kind))&&i.category!="Produce")return false;snapshot();m->ingredient=key;if(m->kind!=MachineKind::SeedHopper)m->dose=i.dose;dirty=true;notice=i.name+(m->kind==MachineKind::SeedHopper?" seed selected.":" selected: "+roundTo(m->dose)+" g per batch.");return true;}catch(...){return false;}}
const DishDesign* Factory::dish(uint64_t id)const{for(auto& d:dishbook)if(d.id==id)return &d;return nullptr;}
bool Factory::setRecipe(Position p,uint64_t id){auto m=at(p);if(!m||!dish(id)||m->item)return false;snapshot();m->recipe=id;dirty=true;notice="Kitchen programmed: "+dish(id)->name;return true;}
uint64_t Factory::newComponent(){if(bench.batch.parts.size()>=16){notice="A plate can hold 16 separate components. Blend or clear a component first.";return 0;}snapshot();bench.selected=nextComponent++;dirty=true;return bench.selected;}
void Factory::clearBench(){snapshot();bench=DishBench{};dirty=true;}
void Factory::benchAdd(const std::string& key,double grams){if(grams<=0||grams>1000)return;if(!sandbox&&stock[key]<grams/100){notice="Not enough pantry stock for this dose.";return;}if(auto missing=missingInputSupply(key,grams);!missing.empty()){notice="Factory input empty: "+missing+". Replenish it in the inspector.";return;}snapshot();if(!bench.selected)bench.selected=nextComponent++;addIngredient(bench.batch,key,grams,bench.selected,nextOperation++);if(!sandbox)stock[key]-=grams/100;consumeInputSupply(key,grams);dirty=true;notice="Added "+roundTo(grams)+" g "+ingredient(key).name+". Process it or create another component.";}
void Factory::benchProcess(Environment e,int ticks){if(!bench.selected)return;snapshot();std::vector<uint64_t> ids={bench.selected};for(int n=0;n<std::clamp(ticks,1,120);n++)processBatch(bench.batch,e,ids,nextOperation++);dirty=true;notice=techniqueName(e.technique)+" complete. Taste the component and inspect its reaction notes.";}
void Factory::benchBlend(){if(bench.batch.parts.size()<2)return;snapshot();std::vector<uint64_t> ids;for(auto& p:bench.batch.parts)ids.push_back(p.id);bench.selected=nextComponent++;blendParts(bench.batch,ids,bench.selected,nextOperation++);dirty=true;notice="Components blended. Re-aerate or homogenise to build a new structure.";}
void Factory::benchPlate(bool all){
 if(bench.batch.parts.empty())return;CulinaryBatch plate;
 if(all){for(auto& op:bench.batch.history)if(!executeStep(plate,op))return;plate.history=bench.batch.history;if(plate.parts.size()>16)return;}
 else{for(auto& p:bench.batch.parts)if(p.id==bench.selected)plate.parts.push_back(p);if(plate.parts.empty())return;plate.history=bench.batch.history;}
 snapshot();bench.batch=std::move(plate);dirty=true;notice=all?"All recipe components restored to the plate.":"Selected component plated. Other fractions become kitchen side output; connect a route for them.";
}
void Factory::benchSeparate(Technique technique,int ticks){if(!bench.selected||bench.batch.parts.size()>=16)return;snapshot();for(int n=0;n<std::clamp(ticks,0,120);n++)processBatch(bench.batch,{technique,20,.8,1},{bench.selected},nextOperation++);auto a=nextComponent++,z=nextComponent++;splitPart(bench.batch,bench.selected,a,z,{technique,20,.5,1},nextOperation++);bench.selected=a;dirty=true;notice="Separated into two usable components. Select each fraction to continue experimenting.";}
void Factory::remap(CulinaryBatch& b,std::vector<uint64_t>* output){
 std::map<uint64_t,uint64_t> ids;auto mapped=[&](uint64_t id){if(!id)return uint64_t(0);auto i=ids.find(id);if(i==ids.end())i=ids.emplace(id,nextComponent++).first;return i->second;};
 for(auto& s:b.history){s.id=nextOperation++;s.result=mapped(s.result);s.side=mapped(s.side);for(auto& id:s.targets)id=mapped(id);}for(auto& p:b.parts)p.id=mapped(p.id);if(output)for(auto& id:*output)id=mapped(id);
}
uint64_t Factory::saveDish(const std::string& name){if(bench.batch.parts.empty()||bench.batch.history.empty()||dishbook.size()>=500)return 0;CulinaryBatch replay;for(auto& op:bench.batch.history)if(!executeStep(replay,op)||replay.parts.size()>16){notice="This recipe exceeds the 16-component staging limit or has an incomplete process history.";return 0;}std::string n=name.substr(0,64);if(n.empty())n="Dish "+std::to_string(nextDish);snapshot();DishDesign d;d.id=nextDish++;d.name=n;d.sample=bench.batch;for(auto& p:d.sample.parts)d.output.push_back(p.id);d.quality=analyse(d.sample).quality;dishbook.push_back(d);bench.name=n;dirty=true;notice="Saved "+n+". Use this recipe in a selected kitchen, or automate it on the same floor.";return d.id;}
bool Factory::loadDish(uint64_t id){auto d=dish(id);if(!d)return false;snapshot();bench.batch=d->sample;bench.name=d->name;remap(bench.batch);bench.selected=bench.batch.parts.empty()?0:bench.batch.parts.front().id;dirty=true;notice="Recipe opened on the plate. Edit it and save a new variation.";return true;}
bool Factory::sampleBatch(Position p){auto m=at(p);if(!m||!m->item||m->item->food.parts.empty()||!m->program.empty())return false;snapshot();bench.batch=m->item->food;bench.name=dish(m->item->dish)?dish(m->item->dish)->name:"Factory sample";remap(bench.batch);bench.selected=bench.batch.parts.front().id;dirty=true;notice="Factory sample copied into the visible plate with its complete process history.";return true;}
bool Factory::replayDish(uint64_t id,CulinaryBatch& output){auto d=dish(id);if(!d)return false;CulinaryBatch b;for(auto& s:d->sample.history)if(!executeStep(b,s))return false;b.history=d->sample.history;b.parts.erase(std::remove_if(b.parts.begin(),b.parts.end(),[&](const FoodPart& p){return std::find(d->output.begin(),d->output.end(),p.id)==d->output.end();}),b.parts.end());output=std::move(b);return !output.parts.empty();}
bool Factory::startKitchen(Machine& m){
 auto d=dish(m.recipe);if(!d){m.status="Choose a saved recipe";return false;}auto inputs=formula(d->sample);double chemicalNeed=0;for(auto& [key,g]:inputs){if(!sandbox&&stock[key]<g/100){m.status="Needs "+ingredient(key).name;return false;}if(auto missing=missingInputSupply(key,g);!missing.empty()){m.status=missing+" supply empty";return false;}if(ingredient(key).category=="Agent")chemicalNeed+=g/1000.;}
 if(chemicals+1e-9<chemicalNeed){m.status="Chemical supply empty";return false;}
 for(auto& [key,g]:inputs){if(!sandbox)stock[key]-=g/100;consumeInputSupply(key,g);}
 auto program=d->sample;auto outputs=d->output;remap(program,&outputs);m.program=program.history;m.programOutput=outputs;m.cursor=m.subtick=0;Batch b;b.id=nextId++;b.dish=d->id;m.item=b;m.ready=false;rawUsed+=int(inputs.size());return true;
}
void Factory::kitchenStep(Machine& m){
 if(m.ready)return;if(m.sideOutput){m.status="Side outlet blocked";return;}
 const auto& spec=machineSpec(m.kind);if(!sandbox&&(m.wear>=1||powerUsed+spec.power>powerCapacity||cash<spec.power*.035)){m.status="Check utilities / cleaning";return;}if(water+1e-9<spec.water){m.status="Water supply empty";return;}
 if(electricity+1e-9<spec.power){m.status="Electricity supply empty";return;}
 if(!m.item&&!startKitchen(m))return;
 powerUsed+=spec.power;energy+=spec.power;electricity=std::max(0.,electricity-spec.power);waterUsed+=spec.water;water=std::max(0.,water-spec.water);if(!sandbox)cash-=spec.power*.035;
 bool processed=false;int budget=256;while(m.cursor<int(m.program.size())&&budget-->0){auto s=m.program[m.cursor];if(s.action==RecipeAction::Process){if(processed)break;s.ticks=1;executeStep(m.item->food,s);m.subtick++;processed=true;if(m.subtick<m.program[m.cursor].ticks)break;m.subtick=0;}else if(!executeStep(m.item->food,s)){m.status="Recipe cannot be replayed";return;}m.cursor++;}
 m.item->age++;m.item->matter=projectCell(m.item->food);m.wear=clamp(m.wear+.0015,0,1.2);m.status="Recipe "+std::to_string(m.cursor)+" / "+std::to_string(m.program.size());
 if(m.cursor>=int(m.program.size())){auto& b=*m.item;b.food.history=m.program;Batch side;side.id=nextId++;side.age=b.age;side.food.history=m.program;for(auto& p:b.food.parts)if(std::find(m.programOutput.begin(),m.programOutput.end(),p.id)==m.programOutput.end())side.food.parts.push_back(p);
  b.food.parts.erase(std::remove_if(b.food.parts.begin(),b.food.parts.end(),[&](const FoodPart& p){return std::find(m.programOutput.begin(),m.programOutput.end(),p.id)==m.programOutput.end();}),b.food.parts.end());if(!side.food.parts.empty()){side.matter=projectCell(side.food);m.sideOutput=side;}b.matter=projectCell(b.food);m.ready=true;m.status="Dish ready";m.program.clear();m.programOutput.clear();m.cursor=m.subtick=0;
 }
}

static bool validBatch(const Batch& b){
 if(b.residue.size()>32)return false;
 if(!b.id||b.age<0||b.age>100000||b.matter.stack.size()>16||b.route.size()>100000||!std::is_sorted(b.route.begin(),b.route.end())||std::adjacent_find(b.route.begin(),b.route.end())!=b.route.end()||!std::isfinite(b.cropQuality)||b.cropQuality<0||b.cropQuality>1||!std::isfinite(b.cropDirt)||b.cropDirt<0||b.cropDirt>1||!std::isfinite(b.cropFreshness)||b.cropFreshness<0||b.cropFreshness>1||!validCulinary(b.food))return false;
 for(auto p:b.route)if(!validPosition(p))return false;
 auto finite=[](double v){return std::isfinite(v)&&std::abs(v)<1e7;};
 for(auto& p:b.matter.stack){if(p.kind!="food"||!data()["FOOD"].contains(p.k))return false;for(auto& k:materialKeys)if(!finite(p.at(k)))return false;for(double v:{p.temp,p.porosity,p.gel,p.foam,p.emulsion,p.crisp,p.brown,p.scorch,p.split,p.ice,p.integrity,p.sourced})if(!finite(v))return false;for(double v:p.aroma)if(!finite(v))return false;}
 for(double v:{b.matter.temp,b.matter.humidity,b.matter.pressure,b.matter.gas,b.matter.charge,b.matter.wave,b.matter.waste,b.matter.delivered})if(!finite(v))return false;for(double v:b.matter.aroma)if(!finite(v))return false;return true;
}
static bool validMachine(const Machine& m){
 if(!validPowerState(m.power))return false;
 if(m.dir<0||m.dir>3||m.duration<1||m.duration>120||m.progress<0||m.progress>10000||m.cooldown<0||m.cooldown>10000||!data()["FOOD"].contains(m.ingredient)||!std::isfinite(m.temperature)||m.temperature < -100||m.temperature>220||!std::isfinite(m.wear)||m.wear<0||m.wear>2)return false;
 if(!std::isfinite(m.dose)||m.dose<.1||m.dose>1000||!std::isfinite(m.shear)||m.shear<0||m.shear>2||m.cursor<0||m.cursor>int(m.program.size())||m.subtick<0||m.subtick>10000||m.program.size()>20000)return false;
 if(int(m.utility)<0||int(m.utility)>=int(UtilityKind::Count)||int(m.serveStyle)<0||int(m.serveStyle)>2||m.serviceFeedback.size()>1200||!std::isfinite(m.utilityCapacity)||m.utilityCapacity<0||m.utilityCapacity>200||!std::isfinite(m.sensorValue)||m.sensorTick < -1||m.sensorHistory.size()>256)return false;for(double sample:m.sensorHistory)if(!std::isfinite(sample))return false;
 if(m.lattice.size()!=0&&m.lattice.size()!=25)return false;if(m.latticePending < -1||m.latticePending>=LatticeFamilyCount||m.latticeMoves<0||m.latticeMoves>1000000)return false;for(int family:m.lattice)if(family < -1||family>=LatticeFamilyCount)return false;
 if(m.farmAge<0||m.farmAge>1000000||!std::isfinite(m.farmGrowth)||m.farmGrowth<0||m.farmGrowth>1.21||!std::isfinite(m.farmMoisture)||m.farmMoisture<0||m.farmMoisture>1||!std::isfinite(m.farmPH)||m.farmPH<0||m.farmPH>14||!std::isfinite(m.farmN)||m.farmN<0||m.farmN>1||!std::isfinite(m.farmP)||m.farmP<0||m.farmP>1||!std::isfinite(m.farmK)||m.farmK<0||m.farmK>1||!std::isfinite(m.farmOrganic)||m.farmOrganic<0||m.farmOrganic>1||!std::isfinite(m.farmHealth)||m.farmHealth<0||m.farmHealth>1||!std::isfinite(m.farmRotationStress)||m.farmRotationStress<0||m.farmRotationStress>1||m.farmLastCrop.size()>32)return false;
 if(isFarmBed(m.kind)&&ingredient(m.ingredient).category!="Produce")return false;
 if(!m.farmLastCrop.empty()&&ingredient(m.farmLastCrop).category!="Produce")return false;
 if(m.kind==MachineKind::LatticeReactor){if(m.latticeBatch&&(!m.item||m.latticeBatch!=m.item->id))return false;if(m.latticePending>=0&&(!m.item||m.lattice.size()!=25))return false;if(m.latticeRelease&&!m.item)return false;}else if(!m.lattice.empty()||m.latticeBatch||m.latticePending>=0||m.latticeRelease)return false;
 return (!m.item||validBatch(*m.item))&&(!m.extra||validBatch(*m.extra))&&(!m.sideOutput||validBatch(*m.sideOutput))&&(!m.lastOutput||validBatch(*m.lastOutput));
}
bool Factory::save()const{
 if(storageFile.empty())return true;
 try{std::error_code ec;std::filesystem::create_directories(storageFile.parent_path(),ec);if(ec)return false;auto temp=storageFile;temp+=".tmp";std::ofstream out(temp,std::ios::binary|std::ios::trunc);out<<json{{"version",11},{"factory",static_cast<const FactoryState&>(*this)}}.dump();out.close();if(!out)return false;std::filesystem::rename(temp,storageFile,ec);
#ifdef _WIN32
 if(ec){std::filesystem::remove(storageFile,ec);ec.clear();std::filesystem::rename(temp,storageFile,ec);}
#endif
 return !ec;}catch(const std::exception&){return false;}
}
bool Factory::load(){
 if(storageFile.empty()||!std::filesystem::exists(storageFile))return true;
 try{if(std::filesystem::file_size(storageFile)>64*1024*1024)return false;std::ifstream input(storageFile);json j;input>>j;int version=j.at("version");if(version<1||version>11)return false;auto state=j.at("factory").get<FactoryState>();
  if(version==1){
   auto upgrade=[&](Batch& b){b.food={};for(auto& z:b.matter.stack)if(z.kind=="food")addIngredient(b.food,z.k,100,state.nextComponent++,state.nextOperation++);b.matter=projectCell(b.food);};
   for(auto& [p,m]:state.machines){m.temperature=20+m.temperature*40;m.dose=ingredient(m.ingredient).dose;if(m.item)upgrade(*m.item);if(m.extra)upgrade(*m.extra);}for(auto& bp:state.blueprints)for(auto& part:bp.parts){part.machine.temperature=20+part.machine.temperature*40;part.machine.dose=ingredient(part.machine.ingredient).dose;}
  }
  bool recovered=false;
  if(version<3){for(auto& [pos,m]:state.machines)for(auto b:{m.item?&*m.item:nullptr,m.extra?&*m.extra:nullptr,m.sideOutput?&*m.sideOutput:nullptr})if(b&&b->route.empty())visit(*b,pos);
   if(!state.bench.batch.parts.empty()){Position p={7,2};while(state.machines.count(p)&&p.x<999999)p.x++;if(state.machines.count(p))return false;Machine dock=device(MachineKind::Dispatch);dock.automatic=false;Batch b;b.id=state.nextId++;b.food=state.bench.batch;b.matter=projectCell(b.food);visit(b,p);dock.item=b;dock.status="Recovered previous draft";state.machines.emplace(p,dock);state.bench=DishBench{};recovered=true;}
  }
  bool expandedOnLoad=false,demoInputsSeeded=false,farmDeliveryConnected=false,powerRoutesTidied=false;
  if(matchesStarterDemo(state.machines,false)){
   auto distillation=fractionalDistillationBlueprint();for(const auto& part:distillation.parts)state.machines.emplace(Position{-7+part.at.x,9+part.at.y},part.machine);
   if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[](const Blueprint& bp){return bp.name=="Fractional distillation train";}))state.blueprints.push_back(distillation);
   expandedOnLoad=true;
  }
  if(matchesStarterDemo(state.machines,true)){
   if(!interconnectStarterDemo(state.machines))return false;refreshExperimentBlueprint(state.blueprints);expandedOnLoad=true;
   if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[](const Blueprint& bp){return bp.name=="Interconnected demo factory";}))state.blueprints.push_back(starterDemoBlueprint(state.machines));
  }
  if(matchesStarterDemo(state.machines,true,true,false)){upgradeStarterLattice(state.machines);refreshExperimentBlueprint(state.blueprints);expandedOnLoad=true;}
  for(auto& i:pantry())if(!state.stock.count(i.key))state.stock[i.key]=40;for(const auto& crop:farmCropKeys())if(!state.seedStock.count(crop))state.seedStock[crop]=12;
  if(version<7){auto garden=marketGardenBlueprint();auto plan=std::find_if(state.blueprints.begin(),state.blueprints.end(),[](const Blueprint& bp){return bp.name=="Market garden / harvest to intake"||bp.name=="Northern market farm / crop rotation";});if(plan==state.blueprints.end())state.blueprints.push_back(garden);else *plan=garden;bool installed=migrateLegacyStarterGarden(state.machines);if(!installed&&matchesStarterDemo(state.machines,true,true,true)){bool free=true;for(const auto& part:garden.parts)if(state.machines.count({northernFarmOrigin.x+part.at.x,northernFarmOrigin.y+part.at.y})){free=false;break;}if(free)for(const auto& part:garden.parts)state.machines.emplace(Position{northernFarmOrigin.x+part.at.x,northernFarmOrigin.y+part.at.y},part.machine);installed=free;}if(installed)expandedOnLoad=true;}
  if(version<8){farmDeliveryConnected=connectNorthernFarmDelivery(state.machines);expandedOnLoad|=farmDeliveryConnected;if(std::none_of(state.blueprints.begin(),state.blueprints.end(),[](const Blueprint& bp){return bp.name=="Farm delivery conveyors";}))state.blueprints.push_back(farmDeliveryBlueprint());}
  if(version<9&&northernFarmAt(state.machines)){if(installPowerDistrictIn(state))expandedOnLoad=true;}
  if(version<11)powerRoutesTidied=tidyDefaultPowerRoutesIn(state);
  bool showExpandedDemo=matchesStarterDemo(state.machines,true,true,true)||northernFarmAt(state.machines);
  if(matchesStarterDemo(state.machines,true,true,true)){auto plan=std::find_if(state.blueprints.begin(),state.blueprints.end(),[](const Blueprint& bp){return bp.name=="Interconnected demo factory";});if(plan==state.blueprints.end()){state.blueprints.push_back(starterDemoBlueprint(state.machines));expandedOnLoad=true;}else if(expandedOnLoad)*plan=starterDemoBlueprint(state.machines);}
  bool noStarterFood=state.tick==0&&state.rawUsed==0&&state.shipped==0&&state.wasted==0&&state.shipments.empty()&&state.dishbook.empty()&&std::all_of(state.machines.begin(),state.machines.end(),[](const auto& entry){const auto& m=entry.second;return !m.item&&!m.extra&&!m.sideOutput&&!m.lastOutput;});
  if(showExpandedDemo&&noStarterFood){auto first=state.machines.find({-7,6}),second=state.machines.find({-5,4});if(first!=state.machines.end()&&second!=state.machines.end()&&first->second.kind==MachineKind::Feeder&&second->second.kind==MachineKind::Feeder&&data()["FOOD"].contains(first->second.ingredient)&&data()["FOOD"].contains(second->second.ingredient)&&first->second.dose>=.1&&first->second.dose<=1000&&second->second.dose>=.1&&second->second.dose<=1000&&(state.sandbox||(state.stock[first->second.ingredient]>=first->second.dose/100&&state.stock[second->second.ingredient]>=second->second.dose/100))){auto needed=[&](const Machine& feeder,const std::string& category){return ingredient(feeder.ingredient).category==category?feeder.dose/1000.:0.;};double farmNeed=needed(first->second,"Produce")+needed(first->second,"Dairy")+needed(second->second,"Produce")+needed(second->second,"Dairy");double chemicalNeed=needed(first->second,"Agent")+needed(second->second,"Agent");if(state.farmProduce+1e-9>=farmNeed&&state.chemicals+1e-9>=chemicalNeed){auto seed=[&](Position pos,Machine& feeder){Batch batch;batch.id=state.nextId++;addIngredient(batch.food,feeder.ingredient,feeder.dose,state.nextComponent++,state.nextOperation++);batch.matter=projectCell(batch.food);visit(batch,pos);feeder.item=std::move(batch);feeder.ready=true;if(!state.sandbox)state.stock[feeder.ingredient]-=feeder.dose/100;const auto& category=ingredient(feeder.ingredient).category;if(category=="Produce"||category=="Dairy")state.farmProduce=std::max(0.,state.farmProduce-feeder.dose/1000.);else if(category=="Agent")state.chemicals=std::max(0.,state.chemicals-feeder.dose/1000.);state.rawUsed++;};seed({-7,6},first->second);seed({-5,4},second->second);demoInputsSeeded=true;expandedOnLoad=true;}}}
  if(!state.nextComponent||!state.nextOperation||!state.nextDish||!validCulinary(state.bench.batch)||state.bench.name.size()>64||state.dishbook.size()>500)return false;
  std::set<uint64_t> dishes;for(auto& d:state.dishbook){if(!d.id||d.id>=state.nextDish||!dishes.insert(d.id).second||d.name.empty()||d.name.size()>64||!validCulinary(d.sample)||d.output.empty()||d.output.size()>16||!std::isfinite(d.quality))return false;CulinaryBatch replay;for(auto& op:d.sample.history){if(!executeStep(replay,op)||replay.parts.size()>16)return false;}for(auto id:d.output){bool found=false;for(auto& p:replay.parts)if(p.id==id)found=true;if(!found)return false;}}
  if(state.machines.size()>100000||state.tick<0||state.request<0||state.request>=int(factoryRequests().size())||state.requestProgress<0||state.requestProgress>=factoryRequests()[state.request].need||state.requestDeadline<0||state.requestDeadline>1000000000||state.shipped<0||state.wasted<0||state.nextId==0||state.blueprints.size()>1000||state.shipments.size()>40||state.outputHistory.size()>80)return false;
  for(double v:{state.cash,state.water,state.powerCapacity,state.electricity,state.farmProduce,state.chemicals,state.energy,state.waterUsed,state.powerUsed,state.revenue})if(!std::isfinite(v)||v<0||v>1e12)return false;
  std::set<uint64_t> ids;uint64_t maxId=0;for(auto& [p,m]:state.machines){if(!validPosition(p)||!validMachine(m))return false;for(auto b:{m.item?&*m.item:nullptr,m.extra?&*m.extra:nullptr,m.sideOutput?&*m.sideOutput:nullptr})if(b){if(!ids.insert(b->id).second)return false;maxId=std::max(maxId,b->id);}}
  for(auto& [pos,m]:state.machines)if(m.lastOutput)maxId=std::max(maxId,m.lastOutput->id);
  if(state.nextId<=maxId)return false;
  auto validIds=[&](const CulinaryBatch& b){for(auto& p:b.parts)if(p.id>=state.nextComponent)return false;for(auto& op:b.history){if(!op.id||op.id>=state.nextOperation||op.result>=state.nextComponent||op.side>=state.nextComponent)return false;for(auto id:op.targets)if(!id||id>=state.nextComponent)return false;}return true;};
  if(!validIds(state.bench.batch))return false;for(auto& d:state.dishbook)if(!validIds(d.sample))return false;
  for(auto& [pos,m]:state.machines){for(auto b:{m.item?&*m.item:nullptr,m.extra?&*m.extra:nullptr,m.sideOutput?&*m.sideOutput:nullptr,m.lastOutput?&*m.lastOutput:nullptr})if(b&&!validIds(b->food))return false;CulinaryBatch program;program.history=m.program;if(!validIds(program))return false;if(m.kind!=MachineKind::RecipeKitchen&&!m.program.empty())return false;}

  auto validPlanLinks=[](const Blueprint& bp){if(bp.links.size()>200000)return false;std::set<std::pair<Position,EnergyMedium>> seen;for(const auto& node:bp.links)if(node.at.x<0||node.at.y<0||node.at.x>=bp.width||node.at.y>=bp.height||!validPowerLink(node.link)||!seen.insert({node.at,node.link.medium}).second)return false;return true;};
  for(double value:{state.grid.generated,state.grid.requested,state.grid.delivered,state.grid.loss,state.grid.stored,state.grid.recoveredHeat,state.grid.ventedMass})if(!std::isfinite(value)||value<0||value>1e12)return false;
  for(auto& p:state.blueprints){if(!validPlanLinks(p))return false;if(p.name.size()>120||p.width<1||p.height<1||p.width>1001||p.height>1001||p.parts.size()>100000)return false;std::set<Position> locations;for(auto& q:p.parts)if(!validMachine(q.machine)||q.machine.item||q.machine.extra||q.machine.sideOutput||q.machine.lastOutput||q.at.x<0||q.at.y<0||q.at.x>=p.width||q.at.y>=p.height||!locations.insert(q.at).second)return false;}
  if(state.dishLayouts.size()>500)return false;for(auto& [id,bp]:state.dishLayouts){if(!validPlanLinks(bp))return false;if(!dishes.count(id)||bp.name.size()>120||bp.width<1||bp.height<1||bp.width>1001||bp.height>1001||bp.parts.empty()||bp.parts.size()>100000)return false;std::set<Position> locations;for(auto& node:bp.parts)if(!validMachine(node.machine)||node.machine.item||node.machine.extra||node.machine.sideOutput||node.machine.lastOutput||node.machine.kind==MachineKind::RecipeKitchen||node.at.x<0||node.at.y<0||node.at.x>=bp.width||node.at.y>=bp.height||!locations.insert(node.at).second)return false;}
  for(auto& [k,v]:state.stock)if(!data()["FOOD"].contains(k)||!std::isfinite(v)||v<0||v>100000000)return false;for(auto& k:data()["SETS"]["food"])if(!state.stock.count(k.get<std::string>()))return false;
  for(auto& [k,p]:state.products)if(k.size()>1000||p.name.size()>2000||p.phase.size()>30||p.count<0||!std::isfinite(p.best)||p.best<0||p.best>100||!std::isfinite(p.totalQuality)||!std::isfinite(p.totalValue))return false;
  for(const auto& [key,kg]:state.farmInventory)if(!std::isfinite(kg)||kg<0||kg>1e8||ingredient(key).category!="Produce")return false;for(const auto& [key,q]:state.farmQuality)if(!std::isfinite(q)||q<0||q>1||ingredient(key).category!="Produce")return false;for(const auto& [key,count]:state.seedStock)if(count<0||count>1000000||ingredient(key).category!="Produce")return false;
  if(state.energyLinks.size()>200000)return false;for(const auto& [p,links]:state.energyLinks){if(std::abs(p.x)>1000000||std::abs(p.y)>1000000||links.size()>EnergyCount)return false;std::set<EnergyMedium> kinds;for(const auto& link:links)if(!validPowerLink(link)||!kinds.insert(link.medium).second)return false;}
  static_cast<FactoryState&>(*this)=std::move(state);history.clear();running=false;dirty=version<11||expandedOnLoad;expandedStarterDemo=showExpandedDemo;notice=version==1?"Older factory migrated. Layout, stocks and records retained; in-flight ingredients rebuilt for the expanded chemistry. Factory paused.":"Workspace loaded exactly as saved. Paused for inspection.";if(demoInputsSeeded)notice="Starter demo staged with real tomato and olive-oil batches. Press Space to route them through the molecular lattice reactor.";else if(expandedOnLoad)notice="Workspace upgraded with the northern six-crop farm. Existing batches, crops and routes were preserved.";if(version<9&&std::any_of(machines.begin(),machines.end(),[](const auto& entry){return entry.second.kind==MachineKind::SolarPanel;}))notice="Workspace upgraded with the western power district. Existing food, growing crops and imported energy were preserved.";if(farmDeliveryConnected)notice="Northern farm connected to the factory by two real conveyor lines. Produce intakes now send harvested batches onward; crops, food and factory zoning are preserved.";if(powerRoutesTidied)notice="Demo wiring tidied into shared service trunks and short branches. Machines, food, crops and stored energy were preserved.";if(recovered)notice="Previous draft recovered in a holding tasting dock near 7, 2. All new experiments use machines on the floor.";return true;
 }catch(const std::exception&){notice="Could not read this factory save. The current factory was preserved.";return false;}
}
} // namespace aftertaste
