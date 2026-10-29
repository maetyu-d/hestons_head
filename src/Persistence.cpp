#include "Model.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <system_error>

namespace aftertaste {
void to_json(json& j,const Piece& p){
 j=json::object();
#define SAVE(field) j[#field]=p.field;
 SAVE(id) SAVE(k) SAVE(kind) SAVE(temp) SAVE(water) SAVE(sugar) SAVE(acid) SAVE(salt) SAVE(protein) SAVE(fat) SAVE(bitter) SAVE(porosity) SAVE(aroma) SAVE(gel) SAVE(foam) SAVE(emulsion) SAVE(crisp) SAVE(brown) SAVE(scorch) SAVE(split) SAVE(ice) SAVE(phase) SAVE(age) SAVE(integrity) SAVE(pressure) SAVE(spent) SAVE(sourced)
#undef SAVE
 j["mask"]=p.kind=="food"?json(nullptr):json(p.mask);
}
void from_json(const json& j,Piece& p){
 p=Piece{};
#define READ(field) if(j.contains(#field)&&!j[#field].is_null())j.at(#field).get_to(p.field);
 READ(id) READ(k) READ(kind) READ(temp) READ(water) READ(sugar) READ(acid) READ(salt) READ(protein) READ(fat) READ(bitter) READ(porosity) READ(aroma) READ(gel) READ(foam) READ(emulsion) READ(crisp) READ(brown) READ(scorch) READ(split) READ(ice) READ(phase) READ(age) READ(mask) READ(integrity) READ(pressure) READ(spent) READ(sourced)
#undef READ
}
void to_json(json& j,const Cell& c){
 j=json::object();
#define SAVE(field) j[#field]=c.field;
 SAVE(stack) SAVE(temp) SAVE(humidity) SAVE(aroma) SAVE(last) SAVE(burst) SAVE(pressure) SAVE(gas) SAVE(charge) SAVE(wave) SAVE(flux) SAVE(flowAmount) SAVE(flowDir) SAVE(flowHue) SAVE(delivered) SAVE(waste) SAVE(fouling) SAVE(dwell) SAVE(energy) SAVE(ruptures) SAVE(lastCascade) SAVE(lastPressure) SAVE(solution)
#undef SAVE
 j["tile"]=c.tile?json(*c.tile):json(nullptr);j["source"]=c.source?json(*c.source):json(nullptr);
}
void from_json(const json& j,Cell& c){
 c=Cell{};
#define READ(field) if(j.contains(#field)&&!j[#field].is_null())j.at(#field).get_to(c.field);
 READ(stack) READ(temp) READ(humidity) READ(aroma) READ(last) READ(burst) READ(pressure) READ(gas) READ(charge) READ(wave) READ(flux) READ(flowAmount) READ(flowDir) READ(flowHue) READ(delivered) READ(waste) READ(fouling) READ(dwell) READ(energy) READ(ruptures) READ(lastCascade) READ(lastPressure) READ(solution)
#undef READ
 if(j.contains("tile")&&!j["tile"].is_null())c.tile=j["tile"].get<Tile>();if(j.contains("source")&&!j["source"].is_null())c.source=j["source"].get<Source>();
}
json Game::stateJson()const{
 return {{"board",board},{"stock",stock},{"path",path},{"moves",moves},{"clock",clock},{"course",course},{"dinerNo",dinerNo},{"diner",diner},{"menu",menu},{"sound",sound},{"mode",mode},{"lift",lift},{"selected",selected},{"tool",tool},{"category",category},{"tasteIndex",tasteIndex},{"events",events},{"report",report?json(*report):json(nullptr)},{"runDiscoveries",runDiscoveries},{"evaluation",evaluate()}};
}
static bool validBoard(const Board& board){
 auto finite=[](double v){return std::isfinite(v)&&std::abs(v)<1e8;};
 for(auto& c:board){if(c.stack.size()>8||!finite(c.temp)||!finite(c.pressure)||!finite(c.gas)||!finite(c.charge)||!finite(c.delivered)||!finite(c.waste)||!finite(c.humidity)||!finite(c.fouling)||!finite(c.energy))return false;
  if(c.tile&&(!data()["COLORS"].contains(c.tile->k)||!floorType(c.tile->k)||c.tile->dir<0||c.tile->dir>3))return false;
  if(c.source&&(!data()["SOURCE_RECIPES"].contains(c.source->key)||!finite(c.source->remaining)||!finite(c.source->rate)))return false;
  for(auto& key:soluteKeys)if(!finite(c.solution.at(key)))return false;if(!finite(c.solution.water))return false;
  for(auto& p:c.stack){if(!data()["COLORS"].contains(p.k)||floorType(p.k)||p.k=="heat"||p.k=="chill"||p.mask<0||p.mask>3||p.id<=0||p.id>=std::numeric_limits<int>::max()-1000000)return false;for(auto& key:materialKeys)if(!finite(p.at(key)))return false;for(double v:{p.temp,p.porosity,p.gel,p.foam,p.emulsion,p.crisp,p.brown,p.scorch,p.split,p.ice,p.integrity,p.pressure,p.sourced})if(!finite(v))return false;for(double v:p.aroma)if(!finite(v))return false;}
  for(double v:c.aroma)if(!finite(v))return false;
 }return true;
}
bool Game::savePersistent()const{
 if(forecasting||storageFile.empty())return true;
 try{std::error_code ec;std::filesystem::create_directories(storageFile.parent_path(),ec);if(ec)return false;auto temp=storageFile;temp+=".tmp";std::ofstream f(temp,std::ios::binary|std::ios::trunc);if(!f)return false;f<<json{{"version",1},{"notebook",notebook},{"highScores",highScores}}.dump(2);f.close();if(!f)return false;
  std::filesystem::rename(temp,storageFile,ec);
#ifdef _WIN32
  if(ec){std::filesystem::remove(storageFile,ec);ec.clear();std::filesystem::rename(temp,storageFile,ec);}
#endif
  return !ec;
 }catch(const std::exception&){return false;}
}
bool Game::loadPersistent(){
 if(storageFile.empty()||!std::filesystem::exists(storageFile))return true;
 try{if(std::filesystem::file_size(storageFile)>32*1024*1024)return false;std::ifstream f(storageFile);json j;f>>j;auto nb=j.at("notebook");if(!nb["discoveries"].is_array()||!nb["plates"].is_array()||nb["plates"].size()>12||nb["discoveries"].size()>data()["PRINCIPLES"].size())return false;
  for(auto& d:nb["discoveries"])if(!d.is_object()||!d.contains("kind")||!d["kind"].is_string()||!data()["PRINCIPLES"].contains(d["kind"].get<std::string>())||!d.contains("at")||!d["at"].is_string()||!d.contains("stack")||!d["stack"].is_string()||!d.contains("tick")||!d["tick"].is_number_integer())return false;
  Game validation;validation.notebook=nb;for(int i=0;i<int(nb["plates"].size());i++)if(!validation.loadPlate(i))return false;
  auto scores=j.value("highScores",std::map<std::string,int>{});for(auto& s:scores)if(s.second<0||s.second>100)return false;notebook=std::move(nb);highScores=std::move(scores);return true;
 }catch(const std::exception&){return false;}
}
bool Game::savePlate(){
 auto backup=notebook;if(notebook["plates"].size()>=12)notebook["plates"].erase(notebook["plates"].end()-1);
 auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
 json plate={{"course",course},{"dinerNo",dinerNo},{"board",board},{"stock",stock},{"path",path},{"moves",moves},{"clock",clock},{"menu",menu},{"sound",sound},{"score",path.size()==6?json(evaluate().final):json(nullptr)},{"number",stamp}};
 notebook["plates"].insert(notebook["plates"].begin(),plate);if(!savePersistent()){notebook=backup;announce("COULD NOT SAVE THE NOTEBOOK. CHECK THE DATA FOLDER.");return false;}announce("EXPERIMENT SAVED / "+data()["COURSES"][course]["title"].get<std::string>());return true;
}
bool Game::loadPlate(int i){
 if(i<0||i>=int(notebook["plates"].size()))return false;
 try{auto& a=notebook["plates"][i];int chapter=a.at("course");if(chapter<0||chapter>6||!a.at("board").is_array()||a.at("board").size()!=121||!(a.at("score").is_null()||a.at("score").is_number_integer()))return false;Board next=a.at("board").get<Board>();if(!validBoard(next))return false;auto route=a.at("path").get<std::vector<int>>();if(route.size()>6)return false;std::set<int> unique;
  for(size_t n=0;n<route.size();n++){int j=route[n];if(j<0||j>=121||!edible(next[j])||!unique.insert(j).second||(n&&!adjacent(route[n-1],j)))return false;}
  auto inventory=a.at("stock").get<std::map<std::string,int>>();for(auto& p:inventory)if(!data()["COLORS"].contains(p.first)||p.second<0||p.second>999999999)return false;
  for(auto& v:data()["SETS"].items())for(auto& k:v.value())if(!inventory.count(k.get<std::string>()))inventory[k.get<std::string>()]=chapter==6?999999999:3;
  int nextMoves=a.at("moves"),nextClock=a.at("clock");if(nextMoves<0||nextClock<0||nextClock>100000000)return false;std::string nextMenu=a.at("menu"),nextSound=a.at("sound");if(!data()["MENU_EXPECT"].contains(nextMenu))return false;if(nextSound!="sea"&&nextSound!="nursery"&&nextSound!="hearth"&&nextSound!="forest"&&nextSound!="silence")return false;
  course=chapter;board=std::move(next);stock=std::move(inventory);path=std::move(route);moves=nextMoves;clock=nextClock;menu=nextMenu;sound=nextSound;diner=makeDiner(course,dinerNo);selected=path.empty()?60:path[0];tool="syrup";category="food";lens="matter";mode="place";lift=-1;history.clear();report.reset();events.clear();runDiscoveries.clear();tasteIndex=-1;
  uid=1;for(auto& c:board)for(auto& p:c.stack)uid=std::max(uid,p.id+1);announce("PLATE RELOADED / NOW TASTE IT WITH "+diner.name);return true;
 }catch(const std::exception&){announce("THIS SAVED PLATE IS INVALID.");return false;}
}
} // namespace aftertaste
