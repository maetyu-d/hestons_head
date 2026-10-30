#pragma once
#include "Factory.h"
#include <functional>
namespace aftertaste {
inline constexpr Position waterWorksOrigin{-17,6};
bool isWaterMachine(MachineKind kind);bool isWaterStorage(MachineKind kind);bool storesWaterMedium(MachineKind kind,EnergyMedium medium);bool isWaterStream(EnergyMedium medium);
std::vector<MachineSpec> waterMachineSpecs();std::vector<EnergyPort> waterPorts(const Machine& m);
void initialiseWaterDevice(Machine& m);void clearWaterInventory(Machine& m);
bool validWaterState(const Machine& m);bool validWaterWorksState(const WaterWorksState& w);
WaterParcel storedWater(const Machine& m,EnergyMedium e);WaterParcel waterSample(const Machine& m);
WaterParcel removeWaterLoad(Machine& m,EnergyMedium e,double volume);
void addWaterParcel(WaterParcel& target,const WaterParcel& parcel);
double putWater(Machine& m,EnergyMedium e,WaterParcel parcel);
bool acceptableWater(WaterParcel parcel);double waterRainfall(int tick);
void collectGreywater(FactoryState& f,double volume);void disposeWaterInventory(FactoryState& f,Machine& m);
bool configureWaterDevice(Factory& f,Machine& m,const std::string& setting);
bool installWaterWorksIn(FactoryState& f);
using WaterPull=std::function<WaterParcel(Position,EnergyMedium,double,double,bool)>;
using WaterPower=std::function<bool(Position,double)>;
void runWaterWorks(Factory& f,const WaterPull& pull,const WaterPower& power);
}
