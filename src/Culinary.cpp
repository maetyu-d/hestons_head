#include "Culinary.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace aftertaste {
const std::vector<Ingredient>& pantry(){
 static const auto list=[] {
  std::vector<Ingredient> v;
  auto add=[&](const char* key,const char* name,const char* code,const char* color,const char* category,const char* description,std::initializer_list<std::pair<Compound,double>> fractions,Scent aroma={},double bitter=0,double umami=0,double buffer=0,double dose=100,double cost=2,double yeast=0){
   Ingredient i;i.key=key;i.name=name;i.code=code;i.color=color;i.category=category;i.description=description;i.aroma=aroma;i.bitter=bitter;i.umami=umami;i.buffer=buffer;i.dose=dose;i.cost=cost;i.yeast=yeast;
   for(auto [k,n]:fractions)i.fractions[k]=n;double sum=std::accumulate(i.fractions.begin(),i.fractions.end(),0.);if(sum<1)i.fractions[Other]+=1-sum;else if(sum>1)for(auto& n:i.fractions)n/=sum;v.push_back(i);
  };
  add("syrup","Syrup","SY","#ff6338","Base","Concentrated sucrose. Acid and heat invert it into reducing sugars; low-water heat caramelises it.",{{Water,.28},{Sucrose,.70},{Acid,.002}},{{0,.4,.05,0,.1,0}});
  add("brine","Brine","BR","#03b7d9","Base","Water and salt. Ionic strength changes protein behaviour, fermentation and gelation.",{{Water,.963},{Salt,.03},{Protein,.002}},{{.5,0,0,0,0,0}});
  add("broth","Broth","BO","#7960cf","Base","Concentrated savoury protein and amino acids. Reducing sugar and dry heat develop roast aromas.",{{Water,.80},{Protein,.12},{Amino,.03},{Fat,.025},{Salt,.015}},{{.1,0,.35,.1,0,0}},0,.12,.025,100,4);
  add("citrus","Citrus juice","CT","#c4ce27","Produce","Acidic juice. Low pH inhibits alginate setting, helps pectin and can curdle dairy proteins.",{{Water,.90},{Reducing,.025},{Sucrose,.015},{Acid,.055},{Fibre,.005}},{{0,.9,0,0,.05,.2}});
  add("cream","Cream","CM","#ffbf48","Dairy","Fat, water and a little protein. Emulsify, freeze, acidify or ferment to explore different structures.",{{Water,.59},{Fat,.35},{Protein,.024},{Reducing,.03},{Calcium,.001}},{{0,.05,.1,0,.45,0}},0,.01,.025,100,4);
  add("cocoa","Cocoa","CO","#495761","Aroma","Bitter particles with fat, protein and starch. Milling reduces grittiness; fat carries roast aroma.",{{Water,.03},{Fat,.18},{Protein,.20},{Starch,.10},{Fibre,.30},{Reducing,.05}},{{0,0,.9,.35,.1,0}},.16,.01);
  add("water","Water","WA","#8ecedc","Base","Dilutes solutes, changes water activity and supplies hydration for hydrocolloids.",{{Water,1}});
  add("milk","Milk","ML","#e8e1ce","Dairy","Buffered dairy with protein, lactose and calcium. Acid and heat can form curds.",{{Water,.87},{Fat,.04},{Protein,.035},{Reducing,.05},{Calcium,.001}},{{0,.02,.05,0,.3,0}},0,.01,.035);
  add("eggwhite","Egg white","EW","#dedbc2","Dairy","Protein-rich liquid. Partial denaturation strengthens foams; too much heat coagulates it.",{{Water,.88},{Protein,.105},{Salt,.003}},{{0,0,.03,0,.05,0}},0,.02,.015);
  add("yolk","Egg yolk","EY","#e8a935","Dairy","Fat, protein and natural lecithin. A useful emulsifier for sauces and custards.",{{Water,.48},{Fat,.32},{Protein,.16},{Lecithin,.012},{Salt,.004}},{{0,0,.1,0,.2,0}},0,.04,.02);
  add("butter","Butter","BT","#efcc77","Dairy","Mostly fat. Dispersing it into water needs shear and an interfacial stabiliser.",{{Water,.16},{Fat,.82},{Protein,.01},{Salt,.005}},{{0,0,.15,0,.65,0}});
  add("oil","Olive oil","OL","#a6ad54","Base","Liquid fat with herbal aroma. Emulsion stability depends on droplet size and surfactants.",{{Fat,.997}},{{0,.1,0,.7,0,.2}},.015);
  add("coconut","Coconut milk","CN","#dce2d0","Dairy","A plant-based fat and water dispersion. Homogenisation can make it smoother.",{{Water,.73},{Fat,.20},{Protein,.025},{Sucrose,.035}},{{0,.2,.1,.05,.6,0}},0,.01,.01);
  add("tomato","Tomato juice","TM","#d95a43","Produce","Acid, natural sugar, umami and suspended particles. Clarify it or build a savoury foam.",{{Water,.93},{Acid,.006},{Reducing,.025},{Protein,.01},{Amino,.006},{Fibre,.015}},{{.05,.25,.02,.35,0,.3}},0,.1,.01);
  add("berry","Strawberry puree","ST","#d35c81","Produce","Fruit sugars, acidity and pectin. Sugar level and pH change its setting behaviour.",{{Water,.87},{Sucrose,.035},{Reducing,.06},{Acid,.015},{Pectin,.006},{Fibre,.012}},{{0,1,0,.05,.1,.4}});
  add("beet","Beet puree","BE","#b85879","Produce","Sweet earthy plant material with fibre. Mill for smoothness or dry into a garnish.",{{Water,.86},{Reducing,.10},{Protein,.015},{Fibre,.02},{Acid,.002}},{{0,.15,.05,.9,0,.1}},.005,.02);
  add("mushroom","Mushroom puree","MU","#9b927a","Produce","Amino acids and savoury particles. Enzymes and heat offer different routes to flavour.",{{Water,.88},{Protein,.045},{Amino,.025},{Fibre,.035},{Reducing,.008}},{{.1,0,.25,1,0,0}},.01,.2,.025);
  add("basil","Basil leaves","BA","#54a96e","Produce","Aromatic leafy herb. Moderate light and steady moisture preserve fresh green aroma; heat and drying concentrate it.",{{Water,.91},{Fibre,.035},{Protein,.025},{Reducing,.012},{Acid,.002}},{{.05,.15,0,.18,.95,.05}},.005,.025);
  add("greens","Tender greens","GR","#72b56b","Produce","Fast-growing leafy crop. Mild nutrient stress can deepen flavour, while drought quickly lowers yield and crispness.",{{Water,.94},{Fibre,.025},{Protein,.025},{Reducing,.006}},{{.05,.15,0,.45,.2,.05}},.003,.015);
  add("carrot","Carrot","CR","#e68c3c","Produce","Sweet root crop that benefits from loose, moist soil and balanced potassium. Roasting develops a strong sweet aroma.",{{Water,.88},{Reducing,.055},{Sucrose,.025},{Fibre,.025},{Protein,.008}},{{0,.35,.05,.25,.05,.08}},.005,.02);
  add("pepper","Sweet pepper","PE","#df6250","Produce","Flowering fruit with crisp water-rich flesh. Pollination improves yield; warmth and bright light build sweetness and aroma.",{{Water,.92},{Reducing,.035},{Sucrose,.018},{Fibre,.018},{Acid,.003}},{{.02,.32,.04,.18,.2,.22}},.003,.02);
  add("coffee","Coffee extract","CF","#805d4a","Aroma","Roast aroma and bitterness in water. Concentration and volatile loss matter.",{{Water,.985},{Acid,.003},{Protein,.002}},{{0,0,1,.15,0,.1}},.09);
  add("tea","Tea extract","TE","#779864","Aroma","Herbal volatiles and tannic bitterness. Clarification and cold infusions preserve different notes.",{{Water,.995},{Acid,.001}},{{0,.1,.05,.8,0,.35}},.035);
  add("vanilla","Vanilla extract","VN","#be9d69","Aroma","Aromatic extract in water and alcohol. Heat and vacuum can strip its volatiles.",{{Water,.65},{Ethanol,.30},{Sucrose,.03}},{{0,.1,.1,0,1,.15}},.005);
  add("wine","Wine","WI","#9a5b7b","Aroma","Acid, ethanol and fruit aroma. Alcohol lowers the freezing point and alters emulsions.",{{Water,.85},{Ethanol,.12},{Acid,.006},{Reducing,.015}},{{0,.7,.15,.25,.1,.2}},.01);
  add("honey","Honey","HN","#ce9b37","Base","Mostly reducing sugars, so it browns differently from pure sucrose.",{{Water,.18},{Reducing,.68},{Sucrose,.12},{Acid,.003}},{{0,.4,.15,.05,.3,.35}});
  add("glucose","Glucose","GU","#e7dcc4","Agent","Reducing sugar. Participates directly in Maillard reactions and depresses the freezing point.",{{Reducing,.98},{Water,.02}}, {},0,0,0,10);
  add("sugar","Sucrose","SU","#eee9df","Agent","Non-reducing sugar until hydrolysed. Controls sweetness, water activity and pectin setting.",{{Sucrose,.999},{Water,.001}},{},0,0,0,10);
  add("salt","Salt","SA","#bbd7df","Agent","Increases ionic strength and changes protein, gel and fermentation behaviour.",{{Salt,.999},{Water,.001}},{},0,0,0,1);
  add("alginate","Sodium alginate","AL","#49b18c","Agent","Hydrates in water and crosslinks with available calcium. Acid and high ionic strength can interfere.",{{Alginate,.96},{Water,.04}},{},0,0,0,.8,6);
  add("calcium","Calcium lactate","CA","#a6c8be","Agent","Supplies calcium ions for alginate and low-sugar pectin networks.",{{Calcium,.22},{Other,.78}},{},.003,0,0,1,5);
  add("agar","Agar","AG","#6da58b","Agent","Needs a hot hydration stage, then cooling to set. Remelts at a much higher temperature than gelatin.",{{Agar,.95},{Water,.05}},{},0,0,0,1,6);
  add("gelatin","Gelatin","GT","#c8ad88","Agent","Dissolves warm and forms a soft, low-melting network on cooling. Proteases can destroy its chains.",{{Gelatin,.88},{Water,.10},{Protein,.02}},{},0,0,0,2,4);
  add("pectin","Pectin","PE","#c3b853","Agent","Can set through acid/sugar concentration or calcium bridging. A different network from agar.",{{Pectin,.95},{Water,.05}},{},0,0,0,1,5);
  add("lecithin","Lecithin","LE","#b8aa57","Agent","An interfacial stabiliser. Helps protect small fat droplets and air bubbles.",{{Lecithin,.60},{Fat,.35},{Water,.05}},{},0,0,0,.8,5);
  add("starch","Corn starch","CS","#e5dec6","Agent","Hydrates and gelatinises with heat; cooling promotes retrogradation. Amylase breaks it into sugar.",{{Starch,.90},{Water,.10}},{},0,0,0,4);
  add("amylase","Amylase","AM","#bbaa7e","Agent","An enzyme that breaks starch into reducing sugars in its active temperature window.",{{Amylase,.5},{Other,.5}},{},0,0,0,.3,8);
  add("papain","Papain","PA","#7caf6c","Agent","Proteolytic enzyme: releases amino acids, softens protein and can prevent gelatin setting. Heat deactivates it.",{{Protease,.5},{Other,.5}},{},0,0,0,.3,8);
  add("culture","Lactic culture","LC","#96b994","Agent","Converts available sugar into acids and gas under favourable temperature and water activity.",{{Culture,.7},{Other,.3}},{},0,0,0,.5,7);
  add("yeast","Yeast","YE","#c4a583","Agent","A culture biased towards ethanol and gas. Sugar, salt, alcohol and temperature change its activity.",{{Culture,.7},{Other,.3}},{},0,0,0,.5,5,1);
  add("citrate","Sodium citrate","SC","#9fb7cf","Agent","Buffers acidity and raises effective pH. Can rescue an acid-inhibited alginate mixture.",{{Base,.10},{Salt,.20},{Other,.70}},{},0,0,.8,1,4);
  add("bicarbonate","Bicarbonate","BC","#cbdad3","Agent","Neutralises acid and generates gas. More alkaline mixtures brown more readily.",{{Base,.75},{Other,.25}},{},0,0,.2,.5);
  add("binder","Transglutaminase","TG","#b48c9f","Agent","Catalyses protein crosslinks in a mild-temperature window, producing a different structure from hydrocolloid gels.",{{Transglutaminase,.6},{Other,.4}},{},0,0,0,.5,9);
  add("xanthan","Xanthan","XA","#a2c0a0","Agent","Cold-thickening polymer. Raises viscosity, slows separation and changes foam drainage without a hot setting stage.",{{Xanthan,.95},{Water,.05}},{},0,0,0,.3,6);
  add("lactic_acid","Lactic acid","LA","#d79b84","Agent","Purified fermentation acid for measured pH adjustment.",{{Acid,.90},{Water,.10}},{},0,0,0,.5,5);
  add("nutrient","Mineral nutrients","NP","#94a46c","Agent","Refined mineral blend for the farm fertigation input.",{{Salt,.25},{Calcium,.10},{Other,.65}},{},0,0,0,1,3);
  return v;
 }();return list;
}
const Ingredient& ingredient(const std::string& key){for(auto& i:pantry())if(i.key==key)return i;throw std::invalid_argument("Unknown ingredient: "+key);}
void extendPantryData(json& d){for(auto& i:pantry()){
 if(!d["TNAME"].contains(i.key))d["TNAME"][i.key]=i.name;if(!d["COLORS"].contains(i.key))d["COLORS"][i.key]=i.color;if(!d["CARTRIDGE_IDS"].contains(i.key))d["CARTRIDGE_IDS"][i.key]=i.code;if(!d["SHAPE_MAP"].contains(i.key))d["SHAPE_MAP"][i.key]=i.category=="Agent"?"powder":"liquid";
 if(!d["FOOD"].contains(i.key)){
  double sweet=(i.fractions[Sucrose]+i.fractions[Reducing])*8;
  d["FOOD"][i.key]={{"water",i.fractions[Water]*1.3},{"sugar",sweet},{"acid",i.fractions[Acid]*35},{"salt",i.fractions[Salt]*40},{"protein",i.fractions[Protein]*8},{"fat",i.fractions[Fat]*4},{"bitter",i.bitter*10},{"porosity",.8}};
  d["recipes"][i.key]={{"a",Aroma{i.aroma[0],i.aroma[1],i.aroma[2],i.aroma[3]}}};
 }
 }}
void to_json(json& j,const Environment& e){j={{"technique",int(e.technique)},{"temperature",e.temperature},{"shear",e.shear},{"pressure",e.pressure},{"cut",e.cut},{"stages",e.stages}};}
void from_json(const json& j,Environment& e){int t=j.at("technique");if(t<0||t>int(Technique::Condense))throw std::invalid_argument("Unknown technique");e.technique=Technique(t);e.temperature=j.value("temperature",20.);e.shear=j.value("shear",.5);e.pressure=j.value("pressure",1.);e.cut=j.value("cut",1);e.stages=j.value("stages",8);if(!std::isfinite(e.temperature)||e.temperature < -150||e.temperature>220||!std::isfinite(e.shear)||e.shear<0||e.shear>2||!std::isfinite(e.pressure)||e.pressure<.01||e.pressure>10||e.cut<0||e.cut>2||e.stages<1||e.stages>20)throw std::invalid_argument("Invalid process parameters");}
void to_json(json& j,const RecipeStep& s){j={{"id",s.id},{"result",s.result},{"side",s.side},{"action",int(s.action)},{"targets",s.targets},{"ingredient",s.ingredient},{"grams",s.grams},{"ticks",s.ticks},{"environment",s.environment}};}
void from_json(const json& j,RecipeStep& s){s.id=j.value("id",uint64_t(0));s.result=j.value("result",uint64_t(0));s.side=j.value("side",uint64_t(0));int a=j.at("action");if(a<0||a>3)throw std::invalid_argument("Invalid recipe action");s.action=RecipeAction(a);s.targets=j.value("targets",std::vector<uint64_t>{});s.ingredient=j.value("ingredient",std::string{});s.grams=j.value("grams",0.);s.ticks=j.value("ticks",1);s.environment=j.value("environment",Environment{});if(s.targets.size()>16||s.ticks<1||s.ticks>10000||!std::isfinite(s.grams)||s.grams<0||s.grams>100000)throw std::invalid_argument("Invalid recipe step");if(s.action==RecipeAction::Add)ingredient(s.ingredient);}
std::string techniqueName(Technique t){static const std::array<std::string,25> n={"Rest","Mix","Set gel","Aerate","Freeze","Heat","Dehydrate","Aroma seal","Thermal cycle","Homogenise","Spherify","Sous vide","Ferment","Enzyme incubation","Centrifuge","Filter","Vacuum","Freeze dry","Extrude","Smoke","Mill","Protein bind","Fractional distil","Column reflux","Condense vapour"};return n.at(size_t(t));}
std::string distillationCutName(int cut){static const std::array<const char*,3> names={"heads","hearts","tails"};return names.at(size_t(std::clamp(cut,0,2)));}
double foodMass(const Food& f){return std::accumulate(f.c.begin(),f.c.end(),0.);}
double foodPH(const Food& f){return clamp(7-2.2*std::log10(1+f.c[Acid]*1000/std::max(.1,f.c[Water]+f.buffer*30)),2,8.3);}
double waterActivity(const Food& f){return clamp(f.c[Water]/std::max(.001,f.c[Water]+.7*(f.c[Sucrose]+f.c[Reducing])+3*f.c[Salt]+.3*f.c[Protein]+1.2*f.c[Starch]),.02,1);}
double viscosity(const Food& f){double m=std::max(.01,foodMass(f));return 1+f.c[Fat]/m*2+f.c[Fibre]/m*(8+80/std::max(20.,f.particle))+f.c[Xanthan]/m*1200+f.c[Starch]/m*(10+100*f.starchCooked)+f.gel*25;}
static FoodPart* part(CulinaryBatch& b,uint64_t id){for(auto& p:b.parts)if(p.id==id)return &p;return nullptr;}
static void append(CulinaryBatch& b,RecipeStep op){
 if(op.action==RecipeAction::Process&&!b.history.empty()){auto& last=b.history.back();if(last.action==op.action&&last.targets==op.targets&&last.environment==op.environment&&last.ticks<9999){last.ticks+=op.ticks;return;}}
 b.history.push_back(std::move(op));
}
void addIngredient(CulinaryBatch& b,const std::string& key,double grams,uint64_t target,uint64_t op,bool record){
 if(grams<=0||grams>100000)return;const auto& i=ingredient(key);auto p=part(b,target);if(!p){b.parts.push_back({target,{}});p=&b.parts.back();}
 auto& f=p->food;double old=foodMass(f);for(int k=0;k<CompoundCount;k++)f.c[k]+=i.fractions[k]*grams;for(int k=0;k<6;k++)f.aroma[k]+=i.aroma[k]*grams*.01;
 f.bitter+=i.bitter*grams;f.umami+=i.umami*grams;f.buffer+=i.buffer*grams;double culture=i.fractions[Culture]*grams;
 f.yeast=f.c[Culture]>0?(f.yeast*(f.c[Culture]-culture)+i.yeast*culture)/f.c[Culture]:0;
 f.ingredients[key]+=grams;f.temperature=(f.temperature*old+20*grams)/(old+grams);
 if(record){RecipeStep s;s.id=op;s.result=target;s.action=RecipeAction::Add;s.ingredient=key;s.grams=grams;append(b,s);}
}
static double fraction(const Food& f,Compound c){return f.c[c]/std::max(.001,foodMass(f));}
void processFood(Food& f,const Environment& e){
 if(foodMass(f)<.001)return;f.events.clear();double m=foodMass(f),previousTemperature=f.temperature,previousPressure=f.pressure,pH=foodPH(f),aw=waterActivity(f);
 const bool heated=e.technique==Technique::Heat||e.technique==Technique::Dry||e.technique==Technique::SousVide||e.technique==Technique::Smoke||e.technique==Technique::Bind||e.technique==Technique::Enzyme||e.technique==Technique::Ferment||e.technique==Technique::Distill;
 if(heated)f.temperature+=(e.temperature-f.temperature)*(e.technique==Technique::SousVide?.28:.38);
 else if(e.technique==Technique::Freeze||e.technique==Technique::FreezeDry||e.technique==Technique::Condense)f.temperature+=(e.temperature-f.temperature)*.55;
 else if(e.technique==Technique::Reflux)f.temperature+=(e.temperature-f.temperature)*.12;
 else if(e.technique==Technique::Cycle){double target=int(f.age/2)%2? -18:70;f.temperature+=(target-f.temperature)*.45;}
 else f.temperature+=(20-f.temperature)*(e.technique==Technique::Gel?.38:.055);
 f.age++;
 // Acid/base chemistry changes both acidity and gas, while conserving mass.
 double neutralise=std::min(f.c[Acid],f.c[Base]*.8);if(neutralise>1e-8){f.c[Acid]-=neutralise;f.c[Base]-=neutralise;f.c[Salt]+=neutralise*.6;f.c[Water]+=neutralise*.8;f.c[CO2]+=neutralise*.6;f.events.push_back("ACID / BASE GAS");}
 pH=foodPH(f);
 // Protein, starch and enzyme histories are stateful; they do not reset between machines.
 if(f.temperature>55)f.denatured=clamp(f.denatured+(f.temperature-55)*.008,0,1);
 if(f.temperature>68&&fraction(f,Protein)>.03&&aw>.5){f.coagulated=clamp(f.coagulated+(f.temperature-68)*.008*f.denatured,0,1);f.events.push_back("THERMAL PROTEIN SET");}
 if(f.temperature>78){for(auto k:{Protease,Amylase,Culture,Transglutaminase}){double denature=f.c[k]*.6;f.c[k]-=denature;f.c[Other]+=denature;}}
 if(f.temperature>35&&f.temperature<70&&aw>.5){
  double rate=std::max(0.,1-std::abs(f.temperature-52)/20);
  double proteolysis=std::min(f.c[Protein]+f.c[Gelatin],f.c[Protease]*rate*5);
  double gelatin=std::min(f.c[Gelatin],proteolysis);f.c[Gelatin]-=gelatin;double protein=std::min(f.c[Protein],proteolysis-gelatin);f.c[Protein]-=protein;f.c[Amino]+=gelatin+protein;
  if(proteolysis>.0001)f.events.push_back("PROTEOLYSIS / UMAMI");
  double starch=std::min(f.c[Starch],f.c[Amylase]*rate*7);f.c[Starch]-=starch;f.c[Reducing]+=starch;if(starch>.0001)f.events.push_back("STARCH TO SUGAR");
 }
 if(f.c[Culture]>.00001&&f.temperature>14&&f.temperature<44&&aw>.8&&fraction(f,Salt)<.09&&fraction(f,Ethanol)<.13){
  double rate=(1-std::abs(f.temperature-29)/16)*f.c[Culture]*1.8*clamp((pH-2.7)/1.8,0,1);
  double used=std::min(f.c[Reducing]+f.c[Sucrose],std::max(0.,rate));double reducing=std::min(f.c[Reducing],used);f.c[Reducing]-=reducing;f.c[Sucrose]-=used-reducing;
  double acid=.65-.5*f.yeast,alcohol=.25+.45*f.yeast;f.c[Acid]+=used*acid;f.c[Ethanol]+=used*alcohol;f.c[CO2]+=used*(1-acid-alcohol);
  if(used>.00001){f.aroma[1]+=used*.03;f.aroma[3]+=used*.01;f.events.push_back("FERMENTATION");}
 }
 double invert=std::min(f.c[Sucrose],std::max(0.,f.temperature-45)*f.c[Acid]*.012);f.c[Sucrose]-=invert;f.c[Reducing]+=invert;
 if(f.temperature>65&&f.c[Starch]>0&&f.c[Water]>f.c[Starch]*2)f.starchCooked=clamp(f.starchCooked+.22,0,1);
 if(f.temperature<35&&f.starchCooked>.4)f.retrograded=clamp(f.retrograded+.035,0,1);
 if(f.temperature>82)f.agarHydrated=clamp(f.agarHydrated+.25,0,1);
 if(f.temperature>44)f.gelatinHydrated=clamp(f.gelatinHydrated+.35,0,1);
 double freeCalcium=std::max(0.,f.c[Calcium]-f.boundCalcium);
 if(f.c[Alginate]>.0001&&freeCalcium>0&&pH>4&&fraction(f,Salt)<.1){double bound=std::min({freeCalcium,f.c[Alginate]*.16,std::max(0.,f.c[Alginate]*.35-f.boundCalcium)});f.boundCalcium+=bound;if(bound>.0001)f.events.push_back("CALCIUM CROSSLINK");}
 double alginate=clamp(f.boundCalcium/std::max(.001,f.c[Alginate]*.3),0,1)*clamp(fraction(f,Alginate)*180,0,1.4);
 double agar=f.agarHydrated*clamp(fraction(f,Agar)*170,0,1.4)*(f.temperature<42?1:(f.temperature<80&&f.gel>.3)?1:0);
 double gelatin=f.gelatinHydrated*clamp(fraction(f,Gelatin)*65,0,1.2)*clamp((34-f.temperature)/14,0,1);
 double pectin=clamp(fraction(f,Pectin)*120,0,1.3)*std::max(clamp((fraction(f,Sucrose)+fraction(f,Reducing)-.45)*8,0,1)*clamp((4-pH)*2,0,1),clamp(f.c[Calcium]/std::max(.01,f.c[Pectin])*.8,0,1));
 if(f.c[Transglutaminase]>.00001&&f.temperature>15&&f.temperature<58&&fraction(f,Protein)>.025){f.proteinBond=clamp(f.proteinBond+f.c[Transglutaminase]/m*25,0,1);f.events.push_back("PROTEIN CROSSLINK");}
 double targetGel=std::max({alginate,agar,gelatin,pectin,f.starchCooked*clamp(fraction(f,Starch)*10,0,.8),f.proteinBond,f.denatured*f.coagulated*clamp(fraction(f,Protein)*8,0,1)});f.gel+=(targetGel-f.gel)*.6;
 if(pH<5&&fraction(f,Protein)>.015&&(f.temperature>43||pH<4.1)){f.coagulated=clamp(f.coagulated+.32*(5-pH),0,1);f.events.push_back("ACID COAGULATION");}
 if(e.technique==Technique::Spherify&&alginate>.25){f.shell=clamp(f.shell+.23*alginate,0,1);f.events.push_back("LIQUID CORE / GEL SHELL");}
 if(e.technique==Technique::Mix||e.technique==Technique::Homogenize||e.technique==Technique::Aerate||e.technique==Technique::Mill){
  double shear=e.shear*(e.technique==Technique::Homogenize?1.7:1);f.droplet=std::max(1.,f.droplet*(1-.42*shear));f.particle=std::max(8.,f.particle*(1-.35*shear));
 }
 double emulsifier=fraction(f,Lecithin)*100+fraction(f,Protein)*(1-f.coagulated)*8;
 double emulsion=clamp(emulsifier,0,1)*clamp((80-f.droplet)/60,0,1);
 if(e.technique!=Technique::Homogenize&&e.technique!=Technique::Mix&&e.technique!=Technique::Aerate)f.droplet=std::min(160.,f.droplet+2.5*(1-clamp(emulsifier,0,1))/(1+viscosity(f)*.2));
 double foamHold=clamp(fraction(f,Lecithin)*150+fraction(f,Protein)*(1-f.coagulated)*5+fraction(f,Xanthan)*160+f.gel*.35,0,1);
 if(e.technique==Technique::Aerate){double fatPenalty=fraction(f,Fat)*(1-emulsion)*2;f.bubbles=clamp(f.bubbles+.24*e.shear*(.25+foamHold)-fatPenalty,0,1.5);}
 else f.bubbles=std::max(0.,f.bubbles-(.17*(1-foamHold)+.025)/(1+viscosity(f)*.08));
 double fp=-20*(fraction(f,Sucrose)+fraction(f,Reducing))-70*fraction(f,Salt)-40*fraction(f,Ethanol);
 double iceTarget=clamp((fp-f.temperature)/12,0,.98);f.ice+=(iceTarget-f.ice)*.5;
 if(f.ice>.05){double growth=std::max(0.,previousTemperature-f.temperature);f.crystalSize=clamp(f.crystalSize+1.5+(growth<2?2:0)-growth*.2-e.shear*(e.technique==Technique::Freeze?1.5:0),1,150);}
 // Drying changes water activity, which feeds back into browning and texture.
 bool dry=e.technique==Technique::Dry||e.technique==Technique::FreezeDry;
 double evaporation=(e.technique==Technique::Distill||e.technique==Technique::Reflux||e.technique==Technique::Condense)?0:std::max(0.,f.temperature-55)*.0006*f.c[Water];if(dry)evaporation+=f.c[Water]*(e.technique==Technique::FreezeDry?.15:.13);
 if(e.technique==Technique::SousVide||e.technique==Technique::Seal)evaporation*=.04;
 evaporation=std::min(f.c[Water],evaporation);f.c[Water]-=evaporation;f.lostMass+=evaporation;
 aw=waterActivity(f);double browningTemp=std::max(0.,f.temperature-80)/55;
 double maillard=std::min({f.c[Reducing],(f.c[Protein]+f.c[Amino])*1.5,m*.006*browningTemp*clamp((pH-3)/3,0,1.8)*clamp((1-aw)*4,.08,1.5)});
 if(maillard>0){f.c[Reducing]-=maillard;double amino=std::min(f.c[Amino],maillard*.15);f.c[Amino]-=amino;double protein=std::min(f.c[Protein],maillard*.15-amino);f.c[Protein]-=protein;f.c[Other]+=maillard+amino+protein;f.brown=clamp(f.brown+maillard/m*35,0,2);f.aroma[2]+=maillard*.2;f.events.push_back("MAILLARD ROAST");}
 double caramel=std::min(f.c[Sucrose]+f.c[Reducing],m*.008*std::max(0.,f.temperature-112)/45*clamp((.85-aw)*4,0,2));
 if(caramel>0){double s=std::min(f.c[Sucrose],caramel);f.c[Sucrose]-=s;f.c[Reducing]-=caramel-s;f.c[Other]+=caramel;f.brown=clamp(f.brown+caramel/m*40,0,2);f.bitter+=caramel*.12;f.aroma[2]+=caramel*.25;f.events.push_back("CARAMELISATION");}
 if(f.temperature>155&&aw<.65){double charRate=(f.temperature-155)*.001;f.scorch=clamp(f.scorch+charRate,0,1.5);f.bitter+=charRate*m*.02;}
 if(e.technique==Technique::Smoke){f.smoke=clamp(f.smoke+.08,0,1);f.aroma[2]+=m*.002;f.aroma[3]+=m*.001;f.events.push_back("SMOKE DEPOSITION");}
 if(e.technique==Technique::Extrude&&viscosity(f)>3){f.gel=std::max(f.gel,.4);f.particle=std::max(5.,f.particle*.7);}
 f.pressure=e.pressure+f.c[CO2]/m*80;
 if(e.technique==Technique::Vacuum){f.bubbles=previousPressure>.5?std::min(1.8,f.bubbles/std::max(.2,e.pressure)):f.bubbles*.35;{double vent=f.c[CO2]*.65;f.c[CO2]-=vent;f.lostMass+=vent;}f.events.push_back("VACUUM DEGASSING");}
 if(e.technique!=Technique::Seal){double gasLost=f.c[CO2]*.28;f.c[CO2]-=gasLost;f.lostMass+=gasLost;}
 if(f.pressure>2.5){f.stability=std::max(.1,f.stability-.08);f.events.push_back("PRESSURE FRACTURE");}
 double aromaLoss=(e.technique==Technique::Distill||e.technique==Technique::Reflux||e.technique==Technique::Condense)?0:(.007+std::max(0.,f.temperature-35)*.0004)*(e.technique==Technique::Seal?.08:1)*(e.technique==Technique::Vacuum?4:1);
 for(auto& a:f.aroma){double loss=a*clamp(aromaLoss,0,.35);a-=loss;f.aromaReleased+=loss;}
 if(e.technique==Technique::Condense&&f.vapor){f.vapor=false;f.events.push_back("VAPOUR CONDENSED");}
 if(f.age>120)f.stability=std::max(.1,f.stability-.002);
 f.stability=clamp(f.stability-f.scorch*.004,0,1);
 if(f.shell>.38)f.phase="pearls";
 else if(f.scorch>.85)f.phase="char";
 else if(f.ice>.5)f.phase="crystal";
 else if(f.coagulated>.6&&pH<5)f.phase="curd";
 else if(aw<.42&&f.c[Water]/std::max(.01,foodMass(f))<.35)f.phase="crisp";
 else if(f.gel>.46)f.phase="gel";
 else if(emulsion>.45&&fraction(f,Fat)>.045)f.phase="emulsion";
 else if(f.bubbles>.34)f.phase="foam";
 else if(viscosity(f)>6)f.phase="puree";
 else f.phase="liquid";
 for(auto& amount:f.c)amount=std::max(0.,amount);
}

static Food combined(const std::vector<FoodPart>& parts){
 Food f;double total=0;for(auto& p:parts)total+=foodMass(p.food);if(total<=0)return f;
 f.temperature=f.stability=f.particle=f.droplet=f.pressure=0;
 bool allVapor=true,mixedFraction=false;std::string commonFraction;
 for(auto& p:parts){const auto& a=p.food;double w=foodMass(a)/total;allVapor=allVapor&&a.vapor;if(commonFraction.empty())commonFraction=a.fraction;else if(commonFraction!=a.fraction)mixedFraction=true;
  for(int k=0;k<CompoundCount;k++)f.c[k]+=a.c[k];for(int k=0;k<6;k++)f.aroma[k]+=a.aroma[k];
  for(auto& [k,n]:a.ingredients)f.ingredients[k]+=n;
  f.bitter+=a.bitter;f.umami+=a.umami;f.buffer+=a.buffer;f.boundCalcium+=a.boundCalcium;f.lostMass+=a.lostMass;f.aromaReleased+=a.aromaReleased;
#define WEIGHT(v) f.v+=a.v*w;
  WEIGHT(temperature) WEIGHT(age) WEIGHT(denatured) WEIGHT(agarHydrated) WEIGHT(gelatinHydrated) WEIGHT(starchCooked) WEIGHT(retrograded) WEIGHT(proteinBond) WEIGHT(particle) WEIGHT(bubbles) WEIGHT(droplet) WEIGHT(crystalSize) WEIGHT(ice) WEIGHT(brown) WEIGHT(scorch) WEIGHT(coagulated) WEIGHT(gel) WEIGHT(shell) WEIGHT(stability) WEIGHT(pressure) WEIGHT(smoke)
#undef WEIGHT
  f.yeast+=a.yeast*a.c[Culture];
 }
 if(f.c[Culture]>0)f.yeast/=f.c[Culture];f.phase=parts.size()==1?parts.front().food.phase:"liquid";f.vapor=allVapor;f.fraction=mixedFraction?"blend":commonFraction;return f;
}
void processBatch(CulinaryBatch& b,const Environment& e,const std::vector<uint64_t>& targets,uint64_t op,bool record){
 for(auto id:targets)if(auto p=part(b,id))processFood(p->food,e);
 if(record&&!targets.empty()){RecipeStep s;s.id=op;s.action=RecipeAction::Process;s.targets=targets;s.environment=e;append(b,s);}
}
void blendParts(CulinaryBatch& b,const std::vector<uint64_t>& inputs,uint64_t output,uint64_t op,bool record){
 std::vector<FoodPart> chosen;for(auto& p:b.parts)if(std::find(inputs.begin(),inputs.end(),p.id)!=inputs.end())chosen.push_back(p);
 if(chosen.empty())return;Food f=combined(chosen);f.shell=0;f.gel*=.65;f.bubbles*=.55;
 b.parts.erase(std::remove_if(b.parts.begin(),b.parts.end(),[&](const FoodPart& p){return std::find(inputs.begin(),inputs.end(),p.id)!=inputs.end();}),b.parts.end());
 b.parts.push_back({output,std::move(f)});
 if(record){RecipeStep s;s.id=op;s.result=output;s.action=RecipeAction::Blend;s.targets=inputs;append(b,s);}
}
void splitPart(CulinaryBatch& b,uint64_t target,uint64_t main,uint64_t side,const Environment& e,uint64_t op,bool record){
 auto source=part(b,target);if(!source)return;Food a=source->food,z=a;double initial=foodMass(a);
 auto mainShare=[&](int k){
  if(e.technique==Technique::Distill){const double ethanol[]={.52,.58,.72},water[]={.96,.88,.70},acid[]={.94,.82,.52},gas[]={.35,.62,.86};if(k==Ethanol)return ethanol[e.cut];if(k==Water)return water[e.cut];if(k==Acid)return acid[e.cut];if(k==CO2)return gas[e.cut];if(k==Reducing||k==Sucrose)return .995;if(k==Amino)return .96;if(k==Other)return .985;return .995;}
  if(e.technique==Technique::Reflux){double stages=std::clamp(e.stages,1,20);if(k==Ethanol)return clamp(.68+stages*.014,0,.96);if(k==Water)return clamp(.57-stages*.021,.12,.55);if(k==Acid)return clamp(.38+stages*.017,.35,.78);if(k==CO2)return .82;if(k==Reducing||k==Sucrose)return .12;if(k==Other)return .22;return .035;}
  double portion=.88;if(e.technique==Technique::Centrifuge)portion=k==Fat||k==Lecithin?.12:k==Protein?.55:k==Fibre||k==Starch?.18:.88;else portion=k==Fibre||k==Starch||k==Alginate?.08:k==Protein?.68:k==Fat?.80:.91;return portion;
 };
 for(int k=0;k<CompoundCount;k++){
  double portion=mainShare(k);
  a.c[k]*=portion;z.c[k]-=a.c[k];
 }
 double ratio=initial>0?foodMass(a)/initial:.5;
 for(int k=0;k<6;k++){double share=ratio;if(e.technique==Technique::Distill){double cutShare[]={.76,.52,.34};double volatility[]={1.14,1.06,.77,.91,.50,1.18};share=clamp(cutShare[e.cut]*volatility[k],.08,.94);}else if(e.technique==Technique::Reflux){double stages=std::clamp(e.stages,1,20);double volatility[]={1.04,.98,.72,.89,.48,1.08};share=clamp((.43+stages*.018)*volatility[k],.08,.96);}a.aroma[k]*=share;z.aroma[k]-=a.aroma[k];}
 for(auto& [k,n]:a.ingredients)n*=ratio;for(auto& [k,n]:z.ingredients)n*=1-ratio;
 for(auto f:{&Food::bitter,&Food::umami,&Food::buffer,&Food::boundCalcium,&Food::lostMass,&Food::aromaReleased}){a.*f*=ratio;z.*f*=1-ratio;}
 if(source->food.c[Calcium]>0){a.boundCalcium=source->food.boundCalcium*a.c[Calcium]/source->food.c[Calcium];z.boundCalcium=source->food.boundCalcium-a.boundCalcium;}
 a.bubbles=z.bubbles=0;a.shell=z.shell=0;a.gel=z.gel=0;a.droplet=z.droplet=120;a.phase=z.phase="liquid";a.vapor=z.vapor=false;a.fraction=z.fraction="";
 if(e.technique==Technique::Distill){a.fraction="stillage";z.fraction=distillationCutName(e.cut);z.vapor=true;a.events={"STILLAGE / VOLATILES REMOVED"};z.events={upper(z.fraction)+" / VOLATILE CUT"};}
 else if(e.technique==Technique::Reflux){a.vapor=source->food.vapor;a.fraction=source->food.fraction;z.fraction="reflux";a.events={"ENRICHED "+upper(a.fraction)+" VAPOUR"};z.events={"WATER-RICH REFLUX"};}
 else{a.events={e.technique==Technique::Centrifuge?"AQUEOUS FRACTION":"CLARIFIED FRACTION"};z.events={"RETAINED CONCENTRATE"};}
 b.parts.erase(std::remove_if(b.parts.begin(),b.parts.end(),[&](const FoodPart& p){return p.id==target;}),b.parts.end());b.parts.push_back({main,a});b.parts.push_back({side,z});
 if(record){RecipeStep s;s.id=op;s.result=main;s.side=side;s.targets={target};s.action=RecipeAction::Split;s.environment=e;append(b,s);}
}
bool executeStep(CulinaryBatch& b,const RecipeStep& s,bool record){
 if(s.action==RecipeAction::Add){addIngredient(b,s.ingredient,s.grams,s.result,s.id,record);return true;}
 for(auto id:s.targets)if(!part(b,id))return false;
 if(s.action==RecipeAction::Process)for(int n=0;n<s.ticks;n++)processBatch(b,s.environment,s.targets,s.id,record);
 if(s.action==RecipeAction::Blend)blendParts(b,s.targets,s.result,s.id,record);
 if(s.action==RecipeAction::Split){if(s.targets.size()!=1)return false;splitPart(b,s.targets.front(),s.result,s.side,s.environment,s.id,record);}
 return true;
}
void mergeHistories(CulinaryBatch& a,const CulinaryBatch& b){
 std::map<uint64_t,RecipeStep> ops;for(auto& s:a.history)ops[s.id]=s;for(auto& s:b.history)ops.emplace(s.id,s);a.history.clear();for(auto& [id,s]:ops)a.history.push_back(s);
 for(auto& p:b.parts)if(!part(a,p.id))a.parts.push_back(p);
}
FoodMetrics analyse(const CulinaryBatch& b){
 FoodMetrics m;if(b.parts.empty())return m;Food f=combined(b.parts);m.mass=foodMass(f);if(m.mass<.001)return m;
 m.pH=foodPH(f);m.waterActivity=waterActivity(f);m.viscosity=viscosity(f);m.temperature=f.temperature;m.gel=f.gel;m.foam=f.bubbles;m.ice=f.ice;m.stability=f.stability;m.brown=f.brown;m.scorch=f.scorch;m.vapor=f.vapor;m.fraction=f.fraction;
 m.emulsion=clamp(fraction(f,Lecithin)*100+fraction(f,Protein)*(1-f.coagulated)*8,0,1)*clamp((80-f.droplet)/60,0,1);
 m.flavour={clamp((fraction(f,Sucrose)+fraction(f,Reducing)*.8)*2.4,0,1),clamp(fraction(f,Acid)*26,0,1),clamp(fraction(f,Salt)*25,0,1),clamp((f.umami+f.c[Amino]*.7)/m.mass*5,0,1),clamp(f.bitter/m.mass*7,0,1)};
 m.aroma=clamp(std::accumulate(f.aroma.begin(),f.aroma.end(),0.)/(m.mass*.012),0,1);
 double intensity=std::accumulate(m.flavour.begin(),m.flavour.end(),0.);int present=0;for(double t:m.flavour)if(t>.08)present++;
 m.flavourBalance=clamp(.55+present*.08-std::max(0.,*std::max_element(m.flavour.begin(),m.flavour.end())-.85)*.8,0,1);
 if(intensity<.08)m.flavourBalance=.28;
 m.phase=b.parts.size()>1?"plated":b.parts.front().food.phase;
 std::set<std::string> textures;double lo=1000,hi=-1000;
 for(auto& p:b.parts){textures.insert(p.food.phase);lo=std::min(lo,p.food.temperature);hi=std::max(hi,p.food.temperature);for(auto& e:p.food.events)if(std::find(m.notes.begin(),m.notes.end(),e)==m.notes.end())m.notes.push_back(e);}
 m.contrast=clamp((textures.size()-1)*.32+(hi-lo)/100,0,1);
 double structure=clamp(std::max({m.gel,m.foam,m.emulsion,m.ice,m.brown})+.2,0,1);
 m.quality=100*clamp(.43*f.stability+.23*m.flavourBalance+.15*m.aroma+.12*structure+.07*m.contrast-.8*f.scorch-.12*f.coagulated,0,1);
 if(f.c[Alginate]>.01&&m.pH<=4)m.notes.push_back("Acidity inhibits alginate: buffer or dilute before setting");
 if(f.c[Alginate]>.01&&f.c[Calcium]<f.c[Alginate]*.08)m.notes.push_back("Alginate needs available calcium");
 if(f.c[Agar]>.01&&f.agarHydrated<.3)m.notes.push_back("Agar needs a hot hydration stage before cooling");
 if(f.c[Gelatin]>.01&&f.c[Protease]>.01)m.notes.push_back("Active protease can break gelatin chains");
 if(f.c[Starch]>.1&&f.starchCooked<.2)m.notes.push_back("Starch needs water and heat to gelatinise");
 if(fraction(f,Fat)>.05&&m.emulsion<.4)m.notes.push_back("Fat dispersion needs shear and an emulsifier");
 if(f.bubbles>.2)m.notes.push_back("Foam drainage depends on protein, surfactants and viscosity");
 if(f.c[Culture]>.01)m.notes.push_back("Culture activity depends on temperature, salt, sugar and alcohol");
 if(f.crystalSize>60)m.notes.push_back("Large ice crystals: cool faster or increase shear");
 if(f.scorch>.15)m.notes.push_back("Scorching reduces aroma and condition");
 return m;
}
Cell projectCell(const CulinaryBatch& b){
 Cell c;double total=0;for(auto& p:b.parts){const auto& f=p.food;double mass=foodMass(f);if(mass<.0001)continue;Piece z;z.id=int(c.stack.size()+1);z.k="water";double largest=0;for(auto& [k,g]:f.ingredients)if(g>largest){largest=g;z.k=k;}
 z.water=fraction(f,Water)*1.5;z.sugar=(fraction(f,Sucrose)+fraction(f,Reducing))*5;z.acid=fraction(f,Acid)*35;z.salt=fraction(f,Salt)*40;z.protein=fraction(f,Protein)*7+fraction(f,Amino)*4;z.fat=fraction(f,Fat)*4;z.bitter=f.bitter/std::max(.01,mass)*10;
 z.temp=(f.temperature-20)/40;z.phase=f.phase;z.age=int(f.age);z.gel=f.gel;z.foam=f.bubbles;z.emulsion=clamp(fraction(f,Lecithin)*100+fraction(f,Protein)*8,0,1)*clamp((80-f.droplet)/60,0,1);z.ice=f.ice;z.brown=f.brown;z.scorch=f.scorch;z.integrity=f.stability;z.crisp=f.phase=="crisp"?1:0;z.sourced=1;
 for(int k=0;k<4;k++)z.aroma[k]=f.aroma[k];c.stack.push_back(z);c.temp+=z.temp*mass;total+=mass;c.pressure=std::max(c.pressure,f.pressure);}
 if(total)c.temp/=total;return c;
}
bool validCulinary(const CulinaryBatch& b){
 if(b.parts.size()>16||b.history.size()>20000)return false;std::set<uint64_t> ids;
 for(auto& p:b.parts){if(!p.id||!ids.insert(p.id).second)return false;json j=p.food;for(auto it=j.begin();it!=j.end();++it)if(it->is_number()&&(!std::isfinite(it->get<double>())||std::abs(it->get<double>())>1e12))return false;
  if(p.food.age<0||p.food.stability<0||p.food.stability>1||p.food.lostMass<0||p.food.boundCalcium<0||p.food.boundCalcium>p.food.c[Calcium]+1e-6)return false;
  if(foodMass(p.food)<=0||p.food.temperature < -150||p.food.temperature>220)return false;
  for(double n:p.food.c)if(!std::isfinite(n)||n<0||n>1e9)return false;for(double n:p.food.aroma)if(!std::isfinite(n)||n<0||n>1e9)return false;
  if(p.food.ingredients.size()>pantry().size()||p.food.events.size()>50||p.food.phase.size()>30)return false;
  for(auto& [key,n]:p.food.ingredients){try{ingredient(key);}catch(...){return false;}if(!std::isfinite(n)||n<0||n>1e9)return false;}
 }
 std::set<uint64_t> ops;for(auto& s:b.history){if(!s.id||!ops.insert(s.id).second||s.targets.size()>16||s.ticks<1||s.ticks>10000||!std::isfinite(s.grams)||s.grams<0||s.grams>100000)return false;try{auto checked=json(s).get<RecipeStep>();(void)checked;}catch(...){return false;}}
 return true;
}
std::map<std::string,double> formula(const CulinaryBatch& b){std::map<std::string,double> r;for(auto& s:b.history)if(s.action==RecipeAction::Add)r[s.ingredient]+=s.grams;return r;}
} // namespace aftertaste
