#include "Control.h"
#include "Power.h"
#include "WaterWorks.h"
#include <algorithm>
#include <stdexcept>
namespace aftertaste {
bool installDemoFeedback(Factory& f){
 using K=MachineKind;using E=EnergyMedium;
 if(std::any_of(f.machines.begin(),f.machines.end(),[](const auto& p){return isControlLogic(p.second.kind);}))return false;
 auto matches=[&](Position p,K k){auto m=f.at(p);return m&&m->kind==k;};
 if(std::any_of(f.energyLinks.begin(),f.energyLinks.end(),[](const auto& p){return std::any_of(p.second.begin(),p.second.end(),[](const PowerLink& l){return l.medium==EnergyMedium::Control;});}))return false;
 if(!matches({-11,-10},K::ExperimentConsole)||!matches({-2,-8},K::SoilProbe)||!matches({3,7},K::TemperatureLogger)||!matches({-12,-4},K::UVSterilizer)||!matches({2,8},K::Burette)||!matches({3,8},K::GlassFlask)||!matches({-11,-9},K::CuriosityPile)||!matches({-11,-4},K::CleanWaterTank)||!matches({-10,-4},K::WaterOutlet)||!matches({-1,-7},K::Fertigation))return false;
 if(f.at({2,8})->item||f.at({3,8})->item)return false;
 const std::vector<Position> free={{2,7},{0,7},{2,9},{4,6},{3,9},{-2,-6},{0,-6},{0,-7},{-12,-9},{-15,-10},{-11,-12},{-9,-12},{-11,-7},{-9,-4},{-9,-2},{-12,-5},{-13,-6},{-11,-3}};
 for(auto p:free)if(f.at(p))return false;
 auto add=[&](Position p,K kind,int dir=0)->Machine&{Machine m;m.kind=kind;m.dir=dir;m.duration=std::max(1,machineSpec(kind).duration);if(isEnergyMachine(kind))initialisePowerDevice(m);initialiseControlDevice(m);auto [i,ok]=f.machines.emplace(p,m);if(!ok)throw std::logic_error("Feedback apparatus overlap");return i->second;};
 auto node=[&](Position p,E medium)->PowerLink&{if(auto l=f.energyLink(p,medium))return *l;PowerLink l;l.medium=medium;l.ports=0;l.rating=60;f.energyLinks[p].push_back(l);return *f.energyLink(p,medium);};
 auto path=[&](E medium,Position a,std::vector<Position> bends){for(auto b:bends)while(a!=b){int d=a.x!=b.x?(a.x<b.x?0:2):(a.y<b.y?1:3);auto q=nextPosition(a,d);node(a,medium).ports|=1<<d;node(q,medium).ports|=1<<((d+2)%4);a=q;}};
 auto connect=[&](Position source,Position sink,int input,std::vector<Position> bends={}){auto a=f.at(source),b=f.at(sink);if(!a||!b)throw std::logic_error("Feedback wire endpoint missing at "+positionName(source)+" -> "+positionName(sink));int out=controlOutputPort(*a),in=controlInputPorts(*b).at(size_t(input));Position first=nextPosition(source,out),last=nextPosition(sink,in);node(first,E::Control).ports|=1<<((out+2)%4);node(last,E::Control).ports|=1<<((in+2)%4);bends.push_back(last);path(E::Control,first,bends);};
 auto comparator=[&](Position p,double target,double tolerance,bool below)->Machine&{auto& m=add(p,K::ThresholdController);m.control.setpoint=target;m.control.deadband=tolerance;m.control.below=below;return m;};
 auto actuator=[&](Position p,int dir,ControlAction a)->Machine&{auto& m=add(p,K::ControlActuator,dir);m.control.action=a;return m;};
 // The research burette repeatedly measures the same food rather than adding
 // a fixed recipe dose. Its next batch receives real, metered lactic acid.
 auto& ph=add({2,7},K::ProcessSensor,1);ph.control.metric=ControlMetric::PH;
 comparator({0,7},4.5,.1,false);auto& dose=actuator({2,9},3,ControlAction::DosePulse);dose.control.period=2;
 f.at({2,8})->ingredient="lactic_acid";f.at({2,8})->dose=.02;
 if(auto culture=f.at({-15,9});culture&&culture->kind==K::CultureVat&&!culture->item)culture->ingredient="lactic_acid";
 connect({2,7},{0,7},0,{{2,6},{-1,6}});connect({0,7},{2,9},0,{{1,10}});
 // The existing temperature logger drives a two-position thermal jacket.
 comparator({4,6},65,4,true);auto& heat=actuator({3,9},3,ControlAction::HeatCool);heat.control.hot=90;heat.control.cold=8;
 f.at({3,8})->duration=80;connect({3,7},{4,6},0);connect({4,6},{3,9},0,{{6,6},{6,10}});
 // Soil moisture enables a short irrigation duty cycle, conserving real water.
 comparator({-2,-6},.62,.08,true);auto& timer=add({0,-6},K::ControlTimer);timer.control.period=6;timer.control.onTicks=2;
 actuator({0,-7},2,ControlAction::RunPermit);connect({-2,-8},{-2,-6},0,{{-3,-6}});connect({-2,-6},{0,-6},0);connect({0,-6},{0,-7},0);
 // Two independent readings form a permissive interlock. It can scram, but
 // restoring the permit never automatically re-arms experimental excitation.
 auto& cooling=add({-12,-9},K::ProcessSensor);cooling.control.metric=ControlMetric::ReactorCooling;
 comparator({-15,-10},.4,.1,false);auto& recorder=*f.at({-11,-10});recorder.control.monitor=true;recorder.control.metric=ControlMetric::ReactorTemperature;recorder.dir=1;
 comparator({-11,-12},80,4,true);add({-9,-12},K::LogicGate);auto& interlock=actuator({-11,-7},3,ControlAction::ReactorInterlock);interlock.control.range=2;
 connect({-12,-9},{-15,-10},0,{{-17,-9},{-17,-10}});connect({-15,-10},{-9,-12},1,{{-14,-13},{-9,-13}});
 connect({-11,-10},{-11,-12},0,{{-12,-11}});connect({-11,-12},{-9,-12},0);connect({-9,-12},{-11,-7},0,{{-6,-12},{-6,-6}});
 // Insert a physical selector between disinfection and clean storage. Moving
 // the two original vessels preserves every litre, contaminant and setting.
 auto outlet=std::move(f.machines.at({-10,-4}));f.machines.erase({-10,-4});auto tank=std::move(f.machines.at({-11,-4}));f.machines.erase({-11,-4});
 f.machines.emplace(Position{-9,-4},std::move(outlet));f.machines.emplace(Position{-10,-4},std::move(tank));add({-11,-4},K::WaterDiverter);add({-9,-2},K::ReedReclaimer);
 path(E::AC,{-8,-4},{{-7,-4},{-7,-1},{-10,-1},{-10,-3},{-9,-3}});node({-9,-3},E::AC).ports|=3;
 path(E::Concentrate,{-9,-3},{{-10,-3}});node({-9,-3},E::Concentrate).ports|=8;
 path(E::Greywater,{-11,-3},{{-11,-2},{-10,-2}});node({-11,-3},E::Greywater).ports|=8;node({-10,-2},E::Greywater).ports|=1;
 path(E::RawWater,{-8,-2},{{-8,-1},{-14,-1},{-14,-2}});node({-8,-2},E::RawWater).ports|=4;
 path(E::Concentrate,{-9,-1},{{-13,-1}});node({-9,-1},E::Concentrate).ports|=8;
 auto& quality=add({-12,-5},K::ProcessSensor,1);quality.control.metric=ControlMetric::WaterQuality;
 auto& qualityThreshold=comparator({-13,-6},.975,.05,false);qualityThreshold.dir=2;actuator({-11,-3},3,ControlAction::DivertWater);
 connect({-12,-5},{-13,-6},0);connect({-13,-6},{-11,-3},0,{{-17,-6},{-17,-2}});
 f.captureBlueprint({-2,6},{6,10},"09 / Measured pH and thermal feedback",false);
 f.captureBlueprint({-3,-8},{1,-6},"10 / Moisture-gated irrigation timer",false);
 f.captureBlueprint({-17,-13},{-6,-6},"11 / Two-channel reactor permissive",false);
 f.captureBlueprint({-16,-6},{-7,-1},"12 / Water quality diversion and retreatment",false);
 return true;
}
bool Factory::addFeedbackControls(){snapshot();if(!installDemoFeedback(*this)){history.pop_back();notice="Feedback examples need the integrated campus with an empty research burette and flask. Build controls from the library, or use Demo for a fresh instrumented campus.";return false;}powerRuntime.reset();dirty=true;notice="Feedback controls installed without resetting the campus: pH dosing, thermal selection, timed irrigation, water diversion and a two-channel reactor interlock. Run or Step to measure; a scrammed reactor needs manual re-arming after its permit is valid.";return true;}
}
