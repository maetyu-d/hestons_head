#include "Model.h"
#include <algorithm>

namespace aftertaste {
bool Game::probeApply(int i,const std::string& k){
 if(mode!="place"||moves>=budget())return false;auto& c=board[i];
 if((!stock.count(k)||stock.at(k)==0)&&!(floorType(k)&&c.tile&&c.tile->k==k&&(k=="valve"||k=="pump"||k=="vent")))return false;
 if(floorType(k)){
  if(c.source&&(!c.tile||c.tile->k!=k))return false;
  if(c.tile&&c.tile->k==k){if(k=="valve"){c.tile->open=!c.tile->open;return true;}if(k=="vent"||k=="pump"){c.tile->dir=(c.tile->dir+1)%4;return true;}return false;}
  c.tile=Tile{k,0};return true;
 }
 if(k=="heat"||k=="chill"){
  int sign=k=="heat"?1:-1;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){int x=xof(i)+dx,y=yof(i)+dy;if(x<0||y<0||x>=N||y>=N)continue;auto& cc=board[idx(x,y)];double strength=dx==0&&dy==0?2.65:std::abs(dx)+std::abs(dy)==1?1.45:.78;cc.temp=clamp(cc.temp+strength*sign,-3.6,3.6);cc.energy+=strength*.16;for(auto& p:cc.stack)p.temp=clamp(p.temp+strength*sign*(p.kind=="food"?.8:.67),-3.6,3.6);}return true;
 }
 if(capType(k)&&!edible(c))return false;if(c.stack.size()>=8)return false;
 if(k=="veil"&&!c.stack.empty()&&c.stack.back().k=="veil"){c.stack.back().mask=c.stack.back().mask%3+1;return true;}
 auto p=makePiece(k);p.temp=c.stack.empty()?c.temp:c.stack.back().temp;if(k=="veil")p.mask=1;c.stack.push_back(p);return true;
}
static bool reactiveEvent(const std::string& e){for(auto key:{"ACTIVATION","REACTION","QUENCH","AERATION","RELEASE","BURST"})if(e.find(key)!=std::string::npos)return true;return false;}
std::vector<ForecastTurn> Game::probeState(int i,bool action,bool perturb,const std::string& k)const{
 Game replica;static_cast<State&>(replica)=static_cast<const State&>(*this);replica.forecasting=true;replica.events.clear();
 if(action&&!replica.probeApply(i,k))return {};
 if(perturb){auto near=neighbours(i);near.insert(near.begin(),i);for(int j:near){auto& c=replica.board[j];c.temp=clamp(c.temp+(j==i?.11:-.035),-3.6,3.6);c.pressure=clamp(c.pressure+(j==i?.09:.025),0,4);if(auto f=exposed(c)){f->temp=clamp(f->temp+(j==i?.11:-.035),-3.6,3.6);f->water=clamp(f->water-(j==i?.045:.012),0,2);}}}
 std::vector<ForecastTurn> turns;
 for(int n=1;n<=3;n++){replica.simulate();auto& c=replica.board[i];auto p=profile(c);ForecastTurn t;t.at=n;t.temp=p.temp;t.pressure=c.pressure;t.charge=c.charge;t.water=p.water;t.phase=p.phase;for(auto& e:replica.events)if(reactiveEvent(e))t.react.push_back(e);t.all=int(replica.events.size());for(auto& v:replica.board)if(v.wave>.35)t.pulses++;t.flow=c.flux;if(replica.path.size()==6)t.score=replica.evaluate().final;t.board=replica.board;turns.push_back(std::move(t));}
 return turns;
}
std::optional<Forecast> Game::forecastAction(int i,bool compare)const{
 if(mode!="place"||moves>=budget()||i<0||i>=121)return {};
 auto base=probeState(i,false,false,tool),acting=probeState(i,true,false,tool);if(acting.empty())return {};
 auto sensitive=probeState(i,true,true,tool);Forecast f;f.i=i;f.tool=tool;f.risk="STABLE";
 for(int n=0;n<3;n++){
  auto t=acting[n];auto& b=base[n];auto& alt=sensitive[n];t.delta=t.temp-b.temp;t.dPressure=t.pressure-b.pressure;t.pressureSpan=std::abs(t.pressure-alt.pressure);t.tempSpan=std::abs(t.temp-alt.temp);
  for(auto& e:t.react)if(std::find(b.react.begin(),b.react.end(),e)==b.react.end())t.newRx++;
  t.flowDelta=t.flow-b.flow;t.changed=t.react!=alt.react||t.phase!=alt.phase;if(t.changed||t.tempSpan>.30||t.pressureSpan>.30)f.risk="SENSITIVE";
  f.chains.insert(f.chains.end(),t.react.begin(),t.react.end());f.turns.push_back(std::move(t));
  for(int j=0;j<121;j++)if(acting[n].board[j].wave-base[n].board[j].wave>.38&&std::find(f.chainTargets.begin(),f.chainTargets.end(),j)==f.chainTargets.end())f.chainTargets.push_back(j);
 }
 auto& last=acting[2].board;auto& b=base[2].board;
 for(int j=0;j<121;j++){if(std::abs(last[j].temp-b[j].temp)>.10||std::abs(last[j].charge-b[j].charge)>.09||std::abs(last[j].pressure-b[j].pressure)>.11||std::abs(last[j].solution.water-b[j].solution.water)>.035||last[j].wave-b[j].wave>.38)f.impact.push_back(j);f.deltas.push_back({j,last[j].temp-b[j].temp,last[j].wave-b[j].wave,last[j].delivered-b[j].delivered});}
 if(compare){auto alt=tool=="heat"?"chill":"heat";auto second=probeState(i,true,false,alt);auto rx=[](const auto& ts){int n=0;for(auto& t:ts)n+=int(t.react.size());return n;};f.variants={{upper(data()["TNAME"][tool]),tool,true,acting[2].score,rx(acting),acting[2].pressure},{upper(data()["TNAME"][alt]),alt,!second.empty(),second.empty()?std::optional<int>{}:second[2].score,rx(second),second.empty()?0:second[2].pressure},{"WAIT","wait",true,base[2].score,rx(base),base[2].pressure}};}
 return f;
}
} // namespace aftertaste
