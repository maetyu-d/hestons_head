#include "Interface.h"
#include "Power.h"
#include "PowerArt.h"
#include <algorithm>
#include <cmath>
namespace aftertaste {
void Interface::drawEnergyRoutes(){
 const float tile=38*factoryZoom;if(tile<6)return;
 const double clock=factory.tick+(factory.running?factoryAccumulator/.55:0);
 std::set<EnergyMedium> focused;
 if(!energyRoutesAll){if(energyLinkSelected)focused.insert(selectedEnergyMedium);else if(auto m=factory.at(factorySelected);m&&isEnergyMachine(m->kind))for(auto port:energyPorts(*m))focused.insert(port.medium);}
 // Offset both axes: parallel pipes stay separate on vertical as well as horizontal runs.
 const std::array<Vector2,EnergyCount> lanes={Vector2{-.12f,-.12f},{-.08f,-.08f},{0,0},{.04f,-.04f},{.08f,.08f},{-.04f,.12f},{.12f,-.08f},{-.12f,.12f},{.16f,0},{0,-.16f}};
 auto center=[&](Position p,EnergyMedium medium){auto r=factoryTile(p);auto lane=lanes[size_t(medium)];return Vector2{r.x+tile*(.5f+lane.x),r.y+tile*(.5f+lane.y)};};
 auto terminal=[&](Position p,EnergyMedium medium,int side)->std::optional<Vector2>{
  auto m=factory.at(p);if(!m||!isEnergyMachine(m->kind))return {};
  Machine unrotated;unrotated.kind=m->kind;auto ports=energyPorts(unrotated);
  for(size_t n=0;n<ports.size();n++){auto port=ports[n];if(port.medium!=medium||(port.direction+m->dir)%4!=side)continue;float shift=0;for(size_t j=0;j<ports.size();j++)if(j!=n&&ports[j].direction==port.direction)shift=j<n?.07f:-.07f;
   float u=port.direction==0?.96f:port.direction==2?.04f:.5f+shift,v=port.direction==1?.96f:port.direction==3?.04f:.5f+shift;
   for(int turn=0;turn<m->dir;turn++){float old=u;u=1-v;v=old;}auto r=factoryTile(p);float pad=std::max(1.f,tile*.065f);return Vector2{r.x+pad+u*(tile-2*pad),r.y+pad+v*(tile-2*pad)};
  }return {};
 };
 for(const auto& [p,links]:factory.energyLinks){auto full=factoryTile(p);if(!CheckCollisionRecs(full,factoryView))continue;
  for(const auto& link:links){const bool selected=focused.count(link.medium),dimmed=!focused.empty()&&!selected;float alpha=dimmed?.08f:selected?.82f:floorFlow?.34f:.20f;if(!link.enabled||link.tripped)alpha*=.42f;
   Color color=energyColor(link.medium);Vector2 c=center(p,link.medium);float thickness=std::clamp(tile*(selected?.041f:.026f),1.f,selected?3.2f:2.f);if(dimmed)thickness=std::max(.7f,thickness*.7f);
   std::vector<std::pair<int,Vector2>> ends;
   for(int d=0;d<4;d++){if(!(link.ports&(1<<d)))continue;Position q=nextPosition(p,d);auto neighbor=factory.energyLink(q,link.medium);auto port=terminal(q,link.medium,(d+2)%4);
    if(port)ends.push_back({d,*port});else if(neighbor&&(neighbor->ports&(1<<((d+2)%4)))){auto nc=center(q,link.medium);ends.push_back({d,{(c.x+nc.x)*.5f,(c.y+nc.y)*.5f}});}else if(link.medium==EnergyMedium::AC){auto consumer=factory.at(q);if(consumer&&!isEnergyMachine(consumer->kind)&&machineSpec(consumer->kind).power>0&&!factory.energyLink(q,link.medium)){auto qr=factoryTile(q);ends.push_back({d,{qr.x+tile*.5f,qr.y+tile*.5f}});}}
   }
   for(auto [d,end]:ends){DrawLineEx(c,end,thickness,Fade(color,alpha));if(link.medium==EnergyMedium::Shaft&&tile>=30&&!dimmed){auto delta=directions[d];Vector2 mid{(c.x+end.x)*.5f,(c.y+end.y)*.5f};DrawLineEx({mid.x-delta[1]*tile*.04f,mid.y+delta[0]*tile*.04f},{mid.x+delta[1]*tile*.04f,mid.y-delta[0]*tile*.04f},1,Fade(color,alpha));}}
   // Small round elbows; only genuine three-way or four-way joins receive a junction dot.
   if(ends.size()==2)DrawCircleV(c,thickness*.5f,Fade(color,alpha));
   if(ends.size()>2){DrawCircleV(c,thickness*1.4f,Fade(Color{244,248,245,255},alpha));DrawCircleV(c,thickness*.9f,Fade(color,alpha));}
   if(ends.empty())DrawCircleLines(int(c.x),int(c.y),std::max(1.5f,thickness),Fade(color,alpha));
   if(floorFlow&&!dimmed&&link.enabled&&!link.tripped&&link.flow>.001){auto out=std::find_if(ends.begin(),ends.end(),[&](auto e){return e.first==link.dir;});if(out!=ends.end()){float phase=float(std::fmod(clock*(.6+std::min(1.,link.flow/link.rating)),1.));Vector2 a{c.x+(out->second.x-c.x)*std::max(0.f,phase-.23f),c.y+(out->second.y-c.y)*std::max(0.f,phase-.23f)},b{c.x+(out->second.x-c.x)*phase,c.y+(out->second.y-c.y)*phase};DrawLineEx(a,b,thickness+1,Fade(color,selected?1.f:.78f));}}
   if(link.tripped||link.heat>.65){DrawCircleV(c,std::max(2.f,tile*.045f),Color{235,83,49,255});if(tile>=40&&!dimmed)text(link.tripped?"TRIP":"HOT",c.x+3,c.y-9,7,Color{203,64,42,255},true);}
  }
 }
 // Directly adjoining terminals, such as relay and switchboard, also have real connections.
 for(const auto& [p,m]:factory.machines){if(!isEnergyMachine(m.kind))continue;for(auto port:energyPorts(m)){if(port.direction!=0&&port.direction!=1)continue;Position q=nextPosition(p,port.direction);auto a=terminal(p,port.medium,port.direction),b=terminal(q,port.medium,(port.direction+2)%4);if(a&&b){bool dimmed=!focused.empty()&&!focused.count(port.medium);DrawLineEx(*a,*b,std::max(1.f,tile*.027f),Fade(energyColor(port.medium),dimmed?.08f:.6f));}}}
 if(!focused.empty()){std::string label=focused.size()==1?upper(energyName(*focused.begin())):"SELECTED EQUIPMENT SERVICES";text(label+" / other services dimmed",factoryView.x+10,factoryView.y+30,8.5f,Color{87,113,121,255},true);}
}
void Interface::drawPowerInspector(Rectangle r){
 const Color ink{23,48,57,255},muted{87,113,121,255},paper{237,243,241,255},line{199,215,216,255},orange{240,83,43,255},green{38,133,111,255};DrawRectangleRec(r,WHITE);DrawRectangleLinesEx(r,1,line);float x=r.x+12,y=r.y+12,w=r.width-24;auto m=factory.at(factorySelected);auto link=energyLinkSelected?factory.energyLink(factorySelected,selectedEnergyMedium):nullptr;
 text(link?upper(energyName(link->medium)):m?upper(machineSpec(m->kind).name):"POWER ENGINEERING",x,y,11,ink,true);y+=23;
 text("TILE "+positionName(factorySelected),x,y,9.5f,muted);y+=20;
 Rectangle art{x,y,w,160};DrawRectangleRounded(art,.08f,5,Color{245,248,246,255});MachineAppearance look;look.kind=link?energyConduit(link->medium):m?m->kind:MachineKind::PowerCable;look.direction=m?m->dir:0;look.active=factory.running&&(link?link->enabled&&!link->tripped&&link->flow>.001:m&&m->enabled&&!m->power.tripped&&(m->power.flow>.001||m->power.generated>.001||m->power.rpm>20));look.closed=link?link->tripped:m&&!m->enabled;look.time=factory.tick+(factory.running?factoryAccumulator/.55:0);if(m){double stored=0;for(auto v:m->power.store)stored+=v;look.progress=stored/std::max(1.,m->power.capacity);}else if(link)look.progress=link->flow/link->rating;
 drawMachineHousing({art.x+5,art.y+5,145,145},look);drawMachineMechanism({art.x+5,art.y+5,145,145},look);float tx=x+154,tw=w-160;if(tw>30){text(link?"LIVE CONDUIT":"LIVE POWER",tx,y+13,8.5f,muted,true);text(fit(link?(link->tripped?"TRIPPED":link->enabled?"CONNECTED":"ISOLATED"):m?m->status:"",tw,10,true),tx,y+34,10,link&&link->tripped?orange:green,true);text(link?roundTo(link->flow,1)+" / "+roundTo(link->rating,0):m?roundTo(m->power.generated,1)+" GEN":"",tx,y+62,11,ink,true);text(link?"HEAT "+roundTo(link->heat*100,0)+"%":m?roundTo(m->power.delivered,1)+" OUT":"",tx,y+83,9,muted);}
 y+=170;
 inspectorBody={x,y,w,r.y+r.height-y-14};inspectorScroll=std::clamp(inspectorScroll,0.f,std::max(0.f,inspectorHeight-inspectorBody.height));beginClip(inspectorBody);y-=inspectorScroll;float start=y;
 button({x,y,w-4,27},energyRoutesAll?"Routes / all services":"Routes / focus this equipment",[this]{energyRoutesAll=!energyRoutesAll;},energyRoutesAll,true,"Show all service routes, or focus the selected equipment and dim the rest",{},10);y+=35;
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
