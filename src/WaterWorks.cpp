#include "WaterWorks.h"
#include "Power.h"
#include <algorithm>
#include <cmath>
namespace aftertaste {
namespace {
using K=MachineKind;using E=EnergyMedium;constexpr double eps=1e-8;
size_t wi(E e){return size_t(e)-size_t(E::RawWater);}size_t ei(E e){return size_t(e);}
WaterParcel scaled(WaterParcel p,double fraction){p.volume*=fraction;p.solids*=fraction;p.salt*=fraction;p.bio*=fraction;return p;}
bool validParcel(WaterParcel p){for(double v:{p.volume,p.solids,p.salt,p.bio})if(!std::isfinite(v)||v<0||v>1e12)return false;return true;}
WaterParcel takeParcel(WaterParcel& from,double volume){double fraction=from.volume>eps?std::clamp(volume/from.volume,0.,1.):0;auto out=scaled(from,fraction);from=scaled(from,1-fraction);return out;}
double room(const Machine& m,E e){return std::max(0.,m.power.capacity-m.power.store[ei(e)]);}
E tankMedium(K k){return k==K::RawWaterTank?E::RawWater:k==K::ConcentrateTank?E::Concentrate:E::CleanWater;}
void stage(Factory& f,Position p,Machine& m,const WaterPull& pull,E in,E out,double energy,double yield=1){
 auto& w=m.waterProcess;double rate=m.power.rate*m.power.throttle/(1+w.fouling);
 double amount=std::min(rate,room(m,out)/yield);if(yield<1)amount=std::min(amount,room(m,E::Concentrate)/(1-yield));
 if(amount<=eps){m.status="Outlet backed up / empty connected storage";return;}
 auto input=pull(p,in,amount,energy,false);if(input.volume<=eps){m.status="Needs "+energyName(in)+" and connected AC";return;}
 WaterParcel good=input,bad;
 if(m.kind==K::SandFilter){double removed=std::min(input.solids*.96,std::max(0.,800-w.captured));w.captured+=removed;good.solids-=removed;w.fouling=std::min(4.,w.captured/400);}
 if(m.kind==K::MembraneFilter){double saltPass=std::clamp(.02+w.fouling*.12+std::max(0.,yield-.75)*.3,.02,.8);good=scaled(input,yield);good.solids=input.solids*.03;good.salt=input.salt*saltPass;good.bio=input.bio*.06;bad={input.volume-good.volume,input.solids-good.solids,input.salt-good.salt,input.bio-good.bio};w.fouling=std::min(4.,w.fouling+(input.solids*.002+input.salt*.0001));}
 if(m.kind==K::ReedReclaimer){good=scaled(input,yield);good.solids=input.solids*.25;good.salt=input.salt*.9;good.bio=input.bio*.2;bad={input.volume-good.volume,input.solids-good.solids,input.salt-good.salt,input.bio-good.bio};f.waterworks.recovered+=good.volume;}
 if(m.kind==K::UVSterilizer){double surviving=good.bio*std::exp(-9*w.uvDose);f.waterworks.killed+=good.bio-surviving;good.bio=surviving;}
 putWater(m,out,good);if(bad.volume>eps){putWater(m,E::Concentrate,bad);w.rejected+=bad.volume;}
 w.produced+=good.volume;m.power.flow=input.volume;w.sample=good;m.status="Processing / "+roundTo(input.volume,2)+" L -> "+roundTo(good.volume,2)+" L";
}
}
bool isWaterMachine(K k){return k>=K::BoreholePump&&k<=K::WaterAnalyzer;}
bool isWaterStorage(K k){return k==K::RawWaterTank||k==K::CleanWaterTank||k==K::ConcentrateTank;}
bool storesWaterMedium(K k,E e){return isWaterStorage(k)&&tankMedium(k)==e;}
bool isWaterStream(E e){return e>=E::RawWater&&e<=E::Concentrate;}
void addWaterParcel(WaterParcel& a,const WaterParcel& b){a.volume+=b.volume;a.solids+=b.solids;a.salt+=b.salt;a.bio+=b.bio;}
WaterParcel storedWater(const Machine& m,E e){const auto& q=m.waterProcess.loads[wi(e)];return {m.power.store[ei(e)],q.solids,q.salt,q.bio};}
WaterParcel waterSample(const Machine& m){WaterParcel best;for(size_t n=0;n<WaterStreamCount;n++){auto p=storedWater(m,E(size_t(E::RawWater)+n));if(p.volume>best.volume)best=p;}return best.volume>eps?best:m.waterProcess.sample;}
WaterParcel removeWaterLoad(Machine& m,E e,double volume){auto parcel=storedWater(m,e);double fraction=parcel.volume>eps?std::clamp(volume/parcel.volume,0.,1.):0;auto out=scaled(parcel,fraction);auto& q=m.waterProcess.loads[wi(e)];q.solids=std::max(0.,q.solids-out.solids);q.salt=std::max(0.,q.salt-out.salt);q.bio=std::max(0.,q.bio-out.bio);return out;}
double putWater(Machine& m,E e,WaterParcel p){double fraction=p.volume>eps?std::clamp(room(m,e)/p.volume,0.,1.):0;p=scaled(p,fraction);m.power.store[ei(e)]+=p.volume;auto& q=m.waterProcess.loads[wi(e)];q.solids+=p.solids;q.salt+=p.salt;q.bio+=p.bio;return p.volume;}
bool acceptableWater(WaterParcel p){return p.volume>eps&&p.solids/p.volume<=.15&&p.salt/p.volume<=1&&p.bio/p.volume<=.02;}
double waterRainfall(int tick){double storm=std::sin(tick*.017+1.2)+.45*std::sin(tick*.053);return std::clamp((storm-.15)*.8,0.,1.);}
std::vector<MachineSpec> waterMachineSpecs(){return {
 {K::RawWaterPipe,"Raw-water pipe","RAW","Carries collected water with its actual suspended solids, salts and biological load.","#759b9b",3,0,0,0},
 {K::FilteredWaterPipe,"Filtered-water pipe","FIL","Moves sand-filter effluent to the membrane. Filtration alone does not remove dissolved salts.","#62b6b2",3,0,0,0},
 {K::PurifiedWaterPipe,"Permeate pipe","PER","Carries membrane permeate to disinfection. This stream can still have a biological load.","#69bfd6",3,0,0,0},
 {K::CleanWaterPipe,"Clean-water pipe","H2O","Connects UV treatment, clean storage and the factory inlet. The inlet tests actual quality.","#259fc9",3,0,0,0},
 {K::GreywaterPipe,"Greywater pipe","GRY","Carries captured process wash-water into reclamation. It remains separate from clean water.","#948b72",3,0,0,0},
 {K::ConcentratePipe,"Concentrate pipe","REJ","Routes membrane reject, backwash and purged water into a finite waste tank.","#bc9257",3,0,0,0},
 {K::BoreholePump,"Borehole pump","WELL","Uses real AC to extract from the shared finite aquifer. Rain slowly recharges it; extra wells share the same resource.","#4b99b9",90,1,0,0},
 {K::RainCollector,"Rain collector","RAIN","Collects only during the local rain cycle. Rain contains less salt than groundwater, but still needs filtration and disinfection.","#78b9c8",45,1,0,0},
 {K::RawWaterTank,"Raw-water buffer","RAW","Blends actual source volumes and contaminant loads. Buffer tanks absorb intermittent rain and extraction.","#78a5a7",60,1,0,0},
 {K::SandFilter,"Layered sand filter","SAND","Captures suspended solids in finite media. Fouling reduces flow; a real two-litre backwash sends the captured load to reject.","#b1a279",65,1,0,0},
 {K::MembraneFilter,"Membrane treatment skid","RO","Splits filtered water into permeate and salty concentrate. Higher recovery leaves less reject but passes more salt. Fouling increases breakthrough.","#69a9bb",120,1,0,0},
 {K::UVSterilizer,"UV treatment chamber","UV","Spends AC on a measured game disinfection dose. Dose changes biological reduction; solids and dissolved salts remain.","#9b85c6",90,1,0,0},
 {K::CleanWaterTank,"Clean-water reservoir","CLEAN","Stores measured treated water. Unsafe mixed contents can be purged into a connected reject route; plans never clone stored water.","#46aeca",70,1,0,0},
 {K::WaterOutlet,"Factory water inlet","H2O+","Tests the actual held water. Only water meeting the game's clarity, salt and biological limits reaches the shared farm/factory input buffer.","#2e9bbd",60,1,0,0},
 {K::GreywaterDrain,"Process-water return","DRAIN","Collects actual recoverable wash-water from factory processes. Irrigation and boiler makeup are not magically recovered here.","#908c77",45,1,0,0},
 {K::ReedReclaimer,"Reed-bed reclamation cell","REED","Recovers 90% of greywater as raw water. Solids and biological load leave in concentrate; recovered water still needs full treatment.","#6b9d79",85,1,0,0},
 {K::ConcentrateTank,"Concentrate holding tank","WASTE","Finite storage for saline reject, backwash solids and purge water. A full tank backs up production; haul its actual contents away to resume.","#b08e60",60,1,0,0},
 {K::WaterAnalyzer,"Water quality monitor","WQ","Point at a water machine to sample its real volume, suspended solids, salinity and biological load. No sample is consumed.","#5fabc0",55,1,0,0}};}
std::vector<EnergyPort> waterPorts(const Machine& m){std::vector<EnergyPort> p;auto add=[&](E e,int d,bool output){p.push_back({e,(d+m.dir)%4,output});};
 if(m.kind!=K::RainCollector)add(E::AC,m.kind==K::WaterOutlet?0:3,false);
 switch(m.kind){case K::BoreholePump:case K::RainCollector:add(E::RawWater,0,true);break;
 case K::RawWaterTank:case K::CleanWaterTank:case K::ConcentrateTank:for(int d=0;d<4;d++)add(tankMedium(m.kind),d,true);if(m.kind==K::CleanWaterTank)add(E::Concentrate,1,true);break;
 case K::SandFilter:add(E::RawWater,2,false);add(E::FilteredWater,0,true);add(E::Concentrate,1,true);break;
 case K::MembraneFilter:add(E::FilteredWater,2,false);add(E::PurifiedWater,0,true);add(E::Concentrate,1,true);break;
 case K::UVSterilizer:add(E::PurifiedWater,2,false);add(E::CleanWater,0,true);break;
 case K::WaterOutlet:add(E::CleanWater,2,false);add(E::Concentrate,1,true);break;
 case K::GreywaterDrain:add(E::Greywater,0,true);break;
 case K::ReedReclaimer:add(E::Greywater,2,false);add(E::RawWater,0,true);add(E::Concentrate,1,true);break;
 default:break;}return p;}
void initialiseWaterDevice(Machine& m){m.waterProcess=WaterState{};m.power.rate=6;m.power.capacity=80;
 switch(m.kind){case K::BoreholePump:m.power.rate=4;break;case K::RainCollector:m.power.rate=3;break;case K::RawWaterTank:case K::CleanWaterTank:m.power.rate=16;m.power.capacity=600;break;case K::ConcentrateTank:m.power.rate=24;m.power.capacity=1200;break;case K::MembraneFilter:m.power.rate=4;break;case K::WaterOutlet:m.power.rate=8;m.dose=180;break;default:break;}}
void clearWaterInventory(Machine& m){double recovery=m.waterProcess.recovery,dose=m.waterProcess.uvDose;m.waterProcess=WaterState{};m.waterProcess.recovery=recovery;m.waterProcess.uvDose=dose;}
bool validWaterState(const Machine& m){const auto& w=m.waterProcess;for(auto q:w.loads)for(double v:{q.solids,q.salt,q.bio})if(!std::isfinite(v)||v<0||v>1e12)return false;for(double v:{w.captured,w.fouling,w.recovery,w.uvDose,w.produced,w.rejected,w.delivered})if(!std::isfinite(v)||v<0||v>1e12)return false;if(w.captured>800.0001||w.fouling>4||w.recovery<.5||w.recovery>.9||w.uvDose<.25||w.uvDose>3||!validParcel(w.sample))return false;for(size_t i=0;i<WaterStreamCount;i++)if(m.power.store[size_t(E::RawWater)+i]<eps){auto q=w.loads[i];if(q.solids+q.salt+q.bio>1e-5)return false;}return true;}
bool validWaterWorksState(const WaterWorksState& w){for(double v:{w.aquifer,w.aquiferCapacity,w.extracted,w.rainCollected,w.recharged,w.delivered,w.recovered,w.killed})if(!std::isfinite(v)||v<0||v>1e12)return false;return w.aquifer<=w.aquiferCapacity&&validParcel(w.greywater)&&validParcel(w.disposed)&&validParcel(w.supply);}
void collectGreywater(FactoryState& f,double volume){if(volume<=eps)return;WaterParcel p{volume,volume*8,volume*1.5,volume*6};double kept=std::min(volume,std::max(0.,500-f.waterworks.greywater.volume));addWaterParcel(f.waterworks.greywater,scaled(p,kept/volume));addWaterParcel(f.waterworks.disposed,scaled(p,1-kept/volume));}
void disposeWaterInventory(FactoryState& f,Machine& m){for(size_t i=0;i<WaterStreamCount;i++){E e=E(size_t(E::RawWater)+i);addWaterParcel(f.waterworks.disposed,storedWater(m,e));m.power.store[ei(e)]=0;m.waterProcess.loads[i]={};}f.waterworks.disposed.solids+=m.waterProcess.captured;m.waterProcess.captured=0;}
bool configureWaterDevice(Factory& f,Machine& m,const std::string& setting){if(!isWaterMachine(m.kind)){f.notice="Select waterworks equipment.";return false;}auto& w=m.waterProcess;
 if(setting=="water-rinse"&&(m.kind==K::SandFilter||m.kind==K::MembraneFilter)){w.rinse=true;f.notice="Backwash queued: needs 2 L supplied clean water, AC and reject capacity.";return true;}
 if(setting=="water-purge"&&(m.kind==K::CleanWaterTank||m.kind==K::WaterOutlet)){w.purge=true;f.notice="Held water will drain into the actual concentrate route.";return true;}
 if(setting=="water-haul"&&m.kind==K::ConcentrateTank){double cost=m.power.store[ei(E::Concentrate)]*.02;if(!f.sandbox&&f.cash<cost){f.notice="Not enough credits to haul the concentrate.";return false;}if(!f.sandbox)f.cash-=cost;disposeWaterInventory(f,m);f.notice="Concentrate hauled away; its volume and loads remain in the waste ledger.";return true;}
 if(setting=="water-recovery+"||setting=="water-recovery-")w.recovery=std::clamp(w.recovery+(setting.back()=='+'?.05:-.05),.5,.9);
 else if(setting=="water-dose+"||setting=="water-dose-")w.uvDose=std::clamp(w.uvDose+(setting.back()=='+'?.25:-.25),.25,3.);
 else if(setting=="water-target+"||setting=="water-target-")m.dose=std::clamp(m.dose+(setting.back()=='+'?20:-20),20.,2000.);
 else {f.notice="This water setting does not apply here.";return false;}f.notice="Water process setting updated.";return true;}
void runWaterWorks(Factory& f,const WaterPull& pull,const WaterPower& power){
 bool present=std::any_of(f.machines.begin(),f.machines.end(),[](const auto& p){return isWaterMachine(p.second.kind);});if(!present)return;
 double recharge=std::min(f.waterworks.aquiferCapacity-f.waterworks.aquifer,waterRainfall(f.tick)*.18);f.waterworks.aquifer+=recharge;f.waterworks.recharged+=recharge;
 for(auto& [p,m]:f.machines)if(isWaterMachine(m.kind)&&m.enabled&&!m.power.tripped){auto& w=m.waterProcess;
  if(w.purge){auto packet=storedWater(m,E::CleanWater);double amount=std::min(packet.volume,room(m,E::Concentrate));if(amount>eps){packet=removeWaterLoad(m,E::CleanWater,amount);m.power.store[ei(E::CleanWater)]-=packet.volume;putWater(m,E::Concentrate,packet);}w.purge=m.power.store[ei(E::CleanWater)]>eps;m.status=w.purge?"Purge backed up / connect concentrate storage":"Purged to concentrate";continue;}
  if(w.rinse){if(f.water<2||room(m,E::Concentrate)<2||!power(p,.3)){m.status="Backwash needs 2 L clean supply, AC and reject room";continue;}f.water-=2;f.waterUsed+=2;putWater(m,E::Concentrate,{2,w.captured,0,0});w.captured=0;w.fouling*=.1;w.rinse=false;m.status="Backwashed / 2 L routed to concentrate";continue;}
  if(m.kind==K::BoreholePump){double amount=std::min({m.power.rate*m.power.throttle,room(m,E::RawWater),f.waterworks.aquifer});if(amount>eps&&power(p,amount*.12)){putWater(m,E::RawWater,{amount,amount,amount*6,amount});w.sample={amount,amount,amount*6,amount};f.waterworks.aquifer-=amount;f.waterworks.extracted+=amount;m.power.flow=amount;w.produced+=amount;m.status="Extracting / "+roundTo(amount,2)+" L / aquifer "+roundTo(f.waterworks.aquifer,0);}else m.status=amount<=eps?"Aquifer empty or outlet full":"Needs connected AC";}
  if(m.kind==K::RainCollector){double amount=std::min(room(m,E::RawWater),m.power.rate*m.power.throttle*waterRainfall(f.tick));putWater(m,E::RawWater,{amount,amount*2,amount*.2,amount*4});w.sample={amount,amount*2,amount*.2,amount*4};f.waterworks.rainCollected+=amount;w.produced+=amount;m.power.flow=amount;m.status=amount>eps?"Collecting rain / "+roundTo(amount,2)+" L":"Dry weather or collector full";}
  if(m.kind==K::GreywaterDrain){auto amount=std::min({m.power.rate*m.power.throttle,room(m,E::Greywater),f.waterworks.greywater.volume});auto packet=takeParcel(f.waterworks.greywater,amount);putWater(m,E::Greywater,packet);w.sample=packet;m.power.flow=amount;m.status=amount>eps?"Captured process return / "+roundTo(amount,2)+" L":"Waiting for actual process wash-water";}
 }
 auto fill=[&](bool raw){for(auto& [p,m]:f.machines)if(isWaterStorage(m.kind)&&m.enabled&&!m.power.tripped&&(m.kind==K::RawWaterTank)==raw){E e=tankMedium(m.kind);auto packet=pull(p,e,std::min(m.power.rate*m.power.throttle,room(m,e)),0,true);putWater(m,e,packet);m.power.flow=packet.volume;if(packet.volume>eps)m.waterProcess.sample=packet;m.status="Stored "+roundTo(m.power.store[ei(e)],1)+" / "+roundTo(m.power.capacity,0)+" L";}};
 fill(true);
 for(K kind:{K::ReedReclaimer,K::SandFilter,K::MembraneFilter,K::UVSterilizer})for(auto& [p,m]:f.machines)if(m.kind==kind&&m.enabled&&!m.power.tripped&&!m.waterProcess.rinse&&!m.waterProcess.purge){if(kind==K::SandFilter&&m.waterProcess.captured>=799.99){m.status="Filter media full / backwash required";continue;}if(kind==K::ReedReclaimer)stage(f,p,m,pull,E::Greywater,E::RawWater,.04,.9);if(kind==K::SandFilter)stage(f,p,m,pull,E::RawWater,E::FilteredWater,.025);if(kind==K::MembraneFilter)stage(f,p,m,pull,E::FilteredWater,E::PurifiedWater,.18,m.waterProcess.recovery);if(kind==K::UVSterilizer)stage(f,p,m,pull,E::PurifiedWater,E::CleanWater,.03*m.waterProcess.uvDose);}
 fill(false);
 for(auto& [p,m]:f.machines)if(m.enabled&&!m.power.tripped){auto& w=m.waterProcess;if(m.kind==K::WaterOutlet&&!w.purge){auto held=storedWater(m,E::CleanWater);if(!acceptableWater(held)&&held.volume>eps){m.status="QUALITY HOLD / inspect solids, salt and biology";continue;}double wanted=std::min(m.power.rate*m.power.throttle,std::max(0.,m.dose-f.water));if(wanted<=eps){m.status="Factory water buffer at target";continue;}auto packet=pull(p,E::CleanWater,std::min(wanted,room(m,E::CleanWater)),0,false);putWater(m,E::CleanWater,packet);held=storedWater(m,E::CleanWater);w.sample=held;if(!acceptableWater(held)){m.status=held.volume>eps?"QUALITY HOLD / inspect solids, salt and biology":"Needs treated water and AC";continue;}double amount=std::min(wanted,held.volume);if(!power(p,amount*.01)){m.status="Clean water held / needs connected AC";continue;}auto delivered=removeWaterLoad(m,E::CleanWater,amount);m.power.store[ei(E::CleanWater)]-=amount;f.water+=amount;addWaterParcel(f.waterworks.supply,delivered);f.waterworks.delivered+=amount;w.delivered+=amount;m.power.flow=amount;m.status="Supplied farm + factory / "+roundTo(amount,2)+" L";}
  if(m.kind==K::WaterAnalyzer){auto source=f.at(nextPosition(p,m.dir));if(source&&isWaterMachine(source->kind)&&power(p,.05)){w.sample=waterSample(*source);m.sensorTick=f.tick;m.sensorValue=w.sample.volume>eps?w.sample.salt/w.sample.volume:0;m.sensorHistory.push_back(m.sensorValue);if(m.sensorHistory.size()>80)m.sensorHistory.erase(m.sensorHistory.begin());m.status="Measured / salt "+roundTo(m.sensorValue,2)+" per L";}else m.status="Point at water equipment / supply AC";}
 }
}
Blueprint Factory::waterWorksBlueprint(){using E=EnergyMedium;Blueprint b;b.name="Waterworks / collection to reuse";b.width=7;b.height=5;
 auto add=[&](Position p,K kind,int d=0)->Machine&{Machine m;m.kind=kind;m.dir=d;initialisePowerDevice(m);b.parts.push_back({p,m});return b.parts.back().machine;};
 add({0,1},K::BoreholePump);add({1,0},K::RainCollector,1);add({1,1},K::RawWaterTank);add({2,1},K::SandFilter);add({3,1},K::MembraneFilter);add({4,1},K::UVSterilizer);add({5,1},K::CleanWaterTank);add({6,1},K::WaterOutlet);add({3,2},K::ConcentrateTank);add({1,2},K::WaterAnalyzer,3);add({0,3},K::GreywaterDrain);add({1,3},K::ReedReclaimer);
 std::map<std::pair<Position,E>,PowerLink> links;auto node=[&](Position p,E e)->PowerLink&{auto [i,fresh]=links.emplace(std::make_pair(p,e),PowerLink{});if(fresh){i->second.medium=e;i->second.rating=30;i->second.ports=0;}return i->second;};
 auto run=[&](E e,std::initializer_list<Position> points){auto i=points.begin();Position p=*i++;node(p,e);for(;i!=points.end();++i)while(p!=*i){int d=p.x!=i->x?(p.x<i->x?0:2):(p.y<i->y?1:3);auto q=nextPosition(p,d);node(p,e).ports|=1<<d;node(q,e).ports|=1<<((d+2)%4);p=q;}};
 run(E::RawWater,{{2,3},{2,2},{1,2}});node({1,2},E::RawWater).ports|=8;node({2,3},E::RawWater).ports|=4;
 run(E::Concentrate,{{2,2},{3,2}});node({2,2},E::Concentrate).ports|=8;
 run(E::Concentrate,{{1,4},{3,4},{3,3}});node({1,4},E::Concentrate).ports|=8;node({3,3},E::Concentrate).ports|=8;
 run(E::Concentrate,{{4,2},{6,2}});node({4,2},E::Concentrate).ports|=4;node({5,2},E::Concentrate).ports|=8;node({6,2},E::Concentrate).ports|=8;
 for(const auto& [key,l]:links)b.links.push_back({key.first,l});return b;
}
bool installWaterWorksIn(FactoryState& f){if(std::any_of(f.machines.begin(),f.machines.end(),[](const auto& p){return isWaterMachine(p.second.kind);}))return false;auto b=Factory::waterWorksBlueprint();auto o=waterWorksOrigin;
 for(int y=0;y<b.height;y++)for(int x=0;x<b.width;x++){Position p{o.x+x,o.y+y};if(f.machines.count(p)||f.energyLinks.count(p))return false;}
 for(const auto& part:b.parts)f.machines.emplace(Position{o.x+part.at.x,o.y+part.at.y},part.machine);
 for(const auto& n:b.links)f.energyLinks[{o.x+n.at.x,o.y+n.at.y}].push_back(n.link);
 for(int x=-10;x<=-9;x++){auto& ls=f.energyLinks[{x,7}];auto i=std::find_if(ls.begin(),ls.end(),[](const auto& l){return l.medium==E::AC;});if(i==ls.end()){PowerLink l;l.medium=E::AC;l.rating=100;l.ports=5;ls.push_back(l);}else i->ports|=4;}
 if(std::none_of(f.blueprints.begin(),f.blueprints.end(),[&](const auto& p){return p.name==b.name;}))f.blueprints.push_back(b);return true;
}
}
