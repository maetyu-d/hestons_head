#include "Interface.h"
#include "Power.h"
#include "PowerArt.h"
#include <algorithm>
#include <cmath>
namespace aftertaste {
namespace {
float wireLength(Vector2 a,Vector2 b){return std::abs(b.x-a.x)+std::abs(b.y-a.y);}
Vector2 wirePoint(const std::vector<Vector2>& path,float fraction){
 float length=0;for(size_t n=1;n<path.size();n++)length+=wireLength(path[n-1],path[n]);float distance=std::clamp(fraction,0.f,1.f)*length;
 for(size_t n=1;n<path.size();n++){float segment=wireLength(path[n-1],path[n]);if(distance<=segment&&segment>.001f){float t=distance/segment;return {path[n-1].x+(path[n].x-path[n-1].x)*t,path[n-1].y+(path[n].y-path[n-1].y)*t};}distance-=segment;}
 return path.back();
}
void squareWireStroke(const std::vector<Vector2>& path,float width,Color color,float from=0,float to=1){
 float length=0;for(size_t n=1;n<path.size();n++)length+=wireLength(path[n-1],path[n]);float start=from*length,end=to*length,at=0;
 for(size_t n=1;n<path.size();n++){Vector2 a=path[n-1],b=path[n];float segment=wireLength(a,b);if(segment>.001f){float low=std::max(0.f,start-at),high=std::min(segment,end-at);if(high>low){Vector2 p{a.x+(b.x-a.x)*low/segment,a.y+(b.y-a.y)*low/segment},q{a.x+(b.x-a.x)*high/segment,a.y+(b.y-a.y)*high/segment};DrawLineEx(p,q,width,color);}}at+=segment;
  if(n+1<path.size()&&at>start&&at<end)DrawRectangleRec({b.x-width*.5f,b.y-width*.5f,width,width},color);
 }
}
}
void Interface::drawEnergyRoutes(){
 const float tile=38*factoryZoom;if(tile<8)return;
 const double clock=factory.tick+(factory.running?factoryAccumulator/.55:0);
 using E=EnergyMedium;using Key=std::pair<Position,E>;
 const Color floor{224,234,232,255};
 auto blend=[](Color a,Color b,float t){return Color{static_cast<unsigned char>(a.r+(b.r-a.r)*t),static_cast<unsigned char>(a.g+(b.g-a.g)*t),static_cast<unsigned char>(a.b+(b.b-a.b)*t),255};};
 std::set<E> focused;bool equipmentFocus=false;
 if(energyLinkSelected){focused.insert(selectedEnergyMedium);equipmentFocus=true;}
 else if(auto m=factory.at(factorySelected);m&&isEnergyMachine(m->kind)){for(auto port:energyPorts(*m))focused.insert(port.medium);equipmentFocus=true;}
 if(!equipmentFocus)focused={E::AC,E::DC,E::Shaft};
 auto visible=[&](E medium){return energyRoutesAll||focused.count(medium);};
 auto terminal=[&](Position p,E medium,int side)->std::optional<Vector2>{
  auto m=factory.at(p);if(!m||!isEnergyMachine(m->kind))return {};
  Machine unrotated;unrotated.kind=m->kind;auto ports=energyPorts(unrotated);
  for(size_t n=0;n<ports.size();n++){auto port=ports[n];if(port.medium!=medium||(port.direction+m->dir)%4!=side)continue;float shift=0;for(size_t j=0;j<ports.size();j++)if(j!=n&&ports[j].direction==port.direction)shift=j<n?.07f:-.07f;
   float u=port.direction==0?.96f:port.direction==2?.04f:.5f+shift,v=port.direction==1?.96f:port.direction==3?.04f:.5f+shift;
   for(int turn=0;turn<m->dir;turn++){float old=u;u=1-v;v=old;}auto r=factoryTile(p);float pad=std::max(1.f,tile*.065f);return Vector2{r.x+pad+u*(tile-2*pad),r.y+pad+v*(tile-2*pad)};
  }return {};
 };
 struct Rail {Vector2 point;const PowerLink* link;int degree=0;};
 std::map<Key,Rail> rails;
 for(const auto& [p,links]:factory.energyLinks)for(const auto& link:links)if(visible(link.medium)){auto r=factoryTile(p);rails.emplace(Key{p,link.medium},Rail{{r.x+tile*.5f,r.y+tile*.5f},&link,0});}
 // Align whole straight runs with their real terminal, rather than inserting a
 // tiny dogleg at each machine. Horizontal and vertical rails solve independently.
 const std::array<float,EnergyCount> lane={0,-.08f,0,.06f,-.06f,.10f,-.10f,.08f,-.08f,.12f};
 for(int axis=0;axis<2;axis++){std::set<Key> seen;int mask=axis?10:5;
  for(const auto& [key,rail]:rails){if(!(rail.link->ports&mask)||seen.count(key))continue;
   std::vector<Key> group{key};seen.insert(key);bool shared=false;std::optional<float> anchor;
   for(size_t n=0;n<group.size();n++){auto current=group[n];const auto& item=rails.at(current);
    auto layers=factory.energyLinks.find(current.first);if(layers!=factory.energyLinks.end())for(const auto& other:layers->second)if(other.medium!=current.second&&visible(other.medium)&&(other.ports&mask))shared=true;
    for(int d:axis?std::array<int,2>{1,3}:std::array<int,2>{0,2}){if(!(item.link->ports&(1<<d)))continue;Position q=nextPosition(current.first,d);auto port=terminal(q,current.second,(d+2)%4);if(port&&!anchor)anchor=axis?port->x:port->y;
     Key next{q,current.second};auto neighbor=rails.find(next);if(neighbor!=rails.end()&&(neighbor->second.link->ports&(1<<((d+2)%4)))&&seen.insert(next).second)group.push_back(next);
    }
   }
   float coordinate=axis?rail.point.x:rail.point.y;coordinate+=shared?lane[size_t(key.second)]*tile:0;if(anchor)coordinate=*anchor;
   for(auto member:group){if(axis)rails.at(member).point.x=coordinate;else rails.at(member).point.y=coordinate;}
  }
 }
 struct Edge {std::vector<Vector2> path;E medium;const PowerLink* link;bool reverse=false,boot=false;double flow=0;};
 std::vector<Edge> edges;
 auto pathBetween=[&](Vector2 a,Vector2 b,int d,Position p){if(a.x==b.x||a.y==b.y)return std::vector<Vector2>{a,b};auto r=factoryTile(p);if(d%2==0){float x=r.x+(d==0?tile:0);return std::vector<Vector2>{a,{x,a.y},{x,b.y},b};}float y=r.y+(d==1?tile:0);return std::vector<Vector2>{a,{a.x,y},{b.x,y},b};};
 for(auto& [key,rail]:rails){Position p=key.first;E medium=key.second;
  for(int d=0;d<4;d++){if(!(rail.link->ports&(1<<d)))continue;Position q=nextPosition(p,d);auto port=terminal(q,medium,(d+2)%4);Key other{q,medium};auto neighbor=rails.find(other);
   if(port){rail.degree++;edges.push_back({pathBetween(rail.point,*port,d,p),medium,rail.link,false,true,rail.link->dir==d?rail.link->flow:0});}
   else if(neighbor!=rails.end()&&(neighbor->second.link->ports&(1<<((d+2)%4)))){rail.degree++;if(q<p)continue;const PowerLink* flow=rail.link;bool reverse=false;double rate=rail.link->dir==d?rail.link->flow:0;if(neighbor->second.link->flow>0&&neighbor->second.link->dir==(d+2)%4){flow=neighbor->second.link;reverse=true;rate=flow->flow;}edges.push_back({pathBetween(rail.point,neighbor->second.point,d,p),medium,flow,reverse,false,rate});}
   else if(medium==E::AC){auto consumer=factory.at(q);if(consumer&&!isEnergyMachine(consumer->kind)&&machineSpec(consumer->kind).power>0&&!factory.energyLink(q,medium)){auto r=factoryTile(q);rail.degree++;edges.push_back({pathBetween(rail.point,{r.x+tile*.5f,r.y+tile*.5f},d,p),medium,rail.link,false,true,rail.link->dir==d?rail.link->flow:0});}}
  }
 }
 for(const auto& [p,m]:factory.machines)if(isEnergyMachine(m.kind))for(auto port:energyPorts(m)){if(!visible(port.medium)||(port.direction!=0&&port.direction!=1))continue;Position q=nextPosition(p,port.direction);auto a=terminal(p,port.medium,port.direction),b=terminal(q,port.medium,(port.direction+2)%4);if(a&&b)edges.push_back({pathBetween(*a,*b,port.direction,p),port.medium,nullptr,false,true});}
 auto width=[&](E medium){return std::clamp(.95f+tile*.021f,1.25f,medium==E::Shaft?3.f:2.65f);};
 auto accent=[&](E medium){Color c=energyColor(medium);return blend(c,Color{100,125,123,255},.42f);};
 // Cable tray, pale inset, restrained conductor. Opaque colours keep joints even
 // instead of darkening every point where separate line fragments overlap.
 for(int pass=0;pass<3;pass++){
  for(const auto& edge:edges){float w=width(edge.medium);bool enabled=!edge.link||edge.link->enabled;Color outer=blend(floor,Color{77,108,112,255},equipmentFocus?.65f:.43f),inset{235,242,238,255},core=blend(floor,accent(edge.medium),equipmentFocus?.9f:.60f);if(!enabled)core=blend(floor,Color{125,143,139,255},.6f);if(edge.link&&edge.link->tripped)core=Color{199,104,81,255};
   squareWireStroke(edge.path,pass==0?w:pass==1?std::max(.7f,w-1.f):std::max(.6f,w*.26f),pass==0?outer:pass==1?inset:core);
  }
  for(const auto& [key,rail]:rails)if(rail.degree>=2){float w=width(key.second),size=rail.degree>2?w+1.7f:w;Color color=pass==0?blend(floor,Color{77,108,112,255},equipmentFocus?.65f:.43f):pass==1?Color{235,242,238,255}:blend(floor,accent(key.second),equipmentFocus?.9f:.60f);if(pass==1)size=std::max(.7f,size-1.f);if(pass==2)size=rail.degree>2?std::max(1.f,w*.55f):std::max(.6f,w*.26f);DrawRectangleRec({rail.point.x-size*.5f,rail.point.y-size*.5f,size,size},color);}
 }
 for(const auto& edge:edges){float w=width(edge.medium);
  if(edge.boot&&tile>=26){Vector2 end=edge.path.back(),prior=edge.path[edge.path.size()-2];float length=wireLength(prior,end),extent=std::min(length,std::clamp(tile*.06f,2.f,5.f));if(length>.001f){Vector2 start{end.x+(prior.x-end.x)*extent/length,end.y+(prior.y-end.y)*extent/length};squareWireStroke({start,end},w+1,Color{93,124,126,255});squareWireStroke({start,end},std::max(.8f,w-.4f),accent(edge.medium));}}
  if(edge.medium==E::Shaft&&tile>=38){auto mid=wirePoint(edge.path,.5f),ahead=wirePoint(edge.path,.51f);bool horizontal=std::abs(ahead.x-mid.x)>=std::abs(ahead.y-mid.y);Rectangle collar{mid.x-(horizontal?1.1f:w),mid.y-(horizontal?w:1.1f),horizontal?2.2f:w*2,horizontal?w*2:2.2f};DrawRectangleRec(collar,Color{120,146,146,255});}
  if(floorFlow&&edge.link&&edge.link->enabled&&!edge.link->tripped&&edge.flow>.001){auto path=edge.path;if(edge.reverse)std::reverse(path.begin(),path.end());float phase=float(std::fmod(clock*(.6+std::min(1.,edge.flow/edge.link->rating)),1.));squareWireStroke(path,std::max(.8f,w*.42f),blend(accent(edge.medium),Color{255,238,183,255},.55f),std::max(0.f,phase-.12f),phase);}
 }
 for(const auto& [key,rail]:rails)if(rail.link->tripped||rail.link->heat>.65){DrawRectangleRec({rail.point.x-2,rail.point.y-2,4,4},Color{220,94,63,255});}
 if(equipmentFocus&&!energyRoutesAll){std::string label=focused.size()==1?upper(energyName(*focused.begin())):"SELECTED EQUIPMENT SERVICES";text(label+" / isolated service view",factoryView.x+10,factoryView.y+30,8.5f,Color{87,113,121,255},true);}
}
void Interface::drawPowerInspector(Rectangle r){
 const Color ink{23,48,57,255},muted{87,113,121,255},paper{237,243,241,255},line{199,215,216,255},orange{240,83,43,255},green{38,133,111,255};DrawRectangleRec(r,WHITE);DrawRectangleLinesEx(r,1,line);float x=r.x+12,y=r.y+12,w=r.width-24;auto m=factory.at(factorySelected);auto link=energyLinkSelected?factory.energyLink(factorySelected,selectedEnergyMedium):nullptr;
 text(link?upper(energyName(link->medium)):m?upper(machineSpec(m->kind).name):"POWER ENGINEERING",x,y,11,ink,true);y+=23;
 text("TILE "+positionName(factorySelected),x,y,9.5f,muted);y+=20;
 Rectangle art{x,y,w,160};DrawRectangleRounded(art,.08f,5,Color{245,248,246,255});MachineAppearance look;look.kind=link?energyConduit(link->medium):m?m->kind:MachineKind::PowerCable;look.direction=m?m->dir:0;look.active=factory.running&&(link?link->enabled&&!link->tripped&&link->flow>.001:m&&m->enabled&&!m->power.tripped&&(m->power.flow>.001||m->power.generated>.001||m->power.rpm>20));look.closed=link?link->tripped:m&&!m->enabled;look.time=factory.tick+(factory.running?factoryAccumulator/.55:0);if(m){double stored=0;for(auto v:m->power.store)stored+=v;look.progress=stored/std::max(1.,m->power.capacity);}else if(link)look.progress=link->flow/link->rating;
 drawMachineHousing({art.x+5,art.y+5,145,145},look);drawMachineMechanism({art.x+5,art.y+5,145,145},look);float tx=x+154,tw=w-160;if(tw>30){text(link?"LIVE CONDUIT":"LIVE POWER",tx,y+13,8.5f,muted,true);text(fit(link?(link->tripped?"TRIPPED":link->enabled?"CONNECTED":"ISOLATED"):m?m->status:"",tw,10,true),tx,y+34,10,link&&link->tripped?orange:green,true);text(link?roundTo(link->flow,1)+" / "+roundTo(link->rating,0):m?roundTo(m->power.generated,1)+" GEN":"",tx,y+62,11,ink,true);text(link?"HEAT "+roundTo(link->heat*100,0)+"%":m?roundTo(m->power.delivered,1)+" OUT":"",tx,y+83,9,muted);}
 y+=170;
 inspectorBody={x,y,w,r.y+r.height-y-14};inspectorScroll=std::clamp(inspectorScroll,0.f,std::max(0.f,inspectorHeight-inspectorBody.height));beginClip(inspectorBody);y-=inspectorScroll;float start=y;
 button({x,y,w-4,27},energyRoutesAll?"Routes / all services":"Routes / selected services",[this]{energyRoutesAll=!energyRoutesAll;},energyRoutesAll,true,"Show all services, or isolate only the services connected to this equipment",{},10);y+=35;
 auto pair=[&](const std::string& label,const std::string& less,const std::string& more){text(label,x,y+7,10.5f,ink,true);button({x+w-58,y,24,27},"-",[this,less]{if(energyLinkSelected)factory.configureEnergyLink(factorySelected,selectedEnergyMedium,less);else factory.configure(factorySelected,less);},false,true,"Adjust this physical setting",{},12);button({x+w-29,y,24,27},"+",[this,more]{if(energyLinkSelected)factory.configureEnergyLink(factorySelected,selectedEnergyMedium,more);else factory.configure(factorySelected,more);},false,true,"Adjust this physical setting",{},12);y+=34;};
 if(link){button({x,y,w-4,28},"Layer / "+energyName(link->medium),[this]{auto i=factory.energyLinks.find(factorySelected);if(i==factory.energyLinks.end())return;for(size_t n=0;n<i->second.size();n++)if(i->second[n].medium==selectedEnergyMedium){selectedEnergyMedium=i->second[(n+1)%i->second.size()].medium;break;}},false,true,"Cycle conduits sharing this tile",{},10);y+=36;
  pair("Rating "+roundTo(link->rating,0),"rating-","rating+");float bw=(w-10)/2;button({x,y,bw,28},link->enabled?"Isolate":"Reconnect",[this]{factory.configureEnergyLink(factorySelected,selectedEnergyMedium,"enabled");},false,true,"Open or close this route",{},10);button({x+bw+5,y,bw,28},"Reset trip",[this]{factory.configureEnergyLink(factorySelected,selectedEnergyMedium,"reset");},false,link->tripped,"Reset after the conductor has cooled",{},10);y+=36;button({x,y,w-4,28},"Reclaim this conduit",[this]{factory.removeEnergyLink(factorySelected,selectedEnergyMedium);energyLinkSelected=false;},false,true,"Remove only this energy layer; floor machines remain in place",orange,10);y+=40;
  y+=paragraph("Different media cross without joining. Matching conduits join at visible junctions. Electrical routes have a finite rating and accumulate overload heat; a tripped conductor carries no power.",x,y,w,10.5f,muted)+12;
 }else if(m){
  y+=paragraph(m->status,x,y,w,11,green,true)+8;pair("Output rate "+roundTo(m->power.rate,0),"power-rate-","power-rate+");pair("Throttle "+roundTo(m->power.throttle*100,0)+"%","power-throttle-","power-throttle+");
  if(m->kind==MachineKind::BatteryRack||m->kind==MachineKind::Flywheel||m->kind==MachineKind::SteamAccumulator||m->kind==MachineKind::GasHolder||m->kind==MachineKind::HeatTank)pair("Capacity "+roundTo(m->power.capacity,0),"power-capacity-","power-capacity+");
  float bw=(w-10)/2;button({x,y,bw,28},m->enabled?"Isolate":"Enable",[this]{factory.configure(factorySelected,"enabled");},false,true,"Open this machine's connections",{},10);button({x+bw+5,y,bw,28},"Rotate ports",[this]{factory.rotate(factorySelected);},false,true,"Rotate all physical ports",{},10);y+=36;
  if(m->power.tripped){button({x,y,w-4,28},"Reset breaker",[this]{factory.configure(factorySelected,"power-reset");},false,true,"Reset after overload heat has dissipated",orange,10);y+=35;}
  if(m->kind==MachineKind::GasEngine||m->kind==MachineKind::BiomassBoiler){button({x,y,w-4,28},m->automatic?"Dispatch / reserve relay":"Dispatch / continuous",[this]{factory.configure(factorySelected,"automatic");},m->automatic,true,"Choose relay-controlled reserve or continuous generation",{},10);y+=35;}
  if(m->kind==MachineKind::PowerRelay){pair("Start below "+roundTo(m->power.low*100,0)+"%","relay-low-","relay-low+");pair("Stop above "+roundTo(m->power.high*100,0)+"%","relay-high-","relay-high+");}
  if(m->kind==MachineKind::Gearbox)pair("Gear ratio "+roundTo(m->shear,1),"shear-","shear+");
  if(m->kind==MachineKind::Flywheel){button({x,y,w-4,28},"Hand crank / +5 shaft energy",[this]{factory.configure(factorySelected,"power-crank");},false,true,"Supply explicit external work for a black start",{},10);y+=36;}
  if(m->kind==MachineKind::HeatTank){button({x,y,w-4,28},m->forward?"Heat bypass / open":"Heat bypass / closed",[this,forward=m->forward]{factory.configure(factorySelected,forward?"mode_hold":"mode_forward");},m->forward,true,"Vent heat above the tank's working reserve to avoid a blocked recovery loop",{},10);y+=36;}
  if(m->kind==MachineKind::GridInlet){text("IMPORT RESERVE / "+roundTo(factory.electricity,0)+" units",x,y,10,ink,true);y+=20;button({x,y,w-4,28},"Deliver import reserve +2500",[this]{factory.receiveElectricity();},false,true,"Optional external electricity delivery; the inlet's output rate still limits imports",{},10);y+=36;}
  text("PHYSICAL PORTS",x,y,10,ink,true);y+=20;static const std::array<const char*,4> names={"E","S","W","N"};for(auto port:energyPorts(*m)){Position q=nextPosition(factorySelected,port.direction);auto cable=factory.energyLink(q,port.medium);bool connected=cable&&(cable->ports&(1<<((port.direction+2)%4)));if(auto adjacent=factory.at(q);adjacent&&isEnergyMachine(adjacent->kind))for(auto other:energyPorts(*adjacent))if(other.medium==port.medium&&other.direction==(port.direction+2)%4)connected=true;text(fit(std::string(port.output?"OUT ":"IN ")+names[size_t(port.direction)]+" / "+energyName(port.medium)+(connected?" / linked":" / unconnected"),w,9.5f,true),x,y,9.5f,connected?energyColor(port.medium):orange,true);y+=18;}
  y+=9;text("ACTUAL STORES",x,y,10,ink,true);y+=20;for(size_t n=0;n<EnergyCount;n++){if(m->power.store[n]<=.001)continue;text(energyName(EnergyMedium(n))+" / "+roundTo(m->power.store[n],2),x,y,9.5f,energyColor(EnergyMedium(n)),true);y+=16;float fraction=float(std::clamp(m->power.store[n]/std::max(1.,m->power.capacity),0.,1.));DrawRectangleRec({x,y,w-4,4},paper);DrawRectangleRec({x,y,(w-4)*fraction,4},energyColor(EnergyMedium(n)));y+=13;}
  if(m->item){text("REAL FEED / "+roundTo(analyse(m->item->food).mass,1)+" g",x,y,10,ink,true);y+=19;}if(m->sideOutput){text("SIDE / "+m->sideOutput->residue+" / "+roundTo(analyse(m->sideOutput->food).mass,1)+" g",x,y,10,orange,true);y+=19;}
  y+=paragraph(machineSpec(m->kind).description,x,y,w,10.5f,muted)+12;
  if(m->power.history.size()>1){text("MEASURED HISTORY",x,y,9.5f,muted,true);y+=18;float top=y,high=float(std::max(1.,*std::max_element(m->power.history.begin(),m->power.history.end())));DrawRectangleRec({x,top,w-4,56},paper);for(size_t n=1;n<m->power.history.size();n++){float a=float(n-1)/(m->power.history.size()-1),b=float(n)/(m->power.history.size()-1);DrawLineEx({x+a*(w-4),top+53-float(m->power.history[n-1])/high*50},{x+b*(w-4),top+53-float(m->power.history[n])/high*50},1.6f,green);}y+=67;}
 }
 y+=10;text("CONNECTED FACTORY GRID",x,y,10,ink,true);y+=22;text("GEN "+roundTo(factory.grid.generated,1)+" / REQUEST "+roundTo(factory.grid.requested,1),x,y,10,muted);y+=20;text("DELIVERED "+roundTo(factory.grid.delivered,1)+" / STORED "+roundTo(factory.grid.stored,1),x,y,10,muted);y+=20;text("HEAT/COLD RECOVERY "+roundTo(factory.grid.recoveredHeat,1),x,y,9.5f,muted);y+=20;text("SUN "+roundTo(solarDaylight(factory.tick)*100,0)+"% / WIND "+roundTo(windStrength(factory.tick)*100,0)+"%",x,y,9.5f,muted);y+=30;
 inspectorHeight=y-start;endClip();
}
}
