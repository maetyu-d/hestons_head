#pragma once
#include "Culinary.h"
#include <cstdint>
#include <memory>

namespace aftertaste {
struct Position {
 int x=0,y=0;
 bool operator<(const Position& b)const{return y!=b.y?y<b.y:x<b.x;}
 bool operator==(const Position& b)const{return x==b.x&&y==b.y;}
 bool operator!=(const Position& b)const{return !(*this==b);}
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Position,x,y)
Position nextPosition(Position p,int direction);
std::string positionName(Position p);

enum class MachineKind {Belt,Feeder,Mixer,Gel,Aerator,Chiller,Heater,Dryer,Cloche,Cycler,Dispatch,Bin,Doser,Homogenizer,Spherifier,SousVide,Fermenter,EnzymeReactor,Centrifuge,Filter,VacuumChamber,FreezeDryer,Extruder,Plater,Smoker,Mill,Binder,RecipeKitchen,GlassTube,GlassCoil,GlassValve,GlassBeaker,GlassFlask,GlassCondenser,Burette,GlassJunction,FractionatingStill,FractionatingColumn,FractionReceiver,LatticeReactor,Wall,UtilityLine,UtilityStation,PHProbe,TemperatureLogger,DropletViewer,AromaSensor,FieldBed,RaisedBed,HydroponicBed,SeedHopper,Fertigation,GrowLight,PollinatorHive,SoilProbe,Harvester,Washer,Grader,ColdStore,ProduceIntake,CompostBin,PowerCable,DCCable,DriveShaft,SteamPipe,ExhaustPipe,RawGasPipe,GasPipe,ReturnPipe,HeatPipe,CoolingPipe,SolarPanel,WindRotor,Gearbox,Alternator,BiomassBoiler,SteamAccumulator,SteamTurbine,PowerCondenser,FeedPump,HeatExchanger,Macerator,AnaerobicDigester,GasScrubber,GasHolder,GasEngine,BatteryRack,Flywheel,Inverter,Switchboard,PowerRelay,GridInlet,HeatTank,CoolingPlant,Count};
enum class UtilityKind {Power,Heat,Cooling,Water,Vacuum,Count};
enum class ServeStyle {OrderPass,ResearchTasting,ChilledPickup};
std::string serveStyleName(ServeStyle style);
std::string utilityName(UtilityKind kind);
std::string utilityCode(UtilityKind kind);
bool isUtilityEquipment(MachineKind kind);
bool isSensor(MachineKind kind);
bool isFarmBed(MachineKind kind);
bool isFarmEquipment(MachineKind kind);
bool isFarmInfrastructure(MachineKind kind);
std::vector<std::string> farmCropKeys();
std::string farmSeasonName(int tick);
double farmSeasonTemperatureOffset(int tick);
double farmSeasonDaylight(int tick);
enum LatticeFamily {LatticeSugar,LatticeProtein,LatticeFat,LatticeHydrocolloid,LatticeStarch,LatticeAcidBase,LatticeCalcium,LatticeLecithin,LatticeFamilyCount};
struct MachineSpec {
 MachineKind kind;std::string name,code,description,color;
 int cost=0,duration=0;double power=0,water=0;
};
const std::array<MachineSpec,size_t(MachineKind::Count)>& machineSpecs();
const MachineSpec& machineSpec(MachineKind kind);
bool isTransport(MachineKind kind);
bool hasSideOutlet(MachineKind kind);
bool isGlassConduit(MachineKind kind);
bool isLabGlassware(MachineKind kind);
bool isMixingMachine(MachineKind kind);
bool isDosingMachine(MachineKind kind);
bool isTastingDock(MachineKind kind);
bool isProcessingMachine(MachineKind kind);
enum class EnergyMedium {AC,DC,Shaft,Steam,Exhaust,RawGas,Gas,Water,Heat,Cooling,Count};
constexpr size_t EnergyCount=size_t(EnergyMedium::Count);
struct PowerState {
 std::array<double,EnergyCount> store{};
 double workCredit=0;double rate=12,capacity=200,throttle=1,temperature=20,rpm=0,flow=0,generated=0,delivered=0,demand=0,voltage=1,heat=0,low=.25,high=.75;
 int priority=1;bool tripped=false,armed=true;
 std::vector<double> history;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PowerState,store,workCredit,rate,capacity,throttle,temperature,rpm,flow,generated,delivered,demand,voltage,heat,low,high,priority,tripped,armed,history)
struct PowerLink {EnergyMedium medium=EnergyMedium::AC;double rating=60,flow=0,heat=0,overload=0;int dir=0;bool enabled=true,tripped=false;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PowerLink,medium,rating,flow,heat,overload,dir,enabled,tripped)
struct EnergyPort {EnergyMedium medium;int direction;bool output=false;};
struct PowerTotals {double generated=0,requested=0,delivered=0,loss=0,stored=0,recoveredHeat=0,ventedMass=0;bool active=false;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PowerTotals,generated,requested,delivered,loss,stored,recoveredHeat,ventedMass,active)
bool isEnergyMachine(MachineKind kind);bool isEnergyConduit(MachineKind kind);bool acceptsEnergyFuel(MachineKind kind);
std::string energyName(EnergyMedium medium);EnergyMedium conduitMedium(MachineKind kind);MachineKind energyConduit(EnergyMedium medium);
struct Machine;std::vector<EnergyPort> energyPorts(const Machine& machine);
class PowerRuntime;
struct Batch {
 uint64_t id=0;int age=0;Cell matter;CulinaryBatch food;uint64_t dish=0;std::vector<Position> route;
 bool cropBatch=false;double cropQuality=1,cropDirt=0,cropFreshness=1;std::string residue;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Batch,id,age,matter,food,dish,route,cropBatch,cropQuality,cropDirt,cropFreshness,residue)
struct Machine {
 PowerState power;
 MachineKind kind=MachineKind::Belt;int dir=0,duration=4,progress=0,cooldown=0;
 std::string ingredient="brine",status="Idle";
 double temperature=90,wear=0,dose=1,shear=.8;
 int cut=1,stages=8;
 UtilityKind utility=UtilityKind::Power;double utilityCapacity=20;
 double sensorValue=0;uint64_t sensorBatch=0;int sensorTick=-1;std::vector<double> sensorHistory;
 ServeStyle serveStyle=ServeStyle::OrderPass;std::string serviceFeedback;
 uint64_t recipe=0;int cursor=0,subtick=0;
 bool farmPlanted=false;int farmAge=0;double farmGrowth=0,farmMoisture=.62,farmPH=6.4,farmN=.55,farmP=.5,farmK=.55,farmOrganic=.3,farmHealth=1,farmRotationStress=0;std::string farmLastCrop;
 std::vector<RecipeStep> program;std::vector<uint64_t> programOutput;
 bool enabled=true,ready=false,automatic=true,forward=false;
 std::vector<int> lattice;uint64_t latticeBatch=0;int latticePending=-1,latticeMoves=0;bool latticeRelease=false;
 std::optional<Batch> item,extra,sideOutput,lastOutput;
};
void to_json(json& j,const Machine& m);
void from_json(const json& j,Machine& m);
struct Request {
 std::string name,phase,description;
 std::map<std::string,double> minimum;
 double lowTemp=-3.6,highTemp=3.6;
 int need=6,bonus=80,window=240;
};
const std::vector<Request>& factoryRequests();
struct Quality {double score=0;bool matches=false;std::string reason;};
struct Product {
 std::string name,phase;int count=0;double best=0,totalQuality=0,totalValue=0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Product,name,phase,count,best,totalQuality,totalValue)
struct Shipment {
 std::string product,phase;double quality=0,value=0;bool requested=false;int tick=0;
 std::string audience,servedAt,feedback;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Shipment,product,phase,quality,value,requested,tick,audience,servedAt,feedback)
struct BlueprintPart {Position at;Machine machine;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BlueprintPart,at,machine)
struct BlueprintLink {Position at;PowerLink link;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BlueprintLink,at,link)
struct Blueprint {std::string name;int width=1,height=1;std::vector<BlueprintPart> parts;std::vector<BlueprintLink> links;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Blueprint,name,width,height,parts,links)
struct FactoryState {
 std::map<Position,std::vector<PowerLink>> energyLinks;
 PowerTotals grid;
 std::map<Position,Machine> machines;
 std::map<std::string,double> stock;
 std::map<std::string,double> farmInventory,farmQuality;
 std::map<std::string,int> seedStock;
 std::map<std::string,Product> products;
 std::vector<Shipment> shipments;
 std::vector<Blueprint> blueprints;
 std::vector<int> outputHistory;
 std::set<std::string> discoveries;
 double cash=1200,water=150,powerCapacity=16,electricity=10000,farmProduce=50,chemicals=2.5,energy=0,waterUsed=0,powerUsed=0,revenue=0;
 int tick=0,shipped=0,wasted=0,rawUsed=0,requestsDone=0,request=0,requestProgress=0,requestDeadline=240;
 uint64_t nextId=1,nextComponent=1,nextOperation=1,nextDish=1;
 DishBench bench;std::vector<DishDesign> dishbook;
 std::map<uint64_t,Blueprint> dishLayouts;
 bool sandbox=true;
};
void to_json(json& j,const FactoryState& s);
void from_json(const json& j,FactoryState& s);
class Factory:public FactoryState {
public:
 Factory();
 bool running=false,dirty=false,expandedStarterDemo=false;
 std::string notice="Sandbox ready. Press Space to run the sample factory, or build anywhere.";
 std::filesystem::path storageFile;
 std::deque<FactoryState> history;
 void reset();
 void snapshot();void undo();
 bool build(Position p,MachineKind kind,int dir=0);
 bool demolish(Position p);bool rotate(Position p);bool configure(Position p,const std::string& setting);
 bool buyStock(const std::string& ingredient,int count=10);
 void refillWater();void upgradePower();void receiveElectricity();void receiveFarmProduce();void receiveChemicals();void flush();void toggleEconomy();
 void setRequest(int index);
 bool dispense(Position p);bool serveAt(Position p);bool sendOnward(Position p);bool discardBatch(Position p,int slot=0);
 const Batch* batchAt(Position p,int slot=0)const;
 uint64_t recordDish(Position p,const std::string& name,int slot=0,uint64_t expectedBatch=0);
 static Blueprint experimentBlueprint();
 static Blueprint glassLabBlueprint();
 static Blueprint fractionalDistillationBlueprint();
 static Blueprint marketGardenBlueprint();
 static Blueprint farmDeliveryBlueprint();
 static Blueprint powerDistrictBlueprint();
 bool installPowerDistrict();
 bool buildEnergyLink(Position p,EnergyMedium medium);bool removeEnergyLink(Position p,EnergyMedium medium);bool configureEnergyLink(Position p,EnergyMedium medium,const std::string& setting);
 PowerLink* energyLink(Position p,EnergyMedium medium);const PowerLink* energyLink(Position p,EnergyMedium medium)const;
 bool powerAvailable(Position p,double amount)const;void usePower(Position p,double amount);
 void beginPowerStep();void finishPowerStep();
 std::optional<Blueprint> productionLine(uint64_t dish)const;
 bool setIngredient(Position p,const std::string& key);
 bool setRecipe(Position p,uint64_t dish);
 bool prepareLattice(Position p);bool swapLattice(Position p,int first,int second);bool releaseLattice(Position p);
 uint64_t newComponent();void benchAdd(const std::string& key,double grams);
 void benchProcess(Environment e,int ticks);void benchBlend();void benchPlate(bool all);void benchSeparate(Technique technique,int ticks=0);
 uint64_t saveDish(const std::string& name);bool loadDish(uint64_t id);bool sampleBatch(Position p);void clearBench();
 const DishDesign* dish(uint64_t id)const;
 bool replayDish(uint64_t id,CulinaryBatch& output);
 void step();
 Machine* at(Position p);const Machine* at(Position p)const;
 int batches()const;int processingMachines()const;
 std::set<Position> enclosedFactoryTiles()const;
 Quality assess(const Batch& b,int requestIndex=-1)const;
 double marketValue(const Batch& b)const;
 bool captureBlueprint(Position a,Position b,const std::string& name="");
 static Blueprint rotatedBlueprint(Blueprint b);
 bool stamp(const Blueprint& b,Position origin);
 bool canStamp(const Blueprint& b,Position origin)const;
 void setPlanName(size_t index,const std::string& name);
 bool save()const;bool load();
private:
 std::shared_ptr<PowerRuntime> powerRuntime;
 static void rebuildLattice(Machine& machine,const Batch& batch);
 static bool latticeReaction(const Machine& machine,const Batch& batch,int family,Technique& technique,std::string& reason);
 void updateTicketClock();
 std::string missingInputSupply(const std::string& ingredient,double grams)const;void consumeInputSupply(const std::string& ingredient,double grams);
 void chemistryStep(Batch& b,const Machine& machine,bool processing);
 bool startKitchen(Machine& machine);void kitchenStep(Machine& machine);
 void remap(CulinaryBatch& batch,std::vector<uint64_t>* output=nullptr);
 Batch makeBatch(const std::string& ingredient,double grams=100);
 bool accept(Machine& destination,Batch item);
 void ship(Machine& machine,Position at);
 void merge(Machine& machine);
};
} // namespace aftertaste
