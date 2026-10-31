#pragma once
#include "Factory.h"
namespace aftertaste {
std::vector<MachineSpec> controlMachineSpecs();
bool isControlDevice(MachineKind kind);
bool isControlLogic(MachineKind kind);
bool isControlSource(const Machine& machine);
std::vector<int> controlInputPorts(const Machine& machine);
int controlOutputPort(const Machine& machine);
Position controlTarget(Position at,const Machine& machine);
std::string controlMetricName(ControlMetric metric);
std::string controlActionName(ControlAction action);
void initialiseControlDevice(Machine& machine);
void resetControlRuntime(Machine& machine);
bool validControl(const ControlState& state);
inline bool controlWaiting(const ControlState& c){return !c.valid&&(c.fault=="No live batch"||c.fault=="No water sample");}
bool configureControl(Factory& factory,Machine& machine,const std::string& setting);
struct ControlSignal {bool valid=false;double value=0;Position source;std::string fault;};
using ControlSignals=std::map<Position,ControlSignal>;
ControlSignal controlInput(const Factory& factory,Position at,int direction,const ControlSignals& signals);
class ControlFrame {
 Factory& factory;
 struct Restore {Position at;bool enabled;double temperature;};
 std::vector<Restore> restored;
public:
 explicit ControlFrame(Factory& factory);
 ~ControlFrame();
 ControlFrame(const ControlFrame&)=delete;
 ControlFrame& operator=(const ControlFrame&)=delete;
};
bool installDemoFeedback(Factory& factory);
}
