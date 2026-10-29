#include "Model.h"
#include "Culinary.h"
#include "Data.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace aftertaste {
const json& data(){static const json d=[] {auto d=json::parse(embeddedData);extendPantryData(d);return d;}();return d;}
std::string coord(int i){return std::string(1,char('A'+xof(i)))+std::to_string(yof(i)+1);}
std::vector<int> neighbours(int i){std::vector<int> out;for(auto d:directions){int x=xof(i)+d[0],y=yof(i)+d[1];if(x>=0&&y>=0&&x<N&&y<N)out.push_back(idx(x,y));}return out;}
std::string upper(std::string s){for(char& c:s)c=char(std::toupper(static_cast<unsigned char>(c)));return s;}
std::string roundTo(double n,int digits){std::ostringstream s;s<<std::fixed<<std::setprecision(digits)<<n;return s.str();}
double& Material::at(const std::string& k){
 if(k=="water")return water;if(k=="sugar")return sugar;if(k=="acid")return acid;if(k=="salt")return salt;if(k=="protein")return protein;if(k=="fat")return fat;if(k=="bitter")return bitter;if(k=="activation")return activation;if(k=="sourced")return sourced;throw std::invalid_argument("Unknown compound: "+k);
}
double Material::at(const std::string& k)const{return const_cast<Material*>(this)->at(k);}
bool networkType(const std::string& k){return k=="pipe"||k=="valve"||k=="membrane"||k=="pump";}
bool floorType(const std::string& k){return networkType(k)||k=="hearth"||k=="ice"||k=="wick"||k=="vent";}
bool capType(const std::string& k){return k=="gel"||k=="foam"||k=="crisp"||k=="veil"||k=="bell"||k=="pulse";}
bool edible(const Cell& c){return std::any_of(c.stack.begin(),c.stack.end(),[](auto& p){return p.kind=="food";});}
std::vector<Piece*> foods(Cell& c){std::vector<Piece*> v;for(auto& p:c.stack)if(p.kind=="food")v.push_back(&p);return v;}
std::vector<const Piece*> foods(const Cell& c){std::vector<const Piece*> v;for(auto& p:c.stack)if(p.kind=="food")v.push_back(&p);return v;}
int faceIndex(const Cell& c){for(int i=int(c.stack.size())-1;i>=0;i--)if(c.stack[i].kind=="food")return i;return -1;}
Piece* exposed(Cell& c){int i=faceIndex(c);return i<0?nullptr:&c.stack[i];}
const Piece* exposed(const Cell& c){int i=faceIndex(c);return i<0?nullptr:&c.stack[i];}
Vec taste(const Piece& p){if(p.kind!="food")return {};return {clamp(p.sugar,0,4.8),clamp(p.acid,0,4.8),clamp(p.salt,0,4.8),clamp(p.protein*1.1+p.fat*.34,0,4.8),clamp(p.bitter+p.brown*.43+p.scorch*.57,0,4.8)};}
Profile profile(const Cell& c){
 Profile v;double weight=0;auto fs=foods(c);
 for(size_t i=0;i<c.stack.size();i++){auto& p=c.stack[i];if(p.kind!="food")continue;double w=std::pow(1.35,double(i));weight+=w;auto f=taste(p);for(int j=0;j<5;j++)v.f[j]+=f[j]*w;}
 if(weight)for(auto& f:v.f)f/=weight;
 v.face=exposed(c);int surface=faceIndex(c);
 for(int i=surface+1;i<int(c.stack.size());i++){auto& p=c.stack[i];if(!p.spent){if(p.k=="veil"&&!v.visible)v.visible=p.mask;if(p.k=="bell")v.seal=true;}}
 for(auto p:fs){for(int j=0;j<4;j++)v.a[j]+=p->aroma[j]*(p==v.face?.7:.35);
  v.chemistry|=p->brown>.23||p->gel>.65||p->foam>.5||p->emulsion>.55||p->ice>.6||p->split>.45||p->scorch>.4;
  v.roast+=p->brown;v.scorch+=p->scorch;v.split+=p->split;}
 for(int j=0;j<4;j++)v.a[j]+=c.aroma[j]*.8;
 v.temp=v.face?v.face->temp*.66+c.temp*.34:c.temp;v.water=v.face?v.face->water:0;v.phase=v.face?v.face->phase:"empty";v.integrity=v.face?v.face->integrity:0;
 v.layers=int(fs.size());v.has=!fs.empty();double divisor=std::max(1,v.layers);v.roast/=divisor;v.scorch/=divisor;v.split/=divisor;
 if(fs.size()>=2){auto first=taste(*fs.front()),last=taste(*fs.back());double sum=0;for(int j=0;j<5;j++)sum+=std::pow(first[j]-last[j],2);v.reveal=clamp(std::sqrt(sum)/4.2,0,1);}
 return v;
}
struct RNG {uint32_t t;double operator()(){t+=0x6D2B79F5u;uint32_t z=t;z=(z^(z>>15))*(z|1);z^=z+(z^(z>>7))*(z|61);return double(z^(z>>14))/4294967296.;}};
Diner makeDiner(int chapter,int number){
 RNG r{uint32_t(19091u+uint32_t(chapter)*761u+uint32_t(number)*105997u)};Diner d;
 d.like=int(r()*5);d.avoid=(d.like+1+int(r()*4))%5;d.contract=int(r()*data()["CONTRACTS"].size());
 const std::array<std::string,6> tex={"gel","crisp","foam","liquid","pearls","crystal"};d.texture=tex[int(r()*6)];d.novelty=.85+r()*1.55;
 std::ostringstream suffix;suffix<<std::setw(2)<<std::setfill('0')<<number+1;
 d.name=data()["BASENAMES"][int(r()*data()["BASENAMES"].size())].get<std::string>()+" "+suffix.str();d.n=number;
 for(auto& s:d.sensitivity)s=.76+r()*.62;d.temperature=r()*1.6-.8;
 d.novelLabel=d.novelty>1.6?"THRILL SEEKER":d.novelty<1.14?"CAUTIOUS":"EXPLORER";
 d.color=std::array<std::string,5>{"#ea3a83","#7352ee","#05a5c9","#f59143","#29b48d"}[d.like];d.memory=.54+r()*.28;return d;
}
std::string palateLabel(const Diner& d){return d.novelLabel+" / +"+flavourNames[d.like]+" / -"+flavourNames[d.avoid];}
Game::Game(){reset(0,false);}
int Game::budget()const{return course==6?999999999:std::array<int,6>{37,36,38,37,38,37}[course];}
Piece Game::makePiece(const std::string& k){
 Piece p;p.id=uid++;p.k=k;
 if(data()["FOOD"].contains(k)){auto& r=data()["FOOD"][k];for(auto& key:materialKeys)p.at(key)=r.value(key,0.);p.porosity=r["porosity"];p.aroma=data()["recipes"][k]["a"].get<Aroma>();}
 else{p.kind=(k=="bell"||k=="pulse")?"machine":"method";p.porosity=k=="gel"?.12:k=="bell"?.025:k=="crisp"?.27:.80;p.phase="apparatus";}
 return p;
}
void Game::seed(int i,const std::string& k){auto p=makePiece(k);p.temp=board[i].temp;board[i].stack.push_back(p);}
void Game::reset(int n,bool keepDiner){
 course=(n%7+7)%7;if(!keepDiner)dinerNo=0;diner=makeDiner(course,dinerNo);board={};runDiscoveries.clear();
 stock={{"syrup",5},{"brine",5},{"broth",5},{"citrus",5},{"cream",5},{"cocoa",4},{"heat",6},{"chill",6},{"gel",5},{"foam",5},{"crisp",5},{"veil",4},{"hearth",2},{"ice",2},{"wick",2},{"vent",2},{"bell",3},{"pulse",3},{"pipe",8},{"valve",3},{"membrane",3},{"pump",3}};
 if(course==6)for(auto& s:stock)s.second=999999999;
 tool="syrup";category="food";lens="matter";mode="place";lift=-1;selected=60;moves=clock=0;report.reset();events.clear();history.clear();path.clear();uid=1;menu=data()["COURSES"][course]["label"];sound=data()["COURSES"][course]["sound"];
 for(int a=0;a<6;a++){int j=idx(a+2,5);seed(j,data()["COURSE_LINES"][course][a]);auto& p=board[j].stack.back();p.temp=data()["COURSES"][course]["targets"][a]["t"].get<double>()*.35;board[j].temp=p.temp*.7;path.push_back(j);}
 const std::array<std::tuple<int,int,std::string>,12> extras={{{2,4,"brine"},{3,4,"citrus"},{4,4,"cream"},{5,4,"cocoa"},{6,4,"broth"},{7,4,"syrup"},{4,6,"broth"},{5,6,"cocoa"},{6,6,"cream"},{7,6,"citrus"},{8,6,"syrup"},{3,7,"brine"}}};
 for(auto [x,y,k]:extras)seed(idx(x,y),course==3&&k=="brine"?"cocoa":course==4&&k=="brine"?"cream":k);
 seed(idx(2,4),"gel");seed(idx(4,4),"broth");seed(idx(4,4),"foam");seed(idx(5,6),"crisp");
 board[idx(2,7)].tile=Tile{"ice",0};board[idx(8,4)].tile=Tile{"hearth",0};board[idx(4,7)].tile=Tile{"wick",0};board[idx(8,7)].tile=Tile{"vent",3};
 const std::array<std::string,9> network={"pipe","pipe","pipe","valve","pipe","pump","pipe","pipe","pipe"};
 for(int x=1;x<=9;x++){auto& c=board[idx(x,5)];c.tile=Tile{network[x-1],0};if(auto f=exposed(c)){double amt=std::min(.14,f->water*.18);f->water-=amt;c.solution.water=amt;for(auto& k:soluteKeys){if(k=="activation"||k=="sourced")continue;double v=f->at(k)*.045;f->at(k)-=v;c.solution.at(k)=v;}}}
 for(int a=0;a<2;a++){int x=a==0?1:9;auto& c=board[idx(x,5)];c.source=Source{data()["COMMISSIONS"][course]["sources"][a],course==6?999.:7.6,.28};c.tile=Tile{"pipe",0};}
 for(int x=2;x<=8;x++)board[idx(x,5)].solution.water+=.015;
 if(course==1)board[idx(4,5)].tile->open=false;
 if(course==2)board[idx(6,5)].tile->dir=2;
 if(course==3)board[idx(6,5)].tile=Tile{"membrane",0};
 if(course==4){board[idx(5,5)].charge=.35;board[idx(5,5)].gas=.19;seed(idx(5,5),"bell");}
 if(course==5){board[idx(4,5)].tile->open=false;board[idx(6,5)].tile->dir=2;stock["heat"]=stock["chill"]=3;stock["pipe"]=4;stock["pump"]=1;}
 tasteIndex=-1;announce("BUILD THE NETWORK - COMMISSIONS REQUIRE REAL RESERVOIR FEED.");
}
void Game::snapshot(){history.push_back(static_cast<const State&>(*this));if(history.size()>85)history.pop_front();}
void Game::undo(){
 if(mode=="tasting"){mode="place";tasteIndex=-1;report.reset();announce("SERVICE CANCELLED - RETURN TO THE REACTOR.");return;}
 if(history.empty()){announce("NOTHING TO UNDO");return;}
 auto discoveries=runDiscoveries;int currentUid=uid;auto view=lens;static_cast<State&>(*this)=history.back();history.pop_back();uid=currentUid;lens=view;runDiscoveries=discoveries;tasteIndex=-1;announce("TURN REWOUND - matter, machinery and palate setup restored.");
}
void Game::chooseCat(const std::string& cat){category=cat;tool=data()["SETS"][cat][0];mode="place";lift=-1;announce(upper(cat)+" TRAY / "+data()["HELP"][tool].get<std::string>());}
void Game::chooseTool(const std::string& k){for(auto it=data()["SETS"].begin();it!=data()["SETS"].end();++it)for(auto& v:it.value())if(v==k)category=it.key();tool=k;mode="place";lift=-1;announce(upper(data()["TNAME"][k])+" / "+data()["HELP"][k].get<std::string>());}
void Game::moveMode(){if(mode=="tasting")return;mode=mode=="move"?"place":"move";lift=-1;report.reset();announce(mode=="move"?"LIFT TOP PIECE - click destination, or the same stack to rotate.":"BUILD MODE");}
void Game::routeMode(){if(mode=="tasting")return;mode=mode=="route"?"place":"route";lift=-1;report.reset();if(mode=="route"){path.clear();announce("CHOOSE SIX CONNECTED EDIBLE SQUARES IN ORDER.");}else announce("BUILD MODE - current route kept.");}
void Game::trace(int i){
 if(!edible(board[i])){announce("ROUTE REQUIRES FOOD.");return;}
 if(!path.empty()&&path.back()==i){path.pop_back();announce("LAST BITE REMOVED.");return;}
 if(std::find(path.begin(),path.end(),i)!=path.end()){announce("A SQUARE MAY BE TASTED ONCE.");return;}
 if(!path.empty()&&!adjacent(path.back(),i)){announce("THE NEXT BITE MUST SHARE AN EDGE.");return;}
 if(path.size()==6){announce("SIX BITES ALREADY SELECTED.");return;}
 path.push_back(i);if(path.size()==6){mode="place";announce("SIX-BITE SEQUENCE SET. SERVE OR KEEP EXPERIMENTING.");}else announce("BITE "+std::to_string(path.size())+" / 6 SELECTED.");
}
void Game::moveTop(int a,int b){
 if(a<0||a>=121||b<0||b>=121)return;
 if(mode=="tasting"){announce("FINISH THE TASTING FIRST.");return;}
 if(moves>=budget()){announce("NO PREPARATION TURNS REMAIN.");return;}
 if(board[a].stack.empty()){announce("NO PIECE TO LIFT.");return;}
 if(a==b&&board[a].stack.size()<2){announce("TWO PIECES REQUIRED TO ROTATE.");return;}
 if(a!=b&&board[b].stack.size()>=8){announce("THIS STACK IS EIGHT PIECES HIGH.");return;}
 auto chip=board[a].stack.back();
 if(a!=b&&chip.kind!="food"&&!edible(board[b])){announce("CAPS REQUIRE AN INGREDIENT BENEATH THEM.");return;}
 if(a!=b&&chip.kind=="food"&&foods(board[a]).size()==1&&board[a].stack.size()>1){announce("LIFT THE LIDS BEFORE REMOVING THEIR LAST INGREDIENT.");return;}
 snapshot();board[a].stack.pop_back();if(a==b)board[a].stack.insert(board[a].stack.begin(),chip);else board[b].stack.push_back(chip);
 for(int j:path)if(!edible(board[j])){path.clear();break;}lift=-1;perform(a==b?"ROTATED "+coord(a)+" / NEW TOP LAYER":"MOVED "+upper(data()["TNAME"][chip.k])+" "+coord(a)+" > "+coord(b));
}
void Game::manipulate(int i){if(lift<0){if(board[i].stack.empty()){announce("CHOOSE A STACK WITH A TOP PIECE.");return;}lift=i;selected=i;announce("LIFTED "+coord(i)+" / CHOOSE DESTINATION.");return;}if(i==lift&&board[i].stack.size()==1){lift=-1;announce("LIFT CANCELLED.");return;}moveTop(lift,i);}
bool Game::place(int i){
 if(moves>=budget()){announce("PREPARATION IS OVER. DRAW A ROUTE AND SERVE.");return false;}auto& c=board[i];auto k=tool;
 if(floorType(k)){
  if(c.source&&(!c.tile||c.tile->k!=k)){announce("FIXED RESERVOIR PORT - CONNECT WITHOUT REPLACING IT.");return false;}
  if(c.tile&&c.tile->k==k){if(k=="valve"){snapshot();c.tile->open=!c.tile->open;perform("VALVE "+std::string(c.tile->open?"OPEN":"CLOSED")+" / "+coord(i));return true;}if(k=="vent"||k=="pump"){snapshot();c.tile->dir=(c.tile->dir+1)%4;perform(upper(data()["TNAME"][k])+" ROTATED");return true;}announce("THIS BOARD TILE IS ALREADY HERE.");return false;}
  if(!stock[k]){announce("NO "+upper(data()["TNAME"][k])+" LEFT IN THE TRAY.");return false;}snapshot();stock[k]--;c.tile=Tile{k,0};perform("BOARD TILE "+upper(data()["TNAME"][k])+" AT "+coord(i));return true;
 }
 if(!stock[k]){announce("NO "+upper(data()["TNAME"][k])+" LEFT IN THE TRAY.");return false;}
 if(k=="heat"||k=="chill"){snapshot();probeApply(i,k);stock[k]--;perform(upper(k)+" BURST AT "+coord(i));return true;}
 if(capType(k)&&!edible(c)){announce("STACK A CAP ABOVE FOOD.");return false;}if(c.stack.size()>=8){announce("MAXIMUM EIGHT PIECES IN A STACK.");return false;}
 snapshot();stock[k]--;if(k=="veil"&&!c.stack.empty()&&c.stack.back().k=="veil"){c.stack.back().mask=c.stack.back().mask%3+1;perform("DISGUISE SHIFTED AT "+coord(i));return true;}
 auto p=makePiece(k);p.temp=c.stack.empty()?c.temp:c.stack.back().temp;if(k=="veil")p.mask=1;c.stack.push_back(p);perform(upper(data()["TNAME"][k])+" STACKED AT "+coord(i));return true;
}
void Game::apply(int i){if(i<0||i>=121)return;if(mode=="tasting"){announce("SERVICE IN PROGRESS - COMPLETE THE SIX BITES.");return;}selected=i;if(mode=="route")trace(i);else if(mode=="move")manipulate(i);else place(i);}
void Game::wait(){if(mode=="tasting"){announce("FINISH THE TASTING FIRST.");return;}if(moves>=budget()){announce("THE KITCHEN IS CLOSED.");return;}snapshot();perform("TIME PASSED - MATERIALS STILL RESPOND");}
void Game::changeSetting(const std::string& which,const std::string& value){if(moves>=budget()){announce("SETTINGS MUST CHANGE BEFORE THE LAST TURN.");return;}auto& setting=which=="menu"?menu:sound;if(setting==value)return;snapshot();setting=value;perform(which=="menu"?"THE NAME CHANGED; EXPECTATIONS SHIFT":"NEW SOUNDSCAPE; EXPECTATIONS SHIFT");}
void Game::changeDiner(){dinerNo++;reset(course,true);announce("NEW DINER: "+diner.name+" / "+palateLabel(diner));}
void Game::perform(const std::string& msg){simulate();moves++;report.reset();announce((events.empty()?"":events[0]+" / ")+msg+" / "+(course==6?"FREE LAB":std::to_string(budget()-moves)+" turns"));}
void Game::flag(int i,const std::string& msg){if(events.size()<32)events.push_back(coord(i)+" · "+msg);board[i].last=msg;if(!forecasting)discovered(msg,i);}
void Game::discovered(const std::string& kind,int i){if(forecasting||!data()["PRINCIPLES"].contains(kind))return;runDiscoveries.insert(kind);for(auto& d:notebook["discoveries"])if(d["kind"]==kind)return;std::string stack;for(auto& p:board[i].stack){if(!stack.empty())stack+=" > ";stack+=data()["TNAME"][p.k].get<std::string>();}notebook["discoveries"].insert(notebook["discoveries"].begin(),json{{"kind",kind},{"at",coord(i)},{"course",course+1},{"stack",stack},{"tick",clock}});savePersistent();}
std::vector<int> Game::networkTrace(int i)const{if(!board[i].tile||!networkType(board[i].tile->k))return {};std::vector<int> found{i};for(size_t p=0;p<found.size();p++)for(int j:neighbours(found[p]))if(connected(found[p],j)&&std::find(found.begin(),found.end(),j)==found.end())found.push_back(j);return found;}
std::string Game::actionPreview(int i)const{
 auto& c=board[i];auto pos=coord(i);int n=int(c.stack.size());
 if(mode=="route")return pos+" / "+(!edible(c)?"NO FOOD TO TASTE":std::find(path.begin(),path.end(),i)!=path.end()?"BITE ALREADY ON ROUTE":!path.empty()&&!adjacent(path.back(),i)?"NEXT BITE MUST TOUCH ROUTE":"SELECT BITE "+std::to_string(path.size()+1)+"/6");
 if(mode=="move")return pos+" / "+(lift<0?(n?"LIFT "+data()["CARTRIDGE_IDS"][c.stack.back().k].get<std::string>():"EMPTY STACK"):lift==i?"ROTATE STACK":"MOVE TOP PIECE HERE");
 if(mode=="tasting")return pos+" / TASTING IN PROGRESS";
 if(moves>=budget())return pos+" / NO TURNS LEFT - ROUTE AND SERVE";
 if(floorType(tool))return pos+" / "+(c.tile&&c.tile->k==tool?(tool=="valve"?"TOGGLE VALVE "+std::string(c.tile->open?"CLOSED":"OPEN"):(tool=="vent"||tool=="pump")?"ROTATE "+upper(data()["TNAME"][tool]):"ALREADY INSTALLED"):"INSTALL "+upper(data()["TNAME"][tool])+" IN FLOOR");
 if(tool=="heat"||tool=="chill")return pos+" / "+upper(tool)+" - AFFECTS 3x3 CHAMBERS";
 if(capType(tool)&&!edible(c))return pos+" / NEEDS FOOD BELOW";if(n>=8)return pos+" / STACK FULL (8/8)";
 return pos+" / ADD "+data()["CARTRIDGE_IDS"][tool].get<std::string>()+" "+upper(data()["TNAME"][tool])+" / "+std::to_string(n)+">"+std::to_string(n+1)+" LAYERS";
}
std::string Game::squareTitle(int i)const{auto& c=board[i];auto p=profile(c);std::string s=coord(i)+" / BASE > TOP: ";for(auto& z:c.stack)s+=upper(data()["TNAME"][z.k])+" ("+roundTo(z.temp)+", "+z.phase+") > ";s+="\n";for(int j=0;j<5;j++)s+=flavourNames[j]+" "+roundTo(p.f[j])+" / ";return s+"\nTEMP "+roundTo(p.temp)+" / WET "+roundTo(p.water)+" / PRESS "+roundTo(c.pressure)+" / ACTIVE "+roundTo(c.charge)+"\nSOURCED "+roundTo(c.delivered,2)+" / WASTE "+roundTo(c.waste,2)+" / "+c.last;}
} // namespace aftertaste
