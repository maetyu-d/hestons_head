#include "Model.h"
#include <algorithm>
#include <numeric>

namespace aftertaste {
namespace {
bool hasAbove(const Cell& c,int z,const std::string& k){for(int t=z+1;t<int(c.stack.size());t++)if(c.stack[t].k==k&&!c.stack[t].spent)return true;return false;}
double sumA(const Aroma& a){return std::accumulate(a.begin(),a.end(),0.);}
}
Board Game::diffusionPass(){
 const Board old=board;Board next=old;
 for(int i=0;i<121;i++){
  const auto& a=old[i];auto& b=next[i];auto near=neighbours(i);double heat=0;
  if(a.tile&&a.tile->k=="hearth")heat+=.52;if(a.tile&&a.tile->k=="ice")heat-=.53;
  double tempSum=0,humiditySum=0;Aroma aromaSum{};
  for(int j:near){if(old[j].tile&&old[j].tile->k=="hearth")heat+=.105;if(old[j].tile&&old[j].tile->k=="ice")heat-=.105;tempSum+=old[j].temp;humiditySum+=old[j].humidity;for(int k=0;k<4;k++)aromaSum[k]+=old[j].aroma[k];}
  b.temp=clamp(a.temp*.85+tempSum*.036+heat,-3.6,3.6);b.humidity=clamp(a.humidity*.83+humiditySum*.036,0,2.5);
  for(int k=0;k<4;k++)b.aroma[k]=clamp(a.aroma[k]*.76+aromaSum[k]*.046,0,4.5);
  b.burst=std::max(0.,a.burst-.26);b.wave=std::max(0.,a.wave-.3);b.flux=a.flux*.38;b.flowAmount=0;b.last.clear();
  b.pressure=clamp(a.pressure*.78+a.gas*.24+std::max(0.,a.temp)*(.018+.026*a.humidity),0,4);b.gas=clamp(a.gas*.86,0,5);b.charge=clamp(a.charge*.87,0,4);
  for(int z=0;z<int(a.stack.size());z++){
   auto& p=a.stack[z];auto& q=b.stack[z];q.age=p.age+1;q.temp=clamp(p.temp+(a.temp-p.temp)*(.25/(1+.11*z)),-3.6,3.6);if(p.kind!="food")continue;
   bool cover=hasAbove(a,z,"bell")||hasAbove(a,z,"gel"),exposure=z==faceIndex(a)&&!cover;
   if(exposure){double evaporate=clamp(.012+std::max(0.,p.temp)*.014,0,.07)*clamp(p.water*.8,0,1);q.water=clamp(q.water-evaporate,0,2);q.sourced=clamp(q.sourced-evaporate*clamp(p.sourced/std::max(p.water,.01),0,1),0,q.water);b.humidity=clamp(b.humidity+evaporate*.42,0,2.5);
    double vol=(.045+std::max(0.,p.temp)*.021)*(p.phase=="foam"?2.0:p.phase=="gel"?.40:1);for(int j=0;j<4;j++){double d=std::min(p.aroma[j],vol*p.aroma[j]);q.aroma[j]-=d;b.aroma[j]=clamp(b.aroma[j]+d,0,5);}
   }else if(cover){for(int t=z+1;t<int(a.stack.size());t++)if(a.stack[t].k=="bell"&&!a.stack[t].spent){auto& lid=b.stack[t];lid.pressure=clamp(lid.pressure+.09+.07*std::max(0.,p.temp)+sumA(p.aroma)*.036,0,6);break;}}
  }
  for(int z=0;z<int(a.stack.size())-1;z++){auto& p=a.stack[z];auto& r=a.stack[z+1];double heatRate=.16*(.55+.45*std::min(p.porosity?p.porosity:.8,r.porosity?r.porosity:.8));double flow=(p.temp-r.temp)*heatRate;b.stack[z].temp-=flow;b.stack[z+1].temp+=flow;}
  std::vector<int> positions;for(int z=0;z<int(a.stack.size());z++)if(a.stack[z].kind=="food")positions.push_back(z);
  for(int v=0;v<int(positions.size())-1;v++){
   int z=positions[v],z2=positions[v+1];auto& p=a.stack[z];auto& r=a.stack[z2];auto& q=b.stack[z];auto& s=b.stack[z2];double permeability=std::min(p.porosity,r.porosity);
   for(int t=z+1;t<z2;t++)permeability*=a.stack[t].spent?1:a.stack[t].porosity;
   double m=(p.water-r.water)*.060*permeability-.006*permeability;m=clamp(m,-r.water*.19,p.water*.19);q.water-=m;s.water+=m;
   for(auto& k:soluteKeys){if(k=="activation"||k=="sourced")continue;double mobility=k=="fat"?.16:k=="protein"?.25:k=="bitter"?.62:1;double dv=(p.at(k)-r.at(k))*.045*permeability*mobility*clamp((p.water+r.water)/1.6,.1,1);q.at(k)-=dv;s.at(k)+=dv;}
   for(int k=0;k<4;k++){double d=(p.aroma[k]-r.aroma[k])*.065*permeability;q.aroma[k]-=d;s.aroma[k]+=d;}
  }
 }
 for(int i=0;i<121;i++)for(int j:neighbours(i))if(j>i){
  const auto& a=old[i];const auto& b=old[j];int ix=faceIndex(a),jx=faceIndex(b);if(ix<0||jx<0)continue;
  auto& p=a.stack[ix];auto& r=b.stack[jx];auto& q=next[i].stack[ix];auto& s=next[j].stack[jx];
  if(p.water<.45||r.water<.45||hasAbove(a,ix,"bell")||hasAbove(b,jx,"bell"))continue;
  double d=(p.water-r.water)*.008;q.water-=d;s.water+=d;
  for(auto k:{"acid","salt","sugar"}){double t=(p.at(k)-r.at(k))*.007;q.at(k)-=t;s.at(k)+=t;}
 }
 for(int i=0;i<121;i++){
  auto& a=old[i];auto& b=next[i];if(!a.tile)continue;
  if(a.tile->k=="wick"){
   for(int j:neighbours(i)){int ix=faceIndex(old[j]);if(ix<0)continue;auto& p=old[j].stack[ix];auto& q=next[j].stack[ix];double taken=std::min(p.water*.065,.085);q.water=clamp(q.water-taken,0,2);b.humidity=clamp(b.humidity+taken,0,2.5);}
   for(auto& bottom:b.stack)if(bottom.kind=="food"){double t=std::min(.095,b.humidity*.16);bottom.water=clamp(bottom.water+t,0,2);b.humidity=std::max(0.,b.humidity-t);break;}
  }
  if(a.tile->k=="vent"){auto d=directions[a.tile->dir];int x=xof(i)+d[0],y=yof(i)+d[1];if(x>=0&&y>=0&&x<N&&y<N){auto& dst=next[idx(x,y)];for(int k=0;k<4;k++){double amount=std::min(b.aroma[k]*.40,.44);b.aroma[k]-=amount;dst.aroma[k]=clamp(dst.aroma[k]+amount,0,5);}}}
 }
 networkPass(old,next);return next;
}
bool Game::connected(int i,int j)const{return connected(i,j,board);}
bool Game::connected(int i,int j,const Board& state){auto& a=state[i].tile;auto& b=state[j].tile;return a&&b&&networkType(a->k)&&networkType(b->k)&&!(a->k=="valve"&&!a->open)&&!(b->k=="valve"&&!b->open);}
double Game::pumpHead(int i,int j,const Board& old){
 int dx=xof(j)-xof(i),dy=yof(j)-yof(i),d=-1;for(int k=0;k<4;k++)if(directions[k][0]==dx&&directions[k][1]==dy)d=k;
 auto& a=old[i].tile;auto& b=old[j].tile;double force=0;if(a&&a->k=="pump"){if(a->dir==d)force+=1.3;else if((a->dir+2)%4==d)force-=.70;}if(b&&b->k=="pump"){if(b->dir==(d+2)%4)force-=1.3;else if(b->dir==d)force+=.70;}return force;
}
void Game::networkPass(const Board& old,Board& next){
 for(int i=0;i<121;i++){
  auto& c=next[i];auto& ref=old[i];if(!ref.tile||!networkType(ref.tile->k))continue;if(ref.tile->k=="pump")c.energy+=.07;
  if(ref.source&&ref.source->remaining>.00001){auto& spec=data()["SOURCE_RECIPES"][ref.source->key];double available=std::min({ref.source->rate,ref.source->remaining,std::max(0.,1.25-c.solution.water)});
   if(available>.00001){c.source->remaining-=available;c.solution.water+=available;c.solution.sourced+=available;for(auto k:{"sugar","acid","salt","protein","fat"})c.solution.at(k)+=available*spec.value(k,0.);int ar=spec["aroma"];c.aroma[ar]=clamp(c.aroma[ar]+available*.25,0,5);c.pressure=clamp(c.pressure+available*.19,0,4);}}
  if(ref.tile->k=="valve"&&!ref.tile->open)continue;int ix=faceIndex(ref);
  if(ix>=0){auto& f=ref.stack[ix];auto& q=c.stack[ix];auto& pool=c.solution;double take=std::min(.115,f.water*.125),traced=std::min(f.sourced,take*clamp(f.sourced/std::max(.05,f.water),0,1));q.water=std::max(0.,q.water-take);q.sourced=std::max(0.,q.sourced-traced);pool.water+=take;pool.sourced+=traced;
   for(auto& key:soluteKeys){if(key=="activation"||key=="sourced")continue;double m=std::max(0.,f.at(key)*.052*clamp(f.water,.2,1.5));q.at(key)-=m;pool.at(key)+=m;}}
 }
 const Board base=next;
 for(int i=0;i<121;i++)for(int j:neighbours(i))if(j>i&&connected(i,j,base)){
  auto& a=base[i];auto& b=base[j];auto& c=next[i];auto& d=next[j];bool membrane=a.tile->k=="membrane"||b.tile->k=="membrane";double gate=membrane?.40:1,head=pumpHead(i,j,base),dp=(a.pressure-b.pressure)*.19*gate;
  c.pressure=clamp(c.pressure-dp,0,4);d.pressure=clamp(d.pressure+dp,0,4);
  double flux=((a.solution.water-b.solution.water)*.25+(a.pressure-b.pressure)*.085+head*.10)*clamp(1-(a.fouling+b.fouling)*.22,.25,1);
  double amount=flux>0?std::min(flux,a.solution.water*.245):-std::min(-flux,b.solution.water*.245);
  auto& from=amount>0?a:b;auto& into=amount>0?d:c;auto& out=amount>0?c:d;
  if(std::abs(amount)>.000001){
   double fraction=std::abs(amount)/std::max(.0001,from.solution.water);out.solution.water=std::max(0.,out.solution.water-std::abs(amount));into.solution.water+=std::abs(amount);
   for(auto& key:soluteKeys){double sieve=membrane?(key=="salt"||key=="acid"?.95:key=="activation"?.14:key=="sugar"?.27:.08):1;double moved=from.solution.at(key)*fraction*sieve;out.solution.at(key)-=moved;into.solution.at(key)+=moved;}
   out.flux+=std::abs(amount);into.flux+=std::abs(amount);int fromi=amount>0?i:j,toi=amount>0?j:i,direction=0;
   for(int k=0;k<4;k++)if(directions[k][0]==xof(toi)-xof(fromi)&&directions[k][1]==yof(toi)-yof(fromi))direction=k;
   into.flowAmount+=std::abs(amount);out.flowAmount+=std::abs(amount);into.flowDir=out.flowDir=direction;
   std::string major="acid";for(auto key:{"salt","sugar","protein","fat"})if(from.solution.at(key)>from.solution.at(major))major=key;
   const std::map<std::string,std::string> hues={{"acid","#d6ed00"},{"salt","#00b6f2"},{"sugar","#ff4d28"},{"protein","#7046f5"},{"fat","#f9c300"}};into.flowHue=out.flowHue=hues.at(major);
   for(auto node:{&out,&into})if(node->tile&&node->tile->k=="membrane")node->fouling=clamp(node->fouling+std::abs(amount)*(.12+.7*(from.solution.protein+from.solution.fat)),0,3);
   if(std::abs(amount)>.10){into.waste+=std::max(0.,std::abs(amount)-.10)*.10;for(auto& ar:into.aroma)ar=std::max(0.,ar-.009);}
  }
  double reactive=(a.charge-b.charge)*.33*gate*(membrane?.27:1);c.charge=clamp(c.charge-reactive,0,5);d.charge=clamp(d.charge+reactive,0,5);
  double heat=(a.temp-b.temp)*.065*gate;c.temp-=heat;d.temp+=heat;double gas=(a.gas-b.gas)*.10*gate;c.gas=clamp(c.gas-gas,0,5);d.gas=clamp(d.gas+gas,0,5);
  for(int n=0;n<4;n++){double flow=(a.aroma[n]-b.aroma[n])*.13*gate;c.aroma[n]=clamp(c.aroma[n]-flow,0,5);d.aroma[n]=clamp(d.aroma[n]+flow,0,5);}
 }
 for(auto& c:next){
  if(!c.tile||!networkType(c.tile->k)||(c.tile->k=="valve"&&!c.tile->open))continue;int ix=faceIndex(c);if(ix<0)continue;
  auto& f=c.stack[ix];auto& pool=c.solution;double intake=std::min({pool.water*.35,std::max(0.,1.65-f.water)*.30,.13}),frac=pool.water>.00001?intake/pool.water:0;pool.water-=intake;f.water=clamp(f.water+intake,0,2);
  for(auto& key:soluteKeys){double amount=pool.at(key)*frac;pool.at(key)-=amount;if(key=="activation")c.charge=clamp(c.charge+amount*.9,0,5);else if(key=="sourced"){f.sourced+=amount;if(!c.source)c.delivered+=amount;}else f.at(key)=clamp(f.at(key)+amount,0,4.5);}
  if(c.tile->k=="pump")c.pressure=clamp(c.pressure+.09,0,4);if(c.tile->k=="membrane"&&c.fouling>.32)c.waste+=c.fouling*.002;
  c.dwell=pool.water>.08?std::min(c.dwell+1,24.):std::max(0.,c.dwell-.75);
 }
}
void Game::cascadePass(){
 for(int i=0;i<121;i++){
  auto& c=board[i];auto fs=foods(c);if(fs.empty())continue;
  for(auto p:fs)if(p->sugar>.55&&p->acid>.27&&p->temp>.62&&p->water>.12){double rate=std::min(.40,std::max(0.,p->temp-.50)*(.07+.09*p->acid)*clamp(p->sugar/2,.3,1.4));c.charge=clamp(c.charge+rate*1.75,0,5);c.gas=clamp(c.gas+rate*.95,0,5);c.temp=clamp(c.temp+rate*.13,-3.6,3.6);p->sugar=std::max(0.,p->sugar-rate*.11);if(c.charge>.24&&clock-c.lastCascade>2){c.wave=1;c.lastCascade=clock;flag(i,"ACTIVATION");}}
  if(c.charge>.125){Piece *protein=nullptr,*fatty=nullptr,*salted=nullptr;for(auto p:fs){if(!protein&&p->protein>.54&&p->water>.12)protein=p;if(!fatty&&p->fat>.44&&p->water>.18)fatty=p;if(!salted&&p->salt>1.0&&p->water>.16)salted=p;}
   if(protein&&clock-c.lastCascade>1){double r=std::min(c.charge*.58,.30);protein->brown=clamp(protein->brown+r*.85,0,1.4);c.waste+=std::max(0.,r-.19)*.18;protein->aroma[2]=clamp(protein->aroma[2]+r*.85,0,5);protein->protein=std::max(0.,protein->protein-r*.08);protein->temp=clamp(protein->temp+r*.65,-3.6,3.6);c.temp=clamp(c.temp+r*.40,-3.6,3.6);c.gas=clamp(c.gas+r*.88,0,5);c.charge=std::max(0.,c.charge-r*.60);c.wave=1;c.lastCascade=clock;flag(i,"CHAIN REACTION");}
   else if(fatty&&clock-c.lastCascade>1){double r=std::min(c.charge*.42,.21);fatty->foam=clamp(fatty->foam+r*1.65,0,1.2);fatty->emulsion=clamp(fatty->emulsion+r*.85,0,1.1);fatty->aroma[1]=clamp(fatty->aroma[1]+r*.72,0,5);c.gas=clamp(c.gas+r*.68,0,5);c.charge=std::max(0.,c.charge-r*.46);c.wave=1;c.lastCascade=clock;flag(i,"FAT AERATION");}
   else if(salted&&clock-c.lastCascade>1){double r=std::min(c.charge*.38,.22);for(auto& p:c.stack)if(p.k=="gel"){salted->gel=clamp(salted->gel+r*.8,0,1.1);break;}c.charge=std::max(0.,c.charge-r*.92);c.gas=clamp(c.gas+r*.25,0,5);c.wave=1;c.lastCascade=clock;flag(i,"SALINE QUENCH");}
   if(c.solution.water>.025){double moved=std::min(.10,c.charge*.09);c.charge-=moved;c.solution.activation+=moved;}
  }
  c.pressure=clamp(c.pressure+c.gas*.09+std::max(0.,c.temp)*c.humidity*.015,0,4);bool sealed=false;for(auto& v:c.stack)if((v.k=="bell"||v.k=="gel")&&!v.spent)sealed=true;
  if(c.pressure>(sealed?.51:.92)&&c.gas>.25&&clock-c.lastPressure>2){c.pressure=std::max(0.,c.pressure-.72);c.gas=std::max(0.,c.gas-.38);c.ruptures++;c.waste+=.11+c.pressure*.06;c.charge=clamp(c.charge+.20,0,5);c.wave=c.burst=1;c.lastPressure=clock;for(auto p:fs){p->integrity=clamp(p->integrity-(sealed?.23:.08),.1,1);p->aroma[2]=clamp(p->aroma[2]+.14,0,5);}flag(i,"PRESSURE RELEASE");}
 }
}
void Game::simulate(){
 clock++;events.clear();board=diffusionPass();
 for(int i=0;i<121;i++){
  auto& c=board[i];if(c.tile&&c.tile->k=="hearth")c.energy+=.08;else if(c.tile&&c.tile->k=="ice")c.energy+=.07;if(c.stack.empty())continue;
  for(int z=0;z<int(c.stack.size());z++){
   auto& cap=c.stack[z];if(cap.k=="pulse"&&!cap.spent){int sign=(clock/2)%2==0?1:-1;for(int t=0;t<z;t++)c.stack[t].temp=clamp(c.stack[t].temp+sign*(.40/(1+.15*(z-t))),-3.6,3.6);}
   if(cap.k=="bell"&&!cap.spent){double sum=0;for(int t=0;t<z;t++)if(c.stack[t].kind=="food")sum+=sumA(c.stack[t].aroma);cap.pressure=clamp(cap.pressure+sum*.028,0,5);
    if(cap.pressure>1.40||cap.age>8){cap.spent=true;c.burst=1;for(int k=0;k<4;k++){double released=0;for(int t=0;t<z;t++){auto& p=c.stack[t];if(p.kind=="food"){double d=p.aroma[k]*.38;p.aroma[k]-=d;released+=d;}}c.aroma[k]=clamp(c.aroma[k]+released,0,5);}flag(i,"CLOCHE BURST");}}
  }
  for(int z=0;z<int(c.stack.size());z++){
   auto& p=c.stack[z];if(p.kind!="food")continue;auto was=p.phase;double oldBrown=p.brown,oldSplit=p.split;const Piece* lid=z+1<int(c.stack.size())?&c.stack[z+1]:nullptr;std::string k=lid&&!lid->spent?lid->k:"";
   if(k=="gel"&&p.water>.20){p.gel=clamp(p.gel+.29*clamp(1.55-p.acid*.15,.55,1.5),0,1.1);p.integrity=std::max(p.integrity,.55);}else p.gel=clamp(p.gel-.023-.032*std::max(0.,p.temp-.2),0,1.1);
   if(k=="foam"&&p.water>.22){for(int t=z-1;t>=0;t--)if(c.stack[t].kind=="food"){auto& lower=c.stack[t];double transfer=std::min(lower.fat*.14,.17);lower.fat-=transfer;p.fat+=transfer;double proteinTransfer=std::min(lower.protein*.036,.055);lower.protein-=proteinTransfer;p.protein+=proteinTransfer;break;}
    double hold=.16+.15*clamp(p.protein,0,1.7);p.foam=clamp(p.foam+hold-.09*p.fat-.02*std::max(0.,p.temp),0,1.2);if(p.water>.28&&p.fat>.33&&p.protein>.25&&p.acid<1.8)p.emulsion=clamp(p.emulsion+.27,0,1.1);
   }else p.foam=std::max(0.,p.foam-.065-.035*std::max(0.,p.temp));
   if(k=="crisp"){p.water=std::max(0.,p.water-.115);p.crisp=clamp(p.crisp+.17,0,1.1);}else if(p.water>.63)p.crisp=clamp(p.crisp-(p.water-.6)*.14,0,1.1);
   p.ice=clamp(p.ice-(p.temp>-.25?.20*(p.temp+1):0),0,1.1);
   if(p.sugar>.35&&p.temp>1.0&&p.water<.96){double d=.065*(p.temp-1)*clamp(1.35-p.water,.25,1.1);p.sugar-=d;p.bitter+=d*.12;p.brown+=d*2.5;p.aroma[2]+=d*.95;}
   if(p.protein>.38&&p.temp>.94&&p.water<1.02){double d=.055*(p.temp-.9)*clamp(1.3-p.water,.2,1.1);p.protein-=d*.17;p.brown+=d*2.1;p.aroma[2]+=d*.80;}
   if(p.acid>.64&&p.fat>.40&&p.temp>.25){double d=.06*(p.acid-.55)*(p.temp+.2);p.split+=d;p.emulsion=std::max(0.,p.emulsion-d*2);p.integrity-=d*.08;}
   if(p.temp<-.90&&p.water>.21){p.ice+=.24*(-.70-p.temp);p.foam=std::max(0.,p.foam-.06);}
   if(p.temp>.65&&p.water>.05)p.water-=.014*(p.temp+.2);
   if(p.temp>1.95&&p.brown>.32){double d=.07*(p.temp-1.9);p.scorch+=d;p.bitter+=d*.3;p.integrity-=d*.2;}
   if(p.temp>1.47){double excess=p.temp-1.47;c.waste+=excess*.008*p.water;p.integrity=clamp(p.integrity-excess*.006,.1,1);}if(p.scorch>.64)c.waste+=.015*(p.scorch-.64);
   if(c.pressure>1.35&&p.foam>.2){p.foam=std::max(0.,p.foam-.06);c.waste+=.005;}
   if(p.temp>1.2&&p.gel>.12)p.gel=clamp(p.gel-.11*(p.temp-.8),0,1.1);if(p.temp>1.4)p.foam=clamp(p.foam-.16*(p.temp-1.2),0,1.1);
   if(p.acid>1.7&&p.gel>.2)p.gel=clamp(p.gel-.08*(p.acid-1.4),0,1.1);if(p.split>.4)p.emulsion=clamp(p.emulsion-.12,0,1.1);if(p.temp>1.5&&p.water<.48)p.crisp=clamp(p.crisp+.07,0,1.1);
   for(auto& key:materialKeys)p.at(key)=clamp(p.at(key),0,key=="water"?2:4.5);for(auto& ar:p.aroma)ar=clamp(ar,0,5);
   p.gel=clamp(p.gel,0,1.1);p.foam=clamp(p.foam,0,1.1);p.ice=clamp(p.ice,0,1.1);p.integrity=clamp(p.integrity,.1,1);
   if(oldBrown<.28&&p.brown>=.28)flag(i,p.protein>.4?"BROWNING":"CARAMEL");if(oldSplit<.45&&p.split>=.45)flag(i,"CURDLING");
   p.phase=p.scorch>.82?"char":p.split>.56?"split":p.ice>.57?"crystal":p.gel>.62?(p.salt>.95?"pearls":"gel"):p.emulsion>.6?"emulsion":p.foam>.55?"foam":p.crisp>.54?"crisp":"liquid";
   if(was!=p.phase&&p.phase!="liquid")flag(i,upper(p.phase));else if(was!=p.phase&&was!="liquid")flag(i,upper(was)+" LOST");
  }
 }
 cascadePass();
}
} // namespace aftertaste
