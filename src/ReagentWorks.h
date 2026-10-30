#pragma once
#include "Factory.h"
#include <functional>
namespace aftertaste {
inline constexpr Position reagentWorksOrigin{-21,13};
bool isReagentMachine(MachineKind kind);
bool acceptsReagentMaterial(MachineKind kind);
bool reagentCompatible(MachineKind kind,const Batch& batch);
std::vector<MachineSpec> reagentMachineSpecs();
std::vector<std::string> reagentRecipes(MachineKind kind);
void initialiseReagentDevice(Machine& machine);
void clearReagentInventory(Machine& machine);
bool validReagentState(const Machine& machine);
bool validReagentLot(const Batch& batch);
bool validReagentWorks(const FactoryState& factory);
double reagentPurity(const Batch& batch);
double reagentConcentration(const Batch& batch);
double genericChemicals(const FactoryState& factory);
bool hasReagentSupply(const FactoryState& factory,const std::string& key,double grams);
bool canSupplyReagents(const FactoryState& factory,const std::map<std::string,double>& grams);
bool consumeReagentSupply(FactoryState& factory,const std::string& key,double grams);
bool configureReagentDevice(Factory& factory,Machine& machine,const std::string& setting);
void disposeReagentInventory(FactoryState& factory,Machine& machine);
bool installReagentWorksIn(FactoryState& factory);
using ReagentPower=std::function<bool(Position,double)>;
void runReagentWorks(Factory& factory,const ReagentPower& power);
}
