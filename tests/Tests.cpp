#include "Model.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace aftertaste;
static int checks=0;
static void require(bool value,const std::string& message){checks++;if(!value)throw std::runtime_error(message);}
static void compare(const json& expected,const json& actual,const std::string& at){
 if(expected.is_number()&&actual.is_number()){double a=expected.get<double>(),b=actual.get<double>();require(std::abs(a-b)<1e-9*(1+std::abs(a)),at+" expected "+expected.dump()+" got "+actual.dump());return;}
 if(expected.is_object()){require(actual.is_object(),at+" object required");for(auto it=expected.begin();it!=expected.end();++it){require(actual.contains(it.key()),at+" missing "+it.key());compare(it.value(),actual.at(it.key()),at+"."+it.key());}return;}
 if(expected.is_array()){require(actual.is_array()&&expected.size()==actual.size(),at+" array length differs");for(size_t i=0;i<expected.size();i++)compare(expected[i],actual[i],at+"["+std::to_string(i)+"]");return;}
 require(expected==actual,at+" expected "+expected.dump()+" got "+actual.dump());
}
static void action(Game& g,const json& a){
 std::string k=a[0];if(k=="wait")g.wait();else if(k=="place"){g.chooseTool(a[1]);g.apply(a[2]);}else if(k=="move")g.moveTop(a[1],a[2]);else if(k=="setting")g.changeSetting(a[1],a[2]);else if(k=="undo")g.undo();else if(k=="serve")g.serve();else if(k=="route"){g.routeMode();for(int i:a[1])g.trace(i);}
}
static json forecastJson(const Forecast& f){
 json turns=json::array(),deltas=json::array();for(auto& t:f.turns)turns.push_back({{"at",t.at},{"temp",t.temp},{"pressure",t.pressure},{"charge",t.charge},{"water",t.water},{"phase",t.phase},{"react",t.react},{"all",t.all},{"pulses",t.pulses},{"flow",t.flow},{"score",t.score?json(*t.score):json(nullptr)},{"delta",t.delta},{"dPressure",t.dPressure},{"pressureSpan",t.pressureSpan},{"tempSpan",t.tempSpan},{"newRx",t.newRx},{"flowDelta",t.flowDelta},{"changed",t.changed}});
 for(auto& d:f.deltas)deltas.push_back({{"i",d.i},{"temp",d.temp},{"chem",d.chem},{"feed",d.feed}});
 return {{"risk",f.risk},{"impact",f.impact},{"chains",f.chains},{"chainTargets",f.chainTargets},{"deltas",deltas},{"turns",turns}};
}
int main(){try{
 std::ifstream stream(FIXTURE_PATH);json reference;stream>>reference;
 for(auto& fixture:reference["fixtures"]){Game g;g.reset(fixture["course"],false);
  if(!fixture["setup"].get<std::string>().empty()){g.board[60].charge=1.6;g.board[60].gas=1.5;g.board[60].stack.push_back(g.makePiece("broth"));g.board[60].stack.push_back(g.makePiece("bell"));g.board[59].charge=.8;g.board[59].stack.push_back(g.makePiece("cream"));g.board[58].charge=.8;}
  for(auto& a:fixture["actions"])action(g,a);compare(fixture["state"],g.stateJson(),fixture["name"].get<std::string>());std::cout<<"PASS "<<fixture["name"]<<'\n';
 }
 Game g;g.chooseTool("heat");auto before=g.stateJson();int uid=g.uid;auto forecast=g.forecastAction(60);require(forecast.has_value(),"forecast must be available");compare(reference["forecast"],forecastJson(*forecast),"forecast");require(g.stateJson()==before&&g.uid==uid&&g.history.empty(),"forecast mutated live state");auto compared=g.forecastAction(60,true);require(compared&&compared->variants.size()==3,"comparison must include action, alternative, wait");require(g.stateJson()==before,"comparison mutated live state");
 g.reset(6,false);g.chooseTool("gel");g.apply(0);require(g.moves==0&&g.board[0].stack.empty(),"cap accepted on empty chamber");g.chooseTool("syrup");for(int i=0;i<9;i++)g.apply(0);require(g.board[0].stack.size()==8&&g.moves==8,"eight-layer stack limit");
 g.reset(0,false);g.stock["heat"]=1;g.chooseTool("heat");g.apply(60);require(g.stock["heat"]==0&&g.board[60].temp>1,"last heat charge did not work");g.undo();require(g.stock["heat"]==1&&g.moves==0,"undo failed to restore inventory");g.chooseTool("hearth");g.apply(56);require(g.board[56].source&&g.board[56].tile->k=="pipe"&&g.moves==0,"fixed reservoir was replaced");
 for(int i=0;i<45;i++)g.wait();require(g.moves==g.budget(),"budget exceeded");
 g.reset(6,false);g.routeMode();g.trace(57);g.trace(59);require(g.path.size()==1,"disconnected route accepted");g.trace(58);g.trace(57);require(g.path.size()==2,"duplicate route accepted");
 auto testDir=std::filesystem::temp_directory_path()/"aftertaste-native-tests";std::filesystem::create_directories(testDir);g.storageFile=testDir/"notebook.json";g.reset(6,false);g.wait();require(g.savePlate(),"plate save failed");auto savedBoard=json(g.board);g.chooseTool("heat");g.apply(60);g.dinerNo=3;require(g.loadPlate(0),"plate load failed");require(json(g.board)==savedBoard&&g.diner.n==3,"plate replay lost state or diner identity");
 Game restored;restored.storageFile=g.storageFile;require(restored.loadPersistent()&&restored.notebook==g.notebook,"notebook persistence mismatch");restored.notebook["plates"][0]["board"][0]["tile"]={{"k","pump"},{"dir",99}};auto unmodified=restored.stateJson();require(!restored.loadPlate(0)&&unmodified==restored.stateJson(),"malformed saved plate mutated state");
 {std::ofstream corrupt(g.storageFile);corrupt<<"not json";}require(!restored.loadPersistent(),"corrupt notebook accepted");std::filesystem::remove_all(testDir);
 std::cout<<"PASS forecasts, boundaries, persistence; "<<checks<<" assertions\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
