#pragma once
#include "json.hpp"
#include <array>
#include <cmath>
#include <deque>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace aftertaste {
using json = nlohmann::json;
using Vec = std::array<double,5>;
using Aroma = std::array<double,4>;
constexpr int N=11, SEQUENCE=6, STACK_LIMIT=8;
inline double clamp(double x,double a,double b){return std::min(b,std::max(a,x));}
inline int idx(int x,int y){return y*N+x;}
inline int xof(int i){return i%N;}
inline int yof(int i){return i/N;}
inline bool adjacent(int i,int j){return i>=0&&j>=0&&std::abs(xof(i)-xof(j))+std::abs(yof(i)-yof(j))==1;}
inline const std::array<std::array<int,2>,4> directions={{{1,0},{0,1},{-1,0},{0,-1}}};
inline const std::array<std::string,7> materialKeys={"water","sugar","acid","salt","protein","fat","bitter"};
inline const std::array<std::string,8> soluteKeys={"sugar","acid","salt","protein","fat","bitter","activation","sourced"};
inline const std::array<std::string,5> flavourNames={"SWEET","ACID","SALTY","UMAMI","BITTER"};
const json& data();
std::string coord(int i);
std::vector<int> neighbours(int i);
std::string upper(std::string s);
std::string roundTo(double n,int digits=1);
bool networkType(const std::string& k);
bool floorType(const std::string& k);
bool capType(const std::string& k);

struct Material {
 double water=0,sugar=0,acid=0,salt=0,protein=0,fat=0,bitter=0,activation=0,sourced=0;
 double& at(const std::string& key);
 double at(const std::string& key) const;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Material,water,sugar,acid,salt,protein,fat,bitter,activation,sourced)
struct Piece:Material {
 int id=0,age=0,mask=0;
 std::string k,kind="food",phase="liquid";
 double temp=0,porosity=.8,gel=0,foam=0,emulsion=0,crisp=0,brown=0,scorch=0,split=0,ice=0,integrity=1,pressure=0;
 Aroma aroma{};
 bool spent=false;
};
// Food has a null mask in the source; normalize it to the inactive value zero.
void to_json(json& j,const Piece& p);
void from_json(const json& j,Piece& p);
struct Tile {std::string k;int dir=0;bool open=true;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Tile,k,dir,open)
struct Source {std::string key;double remaining=0,rate=.28;};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Source,key,remaining,rate)
struct Cell {
 std::vector<Piece> stack;
 std::optional<Tile> tile;
 std::optional<Source> source;
 double temp=0,humidity=.24,burst=0,pressure=0,gas=0,charge=0,wave=0,flux=0,flowAmount=0,delivered=0,waste=0,fouling=0,dwell=0,energy=0,ruptures=0;
 int flowDir=0,lastCascade=-99,lastPressure=-99;
 std::string last,flowHue="#00b6f2";
 Aroma aroma{};
 Material solution;
};
void to_json(json& j,const Cell& c);
void from_json(const json& j,Cell& c);
using Board=std::array<Cell,N*N>;
struct Profile {
 Vec f{};Aroma a{};double temp=0,water=0,integrity=0,roast=0,scorch=0,split=0,reveal=0;
 std::string phase="empty";int visible=0,layers=0;bool seal=false,has=false,chemistry=false;
 const Piece* face=nullptr;
};
bool edible(const Cell& c);
std::vector<Piece*> foods(Cell& c);
std::vector<const Piece*> foods(const Cell& c);
int faceIndex(const Cell& c);
Piece* exposed(Cell& c);
const Piece* exposed(const Cell& c);
Vec taste(const Piece& p);
Profile profile(const Cell& c);
struct Diner {
 std::string name,texture,novelLabel,color;int n=0,like=0,avoid=0,contract=0;
 Vec sensitivity{};double novelty=1,temperature=0,memory=.6;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Diner,name,n,like,avoid,contract,sensitivity,novelty,temperature,texture,novelLabel,color,memory)
Diner makeDiner(int chapter,int number);
std::string palateLabel(const Diner& d);
struct Engineering {
 int n=0,need=0,fed=0,fedNeed=0;double waste=0,energy=0,purity=1,ruptures=0,feedRatio=1,score=0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Engineering,n,need,fed,fedNeed,waste,energy,purity,ruptures,feedRatio,score)
struct Bite {
 int i=0,k=0;double score=0,quality=0,match=0,thermal=0,texture=0,condition=0,delight=0,d=0,surprise=0,context=0,trust=0,trustAfter=0,release=0,aftertaste=0,fed=0;
 Vec expected{},felt{};std::string phase;std::vector<std::string> details;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Bite,i,k,score,quality,match,thermal,texture,condition,delight,expected,felt,d,surprise,context,trust,trustAfter,phase,details,release,aftertaste,fed)
struct Report {
 int final=0,goalN=0,contractN=0;double avg=0,goal=0,contract=0,transport=0,wastePenalty=0,energyPenalty=0,rupturePenalty=0,impurityPenalty=0,flow=0,chemistry=0,structure=0,trust=1;
 std::vector<Bite> bites;Engineering engineering;std::string phrase;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Report,final,avg,goal,contract,transport,wastePenalty,energyPenalty,rupturePenalty,impurityPenalty,flow,chemistry,structure,goalN,contractN,bites,trust,engineering,phrase)
struct State {
 Board board{};std::map<std::string,int> stock;std::vector<int> path;
 int course=0,dinerNo=0,moves=0,clock=0,lift=-1,selected=60,tasteIndex=-1,uid=1;
 std::string tool="syrup",category="food",lens="matter",mode="place",sound="sea",menu="oyster";
 std::optional<Report> report;
 std::vector<std::string> events;std::set<std::string> runDiscoveries;
 Diner diner;
};
struct ForecastTurn {
 int at=0,all=0,pulses=0,newRx=0;double temp=0,pressure=0,charge=0,water=0,flow=0,delta=0,dPressure=0,pressureSpan=0,tempSpan=0,flowDelta=0;
 std::string phase;std::vector<std::string> react;std::optional<int> score;bool changed=false;
 Board board;
};
struct Variant {std::string name,tool;bool ok=false;std::optional<int> score;int rx=0;double pressure=0;};
struct Delta {int i=0;double temp=0,chem=0,feed=0;};
struct Forecast {
 int i=0;std::string tool,risk;std::vector<ForecastTurn> turns;std::vector<int> impact,chainTargets;std::vector<std::string> chains;std::vector<Delta> deltas;std::vector<Variant> variants;
};
class Game:public State {
public:
 Game();
 json notebook={{"discoveries",json::array()},{"plates",json::array()}};
 std::map<std::string,int> highScores;
 std::string notice;
 std::deque<State> history;
 bool forecasting=false;
 std::filesystem::path storageFile;
 int budget() const;
 void reset(int n,bool keepDiner=true);
 Piece makePiece(const std::string& k);
 void seed(int i,const std::string& k);
 void announce(const std::string& s){notice=s;}
 void snapshot();void undo();void chooseCat(const std::string& cat);void chooseTool(const std::string& k);
 void moveMode();void routeMode();void trace(int i);void moveTop(int a,int b);void manipulate(int i);
 bool place(int i);void apply(int i);void wait();void changeSetting(const std::string& which,const std::string& value);void changeDiner();
 void flag(int i,const std::string& msg);void discovered(const std::string& kind,int i);
 Board diffusionPass();void networkPass(const Board& old,Board& next);void cascadePass();void simulate();void perform(const std::string& msg);
 bool connected(int i,int j) const;
 static bool connected(int i,int j,const Board& state);
 static double pumpHead(int i,int j,const Board& old);
 int feedCount(const std::vector<int>& route) const;
 Engineering engineeringMetrics(const std::vector<int>& route) const;
 Vec expectation(const Cell& c) const;double affinity(const Profile& p) const;
 int contractCount(const std::vector<int>& route,const std::vector<Bite>& bites) const;
 Report evaluate(const std::vector<int>& route) const;
 Report evaluate() const{return evaluate(path);}
 void serve();void nextTaste();
 bool probeApply(int i,const std::string& chosenTool);
 std::vector<ForecastTurn> probeState(int i,bool action,bool perturb,const std::string& chosenTool) const;
 std::optional<Forecast> forecastAction(int i,bool compare=false) const;
 std::vector<int> networkTrace(int i) const;
 std::string actionPreview(int i) const;std::string squareTitle(int i) const;
 json stateJson() const;
 bool savePlate();bool loadPlate(int i);
 bool savePersistent() const;bool loadPersistent();
};
double textureFit(const std::string& actual,const std::string& wanted);
} // namespace aftertaste
