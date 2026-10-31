#include "Control.h"
#include "Power.h"
#include "WaterWorks.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace aftertaste {
namespace {
using K=MachineKind;
bool controller(K k){return k==K::ThresholdController||k==K::LogicGate||k==K::ControlTimer;}
bool thermal(K k){return k==K::Heater||k==K::SousVide||k==K::GlassFlask||k==K::Cycler;}
bool compatible(const Machine& m,ControlAction a){
 switch(a){case ControlAction::DosePulse:return isDosingMachine(m.kind);case ControlAction::HeatCool:return thermal(m.kind);case ControlAction::DivertWater:return m.kind==K::WaterDiverter||m.kind==K::WaterOutlet;case ControlAction::ReactorInterlock:return m.kind==K::CuriosityPile||m.kind==K::StarChamber;case ControlAction::RunPermit:return !isControlLogic(m.kind)&&m.kind!=K::Wall&&m.kind!=K::CuriosityPile&&m.kind!=K::StarChamber;default:return false;}
}
ControlMetric metric(const Machine& m){if(m.kind==K::PHProbe)return ControlMetric::PH;if(m.kind==K::TemperatureLogger)return ControlMetric::Temperature;if(m.kind==K::SoilProbe)return ControlMetric::Moisture;if(m.kind==K::DropletViewer)return ControlMetric::Droplet;if(m.kind==K::AromaSensor)return ControlMetric::Aroma;if(m.kind==K::WaterAnalyzer)return ControlMetric::WaterQuality;return m.control.metric;}
ControlSignal sample(const Factory& f,Position p,const Machine& sensor){
 auto q=controlTarget(p,sensor);auto target=f.at(q);ControlSignal s;s.source=p;s.fault="No measured target";if(!target||!target->enabled)return s;
 auto kind=metric(sensor);s.fault="Metric does not match target";
 if(kind==ControlMetric::Moisture){if(!isFarmBed(target->kind))return s;s.value=target->farmMoisture;}
 else if(kind==ControlMetric::ReactorCooling||kind==ControlMetric::ReactorTemperature){if(target->kind!=K::CuriosityPile&&target->kind!=K::StarChamber)return s;s.value=kind==ControlMetric::ReactorCooling?target->power.lab.cooling:target->power.temperature;}
 else if(kind>=ControlMetric::WaterQuality&&kind<=ControlMetric::Biology){if(!isWaterMachine(target->kind))return s;auto water=waterSample(*target);double held=0;for(size_t n=size_t(EnergyMedium::RawWater);n<=size_t(EnergyMedium::Concentrate);n++)held+=target->power.store[n];if(water.volume<=1e-8||(held<=1e-8&&target->power.flow<=1e-8)){s.fault="No water sample";return s;}double solids=water.solids/water.volume,salt=water.salt/water.volume,bio=water.bio/water.volume;s.value=kind==ControlMetric::Solids?solids:kind==ControlMetric::Salinity?salt:kind==ControlMetric::Biology?bio:std::max({solids/.15,salt,bio/.02});}
 else{const Batch* b=target->item?&*target->item:nullptr;if(!b||b->food.parts.empty()){s.fault="No live batch";return s;}auto a=analyse(b->food);if(kind==ControlMetric::PH)s.value=a.pH;else if(kind==ControlMetric::Temperature)s.value=a.temperature;else if(kind==ControlMetric::Aroma)s.value=a.aroma;else{double mass=0,sum=0;for(const auto& part:b->food.parts){double g=foodMass(part.food);mass+=g;sum+=g*part.food.droplet;}s.value=mass>0?sum/mass:0;}}
 s.valid=std::isfinite(s.value);s.fault=s.valid?"":"Non-finite reading";return s;
}
void publish(Machine& m,const ControlSignal& s,int tick){m.control.valid=s.valid;m.control.value=s.value;m.control.sampled=tick;m.control.fault=s.fault;m.control.output=s.valid&&s.value>.5;}
}
bool isControlLogic(K k){return k>=K::ProcessSensor&&k<=K::ControlActuator;}
bool isControlDevice(K k){return isControlLogic(k)||k==K::WaterDiverter;}
bool isControlSource(const Machine& m){return m.kind==K::ProcessSensor||controller(m.kind)||isSensor(m.kind)||m.kind==K::WaterAnalyzer||(m.kind==K::ExperimentConsole&&m.control.monitor);}
int controlOutputPort(const Machine& m){return controller(m.kind)?m.dir:(m.dir+2)%4;}
std::vector<int> controlInputPorts(const Machine& m){if(m.kind==K::LogicGate&&m.control.gate!=2)return {(m.dir+2)%4,(m.dir+3)%4};if(controller(m.kind)||m.kind==K::ControlActuator)return {(m.dir+2)%4};return {};}
Position controlTarget(Position p,const Machine& m){for(int n=0;n<m.control.range;n++)p=nextPosition(p,m.dir);return p;}
std::string controlMetricName(ControlMetric m){static const std::array<const char*,size_t(ControlMetric::Count)> names={"pH","Temperature / C","Soil moisture / 0..1","Droplet size / um","Aroma / 0..1","Water risk / safe <= 1","Suspended solids / L","Dissolved salts / L","Biological load / L","Reactor cooling / 0..1","Reactor temperature / C"};return names.at(size_t(m));}
std::string controlActionName(ControlAction a){static const std::array<const char*,size_t(ControlAction::Count)> names={"Run permit / valve","Metered dosing","Heat / cool selector","Divert unsafe water","Reactor interlock"};return names.at(size_t(a));}
std::vector<MachineSpec> controlMachineSpecs(){return {
 {K::ProcessSensor,"Process sensor","SENSE","Point at actual food, a grow bed, water equipment or a reactor. Select the measured quantity; the rear terminal publishes that reading on dotted signal leads.","#179daa",34,1,0,0},
 {K::ThresholdController,"Threshold controller","CMP","Compares one measured input with a setpoint and tolerance band. Rear input, front output. Inside the band it retains its previous decision.","#e4be26",45,1,0,0},
 {K::LogicGate,"Permissive logic gate","LOGIC","Combines two valid signals as AND or OR, or inverts one signal as NOT. Missing/conflicting inputs fail off. Rear and left inputs; front output.","#7586b6",35,1,0,0},
 {K::ControlTimer,"Signal timer","TIMER","Outputs a configurable on/off cycle in simulation ticks. Gate it with a signal, or explicitly choose free running. Pause freezes the timer.","#5494c9",38,1,0,0},
 {K::ControlActuator,"Process actuator","ACT","Rear signal input; point its target arrow at the controlled machine. Operates run permits, measured dose pulses, thermal selection, water diversion or a latched reactor interlock.","#31aa65",42,1,0,0},
 {K::SignalLead,"Dotted signal lead","SIG","Carries readings and decisions separately from material, electricity and reactor recorder energy. Drag square routes. A network accepts exactly one signal source.","#293e58",3,1,0,0},
 {K::WaterDiverter,"Actuated water valve","DIV","Clean water enters at the rear. Safe water exits the front; the side branch sends the complete parcel to greywater treatment. Missing control fails toward treatment.","#2789df",48,1,0,0}
 };}
void initialiseControlDevice(Machine& m){if(!isControlDevice(m.kind))return;m.control=ControlState{};if(m.kind==K::ProcessSensor)m.control.monitor=true;if(m.kind==K::ControlActuator)m.control.period=1;if(m.kind==K::WaterDiverter)m.forward=true;}
void resetControlRuntime(Machine& m){auto& c=m.control;c.value=0;c.phase=0;c.sampled=-1;c.lastPulse=-1000000;c.valid=c.output=c.driven=c.permit=false;c.fault.clear();}
bool validControl(const ControlState& c){return int(c.metric)>=0&&c.metric<ControlMetric::Count&&int(c.action)>=0&&c.action<ControlAction::Count&&int(c.appliedAction)>=0&&c.appliedAction<ControlAction::Count&&std::isfinite(c.setpoint)&&std::abs(c.setpoint)<=10000&&std::isfinite(c.deadband)&&c.deadband>=0&&c.deadband<=1000&&std::isfinite(c.step)&&c.step>=.001&&c.step<=100&&std::isfinite(c.hot)&&c.hot>=-100&&c.hot<=220&&std::isfinite(c.cold)&&c.cold>=-100&&c.cold<=220&&std::isfinite(c.value)&&std::abs(c.value)<1e12&&c.range>=1&&c.range<=6&&c.period>=1&&c.period<=10000&&c.onTicks>=1&&c.onTicks<=10000&&c.phase>=0&&c.phase<=10000&&c.gate>=0&&c.gate<=2&&c.sampled>=-1&&c.lastPulse>=-1000000&&c.appliedPeriod>=1&&c.appliedPeriod<=10000&&c.fault.size()<=200;}
bool configureControl(Factory& f,Machine& m,const std::string& s){auto& c=m.control;
 if(s=="control-metric")c.metric=ControlMetric((int(c.metric)+1)%int(ControlMetric::Count));
 else if(s=="control-action")c.action=ControlAction((int(c.action)+1)%int(ControlAction::Count));
 else if(s=="control-direction")c.below=!c.below;
 else if(s=="control-step")c.step=c.step<.05?.1:c.step<.5?1:c.step<5?10:.01;
 else if(s=="control-set-")c.setpoint=std::max(-10000.,c.setpoint-c.step);else if(s=="control-set+")c.setpoint=std::min(10000.,c.setpoint+c.step);
 else if(s=="control-band-")c.deadband=std::max(0.,c.deadband-c.step);else if(s=="control-band+")c.deadband=std::min(1000.,c.deadband+c.step);
 else if(s=="control-period-")c.period=std::max(1,c.period-1);else if(s=="control-period+")c.period=std::min(10000,c.period+1);
 else if(s=="control-on-")c.onTicks=std::max(1,c.onTicks-1);else if(s=="control-on+")c.onTicks=std::min(c.period,c.onTicks+1);
 else if(s=="control-free")c.freeRun=!c.freeRun;
 else if(s=="control-gate")c.gate=(c.gate+1)%3;
 else if(s=="control-range-")c.range=std::max(1,c.range-1);else if(s=="control-range+")c.range=std::min(6,c.range+1);
 else if(s=="control-hot-")c.hot=std::max(-100.,c.hot-c.step);else if(s=="control-hot+")c.hot=std::min(220.,c.hot+c.step);
 else if(s=="control-cold-")c.cold=std::max(-100.,c.cold-c.step);else if(s=="control-cold+")c.cold=std::min(220.,c.cold+c.step);
 else if(s=="control-reset"){resetControlRuntime(m);}
 else if(s=="control-pass"&&m.kind==K::WaterDiverter)m.forward=!m.forward;
 else return false;
 c.onTicks=std::min(c.onTicks,c.period);c.phase=0;f.notice="Control settings updated. Decisions are evaluated on the next simulation tick.";return true;
}
ControlSignal controlInput(const Factory& f,Position p,int d,const ControlSignals& signals){
 std::vector<Position> open;std::set<Position> seen,sources;
 auto visit=[&](Position q,int from){auto m=f.at(q);if(m&&isControlSource(*m)&&controlOutputPort(*m)==from){sources.insert(q);return;}if(m&&(isControlLogic(m->kind)||isControlSource(*m)))return;auto wire=f.energyLink(q,EnergyMedium::Control);if(wire&&wire->enabled&&!wire->tripped&&(wire->ports&(1<<from))&&seen.insert(q).second)open.push_back(q);};
 visit(nextPosition(p,d),(d+2)%4);
 for(size_t n=0;n<open.size();n++){auto at=open[n];auto wire=f.energyLink(at,EnergyMedium::Control);for(int side=0;side<4;side++)if(wire->ports&(1<<side))visit(nextPosition(at,side),(side+2)%4);}
 if(sources.size()!=1)return {false,0,{},sources.empty()?"Signal disconnected":"Multiple signal sources"};auto source=*sources.begin();auto i=signals.find(source);return i==signals.end()?ControlSignal{false,0,source,"Source unavailable"}:i->second;
}
ControlFrame::ControlFrame(Factory& f):factory(f){
 ControlSignals signals;for(auto& [p,m]:f.machines){m.control.driven=false;m.control.permit=false;if(isControlSource(m)&&!controller(m.kind)){auto s=m.enabled?sample(f,p,m):ControlSignal{false,0,p,"Sensor disabled"};publish(m,s,f.tick);signals[p]=s;if(m.kind==K::ProcessSensor){m.sensorValue=s.value;m.sensorTick=s.valid?f.tick:-1;if(s.valid){m.sensorHistory.push_back(s.value);if(m.sensorHistory.size()>80)m.sensorHistory.erase(m.sensorHistory.begin());}m.status=s.valid?controlMetricName(metric(m))+" / "+roundTo(s.value,2):s.fault;}}else if(controller(m.kind))signals[p]={m.enabled&&m.control.valid,m.control.output?1.:0.,p,m.control.fault};}
 // Logic reads a snapshot. Chained controllers advance one stage per tick and
 // never change behaviour because their positions sort differently on the floor.
 for(auto& [p,m]:f.machines)if(controller(m.kind)){
  auto& c=m.control;bool valid=m.enabled,output=false;std::string fault=m.enabled?"":"Controller disabled";auto a=controlInput(f,p,(m.dir+2)%4,signals);c.value=a.value;
  if(m.kind==K::ThresholdController){valid=valid&&a.valid;if(!a.valid)fault=a.fault;output=c.output;if(valid){double low=c.setpoint-c.deadband*.5,high=c.setpoint+c.deadband*.5;if(a.value<low)output=c.below;if(a.value>high)output=!c.below;}}
  if(m.kind==K::LogicGate){auto b=c.gate==2?a:controlInput(f,p,(m.dir+3)%4,signals);valid=valid&&a.valid&&b.valid;if(!a.valid)fault=a.fault;else if(!b.valid)fault=b.fault;output=c.gate==2?a.value<=.5:c.gate==0?a.value>.5&&b.value>.5:a.value>.5||b.value>.5;}
  if(m.kind==K::ControlTimer){valid=valid&&(c.freeRun||a.valid);if(!valid)fault=a.fault;bool enabled=valid&&(c.freeRun||a.value>.5);if(enabled){output=c.phase<std::min(c.onTicks,c.period);c.phase=(c.phase+1)%c.period;}else{c.phase=0;output=false;}}
  c.valid=valid;c.output=valid&&output;c.sampled=f.tick;c.fault=valid?"":fault;m.status=valid?(c.output?"SIGNAL ON":"SIGNAL OFF"):(controlWaiting(c)?"WAITING / ":"FAULT / ")+fault;
 }
 for(const auto& [p,m]:f.machines)if(controller(m.kind))signals[p]={m.control.valid,m.control.output?1.:0.,p,m.control.fault};
 std::map<Position,std::vector<Position>> commands;
 for(auto& [p,m]:f.machines)if(m.kind==K::ControlActuator){auto input=controlInput(f,p,(m.dir+2)%4,signals);if(!m.enabled)input={false,0,p,"Actuator disabled"};publish(m,input,f.tick);m.control.output=input.valid&&input.value>.5;commands[controlTarget(p,m)].push_back(p);}
 for(const auto& [p,actors]:commands){auto target=f.at(p);if(!target)continue;auto& actor=*f.at(actors.front());auto& c=target->control;c.driven=true;c.valid=actors.size()==1&&actor.control.valid&&compatible(*target,actor.control.action);c.output=c.valid&&actor.control.output;c.appliedAction=actor.control.action;c.appliedPeriod=actor.control.period;c.fault=c.valid?"":actors.size()>1?"Conflicting actuators":!compatible(*target,actor.control.action)?"Incompatible target":actor.control.fault;c.permit=c.valid&&c.output;restored.push_back({p,target->enabled,target->temperature});
  actor.status=c.valid?(c.output?"ACTUATOR ON / ":"ACTUATOR OFF / ")+controlActionName(c.appliedAction):"FAULT / "+c.fault;
  if(actors.size()>1||!compatible(*target,c.appliedAction)){
   for(auto at:actors){auto& device=*f.at(at);device.control.valid=false;device.control.output=false;device.control.fault=c.fault;device.status="FAULT / "+c.fault;}
   if(target->kind==K::CuriosityPile||target->kind==K::StarChamber){c.appliedAction=ControlAction::ReactorInterlock;target->power.lab.armed=false;}
   else if(target->kind==K::WaterDiverter)c.appliedAction=ControlAction::DivertWater;
   else target->enabled=false;
   continue;
  }
  switch(c.appliedAction){
   case ControlAction::RunPermit:target->enabled=target->enabled&&c.permit;break;
   case ControlAction::DosePulse:if(!c.valid&&target->item)target->enabled=false;if(target->item){target->ready=c.valid&&!c.output;if(target->ready)target->progress=target->duration;}break;
   case ControlAction::HeatCool:target->enabled=target->enabled&&(c.valid||!target->item);target->temperature=c.output?actor.control.hot:actor.control.cold;c.value=target->temperature;c.permit=c.valid;break;
   case ControlAction::DivertWater:c.permit=c.valid;if(target->kind==K::WaterOutlet){if(!c.valid)target->enabled=false;else if(c.output)target->waterProcess.purge=true;}break;
   case ControlAction::ReactorInterlock:if(!c.permit)target->power.lab.armed=false;break;
   default:break;
  }
 }
 for(auto& [p,m]:f.machines)if(m.kind==K::ControlActuator&&m.enabled&&!f.at(controlTarget(p,m))){m.control.valid=false;m.control.output=false;m.control.fault="No target at "+positionName(controlTarget(p,m));m.status=m.control.fault;}
}
ControlFrame::~ControlFrame(){for(const auto& saved:restored)if(auto m=factory.at(saved.at)){m->enabled=saved.enabled;m->temperature=saved.temperature;if(m->control.driven&&((m->control.appliedAction==ControlAction::RunPermit&&!m->control.permit)||(m->control.appliedAction==ControlAction::DosePulse&&!m->control.valid)))m->status="Control hold / "+(m->control.valid?std::string("signal off"):m->control.fault);}}
}
