#pragma once
#include "Model.h"
#include <cstdint>

namespace aftertaste {
enum Compound {Water,Sucrose,Reducing,Acid,Salt,Protein,Fat,Calcium,Alginate,Agar,Gelatin,Pectin,Lecithin,Starch,Culture,Protease,Amylase,Ethanol,Fibre,Base,Amino,CO2,Transglutaminase,Xanthan,Other,CompoundCount};
using Compounds=std::array<double,CompoundCount>;
using Scent=std::array<double,6>;
struct Ingredient {
 std::string key,name,code,color,category,description;
 Compounds fractions{};Scent aroma{};
 double bitter=0,umami=0,buffer=0,yeast=0,dose=100,cost=2;
};
const std::vector<Ingredient>& pantry();
const Ingredient& ingredient(const std::string& key);
void extendPantryData(json& d);
enum class Technique {Rest,Mix,Gel,Aerate,Freeze,Heat,Dry,Seal,Cycle,Homogenize,Spherify,SousVide,Ferment,Enzyme,Centrifuge,Filter,Vacuum,FreezeDry,Extrude,Smoke,Mill,Bind,Distill,Reflux,Condense};
struct Environment {
 Technique technique=Technique::Rest;double temperature=20,shear=.5,pressure=1;
 int cut=1,stages=8;
 bool operator==(const Environment& b)const{return technique==b.technique&&temperature==b.temperature&&shear==b.shear&&pressure==b.pressure&&cut==b.cut&&stages==b.stages;}
};
void to_json(json& j,const Environment& e);void from_json(const json& j,Environment& e);
struct Food {
 Compounds c{};Scent aroma{};std::map<std::string,double> ingredients;
 double temperature=20,age=0,bitter=0,umami=0,buffer=0,yeast=0;
 double denatured=0,agarHydrated=0,gelatinHydrated=0,starchCooked=0,retrograded=0,boundCalcium=0,proteinBond=0,particle=180;
 double bubbles=0,droplet=90,crystalSize=0,ice=0,brown=0,scorch=0,coagulated=0;
 double gel=0,shell=0,stability=1,aromaReleased=0,pressure=1,smoke=0,lostMass=0;
 std::string phase="liquid",fraction;bool vapor=false;std::vector<std::string> events;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Food,c,aroma,ingredients,temperature,age,bitter,umami,buffer,yeast,denatured,agarHydrated,gelatinHydrated,starchCooked,retrograded,boundCalcium,proteinBond,particle,bubbles,droplet,crystalSize,ice,brown,scorch,coagulated,gel,shell,stability,aromaReleased,pressure,smoke,lostMass,phase,fraction,vapor,events)
struct FoodPart {uint64_t id=0;Food food;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FoodPart,id,food)
enum class RecipeAction {Add,Process,Blend,Split};
struct RecipeStep {
 uint64_t id=0,result=0,side=0;
 RecipeAction action=RecipeAction::Add;
 std::vector<uint64_t> targets;
 std::string ingredient;double grams=0;int ticks=1;
 Environment environment;
};
void to_json(json& j,const RecipeStep& s);void from_json(const json& j,RecipeStep& s);
struct CulinaryBatch {
 std::vector<FoodPart> parts;std::vector<RecipeStep> history;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CulinaryBatch,parts,history)
struct FoodMetrics {
 double mass=0,pH=7,waterActivity=1,viscosity=1,gel=0,foam=0,emulsion=0,ice=0,temperature=20;
 double quality=0,flavourBalance=0,contrast=0,aroma=0,stability=1,brown=0,scorch=0;
 Vec flavour{};std::string phase="empty",fraction;bool vapor=false;std::vector<std::string> notes;
};
struct DishDesign {
 uint64_t id=0;std::string name;
 CulinaryBatch sample;std::vector<uint64_t> output;
 double quality=0;int made=0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DishDesign,id,name,sample,output,quality,made)
struct DishBench {CulinaryBatch batch;uint64_t selected=0;std::string name="Untitled dish";};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DishBench,batch,selected,name)
double foodMass(const Food& f);double foodPH(const Food& f);double waterActivity(const Food& f);double viscosity(const Food& f);
FoodMetrics analyse(const CulinaryBatch& b);
void addIngredient(CulinaryBatch& b,const std::string& key,double grams,uint64_t part,uint64_t operation,bool record=true);
void processFood(Food& f,const Environment& e);
void processBatch(CulinaryBatch& b,const Environment& e,const std::vector<uint64_t>& targets,uint64_t operation,bool record=true);
void blendParts(CulinaryBatch& b,const std::vector<uint64_t>& inputs,uint64_t output,uint64_t operation,bool record=true);
void splitPart(CulinaryBatch& b,uint64_t target,uint64_t main,uint64_t side,const Environment& e,uint64_t operation,bool record=true);
bool executeStep(CulinaryBatch& b,const RecipeStep& step,bool record=false);
void mergeHistories(CulinaryBatch& a,const CulinaryBatch& b);
Cell projectCell(const CulinaryBatch& b);
bool validCulinary(const CulinaryBatch& b);
std::map<std::string,double> formula(const CulinaryBatch& b);
std::string techniqueName(Technique t);
std::string distillationCutName(int cut);
} // namespace aftertaste
