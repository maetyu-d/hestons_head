#include "ReagentWorks.h"
#include "Power.h"
#include "WaterWorks.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>
#include <sstream>
namespace aftertaste {
namespace {
using K=MachineKind;using E=EnergyMedium;constexpr double eps=1e-8;
size_t ix(E e){return size_t(e);}bool storage(K k){return k==K::BatteryRack||k==K::Flywheel||k==K::SteamAccumulator||k==K::GasHolder||k==K::HeatTank;}
bool electric(E e){return e==E::AC||e==E::DC;}
E storageMedium(K k){return k==K::BatteryRack?E::DC:k==K::Flywheel?E::Shaft:k==K::SteamAccumulator?E::Steam:k==K::GasHolder?E::Gas:E::Heat;}
}
bool isEnergyConduit(K k){return (k>=K::PowerCable&&k<=K::CoolingPipe)||k==K::VacuumHose||k==K::InstrumentLead||k==K::SignalLead||(k>=K::RawWaterPipe&&k<=K::ConcentratePipe);}
bool isEnergyMachine(K k){return (k>=K::SolarPanel&&k<=K::ReagentRecovery&&!isEnergyConduit(k))||k==K::WaterDiverter;}
bool isContactService(E e){return e==E::AC||e==E::DC||e==E::Water||e==E::Cooling||e==E::Vacuum||e==E::Instrument||e==E::RawWater||e==E::CleanWater||e==E::Concentrate;}
bool powerContactSupports(K a,K b,E e){if(isReagentMachine(a)||isReagentMachine(b))return e==E::AC&&isEnergyMachine(a)&&isEnergyMachine(b)&&a!=K::SolarPanel&&b!=K::SolarPanel;if(isWaterStream(e))return a==b&&isWaterStorage(a)&&((a==K::RawWaterTank&&e==E::RawWater)||(a==K::CleanWaterTank&&e==E::CleanWater)||(a==K::ConcentrateTank&&e==E::Concentrate));return isEnergyMachine(a)&&isEnergyMachine(b)&&isContactService(e)&&((a!=K::SolarPanel&&b!=K::SolarPanel)||e==E::DC);}
bool acceptsEnergyFuel(K k){return acceptsReagentMaterial(k)||k==K::Macerator||k==K::BiomassBoiler||k==K::AnaerobicDigester;}
E conduitMedium(K k){if(k==K::SignalLead)return E::Control;if(k>=K::RawWaterPipe&&k<=K::ConcentratePipe)return E(int(E::RawWater)+int(k)-int(K::RawWaterPipe));return k==K::VacuumHose?E::Vacuum:k==K::InstrumentLead?E::Instrument:E(int(k)-int(K::PowerCable));}K energyConduit(E e){if(e==E::Control)return K::SignalLead;if(e>=E::RawWater)return K(int(K::RawWaterPipe)+int(e)-int(E::RawWater));return e==E::Vacuum?K::VacuumHose:e==E::Instrument?K::InstrumentLead:K(int(K::PowerCable)+int(e));}
std::string energyName(E e){static const std::array<const char*,EnergyCount> names={"AC electricity","DC electricity","shaft power","live steam","exhaust steam","raw biogas","clean gas","return water","recovered heat","cooling","lab vacuum","instrument signal","raw water","filtered water","membrane permeate","clean water","greywater","concentrate","control signal"};return names.at(ix(e));}
double solarDaylight(int tick){return std::max(0.,std::sin((tick%360+60)*6.283185307/360))*farmSeasonDaylight(tick);}
double windStrength(int tick){return clamp(.57+.23*std::sin(tick*.037)+.14*std::sin(tick*.113),.12,.97);}
std::vector<EnergyPort> energyPorts(const Machine& m){
 std::vector<EnergyPort> p;auto add=[&](E medium,int d,bool output){p.push_back({medium,(d+m.dir)%4,output});};
 switch(m.kind){
 case K::CuriosityPile:add(E::Heat,0,true);add(E::Cooling,1,false);add(E::Instrument,3,false);break;
 case K::StarChamber:add(E::AC,2,false);add(E::Heat,0,true);add(E::Vacuum,3,false);add(E::Cooling,1,false);add(E::Instrument,3,false);break;
 case K::LabVacuumPump:add(E::AC,2,false);add(E::Vacuum,0,true);break;
 case K::QuenchBath:add(E::AC,2,false);add(E::Cooling,0,true);break;
 case K::Calorimeter:add(E::Heat,2,false);add(E::DC,0,true);add(E::Cooling,1,false);break;
 case K::ExperimentConsole:add(E::AC,2,false);add(E::Instrument,0,true);break;
 case K::SolarPanel:add(E::DC,0,true);break;
 case K::WindRotor:add(E::Shaft,0,true);break;
 case K::Gearbox:add(E::Shaft,2,false);add(E::Shaft,0,true);break;
 case K::Alternator:add(E::Shaft,2,false);add(E::AC,0,true);break;
 case K::BiomassBoiler:add(E::AC,3,false);add(E::Water,3,false);add(E::Steam,0,true);add(E::Heat,1,true);break;
 case K::SteamTurbine:add(E::Steam,2,false);add(E::Shaft,0,true);add(E::Exhaust,1,true);break;
 case K::PowerCondenser:add(E::Exhaust,2,false);add(E::Water,0,true);add(E::Heat,1,true);break;
 case K::FeedPump:add(E::AC,3,false);add(E::Water,0,true);break;
 case K::HeatExchanger:add(E::Heat,2,false);add(E::Heat,0,true);break;
 case K::AnaerobicDigester:add(E::RawGas,0,true);add(E::Heat,3,false);break;
 case K::GasScrubber:add(E::RawGas,2,false);add(E::Gas,0,true);add(E::Water,3,false);break;
 case K::GasEngine:add(E::Gas,2,false);add(E::Shaft,0,true);add(E::Heat,1,true);add(E::AC,3,false);break;
 case K::Inverter:add(E::DC,2,true);add(E::AC,0,true);break;
 case K::Switchboard:case K::PowerRelay:add(E::AC,2,false);add(E::AC,0,false);add(E::AC,3,false);add(E::AC,1,false);break;
 case K::GridInlet:add(E::AC,0,true);break;
 case K::CoolingPlant:add(E::AC,2,false);add(E::Heat,3,false);add(E::Cooling,0,true);break;
 case K::HeatTank:add(E::Heat,2,true);add(E::Heat,0,true);add(E::Heat,3,true);break;
 case K::BatteryRack:case K::Flywheel:case K::SteamAccumulator:case K::GasHolder:{E e=storageMedium(m.kind);add(e,2,true);add(e,0,true);break;}
 default:if(isReagentMachine(m.kind)){for(int d=0;d<4;d++)add(E::AC,d,false);return p;}if(isWaterMachine(m.kind))return waterPorts(m);break;
 }return p;
}
std::vector<MachineSpec> energyMachineSpecs(){std::vector<MachineSpec> specs={
 {K::PowerCable,"AC power cable","AC","Drag across the floor, beneath equipment. Connected generators and rated cables supply actual machine demand.","#d2a63f",3,0,0,0},
 {K::DCCable,"DC cable","DC","Bridge gaps between solar banks, batteries and inverter DC terminals. Touching equipment connects directly; factory AC remains a separate circuit.","#52aaa2",3,0,0,0},
 {K::DriveShaft,"Drive shaft","SH","Carries actual rotational energy between rotors, engines, gearboxes, flywheels and alternators.","#809398",5,0,0,0},
 {K::SteamPipe,"Live steam pipe","ST","Carries boiler steam into accumulators and turbines. Exhaust uses a separate return pipe.","#d88055",4,0,0,0},
 {K::ExhaustPipe,"Exhaust steam pipe","EX","Returns low-energy turbine exhaust to a condenser. A blocked return stops the turbine.","#b5aaa3",4,0,0,0},
 {K::RawGasPipe,"Raw biogas pipe","RG","Routes actual digester gas into a scrubber before engine use.","#a28c5e",4,0,0,0},
 {K::GasPipe,"Clean gas pipe","GA","Carries scrubbed gas to holders and engines. Fuel is finite and consumed by generation.","#89aa62",4,0,0,0},
 {K::ReturnPipe,"Return water pipe","RW","Connects feed pumps and condensers to boilers. Recovered condensate reduces makeup-water demand.","#579ec4",4,0,0,0},
 {K::HeatPipe,"Heat recovery pipe","HR","Carries measured recovered heat to tanks, exchangers, digesters and factory heaters.","#d36445",4,0,0,0},
 {K::CoolingPipe,"Cooling circuit","CL","Connects a cooling plant to cold stores and chilling equipment.","#65bfd9",4,0,0,0},
 {K::SolarPanel,"Solar array","PV","Shares DC through every touching panel edge. Build rows or blocks and dock a battery or inverter directly. Day, season and nearby tall obstructions affect generation.","#526fa0",95,0,0,0},
 {K::WindRotor,"Wind rotor","WT","Turns shared weather into shaft power. Exposure, gusts, throttle and braking change its output.","#97b9b8",110,0,0,0},
 {K::Gearbox,"Variable gearbox","GB","Dock between a rotor and alternator on matching shaft ports, or bridge a gap with a shaft. Ratio trades rotational speed against torque.","#8d9fa1",48,0,0,0},
 {K::Alternator,"Alternator","AG","Converts connected shaft energy into AC electricity. Requires a real mechanical source.","#c8974b",85,0,0,0},
 {K::BiomassBoiler,"Biomass boiler","BB","Burns real organic food or residues. Wet fuel reduces heat yield; live steam needs return water and warm-up.","#ac6c49",140,0,0,0},
 {K::SteamAccumulator,"Steam accumulator","SA","Dock directly between boiler and turbine on matching steam ports. Stores a finite reserve with a separate discharge rate and insulation losses.","#b58c6b",90,0,0,0},
 {K::SteamTurbine,"Steam turbine","TU","Turns live steam into shaft power and equal-mass exhaust. Needs a clear exhaust route.","#8b9ba7",140,0,0,0},
 {K::PowerCondenser,"Return condenser","RC","Condenses turbine exhaust into reusable boiler water and recoverable low-grade heat.","#70acb7",85,0,0,0},
 {K::FeedPump,"Boiler feed pump","FP","Introduces measured makeup water into the adjoining rack manifold. Touching chassis share return water; pipes bridge gaps. Stops filling when the connected loop is full.","#5f9ab2",42,0,0,0},
 {K::HeatExchanger,"Heat recovery exchanger","HX","Makes recovered heat available to adjacent factory heaters. Its own output has conversion losses.","#c57f56",65,0,0,0},
 {K::Macerator,"Residue macerator","MC","Processes actual food batches for a boiler or digester. Connect conveyors to its food inlet and outlet.","#879369",55,2,0,0},
 {K::AnaerobicDigester,"Anaerobic digester","AD","Converts organic compounds from a real incoming batch into raw gas. Heat affects conversion; retained digestate leaves the side port.","#879a5c",125,18,0,0},
 {K::GasScrubber,"Biogas scrubber","GS","Removes a measured fraction of raw gas as wet contaminants, supplying clean engine fuel. Uses return-loop water.","#9bad7c",65,0,0,0},
 {K::GasHolder,"Gas holder","GH","Stores actual clean fuel gas. The floating roof follows its stored mass.","#92aa6e",70,0,0,0},
 {K::GasEngine,"Gas engine","GE","Consumes clean gas, produces shaft power and useful heat. Starting requires a small AC reserve; a relay can dispatch it automatically.","#819588",125,0,0,0},
 {K::BatteryRack,"Battery rack","BT","Dock beside panels, other batteries or an inverter to share DC without local wiring. Capacity, charging losses and discharge rate remain finite.","#5b8f96",100,0,0,0},
 {K::Flywheel,"Flywheel","FW","Dock between an engine and alternator on matching shaft ports. Stores rotational energy for load changes, with friction and a finite shaft rating.","#839399",70,0,0,0},
 {K::Inverter,"Bidirectional inverter","IV","Converts DC storage and solar output to AC, or charges batteries from an AC surplus. Conversion always loses energy.","#7b8eae",85,0,0,0},
 {K::Switchboard,"Switchboard","SB","Joins rated AC branches. Open its breaker to isolate the circuit; overload heat can trip it.","#ad9569",65,0,0,0},
 {K::PowerRelay,"Reserve relay","RL","Starts connected automatic engines below its low battery threshold and stops them above its high threshold.","#b5975c",45,0,0,0},
 {K::GridInlet,"Metered grid inlet","GI","Optional imported AC with a per-tick limit. Draws only from the delivered electricity reserve; economy charges actual imports.","#a7a38b",40,0,0,0},
 {K::HeatTank,"Insulated heat tank","HTK","Stores recovered heat with standing losses. Side ports dock into recovery equipment; the top service tap supplies a short factory heat header.","#b58669",75,0,0,0},
 {K::CoolingPlant,"Recovery cooling plant","CP","Uses AC electricity and optional recovered heat to supply a connected cooling circuit.","#6aa7bc",115,0,0,0},
 {K::CuriosityPile,"Sealed curiosity pile","NU","A tabletop nuclear experiment in a shielded bell jar. A finite fictional oddium cartridge releases heat; a docked recorder and quench bath keep it stable. Scram stops excitation, with residual heat left to cool.","#8caa64",180,0,0,0},
 {K::StarChamber,"Jam-jar star chamber","STAR","A fictional pulsed nuclear bench. Consumes AC excitation and sealed cartridge charge. Vacuum, resonance tuning and cooling determine heat yield; poor tuning can cost more electricity than the calorimeter recovers.","#ac80b5",210,0,0,0},
 {K::LabVacuumPump,"Vacuum bench and cold trap","VAC","An oil pump and glass trap supply adjoining bench apparatus by contact. A hose can bridge a gap. The chamber loses vacuum when the pump is detached or isolated.","#97a6b3",70,0,0,0},
 {K::QuenchBath,"Recirculating quench bath","QB","A transparent cooling bath with immersed coils. Uses AC and a little supplied water to remove heat from experiments and the calorimeter.","#6db8c7",90,0,0,0},
 {K::Calorimeter,"Twin-bulb calorimeter","CAL","Measures incoming experimental heat and converts part into DC. A cold reference improves conversion; the remaining heat is accounted as loss. Connect DC to a battery or inverter.","#ce9f67",130,0,0,0},
 {K::ExperimentConsole,"Experiment recorder","SCOPE","A dockable oscilloscope and chart recorder. Powers experiment interlocks and logs heat, electrical draw, recovered DC and stability every 24 ticks. Change one setting and compare measured runs.","#8baf9a",85,0,0,0},
 {K::VacuumHose,"Glass vacuum line","VH","Square glass service tubing for the vacuum pump and star chamber. Crosses electrical circuits without joining them.","#9e8db7",4,0,0,0},
 {K::InstrumentLead,"Instrument lead","SIG","Routes the recorder's interlock signal to experimental chambers. An open lead disarms excitation rather than silently continuing.","#b58eaa",3,0,0,0}};auto water=waterMachineSpecs();specs.insert(specs.end(),water.begin(),water.end());auto reagents=reagentMachineSpecs();specs.insert(specs.end(),reagents.begin(),reagents.end());return specs;}
void initialisePowerDevice(Machine& m){
 auto& p=m.power;p=PowerState{};p.rate=12;p.capacity=120;
 switch(m.kind){case K::CuriosityPile:p.rate=60;p.capacity=180;p.throttle=.65;p.lab.tuning=.55;break;case K::StarChamber:p.rate=30;p.capacity=180;p.throttle=.65;p.lab.tuning=.65;m.duration=6;break;case K::LabVacuumPump:p.rate=4;break;case K::QuenchBath:p.rate=12;break;case K::Calorimeter:p.rate=100;p.capacity=250;break;case K::ExperimentConsole:p.rate=8;break;case K::SolarPanel:p.rate=22;break;case K::WindRotor:p.rate=32;break;case K::Alternator:p.rate=30;break;case K::BiomassBoiler:p.rate=5;p.capacity=180;m.temperature=160;break;case K::SteamAccumulator:p.capacity=80;p.rate=10;break;case K::SteamTurbine:p.rate=5;break;case K::PowerCondenser:p.rate=6;break;case K::FeedPump:p.rate=4;p.capacity=12;break;case K::AnaerobicDigester:p.rate=8;p.capacity=120;m.temperature=38;break;case K::GasEngine:p.rate=5;p.capacity=100;break;case K::BatteryRack:p.capacity=800;p.rate=24;break;case K::Flywheel:p.capacity=100;p.rate=35;break;case K::GasHolder:p.capacity=180;p.rate=10;break;case K::Inverter:p.rate=40;break;case K::Switchboard:p.rate=100;p.capacity=100;break;case K::HeatTank:p.capacity=600;p.rate=35;break;case K::GridInlet:p.rate=30;break;case K::CoolingPlant:p.rate=8;break;default:if(isReagentMachine(m.kind))initialiseReagentDevice(m);else if(isWaterMachine(m.kind))initialiseWaterDevice(m);break;}
}
void clearPowerInventory(Machine& m){auto& p=m.power;p.store.fill(0);p.temperature=20;p.workCredit=p.rpm=p.flow=p.generated=p.delivered=p.demand=p.heat=0;p.voltage=1;p.tripped=false;p.history.clear();double tuning=p.lab.tuning;p.lab=NuclearState{};p.lab.tuning=tuning;clearWaterInventory(m);clearReagentInventory(m);}
bool validPowerState(const PowerState& p){for(double v:p.store)if(!std::isfinite(v)||v<0||v>1e9)return false;for(double v:{p.workCredit,p.rate,p.capacity,p.throttle,p.temperature,p.rpm,p.flow,p.generated,p.delivered,p.demand,p.voltage,p.heat,p.low,p.high})if(!std::isfinite(v)||v<0||v>1e9)return false;for(double v:{p.lab.fuel,p.lab.spent,p.lab.tuning,p.lab.flux,p.lab.vacuum,p.lab.stability,p.lab.cooling,p.lab.heatTotal,p.lab.inputTotal,p.lab.dcTotal,p.lab.lastHeat,p.lab.lastInput,p.lab.lastDC})if(!std::isfinite(v)||v<0||v>1e12)return false;
 if(p.lab.tuning>1||p.lab.flux>1||p.lab.vacuum>1||p.lab.stability>1||p.lab.cooling>1||p.lab.fuel>100||p.lab.pulse<0||p.lab.pulse>1000000||p.lab.sampleTicks<0||p.lab.sampleTicks>=24||p.lab.trials.size()>16)return false;
 for(const auto& t:p.lab.trials){if(t.settings.size()>240||t.tick<0||t.stability<0||t.stability>1)return false;for(double v:{t.heat,t.draw,t.dc,t.stability})if(!std::isfinite(v)||v<0||v>1e12)return false;}
 if(p.rate>1000||p.capacity>1e7||p.throttle>1||p.low>p.high||p.high>1||p.priority<0||p.priority>2||p.history.size()>80)return false;for(auto v:p.history)if(!std::isfinite(v)||v<0||v>1e9)return false;return true;}
bool validPowerLink(const PowerLink& p){return p.ports>=0&&p.ports<=15&&int(p.medium)>=0&&int(p.medium)<int(E::Count)&&std::isfinite(p.rating)&&p.rating>=1&&p.rating<=1000&&std::isfinite(p.flow)&&p.flow>=0&&p.flow<1e9&&std::isfinite(p.heat)&&p.heat>=0&&p.heat<=10&&std::isfinite(p.overload)&&p.overload>=0&&p.overload<1e9&&p.dir>=0&&p.dir<4;}
std::string powerContents(const Machine& m){std::string s;for(size_t n=0;n<EnergyCount;n++)if(m.power.store[n]>.01){if(!s.empty())s+=" / ";s+=energyName(E(n))+" "+roundTo(m.power.store[n],1);}return s.empty()?"No stored energy or working fluid":s;}

class PowerRuntime {
public:
 struct Graph {std::map<Position,int> ports,contacts;std::map<Position,int> component;};
 struct Transfer {Position source;E medium;double debit=0,credit=0;std::vector<Position> path;};
 struct Budget {std::map<std::pair<Position,E>,double> balance,loads,used;};
 std::map<std::pair<Position,Position>,std::array<double,EnergyCount>> contactTransfers;
 double lastShaftRPM=0;Factory& f;std::array<Graph,EnergyCount> graphs;std::map<Position,std::vector<Transfer>> allocations;std::map<Position,double> remaining;std::map<std::pair<Position,E>,double> spent;
 explicit PowerRuntime(Factory& factory):f(factory){
  for(auto& [p,links]:f.energyLinks)for(auto& link:links){link.flow=link.overload=0;if(link.enabled&&!link.tripped)graphs[ix(link.medium)].ports[p]=link.ports;}
  for(auto& [p,m]:f.machines){m.power.flow=m.power.generated=m.power.delivered=m.power.demand=0;m.power.voltage=0;if(!isEnergyMachine(m.kind)||!m.enabled||m.power.tripped)continue;std::array<int,EnergyCount> masks{};for(auto port:energyPorts(m))masks[ix(port.medium)]|=1<<port.direction;for(size_t n=0;n<EnergyCount;n++)if(masks[n])graphs[n].ports[p]=masks[n];}
  // Side-by-side chassis share separate bench services. A contact only exists
  // on an occupied cardinal edge; cables still use the actual external ports.
  for(const auto& [p,m]:f.machines)if(isEnergyMachine(m.kind)&&m.enabled&&!m.power.tripped)for(int d=0;d<4;d++){
   auto neighbor=f.at(nextPosition(p,d));if(!neighbor||!isEnergyMachine(neighbor->kind)||!neighbor->enabled||neighbor->power.tripped)continue;
   for(size_t n=0;n<EnergyCount;n++)if(powerContactSupports(m.kind,neighbor->kind,E(n))){graphs[n].ports[p]|=1<<d;graphs[n].contacts[p]|=1<<d;}
  }
  for(const auto& [p,m]:f.machines)if(m.kind==K::Switchboard&&(!m.enabled||m.power.tripped))graphs[ix(E::AC)].ports.erase(p);
  for(auto& graph:graphs){std::set<Position> unseen;for(auto& [p,mask]:graph.ports)unseen.insert(p);int id=0;while(!unseen.empty()){auto first=*unseen.begin();std::vector<Position> open{first};unseen.erase(first);graph.component[first]=id;for(size_t n=0;n<open.size();n++)for(int d=0;d<4;d++){Position q=nextPosition(open[n],d);auto other=graph.ports.find(q);if(other==graph.ports.end()||!(graph.ports[open[n]]&(1<<d))||!(other->second&(1<<((d+2)%4))))continue;if(unseen.erase(q)){open.push_back(q);graph.component[q]=id;}}id++;}}
 }
 bool connected(Position a,Position b,E e)const{auto& c=graphs[ix(e)].component;auto x=c.find(a),y=c.find(b);return x!=c.end()&&y!=c.end()&&x->second==y->second;}
 Position tap(Position p,E e)const{auto& g=graphs[ix(e)];if(g.ports.count(p))return p;for(int d=0;d<4;d++){auto q=nextPosition(p,d);auto node=g.ports.find(q);if(node!=g.ports.end()&&(node->second&(1<<((d+2)%4))))return q;}return {1000001,1000001};}
 bool output(const Machine& m,E e)const{for(auto p:energyPorts(m))if(p.medium==e&&p.output)return true;return false;}
 Budget budget()const{Budget b;for(auto& [p,m]:f.machines)if(isEnergyMachine(m.kind)&&m.enabled&&!m.power.tripped)for(size_t n=0;n<EnergyCount;n++)if(output(m,E(n))){b.balance[{p,E(n)}]=m.power.store[n];auto i=spent.find({p,E(n)});b.used[{p,E(n)}]=i==spent.end()?0:i->second;}for(auto& [p,links]:f.energyLinks)for(auto& link:links)b.loads[{p,link.medium}]=link.flow;return b;}
 std::vector<Transfer> plan(Position sink,E e,double wanted,Budget& b,bool skipStorage=false){
  std::vector<Transfer> result;auto start=tap(sink,e);auto& graph=graphs[ix(e)];if(!graph.ports.count(start))return result;
  auto junction=[](K k){return k==K::Switchboard||k==K::PowerRelay||isReagentMachine(k);};
  auto bidirectional=[](K k){return storage(k)||isWaterStorage(k)||k==K::Inverter;};
  int sinkMask=15;
  if(auto m=f.at(sink);m&&isEnergyMachine(m->kind)&&!junction(m->kind)){
   sinkMask=0;for(auto port:energyPorts(*m))if(port.medium==e&&(!port.output||bidirectional(m->kind)))sinkMask|=1<<port.direction;
   if(isContactService(e))sinkMask|=graph.contacts[sink];
   if(!sinkMask)return result;start=sink;
  }
  std::map<Position,Position> parent;std::vector<Position> open{start};parent[start]=start;
  for(size_t n=0;n<open.size();n++){
   Position current=open[n];auto machine=f.at(current);
   // Process converters remain endpoints. Bench-service backplanes may relay
   // one medium through touching equipment, without converting that medium.
   if(current!=sink&&machine&&isEnergyMachine(machine->kind)&&!junction(machine->kind)){
    bool ownMedium=false;for(auto port:energyPorts(*machine))if(port.medium==e)ownMedium=true;
    if(ownMedium&&!(isContactService(e)&&graph.contacts[current]!=0))continue;
   }
   int mask=current==sink?sinkMask:graph.ports[current];
   for(int d=0;d<4;d++){Position q=nextPosition(current,d);auto other=graph.ports.find(q);if(other==graph.ports.end()||parent.count(q)||!(mask&(1<<d))||!(other->second&(1<<((d+2)%4))))continue;
    auto device=f.at(q);if(device&&isEnergyMachine(device->kind)&&!junction(device->kind)){
     bool ownMedium=false,sourcePort=false;for(auto port:energyPorts(*device))if(port.medium==e){ownMedium=true;if(port.output&&port.direction==(d+2)%4)sourcePort=true;}
     if(ownMedium&&!sourcePort&&!(isContactService(e)&&(graph.contacts[q]&(1<<((d+2)%4)))))continue;
    }
    auto link=f.energyLink(q,e);if(link&&b.loads[{q,e}]>=link->rating-eps)continue;parent[q]=current;open.push_back(q);
   }
  }
  for(auto source:open){if(wanted<=eps)break;auto m=f.at(source);if(!m||source==sink||!m->enabled||m->power.tripped||!output(*m,e)||(skipStorage&&(storage(m->kind)||storesWaterMedium(m->kind,e))))continue;auto key=std::make_pair(source,e);double sourceRate=m->power.rate;if(e==E::Shaft&&m->kind==K::SteamTurbine)sourceRate*=4;if(e==E::Shaft&&m->kind==K::GasEngine)sourceRate*=4.2;if(e==E::Heat&&m->kind==K::GasEngine)sourceRate*=5;if(e==E::Heat&&m->kind==K::PowerCondenser)sourceRate*=8;double available=std::min(b.balance[key],std::max(0.,sourceRate-b.used[key]));if(available<=eps)continue;
   std::vector<Position> path;Position cursor=source;while(true){path.push_back(cursor);if(cursor==start)break;cursor=parent.at(cursor);}double room=available;
   for(auto p:path)if(auto link=f.energyLink(p,e)){double free=std::max(0.,link->rating-b.loads[{p,e}]);if(wanted>free&&electric(e))link->overload=std::max(link->overload,wanted/link->rating);room=std::min(room,free);}
   double efficiency=electric(e)?std::max(.85,1.-.0007*path.size()):1.;double debit=std::min(room,wanted/efficiency),credit=debit*efficiency;if(credit<=eps)continue;
   b.balance[key]-=debit;b.used[key]+=debit;for(auto p:path)b.loads[{p,e}]+=debit;result.push_back({source,e,debit,credit,path});wanted-=credit;
  }return result;
 }
 void commit(const Transfer& tr,double fraction=1){auto m=f.at(tr.source);if(!m)return;double debit=tr.debit*fraction,credit=tr.credit*fraction;m->power.store[ix(tr.medium)]=std::max(0.,m->power.store[ix(tr.medium)]-debit);m->power.delivered+=debit;spent[{tr.source,tr.medium}]+=debit;f.grid.loss+=debit-credit;
  for(size_t n=0;n<tr.path.size();n++){Position p=tr.path[n];
   if(n+1<tr.path.size()){Position q=tr.path[n+1];auto a=f.at(p),b=f.at(q);if(a&&b&&isEnergyMachine(a->kind)&&isEnergyMachine(b->kind)){auto key=q<p?std::make_pair(q,p):std::make_pair(p,q);contactTransfers[key][ix(tr.medium)]+=q<p?-debit:debit;}}
   if(auto link=f.energyLink(p,tr.medium)){link->flow+=debit;if(n+1<tr.path.size()){auto q=tr.path[n+1];link->dir=q.x>p.x?0:q.y>p.y?1:q.x<p.x?2:3;}}}
 }
 double take(Position p,E e,double amount,bool partial=false,bool skipStorage=false){auto b=budget();auto transfers=plan(p,e,amount,b,skipStorage);double total=0;for(auto& x:transfers)total+=x.credit;if(!partial&&total+eps<amount)return 0;lastShaftRPM=0;for(auto& x:transfers){if(e==E::Shaft){auto source=f.at(x.source);if(source)lastShaftRPM+=source->power.rpm*x.credit/std::max(eps,total);}commit(x);}return total;}
 double total(E e)const{double sum=0;for(auto& [p,m]:f.machines)if(m.enabled)sum+=m.power.store[ix(e)];return sum;}
 void put(Machine& m,E e,double value){m.power.store[ix(e)]=std::min(m.power.capacity,m.power.store[ix(e)]+std::max(0.,value));}
 void inverter(Position p,Machine& m,bool early){auto& s=m.power;double limit=s.rate*s.throttle;
  if(early){double got=take(p,E::DC,std::min(limit,std::max(0.,s.capacity-s.store[ix(E::AC)])/ .94),true);put(m,E::AC,got*.94);f.grid.loss+=got*.06;s.flow=got;m.status=got>eps?"DC to AC / "+roundTo(got*.94,1):"Waiting for DC or shaft generation";}
  else if(total(E::DC)<1){double got=take(p,E::AC,std::min(limit,std::max(0.,s.capacity-s.store[ix(E::DC)])),true,true);put(m,E::DC,got*.91);f.grid.loss+=got*.09;if(got>eps)m.status="AC surplus to DC / "+roundTo(got*.91,1);}
 }
 WaterParcel pullWater(Position p,E e,double wanted,double cost,bool skipStorage){
  auto b=budget();auto transfers=plan(p,e,wanted,b,skipStorage);double volume=0;for(const auto& tr:transfers)volume+=tr.credit;if(volume<1e-8)return {};
  auto ac=plan(p,E::AC,volume*cost,b);double supplied=0;for(const auto& tr:ac)supplied+=tr.credit;if(supplied+1e-8<volume*cost)return {};
  for(const auto& tr:ac)commit(tr);WaterParcel result;
  for(const auto& tr:transfers){auto source=f.at(tr.source);if(!source)continue;auto parcel=removeWaterLoad(*source,e,tr.debit);parcel.volume=tr.credit;addWaterParcel(result,parcel);commit(tr);}return result;
 }
 void nuclearBench();
 void recordLabTrials();
 void generate(){
  for(auto& [p,m]:f.machines)if(isEnergyMachine(m.kind)){auto& s=m.power;s.temperature=std::max(20.,s.temperature-(s.temperature-20)*.01);if(!m.enabled||s.tripped){m.status=s.tripped?"Breaker tripped / reset after cooling":"Isolated / disabled";continue;}
   if(m.kind==K::PowerRelay){double charge=0,capacity=0;std::set<Position> banks;for(const auto& [q,inverter]:f.machines)if(inverter.kind==K::Inverter&&inverter.enabled&&connected(p,q,E::AC))for(const auto& [r,bank]:f.machines)if(bank.kind==K::BatteryRack&&bank.enabled&&connected(q,r,E::DC)&&banks.insert(r).second){charge+=bank.power.store[ix(E::DC)];capacity+=bank.power.capacity;}double soc=capacity>0?charge/capacity:0;if(soc<s.low)s.armed=true;else if(soc>s.high)s.armed=false;for(auto& [q,engine]:f.machines)if((engine.kind==K::GasEngine||engine.kind==K::BiomassBoiler)&&engine.automatic&&connected(p,q,E::AC))engine.power.armed=s.armed;m.status=std::string(s.armed?"Reserve requested":"Reserve stopped")+" / battery "+roundTo(soc*100,0)+"%";}
   if(m.kind==K::SolarPanel){double shade=1;for(int d:{1,2,3}){auto block=f.at(nextPosition(p,d));if(block&&(block->kind==K::Wall||block->kind==K::BiomassBoiler||block->kind==K::GasHolder))shade*=.62;}f.grid.loss+=s.store[ix(E::DC)];s.store[ix(E::DC)]=s.rate*solarDaylight(f.tick)*shade*s.throttle;s.generated=s.store[ix(E::DC)];f.grid.generated+=s.generated;m.status="Solar DC / "+roundTo(s.generated,1)+" / exposure "+roundTo(shade*100,0)+"%";}
   if(m.kind==K::WindRotor){double exposure=1;for(int n=1;n<4;n++){auto block=f.at({p.x-n,p.y});if(block&&(block->kind==K::Wall||block->kind==K::WindRotor))exposure*=.65;}double target=s.rate*std::pow(windStrength(f.tick),2)*exposure*s.throttle;s.rpm+=(target/s.rate*3000-s.rpm)*.25;put(m,E::Shaft,target);s.flow=target;m.status="Wind shaft / "+roundTo(target,1)+" / "+roundTo(s.rpm,0)+" rpm";}
   if(m.kind==K::GridInlet){double received=std::min({s.rate*s.throttle,f.electricity,std::max(0.,s.capacity-s.store[ix(E::AC)])});if(!f.sandbox)received=std::min(received,f.cash/.025);f.electricity-=received;if(!f.sandbox)f.cash-=received*.025;put(m,E::AC,received);s.generated=received;f.grid.generated+=received;m.status="Metered import / "+roundTo(received,1)+" / reserve "+roundTo(f.electricity,0);}
   if(storage(m.kind)){E e=storageMedium(m.kind);double loss=s.store[ix(e)]*(m.kind==K::Flywheel?.012:m.kind==K::HeatTank?.004:m.kind==K::SteamAccumulator?.002:.0002);s.store[ix(e)]-=loss;if(m.kind==K::Flywheel)s.rpm=std::sqrt(s.store[ix(e)]/std::max(1.,s.capacity))*3000;f.grid.loss+=electric(e)?loss:0;}
  }
  for(auto& [p,m]:f.machines)if(m.enabled&&m.kind==K::Inverter)inverter(p,m,true);
  for(auto& [p,m]:f.machines)if(m.enabled&&!m.power.tripped){auto& s=m.power;double rate=s.rate*s.throttle;
   if(m.kind==K::FeedPump){double loopWater=0;for(const auto& [q,device]:f.machines)if(connected(p,q,E::Water))loopWater+=device.power.store[ix(E::Water)];if(loopWater>=s.capacity){m.status="Return loop full / pump idle";continue;}double amount=std::min({rate,f.water,std::max(0.,s.capacity-s.store[ix(E::Water)])});if(amount>eps&&take(p,E::AC,.25)>0){f.water-=amount;f.waterUsed+=amount;put(m,E::Water,amount);s.flow=amount;m.status="Makeup water / "+roundTo(amount,1);}else m.status="Feed pump needs water and AC black start";}
   if(m.kind==K::PowerCondenser){double amount=take(p,E::Exhaust,std::min({rate,s.capacity-s.store[ix(E::Water)],(s.capacity-s.store[ix(E::Heat)])/8}),true);put(m,E::Water,amount);put(m,E::Heat,amount*8);s.flow=amount;m.status=amount>eps?"Condensing / return water +"+roundTo(amount,1):"Waiting for turbine exhaust";}
   if(m.kind==K::Macerator){if(!m.item){m.status="Feed actual farm or food residues";continue;}if(m.ready){m.status="Macerated feed ready";continue;}m.progress++;if(m.progress>=m.duration){m.ready=true;m.status="Macerated feed / route to boiler or digester";}else m.status="Macerating real food";}
   if(m.kind==K::AnaerobicDigester&&m.item){if(m.sideOutput){m.status="Digestate outlet blocked";continue;}double heat=take(p,E::Heat,.3,true);s.temperature+=(std::max(20.,m.temperature)-s.temperature)*(.025+heat*.03);double factor=clamp(1.-std::abs(s.temperature-38)/45,.25,1.);double organic=0;for(auto& part:m.item->food.parts)for(auto c:{Sucrose,Reducing,Starch,Fat,Protein})organic+=part.food.c[c];double amount=std::min({organic,rate*factor,(s.capacity-s.store[ix(E::RawGas)])/.7});if(amount<=eps){m.status="Raw gas outlet full / route to scrubber";continue;}double removed=0;for(auto& part:m.item->food.parts){for(auto c:{Sucrose,Reducing,Starch,Fat,Protein}){double v=organic>eps?amount*part.food.c[c]/organic:0;part.food.c[c]-=v;removed+=v;}}put(m,E::RawGas,removed*.7);f.grid.ventedMass+=removed*.3;s.flow=removed*.7;m.progress++;m.status="Digesting / gas +"+roundTo(s.flow,1);if(m.progress>=m.duration||organic-removed<.001){m.item->residue="digestate";m.item->cropBatch=false;m.item->matter=projectCell(m.item->food);m.sideOutput=std::move(m.item);m.item.reset();m.progress=0;}}
   if(m.kind==K::BiomassBoiler){bool firing=!m.automatic||s.armed;if(m.item&&firing&&!m.sideOutput){if(s.temperature<45&&take(p,E::AC,.2)<=eps){m.status="Boiler ignition needs AC black start";continue;}double organic=0,wet=0;for(auto& part:m.item->food.parts){wet+=part.food.c[Water];for(auto c:{Sucrose,Reducing,Starch,Fat,Protein,Fibre})organic+=part.food.c[c];}double fraction=organic>eps?std::min(1.,rate/organic):1.;double burnt=0;for(auto& part:m.item->food.parts)for(auto c:{Sucrose,Reducing,Starch,Fat,Protein,Fibre}){double v=part.food.c[c]*fraction;part.food.c[c]-=v;burnt+=v;}double heat=burnt*15*clamp(1.-wet/std::max(1.,wet+organic)*.65,.3,1.);put(m,E::Heat,heat);f.grid.ventedMass+=burnt;s.temperature=std::min(220.,s.temperature+heat*.30);if(organic-burnt<.001){for(auto& part:m.item->food.parts){f.grid.ventedMass+=part.food.c[Water];part.food.c[Water]=0;}if(analyse(m.item->food).mass>.001){m.item->residue="ash";m.item->cropBatch=false;m.item->matter=projectCell(m.item->food);m.sideOutput=std::move(m.item);}m.item.reset();}}
    if(s.temperature>=75&&s.store[ix(E::Heat)]>=12){double amount=std::min({rate,s.store[ix(E::Heat)]/12,s.capacity-s.store[ix(E::Steam)]});amount=take(p,E::Water,amount,true);s.store[ix(E::Heat)]-=amount*12;put(m,E::Steam,amount);s.flow=amount;m.status="Boiling / steam +"+roundTo(amount,1)+" / "+roundTo(s.temperature,0)+" C";}else m.status=m.item?"Boiler warming / "+roundTo(s.temperature,0)+" C":"Boiler needs real organic fuel";}
   if(m.kind==K::GasScrubber){double amount=std::min(rate,(s.capacity-s.store[ix(E::Gas)])/.9);auto b=budget();auto gas=plan(p,E::RawGas,amount,b);amount=0;for(const auto& tr:gas)amount+=tr.credit;auto water=plan(p,E::Water,.01,b);double wet=0;for(const auto& tr:water)wet+=tr.credit;if(amount>eps&&wet+.000001<.01){m.status="Scrubber needs connected return water";continue;}if(amount>eps){for(const auto& tr:gas)commit(tr);for(const auto& tr:water)commit(tr);put(m,E::Gas,amount*.9);f.grid.ventedMass+=amount*.1+.01;}s.flow=amount;m.status=amount>eps?"Scrubbing / clean gas +"+roundTo(amount*.9,1):"Waiting for raw biogas";}
  }
  for(auto& [p,m]:f.machines)if(m.enabled&&!m.power.tripped){auto& s=m.power;double rate=s.rate*s.throttle;
   if(m.kind==K::SteamTurbine){double amount=take(p,E::Steam,std::min({rate,(s.capacity-s.store[ix(E::Shaft)])/4,s.capacity-s.store[ix(E::Exhaust)]}),true);put(m,E::Shaft,amount*4);put(m,E::Exhaust,amount);s.rpm+=(amount>eps?3000-s.rpm:-s.rpm)*.3;s.flow=amount;m.status=amount>eps?"Turbine / shaft +"+roundTo(amount*4,1):"Steam missing or exhaust backed up";}
   if(m.kind==K::GasEngine){bool run=!m.automatic||s.armed;if(!run){s.rpm*=.8;m.status="Reserve engine on standby";continue;}if(s.rpm<100&&take(p,E::AC,1.5)<=eps){m.status="Engine needs AC black-start reserve";continue;}double amount=take(p,E::Gas,std::min({rate,(s.capacity-s.store[ix(E::Shaft)])/4.2,(s.capacity-s.store[ix(E::Heat)])/5}),true);put(m,E::Shaft,amount*4.2);put(m,E::Heat,amount*5);f.grid.ventedMass+=amount;s.rpm+=(amount>eps?3000-s.rpm:-s.rpm)*.3;s.flow=amount;m.status=amount>eps?"Engine / shaft +"+roundTo(amount*4.2,1):"Engine waiting for clean gas or heat outlet";}
   if(m.kind==K::Gearbox){double amount=take(p,E::Shaft,std::min(rate,s.capacity-s.store[ix(E::Shaft)]),true,true);put(m,E::Shaft,amount*.94);s.rpm=amount>eps?lastShaftRPM*clamp(m.shear,.25,2):0;s.flow=amount;m.status="Gear ratio "+roundTo(m.shear,1)+" / shaft "+roundTo(amount*.94,1);}
  }
  for(auto& [p,m]:f.machines)if(m.enabled&&!m.power.tripped&&m.kind==K::Alternator){auto& s=m.power;double amount=take(p,E::Shaft,std::min(s.rate*s.throttle,(s.capacity-s.store[ix(E::AC)])/.90),true);double efficiency=.90*clamp(lastShaftRPM/1800,.1,1.);put(m,E::AC,amount*efficiency);s.generated=amount*efficiency;s.rpm=amount>eps?lastShaftRPM:0;f.grid.generated+=s.generated;f.grid.loss+=amount*(1-efficiency);m.status="Alternator AC / "+roundTo(s.generated,1)+" / "+roundTo(s.rpm,0)+" rpm";}
  for(auto& [p,m]:f.machines)if(m.enabled&&!m.power.tripped&&m.kind==K::Inverter)inverter(p,m,true);
  nuclearBench();
  runReagentWorks(f,[&](Position p,double amount){return take(p,E::AC,amount)>0;});
  runWaterWorks(f,[&](Position p,E e,double volume,double cost,bool skipStorage){return pullWater(p,e,volume,cost,skipStorage);},[&](Position p,double amount){return amount<=1e-8||take(p,E::AC,amount)>0;});
  for(auto& [p,m]:f.machines)if(m.enabled&&!m.power.tripped){auto& s=m.power;if(m.kind==K::HeatExchanger){double amount=take(p,E::Heat,std::min(s.rate,s.capacity-s.store[ix(E::Heat)]),true,true);put(m,E::Heat,amount*.9);s.flow=amount;m.status="Recovered heat available / "+roundTo(s.store[ix(E::Heat)],1);}if(m.kind==K::CoolingPlant){double amount=take(p,E::AC,std::min(s.rate,(s.capacity-s.store[ix(E::Cooling)])/2),true);double heat=take(p,E::Heat,amount,true);put(m,E::Cooling,amount*2+heat*.4);s.flow=amount;m.status="Cooling produced / "+roundTo(amount*2+heat*.4,1);}}
 }
 void allocate(){
  Budget b=budget();std::vector<std::pair<int,Position>> loads;
  for(auto& [p,m]:f.machines){if(isEnergyMachine(m.kind)||!m.enabled)continue;double demand=machineSpec(m.kind).power;if(demand<=0)continue;bool work=m.kind==K::GrowLight||(m.item&&!m.ready);if(isMixingMachine(m.kind)&&!m.extra&&m.progress==0)work=false;if(m.kind==K::LatticeReactor)work=m.item&&m.latticePending>=0;if(m.kind==K::Harvester){auto bed=f.at(nextPosition(p,(m.dir+2)%4));work=!m.item&&bed&&bed->farmPlanted&&bed->farmGrowth>=.99;}if(machineSpec(m.kind).water>f.water)work=false;if(!work)continue;m.power.demand=demand;f.grid.requested+=demand;loads.push_back({m.power.priority,p});}
  // Rotate equal-priority consumers each tick so a shortage is not decided by map order.
  std::stable_sort(loads.begin(),loads.end(),[&](auto a,auto b){if(a.first!=b.first)return a.first<b.first;auto order=[&](Position p){return ((uint64_t(uint32_t(p.x))*73856093ull^uint64_t(uint32_t(p.y))*19349663ull)+uint64_t(f.tick)*2654435761ull)%1000003;};return order(a.second)<order(b.second);});
  for(auto [priority,p]:loads){auto m=f.at(p);std::vector<Transfer> transfers;double thermal=0;if((m->kind==K::Heater||m->kind==K::SousVide||m->kind==K::GlassFlask||m->kind==K::FractionatingStill||m->kind==K::Cycler)&&m->temperature>20){auto heat=plan(p,E::Heat,m->power.demand*.5,b);for(auto& tr:heat)thermal+=tr.credit;transfers.insert(transfers.end(),heat.begin(),heat.end());}if(m->kind==K::Chiller||m->kind==K::ColdStore||m->kind==K::GlassCondenser||((m->kind==K::Heater||m->kind==K::SousVide||m->kind==K::GlassFlask||m->kind==K::Cycler)&&m->temperature<20)){auto cool=plan(p,E::Cooling,m->power.demand*.5,b);for(auto& tr:cool)thermal+=tr.credit;transfers.insert(transfers.end(),cool.begin(),cool.end());}auto ac=plan(p,E::AC,std::max(0.,m->power.demand-thermal),b);transfers.insert(transfers.end(),ac.begin(),ac.end());double got=0;for(auto& t:transfers)got+=t.credit;allocations[p]=std::move(transfers);remaining[p]=got;m->power.voltage=got/std::max(.001,m->power.demand);}
 }
 bool available(Position p,double amount)const{auto i=remaining.find(p);return i!=remaining.end()&&i->second+eps>=amount;}
 void consume(Position p,double amount){if(!available(p,amount))return;double left=amount;for(auto& tr:allocations[p]){double credit=std::min(left,tr.credit);if(credit<=eps)continue;double fraction=credit/tr.credit;commit(tr,fraction);if(tr.medium==E::Heat||tr.medium==E::Cooling)f.grid.recoveredHeat+=credit;tr.debit*=1-fraction;tr.credit-=credit;left-=credit;if(left<=eps)break;}remaining[p]-=amount;auto m=f.at(p);if(m)m->power.delivered+=amount;f.grid.delivered+=amount;
  // Recovered heat and cooling reduce electrical use by supplying a measured share
  // of the process; their energy is still consumed from actual connected stores.
 }
 void settle(){
  recordLabTrials();
  for(auto& [p,m]:f.machines)if(m.enabled&&storage(m.kind)){E e=storageMedium(m.kind);double room=std::max(0.,m.power.capacity-m.power.store[ix(e)]),eff=e==E::DC?.94:.98;double amount=take(p,e,std::min(m.power.rate,room/eff),true,true);put(m,e,amount*eff);if(electric(e))f.grid.loss+=amount*(1-eff);m.status="Stored "+roundTo(m.power.store[ix(e)],1)+" / "+roundTo(m.power.capacity,0)+" / rate "+roundTo(m.power.rate,0);}
  for(auto& [p,m]:f.machines)if(m.enabled&&m.kind==K::Inverter)inverter(p,m,false);
  for(auto& [p,m]:f.machines)if(m.enabled&&m.kind==K::HeatTank&&m.forward){double released=std::max(0.,m.power.store[ix(E::Heat)]-m.power.capacity*.7);released=std::min(released,m.power.rate);m.power.store[ix(E::Heat)]-=released;if(released>0)m.status="Heat bypass / vented "+roundTo(released,1);}
  for(auto& [p,links]:f.energyLinks)for(auto& link:links){double stress=std::max(link.flow/link.rating,link.overload);link.heat=clamp(link.heat+std::max(0.,stress-.9)*.08-.025,0,2);if(electric(link.medium)&&link.heat>=1)link.tripped=true;}
  for(auto& [p,m]:f.machines){if(storage(m.kind))f.grid.stored+=m.power.store[ix(storageMedium(m.kind))];if(isEnergyMachine(m.kind)){m.power.history.push_back(m.kind==K::ExperimentConsole?m.power.lab.stability*100:m.power.generated>0?m.power.generated:m.power.delivered>0?m.power.delivered:m.power.flow);if(m.power.history.size()>80)m.power.history.erase(m.power.history.begin());if(m.kind==K::Switchboard){double load=0;for(int d=0;d<4;d++)if(auto link=f.energyLink(nextPosition(p,d),E::AC))load=std::max(load,link->flow);m.power.flow=load;m.power.heat=clamp(m.power.heat+std::max(0.,load/m.power.rate-.9)*.08-.02,0,2);if(m.power.heat>=1)m.power.tripped=true;m.status=m.power.tripped?"Breaker tripped":m.enabled?"AC bus / "+roundTo(load,1)+" / rating "+roundTo(m.power.rate,0):"Breaker open";}}}
 }
};
// All nuclear quantities are deliberately fictional laboratory units. This is
// an experimental game loop, not a model of a real reactor or isotope process.
void PowerRuntime::nuclearBench(){
 for(auto& [p,m]:f.machines){auto& s=m.power;auto& l=s.lab;
  if(m.kind==K::ExperimentConsole||m.kind==K::LabVacuumPump||m.kind==K::QuenchBath){
   E medium=m.kind==K::ExperimentConsole?E::Instrument:m.kind==K::LabVacuumPump?E::Vacuum:E::Cooling;
   s.store[ix(medium)]=0;
   if(!m.enabled||s.tripped)continue;
   double draw=m.kind==K::ExperimentConsole?.18:m.kind==K::LabVacuumPump?.4:.35;
   if(m.kind==K::QuenchBath&&f.water<.02){m.status="Quench bath needs supplied water";continue;}
   double got=take(p,E::AC,draw);
   if(got<=eps){m.status="Bench supply missing / connect AC";continue;}
   if(m.kind==K::QuenchBath){f.water-=.02;f.waterUsed+=.02;}
   put(m,medium,s.rate*s.throttle);s.flow=s.rate*s.throttle;l.inputTotal+=got;
   m.status=m.kind==K::ExperimentConsole?"Recording / interlock lead live":m.kind==K::LabVacuumPump?"Evacuating / cold trap active":"Quench circulation / cold reference live";
  }
 }
 for(auto& [p,m]:f.machines)if(m.kind==K::CuriosityPile||m.kind==K::StarChamber){auto& s=m.power;auto& l=s.lab;
  l.cooling=0;double cold=0;if(m.enabled&&!s.tripped){cold=take(p,E::Cooling,m.kind==K::CuriosityPile?1.2:.8,true);l.cooling=std::min(1.,cold/(m.kind==K::CuriosityPile?1.2:.8));}
  s.temperature=std::max(20.,s.temperature-cold*4);
  if(m.kind==K::StarChamber){double vacuum=m.enabled&&!s.tripped?take(p,E::Vacuum,.8,true):0;l.vacuum=clamp(l.vacuum+(vacuum>=.75?.14:-.18),0,1);}else l.vacuum=1;
  bool running=m.enabled&&!s.tripped&&l.armed&&l.fuel>eps;double signal=running?take(p,E::Instrument,.1):0;
  if(running&&signal<=eps){l.armed=false;running=false;m.status="Interlock open / recorder signal missing";}
  double draw=0;if(running&&m.kind==K::StarChamber){draw=take(p,E::AC,1.2+s.throttle*2.5);if(draw<=eps){running=false;m.status="Excitation supply missing / no pulse";}}
  l.inputTotal+=draw;
  double tuning=clamp(1.-std::abs(l.tuning-labTuningTarget(m.kind))*2.8,.08,1.);
  l.stability=clamp(tuning*(.25+.75*l.cooling)*l.vacuum*(1.-std::max(0.,s.temperature-65)/90),0,1);
  double target=running?s.throttle*l.stability:0;l.flux+=(target-l.flux)*.22;l.flux=clamp(l.flux,0,1);
  double heat=0;
  if(m.kind==K::StarChamber){l.pulse=(l.pulse+1)%std::max(2,m.duration);if(running&&l.pulse==0&&l.vacuum>.45)heat=s.rate*l.flux*std::max(2,m.duration);}
  else if(l.fuel>eps)heat=s.rate*l.flux;
  heat=std::min({heat,l.fuel/.0025,std::max(0.,s.capacity-s.store[ix(E::Heat)])});
  l.fuel=std::max(0.,l.fuel-heat*.0025);l.spent+=heat*.0025;l.heatTotal+=heat;put(m,E::Heat,heat);s.flow=heat;s.temperature+=heat*.08;
  if(s.temperature>105){s.tripped=true;l.armed=false;m.status="SCRAM / jacket overheated / cool before reset";}
  else if(!m.enabled||s.tripped){l.armed=false;m.status=s.tripped?"Interlock latched / cooling":"Bench isolated / residual heat decaying";}
  else if(l.fuel<=eps){l.armed=false;m.status="Cartridge exhausted / change sealed specimen";}
  else if(l.armed){m.status=m.kind==K::StarChamber?(l.vacuum<=.45?"Evacuating chamber / waiting for vacuum":heat>0?"PULSE / heat +"+roundTo(heat,1):"Charging electrodes / next pulse "+std::to_string(std::max(2,m.duration)-l.pulse)):"Cartridge excitation / heat +"+roundTo(heat,1);if(l.cooling<.4)m.status+=" / cooling weak";}
  else if(signal>eps||m.status.find("Interlock open")==std::string::npos)m.status="Disarmed / jacket "+roundTo(s.temperature,0)+" C";
 }
 for(auto& [p,m]:f.machines)if(m.kind==K::Calorimeter&&m.enabled&&!m.power.tripped){auto& s=m.power;auto& l=s.lab;double cold=take(p,E::Cooling,.7,true);l.cooling=std::min(1.,cold/.7);double efficiency=.28+.34*l.cooling;double heat=take(p,E::Heat,std::min(s.rate*s.throttle,std::max(0.,s.capacity-s.store[ix(E::DC)])/efficiency),true);double dc=heat*efficiency;put(m,E::DC,dc);l.heatTotal+=heat;l.dcTotal+=dc;s.generated=dc;s.flow=heat;f.grid.generated+=dc;f.grid.loss+=heat-dc;m.status="Measured "+roundTo(heat,1)+" heat / "+roundTo(dc,1)+" DC / "+roundTo(efficiency*100,0)+"% recovery";}
 for(auto& [p,m]:f.machines)if(m.kind==K::Inverter&&m.enabled&&!m.power.tripped)inverter(p,m,true);
}
void PowerRuntime::recordLabTrials(){
 for(auto& [p,m]:f.machines)if(m.kind==K::ExperimentConsole&&m.enabled&&m.power.flow>0){auto& l=m.power.lab;double heat=0,input=0,dc=0,stability=0;int count=0;std::string settings;std::set<Position> converters;
  for(const auto& [q,chamber]:f.machines)if((chamber.kind==K::CuriosityPile||chamber.kind==K::StarChamber)&&connected(p,q,E::Instrument)){
   heat+=chamber.power.lab.heatTotal;input+=chamber.power.lab.inputTotal;stability+=chamber.power.lab.stability;count++;
   if(count<=4){if(!settings.empty())settings+=" / ";settings+=(chamber.kind==K::CuriosityPile?"NU":"STAR")+std::string(" r")+roundTo(chamber.power.lab.tuning*100,0)+" d"+roundTo(chamber.power.throttle*100,0);if(chamber.kind==K::StarChamber)settings+=" p"+std::to_string(std::max(2,chamber.duration));}
   for(const auto& [r,cal]:f.machines)if(cal.kind==K::Calorimeter&&connected(q,r,E::Heat))converters.insert(r);
  }
  for(auto q:converters)dc+=f.at(q)->power.lab.dcTotal;
  for(const auto& [q,support]:f.machines)if((support.kind==K::LabVacuumPump||support.kind==K::QuenchBath||support.kind==K::ExperimentConsole)&&connected(p,q,E::AC))input+=support.power.lab.inputTotal;
  l.heatTotal=heat;l.dcTotal=dc;l.stability=count?stability/count:0;
  if(count&&++l.sampleTicks>=24){l.sampleTicks=0;l.trials.push_back({f.tick,std::max(0.,heat-l.lastHeat),std::max(0.,input-l.lastInput),std::max(0.,dc-l.lastDC),l.stability,settings});if(l.trials.size()>16)l.trials.erase(l.trials.begin());l.lastHeat=heat;l.lastInput=input;l.lastDC=dc;}
 }
}

PowerLink* Factory::energyLink(Position p,E e){auto i=energyLinks.find(p);if(i==energyLinks.end())return nullptr;for(auto& link:i->second)if(link.medium==e)return &link;return nullptr;}
const PowerLink* Factory::energyLink(Position p,E e)const{auto i=energyLinks.find(p);if(i==energyLinks.end())return nullptr;for(auto& link:i->second)if(link.medium==e)return &link;return nullptr;}
bool Factory::buildEnergyLink(Position p,E e){if(energyLink(p,e))return false;if(std::abs(p.x)>1000000||std::abs(p.y)>1000000)return false;if(!sandbox&&cash<3){notice="Insufficient credits for conduit.";return false;}snapshot();PowerLink link;link.medium=e;energyLinks[p].push_back(link);if(!sandbox)cash-=3;dirty=true;notice=energyName(e)+" connected at "+positionName(p)+". Routes can share floor tiles with food equipment.";return true;}
bool Factory::removeEnergyLink(Position p,E e){auto link=energyLink(p,e);if(!link)return false;snapshot();auto& v=energyLinks[p];v.erase(std::remove_if(v.begin(),v.end(),[&](const PowerLink& l){return l.medium==e;}),v.end());if(v.empty())energyLinks.erase(p);dirty=true;return true;}
bool Factory::configureEnergyLink(Position p,E e,const std::string& setting){auto link=energyLink(p,e);if(!link)return false;snapshot();if(setting=="enabled")link->enabled=!link->enabled;else if(setting=="reset"){if(link->heat>.6){notice="Conduit is still hot. Isolate it or reduce load before resetting.";return false;}link->tripped=false;}else if(setting=="rating+")link->rating=std::min(1000.,link->rating+10);else if(setting=="rating-")link->rating=std::max(1.,link->rating-10);dirty=true;return true;}
void Factory::beginPowerStep(){grid=PowerTotals{};for(auto& [p,m]:machines)if(isEnergyMachine(m.kind)){grid.active=true;break;}if(!grid.active){powerRuntime.reset();return;}powerRuntime=std::make_shared<PowerRuntime>(*this);powerRuntime->generate();powerRuntime->allocate();}
double Factory::powerContactFlow(Position a,Position b,E e)const{if(!powerRuntime)return 0;auto key=b<a?std::make_pair(b,a):std::make_pair(a,b);auto i=powerRuntime->contactTransfers.find(key);return i==powerRuntime->contactTransfers.end()?0:i->second[ix(e)]*(b<a?-1:1);}
bool Factory::powerAvailable(Position p,double amount)const{if(amount<=eps)return true;if(grid.active){auto m=at(p);double needed=m&&(m->kind==K::Mixer||m->kind==K::Heater)?amount*.2:amount;return powerRuntime&&powerRuntime->available(p,needed);}return electricity+eps>=amount;}
void Factory::usePower(Position p,double amount){if(amount<=0)return;if(grid.active){if(powerRuntime){auto i=powerRuntime->remaining.find(p);if(i!=powerRuntime->remaining.end())powerRuntime->consume(p,std::min(amount,i->second));}}else electricity=std::max(0.,electricity-amount);}
void Factory::finishPowerStep(){if(powerRuntime)powerRuntime->settle();}
} // namespace aftertaste
